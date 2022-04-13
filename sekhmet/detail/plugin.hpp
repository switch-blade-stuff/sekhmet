//
// Created by switchblade on 2022-01-26.
//

#pragma once

#include <atomic>
#include <vector>

#include "meta_containers.hpp"
#include "static_string.hpp"
#include "type_id.hpp"

namespace sek
{
	namespace detail
	{
		struct plugin_db;

		template<basic_static_string Name>
		struct plugin_name_instance
		{
			constexpr static auto convert_name() noexcept
			{
				constexpr auto N = Name.size() + 1;
				basic_static_string<char, N> result;
				for (auto i = N; i-- > 0;) result[i] = static_cast<char>(Name[i]);
				return result;
			}

			constexpr static auto value = convert_name();
		};
	}	 // namespace detail

	class plugin
	{
		friend struct detail::plugin_db;

	public:
		enum class status_t : std::uint64_t
		{
			DISABLED,
			ENABLED,

			INITIAL = DISABLED,
		};

	protected:
		struct metadata_t
		{
			template<auto V, typename T = std::remove_const_t<decltype(V)>>
			constexpr static metadata_t get() noexcept
			{
				return {
					&sek::auto_constant<V>::value,
					sek::type_name<T>(),
				};
			}

			const void *data;
			meta_view<char> type_name;
		};

	private:
		struct plugin_db_entry
		{
			plugin_db_entry(status_t status, const plugin *p) noexcept : status(status), plugin_ptr(p) {}

			std::atomic<status_t> status = status_t::INITIAL;
			const plugin *plugin_ptr = nullptr;
		};

	public:
		/** Structure used to reference a plugin. */
		class handle
		{
			friend class plugin;

			constexpr explicit handle(plugin_db_entry *entry) noexcept : entry(entry) {}

		public:
			constexpr handle() = default;

			/** Checks if the handle is empty (does not reference a plugin). */
			[[nodiscard]] constexpr bool empty() const noexcept { return entry == nullptr; }
			/** Checks if the handle is not empty. */
			[[nodiscard]] constexpr operator bool() const noexcept { return !empty(); }

			/** Returns current status of the plugin. */
			[[nodiscard]] status_t status() const noexcept { return entry->status; }

			/** Returns name of the plugin. */
			[[nodiscard]] constexpr std::string_view name() const noexcept { return entry->plugin_ptr->name; }

			/** Returns pointer to the specific metadata of the plugin.
			 * @tparam T Type of the metadata.
			 * @return Pointer to the corresponding metadata or nullptr if such metadata was not found. */
			template<typename T>
			[[nodiscard]] constexpr const T *metadata() const noexcept
			{
				for (const auto &meta : entry->plugin_ptr->metadata_view)
					if (std::string_view{meta.type_name.begin(), meta.type_name.end()} == type_name<T>())
						return static_cast<const T *>(meta.data);
				return nullptr;
			}

		private:
			plugin_db_entry *entry = nullptr;
		};

		/** Returns vector of handles to all currently loaded plugins. */
		static SEK_API std::vector<handle> all();

		/** Returns handle for the corresponding plugin.
		 * @param name Name of the plugin to get handle for.
		 * @return Handle to the plugin entry. If an invalid plugin was specified, returns an empty handle. */
		static SEK_API handle get(std::string_view name);

		/** Enables the corresponding plugin.
		 * @param name Name of the plugin to enable.
		 * @return Handle to the plugin entry. If an invalid plugin was specified, returns an empty handle. */
		static SEK_API handle enable(std::string_view name);
		/** Enables the corresponding plugin.
		 * @param h Handle to the plugin. */
		static SEK_API void enable(handle h) noexcept;

		/** Disables the corresponding plugin.
		 * @param name Name of the plugin to enable.
		 * @return Handle to the plugin entry. If an invalid plugin was specified, returns an empty handle. */
		static SEK_API handle disable(std::string_view name);
		/** Disables the corresponding plugin.
		 * @param h Handle to the plugin. */
		static SEK_API void disable(handle h) noexcept;

	protected:
		constexpr static std::size_t queue_count = 2;

		struct exec_t
		{
			constexpr exec_t(const exec_t *next) noexcept : next(next) {}
			constexpr virtual ~exec_t() noexcept = default;

			constexpr virtual void operator()() const noexcept = 0;

			const exec_t *next;
		};

		constexpr explicit plugin(meta_view<metadata_t> metadata, std::string_view name) noexcept
			: metadata_view(metadata), name(name)
		{
		}

		static SEK_API void register_plugin(const plugin *p) noexcept;
		static SEK_API void drop_plugin(const plugin *p) noexcept;

		const exec_t *exec_queues[queue_count] = {nullptr};

	private:
		constexpr void invoke_queue(std::size_t queue_index) const noexcept
		{
			for (auto node = exec_queues[queue_index]; node; node = node->next) node->operator()();
		}
		void invoke_enable_queue() const noexcept { invoke_queue(0); }
		void invoke_disable_queue() const noexcept { invoke_queue(1); }

		meta_view<metadata_t> metadata_view = {};
		std::string_view name = {};
	};
}	 // namespace sek

namespace instantiation
{
	template<sek::basic_static_string Name>
	struct plugin_instance : sek::plugin
	{
		template<std::size_t Queue, sek::basic_static_string File, std::size_t Line>
		struct exec_node final : plugin::exec_t
		{
			static const exec_node node_instance;

			constexpr exec_node() noexcept : exec_t(std::exchange(instance.exec_queues[Queue], this)) {}
			constexpr ~exec_node() noexcept final = default;

			SEK_API_IMPORT void operator()() const noexcept final;
		};

		struct registrar_t
		{
			registrar_t() noexcept { register_plugin(&instance); }
			~registrar_t() noexcept { drop_plugin(&instance); }
		};

		template<auto Value>
		constexpr static metadata_t metadata = metadata_t::get<Value>();

		template<metadata_t... Args>
		constexpr static plugin_instance instantiate() noexcept
		{
			constexpr const auto &meta_array = sek::array_constant<metadata_t, Args...>::value;
			constexpr const auto &name = sek::detail::plugin_name_instance<Name>::value;

			return plugin_instance{{meta_array}, {name.begin(), name.end()}};
		}

		constinit static plugin_instance instance;
		static const registrar_t registrar;

		using plugin::plugin;
	};
}	 // namespace instantiation

/** Declares a new plugin with the specified name and metadata.
 * Metadata (if any) should be specified via `metadata<Value>` template.
 *
 * @example
 * ```cpp
 * SEK_DECLARE_PLUGIN(
 * 						"my_plugin",
 * 						metadata<my_type>,
 * 						metadata<my_other_type>
 * 					 )
 * ``` */
#define SEK_DECLARE_PLUGIN(name, ...)                                                                                   \
	template<>                                                                                                          \
	constinit instantiation::plugin_instance<name> instantiation::plugin_instance<name>::instance = \
		plugin_instance<name>::instantiate<(__VA_ARGS__)>();                                                            \
	template<>                                                                                                          \
	const typename instantiation::plugin_instance<name>::registrar_t                                          \
		instantiation::plugin_instance<name>::registrar = {};

/** Executes the following code when the corresponding execution queue is invoked. */
#define SEK_ON_PLUGIN_QUEUE(name, queue)                                                                                   \
	namespace instantiation                                                                                      \
	{                                                                                                                      \
		template<>                                                                                                         \
		template<>                                                                                                         \
		const typename plugin_instance<name>::exec_node<queue, __FILE__, __LINE__>                                         \
			plugin_instance<name>::exec_node<queue, __FILE__, __LINE__>::node_instance = {};                               \
	}                                                                                                                      \
	template<>                                                                                                             \
	template<>                                                                                                             \
	SEK_API_EXPORT void instantiation::plugin_instance<name>::exec_node<queue, __FILE__, __LINE__>::operator()() \
		const noexcept

/** Executes the following code when a plugin is enabled. */
#define SEK_ON_PLUGIN_ENABLE(name) SEK_ON_PLUGIN_QUEUE(name, 0)

/** Executes the following code when a plugin is disabled. */
#define SEK_ON_PLUGIN_DISABLE(name) SEK_ON_PLUGIN_QUEUE(name, 1)
