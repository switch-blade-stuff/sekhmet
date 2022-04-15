//
// Created by switchblade on 2022-04-14.
//

#pragma once

#include "archive_traits.hpp"

namespace sek::serialization
{
	/** @brief Parent struct for archive manipulators. */
	struct archive_manipulator
	{
	};

	/** @brief Archive manipulator used to specify an explicit name for an entry. */
	template<typename T>
	struct named_entry : archive_manipulator
	{
	private:
		constexpr static bool noexcept_fwd = noexcept(T(std::forward<T>(std::declval<T &&>())));

	public:
		named_entry() = delete;

		/** Constructs a named entry manipulator from a name and a perfectly-forwarded value.
		 * @param name Name of the entry.
		 * @param value Value forwarded by the manipulator. */
		constexpr named_entry(std::string_view name, T &&value) noexcept(noexcept_fwd)
			: name(name), value(std::forward<T>(value))
		{
		}
		/** @copydoc named_entry */
		constexpr named_entry(const char *name, T &&value) noexcept(noexcept(noexcept_fwd))
			: name(name), value(std::forward<T>(value))
		{
		}

		std::string_view name;
		T value;
	};

	template<typename T>
	named_entry(std::string_view name, T &&value) -> named_entry<T>;
	template<typename T>
	named_entry(const char *name, T &&value) -> named_entry<T>;

	/** @brief Policy tag used to indicate that an archive supports reading & writing entries with explicit names. */
	struct named_entry_policy
	{
	};

	namespace detail
	{
		template<typename T>
		concept has_named_entry_policy = requires
		{
			typename T::entry_policy;
			std::is_base_of_v<named_entry_policy, typename T::entry_policy>;
		};
		template<typename A, typename T>
		concept named_entry_input = requires(A &archive, std::remove_cvref_t<T> &data)
		{
			input_archive<A, T>;
			input_archive<A, decltype(named_entry{std::declval<std::string_view>(), data})>;
			input_archive<A, decltype(named_entry{std::declval<const char *>(), data})>;
		};
		template<typename A, typename T>
		concept named_entry_output = requires(A &archive, const std::remove_cvref_t<T> &data)
		{
			output_archive<A, T>;
			output_archive<A, decltype(named_entry{std::declval<std::string_view>(), data})>;
			output_archive<A, decltype(named_entry{std::declval<const char *>(), data})>;
		};
	}	 // namespace detail

	/** @brief Concept satisfied only if archive `A` supports input or output of named entries of `T`. */
	template<typename A, typename T>
	concept named_entry_archive = detail::has_named_entry_policy<A> &&
		(detail::named_entry_input<A, T> || detail::named_entry_output<A, T>);

	template<typename...>
	struct sequence;

	/** @brief Archive manipulator used to switch an archive to sequence IO mode. */
	template<>
	struct sequence<> : archive_manipulator
	{
	};
	sequence() -> sequence<>;

	/** @brief Archive manipulator used to switch an archive to sequence IO mode and read/write fixed sequence size. */
	template<typename T>
	requires std::integral<std::decay_t<T>>
	struct sequence<T> : archive_manipulator
	{
		/** Constructs a sequence manipulator from a perfectly-forwarded sequence size.
		 * @param value Size of the sequence forwarded by the manipulator. */
		constexpr explicit sequence(T &&value) noexcept : value(std::forward<T>(value)) {}

		T value;
	};
	template<typename T>
	sequence(T &&value) -> sequence<T>;

	/** @brief Policy tag used to indicate that an archive supports reading & writing sequences of fixed size. */
	struct fixed_sequence_policy
	{
	};

	namespace detail
	{
		template<typename T>
		concept has_fixed_sequence_policy = requires
		{
			typename T::sequence_policy;
			std::is_base_of_v<fixed_sequence_policy, typename T::sequence_policy>;
		};
		template<typename A>
		concept fixed_sequence_input = requires(A &archive, std::size_t &size)
		{
			input_archive<A, decltype(sequence{size})>;
		};
		template<typename A>
		concept fixed_sequence_output = requires(A &archive, const std::size_t &size)
		{
			output_archive<A, decltype(sequence{size})>;
		};
	}	 // namespace detail

	/** @brief Concept satisfied only if archive `A` supports input or output of fixed-size sequences. */
	template<typename A>
	concept fixed_sequence_archive = detail::has_fixed_sequence_policy<A> &&
		(detail::fixed_sequence_input<A> || detail::fixed_sequence_output<A>);

	/** @brief Archive manipulator used to change archive's pretty-printing mode.
	 * @note If the archive does not support pretty-printing, this manipulator will be ignored. */
	struct pretty_print : archive_manipulator
	{
		pretty_print() = delete;

		/** Initializes the modifier to the specific pretty-print mode.
		 * @param value If set to true, pretty-printing will be enabled, otherwise pretty-printing will be disabled. */
		constexpr explicit pretty_print(bool value) noexcept : value(value) {}

		bool value;
	};
}	 // namespace sek::serialization