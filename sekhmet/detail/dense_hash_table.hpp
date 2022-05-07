//
// Created by switchblade on 07/05/22.
//

#pragma once

#include <vector>

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
	template<typename KeyType, typename ValueType, typename KeyExtract>
	struct dense_table_bucket : ebo_base_helper<ValueType>
	{
		using ebo_base = ebo_base_helper<ValueType>;

		constexpr dense_table_bucket() = default;
		constexpr dense_table_bucket(const dense_table_bucket &) = default;
		constexpr dense_table_bucket &operator=(const dense_table_bucket &) = default;
		constexpr dense_table_bucket(dense_table_bucket &&) = default;
		constexpr dense_table_bucket &operator=(dense_table_bucket &&) = default;
		constexpr ~dense_table_bucket() = default;

		[[nodiscard]] constexpr ValueType &value() const noexcept { return *ebo_base::get(); }
		[[nodiscard]] constexpr const KeyType &key() const noexcept { return KeyExtract{}(value()); }

		/* Offset of the next bucket in the dense array. */
		std::size_t next;
		/* Hash of the key. Cached by the bucket to avoid re-calculating hashes & allow for approximate comparison. */
		hash_t hash;
	};

	template<typename KeyType, typename ValueType, typename KeyHash, typename KeyCompare, typename KeyExtract, typename Allocator>
	class dense_hash_table
	{
	public:
		typedef KeyType key_type;
		typedef ValueType value_type;
		typedef KeyHash hash_type;
		typedef value_type *pointer;
		typedef const value_type *const_pointer;
		typedef value_type &reference;
		typedef const value_type &const_reference;
		typedef std::size_t size_type;
		typedef std::ptrdiff_t difference_type;

	private:
		using sparse_alloc = typename std::allocator_traits<Allocator>::template rebind_alloc<size_type>;
		using sparse_data_t = std::vector<size_type, sparse_alloc>;

		using bucket_t = dense_table_bucket<key_type, value_type, KeyExtract>;
		using dense_alloc = typename std::allocator_traits<Allocator>::template rebind_alloc<bucket_t>;
		using dense_data_t = std::vector<bucket_t, dense_alloc>;

		template<typename Iter>
		class dense_table_iterator
		{
			template<bool B>
			friend class dense_table_iterator;

			friend class dense_hash_table;

			constexpr static auto is_const = std::is_const_v<typename std::iterator_traits<Iter>::value_type>;

		public:
			typedef std::conditional_t<is_const, const ValueType, ValueType> value_type;
			typedef value_type *pointer;
			typedef value_type &reference;
			typedef std::size_t size_type;
			typedef typename std::iterator_traits<Iter>::difference_type difference_type;
			typedef typename std::iterator_traits<Iter>::iterator_category iterator_category;

		private:
			constexpr explicit dense_table_iterator(Iter i) noexcept : i(i) {}

		public:
			constexpr dense_table_iterator() noexcept = default;
			template<typename OtherIter, typename = std::enable_if_t<is_const && !dense_table_iterator<OtherIter>::is_const>>
			constexpr dense_table_iterator(const dense_table_iterator<OtherIter> &other) noexcept
				: dense_table_iterator(other.i)
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
				++i;
				return *this;
			}
			constexpr dense_table_iterator &operator+=(difference_type n) noexcept
			{
				i += n;
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
				--i;
				return *this;
			}
			constexpr dense_table_iterator &operator-=(difference_type n) noexcept
			{
				i -= n;
				return *this;
			}

			constexpr dense_table_iterator operator+(difference_type n) const noexcept
			{
				return dense_table_iterator{i + n};
			}
			constexpr dense_table_iterator operator-(difference_type n) const noexcept
			{
				return dense_table_iterator{i - n};
			}
			constexpr difference_type operator-(const dense_table_iterator &other) const noexcept
			{
				return i - other.i;
			}

			/** Returns pointer to the target element. */
			[[nodiscard]] constexpr pointer get() const noexcept { return &i->value(); }
			/** @copydoc value */
			[[nodiscard]] constexpr pointer operator->() const noexcept { return get(); }

			/** Returns reference to the element at an offset. */
			[[nodiscard]] constexpr reference operator[](difference_type n) const noexcept { return i[n].value(); }
			/** Returns reference to the target element. */
			[[nodiscard]] constexpr reference operator*() const noexcept { return *get(); }

			[[nodiscard]] constexpr auto operator<=>(const dense_table_iterator &) const noexcept = default;
			[[nodiscard]] constexpr bool operator==(const dense_table_iterator &) const noexcept = default;

			friend constexpr void swap(dense_table_iterator &a, dense_table_iterator &b) noexcept
			{
				using std::swap;
				swap(a.i, b.i);
			}

		private:
			Iter i;
		};

		constexpr static float initial_load_factor = .875f;
		constexpr static size_type initial_capacity = 8;

	public:
		typedef dense_table_iterator<typename dense_data_t::iterator> iterator;
		typedef dense_table_iterator<typename dense_data_t::const_iterator> const_iterator;
		typedef std::reverse_iterator<iterator> reverse_iterator;
		typedef std::reverse_iterator<const_iterator> const_reverse_iterator;

	private:
		[[nodiscard]] constexpr static bucket_t &to_bucket(iterator it) noexcept { return *std::to_address(it.i); }
		[[nodiscard]] constexpr static const bucket_t &to_bucket(const_iterator it) noexcept
		{
			return *std::to_address(it.i);
		}

	public:
		constexpr dense_hash_table() = default;
		constexpr dense_hash_table(const dense_hash_table &) = default;
		constexpr dense_hash_table &operator=(const dense_hash_table &) = default;
		constexpr dense_hash_table(dense_hash_table &&) = default;
		constexpr dense_hash_table &operator=(dense_hash_table &&) = default;
		constexpr ~dense_hash_table() = default;

		constexpr iterator erase(const_iterator where)
		{
			auto chain_idx = to_bucket(where.i).hash % index_vector().size();

		}

		[[nodiscard]] constexpr auto &bucket_vector() noexcept { return dense_data.first(); }
		[[nodiscard]] constexpr const auto &bucket_vector() const noexcept { return dense_data.first(); }
		[[nodiscard]] constexpr auto bucket_allocator() const noexcept { return bucket_vector().get_allocator(); }

		[[nodiscard]] constexpr auto &index_vector() noexcept { return sparse_data.first(); }
		[[nodiscard]] constexpr const auto &index_vector() const noexcept { return sparse_data.first(); }
		[[nodiscard]] constexpr auto index_allocator() const noexcept { return index_vector().get_allocator(); }

	private:
		constexpr auto key_hash(const key_type &k) { return sparse_data.second()(k); }
		constexpr auto key_comp(const key_type &a, const key_type &b) { return dense_data.second()(a, b); }

		packed_pair<sparse_data_t, hash_type> sparse_data;
		packed_pair<dense_data_t, KeyCompare> dense_data;

	public:
		float max_load_factor = initial_load_factor;
	};
}	 // namespace sek::detail