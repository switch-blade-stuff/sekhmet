/*
 * Created by switchblade on 2022-04-16
 */

#pragma once

#include "sekhmet/static_string.hpp"

#include "archive_traits.hpp"
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

	template<std::size_t I, basic_static_string Str, typename C, std::size_t N, typename T>
	[[nodiscard]] constexpr auto conv_prefix(basic_static_string<C, N, T> to) noexcept
	{
		if constexpr (I < Str.size())
		{
			to[I] = Str[I];
			return conv_prefix<I + 1, Str>(to);
		}
		else
			return to;
	}
	template<typename C, typename T, basic_static_string Str>
	[[nodiscard]] constexpr auto conv_prefix() noexcept
	{
		return conv_prefix<0, Str>(basic_static_string<C, Str.size(), T>{});
	}
	template<typename C, typename T = std::char_traits<C>, basic_static_string Prefix>
	[[nodiscard]] std::basic_string_view<C, T> generate_key(auto &alloc, std::size_t idx)
	{
		constexpr auto prefix = conv_prefix<C, T, Prefix>();

		/* Format the current index into the buffer. */
		C buffer[20];
		std::size_t i = 20;
		for (;;) /* Write index digits to the buffer. */
		{
			buffer[--i] = static_cast<C>('0') + static_cast<C>(idx % 10);
			if (!(idx = idx / 10)) break;
		}

		auto key_size = SEK_ARRAY_SIZE(buffer) - i + prefix.size();
		auto key_str = static_cast<C *>(alloc.allocate((key_size + 1) * sizeof(C)));
		if (!key_str) [[unlikely]]
			throw std::bad_cast();

		std::copy_n(prefix.data(), prefix.size(), key_str);								 /* Copy prefix. */
		std::copy(buffer + i, buffer + SEK_ARRAY_SIZE(buffer), key_str + prefix.size()); /* Copy digits. */
		key_str[key_size] = '\0';

		return {key_str, key_size};
	}

	// clang-format off
	template<typename T, typename A, typename... Args>
	constexpr void do_serialize(const T &value, A &archive, Args &&...args) requires serializable_with<T, A, Args...> || serializable_with<T, A>
	{
		if constexpr (serializable_with<T, A, Args...>)
			serializer<T, A>::serialize(value, archive, std::forward<Args>(args)...);
		else
			serializer<T, A>::serialize(value, archive);
	}
	template<typename T, typename A, typename... Args>
	constexpr void do_deserialize(T &value, A &archive, Args &&...args) requires deserializable_with<T, A, Args...> || deserializable_with<T, A>
	{
		if constexpr (deserializable_with<T, A, Args...>)
			serializer<T, A>::deserialize(value, archive, std::forward<Args>(args)...);
		else
			serializer<T, A>::deserialize(value, archive);
	}
	template<typename T, typename A, typename... Args>
	[[nodiscard]] constexpr T do_deserialize(std::in_place_type_t<T>, A &archive, Args &&...args)
		requires in_place_deserializable_with<T, A, Args...> || in_place_deserializable_with<T, A>
	{
		if constexpr (in_place_deserializable_with<T, A, Args...>)
			return serializer<T, A>::deserialize(std::in_place, archive, std::forward<Args>(args)...);
		else
			return serializer<T, A>::deserialize(std::in_place, archive);
	}
	// clang-format on
}	 // namespace sek::serialization::detail