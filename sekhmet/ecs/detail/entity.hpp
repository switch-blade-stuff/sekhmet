/*
 * ============================================================================
 * Sekhmet - C++20 game engine & editor
 * Copyright (C) 2022 switchblade
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 * ============================================================================
 *
 * Created by switchblade on 30/05/22
 */

#pragma once

#include <compare>

#include "sekhmet/detail/hash.hpp"

namespace sek::ecs
{
	/** @brief An entity is an internal ID used to refer to a group of components.
	 *
	 * Entities have an index, used to uniquely identify an entity, and a generation,
	 * used to disambiguate entities that have been previously "deleted" from their world.
	 * Entities that do not represent a valid group of components are "invalid" entities,
	 * Such entities have generation of `0xff'ffff`, with the index being irrelevant.
	 * Invalid entities always compare equal to each other. */
	class entity
	{
	public:
		typedef std::uint64_t value_type;

		/** @brief Structure used to represent an entity generation. */
		class generation_type
		{
			friend class entity;

			constexpr static value_type inv_val = 0xff'ffff;
			constexpr static value_type offset = 40;
			constexpr static value_type mask = inv_val << offset;

		public:
			/** Returns value of the entity generation used to indicate that an entity is `invalid`. */
			[[nodiscard]] constexpr static generation_type invalid() noexcept { return generation_type{inv_val}; }
			/** Returns maximum value of entity generation (same as invalid). */
			[[nodiscard]] constexpr static generation_type max() noexcept { return invalid(); }

		public:
			constexpr generation_type() noexcept = default;

			/** Initializes an entity generation from an underlying value type.
			 * @note Value must be 24-bit max. */
			constexpr explicit generation_type(value_type value) noexcept : gen_value(value << offset) {}

			/** Checks if the entity generation is valid. */
			[[nodiscard]] constexpr bool valid() const noexcept { return (gen_value & mask) == mask; }
			/** Returns the underlying integer value of the generation. */
			[[nodiscard]] constexpr value_type value() const noexcept { return gen_value >> offset; }

			[[nodiscard]] constexpr auto operator<=>(const generation_type &) const noexcept = default;
			[[nodiscard]] constexpr bool operator==(const generation_type &) const noexcept = default;

		private:
			value_type gen_value = 0;
		};
		/** @brief Structure used to represent an entity index. */
		class index_type
		{
			friend class entity;

			constexpr static auto max_val = 0xff'ffff'ffff;

		public:
			/** Returns maximum value of entity index. */
			[[nodiscard]] constexpr static index_type max() noexcept { return index_type{max_val}; }

		public:
			constexpr index_type() noexcept = default;

			/** Initializes an entity index from an underlying value type.
			 * @note Value must be 40-bit max. */
			constexpr explicit index_type(value_type value) noexcept : idx_value(value) {}

			/** Returns the underlying integer value of the index. */
			[[nodiscard]] constexpr value_type value() const noexcept { return idx_value; }

			[[nodiscard]] constexpr auto operator<=>(const index_type &) const noexcept = default;
			[[nodiscard]] constexpr bool operator==(const index_type &) const noexcept = default;

		private:
			value_type idx_value = 0;
		};

		/** Returns value of an invalid entity. */
		[[nodiscard]] constexpr static entity invalid() noexcept { return {}; }

	public:
		/** Initializes an invalid entity. */
		constexpr entity() noexcept = default;
		/** Initializes an entity from an index and the default generation (0). */
		constexpr entity(index_type idx) noexcept : ent_value(idx.idx_value) {}
		/** Initializes an entity from a generation and an index. */
		constexpr entity(generation_type gen, index_type idx) noexcept : ent_value(gen.gen_value | idx.idx_value) {}

		/** Checks if the entity valid (invalid entities have generation of `0xff'ffff`). */
		[[nodiscard]] constexpr bool valid() const noexcept { return generation() == generation_type::invalid(); }

		/** Returns generation of the entity. */
		[[nodiscard]] constexpr generation_type generation() const noexcept
		{
			generation_type result;
			result.gen_value = ent_value & generation_type::mask;
			return result;
		}
		/** Returns index of the entity. */
		[[nodiscard]] constexpr index_type index() const noexcept
		{
			return index_type{ent_value & index_type::max_val};
		}
		/** Returns the underlying integer value of the entity. */
		[[nodiscard]] constexpr value_type value() const noexcept { return ent_value; }

		[[nodiscard]] constexpr auto operator<=>(const entity &other) const noexcept
		{
			if (((ent_value & other.ent_value) >> generation_type::offset) == generation_type::inv_val)
				return std::strong_ordering::equivalent;
			else
				return ent_value <=> other.ent_value;
		}
		[[nodiscard]] constexpr bool operator==(const entity &other) const noexcept
		{
			return ((ent_value & other.ent_value) >> generation_type::offset) == generation_type::inv_val ||
				   ent_value == other.ent_value;
		}

	private:
		value_type ent_value = 0;
	};

	[[nodiscard]] constexpr hash_t hash(entity e) noexcept { return e.value(); }
}	 // namespace sek::ecs

template<>
struct std::hash<sek::ecs::entity>
{
	[[nodiscard]] constexpr sek::hash_t operator()(sek::ecs::entity e) noexcept { return sek::ecs::hash(e); }
};