//
// Created by switchblade on 2022-04-24.
//

#pragma once

#include <vector>

#include "define.h"
#include "ebo_base_helper.hpp"
#include <string_view>

namespace sek
{
	/** @brief Handle used to reference and manage plugins. */
	class plugin
	{
		struct plugin_info
		{
			std::string_view (*id)(const void *);
			bool (*enable)(void *);
			void (*disable)(void *);
		};

		/* Implemented in plugin.cpp */
		struct plugin_entry;
		struct plugin_db;

		SEK_API static plugin_db &database();
		SEK_API static void load_plugin(const plugin_info *, void *) noexcept;
		SEK_API static void unload_plugin(const plugin_info *, void *) noexcept;

		template<typename T>
		class registrar : ebo_base_helper<T>
		{
			static registrar instance;

			using ebo_base = ebo_base_helper<T>;

			// clang-format off
			static_assert(requires(T *p) { { p->id() } -> std::convertible_to<std::string_view>; } ||
						  requires { { T::id() } -> std::convertible_to<std::string_view>; },
						  "Plugin must implement `std::string_view id() const` member function");
			// clang-format on

		public:
			constexpr registrar() noexcept(noexcept(ebo_base{})) { load_plugin(&info, ebo_base::get()); }
			constexpr ~registrar() { unload_plugin(&info, ebo_base::get()); }

		private:
			const plugin_info info = {
				+[](const void *p)
				{
					// clang-format off
					if constexpr (requires { static_cast<T *>(p)->id(); })
						return static_cast<T *>(p)->id();
					else
						return T::id();
					// clang-format on
				},
				+[](void *p)
				{
					// clang-format off
					if constexpr (requires { { static_cast<T *>(p)->on_enable() } -> std::same_as<bool>; })
						return static_cast<T *>(p)->on_enable();
					else if constexpr (requires { { T::on_enable() } -> std::same_as<bool>; })
						return T::on_enable();
					else
					{
						if constexpr (requires { static_cast<T *>(p)->on_enable(); })
							static_cast<T *>(p)->on_enable();
						else if constexpr (requires { T::on_enable(); })
							T::on_enable();
						return true;
					}
					// clang-format on
				},
				+[](void *p)
				{
					if constexpr (requires { static_cast<T *>(p)->on_disable(); })
						static_cast<T *>(p)->on_disable();
					else if constexpr (requires { T::on_disable(); })
						T::on_disable();
				},
			};
		};

		template<typename>
		friend class registrar;

	public:
		/** Returns a vector of all currently loaded plugins. */
		SEK_API static std::vector<plugin> get_loaded();
		/** Returns a vector of all currently enabled plugins. */
		SEK_API static std::vector<plugin> get_enabled();

		/** Returns a plugin using it's id. If such plugin does not exist, returns an empty handle. */
		SEK_API static plugin get(std::string_view id);

	private:
		constexpr explicit plugin(plugin_entry *entry) noexcept : entry(entry) {}

	public:
		/** Initializes an empty plugin handle. */
		constexpr plugin() noexcept = default;

		/** Checks if the plugin handle is empty. */
		[[nodiscard]] constexpr bool empty() const noexcept { return entry == nullptr; }
		/** @copydoc empty */
		[[nodiscard]] constexpr operator bool() const noexcept { return !empty(); }

		/** Checks if the plugin is enabled. */
		[[nodiscard]] SEK_API bool enabled() const noexcept;
		/** Returns id of the plugin. */
		[[nodiscard]] SEK_API std::string_view id() const noexcept;
		/** Returns pointer to the instance of the plugin. */
		[[nodiscard]] SEK_API void *data() const noexcept;

		/** Enables the plugin and invokes it's `on_enable` member function.
		 * @returns true on success, false otherwise.
		 * @note Plugin will fail to enable if it is already enabled or if `on_enable` returned false or threw an exception. */
		[[nodiscard]] SEK_API bool enable() const noexcept;
		/** Disables the plugin and invokes it's `on_disable` member function.
		 * @returns true on success, false otherwise.
		 * @note Plugin will fail to disable if it is not enabled. */
		[[nodiscard]] SEK_API bool disable() const noexcept;

		[[nodiscard]] constexpr auto operator<=>(const plugin &) const noexcept = default;
		[[nodiscard]] constexpr bool operator==(const plugin &) const noexcept = default;

	private:
		plugin_entry *entry = nullptr;
	};
}	 // namespace sek

/** @brief Macro used to define and auto-load an instance of a plugin. */
#define SEK_PLUGIN_INSTANCE(plugin_type)                                                                               \
	template<>                                                                                                         \
	sek::plugin::registrar<plugin_type> sek::plugin::registrar<plugin_type>::instance = {};
