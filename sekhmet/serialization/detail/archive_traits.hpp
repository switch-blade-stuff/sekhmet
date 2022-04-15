//
// Created by switch_blade on 2022-04-13.
//

#pragma once

#include <concepts>
#include <iosfwd>
#include <stdexcept>

#include "sekhmet/detail/define.h"
#include <string_view>

namespace sek::serialization
{
	/** @brief Exception thrown by archives on (de)serialization failure. */
	struct archive_error : public std::runtime_error
	{
	public:
		archive_error() : std::runtime_error("Unknown archive error") {}
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

	/** @brief Concept satisfied only if archive `A` supports reading instances of `T`. */
	template<typename A, typename T>
	concept input_archive = requires(A &archive, T &value)
	{
		typename A::archive_category;
		std::is_base_of_v<input_archive_category, typename A::archive_category>;

		// clang-format off
		{ archive >> value } -> std::same_as<A &>;
		{ archive.read(value) } -> std::same_as<A &>;
		{ archive.try_read(value) } -> std::same_as<bool>;
		{ archive.template read<T>() } -> std::same_as<T>;
		// clang-format on
	};

	/** @brief Concept satisfied only if archive `A` supports writing instances of `T`. */
	template<typename A, typename T>
	concept output_archive = requires(A &archive, const T &value)
	{
		typename A::archive_category;
		std::is_base_of_v<output_archive_category, typename A::archive_category>;

		// clang-format off
		{ archive << value } -> std::same_as<A &>;
		{ archive.write(value) } -> std::same_as<A &>;
		{ archive.flush() };
		// clang-format on
	};

	// clang-format off
	/** @brief Concept satisfied only if `T` is serializable using archive of type `A`. */
	template<typename T, typename A>
	concept serializable_with = requires(A &archive, const T &data) { serialize(archive, data); } ||
								requires(A &archive, const T &data) { data.serialize(archive); };
	/** @brief Concept satisfied only if `T` is deserializable using archive of type `A`. */
	template<typename T, typename A>
	concept deserializable_with = requires(A &archive, T &data) { deserialize(archive, data); } ||
								  requires(A &archive, T &data) { data.deserialize(archive); };
	// clang-format on

	/** @brief Concept satisfied only if `A` is a container-like archive. */
	template<typename A>
	concept container_like_archive = requires(A &a, const A &ca)
	{
		typename A::iterator;
		typename A::const_iterator;

		typename A::value_type;
		typename A::reference;
		typename A::const_reference;

		typename A::size_type;
		typename A::difference_type;
		std::same_as<typename A::difference_type, typename std::iterator_traits<typename A::iterator>::difference_type>;
		std::same_as<typename A::difference_type, typename std::iterator_traits<typename A::const_iterator>::difference_type>;

		// clang-format off
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
		// clang-format on
	};
}	 // namespace sek::serialization