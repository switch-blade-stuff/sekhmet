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
 * Created by switchblade on 2022-03-13
 */

#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <string>

#include "define.h"
#include "hash.hpp"

namespace sek
{
	/** @brief UUID Version 4 Variant 1. */
	class uuid
	{
		friend constexpr hash_t hash(const uuid &) noexcept;

	public:
		/** @brief Parent for UUID generators. */
		struct generator
		{
			/** Initializes the passed UUID. */
			virtual void operator()(uuid &) const = 0;
			/** Returns a generated UUID instance. */
			uuid operator()() const
			{
				uuid result;
				operator()(result);
				return result;
			}
		};

		/** @brief UUID generator used to generate a random (version 4 variant 1) UUID.
		 * @note Seed is based on OS-provided entropy. */
		struct version4_t final : public generator
		{
			SEK_API void operator()(uuid &) const noexcept final;
		};

		constexpr static version4_t version4 = {};

	private:
		template<typename C>
		constexpr static int parse_digit(C c)
		{
			// clang-format off
			return (c >= '0' && c <= '9') ? static_cast<int>(c - '0') :
				   (c >= 'A' && c <= 'F') ? static_cast<int>(c - 'A' + 10) :
				   (c >= 'a' && c <= 'f') ? static_cast<int>(c - 'a' + 10) : throw std::runtime_error("Invalid UUID string");
			// clang-format on
		}

	public:
		/** Initializes a nil UUID. */
		constexpr uuid() noexcept = default;

		/** Initializes UUID using the specified generator. */
		constexpr explicit uuid(const generator &gen) noexcept { gen(*this); }

		/** Initializes a UUID from a character range. */
		template<std::forward_iterator Iter>
		constexpr explicit uuid(Iter first, Iter last)
		{
			parse_string(first, last);
		}
		/** Initializes a UUID from a character range. */
		template<std::ranges::forward_range R>
		constexpr explicit uuid(const R &str) : uuid(std::ranges::begin(str), std::ranges::end(str))
		{
		}
		/** Initializes a UUID from a character array. */
		template<typename C, std::size_t N>
		constexpr explicit uuid(const C (&str)[N]) noexcept : uuid(std::begin(str), std::end(str))
		{
		}
		/** Initializes a UUID from a byte array. */
		constexpr explicit uuid(const std::byte (&data)[16]) noexcept { std::copy_n(data, 16, bytes); }

		/** Converts the UUID to string.
		 * @tparam C Character type of the output sequence.
		 * @tparam Traits Character traits of `C`.
		 * @param upper If set to `true`, hex digits would be written using uppercase letters. */
		template<typename C = char, typename Traits = std::char_traits<C>>
		[[nodiscard]] constexpr std::basic_string<C, Traits> to_string(bool upper = false) const
		{
			std::basic_string<C, Traits> result(36, '\0');
			to_string(result.begin(), upper);
			return result;
		}
		/** Writes 36 characters of UUID string representation to the output iterator.
		 * @tparam C Character type of the output sequence.
		 * @param out Iterator to write the characters to.
		 * @param upper If set to `true`, hex digits would be written using uppercase letters.
		 * @note Output must have space for 36 characters. */
		template<typename C, std::output_iterator<C> Iter>
		constexpr void to_string(Iter out, bool upper = false) const
		{
			write_string<C>(out, upper);
		}
		/** Writes 36 characters of UUID string representation to the output iterator.
		 * @param out Iterator to write the characters to.
		 * @param upper If set to `true`, hex digits would be written using uppercase letters.
		 * @note Output must have space for 36 characters. */
		template<std::forward_iterator Iter>
		constexpr void to_string(Iter out, bool upper = false) const
		{
			return to_string<std::iter_value_t<Iter>, Iter>(out, upper);
		}

		[[nodiscard]] constexpr auto operator<=>(const uuid &) const noexcept = default;
		[[nodiscard]] constexpr bool operator==(const uuid &) const noexcept = default;

	private:
		template<typename Iter>
		constexpr void parse_string(Iter first, Iter last) noexcept
		{
			for (std::size_t i = 0; i < SEK_ARRAY_SIZE(bytes) * 2 && first != last; ++first)
			{
				const auto c = *first;
				if (c == '-') [[unlikely]]
					continue;
				auto idx = i++;
				bytes[idx / 2] |= static_cast<std::byte>(parse_digit(c) << (idx % 2 ? 0 : 4));
			}
		}
		template<typename C, typename Iter>
		constexpr void write_string(Iter out, bool upper) const noexcept
		{
			constexpr C alphabet_lower[] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'};
			constexpr C alphabet_upper[] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};

			const auto &alphabet = upper ? alphabet_upper : alphabet_lower;
			for (std::size_t i = 0; i < 16; i++)
			{
				if (i == 4 || i == 6 || i == 8 || i == 10) *out++ = '-';

				auto top = static_cast<std::uint8_t>(bytes[i]) >> 4;
				auto bottom = static_cast<std::uint8_t>(bytes[i]) & 0xf;

				*out++ = alphabet[top];
				*out++ = alphabet[bottom];
			}
		}

		alignas(std::uint64_t[2]) std::byte bytes[16] = {};
	};

	[[nodiscard]] constexpr hash_t hash(const uuid &id) noexcept { return fnv1a(id.bytes, SEK_ARRAY_SIZE(id.bytes)); }

	namespace literals
	{
		[[nodiscard]] constexpr uuid operator""_uuid(const char *str, std::size_t n) noexcept
		{
			return uuid{str, str + n};
		}
		[[nodiscard]] constexpr uuid operator""_uuid(const char8_t *str, std::size_t n) noexcept
		{
			return uuid{str, str + n};
		}
		[[nodiscard]] constexpr uuid operator""_uuid(const char16_t *str, std::size_t n) noexcept
		{
			return uuid{str, str + n};
		}
		[[nodiscard]] constexpr uuid operator""_uuid(const char32_t *str, std::size_t n) noexcept
		{
			return uuid{str, str + n};
		}
		[[nodiscard]] constexpr uuid operator""_uuid(const wchar_t *str, std::size_t n) noexcept
		{
			return uuid{str, str + n};
		}
	}	 // namespace literals
}	 // namespace sek

template<>
struct std::hash<sek::uuid>
{
	[[nodiscard]] constexpr sek::hash_t operator()(sek::uuid id) const noexcept { return sek::hash(id); }
};