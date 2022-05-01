//
// Created by switchblade on 2022-04-16.
//

#pragma once

#include "archive_traits.hpp"
#include "types/ranges.hpp"
#include "types/tuples.hpp"
#include "base64.hpp"

namespace sek::serialization::detail
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
	}	 // namespace sek::serialization