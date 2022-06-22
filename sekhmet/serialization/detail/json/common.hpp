/*
 * Created by switchblade on 2022-04-14
 */

#pragma once

#include "../archive_reader.hpp"
#include "../archive_writer.hpp"
#include "../manipulators.hpp"
#include "../util.hpp"
#include "sekhmet/detail/assert.hpp"
#include "sekhmet/detail/define.h"
#include "sekhmet/detail/dynamic_buffer_resource.hpp"

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
		class parser_base;

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

			friend class parser_base;
			friend class read_frame;
			friend class write_frame;

			static void throw_string_error() { throw archive_error("Invalid Json type, expected string"); }

			constexpr void swap(entry_t &other) noexcept
			{
				std::swap(m_container, other.m_container);
				std::swap(m_type, other.m_type);
			}

			/* Must only be accessible from friends. */
			constexpr entry_t() noexcept : m_container{}, m_type(NO_TYPE) {}

		public:
			entry_t(const entry_t &) = delete;
			entry_t &operator=(const entry_t &) = delete;
			entry_t(entry_t &&) = delete;
			entry_t &operator=(entry_t &&) = delete;

			/** Reads a null value from the entry. Returns `true` if the entry contains a null value, `false` otherwise. */
			constexpr bool try_read(std::nullptr_t, auto &&...) const noexcept { return m_type == NULL_VALUE; }
			/** Reads a null value from the entry.
			 * @throw archive_error If the entry does not contain a null value. */
			constexpr const entry_t &read(std::nullptr_t, auto &&...) const
			{
				if (!try_read(nullptr)) [[unlikely]]
					throw archive_error("Invalid Json type, expected null");
				return *this;
			}
			/** @copydoc read */
			constexpr std::nullptr_t read(std::in_place_type_t<std::nullptr_t>, auto &&...) const
			{
				read(std::nullptr_t{});
				return std::nullptr_t{};
			}

			/** Reads a bool from the entry. Returns `true` if the entry contains a bool, `false` otherwise. */
			constexpr bool try_read(bool &b, auto &&...) const noexcept
			{
				if (m_type & BOOL) [[likely]]
				{
					b = m_type & 1;
					return true;
				}
				else
					return false;
			}
			/** Reads a bool from the entry.
			 * @throw archive_error If the entry does not contain a bool. */
			constexpr const entry_t &read(bool &b, auto &&...) const
			{
				if (!try_read(b)) [[unlikely]]
					throw archive_error("Invalid Json type, expected bool");
				return *this;
			}
			/** @copydoc read */
			constexpr bool read(std::in_place_type_t<bool>, auto &&...) const
			{
				bool result;
				read(result);
				return result;
			}

			/** Reads a character from the entry. Returns `true` if the entry contains a character, `false` otherwise. */
			constexpr bool try_read(CharType &c, auto &&...) const noexcept
				requires((Config & char_value) == char_value)
			{
				if (m_type == CHAR) [[likely]]
				{
					c = m_literal.c;
					return true;
				}
				else
					return false;
			}
			/** Reads a character from the entry.
			 * @throw archive_error If the entry does not contain a character. */
			constexpr const entry_t &read(CharType &c, auto &&...) const
				requires((Config & char_value) == char_value)
			{
				if (!try_read(c)) [[unlikely]]
					throw archive_error("Invalid Json type, expected char");
				return *this;
			}
			/** @copydoc read */
			constexpr CharType read(std::in_place_type_t<CharType>, auto &&...) const
				requires((Config & char_value) == char_value)
			{
				CharType result;
				read(result);
				return result;
			}

			/** Reads a number from the entry. Returns `true` if the entry contains a number, `false` otherwise. */
			template<typename I>
			constexpr bool try_read(I &value, auto &&...) const noexcept
				requires(std::integral<I> || std::floating_point<I>)
			{
				if (m_type & INT_MASK)
				{
					if (m_type & INT_SIGN_BIT)
						value = static_cast<I>(m_literal.si);
					else
						value = static_cast<I>(m_literal.ui);
					return true;
				}
				else if (m_type & FLOAT_MASK)
				{
					value = static_cast<I>(m_literal.fp);
					return true;
				}
				else
					return false;
			}
			/** Reads a number from the entry.
			 * @throw archive_error If the entry does not contain a number. */
			template<typename I>
			constexpr const entry_t &read(I &value, auto &&...) const
				requires(std::integral<I> || std::floating_point<I>)
			{
				if (!try_read(value)) [[unlikely]]
					throw archive_error("Invalid Json type, expected number");
				return *this;
			}
			/** @copydoc read */
			template<typename I>
			constexpr I read(std::in_place_type_t<I>, auto &&...) const
				requires(std::integral<I> || std::floating_point<I>)
			{
				I result;
				read(result);
				return result;
			}

			/** Reads a string from the entry.
			 * @param value STL string to assign the string value to.
			 * @copydoc `true` if the entry contains a string, `false` otherwise. */
			template<typename T = std::char_traits<CharType>, typename A = std::allocator<CharType>>
			constexpr bool try_read(std::basic_string<CharType, T, A> &value, auto &&...) const
			{
				if (m_type == STRING) [[likely]]
				{
					value.assign(m_string);
					return true;
				}
				else
					return false;
			}
			/** Reads a string from the entry.
			 * @param value STL string view to assign the string value to.
			 * @copydoc `true` if the entry contains a string, `false` otherwise. */
			template<typename T = std::char_traits<CharType>>
			constexpr bool try_read(std::basic_string_view<CharType, T> &value, auto &&...) const noexcept
			{
				if (m_type == STRING) [[likely]]
				{
					if constexpr (std::same_as<std::basic_string_view<CharType, T>, std::basic_string_view<CharType>>)
						value = m_string;
					else
						value = {m_string.begin(), m_string.end()};
					return true;
				}
				else
					return false;
			}
			/** Reads a string from the entry.
			 * @param value Output iterator used to write the string value to.
			 * @copydoc `true` if the entry contains a string, `false` otherwise. */
			template<std::output_iterator<CharType> I>
			constexpr bool try_read(I &value, auto &&...) const
			{
				if (m_type == STRING) [[likely]]
				{
					std::copy_n(m_string.data(), m_string.size(), value);
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
			constexpr bool try_read(I &value, S &sent, auto &&...) const
			{
				if (m_type == STRING) [[likely]]
				{
					for (std::size_t i = 0; i != m_string.size && value != sent; ++i, ++value)
						*value = m_string.data[i];
					return true;
				}
				else
					return false;
			}

			/** Reads a string from the entry.
			 * @param value STL string to assign the string value to.
			 * @return Reference to this entry.
			 * @throw archive_error If the entry does not contain a string. */
			template<typename T = std::char_traits<CharType>, typename A = std::allocator<CharType>>
			constexpr const entry_t &read(std::basic_string<CharType, T, A> &value, auto &&...) const
			{
				if (!try_read(value)) [[unlikely]]
					throw_string_error();
				return *this;
			}
			/** @copydoc read */
			template<typename T = std::char_traits<CharType>, typename A = std::allocator<CharType>>
			constexpr std::basic_string<CharType> read(std::in_place_type_t<std::basic_string<CharType, T, A>>, auto &&...) const
			{
				std::basic_string<CharType, T, A> result;
				read(result);
				return result;
			}
			/** Reads a string from the entry.
			 * @param value STL string view to assign the string value to.
			 * @return Reference to this entry.
			 * @throw archive_error If the entry does not contain a string. */
			template<typename T = std::char_traits<CharType>>
			constexpr const entry_t &read(std::basic_string_view<CharType, T> &value, auto &&...) const
			{
				if (!try_read(value)) [[unlikely]]
					throw_string_error();
				return *this;
			}
			/** @copydoc read */
			template<typename T = std::char_traits<CharType>>
			constexpr std::basic_string_view<CharType, T> read(std::in_place_type_t<std::basic_string_view<CharType, T>>,
															   auto &&...) const
			{
				std::basic_string_view<CharType, T> result;
				read(result);
				return result;
			}
			/** Reads a string from the entry.
			 * @param value Output iterator used to write the string value to.
			 * @return Reference to this entry.
			 * @throw archive_error If the entry does not contain a string. */
			template<std::output_iterator<CharType> I>
			constexpr const entry_t &read(I &value, auto &&...) const
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
			constexpr const entry_t &read(I &value, S &sent, auto &&...) const
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
			constexpr const entry_t &operator>>(T &&value) const
			{
				return read(std::forward<T>(value));
			}
			/** @copydoc operator>>
			 * @param args Arguments forwarded to the deserialization function. */
			template<typename T, typename... Args>
			constexpr const entry_t &read(T &&value, Args &&...args) const;
			/** @brief Reads an object or array from the entry in-place.
			 * Uses the in-place `deserialize` overload (taking `std::in_place_type_t<T>`)
			 * or constructor accepting the archive frame as one of it's arguments if available.
			 * Otherwise, default-constructs & deserializes using `read(T &&)`.
			 * @param args Arguments forwarded to the deserialization function.
			 * @return Deserialized instance of `T`.
			 * @throw archive_error On deserialization errors. */
			template<typename T, typename... Args>
			constexpr T read(std::in_place_type_t<T>, Args &&...args) const;
			/** Attempts to read an object or array from the entry.
			 * @param value Forwarded value to be read from the entry.
			 * @param args Arguments forwarded to the deserialization function.
			 * @return `true` if read successfully, `false` otherwise. */
			template<typename T, typename... Args>
			constexpr bool try_read(T &&value, Args &&...args) const
			{
				try
				{
					read(std::forward<T>(value), std::forward<Args>(args)...);
					return true;
				}
				catch (archive_error &)
				{
					return false;
				}
			}

		private:
			constexpr void emit(auto &emitter) const
			{
				switch (m_type)
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
							emitter.on_char(m_literal.c);
							break;
						}
						else
							SEK_NEVER_REACHED;
					}

					case INT_S64:
					case INT_S32:
					case INT_S16:
					case INT_S8: emitter.on_int(m_type, m_literal.si); break;
					case INT_U64:
					case INT_U32:
					case INT_U16:
					case INT_U8: emitter.on_uint(m_type, m_literal.ui); break;

					case FLOAT32: emitter.on_float32(m_literal.f32); break;
					case FLOAT64: emitter.on_float64(m_literal.f64); break;

					case STRING: emitter.on_string(m_string.data(), m_string.size()); break;
					case ARRAY:
					{
						auto frame = emitter.enter_frame();
						auto &array = m_container;

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
						auto &object = m_container;

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
				container_t m_container;
				std::basic_string_view<CharType> m_string;
				literal_t m_literal;
			};
			entry_type m_type;
		};

		struct member_t
		{
			entry_t value;
			std::basic_string_view<CharType> key;
		};

		class parser_base
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

		public:
			constexpr explicit parser_base(json_archive_base &parent) noexcept : m_parent(parent) {}
			~parser_base()
			{
				if (m_parse_stack) [[likely]]
					m_parent.m_upstream->deallocate(m_parse_stack, m_stack_capacity * sizeof(parse_frame));
			}

			template<std::integral S>
			[[nodiscard]] CharType *on_string_alloc(S len) const
			{
				auto size = (static_cast<std::size_t>(len) + 1) * sizeof(CharType);
				auto result = static_cast<CharType *>(m_parent.m_string_pool.allocate(size));
				if (!result) [[unlikely]]
					throw std::bad_alloc();
				return result;
			}

			bool on_null() const
			{
				return on_value([](entry_t &entry) { entry.m_type = NULL_VALUE; });
			}
			bool on_bool(bool b) const
			{
				return on_value([b](entry_t &entry) { entry.m_type = static_cast<entry_type>(BOOL | (b ? 1 : 0)); });
			}
			bool on_true() const
			{
				return on_value([](entry_t &entry) { entry.m_type = BOOL_TRUE; });
			}
			bool on_false() const
			{
				return on_value([](entry_t &entry) { entry.m_type = BOOL_FALSE; });
			}
			bool on_char(CharType c) const
			{
				return on_value(
					[c](entry_t &entry)
					{
						entry.m_type = CHAR;
						entry.m_literal.c = c;
					});
			}
			template<std::integral I>
			bool on_int(I i) const
			{
				auto do_set_int = [i](entry_t &entry)
				{
					if constexpr (std::is_signed_v<I>)
					{
						entry.m_type = INT_S;
						entry.m_literal.si = static_cast<std::intmax_t>(i);
					}
					else
					{
						entry.m_type = INT_U;
						entry.m_literal.ui = static_cast<std::uintmax_t>(i);
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
						entry.m_type = FLOAT_MASK;
						entry.m_literal.fp = static_cast<double>(f);
					});
			}

			template<std::integral S>
			bool on_string(const CharType *str, S len) const
			{
				return on_value(
					[&](entry_t &entry)
					{
						entry.m_type = STRING;
						entry.m_string = std::basic_string_view<CharType>{str, static_cast<std::size_t>(len)};
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
					entry.m_type = OBJECT;
					m_current->container = &entry.m_container;
					m_current->state = parse_state::EXPECT_OBJECT_KEY;
					if (n) resize_container<member_t>(n);
				};

				if (!m_parse_stack) [[unlikely]] /* If stack is empty, this is the top-level object. */
				{
					do_start_object(m_parent.m_top_level);
					return true;
				}
				else
					return on_value(do_start_object);
			}
			template<std::integral S>
			bool on_object_key(const CharType *str, S len)
			{
				switch (m_current->state)
				{
					case parse_state::EXPECT_OBJECT_KEY:
					{
						push_container<member_t>().key = std::basic_string_view<CharType>{str, static_cast<std::size_t>(len)};
						m_current->state = parse_state::EXPECT_OBJECT_VALUE; /* Always expect value after key. */
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
				switch (m_current->state)
				{
					case parse_state::EXPECT_OBJECT_KEY:
					{
						auto *obj = m_current->container;
						obj->object_data = m_current->object_data;
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
					entry.m_type = ARRAY;
					m_current->container = &entry.m_container;
					m_current->state = parse_state::EXPECT_ARRAY_VALUE;
					if (n) resize_container<entry_t>(n);
				};

				if (!m_parse_stack) [[unlikely]] /* If stack is empty, this is the top-level array. */
				{
					do_start_array(m_parent.m_top_level);
					return true;
				}
				else
					return on_value(do_start_array);
			}
			template<std::integral S>
			bool on_array_end(S size)
			{
				switch (m_current->state)
				{
					case parse_state::EXPECT_ARRAY_VALUE:
					{
						auto *arr = m_current->container;
						arr->array_data = m_current->array_data;
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
				auto *old_data = m_current->data_ptr;
				auto old_cap = m_current->capacity * sizeof(T), new_cap = n * sizeof(T);

				auto *new_data = m_parent.m_entry_pool.reallocate(old_data, old_cap, new_cap);
				if (!new_data) [[unlikely]]
					throw std::bad_alloc();

				m_current->data_ptr = new_data;
				m_current->capacity = n;
			}
			template<typename T>
			[[nodiscard]] T &push_container() const
			{
				auto next_idx = m_current->size;
				if (m_current->capacity == m_current->size++) resize_container<T>(m_current->size * 2);
				/* No initialization needed here, since the entry will be initialized by parse events. */
				return static_cast<T *>(m_current->data_ptr)[next_idx];
			}

			bool on_value(auto f) const
			{
				entry_t *entry;
				switch (m_current->state)
				{
					case parse_state::EXPECT_ARRAY_VALUE:
					{
						entry = &push_container<entry_t>();
						break;
					}
					case parse_state::EXPECT_OBJECT_VALUE:
					{
						/* Size is updated by the key event. */
						entry = &(m_current->object_data[m_current->size - 1].value);
						m_current->state = parse_state::EXPECT_OBJECT_KEY;
						break;
					}
					default: return false;
				}

				f(*entry);
				return true;
			}
			void enter_frame()
			{
				if (!m_parse_stack) [[unlikely]]
				{
					auto new_stack = static_cast<parse_frame *>(m_parent.m_upstream->allocate(4 * sizeof(parse_frame)));
					if (!new_stack) [[unlikely]]
						throw std::bad_alloc();
					m_current = m_parse_stack = new_stack;
					m_stack_capacity = 4;
				}
				else if (auto pos = ++m_current - m_parse_stack; static_cast<std::size_t>(pos) == m_stack_capacity)
					[[unlikely]]
				{
					auto new_cap = m_stack_capacity * 2;
					auto new_stack = static_cast<parse_frame *>(m_parent.m_upstream->allocate(new_cap * sizeof(parse_frame)));
					if (!new_stack) [[unlikely]]
						throw std::bad_alloc();

					m_current = std::copy_n(m_parse_stack, pos, new_stack);

					m_parent.m_upstream->deallocate(m_parse_stack, m_stack_capacity * sizeof(parse_frame));
					m_parse_stack = new_stack;
					m_stack_capacity = new_cap;
				}
				std::construct_at(m_current);
			}
			void exit_frame() { --m_current; }

		private:
			json_archive_base &m_parent;
			parse_frame *m_parse_stack = nullptr;
			parse_frame *m_current = nullptr;
			std::size_t m_stack_capacity = 0;
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
			constexpr entry_iterator(const void *ptr, entry_type type) noexcept : m_data_ptr(ptr), m_type(type) {}

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

			/** Checks if the associated entry has a key. */
			[[nodiscard]] constexpr bool has_key() const noexcept { return m_type == OBJECT; }
			/** Returns the key of the associated entry.
			 * If the pointed-to entry is not a keyed entry (object member) returns an empty string view. */
			[[nodiscard]] constexpr std::basic_string_view<CharType> key(std::nothrow_t) const noexcept
			{
				if (m_type != OBJECT) [[unlikely]]
					return {};
				else
					return m_object_data->key;
			}
			/** Returns the key of the associated entry.
			 * @throw archive_error If the pointed-to entry is not a keyed entry (object member). */
			[[nodiscard]] constexpr std::basic_string_view<CharType> key() const
			{
				if (m_type != OBJECT) [[unlikely]]
					throw archive_error("Entry iterator does not point to a keyed entry");
				else
					return m_object_data->key;
			}

			[[nodiscard]] friend constexpr difference_type operator-(entry_iterator a, entry_iterator b) noexcept
			{
				SEK_ASSERT(a.m_type == b.m_type);
				switch (a.m_type)
				{
					case ARRAY: return a.m_array_data - b.m_array_data;
					case OBJECT: return a.m_object_data - b.m_object_data;
					default: return 0;
				}
			}
			[[nodiscard]] friend constexpr entry_iterator operator+(difference_type n, entry_iterator a) noexcept
			{
				return a + n;
			}

			[[nodiscard]] constexpr auto operator<=>(const entry_iterator &other) const noexcept
			{
				return m_data_ptr <=> other.m_data_ptr;
			}
			[[nodiscard]] constexpr bool operator==(const entry_iterator &other) const noexcept
			{
				return m_data_ptr == other.m_data_ptr;
			}

		private:
			[[nodiscard]] constexpr entry_t *get_entry() const noexcept
			{
				switch (m_type)
				{
					case ARRAY: return m_array_data;
					case OBJECT: return &m_object_data->value;
					default: return nullptr;
				}
			}
			constexpr void move_n(difference_type n) noexcept
			{
				switch (m_type)
				{
					case ARRAY: m_array_data += n; break;
					case OBJECT: m_object_data += n; break;
					default: break;
				}
			}

			union
			{
				/** Pointer used for type-agnostic operations. */
				const void *m_data_ptr = nullptr;
				/** Pointer into an array container. */
				entry_t *m_array_data;
				/** Pointer into an object container. */
				member_t *m_object_data;
			};
			/** Type of the frame this iterator was created from. */
			entry_type m_type;
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

			constexpr explicit read_frame(const entry_t &entry) noexcept : m_type(entry.m_type)
			{
				m_frame_view = {
					.begin_ptr = entry.m_container.data_ptr,
					.current_ptr = entry.m_container.data_ptr,
					.end_ptr = entry.m_container.data_ptr,
				};

				if (entry.m_type == OBJECT) [[likely]]
					m_frame_view.end_ptr = entry.m_container.object_data + entry.m_container.size;
				else
					m_frame_view.end_ptr = entry.m_container.array_data + entry.m_container.size;
			}

		public:
			read_frame() = delete;
			read_frame(const read_frame &) = delete;
			read_frame &operator=(const read_frame &) = delete;
			read_frame(read_frame &&) = delete;
			read_frame &operator=(read_frame &&) = delete;

			/** Returns iterator to the first entry of the currently read object or array. */
			[[nodiscard]] constexpr entry_iterator begin() const noexcept { return {m_frame_view.begin_ptr, m_type}; }
			/** @copydoc begin */
			[[nodiscard]] constexpr entry_iterator cbegin() const noexcept { return begin(); }
			/** Returns iterator one past the last entry of the currently read object or array. */
			[[nodiscard]] constexpr entry_iterator end() const noexcept { return {m_frame_view.end_ptr, m_type}; }
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
			 * @param args Arguments forwarded to the deserialization function.
			 * @return `true` if deserialization was successful, `false` otherwise. */
			template<typename T, typename... Args>
			bool try_read(T &&value, Args &&...args)
			{
				entry_iterator current{m_frame_view.current_ptr, m_type};
				if (current < end() && current->try_read(std::forward<T>(value), std::forward<Args>(args)...)) [[likely]]
				{
					m_frame_view.current_ptr = (current + 1).m_data_ptr;
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
			read_frame &operator>>(T &&value)
			{
				return read(std::forward<T>(value));
			}
			/** @copydoc operator>>
			 * @param args Arguments forwarded to the deserialization function. */
			template<typename T, typename... Args>
			read_frame &read(T &&value, Args &&...args)
			{
				entry_iterator current{m_frame_view.current_ptr, m_type};
				m_frame_view.current_ptr = (current + 1).m_data_ptr;
				current->read(std::forward<T>(value), std::forward<Args>(args)...);
				return *this;
			}
			/** @brief Deserializes an instance of `T` from the next Json entry of the archive in-place.
			 * Uses the in-place `deserialize` overload (taking `std::in_place_type_t<T>`)
			 * or constructor accepting the archive frame as one of it's arguments if available.
			 * Otherwise, default-constructs & deserializes using `read(T &&)`.
			 * @param args Arguments forwarded to the deserialization function/constructor.
			 * @return Deserialized instance of `T`.
			 * @throw archive_error On deserialization errors. */
			template<typename T, typename... Args>
			T read(std::in_place_type_t<T>, Args &&...args)
			{
				entry_iterator current{m_frame_view.current_ptr, m_type};
				m_frame_view.current_ptr = (current + 1).m_data_ptr;
				return current->read(std::in_place_type<T>, std::forward<Args>(args)...);
			}

			/** Deserializes the next Json entry using the keyed entry hint.
			 * @param value Named entry containing the entry key hint & forwarded entry value.
			 * @param args Arguments forwarded to the deserialization function.
			 * @return `true` if deserialization was successful, `false` otherwise. */
			template<typename T, typename... Args>
			bool try_read(keyed_entry_t<CharType, T> value, Args &&...args)
			{
				if (m_type == OBJECT) [[likely]]
				{
					if (seek_entry(value.key)) [[likely]]
						return try_read(std::forward<T>(value.value), std::forward<Args>(args)...);
				}
				return false;
			}
			/** Deserializes the next Json entry using the keyed entry hint.
			 * @param value Named entry containing the entry key hint & forwarded entry value.
			 * @param args Arguments forwarded to the deserialization function.
			 * @return Reference to this frame.
			 * @throw archive_exception On deserialization errors. */
			template<typename T>
			read_frame &operator>>(keyed_entry_t<CharType, T> value)
			{
				return read(value);
			}
			/** @copydoc operator>>
			 * @param args Arguments forwarded to the deserialization function. */
			template<typename T, typename... Args>
			read_frame &read(keyed_entry_t<CharType, T> value, Args &&...args)
			{
				if (m_type == ARRAY) [[unlikely]]
					throw archive_error("Named entry modifier cannot be applied to an array entry");

				if (!seek_entry(value.key)) [[unlikely]]
				{
					std::string err{"Invalid Json object member \""};
					err.append(value.key);
					err.append(1, '\"');
					throw std::out_of_range(err);
				}
				else
					read(std::forward<T>(value.value), std::forward<Args>(args)...);
				return *this;
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
			template<typename T>
			[[nodiscard]] constexpr const member_t *find_member(std::basic_string_view<CharType, T> key) const noexcept
			{
				for (auto member = m_frame_view.obj_begin(); member != m_frame_view.obj_end(); ++member)
					if (key == member->key) return member;
				return nullptr;
			}
			template<typename T>
			[[nodiscard]] constexpr const entry_t *seek_entry(std::basic_string_view<CharType, T> key) noexcept
			{
				if (m_frame_view.obj_current() >= m_frame_view.obj_end() || key != m_frame_view.obj_current()->key)
				{
					if (auto member_ptr = find_member(key); !member_ptr) [[unlikely]]
						return nullptr;
					else
						m_frame_view.current_ptr = member_ptr;
				}
				return &m_frame_view.obj_current()->value;
			}

			frame_view_t m_frame_view = {};
			entry_type m_type;
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
			constexpr write_frame(json_archive_base &parent, entry_t &entry) noexcept
				: m_parent(parent), m_current(entry)
			{
			}

		public:
			write_frame() = delete;
			write_frame(const write_frame &) = delete;
			write_frame &operator=(const write_frame &) = delete;
			write_frame(write_frame &&) = delete;
			write_frame &operator=(write_frame &&) = delete;

			/** Serialized the forwarded value to Json.
			 * @param value Value to serialize as Json.
			 * @return Reference to this frame. */
			template<typename T>
			write_frame &operator<<(T &&value)
			{
				return write(std::forward<T>(value));
			}
			/** @copydoc operator<<
			 * @param args Arguments forwarded to the serialization function. */
			template<typename T, typename... Args>
			write_frame &write(T &&value, Args &&...args)
			{
				write_impl(std::forward<T>(value), std::forward<Args>(args)...);
				return *this;
			}

		private:
			[[nodiscard]] CharType *alloc_string(std::size_t n) const
			{
				auto bytes = (n + 1) * sizeof(CharType);
				auto result = static_cast<CharType *>(m_parent.m_string_pool.allocate(bytes));
				if (!result) [[unlikely]]
					throw std::bad_alloc();
				return result;
			}
			template<typename T>
			[[nodiscard]] std::basic_string_view<CharType> copy_string(std::basic_string_view<CharType, T> str) const
			{
				auto result = alloc_string(str.size());
				*std::copy_n(str.data(), str.size(), result) = '\0';
				return {result, str.size()};
			}

			template<typename T>
			void resize_container(std::size_t n) const
			{
				auto *old_data = m_current.m_container.data_ptr;
				auto old_cap = m_current.m_container.capacity * sizeof(T), new_cap = n * sizeof(T);

				auto *new_data = m_parent.m_entry_pool.reallocate(old_data, old_cap, new_cap);
				if (!new_data) [[unlikely]]
					throw std::bad_alloc();

				m_current.m_container.data_ptr = new_data;
				m_current.m_container.capacity = n;
			}
			template<typename T>
			[[nodiscard]] T *push_container() const
			{
				auto next_idx = m_current.m_container.size;
				if (m_current.m_container.capacity == m_current.m_container.size++)
					resize_container<T>(m_current.m_container.size * 2);
				return static_cast<T *>(m_current.m_container.data_ptr) + next_idx;
			}
			[[nodiscard]] entry_t *next_entry() const
			{
				entry_t *entry;
				switch (m_current.m_type)
				{
					default:
					{
						m_current.m_type = OBJECT;
						[[fallthrough]];
					}
					case OBJECT:
					{
						auto member = push_container<member_t>();
						member->key = m_next_key;
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

			template<typename T, typename... Args>
			void write_value(entry_t &entry, T &&value, Args &&...args) const
			{
				write_frame frame{m_parent, entry};
				detail::do_serialize(std::forward<T>(value), frame, std::forward<Args>(args)...);
			}

			void write_value(entry_t &entry, std::nullptr_t) const { entry.m_type = NULL_VALUE; }
			template<typename T>
			void write_value(entry_t &entry, T &&b) const
				requires(std::same_as<std::decay_t<T>, bool>)
			{
				entry.m_type = static_cast<entry_type>(BOOL | static_cast<int>(!!b));
			}
			template<typename T>
			void write_value(entry_t &entry, T &&c) const
				requires(std::same_as<std::decay_t<T>, CharType> && ((Config & char_value) == char_value))
			{
				entry.m_type = CHAR;
				entry.m_literal.character = c;
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
			void write_value(entry_t &entry, I &&i) const
				requires is_uint_value<std::decay_t<I>>
			{
				entry.m_type = static_cast<entry_type>(INT_U | int_size_type(i));
				entry.m_literal.ui = static_cast<std::uintmax_t>(i);
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
			void write_value(entry_t &entry, I &&i) const
				requires is_int_value<std::decay_t<I>>
			{
				entry.m_type = static_cast<entry_type>(INT_S | int_size_type(i));
				entry.m_literal.si = static_cast<std::intmax_t>(i);
			}

			template<typename F>
			void write_value(entry_t &entry, F &&f) const
				requires std::floating_point<std::decay_t<F>>
			{
				if constexpr (sizeof(F) > sizeof(float))
				{
					entry.m_type = FLOAT64;
					entry.m_literal.f64 = static_cast<double>(f);
				}
				else
				{
					entry.m_type = FLOAT32;
					entry.m_literal.f32 = static_cast<float>(f);
				}
			}

			template<typename T>
			void write_value(entry_t &entry, std::basic_string_view<CharType, T> sv) const
			{
				entry.m_type = STRING;
				entry.m_string = copy_string(sv);
			}
			void write_value(entry_t &entry, const CharType *str) const
			{
				write_value(entry, std::basic_string_view<CharType>{str});
			}
			template<typename T, typename A>
			void write_value(entry_t &entry, std::basic_string<CharType, T, A> &&str) const
			{
				write_value(entry, std::basic_string_view<CharType>{str});
			}
			template<typename T, typename A>
			void write_value(entry_t &entry, std::basic_string<CharType, T, A> &str) const
			{
				write_value(entry, std::basic_string_view<CharType>{str});
			}
			template<typename T, typename A>
			void write_value(entry_t &entry, const std::basic_string<CharType, T, A> &str) const
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
			void write_value(entry_t &entry, T &&str) const
				requires std::convertible_to<T, const CharType *>
			{
				write_value(entry, static_cast<const CharType *>(str));
			}

			template<typename T, typename... Args>
			void write_value(T &&value, Args &&...args) const
			{
				auto entry = next_entry();
				SEK_ASSERT(entry != nullptr);

				write_value(*entry, std::forward<T>(value), std::forward<Args>(args)...);

				if constexpr ((Config & container_types) == container_types)
				{
					/* Set container value type if all entries are of the same type.
					 * Use a different condition for integers to account for size-category. */
					if (m_current.m_container.value_type == NO_TYPE) [[unlikely]]
						m_current.m_container.value_type = entry->m_type;
					else if constexpr (!std::is_integral_v<std::decay_t<T>>)
					{
						if (m_current.m_container.value_type != entry->m_type) [[likely]]
							m_current.m_container.value_type = DYNAMIC;
					}
					else
					{
						/* If the current type is also an integer of the same signedness, use the largest size category. */
						if ((m_current.m_container.value_type & INT_S) == (entry->m_type & INT_S))
						{
							if (m_current.m_container.value_type < entry->m_type) [[unlikely]]
								m_current.m_container.value_type = entry->m_type;
						}
						else
							m_current.m_container.value_type = DYNAMIC;
					}
				}
			}

			template<typename T, typename... Args>
			void write_impl(T &&value, Args &&...args)
			{
				if (m_current.m_type != ARRAY) [[likely]]
					m_next_key = generate_key<CharType>(m_parent.m_string_pool, m_current.m_container.size);
				write_value(std::forward<T>(value), std::forward<Args>(args)...);
			}
			template<typename T, typename... Args>
			void write_impl(keyed_entry_t<CharType, T> value, Args &&...args)
			{
				if (m_current.m_type != ARRAY) [[likely]]
					m_next_key = generate_key<CharType>(m_parent.m_string_pool, value.key);
				write_value(std::forward<T>(value.value), std::forward<Args>(args)...);
			}
			template<typename T>
			void write_impl(container_size_t<T> size)
			{
				switch (m_current.m_type)
				{
					default:
					{
						m_current.m_type = OBJECT;
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
				SEK_ASSERT(m_current.m_type != OBJECT, "Array mode modifier applied to object entry");

				m_current.m_type = ARRAY;
			}

			json_archive_base &m_parent;
			entry_t &m_current;
			std::basic_string_view<CharType> m_next_key = {};
		};

		using entry_pool_t = dynamic_buffer_resource<sizeof(entry_t) * 64>;
		using string_pool_t = dynamic_buffer_resource<SEK_KB(1)>;

		json_archive_base() = delete;
		json_archive_base(const json_archive_base &) = delete;
		json_archive_base &operator=(const json_archive_base &) = delete;

		explicit json_archive_base(std::pmr::memory_resource *res) noexcept
			: m_upstream(res), m_entry_pool(res), m_string_pool(res)
		{
		}
		constexpr json_archive_base(json_archive_base &&other) noexcept
			: m_upstream(std::exchange(other.m_upstream, nullptr)),
			  m_entry_pool(std::move(other.m_entry_pool)),
			  m_string_pool(std::move(other.m_string_pool)),
			  m_top_level()
		{
			m_top_level.swap(other.m_top_level);
		}
		constexpr json_archive_base &operator=(json_archive_base &&other) noexcept
		{
			swap(other);
			return *this;
		}
		~json_archive_base() = default;

		void reset()
		{
			m_entry_pool.release();
			m_string_pool.release();
			::new (&m_top_level) entry_t{};
		}
		void reset(std::pmr::memory_resource *res)
		{
			m_entry_pool = entry_pool_t{res};
			m_string_pool = string_pool_t{res};
			::new (&m_top_level) entry_t{};
		}

		template<typename T, typename... Args>
		bool do_try_read(T &&value, Args &&...args)
		{
			return m_top_level.try_read(std::forward<T>(value), std::forward<Args>(args)...);
		}
		template<typename T, typename... Args>
		void do_read(T &&value, Args &&...args)
		{
			m_top_level.read(std::forward<T>(value), std::forward<Args>(args)...);
		}
		template<typename T, typename... Args>
		T do_read(std::in_place_type_t<T>, Args &&...args)
		{
			return m_top_level.read(std::in_place_type<T>, std::forward<Args>(args)...);
		}

		template<typename T, typename... Args>
		void do_write(T &&value, Args &&...args)
		{
			write_frame frame{*this, m_top_level};
			detail::do_serialize(std::forward<T>(value), frame, std::forward<Args>(args)...);
		}
		void do_flush(auto &emitter) const
		{
			if (m_top_level.m_type != NO_TYPE) [[likely]]
				m_top_level.emit(emitter);
		}

		constexpr void swap(json_archive_base &other) noexcept
		{
			std::swap(m_upstream, other.m_upstream);
			m_entry_pool.swap(other.m_entry_pool);
			m_string_pool.swap(other.m_string_pool);
			m_top_level.swap(other.m_top_level);
		}

		std::pmr::memory_resource *m_upstream;

		entry_pool_t m_entry_pool;	 /* Allocation pool used for entry tree allocation. */
		string_pool_t m_string_pool; /* Allocation pool used for string allocation. */
		entry_t m_top_level = {};	 /* Top-most entry of the Json tree. */
	};

	template<typename C, json_archive_config Cfg>
	template<typename T, typename... Args>
	constexpr const typename json_archive_base<C, Cfg>::entry_t &json_archive_base<C, Cfg>::entry_t::read(T &&v, Args &&...args) const
	{
		if (!(m_type & CONTAINER)) [[unlikely]]
			throw archive_error("Invalid Json type, expected array or object");

		read_frame frame{*this};
		detail::do_deserialize(std::forward<T>(v), frame, std::forward<Args>(args)...);
		return *this;
	}
	template<typename C, json_archive_config Cfg>
	template<typename T, typename... Args>
	constexpr T json_archive_base<C, Cfg>::entry_t::read(std::in_place_type_t<T>, Args &&...args) const
	{
		if (!(m_type & CONTAINER)) [[unlikely]]
			throw archive_error("Invalid Json type, expected array or object");

		if constexpr (in_place_deserializable<T, read_frame, Args...> || std::is_constructible_v<T, read_frame &, Args...> ||
					  in_place_deserializable<T, read_frame> || std::is_constructible_v<T, read_frame &>)
		{
			read_frame frame{*this};
			return detail::do_deserialize(std::in_place_type<T>, frame, std::forward<Args>(args)...);
		}
		else
		{
			T result{};
			read(result, std::forward<Args>(args)...);
			return result;
		}
	}
}	 // namespace sek::serialization::detail