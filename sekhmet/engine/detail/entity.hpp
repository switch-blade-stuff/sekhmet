/*
 * Created by switchblade on 30/05/22
 */

#pragma once

#include <compare>

#include "sekhmet/detail/hash.hpp"

namespace sek::engine
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
			constexpr explicit generation_type(value_type value) noexcept : m_value(value << offset) {}

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
			friend class entity;

			constexpr static auto max_val = 0xff'ffff'ffff;

		public:
			/** Returns maximum value of entity index. */
			[[nodiscard]] constexpr static index_type max() noexcept { return index_type{max_val}; }

		public:
			constexpr index_type() noexcept = default;

			/** Initializes an entity index from an underlying value type.
			 * @note Value must be 40-bit max. */
			constexpr explicit index_type(value_type value) noexcept : m_value(value) {}

			/** Returns the underlying integer value of the index. */
			[[nodiscard]] constexpr value_type value() const noexcept { return m_value; }

			[[nodiscard]] constexpr auto operator<=>(const index_type &) const noexcept = default;
			[[nodiscard]] constexpr bool operator==(const index_type &) const noexcept = default;

		private:
			value_type m_value = 0;
		};

		/** Returns value of an invalid entity. */
		[[nodiscard]] constexpr static entity invalid() noexcept { return {}; }

	public:
		/** Initializes an invalid entity. */
		constexpr entity() noexcept = default;
		/** Initializes an entity from an index and the default generation (0). */
		constexpr entity(index_type idx) noexcept : m_value(idx.m_value) {}
		/** Initializes an entity from a generation and an index. */
		constexpr entity(generation_type gen, index_type idx) noexcept : m_value(gen.m_value | idx.m_value) {}

		/** Checks if the entity valid (invalid entities have generation of `0xff'ffff`). */
		[[nodiscard]] constexpr bool valid() const noexcept { return generation() == generation_type::invalid(); }

		/** Returns generation of the entity. */
		[[nodiscard]] constexpr generation_type generation() const noexcept
		{
			generation_type result;
			result.m_value = m_value & generation_type::mask;
			return result;
		}
		/** Returns index of the entity. */
		[[nodiscard]] constexpr index_type index() const noexcept { return index_type{m_value & index_type::max_val}; }
		/** Returns the underlying integer value of the entity. */
		[[nodiscard]] constexpr value_type value() const noexcept { return m_value; }

		[[nodiscard]] constexpr auto operator<=>(const entity &other) const noexcept
		{
			if (((m_value & other.m_value) >> generation_type::offset) == generation_type::inv_val)
				return std::strong_ordering::equivalent;
			else
				return m_value <=> other.m_value;
		}
		[[nodiscard]] constexpr bool operator==(const entity &other) const noexcept
		{
			return ((m_value & other.m_value) >> generation_type::offset) == generation_type::inv_val ||
				   m_value == other.m_value;
		}

	private:
		value_type m_value = 0;
	};

	[[nodiscard]] constexpr hash_t hash(entity e) noexcept { return e.value(); }
}	 // namespace sek::engine

template<>
struct std::hash<sek::engine::entity>
{
	[[nodiscard]] constexpr sek::hash_t operator()(sek::engine::entity e) noexcept { return sek::engine::hash(e); }
};