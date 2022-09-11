/*
 * Created by switch_blade on 2022-04-13.
 */

#pragma once

#include <concepts>
#include <iosfwd>

#include "sekhmet/detail/define.h"

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
	/** @brief Concept satisfied only if archive `A` supports reading instances of `T`. */
	template<typename A, typename T>
	concept input_archive = requires(A &archive, T &value)
	{
		typename A::archive_category;
		std::is_base_of_v<input_archive_category, typename A::archive_category>;

		{ archive >> value } -> std::convertible_to<A &>;
		{ archive.read(value) } -> std::convertible_to<A &>;
		{ archive.try_read(value) } -> std::convertible_to<bool>;
		{ archive.read(std::in_place_type<T>) } -> std::convertible_to<T>;
	};
	/** @brief Concept satisfied only if archive `A` supports writing instances of `T`. */
	template<typename A, typename T>
	concept output_archive = requires(A &archive, const T &value)
	{
		typename A::archive_category;
		std::is_base_of_v<output_archive_category, typename A::archive_category>;

		{ archive << value } -> std::convertible_to<A &>;
		{ archive.write(value) } -> std::convertible_to<A &>;
	};
	// clang-format on

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
		template<typename... Args>
		constexpr static bool mem_serialize = requires(const T &v, Archive &a, Args &&...args) { v.serialize(a, std::forward<Args>(args)...); };
		template<typename... Args>
		constexpr static bool adl_serialize = requires(const T &v, Archive &a, Args &&...args) { serialize(v, a, std::forward<Args>(args)...); };
		template<typename... Args>
		constexpr static bool enable_serialize = mem_serialize<Args...> || adl_serialize<Args...>;

		template<typename... Args>
		constexpr static bool mem_deserialize = requires(T &v, Archive &a, Args &&...args) { v.deserialize(a, std::forward<Args>(args)...); };
		template<typename... Args>
		constexpr static bool adl_deserialize = requires(T &v, Archive &a, Args &&...args) { deserialize(v, a, std::forward<Args>(args)...); };
		template<typename... Args>
		constexpr static bool enable_deserialize = mem_deserialize<Args...> || adl_deserialize<Args...>;

		template<typename... Args>
		constexpr static bool in_place = requires(Archive &a, Args &&...args)
		{
			deserialize(std::in_place_type<T>, a, std::forward<Args>(args)...);
		};
		// clang-format on

	public:
		// clang-format off
		/** Serializes an instance of type `T` using the passed archive and forwarded arguments. */
		template<typename... Args>
		constexpr static void serialize(const T &value, Archive &ar, Args &&...args) requires(enable_serialize<Args...>)
		{
			if constexpr (mem_serialize<Args...>)
				value.serialize(ar, std::forward<Args>(args)...);
			else
				serialize(value, ar, std::forward<Args>(args)...);
		}
		/** Deserializes an instance of type `T` using the passed archive and forwarded arguments. */
		template<typename... Args>
		constexpr static void deserialize(T &value, Archive &ar, Args &&...args) requires(enable_deserialize<Args...>)
		{
			if constexpr (mem_deserialize<Args...>)
				value.deserialize(ar, std::forward<Args>(args)...);
			else
				deserialize(value, ar, std::forward<Args>(args)...);
		}

		/** Deserializes an instance of type `T` in-place using the passed archive and forwarded arguments. */
		template<typename... Args>
		[[nodiscard]] constexpr static T deserialize(std::in_place_t, Archive &ar, Args &&...args) requires(std::constructible_from<Archive, Args...> || in_place<Args...> || enable_deserialize<Args...>)
		{
			if constexpr (std::constructible_from<Archive, Args...>)
				return T{ar, std::forward<Args>(args)...};
			else if constexpr(in_place<Args...>)
				return deserialize(std::in_place_type<T>, ar, std::forward<Args>(args)...);
			else
			{
				T result;
				deserialize(result, ar, std::forward<Args>(args)...);
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
