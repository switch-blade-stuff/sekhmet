/*
 * Created by switchblade on 18/09/22
 */

#pragma once

#include <memory>
#include <utility>

#include "aligned_storage.hpp"

namespace sek
{
	/** @brief Helper smart pointer type used to reference an owned or external object.
	 * @tparam T Type of the referenced object. */
	template<typename T>
	class owned_ptr
	{
	public:
		typedef T value_type;
		typedef const T *pointer;
		typedef const T &reference;

	public:
		owned_ptr() = delete;
		owned_ptr(const owned_ptr &) = delete;
		owned_ptr &operator=(const owned_ptr &) = delete;

		/** Initializes the pointer to reference an external instance. */
		constexpr owned_ptr(const value_type *ptr) noexcept : m_ptr(ptr) {}

		// clang-format off
		/** Initializes a local owned instance by-move. */
		constexpr owned_ptr(value_type &&value) noexcept(std::is_nothrow_move_constructible_v<value_type>)
			requires std::move_constructible<value_type> { init(std::move(value)); }
		/** Initializes a local owned instance by-copy. */
		constexpr owned_ptr(const value_type &value) noexcept(std::is_nothrow_copy_constructible_v<value_type>)
			requires std::copy_constructible<value_type> { init(value); }

		/** Initializes a local owned instance from an initializer list.
		 * @param il Initializer list passed to object's constructor. */
		constexpr owned_ptr(std::initializer_list<T> il) requires std::constructible_from<T, std::initializer_list<T>>
		{
			init(il);
		}
		/** @copydoc owned_ptr */
		constexpr owned_ptr(std::initializer_list<owned_ptr> il) requires std::constructible_from<T, std::initializer_list<owned_ptr>>
		{
			init(il);
		}
		/** @copydoc owned_ptr
		 * @note This overload is available only if `std::same_as<T, U> || std::same_as<owned_ptr, U>` evaluates to `false`. */
		template<typename U>
		constexpr owned_ptr(std::initializer_list<U> il)
			requires std::constructible_from<T, std::initializer_list<U>> &&
					 (std::same_as<T, U> || std::same_as<owned_ptr, U>)
		{
			init(il);
		}

		/** Initializes a local owned instance from the passed arguments. */
		template<typename... Args>
		constexpr owned_ptr(Args &&...args) noexcept(std::is_nothrow_constructible_v<T, Args...>)
			requires std::constructible_from<T, Args...>
		{
			init(std::forward<Args>(args)...);
		}

		constexpr owned_ptr(owned_ptr &&other) noexcept(std::is_nothrow_move_constructible_v<value_type>)
			requires std::move_constructible<value_type>
		{
			if (other.owned()) [[likely]]
				init(std::move(*other.m_owned.template get<value_type>()));
			else
				m_ptr = other.m_ptr;
		}
		constexpr owned_ptr &operator=(owned_ptr &&other)
			noexcept(std::is_nothrow_move_constructible_v<value_type> && std::is_nothrow_move_assignable_v<value_type>)
			requires std::move_constructible<value_type> && std::is_move_assignable_v<value_type>
		{
			const auto other_owned = other.owned();
			const auto this_owned = owned();

			if (other_owned && this_owned)
				*m_owned.template get<value_type>() = std::move(*other.m_owned.template get<value_type>());
			else if (other_owned)
			{
				init(std::move(other.m_owned.template get<value_type>()));
				std::destroy_at(other.m_owned.template get<value_type>());
				other.m_ptr = std::exchange(m_ptr, nullptr);
			}
			else if (this_owned)
			{
				other.init(std::move(m_owned.template get<value_type>()));
				std::destroy_at(m_owned.template get<value_type>());
				m_ptr = std::exchange(other.m_ptr, nullptr);
			}
			else
				m_ptr = other.m_ptr;
		}
		// clang-format on

		constexpr ~owned_ptr()
		{
			if (owned()) [[likely]]
				std::destroy_at(m_owned.template get<value_type>());
		}

		/** Checks if the referenced object is locally owned by the pointer. */
		[[nodiscard]] constexpr bool owned() const { return m_ptr == nullptr; }

		/** Returns either a copy of the externally object or moves the locally owned object. */
		[[nodiscard]] constexpr value_type extract() &&
		{
			if (owned()) [[likely]]
				return std::move(*m_owned.template get<value_type>());
			else
				return *m_ptr;
		}
		/** @copydoc extract */
		[[nodiscard]] constexpr value_type extract() const &&
		{
			if (owned()) [[likely]]
				return std::move(*m_owned.template get<value_type>());
			else
				return *m_ptr;
		}

		/** Returns a const pointer to the external or locally owned object. */
		[[nodiscard]] constexpr pointer get() const { return owned() ? m_ptr : m_owned.template get<value_type>(); }
		/** @copydoc get */
		[[nodiscard]] constexpr pointer operator->() const { return get(); }
		/** Returns a const reference to the external or locally owned object. */
		[[nodiscard]] constexpr reference operator*() const { return *get(); }

		// clang-format off
		constexpr void swap(owned_ptr &other)
			noexcept(std::is_nothrow_move_constructible_v<value_type> && std::is_nothrow_swappable_v<value_type>)
			requires std::move_constructible<value_type> && std::is_swappable_v<value_type>
		{
			using std::swap;

			const auto other_owned = other.owned();
			const auto this_owned = owned();

			if (other_owned && this_owned)
				swap(*m_owned.template get<value_type>(), *other.m_owned.template get<value_type>());
			else if (other_owned)
			{
				init(std::move(other.m_owned.template get<value_type>()));
				std::destroy_at(other.m_owned.template get<value_type>());
				other.m_ptr = std::exchange(m_ptr, nullptr);
			}
			else if (this_owned)
			{
				other.init(std::move(m_owned.template get<value_type>()));
				std::destroy_at(m_owned.template get<value_type>());
				m_ptr = std::exchange(other.m_ptr, nullptr);
			}
			else
				swap(m_ptr, other.m_ptr);
		}
		friend constexpr void swap(owned_ptr &a, owned_ptr &b) noexcept(std::is_nothrow_swappable_v<owned_ptr>) { a.swap(b); }
		// clang-format on

	private:
		template<typename... Args>
		constexpr void init(Args &&...args) noexcept(std::is_nothrow_constructible_v<T, Args...>)
		{
			std::construct_at(m_owned.template get<value_type>(), std::forward<Args>(args)...);
		}

		mutable type_storage<value_type> m_owned = {};
		pointer m_ptr = nullptr;
	};
}	 // namespace sek