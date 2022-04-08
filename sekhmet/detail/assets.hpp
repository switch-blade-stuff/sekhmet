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

#define SEK_PACKAGE_SIGNATURE "\3SEKPAK"

namespace sek
{
	class asset_stream;

	class asset_repository;
	class asset_package;
	class asset;

	namespace detail
	{
		struct package_fragment;
		struct master_package;

		struct asset_record_base
		{
			explicit asset_record_base(package_fragment *parent) noexcept : parent(parent) {}
			virtual ~asset_record_base() = default;
			[[nodiscard]] virtual filemap map_file(filemap::openmode mode) const = 0;

			virtual void serialize(adt::node &) const = 0;
			virtual void deserialize(adt::node &&) = 0;

			package_fragment *parent = nullptr;

			std::string id;
			hset<std::string> tags;
		};
		struct loose_asset_record final : asset_record_base
		{
			explicit loose_asset_record(package_fragment *parent) noexcept : asset_record_base(parent) {}
			~loose_asset_record() final = default;
			[[nodiscard]] filemap map_file(filemap::openmode mode) const final;

			SEK_API void serialize(adt::node &) const final;
			SEK_API void deserialize(adt::node &&) final;

			std::filesystem::path file_path;
			std::filesystem::path metadata_path;
		};
		struct archive_asset_record final : asset_record_base
		{
			explicit archive_asset_record(package_fragment *parent) noexcept : asset_record_base(parent) {}
			~archive_asset_record() final = default;
			[[nodiscard]] filemap map_file(filemap::openmode mode) const final;

			SEK_API void serialize(adt::node &) const final;
			SEK_API void deserialize(adt::node &&) final;

			std::ptrdiff_t file_offset = 0;
			std::size_t file_size = 0;
			std::ptrdiff_t metadata_offset = 0;
			std::size_t metadata_size = 0;
		};

		struct package_base
		{
			enum flags_t : int
			{
				LOOSE_PACKAGE = 1,
			};

			struct record_handle
			{
				SEK_API explicit record_handle(package_fragment *parent);

				constexpr record_handle(record_handle &&other) noexcept : ptr(std::exchange(other.ptr, nullptr)) {}
				constexpr record_handle &operator=(record_handle &&other) noexcept
				{
					ptr = std::exchange(other.ptr, nullptr);
					return *this;
				}
				constexpr explicit record_handle(asset_record_base *ptr) noexcept : ptr(ptr) {}
				~record_handle() { delete ptr; }

				void serialize(adt::node &node) const { ptr->serialize(node); }
				void deserialize(adt::node &&node) const { ptr->deserialize(std::move(node)); }

				asset_record_base *ptr = nullptr;
			};

			package_base() = delete;
			package_base(const package_base &) = delete;

			package_base(package_base &&) noexcept = default;
			package_base(std::filesystem::path &&path, flags_t flags) : path(std::move(path)), flags(flags) {}

			std::filesystem::path path;
			flags_t flags;

			std::vector<record_handle> assets;
		};
		struct package_fragment : package_base
		{
			package_fragment(package_fragment &&other) noexcept : package_base(std::move(other)), padding(other.padding)
			{
			}
			package_fragment(std::filesystem::path &&path, flags_t flags) : package_base(std::move(path), flags) {}
			package_fragment(master_package *master, std::filesystem::path &&path, flags_t flags)
				: package_base(std::move(path), flags), master(master)
			{
			}
			virtual ~package_fragment() = default;

			virtual void acquire();
			virtual void release();
			virtual master_package *get_master() { return master; }

			SEK_API virtual void serialize(adt::node &) const;
			SEK_API virtual void deserialize(adt::node &&);

			union
			{
				std::intptr_t padding = 0;

				std::atomic<std::size_t> ref_count;
				master_package *master;
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
			master_package(std::filesystem::path &&path, flags_t flags) : package_fragment(std::move(path), flags) {}
			~master_package() final = default;

			void acquire() final { ref_count += 1; }
			void release() final
			{
				if (ref_count.fetch_sub(1) == 1) [[unlikely]]
					delete this;
			}
			master_package *get_master() final { return this; }

			SEK_API void serialize(adt::node &) const final;
			SEK_API void deserialize(adt::node &&) final;

			package_fragment &add_fragment(std::filesystem::path &&path, flags_t flags)
			{
				return fragments.emplace_back(this, std::move(path), flags);
			}

			std::vector<package_fragment> fragments;
		};

		void package_fragment::acquire() { master->acquire(); }
		void package_fragment::release() { master->release(); }

		SEK_API master_package *load_package(std::filesystem::path &&);
	}	 // namespace detail

	/** @brief Structure used to reference an asset. */
	class asset
	{
		friend class asset_repository;

		constexpr explicit asset(detail::asset_record_base *record) noexcept : record(record) {}

	public:
		/** Initializes an empty asset. */
		constexpr asset() noexcept = default;
		~asset() { release(); }

		/** Checks if the asset is empty. */
		[[nodiscard]] constexpr bool empty() const noexcept { return record == nullptr; }
		/** Returns the id of the asset. */
		[[nodiscard]] constexpr const std::string &id() const noexcept { return record->id; }

		/** Creates an `asset_stream` from this asset.
		 * @param mode Mode to use while creating the stream. */
		[[nodiscard]] asset_stream get_stream(std::ios::openmode mode = std::ios::in) const;
		/** Creates a `filemap` from this asset.
		 * @param mode Mode to use while creating the `filemap`. */
		[[nodiscard]] filemap get_filemap(filemap::openmode mode = filemap::in) const { return record->map_file(mode); }

	private:
		void acquire()
		{
			if (record) [[likely]]
				record->parent->acquire();
		}
		void release()
		{
			if (record) [[likely]]
				record->parent->release();
		}

		detail::asset_record_base *record = nullptr;
	};

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
		/** Searches an asset using it's id within this repository.
		 * @param id Id of the asset to look for.
		 * @return The requested asset or an empty asset if it was not found. */
		[[nodiscard]] SEK_API asset find(std::string_view id) const noexcept;

		/** Merges other asset repository with this one.
		 * @param other Repository to merge with this.
		 * @return Reference to this repository. */
		SEK_API asset_repository &merge(asset_repository &&other);

	protected:
		constexpr void add_asset_impl(detail::asset_record_base *record) { assets.emplace(record->id, record); }
		void add_fragment_assets(detail::package_fragment *pkg)
		{
			for (auto &handle : pkg->assets) add_asset_impl(handle.ptr);
		}
		void remove_fragment_assets(detail::package_fragment *pkg)
		{
			for (auto &handle : pkg->assets) assets.erase(handle.ptr->id);
		}
		void add_package_impl(detail::master_package *pkg)
		{
			pkg->acquire();
			packages.emplace(pkg->path.native(), pkg);
		}
		void remove_package_impl(typename packages_map_t::const_iterator where)
		{
			auto pkg = where->second;
			packages.erase(where);
			pkg->release();
		}

		packages_map_t packages;
		assets_map_t assets;
	};

}	 // namespace sek
