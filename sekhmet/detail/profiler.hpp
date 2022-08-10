/*
 * Created by switchblade on 09/08/22
 */

#pragma once

#include <string>

#include "access_guard.hpp"
#include "assert.hpp"
#include "dense_map.hpp"
#include "service.hpp"

#ifndef SEK_NO_PROFILER

namespace sek
{
	/** @brief Profiler service used to implement debug profiling support.
	 *
	 * Profiler can be initialized as either a client or a server profiler. Server profilers record profiling data
	 * (both local and external), while client profilers send all profiling data to the server. */
	class profiler : service<access_guard<profiler, std::recursive_mutex>>
	{
	public:
		/** @brief Structure used to group profiling records. */
		class group
		{
		public:
			/** Returns profiler group for the current thread. */
			static SEK_API group &current();

		public:
			/** Initializes a profiler group with an empty name. */
			constexpr group() noexcept = default;

			/** Initializes a profiler group with a given name. */
			constexpr group(std::string_view name) : m_name(name) {}
			/** @copydoc group */
			constexpr group(const std::string &name) : m_name(name) {}
			/** @copydoc group */
			constexpr group(std::string &&name) : m_name(std::move(name)) {}

			/** Sets name of the group. */
			constexpr void name(std::string_view name) { m_name = name; }
			/** @copydoc name */
			constexpr void name(const std::string &name) { m_name = name; }
			/** @copydoc name */
			constexpr void name(std::string &&name) { m_name = std::move(name); }
			/** Returns name of the group. */
			[[nodiscard]] constexpr const std::string &name() const noexcept { return m_name; }

		private:
			std::string m_name;
		};

		/** @brief Structure used to profile execution in RAII-controlled scope. */
		class frame
		{
		public:
			/** Initializes a profiling frame for the current thread group and frame id. */
			explicit frame(std::string_view id) : frame(group::current(), id) {}
			/** @copydoc frame */
			explicit frame(const std::string &id) : frame(group::current(), id) {}
			/** @copydoc frame */
			explicit frame(std::string &&id) : frame(group::current(), std::move(id)) {}
			/** Initializes a profiling frame for the specified group and frame id. */
			constexpr frame(group &parent, std::string_view id) : m_parent(parent), m_id(id) {}
			/** @copydoc frame */
			constexpr frame(group &parent, const std::string &id) : m_parent(parent), m_id(id) {}
			/** @copydoc frame */
			constexpr frame(group &parent, std::string &&id) : m_parent(parent), m_id(std::move(id)) {}

			/** Returns id of the profiling frame. */
			[[nodiscard]] constexpr const std::string &id() const noexcept { return m_id; }

			/** Starts profiling sample for this frame. */
			SEK_API void sample_start();
			/** Ends profiling sample for this frame and publishes it to the parent group.
			 * @note Does nothing if no sample was started. */
			SEK_API void sample_end();

		private:
			group &m_parent;
			std::string m_id;
			bool m_started;
		};

	public:
		/** Initializes debug profiler for the specified port number.
		 * @param client Whether the profiler is initialized as a client profiler and should send data to the server. */
		SEK_API profiler(bool client = false);

		/** Returns`true` if the profiler instance is the client profiler, `false` if server. */
		[[nodiscard]] constexpr bool is_client() const noexcept { return m_is_client; }

	private:
		bool m_is_client;
	};

	extern template class SEK_API_IMPORT service<access_guard<profiler, std::recursive_mutex>>;
}	 // namespace sek

#endif

#if !defined(SEK_NO_PROFILER)
#define SEK_PROFILE_FRAME(...)
#define SEK_PROFILE(...)
#else
#define SEK_PROFILE_FRAME(...)
#define SEK_PROFILE(...)
#endif
