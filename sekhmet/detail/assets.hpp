//
// Created by switchblade on 2022-04-04.
//

#pragma once

#include <atomic>
#include <filesystem>

#include "assert.hpp"
#include "asset_stream.hpp"
#include "dense_map.hpp"
#include "filemap.hpp"
#include "intern.hpp"
#include "service.hpp"
#include "uuid.hpp"
#include <memory_resource>

namespace sek
{
	class asset_repository;
	class asset_package;
	class asset;

	namespace detail
	{
		struct package_base;

		struct loose_asset_info;
		struct archive_asset_info;
		struct asset_info_base
		{
			asset_info_base() noexcept = default;
			asset_info_base(const asset_info_base &) noexcept = default;
			asset_info_base(asset_info_base &&) noexcept = default;

			[[nodiscard]] constexpr loose_asset_info *as_loose() noexcept;
			[[nodiscard]] constexpr archive_asset_info *as_archive() noexcept;

			/* Parent fragment of the asset. */
			package_base *parent = nullptr;
			/* Optional name of the asset. */
			interned_string name;
		};
		struct loose_asset_info : asset_info_base
		{
			loose_asset_info() noexcept = default;
			loose_asset_info(const loose_asset_info &) = default;
			loose_asset_info(loose_asset_info &&) noexcept = default;

			[[nodiscard]] SEK_API std::filesystem::path full_path() const;

			/* Path of the asset file within a loose package. */
			std::filesystem::path file;
		};
		struct archive_asset_info : asset_info_base
		{
			archive_asset_info() noexcept = default;
			archive_asset_info(const archive_asset_info &) = default;
			archive_asset_info(archive_asset_info &&) noexcept = default;

			/* Position & size of the asset within an archive. */
			std::pair<std::ptrdiff_t, std::size_t> slice = {};
		};

		constexpr loose_asset_info *asset_info_base::as_loose() noexcept
		{
			return static_cast<loose_asset_info *>(this);
		}
		constexpr archive_asset_info *asset_info_base::as_archive() noexcept
		{
			return static_cast<archive_asset_info *>(this);
		}

		struct asset_database
		{
			SEK_API void merge(const asset_database &other);

			/* Asset infos are stored by-reference to allow for pool allocation & keep pointers stable.
			 * Multi-key maps are not used since asset names are optional, UUIDs are used as primary keys
			 * and should be preferred instead. */
			dense_map<uuid, asset_info_base *> assets;
			dense_map<std::string_view, uuid> name_table;
		};

		struct master_package;
		struct fragment_package;
		struct package_base
		{
			enum flags_t : int
			{
				NO_FLAGS = 0,
				IS_MASTER = 1,
				IS_ARCHIVE = 2,
			};

			[[nodiscard]] constexpr master_package *as_master() noexcept;
			[[nodiscard]] constexpr fragment_package *as_fragment() noexcept;

			[[nodiscard]] constexpr bool is_archive() const noexcept { return flags & IS_ARCHIVE; }
			[[nodiscard]] constexpr bool is_master() const noexcept { return flags & IS_MASTER; }
			[[nodiscard]] constexpr master_package *master() noexcept;

			SEK_API void acquire();
			SEK_API void release();

			std::filesystem::path path;
			flags_t flags = NO_FLAGS;
		};
		struct fragment_package : package_base
		{
			fragment_package() = delete;
			explicit fragment_package(master_package *master) : master_ptr(master) {}

			master_package *master_ptr;
		};
		struct master_package : package_base
		{
			template<std::derived_from<asset_info_base> T>
			T *alloc_asset_info()
			{
				return static_cast<T *>(info_pool.allocate(sizeof(T), alignof(T)));
			}

			SEK_API void acquire_impl();
			SEK_API void release_impl();

			std::atomic<std::size_t> ref_count;
			std::vector<fragment_package> fragments;
			std::pmr::unsynchronized_pool_resource info_pool = {};
			asset_database database;
		};

		constexpr master_package *package_base::as_master() noexcept { return static_cast<master_package *>(this); }
		constexpr fragment_package *package_base::as_fragment() noexcept
		{
			return static_cast<fragment_package *>(this);
		}

		constexpr master_package *package_base::master() noexcept
		{
			return is_master() ? as_master() : as_fragment()->master_ptr;
		}

		struct asset_handle
		{
			constexpr asset_handle() noexcept = default;

			asset_handle(asset_info_base *info, uuid id) noexcept : info(info), id(id) { acquire(); }
			~asset_handle() { release(); }

			asset_handle(const asset_handle &other) noexcept { copy_from(other); }
			asset_handle &operator=(const asset_handle &other) noexcept
			{
				if (this != &other) [[likely]]
					copy_from(other);
				return *this;
			}

			template<typename C, typename T>
			basic_asset_stream<C, T> to_stream(std::ios::openmode mode) const
			{
				if (parent()->is_archive())
					return basic_asset_stream<C, T>{to_filemap(filemap::in)};
				else
				{
					std::basic_filebuf<C, T> fb;
					if (const auto path = info->as_loose()->full_path(); !std::filesystem::exists(path)) [[unlikely]]
						throw std::runtime_error("Asset file does not exist");
					else if (!fb.open(path, mode)) [[unlikely]]
						throw std::runtime_error("Failed to open asset file");
					return basic_asset_stream<C, T>{std::move(fb)};
				}
			}
			SEK_API filemap to_filemap(filemap::openmode mode) const;

			[[nodiscard]] constexpr bool empty() const noexcept { return info == nullptr; }
			[[nodiscard]] constexpr package_base *parent() const noexcept { return info->parent; }

			[[nodiscard]] constexpr bool operator==(const asset_handle &other) const noexcept
			{
				/* No need to compare UUIDs, asset infos are unique. */
				return info == other.info;
			}

			constexpr void swap(asset_handle &other) noexcept { std::swap(info, other.info); }

			void copy_from(const asset_handle &other) noexcept
			{
				id = other.id;
				if ((info = other.info) != nullptr) [[likely]]
					acquire();
			}
			void acquire() const noexcept { parent()->acquire(); }
			void release() const
			{
				if (!empty()) [[likely]]
					parent()->release();
			}
			void reset()
			{
				release();
				info = nullptr;
			}

			asset_info_base *info = nullptr;
			uuid id;
		};
		struct package_handle
		{
			constexpr package_handle() noexcept = default;

			explicit package_handle(master_package *pkg) noexcept : pkg(pkg) { acquire(); }
			~package_handle() { release(); }

			package_handle(const package_handle &other) noexcept { copy_from(other); }
			package_handle &operator=(const package_handle &other) noexcept
			{
				if (this != &other) [[likely]]
					copy_from(other);
				return *this;
			}

			[[nodiscard]] constexpr bool empty() const noexcept { return pkg == nullptr; }

			[[nodiscard]] constexpr bool operator==(const package_handle &) const noexcept = default;

			constexpr void swap(package_handle &other) noexcept { std::swap(pkg, other.pkg); }

			void copy_from(const package_handle &other) noexcept
			{
				if ((pkg = other.pkg) != nullptr) [[likely]]
					acquire();
			}
			void acquire() const noexcept { pkg->acquire_impl(); }
			void release() const
			{
				if (!empty()) [[likely]]
					pkg->release_impl();
			}
			void reset()
			{
				release();
				pkg = nullptr;
			}

			master_package *pkg = nullptr;
		};
	}	 // namespace detail

	class asset_repository : detail::service<asset_repository>
	{
	protected:
		using service<asset_repository>::instance;

	public:
	protected:
		std::vector<detail::package_handle> packages;
		detail::asset_database database;
	};

	class asset_package
	{
		friend class asset_repository;
		friend class asset;
	};

	class asset
	{
		using handle_t = detail::asset_handle;

		friend class asset_repository;
		friend class asset_package;

	public:
		/** @brief Loads an asset from the global repository.
		 * @param id UUID of the asset. */
		static SEK_API asset load(uuid id);
		/** @copybrief load
		 * @param name Name of the asset. */
		static SEK_API asset load(std::string_view name);

	private:
		asset(detail::asset_info_base *info, uuid id) noexcept : handle(info, id) {}

	public:
		/** Initializes an empty asset. */
		constexpr asset() noexcept = default;

		asset(const asset &) noexcept = default;
		asset &operator=(const asset &) noexcept = default;
		constexpr asset(asset &&other) noexcept { swap(other); }
		constexpr asset &operator=(asset &&other) noexcept
		{
			swap(other);
			return *this;
		}
		~asset() = default;

		/** Resets the asset, making it empty. */
		void reset() { handle.reset(); }

		/** Checks if the asset is empty. */
		[[nodiscard]] constexpr bool empty() const noexcept { return handle.empty(); }
		/** Returns the UUID of the asset. */
		[[nodiscard]] constexpr uuid id() const noexcept { return handle.id; }
		/** Returns the name of the asset.
		 * @note If an asset does not have a name, it will be empty. */
		[[nodiscard]] constexpr const interned_string &name() const noexcept { return handle.info->name; }

		/** Opens an asset stream for this asset.
		 * @param mode Mode in which to open the asset stream.
		 * @note If the asset is part of an archive, mode will be ignored and the mode is equivalent to
		 * `std::ios::in | std::ios::binary`. */
		template<typename C, typename T = std::char_traits<C>>
		[[nodiscard]] basic_asset_stream<C, T> to_stream(std::ios::openmode mode = std::ios::in | std::ios::binary) const
		{
			return handle.template to_stream<C, T>(mode);
		}
		/** Maps asset file into memory.
		 * @param mode Mode in which to map the file.
		 * @note If the asset is part of an archive, `filemap::out` mode will be ignored. */
		[[nodiscard]] filemap to_filemap(filemap::openmode mode = filemap::in) const { return handle.to_filemap(mode); }

		[[nodiscard]] constexpr bool operator==(const asset &) const noexcept = default;

		constexpr void swap(asset &other) noexcept { handle.swap(other.handle); }
		friend constexpr void swap(asset &a, asset &b) noexcept { a.swap(b); }

	private:
		handle_t handle;
	};

	extern template class SEK_API_IMPORT detail::service<asset_repository>;
}	 // namespace sek
