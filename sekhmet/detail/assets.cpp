//
// Created by switchblade on 2022-04-04.
//

#define _CRT_SECURE_NO_WARNINGS

#include "assets.hpp"

#include <cstring>
#include <fstream>
#include <memory>

#ifdef SEK_OS_WIN
#define MANIFEST_FILE_NAME L".manifest"
#else
#define MANIFEST_FILE_NAME ".manifest"
#endif

#define RECORD_ERROR_MSG "Invalid asset record"

namespace sek
{
	template<>
	SEK_API_EXPORT std::atomic<asset_repository *> &basic_service<asset_repository>::global_ptr() noexcept
	{
		static std::atomic<asset_repository *> value;
		return value;
	}
	SEK_API_EXPORT std::shared_mutex &asset_repository::global_mtx() noexcept
	{
		static std::shared_mutex value;
		return value;
	}

	SEK_API_EXPORT asset asset_repository::find(std::string_view id) const noexcept
	{
		auto asset_iter = assets.find(id);
		if (asset_iter == assets.end()) [[unlikely]]
			return asset{};
		return asset{asset_iter->second};
	}

	SEK_API_EXPORT asset_repository &asset_repository::merge(asset_repository &&other)
	{
		/* Transfer & clear other's assets. */
		assets.reserve(assets.size() + other.assets.size());
		for (auto item = other.assets.begin(), end = other.assets.end(); item != end; ++item)
			assets.insert(other.assets.extract(item));
		other.assets.clear();

		/* Transfer & clear other's packages. */
		packages.reserve(packages.size() + other.packages.size());
		for (auto item = other.packages.begin(), end = other.packages.end(); item != end; ++item)
			packages.insert(other.packages.extract(item));
		other.packages.clear();

		return *this;
	}
}	 // namespace sek