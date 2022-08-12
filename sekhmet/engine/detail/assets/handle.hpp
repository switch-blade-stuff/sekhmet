/*
 * Created by switchblade on 10/08/22
 */

#pragma once

#include "asset_source.hpp"
#include "info.hpp"

namespace sek::engine
{
	/** @brief Handle to a unique asset of a package.
	 * @note Asset packages are kept alive as long as any of their assets are referenced. */
	class asset_handle
	{
		friend class detail::asset_table::entry_iterator;
		friend class asset_package;
		friend class asset_database;

		constexpr asset_handle(uuid id, detail::asset_info_ptr &&ptr) noexcept
			: m_id(std::move(id)), m_ptr(std::move(ptr))
		{
		}
		asset_handle(uuid id, detail::asset_info *info) : m_id(std::move(id)), m_ptr(info) { m_ptr.acquire(); }

	public:
		/** Initializes an empty asset handle. */
		constexpr asset_handle() noexcept = default;

		constexpr asset_handle(asset_handle &&) noexcept = default;
		constexpr asset_handle &operator=(asset_handle &&other) noexcept
		{
			m_id = std::move(other.m_id);
			m_ptr = std::move(other.m_ptr);
			return *this;
		}
		asset_handle(const asset_handle &) = default;
		asset_handle &operator=(const asset_handle &) = default;

		/** Checks if the asset handle references an asset. */
		[[nodiscard]] constexpr bool empty() const noexcept { return m_ptr.empty(); }
		/** @copydoc empty */
		[[nodiscard]] constexpr operator bool() const noexcept { return !empty(); }

		/** Returns the id of the asset. If the asset handle does not point to an asset, returns a nil uuid. */
		[[nodiscard]] constexpr uuid id() const noexcept { return m_id; }
		/** Returns reference to the name of the asset. */
		[[nodiscard]] constexpr const interned_string &name() const noexcept { return m_ptr->name; }
		/** Returns a set of string tags of the asset. */
		[[nodiscard]] constexpr const dense_set<interned_string> &tags() const noexcept { return m_ptr->tags; }

		/** Returns a handle to the parent package of the asset. */
		[[nodiscard]] inline asset_package package() const;

		/** @brief Opens an asset source used to read asset's data.
		 * @return `asset_source` containing asset's data.
		 * @throw std::system_error On failure to open the file or archive containing the asset. */
		[[nodiscard]] SEK_API asset_source open() const;
		/** @copybrief open
		 * @return `asset_source` containing asset's data or an error code. */
		[[nodiscard]] SEK_API expected<asset_source, std::error_code> open(std::nothrow_t) const noexcept;

		/** Checks if the asset has metadata. */
		[[nodiscard]] bool has_metadata() const { return m_ptr->has_metadata(); }

		/** @brief Opens an asset source used to read asset's metadata.
		 * @return `asset_source` containing asset's metadata.
		 * @throw std::system_error On failure to open the file or archive containing the asset. */
		[[nodiscard]] SEK_API asset_source metadata() const;
		/** @copybrief open
		 * @return `asset_source` containing asset's metadata or an error code. */
		[[nodiscard]] SEK_API expected<asset_source, std::error_code> metadata(std::nothrow_t) const noexcept;

		constexpr void swap(asset_handle &other) noexcept
		{
			m_id.swap(other.m_id);
			m_ptr.swap(other.m_ptr);
		}
		friend constexpr void swap(asset_handle &a, asset_handle &b) noexcept { a.swap(b); }

		/** Returns true if both asset handles reference the *exact* same asset.
		 * @note Multiple asset handles with the same id may reference different assets.
		 * This may happen if the assets were obtained directly from packages (bypassing the database),
		 * thus no overrides could be resolved. */
		[[nodiscard]] constexpr bool operator==(const asset_handle &) const noexcept = default;

	private:
		uuid m_id = uuid::nil();
		detail::asset_info_ptr m_ptr;
	};

	namespace detail
	{
		class asset_table::entry_ptr
		{
			friend class entry_iterator;

			constexpr explicit entry_ptr(asset_handle &&ref) noexcept : m_ref(std::move(ref)) {}

		public:
			entry_ptr() = delete;
			entry_ptr(const entry_ptr &) = delete;
			entry_ptr &operator=(const entry_ptr &) = delete;

			constexpr entry_ptr(entry_ptr &&other) noexcept : m_ref(std::move(other.m_ref)) {}
			constexpr entry_ptr &operator=(entry_ptr &&other) noexcept
			{
				m_ref = std::move(other.m_ref);
				return *this;
			}

			[[nodiscard]] constexpr const asset_handle *get() const noexcept { return std::addressof(m_ref); }
			[[nodiscard]] constexpr const asset_handle *operator->() const noexcept { return get(); }
			[[nodiscard]] constexpr const asset_handle &operator*() const noexcept { return *get(); }

			constexpr void swap(entry_ptr &other) noexcept { m_ref.swap(other.m_ref); }
			friend constexpr void swap(entry_ptr &a, entry_ptr &b) noexcept { a.swap(b); }

			[[nodiscard]] constexpr bool operator==(const entry_ptr &) const noexcept = default;

		private:
			asset_handle m_ref;
		};
		class asset_table::entry_iterator
		{
			friend struct asset_table;

			using uuid_iter = typename uuid_table_t::const_iterator;

		public:
			typedef asset_handle value_type;
			typedef asset_handle reference;
			typedef entry_ptr pointer;
			typedef std::size_t size_type;
			typedef std::ptrdiff_t difference_type;
			typedef std::bidirectional_iterator_tag iterator_category;

		private:
			constexpr entry_iterator(uuid_iter iter) noexcept : m_iter(iter) {}

		public:
			constexpr entry_iterator() noexcept = default;
			constexpr entry_iterator(const entry_iterator &) noexcept = default;
			constexpr entry_iterator(entry_iterator &&) noexcept = default;
			constexpr entry_iterator &operator=(const entry_iterator &) noexcept = default;
			constexpr entry_iterator &operator=(entry_iterator &&) noexcept = default;

			constexpr entry_iterator &operator++() noexcept
			{
				m_iter++;
				return *this;
			}
			entry_iterator operator++(int) noexcept
			{
				auto temp = *this;
				m_iter++;
				return temp;
			}
			constexpr entry_iterator &operator--() noexcept
			{
				m_iter--;
				return *this;
			}
			entry_iterator operator--(int) noexcept
			{
				auto temp = *this;
				m_iter--;
				return temp;
			}

			[[nodiscard]] reference operator*() const { return reference{m_iter->first, m_iter->second}; }
			[[nodiscard]] pointer operator->() const { return pointer{operator*()}; }

			[[nodiscard]] constexpr bool operator==(const entry_iterator &) const noexcept = default;

			constexpr void swap(entry_iterator &other) noexcept { m_iter.swap(other.m_iter); }
			friend constexpr void swap(entry_iterator &a, entry_iterator &b) noexcept { a.swap(b); }

		private:
			uuid_iter m_iter;
		};

		constexpr auto asset_table::begin() const noexcept { return const_iterator{uuid_table.begin()}; }
		constexpr auto asset_table::end() const noexcept { return const_iterator{uuid_table.end()}; }
		constexpr auto asset_table::rbegin() const noexcept { return const_reverse_iterator{end()}; }
		constexpr auto asset_table::rend() const noexcept { return const_reverse_iterator{begin()}; }

		constexpr auto asset_table::find(uuid id) const { return const_iterator{uuid_table.find(id)}; }
		constexpr auto asset_table::find(std::string_view name) const
		{
			if (auto name_iter = name_table.find(name); name_iter != name_table.end()) [[likely]]
				return find(name_iter->second);
			else
				return end();
		}
		constexpr auto asset_table::match(auto &&pred) const { return std::find_if(begin(), end(), pred); }
		inline auto asset_table::find_all(std::string_view name) const
		{
			std::vector<reference> result;
			std::for_each(begin(),
						  end(),
						  [&result, &name](auto entry)
						  {
							  if (entry.name() == name) result.push_back(std::move(entry));
						  });
			return result;
		}
		inline auto asset_table::match_all(auto &&pred) const
		{
			std::vector<reference> result;
			std::for_each(begin(),
						  end(),
						  [&result, &pred](auto entry)
						  {
							  if (pred(entry)) result.push_back(std::move(entry));
						  });
			return result;
		}
	}	 // namespace detail
}	 // namespace sek::engine