/*
 * Created by switchblade on 26/08/22
 */

#pragma once

#include "dense_hash_table.hpp"

namespace sek::detail
{
	/* Ordered hash tables are similar in design to dense hash tables, additionally preserving insertion order of
	 * elements. This functionality allows the user to efficiently preform key-based operations, while keeping
	 * track of insertion order of elements without the need for external ordering. Internally, every element of
	 * the table maintains a bidirectional linked list of element insertion order. */
	template<typename Key, typename Value, typename Traits, typename Hash, typename Cmp, typename KeyGet, typename Alloc>
	class ordered_hash_table : dense_table_traits<Value, Hash, Cmp, KeyGet>
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

		struct entry_node
		{
			constexpr entry_node() noexcept = default;
			constexpr entry_node(const entry_node &) noexcept = default;
			constexpr entry_node &operator=(const entry_node &) noexcept = default;

			constexpr entry_node(entry_node &&other) noexcept : prev(other.prev), next(other.next) { relink(); }
			constexpr entry_node &operator=(entry_node &&other) noexcept
			{
				prev = other.prev;
				next = other.next;
				return relink();
			}

			constexpr entry_node(entry_node *prev, entry_node *next) noexcept : prev(prev), next(next) {}

			constexpr entry_node &link_before(entry_node *next_ptr) noexcept { return link(next_ptr->prev, next_ptr); }
			constexpr entry_node &link(entry_node *prev_ptr, entry_node *next_ptr) noexcept
			{
				prev = prev_ptr;
				next = next_ptr;
				prev_ptr->next = this;
				next_ptr->prev = this;
				return *this;
			}
			constexpr entry_node &relink() noexcept
			{
				next->prev = this;
				prev->next = this;
				return *this;
			}
			constexpr entry_node &unlink() noexcept
			{
				next->prev = prev;
				prev->next = next;
				return *this;
			}

			constexpr void swap(entry_node &other) noexcept
			{
				std::swap(prev, other.prev);
				std::swap(next, other.next);
			}
			constexpr void swap_head(entry_node &other) noexcept
			{
				const auto old_next = next;
				const auto old_prev = prev;
				const auto other_next = other.next;
				const auto other_prev = other.prev;

				/* If `next` & `prev` do not point to `this`, give them to `other`. */
				if (old_next != this) [[likely]]
					other.next = old_next;
				if (old_prev != this) [[likely]]
					other.prev = old_prev;

				/* If `other.next` & `other.prev` do not point to `other`, take them. */
				if (other_next != &other) [[likely]]
					next = other_next;
				if (other_prev != &other) [[likely]]
					prev = other_prev;
			}

			entry_node *prev = nullptr;
			entry_node *next = nullptr;
		};
		class entry_type : public entry_node, private table_traits::entry_type
		{
			using entry_base = typename table_traits::entry_type;
			using node_base = entry_node;

		public:
			constexpr entry_type() = default;
			constexpr entry_type(const entry_type &) = default;
			constexpr entry_type &operator=(const entry_type &) = default;
			constexpr entry_type(entry_type &&) noexcept(std::is_nothrow_move_constructible_v<entry_base>) = default;
			constexpr entry_type &operator=(entry_type &&) noexcept(std::is_nothrow_move_assignable_v<entry_base>) = default;
			constexpr ~entry_type() = default;

			// clang-format off
			template<typename... Args>
			constexpr explicit entry_type(Args &&...args) requires std::constructible_from<Value, Args...>
				: entry_base(std::forward<Args>(args)...)
			{
			}
			// clang-format on

			template<typename... Args>
			constexpr explicit entry_type(Args &&...args) : entry_base(std::forward<Args>(args)...)
			{
			}
			template<typename... Args>
			constexpr entry_type(entry_node &n, hash_t h, Args &&...args) : entry_base(std::forward<Args>(args)...)
			{
				node_base::link_before(&n);
				entry_base::hash = h;
			}

			using entry_base::bucket_next;
			using entry_base::hash;
			using entry_base::key;
			using entry_base::value;

			constexpr void swap(entry_type &other) noexcept(std::is_nothrow_swappable_v<entry_base>)
			{
				node_base::swap(other);
				entry_base::swap(other);
			}
			friend constexpr void swap(entry_type &a, entry_type &b) noexcept(std::is_nothrow_swappable_v<entry_type>)
			{
				a.swap(b);
			}
		};

		using sparse_alloc = typename std::allocator_traits<Alloc>::template rebind_alloc<size_type>;
		using sparse_data = std::vector<size_type, sparse_alloc>;
		using dense_alloc = typename std::allocator_traits<Alloc>::template rebind_alloc<entry_type>;
		using dense_data = std::vector<entry_type, dense_alloc>;

		template<bool IsConst>
		class ordered_table_iterator
		{
			template<bool>
			friend class ordered_table_iterator;
			friend class ordered_hash_table;

			using iter_t = std::conditional_t<IsConst, typename dense_data::const_iterator, typename dense_data::iterator>;
			using ptr_t = std::conditional_t<IsConst, const entry_node, entry_node> *;

		public:
			typedef typename Traits::value_type value_type;
			typedef std::conditional_t<IsConst, typename Traits::const_pointer, typename Traits::pointer> pointer;
			typedef std::conditional_t<IsConst, typename Traits::const_reference, typename Traits::reference> reference;
			typedef std::size_t size_type;
			typedef std::ptrdiff_t difference_type;
			typedef std::bidirectional_iterator_tag iterator_category;

		private:
			constexpr explicit ordered_table_iterator(iter_t iter) noexcept : m_ptr(std::to_address(iter)) {}
			constexpr explicit ordered_table_iterator(ptr_t ptr) noexcept : m_ptr(ptr) {}

		public:
			constexpr ordered_table_iterator() noexcept = default;
			template<bool OtherConst, typename = std::enable_if_t<IsConst && !OtherConst>>
			constexpr ordered_table_iterator(const ordered_table_iterator<OtherConst> &other) noexcept
				: ordered_table_iterator(other.m_ptr)
			{
			}

			constexpr ordered_table_iterator operator++(int) noexcept
			{
				auto temp = *this;
				++(*this);
				return temp;
			}
			constexpr ordered_table_iterator &operator++() noexcept
			{
				m_ptr = m_ptr->next;
				return *this;
			}
			constexpr ordered_table_iterator operator--(int) noexcept
			{
				auto temp = *this;
				--(*this);
				return temp;
			}
			constexpr ordered_table_iterator &operator--() noexcept
			{
				m_ptr = m_ptr->prev;
				return *this;
			}

			/** Returns pointer to the target element. */
			[[nodiscard]] constexpr pointer get() const noexcept { return pointer{std::addressof(entry()->value)}; }
			/** @copydoc value */
			[[nodiscard]] constexpr pointer operator->() const noexcept { return get(); }
			/** Returns reference to the target element. */
			[[nodiscard]] constexpr reference operator*() const noexcept { return *get(); }

			[[nodiscard]] constexpr auto operator<=>(const ordered_table_iterator &) const noexcept = default;
			[[nodiscard]] constexpr bool operator==(const ordered_table_iterator &) const noexcept = default;

			constexpr void swap(ordered_table_iterator &other) noexcept { std::swap(m_ptr, other.m_ptr); }
			friend constexpr void swap(ordered_table_iterator &a, ordered_table_iterator &b) noexcept { a.swap(b); }

		private:
			[[nodiscard]] constexpr auto *entry() const noexcept
			{
				using entry_ptr = std::conditional_t<IsConst, const entry_type, entry_type> *;
				return static_cast<entry_ptr>(m_ptr);
			}

			ptr_t m_ptr = {};
		};
		template<bool IsConst>
		class ordered_table_bucket_iterator
		{
			template<bool>
			friend class ordered_table_bucket_iterator;
			friend class ordered_hash_table;

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
			constexpr explicit ordered_table_bucket_iterator(ptr_t ptr) noexcept : m_ptr(ptr) {}
			constexpr explicit ordered_table_bucket_iterator(iter_t iter) noexcept : m_ptr(std::to_address(iter)) {}
			constexpr explicit ordered_table_bucket_iterator(iter_t iter, size_type off) noexcept
				: m_ptr(std::to_address(iter)), m_off(off)
			{
			}

		public:
			constexpr ordered_table_bucket_iterator() noexcept = default;
			template<bool OtherConst, typename = std::enable_if_t<IsConst && OtherConst>>
			constexpr ordered_table_bucket_iterator(const ordered_table_bucket_iterator<OtherConst> &other) noexcept
				: ordered_table_bucket_iterator(other.m_ptr, other.m_off)
			{
			}

			constexpr ordered_table_bucket_iterator operator++(int) noexcept
			{
				auto temp = *this;
				++(*this);
				return temp;
			}
			constexpr ordered_table_bucket_iterator &operator++() noexcept
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

			[[nodiscard]] constexpr bool operator==(const ordered_table_bucket_iterator &) const noexcept = default;

			constexpr void swap(ordered_table_bucket_iterator &other) noexcept
			{
				using std::swap;
				swap(m_ptr, other.m_ptr);
				swap(m_off, other.m_off);
			}
			friend constexpr void swap(ordered_table_bucket_iterator &a, ordered_table_bucket_iterator &b) noexcept
			{
				a.swap(b);
			}

		private:
			ptr_t m_ptr = {};
			size_type m_off = 0;
		};

	public:
		typedef ordered_table_iterator<false> iterator;
		typedef ordered_table_iterator<true> const_iterator;
		typedef std::reverse_iterator<iterator> reverse_iterator;
		typedef std::reverse_iterator<const_iterator> const_reverse_iterator;
		typedef ordered_table_bucket_iterator<false> local_iterator;
		typedef ordered_table_bucket_iterator<true> const_local_iterator;

	public:
		constexpr ordered_hash_table() = default;
		constexpr ~ordered_hash_table() = default;

		constexpr explicit ordered_hash_table(const Alloc &alloc) : ordered_hash_table{Cmp{}, Hash{}, alloc} {}
		constexpr ordered_hash_table(const Cmp &equal, const Hash &hash, const Alloc &alloc)
			: ordered_hash_table{initial_capacity, equal, hash, alloc}
		{
		}
		constexpr ordered_hash_table(size_type bucket_count, const Cmp &equal, const Hash &hash, const Alloc &alloc)
			: m_dense{dense_alloc{alloc}, equal},
			  m_sparse{std::piecewise_construct,
					   std::forward_as_tuple(bucket_count, npos, sparse_alloc{alloc}),
					   std::forward_as_tuple(hash)}
		{
		}

		constexpr ordered_hash_table(const ordered_hash_table &other)
			: m_dense{other.m_dense}, m_sparse{other.m_sparse}, max_load_factor{other.max_load_factor}
		{
			rebase_nodes(other);
		}
		constexpr ordered_hash_table &operator=(const ordered_hash_table &other)
		{
			if (this != &other)
			{
				m_dense = other.m_dense;
				m_sparse = other.m_sparse;
				max_load_factor = other.max_load_factor;
				rebase_nodes(other);
			}
			return *this;
		}

		constexpr ordered_hash_table(const ordered_hash_table &other, const Alloc &alloc)
			: m_dense{std::piecewise_construct,
					  std::forward_as_tuple(other.value_vector(), dense_alloc{alloc}),
					  std::forward_as_tuple(other.m_dense.second())},
			  m_sparse{std::piecewise_construct,
					   std::forward_as_tuple(other.bucket_vector(), sparse_alloc{alloc}),
					   std::forward_as_tuple(other.m_sparse.second())},
			  max_load_factor{other.max_load_factor}
		{
			rebase_nodes(other);
		}

		constexpr ordered_hash_table(ordered_hash_table &&) = default;
		constexpr ordered_hash_table &operator=(ordered_hash_table &&) = default;

		constexpr ordered_hash_table(ordered_hash_table &&other, const Alloc &alloc)
			: m_dense{std::piecewise_construct,
					  std::forward_as_tuple(std::move(other.value_vector()), dense_alloc{alloc}),
					  std::forward_as_tuple(std::move(other.m_dense.second()))},
			  m_sparse{std::piecewise_construct,
					   std::forward_as_tuple(std::move(other.bucket_vector()), sparse_alloc{alloc}),
					   std::forward_as_tuple(std::move(other.m_sparse.second()))},
			  max_load_factor{other.max_load_factor}
		{
		}

		[[nodiscard]] constexpr auto begin() noexcept { return iterator{m_head.next}; }
		[[nodiscard]] constexpr auto cbegin() const noexcept { return const_iterator{m_head.next}; }
		[[nodiscard]] constexpr auto begin() const noexcept { return cbegin(); }
		[[nodiscard]] constexpr auto end() noexcept { return iterator{&m_head}; }
		[[nodiscard]] constexpr auto cend() const noexcept { return const_iterator{&m_head}; }
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
			return to_iterator(find_impl(key_hash(key), key));
		}
		[[nodiscard]] constexpr auto find(const auto &key) const noexcept
		{
			return to_iterator(find_impl(key_hash(key), key));
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
			const auto old_size = size();
			auto &entry = value_vector().emplace_back(std::forward<Args>(args)...);

			/* Try to find an existing entry with the same key. */
			const auto h = entry.hash = key_hash(entry.key());
			auto *chain_idx = get_chain(h);
			while (*chain_idx != npos)
				if (auto &candidate = value_vector()[*chain_idx];
					candidate.hash == h && key_comp(entry.key(), candidate.key()))
				{
					/* Found a candidate for replacing. Move-assign it and remove the temporary. */
					candidate = std::move(entry);
					value_vector().pop_back();
					return {iterator{&candidate}, false};
				}
				else
					chain_idx = &candidate.bucket_next;

			/* No suitable entry for replacing was found, add new link. */
			const auto pos = *chain_idx = old_size;
			entry.link_before(&m_head);
			maybe_rehash();
			return {iterator{&entry}, true};
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
			auto result = end();
			while (first < last) result = erase(first);
			return result;
		}
		constexpr auto erase(const_iterator where) { return erase_impl(where.entry()->hash, get_key(*where.get())); }

		// clang-format off
		template<typename T>
		constexpr auto erase(const T &key) requires(!std::same_as<std::decay_t<const_iterator>, T> &&
		                                            !std::same_as<std::decay_t<iterator>, T>)
		{
			return erase_impl(key_hash(key), key);
		}
		// clang-format on

		[[nodiscard]] constexpr auto allocator() const noexcept { return value_vector().get_allocator(); }
		[[nodiscard]] constexpr auto &get_hash() const noexcept { return m_sparse.second(); }
		[[nodiscard]] constexpr auto &get_comp() const noexcept { return m_dense.second(); }

		constexpr void swap(ordered_hash_table &other) noexcept
		{
			using std::swap;
			swap(m_sparse, other.m_sparse);
			swap(m_dense, other.m_dense);
			swap(max_load_factor, other.max_load_factor);

			/* Head nodes should be swapped in-place. */
			m_head.swap_head(other.m_head);
		}

	private:
		[[nodiscard]] constexpr auto &value_vector() noexcept { return m_dense.first(); }
		[[nodiscard]] constexpr const auto &value_vector() const noexcept { return m_dense.first(); }
		[[nodiscard]] constexpr auto &bucket_vector() noexcept { return m_sparse.first(); }
		[[nodiscard]] constexpr const auto &bucket_vector() const noexcept { return m_sparse.first(); }

		constexpr void rebase_nodes(const ordered_hash_table &other)
		{
			for (const auto diff = value_vector().begin() - other.value_vector().begin(); entry_node & node : value_vector())
			{
				/* Rebase next pointer. */
				if (node.next != &other.m_head) [[likely]]
					node.next += diff;
				else
				{
					node.next = &m_head;
					m_head.prev = &node;
				}

				/* Rebase previous pointer. */
				if (node.prev != &other.m_head) [[likely]]
					node.prev += diff;
				else
				{
					node.prev = &m_head;
					m_head.next = &node;
				}
			}
		}

		[[nodiscard]] constexpr auto key_hash(const auto &k) const { return m_sparse.second()(k); }
		[[nodiscard]] constexpr auto key_comp(const auto &a, const auto &b) const { return m_dense.second()(a, b); }
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

		[[nodiscard]] constexpr auto to_iterator(auto idx) noexcept
		{
			return iterator{value_vector().data() + static_cast<difference_type>(idx)};
		}
		[[nodiscard]] constexpr auto to_iterator(auto idx) const noexcept
		{
			return const_iterator{value_vector().data() + static_cast<difference_type>(idx)};
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
			value_vector().emplace_back(m_head, h, std::forward<Args>(args)...);
			maybe_rehash();
			return to_iterator(pos);
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
					return {to_iterator(*chain_idx), false};
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
					return {to_iterator(*chain_idx), false};
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

			/* Go through each entry & re-insert it. No need to re-link insertion order, as we are not moving elements. */
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

				if (entry_ptr->hash == h && key_comp(key, entry_ptr->key()))
				{
					/* Unlink the entry from the bucket and insertion order. */
					auto old_next = entry_ptr->next;
					*chain_idx = entry_ptr->bucket_next;
					entry_ptr->unlink();

					/* If the entry is not at the end, replace it with the last entry. */
					if (const auto end_pos = size() - 1; pos != end_pos)
					{
						/* Move & re-link the last entry. */
						const auto back_ptr = &value_vector().back();
						*entry_ptr = std::move(*back_ptr);

						/* Find the chain offset pointing to the old position & replace it with the new position. */
						for (chain_idx = get_chain(entry_ptr->hash); *chain_idx != npos;
							 chain_idx = &value_vector()[*chain_idx].bucket_next)
							if (*chain_idx == end_pos)
							{
								*chain_idx = pos;
								break;
							}

						/* If the back pointer is the same as old_next, update old_next. */
						if (old_next == back_ptr) [[unlikely]]
							old_next = entry_ptr;
					}

					value_vector().pop_back();
					return iterator{old_next};
				}
				chain_idx = &entry_ptr->bucket_next;
			}
			return end();
		}

		packed_pair<dense_data, Cmp> m_dense;
		packed_pair<sparse_data, Hash> m_sparse = {sparse_data(initial_capacity, npos), Hash{}};

		entry_node m_head = {&m_head, &m_head};

	public:
		float max_load_factor = initial_load_factor;
	};
}	 // namespace sek::detail