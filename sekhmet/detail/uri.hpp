/*
 * Created by switchblade on 10/08/22
 */

#pragma once

#include <string>

#include "define.h"
#include "ebo_base_helper.hpp"

namespace sek
{
	/** @brief Flags used to identify individual components of a URI. */
	enum class uri_component : int
	{
		/** Scheme of the URI (ex. `http`, `https`, `file`). */
		SCHEME = 0b0001'0000,
		/** Query of the URI (ex. `search=sometext` in `mydomain.com/?search=sometext`). */
		QUERY = 0b0010'0000,
		/** Fragment of the URI (ex. `element` in `mydomain.com/#element`). */
		FRAGMENT = 0b0100'0000,

		/** Mask bit common to all authority components. */
		AUTHORITY_MASK = 0b1000'0000,
		/** Mask bit common to all path components. */
		PATH_MASK = 0b1'0000'0000,

		/** Username of the URI authority (ex. `user` in `//user:passwd@mydomain.com:22`) */
		USERNAME = AUTHORITY_MASK | 0b0001,
		/** Password of the URI authority (ex. `passwd` in `//user:passwd@mydomain.com:22`) */
		PASSWORD = AUTHORITY_MASK | 0b0010,
		/** Userinfo of the URI authority (ex. `user:passwd` in `//user:passwd@mydomain.com:22`) */
		USERINFO = USERNAME | PASSWORD,
		/** Host of the URI authority (ex. `mydomain.com` in `//user:passwd@mydomain.com:22`) */
		HOST = AUTHORITY_MASK | 0b0100,
		/** Port of the URI authority (ex. `22` in `//user:passwd@mydomain.com:22`) */
		PORT = AUTHORITY_MASK | 0b1000,
		/** Full authority of the URI. */
		AUTHORITY = USERINFO | HOST | PORT,

		/** File name of the URI path (ex. `index.html` in `https://mydomain.com/index.html`). */
		FILE_NAME = PATH_MASK | 0b0000'0001,
		/** Full path of the URI. */
		PATH = PATH_MASK,
	};

	[[nodiscard]] constexpr uri_component operator~(uri_component lhs) noexcept
	{
		return static_cast<uri_component>(~static_cast<int>(lhs));
	}
	[[nodiscard]] constexpr uri_component operator|(uri_component lhs, uri_component rhs) noexcept
	{
		return static_cast<uri_component>(static_cast<int>(lhs) | static_cast<int>(rhs));
	}
	[[nodiscard]] constexpr uri_component operator&(uri_component lhs, uri_component rhs) noexcept
	{
		return static_cast<uri_component>(static_cast<int>(lhs) & static_cast<int>(rhs));
	}
	[[nodiscard]] constexpr uri_component operator^(uri_component lhs, uri_component rhs) noexcept
	{
		return static_cast<uri_component>(static_cast<int>(lhs) ^ static_cast<int>(rhs));
	}
	[[nodiscard]] constexpr uri_component &operator|=(uri_component &lhs, uri_component rhs) noexcept
	{
		lhs = lhs | rhs;
		return lhs;
	}
	[[nodiscard]] constexpr uri_component &operator&=(uri_component &lhs, uri_component rhs) noexcept
	{
		lhs = lhs & rhs;
		return lhs;
	}
	[[nodiscard]] constexpr uri_component &operator^=(uri_component &lhs, uri_component rhs) noexcept
	{
		lhs = lhs ^ rhs;
		return lhs;
	}

	/** @brief Flags used to identify URI formatting options. */
	enum class uri_format : int
	{
		/** Do not preform any encoding or decoding. Any percent-encoded sequences and non-encoded characters are left untouched. */
		NO_FORMAT = 0,

		/** Decodes all encoded sequences, regardless of component type. */
		DECODE_ALL = 0b01,
		/** Decodes most encoded sequences. Recommended formatting option for use when a human-readable
		 * representation of a URI is required. Decoded characters depend on component type. */
		DECODE_PRETTY = 0b10,
		/** Decodes all non-ASCII Unicode sequences from encoded UTF-8. */
		DECODE_UTF = 0b0100,
		/** Decodes all special encoded characters. Special characters depend on component type. */
		DECODE_SPECIAL = 0b1000,
		/** Decodes all encoded delimiters. Delimiter characters depend on component type. */
		DECODE_DELIMITERS = 0b01'0000,
		/** Decodes all encoded whitespace characters using the current locale. */
		DECODE_WHITESPACE = 0b10'0000,

		/** Encodes all non-ASCII Unicode sequences as encoded UTF-8. */
		ENCODE_UTF = 0x80 | DECODE_UTF,
		/** Encodes all special characters. Special characters depend on component type. */
		ENCODE_SPECIAL = 0x80 | DECODE_SPECIAL,
		/** Encodes all delimiters. Delimiter characters depend on component type. */
		ENCODE_DELIMITERS = 0x80 | DECODE_DELIMITERS,
		/** Encodes all whitespace characters using the current locale. */
		ENCODE_WHITESPACE = 0x80 | DECODE_WHITESPACE,
		/** Encodes all characters not allowed within a URI. Recommended formatting option for use when an ASCII-only
		 * text representation of a URI is required (ex. for network communication). */
		ENCODE_ALL = ENCODE_UTF | ENCODE_SPECIAL | ENCODE_WHITESPACE,
	};

	[[nodiscard]] constexpr uri_format operator~(uri_format lhs) noexcept
	{
		return static_cast<uri_format>(~static_cast<int>(lhs));
	}
	[[nodiscard]] constexpr uri_format operator|(uri_format lhs, uri_format rhs) noexcept
	{
		return static_cast<uri_format>(static_cast<int>(lhs) | static_cast<int>(rhs));
	}
	[[nodiscard]] constexpr uri_format operator&(uri_format lhs, uri_format rhs) noexcept
	{
		return static_cast<uri_format>(static_cast<int>(lhs) & static_cast<int>(rhs));
	}
	[[nodiscard]] constexpr uri_format operator^(uri_format lhs, uri_format rhs) noexcept
	{
		return static_cast<uri_format>(static_cast<int>(lhs) ^ static_cast<int>(rhs));
	}
	[[nodiscard]] constexpr uri_format &operator|=(uri_format &lhs, uri_format rhs) noexcept
	{
		lhs = lhs | rhs;
		return lhs;
	}
	[[nodiscard]] constexpr uri_format &operator&=(uri_format &lhs, uri_format rhs) noexcept
	{
		lhs = lhs & rhs;
		return lhs;
	}
	[[nodiscard]] constexpr uri_format &operator^=(uri_format &lhs, uri_format rhs) noexcept
	{
		lhs = lhs ^ rhs;
		return lhs;
	}

	/** @brief Structure used to represent a platform-independent URI.
	 *
	 * URIs conform to the `RFC 3986` (Uniform Resource Identifier: Generic Syntax) specification,
	 * the `RFC 3491` (Nameprep: A Stringprep Profile for Internationalized Domain Names (IDN)) case folding rules
	 * and partially the `RFC 1738` (Uniform Resource Locators) specification.
	 *
	 * String formatting (percent-encoding of escaped character sequences) of accessor member functions of the URI
	 * and it's components can be controlled with the `uri_format` flags. Different formatting may be desired
	 * for different applications. For example, human-readable representation of the URI may necessitate decoding of
	 * certain percent-encoded sequences.
	 *
	 * @note URIs are stored using native 8-bit signed (`char`) characters. */
	class uri
	{
	public:
		/** @brief Type of string used to store URI data. */
		typedef std::string string_type;
		/** @brief Type of string view used to reference sections of URI. */
		typedef std::string_view string_view_type;

		/** @brief Character type of the URI. */
		typedef typename string_type::value_type value_type;

		typedef std::size_t size_type;
		typedef std::ptrdiff_t difference_type;

		/** Encodes a host string using the ASCII-Compatible Encoding (ACE).
		 * @param str Unencoded string.
		 * @return String encoded in ACE. */
		[[nodiscard]] static SEK_API string_type encode_ace(string_view_type str);
		/** @copydoc encode_ace */
		[[nodiscard]] static SEK_API string_type encode_ace(const string_type &str);
		/** @copydoc encode_ace */
		[[nodiscard]] static SEK_API string_type encode_ace(string_type &&str);

		/** Decodes a host string encoded using the ASCII-Compatible Encoding (ACE).
		 * @param str String in ACE format.
		 * @return Decoded string. */
		[[nodiscard]] static SEK_API string_type decode_ace(string_view_type str);
		/** @copydoc decode_ace */
		[[nodiscard]] static SEK_API string_type decode_ace(const string_type &str);
		/** @copydoc decode_ace */
		[[nodiscard]] static SEK_API string_type decode_ace(string_type &&str);

		/** Returns a URI who's host is decoded from ASCII-Compatible Encoding (ACE).
		 * Equivalent to initializing a URI from `str` and replacing it's host with a decoded string. */
		[[nodiscard]] static SEK_API uri from_ace(string_view_type str);
		/** @copydoc from_ace */
		[[nodiscard]] static SEK_API uri from_ace(const string_type &str);
		/** @copydoc from_ace */
		[[nodiscard]] static SEK_API uri from_ace(string_type &&str);

		/** Produced a URI from a local filesystem path (implicitly using the `file` scheme).
		 * @note Validity of the local path is not verified. */
		[[nodiscard]] static SEK_API uri from_local(string_view_type path);
		/** @copydoc from_local */
		[[nodiscard]] static SEK_API uri from_local(const string_type &str);
		/** @copydoc from_local */
		[[nodiscard]] static SEK_API uri from_local(string_type &&str);

	private:
		struct element
		{
			size_type offset;
			size_type size;
		};

		class data_handle
		{
			friend class uri;

			struct impl;

			constexpr static size_type min_capacity = 2;

		public:
			constexpr data_handle() noexcept = default;

			SEK_API data_handle(const data_handle &);
			SEK_API data_handle &operator=(const data_handle &);
			SEK_API ~data_handle();

			constexpr data_handle(data_handle &&other) noexcept : m_ptr(other.m_ptr) { other.m_ptr = nullptr; }
			constexpr data_handle &operator=(data_handle &&other) noexcept
			{
				swap(other);
				return *this;
			}

			constexpr impl *get() const noexcept { return m_ptr; }
			constexpr impl *operator->() const noexcept { return get(); }

			constexpr void swap(data_handle &other) noexcept { std::swap(m_ptr, other.m_ptr); }

		private:
			impl *m_ptr = nullptr;
		};

	public:
		/** Initializes an empty URI. */
		constexpr uri() noexcept = default;

		/** @brief Initializes a URI from a string.
		 *
		 * Input string is parsed in conformance to the `RFC 3986` (Uniform Resource Identifier: Generic Syntax)
		 * specification and formatted to conform to the case folding rules defined in
		 * `RFC 3491` (Nameprep: A Stringprep Profile for Internationalized Domain Names (IDN)).
		 *
		 * @param str String containing the URI.
		 * @note URI path will be formatted to conform to the `RFC 3491`. */
		explicit uri(string_view_type str) : m_value(str) { parse_components(); }
		/** @copydoc uri */
		explicit uri(const string_type &str) : m_value(str) { parse_components(); }
		/** @copydoc uri */
		explicit uri(string_type &&str) : m_value(std::move(str)) { parse_components(); }

		/** Assigns the URI from a string.
		 * @param str String containing the URI.
		 * @return Reference to this URI. */
		uri &operator=(string_view_type str)
		{
			m_value = str;
			parse_components();
			return *this;
		}
		/** @copydoc operator= */
		uri &operator=(const string_type &str)
		{
			m_value = str;
			parse_components();
			return *this;
		}
		/** @copydoc operator= */
		uri &operator=(string_type &&str)
		{
			m_value = std::move(str);
			parse_components();
			return *this;
		}
		/** @copydoc operator= */
		uri &assign(string_view_type str) { return operator=(str); }
		/** @copydoc operator= */
		uri &assign(const string_type &str) { return operator=(str); }
		/** @copydoc operator= */
		uri &assign(string_type &&str) { return operator=(std::move(str)); }

		/** @brief Initializes a URI from components of another URI.
		 * @param other Uri to copy components from. */
		uri(const uri &other) = default;
		/** @copydoc uri
		 * @param mask Mask specifying which components to copy. */
		SEK_API uri(const uri &other, uri_component mask);

		/** @brief Assigns the URI from another URI.
		 * @param other Uri to copy components from.
		 * @return Reference to this URI. */
		uri &operator=(const uri &other) = default;
		/** @copydoc operator= */
		uri &assign(const uri &other) { return operator=(other); }
		/** @copydoc operator=
		 * @param mask Mask specifying which components to copy. */
		SEK_API uri &assign(const uri &other, uri_component mask);

		/** @copybrief uri
		 * @param other Uri to move components from. */
		constexpr uri(uri &&other) noexcept = default;
		/** @copydoc uri
		 * @param mask Mask specifying which components to move.
		 * @note Host is implied if any authority components are present. */
		SEK_API uri(uri &&other, uri_component mask);

		/** @copybrief operator=
		 * @param other Uri to move components from.
		 * @return Reference to this URI. */
		SEK_API uri &operator=(uri &&other) noexcept;
		/** @copydoc operator= */
		uri &assign(uri &&other) noexcept { return operator=(std::move(other)); }
		/** @copydoc operator=
		 * @param mask Mask specifying which components to move.
		 * @note Host is implied if any authority components are present. */
		SEK_API uri &assign(uri &&other, uri_component mask);

		/** Checks if the URI is empty. */
		[[nodiscard]] constexpr bool empty() const noexcept { return m_value.empty(); }

		/** Returns pointer to the data of the URI's string. */
		[[nodiscard]] constexpr const value_type *data() const noexcept { return m_value.data(); }
		/** @copydoc data */
		[[nodiscard]] constexpr const value_type *c_str() const noexcept { return data(); }

		/** Returns reference to URI's string. */
		[[nodiscard]] constexpr const string_type &string() const noexcept { return m_value; }
		/** Returns copy of the URI's string. */
		[[nodiscard]] constexpr operator string_type() const { return string(); }

		/** Checks if the URI has the components specified by a mask. */
		[[nodiscard]] SEK_API bool has_components(uri_component mask) const noexcept;

		/** Checks if the URI has a scheme. Equivalent to `has_components(uri_component::SCHEME)`. */
		[[nodiscard]] bool has_scheme() const noexcept { return has_components(uri_component::SCHEME); }

		/** Checks if the URI has an authority. Equivalent to `has_components(uri_component::AUTHORITY)`. */
		[[nodiscard]] bool has_authority() const noexcept { return has_components(uri_component::AUTHORITY); }
		/** Checks if the URI has a username. Equivalent to `has_components(uri_component::USERNAME)`. */
		[[nodiscard]] bool has_username() const noexcept { return has_components(uri_component::USERNAME); }
		/** Checks if the URI has a password. Equivalent to `has_components(uri_component::PASSWORD)`. */
		[[nodiscard]] bool has_password() const noexcept { return has_components(uri_component::PASSWORD); }
		/** Checks if the URI has a userinfo (username & password).
		 * Equivalent to `has_components(uri_component::USERINFO)`. */
		[[nodiscard]] bool has_userinfo() const noexcept { return has_components(uri_component::USERINFO); }
		/** Checks if the URI has a host. Equivalent to `has_components(uri_component::HOST)`. */
		[[nodiscard]] bool has_host() const noexcept { return has_components(uri_component::HOST); }
		/** Checks if the URI has a port. Equivalent to `has_components(uri_component::PORT)`. */
		[[nodiscard]] bool has_port() const noexcept { return has_components(uri_component::PORT); }

		/** Checks if the URI has a non-empty path. Equivalent to `has_components(uri_component::PATH)`. */
		[[nodiscard]] bool has_path() const noexcept { return has_components(uri_component::PATH); }
		/** Checks if the URI path has a filename. Equivalent to `has_components(uri_component::FILE_NAME)`. */
		[[nodiscard]] bool has_filename() const noexcept { return has_components(uri_component::FILE_NAME); }

		/** Checks if the URI has a query. Equivalent to `has_components(uri_component::QUERY)`. */
		[[nodiscard]] bool has_query() const noexcept { return has_components(uri_component::QUERY); }
		/** Checks if the URI has a fragment. Equivalent to `has_components(uri_component::FRAGMENT)`. */
		[[nodiscard]] bool has_fragment() const noexcept { return has_components(uri_component::FRAGMENT); }

		/** Checks if the URI refers to a local file (uses the `file` scheme). Equivalent to `scheme() == "file"` */
		[[nodiscard]] SEK_API bool is_local() const noexcept;

		/** Checks if the URI is "clean" (has no query). Equivalent to `!has_query()`. */
		[[nodiscard]] bool is_clean() const noexcept { return !has_query(); }
		/** Checks if the URI is relative (has no scheme). Equivalent to `!has_scheme()`. */
		[[nodiscard]] bool is_relative() const noexcept { return !has_scheme(); }

		/** Returns a formatted copy of the selected components of the URI.
		 * @param mask Component type mask used to select target components.
		 * @param format Formatting options for the resulting string. */
		[[nodiscard]] SEK_API string_type components(uri_component mask, uri_format format = uri_format::NO_FORMAT) const;

		/** Returns a formatted copy of the scheme of the URI.
		 * @param format Formatting options for the resulting string. */
		[[nodiscard]] SEK_API string_type scheme(uri_format format) const;
		/** Returns a string view to the scheme of the URI. */
		[[nodiscard]] SEK_API string_view_type scheme() const noexcept;

		/** Returns a formatted copy of the authority of the URI (i.e. `[username[:password]@]host[:port]`).
		 * @param format Formatting options for the resulting string. */
		[[nodiscard]] SEK_API string_type authority(uri_format format) const;
		/** Returns a string view to the authority of the URI (i.e. `[username[:password]@]host[:port]`). */
		[[nodiscard]] SEK_API string_view_type authority() const noexcept;

		/** Returns a formatted copy of the userinfo of the URI (i.e. `username[:password]`).
		 * @param format Formatting options for the resulting string. */
		[[nodiscard]] SEK_API string_type userinfo(uri_format format) const;
		/** Returns a string view to the userinfo of the URI (i.e. `username[:password]`). */
		[[nodiscard]] SEK_API string_view_type userinfo() const noexcept;

		/** Returns a formatted copy of the username of the URI.
		 * @param format Formatting options for the resulting string. */
		[[nodiscard]] SEK_API string_type username(uri_format format) const;
		/** Returns a string view to the username of the URI. */
		[[nodiscard]] SEK_API string_view_type username() const noexcept;

		/** Returns a formatted copy of the password of the URI.
		 * @param format Formatting options for the resulting string. */
		[[nodiscard]] SEK_API string_type password(uri_format format) const;
		/** Returns a string view to the password of the URI. */
		[[nodiscard]] SEK_API string_view_type password() const noexcept;

		/** Returns a formatted copy of the host of the URI.
		 * @param format Formatting options for the resulting string.
		 * @note If encoding of Unicode sequences is required, the ASCII-Compatible Encoding (ACE) is used. */
		[[nodiscard]] SEK_API string_type host(uri_format format) const;
		/** Returns a string view to the host of the URI. */
		[[nodiscard]] SEK_API string_view_type host() const noexcept;

		/** Returns a formatted copy of the port of the URI.
		 * @param format Formatting options for the resulting string. */
		[[nodiscard]] SEK_API string_type port(uri_format format) const;
		/** Returns a string view to the port of the URI. */
		[[nodiscard]] SEK_API string_view_type port() const noexcept;

		/** Returns a formatted copy of the path of the URI.
		 * @param format Formatting options for the resulting string. */
		[[nodiscard]] SEK_API string_type path(uri_format format) const;
		/** Returns a string view to the path of the URI. */
		[[nodiscard]] SEK_API string_view_type path() const noexcept;

		/** Returns a formatted copy of the filename of the URI.
		 * @param format Formatting options for the resulting string. */
		[[nodiscard]] SEK_API string_type filename(uri_format format) const;
		/** Returns a string view to the filename of the URI. */
		[[nodiscard]] SEK_API string_view_type filename() const noexcept;

		/** Returns a formatted copy of the query of the URI.
		 * @param format Formatting options for the resulting string. */
		[[nodiscard]] SEK_API string_type query(uri_format format) const;
		/** Returns a string view to the query of the URI. */
		[[nodiscard]] SEK_API string_view_type query() const noexcept;

		/** Returns a formatted copy of the fragment of the URI.
		 * @param format Formatting options for the resulting string. */
		[[nodiscard]] SEK_API string_type fragment(uri_format format) const;
		/** Returns a string view to the fragment of the URI. */
		[[nodiscard]] SEK_API string_view_type fragment() const noexcept;

		/** Replaces scheme of the URI with that of `other`.
		 * @param other URI containing the new scheme.
		 * @return Reference to `this`. */
		uri &set_scheme(const uri &other) { return set_scheme(other.scheme()); }
		/** Replaces scheme of the URI.
		 * @param scheme New scheme of the URI.
		 * @return Reference to `this`. */
		SEK_API uri &set_scheme(string_view_type scheme);

		/** Replaces username of the URI with that of `other`.
		 * @param other URI containing the new username.
		 * @return Reference to `this`. */
		uri &set_username(const uri &other) { return set_username(other.username()); }
		/** Replaces username of the URI.
		 * @param username New username of the URI.
		 * @return Reference to `this`. */
		SEK_API uri &set_username(string_view_type username);

		/** Replaces password of the URI with that of `other`.
		 * @param other URI containing the new password.
		 * @return Reference to `this`. */
		uri &set_password(const uri &other) { return set_password(other.password()); }
		/** Replaces password of the URI.
		 * @param password New password of the URI.
		 * @return Reference to `this`. */
		SEK_API uri &set_password(string_view_type password);

		/** Replaces userinfo of the URI with that of `other`.
		 * @param other URI containing the new userinfo.
		 * @return Reference to `this`. */
		uri &set_userinfo(const uri &other) { return set_userinfo(other.userinfo()); }
		/** Replaces userinfo of the URI.
		 * @param userinfo New userinfo of the URI.
		 * @return Reference to `this`. */
		SEK_API uri &set_userinfo(string_view_type userinfo);

		/** Replaces host of the URI with that of `other`.
		 * @param other URI containing the new host.
		 * @return Reference to `this`. */
		uri &set_host(const uri &other) { return set_host(other.host()); }
		/** Replaces host of the URI.
		 * @param host New host of the URI.
		 * @return Reference to `this`. */
		SEK_API uri &set_host(string_view_type host);

		/** Replaces port of the URI with that of `other`.
		 * @param other URI containing the new port.
		 * @return Reference to `this`. */
		uri &set_port(const uri &other) { return set_port(other.port()); }
		/** Replaces port of the URI.
		 * @param port New port of the URI.
		 * @return Reference to `this`. */
		SEK_API uri &set_port(string_view_type port);

		/** Replaces authority of the URI with that of `other`.
		 * @param other URI containing the new authority.
		 * @return Reference to `this`. */
		uri &set_authority(const uri &other) { return set_authority(other.authority()); }
		/** Replaces authority of the URI.
		 * @param authority New authority of the URI.
		 * @return Reference to `this`. */
		SEK_API uri &set_authority(string_view_type authority);

		/** Replaces path of the URI.
		 * @param path New path of the URI.
		 * @return Reference to `this`.
		 * @note URI path will be normalized. */
		SEK_API uri &set_path(string_view_type path);
		/** Replaces path of the URI with that of `other`.
		 * @param other URI containing the new path.
		 * @return Reference to `this`. */
		uri &set_path(const uri &other) { return set_path(other.path()); }

		/** Replaces query of the URI with that of `other`.
		 * @param other URI containing the new query.
		 * @return Reference to `this`. */
		uri &set_query(const uri &other) { return set_query(other.query()); }
		/** Replaces query of the URI.
		 * @param query New query of the URI.
		 * @return Reference to `this`. */
		SEK_API uri &set_query(string_view_type query);
		/** Appends query of the URI with that of `other`.
		 * @param query Query to append.
		 * @param sep Separator character used for the query. May be one of the following: `&`, `;`. Default is `&`.
		 * @return Reference to `this`. */
		SEK_API uri &append_query(string_view_type query, value_type sep = '&');

		/** Replaces fragment of the URI with that of `other`.
		 * @param other URI containing the new fragment.
		 * @return Reference to `this`. */
		uri &set_fragment(const uri &other) { return set_fragment(other.fragment()); }
		/** Replaces fragment of the URI.
		 * @param fragment New fragment of the URI.
		 * @return Reference to `this`. */
		SEK_API uri &set_fragment(string_view_type fragment);

		constexpr void swap(uri &other) noexcept
		{
			m_value.swap(other.m_value);
			m_data.swap(other.m_data);
		}
		friend constexpr void swap(uri &a, uri &b) noexcept { a.swap(b); }

	private:
		SEK_API void parse_components();

		string_type m_value;
		data_handle m_data;
	};

	/** Returns a normalized copy of the URI. That is, a URI who's path does not contain any relative path traversal (`.` and `..`). */
	[[nodiscard]] SEK_API uri normalize(const uri &value);

	/** @brief Status code indicating whether a URI is valid or specifying an error. */
	enum class uri_status
	{
		/** URI is conforming to the `RFC 3986` specification. */
		VALID_URI = 0,
	};

	/** Validates a URI and returns a status code indicating whether it is valid (or the reason why it is not). */
	SEK_API uri_status validate(const uri &value) noexcept;
}	 // namespace sek