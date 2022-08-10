/*
 * Created by switchblade on 2021-11-09
 */

#pragma once

#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>

#include "meta_util.hpp"

namespace sek
{
	typedef std::size_t hash_t;

	constexpr std::uint32_t crc32_table[] = {
		0x00000000, 0x77073096, 0xee0e612c, 0x990951ba, 0x076dc419, 0x706af48f, 0xe963a535, 0x9e6495a3, 0x0edb8832,
		0x79dcb8a4, 0xe0d5e91e, 0x97d2d988, 0x09b64c2b, 0x7eb17cbd, 0xe7b82d07, 0x90bf1d91, 0x1db71064, 0x6ab020f2,
		0xf3b97148, 0x84be41de, 0x1adad47d, 0x6ddde4eb, 0xf4d4b551, 0x83d385c7, 0x136c9856, 0x646ba8c0, 0xfd62f97a,
		0x8a65c9ec, 0x14015c4f, 0x63066cd9, 0xfa0f3d63, 0x8d080df5, 0x3b6e20c8, 0x4c69105e, 0xd56041e4, 0xa2677172,
		0x3c03e4d1, 0x4b04d447, 0xd20d85fd, 0xa50ab56b, 0x35b5a8fa, 0x42b2986c, 0xdbbbc9d6, 0xacbcf940, 0x32d86ce3,
		0x45df5c75, 0xdcd60dcf, 0xabd13d59, 0x26d930ac, 0x51de003a, 0xc8d75180, 0xbfd06116, 0x21b4f4b5, 0x56b3c423,
		0xcfba9599, 0xb8bda50f, 0x2802b89e, 0x5f058808, 0xc60cd9b2, 0xb10be924, 0x2f6f7c87, 0x58684c11, 0xc1611dab,
		0xb6662d3d, 0x76dc4190, 0x01db7106, 0x98d220bc, 0xefd5102a, 0x71b18589, 0x06b6b51f, 0x9fbfe4a5, 0xe8b8d433,
		0x7807c9a2, 0x0f00f934, 0x9609a88e, 0xe10e9818, 0x7f6a0dbb, 0x086d3d2d, 0x91646c97, 0xe6635c01, 0x6b6b51f4,
		0x1c6c6162, 0x856530d8, 0xf262004e, 0x6c0695ed, 0x1b01a57b, 0x8208f4c1, 0xf50fc457, 0x65b0d9c6, 0x12b7e950,
		0x8bbeb8ea, 0xfcb9887c, 0x62dd1ddf, 0x15da2d49, 0x8cd37cf3, 0xfbd44c65, 0x4db26158, 0x3ab551ce, 0xa3bc0074,
		0xd4bb30e2, 0x4adfa541, 0x3dd895d7, 0xa4d1c46d, 0xd3d6f4fb, 0x4369e96a, 0x346ed9fc, 0xad678846, 0xda60b8d0,
		0x44042d73, 0x33031de5, 0xaa0a4c5f, 0xdd0d7cc9, 0x5005713c, 0x270241aa, 0xbe0b1010, 0xc90c2086, 0x5768b525,
		0x206f85b3, 0xb966d409, 0xce61e49f, 0x5edef90e, 0x29d9c998, 0xb0d09822, 0xc7d7a8b4, 0x59b33d17, 0x2eb40d81,
		0xb7bd5c3b, 0xc0ba6cad, 0xedb88320, 0x9abfb3b6, 0x03b6e20c, 0x74b1d29a, 0xead54739, 0x9dd277af, 0x04db2615,
		0x73dc1683, 0xe3630b12, 0x94643b84, 0x0d6d6a3e, 0x7a6a5aa8, 0xe40ecf0b, 0x9309ff9d, 0x0a00ae27, 0x7d079eb1,
		0xf00f9344, 0x8708a3d2, 0x1e01f268, 0x6906c2fe, 0xf762575d, 0x806567cb, 0x196c3671, 0x6e6b06e7, 0xfed41b76,
		0x89d32be0, 0x10da7a5a, 0x67dd4acc, 0xf9b9df6f, 0x8ebeeff9, 0x17b7be43, 0x60b08ed5, 0xd6d6a3e8, 0xa1d1937e,
		0x38d8c2c4, 0x4fdff252, 0xd1bb67f1, 0xa6bc5767, 0x3fb506dd, 0x48b2364b, 0xd80d2bda, 0xaf0a1b4c, 0x36034af6,
		0x41047a60, 0xdf60efc3, 0xa867df55, 0x316e8eef, 0x4669be79, 0xcb61b38c, 0xbc66831a, 0x256fd2a0, 0x5268e236,
		0xcc0c7795, 0xbb0b4703, 0x220216b9, 0x5505262f, 0xc5ba3bbe, 0xb2bd0b28, 0x2bb45a92, 0x5cb36a04, 0xc2d7ffa7,
		0xb5d0cf31, 0x2cd99e8b, 0x5bdeae1d, 0x9b64c2b0, 0xec63f226, 0x756aa39c, 0x026d930a, 0x9c0906a9, 0xeb0e363f,
		0x72076785, 0x05005713, 0x95bf4a82, 0xe2b87a14, 0x7bb12bae, 0x0cb61b38, 0x92d28e9b, 0xe5d5be0d, 0x7cdcefb7,
		0x0bdbdf21, 0x86d3d2d4, 0xf1d4e242, 0x68ddb3f8, 0x1fda836e, 0x81be16cd, 0xf6b9265b, 0x6fb077e1, 0x18b74777,
		0x88085ae6, 0xff0f6a70, 0x66063bca, 0x11010b5c, 0x8f659eff, 0xf862ae69, 0x616bffd3, 0x166ccf45, 0xa00ae278,
		0xd70dd2ee, 0x4e048354, 0x3903b3c2, 0xa7672661, 0xd06016f7, 0x4969474d, 0x3e6e77db, 0xaed16a4a, 0xd9d65adc,
		0x40df0b66, 0x37d83bf0, 0xa9bcae53, 0xdebb9ec5, 0x47b2cf7f, 0x30b5ffe9, 0xbdbdf21c, 0xcabac28a, 0x53b39330,
		0x24b4a3a6, 0xbad03605, 0xcdd70693, 0x54de5729, 0x23d967bf, 0xb3667a2e, 0xc4614ab8, 0x5d681b02, 0x2a6f2b94,
		0xb40bbe37, 0xc30c8ea1, 0x5a05df1b, 0x2d02ef8d,
	};
	[[nodiscard]] constexpr std::uint32_t crc32(const std::byte *data, std::size_t n) noexcept
	{
		std::uint32_t result = 0xffff'ffff;
		for (std::size_t i = 0; i < n; i++)
		{
			const auto idx = (result ^ static_cast<std::uint32_t>(data[i])) & 0xff;
			result = (result >> 8) ^ crc32_table[idx];
		}
		return ~result;
	}

	namespace detail
	{
		constexpr std::uint32_t md5_a = 0x67452301;
		constexpr std::uint32_t md5_b = 0xefcdab89;
		constexpr std::uint32_t md5_c = 0x98badcfe;
		constexpr std::uint32_t md5_d = 0x10325476;

		constexpr int md5_s[] = {7,	 12, 17, 22, 7,	 12, 17, 22, 7,	 12, 17, 22, 7,	 12, 17, 22, 5,	 9,	 14, 20, 5,	 9,
								 14, 20, 5,	 9,	 14, 20, 5,	 9,	 14, 20, 4,	 11, 16, 23, 4,	 11, 16, 23, 4,	 11, 16, 23,
								 4,	 11, 16, 23, 6,	 10, 15, 21, 6,	 10, 15, 21, 6,	 10, 15, 21, 6,	 10, 15, 21};

		constexpr std::uint32_t md5_k[] = {
			0xd76aa478, 0xe8c7b756, 0x242070db, 0xc1bdceee, 0xf57c0faf, 0x4787c62a, 0xa8304613, 0xfd469501,
			0x698098d8, 0x8b44f7af, 0xffff5bb1, 0x895cd7be, 0x6b901122, 0xfd987193, 0xa679438e, 0x49b40821,
			0xf61e2562, 0xc040b340, 0x265e5a51, 0xe9b6c7aa, 0xd62f105d, 0x02441453, 0xd8a1e681, 0xe7d3fbc8,
			0x21e1cde6, 0xc33707d6, 0xf4d50d87, 0x455a14ed, 0xa9e3e905, 0xfcefa3f8, 0x676f02d9, 0x8d2a4c8a,
			0xfffa3942, 0x8771f681, 0x6d9d6122, 0xfde5380c, 0xa4beea44, 0x4bdecfa9, 0xf6bb4b60, 0xbebfbc70,
			0x289b7ec6, 0xeaa127fa, 0xd4ef3085, 0x04881d05, 0xd9d4d039, 0xe6db99e5, 0x1fa27cf8, 0xc4ac5665,
			0xf4292244, 0x432aff97, 0xab9423a7, 0xfc93a039, 0x655b59c3, 0x8f0ccc92, 0xffeff47d, 0x85845dd1,
			0x6fa87e4f, 0xfe2ce6e0, 0xa3014314, 0x4e0811a1, 0xf7537e82, 0xbd3af235, 0x2ad7d2bb, 0xeb86d391};

		constexpr auto md5_f(auto X, auto Y, auto Z) { return ((X & Y) | (~X & Z)); }
		constexpr auto md5_g(auto X, auto Y, auto Z) { return ((X & Z) | (Y & ~Z)); }
		constexpr auto md5_h(auto X, auto Y, auto Z) { return (X ^ Y ^ Z); }
		constexpr auto md5_i(auto X, auto Y, auto Z) { return (Y ^ (X | ~Z)); }

		constexpr std::uint8_t md5_pad[] = {
			0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

		class md5_generator
		{
		public:
			[[nodiscard]] constexpr std::array<std::byte, 16> operator()(const std::byte *data, std::uint64_t n) noexcept
			{
				update(std::bit_cast<const std::uint8_t *>(data), n);
				finalize();
				return digest;
			}

		private:
			constexpr void step(const std::uint32_t data[]) noexcept
			{
				std::uint32_t a = buffer[0], b = buffer[1], c = buffer[2], d = buffer[3];

				std::uint32_t e;
				for (std::size_t j, i = 0; i < 64; ++i)
				{
					switch (i / 16)
					{
						case 0:
							e = md5_f(b, c, d);
							j = i;
							break;
						case 1:
							e = md5_g(b, c, d);
							j = ((i * 5) + 1) % 16;
							break;
						case 2:
							e = md5_h(b, c, d);
							j = ((i * 3) + 5) % 16;
							break;
						default:
							e = md5_i(b, c, d);
							j = (i * 7) % 16;
							break;
					}

					const auto temp = d;
					d = c;
					c = b;
					b = b + std::rotl(a + e + md5_k[i] + data[j], md5_s[i]);
					a = temp;
				}

				buffer[0] += a;
				buffer[1] += b;
				buffer[2] += c;
				buffer[3] += d;
			}
			constexpr void update(const std::uint8_t data[], std::uint64_t n) noexcept
			{
				std::uint32_t work_data[16];

				auto offset = size % 64;
				size += static_cast<std::uint64_t>(n);
				for (std::size_t i = 0; i < n; ++i)
				{
					input[offset++] = data[i];
					if (offset % 64 == 0)
					{
						for (std::size_t j = 0; j < 16; ++j)
						{
							work_data[j] = static_cast<std::uint32_t>(input[(j * 4) + 3]) << 24 |
										   static_cast<std::uint32_t>(input[(j * 4) + 2]) << 16 |
										   static_cast<std::uint32_t>(input[(j * 4) + 1]) << 8 |
										   static_cast<std::uint32_t>(input[(j * 4)]);
						}
						step(work_data);
						offset = 0;
					}
				}
			}
			constexpr void finalize() noexcept
			{
				std::uint32_t work_data[16];
				const auto offset = size % 64;
				const auto padding_size = offset < 56 ? 56 - offset : (56 + 64) - offset;

				update(md5_pad, padding_size);
				size -= padding_size;

				for (std::size_t j = 0; j < 14; ++j)
				{
					work_data[j] = static_cast<std::uint32_t>(input[(j * 4) + 3]) << 24 |
								   static_cast<std::uint32_t>(input[(j * 4) + 2]) << 16 |
								   static_cast<std::uint32_t>(input[(j * 4) + 1]) << 8 |
								   static_cast<std::uint32_t>(input[(j * 4)]);
				}

				work_data[14] = static_cast<std::uint32_t>(size * 8);
				work_data[15] = static_cast<std::uint32_t>((size * 8) >> 32);
				step(work_data);

				for (unsigned int i = 0; i < 4; ++i)
				{
					digest[(i * 4) + 0] = static_cast<std::byte>((buffer[i] & 0x000000ff));
					digest[(i * 4) + 1] = static_cast<std::byte>((buffer[i] & 0x0000ff00) >> 8);
					digest[(i * 4) + 2] = static_cast<std::byte>((buffer[i] & 0x00ff0000) >> 16);
					digest[(i * 4) + 3] = static_cast<std::byte>((buffer[i] & 0xff000000) >> 24);
				}
			}

			std::uint64_t size = 0;
			std::uint32_t buffer[4] = {md5_a, md5_b, md5_c, md5_d};
			std::uint8_t input[64];
			std::array<std::byte, 16> digest;
		};
	}	 // namespace detail

	[[nodiscard]] constexpr std::array<std::byte, 16> md5(const std::byte *data, std::size_t n) noexcept
	{
		return detail::md5_generator{}(data, n);
	}
	[[nodiscard]] constexpr std::array<std::byte, 16> md5(const void *data, std::size_t n) noexcept
	{
		return md5(static_cast<const std::byte *>(data), n);
	}
	template<typename T>
	[[nodiscard]] constexpr std::array<std::byte, 16> md5(const T *data, std::size_t n) noexcept
	{
		return md5(std::bit_cast<const std::byte *>(data), n * sizeof(T));
	}

	[[nodiscard]] constexpr std::size_t sdbm(const std::uint8_t *data, std::size_t len, uint32_t seed)
	{
		std::size_t result = seed;
		for (; len > 0; --len, ++data) result = *data + (result << 6) + (result << 16) - result;
		return result;
	}

#if INTPTR_MAX < INT64_MAX
	constexpr hash_t fnv1a_prime = 0x01000193;
	constexpr hash_t fnv1a_offset = 0x811c9dc5;
#else
	constexpr hash_t fnv1a_prime = 0x00000100000001b3;
	constexpr hash_t fnv1a_offset = 0xcbf29ce484222325;
#endif

	namespace detail
	{
		template<std::size_t Size, std::size_t Byte = 1>
		[[nodiscard]] constexpr hash_t fnv1a_iteration(std::size_t value, hash_t result) noexcept
		{
			/* Iterating by std::size_t value in order to allow for compile-time evaluation. */
			if constexpr (Byte <= Size)
			{
				result ^= static_cast<std::uint8_t>(value >> (8 * (Size - Byte)));
				result *= fnv1a_prime;
				return fnv1a_iteration<Size, Byte + 1>(value, result);
			}
			else
				return result;
		}
	}	 // namespace detail

	template<typename T>
	[[nodiscard]] constexpr hash_t fnv1a(const T *data, std::size_t len, hash_t seed = fnv1a_offset)
	{
		hash_t result = seed;
		while (len--) { result = detail::fnv1a_iteration<sizeof(T)>(static_cast<std::size_t>(data[len]), result); }
		return result;
	}
	[[nodiscard]] constexpr hash_t byte_hash(const void *data, std::size_t len, hash_t seed = fnv1a_offset) noexcept
	{
		return fnv1a(static_cast<const uint8_t *>(data), len, seed);
	}

	template<std::integral I>
	[[nodiscard]] constexpr hash_t hash(I value) noexcept
	{
		return static_cast<hash_t>(value);
	}
	template<typename E>
	[[nodiscard]] constexpr hash_t hash(E value) noexcept
		requires std::is_enum_v<E>
	{
		return hash(static_cast<std::underlying_type_t<E>>(value));
	}
	template<typename T>
	[[nodiscard]] constexpr hash_t hash(T *value) noexcept
	{
		return std::bit_cast<hash_t>(value);
	}
	template<pointer_like T>
	[[nodiscard]] constexpr hash_t hash(T value) noexcept
	{
		return hash(std::to_address(value));
	}
	[[nodiscard]] constexpr hash_t hash(std::nullptr_t) noexcept { return 0; }

	template<typename T>
	concept has_hash = requires(T t) { hash(t); };

	/** Combines hash of the value type with the seed.
	 * @param seed Seed to combine the hash with.
	 * @param value Value to hash.
	 * @return Copy of seed.
	 * @note Value type must have a hash function defined for it. The function is looked up via ADL. */
	template<has_hash T>
	constexpr hash_t hash_combine(hash_t &seed, const T &value) noexcept
	{
		return seed = (seed ^ (hash(value) + 0x9e3779b9 + (seed << 6) + (seed >> 2)));
	}

	template<std::ranges::forward_range R>
	[[nodiscard]] constexpr hash_t hash(const R &r) noexcept
		requires has_hash<std::ranges::range_value_t<R>>
	{
		hash_t result = {};
		for (const auto &value : r) hash_combine(result, value);
		return result;
	}

	namespace detail
	{
		template<typename T, std::size_t... Is>
		constexpr bool hashable_tuple_element(std::index_sequence<Is...>)
		{
			return std::conjunction_v<std::bool_constant<has_hash<std::tuple_element_t<Is, T>>>...>;
		}
		template<typename T>
		concept tuple_like_hash = requires(const T &t) {
									  typename std::tuple_size<T>::type;
									  std::tuple_size_v<T> != 0;
									  hashable_tuple_element<T>(std::make_index_sequence<std::tuple_size_v<T>>());
								  };
		template<typename P>
		concept pair_like_hash = requires(const P &p) {
									 p.first;
									 p.second;
									 requires has_hash<std::decay_t<decltype(p.first)>>;
									 requires has_hash<std::decay_t<decltype(p.second)>>;
								 };
	}	 // namespace detail

	template<detail::tuple_like_hash T>
	[[nodiscard]] constexpr hash_t hash(const T &value) noexcept
	{
		hash_t result = hash(std::get<0>(value));
		if constexpr (std::tuple_size_v<T> != 1)
		{
			constexpr auto unwrap = []<std::size_t... Is>(hash_t & seed, const T &t, std::index_sequence<Is...>)
			{
				(hash_combine(seed, hash(std::get<Is + 1>(t))), ...);
			};
			unwrap(result, value, std::make_index_sequence<std::tuple_size_v<T> - 1>());
		}
		return result;
	}
	template<detail::pair_like_hash P>
	[[nodiscard]] constexpr hash_t hash(const P &p) noexcept
	{
		auto result = hash(p.first);
		return hash_combine(result, hash(p.second));
	}

	template<typename T>
	[[nodiscard]] constexpr hash_t hash(const T &v) noexcept
		requires(!std::integral<T> && !pointer_like<T> && !std::is_pointer_v<T> && !std::same_as<T, std::nullptr_t> &&
				 !std::ranges::forward_range<T> && requires { std::hash<T>{}(v); })
	{
		return std::hash<T>{}(v);
	}

	/** @brief Hasher that calls `hash` function on the passed object via ADL.
	 * If such function is not available, uses `std::hash`. */
	struct default_hash
	{
		template<typename T>
		[[nodiscard]] constexpr hash_t operator()(const T &value) const noexcept
		{
			if constexpr (has_hash<T>)
				return hash(value);
			else
				return std::hash<T>{}(value);
		}
	};
}	 // namespace sek
