/*
 * Created by switchblade on 11/08/22
 */

#pragma once

#include <atomic>
#include <memory>

#include "delegate.hpp"

namespace sek
{
	template<typename T>
	class service;

	namespace detail
	{
		struct service_db;
	}

	template<>
	class service<void>
	{
		friend struct detail::service_db;

	public:
		/** @brief Runtime ID type used to uniquely identify services.
		 *
		 * Service uniqueness must ber guaranteed without reliance on runtime reflection, as such a runtime-generated
		 * unique ID is used. Every instance of `service::id` is guaranteed to be unique within the same process.
		 * Every user-defined specialization of `service<T>` must define a static instance of service ID. */
		class SEK_API id
		{
			friend class service;

			[[nodiscard]] static SEK_API std::size_t generate() noexcept;

		public:
			id(const id &) = delete;
			id &operator=(const id &) = delete;

			id() noexcept : m_id(generate()) {}

			[[nodiscard]] constexpr bool operator==(const id &) const noexcept = default;

		private:
			std::size_t m_id = 0;
		};

	protected:
		static SEK_API std::atomic<void *> &generate_ptr(const id *id) noexcept;
	};

	/** @brief Structure used to implement global services.
	 * @tparam T Type of service instance. */
	template<typename T>
	class SEK_API service : public service<void>
	{
		[[nodiscard]] static std::atomic<void *> &global_ptr() noexcept
		{
			static std::atomic<void *> &ptr = generate_ptr(&id);
			return ptr;
		}

	public:
		/** Global unique ID of the service. */
		static const id id;

		/** Returns pointer to the global instance of the service. */
		[[nodiscard]] static T *instance() noexcept { return static_cast<T *>(global_ptr().load()); }
		/** Exchanges the provided instance pointer with the global instance of the service.
		 * @param ptr Pointer to the new global instance.
		 * @return Pointer to the old global instance. */
		static T *instance(T *ptr) noexcept { return static_cast<T *>(global_ptr().exchange(ptr)); }
	};
}	 // namespace sek