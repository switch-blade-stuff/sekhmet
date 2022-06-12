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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 * ============================================================================
 *
 * Created by switchblade on 12/06/22
 */

#pragma once

#include "manipulators.hpp"
#include "util.hpp"

namespace sek::serialization::binary
{
	typedef int config_flags;
	constexpr config_flags no_flags = 0;

	/** Data is read & written in big-endian mode. */
	constexpr config_flags big_endian = 1;
	/** Data is read & written in little-endian mode. */
	constexpr config_flags little_endian = 2;

	/** @details Archive used to read non-structured binary data. */
	template<config_flags Config>
	class basic_input_archive
	{
	public:
		typedef input_archive_category archive_category;

	private:
	};

	typedef basic_input_archive<little_endian> input_archive;

	/** @details Archive used to write non-structured binary data. */
	template<config_flags Config>
	class basic_output_archive
	{
	public:
		typedef output_archive_category archive_category;

	private:
	};

	typedef basic_output_archive<little_endian> output_archive;
}	 // namespace sek::serialization::binary