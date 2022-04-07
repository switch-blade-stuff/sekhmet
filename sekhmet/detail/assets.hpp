//
// Created by switchblade on 2022-04-04.
//

#pragma once

#include <filesystem>

#include "adt/node.hpp"
#include "adt/serialize_impl.hpp"
#include "basic_service.hpp"
#include "filemap.hpp"

namespace sek
{
	namespace detail
	{
		struct asset_record_base;

		struct loose_asset_record;

		SEK_API void serialize(adt::node &, const loose_asset_record &);
		SEK_API void deserialize(const adt::node &, loose_asset_record &);

		struct archive_asset_record;

		SEK_API void serialize(adt::node &, const archive_asset_record &);
		SEK_API void deserialize(const adt::node &, archive_asset_record &);

		struct package_fragment;
		struct master_package;

		SEK_API void acquire(package_fragment *);
		SEK_API void release(package_fragment *);
		SEK_API master_package *get_master(package_fragment *);

		struct asset_collection;
	}	 // namespace detail

	class asset;
	class asset_package;
	class asset_db;

	//#ifdef SEK_EDITOR
	//	namespace editor
	//	{
	//		class editor_asset_db;
	//	}
	//#endif
	//
	//	/** @brief Structure used to reference an asset package. */
	//	class package_handle
	//	{
	//		friend class asset_db;
	//		friend class asset_handle;
	//		SEK_IF_EDITOR(friend class editor::editor_asset_db;)
	//
	//		constexpr explicit package_handle(detail::asset_package_base *ptr) noexcept { init(ptr); }
	//
	//	public:
	//		/** Initializes an empty package reference. */
	//		constexpr package_handle() noexcept = default;
	//
	//		constexpr package_handle(const package_handle &other) noexcept : package_handle(other.package) {}
	//		constexpr package_handle &operator=(const package_handle &other)
	//		{
	//			if (this != &other)
	//			{
	//				release();
	//				init(other.package);
	//			}
	//			return *this;
	//		}
	//		constexpr package_handle(package_handle &&other) noexcept : package(std::exchange(other.package, nullptr)) {}
	//		constexpr package_handle &operator=(package_handle &&other) noexcept
	//		{
	//			swap(other);
	//			return *this;
	//		}
	//
	//		constexpr ~package_handle() { release(); }
	//
	//		/** Checks if the package reference is empty. */
	//		[[nodiscard]] constexpr bool empty() const noexcept { return package == nullptr; }
	//		/** Checks if the package is read-only. */
	//		[[nodiscard]] constexpr bool is_read_only() const noexcept { return package->is_read_only(); }
	//		/** Checks if the package is an archive. */
	//		[[nodiscard]] constexpr bool is_archive() const noexcept { return package->is_archive(); }
	//		/** Returns the path of the package. */
	//		[[nodiscard]] constexpr const std::filesystem::path &path() const noexcept { return package->path; }
	//
	//		/** Finds an asset within this package using it's id.
	//		 * @param id Id of the asset.
	//		 * @return Handle to the asset or an empty handle if such asset was not found. */
	//		[[nodiscard]] asset_handle find(std::string_view id) const;
	//
	//		/** Resets the package reference to an empty state */
	//		constexpr void reset()
	//		{
	//			release();
	//			package = nullptr;
	//		}
	//
	//		constexpr void swap(package_handle &other) noexcept { std::swap(package, other.package); }
	//		friend constexpr void swap(package_handle &a, package_handle &b) noexcept { a.swap(b); }
	//
	//	private:
	//		[[nodiscard]] constexpr auto *master_ptr() noexcept
	//		{
	//			return static_cast<detail::master_asset_package *>(package);
	//		}
	//		[[nodiscard]] constexpr const auto *master_ptr() const noexcept
	//		{
	//			return static_cast<const detail::master_asset_package *>(package);
	//		}
	//
	//		constexpr void release() const
	//		{
	//			if (package) [[likely]]
	//				package->release();
	//		}
	//		constexpr void acquire() const noexcept
	//		{
	//			if (package) [[likely]]
	//				package->acquire();
	//		}
	//		constexpr void init(detail::asset_package_base *ptr) noexcept
	//		{
	//			package = ptr;
	//			acquire();
	//		}
	//
	//		/** Pointer to the package structure. */
	//		detail::asset_package_base *package = nullptr;
	//	};
	//
	//	/** @brief Structure used to reference an asset. */
	//	class asset_handle
	//	{
	//		friend class asset_db;
	//		friend class package_handle;
	//		SEK_IF_EDITOR(friend class editor::editor_asset_db;)
	//
	//		constexpr explicit asset_handle(detail::asset_record_base *record) noexcept { init(record); }
	//
	//	public:
	//		/** Initializes an empty asset handle. */
	//		constexpr asset_handle() noexcept = default;
	//
	//		constexpr asset_handle(const asset_handle &other) noexcept : asset_handle(other.record) {}
	//		constexpr asset_handle &operator=(const asset_handle &other)
	//		{
	//			if (this != &other)
	//			{
	//				release();
	//				init(other.record);
	//			}
	//			return *this;
	//		}
	//		constexpr asset_handle(asset_handle &&other) noexcept : record(std::exchange(other.record, nullptr)) {}
	//		constexpr asset_handle &operator=(asset_handle &&other) noexcept
	//		{
	//			swap(other);
	//			return *this;
	//		}
	//
	//		constexpr ~asset_handle() { release(); }
	//
	//		/** Checks if the asset handle is empty. */
	//		[[nodiscard]] constexpr bool empty() const noexcept { return record == nullptr; }
	//		/** Returns handle to the package containing the asset. */
	//		[[nodiscard]] constexpr package_handle package() const noexcept
	//		{
	//			return package_handle{record->parent->get_master()};
	//		}
	//		/** Returns the id of the asset. */
	//		[[nodiscard]] constexpr const std::string &id() const noexcept { return record->id; }
	//
	//		/** Resets the handle to an empty state. */
	//		constexpr void reset()
	//		{
	//			release();
	//			record = nullptr;
	//		}
	//
	//		constexpr void swap(asset_handle &other) noexcept { std::swap(record, other.record); }
	//		friend constexpr void swap(asset_handle &a, asset_handle &b) noexcept { a.swap(b); }
	//
	//	private:
	//		constexpr void release() const
	//		{
	//			if (record) [[likely]]
	//				record->parent->release();
	//		}
	//		constexpr void acquire() const noexcept
	//		{
	//			if (record) [[likely]]
	//				record->parent->acquire();
	//		}
	//		constexpr void init(detail::asset_record_base *ptr) noexcept
	//		{
	//			record = ptr;
	//			acquire();
	//		}
	//
	//		/** Pointer to the asset data. */
	//		detail::asset_record_base *record = nullptr;
	//	};
	//
	//	/** @brief Structure used to manage assets & asset packages. */
	//	class asset_db : public basic_service<asset_db>, detail::asset_collection
	//	{
	//		using package_map_t = hmap<std::string_view, detail::master_asset_package *>;
	//
	//	public:
	//		/** Initializes an asset database using the current directory as the data directory. */
	//		asset_db() : asset_db(std::filesystem::current_path()) {}
	//		/** Initializes an asset database using the specified data directory. */
	//		explicit asset_db(const std::filesystem::path &data_dir) : data_dir_path(std::filesystem::canonical(data_dir))
	//		{
	//		}
	//
	//		/** Returns path to the current data directory. */
	//		[[nodiscard]] constexpr const std::filesystem::path &data_dir() const noexcept { return data_dir_path; }
	//		/** Sets data directory path. */
	//		void data_dir(std::filesystem::path new_path) { data_dir_path = std::move(new_path); }
	//
	//		/** Finds a global asset using it's id.
	//		 * @param id Id of the asset.
	//		 * @return Handle to the asset or an empty handle if such asset was not found. */
	//		[[nodiscard]] asset_handle find(std::string_view id) const { return asset_handle{find_record(id)}; }
	//
	//		/** Loads a package at the specified path.
	//		 * @param path Path of the package to load relative to the data directory.
	//		 * @param overwrite If set to true, will replace conflicting global assets.
	//		 * @return Handle to the loaded package or an empty handle if it was not loaded.
	//		 * @note If such package is already loaded, will return the loaded package. */
	//		SEK_API package_handle load_package(const std::filesystem::path &path, bool overwrite = true);
	//		/** Checks if the path references a valid package without loading it.
	//		 * @param path Path of the package to check relative to the data directory.
	//		 * @return `1` if the package is a valid master, `-1` if the package is a valid fragment,
	//		 * `0` if the path is not a valid package. */
	//		[[nodiscard]] SEK_API int check_package(const std::filesystem::path &path) const;
	//
	//	protected:
	//		[[nodiscard]] static std::filesystem::path get_manifest_path(const std::filesystem::path &path)
	//		{
	//			return is_directory(path) ? path / ".manifest" : path;
	//		}
	//		[[nodiscard]] static SEK_API std::fstream open_package_manifest(const std::filesystem::path &, std::ios::openmode);
	//		[[nodiscard]] static SEK_API adt::node load_package_manifest(const std::filesystem::path &);
	//
	//		[[nodiscard]] std::filesystem::path get_relative_path(const std::filesystem::path &path) const
	//		{
	//			return proximate(path, data_dir_path);
	//		}
	//
	//		void insert_package(std::string_view path, detail::master_asset_package *ptr)
	//		{
	//			ptr->acquire();
	//			package_map.emplace(path, ptr);
	//		}
	//		void erase_package(package_map_t::const_iterator where)
	//		{
	//			auto temp = where->second;
	//			package_map.erase(where);
	//			temp->release();
	//		}
	//
	//		std::filesystem::path data_dir_path;
	//		package_map_t package_map;
	//	};
	//
	//	extern template struct basic_service<asset_db>;
}	 // namespace sek
