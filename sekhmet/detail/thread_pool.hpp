/*
 * Created by switchblade on 2021-12-23
 */

#pragma once

#include <future>
#include <mutex>
#include <thread>

#include "define.h"
#include "ebo_base_helper.hpp"
#include <condition_variable>

namespace sek
{
	/** @brief Structure used to manage multiple worker threads.
	 *
	 * Thread pools provide high-level way to schedule & execute asynchronous tasks.
	 * Thread pools manage a set of threads which wait for some work to become available.
	 * Worker threads exist as long as the pool exists. */
	class thread_pool
	{
	public:
		typedef int queue_mode;
		constexpr static queue_mode fifo = 0;
		constexpr static queue_mode filo = 1;

	private:
		struct task_node
		{
			constexpr void link_after(task_node &prev) noexcept
			{
				previous = &prev;
				next = prev.next;
				prev.next->previous = this;
				prev.next = this;
			}
			constexpr task_node *unlink() noexcept
			{
				if (next) next->previous = previous;
				if (previous) previous->next = next;
				return this;
			}

			union
			{
				task_node *front = nullptr;
				task_node *next;
			};
			union
			{
				task_node *back = nullptr;
				task_node *previous;
			};
		};
		struct task_base : task_node
		{
			virtual ~task_base() = default;
			virtual task_base *invoke() noexcept = 0;
		};
		template<typename T, typename F, typename U = std::decay_t<F>>
		struct task_t final : task_base, ebo_base_helper<F>
		{
			using ebo_t = ebo_base_helper<F>;

			task_t(std::promise<T> &&p, F &&f) : ebo_t(std::forward<F>(f)), promise(std::forward<std::promise<T>>(p)) {}
			~task_t() final = default;

			task_base *invoke() noexcept final
			{
				try
				{
					auto &f = *ebo_t::get();
					if constexpr (std::is_void_v<T>)
					{
						f();
						promise.set_value();
					}
					else
						promise.set_value(f());
				}
				catch (...)
				{
					promise.set_exception(std::current_exception());
				}
				return this;
			}

			std::promise<T> promise;
		};

		struct control_block;

		/* Custom worker instead of std::jthread since jthread joins on destruction, and we need to detach. */
		struct worker_t
		{
			static void thread_main(std::stop_token, control_block *) noexcept;

			worker_t(worker_t &&other) noexcept : source(std::move(other.source)), thread(std::move(other.thread)) {}
			explicit worker_t(control_block *cb) : thread(thread_main, source.get_token(), cb) {}
			~worker_t()
			{
				/* Detach the thread to let the worker terminate on it's own. */
				if (source.stop_possible()) [[likely]]
				{
					source.request_stop();
					thread.detach();
				}
			}

			std::stop_source source;
			std::thread thread;
		};

		/* Worker threads may outlive the pool, thus the control block must live as long as any worker lives. */
		struct control_block
		{
			SEK_API control_block(std::size_t n, queue_mode mode);
			SEK_API ~control_block();

			SEK_API void resize(std::size_t n);
			SEK_API void terminate();
			SEK_API void acquire();
			SEK_API void release();

			template<typename T, typename F>
			std::future<T> schedule(std::promise<T> &&promise, F &&task)
			{
				std::future<T> result;
				{
					std::lock_guard<std::mutex> l(mtx);

					auto node = new task_t<T, F>(std::forward<std::promise<T>>(promise), std::forward<F>(task));
					result = node->promise.get_future();
					node->link_after(queue_head);
				}
				cv.notify_one();
				return result;
			}

			void destroy_workers(worker_t *first, worker_t *last);
			void realloc_workers(std::size_t n);

			task_base *pop_task() noexcept
			{
				// NOLINTNEXTLINE static cast is fine here since there is no virtual inheritance
				return static_cast<task_base *>((dispatch_mode == fifo ? queue_head.back : queue_head.front)->unlink());
			}

			std::atomic<std::size_t> ref_count = 1;

			std::condition_variable cv;
			std::mutex mtx;

			worker_t *workers_data = nullptr;
			std::size_t workers_capacity = 0;
			std::size_t workers_count = 0;

			task_node queue_head = {.front = &queue_head, .back = &queue_head};
			queue_mode dispatch_mode;
		};

	public:
		thread_pool(const thread_pool &) = delete;
		thread_pool &operator=(const thread_pool &) = delete;
		thread_pool(thread_pool &&) = delete;
		thread_pool &operator=(thread_pool &&) = delete;

		/** Initializes thread pool with `std::thread::hardware_concurrency` workers & FIFO queue mode. */
		thread_pool() : thread_pool(0, fifo) {}
		/** Initializes thread pool with n threads & the specified queue mode.
		 * @param n Amount of worker threads to initialize the pool with. If set to 0 will use `std::thread::hardware_concurrency` workers.
		 * @param mode Queue mode used for task dispatch. Default is FIFO. */
		explicit thread_pool(std::size_t n, queue_mode mode = fifo) : m_cb(new control_block(n, mode)) {}
		/** Terminates all worker threads & releases internal state. */
		~thread_pool()
		{
			m_cb->terminate();
			m_cb->release();
		}

		/** Returns the current queue dispatch mode of the pool. */
		[[nodiscard]] constexpr queue_mode mode() const noexcept { return m_cb->dispatch_mode; }
		/** Sets pool's queue dispatch mode. */
		constexpr void mode(queue_mode mode) noexcept { m_cb->dispatch_mode = mode; }

		/** Returns the current amount of worker threads in the pool. */
		[[nodiscard]] constexpr std::size_t size() const noexcept { return m_cb->workers_count; }
		/** Resizes the pool to n workers. If n is set to 0, uses `std::thread::hardware_concurrency` workers. */
		void resize(std::size_t n) { m_cb->resize(n); }

		/** Schedules a task to be executed by one of the worker threads.
		 * Tasks are dispatched according to the current queue mode.
		 * @param task Functor to execute on one of the worker threads.
		 * @return `std::future` used to retrieve task result or exceptions.
		 * @note Task functor must be invocable with 0 arguments. */
		template<std::invocable F>
		std::future<std::invoke_result_t<F>> schedule(F &&task)
		{
			return schedule(std::promise<std::invoke_result_t<F>>{}, std::forward<F>(task));
		}
		/** @copydoc schedule
		 * @param promise Promise used to store task's result & exceptions.
		 * @note Task's return type must be implicitly convertible to the promised type. */
		template<typename T, std::invocable F>
		std::future<T> schedule(std::promise<T> &&promise, F &&task)
		{
			return m_cb->schedule(std::forward<std::promise<T>>(promise), std::forward<F>(task));
		}

	private:
		control_block *m_cb;
	};
}	 // namespace sek