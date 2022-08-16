/*
 * Created by switchblade on 10/08/22
 */

#pragma once

#include "sekhmet/detail/basic_pool.hpp"

#include "info.hpp"

namespace sek::engine::detail
{
	class loose_package final : public package_info
	{
	protected:
		class loose_info : public asset_info
		{
		public:
			loose_info(package_info *p, std::string_view asset_path, std::string_view meta_path) : asset_info(p)
			{
				if (!asset_path.empty())
				{
					m_asset_path = static_cast<char *>(malloc((m_asset_path_size = asset_path.size()) + 1));
					if (m_asset_path == nullptr) [[unlikely]]
						throw std::bad_alloc();

					std::copy_n(asset_path.data(), asset_path.size(), m_asset_path);
				}
				if (!meta_path.empty())
				{
					m_meta_path = static_cast<char *>(malloc((m_meta_path_size = meta_path.size()) + 1));
					if (m_meta_path == nullptr) [[unlikely]]
						throw std::bad_alloc();

					std::copy_n(meta_path.data(), meta_path.size(), m_meta_path);
				}
			}
			~loose_info()
			{
				free(m_asset_path);
				free(m_meta_path);
			}

			[[nodiscard]] constexpr std::string_view asset_path() const noexcept
			{
				return {m_asset_path, m_asset_path_size};
			}
			[[nodiscard]] constexpr std::string_view meta_path() const noexcept
			{
				return {m_meta_path, m_meta_path_size};
			}

		private:
			/* Path to asset's main file. */
			char *m_asset_path = nullptr;
			std::size_t m_asset_path_size = 0;

			/* Path to asset's metadata file. */
			char *m_meta_path = nullptr;
			std::size_t m_meta_path_size = 0;
		};

	public:
		explicit loose_package(const uri &location) : package_info(location) {}
		explicit loose_package(uri &&location) noexcept : package_info(std::move(location)) {}

		~loose_package() final { destroy_all(); }

		[[nodiscard]] asset_info *alloc_info() final { return m_pool.allocate(); }
		void dealloc_info(asset_info *info) final { m_pool.deallocate(static_cast<loose_info *>(info)); }
		void destroy_info(asset_info *info) final { std::destroy_at(static_cast<loose_info *>(info)); }

		expected<asset_source, std::error_code> open_asset(const asset_info *) const noexcept final;
		expected<asset_source, std::error_code> open_metadata(const asset_info *) const noexcept final;

		[[nodiscard]] constexpr bool has_metadata(const asset_info *info) const noexcept final
		{
			return !static_cast<const loose_info *>(info)->meta_path().empty();
		}

	private:
		expected<asset_source, std::error_code> open_at(std::string_view) const noexcept;

		sek::detail::basic_pool<loose_info> m_pool;
	};
}	 // namespace sek::engine::detail