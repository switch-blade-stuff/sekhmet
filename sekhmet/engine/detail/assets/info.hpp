/*
 * Created by switchblade on 10/08/22
 */

#pragma once

#include <filesystem>

#include "sekhmet/dense_map.hpp"
#include "sekhmet/dense_set.hpp"
#include "sekhmet/intern.hpp"
#include "sekhmet/uri.hpp"
#include "sekhmet/uuid.hpp"

#include "asset_io.hpp"
#include "fwd.hpp"

namespace sek::engine::detail
{
	struct asset_info
	{
		asset_info() = delete;
		asset_info(const asset_info &) = delete;
		asset_info(asset_info &&) = delete;

		constexpr explicit asset_info(package_info *parent) : parent(parent) {}

		[[nodiscard]] inline bool has_metadata() const noexcept;

		package_info *parent; /* Parent package of the asset. */

		interned_string name;			 /* Optional human-readable name of the asset. */
		dense_set<interned_string> tags; /* Optional tags of the asset. */
	};
	struct asset_table
	{
		using uuid_table_t = dense_map<uuid, asset_info *>;
		using name_table_t = dense_map<std::string_view, uuid>;

		class entry_iterator;
		class entry_ptr;

		typedef asset_handle value_type;
		typedef entry_iterator iterator;
		typedef entry_iterator const_iterator;
		typedef std::reverse_iterator<const_iterator> reverse_iterator;
		typedef std::reverse_iterator<const_iterator> const_reverse_iterator;
		typedef asset_handle reference;
		typedef asset_handle const_reference;
		typedef entry_ptr pointer;
		typedef entry_ptr const_pointer;
		typedef std::size_t size_type;
		typedef std::ptrdiff_t difference_type;

		[[nodiscard]] constexpr bool empty() const noexcept { return uuid_table.empty(); }
		[[nodiscard]] constexpr size_type size() const noexcept { return uuid_table.size(); }

		[[nodiscard]] constexpr auto begin() const noexcept;
		[[nodiscard]] constexpr auto end() const noexcept;
		[[nodiscard]] constexpr auto rbegin() const noexcept;
		[[nodiscard]] constexpr auto rend() const noexcept;

		[[nodiscard]] constexpr auto find(uuid) const;
		[[nodiscard]] constexpr auto find(std::string_view) const;
		[[nodiscard]] constexpr auto match(auto &&) const;
		[[nodiscard]] inline auto find_all(std::string_view) const;
		[[nodiscard]] inline auto match_all(auto &&) const;

		dense_map<uuid, asset_info *> uuid_table;
		dense_map<std::string_view, uuid> name_table;
	};

	class package_info : public asset_table
	{
	public:
		static asset_source make_source(asset_io_data &&, std::uint64_t, std::uint64_t) noexcept;

	public:
		package_info(const uri &location) : m_location(location) {}
		package_info(uri &&location) noexcept : m_location(std::move(location)) {}

		SEK_API virtual ~package_info();

		[[nodiscard]] constexpr const uri &location() const noexcept { return m_location; }

		SEK_API void acquire();
		SEK_API void release();

		void insert(uuid id, asset_info *info);
		void erase(uuid id);

		[[nodiscard]] virtual asset_info *alloc_info() = 0;
		virtual void dealloc_info(asset_info *info) = 0;
		virtual void destroy_info(asset_info *info) = 0;
		void delete_info(asset_info *info)
		{
			destroy_info(info);
			dealloc_info(info);
		}

		expected<system::native_file, std::error_code> open_archive(std::uint64_t offset) const;

		virtual expected<asset_source, std::error_code> open_asset(const asset_info *) const noexcept = 0;
		virtual expected<asset_source, std::error_code> open_metadata(const asset_info *) const noexcept = 0;

		[[nodiscard]] virtual bool has_metadata(const asset_info *) const noexcept = 0;

	protected:
		void destroy_all();

	private:
		std::atomic<std::size_t> m_refs;
		uri m_location;

	public:
#ifdef SEK_EDITOR
		event<void(const asset_handle &)> asset_added;
		event<void(const asset_handle &)> asset_removed;
#endif
	};

	bool asset_info::has_metadata() const noexcept { return parent->has_metadata(this); }

	struct asset_info_ptr
	{
		constexpr asset_info_ptr() noexcept = default;
		~asset_info_ptr() { release(); }

		asset_info_ptr(const asset_info_ptr &other) noexcept : info(other.info) { acquire(); }
		asset_info_ptr &operator=(const asset_info_ptr &other) noexcept
		{
			if (this != &other) reset(other.info);
			return *this;
		}

		constexpr asset_info_ptr(asset_info_ptr &&other) noexcept { swap(other); }
		constexpr asset_info_ptr &operator=(asset_info_ptr &&other) noexcept
		{
			swap(other);
			return *this;
		}

		constexpr explicit asset_info_ptr(asset_info *info) noexcept : info(info) {}

		[[nodiscard]] constexpr bool empty() const noexcept { return info == nullptr; }
		[[nodiscard]] constexpr asset_info *operator->() const noexcept { return info; }

		void acquire() const
		{
			if (info != nullptr) [[likely]]
				info->parent->acquire();
		}
		void release() const
		{
			if (info != nullptr) [[likely]]
				info->parent->release();
		}
		void reset(asset_info *new_info)
		{
			release();
			info = new_info;
			acquire();
		}
		void reset()
		{
			release();
			info = nullptr;
		}

		[[nodiscard]] constexpr bool operator==(const asset_info_ptr &other) const noexcept
		{
			return info == other.info;
		}

		constexpr void swap(asset_info_ptr &other) noexcept { std::swap(info, other.info); }

		asset_info *info = nullptr;
	};
	struct package_info_ptr
	{
		constexpr package_info_ptr() noexcept = default;
		~package_info_ptr() { release(); }

		package_info_ptr(const package_info_ptr &other) noexcept : pkg(other.pkg) { acquire(); }
		package_info_ptr &operator=(const package_info_ptr &other) noexcept
		{
			if (this != &other) reset(other.pkg);
			return *this;
		}

		constexpr package_info_ptr(package_info_ptr &&other) noexcept { swap(other); }
		constexpr package_info_ptr &operator=(package_info_ptr &&other) noexcept
		{
			swap(other);
			return *this;
		}

		constexpr explicit package_info_ptr(package_info *pkg) noexcept : pkg(pkg) {}

		[[nodiscard]] constexpr bool empty() const noexcept { return pkg == nullptr; }
		[[nodiscard]] constexpr package_info *operator->() const noexcept { return pkg; }

		void acquire() const
		{
			if (pkg != nullptr) [[likely]]
				pkg->acquire();
		}
		void release() const
		{
			if (pkg != nullptr) [[likely]]
				pkg->release();
		}
		void reset(package_info *new_pkg)
		{
			release();
			pkg = new_pkg;
			acquire();
		}
		void reset()
		{
			release();
			pkg = nullptr;
		}

		[[nodiscard]] constexpr bool operator==(const package_info_ptr &other) const noexcept
		{
			return pkg == other.pkg;
		}

		constexpr void swap(package_info_ptr &other) noexcept { std::swap(pkg, other.pkg); }

		package_info *pkg = nullptr;
	};
}	 // namespace sek::engine::detail