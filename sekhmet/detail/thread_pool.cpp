//
// Created by switchblade on 2021-12-24.
//

#include "thread_pool.hpp"

#include "../math/detail/util.hpp"
#include "assert.hpp"

namespace sek
{
	thread_pool::thread_pool(std::size_t worker_count)
	{
		/* Spawn worker threads. */
		{
			auto n = math::max<std::size_t>(1, worker_count);
			workers.reserve(n);
			while (n-- > 0) workers.emplace_back(worker{this});
		}

		/* Tell worker threads that they are allowed to run now. */
		set_state(pool_state::RUN);
	}
	thread_pool::~thread_pool()
	{
		/* Stop worker threads & wait for them to terminate. */
		set_state(pool_state::TERMINATE);
		for (auto &worker_thread : workers) worker_thread.join();
	}

	void thread_pool::set_state(thread_pool::pool_state new_state)
	{
		std::unique_lock<std::mutex> l(pool_mtx);

		state = new_state;
		worker_cv.notify_all(); /* Notify all workers to let them know of a
								   state change. */
	}

	const thread_pool::work_node *thread_pool::wait_for_work_or_exit()
	{
		std::unique_lock<std::mutex> wait_lock(pool_mtx);

		const thread_pool::work_node *node = nullptr;
		worker_cv.wait(wait_lock,
					   [&, this]() noexcept -> bool
					   {
						   /* If the state is TERMINATE, return true without setting any work. */
						   if (state == pool_state::TERMINATE) return true;

						   /* Wait until we are allowed to take on work and
							* there is any work available. */
						   if (state == pool_state::RUN && work_queue != nullptr)
						   {
							   node = pop_work();
							   return true;
						   }

						   return false;
					   });
		return node;
	}

	void thread_pool::worker::operator()() const
	{
		for (;;)
		{
			/* Dispatch the work. */
			auto work_node = parent->wait_for_work_or_exit();
			if (!work_node) break;

			try
			{
				work_node->invoke();
			}
			catch (...)
			{
				/* If there are any exceptions, push them to the parent pool. */
				std::unique_lock<std::mutex> l(parent->pool_mtx);

				// NOLINTNEXTLINE(performance-unnecessary-value-param)
				parent->work_exceptions.emplace_back(std::current_exception(), std::this_thread::get_id());
			}

			/* Always notify other threads, even if there was an exception,
			 * since we need to keep dispatching until the queue is empty. */
			std::unique_lock<std::mutex> l(parent->pool_mtx);
			if (parent->work_queue) parent->worker_cv.notify_all();
		}
	}
}	 // namespace sek