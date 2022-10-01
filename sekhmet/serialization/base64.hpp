/*
 * Created by switchblade on 01/05/22
 */

#pragma once

#include <algorithm>
#include <bit>
#include <cstdint>
#include <limits>

namespace sek
{
	namespace detail
	{
		template<typename C>
		constexpr static std::uint8_t base64_decoder[std::numeric_limits<C>::max()] = {
			0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,
			0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	62, 63, 62, 62, 63, 52, 53,
			54, 55, 56, 57, 58, 59, 60, 61, 0,	0,	0,	0,	0,	0,	0,	0,	1,	2,	3,	4,	5,	6,	7,	8,	9,
			10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 0,	0,	0,	0,	63, 0,	26, 27, 28,
			29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51};

		template<typename C>
		constexpr static C base64_encoder[std::numeric_limits<std::uint8_t>::max()] = {
			'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'G', 'K', 'L', 'M', 'N', 'O', 'P',
			'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f',
			'g', 'h', 'i', 'g', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v',
			'w', 'x', 'y', 'z', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '+', '/'};
	}	 // namespace detail

	/** Decodes base64-encoded string into `dest`.
	 * @param dest Destination buffer for decoded data.
	 * @param size Size of the destination buffer.
	 * @param chars Character buffer containing base64 string.
	 * @param len Length of the base64 string.
	 * @return Amount of bytes written to the output buffer, or `0` on decoding failure.
	 * @note To get the required size of the output buffer without doing any decoding, set `dest` to nullptr. */
	template<typename C>
	constexpr std::size_t base64_decode(void *dest, std::size_t size, const C *chars, std::size_t len) noexcept
	{
		/* Base64 decoder as implemented by `polfosol` https://stackoverflow.com/a/37109258/13529335 */
		/* Slightly modified to decode in-place data. */

		auto *buff = std::bit_cast<std::byte *>(dest);
		const std::size_t pad1 = len % 4 || chars[len - 1] == '=', pad2 = pad1 && (len % 4 > 2 || chars[len - 2] != '=');
		const std::size_t last = (len - pad1) / 4 << 2;

		const auto dest_min = (last / 4 * 3) + pad1 + pad2;
		if (dest)
		{
			/* Make sure destination buffer is large enough. */
			if (size < dest_min) [[unlikely]]
				return 0;

			std::size_t pos = 0;
			for (std::size_t i = 0; i < last; i += 4)
			{
				auto n = static_cast<std::size_t>(detail::base64_decoder<C>[static_cast<std::size_t>(chars[i])] << 18 |
												  detail::base64_decoder<C>[static_cast<std::size_t>(chars[i + 1])] << 12 |
												  detail::base64_decoder<C>[static_cast<std::size_t>(chars[i + 2])] << 6 |
												  detail::base64_decoder<C>[static_cast<std::size_t>(chars[i + 3])]);
				buff[pos++] = static_cast<std::byte>(n >> 16);
				buff[pos++] = static_cast<std::byte>(n >> 8 & 0xff);
				buff[pos++] = static_cast<std::byte>(n & 0xff);
			}
			if (pad1)
			{
				auto n = static_cast<std::size_t>(detail::base64_decoder<C>[static_cast<std::size_t>(chars[last])] << 18 |
												  detail::base64_decoder<C>[static_cast<std::size_t>(chars[last + 1])] << 12);
				buff[pos++] = static_cast<std::byte>(n >> 16);
				if (pad2)
				{
					n |= static_cast<std::size_t>(detail::base64_decoder<C>[static_cast<std::size_t>(chars[last + 2])] << 6);
					buff[pos++] = static_cast<std::byte>(n >> 8 & 0xff);
				}
			}
			return pos;
		}
		return dest_min;
	}
	/** Encodes input buffer to base64 string.
	 * @param chars Destination buffer for encoded base64 string.
	 * @param size Size of the destination buffer.
	 * @param data Pointer to source data.
	 * @param len Length of the source data buffer in bytes.
	 * @return Amount of characters written to the output string or `0` on failure.
	 * @note To get the required size of the output buffer without doing any encoding, set `chars` to nullptr. */
	template<typename C>
	constexpr std::size_t base64_encode(C *chars, std::size_t size, const void *data, std::size_t len) noexcept
	{
		/* Base64 encoder as implemented by `polfosol` https://stackoverflow.com/a/37109258/13529335 */
		/* Slightly modified to encode in-place data. */

		const auto dest_len = (len + 2) / 3 * 4;
		if (chars)
		{
			/* Make sure destination buffer is large enough. */
			if (size < dest_len) [[unlikely]]
				return 0;

			std::fill_n(chars, dest_len, '=');

			auto *p = std::bit_cast<const std::uint8_t *>(data);
			auto pad = len % 3;
			const auto last = len - pad;

			std::size_t j = 0;
			for (std::size_t i = 0; i < last; i += 3)
			{
				auto n = static_cast<std::size_t>(std::size_t(p[i]) << 16 | std::size_t(p[i + 1]) << 8 | p[i + 2]);
				chars[j++] = detail::base64_encoder<C>[static_cast<std::uint8_t>(n >> 18)];
				chars[j++] = detail::base64_encoder<C>[static_cast<std::uint8_t>(n >> 12 & 0x3f)];
				chars[j++] = detail::base64_encoder<C>[static_cast<std::uint8_t>(n >> 6 & 0x3f)];
				chars[j++] = detail::base64_encoder<C>[static_cast<std::uint8_t>(n & 0x3f)];
			}
			if (pad)
			{
				auto n = static_cast<std::size_t>(--pad ? std::size_t(p[last]) << 8 | p[last + 1] : p[last]);
				chars[j++] = detail::base64_encoder<C>[static_cast<std::uint8_t>(pad ? n >> 10 & 0x3f : n >> 2)];
				chars[j++] = detail::base64_encoder<C>[static_cast<std::uint8_t>(pad ? n >> 4 & 0x3f : n << 4 & 0x3f)];
				chars[j] = pad ? detail::base64_encoder<C>[static_cast<std::uint8_t>(n << 2 & 0x3f)] : '=';
			}
		}
		return dest_len;
	}
}	 // namespace sek