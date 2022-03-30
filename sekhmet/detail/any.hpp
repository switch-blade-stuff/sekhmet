//
// Created by switchblade on 2022-02-06.
//

#pragma once

#include <utility>

#include "../math/detail/util.hpp"
#include "aligned_storage.hpp"
#include "type_info.hpp"

namespace sek
{
	class any
	{
	private:
		enum flags_t
		{
			IS_EXTERNAL = 0b1,
			IS_CONST = 0b10,
		};

		struct private_storage
		{
			[[nodiscard]] constexpr void *heap_ptr() noexcept { return data.get<void *>(); }
			[[nodiscard]] constexpr const void *heap_ptr() const noexcept { return data.get<void *>(); }

			template<typename T>
			[[nodiscard]] constexpr T *get_local() noexcept { return ; }
			template<typename T>
			[[nodiscard]] constexpr const T *get_local() const noexcept { return data.get<void *>(); }

			type_storage<void *> data;
		};
		union external_storage
		{
			void *mutable_data;
			const void *const_data;
		};

	public:
	private:
		flags_t flags;
		union
		{
			private_storage private_data;
			external_storage external_data;
		};
	};

	template<forward_iterator_for<any> Iter>
	constexpr void type_info::constructor_info::invoke(void *ptr, Iter args_begin, Iter args_end) const
	{
		/* TODO: Implement this. */
	}

	constexpr any type_info::get_attribute(type_id id) const noexcept
	{
		return attribute_info{data->get_attribute(id)}.get();
	}
	constexpr any type_info::attribute_info::get() const noexcept
	{
		/* TODO: Implement this. */
		return {};
	}
}	 // namespace sek