//
// Created by switchblade on 2021-12-23.
//

#pragma once

#include <functional>
#include <list>
#include <mutex>
#include <thread>
#include <vector>

#include "aligned_storage.hpp"
#include "define.h"
#include <condition_variable>

namespace sek
{
	/** @brief Structure used to manage multiple worker threads.
	 *
	 * Thread pools provide high-level way to schedule & execute asynchronous work.
	 * Thread pools manage a set of threads which wait for some work to become available.
	 * Worker threads exist as long as the pool exists. */
	class SEK_API thread_pool
	{
	private:
		enum class pool_state
		{
			/** Threads need to wait. */
			PAUSE,
			/** Threads are free to take on work. */
			RUN,
			/** Threads should terminate immediately. */
			TERMINATE,
		};

		struct worker
		{
			void operator()() const;

			thread_pool *parent;
		};

		struct work_node
		{
			template<typename T>
			using store_locally =
				std::conjunction<std::is_trivially_copyable<T>,
								 std::bool_constant<sizeof(T) <= sizeof(void *) && alignof(T) <= alignof(void *)>>;

			work_node(const work_node &) = delete;
			work_node(work_node &&) = delete;

			template<typename T>
			constexpr explicit work_node(work_node *next, T &&f) : next(next)
			{
				if constexpr (store_locally<T>::value)
				{
					std::construct_at(local_data.template get<T>(), std::forward<T>(f));
					invoke_func = [](const work_node *p)
					{
						T *obj = p->local_data.template get<T>();
						std::invoke(*obj);
					};
					destroy_func = [](const work_node *p)
					{
						T *obj = p->local_data.template get<T>();
						std::destroy_at(obj);
					};
				}
				else
				{
					heap_data = new T(std::forward<T>(f));
					invoke_func = [](const work_node *p) { std::invoke(*static_cast<T *>(p->heap_data)); };
					destroy_func = [](const work_node *p) { delete static_cast<T *>(p->heap_data); };
				}
			}
			constexpr ~work_node() { destroy_func(this); }

			constexpr void invoke() const { return invoke_func(this); }

			union
			{
				mutable aligned_storage<sizeof(void *), alignof(void *)> local_data;
				void *heap_data;
			};

			void (*invoke_func)(const work_node *);
			void (*destroy_func)(const work_node *);

			work_node *next = nullptr;
		};

	public:
		/** Creates a new pool with the specified worker count & dispatch policy.
		 * @param worker_count Amount of worker threads within this pool. Minimum is 1.
		 * @param policy Dispatch policy used by the pool. Default value is first in, last out. */
		explicit thread_pool(std::size_t worker_count);
		~thread_pool();

		/** Queues the provided functor to be dispatched by one of the worker threads. */
		template<typename F, typename... Args>
		void add_work(F &&f, Args &&...args)
		{
			std::unique_lock<std::mutex> l(pool_mtx);

			push_work([f = std::forward<F>(f), ... args = std::forward<Args>(args)]() { std::invoke(f, args...); });
			worker_cv.notify_all();
		}

		/** Pauses the pool. Disables dispatching of worker threads.
		 * All work that is currently in-progress will be finished. */
		void pause() { set_state(pool_state::PAUSE); }
		/** Resumes a paused pool. Enables dispatching of worker threads.
		 * If a pool is not paused, all waiting worker threads would be notified either way. */
		void resume() { set_state(pool_state::PAUSE); }

		/** Returns a vector of exceptions thrown by workers & clears current exception vector. */
		std::vector<std::pair<std::exception_ptr, std::thread::id>> get_work_exceptions()
		{
			std::unique_lock<std::mutex> l(pool_mtx);

			std::vector<std::pair<std::exception_ptr, std::thread::id>> result = {};
			result.swap(work_exceptions);
			return result;
		}

	private:
		template<typename F>
		constexpr void push_work(F &&f)
		{
			work_queue = new work_node{work_queue, std::forward<F>(f)};
		}
		constexpr const work_node *pop_work() noexcept
		{
			auto node = work_queue;
			work_queue = work_queue->next;
			return node;
		}

		void set_state(pool_state new_state);
		const work_node *wait_for_work_or_exit();

		/** Mutex used to synchronise thread operations. */
		mutable std::mutex pool_mtx;
		/** Conditional variable used by the worker threads to wait for work to be available. */
		std::condition_variable worker_cv;

		/** Current state of the pool. */
		pool_state state = pool_state::PAUSE;

		/** Worker threads used by this pool. */
		std::vector<std::thread> workers;
		/** Buffer used to store work for the worker threads. */
		work_node *work_queue = nullptr;
		/** Vector containing exceptions generated by the workers. */
		std::vector<std::pair<std::exception_ptr, std::thread::id>> work_exceptions;
	};
}	 // namespace sek