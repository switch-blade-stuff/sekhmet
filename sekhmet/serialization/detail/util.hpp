/*
 * Created by switchblade on 2022-04-16
 */

#pragma once

#include "sekhmet/static_string.hpp"

#include "base64.hpp"
#include "traits.hpp"

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

	template<typename C, typename T = std::char_traits<C>, basic_static_string Prefix = "__">
	[[nodiscard]] std::basic_string<C, T> generate_key(std::size_t idx)
	{
		constexpr auto prefix = static_string_cast<C, T>(Prefix);
		std::basic_string<C, T> result;

		/* Format the current index into the buffer. */
		C buffer[20];
		std::size_t i = 20;
		for (;;) /* Write index digits to the buffer. */
		{
			buffer[--i] = static_cast<C>('0') + static_cast<C>(idx % 10);
			if (!(idx = idx / 10)) break;
		}

		const auto buffer_size = SEK_ARRAY_SIZE(buffer) - i;
		const auto key_size = buffer_size + prefix.size();
		result.reserve(key_size);

		result.append(prefix.data(), prefix.size()); /* Copy prefix. */
		result.append(buffer + i, buffer_size);		 /* Copy digits. */

		return result;
	}

	// clang-format off
	template<typename T, typename A, typename... Args>
	constexpr void do_serialize(const T &value, A &archive, Args &&...args) requires serializable_with<T, A, Args...>
	{
		serializer<T, A>::serialize(value, archive, std::forward<Args>(args)...);
	}
	template<typename T, typename A, typename... Args>
	constexpr void do_deserialize(T &value, A &archive, Args &&...args) requires deserializable_with<T, A, Args...>
	{
		serializer<T, A>::deserialize(value, archive, std::forward<Args>(args)...);
	}
	template<typename T, typename A, typename... Args>
	[[nodiscard]] constexpr T do_deserialize(std::in_place_type_t<T>, A &archive, Args &&...args) requires in_place_deserializable_with<T, A, Args...>
	{
		return serializer<T, A>::deserialize(std::in_place, archive, std::forward<Args>(args)...);
	}
	// clang-format on
}	 // namespace sek::serialization::detail