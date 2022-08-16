/*
 * Created by switchblade on 10/08/22
 */

#pragma once

#include <filesystem>
#include <string>

#include "define.h"
#include "ebo_base_helper.hpp"

namespace sek
{
	/** @brief Structure used to represent a platform-independent URI.
	 * @note URIs are always stored using native 8-bit `char` encoding. */
	class uri
	{
	public:
		typedef std::string string_type;

		typedef typename string_type::value_type value_type;
		typedef typename string_type::const_pointer pointer;
		typedef typename string_type::const_pointer const_pointer;
		typedef typename string_type::const_reference reference;
		typedef typename string_type::const_reference const_reference;

		typedef std::size_t size_type;
		typedef std::ptrdiff_t difference_type;

	private:
		using sv_type = std::basic_string_view<value_type>;

		struct element
		{
			size_type offset;
			size_type size;
		};

		class list_handle
		{
			friend class uri;

			struct list_header;

			[[nodiscard]] static list_header *alloc_list(size_type capacity);
			static void dealloc_list(list_header *list);

			constexpr static size_type min_capacity = 7; /* 1 schema + 3 authority + 1 path + 1 query + 1 fragment */

		public:
			typedef element value_type;
			typedef element *iterator;
			typedef element *pointer;
			typedef element &reference;
			typedef typename uri::size_type size_type;
			typedef typename uri::difference_type difference_type;

		public:
			constexpr list_handle() noexcept = default;

			SEK_API list_handle(const list_handle &);
			SEK_API list_handle &operator=(const list_handle &);

			constexpr list_handle(list_handle &&other) noexcept : m_data(other.m_data) { other.m_data = 0; }
			constexpr list_handle &operator=(list_handle &&other) noexcept
			{
				swap(other);
				return *this;
			}

			~list_handle();

			[[nodiscard]] inline iterator begin() const noexcept;
			[[nodiscard]] inline iterator cbegin() const noexcept;
			[[nodiscard]] inline iterator end() const noexcept;
			[[nodiscard]] inline iterator cend() const noexcept;

			[[nodiscard]] inline reference front() const noexcept;
			[[nodiscard]] inline reference back() const noexcept;

			[[nodiscard]] inline bool empty() const noexcept;

			[[nodiscard]] inline size_type size() const noexcept;
			[[nodiscard]] inline size_type capacity() const noexcept;

			inline void clear() const noexcept;

			inline iterator push_back(const element &);
			inline iterator erase(iterator) noexcept;

			constexpr void swap(list_handle &other) noexcept { std::swap(m_data, other.m_data); }

		private:
			list_header *m_data = nullptr;
		};

		constexpr static value_type path_separator[] = {'/', '\\'};
		constexpr static value_type scheme_postfix = ':';
		constexpr static value_type auth_prefix[] = {'/', '/'};
		constexpr static value_type user_postfix = '@';
		constexpr static value_type port_prefix = ':';
		constexpr static value_type query_prefix = '?';
		constexpr static value_type frag_prefix = '#';

	public:
		/** Initializes an empty URI. */
		constexpr uri() noexcept = default;

		/** Checks if the URI is empty (only contains a 0-length path). */
		[[nodiscard]] constexpr bool empty() const noexcept { return m_data.empty(); }

		/** Returns pointer to the data of the URI's string. */
		[[nodiscard]] constexpr pointer data() const noexcept { return m_data.data(); }
		/** @copydoc data */
		[[nodiscard]] constexpr pointer c_str() const noexcept { return data(); }

		/** Returns reference to URI's string. */
		[[nodiscard]] constexpr const string_type &string() const noexcept { return m_data; }
		/** Returns copy of the URI's string. */
		[[nodiscard]] constexpr operator string_type() const noexcept { return string(); }

		/** Checks if the URI has a scheme. */
		[[nodiscard]] SEK_API bool has_scheme() const noexcept;
		/** Checks if the URI has an authority. */
		[[nodiscard]] SEK_API bool has_authority() const noexcept;
		/** Checks if the URI has a userinfo. */
		[[nodiscard]] SEK_API bool has_userinfo() const noexcept;
		/** Checks if the URI has a host. */
		[[nodiscard]] SEK_API bool has_host() const noexcept;
		/** Checks if the URI has a port. */
		[[nodiscard]] SEK_API bool has_port() const noexcept;
		/** Checks if the URI has a pathinfo. */
		[[nodiscard]] SEK_API bool has_pathinfo() const noexcept;
		/** Checks if the URI has a query. */
		[[nodiscard]] SEK_API bool has_query() const noexcept;
		/** Checks if the URI has a fragment. */
		[[nodiscard]] SEK_API bool has_fragment() const noexcept;

		/** Checks if the URI refers to a local file (uses the `file` scheme). */
		[[nodiscard]] SEK_API bool is_local() const noexcept;
		/** Checks if the URI is "clean" (does not contain a query). */
		[[nodiscard]] bool is_clean() const noexcept { return !has_query(); }

		/** Returns a new URI consisting only of the scheme of this URI. */
		[[nodiscard]] SEK_API uri scheme() const;
		/** @brief Updates scheme of the URI.
		 * @param scheme URI containing the new scheme.
		 * @return Reference to `this`.
		 * @note If the new scheme is empty, clears the scheme. */
		SEK_API uri &scheme(const uri &scheme);
		/** @copybrief scheme
		 * @param scheme New scheme of the URI.
		 * @return Reference to `this`.
		 * @note If the new scheme is empty, clears the scheme. */
		SEK_API uri &scheme(std::string_view scheme);

		/** Returns a new URI consisting only of the authority of this URI. */
		[[nodiscard]] SEK_API uri authority() const;
		/** @brief Updates authority of the URI.
		 * @param authority URI containing the new authority.
		 * @return Reference to `this`.
		 * @note If the new authority is empty, clears the authority. */
		SEK_API uri &authority(const uri &authority);
		/** @copybrief authority
		 * @param authority New authority of the URI.
		 * @return Reference to `this`.
		 * @note If the new authority is empty, clears the authority. */
		SEK_API uri &authority(std::string_view authority);

		/** Returns a new URI consisting only of the host of this URI. */
		[[nodiscard]] SEK_API uri host() const;

		/** Returns a new URI consisting only of the path of this URI. */
		[[nodiscard]] SEK_API uri path() const;
		/** Returns an `std::filesystem::path` representation of the path of the URI. */
		[[nodiscard]] SEK_API std::filesystem::path fs_path() const;
		/** If the URI is local, returns it's path, otherwise returns an empty path. */
		[[nodiscard]] std::filesystem::path local_path() const
		{
			return is_local() ? fs_path() : std::filesystem::path{};
		}
		/** @brief Updates path of the URI.
		 * @param authority URI containing the new path.
		 * @return Reference to `this`.
		 * @note If the new path is empty, clears the path. */
		SEK_API uri &path(const uri &path);
		/** @copybrief path
		 * @param authority New path of the URI.
		 * @return Reference to `this`.
		 * @note If the new path is empty, clears the path. */
		SEK_API uri &path(std::string_view path);
		/** @copydoc path */
		SEK_API uri &path(const std::filesystem::path &path);

		/** Returns a new URI consisting only of the query of this URI. */
		[[nodiscard]] SEK_API uri query() const;
		/** @brief Updates query of the URI.
		 * @param query URI containing the new query.
		 * @return Reference to `this`.
		 * @note If the new query is empty, clears the query. */
		SEK_API uri &query(const uri &query);
		/** @copybrief query
		 * @param query New query of the URI.
		 * @return Reference to `this`.
		 * @note If the new query is empty, clears the query. */
		SEK_API uri &query(std::string_view query);

		/** Returns a new URI consisting only of the fragment of this URI. */
		[[nodiscard]] SEK_API uri fragment() const;
		/** @brief Updates fragment of the URI.
		 * @param fragment URI containing the new fragment.
		 * @return Reference to `this`.
		 * @note If the new fragment is empty, clears the fragment. */
		SEK_API uri &fragment(const uri &fragment);
		/** @copybrief fragment
		 * @param fragment New fragment of the URI.
		 * @return Reference to `this`.
		 * @note If the new fragment is empty, clears the fragment. */
		SEK_API uri &fragment(std::string_view fragment);

		/** @brief Appends `other` to `this`.
		 *
		 * @param other URI to append to `this`.
		 *
		 * Appends the path component of `other` to path of `this`.
		 * If `other` contains a query and/or fragment, replaces those components in `this`.
		 * If `this` does not have a scheme, userinfo, or port, inherits these components from `other`. */
		uri &operator/=(const uri &other) { return append(other); }
		/** @copydoc operator/=
		 * @param q_sep Optional query separator. If set to non-null character and both `this` and `other` have
		 * non-empty queries, they are concatenated instead using the separator. */
		SEK_API uri &append(const uri &other, value_type q_sep = '\0');
		/** @brief Creates a copy of `other` appended to `this`.
		 * @copydetails operator/= */
		[[nodiscard]] uri operator/(const uri &other) const
		{
			auto result = *this;
			result.append(other);
			return result;
		}

	private:
		[[nodiscard]] constexpr sv_type element_view(const element &e) const noexcept
		{
			return {m_data.data() + e.offset, e.size};
		}

		[[nodiscard]] constexpr auto skip_scheme(auto iter) const noexcept;
		[[nodiscard]] constexpr auto skip_userinfo(auto iter) const noexcept;
		[[nodiscard]] constexpr auto skip_port(auto iter) const noexcept;
		[[nodiscard]] constexpr auto skip_fragment(auto iter) const noexcept;

		[[nodiscard]] inline list_handle::iterator find_scheme() const noexcept;
		[[nodiscard]] inline list_handle::iterator find_userinfo() const noexcept;
		[[nodiscard]] inline list_handle::iterator find_host() const noexcept;
		[[nodiscard]] inline list_handle::iterator find_port() const noexcept;

		[[nodiscard]] inline list_handle::iterator find_query() const noexcept;
		[[nodiscard]] inline list_handle::iterator find_fragment() const noexcept;

		[[nodiscard]] inline std::pair<list_handle::iterator, list_handle::iterator> find_auth() const noexcept;
		[[nodiscard]] inline std::pair<list_handle::iterator, list_handle::iterator> find_path() const noexcept;

		inline void offset_elements(difference_type off) noexcept;

		string_type m_data;
		list_handle m_list;
	};
}	 // namespace sek