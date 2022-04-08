//
// Created by switchblade on 2022-04-04.
//

#pragma once

#include <atomic>
#include <filesystem>
#include <vector>

#include "adt/node.hpp"
#include "adt/serialize_impl.hpp"
#include "basic_service.hpp"
#include "filemap.hpp"
#include "hset.hpp"
#include <shared_mutex>

#define SEK_ARCHIVE_PACKAGE_SIGNATURE "\3SEKPAK"

namespace sek
{
	class asset_repository;
	class asset_package;
	class asset;

	namespace detail
	{
		struct package_fragment;
		struct master_package;

		struct asset_record_base
		{
			[[nodiscard]] virtual filemap map_file(filemap::openmode mode) const = 0;

			package_fragment *parent;

			std::string id;
			hset<std::string> tags;
		};

		struct loose_asset_record final : asset_record_base
		{
			[[nodiscard]] filemap map_file(filemap::openmode mode) const final;

			std::filesystem::path file_path;
			std::filesystem::path metadata_path;
		};

		SEK_API void serialize(adt::node &, const loose_asset_record &);
		SEK_API void deserialize(const adt::node &, loose_asset_record &);

		struct archive_asset_record final : asset_record_base
		{
			[[nodiscard]] filemap map_file(filemap::openmode mode) const final;

			std::ptrdiff_t file_offset;
			std::size_t file_size;
			std::ptrdiff_t metadata_offset;
			std::size_t metadata_size;
		};

		SEK_API void serialize(adt::node &, const archive_asset_record &);
		SEK_API void deserialize(const adt::node &, archive_asset_record &);

		struct package_fragment
		{
			enum flags_t : int
			{
				LOOSE_PACKAGE = 1,
			};

			package_fragment() = delete;
			package_fragment(const package_fragment &) = delete;

			package_fragment(package_fragment &&other) noexcept
				: padding(other.padding), path(std::move(other.path)), flags(other.flags)
			{
				if (is_loose())
					std::construct_at(&loose_assets, std::move(other.loose_assets));
				else
					std::construct_at(&archive_assets, std::move(other.archive_assets));
			}
			explicit package_fragment(std::filesystem::path &&path, flags_t flags) : path(std::move(path)), flags(flags)
			{
				init();
			}
			explicit package_fragment(master_package *master, std::filesystem::path &&path, flags_t flags)
				: master(master), path(std::move(path)), flags(flags)
			{
				init();
			}
			virtual ~package_fragment()
			{
				if (is_loose())
					std::destroy_at(&loose_assets);
				else
					std::destroy_at(&archive_assets);
			}

			virtual void acquire();
			virtual void release();
			virtual master_package *get_master() { return master; }

			[[nodiscard]] constexpr bool is_loose() const noexcept { return flags & LOOSE_PACKAGE; }

			void init()
			{
				if (is_loose())
					std::construct_at(&loose_assets);
				else
					std::construct_at(&archive_assets);
			}

			union
			{
				std::intptr_t padding = 0;

				std::atomic<std::size_t> ref_count;
				master_package *master;
			};

			std::filesystem::path path;
			flags_t flags;

			union
			{
				std::vector<archive_asset_record> archive_assets;
				std::vector<loose_asset_record> loose_assets;
			};
		};

		filemap loose_asset_record::map_file(filemap::openmode mode) const
		{
			return filemap{parent->path / file_path, 0, 0, mode};
		}
		filemap archive_asset_record::map_file(filemap::openmode mode) const
		{
			/* For archives `filemap::out` is not allowed. */
			mode |= mode & filemap::out ? filemap::copy : 0;
			return filemap{parent->path, file_offset, file_size, mode};
		}

		struct master_package final : package_fragment
		{
			explicit master_package(std::filesystem::path &&path, flags_t flags)
				: package_fragment(std::move(path), flags)
			{
			}
			~master_package() final = default;

			void acquire() final { ref_count += 1; }
			void release() final
			{
				if (ref_count.fetch_sub(1) == 1) [[unlikely]]
					delete this;
			}
			master_package *get_master() final { return this; }

			package_fragment &add_fragment(std::filesystem::path &&path, flags_t flags)
			{
				return fragments.emplace_back(this, std::move(path), flags);
			}

			std::vector<package_fragment> fragments;
		};

		void package_fragment::acquire() { master->acquire(); }
		void package_fragment::release() { master->release(); }

		SEK_API void serialize(adt::node &, const package_fragment &);
		SEK_API void deserialize(const adt::node &, package_fragment &);
		SEK_API void serialize(adt::node &, const master_package &);
		SEK_API void deserialize(const adt::node &, master_package &);
		SEK_API master_package *load_package(std::filesystem::path &&);
	}	 // namespace detail

	/** @brief Structure used to manage assets & asset packages. */
	class asset_repository
	{
	public:
		/** Returns pointer to the global asset repository.
		 * @note Global asset repository operations must be synchronized using the `global_mtx` shared mutex. */
		[[nodiscard]] static asset_repository *global() noexcept { return basic_service<asset_repository>::instance(); }
		/** Sets the global asset repository.
		 * @param ptr Pointer to the new global repository.
		 * @return Value of the global repository pointer before the operation. */
		static asset_repository *global(asset_repository *ptr)
		{
			return basic_service<asset_repository>::instance(ptr);
		}

		/** Returns reference to the global repository mutex.
		 * This mutex should be used to synchronize global repository operations. */
		[[nodiscard]] static SEK_API std::shared_mutex &global_mtx() noexcept;

	protected:
		using path_string_view = std::basic_string_view<typename std::filesystem::path::value_type>;
		using packages_map_t = hmap<path_string_view, detail::master_package *>;
		using assets_map_t = hmap<std::string_view, detail::asset_record_base *>;

	public:
		/** Merges other asset repository with this one.
		 * @param other Repository to merge with this.
		 * @return Reference to this repository. */
		constexpr asset_repository &merge(asset_repository &&other)
		{
			assets.reserve(assets.size() + other.assets.size());
			for (auto item = other.assets.begin(), end = other.assets.end(); item != end; ++item)
				assets.insert(other.assets.extract(item));
			other.assets.clear();

			packages.reserve(packages.size() + other.packages.size());
			for (auto item = other.packages.begin(), end = other.packages.end(); item != end; ++item)
				packages.insert(other.packages.extract(item));
			other.packages.clear();

			return *this;
		}

	protected:
		constexpr void add_asset_impl(detail::asset_record_base *record) { assets.emplace(record->id, record); }

		constexpr void add_fragment_assets(detail::package_fragment *pkg)
		{
			auto add_asset = [&](detail::asset_record_base &r) { add_asset_impl(&r); };

			if (pkg->is_loose())
				std::for_each(pkg->loose_assets.begin(), pkg->loose_assets.end(), add_asset);
			else
				std::for_each(pkg->archive_assets.begin(), pkg->archive_assets.end(), add_asset);
		}
		constexpr void remove_fragment_assets(detail::package_fragment *pkg)
		{
			auto remove_asset = [&](detail::asset_record_base &r) { assets.erase(r.id); };

			if (pkg->is_loose())
				std::for_each(pkg->loose_assets.begin(), pkg->loose_assets.end(), remove_asset);
			else
				std::for_each(pkg->archive_assets.begin(), pkg->archive_assets.end(), remove_asset);
		}

		constexpr void add_package_impl(detail::master_package *pkg)
		{
			pkg->acquire();
			packages.emplace(pkg->path.native(), pkg);
		}
		constexpr void remove_package_impl(typename packages_map_t::const_iterator where)
		{
			auto pkg = where->second;
			packages.erase(where);
			pkg->release();
		}

		packages_map_t packages;
		assets_map_t assets;
	};

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
	//		void add_package_impl(std::string_view path, detail::master_asset_package *ptr)
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
