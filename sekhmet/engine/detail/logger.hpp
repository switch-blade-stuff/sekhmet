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
	namespace detail
	{
		template<typename Logger, typename M>
		class logger_base
		{
		public:
			typedef M mutex_type;

			/** Returns the global info logger. */
			[[nodiscard]] static Logger &info();
			/** Returns the global warning logger. */
			[[nodiscard]] static Logger &warn();
			/** Returns the global error logger. */
			[[nodiscard]] static Logger &error();
			/** Returns the global fatal logger. */
			[[nodiscard]] static Logger &fatal();

		public:
			constexpr logger_base() noexcept(std::is_nothrow_default_constructible_v<M>) = default;

		protected:
			template<typename T, typename U>
			[[nodiscard]] constexpr ref_guard<T, M> guard_value(U &&value) const
			{
				return ref_guard<T, M>{std::forward<U>(value), m_mtx};
			}

			template<typename F>
			constexpr decltype(auto) invoke_locked(F &&f)
			{
				std::unique_lock<M> l(m_mtx);
				return std::invoke(std::forward<F>(f));
			}
			template<typename F>
			constexpr decltype(auto) invoke_locked(F &&f) const
			{
				std::unique_lock<M> l(m_mtx);
				return std::invoke(std::forward<F>(f));
			}
			// clang-format off
			template<typename F>
			constexpr decltype(auto) invoke_locked(F &&f) const requires(requires(M m){ m.lock_shared(); })
			{
				std::shared_lock<M> l(m_mtx);
				return std::invoke(std::forward<F>(f));
			}
			// clang-format on

		private:
			mutable M m_mtx;
		};
		template<typename Logger>
		class logger_base<Logger, void>
		{
		protected:
			template<typename T, typename U>
			[[nodiscard]] constexpr T guard_value(U &&value) const
			{
				return T{std::forward<U>(value)};
			}

			template<typename F>
			constexpr decltype(auto) invoke_locked(F &&f)
			{
				return std::invoke(std::forward<F>(f));
			}
			template<typename F>
			constexpr decltype(auto) invoke_locked(F &&f) const
			{
				return std::invoke(std::forward<F>(f));
			}
		};
	}	 // namespace detail

	/** @brief Stream adapter used to preform logging.
	 *
	 * @tparam C Character type of the logger.
	 * @tparam T Traits type of `C`.
	 * @tparam Mutex Mutex used to synchronize log operations. If set to `void`, logger is not synchronized. */
	template<typename C, typename T = std::char_traits<C>, typename Mutex = std::shared_mutex>
	class basic_logger : public detail::logger_base<basic_logger<C, T, Mutex>, Mutex>
	{
	public:
		typedef C value_type;
		typedef T traits_type;

		using string_view_type = std::basic_string_view<value_type, traits_type>;
		using string_type = std::basic_string<value_type, traits_type>;
		using log_event_t = event<void(const string_type &)>;

	private:
		using base_t = detail::logger_base<basic_logger, Mutex>;

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
			base_t::invoke_locked(
				[&]()
				{
					// clang-format off
					string_type str = fmt::format(loc, m_format, fmt::arg("M", msg),
												  fmt::arg("T", fmt::localtime(std::time(nullptr))),
												  fmt::arg("L", m_level_long),
												  fmt::arg("l", m_level_short),
												  std::forward<Args>(args)...);
					// clang-format on
					m_log_event(str);
				});
			return *this;
		}
		/** Logs the provided message.
		 * @param msg Message string. */
		basic_logger &operator<<(string_view_type msg) { return log(msg); }

		/** Returns the current format string. */
		[[nodiscard]] constexpr const string_type &format() const
		{
			base_t::invoke_locked([&]() { return m_format; });
		}
		/** Returns the current long level string. */
		[[nodiscard]] constexpr const string_type &level_long() const
		{
			base_t::invoke_locked([&]() { return m_level_long; });
		}
		/** Returns the current short level string. */
		[[nodiscard]] constexpr const string_type &level_short() const
		{
			base_t::invoke_locked([&]() { return m_level_short; });
		}

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
		void format(const S &str)
		{
			base_t::invoke_locked([&]() { m_format = str; });
		}
		/** @copydoc format */
		void format(string_type &&str)
		{
			base_t::invoke_locked([&]() { m_format = std::move(str); });
		}

		/** @brief Replaces the current log level strings.
		 * @param str String used for both long & short log level. */
		template<typename S>
		void level(const S &str)
		{
			level(str, str);
		}
		/** @copybrief level
		 * @param level_long String used for long log level.
		 * @param level_short String used for short log level. */
		template<typename S0, typename S1>
		void level(const S0 &level_long, const S1 &level_short)
		{
			base_t::invoke_locked(
				[&]()
				{
					m_level_short = level_short;
					m_level_long = level_long;
				});
		}
		/** @copydoc level */
		void level(string_type &&level_long, string_type &&level_short)
		{
			base_t::invoke_locked(
				[&]()
				{
					m_level_short = std::move(level_short);
					m_level_long = std::move(level_long);
				});
		}

		/** Returns a guarded event proxy for the internal log event. */
		[[nodiscard]] constexpr auto on_log()
		{
			return base_t::template guard_value<event_proxy<log_event_t>>(m_log_event);
		}

	private:
		log_event_t m_log_event;
		string_type m_format = {default_format.data(), default_format.size()};
		string_type m_level_short;
		string_type m_level_long;
	};

	namespace detail
	{
		template<typename Logger, typename M>
		Logger &logger_base<Logger, M>::info()
		{
			using C = typename Logger::value_type;
			using T = typename Logger::traits_type;

			static Logger instance{static_string_cast<C, T>("INFO"), static_string_cast<C, T>("I")};
			return instance;
		}
		template<typename Logger, typename M>
		Logger &logger_base<Logger, M>::warn()
		{
			using C = typename Logger::value_type;
			using T = typename Logger::traits_type;

			static Logger instance{static_string_cast<C, T>("WARN"), static_string_cast<C, T>("W")};
			return instance;
		}
		template<typename Logger, typename M>
		Logger &logger_base<Logger, M>::error()
		{
			using C = typename Logger::value_type;
			using T = typename Logger::traits_type;

			static Logger instance{static_string_cast<C, T>("ERROR"), static_string_cast<C, T>("E")};
			return instance;
		}
		template<typename Logger, typename M>
		Logger &logger_base<Logger, M>::fatal()
		{
			using C = typename Logger::value_type;
			using T = typename Logger::traits_type;

			static Logger instance{static_string_cast<C, T>("FATAL"), static_string_cast<C, T>("F")};
			return instance;
		}
	}	 // namespace detail

	/** @brief Alias of `basic_logger<char>`. By default, global logger categories print to `stdout`. */
	typedef basic_logger<char> logger;
	/** @brief Single-threaded alias of `basic_logger<char>`. */
	typedef basic_logger<char, std::char_traits<char>, void> logger_st;

	template<>
	logger &detail::logger_base<logger, std::shared_mutex>::info();
	template<>
	logger &detail::logger_base<logger, std::shared_mutex>::warn();
	template<>
	logger &detail::logger_base<logger, std::shared_mutex>::error();
	template<>
	logger &detail::logger_base<logger, std::shared_mutex>::fatal();

	extern template class SEK_API_IMPORT basic_logger<char>;
	extern template class SEK_API_IMPORT basic_logger<char, std::char_traits<char>, void>;
}	 // namespace sek::engine