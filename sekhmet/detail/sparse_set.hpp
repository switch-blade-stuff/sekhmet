//
// Created by switchblade on 2021-12-11.
//

#pragma once

#include <iterator>

#include "sparse_hash_table.hpp"

namespace sek
{
	/** @brief Sparse table based set providing fast insertion & deletion, but higher memory overhead than a tree-based set.
	 *
	 * Sparse sets are implemented via an open-addressing hash table.
	 * This allows for efficient insertion & deletion at the expense of greater memory overhead.
	 * Sparse sets always retain iterator validity on erasure.
	 * Iterators are invalidated on insertion if a re-hash is required.
	 *
	 * @note Iteration over a sparse set is O(n), where n is the amount of buckets within the set.
	 * This comes from a requirement for iterators to be valid after erasure operations
	 * (thus bucket list may contain tombstones). In addition, de-referencing sparse set iterators requires one level of indirection,
	 * since the buckets do not contain set values themselves.
	 *
	 * @tparam T Type of objects stored in the set.
	 * @tparam KeyHash Functor used to generate hashes for keys. By default uses `default_hash` which calls static
	 * non-member `hash` function via ADL if available, otherwise invokes `std::hash`.
	 * @tparam KeyComp Predicate used to compare keys.
	 * @tparam Alloc Allocator used for the set. */
	template<typename T, typename KeyHash = default_hash, typename KeyComp = std::equal_to<T>, typename Alloc = std::allocator<T>>
	class sparse_set
	{
	public:
		typedef T key_type;
		typedef T value_type;

	private:
		struct ret_identity
		{
			template<typename U>
			constexpr decltype(auto) operator()(U &&val) const noexcept
			{
				return std::forward<U>(val);
			}
		};
		using table_type = detail::sparse_hash_table<T, value_type, KeyHash, KeyComp, ret_identity, Alloc>;

	public:
		typedef typename table_type::pointer pointer;
		typedef typename table_type::const_pointer const_pointer;
		typedef typename table_type::reference reference;
		typedef typename table_type::const_reference const_reference;
		typedef typename table_type::size_type size_type;
		typedef typename table_type::difference_type difference_type;

		typedef typename table_type::value_allocator_type allocator_type;
		typedef typename table_type::bucket_allocator_type bucket_allocator_type;
		typedef typename table_type::hash_type hash_type;

		typedef typename table_type::const_iterator iterator;
		typedef typename table_type::const_iterator const_iterator;
		typedef typename table_type::node_handle node_handle;

	public:
		constexpr sparse_set() noexcept(std::is_nothrow_default_constructible_v<table_type>) = default;
		constexpr ~sparse_set() = default;

		/** Constructs a set with the specified minimum capacity.
		 * @param capacity Capacity of the set.
		 * @param key_compare Key comparator.
		 * @param key_hash Key hasher.
		 * @param value_alloc Allocator used to allocate set's elements.
		 * @param bucket_alloc Allocator used to allocate set's internal bucket array. */
		constexpr explicit sparse_set(size_type capacity,
									  const KeyComp &key_compare = {},
									  const KeyHash &key_hash = {},
									  const allocator_type &value_alloc = {},
									  const bucket_allocator_type &bucket_alloc = {})
			: data_table(capacity, key_compare, key_hash, value_alloc, bucket_alloc)
		{
		}

		/** Constructs a set from a sequence of values.
		 * @param first Iterator to the start of the value sequence.
		 * @param first Iterator to the end of the value sequence.
		 * @param key_compare Key comparator.
		 * @param key_hash Key hasher.
		 * @param value_alloc Allocator used to allocate set's elements.
		 * @param bucket_alloc Allocator used to allocate set's internal bucket array. */
		template<std::random_access_iterator Iterator>
		constexpr sparse_set(Iterator first,
							 Iterator last,
							 const KeyComp &key_compare = {},
							 const KeyHash &key_hash = {},
							 const allocator_type &value_alloc = {},
							 const bucket_allocator_type &bucket_alloc = {})
			: sparse_set(static_cast<size_type>(std::distance(first, last)), key_compare, key_hash, value_alloc, bucket_alloc)
		{
			insert(first, last);
		}
		/** Constructs a set from a sequence of values.
		 * @param first Iterator to the start of the value sequence.
		 * @param first Iterator to the end of the value sequence.
		 * @param key_compare Key comparator.
		 * @param key_hash Key hasher.
		 * @param value_alloc Allocator used to allocate set's elements.
		 * @param bucket_alloc Allocator used to allocate set's internal bucket array. */
		template<std::forward_iterator Iterator>
		constexpr sparse_set(Iterator first,
							 Iterator last,
							 const KeyComp &key_compare = {},
							 const KeyHash &key_hash = {},
							 const allocator_type &value_alloc = {},
							 const bucket_allocator_type &bucket_alloc = {})
			: sparse_set(0, key_compare, key_hash, value_alloc, bucket_alloc)
		{
			insert(first, last);
		}
		/** Constructs a set from an initializer list.
		 * @param init_list Initializer list containing values.
		 * @param key_compare Key comparator.
		 * @param key_hash Key hasher.
		 * @param value_alloc Allocator used to allocate set's elements.
		 * @param bucket_alloc Allocator used to allocate set's internal bucket array. */
		constexpr sparse_set(std::initializer_list<value_type> init_list,
							 const KeyComp &key_compare = {},
							 const KeyHash &key_hash = {},
							 const allocator_type &value_alloc = {},
							 const bucket_allocator_type &bucket_alloc = {})
			: sparse_set(init_list.begin(), init_list.end(), key_compare, key_hash, value_alloc, bucket_alloc)
		{
		}

		/** Copy-constructs the set. Both allocators are copied via `select_on_container_copy_construction`.
		 * @param other Map to copy data and allocators from. */
		constexpr sparse_set(const sparse_set &other) noexcept(std::is_nothrow_copy_constructible_v<table_type>)
			: data_table(other.data_table)
		{
		}
		/** Copy-constructs the set. Bucket allocator is copied via `select_on_container_copy_construction`.
		 * @param other Map to copy data and bucket allocator from.
		 * @param value_alloc Allocator used to allocate set's elements. */
		constexpr sparse_set(const sparse_set &other, const allocator_type &value_alloc) noexcept(
			std::is_nothrow_constructible_v<table_type, const table_type &, const allocator_type &>)
			: data_table(other.data_table, value_alloc)
		{
		}
		/** Copy-constructs the set.
		 * @param other Map to copy data from.
		 * @param value_alloc Allocator used to allocate set's elements.
		 * @param bucket_alloc Allocator used to allocate set's internal bucket array. */
		constexpr sparse_set(const sparse_set &other, const allocator_type &value_alloc, const bucket_allocator_type &bucket_alloc) noexcept(
			std::is_nothrow_constructible_v<table_type, const table_type &, const allocator_type &, const bucket_allocator_type &>)
			: data_table(other.data_table, value_alloc, bucket_alloc)
		{
		}

		/** Move-constructs the set. Both allocators are move-constructed.
		 * @param other Map to move elements and allocators from. */
		constexpr sparse_set(sparse_set &&other) noexcept(std::is_nothrow_move_constructible_v<table_type>)
			: data_table(std::move(other.data_table))
		{
		}
		/** Move-constructs the set. Bucket allocator is move-constructed.
		 * @param other Map to move elements and bucket allocator from.
		 * @param value_alloc Allocator used to allocate set's elements. */
		constexpr sparse_set(sparse_set &&other, const allocator_type &value_alloc) noexcept(
			std::is_nothrow_constructible_v<table_type, table_type &&, const allocator_type &>)
			: data_table(std::move(other.data_table), value_alloc)
		{
		}
		/** Move-constructs the set.
		 * @param other Map to move elements from.
		 * @param value_alloc Allocator used to allocate set's elements.
		 * @param bucket_alloc Allocator used to allocate set's internal bucket array. */
		constexpr sparse_set(sparse_set &&other, const allocator_type &value_alloc, const bucket_allocator_type &bucket_alloc) noexcept(
			std::is_nothrow_constructible_v<table_type, table_type &&, const allocator_type &, const bucket_allocator_type &>)
			: data_table(std::move(other.data_table), value_alloc, bucket_alloc)
		{
		}

		/** Copy-assigns the set.
		 * @param other Map to copy elements from. */
		constexpr sparse_set &operator=(const sparse_set &other)
		{
			if (this != &other) data_table = other.data_table;
			return *this;
		}
		/** Move-assigns the set.
		 * @param other Map to move elements from. */
		constexpr sparse_set &operator=(sparse_set &&other) noexcept(std::is_nothrow_move_assignable_v<table_type>)
		{
			data_table = std::move(other.data_table);
			return *this;
		}

		/** Returns iterator to the start of the set. */
		[[nodiscard]] constexpr iterator begin() noexcept { return data_table.begin(); }
		/** Returns iterator to the end of the set. */
		[[nodiscard]] constexpr iterator end() noexcept { return data_table.end(); }
		/** Returns const iterator to the start of the set. */
		[[nodiscard]] constexpr const_iterator cbegin() const noexcept { return data_table.begin(); }
		/** Returns const iterator to the end of the set. */
		[[nodiscard]] constexpr const_iterator cend() const noexcept { return data_table.end(); }
		/** @copydoc cbegin */
		[[nodiscard]] constexpr const_iterator begin() const noexcept { return cbegin(); }
		/** @copydoc cend */
		[[nodiscard]] constexpr const_iterator end() const noexcept { return cend(); }

		/** Locates an element within the set.
		 * @param key Key to search for.
		 * @return Iterator to the element setped to key. */
		constexpr iterator find(const key_type &key) noexcept { return data_table.find(key); }
		/** @copydoc find */
		constexpr const_iterator find(const key_type &key) const noexcept { return data_table.find(key); }
		/** Checks if the set contains a specific element.
		 * @param key Key to search for. */
		constexpr bool contains(const key_type &key) const noexcept { return find(key) != end(); }

		/** Empties the set's contents. */
		constexpr void clear() { data_table.clear(); }

		/** Re-hashes the set for the specified minimal capacity. */
		constexpr void rehash(size_type capacity) { data_table.rehash(capacity); }
		/** Resizes the internal storage to have space for at least n elements. */
		constexpr void reserve(size_type n) { data_table.reserve(n); }

		/** Constructs a value (of value_type) in-place.
		 * If the same value is already present within the set, replaces that value.
		 * @param args Arguments used to construct the value object.
		 * @return Pair where first element is the iterator to the inserted element
		 * and second is boolean indicating whether the element was inserted or replace (`true` if inserted new, `false` if replaced). */
		template<typename... Args>
		constexpr std::pair<iterator, bool> emplace(Args &&...args)
		{
			return data_table.emplace(std::forward<Args>(args)...);
		}

		/** Attempts to insert a value into the set.
		 * If the same value is already present within the set, does not replace it.
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
		/** Attempts to insert a sequence of values (of value_type) into the set.
		 * If same values are already present within the set, does not replace them.
		 * @param first Iterator to the start of the value sequence.
		 * @param first Iterator to the end of the value sequence.
		 * @return Amount of elements inserted. */
		template<std::forward_iterator Iterator>
		constexpr size_type try_insert(Iterator first, Iterator last)
		{
			return data_table.try_insert(first, last);
		}
		/** Attempts to insert a sequence of values (of value_type) specified by the initializer list into the set.
		 * If same values are already present within the set, does not replace them.
		 * @param init_list Initializer list containing the values.
		 * @return Amount of elements inserted. */
		constexpr size_type try_insert(std::initializer_list<value_type> init_list)
		{
			return try_insert(init_list.begin(), init_list.end());
		}

		/** Inserts a value (of value_type) into the set.
		 * If the same value is already present within the set, replaces that value.
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
		/** Inserts a sequence of values (of value_type) into the set.
		 * If same values are already present within the set, replaces them.
		 * @param first Iterator to the start of the value sequence.
		 * @param first Iterator to the end of the value sequence.
		 * @return Amount of *new* elements inserted. */
		template<std::forward_iterator Iterator>
		constexpr size_type insert(Iterator first, Iterator last)
		{
			return data_table.insert(first, last);
		}
		/** Inserts a sequence of values (of value_type) specified by the initializer list into the set.
		 * If same values are already present within the set, replaces them.
		 * @param init_list Initializer list containing the values.
		 * @return Amount of new elements inserted. */
		constexpr size_type insert(std::initializer_list<value_type> init_list)
		{
			return insert(init_list.begin(), init_list.end());
		}

		/** Removes the specified elements from the set.
		 * @param where Iterator to the target element.
		 * @return Iterator to the element after the erased one. */
		constexpr iterator erase(const_iterator where) { return data_table.erase(where); }
		/** Removes all elements in the [first, last) range.
		 * @param first Iterator to the first element of the target range.
		 * @param last Iterator to the last element of the target range.
		 * @return Iterator to the element after the erased sequence. */
		constexpr iterator erase(const_iterator first, const_iterator last) { return data_table.erase(first, last); }
		/** Removes the specified element from the set if it is present.
		 * @param value Value of the target element.
		 * @return `true` if the element was removed, `false` otherwise. */
		constexpr bool erase(const key_type &value)
		{
			if (auto target = data_table.find(value); target != data_table.end())
			{
				data_table.erase(target);
				return true;
			}
			else
				return false;
		}

		/** Extracts the specified node from the set.
		 * @param where Iterator to the target node.
		 * @return Node handle to the extracted node. */
		constexpr node_handle extract(const_iterator where) { return data_table.extract_node(where); }
		/** Extracts the specified node from the set.
		 * @param key Key of the target node.
		 * @return Node handle to the extracted node or, if the key is not present, an empty node handle. */
		constexpr node_handle extract(const key_type &key)
		{
			if (auto target = data_table.find(key); target != data_table.end())
				return data_table.extract_node(target);
			else
				return {};
		}

		/** Inserts the specified node into the set.
		 * If the same value is already present within the set, replaces that value.
		 * @param node Node to insert.
		 * @return Pair where first element is the iterator to the inserted node
		 * and second is boolean indicating whether the node was inserted or replaced (`true` if inserted new, `false` if replaced). */
		constexpr std::pair<iterator, bool> insert(node_handle &&node)
		{
			return data_table.insert_node(std::forward<node_handle>(node));
		}
		/** @copydetails insert
		 * @param hint Hint for where to insert the node.
		 * @param node Node to insert.
		 * @return Iterator to the inserted node.
		 * @note Hint is required for compatibility with STL algorithms and is ignored. */
		constexpr iterator insert([[maybe_unused]] const_iterator hint, node_handle &&node)
		{
			return insert(std::forward<node_handle>(node)).first;
		}
		/** Attempts to insert the specified node into the set.
		 * If the same value is already present within the set, does not replace it.
		 * @param node Node to insert.
		 * @return Pair where first element is the iterator to the potentially inserted node
		 * and second is boolean indicating whether the node was inserted (`true` if inserted, `false` otherwise). */
		constexpr std::pair<iterator, bool> try_insert(node_handle &&node)
		{
			return data_table.try_insert_node(std::forward<node_handle>(node));
		}
		/** @copydetails try_insert
		 * @param hint Hint for where to insert the node.
		 * @param node Node to insert.
		 * @return Iterator to the potentially inserted node or the node that prevented insertion.
		 * @note Hint is required for compatibility with STL algorithms and is ignored. */
		constexpr iterator try_insert([[maybe_unused]] const_iterator hint, node_handle &&node)
		{
			return insert(std::forward<node_handle>(node)).first;
		}

		/** Returns current amount of elements in the set. */
		[[nodiscard]] constexpr size_type size() const noexcept { return data_table.size(); }
		/** Returns current capacity of the set. */
		[[nodiscard]] constexpr size_type capacity() const noexcept { return data_table.capacity(); }
		/** Returns maximum possible amount of elements in the set. */
		[[nodiscard]] constexpr size_type max_size() const noexcept { return data_table.max_size(); }
		/** Checks if the set is empty. */
		[[nodiscard]] constexpr size_type empty() const noexcept { return size() == 0; }
		/** Returns current amount of buckets in the set. */
		[[nodiscard]] constexpr size_type bucket_count() const noexcept { return data_table.bucket_count(); }

		/** Returns current load factor of the set. */
		[[nodiscard]] constexpr auto load_factor() const noexcept { return data_table.load_factor(); }
		/** Returns current max load factor of the set. */
		[[nodiscard]] constexpr auto max_load_factor() const noexcept { return data_table.max_load_factor; }
		/** Sets current max load factor of the set. */
		constexpr void max_load_factor(float f) noexcept { data_table.max_load_factor = f; }
		/** Returns current tombstone factor of the set. */
		[[nodiscard]] constexpr auto tombstone_factor() const noexcept { return data_table.tombstone_factor(); }
		/** Returns current max tombstone factor of the set. */
		[[nodiscard]] constexpr auto max_tombstone_factor() const noexcept { return data_table.max_tombstone_factor; }
		/** Sets current max tombstone factor of the set. */
		constexpr void max_tombstone_factor(float f) noexcept { data_table.max_tombstone_factor = f; }

		[[nodiscard]] constexpr allocator_type &get_allocator() noexcept { return data_table.get_value_allocator(); }
		[[nodiscard]] constexpr const allocator_type &get_allocator() const noexcept
		{
			return data_table.get_value_allocator();
		}
		[[nodiscard]] constexpr bucket_allocator_type &get_bucket_allocator() noexcept
		{
			return data_table.get_bucket_allocator();
		}
		[[nodiscard]] constexpr const bucket_allocator_type &get_bucket_allocator() const noexcept
		{
			return data_table.get_bucket_allocator();
		}

		[[nodiscard]] constexpr hash_type &hasher() noexcept { return data_table.get_hash(); }
		[[nodiscard]] constexpr const hash_type &hasher() const noexcept { return data_table.get_hash(); }

		[[nodiscard]] constexpr bool operator==(const sparse_set &other) const noexcept
		{
			return std::is_permutation(begin(), end(), other.begin(), other.end());
		}

		constexpr void swap(sparse_set &other) noexcept { data_table.swap(other.data_table); }
		friend constexpr void swap(sparse_set &a, sparse_set &b) noexcept { a.swap(b); }

	private:
		/** Hash table used to implement the set. */
		table_type data_table;
	};
}	 // namespace sek