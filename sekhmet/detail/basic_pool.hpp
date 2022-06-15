/*
 * ============================================================================
 * Sekhmet - C++20 game engine & editor
 * Copyright (C) 2022 switchblade
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 * ============================================================================
 *
 * Created by switchblade on 26/05/22
 */

#pragma once

#include <bit>

#include "define.h"
#include "packed_pair.hpp"

namespace sek::detail
{
	template<typename T, typename Alloc = std::allocator<T>>
	struct basic_pool
	{
		union node_t
		{
			node_t *next;
			T value;
		};
		struct page_header
		{
			page_header *next;
			std::size_t capacity;
		};

		using alloc_type = typename std::allocator_traits<Alloc>::template rebind_alloc<node_t>;

		constexpr static auto header_size = (sizeof(page_header) - sizeof(page_header) % sizeof(node_t)) +
											(sizeof(page_header) % sizeof(node_t) ? sizeof(node_t) : 0);
		constexpr static auto header_nodes = header_size / sizeof(node_t);

		constexpr static auto initial_capacity = 2048 / sizeof(node_t);

		basic_pool(const basic_pool &) = delete;
		basic_pool &operator=(const basic_pool &) = delete;

		constexpr basic_pool() noexcept = default;
		explicit basic_pool(std::size_t cap) { make_page(cap); }
		constexpr basic_pool(basic_pool &&other) noexcept { swap(other); }
		constexpr basic_pool &operator=(basic_pool &&other) noexcept
		{
			swap(other);
			return *this;
		}
		~basic_pool()
		{
			for (auto *page = last_page(); page != nullptr;)
			{
				auto next = page->next;
				get_alloc().deallocate(std::bit_cast<node_t *>(page), header_nodes + page->capacity);
				page = next;
			}
		}

		T *allocate()
		{
			if (!m_next_free) [[unlikely]]
				make_page(m_total_cap ? m_total_cap : initial_capacity);
			return &std::exchange(m_next_free, m_next_free->next)->value;
		}
		void deallocate(void *ptr) { deallocate(static_cast<T *>(ptr)); }
		void deallocate(T *ptr)
		{
			if (ptr) [[likely]]
			{
				/* Add the node to free node list. */
				auto node = std::bit_cast<node_t *>(ptr);
				node->next = std::exchange(m_next_free, node);
			}
		}

		void make_page(std::size_t cap)
		{
			const auto total_nodes = cap + header_nodes; /* Header is aligned to a multiple of node size. */
			auto ptr = get_alloc().allocate(total_nodes);
			auto nodes = ptr + header_nodes;
			for (auto node = nodes, end = nodes + cap;;)
				if (auto prev = node++; node != end) [[likely]]
					prev->next = node;
				else
				{
					prev->next = nullptr;
					break;
				}
			m_next_free = nodes;

			auto header = std::bit_cast<page_header *>(ptr);
			header->next = last_page();
			header->capacity = cap;

			last_page() = header;
			m_total_cap += cap;
		}

		constexpr void swap(basic_pool &other) noexcept
		{
			using std::swap;
			swap(m_alloc_pages, other.m_alloc_pages);
			swap(m_next_free, other.m_next_free);
		}

		constexpr alloc_type &get_alloc() noexcept { return m_alloc_pages.first(); }
		constexpr page_header *&last_page() noexcept { return m_alloc_pages.second(); }

		packed_pair<alloc_type, page_header *> m_alloc_pages;
		node_t *m_next_free = nullptr;
		std::size_t m_total_cap = 0;
	};
}	 // namespace sek::detail