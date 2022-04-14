//
// Created by switchblade on 2022-04-14.
//

#pragma once

#include <vector>

#include "archive_traits.hpp"
#include "manipulators.hpp"
#include "sekhmet/detail/hash.hpp"

namespace sek::serialization
{
	/** @details Archive used to read UBJson data. */
	class ubj_input_archive
	{
	public:
		typedef input_archive_category archive_category;
		typedef fixed_sequence_policy sequence_policy;
		typedef named_entry_policy entry_policy;

	private:
		/** Since object entries are stored in an array in order to improve cache performance & preserve order,
		 * object entry keys are pre-hashed to improve equality comparison performance. */
		struct ubj_object_entry_key
		{
			hash_t key_hash;
			std::string key_value;
		};

		struct ubj_entry
		{
		};

	public:
		typedef ubj_entry value_type;
		typedef std::ptrdiff_t difference_type;
		typedef std::size_t size_type;

	private:
	};

	static_assert(input_archive<ubj_input_archive, bool>);
	static_assert(input_archive<ubj_input_archive, char>);
	static_assert(input_archive<ubj_input_archive, std::uint8_t>);
	static_assert(input_archive<ubj_input_archive, std::int8_t>);
	static_assert(input_archive<ubj_input_archive, std::int16_t>);
	static_assert(input_archive<ubj_input_archive, std::int32_t>);
	static_assert(input_archive<ubj_input_archive, std::int64_t>);
	static_assert(input_archive<ubj_input_archive, float>);
	static_assert(input_archive<ubj_input_archive, double>);
	static_assert(input_archive<ubj_input_archive, std::string>);
	static_assert(container_like_archive<ubj_input_archive>);

	/** @details Archive used to write UBJson data. */
	class ubj_output_archive
	{
	};

	static_assert(output_archive<ubj_output_archive, bool>);
	static_assert(output_archive<ubj_output_archive, char>);
	static_assert(output_archive<ubj_output_archive, std::uint8_t>);
	static_assert(output_archive<ubj_output_archive, std::int8_t>);
	static_assert(output_archive<ubj_output_archive, std::int16_t>);
	static_assert(output_archive<ubj_output_archive, std::int32_t>);
	static_assert(output_archive<ubj_output_archive, std::int64_t>);
	static_assert(output_archive<ubj_output_archive, float>);
	static_assert(output_archive<ubj_output_archive, double>);
	static_assert(output_archive<ubj_output_archive, std::string>);
}	 // namespace sek::serialization