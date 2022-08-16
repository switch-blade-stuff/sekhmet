/*
 * Created by switch_blade on 2022-04-19.
 */

#pragma once

#include <bit>
#include <cstring>

#include <memory_resource>

namespace sek::detail
{
	/** @brief Allocator used to allocate chunks of bytes from a pool, then release all memory at once. */
	template<std::size_t PageSize>
	class buffer_allocator
	{
		struct page_header
		{
			page_header *previous; /* Previous pages are not used for allocation. */
			std::size_t page_size; /* Total size of the page in bytes. */
			std::size_t used_size; /* Amount of data used in bytes. */

			/* Page data follows the header. */
		};

		constexpr static auto page_size_mult = PageSize;

		constexpr static void *align_ptr(void *p, std::align_val_t align) noexcept
		{
			const auto i = std::bit_cast<std::uintptr_t>(p);
			return std::bit_cast<void *>(i + ((~i + 1) & (static_cast<std::size_t>(align) - 1)));
		}

	public:
		buffer_allocator(const buffer_allocator &) = delete;
		buffer_allocator &operator=(const buffer_allocator &) = delete;

		constexpr buffer_allocator() noexcept = default;
		constexpr buffer_allocator(buffer_allocator &&other) noexcept { swap(other); }
		constexpr buffer_allocator &operator=(buffer_allocator &&other) noexcept
		{
			swap(other);
			return *this;
		}

		constexpr ~buffer_allocator() { release(); }

		constexpr void release()
		{
			for (auto *page = m_main_page; page != nullptr;) page = release_page(page);
			m_main_page = nullptr;
		}

		constexpr void deallocate(void *, std::size_t, std::size_t) {}
		constexpr void *allocate(std::size_t n) { return allocate(n, std::max_align_t{}); }
		constexpr void *allocate(std::size_t n, std::align_val_t align)
		{
			/* Add padding to make sure result pointer is aligned. */
			const auto padded = n + static_cast<std::size_t>(align) - 1;
			auto used_size = padded;

			/* Allocate on a new page. */
			if (m_main_page == nullptr || (used_size += m_main_page->used_size) > m_main_page->page_size) [[unlikely]]
				insert_page(used_size = padded);
			return align_ptr(page_data(m_main_page) + std::exchange(m_main_page->used_size, used_size), align);
		}
		constexpr void *reallocate(void *old, std::size_t old_n, std::size_t n, std::size_t align = alignof(std::max_align_t))
		{
			if (n <= old_n) [[unlikely]]
				return old;
			else if (!old) [[unlikely]]
				return allocate(n, align);

			/* Try to expand if old data is the top allocation and there is enough space for it. */
			const auto *main_page_bytes = page_data(m_main_page);
			const auto *old_bytes = static_cast<std::byte *>(old);
			const auto new_used = m_main_page->used_size + n - old_n;
			if (old_bytes + old_n == main_page_bytes + m_main_page->used_size && new_used <= m_main_page->page_size) [[likely]]
			{
				m_main_page->used_size = new_used;
				return old;
			}

			/* Allocate new block & copy. */
			return std::memcpy(allocate(n, align), old, old_n);
		}

		constexpr void swap(buffer_allocator &other) noexcept { std::swap(m_main_page, other.m_main_page); }
		friend constexpr void swap(buffer_allocator &a, buffer_allocator &b) noexcept { a.swap(b); }

	private:
		constexpr std::size_t page_size(std::size_t min_size)
		{
			const auto size = min_size + sizeof(page_header);
			const auto rem = size % page_size_mult;
			return size - rem + (rem ? page_size_mult : 0);
		}
		constexpr void insert_page(std::size_t min_size)
		{
			const auto size = page_size(min_size);
			const auto result = static_cast<page_header *>(::operator new(size));

			/* If the previous main page is empty, deallocate it immediately. */
			if (m_main_page != nullptr && m_main_page->used_size == 0) [[unlikely]]
				result->previous = release_page(m_main_page);
			else
				result->previous = m_main_page;
			result->page_size = size;
			return m_main_page = result;
		}
		constexpr page_header *release_page(page_header *page_ptr)
		{
			const auto size = sizeof(page_header) + page_ptr->page_size;
			const auto previous = page_ptr->previous;
			::operator delete(page_ptr, size);
			return previous;
		}
		constexpr std::byte *page_data(page_header *header) noexcept
		{
			return std::bit_cast<std::byte *>(header) + sizeof(page_header);
		}

		page_header *m_main_page = nullptr;
	};
}	 // namespace sek::detail