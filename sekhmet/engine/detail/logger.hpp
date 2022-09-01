/*
 * Created by switchblade on 03/05/22
 */

#pragma once

#include <atomic>
#include <ctime>
#include <iostream>
#include <vector>

#include "sekhmet/detail/define.h"
#include "sekhmet/event.hpp"
#include "sekhmet/static_string.hpp"

#include <fmt/chrono.h>
#include <fmt/format.h>
#include <fmt/xchar.h>

namespace sek::engine
{
	/** @brief Stream adapter used to preform logging.
	 *
	 * @tparam C Character type of the logger.
	 * @tparam T Traits type of `C`. */
	template<typename C, typename T = std::char_traits<C>>
	class basic_logger
	{
	public:
		typedef C value_type;
		typedef T traits_type;

		using string_view_type = std::basic_string_view<value_type, traits_type>;
		using string_type = std::basic_string<value_type, traits_type>;
		using log_event_t = event<void(const string_type &)>;

	public:
		/** Returns the global info logger. */
		[[nodiscard]] static basic_logger &info();
		/** Returns the global warning logger. */
		[[nodiscard]] static basic_logger &warn();
		/** Returns the global error logger. */
		[[nodiscard]] static basic_logger &error();
		/** Returns the global fatal logger. */
		[[nodiscard]] static basic_logger &fatal();

	private:
		constexpr static auto default_format = static_string_cast<value_type>("[{:%H:%M:%S}][{L}]: {M}\n");

	public:
		/** Initializes default logger. */
		constexpr basic_logger() = default;

		constexpr basic_logger(const basic_logger &) = default;
		constexpr basic_logger &operator=(const basic_logger &) = default;
		constexpr basic_logger(basic_logger &&) noexcept = default;
		constexpr basic_logger &operator=(basic_logger &&) noexcept = default;

		/** Initializes logger with the specified log level.
		 * @param level String used for both long & short log level. */
		template<typename S>
		constexpr explicit basic_logger(const S &level) : basic_logger(level, level)
		{
		}
		/** @copydoc basic_logger */
		constexpr explicit basic_logger(const value_type *level) : basic_logger(level, level) {}
		/** Initializes logger with the specified log level.
		 * @param level_long String used for long log level.
		 * @param level_short String used for short log level. */
		template<typename S0, typename S1>
		constexpr basic_logger(const S0 &level_long, const S1 &level_short)
			: basic_logger(level_long, level_short, default_format)
		{
		}
		/** @copydoc basic_logger */
		constexpr basic_logger(const value_type *level_long, const value_type *level_short)
			: basic_logger(level_long, level_short, default_format)
		{
		}
		/** Initializes logger with the specified log level & format string.
		 * @param level_long String used for long log level.
		 * @param level_short String used for short log level.
		 * @param format String containing log message format.
		 *
		 * Format string should follow the `fmt` <a href="https://fmt.dev/latest/syntax.html#syntax">format string
		 * syntax</a> with the following additional named arguments available by default:
		 * 	* `L` - Long level string.
		 * 	* `l` - Short level string.
		 * 	* `M` - Main log message. */
		template<typename S0, typename S1, typename S2>
		constexpr basic_logger(const S0 &level_long, const S1 &level_short, const S2 &format)
			: m_format(format), m_level_short(level_short), m_level_long(level_long)
		{
		}
		/** @copydoc basic_logger */
		constexpr basic_logger(const value_type *level_long, const value_type *level_short, const value_type *format)
			: m_format(format), m_level_short(level_short), m_level_long(level_long)
		{
		}

		/** Logs the provided message and any additional arguments.
		 * @param msg Message string.
		 * @param args Additional arguments passed to `fmt::format`.
		 * @return Reference to this logger. */
		template<typename... Args>
		basic_logger &log(string_view_type msg, Args &&...args)
		{
			return log(std::locale{}, msg, std::forward<Args>(args)...);
		}
		/** @copydoc log
		 * @param loc Locale passed to `fmt::format`. */
		template<typename... Args>
		basic_logger &log(const std::locale &loc, string_view_type msg, Args &&...args)
		{
			// clang-format off
			string_type str = fmt::format(loc, m_format,
										  fmt::arg("L", m_level_long),
										  fmt::arg("l", m_level_short),
										  fmt::arg("M", msg),
										  std::forward<Args>(args)...);
			// clang-format on

			m_log_event(str);
			return *this;
		}
		/** Logs the provided message.
		 * @param msg Message string. */
		basic_logger &operator<<(string_view_type msg) { return log(msg); }

		/** Returns the current format string. */
		[[nodiscard]] constexpr const string_type &format() const noexcept { return m_format; }
		/** Returns the current long level string. */
		[[nodiscard]] constexpr const string_type &level_long() const noexcept { return m_level_long; }
		/** Returns the current short level string. */
		[[nodiscard]] constexpr const string_type &level_short() const noexcept { return m_level_short; }

		// clang-format off
		/** Replaces the current format string.
		 * @param str String containing log message format.
		 *
		 * Format string should follow the `fmt` <a href="https://fmt.dev/latest/syntax.html#syntax">format string
		 * syntax</a> with the following additional named arguments available by default:
		 * 	* `L` - Long level string.
		 * 	* `l` - Short level string.
		 * 	* `M` - Main log message.*/
		template<typename S>
		void format(const S &str) { m_format = str; }
		/** @copydoc format */
		void format(const value_type *str) { m_format = str; }
		/** @copydoc format */
		void format(string_type &&str) noexcept(std::is_nothrow_move_assignable_v<string_type>) { m_format = std::move(str); }

		/** @brief Replaces the current log level strings.
		 * @param str String used for both long & short log level. */
		template<typename S>
		void level(const S &str) { level(str, str); }
		// clang-format on

		/** @copydoc level */
		void level(const value_type *str) { level(str, str); }
		/** @copybrief level
		 * @param level_long String used for long log level.
		 * @param level_short String used for short log level. */
		template<typename S0, typename S1>
		void level(const S0 &level_long, const S1 &level_short)
		{
			m_level_short = level_short;
			m_level_long = level_long;
		}
		/** @copydoc level */
		void level(const value_type *level_long, const value_type *level_short)
		{
			m_level_short = level_short;
			m_level_long = level_long;
		}
		/** @copydoc level */
		void level(string_type &&level_long, string_type &&level_short) noexcept(std::is_nothrow_move_assignable_v<string_type>)
		{
			m_level_short = std::move(level_short);
			m_level_long = std::move(level_long);
		}

		/** Returns event proxy for the internal log event. */
		[[nodiscard]] constexpr event_proxy<log_event_t> on_log() noexcept { return event_proxy{m_log_event}; }

		constexpr void swap(basic_logger &other) noexcept
		{
			using std::swap;
			swap(m_log_event, other.m_log_event);
			swap(m_format, other.m_format);
			swap(m_level_short, other.m_level_short);
			swap(m_level_long, other.m_level_long);
		}
		friend constexpr void swap(basic_logger &a, basic_logger &b) noexcept { a.swap(b); }

	private:
		log_event_t m_log_event;
		string_type m_format = default_format;
		string_type m_level_short;
		string_type m_level_long;
	};

	template<typename C, typename T>
	basic_logger<C, T> &basic_logger<C, T>::info()
	{
		static basic_logger<C, T> instance{static_string_cast<C, T>("INFO"), static_string_cast<C, T>("I")};
		return instance;
	}
	template<typename C, typename T>
	basic_logger<C, T> &basic_logger<C, T>::warn()
	{
		static basic_logger<C, T> instance{static_string_cast<C, T>("WARN"), static_string_cast<C, T>("W")};
		return instance;
	}
	template<typename C, typename T>
	basic_logger<C, T> &basic_logger<C, T>::error()
	{
		static basic_logger<C, T> instance{static_string_cast<C, T>("ERROR"), static_string_cast<C, T>("E")};
		return instance;
	}
	template<typename C, typename T>
	basic_logger<C, T> &basic_logger<C, T>::fatal()
	{
		static basic_logger<C, T> instance{static_string_cast<C, T>("FATAL"), static_string_cast<C, T>("F")};
		return instance;
	}

	typedef basic_logger<char> logger;
	typedef basic_logger<wchar_t> wlogger;

	template<>
	basic_logger<char> &basic_logger<char>::info();
	template<>
	basic_logger<char> &basic_logger<char>::warn();
	template<>
	basic_logger<char> &basic_logger<char>::error();
	template<>
	basic_logger<char> &basic_logger<char>::fatal();

	extern template class SEK_API_IMPORT basic_logger<char>;

	template<>
	basic_logger<wchar_t> &basic_logger<wchar_t>::info();
	template<>
	basic_logger<wchar_t> &basic_logger<wchar_t>::warn();
	template<>
	basic_logger<wchar_t> &basic_logger<wchar_t>::error();
	template<>
	basic_logger<wchar_t> &basic_logger<wchar_t>::fatal();

	extern template class SEK_API_IMPORT basic_logger<wchar_t>;
}	 // namespace sek::engine