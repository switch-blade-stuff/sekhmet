/*
 * Created by switchblade on 20/08/22
 */

#pragma once

#include "event.hpp"

namespace sek
{
	/** @brief Proxy wrapper around `basic_event`, that exposes subscriber-related functionality without
	 * allowing any other modification of the underlying event.
	 * @note Event proxy should not outlive the event it was created from. */
	template<typename Alloc, typename R, typename... Args>
	class event_proxy<basic_event<R(Args...), Alloc>>
	{
		friend class subscriber_handle<basic_event<R(Args...), Alloc>>;

	public:
		typedef basic_event<R(Args...), Alloc> event_type;
		typedef typename event_type::iterator iterator;
		typedef typename event_type::const_iterator const_iterator;
		typedef typename event_type::reverse_iterator reverse_iterator;
		typedef typename event_type::const_reverse_iterator const_reverse_iterator;
		typedef typename event_type::size_type size_type;
		typedef typename event_type::difference_type difference_type;

	public:
		event_proxy() = delete;

		constexpr event_proxy(const event_proxy &) noexcept = default;
		constexpr event_proxy &operator=(const event_proxy &) noexcept = default;
		constexpr event_proxy(event_proxy &&) noexcept = default;
		constexpr event_proxy &operator=(event_proxy &&) noexcept = default;

		/** Initializes event proxy from an event. */
		constexpr event_proxy(event_type &event) noexcept : m_event(event) {}

		/** Checks if the underlying event is empty (has no subscribers). */
		[[nodiscard]] constexpr bool empty() const noexcept { return m_event.empty(); }
		/** Returns amount of subscribers bound to the underlying event. */
		[[nodiscard]] constexpr size_type size() const noexcept { return m_event.size(); }

		/** Returns iterator to the fist subscriber of the underlying event. */
		[[nodiscard]] constexpr const_iterator begin() const noexcept { return m_event.begin(); }
		/** @copydoc begin */
		[[nodiscard]] constexpr const_iterator cbegin() const noexcept { return m_event.cbegin(); }
		/** Returns iterator one past the last subscriber of the underlying event. */
		[[nodiscard]] constexpr const_iterator end() const noexcept { return m_event.end(); }
		/** @copydoc end */
		[[nodiscard]] constexpr const_iterator cend() const noexcept { return m_event.cend(); }
		/** Returns reverse iterator one past the last subscriber of the underlying event. */
		[[nodiscard]] constexpr const_reverse_iterator rbegin() const noexcept { return m_event.rbegin(); }
		/** @copydoc rbegin */
		[[nodiscard]] constexpr const_reverse_iterator crbegin() const noexcept { return m_event.crbegin(); }
		/** Returns reverse iterator to the first subscriber of the underlying event. */
		[[nodiscard]] constexpr const_reverse_iterator rend() const noexcept { return m_event.rend(); }
		/** @copydoc rend */
		[[nodiscard]] constexpr const_reverse_iterator crend() const noexcept { return m_event.crend(); }

		/** Adds a subscriber delegate to the underlying event at the specified position and returns it's id.
		 * @param where Position within the underlying event's set of subscribers at which to add the new subscriber.
		 * @param subscriber Subscriber delegate.
		 * @return Id of the subscriber. */
		constexpr event_subscriber subscribe(const_iterator where, const delegate<R(Args...)> &subscriber)
		{
			return m_event.subscribe(where, subscriber);
		}
		/** @copydoc subscribe */
		constexpr event_subscriber subscribe(const_iterator where, delegate<R(Args...)> &&subscriber)
		{
			return m_event.subscribe(where, std::move(subscriber));
		}
		/** Adds a subscriber delegate to the underlying event and returns it's id.
		 * @param subscriber Subscriber delegate.
		 * @return Id of the subscriber. */
		constexpr event_subscriber subscribe(const delegate<R(Args...)> &subscriber)
		{
			return m_event.subscribe(subscriber);
		}
		/** @copydoc subscribe */
		constexpr event_subscriber subscribe(delegate<R(Args...)> &&subscriber)
		{
			return m_event.subscribe(std::move(subscriber));
		}
		/** @copydoc subscribe */
		constexpr event_subscriber operator+=(const delegate<R(Args...)> &subscriber) { return m_event += subscriber; }
		/** @copydoc subscribe */
		constexpr event_subscriber operator+=(delegate<R(Args...)> &&subscriber)
		{
			return m_event += std::move(subscriber);
		}

		/** @brief Adds a subscriber delegate to the underlying event after the specified subscriber.
		 * @param id Id of the subscriber after which to subscribe.
		 * @param subscriber Subscriber delegate.
		 * @return Id of the subscriber.
		 * @note If an existing subscriber does not exist, subscribes at the end. */
		constexpr event_subscriber subscribe_after(event_subscriber id, const delegate<R(Args...)> &subscriber)
		{
			return m_event.subscribe_after(id, subscriber);
		}
		/** @copydoc subscribe_after */
		constexpr event_subscriber subscribe_after(event_subscriber id, delegate<R(Args...)> &&subscriber)
		{
			return m_event.subscribe_after(id, std::move(subscriber));
		}
		/** @copybrief subscribe_after
		 * @param existing Delegate comparing equal to an existing subscriber after which to subscribe.
		 * @param subscriber Subscriber delegate.
		 * @return Id of the subscriber.
		 * @note If an existing subscriber does not exist, subscribes at the end. */
		constexpr event_subscriber subscribe_after(const delegate<R(Args...)> &existing, const delegate<R(Args...)> &subscriber)
		{
			return m_event.subscribe_after(existing, subscriber);
		}
		/** @copydoc subscribe_after */
		constexpr event_subscriber subscribe_after(const delegate<R(Args...)> &existing, delegate<R(Args...)> &&subscriber)
		{
			return m_event.subscribe_after(existing, std::move(subscriber));
		}
		/** @copybrief subscribe_after
		 * @param value Data (instance or bound argument) of an existing subscriber after which to subscribe.
		 * @param subscriber Subscriber delegate.
		 * @return Id of the subscriber.
		 * @note If an existing subscriber does not exist, subscribes at the end. */
		template<typename T>
		constexpr event_subscriber subscribe_after(T *value, const delegate<R(Args...)> &subscriber)
		{
			return m_event.template subscribe_after<T>(value, subscriber);
		}
		/** @copydoc subscribe_after */
		template<typename T>
		constexpr event_subscriber subscribe_after(T *value, delegate<R(Args...)> &&subscriber)
		{
			return m_event.template subscribe_after<T>(value, std::move(subscriber));
		}
		/** @copydoc subscribe_after */
		template<typename T>
		constexpr event_subscriber subscribe_after(T &value, const delegate<R(Args...)> &subscriber)
		{
			return m_event.template subscribe_after<T>(value, subscriber);
		}
		/** @copydoc subscribe_after */
		template<typename T>
		constexpr event_subscriber subscribe_after(T &value, delegate<R(Args...)> &&subscriber)
		{
			return m_event.template subscribe_after<T>(value, std::move(subscriber));
		}

		/** @brief Adds a subscriber delegate to the underlying event before the specified subscriber.
		 * @param id Id of the subscriber before which to subscribe.
		 * @param subscriber Subscriber delegate.
		 * @return Id of the subscriber.
		 * @note If an existing subscriber does not exist, subscribes at the start. */
		constexpr event_subscriber subscribe_before(event_subscriber id, const delegate<R(Args...)> &subscriber)
		{
			return m_event.subscribe_before(id, subscriber);
		}
		/** @copydoc subscribe_before */
		constexpr event_subscriber subscribe_before(event_subscriber id, delegate<R(Args...)> &&subscriber)
		{
			return m_event.subscribe_before(id, std::move(subscriber));
		}
		/** @copybrief subscribe_before
		 * @param existing Delegate comparing equal to an existing subscriber before which to subscribe.
		 * @param subscriber Subscriber delegate.
		 * @return Id of the subscriber.
		 * @note If an existing subscriber does not exist, subscribes at the start. */
		constexpr event_subscriber subscribe_before(const delegate<R(Args...)> &existing, const delegate<R(Args...)> &subscriber)
		{
			return m_event.subscribe_before(existing, subscriber);
		}
		/** @copydoc subscribe_before */
		constexpr event_subscriber subscribe_before(const delegate<R(Args...)> &existing, delegate<R(Args...)> &&subscriber)
		{
			return m_event.subscribe_before(existing, std::move(subscriber));
		}
		/** @copybrief subscribe_before
		 * @param value Data (instance or bound argument) of an existing subscriber before which to subscribe.
		 * @param subscriber Subscriber delegate.
		 * @return Id of the subscriber.
		 * @note If an existing subscriber does not exist, subscribes at the start. */
		template<typename T>
		constexpr event_subscriber subscribe_before(T *value, const delegate<R(Args...)> &subscriber)
		{
			return m_event.template subscribe_before<T>(value, subscriber);
		}
		/** @copydoc subscribe_before */
		template<typename T>
		constexpr event_subscriber subscribe_before(T *value, delegate<R(Args...)> &&subscriber)
		{
			return m_event.template subscribe_before<T>(value, std::move(subscriber));
		}
		/** @copydoc subscribe_before */
		template<typename T>
		constexpr event_subscriber subscribe_before(T &value, const delegate<R(Args...)> &subscriber)
		{
			return m_event.template subscribe_before<T>(value, subscriber);
		}
		/** @copydoc subscribe_before */
		template<typename T>
		constexpr event_subscriber subscribe_before(T &value, delegate<R(Args...)> &&subscriber)
		{
			return m_event.template subscribe_before<T>(value, std::move(subscriber));
		}

		/** Removes a subscriber delegate pointed to by the specified iterator from the underlying event.
		 * @param where Iterator pointing to the subscriber to be removed from the underlying event.
		 * @return true if the subscriber was unsubscribed, false otherwise. */
		constexpr bool unsubscribe(const_iterator where) { return m_event.unsubscribe(where); }
		/** Removes a subscriber delegate from the underlying event.
		 * @param subscriber Delegate to remove from the underlying event.
		 * @return true if the subscriber was unsubscribed, false otherwise. */
		constexpr bool unsubscribe(const delegate<R(Args...)> &subscriber) { return m_event.unsubscribe(subscriber); }
		/** @copydoc unsubscribe */
		constexpr bool operator-=(const delegate<R(Args...)> &subscriber) { return m_event -= subscriber; }
		/** Removes a subscriber delegate from the underlying event.
		 * @param id Id of the underlying event's event_subscriber.
		 * @return true if the subscriber was unsubscribed, false otherwise. */
		constexpr bool unsubscribe(event_subscriber id) { return m_event.unsubscribe(id); }
		/** @copydoc unsubscribe */
		constexpr bool operator-=(event_subscriber id) { return m_event -= id; }

		/** Returns iterator to the subscriber delegate using it's id or if such subscriber is not found. */
		[[nodiscard]] constexpr iterator find(event_subscriber id) const noexcept { return m_event.find(id); }
		/** Returns iterator to the subscriber delegate that compares equal to the provided delegate or the end
		 * iterator if such subscriber is not found. */
		[[nodiscard]] constexpr iterator find(const delegate<R(Args...)> &subscriber) const noexcept
		{
			return m_event.find(subscriber);
		}

		/** Returns iterator to the subscriber delegate bound to the specified data instance, or an end iterator
		 * if such subscriber is not found. */
		template<typename T>
		[[nodiscard]] constexpr iterator find(T *value) const noexcept
		{
			return m_event.template find<T>(value);
		}
		/** @copydoc find */
		template<typename T>
		[[nodiscard]] constexpr iterator find(T &value) const noexcept
		{
			return m_event.template find<T>(value);
		}

		// clang-format off
		/** Returns iterator to the subscriber delegate bound to the specified member or free function or an end iterator
		 * if such subscriber is not found. */
		template<auto F>
		[[nodiscard]] constexpr iterator find() const noexcept
			requires(requires{ delegate{delegate_func_t<F>{}}; })
		{
			return m_event.template find<F>();
		}
		/** @copydoc find */
		template<auto F>
		[[nodiscard]] constexpr iterator find(delegate_func_t<F>) const noexcept
			requires(requires{ find<F>(); })
		{
			return m_event.find(delegate_func_t<F>{});
		}
		/** Returns iterator to the subscriber delegate bound to the specified member or free function and the specified
		 * data instance, or an end iterator if such subscriber is not found. */
		template<auto F, typename T>
		[[nodiscard]] constexpr iterator find(T *value) const noexcept
			requires(requires{ delegate{delegate_func_t<F>{}, value}; })
		{
			return m_event.template find<F, T>(value);
		}
		/** @copydoc find */
		template<auto F, typename T>
		[[nodiscard]] constexpr iterator find(T &value) const noexcept
			requires(requires{ delegate{delegate_func_t<F>{}, value}; })
		{
			return m_event.template find<F, T>(value);
		}
		/** @copydoc find */
		template<auto F, typename T>
		[[nodiscard]] constexpr iterator find(delegate_func_t<F>, T *value) const noexcept
			requires(requires{ find<F>(value); })
		{
			return m_event.find(delegate_func_t<F>{}, value);
		}
		/** @copydoc find */
		template<auto F, typename T>
		[[nodiscard]] constexpr iterator find(delegate_func_t<F>, T &value) const noexcept
			requires(requires{ find<F>(value); })
		{
			return m_event.find(delegate_func_t<F>{}, value);
		}
		// clang-format on

	private:
		event_type &m_event;
	};

	template<typename Alloc, typename R, typename... Args>
	event_proxy(basic_event<R(Args...), Alloc> &) -> event_proxy<basic_event<R(Args...), Alloc>>;

	/** @brief RAII handle used to automatically un-register event subscribers on destruction.
	 * @warning Subscriber handle can not outlive the event it was created for. */
	template<typename Alloc, typename R, typename... Args>
	class subscriber_handle<basic_event<R(Args...), Alloc>>
	{
	public:
		typedef basic_event<R(Args...), Alloc> event_type;

	public:
		subscriber_handle(const subscriber_handle &) = delete;
		subscriber_handle &operator=(const subscriber_handle &) = delete;

		/** Initializes an empty handle. */
		constexpr subscriber_handle() noexcept = default;
		constexpr ~subscriber_handle()
		{
			if (m_event != nullptr) [[likely]]
				m_event->unsubscribe(m_sub);
		}

		constexpr subscriber_handle(subscriber_handle &&other) noexcept { swap(other); }
		constexpr subscriber_handle &operator=(subscriber_handle &&other) noexcept
		{
			swap(other);
			return *this;
		}

		/** Initializes a handle to manage an event subscriber. */
		constexpr subscriber_handle(event_subscriber id, event_proxy<event_type> proxy) noexcept
			: m_event(&proxy.m_event), m_sub(id)
		{
		}

		/** Checks if the handle manages an event subscriber. */
		[[nodiscard]] constexpr bool empty() const noexcept { return m_event == nullptr; }
		/** Returns id of the subscription. */
		[[nodiscard]] constexpr event_subscriber id() const noexcept { return m_sub; }
		/** Returns proxy to the host event of the subscription.
		 * @note If the handle is empty will result in undefined behavior. */
		[[nodiscard]] constexpr event_proxy<event_type> proxy() const noexcept
		{
			return event_proxy<event_type>{*m_event};
		}

		/** Resets the handle & manages a new subscription.
		 * @param id Id of the subscription to manage.
		 * @param proxy Event proxy to the host event.
		 * @return true if a previous subscription was reset, false otherwise (ex. if handle was empty). */
		constexpr bool manage(event_subscriber id, event_proxy<event_type> proxy) noexcept
		{
			const auto result = reset();
			m_event = &proxy.m_event;
			m_sub = id;
			return result;
		}

		/** Releases the subscription without resetting it.
		 * @return Id of the subscription previously managed by the handle. */
		[[nodiscard]] constexpr event_subscriber release() noexcept
		{
			m_event = nullptr;
			return m_sub;
		}
		/** Resets the subscription.
		 * @return true if the subscription was reset, false otherwise (ex. if handle was empty). */
		constexpr bool reset() noexcept
		{
			if (auto *ptr = std::exchange(m_event, nullptr); ptr != nullptr) [[likely]]
			{
				std::exchange(m_event, nullptr)->unsubscribe(m_sub);
				return true;
			}
			return false;
		}

		constexpr void swap(subscriber_handle &other) noexcept
		{
			std::swap(m_event, other.m_event);
			std::swap(m_sub, other.m_sub);
		}
		friend constexpr void swap(subscriber_handle &a, subscriber_handle &b) noexcept { a.swap(b); }

	private:
		event_type *m_event = nullptr;
		event_subscriber m_sub = -1;
	};
}	 // namespace sek