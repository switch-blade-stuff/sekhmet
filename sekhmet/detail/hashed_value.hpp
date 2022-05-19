//
// Created by switchblade on 19/05/22.
//

#pragma once

#include "ebo_base_helper.hpp"
#include "hash.hpp"

namespace sek
{
	template<typename T, typename Hash = default_hash>
	class hashed_value : ebo_base_helper<Hash>, ebo_base_helper<T>
	{
		using value_base = ebo_base_helper<T>;
		using hash_base = ebo_base_helper<Hash>;

	public:
		typedef T value_type;
		typedef Hash hash_type;

	public:
		// clang-format off
		constexpr hashed_value() noexcept(noexcept(value_base{}) && noexcept(hash_base{})) { rehash(); }
		constexpr explicit hashed_value(const hash_base &hash)
			noexcept(noexcept(value_base{}) && std::is_nothrow_copy_constructible_v<hash_base>)
			: hash_base{hash}
		{
			rehash();
		}

		constexpr hashed_value(const hashed_value &other)
			noexcept(std::is_nothrow_copy_constructible_v<hash_base> && std::is_nothrow_copy_constructible_v<value_base>)
			: hash_base{std::forward<hash_base>(other)},
			  value_base(std::forward<value_base>(other))
		{
			rehash();
		}
		constexpr hashed_value &operator=(const hashed_value &other)
			noexcept(std::is_nothrow_copy_assignable_v<hash_base> && std::is_nothrow_copy_assignable_v<value_base>)
		{
			hash_base::operator=(other);
			value_base::operator=(other);
			rehash();
			return *this;
		}
		constexpr hashed_value(hashed_value &&other)
			noexcept(std::is_nothrow_move_constructible_v<hash_base> &&	std::is_nothrow_move_constructible_v<value_base>)
			: hash_base{std::forward<hash_base>(other)},
			  value_base(std::forward<value_base>(other)),
			  hash_value(other.hash_value)
		{
		}
		constexpr hashed_value &operator=(hashed_value &&other)
			noexcept(std::is_nothrow_move_assignable_v<hash_base> && std::is_nothrow_move_assignable_v<value_base>)
		{
			hash_base::operator=(std::move(other));
			value_base::operator=(std::move(other));
			hash_value = other.hash_value;
			return *this;
		}

		template<typename... Args>
		constexpr hashed_value(Args &&...args)
			noexcept(std::is_nothrow_constructible_v<value_base, Args...>)
			requires std::is_constructible_v<value_base, Args...>
			: value_base(std::forward<Args>(args)...)
		{
			rehash();
		}
		template<typename... Args>
		constexpr hashed_value(const hash_type &hash, Args &&...args)
			noexcept(std::is_nothrow_constructible_v<value_base, Args...>)
			requires std::is_constructible_v<value_base, Args...>
			: hash_base{hash}, value_base(std::forward<Args>(args)...)
		{
			rehash();
		}
		// clang-format on

		/** Returns reference to the value. */
		[[nodiscard]] constexpr value_type &value() noexcept { return *value_base::get(); }
		/** @copydoc value */
		[[nodiscard]] constexpr const value_type &value() const noexcept { return *value_base::get(); }
		/** Returns hash of the value. */
		[[nodiscard]] constexpr hash_t hash() const noexcept { return hash_value; }

		/** Returns reference to the hash function. */
		[[nodiscard]] constexpr const value_type &hash_function() const noexcept { return *value_base::get(); }

	private:
		constexpr void rehash() noexcept { hash_value = hash_function()(value()); }

		hash_t hash_value;
	};
}	 // namespace sek
