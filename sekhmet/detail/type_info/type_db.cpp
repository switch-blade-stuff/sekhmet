/*
 * Created by switchblade on 05/10/22
 */

#include "type_db.hpp"

#include "type_info.hpp"

SEK_EXPORT_TYPE_INFO(sek::type_database)

namespace sek
{
	type_info type_info::get(std::string_view name)
	{
		auto db = type_database::instance().access_shared();
		auto &types = db->types();

		const auto iter = types.find(name);
		if (iter != types.end()) [[likely]]
			return *iter;
		return {};
	}

	namespace detail
	{
		/* Type database should be registered on static initialization to allow it to be initialized before every other service. */
		struct type_db_registrar
		{
			type_db_registrar() { service_locator::instance()->load<type_database>(std::in_place_type<type_database>); }
			~type_db_registrar() { service_locator::instance()->reset<type_database>(); }
		};
		static const auto registrar = type_db_registrar{};
	}	 // namespace detail

	void type_database::reset(std::string_view type)
	{
		const auto iter = m_type_table.find(type);
		if (iter != m_type_table.end()) [[likely]]
		{
			/* Remove the type from the attribute map. */
			for (auto &attr : iter->m_data->attributes)
			{
				const auto attr_iter = m_attr_table.find(attr.type->name);
				if (attr_iter != m_attr_table.end()) [[likely]]
					attr_iter->second.erase(type);
			}

			/* Reset the type to its original "unreflected" state and remove from the set. */
			iter->m_data->reset(iter->m_data);
			m_type_table.erase(iter);
		}
	}
	detail::type_data *type_database::reflect(detail::type_handle handle)
	{
		auto *data = handle.get();
		auto iter = m_type_table.find(data->name);
		if (iter == m_type_table.end()) [[likely]]
		{
			const auto type = type_info{data};
			iter = m_type_table.insert(type).first;

			/* Add the type to the attribute map. */
			for (auto &attr : iter->m_data->attributes)
			{
				const auto attr_name = attr.type->name;
				auto attr_iter = m_attr_table.find(attr_name);

				// clang-format off
				if (attr_iter == m_attr_table.end()) [[likely]]
				{
					attr_iter = m_attr_table.emplace(std::piecewise_construct,
									std::forward_as_tuple(attr_name),
									std::forward_as_tuple())
								.first;
				}
				// clang-format on
				attr_iter->second.try_insert(type);
			}
		}
		return iter->m_data;
	}

	type_query &type_query::with_parent(type_info type)
	{
		if (!m_started) [[unlikely]]
		{
			/* If the query does not have a set yet, go through each reflected type & check it. */
			for (auto &candidate : m_db.m_type_table)
				if (candidate.has_parent(type)) m_types.insert(type);
			m_started = true;
		}
		else
		{
			/* Otherwise, remove all types that are not part of the attribute's set. */
			for (auto pos = m_types.end(), end = m_types.begin(); pos-- != end;)
				if (!pos->has_parent(type)) m_types.erase(pos);
		}
	}
	type_query &type_query::with_attribute(type_info type)
	{
		if (const auto iter = m_db.m_attr_table.find(type.name()); iter != m_db.m_attr_table.end()) [[likely]]
		{
			if (!m_started) [[unlikely]]
			{
				/* If the query does not have a set yet, copy the attribute's set. */
				m_types = iter->second;
				m_started = true;
			}
			else
			{
				/* Otherwise, remove all types that are not part of the attribute's set. */
				for (auto pos = m_types.end(), end = m_types.begin(); pos-- != end;)
					if (!iter->second.contains(*pos)) m_types.erase(pos);
			}
		}
		return *this;
	}
}	 // namespace sek