//
// Created by switchblade on 2022-04-14.
//

#pragma once

#include <bit>
#include <cstdlib>
#include <cstring>

#include "../manipulators.hpp"
#include "sekhmet/detail/assert.hpp"
#include "sekhmet/detail/define.h"
#include "sekhmet/detail/hash.hpp"
#include <memory_resource>

namespace sek::serialization::detail
{
	template<typename CharType = char>
	class json_input_archive_base
	{
	public:
		class json_entry;
		class entry_iterator;
		class archive_frame;

	private:
		using sv_type = std::basic_string_view<CharType>;

		struct pool_allocator
		{
			struct page_header
			{
				const page_header *previous; /* Previous pages are not used for allocation. */
				std::size_t page_size;		 /* Total size of the page in bytes. */
				std::size_t used_size;		 /* Amount of data used in bytes. */

				/* Page data follows the header. */
			};

			constexpr static auto page_size_mult = SEK_KB(1);

			constexpr pool_allocator(std::pmr::memory_resource *res) noexcept : upstream(res) {}

			void release()
			{
				for (auto *page = main_page; page != nullptr;)
				{
					auto previous = page->previous;
					upstream->deallocate(page, sizeof(page_header) + page->page_size);
					page = previous;
				}
				main_page = nullptr;
			}
			void *allocate(std::size_t n)
			{
				/* Allocate on a new page. */
				void *result;
				if (auto new_used = n; !main_page || (new_used += main_page->used_size) > main_page->page_size) [[unlikely]]
				{
					auto new_page = insert_page(new_used * 2, new_used);
					if (!new_page) [[unlikely]]
						return nullptr;
					result = page_data(new_page);
				}
				else
				{
					result = static_cast<void *>(page_data(main_page) + main_page->used_size);
					main_page->used_size = new_used;
				}
				return result;
			}
			void *reallocate(void *old, std::size_t old_n, std::size_t n)
			{
				if (!old) [[unlikely]]
					return allocate(n);

				/* Do nothing if new size is less or same. */
				if (n <= old_n) [[unlikely]]
					return old;

				/* If there is old data, main page should exist. */
				auto main_page_bytes = page_data(main_page);
				auto new_used = main_page->used_size + n;

				/* Try to expand if old data is the top allocation and there is enough space for it. */
				auto old_bytes = static_cast<std::byte *>(old);
				if (old_bytes + old_n == main_page_bytes + main_page->used_size) [[likely]]
				{
					if (new_used -= old_n; new_used <= main_page->page_size) [[likely]]
					{
						main_page->used_size = new_used;
						return old;
					}
					else
						goto use_new_page;
				}

				/* Allocate new block & memcpy. */
				void *result;
				if (new_used > main_page->page_size) [[unlikely]]
				{
				use_new_page:
					auto new_page = insert_page(new_used * 2, new_used);
					if (!new_page) [[unlikely]]
						return nullptr;
					result = page_data(new_page);
				}
				else
				{
					result = static_cast<void *>(main_page_bytes + main_page->used_size);
					main_page->used_size = new_used;
				}
				memcpy(result, old, old_n);
				return result;
			}

			page_header *insert_page(std::size_t n, std::size_t used)
			{
				auto total_size = n + sizeof(page_header);
				auto rem = total_size % page_size_mult;
				total_size = total_size - rem + (rem ? page_size_mult : 0);

				auto result = static_cast<page_header *>(upstream->allocate(total_size));
				if (!result) [[unlikely]]
					return nullptr;
				result->previous = main_page;
				result->page_size = 0;
				result->used_size = used;
				return main_page = result;
			}
			constexpr std::byte *page_data(page_header *header) noexcept
			{
				return std::bit_cast<std::byte *>(header) + sizeof(page_header);
			}

			std::pmr::memory_resource *upstream;
			page_header *main_page = nullptr;
		};

		typedef int entry_flags;

		enum entry_type : int
		{
			NULL_ENTRY = 0,
			CHAR = 1,
			INT = 2,
			FLOAT = 3,
			STRING = 4,

			BOOL = 8,
			BOOL_FALSE = BOOL | 0,
			BOOL_TRUE = BOOL | 1,

			CONTAINER = 16,
			ARRAY = CONTAINER | 0,
			OBJECT = CONTAINER | 1,
		};

		union literal_t
		{
			char character;
			std::uint64_t integer;
			double floating;
		};
		struct string_t
		{
			CharType *data;
			std::size_t size;
		};
		struct array_t
		{
			json_entry *data_begin;
			json_entry *data_end;
		};
		struct member_t;
		struct object_t
		{
			member_t *members_begin;
			member_t *members_end;
		};

	public:
		/** @brief Structure used to represent a Json entry. */
		class json_entry
		{
			friend class json_input_archive_base;

		public:
			json_entry() = delete;
			json_entry(const json_entry &) = delete;
			json_entry &operator=(const json_entry &) = delete;
			json_entry(json_entry &&) = delete;
			json_entry &operator=(json_entry &&) = delete;

			constexpr bool try_read(std::nullptr_t) noexcept { return type == entry_type::NULL_ENTRY; }
			constexpr archive_frame &read(std::nullptr_t) noexcept
			{
				if (!try_read(nullptr)) [[unlikely]]
				{ /* TODO: Throw archive type exception. */
				}
				return *this;
			}
			constexpr bool try_read(bool &b) noexcept
			{
				if (type & entry_type::BOOL) [[likely]]
				{
					b = type & 1;
					return true;
				}
				else
					return false;
			}
			constexpr archive_frame &read(bool &b) noexcept
			{
				if (!try_read(b)) [[unlikely]]
					throw archive_error("Invalid Json type, expected boolean");
				return *this;
			}
			constexpr bool try_read(char &c) noexcept
			{
				if (type == entry_type::CHAR) [[likely]]
				{
					c = literal.character;
					return true;
				}
				else
					return false;
			}
			constexpr archive_frame &read(char &c) noexcept
			{
				if (!try_read(c)) [[unlikely]]
					throw archive_error("Invalid Json type, expected character");
				return *this;
			}
			template<typename I>
			constexpr bool try_read(I &value) noexcept requires(std::integral<I> || std::floating_point<I>)
			{
				switch (type)
				{
					case entry_type::INT: value = static_cast<I>(literal.integer); return true;
					case entry_type::FLOAT: value = static_cast<I>(literal.floating); return true;
					default: return false;
				}
			}
			template<typename I>
			constexpr archive_frame &read(I &value) noexcept requires(std::integral<I> || std::floating_point<I>)
			{
				if (!try_read(value)) [[unlikely]]
					throw archive_error("Invalid Json type, expected number");
				return *this;
			}

			constexpr bool try_read(std::basic_string<CharType> &value) noexcept
			{
				if (type == entry_type::STRING) [[likely]]
				{
					value.assign(string.data, string.size);
					return true;
				}
				else
					return false;
			}
			constexpr archive_frame &read(std::basic_string<CharType> &value) noexcept
			{
				if (!try_read(value)) [[unlikely]]
					throw archive_error("Invalid Json type, expected string");
				return *this;
			}
			constexpr bool try_read(sv_type &value) noexcept
			{
				if (type == entry_type::STRING) [[likely]]
				{
					value = sv_type{string.data, string.size};
					return true;
				}
				else
					return false;
			}
			constexpr archive_frame &read(sv_type &value) noexcept
			{
				if (!try_read(value)) [[unlikely]]
					throw archive_error("Invalid Json type, expected string");
				return *this;
			}

			template<typename I>
			constexpr bool try_read(I &value) noexcept
			{
				if (type & entry_type::CONTAINER) [[likely]]
				{
					/* TODO: Enter new archive frame. */
					return true;
				}
				else
					return false;
			}
			template<typename I>
			constexpr archive_frame &read(I &value) noexcept
			{
				if (!(type & entry_type::CONTAINER)) [[unlikely]]
					throw archive_error("Invalid Json type, expected array or object");

				/* TODO: Enter new archive frame. */
				return *this;
			}

			template<typename T>
			constexpr archive_frame &operator>>(T &value) noexcept
			{
				return read(value);
			}
			template<std::default_initializable T>
			constexpr T read() noexcept
			{
				T result;
				read(result);
				return result;
			}

		private:
			union
			{
				literal_t literal;
				string_t string;
				array_t array;
				object_t object;
			};
			entry_type type;
		};

	private:
		struct member_t
		{
			json_entry value;
			string_t key;
		};

		enum frame_type : short
		{
			ARRAY = array_type,	  /* Parsing/reading array. */
			OBJECT = object_type, /* Parsing/reading object. */
		};

		/** @brief Frame used for container parsing. */
		struct parse_frame
		{
			enum container_state : short
			{
				VALUE, /* Expect value. */
				KEY,   /* Expect key. */
			};

			/* Pointer to the actual container being parsed. */
			union
			{
				array_t *array_ptr;
				object_t *object_ptr;
			};
			/* Pointer to container's data. */
			union
			{
				void *data_ptr;
				json_entry *array_data;
				member_t *object_data;
			};

			/* 32-bit integers are used to avoid wasting space on 64-bit.  */
			std::uint32_t current_capacity; /* Current amortized capacity of the container. */
			std::uint32_t current_size;		/* Current size of the container. */

			container_state state;
			frame_type type;
		};

	public:
		/** @brief Iterator providing read-only access to a Json entry. */
		class entry_iterator
		{
			friend class json_input_archive_base;
			friend class archive_frame;

		public:
			typedef json_entry value_type;
			typedef const json_entry *pointer;
			typedef const json_entry &reference;
			typedef std::ptrdiff_t difference_type;

			typedef std::random_access_iterator_tag iterator_category;

		private:
			constexpr entry_iterator(void *ptr, frame_type type) noexcept : ptr_value(ptr), type(type) {}

		public:
			constexpr entry_iterator() noexcept = default;
			constexpr entry_iterator(const entry_iterator &) noexcept = default;
			constexpr entry_iterator &operator=(const entry_iterator &) noexcept = default;
			constexpr entry_iterator(entry_iterator &&) noexcept = default;
			constexpr entry_iterator &operator=(entry_iterator &&) noexcept = default;

			constexpr entry_iterator &operator+=(difference_type n) noexcept
			{
				move_n(n);
				return *this;
			}
			constexpr entry_iterator &operator++() noexcept
			{
				move_n(1);
				return *this;
			}
			constexpr entry_iterator operator++(int) noexcept
			{
				auto temp = *this;
				operator++();
				return temp;
			}
			constexpr entry_iterator &operator-=(difference_type n) noexcept
			{
				move_n(-n);
				return *this;
			}
			constexpr entry_iterator &operator--() noexcept
			{
				move_n(-1);
				return *this;
			}
			constexpr entry_iterator operator--(int) noexcept
			{
				auto temp = *this;
				operator--();
				return temp;
			}

			[[nodiscard]] constexpr entry_iterator operator+(difference_type n) const noexcept
			{
				auto result = *this;
				result.move_n(n);
				return result;
			}
			[[nodiscard]] constexpr entry_iterator operator-(difference_type n) const noexcept
			{
				auto result = *this;
				result.move_n(-n);
				return result;
			}

			/** Returns pointer to the associated entry. */
			[[nodiscard]] constexpr pointer get() const noexcept { return get_entry(); }
			/** @copydoc get */
			[[nodiscard]] constexpr pointer operator->() const noexcept { return get(); }
			/** Returns reference to the associated entry. */
			[[nodiscard]] constexpr reference operator*() const noexcept { return *get(); }
			/** Returns reference to the entry at `n` offset from the iterator. */
			[[nodiscard]] constexpr reference operator[](difference_type n) const noexcept { return get()[n]; }

			[[nodiscard]] friend constexpr difference_type operator-(entry_iterator a, entry_iterator b) noexcept
			{
				SEK_ASSERT(a.type != b.type);
				switch (a.type)
				{
					case frame_type::ARRAY: return a.array_element - b.array_element;
					case frame_type::OBJECT: return a.object_element - b.object_element;
				}
			}
			[[nodiscard]] friend constexpr entry_iterator operator+(difference_type n, entry_iterator a) noexcept
			{
				return a + n;
			}

			[[nodiscard]] constexpr auto operator<=>(const entry_iterator &other) const noexcept
			{
				return ptr_value <=> other.ptr_value;
			}
			[[nodiscard]] constexpr bool operator==(const entry_iterator &other) const noexcept
			{
				return ptr_value == other.ptr_value;
			}

		private:
			[[nodiscard]] constexpr json_entry *get_entry() const noexcept
			{
				switch (type)
				{
					case frame_type::ARRAY: return array_element;
					case frame_type::OBJECT: return &object_element->value;
					default: return nullptr;
				}
			}
			constexpr void move_n(difference_type n) noexcept
			{
				switch (type)
				{
					case frame_type::ARRAY: array_element += n; break;
					case frame_type::OBJECT: object_element += n; break;
				}
			}

			union
			{
				/** Pointer used for type-agnostic operations. */
				void *ptr_value = nullptr;
				/** Pointer into an array container. */
				json_entry *array_element;
				/** Pointer into an object container. */
				member_t *object_element;
			};
			/** Type of the frame this iterator was created from. */
			frame_type type;
		};

		/** @brief Helper structure used as the API interface for Json input archive operations. */
		class archive_frame
		{
			friend class json_input_archive_base;

		public:
			typedef entry_iterator iterator;
			typedef entry_iterator const_iterator;
			typedef typename entry_iterator::value_type value_type;
			typedef typename entry_iterator::pointer pointer;
			typedef typename entry_iterator::pointer const_pointer;
			typedef typename entry_iterator::reference reference;
			typedef typename entry_iterator::reference const_reference;
			typedef typename entry_iterator::difference_type difference_type;
			typedef std::size_t size_type;

		private:
			constexpr explicit archive_frame(json_entry *entry) noexcept
				: type(static_cast<frame_type>(entry->flags & type_mask))
			{
				switch (type)
				{
					case frame_type::ARRAY: array = {entry->array.data_begin, entry->array.data_end}; break;
					case frame_type::OBJECT: array = {entry->object->members_begin, entry->object->members_end}; break;
					default: break;
				}
			}
			constexpr explicit archive_frame(array_t *array) noexcept
				: array{array->data_begin, array->data_end}, type(frame_type::ARRAY)
			{
			}
			constexpr explicit archive_frame(object_t *object) noexcept
				: object{object->members_begin, object->members_end}, type(frame_type::OBJECT)
			{
			}

		public:
			/** Returns iterator to the first entry of the currently read object or array. */
			[[nodiscard]] constexpr entry_iterator begin() const noexcept { return {raw.begin_ptr, type}; }
			/** Returns iterator one past the last entry of the currently read object or array. */
			[[nodiscard]] constexpr entry_iterator end() const noexcept { return {raw.end_ptr, type}; }

			/** Returns reference to the first entry of the currently read object or array. */
			[[nodiscard]] constexpr const_reference front() const noexcept { return *begin(); }
			/** Returns reference to the last entry of the currently read object or array. */
			[[nodiscard]] constexpr const_reference back() const noexcept { return *(begin() - 1); }
			/** Returns reference to the nth entry of the currently read object or array. */
			[[nodiscard]] constexpr const_reference at(size_type i) const noexcept
			{
				return begin()[static_cast<difference_type>(i)];
			}

			/** Checks if the currently read object or array is empty (has no entries). */
			[[nodiscard]] constexpr bool empty() const noexcept { return begin() == end(); }
			/** Returns the size of the currently read object or array (amount of entries). */
			[[nodiscard]] constexpr size_type size() const noexcept { return static_cast<size_type>(end() - begin()); }
			/** Returns the max possible size of an object or array. */
			[[nodiscard]] constexpr size_type max_size() const noexcept
			{
				return static_cast<size_type>(std::numeric_limits<std::uint32_t>::max());
			}

			/* Regular reads are forwarded to the entry. */

			template<typename T>
			constexpr bool try_read(T &value) noexcept
			{
				entry_iterator current{raw.current_ptr, type};
				if (current->try_read(value)) [[likely]]
					raw.current_ptr = (current + 1).ptr_value;
				return *this;
			}
			template<typename T>
			constexpr archive_frame &read(T &value) noexcept
			{
				entry_iterator current{raw.current_ptr, type};
				current->read(value);
				raw.current_ptr = (current + 1).ptr_value;
				return *this;
			}
			template<typename T>
			constexpr archive_frame &operator>>(T &value) noexcept
			{
				return read(value);
			}

			/* Modifier reads are applied directly to the frame. */

			template<typename T>
			constexpr bool try_read(named_entry<T> value) noexcept requires std::is_reference_v<T>
			{
				if (type == frame_type::OBJECT) [[likely]]
				{
					auto entry = seek_entry(value.name);
					if (!entry) [[likely]]
						return entry->try_read(value);
				}
				return false;
			}
			template<typename T>
			constexpr archive_frame &read(named_entry<T> value) noexcept requires std::is_reference_v<T>
			{
				/* TODO: Assert that the frame is an object frame. */

				if (auto entry = seek_entry(value.name); !entry) [[unlikely]]
				{ /* TODO: Throw entry not found exception. */
				}
				else
					entry->read(value);
				return *this;
			}
			template<typename T>
			constexpr archive_frame &operator>>(named_entry<T> mod) noexcept requires std::is_reference_v<T>
			{
				return read(mod);
			}

			template<std::integral I>
			constexpr bool try_read(sequence<I> mod) const noexcept
			{
				mod.value = static_cast<I>(size());
				return true;
			}
			template<std::integral I>
			constexpr archive_frame &read(sequence<I> mod) noexcept
			{
				try_read(mod);
				return *this;
			}
			template<std::integral I>
			constexpr archive_frame &operator>>(sequence<I> mod) noexcept
			{
				return read(mod);
			}

		private:
			struct raw_ptrs
			{
				const void *begin_ptr = nullptr;
				const void *end_ptr = nullptr;
				const void *current_ptr = nullptr;
			};
			struct array_view
			{
				const json_entry *begin_ptr;
				const json_entry *end_ptr;
				const json_entry *current_ptr;
			};
			struct object_view
			{
				const member_t *begin_ptr;
				const member_t *end_ptr;
				const member_t *current_ptr;
			};

			constexpr json_entry *seek_entry(sv_type key) noexcept
			{
				if (auto member_ptr = search_entry(key); member_ptr) [[likely]]
				{
					object.current_ptr = member_ptr;
					return member_ptr->value;
				}
				else
					return nullptr;
			}
			constexpr member_t *search_entry(sv_type key) const noexcept
			{
				for (auto member = object.members_begin; member < object.members_end; ++member)
					if (key == sv_type{member->key.data, member->key.size}) return member;
				return nullptr;
			}

			union
			{
				raw_ptrs raw = {};
				array_view array;
				object_view object;
			};

			frame_type type;
		};

	public:
		json_input_archive_base() : json_input_archive_base(std::pmr::get_default_resource()) {}
		explicit json_input_archive_base(std::pmr::memory_resource *res) : data_pool(res) {}

		void do_parse()
		{ /* TODO: Initialize temporary parser & parse input. */
		}

	private:
		CharType *duplicate_string(const char *str, std::size_t n)
		{
			auto result = static_cast<CharType *>(data_pool.allocate(str, (n + 1) * sizeof(CharType)));
			*std::copy_n(str, n, result) = '\0';
			return result;
		}
		template<typename T>
		void resize_frame_container(parse_frame &frame, std::size_t capacity)
		{
			auto new_bytes = capacity * sizeof(T), old_bytes = frame.current_capacity * sizeof(T);
			auto *new_data = data_pool.reallocate(frame.data_ptr, old_bytes, new_bytes);
			if (!new_data) [[unlikely]]
				throw std::bad_alloc();

			frame.data_ptr = new_data;
			frame.current_capacity = capacity;
		}
		void finalize_array(parse_frame &frame)
		{
			frame.array_ptr->data_begin = frame.array_data;
			frame.array_ptr->data_end = frame.array_data + frame.current_size;
		}
		void finalize_object(parse_frame &frame)
		{
			frame.object_ptr->members_begin = frame.object_data;
			frame.object_ptr->members_end = frame.object_data + frame.current_size;
		}

		pool_allocator data_pool;	   /* Allocation pool used for entry & string data allocation. */
		object_t *top_level = nullptr; /* Top-most object of the Json tree. */
	};
}	 // namespace sek::serialization::detail