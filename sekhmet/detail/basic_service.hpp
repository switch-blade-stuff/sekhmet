//
// Created by switchblade on 2022-04-04.
//

#pragma once

#include <atomic>
#include <mutex>

namespace sek
{
	/** @brief Base CRTP type for singleton systems.
	 * @note Must be explicitly instantiated & exported in order to link properly. */
	template<typename Child>
	struct basic_service
	{
		/** Returns pointer to the global instance of the system. If the global instance is not initialized,
		 * uses the passed pointer as the global instance.
		 * @param ptr System to be used as the global instance in case the global pointer is not initialized.
		 * @return Pointer to the current global instance.
		 * @note In case the passed pointer is null, initialized a default-constructed global instance. */
		static Child *instance(Child *ptr = nullptr)
		{
			Child *result = data.ptr.load(std::memory_order::acquire);
			if (!result) [[unlikely]]
			{
				std::lock_guard<std::mutex> l(data.mtx);
				if (!(result = data.ptr.load(std::memory_order::relaxed)))
				{
					result = ptr ? ptr : local();
					data.ptr.store(result, std::memory_order::release);
				}
			}
			return result;
		}

	private:
		struct sync_data
		{
			mutable std::mutex mtx;
			std::atomic<Child *> ptr;
		};

		static Child *local();

		static sync_data data;
	};

	template<typename T>
	T *basic_service<T>::local()
	{
		static T value;
		return &value;
	}
	template<typename T>
	typename basic_service<T>::sync_data basic_service<T>::data = {};
}	 // namespace sek