//
// Created by switch_blade on 2022-10-03.
//

#pragma once

#include "fwd.hpp"
#include <type_traits>

namespace sek::detail
{
	template<typename R, typename... Args, R (*F)(Args...)>
	struct func_traits<F>
	{
		using return_type = R;
		using arg_types = type_seq_t<Args...>;
	};
	template<typename R, typename I, typename... Args, R (I::*F)(Args...)>
	struct func_traits<F>
	{
		using return_type = R;
		using instance_type = I;
		using arg_types = type_seq_t<Args...>;
	};
	template<typename R, typename I, typename... Args, R (I::*F)(Args...) const>
	struct func_traits<F>
	{
		using return_type = R;
		using instance_type = const I;
		using arg_types = type_seq_t<Args...>;
	};
	template<typename R, typename I, typename... Args, R (I::*F)(Args...) volatile>
	struct func_traits<F>
	{
		using return_type = R;
		using instance_type = volatile I;
		using arg_types = type_seq_t<Args...>;
	};
	template<typename R, typename I, typename... Args, R (I::*F)(Args...) const volatile>
	struct func_traits<F>
	{
		using return_type = R;
		using instance_type = const volatile I;
		using arg_types = type_seq_t<Args...>;
	};

	// clang-format off
	template<typename T, typename... Ts>
	concept allowed_types = std::disjunction_v<std::is_same<std::remove_cvref_t<Ts>, T>...>;
	template<typename T>
	concept string_like_type = std::ranges::contiguous_range<T> && std::constructible_from<
		std::basic_string_view<std::ranges::range_value_t<T>>, std::ranges::iterator_t<T>, std::ranges::iterator_t<T>>;
	// clang-format on
}	 // namespace sek::detail