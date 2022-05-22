//
// Created by switchblade on 07/05/22.
//

#pragma once

#include <iterator>
#include <stdexcept>

#include "dense_hash_table.hpp"
#include "table_util.hpp"

namespace sek
{
	/** @brief One-to-one dense table based associative container providing fast iteration & insertion.
	 *
	 * Dense maps are implemented via a closed-addressing contiguous (packed) storage hash table.
	 * This allows for efficient iteration & insertion (iterate over a packed array & push on top of the array).
	 * Dense maps may invalidate iterators on insertion due to the internal packed storage being re-sized.
	 * On erasure, iterators to the erased element and elements after the erased one may be invalidated.
	 *
	 * @note Dense map iterators do not return references/pointers to pairs, instead they return special proxy
	 * reference & pointer types. This comes from a requirement for dense array elements to be assignable,
	 * thus using pairs where one of the elements is const is not possible.
	 * @note Dense maps do not provide node functionality, since the data is laid out in a contiguous packed array.
	 *
	 * @tparam K Type of objects used as keys.
	 * @tparam M Type of objects associated with keys.
	 * @tparam KeyHash Functor used to generate hashes for keys. By default uses `default_hash` which calls static
	 * non-member `hash` function via ADL if available, otherwise invokes `std::hash`.
	 * @tparam KeyComp Predicate used to compare keys.
	 * @tparam Alloc Allocator used for the map. */
	template<typename K, typename M, typename KeyHash = default_hash, typename KeyComp = std::equal_to<K>, typename Alloc = std::allocator<std::pair<const K, M>>>
	class dense_map
	{
	public:
		typedef K key_type;
		typedef M mapped_type;
		typedef std::pair<const key_type, mapped_type> value_type;

	private:
		/* Since we cannot store value_type directly within the table (const key cannot be assigned by the dense
		 * vector), need to do this ugliness with a proxy reference/value & proxy pointer types. */

		// clang-format off
		template<bool Const>
		class value_pointer;
		template<bool Const>
		class value_reference
		{
		public:
			value_reference() = delete;

			/* Conversions from relevant references. */
			constexpr value_reference(value_type &ref) noexcept : first{ref.first}, second{ref.second} {}
			constexpr value_reference(std::pair<key_type, mapped_type> &ref) noexcept : first{ref.first}, second{ref.second} {}
			constexpr value_reference(const value_type &ref) noexcept requires(Const) : first{ref.first}, second{ref.second} {}
			constexpr value_reference(const std::pair<key_type, mapped_type> &ref) noexcept requires(Const) : first{ref.first}, second{ref.second} {}

			/* Here overloading operator& is fine, since we want to use value_pointer for pointers. */
			[[nodiscard]] constexpr value_pointer<Const> operator&() const noexcept { return value_pointer<Const>{this}; }

			[[nodiscard]] constexpr auto operator<=>(const value_reference &other) const noexcept
			{
				using res_t = typename std::common_comparison_category<decltype(std::compare_three_way{}(first, other.first)),
				                                                       decltype(std::compare_three_way{}(second, other.second))>::type;

				if (const res_t cmp_first = std::compare_three_way{}(first, other.first); cmp_first == std::strong_ordering::equal)
					return res_t{std::compare_three_way{}(second, other.second)};
				else
					return cmp_first;
			}
			[[nodiscard]] constexpr auto operator<=>(const value_type &other) const noexcept
			{
				using res_t = typename std::common_comparison_category<decltype(std::compare_three_way{}(first, other.first)),
																	   decltype(std::compare_three_way{}(second, other.second))>::type;

				if (const res_t cmp_first = std::compare_three_way{}(first, other.first); cmp_first == std::strong_ordering::equal)
					return res_t{std::compare_three_way{}(second, other.second)};
				else
					return cmp_first;
			}
			[[nodiscard]] constexpr bool operator==(const value_reference &other) const noexcept
			{
				return first == other.first ? true : second == other.second;
			}
			[[nodiscard]] constexpr bool operator==(const value_type &other) const noexcept
			{
				return first == other.first ? true : second == other.second;
			}

			const key_type &first;
			std::conditional_t<Const, const mapped_type, mapped_type> &second;
		};
		template<bool Const>
		class value_pointer
		{
		public:
			using pointer = value_reference<Const> *;

		public:
			value_pointer(const value_pointer &) = delete;
			value_pointer &operator=(const value_pointer &) = delete;

			constexpr value_pointer(value_pointer &&) noexcept = default;
			constexpr value_pointer &operator=(value_pointer &&) noexcept = default;

			/* Conversions from relevant pointers. */
			constexpr value_pointer(value_type *ptr) noexcept : ref{*ptr} {}
			constexpr value_pointer(std::pair<key_type, mapped_type> *ptr) noexcept : ref{*ptr} {}
			constexpr value_pointer(const value_type *ptr) noexcept requires(Const) : ref{*ptr} {}
			constexpr value_pointer(const std::pair<key_type, mapped_type> *ptr) noexcept requires(Const) : ref{*ptr} {}

			/* Conversion from value reference. */
			constexpr value_pointer(value_reference<Const> &&ref) noexcept : ref{ref} {}
			constexpr value_pointer(const value_reference<Const> *ptr) noexcept : ref{*ptr} {}

			/* Pointer-like operators. */
			[[nodiscard]] constexpr pointer operator->() noexcept { return std::addressof(ref); }
			[[nodiscard]] constexpr value_reference<Const> operator*() const noexcept { return ref; }

			/* Since value_reference stores reference to adjacent pair members, comparing the first element is enough. */
			[[nodiscard]] constexpr auto operator<=>(const value_pointer &other) const noexcept
			{
				return std::addressof(ref.first) <=> std::addressof(other.ref.first);
			}
			[[nodiscard]] constexpr bool operator==(const value_pointer &other) const noexcept
			{
				return std::addressof(ref.first) == std::addressof(other.ref.first);
			}

		private:
			value_reference<Const> ref;
		};
		// clang-format on

		struct value_traits
		{
			template<bool Const>
			using iterator_value = value_reference<Const>;
			template<bool Const>
			using iterator_reference = value_reference<Const>;
			template<bool Const>
			using iterator_pointer = value_pointer<Const>;
		};

		using table_type =
			detail::dense_hash_table<K, std::pair<key_type, mapped_type>, value_traits, KeyHash, KeyComp, pair_first, Alloc>;

	public:
		typedef value_pointer<false> pointer;
		typedef value_pointer<true> const_pointer;
		typedef value_reference<false> reference;
		typedef value_reference<true> const_reference;
		typedef typename table_type::size_type size_type;
		typedef typename table_type::difference_type difference_type;

		typedef typename table_type::value_allocator_type allocator_type;
		typedef typename table_type::bucket_allocator_type bucket_allocator_type;
		typedef typename table_type::hash_type hash_type;
		typedef typename table_type::key_equal key_equal;

		typedef typename table_type::iterator iterator;
		typedef typename table_type::const_iterator const_iterator;
		typedef typename table_type::reverse_iterator reverse_iterator;
		typedef typename table_type::const_reverse_iterator const_reverse_iterator;
		typedef typename table_type::local_iterator local_iterator;
		typedef typename table_type::const_local_iterator const_local_iterator;

	public:
		constexpr dense_map() noexcept(std::is_nothrow_default_constructible_v<table_type>) = default;
		constexpr ~dense_map() = default;

		/** Constructs a map with the specified allocators.
		 * @param value_alloc Allocator used to allocate map's value array.
		 * @param bucket_alloc Allocator used to allocate map's bucket array. */
		constexpr explicit dense_map(const allocator_type &value_alloc,
									 const bucket_allocator_type &bucket_alloc = bucket_allocator_type{})
			: dense_map(key_equal{}, hash_type{}, value_alloc, bucket_alloc)
		{
		}
		/** Constructs a map with the specified hasher & allocators.
		 * @param key_hash Key hasher.
		 * @param value_alloc Allocator used to allocate map's value array.
		 * @param bucket_alloc Allocator used to allocate map's bucket array. */
		constexpr explicit dense_map(const hash_type &key_hash,
									 const allocator_type &value_alloc = allocator_type{},
									 const bucket_allocator_type &bucket_alloc = bucket_allocator_type{})
			: dense_map(key_equal{}, key_hash, value_alloc, bucket_alloc)
		{
		}
		/** Constructs a map with the specified comparator, hasher & allocators.
		 * @param key_compare Key comparator.
		 * @param key_hash Key hasher.
		 * @param value_alloc Allocator used to allocate map's value array.
		 * @param bucket_alloc Allocator used to allocate map's bucket array. */
		constexpr explicit dense_map(const key_equal &key_compare,
									 const hash_type &key_hash = {},
									 const allocator_type &value_alloc = allocator_type{},
									 const bucket_allocator_type &bucket_alloc = bucket_allocator_type{})
			: data_table(key_compare, key_hash, value_alloc, bucket_alloc)
		{
		}
		/** Constructs a map with the specified minimum capacity.
		 * @param capacity Capacity of the map.
		 * @param key_compare Key comparator.
		 * @param key_hash Key hasher.
		 * @param value_alloc Allocator used to allocate map's value array.
		 * @param bucket_alloc Allocator used to allocate map's bucket array. */
		constexpr explicit dense_map(size_type capacity,
									 const key_equal &key_compare = {},
									 const hash_type &key_hash = {},
									 const allocator_type &value_alloc = allocator_type{},
									 const bucket_allocator_type &bucket_alloc = bucket_allocator_type{})
			: data_table(capacity, key_compare, key_hash, value_alloc, bucket_alloc)
		{
		}

		/** Constructs a map from a sequence of values.
		 * @param first Iterator to the start of the value sequence.
		 * @param first Iterator to the end of the value sequence.
		 * @param key_compare Key comparator.
		 * @param key_hash Key hasher.
		 * @param value_alloc Allocator used to allocate map's value array.
		 * @param bucket_alloc Allocator used to allocate map's bucket array. */
		template<std::random_access_iterator Iterator>
		constexpr dense_map(Iterator first,
							Iterator last,
							const key_equal &key_compare = {},
							const hash_type &key_hash = {},
							const allocator_type &value_alloc = allocator_type{},
							const bucket_allocator_type &bucket_alloc = bucket_allocator_type{})
			: dense_map(static_cast<size_type>(std::distance(first, last)), key_compare, key_hash, value_alloc, bucket_alloc)
		{
			insert(first, last);
		}
		/** Constructs a map from a sequence of values.
		 * @param first Iterator to the start of the value sequence.
		 * @param first Iterator to the end of the value sequence.
		 * @param key_compare Key comparator.
		 * @param key_hash Key hasher.
		 * @param value_alloc Allocator used to allocate map's value array.
		 * @param bucket_alloc Allocator used to allocate map's bucket array. */
		template<std::forward_iterator Iterator>
		constexpr dense_map(Iterator first,
							Iterator last,
							const key_equal &key_compare = {},
							const hash_type &key_hash = {},
							const allocator_type &value_alloc = allocator_type{},
							const bucket_allocator_type &bucket_alloc = bucket_allocator_type{})
			: dense_map(key_compare, key_hash, value_alloc, bucket_alloc)
		{
			insert(first, last);
		}
		/** Constructs a map from an initializer list.
		 * @param il Initializer list containing values.
		 * @param key_compare Key comparator.
		 * @param key_hash Key hasher.
		 * @param value_alloc Allocator used to allocate map's value array.
		 * @param bucket_alloc Allocator used to allocate map's bucket array. */
		constexpr dense_map(std::initializer_list<value_type> il,
							const key_equal &key_compare = {},
							const hash_type &key_hash = {},
							const allocator_type &value_alloc = allocator_type{},
							const bucket_allocator_type &bucket_alloc = bucket_allocator_type{})
			: dense_map(il.begin(), il.end(), key_compare, key_hash, value_alloc, bucket_alloc)
		{
		}

		/** Copy-constructs the map. Both allocators are copied via `select_on_container_copy_construction`.
		 * @param other Map to copy data and allocators from. */
		constexpr dense_map(const dense_map &other) noexcept(std::is_nothrow_copy_constructible_v<table_type>)
			: data_table(other.data_table)
		{
		}
		/** Copy-constructs the map. Bucket allocator is copied via `select_on_container_copy_construction`.
		 * @param other Map to copy data and bucket allocator from.
		 * @param value_alloc Allocator used to allocate map's value array. */
		constexpr dense_map(const dense_map &other, const allocator_type &value_alloc) noexcept(
			std::is_nothrow_constructible_v<table_type, const table_type &, const allocator_type &>)
			: data_table(other.data_table, value_alloc)
		{
		}
		/** Copy-constructs the map.
		 * @param other Map to copy data from.
		 * @param value_alloc Allocator used to allocate map's value array.
		 * @param bucket_alloc Allocator used to allocate map's bucket array. */
		constexpr dense_map(const dense_map &other, const allocator_type &value_alloc, const bucket_allocator_type &bucket_alloc) noexcept(
			std::is_nothrow_constructible_v<table_type, const table_type &, const allocator_type &, const bucket_allocator_type &>)
			: data_table(other.data_table, value_alloc, bucket_alloc)
		{
		}

		/** Move-constructs the map. Both allocators are move-constructed.
		 * @param other Map to move elements and allocators from. */
		constexpr dense_map(dense_map &&other) noexcept(std::is_nothrow_move_constructible_v<table_type>)
			: data_table(std::move(other.data_table))
		{
		}
		/** Move-constructs the map. Bucket allocator is move-constructed.
		 * @param other Map to move elements and bucket allocator from.
		 * @param value_alloc Allocator used to allocate map's value array. */
		constexpr dense_map(dense_map &&other, const allocator_type &value_alloc) noexcept(
			std::is_nothrow_constructible_v<table_type, table_type &&, const allocator_type &>)
			: data_table(std::move(other.data_table), value_alloc)
		{
		}
		/** Move-constructs the map.
		 * @param other Map to move elements from.
		 * @param value_alloc Allocator used to allocate map's value array.
		 * @param bucket_alloc Allocator used to allocate map's bucket array. */
		constexpr dense_map(dense_map &&other, const allocator_type &value_alloc, const bucket_allocator_type &bucket_alloc) noexcept(
			std::is_nothrow_constructible_v<table_type, table_type &&, const allocator_type &, const bucket_allocator_type &>)
			: data_table(std::move(other.data_table), value_alloc, bucket_alloc)
		{
		}

		/** Copy-assigns the map.
		 * @param other Map to copy elements from. */
		constexpr dense_map &operator=(const dense_map &other)
		{
			if (this != &other) data_table = other.data_table;
			return *this;
		}
		/** Move-assigns the map.
		 * @param other Map to move elements from. */
		constexpr dense_map &operator=(dense_map &&other) noexcept(std::is_nothrow_move_assignable_v<table_type>)
		{
			data_table = std::move(other.data_table);
			return *this;
		}

		/** Returns iterator to the start of the map. */
		[[nodiscard]] constexpr iterator begin() noexcept { return data_table.begin(); }
		/** Returns iterator to the end of the map. */
		[[nodiscard]] constexpr iterator end() noexcept { return data_table.end(); }
		/** Returns const iterator to the start of the map. */
		[[nodiscard]] constexpr const_iterator cbegin() const noexcept { return data_table.begin(); }
		/** Returns const iterator to the end of the map. */
		[[nodiscard]] constexpr const_iterator cend() const noexcept { return data_table.end(); }
		/** @copydoc cbegin */
		[[nodiscard]] constexpr const_iterator begin() const noexcept { return cbegin(); }
		/** @copydoc cend */
		[[nodiscard]] constexpr const_iterator end() const noexcept { return cend(); }

		/** Returns reverse iterator to the end of the map. */
		[[nodiscard]] constexpr reverse_iterator rbegin() noexcept { return data_table.rbegin(); }
		/** Returns reverse iterator to the start of the map. */
		[[nodiscard]] constexpr reverse_iterator rend() noexcept { return data_table.rend(); }
		/** Returns const reverse iterator to the end of the map. */
		[[nodiscard]] constexpr const_reverse_iterator crbegin() const noexcept { return data_table.crbegin(); }
		/** Returns const reverse iterator to the start of the map. */
		[[nodiscard]] constexpr const_reverse_iterator crend() const noexcept { return data_table.crend(); }
		/** @copydoc crbegin */
		[[nodiscard]] constexpr const_reverse_iterator rbegin() const noexcept { return crbegin(); }
		/** @copydoc crend */
		[[nodiscard]] constexpr const_reverse_iterator rend() const noexcept { return crend(); }

		/** Locates an element for the specific key.
		 * @param key Key to search for.
		 * @return Iterator to the element mapped to key. */
		constexpr iterator find(const key_type &key) noexcept { return data_table.find(key); }
		/** @copydoc find */
		constexpr const_iterator find(const key_type &key) const noexcept { return data_table.find(key); }

		/** Checks if the map contains an element with specific key.
		 * @param key Key to search for. */
		constexpr bool contains(const key_type &key) const noexcept { return find(key) != end(); }

		/** Returns reference to object mapped to the specific key.
		 * @param key Key to search for.
		 * @return Reference to the object mapped to key.
		 * @throw std::out_of_range If the specified key is not present in the map. */
		constexpr mapped_type &at(const key_type &key)
		{
			if (auto iter = find(key); iter != end()) [[likely]]
				return iter->second;
			else
				throw std::out_of_range("Specified key is not present within the map");
		}
		/** Returns const reference to object mapped to the specific key.
		 * @param key Key to search for.
		 * @return Reference to the object mapped to key.
		 * @throw std::out_of_range If the specified key is not present in the map. */
		constexpr const mapped_type &at(const key_type &key) const
		{
			if (auto iter = find(key); iter != end()) [[likely]]
				return iter->second;
			else
				throw std::out_of_range("Specified key is not present within the map");
		}

		/** Returns reference to object at the specific key or inserts a new value if it does not exist.
		 * @param key Key to search for.
		 * @return Reference to the object mapped to key. */
		constexpr mapped_type &operator[](const key_type &key) noexcept
		{
			return try_emplace(key, mapped_type{}).first->second;
		}
		/** @copydoc operator[] */
		constexpr mapped_type &operator[](key_type &&key) noexcept
		{
			return try_emplace(std::forward<key_type>(key), mapped_type{}).first->second;
		}

		/** Empties the map's contents. */
		constexpr void clear() { data_table.clear(); }

		/** Re-hashes the map for the specified minimal capacity. */
		constexpr void rehash(size_type capacity) { data_table.rehash(capacity); }
		/** Resizes the internal storage to have space for at least n elements. */
		constexpr void reserve(size_type n) { data_table.reserve(n); }

		/** Attempts to construct a value in-place at the specified key.
		 * If such key is already associated with a value, does nothing.
		 * @param key Key for which to insert the value.
		 * @param args Arguments used to construct the mapped object.
		 * @return Pair where first element is the iterator to the potentially inserted element
		 * and second is boolean indicating whether the element was inserted (`true` if inserted, `false` otherwise). */
		template<typename... Args>
		constexpr std::pair<iterator, bool> try_emplace(key_type &&key, Args &&...args)
		{
			return data_table.try_emplace(std::forward<key_type>(key), std::forward<Args>(args)...);
		}
		/** @copydoc try_emplace */
		template<typename... Args>
		constexpr std::pair<iterator, bool> try_emplace(const key_type &key, Args &&...args)
		{
			return data_table.try_emplace(key, std::forward<Args>(args)...);
		}
		/** Constructs a value (of value_type) in-place.
		 * If a value for the constructed key is already present within the map, replaces that value.
		 * @param args Arguments used to construct the value object.
		 * @return Pair where first element is the iterator to the inserted element
		 * and second is boolean indicating whether the element was inserted or replace (`true` if inserted new, `false` if replaced). */
		template<typename... Args>
		constexpr std::pair<iterator, bool> emplace(Args &&...args)
		{
			return data_table.emplace(std::forward<Args>(args)...);
		}

		/** Attempts to insert a value into the map.
		 * If a value with the same key is already present within the map, does not replace it.
		 * @param value Value to insert.
		 * @return Pair where first element is the iterator to the potentially inserted element
		 * and second is boolean indicating whether the element was inserted (`true` if inserted, `false` otherwise). */
		constexpr std::pair<iterator, bool> try_insert(value_type &&value)
		{
			return data_table.try_insert(std::forward<value_type>(value));
		}
		/** @copydoc try_insert */
		constexpr std::pair<iterator, bool> try_insert(const value_type &value) { return data_table.try_insert(value); }
		/** @copydetails try_insert
		 * @param hint Hint for where to insert the value.
		 * @param value Value to insert.
		 * @return Iterator to the potentially inserted element or the element that prevented insertion.
		 * @note Hint is required for compatibility with STL algorithms and is ignored.  */
		constexpr iterator try_insert([[maybe_unused]] const_iterator hint, value_type &&value)
		{
			return try_insert(std::forward<value_type>(value)).first;
		}
		/** @copydoc try_insert */
		constexpr iterator try_insert([[maybe_unused]] const_iterator hint, const value_type &value)
		{
			return try_insert(value).first;
		}
		/** Attempts to insert a sequence of values (of value_type) into the map.
		 * If values with the same key are already present within the map, does not replace them.
		 * @param first Iterator to the start of the value sequence.
		 * @param first Iterator to the end of the value sequence.
		 * @return Amount of elements inserted. */
		template<std::forward_iterator Iterator>
		constexpr size_type try_insert(Iterator first, Iterator last)
		{
			return data_table.try_insert(first, last);
		}
		/** Attempts to insert a sequence of values (of value_type) specified by the initializer list into the map.
		 * If values with the same key are already present within the map, does not replace them.
		 * @param il Initializer list containing the values.
		 * @return Amount of elements inserted. */
		constexpr size_type try_insert(std::initializer_list<value_type> il)
		{
			return try_insert(il.begin(), il.end());
		}

		/** Inserts a value (of value_type) into the map.
		 * If a value with the same key is already present within the map, replaces that value.
		 * @param value Value to insert.
		 * @return Pair where first element is the iterator to the inserted element
		 * and second is boolean indicating whether the element was inserted or replaced (`true` if inserted new, `false` if replaced). */
		constexpr std::pair<iterator, bool> insert(value_type &&value)
		{
			return data_table.insert(std::forward<value_type>(value));
		}
		/** @copydoc insert */
		constexpr std::pair<iterator, bool> insert(const value_type &value) { return data_table.insert(value); }
		/** @copydetails insert
		 * @param hint Hint for where to insert the value.
		 * @param value Value to insert.
		 * @return Iterator to the inserted element.
		 * @note Hint is required for compatibility with STL algorithms and is ignored. */
		constexpr iterator insert([[maybe_unused]] const_iterator hint, value_type &&value)
		{
			return insert(std::forward<value_type>(value)).first;
		}
		/** @copydoc insert */
		constexpr iterator insert([[maybe_unused]] const_iterator hint, const value_type &value)
		{
			return insert(value).first;
		}
		/** Inserts a sequence of values (of value_type) into the map.
		 * If values with the same key are already present within the map, replaces them.
		 * @param first Iterator to the start of the value sequence.
		 * @param first Iterator to the end of the value sequence.
		 * @return Amount of *new* elements inserted. */
		template<std::forward_iterator Iterator>
		constexpr size_type insert(Iterator first, Iterator last)
		{
			return data_table.insert(first, last);
		}
		/** Inserts a sequence of values (of value_type) specified by the initializer list into the map.
		 * If values with the same key are already present within the map, replaces them.
		 * @param il Initializer list containing the values.
		 * @return Amount of new elements inserted. */
		constexpr size_type insert(std::initializer_list<value_type> il)
		{
			return insert(il.begin(), il.end());
		}

		/** Removes the specified element from the map.
		 * @param where Iterator to the target element.
		 * @return Iterator to the element after the erased one. */
		constexpr iterator erase(const_iterator where) { return data_table.erase(where); }
		/** Removes all elements in the [first, last) range.
		 * @param first Iterator to the first element of the target range.
		 * @param last Iterator to the last element of the target range.
		 * @return Iterator to the element after the erased sequence. */
		constexpr iterator erase(const_iterator first, const_iterator last) { return data_table.erase(first, last); }
		/** Removes element mapped to the specified key from the map if it is present.
		 * @param key Key of the target element.
		 * @return `true` if the element was removed, `false` otherwise. */
		constexpr bool erase(const key_type &key)
		{
			if (auto target = data_table.find(key); target != data_table.end())
			{
				data_table.erase(target);
				return true;
			}
			else
				return false;
		}

		/** Returns current amount of elements in the map. */
		[[nodiscard]] constexpr size_type size() const noexcept { return data_table.size(); }
		/** Returns current capacity of the map. */
		[[nodiscard]] constexpr size_type capacity() const noexcept { return data_table.capacity(); }
		/** Returns maximum possible amount of elements in the map. */
		[[nodiscard]] constexpr size_type max_size() const noexcept { return data_table.max_size(); }
		/** Checks if the map is empty. */
		[[nodiscard]] constexpr size_type empty() const noexcept { return size() == 0; }

		/** Returns current amount of buckets in the map. */
		[[nodiscard]] constexpr size_type bucket_count() const noexcept { return data_table.bucket_count(); }
		/** Returns the maximum amount of buckets. */
		[[nodiscard]] constexpr size_type max_bucket_count() const noexcept { return data_table.max_bucket_count(); }

		/** Returns local iterator to the start of a bucket. */
		[[nodiscard]] constexpr local_iterator begin(size_type bucket) noexcept { return data_table.begin(bucket); }
		/** Returns const local iterator to the start of a bucket. */
		[[nodiscard]] constexpr const_local_iterator cbegin(size_type bucket) const noexcept
		{
			return data_table.cbegin(bucket);
		}
		/** @copydoc cbegin */
		[[nodiscard]] constexpr const_local_iterator begin(size_type bucket) const noexcept
		{
			return data_table.begin(bucket);
		}
		/** Returns local iterator to the end of a bucket. */
		[[nodiscard]] constexpr local_iterator end(size_type bucket) noexcept { return data_table.end(bucket); }
		/** Returns const local iterator to the end of a bucket. */
		[[nodiscard]] constexpr const_local_iterator cend(size_type bucket) const noexcept
		{
			return data_table.cend(bucket);
		}
		/** @copydoc cbegin */
		[[nodiscard]] constexpr const_local_iterator end(size_type bucket) const noexcept
		{
			return data_table.end(bucket);
		}

		/** Returns the amount of elements stored within the bucket. */
		[[nodiscard]] constexpr size_type bucket_size(size_type bucket) const noexcept
		{
			return data_table.bucket_size(bucket);
		}
		/** Returns the index of the bucket associated with a key. */
		[[nodiscard]] constexpr size_type bucket(const key_type &key) const noexcept { return data_table.bucket(key); }
		/** Returns the index of the bucket containing the pointed-to element. */
		[[nodiscard]] constexpr size_type bucket(const_iterator iter) const noexcept { return data_table.bucket(iter); }

		/** Returns current load factor of the map. */
		[[nodiscard]] constexpr auto load_factor() const noexcept { return data_table.load_factor(); }
		/** Returns current max load factor of the map. */
		[[nodiscard]] constexpr auto max_load_factor() const noexcept { return data_table.max_load_factor; }
		/** Sets current max load factor of the map. */
		constexpr void max_load_factor(float f) noexcept
		{
			SEK_ASSERT(f > .0f);
			data_table.max_load_factor = f;
		}

		[[nodiscard]] constexpr allocator_type get_allocator() const noexcept { return data_table.value_allocator(); }
		[[nodiscard]] constexpr bucket_allocator_type get_bucket_allocator() const noexcept
		{
			return data_table.bucket_allocator();
		}

		[[nodiscard]] constexpr hash_type hash_function() const noexcept { return data_table.get_hash(); }
		[[nodiscard]] constexpr key_equal key_eq() const noexcept { return data_table.get_comp(); }

		[[nodiscard]] constexpr bool operator==(const dense_map &other) const noexcept
		{
			return std::is_permutation(begin(), end(), other.begin(), other.end());
		}

		constexpr void swap(dense_map &other) noexcept { data_table.swap(other.data_table); }
		friend constexpr void swap(dense_map &a, dense_map &b) noexcept { a.swap(b); }

	private:
		/** Hash table used to implement the map. */
		table_type data_table;
	};
}	 // namespace sek