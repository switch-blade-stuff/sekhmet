/*
 * Created by switchblade on 09/08/22
 */

#include "profiler.hpp"

#ifndef SEK_NO_PROFILER

namespace sek
{
	template class SEK_API_EXPORT service<access_guard<profiler, std::recursive_mutex>>;

	profiler::group &profiler::group::current()
	{
		thread_local group instance;
		return instance;
	}

	/* TODO: Implement network-based profiler clients.
	 * Candidate backends:
	 *  - Networking TS         - cons: not standard yet.
	 *  - ASIO                  - cons: heavy. */
}	 // namespace sek

#endif