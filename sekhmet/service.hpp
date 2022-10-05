/*
 * Created by switchblade on 11/08/22
 */

#pragma once

#include <atomic>
#include <memory>

#include "access_guard.hpp"
#include "dense_map.hpp"
#include "event.hpp"
#include "type_info.hpp"

namespace sek
{
	class service_locator;
	template<typename T>
	class SEK_API service;

	/** @brief Global dynamic database of singleton services. */
	class SEK_API service_locator
	{
		template<typename>
		friend class service;

		using guard_t = access_guard<service_locator>;
		using handle_t = access_handle<service_locator, typename guard_t::unique_lock>;

		struct service_entry;

	public:
		/** Returns access handle to the global service locator instance. */
		[[nodiscard]] static handle_t instance() noexcept;

	private:
		service_locator() = default;

	public:
		service_locator(const service_locator &) = delete;
		service_locator &operator=(const service_locator &) = delete;
		service_locator(service_locator &&) = delete;
		service_locator &operator=(service_locator &&) = delete;

		/** Resets the service `T`, releasing the implementation instance if it is loaded. */
		template<typename T>
		void reset()
		{
			reset_impl(type_info::get<T>());
		}

		/** If the specified type has an `implements_service<T>` attribute, instantiates it as a service of type `T`.
		 * @param type Type that implements a service of type `T`.
		 * @return Pointer to the loaded service, or `nullptr` if the type does not implement `T`.
		 * @note If the service is already loaded, replaces the old instance. */
		template<typename T>
		[[nodiscard]] T *load(type_info type)
		{
			return static_cast<T *>(load_impl(type_info::get<T>(), type, true));
		}
		/** If the specified type has an `implements_service<T>` attribute, instantiates it as a service of type `T`
		 * if it is not already loaded.
		 * @param type Type that implements a service of type `T`.
		 * @return If the service is already loaded, pointer to the old instance. Otherwise,
		 * pointer to the loaded service, or `nullptr` if the type does not implement `T`. */
		template<typename T>
		[[nodiscard]] T *try_load(type_info type)
		{
			return static_cast<T *>(load_impl(type_info::get<T>(), type, false));
		}

		/** Locates a service implementation type with the specified id and instantiates it as a service of type `T`.
		 * @param id Id of the service implementation type, as specified by the `implements_service<T>` attribute.
		 * @return Pointer to the loaded service, or `nullptr` if the no such type is found.
		 * @note If the service is already loaded, replaces the old instance. */
		template<typename T>
		[[nodiscard]] T *load(std::string_view id)
		{
			return static_cast<T *>(load_impl(type_info::get<T>(), id, true));
		}
		/** Locates a service implementation type with the specified id and instantiates it as a service of type `T`,
		 * if it is not already loaded.
		 * @param id Id of the service implementation type, as specified by the `implements_service<T>` attribute.
		 * @return If the service is already loaded, pointer to the old instance. Otherwise,
		 * pointer to the loaded service, or `nullptr` if the no such type is found. */
		template<typename T>
		[[nodiscard]] T *try_load(std::string_view id)
		{
			return static_cast<T *>(load_impl(type_info::get<T>(), id, false));
		}

		// clang-format off
		/** Loads a service implementation object of type `U` as a service of type `T`.
		 * @param impl Pointer to the service implementation object instance.
		 * @return Pointer to the loaded service.
		 * @note If the service is already loaded, replaces the old instance. */
		template<typename T, typename U>
		[[nodiscard]] T *load(U *impl) requires std::is_base_of_v<T, U>
		{
			return static_cast<T *>(load_impl(type_info::get<T>(), impl, true));
		}
		/** Loads a service implementation object of type `U` as a service of type `T`, if it is not already loaded.
		 * @param impl Pointer to the service implementation object instance.
		 * @return If the service is already loaded, pointer to the old instance. Otherwise, pointer to the new instance. */
		template<typename T, typename U>
		[[nodiscard]] T *try_load(U *impl) requires std::is_base_of_v<T, U>
		{
			return static_cast<T *>(load_impl(type_info::get<T>(), impl, false));
		}
		// clang-format on

		/** If an implementation of service `T` exists, returns pointer to the service. Otherwise, returns `nullptr`. */
		template<typename T>
		[[nodiscard]] T *get() const
		{
			return static_cast<T *>(get_impl(type_info::get<T>()).load());
		}

		/** Returns event proxy for service load event for service type `T`. This event
		 * is invoked when a new service instance loaded via either `load` or `try_load`. */
		template<typename T>
		[[nodiscard]] event_proxy<event<void()>> on_load()
		{
			return {on_load_impl(type_info::get<T>())};
		}
		/** Returns event proxy for service reset event for service type `T`. This event is invoked when a
		 * service instance is reset either via `reset`, or when a new service instance is loaded via `load`. */
		template<typename T>
		[[nodiscard]] event_proxy<event<void()>> on_reset()
		{
			return {on_reset_impl(type_info::get<T>())};
		}

	private:
		[[nodiscard]] service_entry &get_entry(type_info type);

		void reset_impl(type_info type);
		[[nodiscard]] std::atomic<void *> &get_impl(type_info type);
		[[nodiscard]] void *load_impl(type_info service_type, void *impl, bool replace);
		[[nodiscard]] void *load_impl(type_info service_type, type_info impl_type, bool replace);
		[[nodiscard]] void *load_impl(type_info service_type, std::string_view impl_id, bool replace);

		[[nodiscard]] event<void()> &on_load_impl(type_info type);
		[[nodiscard]] event<void()> &on_reset_impl(type_info type);

		dense_map<std::string_view, std::unique_ptr<service_entry>> m_entries;
	};

	/** @brief Base type used to implement global services. Provides interface to the service locator.
	 * @tparam T Type of service instance. */
	template<typename T>
	class SEK_API service
	{
		[[nodiscard]] static std::atomic<void *> &global_ptr() noexcept
		{
			static auto &ptr_ref = service_locator::instance()->get_impl(type_info::get<T>());
			return ptr_ref;
		}

	public:
		/** Returns pointer to the global instance of the service. */
		[[nodiscard]] static T *instance() noexcept { return static_cast<T *>(global_ptr().load()); }
	};
}	 // namespace sek