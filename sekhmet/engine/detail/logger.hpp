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
		/** Returns the global debug logger. */
		[[nodiscard]] static shared_guard<basic_logger> &debug();
		/** Returns the global error logger. */
		[[nodiscard]] static shared_guard<basic_logger> &error();
		/** Returns the global fatal logger. */
		[[nodiscard]] static shared_guard<basic_logger> &fatal();

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
		 * 	* `L` - Long level string as `const string_type &`.
		 * 	* `l` - Short level string as `const string_type &`.
		 * 	* `M` - Main log message as `const string_type &`.
		 * 	* `T` - Current time as `std::tm`. */
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

		/** Checks if the logger is enabled. */
		[[nodiscard]] constexpr bool is_enabled() const noexcept { return m_enabled; }
		/** Returns the current format string. */
		[[nodiscard]] constexpr const string_type &format() const noexcept { return m_format; }
		/** Returns the current long level string. */
		[[nodiscard]] constexpr const string_type &level_long() const noexcept { return m_level_long; }
		/** Returns the current short level string. */
		[[nodiscard]] constexpr const string_type &level_short() const noexcept { return m_level_short; }

		/** Enables the logger. */
		constexpr void enable() noexcept { m_enabled = true; }
		/** Disables the logger. */
		constexpr void disable() noexcept { m_enabled = false; }

		// clang-format off
		/** Replaces the current format string.
		 * @param str String containing log message format.
		 *
		 * Format string should follow the `fmt` <a href="https://fmt.dev/latest/syntax.html#syntax">format string
		 * syntax</a> with the following additional named arguments available by default:
		 * 	* `L` - Long level string as `const string_type &`.
		 * 	* `l` - Short level string as `const string_type &`.
		 * 	* `M` - Main log message as `const string_type &`.
		 * 	* `T` - Current time as `std::tm`. */
		template<typename S>
		constexpr void format(const S &str) { m_format = str; }
		/** @brief Replaces the current log level strings.
		 * @param str String used for both long & short log level. */
		template<typename S>
		constexpr void level(const S &str) { level(str, str); }
		// clang-format on

		/** @copydoc format */
		constexpr void format(string_type &&str) { m_format = std::move(str); }
		/** @copybrief level
		 * @param level_long String used for long log level.
		 * @param level_short String used for short log level. */
		template<typename S0, typename S1>
		constexpr void level(const S0 &level_long, const S1 &level_short)
		{
			m_level_short = level_short;
			m_level_long = level_long;
		}
		/** @copydoc level */
		constexpr void level(string_type &&level_long, string_type &&level_short)
		{
			m_level_short = std::move(level_short);
			m_level_long = std::move(level_long);
		}

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
			string_type str = fmt::format(loc, m_format, fmt::arg("M", msg),
										  fmt::arg("T", fmt::localtime(std::time(nullptr))),
										  fmt::arg("L", m_level_long),
										  fmt::arg("l", m_level_short),
										  std::forward<Args>(args)...);
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
		string_type m_level_short;
		string_type m_level_long;
		bool m_enabled = true;
	};

	// clang-format off
	template<typename C, typename T>
	shared_guard<basic_logger<C, T>> &basic_logger<C, T>::info()
	{
		static shared_guard<basic_logger> instance{
			std::in_place_type<basic_logger>,
			static_string_cast<C, T>("INFO"),
			static_string_cast<C, T>("I")};
		return instance;
	}
	template<typename C, typename T>
	shared_guard<basic_logger<C, T>> &basic_logger<C, T>::warn()
	{
		static shared_guard<basic_logger> instance{
			std::in_place_type<basic_logger>,
			static_string_cast<C, T>("WARN"),
			static_string_cast<C, T>("W")};
		return instance;
	}
	template<typename C, typename T>
	shared_guard<basic_logger<C, T>> &basic_logger<C, T>::debug()
	{
		static shared_guard<basic_logger> instance{
				std::in_place_type<basic_logger>,
				static_string_cast<C, T>("DEBUG"),
				static_string_cast<C, T>("D")};
		return instance;
	}
	template<typename C, typename T>
	shared_guard<basic_logger<C, T>> &basic_logger<C, T>::error()
	{
		static shared_guard<basic_logger> instance{
			std::in_place_type<basic_logger>,
			static_string_cast<C, T>("ERROR"),
			static_string_cast<C, T>("E")};
		return instance;
	}
	template<typename C, typename T>
	shared_guard<basic_logger<C, T>> &basic_logger<C, T>::fatal()
	{
		static shared_guard<basic_logger> instance{
			std::in_place_type<basic_logger>,
			static_string_cast<C, T>("FATAL"),
			static_string_cast<C, T>("F")};
		return instance;
	}
	// clang-format on

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