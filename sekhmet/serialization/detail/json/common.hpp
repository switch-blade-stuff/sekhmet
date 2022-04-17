//
// Created by switchblade on 2022-04-14.
//

#pragma once

#include <bit>
#include <cstdlib>
#include <cstring>

#include "../manipulators.hpp"
#include "../serialization.hpp"
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
		class read_frame;
		class parse_event_handler;

	private:
		using mem_res_type = std::pmr::memory_resource;
		using sv_type = std::basic_string_view<CharType>;

		template<std::size_t PageSize>
		struct pool_allocator
		{
			struct page_header
			{
				page_header *previous; /* Previous pages are not used for allocation. */
				std::size_t page_size; /* Total size of the page in bytes. */
				std::size_t used_size; /* Amount of data used in bytes. */

				/* Page data follows the header. */
			};

			constexpr static auto page_size_mult = PageSize;

			pool_allocator(const pool_allocator &) = delete;
			pool_allocator &operator=(const pool_allocator &) = delete;

			constexpr pool_allocator() noexcept = default;
			constexpr pool_allocator(pool_allocator &&other) noexcept
				: main_page(std::exchange(other.main_page, nullptr))
			{
			}
			constexpr pool_allocator &operator=(pool_allocator &&other) noexcept
			{
				swap(other);
				return *this;
			}

			void release(mem_res_type *upstream)
			{
				for (auto *page = main_page; page != nullptr;) page = release_page(upstream, page);
				main_page = nullptr;
			}
			void *allocate(mem_res_type *upstream, std::size_t n)
			{
				/* Allocate on a new page. */
				void *result;
				if (auto new_used = n; !main_page || (new_used += main_page->used_size) > main_page->page_size) [[unlikely]]
					result = alloc_new_page(upstream, n);
				else
				{
					result = static_cast<void *>(page_data(main_page) + main_page->used_size);
					main_page->used_size = new_used;
				}
				return result;
			}
			void *reallocate(mem_res_type *upstream, void *old, std::size_t old_n, std::size_t n)
			{
				if (!old) [[unlikely]]
					return allocate(upstream, n);

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
					goto use_new_page;
				}

				/* Allocate new block & memcpy. */
				void *result;
				if (new_used > main_page->page_size) [[unlikely]]
				{
				use_new_page:
					result = alloc_new_page(upstream, new_used);
				}
				else
				{
					result = static_cast<void *>(main_page_bytes + main_page->used_size);
					main_page->used_size = new_used;
				}
				std::memcpy(result, old, old_n);
				return result;
			}

			void *alloc_new_page(mem_res_type *upstream, std::size_t n)
			{
				auto page_size = n + sizeof(page_header);
				auto rem = page_size % page_size_mult;
				page_size = page_size - rem + (rem ? page_size_mult : 0);

				auto new_page = insert_page(upstream, page_size);
				new_page->used_size = n;
				if (!new_page) [[unlikely]]
					return nullptr;
				return static_cast<void *>(page_data(new_page));
			}
			page_header *release_page(mem_res_type *upstream, page_header *page_ptr)
			{
				auto previous = page_ptr->previous;
				upstream->deallocate(page_ptr, sizeof(page_header) + page_ptr->page_size);
				return previous;
			}
			page_header *insert_page(mem_res_type *upstream, std::size_t n)
			{
				auto result = static_cast<page_header *>(upstream->allocate(n));
				if (!result) [[unlikely]]
					return nullptr;
				/* If the previous main page is empty, deallocate it immediately. */
				if (main_page && !main_page->used_size) [[unlikely]]
					result->previous = release_page(upstream, main_page);
				else
					result->previous = main_page;
				result->page_size = 0;
				return main_page = result;
			}
			constexpr std::byte *page_data(page_header *header) noexcept
			{
				return std::bit_cast<std::byte *>(header) + sizeof(page_header);
			}

			constexpr void swap(pool_allocator &other) noexcept { std::swap(main_page, other.main_page); }

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
			const CharType *data;
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
			friend class parse_event_handler;

			static void throw_string_error() { throw archive_error("Invalid Json type, expected string"); }

		public:
			json_entry() = delete;
			json_entry(const json_entry &) = delete;
			json_entry &operator=(const json_entry &) = delete;
			json_entry(json_entry &&) = delete;
			json_entry &operator=(json_entry &&) = delete;

			bool try_read(std::nullptr_t) const noexcept { return type == entry_type::NULL_ENTRY; }
			const json_entry &read(std::nullptr_t) const
			{
				if (!try_read(nullptr)) [[unlikely]]
					throw archive_error("Invalid Json type, expected null");
				return *this;
			}
			bool try_read(bool &b) const noexcept
			{
				if (type & entry_type::BOOL) [[likely]]
				{
					b = type & 1;
					return true;
				}
				else
					return false;
			}
			const json_entry &read(bool &b) const
			{
				if (!try_read(b)) [[unlikely]]
					throw archive_error("Invalid Json type, expected bool");
				return *this;
			}
			bool try_read(char &c) const noexcept
			{
				if (type == entry_type::CHAR) [[likely]]
				{
					c = literal.character;
					return true;
				}
				else
					return false;
			}
			const json_entry &read(char &c) const
			{
				if (!try_read(c)) [[unlikely]]
					throw archive_error("Invalid Json type, expected char");
				return *this;
			}
			template<typename I>
			bool try_read(I &value) const noexcept requires(std::integral<I> || std::floating_point<I>)
			{
				switch (type)
				{
					case entry_type::INT: value = static_cast<I>(literal.integer); return true;
					case entry_type::FLOAT: value = static_cast<I>(literal.floating); return true;
					default: return false;
				}
			}
			template<typename I>
			const json_entry &read(I &value) const requires(std::integral<I> || std::floating_point<I>)
			{
				if (!try_read(value)) [[unlikely]]
					throw archive_error("Invalid Json type, expected number");
				return *this;
			}

			bool try_read(std::basic_string<CharType> &value) const
			{
				if (type == entry_type::STRING) [[likely]]
				{
					value.assign(string.data, string.size);
					return true;
				}
				else
					return false;
			}
			const json_entry &read(std::basic_string<CharType> &value) const
			{
				if (!try_read(value)) [[unlikely]]
					throw_string_error();
				return *this;
			}
			bool try_read(sv_type &value) const noexcept
			{
				if (type == entry_type::STRING) [[likely]]
				{
					value = sv_type{string.data, string.size};
					return true;
				}
				else
					return false;
			}
			const json_entry &read(sv_type &value) const
			{
				if (!try_read(value)) [[unlikely]]
					throw_string_error();
				return *this;
			}
			template<std::output_iterator<CharType> I>
			bool try_read(I &value) const
			{
				if (type == entry_type::STRING) [[likely]]
				{
					std::copy_n(string.data, string.size, value);
					return true;
				}
				else
					return false;
			}
			template<std::output_iterator<CharType> I>
			const json_entry &read(I &value) const
			{
				if (!try_read(value)) [[unlikely]]
					throw_string_error();
				return *this;
			}
			template<std::output_iterator<CharType> I, std::sentinel_for<I> S>
			bool try_read(I &value, S &sent) const
			{
				if (type == entry_type::STRING) [[likely]]
				{
					for (std::size_t i = 0; i != string.size && value != sent; ++i, ++value) *value = string.data[i];
					return true;
				}
				else
					return false;
			}
			template<std::output_iterator<CharType> I, std::sentinel_for<I> S>
			const json_entry &read(I &value, S &sent) const
			{
				if (!try_read(value, sent)) [[unlikely]]
					throw_string_error();
				return *this;
			}

			template<typename T>
			const json_entry &read(T &&value) const;
			template<typename T>
			bool try_read(T &&value) const
			{
				try
				{
					read(std::forward<T>(value));
					return true;
				}
				catch (archive_error &)
				{
					return false;
				}
			}
			template<typename T>
			const json_entry &operator>>(T &&value) const
			{
				return read(std::forward<T>(value));
			}
			template<std::default_initializable T>
			T read() const
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

		enum read_frame_type : int
		{
			ARRAY_FRAME = entry_type::ARRAY,   /* Parsing/reading array. */
			OBJECT_FRAME = entry_type::OBJECT, /* Parsing/reading object. */
		};

	public:
		/** @brief Iterator providing read-only access to a Json entry. */
		class entry_iterator
		{
			friend class json_input_archive_base;
			friend class read_frame;

		public:
			typedef json_entry value_type;
			typedef const json_entry *pointer;
			typedef const json_entry &reference;
			typedef std::ptrdiff_t difference_type;

			typedef std::random_access_iterator_tag iterator_category;

		private:
			constexpr entry_iterator(const void *ptr, read_frame_type type) noexcept : ptr_value(ptr), type(type) {}

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
				SEK_ASSERT(a.type == b.type);
				switch (a.type)
				{
					case read_frame_type::ARRAY_FRAME: return a.array_element - b.array_element;
					case read_frame_type::OBJECT_FRAME: return a.object_element - b.object_element;
					default: return 0;
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
					case read_frame_type::ARRAY_FRAME: return array_element;
					case read_frame_type::OBJECT_FRAME: return &object_element->value;
					default: return nullptr;
				}
			}
			constexpr void move_n(difference_type n) noexcept
			{
				switch (type)
				{
					case read_frame_type::ARRAY_FRAME: array_element += n; break;
					case read_frame_type::OBJECT_FRAME: object_element += n; break;
					default: break;
				}
			}

			union
			{
				/** Pointer used for type-agnostic operations. */
				const void *ptr_value = nullptr;
				/** Pointer into an array container. */
				json_entry *array_element;
				/** Pointer into an object container. */
				member_t *object_element;
			};
			/** Type of the frame this iterator was created from. */
			read_frame_type type;
		};

		/** @brief Helper structure used as the API interface for Json input archive operations. */
		class read_frame
		{
			friend class json_input_archive_base;

		public:
			typedef input_archive_category archive_category;

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
			constexpr explicit read_frame(const json_entry *entry) noexcept
				: type(static_cast<read_frame_type>(entry->type))
			{
				switch (type)
				{
					case read_frame_type::ARRAY_FRAME:
						array = array_view{
							entry->array.data_begin,
							entry->array.data_end,
							entry->array.data_begin,
						};
						break;
					case read_frame_type::OBJECT_FRAME:
						object = object_view{
							entry->object.members_begin,
							entry->object.members_end,
							entry->object.members_begin,
						};
						break;
					default: break;
				}
			}

		public:
			/** Returns iterator to the first entry of the currently read object or array. */
			[[nodiscard]] constexpr entry_iterator begin() const noexcept { return {raw.begin_ptr, type}; }
			/** @copydoc begin */
			[[nodiscard]] constexpr entry_iterator cbegin() const noexcept { return begin(); }
			/** Returns iterator one past the last entry of the currently read object or array. */
			[[nodiscard]] constexpr entry_iterator end() const noexcept { return {raw.end_ptr, type}; }
			/** @copydoc end */
			[[nodiscard]] constexpr entry_iterator cend() const noexcept { return end(); }

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

			/** Attempts to deserialize the next Json entry of the archive & advance the entry.
			 * @param value Value to deserialize from the Json entry.
			 * @return true if deserialization was successful, false otherwise. */
			template<typename T>
			bool try_read(T &&value)
			{
				entry_iterator current{raw.current_ptr, type};
				if (current->try_read(std::forward<T>(value))) [[likely]]
				{
					raw.current_ptr = (current + 1).ptr_value;
					return true;
				}
				else
					return false;
			}
			/** Deserializes the next Json entry of the archive & advance the entry.
			 * @param value Value to deserialize from the Json entry.
			 * @return Reference to this frame.
			 * @throw archive_exception On deserialization errors. */
			template<typename T>
			read_frame &read(T &&value)
			{
				entry_iterator current{raw.current_ptr, type};
				current->read(std::forward<T>(value));
				raw.current_ptr = (current + 1).ptr_value;
				return *this;
			}
			/** @copydoc read */
			template<typename T>
			read_frame &operator>>(T &&value)
			{
				return read(std::forward<T>(value));
			}
			/** Deserializes an instance of `T` from the next Json entry of the archive.
			 * @return Deserialized instance of `T`.
			 * @throw archive_error On deserialization errors. */
			template<std::default_initializable T>
			T read()
			{
				T result;
				read(result);
				return result;
			}

			template<typename T>
			bool try_read(named_entry<T> value) requires std::is_reference_v<T>
			{
				if (type == read_frame_type::OBJECT_FRAME) [[likely]]
				{
					if (seek_entry(value.name)) [[likely]]
						return try_read(std::forward<T>(value.value));
				}
				return false;
			}
			template<typename T>
			read_frame &read(named_entry<T> value) requires std::is_reference_v<T>
			{
				if (type != read_frame_type::OBJECT_FRAME) [[unlikely]]
					throw archive_error("Invalid Json type, expected object");

				if (!seek_entry(value.name)) [[unlikely]]
				{
					std::string err{"Invalid Json object member \""};
					err.append(value.name);
					err.append(1, '\"');
					throw std::out_of_range(err);
				}
				else
					read(std::forward<T>(value.value));
				return *this;
			}
			template<typename T>
			read_frame &operator>>(named_entry<T> mod) requires std::is_reference_v<T>
			{
				return read(std::forward<named_entry<T>>(mod));
			}

			template<typename I>
			bool try_read(array_entry<I> mod) noexcept
			{
				mod.value = static_cast<std::decay_t<I>>(size());
				return true;
			}
			template<typename I>
			read_frame &read(array_entry<I> mod) noexcept
			{
				try_read(mod);
				return *this;
			}
			template<typename I>
			read_frame &operator>>(array_entry<I> mod) noexcept
			{
				return read(mod);
			}

			template<typename I>
			bool try_read(object_entry<I> mod) noexcept
			{
				mod.value = static_cast<I>(size());
				return true;
			}
			template<typename I>
			read_frame &read(object_entry<I> mod) noexcept
			{
				try_read(mod);
				return *this;
			}
			template<typename I>
			read_frame &operator>>(object_entry<I> mod) noexcept
			{
				return read(mod);
			}

			template<typename T>
			bool try_read(binary_entry<T> mod) noexcept requires std::same_as<CharType, char>
			{
				auto sv = read<sv_type>();
				return base64_decode(&mod.data, sizeof(std::decay_t<decltype(mod.data)>), sv.data(), sv.size());
			}
			template<typename T>
			read_frame &read(binary_entry<T> mod) noexcept
			{
				if (!try_read(mod)) [[unlikely]]
					throw archive_error("Failed to decode base64 data");
				return *this;
			}
			template<typename T>
			read_frame &operator>>(binary_entry<T> mod) noexcept
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

			constexpr const json_entry *seek_entry(sv_type key) noexcept
			{
				if (auto member_ptr = search_entry(key); member_ptr) [[likely]]
				{
					object.current_ptr = member_ptr;
					return &member_ptr->value;
				}
				else
					return nullptr;
			}
			constexpr const member_t *search_entry(sv_type key) const noexcept
			{
				for (auto member = object.begin_ptr; member < object.end_ptr; ++member)
					if (key == sv_type{member->key.data, member->key.size}) return member;
				return nullptr;
			}

			union
			{
				raw_ptrs raw = {};
				array_view array;
				object_view object;
			};

			read_frame_type type;
		};

		/** @brief Temporary event handler used to parse Json input. */
		class parse_event_handler
		{
			enum parse_state : int
			{
				EXPECT_ARRAY_VALUE,
				EXPECT_OBJECT_VALUE,
				EXPECT_OBJECT_KEY,
			};

			/** @brief Frame used for container parsing. */
			struct parse_frame
			{
				/* Pointer to the actual container being parsed. */
				union
				{
					void *container_ptr = nullptr;
					array_t *array_ptr;
					object_t *object_ptr;
				};
				/* Pointer to container's data. */
				union
				{
					void *data_ptr = nullptr;
					json_entry *array_data;
					member_t *object_data;
				};

				std::size_t current_capacity = 0; /* Current amortized capacity of the container. */
				std::size_t current_size = 0;	  /* Current size of the container. */

				parse_state state;
			};

		public:
			constexpr explicit parse_event_handler(json_input_archive_base *parent) noexcept
				: parent(parent), parse_stack(parent->upstream_alloc)
			{
			}

			template<std::integral S>
			[[nodiscard]] CharType *on_string_alloc(S len) const
			{
				return parent->alloc_string(static_cast<std::size_t>(len));
			}

			bool on_null() const
			{
				return on_value([](auto &entry) { entry.type = entry_type::NULL_ENTRY; });
			}
			bool on_bool(bool b) const
			{
				return on_value([b](auto &entry) { entry.type = entry_type::BOOL | (b ? 1 : 0); });
			}
			bool on_true() const
			{
				return on_value([](auto &entry) { entry.type = entry_type::BOOL_TRUE; });
			}
			bool on_false() const
			{
				return on_value([](auto &entry) { entry.type = entry_type::BOOL_FALSE; });
			}
			bool on_char(CharType c) const
			{
				return on_value(
					[c](auto &entry)
					{
						entry.type = entry_type::CHAR;
						entry.literal.character = c;
					});
			}
			template<std::integral I>
			bool on_int(I i) const
			{
				return on_value(
					[i](auto &entry)
					{
						entry.type = entry_type::INT;
						entry.literal.integer = static_cast<std::uint64_t>(i);
					});
			}
			template<std::floating_point F>
			bool on_float(F f) const
			{
				return on_value(
					[f](auto &entry)
					{
						entry.type = entry_type::FLOAT;
						entry.literal.floating = static_cast<double>(f);
					});
			}

			template<std::integral S>
			bool on_string(const CharType *str, S len) const
			{
				return on_value(
					[&](auto &entry)
					{
						entry.type = entry_type::STRING;
						entry.string = string_t{str, static_cast<std::size_t>(len)};
					});
			}
			template<std::integral S>
			bool on_string_copy(const CharType *str, S len) const
			{
				auto dest = on_string_alloc(len);
				*std::copy_n(str, static_cast<std::size_t>(len), dest) = '\0';
				return on_string(str, len);
			}

			bool on_object_start(std::size_t n = 0)
			{
				auto do_start_object = [&](json_entry &entry)
				{
					enter_frame();
					entry.type = entry_type::OBJECT;
					current->object_ptr = &entry.object;
					current->state = parse_state::EXPECT_OBJECT_KEY;
					if (n) resize_container<member_t>(n);
				};

				if (!parent->top_level) [[unlikely]] /* This is the top-level object. */
				{
					do_start_object(parent->alloc_top_level());
					return true;
				}
				else
					return on_value(do_start_object);
			}
			template<std::integral S>
			bool on_object_key(const CharType *str, S len)
			{
				switch (current->state)
				{
					case parse_state::EXPECT_OBJECT_KEY:
					{
						push_element<member_t>().key = string_t{str, static_cast<std::size_t>(len)};
						current->state = parse_state::EXPECT_OBJECT_VALUE; /* Always expect value after key. */
						return true;
					}
					default: return false;
				}
			}
			template<std::integral S>
			bool on_object_key_copy(const CharType *str, S len)
			{
				auto dest = on_string_alloc(len);
				*std::copy_n(str, static_cast<std::size_t>(len), dest) = '\0';
				return on_object_key(dest, len);
			}
			template<std::integral S>
			bool on_object_end(S size)
			{
				switch (current->state)
				{
					case parse_state::EXPECT_OBJECT_KEY:
					{
						auto *obj = current->object_ptr;
						obj->members_begin = current->object_data;
						obj->members_end = current->object_data + static_cast<std::size_t>(size);
						exit_frame();
						return true;
					}
					default: return false;
				}
			}

			bool on_array_start(std::size_t n = 0)
			{
				auto do_start_array = [&](json_entry &entry)
				{
					enter_frame();
					entry.type = entry_type::ARRAY;
					current->array_ptr = &entry.array;
					current->state = parse_state::EXPECT_ARRAY_VALUE;
					if (n) resize_container<json_entry>(n);
				};

				if (!parent->top_level) [[unlikely]] /* This is the top-level array. */
				{
					do_start_array(parent->alloc_top_level());
					return true;
				}
				else
					return on_value(do_start_array);
			}
			template<std::integral S>
			bool on_array_end(S size)
			{
				switch (current->state)
				{
					case parse_state::EXPECT_ARRAY_VALUE:
					{
						auto *arr = current->array_ptr;
						arr->data_begin = current->array_data;
						arr->data_end = current->array_data + static_cast<std::size_t>(size);
						exit_frame();
						return true;
					}
					default: return false;
				}
			}

		private:
			template<typename T>
			void resize_container(std::size_t n) const
			{
				current->data_ptr = parent->realloc_data<T>(current->data_ptr, current->current_capacity, n);
				current->current_capacity = n;
			}
			template<typename T>
			[[nodiscard]] T &push_element() const
			{
				auto next_idx = current->current_size;
				if (current->current_capacity == current->current_size++)
					resize_container<T>(current->current_size * 2);
				return static_cast<T *>(current->data_ptr)[next_idx];
			}

			bool on_value(auto f) const
			{
				json_entry *entry;
				switch (current->state)
				{
					case parse_state::EXPECT_ARRAY_VALUE:
					{
						entry = &push_element<json_entry>();
						break;
					}
					case parse_state::EXPECT_OBJECT_VALUE:
					{
						/* Size is updated by the key event. */
						entry = &(current->object_data[current->current_size - 1].value);
						current->state = parse_state::EXPECT_OBJECT_KEY;
						break;
					}
					default: return false;
				}

				f(*entry);
				return true;
			}
			void enter_frame() { current = &parse_stack.emplace_back(); }
			void exit_frame()
			{
				parse_stack.pop_back();
				current = &parse_stack.back();
			}

			json_input_archive_base *parent;
			std::pmr::vector<parse_frame> parse_stack;
			parse_frame *current = nullptr;
		};

	public:
		json_input_archive_base(const json_input_archive_base &) = delete;
		json_input_archive_base &operator=(const json_input_archive_base &) = delete;

		json_input_archive_base() : upstream_alloc(std::pmr::get_default_resource()) {}
		constexpr explicit json_input_archive_base(mem_res_type *res) : upstream_alloc(res) {}
		constexpr json_input_archive_base(json_input_archive_base &&other) noexcept
			: top_level(std::exchange(other.top_level, nullptr)),
			  upstream_alloc(std::exchange(other.upstream_alloc, nullptr)),
			  data_pool(std::move(other.data_pool)),
			  string_pool(std::move(other.string_pool))
		{
		}
		constexpr json_input_archive_base &operator=(json_input_archive_base &&other) noexcept
		{
			swap(other);
			return *this;
		}
		~json_input_archive_base() { destroy(); }

		template<typename T>
		bool do_try_read(T &&value) const
		{
			return top_level->try_read(std::forward<T>(value));
		}
		template<typename T>
		void do_read(T &&value) const
		{
			top_level->read(std::forward<T>(value));
		}

		void reset()
		{
			destroy();
			top_level = nullptr;
		}
		void reset(mem_res_type *res)
		{
			reset();
			upstream_alloc = res;
		}

		constexpr void swap(json_input_archive_base &other) noexcept
		{
			std::swap(top_level, other.top_level);
			std::swap(upstream_alloc, other.upstream_alloc);
			data_pool.swap(other.data_pool);
			string_pool.swap(other.string_pool);
		}

	private:
		json_entry &alloc_top_level()
		{
			top_level = static_cast<json_entry *>(data_pool.allocate(upstream_alloc, sizeof(json_entry)));
			if (!top_level) [[unlikely]]
				throw std::bad_alloc();
			return *top_level;
		}
		template<typename T>
		T *realloc_data(void *data, std::size_t old_cap, std::size_t new_cap)
		{
			auto *new_data = data_pool.reallocate(upstream_alloc, data, old_cap * sizeof(T), new_cap * sizeof(T));
			if (!new_data) [[unlikely]]
				throw std::bad_alloc();
			return static_cast<T *>(new_data);
		}
		CharType *alloc_string(std::size_t n)
		{
			auto result = static_cast<CharType *>(string_pool.allocate(upstream_alloc, (n + 1) * sizeof(CharType)));
			if (!result) [[unlikely]]
				throw std::bad_alloc();
			return result;
		}

		void destroy()
		{
			data_pool.release(upstream_alloc);
			string_pool.release(upstream_alloc);
		}

		json_entry *top_level = nullptr;						/* Top-most entry of the Json tree. */
		mem_res_type *upstream_alloc;							/* Upstream allocator used for memory pools. */
		pool_allocator<sizeof(json_entry) * 64> data_pool = {}; /* Allocation pool used for entry allocation. */
		pool_allocator<SEK_KB(1)> string_pool = {};				/* Allocation pool used for string allocation. */
	};

	template<typename C>
	template<typename T>
	const typename json_input_archive_base<C>::json_entry &json_input_archive_base<C>::json_entry::read(T &&value) const
	{
		if (!(type & entry_type::CONTAINER)) [[unlikely]]
			throw archive_error("Invalid Json type, expected array or object");

		read_frame frame{this};
		detail::invoke_deserialize(std::forward<T>(value), frame);
		return *this;
	}
}	 // namespace sek::serialization::detail