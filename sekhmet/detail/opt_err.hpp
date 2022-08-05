/*
 * Created by switchblade on 03/08/22
 */

#pragma once

#include <functional>
#include <optional>

namespace sek
{
	template<typename T, typename Err>
	class opt_err;

	/** @brief Helper type used to initialize `opt_err` with an error value. */
	template<typename E>
	class erropt_t
	{
		template<typename, typename>
		friend class opt_err;

	public:
		constexpr erropt_t() noexcept(std::is_nothrow_default_constructible_v<E>) = default;
		constexpr erropt_t(const erropt_t &) noexcept(std::is_nothrow_copy_constructible_v<E>) = default;
		constexpr erropt_t &operator=(const erropt_t &) noexcept(std::is_nothrow_copy_assignable_v<E>) = default;
		constexpr erropt_t(erropt_t &&) noexcept(std::is_nothrow_move_constructible_v<E>) = default;
		constexpr erropt_t &operator=(erropt_t &&) noexcept(std::is_nothrow_move_assignable_v<E>) = default;

		template<typename... Args>
		constexpr erropt_t(Args &&...args) noexcept(std::is_nothrow_constructible_v<E, Args &&...>)
			: m_error(std::forward<Args>(args)...)
		{
		}

		constexpr erropt_t(const E &value) noexcept(std::is_nothrow_copy_constructible_v<E>) : m_error(value) {}
		constexpr erropt_t(E &&value) noexcept(std::is_nothrow_move_constructible_v<E>) : m_error(std::move(value)) {}

	private:
		E m_error = {};
	};

	template<typename E>
	erropt_t(const E &) -> erropt_t<E>;
	template<typename E>
	erropt_t(E &&) -> erropt_t<E>;

	/** @brief Structure used to store an optional result and an error status.
	 *
	 * `opt_err` is made for situations where an error is an expected result of a function, but the erroneous status cannot
	 * be precisely represented via returning `nullptr` or `std::optional` and a more verbose error code is required.
	 *
	 * `opt_err` mirrors API of `std::optional`, with the only caveat being that it requires an error code instead of `std::nullopt`.
	 *
	 * @tparam T Type of non-error value stored by the `opt_err`.
	 * @tparam Err Error type stored by the `opt_err`. */
	template<typename T, typename Err>
	class opt_err
	{
		template<typename, typename>
		friend class opt_err;

		// clang-format off
		constexpr static void move_swap(opt_err *src, opt_err *dst)
			noexcept(std::is_nothrow_move_constructible_v<Err> &&
			         std::is_nothrow_move_constructible_v<T>)
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
		// clang-format on

		template<typename U, typename E>
		constexpr static bool allow_ctor =
			!(std::is_constructible_v<T, opt_err<U, E> &> || std::is_constructible_v<T, const opt_err<U, E> &> ||
			  std::is_constructible_v<T, opt_err<U, E> &&> || std::is_constructible_v<T, const opt_err<U, E> &&> ||
			  std::is_convertible_v<opt_err<U, E> &, T> || std::is_convertible_v<const opt_err<U, E> &, T> ||
			  std::is_convertible_v<opt_err<U, E> &&, T> || std::is_convertible_v<const opt_err<U, E> &&, T>);
		template<typename U, typename E>
		constexpr static bool allow_assign =
			!(std::is_constructible_v<T, opt_err<U, E> &> || std::is_constructible_v<T, const opt_err<U, E> &> ||
			  std::is_constructible_v<T, opt_err<U, E> &&> || std::is_constructible_v<T, const opt_err<U, E> &&> ||
			  std::is_convertible_v<opt_err<U, E> &, T> || std::is_convertible_v<const opt_err<U, E> &, T> ||
			  std::is_convertible_v<opt_err<U, E> &&, T> || std::is_convertible_v<const opt_err<U, E> &&, T> ||
			  std::is_assignable_v<T &, opt_err<U, E> &> || std::is_assignable_v<T &, const opt_err<U, E> &> ||
			  std::is_assignable_v<T &, opt_err<U, E> &&> || std::is_assignable_v<T &, const opt_err<U, E> &&>);

		template<typename U, typename E>
		constexpr static bool is_constructible = std::constructible_from<T, U> && std::constructible_from<Err, E>;
		template<typename U, typename E>
		constexpr static bool is_assignable = std::assignable_from<T, U> && std::assignable_from<Err, E>;

	public:
		/** Initializes `opt_err` with default-constructed error value. */
		constexpr opt_err() noexcept(std::is_nothrow_default_constructible_v<Err>) : m_error(), m_has_value(false) {}
		constexpr ~opt_err() noexcept { destroy(); }

		/** Copy-constructs the contained value. */
		constexpr opt_err(const opt_err &other) : m_has_value(other.m_has_value)
		{
			if (has_value())
				std::construct_at(std::addressof(m_value), other.m_value);
			else
				std::construct_at(std::addressof(m_error), other.m_error);
		}

		// clang-format off
		/** Move-constructs the contained value. */
		constexpr opt_err(opt_err &&other) noexcept(std::is_nothrow_move_constructible_v<T> && std::is_nothrow_move_constructible_v<Err>)
			: m_has_value(other.m_has_value)
		{
			if (has_value())
				std::construct_at(std::addressof(m_value), std::move(other.m_value));
			else
				std::construct_at(std::addressof(m_error), std::move(other.m_error));
		}
		// clang-format on

		/** Copy-assigns the `opt_err`. If both `this` and `other` contain the same value, copy-assigns said value.
		 * Otherwise, destroys the value contained by `this` and copy-constructs from `other`. */
		constexpr opt_err &operator=(const opt_err &other)
		{
			const auto other_valid = other.has_value();
			const auto this_valid = has_value();

			if (this_valid == other_valid)
			{
				if (this_valid)
					m_value = other.m_value;
				else
					m_error = other.m_error;
			}
			else
			{
				destroy();
				if (other_valid)
					std::construct_at(m_value, other.m_value);
				else
					std::construct_at(m_error, other.m_error);
			}
			m_has_value = other.m_has_value;
			return *this;
		}

		// clang-format off
		/** Copy-constructs the contained value from another `opt_err`. */
		template<typename U, typename E>
		constexpr opt_err(const opt_err<U, E> &other) requires(allow_ctor<U, Err> && is_constructible<const U &, const E &> && std::is_convertible_v<U &&, T>)
			: m_has_value(other.m_has_value)
		{
			if (has_value())
				std::construct_at(std::addressof(m_value), other.m_value);
			else
				std::construct_at(std::addressof(m_error), other.m_error);
		}
		/** @copydoc opt_err */
		template<typename U, typename E>
		constexpr explicit opt_err(const opt_err<U, E> &other) requires(allow_ctor<U, Err> && is_constructible<const U &, const E &>)
			: m_has_value(other.m_has_value)
		{
			if (has_value())
				std::construct_at(std::addressof(m_value), other.m_value);
			else
				std::construct_at(std::addressof(m_error), other.m_error);
		}

		/** Move-constructs the contained value from another `opt_err`. */
		template<typename U, typename E>
		constexpr opt_err(opt_err<U, E> &&other) noexcept(std::is_nothrow_constructible_v<T, U &&> && std::is_nothrow_constructible_v<Err, E &&>)
			requires(allow_ctor<U, Err> && is_constructible<U &&, E &&> && std::is_convertible_v<U &&, T>) : m_has_value(other.m_has_value)
		{
			if (has_value())
				std::construct_at(std::addressof(m_value), std::move(other.m_value));
			else
				std::construct_at(std::addressof(m_error), std::move(other.m_error));
		}
		/** @copydoc opt_err */
		template<typename U, typename E>
		constexpr explicit opt_err(opt_err<U, E> &&other) noexcept(std::is_nothrow_constructible_v<T, U &&> && std::is_nothrow_constructible_v<Err, E &&>)
			requires(allow_ctor<U, Err> && is_constructible<U &&, E &&>) : m_has_value(other.m_has_value)
		{
			if (has_value())
				std::construct_at(std::addressof(m_value), std::move(other.m_value));
			else
				std::construct_at(std::addressof(m_error), std::move(other.m_error));
		}

		/** Constructs the contained non-error value in-place. */
		template<typename... Args>
		constexpr explicit opt_err(std::in_place_type_t<T>, Args &&...args) noexcept(std::is_nothrow_constructible_v<T, Args &&...>)
			: m_value(std::forward<Args>(args)...), m_has_value(true)
		{
		}
		/** @copydoc opt_err */
		template<typename U, typename... Args>
		constexpr opt_err(std::in_place_type_t<T>, std::initializer_list<U> il, Args &&...args)
			noexcept(std::is_nothrow_constructible_v<T, std::initializer_list<U> &, Args &&...>)
			: m_value(il, std::forward<Args>(args)...), m_has_value(true)
	   {
	   }
		/** Constructs the contained error value in-place.
		 * @note This constructor participates in overload resolution only if `std::same_as<T, Err>` evaluates to `false`. */
		template<typename... Args>
		constexpr explicit opt_err(std::in_place_type_t<Err>, Args &&...args) noexcept(std::is_nothrow_constructible_v<Err, Args &&...>)
			requires(!std::same_as<T, Err>) : m_error(std::forward<Args>(args)...), m_has_value(false)
		{
		}

		/** Move-constructs `opt_err` with a non-error value. */
		template<typename U = T>
		constexpr opt_err(U &&value)  noexcept(std::is_nothrow_constructible_v<T, U &&>)
			requires(std::constructible_from<T, U &&> && std::is_convertible_v<U &&, T>)
			: m_value(std::forward<U>(value)), m_has_value(true)
		{
		}
		/** @copydoc opt_err */
		template<typename U = T>
		constexpr explicit opt_err(U &&value) noexcept(std::is_nothrow_constructible_v<T, U &&>)
			requires(std::constructible_from<T, U &&>) : m_value(std::forward<U>(value)), m_has_value(true)
		{
		}
		/** Move-constructs `opt_err` with an error value contained by `erropt_t`. */
		template<typename E = Err>
		constexpr opt_err(erropt_t<E> e) noexcept(std::is_nothrow_constructible_v<Err, E>)
			requires(std::constructible_from<Err, E>) : m_error(std::move(e.m_error)), m_has_value(false)
		{
		}

		/** Move-assigns the `opt_err`. If both `this` and `other` contain the same value, move-assigns said value.
		 * Otherwise, destroys the value contained by `this` and move-constructs from `other`. */
		constexpr opt_err &operator=(opt_err &&other) noexcept(is_constructible<T &&, Err &&> && is_assignable<T &&, Err &&>)
		{
			const auto other_valid = other.has_value();
			const auto this_valid = has_value();

			if (this_valid == other_valid)
			{
				if (this_valid)
					m_value = std::move(other.m_value);
				else
					m_error = std::move(other.m_error);
			}
			else
			{
				destroy();
				if (other_valid)
					std::construct_at(m_value, std::move(other.m_value));
				else
					std::construct_at(m_error, std::move(other.m_error));
			}
			m_has_value = other.m_has_value;
			return *this;
		}

		/** Move-assigns `opt_err` with a non-error value. If `opt_err` contains an error, destroys it and
		 * move-constructs the non-error value, otherwise move-assigns the non-error value. */
		template<typename U = T>
		constexpr opt_err &operator=(U &&value) noexcept(std::is_nothrow_assignable_v<T, U> && std::is_nothrow_constructible_v<T, U>)
			requires(std::assignable_from<T, U &&> && std::constructible_from<T, U &&>)
		{
			if (has_value())
				m_value = std::move(value);
			else
			{
				std::destroy_at(std::addressof(m_error));
				std::construct_at(m_value, std::move(value));
			}
			m_has_value = true;
			return *this;
		}
		/** Move-assigns `opt_err` with an error value contained by `erropt_t`. If `opt_err` contains a non-error value,
		 * destroys it and move-constructs the error value, otherwise move-assigns the error value. */
		template<typename E = Err>
		constexpr opt_err &operator=(erropt_t<E> e) noexcept(std::is_nothrow_assignable_v<Err, E> && std::is_nothrow_constructible_v<Err, E>)
			requires(std::assignable_from<Err, E &&> && std::constructible_from<Err, E &&>)
		{
			if (!has_value())
				m_error = std::move(std::move(e.m_error));
			else
			{
				std::destroy_at(std::addressof(m_value));
				std::construct_at(m_error, std::move(e.m_error));
			}
			m_has_value = false;
			return *this;
		}
		/** Move-assigns `opt_err` with an error value. If `opt_err` contains a non-error value,
		 * destroys it and move-constructs the error value, otherwise move-assigns the error value.
		 * @note This operator participates in overload resolution only if `std::same_as<T, Err>` evaluates to `false`. */
		template<typename E = Err>
		constexpr opt_err &operator=(E &&e) noexcept(std::is_nothrow_assignable_v<Err, E> && std::is_nothrow_constructible_v<Err, E>)
			requires(std::assignable_from<Err, E &&> && std::constructible_from<Err, E &&> && !std::same_as<T, Err>)
		{
			if (!has_value())
				m_error = std::move(std::move(e.m_error));
			else
			{
				std::destroy_at(std::addressof(m_value));
				std::construct_at(m_error, std::move(e.m_error));
			}
			m_has_value = false;
			return *this;
		}
		// clang-format on

		template<typename U, typename E>
		constexpr opt_err &operator=(const opt_err<U, E> &other)
			requires(allow_assign<U, E> && is_constructible<const U &, const E &> && is_assignable<const U &, const E &>)
		{
			const auto other_valid = other.has_value();
			const auto this_valid = has_value();

			if (std::is_assignable_v<T, const U &> && std::is_assignable_v<Err, const E &> && this_valid == other_valid)
			{
				if (this_valid)
					m_value = other.m_value;
				else
					m_error = other.m_error;
			}
			else
			{
				destroy();
				if (other_valid)
					std::construct_at(m_value, other.m_value);
				else
					std::construct_at(m_error, other.m_error);
			}
			m_has_value = other.m_has_value;
			return *this;
		}
		template<typename U, typename E>
		constexpr opt_err &operator=(opt_err<U, E> &&other)
			requires(allow_assign<U, E> && is_constructible<U, E &&> && is_assignable<U, E &&>)
		{
			const auto other_valid = other.has_value();
			const auto this_valid = has_value();

			if (std::is_assignable_v<T, U> && std::is_assignable_v<Err, E> && this_valid == other_valid)
			{
				if (this_valid)
					m_value = std::move(other.m_value);
				else
					m_error = std::move(other.m_error);
			}
			else
			{
				destroy();
				if (other_valid)
					std::construct_at(m_value, std::move(other.m_value));
				else
					std::construct_at(m_error, std::move(other.m_error));
			}
			m_has_value = other.m_has_value;
			return *this;
		}

		/** Returns `true` if `opt_err` contains a value, `false` if it contains an error. */
		[[nodiscard]] constexpr bool has_value() const noexcept { return m_has_value; }
		/** @copydoc has_value */
		[[nodiscard]] constexpr operator bool() const noexcept { return has_value(); }

		/** Returns pointer to the contained (non-error) value. */
		[[nodiscard]] constexpr T *operator->() noexcept { return std::addressof(m_value); }
		/** @copydoc operator-> */
		[[nodiscard]] constexpr const T *operator->() const noexcept { return std::addressof(m_value); }

		/** Returns reference to the contained (non-error) value. */
		[[nodiscard]] constexpr T &operator*() &noexcept { return m_value; }
		/** @copydoc operator* */
		[[nodiscard]] constexpr const T &operator*() const &noexcept { return m_value; }
		/** @copydoc operator* */
		[[nodiscard]] constexpr T &&operator*() &&noexcept { return std::move(m_value); }
		/** @copydoc operator* */
		[[nodiscard]] constexpr const T &&operator*() const &&noexcept { return std::move(m_value); }

		/** @copydoc operator*
		 * @throw std::bad_optional_access if `opt_err` contains an error. */
		[[nodiscard]] constexpr T &value() &
		{
			if (!has_value()) [[unlikely]]
				throw std::bad_optional_access();
			return operator*();
		}
		/** @copydoc value */
		[[nodiscard]] constexpr const T &value() const &
		{
			if (!has_value()) [[unlikely]]
				throw std::bad_optional_access();
			return operator*();
		}
		/** @copydoc value */
		[[nodiscard]] constexpr T &&value() &&
		{
			if (!has_value()) [[unlikely]]
				throw std::bad_optional_access();
			return operator*();
		}
		/** @copydoc value */
		[[nodiscard]] constexpr const T &&value() const &&
		{
			if (!has_value()) [[unlikely]]
				throw std::bad_optional_access();
			return operator*();
		}

		/** Returns contained (non-error) value or, if `opt_err` contains an error, returns the default value. */
		template<typename U>
		[[nodiscard]] constexpr T value_or(U &&def) const &
		{
			return has_value() ? operator*() : static_cast<T>(std::forward<U>(def));
		}
		/** @copydoc value_or */
		template<typename U>
		[[nodiscard]] constexpr T value_or(U &&def) &&
		{
			return has_value() ? std::move(operator*()) : static_cast<T>(std::forward<U>(def));
		}

		/** Returns reference to the contained error.
		 * @throw std::bad_optional_access if `opt_err` does not contain an error. */
		[[nodiscard]] constexpr const Err &error() const
		{
			if (has_value()) [[unlikely]]
				throw std::bad_optional_access();
			return m_error;
		}
		/** Casts `opt_err` to error type.
		 * @throw std::bad_optional_access if `opt_err` does not contain an error. */
		[[nodiscard]] constexpr operator Err() const { return error(); }

		/** Resets the contained value or error.
		 * @param e New value for the error. */
		constexpr void reset(Err e = Err{}) noexcept(std::is_nothrow_default_constructible_v<Err>)
		{
			if (has_value())
			{
				std::destroy_at(std::addressof(m_value));
				std::construct_at(std::addressof(m_error), std::move(e));
			}
			else
				m_error = std::move(e);
		}

		/** Constructs the contained value in-place. */
		template<typename... Args>
		constexpr T &emplace(Args &&...args)
			requires std::constructible_from<T, Args &&...>
		{
			destroy();
			std::construct_at(std::addressof(m_value), std::forward<Args>(args)...);
			m_has_value = true;
		}
		/** @copydoc emplace */
		template<typename U, typename... Args>
		constexpr T &emplace(std::initializer_list<U> il, Args &&...args)
			requires std::constructible_from<T, std::initializer_list<U> &, Args &&...>
		{
			destroy();
			std::construct_at(std::addressof(m_value), il, std::forward<Args>(args)...);
			m_has_value = true;
		}

		// clang-format off
		constexpr void swap(opt_err &other)
			noexcept(std::is_nothrow_move_constructible_v<Err> && std::is_nothrow_swappable_v<Err> &&
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
		}
		friend constexpr void swap(opt_err &a, opt_err &b)
			noexcept(std::is_nothrow_move_constructible_v<Err> && std::is_nothrow_swappable_v<Err> &&
					 std::is_nothrow_move_constructible_v<T> && std::is_nothrow_swappable_v<T>)
		{
			a.swap(b);
		}
		// clang-format on

	private:
		constexpr void destroy() noexcept
		{
			if (has_value())
				std::destroy_at(std::addressof(m_value));
			else
				std::destroy_at(std::addressof(m_error));
		}

		union
		{
			Err m_error;
			T m_value;
		};

		bool m_has_value = false;
	};
}	 // namespace sek