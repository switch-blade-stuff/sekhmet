/*
 * Created by switchblade on 18/08/22
 */

#pragma once

#include "../service.hpp"

#ifndef SEK_NO_PROFILER

namespace sek::engine
{
	/** Profiler spec.
	 *
	 * Internally, profiler operates through the debug server/client structure as a debug service.
	 * Profiler sends information to the debug client, which forwards it to the debug server.
	 * The debug server then forwards client profiling data to the server-side profiler service.
	 *
	 * Profilers provide a way to measure and organize execution time of functions or sections of code.
	 * To profile a section of code, a profiling group must be created, default profiling groups exist
	 * for each thread. Once profiler receives profiling data from a client, it uses thread groups
	 * to organize this data.
	 *
	 * After a group has been obtained, a profiling frame can be created. Profiling frames are RAII-enabled structures
	 * that measure execution of code between initialization and destruction of the frame. Alternatively, `begin_frame`
	 * and `end_frame` member functions of a profiler group may be used to start & end a measurement. In either case,
	 * frames should be assigned a unique string ID used to distinguish measurements per-snapshot. Frame IDs will
	 * usually be the names of profiled functions or code sections.
	 *
	 * Thread groups store profiling data and once a profiler snapshot is requested, send all recorded profiling data
	 * information to the profiler service, which stores it in internal per-client buffer. Maximum size of the internal
	 * buffer can be configured through the profiler server. */
}	 // namespace sek::engine

#endif
