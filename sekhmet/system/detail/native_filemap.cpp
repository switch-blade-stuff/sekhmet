/*
 * Created by switchblade on 12/09/22
 */

#include "../native_filemap.hpp"

#include <utility>

#if defined(SEK_OS_UNIX)
#include "unix/native_filemap.cpp"	  // NOLINT
#endif

namespace sek
{
	void native_filemap::map(const sek::native_file &file, std::uint64_t off, std::uint64_t n, mapmode mode)
	{
		if (const auto result = map(std::nothrow, file, off, n, mode); !result.has_value()) [[unlikely]]
			throw std::system_error(result.error());
	}
	void native_filemap::unmap()
	{
		if (const auto result = unmap(std::nothrow); !result.has_value()) [[unlikely]]
			throw std::system_error(result.error());
	}
}	 // namespace sek