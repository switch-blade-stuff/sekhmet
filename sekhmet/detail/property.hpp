/*
 * Created by switchblade on 18/08/22
 */

#pragma once

#include <functional>
#include <utility>

#include "define.h"
#include "ebo_base_helper.hpp"

namespace sek
{
	namespace detail
	{
		template<typename T>
		struct property_instance
		{
			constexpr property_instance() noexcept = default;
			constexpr explicit property_instance(T *value) noexcept : value(value) {}

			constexpr void swap(property_instance &other) noexcept { std::swap(value, other.value); }

			T *value = nullptr;
		};
		template<>
		struct property_instance<void>
		{
			constexpr void swap(property_instance &) noexcept {}
		};

		template<typename T>
		struct accessor_base : ebo_base_helper<T>
		{
			using base_t = ebo_base_helper<T>;

			// clang-format off
				constexpr accessor_base() noexcept(std::is_nothrow_default_constructible_v<T>) = default;
				constexpr accessor_base(const accessor_base &) noexcept(std::is_nothrow_copy_constructible_v<T>) = default;
				constexpr accessor_base &operator=(const accessor_base &) noexcept(std::is_nothrow_copy_assignable_v<T>) = default;
				constexpr accessor_base(accessor_base &&) noexcept(std::is_nothrow_move_constructible_v<T>) = default;
				constexpr accessor_base &operator=(accessor_base &&) noexcept(std::is_nothrow_move_assignable_v<T>) = default;

				template<typename... Args>
				constexpr explicit accessor_base(Args &&...args) noexcept(std::is_nothrow_constructible_v<T, Args...>)
					requires std::is_constructible_v<T, Args...> : base_t(std::forward<Args>(args)...)
				{
				}

				template<typename... Args>
				constexpr auto operator()(Args &&...args) noexcept(std::is_nothrow_invocable_v<T, Args...>)
					-> std::invoke_result_t<T, Args...> requires std::is_invocable_v<T, Args...>
				{
					return std::invoke(*base_t::get(), std::forward<Args>(args)...);
				}
				template<typename... Args>
				constexpr auto operator()(Args &&...args) const noexcept(std::is_nothrow_invocable_v<const T, Args...>)
					-> std::invoke_result_t<const T, Args...> requires std::is_invocable_v<const T, Args...>
				{
					return std::invoke(*base_t::get(), std::forward<Args>(args)...);
				}
			// clang-format on

			constexpr void swap(accessor_base &other) noexcept(std::is_nothrow_swappable_v<T>) { base_t::swap(other); }
		};
		template<>
		struct accessor_base<void>
		{
			constexpr accessor_base() noexcept = default;
			constexpr accessor_base(const accessor_base &) noexcept = default;
			constexpr accessor_base &operator=(const accessor_base &) noexcept = default;
			constexpr accessor_base(accessor_base &&) noexcept = default;
			constexpr accessor_base &operator=(accessor_base &&) noexcept = default;

			constexpr void swap(accessor_base &) noexcept {}
		};
	}	 // namespace detail

	/** @brief Helper structure used to wrap member and free function pointers for a property wrapper. */
	template<auto... Funcs>
	class accessor_t
	{
		// clang-format off
		template<auto F, auto... Fs, typename I, typename... Args>
		constexpr static decltype(auto) dispatch(I *instance, Args &&...args) requires std::is_member_function_pointer_v<decltype(F)>
		{
			if constexpr (!std::is_invocable_v<decltype(F), Args...>)
				return dispatch<Fs...>(std::forward<Args>(args)...);
			else
				return (instance->*F)(std::forward<Args>(args)...);
		}
		template<auto F, auto... Fs, typename... Args>
		constexpr static decltype(auto) dispatch(Args &&...args)
		{
			if constexpr (!std::is_invocable_v<decltype(F), Args...>)
				return dispatch<Fs...>(std::forward<Args>(args)...);
			else
				return std::invoke(F, std::forward<Args>(args)...);
		}
		// clang-format on

	public:
		template<typename... Args>
		constexpr decltype(auto) operator()(Args &&...args) const
		{
			return dispatch<Funcs...>(std::forward<Args>(args)...);
		}
	};
	/** Instance of the `accessor_t` helper type. */
	template<auto... Funcs>
	constexpr auto accessor = accessor_t<Funcs...>{};

	// clang-format off
	template<auto>
	struct member_getter_t;
	template<auto>
	struct member_setter_t;

	/** @brief Helper structure used to create a simple member object getter for a property wrapper. */
	template<typename T, typename I, T I::*M>
	struct member_getter_t<M>
	{
		[[nodiscard]] constexpr auto &operator()(I *instance) const noexcept
		{
			return instance->*M;
		}
		[[nodiscard]] constexpr auto &operator()(const I *instance) const noexcept
		{
			return instance->*M;
		}
	};
	/** @brief Helper structure used to create a simple member object setter for a property wrapper. */
	template<typename T, typename I, T I::*M>
	struct member_setter_t<M>
	{
		template<typename U>
		constexpr void operator()(I *instance, U &&value) const noexcept requires std::assignable_from<T &, U>
		{
			instance->*M = std::forward<U>(value);
		}
	};
	// clang-format on

	/** Instance of the `member_getter_t` helper type. */
	template<auto M>
	constexpr auto member_getter = member_getter_t<M>{};
	/** Instance of the `member_setter_t` helper type. */
	template<auto M>
	constexpr auto member_setter = member_setter_t<M>{};

	// clang-format off
	/** @brief Property wrapper used to invoke get & set functors when the underlying value is accessed.
	 *
	 * Property wrappers are used to provide member access without explicit setter & getter functors.
	 * An accessor function can simply return a property wrapper, which would then invoke the specified
	 * functors when the member is dereferenced or modified.
	 *
	 * @tparam Get Getter functor invoked when the property is dereferenced.
	 * @tparam Set Setter functor invoked when the property is assigned.
	 * @tparam I Instance type referenced by the wrapper and passed to the getter & setter functors.
	 * @note `Get`, `Set` and `I` may be set to `void`, in which case the corresponding functionality will be disabled. */
	template<typename Get, typename Set, typename I = void>
	class property_wrapper : detail::property_instance<I>, detail::accessor_base<Get>, detail::accessor_base<Set>
	{
		template<typename, typename, typename>
		friend class property_wrapper;

		using instance_base = detail::property_instance<I>;
		using getter_base = detail::accessor_base<Get>;
		using setter_base = detail::accessor_base<Set>;

		template<typename... Args>
		constexpr static bool is_gettable = std::disjunction_v<std::conjunction<std::is_invocable<getter_base, I *, Args...>,
		                                                                        std::negation<std::is_void<I>>>,
															   std::conjunction<std::is_invocable<getter_base, Args...>,
																				std::is_void<I>>>;
		template<typename... Args>
		constexpr static bool is_nothrow_gettable = std::disjunction_v<std::conjunction<std::is_nothrow_invocable<getter_base, I *, Args...>,
																				std::negation<std::is_void<I>>>,
																	   std::conjunction<std::is_nothrow_invocable<getter_base, Args...>,
																				std::is_void<I>>>;
		template<typename... Args>
		constexpr static bool is_settable = std::disjunction_v<std::conjunction<std::is_invocable<setter_base, I *, Args...>,
		                                                                        std::negation<std::is_void<I>>>,
															   std::conjunction<std::is_invocable<setter_base, Args...>,
																				std::is_void<I>>>;
		template<typename... Args>
		constexpr static bool is_nothrow_settable = std::disjunction_v<std::conjunction<std::is_nothrow_invocable<setter_base, I *, Args...>,
																						std::negation<std::is_void<I>>>,
																	   std::conjunction<std::is_nothrow_invocable<setter_base, Args...>,
																						std::is_void<I>>>;

	public:
		constexpr property_wrapper(const property_wrapper &) = default;
		constexpr property_wrapper &operator=(const property_wrapper &) = default;
		constexpr property_wrapper(property_wrapper &&) noexcept = default;
		constexpr property_wrapper &operator=(property_wrapper &&) noexcept = default;

		/** Default-initializes the property wrapper.
		 * @note This overload is available only if `std::is_void_v<I>` evaluates to `true`. */
		constexpr property_wrapper() noexcept(std::is_nothrow_default_constructible_v<getter_base> &&
											  std::is_nothrow_default_constructible_v<setter_base>)
			requires(std::is_void_v<I>) = default;
		/** Initializes property wrapper with the specified getter.
		 * @param get Value of the getter functor.
		 * @note This overload is available only if `std::is_void_v<I>` evaluates to `true` and
		 * `std::is_void_v<Get>` evaluates to `false`. */
		constexpr property_wrapper(const Get &get) noexcept(std::is_nothrow_default_constructible_v<setter_base> &&
															std::is_nothrow_copy_constructible_v<Get>)
			requires(std::is_copy_constructible_v<Get> && std::is_void_v<I> && !std::is_void_v<Get>)
			: getter_base(get)
		{
		}
		/** Initializes property wrapper with the specified setter.
		 * @param set Value of the setter functor.
		 * @note This overload is available only if `std::is_void_v<I>` evaluates to `true` and
		 * `std::is_void_v<Set>` evaluates to `false`. */
		constexpr property_wrapper(const Set &set) noexcept(std::is_nothrow_default_constructible_v<getter_base> &&
															std::is_nothrow_copy_constructible_v<Set>)
			requires(std::is_copy_constructible_v<Set> && std::is_void_v<I> && !std::is_void_v<Set>)
			: setter_base(set)
		{
		}
		/** Initializes property wrapper with the specified getter and setter.
		 * @param get Value of the getter functor.
		 * @param set Value of the setter functor.
		 * @note This overload is available only if `std::is_void_v<I>` evaluates to `true` and
		 * `std::is_void_v<Get>` and `std::is_void_v<Set>` evaluate to `false`. */
		constexpr property_wrapper(const Get &get, const Set &set) noexcept(std::is_nothrow_copy_constructible_v<Get> &&
																			std::is_nothrow_copy_constructible_v<Set>)
			requires(std::is_copy_constructible_v<Get> && std::is_copy_constructible_v<Set> &&
			         std::is_void_v<I> && !std::is_void_v<Get> && !std::is_void_v<Set>)
			: getter_base(get), setter_base(set)
		{
		}

		/** Initializes property wrapper with the specified instance.
		 * @param ptr Pointer to the bound instance.
		 * @note This overload is available only if `std::is_void_v<I>` evaluates to `false`. */
		constexpr property_wrapper(I *ptr) noexcept(std::is_nothrow_default_constructible_v<getter_base> &&
		                                            std::is_nothrow_default_constructible_v<setter_base>)
			requires(!std::is_void_v<I>) : instance_base(ptr) {}
		/** Initializes property wrapper with the specified instance and getter.
		 * @param ptr Pointer to the bound instance.
		 * @param get Value of the getter functor.
		 * @note This overload is available only if `std::is_void_v<I>` and `std::is_void_v<Get>` evaluate to `false`. */
		constexpr property_wrapper(I *ptr, const Get &get) noexcept(std::is_nothrow_default_constructible_v<setter_base> &&
																	std::is_nothrow_copy_constructible_v<Get>)
			requires(std::is_copy_constructible_v<Get> && !std::is_void_v<I> && !std::is_void_v<Get>)
			: instance_base(ptr), getter_base(get)
		{
		}
		/** Initializes property wrapper with the specified instance and setter.
		 * @param ptr Pointer to the bound instance.
		 * @param set Value of the setter functor.
		 * @note This overload is available only if `std::is_void_v<I>` and `std::is_void_v<Set>` evaluate to `false`. */
		constexpr property_wrapper(I *ptr, const Set &set) noexcept(std::is_nothrow_default_constructible_v<getter_base> &&
																	std::is_nothrow_copy_constructible_v<Set>)
			requires(std::is_copy_constructible_v<Set> && !std::is_void_v<I> && !std::is_void_v<Set>)
			: instance_base(ptr), setter_base(set)
		{
		}
		/** Initializes property wrapper with the specified instance, getter and setter.
		 * @param ptr Pointer to the bound instance.
		 * @param get Value of the getter functor.
		 * @param set Value of the setter functor.
		 * @note This overload is available only if `std::is_void_v<I>`, `std::is_void_v<Set>` and `std::is_void_v<Set>` evaluates to `false`. */
		constexpr property_wrapper(I *ptr, const Get &get, const Set &set) noexcept(std::is_nothrow_copy_constructible_v<Get> &&
																					std::is_nothrow_copy_constructible_v<Set>)
			requires(std::is_copy_constructible_v<Get> && std::is_copy_constructible_v<Set> &&
					 !std::is_void_v<I> && !std::is_void_v<Get> && !std::is_void_v<Set>)
			: instance_base(ptr), getter_base(get), setter_base(set)
		{
		}

		/** Copy-assigns the property wrapper.
		 * @note This overload is available only if the setter is not invocable with `const property_wrapper &`. */
		constexpr property_wrapper &operator=(const property_wrapper &other) requires (!is_settable<const property_wrapper &>)
		{
			instance_base::operator=(other);
			getter_base::operator=(other);
			setter_base::operator=(other);
			return *this;
		}
		/** Move-assigns the property wrapper.
		 * @note This overload is available only if the setter is not invocable with `property_wrapper &&`. */
		constexpr property_wrapper &operator=(property_wrapper &&other) noexcept requires (!is_settable<property_wrapper &&>)
		{
			instance_base::operator=(std::move(other));
			getter_base::operator=(std::move(other));
			setter_base::operator=(std::move(other));
			return *this;
		}

		/** Copy-assigns the property wrapper from another who's instance type is not const-qualified.
		 * @note This overload is available only if the setter is not invocable with `const property_wrapper<T, Get, Set, J> &`. */
		template<typename J, typename = std::enable_if_t<std::is_void_v<J> && std::is_const_v<I> && !std::is_const_v<J>>>
		constexpr property_wrapper(const property_wrapper<Get, Set, J> &other)
			: instance_base(other.value), getter_base(other), setter_base(other)
		{
		}
		/** Move-assigns the property wrapper from another who's instance type is not const-qualified.
		 * @note This overload is available only if the setter is not invocable with `property_wrapper<T, Get, Set, J> &`. */
		template<typename J, typename = std::enable_if_t<std::is_void_v<J> && std::is_const_v<I> && !std::is_const_v<J>>>
		constexpr property_wrapper(property_wrapper<Get, Set, J> &&other) noexcept
			: instance_base(other.value), getter_base(std::move(other)), setter_base(std::move(other))
		{
		}

		/** Copy-assigns the property wrapper from another who's instance type is not const-qualified.
		 * @note This overload is available only if the setter is not invocable with `const property_wrapper<T, Get, Set, J> &`. */
		template<typename J, typename = std::enable_if_t<std::is_void_v<J> && std::is_const_v<I> && !std::is_const_v<J>>>
		constexpr property_wrapper &operator=(const property_wrapper<Get, Set, J> &other) requires (!is_settable<const property_wrapper<Get, Set, J> &>)
		{
			instance_base::value = other.value;
			getter_base::operator=(other);
			setter_base::operator=(other);
			return *this;
		}
		/** Move-assigns the property wrapper from another who's instance type is not const-qualified.
		 * @note This overload is available only if the setter is not invocable with `property_wrapper<T, Get, Set, J> &`. */
		template<typename J, typename = std::enable_if_t<std::is_void_v<J> && std::is_const_v<I> && !std::is_const_v<J>>>
		constexpr property_wrapper &operator=(property_wrapper<Get, Set, J> &&other) noexcept requires (!is_settable<const property_wrapper<Get, Set, J> &&>)
		{
			instance_base::value = other.value;
			getter_base::operator=(std::move(other));
			setter_base::operator=(std::move(other));
			return *this;
		}

		/** @brief Invokes the getter of this property.
		 * @param args Arguments passed to the getter.
		 * @return Any value returned by the getter. */
		template<typename... Args>
		constexpr decltype(auto) get(Args &&...args) const noexcept(is_nothrow_gettable<Args...>) requires is_gettable<Args...>
		{
			if constexpr (std::is_void_v<I>)
				return getter_base::operator()(instance(), std::forward<Args>(args)...);
			else
				return getter_base::operator()(std::forward<Args>(args)...);
		}
		/** Invokes the getter of this property with no arguments. Equivalent to `get()`. */
		template<typename U>
		[[nodiscard]] constexpr decltype(auto) operator*() const noexcept(is_nothrow_settable<>) requires is_settable<> { return get(); }
		/** Invokes the getter of this property with no arguments and returns address of the result.
		 * Equivalent to `std::addressof(get())`. */
		template<typename U>
		[[nodiscard]] constexpr auto operator->() const noexcept(is_nothrow_settable<>) requires is_settable<> { return std::addressof(get()); }

		/** @brief Invokes the setter of this property.
		 * @param args Arguments passed to the setter.
		 * @return Any value returned by the setter. */
		template<typename... Args>
		constexpr decltype(auto) set(Args &&...args) const noexcept(is_nothrow_settable<Args...>) requires is_settable<Args...>
		{
			if constexpr (std::is_void_v<I>)
				return setter_base::operator()(instance(), std::forward<Args>(args)...);
			else
				return setter_base::operator()(std::forward<Args>(args)...);
		}
		/** @copybrief set
		 * @param value Value assigned to the wrapped object.
		 * @note This overload is available only if the setter is not invocable with `U`. */
		template<typename U>
		constexpr property_wrapper &operator=(U &&value) const noexcept(is_nothrow_settable<U>) requires is_settable<U>
		{
			set(std::forward<U>(value));
			return *this;
		}

		/** Returns pointer to the bound instance of the proxy wrapper. */
		[[nodiscard]] constexpr I *instance() const noexcept requires (!std::is_void_v<I>)
		{
			return instance_base::value;
		}
		/** Rebinds the instance pointer of the property wrapper.
		 * @return Reference to this property wrapper. */
		constexpr property_wrapper &rebind(I *ptr) noexcept requires (!std::is_void_v<I>)
		{
			instance_base::rebind(ptr);
			return *this;
		}

		constexpr void swap(property_wrapper &other) noexcept requires (std::is_swappable_v<getter_base> && std::is_swappable_v<setter_base>)
		{
			instance_base::swap(other);
			getter_base::swap(other);
			setter_base::swap(other);
		}
		friend constexpr void swap(property_wrapper &a, property_wrapper &b) noexcept requires std::is_swappable_v<property_wrapper> { a.swap(b); }

	private:
		template<typename... Args>
		constexpr decltype(auto) invoke_getter(Args &&...args)
		{
			if constexpr (std::is_void_v<I>)
				return getter_base::operator()(instance(), std::forward<Args>(args)...);
			else
				return getter_base::operator()(std::forward<Args>(args)...);
		}
		template<typename... Args>
		constexpr decltype(auto) invoke_getter(Args &&...args) const
		{
			if constexpr (std::is_void_v<I>)
				return getter_base::operator()(instance(), std::forward<Args>(args)...);
			else
				return getter_base::operator()(std::forward<Args>(args)...);
		}
	};

	template<typename Get>
	property_wrapper(const Get &) -> property_wrapper<Get, void>;
	template<typename Get, typename Set>
	property_wrapper(const Get &, const Set &) -> property_wrapper<Get, Set>;
	template<typename I, typename Get>
	property_wrapper(I *, const Get &) -> property_wrapper<Get, void, I>;
	template<typename I, typename Get, typename Set>
	property_wrapper(I *, const Get &, const Set &) -> property_wrapper<Get, Set, I>;
	// clang-format on

	/** @brief Alias used to create a getter-only property wrapper. */
	template<typename Get, typename I = void>
	using get_wrapper = property_wrapper<Get, void, I>;
	/** @brief Alias used to create a setter-only property wrapper. */
	template<typename Set, typename I = void>
	using set_wrapper = property_wrapper<void, Set, I>;
}	 // namespace sek