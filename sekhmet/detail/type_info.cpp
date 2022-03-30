//
// Created by switchblade on 2022-02-04.
//

#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "type_info.hpp"

#include "hmap.hpp"
#include <shared_mutex>

namespace sek
{
	bad_type_exception::bad_type_exception(const char *src) noexcept : bad_type_exception()
	{
		auto tmp_msg = static_cast<char *>(malloc(sizeof(char) * (strlen(src) + 1)));
		if (tmp_msg) [[likely]]
			msg = strcpy(tmp_msg, src);
	}
	bad_type_exception::bad_type_exception(type_id type) noexcept : bad_type_exception()
	{
		auto name = type.name();
		auto tmp_msg = static_cast<char *>(malloc(sizeof(char) * static_cast<unsigned long>(39 + name.size())));
		if (!tmp_msg) [[unlikely]]
			return;

		strcpy(tmp_msg, "Bad or unexpected type \"");
		strncpy(tmp_msg, reinterpret_cast<const char *>(name.data()), static_cast<std::size_t>(name.size()));
		strcpy(tmp_msg, "\"");

		msg = tmp_msg;
	}
	bad_type_exception::~bad_type_exception()
	{
		if (msg) free((void *) msg);
	}

	struct type_db
	{
		static type_db &get() noexcept
		{
			static type_db instance = {};
			return instance;
		}

		std::shared_mutex type_mtx;
		hmap<type_id, detail::type_data::handle> type_table;
	};

	bool type_info::register_type(type_info type)
	{
		if (type.empty()) [[unlikely]]
			return false;

		auto &db = type_db::get();
		std::lock_guard<std::shared_mutex> l(db.type_mtx);

		return db.type_table.try_emplace(type.tid(), type.data).second;
	}
	bool type_info::deregister_type(type_info type)
	{
		if (type.empty()) [[unlikely]]
			return false;

		auto &db = type_db::get();
		std::lock_guard<std::shared_mutex> l(db.type_mtx);

		return db.type_table.erase(type.tid());
	}

	type_info type_info::get(type_id tid) noexcept
	{
		auto &db = type_db::get();
		db.type_mtx.lock_shared();

		type_info result = {};
		if (auto data_iterator = db.type_table.find(tid); data_iterator != db.type_table.end()) [[likely]]
			result.data = data_iterator->second;

		db.type_mtx.unlock_shared();
		return result;
	}
	std::vector<type_info> type_info::all()
	{
		auto &db = type_db::get();
		db.type_mtx.lock_shared();

		std::vector<type_info> result;
		result.reserve(db.type_table.size());
		for (auto &entry : db.type_table) result.push_back(type_info{entry.second});

		db.type_mtx.unlock_shared();
		return result;
	}
}	 // namespace sek