/*
 * ============================================================================
 * Sekhmet - C++20 game engine & editor
 * Copyright (C) 2022 switchblade
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 * ============================================================================
 *
 * Created by switchblade on 07/05/22
 */

#pragma once

#include <vector>

#include "assert.hpp"
#include "packed_pair.hpp"

namespace sek::detail
{
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
	template<typename KeyType, typename ValueType, typename ValueTraits, typename KeyHash, typename KeyCompare, typename KeyExtract, typename Allocator>
	class dense_hash_table
	{
	public:
		typedef KeyType key_type;
		typedef ValueType value_type;
		typedef KeyCompare key_equal;
		typedef KeyHash hash_type;
		typedef value_type *pointer;
		typedef const value_type *const_pointer;
		typedef value_type &reference;
		typedef const value_type &const_reference;
		typedef std::size_t size_type;
		typedef std::ptrdiff_t difference_type;

	private:
		template<bool Const>
		using iterator_value = typename ValueTraits::template iterator_value<Const>;
		template<bool Const>
		using iterator_pointer = typename ValueTraits::template iterator_pointer<Const>;
		template<bool Const>
		using iterator_reference = typename ValueTraits::template iterator_reference<Const>;

		using sparse_alloc = typename std::allocator_traits<Allocator>::template rebind_alloc<size_type>;
		using sparse_data_t = std::vector<size_type, sparse_alloc>;

		constexpr static decltype(auto) get_key(const auto &v) { return KeyExtract{}(v); }

		constexpr static float initial_load_factor = .875f;
		constexpr static size_type initial_capacity = 8;
		constexpr static size_type npos = std::numeric_limits<size_type>::max();

		struct entry_type : ebo_base_helper<ValueType>
		{
			using ebo_base = ebo_base_helper<ValueType>;

			constexpr entry_type() = default;
			constexpr entry_type(const entry_type &) = default;
			constexpr entry_type &operator=(const entry_type &) = default;
			constexpr entry_type(entry_type &&) noexcept(std::is_nothrow_move_constructible_v<ebo_base>) = default;
			constexpr entry_type &operator=(entry_type &&) noexcept(std::is_nothrow_move_assignable_v<ebo_base>) = default;
			constexpr ~entry_type() = default;

			template<typename... Args>
			constexpr explicit entry_type(Args &&...args) : ebo_base(std::forward<Args>(args)...)
			{
			}

			[[nodiscard]] constexpr value_type &value() noexcept { return *ebo_base::get(); }
			[[nodiscard]] constexpr const value_type &value() const noexcept { return *ebo_base::get(); }
			[[nodiscard]] constexpr decltype(auto) key() const noexcept { return get_key(value()); }

			constexpr void swap(entry_type &other) noexcept(std::is_nothrow_swappable_v<ebo_base>)
			{
				using std::swap;
				ebo_base::swap(other);
				swap(next, other.next);
				swap(hash, other.hash);
			}

			/* Offset of the next bucket in the dense array. */
			size_type next = npos;
			/* Hash of the key. Cached by the entry to avoid re-calculating hashes & allow for approximate comparison. */
			hash_t hash;
		};

		using dense_alloc = typename std::allocator_traits<Allocator>::template rebind_alloc<entry_type>;
		using dense_data_t = std::vector<entry_type, dense_alloc>;

		template<bool IsConst>
		class dense_table_iterator
		{
			template<bool>
			friend class dense_table_iterator;

			friend class dense_hash_table;

			using iter_t = std::conditional_t<IsConst, typename dense_data_t::const_iterator, typename dense_data_t::iterator>;
			using ptr_t = typename std::iterator_traits<iter_t>::pointer;

		public:
			typedef iterator_value<IsConst> value_type;
			typedef iterator_pointer<IsConst> pointer;
			typedef iterator_reference<IsConst> reference;
			typedef std::size_t size_type;
			typedef std::ptrdiff_t difference_type;
			typedef std::random_access_iterator_tag iterator_category;

		private:
			constexpr explicit dense_table_iterator(ptr_t ptr) noexcept : m_ptr(ptr) {}
			constexpr explicit dense_table_iterator(iter_t iter) noexcept : m_ptr(std::to_address(iter)) {}

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

			constexpr dense_table_iterator operator+(difference_type n) const noexcept
			{
				return dense_table_iterator{m_ptr + n};
			}
			constexpr dense_table_iterator operator-(difference_type n) const noexcept
			{
				return dense_table_iterator{m_ptr - n};
			}
			constexpr difference_type operator-(const dense_table_iterator &other) const noexcept
			{
				return m_ptr - other.m_ptr;
			}

			/** Returns pointer to the target element. */
			[[nodiscard]] constexpr pointer get() const noexcept { return pointer{&m_ptr->value()}; }
			/** @copydoc value */
			[[nodiscard]] constexpr pointer operator->() const noexcept { return get(); }

			/** Returns reference to the element at an offset. */
			[[nodiscard]] constexpr reference operator[](difference_type n) const noexcept { return m_ptr[n].value(); }
			/** Returns reference to the target element. */
			[[nodiscard]] constexpr reference operator*() const noexcept { return *get(); }

			[[nodiscard]] constexpr auto operator<=>(const dense_table_iterator &) const noexcept = default;
			[[nodiscard]] constexpr bool operator==(const dense_table_iterator &) const noexcept = default;

			constexpr void swap(dense_table_iterator &other) noexcept { std::swap(m_ptr, other.m_ptr); }
			friend constexpr void swap(dense_table_iterator &a, dense_table_iterator &b) noexcept { a.swap(b); }

		private:
			ptr_t m_ptr;
		};

		template<bool IsConst>
		class dense_table_bucket_iterator
		{
			template<bool>
			friend class dense_table_bucket_iterator;

			friend class dense_hash_table;

			using iter_t = std::conditional_t<IsConst, typename dense_data_t::const_iterator, typename dense_data_t::iterator>;
			using ptr_t = typename std::iterator_traits<iter_t>::pointer;

		public:
			typedef iterator_value<IsConst> value_type;
			typedef iterator_pointer<IsConst> pointer;
			typedef iterator_reference<IsConst> reference;
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
				m_off = m_ptr[static_cast<difference_type>(m_off)].next;
				return *this;
			}

			/** Returns pointer to the target element. */
			[[nodiscard]] constexpr pointer get() const noexcept { return pointer{&m_ptr->value()}; }
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
			ptr_t m_ptr;
			size_type m_off = 0;
		};

	public:
		typedef dense_table_iterator<false> iterator;
		typedef dense_table_iterator<true> const_iterator;
		typedef std::reverse_iterator<iterator> reverse_iterator;
		typedef std::reverse_iterator<const_iterator> const_reverse_iterator;
		typedef dense_table_bucket_iterator<false> local_iterator;
		typedef dense_table_bucket_iterator<true> const_local_iterator;

		typedef dense_alloc value_allocator_type;
		typedef sparse_alloc bucket_allocator_type;

	public:
		constexpr dense_hash_table() = default;
		constexpr dense_hash_table(const dense_hash_table &) = default;
		constexpr dense_hash_table &operator=(const dense_hash_table &) = default;
		constexpr dense_hash_table(dense_hash_table &&) = default;
		constexpr dense_hash_table &operator=(dense_hash_table &&) = default;
		constexpr ~dense_hash_table() = default;

		constexpr explicit dense_hash_table(const value_allocator_type &value_alloc)
			: dense_hash_table{key_equal{}, hash_type{}, value_alloc, bucket_allocator_type{}}
		{
		}
		constexpr dense_hash_table(const key_equal &equal,
								   const hash_type &hash,
								   const value_allocator_type &value_alloc,
								   const bucket_allocator_type &bucket_alloc)
			: dense_hash_table{initial_capacity, equal, hash, value_alloc, bucket_alloc}
		{
		}
		constexpr dense_hash_table(size_type bucket_count,
								   const key_equal &equal,
								   const hash_type &hash,
								   const value_allocator_type &value_alloc,
								   const bucket_allocator_type &bucket_alloc)
			: m_dense_data{value_alloc, equal},
			  m_sparse_data{std::piecewise_construct,
							std::forward_as_tuple(bucket_count, npos, bucket_alloc),
							std::forward_as_tuple(hash)}
		{
		}
		constexpr dense_hash_table(const dense_hash_table &other, const value_allocator_type &value_alloc)
			: m_dense_data{std::piecewise_construct,
						   std::forward_as_tuple(other.value_vector(), value_alloc),
						   std::forward_as_tuple(other.m_dense_data.second())},
			  m_sparse_data{other.m_sparse_data},
			  max_load_factor{other.max_load_factor}
		{
		}
		constexpr dense_hash_table(const dense_hash_table &other,
								   const value_allocator_type &value_alloc,
								   const bucket_allocator_type &bucket_alloc)
			: m_dense_data{std::piecewise_construct,
						   std::forward_as_tuple(other.value_vector(), value_alloc),
						   std::forward_as_tuple(other.m_dense_data.second())},
			  m_sparse_data{std::piecewise_construct,
							std::forward_as_tuple(other.bucket_vector(), bucket_alloc),
							std::forward_as_tuple(other.m_sparse_data.second())},
			  max_load_factor{other.max_load_factor}
		{
		}

		constexpr dense_hash_table(dense_hash_table &&other, const value_allocator_type &value_alloc)
			: m_dense_data{std::piecewise_construct,
						   std::forward_as_tuple(std::move(other.value_vector()), value_alloc),
						   std::forward_as_tuple(std::move(other.m_dense_data.second()))},
			  m_sparse_data{std::move(other.m_sparse_data)},
			  max_load_factor{other.max_load_factor}
		{
		}
		constexpr dense_hash_table(dense_hash_table &&other,
								   const value_allocator_type &value_alloc,
								   const bucket_allocator_type &bucket_alloc)
			: m_dense_data{std::piecewise_construct,
						   std::forward_as_tuple(std::move(other.value_vector()), value_alloc),
						   std::forward_as_tuple(std::move(other.m_dense_data.second()))},
			  m_sparse_data{std::piecewise_construct,
							std::forward_as_tuple(std::move(other.bucket_vector()), bucket_alloc),
							std::forward_as_tuple(std::move(other.m_sparse_data.second()))},
			  max_load_factor{other.max_load_factor}
		{
		}

		[[nodiscard]] constexpr iterator begin() noexcept { return iterator{value_vector().begin()}; }
		[[nodiscard]] constexpr const_iterator cbegin() const noexcept
		{
			return const_iterator{value_vector().begin()};
		}
		[[nodiscard]] constexpr const_iterator begin() const noexcept { return cbegin(); }

		[[nodiscard]] constexpr iterator end() noexcept { return iterator{value_vector().end()}; }
		[[nodiscard]] constexpr const_iterator cend() const noexcept { return const_iterator{value_vector().end()}; }
		[[nodiscard]] constexpr const_iterator end() const noexcept { return cend(); }

		[[nodiscard]] constexpr reverse_iterator rbegin() noexcept { return reverse_iterator{end()}; }
		[[nodiscard]] constexpr const_reverse_iterator crbegin() const noexcept
		{
			return const_reverse_iterator{cend()};
		}
		[[nodiscard]] constexpr const_reverse_iterator rbegin() const noexcept { return crbegin(); }

		[[nodiscard]] constexpr reverse_iterator rend() noexcept { return reverse_iterator{begin()}; }
		[[nodiscard]] constexpr const_reverse_iterator crend() const noexcept
		{
			return const_reverse_iterator{cbegin()};
		}
		[[nodiscard]] constexpr const_reverse_iterator rend() const noexcept { return crend(); }

		[[nodiscard]] constexpr size_type size() const noexcept { return value_vector().size(); }
		[[nodiscard]] constexpr size_type capacity() const noexcept
		{
			/* Capacity needs to take into account the max load factor. */
			return static_cast<size_type>(static_cast<float>(bucket_count()) * max_load_factor);
		}
		[[nodiscard]] constexpr size_type max_size() const noexcept
		{
			/* Max size cannot exceed max load factor of max entry capacity. */
			return static_cast<size_type>(static_cast<float>(value_vector().max_size()) * max_load_factor);
		}
		[[nodiscard]] constexpr float load_factor() const noexcept
		{
			return static_cast<float>(size()) / static_cast<float>(bucket_count());
		}

		[[nodiscard]] constexpr size_type bucket_count() const noexcept { return bucket_vector().size(); }
		[[nodiscard]] constexpr size_type max_bucket_count() const noexcept { return bucket_vector().max_size(); }

		[[nodiscard]] constexpr local_iterator begin(size_type bucket) noexcept
		{
			return local_iterator{value_vector().begin(), bucket};
		}
		[[nodiscard]] constexpr const_local_iterator cbegin(size_type bucket) const noexcept
		{
			return const_local_iterator{value_vector().begin(), bucket};
		}
		[[nodiscard]] constexpr const_local_iterator begin(size_type bucket) const noexcept { return cbegin(bucket); }

		[[nodiscard]] constexpr local_iterator end(size_type) noexcept
		{
			return local_iterator{value_vector().begin(), npos};
		}
		[[nodiscard]] constexpr const_local_iterator cend(size_type) const noexcept
		{
			return const_local_iterator{value_vector().begin(), npos};
		}
		[[nodiscard]] constexpr const_local_iterator end(size_type bucket) const noexcept { return cend(bucket); }

		[[nodiscard]] constexpr size_type bucket_size(size_type bucket) const noexcept
		{
			return static_cast<size_type>(std::distance(begin(bucket), end(bucket)));
		}
		[[nodiscard]] constexpr size_type bucket(const auto &key) const noexcept { return *get_chain(key_hash(key)); }
		[[nodiscard]] constexpr size_type bucket(const_iterator iter) const noexcept
		{
			return *get_chain(iter.m_ptr->hash);
		}

		[[nodiscard]] constexpr iterator find(const auto &key) noexcept
		{
			return begin() + static_cast<difference_type>(find_impl(key_hash(key), key));
		}
		[[nodiscard]] constexpr const_iterator find(const auto &key) const noexcept
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
			/* Adjust the capacity to be at least large enough to fit the current size. */
			new_cap = math::max(static_cast<size_type>(static_cast<float>(size()) / max_load_factor), new_cap, initial_capacity);

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
					/* Found a candidate for replacing. Move-assign or swap it from the temporary back entry. */
					if constexpr (std::is_move_assignable_v<entry_type>)
						candidate = std::move(value_vector().back());
					else
					{
						using std::swap;
						swap(candidate, value_vector().back());
					}
					value_vector().pop_back(); /* Pop the temporary. */
					return {begin() + static_cast<difference_type>(*chain_idx), false};
				}
				else
					chain_idx = &candidate.next;

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
		constexpr std::pair<iterator, bool> try_emplace(key_type &&key, Args &&...args)
		{
			// clang-format off
			return try_insert_impl(key, std::piecewise_construct,
								   std::forward_as_tuple(std::forward<key_type>(key)),
								   std::forward_as_tuple(std::forward<Args>(args)...));
			// clang-format on
		}

		template<std::forward_iterator Iter>
		constexpr size_type insert(Iter first, Iter last)
		{
			size_type inserted = 0;
			while (first != last) inserted += insert(*first++).second;
			return inserted;
		}
		constexpr std::pair<iterator, bool> insert(const value_type &value)
		{
			return insert_impl(get_key(value), value);
		}
		constexpr std::pair<iterator, bool> insert(value_type &&value)
		{
			return insert_impl(get_key(value), std::forward<value_type>(value));
		}

		template<std::forward_iterator Iter>
		constexpr size_type try_insert(Iter first, Iter last)
		{
			size_type inserted = 0;
			while (first != last) inserted += try_insert(*first++).second;
			return inserted;
		}
		constexpr std::pair<iterator, bool> try_insert(const value_type &value)
		{
			return try_insert_impl(get_key(value), value);
		}
		constexpr std::pair<iterator, bool> try_insert(value_type &&value)
		{
			return try_insert_impl(get_key(value), std::forward<value_type>(value));
		}

		constexpr iterator erase(const_iterator first, const_iterator last)
		{
			/* Iterate backwards here, since iterators after the erased one can be invalidated. */
			iterator result = end();
			while (first < last) result = erase(--last);
			return result;
		}
		constexpr iterator erase(const_iterator where) { return erase_impl(where.m_ptr->hash, get_key(*where.get())); }

		// clang-format off
		template<typename T>
		constexpr iterator erase(const T &key) requires(!std::same_as<std::decay_t<const_iterator>, T> &&
		                                                !std::same_as<std::decay_t<iterator>, T>)
		{
			return erase_impl(key_hash(key), key);
		}
		// clang-format on

		[[nodiscard]] constexpr auto value_allocator() const noexcept { return value_vector().get_allocator(); }
		[[nodiscard]] constexpr auto bucket_allocator() const noexcept { return bucket_vector().get_allocator(); }
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
					idx = &entry.next;
			return value_vector().size();
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
					if constexpr (requires { candidate.value() = std::forward<T>(value); })
						candidate.value() = std::forward<T>(value);
					else
					{
						std::destroy_at(&candidate);
						std::construct_at(&candidate, std::forward<T>(value));
					}
					candidate.hash = h;
					return {begin() + static_cast<difference_type>(*chain_idx), false};
				}
				else
					chain_idx = &candidate.next;

			/* No candidate for replacing found, create new entry. */
			const auto pos = *chain_idx = size();
			value_vector().emplace_back(std::forward<T>(value)).hash = h;
			maybe_rehash();
			return {begin() + static_cast<difference_type>(pos), true};
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
					chain_idx = &existing.next;

			/* No existing entry found, create new entry. */
			const auto pos = *chain_idx = size();
			value_vector().emplace_back(std::forward<Args>(args)...).hash = h;
			maybe_rehash();
			return {begin() + static_cast<difference_type>(pos), true};
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
			for (std::size_t i = 0; i < value_vector().size(); ++i)
			{
				auto &entry = value_vector()[i];
				auto *chain_idx = get_chain(entry.hash);

				/* Will also handle cases where chain_idx is npos (empty chain). */
				entry.next = *chain_idx;
				*chain_idx = i;
			}
		}

		constexpr iterator erase_impl(hash_t h, const auto &key)
		{
			/* Remove the entry from it's chain. */
			for (auto *chain_idx = get_chain(h); *chain_idx != npos;)
			{
				const auto pos = *chain_idx;
				auto entry_iter = value_vector().begin() + static_cast<difference_type>(pos);

				/* Un-link the entry from the chain & swap with the last entry. */
				if (entry_iter->hash == h && key_comp(key, entry_iter->key()))
				{
					*chain_idx = entry_iter->next;
					if (const auto end_pos = size() - 1; pos != end_pos)
					{
						if constexpr (std::is_move_assignable_v<entry_type>)
							*entry_iter = std::move(value_vector().back());
						else
							entry_iter->swap(value_vector().back());

						/* Find the chain offset pointing to the old position & replace it with the new position. */
						for (chain_idx = get_chain(entry_iter->hash); *chain_idx != npos;
							 chain_idx = &value_vector()[*chain_idx].next)
							if (*chain_idx == end_pos)
							{
								*chain_idx = pos;
								break;
							}
					}

					value_vector().pop_back();
					return begin() + static_cast<difference_type>(pos);
				}
				chain_idx = &entry_iter->next;
			}
			return end();
		}

		packed_pair<dense_data_t, key_equal> m_dense_data;
		packed_pair<sparse_data_t, hash_type> m_sparse_data = {sparse_data_t(initial_capacity, npos), hash_type{}};

	public:
		float max_load_factor = initial_load_factor;
	};
}	 // namespace sek::detail