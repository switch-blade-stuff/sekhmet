//
// Created by switchblade on 2022-03-14.
//

#pragma once

#include <algorithm>
#include <bit>
#include <limits>

#include "../../detail/define.h"

namespace sek::math
{
	template<typename T, typename I>
	concept is_seed_generator = requires(const I (&state)[4], T &gen)
	{
		typename T::result_type;
		std::same_as<typename T::result_type, I>;
		gen.generate(std::begin(state), std::end(state));
	};

	constexpr static auto mix_seed_xor(auto &seed) noexcept { return std::rotl(seed, 19) ^ seed; }
	constexpr static auto uint64_to_double(auto value) noexcept { return static_cast<double>(value >> 11) * 0x1.0p-53; }
	constexpr static auto uint32_to_float(auto value) noexcept { return static_cast<float>(value >> 8) * 0x1.0p-24f; }

	template<typename, std::size_t>
	struct xoroshiro_base;

	template<>
	struct xoroshiro_base<std::uint64_t, 256>
	{
		typedef std::uint64_t result_type;
		typedef std::uint64_t state_type[4];

		constexpr static state_type initial = {0x4424e023cd1d52, 0x53e25f3254fc82, 0x182982e2f107bb, 0x0ef936c5c27271};
		constexpr static state_type jmp_short = {0x180ec6d33cfd0aba, 0xd5a61266f0c9392c, 0xa9582618e03fc9aa, 0x39abdc4529b1661c};
		constexpr static state_type jmp_long = {0x76e15d3efefdcbbf, 0xc5004e441c522fb3, 0x77710069854ee241, 0x39109bb02acbe635};

		constexpr auto do_next() noexcept
		{
			const auto result = std::rotl(state[1] * 5, 7) * 9;
			const auto t = state[1] << 17;

			state[2] ^= state[0];
			state[3] ^= state[1];
			state[1] ^= state[2];
			state[0] ^= state[3];

			state[2] ^= t;
			state[3] = std::rotl(state[3], 45);

			return result;
		}
		constexpr void do_jump(const result_type (&jmp_arr)[4]) noexcept
		{
			result_type temp[4] = {0};
			for (auto jmp : jmp_arr)
				for (auto b = 0; b < 64; b++)
				{
					if (jmp & static_cast<result_type>(1) << b)
					{
						temp[0] ^= state[0];
						temp[1] ^= state[1];
						temp[2] ^= state[2];
						temp[3] ^= state[3];
					}
					do_next();
				}

			std::copy_n(temp, SEK_ARRAY_SIZE(state), state);
		}

		state_type state;
	};
	template<>
	struct xoroshiro_base<std::uint64_t, 128>
	{
		typedef std::uint64_t result_type;
		typedef std::uint64_t state_type[2];

		constexpr static state_type initial = {0x4424e0232e2f107b, 0x70865936c5c27271};
		constexpr static state_type jmp_short = {0xdf900294d8f554a5, 0x170865df4b3201fc};
		constexpr static state_type jmp_long = {0xd2a98b26625eee7b, 0xdddf9b1090aa7ac1};

		constexpr auto do_next() noexcept
		{
			const auto s0 = state[0];
			const auto result = std::rotl(s0 * 5, 7) * 9;
			auto s1 = state[1];

			s1 ^= s0;
			state[0] = std::rotl(s0, 24) ^ s1 ^ (s1 << 16);
			state[1] = std::rotl(s1, 37);

			return result;
		}
		constexpr void do_jump(const result_type (&jmp_arr)[4]) noexcept
		{
			result_type temp[4] = {0};
			for (auto jmp : jmp_arr)
				for (auto b = 0; b < 64; b++)
				{
					if (jmp & static_cast<result_type>(1) << b)
					{
						temp[0] ^= state[0];
						temp[1] ^= state[1];
					}
					do_next();
				}

			std::copy_n(temp, SEK_ARRAY_SIZE(state), state);
		}

		state_type state;
	};
	template<>
	struct xoroshiro_base<double, 256>
	{
		typedef double result_type;
		typedef std::uint64_t state_type[4];

		constexpr static state_type initial = {0x4424e023cd1d52, 0x53e25f3254fc82, 0x182982e2f107bb, 0x0ef936c5c27271};
		constexpr static state_type jmp_short = {0x180ec6d33cfd0aba, 0xd5a61266f0c9392c, 0xa9582618e03fc9aa, 0x39abdc4529b1661c};
		constexpr static state_type jmp_long = {0x76e15d3efefdcbbf, 0xc5004e441c522fb3, 0x77710069854ee241, 0x39109bb02acbe635};

		constexpr auto do_next() noexcept
		{
			const auto result = state[0] + state[3];
			const auto temp = state[1] << 17;

			state[2] ^= state[0];
			state[3] ^= state[1];
			state[1] ^= state[2];
			state[0] ^= state[3];

			state[2] ^= temp;
			state[3] = std::rotl(state[3], 45);

			return uint64_to_double(result);
		}
		constexpr void do_jump(const state_type &jmp_arr) noexcept
		{
			state_type temp = {0};
			for (auto jmp : jmp_arr)
				for (auto b = 0; b < 64; b++)
				{
					if (jmp & static_cast<std::uint64_t>(1) << b)
					{
						temp[0] ^= state[0];
						temp[1] ^= state[1];
						temp[2] ^= state[2];
						temp[3] ^= state[3];
					}
					do_next();
				}

			std::copy_n(temp, SEK_ARRAY_SIZE(state), state);
		}

		state_type state;
	};
	template<>
	struct xoroshiro_base<double, 128>
	{
		typedef double result_type;
		typedef std::uint64_t state_type[2];

		constexpr static state_type initial = {0x4424e0232e2f107b, 0x70865936c5c27271};
		constexpr static state_type jmp_short = {0xdf900294d8f554a5, 0x170865df4b3201fc};
		constexpr static state_type jmp_long = {0xd2a98b26625eee7b, 0xdddf9b1090aa7ac1};

		constexpr auto do_next() noexcept
		{
			const auto s0 = state[0];
			auto s1 = state[1];
			const auto result = s0 + s1;

			s1 ^= s0;
			state[0] = std::rotl(s0, 24) ^ s1 ^ (s1 << 16);
			state[1] = std::rotl(s1, 37);

			return uint64_to_double(result);
		}
		constexpr void do_jump(const state_type &jmp_arr) noexcept
		{
			state_type temp = {0};
			for (auto jmp : jmp_arr)
				for (auto b = 0; b < 64; b++)
				{
					if (jmp & static_cast<std::uint64_t>(1) << b)
					{
						temp[0] ^= state[0];
						temp[1] ^= state[1];
					}
					do_next();
				}

			std::copy_n(temp, SEK_ARRAY_SIZE(state), state);
		}

		state_type state;
	};

	template<>
	struct xoroshiro_base<std::uint32_t, 128>
	{
		typedef std::uint32_t result_type;
		typedef std::uint32_t state_type[4];

		constexpr static state_type initial = {0x4e2e2f7b, 0x836c6597, 0xf542d035, 0xa0e582d5};
		constexpr static state_type jmp_short = {0x8764000b, 0xf542d2d3, 0x6fa035c3, 0x77f2db5b};
		constexpr static state_type jmp_long = {0xb523952e, 0x0b6f099f, 0xccf5a0ef, 0x1c580662};

		constexpr auto do_next() noexcept
		{
			const auto result = std::rotl(state[1] * 5, 7) * 9;
			const auto temp = state[1] << 9;

			state[2] ^= state[0];
			state[3] ^= state[1];
			state[1] ^= state[2];
			state[0] ^= state[3];

			state[2] ^= temp;
			state[3] = std::rotl(state[3], 11);

			return result;
		}
		constexpr void do_jump(const result_type (&jmp_arr)[4]) noexcept
		{
			result_type temp[4] = {0};
			for (auto jmp : jmp_arr)
				for (auto b = 0; b < 32; b++)
				{
					if (jmp & static_cast<result_type>(1) << b)
					{
						temp[0] ^= state[0];
						temp[1] ^= state[1];
					}
					do_next();
				}

			std::copy_n(temp, SEK_ARRAY_SIZE(state), state);
		}

		state_type state;
	};
	template<>
	struct xoroshiro_base<float, 128>
	{
		typedef float result_type;
		typedef std::uint32_t state_type[4];

		constexpr static state_type initial = {0x4e2e2f7b, 0x836c6597, 0xf542d035, 0xa0e582d5};
		constexpr static state_type jmp_short = {0x8764000b, 0xf542d2d3, 0x6fa035c3, 0x77f2db5b};
		constexpr static state_type jmp_long = {0xb523952e, 0x0b6f099f, 0xccf5a0ef, 0x1c580662};

		constexpr auto do_next() noexcept
		{
			const auto result = state[0] + state[3];
			const auto temp = state[1] << 9;

			state[2] ^= state[0];
			state[3] ^= state[1];
			state[1] ^= state[2];
			state[0] ^= state[3];

			state[2] ^= temp;
			state[3] = std::rotl(state[3], 11);

			return uint32_to_float(result);
		}
		constexpr void do_jump(const state_type &jmp_arr) noexcept
		{
			state_type temp = {0};
			for (auto jmp : jmp_arr)
				for (auto b = 0; b < 32; b++)
				{
					if (jmp & static_cast<std::uint32_t>(1) << b)
					{
						temp[0] ^= state[0];
						temp[1] ^= state[1];
					}
					do_next();
				}

			std::copy_n(temp, SEK_ARRAY_SIZE(state), state);
		}

		state_type state;
	};

	/** xoroshiro random number engine.
	 * Implements RandomNumberEngine named requirement.
	 * @tparam T Type generated by the engine. Supported types: std::uint64_t, std::uint32_t, double, float.
	 * @tparam Width Width of the internal state. Supported values: 256 (std::uint64_t & double only), 128. */
	template<typename T, std::size_t Width>
	class xoroshiro : xoroshiro_base<T, Width>
	{
		using base = xoroshiro_base<T, Width>;

	public:
		typedef typename base::result_type result_type;

		constexpr static result_type min() { return std::numeric_limits<result_type>::min(); }
		constexpr static result_type max() { return std::numeric_limits<result_type>::max(); }

	public:
		/** Initializes the generator to a default state. */
		constexpr xoroshiro() noexcept { seed(); }
		/** Initializes the generator from a seed. */
		constexpr explicit xoroshiro(result_type s) noexcept { seed(s); }
		/** Initializes the generator from a seed sequence. */
		template<is_seed_generator<result_type> S>
		constexpr explicit xoroshiro(S s) noexcept
		{
			seed(s);
		}

		/** Seeds the generator with a default seed. */
		constexpr void seed() noexcept { std::copy(std::begin(base::initial), std::end(base::initial), base::state); }
		/** Seeds the generator with an integer seed. */
		constexpr void seed(result_type seed) noexcept
		{
			for (auto &s : base::state)
			{
				s = seed;
				mix_seed_xor(seed);
			}
		}
		/** Seeds the generator with a seed sequence. */
		template<is_seed_generator<result_type> S>
		constexpr void seed(S &s) noexcept
		{
			s.generate(std::begin(base::state), std::end(base::state));
		}

		/** Returns the next random number. */
		constexpr result_type next() noexcept { return base::do_next(); }
		/** @copydoc next */
		constexpr result_type operator()() noexcept { return next(); }

		/** Advances the generator by `n`. */
		constexpr void discard(std::size_t n) noexcept
		{
			/* No fast jump here, `n` max is < 2^128. */
			while (n-- > 0) next();
		}

		/** Advances the generator by 2^128. */
		constexpr void jump() noexcept { base::do_jump(base::jmp_short); }
		/** Advances the generator by 2^192. */
		constexpr void long_jump() noexcept { base::do_jump(base::jmp_long); }

		[[nodiscard]] constexpr bool operator==(const xoroshiro &) const noexcept = default;
	};

	/* TODO: implement stream input & output. */
}	 // namespace sek::math