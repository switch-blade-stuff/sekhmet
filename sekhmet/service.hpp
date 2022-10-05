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
	template<typename T>
	class service;
	class service_locator;

	namespace detail
	{
		/* Helper attribute used to filter generic types. */
		struct service_impl_tag
		{
		};
		/* Generic base type of the `implements_service<S>` attribute. */
		struct service_attr_data
		{
			type_info m_instance_type;
			std::string_view m_name;
			std::string_view m_id;

			void (*m_deleter)(service<void> *);
			service<void> *(*m_factory)();
		};
	}	 // namespace detail
	namespace attributes
	{
		template<typename S>
		class implements_service;
	}

	/** @brief Common generic base type for all services. */
	template<>
	class service<void>
	{
	};

	/** @brief Global dynamic database of singleton services. */
	class SEK_API service_locator
	{
		template<typename>
		friend class service;

		using guard_t = access_guard<service_locator>;
		using handle_t = access_handle<service_locator, typename guard_t::unique_lock>;
		using attr_data_t = detail::service_attr_data;

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
			const auto service_type = type_info::get<T>();
			const auto attrib_type = type_info::get<attributes::implements_service<T>>();
			return static_cast<T *>(load_impl(service_type, attrib_type, type, true));
		}
		/** If the specified type has an `implements_service<T>` attribute, instantiates it as a service of type `T`
		 * if it is not already loaded.
		 * @param type Type that implements a service of type `T`.
		 * @return If the service is already loaded, pointer to the old instance. Otherwise,
		 * pointer to the loaded service, or `nullptr` if the type does not implement `T`. */
		template<typename T>
		[[nodiscard]] T *try_load(type_info type)
		{
			const auto service_type = type_info::get<T>();
			const auto attrib_type = type_info::get<attributes::implements_service<T>>();
			return static_cast<T *>(load_impl(service_type, attrib_type, type, false));
		}

		/** Locates a service implementation type with the specified id and instantiates it as a service of type `T`.
		 * @param id Id of the service implementation type, as specified by the `implements_service<T>` attribute.
		 * @return Pointer to the loaded service, or `nullptr` if the no such type is found.
		 * @note If the service is already loaded, replaces the old instance. */
		template<typename T>
		[[nodiscard]] T *load(std::string_view id)
		{
			const auto service_type = type_info::get<T>();
			const auto attrib_type = type_info::get<attributes::implements_service<T>>();
			return static_cast<T *>(load_impl(service_type, attrib_type, id, true));
		}
		/** Locates a service implementation type with the specified id and instantiates it as a service of type `T`,
		 * if it is not already loaded.
		 * @param id Id of the service implementation type, as specified by the `implements_service<T>` attribute.
		 * @return If the service is already loaded, pointer to the old instance. Otherwise,
		 * pointer to the loaded service, or `nullptr` if the no such type is found. */
		template<typename T>
		[[nodiscard]] T *try_load(std::string_view id)
		{
			const auto service_type = type_info::get<T>();
			const auto attrib_type = type_info::get<attributes::implements_service<T>>();
			return static_cast<T *>(load_impl(service_type, attrib_type, id, false));
		}

		// clang-format off
		/** Loads a service implementation object of type `U` as a service of type `T`.
		 * @param impl Pointer to the service implementation object instance.
		 * @return Pointer to the loaded service.
		 * @note If the service is already loaded, replaces the old instance. */
		template<typename T, typename U>
		[[nodiscard]] T *load(U *impl) requires std::is_base_of_v<T, U>
		{
			const auto impl_type = type_info::get<U>();
			const auto service_type = type_info::get<T>();
			const auto base_ptr = static_cast<service<T> *>(impl);

			const auto result = load_impl(service_type, impl_type, static_cast<service<void> *>(base_ptr), true));
			return static_cast<T *>(static_cast<service<T> *>(result));
		}
		/** Loads a service implementation object of type `U` as a service of type `T`, if it is not already loaded.
		 * @param impl Pointer to the service implementation object instance.
		 * @return If the service is already loaded, pointer to the old instance. Otherwise, pointer to the new instance. */
		template<typename T, typename U>
		[[nodiscard]] T *try_load(U *impl) requires std::is_base_of_v<T, U>
		{
			const auto impl_type = type_info::get<U>();
			const auto service_type = type_info::get<T>();
			const auto base_ptr = static_cast<service<T> *>(impl);

			const auto result = load_impl(service_type, impl_type, static_cast<service<void> *>(base_ptr), false);
			return static_cast<T *>(static_cast<service<T> *>(result));
		}
		// clang-format on

		/** If an implementation of service `T` exists, returns pointer to the service. Otherwise, returns `nullptr`. */
		template<typename T>
		[[nodiscard]] T *get()
		{
			return static_cast<T *>(get_impl(type_info::get<T>()).load());
		}
		/** If an implementation of service `T` exists, returns the actual `type_info` of the implementation object.
		 * Otherwise, returns an invalid `type_info`. */
		template<typename T>
		[[nodiscard]] type_info instance_type()
		{
			return instance_type_impl(type_info::get<T>());
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

		[[nodiscard]] service<void> *load_impl(type_info service_type, type_info impl_type, service<void> *impl, bool replace);
		[[nodiscard]] service<void> *load_impl(type_info service_type, type_info attr_type, type_info impl_type, bool replace);
		[[nodiscard]] service<void> *load_impl(type_info service_type, type_info attr_type, std::string_view id, bool replace);

		[[nodiscard]] std::atomic<service<void> *> &get_impl(type_info type);
		[[nodiscard]] type_info instance_type_impl(type_info type);

		[[nodiscard]] event<void()> &on_load_impl(type_info type);
		[[nodiscard]] event<void()> &on_reset_impl(type_info type);

		dense_map<std::string_view, std::unique_ptr<service_entry>> m_entries;
	};

	/** @brief Base type used to implement global services. Provides interface to the service locator.
	 * @tparam T Type of service instance. */
	template<typename T>
	class service : service<void>
	{
		template<typename>
		friend class attributes::implements_service;

		[[nodiscard]] static std::atomic<service<void> *> &global_ptr() noexcept
		{
			static auto &ptr_ref = service_locator::instance()->get_impl(type_info::get<T>());
			return ptr_ref;
		}

	public:
		/** Returns pointer to the global instance of the service. */
		[[nodiscard]] static T *instance() noexcept
		{
			/* Need to upcast from the generic `service<void>` pointer first. */
			const auto base_ptr = static_cast<service<T> *>(global_ptr().load());
			return static_cast<T *>(base_ptr);
		}
	};

	namespace attributes
	{
		/** @brief Service type used to declare a service implementation. */
		template<typename S>
		class implements_service : detail::service_attr_data
		{
		public:
			implements_service() = delete;
			implements_service(const implements_service &) = delete;
			implements_service &operator=(const implements_service &) = delete;
			implements_service(implements_service &&) = delete;
			implements_service &operator=(implements_service &&) = delete;

			// clang-format off
			/** Initializes an `implements_service` attribute for type `T`.
			 * @note Instance name and id are generated from the type name */
			template<typename T>
			constexpr implements_service(type_factory<T> &factory) noexcept
				requires std::is_base_of_v<S, T> : implements_service(factory, type_name<T>(), type_name<T>())
			{
			}
			/** Initializes an `implements_service` attribute for type `T`.
			 * @param name Debug name of the service instance.
			 * @note Instance id is generated from the type name */
			template<typename T>
			constexpr implements_service(type_factory<T> &factory, std::string_view name) noexcept
				requires std::is_base_of_v<S, T> : implements_service(factory, name, type_name<T>())
			{
			}
			// clang-format on

			/** Initializes an `implements_service` attribute for type `T`.
			 * @param name Debug name of the service instance.
			 * @param id Unique lookup id of the service instance. */
			template<typename T>
			constexpr implements_service(type_factory<T> &factory, std::string_view name, std::string_view id) noexcept
				requires std::is_base_of_v<S, T>
			{
				/* Add required metadata to the target type. */
				factory.template attribute<detail::service_impl_tag>();
				factory.template parent<S>();

				m_instance_type = factory.type();
				m_name = name;
				m_id = id;

				/* Ugly casts are needed since generic `service<void> *` is used. */
				m_deleter = +[](service<void> *ptr)
				{
					const auto base_ptr = static_cast<service<S> *>(ptr);
					delete static_cast<T *>(base_ptr);
				};
				m_factory = +[]() -> service<void> *
				{
					const auto base_ptr = static_cast<service<S> *>(new T{});
					return static_cast<service<void> *>(base_ptr);
				};
			}

			/** Returns the id of the service instance. */
			[[nodiscard]] constexpr std::string_view id() const noexcept { return m_id; }
			/** Returns the debug name of the service instance. */
			[[nodiscard]] constexpr std::string_view name() const noexcept { return m_name; }
		};
	}	 // namespace attributes
}	 // namespace sek