//
// Created by switchblade on 16/05/22.
//

#include "dense_map.hpp"
#include "logger.hpp"
#include "reflection.hpp"
#include <shared_mutex>

namespace sek
{
	struct type_db
	{
		static type_db &instance()
		{
			static type_db value;
			return value;
		}

		mutable std::shared_mutex mtx;
		dense_map<std::string_view, detail::type_handle> types;
	};

	type_info::data_t &type_info::register_type(handle_t handle) noexcept
	{
		auto &db = type_db::instance();
		std::lock_guard<std::shared_mutex> l(db.mtx);
		return *db.types.try_emplace(handle->name, handle).first->second;
	}
	type_info type_info::get(std::string_view name) noexcept
	{
		auto &db = type_db::instance();
		std::shared_lock<std::shared_mutex> l(db.mtx);

		if (auto handle_iter = db.types.find(name); handle_iter != db.types.end()) [[likely]]
			return type_info{handle_iter->second};
		else
			return type_info{};
	}
	void type_info::reset(std::string_view name) noexcept
	{
		auto &db = type_db::instance();
		std::lock_guard<std::shared_mutex> l(db.mtx);
		db.types.erase(name);
	}
}	 // namespace sek
