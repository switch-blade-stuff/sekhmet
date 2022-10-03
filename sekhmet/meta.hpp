/*
 * Created by switchblade on 2022-03-13
 */

#pragma once

#include <concepts>
#include <ranges>

#include "../define.h"
#include <type_traits>

namespace sek
{
	namespace detail
	{
		template<typename...>
		struct is_in;
		template<typename T>
		struct is_in<T> : std::false_type
		{
		};
		template<typename T0, typename... Ts>
		struct is_in<T0, T0, Ts...> : std::true_type
		{
		};
		template<typename T0, typename T1, typename... Ts>
		struct is_in<T0, T1, Ts...> : is_in<T0, Ts...>
		{
		};
	}	 // namespace detail

	/** @brief Helper type used to check if type `T` is in parameter pack `Ts`. */
	template<typename T, typename... Ts>
	struct is_in : detail::is_in<T, Ts...>
	{
	};
	/** @brief Alias for `is_in<T, Ts...>::value`. */
	template<typename T, typename... Ts>
	constexpr auto is_in_v = is_in<T, Ts...>::value;

	namespace detail
	{
		template<typename...>
		struct is_template : std::false_type
		{
		};
		template<template<typename...> typename T>
		struct is_template<T<>> : std::true_type
		{
			constexpr static std::size_t count = 0;
		};
		template<template<typename...> typename T, typename... Ts>
		struct is_template<T<Ts...>> : std::true_type
		{
			constexpr static std::size_t count = sizeof...(Ts);
		};

		template<template<typename...> typename, typename>
		struct is_template_instance : std::false_type
		{
		};
		template<template<typename...> typename T, typename... Ts>
		struct is_template_instance<T, T<Ts...>> : std::true_type
		{
		};
	}	 // namespace detail

	// clang-format off
	/** @brief Helper type used to check if type `T` is a template instance of template `U`. */
	template<typename T, template<typename...> typename U>
	struct is_template_instance : detail::is_template_instance<U, T>
	{
	};
	/** @brief Alias for `is_template_instance<T, U>::value`. */
	template<typename T, template<typename...> typename U>
	constexpr auto is_template_instance_v = is_template_instance<T, U>::value;
	// clang-format on

	/** @brief Concept used to check if type `T` is an instance of template `U`. */
	template<typename T, template<typename...> typename U>
	concept template_instance = is_template_instance_v<T, U>;

	namespace detail
	{
		template<typename, std::size_t, typename...>
		struct pack_element;
		template<typename T, typename U0, typename... Us>
		struct pack_element<T, 0, U0, Us...>
		{
			typedef U0 type;
		};
		template<typename T, std::size_t I, typename U0, typename... Us>
		struct pack_element<T, I, U0, Us...> : public pack_element<T, I - 1, Us...>
		{
		};

		// clang-format off
		template<template<typename...> typename T, typename... Ts, std::size_t I> requires(I < sizeof...(Ts))
		struct pack_element<T<Ts...>, I> : public pack_element<T<Ts...>, I, Ts...>
		{
		};
		// clang-format on
	}	 // namespace detail

	/** @brief Helper type used to obtain `I`th element of a template parameter pack of type `T`. */
	template<std::size_t I, typename T>
	struct pack_element : detail::pack_element<T, I>
	{
	};
	/** @brief Alias for `typename pack_element<T, I>::type` */
	template<std::size_t I, typename T>
	using pack_element_t = typename pack_element<I, T>::type;

	/** @brief Helper type used to pass a template type as a function argument. */
	template<typename T>
	struct type_selector_t
	{
		using type = T;
	};
	/** @brief Instance of `type_selector_t<T>`. */
	template<typename T>
	constexpr type_selector_t<T> type_selector;

	/** @brief Helper type used to pass a template index as a function argument. */
	template<std::size_t I>
	struct index_selector_t : std::integral_constant<std::size_t, I>
	{
	};
	/** @brief Instance of `index_selector_t<T>`. */
	template<std::size_t I>
	constexpr auto index_selector = index_selector_t<I>{};

	/** @brief Helper type used to group a sequence of types into a single type. */
	template<typename... Ts>
	struct type_seq_t
	{
		typedef type_seq_t<Ts...> type;
		constexpr static auto size = sizeof...(Ts);
	};

	/** @brief Instance of `type_seq_t<Ts...>`. */
	template<typename... Ts>
	constexpr auto type_seq = type_seq_t<Ts...>{};
	/** @brief Alias for `type_seq_t<Ts...>::size`. */
	template<typename... Ts>
	constexpr auto type_seq_size = type_seq_t<Ts...>::size;

	namespace detail
	{
		template<typename...>
		struct concat_seq;
		template<typename... Ts>
		struct concat_seq<type_seq_t<Ts...>>
		{
			using type = type_seq_t<Ts...>;
		};
		template<typename... Ts0, typename... Ts1>
		struct concat_seq<type_seq_t<Ts0...>, type_seq_t<Ts1...>>
		{
			using type = type_seq_t<Ts0..., Ts1...>;
		};
		template<typename... Ts0, typename... Ts1, typename... S>
		struct concat_seq<type_seq_t<Ts0...>, type_seq_t<Ts1...>, S...>
		{
			using type = typename concat_seq<type_seq_t<Ts0..., Ts1...>, S...>::type;
		};
	}	 // namespace detail

	/** @brief Helper type used to concatenate multiple type sequences together */
	template<template_instance<type_seq_t> S, template_instance<type_seq_t>... Rest>
	struct concat_type_seq : detail::concat_seq<S, Rest...>
	{
	};
	/** @brief Alias for `typename concat_type_seq<S, Rest...>::type` */
	template<template_instance<type_seq_t> S, template_instance<type_seq_t>... Rest>
	using concat_type_seq_t = typename concat_type_seq<S, Rest...>::type;

	namespace detail
	{
		template<typename, typename>
		struct is_in_seq;
		template<typename T, typename... Ts>
		struct is_in_seq<T, type_seq_t<Ts...>> : is_in<T, Ts...>
		{
		};
	}	 // namespace detail

	/** @brief Helper type used to check if type `T` is in type sequence `S`. */
	template<typename T, template_instance<type_seq_t> S>
	struct is_in_seq : detail::is_in_seq<T, S>
	{
	};
	/** @brief Alias for `is_in_seq<T, S>::value`. */
	template<typename T, template_instance<type_seq_t> S>
	constexpr auto is_in_seq_v = is_in_seq<T, S>::value;

	namespace detail
	{
		template<typename, typename, typename...>
		struct remove_from_seq;

		template<typename T, typename... Ts>
		struct remove_from_seq<T, type_seq_t<>, type_seq_t<Ts...>>
		{
			using type = type_seq_t<Ts...>;
		};
		template<typename T, typename... Us, typename... Ts>
		struct remove_from_seq<T, type_seq_t<T, Us...>, type_seq_t<Ts...>>
			: remove_from_seq<T, type_seq_t<Us...>, type_seq_t<Ts...>>
		{
		};
		template<typename T, typename U, typename... Us, typename... Ts>
		struct remove_from_seq<T, type_seq_t<U, Us...>, type_seq_t<Ts...>>
			: remove_from_seq<T, type_seq_t<Us...>, type_seq_t<U, Ts...>>
		{
		};

		template<typename T, typename... Ts>
		struct remove_from_seq<T, type_seq_t<Ts...>> : remove_from_seq<T, type_seq_t<Ts...>, type_seq_t<>>
		{
		};
	}	 // namespace detail

	/** @brief Helper type used to remove all occurrences of type `T` from type sequence `S`. */
	template<typename T, template_instance<type_seq_t> S>
	struct remove_from_seq : detail::remove_from_seq<T, S>
	{
	};
	/** @brief Alias for `typename remove_from_seq<T, S>::type`. */
	template<typename T, template_instance<type_seq_t> S>
	using remove_from_seq_t = typename remove_from_seq<T, S>::type;

	namespace detail
	{
		template<std::size_t, std::size_t, typename...>
		struct make_seq;

		template<std::size_t N, typename T, typename... Ts>
		struct make_seq<N, N, T, Ts...>
		{
			using type = type_seq_t<Ts...>;
		};
		template<std::size_t N, std::size_t I, typename T, typename... Ts>
		struct make_seq<N, I, T, Ts...> : make_seq<N, I + 1, T, T, Ts...>
		{
		};
	}	 // namespace detail

	/** @brief Helper type used to create a type sequence containing `N` instances of `T`. */
	template<std::size_t N, typename T>
	struct make_type_seq
	{
		using type = typename detail::make_seq<N, 0, T>::type;
	};
	/** @brief Alias for `typename make_type_seq<T, S>::type`. */
	template<std::size_t N, typename T>
	using make_type_seq_t = typename make_type_seq<N, T>::type;

	namespace detail
	{
		template<std::size_t, typename, typename>
		struct type_seq_index;
		template<std::size_t I, typename T, typename... Us>
		struct type_seq_index<I, T, type_seq_t<T, Us...>> : std::integral_constant<std::size_t, I>
		{
		};
		template<std::size_t I, typename T, typename U, typename... Us>
		struct type_seq_index<I, T, type_seq_t<U, Us...>> : type_seq_index<I + 1, T, type_seq_t<Us...>>
		{
		};
	}	 // namespace detail

	/** @brief Helper type used to obtain index of type `T` in type sequence `S`.
	 * @note If `T` is not present within `S`, `type_seq_index` is ill-formed. */
	template<typename T, template_instance<type_seq_t> S>
	struct type_seq_index : detail::type_seq_index<0, T, S>
	{
	};
	/** @brief Alias for `type_seq_index<T, S>::value` */
	template<typename T, template_instance<type_seq_t> S>
	constexpr auto type_seq_index_v = type_seq_index<T, S>::value;

	namespace detail
	{
		template<typename, typename...>
		struct unique_seq;
		template<>
		struct unique_seq<type_seq_t<>>
		{
			using type = type_seq_t<>;
		};
		template<typename S, typename... Ts>
		struct unique_seq<S, type_seq_t<Ts...>>
		{
			using type = type_seq_t<Ts...>;
		};

		// clang-format off
		template<typename S, typename... Ts, typename U, typename... Us> requires(is_in_v<U, Ts...>)
		struct unique_seq<S, type_seq_t<Ts...>, U, Us...> : unique_seq<S, type_seq_t<Ts...>, Us...>
		{
		};
		template<typename S, typename... Ts, typename U, typename... Us> requires(!is_in_v<U, Ts...>)
		struct unique_seq<S, type_seq_t<Ts...>, U, Us...> : unique_seq<S, type_seq_t<U, Ts...>, Us...>
		{
		};
		// clang-format on

		template<typename... Ts>
		struct unique_seq<type_seq_t<Ts...>> : unique_seq<type_seq_t<Ts...>, type_seq_t<>, Ts...>
		{
		};
	}	 // namespace detail

	/** @brief Helper type used to remove all duplicates from type sequence `S`. */
	template<template_instance<type_seq_t> S>
	struct unique_type_seq : detail::unique_seq<S>
	{
	};
	/** @brief Alias for `typename unique_type_seq<S>::type`. */
	template<template_instance<type_seq_t> S>
	using unique_type_seq_t = typename unique_type_seq<S>::type;

	namespace detail
	{
		template<typename, std::size_t, std::size_t, typename...>
		struct get_seq;
		template<typename T, std::size_t I, typename... Ts>
		struct get_seq<T, I, I, T, Ts...>
		{
			[[nodiscard]] constexpr static decltype(auto) dispatch(T &&val, Ts &&...) noexcept
			{
				return std::forward<T>(val);
			}
		};
		template<typename T, std::size_t I, std::size_t J, typename U, typename... Us>
		struct get_seq<T, I, J, U, Us...>
		{
			[[nodiscard]] constexpr static decltype(auto) dispatch(U &&, Us &&...seq) noexcept
			{
				return get_seq<T, I, J + 1, Us...>::dispatch(std::forward<Us>(seq)...);
			}
		};
	}	 // namespace detail

	/** @brief Forwards `N`th argument of `args`. */
	template<std::size_t N, typename... Args>
	[[nodiscard]] constexpr decltype(auto) get(type_seq_t<Args...>, Args &&...args)
	{
		using impl_type = detail::get_seq<pack_element_t<N, type_seq_t<Args...>>, N, 0, Args...>;
		return impl_type::dispatch(std::forward<Args>(args)...);
	}

	/** @brief Structure used to define a compile-time constant instance of an NTTP variable.
	 * @tparam Value NTTP object to create constant of. */
	template<auto Value>
	struct auto_constant
	{
		constexpr static auto value = Value;
	};

	// clang-format off
	/** Concept used to check if a range is a forward range with a specific value type. */
	template<typename R, typename T>
	concept forward_range_for = std::ranges::forward_range<R> && std::same_as<std::ranges::range_value_t<R>, T>;
	/** Concept used to check if an iterator is a forward iterator with a specific value type. */
	template<typename R, typename T>
	concept forward_iterator_for = std::forward_iterator<R> && std::same_as<std::iter_value_t<R>, T>;
	// clang-format on

	namespace detail
	{
		template<typename From, typename To>
		struct transfer_cv
		{
			using type = std::remove_cv_t<To>;
		};
		template<typename From, typename To>
		struct transfer_cv<const From, To>
		{
			using type = std::add_const_t<std::remove_volatile_t<To>>;
		};
		template<typename From, typename To>
		struct transfer_cv<volatile From, To>
		{
			using type = std::add_volatile_t<std::remove_const_t<To>>;
		};
		template<typename From, typename To>
		struct transfer_cv<const volatile From, To>
		{
			using type = std::add_cv_t<To>;
		};
	}	 // namespace detail

	/** @brief Helper type used to transfer `const` & `volatile` qualifiers of the `From` type to the `To` type. */
	template<typename From, typename To>
	struct transfer_cv : detail::transfer_cv<From, To>
	{
		using type = std::remove_cv_t<To>;
	};
	/** @brief Alias for `typename transfer_cv<From, To>::type`. */
	template<typename From, typename To>
	using transfer_cv_t = typename transfer_cv<From, To>::type;

	namespace detail
	{
		template<typename From, typename To>
		struct is_preserving_cv_cast : std::true_type
		{
		};
		template<typename From, typename To>
		struct is_preserving_cv_cast<const From, To> : std::is_const<To>
		{
		};
		template<typename From, typename To>
		struct is_preserving_cv_cast<volatile From, To> : std::is_volatile<To>
		{
		};
		template<typename From, typename To>
		struct is_preserving_cv_cast<const volatile From, To> : std::conjunction<std::is_const<To>, std::is_volatile<To>>
		{
		};
	}	 // namespace detail

	/** @brief Helper type used to check if a cast from `From` to `To` will not cast away qualifiers. */
	template<typename From, typename To>
	struct is_preserving_cv_cast : detail::is_preserving_cv_cast<From, To>
	{
	};
	/** @brief Alias for `is_preserving_cv_cast<From, To>::value`. */
	template<typename From, typename To>
	constexpr auto is_preserving_cv_cast_v = is_preserving_cv_cast<From, To>::value;

	// clang-format off
	/** @brief Concept used to check if type `T` is tuple-like (has a well-formed `std::tuple_size` overload). */
	template<typename T>
	concept tuple_like = !std::is_reference_v<T> && requires(T t)
	{
		typename std::tuple_size<T>::type;
		requires std::derived_from<std::tuple_size<T>, std::integral_constant<std::size_t, std::tuple_size_v<T>>>;
	};
	/** @brief Concept used to check if type `T` is pair-like (has a `first` and `second` members). */
	template<typename T>
	concept pair_like = requires(T &p)
	{
		p.first;
		p.second;
	};
	// clang-format on
}	 // namespace sek
