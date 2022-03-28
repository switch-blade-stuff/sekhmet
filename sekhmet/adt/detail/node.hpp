//
// Created by switchblade on 2022-02-12.
//

#pragma once

#include <string>
#include <vector>

#include "../../detail/aligned_storage.hpp"
#include "../../detail/engine_exception.hpp"
#include "../../detail/hmap.hpp"
#include "../../detail/type_info.hpp"
#include "../../math/detail/util.hpp"

#if defined(__cpp_lib_constexpr_string) && __cpp_lib_constexpr_string >= 201907L
#define SEK_ADT_NODE_CONSTEXPR_STRING constexpr
#else
#define SEK_ADT_NODE_CONSTEXPR_STRING
#endif

#if defined(__cpp_lib_constexpr_vector) && __cpp_lib_constexpr_vector >= 201907L
#define SEK_ADT_NODE_CONSTEXPR_VECTOR constexpr
#else
#define SEK_ADT_NODE_CONSTEXPR_VECTOR
#endif

#if defined(SEK_ADT_NODE_CONSTEXPR_STRING) && defined(SEK_ADT_NODE_CONSTEXPR_VECTOR)
#define SEK_ADT_NODE_CONSTEXPR
#endif

namespace sek::adt
{
	class node;

	enum class node_state_t : std::uint32_t
	{
		EMPTY = 0,
		BOOL = 1,
		CHAR = 2,
		POINTER = 3,
		STRING = 4,
		BINARY = 5,
		ARRAY = 6,
		TABLE = 7,
		UINT8 = 0b1000,
		INT8 = 0b1'0000,
		INT16 = 0b10'0000,
		INT32 = 0b100'0000,
		INT64 = 0b1000'0000,
		FLOAT32 = 0b1'0000'0000,
		FLOAT64 = 0b10'0000'0000,

		INT = UINT8 | INT8 | INT16 | INT32 | INT64,
		FLOAT = FLOAT32 | FLOAT64,
		NUMBER = INT | FLOAT,
	};

	namespace detail
	{
		template<typename T>
		struct node_getter;

		template<typename, typename = void>
		struct has_node_getter_impl : std::false_type
		{
		};
		template<typename T>
		struct has_node_getter_impl<T, std::void_t<decltype(sizeof(node_getter<T>))>> : std::true_type
		{
		};
		template<typename T>
		concept has_node_getter = has_node_getter_impl<T>::value;

		template<typename T>
		struct node_setter;

		template<typename, typename = void>
		struct has_node_setter_impl : std::false_type
		{
		};
		template<typename T>
		struct has_node_setter_impl<T, std::void_t<decltype(sizeof(node_setter<T>))>> : std::true_type
		{
		};
		template<typename T>
		concept has_node_setter = has_node_setter_impl<T>::value;
	}	 // namespace detail

	/** @brief Helper structure used to store temporary node sequence for node initialization. */
	template<std::size_t N>
	struct sequence;
	/** @brief Helper structure used to store temporary node table for node initialization. */
	struct table;
	/** @brief Helper structure used to store temporary byte array for node initialization. */
	template<std::size_t N>
	struct bytes;

	/** @brief Exception thrown by nodes when (de)serialization of a type fails.
	 *
	 * `node_type_exception` is thrown on node type mismatch - when stored type differs from requested.
	 * @example `as_table` was called on an empty node. */
	class node_type_exception : engine_exception
	{
		static std::string_view state_str(node_state_t state)
		{
			switch (state)
			{
				case node_state_t::EMPTY: return "empty";
				case node_state_t::BOOL: return "bool";
				case node_state_t::CHAR: return "char";
				case node_state_t::UINT8: return "std::uint8";
				case node_state_t::INT8: return "int8";
				case node_state_t::INT16: return "int16";
				case node_state_t::INT32: return "int32";
				case node_state_t::INT64: return "int64";
				case node_state_t::INT: return "int";
				case node_state_t::FLOAT32: return "float32";
				case node_state_t::FLOAT64: return "float64";
				case node_state_t::FLOAT: return "float";
				case node_state_t::NUMBER: return "number";
				case node_state_t::POINTER: return "pointer";
				case node_state_t::STRING: return "string";
				case node_state_t::BINARY: return "binary";
				case node_state_t::ARRAY: return "sequence";
				case node_state_t::TABLE: return "table";
				default: return {};
			}
		}
		static std::string get_msg(node_state_t expected, node_state_t actual)
		{
			std::string result;
			result.append("Mismatched adt node value type. Expected: \"");
			result.append(state_str(expected));
			result.append("\". Actual: \"");
			result.append(state_str(actual));
			result.append(1, '"');

			return result;
		}

	public:
		node_type_exception(node_state_t expected, node_state_t actual) : msg(get_msg(expected, actual)) {}
		~node_type_exception() noexcept override = default;

		[[nodiscard]] const char *what() const noexcept override { return msg.c_str(); }

	private:
		std::string msg;
	};

	/** @brief Structure used to store format-independent serialized data.
	 *
	 * A non-empty node stores one of the following:
	 * * `bool_type` - bool.
	 * * `char_type` - char character.
	 * * `std::uint8_type` - 8-bit unsigned integer.
	 * * `int8_type` - 8-bit signed integer.
	 * * `int16_type` - 16-bit signed integer.
	 * * `int32_type` - 32-bit signed integer.
	 * * `int64_type` - 64-bit signed integer.
	 * * `float32_type` - Single-precision float.
	 * * `float64_type` - Double-precision float.
	 * * `pointer_type` - Const pointer to a node.
	 * * `string_type` - Utf-8 string.
	 * * `binary_type` - Vector of `std::byte`.
	 * * `sequence_type` - Vector of nodes.
	 * * `table_type` - Map of nodes keyed by Utf-8 strings.
	 *
	 *
	 * Floating-point and integer states can be either treated as separate state or as a "number" state. */
	class node
	{
		template<typename>
		friend struct detail::node_getter;
		template<typename>
		friend struct detail::node_setter;

	public:
		typedef bool bool_type;
		typedef char char_type;
		typedef std::uint8_t uint8_type;
		typedef std::int8_t int8_type;
		typedef std::int16_t int16_type;
		typedef std::int32_t int32_type;
		typedef std::int64_t int64_type;
		typedef float float32_type;
		typedef double float64_type;
		typedef const node *pointer_type;

		typedef std::string string_type;
		typedef std::vector<std::byte> binary_type;
		typedef std::vector<node> sequence_type;
		typedef sek::hmap<string_type, node> table_type;

		typedef node_state_t state_type;

	private:
		constexpr static auto literal_value_size = max(sizeof(int64_type), sizeof(float64_type), sizeof(pointer_type));
		constexpr static auto literal_value_align = max(alignof(int64_type), alignof(float64_type), alignof(pointer_type));

		using literal_value_storage_t = sek::aligned_storage<literal_value_size, literal_value_align>;

	public:
		/** Initializes an empty node. */
		constexpr node() noexcept : node_state(state_type::EMPTY) {}
		/** Destroys the stored value. */
		SEK_ADT_NODE_CONSTEXPR ~node() { destroy(); }

		/** Copy-constructs the node. */
		SEK_ADT_NODE_CONSTEXPR node(const node &other) { copy_from(other); }
		/** Copy-assigns the node. */
		SEK_ADT_NODE_CONSTEXPR node &operator=(const node &other)
		{
			if (this != &other)
			{
				destroy();
				copy_from(other);
			}
			return *this;
		}
		/** Move-constructs the node. */
		SEK_ADT_NODE_CONSTEXPR node(node &&other) noexcept { move_from(std::forward<node>(other)); }
		/** Copy-assigns the node. */
		SEK_ADT_NODE_CONSTEXPR node &operator=(node &&other)
		{
			if (state() != other.state()) destroy();
			move_from(std::forward<node>(other));
			return *this;
		}

		/** Constructs a string node in-place from args.
		 * @param args Arguments passed to the string's constructor. */
		template<typename... Args>
		SEK_ADT_NODE_CONSTEXPR_STRING explicit node(std::in_place_type_t<string_type>,
													Args &&...args) noexcept requires std::constructible_from<string_type, Args...>
			: node_state(state_type::STRING), string_value(std::forward<Args>(args)...)
		{
		}
		/** Constructs a binary node in-place from args.
		 * @param args Arguments passed to the vector's constructor. */
		template<typename... Args>
		SEK_ADT_NODE_CONSTEXPR_VECTOR explicit node(std::in_place_type_t<binary_type>,
													Args &&...args) noexcept requires std::constructible_from<binary_type, Args...>
			: node_state(state_type::BINARY), binary_value(std::forward<Args>(args)...)
		{
		}
		/** Constructs an sequence node in-place from args.
		 * @param args Arguments passed to the vector's constructor. */
		template<typename... Args>
		SEK_ADT_NODE_CONSTEXPR_VECTOR explicit node(std::in_place_type_t<sequence_type>, Args &&...args) noexcept
			: node_state(state_type::ARRAY), sequence_value(std::forward<Args>(args)...)
		{
		}
		/** Constructs a table node in-place from args.
		 * @param args Arguments passed to the hmap's constructor. */
		template<typename... Args>
		constexpr explicit node(std::in_place_type_t<table_type>, Args &&...args) noexcept
			: node_state(state_type::TABLE), table_value(std::forward<Args>(args)...)
		{
		}

		/** Initializes the node an object by calling `set`.
		 * @param value Value to store in the node. */
		template<detail::has_node_setter T>
		constexpr node(const T &value) : node()
		{
			set(value);
		}

		/** Initializes a node from a bool.
		 * @param value Value to store in the node. */
		constexpr node(bool value) noexcept : node_state(state_type::BOOL), bool_value(value) {}

		/** Initializes a node from a char character.
		 * @param value Value to store in the node. */
		constexpr node(char_type value) noexcept : node_state(state_type::CHAR), char_value(value) {}

		/** Initializes a node from an 8-bit unsigned integer.
		 * @param value Value to store in the node. */
		constexpr node(uint8_type value) noexcept : node_state(state_type::UINT8), uint8_value(value) {}
		/** Initializes a node from an 8-bit signed integer.
		 * @param value Value to store in the node. */
		constexpr node(int8_type value) noexcept : node_state(state_type::INT8), int8_value(value) {}
		/** Initializes a node from a 16-bit signed integer.
		 * @param value Value to store in the node. */
		constexpr node(int16_type value) noexcept : node_state(state_type::INT16), int16_value(value) {}
		/** Initializes a node from a 32-bit signed integer.
		 * @param value Value to store in the node. */
		constexpr node(int32_type value) noexcept : node_state(state_type::INT32), int32_value(value) {}
		/** Initializes a node from a 64-bit signed integer.
		 * @param value Value to store in the node. */
		constexpr node(int64_type value) noexcept : node_state(state_type::INT64), int64_value(value) {}

		/** Initializes a node from a float.
		 * @param value Value to store in the node. */
		constexpr node(float32_type value) noexcept : node_state(state_type::FLOAT32), float32_value(value) {}
		/** Initializes a node from a double.
		 * @param value Value to store in the node. */
		constexpr node(float64_type value) noexcept : node_state(state_type::FLOAT64), float64_value(value) {}

		/** Initializes a node from a node pointer.
		 * @param value Pointer to a different node.
		 * @note Referenced node's lifetime must be managed by the user.
		 * Node does not manage the other node's lifetime. */
		constexpr node(pointer_type value) noexcept : node_state(state_type::POINTER), pointer_value(value) {}

		/** Initializes the node from a string by copy.
		 * @param str String to copy into the node. */
		SEK_ADT_NODE_CONSTEXPR_STRING node(const string_type &str) : node(std::in_place_type<string_type>, str) {}
		/** Initializes the node from a string by move.
		 * @param str String to move into the node. */
		SEK_ADT_NODE_CONSTEXPR_STRING node(string_type &&str) noexcept
			: node(std::in_place_type<string_type>, std::move(str))
		{
		}
		/** Initializes the node from a C-style string. */
		SEK_ADT_NODE_CONSTEXPR_STRING node(const typename string_type::value_type *str)
			: node(std::in_place_type<string_type>, str)
		{
		}
		/** Initializes the node from a character sequence.
		 * @param first Iterator to the start of the character sequence.
		 * @param last Iterator to the end of the character sequence. */
		template<sek::forward_iterator_for<typename string_type::value_type> I>
		SEK_ADT_NODE_CONSTEXPR_STRING node(I first, I last) : node(std::in_place_type<string_type>, first, last)
		{
		}
		/** Initializes the node from a character range.
		 * @param range Range containing string characters. */
		template<sek::forward_range_for<typename string_type::value_type> R>
		SEK_ADT_NODE_CONSTEXPR_STRING node(const R &range) : node(std::in_place_type<string_type>, range)
		{
		}

		/** Initializes the node from a byte vector by copy.
		 * @param bytes Vector of bytes to copy into the node. */
		SEK_ADT_NODE_CONSTEXPR_VECTOR node(const binary_type &bytes) : node(std::in_place_type<binary_type>, bytes) {}
		/** Initializes the node from a byte vector by move.
		 * @param bytes Vector of bytes to move into the node. */
		SEK_ADT_NODE_CONSTEXPR_VECTOR node(binary_type &&bytes) noexcept
			: node(std::in_place_type<binary_type>, std::move(bytes))
		{
		}
		/** Initializes the node from a temporary byte array.
		 * @param init_bytes Bytes to fill the byte sequence with. */
		template<std::size_t N>
		SEK_ADT_NODE_CONSTEXPR_VECTOR node(bytes<N> init_bytes) noexcept;
		/** Initializes the node from a byte sequence.
		 * @param first Iterator to the start of the byte sequence.
		 * @param last Iterator to the end of the byte sequence. */
		template<std::forward_iterator I>
		SEK_ADT_NODE_CONSTEXPR_VECTOR
			node(I first, I last) requires std::same_as<typename binary_type::value_type, std::iter_value_t<I>>
			: node(std::in_place_type<binary_type>, first, last)
		{
		}
		/** Initializes the node from a byte range.
		 * @param range Range containing bytes to store in the node. */
		template<std::ranges::forward_range R>
		SEK_ADT_NODE_CONSTEXPR_VECTOR
			node(const R &range) requires std::same_as<typename binary_type::value_type, std::ranges::range_value_t<R>>
			: node(std::in_place_type<binary_type>, range)
		{
		}

		/** Initializes the node from a node vector by copy.
		 * @param nodes Vector of nodes to copy into the node. */
		SEK_ADT_NODE_CONSTEXPR_VECTOR node(const sequence_type &nodes) : node(std::in_place_type<sequence_type>, nodes)
		{
		}
		/** Initializes the node from a node vector by move.
		 * @param bytes Vector of nodes to move into the node. */
		SEK_ADT_NODE_CONSTEXPR_VECTOR node(sequence_type &&nodes) noexcept
			: node(std::in_place_type<sequence_type>, std::move(nodes))
		{
		}

		/** Initializes the node from a temporary array of nodes.
		 * @param sequence_init Data to fill the sequence with. */
		template<std::size_t N>
		SEK_ADT_NODE_CONSTEXPR_VECTOR node(sequence<N> sequence_init) noexcept;
		/** Initializes the node from a node sequence.
		 * @param first Iterator to the start of the node sequence.
		 * @param last Iterator to the end of the node sequence. */
		template<std::forward_iterator I>
		SEK_ADT_NODE_CONSTEXPR_VECTOR
			node(I first, I last) requires std::same_as<typename sequence_type::value_type, std::iter_value_t<I>>
			: node(std::in_place_type<sequence_type>, first, last)
		{
		}
		/** Initializes the node from a node range.
		 * @param range Range containing nodes to store in the node. */
		template<std::ranges::forward_range R>
		SEK_ADT_NODE_CONSTEXPR_VECTOR
			node(const R &range) requires std::same_as<typename sequence_type::value_type, std::ranges::range_value_t<R>>
			: node(std::in_place_type<sequence_type>, range)
		{
		}

		/** Initializes the node from a node table by copy.
		 * @param table Table of nodes to copy into the node. */
		constexpr node(const table_type &table) : node(std::in_place_type<table_type>, table) {}
		/** Initializes the node from a node table by move.
		 * @param table Table of nodes to move into the node. */
		constexpr node(table_type &&table) noexcept : node(std::in_place_type<table_type>, std::move(table)) {}
		/** Initializes the node from a temporary table of nodes.
		 * @param table_init Data to fill the sequence with. */
		constexpr node(table table_init) noexcept;
		/** Initializes the node from a string-node pair sequence.
		 * @param first Iterator to the start of the sequence.
		 * @param last Iterator to the end of the sequence. */
		template<std::forward_iterator I>
		constexpr node(I first, I last) requires std::same_as<typename table_type::value_type, std::iter_value_t<I>>
			: node(std::in_place_type<table_type>, first, last)
		{
		}
		/** Initializes the node from a range of string-node pairs.
		 * @param range Range containing string-node pairs to store in the node. */
		template<std::ranges::forward_range R>
		constexpr node(const R &range) requires std::same_as<typename table_type::value_type, std::ranges::range_value_t<R>>
			: node(std::in_place_type<table_type>, range)
		{
		}

		/** Resets the node to an empty state. */
		SEK_ADT_NODE_CONSTEXPR void reset()
		{
			destroy();
			node_state = state_type::EMPTY;
		}

		/** Returns the state of the node. */
		[[nodiscard]] constexpr state_type state() const noexcept { return node_state; }
		/** Checks if the node is empty. */
		[[nodiscard]] constexpr bool empty() const noexcept { return state() == state_type::EMPTY; }

		/** Deserializes the stored value as the specified type.
		 * @param value Reference to an instance of T to be deserialized. */
		template<detail::has_node_getter T>
		constexpr void get(T &value) const
		{
			detail::node_getter<T>{}(*this, value);
		}
		/** Deserializes the stored value as the specified type.
		 * @param value Reference to an instance of T to be deserialized.
		 * @return true if deserialized successfully. false otherwise.
		 * @note If `node_type_exception` (serialization failure) is thrown, returns false.
		 * Any other exception will be passed through and should be handled by the user. */
		template<detail::has_node_getter T>
		constexpr bool get(T &value, std::nothrow_t) const
		{
			return detail::node_getter<T>{}(*this, value, std::nothrow);
		}
		/** Deserializes the stored value as the specified type.
		 * @return Deserialized instance of T. */
		template<detail::has_node_getter T>
		[[nodiscard]] constexpr T get() const requires std::is_default_constructible_v<T>
		{
			T value = {};
			get(value);
			return value;
		}
		/** Deserializes an instance of an object using it's `serializable_as_attribute` attribute.
		 * @param value `any_ref` referencing the object to be deserialized.
		 * @throw bad_type_exception If the referenced type does not have `serializable_as_attribute` attribute. */
		void get(sek::any_ref value) const;

		/** Serializes an instance of specified type into the node.
		 * @param value Reference to the value to be serialized.
		 * @return Reference to this node. */
		template<detail::has_node_setter T>
		constexpr node &set(const T &value)
		{
			detail::node_setter<T>{}(*this, value);
			return *this;
		}
		/** Serializes an instance of an object using it's `serializable_as_attribute` attribute.
		 * @param value `any_ref` referencing the object to be serialized.
		 * @return Reference to this node.
		 * @throw bad_type_exception If the referenced type does not have `serializable_as_attribute` attribute. */
		node &set(sek::any_ref value);

		/** Checks if the node contains a bool. */
		[[nodiscard]] constexpr bool is_bool() const noexcept { return state() == state_type::BOOL; }
		/** Returns reference to the stored bool.
		 * @throw node_type_exception If the node does not store a bool. */
		[[nodiscard]] constexpr bool_type &as_bool()
		{
			assert_state<state_type::BOOL>();
			return bool_value;
		}
		/** @copydoc as_bool */
		[[nodiscard]] constexpr const bool_type &as_bool() const
		{
			assert_state<state_type::BOOL>();
			return bool_value;
		}

		/** Checks if the node contains a character. */
		[[nodiscard]] constexpr bool is_char() const noexcept { return state() == state_type::CHAR; }
		/** Returns reference to the stored character.
		 * @throw node_type_exception If the node does not store a character. */
		[[nodiscard]] constexpr char_type &as_char()
		{
			assert_state<state_type::CHAR>();
			return char_value;
		}
		/** @copydoc as_char */
		[[nodiscard]] constexpr const char_type &as_char() const
		{
			assert_state<state_type::CHAR>();
			return char_value;
		}

		/** Checks if the node contains an 8-bit unsigned integer. */
		[[nodiscard]] constexpr bool is_uint8() const noexcept { return state() == state_type::UINT8; }
		/** Returns reference to the stored 8-bit unsigned integer.
		 * @throw node_type_exception If the node does not store an 8-bit unsigned integer. */
		[[nodiscard]] constexpr uint8_type &as_uint8()
		{
			assert_state<state_type::UINT8>();
			return uint8_value;
		}
		/** @copydoc as_std::uint8 */
		[[nodiscard]] constexpr const uint8_type &as_uint8() const
		{
			assert_state<state_type::UINT8>();
			return uint8_value;
		}
		/** Checks if the node contains an 8-bit signed integer. */
		[[nodiscard]] constexpr bool is_int8() const noexcept { return state() == state_type::INT8; }
		/** Returns reference to the stored 8-bit signed integer.
		 * @throw node_type_exception If the node does not store an 8-bit signed integer. */
		[[nodiscard]] constexpr int8_type &as_int8()
		{
			assert_state<state_type::INT8>();
			return int8_value;
		}
		/** @copydoc as_int8 */
		[[nodiscard]] constexpr const int8_type &as_int8() const
		{
			assert_state<state_type::INT8>();
			return int8_value;
		}
		/** Checks if the node contains an 16-bit signed integer. */
		[[nodiscard]] constexpr bool is_int16() const noexcept { return state() == state_type::INT16; }
		/** Returns reference to the stored 16-bit signed integer.
		 * @throw node_type_exception If the node does not store an 16-bit signed integer. */
		[[nodiscard]] constexpr int16_type &as_int16()
		{
			assert_state<state_type::INT16>();
			return int16_value;
		}
		/** @copydoc as_int16 */
		[[nodiscard]] constexpr const int16_type &as_int16() const
		{
			assert_state<state_type::INT16>();
			return int16_value;
		}
		/** Checks if the node contains an 32-bit signed integer. */
		[[nodiscard]] constexpr bool is_int32() const noexcept { return state() == state_type::INT32; }
		/** Returns reference to the stored 32-bit signed integer.
		 * @throw node_type_exception If the node does not store an 32-bit signed integer. */
		[[nodiscard]] constexpr int32_type &as_int32()
		{
			assert_state<state_type::INT32>();
			return int32_value;
		}
		/** @copydoc as_int32 */
		[[nodiscard]] constexpr const int32_type &as_int32() const
		{
			assert_state<state_type::INT32>();
			return int32_value;
		}
		/** Checks if the node contains an 64-bit signed integer. */
		[[nodiscard]] constexpr bool is_int64() const noexcept { return state() == state_type::INT64; }
		/** Returns reference to the stored 64-bit signed integer.
		 * @throw node_type_exception If the node does not store an 64-bit signed integer. */
		[[nodiscard]] constexpr int64_type &as_int64()
		{
			assert_state<state_type::INT64>();
			return int64_value;
		}
		/** @copydoc as_int64 */
		[[nodiscard]] constexpr const int64_type &as_int64() const
		{
			assert_state<state_type::INT64>();
			return int64_value;
		}
		/** Checks if the node contains an integer. */
		[[nodiscard]] constexpr bool is_int() const noexcept
		{
			return static_cast<std::uint32_t>(state()) & static_cast<std::uint32_t>(state_type::INT);
		}
		/** Returns value of the stored integer.
		 * @note Will preform conversions from the underlying int type if appropriate.
		 * @throw node_type_exception If the node does not store an integer. */
		template<std::integral T>
		[[nodiscard]] constexpr T as_int() const
		{
			switch (state())
			{
				case state_type::UINT8: return static_cast<T>(uint8_value);
				case state_type::INT8: return static_cast<T>(int8_value);
				case state_type::INT16: return static_cast<T>(int16_value);
				case state_type::INT32: return static_cast<T>(int32_value);
				case state_type::INT64: return static_cast<T>(int64_value);
				default: throw node_type_exception(state_type::INT, node_state);
			}
		}

		/** Checks if the node contains a single-precision float. */
		[[nodiscard]] constexpr bool is_float32() const noexcept { return state() == state_type::FLOAT32; }
		/** Returns reference to the stored single-precision float.
		 * @throw node_type_exception If the node does not store a single-precision float. */
		[[nodiscard]] constexpr float32_type &as_float32()
		{
			assert_state<state_type::FLOAT32>();
			return float32_value;
		}
		/** @copydoc as_float32 */
		[[nodiscard]] constexpr const float32_type &as_float32() const
		{
			assert_state<state_type::FLOAT32>();
			return float32_value;
		}
		/** Checks if the node contains a double-precision float. */
		[[nodiscard]] constexpr bool is_float64() const noexcept { return state() == state_type::FLOAT64; }
		/** Returns reference to the stored double-precision float.
		 * @throw node_type_exception If the node does not store a double-precision float. */
		[[nodiscard]] constexpr float64_type &as_float64()
		{
			assert_state<state_type::FLOAT64>();
			return float64_value;
		}
		/** @copydoc as_float64 */
		[[nodiscard]] constexpr const float64_type &as_float64() const
		{
			assert_state<state_type::FLOAT64>();
			return float64_value;
		}
		/** Checks if the node contains a float. */
		[[nodiscard]] constexpr bool is_float() const noexcept
		{
			return static_cast<std::uint32_t>(state()) & static_cast<std::uint32_t>(state_type::FLOAT);
		}
		/** Returns reference to the stored float.
		 * @note Will preform conversions from the underlying float type if appropriate.
		 * @throw node_type_exception If the node does not store a float. */
		template<std::floating_point T>
		[[nodiscard]] constexpr T as_float() const
		{
			switch (state())
			{
				case state_type::FLOAT32: return static_cast<T>(float32_value);
				case state_type::FLOAT64: return static_cast<T>(float64_value);
				default: throw node_type_exception(state_type::FLOAT, node_state);
			}
		}

		/** Checks if the node contains a number (int, or float of any width). */
		[[nodiscard]] constexpr bool is_number() const noexcept
		{
			return static_cast<std::uint32_t>(state()) & static_cast<std::uint32_t>(state_type::NUMBER);
		}
		/** Returns copy of the stored number.
		 * @note Will preform conversions from the underlying number type if appropriate.
		 * @throw node_type_exception If the node does not store a number. */
		template<typename T>
		[[nodiscard]] constexpr T as_number() const requires std::is_arithmetic_v<T>
		{
			switch (state())
			{
				case state_type::UINT8: return static_cast<T>(uint8_value);
				case state_type::INT8: return static_cast<T>(int8_value);
				case state_type::INT16: return static_cast<T>(int16_value);
				case state_type::INT32: return static_cast<T>(int32_value);
				case state_type::INT64: return static_cast<T>(int64_value);
				case state_type::FLOAT32: return static_cast<T>(float32_value);
				case state_type::FLOAT64: return static_cast<T>(float64_value);
				default: throw node_type_exception(state_type::NUMBER, node_state);
			}
		}

		/** Checks if the node contains a node pointer. */
		[[nodiscard]] constexpr bool is_pointer() const noexcept { return state() == state_type::POINTER; }
		/** Returns reference to the stored node pointer.
		 * @throw node_type_exception If the node does not hold a pointer. */
		[[nodiscard]] constexpr pointer_type &as_pointer()
		{
			assert_state<state_type::POINTER>();
			return pointer_value;
		}
		/** Returns reference to the stored node pointer.
		 * @throw node_type_exception If the node does not hold a pointer. */
		[[nodiscard]] constexpr const pointer_type &as_pointer() const
		{
			assert_state<state_type::POINTER>();
			return pointer_value;
		}

		/** Checks if the node contains a string. */
		[[nodiscard]] constexpr bool is_string() const noexcept { return state() == state_type::STRING; }
		/** Returns reference to the stored string.
		 * @throw node_type_exception If the node does not store a string. */
		[[nodiscard]] constexpr string_type &as_string()
		{
			assert_state<state_type::STRING>();
			return string_value;
		}
		/** @copydoc as_binary */
		[[nodiscard]] constexpr const string_type &as_string() const
		{
			assert_state<state_type::STRING>();
			return string_value;
		}

		/** Checks if the node contains a byte sequence. */
		[[nodiscard]] constexpr bool is_binary() const noexcept { return state() == state_type::BINARY; }
		/** Returns reference to the stored byte sequence.
		 * @throw node_type_exception If the node does not store a byte sequence. */
		[[nodiscard]] constexpr binary_type &as_binary()
		{
			assert_state<state_type::BINARY>();
			return binary_value;
		}
		/** @copydoc as_binary */
		[[nodiscard]] constexpr const binary_type &as_binary() const
		{
			assert_state<state_type::BINARY>();
			return binary_value;
		}

		/** Checks if the node contains an sequence of nodes. */
		[[nodiscard]] constexpr bool is_sequence() const noexcept { return state() == state_type::ARRAY; }
		/** Returns reference to the stored node sequence.
		 * @throw node_type_exception If the node does not store a node sequence. */
		[[nodiscard]] constexpr sequence_type &as_sequence()
		{
			assert_state<state_type::ARRAY>();
			return sequence_value;
		}
		/** @copydoc as_sequence */
		[[nodiscard]] constexpr const sequence_type &as_sequence() const
		{
			assert_state<state_type::ARRAY>();
			return sequence_value;
		}

		/** Checks if the node contains a table of nodes. */
		[[nodiscard]] constexpr bool is_table() const noexcept { return state() == state_type::TABLE; }
		/** Returns reference to the stored node table.
		 * @throw node_type_exception If the node does not store a table. */
		[[nodiscard]] constexpr table_type &as_table()
		{
			assert_state<state_type::TABLE>();
			return table_value;
		}
		/** @copydoc as_table */
		[[nodiscard]] constexpr const table_type &as_table() const
		{
			assert_state<state_type::TABLE>();
			return table_value;
		}

	private:
		template<state_type State>
		constexpr void assert_state() const
		{
			if (State != state()) [[unlikely]]
				throw node_type_exception(State, node_state);
		}

		SEK_ADT_NODE_CONSTEXPR void copy_from(const node &other)
		{
			switch (node_state = other.state())
			{
				case state_type::BOOL:
				case state_type::CHAR:
				case state_type::UINT8:
				case state_type::INT8:
				case state_type::INT16:
				case state_type::INT32:
				case state_type::INT64:
				case state_type::FLOAT32:
				case state_type::FLOAT64:
				case state_type::POINTER: std::construct_at(&literal_value_storage, other.literal_value_storage); break;
				case state_type::STRING: std::construct_at(&string_value, other.string_value); break;
				case state_type::BINARY: std::construct_at(&binary_value, other.binary_value); break;
				case state_type::ARRAY: std::construct_at(&sequence_value, other.sequence_value); break;
				case state_type::TABLE: std::construct_at(&table_value, other.table_value); break;
				default: break;
			}
		}
		SEK_ADT_NODE_CONSTEXPR void move_from(node &&other) noexcept
		{
			switch (node_state = other.state())
			{
				case state_type::BOOL:
				case state_type::CHAR:
				case state_type::UINT8:
				case state_type::INT8:
				case state_type::INT16:
				case state_type::INT32:
				case state_type::INT64:
				case state_type::FLOAT32:
				case state_type::FLOAT64:
				case state_type::POINTER: std::construct_at(&literal_value_storage, other.literal_value_storage); break;
				case state_type::STRING: std::construct_at(&string_value, other.string_value); break;
				case state_type::BINARY: std::construct_at(&binary_value, other.binary_value); break;
				case state_type::ARRAY: std::construct_at(&sequence_value, other.sequence_value); break;
				case state_type::TABLE: std::construct_at(&table_value, other.table_value); break;
				default: break;
			}
		}
		SEK_ADT_NODE_CONSTEXPR void destroy()
		{
			switch (state())
			{
				case state_type::STRING: std::destroy_at(&string_value); break;
				case state_type::BINARY: std::destroy_at(&binary_value); break;
				case state_type::ARRAY: std::destroy_at(&sequence_value); break;
				case state_type::TABLE: std::destroy_at(&table_value); break;
				default: break;
			}
		}

		/** State of the node. */
		state_type node_state = state_type::EMPTY;

		union
		{
			literal_value_storage_t literal_value_storage;
			bool_type bool_value;
			char_type char_value;
			uint8_type uint8_value;
			int8_type int8_value;
			int16_type int16_value;
			int32_type int32_value;
			int64_type int64_value;
			float32_type float32_value;
			float64_type float64_value;
			pointer_type pointer_value;

			string_type string_value;
			binary_type binary_value;
			sequence_type sequence_value;
			table_type table_value;
		};
	};

	namespace detail
	{
		template<typename T>
		concept has_adl_serialize = requires(node &node, const T &value)
		{
			serialize(node, value);
		};
		template<typename T>
		concept has_adl_deserialize = requires(const node &node, T &value)
		{
			deserialize(node, value);
		};

		template<has_adl_deserialize T>
		struct node_getter<T>
		{
			constexpr void operator()(const node &n, T &value) const { deserialize(n, value); }
			constexpr bool operator()(const node &n, T &value, std::nothrow_t) const
			{
				try
				{
					operator()(n, value);
					return true;
				}
				catch (node_type_exception &)
				{
					/* Only catch `node_type_exception` exceptions since they indicate deserialization failure. */
					/* TODO: Log exception message. */
					return false;
				}
			}
		};
		template<has_adl_serialize T>
		struct node_setter<T>
		{
			SEK_ADT_NODE_CONSTEXPR void operator()(node &n, const T &value) const
			{
				n.destroy();
				serialize(n, value);
			}
		};

		template<>
		struct node_getter<typename node::bool_type>
		{
			constexpr void operator()(const node &n, typename node::bool_type &value) const { value = n.as_bool(); }
			constexpr bool operator()(const node &n, typename node::bool_type &value, std::nothrow_t) const noexcept
			{
				if (n.is_bool()) [[likely]]
				{
					value = n.bool_value;
					return true;
				}
				else
					return false;
			}
		};
		template<>
		struct node_setter<typename node::bool_type>
		{
			SEK_ADT_NODE_CONSTEXPR void operator()(node &n, typename node::bool_type value) const noexcept
			{
				n.destroy();
				n.node_state = node_state_t::BOOL;
				n.bool_value = value;
			}
		};

		template<>
		struct node_getter<typename node::char_type>
		{
			constexpr void operator()(const node &n, typename node::char_type &value) const { value = n.as_char(); }
			constexpr bool operator()(const node &n, typename node::char_type &value, std::nothrow_t) const noexcept
			{
				if (n.is_char()) [[likely]]
				{
					value = n.char_value;
					return true;
				}
				else
					return false;
			}
		};
		template<>
		struct node_setter<typename node::char_type>
		{
			SEK_ADT_NODE_CONSTEXPR void operator()(node &n, typename node::char_type value) const noexcept
			{
				n.destroy();
				n.node_state = node_state_t::CHAR;
				n.char_value = value;
			}
		};

		template<>
		struct node_getter<typename node::uint8_type>
		{
			constexpr void operator()(const node &n, typename node::uint8_type &value) const { value = n.as_uint8(); }
			constexpr bool operator()(const node &n, typename node::uint8_type &value, std::nothrow_t) const noexcept
			{
				if (n.is_uint8()) [[likely]]
				{
					value = n.uint8_value;
					return true;
				}
				else
					return false;
			}
		};
		template<>
		struct node_setter<typename node::uint8_type>
		{
			SEK_ADT_NODE_CONSTEXPR void operator()(node &n, typename node::uint8_type value) const noexcept
			{
				n.destroy();
				n.node_state = node_state_t::UINT8;
				n.uint8_value = value;
			}
		};
		template<>
		struct node_getter<typename node::int8_type>
		{
			constexpr void operator()(const node &n, typename node::int8_type &value) const { value = n.as_int8(); }
			constexpr bool operator()(const node &n, typename node::int8_type &value, std::nothrow_t) const noexcept
			{
				if (n.is_int8()) [[likely]]
				{
					value = n.int8_value;
					return true;
				}
				else
					return false;
			}
		};
		template<>
		struct node_setter<typename node::int8_type>
		{
			SEK_ADT_NODE_CONSTEXPR void operator()(node &n, typename node::int8_type value) const noexcept
			{
				n.destroy();
				n.node_state = node_state_t::INT8;
				n.int8_value = value;
			}
		};
		template<>
		struct node_getter<typename node::int16_type>
		{
			constexpr void operator()(const node &n, typename node::int16_type &value) const { value = n.as_int16(); }
			constexpr bool operator()(const node &n, typename node::int16_type &value, std::nothrow_t) const noexcept
			{
				if (n.is_int16()) [[likely]]
				{
					value = n.int16_value;
					return true;
				}
				else
					return false;
			}
		};
		template<>
		struct node_setter<typename node::int16_type>
		{
			SEK_ADT_NODE_CONSTEXPR void operator()(node &n, typename node::int16_type value) const noexcept
			{
				n.destroy();
				n.node_state = node_state_t::INT16;
				n.int16_value = value;
			}
		};
		template<>
		struct node_getter<typename node::int32_type>
		{
			constexpr void operator()(const node &n, typename node::int32_type &value) const { value = n.as_int32(); }
			constexpr bool operator()(const node &n, typename node::int32_type &value, std::nothrow_t) const noexcept
			{
				if (n.is_int32()) [[likely]]
				{
					value = n.int32_value;
					return true;
				}
				else
					return false;
			}
		};
		template<>
		struct node_setter<typename node::int32_type>
		{
			SEK_ADT_NODE_CONSTEXPR void operator()(node &n, typename node::int32_type value) const noexcept
			{
				n.destroy();
				n.node_state = node_state_t::INT32;
				n.int32_value = value;
			}
		};
		template<>
		struct node_getter<typename node::int64_type>
		{
			constexpr void operator()(const node &n, typename node::int64_type &value) const { value = n.as_int64(); }
			constexpr bool operator()(const node &n, typename node::int64_type &value, std::nothrow_t) const noexcept
			{
				if (n.is_int64()) [[likely]]
				{
					value = n.int64_value;
					return true;
				}
				else
					return false;
			}
		};
		template<>
		struct node_setter<typename node::int64_type>
		{
			SEK_ADT_NODE_CONSTEXPR void operator()(node &n, typename node::int64_type value) const noexcept
			{
				n.destroy();
				n.node_state = node_state_t::INT64;
				n.int64_value = value;
			}
		};
		template<std::integral T>
		struct node_getter<T>
		{
			constexpr void operator()(const node &n, T &value) const { value = n.as_int<T>(); }
			constexpr bool operator()(const node &n, T &value, std::nothrow_t) const noexcept
			{
				if (n.is_int()) [[likely]]
				{
					value = n.as_int<T>();
					return true;
				}
				else
					return false;
			}
		};
		template<std::integral T>
		struct node_setter<T>
		{
			SEK_ADT_NODE_CONSTEXPR void operator()(node &n, T value) const noexcept
			{
				n.destroy();
				if constexpr (sizeof(T) > sizeof(typename node::int32_type))
				{
					n.node_state = node_state_t::INT64;
					n.int64_value = static_cast<typename node::int64_type>(value);
				}
				else if (sizeof(T) > sizeof(typename node::int16_type))
				{
					n.node_state = node_state_t::INT32;
					n.int32_value = static_cast<typename node::int32_type>(value);
				}
				else if (sizeof(T) > sizeof(typename node::int8_type))
				{
					n.node_state = node_state_t::INT16;
					n.int16_value = static_cast<typename node::int16_type>(value);
				}
				else if (std::is_signed_v<T>)
				{
					n.node_state = node_state_t::INT8;
					n.int8_value = static_cast<typename node::int8_type>(value);
				}
				else
				{
					n.node_state = node_state_t::UINT8;
					n.uint8_value = static_cast<typename node::uint8_type>(value);
				}
			}
		};

		template<>
		struct node_getter<float>
		{
			constexpr void operator()(const node &n, float &value) const { value = n.as_float32(); }
			constexpr bool operator()(const node &n, float &value, std::nothrow_t) const noexcept
			{
				if (n.is_float32()) [[likely]]
				{
					value = n.float32_value;
					return true;
				}
				else
					return false;
			}
		};
		template<>
		struct node_setter<float>
		{
			SEK_ADT_NODE_CONSTEXPR void operator()(node &n, float value) const noexcept
			{
				n.destroy();
				n.node_state = node_state_t::FLOAT32;
				n.float32_value = value;
			}
		};
		template<>
		struct node_getter<double>
		{
			constexpr void operator()(const node &n, double &value) const { value = n.as_float64(); }
			constexpr bool operator()(const node &n, double &value, std::nothrow_t) const noexcept
			{
				if (n.is_float64()) [[likely]]
				{
					value = n.float64_value;
					return true;
				}
				else
					return false;
			}
		};
		template<>
		struct node_setter<double>
		{
			SEK_ADT_NODE_CONSTEXPR void operator()(node &n, double value) const noexcept
			{
				n.destroy();
				n.node_state = node_state_t::FLOAT64;
				n.float64_value = value;
			}
		};
		template<std::floating_point T>
		struct node_getter<T>
		{
			constexpr void operator()(const node &n, T &value) const { value = n.as_float<T>(); }
			constexpr bool operator()(const node &n, T &value, std::nothrow_t) const noexcept
			{
				if (n.is_float()) [[likely]]
				{
					value = n.as_float<T>();
					return true;
				}
				else
					return false;
			}
		};
		template<std::floating_point T>
		struct node_setter<T>
		{
			SEK_ADT_NODE_CONSTEXPR void operator()(node &n, T value) const noexcept
			{
				n.destroy();
				if constexpr (sizeof(T) > sizeof(typename node::float32_type))
				{
					n.node_state = node_state_t::FLOAT64;
					n.float64_value = static_cast<typename node::float64_type>(value);
				}
				else
				{
					n.node_state = node_state_t::FLOAT32;
					n.float32_value = static_cast<typename node::float32_type>(value);
				}
			}
		};

		template<>
		struct node_getter<const node *>
		{
			constexpr void operator()(const node &n, typename node::pointer_type &value) const
			{
				value = n.as_pointer();
			}
			constexpr bool operator()(const node &n, typename node::pointer_type &value, std::nothrow_t) const noexcept
			{
				if (n.is_bool()) [[likely]]
				{
					value = n.pointer_value;
					return true;
				}
				else
					return false;
			}
		};
		template<typename T>
		requires(std::same_as<T, node *> || std::same_as<T, const node *>) struct node_setter<T>
		{
			SEK_ADT_NODE_CONSTEXPR void operator()(node &n, typename node::pointer_type value) const noexcept
			{
				n.destroy();
				n.node_state = node_state_t::POINTER;
				n.pointer_value = value;
			}
		};

		template<>
		struct node_getter<typename node::string_type>
		{
			SEK_ADT_NODE_CONSTEXPR_STRING void operator()(const node &n, typename node::string_type &value) const
			{
				value = n.as_string();
			}
			SEK_ADT_NODE_CONSTEXPR_STRING bool operator()(const node &n, typename node::string_type &value, std::nothrow_t) const
			{
				if (n.is_string()) [[likely]]
				{
					value = n.string_value;
					return true;
				}
				else
					return false;
			}
		};
		template<>
		struct node_setter<typename node::string_type>
		{
			SEK_ADT_NODE_CONSTEXPR_STRING void operator()(node &n, const typename node::string_type &value) const
			{
				if (n.is_string()) [[unlikely]]
					n.string_value = value;
				else
				{
					n.destroy();
					std::construct_at(&n.string_value, value);
					n.node_state = node_state_t::STRING;
				}
			}
		};

		template<>
		struct node_getter<typename node::binary_type>
		{
			SEK_ADT_NODE_CONSTEXPR_VECTOR void operator()(const node &n, typename node::binary_type &value) const
			{
				value = n.as_binary();
			}
			SEK_ADT_NODE_CONSTEXPR_VECTOR bool operator()(const node &n, typename node::binary_type &value, std::nothrow_t) const
			{
				if (n.is_binary()) [[likely]]
				{
					value = n.binary_value;
					return true;
				}
				else
					return false;
			}
		};
		template<>
		struct node_setter<typename node::binary_type>
		{
			SEK_ADT_NODE_CONSTEXPR_VECTOR void operator()(node &n, const typename node::binary_type &value) const
			{
				if (n.is_binary()) [[unlikely]]
					n.binary_value = value;
				else
				{
					n.destroy();
					std::construct_at(&n.binary_value, value);
					n.node_state = node_state_t::BINARY;
				}
			}
		};

		template<>
		struct node_getter<typename node::sequence_type>
		{
			SEK_ADT_NODE_CONSTEXPR_VECTOR void operator()(const node &n, typename node::sequence_type &value) const
			{
				value = n.as_sequence();
			}
			SEK_ADT_NODE_CONSTEXPR_VECTOR bool operator()(const node &n, typename node::sequence_type &value, std::nothrow_t) const
			{
				if (n.is_sequence()) [[likely]]
				{
					value = n.sequence_value;
					return true;
				}
				else
					return false;
			}
		};
		template<>
		struct node_setter<typename node::sequence_type>
		{
			SEK_ADT_NODE_CONSTEXPR_VECTOR void operator()(node &n, const typename node::sequence_type &value) const
			{
				if (n.is_sequence()) [[unlikely]]
					n.sequence_value = value;
				else
				{
					n.destroy();
					std::construct_at(&n.sequence_value, value);
					n.node_state = node_state_t::ARRAY;
				}
			}
		};

		template<>
		struct node_getter<typename node::table_type>
		{
			constexpr void operator()(const node &n, typename node::table_type &value) const { value = n.as_table(); }
			constexpr bool operator()(const node &n, typename node::table_type &value, std::nothrow_t) const
			{
				if (n.is_table()) [[likely]]
				{
					value = n.table_value;
					return true;
				}
				else
					return false;
			}
		};
		template<>
		struct node_setter<typename node::table_type>
		{
			SEK_ADT_NODE_CONSTEXPR void operator()(node &n, const typename node::table_type &value) const
			{
				if (n.is_table()) [[unlikely]]
					n.table_value = value;
				else
				{
					n.destroy();
					std::construct_at(&n.table_value, value);
					n.node_state = node_state_t::TABLE;
				}
			}
		};
	}	 // namespace detail

	template<std::size_t N>
	struct bytes
	{
		template<typename... Ts>
		constexpr bytes(Ts &&...vals) : data{std::byte(vals)...}
		{
		}

		std::byte data[N];
	};
	template<typename... Ts>
	bytes(Ts &&...) -> bytes<sizeof...(Ts)>;

	template<std::size_t N>
	SEK_ADT_NODE_CONSTEXPR_VECTOR node::node(bytes<N> init_bytes) noexcept : node(std::in_place_type<binary_type>)
	{
		binary_value.reserve(N);
		for (auto byte : init_bytes.data) binary_value.push_back(byte);
	}

	template<std::size_t N>
	struct sequence
	{
		template<typename... Ts>
		constexpr sequence(Ts &&...vals) : data{vals...}
		{
		}

		node data[N];
	};
	template<typename... Ts>
	sequence(Ts &&...) -> sequence<sizeof...(Ts)>;

	template<std::size_t N>
	SEK_ADT_NODE_CONSTEXPR_VECTOR node::node(sequence<N> sequence_init) noexcept
		: node(std::in_place_type<sequence_type>)
	{
		sequence_value.reserve(N);
		for (auto &node : sequence_init.data) sequence_value.push_back(std::move(node));
	}

	struct table
	{
		constexpr table(std::initializer_list<typename node::table_type::value_type> data) : data(data) {}
		std::initializer_list<typename node::table_type::value_type> data;
	};
	constexpr node::node(table table_init) noexcept : node(std::in_place_type<table_type>, table_init.data) {}
}	 // namespace sek::adt