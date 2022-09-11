/*
 * Created by switchblade on 07/07/22
 */

#pragma once

#include <vector>

#include "sekhmet/ordered_map.hpp"

#include "../archive_error.hpp"
#include "../manipulators.hpp"
#include "../types/tuples.hpp"
#include "../util.hpp"
#include "type.hpp"

namespace sek::serialization
{
	/** @brief Exception thrown by archives on runtime errors. */
	class SEK_API json_error : public archive_error
	{
	public:
		json_error() : archive_error("Unknown archive error") {}
		explicit json_error(std::string &&msg) : archive_error(std::move(msg)) {}
		explicit json_error(const std::string &msg) : archive_error(msg) {}
		explicit json_error(const char *msg) : archive_error(msg) {}
		~json_error() override;
	};

	namespace detail
	{
		[[noreturn]] SEK_API void throw_invalid_type(json_type expected, json_type actual);
		[[noreturn]] SEK_API void throw_invalid_type(json_type expected);

		[[nodiscard]] constexpr std::string_view type_string(json_type type) noexcept
		{
			switch (type)
			{
				case json_type::CONTAINER_FLAG: return "container";
				case json_type::NUMBER_FLAG: return "number";
				case json_type::NULL_VALUE: return "null";
				case json_type::INT: return "int";
				case json_type::UINT: return "uint";
				case json_type::FLOAT: return "float";
				case json_type::ARRAY: return "array";
				case json_type::TABLE: return "table";
				case json_type::STRING: return "string";
				default: return "unknown";
			}
		}
	}	 // namespace detail

	/** @brief Structure representing a Json object (table, array or value) and providing serialization archive operations.
	 *
	 * Json objects can either be used on their own as a way to store Json data or as a base type to implement
	 * Json-like serialization archives. Json objects provide both a serialization archive functionality and
	 * a general Json container functionality, allowing to create arbitrary Json structures.
	 *
	 * @tparam C Character type used for Json strings.
	 * @tparam T Traits type of `C`.
	 * @tparam Alloc Allocator template used to allocate memory of the Json object. */
	template<typename C, typename T = std::char_traits<C>, template<typename...> typename Alloc = std::allocator>
	class basic_json_object
	{
	public:
		typedef inout_archive_category archive_category;

		typedef Alloc<C> string_allocator;

		typedef std::intmax_t int_type;
		typedef std::uintmax_t uint_type;
		typedef double float_type;

		typedef std::basic_string<C, T, string_allocator> string_type;
		typedef std::basic_string_view<C, T> string_view_type;
		typedef string_type key_type;

		typedef Alloc<std::pair<const key_type, basic_json_object>> table_allocator;
		typedef Alloc<basic_json_object> array_allocator;

		typedef ordered_map<key_type, basic_json_object, table_allocator> table_type;
		typedef std::vector<basic_json_object, array_allocator> array_type;

	private:
		template<typename U>
		constexpr static bool is_compatible_bool = std::is_convertible_v<U, bool> && !std::integral<U>;
		template<typename U>
		constexpr static bool is_compatible_string = std::is_constructible_v<string_type, U> && !std::integral<U>;

		template<detail::pair_like U, typename... Args>
		[[nodiscard]] constexpr static bool is_compatible_table_pair() noexcept
		{
			using first_t = std::remove_cvref_t<decltype(std::declval<U>().first)>;
			using second_t = std::remove_cvref_t<decltype(std::declval<U>().second)>;
			return is_compatible_string<first_t> && std::constructible_from<basic_json_object, second_t, Args...>;
		}
		template<typename U, typename... Args>
		[[nodiscard]] constexpr static bool is_compatible_table_pair() noexcept
		{
			return false;
		}

		// clang-format off
		template<typename U, typename... Args>
		constexpr static bool is_compatible_table = std::ranges::range<U> && is_compatible_table_pair<std::ranges::range_value_t<U>, Args...>();
		template<typename U, typename... Args>
		constexpr static bool is_compatible_array = std::ranges::range<U> && std::constructible_from<basic_json_object, U, Args...> &&
				                                    !is_compatible_table_pair<std::ranges::range_value_t<U>, Args...>();
		// clang-format on

		template<typename U, typename... Args>
		constexpr static bool is_value = is_compatible_string<U> || std::integral<U> || std::floating_point<U> ||
										 is_compatible_bool<U> || is_compatible_table<U, Args...> ||
										 is_compatible_array<U, Args...>;
		template<typename U, typename... Args>
		constexpr static bool is_serializable = serializable_with<U, basic_json_object, Args...>;

		template<bool IsConst>
		class object_iterator
		{
			// clang-format off
			using table_iter = std::conditional_t<IsConst, typename table_type::const_iterator, typename table_type::iterator>;
			using array_iter = std::conditional_t<IsConst, typename array_type::const_iterator, typename array_type::iterator>;
			// clang-format on

			static_assert(std::is_trivially_copyable_v<table_iter>);
			static_assert(std::is_trivially_copyable_v<array_iter>);

		public:
			typedef basic_json_object value_type;
			typedef std::conditional_t<IsConst, const value_type, value_type> *pointer;
			typedef std::conditional_t<IsConst, const value_type, value_type> &reference;
			typedef std::size_t size_type;
			typedef std::ptrdiff_t difference_type;

			typedef std::bidirectional_iterator_tag iterator_category;

		public:
			constexpr object_iterator() noexcept = default;

			constexpr object_iterator(const object_iterator &other) noexcept
				: m_padding(other.m_padding), m_is_array(other.m_is_array)
			{
			}
			constexpr object_iterator &operator=(const object_iterator &other) noexcept
			{
				if (this != &other) [[likely]]
				{
					m_padding = other.m_padding;
					m_is_array = other.m_is_array;
				}
				return *this;
			}

			constexpr object_iterator(object_iterator &&other) noexcept
				: m_padding(other.m_padding), m_is_array(other.m_is_array)
			{
			}
			constexpr object_iterator &operator=(object_iterator &&other) noexcept
			{
				m_padding = other.m_padding;
				m_is_array = other.m_is_array;
				return *this;
			}

			template<bool OtherConst, typename = std::enable_if<IsConst && !OtherConst>>
			constexpr object_iterator(const object_iterator<OtherConst> &other) noexcept
				: m_padding(other.m_padding), m_is_array(other.m_is_array)
			{
			}

			constexpr object_iterator operator++(int) noexcept
			{
				auto temp = *this;
				++(*this);
				return temp;
			}
			constexpr object_iterator &operator++() noexcept
			{
				if (!m_is_array)
					++m_table;
				else
					++m_array;
				return *this;
			}
			constexpr object_iterator operator--(int) noexcept
			{
				auto temp = *this;
				--(*this);
				return temp;
			}
			constexpr object_iterator &operator--() noexcept
			{
				if (!m_is_array)
					--m_table;
				else
					--m_array;
				return *this;
			}

			/** Checks if the target Json object has a key. */
			[[nodiscard]] constexpr bool has_key() const noexcept { return !m_is_array; }
			/** Returns key of the target Json object. If it does not have a key, returns an empty string view. */
			[[nodiscard]] constexpr key_type key() const noexcept { return has_key() ? m_table->first : key_type{}; }
			/** Returns reference to the target Json object. */
			[[nodiscard]] constexpr reference object() const noexcept { return has_key() ? m_table->second : *m_array; }

			/** @copydoc object */
			[[nodiscard]] constexpr reference operator*() const noexcept { return object(); }
			/** Returns pointer to the target Json object. */
			[[nodiscard]] constexpr reference get() const noexcept { return std::addressof(operator*()); }
			/** @copydoc get */
			[[nodiscard]] constexpr reference operator->() const noexcept { return get(); }

			constexpr void swap(object_iterator &other) noexcept
			{
				std::swap(m_padding, other.m_padding);
				std::swap(m_is_array, other.m_is_array);
			}
			friend constexpr void swap(object_iterator &a, object_iterator &b) noexcept { a.swap(b); }

			[[nodiscard]] constexpr auto operator<=>(const object_iterator &other) const noexcept
				-> std::common_type_t<std::compare_three_way_result_t<table_iter>, std::compare_three_way_result_t<array_iter>>
			{
				if (!m_is_array)
					return m_table <=> other.m_table;
				else
					return m_array <=> other.m_array;
			}
			[[nodiscard]] constexpr bool operator==(const object_iterator &other) const noexcept
			{
				if (!m_is_array)
					return m_table == other.m_table;
				else
					return m_array == other.m_array;
			}

		private:
			union
			{
				std::byte m_padding[std::max(sizeof(table_iter), sizeof(array_iter))];

				table_iter m_table = {};
				array_iter m_array;
			};
			bool m_is_array = false;
		};

	public:
		typedef basic_json_object value_type;
		typedef object_iterator<false> iterator;
		typedef object_iterator<true> const_iterator;
		typedef std::reverse_iterator<iterator> reverse_iterator;
		typedef std::reverse_iterator<const_iterator> const_reverse_iterator;
		typedef typename iterator::pointer pointer;
		typedef typename const_iterator::pointer const_pointer;
		typedef typename iterator::reference reference;
		typedef typename const_iterator::reference const_reference;
		typedef typename iterator::size_type size_type;
		typedef typename iterator::difference_type difference_type;

	private:
		constexpr static void move_swap(basic_json_object &a, basic_json_object &b)
		{
			auto tmp = basic_json_object{std::move(b)};
			std::destroy_at(&b);
			std::construct_at(&b, std::move(a));
			std::destroy_at(&a);
			std::construct_at(&a, std::move(tmp));
		}

	public:
		/** Initializes an empty Json table object. */
		constexpr basic_json_object() noexcept : m_table(), m_type(json_type::TABLE) {}

		basic_json_object(const basic_json_object &other) : m_type(other.m_type) { copy_init(other); }
		basic_json_object &operator=(const basic_json_object &other)
		{
			if (this != &other) [[likely]]
				copy_assign(other);
			return *this;
		}

		// clang-format off
		basic_json_object(basic_json_object &&other)
			noexcept(std::is_nothrow_move_constructible_v<string_type> &&
					 std::is_nothrow_move_constructible_v<table_type> &&
			         std::is_nothrow_move_constructible_v<array_type>)
			: m_type(other.m_type) { move_init(other); }
		basic_json_object &operator=(basic_json_object &&other)
			noexcept(std::is_nothrow_move_constructible_v<string_type> &&
					 std::is_nothrow_move_constructible_v<table_type> &&
			         std::is_nothrow_move_constructible_v<array_type> &&
			         std::is_nothrow_move_assignable_v<string_type> &&
			         std::is_nothrow_move_assignable_v<table_type> &&
					 std::is_nothrow_move_assignable_v<array_type>)
		{
			move_assign(other);
			return *this;
		}
		// clang-format on

		/** Initializes a null object. */
		constexpr basic_json_object(std::nullptr_t) noexcept : m_type(json_type::NULL_VALUE) {}

		// clang-format off
		/** Initializes a boolean object. */
		template<typename U>
		constexpr basic_json_object(U &&object) noexcept requires is_compatible_bool<std::remove_reference_t<U>>
			: m_bool(static_cast<bool>(object)), m_type(json_type::BOOL) {}
		/** Initializes a floating-point number object. */
		template<typename U>
		constexpr basic_json_object(U &&object) noexcept requires std::floating_point<std::remove_reference_t<U>>
			: m_float(static_cast<float_type>(object)), m_type(json_type::FLOAT) {}
		/** Initializes an integer number object. */
		template<typename U>
		constexpr basic_json_object(U &&object) noexcept requires(std::integral<std::remove_reference_t<U>> && std::is_signed_v<std::remove_reference_t<U>>)
			: m_int(static_cast<int_type>(object)), m_type(json_type::INT)
		{
		}
		/** @copydoc basic_json_object */
		template<typename U>
		constexpr basic_json_object(U &&object) noexcept requires(std::integral<std::remove_reference_t<U>> && !std::is_signed_v<std::remove_reference_t<U>>)
			: m_uint(static_cast<uint_type>(object)), m_type(json_type::UINT)
		{
		}

		/** Initializes a string object.
		 * @param object Value passed to the string's constructor.
		 * @param alloc Allocator used for the string. */
		template<typename U>
		basic_json_object(U &&object, const string_allocator &alloc = {}) requires is_compatible_string<std::remove_reference_t<U>>
			: m_string(std::forward<U>(object), alloc), m_type(json_type::STRING)
		{
		}
		/** Initializes a Json table.
		 * @param object Value passed to the table's constructor.
		 * @param alloc Allocator used for the table. */
		template<typename U>
		basic_json_object(U &&object, const table_allocator &alloc = {}) requires is_compatible_table<std::remove_reference_t<U>>
			: m_table(std::forward<U>(object), alloc), m_type(json_type::TABLE)
		{
		}
		/** Initializes a Json table from an initializer list of key-object pairs.
		 * @param il Initializer list containing key-object pairs of the table.
		 * @param alloc Allocator used for the table. */
		basic_json_object(std::initializer_list<typename table_type::object_type> il, const table_allocator &alloc = {})
			: m_table(il, alloc), m_type(json_type::TABLE)
		{
		}
		/** Initializes a Json array.
		 * @param object Value passed to the array's constructor.
		 * @param alloc Allocator used for the array. */
		template<typename U>
		basic_json_object(U &&object, const array_allocator &alloc = {}) requires is_compatible_array<std::remove_reference_t<U>>
			: m_array(std::forward<U>(object), alloc), m_type(json_type::ARRAY)
		{
		}
		/** Initializes a Json array from an initializer list of objects.
		 * @param il Initializer list containing objects of the array.
		 * @param alloc Allocator used for the array. */
		basic_json_object(std::initializer_list<typename array_type::object_type> il, const array_allocator &alloc = {})
			: m_array(il, alloc), m_type(json_type::ARRAY)
		{
		}
		// clang-format on

		~basic_json_object() { destroy_impl(); }

		/** Returns the underlying type of the Json object. */
		[[nodiscard]] constexpr json_type type() const noexcept { return m_type; }

		/** Checks if the contained object is null. */
		[[nodiscard]] constexpr bool is_null() const noexcept { return m_type == json_type::NULL_VALUE; }
		/** Checks if the contained object is a boolean. */
		[[nodiscard]] constexpr bool is_bool() const noexcept { return m_type == json_type::BOOL; }
		/** Checks if the contained object is a string. */
		[[nodiscard]] constexpr bool is_string() const noexcept { return m_type == json_type::STRING; }

		/** Checks if the contained object is a signed integer. */
		[[nodiscard]] constexpr bool is_int() const noexcept { return m_type == json_type::INT; }
		/** Checks if the contained object is an unsigned integer. */
		[[nodiscard]] constexpr bool is_uint() const noexcept { return m_type == json_type::UINT; }
		/** Checks if the contained object is a floating-point number. */
		[[nodiscard]] constexpr bool is_float() const noexcept { return m_type == json_type::FLOAT; }
		/** Checks if the contained object is a number (integer or floating-point). */
		[[nodiscard]] constexpr bool is_number() const noexcept
		{
			return (m_type & json_type::NUMBER_FLAG) != json_type{0};
		}

		/** Checks if the contained object is a Json array. */
		[[nodiscard]] constexpr bool is_array() const noexcept { return m_type == json_type::ARRAY; }
		/** Checks if the contained object is a Json table. */
		[[nodiscard]] constexpr bool is_table() const noexcept { return m_type == json_type::TABLE; }
		/** Checks if the contained object is a Json container (array or table). */
		[[nodiscard]] constexpr bool is_container() const noexcept
		{
			return (m_type & json_type::CONTAINER_FLAG) != json_type{0};
		}

		/** Returns `true` if the Json object is `null`, or an empty container. */
		[[nodiscard]] constexpr bool empty() const noexcept
		{
			return is_null() || (is_array() && m_array.empty()) || (is_table() && m_table.empty());
		}
		/** If the contained object is a Json container (array or table), returns it's size.
		 * If the contained object is not a Json container, returns `0`. */
		[[nodiscard]] constexpr size_type size() const noexcept
		{
			return is_array() ? m_array.size() : is_table() ? m_table.size() : 0;
		}

		/** Returns iterator to the first element of the container (array or table), or end iterator if the Json object is not a container. */
		[[nodiscard]] constexpr iterator begin() noexcept
		{
			if (is_array())
				return iterator{m_array.begin()};
			else if (is_table())
				return iterator{m_table.begin()};
			return iterator{};
		}
		/** @copydoc begin */
		[[nodiscard]] constexpr const_iterator begin() const noexcept
		{
			if (is_array())
				return const_iterator{m_array.begin()};
			else if (is_table())
				return const_iterator{m_table.begin()};
			return const_iterator{};
		}
		/** @copydoc begin */
		[[nodiscard]] constexpr const_iterator cbegin() const noexcept { return begin(); }
		/** Returns iterator one past the last element of the container (array or table), or a placeholder if the Json object is not a container. */
		[[nodiscard]] constexpr iterator end() noexcept
		{
			if (is_array())
				return iterator{m_array.end()};
			else if (is_table())
				return iterator{m_table.end()};
			return iterator{};
		}
		/** @copydoc end */
		[[nodiscard]] constexpr const_iterator end() const noexcept
		{
			if (is_array())
				return const_iterator{m_array.end()};
			else if (is_table())
				return const_iterator{m_table.end()};
			return const_iterator{};
		}
		/** @copydoc end */
		[[nodiscard]] constexpr const_iterator cend() const noexcept { return end(); }

		/** Returns reverse iterator to the last element of the container (array or table), or end iterator if the Json object is not a container. */
		[[nodiscard]] constexpr reverse_iterator rbegin() noexcept { return reverse_iterator{end()}; }
		/** @copydoc rbegin */
		[[nodiscard]] constexpr const_reverse_iterator rbegin() const noexcept { return const_reverse_iterator{end()}; }
		/** @copydoc rbegin */
		[[nodiscard]] constexpr const_reverse_iterator crbegin() const noexcept { return rbegin(); }
		/** Returns reverse iterator one past the first element of the container (array or table), or a placeholder if the Json object is not a container. */
		[[nodiscard]] constexpr reverse_iterator rend() noexcept { return reverse_iterator{begin()}; }
		/** @copydoc rend */
		[[nodiscard]] constexpr const_reverse_iterator rend() const noexcept { return const_reverse_iterator{begin()}; }
		/** @copydoc rend */
		[[nodiscard]] constexpr const_reverse_iterator crend() const noexcept { return rend(); }

		/** Returns reference to first element of the underlying container (array or table).
		 * @return Reference to the first element.
		 * @throw json_error If the Json object is not a container. */
		[[nodiscard]] reference front()
		{
			switch (m_type)
			{
				case json_type::ARRAY: return m_array.front();
				case json_type::TABLE: return m_table.front().second;
				default: detail::throw_invalid_type(json_type::CONTAINER_FLAG, m_type);
			}
		}
		/** @copydoc front */
		[[nodiscard]] const_reference front() const
		{
			switch (m_type)
			{
				case json_type::ARRAY: return m_array.front();
				case json_type::TABLE: return m_table.front().second;
				default: detail::throw_invalid_type(json_type::CONTAINER_FLAG, m_type);
			}
		}
		/** Returns reference to last element of the underlying container (array or table).
		 * @return Reference to the last element.
		 * @throw json_error If the Json object is not a container. */
		[[nodiscard]] reference back()
		{
			switch (m_type)
			{
				case json_type::ARRAY: return m_array.front();
				case json_type::TABLE: return m_table.front().second;
				default: detail::throw_invalid_type(json_type::CONTAINER_FLAG, m_type);
			}
		}
		/** @copydoc back */
		[[nodiscard]] const_reference back() const
		{
			switch (m_type)
			{
				case json_type::ARRAY: return m_array.back();
				case json_type::TABLE: return m_table.back().second;
				default: detail::throw_invalid_type(json_type::CONTAINER_FLAG, m_type);
			}
		}

		/** Returns a reference to the underlying boolean.
		 * @return Reference to the underlying boolean.
		 * @throw json_error If the Json object is not a boolean. */
		[[nodiscard]] bool &get_bool()
		{
			assert_exact(json_type::BOOL);
			return m_bool;
		}
		/** @copydoc get_bool */
		[[nodiscard]] const bool &get_bool() const
		{
			assert_exact(json_type::BOOL);
			return m_bool;
		}
		/** Returns a pointer to the underlying boolean or `nullptr`, if the Json object is not a boolean. */
		[[nodiscard]] constexpr bool *try_get_bool() noexcept { return m_type == json_type::BOOL ? &m_bool : nullptr; }
		/** @copydoc try_get_bool */
		[[nodiscard]] constexpr const bool *try_get_bool() const noexcept
		{
			return m_type == json_type::BOOL ? &m_bool : nullptr;
		}

		/** Returns a reference to the underlying signed integer.
		 * @return Reference to the underlying signed integer.
		 * @throw json_error If the Json object is not a signed integer. */
		[[nodiscard]] int_type &get_int()
		{
			assert_exact(json_type::INT);
			return m_int;
		}
		/** @copydoc get_int */
		[[nodiscard]] const int_type &get_int() const
		{
			assert_exact(json_type::INT);
			return m_int;
		}
		/** Returns a pointer to the underlying signed integer or `nullptr`, if the Json object is not a signed integer. */
		[[nodiscard]] constexpr int_type *try_get_int() noexcept { return m_type == json_type::INT ? &m_int : nullptr; }
		/** @copydoc try_get_int */
		[[nodiscard]] constexpr const int_type *try_get_int() const noexcept
		{
			return m_type == json_type::INT ? &m_int : nullptr;
		}

		/** Returns a reference to the underlying unsigned integer.
		 * @return Reference to the underlying unsigned integer.
		 * @throw json_error If the Json object is not an unsigned integer. */
		[[nodiscard]] uint_type &get_uint()
		{
			assert_exact(json_type::UINT);
			return m_uint;
		}
		/** @copydoc get_uint */
		[[nodiscard]] const uint_type &get_uint() const
		{
			assert_exact(json_type::UINT);
			return m_uint;
		}
		/** Returns a pointer to the underlying unsigned integer or `nullptr`, if the Json object is not an unsigned integer. */
		[[nodiscard]] constexpr uint_type *try_get_uint() noexcept
		{
			return m_type == json_type::UINT ? &m_uint : nullptr;
		}
		/** @copydoc try_get_uint */
		[[nodiscard]] constexpr const uint_type *try_get_uint() const noexcept
		{
			return m_type == json_type::UINT ? &m_uint : nullptr;
		}

		/** Returns a reference to the underlying floating-point number.
		 * @return Reference to the underlying floating-point number.
		 * @throw json_error If the Json object is not a floating-point number. */
		[[nodiscard]] float_type &get_float()
		{
			assert_exact(json_type::FLOAT);
			return m_float;
		}
		/** @copydoc get_float */
		[[nodiscard]] const float_type &get_float() const
		{
			assert_exact(json_type::FLOAT);
			return m_float;
		}
		/** Returns a pointer to the underlying floating-point number or `nullptr`, if the Json object is not a floating-point number. */
		[[nodiscard]] constexpr float_type *try_get_float() noexcept
		{
			return m_type == json_type::FLOAT ? &m_float : nullptr;
		}
		/** @copydoc try_get_float */
		[[nodiscard]] constexpr const float_type *try_get_float() const noexcept
		{
			return m_type == json_type::FLOAT ? &m_float : nullptr;
		}

		/** Returns a reference to the underlying string.
		 * @return Reference to the underlying string.
		 * @throw json_error If the Json object is not a string. */
		[[nodiscard]] string_type &get_string()
		{
			assert_exact(json_type::STRING);
			return m_string;
		}
		/** @copydoc get_string */
		[[nodiscard]] const string_type &get_string() const
		{
			assert_exact(json_type::STRING);
			return m_string;
		}
		/** Returns a pointer to the underlying string or `nullptr`, if the Json object is not a string. */
		[[nodiscard]] constexpr string_type *try_get_string() noexcept
		{
			return m_type == json_type::STRING ? &m_string : nullptr;
		}
		/** @copydoc try_get_string */
		[[nodiscard]] constexpr const string_type *try_get_string() const noexcept
		{
			return m_type == json_type::STRING ? &m_string : nullptr;
		}

		/** Returns a reference to the underlying array.
		 * @return Reference to the underlying array.
		 * @throw json_error If the Json object is not an array. */
		[[nodiscard]] array_type &get_array()
		{
			assert_exact(json_type::ARRAY);
			return m_array;
		}
		/** @copydoc get_array */
		[[nodiscard]] const array_type &get_array() const
		{
			assert_exact(json_type::ARRAY);
			return m_array;
		}
		/** Returns a pointer to the underlying array or `nullptr`, if the Json object is not an array. */
		[[nodiscard]] constexpr array_type *try_get_array() noexcept
		{
			return m_type == json_type::ARRAY ? &m_array : nullptr;
		}
		/** @copydoc try_get_array */
		[[nodiscard]] constexpr const array_type *try_get_array() const noexcept
		{
			return m_type == json_type::ARRAY ? &m_array : nullptr;
		}

		/** Returns a reference to the underlying table.
		 * @return Reference to the underlying table.
		 * @throw json_error If the Json object is not a table. */
		[[nodiscard]] table_type &get_table()
		{
			assert_exact(json_type::TABLE);
			return m_table;
		}
		/** @copydoc get_table */
		[[nodiscard]] const table_type &get_table() const
		{
			assert_exact(json_type::TABLE);
			return m_table;
		}
		/** Returns a pointer to the underlying table or `nullptr`, if the Json object is not a table. */
		[[nodiscard]] constexpr table_type *try_get_table() noexcept
		{
			return m_type == json_type::TABLE ? &m_table : nullptr;
		}
		/** @copydoc try_get_table */
		[[nodiscard]] constexpr const table_type *try_get_table() const noexcept
		{
			return m_type == json_type::TABLE ? &m_table : nullptr;
		}

		// clang-format off
		/** Converts the underlying integer or floating-point number to the specified type.
		 * @tparam I Numeric type to convert to.
		 * @return Underlying numeric value, converted to `I`.
		 * @throw json_error If the Json object is not an integer or floating-point number. */
		template<typename I>
		[[nodiscard]] I get_number() const requires std::is_arithmetic_v<I>
		{
			switch (m_type)
			{
				case json_type::INT: return static_cast<I>(m_int);
				case json_type::UINT: return static_cast<I>(m_uint);
				case json_type::FLOAT: return static_cast<I>(m_float);
				default: detail::throw_invalid_type(json_type::NUMBER_FLAG, m_type);
			}
		}
		/** Converts the underlying integer or floating-point number to the specified type.
		 * @tparam I Numeric type to convert to.
		 * @param def Default value to return if the Json object is not a number.
		 * @return Underlying numeric value, converted to `I`, or `def`. */
		template<typename I>
		[[nodiscard]] I get_number_or(I def = I{}) const noexcept requires std::is_arithmetic_v<I>
		{
			switch (m_type)
			{
				case json_type::INT: return static_cast<I>(m_int);
				case json_type::UINT: return static_cast<I>(m_uint);
				case json_type::FLOAT: return static_cast<I>(m_float);
				default: return def;
			}
		}
		/** Converts the underlying integer or floating-point number to the specified type.
		 * @tparam I Numeric type to convert to.
		 * @return Underlying numeric value, converted to `I`, or `std::nullopt`. */
		template<typename I>
		[[nodiscard]] std::optional<I> try_get_number() const noexcept requires std::is_arithmetic_v<I>
		{
			switch (m_type)
			{
				case json_type::INT: return static_cast<I>(m_int);
				case json_type::UINT: return static_cast<I>(m_uint);
				case json_type::FLOAT: return static_cast<I>(m_float);
				default: return std::nullopt;
			}
		}
		// clang-format on

		/** Converts the Json object to a null value. */
		void as_null() noexcept
		{
			destroy_impl();
			m_type = json_type::NULL_VALUE;
		}
		/** Converts the Json object to a boolean.
		 * @return Reference to the underlying boolean. */
		bool &as_bool() noexcept
		{
			if (m_type != json_type::BOOL)
			{
				destroy_impl();
				m_type = json_type::BOOL;
				m_bool = {};
			}
			return m_bool;
		}
		/** Converts the Json object to a signed integer.
		 * @return Reference to the underlying signed integer. */
		int_type &as_int() noexcept
		{
			if (m_type != json_type::INT)
			{
				destroy_impl();
				m_type = json_type::INT;
				m_int = {};
			}
			return m_int;
		}
		/** Converts the Json object to an unsigned integer.
		 * @return Reference to the underlying unsigned integer. */
		uint_type &as_uint() noexcept
		{
			if (m_type != json_type::UINT)
			{
				destroy_impl();
				m_type = json_type::UINT;
				m_uint = {};
			}
			return m_uint;
		}
		/** Converts the Json object to a floating-point number.
		 * @return Reference to the underlying floating-point number. */
		float_type &as_float() noexcept
		{
			if (m_type != json_type::FLOAT)
			{
				destroy_impl();
				m_type = json_type::FLOAT;
				m_float = {};
			}
			return m_float;
		}
		/** Converts the Json object to a string.
		 * @return Reference to the underlying string. */
		string_type &as_string()
		{
			if (m_type != json_type::STRING)
			{
				destroy_impl();
				std::construct_at(&m_string);
				m_type = json_type::STRING;
			}
			return m_string;
		}
		/** Converts the Json object to an array.
		 * @return Reference to the underlying array. */
		array_type &as_array()
		{
			if (m_type != json_type::ARRAY)
			{
				destroy_impl();
				std::construct_at(&m_array);
				m_type = json_type::ARRAY;
			}
			return m_array;
		}
		/** Converts the Json object to a table.
		 * @return Reference to the underlying table. */
		table_type &as_table()
		{
			if (m_type != json_type::TABLE)
			{
				destroy_impl();
				std::construct_at(&m_table);
				m_type = json_type::TABLE;
			}
			return m_table;
		}

		// clang-format off
		constexpr void swap(basic_json_object &other)
			noexcept(std::is_nothrow_move_constructible_v<string_type> &&
					 std::is_nothrow_move_constructible_v<table_type> &&
					 std::is_nothrow_move_constructible_v<array_type> &&
					 std::is_nothrow_swappable_v<string_type> &&
					 std::is_nothrow_swappable_v<table_type> &&
					 std::is_nothrow_swappable_v<array_type>)
		{
			if (m_type != other.m_type)
				move_swap(*this, other);
			else
			{
				using std::swap;
				switch (m_type)
				{
					case json_type::STRING: swap(m_string, other.m_string); break;
					case json_type::TABLE: swap(m_table, other.m_table); break;
					case json_type::ARRAY: swap(m_array, other.m_array); break;
					default: swap(m_value_pad, other.m_value_pad); break;
				}
			}
		}
		friend constexpr void swap(basic_json_object &a, basic_json_object &b) noexcept(std::is_nothrow_swappable_v<basic_json_object>)
		{
			a.swap(b);
		}
		// clang-format on

	private:
		void assert_exact(json_type type) const
		{
			if (m_type != type) [[unlikely]]
				detail::throw_invalid_type(type, m_type);
		}

		void destroy_impl()
		{
			switch (m_type)
			{
				case json_type::STRING: std::destroy_at(&m_string); break;
				case json_type::TABLE: std::destroy_at(&m_table); break;
				case json_type::ARRAY: std::destroy_at(&m_array); break;
				default: break;
			}
		}
		void move_init(basic_json_object &other)
		{
			switch (other.m_type)
			{
				case json_type::TABLE: std::construct_at(&m_table, std::move(other.m_table)); break;
				case json_type::ARRAY: std::construct_at(&m_array, std::move(other.m_array)); break;
				case json_type::STRING: std::construct_at(&m_string, std::move(other.m_string)); break;
				default: m_value_pad = other.m_value_pad; break;
			}
		}
		void copy_init(basic_json_object &other)
		{
			switch (other.m_type)
			{
				case json_type::TABLE: std::construct_at(&m_table, other.m_table); break;
				case json_type::ARRAY: std::construct_at(&m_array, other.m_array); break;
				case json_type::STRING: std::construct_at(&m_string, other.m_string); break;
				default: m_value_pad = other.m_value_pad; break;
			}
		}
		void move_assign(basic_json_object &other)
		{
			if (m_type != other.m_type)
			{
				destroy_impl();
				move_init(other);
				m_type = other.m_type;
			}
			else
				switch (other.m_type)
				{
					case json_type::TABLE: m_table = std::move(other.m_table); break;
					case json_type::ARRAY: m_array = std::move(other.m_array); break;
					case json_type::STRING: m_string = std::move(other.m_string); break;
					default: m_value_pad = other.m_value_pad; break;
				}
		}
		void copy_assign(basic_json_object &other)
		{
			if (m_type != other.m_type)
			{
				destroy_impl();
				copy_init(other);
				m_type = other.m_type;
			}
			else
				switch (other.m_type)
				{
					case json_type::TABLE: m_table = other.m_table; break;
					case json_type::ARRAY: m_array = other.m_array; break;
					case json_type::STRING: m_string = other.m_string; break;
					default: m_value_pad = other.m_value_pad; break;
				}
		}

		void write_impl(std::nullptr_t) noexcept
		{
			destroy_impl();
			m_type = json_type::NULL_VALUE;
		}

		// clang-format off
		template<typename U>
		void write_impl(U &&object) noexcept requires is_compatible_bool<std::remove_reference_t<U>>
		{
			m_bool = static_cast<bool>(object);
			m_type = json_type::BOOL;
		}
		template<typename U>
		void write_impl(U &&object) noexcept requires std::integral<std::remove_reference_t<U>>
		{
			destroy_impl();
			if constexpr (std::is_signed_v<U>)
			{
				m_int = static_cast<int_type>(object);
				m_type = json_type::INT;
			}
			else
			{
				m_uint = static_cast<uint_type>(object);
				m_type = json_type::UINT;
			}
		}
		template<typename U>
		void write_impl(U &&object) noexcept requires std::floating_point<std::remove_reference_t<U>>
		{
			destroy_impl();
			m_float = static_cast<float_type>(object);
			m_type = json_type::FLOAT;
		}
		template<typename U>
		void write_impl(U &&object) requires is_compatible_string<std::remove_reference_t<U>>
		{
			if (is_string())
				m_string = string_type{std::forward<U>(object)};
			else
			{
				destroy_impl();
				std::construct_at(&m_string, std::forward<U>(object));
				m_type = json_type::STRING;
			}
		}
		template<typename U>
		void write_impl(U &&object) requires is_compatible_array<std::remove_reference_t<U>>
		{
			if (is_array())
				m_array = array_type{std::forward<U>(object)};
			else
			{
				destroy_impl();
				std::construct_at(&m_array, std::forward<U>(object));
				m_type = json_type::ARRAY;
			}
		}
		template<typename U>
		void write_impl(U &&object) requires is_compatible_table<std::remove_reference_t<U>>
		{
			if (is_table())
				m_table = table_type{std::forward<U>(object)};
			else
			{
				destroy_impl();
				std::construct_at(&m_table, std::forward<U>(object));
				m_type = json_type::TABLE;
			}
		}
		// clang-format on

		union
		{
			std::byte m_value_pad[std::max(sizeof(int_type), sizeof(float_type))] = {};

			bool m_bool;
			int_type m_int;
			uint_type m_uint;
			float_type m_float;
			string_type m_string;

			array_type m_array;
			table_type m_table;
		};
		json_type m_type;
	};

	using json_object = basic_json_object<char>;
}	 // namespace sek::serialization