//
// Created by switchblade on 2022-04-24.
//

#pragma once

#include <atomic>
#include <vector>

#include "define.h"
#include "static_string.hpp"
#include <string_view>

namespace sek
{
	namespace detail
	{
		struct plugin_data
		{
			template<basic_static_string>
			struct instance
			{
				static plugin_data value;
			};

			enum status_t : int
			{
				DISABLED,
				ENABLED,
			};

			struct event_queue
			{
				template<sek::basic_static_string, std::size_t>
				struct instance
				{
					static event_queue value;
				};

				struct node_t
				{
					constexpr explicit node_t(void (*invoke)()) noexcept : invoke(invoke) {}

					const node_t *next = nullptr;
					void (*invoke)();
				};

				constexpr explicit event_queue(std::size_t id) noexcept : id(id) {}

				constexpr void invoke() const
				{
					// clang-format off
					for (auto node = nodes; node != nullptr; node = node->next)
						node->invoke();
					// clang-format on
				}

				event_queue *next;
				std::size_t id;

				const node_t *nodes = nullptr;
			};

			template<sek::basic_static_string P, std::size_t I>
			struct event_registrar : event_queue::node_t
			{
				explicit event_registrar(void (*f)()) noexcept : node_t(f)
				{
					auto &dest = instance<P>::value.find_queue(I);
					if (dest == nullptr) [[unlikely]]
					{
						auto &queue = event_queue::instance<P, I>::value;
						queue.next = std::exchange(dest, &queue);
					}
					next = std::exchange(dest->nodes, this);
				}
			};

			SEK_API static void load(plugin_data *) noexcept;
			SEK_API static void unload(plugin_data *) noexcept;

			explicit plugin_data(std::string_view name) noexcept : name(name) { load(this); }
			~plugin_data() { unload(this); }

			constexpr event_queue *&find_queue(std::size_t id) noexcept
			{
				for (auto *&next_ptr = queues;; next_ptr = next_ptr->next)
				{
					if (next_ptr == nullptr || next_ptr->id > id) [[likely]]
						return next_ptr;
				}
			}

			std::atomic<status_t> status = DISABLED;

			std::string_view name;
			event_queue *queues = nullptr;
		};

		template<sek::basic_static_string Plugin, std::size_t Id>
		plugin_data::event_queue plugin_data::event_queue::instance<Plugin, Id>::value = event_queue{Id};
	}	 // namespace detail

	/** @brief Structure used to represent an engine plugin. */
	class plugin
	{
	public:
		typedef std::size_t event_id;

		/** Id of the enable event queue. */
		constexpr static event_id enable_event = 0;
		/** Id of the disable event queue. */
		constexpr static event_id disable_event = 1;

		/** Returns a vector containing all loaded plugins. */
		SEK_API static std::vector<plugin> get_loaded();
		/** Returns a vector containing all enabled plugins. */
		SEK_API static std::vector<plugin> get_enabled();

	private:
		constexpr explicit plugin(detail::plugin_data *data) noexcept : data(data) {}

	public:
		plugin() = delete;

		plugin(const plugin &) noexcept = default;
		plugin &operator=(const plugin &) noexcept = default;
		plugin(plugin &&) noexcept = default;
		plugin &operator=(plugin &&) noexcept = default;

		/** Returns display name of the plugin. */
		[[nodiscard]] constexpr std::string_view name() const noexcept { return data->name; }
		/** Checks if the plugin is enabled. */
		[[nodiscard]] bool enabled() const noexcept { return data->status == detail::plugin_data::ENABLED; }

		/** Invokes the corresponding plugin event queue.
		 * @param event Id of the event queue to invoke.
		 * @note Manually invoking enable or disable queues (id 0 and 1) is not recommended. */
		constexpr void invoke_event(event_id event) const
		{
			for (auto queue = data->queues; queue != nullptr; queue = queue->next)
				if (queue->id == event)
				{
					queue->invoke();
					break;
				}
		}

		/** Enables the plugin and invokes it's enable event queue. */
		SEK_API void enable();
		/** Disables the plugin and invokes it's disable event queue. */
		SEK_API void disable();

	private:
		detail::plugin_data *data;
	};
}	 // namespace sek

namespace registration
{
	template<sek::basic_static_string, std::size_t, sek::basic_static_string, std::size_t>
	struct sek_plugin_event;
}

#define SEK_PLUGIN(name)                                                                                               \
	template<>                                                                                                         \
	sek::detail::plugin_data sek::detail::plugin_data::instance<name>::value = plugin_data{name};

#define SEK_PLUGIN_EVENT_TYPE(name, id) registration::sek_plugin_event<name, id, (SEK_FILE), (SEK_LINE)>

#define SEK_ON_PLUGIN_EVENT(plugin_name, event)                                                                          \
	template<>                                                                                                           \
	struct SEK_PLUGIN_EVENT_TYPE(plugin_name, event)                                                                     \
	{                                                                                                                    \
		static void invoke();                                                                                            \
                                                                                                                         \
		static sek::detail::plugin_data::event_registrar<plugin_name, event> registrar;                                  \
	};                                                                                                                   \
	sek::detail::plugin_data::event_registrar<plugin_name, event> SEK_PLUGIN_EVENT_TYPE(plugin_name, event)::registrar = \
		sek::detail::plugin_data::event_registrar<plugin_name, event>{invoke};                                           \
                                                                                                                         \
	void SEK_PLUGIN_EVENT_TYPE(plugin_name, event)::invoke()

#define SEK_ON_PLUGIN_ENABLE(plugin_name) SEK_ON_PLUGIN_EVENT(plugin_name, sek::plugin::enable_event)
#define SEK_ON_PLUGIN_DISABLE(plugin_name) SEK_ON_PLUGIN_EVENT(plugin_name, sek::plugin::disable_event)
