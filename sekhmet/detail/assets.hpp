//
// Created by switchblade on 2022-04-04.
//

#pragma once

#include <atomic>
#include <filesystem>
#include <fstream>
#include <memory>
#include <utility>

#include "adt/node.hpp"
#include "adt/serialize_impl.hpp"
#include "basic_service.hpp"
#include "hset.hpp"
#include "uuid.hpp"

namespace sek
{
	class asset_handle;
	class package_handle;
	class asset_db;

	namespace detail
	{
		struct asset_package_base;
		struct asset_record_base
		{
			asset_package_base *parent;

			std::string id;
			hset<std::string> tags;
		};

		struct loose_asset_record : asset_record_base
		{
			std::filesystem::path asset_path;
			std::filesystem::path metadata_path;
		};

		void serialize(adt::node &node, const loose_asset_record &record)
		{
			node = adt::table{
				{"id", record.id},
				{"file", record.asset_path.string()},
			};

			if (!record.metadata_path.empty()) node.as_table().emplace("metadata", record.metadata_path.string());
			if (!record.tags.empty()) node.as_table()["tags"].set(record.tags);
		}
		void deserialize(const adt::node &node, loose_asset_record &record)
		{
			if (node.is_table()) [[likely]]
			{
				record.id = node.at("id").as_string();
				record.asset_path = node.at("file").as_string();

				if (node.as_table().contains("tags"))
				{
					record.tags.clear();
					node.at("tags").get(record.tags);
				}
				if (node.as_table().contains("metadata")) record.asset_path = node.at("metadata").as_string();
			}
		}

		static_assert(adt::detail::serializable_type<loose_asset_record>);
		static_assert(adt::detail::deserializable_type<loose_asset_record>);

		struct archive_asset_record : asset_record_base
		{
			std::ptrdiff_t asset_offset = 0;
			std::ptrdiff_t asset_size = 0;
			std::ptrdiff_t metadata_offset = 0;
			std::ptrdiff_t metadata_size = 0;
		};

		void serialize(adt::node &node, const archive_asset_record &record)
		{
			node = adt::sequence{
				record.id,
				record.asset_offset,
				record.asset_size,
				record.metadata_offset,
				record.metadata_size,
			};

			if (!record.tags.empty())
			{
				adt::node tags_node;
				tags_node.set(record.tags);
				node.as_sequence().push_back(std::move(tags_node));
			}
		}
		void deserialize(const adt::node &node, archive_asset_record &record)
		{
			if (node.is_sequence()) [[likely]]
			{
				auto &seq = node.as_sequence();
				if (seq.size() >= 5) [[likely]]
				{
					seq[0].get(record.id);
					seq[1].get(record.asset_offset);
					seq[2].get(record.asset_size);
					seq[3].get(record.metadata_offset);
					seq[4].get(record.metadata_size);
					if (seq.size() > 5 && seq[5].is_sequence()) seq[5].get(record.tags);
				}
			}
		}

		static_assert(adt::detail::serializable_type<archive_asset_record>);
		static_assert(adt::detail::deserializable_type<archive_asset_record>);

		struct asset_collection
		{
			mutable std::mutex mtx;
			hmap<std::string_view, asset_record_base *> asset_map;
		};

		struct master_asset_package;

		struct asset_package_base
		{
			enum flags_t : int
			{
				ARCHIVE_PACKAGE = 1,
				READ_ONLY_PACKAGE = ARCHIVE_PACKAGE,
				MASTER_PACKAGE = 2,
			};

			asset_package_base(std::filesystem::path path, flags_t flags) : bytes(), path(std::move(path)), flags(flags)
			{
				if (is_archive())
					std::construct_at(&archive_records);
				else
					std::construct_at(&loose_records);
			}
			virtual ~asset_package_base()
			{
				if (is_archive())
					std::destroy_at(&archive_records);
				else
					std::destroy_at(&loose_records);
			}

			[[nodiscard]] constexpr bool is_read_only() const noexcept
			{
				return (flags & READ_ONLY_PACKAGE) == READ_ONLY_PACKAGE;
			}
			[[nodiscard]] constexpr bool is_archive() const noexcept { return flags & ARCHIVE_PACKAGE; }
			[[nodiscard]] constexpr bool is_master() const noexcept { return flags & MASTER_PACKAGE; }

			[[nodiscard]] constexpr master_asset_package *get_master() noexcept;

			virtual void acquire() noexcept;
			virtual void release();

			union
			{
				std::atomic<std::size_t> ref_count;
				master_asset_package *master; /* Non-master packages do not have a reference counter. */

				std::byte bytes[sizeof(ref_count) > sizeof(master) ? sizeof(ref_count) : sizeof(master)] = {};
			};

			std::filesystem::path path;
			flags_t flags;

			union
			{
				std::vector<archive_asset_record> archive_records;
				std::vector<loose_asset_record> loose_records;
			};
		};

		struct master_asset_package final : asset_package_base, asset_collection
		{
			master_asset_package(std::filesystem::path path, flags_t flags) : asset_package_base(std::move(path), flags)
			{
			}
			~master_asset_package() override = default;

			void acquire() noexcept final { ref_count.fetch_add(1); }
			void release() final
			{
				if (ref_count.fetch_sub(1) == 1) [[unlikely]]
					delete this;
			}

			std::vector<std::unique_ptr<asset_package_base>> fragments;
		};

		void asset_package_base::acquire() noexcept
		{
			if (!is_master()) [[unlikely]]
				master->acquire();
		}
		void asset_package_base::release()
		{
			if (!is_master()) [[unlikely]]
				master->release();
		}
		constexpr master_asset_package *asset_package_base::get_master() noexcept
		{
			return is_master() ? static_cast<master_asset_package *>(this) : master->get_master();
		}
	}	 // namespace detail

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
	class asset_db : public basic_service<asset_db>, detail::asset_collection
	{
		using package_map_t = hmap<std::string_view, detail::master_asset_package *>;

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

		/** Loads a package at the specified path.
		 * @param path Path of the package to load relative to the data directory.
		 * @param overwrite If set to true, will replace conflicting global assets.
		 * @return Handle to the loaded package or an empty handle if it was not loaded.
		 * @note If such package is already loaded, will return the loaded package. */
		SEK_API package_handle load_package(const std::filesystem::path &path, bool overwrite = true);
		/** Checks if the path references a valid package without loading it.
		 * @param path Path of the package to check relative to the data directory.
		 * @return `1` if the package is a valid master, `-1` if the package is a valid fragment,
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

		void insert_package(std::string_view path, detail::master_asset_package *ptr)
		{
			ptr->acquire();
			package_map.emplace(path, ptr);
		}
		void erase_package(package_map_t::const_iterator where)
		{
			auto temp = where->second;
			package_map.erase(where);
			temp->release();
		}

		std::filesystem::path data_dir_path;
		package_map_t package_map;
	};

	extern template struct SEK_API_IMPORT basic_service<asset_db>;
}	 // namespace sek