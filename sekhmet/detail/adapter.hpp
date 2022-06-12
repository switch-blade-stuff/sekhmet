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
 * Created by switchblade on 2021-11-08
 */

#pragma once

#include "delegate.hpp"
#include "meta_util.hpp"

namespace sek
{
	class adapter_error : public std::runtime_error
	{
	public:
		adapter_error() : std::runtime_error("Unknown adapter error") {}
		explicit adapter_error(std::string &&msg) : std::runtime_error(std::move(msg)) {}
		explicit adapter_error(const std::string &msg) : std::runtime_error(msg) {}
		explicit adapter_error(const char *msg) : std::runtime_error(msg) {}
		~adapter_error() override = default;
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

		struct adapter_instance
		{
			constexpr adapter_instance() noexcept = default;

			template<typename T>
			constexpr explicit adapter_instance(T &ptr) noexcept : data(&ptr)
			{
			}

			template<typename T>
			[[nodiscard]] constexpr T &get() const noexcept
			{
				if constexpr (std::is_const_v<T>)
					return *static_cast<T *>(data);
				else
					return *static_cast<T *>(const_cast<void *>(data));
			}

			[[nodiscard]] constexpr bool empty() const noexcept { return sentinel == 0; }

			union
			{
				std::uintptr_t sentinel = 0;
				const void *data;
			};
		};

		template<typename...>
		class adapter_vtable;

		template<>
		class adapter_vtable<>
		{
		};

		template<typename FuncT, typename... FuncTs>
		class adapter_vtable<FuncT, FuncTs...> : adapter_vtable<FuncTs...>
		{
		public:
			constexpr explicit adapter_vtable(FuncT f, FuncTs... fs) noexcept
				: adapter_vtable<FuncTs...>(fs...), func(f)
			{
			}

			template<typename F>
			constexpr auto get() const noexcept
				requires is_in_v<F, FuncTs...>
			{
				return adapter_vtable<FuncTs...>::template get<F>();
			}
			template<typename F>
			constexpr FuncT get() const noexcept
				requires std::same_as<F, FuncT>
			{
				return func;
			}

		private:
			/** Actual function pointer stored by the vtable type. */
			const FuncT func;
		};
	}	 // namespace detail

	template<>
	class adapter_proxy<>
	{
	};

	/** Parent type for all adapter proxy objects. */
	template<typename R, typename... Args>
	class adapter_proxy<R(Args...)> : adapter_proxy<>
	{
		template<detail::valid_adapter_proxy...>
		friend class adapter;

		using func_type = R (*)(detail::adapter_instance, Args &&...);

		template<typename T, typename Proxy>
		constexpr static func_type bind_proxy() noexcept
		{
			return +[](detail::adapter_instance i, Args &&...args) -> R
			{ return Proxy{}(i.get<T>(), std::forward<Args>(args)...); };
		}

	private:
		template<typename A>
		constexpr static delegate<R(Args...)> make_delegate(const A *adapter) noexcept
		{
			return sek::delegate{+[](const A *a, Args... args)
								 { return a->template invoke<adapter_proxy>(std::forward<Args>(args)...); },
								 adapter};
		}
	};

	/** @brief Structure used to implement type erasure.
	 *
	 * An adapter is a special type used to implement type erasure without
	 * using virtual inheritance. An adapter is defined with a set of
	 * proxy objects. These proxy objects specify the "virtual" functions
	 * callable through the adapter.
	 *
	 * A proxy must be an empty, default-constructible type with a public const
	 * call operator that takes a reference to the instance as the
	 * first argument and an arbitrary amount of other arguments. Every proxy
	 * must inherit from `adapter_proxy` template, specifying the return type
	 * and call signature (minus the instance type).
	 *
	 * Ex:
	 * @code{.cpp}
	 * 		struct size_proxy : adapter_proxy<std::size_t()>
	 * 		{
	 * 			template<typename T>
	 * 			std::size_t operator()(const T &ptr) const noexcept { return ptr.size(); }
	 * 		};
	 * @endcode
	 *
	 * `adapter` is lightweight, since it only stores a pointer to the bound
	 * object and a pointer to per-type vtable generated during the binding.
	 *
	 * It can be used to avoid virtual inheritance overhead for types that not
	 * necessarily need to be virtual, to add virtual inheritance
	 * functionality to types that do not support it or to create "fake" virtual
	 * inheritance between unrelated types. */
	template<detail::valid_adapter_proxy... ProxyTs>
	class adapter
	{
	private:
		template<typename Proxy>
		using proxy_func_type = typename detail::adapter_proxy_parent_t<Proxy>::func_type;
		using vtable_type = detail::adapter_vtable<proxy_func_type<ProxyTs>...>;

		template<typename T, typename Proxy>
		constexpr static proxy_func_type<Proxy> bind_proxy_func() noexcept
		{
			return detail::adapter_proxy_parent_t<Proxy>::template bind_proxy<T, Proxy>();
		}

		template<typename T>
		struct vtable_instance
		{
			constexpr static vtable_type value{bind_proxy_func<T, ProxyTs>()...};
		};

		template<typename T>
		constexpr static const vtable_type *generate_vtable() noexcept
		{
			return &vtable_instance<T>::value;
		}

	public:
		/** Initializes an empty adapter. */
		constexpr adapter() = default;
		/** Binds a type instance to the adapter. */
		template<typename T>
		constexpr explicit adapter(T &instance) : m_instance(instance), m_vtable(generate_vtable<T>())
		{
		}

		/** Resets the adapter to an empty state. */
		constexpr void reset() noexcept
		{
			m_instance = {};
			m_vtable = nullptr;
		}

		/** Re-binds the adapter for a different instance.
		 * @param new_instance Instance of the bound type.
		 * @return Reference to this adapter. */
		template<typename T>
		constexpr adapter &rebind(T &new_instance)
		{
			m_instance = detail::adapter_instance{new_instance};
			m_vtable = generate_vtable<T>();

			return *this;
		}

		/** Calls the specified proxy with the provided arguments.
		 * @tparam Proxy Proxy to call.
		 * @tparam Args Types of passed arguments.
		 * @param args Arguments passed to the proxy.
		 * @return Value returned by the proxy. */
		template<typename Proxy, typename... Args>
		constexpr decltype(auto) invoke(Args &&...args) const
		{
			if (empty()) [[unlikely]]
				throw adapter_error("Attempted to invoke a proxy on an empty adapter");
			else
				return m_vtable->template get<proxy_func_type<Proxy>>()(m_instance, std::forward<Args>(args)...);
		}
		/** Returns a delegate used to call a specific proxy on this adapter.
		 * @tparam Proxy Proxy to call.
		 * @return Delegate used to call the proxy. */
		template<typename Proxy>
		[[nodiscard]] constexpr decltype(auto) delegate() const noexcept
		{
			return Proxy::make_delegate(this);
		}

		/** Checks if an adapter is empty. */
		[[nodiscard]] constexpr bool empty() const noexcept { return m_instance.empty(); }
		/** @copydoc empty */
		[[nodiscard]] constexpr explicit operator bool() const noexcept { return !empty(); }

		constexpr void swap(adapter &other) noexcept
		{
			using std::swap;
			swap(m_instance, other.m_instance);
			swap(m_vtable, other.m_vtable);
		}

		friend constexpr void swap(adapter &a, adapter &b) noexcept { a.swap(b); }

	private:
		detail::adapter_instance m_instance = {};
		const vtable_type *m_vtable = nullptr;
	};
}	 // namespace sek