//
// Created by switchblade on 2022-04-16.
//

#pragma once

#include "archive_traits.hpp"

namespace sek::serialization::detail
{
	template<typename A, typename T>
	constexpr void invoke_serialize(A &archive, T &&value) requires serializable_with<A, std::remove_reference_t<T>>
	{
		if constexpr (!std::is_pointer_v<T> && requires { value.serialize(archive); })
			value.serialize(archive);
		else
			serialize(archive, std::forward<T>(value));
	}
	template<typename A, typename T>
	constexpr void invoke_deserialize(A &archive, T &&value) requires deserializable_with<A, std::remove_reference_t<T>>
	{
		if constexpr (!std::is_pointer_v<T> && requires { value.deserialize(archive); })
			value.deserialize(archive);
		else
			deserialize(archive, std::forward<T>(value));
	}
}	 // namespace sek::serialization::detail