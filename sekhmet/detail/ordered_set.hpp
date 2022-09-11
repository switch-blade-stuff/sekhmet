/*
 * Created by switchblade on 07/05/22
 */

#pragma once

#include <iterator>
#include <stdexcept>

#include "ordered_hash_table.hpp"
#include "table_util.hpp"

namespace sek
{
	/** @brief Set container providing fast insertion while preserving insertion order.
	 *
	 * Ordered sets are implemented via a closed-addressing contiguous (packed) storage hash table and a linked list
	 * used to keep track of the insertion order. This allows for efficient constant-time insertion and optimal cache locality.
	 * Ordered sets may invalidate iterators on insertion due to the internal packed storage being resized.
	 * On erasure, iterators to the erased element are invalidated.
	 *
	 * @tparam T Type of objects stored in the set.
	 * @tparam KeyHash Functor used to generate hashes for keys. By default uses `default_hash` which calls static
	 * non-member `hash` function via ADL if available, otherwise invokes `std::hash`.
	 * @tparam KeyComp Predicate used to compare keys.
	 * @tparam Alloc Allocator used for the set. */
	template<typename T, typename KeyHash = default_hash, typename KeyComp = std::equal_to<T>, typename Alloc = std::allocator<T>>
	class ordered_set
	{
	public:
		typedef T key_type;
		typedef T value_type;
		typedef Alloc allocator_type;

	private:
		struct value_traits
		{
			using value_type = T;

			using reference = value_type &;
			using const_reference = const value_type &;

			using pointer = value_type *;
			using const_pointer = const value_type *;
		};

		using table_type = detail::ordered_hash_table<T, T, value_traits, KeyHash, KeyComp, forward_identity, Alloc>;

		// clang-format off
		constexpr static bool transparent_key = requires
		{
			typename KeyHash::is_transparent;
			typename KeyComp::is_transparent;
		};
		// clang-format on

	public:
		typedef typename value_traits::const_pointer pointer;
		typedef typename value_traits::const_pointer const_pointer;
		typedef typename value_traits::const_reference reference;
		typedef typename value_traits::const_reference const_reference;

		typedef typename table_type::hash_type hash_type;
		typedef typename table_type::key_equal key_equal;

		typedef typename table_type::const_iterator iterator;
		typedef typename table_type::const_iterator const_iterator;
		typedef typename table_type::const_reverse_iterator reverse_iterator;
		typedef typename table_type::const_reverse_iterator const_reverse_iterator;
		typedef typename table_type::const_local_iterator local_iterator;
		typedef typename table_type::const_local_iterator const_local_iterator;
		typedef typename table_type::size_type size_type;
		typedef typename table_type::difference_type difference_type;

	public:
		constexpr ordered_set() noexcept(std::is_nothrow_default_constructible_v<table_type>) = default;
		constexpr ~ordered_set() = default;

		/** Constructs a set with the specified allocators.
		 * @param alloc Allocator used to allocate set's value array.
		 * @param bucket_alloc Allocator used to allocate set's bucket array. */
		constexpr explicit ordered_set(const allocator_type &alloc) : ordered_set(key_equal{}, hash_type{}, alloc) {}
		/** Constructs a set with the specified hasher & allocators.
		 * @param key_hash Key hasher.
		 * @param alloc Allocator used to allocate set's value array. */
		constexpr explicit ordered_set(const hash_type &key_hash, const allocator_type &alloc = allocator_type{})
			: ordered_set(key_equal{}, key_hash, alloc)
		{
		}
		/** Constructs a set with the specified comparator, hasher & allocators.
		 * @param key_compare Key comparator.
		 * @param key_hash Key hasher.
		 * @param alloc Allocator used to allocate set's value array. */
		constexpr explicit ordered_set(const key_equal &key_compare,
									   const hash_type &key_hash = {},
									   const allocator_type &alloc = allocator_type{})
			: m_table(key_compare, key_hash, alloc)
		{
		}
		/** Constructs a set with the specified minimum capacity.
		 * @param capacity Capacity of the set.
		 * @param key_compare Key comparator.
		 * @param key_hash Key hasher.
		 * @param alloc Allocator used to allocate set's value array. */
		constexpr explicit ordered_set(size_type capacity,
									   const KeyComp &key_compare = {},
									   const KeyHash &key_hash = {},
									   const allocator_type &alloc = allocator_type{})
			: m_table(capacity, key_compare, key_hash, alloc)
		{
		}

		/** Constructs a set from a sequence of values.
		 * @param first Iterator to the start of the value sequence.
		 * @param first Iterator to the end of the value sequence.
		 * @param key_compare Key comparator.
		 * @param key_hash Key hasher.
		 * @param alloc Allocator used to allocate set's value array. */
		template<std::random_access_iterator Iterator>
		constexpr ordered_set(Iterator first,
							  Iterator last,
							  const KeyComp &key_compare = {},
							  const KeyHash &key_hash = {},
							  const allocator_type &alloc = allocator_type{})
			: ordered_set(static_cast<size_type>(std::distance(first, last)), key_compare, key_hash, alloc)
		{
			insert(first, last);
		}
		/** Constructs a set from a sequence of values.
		 * @param first Iterator to the start of the value sequence.
		 * @param first Iterator to the end of the value sequence.
		 * @param key_compare Key comparator.
		 * @param key_hash Key hasher.
		 * @param alloc Allocator used to allocate set's value array. */
		template<std::forward_iterator Iterator>
		constexpr ordered_set(Iterator first,
							  Iterator last,
							  const KeyComp &key_compare = {},
							  const KeyHash &key_hash = {},
							  const allocator_type &alloc = allocator_type{})
			: ordered_set(key_compare, key_hash, alloc)
		{
			insert(first, last);
		}
		/** Constructs a set from an initializer list.
		 * @param il Initializer list containing values.
		 * @param key_compare Key comparator.
		 * @param key_hash Key hasher.
		 * @param alloc Allocator used to allocate set's value array. */
		constexpr ordered_set(std::initializer_list<value_type> il,
							  const KeyComp &key_compare = {},
							  const KeyHash &key_hash = {},
							  const allocator_type &alloc = allocator_type{})
			: ordered_set(il.begin(), il.end(), key_compare, key_hash, alloc)
		{
		}

		/** Copy-constructs the set. Allocator is copied via `select_on_container_copy_construction`.
		 * @param other Map to copy data and allocators from. */
		constexpr ordered_set(const ordered_set &other) noexcept(std::is_nothrow_copy_constructible_v<table_type>)
			: m_table(other.m_table)
		{
		}
		/** Copy-constructs the set.
		 * @param other Map to copy data and bucket allocator from.
		 * @param alloc Allocator used to allocate set's value array. */
		constexpr ordered_set(const ordered_set &other, const allocator_type &alloc) noexcept(
			std::is_nothrow_constructible_v<table_type, const table_type &, const allocator_type &>)
			: m_table(other.m_table, alloc)
		{
		}
		/** Move-constructs the set. Allocator is move-constructed.
		 * @param other Map to move elements and bucket allocator from. */
		constexpr ordered_set(ordered_set &&other) noexcept(std::is_nothrow_move_constructible_v<table_type>)
			: m_table(std::move(other.m_table))
		{
		}
		/** Move-constructs the set.
		 * @param other Map to move elements and bucket allocator from.
		 * @param alloc Allocator used to allocate set's value array. */
		constexpr ordered_set(ordered_set &&other, const allocator_type &alloc) noexcept(
			std::is_nothrow_constructible_v<table_type, table_type &&, const allocator_type &>)
			: m_table(std::move(other.m_table), alloc)
		{
		}

		/** Copy-assigns the set.
		 * @param other Map to copy elements from. */
		constexpr ordered_set &operator=(const ordered_set &other)
		{
			if (this != &other) m_table = other.m_table;
			return *this;
		}
		/** Move-assigns the set.
		 * @param other Map to move elements from. */
		constexpr ordered_set &operator=(ordered_set &&other) noexcept(std::is_nothrow_move_assignable_v<table_type>)
		{
			m_table = std::move(other.m_table);
			return *this;
		}

		/** Returns iterator to the start of the set. */
		[[nodiscard]] constexpr iterator begin() noexcept { return m_table.begin(); }
		/** Returns iterator to the end of the set. */
		[[nodiscard]] constexpr iterator end() noexcept { return m_table.end(); }
		/** Returns const iterator to the start of the set. */
		[[nodiscard]] constexpr const_iterator cbegin() const noexcept { return m_table.begin(); }
		/** Returns const iterator to the end of the set. */
		[[nodiscard]] constexpr const_iterator cend() const noexcept { return m_table.end(); }
		/** @copydoc cbegin */
		[[nodiscard]] constexpr const_iterator begin() const noexcept { return cbegin(); }
		/** @copydoc cend */
		[[nodiscard]] constexpr const_iterator end() const noexcept { return cend(); }

		/** Returns reverse iterator to the end of the set. */
		[[nodiscard]] constexpr reverse_iterator rbegin() noexcept { return m_table.rbegin(); }
		/** Returns reverse iterator to the start of the set. */
		[[nodiscard]] constexpr reverse_iterator rend() noexcept { return m_table.rend(); }
		/** Returns const reverse iterator to the end of the set. */
		[[nodiscard]] constexpr const_reverse_iterator crbegin() const noexcept { return m_table.crbegin(); }
		/** Returns const reverse iterator to the start of the set. */
		[[nodiscard]] constexpr const_reverse_iterator crend() const noexcept { return m_table.crend(); }
		/** @copydoc crbegin */
		[[nodiscard]] constexpr const_reverse_iterator rbegin() const noexcept { return crbegin(); }
		/** @copydoc crend */
		[[nodiscard]] constexpr const_reverse_iterator rend() const noexcept { return crend(); }

		/** Locates an element within the set.
		 * @param key Key to search for.
		 * @return Iterator to the element setped to key. */
		[[nodiscard]] constexpr const_iterator find(const key_type &key) const noexcept { return m_table.find(key); }
		/** @copydoc find
		 * @note This overload participates in overload resolution only
		 * if both key hasher and key comparator are transparent. */
		[[nodiscard]] constexpr const_iterator find(const auto &key) const noexcept
			requires transparent_key
		{
			return m_table.find(key);
		}
		/** Checks if the set contains a specific element.
		 * @param key Key to search for. */
		[[nodiscard]] constexpr bool contains(const key_type &key) const noexcept { return find(key) != end(); }
		/** @copydoc contains
		 * @note This overload participates in overload resolution only
		 * if both key hasher and key comparator are transparent. */
		[[nodiscard]] constexpr bool contains(const auto &key) const noexcept
			requires transparent_key
		{
			return find(key) != end();
		}

		/** Returns reference to the first element of the set. */
		[[nodiscard]] constexpr reference front() noexcept { return *begin(); }
		/** @copydoc front */
		[[nodiscard]] constexpr const_reference front() const noexcept { return *begin(); }
		/** Returns reference to the last element of the set. */
		[[nodiscard]] constexpr reference back() noexcept { return *rbegin(); }
		/** @copydoc front */
		[[nodiscard]] constexpr const_reference back() const noexcept { return *rbegin(); }

		/** Empties the set's contents. */
		constexpr void clear() { m_table.clear(); }

		/** Re-hashes the set for the specified minimal capacity. */
		constexpr void rehash(size_type capacity) { m_table.rehash(capacity); }
		/** Resizes the internal storage to have space for at least n elements. */
		constexpr void reserve(size_type n) { m_table.reserve(n); }

		/** Constructs a value (of element_t) in-place.
		 * If the same value is already present within the set, replaces that value.
		 * @param args Arguments used to construct the value object.
		 * @return Pair where first element is the iterator to the inserted element
		 * and second is boolean indicating whether the element was inserted or replace (`true` if inserted new, `false` if replaced). */
		template<typename... Args>
		constexpr std::pair<iterator, bool> emplace(Args &&...args)
		{
			return m_table.emplace(std::forward<Args>(args)...);
		}

		/** Attempts to insert a value into the set.
		 * If the same value is already present within the set, does not replace it.
		 * @param value Value to insert.
		 * @return Pair where first element is the iterator to the potentially inserted element
		 * and second is boolean indicating whether the element was inserted (`true` if inserted, `false` otherwise). */
		constexpr std::pair<iterator, bool> try_insert(value_type &&value)
		{
			return m_table.try_insert(std::forward<value_type>(value));
		}
		/** @copydoc try_insert */
		constexpr std::pair<iterator, bool> try_insert(const value_type &value) { return m_table.try_insert(value); }
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
		/** Attempts to insert a sequence of values (of element_t) into the set.
		 * If same values are already present within the set, does not replace them.
		 * @param first Iterator to the start of the value sequence.
		 * @param first Iterator to the end of the value sequence.
		 * @return Amount of elements inserted. */
		template<std::forward_iterator Iterator>
		constexpr size_type try_insert(Iterator first, Iterator last)
		{
			return m_table.try_insert(first, last);
		}
		/** Attempts to insert a sequence of values (of element_t) specified by the initializer list into the set.
		 * If same values are already present within the set, does not replace them.
		 * @param il Initializer list containing the values.
		 * @return Amount of elements inserted. */
		constexpr size_type try_insert(std::initializer_list<value_type> il)
		{
			return try_insert(il.begin(), il.end());
		}

		/** Inserts a value (of element_t) into the set.
		 * If the same value is already present within the set, replaces that value.
		 * @param value Value to insert.
		 * @return Pair where first element is the iterator to the inserted element
		 * and second is boolean indicating whether the element was inserted or replaced (`true` if inserted new, `false` if replaced). */
		constexpr std::pair<iterator, bool> insert(value_type &&value)
		{
			return m_table.insert(std::forward<value_type>(value));
		}
		/** @copydoc insert */
		constexpr std::pair<iterator, bool> insert(const value_type &value) { return m_table.insert(value); }
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
		/** Inserts a sequence of values (of element_t) into the set.
		 * If same values are already present within the set, replaces them.
		 * @param first Iterator to the start of the value sequence.
		 * @param first Iterator to the end of the value sequence.
		 * @return Amount of *new* elements inserted. */
		template<std::forward_iterator Iterator>
		constexpr size_type insert(Iterator first, Iterator last)
		{
			return m_table.insert(first, last);
		}
		/** Inserts a sequence of values (of element_t) specified by the initializer list into the set.
		 * If same values are already present within the set, replaces them.
		 * @param il Initializer list containing the values.
		 * @return Amount of new elements inserted. */
		constexpr size_type insert(std::initializer_list<value_type> il) { return insert(il.begin(), il.end()); }

		/** Removes the specified element from the set.
		 * @param where Iterator to the target element.
		 * @return Iterator to the element after the erased one. */
		constexpr iterator erase(const_iterator where) { return m_table.erase(where); }
		/** Removes all elements in the [first, last) range.
		 * @param first Iterator to the first element of the target range.
		 * @param last Iterator to the last element of the target range.
		 * @return Iterator to the element after the erased sequence. */
		constexpr iterator erase(const_iterator first, const_iterator last) { return m_table.erase(first, last); }
		/** Removes the specified element from the set if it is present.
		 * @param value Value of the target element.
		 * @return `true` if the element was removed, `false` otherwise. */
		constexpr bool erase(const key_type &value)
		{
			if (auto target = m_table.find(value); target != m_table.end())
			{
				m_table.erase(target);
				return true;
			}
			else
				return false;
		}
		/** @copydoc erase
		 * @note This overload participates in overload resolution only
		 * if both key hasher and key comparator are transparent. */
		constexpr bool erase(const auto &value)
			requires transparent_key
		{
			if (auto target = m_table.find(value); m_table.end() != target)
			{
				m_table.erase(target);
				return true;
			}
			else
				return false;
		}

		/** Returns current amount of elements in the set. */
		[[nodiscard]] constexpr size_type size() const noexcept { return m_table.size(); }
		/** Returns current capacity of the set. */
		[[nodiscard]] constexpr size_type capacity() const noexcept { return m_table.capacity(); }
		/** Returns maximum possible amount of elements in the set. */
		[[nodiscard]] constexpr size_type max_size() const noexcept { return m_table.max_size(); }
		/** Checks if the set is empty. */
		[[nodiscard]] constexpr bool empty() const noexcept { return size() == 0; }

		/** Returns current amount of buckets in the set. */
		[[nodiscard]] constexpr size_type bucket_count() const noexcept { return m_table.bucket_count(); }
		/** Returns the maximum amount of buckets. */
		[[nodiscard]] constexpr size_type max_bucket_count() const noexcept { return m_table.max_bucket_count(); }

		/** Returns local iterator to the start of a bucket. */
		[[nodiscard]] constexpr local_iterator begin(size_type bucket) const noexcept { return m_table.begin(bucket); }
		/** Returns const local iterator to the start of a bucket. */
		[[nodiscard]] constexpr const_local_iterator cbegin(size_type bucket) const noexcept
		{
			return m_table.cbegin(bucket);
		}
		/** Returns local iterator to the end of a bucket. */
		[[nodiscard]] constexpr local_iterator end(size_type bucket) const noexcept { return m_table.end(bucket); }
		/** Returns const local iterator to the end of a bucket. */
		[[nodiscard]] constexpr const_local_iterator cend(size_type bucket) const noexcept
		{
			return m_table.cend(bucket);
		}

		/** Returns the amount of elements stored within the bucket. */
		[[nodiscard]] constexpr size_type bucket_size(size_type bucket) const noexcept
		{
			return m_table.bucket_size(bucket);
		}
		/** Returns the index of the bucket associated with a key. */
		[[nodiscard]] constexpr size_type bucket(const key_type &key) const noexcept { return m_table.bucket(key); }
		/** @copydoc bucket
		 * @note This overload participates in overload resolution only
		 * if both key hasher and key comparator are transparent. */
		[[nodiscard]] constexpr size_type bucket(const auto &key) const noexcept
			requires transparent_key
		{
			return m_table.bucket(key);
		}
		/** Returns the index of the bucket containing the pointed-to element. */
		[[nodiscard]] constexpr size_type bucket(const_iterator iter) const noexcept { return m_table.bucket(iter); }

		/** Returns current load factor of the set. */
		[[nodiscard]] constexpr auto load_factor() const noexcept { return m_table.load_factor(); }
		/** Returns current max load factor of the set. */
		[[nodiscard]] constexpr auto max_load_factor() const noexcept { return m_table.max_load_factor; }
		/** Sets current max load factor of the set. */
		constexpr void max_load_factor(float f) noexcept
		{
			SEK_ASSERT(f > .0f);
			m_table.max_load_factor = f;
		}

		[[nodiscard]] constexpr allocator_type get_allocator() const noexcept { return m_table.allocator(); }

		[[nodiscard]] constexpr hash_type hash_function() const noexcept { return m_table.get_hash(); }
		[[nodiscard]] constexpr key_equal key_eq() const noexcept { return m_table.get_comp(); }

		[[nodiscard]] constexpr bool operator==(const ordered_set &other) const noexcept
			requires(requires(const_iterator a, const_iterator b) { std::equal_to<>{}(*a, *b); })
		{
			return std::is_permutation(begin(), end(), other.begin(), other.end());
		}

		constexpr void swap(ordered_set &other) noexcept { m_table.swap(other.m_table); }
		friend constexpr void swap(ordered_set &a, ordered_set &b) noexcept { a.swap(b); }

	private:
		/** Hash table used to implement the set. */
		table_type m_table;
	};
}	 // namespace sek