/*
 * Created by switchblade on 03/05/22
 */

#pragma once

#include <ctime>

#include "sekhmet/access_guard.hpp"
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
		using log_event = event<void(const string_type &)>;

	public:
		/** Returns the global info logger. */
		[[nodiscard]] static shared_guard<basic_logger> &info();
		/** Returns the global warning logger. */
		[[nodiscard]] static shared_guard<basic_logger> &warn();
		/** Returns the global error logger. */
		[[nodiscard]] static shared_guard<basic_logger> &error();
		/** Returns the global fatal logger. */
		[[nodiscard]] static shared_guard<basic_logger> &fatal();
		/** Returns the global debug logger.
		 * @note This logger is disabled by default in release mode. */
		[[nodiscard]] static shared_guard<basic_logger> &debug();

	private:
		constexpr static auto default_format = static_string_cast<value_type>("[{T:%H:%M:%S}][{L}]: {M}\n");

	public:
		basic_logger(const basic_logger &) = delete;
		basic_logger &operator=(const basic_logger &) = delete;
		basic_logger(basic_logger &&) = delete;
		basic_logger &operator=(basic_logger &&) = delete;

		/** Initializes default logger. */
		constexpr basic_logger() = default;

		/** Initializes logger with the specified log level.
		 * @param level String containing the log level. */
		template<typename S>
		constexpr explicit basic_logger(const S &level) : m_level(level)
		{
		}
		/** Initializes logger with the specified log level & format string.
		 * @param level String containing the log level.
		 * @param format String containing log message format.
		 *
		 * Format string should follow the `fmt` <a href="https://fmt.dev/latest/syntax.html#syntax">format string
		 * syntax</a> with the following additional named arguments available by default:
		 * 	* `M` - Main log message as `const string_type &`. This is always the first argument.
		 * 	* `L` - Level string as `const string_type &`.
		 * 	* `T` - Current time as `std::tm`. */
		template<typename L, typename F>
		constexpr basic_logger(const L &level, const F &format) : m_format(format), m_level(level)
		{
		}

		/** Checks if the logger is enabled. */
		[[nodiscard]] constexpr bool is_enabled() const noexcept { return m_enabled; }
		/** Returns the current level string. */
		[[nodiscard]] constexpr const string_type &level() const noexcept { return m_level; }
		/** Returns the current format string. */
		[[nodiscard]] constexpr const string_type &format() const noexcept { return m_format; }

		/** Enables the logger. */
		constexpr void enable() noexcept { m_enabled = true; }
		/** Disables the logger. */
		constexpr void disable() noexcept { m_enabled = false; }

		// clang-format off
		/** @brief Replaces the current log level strings.
		 * @param str String used for both long & short log level. */
		template<typename S>
		constexpr void level(const S &str) { m_level = str; }
		/** Replaces the current format string.
		 * @param str String containing log message format.
		 *
		 * Format string should follow the `fmt` <a href="https://fmt.dev/latest/syntax.html#syntax">format string
		 * syntax</a> with the following additional named arguments available by default:
		 * 	* `M` - Main log message as `const string_type &`. This is always the first argument.
		 * 	* `L` - Level string as `const string_type &`.
		 * 	* `T` - Current time as `std::tm`. */
		template<typename S>
		constexpr void format(const S &str) { m_format = str; }
		// clang-format on

		/** @copydoc level */
		constexpr void level(string_type &&str) { m_level = std::move(str); }
		/** @copydoc format */
		constexpr void format(string_type &&str) { m_format = std::move(str); }

		/** @brief Logs the provided message and any additional arguments even if the logger is disabled.
		 * @param msg Message string.
		 * @param args Additional arguments passed to `fmt::format`.
		 * @return Reference to this logger. */
		template<typename... Args>
		basic_logger &log_explicit(string_view_type msg, Args &&...args)
		{
			return log_explicit(std::locale{}, msg, std::forward<Args>(args)...);
		}
		/** @brief If the logger is enabled, logs the provided message and any additional arguments.
		 * @copydetails log_explicit */
		template<typename... Args>
		basic_logger &log(string_view_type msg, Args &&...args)
		{
			return log(std::locale{}, msg, std::forward<Args>(args)...);
		}

		/** @copydoc log_explicit
		 * @param loc Locale passed to `fmt::format`. */
		template<typename... Args>
		basic_logger &log_explicit(const std::locale &loc, string_view_type msg, Args &&...args)
		{
			// clang-format off
			string_type str = fmt::format(loc, m_format, fmt::arg("M", msg), std::forward<Args>(args)...,
										  fmt::arg("T", fmt::localtime(std::time(nullptr))),
										  fmt::arg("L", m_level));
			// clang-format on
			m_log_event(str);
			return *this;
		}
		/** @copydoc log
		 * @param loc Locale passed to `fmt::format`. */
		template<typename... Args>
		basic_logger &log(const std::locale &loc, string_view_type msg, Args &&...args)
		{
			if (m_enabled) log_explicit(loc, msg, std::forward<Args>(args)...);
			return *this;
		}
		/** If the logger is enabled, logs the provided message.
		 * @param msg Message string. */
		basic_logger &operator<<(string_view_type msg) { return log(msg); }

		/** Returns an event proxy for the internal log event. */
		[[nodiscard]] constexpr event_proxy<log_event> on_log() noexcept { return event_proxy{m_log_event}; }

	private:
		log_event m_log_event;
		string_type m_format = {default_format.data(), default_format.size()};
		string_type m_level;
		bool m_enabled = true;
	};

	template<typename C, typename T>
	shared_guard<basic_logger<C, T>> &basic_logger<C, T>::info()
	{
		static shared_guard<basic_logger> instance{static_string_cast<C, T>("INFO").data()};
		return instance;
	}
	template<typename C, typename T>
	shared_guard<basic_logger<C, T>> &basic_logger<C, T>::warn()
	{
		static shared_guard<basic_logger> instance{static_string_cast<C, T>("WARN").data()};
		return instance;
	}
	template<typename C, typename T>
	shared_guard<basic_logger<C, T>> &basic_logger<C, T>::debug()
	{
		static shared_guard<basic_logger> instance{static_string_cast<C, T>("DEBUG").data()};
		return instance;
	}
	template<typename C, typename T>
	shared_guard<basic_logger<C, T>> &basic_logger<C, T>::error()
	{
		static shared_guard<basic_logger> instance{static_string_cast<C, T>("ERROR").data()};
		return instance;
	}
	template<typename C, typename T>
	shared_guard<basic_logger<C, T>> &basic_logger<C, T>::fatal()
	{
		static shared_guard<basic_logger> instance{static_string_cast<C, T>("FATAL").data()};
		return instance;
	}

	/** @brief Alias of `basic_logger` for `char` type. By default, global logger categories print to `stdout`. */
	typedef basic_logger<char> logger;

	template<>
	shared_guard<logger> &logger::info();
	template<>
	shared_guard<logger> &logger::warn();
	template<>
	shared_guard<logger> &logger::debug();
	template<>
	shared_guard<logger> &logger::error();
	template<>
	shared_guard<logger> &logger::fatal();

	extern template class SEK_API_IMPORT basic_logger<char>;
}	 // namespace sek::engine