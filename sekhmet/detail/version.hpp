//
// Created by switchblade on 2021-12-12.
//

#pragma once

#include <compare>

#include "alloc_util.hpp"
#include "define.h"
#include "hash.hpp"

namespace sek
{
	namespace detail
	{
		template<std::size_t I, std::integral...>
		struct basic_version_base;

		template<std::size_t I, typename T, typename... Ts>
		constexpr T &extract_component(basic_version_base<I, T, Ts...> &) noexcept;
		template<std::size_t I, typename T, typename... Ts>
		constexpr const T &extract_component(const basic_version_base<I, T, Ts...> &) noexcept;

		template<std::size_t I, std::integral T>
		struct basic_version_base<I, T>
		{
			friend constexpr T &extract_component<>(basic_version_base<I, T> &) noexcept;
			friend constexpr const T &extract_component<>(const basic_version_base<I, T> &) noexcept;

		public:
			constexpr basic_version_base() noexcept = default;
			constexpr explicit basic_version_base(T component) noexcept : component(component) {}

		private:
			T component = {};
		};

		template<std::size_t I, std::integral T, std::integral... Ts>
		struct basic_version_base<I, T, Ts...> : basic_version_base<I + 1, Ts...>
		{
			friend constexpr T &extract_component<>(basic_version_base<I, T, Ts...> &) noexcept;
			friend constexpr const T &extract_component<>(const basic_version_base<I, T, Ts...> &) noexcept;

		public:
			constexpr basic_version_base() noexcept = default;
			template<std::integral... Args>
			constexpr explicit basic_version_base(T component, Args... args) noexcept
				: basic_version_base<I + 1, Ts...>(args...), component(component)
			{
			}

		private:
			T component = {};
		};

		template<std::size_t I, typename T, typename... Ts>
		constexpr T &extract_component(basic_version_base<I, T, Ts...> &v) noexcept
		{
			return v.component;
		}
		template<std::size_t I, typename T, typename... Ts>
		constexpr const T &extract_component(const basic_version_base<I, T, Ts...> &v) noexcept
		{
			return v.component;
		}

		template<typename U, typename C>
		constexpr U parse_version_char(C c)
		{
			return c >= '0' && c <= '9' ? static_cast<U>(c - '0') : throw std::runtime_error("Invalid version string");
		}
		template<std::size_t I, std::forward_iterator Iter, typename U, typename... Us>
		constexpr void parse_version(Iter first, Iter second, basic_version_base<I, U, Us...> &v) noexcept
		{
			for (std::iter_value_t<Iter> c; first != second && (c = *first++) != '\0';)
				if (c == '.') [[unlikely]]
				{
					if constexpr (sizeof...(Us) != 0) parse_version<I + 1>(first, second, v);
					break;
				}
				else
				{
					/* Keep parsing this component until a separator. */
					const U a = extract_component(v) * 10;
					const U b = parse_version_char<U>(c);
					extract_component(v) = a + b;
				}
		}
	}	 // namespace detail

	template<std::integral... Components>
		requires(sizeof...(Components) != 0)
	struct basic_version : detail::basic_version_base<0, Components...>
	{
		template<typename... Ts>
		friend constexpr hash_t hash(const basic_version<Ts...> &) noexcept;

		constexpr basic_version() noexcept = default;
		template<std::integral... Args>
		constexpr basic_version(Args... args) noexcept
			requires std::conjunction_v<std::is_constructible<Components, Args>
										...> : detail::basic_version_base<0, Components...>(args...)
		{
		}

		template<std::forward_iterator Iter>
		constexpr explicit basic_version(Iter first, Iter last)
		{
			detail::parse_version(first, last, *this);
		}
		template<std::ranges::forward_range R>
		constexpr explicit basic_version(const R &str) : basic_version(std::ranges::begin(str), std::ranges::end(str))
		{
		}
		template<typename C, std::size_t N>
		constexpr explicit basic_version(const C (&str)[N]) noexcept : basic_version(std::begin(str), std::end(str))
		{
		}

		/** Returns reference to the specified component of the version. */
		template<std::size_t I>
		[[nodiscard]] constexpr auto &component() noexcept
		{
			return extract_component<I>(*this);
		}
		/** @copydoc component */
		template<std::size_t I>
		[[nodiscard]] constexpr const auto &component() const noexcept
		{
			return extract_component<I>(*this);
		}

		[[nodiscard]] constexpr auto operator<=>(const basic_version &other) const noexcept
		{
			return compare_three_way_impl<0>(other);
		}
		[[nodiscard]] constexpr bool operator==(const basic_version &other) const noexcept
		{
			return compare_eq_impl<0>(other);
		}

		/** Writes the version as a string to the output iterator.
		 * @tparam C Character type of the output sequence.
		 * @param out Iterator to write the characters to.
		 * @note Output must have enough space for the version string. */
		template<typename C, std::output_iterator<C> Iter>
		constexpr void to_string(Iter out) const
		{
			to_string_impl<C, 0>(out);
		}
		/** Writes the version as a string to the output iterator.
		 * @param out Iterator to write the characters to.
		 * @note Output must have enough space for the version string. */
		template<std::forward_iterator Iter>
		constexpr void to_string(Iter out) const
		{
			return to_string<std::iter_value_t<Iter>, Iter>(out);
		}

		constexpr void swap(basic_version &other) noexcept { swap_impl<0>(other); }

	private:
		template<std::size_t I>
		[[nodiscard]] constexpr std::strong_ordering compare_three_way_impl(const basic_version &other) const noexcept
		{
			std::strong_ordering result = component<I>() <=> other.template component<I>();

			if constexpr (I + 1 < sizeof...(Components))
			{
				if (result == std::strong_ordering::equal || result == std::strong_ordering::equivalent)
					return compare_three_way_impl<I + 1>(other);
			}

			return result;
		}
		template<std::size_t I>
		[[nodiscard]] constexpr bool compare_eq_impl(const basic_version &other) const noexcept
		{
			bool result = component<I>() == other.template component<I>();

			if constexpr (I + 1 < sizeof...(Components))
			{
				if (result) return compare_eq_impl<I + 1>(other);
			}

			return result;
		}

		template<typename C, typename Iter, typename T>
		constexpr void component_to_string(Iter &out, T val) const noexcept
		{
			if constexpr (std::is_signed_v<T>)
				if (val < 0)
				{
					*out++ = '-';
					val = -val;
				}

			constexpr C alphabet[] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9'};

			T div = 1;
			T digits = 1;
			for (; div <= val / 10; ++digits) div *= 10;
			for (; digits > 0; --digits)
			{
				*out++ = alphabet[val / div];
				val %= div;
				div /= 10;
			}
		}
		template<typename C, std::size_t I, typename Iter>
		constexpr void to_string_impl(Iter &out) const noexcept
		{
			component_to_string<C>(out, component<I>());

			if constexpr (I + 1 < sizeof...(Components))
			{
				*out++ = '.';
				to_string_impl<C, I + 1>(out);
			}
		}

		template<std::size_t I>
		constexpr void swap_impl(basic_version &other) noexcept
		{
			using std::swap;
			swap(component<I>(), other.template component<I>());

			if constexpr (I + 1 < sizeof...(Components)) return swap_impl<I + 1>(other);
		}
		template<std::size_t I>
		[[nodiscard]] constexpr hash_t hash_impl(hash_t seed = 0) const noexcept
		{
			hash_combine(seed, component<I>());

			if constexpr (I + 1 < sizeof...(Components))
				return hash_impl<I + 1>(seed);
			else
				return seed;
		}
	};

	template<typename... Ts>
	constexpr void swap(basic_version<Ts...> &a, basic_version<Ts...> &b) noexcept
	{
		a.swap(b);
	}

	template<std::size_t I, typename... Ts>
	[[nodiscard]] constexpr auto &get(basic_version<Ts...> &v) noexcept
	{
		return v.template component<I>();
	}
	template<std::size_t I, typename... Ts>
	[[nodiscard]] constexpr auto &get(const basic_version<Ts...> &v) noexcept
	{
		return v.template component<I>();
	}

	template<typename... Ts>
	[[nodiscard]] constexpr hash_t hash(const basic_version<Ts...> &v) noexcept
	{
		return v.template hash_impl<0>();
	}
}	 // namespace sek

template<typename... Ts>
struct std::hash<sek::basic_version<Ts...>>
{
	[[nodiscard]] constexpr sek::hash_t operator()(const sek::basic_version<Ts...> &v) const noexcept
	{
		return sek::hash(v);
	}
};
template<typename... Ts>
struct std::tuple_size<sek::basic_version<Ts...>> : std::integral_constant<std::size_t, sizeof...(Ts)>
{
};

namespace sek
{
	using version_base_t = basic_version<std::uint16_t, std::uint16_t, std::uint32_t>;

	/** @brief Structure holding 3 integers representing a version number. Equivalent to `basic_version<std::uint16_t, std::uint16_t, std::uint32_t>`. */
	struct version : version_base_t
	{
		constexpr version() noexcept = default;
		/** Initializes a version from the major, minor & patch components.
		 * @param major Major component.
		 * @param minor Minor component.
		 * @param patch Patch component. */
		constexpr version(std::uint16_t major, std::uint16_t minor, std::uint32_t patch) noexcept
			: basic_version(major, minor, patch)
		{
		}

		/** Initializes a version from a from a character range.
		 * @note Version string must contain base-10 integers separated with dots ('.'). */
		template<std::forward_iterator Iter>
		constexpr explicit version(Iter first, Iter last) : basic_version(first, last)
		{
		}
		/** @copydoc uuid */
		template<std::ranges::forward_range R>
		constexpr explicit version(const R &str) : version(std::ranges::begin(str), std::ranges::end(str))
		{
		}
		/**  Initializes a version from a from a character array. */
		template<typename C, std::size_t N>
		constexpr explicit version(const C (&str)[N]) noexcept : version(std::begin(str), std::end(str))
		{
		}

		/** Returns reference to the major component of the version. */
		[[nodiscard]] constexpr std::uint16_t &major() noexcept { return component<0>(); }
		/** @copydoc major */
		[[nodiscard]] constexpr const std::uint16_t &major() const noexcept { return component<0>(); }

		/** Returns reference to the minor component of the version. */
		[[nodiscard]] constexpr std::uint16_t &minor() noexcept { return component<1>(); }
		/** @copydoc minor */
		[[nodiscard]] constexpr const std::uint16_t &minor() const noexcept { return component<1>(); }

		/** Returns reference to the patch component of the version. */
		[[nodiscard]] constexpr std::uint32_t &patch() noexcept { return component<2>(); }
		/** @copydoc patch */
		[[nodiscard]] constexpr const std::uint32_t &patch() const noexcept { return component<2>(); }

		/** Returns 64-bit integer representation of the version . */
		[[nodiscard]] constexpr std::uint64_t as_uint64() const noexcept
		{
			return (static_cast<std::uint64_t>(major()) << 48) | (static_cast<std::uint64_t>(minor()) << 32) |
				   (static_cast<std::uint64_t>(patch()));
		}
	};

	template<std::size_t I>
	[[nodiscard]] constexpr auto &get(version &v) noexcept
	{
		return v.template component<I>();
	}
	template<std::size_t I>
	[[nodiscard]] constexpr auto &get(const version &v) noexcept
	{
		return v.template component<I>();
	}

	[[nodiscard]] constexpr hash_t hash(const version &v) noexcept { return hash(v.as_uint64()); }

	namespace literals
	{
		[[nodiscard]] constexpr version operator""_ver(const char *str, std::size_t n) noexcept
		{
			return version{str, str + n};
		}
		[[nodiscard]] constexpr version operator""_ver(const char8_t *str, std::size_t n) noexcept
		{
			return version{str, str + n};
		}
		[[nodiscard]] constexpr version operator""_ver(const char16_t *str, std::size_t n) noexcept
		{
			return version{str, str + n};
		}
		[[nodiscard]] constexpr version operator""_ver(const char32_t *str, std::size_t n) noexcept
		{
			return version{str, str + n};
		}
		[[nodiscard]] constexpr version operator""_ver(const wchar_t *str, std::size_t n) noexcept
		{
			return version{str, str + n};
		}
	}	 // namespace literals
}	 // namespace sek

template<>
struct std::hash<sek::version>
{
	[[nodiscard]] constexpr sek::hash_t operator()(const sek::version &v) const noexcept { return sek::hash(v); }
};
template<>
struct std::tuple_size<sek::version> : std::integral_constant<std::size_t, 3>
{
};

#define SEK_VERSION(major, minor, patch) (::sek::version{(major), (minor), (patch)})