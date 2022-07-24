/*
 * Created by switchblade on 30/05/22
 */

#pragma once

#include <algorithm>
#include <compare>
#include <vector>

#include "sekhmet/detail/alloc_util.hpp"
#include "sekhmet/detail/assert.hpp"
#include "sekhmet/detail/ebo_base_helper.hpp"

#include "fwd.hpp"

namespace sek::engine
{
	/** @brief An entity is an internal ID used to refer to a group of components.
	 *
	 * Entities have an index, used to uniquely identify an entity, and a generation,
	 * used to disambiguate entities that have been previously "deleted" from their world.
	 * Entities that do not represent a valid group of components are "tombstone" entities.
	 * Tombstone entities always compare equal to each other. */
	class entity_t
	{
	public:
		typedef std::size_t value_type;

		/** @brief Structure used to represent an entity generation. */
		class generation_type
		{
			friend class entity_t;

			constexpr static value_type mask = sizeof(value_type) >= sizeof(std::uint64_t) ? 0xff'ffff : 0xffff;
			constexpr static value_type offset = sizeof(value_type) >= sizeof(std::uint64_t) ? 40 : 16;

		public:
			/** Returns tombstone value of entity generation. */
			[[nodiscard]] constexpr static generation_type tombstone() noexcept { return generation_type{mask}; }
			/** Returns maximum valid value of entity generation. */
			[[nodiscard]] constexpr static generation_type max() noexcept { return generation_type{mask - 1}; }

		public:
			constexpr generation_type() noexcept = default;

			/** Initializes an entity generation from an underlying value type.
			 * @note Value must be 24-bit max. */
			constexpr explicit generation_type(value_type value) noexcept : m_value(value << offset) {}

			/** Checks if the entity generation is a tombstone. */
			[[nodiscard]] constexpr bool is_tombstone() const noexcept { return *this == tombstone(); }

			/** Checks if the entity generation is valid. */
			[[nodiscard]] constexpr bool valid() const noexcept { return (m_value & mask) == mask; }
			/** Returns the underlying integer value of the generation. */
			[[nodiscard]] constexpr value_type value() const noexcept { return m_value >> offset; }

			[[nodiscard]] constexpr auto operator<=>(const generation_type &) const noexcept = default;
			[[nodiscard]] constexpr bool operator==(const generation_type &) const noexcept = default;

		private:
			value_type m_value = 0;
		};
		/** @brief Structure used to represent an entity index. */
		class index_type
		{
			friend class entity_t;

			constexpr static value_type mask = sizeof(value_type) == sizeof(std::uint64_t) ? 0xff'ffff'ffff : 0xffff;

		public:
			/** Returns tombstone value of entity index. */
			[[nodiscard]] constexpr static index_type tombstone() noexcept { return index_type{mask}; }
			/** Returns maximum valid value of entity index. */
			[[nodiscard]] constexpr static index_type max() noexcept { return index_type{mask - 1}; }

		public:
			constexpr index_type() noexcept = default;

			/** Initializes an entity index from an underlying value type.
			 * @note Value must be 40-bit max. */
			constexpr explicit index_type(value_type value) noexcept : m_value(value) {}

			/** Checks if the entity index is a tombstone. */
			[[nodiscard]] constexpr bool is_tombstone() const noexcept { return *this == tombstone(); }

			/** Returns the underlying integer value of the index. */
			[[nodiscard]] constexpr value_type value() const noexcept { return m_value; }

			[[nodiscard]] constexpr auto operator<=>(const index_type &) const noexcept = default;
			[[nodiscard]] constexpr bool operator==(const index_type &) const noexcept = default;

		private:
			value_type m_value = 0;
		};

		/** Returns value of an invalid entity. */
		[[nodiscard]] constexpr static entity_t tombstone() noexcept;

	public:
		/** Initializes an invalid entity. */
		constexpr entity_t() noexcept = default;
		/** Initializes an entity from an index and the default generation (0). */
		constexpr entity_t(index_type idx) noexcept : m_value(idx.m_value) {}
		/** Initializes an entity from a generation and an index. */
		constexpr entity_t(generation_type gen, index_type idx) noexcept : m_value(gen.m_value | idx.m_value) {}

		/** Checks if the entity is a tombstone. */
		[[nodiscard]] constexpr bool is_tombstone() const noexcept
		{
			return generation() == generation_type::tombstone();
		}

		/** Returns generation of the entity. */
		[[nodiscard]] constexpr generation_type generation() const noexcept
		{
			generation_type result;
			result.m_value = m_value & (generation_type::mask << generation_type::offset);
			return result;
		}
		/** Returns index of the entity. */
		[[nodiscard]] constexpr index_type index() const noexcept { return index_type{m_value & index_type::mask}; }
		/** Returns the underlying integer value of the entity. */
		[[nodiscard]] constexpr value_type value() const noexcept { return m_value; }

		[[nodiscard]] constexpr auto operator<=>(const entity_t &other) const noexcept
		{
			if (((m_value & other.m_value) >> generation_type::offset) == generation_type::mask)
				return std::strong_ordering::equivalent;
			else
				return m_value <=> other.m_value;
		}
		[[nodiscard]] constexpr bool operator==(const entity_t &other) const noexcept
		{
			return ((m_value & other.m_value) >> generation_type::offset) == generation_type::mask || m_value == other.m_value;
		}

	private:
		value_type m_value = 0;
	};

	constexpr entity_t entity_t::tombstone() noexcept
	{
		return {generation_type::tombstone(), index_type::tombstone()};
	}

	[[nodiscard]] constexpr hash_t hash(entity_t e) noexcept { return e.value(); }
}	 // namespace sek::engine

template<>
struct std::hash<sek::engine::entity_t>
{
	[[nodiscard]] constexpr sek::hash_t operator()(sek::engine::entity_t e) noexcept { return sek::engine::hash(e); }
};