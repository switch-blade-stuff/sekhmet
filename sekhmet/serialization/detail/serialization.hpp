//
// Created by switchblade on 2022-04-16.
//

#pragma once

#include "archive_traits.hpp"
#include "types/ranges.hpp"
#include "types/tuples.hpp"

namespace sek::serialization::detail
{
	template<typename T>
	constexpr void base64_decode(T &value, const char *chars, std::size_t len) noexcept
	{
		/* Base64 decoder as implemented by `polfosol` https://stackoverflow.com/a/37109258/13529335 */
		/* Slightly modified to decode in-place data. */

		constexpr std::uint8_t decode_table[CHAR_MAX] = {
			0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,
			0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	62, 63, 62, 62, 63, 52, 53,
			54, 55, 56, 57, 58, 59, 60, 61, 0,	0,	0,	0,	0,	0,	0,	0,	1,	2,	3,	4,	5,	6,	7,	8,	9,
			10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 0,	0,	0,	0,	63, 0,	26, 27, 28,
			29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51};

		auto *dest = std::bit_cast<std::byte *>(&value);
		auto *data = std::bit_cast<const std::uint8_t *>(chars);
		const std::size_t pad1 = len % 4 || data[len - 1] == '=', pad2 = pad1 && (len % 4 > 2 || data[len - 2] != '=');
		const std::size_t last = (len - pad1) / 4 << 2;

		std::size_t j = 0;
		for (std::size_t i = 0; i < last; i += 4)
		{
			auto n = static_cast<std::size_t>(decode_table[data[i]] << 18 | decode_table[data[i + 1]] << 12 |
											  decode_table[data[i + 2]] << 6 | decode_table[data[i + 3]]);
			dest[j++] = static_cast<std::byte>(n >> 16);
			dest[j++] = static_cast<std::byte>(n >> 8 & 0xff);
			dest[j++] = static_cast<std::byte>(n & 0xff);
		}
		if (pad1)
		{
			auto n = static_cast<std::size_t>(decode_table[data[last]] << 18 | decode_table[data[last + 1]] << 12);
			dest[j++] = static_cast<std::byte>(n >> 16);
			if (pad2)
			{
				n |= static_cast<std::size_t>(decode_table[data[last + 2]] << 6);
				dest[j] = static_cast<std::byte>(n >> 8 & 0xff);
			}
		}
	}
	template<typename T>
	constexpr std::size_t base64_encode(const T &value, char *chars) noexcept
	{
		/* Base64 encoder as implemented by `polfosol` https://stackoverflow.com/a/37109258/13529335 */
		/* Slightly modified to encode in-place data. */

		constexpr char alphabet[UINT8_MAX] = {'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'G', 'K', 'L', 'M',
											  'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z',
											  'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'g', 'k', 'l', 'm',
											  'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z',
											  '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '+', '/'};

		const auto src_len = sizeof(value);
		const auto dest_len = (src_len + 2) / 3 * 4;
		if (chars)
		{
			auto *data = std::bit_cast<const std::uint8_t *>(&value);
			auto pad = src_len % 3;
			const auto last = src_len - pad;

			std::size_t j = 0;
			for (std::size_t i = 0; i < last; i += 3)
			{
				auto n = static_cast<std::size_t>(int(data[i]) << 16 | int(data[i + 1]) << 8 | data[i + 2]);
				chars[j++] = alphabet[static_cast<std::uint8_t>(n >> 18)];
				chars[j++] = alphabet[static_cast<std::uint8_t>(n >> 12 & 0x3f)];
				chars[j++] = alphabet[static_cast<std::uint8_t>(n >> 6 & 0x3f)];
				chars[j++] = alphabet[static_cast<std::uint8_t>(n & 0x3f)];
			}
			if (pad)
			{
				auto n = static_cast<std::size_t>(--pad ? int(data[last]) << 8 | data[last + 1] : data[last]);
				chars[j++] = alphabet[static_cast<std::uint8_t>(pad ? n >> 10 & 0x3f : n >> 2)];
				chars[j++] = alphabet[static_cast<std::uint8_t>(pad ? n >> 4 & 0x3f : n << 4 & 0x3f)];
				chars[j] = pad ? alphabet[static_cast<std::uint8_t>(n << 2 & 0x3f)] : '=';
			}
		}
		return dest_len;
	}

	template<typename A, typename T>
	constexpr void invoke_serialize(T &&value, A &archive)
	{
		using sek::serialization::serialize;
		if constexpr (!std::is_pointer_v<T> && requires { value.serialize(archive); })
			value.serialize(archive);
		else
			serialize(std::forward<T>(value), archive);
	}
	template<typename A, typename T>
	constexpr void invoke_deserialize(T &&value, A &archive)
	{
		using sek::serialization::deserialize;
		if constexpr (!std::is_pointer_v<T> && requires { value.deserialize(archive); })
			value.deserialize(archive);
		else
			deserialize(std::forward<T>(value), archive);
	}
}	 // namespace sek::serialization::detail