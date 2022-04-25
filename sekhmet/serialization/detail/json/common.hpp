//
// Created by switchblade on 2022-04-14.
//

#pragma once

#include "../manipulators.hpp"
#include "../util.hpp"
#include "sekhmet/detail/assert.hpp"
#include "sekhmet/detail/define.h"
#include "sekhmet/detail/pool_resource.hpp"

namespace sek::serialization::detail
{
	using namespace sek::detail;

	typedef int json_archive_config;

	constexpr json_archive_config container_types = 1;
	constexpr json_archive_config char_value = 2;

	template<typename CharType, json_archive_config Config = 0>
	struct json_archive_base
	{
		class entry_t;
		class entry_iterator;
		class read_frame;
		class write_frame;

		struct parser_base;

		enum entry_type : int
		{
			NO_TYPE = 0,
			DYNAMIC = 1,

			BOOL = 2,
			BOOL_FALSE = BOOL | 0,
			BOOL_TRUE = BOOL | 1,

			CONTAINER = 4,
			ARRAY = CONTAINER | 0,
			OBJECT = CONTAINER | 1,

			NULL_VALUE = 8,
			CHAR = 9,
			STRING = 10,

			INT_MASK = 16,
			INT_SIGN_BIT = 32,
			INT_U = INT_MASK,
			INT_S = INT_MASK | INT_SIGN_BIT,
			INT_SIZE_MASK = 0xf,
			INT_8 = 0,
			INT_16 = 1,
			INT_32 = 2,
			INT_64 = 3,

			INT_U8 = INT_U | INT_8,
			INT_U16 = INT_U | INT_16,
			INT_U32 = INT_U | INT_32,
			INT_U64 = INT_U | INT_64,
			INT_S8 = INT_S | INT_8,
			INT_S16 = INT_S | INT_16,
			INT_S32 = INT_S | INT_32,
			INT_S64 = INT_S | INT_64,

			FLOAT_MASK = 1024,
			FLOAT32 = FLOAT_MASK | 0,
			FLOAT64 = FLOAT_MASK | 1,
		};

		union literal_t
		{
			CharType c;

			std::intmax_t si;
			std::uintmax_t ui;

			/* Used for input. */
			double fp;

			/* Used for output. */
			float f32;
			double f64;
		};
		struct member_t;
		struct container_t
		{
			union
			{
				void *data_ptr = nullptr;
				entry_t *array_data;
				member_t *object_data;
			};
			std::size_t size = 0;
			std::size_t capacity = 0;
			entry_type value_type = NO_TYPE;
		};

		/** @brief Structure used to represent a Json entry. */
		class entry_t
		{
			friend struct json_archive_base;
			friend struct member_t;
			friend struct parser_base;

			friend class read_frame;
			friend class write_frame;

			static void throw_string_error() { throw archive_error("Invalid Json type, expected string"); }

			constexpr void swap(entry_t &other) noexcept
			{
				std::swap(container, other.container);
				std::swap(type, other.type);
			}

			/* Must only be accessible from friends. */
			constexpr entry_t() noexcept : container{}, type(NO_TYPE) {}

		public:
			entry_t(const entry_t &) = delete;
			entry_t &operator=(const entry_t &) = delete;
			entry_t(entry_t &&) = delete;
			entry_t &operator=(entry_t &&) = delete;

			/** Reads a null value from the entry. Returns `true` if the entry contains a null value, `false` otherwise. */
			constexpr bool try_read(std::nullptr_t) const noexcept { return type == NULL_VALUE; }
			/** Reads a null value from the entry.
			 * @throw archive_error If the entry does not contain a null value. */
			constexpr const entry_t &read(std::nullptr_t) const
			{
				if (!try_read(nullptr)) [[unlikely]]
					throw archive_error("Invalid Json type, expected null");
				return *this;
			}

			/** Reads a bool from the entry. Returns `true` if the entry contains a bool, `false` otherwise. */
			constexpr bool try_read(bool &b) const noexcept
			{
				if (type & BOOL) [[likely]]
				{
					b = type & 1;
					return true;
				}
				else
					return false;
			}
			/** Reads a bool from the entry.
			 * @throw archive_error If the entry does not contain a bool. */
			constexpr const entry_t &read(bool &b) const
			{
				if (!try_read(b)) [[unlikely]]
					throw archive_error("Invalid Json type, expected bool");
				return *this;
			}

			/** Reads a character from the entry. Returns `true` if the entry contains a character, `false` otherwise. */
			constexpr bool try_read(CharType &c) const noexcept requires((Config & char_value) == char_value)
			{
				if (type == CHAR) [[likely]]
				{
					c = literal.c;
					return true;
				}
				else
					return false;
			}
			/** Reads a character from the entry.
			 * @throw archive_error If the entry does not contain a character. */
			constexpr const entry_t &read(CharType &c) const requires((Config & char_value) == char_value)
			{
				if (!try_read(c)) [[unlikely]]
					throw archive_error("Invalid Json type, expected char");
				return *this;
			}

			/** Reads a number from the entry. Returns `true` if the entry contains a number, `false` otherwise. */
			template<typename I>
			constexpr bool try_read(I &value) const noexcept requires(std::integral<I> || std::floating_point<I>)
			{
				if (type & INT_MASK)
				{
					if (type & INT_SIGN_BIT)
						value = static_cast<I>(literal.si);
					else
						value = static_cast<I>(literal.ui);
					return true;
				}
				else if (type & FLOAT_MASK)
				{
					value = static_cast<I>(literal.fp);
					return true;
				}
				else
					return false;
			}
			/** Reads a number from the entry.
			 * @throw archive_error If the entry does not contain a number. */
			template<typename I>
			constexpr const entry_t &read(I &value) const requires(std::integral<I> || std::floating_point<I>)
			{
				if (!try_read(value)) [[unlikely]]
					throw archive_error("Invalid Json type, expected number");
				return *this;
			}

			/** Reads a string from the entry.
			 * @param value STL string to assign the string value to.
			 * @copydoc `true` if the entry contains a string, `false` otherwise. */
			constexpr bool try_read(std::basic_string<CharType> &value) const
			{
				if (type == STRING) [[likely]]
				{
					value.assign(string);
					return true;
				}
				else
					return false;
			}
			/** Reads a string from the entry.
			 * @param value STL string view to assign the string value to.
			 * @copydoc `true` if the entry contains a string, `false` otherwise. */
			constexpr bool try_read(std::basic_string_view<CharType> &value) const noexcept
			{
				if (type == STRING) [[likely]]
				{
					value = string;
					return true;
				}
				else
					return false;
			}
			/** Reads a string from the entry.
			 * @param value Output iterator used to write the string value to.
			 * @copydoc `true` if the entry contains a string, `false` otherwise. */
			template<std::output_iterator<CharType> I>
			constexpr bool try_read(I &value) const
			{
				if (type == STRING) [[likely]]
				{
					std::copy_n(string.data(), string.size(), value);
					return true;
				}
				else
					return false;
			}
			/** Reads a string from the entry.
			 * @param value Output iterator used to write the string value to.
			 * @param sent Sentinel for the output iterator.
			 * @copydoc `true` if the entry contains a string, `false` otherwise. */
			template<std::output_iterator<CharType> I, std::sentinel_for<I> S>
			constexpr bool try_read(I &value, S &sent) const
			{
				if (type == STRING) [[likely]]
				{
					for (std::size_t i = 0; i != string.size && value != sent; ++i, ++value) *value = string.data[i];
					return true;
				}
				else
					return false;
			}

			/** Reads a string from the entry.
			 * @param value STL string to assign the string value to.
			 * @return Reference to this entry.
			 * @throw archive_error If the entry does not contain a string. */
			constexpr const entry_t &read(std::basic_string<CharType> &value) const
			{
				if (!try_read(value)) [[unlikely]]
					throw_string_error();
				return *this;
			}
			/** Reads a string from the entry.
			 * @param value STL string view to assign the string value to.
			 * @return Reference to this entry.
			 * @throw archive_error If the entry does not contain a string. */
			constexpr const entry_t &read(std::basic_string_view<CharType> &value) const
			{
				if (!try_read(value)) [[unlikely]]
					throw_string_error();
				return *this;
			}
			/** Reads a string from the entry.
			 * @param value Output iterator used to write the string value to.
			 * @return Reference to this entry.
			 * @throw archive_error If the entry does not contain a string. */
			template<std::output_iterator<CharType> I>
			constexpr const entry_t &read(I &value) const
			{
				if (!try_read(value)) [[unlikely]]
					throw_string_error();
				return *this;
			}
			/** Reads a string from the entry.
			 * @param value Output iterator used to write the string value to.
			 * @param sent Sentinel for the output iterator.
			 * @return Reference to this entry.
			 * @throw archive_error If the entry does not contain a string. */
			template<std::output_iterator<CharType> I, std::sentinel_for<I> S>
			constexpr const entry_t &read(I &value, S &sent) const
			{
				if (!try_read(value, sent)) [[unlikely]]
					throw_string_error();
				return *this;
			}

			/** Reads an object or array from the entry.
			 * @param value Forwarded value to be read from the entry.
			 * @return Reference to this entry.
			 * @throw archive_error If the entry does not contain an object or array. */
			template<typename T>
			constexpr const entry_t &read(T &&value) const;
			/** @copydoc read */
			template<typename T>
			constexpr const entry_t &operator>>(T &&value) const
			{
				return read(std::forward<T>(value));
			}
			/** Attempts to read an object or array from the entry.
			 * @param value Forwarded value to be read from the entry.
			 * @return `true` if read successfully, `false` otherwise. */
			template<typename T>
			constexpr bool try_read(T &&value) const
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

			/** Reads a default-initialized instance of `T` from the entry. */
			template<std::default_initializable T>
			constexpr T read() const
			{
				T result;
				read(result);
				return result;
			}

		private:
			constexpr void emit(auto &emitter) const
			{
				switch (type)
				{
					case NULL_VALUE: emitter.on_null(); break;
					case BOOL_FALSE:
					{
						if constexpr (requires { emitter.on_false(); })
							emitter.on_false();
						else
							emitter.on_bool(false);
						break;
					}
					case BOOL_TRUE:
					{
						if constexpr (requires { emitter.on_true(); })
							emitter.on_true();
						else
							emitter.on_bool(true);
						break;
					}
					case CHAR:
					{
						if constexpr ((Config & char_value) == char_value)
						{
							emitter.on_char(literal.c);
							break;
						}
						else
							SEK_NEVER_REACHED;
					}

					case INT_S64:
					case INT_S32:
					case INT_S16:
					case INT_S8: emitter.on_int(type, literal.si); break;
					case INT_U64:
					case INT_U32:
					case INT_U16:
					case INT_U8: emitter.on_uint(type, literal.ui); break;

					case FLOAT32: emitter.on_float32(literal.f32); break;
					case FLOAT64: emitter.on_float64(literal.f64); break;

					case STRING: emitter.on_string(string.data(), string.size()); break;
					case ARRAY:
					{
						auto frame = emitter.enter_frame();
						auto &array = container;

						emitter.on_array_start(array.size, array.value_type);
						for (auto value = array.array_data, end = value + array.size; value != end; ++value)
							value->emit(emitter);
						emitter.on_array_end();

						emitter.exit_frame(frame);
						break;
					}
					case OBJECT:
					{
						auto frame = emitter.enter_frame();
						auto &object = container;

						emitter.on_object_start(object.size, object.value_type);
						for (auto member = object.object_data, end = member + object.size; member != end; ++member)
						{
							emitter.on_object_key(member->key.data(), member->key.size());
							member->value.emit(emitter);
						}
						emitter.on_object_end();

						emitter.exit_frame(frame);
						break;
					}
					default: break;
				}
			}

			union
			{
				container_t container;
				std::basic_string_view<CharType> string;
				literal_t literal;
			};
			entry_type type;
		};

		struct member_t
		{
			entry_t value;
			std::basic_string_view<CharType> key;
		};

		struct parser_base
		{
			enum parse_state : int
			{
				EXPECT_OBJECT_KEY,
				EXPECT_OBJECT_VALUE,
				EXPECT_ARRAY_VALUE,
			};

			struct parse_frame
			{
				container_t *container = nullptr; /* Pointer to the actual container being parsed. */

				union
				{
					void *data_ptr = nullptr;
					entry_t *array_data;
					member_t *object_data;
				};
				std::size_t capacity = 0; /* Current amortized capacity of the container. */
				std::size_t size = 0;	  /* Current size of the container. */

				parse_state state;
			};

			constexpr explicit parser_base(json_archive_base &parent) noexcept : parent(parent) {}
			~parser_base()
			{
				if (parse_stack) [[likely]]
					parent.upstream->deallocate(parse_stack, stack_capacity * sizeof(parse_frame));
			}

			template<std::integral S>
			[[nodiscard]] CharType *on_string_alloc(S len) const
			{
				auto size = (static_cast<std::size_t>(len) + 1) * sizeof(CharType);
				auto result = static_cast<CharType *>(parent.string_pool.allocate(size));
				if (!result) [[unlikely]]
					throw std::bad_alloc();
				return result;
			}

			bool on_null() const
			{
				return on_value([](entry_t &entry) { entry.type = NULL_VALUE; });
			}
			bool on_bool(bool b) const
			{
				return on_value([b](entry_t &entry) { entry.type = static_cast<entry_type>(BOOL | (b ? 1 : 0)); });
			}
			bool on_true() const
			{
				return on_value([](entry_t &entry) { entry.type = BOOL_TRUE; });
			}
			bool on_false() const
			{
				return on_value([](entry_t &entry) { entry.type = BOOL_FALSE; });
			}
			bool on_char(CharType c) const
			{
				return on_value(
					[c](entry_t &entry)
					{
						entry.type = CHAR;
						entry.literal.c = c;
					});
			}
			template<std::integral I>
			bool on_int(I i) const
			{
				auto do_set_int = [i](entry_t &entry)
				{
					if constexpr (std::is_signed_v<I>)
					{
						entry.type = INT_S;
						entry.literal.si = static_cast<std::intmax_t>(i);
					}
					else
					{
						entry.type = INT_U;
						entry.literal.ui = static_cast<std::uintmax_t>(i);
					}
				};

				return on_value(do_set_int);
			}
			template<std::floating_point F>
			bool on_float(F f) const
			{
				return on_value(
					[f](entry_t &entry)
					{
						entry.type = FLOAT_MASK;
						entry.literal.fp = static_cast<double>(f);
					});
			}

			template<std::integral S>
			bool on_string(const CharType *str, S len) const
			{
				return on_value(
					[&](entry_t &entry)
					{
						entry.type = STRING;
						entry.string = std::basic_string_view<CharType>{str, static_cast<std::size_t>(len)};
					});
			}
			template<std::integral S>
			bool on_string_copy(const CharType *str, S len) const
			{
				auto dest = on_string_alloc(len);
				*std::copy_n(str, static_cast<std::size_t>(len), dest) = '\0';
				return on_string(dest, len);
			}

			bool on_object_start(std::size_t n = 0)
			{
				auto do_start_object = [&](entry_t &entry)
				{
					enter_frame();
					entry.type = OBJECT;
					current->container = &entry.container;
					current->state = parse_state::EXPECT_OBJECT_KEY;
					if (n) resize_container<member_t>(n);
				};

				if (!parse_stack) [[unlikely]] /* If stack is empty, this is the top-level object. */
				{
					do_start_object(parent.top_level);
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
						push_container<member_t>().key = std::basic_string_view<CharType>{str, static_cast<std::size_t>(len)};
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
						auto *obj = current->container;
						obj->object_data = current->object_data;
						obj->size = static_cast<std::size_t>(size);
						exit_frame();
						return true;
					}
					default: return false;
				}
			}

			bool on_array_start(std::size_t n = 0)
			{
				auto do_start_array = [&](entry_t &entry)
				{
					enter_frame();
					entry.type = ARRAY;
					current->container = &entry.container;
					current->state = parse_state::EXPECT_ARRAY_VALUE;
					if (n) resize_container<entry_t>(n);
				};

				if (!parse_stack) [[unlikely]] /* If stack is empty, this is the top-level array. */
				{
					do_start_array(parent.top_level);
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
						auto *arr = current->container;
						arr->array_data = current->array_data;
						arr->size = static_cast<std::size_t>(size);
						exit_frame();
						return true;
					}
					default: return false;
				}
			}

			template<typename T>
			void resize_container(std::size_t n) const
			{
				auto *old_data = current->data_ptr;
				auto old_cap = current->capacity * sizeof(T), new_cap = n * sizeof(T);

				auto *new_data = parent.entry_pool.reallocate(old_data, old_cap, new_cap);
				if (!new_data) [[unlikely]]
					throw std::bad_alloc();

				current->data_ptr = new_data;
				current->capacity = n;
			}
			template<typename T>
			[[nodiscard]] T &push_container() const
			{
				auto next_idx = current->size;
				if (current->capacity == current->size++) resize_container<T>(current->size * 2);
				/* No initialization needed here, since the entry will be initialized by parse events. */
				return static_cast<T *>(current->data_ptr)[next_idx];
			}

			bool on_value(auto f) const
			{
				entry_t *entry;
				switch (current->state)
				{
					case parse_state::EXPECT_ARRAY_VALUE:
					{
						entry = &push_container<entry_t>();
						break;
					}
					case parse_state::EXPECT_OBJECT_VALUE:
					{
						/* Size is updated by the key event. */
						entry = &(current->object_data[current->size - 1].value);
						current->state = parse_state::EXPECT_OBJECT_KEY;
						break;
					}
					default: return false;
				}

				f(*entry);
				return true;
			}
			void enter_frame()
			{
				if (!parse_stack) [[unlikely]]
				{
					auto new_stack = static_cast<parse_frame *>(parent.upstream->allocate(4 * sizeof(parse_frame)));
					if (!new_stack) [[unlikely]]
						throw std::bad_alloc();
					current = parse_stack = new_stack;
					stack_capacity = 4;
				}
				else if (auto pos = ++current - parse_stack; static_cast<std::size_t>(pos) == stack_capacity) [[unlikely]]
				{
					auto new_cap = stack_capacity * 2;
					auto new_stack = static_cast<parse_frame *>(parent.upstream->allocate(new_cap * sizeof(parse_frame)));
					if (!new_stack) [[unlikely]]
						throw std::bad_alloc();

					current = std::copy_n(parse_stack, pos, new_stack);

					parent.upstream->deallocate(parse_stack, stack_capacity * sizeof(parse_frame));
					parse_stack = new_stack;
					stack_capacity = new_cap;
				}
				std::construct_at(current);
			}
			void exit_frame() { --current; }

			json_archive_base &parent;
			parse_frame *parse_stack = nullptr;
			parse_frame *current = nullptr;
			std::size_t stack_capacity = 0;
		};

		/** @brief Iterator providing read-only access to a Json entry. */
		class entry_iterator
		{
			friend struct json_archive_base;
			friend class read_frame;

		public:
			typedef entry_t value_type;
			typedef const entry_t *pointer;
			typedef const entry_t &reference;
			typedef std::ptrdiff_t difference_type;

			typedef std::random_access_iterator_tag iterator_category;

		private:
			constexpr entry_iterator(const void *ptr, entry_type type) noexcept : data_ptr(ptr), type(type) {}

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

			/** Returns the key of the associated entry.
			 * If the pointed-to entry is not a keyed entry (object member) returns an empty string view. */
			[[nodiscard]] constexpr std::basic_string_view<CharType> key(std::nothrow_t) const noexcept
			{
				if (type != OBJECT) [[unlikely]]
					return {};
				else
					return object_data->key;
			}
			/** Returns the key of the associated entry.
			 * @throw archive_error If the pointed-to entry is not a keyed entry (object member). */
			[[nodiscard]] constexpr std::basic_string_view<CharType> key() const
			{
				if (type != OBJECT) [[unlikely]]
					throw archive_error("Entry iterator does not point to a keyed entry");
				else
					return object_data->key;
			}

			[[nodiscard]] friend constexpr difference_type operator-(entry_iterator a, entry_iterator b) noexcept
			{
				SEK_ASSERT(a.type == b.type);
				switch (a.type)
				{
					case ARRAY: return a.array_data - b.array_data;
					case OBJECT: return a.object_data - b.object_data;
					default: return 0;
				}
			}
			[[nodiscard]] friend constexpr entry_iterator operator+(difference_type n, entry_iterator a) noexcept
			{
				return a + n;
			}

			[[nodiscard]] constexpr auto operator<=>(const entry_iterator &other) const noexcept
			{
				return data_ptr <=> other.data_ptr;
			}
			[[nodiscard]] constexpr bool operator==(const entry_iterator &other) const noexcept
			{
				return data_ptr == other.data_ptr;
			}

		private:
			[[nodiscard]] constexpr entry_t *get_entry() const noexcept
			{
				switch (type)
				{
					case ARRAY: return array_data;
					case OBJECT: return &object_data->value;
					default: return nullptr;
				}
			}
			constexpr void move_n(difference_type n) noexcept
			{
				switch (type)
				{
					case ARRAY: array_data += n; break;
					case OBJECT: object_data += n; break;
					default: break;
				}
			}

			union
			{
				/** Pointer used for type-agnostic operations. */
				const void *data_ptr = nullptr;
				/** Pointer into an array container. */
				entry_t *array_data;
				/** Pointer into an object container. */
				member_t *object_data;
			};
			/** Type of the frame this iterator was created from. */
			entry_type type;
		};
		/** @brief Helper structure used as the API interface for Json input archive operations. */
		class read_frame
		{
			friend struct json_archive_base;
			friend class entry_t;

		public:
			typedef input_archive_category archive_category;
			typedef CharType char_type;

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
			struct frame_view_t
			{
				[[nodiscard]] constexpr auto *obj_begin() const noexcept
				{
					return static_cast<const member_t *>(begin_ptr);
				}
				[[nodiscard]] constexpr auto *obj_current() const noexcept
				{
					return static_cast<const member_t *>(current_ptr);
				}
				[[nodiscard]] constexpr auto *obj_end() const noexcept
				{
					return static_cast<const member_t *>(end_ptr);
				}

				const void *begin_ptr = nullptr;
				const void *current_ptr = nullptr;
				const void *end_ptr = nullptr;
			};

			constexpr explicit read_frame(const entry_t &entry) noexcept : type(entry.type)
			{
				frame_view = {
					.begin_ptr = entry.container.data_ptr,
					.current_ptr = entry.container.data_ptr,
					.end_ptr = entry.container.data_ptr,
				};

				if (entry.type == OBJECT) [[likely]]
					frame_view.end_ptr = entry.container.object_data + entry.container.size;
				else
					frame_view.end_ptr = entry.container.array_data + entry.container.size;
			}

		public:
			read_frame() = delete;
			read_frame(const read_frame &) = delete;
			read_frame &operator=(const read_frame &) = delete;
			read_frame(read_frame &&) = delete;
			read_frame &operator=(read_frame &&) = delete;

			/** Returns iterator to the first entry of the currently read object or array. */
			[[nodiscard]] constexpr entry_iterator begin() const noexcept { return {frame_view.begin_ptr, type}; }
			/** @copydoc begin */
			[[nodiscard]] constexpr entry_iterator cbegin() const noexcept { return begin(); }
			/** Returns iterator one past the last entry of the currently read object or array. */
			[[nodiscard]] constexpr entry_iterator end() const noexcept { return {frame_view.end_ptr, type}; }
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
			 * @param value Value to deserialize.
			 * @return `true` if deserialization was successful, `false` otherwise. */
			template<typename T>
			bool try_read(T &&value)
			{
				entry_iterator current{frame_view.current_ptr, type};
				if (current->try_read(std::forward<T>(value))) [[likely]]
				{
					frame_view.current_ptr = (current + 1).data_ptr;
					return true;
				}
				else
					return false;
			}
			/** Deserializes the next Json entry of the archive & advance the entry.
			 * @param value Value to deserialize.
			 * @return Reference to this frame.
			 * @throw archive_exception On deserialization errors. */
			template<typename T>
			read_frame &read(T &&value)
			{
				entry_iterator current{frame_view.current_ptr, type};
				current->read(std::forward<T>(value));
				frame_view.current_ptr = (current + 1).data_ptr;
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

			/** Deserializes the next Json entry using the keyed entry hint.
			 * @param value Named entry containing the entry key hint & forwarded entry value.
			 * @return `true` if deserialization was successful, `false` otherwise. */
			template<typename T>
			bool try_read(keyed_entry_t<CharType, T> value)
			{
				if (type == OBJECT) [[likely]]
				{
					if (seek_entry(value.key)) [[likely]]
						return try_read(std::forward<T>(value.value));
				}
				return false;
			}
			/** Deserializes the next Json entry using the keyed entry hint.
			 * @param value Named entry containing the entry key hint & forwarded entry value.
			 * @return Reference to this frame.
			 * @throw archive_exception On deserialization errors. */
			template<typename T>
			read_frame &read(keyed_entry_t<CharType, T> value)
			{
				if (type == ARRAY) [[unlikely]]
					throw archive_error("Named entry modifier cannot be applied to an array entry");

				if (!seek_entry(value.key)) [[unlikely]]
				{
					std::string err{"Invalid Json object member \""};
					err.append(value.key);
					err.append(1, '\"');
					throw std::out_of_range(err);
				}
				else
					read(std::forward<T>(value.value));
				return *this;
			}
			/** @copydoc read */
			template<typename T>
			read_frame &operator>>(keyed_entry_t<CharType, T> value)
			{
				return read(value);
			}

			/** Reads size of the frame's container (array or object) entry.
			 * @param value Size modifier containing the forwarded size.
			 * @return Always returns `true`. */
			template<typename I>
			bool try_read(container_size_t<I> value) noexcept
			{
				value.value = static_cast<std::decay_t<I>>(size());
				return true;
			}
			/** Reads size of the frame's container (array or object) entry.
			 * @param value Size modifier containing the forwarded size.
			 * @return Reference to this frame. */
			template<typename I>
			read_frame &read(container_size_t<I> value) noexcept
			{
				try_read(value);
				return *this;
			}
			/** @copydoc read */
			template<typename I>
			read_frame &operator>>(container_size_t<I> value) noexcept
			{
				return read(value);
			}

		private:
			[[nodiscard]] constexpr const member_t *find_member(std::basic_string_view<CharType> key) const noexcept
			{
				for (auto member = frame_view.obj_begin(); member != frame_view.obj_end(); ++member)
					if (key == member->key) return member;
				return nullptr;
			}
			[[nodiscard]] constexpr const entry_t *seek_entry(std::basic_string_view<CharType> key) noexcept
			{
				if (frame_view.obj_current() >= frame_view.obj_end() || key != frame_view.obj_current()->key)
				{
					if (auto member_ptr = find_member(key); !member_ptr) [[unlikely]]
						return nullptr;
					else
						frame_view.current_ptr = member_ptr;
				}
				return &frame_view.obj_current()->value;
			}

			frame_view_t frame_view = {};
			entry_type type;
		};
		/** @brief Helper structure used as the API interface for Json output archive operations. */
		class write_frame
		{
			friend struct json_archive_base;

		public:
			typedef output_archive_category archive_category;
			typedef CharType char_type;
			typedef std::size_t size_type;

		private:
			constexpr write_frame(json_archive_base &parent, entry_t &entry) noexcept : parent(parent), current(entry)
			{
			}

		public:
			write_frame() = delete;
			write_frame(const write_frame &) = delete;
			write_frame &operator=(const write_frame &) = delete;
			write_frame(write_frame &&) = delete;
			write_frame &operator=(write_frame &&) = delete;

			/** Serialized the forwarded value to UBJson.
			 * @param value Value to serialize as UBJson.
			 * @return Reference to this frame. */
			template<typename T>
			write_frame &write(T &&value)
			{
				write_impl(std::forward<T>(value));
				return *this;
			}
			/** @copydoc write */
			template<typename T>
			write_frame &operator<<(T &&value)
			{
				return write(std::forward<T>(value));
			}

		private:
			[[nodiscard]] CharType *alloc_string(std::size_t n) const
			{
				auto bytes = (n + 1) * sizeof(CharType);
				auto result = static_cast<CharType *>(parent.string_pool.allocate(bytes));
				if (!result) [[unlikely]]
					throw std::bad_alloc();
				return result;
			}
			[[nodiscard]] std::basic_string_view<CharType> copy_string(std::basic_string_view<CharType> str) const
			{
				auto result = alloc_string(str.size());
				*std::copy_n(str.data(), str.size(), result) = '\0';
				return {result, str.size()};
			}

			template<typename T>
			void resize_container(std::size_t n) const
			{
				auto *old_data = current.container.data_ptr;
				auto old_cap = current.container.capacity * sizeof(T), new_cap = n * sizeof(T);

				auto *new_data = parent.entry_pool.reallocate(old_data, old_cap, new_cap);
				if (!new_data) [[unlikely]]
					throw std::bad_alloc();

				current.container.data_ptr = new_data;
				current.container.capacity = n;
			}
			template<typename T>
			[[nodiscard]] T *push_container() const
			{
				auto next_idx = current.container.size;
				if (current.container.capacity == current.container.size++)
					resize_container<T>(current.container.size * 2);
				return static_cast<T *>(current.container.data_ptr) + next_idx;
			}
			[[nodiscard]] entry_t *next_entry() const
			{
				entry_t *entry;
				switch (current.type)
				{
					default:
					{
						current.type = OBJECT;
						[[fallthrough]];
					}
					case OBJECT:
					{
						auto member = push_container<member_t>();
						member->key = next_key;
						entry = &member->value;
						break;
					}
					case ARRAY:
					{
						entry = push_container<entry_t>();
						break;
					}
				}

				::new (entry) entry_t{};
				return entry;
			}

			template<typename T>
			void write_value(entry_t &entry, T &&value) const
			{
				write_frame frame{parent, entry};
				detail::invoke_serialize(std::forward<T>(value), frame);
			}

			void write_value(entry_t &entry, std::nullptr_t) const { entry.type = NULL_VALUE; }
			template<typename T>
			void write_value(entry_t &entry, T &&b) const requires(std::same_as<std::decay_t<T>, bool>)
			{
				entry.type = static_cast<entry_type>(BOOL | static_cast<int>(!!b));
			}
			template<typename T>
			void write_value(entry_t &entry, T &&c) const
				requires(std::same_as<std::decay_t<T>, CharType> && ((Config & char_value) == char_value))
			{
				entry.type = CHAR;
				entry.literal.character = c;
			}

			template<typename T>
			constexpr static bool is_uint_value =
				std::unsigned_integral<T> && !std::same_as<T, bool> && !std::same_as<T, CharType>;
			template<typename T>
			constexpr static bool is_int_value =
				std::signed_integral<T> && !std::same_as<T, bool> && !std::same_as<T, CharType>;

			template<std::unsigned_integral I>
			constexpr static int int_size_type(I i) noexcept
			{
				return int_size_category(i);
			}
			template<typename I>
			void write_value(entry_t &entry, I &&i) const requires is_uint_value<std::decay_t<I>>
			{
				entry.type = static_cast<entry_type>(INT_U | int_size_type(i));
				entry.literal.ui = static_cast<std::uintmax_t>(i);
			}
			template<std::signed_integral I>
			constexpr static int int_size_type(I i) noexcept
			{
				static_assert(-1 == ~0, "Serialization assumes 2's complement");

				/* If the integer is negative, negate the 2s complement integer.
				 * This will give a "size mask" for 2s complement.
				 * Use that mask's unsigned value to get the type. */
				// clang-format off
				const auto size_mask = i < 0 ? static_cast<std::make_unsigned_t<I>>(~i) :
				                               static_cast<std::make_unsigned_t<I>>(i);
				// clang-format on
				return int_size_category(size_mask);
			}
			template<typename I>
			void write_value(entry_t &entry, I &&i) const requires is_int_value<std::decay_t<I>>
			{
				entry.type = static_cast<entry_type>(INT_S | int_size_type(i));
				entry.literal.si = static_cast<std::intmax_t>(i);
			}

			template<typename F>
			void write_value(entry_t &entry, F &&f) const requires std::floating_point<std::decay_t<F>>
			{
				if constexpr (sizeof(F) > sizeof(float))
				{
					entry.type = FLOAT64;
					entry.literal.f64 = static_cast<double>(f);
				}
				else
				{
					entry.type = FLOAT32;
					entry.literal.f32 = static_cast<float>(f);
				}
			}

			void write_value(entry_t &entry, std::basic_string_view<CharType> sv) const
			{
				entry.type = STRING;
				entry.string = copy_string(sv);
			}
			void write_value(entry_t &entry, const CharType *str) const
			{
				write_value(entry, std::basic_string_view<CharType>{str});
			}
			template<typename Traits>
			void write_value(entry_t &entry, std::basic_string<CharType, Traits> &&str) const
			{
				write_value(entry, std::basic_string_view<CharType>{str});
			}
			template<typename Traits>
			void write_value(entry_t &entry, std::basic_string<CharType, Traits> &str) const
			{
				write_value(entry, std::basic_string_view<CharType>{str});
			}
			template<typename Traits>
			void write_value(entry_t &entry, const std::basic_string<CharType, Traits> &str) const
			{
				write_value(entry, std::basic_string_view<CharType>{str});
			}
			template<typename T>
			void write_value(entry_t &entry, T &&str) const
				requires(std::constructible_from<std::basic_string_view<CharType>, T>)
			{
				write_value(entry, std::basic_string_view<CharType>{std::forward<T>(str)});
			}
			template<typename T>
			void write_value(entry_t &entry, T &&str) const requires std::convertible_to<T, const CharType *>
			{
				write_value(entry, static_cast<const CharType *>(str));
			}

			template<typename T>
			void write_value(T &&value) const
			{
				auto entry = next_entry();
				SEK_ASSERT(entry != nullptr);

				write_value(*entry, std::forward<T>(value));

				if constexpr ((Config & container_types) == container_types)
				{
					/* Set container value type if all entries are of the same type.
					 * Use a different condition for integers to account for size-category. */
					if (current.container.value_type == NO_TYPE) [[unlikely]]
						current.container.value_type = entry->type;
					else if constexpr (!std::is_integral_v<std::decay_t<T>>)
					{
						if (current.container.value_type != entry->type) [[likely]]
							current.container.value_type = DYNAMIC;
					}
					else
					{
						/* If the current type is also an integer of the same signedness, use the largest size category. */
						if ((current.container.value_type & INT_S) == (entry->type & INT_S))
						{
							if (current.container.value_type < entry->type) [[unlikely]]
								current.container.value_type = entry->type;
						}
						else
							current.container.value_type = DYNAMIC;
					}
				}
			}

			template<typename T>
			void write_impl(T &&value)
			{
				if (current.type != ARRAY) [[likely]]
					next_key = generate_key<CharType>(parent.string_pool, current.container.size);
				write_value(std::forward<T>(value));
			}
			template<typename T>
			void write_impl(keyed_entry_t<CharType, T> value)
			{
				if (current.type != ARRAY) [[likely]]
					next_key = generate_key<CharType>(parent.string_pool, value.key);
				write_value(std::forward<T>(value.value));
			}
			template<typename T>
			void write_impl(container_size_t<T> size)
			{
				switch (current.type)
				{
					default:
					{
						current.type = OBJECT;
						[[fallthrough]];
					}
					case OBJECT:
					{
						resize_container<member_t>(static_cast<std::size_t>(size.value));
						break;
					}
					case ARRAY:
					{
						resize_container<entry_t>(static_cast<std::size_t>(size.value));
						break;
					}
				}
			}
			void write_impl(array_mode_t)
			{
				SEK_ASSERT(current.type != OBJECT, "Array mode modifier applied to object entry");

				current.type = ARRAY;
			}

			json_archive_base &parent;
			entry_t &current;
			std::basic_string_view<CharType> next_key = {};
		};

		using entry_pool_t = basic_pool_resource<sizeof(entry_t) * 64>;
		using string_pool_t = basic_pool_resource<SEK_KB(1)>;

		json_archive_base() = delete;
		json_archive_base(const json_archive_base &) = delete;
		json_archive_base &operator=(const json_archive_base &) = delete;

		explicit json_archive_base(std::pmr::memory_resource *res) noexcept
			: upstream(res), entry_pool(res), string_pool(res)
		{
		}
		constexpr json_archive_base(json_archive_base &&other) noexcept
			: upstream(std::exchange(other.upstream, nullptr)),
			  entry_pool(std::move(other.entry_pool)),
			  string_pool(std::move(other.string_pool)),
			  top_level()
		{
			top_level.swap(other.top_level);
		}
		constexpr json_archive_base &operator=(json_archive_base &&other) noexcept
		{
			swap(other);
			return *this;
		}
		~json_archive_base() = default;

		void reset()
		{
			entry_pool.release();
			string_pool.release();
			::new (&top_level) entry_t{};
		}
		void reset(std::pmr::memory_resource *res)
		{
			entry_pool = entry_pool_t{res};
			string_pool = string_pool_t{res};
			::new (&top_level) entry_t{};
		}

		template<typename T>
		bool do_try_read(T &&value)
		{
			return top_level.try_read(std::forward<T>(value));
		}
		template<typename T>
		void do_read(T &&value)
		{
			top_level.read(std::forward<T>(value));
		}
		template<typename T>
		void do_write(T &&value)
		{
			write_frame frame{*this, top_level};
			detail::invoke_serialize(std::forward<T>(value), frame);
		}

		void do_flush(auto &emitter) const
		{
			if (top_level.type != NO_TYPE) [[likely]]
				top_level.emit(emitter);
		}

		constexpr void swap(json_archive_base &other) noexcept
		{
			std::swap(upstream, other.upstream);
			entry_pool.swap(other.entry_pool);
			string_pool.swap(other.string_pool);
			top_level.swap(other.top_level);
		}

		std::pmr::memory_resource *upstream;

		entry_pool_t entry_pool;   /* Allocation pool used for entry tree allocation. */
		string_pool_t string_pool; /* Allocation pool used for string allocation. */
		entry_t top_level = {};	   /* Top-most entry of the Json tree. */
	};

	template<typename C, json_archive_config Cfg>
	template<typename T>
	constexpr const typename json_archive_base<C, Cfg>::entry_t &json_archive_base<C, Cfg>::entry_t::read(T &&value) const
	{
		if (!(type & CONTAINER)) [[unlikely]]
			throw archive_error("Invalid Json type, expected array or object");

		read_frame frame{*this};
		detail::invoke_deserialize(std::forward<T>(value), frame);
		return *this;
	}
}	 // namespace sek::serialization::detail