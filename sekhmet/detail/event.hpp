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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
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

	/** @brief Id used to uniquely reference event subscribers. */
	using event_id = std::ptrdiff_t;

	namespace detail
	{
		constexpr auto event_placeholder = static_cast<event_id>(-1);
	}

	/** @brief Structure used to manage a set of delegates.
	 *
	 * @tparam R Return type of the event's delegates
	 * @tparam Args Arguments passed to event's delegates.
	 * @tparam Alloc Allocator type used for the internal state. */
	template<typename Alloc, typename R, typename... Args>
	class basic_event<R(Args...), Alloc>
	{
		using id_alloc_t = typename std::allocator_traits<Alloc>::template rebind_alloc<event_id>;
		using id_data_t = std::vector<event_id, id_alloc_t>;

		using delegate_t = delegate<R(Args...)>;
		struct subscriber
		{
			constexpr subscriber() noexcept = default;
			constexpr explicit subscriber(delegate_t d) noexcept : callback(std::move(d)) {}

			constexpr R operator()(Args... args) const noexcept { return callback(std::forward<Args>(args)...); }
			constexpr bool operator==(const delegate_t &d) const noexcept { return callback == d; }

			delegate_t callback;
			event_id id;
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

			constexpr explicit event_iterator(ptr_t ptr) noexcept : ptr(ptr) {}
			constexpr explicit event_iterator(iter_t iter) noexcept : ptr(std::to_address(iter)) {}

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
				++ptr;
				return *this;
			}
			constexpr event_iterator &operator+=(difference_type n) noexcept
			{
				ptr += n;
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
				--ptr;
				return *this;
			}
			constexpr event_iterator &operator-=(difference_type n) noexcept
			{
				ptr -= n;
				return *this;
			}

			constexpr event_iterator operator+(difference_type n) const noexcept { return event_iterator{ptr + n}; }
			constexpr event_iterator operator-(difference_type n) const noexcept { return event_iterator{ptr - n}; }
			constexpr difference_type operator-(const event_iterator &other) const noexcept { return ptr - other.ptr; }

			/** Returns pointer to the target element. */
			[[nodiscard]] constexpr pointer get() const noexcept { return &ptr->callback; }
			/** @copydoc value */
			[[nodiscard]] constexpr pointer operator->() const noexcept { return get(); }

			/** Returns reference to the element at an offset. */
			[[nodiscard]] constexpr reference operator[](difference_type n) const noexcept { return ptr[n].callback; }
			/** Returns reference to the target element. */
			[[nodiscard]] constexpr reference operator*() const noexcept { return *get(); }

			[[nodiscard]] constexpr auto operator<=>(const event_iterator &) const noexcept = default;
			[[nodiscard]] constexpr bool operator==(const event_iterator &) const noexcept = default;

			constexpr void swap(event_iterator &other) noexcept { std::swap(ptr, other.ptr); }
			friend constexpr void swap(event_iterator &a, event_iterator &b) noexcept { a.swap(b); }

		private:
			ptr_t ptr;
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
			: id_data(id_alloc), sub_data(sub_alloc)
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
			for (auto &d : il) sub_data.emplace_back(d);
		}

		/** Checks if the event is empty (has no subscribers). */
		[[nodiscard]] constexpr bool empty() const noexcept { return sub_data.empty(); }
		/** Returns amount of subscribers bound to this event. */
		[[nodiscard]] constexpr size_type size() const noexcept { return sub_data.size(); }

		/** Returns iterator to the fist subscriber of the event. */
		[[nodiscard]] constexpr const_iterator begin() const noexcept { return const_iterator{sub_data.begin()}; }
		/** @copydoc begin */
		[[nodiscard]] constexpr const_iterator cbegin() const noexcept { return begin(); }
		/** Returns iterator one past the last subscriber of the event. */
		[[nodiscard]] constexpr const_iterator end() const noexcept { return const_iterator{sub_data.end()}; }
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

		/** Adds a subscriber delegate to the event at the specified position and returns it's subscription id.
		 * @param where Position within the event's set of subscribers at which to add the new subscriber.
		 * @param subscriber Subscriber delegate.
		 * @return Id of the subscription. */
		constexpr event_id subscribe(const_iterator where, delegate<R(Args...)> subscriber)
		{
			/* If there already is a subscriber at that position, increment it's id. */
			const auto pos = std::distance(begin(), where);
			if (where < end()) ++(id_data.begin()[sub_data.begin()[pos].id]);

			/* Insert the subscriber & resize the id list if needed. */
			sub_data.emplace(sub_data.begin() + pos, subscriber);
			if (sub_data.size() > id_data.size()) [[unlikely]]
				id_data.resize(sub_data.size() * 2, detail::event_placeholder);

			/* If there already is an id we can re-use, use that id. */
			if (next_id != detail::event_placeholder)
			{
				const auto id = next_id;
				auto id_iter = id_data.begin() + id;

				next_id = *id_iter;
				*id_iter = pos;
				return sub_data.begin()[pos].id = static_cast<event_id>(id);
			}

			/* Otherwise, generate new id starting at subscriber position. If there are no empty id slots,
			 * it is likely that every slot before the insert position is already occupied. */
			for (auto id_iter = id_data.begin() + pos;; ++id_iter)
			{
				SEK_ASSERT(id_iter != id_data.end(), "End of id list should never be reached");
				if (*id_iter == detail::event_placeholder)
				{
					*id_iter = pos;
					return sub_data.begin()[pos].id = static_cast<event_id>(std::distance(id_data.begin(), id_iter));
				}
			}
		}
		/** Adds a subscriber delegate to the event and returns it's subscription id.
		 * @param subscriber Subscriber delegate.
		 * @return Id of the subscription. */
		constexpr event_id subscribe(delegate<R(Args...)> subscriber) { return subscribe(end(), subscriber); }
		/** @copydoc subscribe */
		constexpr event_id operator+=(delegate<R(Args...)> subscriber) { return subscribe(subscriber); }

		/** @brief Adds a subscriber delegate to the event after the specified subscriber.
		 * @param id Id of the subscription after which to subscribe.
		 * @param subscriber Subscriber delegate.
		 * @return Id of the subscription.
		 * @note If an existing subscriber does not exist, subscribes at the end. */
		constexpr event_id subscribe_after(event_id id, delegate<R(Args...)> subscriber)
		{
			if (const auto where = find(id); where != end()) [[likely]]
				return subscribe(std::next(where), std::move(subscriber));
			else
				return subscribe(end(), std::move(subscriber));
		}
		/** @copybrief subscribe_after
		 * @param existing Delegate comparing equal to an existing subscriber after which to subscribe.
		 * @param subscriber Subscriber delegate.
		 * @return Id of the subscription.
		 * @note If an existing subscriber does not exist, subscribes at the end. */
		constexpr event_id subscribe_after(delegate<R(Args...)> existing, delegate<R(Args...)> subscriber)
		{
			if (const auto where = find(existing); where != end()) [[likely]]
				return subscribe(std::next(where), std::move(subscriber));
			else
				return subscribe(end(), std::move(subscriber));
		}
		/** @copybrief subscribe_after
		 * @param value Data (instance or bound argument) of an existing subscriber after which to subscribe.
		 * @param subscriber Subscriber delegate.
		 * @return Id of the subscription.
		 * @note If an existing subscriber does not exist, subscribes at the end. */
		template<typename T>
		constexpr event_id subscribe_after(T *value, delegate<R(Args...)> subscriber)
		{
			if (const auto where = find<T>(value); where != end()) [[likely]]
				return subscribe(std::next(where), std::move(subscriber));
			else
				return subscribe(end(), std::move(subscriber));
		}
		/** @copydoc subscribe_after */
		template<typename T>
		constexpr event_id subscribe_after(T &value, delegate<R(Args...)> subscriber)
		{
			if (const auto where = find<T>(value); where != end()) [[likely]]
				return subscribe(std::next(where), std::move(subscriber));
			else
				return subscribe(end(), std::move(subscriber));
		}

		/** @brief Adds a subscriber delegate to the event before the specified subscriber.
		 * @param id Id of the subscription before which to subscribe.
		 * @param subscriber Subscriber delegate.
		 * @return Id of the subscription.
		 * @note If an existing subscriber does not exist, subscribes at the start. */
		constexpr event_id subscribe_before(event_id id, delegate<R(Args...)> subscriber)
		{
			if (const auto where = find(id); where != end()) [[likely]]
				return subscribe(where, std::move(subscriber));
			else
				return subscribe(begin(), std::move(subscriber));
		}
		/** @copybrief subscribe_before
		 * @param existing Delegate comparing equal to an existing subscriber before which to subscribe.
		 * @param subscriber Subscriber delegate.
		 * @return Id of the subscription.
		 * @note If an existing subscriber does not exist, subscribes at the start. */
		constexpr event_id subscribe_before(delegate<R(Args...)> existing, delegate<R(Args...)> subscriber)
		{
			if (const auto where = find(existing); where != end()) [[likely]]
				return subscribe(where, std::move(subscriber));
			else
				return subscribe(begin(), std::move(subscriber));
		}
		/** @copybrief subscribe_before
		 * @param value Data (instance or bound argument) of an existing subscriber before which to subscribe.
		 * @param subscriber Subscriber delegate.
		 * @return Id of the subscription.
		 * @note If an existing subscriber does not exist, subscribes at the start. */
		template<typename T>
		constexpr event_id subscribe_before(T *value, delegate<R(Args...)> subscriber)
		{
			if (const auto where = find<T>(value); where != end()) [[likely]]
				return subscribe(where, std::move(subscriber));
			else
				return subscribe(begin(), std::move(subscriber));
		}
		/** @copydoc subscribe_before */
		template<typename T>
		constexpr event_id subscribe_before(T &value, delegate<R(Args...)> subscriber)
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
				const auto old_id = sub_data[pos].id;

				/* Release id of the subscriber & add it to the re-use list. */
				id_data.begin()[old_id] = next_id;
				next_id = static_cast<event_id>(old_id);

				/* Swap & pop the subscriber, updating the replacement one's id. */
				auto &replacement = (sub_data[pos] = std::move(sub_data.back()));
				id_data.begin()[replacement.id] = static_cast<event_id>(pos);
				sub_data.pop_back();
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
		 * @param sub_id Id of the event's subscription.
		 * @return true if the subscriber was unsubscribed, false otherwise. */
		constexpr bool unsubscribe(event_id sub_id)
		{
			SEK_ASSERT(static_cast<size_type>(sub_id) < id_data.size());
			return unsubscribe(begin() + id_data.begin()[sub_id]);
		}
		/** @copydoc unsubscribe */
		constexpr bool operator-=(event_id sub_id) { return unsubscribe(sub_id); }

		/** Returns iterator to the subscriber delegate using it's subscription id or if such subscriber is not found. */
		[[nodiscard]] constexpr iterator find(event_id sub_id) const noexcept
		{
			auto iter = std::find_if(sub_data.begin(), sub_data.end(), [sub_id](auto &s) { return s.id == sub_id; });
			return begin() + (iter - sub_data.begin());
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
			for (auto &subscriber : sub_data) subscriber(std::forward<Args>(args)...);
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
			for (auto &subscriber : sub_data)
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
			swap(id_data, other.id_data);
			swap(sub_data, other.sub_data);
			swap(next_id, other.next_id);
		}
		friend constexpr void swap(basic_event &a, basic_event &b) noexcept { a.swap(b); }

	private:
		id_data_t id_data;
		sub_data_t sub_data;
		event_id next_id = detail::event_placeholder;
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
		constexpr event_proxy(event_type &event) noexcept : event(event) {}

		/** Checks if the underlying event is empty (has no subscribers). */
		[[nodiscard]] constexpr bool empty() const noexcept { return event.empty(); }
		/** Returns amount of subscribers bound to the underlying event. */
		[[nodiscard]] constexpr size_type size() const noexcept { return event.size(); }

		/** Returns iterator to the fist subscriber of the underlying event. */
		[[nodiscard]] constexpr const_iterator begin() const noexcept { return event.begin(); }
		/** @copydoc begin */
		[[nodiscard]] constexpr const_iterator cbegin() const noexcept { return event.cbegin(); }
		/** Returns iterator one past the last subscriber of the underlying event. */
		[[nodiscard]] constexpr const_iterator end() const noexcept { return event.end(); }
		/** @copydoc end */
		[[nodiscard]] constexpr const_iterator cend() const noexcept { return event.cend(); }
		/** Returns reverse iterator one past the last subscriber of the underlying event. */
		[[nodiscard]] constexpr const_reverse_iterator rbegin() const noexcept { return event.rbegin(); }
		/** @copydoc rbegin */
		[[nodiscard]] constexpr const_reverse_iterator crbegin() const noexcept { return event.crbegin(); }
		/** Returns reverse iterator to the first subscriber of the underlying event. */
		[[nodiscard]] constexpr const_reverse_iterator rend() const noexcept { return event.rend(); }
		/** @copydoc rend */
		[[nodiscard]] constexpr const_reverse_iterator crend() const noexcept { return event.crend(); }

		/** Adds a subscriber delegate to the underlying event at the specified position and returns it's subscription id.
		 * @param where Position within the underlying event's set of subscribers at which to add the new subscriber.
		 * @param subscriber Subscriber delegate.
		 * @return Id of the subscription. */
		constexpr event_id subscribe(const_iterator where, delegate<R(Args...)> subscriber) const
		{
			return event.subscribe(where, std::move(subscriber));
		}
		/** Adds a subscriber delegate to the underlying event and returns it's subscription id.
		 * @param subscriber Subscriber delegate.
		 * @return Id of the subscription. */
		constexpr event_id subscribe(delegate<R(Args...)> subscriber) const
		{
			return event.subscribe(std::move(subscriber));
		}
		/** @copydoc subscribe */
		constexpr event_id operator+=(delegate<R(Args...)> subscriber) const { return event += std::move(subscriber); }

		/** @brief Adds a subscriber delegate to the underlying event after the specified subscriber.
		 * @param id Id of the subscription after which to subscribe.
		 * @param subscriber Subscriber delegate.
		 * @return Id of the subscription.
		 * @note If an existing subscriber does not exist, subscribes at the end. */
		constexpr event_id subscribe_after(event_id id, delegate<R(Args...)> subscriber) const
		{
			return event.subscribe_after(id, std::move(subscriber));
		}
		/** @copybrief subscribe_after
		 * @param existing Delegate comparing equal to an existing subscriber after which to subscribe.
		 * @param subscriber Subscriber delegate.
		 * @return Id of the subscription.
		 * @note If an existing subscriber does not exist, subscribes at the end. */
		constexpr event_id subscribe_after(delegate<R(Args...)> existing, delegate<R(Args...)> subscriber) const
		{
			return event.subscribe_after(std::move(existing), std::move(subscriber));
		}
		/** @copybrief subscribe_after
		 * @param value Data (instance or bound argument) of an existing subscriber after which to subscribe.
		 * @param subscriber Subscriber delegate.
		 * @return Id of the subscription.
		 * @note If an existing subscriber does not exist, subscribes at the end. */
		template<typename T>
		constexpr event_id subscribe_after(T *value, delegate<R(Args...)> subscriber) const
		{
			return event.template subscribe_after<T>(value, std::move(subscriber));
		}
		/** @copydoc subscribe_after */
		template<typename T>
		constexpr event_id subscribe_after(T &value, delegate<R(Args...)> subscriber) const
		{
			return event.template subscribe_after<T>(value, std::move(subscriber));
		}

		/** @brief Adds a subscriber delegate to the underlying event before the specified subscriber.
		 * @param id Id of the subscription before which to subscribe.
		 * @param subscriber Subscriber delegate.
		 * @return Id of the subscription.
		 * @note If an existing subscriber does not exist, subscribes at the start. */
		constexpr event_id subscribe_before(event_id id, delegate<R(Args...)> subscriber) const
		{
			return event.subscribe_before(id, std::move(subscriber));
		}
		/** @copybrief subscribe_before
		 * @param existing Delegate comparing equal to an existing subscriber before which to subscribe.
		 * @param subscriber Subscriber delegate.
		 * @return Id of the subscription.
		 * @note If an existing subscriber does not exist, subscribes at the start. */
		constexpr event_id subscribe_before(delegate<R(Args...)> existing, delegate<R(Args...)> subscriber) const
		{
			return event.subscribe_before(std::move(existing), std::move(subscriber));
		}
		/** @copybrief subscribe_before
		 * @param value Data (instance or bound argument) of an existing subscriber before which to subscribe.
		 * @param subscriber Subscriber delegate.
		 * @return Id of the subscription.
		 * @note If an existing subscriber does not exist, subscribes at the start. */
		template<typename T>
		constexpr event_id subscribe_before(T *value, delegate<R(Args...)> subscriber) const
		{
			return event.template subscribe_before<T>(value, std::move(subscriber));
		}
		/** @copydoc subscribe_before */
		template<typename T>
		constexpr event_id subscribe_before(T &value, delegate<R(Args...)> subscriber) const
		{
			return event.template subscribe_before<T>(value, std::move(subscriber));
		}

		/** Removes a subscriber delegate pointed to by the specified iterator from the underlying event.
		 * @param where Iterator pointing to the subscriber to be removed from the underlying event.
		 * @return true if the subscriber was unsubscribed, false otherwise. */
		constexpr bool unsubscribe(const_iterator where) const { return event.unsubscribe(where); }
		/** Removes a subscriber delegate from the underlying event.
		 * @param subscriber Delegate to remove from the underlying event.
		 * @return true if the subscriber was unsubscribed, false otherwise. */
		constexpr bool unsubscribe(delegate<R(Args...)> subscriber) const
		{
			return event.unsubscribe(std::move(subscriber));
		}
		/** @copydoc unsubscribe */
		constexpr bool operator-=(delegate<R(Args...)> subscriber) const { return event -= std::move(subscriber); }
		/** Removes a subscriber delegate from the underlying event.
		 * @param sub_id Id of the underlying event's subscription.
		 * @return true if the subscriber was unsubscribed, false otherwise. */
		constexpr bool unsubscribe(event_id sub_id) const { return event.unsubscribe(sub_id); }
		/** @copydoc unsubscribe */
		constexpr bool operator-=(event_id sub_id) const { return event -= sub_id; }

		/** Returns iterator to the subscriber delegate using it's subscription id or if such subscriber is not found. */
		[[nodiscard]] constexpr iterator find(event_id sub_id) const noexcept { return event.find(sub_id); }
		/** Returns iterator to the subscriber delegate that compares equal to the provided delegate or the end
		 * iterator if such subscriber is not found. */
		[[nodiscard]] constexpr iterator find(delegate<R(Args...)> subscriber) const noexcept
		{
			return event.find(std::move(subscriber));
		}

		/** Returns iterator to the subscriber delegate bound to the specified data instance, or an end iterator
		 * if such subscriber is not found. */
		template<typename T>
		[[nodiscard]] constexpr iterator find(T *value) const noexcept
		{
			return event.template find<T>(value);
		}
		/** @copydoc find */
		template<typename T>
		[[nodiscard]] constexpr iterator find(T &value) const noexcept
		{
			return event.template find<T>(value);
		}

		// clang-format off
		/** Returns iterator to the subscriber delegate bound to the specified member or free function or an end iterator
		 * if such subscriber is not found. */
		template<auto F>
		[[nodiscard]] constexpr iterator find() const noexcept
			requires(requires{ delegate{func_t<F>{}}; })
		{
			return event.template find<F>();
		}
		/** @copydoc find */
		template<auto F>
		[[nodiscard]] constexpr iterator find(func_t<F>) const noexcept
			requires(requires{ find<F>(); })
		{
			return event.find(func_t<F>{});
		}
		/** Returns iterator to the subscriber delegate bound to the specified member or free function and the specified
		 * data instance, or an end iterator if such subscriber is not found. */
		template<auto F, typename T>
		[[nodiscard]] constexpr iterator find(T *value) const noexcept
			requires(requires{ delegate{func_t<F>{}, value}; })
		{
			return event.template find<F, T>(value);
		}
		/** @copydoc find */
		template<auto F, typename T>
		[[nodiscard]] constexpr iterator find(T &value) const noexcept
			requires(requires{ delegate{func_t<F>{}, value}; })
		{
			return event.template find<F, T>(value);
		}
		/** @copydoc find */
		template<auto F, typename T>
		[[nodiscard]] constexpr iterator find(func_t<F>, T *value) const noexcept
			requires(requires{ find<F>(value); })
		{
			return event.find(func_t<F>{}, value);
		}
		/** @copydoc find */
		template<auto F, typename T>
		[[nodiscard]] constexpr iterator find(func_t<F>, T &value) const noexcept
			requires(requires{ find<F>(value); })
		{
			return event.find(func_t<F>{}, value);
		}
		// clang-format on

	private:
		event_type &event;
	};

	template<typename Alloc, typename R, typename... Args>
	event_proxy(basic_event<R(Args...), Alloc> &) -> event_proxy<basic_event<R(Args...), Alloc>>;
}	 // namespace sek