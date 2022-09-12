/*
 * Created by switchblade on 03/08/22
 */

#pragma once

#include <functional>
#include <optional>

#include "define.h"

#if __cplusplus <= 202002L
namespace sek
{
	/** @brief Pre-C++23 implementation of `unexpect_t`. */
	struct unexpect_t
	{
		explicit unexpect_t() = default;
	};
	/** @brief Pre-C++23 implementation of `unexpect`. */
	constexpr auto unexpect = unexpect_t{};

	/** @brief Pre-C++23 implementation of `bad_expected_access`. */
	template<typename E>
	class bad_expected_access;
	/** @brief Pre-C++23 implementation of `expected`. */
	template<typename T, typename E>
	class expected;

	template<>
	class SEK_API bad_expected_access<void> : public std::exception
	{
		template<typename, typename>
		friend class expected;

	protected:
		bad_expected_access() noexcept = default;
		bad_expected_access(const bad_expected_access &) = default;
		bad_expected_access(bad_expected_access &&) = default;
		bad_expected_access &operator=(const bad_expected_access &) = default;
		bad_expected_access &operator=(bad_expected_access &&) = default;

		~bad_expected_access() override;

	public:
		[[nodiscard]] const char *what() const noexcept override { return "Bad access to expected"; }
	};
	template<typename E>
	class bad_expected_access : public bad_expected_access<void>
	{
	public:
		// clang-format off
		explicit bad_expected_access(E &&error) noexcept(std::is_nothrow_move_constructible_v<E>) : m_error(std::move(error)) {}
		explicit bad_expected_access(const E &error) : m_error(error) {}
		// clang-format on

		/** Returns reference to the underlying error. */
		[[nodiscard]] constexpr E &error() &noexcept { return m_error; }
		/** @copydoc error */
		[[nodiscard]] constexpr const E &error() const &noexcept { return m_error; }
		/** @copydoc error */
		[[nodiscard]] constexpr E &&error() &&noexcept { return m_error; }
		/** @copydoc error */
		[[nodiscard]] constexpr const E &&error() const &&noexcept { return m_error; }

	private:
		E m_error;
	};

	/** @brief Pre-C++23 implementation of `unexpected`. */
	template<typename E>
	class unexpected
	{
	public:
		constexpr unexpected() noexcept(std::is_nothrow_default_constructible_v<E>) = default;

		constexpr unexpected(const unexpected &) = default;
		constexpr unexpected &operator=(const unexpected &) = default;

		constexpr unexpected(unexpected &&) noexcept(std::is_nothrow_move_constructible_v<E>) = default;
		constexpr unexpected &operator=(unexpected &&) noexcept(std::is_nothrow_move_assignable_v<E>) = default;

		/** Initializes underlying value in-place.
		 * @param args Arguments passed to `E`'s constructor. */
		template<typename... Args>
		constexpr explicit unexpected(std::in_place_t, Args &&...args) noexcept(std::is_nothrow_constructible_v<E, Args...>)
			: m_value(std::forward<Args>(args)...)
		{
		}
		/** Initializes underlying value from an rvalue reference.
		 * Equivalent to `unexpected(std::in_place_t, std::forward<U>(value))`. */
		template<typename U = E>
		constexpr explicit unexpected(U &&value) : unexpected(std::in_place, std::forward<U>(value))
		{
		}

		// clang-format off
		/** @copydoc unexpected
		 * @param il Initializer list passed to `E`'s constructor. */
		template<typename U, typename... Args>
		constexpr explicit unexpected(std::in_place_t, std::initializer_list<U> il, Args &&...args)
			noexcept(std::is_nothrow_constructible_v<E, std::initializer_list<U> &, Args...>)
			requires(std::constructible_from<E, std::initializer_list<U> &, Args...>)
			: m_value(il, std::forward<Args>(args)...)
		{
		}
		// clang-format on

		/** Returns reference to the underlying value. */
		[[nodiscard]] constexpr const E &error() const &noexcept { return m_value; }
		/** @copydoc error */
		[[nodiscard]] constexpr E &error() &noexcept { return m_value; }
		/** @copydoc error */
		[[nodiscard]] constexpr const E &&error() const &&noexcept { return m_value; }
		/** @copydoc error */
		[[nodiscard]] constexpr E &&error() &&noexcept { return m_value; }

		constexpr void swap(unexpected &other) noexcept(std::is_nothrow_swappable_v<E>) { m_value.swap(other.m_value); }
		friend constexpr void swap(unexpected &a, unexpected &b) noexcept(noexcept(a.swap(b))) { a.swap(b); }

	private:
		E m_value = {};
	};

	template<typename E1, typename E2>
	[[nodiscard]] constexpr auto operator<=>(const unexpected<E1> &lhs, const unexpected<E2> &rhs)
	{
		return lhs.error() <=> rhs.error();
	}
	template<typename E1, typename E2>
	[[nodiscard]] constexpr bool operator==(const unexpected<E1> &lhs, const unexpected<E2> &rhs)
	{
		return lhs.error() == rhs.error();
	}

	template<typename E>
	unexpected(E) -> unexpected<E>;

	template<std::destructible T, typename E>
	class expected<T, E>
	{
		template<typename, typename>
		friend class expected;

	public:
		typedef T value_type;
		typedef E error_type;
		typedef unexpected<E> unexpected_type;

		template<typename U>
		using rebind = expected<U, error_type>;

	private:
		constexpr static void move_swap(expected *src, expected *dst)
		{
			/* Save temporary error. */
			auto tmp_err = std::move(src->m_error);
			std::destroy_at(std::addressof(src->m_error));

			/* Move-construct the value. */
			std::construct_at(std::addressof(dst->m_value), std::move(src->m_value));
			std::destroy_at(std::addressof(src->m_value));

			/* Move construct error from temporary. */
			std::construct_at(std::addressof(dst->m_error), std::move(tmp_err));
		}

		template<typename T2, typename E2>
		constexpr static bool is_constructible = std::constructible_from<T, T2> && std::constructible_from<E, E2>;
		template<typename T2, typename E2>
		constexpr static bool is_assignable = std::assignable_from<T &, T2> && std::assignable_from<E &, E2>;

		// clang-format off
		template<typename T2, typename E2>
		constexpr static bool allow_ctor = !(std::same_as<T, T2> && std::same_as<E, E2>) &&
			!(std::is_constructible_v<T, expected<T2, E2> &> || std::is_constructible_v<T, const expected<T2, E2> &> ||
			  std::is_constructible_v<T, expected<T2, E2> &&> || std::is_constructible_v<T, const expected<T2, E2> &&> ||
			  std::is_convertible_v<expected<T2, E2> &, T> || std::is_convertible_v<const expected<T2, E2> &, T> ||
			  std::is_convertible_v<expected<T2, E2> &&, T> || std::is_convertible_v<const expected<T2, E2> &&, T>);
		template<typename T2, typename E2>
		constexpr static bool allow_assign = !(std::same_as<T, T2> && std::same_as<E, E2>) &&
			!(std::is_constructible_v<T, expected<T2, E2> &> || std::is_constructible_v<T, const expected<T2, E2> &> ||
			  std::is_constructible_v<T, expected<T2, E2> &&> || std::is_constructible_v<T, const expected<T2, E2> &&> ||
			  std::is_assignable_v<T &, expected<T2, E2> &> || std::is_assignable_v<T &, const expected<T2, E2> &> ||
			  std::is_assignable_v<T &, expected<T2, E2> &&> || std::is_assignable_v<T &, const expected<T2, E2> &&> ||
			  std::is_convertible_v<expected<T2, E2> &, T> || std::is_convertible_v<const expected<T2, E2> &, T> ||
			  std::is_convertible_v<expected<T2, E2> &&, T> || std::is_convertible_v<const expected<T2, E2> &&, T>);
		// clang-format on

	public:
		/** Initializes `expected` with an default-constructed expected value. */
		constexpr expected() noexcept(std::is_nothrow_default_constructible_v<T>) : m_has_value(true), m_value() {}
		constexpr ~expected() { destroy(); }

		// clang-format off
		constexpr expected(const expected &other) requires(is_constructible<const T &, const E &>)
			: m_has_value(other.m_has_value)
		{
			copy_init(other);
		}
		constexpr expected &operator=(const expected &other)
			requires(is_constructible<const T &, const E &> && is_assignable<const T &, const E &>)
		{
			if (this != &other)
				copy_assign(other);
			return *this;
		}

		constexpr expected(expected &&other) noexcept(is_constructible<T &&, E &&>)
			requires(std::copy_constructible<T> && std::copy_constructible<E>)
			: m_has_value(other.m_has_value)
		{
			move_init(other);
		}
		constexpr expected &operator=(expected &&other)
			noexcept(is_constructible<T &&, E &&> && is_assignable<T &&, E &&>)
			requires(std::is_move_constructible_v<T> && std::is_move_assignable_v<T> &&
					 std::is_move_constructible_v<E> && std::is_copy_assignable_v<E>)
		{
			move_assign(other);
			return *this;
		}

		template<typename T2, typename E2>
		constexpr expected(const expected<T2, E2> &other) requires(allow_ctor<T2, E2> && is_constructible<const T2 &, const E2 &> && std::is_convertible_v<T2 &&, T>)
			: m_has_value(other.m_has_value)
		{
			copy_init(other);
		}
		template<typename T2, typename E2>
		constexpr explicit expected(const expected<T2, E2> &other) requires(allow_ctor<T2, E2> && is_constructible<const T2 &, const E2 &>)
			: m_has_value(other.m_has_value)
		{
			copy_init(other);
		}
		template<typename E2>
		constexpr explicit expected(const expected<void, E2> &other) requires(allow_ctor<void, E2> && std::is_constructible_v<E, const E2 &>)
			: m_has_value(other.m_has_value)
		{
			copy_init(other);
		}

		template<typename T2, typename E2>
		constexpr expected(expected<T2, E2> &&other) requires(allow_ctor<T2, E2> && is_constructible<T2 &&, E2 &&> && std::is_convertible_v<T2 &&, T>)
			: m_has_value(other.m_has_value)
		{
			move_init(other);
		}
		template<typename T2, typename E2>
		constexpr explicit expected(expected<T2, E2> &&other) requires(allow_ctor<T2, E2> && is_constructible<T2 &&, E2 &&>)
			: m_has_value(other.m_has_value)
		{
			move_init(other);
		}
		template<typename E2>
		constexpr explicit expected(expected<void, E2> &&other) requires(allow_ctor<void, E2> && std::is_constructible_v<E, E2 &&>)
			: m_has_value(other.m_has_value)
		{
			move_init(other);
		}
		// clang-format on

		/** Initializes the underlying expected value from an rvalue reference.
		 * Equivalent to `expected(std::in_place_t, std::forward<U>(value))`. */
		template<typename U = T>
		constexpr expected(U &&value) noexcept(std::is_nothrow_constructible_v<T, U &&>)
			: expected(std::in_place, std::forward<U>(value))
		{
		}

		/** Initializes the underlying expected value in-place.
		 * @param args Arguments passed to `T`'s constructor. */
		template<typename... Args>
		constexpr explicit expected(std::in_place_t, Args &&...args)
			: m_has_value(true), m_value(std::forward<Args>(args)...)
		{
		}

		// clang-format off
		/** @copydoc expected
		 * @param il Initializer list passed to `T`'s constructor. */
		template<typename U, typename... Args>
		constexpr explicit expected(std::in_place_t, std::initializer_list<U> il, Args &&...args)
			noexcept(std::is_nothrow_constructible_v<T, std::initializer_list<U> &, Args...>)
			requires(std::constructible_from<T, std::initializer_list<U> &, Args...>)
			: m_has_value(true), m_value(il, std::forward<Args>(args)...)
		{
		}

		template<typename U = T>
		constexpr expected &operator=(U &&value) noexcept(std::is_nothrow_constructible_v<T, U &&> && std::is_nothrow_assignable_v<T, U &&>)
		{
			if (!has_value())
			{
				std::destroy_at(std::addressof(m_error));
				std::construct_at(std::addressof(m_value), std::forward<U>(value));
				m_has_value = true;
			}
			else
				m_value = std::forward<U>(value);
			return *this;
		}
		// clang-format on

		/** Initializes the underlying unexpected value.
		 * Equivalent to `expected(unexpect, u.error())`. */
		template<typename G>
		constexpr expected(const unexpected<G> &u) : expected(unexpect, u.error())
		{
		}
		/** @copydoc expected */
		template<typename G>
		constexpr expected(unexpected<G> &&u) noexcept(std::is_nothrow_constructible_v<E, G &&>)
			: expected(unexpect, std::move(u.error()))
		{
		}

		/** Initializes the underlying unexpected value in-place.
		 * @param args Arguments passed to `E`'s constructor. */
		template<typename... Args>
		constexpr explicit expected(unexpect_t, Args &&...args) noexcept(std::is_nothrow_constructible_v<E, Args...>)
			: m_has_value(false), m_error(std::forward<Args>(args)...)
		{
		}

		// clang-format off
		/** @copydoc expected
		 * @param il Initializer list passed to `E`'s constructor. */
		template<typename U, typename... Args>
		constexpr explicit expected(unexpect_t, std::initializer_list<U> il, Args &&...args)
			noexcept(std::is_nothrow_constructible_v<E, std::initializer_list<U> &, Args...>)
			requires(std::constructible_from<E, std::initializer_list<U> &, Args...>)
			: m_has_value(false), m_error(il, std::forward<Args>(args)...)
		{
		}

		template<typename G>
		constexpr expected &operator=(const unexpected<G> &value)
		{
			if (has_value())
			{
				std::destroy_at(std::addressof(m_value));
				std::construct_at(std::addressof(m_error), value.error());
				m_has_value = false;
			}
			else
				m_error = value.error();
			return *this;
		}
		template<typename G>
		constexpr expected &operator=(unexpected<G> &&value) noexcept(std::is_nothrow_constructible_v<E, G &&> && std::is_nothrow_assignable_v<E, G &&>)
		{
			if (has_value())
			{
				std::destroy_at(std::addressof(m_value));
				std::construct_at(std::addressof(m_error), std::move(value.error()));
				m_has_value = false;
			}
			else
				m_error = std::move(value.error());
			return *this;
		}
		// clang-format on

		/** Constructs the underlying expected value in-place.
		 * @param args Arguments passed to `T`'s constructor.
		 * @return Reference to the expected value. */
		template<typename... Args>
		constexpr T &emplace(Args &&...args) noexcept
		{
			destroy();
			std::construct_at(std::addressof(m_value), std::forward<Args>(args)...);
			m_has_value = true;
			return m_value;
		}
		/** @copydoc emplace
		 * @param il Initializer list passed to `T`'s constructor. */
		template<typename U, class... Args>
		constexpr T &emplace(std::initializer_list<U> il, Args &&...args) noexcept
		{
			destroy();
			std::construct_at(std::addressof(m_value), il, std::forward<Args>(args)...);
			m_has_value = true;
			return m_value;
		}

		/** Returns `true` if the instance contains an expected value, `false` otherwise. */
		[[nodiscard]] constexpr bool has_value() const noexcept { return m_has_value; }
		/** @copydoc has_value */
		[[nodiscard]] constexpr explicit operator bool() const noexcept { return has_value(); }

		/** Returns pointer to the underlying expected value. */
		[[nodiscard]] constexpr T *operator->() noexcept { return std::addressof(m_value); }
		/** @copydoc operator-> */
		[[nodiscard]] constexpr const T *operator->() const noexcept { return std::addressof(m_value); }

		/** Returns reference to the underlying expected value. */
		[[nodiscard]] constexpr T &operator*() &noexcept { return m_value; }
		/** @copydoc operator* */
		[[nodiscard]] constexpr const T &operator*() const &noexcept { return m_value; }
		/** @copydoc operator* */
		[[nodiscard]] constexpr T &&operator*() &&noexcept { return std::move(m_value); }
		/** @copydoc operator* */
		[[nodiscard]] constexpr const T &&operator*() const &&noexcept { return std::move(m_value); }

		/** @copydoc operator*
		 * @throw bad_expected_access<E> If the instance contains an unexpected value. */
		[[nodiscard]] constexpr T &value() &
		{
			if (!has_value()) [[unlikely]]
				throw bad_expected_access<E>(m_error);
			return operator*();
		}
		/** @copydoc value */
		[[nodiscard]] constexpr const T &value() const &
		{
			if (!has_value()) [[unlikely]]
				throw bad_expected_access<E>(m_error);
			return operator*();
		}
		/** @copydoc value */
		[[nodiscard]] constexpr T &&value() &&
		{
			if (!has_value()) [[unlikely]]
				throw bad_expected_access<E>(std::move(m_error));
			return std::move(operator*());
		}
		/** @copydoc value */
		[[nodiscard]] constexpr const T &&value() const &&
		{
			if (!has_value()) [[unlikely]]
				throw bad_expected_access<E>(std::move(m_error));
			return std::move(operator*());
		}

		/** Returns reference to the underlying unexpected value.
		 * @throw bad_expected_access<void> If the instance contains an expected value. */
		[[nodiscard]] constexpr E &error() &
		{
			if (has_value()) [[unlikely]]
				throw bad_expected_access<void>();
			return m_error;
		}
		/** @copydoc error */
		[[nodiscard]] constexpr const E &error() const &
		{
			if (has_value()) [[unlikely]]
				throw bad_expected_access<void>();
			return m_error;
		}
		/** @copydoc error */
		[[nodiscard]] constexpr E &&error() &&
		{
			if (has_value()) [[unlikely]]
				throw bad_expected_access<void>();
			return m_error;
		}
		/** @copydoc error */
		[[nodiscard]] constexpr const E &&error() const &&
		{
			if (has_value()) [[unlikely]]
				throw bad_expected_access<void>();
			return m_error;
		}

		/** Returns the underlying expected value or a new instance constructed from `value`. */
		template<typename U>
		[[nodiscard]] constexpr T value_or(U &&value) const &
		{
			if (!has_value())
				return T{std::forward<U>(value)};
			else
				return m_value;
		}
		/** @copydoc value_or */
		template<typename U>
		[[nodiscard]] constexpr T value_or(U &&value) &&
		{
			if (!has_value())
				return T{std::forward<U>(value)};
			else
				return std::move(m_value);
		}

		template<typename T2, typename E2>
		[[nodiscard]] friend constexpr bool operator==(const expected &lhs, const expected<T2, E2> &rhs)
		{
			if (lhs.has_value() != rhs.has_value()) [[unlikely]]
				return false;
			else if (!lhs.has_value())
				return lhs.error() == rhs.error();
			else
				return *lhs == *rhs;
		}
		template<typename E2>
		[[nodiscard]] friend constexpr bool operator==(const expected &lhs, const unexpected<E2> &rhs)
		{
			return !lhs.has_value() && lhs.error() == rhs.error();
		}
		template<typename T2>
		[[nodiscard]] friend constexpr bool operator==(const expected &lhs, const T2 &rhs)
		{
			return lhs.has_value() && *lhs == rhs;
		}

		// clang-format off
		constexpr void swap(expected &other)
				noexcept(std::is_nothrow_move_constructible_v<E> && std::is_nothrow_swappable_v<E> &&
						 std::is_nothrow_move_constructible_v<T> && std::is_nothrow_swappable_v<T>)
		{
			const auto other_val = other.has_value();
			const auto this_val = has_value();


			using std::swap;
			if (this_val != other_val) [[unlikely]]
			{
				if (other_val)
					move_swap(this, &other);
				else
					move_swap(&other, this);
			}
			else if (this_val)
				swap(m_value, other.m_value);
			else
				swap(m_error, other.m_error);
			swap(m_value, other.m_value);
		}
		friend constexpr void swap(expected &a, expected &b)
				noexcept(std::is_nothrow_move_constructible_v<E> && std::is_nothrow_swappable_v<E> &&
						 std::is_nothrow_move_constructible_v<T> && std::is_nothrow_swappable_v<T>)
		{
			a.swap(b);
		}
		// clang-format on

	private:
		template<typename T2, typename E2>
		constexpr void copy_assign(const expected<T2, E2> &other)
		{
			if (has_value() != other.has_value())
			{
				destroy();
				copy_init(other);
				m_has_value = other.m_has_value;
			}
			else if (has_value())
				m_value = other.m_value;
			else
				m_error = other.m_error;
		}
		template<typename T2, typename E2>
		constexpr void move_assign(expected<T2, E2> &other)
		{
			if (has_value() != other.has_value())
			{
				destroy();
				move_init(other);
				m_has_value = other.m_has_value;
			}
			else if (has_value())
				m_value = std::move(other.m_value);
			else
				m_error = std::move(other.m_error);
		}
		template<typename T2, typename E2>
		constexpr void copy_init(const expected<T2, E2> &other)
		{
			if (other.has_value())
				std::construct_at(std::addressof(m_value), other.m_value);
			else
				std::construct_at(std::addressof(m_error), other.m_error);
		}
		template<typename T2, typename E2>
		constexpr void move_init(expected<T2, E2> &other)
		{
			if (other.has_value())
				std::construct_at(std::addressof(m_value), std::move(other.m_value));
			else
				std::construct_at(std::addressof(m_error), std::move(other.m_error));
		}

		template<typename E2>
		constexpr void copy_assign(const expected<void, E2> &other)
		{
			if (has_value() != other.has_value())
			{
				destroy();
				copy_init(other);
				m_has_value = other.m_has_value;
			}
			else if (!has_value())
				m_error = other.m_error;
		}
		template<typename E2>
		constexpr void move_assign(expected<void, E2> &other)
		{
			if (has_value() != other.has_value())
			{
				destroy();
				move_init(other);
				m_has_value = other.m_has_value;
			}
			else if (!has_value())
				m_error = std::move(other.m_error);
		}
		template<typename E2>
		constexpr void copy_init(const expected<void, E2> &other)
		{
			if (!other.has_value())
				std::construct_at(std::addressof(m_error), other.m_error);
			else
				std::construct_at(std::addressof(m_value));
		}
		template<typename E2>
		constexpr void move_init(expected<void, E2> &other)
		{
			if (!other.has_value())
				std::construct_at(std::addressof(m_error), std::move(other.m_error));
			else
				std::construct_at(std::addressof(m_value));
		}

		constexpr void destroy() noexcept
		{
			if (has_value())
				std::destroy_at(std::addressof(m_value));
			else
				std::destroy_at(std::addressof(m_error));
		}

		bool m_has_value;
		union
		{
			T m_value;
			E m_error;
		};
	};

	template<typename E>
	class expected<void, E>
	{
		template<typename, typename>
		friend class expected;

	public:
		typedef void value_type;
		typedef E error_type;
		typedef unexpected<E> unexpected_type;

		template<typename U>
		using rebind = expected<U, error_type>;

	public:
		/** Initializes the instance to expected state. */
		constexpr expected() noexcept : m_has_value(true) {}
		/** Initializes the instance to expected state. */
		constexpr explicit expected(std::in_place_t) noexcept : expected() {}

		constexpr ~expected() { destroy(); }

		// clang-format off
		constexpr expected(const expected &other) requires(std::is_copy_constructible_v<E>) : m_has_value(other.m_has_value) { copy_init(other); }
		constexpr expected &operator=(const expected &other) requires(std::is_copy_constructible_v<E> && std::is_copy_assignable_v<E>)
		{
			if (this != &other) [[likely]]
				copy_assign(other);
			return *this;
		}

		constexpr expected(expected &&other) noexcept(std::is_nothrow_move_constructible_v<E>) requires(std::is_move_constructible_v<E>)
			: m_has_value(other.m_has_value)
		{
			move_init(other);
		}
		constexpr expected &operator=(expected &&other)
			noexcept(std::is_nothrow_move_constructible_v<E> && std::is_nothrow_move_assignable_v<E>)
			requires(std::is_move_constructible_v<E> && std::is_move_assignable_v<E>)
		{
			move_assign(other);
			return *this;
		}

		template<typename T2, typename E2>
		constexpr expected(const expected<T2, E2> &other) requires(!(std::is_void_v<T2> && std::same_as<E, E2>) && std::is_constructible_v<E, const E2 &>)
			: m_has_value(other.m_has_value)
		{
			copy_init(other);
		}
		template<typename T2, typename E2>
		constexpr expected(expected<T2, E2> &&other) noexcept(std::is_nothrow_constructible_v<E, E2 &&>)
			requires(!(std::is_void_v<T2> && std::same_as<E, E2>) && std::is_constructible_v<E, E2 &&>)
			: m_has_value(other.m_has_value)
		{
			move_init(other);
		}
		// clang-format on

		/** Initializes the underlying unexpected value.
		 * Equivalent to `expected(unexpect, u.error())`. */
		template<typename G>
		constexpr expected(const unexpected<G> &u) : expected(unexpect, u.error())
		{
		}
		/** @copydoc expected */
		template<typename G>
		constexpr expected(unexpected<G> &&u) noexcept(std::is_nothrow_constructible_v<E, G &&>)
			: expected(unexpect, std::move(u.error()))
		{
		}

		/** Initializes the underlying unexpected value in-place.
		 * @param args Arguments passed to `E`'s constructor. */
		template<typename... Args>
		constexpr explicit expected(unexpect_t, Args &&...args) noexcept(std::is_nothrow_constructible_v<E, Args...>)
			: m_has_value(false), m_error(std::forward<Args>(args)...)
		{
		}

		// clang-format off
		/** @copydoc expected
		 * @param il Initializer list passed to `E`'s constructor. */
		template<typename U, typename... Args>
		constexpr explicit expected(unexpect_t, std::initializer_list<U> il, Args &&...args)
			noexcept(std::is_nothrow_constructible_v<E, std::initializer_list<U> &, Args...>)
			requires(std::constructible_from<E, std::initializer_list<U> &, Args...>)
			: m_has_value(false), m_error(il, std::forward<Args>(args)...)
		{
		}

		template<typename G>
		constexpr expected &operator=(const unexpected<G> &value)
		{
			if (has_value())
			{
				std::construct_at(std::addressof(m_error), value.error());
				m_has_value = false;
			}
			else
				m_error = value.error();
			return *this;
		}
		template<typename G>
		constexpr expected &operator=(unexpected<G> &&value) noexcept(std::is_nothrow_constructible_v<E, G &&> && std::is_nothrow_assignable_v<E, G &&>)
		{
			if (has_value())
			{
				std::construct_at(std::addressof(m_error), std::move(value.error()));
				m_has_value = false;
			}
			else
				m_error = std::move(value.error());
			return *this;
		}
		// clang-format on

		/** Sets the instance to expected state. */
		constexpr void emplace() noexcept { m_has_value = true; }

		/** Returns `true` if the instance contains an expected value, `false` otherwise. */
		[[nodiscard]] constexpr bool has_value() const noexcept { return m_has_value; }
		/** @copydoc has_value */
		[[nodiscard]] constexpr explicit operator bool() const noexcept { return has_value(); }

		/** Returns `void`. */
		constexpr void operator*() const noexcept {}

		/** @copydoc operator*
		 * @throw bad_expected_access<E> If the instance contains an unexpected value. */
		constexpr void value() &
		{
			if (!has_value()) [[unlikely]]
				throw bad_expected_access<E>(m_error);
		}
		/** @copydoc value */
		constexpr void value() const &
		{
			if (!has_value()) [[unlikely]]
				throw bad_expected_access<E>(m_error);
		}
		/** @copydoc value */
		constexpr void value() &&
		{
			if (!has_value()) [[unlikely]]
				throw bad_expected_access<E>(std::move(m_error));
		}
		/** @copydoc value */
		constexpr void value() const &&
		{
			if (!has_value()) [[unlikely]]
				throw bad_expected_access<E>(std::move(m_error));
		}

		/** Returns reference to the underlying unexpected value.
		 * @throw bad_expected_access<void> If the instance contains an expected value. */
		[[nodiscard]] constexpr E &error() &
		{
			if (has_value()) [[unlikely]]
				throw bad_expected_access<void>();
			return m_error;
		}
		/** @copydoc error */
		[[nodiscard]] constexpr const E &error() const &
		{
			if (has_value()) [[unlikely]]
				throw bad_expected_access<void>();
			return m_error;
		}
		/** @copydoc error */
		[[nodiscard]] constexpr E &&error() &&
		{
			if (has_value()) [[unlikely]]
				throw bad_expected_access<void>();
			return m_error;
		}
		/** @copydoc error */
		[[nodiscard]] constexpr const E &&error() const &&
		{
			if (has_value()) [[unlikely]]
				throw bad_expected_access<void>();
			return m_error;
		}

		template<typename T2, typename E2>
		[[nodiscard]] friend constexpr bool operator==(const expected &lhs, const expected<T2, E2> &rhs)
		{
			if (lhs.has_value() != rhs.has_value()) [[unlikely]]
				return false;
			else if (!lhs.has_value())
				return lhs.error() == rhs.error();
			else
				return true;
		}
		template<typename E2>
		[[nodiscard]] friend constexpr bool operator==(const expected &lhs, const unexpected<E2> &rhs)
		{
			return !lhs.has_value() && lhs.error() == rhs.error();
		}

		// clang-format off
		constexpr void swap(expected &other) noexcept(std::is_nothrow_move_constructible_v<E> && std::is_nothrow_swappable_v<E>)
		{
			using std::swap;
			if (has_value() != other.has_value()) [[unlikely]]
			{
				if (has_value())
					other.move_init(*this);
				else
					move_init(other);
			}
			else if (!has_value())
				swap(m_error, other.m_error);
			swap(m_has_value, other.m_has_value);
		}
		friend constexpr void swap(expected &a, expected &b) noexcept(std::is_nothrow_move_constructible_v<E> && std::is_nothrow_swappable_v<E>)
		{
			a.swap(b);
		}
		// clang-format on

	private:
		template<typename T2, typename E2>
		constexpr void copy_assign(const expected<T2, E2> &other)
		{
			if (has_value() != other.has_value())
			{
				destroy();
				copy_init(other);
				m_has_value = other.m_has_value;
			}
			else if (!has_value())
				m_error = other.m_error;
		}
		template<typename T2, typename E2>
		constexpr void move_assign(expected<T2, E2> &other)
		{
			if (has_value() != other.has_value())
			{
				destroy();
				move_init(other);
				m_has_value = other.m_has_value;
			}
			else if (!has_value())
				m_error = std::move(other.m_error);
		}
		template<typename T2, typename E2>
		constexpr void copy_init(const expected<T2, E2> &other)
		{
			if (!other.has_value()) std::construct_at(std::addressof(m_error), other.m_error);
		}
		template<typename T2, typename E2>
		constexpr void move_init(expected<T2, E2> &other)
		{
			if (!other.has_value()) std::construct_at(std::addressof(m_error), std::move(other.m_error));
		}

		constexpr void destroy() noexcept
		{
			if (!has_value()) std::destroy_at(std::addressof(m_error));
		}

		bool m_has_value;
		union
		{
			E m_error;
		};
	};
}	 // namespace sek
#else

#include <expected>

namespace sek
{
	/** Convenience alias for `std::unexpect_t` */
	using unexpect_t = std::unexpect_t;
	/** Convenience alias for `std::unexpect` */
	constexpr auto unexpect = std::unexpect;

	/** Convenience alias for `std::unexpected` */
	template<typename E>
	using unexpected = std::unexpected<E>;
	/** Convenience alias for `std::expected` */
	template<typename T, typename E>
	using expected = std::expected<T, E>;

	/** Convenience alias for `std::bad_expected_access` */
	template<typename E>
	using bad_expected_access = std::bad_expected_access<E>;
}	 // namespace sek
#endif