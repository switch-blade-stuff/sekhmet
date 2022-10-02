//
// Created by switch_blade on 2022-10-01.
//

#pragma once

namespace sek
{
	/** @brief Enum containing texture formats. */
	enum class texture_format : int
	{
		/** Four-channel (RGBA) texture. 4-bit uint per channel. */
		R4G4B4A4,
		/** Four-channel (ARGB) texture. 4-bit uint per channel. */
		A4R4G4B4,
		/** @copydoc R4G4B4A4. */
		RGBA4U = R4G4B4A4,
		/** @copydoc A4R4G4B4. */
		ARGB4U = A4R4G4B4,

		/* Single-channel (A) texture. 8-bit uint per channel. */
		A8U,
		/* Single-channel (R) texture. 8-bit uint per channel. */
		R8U,
		/* Two-channel (RG) texture. 8-bit uint per channel. */
		R8G8U,
		/* Three-channel (RGB) texture. 8-bit uint per channel. */
		R8G8B8U,
		/* Four-channel (RGBA) texture. 8-bit uint per channel. */
		R8G8B8A8U,
		/* Four-channel (ARGB) texture. 8-bit uint per channel. */
		A8R8G8B8U,
		/* Four-channel (ABGR) texture. 8-bit uint per channel. */
		A8B8G8R8U,
		/* Four-channel (BGRA) texture. 8-bit uint per channel. */
		B8G8R8A8U,
		/** @copydoc R8G8B8U. */
		RGB8U = R8G8B8U,
		/** @copydoc R8G8B8A8U. */
		RGBA8U = R8G8B8A8U,
		/** @copydoc A8B8G8R8U. */
		ARGB8U = A8R8G8B8U,
		/** @copydoc A8B8G8R8U. */
		ABGR8U = A8B8G8R8U,
		/** @copydoc B8G8R8A8U. */
		BGRA8U = B8G8R8A8U,

		/* Single-channel (R) texture. 16-bit uint per channel. */
		R16U,

		/* Single-channel (R) texture. 32-bit uint per channel. */
		R32U,
		/* Two-channel (RG) texture format. 32-bit uint per channel. */
		R32G32U,
		/* Three-channel (RGB) texture format. 32-bit uint per channel. */
		R32G32B32U,
		/* Four-channel (RGBA) texture format. 32-bit uint per channel. */
		R32G32B32A32U,
		/** @copydoc R32G32B32U. */
		RGB32U = R32G32B32U,
		/** @copydoc R32G32B32A32U. */
		RGBA32U = R32G32B32A32U,

		/* Single-channel (R) texture. 16-bit (half-precision) float per channel. */
		R16F,
		/* Two-channel (RG) texture. 16-bit (half-precision) float per channel. */
		R16G16F,
		/* Four-channel (RGBA) texture. 16-bit (half-precision) float per channel. */
		R16G16B16A16F,
		/* Four-channel (ABGR) texture. 16-bit (half-precision) float per channel. */
		A16B16G16R16F,
		/** @copydoc R16G16B16A16F. */
		RGBA16F = R16G16B16A16F,
		/** @copydoc A16B16G16R16F. */
		ABGR16F = A16B16G16R16F,

		/* Single-channel (R) texture. 32-bit (single-precision) float per channel. */
		R32F,
		/* Two-channel (RG) texture. 32-bit (single-precision) float per channel. */
		R32G32F,
		/* Four-channel (RGBA) texture. 32-bit (single-precision) float per channel. */
		R32G32B32A32F,
		/* Four-channel (ABGR) texture. 32-bit (single-precision) float per channel. */
		A32B32G32R32F,

		/* YUV-space texture. */
		UYVY,

		/* Texture compressed with DXT1. */
		DXT1,
		/* Texture compressed with DXT3. */
		DXT3,
		/* Texture compressed with DXT5. */
		DXT5,

		/* Single-channel (R) texture compressed with BC4. */
		BC4,
		/* Two-channel (RG) texture compressed with BC5. */
		BC5,
		/* 3-channel HDR texture compressed with BC6H. */
		BC6H,
		/* 3- or 4-channel texture compressed with BC7. */
		BC7,

		PF_ETC1,
		PF_ETC2_RGB,
		PF_ETC2_RGBA,

		/** Platform-specific default format used for regular LDR textures. */
		DEFAULT_LDR,
		/** Platform-specific default format used for regular HDR textures. */
		DEFAULT_HDR,

		/** Platform-specific default format used for normal textures. */
		DEFAULT_NORMAL,

		/** Platform-specific default format used for depth textures. */
		DEFAULT_DEPTH,
		/** Platform-specific default format used for stencil textures. */
		DEFAULT_STENCIL,
		/** Platform-specific default format used for depth/stencil textures. */
		DEFAULT_DEPTH_STENCIL,

		/** Platform-specific default format used for video frame buffers. */
		DEFAULT_VIDEO,
	};
}	 // namespace sek