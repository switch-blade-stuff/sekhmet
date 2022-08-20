/*
 * Created by switchblade on 25/05/22
 */

#pragma once

#include <mutex>

#include "sekhmet/access_guard.hpp"
#include "sekhmet/dense_set.hpp"
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
		struct queue_sync;
		template<>
		struct queue_sync<message_scope::GLOBAL>
		{
			std::mutex mtx;
		};
		template<>
		struct queue_sync<message_scope::THREAD>
		{
		};

		template<typename T, message_scope S>
		struct queue_data final : queue_data<void, S>
		{
			constexpr queue_data() noexcept { this->type = type_info::get<T>(); }
			~queue_data() final = default;

			event<bool(const T &)> receive_event;
			event<bool(const T &)> send_event;

		private:
			void dispatch_receive(const any &value) final { receive_event.dispatch(value.template cast<const T &>()); }
			bool dispatch_send(const any &value) final
			{
				bool result = false;
				send_event.dispatch([&result](bool b) { return result = b; }, value.template cast<const T &>());
				return result;
			}
		};
		template<message_scope S>
		struct queue_data<void, S> : queue_sync<S>
		{
			virtual ~queue_data() = default;

			void dispatch()
			{
				for (const auto &m : m_data) dispatch_receive(m);
				m_data.clear();
			}
			void queue(any &&value)
			{
				if (dispatch_send(value)) [[likely]]
					m_data.emplace_back(std::move(value));
			}
			void send(any &&value)
			{
				if (dispatch_send(value)) [[likely]]
					dispatch_receive(value);
			}

			type_info type;
			event<bool(any_ref)> receive_event;
			event<bool(any_ref)> send_event;

		protected:
			virtual void dispatch_receive(const any &) = 0;
			virtual bool dispatch_send(const any &) = 0;

			std::vector<any> m_data;
		};

		template<message_scope S>
		struct message_table_base
		{
			using data_type = queue_data<void, S>;
			using factory_t = data_type *(*) ();

			using entry_type = std::pair<factory_t, data_type *>;

			struct data_hash
			{
				typedef std::true_type is_transparent;

				hash_t operator()(const entry_type &data) const noexcept { return hash(data.second->type); }
				hash_t operator()(std::string_view key) const noexcept { return fnv1a(key.data(), key.size()); }
			};
			struct data_cmp
			{
				typedef std::true_type is_transparent;

				bool operator()(const entry_type &a, const entry_type &b) const noexcept { return a == b; }
				bool operator()(const entry_type &a, std::string_view b) const noexcept
				{
					return a.second->type.name() == b;
				}
				bool operator()(std::string_view a, const entry_type &b) const noexcept
				{
					return a == b.second->type.name();
				}
			};

			[[nodiscard]] inline data_type *find(std::string_view type) const noexcept;

			inline data_type &try_insert(std::string_view, data_type *(*) ());
			inline void erase(std::string_view, data_type *(*) ());
			inline void dispatch_all() const;

			dense_set<entry_type, data_hash, data_cmp> m_table;
		};

		template<message_scope S>
		class message_table;
		template<>
		class SEK_API message_table<message_scope::THREAD> : message_table_base<message_scope::THREAD>
		{
			using base_t = message_table_base<message_scope::THREAD>;
			using data_type = typename base_t::data_type;

		public:
			static message_table &instance();

		public:
			[[nodiscard]] data_type *find(std::string_view type) const noexcept;

			data_type &try_insert(std::string_view, data_type *(*) ());
			void erase(std::string_view, data_type *(*) ());
			void dispatch_all() const;
		};
		template<>
		class SEK_API message_table<message_scope::GLOBAL> : message_table_base<message_scope::GLOBAL>
		{
			using base_t = message_table_base<message_scope::GLOBAL>;
			using data_type = typename base_t::data_type;

		public:
			static message_table &instance();

		public:
			[[nodiscard]] data_type *find(std::string_view type) const;

			data_type &try_insert(std::string_view, data_type *(*) ());
			void erase(std::string_view, data_type *(*) ());
			void dispatch_all() const;

		private:
			mutable std::shared_mutex m_mtx;
		};

		template<message_scope S>
		typename message_table_base<S>::data_type *message_table_base<S>::find(std::string_view type) const noexcept
		{
			if (const auto pos = m_table.find(type); pos != m_table.end()) [[likely]]
				return pos->second;
			return nullptr;
		}
		template<message_scope S>
		typename message_table_base<S>::data_type &message_table_base<S>::try_insert(std::string_view t, data_type *(*f)())
		{
			auto pos = m_table.find(t);
			if (pos == m_table.end()) [[unlikely]]
				pos = m_table.emplace(f, f()).first;
			return *pos->second;
		}
		template<message_scope S>
		void message_table_base<S>::erase(std::string_view type, data_type *(*factory)())
		{
			const auto pos = m_table.find(type);
			if (pos != m_table.end() && pos->first == factory) [[likely]]
				m_table.erase(pos);
		}
		template<message_scope S>
		void message_table_base<S>::dispatch_all() const
		{
			for (auto &e : m_table) e.second->dispatch();
		}

		extern template class SEK_API_IMPORT message_table<message_scope::THREAD>;
		extern template class SEK_API_IMPORT message_table<message_scope::GLOBAL>;
	}	 // namespace detail

	/** @brief Generic message queue used to queue & dispatch messages in a type-erased way.
	 * @tparam Scope Target scope of the message queue.
	 * @note Queues of different scopes are separate from each other. */
	template<message_scope Scope = message_scope::ALL>
	class generic_message_queue
	{
		using table_type = detail::message_table<Scope>;

	public:
		/** Queues a message for later dispatch.
		 * @tparam type Type of the message to be queued.
		 * @param value `any` containing value of the message. */
		static void queue(std::string_view type, any value)
		{
			auto *queue = detail::message_table<Scope>::instance().find(type);
			if (queue != nullptr) [[likely]]
				queue->queue(std::forward<any>(value));
		}
		/** @copydoc send */
		static void queue(type_info type, any value) { queue(type.name(), std::move(value)); }
		/** Sends a message immediately, bypassing the queue.
		 * @tparam type Type of the message to be sent.
		 * @param value `any` containing value of the message. */
		static void send(std::string_view type, any value)
		{
			auto *queue = detail::message_table<Scope>::instance().find(type);
			if (queue != nullptr) [[likely]]
				queue->send(std::forward<any>(value));
		}
		/** @copydoc send */
		static void send(type_info type, any value) { send(type.name(), std::move(value)); }

		/** Dispatches queued messages of the specified type. */
		static void dispatch(std::string_view type)
		{
			auto *queue = detail::message_table<Scope>::instance().find(type);
			if (queue != nullptr) [[likely]]
				queue->dispatch();
		}
		/** @copydoc send */
		static void dispatch(type_info type) { dispatch(type.name()); }
		/** Dispatches queued messages of all types. */
		static void dispatch() { detail::message_table<Scope>::instance().dispatch_all(); }
	};

	template<>
	void generic_message_queue<message_scope::ALL>::dispatch(std::string_view type)
	{
		generic_message_queue<message_scope::GLOBAL>::dispatch(type);
		generic_message_queue<message_scope::THREAD>::dispatch(type);
	}
	template<>
	void generic_message_queue<message_scope::ALL>::dispatch()
	{
		generic_message_queue<message_scope::GLOBAL>::dispatch();
		generic_message_queue<message_scope::THREAD>::dispatch();
	}
	template<>
	void generic_message_queue<message_scope::ALL>::queue(std::string_view type, any value)
	{
		generic_message_queue<message_scope::GLOBAL>::queue(type, value);
		generic_message_queue<message_scope::THREAD>::queue(type, std::move(value));
	}
	template<>
	void generic_message_queue<message_scope::ALL>::send(std::string_view type, any value)
	{
		generic_message_queue<message_scope::GLOBAL>::send(type, value);
		generic_message_queue<message_scope::THREAD>::send(type, std::move(value));
	}

	/** @brief Type-specific message queue used to queue & dispatch messages.
	 * @tparam T Message type handled by the message queue.
	 * @tparam Scope Target scope of the message queue.
	 * @note Queues of different scopes are separate from each other. */
	template<typename T, message_scope Scope = message_scope::ALL>
	class message_queue
	{
	public:
		/** Queues a message for later dispatch.
		 * @param args Arguments passed to the constructor of the message.
		 * @note Passed arguments are used twice - once for the global message and once for the thread-local message. */
		template<typename... Args>
		inline static void queue(Args &&...args)
		{
			queue_impl(std::forward<Args>(args)...);
		}
		/** Queues a message for later dispatch.
		 * @param value Value of the message to be sent. */
		inline static void queue(const T &value) { queue_impl(value); }
		/** @copydoc send */
		inline static void queue(T &&value) { queue_impl(std::forward<T>(value)); }

		/** Sends a message immediately, bypassing the queue.
		 * @param args Arguments passed to the constructor of the message.
		 * @note Passed arguments are used twice - once for the global message and once for the thread-local message. */
		template<typename... Args>
		inline static void send(Args &&...args)
		{
			send_impl(std::forward<Args>(args)...);
		}
		/** Sends a message immediately, bypassing the queue.
		 * @param value Value of the message to be sent. */
		inline static void send(const T &value) { send_impl(value); }
		/** @copydoc send */
		inline static void send(T &&value) { send_impl(std::forward<T>(value)); }

		/** Dispatches all queued messages. */
		inline static void dispatch()
		{
			message_queue<T, message_scope::GLOBAL>::dispatch();
			message_queue<T, message_scope::THREAD>::dispatch();
		}

	private:
		template<typename... Args>
		inline static void queue_impl(Args &&...args)
		{
			message_queue<T, message_scope::GLOBAL>::queue(std::forward<Args>(args)...);
			message_queue<T, message_scope::THREAD>::queue(std::forward<Args>(args)...);
		}
		template<typename... Args>
		inline static void send_impl(Args &&...args)
		{
			message_queue<T, message_scope::GLOBAL>::send(std::forward<Args>(args)...);
			message_queue<T, message_scope::THREAD>::send(std::forward<Args>(args)...);
		}
	};

	/** @brief Global specialization of `message_queue`. Global queues are synchronized via an internal mutex. */
	template<typename T>
	class message_queue<T, message_scope::GLOBAL>
	{
	public:
		typedef event<bool(const T &)> event_type;

	public:
		/** Queues a message for later dispatch.
		 * @param args Arguments passed to the constructor of the message. */
		template<typename... Args>
		static void queue(Args &&...args)
		{
			queue_impl(std::forward<Args>(args)...);
		}
		/** Queues a message for later dispatch.
		 * @param value Value of the message to be sent. */
		static void queue(const T &value) { queue_impl(value); }
		/** @copydoc send */
		static void queue(T &&value) { queue_impl(std::forward<T>(value)); }

		/** Sends a message immediately, bypassing the queue.
		 * @param args Arguments passed to the constructor of the message. */
		template<typename... Args>
		static void send(Args &&...args)
		{
			send_impl(std::forward<Args>(args)...);
		}
		/** Sends a message immediately, bypassing the queue.
		 * @param value Value of the message to be sent. */
		static void send(const T &value) { send_impl(value); }
		/** @copydoc send */
		static void send(T &&value) { send_impl(std::forward<T>(value)); }

		/** Dispatches all queued messages. */
		static void dispatch()
		{
			auto &data = global_data();
			std::unique_lock<std::mutex> l(data.mtx);
			data.dispatch();
		}

		/** @brief Returns proxy for the receive event.
		 *
		 * Receive event is invoked when a message is sent or dispatched and is used to listen for message data.
		 * Event subscribers can return `false` to prematurely terminate message dispatching.
		 *
		 * @return Access guard to the event proxy. */
		static ref_guard<event_proxy<event_type>, std::mutex> on_receive()
		{
			auto &data = global_data();
			return {event_proxy<event_type>{data.receive_event}, mutex_ref{data.mtx}};
		}
		/** @brief Returns proxy for the send event.
		 *
		 * Send event is invoked when a message is sent or queued and is used to filter message data. Event subscribers
		 * can return `false` to prematurely terminate message sending (terminated message will not be dispatched to
		 * the receive event).
		 *
		 * @return Access guard to the event proxy. */
		static ref_guard<event_proxy<event_type>, std::mutex> on_send()
		{
			auto &data = global_data();
			return {event_proxy<event_type>{data.send_event}, mutex_ref{data.mtx}};
		}

	private:
		static detail::queue_data<void, message_scope::GLOBAL> *local_data()
		{
			static detail::queue_data<T, message_scope::GLOBAL> data;
			return &data;
		}
		static detail::queue_data<T, message_scope::GLOBAL> &global_data()
		{
			auto &ref = detail::message_table<message_scope::GLOBAL>::instance().try_insert(type_name<T>(), &local_data);
			return static_cast<detail::queue_data<T, message_scope::GLOBAL> &>(ref);
		}

		template<typename... Args>
		static void queue_impl(Args &&...args)
		{
			auto &data = global_data();
			std::unique_lock<std::mutex> l(data.mtx);
			data.queue(make_any<T>(std::forward<Args>(args)...));
		}
		template<typename... Args>
		static void send_impl(Args &&...args)
		{
			auto &data = global_data();
			std::unique_lock<std::mutex> l(data.mtx);
			data.send(make_any<T>(std::forward<Args>(args)...));
		}
	};
	/** @brief Thread-local specialization of `message_queue`. Thread-local queues are not synchronized. */
	template<typename T>
	class message_queue<T, message_scope::THREAD>
	{
	public:
		typedef event<bool(const T &)> event_type;

	public:
		/** Queues a message for later dispatch.
		 * @param args Arguments passed to the constructor of the message. */
		template<typename... Args>
		static void queue(Args &&...args)
		{
			queue_impl(std::forward<Args>(args)...);
		}
		/** Queues a message for later dispatch.
		 * @param value Value of the message to be sent. */
		static void queue(const T &value) { queue_impl(value); }
		/** @copydoc send */
		static void queue(T &&value) { queue_impl(std::forward<T>(value)); }

		/** Sends a message immediately, bypassing the queue.
		 * @param args Arguments passed to the constructor of the message. */
		template<typename... Args>
		static void send(Args &&...args)
		{
			send_impl(std::forward<Args>(args)...);
		}
		/** Sends a message immediately, bypassing the queue.
		 * @param value Value of the message to be sent. */
		static void send(const T &value) { send_impl(value); }
		/** @copydoc send */
		static void send(T &&value) { send_impl(std::forward<T>(value)); }

		/** Dispatches all queued messages. */
		static void dispatch() { global_data().dispatch(); }

		/** @brief Returns proxy for the receive event.
		 *
		 * Receive event is invoked when a message is sent or dispatched and is used to listen for message data.
		 * Event subscribers can return `false` to prematurely terminate message dispatching.
		 *
		 * @return Event proxy for the receive event. */
		static event_proxy<event_type> on_receive() { return {global_data().receive_event}; }
		/** @brief Returns proxy for the send event.
		 *
		 * Send event is invoked when a message is sent or queued and is used to filter message data. Event subscribers
		 * can return `false` to prematurely terminate message sending (terminated message will not be dispatched to
		 * the receive event).
		 *
		 * @return Event proxy for the send event. */
		static event_proxy<event_type> on_send() { return {global_data().send_event}; }

	private:
		static detail::queue_data<void, message_scope::THREAD> *local_data()
		{
			thread_local detail::queue_data<T, message_scope::THREAD> data;
			return &data;
		}
		static detail::queue_data<T, message_scope::THREAD> &global_data()
		{
			auto &ref = detail::message_table<message_scope::THREAD>::instance().try_insert(type_name<T>(), &local_data);
			return static_cast<detail::queue_data<T, message_scope::THREAD> &>(ref);
		}

		template<typename... Args>
		static void queue_impl(Args &&...args)
		{
			global_data().queue(make_any<T>(std::forward<Args>(args)...));
		}
		template<typename... Args>
		static void send_impl(Args &&...args)
		{
			global_data().send(make_any<T>(std::forward<Args>(args)...));
		}
	};
}	 // namespace sek::engine