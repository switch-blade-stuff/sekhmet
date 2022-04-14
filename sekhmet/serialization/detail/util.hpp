//
// Created by switch_blade on 2022-04-13.
//

#pragma once

#include <concepts>
#include <iosfwd>

#include "sekhmet/detail/define.h"
#include <string_view>

namespace sek::serialization
{
	/** @brief Policy tag used to indicate that an archive supports input operations. */
	struct input_archive_policy
	{
	};
	/** @brief Policy tag used to indicate that an archive supports output operations. */
	struct output_archive_policy
	{
	};
	/** @brief Policy tag used to indicate that an archive supports both input and output operations. */
	struct inout_archive_policy : input_archive_policy, output_archive_policy
	{
	};

	/** @brief Concept satisfied only if archive `A` supports reading instances of `T`. */
	template<typename A, typename T>
	concept input_archive = requires(A &archive, T &value)
	{
		typename A::category_policy;
		std::is_base_of_v<input_archive_policy, typename A::category_policy>;

		std::default_initializable<A>;
		std::movable<A>;
		std::constructible_from<A, FILE *>;
		std::constructible_from<A, const char *, std::size_t>;
		std::constructible_from<A, std::streambuf *>;
		std::constructible_from<A, std::istream &>;

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
		typename A::category_policy;
		std::is_base_of_v<output_archive_policy, typename A::category_policy>;

		std::default_initializable<A>;
		std::movable<A>;
		std::constructible_from<A, FILE *>;
		std::constructible_from<A, char *, std::size_t>;
		std::constructible_from<A, std::streambuf *>;
		std::constructible_from<A, std::ostream &>;

		// clang-format off
		{ archive << value } -> std::same_as<A &>;
		{ archive.write(value) } -> std::same_as<A &>;
		// clang-format on
	};

	/** @brief Archive modifier used to specify an explicit name for an entry. */
	template<typename T>
	struct named_entry
	{
	private:
		constexpr static bool noexcept_fwd = noexcept(T(std::forward<T>(std::declval<T &&>())));

	public:
		/** Constructs a named entry modifier from a name and a perfectly-forwarded value.
		 * @param name Name of the entry.
		 * @param value Value forwarded by the modifier. */
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
		concept named_entry_input = requires(A &archive, T &data)
		{
			input_archive<A, T>;
			input_archive<A, decltype(named_entry{std::declval<std::string_view>(), data})>;
			input_archive<A, decltype(named_entry{std::declval<const char *>(), data})>;
		};
		template<typename A, typename T>
		concept named_entry_output = requires(A &archive, const T &data)
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

	/** @brief Archive modifier used to switch an archive to sequence IO mode. */
	template<>
	struct sequence<>
	{
	};
	sequence() -> sequence<>;

	/** @brief Archive modifier used to switch an archive to sequence IO mode and read/write explicit sequence size. */
	template<typename T>
	requires std::integral<std::decay_t<T>>
	struct sequence<T>
	{
		/** Constructs a sequence modifier from a perfectly-forwarded sequence size.
		 * @param value Size of the sequence forwarded by the modifier. */
		constexpr explicit sequence(T &&value) noexcept : value(std::forward<T>(value)) {}

		T value;
	};
	template<typename T>
	sequence(T &&value) -> sequence<T>;

	/** @brief Policy tag used to indicate that an archive supports reading & writing sequences with explicit size. */
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
}	 // namespace sek::serialization