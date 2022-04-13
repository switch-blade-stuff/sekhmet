//
// Created by switchblade on 2022-04-04.
//

#pragma once

#include <atomic>

#include "define.h"

namespace sek
{
	/** @brief Base CRTP type for singleton systems.
	 * @note Must be explicitly instantiated & exported in order to link properly. */
	template<typename Child>
	struct basic_service
	{
		/** Sets the the global instance of the system.
		 * @param ptr System to be used as the global instance in case the global pointer is not initialized.
		 * @return Value of the global instance pointer before the operation. */
		static Child *instance(Child *p) { return global_ptr().exchange(p); }
		/** Returns pointer to the global instance of the system.
		 * @param ptr System to be used as the global instance in case the global pointer is not initialized.
		 * @return Pointer to the current global instance.
		 * @note In case the global instance is not set, initialized a default-constructed global instance. */
		static Child *instance()
		{
			auto &ptr = global_ptr();
			auto result = ptr.load();
			if (!result) [[unlikely]]
				ptr.store(result = local());
			return result;
		}

		static SEK_API_IMPORT std::atomic<Child *> &global_ptr() noexcept;

	private:
		static Child *local()
		{
			static Child value;
			return &value;
		}
	};
}	 // namespace sek