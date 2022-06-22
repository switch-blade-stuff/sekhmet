/*
 * Created by switch_blade on 2022-04-13.
 */

#pragma once

#include <concepts>
#include <iosfwd>
#include <stdexcept>

#include "sekhmet/detail/define.h"
#include <string_view>

namespace sek::serialization
{
	/** @brief Exception thrown by archives on (de)serialization failure. */
	class archive_error : public std::runtime_error
	{
	public:
		archive_error() : std::runtime_error("Unknown archive error") {}
		explicit archive_error(std::string &&msg) : std::runtime_error(std::move(msg)) {}
		explicit archive_error(const std::string &msg) : std::runtime_error(msg) {}
		explicit archive_error(const char *msg) : std::runtime_error(msg) {}
		~archive_error() override = default;
	};

	/** @brief Category tag used to indicate that an archive supports input operations. */
	struct input_archive_category
	{
	};
	/** @brief Category tag used to indicate that an archive supports output operations. */
	struct output_archive_category
	{
	};
	/** @brief Category tag used to indicate that an archive supports both input and output operations. */
	struct inout_archive_category : input_archive_category, output_archive_category
	{
	};

	// clang-format off
	/** @brief Concept satisfied only if archive `A` supports reading instances of `T`. */
	template<typename A, typename T>
	concept input_archive = requires(A &archive, T &value)
	{
		typename A::archive_category;
		typename A::char_type;
		typename A::size_type;
		std::is_base_of_v<input_archive_category, typename A::archive_category>;

		{ archive >> value } -> std::convertible_to<A &>;
		{ archive.read(value) } -> std::convertible_to<A &>;
		{ archive.try_read(value) } -> std::convertible_to<bool>;
		{ archive.read(std::in_place_type<T>) } -> std::convertible_to<T>;
	};

	/** @brief Concept satisfied only if archive `A` supports writing instances of `T`. */
	template<typename A, typename T>
	concept output_archive = requires(A &archive, const T &value)
	{
		typename A::archive_category;
		typename A::char_type;
		typename A::size_type;
		std::is_base_of_v<output_archive_category, typename A::archive_category>;

		{ archive << value } -> std::convertible_to<A &>;
		{ archive.write(value) } -> std::convertible_to<A &>;
		{ archive.flush() };
	};

	/** @brief Concept satisfied only if `A` is an archive of structured data format. */
	template<typename A>
	concept structured_data_archive = requires(A &a, const A &ca)
	{
		typename A::iterator;
		typename A::const_iterator;

		typename A::value_type;
		typename A::reference;
		typename A::const_reference;

		typename A::size_type;
		typename A::difference_type;
		requires std::same_as<typename A::difference_type, typename std::iterator_traits<typename A::iterator>::difference_type>;
		requires std::same_as<typename A::difference_type, typename std::iterator_traits<typename A::const_iterator>::difference_type>;

		{ ca.size() } -> std::same_as<typename A::size_type>;
		{ ca.max_size() } -> std::same_as<typename A::size_type>;
		{ ca.empty() } -> std::same_as<bool>;

		{ a.begin() } -> std::same_as<typename A::iterator>;
		{ a.end() } -> std::same_as<typename A::iterator>;
		{ a.cbegin() } -> std::same_as<typename A::const_iterator>;
		{ a.cend() } -> std::same_as<typename A::const_iterator>;
		{ ca.begin() } -> std::same_as<typename A::const_iterator>;
		{ ca.end() } -> std::same_as<typename A::const_iterator>;
		{ ca.cbegin() } -> std::same_as<typename A::const_iterator>;
		{ ca.cend() } -> std::same_as<typename A::const_iterator>;

		{ a.front() } -> std::same_as<typename A::reference>;
		{ a.back() } -> std::same_as<typename A::reference>;
		{ ca.front() } -> std::same_as<typename A::const_reference>;
		{ ca.back() } -> std::same_as<typename A::const_reference>;
	};
	// clang-format on
}	 // namespace sek::serialization
