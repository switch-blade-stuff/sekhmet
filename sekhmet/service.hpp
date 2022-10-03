/*
 * Created by switchblade on 11/08/22
 */

#pragma once

#include <atomic>
#include <memory>

#include "delegate.hpp"

namespace sek
{
	/** @brief Structure used to implement global services.
	 * @tparam T Type of service instance. */
	template<typename T>
	class SEK_API service
	{
		[[nodiscard]] static std::atomic<void *> &global_ptr() noexcept
		{
			constinit static std::atomic<void *> ptr;
			return ptr;
		}

	public:
		/** Returns pointer to the global instance of the service. */
		[[nodiscard]] static T *instance() noexcept { return static_cast<T *>(global_ptr().load()); }
		/** Exchanges the provided instance pointer with the global instance of the service.
		 * @param ptr Pointer to the new global instance.
		 * @return Pointer to the old global instance. */
		static T *instance(T *ptr) noexcept { return static_cast<T *>(global_ptr().exchange(ptr)); }
	};
}	 // namespace sek

/** @brief Helper macro used to define a template instance export for a service type. Must be placed in a source file. */
#define SEK_EXPORT_SERVICE(T) template class SEK_API_EXPORT sek::service<T>;
/** @brief Helper macro used to declare a template instance export for a service type. Must be placed in a header file. */
#define SEK_EXTERN_SERVICE(T) extern template class SEK_API_IMPORT sek::service<T>;