/*
 * Created by switchblade on 2021-12-16
 */

#include <gtest/gtest.h>

#include "sekhmet/math.hpp"

static_assert(std::is_trivially_copyable_v<sek::math::dvec4>);

TEST(math_tests, vector_test)
{
	{
		const sek::math::dvec4 v4d_1 = {1, 0, 1, 0};
		const sek::math::dvec4 v4d_2 = {1, 1, 1, 1};

		const auto cmp = v4d_1 == v4d_2;
		EXPECT_EQ(cmp, sek::math::vec4<bool>(true, false, true, false));
		EXPECT_TRUE(any(cmp));
		EXPECT_FALSE(all(cmp));
		EXPECT_FALSE(none(cmp));
	}
	{
		sek::math::dvec4 v4_1 = {0, 0, 0, 0}, v4_2 = {1, 2, 3, 4};
		auto v4_3 = v4_1 + v4_2;
		EXPECT_EQ(v4_3, v4_2);
		EXPECT_EQ(sek::math::dot(v4_3, v4_2), 1 + 2 * 2 + 3 * 3 + 4 * 4);
		EXPECT_EQ(abs(sek::math::dvec4{-1.0, 2.0, 3.0, 4.0}), (sek::math::dvec4{1.0, 2.0, 3.0, 4.0}));
		EXPECT_EQ(max(v4_3, v4_1), v4_2);
	}
	{
		constexpr sek::math::ivec2 v2i_1 = {1, 0}, v2i_2 = {0, -1};
		EXPECT_EQ(v2i_1 + v2i_2, sek::math::ivec2(1, -1));
		EXPECT_EQ(abs(v2i_1 + v2i_2), sek::math::ivec2(1, 1));
	}
	{
		sek::math::dvec3 v3d_1 = {1, 2, 3};
		EXPECT_EQ(sek::math::dot(v3d_1, v3d_1), 1 + 2 * 2 + 3 * 3);
		auto v3d_2 = sek::math::cross(v3d_1, {4, 5, 6});
		EXPECT_EQ(v3d_2, (sek::math::dvec3{-3, 6, -3}));
	}
	{
		sek::math::fvec3 v3f_1 = {1, 2, 3};
		auto n1 = sek::math::norm(v3f_1);
		auto n2 = v3f_1 / sek::math::magn(v3f_1);
		EXPECT_EQ(n1, n2);

		auto v3f_2 = sek::math::cross(v3f_1, {4, 5, 6});
		EXPECT_EQ(v3f_2, (sek::math::fvec3{-3, 6, -3}));
	}
	{
		sek::math::fvec3 v3f = {1, 2, 3};

		EXPECT_EQ((sek::math::shuffle<2, 1>(v3f)), (sek::math::fvec2{3, 2}));
		EXPECT_EQ((sek::math::shuffle<0, 1, 2, 2>(v3f)), (sek::math::fvec4{1, 2, 3, 3}));
	}
	{
		sek::math::dvec2 v2d = {1, 2};
		EXPECT_EQ((sek::math::shuffle<1, 0>(v2d)), (sek::math::dvec2{2, 1}));
		EXPECT_EQ((sek::math::shuffle<1, 0, 0>(v2d)), (sek::math::dvec3{2, 1, 1}));
	}
	{
		sek::math::dvec2_packed v2dp = {1, 2};
		sek::math::dvec2 v2d = {1, 2};

		EXPECT_EQ(sek::math::dvec2{v2dp}, v2d);
		EXPECT_EQ(sek::math::dvec2_packed{v2d}, v2dp);
	}
	{
		sek::math::dvec4 v4d = {1, 2, 3, 4};
		sek::math::dvec3 v3d = {2, 4, 3};

		EXPECT_EQ(v4d.ywz(), v3d);
		EXPECT_EQ(v4d.argb(), (shuffle<3, 0, 1, 2>(v4d)));
	}
	{
		sek::math::dvec4 v4d = {.1, .2, 3.5, 2.4};
		sek::math::dvec4 v4d_round = {0, 0, 4, 2};
		sek::math::dvec4 v4d_floor = {0, 0, 3, 2};
		sek::math::dvec4 v4d_ceil = {1, 1, 4, 3};

		EXPECT_EQ(round(v4d), v4d_round);
		EXPECT_EQ(floor(v4d), v4d_floor);
		EXPECT_EQ(ceil(v4d), v4d_ceil);
	}
}

TEST(math_tests, matrix_test)
{
	{
		auto m4f_i = sek::math::fmat4{};
		auto m4f_1 = sek::math::fmat4{1};
		EXPECT_EQ(m4f_i, m4f_1);
	}
	{
		auto m2f_1 = sek::math::fmat2{1};
		auto m2f_2 = sek::math::fmat2{1, 2, 2, 1};
		auto m2f_3 = m2f_1 + m2f_2;

		EXPECT_EQ(m2f_3, (sek::math::fmat2{2, 2, 2, 2}));
	}
	{
		EXPECT_EQ(sek::math::transpose(sek::math::fmat3{1}), (sek::math::fmat3{1}));
		EXPECT_EQ(sek::math::transpose(sek::math::fmat3x2{1, 4, 0, 5, 1, 0}), (sek::math::fmat2x3{1, 5, 4, 1, 0, 0}));
	}
	{
		auto m3x2f = sek::math::fmat3x2{0, 4, -2, -4, -3, 0};
		auto m2x3f = sek::math::fmat2x3{0, 1, 1, -1, 2, 3};
		auto m2f = m3x2f * m2x3f;

		EXPECT_EQ(m2f, (sek::math::fmat2{0, -10, -3, -1}));
	}
	{
		auto m3x2f = sek::math::fmat3x2{1, -1, 2, 0, -3, 1};
		auto v3f = sek::math::fvec3{2, 1, 0};
		auto v2f = m3x2f * v3f;

		EXPECT_EQ(v2f, (sek::math::fvec2{1, -3}));
	}
}

TEST(math_tests, quaternion_test)
{
	{
//		const auto rot1 = sek::math::fvec3{60, 30, 90};
//
//		const auto q1 = sek::math::fquat::from_euler(rad(rot1));
//		EXPECT_EQ(sek::math::deg(q1.pitch()), rot1.x());
//		EXPECT_EQ(sek::math::deg(q1.yaw()), rot1.y());
//		EXPECT_EQ(sek::math::deg(q1.roll()), rot1.z());
//
//		const auto m1 = q1.to_matrix();
//		const auto q2 = sek::math::fquat::from_matrix(m1);
//		EXPECT_EQ(q1, q2);
//
//		const auto rot2 = q1.to_euler();
//		const auto rot3 = q2.to_euler();
//
//		EXPECT_EQ(rot2, rot1);
//		EXPECT_EQ(rot3, rot1);
	}
}

TEST(math_tests, random_test)
{
	{
		sek::math::xoroshiro256<uint64_t> r1;
		sek::math::xoroshiro256<uint64_t> r2 = r1;

		EXPECT_EQ(r1, r2);
		EXPECT_EQ(r1(), r2());
		EXPECT_NE(r1(), r1());
	}
	{
		sek::math::xoroshiro128<float> r1;
		sek::math::xoroshiro128<float> r2;

		EXPECT_EQ(r1(), r2());
		EXPECT_NE(r1(), r1());

		std::stringstream ss;
		ss << r1;
		ss >> r2;
		EXPECT_EQ(r1(), r2());
	}
}