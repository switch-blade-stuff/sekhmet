/*
 * Created by switchblade on 23/05/22
 */

#pragma once

#include "sekhmet/detail/static_string.hpp"

namespace sek::engine
{
	namespace detail
	{
		template<basic_static_string Src, std::size_t J, std::size_t I, std::size_t Last, std::size_t N>
		consteval auto format_type_name(basic_static_string<char, N> result) noexcept
		{
			if constexpr (I == Last)
			{
				result[J] = '\0';
				return result;
			}
			else
			{
				result[J] = static_cast<typename decltype(result)::value_type>(Src[I]);
				return format_type_name<Src, J + 1, I + 1, Last>(result);
			}
		}
		template<basic_static_string Src, std::size_t J, std::size_t I, std::size_t Last, std::size_t N>
		consteval auto format_type_name() noexcept
		{
			return format_type_name<Src, J, I, Last, N>({});
		}
		template<basic_static_string Name>
		consteval auto format_type_name() noexcept
		{
#if defined(__clang__) || defined(__GNUC__)
			constexpr auto offset_start = Name.find_first('=') + 2;
			constexpr auto offset_end = Name.find_last(']');
			constexpr auto trimmed_length = offset_end - offset_start + 1;
#elif defined(_MSC_VER)
			constexpr auto offset_start = Name.find_first('<') + 1;
			constexpr auto offset_end = Name.find_last('>');
			constexpr auto trimmed_length = offset_end - offset_start + 1;
#else
#error "Implement type name generation for this compiler"
#endif
			return format_type_name<Name, 0, offset_start, offset_end, trimmed_length>();
		}

		template<typename T>
		[[nodiscard]] constexpr std::basic_string_view<char> generate_type_name() noexcept
		{
			constexpr auto &value = auto_constant<format_type_name<SEK_PRETTY_FUNC>()>::value;
			return std::basic_string_view<char>{value.begin(), value.end()};
		}
	}	 // namespace detail

	/** Returns name of the specified type.
	 * @warning Consistency of generated type names across different compilers is not guaranteed.
	 * To generate consistent type names, overload this function for the desired type. */
	template<typename T>
	[[nodiscard]] constexpr std::string_view type_name() noexcept
	{
		return detail::generate_type_name<T>();
	}
}	 // namespace sek::engine