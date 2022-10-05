//
// Created by switch_blade on 2022-10-05.
//

#include "../service.hpp"

#include "type_info/type_db.hpp"

namespace sek
{
	detail::service_storage<void>::~service_storage() = default;

	typename service_locator::guard_t service_locator::instance() noexcept
	{
		static std::recursive_mutex mtx;
		static service_locator locator;
		return guard_t{locator, mtx};
	}

	struct service_locator::service_entry
	{
		~service_entry() { delete instance.exchange(nullptr); }

		void reset()
		{
			reset_event();
			delete instance.exchange(nullptr);
			instance_type = {};
		}
		detail::service_storage<void> *load(detail::service_storage<void> *(*factory)(), type_info type, bool replace)
		{
			/* Reads are synchronized by locator's mutex. */
			const auto old_ptr = instance.load(std::memory_order_relaxed);

			/* Reset the old instance if needed. */
			if (old_ptr != nullptr)
			{
				if (replace)
				{
					reset_event();
					delete old_ptr;
				}
				else
					return old_ptr;
			}

			/* Load the new instance. */
			const auto new_ptr = factory();
			instance.store(new_ptr);
			instance_type = type;
			load_event();

			return new_ptr;
		}

		/* `instance` is atomic, to allow direct access without locking the locator mutex. */
		std::atomic<detail::service_storage<void> *> instance = nullptr;
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

	std::atomic<detail::service_storage<void> *> &service_locator::get_impl(type_info type)
	{
		return get_entry(type).instance;
	}
	type_info service_locator::instance_type_impl(type_info type) { return get_entry(type).instance_type; }

	event<void()> &service_locator::on_load_impl(type_info type) { return get_entry(type).load_event; }
	event<void()> &service_locator::on_reset_impl(type_info type) { return get_entry(type).reset_event; }

	void service_locator::reset_impl(type_info type)
	{
		const auto iter = m_entries.find(type.name());
		if (iter != m_entries.end()) [[likely]]
			iter->second->reset();
	}

	detail::service_storage<void> *service_locator::load_impl(type_info service, type_info impl_type, factory_t factory, bool r)
	{
		return get_entry(service).load(factory, impl_type, r);
	}
	detail::service_storage<void> *service_locator::load_impl(type_info service, type_info attr_type, type_info impl_type, bool r)
	{
		if (!impl_type.has_attribute(attr_type)) [[unlikely]]
			return nullptr;

		/* Can safely cast to the generic data. */
		const auto attr_any = impl_type.attribute(attr_type);
		auto *attr = static_cast<const attr_data_t *>(attr_any.data());

		/* Load using the attribute's factory. */
		return get_entry(service).load(attr->m_factory, attr->m_instance_type, r);
	}
	detail::service_storage<void> *service_locator::load_impl(type_info service, type_info attr_type, std::string_view id, bool r)
	{
		/* `detail::service_impl_tag` is used to query all attribute types. */
		auto type_db = type_database::instance().access_shared();
		auto query = type_db->query().template with_attributes<detail::service_impl_tag>();

		for (auto &type : query)
			if (type.has_attribute(attr_type)) /* Check for the actual service attribute. */
			{
				/* Can safely cast to the generic data. */
				const auto attr_any = type.attribute(attr_type);
				auto *attr = static_cast<const attr_data_t *>(attr_any.data());

				/* Only select if the ids match. */
				if (attr->m_id != id) continue;
				/* Load using the attribute's factory. */
				return get_entry(service).load(attr->m_factory, attr->m_instance_type, r);
			}
		return nullptr;
	}
}	 // namespace sek