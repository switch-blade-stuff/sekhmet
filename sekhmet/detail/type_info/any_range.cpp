//
// Created by switch_blade on 2022-10-03.
//

#include "any_range.hpp"

#include "type_info.hpp"
#include <fmt/format.h>

namespace sek
{
	any_range::range_iterator::range_iterator(const range_iterator &other)
		: m_iter(other.m_iter ? other.m_iter->make_copy() : nullptr)
	{
	}
	any_range::range_iterator &any_range::range_iterator::operator=(const range_iterator &other)
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

	bool any_range::range_iterator::is_bidirectional() const noexcept { return m_iter->is_bidirectional(); }
	bool any_range::range_iterator::is_random_access() const noexcept { return m_iter->is_random_access(); }

	any_range::range_iterator any_range::range_iterator::operator++(int)
	{
		if (m_iter) [[likely]]
		{
			auto tmp = *this;
			++(*this);
			return tmp;
		}
		return {};
	}
	any_range::range_iterator &any_range::range_iterator::operator++()
	{
		if (m_iter) [[likely]]
			m_iter->inc();
		return *this;
	}
	any_range::range_iterator &any_range::range_iterator::operator+=(difference_type n)
	{
		if (m_iter) [[likely]]
			m_iter->inc(n);
		return *this;
	}
	any_range::range_iterator any_range::range_iterator::operator--(int)
	{
		if (m_iter) [[likely]]
		{
			auto tmp = *this;
			--(*this);
			return tmp;
		}
		return {};
	}
	any_range::range_iterator &any_range::range_iterator::operator--()
	{
		if (m_iter) [[likely]]
			m_iter->dec();
		return *this;
	}
	any_range::range_iterator &any_range::range_iterator::operator-=(difference_type n)
	{
		if (m_iter) [[likely]]
			m_iter->dec(n);
		return *this;
	}

	any_range::range_iterator any_range::range_iterator::operator+(difference_type n) const
	{
		if (m_iter) [[likely]]
		{
			auto result = *this;
			result.m_iter->inc(n);
			return result;
		}
		return *this;
	}
	any_range::range_iterator any_range::range_iterator::operator-(difference_type n) const
	{
		if (m_iter) [[likely]]
		{
			auto result = *this;
			result.m_iter->dec(n);
			return result;
		}
		return *this;
	}
	std::ptrdiff_t any_range::range_iterator::operator-(const range_iterator &other) const
	{
		if (m_iter && other.m_iter) [[likely]]
			return *m_iter - *other.m_iter;
		return 0;
	}

	any any_range::range_iterator::value() const { return m_iter->value(); }
	any any_range::range_iterator::operator[](difference_type n) const
	{
		if (is_random_access()) [[likely]]
			return *(*this - n);
		return any{};
	}

	bool any_range::range_iterator::operator==(const range_iterator &other) const
	{
		return m_iter == other.m_iter || (m_iter && other.m_iter && *m_iter == *other.m_iter);
	}
	bool any_range::range_iterator::operator>(const range_iterator &other) const
	{
		return m_iter && other.m_iter && *m_iter > *other.m_iter;
	}
	bool any_range::range_iterator::operator>=(const range_iterator &other) const
	{
		return m_iter == other.m_iter || (m_iter && other.m_iter && *m_iter >= *other.m_iter);
	}
	bool any_range::range_iterator::operator<(const range_iterator &other) const
	{
		return m_iter && other.m_iter && *m_iter < *other.m_iter;
	}
	bool any_range::range_iterator::operator<=(const range_iterator &other) const
	{
		return m_iter == other.m_iter || (m_iter && other.m_iter && *m_iter <= *other.m_iter);
	}

	const detail::range_type_data *any_range::assert_data(const detail::type_data *data)
	{
		if (data->range_data == nullptr) [[unlikely]]
			throw type_error(make_error_code(type_errc::INVALID_TYPE), fmt::format("<{}> is not a range", data->name));
		return data->range_data;
	}

	typename any_range::iterator any_range::begin()
	{
		if (is_forward_range()) [[likely]]
			return iterator{m_data->begin(m_target)};
		return iterator{};
	}
	typename any_range::const_iterator any_range::begin() const
	{
		if (is_forward_range()) [[likely]]
			return const_iterator{m_data->cbegin(m_target)};
		return const_iterator{};
	}
	typename any_range::iterator any_range::end()
	{
		if (is_forward_range()) [[likely]]
			return iterator{m_data->end(m_target)};
		return iterator{};
	}
	typename any_range::const_iterator any_range::end() const
	{
		if (is_forward_range()) [[likely]]
			return const_iterator{m_data->cend(m_target)};
		return const_iterator{};
	}
	typename any_range::reverse_iterator any_range::rbegin()
	{
		if (is_bidirectional_range()) [[likely]]
			return reverse_iterator{iterator{m_data->rbegin(m_target)}};
		return reverse_iterator{};
	}
	typename any_range::const_reverse_iterator any_range::rbegin() const
	{
		if (is_bidirectional_range()) [[likely]]
			return const_reverse_iterator{const_iterator{m_data->crbegin(m_target)}};
		return const_reverse_iterator{};
	}
	typename any_range::reverse_iterator any_range::rend()
	{
		if (is_bidirectional_range()) [[likely]]
			return reverse_iterator{iterator{m_data->rend(m_target)}};
		return reverse_iterator{};
	}
	typename any_range::const_reverse_iterator any_range::rend() const
	{
		if (is_bidirectional_range()) [[likely]]
			return const_reverse_iterator{const_iterator{m_data->crend(m_target)}};
		return const_reverse_iterator{};
	}

	bool any_range::empty() const { return m_data->empty(m_target.data()); }
	std::size_t any_range::size() const { return is_sized_range() ? m_data->size(m_target.data()) : 0; }

	any any_range::front() { return is_forward_range() ? m_data->front(m_target) : any{}; }
	any any_range::front() const { return is_forward_range() ? m_data->cfront(m_target) : any{}; }
	any any_range::back() { return is_bidirectional_range() ? m_data->back(m_target) : any{}; }
	any any_range::back() const { return is_bidirectional_range() ? m_data->cback(m_target) : any{}; }
	any any_range::at(size_type n) { return is_random_access_range() ? m_data->at(m_target, n) : any{}; }
	any any_range::at(size_type n) const { return is_random_access_range() ? m_data->cat(m_target, n) : any{}; }
}	 // namespace sek