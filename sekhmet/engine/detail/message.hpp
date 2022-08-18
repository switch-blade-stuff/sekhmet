/*
 * Created by switchblade on 25/05/22
 */

#pragma once

#include <mutex>

#include "sekhmet/access_guard.hpp"
#include "sekhmet/dense_map.hpp"
#include "sekhmet/event.hpp"

#include "type_info.hpp"

namespace sek::engine
{
	enum class message_scope : int
	{
		/** Messages are dispatched through the synchronized global message queue. */
		GLOBAL = 1,
		/** Messages are dispatched through the thread-local message queue. */
		THREAD = 2,
		/** Messages are dispatched through both the synchronized global and thread-local message queues. */
		ALL = GLOBAL | THREAD,
	};

	namespace detail
	{
		template<message_scope>
		struct message_sync
		{
			std::mutex mtx;
		};
		template<>
		struct message_sync<message_scope::THREAD>
		{
		};

		template<typename T, message_scope S>
		struct queue_data : queue_data<void, S>
		{
			event<bool(const T &)> receive_event;
			event<bool(const T &)> send_event;
		};
		template<message_scope S>
		struct queue_data<void, S> : message_sync<S>
		{
			std::vector<any> messages;
		};
	}	 // namespace detail

	/** @brief Generic message queue used to queue & dispatch messages in a type-erased way.
	 * @tparam Scope Target scope of the message queue.
	 * @note Queues of different scopes are separate from each other. */
	template<message_scope Scope = message_scope::ALL>
	class generic_message_queue
	{
	public:
		/** Queues a message for later dispatch.
		 * @tparam type Type of the message to be queued.
		 * @param value `any` containing value of the message. */
		inline static void queue(std::string_view type, any value);
		/** @copydoc send */
		inline static void queue(type_info type, any value);
		/** Sends a message immediately, bypassing the queue.
		 * @tparam type Type of the message to be sent.
		 * @param value `any` containing value of the message. */
		inline static void send(std::string_view type, any value);
		/** @copydoc send */
		inline static void send(type_info type, any value);

		/** Dispatches queued messages of the specified type. */
		inline static void dispatch(std::string_view type);
		/** @copydoc send */
		inline static void dispatch(type_info type);
		/** Dispatches queued messages of all types. */
		inline static void dispatch();
	};

	/** @brief Type-specific message queue used to queue & dispatch messages.
	 * @tparam T Message type handled by the message queue.
	 * @tparam Scope Target scope of the message queue.
	 * @note Queues of different scopes are separate from each other. */
	template<typename T, message_scope Scope = message_scope::ALL>
	class message_queue
	{
	public:
		typedef event<bool(const T &)> event_type;

	public:
		/** Queues a message for later dispatch.
		 * @param value `any` containing the value of the message. */
		inline static void queue(any value);
		/** Sends a message immediately, bypassing the queue.
		 * @param value `any` containing the value of the message. */
		inline static void send(any value);
		/** Dispatches all queued messages. */
		inline static void dispatch();

		/** @brief Returns proxy for the receive event.
		 *
		 * Receive event is invoked when a message is sent or dispatched and is used to listen for message data.
		 *
		 * Event subscribers can return `false` to prematurely terminate message dispatching.
		 *
		 * @return Pair where first is the lock used to synchronize message queue, and second is the event proxy. */
		inline static ref_guard<event_proxy<event_type>, std::mutex> on_receive();
		/** @brief Returns proxy for the send event.
		 *
		 * Send event is invoked when a message is sent or queued and can be used to filter the message data.
		 *
		 * Event subscribers can return `false` to prematurely terminate message sending
		 * (terminated message will not be dispatched to the receive event).
		 * @return Pair where first is the lock used to synchronize message queue, and second is the event proxy. */
		inline static ref_guard<event_proxy<event_type>, std::mutex> on_send();
	};
	/** @brief Global `message_queue` of type `T`. Global queues are synchronized via an internal mutex. */
	template<typename T>
	class message_queue<T, message_scope::GLOBAL>
	{
	public:
		typedef event<bool(const T &)> event_type;

	public:
		/** Queues a message for later dispatch.
		 * @param value Value of the message to be sent. */
		inline static void queue(const T &value);
		/** Sends a message immediately, bypassing the queue.
		 * @param value Value of the message to be sent. */
		inline static void send(const T &value);
		/** Dispatches all queued messages. */
		inline static void dispatch();

		/** @brief Returns proxy for the receive event.
		 *
		 * Receive event is invoked when a message is sent or dispatched and is used to listen for message data.
		 * Event subscribers can return `false` to prematurely terminate message dispatching.
		 *
		 * @return Access guard to the event proxy. */
		inline static ref_guard<event_proxy<event_type>, std::mutex> on_receive();
		/** @brief Returns proxy for the send event.
		 *
		 * Send event is invoked when a message is sent or queued and is used to filter message data. Event subscribers
		 * can return `false` to prematurely terminate message sending (terminated message will not be dispatched to
		 * the receive event).
		 *
		 * @return Access guard to the event proxy. */
		inline static ref_guard<event_proxy<event_type>, std::mutex> on_send();
	};
	/** @brief Thread-local `message_queue` of type `T`. Thread-local queues are not synchronized. */
	template<typename T>
	class message_queue<T, message_scope::THREAD>
	{
	public:
		typedef event<bool(const T &)> event_type;

	public:
		/** Queues a message for later dispatch.
		 * @param value Value of the message to be sent. */
		inline static void queue(const T &value);
		/** Sends a message immediately, bypassing the queue.
		 * @param value Value of the message to be sent. */
		inline static void send(const T &value);
		/** Dispatches all queued messages. */
		inline static void dispatch();

		/** @brief Returns proxy for the receive event.
		 *
		 * Receive event is invoked when a message is sent or dispatched and is used to listen for message data.
		 * Event subscribers can return `false` to prematurely terminate message dispatching.
		 *
		 * @return Event proxy for the receive event. */
		inline static event_proxy<event_type> on_receive();
		/** @brief Returns proxy for the send event.
		 *
		 * Send event is invoked when a message is sent or queued and is used to filter message data. Event subscribers
		 * can return `false` to prematurely terminate message sending (terminated message will not be dispatched to
		 * the receive event).
		 *
		 * @return Event proxy for the send event. */
		inline static event_proxy<event_type> on_send();
	};

	namespace attributes
	{
		/** @brief Attribute used to send messages of a specific type at runtime in a type-agnostic way. */
		class message_type
		{
		public:
			message_type() = delete;

			template<typename T>
			constexpr message_type(type_selector_t<T>) noexcept : m_type(type_info::get<T>())
			{
			}

			constexpr message_type(const message_type &) noexcept = default;
			constexpr message_type &operator=(const message_type &) noexcept = default;
			constexpr message_type(message_type &&) noexcept = default;
			constexpr message_type &operator=(message_type &&) noexcept = default;

			/** Returns type info of the underlying message type. */
			[[nodiscard]] constexpr type_info type() const noexcept { return m_type; }

			/** Queues message using the message queue for the bound type.
			 * @tparam Scope Scope of the target message queue.
			 * @param value `any` containing value of the message. */
			template<message_scope Scope = message_scope::GLOBAL>
			void queue(any data) const
			{
				generic_message_queue<Scope>::queue(m_type, std::move(data));
			}
			/** Sends message using the message queue for the bound type.
			 * @tparam Scope Scope of the target message queue.
			 * @param value `any` containing value of the message. */
			template<message_scope Scope = message_scope::GLOBAL>
			void send(any data) const
			{
				generic_message_queue<Scope>::send(m_type, std::move(data));
			}
			/** Dispatches the message queue for the bound type.
			 * @tparam Scope Scope of the target message queue. */
			template<message_scope Scope = message_scope::GLOBAL>
			void dispatch() const
			{
				generic_message_queue<Scope>::dispatch(m_type);
			}

		private:
			type_info m_type;
		};

		/** Creates an instance of `message_type` attribute for type `T`. */
		template<typename T>
		constexpr static message_type make_message_type{type_selector<T>};
	}	 // namespace attributes
}	 // namespace sek::engine