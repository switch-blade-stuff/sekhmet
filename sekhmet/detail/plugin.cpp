//
// Created by switchblade on 28/04/22.
//

#include "plugin.hpp"

#include "hset.hpp"
#include <shared_mutex>

namespace sek
{
	namespace detail
	{
		struct plugin_db
		{
			struct ptr_hash
			{
				constexpr hash_t operator()(plugin_data *value) const noexcept
				{
#if UINTPTR_MAX > UINT32_MAX
					return int64_hash(std::bit_cast<std::uint64_t>(value));
#else
					return int32_hash(std::bit_cast<std::uint32_t>(value));
#endif
				}
			};

			static plugin_db instance;

			std::shared_mutex mtx;
			hset<plugin_data *, ptr_hash> data;
		};

		plugin_db plugin_db::instance = {};

		void plugin_data::load(plugin_data *data) noexcept
		{
			std::lock_guard l(plugin_db::instance.mtx);
			plugin_db::instance.data.insert(data);
		}
		void plugin_data::unload(plugin_data *data) noexcept
		{
			auto &db = plugin_db::instance;

			std::lock_guard l(db.mtx);
			if (auto pos = db.data.find(data); pos != db.data.end()) [[likely]]
				db.data.erase(pos);
		}
	}	 // namespace detail

	std::vector<plugin> plugin::get_loaded()
	{
		auto &db = detail::plugin_db::instance;

		std::shared_lock l(db.mtx);
		std::vector<plugin> result;

		// clang-format off
		for (auto &ptr : db.data)
			result.push_back(plugin{ptr});
		// clang-format on

		return result;
	}
	std::vector<plugin> plugin::get_enabled()
	{
		auto &db = detail::plugin_db::instance;

		std::shared_lock l(db.mtx);
		std::vector<plugin> result;

		// clang-format off
		for (auto &ptr : db.data)
		{
			if (ptr->status == detail::plugin_data::ENABLED)
				result.push_back(plugin{ptr});
		}
		// clang-format on

		return result;
	}

	void plugin::enable()
	{
		auto expected = detail::plugin_data::DISABLED;
		if (data->status.compare_exchange_strong(expected, detail::plugin_data::ENABLED)) [[likely]]
			invoke_event(enable_event);
	}
	void plugin::disable()
	{
		auto expected = detail::plugin_data::ENABLED;
		if (data->status.compare_exchange_strong(expected, detail::plugin_data::DISABLED)) [[likely]]
			invoke_event(disable_event);
	}
}	 // namespace sek
