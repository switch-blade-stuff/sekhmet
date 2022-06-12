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

#include <algorithm>
#include <limits>
#include <vector>

#include "assert.hpp"
#include "delegate.hpp"

namespace sek
{
	template<typename, typename>
	class basic_event;
	template<typename>
	class event_proxy;
	template<typename>
	class subscriber_handle;

	/** @brief Id used to uniquely reference event subscribers. */
	using sub_id = std::ptrdiff_t;

	namespace detail
	{
		constexpr auto event_placeholder = static_cast<sub_id>(-1);
	}

	/** @brief Structure used to manage a set of delegates.
	 *
	 * @tparam R Return type of the event's delegates
	 * @tparam Args Arguments passed to event's delegates.
	 * @tparam Alloc Allocator type used for the internal state. */
	template<typename Alloc, typename R, typename... Args>
	class basic_event<R(Args...), Alloc>
	{
		using id_alloc_t = typename std::allocator_traits<Alloc>::template rebind_alloc<sub_id>;
		using id_data_t = std::vector<sub_id, id_alloc_t>;

		using delegate_t = delegate<R(Args...)>;
		struct subscriber
		{
			constexpr subscriber() noexcept = default;
			constexpr explicit subscriber(delegate_t d) noexcept : callback(std::move(d)) {}

			constexpr R operator()(Args... args) const noexcept { return callback(std::forward<Args>(args)...); }
			constexpr bool operator==(const delegate_t &d) const noexcept { return callback == d; }

			delegate_t callback;
			sub_id id;
		};

		using sub_alloc_t = typename std::allocator_traits<Alloc>::template rebind_alloc<subscriber>;
		using sub_data_t = std::vector<subscriber, sub_alloc_t>;

		// clang-format off
		template<typename F>
		constexpr static bool valid_collector = !std::is_void_v<R> && requires(F &&f, const delegate_t &d, Args &&...args) { f(d(std::forward<Args>(args)...)); };
		// clang-format on

		class event_iterator
		{
			friend class basic_event;

			using iter_t = typename sub_data_t::const_iterator;
			using ptr_t = typename sub_data_t::const_pointer;

			constexpr explicit event_iterator(ptr_t ptr) noexcept : m_ptr(ptr) {}
			constexpr explicit event_iterator(iter_t iter) noexcept : m_ptr(std::to_address(iter)) {}

		public:
			typedef delegate_t value_type;
			typedef const value_type *pointer;
			typedef const value_type &reference;
			typedef std::size_t size_type;
			typedef std::ptrdiff_t difference_type;
			typedef std::random_access_iterator_tag iterator_category;

		public:
			constexpr event_iterator() noexcept = default;

			constexpr event_iterator operator++(int) noexcept
			{
				auto temp = *this;
				++(*this);
				return temp;
			}
			constexpr event_iterator &operator++() noexcept
			{
				++m_ptr;
				return *this;
			}
			constexpr event_iterator &operator+=(difference_type n) noexcept
			{
				m_ptr += n;
				return *this;
			}
			constexpr event_iterator operator--(int) noexcept
			{
				auto temp = *this;
				--(*this);
				return temp;
			}
			constexpr event_iterator &operator--() noexcept
			{
				--m_ptr;
				return *this;
			}
			constexpr event_iterator &operator-=(difference_type n) noexcept
			{
				m_ptr -= n;
				return *this;
			}

			constexpr event_iterator operator+(difference_type n) const noexcept { return event_iterator{m_ptr + n}; }
			constexpr event_iterator operator-(difference_type n) const noexcept { return event_iterator{m_ptr - n}; }
			constexpr difference_type operator-(const event_iterator &other) const noexcept
			{
				return m_ptr - other.m_ptr;
			}

			/** Returns pointer to the target element. */
			[[nodiscard]] constexpr pointer get() const noexcept { return &m_ptr->callback; }
			/** @copydoc value */
			[[nodiscard]] constexpr pointer operator->() const noexcept { return get(); }

			/** Returns reference to the element at an offset. */
			[[nodiscard]] constexpr reference operator[](difference_type n) const noexcept { return m_ptr[n].callback; }
			/** Returns reference to the target element. */
			[[nodiscard]] constexpr reference operator*() const noexcept { return *get(); }

			[[nodiscard]] constexpr auto operator<=>(const event_iterator &) const noexcept = default;
			[[nodiscard]] constexpr bool operator==(const event_iterator &) const noexcept = default;

			constexpr void swap(event_iterator &other) noexcept { std::swap(m_ptr, other.m_ptr); }
			friend constexpr void swap(event_iterator &a, event_iterator &b) noexcept { a.swap(b); }

		private:
			ptr_t m_ptr;
		};

	public:
		typedef event_iterator iterator;
		typedef event_iterator const_iterator;
		typedef std::reverse_iterator<event_iterator> reverse_iterator;
		typedef std::reverse_iterator<event_iterator> const_reverse_iterator;
		typedef typename sub_data_t::allocator_type allocator_type;
		typedef typename sub_data_t::size_type size_type;
		typedef typename iterator::difference_type difference_type;
		typedef typename id_data_t::allocator_type id_allocator_type;

	public:
		/** Initializes an empty event. */
		constexpr basic_event() noexcept(noexcept(sub_data_t{}) &&noexcept(id_data_t{})) = default;

		/** Initializes an empty event.
		 * @param sub_alloc Allocator used to initialize internal subscriber storage. */
		constexpr explicit basic_event(const allocator_type &sub_alloc, const id_allocator_type &id_alloc = id_allocator_type{})
			: m_id_data(id_alloc), m_sub_data(sub_alloc)
		{
		}
		/** Initializes event with a set of delegates.
		 * @param il Initializer list containing delegates of the event.
		 * @param sub_alloc Allocator used to initialize internal subscriber storage.
		 * @param id_alloc Allocator used to initialize internal id storage. */
		constexpr basic_event(std::initializer_list<delegate<R(Args...)>> il,
							  const allocator_type &sub_alloc = allocator_type{},
							  const id_allocator_type &id_alloc = id_allocator_type{})
			: basic_event(id_alloc, sub_alloc)
		{
			for (auto &d : il) m_sub_data.emplace_back(d);
		}

		/** Checks if the event is empty (has no subscribers). */
		[[nodiscard]] constexpr bool empty() const noexcept { return m_sub_data.empty(); }
		/** Returns amount of subscribers bound to this event. */
		[[nodiscard]] constexpr size_type size() const noexcept { return m_sub_data.size(); }

		/** Returns iterator to the fist subscriber of the event. */
		[[nodiscard]] constexpr const_iterator begin() const noexcept { return const_iterator{m_sub_data.begin()}; }
		/** @copydoc begin */
		[[nodiscard]] constexpr const_iterator cbegin() const noexcept { return begin(); }
		/** Returns iterator one past the last subscriber of the event. */
		[[nodiscard]] constexpr const_iterator end() const noexcept { return const_iterator{m_sub_data.end()}; }
		/** @copydoc end */
		[[nodiscard]] constexpr const_iterator cend() const noexcept { return end(); }
		/** Returns reverse iterator one past the last subscriber of the event. */
		[[nodiscard]] constexpr const_reverse_iterator rbegin() const noexcept { return const_reverse_iterator{end()}; }
		/** @copydoc rbegin */
		[[nodiscard]] constexpr const_reverse_iterator crbegin() const noexcept { return rbegin(); }
		/** Returns reverse iterator to the first subscriber of the event. */
		[[nodiscard]] constexpr const_reverse_iterator rend() const noexcept { return const_reverse_iterator{begin()}; }
		/** @copydoc rend */
		[[nodiscard]] constexpr const_reverse_iterator crend() const noexcept { return rend(); }

		/** Adds a subscriber delegate to the event at the specified position and returns it's sub_id id.
		 * @param where Position within the event's set of subscribers at which to add the new subscriber.
		 * @param subscriber Subscriber delegate.
		 * @return Id of the sub_id. */
		constexpr sub_id subscribe(const_iterator where, delegate<R(Args...)> subscriber)
		{
			/* If there already is a subscriber at that position, increment it's id. */
			const auto pos = std::distance(begin(), where);
			if (where < end()) ++(m_id_data.begin()[m_sub_data.begin()[pos].id]);

			/* Insert the subscriber & reserve the id list if needed. */
			m_sub_data.emplace(m_sub_data.begin() + pos, subscriber);
			if (m_sub_data.size() > m_id_data.size()) [[unlikely]]
				m_id_data.resize(m_sub_data.size() * 2, detail::event_placeholder);

			/* If there already is an id we can re-use, use that id. */
			if (m_next_id != detail::event_placeholder)
			{
				const auto id = m_next_id;
				auto id_iter = m_id_data.begin() + id;

				m_next_id = *id_iter;
				*id_iter = pos;
				return m_sub_data.begin()[pos].id = static_cast<sub_id>(id);
			}

			/* Otherwise, generate new id starting at subscriber position. If there are no empty id slots,
			 * it is likely that every slot before the insert position is already occupied. */
			for (auto id_iter = m_id_data.begin() + pos;; ++id_iter)
			{
				SEK_ASSERT(id_iter != m_id_data.end(), "End of id list should never be reached");
				if (*id_iter == detail::event_placeholder)
				{
					*id_iter = pos;
					return m_sub_data.begin()[pos].id = static_cast<sub_id>(std::distance(m_id_data.begin(), id_iter));
				}
			}
		}
		/** Adds a subscriber delegate to the event and returns it's sub_id id.
		 * @param subscriber Subscriber delegate.
		 * @return Id of the sub_id. */
		constexpr sub_id subscribe(delegate<R(Args...)> subscriber) { return subscribe(end(), subscriber); }
		/** @copydoc subscribe */
		constexpr sub_id operator+=(delegate<R(Args...)> subscriber) { return subscribe(subscriber); }

		/** @brief Adds a subscriber delegate to the event after the specified subscriber.
		 * @param id Id of the sub_id after which to subscribe.
		 * @param subscriber Subscriber delegate.
		 * @return Id of the sub_id.
		 * @note If an existing subscriber does not exist, subscribes at the end. */
		constexpr sub_id subscribe_after(sub_id id, delegate<R(Args...)> subscriber)
		{
			if (const auto where = find(id); where != end()) [[likely]]
				return subscribe(std::next(where), std::move(subscriber));
			else
				return subscribe(end(), std::move(subscriber));
		}
		/** @copybrief subscribe_after
		 * @param existing Delegate comparing equal to an existing subscriber after which to subscribe.
		 * @param subscriber Subscriber delegate.
		 * @return Id of the sub_id.
		 * @note If an existing subscriber does not exist, subscribes at the end. */
		constexpr sub_id subscribe_after(delegate<R(Args...)> existing, delegate<R(Args...)> subscriber)
		{
			if (const auto where = find(existing); where != end()) [[likely]]
				return subscribe(std::next(where), std::move(subscriber));
			else
				return subscribe(end(), std::move(subscriber));
		}
		/** @copybrief subscribe_after
		 * @param value Data (instance or bound argument) of an existing subscriber after which to subscribe.
		 * @param subscriber Subscriber delegate.
		 * @return Id of the sub_id.
		 * @note If an existing subscriber does not exist, subscribes at the end. */
		template<typename T>
		constexpr sub_id subscribe_after(T *value, delegate<R(Args...)> subscriber)
		{
			if (const auto where = find<T>(value); where != end()) [[likely]]
				return subscribe(std::next(where), std::move(subscriber));
			else
				return subscribe(end(), std::move(subscriber));
		}
		/** @copydoc subscribe_after */
		template<typename T>
		constexpr sub_id subscribe_after(T &value, delegate<R(Args...)> subscriber)
		{
			if (const auto where = find<T>(value); where != end()) [[likely]]
				return subscribe(std::next(where), std::move(subscriber));
			else
				return subscribe(end(), std::move(subscriber));
		}

		/** @brief Adds a subscriber delegate to the event before the specified subscriber.
		 * @param id Id of the sub_id before which to subscribe.
		 * @param subscriber Subscriber delegate.
		 * @return Id of the sub_id.
		 * @note If an existing subscriber does not exist, subscribes at the start. */
		constexpr sub_id subscribe_before(sub_id id, delegate<R(Args...)> subscriber)
		{
			if (const auto where = find(id); where != end()) [[likely]]
				return subscribe(where, std::move(subscriber));
			else
				return subscribe(begin(), std::move(subscriber));
		}
		/** @copybrief subscribe_before
		 * @param existing Delegate comparing equal to an existing subscriber before which to subscribe.
		 * @param subscriber Subscriber delegate.
		 * @return Id of the sub_id.
		 * @note If an existing subscriber does not exist, subscribes at the start. */
		constexpr sub_id subscribe_before(delegate<R(Args...)> existing, delegate<R(Args...)> subscriber)
		{
			if (const auto where = find(existing); where != end()) [[likely]]
				return subscribe(where, std::move(subscriber));
			else
				return subscribe(begin(), std::move(subscriber));
		}
		/** @copybrief subscribe_before
		 * @param value Data (instance or bound argument) of an existing subscriber before which to subscribe.
		 * @param subscriber Subscriber delegate.
		 * @return Id of the sub_id.
		 * @note If an existing subscriber does not exist, subscribes at the start. */
		template<typename T>
		constexpr sub_id subscribe_before(T *value, delegate<R(Args...)> subscriber)
		{
			if (const auto where = find<T>(value); where != end()) [[likely]]
				return subscribe(where, std::move(subscriber));
			else
				return subscribe(begin(), std::move(subscriber));
		}
		/** @copydoc subscribe_before */
		template<typename T>
		constexpr sub_id subscribe_before(T &value, delegate<R(Args...)> subscriber)
		{
			if (const auto where = find<T>(value); where != end()) [[likely]]
				return subscribe(where, std::move(subscriber));
			else
				return subscribe(begin(), std::move(subscriber));
		}

		/** Removes a subscriber delegate pointed to by the specified iterator from the event.
		 * @param where Iterator pointing to the subscriber to be removed from the event.
		 * @return true if the subscriber was unsubscribed, false otherwise. */
		constexpr bool unsubscribe(const_iterator where)
		{
			if (where != end()) [[likely]]
			{
				const auto pos = static_cast<size_type>(std::distance(begin(), where));
				const auto old_id = m_sub_data[pos].id;

				/* Release id of the subscriber & add it to the re-use list. */
				m_id_data.begin()[old_id] = m_next_id;
				m_next_id = static_cast<sub_id>(old_id);

				/* Swap & pop the subscriber, updating the replacement one's id. */
				auto &replacement = (m_sub_data[pos] = std::move(m_sub_data.back()));
				m_id_data.begin()[replacement.id] = static_cast<sub_id>(pos);
				m_sub_data.pop_back();
				return true;
			}
			else
				return false;
		}
		/** Removes a subscriber delegate from the event.
		 * @param subscriber Delegate to remove from the event.
		 * @return true if the subscriber was unsubscribed, false otherwise. */
		constexpr bool unsubscribe(delegate<R(Args...)> subscriber)
		{
			return unsubscribe(std::find(begin(), end(), subscriber));
		}
		/** @copydoc unsubscribe */
		constexpr bool operator-=(delegate<R(Args...)> subscriber) { return unsubscribe(subscriber); }
		/** Removes a subscriber delegate from the event.
		 * @param sub_id Id of the event's sub_id.
		 * @return true if the subscriber was unsubscribed, false otherwise. */
		constexpr bool unsubscribe(sub_id sub_id)
		{
			SEK_ASSERT(static_cast<size_type>(sub_id) < m_id_data.size());
			return unsubscribe(begin() + m_id_data.begin()[sub_id]);
		}
		/** @copydoc unsubscribe */
		constexpr bool operator-=(sub_id sub_id) { return unsubscribe(sub_id); }

		/** Resets the event, removing all subscribers. */
		constexpr void clear()
		{
			m_id_data.clear();
			m_sub_data.clear();
			m_next_id = detail::event_placeholder;
		}

		/** Returns iterator to the subscriber delegate using it's sub_id id or if such subscriber is not found. */
		[[nodiscard]] constexpr iterator find(sub_id sub_id) const noexcept
		{
			auto iter = std::find_if(m_sub_data.begin(), m_sub_data.end(), [sub_id](auto &s) { return s.id == sub_id; });
			return begin() + (iter - m_sub_data.begin());
		}
		/** Returns iterator to the subscriber delegate that compares equal to the provided delegate or the end
		 * iterator if such subscriber is not found. */
		[[nodiscard]] constexpr iterator find(delegate<R(Args...)> subscriber) const noexcept
		{
			return std::find(begin(), end(), subscriber);
		}

		/** Returns iterator to the subscriber delegate bound to the specified data instance, or an end iterator
		 * if such subscriber is not found. */
		template<typename T>
		[[nodiscard]] constexpr iterator find(T *value) const noexcept
		{
			return std::find_if(begin(), end(), [value](auto &l) { return l.data() == value; });
		}
		/** @copydoc find */
		template<typename T>
		[[nodiscard]] constexpr iterator find(T &value) const noexcept
		{
			return find(std::addressof(value));
		}

		// clang-format off
		/** Returns iterator to the subscriber delegate bound to the specified member or free function or an end iterator
		 * if such subscriber is not found. */
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
		/** Returns iterator to the subscriber delegate bound to the specified member or free function and the specified
		 * data instance, or an end iterator if such subscriber is not found. */
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

		/** Invokes subscribers of the event with the passed arguments.
		 * @param args Arguments passed to the subscriber delegates.
		 * @return Reference to this event. */
		constexpr const basic_event &dispatch(Args... args) const
		{
			for (auto &subscriber : m_sub_data) subscriber(std::forward<Args>(args)...);
			return *this;
		}
		/** @copydoc dispatch */
		constexpr const basic_event &operator()(Args... args) const { return dispatch(std::forward<Args>(args)...); }
		/** Invokes subscribers of the event with the passed arguments and collects the results using a callback.
		 *
		 * @param col Collector callback receiving results of subscriber calls.
		 * @param args Arguments passed to the subscriber delegates.
		 * @return Reference to this event.
		 *
		 * @note Collector may return a boolean indicating whether to continue execution of delegates. */
		template<typename F>
		constexpr const basic_event &dispatch(F &&col, Args... args) const
			requires valid_collector<F>
		{
			for (auto &subscriber : m_sub_data)
			{
				// clang-format off
				if constexpr (requires { { col(subscriber(std::forward<Args>(args)...)) } -> std::same_as<bool>; })
				{
					if (!col(subscriber(std::forward<Args>(args)...)))
						break;
				}
				else
					col(subscriber(std::forward<Args>(args)...));
				// clang-format on
			}
			return *this;
		}
		/** @copydoc dispatch */
		template<typename F>
		constexpr const basic_event &operator()(F &&col, Args... args) const
		{
			return dispatch(std::forward<F>(col), std::forward<Args>(args)...);
		}

		constexpr void swap(basic_event &other) noexcept
		{
			using std::swap;
			swap(m_id_data, other.m_id_data);
			swap(m_sub_data, other.m_sub_data);
			swap(m_next_id, other.m_next_id);
		}
		friend constexpr void swap(basic_event &a, basic_event &b) noexcept { a.swap(b); }

	private:
		id_data_t m_id_data;
		sub_data_t m_sub_data;
		sub_id m_next_id = detail::event_placeholder;
	};

	namespace detail
	{
		template<typename...>
		struct event_alloc;
		template<typename R, typename... Args>
		struct event_alloc<R(Args...)>
		{
			using type = std::allocator<delegate<R(Args...)>>;
		};
	}	 // namespace detail

	/** @brief Alias used to create an event type with a default allocator.
	 * @tparam Sign Signature of the event in the form of `R(Args...)`. */
	template<typename Sign>
	using event = basic_event<Sign, typename detail::event_alloc<Sign>::type>;

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

		/** Adds a subscriber delegate to the underlying event at the specified position and returns it's sub_id id.
		 * @param where Position within the underlying event's set of subscribers at which to add the new subscriber.
		 * @param subscriber Subscriber delegate.
		 * @return Id of the sub_id. */
		constexpr sub_id subscribe(const_iterator where, delegate<R(Args...)> subscriber) const
		{
			return m_event.subscribe(where, std::move(subscriber));
		}
		/** Adds a subscriber delegate to the underlying event and returns it's sub_id id.
		 * @param subscriber Subscriber delegate.
		 * @return Id of the sub_id. */
		constexpr sub_id subscribe(delegate<R(Args...)> subscriber) const
		{
			return m_event.subscribe(std::move(subscriber));
		}
		/** @copydoc subscribe */
		constexpr sub_id operator+=(delegate<R(Args...)> subscriber) const { return m_event += std::move(subscriber); }

		/** @brief Adds a subscriber delegate to the underlying event after the specified subscriber.
		 * @param id Id of the sub_id after which to subscribe.
		 * @param subscriber Subscriber delegate.
		 * @return Id of the sub_id.
		 * @note If an existing subscriber does not exist, subscribes at the end. */
		constexpr sub_id subscribe_after(sub_id id, delegate<R(Args...)> subscriber) const
		{
			return m_event.subscribe_after(id, std::move(subscriber));
		}
		/** @copybrief subscribe_after
		 * @param existing Delegate comparing equal to an existing subscriber after which to subscribe.
		 * @param subscriber Subscriber delegate.
		 * @return Id of the sub_id.
		 * @note If an existing subscriber does not exist, subscribes at the end. */
		constexpr sub_id subscribe_after(delegate<R(Args...)> existing, delegate<R(Args...)> subscriber) const
		{
			return m_event.subscribe_after(std::move(existing), std::move(subscriber));
		}
		/** @copybrief subscribe_after
		 * @param value Data (instance or bound argument) of an existing subscriber after which to subscribe.
		 * @param subscriber Subscriber delegate.
		 * @return Id of the sub_id.
		 * @note If an existing subscriber does not exist, subscribes at the end. */
		template<typename T>
		constexpr sub_id subscribe_after(T *value, delegate<R(Args...)> subscriber) const
		{
			return m_event.template subscribe_after<T>(value, std::move(subscriber));
		}
		/** @copydoc subscribe_after */
		template<typename T>
		constexpr sub_id subscribe_after(T &value, delegate<R(Args...)> subscriber) const
		{
			return m_event.template subscribe_after<T>(value, std::move(subscriber));
		}

		/** @brief Adds a subscriber delegate to the underlying event before the specified subscriber.
		 * @param id Id of the sub_id before which to subscribe.
		 * @param subscriber Subscriber delegate.
		 * @return Id of the sub_id.
		 * @note If an existing subscriber does not exist, subscribes at the start. */
		constexpr sub_id subscribe_before(sub_id id, delegate<R(Args...)> subscriber) const
		{
			return m_event.subscribe_before(id, std::move(subscriber));
		}
		/** @copybrief subscribe_before
		 * @param existing Delegate comparing equal to an existing subscriber before which to subscribe.
		 * @param subscriber Subscriber delegate.
		 * @return Id of the sub_id.
		 * @note If an existing subscriber does not exist, subscribes at the start. */
		constexpr sub_id subscribe_before(delegate<R(Args...)> existing, delegate<R(Args...)> subscriber) const
		{
			return m_event.subscribe_before(std::move(existing), std::move(subscriber));
		}
		/** @copybrief subscribe_before
		 * @param value Data (instance or bound argument) of an existing subscriber before which to subscribe.
		 * @param subscriber Subscriber delegate.
		 * @return Id of the sub_id.
		 * @note If an existing subscriber does not exist, subscribes at the start. */
		template<typename T>
		constexpr sub_id subscribe_before(T *value, delegate<R(Args...)> subscriber) const
		{
			return m_event.template subscribe_before<T>(value, std::move(subscriber));
		}
		/** @copydoc subscribe_before */
		template<typename T>
		constexpr sub_id subscribe_before(T &value, delegate<R(Args...)> subscriber) const
		{
			return m_event.template subscribe_before<T>(value, std::move(subscriber));
		}

		/** Removes a subscriber delegate pointed to by the specified iterator from the underlying event.
		 * @param where Iterator pointing to the subscriber to be removed from the underlying event.
		 * @return true if the subscriber was unsubscribed, false otherwise. */
		constexpr bool unsubscribe(const_iterator where) const { return m_event.unsubscribe(where); }
		/** Removes a subscriber delegate from the underlying event.
		 * @param subscriber Delegate to remove from the underlying event.
		 * @return true if the subscriber was unsubscribed, false otherwise. */
		constexpr bool unsubscribe(delegate<R(Args...)> subscriber) const
		{
			return m_event.unsubscribe(std::move(subscriber));
		}
		/** @copydoc unsubscribe */
		constexpr bool operator-=(delegate<R(Args...)> subscriber) const { return m_event -= std::move(subscriber); }
		/** Removes a subscriber delegate from the underlying event.
		 * @param sub_id Id of the underlying event's sub_id.
		 * @return true if the subscriber was unsubscribed, false otherwise. */
		constexpr bool unsubscribe(sub_id sub_id) const { return m_event.unsubscribe(sub_id); }
		/** @copydoc unsubscribe */
		constexpr bool operator-=(sub_id sub_id) const { return m_event -= sub_id; }

		/** Returns iterator to the subscriber delegate using it's sub_id id or if such subscriber is not found. */
		[[nodiscard]] constexpr iterator find(sub_id sub_id) const noexcept { return m_event.find(sub_id); }
		/** Returns iterator to the subscriber delegate that compares equal to the provided delegate or the end
		 * iterator if such subscriber is not found. */
		[[nodiscard]] constexpr iterator find(delegate<R(Args...)> subscriber) const noexcept
		{
			return m_event.find(std::move(subscriber));
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
			requires(requires{ delegate{func_t<F>{}}; })
		{
			return m_event.template find<F>();
		}
		/** @copydoc find */
		template<auto F>
		[[nodiscard]] constexpr iterator find(func_t<F>) const noexcept
			requires(requires{ find<F>(); })
		{
			return m_event.find(func_t<F>{});
		}
		/** Returns iterator to the subscriber delegate bound to the specified member or free function and the specified
		 * data instance, or an end iterator if such subscriber is not found. */
		template<auto F, typename T>
		[[nodiscard]] constexpr iterator find(T *value) const noexcept
			requires(requires{ delegate{func_t<F>{}, value}; })
		{
			return m_event.template find<F, T>(value);
		}
		/** @copydoc find */
		template<auto F, typename T>
		[[nodiscard]] constexpr iterator find(T &value) const noexcept
			requires(requires{ delegate{func_t<F>{}, value}; })
		{
			return m_event.template find<F, T>(value);
		}
		/** @copydoc find */
		template<auto F, typename T>
		[[nodiscard]] constexpr iterator find(func_t<F>, T *value) const noexcept
			requires(requires{ find<F>(value); })
		{
			return m_event.find(func_t<F>{}, value);
		}
		/** @copydoc find */
		template<auto F, typename T>
		[[nodiscard]] constexpr iterator find(func_t<F>, T &value) const noexcept
			requires(requires{ find<F>(value); })
		{
			return m_event.find(func_t<F>{}, value);
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

		/** Initializes a handle to manage an event subscription. */
		constexpr subscriber_handle(sub_id id, event_proxy<event_type> proxy) noexcept
			: m_event(&proxy.m_event), m_sub(id)
		{
		}

		/** Checks if the handle manages an event subscriber. */
		[[nodiscard]] constexpr bool empty() const noexcept { return m_event == nullptr; }
		/** Returns id of the subscription. */
		[[nodiscard]] constexpr sub_id id() const noexcept { return m_sub; }
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
		constexpr bool manage(sub_id id, event_proxy<event_type> proxy) noexcept
		{
			const auto result = reset();
			m_event = &proxy.m_event;
			m_sub = id;
			return result;
		}

		/** Releases the subscription without resetting it.
		 * @return Id of the subscription previously managed by the handle. */
		[[nodiscard]] constexpr sub_id release() noexcept
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
		sub_id m_sub;
	};
}	 // namespace sek