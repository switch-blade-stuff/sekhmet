/*
 * Created by switch_blade on 2022-04-13.
 */

#pragma once

#include <concepts>
#include <iosfwd>

#include "sekhmet/detail/define.h"

#include "types/ranges.hpp"
#include "types/tuples.hpp"
#include <string_view>

namespace sek::serialization
{
	/** @brief Category tag used to indicate that an archive supports input operations. */
	struct input_archive_category
	{
	};
	/** @brief Category tag used to indicate that an archive supports output operations. */
	struct output_archive_category
	{
	};
	/** @brief Category tag used to indicate that an archive supports both input and output operations. */
	struct inout_archive_category : input_archive_category, output_archive_category
	{
	};

	// clang-format off
	/** @brief Concept satisfied only if archive `A` has the `input_archive_category` category. */
	template<typename A>
	concept input_archive = requires
	{
		typename A::archive_category;
		std::is_base_of_v<input_archive_category, typename A::archive_category>;
	};
	/** @brief Concept satisfied only if archive `A` has the `output_archive_category` category. */
	template<typename A>
	concept output_archive = requires
	{
		typename A::archive_category;
		std::is_base_of_v<output_archive_category, typename A::archive_category>;
	};
	// clang-format on

	namespace detail
	{
		// clang-format off
		template<typename T, typename A, typename... Args>
		concept mem_serialize = requires(const T &v, A &a, Args &&...args) { v.serialize(a, std::forward<Args>(args)...); };
		template<typename T, typename A, typename... Args>
		concept adl_serialize = requires(const T &v, A &a, Args &&...args) { serialize(v, a, std::forward<Args>(args)...); };
		template<typename T, typename A, typename... Args>
		concept enable_serialize = mem_serialize<T, A, Args...> || adl_serialize<T, A, Args...>;

		template<typename T, typename A, typename... Args>
		concept mem_deserialize = requires(T &v, A &a, Args &&...args) { v.deserialize(a, std::forward<Args>(args)...); };
		template<typename T, typename A, typename... Args>
		concept adl_deserialize = requires(T &v, A &a, Args &&...args) { deserialize(v, a, std::forward<Args>(args)...); };
		template<typename T, typename A, typename... Args>
		concept enable_deserialize = mem_deserialize<T, A, Args...> || adl_deserialize<T, A, Args...>;

		template<typename T, typename A, typename... Args>
		concept in_place_deserialize = requires(A &a, Args &&...args) { deserialize(std::in_place_type<T>, a, std::forward<Args>(args)...); };
		// clang-format on
	}	 // namespace detail

	/** @brief User-overloadable serializer type used to serialize objects of type `T` using an archive `Archive`.
	 *
	 * The default serializer implementation attempts to use member serialization functions if they are defined,
	 * otherwise selects serialization functions via ADL. If no valid serialization function exists, the respective
	 * `serialize` and/or `deserialize` functions are not defined for `serializer`.
	 *
	 * In-place overload of `deserialize` uses the deserialization constructor if it exists, otherwise attempts to use
	 * an in-place `deserialize` function selected via ADL. If no in-place serialization functions exist,
	 * default-constructs an instance of `T` and deserializes via instance `deserialize`.
	 *
	 * @tparam T Serializable type.
	 * @tparam Archive Archive used for serialization. */
	template<typename T, typename Archive>
	struct serializer
	{
	private:
		// clang-format off
		// clang-format on

	public:
		// clang-format off
		/** Serializes an instance of type `T` using the passed archive and forwarded arguments. */
		template<typename... Args>
		constexpr static void serialize(const T &value, Archive &ar, Args &&...args) requires detail::enable_serialize<T, Archive, Args...>
		{
			if constexpr (detail::mem_serialize<T, Archive, Args...>)
				value.serialize(ar, std::forward<Args>(args)...);
			else
				serialize(value, ar, std::forward<Args>(args)...);
		}
		/** Deserializes an instance of type `T` using the passed archive and forwarded arguments. */
		template<typename... Args>
		constexpr static void deserialize(T &value, Archive &ar, Args &&...args) requires detail::enable_deserialize<T, Archive, Args...>
		{
			if constexpr (detail::mem_deserialize<T, Archive, Args...>)
				value.deserialize(ar, std::forward<Args>(args)...);
			else
				deserialize(value, ar, std::forward<Args>(args)...);
		}

		/** Deserializes an instance of type `T` in-place using the passed archive and forwarded arguments. */
		template<typename... Args>
		[[nodiscard]] constexpr static T deserialize(std::in_place_t, Archive &ar, Args &&...args)
			requires detail::in_place_deserializable<T, Archive, Args...> ||
					 detail::enable_deserialize<T, Archive, Args...> ||
					 std::constructible_from<T, Archive &, Args...>
		{
			if constexpr (std::constructible_from<T, Archive &, Args...>)
				return T{ar, std::forward<Args>(args)...};
			else if constexpr (detail::in_place_deserializable<T, Archive, Args...>)
				return deserialize(std::in_place_type<T>, ar, std::forward<Args>(args)...);
			else
			{
				T result;
				serializer::deserialize(result, ar, std::forward<Args>(args)...);
				return result;
			}
		}
		// clang-format on
	};

	// clang-format off
	template<typename T, typename A, typename... Args>
	concept serializable_with = requires(const T &value, A &archive, Args &&...args)
	{			   
		typename serializer<T, A>;
		serializer<T, A>::serialize(value, archive, std::forward<Args>(args)...);
	};
	template<typename T, typename A, typename... Args>
	concept deserializable_with = requires(T &value, A &archive, Args &&...args)
	{
		typename serializer<T, A>;
		serializer<T, A>::deserialize(value, archive, std::forward<Args>(args)...);
	};
	template<typename T, typename A, typename... Args>
	concept in_place_deserializable_with = requires(A &archive, Args &&...args)
	{
		typename serializer<T, A>;
		serializer<T, A>::deserialize(std::in_place, archive, std::forward<Args>(args)...);
	};
	// clang-format on
}	 // namespace sek::serialization
