/*
 * Created by switchblade on 18/08/22
 */

#include "../message.hpp"

namespace sek::engine::detail
{
	template class SEK_API_EXPORT message_table<message_scope::THREAD>;
	template class SEK_API_EXPORT message_table<message_scope::GLOBAL>;

	message_table<message_scope::THREAD> &message_table<message_scope::THREAD>::instance()
	{
		thread_local message_table table;
		return table;
	}
	message_table<message_scope::GLOBAL> &message_table<message_scope::GLOBAL>::instance()
	{
		static message_table table;
		return table;
	}

	// clang-format off
	typename message_table<message_scope::THREAD>::data_type *message_table<message_scope::THREAD>::find(std::string_view t) const noexcept
	{
		return base_t::find(t);
	}
	typename message_table<message_scope::GLOBAL>::data_type *message_table<message_scope::GLOBAL>::find(std::string_view t) const
	{
		std::shared_lock<std::shared_mutex> l(m_mtx);
		return base_t::find(t);
	}

	typename message_table<message_scope::THREAD>::data_type &message_table<message_scope::THREAD>::try_insert(std::string_view t, data_type *(*f)())
	{
		return base_t::try_insert(t, f);
	}
	typename message_table<message_scope::GLOBAL>::data_type &message_table<message_scope::GLOBAL>::try_insert(std::string_view t, data_type *(*f)())
	{
		std::unique_lock<std::shared_mutex> l(m_mtx);
		return base_t::try_insert(t, f);
	}
	// clang-format on

	void message_table<message_scope::THREAD>::erase(std::string_view type, data_type *(*factory)())
	{
		return base_t::erase(type, factory);
	}
	void message_table<message_scope::GLOBAL>::erase(std::string_view type, data_type *(*factory)())
	{
		std::unique_lock<std::shared_mutex> l(m_mtx);
		return base_t::erase(type, factory);
	}

	void message_table<message_scope::THREAD>::dispatch_all() const
	{
		for (auto &e : m_table) e.second->dispatch();
	}
	void message_table<message_scope::GLOBAL>::dispatch_all() const
	{
		std::unique_lock<std::shared_mutex> l(m_mtx);
		for (auto &e : m_table) e.second->dispatch();
	}
}	 // namespace sek::engine::detail