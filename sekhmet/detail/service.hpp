//
// Created by switchblade on 18/05/22.
//

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