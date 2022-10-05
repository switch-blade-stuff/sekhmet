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
				deleter(instance.exchange(nullptr));
		}

		void reset()
		{
			reset_event();
			const auto old_ptr = instance.exchange(nullptr);
			if (deleter != nullptr) [[likely]]
				deleter(old_ptr);
			instance_type = {};
			deleter = nullptr;
		}
		template<typename F>
		service<void> *load(F &&factory, void (*del)(service<void> *), type_info type, bool replace)
		{
			/* Reads are synchronized by locator's mutex. */
			const auto old_ptr = instance.load(std::memory_order_relaxed);
			const auto old_del = std::exchange(deleter, del);

			/* Reset the old instance if needed. */
			if (old_ptr != nullptr)
			{
				if (replace)
				{
					reset_event();
					if (old_del != nullptr) [[likely]]
						old_del(old_ptr);
				}
				else
					return old_ptr;
			}

			/* Load the new instance. */
			const auto new_ptr = factory();
			instance.store(new_ptr);
			deleter = del;
			instance_type = type;
			load_event();

			return new_ptr;
		}

		/* `instance` is atomic, to allow direct access without locking the locator mutex. */
		std::atomic<service<void> *> instance = nullptr;
		void (*deleter)(service<void> *) = nullptr;
		type_info instance_type;

		event<void()> load_event;
		event<void()> reset_event;
	};

	service_locator::service_entry &service_locator::get_entry(type_info type)
	{
		auto iter = m_entries.find(type.name());
		if (iter == m_entries.end()) [[unlikely]]
			iter = m_entries.emplace(type.name(), new service_entry{}).first;
		return *iter->second;
	}

	std::atomic<service<void> *> &service_locator::get_impl(type_info type) { return get_entry(type).instance; }
	type_info service_locator::instance_type_impl(type_info type) { return get_entry(type).instance_type; }

	event<void()> &service_locator::on_load_impl(type_info type) { return get_entry(type).load_event; }
	event<void()> &service_locator::on_reset_impl(type_info type) { return get_entry(type).reset_event; }

	void service_locator::reset_impl(type_info type)
	{
		const auto iter = m_entries.find(type.name());
		if (iter != m_entries.end()) [[likely]]
			iter->second->reset();
	}

	service<void> *service_locator::load_impl(type_info service_type, type_info impl_type, service<void> *impl, bool replace)
	{
		return get_entry(service_type).load([&]() { return impl; }, nullptr, impl_type, replace);
	}
	service<void> *service_locator::load_impl(type_info service_type, type_info attr_type, type_info impl_type, bool replace)
	{
		if (!impl_type.has_attribute(attr_type)) [[unlikely]]
			return nullptr;

		/* Can safely cast to the generic data. */
		const auto attr_any = impl_type.attribute(attr_type);
		auto *attr = static_cast<const attr_data_t *>(attr_any.data());

		/* Load using the attribute's factory. */
		return get_entry(service_type).load(attr->m_factory, attr->m_deleter, attr->m_instance_type, replace);
	}
	service<void> *service_locator::load_impl(type_info service_type, type_info attr_type, std::string_view id, bool replace)
	{
		/* `detail::service_impl_tag` is used to query all attribute types. */
		auto type_db = type_database::instance()->acquire_shared();
		auto query = type_db->query().with_attributes<detail::service_impl_tag>();

		for (auto &type : query)
			if (type.has_attribute(attr_type)) /* Check for the actual service attribute. */
			{
				/* Can safely cast to the generic data. */
				const auto attr_any = impl_type.attribute(attr_type);
				auto *attr = static_cast<const attr_data_t *>(attr_any.data());

				/* Only select if the ids match. */
				if (attr->m_id != id) continue;
				/* Load using the attribute's factory. */
				return get_entry(service_type).load(attr->m_factory, attr->m_deleter, attr->m_instance_type, replace);
			}
		return nullptr;
	}
}	 // namespace sek