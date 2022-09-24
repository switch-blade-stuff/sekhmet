/*
 * Created by switchblade on 09/05/22
 */

#pragma once

#include <memory>
#include <stdexcept>
#include <utility>

#include "define.h"
#include "detail/ebo_base_helper.hpp"

namespace sek
{
	/** @brief Exception thrown when a delegate cannot be invoked. */
	class SEK_API delegate_error : public std::runtime_error
	{
	public:
		delegate_error() : runtime_error("Failed to invoke a delegate") {}
		explicit delegate_error(const std::string &msg) : runtime_error(msg) {}
		explicit delegate_error(const char *msg) : runtime_error(msg) {}
		~delegate_error() override;
	};

	namespace detail
	{
		// clang-format off
		template<typename...>
		struct is_delegate_compatible : std::false_type {};
		template<typename RDel, typename... ArgsDel, typename RFunc, typename... ArgsFunc>
		requires(sizeof...(ArgsDel) == sizeof...(ArgsFunc))
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
		template<typename RF, typename... ArgsF, RF (&F)(ArgsF...), typename RD, typename... ArgsD>
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

	/** @brief Helper type used to specify a compile-time function. */
	template<auto F>
	struct delegate_func_t;
	template<typename R, typename... Args, R (*F)(Args...)>
	struct delegate_func_t<F>
	{
	};
	template<typename R, typename... Args, R (&F)(Args...)>
	struct delegate_func_t<F>
	{
	};
	template<typename R, typename I, typename... Args, R (I::*F)(Args...)>
	struct delegate_func_t<F>
	{
	};
	template<typename R, typename I, typename... Args, R (I::*F)(Args...) const>
	struct delegate_func_t<F>
	{
	};
	template<typename R, typename I, typename... Args, R (I::*F)(Args...) volatile>
	struct delegate_func_t<F>
	{
	};
	template<typename R, typename I, typename... Args, R (I::*F)(Args...) const volatile>
	struct delegate_func_t<F>
	{
	};

	/** Instance of the `delegate_func_t` helper type. */
	template<auto F>
	constexpr auto delegate_func = delegate_func_t<F>{};

	template<typename>
	class delegate;

	/** @brief Type-erased function wrapper meant for use with the event system.
	 *
	 * Delegates provide a similar set of functionality to `std::function`, with certain extra features to make it
	 * suitable for the event system. In particular, aside from storing a heap-allocated state, delegates can store a
	 * single 16-bit aligned pointer as a bound member. This can be used for efficient storage of instance pointers
	 * for use with member functions, as no memory allocation will be necessary. In addition, delegates allow the user
	 * to retrieve a generic `void *` pointer to the bound state at runtime, which can be used to check for equality
	 * of delegate references.
	 *
	 * @tparam R Type returned by the delegate.
	 * @tparam Args Types of arguments used to invoke the delegate. */
	template<typename R, typename... Args>
	class delegate<R(Args...)>
	{
		class control_block
		{
		public:
			constexpr void *copy() const { return m_copy(this); }
			constexpr void destroy() { m_delete(this); }

		protected:
			void *(*m_copy)(const void *);
			void (*m_delete)(void *);
		};
		template<typename T>
		class heap_data : control_block, ebo_base_helper<T>
		{
			using base_t = ebo_base_helper<T>;

		public:
			heap_data() = delete;

			constexpr heap_data(const heap_data &other) : base_t(other) {}
			template<typename... TArgs>
			constexpr explicit heap_data(std::in_place_t, TArgs &&...args) : base_t(std::forward<TArgs>(args)...)
			{
				this->m_copy = +[](const void *p) -> void * { return make_data<T>(*static_cast<const heap_data *>(p)); };
				this->m_delete = +[](void *p) { delete static_cast<heap_data *>(p); };
			}

			using base_t::get;
		};

		template<typename T, typename... TArgs>
		constexpr static heap_data<T> *make_data(TArgs &&...args)
		{
			return new heap_data<T>{std::in_place, std::forward<TArgs>(args)...};
		}
		template<typename T>
		constexpr static heap_data<T> *make_data(const heap_data<T> &other)
		{
			return new heap_data<T>{other};
		}

		// clang-format off
		/* State object is stored by-value only if the state is an aligned pointer or the state is empty (since then
		 * the pointer does not matter). */
		template<typename T>
		constexpr static bool is_aligned =
			std::disjunction_v<std::conjunction<std::is_pointer<T>, std::negation<std::is_function<std::remove_pointer_t<T>>>,
												std::disjunction<std::bool_constant<(alignof(std::remove_pointer_t<T>) > alignof(std::byte))>,
																 std::is_empty<std::remove_pointer_t<T>>>>,
							   std::is_empty<T>>;
		// clang-format on

		class data_t
		{
			constexpr static std::uintptr_t external_flag = 2;
			constexpr static std::uintptr_t managed_flag = 1;

			[[nodiscard]] constexpr static bool is_external(std::uintptr_t ptr) noexcept { return ptr & external_flag; }
			[[nodiscard]] constexpr static bool is_managed(std::uintptr_t ptr) noexcept { return ptr & managed_flag; }

			[[nodiscard]] constexpr static control_block *heap_cb(std::uintptr_t ptr) noexcept
			{
				return std::bit_cast<control_block *>(ptr);
			}
			[[nodiscard]] constexpr static void *heap_ptr(std::uintptr_t ptr) noexcept
			{
				return std::bit_cast<void *>(heap_cb(ptr) + 1);
			}

		public:
			[[nodiscard]] constexpr static void *get(std::uintptr_t ptr) noexcept
			{
				auto result = std::bit_cast<void *>(ptr);
				if (is_managed(ptr))
				{
					/* If state is managed, get pointer to the heap data. */
					result = heap_ptr(ptr);
					/* If heap data is a pointer to some external instance, dereference heap data. */
					if (is_external(ptr)) result = *static_cast<void **>(result);
				}
				return result;
			}

		public:
			constexpr data_t() noexcept = default;

			constexpr data_t(const data_t &other) { copy(other); }
			constexpr data_t &operator=(const data_t &other)
			{
				if (this != &other)
				{
					destroy();
					copy(other);
				}
				return *this;
			}

			constexpr data_t(data_t &&other) noexcept { swap(other); }
			constexpr data_t &operator=(data_t &&other) noexcept
			{
				swap(other);
				return *this;
			}

			constexpr ~data_t() { destroy(); }

			// clang-format off
			template<typename T, typename... TArgs>
			constexpr explicit data_t(std::in_place_type_t<T>, TArgs &&...args) requires std::is_function_v<std::remove_pointer_t<T>>
			{
				init_managed<T>(std::forward<TArgs>(args)...);
			}
			template<typename T, typename... TArgs>
			constexpr explicit data_t(std::in_place_type_t<T>, TArgs &&...args)
			{
				if (!is_aligned<T>)
					init_managed<T>(std::forward<TArgs>(args)...);
				else
					init_local<T>(std::forward<TArgs>(args)...);
			}
			// clang-format on

			[[nodiscard]] constexpr void *get() const noexcept { return get(m_ptr); }
			[[nodiscard]] constexpr std::uintptr_t value() const noexcept { return m_ptr; }

			constexpr void reset()
			{
				destroy();
				m_ptr = 0;
			}

			constexpr void swap(data_t &other) noexcept { std::swap(m_ptr, other.m_ptr); }

		private:
			template<typename T, typename... TArgs>
			constexpr void init_managed(TArgs &&...args)
			{
				m_ptr = std::bit_cast<std::uintptr_t>(make_data<T>(std::forward<TArgs>(args)...));
				m_ptr |= managed_flag;

				/* If state is an external pointer that did not fit into local alignment, set the external bit. */
				if constexpr (std::is_pointer_v<T>) m_ptr |= external_flag;
			}
			template<typename T, typename... TArgs>
			constexpr void init_local(TArgs &&...args)
			{
				std::construct_at(std::bit_cast<T *>(&m_ptr), std::forward<TArgs>(args)...);
			}

			constexpr void copy(const data_t &other)
			{
				/* Duplicate the heap-allocated data block if it is not local. */
				if (is_managed(other.m_ptr))
					m_ptr = (std::bit_cast<std::uintptr_t>(heap_cb(other.m_ptr)->copy())) |
							(managed_flag | (other.m_ptr & external_flag));
				else
					m_ptr = other.m_ptr;
			}
			constexpr void destroy()
			{
				if (is_managed(m_ptr)) heap_cb(m_ptr)->destroy();
			}

			std::uintptr_t m_ptr = 0;
		};

		template<typename T, typename... Inject>
		constexpr static bool valid_ftor = std::is_object_v<T> && std::is_invocable_r_v<R, T, Inject..., Args...>;
		template<typename T, typename... Inject>
		constexpr static bool empty_ftor = valid_ftor<std::remove_reference_t<T>, Inject...> && std::is_empty_v<T>;
		template<typename RF, typename... ArgsF>
		constexpr static bool valid_sign = detail::is_delegate_compatible_v<R(Args...), RF(ArgsF...)>;
		template<auto F, typename... Inject>
		constexpr static bool valid_func = detail::is_delegate_func_v<F, R(Inject..., Args...)>;
		template<auto F, typename... Inject>
		constexpr static bool free_func = std::is_function_v<std::remove_pointer_t<decltype(F)>> && valid_func<F, Inject...>;
		template<auto F>
		constexpr static bool mem_func = std::is_member_function_pointer_v<decltype(F)> && valid_func<F>;

	public:
		/** Initializes an empty (non-bound) delegate. */
		constexpr delegate() noexcept = default;

		constexpr delegate(const delegate &) = default;
		constexpr delegate &operator=(const delegate &) = default;
		constexpr delegate(delegate &&) noexcept = default;
		constexpr delegate &operator=(delegate &&) noexcept = default;

		// clang-format off
		/** @brief Initializes the delegate from a free function pointer.
		 * @param f Pointer to the function bound by the delegate.
		 * @note This overload allocates memory, as alignment of function pointers is unknown. */
		template<typename FR, typename... FArgs>
		constexpr delegate(FR (*f)(FArgs...)) noexcept requires valid_sign<FR, FArgs...>
		{
			assign(f);
		}

		/** @brief Binds a free function pointer to the delegate.
		 * @copydetails delegate */
		template<typename FR, typename... FArgs>
		constexpr delegate &assign(FR (*f)(FArgs...)) noexcept requires valid_sign<FR, FArgs...>
		{
			using F = FR (*)(FArgs...);

			m_proxy = +[](std::uintptr_t data, Args...args) -> R
			{
				const auto func = std::bit_cast<F>(data_t::get(data));
				return (func)(std::forward<Args>(args)...);
			};
			m_data = data_t{std::in_place_type<F>, f};
			return *this;
		}
		/** @copydoc assign */
		template<typename FR, typename... FArgs>
		constexpr delegate &operator=(FR (*f)(FArgs...)) noexcept requires valid_sign<FR, FArgs...>
		{
			return assign(f);
		}

		/** Initializes the delegate from a a free function pointer and an instance argument.
		 * @param f Pointer to the function bound by the delegate.
		 * @param instance Instance object passed as the first parameter to the function.
		 * @note This overload may allocate memory if the following expression evaluates to `false`:
		 * `std::is_empty_v<I> || alignof(I) > alignof(std::byte)`. */
		template<typename I>
		constexpr delegate(R (*f)(I *, Args...), I *instance) noexcept requires is_aligned<I *>
		{
			assign(f, instance);
		}
		/** @copydoc delegate */
		template<typename I>
		constexpr delegate(R (*f)(I *, Args...), I &instance) noexcept requires is_aligned<I *>
		{
			assign(f, instance);
		}
		/** @copydoc delegate */
		template<typename I>
		constexpr delegate(R (*f)(I &, Args...), I &instance) noexcept requires is_aligned<I *>
		{
			assign(f, instance);
		}

		/** @brief Binds a free function pointer and an instance argument to the delegate.
		 * @copydetails delegate */
		template<typename I>
		constexpr delegate &assign(R (*f)(I *, Args...), I *instance) noexcept requires is_aligned<I *>
		{
			/* Since the first argument is a pointer, bit_cast to `uintptr_t` is safe. */
			m_proxy = std::bit_cast<R (*)(std::uintptr_t, Args...)>(f);
			m_data = data_t{std::in_place_type<I *>, instance};
			return *this;
		}
		/** @copydoc assign */
		template<typename I>
		constexpr delegate &assign(R (*f)(I *, Args...), I &instance) noexcept requires is_aligned<I *>
		{
			return assign(f, std::addressof(instance));
		}
		/** @copydoc assign */
		template<typename I>
		constexpr delegate &assign(R (*f)(I &, Args...), I &instance) noexcept requires is_aligned<I *>
		{
			return assign(std::bit_cast<R (*)(I *, Args...)>(f), std::addressof(instance));
		}

		/** @brief Initializes the delegate with an in-place constructed functor.
		 * @note This overload may allocate memory if the functor type is non-empty. */
		template<typename F, typename... FArgs>
		constexpr explicit delegate(std::in_place_type_t<F>, FArgs &&...args) noexcept requires valid_ftor<std::remove_reference_t<F>>
		{
			assign(std::in_place_type<F>, std::forward<FArgs>(args)...);
		}

		/** @brief Binds an in-place constructed functor to the delegate.
		 * @copydetails delegate */
		template<typename F, typename... FArgs>
		constexpr delegate &assign(std::in_place_type_t<F>, FArgs &&...args) noexcept requires valid_ftor<std::remove_reference_t<F>>
		{
			m_proxy = +[](std::uintptr_t  data, Args...args) -> R
			{
			    const auto func = static_cast<F *>(data_t::get(data));
				return (func)(std::forward<Args>(args)...);
			};
			m_data = data_t{std::in_place_type<F>, std::forward<FArgs>(args)...};
			return *this;
		}

		/** @brief Initializes the delegate with a functor.
		 * @note This overload may allocate memory if the functor type is non-empty. */
		template<typename F>
		constexpr delegate(F &&f) noexcept requires valid_ftor<std::remove_reference_t<F>>
		{
			assign(std::forward<F>(f));
		}

		/** Binds a functor to the delegate.
		 * @copydetails delegate */
		template<typename F>
		constexpr delegate &assign(F &&f) noexcept requires valid_ftor<std::remove_reference_t<F>>
		{
			return operator=(std::forward<F>(f));
		}
		/** @copydoc assign */
		template<typename F>
		constexpr delegate &operator=(F &&f) noexcept requires valid_ftor<std::remove_reference_t<F>>
		{
			return assign(std::in_place_type<F>, std::forward<F>(f));
		}

		/** @brief Initializes the delegate from an empty functor and an instance argument.
		 * @param instance Instance object passed as the first parameter to the functor.
		 * @note This overload may allocate memory if the following expression evaluates to `false`:
		 * `std::is_empty_v<I> || alignof(I) > alignof(std::byte)`. */
		template<typename F, typename I>
		constexpr delegate(F &&f, I *instance) noexcept requires empty_ftor<std::remove_reference_t<F>, I *>
		{
			assign(std::forward<F>(f), instance);
		}
		/** @copydoc delegate */
		template<typename F, typename I>
		constexpr delegate(F &&f, I &instance) noexcept requires empty_ftor<std::remove_reference_t<F>, I *>
		{
			assign(std::forward<F>(f), instance);
		}
		/** @copydoc delegate */
		template<typename F, typename I>
		constexpr delegate(F &&f, I &instance) noexcept requires empty_ftor<std::remove_reference_t<F>, I &>
		{
			assign(std::forward<F>(f), instance);
		}

		/** @brief Binds an empty functor and an instance argument to the delegate.
		 * @copydetails delegate */
		template<typename F, typename I>
		constexpr delegate &assign(F &&, I *instance) noexcept requires empty_ftor<std::remove_reference_t<F>, I *>
		{
			m_proxy = +[](std::uintptr_t data, Args...args) -> R
			{
				const auto ptr = static_cast<I *>(data_t::get(data));
				return F{}(ptr, std::forward<Args>(args)...);
			};
			m_data = data_t{std::in_place_type<I *>, instance};
			return *this;
		}
		/** @copydoc assign */
		template<typename F, typename I>
		constexpr delegate &assign(F &&, I &instance) noexcept requires empty_ftor<std::remove_reference_t<F>, I *>
		{
			m_proxy = +[](std::uintptr_t data, Args...args) -> R
			{
				const auto ptr = static_cast<I *>(data_t::get(data));
				return F{}(ptr, std::forward<Args>(args)...);
			};
			m_data = data_t{std::in_place_type<I *>, std::addressof(instance)};
			return *this;
		}
		/** @copydoc assign */
		template<typename F, typename I>
		constexpr delegate &assign(F &&, I &instance) noexcept requires empty_ftor<std::remove_reference_t<F>, I &>
		{
			m_proxy = +[](std::uintptr_t data, Args...args) -> R
			{
				const auto ptr = static_cast<I *>(data_t::get(data));
				return F{}(*ptr, std::forward<Args>(args)...);
			};
			m_data = data_t{std::in_place_type<I *>, std::addressof(instance)};
			return *this;
		}

		/** Initializes the delegate from a free function. */
		template<auto F>
		constexpr delegate(delegate_func_t<F>) noexcept requires free_func<F>
		{
			assign<F>();
		}

		/** Binds a free function to the delegate. */
		template<auto F>
		constexpr delegate &assign() noexcept requires free_func<F>
		{
			return operator=(delegate_func<F>);
		}
		/** @copydoc assign */
		template<auto F>
		constexpr delegate &assign(delegate_func_t<F>) noexcept requires free_func<F>
		{
			return operator=(delegate_func<F>);
		}
		/** @copydoc assign */
		template<auto F>
		constexpr delegate &operator=(delegate_func_t<F>) noexcept requires free_func<F>
		{
			m_proxy = +[](std::uintptr_t, Args...args) -> R { return F(std::forward<Args>(args)...); };
			m_data = {};
			return *this;
		}

		/** @brief Initializes the delegate from a free function and a bound instance argument.
		 * @param instance Instance object passed as the first parameter to the function.
		 * @note This overload may allocate memory if the following expression evaluates to `false`:
		 * `std::is_empty_v<I> || alignof(I) > alignof(std::byte)`. */
		template<auto F, typename I>
		constexpr delegate(delegate_func_t<F>, I *arg) noexcept requires free_func<F, I *>
		{
			assign<F>(arg);
		}
		/** @copydoc delegate */
		template<auto F, typename I>
		constexpr delegate(delegate_func_t<F>, I &arg) noexcept requires free_func<F, I *>
		{
			assign<F>(arg);
		}
		/** @copydoc delegate */
		template<auto F, typename I>
		constexpr delegate(delegate_func_t<F>, I &arg) noexcept requires free_func<F, I &>
		{
			assign<F>(arg);
		}

		/** @brief Binds a free function and an instance argument to the delegate.
		 * @copydetails delegate */
		template<auto F, typename I>
		constexpr delegate &assign(I *instance) noexcept requires free_func<F, I *>
		{
			m_proxy = +[](std::uintptr_t data, Args...args) -> R
			{
				const auto ptr = static_cast<I *>(data_t::get(data));
				return F(ptr, std::forward<Args>(args)...);
			};
			m_data = data_t{std::in_place_type<I *>, instance};
			return *this;
		}
		/** @copydoc assign */
		template<auto F, typename I>
		constexpr delegate &assign(I &instance) noexcept requires free_func<F, I *>
		{
			m_proxy = +[](std::uintptr_t data, Args...args) -> R
			{
				const auto ptr = static_cast<I *>(data_t::get(data));
				return F(ptr, std::forward<Args>(args)...);
			};
			m_data = data_t{std::in_place_type<I *>, std::addressof(instance)};
			return *this;
		}
		/** @copydoc assign */
		template<auto F, typename I>
		constexpr delegate &assign(I &instance) noexcept requires free_func<F, I &>
		{
			m_proxy = +[](std::uintptr_t data, Args...args) -> R
			{
				const auto ptr = static_cast<I *>(data_t::get(data));
				return F(*ptr, std::forward<Args>(args)...);
			};
			m_data = data_t{std::in_place_type<I *>, std::addressof(instance)};
			return *this;
		}
		/** @copydoc assign */
		template<auto F, typename I>
		constexpr delegate &assign(delegate_func_t<F>, I *instance) noexcept requires free_func<F, I *>
		{
			return assign<F>(instance);
		}
		/** @copydoc assign */
		template<auto F, typename I>
		constexpr delegate &assign(delegate_func_t<F>, I &instance) noexcept requires free_func<F, I *>
		{
			return assign<F>(instance);
		}
		/** @copydoc assign */
		template<auto F, typename I>
		constexpr delegate &assign(delegate_func_t<F>, I &instance) noexcept requires free_func<F, I &>
		{
			return assign<F>(instance);
		}

		/** @brief Initializes the delegate from a member function and it's object instance.
		 * @param instance Bound instance the member function is invoked for.
		 * @note This overload may allocate memory if the following expression evaluates to `false`:
		 * `std::is_empty_v<I> || alignof(I) > alignof(std::byte)`. */
		template<auto F, typename I>
		constexpr delegate(delegate_func_t<F>, I *instance) noexcept requires mem_func<F>
		{
			assign<F>(instance);
		}
		/** @copydoc delegate */
		template<auto F, typename I>
		constexpr delegate(delegate_func_t<F>, I &instance) noexcept requires mem_func<F>
		{
			assign<F>(instance);
		}

		/** @brief Binds a member function and it's object instance to the delegate.
		 * @copydetails delegate */
		template<auto F, typename I>
		constexpr delegate &assign(I *instance) noexcept requires mem_func<F>
		{
			m_proxy = +[](std::uintptr_t data, Args...args) -> R
			{
			    const auto ptr = static_cast<I *>(data_t::get(data));
			    return (ptr->*F)(std::forward<Args>(args)...);
			};
			m_data = data_t{std::in_place_type<I *>, instance};
			return *this;
		}
		/** @copydoc assign */
		template<auto F, typename I>
		constexpr delegate &assign(delegate_func_t<F>, I *instance) noexcept requires mem_func<F>
		{
			return assign<F>(instance);
		}
		/** @copydoc assign */
		template<auto F, typename I>
		constexpr delegate &assign(I &instance) noexcept requires mem_func<F>
		{
			return assign(std::addressof(instance));
		}
		/** @copydoc assign */
		template<auto F, typename I>
		constexpr delegate &assign(delegate_func_t<F>, I &instance) noexcept requires mem_func<F>
		{
			return assign<F>(instance);
		}
		// clang-format on

		/** Checks if the delegate is a valid invocation target (is bound to a function or functor). */
		[[nodiscard]] constexpr bool valid() const noexcept { return m_proxy != nullptr; }
		/** Returns pointer to the data of the bound argument or object instance. */
		[[nodiscard]] constexpr const void *data() const noexcept { return m_data.get(); }

		/** Invokes the bound function.
		 * @param args Arguments passed to the function.
		 * @return Value returned by the function.
		 * @throw delegate_error If the delegate is not bound to a function. */
		constexpr R invoke(Args... args) const
		{
			if (valid()) [[likely]]
				return m_proxy(m_data.value(), std::forward<Args>(args)...);
			else
				throw delegate_error("Attempted to invoke an unbound delegate");
		}
		/** @copydoc invoke */
		constexpr R operator()(Args... args) const { return invoke(std::forward<Args>(args)...); }

		[[nodiscard]] constexpr bool operator==(const delegate &other) const noexcept
		{
			return m_proxy == other.m_proxy && data() == other.data();
		}

		constexpr void swap(delegate &other) noexcept
		{
			std::swap(m_proxy, other.m_proxy);
			m_data.swap(other.m_data);
		}
		friend constexpr void swap(delegate &a, delegate &b) noexcept { a.swap(b); }

	private:
		R (*m_proxy)(std::uintptr_t, Args...) = nullptr;
		data_t m_data = {};
	};

	template<typename R, typename... Args>
	delegate(R (*)(Args...)) -> delegate<R(Args...)>;

	template<typename R, typename I, typename... Args>
	delegate(R (*)(I *, Args...), I *) -> delegate<R(Args...)>;
	template<typename R, typename I, typename... Args>
	delegate(R (*)(I *, Args...), I &) -> delegate<R(Args...)>;
	template<typename R, typename I, typename... Args>
	delegate(R (*)(I &, Args...), I &) -> delegate<R(Args...)>;

	template<typename R, typename... Args, R (*F)(Args...)>
	delegate(delegate_func_t<F>) -> delegate<R(Args...)>;
	template<typename R, typename... Args, R (&F)(Args...)>
	delegate(delegate_func_t<F>) -> delegate<R(Args...)>;

	template<typename R, typename I, typename... Args, R (*F)(I *, Args...)>
	delegate(delegate_func_t<F>, I *) -> delegate<R(Args...)>;
	template<typename R, typename I, typename... Args, R (*F)(I *, Args...)>
	delegate(delegate_func_t<F>, I &) -> delegate<R(Args...)>;
	template<typename R, typename I, typename... Args, R (*F)(I &, Args...)>
	delegate(delegate_func_t<F>, I &) -> delegate<R(Args...)>;
	template<typename R, typename I, typename... Args, R (&F)(I *, Args...)>
	delegate(delegate_func_t<F>, I *) -> delegate<R(Args...)>;
	template<typename R, typename I, typename... Args, R (&F)(I *, Args...)>
	delegate(delegate_func_t<F>, I &) -> delegate<R(Args...)>;
	template<typename R, typename I, typename... Args, R (&F)(I &, Args...)>
	delegate(delegate_func_t<F>, I &) -> delegate<R(Args...)>;

	template<typename R, typename I, typename... Args, R (I::*F)(Args...)>
	delegate(delegate_func_t<F>, I *) -> delegate<R(Args...)>;
	template<typename R, typename I, typename... Args, R (I::*F)(Args...) const>
	delegate(delegate_func_t<F>, I *) -> delegate<R(Args...)>;
	template<typename R, typename I, typename... Args, R (I::*F)(Args...) const>
	delegate(delegate_func_t<F>, const I *) -> delegate<R(Args...)>;
	template<typename R, typename I, typename... Args, R (I::*F)(Args...) volatile>
	delegate(delegate_func_t<F>, I *) -> delegate<R(Args...)>;
	template<typename R, typename I, typename... Args, R (I::*F)(Args...) volatile>
	delegate(delegate_func_t<F>, volatile I *) -> delegate<R(Args...)>;
	template<typename R, typename I, typename... Args, R (I::*F)(Args...) const volatile>
	delegate(delegate_func_t<F>, I *) -> delegate<R(Args...)>;
	template<typename R, typename I, typename... Args, R (I::*F)(Args...) const volatile>
	delegate(delegate_func_t<F>, const I *) -> delegate<R(Args...)>;
	template<typename R, typename I, typename... Args, R (I::*F)(Args...) const volatile>
	delegate(delegate_func_t<F>, volatile I *) -> delegate<R(Args...)>;
	template<typename R, typename I, typename... Args, R (I::*F)(Args...) const volatile>
	delegate(delegate_func_t<F>, const volatile I *) -> delegate<R(Args...)>;

	template<typename R, typename I, typename... Args, R (I::*F)(Args...)>
	delegate(delegate_func_t<F>, I &) -> delegate<R(Args...)>;
	template<typename R, typename I, typename... Args, R (I::*F)(Args...) const>
	delegate(delegate_func_t<F>, I &) -> delegate<R(Args...)>;
	template<typename R, typename I, typename... Args, R (I::*F)(Args...) const>
	delegate(delegate_func_t<F>, const I &) -> delegate<R(Args...)>;
	template<typename R, typename I, typename... Args, R (I::*F)(Args...) volatile>
	delegate(delegate_func_t<F>, I &) -> delegate<R(Args...)>;
	template<typename R, typename I, typename... Args, R (I::*F)(Args...) volatile>
	delegate(delegate_func_t<F>, volatile I &) -> delegate<R(Args...)>;
	template<typename R, typename I, typename... Args, R (I::*F)(Args...) const volatile>
	delegate(delegate_func_t<F>, I &) -> delegate<R(Args...)>;
	template<typename R, typename I, typename... Args, R (I::*F)(Args...) const volatile>
	delegate(delegate_func_t<F>, const I &) -> delegate<R(Args...)>;
	template<typename R, typename I, typename... Args, R (I::*F)(Args...) const volatile>
	delegate(delegate_func_t<F>, volatile I &) -> delegate<R(Args...)>;
	template<typename R, typename I, typename... Args, R (I::*F)(Args...) const volatile>
	delegate(delegate_func_t<F>, const volatile I &) -> delegate<R(Args...)>;
}	 // namespace sek