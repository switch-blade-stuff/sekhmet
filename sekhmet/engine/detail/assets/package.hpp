/*
 * Created by switchblade on 10/08/22
 */

#pragma once

#include "handle.hpp"

namespace sek::engine
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
		[[nodiscard]] static SEK_API asset_package load(const std::filesystem::path &path);
		/** Load all packages in the specified directory.
		 * @throw asset_error If the path is not a valid directory. */
		[[nodiscard]] static SEK_API std::vector<asset_package> load_all(const std::filesystem::path &path);

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

		/** Returns URI location of the asset package. */
		[[nodiscard]] constexpr const uri &location() const noexcept { return m_ptr->location(); }

		/** Checks if the asset package is empty (does not contain any assets). */
		[[nodiscard]] constexpr bool empty() const noexcept { return m_ptr->empty(); }
		/** Returns the number of assets contained within the package. */
		[[nodiscard]] constexpr auto size() const noexcept { return m_ptr->size(); }

		/** Returns iterator to the first asset of the package. */
		[[nodiscard]] constexpr const_iterator begin() const noexcept { return m_ptr->begin(); }
		/** @copydoc begin */
		[[nodiscard]] constexpr const_iterator cbegin() const noexcept { return begin(); }
		/** Returns iterator one past the last asset of the package. */
		[[nodiscard]] constexpr const_iterator end() const noexcept { return m_ptr->end(); }
		/** @copydoc end */
		[[nodiscard]] constexpr const_iterator cend() const noexcept { return end(); }
		/** Returns reverse iterator to the last asset of the package. */
		[[nodiscard]] constexpr const_reverse_iterator rbegin() const noexcept { return m_ptr->rbegin(); }
		/** Returns reverse iterator to the first asset of the package. */
		[[nodiscard]] constexpr const_reverse_iterator rend() const noexcept { return m_ptr->rend(); }

		/** Returns iterator to the asset with a given id. */
		[[nodiscard]] constexpr const_iterator find(uuid id) const { return m_ptr->find(id); }
		/** Returns iterator to the asset with a given name. */
		[[nodiscard]] constexpr const_iterator find(std::string_view name) const { return m_ptr->find(name); }
		/** Returns a vector of all assets with the specified name. */
		[[nodiscard]] std::vector<reference> find_all(std::string_view name) const { return m_ptr->find_all(name); }
		/** Checks if the package contains an asset with a given id. */
		[[nodiscard]] constexpr bool constins(uuid id) const { return find(id) != end(); }
		/** Checks if the package contains an asset with a given name. */
		[[nodiscard]] constexpr bool constins(std::string_view name) const { return find(name) != end(); }

		/** Returns iterator to the first asset that matches the predicate. */
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
}	 // namespace sek::engine