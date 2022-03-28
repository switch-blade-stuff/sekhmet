//
// Created by switchblade on 2021-10-21.
//

#pragma once

#include <concepts>
#include <ranges>

#include "define.h"
#include <type_traits>

namespace sek
{
	template<std::size_t I>
	struct index_selector_t : index_selector_t<I - 1>
	{
	};
	template<>
	struct index_selector_t<0> : std::integral_constant<std::size_t, 0>
	{
	};
	template<std::size_t I>
	constexpr auto index_selector = index_selector_t<I>{};

	template<typename... Ts>
	struct type_seq_t
	{
		using type = type_seq_t<Ts...>;
		constexpr static std::size_t size = sizeof...(Ts);
	};
	template<typename... Ts>
	constexpr auto type_seq = type_seq_t<Ts...>{};
	template<typename... Ts>
	constexpr auto type_seq_size = type_seq_t<Ts...>::size;

	template<typename...>
	struct concat_type_seq;
	template<typename... Ts>
	struct concat_type_seq<type_seq_t<>, type_seq_t<Ts...>>
	{
		using type = type_seq_t<Ts...>;
	};
	template<typename... Ts>
	struct concat_type_seq<type_seq_t<Ts...>, type_seq_t<>>
	{
		using type = type_seq_t<Ts...>;
	};
	template<typename... Ts1, typename... Ts2>
	struct concat_type_seq<type_seq_t<Ts1...>, type_seq_t<Ts2...>>
	{
		using type = type_seq_t<Ts1..., Ts2...>;
	};
	template<typename Seq1, typename Seq2>
	using concat_type_seq_t = typename concat_type_seq<Seq1, Seq2>::type;

	namespace detail
	{
		template<std::size_t, std::size_t, typename...>
		struct trim_type_seq_impl;
		template<std::size_t N, std::size_t I>
		struct trim_type_seq_impl<N, I>
		{
			using type = type_seq_t<>;
		};
		template<std::size_t N, std::size_t I, typename T0, typename... Ts>
		struct trim_type_seq_impl<N, I, T0, Ts...>
		{
			using next = typename trim_type_seq_impl<N, I + 1, Ts...>::type;
			using type = std::conditional_t < I<N, concat_type_seq_t<type_seq_t<T0>, next>, type_seq_t<>>;
		};
	}	 // namespace detail

	template<std::size_t, typename>
	struct trim_type_seq;
	template<std::size_t N, typename... Ts>
	struct trim_type_seq<N, type_seq_t<Ts...>>
	{
		using type = typename detail::trim_type_seq_impl<N, 0, Ts...>::type;
	};
	template<std::size_t N, typename Seq>
	using trim_type_seq_t = typename trim_type_seq<N, Seq>::type;

	namespace detail
	{
		template<typename...>
		struct is_in_impl;
		template<typename T>
		struct is_in_impl<T> : std::false_type
		{
		};
		template<typename T0, typename... Ts>
		struct is_in_impl<T0, T0, Ts...> : std::true_type
		{
		};
		template<typename T0, typename T1, typename... Ts>
		struct is_in_impl<T0, T1, Ts...> : is_in_impl<T0, Ts...>
		{
		};
	}	 // namespace detail

	template<typename T, typename... Ts>
	struct is_in : detail::is_in_impl<T, Ts...>
	{
	};
	template<typename T, typename... Ts>
	constexpr auto is_in_v = is_in<T, Ts...>::value;

	template<typename...>
	struct remove_from_seq;
	template<typename... Rs, typename T0>
	struct remove_from_seq<type_seq_t<Rs...>, T0>
	{
		using type = std::conditional_t<is_in_v<T0, Rs...>, type_seq_t<>, type_seq_t<T0>>;
	};
	template<typename... Rs, typename T0, typename... Ts>
	struct remove_from_seq<type_seq_t<Rs...>, T0, Ts...>
	{
		using type =
			std::conditional_t<is_in_v<T0, Rs...>,
							   typename remove_from_seq<type_seq_t<Rs...>, Ts...>::type,
							   concat_type_seq_t<type_seq_t<T0>, typename remove_from_seq<type_seq_t<Rs...>, Ts...>::type>>;
	};
	template<typename... Rs, typename... Ts>
	struct remove_from_seq<type_seq_t<Rs...>, type_seq_t<Ts...>>
	{
		using type = typename remove_from_seq<type_seq_t<Rs...>, Ts...>::type;
	};
	template<typename Seq1, typename Seq2>
	using remove_from_seq_t = typename remove_from_seq<Seq1, Seq2>::type;

	namespace detail
	{
		template<std::size_t N, std::size_t I, typename T, typename... Ts>
		struct make_type_seq_impl
		{
			using type = typename make_type_seq_impl<N, I + 1, T, T, Ts...>::type;
		};
		template<std::size_t N, typename T, typename... Ts>
		struct make_type_seq_impl<N, N, T, Ts...>
		{
			using type = type_seq_t<Ts...>;
		};
	}	 // namespace detail

	template<std::size_t N, typename T>
	struct make_type_seq
	{
		using type = typename detail::make_type_seq_impl<N, 0, T>::type;
	};
	template<std::size_t N, typename T>
	using make_type_seq_t = typename make_type_seq<N, T>::type;

	namespace detail
	{
		template<std::size_t, std::size_t, typename...>
		struct type_seq_element_impl;
		template<std::size_t I>
		struct type_seq_element_impl<I, I>
		{
			typedef void type;
		};
		template<std::size_t I, typename T0, typename... Ts>
		struct type_seq_element_impl<I, I, T0, Ts...>
		{
			typedef T0 type;
		};
		template<std::size_t J, std::size_t I, typename T0, typename... Ts>
		struct type_seq_element_impl<J, I, T0, Ts...> : public type_seq_element_impl<J, I + 1, Ts...>
		{
		};
	}	 // namespace detail

	template<std::size_t, typename...>
	struct type_seq_element;
	template<std::size_t I, typename... Ts>
	struct type_seq_element<I, type_seq_t<Ts...>> : detail::type_seq_element_impl<I, 0, Ts...>
	{
	};
	template<std::size_t I, typename Seq>
	using type_seq_element_t = typename type_seq_element<I, Seq>::type;

	namespace detail
	{
		template<std::size_t, typename...>
		struct type_seq_index_impl;
		template<std::size_t I, typename T, typename... Ts>
		struct type_seq_index_impl<I, T, T, Ts...> : std::integral_constant<std::size_t, I>
		{
			typedef T type;
		};
		template<std::size_t I, typename T, typename T0, typename... Ts>
		struct type_seq_index_impl<I, T, T0, Ts...> : type_seq_index_impl<I + 1, T, Ts...>
		{
		};
	}	 // namespace detail

	template<typename, typename...>
	struct type_seq_index;
	template<typename T, typename... Ts>
	struct type_seq_index<T, type_seq_t<Ts...>> : detail::type_seq_index_impl<0, T, Ts...>
	{
	};
	template<typename T, typename Seq>
	constexpr auto type_seq_index_v = type_seq_index<T, Seq>::value;

	namespace detail
	{
		template<typename, std::size_t, std::size_t, typename...>
		struct get_type_seq_impl;
		template<std::size_t I, typename T0, typename... Ts>
		struct get_type_seq_impl<T0, I, I, T0, Ts...>
		{
			constexpr static decltype(auto) dispatch(T0 &&target_val, Ts &&...) { return std::forward<T0>(target_val); }
		};
		template<typename Target, std::size_t I, std::size_t J, typename T0, typename... Ts>
		struct get_type_seq_impl<Target, I, J, T0, Ts...>
		{
			constexpr static decltype(auto) dispatch(T0 &&, Ts &&...seq)
			{
				using next = get_type_seq_impl<Target, I, J + 1, Ts...>;
				return next::dispatch(std::forward<Ts>(seq)...);
			}
		};
	}	 // namespace detail

	/** Forwards Nth argument. */
	template<std::size_t N, typename... Args>
	[[nodiscard]] constexpr decltype(auto) get(type_seq_t<Args...>, Args &&...args)
	{
		using impl_type = detail::get_type_seq_impl<type_seq_element_t<N, type_seq_t<Args...>>, N, 0, Args...>;
		return impl_type::dispatch(std::forward<Args>(args)...);
	}

	template<typename T>
	struct type_selector_t
	{
		using type = T;
	};
	template<typename T>
	constexpr type_selector_t<T> type_selector;

	namespace detail
	{
		template<typename...>
		struct is_template_impl : std::false_type
		{
		};
		template<template<typename...> typename T>
		struct is_template_impl<T<>> : std::true_type
		{
			constexpr static std::size_t count = 0;
		};
		template<template<typename...> typename T, typename... Ts>
		struct is_template_impl<T<Ts...>> : std::true_type
		{
			constexpr static std::size_t count = sizeof...(Ts);
		};
	}	 // namespace detail

	template<typename T>
	struct is_template : detail::is_template_impl<T>
	{
	};
	template<typename T>
	constexpr auto is_template_v = is_template<T>::value;
	template<typename T>
	concept template_type = is_template_v<T>;

	namespace detail
	{
		template<template<typename...> typename, typename>
		struct is_template_instance_impl : std::false_type
		{
		};
		template<template<typename...> typename T, typename... Ts>
		struct is_template_instance_impl<T, T<Ts...>> : std::true_type
		{
		};
	}	 // namespace detail

	template<template_type U, template<typename...> typename T>
	struct is_template_instance : detail::is_template_instance_impl<T, U>
	{
	};
	template<template_type U, template<typename...> typename T>
	constexpr auto is_template_instance_v = is_template_instance<U, T>::value;
	template<typename U, template<typename...> typename T>
	concept template_type_instance = is_template_instance_v<U, T>;

	template<template_type T>
	constexpr std::size_t template_extent = detail::is_template_impl<T>::count;

	template<typename, std::size_t I>
	struct pack_member;
	template<template<typename...> typename T, typename... Ts, std::size_t I>
	struct pack_member<T<Ts...>, I> : detail::type_seq_element_impl<I, 0, Ts...>
	{
	};

	template<template_type T, std::size_t I = 0>
	using pack_member_t = typename pack_member<T, I>::type;

	namespace detail
	{
		template<std::size_t I, auto Pred, std::size_t... Is>
		constexpr static auto type_index_sequence_impl(type_seq_t<>, std::index_sequence<Is...>) noexcept
		{
			return std::index_sequence<Is...>{};
		}
		template<std::size_t I, auto Pred, typename T, typename... Ts, std::size_t... Is>
		constexpr static auto type_index_sequence_impl(std::index_sequence<Is...>) noexcept
		{
			if constexpr (Pred(type_selector<T>))
				return type_index_sequence_impl<I + 1, Pred, Ts...>(std::index_sequence<I, Is...>{});
			else
				return type_index_sequence_impl<I + 1, Pred, Ts...>(std::index_sequence<Is...>{});
		}
		template<auto Pred, typename... Ts>
		constexpr static auto type_index_sequence_impl() noexcept
		{
			return type_index_sequence_impl<0, Pred, Ts...>(std::index_sequence<>{});
		}
	}	 // namespace detail

	template<auto Pred, typename... Ts>
	using filter_index_sequence = decltype(detail::type_index_sequence_impl<Pred, Ts...>());

	/** @brief Structure used to define a compile-time constant instance of an NTTP variable.
	 * @tparam Value NTTP object to create constant of. */
	template<auto Value>
	struct auto_constant
	{
		constexpr static auto value = Value;
	};

	/** @brief Structure used to define a compile-time non-constant global instance of a type.
	 * @tparam T Type of the global value.
	 * @tparam Value NTTP value used to initialize the type instance. */
	template<typename T, auto Value>
	struct mutable_global
	{
		constinit static T value;
	};
	template<typename T, auto V>
	constinit T mutable_global<T, V>::value = V;

	template<typename T, auto... Values>
	struct is_in_values : is_in<T, std::remove_const_t<decltype(Values)>...>
	{
	};

	/** Constant that evaluates to true if the passed NTTP pack contains a value of specific type. */
	template<typename T, auto... Values>
	constexpr bool is_in_values_v = is_in_values_v<T, Values...>;

	template<auto V>
	constexpr bool contains_value() noexcept
	{
		return false;
	}
	template<auto V, auto Elem, auto... Values>
	constexpr bool contains_value() noexcept
	{
		if constexpr (V == Elem)
			return true;
		else
			return contains_value<V, Values...>();
	}

	/** Constant that evaluates to true if the passed NTTP pack contains a value. */
	template<auto V, auto... Values>
	constexpr bool contains_value_v = contains_value<V, Values...>();

	/** Concept used to check if a range is a forward range with a specific value type. */
	template<typename R, typename T>
	concept forward_range_for = std::ranges::forward_range<R> && std::same_as<std::ranges::range_value_t<R>, T>;
	/** Concept used to check if an iterator is a forward iterator with a specific value type. */
	template<typename R, typename T>
	concept forward_iterator_for = std::forward_iterator<R> && std::same_as<std::iter_value_t<R>, T>;

	template<typename T>
	concept pointer_like = requires(T t)
	{
		std::is_object_v<T>;
		*t;
		t.operator->();
	};

	template<typename T>
	constexpr bool is_pointer_like_v = false;
	template<pointer_like T>
	constexpr bool is_pointer_like_v<T> = true;

	template<typename T>
	concept trivial_type = std::is_trivial_v<T>;
	template<typename T>
	concept not_void = !std::is_void_v<T>;
}	 // namespace sek
