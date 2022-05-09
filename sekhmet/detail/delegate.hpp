//
// Created by switchblade on 09/05/22.
//

#pragma once

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

	template<typename...>
	class adapter_proxy;

	namespace detail
	{
		template<typename... Ts>
		constexpr adapter_proxy<Ts...> adapter_proxy_parent(adapter_proxy<Ts...>) noexcept;
		template<typename P>
		using adapter_proxy_parent_t = decltype(adapter_proxy_parent(std::declval<P>()));
		template<typename P>
		concept valid_adapter_proxy = (std::is_base_of_v<adapter_proxy<>, P> &&
									   template_extent<adapter_proxy_parent_t<P>> != 0);

		// clang-format off
		template<typename R, typename F, typename... Args>
		concept delegate_free_func = std::is_function_v<F> && std::is_invocable_r_v<R, F, Args...>;
		template<typename R, typename F, typename I, typename... Args>
		concept delegate_mem_func = std::is_member_function_pointer_v<F> &&
									requires(F f, I *i, Args &&...args) {
										{ (i->*f)(std::forward<Args>(args)...) } -> std::same_as<R>;
									};
		// clang-format on
	}	 // namespace detail

	template<detail::valid_adapter_proxy... ProxyTs>
	class adapter;

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

	/** @brief Type-erased function wrapper.
	 *
	 * Delegate is a more lightweight alternative to `std::function` providing a slightly different set of features.
	 * As opposed to `std::function`, delegates do not allocate any memory (and thus cannot use stateful functors),
	 * instead if a member function needs to be called, delegates can be bound to an instance of a type. */
	template<typename Sign>
	class delegate;

	template<typename Ret, typename... Args>
	class delegate<Ret(Args...)>
	{
		template<typename...>
		friend class adapter_proxy;

	private:
		template<auto F>
		constexpr static bool free_func = detail::delegate_free_func<Ret, decltype(F), Args...>;
		template<auto F, typename I>
		constexpr static bool mem_func = detail::delegate_mem_func<Ret, decltype(F), I, Args...>;

		constexpr delegate(Ret (*proxy)(const void *, Args &&...), const void *data) noexcept : proxy(proxy), data(data)
		{
		}

	public:
		/** Initializes an empty (non-bound) delegate. */
		constexpr delegate() noexcept = default;

		// clang-format off
		/** Initializes a delegate from a free function. */
		template<auto F>
		constexpr delegate(func_t<F>) noexcept requires free_func<F>
		{
			assign<F>();
		}
		/** Initializes a delegate from a member function and an instance pointer. */
		template<auto F, typename I>
		constexpr delegate(func_t<F>, I *instance) noexcept requires mem_func<F, I>
		{
			assign<F>(instance);
		}
		/** Initializes a delegate from a member function and an instance reference. */
		template<auto F, typename I>
		constexpr delegate(func_t<F>, I &instance) noexcept requires mem_func<F, I>
		{
			assign<F>(instance);
		}

		/** Binds a free function to the delegate. */
		template<auto F>
		constexpr delegate &assign() noexcept requires free_func<F>
		{
			proxy = +[](const void *, Args &&...args) { return F(std::forward<Args>(args)...); };
			data = nullptr;
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

		/** Binds a member function to the delegate. */
		template<auto F, typename I>
		constexpr delegate &assign(I *instance) noexcept requires mem_func<F, I>
		{
			proxy = +[](const void *p, Args &&...args)
			{
				using U = std::add_const_t<I>;
				return (const_cast<I *>(static_cast<U *>(p))->*F)(std::forward<Args>(args)...);
			};
			data = static_cast<const void *>(instance);
			return *this;
		}
		/** @copydoc assign */
		template<auto F, typename I>
		constexpr delegate &assign(func_t<F>, I *instance) noexcept requires mem_func<F, I>
		{
			return assign<F>(instance);
		}
		/** Binds a member function to the delegate. */
		template<auto F, typename I>
		constexpr delegate &assign(I &instance) noexcept requires mem_func<F, I>
		{
			proxy = +[](const void *p, Args &&...args)
			{
				using U = std::add_const_t<I>;
				return (const_cast<I *>(static_cast<U *>(p))->*F)(std::forward<Args>(args)...);
			};
			data = static_cast<const void *>(&instance);
			return *this;
		}
		/** @copydoc assign */
		template<auto F, typename I>
		constexpr delegate &assign(func_t<F>, I &instance) noexcept requires mem_func<F, I>
		{
			return assign<F>(instance);
		}
		// clang-format on

		/** Checks if the delegate is bound to a function. */
		[[nodiscard]] constexpr bool valid() const noexcept { return proxy != nullptr; }

		/** Invokes the bound function.
		 * @param args Arguments passed to the function.
		 * @return Value returned by the function.
		 * @throw delegate_error If the delegate is not bound to a function. */
		constexpr Ret invoke(Args &&...args) const
		{
			if (valid()) [[likely]]
				return proxy(data, std::forward<Args>(args)...);
			else
				throw delegate_error();
		}
		/** @copydoc invoke */
		constexpr Ret operator()(Args &&...args) const { return invoke(std::forward<Args>(args)...); }

	private:
		Ret (*proxy)(const void *, Args &&...) = nullptr;
		/* Optional. No mutable version, const_cast is used instead. */
		const void *data = nullptr;
	};

	template<typename R, typename... Args, R (*F)(Args...)>
	delegate(func_t<F>) -> delegate<R(Args...)>;
	template<typename R, typename... Args, R (&F)(Args...)>
	delegate(func_t<F>) -> delegate<R(Args...)>;

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