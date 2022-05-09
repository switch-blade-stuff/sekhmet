//
// Created by switchblade on 09/05/22.
//

#pragma once

#include <vector>

#include "delegate.hpp"

namespace sek
{
	template<typename, typename>
	class basic_event;

	template<typename>
	class event_view;

	class event_connection;

	namespace detail
	{
		template<typename Alloc, typename R, typename... Args>
		constexpr event_connection connect_event(basic_event<R(Args...), Alloc> *, const delegate<R(Args...)> *);
		template<typename Alloc, typename R, typename... Args>
		constexpr bool disconnect_event(basic_event<R(Args...), Alloc> *, event_connection &);
	}	 // namespace detail

	/** @brief RAII structure used to manage a listener delegate connected to an event.
	 * @warning Connection may not outlive it's parent event! */
	class event_connection
	{
		template<typename Alloc, typename R, typename... Args>
		friend constexpr event_connection detail::connect_event(basic_event<R(Args...), Alloc> *,
																const delegate<R(Args...)> *);
		template<typename Alloc, typename R, typename... Args>
		friend constexpr bool detail::disconnect_event(basic_event<R(Args...), Alloc> *, event_connection &);

		constexpr event_connection(delegate<void(void *)> disconnect_func, void *event)
			: disconnect_func(disconnect_func), event(event)
		{
		}

	public:
		event_connection() = delete;

		constexpr event_connection(const event_connection &) noexcept = default;
		constexpr event_connection &operator=(const event_connection &) noexcept = default;
		constexpr event_connection(event_connection &&) noexcept = default;
		constexpr event_connection &operator=(event_connection &&) noexcept = default;
		constexpr ~event_connection() { disconnect(); }

		/** Disconnects the managed listener from it's event. */
		constexpr void disconnect()
		{
			if (event) disconnect_func(event);
			event = nullptr;
		}

	private:
		delegate<void(void *)> disconnect_func;
		void *event;
	};

	namespace detail
	{
		template<typename Alloc, typename R, typename... Args>
		constexpr event_connection connect_event(basic_event<R(Args...), Alloc> *e, const delegate<R(Args...)> *l)
		{
			constexpr auto disconnect = +[](const delegate<R(Args...)> &listener, void *p)
			{
				auto *event = static_cast<basic_event<R(Args...), Alloc> *>(p);
				event->disconnect(listener);
			};
			return event_connection{delegate{disconnect, l}, e};
		}
		template<typename Alloc, typename R, typename... Args>
		constexpr bool disconnect_event(basic_event<R(Args...), Alloc> *e, event_connection &c)
		{
			if (c.event == e)
			{
				c.disconnect();
				return true;
			}
			else
				return false;
		}
	}	 // namespace detail

	/** @brief Structure used to manage a set of delegates.
	 *
	 * @tparam R Return type of the event's delegates
	 * @tparam Args Arguments passed to event's delegates.
	 * @tparam Alloc Allocator type used for the internal state. */
	template<typename Alloc, typename R, typename... Args>
	class basic_event<R(Args...), Alloc>
	{
		friend constexpr event_connection detail::connect_event(basic_event<R(Args...), Alloc> *,
																const delegate<R(Args...)> *);
		friend class event_view<basic_event>;

		using delegate_t = delegate<R(Args...)>;
		using alloc_t = typename std::allocator_traits<Alloc>::template rebind_alloc<delegate_t>;
		using data_t = std::vector<delegate_t, alloc_t>;

		// clang-format off
		template<typename F>
		constexpr static bool valid_collector = !std::is_void_v<R> && requires(F &&f, const delegate_t &d, Args &&...args) { f(d(std::forward<Args>(args)...)); };
		// clang-format on

	public:
		typedef typename data_t::const_iterator iterator;
		typedef typename data_t::const_iterator const_iterator;
		typedef typename data_t::const_reverse_iterator reverse_iterator;
		typedef typename data_t::const_reverse_iterator const_reverse_iterator;
		typedef typename data_t::allocator_type allocator_type;
		typedef typename data_t::size_type size_type;
		typedef typename data_t::difference_type difference_type;

	public:
		/** Initializes an empty event. */
		constexpr basic_event() noexcept(noexcept(data_t{})) = default;

		/** Initializes an empty event.
		 * @param alloc Allocator used to initialize internal state. */
		constexpr explicit basic_event(const allocator_type &alloc) : data(alloc) {}
		/** Initializes event with a set of delegates.
		 * @param init_list Initializer list containing delegates of the event.
		 * @param alloc Allocator used to initialize internal state. */
		constexpr basic_event(std::initializer_list<delegate<R(Args...)>> init_list, const allocator_type &alloc = alloc_t{})
			: data(init_list, alloc)
		{
		}

		/** Checks if the event is empty (has no listeners). */
		[[nodiscard]] constexpr bool empty() const noexcept { return data.empty(); }
		/** Returns amount of listeners bound to this event. */
		[[nodiscard]] constexpr size_type size() const noexcept { return data.size(); }

		/** Returns iterator to the fist listener of the event. */
		[[nodiscard]] constexpr const_iterator begin() const noexcept { return data.begin(); }
		/** @copydoc begin */
		[[nodiscard]] constexpr const_iterator cbegin() const noexcept { return data.cbegin(); }
		/** Returns iterator one past the last listener of the event. */
		[[nodiscard]] constexpr const_iterator end() const noexcept { return data.end(); }
		/** @copydoc end */
		[[nodiscard]] constexpr const_iterator cend() const noexcept { return data.cend(); }
		/** Returns reverse iterator one past the last listener of the event. */
		[[nodiscard]] constexpr const_reverse_iterator rbegin() const noexcept { return data.rbegin(); }
		/** @copydoc rbegin */
		[[nodiscard]] constexpr const_reverse_iterator crbegin() const noexcept { return data.crbegin(); }
		/** Returns reverse iterator to the first listener of the event. */
		[[nodiscard]] constexpr const_reverse_iterator rend() const noexcept { return data.rend(); }
		/** @copydoc rend */
		[[nodiscard]] constexpr const_reverse_iterator crend() const noexcept { return data.crend(); }

		/** Adds a listener delegate to the event at the specified position.
		 * @param where Position within the event's set of listeners at which to add the new listener.
		 * @param listener Delegate used to listen on the event.
		 * @return Reference to this event. */
		constexpr basic_event &listen(const_iterator where, delegate<delegate<R(Args...)>> listener)
		{
			data.insert(where, listener);
			return *this;
		}
		/** Adds a listener delegate to the event.
		 * @param listener Delegate used to listen on the event.
		 * @return Reference to this event.
		 * @note Listener is added to the end of the event. */
		constexpr basic_event &listen(delegate<delegate<R(Args...)>> listener)
		{
			data.push_back(listener);
			return *this;
		}
		/** @copydoc listen */
		constexpr basic_event &operator+=(delegate<delegate<R(Args...)>> listener) { return listen(listener); }

		/** Adds a listener delegate to the event at the specified position and returns it's connection.
		 * @param where Position within the event's set of listeners at which to add the new listener.
		 * @param listener Delegate used to listen on the event.
		 * @return RAII connection used to manage the listener. */
		[[nodiscard]] constexpr event_connection connect(const_iterator where, delegate<delegate<R(Args...)>> listener)
		{
			listen(where, listener);
			return detail::connect_event(this, listener);
		}
		/** Adds a listener delegate to the event and returns it's connection.
		 * @param listener Delegate used to listen on the event.
		 * @return RAII connection used to manage the listener.
		 * @note Listener is added to the end of the event. */
		[[nodiscard]] constexpr event_connection connect(delegate<delegate<R(Args...)>> listener)
		{
			listen(listener);
			return detail::connect_event(this, listener);
		}

		/** Removes a listener delegate pointed to by the specified iterator from the event.
		 * @param where Iterator pointing to the listener to be removed from the event.
		 * @return True if the listener was disconnected, false otherwise. */
		constexpr bool disconnect(const_iterator where)
		{
			if (where != data.end()) [[likely]]
			{
				data.erase(where);
				return true;
			}
			else
				return false;
		}
		/** Removes a listener delegate from the event.
		 * @param listener Delegate to remove from the event.
		 * @return True if the listener was disconnected, false otherwise. */
		constexpr bool disconnect(delegate<delegate<R(Args...)>> listener)
		{
			return disconnect(std::find(data.begin(), data.end(), listener));
		}
		/** Removes a listener delegate from the event.
		 * @param connection Connection of the listener to this event.
		 * @return True if the listener was disconnected, false otherwise. */
		constexpr bool disconnect(event_connection &connection) { return detail::disconnect_event(this, connection); }

		/** Invokes listeners of the event with the passed arguments.
		 *
		 * @param args Arguments passed to the listener delegates.
		 * @return Reference to this event. */
		constexpr basic_event &dispatch(Args... args) const
		{
			for (auto &listener : data) listener(std::forward<Args>(args)...);
			return *this;
		}
		/** @copydoc dispatch */
		constexpr basic_event &operator()(Args... args) const { return dispatch(std::forward<Args>(args)...); }
		/** Invokes listeners of the event with the passed arguments and collects the results using a callback.
		 *
		 * @param col Collector callback receiving results of listener calls.
		 * @param args Arguments passed to the listener delegates.
		 * @return Reference to this event.
		 *
		 * @note Collector may return a boolean indicating whether to continue execution of delegates. */
		template<typename F>
		constexpr basic_event &dispatch(F &&col, Args... args) const
			requires valid_collector<F>
		{
			for (auto &listener : data)
			{
				// clang-format off
				if constexpr (requires { { listener(std::forward<Args>(args)...) } -> std::same_as<bool>; })
				{
					if (!listener(std::forward<Args>(args)...))
						break;
				}
				else
					listener(std::forward<Args>(args)...);
				// clang-format on
			}
			return *this;
		}

		/** Returns iterator to the listener delegate that compares equal to the provided delegate or the end
		 * iterator if such listener is not found. */
		[[nodiscard]] constexpr iterator find(delegate<delegate<R(Args...)>> listener) const noexcept
		{
			return std::find(begin(), end(), listener);
		}

		/** Returns iterator to the listener delegate bound to the specified data instance, or an end iterator
		 * if such listener is not found. */
		template<typename T>
		[[nodiscard]] constexpr iterator find(T *value) const noexcept
		{
			return std::find_if(begin(), end(), [value](auto &l) { l.data() == value; });
		}
		/** @copydoc find */
		template<typename T>
		[[nodiscard]] constexpr iterator find(T &value) const noexcept
		{
			return find(std::addressof(value));
		}

		// clang-format off
		/** Returns iterator to the listener delegate bound to the specified member or free function or an end iterator
		 * if such listener is not found. */
		template<auto F>
		[[nodiscard]] constexpr iterator find() const noexcept
			requires(requires{ delegate{func_t<F>{}}; })
		{
			return find(delegate{func_t<F>{}});
		}
		/** @copydoc find */
		template<auto F>
		[[nodiscard]] constexpr iterator find(func_t<F>) const noexcept
			requires(requires{ find<F>(); })
		{
			return find<F>();
		}
		/** Returns iterator to the listener delegate bound to the specified member or free function and the specified
		 * data instance, or an end iterator if such listener is not found. */
		template<auto F, typename T>
		[[nodiscard]] constexpr iterator find(T *value) const noexcept
			requires(requires{ delegate{func_t<F>{}, value}; })
		{
			return find(delegate{func_t<F>{}, value});
		}
		/** @copydoc find */
		template<auto F, typename T>
		[[nodiscard]] constexpr iterator find(T &value) const noexcept
			requires(requires{ delegate{func_t<F>{}, value}; })
		{
			return find(delegate{func_t<F>{}, value});
		}
		/** @copydoc find */
		template<auto F, typename T>
		[[nodiscard]] constexpr iterator find(func_t<F>, T *value) const noexcept
			requires(requires{ find<F>(value); })
		{
			return find<F>(value);
		}
		/** @copydoc find */
		template<auto F, typename T>
		[[nodiscard]] constexpr iterator find(func_t<F>, T &value) const noexcept
			requires(requires{ find<F>(value); })
		{
			return find<F>(value);
		}
		// clang-format on

		constexpr void swap(basic_event &other) noexcept
		{
			using std::swap;
			swap(data, other.data);
		}
		friend constexpr void swap(basic_event &a, basic_event &b) noexcept { a.swap(b); }

	private:
		data_t data;
	};

	namespace detail
	{
		template<typename...>
		class event_alloc
		{
		};
		template<typename R, typename Alloc, typename... Args>
		class event_alloc<R(Args...), Alloc>
		{
			using type = std::allocator<delegate<R(Args...)>>;
		};
	}	 // namespace detail

	/** @brief Alias used to create an event type with a default allocator.
	 * @tparam Sign Signature of the event in the form of `R(Args...)`. */
	template<typename Sign>
	using event = basic_event<Sign, typename detail::event_alloc<Sign>::type>;
}	 // namespace sek