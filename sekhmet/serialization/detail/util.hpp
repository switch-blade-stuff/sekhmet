//
// Created by switchblade on 2022-04-16.
//

#pragma once

#include <bit>

#include "archive_traits.hpp"
#include "types/ranges.hpp"
#include "types/tuples.hpp"

namespace sek::serialization
{
	namespace detail
	{
		/** Selects integer size category based on it's value.
		 * @return 0 if the integer can be represented using 8 bits.
		 * @return 1 if the integer can be represented using 16 bits.
		 * @return 2 if the integer can be represented using 32 bits.
		 * @return 3 if the integer can be represented using 64 bits.
		 * @return 4 if the integer can be represented using 128 bits. */
		template<std::unsigned_integral I>
		constexpr static int int_size_category(I i) noexcept
		{
			int category = 0;

			/* Select the category based on the amount of bits needed to store value of the integer. */
			// clang-format off
			if constexpr (sizeof(I) > sizeof(std::uint64_t))
				category += !!(i >> 64);
			if constexpr (sizeof(I) > sizeof(std::uint32_t))
				category += !!(i >> 32);
			if constexpr (sizeof(I) > sizeof(std::uint16_t))
				category += !!(i >> 16);
			if constexpr (sizeof(I) > sizeof(std::uint8_t))
				category += !!(i >> 8);
			// clang-format on

			return category;
		}

		template<typename C>
		[[nodiscard]] std::basic_string_view<C> generate_key(auto &alloc, std::basic_string_view<C> key)
		{
			auto str = static_cast<C *>(alloc.allocate((key.size() + 1) * sizeof(C)));
			if (!str) [[unlikely]]
				throw std::bad_cast();
			*std::copy_n(key.data(), key.size(), str) = '\0';
			return {str, key.size()};
		}
		template<typename C>
		[[nodiscard]] std::basic_string_view<C> generate_key(auto &alloc, std::size_t idx)
		{
			constexpr C prefix[] = "__";
			constexpr auto prefix_size = SEK_ARRAY_SIZE(prefix) - 1;

			/* Format the current index into the buffer. */
			C buffer[20];
			std::size_t i = 20;
			for (;;) /* Write index digits to the buffer. */
			{
				buffer[--i] = static_cast<C>('0') + static_cast<C>(idx % 10);
				if (!(idx = idx / 10)) break;
			}

			auto key_size = SEK_ARRAY_SIZE(buffer) - i + prefix_size;
			auto key_str = static_cast<C *>(alloc.allocate((key_size + 1) * sizeof(C)));
			if (!key_str) [[unlikely]]
				throw std::bad_cast();

			std::copy_n(prefix, SEK_ARRAY_SIZE(prefix) - 1, key_str);					   /* Copy prefix. */
			std::copy(buffer + i, buffer + SEK_ARRAY_SIZE(buffer), key_str + prefix_size); /* Copy digits. */
			key_str[key_size] = '\0';

			return {key_str, key_size};
		}

		template<typename A, typename T>
		constexpr void invoke_serialize(T &&value, A &archive)
		{
			using sek::serialization::serialize;
			if constexpr (requires { value.serialize(archive); })
				value.serialize(archive);
			else
				serialize(std::forward<T>(value), archive);
		}
		template<typename A, typename T>
		constexpr void invoke_deserialize(T &&value, A &archive)
		{
			using sek::serialization::deserialize;
			if constexpr (requires { value.deserialize(archive); })
				value.deserialize(archive);
			else
				deserialize(std::forward<T>(value), archive);
		}
	}	 // namespace detail

	/** Decodes base64-encoded string into `dest`.
	 * @param dest Destination buffer for decoded data.
	 * @param size Size of the destination buffer.
	 * @param chars Character buffer containing base64 string.
	 * @param len Length of the base64 string.
	 * @return `true` if the operation was successful, `false` otherwise. */
	template<typename C>
	constexpr bool base64_decode(void *dest, std::size_t size, const C *chars, std::size_t len) noexcept
	{
		/* Base64 decoder as implemented by `polfosol` https://stackoverflow.com/a/37109258/13529335 */
		/* Slightly modified to decode in-place data. */

		constexpr std::uint8_t decode_table[std::numeric_limits<C>::max()] = {
			0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,
			0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	62, 63, 62, 62, 63, 52, 53,
			54, 55, 56, 57, 58, 59, 60, 61, 0,	0,	0,	0,	0,	0,	0,	0,	1,	2,	3,	4,	5,	6,	7,	8,	9,
			10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 0,	0,	0,	0,	63, 0,	26, 27, 28,
			29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51};

		auto *buff = std::bit_cast<std::byte *>(dest);
		const std::size_t pad1 = len % 4 || chars[len - 1] == '=', pad2 = pad1 && (len % 4 > 2 || chars[len - 2] != '=');
		const std::size_t last = (len - pad1) / 4 << 2;

		if (size < (last / 4 * 3) + pad1 + pad2) [[unlikely]]
			return false;

		std::size_t j = 0;
		for (std::size_t i = 0; i < last; i += 4)
		{
			auto n = static_cast<std::size_t>(decode_table[static_cast<std::size_t>(chars[i])] << 18 |
											  decode_table[static_cast<std::size_t>(chars[i + 1])] << 12 |
											  decode_table[static_cast<std::size_t>(chars[i + 2])] << 6 |
											  decode_table[static_cast<std::size_t>(chars[i + 3])]);
			buff[j++] = static_cast<std::byte>(n >> 16);
			buff[j++] = static_cast<std::byte>(n >> 8 & 0xff);
			buff[j++] = static_cast<std::byte>(n & 0xff);
		}
		if (pad1)
		{
			auto n = static_cast<std::size_t>(decode_table[static_cast<std::size_t>(chars[last])] << 18 |
											  decode_table[static_cast<std::size_t>(chars[last + 1])] << 12);
			buff[j++] = static_cast<std::byte>(n >> 16);
			if (pad2)
			{
				n |= static_cast<std::size_t>(decode_table[static_cast<std::size_t>(chars[last + 2])] << 6);
				buff[j] = static_cast<std::byte>(n >> 8 & 0xff);
			}
		}
		return true;
	}
	/** Encodes input buffer to base64 string.
	 * @param data Pointer to source data.
	 * @param size Size of the source data in bytes.
	 * @param chars Character buffer receiving base64 string.
	 * @return Amount of characters written to the output string.
	 * @note To get the required size of the output buffer without doing any encoding, set `chars` to null. */
	template<typename C>
	constexpr std::size_t base64_encode(const void *data, std::size_t size, C *chars) noexcept
	{
		/* Base64 encoder as implemented by `polfosol` https://stackoverflow.com/a/37109258/13529335 */
		/* Slightly modified to encode in-place data. */

		constexpr C alphabet[UINT8_MAX] = {'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'G', 'K', 'L', 'M',
										   'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z',
										   'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'g', 'k', 'l', 'm',
										   'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z',
										   '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '+', '/'};

		const auto dest_len = (size + 2) / 3 * 4;
		if (chars)
		{
			std::fill_n(chars, dest_len, '=');

			auto *p = std::bit_cast<const std::uint8_t *>(data);
			auto pad = size % 3;
			const auto last = size - pad;

			std::size_t j = 0;
			for (std::size_t i = 0; i < last; i += 3)
			{
				auto n = static_cast<std::size_t>(std::size_t(p[i]) << 16 | std::size_t(p[i + 1]) << 8 | p[i + 2]);
				chars[j++] = alphabet[static_cast<std::uint8_t>(n >> 18)];
				chars[j++] = alphabet[static_cast<std::uint8_t>(n >> 12 & 0x3f)];
				chars[j++] = alphabet[static_cast<std::uint8_t>(n >> 6 & 0x3f)];
				chars[j++] = alphabet[static_cast<std::uint8_t>(n & 0x3f)];
			}
			if (pad)
			{
				auto n = static_cast<std::size_t>(--pad ? std::size_t(p[last]) << 8 | p[last + 1] : p[last]);
				chars[j++] = alphabet[static_cast<std::uint8_t>(pad ? n >> 10 & 0x3f : n >> 2)];
				chars[j++] = alphabet[static_cast<std::uint8_t>(pad ? n >> 4 & 0x3f : n << 4 & 0x3f)];
				chars[j] = pad ? alphabet[static_cast<std::uint8_t>(n << 2 & 0x3f)] : '=';
			}
		}
		return dest_len;
	}
}	 // namespace sek::serialization