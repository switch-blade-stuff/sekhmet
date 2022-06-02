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
 * Created by switchblade on 09/05/22
 */

#pragma once

#include <bit>
#include <stdexcept>
#include <utility>

#include "define.h"

namespace sek
{
	/** @brief Exception thrown when a non-bound delegate is invoked. */
	class delegate_error : public std::runtime_error
	{
	public:
		delegate_error() : runtime_error("Invoked an empty (non-bound) delegate") {}
		~delegate_error() override = default;
	};

	namespace detail
	{
		// clang-format off
		template<typename...>
		struct is_delegate_compatible : std::false_type {};
		template<typename RDel, typename... ArgsDel, typename RFunc, typename... ArgsFunc>
		requires (sizeof...(ArgsDel) == sizeof...(ArgsFunc))
		struct is_delegate_compatible<RDel(ArgsDel...), RFunc(ArgsFunc...)>
			: std::conjunction<std::is_convertible<RFunc, RDel>, std::is_convertible<ArgsDel, ArgsFunc>...>
		{
		};
		template<typename SignD, typename SignF>
		constexpr auto is_delegate_compatible_v = is_delegate_compatible<SignD, SignF>::value;

		template<auto, typename>
		struct is_delegate_func : std::false_type {};
		template<typename RF, typename... ArgsF, RF (*F)(ArgsF...), typename RD, typename... ArgsD>
		struct is_delegate_func<F, RD(ArgsD...)> : is_delegate_compatible<RD(ArgsD...), RF(ArgsF...)> {};
		template<typename RF, typename I, typename... ArgsF, RF (I::*F)(ArgsF...), typename RD, typename... ArgsD>
		struct is_delegate_func<F, RD(ArgsD...)> : is_delegate_compatible<RD(ArgsD...), RF(ArgsF...)> {};
		template<typename RF, typename I, typename... ArgsF, RF (I::*F)(ArgsF...) const, typename RD, typename... ArgsD>
		struct is_delegate_func<F, RD(ArgsD...)> : is_delegate_compatible<RD(ArgsD...), RF(ArgsF...)> {};
		template<typename RF, typename I, typename... ArgsF, RF (I::*F)(ArgsF...) volatile, typename RD, typename... ArgsD>
		struct is_delegate_func<F, RD(ArgsD...)> : is_delegate_compatible<RD(ArgsD...), RF(ArgsF...)> {};
		template<typename RF, typename I, typename... ArgsF, RF (I::*F)(ArgsF...) const volatile, typename RD, typename... ArgsD>
		struct is_delegate_func<F, RD(ArgsD...)> : is_delegate_compatible<RD(ArgsD...), RF(ArgsF...)> {};
		template<auto F, typename Sign>
		constexpr auto is_delegate_func_v = is_delegate_func<F, Sign>::value;
		// clang-format on
	}	 // namespace detail

	// clang-format off
	/** @brief Helper type used to specify a compile-time function. */
	template<auto F>
	struct func_t;

	template<auto F> requires std::is_function_v<decltype(F)>
	struct func_t<F>
	{
	};
	template<auto F> requires std::is_member_function_pointer_v<decltype(F)>
	struct func_t<F>
	{
	};
	// clang-format on

	template<typename>
	class delegate;

	/** @brief Type-erased function wrapper.
	 *
	 * Delegate is a more lightweight alternative to `std::function` providing a slightly different set of features.
	 * As opposed to `std::function`, delegates do not allocate any memory (and thus cannot use stateful functors),
	 * instead if a member function needs to be called, delegates can be bound to an instance of a type.
	 *
	 * @tparam R Type returned by the delegate.
	 * @tparam Args Types of arguments used to invoke the delegate. */
	template<typename R, typename... Args>
	class delegate<R(Args...)>
	{
	private:
		// clang-format off
		template<typename RF, typename... ArgsF>
		constexpr static bool compatible_sign = detail::is_delegate_compatible_v<R(Args...), RF(ArgsF...)>;
		template<auto F, typename... Inject>
		constexpr static bool compatible_func = detail::is_delegate_func_v<F, R(Inject..., Args...)>;
		template<auto F, typename... Inject>
		constexpr static bool free_func = std::is_function_v<decltype(F)> && compatible_func<F, Inject...>;
		template<auto F>
		constexpr static bool mem_func = std::is_member_function_pointer_v<decltype(F)> && compatible_func<F>;
		template<typename T, typename... Inject>
		constexpr static bool empty_ftor = std::is_object_v<T> && std::is_empty_v<T> && std::is_invocable_r_v<R, T, Inject..., Args...>;
		// clang-format on

		constexpr delegate(R (*proxy)(const void *, Args...), const void *data) noexcept : proxy(proxy), data_ptr(data)
		{
		}

	public:
		/** Initializes an empty (non-bound) delegate. */
		constexpr delegate() noexcept = default;

		// clang-format off
		/** Initializes a delegate from a free function pointer. */
		template<typename RF, typename... ArgsF>
		constexpr delegate(RF (*f)(ArgsF...)) noexcept requires compatible_sign<RF, ArgsF...>
		{
			assign(f);
		}
		/** Initializes a delegate from a free function pointer and a bound argument. */
		template<typename Arg>
		constexpr delegate(R (*f)(Arg *, Args...), Arg *arg) noexcept
		{
			assign(f, arg);
		}
		/** @copydoc delegate */
		template<typename Arg>
		constexpr delegate(R (*f)(Arg *, Args...), Arg &arg) noexcept
		{
			assign(f, arg);
		}
		/** @copydoc delegate */
		template<typename Arg>
		constexpr delegate(R (*f)(Arg &, Args...), Arg &arg) noexcept
		{
			assign(f, arg);
		}

		/** Initializes a delegate from an empty functor. */
		template<typename F>
		constexpr delegate(F ftor) noexcept requires empty_ftor<F>
		{
			assign(std::forward<F>(ftor));
		}
		/** Initializes a delegate from an empty functor and a bound argument. */
		template<typename F, typename Arg>
		constexpr delegate(F ftor, Arg *arg) noexcept requires empty_ftor<F, Arg *>
		{
			assign(std::forward<F>(ftor), arg);
		}
		/** @copydoc delegate */
		template<typename F, typename Arg>
		constexpr delegate(F ftor, Arg &arg) noexcept requires empty_ftor<F, Arg *>
		{
			assign(std::forward<F>(ftor), arg);
		}
		/** @copydoc delegate */
		template<typename F, typename Arg>
		constexpr delegate(F ftor, Arg &arg) noexcept requires empty_ftor<F, Arg &>
		{
			assign(std::forward<F>(ftor), arg);
		}

		/** Initializes a delegate from a free function. */
		template<auto F>
		constexpr delegate(func_t<F>) noexcept requires free_func<F>
		{
			assign<F>();
		}
		/** Initializes a delegate from a free and a bound argument. */
		template<auto F, typename Arg>
		constexpr delegate(func_t<F>, Arg *arg) noexcept requires free_func<F, Arg *>
		{
			assign<F>(arg);
			data_ptr = static_cast<const void *>(arg);
		}
		/** @copydoc delegate */
		template<auto F, typename Arg>
		constexpr delegate(func_t<F>, Arg &arg) noexcept requires free_func<F, Arg *>
		{
			assign<F>(arg);
			data_ptr = static_cast<const void *>(std::addressof(arg));
		}
		/** @copydoc delegate */
		template<auto F, typename Arg>
		constexpr delegate(func_t<F>, Arg &arg) noexcept requires free_func<F, Arg &>
		{
			assign<F>(arg);
			data_ptr = static_cast<const void *>(std::addressof(arg));
		}
		/** Initializes a delegate from a member function and an instance pointer. */
		template<auto F, typename I>
		constexpr delegate(func_t<F>, I *instance) noexcept requires mem_func<F>
		{
			assign<F>(instance);
		}
		/** Initializes a delegate from a member function and an instance reference. */
		template<auto F, typename I>
		constexpr delegate(func_t<F>, I &instance) noexcept requires mem_func<F>
		{
			assign<F>(instance);
		}

		/** Binds a free function pointer to the delegate. */
		template<typename RF, typename... ArgsF>
		constexpr delegate &assign(RF (*f)(ArgsF...)) noexcept requires compatible_sign<RF, ArgsF...>
		{
			proxy = +[](const void *p, Args ...args) -> R { return std::bit_cast<RF (*)(ArgsF...)>(p)(std::forward<Args>(args)...); };
			data_ptr = std::bit_cast<const void *>(f);
			return *this;
		}
		/** @copydoc assign */
		template<typename RF, typename... ArgsF>
		constexpr delegate &operator=(RF (*f)(ArgsF...)) noexcept requires compatible_sign<RF, ArgsF...>
		{
			return assign(f);
		}
		/** Binds a free function pointer and an argument to the delegate. */
		template<typename Arg>
		constexpr delegate &assign(R (*f)(Arg *, Args...), Arg *arg)  noexcept
		{
			proxy = std::bit_cast<R (*)(const void *, Args...)>(f);
			data_ptr = static_cast<const void *>(arg);
			return *this;
		}
		/** @copydoc assign */
		template<typename Arg>
		constexpr delegate &assign(R (*f)(Arg *, Args...), Arg &arg)  noexcept
		{
			proxy = std::bit_cast<R (*)(const void *, Args...)>(f);
			data_ptr = static_cast<const void *>(std::addressof(arg));
			return *this;
		}
		/** @copydoc assign */
		template<typename Arg>
		constexpr delegate &assign(R (*f)(Arg &, Args...), Arg &arg)  noexcept
		{
			proxy = std::bit_cast<R (*)(const void *, Args...)>(f);
			data_ptr = static_cast<const void *>(std::addressof(arg));
			return *this;
		}

		/** Binds an empty functor to the delegate. */
		template<typename F>
		constexpr delegate &assign(F) noexcept requires empty_ftor<F>
		{
			proxy = +[](const void *, Args...args) -> R
			{
				return F{}(std::forward<Args>(args)...);
			};
			data_ptr = nullptr;
			return *this;
		}
		/** @copydoc assign */
		template<typename F>
		constexpr delegate &operator=(F f) noexcept requires empty_ftor<F>
		{
			return assign(std::forward<F>(f));
		}
		/** Binds an empty functor and an argument to the delegate. */
		template<typename F, typename Arg>
		constexpr delegate &assign(F, Arg *arg) noexcept requires empty_ftor<F, Arg *>
		{
			proxy = +[](const void *p, Args...args) -> R
			{
				using U = std::add_const_t<Arg>;
				return F{}(const_cast<Arg *>(static_cast<U *>(p)), std::forward<Args>(args)...);
			};
			data_ptr = static_cast<const void *>(arg);
			return *this;
		}
		/** @copydoc assign */
		template<typename F, typename Arg>
		constexpr delegate &assign(F, Arg &arg) noexcept requires empty_ftor<F, Arg *>
		{
			proxy = +[](const void *p, Args...args) -> R
			{
				using U = std::add_const_t<Arg>;
				return F{}(const_cast<Arg *>(static_cast<U *>(p)), std::forward<Args>(args)...);
			};
			data_ptr = static_cast<const void *>(std::addressof(arg));
			return *this;
		}
		/** @copydoc assign */
		template<typename F, typename Arg>
		constexpr delegate &assign(F, Arg &arg) noexcept requires empty_ftor<F, Arg &>
		{
			proxy = +[](const void *p, Args...args) -> R
			{
				using U = std::add_const_t<Arg>;
				return F{}(*const_cast<Arg *>(static_cast<U *>(p)), std::forward<Args>(args)...);
			};
			data_ptr = static_cast<const void *>(std::addressof(arg));
			return *this;
		}

		/** Binds a free function to the delegate. */
		template<auto F>
		constexpr delegate &assign() noexcept requires free_func<F>
		{
			proxy = +[](const void *, Args...args) -> R { return F(std::forward<Args>(args)...); };
			data_ptr = nullptr;
			return *this;
		}
		/** @copydoc assign */
		template<auto F>
		constexpr delegate &assign(func_t<F>) noexcept requires free_func<F>
		{
			return assign<F>();
		}
		/** @copydoc assign */
		template<auto F>
		constexpr delegate &operator=(func_t<F>) noexcept requires free_func<F>
		{
			return assign<F>();
		}

		/** Binds a free function and an argument to the delegate. */
		template<auto F, typename Arg>
		constexpr delegate &assign(Arg *arg) noexcept requires free_func<F, Arg *>
		{
			proxy = +[](const void *p, Args...args) -> R
			{
				using U = std::add_const_t<Arg>;
				return F(const_cast<Arg *>(static_cast<U *>(p)), std::forward<Args>(args)...);
			};
			data_ptr = static_cast<const void *>(arg);
			return *this;
		}
		/** @copydoc assign */
		template<auto F, typename Arg>
		constexpr delegate &assign(Arg &arg) noexcept requires free_func<F, Arg *>
		{
			proxy = +[](const void *p, Args...args) -> R
			{
				using U = std::add_const_t<Arg>;
				return F(const_cast<Arg *>(static_cast<U *>(p)), std::forward<Args>(args)...);
			};
			data_ptr = static_cast<const void *>(std::addressof(arg));
			return *this;
		}
		/** @copydoc assign */
		template<auto F, typename Arg>
		constexpr delegate &assign(Arg &arg) noexcept requires free_func<F, Arg &>
		{
			proxy = +[](const void *p, Args...args) -> R
			{
				using U = std::add_const_t<Arg>;
				return F(*const_cast<Arg *>(static_cast<U *>(p)), std::forward<Args>(args)...);
			};
			data_ptr = static_cast<const void *>(std::addressof(arg));
			return *this;
		}
		/** @copydoc assign */
		template<auto F, typename Arg>
		constexpr delegate &assign(func_t<F>, Arg *arg) noexcept requires free_func<F, Arg *>
		{
			return assign<F>(arg);
		}
		/** @copydoc assign */
		template<auto F, typename Arg>
		constexpr delegate &assign(func_t<F>, Arg &arg) noexcept requires free_func<F, Arg *>
		{
			return assign<F>(arg);
		}
		/** @copydoc assign */
		template<auto F, typename Arg>
		constexpr delegate &assign(func_t<F>, Arg &arg) noexcept requires free_func<F, Arg &>
		{
			return assign<F>(arg);
		}

		/** Binds a member function to the delegate. */
		template<auto F, typename I>
		constexpr delegate &assign(I *instance) noexcept requires mem_func<F>
		{
			proxy = +[](const void *p, Args...args) -> R
			{
				using U = std::add_const_t<I>;
				return (const_cast<I *>(static_cast<U *>(p))->*F)(std::forward<Args>(args)...);
			};
			data_ptr = static_cast<const void *>(instance);
			return *this;
		}
		/** @copydoc assign */
		template<auto F, typename I>
		constexpr delegate &assign(func_t<F>, I *instance) noexcept requires mem_func<F>
		{
			return assign<F>(instance);
		}
		/** Binds a member function to the delegate. */
		template<auto F, typename I>
		constexpr delegate &assign(I &instance) noexcept requires mem_func<F>
		{
			proxy = +[](const void *p, Args...args) -> R
			{
				using U = std::add_const_t<I>;
				return (const_cast<I *>(static_cast<U *>(p))->*F)(std::forward<Args>(args)...);
			};
			data_ptr = static_cast<const void *>(std::addressof(instance));
			return *this;
		}
		/** @copydoc assign */
		template<auto F, typename I>
		constexpr delegate &assign(func_t<F>, I &instance) noexcept requires mem_func<F>
		{
			return assign<F>(instance);
		}
		// clang-format on

		/** Checks if the delegate is bound to a function. */
		[[nodiscard]] constexpr bool valid() const noexcept { return proxy != nullptr; }
		/** Returns pointer to the data of the bound argument or object instance. */
		[[nodiscard]] constexpr const void *data() const noexcept { return data_ptr; }

		/** Invokes the bound function.
		 * @param args Arguments passed to the function.
		 * @return Value returned by the function.
		 * @throw delegate_error If the delegate is not bound to a function. */
		constexpr R invoke(Args... args) const
		{
			if (valid()) [[likely]]
				return proxy(data_ptr, std::forward<Args>(args)...);
			else
				throw delegate_error();
		}
		/** @copydoc invoke */
		constexpr R operator()(Args... args) const { return invoke(std::forward<Args>(args)...); }

		[[nodiscard]] constexpr bool operator==(const delegate &) const noexcept = default;

		constexpr void swap(delegate &other) noexcept
		{
			using std::swap;
			swap(proxy, other.proxy);
			swap(data_ptr, other.data_ptr);
		}
		friend constexpr void swap(delegate &a, delegate &b) noexcept { a.swap(b); }

	private:
		R (*proxy)(const void *, Args...) = nullptr;
		/* Optional. No mutable version, const_cast is used instead. */
		const void *data_ptr = nullptr;
	};

	template<typename R, typename... Args>
	delegate(R (*)(Args...)) -> delegate<R(Args...)>;
	template<typename R, typename Arg, typename... Args>
	delegate(R (*)(Arg *, Args...), Arg *) -> delegate<R(Args...)>;
	template<typename R, typename Arg, typename... Args>
	delegate(R (*)(Arg *, Args...), Arg &) -> delegate<R(Args...)>;
	template<typename R, typename Arg, typename... Args>
	delegate(R (*)(Arg &, Args...), Arg &) -> delegate<R(Args...)>;

	template<typename R, typename... Args, R (*F)(Args...)>
	delegate(func_t<F>) -> delegate<R(Args...)>;
	template<typename R, typename... Args, R (&F)(Args...)>
	delegate(func_t<F>) -> delegate<R(Args...)>;

	template<typename R, typename Arg, typename... Args, R (*F)(Arg *, Args...)>
	delegate(func_t<F>, Arg *) -> delegate<R(Args...)>;
	template<typename R, typename Arg, typename... Args, R (*F)(Arg *, Args...)>
	delegate(func_t<F>, Arg &) -> delegate<R(Args...)>;
	template<typename R, typename Arg, typename... Args, R (*F)(Arg &, Args...)>
	delegate(func_t<F>, Arg &) -> delegate<R(Args...)>;
	template<typename R, typename Arg, typename... Args, R (&F)(Arg *, Args...)>
	delegate(func_t<F>, Arg *) -> delegate<R(Args...)>;
	template<typename R, typename Arg, typename... Args, R (&F)(Arg *, Args...)>
	delegate(func_t<F>, Arg &) -> delegate<R(Args...)>;
	template<typename R, typename Arg, typename... Args, R (&F)(Arg &, Args...)>
	delegate(func_t<F>, Arg &) -> delegate<R(Args...)>;

	template<typename R, typename I, typename... Args, R (I::*F)(Args...)>
	delegate(func_t<F>, I *) -> delegate<R(Args...)>;
	template<typename R, typename I, typename... Args, R (I::*F)(Args...) const>
	delegate(func_t<F>, I *) -> delegate<R(Args...)>;
	template<typename R, typename I, typename... Args, R (I::*F)(Args...) const>
	delegate(func_t<F>, const I *) -> delegate<R(Args...)>;
	template<typename R, typename I, typename... Args, R (I::*F)(Args...) volatile>
	delegate(func_t<F>, I *) -> delegate<R(Args...)>;
	template<typename R, typename I, typename... Args, R (I::*F)(Args...) volatile>
	delegate(func_t<F>, volatile I *) -> delegate<R(Args...)>;
	template<typename R, typename I, typename... Args, R (I::*F)(Args...) const volatile>
	delegate(func_t<F>, I *) -> delegate<R(Args...)>;
	template<typename R, typename I, typename... Args, R (I::*F)(Args...) const volatile>
	delegate(func_t<F>, const I *) -> delegate<R(Args...)>;
	template<typename R, typename I, typename... Args, R (I::*F)(Args...) const volatile>
	delegate(func_t<F>, volatile I *) -> delegate<R(Args...)>;
	template<typename R, typename I, typename... Args, R (I::*F)(Args...) const volatile>
	delegate(func_t<F>, const volatile I *) -> delegate<R(Args...)>;

	template<typename R, typename I, typename... Args, R (I::*F)(Args...)>
	delegate(func_t<F>, I &) -> delegate<R(Args...)>;
	template<typename R, typename I, typename... Args, R (I::*F)(Args...) const>
	delegate(func_t<F>, I &) -> delegate<R(Args...)>;
	template<typename R, typename I, typename... Args, R (I::*F)(Args...) const>
	delegate(func_t<F>, const I &) -> delegate<R(Args...)>;
	template<typename R, typename I, typename... Args, R (I::*F)(Args...) volatile>
	delegate(func_t<F>, I &) -> delegate<R(Args...)>;
	template<typename R, typename I, typename... Args, R (I::*F)(Args...) volatile>
	delegate(func_t<F>, volatile I &) -> delegate<R(Args...)>;
	template<typename R, typename I, typename... Args, R (I::*F)(Args...) const volatile>
	delegate(func_t<F>, I &) -> delegate<R(Args...)>;
	template<typename R, typename I, typename... Args, R (I::*F)(Args...) const volatile>
	delegate(func_t<F>, const I &) -> delegate<R(Args...)>;
	template<typename R, typename I, typename... Args, R (I::*F)(Args...) const volatile>
	delegate(func_t<F>, volatile I &) -> delegate<R(Args...)>;
	template<typename R, typename I, typename... Args, R (I::*F)(Args...) const volatile>
	delegate(func_t<F>, const volatile I &) -> delegate<R(Args...)>;
}	 // namespace sek