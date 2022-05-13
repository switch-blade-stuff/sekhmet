//
// Created by switchblade on 2022-03-13.
//

#include "uuid.hpp"

#include <chrono>
#include <random>

#include "sekhmet/math/detail/sysrandom.hpp"
#include "sekhmet/math/detail/xoroshiro.hpp"

namespace sek
{
	struct sysrandom_seq
	{
		typedef std::uint64_t result_type;

		template<typename Iter>
		constexpr void generate(Iter out, Iter) noexcept
		{
			result_type seeds[4] = {0};
			if (math::sys_random(&seeds, sizeof(seeds)) != sizeof(seeds)) [[unlikely]]
			{
				/* If sys_random failed, try to generate using rand. */
				for (auto i = SEK_ARRAY_SIZE(seeds); i > 0;) seeds[--i] = static_cast<result_type>(rand());
			}
			std::copy_n(std::begin(seeds), 4, out);
		}
	};

	static auto uuid_rng = math::xoroshiro<std::uint64_t, 256>{sysrandom_seq{}};

	SEK_API_EXPORT void uuid::version4_t::operator()(uuid &id) const noexcept
	{
		/* Fill with random bits. */
		for (std::size_t i = 0; i < SEK_ARRAY_SIZE(id.bytes) / sizeof(std::uint64_t); ++i)
			reinterpret_cast<std::uint64_t *>(id.bytes)[i] = uuid_rng.next();

		/* Apply version & variant. */
		constexpr std::uint8_t version_mask = 0b0000'1111;
		constexpr std::uint8_t version_bits = 0b0100'0000;
		constexpr std::uint8_t variant_mask = 0b0011'1111;
		constexpr std::uint8_t variant_bits = 0b1000'0000;

		id.bytes[6] = static_cast<std::byte>((static_cast<std::uint8_t>(id.bytes[6]) & version_mask) | version_bits);
		id.bytes[8] = static_cast<std::byte>((static_cast<std::uint8_t>(id.bytes[8]) & variant_mask) | variant_bits);
	}
}	 // namespace sek