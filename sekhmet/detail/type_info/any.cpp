//
// Created by switch_blade on 2022-10-03.
//

#include "any.hpp"

#include "any_range.hpp"
#include "any_table.hpp"
#include "any_tuple.hpp"

namespace sek
{
	expected<any_range, std::error_code> any::range(std::nothrow_t)
	{
		if (empty()) [[unlikely]]
			return unexpected{make_error_code(type_errc::UNEXPECTED_EMPTY_ANY)};
		else if (m_type->range_data == nullptr) [[unlikely]]
			return unexpected{make_error_code(type_errc::INVALID_TYPE)};
		return any_range{std::in_place, any_ref{*this}};
	}
	expected<any_range, std::error_code> any::range(std::nothrow_t) const
	{
		if (empty()) [[unlikely]]
			return unexpected{make_error_code(type_errc::UNEXPECTED_EMPTY_ANY)};
		else if (m_type->range_data == nullptr) [[unlikely]]
			return unexpected{make_error_code(type_errc::INVALID_TYPE)};
		return any_range{std::in_place, any_ref{*this}};
	}
	expected<any_table, std::error_code> any::table(std::nothrow_t)
	{
		if (empty()) [[unlikely]]
			return unexpected{make_error_code(type_errc::UNEXPECTED_EMPTY_ANY)};
		else if (m_type->table_data == nullptr) [[unlikely]]
			return unexpected{make_error_code(type_errc::INVALID_TYPE)};
		return any_table{std::in_place, any_ref{*this}};
	}
	expected<any_table, std::error_code> any::table(std::nothrow_t) const
	{
		if (empty()) [[unlikely]]
			return unexpected{make_error_code(type_errc::UNEXPECTED_EMPTY_ANY)};
		else if (m_type->table_data == nullptr) [[unlikely]]
			return unexpected{make_error_code(type_errc::INVALID_TYPE)};
		return any_table{std::in_place, any_ref{*this}};
	}
	expected<any_tuple, std::error_code> any::tuple(std::nothrow_t)
	{
		if (empty()) [[unlikely]]
			return unexpected{make_error_code(type_errc::UNEXPECTED_EMPTY_ANY)};
		else if (m_type->tuple_data == nullptr) [[unlikely]]
			return unexpected{make_error_code(type_errc::INVALID_TYPE)};
		return any_tuple{std::in_place, any_ref{*this}};
	}
	expected<any_tuple, std::error_code> any::tuple(std::nothrow_t) const
	{
		if (empty()) [[unlikely]]
			return unexpected{make_error_code(type_errc::UNEXPECTED_EMPTY_ANY)};
		else if (m_type->tuple_data == nullptr) [[unlikely]]
			return unexpected{make_error_code(type_errc::INVALID_TYPE)};
		return any_tuple{std::in_place, any_ref{*this}};
	}

	any_range any::range() { return any_range{any_ref{*this}}; }
	any_range any::range() const { return any_range{any_ref{*this}}; }
	any_table any::table() { return any_table{any_ref{*this}}; }
	any_table any::table() const { return any_table{any_ref{*this}}; }
	any_tuple any::tuple() { return any_tuple{any_ref{*this}}; }
	any_tuple any::tuple() const { return any_tuple{any_ref{*this}}; }

	expected<any_range, std::error_code> any_ref::range(std::nothrow_t)
	{
		if (m_type->range_data == nullptr) [[unlikely]]
			return unexpected{make_error_code(type_errc::INVALID_TYPE)};
		return any_range{std::in_place, *this};
	}
	expected<any_range, std::error_code> any_ref::range(std::nothrow_t) const
	{
		if (m_type->range_data == nullptr) [[unlikely]]
			return unexpected{make_error_code(type_errc::INVALID_TYPE)};
		return any_range{std::in_place, *this};
	}
	expected<any_table, std::error_code> any_ref::table(std::nothrow_t)
	{
		if (m_type->table_data == nullptr) [[unlikely]]
			return unexpected{make_error_code(type_errc::INVALID_TYPE)};
		return any_table{std::in_place, *this};
	}
	expected<any_table, std::error_code> any_ref::table(std::nothrow_t) const
	{
		if (m_type->table_data == nullptr) [[unlikely]]
			return unexpected{make_error_code(type_errc::INVALID_TYPE)};
		return any_table{std::in_place, *this};
	}
	expected<any_tuple, std::error_code> any_ref::tuple(std::nothrow_t)
	{
		if (m_type->tuple_data == nullptr) [[unlikely]]
			return unexpected{make_error_code(type_errc::INVALID_TYPE)};
		return any_tuple{std::in_place, *this};
	}
	expected<any_tuple, std::error_code> any_ref::tuple(std::nothrow_t) const
	{
		if (m_type->tuple_data == nullptr) [[unlikely]]
			return unexpected{make_error_code(type_errc::INVALID_TYPE)};
		return any_tuple{std::in_place, *this};
	}

	any_range any_ref::range() { return any_range{*this}; }
	any_range any_ref::range() const { return any_range{*this}; }
	any_table any_ref::table() { return any_table{*this}; }
	any_table any_ref::table() const { return any_table{*this}; }
	any_tuple any_ref::tuple() { return any_tuple{*this}; }
	any_tuple any_ref::tuple() const { return any_tuple{*this}; }
}	 // namespace sek