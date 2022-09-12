/*
 * Created by switchblade on 07/07/22
 */

#pragma once

#include <vector>

#include "sekhmet/ordered_map.hpp"

#include "../manipulators.hpp"
#include "../types/tuples.hpp"
#include "../util.hpp"
#include "json_error.hpp"

namespace sek::serialization
{
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

		typedef std::intmax_t int_type;
		typedef std::uintmax_t uint_type;
		typedef double float_type;

		typedef std::basic_string<C, T, Alloc<C>> string_type;
		typedef typename string_type::allocator_type string_allocator;

		typedef ordered_map<string_type, basic_json_object, default_hash, std::equal_to<>, Alloc<std::pair<const string_type, basic_json_object>>> table_type;
		typedef typename table_type::allocator_type table_allocator;

		typedef std::vector<basic_json_object, Alloc<basic_json_object>> array_type;
		typedef typename table_type::allocator_type array_allocator;

	private:
		constexpr static void move_swap(basic_json_object &a, basic_json_object &b)
		{
			auto tmp = basic_json_object{std::move(b)};
			std::destroy_at(&b);
			std::construct_at(&b, std::move(a));
			std::destroy_at(&a);
			std::construct_at(&a, std::move(tmp));
		}

		template<typename U>
		constexpr static bool is_writeable_bool = std::is_convertible_v<U, bool> && !std::integral<U>;
		template<typename U>
		constexpr static bool is_writeable_int = std::signed_integral<U> || std::is_convertible_v<U, int_type>;
		template<typename U>
		constexpr static bool is_writeable_uint = std::unsigned_integral<U> || std::is_convertible_v<U, uint_type>;
		template<typename U>
		constexpr static bool is_writeable_float = std::floating_point<U> || std::is_convertible_v<U, float_type>;
		template<typename U>
		constexpr static bool is_writeable_string = std::is_constructible_v<string_type, U> && !std::integral<U>;

		// clang-format off
		template<typename U>
		struct is_table_pair_impl : std::false_type
		{
		};
		template<detail::pair_like U>
		struct is_table_pair_impl<U> : std::bool_constant<
			std::constructible_from<basic_json_object, typename detail::pair_traits<U>::second_type> &&
			is_writeable_string<typename detail::pair_traits<U>::first_type>>
		{
		};
		// clang-format on

		template<typename U>
		constexpr static bool is_table_pair = is_table_pair_impl<std::remove_reference_t<U>>::value;

		template<typename U>
		struct is_writeable_table_iter_impl : std::false_type
		{
		};
		template<std::forward_iterator U>
		struct is_writeable_table_iter_impl<U> : std::bool_constant<is_table_pair<std::iter_value_t<U>>>
		{
		};

		template<typename U>
		constexpr static bool is_writeable_table_iter = is_writeable_table_iter_impl<std::remove_reference_t<U>>::value;

		template<typename U>
		struct is_writeable_table_impl : std::false_type
		{
		};
		template<std::ranges::range U>
		struct is_writeable_table_impl<U> : std::bool_constant<is_writeable_table_iter<std::ranges::iterator_t<U>>>
		{
		};

		template<typename U>
		constexpr static bool is_writeable_table = is_writeable_table_impl<std::remove_reference_t<U>>::value;

		// clang-format off
		template<typename U>
		struct is_writeable_array_iter_impl : std::false_type
		{
		};
		template<std::forward_iterator U>
		struct is_writeable_array_iter_impl<U> : std::bool_constant<std::constructible_from<basic_json_object, std::iter_value_t<U>> &&
																	!is_table_pair<std::iter_value_t<U>>>
		{
		};
		// clang-format on

		template<typename U>
		constexpr static bool is_writeable_array_iter = is_writeable_table_iter_impl<std::remove_reference_t<U>>::value;

		template<typename U>
		struct is_writeable_array_impl : std::false_type
		{
		};
		template<std::ranges::range U>
		struct is_writeable_array_impl<U> : std::bool_constant<is_writeable_array_iter<std::ranges::iterator_t<U>>>
		{
		};

		template<typename U>
		constexpr static bool is_writeable_array = is_writeable_array_impl<std::remove_reference_t<U>>::value;

		template<bool IsConst>
		class object_iterator
		{
			friend class basic_json_object;

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

		private:
			constexpr explicit object_iterator(table_iter iter) noexcept : m_table(iter) {}
			constexpr explicit object_iterator(array_iter iter) noexcept : m_array(iter), m_is_array(true) {}

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
			[[nodiscard]] constexpr std::basic_string_view<C, T> key() const noexcept
			{
				return has_key() ? m_table->first : std::basic_string_view<C, T>{};
			}
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

		class read_frame;
		class write_frame;

	private:
		// clang-format off
		template<typename U, typename... Args>
		constexpr static bool is_serializable = serializable_with<std::remove_cvref_t<U>, write_frame, Args...>;
		template<typename U, typename... Args>
		constexpr static bool is_deserializable = deserializable_with<std::remove_cvref_t<U>, write_frame, Args...>;
		template<typename U, typename... Args>
		constexpr static bool is_in_place_deserializable = in_place_deserializable_with<std::remove_cvref_t<U>, write_frame, Args...>;
		// clang-format on

	public:
		/** @brief Archive frame used to read Json objects. */
		class read_frame
		{
		public:
			typedef input_archive_category archive_category;

		public:
			read_frame(const read_frame &) = delete;
			read_frame &operator=(const read_frame &) = delete;
			read_frame(read_frame &&) = delete;
			read_frame &operator=(read_frame &&) = delete;

			/** Initializes a read frame for the specified Json object. */
			constexpr read_frame(const value_type &target) : read_frame(target, target.begin()) {}
			/** @copydoc read_frame
			 * @param pos Initial read position within the target Json object. */
			constexpr read_frame(const value_type &target, const_iterator pos) : m_target(target), m_pos(pos) {}

			/** Returns reference to the target Json object of this frame. */
			[[nodiscard]] constexpr const value_type &target() const noexcept { return m_target; }

			/** Returns iterator to the current read position of this frame. */
			[[nodiscard]] constexpr const_iterator pos() const noexcept { return m_pos; }
			/** Checks if the frame is at the end of the container. */
			[[nodiscard]] constexpr bool is_end() const noexcept { return pos() == target().end(); }

			/** Attempts to seek the current container to the specified key.
			 * @param key Key of the target Json object to seek to.
			 * @return `true` on success, `false` on failure.
			 * @note Read position is modified only on success. */
			constexpr bool try_seek(string_type &&key) noexcept
			{
				return try_seek_impl([&key]() { return std::move(key); });
			}
			template<typename S>
			constexpr bool try_seek(const S &key) noexcept
			{
				return try_seek_impl([&key]() { return key; });
			}

			/** Seeks the current container to the specified key.
			 * @param key Key of the target Json object to seek to.
			 * @return Reference to this frame.
			 * @throw json_error If the target Json object of the frame is not a table.
			 * @note If the key is not present within the container, seeks to the end. */
			read_frame &seek(string_type &&key)
			{
				seek_impl([&key]() { return std::move(key); });
				return *this;
			}
			/** @copydoc seek */
			template<typename S>
			read_frame &seek(const S &key)
			{
				seek_impl([&key]() { return key; });
				return *this;
			}

			/** Seeks the current container to the specified iterator.
			 * @param pos New read position within the current container.
			 * @return Reference to this frame. */
			constexpr read_frame &seek(const_iterator pos) noexcept
			{
				m_pos = pos;
				return *this;
			}

			/** Returns the next Json object of the current container and advances read position.
			 * @return Reference to the next Json object.
			 * @throw json_error If the frame is at the end of the container. */
			[[nodiscard]] const value_type &next()
			{
				assert_not_end();
				return *(m_pos++);
			}

			/** Attempts to read a value from the next Json object in the current container and advance the read position.
			 * @param value Value to read.
			 * @return `true` if read successfully, `false` otherwise.
			 * @note Read position is advanced only on successful read. */
			template<typename U>
			bool try_read(U &value)
			{
				if (!is_end() && m_pos->try_read(value)) [[likely]]
				{
					++m_pos;
					return true;
				}
				return false;
			}
			/** Reads the a value from the next Json object in the current container.
			 * @param value Value to read.
			 * @return Reference to this frame.
			 * @throw json_error On serialization errors.
			 * @note Read position is advanced only on success. */
			template<typename U>
			read_frame &read(U &value)
			{
				assert_not_end();
				m_pos->read(value);
				m_pos++; /* Exception guarantee. */
				return *this;
			}
			/** @copydoc read */
			template<typename U>
			read_frame &operator>>(U &value)
			{
				return read(value);
			}
			/** Reads a value of type `U` from the Json object.
			 * @return Value of type `U`.
			 * @throw json_error On serialization errors. */
			template<typename U, typename... Args>
			[[nodiscard]] U read(std::in_place_type_t<U>)
			{
				assert_not_end();
				U result = m_pos->read(std::in_place_type<U>);
				m_pos++; /* Exception guarantee. */
				return result;
			}

			/** @copybrief try_read
			 * @param value `keyed_entry_t` instance containing the value to read and it's key.
			 * @return `true` if read successfully, `false` otherwise.
			 * @note Read position is advanced only on success. */
			template<typename U>
			bool try_read(keyed_entry_t<C, U> value)
			{
				if (try_seek(value.key) && m_pos->try_read(std::forward<U>(value.value))) [[likely]]
				{
					++m_pos;
					return true;
				}
				return false;
			}
			/** @copybrief operator>>
			 * @param value `keyed_entry_t` instance containing the value to read and it's key.
			 * @return Reference to this frame.
			 * @throw json_error On serialization errors.
			 * @note Read position is advanced only on success. */
			template<typename U>
			read_frame &read(keyed_entry_t<C, U> value)
			{
				seek(value.key);
				m_pos->read(std::forward<U>(value.value));
				m_pos++; /* Exception guarantee. */
				return *this;
			}
			/** @copydoc read */
			template<typename U>
			read_frame &operator>>(keyed_entry_t<C, U> value)
			{
				return read(value);
			}

			// clang-format off
			/** @brief Attempts to deserialize the passed object from the next Json object in the current container
			 * and advance the read position.
			 * @param value Object to deserialize.
			 * @param args Arguments forwarded to the serialization function.
			 * @return `true` if read successfully, `false` otherwise.
			 * @note Read position is advanced only on success. */
			template<typename U, typename... Args>
			bool try_read(U &value, Args &&...args) requires is_deserializable<U, Args...>
			{
				if (!is_end() && m_pos->try_read(value, std::forward<Args>(args)...)) [[likely]]
				{
					++m_pos;
					return true;
				}
				return false;
			}
			/** Deserializes the passed object from the next Json object in the current container.
			 * @param value Object to deserialize.
			 * @return Reference to this frame.
			 * @throw json_error On serialization errors.
			 * @note Read position is advanced only on success. */
			template<typename U>
			read_frame &operator>>(U &value) requires is_deserializable<U> { return read(value); }
			/** @copydoc operator>>
			 * @param args Arguments forwarded to the serialization function. */
			template<typename U, typename... Args>
			read_frame &read(U &value, Args &&...args) requires is_deserializable<U, Args...>
			{
				assert_not_end();
				m_pos->read(value, std::forward<Args>(args)...);
				m_pos++; /* Exception guarantee. */
				return *this;
			}
			/** Deserializes an object of type `U` from the Json object.
			 * @param args Arguments forwarded to the serialization function.
			 * @return Deserialized instance of `U`.
			 * @throw json_error On serialization errors. */
			template<typename U, typename... Args>
			[[nodiscard]] U read(std::in_place_type_t<U>, Args &&...args) requires is_in_place_deserializable<U, Args...>
			{
				assert_not_end();
				U result = m_pos->read(std::in_place_type<U>, std::forward<Args>(args)...);
				m_pos++; /* Exception guarantee. */
				return result;
			}

			/** @copybrief try_read
			 * @param value `keyed_entry_t` instance containing the object to deserialize and it's key.
			 * @param args Arguments forwarded to the serialization function.
			 * @return `true` if read successfully, `false` otherwise.
			 * @note Read position is advanced only on success. */
			template<typename U, typename... Args>
			bool try_read(keyed_entry_t<C, U> value, Args &&...args) requires is_deserializable<U, Args...>
			{
				if (try_seek(value.key) && m_pos->try_read(std::forward<U>(value.value), std::forward<Args>(args)...)) [[likely]]
				{
					++m_pos;
					return true;
				}
				return false;
			}
			/** @copybrief operator>>
			 * @param value `keyed_entry_t` instance containing the object to deserialize and it's key.
			 * @return Reference to this frame.
			 * @throw json_error On serialization errors.
			 * @note Read position is advanced only on success. */
			template<typename U>
			read_frame &operator>>(keyed_entry_t<C, U> value) requires is_deserializable<U> { return read(value); }
			/** @copydoc operator>>
			 * @param args Arguments forwarded to the serialization function. */
			template<typename U, typename... Args>
			read_frame &read(keyed_entry_t<C, U> value, Args &&...args) requires is_deserializable<U, Args...>
			{
				seek(value.key);
				m_pos->read(std::forward<U>(value.value), std::forward<Args>(args)...);
				m_pos++; /* Exception guarantee. */
				return *this;
			}
			// clang-format on

			/** Attempts to read the size of the current container.
			 * @param size `container_size_t` instance receiving the container size.
			 * @return `true` if read successfully, `false` otherwise. */
			template<typename U>
			constexpr bool try_read(container_size_t<U> size) const noexcept
			{
				return m_target.try_read(size);
			}
			/** Reads the size of the current container.
			 * @param size `container_size_t` instance receiving the container size.
			 * @return Reference to this frame. */
			template<typename U>
			read_frame &read(container_size_t<U> size)
			{
				m_target.read(size);
				return *this;
			}
			/** @copydoc read */
			template<typename U>
			const read_frame &read(container_size_t<U> size) const
			{
				m_target.read(size);
				return *this;
			}
			/** @copydoc read */
			template<typename U>
			read_frame &operator>>(container_size_t<U> size)
			{
				return read(size);
			}
			/** @copydoc read */
			template<typename U>
			const read_frame &operator>>(container_size_t<U> size) const
			{
				return read(size);
			}

		private:
			void assert_not_end() const
			{
				if (is_end()) [[unlikely]]
					throw json_error("Unexpected end of frame");
			}

			template<typename F>
			[[nodiscard]] constexpr bool try_seek_impl(F &&key_factory) noexcept
			{
				const auto result = m_target.is_table();
				if (result) [[likely]]
				{
					const auto pos = m_target.m_table.find(key_factory());
					if (pos == m_target.m_table.end()) [[unlikely]]
						return false;
					m_pos = const_iterator{pos};
				}
				return result;
			}
			template<typename F>
			constexpr const_iterator seek_impl(F &&key_factory)
			{
				return m_pos = const_iterator{m_target.get_table().find(key_factory())};
			}

			const value_type &m_target;
			const_iterator m_pos;
		};

		/** @brief Archive frame used to write Json objects. */
		class write_frame
		{
		public:
			typedef output_archive_category archive_category;

		public:
			write_frame(const write_frame &) = delete;
			write_frame &operator=(const write_frame &) = delete;
			write_frame(write_frame &&) = delete;
			write_frame &operator=(write_frame &&) = delete;

			/** Initializes a write frame for the specified Json object.
			 * @note All previous contents of the object will be overwritten. */
			constexpr write_frame(value_type &target) : m_target(target) { m_target.as_table().clear(); }

			/** Returns reference to the target Json object of this frame. */
			[[nodiscard]] constexpr value_type &target() noexcept { return m_target; }
			/** @copydoc target */
			[[nodiscard]] constexpr const value_type &target() const noexcept { return m_target; }

			/** @brief Inserts a new Json object into the current container.
			 * @param key Key used if the current container is a table.
			 * @return Reference to the inserted Json object. */
			[[nodiscard]] value_type &next(string_type &&key)
			{
				return next_impl([&key]() { return std::move(key); });
			}
			/** @copydoc next */
			template<typename S>
			[[nodiscard]] value_type &next(const S &key)
			{
				return next_impl([&key]() { return key; });
			}
			/** @copybrief next
			 * @return Reference to the inserted Json object.
			 * @note Key will be automatically generated using the current container position. */
			[[nodiscard]] value_type &next()
			{
				return next_impl([&table = m_target.m_table]() { return detail::generate_key<C, T>(table.size()); });
			}

			/** @brief Inserts a new Json object into the current container and writes the passed value to it.
			 * @param value Value to write.
			 * @return Reference to this frame. */
			template<typename U>
			write_frame &write(U &&value)
			{
				next().write(std::forward<U>(value));
				return *this;
			}
			/** @copydoc write */
			template<typename U>
			write_frame &operator<<(U &&value)
			{
				return write(std::forward<U>(value));
			}
			/** @copybrief write
			 * @param value `keyed_entry_t` instance containing the value to write and it's key.
			 * @return Reference to this frame. */
			template<typename U>
			write_frame &write(keyed_entry_t<C, U> value)
			{
				next(value.key).write(std::forward<U>(value.value));
				return *this;
			}
			/** @copydoc write */
			template<typename U>
			write_frame &operator<<(keyed_entry_t<C, U> value)
			{
				return write(std::forward<U>(value));
			}

			// clang-format off
			/** @brief Inserts a new Json object into the current container and serializes the passed object to it.
			 * @param value Object to serialize.
			 * @return Reference to this frame.
			 * @throw json_error On serialization errors. */
			template<typename U>
			write_frame &operator<<(U &&value) requires is_serializable<U>
			{
				return write(std::forward<U>(value));
			}
			/** @copydoc operator<<
			 * @param args Arguments forwarded to the serialization function (if any). */
			template<typename U, typename... Args>
			write_frame &write(U &&value, Args &&...args) requires is_serializable<U, Args...>
			{
				next().write(std::forward<U>(value), std::forward<Args>(args)...);
				return *this;
			}
			/** @copybrief operator<<
			 * @param value `keyed_entry_t` instance containing the object to serialize and it's key.
			 * @return Reference to this frame.
			 * @throw json_error On serialization errors. */
			template<typename U>
			write_frame &operator<<(keyed_entry_t<C, U> value) requires is_serializable<U>
			{
				return write(std::forward<U>(value));
			}
			/** @copydoc operator<<
			 * @param args Arguments forwarded to the serialization function (if any). */
			template<typename U, typename... Args>
			write_frame &write(keyed_entry_t<C, U> value, Args &&...args) requires is_serializable<U, Args...>
			{
				next(value.key).write(std::forward<U>(value.value), std::forward<Args>(args)...);
				return *this;
			}
			// clang-format on

			/** Uses the provided size hint to reserve space in the current container.
			 * @param size `container_size_t` instance containing the size hint.
			 * @return Reference to this frame. */
			template<typename U>
			write_frame &write(container_size_t<U> size)
			{
				m_target.write(size);
				return *this;
			}
			/** @copydoc write */
			template<typename U>
			write_frame &operator<<(container_size_t<U> size)
			{
				return write(size);
			}

			/** Switches the frame to array mode (converts the target Json object to array).
			 * @return Reference to this frame.
			 * @throw json_error If the target Json object is a non-empty table. */
			write_frame &write(array_mode_t)
			{
				m_target.write(array_mode());
				return *this;
			}
			/** @copydoc write */
			write_frame &operator<<(array_mode_t) { return write(array_mode()); }

		private:
			template<typename F>
			[[nodiscard]] value_type &next_impl(F &&key_factory)
			{
				switch (m_target.type())
				{
					default: m_target.as_table();
					case json_type::TABLE:
					{
						// clang-format off
						return m_target.m_table
							.emplace(std::piecewise_construct,
									 std::forward_as_tuple(key_factory()),
									 std::forward_as_tuple())
							.first->second;
						// clang-format on
					}
					case json_type::ARRAY: return m_target.m_array.emplace_back();
				}
			}

			value_type &m_target;
		};

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
		constexpr basic_json_object(std::nullptr_t) noexcept : basic_json_object(std::in_place_type<std::nullptr_t>) {}
		/** @copydoc basic_json_object */
		constexpr basic_json_object(std::in_place_type_t<std::nullptr_t>) noexcept : m_type(json_type::NULL_VALUE) {}

		// clang-format off
		/** Initializes a boolean object.
		 * @param value Value of the boolean. */
		template<typename U>
		constexpr basic_json_object(U &&value) noexcept requires is_writeable_bool<U>
			: basic_json_object(std::in_place_type<bool>, std::forward<U>(value)) {}
		/** Initializes a boolean object in-place.
		 * @param value Value of the boolean. */
		template<typename U>
		constexpr basic_json_object(std::in_place_type_t<bool>, U &&value) noexcept requires std::constructible_from<bool, U>
			: m_bool(std::forward<U>(value)), m_type(json_type::BOOL) {}

		/** Initializes a signed integer object.
		 * @param value Value of the integer.  */
		template<typename U>
		constexpr basic_json_object(U &&value) noexcept requires is_writeable_int<U>
			: basic_json_object(std::in_place_type<int_type>, std::forward<U>(value)) {}
		/** Initializes a signed integer object in-place.
		 * @param value Value of the integer. */
		template<typename U>
		constexpr basic_json_object(std::in_place_type_t<int_type>, U &&value) noexcept requires std::constructible_from<int_type, U>
			: m_int(std::forward<U>(value)), m_type(json_type::INT) {}

		/** Initializes an unsigned integer object.
		 * @param value Value of the integer. */
		template<typename U>
		constexpr basic_json_object(U &&value) noexcept requires is_writeable_uint<U>
			: basic_json_object(std::in_place_type<uint_type>, std::forward<U>(value)) {}
		/** Initializes an unsigned integer object in-place.
		 * @param value Value of the integer. */
		template<typename U>
		constexpr basic_json_object(std::in_place_type_t<uint_type>, U &&value) noexcept requires std::constructible_from<uint_type, U>
			: m_uint(std::forward<U>(value)), m_type(json_type::UINT) {}

		/** Initializes a floating-point number object.
		 * @param value Value of the floating-point number. */
		template<typename U>
		constexpr basic_json_object(U &&value) noexcept requires is_writeable_float<U>
			: basic_json_object(std::in_place_type<float_type>, std::forward<U>(value)) {}
		/** Initializes a floating-point number object in-place.
		 * @param value Value of the floating-point number. */
		template<typename U>
		constexpr basic_json_object(std::in_place_type_t<float_type>, U &&value) noexcept requires std::constructible_from<float_type, U>
			: m_float(std::forward<U>(value)), m_type(json_type::FLOAT) {}

		/** Initializes a string object.
		 * @param value Value passed to string's constructor.
		 * @param alloc Allocator used for the string. */
		template<typename U>
		basic_json_object(U &&value, const string_allocator &alloc = {}) requires is_writeable_string<U>
			: basic_json_object(std::in_place_type<string_type>, std::forward<U>(value), alloc) {}
		/** Initializes a string object in-place.
		 * @param args Arguments passed to string's constructor. */
		template<typename... Args>
		basic_json_object(std::in_place_type_t<string_type>, Args &&...args) requires std::constructible_from<string_type, Args...>
			: m_string(std::forward<Args>(args)...), m_type(json_type::STRING) {}

		/** Initializes a Json table from an initializer list of key-value pairs.
		 * @param il Initializer list containing key-value pairs of the table.
		 * @param alloc Allocator used for the table. */
		basic_json_object(std::initializer_list<typename table_type::value_type> il, const table_allocator &alloc = {})
			: basic_json_object(std::in_place_type<table_type>, il, alloc) {}
		/** Initializes a Json table from a range of elements.
		 * @param value Range containing table elements.
		 * @param alloc Allocator used for the array. */
		template<typename U>
		basic_json_object(U &&value, const table_allocator &alloc = {}) requires is_writeable_table<U>
			: basic_json_object(std::in_place_type<table_type>, std::begin(value), std::end(value), alloc) {}
		/** Initializes a Json table from a pair of iterators.
		 * @param first Iterator to the first element.
		 * @param last Iterator one past the last element.
		 * @param alloc Allocator used for the array. */
		template<std::forward_iterator I, std::sentinel_for<I> S>
		basic_json_object(I first, S last, const table_allocator &alloc = {}) requires is_writeable_table_iter<I>
			: basic_json_object(std::in_place_type<table_type>, first, last, alloc) {}
		/** Initializes a Json table in-place.
		 * @param args Arguments passed to table's constructor. */
		template<typename... Args>
		basic_json_object(std::in_place_type_t<table_type>, Args &&...args) requires std::constructible_from<table_type, Args...>
			: m_table(std::forward<Args>(args)...), m_type(json_type::TABLE) {}

		/** Initializes a Json array from an initializer list of value.
		 * @param il Initializer list containing value of the array.
		 * @param alloc Allocator used for the array. */
		basic_json_object(std::initializer_list<typename array_type::value_type> il, const array_allocator &alloc = {})
			: basic_json_object(std::in_place_type<array_type>, il, alloc) {}
		/** Initializes a Json array from a range of elements.
		 * @param value Range containing array elements.
		 * @param alloc Allocator used for the array. */
		template<typename U>
		basic_json_object(U &&value, const array_allocator &alloc = {}) requires is_writeable_array<U>
			: basic_json_object(std::begin(value), std::end(value), alloc) {}
		/** Initializes a Json array from a pair of iterators.
		 * @param first Iterator to the first element.
		 * @param last Iterator one past the last element.
		 * @param alloc Allocator used for the array. */
		template<std::forward_iterator I, std::sentinel_for<I> S>
		basic_json_object(I first, S last, const array_allocator &alloc = {}) requires is_writeable_array_iter<I>
			: basic_json_object(std::in_place_type<array_type>, first, last, alloc) {}
		/** Initializes a Json array in-place.
		 * @param args Arguments passed to array's constructor. */
		template<typename... Args>
		basic_json_object(std::in_place_type_t<array_type>, Args &&...args) requires std::constructible_from<array_type, Args...>
			: m_array(std::forward<Args>(args)...), m_type(json_type::ARRAY)
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
				default: detail::invalid_json_type(json_type::CONTAINER_FLAG, m_type);
			}
		}
		/** @copydoc front */
		[[nodiscard]] const_reference front() const
		{
			switch (m_type)
			{
				case json_type::ARRAY: return m_array.front();
				case json_type::TABLE: return m_table.front().second;
				default: detail::invalid_json_type(json_type::CONTAINER_FLAG, m_type);
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
				default: detail::invalid_json_type(json_type::CONTAINER_FLAG, m_type);
			}
		}
		/** @copydoc back */
		[[nodiscard]] const_reference back() const
		{
			switch (m_type)
			{
				case json_type::ARRAY: return m_array.back();
				case json_type::TABLE: return m_table.back().second;
				default: detail::invalid_json_type(json_type::CONTAINER_FLAG, m_type);
			}
		}

		/** Converts the Json object to a null value. */
		void as_null() noexcept
		{
			destroy_impl();
			m_type = json_type::NULL_VALUE;
		}

		/** Writes a null value to the Json object.
		 * @return Reference to this Json object.
		 * @note Overwrites all previous contents of the object. */
		basic_json_object &write(std::nullptr_t) noexcept
		{
			as_null();
			return *this;
		}
		/** @copydoc write */
		basic_json_object &operator<<(std::nullptr_t) noexcept { return write(nullptr); }

		/** Attempts to read a null value from the Json object. Equivalent to `is_null`.
		 * @return `true` if read successfully, `false` otherwise. */
		constexpr bool try_read(std::nullptr_t) const noexcept { return is_null(); }
		/** Reads a null value from the Json object.
		 * @return Reference to this Json object.
		 * @throw json_error If the Json object is not a null value. */
		const basic_json_object &read(std::nullptr_t) const
		{
			assert_type(json_type::NULL_VALUE);
			return *this;
		}
		/** @copydoc read */
		basic_json_object &operator>>(std::nullptr_t) const { return read(nullptr); }
		/** Reads a null value from the Json object.
		 * @throw json_error If the Json object is not a null value. */
		void read(std::in_place_type_t<std::nullptr_t>) const { assert_type(json_type::NULL_VALUE); }

		/** Converts the Json object to a boolean.
		 * @param value Initial value of the boolean.
		 * @return Reference to the underlying boolean. */
		bool &as_bool(bool value = false) noexcept
		{
			if (m_type != json_type::BOOL)
			{
				destroy_impl();
				m_type = json_type::BOOL;
			}
			return m_bool = value;
		}

		// clang-format off
		/** Writes a boolean to the Json object.
		 * @param value Boolean to write.
		 * @return Reference to this Json object.
		 * @note Overwrites all previous contents of the object. */
		template<typename U>
		basic_json_object &write(U &&value) noexcept requires is_writeable_bool<U>
		{
			as_bool() = value;
			return *this;
		}
		/** @copydoc write */
		template<typename U>
		basic_json_object &operator<<(U &&value) noexcept requires is_writeable_bool<U>
		{
			return write(std::forward<U>(value));
		}
		// clang-format on

		/** Returns a reference to the underlying boolean.
		 * @return Reference to the underlying boolean.
		 * @throw json_error If the Json object is not a boolean. */
		[[nodiscard]] bool &get_bool()
		{
			assert_type(json_type::BOOL);
			return m_bool;
		}
		/** @copydoc get_bool */
		[[nodiscard]] const bool &get_bool() const
		{
			assert_type(json_type::BOOL);
			return m_bool;
		}
		/** Returns a pointer to the underlying boolean or `nullptr`, if the Json object is not a boolean. */
		[[nodiscard]] constexpr bool *try_get_bool() noexcept { return is_bool() ? &m_bool : nullptr; }
		/** @copydoc try_get_bool */
		[[nodiscard]] constexpr const bool *try_get_bool() const noexcept { return is_bool() ? &m_bool : nullptr; }

		/** Attempts to read a boolean from the Json object.
		 * @param value Boolean to read.
		 * @return `true` if read successfully, `false` otherwise. */
		constexpr bool try_read(bool &value) const noexcept
		{
			const auto result = is_bool();
			if (result) [[likely]]
				value = m_bool;
			return result;
		}
		/** Reads a boolean from the Json object.
		 * @param value Boolean to read.
		 * @return Reference to this Json object.
		 * @throw json_error If the Json object is not a boolean. */
		const basic_json_object &read(bool &value) const
		{
			value = get_bool();
			return *this;
		}
		/** @copydoc read */
		basic_json_object &operator>>(bool &value) const { return read(value); }
		/** Reads a boolean from the Json object. Equivalent to `get_bool`.
		 * @return Boolean value.
		 * @throw json_error If the Json object is not a boolean. */
		[[nodiscard]] bool read(std::in_place_type_t<bool>) const { return get_bool(); }

		/** Converts the Json object to a signed integer.
		 * @param value Initial value of the integer.
		 * @return Reference to the underlying signed integer. */
		template<std::signed_integral I = int_type>
		int_type &as_int(I value = I{}) noexcept
		{
			if (m_type != json_type::INT)
			{
				destroy_impl();
				m_type = json_type::INT;
			}
			return m_int = int_type{value};
		}

		// clang-format off
		/** Writes a signed integer to the Json object.
		 * @param value Signed integer to write.
		 * @return Reference to this Json object.
		 * @note Overwrites all previous contents of the object. */
		template<typename U>
		basic_json_object &write(U &&value) noexcept requires is_writeable_int<U>
		{
			as_int() = value;
			return *this;
		}
		/** @copydoc write */
		template<typename U>
		basic_json_object &operator<<(U &&value) noexcept requires is_writeable_int<U>
		{
			return write(std::forward<U>(value));
		}
		// clang-format on

		/** Returns a reference to the underlying signed integer.
		 * @return Reference to the underlying signed integer.
		 * @throw json_error If the Json object is not a signed integer. */
		[[nodiscard]] int_type &get_int()
		{
			assert_type(json_type::INT);
			return m_int;
		}
		/** @copydoc get_int */
		[[nodiscard]] const int_type &get_int() const
		{
			assert_type(json_type::INT);
			return m_int;
		}
		/** Returns a pointer to the underlying signed integer or `nullptr`, if the Json object is not a signed integer. */
		[[nodiscard]] constexpr int_type *try_get_int() noexcept { return m_type == json_type::INT ? &m_int : nullptr; }
		/** @copydoc try_get_int */
		[[nodiscard]] constexpr const int_type *try_get_int() const noexcept
		{
			return m_type == json_type::INT ? &m_int : nullptr;
		}

		/** Converts the Json object to an unsigned integer.
		 * @param value Initial value of the integer.
		 * @return Reference to the underlying unsigned integer. */
		template<std::unsigned_integral I = uint_type>
		uint_type &as_uint(I value = I{}) noexcept
		{
			if (m_type != json_type::UINT)
			{
				destroy_impl();
				m_type = json_type::UINT;
			}
			return m_uint = uint_type{value};
		}

		// clang-format off
		/** Writes an unsigned integer to the Json object.
		 * @param value Unsigned integer to write.
		 * @return Reference to this Json object.
		 * @note Overwrites all previous contents of the object. */
		template<typename U>
		basic_json_object &write(U &&value) noexcept requires is_writeable_uint<U>
		{
			as_uint() = value;
			return *this;
		}
		/** @copydoc write */
		template<typename U>
		basic_json_object &operator<<(U &&value) noexcept requires is_writeable_uint<U>
		{
			return write(std::forward<U>(value));
		}
		// clang-format on

		/** Returns a reference to the underlying unsigned integer.
		 * @return Reference to the underlying unsigned integer.
		 * @throw json_error If the Json object is not an unsigned integer. */
		[[nodiscard]] uint_type &get_uint()
		{
			assert_type(json_type::UINT);
			return m_uint;
		}
		/** @copydoc get_uint */
		[[nodiscard]] const uint_type &get_uint() const
		{
			assert_type(json_type::UINT);
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

		/** Converts the Json object to a floating-point number.
		 * @param value Initial value of the floating-point number.
		 * @return Reference to the underlying floating-point number. */
		template<std::floating_point F = float_type>
		float_type &as_float(F value = F{}) noexcept
		{
			if (m_type != json_type::FLOAT)
			{
				destroy_impl();
				m_type = json_type::FLOAT;
			}
			return m_float = float_type{value};
		}

		// clang-format off
		/** Writes a floating-point number to the Json object.
		 * @param value Floating-point number to write.
		 * @return Reference to this Json object.
		 * @note Overwrites all previous contents of the object. */
		template<typename U>
		basic_json_object &write(U &&value) noexcept requires is_writeable_float<U>
		{
			as_float() = value;
			return *this;
		}
		/** @copydoc write */
		template<typename U>
		basic_json_object &operator<<(U &&value) noexcept requires is_writeable_float<U>
		{
			return write(std::forward<U>(value));
		}
		// clang-format on

		/** Returns a reference to the underlying floating-point number.
		 * @return Reference to the underlying floating-point number.
		 * @throw json_error If the Json object is not a floating-point number. */
		[[nodiscard]] float_type &get_float()
		{
			assert_type(json_type::FLOAT);
			return m_float;
		}
		/** @copydoc get_float */
		[[nodiscard]] const float_type &get_float() const
		{
			assert_type(json_type::FLOAT);
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
				default: detail::invalid_json_type(json_type::NUMBER_FLAG, m_type);
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

		/** Attempts to read an integer or floating-point number from the Json object.
		 * @param value Number to read.
		 * @return `true` if read successfully, `false` otherwise. */
		template<typename I>
		constexpr bool try_read(I &value) const noexcept requires std::is_arithmetic_v<I>
		{
			const auto result = try_get_number<I>();
			if (result) [[likely]]
				value = *result;
			return result;
		}
		/** Reads an integer or floating-point number from the Json object.
		 * @param value Number to read.
		 * @return Reference to this Json object.
		 * @throw json_error If the Json object is not an integer or floating-point number. */
		template<typename I>
		const basic_json_object &read(I &value) const requires std::is_arithmetic_v<I>
		{
			value = get_number<I>();
			return *this;
		}
		/** @copydoc read */
		template<typename I>
		const basic_json_object &operator>>(I &value) const requires std::is_arithmetic_v<I> { return read(value); }
		/** Reads an integer or floating-point number from the Json object.
		 * @return Number value.
		 * @throw json_error If the Json object is not an integer or floating-point number. */
		template<typename I>
		[[nodiscard]] I read(std::in_place_type_t<I>) const requires std::is_arithmetic_v<I>
		{
			return get_number<I>();
		}
		// clang-format on

		/** Converts the Json object to a string.
		 * @param args Arguments passed to string's constructor.
		 * @return Reference to the underlying string. */
		template<typename... Args>
		string_type &as_string(Args &&...args)
		{
			if (m_type == json_type::STRING)
				m_string = string_type{std::forward<Args>(args)...};
			else
			{
				destroy_impl();
				std::construct_at(&m_string, std::forward<Args>(args)...);
				m_type = json_type::STRING;
			}
			return m_string;
		}

		// clang-format off
		/** Writes a string to the Json object.
		 * @param value String to write.
		 * @return Reference to this Json object.
		 * @note Overwrites all previous contents of the object. */
		template<typename S>
		basic_json_object &write(S &&value) requires is_writeable_string<S>
		{
			as_string(std::forward<S>(value));
			return *this;
		}
		/** @copydoc write */
		template<typename S>
		basic_json_object &operator<<(S &&value) requires is_writeable_string<S>
		{
			return write(std::forward<S>(value));
		}
		// clang-format on

		/** Returns a reference to the underlying string.
		 * @return Reference to the underlying string.
		 * @throw json_error If the Json object is not a string. */
		[[nodiscard]] string_type &get_string()
		{
			assert_type(json_type::STRING);
			return m_string;
		}
		/** @copydoc get_string */
		[[nodiscard]] const string_type &get_string() const
		{
			assert_type(json_type::STRING);
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

		/** @brief Attempts to read a string from the Json object.
		 * @param value String to read.
		 * @return `true` if read successfully, `false` otherwise. */
		template<typename U = std::char_traits<C>, typename A = std::allocator<C>>
		constexpr bool try_read(std::basic_string<C, U, A> &value) const
		{
			const auto result = is_string();
			if (result) [[likely]]
				value.assign(m_string.begin(), m_string.end());
			return result;
		}
		/** @copydoc try_read
		 * @note The string view is valid for as long as the underlying string value of the Json object exists. */
		template<typename U = std::char_traits<C>>
		constexpr bool try_read(std::basic_string_view<C, U> &value) const
		{
			const auto result = is_string();
			if (result) [[likely]]
				value = std::basic_string_view<C, U>{m_string.begin(), m_string.end()};
			return result;
		}
		/** @copybrief try_read
		 * @param out Output iterator to read the string into.
		 * @return `true` if read successfully, `false` otherwise. */
		template<std::output_iterator<C> I>
		constexpr bool try_read(I out) const
		{
			const auto result = is_string();
			if (result) [[likely]]
				std::copy(m_string.begin(), m_string.end(), out);
			return result;
		}
		/** @copydoc try_read
		 * @param sent Sentinel for the `out` iterator. */
		template<std::output_iterator<C> I, std::sentinel_for<I> S>
		constexpr bool try_read(I out, S sent) const
		{
			const auto result = is_string();
			if (result) [[likely]]
				for (auto iter = m_string.begin(), last = m_string.end(); iter != last && out != sent; ++iter, ++out)
					*out = *iter;
			return result;
		}

		/** @brief Reads a string from the Json object.
		 * @param value String to read.
		 * @return Reference to this Json object.
		 * @throw json_error If the Json object is not a string. */
		template<typename U = std::char_traits<C>, typename A = std::allocator<C>>
		const basic_json_object &read(std::basic_string<C, U, A> &value) const
		{
			const auto &string = as_string();
			value.assign(string.begin(), string.end());
			return *this;
		}
		/** @copydoc read */
		template<typename U = std::char_traits<C>, typename A = std::allocator<C>>
		const basic_json_object &operator>>(std::basic_string<C, U, A> &value) const
		{
			return read(value);
		}
		/** @copydoc read
		 * @note The string view is valid for as long as the underlying string value of the Json object exists. */
		template<typename U = std::char_traits<C>>
		const basic_json_object &read(std::basic_string_view<C, U> &value) const
		{
			const auto &string = as_string();
			value = std::basic_string_view<C, U>{string.begin(), string.end()};
			return *this;
		}
		/** @copydoc read */
		template<typename U = std::char_traits<C>>
		const basic_json_object &operator>>(std::basic_string_view<C, U> &value) const
		{
			return read(value);
		}
		/** @copybrief read
		 * @param out Output iterator to read the string into.
		 * @return Reference to this Json object.
		 * @throw json_error If the Json object is not a string. */
		template<std::output_iterator<C> I>
		const basic_json_object &read(I out) const
		{
			const auto &string = as_string();
			std::copy(string.begin(), string.end(), out);
			return *this;
		}
		/** @copydoc read */
		template<std::output_iterator<C> I>
		const basic_json_object &operator>>(I out) const
		{
			return read(out);
		}
		/** @copydoc read
		 * @param sent Sentinel for the `out` iterator. */
		template<std::output_iterator<C> I, std::sentinel_for<I> S>
		const basic_json_object &read(I out, S sent) const
		{
			const auto &string = as_string();
			for (auto iter = string.begin(), last = string.end(); iter != last && out != sent; ++iter, ++out)
				*out = *iter;
			return *this;
		}

		/** Converts the Json object to an array.
		 * @param args Arguments passed to array's constructor.
		 * @return Reference to the underlying array. */
		template<typename... Args>
		array_type &as_array(Args &&...args)
		{
			if (m_type == json_type::ARRAY)
				m_array = array_type{std::forward<Args>(args)...};
			else
			{
				destroy_impl();
				std::construct_at(&m_array, std::forward<Args>(args)...);
				m_type = json_type::ARRAY;
			}
			return m_array;
		}

		// clang-format off
		/** Writes an array to the Json object.
		 * @param value Array to write.
		 * @return Reference to this Json object.
		 * @note Overwrites all previous contents of the object. */
		template<typename U>
		basic_json_object &write(U &&value) requires is_writeable_array<U>
		{
			as_array(std::forward<U>(value));
			return *this;
		}
		/** @copydoc write */
		template<typename U>
		basic_json_object &operator<<(U &&value) requires is_writeable_array<U>
		{
			return write(std::forward<U>(value));
		}
		/** Writes an array to the Json object.
		 * @param first Iterator to the first element.
		 * @param last Iterator one past the last element.
		 * @return Reference to this Json object.
		 * @note Overwrites all previous contents of the object. */
		template<std::forward_iterator I, std::sentinel_for<I> S>
		basic_json_object &write(I first, S last) requires is_writeable_array_iter<I>
		{
			as_array(first, last);
			return *this;
		}
		// clang-format on

		/** Returns a reference to the underlying array.
		 * @return Reference to the underlying array.
		 * @throw json_error If the Json object is not an array. */
		[[nodiscard]] array_type &get_array()
		{
			assert_type(json_type::ARRAY);
			return m_array;
		}
		/** @copydoc get_array */
		[[nodiscard]] const array_type &get_array() const
		{
			assert_type(json_type::ARRAY);
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

		/** Converts the Json object to a table.
		 * @param args Arguments passed to table's constructor.
		 * @return Reference to the underlying table. */
		template<typename... Args>
		table_type &as_table(Args &&...args)
		{
			if (m_type == json_type::TABLE)
				m_table = table_type{std::forward<Args>(args)...};
			else
			{
				destroy_impl();
				std::construct_at(&m_table, std::forward<Args>(args)...);
				m_type = json_type::TABLE;
			}
			return m_table;
		}

		// clang-format off
		/** Writes a table to the Json object.
		 * @param value Table to write.
		 * @return Reference to this Json object.
		 * @note Overwrites all previous contents of the object. */
		template<typename U>
		basic_json_object &write(U &&value) requires is_writeable_table<U>
		{
			as_table(std::forward<U>(value));
			return *this;
		}
		/** @copydoc write */
		template<typename U>
		basic_json_object &operator<<(U &&value) requires is_writeable_table<U>
		{
			return write(std::forward<U>(value));
		}
		/** Writes a table to the Json object.
		 * @param first Iterator to the first element.
		 * @param last Iterator one past the last element.
		 * @return Reference to this Json object.
		 * @note Overwrites all previous contents of the object. */
		template<std::forward_iterator I, std::sentinel_for<I> S>
		basic_json_object &write(I first, S last) requires is_writeable_table_iter<I>
		{
			as_table(first, last);
			return *this;
		}
		// clang-format on

		/** Returns a reference to the underlying table.
		 * @return Reference to the underlying table.
		 * @throw json_error If the Json object is not a table. */
		[[nodiscard]] table_type &get_table()
		{
			assert_type(json_type::TABLE);
			return m_table;
		}
		/** @copydoc get_table */
		[[nodiscard]] const table_type &get_table() const
		{
			assert_type(json_type::TABLE);
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
		/** Attempts to deserialize the passed object from the Json object.
		 * @param value Object to deserialize.
		 * @param args Arguments forwarded to the serialization function.
		 * @return `true` if read successfully, `false` otherwise. */
		template<typename U, typename... Args>
		bool try_read(U &value, Args &&...args) const requires is_deserializable<U, Args...>
		{
			if (is_container()) [[likely]]
			{
				try
				{
					read_frame frame{*this};
					detail::do_deserialize(value, frame, std::forward<Args>(args)...);
					return true;
				}
				catch (archive_error &) { /* return `false` on serialization errors. */ }
			}
			return false;
		}
		/** Deserializes the passed object from the Json object.
		 * @param value Object to deserialize.
		 * @return Reference to this Json object.
		 * @throw json_error On serialization errors. */
		template<typename U>
		const basic_json_object &operator>>(U &value) const requires is_deserializable<U> { return read(value); }
		/** @copydoc operator>>
		 * @param args Arguments forwarded to the serialization function. */
		template<typename U, typename... Args>
		const basic_json_object &read(U &value, Args &&...args) const requires is_deserializable<U, Args...>
		{
			if (!is_container()) [[unlikely]]
				detail::invalid_json_type(json_type::CONTAINER_FLAG, m_type);

			read_frame frame{*this};
			detail::do_deserialize(value, frame, std::forward<Args>(args)...);
			return *this;
		}
		/** Deserializes an object of type `U` from the Json object.
		 * @param args Arguments forwarded to the serialization function.
		 * @return Deserialized instance of `U`.
		 * @throw json_error On serialization errors. */
		template<typename U, typename... Args>
		[[nodiscard]] U read(std::in_place_type_t<U>, Args &&...args) const requires is_in_place_deserializable<U, Args...>
		{
			if (!is_container()) [[unlikely]]
				detail::invalid_json_type(json_type::CONTAINER_FLAG, m_type);

			read_frame frame{*this};
			return detail::do_deserialize(std::in_place_type<U>, frame, std::forward<Args>(args)...);
		}

		/** Serializes the passed object to the Json object.
		 * @param value Object to serialize.
		 * @return Reference to this Json object.
		 * @throw json_error On serialization errors.
		 * @note Overwrites all previous contents of the object. */
		template<typename U>
		basic_json_object &operator<<(const U &value) requires is_serializable<U> { return write(value); }
		/** @copydoc operator<<
		 * @param args Arguments forwarded to the serialization function. */
		template<typename U, typename... Args>
		basic_json_object &write(const U &value, Args &&...args) requires is_serializable<U, Args...>
		{
			write_frame frame{*this};
			detail::do_serialize(value, frame, std::forward<Args>(args)...);
			return *this;
		}
		// clang-format on

		/** Attempts to read the size of the Json container (array or table).
		 * @param size `container_size_t` instance receiving the container size.
		 * @return `true` if read successfully, `false` otherwise. */
		template<typename U>
		constexpr bool try_read(container_size_t<U> size) const noexcept
		{
			switch (m_type)
			{
				case json_type::ARRAY: size.value = m_array.size(); return true;
				case json_type::TABLE: size.value = m_table.size(); return true;
				default: return false;
			}
		}
		/** Reads the size of the Json container (array or table).
		 * @param size `container_size_t` instance receiving the container size.
		 * @return Reference to this Json object.
		 * @throw json_error If the Json object is not a container (array or table). */
		template<typename U>
		const basic_json_object &read(container_size_t<U> size) const
		{
			switch (m_type)
			{
				case json_type::ARRAY: size.value = m_array.size(); break;
				case json_type::TABLE: size.value = m_table.size(); break;
				default: detail::invalid_json_type(json_type::CONTAINER_FLAG, m_type);
			}
			return *this;
		}
		/** @copydoc read */
		template<typename U>
		const basic_json_object &operator>>(container_size_t<U> size) const
		{
			return read(size);
		}

		/** Uses the provided size hint to reserve space in the current container (array or table).
		 * @param size `container_size_t` instance containing the size hint.
		 * @return Reference to this Json object.
		 * @note If the Json object is not a container, converts it to a table as if via `as_table`. */
		template<typename U>
		basic_json_object &write(container_size_t<U> size)
		{
			switch (m_type)
			{
				default: as_table();
				case json_type::TABLE: m_table.reserve(static_cast<size_type>(size.value)); break;
				case json_type::ARRAY: m_array.reserve(static_cast<size_type>(size.value)); break;
			}
			return *this;
		}
		/** @copydoc write */
		template<typename U>
		basic_json_object &operator<<(container_size_t<U> size)
		{
			return write(size);
		}

		/** If the Json object is not a container or is an empty table, converts it to array as if via `as_array`.
		 * @return Reference to this Json object.
		 * @throw json_error If the Json object is a non-empty table. */
		write_frame &write(array_mode_t)
		{
			switch (m_type)
			{
				case json_type::ARRAY: break;
				case json_type::TABLE: assert_empty();
				default: as_array(); break;
			}
			return *this;
		}
		/** @copydoc write */
		write_frame &operator<<(array_mode_t) { return write(array_mode()); }

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
					default: swap(m_literal_pad, other.m_literal_pad); break;
				}
			}
		}
		friend constexpr void swap(basic_json_object &a, basic_json_object &b) noexcept(std::is_nothrow_swappable_v<basic_json_object>)
		{
			a.swap(b);
		}
		// clang-format on

	private:
		void assert_type(json_type type) const
		{
			if (m_type != type) [[unlikely]]
				detail::invalid_json_type(type, m_type);
		}
		void assert_empty() const
		{
			if (!empty()) [[unlikely]]
				throw json_error("Expected empty Json object");
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
				default: m_literal_pad = other.m_literal_pad; break;
			}
		}
		void copy_init(basic_json_object &other)
		{
			switch (other.m_type)
			{
				case json_type::TABLE: std::construct_at(&m_table, other.m_table); break;
				case json_type::ARRAY: std::construct_at(&m_array, other.m_array); break;
				case json_type::STRING: std::construct_at(&m_string, other.m_string); break;
				default: m_literal_pad = other.m_literal_pad; break;
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
					default: m_literal_pad = other.m_literal_pad; break;
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
					default: m_literal_pad = other.m_literal_pad; break;
				}
		}

		union
		{
			std::byte m_literal_pad[std::max(sizeof(int_type), sizeof(float_type))] = {};

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

	/** @brief `basic_json_object` alias for `char` character type. */
	using json_object = basic_json_object<char>;

	/* TODO: Implement read & write overloads that accept `std::nothrow` and return an `expected<void, std::error_code>`. */
	/* TODO: Refactor existing read & write operations to fit with the `std::error_code` overloads. */
}	 // namespace sek::serialization