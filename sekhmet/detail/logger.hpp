//
// Created by switchblade on 03/05/22.
//

#pragma once

#include <atomic>
#include <ctime>
#include <functional>
#include <iostream>
#include <utility>
#include <vector>

#include "aligned_storage.hpp"
#include "define.h"
#include "static_string.hpp"
#include <fmt/chrono.h>
#include <fmt/format.h>
#include <fmt/xchar.h>

namespace sek
{
	namespace detail
	{
		template<typename U, typename C, typename T>
		concept log_listener =
			requires(U &dest, std::basic_string_view<C, T> msg) {
				// clang-format off
				requires requires { dest << msg; } ||
						 requires { dest.write(msg); } ||
						 requires { dest.write(msg.data(), msg.size()); } ||
						 requires { dest.write(msg.data(), static_cast<std::streamsize>(msg.size())); } ||
						 requires { dest << msg.data(); } ||
						 requires { dest.write(msg.data()); };
				// clang-format on
			};
	}

	/** @brief Stream adapter used to preform logging. */
	template<typename C, typename T = std::char_traits<C>>
	class basic_logger
	{
	public:
		typedef C value_type;
		typedef T traits_type;

	private:
		using sv_t = std::basic_string_view<value_type, traits_type>;
		using str_t = std::basic_string<value_type, traits_type>;

		constexpr static const value_type *msg_cat = "Message";
		constexpr static const value_type *warn_cat = "Warning";
		constexpr static const value_type *error_cat = "Error";

		/* Character array instead of a string is used to enable conversion to target character type. */
		constexpr static value_type format_str[] = {
			'[', '{', ':', '%', 'H', ':', '%', 'M', ':', '%', 'S', '}', ']', '>', ' ', '{', '}', '\0'};

		static str_t default_format(sv_t msg)
		{
			auto now = std::time(nullptr);
			return fmt::format(format_str, fmt::localtime(now), msg);
		}

		static std::atomic<basic_logger *> &msg_ptr()
		{
			static basic_logger instance{msg_cat};
			static std::atomic<basic_logger *> ptr = &instance;
			return ptr;
		}
		static std::atomic<basic_logger *> &warn_ptr()
		{
			static basic_logger instance{warn_cat};
			static std::atomic<basic_logger *> ptr = &instance;
			return ptr;
		}
		static std::atomic<basic_logger *> &error_ptr()
		{
			static basic_logger instance{error_cat};
			static std::atomic<basic_logger *> ptr = &instance;
			return ptr;
		}

	public:
		/** Returns the global message logger. */
		[[nodiscard]] static basic_logger &msg() { return *msg_ptr(); }
		/** Sets the global message logger and returns reference to the old logger. */
		static basic_logger &msg(basic_logger &l) { return *msg_ptr().exchange(&l); }

		/** Returns the global warning logger. */
		[[nodiscard]] static basic_logger &warn() { return *warn_ptr(); }
		/** Sets the global warning logger and returns reference to the old logger. */
		static basic_logger &warn(basic_logger &l) { return *warn_ptr().exchange(&l); }

		/** Returns the global error logger. */
		[[nodiscard]] static basic_logger &error() { return *error_ptr(); }
		/** Sets the global error logger and returns reference to the old logger. */
		static basic_logger &error(basic_logger &l) { return *error_ptr().exchange(&l); }

	private:
		template<typename U>
		constexpr static void write(U &dest, sv_t msg)
		{
			if constexpr (requires { dest << msg; })
				dest << msg;
			else if constexpr (requires { dest.write(msg); })
				dest.write(msg);
			else if constexpr (requires { dest.write(msg.data(), std::streamsize{}); })
				dest.write(msg.data(), static_cast<std::streamsize>(msg.size()));
			else if constexpr (requires { dest.write(msg.data(), std::size_t{}); })
				dest.write(msg.data(), msg.size());
			else if constexpr (requires { dest << msg.data(); })
				dest << msg.data();
			else if constexpr (requires { dest.write(msg.data()); })
				dest.write(msg.data());
		}

	public:
		/** Initializes logger with a default format. */
		constexpr basic_logger() = default;
		/** Initializes logger with a specified category and default format. */
		constexpr explicit basic_logger(std::basic_string_view<value_type, traits_type> cat) : category_str(cat) {}
		/** Initializes logger with specified formatter and category. */
		template<typename F>
		constexpr basic_logger(std::basic_string_view<value_type, traits_type> cat, F &&f)
			: format_func(std::forward<F>(f)), category_str(cat)
		{
		}

		// clang-format off
		/** Sets logger formatter. */
		template<typename F>
		constexpr void formatter(F &&f) noexcept requires std::is_invocable_r_v<str_t, F, sv_t>
		{
			format_func = std::forward<F>(f);
		}
		// clang-format on

		/** Returns logger category. */
		[[nodiscard]] constexpr const auto &category() const noexcept { return category_str; }
		/** Sets logger category. */
		constexpr void category(std::basic_string_view<value_type, traits_type> cat) { category_str = cat; }

		/** Constructs a listener in-place. */
		template<detail::log_listener<C, T> S, typename... Args>
		basic_logger &listen(std::in_place_type_t<S>, Args &&...args)
		{
			listeners.template emplace_back([s = S{std::forward<Args>(args)...}](sv_t msg) mutable { write(s, msg); });
			return *this;
		}
		/** Adds a listener stream to the logger. */
		template<detail::log_listener<C, T> S>
		basic_logger &listen(S &s)
		{
			listeners.template emplace_back([sp = &s](sv_t msg) { write(*sp, msg); });
			return *this;
		}
		/** @copydoc listen */
		template<detail::log_listener<C, T> S>
		basic_logger &operator+=(S &s)
		{
			return listen(s);
		}

		/** Logs the provided message. */
		basic_logger &log(std::basic_string_view<value_type, traits_type> msg)
		{
			const auto final_msg = fmt::format("[{}]{}", category_str, format_func(msg));
			for (auto &listener : listeners) listener(final_msg);
			return *this;
		}
		/** @copydoc log */
		basic_logger &operator<<(std::basic_string_view<value_type, traits_type> msg) { return log(msg); }

		constexpr void swap(basic_logger &other) noexcept
		{
			using std::swap;
			swap(format_func, other.format_func);
			swap(listeners, other.listeners);
			swap(category_str, other.category_str);
		}

	private:
		std::function<str_t(sv_t)> format_func = default_format;
		std::vector<std::function<void(sv_t)>> listeners;
		str_t category_str = msg_cat;
	};

	typedef basic_logger<std::ostream::char_type> logger;

	extern template class SEK_API basic_logger<std::ostream::char_type>;
}	 // namespace sek