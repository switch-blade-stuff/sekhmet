//
// Created by switchblade on 2021-12-24.
//

#include "thread_pool.hpp"

#include "assert.hpp"
#include "logger.hpp"

namespace sek
{
	static void adjust_worker_count(std::size_t &n)
	{
		if (!n && !(n = std::thread::hardware_concurrency())) [[unlikely]]
			throw std::runtime_error("`std::thread::hardware_concurrency` returned 0");
	}

	thread_pool::control_block::control_block(std::pmr::memory_resource *res, std::size_t n, thread_pool::queue_mode mode)
		: task_alloc(res), dispatch_mode(mode)
	{
		adjust_worker_count(n);

		/* Initialize n workers. */
		realloc_workers(n);
		// clang-format off
		for (auto begin = workers_data, end = workers_data + n; begin != end; ++begin)
			std::construct_at(begin, this);
		// clang-format on
		workers_count = n;
	}
	thread_pool::control_block::~control_block()
	{
		/* Workers should be terminated by now, no need to destroy them again. */
		task_alloc.upstream_resource()->deallocate(workers_data, workers_capacity * sizeof(worker_t), alignof(worker_t));
		for (auto task = queue_head.front, end = queue_head.back; task != end;)
			delete_task(static_cast<task_base *>(std::exchange(task, task->next)));	   // NOLINT
	}

	void thread_pool::control_block::resize(std::size_t n)
	{
		adjust_worker_count(n);

		if (n == workers_count) [[unlikely]]
			return;
		else if (n < workers_count) /* Terminate size - n threads. */
			destroy_workers(workers_data + n, workers_data + workers_count);
		else /* Start n - size threads. */
		{
			if (n > workers_capacity) [[unlikely]]
				realloc_workers(n);
			for (auto worker = workers_data + workers_count, end = workers_data + n; worker != end; ++worker)
				std::construct_at(worker, this);
		}
		workers_count = n;
	}
	void thread_pool::control_block::terminate()
	{
		destroy_workers(workers_data, workers_data + workers_count);
	}

	void thread_pool::worker_t::thread_main(std::stop_token st, control_block *cb) noexcept
	{
		cb->acquire();
		for (;;)
		{
			task_base *task;
			try
			{
				/* Wait for termination or available tasks. */
				std::unique_lock<std::mutex> lock(cb->mtx);
				cb->cv.wait(lock, [&]() { return st.stop_requested() || cb->queue_head.next != &cb->queue_head; });

				/* Break out of the loop if termination was requested, otherwise get the next task. */
				if (st.stop_requested()) [[unlikely]]
					break;
				else
					task = cb->pop_task();
			}
			catch (std::system_error &e) /* Mutex error. */
			{
				logger::error() << "Mutex error in worker thread: " << e.what();
			}

			/* Execute & delete the task. */
			cb->delete_task(task->invoke());
		}
		cb->release();
	}
}	 // namespace sek