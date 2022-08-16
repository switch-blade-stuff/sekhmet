/*
 * Created by switchblade on 14/08/22
 */

#include "uri.hpp"

#include <utility>

#include <string_view>

namespace sek
{
	struct uri::list_handle::list_header
	{
		explicit list_header(size_type capacity) noexcept : capacity(capacity), size(0) {}
		list_header(const list_header &other) : capacity(other.size), size(other.size)
		{
			std::uninitialized_copy_n(other.begin(), size, begin());
		}
		list_header(const list_header &other, size_type capacity) : capacity(capacity), size(other.size)
		{
			std::uninitialized_copy_n(other.begin(), size, begin());
		}

		[[nodiscard]] constexpr iterator begin() noexcept { return iterator{data()}; }
		[[nodiscard]] constexpr const_iterator begin() const noexcept { return const_iterator{data()}; }
		[[nodiscard]] constexpr iterator end() noexcept { return iterator{data() + size}; }
		[[nodiscard]] constexpr const_iterator end() const noexcept { return const_iterator{data() + size}; }

		[[nodiscard]] constexpr reference front() noexcept { return *begin(); }
		[[nodiscard]] constexpr const_reference front() const noexcept { return *begin(); }
		[[nodiscard]] constexpr reference back() noexcept { return end()[-1]; }
		[[nodiscard]] constexpr const_reference back() const noexcept { return end()[-1]; }

		[[nodiscard]] constexpr pointer data() noexcept { return std::bit_cast<pointer>(this + 1); }
		[[nodiscard]] constexpr const_pointer data() const noexcept { return std::bit_cast<const_pointer>(this + 1); }

		size_type capacity;
		size_type size;

		/* Elements follow the list. */
	};

	uri::list_handle::list_header *uri::list_handle::alloc_list(size_type cap)
	{
		const auto bytes = sizeof(list_header) + cap * sizeof(element);
		const auto align = std::align_val_t{alignof(list_header)};
		return static_cast<list_header *>(::operator new(bytes, align));
	}
	void uri::list_handle::dealloc_list(list_header *list)
	{
		const auto bytes = sizeof(list_header) + list->capacity * sizeof(element);
		const auto align = std::align_val_t{alignof(list_header)};
		::operator delete(list, bytes, align);
	}

	uri::list_handle::list_handle(const list_handle &other)
	{
		if (other.m_data != nullptr)
		{
			m_data = alloc_list(other.size());
			std::construct_at(m_data, *other.m_data);
		}
	}
	uri::list_handle &uri::list_handle::operator=(const list_handle &other)
	{
		if (this != &other)
		{
			if (other.m_data != nullptr)
			{
				if (m_data == nullptr || m_data->capacity < other.m_data->size) [[unlikely]]
				{
					const auto old_list = m_data;

					m_data = alloc_list(other.m_data->size);
					dealloc_list(old_list);
				}

				std::construct_at(m_data, *other.m_data);
			}
			else
				m_data->size = 0;
		}
		return *this;
	}
	uri::list_handle::~list_handle() { dealloc_list(m_data); }

	uri::list_handle::iterator uri::list_handle::begin() const noexcept
	{
		const auto header = m_data;
		return header != nullptr ? header->begin() : iterator{};
	}
	uri::list_handle::iterator uri::list_handle::cbegin() const noexcept { return begin(); }
	uri::list_handle::iterator uri::list_handle::end() const noexcept
	{
		const auto header = m_data;
		return header != nullptr ? header->end() : iterator{};
	}
	uri::list_handle::iterator uri::list_handle::cend() const noexcept { return end(); }

	uri::list_handle::size_type uri::list_handle::size() const noexcept { return m_data->size; }
	uri::list_handle::size_type uri::list_handle::capacity() const noexcept { return m_data->capacity; }

	uri::list_handle::reference uri::list_handle::front() const noexcept { return m_data->front(); }
	uri::list_handle::reference uri::list_handle::back() const noexcept { return m_data->back(); }

	bool uri::list_handle::empty() const noexcept
	{
		const auto header = m_data;
		return header == nullptr || header->size == 0;
	}
	void uri::list_handle::clear() const noexcept
	{
		if (const auto header = m_data; header != nullptr) [[likely]]
			header->size = 0;
	}

	uri::list_handle::iterator uri::list_handle::push_back(const element &value)
	{
		list_header *header;
		size_type old_size;

		if (header = m_data; header == nullptr) [[unlikely]]
		{
			m_data = header = alloc_list(min_capacity);
			std::construct_at(header, min_capacity);
		}
		else if (old_size == header->capacity) [[unlikely]]
		{
			const auto new_capacity = old_size * 2;
			m_data = alloc_list(new_capacity);

			std::construct_at(m_data, *header, new_capacity);
			dealloc_list(std::exchange(header, m_data));
		}
		old_size = header->size++;

		const auto result = header->begin() + old_size;
		*result = value;

		return result;
	}
	uri::list_handle::iterator uri::list_handle::erase(iterator where) noexcept
	{
		const auto dst = begin() + std::distance(cbegin(), where);
		std::uninitialized_copy(where + 1, cend(), dst);
		return dst;
	}

	constexpr auto uri::skip_scheme(auto iter) const noexcept
	{
		if (const auto idx = iter->offset + iter->size; m_data.size() > idx && m_data[idx] == scheme_postfix)
			return std::next(iter);
		return iter;
	}
	constexpr auto uri::skip_userinfo(auto iter) const noexcept
	{
		if (const auto idx = iter->offset + iter->size; m_data.size() > idx && m_data[idx] == user_postfix)
			return std::next(iter);
		return iter;
	}
	constexpr auto uri::skip_port(auto iter) const noexcept
	{
		if (m_data[iter->offset - 1] == port_prefix) return std::next(iter);
		return iter;
	}
	constexpr auto uri::skip_fragment(auto iter) const noexcept
	{
		if (iter->offset > 0 && m_data[iter->offset - 1] == frag_prefix) return std::prev(iter);
		return iter;
	}

	uri::list_handle::iterator uri::find_scheme() const noexcept
	{
		const auto header = m_list.m_data;
		if (header != nullptr && header->size > 1) [[likely]]
		{
			const auto iter = header->begin();
			if (const auto idx = iter->offset + iter->size; m_data.size() > idx && m_data[idx] == scheme_postfix)
				return iter;
		}
		return header->end();
	}
	uri::list_handle::iterator uri::find_userinfo() const noexcept
	{
		const auto header = m_list.m_data;
		if (header != nullptr && header->size > 1) [[likely]]
		{
			const auto iter = skip_scheme(header->begin());
			const auto idx = iter->offset + iter->size;
			if (sv_type{m_data.data(), iter->offset}.ends_with(sv_type{auth_prefix}) && m_data.size() > idx &&
				m_data[idx] == user_postfix)
				return iter;
		}
		return header->end();
	}
	uri::list_handle::iterator uri::find_host() const noexcept
	{
		const auto header = m_list.m_data;
		if (header != nullptr && header->size > 1) [[likely]]
		{
			const auto iter = skip_scheme(header->begin());
			if (sv_type{m_data.data(), iter->offset}.ends_with(sv_type{auth_prefix})) return skip_userinfo(iter);
		}
		return header->end();
	}
	uri::list_handle::iterator uri::find_port() const noexcept
	{
		const auto header = m_list.m_data;
		if (header != nullptr && header->size > 1) [[likely]]
		{
			if (auto iter = skip_scheme(header->begin()); sv_type{m_data.data(), iter->offset}.ends_with(sv_type{auth_prefix}))
			{
				iter = skip_userinfo(iter) + 1;
				if (m_data[iter->offset - 1] == port_prefix) return iter;
			}
		}
		return header->end();
	}

	uri::list_handle::iterator uri::find_query() const noexcept
	{
		const auto header = m_list.m_data;
		if (header != nullptr && header->size > 1) [[likely]]
		{
			const auto iter = skip_fragment(m_list.end() - 1);
			if (iter->offset > 0 && m_data[iter->offset - 1] == query_prefix) return iter;
		}
		return header->end();
	}
	uri::list_handle::iterator uri::find_fragment() const noexcept
	{
		const auto header = m_list.m_data;
		if (header != nullptr && header->size > 1) [[likely]]
		{
			const auto iter = header->end() - 1;
			const auto idx = iter->offset;
			if (idx > 0 && m_data[idx - 1] == frag_prefix) return iter;
		}
		return header->end();
	}

	std::pair<uri::list_handle::iterator, uri::list_handle::iterator> uri::find_path() const noexcept
	{
		if (const auto header = m_list.m_data; header != nullptr && header->size != 0) [[likely]]
		{
			/* Find the first path element. */
			auto first = skip_scheme(header->begin());

			/* Skip authority. */
			if (sv_type{m_data.data(), first->offset}.ends_with(auth_prefix))
				first = skip_port(skip_userinfo(first) + 1);

			/* Find the last path element. */
			auto last = m_list.end();

			/* Skip fragment. */
			if (const auto fragment = last - 1; fragment->offset > 0 && m_data[fragment->offset - 1] == frag_prefix)
				last = fragment;
			/* Skip query. */
			if (const auto query = last - 1; query->offset > 0 && m_data[query->offset - 1] == query_prefix)
				last = query;

			return {first, last};
		}
		return {};
	}
	std::pair<uri::list_handle::iterator, uri::list_handle::iterator> uri::find_auth() const noexcept
	{
		if (const auto header = m_list.m_data; header != nullptr && header->size > 1) [[likely]]
		{
			/* If authority prefix is present, find the end element. */
			if (auto iter = skip_scheme(header->begin()); sv_type{m_data.data(), iter->offset}.ends_with(sv_type{auth_prefix}))
			{
				const auto first = ++iter;
				return {first, skip_port(skip_userinfo(first) + 1)};
			}
		}
		return {};
	}

	bool uri::has_scheme() const noexcept
	{
		if (const auto header = m_list.m_data; header != nullptr && header->size > 1) [[likely]]
		{
			const auto idx = header->front().offset + header->front().size;
			return m_data.size() > idx && m_data[idx] == scheme_postfix;
		}
		return false;
	}

	bool uri::has_authority() const noexcept
	{
		if (const auto header = m_list.m_data; header != nullptr && header->size > 1) [[likely]]
		{
			const auto prefix = sv_type{auth_prefix};

			/* If there is no scheme, authority will start from a non-0 index due to the prefix.
			 * Otherwise, authority will be the second element. */
			if (auto iter = header->begin(); iter->offset >= prefix.length() || ++iter != header->end())
				return sv_type{m_data.data(), iter->offset}.ends_with(sv_type{auth_prefix});
		}
		return false;
	}
	bool uri::has_userinfo() const noexcept
	{
		if (const auto header = m_list.m_data; header != nullptr && header->size > 1) [[likely]]
		{
			/* Userinfo exists if there is an authority and the first
			 * element of the authority ends with the user postfix. */
			if (auto iter = skip_scheme(header->begin()); sv_type{m_data.data(), iter->offset}.ends_with(sv_type{auth_prefix}))
			{
				const auto idx = iter->offset + iter->size;
				return m_data.size() > idx && m_data[idx] == user_postfix;
			}
		}
		return false;
	}
	bool uri::has_host() const noexcept
	{
		/* Host is required for an authority. */
		return has_authority();
	}
	bool uri::has_port() const noexcept
	{
		if (const auto header = m_list.m_data; header != nullptr && header->size > 1) [[likely]]
		{
			auto iter = skip_scheme(header->begin());

			/* Port exists if there is an authority and the last element of the authority starts with the user postfix. */
			if (sv_type{m_data.data(), iter->offset}.ends_with(sv_type{auth_prefix}))
			{
				/* Skip userinfo & host. */
				iter = skip_userinfo(iter) + 1;

				/* Check for port prefix. Host is never the last element since the path always exists. */
				return m_data[iter->offset - 1] == port_prefix;
			}
		}
		return false;
	}

	bool uri::has_pathinfo() const noexcept
	{
		/* Pathinfo exists if the size of the path is at least 2. */
		const auto path = find_path();
		return std::distance(path.first, path.second) >= 2;
	}

	bool uri::has_query() const noexcept
	{
		if (const auto header = m_list.m_data; header != nullptr && header->size != 0) [[likely]]
		{
			const auto iter = skip_fragment(m_list.end() - 1);
			return iter->offset > 0 && m_data[iter->offset - 1] == query_prefix;
		}
		return false;
	}
	bool uri::has_fragment() const noexcept
	{
		if (const auto header = m_list.m_data; header != nullptr && header->size != 0) [[likely]]
		{
			const auto idx = header->back().offset;
			return idx > 0 && m_data[idx - 1] == frag_prefix;
		}
		return false;
	}

	bool uri::is_local() const noexcept { return m_data.starts_with("file:"); }

	void uri::offset_elements(difference_type off) noexcept
	{
		for (auto &element : m_list) element.offset += static_cast<size_type>(off);
	}

	uri uri::scheme() const
	{
		uri result;
		if (const auto iter = find_scheme(); iter != m_list.end()) [[likely]]
		{
			result.m_data = sv_type{m_data.data() + iter->offset, iter->size + 1};
			result.m_list.push_back(*iter);
		}
		return result;
	}

	uri &uri::scheme(const uri &new_scheme)
	{
		std::string_view scheme_str = {};

		if (const auto iter = new_scheme.find_scheme(); iter != new_scheme.m_list.end()) [[likely]]
			scheme_str = new_scheme.element_view(*iter);

		return scheme(scheme_str);
	}
	uri &uri::scheme(std::string_view new_scheme)
	{
		auto diff = static_cast<uri::difference_type>(new_scheme.size());

		/* If there is a scheme, take the old scheme's length into account. */
		if (const auto iter = find_scheme(); iter != m_list.end()) [[likely]]
		{
			diff -= static_cast<uri::difference_type>(iter->size);

			/* If the new scheme is empty, erase the element, otherwise update length. */
			if (!new_scheme.empty()) [[likely]]
				iter->size = new_scheme.size();
			else
				m_list.erase(iter);
		}

		if (diff > 0) /* Insert or erase padding bytes. */
			m_data.insert(0, static_cast<uri::size_type>(diff), '\0');
		else if (diff < 0)
			m_data.erase(0, static_cast<uri::size_type>(-diff));

		/* Copy the scheme & offset all elements. */
		std::uninitialized_copy_n(new_scheme.data(), new_scheme.size(), m_data.begin());
		offset_elements(diff);

		return *this;
	}

	uri uri::authority() const
	{
		uri result;
		const auto auth = find_auth();
		for (auto iter = auth.first, end = auth.second; iter != end; ++iter)
		{
			if (iter == auth.first) /* If it is the first element, add authority prefix. */
				result.m_data.append(sv_type{auth_prefix});

			const auto offset = iter->offset;
			const auto size = iter->size;

			/* New element offset needs to be adjusted for the size of the new string. */
			result.m_list.push_back(element{result.m_data.size(), size});
			result.m_data.append(sv_type{m_data.data() + offset, size});

			if (iter + 1 != end) /* If it is not the last element, add postfix. */
				result.m_data.append(1, m_data[offset + size]);
		}
		return result;
	}

	uri uri::host() const
	{
		uri result;
		if (const auto iter = find_host(); iter != m_list.end()) [[likely]]
		{
			const auto prefix = sv_type{auth_prefix};

			result.m_data.append(prefix);
			result.m_data.append(sv_type{m_data.data() + iter->offset, iter->size});
			result.m_list.push_back(element{prefix.size(), iter->size});
		}
		return result;
	}

	uri uri::path() const
	{
		uri result;
		const auto path = find_path();
		for (auto iter = path.first, end = path.second; iter != end; ++iter)
		{
			const auto offset = iter->offset;
			const auto size = iter->size;

			/* If path does not start from 0 and previous character is a separator, include the separator. */
			if (const auto sep_off = offset - 1; offset != 0 && m_data[sep_off] != scheme_postfix) [[likely]]
				result.m_data.append(1, m_data[sep_off]);

			/* New element offset needs to be adjusted for the size of the new string. */
			result.m_list.push_back(element{result.m_data.size(), size});
			result.m_data.append(sv_type{m_data.data() + offset, size});
		}
		return result;
	}
	std::filesystem::path uri::fs_path() const
	{
		std::filesystem::path result;

		// clang-format off
		const auto path = find_path();
		for (auto iter = path.first, end = path.second; iter != end; ++iter)
			result /= element_view(*iter);
		// clang-format on

		return result;
	}

	uri uri::query() const
	{
		uri result;
		if (const auto iter = find_query(); iter != m_list.end()) [[likely]]
		{
			const auto offset = iter->offset;
			const auto size = iter->size;

			result.m_data.append(1, query_prefix);
			result.m_data.append(sv_type{m_data.data() + offset, size});
			result.m_list.push_back(element{1, size});
		}
		return result;
	}

	uri uri::fragment() const
	{
		uri result;
		if (const auto iter = find_fragment(); iter != m_list.end()) [[likely]]
		{
			const auto offset = iter->offset;
			const auto size = iter->size;

			result.m_data.append(1, frag_prefix);
			result.m_data.append(sv_type{m_data.data() + offset, size});
			result.m_list.push_back(element{1, size});
		}
		return result;
	}

	uri &uri::append(const uri &other, value_type q_sep)
	{ /* TODO: Implement this. */
	}
}	 // namespace sek