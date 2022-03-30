//
// Created by switchblade on 2022-03-13.
//

#pragma once

#include <cstddef>
#include <cstdint>
#include <iterator>

#include "define.h"
#include "hash.hpp"

namespace sek
{
	/** @brief UUID Version 4 Variant 1. */
	class uuid
	{
		friend constexpr hash_t hash(uuid) noexcept;

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

		/** Writes 36 characters of UUID string representation to the output iterator.
		 * @tparam C Character type of the output sequence.
		 * @param out Iterator to write the characters to.
		 * @param upper If set to true, hex digits would be written using uppercase letters.
		 * @note Output must have space for 36 characters. */
		template<typename C, std::output_iterator<C> Iter>
		constexpr void to_string(Iter out, bool upper = false) const
		{
			write_string<C>(out, upper);
		}
		/** Writes 36 characters of UUID string representation to the output iterator.
		 * @param out Iterator to write the characters to.
		 * @param upper If set to true, hex digits would be written using uppercase letters.
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
			for (std::size_t i = 0; first != last; ++first)
			{
				auto digit = 0;
				if (auto c = *first; c >= '0' && c <= '9')
					digit = c - '0';
				else if (c >= 'A' && c <= 'F')
					digit = c - 'A' + 10;
				else if (c >= 'a' && c <= 'f')
					digit = c - 'a' + 10;
				else
					continue;

				auto idx = i++;
				bytes[idx / 2] |= static_cast<std::byte>(digit << (idx % 2 ? 0 : 4));
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

	[[nodiscard]] constexpr hash_t hash(uuid id) noexcept { return fnv1a(id.bytes, SEK_ARRAY_SIZE(id.bytes)); }
}	 // namespace sek

template<>
struct std::hash<sek::uuid>
{
	[[nodiscard]] constexpr sek::hash_t operator()(sek::uuid id) const noexcept { return sek::hash(id); }
};