//
// Created by switchblade on 2022-04-04.
//

#pragma once

#include <fstream>

#include "asset_info.hpp"
#include "basic_service.hpp"

namespace sek
{
	class asset_db;
	class asset_handle;

	/** @brief Structure used to reference an asset package. */
	class package_handle
	{
		friend class asset_db;
		friend class asset_handle;

		constexpr explicit package_handle(detail::asset_package_base *ptr) noexcept { init(ptr); }

	public:
		/** Initializes an empty package reference. */
		constexpr package_handle() noexcept = default;

		constexpr package_handle(const package_handle &other) noexcept : package_handle(other.package) {}
		constexpr package_handle &operator=(const package_handle &other)
		{
			if (this != &other)
			{
				release();
				init(other.package);
			}
			return *this;
		}
		constexpr package_handle(package_handle &&other) noexcept : package(std::exchange(other.package, nullptr)) {}
		constexpr package_handle &operator=(package_handle &&other) noexcept
		{
			swap(other);
			return *this;
		}

		constexpr ~package_handle() { release(); }

		/** Checks if the package reference is empty. */
		[[nodiscard]] constexpr bool empty() const noexcept { return package == nullptr; }
		/** Checks if the package is read-only. */
		[[nodiscard]] constexpr bool is_read_only() const noexcept { return package->is_read_only(); }
		/** Checks if the package is an archive. */
		[[nodiscard]] constexpr bool is_archive() const noexcept { return package->is_archive(); }
		/** Returns the path of the package. */
		[[nodiscard]] constexpr const std::filesystem::path &path() const noexcept { return package->path; }

		/** Resets the package reference to an empty state */
		constexpr void reset()
		{
			release();
			package = nullptr;
		}

		constexpr void swap(package_handle &other) noexcept { std::swap(package, other.package); }
		friend constexpr void swap(package_handle &a, package_handle &b) noexcept { a.swap(b); }

	private:
		constexpr void release() const
		{
			if (package) [[likely]]
				package->release();
		}
		constexpr void acquire() const noexcept
		{
			if (package) [[likely]]
				package->acquire();
		}
		constexpr void init(detail::asset_package_base *ptr) noexcept
		{
			package = ptr;
			acquire();
		}

		/** Pointer to the package structure. */
		detail::asset_package_base *package = nullptr;
	};

	/** @brief Structure used to reference an asset. */
	class asset_handle
	{
		friend class asset_db;

		constexpr explicit asset_handle(detail::asset_record_base *record) noexcept { init(record); }

	public:
		/** Initializes an empty asset handle. */
		constexpr asset_handle() noexcept = default;

		constexpr asset_handle(const asset_handle &other) noexcept : asset_handle(other.record) {}
		constexpr asset_handle &operator=(const asset_handle &other)
		{
			if (this != &other)
			{
				release();
				init(other.record);
			}
			return *this;
		}
		constexpr asset_handle(asset_handle &&other) noexcept : record(std::exchange(other.record, nullptr)) {}
		constexpr asset_handle &operator=(asset_handle &&other) noexcept
		{
			swap(other);
			return *this;
		}

		constexpr ~asset_handle() { release(); }

		/** Checks if the asset handle is empty. */
		[[nodiscard]] constexpr bool empty() const noexcept { return record == nullptr; }
		/** Returns handle to the package containing the asset. */
		[[nodiscard]] constexpr package_handle package() const noexcept
		{
			return package_handle{record->parent->get_master()};
		}
		/** Returns the id of the asset. */
		[[nodiscard]] constexpr const std::string &id() const noexcept { return record->id; }

		/** Resets the handle to an empty state. */
		constexpr void reset()
		{
			release();
			record = nullptr;
		}

		constexpr void swap(asset_handle &other) noexcept { std::swap(record, other.record); }
		friend constexpr void swap(asset_handle &a, asset_handle &b) noexcept { a.swap(b); }

	private:
		constexpr void release() const
		{
			if (record) [[likely]]
				record->parent->release();
		}
		constexpr void acquire() const noexcept
		{
			if (record) [[likely]]
				record->parent->acquire();
		}
		constexpr void init(detail::asset_record_base *ptr) noexcept
		{
			record = ptr;
			acquire();
		}

		/** Pointer to the asset data. */
		detail::asset_record_base *record = nullptr;
	};

	/** @brief Structure used to manage assets & asset packages. */
	class asset_db : detail::asset_collection, public basic_service<asset_db>
	{
	public:
		/** Initializes an asset database using the current directory as the data directory. */
		asset_db() : asset_db(std::filesystem::current_path()) {}
		/** Initializes an asset database using the specified data directory. */
		explicit asset_db(const std::filesystem::path &data_dir) : data_dir_path(std::filesystem::canonical(data_dir))
		{
		}

		/** Returns path to the current data directory. */
		[[nodiscard]] constexpr const std::filesystem::path &data_dir() const noexcept { return data_dir_path; }
		/** Sets data directory path. */
		void data_dir(std::filesystem::path new_path) { data_dir_path = std::move(new_path); }

		/** Returns a vector containing all currently loaded assets. */
		[[nodiscard]] std::vector<asset_handle> assets() const
		{
			std::vector<asset_handle> result;
			result.reserve(asset_map.size());
			for (auto &pkg : asset_map) result.push_back(to_asset(pkg.second));
			return result;
		}
		/** Searches for a global asset with a specific id.
		 * @param id Id of the asset.
		 * @note If such asset does not exist, returns an empty asset handle. */
		[[nodiscard]] asset_handle get_asset(std::string_view id) const
		{
			auto iter = asset_map.find(id);
			return iter != asset_map.end() ? to_asset(iter->second) : asset_handle{};
		}

		/** Returns a vector containing all currently loaded packages. */
		[[nodiscard]] std::vector<package_handle> packages() const
		{
			std::vector<package_handle> result;
			result.reserve(package_map.size());
			for (auto &pkg : package_map) result.push_back(pkg.second);
			return result;
		}
		/** Searches for a package loaded at the specified path relative to the data directory.
		 * @param path Path of the package relative to the data directory.
		 * @return Handle to the requested package, or an empty handle if such package was not loaded. */
		[[nodiscard]] package_handle get_package(const std::filesystem::path &path) const
		{
			/* Use path relative to the current data directory. */
			auto iter = package_map.find(get_relative_path(path).native());
			return iter != package_map.end() ? iter->second : package_handle{};
		}

		/** Loads a package at the specified path.
		 * @param path Path of the package to load relative to the data directory.
		 * @param overwrite If set to true, will override conflicting global assets.
		 * @return Handle to the loaded package or an empty handle if it was not loaded. */
		SEK_API package_handle load_package(const std::filesystem::path &path, bool overwrite = true);
		/** Checks if the path references a valid package without loading it.
		 * @param path Path of the package to check relative to the data directory.
		 * @return `1` if the package is a valid master package, `-1` if the package is a valid fragment,
		 * `0` if the path is not a valid package. */
		[[nodiscard]] SEK_API int check_package(const std::filesystem::path &path) const;

	protected:
		/* Expose private package & asset access to database children. */
		constexpr static asset_handle to_asset(detail::asset_record_base *asset) { return asset_handle{asset}; }
		constexpr static package_handle to_package(detail::asset_package_base *package)
		{
			return package_handle{package};
		}
		constexpr static detail::asset_record_base *to_asset_ptr(const asset_handle &asset) { return asset.record; }
		constexpr static detail::asset_package_base *to_package_ptr(const package_handle &package)
		{
			return package.package;
		}
		constexpr static detail::asset_package_base *to_package_ptr(const asset_handle &asset)
		{
			return asset.record->parent;
		}

		[[nodiscard]] static std::filesystem::path get_manifest_path(const std::filesystem::path &path)
		{
			return is_directory(path) ? path / ".manifest" : path;
		}
		[[nodiscard]] static SEK_API std::fstream open_package_manifest(const std::filesystem::path &, std::ios::openmode);
		[[nodiscard]] static SEK_API adt::node load_package_manifest(const std::filesystem::path &);

		[[nodiscard]] std::filesystem::path get_relative_path(const std::filesystem::path &path) const
		{
			return proximate(path, data_dir_path);
		}

		/** Path to the data directory. */
		std::filesystem::path data_dir_path;
		/** Packages mapped to their canonical filename strings. */
		hmap<std::filesystem::path::string_type, package_handle> package_map;
	};

	extern template struct SEK_API_IMPORT basic_service<asset_db>;
}	 // namespace sek