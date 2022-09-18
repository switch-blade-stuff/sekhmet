/*
 * Created by switchblade on 07/07/22
 */

#pragma once

#include <vector>

#include "../../../detail/owned_ptr.hpp"
#include "../../../ordered_map.hpp"
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
		typedef typename array_type::allocator_type array_allocator;

		typedef std::initializer_list<owned_ptr<basic_json_object>> initializer_list;

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
		constexpr static bool is_value_type = std::same_as<U, std::nullptr_t> || std::same_as<U, bool> ||
											  std::same_as<U, int_type> || std::same_as<U, uint_type> ||
											  std::same_as<U, float_type> || std::same_as<U, string_type> ||
											  std::same_as<U, table_type> || std::same_as<U, array_type>;

		template<typename U>
		constexpr static bool is_compatible_int = std::signed_integral<U> && !std::same_as<U, int_type>;
		template<typename U>
		constexpr static bool is_compatible_uint = std::unsigned_integral<U> && !std::same_as<U, uint_type>;
		template<typename U>
		constexpr static bool is_compatible_float = std::floating_point<U> && !std::same_as<U, float_type>;

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
			constexpr object_iterator() noexcept : m_table(), m_is_array(false) {}

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
				std::array<std::byte, std::max(sizeof(table_iter), sizeof(array_iter))> m_padding;

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
			 * @throw archive_error If the target Json object of the frame is not a table.
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
			 * @throw archive_error If the frame is at the end of the container. */
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
			 * @throw archive_error On serialization errors.
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
			 * @throw archive_error On serialization errors. */
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
			 * @throw archive_error On serialization errors.
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
			 * @throw archive_error On serialization errors.
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
			 * @throw archive_error On serialization errors. */
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
			 * @throw archive_error On serialization errors.
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

			/* TODO: Finish refactoring this. */

		private:
			void assert_not_end() const
			{
				if (is_end()) [[unlikely]]
					throw archive_error{make_error_code(archive_errc::UNEXPECTED_END)};
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
			 * @param value Value to write. */
			template<typename U>
			void write(U &&value)
			{
				next().write(std::forward<U>(value));
			}
			/** @copydoc write(U &&) */
			template<typename U>
			expected<void, std::error_code> write(std::nothrow_t, U &&value)
			{
				return next().write(std::nothrow, std::forward<U>(value));
			}
			/** @copydoc write(U &&)
			 * @return Reference to this frame. */
			template<typename U>
			write_frame &operator<<(U &&value)
			{
				return write(std::forward<U>(value));
			}

			/** @brief Inserts a new Json object into the current container at the specified key and writes the passed value to it.
			 * @param value `keyed_entry_t` instance containing the value to write and it's key.
			 * @throw archive_error If the underlying container is not a table. */
			template<typename U>
			void write(keyed_entry_t<C, U> value)
			{
				next(value.key).write(std::forward<U>(value.value));
			}
			/** @copybrief write(keyed_entry_t<C, U>)
			 * @param value `keyed_entry_t` instance containing the value to write and it's key.
			 * @return `void`, or `archive_errc::INVALID_TYPE` if the underlying container is not a table. */
			template<typename U>
			expected<void, std::error_code> write(std::nothrow_t, keyed_entry_t<C, U> value)
			{
				if (m_target.is_table()) [[likely]]
					return next(value.key).write(std::nothrow, std::forward<U>(value.value));
				return unexpected{make_error_code(archive_errc::INVALID_TYPE)};
			}
			/** @copydoc write(keyed_entry_t<C, U>)
			 * @return Reference to this frame. */
			template<typename U>
			write_frame &operator<<(keyed_entry_t<C, U> value)
			{
				return write(std::forward<U>(value));
			}

			// clang-format off
			/** @brief Inserts a new Json object into the current container and serializes the passed object to it.
			 * @param value Object to serialize.
			 * @param args Arguments forwarded to the serialization function (if any).
			 * @throw archive_error On serialization errors. */
			template<typename V, typename... Args>
			void write(V &&value, Args &&...args) requires is_serializable<V, Args...>
			{
				next().write(std::forward<V>(value), std::forward<Args>(args)...);
			}
			/** @copybrief write(V &&, Args &&...)
			 * @param value Object to serialize.
			 * @param args Arguments forwarded to the serialization function (if any).
			 * @return `void`, or an error code on deserialization error. */
			template<typename V, typename... Args>
			expected<void, std::error_code> write(std::nothrow_t, V &&value, Args &&...args) requires is_serializable<V, Args...>
			{
				return next().write(std::nothrow, std::forward<V>(value), std::forward<Args>(args)...);
			}
			/** @brief Inserts a new Json object into the current container and serializes the passed object to it.
			 * @param value Object to serialize.
			 * @return Reference to this frame.
			 * @throw archive_error On serialization errors. */
			template<typename V>
			write_frame &operator<<(V &&value) requires is_serializable<V>
			{
				write(std::forward<V>(value));
				return *this;
			}

			/** @copybrief write(V &&, Args &&...)
			 * @param value `keyed_entry_t` instance containing the object to serialize and it's key.
			 * @param args Arguments forwarded to the serialization function (if any).
			 * @throw archive_error On serialization errors. */
			template<typename V, typename... Args>
			void write(keyed_entry_t<C, V> value, Args &&...args) requires is_serializable<V, Args...>
			{
				next(value.key).write(std::forward<V>(value.value), std::forward<Args>(args)...);
			}
			/** @copybrief write(V &&, Args &&...)
			 * @param value `keyed_entry_t` instance containing the object to serialize and it's key.
			 * @param args Arguments forwarded to the serialization function (if any).
			 * @return `void`, or an error code on deserialization error. */
			template<typename V, typename... Args>
			expected<void, std::error_code> write(std::nothrow_t, keyed_entry_t<C, V> value, Args &&...args) requires is_serializable<V, Args...>
			{
				return next(value.key).write(std::nothrow, std::forward<V>(value), std::forward<Args>(args)...);
			}
			/** @copybrief operator<<(V &&)
			 * @param value `keyed_entry_t` instance containing the object to serialize and it's key.
			 * @return Reference to this frame.
			 * @throw archive_error On serialization errors. */
			template<typename U>
			write_frame &operator<<(keyed_entry_t<C, U> value) requires is_serializable<U>
			{
				write(std::forward<U>(value));
				return *this;
			}
			// clang-format on

			/** Uses the provided size hint to reserve space in the current container.
			 * @param size `container_size_t` instance containing the size hint. */
			template<typename U>
			void write(container_size_t<U> size)
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
			 * @throw archive_error If the target Json object is a non-empty table. */
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

		// clang-format off
		/** Creates an array Json object.
		 * @param il Initializer list containing array elements.
		 * @param alloc Allocator used for the array.
		 * @return Created Json object instance. */
		[[nodiscard]] static basic_json_object make_array(initializer_list il = {}, const array_allocator &alloc = array_allocator{})
		{
			basic_json_object result;
			result.init_array(il, alloc);
			return result;
		}
		/** @brief Creates a table Json object.
		 * @param il Initializer list containing table elements.
		 * @param alloc Allocator used for the table.
		 * @return Created Json object instance.
		 * @throw archive_error If the initializer list does not consist of key-value pairs. */
		[[nodiscard]] static basic_json_object make_table(initializer_list il = {}, const table_allocator &alloc = table_allocator{})
		{
			basic_json_object result;
			result.init_table(il, alloc);
			return result;
		}
		/** @copybrief make_table
		 * @param il Initializer list containing table elements.
		 * @param alloc Allocator used for the table.
		 * @return Created Json object instance or `archive_errc::INVALID_DATA` if the
		 * initializer list does not consist of key-value pairs. */
		[[nodiscard]] static auto make_table(std::nothrow_t, initializer_list il = {}, const table_allocator &alloc = table_allocator{})
			-> expected<basic_json_object, std::error_code>
		{
			expected<basic_json_object, std::error_code> result;
			if (const auto err = result.init_table(std::nothrow, il, alloc); !err) [[unlikely]]
				result = expected<basic_json_object, std::error_code>{err};
			return result;
		}
		// clang-format on

	public:
		/** Initializes a null Json object. */
		constexpr basic_json_object() noexcept : m_literal_bytes(), m_type(json_type::NULL_VALUE) {}

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

		~basic_json_object() { destroy_impl(); }

		/** Returns the underlying type of the Json object. */
		[[nodiscard]] constexpr json_type type() const noexcept { return m_type; }

		/** Checks if the contained object is null. */
		[[nodiscard]] constexpr bool is_null() const noexcept { return m_type == json_type::NULL_VALUE; }
		/** Checks if the contained object is a boolean. */
		[[nodiscard]] constexpr bool is_bool() const noexcept { return m_type == json_type::BOOL; }
		/** Checks if the contained object is a signed integer. */
		[[nodiscard]] constexpr bool is_int() const noexcept { return m_type == json_type::INT; }
		/** Checks if the contained object is an unsigned integer. */
		[[nodiscard]] constexpr bool is_uint() const noexcept { return m_type == json_type::UINT; }
		/** Checks if the contained object is a floating-point number. */
		[[nodiscard]] constexpr bool is_float() const noexcept { return m_type == json_type::FLOAT; }
		/** Checks if the contained object is a number (integer or floating-point). */
		[[nodiscard]] constexpr bool is_number() const noexcept
		{
			return (type() & json_type::NUMBER_FLAG) != json_type{0};
		}
		/** Checks if the contained object is a string. */
		[[nodiscard]] constexpr bool is_string() const noexcept { return m_type == json_type::STRING; }
		/** Checks if the contained object is a Json array. */
		[[nodiscard]] constexpr bool is_array() const noexcept { return m_type == json_type::ARRAY; }
		/** Checks if the contained object is a Json table. */
		[[nodiscard]] constexpr bool is_table() const noexcept { return m_type == json_type::TABLE; }
		/** Checks if the contained object is a Json container. */
		[[nodiscard]] constexpr bool is_container() const noexcept
		{
			return (type() & json_type::CONTAINER_FLAG) != json_type{0};
		}

		/** Returns `true` if the Json object is `null`, or an empty container. */
		[[nodiscard]] constexpr bool empty() const noexcept
		{
			return is_null() || (is_array() && m_array.empty()) || (is_table() && m_table.empty());
		}
		/** If the contained object is a Json container, returns it's size.
		 * If the contained object is not a Json container, returns `0`. */
		[[nodiscard]] constexpr size_type size() const noexcept
		{
			return is_array() ? m_array.size() : is_table() ? m_table.size() : 0;
		}

		/** Returns iterator to the first element of the container, or end iterator if the Json object is not a container. */
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
		/** Returns iterator one past the last element of the container, or a placeholder if the Json object is not a container. */
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

		/** Returns reverse iterator to the last element of the container, or end iterator if the Json object is not a container. */
		[[nodiscard]] constexpr reverse_iterator rbegin() noexcept { return reverse_iterator{end()}; }
		/** @copydoc rbegin */
		[[nodiscard]] constexpr const_reverse_iterator rbegin() const noexcept { return const_reverse_iterator{end()}; }
		/** @copydoc rbegin */
		[[nodiscard]] constexpr const_reverse_iterator crbegin() const noexcept { return rbegin(); }
		/** Returns reverse iterator one past the first element of the container, or a placeholder if the Json object is not a container. */
		[[nodiscard]] constexpr reverse_iterator rend() noexcept { return reverse_iterator{begin()}; }
		/** @copydoc rend */
		[[nodiscard]] constexpr const_reverse_iterator rend() const noexcept { return const_reverse_iterator{begin()}; }
		/** @copydoc rend */
		[[nodiscard]] constexpr const_reverse_iterator crend() const noexcept { return rend(); }

		/** Returns reference to first element of the underlying container.
		 * @return Reference to the first element.
		 * @throw archive_error If the Json object is not a container. */
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
		/** Returns reference to last element of the underlying container.
		 * @return Reference to the last element.
		 * @throw archive_error If the Json object is not a container. */
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

		// clang-format off
		/** Converts the Json object to the specified value type.
		 * @tparam U Value type to convert the Json object to. Must be one of the value types (`std::nullptr_t`,
		 * `bool`, `int_type`, `uint_type`, `float_type`, `string_type`, `table_type`, `array_type`).
		 * @tparam Args Arguments passed to the constructor of the value type.
		 * @param args Arguments passed to the constructor of the value type.
		 * @return Reference to this Json object.
		 * @note Previous value of the Json object will be overwritten.
		 * @note If any exception is thrown during initialization of the value type, Json object is set to the null value. */
		template<typename U, typename... Args>
		basic_json_object &as(Args &&...args) requires is_value_type<U> && std::constructible_from<U, Args...>
		{
			as_impl<U>(std::forward<Args>(args)...);
			return *this;
		}
		/** Converts the Json object to a signed integer via converting to `int_type`.
		 * @param value Value of the signed integer.
		 * @return Reference to this Json object.
		 * @note Previous value of the Json object will be overwritten. */
		template<typename I>
		basic_json_object &as(I &&value) requires is_compatible_int<std::remove_cvref_t<I>>
		{
			return as<int_type>(static_cast<int_type>(std::forward<I>(value)));
		}
		/** Converts the Json object to an unsigned integer via converting to `uint_type`.
		 * @param value Value of the unsigned integer.
		 * @return Reference to this Json object.
		 * @note Previous value of the Json object will be overwritten. */
		template<typename I>
		basic_json_object &as(I &&value) requires is_compatible_uint<std::remove_cvref_t<I>>
		{
			return as<uint_type>(static_cast<uint_type>(std::forward<I>(value)));
		}
		/** Converts the Json object to a floating-point number via converting to `uint_type`.
		 * @param value Value of the unsigned integer.
		 * @return Reference to this Json object.
		 * @note Previous value of the Json object will be overwritten. */
		template<typename F>
		basic_json_object &as(F &&value) requires is_compatible_float<std::remove_cvref_t<F>>
		{
			return as<float_type>(static_cast<float_type>(std::forward<F>(value)));
		}
		// clang-format on

		// clang-format off
		/** @brief Returns copy of the specified value type.
		 * @tparam U Value type to retrieve from the Json object. Must either be one of the value types (`bool`, `int_type`,
		 * `uint_type`, `float_type`, `string_type`, `table_type`, `array_type`), or a compatible arithmetic type.
		 * @return Copy of the requested value of the Json object.
		 * @note If the requested type is a non-`bool` arithmetic (number) type, appropriate conversions will be preformed.
		 * @throw archive_error If the Json object does not contain the specified type.
		 * @example
		 * @code{cpp}
		 * int32_t int32_value = json.get<int32_t>();
		 * int16_t int16_value = json.get<int16_t>();
		 * @endcode */
		template<typename U>
		[[nodiscard]] U get() const requires(is_value_type<U> || std::is_arithmetic_v<U>) { return get_impl<U>(); }
		/** @copybrief get
		 * @tparam U Value type to retrieve from the Json object. Must either be one of the value types (`bool`, `int_type`,
		 * `uint_type`, `float_type`, `string_type`, `table_type`, `array_type`), or a compatible arithmetic type.
		 * @return Copy of the requested value of the Json object, or `archive_errc::INVALID_TYPE`
		 * if the Json object does not contain the specified type.
		 * @note If the requested type is a non-`bool` arithmetic (number) type, appropriate conversions will be preformed.
		 * @example
		 * @code{cpp}
		 * expected<int32_t, std::error_code> int32_value = json.get<int32_t>(std::nothrow);
		 * expected<int16_t, std::error_code> int16_value = json.get<int16_t>(std::nothrow);
		 * @endcode */
		template<typename U>
		[[nodiscard]] expected<U, std::error_code> get(std::nothrow_t) const requires(is_value_type<U> || std::is_arithmetic_v<U>)
		{
			return get_impl<U>(std::nothrow);
		}

		/** Returns reference to the specified value type.
		 * @tparam R Rvalue reference to the value type to retrieve from the Json object. Must be one of the value
		 * types (`bool`, `int_type`, `uint_type`, `float_type`, `string_type`, `table_type`, `array_type`).
		 * @return Reference of the value of the Json object.
		 * @throw archive_error If the Json object does not contain the specified type.
		 * @example
		 * @code{cpp}
		 * auto &bool_value = json.get<bool &>();
		 * @endcode */
		template<typename R>
		[[nodiscard]] R get() requires(std::is_rvalue_reference_v<R> && is_value_type<std::remove_cvref_t<R>>)
		{
			return get_ref_impl<std::remove_cvref_t<R>>();
		}
		/** @copydoc get */
		template<typename R>
		[[nodiscard]] auto get() const -> std::add_const_t<std::remove_reference_t<R>> &
			requires(std::is_rvalue_reference_v<R> && is_value_type<std::remove_cvref_t<R>>)
		{
			return get_ref_impl<std::remove_cvref_t<R>>();
		}

		/** Returns pointer to the specified value type.
		 * @tparam P Pointer to the value type to retrieve from the Json object. Must be one of the value
		 * types (`bool`, `int_type`, `uint_type`, `float_type`, `string_type`, `table_type`, `array_type`).
		 * @return Pointer of the value of the Json object, or `nullptr` if the Json object does not contain the specified type.
		 * @example
		 * @code{cpp}
		 * auto *bool_value = json.get<bool *>();
		 * @endcode */
		template<typename P>
		[[nodiscard]] P get() noexcept requires(std::is_pointer_v<P> && is_value_type<std::remove_cv_t<std::remove_pointer_t<P>>>)
		{
			return get_ptr_impl<std::remove_pointer_t<P>>();
		}
		/** @copydoc get */
		template<typename P>
		[[nodiscard]] auto get() const noexcept -> std::add_const_t<std::remove_pointer_t<P>> *
			requires(std::is_pointer_v<P> && is_value_type<std::remove_cv_t<std::remove_pointer_t<P>>>)
		{
			return get_ptr_impl<std::remove_cv_t<std::remove_pointer_t<P>>>();
		}
		// clang-format on

		// clang-format off
		constexpr void swap(basic_json_object &other)
			noexcept(std::is_nothrow_move_constructible_v<string_type> &&
					 std::is_nothrow_move_constructible_v<table_type> &&
					 std::is_nothrow_move_constructible_v<array_type> &&
					 std::is_nothrow_swappable_v<string_type> &&
					 std::is_nothrow_swappable_v<table_type> &&
					 std::is_nothrow_swappable_v<array_type>)
		{
			if (type() != other.m_type)
				move_swap(*this, other);
			else
			{
				using std::swap;
				switch (m_type)
				{
					case json_type::STRING: swap(m_string, other.m_string); break;
					case json_type::TABLE: swap(m_table, other.m_table); break;
					case json_type::ARRAY: swap(m_array, other.m_array); break;
					default: swap(m_literal_bytes, other.m_literal_bytes); break;
				}
			}
		}
		friend constexpr void swap(basic_json_object &a, basic_json_object &b) noexcept(std::is_nothrow_swappable_v<basic_json_object>)
		{
			a.swap(b);
		}
		// clang-format on

	private:
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
		void copy_init(const basic_json_object &other)
		{
			switch (other.m_type)
			{
				case json_type::TABLE: std::construct_at(&m_table, other.m_table); break;
				case json_type::ARRAY: std::construct_at(&m_array, other.m_array); break;
				case json_type::STRING: std::construct_at(&m_string, other.m_string); break;
				default: m_literal_bytes = other.m_literal_bytes; break;
			}
		}
		void copy_assign(const basic_json_object &other)
		{
			if (type() != other.m_type)
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
					default: m_literal_bytes = other.m_literal_bytes; break;
				}
		}
		void move_init(basic_json_object &other)
		{
			switch (other.m_type)
			{
				case json_type::TABLE: std::construct_at(&m_table, std::move(other.m_table)); break;
				case json_type::ARRAY: std::construct_at(&m_array, std::move(other.m_array)); break;
				case json_type::STRING: std::construct_at(&m_string, std::move(other.m_string)); break;
				default: m_literal_bytes = other.m_literal_bytes; break;
			}
		}
		void move_assign(basic_json_object &other)
		{
			if (type() != other.m_type)
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
					default: m_literal_bytes = other.m_literal_bytes; break;
				}
		}

		void init_array(initializer_list il, const array_allocator &alloc)
		{
			/* Reserve & insert elements. */
			std::construct_at(&m_array, il.size(), alloc);
			for (auto &ptr : il) m_array.emplace_back(std::move(ptr).extract());

			m_type = json_type::ARRAY;
		}
		void init_table(initializer_list il, const table_allocator &alloc)
		{
			if (const auto err = init_table(std::nothrow, il, alloc); !err) [[unlikely]]
				throw archive_error(err.error(), "Expected a sequence of key-value pairs");
		}
		expected<void, std::error_code> init_table(std::nothrow_t, initializer_list il, const table_allocator &alloc)
		{
			/* Assert that the initializer list consists of key-value pairs */
			constexpr auto pred = [](const owned_ptr<basic_json_object> &ptr)
			{
				if (ptr->is_array()) [[likely]]
					return ptr->m_array.size() == 2 && ptr->m_array[0].is_string();
				return false;
			};
			if (!std::all_of(il.begin(), il.end(), pred)) [[unlikely]]
				return unexpected{make_error_code(archive_errc::INVALID_DATA)};

			/* Reserve & insert elements. */
			std::construct_at(&m_table, il.size(), alloc);
			for (auto &ptr : il)
			{
				auto entry = std::move(ptr).extract();
				m_table.emplace(std::move(entry.m_array[0].m_string), std::move(entry.m_array[1]));
			}
			m_type = json_type::TABLE;
			return {};
		}

		bool as_type_impl(json_type type)
		{
			if (m_type != type)
			{
				destroy_impl();
				m_type = type;
				return true;
			}
			return false;
		}
		template<typename F>
		bool as_type_impl(json_type type, F &&factory)
		{
			if (m_type != type)
			{
				destroy_impl();

				/* Exception guarantee. */
				m_type = json_type::NULL_VALUE;
				factory();
				m_type = type;

				return true;
			}
			return false;
		}

		template<std::same_as<std::nullptr_t> U, typename... Args>
		void as_impl(Args &&...)
		{
			as_type_impl(json_type::NULL_VALUE);
		}
		template<std::same_as<bool> U, typename... Args>
		void as_impl(Args &&...args)
		{
			as_type_impl(json_type::BOOL);
			m_bool = bool{std::forward<Args>(args)...};
		}
		template<std::same_as<int_type> U, typename... Args>
		void as_impl(Args &&...args)
		{
			as_type_impl(json_type::INT);
			m_int = int_type{std::forward<Args>(args)...};
		}
		template<std::same_as<uint_type> U, typename... Args>
		void as_impl(Args &&...args)
		{
			as_type_impl(json_type::UINT);
			m_uint = uint_type{std::forward<Args>(args)...};
		}
		template<std::same_as<float_type> U, typename... Args>
		void as_impl(Args &&...args)
		{
			as_type_impl(json_type::FLOAT);
			m_float = float_type{std::forward<Args>(args)...};
		}
		template<std::same_as<string_type> U, typename... Args>
		void as_impl(Args &&...args)
		{
			if (!as_type_impl(json_type::STRING, [&]() { std::construct_at(&m_string, std::forward<Args>(args)...); }))
				m_string = string_type{std::forward<Args>(args)...};
		}
		template<std::same_as<table_type> U, typename... Args>
		void as_impl(Args &&...args)
		{
			if (!as_type_impl(json_type::TABLE, [&]() { std::construct_at(&m_table, std::forward<Args>(args)...); }))
				m_table = table_type{std::forward<Args>(args)...};
		}
		template<std::same_as<array_type> U, typename... Args>
		void as_impl(Args &&...args)
		{
			if (!as_type_impl(json_type::ARRAY, [&]() { std::construct_at(&m_array, std::forward<Args>(args)...); }))
				m_array = array_type{std::forward<Args>(args)...};
		}

		void assert_type(json_type type) const
		{
			if (m_type != type) [[unlikely]]
				detail::invalid_json_type(type, m_type);
		}

		template<std::same_as<bool> U>
		[[nodiscard]] auto get_impl() const
		{
			assert_type(json_type::BOOL);
			return m_bool;
		}
		template<std::same_as<bool> U>
		[[nodiscard]] auto get_impl(std::nothrow_t) const -> expected<U, std::error_code>
		{
			if (m_type != json_type::BOOL) [[unlikely]]
				return unexpected{make_error_code(archive_errc::INVALID_TYPE)};
			return m_bool;
		}
		template<std::same_as<string_type> U>
		[[nodiscard]] auto get_impl() const
		{
			assert_type(json_type::STRING);
			return m_string;
		}
		template<std::same_as<string_type> U>
		[[nodiscard]] auto get_impl(std::nothrow_t) const -> expected<U, std::error_code>
		{
			if (m_type != json_type::STRING) [[unlikely]]
				return unexpected{make_error_code(archive_errc::INVALID_TYPE)};
			return m_string;
		}
		template<std::same_as<table_type> U>
		[[nodiscard]] auto get_impl() const
		{
			assert_type(json_type::TABLE);
			return m_table;
		}
		template<std::same_as<table_type> U>
		[[nodiscard]] auto get_impl(std::nothrow_t) const -> expected<U, std::error_code>
		{
			if (m_type != json_type::TABLE) [[unlikely]]
				return unexpected{make_error_code(archive_errc::INVALID_TYPE)};
			return m_table;
		}
		template<std::same_as<array_type> U>
		[[nodiscard]] auto get_impl() const
		{
			assert_type(json_type::ARRAY);
			return m_array;
		}
		template<std::same_as<array_type> U>
		[[nodiscard]] auto get_impl(std::nothrow_t) const -> expected<U, std::error_code>
		{
			if (m_type != json_type::ARRAY) [[unlikely]]
				return unexpected{make_error_code(archive_errc::INVALID_TYPE)};
			return m_array;
		}

		// clang-format off
		template<typename U>
		[[nodiscard]] auto get_impl() const requires std::is_arithmetic_v<U>
		{
			switch (m_type)
			{
				case json_type::INT: return static_cast<U>(m_int);
				case json_type::UINT: return static_cast<U>(m_uint);
				case json_type::FLOAT: return static_cast<U>(m_float);
				default: detail::invalid_json_type(json_type::NUMBER_FLAG, m_type);
			}
		}
		template<typename U>
		[[nodiscard]] auto get_impl() const -> expected<U, std::error_code> requires std::is_arithmetic_v<U>
		{
			switch (m_type)
			{
				case json_type::INT: return static_cast<U>(m_int);
				case json_type::UINT: return static_cast<U>(m_uint);
				case json_type::FLOAT: return static_cast<U>(m_float);
				default: return unexpected{make_error_code(archive_errc::INVALID_TYPE)};
			}
		}
		// clang-format on

		template<std::same_as<bool> U>
		[[nodiscard]] auto *get_ptr_impl() noexcept
		{
			return is_bool() ? &m_bool : nullptr;
		}
		template<std::same_as<int_type> U>
		[[nodiscard]] auto *get_ptr_impl() noexcept
		{
			return is_int() ? &m_int : nullptr;
		}
		template<std::same_as<uint_type> U>
		[[nodiscard]] auto *get_ptr_impl() noexcept
		{
			return is_uint() ? &m_uint : nullptr;
		}
		template<std::same_as<float_type> U>
		[[nodiscard]] auto *get_ptr_impl() noexcept
		{
			return is_float() ? &m_float : nullptr;
		}
		template<std::same_as<string_type> U>
		[[nodiscard]] auto *get_ptr_impl() noexcept
		{
			return is_string() ? &m_string : nullptr;
		}
		template<std::same_as<table_type> U>
		[[nodiscard]] auto *get_ptr_impl() noexcept
		{
			return is_table() ? &m_table : nullptr;
		}
		template<std::same_as<array_type> U>
		[[nodiscard]] auto *get_ptr_impl() noexcept
		{
			return is_array() ? &m_array : nullptr;
		}

		template<std::same_as<bool> U>
		[[nodiscard]] auto *get_ptr_impl() const noexcept
		{
			return is_bool() ? &m_bool : nullptr;
		}
		template<std::same_as<int_type> U>
		[[nodiscard]] auto *get_ptr_impl() const noexcept
		{
			return is_int() ? &m_int : nullptr;
		}
		template<std::same_as<uint_type> U>
		[[nodiscard]] auto *get_ptr_impl() const noexcept
		{
			return is_uint() ? &m_uint : nullptr;
		}
		template<std::same_as<float_type> U>
		[[nodiscard]] auto *get_ptr_impl() const noexcept
		{
			return is_float() ? &m_float : nullptr;
		}
		template<std::same_as<string_type> U>
		[[nodiscard]] auto *get_ptr_impl() const noexcept
		{
			return is_string() ? &m_string : nullptr;
		}
		template<std::same_as<table_type> U>
		[[nodiscard]] auto *get_ptr_impl() const noexcept
		{
			return is_table() ? &m_table : nullptr;
		}
		template<std::same_as<array_type> U>
		[[nodiscard]] auto *get_ptr_impl() const noexcept
		{
			return is_array() ? &m_array : nullptr;
		}

		template<std::same_as<bool> U>
		[[nodiscard]] auto &get_ref_impl()
		{
			assert_type(json_type::BOOL);
			return m_bool;
		}
		template<std::same_as<int_type> U>
		[[nodiscard]] auto &get_ref_impl()
		{
			assert_type(json_type::INT);
			return m_int;
		}
		template<std::same_as<uint_type> U>
		[[nodiscard]] auto &get_ref_impl()
		{
			assert_type(json_type::UINT);
			return m_uint;
		}
		template<std::same_as<float_type> U>
		[[nodiscard]] auto &get_ref_impl()
		{
			assert_type(json_type::FLOAT);
			return m_float;
		}
		template<std::same_as<string_type> U>
		[[nodiscard]] auto &get_ref_impl()
		{
			assert_type(json_type::STRING);
			return m_string;
		}
		template<std::same_as<table_type> U>
		[[nodiscard]] auto &get_ref_impl()
		{
			assert_type(json_type::TABLE);
			return m_table;
		}
		template<std::same_as<array_type> U>
		[[nodiscard]] auto &get_ref_impl()
		{
			assert_type(json_type::ARRAY);
			return m_array;
		}

		template<std::same_as<bool> U>
		[[nodiscard]] auto &get_ref_impl() const
		{
			assert_type(json_type::BOOL);
			return m_bool;
		}
		template<std::same_as<int_type> U>
		[[nodiscard]] auto &get_ref_impl() const
		{
			assert_type(json_type::INT);
			return m_int;
		}
		template<std::same_as<uint_type> U>
		[[nodiscard]] auto &get_ref_impl() const
		{
			assert_type(json_type::UINT);
			return m_uint;
		}
		template<std::same_as<float_type> U>
		[[nodiscard]] auto &get_ref_impl() const
		{
			assert_type(json_type::FLOAT);
			return m_float;
		}
		template<std::same_as<string_type> U>
		[[nodiscard]] auto &get_ref_impl() const
		{
			assert_type(json_type::STRING);
			return m_string;
		}
		template<std::same_as<table_type> U>
		[[nodiscard]] auto &get_ref_impl() const
		{
			assert_type(json_type::TABLE);
			return m_table;
		}
		template<std::same_as<array_type> U>
		[[nodiscard]] auto &get_ref_impl() const
		{
			assert_type(json_type::ARRAY);
			return m_array;
		}

		union
		{
			aligned_storage<std::max(sizeof(int_type), sizeof(float_type)), std::max(alignof(int_type), alignof(float_type))>
				m_literal_bytes = {};

			bool m_bool;
			int_type m_int;
			uint_type m_uint;
			float_type m_float;

			string_type m_string;
			array_type m_array;
			table_type m_table;
		};
		json_type m_type = json_type::NULL_VALUE;
	};

	extern template class SEK_API_IMPORT basic_json_object<char>;

	/** @brief `basic_json_object` alias for `char` character type. */
	using json_object = basic_json_object<char>;
}	 // namespace sek::serialization