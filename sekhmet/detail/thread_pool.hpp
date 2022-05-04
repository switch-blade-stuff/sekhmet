//
// Created by switchblade on 2021-12-23.
//

#pragma once

#include <future>
#include <mutex>
#include <thread>

#include "aligned_storage.hpp"
#include "define.h"
#include <condition_variable>
#include <memory_resource>

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
			constexpr task_base *invoke() noexcept
			{
				invoke_func(this);
				return this;
			}
			constexpr void destroy(std::pmr::unsynchronized_pool_resource *pool) noexcept { destroy_func(this, pool); }

			void (*invoke_func)(void *) noexcept;
			void (*destroy_func)(void *, std::pmr::unsynchronized_pool_resource *) noexcept;

			union
			{
				type_storage<void *> local_data;
				void *heap_data;
			};
		};
		template<typename T, typename F, typename U = std::decay_t<F>>
		struct task_t : task_base
		{
			constexpr static bool in_place = sizeof(U) <= sizeof(void *);

			task_t(std::pmr::unsynchronized_pool_resource *pool, std::promise<T> &&promise, F &&f)
				: promise(std::forward<std::promise<T>>(promise))
			{
				if constexpr (in_place)
				{
					std::construct_at(local_data.template get<U>(), std::forward<F>(f));

					invoke_func = +[](void *ptr) noexcept
					{
						auto task = static_cast<task_t *>(ptr);
						task->do_invoke(*task->local_data.template get<U>());
					};
				}
				else
				{
					heap_data = pool->allocate(sizeof(U), alignof(U));
					std::construct_at(static_cast<U *>(heap_data), std::forward<F>(f));

					invoke_func = +[](void *ptr) noexcept
					{
						auto task = static_cast<task_t *>(ptr);
						task->do_invoke(*static_cast<U *>(task->heap_data));
					};
				}

				destroy_func = +[](void *ptr, std::pmr::unsynchronized_pool_resource *pool) noexcept
				{
					auto task = static_cast<task_t *>(ptr);
					task->do_destroy(pool);
				};
			}

			void do_invoke(F &f) noexcept
			{
				try
				{
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
			}
			void do_destroy(std::pmr::unsynchronized_pool_resource *pool)
			{
				if constexpr (in_place)
					std::destroy_at(local_data.template get<U>());
				else
				{
					std::destroy_at(static_cast<U *>(heap_data));
					pool->deallocate(heap_data, sizeof(U), alignof(U));
				}

				std::destroy_at(this);
				pool->deallocate(this, sizeof(task_t), alignof(task_t));
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
			static control_block *make_control_block(std::pmr::memory_resource *res, std::size_t n, thread_pool::queue_mode mode)
			{
				auto *cb = static_cast<control_block *>(res->allocate(sizeof(control_block), alignof(control_block)));
				if (!cb) [[unlikely]]
					throw std::bad_alloc();
				return std::construct_at(cb, res, n, mode);
			}

			SEK_API control_block(std::pmr::memory_resource *res, std::size_t n, queue_mode mode);
			SEK_API ~control_block();

			SEK_API void resize(std::size_t n);
			SEK_API void terminate();

			template<typename T, typename F>
			std::future<T> schedule(std::promise<T> &&promise, F &&task)
			{
				std::future<T> result;
				{
					std::lock_guard<std::mutex> l(mtx);

					using task_type = task_t<T, F>;
					auto task_ptr = static_cast<task_type *>(task_alloc.allocate(sizeof(task_type), alignof(task_type)));
					if (!task_ptr) [[unlikely]]
						throw std::bad_alloc();

					std::construct_at(task_ptr, &task_alloc, std::forward<std::promise<T>>(promise), std::forward<F>(task));
					task_ptr->link_after(queue_head);
					result = task_ptr->promise.get_future();
				}
				cv.notify_one();
				return result;
			}

			void realloc_workers(std::size_t n)
			{
				auto alloc = task_alloc.upstream_resource();

				auto new_workers = static_cast<worker_t *>(alloc->allocate(n * sizeof(worker_t), alignof(worker_t)));
				if (!new_workers) [[unlikely]]
					throw std::bad_alloc();

				/* Move old workers if there are any. */
				if (workers_data)
				{
					for (auto src = workers_data, end = workers_data + workers_count, dst = new_workers; src < end; ++src, ++dst)
					{
						std::construct_at(dst, std::move(*src));
						std::destroy_at(src);
					}
					alloc->deallocate(workers_data, workers_capacity * sizeof(worker_t), alignof(worker_t));
				}

				workers_data = new_workers;
				workers_capacity = n;
			}
			void destroy_workers(worker_t *first, worker_t *last)
			{
				std::destroy(first, last);
				cv.notify_all();
			}

			task_base *pop_task() noexcept
			{
				// NOLINTNEXTLINE static cast is fine here since there is no virtual inheritance
				return static_cast<task_base *>((dispatch_mode == fifo ? queue_head.back : queue_head.front)->unlink());
			}
			void delete_task(task_base *task) { task->destroy(&task_alloc); }

			void acquire() { ref_count++; }
			void release()
			{
				if (ref_count-- == 1) [[unlikely]]
				{
					std::destroy_at(this);
					task_alloc.upstream_resource()->deallocate(this, sizeof(control_block), alignof(control_block));
				}
			}

			std::pmr::unsynchronized_pool_resource task_alloc;
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
		explicit thread_pool(std::size_t n, queue_mode mode = fifo)
			: thread_pool(std::pmr::get_default_resource(), n, mode)
		{
		}
		/** @copydoc thread_pool
		 * @param res Memory resource used to allocate internal state. */
		thread_pool(std::pmr::memory_resource *res, std::size_t n, queue_mode mode = fifo)
			: cb(control_block::make_control_block(res, n, mode))
		{
		}
		/** Terminates all worker threads & releases internal state. */
		~thread_pool()
		{
			cb->terminate();
			cb->release();
		}

		/** Returns the current queue dispatch mode of the pool. */
		[[nodiscard]] constexpr queue_mode mode() const noexcept { return cb->dispatch_mode; }
		/** Sets pool's queue dispatch mode. */
		constexpr void mode(queue_mode mode) noexcept { cb->dispatch_mode = mode; }

		/** Returns the current amount of worker threads in the pool. */
		[[nodiscard]] constexpr std::size_t size() const noexcept { return cb->workers_count; }
		/** Resizes the pool to n workers. If n is set to 0, uses `std::thread::hardware_concurrency` workers. */
		void resize(std::size_t n) { cb->resize(n); }

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
			return cb->template schedule(std::forward<std::promise<T>>(promise), std::forward<F>(task));
		}

	private:
		control_block *cb;
	};
}	 // namespace sek