/*
 * ============================================================================
 * Sekhmet - C++20 game engine & editor
 * Copyright (C) 2022 switchblade
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 * ============================================================================
 *
 * Created by switchblade on 18/05/22
 */

#pragma once

#include <atomic>

#include "define.h"
#include "event.hpp"

namespace sek
{
	/** @brief Structure used to implement global services.
	 * @tparam T Type of service instance. */
	template<typename T>
	class SEK_API service
	{
		static std::atomic<T *> &global_ptr() noexcept;

	public:
		static T *instance(T *new_ptr) noexcept { return global_ptr().exchange(new_ptr); }
		static T *instance() noexcept { return global_ptr().load(); }
	};

	template<typename T>
	std::atomic<T *> &service<T>::global_ptr() noexcept
	{
		static std::atomic<T *> ptr;
		return ptr;
	}
}	 // namespace sek