/*
 * Created by switchblade on 2021-12-03
 */

#pragma once

#include <bit>
#include <cstddef>

namespace sek
{
	template<std::size_t Size, std::size_t Align>
	class alignas(Align) aligned_storage
	{
	public:
		template<typename T>
		requires(sizeof(T) <= Size) [[nodiscard]] constexpr T *get() noexcept { return std::bit_cast<T *>(data()); }
		template<typename T>
		requires(sizeof(T) <= Size) [[nodiscard]] constexpr const T *get() const noexcept
		{
			return std::bit_cast<const T *>(data());
		}

		[[nodiscard]] constexpr void *data() noexcept { return static_cast<void *>(m_bytes); }
		[[nodiscard]] constexpr const void *data() const noexcept { return static_cast<const void *>(m_bytes); }

	private:
		std::byte m_bytes[Size];
	};

	template<typename T>
	using type_storage = aligned_storage<sizeof(T), alignof(T)>;
}	 // namespace sek