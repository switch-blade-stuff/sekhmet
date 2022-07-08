/*
 * Created by switchblade on 07/07/22
 */

#pragma once

#include <string>

#include "sekhmet/detail/access_guard.hpp"
#include "sekhmet/detail/define.h"
#include "sekhmet/detail/service.hpp"
#include "sekhmet/detail/type_info.hpp"
#include "sekhmet/system/native_file.hpp"

#include <shared_mutex>

namespace sek::engine
{
	class config_registry;

	namespace detail
	{
		using namespace sek::detail;

		using config_guard = access_guard<config_registry, std::shared_mutex>;
	}	 // namespace detail

	/** @brief Runtime exception thrown by the config registry. */
	class SEK_API config_error : public std::runtime_error
	{
	public:
		config_error() : std::runtime_error("Unknown config registry error") {}
		explicit config_error(std::string &&msg) : std::runtime_error(std::move(msg)) {}
		explicit config_error(const std::string &msg) : std::runtime_error(msg) {}
		explicit config_error(const char *msg) : std::runtime_error(msg) {}
		~config_error() override;
	};

	/** @brief Path-like structure used to uniquely identify a config registry entry.
	 *
	 * Config paths consist of entry names separated by forward slashes `/`. The first entry is the category entry.
	 * Path entry names can only contain alphanumeric characters, as well as any of the following
	 * special characters: `_`, `-`, `.`.
	 *
	 * Paths are case-sensitive and are always absolute (since there is no "current" config path).
	 * Sequential slashes (ex. `///`) are concatenated. */
	class cfg_path
	{
	private:
		struct slice_t
		{
			std::size_t first;
			std::size_t last;
		};

	public:
		/** Initializes an empty config path. */
		constexpr cfg_path() noexcept = default;

		/** Initializes a config path from a string view.
		 * @throw config_error If the specified path is invalid. */
		explicit cfg_path(std::string_view str) : m_path(str) { parse(); }
		/** Initializes a config path from a string.
		 * @throw config_error If the specified path is invalid. */
		explicit cfg_path(std::string &&str) : m_path(std::move(str)) { parse(); }
		/** @copydoc cfg_path */
		explicit cfg_path(const std::string &str) : m_path(str) { parse(); }

		/** Initializes a config path from a C-style string.
		 * @throw config_error If the specified path is invalid. */
		explicit cfg_path(const char *str) : cfg_path(std::string_view{str}) {}
		/** Initializes a config path from a character array.
		 * @throw config_error If the specified path is invalid. */
		cfg_path(const char *str, std::size_t n) : cfg_path(std::string_view{str, n}) {}

		/** Returns reference to the underlying path string. */
		[[nodiscard]] constexpr auto &string() noexcept { return m_path; }
		/** @copydoc string */
		[[nodiscard]] constexpr auto &string() const noexcept { return m_path; }

		/** Returns the category component of the path. */
		[[nodiscard]] cfg_path category() const
		{
			auto ptr = &m_slices.back();
			return to_component(ptr, ptr + 1);
		}
		/** Returns the last entry name of the path (ex. for path 'graphics/quality' will return `quality`). */
		[[nodiscard]] cfg_path entry_name() const
		{
			auto ptr = &m_slices.back();
			return to_component(ptr, ptr + 1);
		}
		/** Returns the entry path without the category component. */
		[[nodiscard]] cfg_path entry_path() const
		{
			return to_component(m_slices.data() + 1, m_slices.data() + m_slices.size());
		}

		/** Appends another path to this path.
		 * @return Reference to this path.
		 * @throw config_error If the resulting path is invalid. */
		cfg_path &append(const cfg_path &path)
		{
			m_path.append(path.m_path);
			parse();
			return *this;
		}
		/** @copydoc append */
		cfg_path &operator/=(const cfg_path &path) { return append(path); }
		/** Returns a path produced from concatenating two paths.
		 * @return Concatenated path.
		 * @throw config_error If the resulting path is invalid. */
		[[nodiscard]] cfg_path operator/(const cfg_path &path) const
		{
			auto tmp = *this;
			tmp /= path;
			return tmp;
		}

		/** Appends a string to the path.
		 * @return Reference to this path.
		 * @throw config_error If the resulting path is invalid. */
		cfg_path &append(std::string_view str)
		{
			m_path.append(str);
			parse();
			return *this;
		}
		/** @copydoc append */
		cfg_path &append(const std::string &str)
		{
			m_path.append(str);
			parse();
			return *this;
		}
		/** @copydoc append */
		cfg_path &append(const char *str) { return append(std::string_view{str}); }
		/** @copydoc append */
		cfg_path &append(const char *str, std::size_t n) { return append(std::string_view{str, n}); }
		/** @copydoc append */
		cfg_path &operator/=(std::string_view str) { return append(std::string_view{str}); }
		/** @copydoc append */
		cfg_path &operator/=(const std::string &str) { return append(std::string_view{str}); }
		/** @copydoc append */
		cfg_path &operator/=(const char *str) { return append(std::string_view{str}); }

		/** Returns a path produced from concatenating a path with a string.
		 * @return Concatenated path.
		 * @throw config_error If the resulting path is invalid. */
		[[nodiscard]] cfg_path operator/(std::string_view str) const
		{
			auto tmp = *this;
			tmp /= str;
			return tmp;
		}
		/** @copydoc operator/ */
		[[nodiscard]] cfg_path operator/(const std::string &str) const
		{
			auto tmp = *this;
			tmp /= str;
			return tmp;
		}
		/** @copydoc operator/ */
		[[nodiscard]] cfg_path operator/(const char *str) const
		{
			auto tmp = *this;
			tmp /= str;
			return tmp;
		}

		[[nodiscard]] constexpr auto operator<=>(const cfg_path &other) noexcept { return m_path <=> other.m_path; }
		[[nodiscard]] constexpr bool operator==(const cfg_path &other) noexcept { return m_path == other.m_path; }

		constexpr void swap(cfg_path &other) noexcept
		{
			using std::swap;
			swap(m_path, other.m_path);
			swap(m_slices, other.m_slices);
		}
		friend constexpr void swap(cfg_path &a, cfg_path &b) noexcept { a.swap(b); }

	private:
		[[nodiscard]] SEK_API cfg_path to_component(const slice_t *first, const slice_t *last) const;

		SEK_API void parse();

		/** String containing the full normalized path. */
		std::string m_path;
		/** Individual elements of the path. */
		std::vector<slice_t> m_slices;
	};

	/** @brief Service used to manage configuration entries.
	 *
	 * Engine configuration is stored as entries within the config registry.
	 * Every entry belongs to a category, and is created at plugin initialization time.
	 * Categories are de-serialized from individual Json files or loaded directly from Json node trees.
	 * When a new entry is added, it is de-serialized from the cached category tree. */
	class config_registry : public service<detail::config_guard>
	{
		friend detail::config_guard;

	public:
	};
}	 // namespace sek::engine

extern template class SEK_API_IMPORT sek::service<sek::engine::detail::config_guard>;