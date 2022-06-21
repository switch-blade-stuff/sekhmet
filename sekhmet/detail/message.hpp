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
 * Created by switchblade on 25/05/22
 */

#pragma once

#include <mutex>

#include "event.hpp"
#include "type_info.hpp"

namespace sek
{
	enum class message_scope : int
	{
		/** Messages are dispatched through the global synchronized message queue. */
		GLOBAL,
		/** Messages are dispatched through the thread-local message queue. */
		THREAD,
	};

	namespace detail
	{
		template<message_scope>
		struct message_sync
		{
			std::unique_lock<std::mutex> make_lock() { return std::unique_lock<std::mutex>{mtx}; }

			std::mutex mtx;
		};
		template<>
		struct message_sync<message_scope::THREAD>
		{
		};

		template<typename T, message_scope Scope>
		struct queue_data : message_sync<Scope>
		{
			static queue_data &instance()
			{
				if constexpr (Scope == message_scope::GLOBAL)
				{
					static queue_data value;
					return value;
				}
				else
				{
					static thread_local queue_data value;
					return value;
				}
			}

			void receive(const T &value) const
			{
				receive_event([](auto b) { return b; }, value);
			}
			bool send(const T &value) const
			{
				bool result = false;
				send_event([&result](auto b) { return result = b; }, value);
				return result;
			}

			std::vector<T> messages;
			event<bool(const T &)> receive_event;
			event<bool(const T &)> send_event;
		};
	}	 // namespace detail

	/** @brief Message queue used to queue & dispatch messages type-specific messages.
	 * @tparam Scope Scope of the message queue (global/thread local).
	 * @note Queues of different scopes are separate from each other. */
	template<std::copyable T, message_scope Scope = message_scope::GLOBAL>
	class message_queue
	{
	public:
		typedef event<bool(const T &)> receive_event_type;
		typedef event<bool(const T &)> send_event_type;

		typedef std::unique_lock<std::mutex> lock_type;

	private:
		using queue_data = detail::queue_data<T, Scope>;

		static queue_data &instance() { return queue_data::instance(); }

	public:
		/** Queues a message for later dispatch.
		 * @param data Data of the message.
		 * @note Message data is copied by the queue. */
		static void queue(const T &data = T{});
		/** Dispatches all queued messages. */
		static void dispatch();
		/** Sends a message immediately, bypassing the queue.
		 * @param data Data of the message. */
		static void send(const T &data = T{});

		/** @brief Returns proxy for the receive event.
		 *
		 * Receive event is invoked when a message is sent or dispatched and is used to listen for message data.
		 *
		 * Event subscribers can return `false` to prematurely terminate message dispatching.
		 *
		 * @return Pair where first is the lock used to synchronize message queue, and second is the event proxy. */
		static std::pair<lock_type, event_proxy<receive_event_type>> on_receive();
		/** @brief Returns proxy for the send event.
		 *
		 * Send event is invoked when a message is sent or queued and can be used to filter the message data.
		 *
		 * Event subscribers can return `false` to prematurely terminate message sending
		 * (terminated message will not be dispatched to the receive event).
		 * @return Pair where first is the lock used to synchronize message queue, and second is the event proxy. */
		static std::pair<lock_type, event_proxy<send_event_type>> on_send();
	};

	template<std::copyable T, message_scope S>
	void message_queue<T, S>::queue(const T &data)
	{
		auto &inst = instance();
		auto l = inst.make_lock();
		if (inst.send(data)) [[likely]]
			inst.messages.emplace_back(data);
	}
	template<std::copyable T, message_scope S>
	void message_queue<T, S>::dispatch()
	{
		// clang-format off
		auto &inst = instance();
		auto l = inst.make_lock();
		for (auto &data : inst.messages)
			inst.receive(data);
		inst.messages.clear();
		// clang-format on
	}
	template<std::copyable T, message_scope S>
	void message_queue<T, S>::send(const T &data)
	{
		auto &inst = instance();
		auto l = inst.make_lock();
		if (inst.send(data)) [[likely]]
			inst.receive(data);
	}

	template<std::copyable T, message_scope S>
	std::pair<typename message_queue<T, S>::lock_type, event_proxy<typename message_queue<T, S>::receive_event_type>>
		message_queue<T, S>::on_receive()
	{
		auto &inst = instance();
		return {inst.make_lock(), inst.receive_event};
	}
	template<std::copyable T, message_scope S>
	std::pair<typename message_queue<T, S>::lock_type, event_proxy<typename message_queue<T, S>::send_event_type>>
		message_queue<T, S>::on_send()
	{
		auto &inst = instance();
		return {inst.make_lock(), inst.send_event};
	}

	/** @copydoc message_queue
	 * Thread-local message queue overload. */
	template<std::copyable T>
	class message_queue<T, message_scope::THREAD>
	{
	public:
		typedef event<bool(const T &)> receive_event_type;
		typedef event<bool(T &)> send_event_type;

	private:
		using queue_data = detail::queue_data<T, message_scope::THREAD>;

		static queue_data &instance() { return queue_data::instance(); }

	public:
		/** @copydoc message_queue::queue */
		static void queue(const T &data = T{});
		/** @copydoc message_queue::dispatch */
		static void dispatch();
		/** @copydoc message_queue::send */
		static void send(const T &data = T{});

		/** @brief Returns proxy for the receive event.
		 *
		 * Receive event is invoked when a message is sent or dispatched and is used to listen for message data.
		 *
		 * Event subscribers can return `false` to prematurely terminate message dispatching. */
		static event_proxy<receive_event_type> on_receive();
		/** @brief Returns proxy for the send event.
		 *
		 * Send event is invoked when a message is sent or queued and can be used to filter the message data.
		 *
		 * Event subscribers can return `false` to prematurely terminate message sending
		 * (terminated message will not be dispatched to the receive event). */
		static event_proxy<send_event_type> on_send();
	};

	template<std::copyable T>
	void message_queue<T, message_scope::THREAD>::queue(const T &data)
	{
		auto &inst = instance();
		if (inst.send(data)) [[likely]]
			inst.messages.emplace_back(data);
	}
	template<std::copyable T>
	void message_queue<T, message_scope::THREAD>::dispatch()
	{
		// clang-format off
		auto &inst = instance();
		for (auto &data : inst.messages)
			inst.receive(data);
		inst.messages.clear();
		// clang-format on
	}
	template<std::copyable T>
	void message_queue<T, message_scope::THREAD>::send(const T &data)
	{
		auto &inst = instance();
		if (inst.send(data)) [[likely]]
			inst.receive(data);
	}

	template<std::copyable T>
	event_proxy<typename message_queue<T, message_scope::THREAD>::receive_event_type>
		message_queue<T, message_scope::THREAD>::on_receive()
	{
		return instance().receive_event;
	}
	template<std::copyable T>
	event_proxy<typename message_queue<T, message_scope::THREAD>::send_event_type>
		message_queue<T, message_scope::THREAD>::on_send()
	{
		return instance().send_event;
	}

	namespace attributes
	{
		/** @brief Attribute used to send messages of a specific type at runtime in a type-agnostic way. */
		class message_type
		{
		private:
			struct vtable_t
			{
				template<typename, message_scope>
				constinit static const vtable_t instance;

				void (*queue)(any);
				void (*dispatch)();
				void (*send)(any);
			};

		public:
			message_type() = delete;

			template<typename T>
			constexpr explicit message_type(type_selector_t<T>) noexcept
				: m_global(&vtable_t::instance<T, message_scope::GLOBAL>),
				  m_thread(&vtable_t::instance<T, message_scope::THREAD>)
			{
			}

			constexpr message_type(const message_type &) noexcept = default;
			constexpr message_type &operator=(const message_type &) noexcept = default;
			constexpr message_type(message_type &&) noexcept = default;
			constexpr message_type &operator=(message_type &&) noexcept = default;

			/** Queues message using the bound message queue.
			 * @tparam Scope Scope of the message queue to queue the message on.
			 * @param data Data of the message. */
			template<message_scope Scope = message_scope::GLOBAL>
			void queue(any data) const
			{
				if constexpr (Scope == message_scope::GLOBAL)
					m_global->queue(std::move(data));
				else
					m_thread->queue(std::move(data));
			}
			/** Dispatches the bound message queue.
			 * @tparam Scope Scope of the message queue to dispatch. */
			template<message_scope Scope = message_scope::GLOBAL>
			void dispatch() const
			{
				if constexpr (Scope == message_scope::GLOBAL)
					m_global->dispatch();
				else
					m_thread->dispatch();
			}
			/** Sends message using the bound message queue.
			 * @tparam Scope Scope of the message queue to send the message on.
			 * @param data Data of the message. */
			template<message_scope Scope = message_scope::GLOBAL>
			void send(any data) const
			{
				if constexpr (Scope == message_scope::GLOBAL)
					m_global->send(std::move(data));
				else
					m_thread->send(std::move(data));
			}

		private:
			const vtable_t *m_global;
			const vtable_t *m_thread;
		};

		template<typename T, message_scope S>
		constinit const message_type::vtable_t message_type::vtable_t::instance = {
			.queue = +[](any a) { message_queue<T, S>::queue(a.cast<const T &>()); },
			.dispatch = +[]() { message_queue<T, S>::dispatch(); },
			.send = +[](any a) { message_queue<T, S>::send(a.cast<const T &>()); },
		};

		/** Creates an instance of message source attribute for type `T`. */
		template<typename T>
		constexpr static message_type make_message_type{type_selector<T>};
	}	 // namespace attributes
}	 // namespace sek