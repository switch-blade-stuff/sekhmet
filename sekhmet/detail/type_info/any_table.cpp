//
// Created by switch_blade on 2022-10-03.
//

#include "any_table.hpp"

#include "type_info.hpp"
#include <fmt/format.h>

namespace sek
{
	any_table::table_iterator::table_iterator(const table_iterator &other)
		: m_iter(other.m_iter ? other.m_iter->make_copy() : nullptr)
	{
	}
	any_table::table_iterator &any_table::table_iterator::operator=(const table_iterator &other)
	{
		if (this != &other)
		{
			if (other.m_iter) [[likely]]
				m_iter.reset(other.m_iter->make_copy());
			else
				m_iter.reset();
		}
		return *this;
	}

	bool any_table::table_iterator::is_bidirectional() const noexcept { return m_iter->is_bidirectional(); }
	bool any_table::table_iterator::is_random_access() const noexcept { return m_iter->is_random_access(); }

	any_table::table_iterator any_table::table_iterator::operator++(int)
	{
		if (m_iter) [[likely]]
		{
			auto tmp = *this;
			++(*this);
			return tmp;
		}
		return {};
	}
	any_table::table_iterator &any_table::table_iterator::operator++()
	{
		if (m_iter) [[likely]]
			m_iter->inc();
		return *this;
	}
	any_table::table_iterator &any_table::table_iterator::operator+=(difference_type n)
	{
		if (m_iter) [[likely]]
			m_iter->inc(n);
		return *this;
	}
	any_table::table_iterator any_table::table_iterator::operator--(int)
	{
		if (m_iter) [[likely]]
		{
			auto tmp = *this;
			--(*this);
			return tmp;
		}
		return {};
	}
	any_table::table_iterator &any_table::table_iterator::operator--()
	{
		if (m_iter) [[likely]]
			m_iter->dec();
		return *this;
	}
	any_table::table_iterator &any_table::table_iterator::operator-=(difference_type n)
	{
		if (m_iter) [[likely]]
			m_iter->dec(n);
		return *this;
	}

	any_table::table_iterator any_table::table_iterator::operator+(difference_type n) const
	{
		if (m_iter) [[likely]]
		{
			auto result = *this;
			result.m_iter->inc(n);
			return result;
		}
		return *this;
	}
	any_table::table_iterator any_table::table_iterator::operator-(difference_type n) const
	{
		if (m_iter) [[likely]]
		{
			auto result = *this;
			result.m_iter->dec(n);
			return result;
		}
		return *this;
	}
	std::ptrdiff_t any_table::table_iterator::operator-(const table_iterator &other) const
	{
		if (m_iter && other.m_iter) [[likely]]
			return *m_iter - *other.m_iter;
		return 0;
	}

	any any_table::table_iterator::value() const { return m_iter->value(); }
	any any_table::table_iterator::key() const { return m_iter->key(); }
	any any_table::table_iterator::mapped() const { return m_iter->mapped(); }
	any any_table::table_iterator::operator[](difference_type n) const
	{
		if (is_random_access()) [[likely]]
			return *(*this - n);
		return any{};
	}

	bool any_table::table_iterator::operator==(const table_iterator &other) const
	{
		return m_iter == other.m_iter || (m_iter && other.m_iter && *m_iter == *other.m_iter);
	}
	bool any_table::table_iterator::operator>(const table_iterator &other) const
	{
		return m_iter && other.m_iter && *m_iter > *other.m_iter;
	}
	bool any_table::table_iterator::operator>=(const table_iterator &other) const
	{
		return m_iter == other.m_iter || (m_iter && other.m_iter && *m_iter >= *other.m_iter);
	}
	bool any_table::table_iterator::operator<(const table_iterator &other) const
	{
		return m_iter && other.m_iter && *m_iter < *other.m_iter;
	}
	bool any_table::table_iterator::operator<=(const table_iterator &other) const
	{
		return m_iter == other.m_iter || (m_iter && other.m_iter && *m_iter <= *other.m_iter);
	}

	const detail::table_type_data *any_table::assert_data(const detail::type_data *data)
	{
		if (data->table_data == nullptr) [[unlikely]]
			throw type_error(make_error_code(type_errc::INVALID_TYPE),
							 fmt::format("<{}> is not a table-like range", data->name));
		return data->table_data;
	}

	typename any_table::iterator any_table::begin() { return iterator{m_data->begin(m_target)}; }
	typename any_table::const_iterator any_table::begin() const { return const_iterator{m_data->cbegin(m_target)}; }
	typename any_table::iterator any_table::end() { return iterator{m_data->end(m_target)}; }
	typename any_table::const_iterator any_table::end() const { return const_iterator{m_data->cend(m_target)}; }
	typename any_table::reverse_iterator any_table::rbegin()
	{
		if (is_bidirectional_range()) [[likely]]
			return reverse_iterator{iterator{m_data->rbegin(m_target)}};
		return reverse_iterator{};
	}
	typename any_table::const_reverse_iterator any_table::rbegin() const
	{
		if (is_bidirectional_range()) [[likely]]
			return const_reverse_iterator{const_iterator{m_data->crbegin(m_target)}};
		return const_reverse_iterator{};
	}
	typename any_table::reverse_iterator any_table::rend()
	{
		if (is_bidirectional_range()) [[likely]]
			return reverse_iterator{iterator{m_data->rend(m_target)}};
		return reverse_iterator{};
	}
	typename any_table::const_reverse_iterator any_table::rend() const
	{
		if (is_bidirectional_range()) [[likely]]
			return const_reverse_iterator{const_iterator{m_data->crend(m_target)}};
		return const_reverse_iterator{};
	}

	bool any_table::contains(const any &key) const
	{
		return m_data->contains && m_data->contains(m_target.data(), key);
	}
	typename any_table::iterator any_table::find(const any &key)
	{
		return m_data->find ? iterator{m_data->find(m_target, key)} : iterator{};
	}
	typename any_table::const_iterator any_table::find(const any &key) const
	{
		return m_data->find ? const_iterator{m_data->cfind(m_target, key)} : const_iterator{};
	}

	bool any_table::empty() const { return m_data->empty(m_target.data()); }
	std::size_t any_table::size() const { return is_sized_range() ? m_data->size(m_target.data()) : 0; }

	any any_table::at(const any &key) { return m_data->at(m_target, key); }
	any any_table::at(const any &key) const { return m_data->cat(m_target, key); }
}	 // namespace sek