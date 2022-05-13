//
// Created by switchblade on 03/05/22.
//

#pragma once

#include <atomic>
#include <ctime>
#include <iostream>
#include <vector>

#include "define.h"
#include "event.hpp"
#include <fmt/chrono.h>
#include <fmt/format.h>
#include <fmt/xchar.h>

namespace sek
{
	namespace detail
	{
		// clang-format off
		template<typename U, typename C, typename T>
		concept log_listener = requires(U &dest, std::basic_string_view<C, T> msg) {
				requires requires { dest(msg); } ||
						 requires { dest << msg; } ||
						 requires { dest.write(msg); } ||
						 requires { dest.write(msg.data(), msg.size()); } ||
						 requires { dest.write(msg.data(), static_cast<std::streamsize>(msg.size())); } ||
						 requires { dest << msg.data(); } ||
						 requires { dest.write(msg.data()); };
			};
		// clang-format on
	}	 // namespace detail

	/** @brief Stream adapter used to preform logging.
	 *
	 * Internally, logger is a wrapper around a log event and additional formatting metadata.
	 * Several builtin "global" loggers are provided:
	 * 1. Message logger - Used to log generic messages.
	 * 2. Warning logger - Used to log important non-error messages (ex. runtime deprecation warnings).
	 * 3. Error logger - Used to log non-critical error messages (ex. from recoverable exceptions).
	 * 4. Critical logger - Used to log critical (potentially fatal) messages (ex. graphics initialization failure). */
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
		constexpr static const value_type *crit_cat = "Critical";

		static std::atomic<basic_logger *> &msg_ptr();
		static std::atomic<basic_logger *> &warn_ptr();
		static std::atomic<basic_logger *> &error_ptr();
		static std::atomic<basic_logger *> &crit_ptr();

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

		/** Returns the global critical logger. */
		[[nodiscard]] static basic_logger &fatal() { return *crit_ptr(); }
		/** Sets the global critical logger and returns reference to the old logger. */
		static basic_logger &fatal(basic_logger &l) { return *crit_ptr().exchange(&l); }

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

		/* Character array instead of a string is used to enable conversion to target character type. */
		constexpr static value_type format_str[] = {'[', '{', ':', '%', 'H', ':', '%', 'M', ':', '%',  'S', '}',
													']', '[', '{', '}', ']', ':', ' ', '{', '}', '\n', '\0'};
		static str_t default_format(sv_t cat, sv_t msg)
		{
			auto now = std::time(nullptr);
			return fmt::format(format_str, fmt::localtime(now), cat, msg);
		}

	public:
		/** Initializes logger with a default format. */
		constexpr basic_logger() = default;

		constexpr basic_logger(const basic_logger &other)
			: format_func(other.format_func), log_event(other.log_event), category_str(other.category_str)
		{
		}
		constexpr basic_logger &operator=(const basic_logger &other)
		{
			format_func = other.format_func;
			log_event = other.log_event;
			category_str = other.category_str;
			return *this;
		}
		constexpr basic_logger(basic_logger &&other) noexcept
			: format_func(std::move(other.format_func)),
			  log_event(std::move(other.log_event)),
			  category_str(std::move(other.category_str))
		{
		}
		constexpr basic_logger &operator=(basic_logger &&other) noexcept
		{
			format_func = std::move(other.format_func);
			log_event = std::move(other.log_event);
			category_str = std::move(other.category_str);
			return *this;
		}

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
		constexpr void formatter(delegate<str_t(sv_t, sv_t)> f) noexcept
		{
			sync_busy([&]() { format_func = std::move(f); });
		}
		// clang-format on

		/** Returns logger category. */
		[[nodiscard]] constexpr const auto &category() const noexcept { return category_str; }
		/** Sets logger category. */
		constexpr void category(std::basic_string_view<value_type, traits_type> cat)
		{
			sync_busy([&]() { category_str = cat; });
		}

		/** Adds a listener delegate to the logger.
		 * @param listener Listener delegate.
		 * @return Reference to this stream. */
		basic_logger &listen(delegate<void(std::basic_string_view<C, T>)> listener)
		{
			sync_busy([&]() { log_event += listener; });
			return *this;
		}
		/** @copydoc listen */
		basic_logger &operator+=(delegate<void(std::basic_string_view<C, T>)> listener) { return listen(listener); }
		/** Adds a listener object to the logger.
		 * @param listener Reference to the stream-like listener object.
		 * @return Reference to this stream. */
		template<detail::log_listener<C, T> L>
		basic_logger &listen(L &listener)
		{
			listen(delegate{+[](L *lp, sv_t msg) { write(*lp, msg); }, &listener});
			return *this;
		}
		/** @copydoc listen */
		template<detail::log_listener<C, T> L>
		basic_logger &operator+=(L &listener)
		{
			return listen(listener);
		}
		/** Removes (silences) a listener associated with this logger.
		 * @param listener Reference to a previously added listener.
		 * @return true if the listener was removed, false otherwise. */
		template<detail::log_listener<C, T> L>
		bool silence(L &listener)
		{
			bool res;
			sync_busy(
				[&]()
				{
					const auto log_iter = log_event.find(&listener);
					if ((res = log_iter != log_event.end())) log_event.unsubscribe(log_iter);
				});
			return res;
		}
		/** @copydoc silence */
		template<detail::log_listener<C, T> L>
		bool operator-=(L &listener)
		{
			return silence(listener);
		}

		/** Logs the provided message. */
		basic_logger &log(std::basic_string_view<value_type, traits_type> msg)
		{
			sync_busy([&]() { log_event(format_func(category_str, msg)); });
			return *this;
		}
		/** @copydoc log */
		basic_logger &operator<<(std::basic_string_view<value_type, traits_type> msg) { return log(msg); }

		constexpr void swap(basic_logger &other) noexcept
		{
			using std::swap;
			swap(format_func, other.format_func);
			swap(log_event, other.log_event);
			swap(category_str, other.category_str);
		}

		friend constexpr void swap(basic_logger &a, basic_logger &b) noexcept { a.swap(b); }

	private:
		void sync_busy(auto &&f)
		{
			/* Wait until the flag is clear, then set the flag. */
			busy.wait(true);
			while (busy.test_and_set()) /* Keep waiting if the flag was already set by another thread. */
				[[likely]] busy.wait(true);

			/* Do whatever we need to do while the flag is set. */
			f();

			/* Clear the flag & notify the next waiting thread. */
			busy.clear(std::memory_order_relaxed);
			busy.notify_one();
		}

		delegate<str_t(sv_t, sv_t)> format_func = default_format;
		event<void(sv_t)> log_event;
		str_t category_str = msg_cat;
		std::atomic_flag busy;
	};

	typedef basic_logger<char> logger;

	template<typename C, typename T>
	std::atomic<basic_logger<C, T> *> &basic_logger<C, T>::msg_ptr()
	{
		static basic_logger instance{msg_cat};
		static std::atomic<basic_logger *> ptr = &instance;
		return ptr;
	}
	template<typename C, typename T>
	std::atomic<basic_logger<C, T> *> &basic_logger<C, T>::warn_ptr()
	{
		static basic_logger instance{warn_cat};
		static std::atomic<basic_logger *> ptr = &instance;
		return ptr;
	}
	template<typename C, typename T>
	std::atomic<basic_logger<C, T> *> &basic_logger<C, T>::error_ptr()
	{
		static basic_logger instance{error_cat};
		static std::atomic<basic_logger *> ptr = &instance;
		return ptr;
	}
	template<typename C, typename T>
	std::atomic<basic_logger<C, T> *> &basic_logger<C, T>::crit_ptr()
	{
		static basic_logger instance{crit_cat};
		static std::atomic<basic_logger *> ptr = &instance;
		return ptr;
	}

	template<>
	SEK_API_IMPORT std::atomic<logger *> &logger::msg_ptr();
	template<>
	SEK_API_IMPORT std::atomic<logger *> &logger::warn_ptr();
	template<>
	SEK_API_IMPORT std::atomic<logger *> &logger::error_ptr();
	template<>
	SEK_API_IMPORT std::atomic<logger *> &logger::crit_ptr();

	extern template class SEK_API_IMPORT basic_logger<char>;
}	 // namespace sek