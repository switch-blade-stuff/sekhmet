//
// Created by switchblade on 2022-01-23.
//

#pragma once

#include "define.h"
#include "meta_containers.hpp"
#include "static_string.hpp"

namespace instantiation
{
	template<typename>
	struct type_factory;
}	 // namespace instantiation

namespace sek
{
	class type_info;

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
		consteval auto generate_type_name_impl() noexcept
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
			constexpr auto &value = auto_constant<generate_type_name_impl<SEK_PRETTY_FUNC>()>::value;
			return std::basic_string_view<char>{value.begin(), value.end()};
		}
	}	 // namespace detail

	/** Returns name of the specified type.
	 * @warning Consistency of generated type names across different compilers is not guaranteed.
	 * To generate consistent type names, overload this function for the desired type or use `SEK_REFLECT_TYPE`. */
	template<typename T>
	[[nodiscard]] constexpr std::string_view type_name() noexcept
	{
		return detail::generate_type_name<T>();
	}

	/** @brief Structure used to reference reflected information of a type. */
	class type_info
	{
		template<typename>
		friend struct instantiation::type_factory;

		using name_t = std::string_view;

		struct data_t
		{
			template<typename>
			struct instance
			{
				static const data_t value;
			};

			template<typename T>
			constexpr explicit data_t(type_selector_t<T>) noexcept : name(type_name<T>())
			{
			}

			const name_t name;
			meta_view<const data_t *> parents;
		};

		template<typename T>
		class factory_base
		{
			template<typename>
			friend struct data_t::instance;

			template<typename... Ts>
			constexpr static bool parent_list = std::conjunction_v<std::is_base_of<Ts, T>...>;

		protected:
			/** Specifies list of parents of the type. */
			template<typename... Ts>
			constexpr void parents() noexcept
				requires parent_list<Ts...>
			{
				if constexpr (sizeof...(Ts) != 0) parents_impl(unique_type_seq_t<type_seq_t<Ts...>>{});
			}

		private:
			template<typename... Ts>
			constexpr void parents_impl(type_seq_t<Ts...>) noexcept
			{
				data.parents = {array_constant<const data_t *, &data_t::instance<Ts>::value...>::value};
			}

			data_t data = data_t{type_selector<T>};
		};

	public:
		/** Returns type info of `T`. */
		template<typename T>
		constexpr static type_info get() noexcept
		{
			return type_info{&data_t::instance<T>::value};
		}

	private:
		constexpr explicit type_info(const data_t *data) noexcept : data(data) {}

	public:
		/** Returns name of the underlying type. */
		[[nodiscard]] constexpr std::string_view name() const noexcept { return data->name; }

		/** Checks if the underlying type has the specified parent. */
		[[nodiscard]] constexpr bool has_parent(std::string_view name) const noexcept
		{
			auto pred = [name](auto p) { return p->name == name || type_info{p}.has_parent(name); };
			return std::ranges::any_of(data->parents, pred);
		}
		/** Checks if `T` is a parent of the underlying type. */
		template<typename T>
		[[nodiscard]] constexpr bool has_parent() const noexcept
		{
			return has_parent(type_name<T>());
		}

	private:
		const data_t *data = nullptr;
	};

	template<typename T>
	const type_info::data_t type_info::data_t::instance<T>::value = data_t{type_selector<T>};
}	 // namespace sek

#define SEK_REFLECT_TYPE_1(T)                                                                                          \
	template<>                                                                                                         \
	struct instantiation::type_factory<T> : sek::type_info::factory_base<T>                                            \
	{                                                                                                                  \
		constexpr type_factory() noexcept;                                                                             \
	};                                                                                                                 \
	template<>                                                                                                         \
	const sek::type_info::data_t sek::type_info::data_t::instance<T>::value = instantiation::type_factory<T>{}.data;   \
                                                                                                                       \
	constexpr instantiation::type_factory<T>::type_factory() noexcept : factory_base()
#define SEK_REFLECT_TYPE_2(T, name)                                                                                    \
	static_assert(SEK_ARRAY_SIZE(name), "Type name can not be empty");                                                 \
	template<>                                                                                                         \
	[[nodiscard]] constexpr std::string_view sek::type_name<T>() noexcept                                              \
	{                                                                                                                  \
		return {name};                                                                                                 \
	}                                                                                                                  \
                                                                                                                       \
	SEK_REFLECT_TYPE_1(T)

/** @brief Macro used to reflect information about a type.
 * @param T Type currently being reflected.
 * @param name Optional name for the type.
 *
 * @note Specifying an explicit name will create an overload for `sek::type_name`.
 * It is recommended to do so if the type name would be used at runtime (ex. as a resource or prefab component). */
#define SEK_REFLECT_TYPE(...) SEK_GET_MACRO_2(__VA_ARGS__, SEK_REFLECT_TYPE_2, SEK_REFLECT_TYPE_1)(__VA_ARGS__)