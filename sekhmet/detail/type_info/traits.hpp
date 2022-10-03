//
// Created by switch_blade on 2022-10-03.
//

#pragma once

#include <type_traits>

#include "fwd.hpp"

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
	template<typename... Ts>
	concept any_args = std::conjunction_v<std::disjunction<std::is_same<std::decay_t<Ts>, any_ref>>,
	                                                       std::is_same<std::decay_t<Ts>, any>...>;
	// clang-format on
}	 // namespace sek::detail