//
// Created by switchblade on 26/05/22.
//

#include "asset_stream.hpp"

#include "zstd.h"

namespace sek
{
	namespace detail
	{
		struct zstd_compress_ctx
		{
			zstd_compress_ctx() : ctx(ZSTD_createCCtx())
			{
				if (ctx == nullptr) [[unlikely]]
					throw std::bad_alloc();
			}
			~zstd_compress_ctx() { ZSTD_freeCCtx(ctx); }

			[[nodiscard]] constexpr ZSTD_CCtx *operator->() const noexcept { return ctx; }
			[[nodiscard]] constexpr ZSTD_CCtx &operator*() const noexcept { return *ctx; }
			[[nodiscard]] constexpr operator ZSTD_CCtx *() const noexcept { return ctx; }

			ZSTD_CCtx *ctx;
		};
		struct zstd_decompress_ctx
		{
			zstd_decompress_ctx() : ctx(ZSTD_createDStream())
			{
				if (ctx == nullptr) [[unlikely]]
					throw std::bad_alloc();
			}
			~zstd_decompress_ctx() { ZSTD_freeDStream(ctx); }

			[[nodiscard]] constexpr ZSTD_DStream *operator->() const noexcept { return ctx; }
			[[nodiscard]] constexpr ZSTD_DStream &operator*() const noexcept { return *ctx; }
			[[nodiscard]] constexpr operator ZSTD_DStream *() const noexcept { return ctx; }

			ZSTD_DStream *ctx;
		};

		/* Zstd context is thread-local and is created on request. */
		[[maybe_unused]] static zstd_compress_ctx &compress_ctx()
		{
			thread_local zstd_compress_ctx ctx;
			return ctx;
		}
		[[maybe_unused]] static zstd_decompress_ctx &decompress_ctx()
		{
			thread_local zstd_decompress_ctx ctx;
			return ctx;
		}

		FILE *open_asset_file(const std::filesystem::path &path, std::ios::openmode mode) noexcept
		{
			/* Init mode string. */
			std::array<typename std::filesystem::path::value_type, 4> mode_chars;
			/* +-----------------------------+
			 * | in  out  trunc  app         |
			 * +-----------------------------+
			 * |      +                wb    |
			 * |      +           +    ab    |
			 * |                  +    ab    |
			 * |      +     +          wb    |
			 * |  +                    rb    |
			 * |  +   +                r+b   |
			 * |  +   +     +          w+b   |
			 * |  +   +           +    a+b   |
			 * |  +               +    a+b   |
			 * +-----------------------------+
			 *
			 * Binary flag is always used, since assets may need to be decompressed
			 * (and otherwise, assets are treated as binary files). */
			switch (static_cast<int>(mode & (std::ios::in | std::ios::out | std::ios::trunc | std::ios::app)))
			{
				case std::ios::out:
				case std::ios::out | std::ios::trunc:
				{
					mode_chars = {'w', 'b', '\0', '\0'};
					break;
				}
				case std::ios::app:
				case std::ios::out | std::ios::app:
				{
					mode_chars = {'a', 'b', '\0', '\0'};
					break;
				}
				case std::ios::in:
				{
					mode_chars = {'r', 'b', '\0', '\0'};
					break;
				}
				case std::ios::in | std::ios::out:
				{
					mode_chars = {'r', '+', 'b', '\0'};
					break;
				}
				case std::ios::in | std::ios::out | std::ios::trunc:
				{
					mode_chars = {'w', '+', 'b', '\0'};
					break;
				}
				case std::ios::in | std::ios::app:
				case std::ios::in | std::ios::out | std::ios::app:
				{
					mode_chars = {'a', '+', 'b', '\0'};
					break;
				}
				default: return nullptr; /* Other modes are invalid. */
			}

				/* Open the file using fopen/fopen64/_wfopen */
#if defined(_POSIX_C_SOURCE) && _FILE_OFFSET_BITS < 64
			return fopen64(path.c_str(), mode_chars.data());
#else
#if defined(SEK_OS_WIN)
			if constexpr (std::same_as<wchar_t, typename std::filesystem::path::value_type>)
				return _wfopen(path.c_str(), mode_chars.data());
			else
#endif
				return fopen(path.c_str(), mode_chars.data());
#endif
		}
	}	 // namespace detail
}	 // namespace sek