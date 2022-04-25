//
// Created by switchblade on 2021-12-24.
//

#include "thread_pool.hpp"

#include "assert.hpp"

namespace sek
{
	static void adjust_worker_count(std::size_t &n)
	{
		if (!n && !(n = std::thread::hardware_concurrency())) [[unlikely]]
			throw std::runtime_error("`std::thread::hardware_concurrency` returned 0");
	}

	thread_pool::thread_pool(std::pmr::memory_resource *res, std::size_t n, thread_pool::queue_mode mode)
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
	thread_pool::~thread_pool()
	{
		/* Destroy & terminate workers, then destroy unfinished tasks. */
		destroy_workers(workers_data, workers_data + size());
		allocator()->deallocate(workers_data, workers_capacity * sizeof(worker), alignof(worker));

		for (auto task = queue_head.front, end = queue_head.back; task != end;)
			delete_task(*static_cast<task_base *>(std::exchange(task, task->next)));	// NOLINT
	}

	void thread_pool::resize(std::size_t n)
	{
		adjust_worker_count(n);

		if (n == size()) [[unlikely]]
			return;
		else if (n < size()) /* Terminate size - n threads. */
			destroy_workers(workers_data + n, workers_data + size());
		else /* Start n - size threads. */
		{
			if (n > workers_capacity) [[unlikely]]
				realloc_workers(n);
			for (auto worker = workers_data + size(), end = workers_data + n; worker != end; ++worker)
				std::construct_at(worker, this);
		}
		workers_count = n;
	}

	void thread_pool::worker::thread_main(std::stop_token st, thread_pool *p) noexcept
	{
		for (;;)
		{
			try
			{
				/* Wait for termination or available tasks. */
				std::unique_lock<std::mutex> lock(p->mtx);
				p->cv.wait(lock, [&]() { return st.stop_requested() || p->queue_head.next != &p->queue_head; });

				/* Break out of the loop if termination was requested. */
				if (st.stop_requested()) [[unlikely]]
					break;
			}
			catch (std::system_error &e) /* Mutex error. */
			{
				/* TODO: Log exception */
			}

			/* Get task from the parent's queue, execute it, then delete it. */
			p->delete_task(p->pop_task().invoke());
		}
	}
}	 // namespace sek