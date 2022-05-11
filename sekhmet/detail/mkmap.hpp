//
// Created by switchblade on 11/05/22.
//

#pragma once

#include <tuple>
#include <vector>

#include "define.h"
#include "ebo_base_helper.hpp"
#include "hash.hpp"
#include "meta_util.hpp"
#include "table_util.hpp"

namespace sek
{
	/** @brief Helper type used to specify key, hasher & comparator for a multimap key. */
	template<typename K, typename KeyHash = default_hash, typename KeyComp = std::equal_to<K>>
	struct mkmap_key
	{
		using key_type = K;
		using hash_type = KeyHash;
		using key_equal = KeyComp;
	};

	/** @brief Helper type used to group key types for a multi-key map specified via `mkmap_key`. */
	template<template_type_instance<mkmap_key>...>
	struct multikey
	{
	};

	template<typename, typename>
	struct mkmap_value;
	/** @brief Helper used to obtain a value type for a multikey map. */
	template<typename M, typename... Ks>
	struct mkmap_value<multikey<Ks...>, M>
	{
		using type = std::tuple<const typename Ks::key_value..., M>;
	};

	template<template_type_instance<multikey> Multikey, typename Mapped>
	using mkmap_value_t = typename mkmap_value<Multikey, Mapped>::type;

	namespace detail
	{
		template<typename...>
		class mkmap_impl;

		template<std::size_t, typename...>
		class mkmap_ebo_base
		{
		};
		template<std::size_t I, typename T, typename... Ts>
		class mkmap_ebo_base<I, T, Ts...> : ebo_base_helper<T>, mkmap_ebo_base<I + 1, Ts...>
		{
			using expand_base = mkmap_ebo_base<I + 1, Ts...>;
			using ebo_base = ebo_base_helper<T>;

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

			template<std::size_t N>
			constexpr decltype(auto) get()
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
		using mkmap_equal_base = mkmap_ebo_base<0, typename Ks::key_equal...>;

		template<typename M, typename... Ks, typename Alloc>
		class mkmap_impl<multikey<Ks...>, M, Alloc> : mkmap_hash_base<Ks...>, mkmap_equal_base<Ks...>
		{
			static_assert(sizeof...(Ks) != 0);

			using hash_base = mkmap_hash_base<Ks...>;
			using key_base = mkmap_equal_base<Ks...>;
			using key_ts = type_seq_t<Ks...>;

		public:
			typedef M mapped_type;
			typedef mkmap_value_t<multikey<Ks...>, M> value_type;
			typedef std::size_t size_type;
			typedef std::ptrdiff_t difference_type;

			/** @brief Helper type used to explicitly select a particular key of a multimap. */
			template<size_type I>
			struct key_select_t
			{
				constexpr static auto key = I;
			};
			template<size_type I>
			constexpr static auto key_select = key_select_t<I>{};

			constexpr static auto key_count = sizeof...(Ks);

			template<size_type I>
			using key_type = typename type_seq_element_t<I, key_ts>::key_type;
			template<size_type I>
			using hash_type = typename type_seq_element_t<I, key_ts>::key_hash;
			template<size_type I>
			using key_equal = typename type_seq_element_t<I, key_ts>::key_equal;

		private:
			constexpr static float initial_load_factor = .875f;
			constexpr static size_type initial_capacity = 8;
			constexpr static size_type npos = std::numeric_limits<size_type>::max();

			template<size_type I>
			constexpr static decltype(auto) key_get(const value_type &v)
				requires(I < key_count)
			{
				return std::get<I>(v);
			}

			struct entry_type
			{
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

				constexpr void swap(entry_type &other) noexcept(std::is_nothrow_swappable_v<value_type>)
				{
					using std::swap;
					swap(next, other.next);
					swap(hash, other.hash);
					swap(value, other.value);
				}

				size_type next = npos;
				hash_t hash[key_count];
				value_type value;
			};

			using sparse_alloc = typename std::allocator_traits<Alloc>::template rebind_alloc<size_type>;
			using sparse_data_t = std::vector<size_type, sparse_alloc>;
			using dense_alloc = typename std::allocator_traits<Alloc>::template rebind_alloc<entry_type>;
			using dense_data_t = std::vector<entry_type, dense_alloc>;

		public:
		private:
			/* Array of sparse vectors containing buckets for every key. */
			sparse_data_t buckets[key_count];
			/* Dense vectors containing values. */
			dense_data_t entries;
			float max_load_factor = initial_load_factor;
		};
	}	 // namespace detail

	/** @brief Special associative container that allows to associate multiple keys to the same value.
	 * @example
	 * @code{.cpp}
	 * using my_map = basic_mkmap<multikey<mkmap_key<std::string>, mkmap_key<int>>, float>;
	 * @endcode*/
	template<template_type_instance<multikey> Multikey, typename Mapped, typename Alloc = std::allocator<mkmap_value_t<Multikey, Mapped>>>
	using basic_mkmap = detail::mkmap_impl<Multikey, Mapped, Alloc>;
}	 // namespace sek