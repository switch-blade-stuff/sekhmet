/*
 * Created by switchblade on 2022-04-14
 */

#pragma once

#include "../archive_reader.hpp"
#include "../archive_writer.hpp"
#include "../manipulators.hpp"
#include "../node_tree.hpp"

namespace sek::serialization
{
	namespace detail
	{
		using namespace sek::detail;

		typedef int json_archive_config;

		constexpr json_archive_config container_types = 1;
		constexpr json_archive_config char_value = 2;

		enum json_type : int
		{
			NO_TYPE = 0,
			DYNAMIC = 1,

			BOOL = 2,
			BOOL_FALSE = BOOL | 0,
			BOOL_TRUE = BOOL | 1,

			NULL_VALUE = 4,
			CHAR = 5,
			STRING = 6,

			INT_TYPE = 8,
			INT_SIGN_BIT = 16,
			INT_U = INT_TYPE,
			INT_S = INT_TYPE | INT_SIGN_BIT,
			INT_SIZE_MASK = 7,
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

			FLOAT_TYPE = 32,
			FLOAT32 = FLOAT_TYPE | 0,
			FLOAT64 = FLOAT_TYPE | 1,
		};
		template<typename C>
		union json_node_value
		{
			typedef json_type type_selector;

			std::basic_string_view<C> string;
			C character;

			std::intmax_t si;
			std::uintmax_t ui;

			double fp;
		};
		template<typename C, typename T = std::char_traits<C>>
		using basic_json_tree = basic_node_tree<C, json_node_value<C>, void, T>;

		template<typename C, typename T = std::char_traits<C>, json_archive_config Config = 0>
		struct json_archive_base
		{
			using tree_type = basic_json_tree<C, T>;
			using tree_node = typename tree_type::node_type;
			using value_node = typename tree_type::value_node;
			using array_node = typename tree_type::array_node;
			using table_node = typename tree_type::table_node;

			class entry_t;
			class entry_iterator;
			class read_frame;
			class write_frame;
			class parser_base;

			constexpr static void emit_value(const tree_node &node, auto &emitter)
			{
				const auto type = node.type.value;
				const auto &value = node.value();

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
							emitter.on_char(value.character);
							break;
						}
						else
							SEK_NEVER_REACHED;
					}

					case INT_S64:
					case INT_S32:
					case INT_S16:
					case INT_S8: emitter.on_int(type, value.si); break;
					case INT_U64:
					case INT_U32:
					case INT_U16:
					case INT_U8: emitter.on_uint(type, value.ui); break;

					case FLOAT32: emitter.on_float32(static_cast<float>(value.fp)); break;
					case FLOAT64: emitter.on_float64(value.fp); break;

					case STRING: emitter.on_string(value.string.data(), value.string.size()); break;
					default: break;
				}
			}
			constexpr static void emit_array(const array_node &array, auto &emitter)
			{
				auto frame = emitter.enter_frame();
				{
					emitter.on_array_start(array.size(), array.element_type);
					for (auto &element : array) emit_node(element.value, emitter);
					emitter.on_array_end();
				}
				emitter.exit_frame(frame);
			}
			constexpr static void emit_table(const table_node &table, auto &emitter)
			{
				auto frame = emitter.enter_frame();
				{
					emitter.on_object_start(table.size(), table.element_type);
					for (auto &element : table)
					{
						emitter.on_object_key(element.key.data(), element.key.size());
						emit_node(element.value, emitter);
					}
					emitter.on_object_end();
				}
				emitter.exit_frame(frame);
			}
			constexpr static void emit_node(const tree_node &node, auto &emitter)
			{
				switch (node.type.storage)
				{
					case tree_type::type_selector::VALUE: emit_value(node, emitter); break;
					case tree_type::type_selector::ARRAY: emit_array(node.array(), emitter); break;
					case tree_type::type_selector::TABLE: emit_table(node.table(), emitter); break;
					default: throw archive_error("Invalid node type");
				}
			}

			/** @brief Structure used to represent a Json entry. */
			class entry_t : tree_node
			{
				friend struct json_archive_base;

				friend class parser_base;
				friend class read_frame;
				friend class write_frame;

				static void throw_string_error() { throw archive_error("Invalid Json type, expected string"); }

				/* Must only be accessible from friends. */
				constexpr entry_t() noexcept : tree_node() { tree_node::type.value = NO_TYPE; }

			public:
				entry_t(const entry_t &) = delete;
				entry_t &operator=(const entry_t &) = delete;
				entry_t(entry_t &&) = delete;
				entry_t &operator=(entry_t &&) = delete;

				/** Reads a null value from the entry. Returns `true` if the entry contains a null value, `false` otherwise. */
				constexpr bool try_read(std::nullptr_t, auto &&...) const noexcept
				{
					return value_type() == NULL_VALUE;
				}
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
					if (value_type() & BOOL) [[likely]]
					{
						b = value_type() & 1;
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
				constexpr bool try_read(C &c, auto &&...) const noexcept
					requires((Config & char_value) == char_value)
				{
					if (value_type() == CHAR) [[likely]]
					{
						c = character();
						return true;
					}
					else
						return false;
				}
				/** Reads a character from the entry.
				 * @throw archive_error If the entry does not contain a character. */
				constexpr const entry_t &read(C &c, auto &&...) const
					requires((Config & char_value) == char_value)
				{
					if (!try_read(c)) [[unlikely]]
						throw archive_error("Invalid Json type, expected char");
					return *this;
				}
				/** @copydoc read */
				constexpr C read(std::in_place_type_t<C>, auto &&...) const
					requires((Config & char_value) == char_value)
				{
					C result;
					read(result);
					return result;
				}

				/** Reads a number from the entry. Returns `true` if the entry contains a number, `false` otherwise. */
				template<typename I>
				constexpr bool try_read(I &value, auto &&...) const noexcept
					requires(std::integral<I> || std::floating_point<I>)
				{
					if (value_type() & INT_TYPE)
					{
						if (value_type() & INT_SIGN_BIT)
							value = static_cast<I>(si());
						else
							value = static_cast<I>(ui());
						return true;
					}
					else if (value_type() & FLOAT_TYPE)
					{
						value = static_cast<I>(fp());
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
				template<typename U = std::char_traits<C>, typename A = std::allocator<C>>
				constexpr bool try_read(std::basic_string<C, T, A> &value, auto &&...) const
				{
					if (value_type() == STRING) [[likely]]
					{
						value.assign(string());
						return true;
					}
					else
						return false;
				}
				/** Reads a string from the entry.
				 * @param value STL string view to assign the string value to.
				 * @copydoc `true` if the entry contains a string, `false` otherwise. */
				template<typename U = std::char_traits<C>>
				constexpr bool try_read(std::basic_string_view<C, T> &value, auto &&...) const noexcept
				{
					if (value_type() == STRING) [[likely]]
					{
						if constexpr (std::same_as<std::basic_string_view<C, T>, std::basic_string_view<C>>)
							value = string();
						else
							value = {string().begin(), string().end()};
						return true;
					}
					else
						return false;
				}
				/** Reads a string from the entry.
				 * @param value Output iterator used to write the string value to.
				 * @copydoc `true` if the entry contains a string, `false` otherwise. */
				template<std::output_iterator<C> I>
				constexpr bool try_read(I &value, auto &&...) const
				{
					if (value_type() == STRING) [[likely]]
					{
						std::copy_n(string().data(), string().size(), value);
						return true;
					}
					else
						return false;
				}
				/** Reads a string from the entry.
				 * @param value Output iterator used to write the string value to.
				 * @param sent Sentinel for the output iterator.
				 * @copydoc `true` if the entry contains a string, `false` otherwise. */
				template<std::output_iterator<C> I, std::sentinel_for<I> S>
				constexpr bool try_read(I &value, S &sent, auto &&...) const
				{
					if (value_type() == STRING) [[likely]]
					{
						for (std::size_t i = 0; i != string().size() && value != sent; ++i, ++value)
							*value = string().data()[i];
						return true;
					}
					else
						return false;
				}

				/** Reads a string from the entry.
				 * @param value STL string to assign the string value to.
				 * @return Reference to this entry.
				 * @throw archive_error If the entry does not contain a string. */
				template<typename U = std::char_traits<C>, typename A = std::allocator<C>>
				constexpr const entry_t &read(std::basic_string<C, U, A> &value, auto &&...) const
				{
					if (!try_read(value)) [[unlikely]]
						throw_string_error();
					return *this;
				}
				/** @copydoc read */
				template<typename U = std::char_traits<C>, typename A = std::allocator<C>>
				constexpr std::basic_string<C, U, A> read(std::in_place_type_t<std::basic_string<C, U, A>>, auto &&...) const
				{
					std::basic_string<C, U, A> result;
					read(result);
					return result;
				}
				/** Reads a string from the entry.
				 * @param value STL string view to assign the string value to.
				 * @return Reference to this entry.
				 * @throw archive_error If the entry does not contain a string. */
				template<typename U = std::char_traits<C>>
				constexpr const entry_t &read(std::basic_string_view<C, U> &value, auto &&...) const
				{
					if (!try_read(value)) [[unlikely]]
						throw_string_error();
					return *this;
				}
				/** @copydoc read */
				template<typename U = std::char_traits<C>>
				constexpr std::basic_string_view<C, U> read(std::in_place_type_t<std::basic_string_view<C, U>>, auto &&...) const
				{
					std::basic_string_view<C, U> result;
					read(result);
					return result;
				}
				/** Reads a string from the entry.
				 * @param value Output iterator used to write the string value to.
				 * @return Reference to this entry.
				 * @throw archive_error If the entry does not contain a string. */
				template<std::output_iterator<C> I>
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
				template<std::output_iterator<C> I, std::sentinel_for<I> S>
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
				template<typename U>
				constexpr const entry_t &operator>>(U &&value) const
				{
					return read(std::forward<U>(value));
				}
				/** @copydoc operator>>
				 * @param args Arguments forwarded to the deserialization function. */
				template<typename U, typename... Args>
				constexpr const entry_t &read(U &&value, Args &&...args) const;
				/** @brief Reads an object or array from the entry in-place.
				 * Uses the in-place `deserialize` overload (taking `std::in_place_type_t<T>`)
				 * or constructor accepting the archive frame as one of it's arguments if available.
				 * Otherwise, default-constructs & deserializes using `read(T &&)`.
				 * @param args Arguments forwarded to the deserialization function.
				 * @return Deserialized instance of `T`.
				 * @throw archive_error On deserialization errors. */
				template<typename U, typename... Args>
				constexpr U read(std::in_place_type_t<U>, Args &&...args) const;
				/** Attempts to read an object or array from the entry.
				 * @param value Forwarded value to be read from the entry.
				 * @param args Arguments forwarded to the deserialization function.
				 * @return `true` if read successfully, `false` otherwise. */
				template<typename U, typename... Args>
				constexpr bool try_read(U &&value, Args &&...args) const
				{
					try
					{
						read(std::forward<U>(value), std::forward<Args>(args)...);
						return true;
					}
					catch ([[maybe_unused]] archive_error &e)
					{
						return false;
					}
				}

			private:
				[[nodiscard]] constexpr auto value_type() const noexcept { return tree_node::type.value; }
				[[nodiscard]] constexpr auto string() const noexcept { return tree_node::value().string; }
				[[nodiscard]] constexpr auto character() const noexcept { return tree_node::value().character; }
				[[nodiscard]] constexpr auto si() const noexcept { return tree_node::value().si; }
				[[nodiscard]] constexpr auto ui() const noexcept { return tree_node::value().ui; }
				[[nodiscard]] constexpr auto fp() const noexcept { return tree_node::value().fp; }
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
					union
					{
						void *ptr = nullptr;
						array_node *array;
						table_node *table;
					};
					std::ptrdiff_t pos;
					parse_state state;
				};

			public:
				constexpr explicit parser_base(json_archive_base &parent) noexcept : m_parent(parent) {}
				~parser_base()
				{
					if (m_parse_stack) [[likely]]
						m_parent.upstream->deallocate(m_parse_stack, m_stack_capacity * sizeof(parse_frame));
				}

				template<std::integral S>
				[[nodiscard]] C *on_string_alloc(S len) const
				{
					return m_parent.tree->alloc_string(static_cast<std::size_t>(len));
				}

				bool on_null() const
				{
					return on_value([](tree_node &entry) { entry.to_value().type.value = NULL_VALUE; });
				}
				bool on_bool(bool b) const
				{
					return on_value(
						[b](tree_node &entry)
						{
							const auto bool_type = static_cast<json_type>(BOOL | (b ? 1 : 0));
							entry.to_value().type.value = bool_type;
						});
				}
				bool on_true() const
				{
					return on_value([](tree_node &entry) { entry.to_value().type.value = BOOL_TRUE; });
				}
				bool on_false() const
				{
					return on_value([](tree_node &entry) { entry.to_value().type.value = BOOL_FALSE; });
				}
				bool on_char(C c) const
				{
					return on_value(
						[c](tree_node &entry)
						{
							entry.to_value().type.value = CHAR;
							entry.value().character = c;
						});
				}
				template<std::integral I>
				bool on_int(I i) const
				{
					auto do_set_int = [i](tree_node &entry)
					{
						if constexpr (std::is_signed_v<I>)
						{
							entry.to_value().type.value = INT_S;
							entry.value().si = static_cast<std::intmax_t>(i);
						}
						else
						{
							entry.to_value().type.value = INT_U;
							entry.value().ui = static_cast<std::uintmax_t>(i);
						}
					};

					return on_value(do_set_int);
				}
				template<std::floating_point F>
				bool on_float(F f) const
				{
					return on_value(
						[f](tree_node &entry)
						{
							entry.to_value().type.value = FLOAT_TYPE;
							entry.value().fp = static_cast<double>(f);
						});
				}

				template<std::integral S>
				bool on_string(const C *str, S len) const
				{
					return on_value(
						[&](tree_node &entry)
						{
							entry.to_value().type.value = STRING;
							entry.value().string = std::basic_string_view<C>{str, static_cast<std::size_t>(len)};
						});
				}
				template<std::integral S>
				bool on_string_copy(const C *str, S len) const
				{
					auto dest = on_string_alloc(len);
					*std::copy_n(str, static_cast<std::size_t>(len), dest) = '\0';
					return on_string(dest, len);
				}

				bool on_object_start(std::size_t n = 0)
				{
					auto do_start_object = [&](tree_node &entry)
					{
						enter_frame();
						m_current->table = &entry.to_table().table();
						m_current->state = parse_state::EXPECT_OBJECT_KEY;
						if (n) reserve_container(*m_current->table, n);
					};

					if (!m_parse_stack) [[unlikely]] /* If stack is empty, this is the top-level object. */
					{
						do_start_object(m_parent.tree->top_level);
						return true;
					}
					else
						return on_value(do_start_object);
				}
				template<std::integral S>
				bool on_object_key(const C *str, S len)
				{
					switch (m_current->state)
					{
						case parse_state::EXPECT_OBJECT_KEY:
						{
							push_container(*m_current->table).key =
								std::basic_string_view<C>{str, static_cast<std::size_t>(len)};
							m_current->state = parse_state::EXPECT_OBJECT_VALUE; /* Always expect value after key. */
							return true;
						}
						default: return false;
					}
				}
				template<std::integral S>
				bool on_object_key_copy(const C *str, S len)
				{
					auto dest = on_string_alloc(len);
					*std::copy_n(str, static_cast<std::size_t>(len), dest) = '\0';
					return on_object_key(dest, len);
				}
				template<std::integral S>
				bool on_object_end(S /*size*/)
				{
					switch (m_current->state)
					{
						case parse_state::EXPECT_OBJECT_KEY: exit_frame(); return true;
						default: return false;
					}
				}

				bool on_array_start(std::size_t n = 0)
				{
					auto do_start_array = [&](tree_node &entry)
					{
						enter_frame();
						m_current->array = &entry.to_array().array();
						m_current->state = parse_state::EXPECT_ARRAY_VALUE;
						if (n) reserve_container(*m_current->array, n);
					};

					if (!m_parse_stack) [[unlikely]] /* If stack is empty, this is the top-level array. */
					{
						do_start_array(m_parent.tree->top_level);
						return true;
					}
					else
						return on_value(do_start_array);
				}
				template<std::integral S>
				bool on_array_end(S /*size*/)
				{
					switch (m_current->state)
					{
						case parse_state::EXPECT_ARRAY_VALUE: exit_frame(); return true;
						default: return false;
					}
				}

				void reserve_container(auto &node, std::size_t n) const
				{
					node = m_parent.tree->reserve_container(node, n);
				}
				[[nodiscard]] auto &push_container(auto &node) const
				{
					const auto old_pos = m_current->pos++;
					if (static_cast<std::size_t>(old_pos) >= node.capacity()) [[unlikely]]
						reserve_container(node, node.empty() ? 1 : node.size() * 2);

					node.insert(node.cbegin() + old_pos, {});
					return node.begin()[old_pos];
				}

				bool on_value(auto f) const
				{
					tree_node *node;
					switch (m_current->state)
					{
						case parse_state::EXPECT_ARRAY_VALUE:
						{
							node = &push_container(*m_current->array).value;
							break;
						}
						case parse_state::EXPECT_OBJECT_VALUE:
						{
							/* Size is updated by the key event. */
							node = &(m_current->table->begin()[m_current->pos - 1].value);
							m_current->state = parse_state::EXPECT_OBJECT_KEY;
							break;
						}
						default: return false;
					}

					f(*node);
					return true;
				}
				void enter_frame()
				{
					if (!m_parse_stack) [[unlikely]]
					{
						auto new_stack = static_cast<parse_frame *>(m_parent.upstream->allocate(4 * sizeof(parse_frame)));
						if (!new_stack) [[unlikely]]
							throw std::bad_alloc();
						m_current = m_parse_stack = new_stack;
						m_stack_capacity = 4;
					}
					else if (auto pos = ++m_current - m_parse_stack; static_cast<std::size_t>(pos) == m_stack_capacity)
						[[unlikely]]
					{
						auto new_cap = m_stack_capacity * 2;
						auto new_stack =
							static_cast<parse_frame *>(m_parent.upstream->allocate(new_cap * sizeof(parse_frame)));
						if (!new_stack) [[unlikely]]
							throw std::bad_alloc();

						m_current = std::copy_n(m_parse_stack, pos, new_stack);

						m_parent.upstream->deallocate(m_parse_stack, m_stack_capacity * sizeof(parse_frame));
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
				constexpr entry_iterator(typename array_node::const_iterator array_pos,
										 typename tree_type::type_selector type) noexcept
					: m_array_pos(array_pos), m_type(type)
				{
				}
				constexpr entry_iterator(typename table_node::const_iterator table_pos,
										 typename tree_type::type_selector type) noexcept
					: m_table_pos(table_pos), m_type(type)
				{
				}

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
				[[nodiscard]] constexpr pointer get() const noexcept { return static_cast<pointer>(get_value()); }
				/** @copydoc get */
				[[nodiscard]] constexpr pointer operator->() const noexcept { return get(); }
				/** Returns reference to the associated entry. */
				[[nodiscard]] constexpr reference operator*() const noexcept { return *get(); }
				/** Returns reference to the entry at `n` offset from the iterator. */
				[[nodiscard]] constexpr reference operator[](difference_type n) const noexcept { return get()[n]; }

				/** Checks if the associated entry has a key. */
				[[nodiscard]] constexpr bool has_key() const noexcept { return is_table(); }
				/** Returns the key of the associated entry.
				 * If the pointed-to entry is not a keyed entry (object member) returns an empty string view. */
				[[nodiscard]] constexpr std::basic_string_view<C> key(std::nothrow_t) const noexcept
				{
					if (is_table()) [[unlikely]]
						return {};
					else
						return m_table_pos->key;
				}
				/** Returns the key of the associated entry.
				 * @throw archive_error If the pointed-to entry is not a keyed entry (object member). */
				[[nodiscard]] constexpr std::basic_string_view<C> key() const
				{
					if (!is_table()) [[unlikely]]
						throw archive_error("Entry iterator does not point to a keyed entry");
					else
						return m_table_pos->key;
				}

				[[nodiscard]] friend constexpr difference_type operator-(entry_iterator a, entry_iterator b) noexcept
				{
					SEK_ASSERT(a.m_type == b.m_type);
					switch (a.m_type.storage)
					{
						case tree_type::type_selector::ARRAY: return a.m_array_pos - b.m_array_pos;
						case tree_type::type_selector::TABLE: return a.m_table_pos - b.m_table_pos;
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
				[[nodiscard]] constexpr const tree_node *get_value() const noexcept
				{
					switch (m_type.storage)
					{
						case tree_type::type_selector::ARRAY: return &m_array_pos->value;
						case tree_type::type_selector::TABLE: return &m_table_pos->value;
						default: return nullptr;
					}
				}
				constexpr void move_n(difference_type n) noexcept
				{
					switch (m_type.storage)
					{
						case tree_type::type_selector::ARRAY: m_array_pos += n; break;
						case tree_type::type_selector::TABLE: m_table_pos += n; break;
						default: break;
					}
				}

				[[nodiscard]] constexpr bool is_array() const noexcept
				{
					return m_type.storage == tree_type::type_selector::ARRAY;
				}
				[[nodiscard]] constexpr bool is_table() const noexcept
				{
					return m_type.storage == tree_type::type_selector::TABLE;
				}

				union
				{
					void *m_data_ptr = nullptr; /* Pointer used for iterator comparison purposes. */
					typename array_node::const_iterator m_array_pos;
					typename table_node::const_iterator m_table_pos;
				};
				/** Type of the frame this iterator was created from. */
				typename tree_type::type_selector m_type;
			};
			/** @brief Helper structure used as the API interface for Json input archive operations. */
			class read_frame
			{
				friend struct json_archive_base;
				friend class entry_t;

			public:
				typedef input_archive_category archive_category;
				typedef C char_type;

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
					union
					{
						const void *begin_ptr = nullptr;
						typename array_node::const_iterator array_begin;
						typename table_node::const_iterator table_begin;
					};
					union
					{
						const void *current_ptr = nullptr;
						typename array_node::const_iterator array_pos;
						typename table_node::const_iterator table_pos;
					};
					union
					{
						const void *end_ptr = nullptr;
						typename array_node::const_iterator array_end;
						typename table_node::const_iterator table_end;
					};
				};

				constexpr explicit read_frame(const tree_node &node) noexcept : m_type(node.type)
				{
					switch (node.type.storage)
					{
						case tree_type::type_selector::ARRAY:
						{
							auto &array = node.array();
							m_frame_view.array_begin = array.cbegin();
							m_frame_view.array_pos = array.cbegin();
							m_frame_view.array_end = array.cend();
							break;
						}
						case tree_type::type_selector::TABLE:
						{
							auto &table = node.table();
							m_frame_view.table_begin = table.cbegin();
							m_frame_view.table_pos = table.cbegin();
							m_frame_view.table_end = table.cend();
							break;
						}
						default: break;
					}
				}

			public:
				read_frame() = delete;
				read_frame(const read_frame &) = delete;
				read_frame &operator=(const read_frame &) = delete;
				read_frame(read_frame &&) = delete;
				read_frame &operator=(read_frame &&) = delete;

				/** Returns iterator to the first entry of the currently read object or array. */
				[[nodiscard]] constexpr entry_iterator begin() const noexcept
				{
					entry_iterator result;
					switch (m_type.storage)
					{
						case tree_type::type_selector::ARRAY: result = {m_frame_view.array_begin, m_type}; break;
						case tree_type::type_selector::TABLE: result = {m_frame_view.table_begin, m_type}; break;
						default: break;
					}
					return result;
				}
				/** @copydoc begin */
				[[nodiscard]] constexpr entry_iterator cbegin() const noexcept { return begin(); }
				/** Returns iterator one past the last entry of the currently read object or array. */
				[[nodiscard]] constexpr entry_iterator end() const noexcept
				{
					entry_iterator result;
					switch (m_type.storage)
					{
						case tree_type::type_selector::ARRAY: result = {m_frame_view.array_end, m_type}; break;
						case tree_type::type_selector::TABLE: result = {m_frame_view.table_end, m_type}; break;
						default: break;
					}
					return result;
				}
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
				[[nodiscard]] constexpr size_type size() const noexcept
				{
					return static_cast<size_type>(end() - begin());
				}
				/** Returns the max possible size of an object or array. */
				[[nodiscard]] constexpr size_type max_size() const noexcept
				{
					return static_cast<size_type>(std::numeric_limits<std::uint32_t>::max());
				}

				/** Attempts to deserialize the next Json entry of the archive & advance the entry.
				 * @param value Value to deserialize.
				 * @param args Arguments forwarded to the deserialization function.
				 * @return `true` if deserialization was successful, `false` otherwise. */
				template<typename U, typename... Args>
				bool try_read(U &&value, Args &&...args)
				{
					auto current = pos();
					if (current < end() && current->try_read(std::forward<U>(value), std::forward<Args>(args)...)) [[likely]]
					{
						move_next();
						return true;
					}
					else
						return false;
				}
				/** Deserializes the next Json entry of the archive & advance the entry.
				 * @param value Value to deserialize.
				 * @return Reference to this frame.
				 * @throw archive_exception On deserialization errors. */
				template<typename U>
				read_frame &operator>>(U &&value)
				{
					return read(std::forward<U>(value));
				}
				/** @copydoc operator>>
				 * @param args Arguments forwarded to the deserialization function. */
				template<typename U, typename... Args>
				read_frame &read(U &&value, Args &&...args)
				{
					next()->read(std::forward<U>(value), std::forward<Args>(args)...);
					return *this;
				}
				/** @brief Deserializes an instance of `T` from the next Json entry of the archive in-place.
				 * Uses the in-place `deserialize` overload (taking `std::in_place_type_t<T>`)
				 * or constructor accepting the archive frame as one of it's arguments if available.
				 * Otherwise, default-constructs & deserializes using `read(T &&)`.
				 * @param args Arguments forwarded to the deserialization function/constructor.
				 * @return Deserialized instance of `T`.
				 * @throw archive_error On deserialization errors. */
				template<typename U, typename... Args>
				U read(std::in_place_type_t<U>, Args &&...args)
				{
					return next()->read(std::in_place_type<U>, std::forward<Args>(args)...);
				}

				/** Deserializes the next Json entry using the keyed entry hint.
				 * @param value Named entry containing the entry key hint & forwarded entry value.
				 * @param args Arguments forwarded to the deserialization function.
				 * @return `true` if deserialization was successful, `false` otherwise. */
				template<typename U, typename... Args>
				bool try_read(keyed_entry_t<C, U> value, Args &&...args)
				{
					if (m_type.storage == tree_type::type_selector::TABLE) [[likely]]
					{
						if (seek_node(value.key)) [[likely]]
							return try_read(std::forward<U>(value.value), std::forward<Args>(args)...);
					}
					return false;
				}
				/** Deserializes the next Json entry using the keyed entry hint.
				 * @param value Named entry containing the entry key hint & forwarded entry value.
				 * @param args Arguments forwarded to the deserialization function.
				 * @return Reference to this frame.
				 * @throw archive_exception On deserialization errors.
				 * @throw std::out_of_range If the object does not contain an entry with the specified key. */
				template<typename U>
				read_frame &operator>>(keyed_entry_t<C, U> value)
				{
					return read(value);
				}
				/** @copydoc operator>>
				 * @param args Arguments forwarded to the deserialization function. */
				template<typename U, typename... Args>
				read_frame &read(keyed_entry_t<C, U> value, Args &&...args)
				{
					if (m_type.storage != tree_type::type_selector::TABLE) [[unlikely]]
						throw archive_error("Named entry modifier can only be applied to an object");

					if (!seek_node(value.key)) [[unlikely]]
					{
						std::string err{"Invalid Json object member \""};
						err.append(value.key);
						err.append(1, '\"');
						throw std::out_of_range(err);
					}
					else
						read(std::forward<U>(value.value), std::forward<Args>(args)...);
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
				template<typename U>
				[[nodiscard]] constexpr auto find_member(std::basic_string_view<C, U> key) const noexcept
				{
					for (auto entry = m_frame_view.table_begin; entry != m_frame_view.table_end; ++entry)
						if (key == entry->key) return entry;
					return m_frame_view.table_end;
				}
				template<typename U>
				[[nodiscard]] constexpr const tree_node *seek_node(std::basic_string_view<C, U> key) noexcept
				{
					if (m_frame_view.table_pos >= m_frame_view.table_end || key != m_frame_view.table_pos->key)
					{
						if (auto member = find_member(key); member == m_frame_view.table_end) [[unlikely]]
							return nullptr;
						else
							m_frame_view.table_pos = member;
					}
					return &m_frame_view.table_pos->value;
				}

				[[nodiscard]] constexpr auto next() noexcept
				{
					entry_iterator result;
					switch (m_type.storage)
					{
						case tree_type::type_selector::ARRAY: result = {m_frame_view.array_pos++, m_type}; break;
						case tree_type::type_selector::TABLE: result = {m_frame_view.table_pos++, m_type}; break;
						default: break;
					}
					return result;
				}
				[[nodiscard]] constexpr auto pos() noexcept
				{
					entry_iterator result;
					switch (m_type.storage)
					{
						case tree_type::type_selector::ARRAY: result = {m_frame_view.array_pos, m_type}; break;
						case tree_type::type_selector::TABLE: result = {m_frame_view.table_pos, m_type}; break;
						default: break;
					}
					return result;
				}
				constexpr void move_next() noexcept
				{
					switch (m_type.storage)
					{
						case tree_type::type_selector::ARRAY: ++m_frame_view.array_pos; break;
						case tree_type::type_selector::TABLE: ++m_frame_view.table_pos; break;
						default: break;
					}
				}

				frame_view_t m_frame_view = {};
				typename tree_type::type_selector m_type;
			};
			/** @brief Helper structure used as the API interface for Json output archive operations. */
			class write_frame
			{
				friend struct json_archive_base;

			public:
				typedef output_archive_category archive_category;
				typedef C char_type;
				typedef std::size_t size_type;

			private:
				constexpr write_frame(json_archive_base &parent, tree_node &node) noexcept
					: m_parent(parent), m_current(node)
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
				template<typename U>
				write_frame &operator<<(U &&value)
				{
					return write(std::forward<U>(value));
				}
				/** @copydoc operator<<
				 * @param args Arguments forwarded to the serialization function. */
				template<typename U, typename... Args>
				write_frame &write(U &&value, Args &&...args)
				{
					write_impl(std::forward<U>(value), std::forward<Args>(args)...);
					return *this;
				}

			private:
				[[nodiscard]] C *alloc_string(std::size_t n) const { return m_parent.tree->alloc_string(n); }
				template<typename U>
				[[nodiscard]] std::basic_string_view<C> copy_string(std::basic_string_view<C, U> str) const
				{
					return m_parent.tree->copy_string(str.data(), str.size());
				}

				void reserve_container(auto &node, std::size_t n) const
				{
					node = m_parent.tree->reserve_container(node, n);
				}
				[[nodiscard]] auto &push_container(auto &node) const
				{
					const auto old_size = node.size();
					if (old_size >= node.capacity()) [[unlikely]]
						reserve_container(node, node.empty() ? 1 : old_size * 2);

					node.insert(node.cbegin() + static_cast<std::ptrdiff_t>(old_size), {});
					return node.begin()[static_cast<std::ptrdiff_t>(old_size)];
				}
				[[nodiscard]] auto *next_node() const
				{
					tree_node *node;
					switch (m_current.type.storage)
					{
						case tree_type::type_selector::ARRAY:
						{
							node = &push_container(m_current.array()).value;
							break;
						}
						default:
						{
							m_current.to_table();
							[[fallthrough]];
						}
						case tree_type::type_selector::TABLE:
						{
							auto &member = push_container(m_current.table());
							member.key = m_next_key;
							node = &member.value;
							break;
						}
					}
					return node;
				}

				template<typename U, typename... Args>
				void write_value(tree_node &node, U &&value, Args &&...args) const
				{
					write_frame frame{m_parent, node};
					detail::do_serialize(std::forward<U>(value), frame, std::forward<Args>(args)...);
				}

				void write_value(tree_node &node, std::nullptr_t) const { node.to_value().type.value = NULL_VALUE; }
				// clang-format off
			template<typename U>
			void write_value(tree_node &node, U &&b) const requires(std::same_as<std::decay_t<U>, bool>)
			{
				node.to_value().type.value = static_cast<json_type>(BOOL | static_cast<int>(!!b));
			}
			template<typename U>
			void write_value(tree_node &node, U &&c) const requires(std::same_as<std::decay_t<U>, C> && ((Config & char_value) == char_value))
			{
				node.to_value().type.value = CHAR;
				node.value().character = c;
			}
				// clang-format on

				template<typename U>
				constexpr static bool is_uint_value =
					std::unsigned_integral<U> && !std::same_as<U, bool> && !std::same_as<U, C>;
				template<typename U>
				constexpr static bool is_int_value =
					std::signed_integral<U> && !std::same_as<U, bool> && !std::same_as<U, C>;

				template<std::unsigned_integral I>
				constexpr static int int_size_type(I i) noexcept
				{
					return int_size_category(i);
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

				// clang-format off
			template<typename I>
			void write_value(tree_node &node, I &&i) const requires is_uint_value<std::decay_t<I>>
			{
				node.to_value().type.value = static_cast<json_type>(INT_U | int_size_type(i));
				node.value().ui = static_cast<std::uintmax_t>(i);
			}
			template<typename I>
			void write_value(tree_node &node, I &&i) const requires is_int_value<std::decay_t<I>>
			{
				node.to_value().type.value = static_cast<json_type>(INT_S | int_size_type(i));
				node.value().si = static_cast<std::intmax_t>(i);
			}
			template<typename F>
			void write_value(tree_node &node, F &&f) const requires std::floating_point<std::decay_t<F>>
			{
				if constexpr (sizeof(F) > sizeof(float))
					node.to_value().type.value = FLOAT64;
				else
					node.to_value().type.value = FLOAT32;
				node.value().fp = static_cast<double>(f);
			}
				// clang-format on

				template<typename U>
				void write_value(tree_node &node, std::basic_string_view<C, U> sv) const
				{
					node.to_value().type.value = STRING;
					node.value().string = copy_string(sv);
				}
				void write_value(tree_node &node, const C *str) const
				{
					write_value(node, std::basic_string_view<C>{str});
				}
				template<typename U, typename A>
				void write_value(tree_node &node, std::basic_string<C, U, A> &&str) const
				{
					write_value(node, std::basic_string_view<C>{str});
				}
				template<typename U, typename A>
				void write_value(tree_node &node, std::basic_string<C, U, A> &str) const
				{
					write_value(node, std::basic_string_view<C>{str});
				}
				template<typename U, typename A>
				void write_value(tree_node &node, const std::basic_string<C, U, A> &str) const
				{
					write_value(node, std::basic_string_view<C>{str});
				}
				// clang-format off
				template<typename U>
				void write_value(tree_node &node, U &&str) const requires(std::constructible_from<std::basic_string_view<C>, U>)
				{
					write_value(node, std::basic_string_view<C>{std::forward<U>(str)});
				}
				template<typename U>
				void write_value(tree_node &node, U &&str) const requires(std::constructible_from<std::basic_string<C>, U>)
				{
					write_value(node, std::basic_string<C>{std::forward<U>(str)});
				}
				template<typename U>
				void write_value(tree_node &node, U &&str) const requires std::convertible_to<U, const C *>
				{
					write_value(node, static_cast<const C *>(str));
				}
				// clang-format on

				template<typename U, typename... Args>
				void write_value(U &&value, Args &&...args) const
				{
					auto *node = next_node();
					SEK_ASSERT(node != nullptr);

					write_value(*node, std::forward<U>(value), std::forward<Args>(args)...);

					if constexpr ((Config & container_types) == container_types)
					{
						typename tree_type::type_selector &type = m_current.is_array() ? m_current.array().element_type :
																						 m_current.table().element_type;

						/* Set container value type if all entries are of the same type.
						 * Use a different condition for integers to account for size-category. */
						if (type.storage == tree_type::type_selector::DYNAMIC && type.value == json_type::NO_TYPE) [[unlikely]]
							type = node->type;
						else if (type != node->type) [[likely]]
						{
							if constexpr (!std::is_integral_v<std::decay_t<T>>)
							{
								/* If the current type is also an integer of the same signedness, use the largest size category. */
								if ((type.value & INT_S) == (node->type.value & INT_S))
								{
									if (type.value < node->type.value) [[unlikely]]
										type.value = node->type.value;
									return;
								}
							}

							type.value = json_type::DYNAMIC;
							type.storage = tree_type::type_selector::DYNAMIC;
						}
					}
				}

				template<typename U, typename... Args>
				void write_impl(U &&value, Args &&...args)
				{
					if (!m_current.is_array()) [[likely]]
						m_next_key = m_parent.tree->make_key(m_current.table().size());
					write_value(std::forward<U>(value), std::forward<Args>(args)...);
				}
				template<typename U, typename... Args>
				void write_impl(keyed_entry_t<C, U> value, Args &&...args)
				{
					if (!m_current.is_array()) [[likely]]
						m_next_key = m_parent.tree->copy_string(value.key);
					write_value(std::forward<U>(value.value), std::forward<Args>(args)...);
				}
				template<typename U>
				void write_impl(container_size_t<U> size)
				{
					switch (m_current.type.storage)
					{
						case tree_type::type_selector::ARRAY:
						{
							reserve_container(m_current.array(), static_cast<std::size_t>(size.value));
							break;
						}
						default:
						{
							m_current.to_table();
							[[fallthrough]];
						}
						case tree_type::type_selector::TABLE:
						{
							reserve_container(m_current.table(), static_cast<std::size_t>(size.value));
							break;
						}
					}
				}
				void write_impl(array_mode_t)
				{
					SEK_ASSERT(!m_current.is_table(), "Array mode modifier applied to object entry");
					m_current.to_array();
				}

				json_archive_base &m_parent;
				tree_node &m_current;
				std::basic_string_view<C> m_next_key = {};
			};

			json_archive_base() = delete;
			json_archive_base(const json_archive_base &) = delete;
			json_archive_base &operator=(const json_archive_base &) = delete;

			explicit json_archive_base(std::pmr::memory_resource *res) noexcept
				: own_tree(new tree_type{res}), tree(own_tree), upstream(res)
			{
			}
			explicit json_archive_base(tree_type &&tree, std::pmr::memory_resource *res) noexcept
				: own_tree(new tree_type{std::move(tree)}), tree(own_tree), upstream(res)
			{
			}
			explicit json_archive_base(tree_type &tree, std::pmr::memory_resource *res) noexcept
				: tree(&tree), upstream(res)
			{
			}
			constexpr json_archive_base(json_archive_base &&other) noexcept
				: own_tree(std::exchange(other.own_tree, nullptr)),
				  tree(std::exchange(other.own_tree, nullptr)),
				  upstream(std::exchange(other.upstream, nullptr))
			{
			}
			constexpr json_archive_base &operator=(json_archive_base &&other) noexcept
			{
				swap(other);
				return *this;
			}
			~json_archive_base() { delete own_tree; }

			void reset(std::pmr::memory_resource *res) { tree->reset(res); }
			void reset() { tree->reset(); }

			template<typename U, typename... Args>
			bool do_try_read(U &&value, Args &&...args)
			{
				return top_entry().try_read(std::forward<U>(value), std::forward<Args>(args)...);
			}
			template<typename U, typename... Args>
			void do_read(U &&value, Args &&...args)
			{
				top_entry().read(std::forward<U>(value), std::forward<Args>(args)...);
			}
			template<typename U, typename... Args>
			U do_read(std::in_place_type_t<U>, Args &&...args)
			{
				return top_entry().read(std::in_place_type<U>, std::forward<Args>(args)...);
			}

			template<typename U, typename... Args>
			void do_write(U &&value, Args &&...args)
			{
				write_frame frame{*this, tree->top_level};
				detail::do_serialize(std::forward<U>(value), frame, std::forward<Args>(args)...);
			}
			void do_flush(auto &emitter) const
			{
				if (tree->top_level.type.storage != tree_type::type_selector::DYNAMIC) [[likely]]
					emit_node(tree->top_level, emitter);
			}

			constexpr void swap(json_archive_base &other) noexcept
			{
				std::swap(own_tree, other.own_tree);
				std::swap(tree, other.tree);
				std::swap(upstream, other.upstream);
			}

			[[nodiscard]] constexpr auto &top_entry() noexcept { return static_cast<entry_t &>(tree->top_level); }

			tree_type *own_tree = nullptr; /* Tree allocated by this archive. */
			tree_type *tree;

			std::pmr::memory_resource *upstream;
		};

		template<typename C, typename T, json_archive_config Cfg>
		template<typename U, typename... Args>
		constexpr const typename json_archive_base<C, T, Cfg>::entry_t &
			json_archive_base<C, T, Cfg>::entry_t::read(U &&v, Args &&...args) const
		{
			if (tree_node::is_value()) [[unlikely]]
				throw archive_error("Invalid Json type, expected array or object");

			read_frame frame{*this};
			detail::do_deserialize(std::forward<U>(v), frame, std::forward<Args>(args)...);
			return *this;
		}
		template<typename C, typename T, json_archive_config Cfg>
		template<typename U, typename... Args>
		constexpr U json_archive_base<C, T, Cfg>::entry_t::read(std::in_place_type_t<U>, Args &&...args) const
		{
			if (tree_node::is_value()) [[unlikely]]
				throw archive_error("Invalid Json type, expected array or object");

			if constexpr (in_place_deserializable<U, read_frame, Args...> ||
						  std::is_constructible_v<U, read_frame &, Args...> || in_place_deserializable<U, read_frame> ||
						  std::is_constructible_v<U, read_frame &>)
			{
				read_frame frame{*this};
				return detail::do_deserialize(std::in_place_type<U>, frame, std::forward<Args>(args)...);
			}
			else
			{
				U result{};
				read(result, std::forward<Args>(args)...);
				return result;
			}
		}
	}	 // namespace detail

	template<typename C, typename T = std::char_traits<C>>
	using basic_json_tree = detail::basic_json_tree<C, T>;
	using json_tree = detail::basic_json_tree<char>;
}	 // namespace sek::serialization