/*
 * Created by switchblade on 07/07/22
 */

#pragma once

#include <vector>

#include "../../../detail/owned_ptr.hpp"
#include "../../../ordered_map.hpp"
#include "../manipulators.hpp"
#include "../tuples.hpp"
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
		typedef typename table_type::key_type key_type;

		typedef std::vector<basic_json_object, Alloc<basic_json_object>> array_type;
		typedef typename array_type::allocator_type array_allocator;

		typedef std::initializer_list<owned_ptr<basic_json_object>> initializer_list;

	private:
		[[nodiscard]] constexpr static bool is_table_list(const initializer_list &il) noexcept
		{
			constexpr auto pred = [](const owned_ptr<basic_json_object> &ptr)
			{
				if (ptr->is_array()) [[likely]]
					return ptr->m_array.size() == 2 && ptr->m_array[0].is_string();
				return false;
			};
			return std::all_of(il.begin(), il.end(), pred);
		}

		template<std::same_as<std::nullptr_t> U>
		[[nodiscard]] static constexpr json_type select_type() noexcept
		{
			return json_type::NULL_VALUE;
		}
		template<std::same_as<bool> U>
		[[nodiscard]] static constexpr json_type select_type() noexcept
		{
			return json_type::BOOL;
		}
		template<std::signed_integral U>
		[[nodiscard]] static constexpr json_type select_type() noexcept
		{
			return json_type::INT;
		}
		template<std::unsigned_integral U>
		[[nodiscard]] static constexpr json_type select_type() noexcept
		{
			return json_type::UINT;
		}
		template<std::floating_point U>
		[[nodiscard]] static constexpr json_type select_type() noexcept
		{
			return json_type::FLOAT;
		}
		template<std::same_as<string_type> U>
		[[nodiscard]] static constexpr json_type select_type() noexcept
		{
			return json_type::STRING;
		}
		template<std::same_as<table_type> U>
		[[nodiscard]] static constexpr json_type select_type() noexcept
		{
			return json_type::TABLE;
		}
		template<std::same_as<array_type> U>
		[[nodiscard]] static constexpr json_type select_type() noexcept
		{
			return json_type::ARRAY;
		}

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

		template<typename U, typename V = std::remove_cvref_t<U>>
		constexpr static bool is_compatible_type = is_value_type<V> || std::is_arithmetic_v<V>;

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
		constexpr static bool is_serializable = serializable<std::remove_cvref_t<U>, write_frame, Args...>;
		template<typename U, typename... Args>
		constexpr static bool is_deserializable = deserializable<std::remove_cvref_t<U>, write_frame, Args...>;
		template<typename U, typename... Args>
		constexpr static bool is_in_place_deserializable = in_place_deserializable<std::remove_cvref_t<U>, write_frame, Args...>;

		template<typename K>
		constexpr static bool compatible_key = requires(table_type t, K &&k) { t.at(std::forward<K>(k)); };
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

			/** Seeks the current container to the specified iterator.
			 * @param pos New read position within the current container. */
			void seek(const_iterator pos) noexcept { m_pos = pos; }

			/** Seeks the current container to the specified key.
			 * @param key Key of the target Json object to seek to.
			 * @throw archive_error If the target Json object of the frame is not a table.
			 * @note If the key is not present within the container, seeks to the end. */
			void seek(string_type &&key)
			{
				m_pos = seek_impl([&key]() { return std::move(key); });
			}
			/** @copydoc seek */
			template<typename S>
			void seek(const S &key)
			{
				m_pos = seek_impl([&key]() { return key; });
			}
			/** Seeks the current container to the specified key.
			 * @param key Key of the target Json object to seek to.
			 * @return `void`, or `archive_errc::INVALID_TYPE` if the target Json object is not a table.
			 * @note If the key is not present within the container, seeks to the end. */
			expected<void, std::error_code> seek(std::nothrow_t, string_type &&key)
			{
				const auto result = seek_impl(std::nothrow, [&key]() { return std::move(key); });
				if (!result) [[unlikely]]
					return expected<void, std::error_code>{result};
				m_pos = *result;
				return {};
			}
			/** @copydoc seek */
			template<typename S>
			expected<void, std::error_code> seek(std::nothrow_t, const S &key)
			{
				const auto result = seek_impl(std::nothrow, [&key]() { return key; });
				if (!result) [[unlikely]]
					return expected<void, std::error_code>{result};
				m_pos = *result;
				return {};
			}

			/** Returns the next Json object of the current container and advances read position.
			 * @return Reference to the next Json object.
			 * @throw archive_error If the frame is at the end of the container.
			 * @note Read position is advanced only on success. */
			[[nodiscard]] const value_type &next()
			{
				if (is_end()) [[unlikely]]
					throw archive_error{make_error_code(archive_errc::UNEXPECTED_END)};
				return *(m_pos++);
			}

			/** Deserializes the an instance of type `U` from the next Json object in the current container.
			 * @param value Object to deserialize.
			 * @param args Arguments passed to the deserialization function.
			 * @note Read position is advanced only on success. */
			template<typename U, typename... Args>
			void read(U &value, Args &&...args)
			{
				if (is_end()) [[unlikely]]
					throw archive_error{make_error_code(archive_errc::UNEXPECTED_END)};

				m_pos->read(value, std::forward<Args>(args)...);
				m_pos++; /* Exception guarantee. */
			}
			/** @copydoc read
			 * @return `void`, or an error code on deserialization errors. */
			template<typename U, typename... Args>
			expected<void, std::error_code> read(std::nothrow_t, U &value, Args &&...args)
			{
				if (is_end()) [[unlikely]]
					return unexpected{make_error_code(archive_errc::UNEXPECTED_END)};

				auto result = m_pos->read(std::nothrow, value, std::forward<Args>(args)...);
				m_pos++; /* Exception guarantee. */
				return result;
			}
			/** Deserializes the an instance of type `U` from the next Json object in the current container.
			 * @param value Object to deserialize.
			 * @return Reference to this frame. */
			template<typename U>
			read_frame &operator>>(U &value)
			{
				read(value);
				return *this;
			}

			/** @brief Deserializes the an instance of type `U` from the next Json object in-place in the current container.
			 * @param args Arguments passed to the deserialization function.
			 * @return Instance of `U` deserialized from the Json object.
			 * @note Read position is advanced only on success. */
			template<typename U, typename... Args>
			[[nodiscard]] U read(std::in_place_type_t<U>, Args &&...args)
			{
				if (is_end()) [[unlikely]]
					throw archive_error{make_error_code(archive_errc::UNEXPECTED_END)};

				auto result = m_pos->read(std::in_place_type<U>, std::forward<Args>(args)...);
				m_pos++; /* Exception guarantee. */
				return result;
			}
			/** @copybrief read
			 * @param args Arguments passed to the deserialization function.
			 * @return Instance of `U` deserialized from the Json object, or an error code on deserialization errors.
			 * @note If the requested type is a non-`bool` arithmetic (number) type, appropriate conversions will be preformed.
			 * @note Read position is advanced only on success. */
			template<typename U, typename... Args>
			[[nodiscard]] expected<U, std::error_code> read(std::nothrow_t, std::in_place_type_t<U>, Args &&...args)
			{
				if (is_end()) [[unlikely]]
					return unexpected{make_error_code(archive_errc::UNEXPECTED_END)};

				auto result = m_pos->read(std::nothrow, std::in_place_type<U>, std::forward<Args>(args)...);
				m_pos++; /* Exception guarantee. */
				return result;
			}

			/** @brief Deserializes the an instance of type `U` from the next Json object in
			 * the current container using the provided key.
			 * @param value `keyed_entry_t` instance containing the object to deserialize and it's key.
			 * @param args Arguments passed to the deserialization function.
			 * @throw archive_error If the underlying Json object is not a table or the requested key does not exist.
			 * @note Read position is advanced only on success. */
			template<typename U, typename... Args>
			void read(keyed_entry_t<C, U> value, Args &&...args)
			{
				const auto iter = seek_impl([&value]() { return value.key; });
				if (iter.m_table == m_target.m_table.end()) [[unlikely]]
					throw archive_error{make_error_code(archive_errc::INVALID_DATA)};

				iter->read(value, std::forward<Args>(args)...);
				m_pos = iter; /* Exception guarantee. */
			}
			/** @copybrief read
			 * @param value `keyed_entry_t` instance containing the object to deserialize and it's key.
			 * @param args Arguments passed to the deserialization function.
			 * @return `void` on success, `archive_errc::INVALID_TYPE` if the underlying Json object is not a table
			 * or `archive_errc::INVALID_DATA` if the requested key does not exist.
			 * @note Read position is advanced only on success. */
			template<typename U, typename... Args>
			expected<void, std::error_code> read(std::nothrow_t, keyed_entry_t<C, U> value, Args &&...args)
			{
				const auto iter = seek_impl(std::nothrow, [&value]() { return value.key; });
				if (!iter) [[unlikely]]
					return expected<void, std::error_code>{iter};
				else if (iter->m_table == m_target.m_table.end()) [[unlikely]]
					return unexpected{archive_errc::INVALID_DATA};

				iter->read(value, std::forward<Args>(args)...);
				m_pos = iter; /* Exception guarantee. */
			}
			/** @copybrief read
			 * @param value `keyed_entry_t` instance containing the object to deserialize and it's key.
			 * @param args Arguments passed to the deserialization function.
			 * @return Reference to this frame.
			 * @throw archive_error If the underlying Json object is not a table or the requested key does not exist.
			 * @note Read position is advanced only on success. */
			template<typename U>
			read_frame &operator>>(keyed_entry_t<C, U> value)
			{
				read(value);
				return *this;
			}

			/** Reads the size of the current container.
			 * @param size `container_size_t` instance receiving the container size. */
			template<typename U>
			void read(container_size_t<U> size)
			{
				size.value = static_cast<std::remove_cvref_t<U>>(m_target.size());
			}
			/** @copydoc read
			 * @return Reference to this frame. */
			template<typename U>
			read_frame &operator>>(container_size_t<U> size)
			{
				read(size);
				return *this;
			}
			/** @copydoc read
			 * @return `void`, or an error code on deserialization errors. */
			template<typename U>
			expected<void, std::error_code> read(std::nothrow_t, container_size_t<U> size)
			{
				read(size);
				return {};
			}

		private:
			expected<const_iterator, std::error_code> seek_impl(std::nothrow_t, auto &&key_factory)
			{
				if (!m_target.is_table()) [[unlikely]]
					return unexpected{make_error_code(archive_errc::INVALID_TYPE)};

				return const_iterator{m_target.m_table.find(key_factory())};
			}
			const_iterator seek_impl(auto &&key_factory)
			{
				return const_iterator{m_target.get<table_type &>().find(key_factory())};
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
			constexpr write_frame(value_type &target) : m_target(target)
			{
				if (!m_target.is_table()) [[unlikely]]
					m_target.as<table_type>();
				else
					m_target.m_table.clear();
			}

			/** Returns reference to the target Json object of this frame. */
			[[nodiscard]] constexpr value_type &target() noexcept { return m_target; }
			/** @copydoc target */
			[[nodiscard]] constexpr const value_type &target() const noexcept { return m_target; }

			/** @brief Inserts a new Json object into the underlying container.
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

			/** Inserts a new Json object into the underlying container and serializes an instance of type `U` to it.
			 * @param value Object to serialize.
			 * @param args Arguments passed to the serialization function.
			 * @note If an error occurs during serialization, the underlying Json object is left in a defined state. */
			template<typename U, typename... Args>
			void write(U &&value, Args &&...args)
			{
				next().write(std::forward<U>(value), std::forward<Args>(args)...);
			}
			/** @copydoc write
			 * @return `void`, or an error code on serialization errors. */
			template<typename U, typename... Args>
			expected<void, std::error_code> write(std::nothrow_t, U &&value, Args &&...args)
			{
				return next().write(std::nothrow, std::forward<U>(value), std::forward<Args>(args)...);
			}
			/** Inserts a new Json object into the underlying container and serializes an instance of type `U` to it.
			 * @param value Object to serialize.
			 * @return Reference to this frame.
			 * @note If an error occurs during serialization, the underlying Json object is left in a defined state. */
			template<typename U>
			write_frame &operator<<(U &&value)
			{
				write(std::forward<U>(value));
				return *this;
			}

			/** Inserts a new Json object into the underlying container using the
			 * provided key hint and serializes an instance of type `U` to it.
			 * @param value Keyed entry manipulator containing the object to serialize and a key hint.
			 * @param args Arguments passed to the serialization function.
			 * @note If an error occurs during serialization, the underlying Json object is left in a defined state. */
			template<typename U, typename... Args>
			void write(keyed_entry_t<C, U> value, Args &&...args)
			{
				next(value.key).write(std::forward<U>(value.value), std::forward<Args>(args)...);
			}
			/** @copydoc write
			 * @return `void`, or an error code on serialization errors. */
			template<typename U, typename... Args>
			expected<void, std::error_code> write(std::nothrow_t, keyed_entry_t<C, U> value, Args &&...args)
			{
				return next(value.key).write(std::nothrow, std::forward<U>(value.value), std::forward<Args>(args)...);
			}
			/** Inserts a new Json object into the underlying container using the
			 * provided key hint and serializes an instance of type `U` to it.
			 * @param value Keyed entry manipulator containing the object to serialize and a key hint.
			 * @return Reference to this frame.
			 * @note If an error occurs during serialization, the underlying Json object is left in a defined state. */
			template<typename U>
			write_frame &operator<<(keyed_entry_t<C, U> value)
			{
				write(value);
				return *this;
			}

			/** Switches the archive to array mode. If the underlying container is a non-empty
			 * table, it's elements are copied to the array as key-value pairs. */
			write_frame &operator<<(array_mode_t)
			{
				write(array_mode());
				return *this;
			}
			/** @copydoc operator<<
			 * @param alloc Allocator used for the array. */
			void write(array_mode_t, const array_allocator &alloc = array_allocator{})
			{
				switch (m_target.type())
				{
					case json_type::TABLE: to_array_impl(alloc);
					case json_type::ARRAY: break;
					default: detail::invalid_json_type(json_type::CONTAINER_FLAG, m_target.type());
				}
			}
			/** @copydoc write
			 * @return `void`, or an error code on serialization errors. */
			expected<void, std::error_code> write(std::nothrow_t, array_mode_t, const array_allocator &alloc = array_allocator{})
			{
				switch (m_target.type())
				{
					case json_type::TABLE: to_array_impl(alloc);
					case json_type::ARRAY: return {};
					default: return unexpected{make_error_code(archive_errc::INVALID_TYPE)};
				}
			}

			/** Uses the provided hint to reserve size of the underlying container. */
			template<typename U>
			write_frame &operator<<(container_size_t<U> size)
			{
				write(size);
				return *this;
			}
			/** @copydoc operator<< */
			template<typename U>
			void write(container_size_t<U> size)
			{
				m_target.reserve(static_cast<size_type>(size.value));
			}
			/** @copydoc write
			 * @return `void`, or an error code on serialization errors. */
			template<typename U>
			expected<void, std::error_code> write(std::nothrow_t, container_size_t<U> size)
			{
				return m_target.reserve(std::nothrow, static_cast<size_type>(size.value));
			}

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

			void to_array_impl(const array_allocator &alloc)
			{
				if (auto &table = m_target.m_table; table.empty()) [[likely]]
					m_target.as<array_type>(table.size(), alloc);
				else
				{
					/* Create a new array and copy elements. */
					array_type array{table.size(), alloc};
					for (decltype(auto) pair : table)
					{
						auto &pair_obj = array.emplace_back(std::in_place_type<array_type>, 2u, alloc);
						auto *pair_array = pair_obj.template get<array_type *>();
						pair_array->emplace_back(std::move(pair.first));
						pair_array->emplace_back(std::move(pair.second));
					}

					/* Convert to the new array. */
					m_target.as<array_type>(std::move(array));
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
			if (const auto err = result->init_table(std::nothrow, il, alloc); !err) [[unlikely]]
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

		// clang-format off
		/** @brief Initializes a boolean Json object.
		 * @param value Value of the integer. */
		basic_json_object(bool value) : basic_json_object(std::in_place_type<bool>, value) {}
		/** @copydoc basic_json_object */
		template<typename B>
		basic_json_object(std::in_place_type_t<bool>, B &&value) requires std::constructible_from<bool, B>
			: m_bool(value), m_type(json_type::BOOL) {}

		/** @brief Initializes a signed integer Json object.
		 * @param value Value of the integer. */
		template<typename I>
		basic_json_object(I &&value) requires std::signed_integral<std::remove_cvref_t<I>>
			: basic_json_object(std::in_place_type<int_type>, value) {}
		/** @copydoc basic_json_object */
		template<typename I>
		basic_json_object(std::in_place_type_t<int_type>, I &&value) requires std::constructible_from<int_type, I>
			: m_int(value), m_type(json_type::INT) {}

		/** @brief Initializes an unsigned integer Json object.
		 * @param value Value of the integer. */
		template<typename U>
		basic_json_object(U &&value) requires std::unsigned_integral<std::remove_cvref_t<U>>
			: basic_json_object(std::in_place_type<uint_type>, std::forward<U>(value)) {}
		/** @copydoc basic_json_object */
		template<typename U>
		basic_json_object(std::in_place_type_t<uint_type>, U &&value) requires std::constructible_from<uint_type, U>
			: m_uint(value), m_type(json_type::UINT) {}

		/** @brief Initializes a floating-point number Json object.
		 * @param value Value of the floating-point number. */
		template<typename F>
		basic_json_object(F &&value) requires std::floating_point<std::remove_cvref_t<F>>
			: basic_json_object(std::in_place_type<float_type>, std::forward<F>(value)) {}
		/** @copydoc basic_json_object */
		template<typename F>
		basic_json_object(std::in_place_type_t<float_type>, F &&value) requires std::constructible_from<float_type, F>
			: m_float(value), m_type(json_type::FLOAT) {}

		/** @brief Initializes a string Json object.
		 * @param value Value of the string. */
		basic_json_object(const string_type &value) : basic_json_object(std::in_place_type<string_type>, value) {}
		/** @copydoc basic_json_object */
		basic_json_object(string_type &&value) : basic_json_object(std::in_place_type<string_type>, std::move(value)) {}
		/** @copydoc basic_json_object */
		template<typename S>
		basic_json_object(const S &value) requires(std::constructible_from<string_type, S> && !std::same_as<S, string_type>)
			: basic_json_object(std::in_place_type<string_type>, value) {}
		/** @copybrief basic_json_object
		 * @param args Arguments passed to string's constructor. */
		template<typename... Args>
		basic_json_object(std::in_place_type_t<string_type>, Args &&...args) requires std::constructible_from<string_type, Args...>
			: m_string(std::forward<Args>(args)...), m_type(json_type::STRING) {}

		/** @brief Initializes a table Json object.
		 * @param value Value of the table. */
		basic_json_object(const table_type &value) : basic_json_object(std::in_place_type<table_type>, value) {}
		/** @copydoc basic_json_object */
		basic_json_object(table_type &&value) : basic_json_object(std::in_place_type<table_type>, std::move(value)) {}
		/** @copybrief basic_json_object
		 * @param args Arguments passed to table's constructor. */
		template<typename... Args>
		basic_json_object(std::in_place_type_t<table_type>, Args &&...args) requires std::constructible_from<table_type, Args...>
		    : m_table(std::forward<Args>(args)...), m_type(json_type::TABLE) {}

		/** @brief Initializes an array Json object.
		 * @param value Value of the table. */
		basic_json_object(const array_type &value) : basic_json_object(std::in_place_type<array_type>, value) {}
		/** @copydoc basic_json_object */
		basic_json_object(array_type &&value) : basic_json_object(std::in_place_type<array_type>, std::move(value)) {}
		/** @copybrief basic_json_object
		 * @param args Arguments passed to array's constructor. */
		template<typename... Args>
		basic_json_object(std::in_place_type_t<array_type>, Args &&...args) requires std::constructible_from<array_type, Args...>
		    : m_array(std::forward<Args>(args)...), m_type(json_type::ARRAY) {}
		// clang-format on

		/** Initializes a container (table or array) Json object from an initializer list.
		 *
		 * If the initializer list contains key-value pairs, where every json object is an array of at least 2 elements
		 * with the first one being a string, the Json object is initialized as a table. Otherwise, it is initialized
		 * as an array.
		 *
		 * To initialize create an explicit table or array Json object, use `make_table` and `make_array`.
		 *
		 * @param il Initializer list containing elements of the container. */
		basic_json_object(initializer_list il)
		{
			if (is_table_list(il))
				init_table_impl(il, table_allocator{});
			else
				init_array(il, array_allocator{});
		}

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

		/** Returns reference to an element at the specified index within the underlying array.
		 * @return Reference to the element located at the index.
		 * @throw archive_error If the Json object is not an array. */
		[[nodiscard]] reference operator[](size_type i) { return get<array_type &>()[i]; }
		/** @copydoc operator[] */
		[[nodiscard]] const_reference operator[](size_type i) const { return get<array_type &>()[i]; }
		/** @copydoc operator[]
		 * @throw std::out_of_range If index is out of range of the array. */
		[[nodiscard]] reference at(size_type i) { return get<array_type &>().at(i); }
		/** @copydoc at */
		[[nodiscard]] const_reference at(size_type i) const { return get<array_type &>().at(i); }

		/** Returns reference to an element at the specified key within the underlying table.
		 * @return Reference to the element located at the specified key.
		 * @throw archive_error If the Json object is not a table.
		 * @throw std::out_of_range If the key is not present within the table. */
		[[nodiscard]] reference at(const key_type &key) { return get<table_type &>().at(key); }
		/** @copydoc at */
		[[nodiscard]] const_reference at(const key_type &key) const { return get<table_type &>().at(key); }
		/** @copydoc at */
		[[nodiscard]] const_reference operator[](const key_type &key) const { return at(key); }

		// clang-format off
		/** @copydoc at */
		template<typename K>
		[[nodiscard]] reference at(K &&key) requires(!std::same_as<key_type, std::decay_t<K>> && compatible_key<K>)
		{
			return get<table_type &>().at(std::forward<K>(key));
		}
		/** @copydoc at */
		template<typename K>
		[[nodiscard]] const_reference at(K &&key) const requires(!std::same_as<key_type, std::decay_t<K>> && compatible_key<K>)
		{
			return get<table_type &>().at(std::forward<K>(key));
		}
		/** @copydoc at */
		template<typename K>
		[[nodiscard]] const_reference operator[](K &&key) const requires(!std::same_as<key_type, std::decay_t<K>> && compatible_key<K>)
		{
			return at(std::forward<K>(key));
		}
		// clang-format on

		/** Returns reference to an element at the specified key within the underlying table.
		 * If the key is not present within the table, inserts a new element.
		 * @return Reference to the element located at the specified key.
		 * @throw archive_error If the Json object is not a table. */
		[[nodiscard]] reference operator[](const key_type &key) { return get<table_type &>()[key]; }
		/** @copydoc operator[] */
		[[nodiscard]] reference operator[](key_type &&key) { return get<table_type &>()[std::move(key)]; }
		// clang-format off
		/** @copydoc operator[] */
		template<typename K>
		[[nodiscard]] reference operator[](K &&key) requires(!std::same_as<key_type, std::decay_t<K>> && compatible_key<K>)
		{
			return get<table_type &>()[std::forward<K>(key)];
		}
		// clang-format on

		/** Reserves storage for at least `n` elements in the container.
		 * @throw archive_error If the Json object is not a container. */
		void reserve(size_type size)
		{
			switch (m_type)
			{
				case json_type::ARRAY: m_array.reserve(size); break;
				case json_type::TABLE: m_table.reserve(size); break;
				default: detail::invalid_json_type(json_type::CONTAINER_FLAG, m_type);
			}
		}
		/** Reserves storage for at least `n` elements in the container.
		 * @return `void`, or `archive_errc::INVALID_TYPE` if the Json object is not a container. */
		expected<void, std::error_code> reserve(std::nothrow_t, size_type size)
		{
			switch (m_type)
			{
				case json_type::ARRAY: m_array.reserve(size); break;
				case json_type::TABLE: m_table.reserve(size); break;
				default: unexpected{make_error_code(archive_errc::INVALID_TYPE)};
			}
			return {};
		}

		/** Erases an element of the container.
		 * @param which Iterator to the element to erase.
		 * @return Iterator to the next element of the container, or an end iterator.
		 * @throw archive_error If the Json object is not a container. */
		iterator erase(const_iterator which)
		{
			switch (m_type)
			{
				case json_type::ARRAY: return iterator{m_array.erase(which.m_array)};
				case json_type::TABLE: return iterator{m_table.erase(which.m_table)};
				default: detail::invalid_json_type(json_type::CONTAINER_FLAG, m_type);
			}
		}
		/** Erases an element of the container.
		 * @param which Iterator to the element to erase.
		 * @return Iterator to the next element of the container, an end iterator or
		 * `archive_errc::INVALID_TYPE` if the Json object is not a container. */
		expected<iterator, std::error_code> erase(std::nothrow_t, const_iterator which)
		{
			switch (m_type)
			{
				case json_type::ARRAY: return iterator{m_array.erase(which.m_array)};
				case json_type::TABLE: return iterator{m_table.erase(which.m_table)};
				default: unexpected{make_error_code(archive_errc::INVALID_TYPE)};
			}
		}

		// clang-format off
		/** Converts the Json object to the specified value type.
		 * @tparam U Value type to convert the Json object to. Must be one of the following value types: `std::nullptr_t`,
		 * `bool`, `int_type`, `uint_type`, `float_type`, `string_type`, `table_type`, `array_type`.
		 * @return Reference to this Json object.
		 * @note Previous value of the Json object will be overwritten.
		 * @note If an error occurs during initialization of the value type, Json object is left in a defined state. */
		template<typename U>
		basic_json_object &as(U &&value) requires is_value_type<std::remove_cvref_t<U>>
		{
			as_impl<U>(std::forward<U>(value));
			return *this;
		}
		/** @copydoc as
		 * @param args Arguments passed to the constructor of the value type. */
		template<typename U, typename... Args>
		basic_json_object &as(Args &&...args) requires is_value_type<U> && std::constructible_from<U, Args...>
		{
			as_impl<U>(std::forward<Args>(args)...);
			return *this;
		}
		/** Converts the Json object to a signed integer via converting to `int_type`.
		 * @return Reference to this Json object.
		 * @note Previous value of the Json object will be overwritten. */
		template<typename I>
		basic_json_object &as(I &&value) requires is_compatible_int<std::remove_cvref_t<I>>
		{
			return as<int_type>(static_cast<int_type>(std::forward<I>(value)));
		}
		/** Converts the Json object to an unsigned integer via converting to `uint_type`.
		 * @return Reference to this Json object.
		 * @note Previous value of the Json object will be overwritten. */
		template<typename I>
		basic_json_object &as(I &&value) requires is_compatible_uint<std::remove_cvref_t<I>>
		{
			return as<uint_type>(static_cast<uint_type>(std::forward<I>(value)));
		}
		/** Converts the Json object to a floating-point number via converting to `uint_type`.
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
		 * @tparam U Value type to retrieve from the Json object. Must either be one of the following value types:
		 * `bool`, `int_type`, `uint_type`, `float_type`, `string_type`, `table_type`, `array_type`, or a compatible arithmetic type.
		 * @return Copy of the requested value of the Json object.
		 * @note If the requested type is a non-`bool` arithmetic (number) type, appropriate conversions will be preformed.
		 * @throw archive_error If the Json object does not contain the specified type.
		 * @example
		 * @code{cpp}
		 * int32_t int32_value = json.get<int32_t>();
		 * int16_t int16_value = json.get<int16_t>();
		 * @endcode */
		template<typename U>
		[[nodiscard]] U get() const requires is_compatible_type<U> { return get_impl<U>(); }
		/** @copybrief get
		 * @tparam U Value type to retrieve from the Json object. Must either be one of the following value types:
		 * `bool`, `int_type`, `uint_type`, `float_type`, `string_type`, `table_type`, `array_type`, or a compatible arithmetic type.
		 * @return Copy of the requested value of the Json object, or `archive_errc::INVALID_TYPE`
		 * if the Json object does not contain the specified type.
		 * @note If the requested type is a non-`bool` arithmetic (number) type, appropriate conversions will be preformed.
		 * @example
		 * @code{cpp}
		 * expected<int32_t, std::error_code> int32_value = json.get<int32_t>(std::nothrow);
		 * expected<int16_t, std::error_code> int16_value = json.get<int16_t>(std::nothrow);
		 * @endcode */
		template<typename U>
		[[nodiscard]] auto get(std::nothrow_t) const -> expected<U, std::error_code> requires is_compatible_type<U>
		{
			return get_impl<U>(std::nothrow);
		}

		/** Returns reference to the specified value type.
		 * @tparam R Rvalue reference to the value type to retrieve from the Json object. Must be one of the following
		 * value types: `bool`, `int_type`, `uint_type`, `float_type`, `string_type`, `table_type`, `array_type`.
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
		 * @tparam P Pointer to the value type to retrieve from the Json object. Must be one of the following value
		 * types: `bool`, `int_type`, `uint_type`, `float_type`, `string_type`, `table_type`, `array_type`.
		 * @return Pointer of the value of the Json object, or `nullptr` if the Json object does not contain the specified type.
		 * @example
		 * @code{cpp}
		 * auto *bool_value = json.get<bool *>();
		 * @endcode */
		template<typename P>
		[[nodiscard]] constexpr P get() noexcept requires(std::is_pointer_v<P> && is_value_type<std::remove_cv_t<std::remove_pointer_t<P>>>)
		{
			return get_ptr_impl<std::remove_pointer_t<P>>();
		}
		/** @copydoc get */
		template<typename P>
		[[nodiscard]] constexpr auto get() const noexcept -> std::add_const_t<std::remove_pointer_t<P>> *
			requires(std::is_pointer_v<P> && is_value_type<std::remove_cv_t<std::remove_pointer_t<P>>>)
		{
			return get_ptr_impl<std::remove_cv_t<std::remove_pointer_t<P>>>();
		}
		// clang-format on

		// clang-format off
		/** Writes a value to the Json object.
		 * @param value Value to write. Must be an instance of the following value types: `std::nullptr_t`, `bool`,
		 * `int_type`, `uint_type`, `float_type`, `string_type`, `table_type`, `array_type`, or a compatible arithmetic type.
		 * @note Previous value of the Json object will be overwritten.
		 * @note If an error occurs during the operation, Json object is left in a defined state. */
		template<typename U>
		void write(U &&value) requires is_compatible_type<U>
		{
			as<std::remove_cvref_t<U>>(std::forward<U>(value));
		}
		/** @copydoc write
		 * @note `void` or an error code on implementation-defined write errors. */
		template<typename U>
		expected<void, std::error_code> write(std::nothrow_t, U &&value) requires is_compatible_type<U>
		{
			try
			{
				write(std::forward<U>(value));
				return {};
			}
			catch (archive_error &e)
			{
				return unexpected{e.code()};
			}
		}
		/** @copydoc write(U &&) */
		template<typename U>
		basic_json_object &operator<<(U &&value) requires is_compatible_type<U>
		{
			write(std::forward<U>(value));
			return *this;
		}

		/** Serializes an instance of type `U` to the Json object.
		 * @param value Object to serialize.
		 * @param args Arguments passed to the serialization function.
		 * @note Previous value of the Json object will be overwritten.
		 * @note If an error occurs during serialization, Json object is left in a defined state. */
		template<typename U, typename... Args>
		void write(U &&value, Args &&...args) requires(!is_compatible_type<U> && is_serializable<U, Args...>)
		{
			if (!is_container()) [[unlikely]]
				detail::invalid_json_type(json_type::CONTAINER_FLAG, m_type);

			write_frame frame{*this};
			detail::do_serialize(value, frame, std::forward<Args>(args)...);
		}
		/** @copydoc write
		 * @return `void`, or an error code on serialization errors. */
		template<typename U, typename... Args>
		expected<void, std::error_code> write(std::nothrow_t, U &&value, Args &&...args) requires(!is_compatible_type<U> && is_serializable<U, Args...>)
		{
			if (!is_container()) [[unlikely]]
				return unexpected{make_error_code(archive_errc::INVALID_TYPE)};

			try
			{
				write_frame frame{*this};
				detail::do_serialize(value, frame, std::forward<Args>(args)...);
				return {};
			}
			catch (archive_error &e)
			{
				return unexpected{e.code()};
			}
		}
		/** Serializes an instance of type `U` to the Json object.
		 * @param value Object to serialize.
		 * @return Reference to this Json object.
		 * @note Previous value of the Json object will be overwritten.
		 * @note If an error occurs during serialization, Json object is left in a defined state. */
		template<typename U>
		basic_json_object &operator<<(U &&value) requires(!is_compatible_type<U> && is_serializable<U>)
		{
			write(std::forward<U>(value));
			return *this;
		}
		// clang-format on

		// clang-format off
		/** Reads a value from the Json object.
		 * @param value Value to read. Must be an instance of the following value types: `std::nullptr_t`, `bool`,
		 * `int_type`, `uint_type`, `float_type`, `string_type`, `table_type`, `array_type`, or a compatible arithmetic type.
		 * @note If the requested type is a non-`bool` arithmetic (number) type, appropriate conversions will be preformed. */
		template<typename U>
		void read(U &value) const requires is_compatible_type<U>
		{
			if constexpr (is_value_type<std::remove_cv_t<U>>)
				value = get<U &>();
			else
				value = get<U>();
		}
		/** @copydoc read
		 * @note `void` or an error code on implementation-defined read errors. */
		template<typename U>
		expected<void, std::error_code> read(std::nothrow_t, U &value) const requires is_compatible_type<U>
		{
			if (m_type != select_type<U>()) [[unlikely]]
				return unexpected{make_error_code(archive_errc::INVALID_TYPE)};

			if constexpr (!is_value_type<std::remove_cv_t<U>>)
				value = *get<U>(std::nothrow);
			else
				value = *get<U *>();
			return {};
		}
		/** @copydoc read(U &value)
		 * @return Reference to this Json object. */
		template<typename U>
		const basic_json_object &operator>>(U &value) const requires is_compatible_type<U>
		{
			read(value);
			return *this;
		}

		/** @brief Reads a value from the Json object in-place.
		 * @param value Value to read. Must be an instance of the following value types: `std::nullptr_t`, `bool`,
		 * `int_type`, `uint_type`, `float_type`, `string_type`, `table_type`, `array_type`, or a compatible arithmetic type.
		 * @return Instance of the value read from the Json object.
		 * @note If the requested type is a non-`bool` arithmetic (number) type, appropriate conversions will be preformed. */
		template<typename U>
		[[nodiscard]] U read(std::in_place_type_t<U>) const requires is_compatible_type<U> { return get<U>(); }
		/** @copybrief read
		 * @param value Value to read. Must be an instance of the following value types: `std::nullptr_t`, `bool`,
		 * `int_type`, `uint_type`, `float_type`, `string_type`, `table_type`, `array_type`, or a compatible arithmetic type.
		 * @return Instance of the value read from the Json object, or an error code on deserialization errors.
		 * @note If the requested type is a non-`bool` arithmetic (number) type, appropriate conversions will be preformed. */
		template<typename U>
		[[nodiscard]] expected<U, std::error_code> read(std::nothrow_t, std::in_place_type_t<U>) const requires is_compatible_type<U>
		{
			return get<U>(std::nothrow);
		}

		/** Deserializes an instance of type `U` from the Json object.
		 * @param value Object to deserialize.
		 * @param args Arguments passed to the deserialization function. */
		template<typename U, typename... Args>
		void read(U &value, Args &&...args) const requires(!is_compatible_type<U> && is_deserializable<U, Args...>)
		{
			if (!is_container()) [[unlikely]]
				detail::invalid_json_type(json_type::CONTAINER_FLAG, m_type);

			read_frame frame{*this};
			detail::do_deserialize(value, frame, std::forward<Args>(args)...);
		}
		/** @copydoc read
		 * @return `void`, or an error code on deserialization errors. */
		template<typename U, typename... Args>
		expected<void, std::error_code> read(std::nothrow_t, U &value, Args &&...args) const requires(!is_compatible_type<U> && is_deserializable<U, Args...>)
		{
			if (!is_container()) [[unlikely]]
				return unexpected{make_error_code(archive_errc::INVALID_TYPE)};

			try
			{
				read_frame frame{*this};
				detail::do_deserialize(value, frame, std::forward<Args>(args)...);
				return {};
			}
			catch (archive_error &e)
			{
				return unexpected{e.code()};
			}
		}
		/** Deserializes an instance of type `U` from the Json object.
		 * @param value Object to deserialize.
		 * @return Reference to this Json object. */
		template<typename U>
		const basic_json_object &operator>>(U &value) const requires(!is_compatible_type<U> && is_deserializable<U>)
		{
			read(value);
			return *this;
		}

		/** @brief Deserializes an instance of type `U` from the Json object in-place.
		 * @param value Object to deserialize.
		 * @param args Arguments passed to the deserialization function.
		 * @return Instance of `U` deserialized from the Json object. */
		template<typename U, typename... Args>
		[[nodiscard]] U read(std::in_place_type_t<U>, Args &&...args) const requires(!is_compatible_type<U> && is_in_place_deserializable<U, Args...>)
		{
			if (!is_container()) [[unlikely]]
				detail::invalid_json_type(json_type::CONTAINER_FLAG, m_type);

			read_frame frame{*this};
			return detail::do_deserialize(std::in_place_type<U>, frame, std::forward<Args>(args)...);
		}
		/** @copybrief read
		 * @param value Object to deserialize.
		 * @param args Arguments passed to the deserialization function.
		 * @return Instance of `U` deserialized from the Json object, or an error code on deserialization errors.
		 * @note If the requested type is a non-`bool` arithmetic (number) type, appropriate conversions will be preformed. */
		template<typename U, typename... Args>
		[[nodiscard]] expected<U, std::error_code> read(std::nothrow_t, std::in_place_type_t<U>, Args &&...args) const
			requires(!is_compatible_type<U> && is_in_place_deserializable<U, Args...>)
		{
			if (!is_container()) [[unlikely]]
				return unexpected{make_error_code(archive_errc::INVALID_TYPE)};

			try
			{
				read_frame frame{*this};
				return detail::do_deserialize(std::in_place_type<U>, frame, std::forward<Args>(args)...);
			}
			catch (archive_error &e)
			{
				return unexpected{e.code()};
			}
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

		void init_table_impl(const initializer_list &il, const table_allocator &alloc)
		{
			std::construct_at(&m_table, il.size(), typename table_type::key_equal{}, typename table_type::hash_type{}, alloc);
			for (auto &ptr : il)
			{
				auto entry = std::move(ptr).extract();
				m_table.emplace(std::move(entry.m_array[0].m_string), std::move(entry.m_array[1]));
			}
			m_type = json_type::TABLE;
		}
		expected<void, std::error_code> init_table(std::nothrow_t, const initializer_list &il, const table_allocator &alloc)
		{
			/* Assert that the initializer list consists of key-value pairs */
			if (is_table_list(il)) [[unlikely]]
				return unexpected{make_error_code(archive_errc::INVALID_DATA)};

			init_table_impl(il, alloc);
			return {};
		}
		void init_table(const initializer_list &il, const table_allocator &alloc)
		{
			if (const auto err = init_table(std::nothrow, il, alloc); !err) [[unlikely]]
				throw archive_error(err.error(), "Expected a sequence of key-value pairs");
		}
		void init_array(const initializer_list &il, const array_allocator &alloc)
		{
			/* Reserve & insert elements. */
			std::construct_at(&m_array, il.size(), alloc);
			for (auto &ptr : il) m_array.emplace_back(std::move(ptr).extract());
			m_type = json_type::ARRAY;
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
			as_type_impl(select_type<U>());
		}
		template<std::same_as<bool> U, typename... Args>
		void as_impl(Args &&...args)
		{
			as_type_impl(select_type<U>());
			m_bool = bool{std::forward<Args>(args)...};
		}
		template<std::same_as<int_type> U, typename... Args>
		void as_impl(Args &&...args)
		{
			as_type_impl(select_type<U>());
			m_int = int_type{std::forward<Args>(args)...};
		}
		template<std::same_as<uint_type> U, typename... Args>
		void as_impl(Args &&...args)
		{
			as_type_impl(select_type<U>());
			m_uint = uint_type{std::forward<Args>(args)...};
		}
		template<std::same_as<float_type> U, typename... Args>
		void as_impl(Args &&...args)
		{
			as_type_impl(select_type<U>());
			m_float = float_type{std::forward<Args>(args)...};
		}
		template<std::same_as<string_type> U, typename... Args>
		void as_impl(Args &&...args)
		{
			if (!as_type_impl(select_type<U>(), [&]() { std::construct_at(&m_string, std::forward<Args>(args)...); }))
				m_string = string_type{std::forward<Args>(args)...};
		}
		template<std::same_as<table_type> U, typename... Args>
		void as_impl(Args &&...args)
		{
			if (!as_type_impl(select_type<U>(), [&]() { std::construct_at(&m_table, std::forward<Args>(args)...); }))
				m_table = table_type{std::forward<Args>(args)...};
		}
		template<std::same_as<array_type> U, typename... Args>
		void as_impl(Args &&...args)
		{
			if (!as_type_impl(select_type<U>(), [&]() { std::construct_at(&m_array, std::forward<Args>(args)...); }))
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
			assert_type(select_type<U>());
			return m_bool;
		}
		template<std::same_as<bool> U>
		[[nodiscard]] auto get_impl(std::nothrow_t) const -> expected<U, std::error_code>
		{
			if (m_type != select_type<U>()) [[unlikely]]
				return unexpected{make_error_code(archive_errc::INVALID_TYPE)};
			return m_bool;
		}
		template<std::same_as<string_type> U>
		[[nodiscard]] auto get_impl() const
		{
			assert_type(select_type<U>());
			return m_string;
		}
		template<std::same_as<string_type> U>
		[[nodiscard]] auto get_impl(std::nothrow_t) const -> expected<U, std::error_code>
		{
			if (m_type != select_type<U>()) [[unlikely]]
				return unexpected{make_error_code(archive_errc::INVALID_TYPE)};
			return m_string;
		}
		template<std::same_as<table_type> U>
		[[nodiscard]] auto get_impl() const
		{
			assert_type(select_type<U>());
			return m_table;
		}
		template<std::same_as<table_type> U>
		[[nodiscard]] auto get_impl(std::nothrow_t) const -> expected<U, std::error_code>
		{
			if (m_type != select_type<U>()) [[unlikely]]
				return unexpected{make_error_code(archive_errc::INVALID_TYPE)};
			return m_table;
		}
		template<std::same_as<array_type> U>
		[[nodiscard]] auto get_impl() const
		{
			assert_type(select_type<U>());
			return m_array;
		}
		template<std::same_as<array_type> U>
		[[nodiscard]] auto get_impl(std::nothrow_t) const -> expected<U, std::error_code>
		{
			if (m_type != select_type<U>()) [[unlikely]]
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
		[[nodiscard]] auto &get_ref_impl()
		{
			assert_type(select_type<U>());
			return m_bool;
		}
		template<std::same_as<int_type> U>
		[[nodiscard]] auto &get_ref_impl()
		{
			assert_type(select_type<U>());
			return m_int;
		}
		template<std::same_as<uint_type> U>
		[[nodiscard]] auto &get_ref_impl()
		{
			assert_type(select_type<U>());
			return m_uint;
		}
		template<std::same_as<float_type> U>
		[[nodiscard]] auto &get_ref_impl()
		{
			assert_type(select_type<U>());
			return m_float;
		}
		template<std::same_as<string_type> U>
		[[nodiscard]] auto &get_ref_impl()
		{
			assert_type(select_type<U>());
			return m_string;
		}
		template<std::same_as<table_type> U>
		[[nodiscard]] auto &get_ref_impl()
		{
			assert_type(select_type<U>());
			return m_table;
		}
		template<std::same_as<array_type> U>
		[[nodiscard]] auto &get_ref_impl()
		{
			assert_type(select_type<U>());
			return m_array;
		}

		template<std::same_as<bool> U>
		[[nodiscard]] auto &get_ref_impl() const
		{
			assert_type(select_type<U>());
			return m_bool;
		}
		template<std::same_as<int_type> U>
		[[nodiscard]] auto &get_ref_impl() const
		{
			assert_type(select_type<U>());
			return m_int;
		}
		template<std::same_as<uint_type> U>
		[[nodiscard]] auto &get_ref_impl() const
		{
			assert_type(select_type<U>());
			return m_uint;
		}
		template<std::same_as<float_type> U>
		[[nodiscard]] auto &get_ref_impl() const
		{
			assert_type(select_type<U>());
			return m_float;
		}
		template<std::same_as<string_type> U>
		[[nodiscard]] auto &get_ref_impl() const
		{
			assert_type(select_type<U>());
			return m_string;
		}
		template<std::same_as<table_type> U>
		[[nodiscard]] auto &get_ref_impl() const
		{
			assert_type(select_type<U>());
			return m_table;
		}
		template<std::same_as<array_type> U>
		[[nodiscard]] auto &get_ref_impl() const
		{
			assert_type(select_type<U>());
			return m_array;
		}

		template<std::same_as<bool> U>
		[[nodiscard]] constexpr auto *get_ptr_impl() noexcept
		{
			return is_bool() ? &m_bool : nullptr;
		}
		template<std::same_as<int_type> U>
		[[nodiscard]] constexpr auto *get_ptr_impl() noexcept
		{
			return is_int() ? &m_int : nullptr;
		}
		template<std::same_as<uint_type> U>
		[[nodiscard]] constexpr auto *get_ptr_impl() noexcept
		{
			return is_uint() ? &m_uint : nullptr;
		}
		template<std::same_as<float_type> U>
		[[nodiscard]] constexpr auto *get_ptr_impl() noexcept
		{
			return is_float() ? &m_float : nullptr;
		}
		template<std::same_as<string_type> U>
		[[nodiscard]] constexpr auto *get_ptr_impl() noexcept
		{
			return is_string() ? &m_string : nullptr;
		}
		template<std::same_as<table_type> U>
		[[nodiscard]] constexpr auto *get_ptr_impl() noexcept
		{
			return is_table() ? &m_table : nullptr;
		}
		template<std::same_as<array_type> U>
		[[nodiscard]] constexpr auto *get_ptr_impl() noexcept
		{
			return is_array() ? &m_array : nullptr;
		}

		template<std::same_as<bool> U>
		[[nodiscard]] constexpr auto *get_ptr_impl() const noexcept
		{
			return is_bool() ? &m_bool : nullptr;
		}
		template<std::same_as<int_type> U>
		[[nodiscard]] constexpr auto *get_ptr_impl() const noexcept
		{
			return is_int() ? &m_int : nullptr;
		}
		template<std::same_as<uint_type> U>
		[[nodiscard]] constexpr auto *get_ptr_impl() const noexcept
		{
			return is_uint() ? &m_uint : nullptr;
		}
		template<std::same_as<float_type> U>
		[[nodiscard]] constexpr auto *get_ptr_impl() const noexcept
		{
			return is_float() ? &m_float : nullptr;
		}
		template<std::same_as<string_type> U>
		[[nodiscard]] constexpr auto *get_ptr_impl() const noexcept
		{
			return is_string() ? &m_string : nullptr;
		}
		template<std::same_as<table_type> U>
		[[nodiscard]] constexpr auto *get_ptr_impl() const noexcept
		{
			return is_table() ? &m_table : nullptr;
		}
		template<std::same_as<array_type> U>
		[[nodiscard]] constexpr auto *get_ptr_impl() const noexcept
		{
			return is_array() ? &m_array : nullptr;
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