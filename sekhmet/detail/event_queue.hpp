//
// Created by switchblade on 06/05/22.
//

#pragma once

#include <memory>
#include <vector>

#include "type_id.hpp"

namespace sek
{
	template<typename>
	class event_queue;

	/** @brief Structure implementing generic event queue functionality. */
	template<>
	class event_queue<void>
	{
		using sub_func = void (*)(void *, void *);

		template<typename>
		friend class event_queue;

	public:
		/** @brief Unique identifier for a subscriber. */
		class subscriber
		{
			friend class event_queue<void>;

		public:
			subscriber() = delete;

			constexpr subscriber(const subscriber &) noexcept = default;
			constexpr subscriber &operator=(const subscriber &) noexcept = default;
			constexpr subscriber(subscriber &&) noexcept = default;
			constexpr subscriber &operator=(subscriber &&) noexcept = default;

			[[nodiscard]] constexpr auto operator<=>(const subscriber &) const noexcept = default;
			[[nodiscard]] constexpr bool operator==(const subscriber &) const noexcept = default;

		private:
		};

		/** Returns instance of an event queue for a particular event type.
		 * @param event Type id of the event.
		 * @return Reference to an instance of the event queue. */
		SEK_API static event_queue &instance(type_id id);

	public:
		event_queue(const event_queue &) = delete;
		event_queue &operator=(const event_queue &) = delete;
		event_queue(event_queue &&) = delete;
		event_queue &operator=(event_queue &&) = delete;

		event_queue() = default;
		~event_queue() = default;

		/** Adds a subscriber for a particular event.
		 * @param event Type id of the event.
		 * @param func Function used to listen for the event, invoked with pointers to the subscriber data & event data.
		 * @param data Shared pointer to the subscriber's data (can be null).
		 * @return Unique id of the subscriber. */
		SEK_API subscriber subscribe(sub_func func, std::shared_ptr<void> data = nullptr);
		/** Removes a subscriber from a particular event.
		 * @param event Type id of the event.
		 * @param sub Id of the subscriber obtained via a call to `subscribe`. */
		SEK_API void unsubscribe(subscriber sub);

		/** Queues an event for dispatching.
		 * @param event Type id of the event.
		 * @param data Shared pointer to event's data. */
		SEK_API void queue(std::shared_ptr<void> data);

		/** Dispatches queued events of a specific type (single-threaded).
		 * @param event Type id of the event. */
		SEK_API void dispatch();
		/** Dispatches queued events of a specific type (multi-threaded).
		 * @param event Type id of the event. */
		SEK_API void dispatch_async();

		/** Immediately sends an event to all subscribes (single-threaded).
		 * @param event Type id of the event.
		 * @param data Pointer to event's data (can be null). */
		SEK_API void send(void *data);
		/** Immediately sends an event to all subscribes (multi-threaded).
		 * @param event Type id of the event.
		 * @param data Pointer to event's data (can be null). */
		SEK_API void send_async(void *data);

	private:
	};

	extern template class SEK_API event_queue<void>;

	/** @brief Structure used to manage event listeners & events of a specific type. */
	template<typename EventType>
	class event_queue
	{
	public:
		typedef EventType event_type;

	private:
		static event_queue<void> &instance()
		{
			return static_cast<event_queue &>(event_queue<void>::instance(type_id::get<event_type>()));
		}

	public:
		/** Adds a subscriber to this queue.
		 * @param func Functor used to listen for the event.
		 * @return Integer id of the subscriber (unique within the event). */
		template<typename F>
		static event_queue<void>::subscriber subscribe(F &&f)
		{
			struct proxy
			{
				static void invoke(void *p, void *data)
				{
					std::invoke(static_cast<proxy *>(p)->f, static_cast<event_type *>(data));
				}

				constexpr proxy(F &&f) : f(std::forward<F>(f)) {}

				F f;
			};

			return instance().subscribe(proxy::invoke,
										std::static_pointer_cast<void>(std::make_shared<proxy>(std::forward<F>(f))));
		}
		/** Removes a subscriber from this queue.
		 * @param sub Id of the subscriber obtained via a call to `subscribe`. */
		static void unsubscribe(event_queue<void>::subscriber sub) { instance().unsubscribe(sub); }

		/** Queues an event for dispatching.
		 * @param data Shared pointer to event's data. */
		static void queue(std::shared_ptr<event_type> data)
		{
			instance().queue(std::static_pointer_cast<void>(std::move(data)));
		}

		/** Dispatches queued events (single-threaded). */
		static void dispatch() { instance().dispatch(); }
		/** Dispatches queued events (multi-threaded). */
		static void dispatch_async() { instance().dispatch_async(); }

		/** Immediately sends an event to all subscribes (single-threaded).
		 * @param data Pointer to event's data (can be null). */
		static void send(event_type *data) { instance().send(static_cast<void *>(data)); }
		/** Immediately sends an event to all subscribes (multi-threaded).
		 * @param data Pointer to event's data (can be null). */
		static void send_async(event_type *data) { instance().send_async(static_cast<void *>(data)); }
	};
}	 // namespace sek