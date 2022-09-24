/*
 * Created by switchblade on 12/05/22
 */

#pragma once

#include <vector>

#include "../event.hpp"
#include "../static_string.hpp"
#include "../version.hpp"

namespace sek::engine
{
	namespace detail
	{
		struct plugin_info
		{
			consteval plugin_info(version engine_ver, version plugin_ver, std::string_view id) noexcept
				: engine_ver(engine_ver), plugin_ver(plugin_ver), id(id)
			{
			}

			/** Version of the engine the plugin was compiled for. */
			const version engine_ver;
			/** Version of the plugin. */
			const version plugin_ver;
			/** Id of the plugin. */
			const std::string_view id;
		};
		struct plugin_data
		{
			enum status_t
			{
				INITIAL,
				DISABLED,
				ENABLED,
			};

			SEK_API static void load(plugin_data *data, void (*init)(void *));
			static void load_impl(plugin_data *data, void (*init)(void *));

			SEK_API static void unload(plugin_data *data);
			static void unload_impl(plugin_data *data);

			explicit plugin_data(plugin_info info) noexcept : info(info) {}

			[[nodiscard]] bool enable() const
			{
				bool result;
				on_enable.dispatch([&result](bool b) { return (result = b); });
				return result;
			}
			void disable() const { on_disable(); }

			/** Compile-time information about this plugin. */
			const plugin_info info;
			/** Event dispatched when a plugin is enabled by the engine. */
			event<bool(void)> on_enable;
			/** Event dispatched when a plugin is disabled by the engine. */
			event<void(void)> on_disable;

			status_t status;
		};

		template<typename Child>
		class plugin_base : plugin_data
		{
		public:
			static Child &instance();

			explicit plugin_base(plugin_info info) noexcept : plugin_data(info)
			{
				plugin_data::load(this, [](void *p) { static_cast<Child *>(p)->init(); });
			}

			using plugin_data::info;
			using plugin_data::on_disable;
			using plugin_data::on_enable;
		};
	}	 // namespace detail

	/** @brief Handle used to reference and manage plugins. */
	class plugin
	{
	public:
		/** Returns a vector of all currently loaded plugins. */
		SEK_API static std::vector<plugin> get_loaded();
		/** Returns a vector of all currently enabled plugins. */
		SEK_API static std::vector<plugin> get_enabled();

		/** Returns a plugin using it's id. If such plugin does not exist, returns an empty handle. */
		SEK_API static plugin get(std::string_view id);

	private:
		constexpr explicit plugin(detail::plugin_data *data) noexcept : m_data(data) {}

	public:
		/** Initializes an empty plugin handle. */
		constexpr plugin() noexcept = default;

		/** Checks if the plugin handle is empty. */
		[[nodiscard]] constexpr bool empty() const noexcept { return m_data == nullptr; }
		/** @copydoc empty */
		[[nodiscard]] constexpr operator bool() const noexcept { return !empty(); }

		/** Returns id of the plugin. */
		[[nodiscard]] constexpr std::string_view id() const noexcept { return m_data->info.id; }
		/** Returns engine version of the plugin. */
		[[nodiscard]] constexpr version engine_ver() const noexcept { return m_data->info.engine_ver; }

		/** Checks if the plugin is enabled. */
		[[nodiscard]] SEK_API bool enabled() const noexcept;
		/** Enables the plugin and invokes it's `on_enable` member function.
		 * @returns true on success, false otherwise.
		 * @note Plugin will fail to enable if it is already enabled or not loaded or if `on_enable` returned false or threw an exception. */
		[[nodiscard]] SEK_API bool enable() const noexcept;
		/** Disables the plugin and invokes it's `on_disable` member function.
		 * @returns true on success, false otherwise.
		 * @note Plugin will fail to disable if it is not enabled or not loaded. */
		[[nodiscard]] SEK_API bool disable() const noexcept;

		[[nodiscard]] constexpr auto operator<=>(const plugin &) const noexcept = default;
		[[nodiscard]] constexpr bool operator==(const plugin &) const noexcept = default;

	private:
		detail::plugin_data *m_data = nullptr;
	};
}	 // namespace sek::engine

namespace impl
{
	template<typename F>
	struct static_exec
	{
		constexpr static_exec() noexcept : static_exec(F{}) {}
		constexpr static_exec(F &&f) noexcept { std::invoke(f); }
	};
	template<typename F>
	static_exec(F &&) -> static_exec<F>;

	template<sek::basic_static_string Id>
	class plugin_instance : sek::engine::detail::plugin_base<plugin_instance<Id>>
	{
		friend class sek::engine::detail::plugin_base<plugin_instance>;

		static static_exec<void (*)()> bootstrap;

	public:
		using sek::engine::detail::plugin_base<plugin_instance<Id>>::instance;
		using sek::engine::detail::plugin_base<plugin_instance<Id>>::on_disable;
		using sek::engine::detail::plugin_base<plugin_instance<Id>>::on_enable;
		using sek::engine::detail::plugin_base<plugin_instance<Id>>::info;

	private:
		plugin_instance();
		void init();
	};

	template<sek::basic_static_string Func, sek::basic_static_string Plugin>
	struct exec_on_plugin_enable : static_exec<exec_on_plugin_enable<Func, Plugin>>
	{
		static exec_on_plugin_enable instance;
		static bool invoke();

		constexpr void operator()() const noexcept { plugin_instance<Plugin>::instance().on_enable += invoke; }
	};
	template<sek::basic_static_string Func, sek::basic_static_string Plugin>
	struct exec_on_plugin_disable : static_exec<exec_on_plugin_disable<Func, Plugin>>
	{
		static exec_on_plugin_disable instance;
		static void invoke();

		constexpr void operator()() const noexcept { plugin_instance<Plugin>::instance().on_disable += invoke; }
	};
}	 // namespace impl

/** @brief Macro used to reference the internal unique type of a plugin.
 * @param id String id used to uniquely identify a plugin. */
#define SEK_PLUGIN(id) impl::plugin_instance<(id)>

/** @brief Macro used to define an instance of a plugin.
 * @param id String id used to uniquely identify a plugin.
 * @param ver Version of the plugin in the following format: `"<major>.<minor>.<patch>"`.
 *
 * @example
 * @code{.cpp}
 * SEK_PLUGIN_INSTANCE("my_plugin", "0.1.2")
 * {
 * 	printf("%s is initializing! version: %d.%d.%d\n",
 * 		   info.id.data(),
 * 		   info.engine_ver.major(),
 * 		   info.engine_ver.minor(),
 * 		   info.engine_ver.patch());
 *
 * 	on_enable += +[]()
 * 	{
 * 		printf("Enabling my_plugin\n");
 * 		return true;
 * 	};
 * 	on_disable += +[]() { printf("Disabling my_plugin\n"); };
 * }
 * @endcode */
#define SEK_PLUGIN_INSTANCE(id, ver)                                                                                   \
	static_assert(SEK_ARRAY_SIZE(id), "Plugin id must not be empty");                                                  \
                                                                                                                       \
	/* Definition of plugin instance constructor. */                                                                   \
	template<>                                                                                                         \
	SEK_PLUGIN(id)::plugin_instance() : plugin_base({sek::version{SEK_ENGINE_VERSION}, sek::version{ver}, (id)})       \
	{                                                                                                                  \
	}                                                                                                                  \
	/* Static instantiation & bootstrap implementation. 2-stage bootstrap is required in order                         \
	 * to allow for `SEK_PLUGIN(id)::instance()` to be called on static initialization. */                             \
	template<>                                                                                                         \
	impl::plugin_instance<(id)> &sek::engine::detail::plugin_base<impl::plugin_instance<(id)>>::instance()             \
	{                                                                                                                  \
		static impl::plugin_instance<(id)> value;                                                                      \
		return value;                                                                                                  \
	}                                                                                                                  \
	template<>                                                                                                         \
	impl::static_exec<void (*)()> impl::plugin_instance<(id)>::bootstrap = +[]() { instance(); };                      \
                                                                                                                       \
	/* User-facing initialization function. */                                                                         \
	template<>                                                                                                         \
	void impl::plugin_instance<(id)>::init()

/** @brief Macro used to bootstrap code execution when a plugin is enabled.
 * @param plugin Id of the target plugin.
 * @param func Unique name used to disambiguate functions executed during plugin enable process.
 *
 * @example
 * @code{.cpp}
 * SEK_ON_PLUGIN_ENABLE("test_plugin", "test_enable")
 * {
 * 	printf("test_plugin is enabled\n");
 * 	return true;
 * }
 * @endcode */
#define SEK_ON_PLUGIN_ENABLE(plugin, func)                                                                             \
	template<>                                                                                                         \
	bool impl::exec_on_plugin_enable<(plugin), (func)>::invoke();                                                      \
	template<>                                                                                                         \
	impl::exec_on_plugin_enable<(plugin), (func)> impl::exec_on_plugin_enable<(plugin), (func)>::instance = {};        \
                                                                                                                       \
	template<>                                                                                                         \
	bool impl::exec_on_plugin_enable<(plugin), (func)>::invoke()
/** @brief Macro used to bootstrap code execution when a plugin is disabled.
 * @param plugin Id of the target plugin.
 * @param func Unique name used to disambiguate functions executed during plugin disable process.
 *
 * @example
 * @code{.cpp}
 * SEK_ON_PLUGIN_DISABLE("test_plugin", "test_disable")
 * {
 * 	printf("test_plugin is disabled\n");
 * }
 * @endcode */
#define SEK_ON_PLUGIN_DISABLE(plugin, func)                                                                            \
	template<>                                                                                                         \
	void impl::exec_on_plugin_disable<(plugin), (func)>::invoke();                                                     \
	template<>                                                                                                         \
	impl::exec_on_plugin_disable<(plugin), (func)> impl::exec_on_plugin_disable<(plugin), (func)>::instance = {};      \
                                                                                                                       \
	template<>                                                                                                         \
	void impl::exec_on_plugin_disable<(plugin), (func)>::invoke()

#if defined(SEK_PLUGIN_NAME) && defined(SEK_PLUGIN_VERSION)
/** @brief Macro used to define a plugin with name & version specified by the current plugin project.
 * See `SEK_PLUGIN_INSTANCE` for details. */
#define SEK_PROJECT_PLUGIN_INSTANCE() SEK_PLUGIN_INSTANCE(SEK_PLUGIN_NAME, SEK_PLUGIN_VERSION)
/** @brief Macro used to reference the type of a plugin with name & version specified by the current plugin project.
 * See `SEK_PLUGIN` for details. */
#define SEK_PROJECT_PLUGIN SEK_PLUGIN(SEK_PLUGIN_NAME)
/** @brief Macro used to bootstrap code execution when the current project's plugin is enabled.
 * See `SEK_ON_PLUGIN_ENABLE` for details. */
#define SEK_ON_PROJECT_PLUGIN_ENABLE(func) SEK_ON_PLUGIN_ENABLE(SEK_PLUGIN_NAME, func)
/** @brief Macro used to bootstrap code execution when the current project's plugin is disabled.
 * See `SEK_ON_PLUGIN_DISABLE` for details. */
#define SEK_ON_PROJECT_PLUGIN_DISABLE(func) SEK_ON_PLUGIN_DISABLE(SEK_PLUGIN_NAME, func)
#endif
// clang-format on
