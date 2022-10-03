/*
 * Created by switchblade on 07/05/22
 */

#pragma once

#include <limits>
#include <memory>
#include <utility>
#include <vector>

#include "../assert.hpp"
#include "packed_pair.hpp"

namespace sek::detail
{
	template<typename Value, typename KeyGet>
	class dense_table_entry
	{
	public:
		typedef std::size_t size_type;
		typedef std::ptrdiff_t difference_type;

		constexpr static size_type npos = std::numeric_limits<size_type>::max();

	public:
		constexpr dense_table_entry() = default;
		constexpr dense_table_entry(const dense_table_entry &) = default;
		constexpr dense_table_entry &operator=(const dense_table_entry &) = default;
		constexpr dense_table_entry(dense_table_entry &&) noexcept(std::is_nothrow_move_constructible_v<Value>) = default;
		constexpr dense_table_entry &operator=(dense_table_entry &&) noexcept(std::is_nothrow_move_assignable_v<Value>) = default;
		constexpr ~dense_table_entry() = default;

		// clang-format off
		template<typename... Args>
		constexpr explicit dense_table_entry(Args &&...args) requires std::constructible_from<Value, Args...>
			: value(std::forward<Args>(args)...)
		{
		}
		// clang-format on

		[[nodiscard]] constexpr decltype(auto) key() const noexcept { return KeyGet{}(value); }

		constexpr void swap(dense_table_entry &other) noexcept(std::is_nothrow_swappable_v<Value>)
		{
			using std::swap;

			swap(value, other.value);
			swap(bucket_next, other.bucket_next);
			swap(hash, other.hash);
		}
		friend constexpr void swap(dense_table_entry &a, dense_table_entry &b) noexcept(std::is_nothrow_swappable_v<Value>)
		{
			a.swap(b);
		}

		Value value;
		size_type bucket_next = npos;
		hash_t hash = {};
	};

	template<typename Value, typename Hash, typename Cmp, typename KeyGet>
	struct dense_table_traits
	{
		typedef Cmp key_equal;
		typedef Hash hash_type;

		typedef dense_table_entry<Value, KeyGet> entry_type;
		typedef std::size_t size_type;
		typedef std::ptrdiff_t difference_type;

		[[nodiscard]] constexpr static decltype(auto) get_key(const auto &v) { return KeyGet{}(v); }

		constexpr static float initial_load_factor = .875f;

		constexpr static size_type npos = std::numeric_limits<size_type>::max();
		constexpr static size_type initial_capacity = 8;
	};

	/* Dense hash tables are implemented via a sparse array of bucket indices & a dense array of buckets,
	 * which form a closed addressing table.
	 * This allows for cache-efficient iteration over the table (iterators point to the dense array elements),
	 * as well as reduced memory overhead, since there are no empty buckets within the dense array.
	 *
	 * However, dense tables cannot provide iterator stability on erasure or insertion.
	 * This is due to the buckets being stored in the dense array by-value,
	 * thus on erasure buckets must be moved (or rather, the erased bucket is swapped with the last one),
	 * and on insertion the dense array may be re-allocated.
	 *
	 * The sparse array contains indices into the dense array of buckets, thus the buckets can be freely moved,
	 * as long as the sparse index is updated accordingly.
	 *
	 * To solve bucket contention, every bucket contains an offset into the dense array,
	 * where the next bucket in a chain is located. Since buckets are appended on top of the dense array,
	 * these offsets are stable.
	 *
	 * When a bucket is erased (and is swapped with the last bucket in the dense array), the offset links pointing
	 * to the erased and the swapped-with bucket are also updated. In order to do so, the swapped-with bucket's
	 * chain is traversed and is updated accordingly.
	 *
	 * In order for this to not affect performance, the default load factor is set to be below 1. */
	template<typename Key, typename Value, typename Traits, typename Hash, typename Cmp, typename KeyGet, typename Alloc>
	class dense_hash_table : dense_table_traits<Value, Hash, Cmp, KeyGet>
	{
		using table_traits = dense_table_traits<Value, Hash, Cmp, KeyGet>;

	public:
		typedef typename table_traits::key_equal key_equal;
		typedef typename table_traits::hash_type hash_type;
		typedef typename table_traits::size_type size_type;
		typedef typename table_traits::difference_type difference_type;

	private:
		using table_traits::get_key;
		using table_traits::initial_capacity;
		using table_traits::initial_load_factor;
		using table_traits::npos;

		using entry_type = typename table_traits::entry_type;

		using sparse_alloc = typename std::allocator_traits<Alloc>::template rebind_alloc<size_type>;
		using sparse_data = std::vector<size_type, sparse_alloc>;
		using dense_alloc = typename std::allocator_traits<Alloc>::template rebind_alloc<entry_type>;
		using dense_data = std::vector<entry_type, dense_alloc>;

		template<bool IsConst>
		class dense_table_iterator
		{
			template<bool>
			friend class dense_table_iterator;
			friend class dense_hash_table;

			using iter_t = std::conditional_t<IsConst, typename dense_data::const_iterator, typename dense_data::iterator>;
			using ptr_t = std::conditional_t<IsConst, const entry_type, entry_type> *;

		public:
			typedef typename Traits::value_type value_type;
			typedef std::conditional_t<IsConst, typename Traits::const_pointer, typename Traits::pointer> pointer;
			typedef std::conditional_t<IsConst, typename Traits::const_reference, typename Traits::reference> reference;
			typedef std::size_t size_type;
			typedef std::ptrdiff_t difference_type;
			typedef std::random_access_iterator_tag iterator_category;

		private:
			constexpr explicit dense_table_iterator(iter_t iter) noexcept : m_ptr(std::to_address(iter)) {}
			constexpr explicit dense_table_iterator(ptr_t ptr) noexcept : m_ptr(ptr) {}

		public:
			constexpr dense_table_iterator() noexcept = default;
			template<bool OtherConst, typename = std::enable_if_t<IsConst && !OtherConst>>
			constexpr dense_table_iterator(const dense_table_iterator<OtherConst> &other) noexcept
				: dense_table_iterator(other.m_ptr)
			{
			}

			constexpr dense_table_iterator operator++(int) noexcept
			{
				auto temp = *this;
				++(*this);
				return temp;
			}
			constexpr dense_table_iterator &operator++() noexcept
			{
				++m_ptr;
				return *this;
			}
			constexpr dense_table_iterator &operator+=(difference_type n) noexcept
			{
				m_ptr += n;
				return *this;
			}
			constexpr dense_table_iterator operator--(int) noexcept
			{
				auto temp = *this;
				--(*this);
				return temp;
			}
			constexpr dense_table_iterator &operator--() noexcept
			{
				--m_ptr;
				return *this;
			}
			constexpr dense_table_iterator &operator-=(difference_type n) noexcept
			{
				m_ptr -= n;
				return *this;
			}

			[[nodiscard]] constexpr dense_table_iterator operator+(difference_type n) const noexcept
			{
				return dense_table_iterator{m_ptr + n};
			}
			[[nodiscard]] constexpr dense_table_iterator operator-(difference_type n) const noexcept
			{
				return dense_table_iterator{m_ptr - n};
			}
			[[nodiscard]] constexpr difference_type operator-(const dense_table_iterator &other) const noexcept
			{
				return m_ptr - other.m_ptr;
			}

			/** Returns pointer to the target element. */
			[[nodiscard]] constexpr pointer get() const noexcept { return pointer{std::addressof(m_ptr->value)}; }
			/** @copydoc value */
			[[nodiscard]] constexpr pointer operator->() const noexcept { return get(); }

			/** Returns reference to the element at an offset. */
			[[nodiscard]] constexpr reference operator[](difference_type n) const noexcept { return m_ptr[n].value; }
			/** Returns reference to the target element. */
			[[nodiscard]] constexpr reference operator*() const noexcept { return *get(); }

			[[nodiscard]] constexpr auto operator<=>(const dense_table_iterator &) const noexcept = default;
			[[nodiscard]] constexpr bool operator==(const dense_table_iterator &) const noexcept = default;

			constexpr void swap(dense_table_iterator &other) noexcept { std::swap(m_ptr, other.m_ptr); }
			friend constexpr void swap(dense_table_iterator &a, dense_table_iterator &b) noexcept { a.swap(b); }

		private:
			ptr_t m_ptr = {};
		};
		template<bool IsConst>
		class dense_table_bucket_iterator
		{
			template<bool>
			friend class dense_table_bucket_iterator;
			friend class dense_hash_table;

			using iter_t = std::conditional_t<IsConst, typename dense_data::const_iterator, typename dense_data::iterator>;
			using ptr_t = std::conditional_t<IsConst, const entry_type, entry_type> *;

		public:
			typedef typename Traits::value_type value_type;
			typedef std::conditional_t<IsConst, typename Traits::const_pointer, typename Traits::pointer> pointer;
			typedef std::conditional_t<IsConst, typename Traits::const_reference, typename Traits::reference> reference;
			typedef std::size_t size_type;
			typedef std::ptrdiff_t difference_type;
			typedef std::forward_iterator_tag iterator_category;

		private:
			constexpr explicit dense_table_bucket_iterator(ptr_t ptr) noexcept : m_ptr(ptr) {}
			constexpr explicit dense_table_bucket_iterator(iter_t iter) noexcept : m_ptr(std::to_address(iter)) {}
			constexpr explicit dense_table_bucket_iterator(iter_t iter, size_type off) noexcept
				: m_ptr(std::to_address(iter)), m_off(off)
			{
			}

		public:
			constexpr dense_table_bucket_iterator() noexcept = default;
			template<bool OtherConst, typename = std::enable_if_t<IsConst && OtherConst>>
			constexpr dense_table_bucket_iterator(const dense_table_bucket_iterator<OtherConst> &other) noexcept
				: dense_table_bucket_iterator(other.m_ptr, other.m_off)
			{
			}

			constexpr dense_table_bucket_iterator operator++(int) noexcept
			{
				auto temp = *this;
				++(*this);
				return temp;
			}
			constexpr dense_table_bucket_iterator &operator++() noexcept
			{
				m_off = m_ptr[static_cast<difference_type>(m_off)].bucket_next;
				return *this;
			}

			/** Returns pointer to the target element. */
			[[nodiscard]] constexpr pointer get() const noexcept { return pointer{std::addressof(m_ptr->value)}; }
			/** @copydoc value */
			[[nodiscard]] constexpr pointer operator->() const noexcept { return get(); }
			/** Returns reference to the target element. */
			[[nodiscard]] constexpr reference operator*() const noexcept { return *get(); }

			[[nodiscard]] constexpr bool operator==(const dense_table_bucket_iterator &) const noexcept = default;

			constexpr void swap(dense_table_bucket_iterator &other) noexcept
			{
				using std::swap;
				swap(m_ptr, other.m_ptr);
				swap(m_off, other.m_off);
			}
			friend constexpr void swap(dense_table_bucket_iterator &a, dense_table_bucket_iterator &b) noexcept
			{
				a.swap(b);
			}

		private:
			ptr_t m_ptr = {};
			size_type m_off = 0;
		};

	public:
		typedef dense_table_iterator<false> iterator;
		typedef dense_table_iterator<true> const_iterator;
		typedef std::reverse_iterator<iterator> reverse_iterator;
		typedef std::reverse_iterator<const_iterator> const_reverse_iterator;
		typedef dense_table_bucket_iterator<false> local_iterator;
		typedef dense_table_bucket_iterator<true> const_local_iterator;

	public:
		constexpr dense_hash_table() = default;
		constexpr dense_hash_table(const dense_hash_table &) = default;
		constexpr dense_hash_table &operator=(const dense_hash_table &) = default;
		constexpr dense_hash_table(dense_hash_table &&) = default;
		constexpr dense_hash_table &operator=(dense_hash_table &&) = default;
		constexpr ~dense_hash_table() = default;

		constexpr explicit dense_hash_table(const Alloc &alloc) : dense_hash_table{Cmp{}, Hash{}, alloc} {}
		constexpr dense_hash_table(const Cmp &equal, const Hash &hash, const Alloc &alloc)
			: dense_hash_table{initial_capacity, equal, hash, alloc}
		{
		}
		constexpr dense_hash_table(size_type bucket_count, const Cmp &equal, const Hash &hash, const Alloc &alloc)
			: m_dense_data{dense_alloc{alloc}, equal},
			  m_sparse_data{std::piecewise_construct,
							std::forward_as_tuple(bucket_count, npos, sparse_alloc{alloc}),
							std::forward_as_tuple(hash)}
		{
		}
		constexpr dense_hash_table(const dense_hash_table &other, const Alloc &alloc)
			: m_dense_data{std::piecewise_construct,
						   std::forward_as_tuple(other.value_vector(), dense_alloc{alloc}),
						   std::forward_as_tuple(other.m_dense_data.second())},
			  m_sparse_data{std::piecewise_construct,
							std::forward_as_tuple(other.bucket_vector(), sparse_alloc{alloc}),
							std::forward_as_tuple(other.m_sparse_data.second())},
			  max_load_factor{other.max_load_factor}
		{
		}

		constexpr dense_hash_table(dense_hash_table &&other, const Alloc &alloc)
			: m_dense_data{std::piecewise_construct,
						   std::forward_as_tuple(std::move(other.value_vector()), dense_alloc{alloc}),
						   std::forward_as_tuple(std::move(other.m_dense_data.second()))},
			  m_sparse_data{std::piecewise_construct,
							std::forward_as_tuple(std::move(other.bucket_vector()), sparse_alloc{alloc}),
							std::forward_as_tuple(std::move(other.m_sparse_data.second()))},
			  max_load_factor{other.max_load_factor}
		{
		}

		[[nodiscard]] constexpr auto begin() noexcept { return iterator{value_vector().begin()}; }
		[[nodiscard]] constexpr auto cbegin() const noexcept { return const_iterator{value_vector().begin()}; }
		[[nodiscard]] constexpr auto begin() const noexcept { return cbegin(); }
		[[nodiscard]] constexpr auto end() noexcept { return iterator{value_vector().end()}; }
		[[nodiscard]] constexpr auto cend() const noexcept { return const_iterator{value_vector().end()}; }
		[[nodiscard]] constexpr auto end() const noexcept { return cend(); }
		[[nodiscard]] constexpr auto rbegin() noexcept { return reverse_iterator{end()}; }
		[[nodiscard]] constexpr auto crbegin() const noexcept { return const_reverse_iterator{cend()}; }
		[[nodiscard]] constexpr auto rbegin() const noexcept { return crbegin(); }
		[[nodiscard]] constexpr auto rend() noexcept { return reverse_iterator{begin()}; }
		[[nodiscard]] constexpr auto crend() const noexcept { return const_reverse_iterator{cbegin()}; }
		[[nodiscard]] constexpr auto rend() const noexcept { return crend(); }

		[[nodiscard]] constexpr size_type size() const noexcept { return value_vector().size(); }
		[[nodiscard]] constexpr size_type capacity() const noexcept
		{
			/* Capacity needs to take into account the max load factor. */
			return static_cast<size_type>(static_cast<float>(bucket_count()) * max_load_factor);
		}
		[[nodiscard]] constexpr size_type max_size() const noexcept
		{
			const auto max_idx = std::min(value_vector().max_size(), npos - 1);
			return static_cast<size_type>(static_cast<float>(max_idx) * max_load_factor);
		}
		[[nodiscard]] constexpr float load_factor() const noexcept
		{
			return static_cast<float>(size()) / static_cast<float>(bucket_count());
		}

		[[nodiscard]] constexpr size_type bucket_count() const noexcept { return bucket_vector().size(); }
		[[nodiscard]] constexpr size_type max_bucket_count() const noexcept { return bucket_vector().max_size(); }

		[[nodiscard]] constexpr auto begin(size_type bucket) noexcept
		{
			return local_iterator{value_vector().begin(), bucket};
		}
		[[nodiscard]] constexpr auto cbegin(size_type bucket) const noexcept
		{
			return const_local_iterator{value_vector().begin(), bucket};
		}
		[[nodiscard]] constexpr auto begin(size_type bucket) const noexcept { return cbegin(bucket); }
		[[nodiscard]] constexpr auto end(size_type) noexcept { return local_iterator{value_vector().begin(), npos}; }
		[[nodiscard]] constexpr auto cend(size_type) const noexcept
		{
			return const_local_iterator{value_vector().begin(), npos};
		}
		[[nodiscard]] constexpr auto end(size_type bucket) const noexcept { return cend(bucket); }

		[[nodiscard]] constexpr size_type bucket_size(size_type bucket) const noexcept
		{
			return static_cast<size_type>(std::distance(begin(bucket), end(bucket)));
		}
		[[nodiscard]] constexpr size_type bucket(const auto &key) const noexcept { return *get_chain(key_hash(key)); }
		[[nodiscard]] constexpr size_type bucket(const_iterator iter) const noexcept
		{
			return *get_chain(iter.m_ptr->hash);
		}

		[[nodiscard]] constexpr auto find(const auto &key) noexcept
		{
			return iterator{value_vector().begin() + static_cast<difference_type>(find_impl(key_hash(key), key))};
		}
		[[nodiscard]] constexpr auto find(const auto &key) const noexcept
		{
			return cbegin() + static_cast<difference_type>(find_impl(key_hash(key), key));
		}

		constexpr void clear()
		{
			std::fill_n(bucket_vector().data(), bucket_count(), npos);
			value_vector().clear();
		}

		constexpr void rehash(size_type new_cap)
		{
			using std::max;

			/* Adjust the capacity to be at least large enough to fit the current size. */
			new_cap = max(max(static_cast<size_type>(static_cast<float>(size()) / max_load_factor), new_cap), initial_capacity);

			/* Don't do anything if the capacity did not change after the adjustment. */
			if (new_cap != bucket_vector().capacity()) [[likely]]
				rehash_impl(new_cap);
		}
		constexpr void reserve(size_type n)
		{
			value_vector().reserve(n);
			rehash(static_cast<size_type>(static_cast<float>(n) / max_load_factor));
		}

		template<typename... Args>
		constexpr std::pair<iterator, bool> emplace(Args &&...args)
		{
			/* Temporary entry needs to be created at first. */
			auto &entry = value_vector().emplace_back(std::forward<Args>(args)...);
			const auto h = entry.hash = key_hash(entry.key());
			auto *chain_idx = get_chain(h);
			while (*chain_idx != npos)
				if (auto &candidate = value_vector()[*chain_idx];
					candidate.hash == h && key_comp(entry.key(), candidate.key()))
				{
					/* Found a candidate for replacing. */
					candidate = std::move(value_vector().back());
					value_vector().pop_back(); /* Pop the temporary. */
					return {begin() + static_cast<difference_type>(*chain_idx), false};
				}
				else
					chain_idx = &candidate.bucket_next;

			/* No suitable entry for replacing was found, add new link. */
			const auto pos = *chain_idx = size() - 1;
			maybe_rehash();
			return {begin() + static_cast<difference_type>(pos), true};
		}
		template<typename... Args>
		constexpr std::pair<iterator, bool> try_emplace(const auto &key, Args &&...args)
		{
			// clang-format off
			return try_insert_impl(key, std::piecewise_construct,
								   std::forward_as_tuple(key),
								   std::forward_as_tuple(std::forward<Args>(args)...));
			// clang-format on
		}
		template<typename... Args>
		constexpr std::pair<iterator, bool> try_emplace(Key &&key, Args &&...args)
		{
			// clang-format off
			return try_insert_impl(key, std::piecewise_construct,
								   std::forward_as_tuple(std::forward<Key>(key)),
								   std::forward_as_tuple(std::forward<Args>(args)...));
			// clang-format on
		}

		template<std::forward_iterator Iter>
		constexpr size_type insert(Iter first, Iter last)
		{
			size_type inserted = 0;
			while (first != last) inserted += emplace(*first++).second;
			return inserted;
		}
		constexpr size_type insert(iterator first, iterator last)
		{
			size_type inserted = 0;
			while (first != last) inserted += insert(*first++).second;
			return inserted;
		}
		constexpr size_type insert(const_iterator first, const_iterator last)
		{
			size_type inserted = 0;
			while (first != last) inserted += insert(*first++).second;
			return inserted;
		}

		constexpr std::pair<iterator, bool> insert(const Value &value) { return insert_impl(get_key(value), value); }
		constexpr std::pair<iterator, bool> insert(Value &&value)
		{
			return insert_impl(get_key(value), std::forward<Value>(value));
		}

		template<std::forward_iterator Iter>
		constexpr size_type try_insert(Iter first, Iter last)
		{
			size_type inserted = 0;
			while (first != last) inserted += try_insert(*first++).second;
			return inserted;
		}

		constexpr std::pair<iterator, bool> try_insert(const Value &value)
		{
			return try_insert_impl(get_key(value), value);
		}
		constexpr std::pair<iterator, bool> try_insert(Value &&value)
		{
			return try_insert_impl(get_key(value), std::forward<Value>(value));
		}

		constexpr auto erase(const_iterator first, const_iterator last)
		{
			/* Iterate backwards here, since iterators after the erased one can be invalidated. */
			auto result = end();
			while (first < last) result = erase(--last);
			return result;
		}
		constexpr auto erase(const_iterator where) { return erase_impl(where.m_ptr->hash, get_key(*where.get())); }

		// clang-format off
		template<typename T>
		constexpr auto erase(const T &key) requires(!std::same_as<std::decay_t<const_iterator>, T> &&
		                                            !std::same_as<std::decay_t<iterator>, T>)
		{
			return erase_impl(key_hash(key), key);
		}
		// clang-format on

		[[nodiscard]] constexpr auto allocator() const noexcept { return value_vector().get_allocator(); }
		[[nodiscard]] constexpr auto &get_hash() const noexcept { return m_sparse_data.second(); }
		[[nodiscard]] constexpr auto &get_comp() const noexcept { return m_dense_data.second(); }

		constexpr void swap(dense_hash_table &other) noexcept
		{
			using std::swap;
			swap(m_sparse_data, other.m_sparse_data);
			swap(m_dense_data, other.m_dense_data);
			swap(max_load_factor, other.max_load_factor);
		}

	private:
		[[nodiscard]] constexpr auto &value_vector() noexcept { return m_dense_data.first(); }
		[[nodiscard]] constexpr const auto &value_vector() const noexcept { return m_dense_data.first(); }
		[[nodiscard]] constexpr auto &bucket_vector() noexcept { return m_sparse_data.first(); }
		[[nodiscard]] constexpr const auto &bucket_vector() const noexcept { return m_sparse_data.first(); }

		[[nodiscard]] constexpr auto key_hash(const auto &k) const { return m_sparse_data.second()(k); }
		[[nodiscard]] constexpr auto key_comp(const auto &a, const auto &b) const
		{
			return m_dense_data.second()(a, b);
		}
		[[nodiscard]] constexpr auto *get_chain(hash_t h) noexcept
		{
			auto idx = h % bucket_vector().size();
			return bucket_vector().data() + idx;
		}
		[[nodiscard]] constexpr auto *get_chain(hash_t h) const noexcept
		{
			auto idx = h % bucket_vector().size();
			return bucket_vector().data() + idx;
		}

		[[nodiscard]] constexpr size_type find_impl(hash_t h, const auto &key) const noexcept
		{
			for (auto *idx = get_chain(h); *idx != npos;)
				if (auto &entry = value_vector()[*idx]; entry.hash == h && key_comp(key, entry.key()))
					return *idx;
				else
					idx = &entry.bucket_next;
			return value_vector().size();
		}

		template<typename... Args>
		[[nodiscard]] constexpr iterator insert_new(hash_t h, auto *chain_idx, Args &&...args) noexcept
		{
			const auto pos = *chain_idx = size();
			value_vector().emplace_back(std::forward<Args>(args)...).hash = h;
			maybe_rehash();

			return begin() + static_cast<difference_type>(pos);
		}
		template<typename T>
		[[nodiscard]] constexpr std::pair<iterator, bool> insert_impl(const auto &key, T &&value) noexcept
		{
			/* See if we can replace any entry. */
			const auto h = key_hash(key);
			auto *chain_idx = get_chain(h);
			while (*chain_idx != npos)
				if (auto &candidate = value_vector()[*chain_idx]; candidate.hash == h && key_comp(key, candidate.key()))
				{
					/* Found a candidate for replacing, replace the value & hash. */
					if constexpr (requires { candidate.value = std::forward<T>(value); })
						candidate.value = std::forward<T>(value);
					else
					{
						std::destroy_at(&candidate.value);
						std::construct_at(&candidate.value, std::forward<T>(value));
					}
					candidate.hash = h;
					return {begin() + static_cast<difference_type>(*chain_idx), false};
				}
				else
					chain_idx = &candidate.bucket_next;

			/* No candidate for replacing found, create new entry. */
			return {insert_new(h, chain_idx, std::forward<T>(value)), true};
		}
		template<typename... Args>
		[[nodiscard]] constexpr std::pair<iterator, bool> try_insert_impl(const auto &key, Args &&...args) noexcept
		{
			/* See if an entry already exists. */
			const auto h = key_hash(key);
			auto *chain_idx = get_chain(h);
			while (*chain_idx != npos)
				if (auto &existing = value_vector()[*chain_idx]; existing.hash == h && key_comp(key, existing.key()))
					return {begin() + static_cast<difference_type>(*chain_idx), false};
				else
					chain_idx = &existing.bucket_next;

			/* No existing entry found, create new entry. */
			return {insert_new(h, chain_idx, std::forward<Args>(args)...), true};
		}

		constexpr void maybe_rehash()
		{
			if (load_factor() > max_load_factor) [[unlikely]]
				rehash(bucket_count() * 2);
		}
		constexpr void rehash_impl(size_type new_cap)
		{
			/* Clear & reserve the vector filled with npos. */
			bucket_vector().clear();
			bucket_vector().resize(new_cap, npos);

			/* Go through each entry & re-insert it. */
			for (size_type i = 0; i < value_vector().size(); ++i)
			{
				auto &entry = value_vector()[i];
				auto *chain_idx = get_chain(entry.hash);

				/* Will also handle cases where chain_idx is npos (empty chain). */
				entry.bucket_next = *chain_idx;
				*chain_idx = i;
			}
		}

		constexpr auto erase_impl(hash_t h, const auto &key)
		{
			/* Remove the entry from its chain. */
			for (auto *chain_idx = get_chain(h); *chain_idx != npos;)
			{
				const auto pos = *chain_idx;
				auto entry_ptr = value_vector().data() + static_cast<difference_type>(pos);

				/* Un-link the entry from the chain & swap with the last entry. */
				if (entry_ptr->hash == h && key_comp(key, entry_ptr->key()))
				{
					*chain_idx = entry_ptr->bucket_next;
					if (const auto end_pos = size() - 1; pos != end_pos)
					{
						*entry_ptr = std::move(value_vector().back());

						/* Find the chain offset pointing to the old position & replace it with the new position. */
						for (chain_idx = get_chain(entry_ptr->hash); *chain_idx != npos;
							 chain_idx = &value_vector()[*chain_idx].bucket_next)
							if (*chain_idx == end_pos)
							{
								*chain_idx = pos;
								break;
							}
					}

					value_vector().pop_back();
					return begin() + static_cast<difference_type>(pos);
				}
				chain_idx = &entry_ptr->bucket_next;
			}
			return end();
		}

		packed_pair<dense_data, Cmp> m_dense_data;
		packed_pair<sparse_data, Hash> m_sparse_data = {sparse_data(initial_capacity, npos), Hash{}};

	public:
		float max_load_factor = initial_load_factor;
	};
}	 // namespace sek::detail