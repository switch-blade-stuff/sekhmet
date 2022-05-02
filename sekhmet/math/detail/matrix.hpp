//
// Created by switchblade on 02/05/22.
//

#pragma once

#include "sekhmet/detail/assert.hpp"
#include "sekhmet/detail/hash.hpp"
#include "util.hpp"
#include "vector.hpp"

namespace sek::math
{
	/** Generic matrix.
	 * @note Generic matrix types are not guaranteed to be SIMD-optimized. */
	template<arithmetic T, std::size_t N, std::size_t M>
	union basic_matrix
	{
	private:
		typedef basic_vector<T, M> col_t;
		typedef basic_vector<T, N> row_t;

	private:
		col_t data[N];
	};
}	 // namespace sek::math