/*
 * ============================================================================
 * Sekhmet - C++20 game engine & editor
 * Copyright (C) 2022 switchblade
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 * ============================================================================
 *
 * Created by switchblade on 03/05/22
 */

#pragma once

#include <atomic>
#include <ctime>
#include <iostream>
#include <vector>

#include "sekhmet/detail/define.h"
#include "sekhmet/detail/event.hpp"
#include <fmt/chrono.h>
#include <fmt/format.h>
#include <fmt/xchar.h>

namespace sek::engine
{
	/** @vrief Log levels of `basic_logger`. */
	enum class log_level : int
	{
		info = 0,
		warn = 1,
		error = 2,
		fatal = 3
	};

	/** @brief Stream adapter used to preform logging.
	 *
	 * Internally, logger is a wrapper around a log event and additional formatting metadata.
	 * There are several builtin log levels:
	 * 1. Info level - Used to log generic info messages.
	 * 2. Warning level - Used to log important non-error messages (ex. runtime deprecation warnings).
	 * 3. Error level - Used to log non-fatal error messages (ex. from recoverable exceptions).
	 * 4. Fatal level - Used to log fatal error messages (ex. graphics initialization failure). */
	template<typename C, typename T = std::char_traits<C>>
	class basic_logger
	{
	public:
		typedef C value_type;
		typedef T traits_type;

	private:
		using sv_t = std::basic_string_view<value_type, traits_type>;
		using str_t = std::basic_string<value_type, traits_type>;
		using formatter_t = delegate<str_t(log_level, sv_t)>;
		using event_t = event<void(sv_t)>;

		template<log_level L>
		static std::atomic<basic_logger *> &global_ptr()
		{
			static basic_logger instance{L};
			static std::atomic<basic_logger *> ptr = &instance;
			return ptr;
		}

	public:
		/** Returns the global logger for the specified level. */
		template<log_level L>
		[[nodiscard]] static basic_logger &global()
		{
			return *global_ptr<L>();
		}
		/** Sets the global logger for the specified level and returns reference to the old logger. */
		template<log_level L>
		static basic_logger &global(basic_logger &l)
		{
			return *global_ptr<L>().exchange(&l);
		}

		/** Returns the global info logger. */
		[[nodiscard]] static basic_logger &info() { return global<log_level::info>(); }
		/** Returns the global warning logger. */
		[[nodiscard]] static basic_logger &warn() { return global<log_level::warn>(); }
		/** Returns the global error logger. */
		[[nodiscard]] static basic_logger &error() { return global<log_level::error>(); }
		/** Returns the global fatal logger. */
		[[nodiscard]] static basic_logger &fatal() { return global<log_level::fatal>(); }

	private:
		/* Character array instead of a string is used to enable conversion to target character type. */
		constexpr static value_type format_str_generic[] = {
			'[', '{', ':', '%', 'H', ':', '%', 'M', ':', '%', 'S', '}', ']', ':', ' ', '{', '}', '\n', '\0'};
		constexpr static value_type format_str[] = {'[', '{', ':', '%', 'H', ':', '%', 'M', ':', '%',  'S', '}',
													']', '[', '{', '}', ']', ':', ' ', '{', '}', '\n', '\0'};

		static str_t default_format(log_level lvl, sv_t msg)
		{
			const auto now = std::time(nullptr);
			std::string_view lvl_str;
			switch (lvl)
			{
				default: return fmt::format(format_str_generic, fmt::localtime(now), msg);
				case log_level::info: lvl_str = "Info"; break;
				case log_level::warn: lvl_str = "Warn"; break;
				case log_level::error: lvl_str = "Error"; break;
				case log_level::fatal: lvl_str = "Fatal"; break;
			}
			return fmt::format(format_str, fmt::localtime(now), lvl_str, msg);
		}

	public:
		/** Initializes default logger. */
		constexpr basic_logger() = default;

		constexpr basic_logger(const basic_logger &) = default;
		constexpr basic_logger &operator=(const basic_logger &) = default;
		constexpr basic_logger(basic_logger &&) noexcept = default;
		constexpr basic_logger &operator=(basic_logger &&) noexcept = default;

		/** Initializes logger with the specified level.
		 * A formatter receives the logged message & it's level and must return the formatted message string. */
		constexpr explicit basic_logger(log_level level) : level(level) {}
		/** Initializes logger with the specified formatter and level.
		 * A formatter receives the logged message & it's level and must return the formatted message string. */
		constexpr explicit basic_logger(formatter_t f, log_level level = log_level::info)
			: formatter(std::move(f)), level(level)
		{
		}

		/** Logs the provided message.
		 * @param msg Message string.
		 * @return Reference to this logger. */
		basic_logger &log(sv_t msg)
		{
			log_event(formatter(level, msg));
			return *this;
		}
		/** @copydoc log */
		basic_logger &operator<<(sv_t msg) { return log(msg); }

		/** Returns event proxy for the internal log event. */
		[[nodiscard]] constexpr event_proxy<event_t> on_log() noexcept { return event_proxy{log_event}; }

		constexpr void swap(basic_logger &other) noexcept
		{
			using std::swap;
			swap(log_event, other.log_event);
			swap(formatter, other.formatter);
			swap(level, other.level);
		}

		friend constexpr void swap(basic_logger &a, basic_logger &b) noexcept { a.swap(b); }

	private:
		/** Event used to dispatch logged messages. */
		event_t log_event;

	public:
		/* No reason to make getters & setters for these. */

		/** Formatter delegate used to format logged messages. */
		formatter_t formatter = default_format;
		/** Log level of this logger. */
		log_level level = log_level::info;
	};

	typedef basic_logger<char> logger;

	template<>
	template<>
	SEK_API std::atomic<logger *> &logger::global_ptr<log_level::info>();
	template<>
	template<>
	SEK_API std::atomic<logger *> &logger::global_ptr<log_level::warn>();
	template<>
	template<>
	SEK_API std::atomic<logger *> &logger::global_ptr<log_level::error>();
	template<>
	template<>
	SEK_API std::atomic<logger *> &logger::global_ptr<log_level::fatal>();
}	 // namespace sek