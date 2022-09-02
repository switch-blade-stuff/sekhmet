/*
 * Created by switchblade on 03/05/22
 */

#include "logger.hpp"

#include <cstdio>

namespace sek::engine
{
	/* Need a wrapper factory here, since logger is not movable or copyable. */
	template<typename L>
	class logger_init
	{
		static void log_message(const typename L::string_type &msg) { fmt::print("{}\n", msg); }

	public:
		constexpr logger_init(const auto &lvl_long, const auto &lvl_short)
		{
			m_logger.value().on_log() += delegate_func<log_message>;
			m_logger.value().level(lvl_long, lvl_short);
		}

		constexpr operator shared_guard<L> &() noexcept { return m_logger; }

	private:
		shared_guard<L> m_logger;
	};

	template<>
	shared_guard<logger> &logger::info()
	{
		static auto instance = logger_init<logger>{"INFO", "I"};
		return instance;
	}
	template<>
	shared_guard<logger> &logger::warn()
	{
		static auto instance = logger_init<logger>{"WARN", "W"};
		return instance;
	}
	template<>
	shared_guard<logger> &logger::debug()
	{
		static auto instance = logger_init<logger>{"DEBUG", "D"};
		return instance;
	}
	template<>
	shared_guard<logger> &logger::error()
	{
		static auto instance = logger_init<logger>{"ERROR", "E"};
		return instance;
	}
	template<>
	shared_guard<logger> &logger::fatal()
	{
		static auto instance = logger_init<logger>{"FATAL", "F"};
		return instance;
	}

	template class SEK_API_EXPORT basic_logger<char>;
}	 // namespace sek::engine