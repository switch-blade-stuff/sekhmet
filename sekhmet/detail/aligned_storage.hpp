//
// Created by switchblade on 2021-12-03.
//

#pragma once

#include <cstddef>

namespace sek::detail
{
	template<std::size_t Size, std::size_t Align>
	class alignas(Align) aligned_storage
	{
	public:
		template<typename T>
		requires(sizeof(T) <= Size) [[nodiscard]] constexpr T *get() noexcept
		{
			return reinterpret_cast<T *>(data_bytes);
		}
		template<typename T>
		requires(sizeof(T) <= Size) [[nodiscard]] constexpr const T *get() const noexcept
		{
			return reinterpret_cast<const T *>(data_bytes);
		}

		[[nodiscard]] constexpr std::byte *data() noexcept { return data_bytes; }
		[[nodiscard]] constexpr const std::byte *data() const noexcept { return data_bytes; }

	private:
		std::byte data_bytes[Size];
	};

	template<typename T>
	using type_storage = aligned_storage<sizeof(T), alignof(T)>;
}	 // namespace sek::detail