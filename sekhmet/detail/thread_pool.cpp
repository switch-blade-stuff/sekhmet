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
		: task_alloc(std::pmr::pool_options{0, max_pool_block}, res), dispatch_mode(mode)
	{
		adjust_worker_count(n);

		/* Initialize n workers. */
		realloc_workers(n);
		for (auto begin = workers_data, end = workers_data + n; begin != end; ++begin) std::construct_at(begin, this);
		workers_count = n;
	}
	thread_pool::~thread_pool()
	{
		auto alloc = allocator();

		// clang-format off
		destroy_workers(workers_data, workers_data + size());
		for (auto task = queue_begin, end = queue_end; task != end; ++task)
			delete_task(**task);
		// clang-format on

		alloc->deallocate(workers_data, workers_capacity * sizeof(std::jthread), alignof(std::jthread));
		alloc->deallocate(queue_data, queue_capacity * sizeof(task_base *), alignof(task_base *));
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
	void thread_pool::expand_queue()
	{
		if (auto alloc = allocator(); !queue_data) [[unlikely]]
		{
			queue_end = queue_begin = queue_data =
				static_cast<task_base **>(alloc->allocate((queue_capacity = 8) * sizeof(task_base *), alignof(task_base *)));
			if (!queue_data) [[unlikely]]
				throw std::bad_alloc();
		}
		else
		{
			auto new_capacity = queue_capacity * 2;
			auto new_data = alloc->allocate(new_capacity * sizeof(task_base *), alignof(task_base *));
			if (!new_data) [[unlikely]]
				throw std::bad_alloc();

			auto old_data = queue_data;
			auto old_begin = queue_begin;

			queue_data = static_cast<task_base **>(new_data);
			queue_begin = queue_data + (old_begin - old_data);
			queue_end = std::copy(old_begin, queue_end, queue_begin);

			alloc->deallocate(old_data, queue_capacity * sizeof(task_base *), alignof(task_base *));
			queue_capacity = new_capacity;
		}
	}

	void thread_pool::worker::thread_main(std::stop_token st, thread_pool *p) noexcept
	{
		for (;;)
		{
			try
			{
				/* Wait for termination or available tasks. */
				std::unique_lock<std::mutex> lock(p->mtx);
				p->cv.wait(lock, [&]() { return st.stop_requested() || p->queue_end != p->queue_begin; });

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