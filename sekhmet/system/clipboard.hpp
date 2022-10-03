//
// Created by switch_blade on 2022-10-02.
//

#pragma once

#include <concepts>
#include <variant>
#include <vector>

#include "../define.h"
#include <string_view>

namespace sek::detail
{
	struct native_clipboard_type;
}	 // namespace sek::detail

namespace sek
{
	/** @brief Handle used to reference a unique clipboard data type. */
	class SEK_API clipboard_type
	{
	public:
		enum class match_result : int
		{
			PARTIAL = -1,
			COMPLETE = 0,
			UNMATCHED = 1,
		};

	private:
		friend clipboard_type::match_result match(const clipboard_type &, const clipboard_type &) noexcept;

	public:
		/** Clipboard type used for text (`text/text`). */
		[[nodiscard]] static clipboard_type text();
		/** Clipboard type used for html (`text/html`). */
		[[nodiscard]] static clipboard_type html();
		/** Clipboard type used for files (platform-specific). */
		[[nodiscard]] static clipboard_type file();
		/** Clipboard type used for uri lists (`text/uri-list`). */
		[[nodiscard]] static clipboard_type uri_list();
		/** Clipboard type used for bitmap image data. */
		[[nodiscard]] static clipboard_type bitmap();

	private:
		using group_data = std::vector<clipboard_type>;
		using native_data = detail::native_clipboard_type;

	public:
		clipboard_type() = delete;

		clipboard_type(const clipboard_type &) = default;
		clipboard_type &operator=(const clipboard_type &) = default;
		clipboard_type(clipboard_type &&) noexcept = default;
		clipboard_type &operator=(clipboard_type &&) noexcept = default;

		/** Creates a custom clipboard type.
		 * @param type String identifying the clipboard type.
		 * @note While it is not required, it is recommended to use MIME-format types for the `type` string. */
		clipboard_type(std::string_view type) : m_data(std::in_place_type<native_data>, type) {}
		/** @copydoc clipboard_type */
		clipboard_type(const std::string &type) : clipboard_type(std::string_view{type}) {}

		// clang-format off
		/** @copydoc clipboard_type */
		template<typename S>
		clipboard_type(const S &type) requires std::constructible_from<std::string_view, const S &>
			: clipboard_type(std::string_view{type}) {}
		// clang-format on

		// clang-format off
		/** Creates a group type from a range of clipboard types.
		 * @param first Iterator to the first element of the group.
		 * @param last Sentinel for the `first` iterator. */
		template<std::forward_iterator I, std::sentinel_for<I> S>
		clipboard_type(I first, S last) : m_data(std::in_place_type<group_data>, first, last) {}
		/** Creates a group type from a range of clipboard types.
		 * @param t Range containing group elements. */
		 template<std::ranges::forward_range R>
		clipboard_type(const R &r) : clipboard_type(std::ranges::begin(r), std::ranges::end(r)) {}
		// clang-format on
		/** Creates a group type from an initializer list of clipboard types.
		 * @param il Initializer list containing group elements. */
		clipboard_type(std::initializer_list<clipboard_type> il) : clipboard_type(il.begin(), il.end()) {}

		/** Checks if the clipboard type is a group type. */
		[[nodiscard]] constexpr bool is_group() const noexcept { return holds_alternative<group_data>(m_data); }
		/** If the clipboard type is a group, returns a vector of the grouped clipboard types. Otherwise, returns an empty vector. */
		[[nodiscard]] group_data group() const { return is_group() ? get<1>(m_data) : group_data{}; }

		/** Returns a string name of the type. If the type is a group, returns
		 * a colon-separated list of types (ex. `text/text;text/html`). */
		[[nodiscard]] std::string name() const;

		void swap(clipboard_type &other) noexcept { m_data.swap(other.m_data); }
		friend void swap(clipboard_type &a, clipboard_type &b) noexcept { a.swap(b); }

	private:
		[[nodiscard]] match_result match_group(const clipboard_type &group) const noexcept;
		[[nodiscard]] match_result match(const clipboard_type &other) const noexcept;

		std::variant<native_data, group_data> m_data;
	};

	/** Compares `lhs` clipboard type with `rhs` and returns one of the following:
	 * <ul>
	 * <li>`match_result::COMPLETE` if `lhs` is equal to `rhs`.</li>
	 * <li>`match_result::UNMATCHED` if `lhs` does not intersect with `rhs`.</li>
	 * <li>`match_result::PARTIAL` if `lhs` intersects with `rhs` but is not equal to it.</li>
	 * </ul> */
	[[nodiscard]] clipboard_type::match_result match(const clipboard_type &lhs, const clipboard_type &rhs) noexcept
	{
		return lhs.match(rhs);
	}

	/** @brief Handle to a platform-specific clipboard interface. */
	class SEK_API clipboard;

	/* TODO: Implement clipboard as part of application. */
}	 // namespace sek