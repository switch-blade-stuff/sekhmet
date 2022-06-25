/*
 * Created by switchblade on 22/06/22
 */
#pragma once

#define SEK_DETAIL_Q_SHUFFLE(...) (shuffle<__VA_ARGS__>(vector()))

#define SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T, name, ...)                                                                 \
	[[nodiscard]] constexpr decltype(auto) name() const noexcept                                                       \
	{                                                                                                                  \
		return T{SHUFFLE(__VA_ARGS__)};                                                                                \
	}

#define SEK_DETAIL_SHUFFLE_2(SHUFFLE, T, x, y)                                                                         \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(2), SEK_CONCAT(x, x), 0, 0)                                                     \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(2), SEK_CONCAT(x, y), 0, 1)                                                     \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(2), SEK_CONCAT(y, x), 1, 0)                                                     \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(2), SEK_CONCAT(y, y), 1, 1)

#define SEK_DETAIL_SHUFFLE_3(SHUFFLE, T, x, y, z)                                                                      \
	SEK_DETAIL_SHUFFLE_2(SHUFFLE, T, x, y)                                                                             \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(2), SEK_CONCAT(x, z), 0, 2)                                                     \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(2), SEK_CONCAT(y, z), 1, 2)                                                     \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(2), SEK_CONCAT(z, x), 2, 0)                                                     \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(2), SEK_CONCAT(z, y), 2, 1)                                                     \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(2), SEK_CONCAT(z, z), 2, 2)                                                     \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(3), SEK_CONCAT(x, x, x), 0, 0, 0)                                               \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(3), SEK_CONCAT(x, x, y), 0, 0, 1)                                               \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(3), SEK_CONCAT(x, x, z), 0, 0, 2)                                               \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(3), SEK_CONCAT(x, y, x), 0, 1, 0)                                               \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(3), SEK_CONCAT(x, y, y), 0, 1, 1)                                               \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(3), SEK_CONCAT(x, y, z), 0, 1, 2)                                               \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(3), SEK_CONCAT(x, z, x), 0, 2, 0)                                               \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(3), SEK_CONCAT(x, z, y), 0, 2, 1)                                               \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(3), SEK_CONCAT(x, z, z), 0, 2, 2)                                               \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(3), SEK_CONCAT(y, x, x), 1, 0, 0)                                               \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(3), SEK_CONCAT(y, x, y), 1, 0, 1)                                               \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(3), SEK_CONCAT(y, x, z), 1, 0, 2)                                               \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(3), SEK_CONCAT(y, y, x), 1, 1, 0)                                               \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(3), SEK_CONCAT(y, y, y), 1, 1, 1)                                               \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(3), SEK_CONCAT(y, y, z), 1, 1, 2)                                               \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(3), SEK_CONCAT(y, z, x), 1, 2, 0)                                               \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(3), SEK_CONCAT(y, z, y), 1, 2, 1)                                               \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(3), SEK_CONCAT(y, z, z), 1, 2, 2)                                               \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(3), SEK_CONCAT(z, x, x), 2, 0, 0)                                               \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(3), SEK_CONCAT(z, x, y), 2, 0, 1)                                               \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(3), SEK_CONCAT(z, x, z), 2, 0, 2)                                               \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(3), SEK_CONCAT(z, y, x), 2, 1, 0)                                               \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(3), SEK_CONCAT(z, y, y), 2, 1, 1)                                               \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(3), SEK_CONCAT(z, y, z), 2, 1, 2)                                               \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(3), SEK_CONCAT(z, z, x), 2, 2, 0)                                               \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(3), SEK_CONCAT(z, z, y), 2, 2, 1)                                               \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(3), SEK_CONCAT(z, z, z), 2, 2, 2)

#define SEK_DETAIL_SHUFFLE_4(SHUFFLE, T, x, y, z, w)                                                                   \
	SEK_DETAIL_SHUFFLE_3(SHUFFLE, T, x, y, z)                                                                          \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(2), SEK_CONCAT(x, w), 0, 3)                                                     \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(2), SEK_CONCAT(y, w), 1, 3)                                                     \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(2), SEK_CONCAT(z, w), 2, 3)                                                     \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(2), SEK_CONCAT(w, x), 3, 0)                                                     \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(2), SEK_CONCAT(w, y), 3, 1)                                                     \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(2), SEK_CONCAT(w, z), 3, 2)                                                     \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(2), SEK_CONCAT(w, w), 3, 3)                                                     \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(3), SEK_CONCAT(x, x, w), 0, 0, 3)                                               \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(3), SEK_CONCAT(x, y, w), 0, 1, 3)                                               \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(3), SEK_CONCAT(x, z, w), 0, 2, 3)                                               \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(3), SEK_CONCAT(y, x, w), 1, 0, 3)                                               \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(3), SEK_CONCAT(y, y, w), 1, 1, 3)                                               \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(3), SEK_CONCAT(y, z, w), 1, 2, 3)                                               \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(3), SEK_CONCAT(z, x, w), 2, 0, 3)                                               \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(3), SEK_CONCAT(z, y, w), 2, 1, 3)                                               \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(3), SEK_CONCAT(z, z, w), 2, 2, 3)                                               \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(3), SEK_CONCAT(x, w, x), 0, 3, 0)                                               \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(3), SEK_CONCAT(x, w, y), 0, 3, 1)                                               \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(3), SEK_CONCAT(x, w, z), 0, 3, 2)                                               \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(3), SEK_CONCAT(x, w, w), 0, 3, 3)                                               \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(3), SEK_CONCAT(y, w, x), 1, 3, 0)                                               \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(3), SEK_CONCAT(y, w, y), 1, 3, 1)                                               \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(3), SEK_CONCAT(y, w, z), 1, 3, 2)                                               \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(3), SEK_CONCAT(y, w, w), 1, 3, 3)                                               \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(3), SEK_CONCAT(z, w, x), 2, 3, 0)                                               \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(3), SEK_CONCAT(z, w, y), 2, 3, 1)                                               \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(3), SEK_CONCAT(z, w, z), 2, 3, 2)                                               \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(3), SEK_CONCAT(z, w, w), 2, 3, 3)                                               \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(3), SEK_CONCAT(w, x, x), 3, 0, 0)                                               \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(3), SEK_CONCAT(w, x, y), 3, 0, 1)                                               \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(3), SEK_CONCAT(w, x, z), 3, 0, 2)                                               \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(3), SEK_CONCAT(w, x, w), 3, 0, 3)                                               \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(3), SEK_CONCAT(w, y, x), 3, 1, 0)                                               \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(3), SEK_CONCAT(w, y, y), 3, 1, 1)                                               \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(3), SEK_CONCAT(w, y, z), 3, 1, 2)                                               \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(3), SEK_CONCAT(w, y, w), 3, 1, 3)                                               \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(3), SEK_CONCAT(w, z, x), 3, 2, 0)                                               \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(3), SEK_CONCAT(w, z, y), 3, 2, 1)                                               \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(3), SEK_CONCAT(w, z, z), 3, 2, 2)                                               \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(3), SEK_CONCAT(w, z, w), 3, 2, 3)                                               \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(3), SEK_CONCAT(w, w, x), 3, 3, 0)                                               \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(3), SEK_CONCAT(w, w, y), 3, 3, 1)                                               \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(3), SEK_CONCAT(w, w, z), 3, 3, 2)                                               \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(3), SEK_CONCAT(w, w, w), 3, 3, 3)                                               \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(4), SEK_CONCAT(x, x, x, x), 0, 0, 0, 0)                                         \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(4), SEK_CONCAT(x, x, x, y), 0, 0, 0, 1)                                         \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(4), SEK_CONCAT(x, x, x, z), 0, 0, 0, 2)                                         \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(4), SEK_CONCAT(x, x, x, w), 0, 0, 0, 3)                                         \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(4), SEK_CONCAT(x, x, y, x), 0, 0, 1, 0)                                         \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(4), SEK_CONCAT(x, x, y, y), 0, 0, 1, 1)                                         \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(4), SEK_CONCAT(x, x, y, z), 0, 0, 1, 2)                                         \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(4), SEK_CONCAT(x, x, y, w), 0, 0, 1, 3)                                         \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(4), SEK_CONCAT(x, x, z, x), 0, 0, 2, 0)                                         \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(4), SEK_CONCAT(x, x, z, y), 0, 0, 2, 1)                                         \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(4), SEK_CONCAT(x, x, z, z), 0, 0, 2, 2)                                         \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(4), SEK_CONCAT(x, x, z, w), 0, 0, 2, 3)                                         \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(4), SEK_CONCAT(x, x, w, x), 0, 0, 3, 0)                                         \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(4), SEK_CONCAT(x, x, w, y), 0, 0, 3, 1)                                         \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(4), SEK_CONCAT(x, x, w, z), 0, 0, 3, 2)                                         \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(4), SEK_CONCAT(x, x, w, w), 0, 0, 3, 3)                                         \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(4), SEK_CONCAT(x, y, x, x), 0, 1, 0, 0)                                         \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(4), SEK_CONCAT(x, y, x, y), 0, 1, 0, 1)                                         \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(4), SEK_CONCAT(x, y, x, z), 0, 1, 0, 2)                                         \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(4), SEK_CONCAT(x, y, x, w), 0, 1, 0, 3)                                         \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(4), SEK_CONCAT(x, y, y, x), 0, 1, 1, 0)                                         \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(4), SEK_CONCAT(x, y, y, y), 0, 1, 1, 1)                                         \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(4), SEK_CONCAT(x, y, y, z), 0, 1, 1, 2)                                         \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(4), SEK_CONCAT(x, y, y, w), 0, 1, 1, 3)                                         \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(4), SEK_CONCAT(x, y, z, x), 0, 1, 2, 0)                                         \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(4), SEK_CONCAT(x, y, z, y), 0, 1, 2, 1)                                         \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(4), SEK_CONCAT(x, y, z, z), 0, 1, 2, 2)                                         \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(4), SEK_CONCAT(x, y, z, w), 0, 1, 2, 3)                                         \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(4), SEK_CONCAT(x, y, w, x), 0, 1, 3, 0)                                         \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(4), SEK_CONCAT(x, y, w, y), 0, 1, 3, 1)                                         \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(4), SEK_CONCAT(x, y, w, z), 0, 1, 3, 2)                                         \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(4), SEK_CONCAT(x, y, w, w), 0, 1, 3, 3)                                         \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(4), SEK_CONCAT(x, z, x, x), 0, 2, 0, 0)                                         \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(4), SEK_CONCAT(x, z, x, y), 0, 2, 0, 1)                                         \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(4), SEK_CONCAT(x, z, x, z), 0, 2, 0, 2)                                         \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(4), SEK_CONCAT(x, z, x, w), 0, 2, 0, 3)                                         \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(4), SEK_CONCAT(x, z, y, x), 0, 2, 1, 0)                                         \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(4), SEK_CONCAT(x, z, y, y), 0, 2, 1, 1)                                         \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(4), SEK_CONCAT(x, z, y, z), 0, 2, 1, 2)                                         \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(4), SEK_CONCAT(x, z, y, w), 0, 2, 1, 3)                                         \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(4), SEK_CONCAT(x, z, z, x), 0, 2, 2, 0)                                         \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(4), SEK_CONCAT(x, z, z, y), 0, 2, 2, 1)                                         \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(4), SEK_CONCAT(x, z, z, z), 0, 2, 2, 2)                                         \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(4), SEK_CONCAT(x, z, z, w), 0, 2, 2, 3)                                         \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(4), SEK_CONCAT(x, z, w, x), 0, 2, 3, 0)                                         \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(4), SEK_CONCAT(x, z, w, y), 0, 2, 3, 1)                                         \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(4), SEK_CONCAT(x, z, w, z), 0, 2, 3, 2)                                         \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(4), SEK_CONCAT(x, z, w, w), 0, 2, 3, 3)                                         \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(4), SEK_CONCAT(x, w, x, x), 0, 3, 0, 0)                                         \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(4), SEK_CONCAT(x, w, x, y), 0, 3, 0, 1)                                         \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(4), SEK_CONCAT(x, w, x, z), 0, 3, 0, 2)                                         \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(4), SEK_CONCAT(x, w, x, w), 0, 3, 0, 3)                                         \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(4), SEK_CONCAT(x, w, y, x), 0, 3, 1, 0)                                         \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(4), SEK_CONCAT(x, w, y, y), 0, 3, 1, 1)                                         \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(4), SEK_CONCAT(x, w, y, z), 0, 3, 1, 2)                                         \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(4), SEK_CONCAT(x, w, y, w), 0, 3, 1, 3)                                         \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(4), SEK_CONCAT(x, w, z, x), 0, 3, 2, 0)                                         \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(4), SEK_CONCAT(x, w, z, y), 0, 3, 2, 1)                                         \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(4), SEK_CONCAT(x, w, z, z), 0, 3, 2, 2)                                         \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(4), SEK_CONCAT(x, w, z, w), 0, 3, 2, 3)                                         \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(4), SEK_CONCAT(x, w, w, x), 0, 3, 3, 0)                                         \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(4), SEK_CONCAT(x, w, w, y), 0, 3, 3, 1)                                         \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(4), SEK_CONCAT(x, w, w, z), 0, 3, 3, 2)                                         \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(4), SEK_CONCAT(x, w, w, w), 0, 3, 3, 3)                                         \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(4), SEK_CONCAT(y, x, x, x), 1, 0, 0, 0)                                         \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(4), SEK_CONCAT(y, x, x, y), 1, 0, 0, 1)                                         \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(4), SEK_CONCAT(y, x, x, z), 1, 0, 0, 2)                                         \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(4), SEK_CONCAT(y, x, x, w), 1, 0, 0, 3)                                         \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(4), SEK_CONCAT(y, x, y, x), 1, 0, 1, 0)                                         \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(4), SEK_CONCAT(y, x, y, y), 1, 0, 1, 1)                                         \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(4), SEK_CONCAT(y, x, y, z), 1, 0, 1, 2)                                         \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(4), SEK_CONCAT(y, x, y, w), 1, 0, 1, 3)                                         \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(4), SEK_CONCAT(y, x, z, x), 1, 0, 2, 0)                                         \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(4), SEK_CONCAT(y, x, z, y), 1, 0, 2, 1)                                         \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(4), SEK_CONCAT(y, x, z, z), 1, 0, 2, 2)                                         \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(4), SEK_CONCAT(y, x, z, w), 1, 0, 2, 3)                                         \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(4), SEK_CONCAT(y, x, w, x), 1, 0, 3, 0)                                         \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(4), SEK_CONCAT(y, x, w, y), 1, 0, 3, 1)                                         \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(4), SEK_CONCAT(y, x, w, z), 1, 0, 3, 2)                                         \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(4), SEK_CONCAT(y, x, w, w), 1, 0, 3, 3)                                         \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(4), SEK_CONCAT(y, y, x, x), 1, 1, 0, 0)                                         \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(4), SEK_CONCAT(y, y, x, y), 1, 1, 0, 1)                                         \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(4), SEK_CONCAT(y, y, x, z), 1, 1, 0, 2)                                         \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(4), SEK_CONCAT(y, y, x, w), 1, 1, 0, 3)                                         \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(4), SEK_CONCAT(y, y, y, x), 1, 1, 1, 0)                                         \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(4), SEK_CONCAT(y, y, y, y), 1, 1, 1, 1)                                         \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(4), SEK_CONCAT(y, y, y, z), 1, 1, 1, 2)                                         \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(4), SEK_CONCAT(y, y, y, w), 1, 1, 1, 3)                                         \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(4), SEK_CONCAT(y, y, z, x), 1, 1, 2, 0)                                         \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(4), SEK_CONCAT(y, y, z, y), 1, 1, 2, 1)                                         \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(4), SEK_CONCAT(y, y, z, z), 1, 1, 2, 2)                                         \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(4), SEK_CONCAT(y, y, z, w), 1, 1, 2, 3)                                         \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(4), SEK_CONCAT(y, y, w, x), 1, 1, 3, 0)                                         \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(4), SEK_CONCAT(y, y, w, y), 1, 1, 3, 1)                                         \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(4), SEK_CONCAT(y, y, w, z), 1, 1, 3, 2)                                         \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(4), SEK_CONCAT(y, y, w, w), 1, 1, 3, 3)                                         \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(4), SEK_CONCAT(y, z, x, x), 1, 2, 0, 0)                                         \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(4), SEK_CONCAT(y, z, x, y), 1, 2, 0, 1)                                         \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(4), SEK_CONCAT(y, z, x, z), 1, 2, 0, 2)                                         \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(4), SEK_CONCAT(y, z, x, w), 1, 2, 0, 3)                                         \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(4), SEK_CONCAT(y, z, y, x), 1, 2, 1, 0)                                         \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(4), SEK_CONCAT(y, z, y, y), 1, 2, 1, 1)                                         \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(4), SEK_CONCAT(y, z, y, z), 1, 2, 1, 2)                                         \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(4), SEK_CONCAT(y, z, y, w), 1, 2, 1, 3)                                         \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(4), SEK_CONCAT(y, z, z, x), 1, 2, 2, 0)                                         \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(4), SEK_CONCAT(y, z, z, y), 1, 2, 2, 1)                                         \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(4), SEK_CONCAT(y, z, z, z), 1, 2, 2, 2)                                         \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(4), SEK_CONCAT(y, z, z, w), 1, 2, 2, 3)                                         \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(4), SEK_CONCAT(y, z, w, x), 1, 2, 3, 0)                                         \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(4), SEK_CONCAT(y, z, w, y), 1, 2, 3, 1)                                         \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(4), SEK_CONCAT(y, z, w, z), 1, 2, 3, 2)                                         \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(4), SEK_CONCAT(y, z, w, w), 1, 2, 3, 3)                                         \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(4), SEK_CONCAT(y, w, x, x), 1, 3, 0, 0)                                         \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(4), SEK_CONCAT(y, w, x, y), 1, 3, 0, 1)                                         \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(4), SEK_CONCAT(y, w, x, z), 1, 3, 0, 2)                                         \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(4), SEK_CONCAT(y, w, x, w), 1, 3, 0, 3)                                         \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(4), SEK_CONCAT(y, w, y, x), 1, 3, 1, 0)                                         \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(4), SEK_CONCAT(y, w, y, y), 1, 3, 1, 1)                                         \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(4), SEK_CONCAT(y, w, y, z), 1, 3, 1, 2)                                         \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(4), SEK_CONCAT(y, w, y, w), 1, 3, 1, 3)                                         \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(4), SEK_CONCAT(y, w, z, x), 1, 3, 2, 0)                                         \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(4), SEK_CONCAT(y, w, z, y), 1, 3, 2, 1)                                         \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(4), SEK_CONCAT(y, w, z, z), 1, 3, 2, 2)                                         \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(4), SEK_CONCAT(y, w, z, w), 1, 3, 2, 3)                                         \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(4), SEK_CONCAT(y, w, w, x), 1, 3, 3, 0)                                         \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(4), SEK_CONCAT(y, w, w, y), 1, 3, 3, 1)                                         \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(4), SEK_CONCAT(y, w, w, z), 1, 3, 3, 2)                                         \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(4), SEK_CONCAT(y, w, w, w), 1, 3, 3, 3)                                         \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(4), SEK_CONCAT(z, x, x, x), 2, 0, 0, 0)                                         \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(4), SEK_CONCAT(z, x, x, y), 2, 0, 0, 1)                                         \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(4), SEK_CONCAT(z, x, x, z), 2, 0, 0, 2)                                         \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(4), SEK_CONCAT(z, x, x, w), 2, 0, 0, 3)                                         \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(4), SEK_CONCAT(z, x, y, x), 2, 0, 1, 0)                                         \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(4), SEK_CONCAT(z, x, y, y), 2, 0, 1, 1)                                         \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(4), SEK_CONCAT(z, x, y, z), 2, 0, 1, 2)                                         \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(4), SEK_CONCAT(z, x, y, w), 2, 0, 1, 3)                                         \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(4), SEK_CONCAT(z, x, z, x), 2, 0, 2, 0)                                         \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(4), SEK_CONCAT(z, x, z, y), 2, 0, 2, 1)                                         \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(4), SEK_CONCAT(z, x, z, z), 2, 0, 2, 2)                                         \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(4), SEK_CONCAT(z, x, z, w), 2, 0, 2, 3)                                         \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(4), SEK_CONCAT(z, x, w, x), 2, 0, 3, 0)                                         \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(4), SEK_CONCAT(z, x, w, y), 2, 0, 3, 1)                                         \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(4), SEK_CONCAT(z, x, w, z), 2, 0, 3, 2)                                         \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(4), SEK_CONCAT(z, x, w, w), 2, 0, 3, 3)                                         \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(4), SEK_CONCAT(z, y, x, x), 2, 1, 0, 0)                                         \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(4), SEK_CONCAT(z, y, x, y), 2, 1, 0, 1)                                         \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(4), SEK_CONCAT(z, y, x, z), 2, 1, 0, 2)                                         \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(4), SEK_CONCAT(z, y, x, w), 2, 1, 0, 3)                                         \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(4), SEK_CONCAT(z, y, y, x), 2, 1, 1, 0)                                         \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(4), SEK_CONCAT(z, y, y, y), 2, 1, 1, 1)                                         \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(4), SEK_CONCAT(z, y, y, z), 2, 1, 1, 2)                                         \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(4), SEK_CONCAT(z, y, y, w), 2, 1, 1, 3)                                         \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(4), SEK_CONCAT(z, y, z, x), 2, 1, 2, 0)                                         \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(4), SEK_CONCAT(z, y, z, y), 2, 1, 2, 1)                                         \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(4), SEK_CONCAT(z, y, z, z), 2, 1, 2, 2)                                         \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(4), SEK_CONCAT(z, y, z, w), 2, 1, 2, 3)                                         \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(4), SEK_CONCAT(z, y, w, x), 2, 1, 3, 0)                                         \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(4), SEK_CONCAT(z, y, w, y), 2, 1, 3, 1)                                         \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(4), SEK_CONCAT(z, y, w, z), 2, 1, 3, 2)                                         \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(4), SEK_CONCAT(z, y, w, w), 2, 1, 3, 3)                                         \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(4), SEK_CONCAT(z, z, x, x), 2, 2, 0, 0)                                         \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(4), SEK_CONCAT(z, z, x, y), 2, 2, 0, 1)                                         \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(4), SEK_CONCAT(z, z, x, z), 2, 2, 0, 2)                                         \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(4), SEK_CONCAT(z, z, x, w), 2, 2, 0, 3)                                         \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(4), SEK_CONCAT(z, z, y, x), 2, 2, 1, 0)                                         \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(4), SEK_CONCAT(z, z, y, y), 2, 2, 1, 1)                                         \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(4), SEK_CONCAT(z, z, y, z), 2, 2, 1, 2)                                         \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(4), SEK_CONCAT(z, z, y, w), 2, 2, 1, 3)                                         \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(4), SEK_CONCAT(z, z, z, x), 2, 2, 2, 0)                                         \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(4), SEK_CONCAT(z, z, z, y), 2, 2, 2, 1)                                         \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(4), SEK_CONCAT(z, z, z, z), 2, 2, 2, 2)                                         \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(4), SEK_CONCAT(z, z, z, w), 2, 2, 2, 3)                                         \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(4), SEK_CONCAT(z, z, w, x), 2, 2, 3, 0)                                         \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(4), SEK_CONCAT(z, z, w, y), 2, 2, 3, 1)                                         \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(4), SEK_CONCAT(z, z, w, z), 2, 2, 3, 2)                                         \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(4), SEK_CONCAT(z, z, w, w), 2, 2, 3, 3)                                         \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(4), SEK_CONCAT(z, w, x, x), 2, 3, 0, 0)                                         \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(4), SEK_CONCAT(z, w, x, y), 2, 3, 0, 1)                                         \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(4), SEK_CONCAT(z, w, x, z), 2, 3, 0, 2)                                         \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(4), SEK_CONCAT(z, w, x, w), 2, 3, 0, 3)                                         \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(4), SEK_CONCAT(z, w, y, x), 2, 3, 1, 0)                                         \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(4), SEK_CONCAT(z, w, y, y), 2, 3, 1, 1)                                         \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(4), SEK_CONCAT(z, w, y, z), 2, 3, 1, 2)                                         \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(4), SEK_CONCAT(z, w, y, w), 2, 3, 1, 3)                                         \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(4), SEK_CONCAT(z, w, z, x), 2, 3, 2, 0)                                         \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(4), SEK_CONCAT(z, w, z, y), 2, 3, 2, 1)                                         \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(4), SEK_CONCAT(z, w, z, z), 2, 3, 2, 2)                                         \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(4), SEK_CONCAT(z, w, z, w), 2, 3, 2, 3)                                         \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(4), SEK_CONCAT(z, w, w, x), 2, 3, 3, 0)                                         \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(4), SEK_CONCAT(z, w, w, y), 2, 3, 3, 1)                                         \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(4), SEK_CONCAT(z, w, w, z), 2, 3, 3, 2)                                         \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(4), SEK_CONCAT(z, w, w, w), 2, 3, 3, 3)                                         \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(4), SEK_CONCAT(w, x, x, x), 3, 0, 0, 0)                                         \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(4), SEK_CONCAT(w, x, x, y), 3, 0, 0, 1)                                         \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(4), SEK_CONCAT(w, x, x, z), 3, 0, 0, 2)                                         \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(4), SEK_CONCAT(w, x, x, w), 3, 0, 0, 3)                                         \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(4), SEK_CONCAT(w, x, y, x), 3, 0, 1, 0)                                         \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(4), SEK_CONCAT(w, x, y, y), 3, 0, 1, 1)                                         \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(4), SEK_CONCAT(w, x, y, z), 3, 0, 1, 2)                                         \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(4), SEK_CONCAT(w, x, y, w), 3, 0, 1, 3)                                         \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(4), SEK_CONCAT(w, x, z, x), 3, 0, 2, 0)                                         \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(4), SEK_CONCAT(w, x, z, y), 3, 0, 2, 1)                                         \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(4), SEK_CONCAT(w, x, z, z), 3, 0, 2, 2)                                         \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(4), SEK_CONCAT(w, x, z, w), 3, 0, 2, 3)                                         \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(4), SEK_CONCAT(w, x, w, x), 3, 0, 3, 0)                                         \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(4), SEK_CONCAT(w, x, w, y), 3, 0, 3, 1)                                         \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(4), SEK_CONCAT(w, x, w, z), 3, 0, 3, 2)                                         \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(4), SEK_CONCAT(w, x, w, w), 3, 0, 3, 3)                                         \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(4), SEK_CONCAT(w, y, x, x), 3, 1, 0, 0)                                         \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(4), SEK_CONCAT(w, y, x, y), 3, 1, 0, 1)                                         \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(4), SEK_CONCAT(w, y, x, z), 3, 1, 0, 2)                                         \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(4), SEK_CONCAT(w, y, x, w), 3, 1, 0, 3)                                         \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(4), SEK_CONCAT(w, y, y, x), 3, 1, 1, 0)                                         \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(4), SEK_CONCAT(w, y, y, y), 3, 1, 1, 1)                                         \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(4), SEK_CONCAT(w, y, y, z), 3, 1, 1, 2)                                         \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(4), SEK_CONCAT(w, y, y, w), 3, 1, 1, 3)                                         \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(4), SEK_CONCAT(w, y, z, x), 3, 1, 2, 0)                                         \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(4), SEK_CONCAT(w, y, z, y), 3, 1, 2, 1)                                         \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(4), SEK_CONCAT(w, y, z, z), 3, 1, 2, 2)                                         \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(4), SEK_CONCAT(w, y, z, w), 3, 1, 2, 3)                                         \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(4), SEK_CONCAT(w, y, w, x), 3, 1, 3, 0)                                         \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(4), SEK_CONCAT(w, y, w, y), 3, 1, 3, 1)                                         \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(4), SEK_CONCAT(w, y, w, z), 3, 1, 3, 2)                                         \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(4), SEK_CONCAT(w, y, w, w), 3, 1, 3, 3)                                         \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(4), SEK_CONCAT(w, z, x, x), 3, 2, 0, 0)                                         \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(4), SEK_CONCAT(w, z, x, y), 3, 2, 0, 1)                                         \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(4), SEK_CONCAT(w, z, x, z), 3, 2, 0, 2)                                         \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(4), SEK_CONCAT(w, z, x, w), 3, 2, 0, 3)                                         \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(4), SEK_CONCAT(w, z, y, x), 3, 2, 1, 0)                                         \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(4), SEK_CONCAT(w, z, y, y), 3, 2, 1, 1)                                         \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(4), SEK_CONCAT(w, z, y, z), 3, 2, 1, 2)                                         \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(4), SEK_CONCAT(w, z, y, w), 3, 2, 1, 3)                                         \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(4), SEK_CONCAT(w, z, z, x), 3, 2, 2, 0)                                         \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(4), SEK_CONCAT(w, z, z, y), 3, 2, 2, 1)                                         \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(4), SEK_CONCAT(w, z, z, z), 3, 2, 2, 2)                                         \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(4), SEK_CONCAT(w, z, z, w), 3, 2, 2, 3)                                         \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(4), SEK_CONCAT(w, z, w, x), 3, 2, 3, 0)                                         \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(4), SEK_CONCAT(w, z, w, y), 3, 2, 3, 1)                                         \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(4), SEK_CONCAT(w, z, w, z), 3, 2, 3, 2)                                         \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(4), SEK_CONCAT(w, z, w, w), 3, 2, 3, 3)                                         \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(4), SEK_CONCAT(w, w, x, x), 3, 3, 0, 0)                                         \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(4), SEK_CONCAT(w, w, x, y), 3, 3, 0, 1)                                         \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(4), SEK_CONCAT(w, w, x, z), 3, 3, 0, 2)                                         \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(4), SEK_CONCAT(w, w, x, w), 3, 3, 0, 3)                                         \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(4), SEK_CONCAT(w, w, y, x), 3, 3, 1, 0)                                         \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(4), SEK_CONCAT(w, w, y, y), 3, 3, 1, 1)                                         \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(4), SEK_CONCAT(w, w, y, z), 3, 3, 1, 2)                                         \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(4), SEK_CONCAT(w, w, y, w), 3, 3, 1, 3)                                         \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(4), SEK_CONCAT(w, w, z, x), 3, 3, 2, 0)                                         \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(4), SEK_CONCAT(w, w, z, y), 3, 3, 2, 1)                                         \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(4), SEK_CONCAT(w, w, z, z), 3, 3, 2, 2)                                         \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(4), SEK_CONCAT(w, w, z, w), 3, 3, 2, 3)                                         \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(4), SEK_CONCAT(w, w, w, x), 3, 3, 3, 0)                                         \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(4), SEK_CONCAT(w, w, w, y), 3, 3, 3, 1)                                         \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(4), SEK_CONCAT(w, w, w, z), 3, 3, 3, 2)                                         \
	SEK_DETAIL_SHUFFLE_FUNC(SHUFFLE, T(4), SEK_CONCAT(w, w, w, w), 3, 3, 3, 3)

#define SEK_QUATERNION_GENERATE_SHUFFLE(x, y, z, w)                                                                    \
	SEK_DETAIL_SHUFFLE_4(SEK_DETAIL_Q_SHUFFLE, SEK_DETAIL_V_TYPE, x, y, z, w)
