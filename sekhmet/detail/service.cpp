/*
 * Created by switchblade on 23/09/22
 */

#include "../service.hpp"

#include <mutex>

#include "../assert.hpp"
#include "../sparse_map.hpp"

namespace sek
{
	namespace detail
	{
		struct service_db
		{
			[[nodiscard]] static service_db &instance()
			{
				static service_db instance;
				return instance;
			}

			sparse_map<std::size_t, std::atomic<void *>> entries;
			std::mutex mtx;
		};
	}	 // namespace detail

	std::size_t service<void>::id::generate() noexcept
	{
		static std::atomic<std::size_t> id_counter = 0;

		const auto result = ++id_counter;
		SEK_ASSERT(result != 0, "Service ID overflow detected");
		return result;
	}

	std::atomic<void *> &service<void>::generate_ptr(const id *id) noexcept
	{
		auto &db = detail::service_db::instance();
		std::lock_guard<std::mutex> l(db.mtx);

		auto iter = db.entries.find(id->m_id);
		if (iter == db.entries.end()) [[unlikely]]
			iter = db.entries.emplace(std::piecewise_construct, std::forward_as_tuple(id->m_id), std::forward_as_tuple()).first;
		return iter->second;
	}
}	 // namespace sek
