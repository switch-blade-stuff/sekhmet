//
// Created by switch_blade on 2022-10-05.
//

#include "../service.hpp"

namespace sek
{
	typename service_locator::handle_t service_locator::instance() noexcept
	{
		static guard_t guard;
		return guard.access();
	}

	struct service_locator::service_entry
	{
		~service_entry()
		{
			if (deleter != nullptr) [[unlikely]]
				deleter(instance.load());
		}
		void reset()
		{
			reset_event();
			if (deleter != nullptr) [[likely]]
				deleter(instance.load());
			instance = nullptr;
			deleter = nullptr;
		}

		void (*deleter)(void *) = nullptr;
		std::atomic<void *> instance;

		event<void()> load_event;
		event<void()> reset_event;
	};

	service_locator::service_entry &service_locator::get_entry(type_info type)
	{
		auto iter = m_entries.find(type.name());
		if (iter == m_entries.end()) [[unlikely]]
			iter = m_entries.emplace(type.name(), new service_entry{});
		return **iter;
	}

	void service_locator::reset_impl(type_info type)
	{
		const auto iter = m_entries.find(type.name());
		if (iter != m_entries.end()) [[likely]]
			(*iter)->reset();
	}
	std::atomic<void *> &service_locator::get_impl(type_info type) { return get_entry(type).instance; }
	event<void()> &service_locator::on_load_impl(type_info type) { return get_entry(type).load_event; }
	event<void()> &service_locator::on_reset_impl(type_info type) { return get_entry(type).reset_event; }

}	 // namespace sek