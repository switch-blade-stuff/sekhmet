//
// Created by switch_blade on 2022-10-03.
//

#pragma once

#include "../../dense_map.hpp"
#include "../../dense_set.hpp"
#include "../../service.hpp"
#include "type_info.hpp"

namespace sek
{
	template<>
	struct service_traits<type_database>
	{
		typedef type_database type;
		typedef std::shared_mutex mutex_type;
	};

	/** @brief Global synchronized service used to store reflection type database. */
	class SEK_API type_database : public service<type_database>
	{
		template<service_type>
		friend class service;
		friend class type_query;

		struct type_cmp
		{
			typedef std::true_type is_transparent;

			template<typename T>
			constexpr bool operator()(const T &name, const type_info &type) const noexcept
				requires std::is_convertible_v<T, std::string_view>
			{
				return type.valid() && type.name() == static_cast<std::string_view>(name);
			}
			template<typename T>
			constexpr bool operator()(const type_info &type, const T &name) const noexcept
				requires std::is_convertible_v<T, std::string_view>
			{
				return type.valid() && type.name() == static_cast<std::string_view>(name);
			}
			constexpr bool operator()(const type_info &a, const type_info &b) const noexcept { return a == b; }
		};
		struct type_hash
		{
			typedef std::true_type is_transparent;

			template<typename T>
			constexpr hash_t operator()(const T &name) const noexcept
				requires std::is_convertible_v<T, std::string_view>
			{
				const auto sv = static_cast<std::string_view>(name);
				return fnv1a(sv.data(), sv.size());
			}
			constexpr hash_t operator()(const type_info &type) const noexcept { return hash(type); }
		};

		using type_table_t = dense_set<type_info, type_hash, type_cmp>;
		using attr_table_t = dense_map<std::string_view, type_table_t>;

		type_database() = default;

	public:
		/** Adds the type to the internal database & returns a type factory for it. */
		template<typename T>
		[[nodiscard]] type_factory<T> reflect()
		{
			static_assert(is_type_info_exported_v<T>, "Reflected type must be exported via `SEK_EXTERN_TYPE_INFO`");
			return type_factory<T>{reflect(type_info::handle<T>())};
		}
		/** Removes a previously reflected type from the internal database. */
		void reset(std::string_view type);
		/** @copydoc reset */
		template<typename T>
		void reset()
		{
			reset(type_info::get<T>());
		}

		/** Creates a type query used to filter reflected types. */
		[[nodiscard]] constexpr type_query query() const noexcept;
		/** Returns reference to the internal set of types. */
		[[nodiscard]] constexpr const type_table_t &types() const noexcept { return m_type_table; }

	private:
		[[nodiscard]] detail::type_data *reflect(detail::type_handle);

		type_table_t m_type_table;
		attr_table_t m_attr_table;
	};

	/** @brief Structure used to obtain a filtered subset of types from the type database. */
	class SEK_API type_query
	{
		using type_set_t = typename type_database::type_table_t;

	public:
		type_query() = delete;

		/** Creates a type query for the specified type database. */
		constexpr explicit type_query(const type_database &db) : m_db(db) {}

		/** Excludes all types that do not have the specified parent. */
		[[nodiscard]] type_query &with_parent(type_info type);
		/** @copydoc with_parent */
		template<typename T>
		[[nodiscard]] type_query &with_parent()
		{
			return with_parent(type_info::get<T>());
		}

		/** Excludes all types that do not have the specified attribute. */
		[[nodiscard]] type_query &with_attribute(type_info type);
		/** @copydoc with_attribute */
		template<typename T>
		[[nodiscard]] type_query &with_attribute()
		{
			return with_attributes(type_info::get<T>());
		}

		/** Returns reference to the set of types that was matched by the query. */
		[[nodiscard]] constexpr const type_set_t &types() const noexcept { return m_types; }

	private:
		const type_database &m_db;
		type_set_t m_types;
		bool m_started = false; /* If set to `false`, the type set is safe to overwrite. */
	};

	constexpr type_query type_database::query() const noexcept { return type_query{*this}; }

	template<typename T>
	type_factory<T> type_info::reflect()
	{
		return type_database::instance()->template reflect<T>();
	}
	void type_info::reset(std::string_view name) { type_database::instance()->reset(name); }
}	 // namespace sek

SEK_EXTERN_TYPE_INFO(sek::type_database)