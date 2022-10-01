/*
 * Created by switchblade on 2021-12-16
 */

#include <gtest/gtest.h>

#include "sekhmet/math.hpp"

static_assert(std::is_trivially_copyable_v<sek::vec4f>);
static_assert(std::is_trivially_copyable_v<sek::vec2f>);
static_assert(std::is_trivially_copyable_v<sek::vec4d>);
static_assert(std::is_trivially_copyable_v<sek::vec2d>);

TEST(math_tests, vector_test)
{
	{
		const sek::vec4d v4d_1 = {1, 0, 1, 0};
		const sek::vec4d v4d_2 = {1, 1, 1, 1};

		const auto cmp = v4d_1 == v4d_2;
		EXPECT_TRUE(all(cmp == sek::vec4d_mask(true, false, true, false)));
		EXPECT_TRUE(any(cmp));
		EXPECT_FALSE(all(cmp));
		EXPECT_FALSE(none(cmp));
	}
	{
		const sek::vec2d_packed v2dp = {1, 2};
		const sek::vec2d v2d = {1, 2};
		EXPECT_TRUE(all(sek::vec2d{v2dp} == v2d));
		EXPECT_TRUE(all(sek::vec2d_packed{v2d} == v2dp));
	}
	{
		const sek::vec4i_mask mask = {true, false, true, false};
		const sek::vec4i v4d_1 = {0xaa, 0xaa, 0xbb, 0xbb};
		const sek::vec4i v4d_2 = {0xcc, 0xcc, 0xdd, 0xdd};
		const auto v4d_3 = interleave(v4d_1, v4d_2, mask);

		EXPECT_FALSE(all(v4d_1 == v4d_3));
		EXPECT_FALSE(all(v4d_2 == v4d_3));
		EXPECT_TRUE(all(v4d_3 == sek::vec4i{0xaa, 0xcc, 0xbb, 0xdd}));
	}
	{
		const sek::vec4d v4_1 = {0, 0, 0, 0}, v4_2 = {1, 2, 3, 4};
		const auto v4_3 = v4_1 + v4_2;
		EXPECT_TRUE(all(v4_3 == v4_2));
		EXPECT_EQ(sek::dot(v4_3, v4_2), 1 + 2 * 2 + 3 * 3 + 4 * 4);

		const auto v4_4 = abs(sek::vec4d{-1.0, 2.0, 3.0, 4.0});
		const auto v4_5 = max(v4_3, v4_1);

		EXPECT_TRUE(all(v4_4 == sek::vec4d{1.0, 2.0, 3.0, 4.0}));
		EXPECT_TRUE(all(v4_5 == v4_2));
	}
	{
		constexpr sek::vec2i v2i_1 = {1, 0}, v2i_2 = {0, -1};
		EXPECT_TRUE(all(v2i_1 + v2i_2 == sek::vec2i(1, -1)));
		EXPECT_TRUE(all(abs(v2i_1 + v2i_2) == sek::vec2i(1, 1)));
	}
	{
		const sek::vec3d v3d_1 = {1, 2, 3};
		EXPECT_EQ(dot(v3d_1, v3d_1), 1 + 2 * 2 + 3 * 3);
		const auto v3d_2 = cross(v3d_1, {4, 5, 6});
		EXPECT_TRUE(all(v3d_2 == (sek::vec3d{-3, 6, -3})));
	}
	{
		sek::vec3f v3f_1 = {1, 2, 3};
		const auto n1 = norm(v3f_1);
		const auto n2 = v3f_1 / magn(v3f_1);
		EXPECT_TRUE(all(n1 == n2));

		const auto v3f_2 = cross(v3f_1, {4, 5, 6});
		EXPECT_TRUE(all(v3f_2 == sek::vec3f{-3, 6, -3}));
	}
	{
		const sek::vec3f v3f = {1, 2, 3};
		EXPECT_TRUE(all(shuffle<2, 1>(v3f) == sek::vec2f{3, 2}));
		EXPECT_TRUE(all(shuffle<0, 1, 2, 2>(v3f) == sek::vec4f{1, 2, 3, 3}));
	}
	{
		const sek::vec2d v2d = {1, 2};
		EXPECT_TRUE(all(shuffle<1, 0>(v2d) == sek::vec2d{2, 1}));
		EXPECT_TRUE(all(shuffle<1, 0, 0>(v2d) == sek::vec3d{2, 1, 1}));
	}
	{
		const sek::vec4d v4d = {1, 2, 3, 4};
		const sek::vec3d v3d = {2, 4, 3};

		EXPECT_TRUE(all(v4d.ywz() == v3d));
		EXPECT_TRUE(all(v4d.argb() == shuffle<3, 0, 1, 2>(v4d)));
	}
	{
		const sek::vec4d v4d = {.1, .2, 3.5, 2.4};
		const sek::vec4d v4d_round = {0, 0, 4, 2};
		const sek::vec4d v4d_floor = {0, 0, 3, 2};
		const sek::vec4d v4d_ceil = {1, 1, 4, 3};

		EXPECT_TRUE(all(round(v4d) == v4d_round));
		EXPECT_TRUE(all(floor(v4d) == v4d_floor));
		EXPECT_TRUE(all(ceil(v4d) == v4d_ceil));
	}
	{
		const sek::vec4d v4d_val = {0.1, 2.1, 3.1, -4};
		const sek::vec4d v4d_min = {0, 0, 1, -10};
		const sek::vec4d v4d_max = {1, 1, 2, 0};
		const auto res = fclamp(v4d_val, v4d_min, v4d_max);

		EXPECT_TRUE(all(fcmp_eq(res, sek::vec4d{0.1, 1, 2, -4})));
	}
	{
		const auto v4d_nan = sek::vec4d{std::numeric_limits<double>::quiet_NaN()};
		const auto v4f_nan = sek::vec4f{std::numeric_limits<float>::quiet_NaN()};

		EXPECT_TRUE(all(is_nan(v4d_nan)));
		EXPECT_TRUE(all(is_nan(v4f_nan)));
		EXPECT_FALSE(all(is_inf(v4d_nan)));
		EXPECT_FALSE(all(is_inf(v4f_nan)));
		EXPECT_FALSE(all(is_fin(v4d_nan)));
		EXPECT_FALSE(all(is_fin(v4f_nan)));
		EXPECT_FALSE(all(is_neg(v4f_nan)));
		EXPECT_FALSE(all(is_neg(v4f_nan)));
		EXPECT_FALSE(all(is_norm(v4f_nan)));
		EXPECT_FALSE(all(is_norm(v4f_nan)));

		const auto v4d_inf = sek::vec4d{std::numeric_limits<double>::infinity()};
		const auto v4f_inf = sek::vec4f{std::numeric_limits<float>::infinity()};

		EXPECT_FALSE(all(is_nan(v4d_inf)));
		EXPECT_FALSE(all(is_nan(v4f_inf)));
		EXPECT_TRUE(all(is_inf(v4d_inf)));
		EXPECT_TRUE(all(is_inf(v4f_inf)));
		EXPECT_FALSE(all(is_fin(v4d_inf)));
		EXPECT_FALSE(all(is_fin(v4f_inf)));
		EXPECT_FALSE(all(is_neg(v4f_inf)));
		EXPECT_FALSE(all(is_neg(v4f_inf)));
		EXPECT_FALSE(all(is_norm(v4f_inf)));
		EXPECT_FALSE(all(is_norm(v4f_inf)));

		const auto v4d_zero = sek::vec4d{0};
		const auto v4f_zero = sek::vec4f{0};

		EXPECT_FALSE(all(is_nan(v4d_zero)));
		EXPECT_FALSE(all(is_nan(v4f_zero)));
		EXPECT_FALSE(all(is_inf(v4d_zero)));
		EXPECT_FALSE(all(is_inf(v4f_zero)));
		EXPECT_TRUE(all(is_fin(v4d_zero)));
		EXPECT_TRUE(all(is_fin(v4f_zero)));
		EXPECT_FALSE(all(is_neg(v4d_zero)));
		EXPECT_FALSE(all(is_neg(v4f_zero)));
		EXPECT_FALSE(all(is_norm(v4d_zero)));
		EXPECT_FALSE(all(is_norm(v4f_zero)));

		const auto v4d_mzero = sek::vec4d{-0.};
		const auto v4f_mzero = sek::vec4f{-0.};

		EXPECT_FALSE(all(is_nan(v4d_mzero)));
		EXPECT_FALSE(all(is_nan(v4f_mzero)));
		EXPECT_FALSE(all(is_inf(v4d_mzero)));
		EXPECT_FALSE(all(is_inf(v4f_mzero)));
		EXPECT_TRUE(all(is_fin(v4d_mzero)));
		EXPECT_TRUE(all(is_fin(v4f_mzero)));
		EXPECT_TRUE(all(is_neg(v4d_mzero)));
		EXPECT_TRUE(all(is_neg(v4f_mzero)));
		EXPECT_FALSE(all(is_norm(v4d_mzero)));
		EXPECT_FALSE(all(is_norm(v4f_mzero)));

		const auto v4d_one = sek::vec4d{1.};
		const auto v4f_one = sek::vec4f{1.};

		EXPECT_FALSE(all(is_nan(v4d_one)));
		EXPECT_FALSE(all(is_nan(v4f_one)));
		EXPECT_FALSE(all(is_inf(v4d_one)));
		EXPECT_FALSE(all(is_inf(v4f_one)));
		EXPECT_TRUE(all(is_fin(v4d_one)));
		EXPECT_TRUE(all(is_fin(v4f_one)));
		EXPECT_FALSE(all(is_neg(v4d_one)));
		EXPECT_FALSE(all(is_neg(v4f_one)));
		EXPECT_TRUE(all(is_norm(v4d_one)));
		EXPECT_TRUE(all(is_norm(v4f_one)));

		const auto v4d_mone = sek::vec4d{-1.};
		const auto v4f_mone = sek::vec4f{-1.};

		EXPECT_FALSE(all(is_nan(v4d_mone)));
		EXPECT_FALSE(all(is_nan(v4f_mone)));
		EXPECT_FALSE(all(is_inf(v4d_mone)));
		EXPECT_FALSE(all(is_inf(v4f_mone)));
		EXPECT_TRUE(all(is_fin(v4d_mone)));
		EXPECT_TRUE(all(is_fin(v4f_mone)));
		EXPECT_TRUE(all(is_neg(v4d_mone)));
		EXPECT_TRUE(all(is_neg(v4f_mone)));
		EXPECT_TRUE(all(is_norm(v4d_mone)));
		EXPECT_TRUE(all(is_norm(v4f_mone)));
	}
	{
		const auto v4f_0 = sek::vec4f{2};
		const auto v4f_1 = sek::vec4f{4};
		const auto v4f_2 = sek::vec4f{1};

		auto v4f_3 = fmadd(v4f_0, v4f_1, v4f_2);
		auto v4f_4 = (v4f_0 * v4f_1) + v4f_2;
		EXPECT_TRUE(all(v4f_3 == v4f_4));

		v4f_3 = fmsub(v4f_0, v4f_1, v4f_2);
		v4f_4 = (v4f_0 * v4f_1) - v4f_2;
		EXPECT_TRUE(all(v4f_3 == v4f_4));
	}
}

TEST(math_tests, exp_test)
{
	constexpr auto exec_test = [](auto test_func)
	{
		test_func(0.0f);
		test_func(1.0f);
		test_func(2.0f);
		test_func(3.0f);
		test_func(4.0f);
		test_func(4.5f);
	};

	exec_test(
		[](float x)
		{
			auto v4f_0 = sek::vec4f{std::exp(x)};
			auto v4f_1 = exp(sek::vec4f{x});
			EXPECT_TRUE(all(fcmp_eq(v4f_0, v4f_1, 0.0001f)));
		});
	exec_test(
		[](float x)
		{
			auto v2f_0 = sek::vec2f{std::exp(x)};
			auto v2f_1 = exp(sek::vec2f{x});
			EXPECT_TRUE(all(fcmp_eq(v2f_0, v2f_1, 0.0001f)));
		});
	exec_test(
		[](double x)
		{
			auto v4d_0 = sek::vec4d{std::exp(x)};
			auto v4d_1 = exp(sek::vec4d{x});
			EXPECT_TRUE(all(fcmp_eq(v4d_0, v4d_1, 0.000001)));
		});
	exec_test(
		[](double x)
		{
			auto v2d_0 = sek::vec2d{std::exp(x)};
			auto v2d_1 = exp(sek::vec2d{x});
			EXPECT_TRUE(all(fcmp_eq(v2d_0, v2d_1, 0.000001)));
		});
}
TEST(math_tests, exp2_test)
{
	constexpr auto exec_test = [](auto test_func)
	{
		test_func(0.0f);
		test_func(1.0f);
		test_func(2.0f);
		test_func(3.0f);
		test_func(4.0f);
		test_func(4.5f);
	};

	exec_test(
		[](float x)
		{
			auto v4f_0 = sek::vec4f{std::exp2(x)};
			auto v4f_1 = exp2(sek::vec4f{x});
			EXPECT_TRUE(all(fcmp_eq(v4f_0, v4f_1, 0.0001f)));
		});
	exec_test(
		[](float x)
		{
			auto v2f_0 = sek::vec2f{std::exp2(x)};
			auto v2f_1 = exp2(sek::vec2f{x});
			EXPECT_TRUE(all(fcmp_eq(v2f_0, v2f_1, 0.0001f)));
		});
	exec_test(
		[](double x)
		{
			auto v4d_0 = sek::vec4d{std::exp2(x)};
			auto v4d_1 = exp2(sek::vec4d{x});
			EXPECT_TRUE(all(fcmp_eq(v4d_0, v4d_1, 0.000001)));
		});
	exec_test(
		[](double x)
		{
			auto v2d_0 = sek::vec2d{std::exp2(x)};
			auto v2d_1 = exp2(sek::vec2d{x});
			EXPECT_TRUE(all(fcmp_eq(v2d_0, v2d_1, 0.000001)));
		});
}
TEST(math_tests, expm1_test)
{
	constexpr auto exec_test = [](auto test_func)
	{
		test_func(0.0f);
		test_func(1.0f);
		test_func(2.0f);
		test_func(3.0f);
		test_func(4.0f);
		test_func(4.5f);
	};

	exec_test(
		[](float x)
		{
			auto v4f_0 = sek::vec4f{std::expm1(x)};
			auto v4f_1 = expm1(sek::vec4f{x});
			EXPECT_TRUE(all(fcmp_eq(v4f_0, v4f_1, 0.0001f)));
		});
	exec_test(
		[](float x)
		{
			auto v2f_0 = sek::vec2f{std::expm1(x)};
			auto v2f_1 = expm1(sek::vec2f{x});
			EXPECT_TRUE(all(fcmp_eq(v2f_0, v2f_1, 0.0001f)));
		});
	exec_test(
		[](double x)
		{
			auto v4d_0 = sek::vec4d{std::expm1(x)};
			auto v4d_1 = expm1(sek::vec4d{x});
			EXPECT_TRUE(all(fcmp_eq(v4d_0, v4d_1, 0.000001)));
		});
	exec_test(
		[](double x)
		{
			auto v2d_0 = sek::vec2d{std::expm1(x)};
			auto v2d_1 = expm1(sek::vec2d{x});
			EXPECT_TRUE(all(fcmp_eq(v2d_0, v2d_1, 0.000001)));
		});
}

TEST(math_tests, log_test)
{
	constexpr auto exec_test = [](auto test_func)
	{
		test_func(std::exp(-0.5f));
		test_func(std::exp(0.0f));
		test_func(std::exp(0.5f));
		test_func(std::exp(1.0f));
		test_func(std::exp(2.0f));
		test_func(std::exp(3.0f));
		test_func(std::exp(4.0f));
		test_func(std::exp(4.5f));
	};

	exec_test(
		[](float x)
		{
			auto v4f_0 = sek::vec4f{std::log(x)};
			auto v4f_1 = log(sek::vec4f{x});
			EXPECT_TRUE(all(fcmp_eq(v4f_0, v4f_1, 0.0001f)));
		});
	exec_test(
		[](float x)
		{
			auto v2f_0 = sek::vec2f{std::log(x)};
			auto v2f_1 = log(sek::vec2f{x});
			EXPECT_TRUE(all(fcmp_eq(v2f_0, v2f_1, 0.0001f)));
		});
	exec_test(
		[](double x)
		{
			auto v4d_0 = sek::vec4d{std::log(x)};
			auto v4d_1 = log(sek::vec4d{x});
			EXPECT_TRUE(all(fcmp_eq(v4d_0, v4d_1, 0.000001)));
		});
	exec_test(
		[](double x)
		{
			auto v2d_0 = sek::vec2d{std::log(x)};
			auto v2d_1 = log(sek::vec2d{x});
			EXPECT_TRUE(all(fcmp_eq(v2d_0, v2d_1, 0.000001)));
		});
}
TEST(math_tests, log1p_test)
{
	constexpr auto exec_test = [](auto test_func)
	{
		test_func(std::exp(-0.5f));
		test_func(std::exp(0.0f));
		test_func(std::exp(0.5f));
		test_func(std::exp(1.0f));
		test_func(std::exp(2.0f));
		test_func(std::exp(3.0f));
		test_func(std::exp(4.0f));
		test_func(std::exp(4.5f));
	};

	exec_test(
		[](float x)
		{
			auto v4f_0 = sek::vec4f{std::log1p(x)};
			auto v4f_1 = log1p(sek::vec4f{x});
			EXPECT_TRUE(all(fcmp_eq(v4f_0, v4f_1, 0.0001f)));
		});
	exec_test(
		[](float x)
		{
			auto v2f_0 = sek::vec2f{std::log1p(x)};
			auto v2f_1 = log1p(sek::vec2f{x});
			EXPECT_TRUE(all(fcmp_eq(v2f_0, v2f_1, 0.0001f)));
		});
	exec_test(
		[](double x)
		{
			auto v4d_0 = sek::vec4d{std::log1p(x)};
			auto v4d_1 = log1p(sek::vec4d{x});
			EXPECT_TRUE(all(fcmp_eq(v4d_0, v4d_1, 0.000001)));
		});
	exec_test(
		[](double x)
		{
			auto v2d_0 = sek::vec2d{std::log1p(x)};
			auto v2d_1 = log1p(sek::vec2d{x});
			EXPECT_TRUE(all(fcmp_eq(v2d_0, v2d_1, 0.000001)));
		});
}
TEST(math_tests, log2_test)
{
	constexpr auto exec_test = [](auto test_func)
	{
		test_func(std::exp(-0.5f));
		test_func(std::exp(0.0f));
		test_func(std::exp(0.5f));
		test_func(std::exp(1.0f));
		test_func(std::exp(2.0f));
		test_func(std::exp(3.0f));
		test_func(std::exp(4.0f));
		test_func(std::exp(4.5f));
	};

	exec_test(
		[](float x)
		{
			auto v4f_0 = sek::vec4f{std::log2(x)};
			auto v4f_1 = log2(sek::vec4f{x});
			EXPECT_TRUE(all(fcmp_eq(v4f_0, v4f_1, 0.0001f)));
		});
	exec_test(
		[](float x)
		{
			auto v2f_0 = sek::vec2f{std::log2(x)};
			auto v2f_1 = log2(sek::vec2f{x});
			EXPECT_TRUE(all(fcmp_eq(v2f_0, v2f_1, 0.0001f)));
		});
	exec_test(
		[](double x)
		{
			auto v4d_0 = sek::vec4d{std::log2(x)};
			auto v4d_1 = log2(sek::vec4d{x});
			EXPECT_TRUE(all(fcmp_eq(v4d_0, v4d_1, 0.000001)));
		});
	exec_test(
		[](double x)
		{
			auto v2d_0 = sek::vec2d{std::log2(x)};
			auto v2d_1 = log2(sek::vec2d{x});
			EXPECT_TRUE(all(fcmp_eq(v2d_0, v2d_1, 0.000001)));
		});
}
TEST(math_tests, log10_test)
{
	constexpr auto exec_test = [](auto test_func)
	{
		test_func(std::exp(-0.5f));
		test_func(std::exp(0.0f));
		test_func(std::exp(0.5f));
		test_func(std::exp(1.0f));
		test_func(std::exp(2.0f));
		test_func(std::exp(3.0f));
		test_func(std::exp(4.0f));
		test_func(std::exp(4.5f));
	};

	exec_test(
		[](float x)
		{
			auto v4f_0 = sek::vec4f{std::log10(x)};
			auto v4f_1 = log10(sek::vec4f{x});
			EXPECT_TRUE(all(fcmp_eq(v4f_0, v4f_1, 0.0001f)));
		});
	exec_test(
		[](float x)
		{
			auto v2f_0 = sek::vec2f{std::log10(x)};
			auto v2f_1 = log10(sek::vec2f{x});
			EXPECT_TRUE(all(fcmp_eq(v2f_0, v2f_1, 0.0001f)));
		});
	exec_test(
		[](double x)
		{
			auto v4d_0 = sek::vec4d{std::log10(x)};
			auto v4d_1 = log10(sek::vec4d{x});
			EXPECT_TRUE(all(fcmp_eq(v4d_0, v4d_1, 0.000001)));
		});
	exec_test(
		[](double x)
		{
			auto v2d_0 = sek::vec2d{std::log10(x)};
			auto v2d_1 = log10(sek::vec2d{x});
			EXPECT_TRUE(all(fcmp_eq(v2d_0, v2d_1, 0.000001)));
		});
}

TEST(math_tests, sin_test)
{
	constexpr auto exec_test = [](auto test_func)
	{
		test_func(sek::rad(0.0f));
		test_func(sek::rad(45.0f));
		test_func(sek::rad(90.0f));
		test_func(sek::rad(135.0f));
		test_func(sek::rad(180.0f));
		test_func(sek::rad(225.0f));
		test_func(sek::rad(270.0f));
		test_func(sek::rad(315.0f));
		test_func(sek::rad(360.0f));
	};

	exec_test(
		[](float angle)
		{
			auto v4f_0 = sek::vec4f{std::sin(angle)};
			auto v4f_1 = sin(sek::vec4f{angle});
			EXPECT_TRUE(all(fcmp_eq(v4f_0, v4f_1, 0.0001f)));
		});
	exec_test(
		[](float angle)
		{
			auto v2f_0 = sek::vec2f{std::sin(angle)};
			auto v2f_1 = sin(sek::vec2f{angle});
			EXPECT_TRUE(all(fcmp_eq(v2f_0, v2f_1, 0.0001f)));
		});
	exec_test(
		[](double angle)
		{
			auto v4d_0 = sek::vec4d{std::sin(angle)};
			auto v4d_1 = sin(sek::vec4d{angle});
			EXPECT_TRUE(all(fcmp_eq(v4d_0, v4d_1, 0.000001)));
		});
	exec_test(
		[](double angle)
		{
			auto v2d_0 = sek::vec2d{std::sin(angle)};
			auto v2d_1 = sin(sek::vec2d{angle});
			EXPECT_TRUE(all(fcmp_eq(v2d_0, v2d_1, 0.000001)));
		});
}
TEST(math_tests, cos_test)
{
	constexpr auto exec_test = [](auto test_func)
	{
		test_func(sek::rad(0.0f));
		test_func(sek::rad(45.0f));
		test_func(sek::rad(90.0f));
		test_func(sek::rad(135.0f));
		test_func(sek::rad(180.0f));
		test_func(sek::rad(225.0f));
		test_func(sek::rad(270.0f));
		test_func(sek::rad(315.0f));
		test_func(sek::rad(360.0f));
	};

	exec_test(
		[](float angle)
		{
			auto v4f_0 = sek::vec4f{std::cos(angle)};
			auto v4f_1 = cos(sek::vec4f{angle});
			EXPECT_TRUE(all(fcmp_eq(v4f_0, v4f_1, 0.0001f)));
		});
	exec_test(
		[](float angle)
		{
			auto v2f_0 = sek::vec2f{std::cos(angle)};
			auto v2f_1 = cos(sek::vec2f{angle});
			EXPECT_TRUE(all(fcmp_eq(v2f_0, v2f_1, 0.0001f)));
		});
	exec_test(
		[](double angle)
		{
			auto v4d_0 = sek::vec4d{std::cos(angle)};
			auto v4d_1 = cos(sek::vec4d{angle});
			EXPECT_TRUE(all(fcmp_eq(v4d_0, v4d_1, 0.000001)));
		});
	exec_test(
		[](double angle)
		{
			auto v2d_0 = sek::vec2d{std::cos(angle)};
			auto v2d_1 = cos(sek::vec2d{angle});
			EXPECT_TRUE(all(fcmp_eq(v2d_0, v2d_1, 0.000001)));
		});
}
TEST(math_tests, tan_test)
{
	constexpr auto exec_test = [](auto test_func)
	{
		test_func(sek::rad(0.0f));
		test_func(sek::rad(45.0f));
		test_func(sek::rad(135.0f));
		test_func(sek::rad(180.0f));
		test_func(sek::rad(225.0f));
		test_func(sek::rad(315.0f));
		test_func(sek::rad(360.0f));
	};

	exec_test(
		[](float angle)
		{
			auto v4f_0 = sek::vec4f{std::tan(angle)};
			auto v4f_1 = tan(sek::vec4f{angle});
			EXPECT_TRUE(all(fcmp_eq(v4f_0, v4f_1, 0.0001f)));
		});
	exec_test(
		[](float angle)
		{
			auto v2f_0 = sek::vec2f{std::tan(angle)};
			auto v2f_1 = tan(sek::vec2f{angle});
			EXPECT_TRUE(all(fcmp_eq(v2f_0, v2f_1, 0.0001f)));
		});
	exec_test(
		[](double angle)
		{
			auto v4d_0 = sek::vec4d{std::tan(angle)};
			auto v4d_1 = tan(sek::vec4d{angle});
			EXPECT_TRUE(all(fcmp_eq(v4d_0, v4d_1, 0.000001)));
		});
	exec_test(
		[](double angle)
		{
			auto v2d_0 = sek::vec2d{std::tan(angle)};
			auto v2d_1 = tan(sek::vec2d{angle});
			EXPECT_TRUE(all(fcmp_eq(v2d_0, v2d_1, 0.000001)));
		});
}
TEST(math_tests, cot_test)
{
	constexpr auto exec_test = [](auto test_func)
	{
		test_func(sek::rad(45.0f));
		test_func(sek::rad(90.0f));
		test_func(sek::rad(135.0f));
		test_func(sek::rad(225.0f));
		test_func(sek::rad(270.0f));
		test_func(sek::rad(315.0f));
	};

	exec_test(
		[](float angle)
		{
			auto v4f_0 = sek::vec4f{1 / std::tan(angle)};
			auto v4f_1 = cot(sek::vec4f{angle});
			EXPECT_TRUE(all(fcmp_eq(v4f_0, v4f_1, 0.0001f)));
		});
	exec_test(
		[](float angle)
		{
			auto v2f_0 = sek::vec2f{1 / std::tan(angle)};
			auto v2f_1 = cot(sek::vec2f{angle});
			EXPECT_TRUE(all(fcmp_eq(v2f_0, v2f_1, 0.0001f)));
		});
	exec_test(
		[](double angle)
		{
			auto v4d_0 = sek::vec4d{1 / std::tan(angle)};
			auto v4d_1 = cot(sek::vec4d{angle});
			EXPECT_TRUE(all(fcmp_eq(v4d_0, v4d_1, 0.000001)));
		});
	exec_test(
		[](double angle)
		{
			auto v2d_0 = sek::vec2d{1 / std::tan(angle)};
			auto v2d_1 = cot(sek::vec2d{angle});
			EXPECT_TRUE(all(fcmp_eq(v2d_0, v2d_1, 0.000001)));
		});
}

TEST(math_tests, asin_test)
{
	constexpr auto exec_test = [](auto test_func)
	{
		test_func(-1.0f);
		test_func(-0.5f);
		test_func(0.0f);
		test_func(0.5f);
		test_func(1.0f);
	};

	exec_test(
		[](float angle)
		{
			auto v4f_0 = sek::vec4f{std::asin(angle)};
			auto v4f_1 = asin(sek::vec4f{angle});
			EXPECT_TRUE(all(fcmp_eq(v4f_0, v4f_1, 0.0001f)));
		});
	exec_test(
		[](float angle)
		{
			auto v2f_0 = sek::vec2f{std::asin(angle)};
			auto v2f_1 = asin(sek::vec2f{angle});
			EXPECT_TRUE(all(fcmp_eq(v2f_0, v2f_1, 0.0001f)));
		});
	exec_test(
		[](double angle)
		{
			auto v4d_0 = sek::vec4d{std::asin(angle)};
			auto v4d_1 = asin(sek::vec4d{angle});
			EXPECT_TRUE(all(fcmp_eq(v4d_0, v4d_1, 0.000001)));
		});
	exec_test(
		[](double angle)
		{
			auto v2d_0 = sek::vec2d{std::asin(angle)};
			auto v2d_1 = asin(sek::vec2d{angle});
			EXPECT_TRUE(all(fcmp_eq(v2d_0, v2d_1, 0.000001)));
		});
}
TEST(math_tests, acos_test)
{
	constexpr auto exec_test = [](auto test_func)
	{
		test_func(-1.0f);
		test_func(-0.5f);
		test_func(0.0f);
		test_func(0.5f);
		test_func(1.0f);
	};

	exec_test(
		[](float angle)
		{
			auto v4f_0 = sek::vec4f{std::acos(angle)};
			auto v4f_1 = acos(sek::vec4f{angle});
			EXPECT_TRUE(all(fcmp_eq(v4f_0, v4f_1, 0.0001f)));
		});
	exec_test(
		[](float angle)
		{
			auto v2f_0 = sek::vec2f{std::acos(angle)};
			auto v2f_1 = acos(sek::vec2f{angle});
			EXPECT_TRUE(all(fcmp_eq(v2f_0, v2f_1, 0.0001f)));
		});
	exec_test(
		[](double angle)
		{
			auto v4d_0 = sek::vec4d{std::acos(angle)};
			auto v4d_1 = acos(sek::vec4d{angle});
			EXPECT_TRUE(all(fcmp_eq(v4d_0, v4d_1, 0.000001)));
		});
	exec_test(
		[](double angle)
		{
			auto v2d_0 = sek::vec2d{std::acos(angle)};
			auto v2d_1 = acos(sek::vec2d{angle});
			EXPECT_TRUE(all(fcmp_eq(v2d_0, v2d_1, 0.000001)));
		});
}
TEST(math_tests, atan_test)
{
	constexpr auto exec_test = [](auto test_func)
	{
		test_func(sek::rad(0.0f));
		test_func(sek::rad(45.0f));
		test_func(sek::rad(90.0f));
		test_func(sek::rad(135.0f));
		test_func(sek::rad(180.0f));
		test_func(sek::rad(225.0f));
		test_func(sek::rad(270.0f));
		test_func(sek::rad(315.0f));
		test_func(sek::rad(360.0f));
	};

	exec_test(
		[](float angle)
		{
			auto v4f_0 = sek::vec4f{std::atan(angle)};
			auto v4f_1 = atan(sek::vec4f{angle});
			EXPECT_TRUE(all(fcmp_eq(v4f_0, v4f_1, 0.001f)));
		});
	exec_test(
		[](float angle)
		{
			auto v2f_0 = sek::vec2f{std::atan(angle)};
			auto v2f_1 = atan(sek::vec2f{angle});
			EXPECT_TRUE(all(fcmp_eq(v2f_0, v2f_1, 0.001f)));
		});
	exec_test(
		[](double angle)
		{
			auto v4d_0 = sek::vec4d{std::atan(angle)};
			auto v4d_1 = atan(sek::vec4d{angle});
			EXPECT_TRUE(all(fcmp_eq(v4d_0, v4d_1, 0.000001)));
		});
	exec_test(
		[](double angle)
		{
			auto v2d_0 = sek::vec2d{std::atan(angle)};
			auto v2d_1 = atan(sek::vec2d{angle});
			EXPECT_TRUE(all(fcmp_eq(v2d_0, v2d_1, 0.000001)));
		});
}
TEST(math_tests, acot_test)
{
	constexpr auto exec_test = [](auto test_func)
	{
		test_func(sek::rad(0.0f));
		test_func(sek::rad(45.0f));
		test_func(sek::rad(90.0f));
		test_func(sek::rad(135.0f));
		test_func(sek::rad(180.0f));
		test_func(sek::rad(225.0f));
		test_func(sek::rad(270.0f));
		test_func(sek::rad(315.0f));
		test_func(sek::rad(360.0f));
	};

	exec_test(
		[](float angle)
		{
			auto v4f_0 = sek::vec4f{std::numbers::pi_v<float> / 2 - std::atan(angle)};
			auto v4f_1 = acot(sek::vec4f{angle});
			EXPECT_TRUE(all(fcmp_eq(v4f_0, v4f_1, 0.001f)));
		});
	exec_test(
		[](float angle)
		{
			auto v2f_0 = sek::vec2f{std::numbers::pi_v<float> / 2 - std::atan(angle)};
			auto v2f_1 = acot(sek::vec2f{angle});
			EXPECT_TRUE(all(fcmp_eq(v2f_0, v2f_1, 0.001f)));
		});
	exec_test(
		[](double angle)
		{
			auto v4d_0 = sek::vec4d{std::numbers::pi_v<double> / 2 - std::atan(angle)};
			auto v4d_1 = acot(sek::vec4d{angle});
			EXPECT_TRUE(all(fcmp_eq(v4d_0, v4d_1, 0.000001)));
		});
	exec_test(
		[](double angle)
		{
			auto v2d_0 = sek::vec2d{std::numbers::pi_v<double> / 2 - std::atan(angle)};
			auto v2d_1 = acot(sek::vec2d{angle});
			EXPECT_TRUE(all(fcmp_eq(v2d_0, v2d_1, 0.000001)));
		});
}

TEST(math_tests, sinh_test)
{
	constexpr auto exec_test = [](auto test_func)
	{
		test_func(sek::rad(45.0f));
		test_func(sek::rad(90.0f));
		test_func(sek::rad(135.0f));
		test_func(sek::rad(180.0f));
		test_func(sek::rad(225.0f));
		test_func(sek::rad(270.0f));
		test_func(sek::rad(315.0f));
		test_func(sek::rad(360.0f));
	};

	exec_test(
		[](float angle)
		{
			auto v4f_0 = sek::vec4f{std::sinh(angle)};
			auto v4f_1 = sinh(sek::vec4f{angle});
			EXPECT_TRUE(all(fcmp_eq(v4f_0, v4f_1, 0.0001f)));
		});
	exec_test(
		[](float angle)
		{
			auto v2f_0 = sek::vec2f{std::sinh(angle)};
			auto v2f_1 = sinh(sek::vec2f{angle});
			EXPECT_TRUE(all(fcmp_eq(v2f_0, v2f_1, 0.0001f)));
		});
	exec_test(
		[](double angle)
		{
			auto v4d_0 = sek::vec4d{std::sinh(angle)};
			auto v4d_1 = sinh(sek::vec4d{angle});
			EXPECT_TRUE(all(fcmp_eq(v4d_0, v4d_1, 0.000001)));
		});
	exec_test(
		[](double angle)
		{
			auto v2d_0 = sek::vec2d{std::sinh(angle)};
			auto v2d_1 = sinh(sek::vec2d{angle});
			EXPECT_TRUE(all(fcmp_eq(v2d_0, v2d_1, 0.000001)));
		});
}
TEST(math_tests, cosh_test)
{
	constexpr auto exec_test = [](auto test_func)
	{
		test_func(sek::rad(45.0f));
		test_func(sek::rad(90.0f));
		test_func(sek::rad(135.0f));
		test_func(sek::rad(180.0f));
		test_func(sek::rad(225.0f));
		test_func(sek::rad(270.0f));
		test_func(sek::rad(315.0f));
		test_func(sek::rad(360.0f));
	};

	exec_test(
		[](float angle)
		{
			auto v4f_0 = sek::vec4f{std::cosh(angle)};
			auto v4f_1 = cosh(sek::vec4f{angle});
			EXPECT_TRUE(all(fcmp_eq(v4f_0, v4f_1, 0.0001f)));
		});
	exec_test(
		[](float angle)
		{
			auto v2f_0 = sek::vec2f{std::cosh(angle)};
			auto v2f_1 = cosh(sek::vec2f{angle});
			EXPECT_TRUE(all(fcmp_eq(v2f_0, v2f_1, 0.0001f)));
		});
	exec_test(
		[](double angle)
		{
			auto v4d_0 = sek::vec4d{std::cosh(angle)};
			auto v4d_1 = cosh(sek::vec4d{angle});
			EXPECT_TRUE(all(fcmp_eq(v4d_0, v4d_1, 0.000001)));
		});
	exec_test(
		[](double angle)
		{
			auto v2d_0 = sek::vec2d{std::cosh(angle)};
			auto v2d_1 = cosh(sek::vec2d{angle});
			EXPECT_TRUE(all(fcmp_eq(v2d_0, v2d_1, 0.000001)));
		});
}
TEST(math_tests, tanh_test)
{
	constexpr auto exec_test = [](auto test_func)
	{
		test_func(sek::rad(45.0f));
		test_func(sek::rad(135.0f));
		test_func(sek::rad(180.0f));
		test_func(sek::rad(225.0f));
		test_func(sek::rad(315.0f));
		test_func(sek::rad(360.0f));
	};

	exec_test(
		[](float angle)
		{
			auto v4f_0 = sek::vec4f{std::tanh(angle)};
			auto v4f_1 = tanh(sek::vec4f{angle});
			EXPECT_TRUE(all(fcmp_eq(v4f_0, v4f_1, 0.0001f)));
		});
	exec_test(
		[](float angle)
		{
			auto v2f_0 = sek::vec2f{std::tanh(angle)};
			auto v2f_1 = tanh(sek::vec2f{angle});
			EXPECT_TRUE(all(fcmp_eq(v2f_0, v2f_1, 0.0001f)));
		});
	exec_test(
		[](double angle)
		{
			auto v4d_0 = sek::vec4d{std::tanh(angle)};
			auto v4d_1 = tanh(sek::vec4d{angle});
			EXPECT_TRUE(all(fcmp_eq(v4d_0, v4d_1, 0.000001)));
		});
	exec_test(
		[](double angle)
		{
			auto v2d_0 = sek::vec2d{std::tanh(angle)};
			auto v2d_1 = tanh(sek::vec2d{angle});
			EXPECT_TRUE(all(fcmp_eq(v2d_0, v2d_1, 0.000001)));
		});
}
TEST(math_tests, coth_test)
{
	constexpr auto exec_test = [](auto test_func)
	{
		test_func(sek::rad(45.0f));
		test_func(sek::rad(135.0f));
		test_func(sek::rad(180.0f));
		test_func(sek::rad(225.0f));
		test_func(sek::rad(315.0f));
		test_func(sek::rad(360.0f));
	};

	exec_test(
		[](float angle)
		{
			auto v4f_0 = sek::vec4f{1 / std::tanh(angle)};
			auto v4f_1 = coth(sek::vec4f{angle});
			EXPECT_TRUE(all(fcmp_eq(v4f_0, v4f_1, 0.0001f)));
		});
	exec_test(
		[](float angle)
		{
			auto v2f_0 = sek::vec2f{1 / std::tanh(angle)};
			auto v2f_1 = coth(sek::vec2f{angle});
			EXPECT_TRUE(all(fcmp_eq(v2f_0, v2f_1, 0.0001f)));
		});
	exec_test(
		[](double angle)
		{
			auto v4d_0 = sek::vec4d{1 / std::tanh(angle)};
			auto v4d_1 = coth(sek::vec4d{angle});
			EXPECT_TRUE(all(fcmp_eq(v4d_0, v4d_1, 0.000001)));
		});
	exec_test(
		[](double angle)
		{
			auto v2d_0 = sek::vec2d{1 / std::tanh(angle)};
			auto v2d_1 = coth(sek::vec2d{angle});
			EXPECT_TRUE(all(fcmp_eq(v2d_0, v2d_1, 0.000001)));
		});
}

TEST(math_tests, asinh_test)
{
	constexpr auto exec_test = [](auto test_func)
	{
		test_func(-1.0f);
		test_func(-0.5f);
		test_func(0.0f);
		test_func(0.5f);
		test_func(1.0f);
	};

	exec_test(
		[](float angle)
		{
			auto v4f_0 = sek::vec4f{std::asinh(angle)};
			auto v4f_1 = asinh(sek::vec4f{angle});
			EXPECT_TRUE(all(fcmp_eq(v4f_0, v4f_1, 0.0001f)));
		});
	exec_test(
		[](float angle)
		{
			auto v2f_0 = sek::vec2f{std::asinh(angle)};
			auto v2f_1 = asinh(sek::vec2f{angle});
			EXPECT_TRUE(all(fcmp_eq(v2f_0, v2f_1, 0.0001f)));
		});
	exec_test(
		[](double angle)
		{
			auto v4d_0 = sek::vec4d{std::asinh(angle)};
			auto v4d_1 = asinh(sek::vec4d{angle});
			EXPECT_TRUE(all(fcmp_eq(v4d_0, v4d_1, 0.000001)));
		});
	exec_test(
		[](double angle)
		{
			auto v2d_0 = sek::vec2d{std::asinh(angle)};
			auto v2d_1 = asinh(sek::vec2d{angle});
			EXPECT_TRUE(all(fcmp_eq(v2d_0, v2d_1, 0.000001)));
		});
}
TEST(math_tests, acosh_test)
{
	constexpr auto exec_test = [](auto test_func)
	{
		test_func(1.0f);
		test_func(1.5f);
		test_func(2.0f);
		test_func(2.5f);
		test_func(3.0f);
	};

	exec_test(
		[](float angle)
		{
			auto v4f_0 = sek::vec4f{std::acosh(angle)};
			auto v4f_1 = acosh(sek::vec4f{angle});
			EXPECT_TRUE(all(fcmp_eq(v4f_0, v4f_1, 0.0001f)));
		});
	exec_test(
		[](float angle)
		{
			auto v2f_0 = sek::vec2f{std::acosh(angle)};
			auto v2f_1 = acosh(sek::vec2f{angle});
			EXPECT_TRUE(all(fcmp_eq(v2f_0, v2f_1, 0.0001f)));
		});
	exec_test(
		[](double angle)
		{
			auto v4d_0 = sek::vec4d{std::acosh(angle)};
			auto v4d_1 = acosh(sek::vec4d{angle});
			EXPECT_TRUE(all(fcmp_eq(v4d_0, v4d_1, 0.000001)));
		});
	exec_test(
		[](double angle)
		{
			auto v2d_0 = sek::vec2d{std::acosh(angle)};
			auto v2d_1 = acosh(sek::vec2d{angle});
			EXPECT_TRUE(all(fcmp_eq(v2d_0, v2d_1, 0.000001)));
		});
}
TEST(math_tests, atanh_test)
{
	constexpr auto exec_test = [](auto test_func)
	{
		test_func(-0.5f);
		test_func(0.0f);
		test_func(0.5f);
	};

	exec_test(
		[](float angle)
		{
			auto v4f_0 = sek::vec4f{std::atanh(angle)};
			auto v4f_1 = atanh(sek::vec4f{angle});
			EXPECT_TRUE(all(fcmp_eq(v4f_0, v4f_1, 0.0001f)));
		});
	exec_test(
		[](float angle)
		{
			auto v2f_0 = sek::vec2f{std::atanh(angle)};
			auto v2f_1 = atanh(sek::vec2f{angle});
			EXPECT_TRUE(all(fcmp_eq(v2f_0, v2f_1, 0.0001f)));
		});
	exec_test(
		[](double angle)
		{
			auto v4d_0 = sek::vec4d{std::atanh(angle)};
			auto v4d_1 = atanh(sek::vec4d{angle});
			EXPECT_TRUE(all(fcmp_eq(v4d_0, v4d_1, 0.000001)));
		});
	exec_test(
		[](double angle)
		{
			auto v2d_0 = sek::vec2d{std::atanh(angle)};
			auto v2d_1 = atanh(sek::vec2d{angle});
			EXPECT_TRUE(all(fcmp_eq(v2d_0, v2d_1, 0.000001)));
		});
}
TEST(math_tests, acoth_test)
{
	constexpr auto exec_test = [](auto test_func)
	{
		test_func(-3.0f);
		test_func(-2.5f);
		test_func(-2.0f);
		test_func(-1.5f);
		test_func(1.5f);
		test_func(2.0f);
		test_func(2.5f);
		test_func(3.0f);
	};

	exec_test(
		[](float angle)
		{
			auto v4f_0 = sek::vec4f{0.5f * std::log((angle + 1.0f) / (angle - 1.0f))};
			auto v4f_1 = acoth(sek::vec4f{angle});
			EXPECT_TRUE(all(fcmp_eq(v4f_0, v4f_1, 0.0001f)));
		});
	exec_test(
		[](float angle)
		{
			auto v2f_0 = sek::vec2f{0.5f * std::log((angle + 1.0f) / (angle - 1.0f))};
			auto v2f_1 = acoth(sek::vec2f{angle});
			EXPECT_TRUE(all(fcmp_eq(v2f_0, v2f_1, 0.0001f)));
		});
	exec_test(
		[](double angle)
		{
			auto v4d_0 = sek::vec4d{0.5 * std::log((angle + 1.0) / (angle - 1.0))};
			auto v4d_1 = acoth(sek::vec4d{angle});
			EXPECT_TRUE(all(fcmp_eq(v4d_0, v4d_1, 0.000001)));
		});
	exec_test(
		[](double angle)
		{
			auto v2d_0 = sek::vec2d{0.5 * std::log((angle + 1.0) / (angle - 1.0))};
			auto v2d_1 = acoth(sek::vec2d{angle});
			EXPECT_TRUE(all(fcmp_eq(v2d_0, v2d_1, 0.000001)));
		});
}

TEST(math_tests, matrix_test)
{
	{
		auto m4f_i = sek::fmat4{};
		auto m4f_1 = sek::fmat4{1};
		EXPECT_EQ(m4f_i, m4f_1);
	}
	{
		auto m2f_1 = sek::fmat2{1};
		auto m2f_2 = sek::fmat2{1, 2, 2, 1};
		auto m2f_3 = m2f_1 + m2f_2;

		EXPECT_EQ(m2f_3, (sek::fmat2{2, 2, 2, 2}));
	}
	{
		EXPECT_EQ(transpose(sek::fmat3{1}), (sek::fmat3{1}));
		EXPECT_EQ(transpose(sek::fmat3x2{1, 4, 0, 5, 1, 0}), (sek::fmat2x3{1, 5, 4, 1, 0, 0}));
	}
	{
		auto m3x2f = sek::fmat3x2{0, 4, -2, -4, -3, 0};
		auto m2x3f = sek::fmat2x3{0, 1, 1, -1, 2, 3};
		auto m2f = m3x2f * m2x3f;

		EXPECT_EQ(m2f, (sek::fmat2{0, -10, -3, -1}));
	}
	{
		auto m3x2f = sek::fmat3x2{1, -1, 2, 0, -3, 1};
		auto v3f = sek::vec3f{2, 1, 0};
		auto v2f = m3x2f * v3f;

		EXPECT_TRUE(all(v2f == sek::vec2f{1, -3}));
	}
}
TEST(math_tests, quaternion_test)
{
	{
		const auto rot1 = sek::vec3f{60, 30, 90};

		const auto q1 = sek::fquat::from_euler(rad(rot1));
		const auto e1 = sek::deg(q1.to_euler());
		EXPECT_TRUE(all(fcmp_eq(e1, rot1, 0.0001f)));

		const auto m1 = q1.to_mat();
		const auto q2 = sek::fquat::from_mat(m1);
		EXPECT_TRUE(all(fcmp_eq(q1, q2, 0.0001f)));

		const auto e2 = deg(q1.to_euler());
		const auto e3 = deg(q2.to_euler());
		EXPECT_TRUE(all(fcmp_eq(e2, e1, 0.0001f)));
		EXPECT_TRUE(all(fcmp_eq(e3, e1, 0.0001f)));
	}
	{
		const auto rot1 = sek::vec3f{60, 30, 90};
		const auto q1 = sek::fquat::from_euler(rad(rot1));

		const auto u1 = q1.angle();
		const auto a1 = q1.axis();
		const auto q2 = sek::fquat::from_angle_axis(u1, a1);
		EXPECT_TRUE(all(fcmp_eq(q1, q2, 0.0001f)));

		const auto u2 = q2.angle();
		const auto a2 = q2.axis();
		EXPECT_TRUE(sek::fcmp_eq(u2, u1, 0.0001f));
		EXPECT_TRUE(all(fcmp_eq(a2, a1, 0.0001f)));
	}
	{
		const auto fwd = sek::vec3f{-1, 0, 0};
		const auto inv = sek::vec3f{1, 0, 0};

		auto q = sek::fquat::look_at_l(fwd);
		q.rotate(sek::rad(180.0f), sek::vec3f{0, 1, 0});

		const auto v = fwd * q;
		EXPECT_TRUE(all(fcmp_eq(inv, v, 0.0001f)));
	}
}

TEST(math_tests, random_test)
{
	{
		sek::xoroshiro256<uint64_t> r1;
		sek::xoroshiro256<uint64_t> r2 = r1;

		EXPECT_EQ(r1, r2);
		EXPECT_EQ(r1(), r2());
		EXPECT_NE(r1(), r1());
	}
	{
		sek::xoroshiro128<float> r1;
		sek::xoroshiro128<float> r2;

		EXPECT_EQ(r1(), r2());
		EXPECT_NE(r1(), r1());

		std::stringstream ss;
		ss << r1;
		ss >> r2;
		EXPECT_EQ(r1(), r2());
	}
}