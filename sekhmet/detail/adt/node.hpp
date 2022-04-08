//
// Created by switchblade on 2022-02-12.
//

#pragma once

#include <string>
#include <vector>

#include "../../math/detail/util.hpp"
#include "../aligned_storage.hpp"
#include "../hmap.hpp"

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
		// POINTER = 3,
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
		template<typename T>
		struct node_setter;
	}	 // namespace detail

	/** @brief Helper structure used to store temporary node sequence for node initialization. */
	template<std::size_t N>
	struct sequence;
	/** @brief Helper structure used to store temporary node table for node initialization. */
	struct table;
	/** @brief Helper structure used to store temporary byte array for node initialization. */
	template<std::size_t N>
	struct bytes;

	/** @brief Exception thrown by nodes when (de)serialization fails. */
	class node_error : std::runtime_error
	{
	public:
		node_error() : std::runtime_error("Invalid ADT node type") {}
		explicit node_error(const char *msg) : std::runtime_error(msg) {}
		~node_error() noexcept override = default;
	};

	/** @brief Structure used to contain format-independent serialized data.
	 *
	 * A non-empty node contains one of the following:
	 * * `bool_type` - bool.
	 * * `char_type` - char character.
	 * * `std::uint8_type` - 8-bit unsigned integer.
	 * * `int8_type` - 8-bit signed integer.
	 * * `int16_type` - 16-bit signed integer.
	 * * `int32_type` - 32-bit signed integer.
	 * * `int64_type` - 64-bit signed integer.
	 * * `float32_type` - Single-precision float.
	 * * `float64_type` - Double-precision float.
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
		// typedef const node *pointer_type;

		typedef std::string string_type;
		typedef std::vector<std::byte> binary_type;
		typedef std::vector<node> sequence_type;
		typedef sek::hmap<string_type, node> table_type;

		typedef node_state_t state_type;

	private:
		constexpr static auto literal_value_size =
			math::max(sizeof(int64_type), sizeof(float64_type) /*, sizeof(pointer_type)*/);
		constexpr static auto literal_value_align =
			math::max(alignof(int64_type), alignof(float64_type) /*, alignof(pointer_type)*/);

		using literal_value_storage_t = sek::aligned_storage<literal_value_size, literal_value_align>;

	public:
		/** Initializes an empty node. */
		constexpr node() noexcept : node_state(state_type::EMPTY) {}
		/** Destroys the contained value. */
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
		template<typename T>
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

		/** Deserializes the contained value as the specified type.
		 * @param value Reference to an instance of T to be deserialized. */
		template<typename T>
		constexpr void get(T &value) const &
		{
			detail::node_getter<T>{}(*this, value);
		}
		/** @copydoc get
		 * @note This version uses rvalue overload of the `deserialize` function if available. */
		template<typename T>
		constexpr void get(T &value) &&
		{
			detail::node_getter<T>{}(std::forward<node>(*this), value);
		}
		/** Deserializes the contained value as the specified type.
		 * @param value Reference to an instance of T to be deserialized.
		 * @return true if deserialized successfully. false otherwise.
		 * @note If `node_type_exception` (serialization failure) is thrown, returns false.
		 * Any other exception will be passed through and should be handled by the user. */
		template<typename T>
		constexpr bool get(T &value, std::nothrow_t) const &
		{
			return detail::node_getter<T>{}(*this, value, std::nothrow);
		}
		/** @copydoc get
		 * @note This version uses rvalue overload of the `deserialize` function if available. */
		template<typename T>
		constexpr bool get(T &value, std::nothrow_t) &&
		{
			return detail::node_getter<T>{}(std::forward<node>(*this), value, std::nothrow);
		}
		/** Deserializes the contained value as the specified type.
		 * @return Deserialized instance of T. */
		template<typename T>
		[[nodiscard]] constexpr T get() const &
		{
			T value = {};
			get(value);
			return value;
		}
		/** @copydoc get */
		template<typename T>
		[[nodiscard]] constexpr operator T() const &
		{
			return get<T>();
		}
		/** @copydoc get
		 * @note This version uses rvalue overload of the `deserialize` function if available. */
		template<typename T>
		[[nodiscard]] constexpr T get() &&
		{
			T value = {};
			std::move(*this).get(value);
			return value;
		}
		/** @copydoc get */
		template<typename T>
		[[nodiscard]] constexpr operator T() &&
		{
			return std::move(*this).get<T>();
		}

		/** Serializes an instance of specified type into the node.
		 * @param value Reference to the value to be serialized.
		 * @return Reference to this node. */
		template<typename T>
		constexpr node &set(const T &value)
		{
			detail::node_setter<T>{}(*this, value);
			return *this;
		}
		/** @copydoc set
		 * @note This version uses rvalue overload of the `serialize` function if available. */
		template<typename T>
		constexpr node &set(T &&value)
		{
			detail::node_setter<T>{}(*this, std::move(value));
			return *this;
		}

		/** Checks if the node contains a bool. */
		[[nodiscard]] constexpr bool is_bool() const noexcept { return state() == state_type::BOOL; }
		/** Asserts that `is_bool` evaluates to true.
		 * @throw node_type_exception If the node does not contain a bool. */
		constexpr void require_bool() const { assert_state<state_type::BOOL>(); }
		/** Returns reference to the contained bool.
		 * @throw node_type_exception If the node does not contain a bool. */
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
		/** Asserts that `is_char` evaluates to true.
		 * @throw node_type_exception If the node does not contain a character. */
		constexpr void require_char() const { assert_state<state_type::CHAR>(); }
		/** Returns reference to the contained character.
		 * @throw node_type_exception If the node does not contain a character. */
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
		/** Asserts that `is_uint8` evaluates to true.
		 * @throw node_type_exception If the node does not contain an 8-bit unsigned integer. */
		constexpr void require_uint8() const { assert_state<state_type::UINT8>(); }
		/** Returns reference to the contained 8-bit unsigned integer.
		 * @throw node_type_exception If the node does not contain an 8-bit unsigned integer. */
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
		/** Asserts that `is_int8` evaluates to true.
		 * @throw node_type_exception If the node does not contain an 8-bit signed integer. */
		constexpr void require_int8() const { assert_state<state_type::INT8>(); }
		/** Returns reference to the contained 8-bit signed integer.
		 * @throw node_type_exception If the node does not contain an 8-bit signed integer. */
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
		/** Asserts that `is_int16` evaluates to true.
		 * @throw node_type_exception If the node does not contain a 16-bit signed integer. */
		constexpr void require_int16() const { assert_state<state_type::INT16>(); }
		/** Returns reference to the contained 16-bit signed integer.
		 * @throw node_type_exception If the node does not contain a 16-bit signed integer. */
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
		/** Asserts that `is_int32` evaluates to true.
		 * @throw node_type_exception If the node does not contain a 32-bit signed integer. */
		constexpr void require_int32() const { assert_state<state_type::INT32>(); }
		/** Returns reference to the contained 32-bit signed integer.
		 * @throw node_type_exception If the node does not contain a 32-bit signed integer. */
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
		/** Asserts that `is_int64` evaluates to true.
		 * @throw node_type_exception If the node does not contain a 64-bit signed integer. */
		constexpr void require_int64() const { assert_state<state_type::INT64>(); }
		/** Returns reference to the contained 64-bit signed integer.
		 * @throw node_type_exception If the node does not contain a 64-bit signed integer. */
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
		/** Asserts that `is_int` evaluates to true.
		 * @throw node_type_exception If the node does not contain an integer. */
		constexpr void require_int() const
		{
			if (!is_int()) [[unlikely]]
				throw node_error();
		}
		/** Returns value of the contained integer.
		 * @note Will preform conversions from the underlying int type if appropriate.
		 * @throw node_type_exception If the node does not contain an integer. */
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
				default: throw node_error();
			}
		}

		/** Checks if the node contains a single-precision float. */
		[[nodiscard]] constexpr bool is_float32() const noexcept { return state() == state_type::FLOAT32; }
		/** Asserts that `is_float32` evaluates to true.
		 * @throw node_type_exception If the node does not contain a single-precision float. */
		constexpr void require_float32() const { assert_state<state_type::FLOAT32>(); }
		/** Returns reference to the contained single-precision float.
		 * @throw node_type_exception If the node does not contain a single-precision float. */
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
		/** Asserts that `is_float64` evaluates to true.
		 * @throw node_type_exception If the node does not contain a double-precision float. */
		constexpr void require_float64() const { assert_state<state_type::FLOAT64>(); }
		/** Returns reference to the contained double-precision float.
		 * @throw node_type_exception If the node does not contain a double-precision float. */
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
		/** Asserts that `is_float` evaluates to true.
		 * @throw node_type_exception If the node does not contain a float. */
		constexpr void require_float() const
		{
			if (!is_float()) [[unlikely]]
				throw node_error();
		}
		/** Returns reference to the contained float.
		 * @note Will preform conversions from the underlying float type if appropriate.
		 * @throw node_type_exception If the node does not contain a float. */
		template<std::floating_point T>
		[[nodiscard]] constexpr T as_float() const
		{
			switch (state())
			{
				case state_type::FLOAT32: return static_cast<T>(float32_value);
				case state_type::FLOAT64: return static_cast<T>(float64_value);
				default: throw node_error();
			}
		}

		/** Checks if the node contains a number (int, or float of any width). */
		[[nodiscard]] constexpr bool is_number() const noexcept
		{
			return static_cast<std::uint32_t>(state()) & static_cast<std::uint32_t>(state_type::NUMBER);
		}
		/** Asserts that `is_number` evaluates to true.
		 * @throw node_type_exception If the node does not contain a number. */
		constexpr void require_number() const
		{
			if (!is_number()) [[unlikely]]
				throw node_error();
		}
		/** Returns copy of the contained number.
		 * @note Will preform conversions from the underlying number type if appropriate.
		 * @throw node_type_exception If the node does not contain a number. */
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
				default: throw node_error();
			}
		}

		/** Checks if the node contains a string. */
		[[nodiscard]] constexpr bool is_string() const noexcept { return state() == state_type::STRING; }
		/** Asserts that `is_string` evaluates to true.
		 * @throw node_type_exception If the node does not contain a string. */
		constexpr void require_string() const { assert_state<state_type::STRING>(); }
		/** Returns reference to the contained string.
		 * @throw node_type_exception If the node does not contain a string. */
		[[nodiscard]] constexpr string_type &as_string()
		{
			require_string();
			return string_value;
		}
		/** @copydoc as_binary */
		[[nodiscard]] constexpr const string_type &as_string() const
		{
			require_string();
			return string_value;
		}

		/** Checks if the node contains a byte sequence. */
		[[nodiscard]] constexpr bool is_binary() const noexcept { return state() == state_type::BINARY; }
		/** Asserts that `is_binary` evaluates to true.
		 * @throw node_type_exception If the node does not contain a byte sequence. */
		constexpr void require_binary() const { assert_state<state_type::BINARY>(); }
		/** Returns reference to the contained byte sequence.
		 * @throw node_type_exception If the node does not contain a byte sequence. */
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

		/** Checks if the node contains a sequence of nodes. */
		[[nodiscard]] constexpr bool is_sequence() const noexcept { return state() == state_type::ARRAY; }
		/** Asserts that `is_sequence` evaluates to true.
		 * @throw node_type_exception If the node does not contain a node sequence. */
		constexpr void require_sequence() const { assert_state<state_type::ARRAY>(); }
		/** Returns reference to the contained node sequence.
		 * @throw node_type_exception If the node does not contain a node sequence. */
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
		/** Asserts that `is_table` evaluates to true.
		 * @throw node_type_exception If the node does not contain a table. */
		constexpr void require_table() const { assert_state<state_type::TABLE>(); }
		/** Returns reference to the contained node table.
		 * @throw node_type_exception If the node does not contain a table. */
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

		/** Calls `operator[]` on the contained sequence. */
		[[nodiscard]] SEK_ADT_NODE_CONSTEXPR_VECTOR auto &operator[](sequence_type::size_type i)
		{
			return as_sequence()[i];
		}
		/** @copydoc operator[] */
		[[nodiscard]] SEK_ADT_NODE_CONSTEXPR_VECTOR const auto &operator[](sequence_type::size_type i) const
		{
			return as_sequence()[i];
		}
		/** Calls `at` on the contained sequence. */
		[[nodiscard]] SEK_ADT_NODE_CONSTEXPR_VECTOR auto &at(sequence_type::size_type i) { return as_sequence().at(i); }
		/** @copydoc at */
		[[nodiscard]] SEK_ADT_NODE_CONSTEXPR_VECTOR const auto &at(sequence_type::size_type i) const
		{
			return as_sequence().at(i);
		}

		/** Calls `operator[]` on the contained table. */
		[[nodiscard]] constexpr auto &operator[](const table_type::key_type &key) { return as_table()[key]; }
		/** Calls `at` on the contained table. */
		[[nodiscard]] constexpr auto &at(const table_type::key_type &key) { return as_table().at(key); }
		/** @copydoc at */
		[[nodiscard]] constexpr const auto &at(const table_type::key_type &key) const { return as_table().at(key); }

	private:
		template<state_type State>
		constexpr void assert_state() const
		{
			if (State != state()) [[unlikely]]
				throw node_error();
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
				case state_type::FLOAT64: std::construct_at(&literal_value_storage, other.literal_value_storage); break;
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
				case state_type::FLOAT64: std::construct_at(&literal_value_storage, other.literal_value_storage); break;
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
			// pointer_type pointer_value;

			string_type string_value;
			binary_type binary_value;
			sequence_type sequence_value;
			table_type table_value;
		};
	};

	namespace detail
	{
		template<typename T>
		concept has_adl_deserialize = requires(const node &node, T &value)
		{
			deserialize(node, value);
		};
		template<typename T>
		concept has_adl_deserialize_move = requires(node &&node, T &value)
		{
			deserialize_move(std::move(node), value);
		};
		template<typename T>
		concept has_member_deserialize = requires(const node &node, T &value)
		{
			value.deserialize(node);
		};
		template<typename T>
		concept has_member_deserialize_move = requires(node &node, T &&value)
		{
			value.serialize_move(std::move(node));
		};
		template<typename T>
		concept has_deserialize = has_adl_deserialize<T> || has_adl_deserialize_move<T> || has_member_deserialize<T> ||
			has_member_deserialize_move<T>;

		template<typename T>
		concept has_adl_serialize = requires(node &node, const T &value)
		{
			serialize(node, value);
		};
		template<typename T>
		concept has_adl_serialize_move = requires(node &node, T &&value)
		{
			serialize_move(node, std::move(value));
		};
		template<typename T>
		concept has_member_serialize = requires(node &node, const T &value)
		{
			value.serialize(node);
		};
		template<typename T>
		concept has_member_serialize_move = requires(node &node, T &&value)
		{
			std::move(value).serialize_move(node);
		};
		template<typename T>
		concept has_serialize = has_adl_serialize<T> || has_adl_serialize_move<T> || has_member_serialize<T> ||
			has_member_serialize_move<T>;

		template<has_deserialize T>
		struct node_getter<T>
		{
			constexpr void operator()(const node &n, T &value) const
				requires(has_adl_deserialize<T> || has_member_deserialize<T>)
			{
				if constexpr (has_adl_serialize<T>)
					deserialize(n, value);
				else
					value.deserialize(n);
			}
			constexpr bool operator()(const node &n, T &value, std::nothrow_t) const
			{
				try
				{
					operator()(n, value);
					return true;
				}
				catch (node_error &)
				{
					/* Only catch `node_type_exception` exceptions since they indicate deserialization failure. */
					/* TODO: Log exception message. */
					return false;
				}
			}

			constexpr void operator()(node &&n, T &value) const
				requires(has_adl_deserialize_move<T> || has_member_deserialize_move<T>)
			{
				if constexpr (has_adl_serialize_move<T>)
					deserialize(std::move(n), value);
				else
					value.deserialize(std::move(n));
			}
			constexpr bool operator()(node &&n, T &value, std::nothrow_t) const
			{
				try
				{
					operator()(std::move(n), value);
					return true;
				}
				catch (node_error &)
				{
					/* Only catch `node_type_exception` exceptions since they indicate deserialization failure. */
					/* TODO: Log exception message. */
					return false;
				}
			}
		};
		template<has_serialize T>
		struct node_setter<T>
		{
			SEK_ADT_NODE_CONSTEXPR void operator()(node &n, const T &value) const
				requires(has_adl_serialize<T> || has_member_serialize<T>)
			{
				n.destroy();
				if constexpr (has_adl_serialize<T>)
					serialize(n, value);
				else
					value.serialize(n);
			}

			SEK_ADT_NODE_CONSTEXPR void operator()(node &n, T &&value) const
				requires(has_adl_serialize_move<T> || has_member_serialize_move<T>)
			{
				n.destroy();
				if constexpr (has_adl_serialize_move<T>)
					serialize(n, std::move(value));
				else
					std::move(value).serialize(n);
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
		struct node_getter<typename node::string_type>
		{
			SEK_ADT_NODE_CONSTEXPR_STRING void operator()(const node &n, typename node::string_type &value) const
			{
				value = n.as_string();
			}
			SEK_ADT_NODE_CONSTEXPR_STRING void operator()(node &&n, typename node::string_type &value) const
			{
				value = std::move(n.as_string());
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
			SEK_ADT_NODE_CONSTEXPR_STRING bool operator()(node &&n, typename node::string_type &value, std::nothrow_t) const
			{
				if (n.is_string()) [[likely]]
				{
					value = std::move(n.string_value);
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
			SEK_ADT_NODE_CONSTEXPR_STRING void operator()(node &n, typename node::string_type &&value) const
			{
				if (n.is_string()) [[unlikely]]
					n.string_value = std::move(value);
				else
				{
					n.destroy();
					std::construct_at(&n.string_value, std::move(value));
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
			SEK_ADT_NODE_CONSTEXPR_VECTOR void operator()(node &&n, typename node::binary_type &value) const
			{
				value = std::move(n.as_binary());
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
			SEK_ADT_NODE_CONSTEXPR_VECTOR bool operator()(node &&n, typename node::binary_type &value, std::nothrow_t) const
			{
				if (n.is_binary()) [[likely]]
				{
					value = std::move(n.binary_value);
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
			SEK_ADT_NODE_CONSTEXPR_VECTOR void operator()(node &n, typename node::binary_type &&value) const
			{
				if (n.is_binary()) [[unlikely]]
					n.binary_value = std::move(value);
				else
				{
					n.destroy();
					std::construct_at(&n.binary_value, std::move(value));
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
			SEK_ADT_NODE_CONSTEXPR_VECTOR void operator()(node &&n, typename node::sequence_type &value) const
			{
				value = std::move(n.as_sequence());
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
			SEK_ADT_NODE_CONSTEXPR_VECTOR bool operator()(node &&n, typename node::sequence_type &value, std::nothrow_t) const
			{
				if (n.is_sequence()) [[likely]]
				{
					value = std::move(n.sequence_value);
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
			SEK_ADT_NODE_CONSTEXPR_VECTOR void operator()(node &n, typename node::sequence_type &&value) const
			{
				if (n.is_sequence()) [[unlikely]]
					n.sequence_value = std::move(value);
				else
				{
					n.destroy();
					std::construct_at(&n.sequence_value, std::move(value));
					n.node_state = node_state_t::ARRAY;
				}
			}
		};

		template<>
		struct node_getter<typename node::table_type>
		{
			constexpr void operator()(const node &n, typename node::table_type &value) const { value = n.as_table(); }
			constexpr void operator()(node &&n, typename node::table_type &value) const
			{
				value = std::move(n.as_table());
			}
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
			constexpr bool operator()(node &&n, typename node::table_type &value, std::nothrow_t) const
			{
				if (n.is_table()) [[likely]]
				{
					value = std::move(n.table_value);
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
			SEK_ADT_NODE_CONSTEXPR void operator()(node &n, typename node::table_type &&value) const
			{
				if (n.is_table()) [[unlikely]]
					n.table_value = std::move(value);
				else
				{
					n.destroy();
					std::construct_at(&n.table_value, std::move(value));
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
	template<>
	struct sequence<0>
	{
		template<typename... Ts>
		constexpr sequence(Ts &&...)
		{
		}
	};
	template<typename... Ts>
	sequence(Ts &&...) -> sequence<sizeof...(Ts)>;

	template<std::size_t N>
	SEK_ADT_NODE_CONSTEXPR_VECTOR node::node(sequence<N> sequence_init) noexcept
		: node(std::in_place_type<sequence_type>)
	{
		if constexpr (N != 0)
		{
			sequence_value.reserve(N);
			for (auto &node : sequence_init.data) sequence_value.push_back(std::move(node));
		}
	}

	struct table
	{
		constexpr table(std::initializer_list<typename node::table_type::value_type> data) : data(data) {}
		std::initializer_list<typename node::table_type::value_type> data;
	};
	constexpr node::node(table table_init) noexcept : node(std::in_place_type<table_type>, table_init.data) {}
}	 // namespace sek::adt