//
// Created by switchblade on 20/05/22.
//

#pragma once

#include "../archive_traits.hpp"
#include "ranges.hpp"
#include "tuples.hpp"

namespace sek::serialization
{
	// clang-format off
	template<typename T, typename A, typename... Args>
	concept adl_serializable = requires(T &&v, A &ar, Args &&...args) { serialize(v, ar, std::forward<Args>(args)...); };
	template<typename T, typename A, typename... Args>
	concept member_serializable = requires(T &&v, A &ar, Args &&...args) { v.serialize(ar, std::forward<Args>(args)...); };
	template<typename T, typename A, typename... Args>
	concept serializable = adl_serializable<T, A, Args...> || member_serializable<T, A, Args...>;

	template<typename T, typename A, typename... Args>
	concept adl_deserializable = requires(T &&v, A &ar, Args &&...args) { deserialize(v, ar, std::forward<Args>(args)...); };
	template<typename T, typename A, typename... Args>
	concept member_deserializable = requires(T &&v, A &ar, Args &&...args) { v.deserialize(ar, std::forward<Args>(args)...); };
	template<typename T, typename A, typename... Args>
	concept deserializable = adl_deserializable<T, A, Args...> || member_deserializable<T, A, Args...>;
	template<typename T, typename A, typename... Args>
	concept in_place_deserializable = requires(A &ar, Args &&...args) { deserialize(std::in_place_type<T>, ar, std::forward<Args>(args)...); };
	// clang-format on
}	 // namespace sek::serialization