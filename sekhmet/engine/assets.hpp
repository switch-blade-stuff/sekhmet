/*
 * Created by switchblade on 2022-04-04
 */

#pragma once

#include <atomic>
#include <filesystem>
#include <memory>
#include <utility>

#include "../access_guard.hpp"
#include "../dense_map.hpp"
#include "../dense_set.hpp"
#include "../detail/basic_pool.hpp"
#include "../event.hpp"
#include "../expected.hpp"
#include "../intern.hpp"
#include "../service.hpp"
#include "../system/native_file.hpp"
#include "../uuid.hpp"
#include <shared_mutex>

namespace sek
{
	/** @brief Reference-counted handle used to reference an asset package. */
	class asset_package
	{
		friend class asset_handle;
		friend class asset_database;

		using table_t = detail::asset_table;

	public:
		typedef typename table_t::value_type value_type;
		typedef typename table_t::iterator iterator;
		typedef typename table_t::const_iterator const_iterator;
		typedef typename table_t::reverse_iterator reverse_iterator;
		typedef typename table_t::const_reverse_iterator const_reverse_iterator;
		typedef typename table_t::pointer pointer;
		typedef typename table_t::const_pointer const_pointer;
		typedef typename table_t::reference reference;
		typedef typename table_t::const_reference const_reference;
		typedef typename table_t::size_type size_type;
		typedef typename table_t::difference_type difference_type;

		/** Loads a package at the specified path.
		 * @throw asset_error If the path does not contain a valid package or
		 * an implementation-defined error occurred during loading of package metadata. */
		[[nodiscard]] static SEK_API asset_package load(const std::filepath &path);
		/** Load all packages in the specified directory.
		 * @throw asset_error If the path is not a valid directory. */
		[[nodiscard]] static SEK_API std::vector<asset_package> load_all(const std::filepath &path);

	private:
		constexpr explicit asset_package(detail::package_info_ptr &&ptr) noexcept : m_ptr(std::move(ptr)) {}
		SEK_API explicit asset_package(detail::package_info *pkg);

	public:
		asset_package() = delete;

		constexpr asset_package(asset_package &&) noexcept = default;
		constexpr asset_package &operator=(asset_package &&other) noexcept
		{
			m_ptr = std::move(other.m_ptr);
			return *this;
		}
		asset_package(const asset_package &) = default;
		asset_package &operator=(const asset_package &) = default;

		/** Returns path of the asset package. */
		[[nodiscard]] constexpr const std::filepath &path() const noexcept { return m_ptr->path; }

		/** Checks if the asset package is empty (does not contain any assets). */
		[[nodiscard]] constexpr bool empty() const noexcept { return m_ptr->empty(); }
		/** Returns the number of assets contained within the package. */
		[[nodiscard]] constexpr auto size() const noexcept { return m_ptr->size(); }

		/** Returns range_type_iterator to the first asset of the package. */
		[[nodiscard]] constexpr const_iterator begin() const noexcept { return m_ptr->begin(); }
		/** @copydoc begin */
		[[nodiscard]] constexpr const_iterator cbegin() const noexcept { return begin(); }
		/** Returns range_type_iterator one past the last asset of the package. */
		[[nodiscard]] constexpr const_iterator end() const noexcept { return m_ptr->end(); }
		/** @copydoc end */
		[[nodiscard]] constexpr const_iterator cend() const noexcept { return end(); }
		/** Returns reverse range_type_iterator to the last asset of the package. */
		[[nodiscard]] constexpr const_reverse_iterator rbegin() const noexcept { return m_ptr->rbegin(); }
		/** Returns reverse range_type_iterator to the first asset of the package. */
		[[nodiscard]] constexpr const_reverse_iterator rend() const noexcept { return m_ptr->rend(); }

		/** Returns range_type_iterator to the asset with a given id. */
		[[nodiscard]] constexpr const_iterator find(uuid id) const { return m_ptr->find(id); }
		/** Returns range_type_iterator to the asset with a given name. */
		[[nodiscard]] constexpr const_iterator find(std::string_view name) const { return m_ptr->find(name); }
		/** Returns a vector of all assets with the specified name. */
		[[nodiscard]] std::vector<reference> find_all(std::string_view name) const { return m_ptr->find_all(name); }
		/** Checks if the package contains an asset with a given id. */
		[[nodiscard]] constexpr bool constins(uuid id) const { return find(id) != end(); }
		/** Checks if the package contains an asset with a given name. */
		[[nodiscard]] constexpr bool constins(std::string_view name) const { return find(name) != end(); }

		/** Returns range_type_iterator to the first asset that matches the predicate. */
		template<typename P>
		[[nodiscard]] constexpr const_iterator match(P &&pred) const
		{
			return m_ptr->match(std::forward<P>(pred));
		}
		/** Returns a vector of all assets that match the predicate. */
		template<typename P>
		[[nodiscard]] std::vector<reference> match_all(P &&pred) const
		{
			return m_ptr->match_all(std::forward<P>(pred));
		}

#ifdef SEK_EDITOR
		/** Returns event proxy for asset removal event. */
		[[nodiscard]] event_proxy<event<void(const asset_handle &)>> on_asset_removed() const
		{
			return m_ptr->asset_removed;
		}
		/** Returns event proxy for asset creation event. */
		[[nodiscard]] event_proxy<event<void(const asset_handle &)>> on_asset_added() const
		{
			return m_ptr->asset_added;
		}
#endif

		constexpr void swap(asset_package &other) noexcept { m_ptr.swap(other.m_ptr); }
		friend constexpr void swap(asset_package &a, asset_package &b) noexcept { a.swap(b); }

		[[nodiscard]] constexpr bool operator==(const asset_package &) const noexcept = default;

	private:
		detail::package_info_ptr m_ptr;
	};

	inline asset_package asset_handle::package() const { return asset_package{m_ptr->parent}; }

	/** @brief Proxy range used to manipulate load order of asset database's packages.
	 *
	 * @note Any modifications to the load order of the packages will trigger an update of the
	 * parent database's asset tables.
	 * @warning Proxy may not outlive parent database. */
	template<bool Mutable>
	class package_proxy;

	/** @brief Service used to manage global database of assets and asset packages. */
	class asset_database : public service<shared_guard<asset_database>>
	{
		template<bool>
		friend class package_proxy;

		friend shared_guard<asset_database>;

	protected:
		using packages_t = std::vector<asset_package>;
		using assets_t = detail::asset_table;

	public:
		typedef typename assets_t::value_type value_type;
		typedef typename assets_t::iterator iterator;
		typedef typename assets_t::const_iterator const_iterator;
		typedef typename assets_t::reverse_iterator reverse_iterator;
		typedef typename assets_t::const_reverse_iterator const_reverse_iterator;
		typedef typename assets_t::pointer pointer;
		typedef typename assets_t::const_pointer const_pointer;
		typedef typename assets_t::reference reference;
		typedef typename assets_t::const_reference const_reference;
		typedef typename assets_t::size_type size_type;
		typedef typename assets_t::difference_type difference_type;

	protected:
		constexpr asset_database() = default;

	public:
		/** Checks if the asset database is empty (does not contain any assets). */
		[[nodiscard]] constexpr bool empty() const noexcept { return m_assets.empty(); }
		/** Returns the number of assets contained within the database. */
		[[nodiscard]] constexpr auto size() const noexcept { return m_assets.size(); }

		/** Returns range_type_iterator to the first asset of the database. */
		[[nodiscard]] constexpr const_iterator begin() const noexcept { return m_assets.begin(); }
		/** @copydoc begin */
		[[nodiscard]] constexpr const_iterator cbegin() const noexcept { return begin(); }
		/** Returns range_type_iterator one past the last asset of the database. */
		[[nodiscard]] constexpr const_iterator end() const noexcept { return m_assets.end(); }
		/** @copydoc end */
		[[nodiscard]] constexpr const_iterator cend() const noexcept { return end(); }
		/** Returns reverse range_type_iterator to the last asset of the database. */
		[[nodiscard]] constexpr const_reverse_iterator rbegin() const noexcept { return m_assets.rbegin(); }
		/** Returns reverse range_type_iterator to the first asset of the database. */
		[[nodiscard]] constexpr const_reverse_iterator rend() const noexcept { return m_assets.rend(); }

		/** Returns range_type_iterator to the asset with a given id. */
		[[nodiscard]] constexpr const_iterator find(uuid id) const { return m_assets.find(id); }
		/** Returns range_type_iterator to the asset with a given name. */
		[[nodiscard]] constexpr const_iterator find(std::string_view name) const { return m_assets.find(name); }
		/** Returns a vector of all assets with the specified name. */
		[[nodiscard]] std::vector<reference> find_all(std::string_view name) const { return m_assets.find_all(name); }
		/** Checks if the database contains an asset with a given id. */
		[[nodiscard]] constexpr bool constins(uuid id) const { return find(id) != end(); }
		/** Checks if the database contains an asset with a given name. */
		[[nodiscard]] constexpr bool constins(std::string_view name) const { return find(name) != end(); }

		/** Returns range_type_iterator to the first asset that matches the predicate. */
		template<typename P>
		[[nodiscard]] constexpr const_iterator match(P &&pred) const
		{
			return m_assets.match(std::forward<P>(pred));
		}
		/** Returns a vector of all assets that match the predicate. */
		template<typename P>
		[[nodiscard]] std::vector<reference> match_all(P &&pred) const
		{
			return m_assets.match_all(std::forward<P>(pred));
		}

		/** Clears the contents of the asset database by removing all assets & packages. */
		SEK_API void clear();

		/** Returns a proxy range used to manipulate the load order of packages. */
		[[nodiscard]] constexpr auto packages() noexcept;
		/** @copydoc packages */
		[[nodiscard]] constexpr auto packages() const noexcept;

	protected:
		SEK_API void restore_asset(typename packages_t::const_iterator, uuid, const detail::asset_info *);
		SEK_API void override_asset(typename packages_t::const_iterator, uuid, detail::asset_info *);

		typename packages_t::const_iterator insert_impl(typename packages_t::iterator);
		SEK_API typename packages_t::const_iterator insert(typename packages_t::const_iterator, const asset_package &);
		SEK_API typename packages_t::const_iterator insert(typename packages_t::const_iterator, asset_package &&);

		typename packages_t::const_iterator erase_impl(typename packages_t::iterator);
		SEK_API typename packages_t::const_iterator erase(typename packages_t::const_iterator,
														  typename packages_t::const_iterator);
		SEK_API typename packages_t::const_iterator erase(typename packages_t::const_iterator);

		SEK_API void swap(typename packages_t::const_iterator, typename packages_t::const_iterator);

#ifdef SEK_ENGINE
		void handle_asset_removed(const asset_handle &);
		void handle_asset_added(const asset_handle &);
#endif

		packages_t m_packages;
		assets_t m_assets;
	};

	template<>
	class package_proxy<false>
	{
		friend class asset_database;

	protected:
		using packages_t = typename asset_database::packages_t;

	public:
		typedef typename packages_t::value_type value_type;
		typedef typename packages_t::const_pointer pointer;
		typedef typename packages_t::const_pointer const_pointer;
		typedef typename packages_t::const_reference reference;
		typedef typename packages_t::const_reference const_reference;
		typedef typename packages_t::const_iterator iterator;
		typedef typename packages_t::const_iterator const_iterator;
		typedef typename packages_t::const_reverse_iterator reverse_iterator;
		typedef typename packages_t::const_reverse_iterator const_reverse_iterator;
		typedef typename packages_t::size_type size_type;
		typedef typename packages_t::difference_type difference_type;

	protected:
		constexpr explicit package_proxy(const asset_database &parent) noexcept : m_parent(&parent) {}

	public:
		package_proxy() = delete;

		constexpr package_proxy(const package_proxy &) noexcept = default;
		constexpr package_proxy(package_proxy &&) noexcept = default;
		constexpr package_proxy &operator=(const package_proxy &) noexcept = default;
		constexpr package_proxy &operator=(package_proxy &&) noexcept = default;

		constexpr package_proxy(const package_proxy<true> &other) noexcept;
		constexpr package_proxy(package_proxy<true> &&other) noexcept;
		constexpr package_proxy &operator=(const package_proxy<true> &other) noexcept;
		constexpr package_proxy &operator=(package_proxy<true> &&other) noexcept;

		/** Returns range_type_iterator to the first package. */
		[[nodiscard]] constexpr const_iterator cbegin() const noexcept { return packages().cbegin(); }
		/** @copydoc cbegin */
		[[nodiscard]] constexpr const_iterator begin() const noexcept { return cbegin(); }
		/** Returns range_type_iterator one past the last package. */
		[[nodiscard]] constexpr const_iterator cend() const noexcept { return packages().cend(); }
		/** @copydoc cend */
		[[nodiscard]] constexpr const_iterator end() const noexcept { return cend(); }
		/** Returns reverse range_type_iterator one past the last package. */
		[[nodiscard]] constexpr const_reverse_iterator crbegin() const noexcept { return packages().crbegin(); }
		/** @copydoc crbegin */
		[[nodiscard]] constexpr const_reverse_iterator rbegin() const noexcept { return crbegin(); }
		/** Returns reverse range_type_iterator to the fist package. */
		[[nodiscard]] constexpr const_reverse_iterator crend() const noexcept { return packages().crend(); }
		/** @copydoc crbegin */
		[[nodiscard]] constexpr const_reverse_iterator rend() const noexcept { return crend(); }

		/** Returns reference to the first package. */
		[[nodiscard]] constexpr const_reference front() const noexcept { return packages().front(); }
		/** Returns reference to the last package. */
		[[nodiscard]] constexpr const_reference back() const noexcept { return packages().back(); }
		/** Returns reference to the package at the specified index. */
		[[nodiscard]] constexpr const_reference at(size_type i) const noexcept { return packages().at(i); }
		/** @copydoc at */
		[[nodiscard]] constexpr const_reference operator[](size_type i) const noexcept { return at(i); }

		/** Returns the current amount of loaded packages. */
		[[nodiscard]] constexpr size_type size() const noexcept { return packages().size(); }
		/** Checks if no packages are loaded. */
		[[nodiscard]] constexpr bool empty() const noexcept { return packages().empty(); }

		constexpr void swap(package_proxy &other) noexcept { std::swap(m_parent, other.m_parent); }
		friend constexpr void swap(package_proxy &a, package_proxy &b) noexcept { a.swap(b); }

	protected:
		const asset_database *m_parent;

	private:
		[[nodiscard]] constexpr const packages_t &packages() const noexcept { return m_parent->m_packages; }
	};
	template<>
	class package_proxy<true> : public package_proxy<false>
	{
		friend class asset_database;

		using base_t = package_proxy<false>;

	public:
		typedef typename base_t::value_type value_type;
		typedef typename base_t::const_pointer pointer;
		typedef typename base_t::const_pointer const_pointer;
		typedef typename base_t::const_reference reference;
		typedef typename base_t::const_reference const_reference;
		typedef typename base_t::const_iterator iterator;
		typedef typename base_t::const_iterator const_iterator;
		typedef typename base_t::const_reverse_iterator reverse_iterator;
		typedef typename base_t::const_reverse_iterator const_reverse_iterator;
		typedef typename base_t::size_type size_type;
		typedef typename base_t::difference_type difference_type;

	protected:
		constexpr explicit package_proxy(asset_database &parent) noexcept : base_t(parent) {}

	public:
		package_proxy() = delete;

		constexpr package_proxy(const package_proxy &) noexcept = default;
		constexpr package_proxy(package_proxy &&) noexcept = default;
		constexpr package_proxy &operator=(const package_proxy &) noexcept = default;
		constexpr package_proxy &operator=(package_proxy &&) noexcept = default;

		/** Removes a package at the specified position from the load order.
		 * @return Iterator to the package after the erased one. */
		const_iterator erase(const_iterator where) { return parent()->erase(where); }
		/** Removes all packages between [first, last) from the load order.
		 * @return Iterator to the package after the erased range. */
		const_iterator erase(const_iterator first, const_iterator last) { return parent()->erase(first, last); }

		/** Inserts a package at the specified position into the load order.
		 * @return Iterator to the inserted package. */
		const_iterator insert(const_iterator where, const asset_package &pkg) { return parent()->insert(where, pkg); }
		/** @copydoc insert */
		const_iterator insert(const_iterator where, asset_package &&pkg)
		{
			return parent()->insert(where, std::forward<asset_package>(pkg));
		}
		/** Inserts a package at the end of the load order. */
		void push_back(const asset_package &pkg) { insert(end(), pkg); }
		/** @copydoc push_back */
		void push_back(asset_package &&pkg) { insert(end(), std::forward<asset_package>(pkg)); }

		/** Swaps load order of packages `a` and `b`.
		 * @return Iterator to the package after the erased range. */
		void swap(const_iterator a, const_iterator b) { return parent()->swap(a, b); }

		constexpr void swap(package_proxy &other) noexcept { base_t::swap(other); }
		friend constexpr void swap(package_proxy &a, package_proxy &b) noexcept { a.swap(b); }

	private:
		[[nodiscard]] constexpr asset_database *parent() const noexcept
		{
			return const_cast<asset_database *>(base_t::m_parent);
		}
		[[nodiscard]] constexpr typename base_t::packages_t &packages() const noexcept { return parent()->m_packages; }
	};

	constexpr package_proxy<false>::package_proxy(const package_proxy<true> &other) noexcept : m_parent(other.m_parent)
	{
	}
	constexpr package_proxy<false>::package_proxy(package_proxy<true> &&other) noexcept : m_parent(other.m_parent) {}
	constexpr package_proxy<false> &package_proxy<false>::operator=(const package_proxy<true> &other) noexcept
	{
		m_parent = other.m_parent;
		return *this;
	}
	constexpr package_proxy<false> &package_proxy<false>::operator=(package_proxy<true> &&other) noexcept
	{
		m_parent = other.m_parent;
		return *this;
	}

	constexpr auto asset_database::packages() noexcept { return package_proxy<true>{*this}; }
	constexpr auto asset_database::packages() const noexcept { return package_proxy<false>{*this}; }

	/* TODO: Refactor implementation to support error_code */
}	 // namespace sek

extern template class SEK_API_IMPORT sek::service<sek::shared_guard<sek::asset_database>>;
