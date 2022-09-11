/*
 * Created by switchblade on 03/05/22
 */

#include "logger.hpp"

#include <cstdio>

namespace sek::engine
{
	template<typename L>
	class logger_init
	{
		static void log_message(const typename L::string_type &msg) { fmt::print("{}\n", msg); }

	public:
		constexpr logger_init(const auto &level, bool enable = true) : m_logger(level)
		{
			m_logger.value().on_log() += delegate_func<log_message>;
			if (!enable) m_logger.value().disable();
		}

		constexpr operator shared_guard<L> &() noexcept { return m_logger; }

	private:
		shared_guard<L> m_logger;
	};

	template<>
	shared_guard<logger> &logger::info()
	{
		static auto instance = logger_init<logger>{"INFO"};
		return instance;
	}
	template<>
	shared_guard<logger> &logger::warn()
	{
		static auto instance = logger_init<logger>{"WARN"};
		return instance;
	}
	template<>
	shared_guard<logger> &logger::debug()
	{
#ifndef SEK_DEBUG
		static auto instance = logger_init<logger>{"DEBUG", false};
#else
		static auto instance = logger_init<logger>{"DEBUG"};
#endif
		return instance;
	}
	template<>
	shared_guard<logger> &logger::error()
	{
		static auto instance = logger_init<logger>{"ERROR"};
		return instance;
	}
	template<>
	shared_guard<logger> &logger::fatal()
	{
		static auto instance = logger_init<logger>{"FATAL"};
		return instance;
	}

	template class SEK_API_EXPORT basic_logger<char>;
}	 // namespace sek::engine