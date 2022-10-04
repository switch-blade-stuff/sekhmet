//
// Created by switch_blade on 2022-10-03.
//

#pragma once

#include "type_data.hpp"

namespace sek
{
	/** @brief Structure used to reflect information about a type. */
	template<typename T>
	class type_factory
	{
		friend class type_info;
		friend class type_database;

		using handle_t = detail::type_handle;

		constexpr explicit type_factory(handle_t handle) noexcept : m_handle(handle) {}

	public:
		constexpr ~type_factory() { submit(); }
		constexpr type_factory(const type_factory &) noexcept = default;
		constexpr type_factory &operator=(const type_factory &) noexcept = default;
		constexpr type_factory(type_factory &&) noexcept = default;
		constexpr type_factory &operator=(type_factory &&) noexcept = default;

		/** Finalizes the type and inserts it into the type database.
		 * @note After call to `submit` type factory is in invalid state. */
		inline void submit();

	private:
		handle_t m_handle;
	};
}	 // namespace sek