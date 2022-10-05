/*
 * Created by switchblade on 11/08/22
 */

#pragma once

#include <atomic>
#include <memory>

#include "access_guard.hpp"
#include "dense_map.hpp"
#include "detail/type_info/type_info.hpp"
#include "event.hpp"

namespace sek
{
	/** @brief Customization point for service types.
	 *
	 * A `service_traits` specialization must have a `type` member alias of `T`.
	 * Additionally, it may define the following member types:
	 * <ul>
	 * <li>`mutex_type` - type used for synchronization of the service. If undefined, the service will be unsynchronized.</li>
	 * </ul> */
	template<typename>
	struct service_traits;

	/** @brief Concept used to check that a type has a well-format `service_traits<T>` overload. */
	template<typename T>
	concept service_type = requires { typename service_traits<T>::type; };
	/** @brief Concept used to check that a type has a well-format `service_traits` overload with a valid `mutex_type` member. */
	template<typename T>
	concept synchronized_service = service_type<T> && requires { typename service_traits<T>::mutex_type; };

	template<service_type T>
	class service;
	class service_locator;

	namespace detail
	{
		/* Generic service storage. */
		template<typename...>
		struct service_storage;
		template<>
		struct SEK_API service_storage<void>
		{
			virtual ~service_storage();
		};

		template<service_type S>
		struct SEK_API service_storage<S> : service_storage<void>
		{
			using value_type = typename service_traits<S>::type;

			virtual ~service_storage() override = default;
			virtual value_type *get() noexcept = 0;
		};
		template<synchronized_service S>
		struct SEK_API service_storage<S> : service_storage<void>
		{
			using value_type = typename service_traits<S>::type;
			using mutex_type = typename service_traits<S>::mutex_type;

			virtual ~service_storage() override = default;
			virtual value_type *get() noexcept = 0;
			virtual mutex_type *mutex() noexcept = 0;
		};

		template<service_type S, typename U>
		struct service_storage<S, U> final : service_storage<S>
		{
			using value_type = typename service_traits<S>::type;

			~service_storage() final = default;
			value_type *get() noexcept final { return &instance; };

			value_type instance;
		};
		template<synchronized_service S, typename U>
		struct service_storage<S, U> final : service_storage<S>
		{
			using value_type = typename service_traits<S>::type;
			using mutex_type = typename service_traits<S>::mutex_type;

			~service_storage() final = default;
			value_type *get() noexcept final { return &instance; };
			mutex_type *mutex() noexcept final { return &mtx; };

			value_type instance;
			mutex_type mtx;
		};

		/* Helper attribute used to filter generic types. */
		struct service_impl_tag
		{
		};
		/* Generic base type of the `implements_service<S>` attribute. */
		struct service_attr_data
		{
			service_storage<void> *(*m_factory)();
			type_info m_instance_type;
			std::string_view m_name;
			std::string_view m_id;
		};
	}	 // namespace detail
	namespace attributes
	{
		template<typename S>
		class implements_service;
	}

	/** @brief Global dynamic database of singleton services. */
	class SEK_API service_locator
	{
		template<service_type>
		friend class service;

		using guard_t = access_guard<service_locator, std::recursive_mutex>;
		using storage_t = detail::service_storage<void>;
		using attr_data_t = detail::service_attr_data;
		using factory_t = storage_t *(*) ();

		struct service_entry;

	public:
		/** Returns access handle to the global service locator instance. */
		[[nodiscard]] static guard_t instance() noexcept;

	private:
		service_locator() = default;

	public:
		service_locator(const service_locator &) = delete;
		service_locator &operator=(const service_locator &) = delete;
		service_locator(service_locator &&) = delete;
		service_locator &operator=(service_locator &&) = delete;

		/** Resets the service `T`, releasing the implementation instance if it is loaded. */
		template<service_type T>
		void reset()
		{
			reset_impl(type_info::get<T>());
		}

		/** If the specified type has an `implements_service<T>` attribute, instantiates it as a service of type `T`.
		 * @param type Type that implements a service of type `T`.
		 * @return Access guard or pointer to the loaded service, or an empty guard or `nullptr` if the type does not implement `T`.
		 * @note If the service is already loaded, replaces the old instance. */
		template<service_type T>
		decltype(auto) load(type_info type);
		/** If the specified type has an `implements_service<T>` attribute, instantiates it as a service of type `T`
		 * if it is not already loaded.
		 * @param type Type that implements a service of type `T`.
		 * @return If the service is already loaded, access guard or pointer to the old instance. Otherwise,
		 * access guard or pointer to the loaded service, or an empty guard or `nullptr` if the type does not implement `T`. */
		template<service_type T>
		decltype(auto) try_load(type_info type);

		/** Locates a service implementation type with the specified id and instantiates it as a service of type `T`.
		 * @param id Id of the service implementation type, as specified by the `implements_service<T>` attribute.
		 * @return Access guard or pointer to the loaded service, or an empty guard or `nullptr` if the no such type is found.
		 * @note If the service is already loaded, replaces the old instance. */
		template<service_type T>
		decltype(auto) load(std::string_view id);
		/** Locates a service implementation type with the specified id and instantiates it as a service of type `T`,
		 * if it is not already loaded.
		 * @param id Id of the service implementation type, as specified by the `implements_service<T>` attribute.
		 * @return If the service is already loaded, access guard or pointer to the old instance. Otherwise,
		 * access guard or pointer to the loaded service, or an empty guard or `nullptr` if the no such type is found. */
		template<service_type T>
		decltype(auto) try_load(std::string_view id);

		// clang-format off
		/** Creates a service implementation object of type `U` in-place.
		 * @return Access guard or unsynchronized pointer to the created service.
		 * @note If the service is already loaded, replaces the old instance. */
		template<service_type T, typename U>
		decltype(auto) load(std::in_place_type_t<U>) requires std::is_base_of_v<T, U>;
		/** Creates a service implementation object of type `U` in-place.
		 * @return Access guard or unsynchronized pointer to either the old instance, or the new instance if the service was not loaded yet. */
		template<service_type T, typename U>
		decltype(auto) try_load(std::in_place_type_t<U>) requires std::is_base_of_v<T, U>;
		// clang-format on

		/** If an implementation of service `T` exists, returns access guard or pointer to the service. Otherwise, returns empty guard or `nullptr`. */
		template<service_type T>
		[[nodiscard]] decltype(auto) get();
		/** If an implementation of service `T` exists, returns the actual `type_info` of the implementation object. Otherwise, returns an invalid `type_info`. */
		template<service_type T>
		[[nodiscard]] type_info instance_type()
		{
			return instance_type_impl(type_info::get<T>());
		}

		/** Returns event proxy for service load event for service type `T`. This event
		 * is invoked when a new service instance loaded via either `load` or `try_load`. */
		template<service_type T>
		[[nodiscard]] event_proxy<event<void()>> on_load()
		{
			return {on_load_impl(type_info::get<T>())};
		}
		/** Returns event proxy for service reset event for service type `T`. This event is invoked when a
		 * service instance is reset either via `reset`, or when a new service instance is loaded via `load`. */
		template<service_type T>
		[[nodiscard]] event_proxy<event<void()>> on_reset()
		{
			return {on_reset_impl(type_info::get<T>())};
		}

	private:
		[[nodiscard]] service_entry &get_entry(type_info type);

		void reset_impl(type_info type);

		[[nodiscard]] storage_t *load_impl(type_info service_type, type_info impl_type, factory_t factory, bool replace);
		[[nodiscard]] storage_t *load_impl(type_info service_type, type_info attr_type, type_info impl_type, bool replace);
		[[nodiscard]] storage_t *load_impl(type_info service_type, type_info attr_type, std::string_view id, bool replace);

		[[nodiscard]] std::atomic<storage_t *> &get_impl(type_info type);
		[[nodiscard]] type_info instance_type_impl(type_info type);

		[[nodiscard]] event<void()> &on_load_impl(type_info type);
		[[nodiscard]] event<void()> &on_reset_impl(type_info type);

		dense_map<std::string_view, std::unique_ptr<service_entry>> m_entries;
	};

	/** @brief Base type used to implement global singleton services. Provides interface to the service locator.
	 * @tparam T Type of service instance. */
	template<service_type T>
	class service
	{
		friend class service_locator;
		template<typename>
		friend class attributes::implements_service;

		using instance_type = T *;

		using base_storage_t = detail::service_storage<void>;
		template<typename U>
		using instance_storage_t = detail::service_storage<T, U>;
		using storage_t = detail::service_storage<T>;

		template<typename U>
		static base_storage_t *factory()
		{
			return new instance_storage_t<U>{};
		}

		[[nodiscard]] static std::atomic<base_storage_t *> &global_ptr() noexcept
		{
			/* Cache the entry pointer. */
			static auto &ptr = service_locator::instance()->get_impl(type_info::get<T>());
			return ptr;
		}
		[[nodiscard]] constexpr static instance_type cast(base_storage_t *ptr) noexcept
		{
			return static_cast<storage_t *>(ptr)->get();
		}

	public:
		/** Returns an unsynchronized pointer to the global service instance. */
		[[nodiscard]] static instance_type instance() noexcept { return cast(global_ptr().load()); }
	};
	/** @brief `service` overload for synchronized service types. */
	template<synchronized_service T>
	class service<T>
	{
		friend class service_locator;
		template<typename>
		friend class attributes::implements_service;

		using value_type = typename service_traits<T>::type;
		using mutex_type = typename service_traits<T>::mutex_type;
		using instance_type = access_guard<value_type, mutex_type>;

		using base_storage_t = detail::service_storage<void>;
		template<typename U>
		using instance_storage_t = detail::service_storage<T, U>;
		using storage_t = detail::service_storage<T>;

		template<typename U>
		static base_storage_t *factory()
		{
			return new instance_storage_t<U>{};
		}

		[[nodiscard]] static std::atomic<base_storage_t *> &global_ptr() noexcept
		{
			/* Cache the entry pointer. */
			static auto &ptr = service_locator::instance()->get_impl(type_info::get<T>());
			return ptr;
		}
		[[nodiscard]] constexpr static instance_type cast(base_storage_t *ptr) noexcept
		{
			const auto storage = static_cast<storage_t *>(ptr);
			return instance_type{storage->get(), storage->mutex()};
		}

	public:
		/** Returns an access guard to the global service instance. */
		[[nodiscard]] static instance_type instance() noexcept { return cast(global_ptr().load()); }
	};

	template<service_type T>
	decltype(auto) service_locator::load(type_info type)
	{
		const auto service_type = type_info::get<T>();
		const auto attrib_type = type_info::get<attributes::implements_service<T>>();
		return service<T>::cast(load_impl(service_type, attrib_type, type, true));
	}
	template<service_type T>
	decltype(auto) service_locator::try_load(type_info type)
	{
		const auto service_type = type_info::get<T>();
		const auto attrib_type = type_info::get<attributes::implements_service<T>>();
		return service<T>::cast(load_impl(service_type, attrib_type, type, false));
	}

	template<service_type T>
	decltype(auto) service_locator::load(std::string_view id)
	{
		const auto service_type = type_info::get<T>();
		const auto attrib_type = type_info::get<attributes::implements_service<T>>();
		return service<T>::cast(load_impl(service_type, attrib_type, id, true));
	}
	template<service_type T>
	decltype(auto) service_locator::try_load(std::string_view id)
	{
		const auto service_type = type_info::get<T>();
		const auto attrib_type = type_info::get<attributes::implements_service<T>>();
		return service<T>::cast(load_impl(service_type, attrib_type, id, false));
	}

	// clang-format off
	template<service_type T, typename U>
	decltype(auto) service_locator::load(std::in_place_type_t<U>) requires std::is_base_of_v<T, U>
	{
		const auto impl_type = type_info::get<U>();
		const auto service_type = type_info::get<T>();
		const auto factory = service<T>::template factory<U>;
		return service<T>::cast(load_impl(service_type, impl_type, factory, true));
	}
	template<service_type T, typename U>
	decltype(auto) service_locator::try_load(std::in_place_type_t<U>) requires std::is_base_of_v<T, U>
	{
		const auto impl_type = type_info::get<U>();
		const auto service_type = type_info::get<T>();
		const auto factory = service<T>::template factory<U>;
		return service<T>::cast(load_impl(service_type, impl_type, factory, false));
	}
	// clang-format on

	template<service_type T>
	decltype(auto) service_locator::get()
	{
		return service<T>::cast(get_impl(type_info::get<T>()).load());
	}

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

				/* Initialize the attribute data. */
				m_factory = service<S>::template factory<T>;
				m_instance_type = factory.type();
				m_name = name;
				m_id = id;
			}

			/** Returns the id of the service instance. */
			[[nodiscard]] constexpr std::string_view id() const noexcept { return m_id; }
			/** Returns the debug name of the service instance. */
			[[nodiscard]] constexpr std::string_view name() const noexcept { return m_name; }
		};
	}	 // namespace attributes
}	 // namespace sek