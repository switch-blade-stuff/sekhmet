//
// Created by switch_blade on 2022-04-19.
//

#pragma once

#include <bit>
#include <cstring>

#include <memory_resource>

namespace sek::detail
{
	/** @brief Memory resource used to allocate chunks of bytes from a pool, then release all memory at once. */
	template<std::size_t PageSize>
	class basic_pool_resource : public std::pmr::memory_resource
	{
		struct page_header
		{
			page_header *previous; /* Previous pages are not used for allocation. */
			std::size_t page_size; /* Total size of the page in bytes. */
			std::size_t used_size; /* Amount of data used in bytes. */

			/* Page data follows the header. */
		};

		constexpr static auto page_size_mult = PageSize;

		constexpr static void *align_ptr(void *p, std::size_t align) noexcept
		{
			const auto i = std::bit_cast<std::uintptr_t>(p);
			return std::bit_cast<void *>(i + ((~i + 1) & (align - 1)));
		}

	public:
		basic_pool_resource(const basic_pool_resource &) = delete;
		basic_pool_resource &operator=(const basic_pool_resource &) = delete;

		constexpr basic_pool_resource() noexcept = default;
		constexpr explicit basic_pool_resource(std::pmr::memory_resource *upstream) noexcept : upstream(upstream) {}
		constexpr basic_pool_resource(basic_pool_resource &&other) noexcept { swap(other); }
		constexpr basic_pool_resource &operator=(basic_pool_resource &&other) noexcept
		{
			swap(other);
			return *this;
		}

		~basic_pool_resource() override { release(); }

		void release()
		{
			for (auto *page = main_page; page != nullptr;) page = release_page(page);
			main_page = nullptr;
		}

		void *reallocate(void *old, std::size_t old_n, std::size_t n, std::size_t align = alignof(std::max_align_t))
		{
			if (n <= old_n) [[unlikely]]
				return old;
			else if (!old) [[unlikely]]
				return do_allocate(n, align);

			/* Try to expand if old data is the top allocation and there is enough space for it. */
			const auto *main_page_bytes = page_data(main_page);
			const auto *old_bytes = static_cast<std::byte *>(old);
			const auto new_used = main_page->used_size + n - old_n;
			if (old_bytes + old_n == main_page_bytes + main_page->used_size && new_used <= main_page->page_size) [[likely]]
			{
				main_page->used_size = new_used;
				return old;
			}

			/* Allocate new block & copy. */
			return std::memcpy(do_allocate(n, align), old, old_n);
		}

		constexpr void swap(basic_pool_resource &other) noexcept { std::swap(main_page, other.main_page); }

	protected:
		void *do_allocate(std::size_t n, std::size_t align) override
		{
			/* Add padding to make sure result pointer is aligned. */
			const auto padded = n + align - 1;

			/* Allocate on a new page. */
			if (auto used = padded; !main_page || (used += main_page->used_size) > main_page->page_size) [[unlikely]]
				return alloc_new_page(padded, align);
			else
				return align_ptr(page_data(main_page) + std::exchange(main_page->used_size, used), align);
		}

		void do_deallocate(void *, std::size_t, std::size_t) noexcept override {}
		[[nodiscard]] bool do_is_equal(const std::pmr::memory_resource &) const noexcept override { return false; }

	private:
		void *alloc_new_page(std::size_t padded, std::size_t align)
		{
			auto page_size = padded + sizeof(page_header);
			auto rem = page_size % page_size_mult;
			page_size = page_size - rem + (rem ? page_size_mult : 0);

			auto new_page = insert_page(page_size);
			if (!new_page) [[unlikely]]
				return nullptr;
			new_page->used_size = padded;
			return align_ptr(page_data(new_page), align);
		}
		page_header *release_page(page_header *page_ptr)
		{
			auto previous = page_ptr->previous;
			upstream->deallocate(page_ptr, sizeof(page_header) + page_ptr->page_size);
			return previous;
		}
		page_header *insert_page(std::size_t n)
		{
			auto result = static_cast<page_header *>(upstream->allocate(n));
			if (!result) [[unlikely]]
				return nullptr;
			/* If the previous main page is empty, deallocate it immediately. */
			if (main_page && !main_page->used_size) [[unlikely]]
				result->previous = release_page(main_page);
			else
				result->previous = main_page;
			result->page_size = 0;
			return main_page = result;
		}
		constexpr std::byte *page_data(page_header *header) noexcept
		{
			return std::bit_cast<std::byte *>(header) + sizeof(page_header);
		}

		std::pmr::memory_resource *upstream = nullptr;
		page_header *main_page = nullptr;
	};
}	 // namespace sek::detail