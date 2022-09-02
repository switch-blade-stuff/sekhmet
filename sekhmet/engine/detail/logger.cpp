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
		constexpr logger_init() { init(); }
		constexpr logger_init(const auto &lvl_long, const auto &lvl_short) : m_logger(lvl_long, lvl_short) { init(); }

		constexpr operator L &() noexcept { return m_logger; }

	private:
		void init()
		{
			if constexpr (requires { typename L::mutex_type; })
				m_logger.on_log().access()->subscribe(delegate_func<log_message>);
			else
				m_logger.on_log().subscribe(delegate_func<log_message>);
		}

		L m_logger;
	};

	template<>
	logger &detail::logger_base<logger, std::shared_mutex>::info()
	{
		static auto instance = logger_init<logger>{"INFO", "I"};
		return instance;
	}
	template<>
	logger &detail::logger_base<logger, std::shared_mutex>::warn()
	{
		static auto instance = logger_init<logger>{"WARN", "W"};
		return instance;
	}
	template<>
	logger &detail::logger_base<logger, std::shared_mutex>::error()
	{
		static auto instance = logger_init<logger>{"ERROR", "E"};
		return instance;
	}
	template<>
	logger &detail::logger_base<logger, std::shared_mutex>::fatal()
	{
		static auto instance = logger_init<logger>{"FATAL", "F"};
		return instance;
	}

	template class SEK_API_EXPORT basic_logger<char>;
	template class SEK_API_EXPORT basic_logger<char, std::char_traits<char>, void>;
}	 // namespace sek::engine