//
// Created by switchblade on 11/05/22.
//

#pragma once

#include <tuple>
#include <vector>

#include "assert.hpp"
#include "define.h"
#include "ebo_base_helper.hpp"
#include "hash.hpp"
#include "meta_util.hpp"
#include "table_util.hpp"

namespace sek
{
	/** @brief Helper type used to specify key, hash & comparator for a multimap key. */
	template<typename K, typename KeyHash = default_hash, typename KeyComp = std::equal_to<K>>
	struct key_t
	{
		using key_type = K;
		using hash_type = KeyHash;
		using key_equal = KeyComp;
	};

	/** @brief Structure used as a compound key type for multi-key map.
	 * @tparam Ks Template instances of `key_t` used to specify individual multi-key map keys. */
	template<template_type_instance<key_t>... Ks>
	class multikey
	{
		using data_t = std::tuple<typename Ks::key_type...>;

	public:
		template<std::size_t I>
		using key_type = type_seq_element_t<I, type_seq_t<typename Ks::key_type...>>;

		// clang-format off
		constexpr multikey() noexcept(noexcept(data_t{})) = default;
		constexpr multikey(const multikey &) noexcept(std::is_nothrow_copy_constructible_v<data_t>) = default;
		constexpr multikey &operator=(const multikey &) noexcept(std::is_nothrow_copy_assignable_v<data_t>) = default;
		constexpr multikey(multikey &&) noexcept(std::is_nothrow_move_constructible_v<data_t>) = default;
		constexpr multikey &operator=(multikey &&) noexcept(std::is_nothrow_move_assignable_v<data_t>) = default;
		constexpr ~multikey() = default;

		template<typename... Args>
		constexpr explicit multikey(Args &&...args) noexcept(std::is_nothrow_constructible_v<data_t, Args...>)
			: data{std::forward<Args>(args)...}
		{
		}
		// clang-format on

		/** Returns reference to the Ith key. */
		template<std::size_t I>
		[[nodiscard]] constexpr const key_type<I> &get() const noexcept
			requires(I < sizeof...(Ks))
		{
			return std::get<I>(data);
		}
		/** @copydoc get */
		template<std::size_t I>
		[[nodiscard]] friend constexpr const key_type<I> &get(const multikey &mk) noexcept
			requires(I < sizeof...(Ks))
		{
			return mk.get<I>();
		}

		[[nodiscard]] constexpr auto operator<=>(const multikey &) const = default;
		[[nodiscard]] constexpr bool operator==(const multikey &) const = default;

		constexpr void swap(multikey &other) noexcept(std::is_nothrow_swappable_v<data_t>)
		{
			std::swap(data, other.data);
		}
		friend constexpr void swap(multikey &a, multikey &b) noexcept(std::is_nothrow_swappable_v<data_t>)
		{
			a.swap(b);
		}

	private:
		data_t data;
	};

	template<typename, typename>
	struct mkmap_value;
	/** @brief Helper used to obtain a value type for a multikey map. */
	template<typename M, typename... Ks>
	struct mkmap_value<multikey<Ks...>, M>
	{
		using type = std::pair<multikey<Ks...>, M>;
	};

	template<template_type_instance<multikey> Multikey, typename Mapped>
	using mkmap_value_t = typename mkmap_value<Multikey, Mapped>::type;

	namespace detail
	{
		template<std::size_t, typename...>
		struct mkmap_key_select;
		template<std::size_t I, typename T>
		struct mkmap_key_select<I, T> : std::false_type
		{
		};
		template<std::size_t I, typename T, typename K, typename... Ks>
			requires std::constructible_from<K, T>
		struct mkmap_key_select<I, T, K, Ks...> : std::true_type
		{
			constexpr static std::size_t idx = I;
		};
		template<std::size_t I, typename T, typename K, typename... Ks>
		struct mkmap_key_select<I, T, K, Ks...> : mkmap_key_select<I + 1, T, Ks...>
		{
		};

		template<typename...>
		class mkmap_impl;

		template<std::size_t, typename T>
		class mkmap_ebo_base_wrapper : ebo_base_helper<T>
		{
			using ebo_base = ebo_base_helper<T>;

		public:
			// clang-format off
			constexpr mkmap_ebo_base_wrapper() noexcept(noexcept(ebo_base{})) = default;
			constexpr mkmap_ebo_base_wrapper(const mkmap_ebo_base_wrapper &)
				noexcept(std::is_nothrow_copy_constructible_v<ebo_base>) = default;
			constexpr mkmap_ebo_base_wrapper &operator=(const mkmap_ebo_base_wrapper &)
				noexcept(std::is_nothrow_copy_assignable_v<ebo_base>) = default;
			constexpr mkmap_ebo_base_wrapper(mkmap_ebo_base_wrapper &&)
				noexcept(std::is_nothrow_move_constructible_v<ebo_base>) = default;
			constexpr mkmap_ebo_base_wrapper &operator=(mkmap_ebo_base_wrapper &&)
				noexcept(std::is_nothrow_move_assignable_v<ebo_base>) = default;
			constexpr ~mkmap_ebo_base_wrapper() = default;

			template<typename... Args>
			constexpr explicit mkmap_ebo_base_wrapper(Args &&...args)
				noexcept(std::is_nothrow_constructible_v<ebo_base, Args...>)
				: ebo_base{std::forward<Args>(args)...}
			{
			}
			// clang-format on

			using ebo_base::get;
		};

		template<std::size_t, typename...>
		class mkmap_ebo_base
		{
		};
		template<std::size_t I, typename T, typename... Ts>
		class mkmap_ebo_base<I, T, Ts...> : mkmap_ebo_base_wrapper<I, T>, mkmap_ebo_base<I + 1, Ts...>
		{
			template<typename...>
			friend class mkmap_impl;

			using expand_base = mkmap_ebo_base<I + 1, Ts...>;
			using ebo_base = mkmap_ebo_base_wrapper<I, T>;

		public:
			// clang-format off
			constexpr mkmap_ebo_base() noexcept(noexcept(ebo_base{}) && noexcept(expand_base{})) = default;
			constexpr mkmap_ebo_base(const mkmap_ebo_base &)
				noexcept(std::is_nothrow_copy_constructible_v<ebo_base> &&
						 std::is_nothrow_copy_constructible_v<expand_base>) = default;
			constexpr mkmap_ebo_base &operator=(const mkmap_ebo_base &)
				noexcept(std::is_nothrow_copy_assignable_v<ebo_base> &&
						 std::is_nothrow_copy_assignable_v<expand_base>) = default;
			constexpr mkmap_ebo_base(mkmap_ebo_base &&)
				noexcept(std::is_nothrow_move_constructible_v<ebo_base> &&
				         std::is_nothrow_move_constructible_v<expand_base>) = default;
			constexpr mkmap_ebo_base &operator=(mkmap_ebo_base &&)
				noexcept(std::is_nothrow_move_assignable_v<ebo_base> &&
				         std::is_nothrow_move_assignable_v<expand_base>) = default;
			constexpr ~mkmap_ebo_base() = default;

			template<typename... Args>
			constexpr explicit mkmap_ebo_base(const T &val, Args &&...args)
				noexcept(std::is_nothrow_constructible_v<ebo_base, const T &> &&
						 std::is_nothrow_constructible_v<expand_base, Args...>)
				: ebo_base{val}, expand_base{std::forward<Args>(args)...}
			{
			}
			// clang-format on

		protected:
			template<std::size_t N>
			constexpr decltype(auto) get() const
			{
				if constexpr (N == I)
					return *ebo_base::get();
				else
					return expand_base::template get<N>();
			}
		};

		template<typename... Ks>
		using mkmap_hash_base = mkmap_ebo_base<0, typename Ks::hash_type...>;
		template<typename... Ks>
		using mkmap_comp_base = mkmap_ebo_base<0, typename Ks::key_equal...>;

		template<typename M, typename... Ks, typename Alloc>
		class mkmap_impl<multikey<Ks...>, M, Alloc> : mkmap_comp_base<Ks...>, mkmap_hash_base<Ks...>
		{
			static_assert(sizeof...(Ks) != 0, "Multi-key map must have at least 1 key type");

			using hash_base = mkmap_hash_base<Ks...>;
			using comp_base = mkmap_comp_base<Ks...>;
			using key_ts = type_seq_t<Ks...>;

		public:
			typedef M mapped_type;
			typedef mkmap_value_t<multikey<Ks...>, M> value_type;

			/** @brief Helper type used to explicitly select a particular key of a multimap. */
			template<std::size_t I>
			struct key_select_t
			{
				constexpr static auto key = I;
			};
			template<std::size_t I>
			constexpr static auto key_select = key_select_t<I>{};

			constexpr static auto key_count = sizeof...(Ks);

			template<std::size_t I = 0>
			using key_type = typename type_seq_element_t<I, key_ts>::key_type;
			template<std::size_t I = 0>
			using hash_type = typename type_seq_element_t<I, key_ts>::key_hash;
			template<std::size_t I = 0>
			using key_equal = typename type_seq_element_t<I, key_ts>::key_equal;

		private:
			constexpr static float initial_load_factor = .875f;
			constexpr static std::size_t initial_capacity = 8;
			constexpr static std::size_t npos = std::numeric_limits<std::size_t>::max();

			template<typename T>
			constexpr static auto key_index = mkmap_key_select<0, std::decay_t<T>, typename Ks::key_type...>::idx;
			template<typename T>
			constexpr static bool is_key = mkmap_key_select<0, std::decay_t<T>, typename Ks::key_type...>::value;

			template<typename F, typename... Args>
			constexpr static void foreach_key(F &&f, Args &&...args)
			{
				auto unwrap = [&]<size_type... Is>(std::index_sequence<Is...>, F && f, Args && ...fwd)
				{
					/* Unwrap this for every key. */
					(..., f(index_selector<Is>, std::forward<Args>(fwd)...));
				};
				unwrap(std::make_index_sequence<key_count>{}, std::forward<F>(f), std::forward<Args>(args)...);
			}
			template<std::size_t I>
			[[nodiscard]] constexpr static decltype(auto) get_key(const value_type &v)
				requires(I < key_count)
			{
				return v.first.template get<I>();
			}

			struct entry_type
			{
				struct sparse_link
				{
					std::size_t next = npos;
					hash_t hash;
				};

				constexpr entry_type() = default;
				constexpr entry_type(const entry_type &) = default;
				constexpr entry_type &operator=(const entry_type &) = default;
				constexpr entry_type(entry_type &&) noexcept(std::is_nothrow_move_constructible_v<value_type>) = default;
				constexpr entry_type &operator=(entry_type &&) noexcept(std::is_nothrow_move_assignable_v<value_type>) = default;
				constexpr ~entry_type() = default;

				template<typename... Args>
				constexpr explicit entry_type(Args &&...args) : value(std::forward<Args>(args)...)
				{
				}

				template<std::size_t I>
				[[nodiscard]] constexpr decltype(auto) key() const noexcept
				{
					return get_key<I>(value);
				}
				template<std::size_t I>
				[[nodiscard]] constexpr std::size_t &next() noexcept
				{
					return links[I].next;
				}
				template<std::size_t I>
				[[nodiscard]] constexpr const std::size_t &next() const noexcept
				{
					return links[I].next;
				}
				template<std::size_t I>
				[[nodiscard]] constexpr hash_t &hash() noexcept
				{
					return links[I].hash;
				}
				template<std::size_t I>
				[[nodiscard]] constexpr hash_t hash() const noexcept
				{
					return links[I].hash;
				}

				constexpr void swap(entry_type &other) noexcept(std::is_nothrow_swappable_v<value_type>)
				{
					using std::swap;
					swap(links, other.links);
					swap(value, other.value);
				}

				sparse_link links[key_count];
				value_type value;
			};

			struct bucket_type
			{
				constexpr bucket_type() noexcept { std::fill_n(offsets, SEK_ARRAY_SIZE(offsets), npos); }

				template<std::size_t I>
				[[nodiscard]] constexpr std::size_t &off() noexcept
				{
					return offsets[I];
				}
				template<std::size_t I>
				[[nodiscard]] constexpr const std::size_t &off() const noexcept
				{
					return offsets[I];
				}

				std::size_t offsets[key_count];
			};

			using sparse_alloc = typename std::allocator_traits<Alloc>::template rebind_alloc<bucket_type>;
			using sparse_data_t = std::vector<bucket_type, sparse_alloc>;
			using dense_alloc = typename std::allocator_traits<Alloc>::template rebind_alloc<entry_type>;
			using dense_data_t = std::vector<entry_type, dense_alloc>;

			template<bool IsConst>
			class mkmap_iterator
			{
				template<bool>
				friend class mkmap_iterator;

				friend class mkmap_impl;

				using iter_t =
					std::conditional_t<IsConst, typename dense_data_t::const_iterator, typename dense_data_t::iterator>;

			public:
				typedef mkmap_value_t<multikey<Ks...>, M> value_type;
				typedef std::conditional_t<IsConst, const value_type, value_type> *pointer;
				typedef std::conditional_t<IsConst, const value_type, value_type> &reference;
				typedef std::size_t size_type;
				typedef std::ptrdiff_t difference_type;
				typedef std::random_access_iterator_tag iterator_category;

			private:
				constexpr explicit mkmap_iterator(iter_t iter) noexcept : iter(iter) {}

			public:
				constexpr mkmap_iterator() noexcept = default;
				template<bool OtherConst, typename = std::enable_if_t<IsConst && !OtherConst>>
				constexpr mkmap_iterator(const mkmap_iterator<OtherConst> &other) noexcept : mkmap_iterator(other.iter)
				{
				}

				constexpr mkmap_iterator operator++(int) noexcept
				{
					auto temp = *this;
					++(*this);
					return temp;
				}
				constexpr mkmap_iterator &operator++() noexcept
				{
					++iter;
					return *this;
				}
				constexpr mkmap_iterator &operator+=(difference_type n) noexcept
				{
					iter += n;
					return *this;
				}
				constexpr mkmap_iterator operator--(int) noexcept
				{
					auto temp = *this;
					--(*this);
					return temp;
				}
				constexpr mkmap_iterator &operator--() noexcept
				{
					--iter;
					return *this;
				}
				constexpr mkmap_iterator &operator-=(difference_type n) noexcept
				{
					iter -= n;
					return *this;
				}

				constexpr mkmap_iterator operator+(difference_type n) const noexcept
				{
					return mkmap_iterator{iter + n};
				}
				constexpr mkmap_iterator operator-(difference_type n) const noexcept
				{
					return mkmap_iterator{iter - n};
				}
				constexpr difference_type operator-(const mkmap_iterator &other) const noexcept
				{
					return iter - other.iter;
				}

				/** Returns pointer to the target element. */
				[[nodiscard]] constexpr pointer get() const noexcept { return &iter->value; }
				/** @copydoc value */
				[[nodiscard]] constexpr pointer operator->() const noexcept { return get(); }

				/** Returns reference to the element at an offset. */
				[[nodiscard]] constexpr reference operator[](difference_type n) const noexcept { return iter[n].value; }
				/** Returns reference to the target element. */
				[[nodiscard]] constexpr reference operator*() const noexcept { return *get(); }

				[[nodiscard]] constexpr auto operator<=>(const mkmap_iterator &) const noexcept = default;
				[[nodiscard]] constexpr bool operator==(const mkmap_iterator &) const noexcept = default;

				constexpr void swap(mkmap_iterator &other) noexcept { std::swap(iter, other.iter); }
				friend constexpr void swap(mkmap_iterator &a, mkmap_iterator &b) noexcept { a.swap(b); }

			private:
				[[nodiscard]] constexpr auto &entry() const noexcept { return *iter; }

				iter_t iter;
			};

		public:
			typedef value_type *pointer;
			typedef const value_type *const_pointer;
			typedef value_type &reference;
			typedef const value_type &const_reference;

			typedef mkmap_iterator<false> iterator;
			typedef mkmap_iterator<true> const_iterator;
			typedef std::reverse_iterator<iterator> reverse_iterator;
			typedef std::reverse_iterator<const_iterator> const_reverse_iterator;

			typedef std::size_t size_type;
			typedef std::ptrdiff_t difference_type;
			typedef dense_alloc allocator_type;
			typedef sparse_alloc bucket_allocator_type;

		public:
			// clang-format off
			constexpr mkmap_impl() = default;
			constexpr mkmap_impl(const mkmap_impl &) = default;
			constexpr mkmap_impl(mkmap_impl &&)
				noexcept(std::is_nothrow_move_constructible_v<dense_data_t> &&
						 std::is_nothrow_move_constructible_v<sparse_data_t>) = default;
			constexpr mkmap_impl &operator=(const mkmap_impl &) = default;
			constexpr mkmap_impl &operator=(mkmap_impl &&)
				noexcept(std::is_nothrow_move_assignable_v<dense_data_t> &&
				         std::is_nothrow_move_assignable_v<sparse_data_t>) = default;
			// clang-format on

			/** Constructs a map with the specified allocators.
			 * @param value_alloc Allocator used to allocate map's value array.
			 * @param bucket_alloc Allocator used to allocate map's bucket array. */
			constexpr explicit mkmap_impl(const allocator_type &value_alloc,
										  const bucket_allocator_type &bucket_alloc = bucket_allocator_type{})
				: mkmap_impl(initial_capacity, value_alloc, bucket_alloc)
			{
			}
			/** Constructs a map with the specified minimum capacity and allocators.
			 * @param capacity Capacity of the map.
			 * @param value_alloc Allocator used to allocate map's value array.
			 * @param bucket_alloc Allocator used to allocate map's bucket array. */
			constexpr explicit mkmap_impl(size_type capacity,
										  const allocator_type &value_alloc,
										  const bucket_allocator_type &bucket_alloc = bucket_allocator_type{})
				: entries(value_alloc), buckets(capacity, bucket_type{}, bucket_alloc)
			{
			}

			/** Constructs a map from a sequence of values.
			 * @param first Iterator to the start of the value sequence.
			 * @param first Iterator to the end of the value sequence.
			 * @param value_alloc Allocator used to allocate map's value array.
			 * @param bucket_alloc Allocator used to allocate map's bucket array. */
			template<std::random_access_iterator Iterator>
			constexpr mkmap_impl(Iterator first,
								 Iterator last,
								 const allocator_type &value_alloc = allocator_type{},
								 const bucket_allocator_type &bucket_alloc = bucket_allocator_type{})
				: mkmap_impl(static_cast<size_type>(std::distance(first, last)), value_alloc, bucket_alloc)
			{
				insert(first, last);
			}

			/** Constructs a map from a sequence of values.
			 * @param first Iterator to the start of the value sequence.
			 * @param first Iterator to the end of the value sequence.
			 * @param value_alloc Allocator used to allocate map's value array.
			 * @param bucket_alloc Allocator used to allocate map's bucket array. */
			template<std::forward_iterator Iterator>
			constexpr mkmap_impl(Iterator first,
								 Iterator last,
								 const allocator_type &value_alloc = allocator_type{},
								 const bucket_allocator_type &bucket_alloc = bucket_allocator_type{})
				: mkmap_impl(value_alloc, bucket_alloc)
			{
				insert(first, last);
			}
			/** Constructs a map from an initializer list.
			 * @param init_list Initializer list containing values.
			 * @param value_alloc Allocator used to allocate map's value array.
			 * @param bucket_alloc Allocator used to allocate map's bucket array. */
			constexpr mkmap_impl(std::initializer_list<value_type> init_list,
								 const allocator_type &value_alloc = allocator_type{},
								 const bucket_allocator_type &bucket_alloc = bucket_allocator_type{})
				: mkmap_impl(init_list.begin(), init_list.end(), value_alloc, bucket_alloc)
			{
			}

			/** Constructs a map with the specified hashers & allocators.
			 * @param kh Hash functors for every key.
			 * @param value_alloc Allocator used to allocate map's value array.
			 * @param bucket_alloc Allocator used to allocate map's bucket array. */
			constexpr explicit mkmap_impl(const typename Ks::hash_type &...kh,
										  const allocator_type &value_alloc = allocator_type{},
										  const bucket_allocator_type &bucket_alloc = bucket_allocator_type{})
				: mkmap_impl((typename Ks::key_equal{})..., kh..., value_alloc, bucket_alloc)
			{
			}
			/** Constructs a map with the specified comparator, hasher & allocators.
			 * @param kc Compare functors for every key.
			 * @param kh Hash functors for every key.
			 * @param value_alloc Allocator used to allocate map's value array.
			 * @param bucket_alloc Allocator used to allocate map's bucket array. */
			constexpr explicit mkmap_impl(const typename Ks::key_equal &...kc,
										  const typename Ks::hash_type &...kh,
										  const allocator_type &value_alloc = allocator_type{},
										  const bucket_allocator_type &bucket_alloc = bucket_allocator_type{})
				: mkmap_impl(initial_capacity, kc..., kh..., value_alloc, bucket_alloc)
			{
			}
			/** Constructs a map with the specified minimum capacity.
			 * @param capacity Capacity of the map.
			 * @param kc Compare functors for every key.
			 * @param kh Hash functors for every key.
			 * @param value_alloc Allocator used to allocate map's value array.
			 * @param bucket_alloc Allocator used to allocate map's bucket array. */
			constexpr explicit mkmap_impl(size_type capacity,
										  const typename Ks::key_equal &...kc,
										  const typename Ks::hash_type &...kh,
										  const allocator_type &value_alloc = allocator_type{},
										  const bucket_allocator_type &bucket_alloc = bucket_allocator_type{})
				: comp_base(kc...), hash_base(kh...), entries(value_alloc), buckets(capacity, bucket_type{}, bucket_alloc)
			{
			}

			/** Constructs a map from a sequence of values.
			 * @param first Iterator to the start of the value sequence.
			 * @param first Iterator to the end of the value sequence.
			 * @param kc Compare functors for every key.
			 * @param kh Hash functors for every key.
			 * @param value_alloc Allocator used to allocate map's value array.
			 * @param bucket_alloc Allocator used to allocate map's bucket array. */
			template<std::random_access_iterator Iterator>
			constexpr mkmap_impl(Iterator first,
								 Iterator last,
								 const typename Ks::key_equal &...kc,
								 const typename Ks::hash_type &...kh,
								 const allocator_type &value_alloc = allocator_type{},
								 const bucket_allocator_type &bucket_alloc = bucket_allocator_type{})
				: mkmap_impl(static_cast<size_type>(std::distance(first, last)), kc..., kh..., value_alloc, bucket_alloc)
			{
				insert(first, last);
			}
			/** Constructs a map from a sequence of values.
			 * @param first Iterator to the start of the value sequence.
			 * @param first Iterator to the end of the value sequence.
			 * @param kc Compare functors for every key.
			 * @param kh Hash functors for every key.
			 * @param value_alloc Allocator used to allocate map's value array.
			 * @param bucket_alloc Allocator used to allocate map's bucket array. */
			template<std::forward_iterator Iterator>
			constexpr mkmap_impl(Iterator first,
								 Iterator last,
								 const typename Ks::key_equal &...kc,
								 const typename Ks::hash_type &...kh,
								 const allocator_type &value_alloc = allocator_type{},
								 const bucket_allocator_type &bucket_alloc = bucket_allocator_type{})
				: mkmap_impl(kc..., kh..., value_alloc, bucket_alloc)
			{
				insert(first, last);
			}
			/** Constructs a map from an initializer list.
			 * @param init_list Initializer list containing values.
			 * @param kc Compare functors for every key.
			 * @param kh Hash functors for every key.
			 * @param value_alloc Allocator used to allocate map's value array.
			 * @param bucket_alloc Allocator used to allocate map's bucket array. */
			constexpr mkmap_impl(std::initializer_list<value_type> init_list,
								 const typename Ks::key_equal &...kc,
								 const typename Ks::hash_type &...kh,
								 const allocator_type &value_alloc = allocator_type{},
								 const bucket_allocator_type &bucket_alloc = bucket_allocator_type{})
				: mkmap_impl(init_list.begin(), init_list.end(), kc..., kh..., value_alloc, bucket_alloc)
			{
			}

			/** Returns iterator to the start of the map. */
			[[nodiscard]] constexpr iterator begin() noexcept { return iterator{entries.begin()}; }
			/** Returns const iterator to the start of the map. */
			[[nodiscard]] constexpr const_iterator cbegin() const noexcept { return const_iterator{entries.begin()}; }
			/** @copydoc cbegin */
			[[nodiscard]] constexpr const_iterator begin() const noexcept { return cbegin(); }
			/** Returns iterator to the end of the map. */
			[[nodiscard]] constexpr iterator end() noexcept { return iterator{entries.end()}; }
			/** Returns const iterator to the end of the map. */
			[[nodiscard]] constexpr const_iterator cend() const noexcept { return const_iterator{entries.end()}; }
			/** @copydoc cend */
			[[nodiscard]] constexpr const_iterator end() const noexcept { return cend(); }
			/** Returns reverse iterator to the end of the map. */
			[[nodiscard]] constexpr reverse_iterator rbegin() noexcept { return reverse_iterator{end()}; }
			/** Returns const reverse iterator to the end of the map. */
			[[nodiscard]] constexpr const_reverse_iterator crbegin() const noexcept
			{
				return const_reverse_iterator{cend()};
			}
			/** @copydoc crbegin */
			[[nodiscard]] constexpr const_reverse_iterator rbegin() const noexcept { return crbegin(); }
			/** Returns reverse iterator to the start of the map. */
			[[nodiscard]] constexpr reverse_iterator rend() noexcept { return reverse_iterator{begin()}; }
			/** Returns const reverse iterator to the start of the map. */
			[[nodiscard]] constexpr const_reverse_iterator crend() const noexcept
			{
				return const_reverse_iterator{cbegin()};
			}
			/** @copydoc crend */
			[[nodiscard]] constexpr const_reverse_iterator rend() const noexcept { return crend(); }

			/** Returns the current size of the map. */
			[[nodiscard]] constexpr size_type size() const noexcept { return entries.size(); }
			/** Returns the current capacity of the map. */
			[[nodiscard]] constexpr size_type capacity() const noexcept
			{
				/* Capacity needs to take into account the max load factor. */
				return static_cast<size_type>(static_cast<float>(bucket_count()) * load_factor_mult);
			}
			/** Returns the maximum possible size of the map. */
			[[nodiscard]] constexpr size_type max_size() const noexcept
			{
				/* Max size cannot exceed max load factor of max entry capacity. */
				return static_cast<size_type>(static_cast<float>(entries.max_size()) * load_factor_mult);
			}
			/** Checks if the map is empty. */
			[[nodiscard]] constexpr size_type empty() const noexcept { return size() == 0; }

			/** Removes all elements from the map. */
			constexpr void clear()
			{
				std::fill_n(buckets.data(), bucket_count(), bucket_type{});
				entries.clear();
			}

			/** Re-hashes the map for the specified minimal capacity. */
			constexpr void rehash(size_type new_cap)
			{
				/* Adjust the capacity to be at least large enough to fit the current size. */
				new_cap = math::max(
					static_cast<size_type>(static_cast<float>(size()) / load_factor_mult), new_cap, initial_capacity);

				/* Don't do anything if the capacity did not change after the adjustment. */
				if (new_cap != buckets.capacity()) [[likely]]
					rehash_impl(new_cap);
			}
			/** Resizes the internal storage to have space for at least n elements. */
			constexpr void reserve(size_type n)
			{
				entries.reserve(n);
				rehash(static_cast<size_type>(static_cast<float>(n) / load_factor_mult));
			}
			/** Shrinks internal storage to only occupy the required amount of space for it's size. */
			constexpr void narrow()
			{
				buckets.shrink_to_fit();
				entries.shrink_to_fit();
				buckets.resize(math::next_pow_2(static_cast<size_type>(static_cast<float>(size()) / load_factor_mult)),
							   bucket_type{});
			}
			/** @copydoc narrow */
			constexpr void shrink_to_fit() { narrow(); }

			/** Locates an element for the specific key.
			 * @tparam I Index of the desired key.
			 * @param key Key to search for.
			 * @return Iterator to the element mapped to key. */
			template<size_type I>
			constexpr iterator find(const key_type<I> &key) noexcept
			{
				return begin() + static_cast<difference_type>(find_impl<I>(key_hash<I>(key), key));
			}
			/** @copydoc find */
			template<size_type I>
			constexpr const_iterator find(const key_type<I> &key) const noexcept
			{
				return cbegin() + static_cast<difference_type>(find_impl<I>(key_hash<I>(key), key));
			}
			/** @copydoc find */
			template<size_type I>
			constexpr iterator find(key_select_t<I>, const key_type<I> &key) noexcept
			{
				return find<I>(key);
			}
			/** @copydoc find */
			template<size_type I>
			constexpr const_iterator find(key_select_t<I>, const key_type<I> &key) const noexcept
			{
				return find<I>(key);
			}

			/** Locates an element for the specific key.
			 * @param key Key to search for.
			 * @return Iterator to the element mapped to key.
			 * @note This overload selects the first key of matching type. */
			template<typename T>
			constexpr iterator find(const T &key) noexcept
				requires is_key<std::decay_t<T>>
			{
				return find<key_index<std::decay_t<T>>>(key);
			}
			/** @copydoc find */
			template<typename T>
			constexpr const_iterator find(const T &key) const noexcept
				requires is_key<std::decay_t<T>>
			{
				return find<key_index<std::decay_t<T>>>(key);
			}

			/** Checks if the map contains an element with specific key.
			 * @tparam I Index of the desired key.
			 * @param key Key to search for. */
			template<size_type I>
			constexpr bool contains(const key_type<I> &key) const noexcept
			{
				return find<I>(key) != end();
			}
			/** @copydoc contains */
			template<size_type I>
			constexpr bool contains(key_select_t<I>, const key_type<I> &key) const noexcept
			{
				return contains<I>(key);
			}

			/** Checks if the map contains an element with specific key.
			 * @param key Key to search for.
			 * @note This overload selects the first key of matching type. */
			template<typename T>
			constexpr bool contains(const T &key) const noexcept
				requires is_key<std::decay_t<T>>
			{
				return contains<key_index<std::decay_t<T>>>(key);
			}

			/** Returns reference to object mapped to the specific key.
			 * @tparam I Index of the desired key.
			 * @param key Key to search for.
			 * @return Reference to the object mapped to key.
			 * @throw std::out_of_range If the specified key is not present in the map. */
			template<size_type I>
			constexpr mapped_type &at(const key_type<I> &key)
			{
				if (auto iter = find<I>(key); iter != end()) [[likely]]
					return iter->second;
				else
					throw std::out_of_range("Specified key is not present within the map");
			}
			/** @copydoc at */
			template<size_type I>
			constexpr mapped_type &at(key_select_t<I>, const key_type<I> &key)
			{
				return at<I>(key);
			}
			/** Returns reference to object mapped to the specific key.
			 * @param key Key to search for.
			 * @return Reference to the object mapped to key.
			 * @throw std::out_of_range If the specified key is not present in the map.
			 * @note This overload selects the first key of matching type. */
			template<typename T>
			constexpr mapped_type &at(const T &key)
				requires is_key<std::decay_t<T>>
			{
				return at<key_index<std::decay_t<T>>>(key);
			}
			/** Returns const reference to object mapped to the specific key.
			 * @param key Key to search for.
			 * @return Reference to the object mapped to key.
			 * @throw std::out_of_range If the specified key is not present in the map. */
			template<size_type I>
			constexpr const mapped_type &at(const key_type<I> &key) const
			{
				if (auto iter = find<I>(key); iter != end()) [[likely]]
					return iter->second;
				else
					throw std::out_of_range("Specified key is not present within the map");
			}
			/** @copydoc at */
			template<size_type I>
			constexpr const mapped_type &at(key_select_t<I>, const key_type<I> &key) const
			{
				return at<I>(key);
			}
			/** Returns const reference to object mapped to the specific key.
			 * @param key Key to search for.
			 * @return Reference to the object mapped to key.
			 * @throw std::out_of_range If the specified key is not present in the map.
			 * @note This overload selects the first key of matching type. */
			template<typename T>
			constexpr const mapped_type &at(const T &key) const
				requires is_key<std::decay_t<T>>
			{
				return at<key_index<std::decay_t<T>>>(key);
			}

			/** Attempts to construct a value in-place using the specified keys.
			 * If any of the specified keys is already associated with a value, does nothing.
			 * @param key Key for which to insert the value.
			 * @param args Arguments used to construct the mapped object.
			 * @return Pair where first element is the iterator to the potentially inserted element
			 * and second is boolean indicating whether the element was inserted (`true` if inserted, `false` otherwise).
			 * If the element was not inserted, iterator points to an entry that has at least one matching key. */
			template<typename... Args>
			constexpr std::pair<iterator, bool> try_emplace(typename Ks::key_type &&...keys, Args &&...args)
			{
				// clang-format off
				return try_insert_impl(std::forward<typename Ks::key_type>(keys)...,
				                       std::piecewise_construct,
									   std::forward_as_tuple(std::forward<typename Ks::key_type>(keys)...),
									   std::forward_as_tuple(std::forward<Args>(args)...));
				// clang-format on
			}
			/** @copydoc try_emplace */
			template<typename... Args>
			constexpr std::pair<iterator, bool> try_emplace(const typename Ks::key_type &...keys, Args &&...args)
			{
				// clang-format off
				return try_insert_impl(keys...,
				                       std::piecewise_construct,
									   std::forward_as_tuple(std::forward<typename Ks::key_type>(keys)...),
									   std::forward_as_tuple(std::forward<Args>(args)...));
				// clang-format on
			}
			/** Constructs a value (of value_type) in-place.
			 * If a value for any of the the constructed keys is already present within the map, replaces that value.
			 * @param args Arguments used to construct the value object.
			 * @return Pair where first element is the iterator to the inserted element
			 * and second is the amount of existing entries replaced. */
			template<typename... Args>
			constexpr std::pair<iterator, size_type> emplace(Args &&...args)
			{
				return insert_impl(value_type{std::forward<Args>(args)...});
			}

			/** Attempts to insert a value into the map.
			 * If any value with the same key is already present within the map, does nothing.
			 * @param value Value to insert.
			 * @return Pair where first element is the iterator to the potentially inserted element
			 * and second is boolean indicating whether the element was inserted (`true` if inserted, `false` otherwise). */
			constexpr std::pair<iterator, bool> try_insert(value_type &&value)
			{
				return try_insert_impl(std::forward<value_type>(value));
			}
			/** @copydoc try_insert */
			constexpr std::pair<iterator, bool> try_insert(const value_type &value) { return try_insert_impl(value); }
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
			 * Does not replace values with conflicting keys.
			 * @param first Iterator to the start of the value sequence.
			 * @param first Iterator to the end of the value sequence.
			 * @return Amount of elements inserted. */
			template<std::forward_iterator Iterator>
			constexpr size_type try_insert(Iterator first, Iterator last)
			{
				size_type inserted = 0;
				while (first != last) inserted += try_insert(*first++).second;
				return inserted;
			}
			/** Attempts to insert a sequence of values (of value_type) specified by the initializer list into the map.
			 * Does not replace values with conflicting keys.
			 * @param init_list Initializer list containing the values.
			 * @return Amount of elements inserted. */
			constexpr size_type try_insert(std::initializer_list<value_type> init_list)
			{
				return try_insert(init_list.begin(), init_list.end());
			}

			/** Inserts a value (of value_type) into the map.
			 * If a value for any of the the keys is already present within the map, replaces that value.
			 * @param value Value to insert.
			 * @return Pair where first element is the iterator to the inserted element
			 * and second is the amount of existing entries replaced. */
			constexpr std::pair<iterator, size_type> insert(value_type &&value)
			{
				return insert_impl(std::forward<value_type>(value));
			}
			/** @copydoc insert */
			constexpr std::pair<iterator, size_type> insert(const value_type &value) { return insert_impl(value); }
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
			 * If a value for any of the the keys is already present within the map, replaces that value.
			 * @param first Iterator to the start of the value sequence.
			 * @param first Iterator to the end of the value sequence.
			 * @return Amount of *new* elements inserted. */
			template<std::forward_iterator Iterator>
			constexpr size_type insert(Iterator first, Iterator last)
			{
				size_type inserted = 0;
				while (first != last) inserted += insert(*first++).second;
				return inserted;
			}
			/** Inserts a sequence of values (of value_type) specified by the initializer list into the map.
			 * If a value for any of the the keys is already present within the map, replaces that value.
			 * @param init_list Initializer list containing the values.
			 * @return Amount of new elements inserted. */
			constexpr size_type insert(std::initializer_list<value_type> init_list)
			{
				return insert(init_list.begin(), init_list.end());
			}

			/** Removes the specified element from the map.
			 * @param where Iterator to the target element.
			 * @return Iterator to the element after the erased one. */
			constexpr iterator erase(const_iterator where) { return erase_impl(where.iter); }
			/** Removes all elements in the [first, last) range.
			 * @param first Iterator to the first element of the target range.
			 * @param last Iterator to the last element of the target range.
			 * @return Iterator to the element after the erased sequence. */
			constexpr iterator erase(const_iterator first, const_iterator last)
			{
				iterator result = end();
				while (first < last) result = erase(--last);
				return result;
			}
			/** Removes element mapped to the specified key from the map if it is present.
			 * @tparam I Index of the desired key.
			 * @param key Key of the target element.
			 * @return `true` if the element was removed, `false` otherwise. */
			template<size_type I>
			constexpr bool erase(const key_type<I> &key)
			{
				if (auto target = find<I>(key); target != end())
				{
					erase(target);
					return true;
				}
				else
					return false;
			}
			/** @copydoc erase */
			template<size_type I>
			constexpr bool erase(key_select_t<I>, const key_type<I> &key)
			{
				return erase<I>(key);
			}

			/** Removes element mapped to the specified key from the map if it is present.
			 * @param key Key of the target element.
			 * @return `true` if the element was removed, `false` otherwise.
			 * @note This overload selects the first key of matching type. */
			template<typename T>
			constexpr bool erase(const T &key)
				requires is_key<std::decay_t<T>>
			{
				return erase<key_index<std::decay_t<T>>>(key);
			}

			/** Returns current bucket count (same for all keys). */
			[[nodiscard]] constexpr size_type bucket_count() const noexcept { return buckets.size(); }
			/** Returns maximum possible bucket count (same for all keys). */
			[[nodiscard]] constexpr size_type max_bucket_count() const noexcept { return buckets.max_size(); }

			/** Returns current load factor of the map. */
			[[nodiscard]] constexpr auto load_factor() const noexcept
			{
				return static_cast<float>(size()) / static_cast<float>(bucket_count());
			}
			/** Returns current max load factor of the map. */
			[[nodiscard]] constexpr auto max_load_factor() const noexcept { return load_factor_mult; }
			/** Sets current max load factor of the map. */
			constexpr void max_load_factor(float f) noexcept
			{
				SEK_ASSERT(f > .0f);
				load_factor_mult = f;
			}

			[[nodiscard]] constexpr allocator_type get_allocator() const noexcept { return entries.get_allocator(); }
			[[nodiscard]] constexpr bucket_allocator_type get_bucket_allocator() const noexcept
			{
				return buckets.get_allocator();
			}

			template<size_type I = 0>
			[[nodiscard]] constexpr hash_type<I> hash_function() const noexcept
			{
				return hash_base::template get<I>();
			}
			template<size_type I>
			[[nodiscard]] constexpr hash_type<I> hash_function(key_select_t<I>) const noexcept
			{
				return hash_function<I>();
			}
			template<size_type I = 0>
			[[nodiscard]] constexpr key_equal<I> key_eq() const noexcept
			{
				return comp_base::template get<I>();
			}
			template<size_type I = 0>
			[[nodiscard]] constexpr key_equal<I> key_eq(key_select_t<I>) const noexcept
			{
				return key_eq<I>();
			}

			[[nodiscard]] constexpr bool operator==(const mkmap_impl &other) const noexcept
			{
				return std::is_permutation(begin(), end(), other.begin(), other.end());
			}

			constexpr void swap(mkmap_impl &other) noexcept
			{
				using std::swap;
				swap(buckets, other.buckets);
				swap(entries, other.entries);
				swap(load_factor_mult, other.load_factor_mult);
			}
			friend constexpr void swap(mkmap_impl &a, mkmap_impl &b) noexcept { a.swap(b); }

		private:
			template<size_type I>
			[[nodiscard]] constexpr auto key_hash(const key_type<I> &key) const
			{
				return hash_base::template get<I>()(key);
			}
			template<size_type I>
			[[nodiscard]] constexpr auto key_comp(const key_type<I> &a, const key_type<I> &b) const
			{
				return comp_base::template get<I>()(a, b);
			}

			template<size_type I>
			[[nodiscard]] constexpr auto *get_chain(hash_t h) noexcept
			{
				return &buckets[h % buckets.size()].template off<I>();
			}
			template<size_type I>
			[[nodiscard]] constexpr auto *get_chain(hash_t h) const noexcept
			{
				return &buckets[h % buckets.size()].template off<I>();
			}

			constexpr void insert_entry(entry_type &entry, size_type pos)
			{
				/* Initialize buckets for every key. */
				foreach_key(
					[&]<size_type I>(index_selector_t<I>)
					{
						for (auto chain_idx = get_chain<I>(entry.template hash<I>());;
							 chain_idx = &entries[*chain_idx].template next<I>())
							if (*chain_idx == npos)
							{
								*chain_idx = pos;
								break;
							}
					});
			}
			constexpr void remove_entry(const entry_type &entry, size_type pos)
			{
				foreach_key(
					[&]<size_type I>(index_selector_t<I>)
					{
						/* Find the chain offset pointing to the entry position & replace it with the next position. */
						for (auto chain_idx = get_chain<I>(entry.template hash<I>()); *chain_idx != npos;
							 chain_idx = &entries[*chain_idx].template next<I>())
							if (*chain_idx == pos)
							{
								*chain_idx = entry.template next<I>();
								break;
							}
					});
			}
			constexpr void move_entry(const entry_type &entry, size_type old_pos, size_type new_pos)
			{
				foreach_key(
					[&]<size_type I>(index_selector_t<I>)
					{
						/* Find the chain offset pointing to the old position & replace it with the new position. */
						for (auto chain_idx = get_chain<I>(entry.template hash<I>()); *chain_idx != npos;
							 chain_idx = &entries[*chain_idx].template next<I>())
							if (*chain_idx == old_pos)
							{
								*chain_idx = new_pos;
								break;
							}
					});
			}
			constexpr void rehash_entry(entry_type &entry)
			{
				/* Re-calculate hashes for every key. */
				foreach_key([&]<size_type I>(index_selector_t<I>)
							{ entry.template hash<I>() = key_hash<I>(entry.template key<I>()); });
			}
			template<size_type I>
			constexpr void rehash_entry(entry_type &entry, hash_t h)
			{
				/* Re-calculate hashes for every key except h. */
				foreach_key(
					[&]<size_type J>(index_selector_t<J>)
					{
						if constexpr (J == I)
							entry.template hash<J>() = h;
						else
							entry.template hash<J>() = key_hash<J>(entry.template key<J>());
					});
			}

			template<typename... Args>
			[[nodiscard]] constexpr iterator insert_new(Args &&...args) noexcept
			{
				const auto pos = size();
				auto &entry = entries.emplace_back(std::forward<Args>(args)...);
				rehash_entry(entry);
				insert_entry(entry, pos);
				maybe_rehash();
				return begin() + static_cast<difference_type>(pos);
			}
			template<typename T>
			[[nodiscard]] constexpr std::pair<iterator, size_type> insert_impl(const typename Ks::key_type &...keys,
																			   T &&value) noexcept
			{
				/* See if any entries already exist and erase them. */
				size_type erase_count = false;
				foreach_key(
					[&]<size_type I>(index_selector_t<I>)
					{
						const auto &key = get<I>(type_seq_t<const typename Ks::key_type &...>{}, keys...);
						const auto h = key_hash<I>(key);
						auto *chain_idx = get_chain<I>(h);
						while (*chain_idx != npos)
							if (auto existing_iter = entries.begin() + *chain_idx;
								existing_iter->template hash<I>() == h && key_comp<I>(key, existing_iter->template key<I>()))
							{
								erase_impl(existing_iter);
								++erase_count;
								break;
							}
							else
								chain_idx = &existing_iter->template next<I>();
					});

				return {insert_new(std::forward<T>(value)), erase_count};
			}
			template<typename T>
			[[nodiscard]] constexpr std::pair<iterator, size_type> insert_impl(T &&value) noexcept
			{
				auto unwrap = [&]<size_type... Is>(std::index_sequence<Is...>)
				{
					return insert_impl(get_key<Is>(value)..., std::forward<T>(value));
				};
				return unwrap(std::make_index_sequence<key_count>{});
			}
			template<typename... Args>
			[[nodiscard]] constexpr std::pair<iterator, bool> try_insert_impl(const typename Ks::key_type &...keys,
																			  Args &&...args) noexcept
			{
				/* See if an entry already exists. */
				iterator result = end();
				foreach_key(
					[&]<size_type I>(index_selector_t<I>)
					{
						const auto &key = get<I>(type_seq_t<const typename Ks::key_type &...>{}, keys...);
						const auto h = key_hash<I>(key);
						auto *chain_idx = get_chain<I>(h);
						while (*chain_idx != npos)
							if (auto &existing = entries[*chain_idx];
								existing.template hash<I>() == h && key_comp<I>(key, existing.template key<I>()))
							{
								result = begin() + static_cast<difference_type>(*chain_idx);
								break;
							}
							else
								chain_idx = &existing.template next<I>();
					});

				if (result == end()) /* No existing entry found, create new entry. */
					return {insert_new(std::forward<Args>(args)...), true};
				else
					return {result, false};
			}
			template<typename T>
			[[nodiscard]] constexpr std::pair<iterator, bool> try_insert_impl(T &&value) noexcept
			{
				auto unwrap = [&]<size_type... Is>(std::index_sequence<Is...>)
				{
					return try_insert_impl(get_key<Is>(value)..., std::forward<T>(value));
				};
				return unwrap(std::make_index_sequence<key_count>{});
			}

			template<size_type I>
			[[nodiscard]] constexpr size_type find_impl(hash_t h, const key_type<I> &key) const noexcept
			{
				for (auto *idx = get_chain<I>(h); *idx != npos;)
				{
					auto &entry = entries[*idx];
					if (entry.template hash<I>() == h && key_comp<I>(key, entry.template key<I>())) [[likely]]
						return *idx;
					idx = &entry.template next<I>();
				}
				return entries.size();
			}

			constexpr void maybe_rehash()
			{
				if (load_factor() > load_factor_mult) [[unlikely]]
					rehash(bucket_count() * 2);
			}
			constexpr void rehash_impl(size_type new_cap)
			{
				/* Clear & resize the bucket vector filled with npos. */
				buckets.clear();
				buckets.resize(new_cap);

				/* Go through each entry & re-insert it. */
				for (std::size_t i = 0; i < entries.size(); ++i)
				{
					auto &entry = entries[i];
					foreach_key(
						[&]<size_type I>(index_selector_t<I>)
						{
							auto *chain_idx = get_chain<I>(entry.template hash<I>());
							entry.template next<I>() = *chain_idx;
							*chain_idx = i;
						});
				}
			}

			constexpr iterator erase_impl(typename dense_data_t::const_iterator where)
			{
				/* Un-link the entry from the chain & swap with the last entry. */
				const auto pos = static_cast<size_type>(where - entries.begin());
				remove_entry(*where, pos);

				const auto old_pos = size() - 1;
				if (pos != old_pos) /* Make sure we are not erasing the last item. */
				{
					if constexpr (std::is_move_assignable_v<entry_type>)
						entries[pos] = std::move(entries.back());
					else
						entries[pos].swap(entries.back());
					move_entry(*where, old_pos, pos);
				}

				entries.pop_back();
				return begin() + pos;
			}

			dense_data_t entries;
			sparse_data_t buckets = sparse_data_t(initial_capacity, bucket_type{});
			float load_factor_mult = initial_load_factor;
		};
	}	 // namespace detail

	/** @brief Special associative container that allows to associate multiple keys to the same value.
	 *
	 * Internally, multi-key map is implemented as a dense map where each bucket contains multiple values
	 * (one for each key). Same limitations apply as for a dense map.
	 *
	 * @tparam Multikey Instance of the `multikey` template used to specify keys of the multi-key map.
	 * @tparam M Type of objects associated with keys.
	 * @tparam Alloc Allocator used for the map.
	 *
	 * @example
	 * @code{.cpp}
	 * using my_map = mkmap<multikey<key_t<std::string>, key_t<int>>, float>;
	 * @endcode */
	template<template_type_instance<multikey> Multikey, typename Mapped, typename Alloc = std::allocator<mkmap_value_t<Multikey, Mapped>>>
	using mkmap = detail::mkmap_impl<Multikey, Mapped, Alloc>;
}	 // namespace sek