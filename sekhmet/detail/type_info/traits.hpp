//
// Created by switch_blade on 2022-10-03.
//

#pragma once

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
}	 // namespace sek::detail