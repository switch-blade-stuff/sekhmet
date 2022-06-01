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
 * Created by switchblade on 31/05/22
 */

#include "compound_exception.hpp"

namespace sek
{
	void compound_exception::push(std::exception_ptr ptr) { exceptions.push_back(std::move(ptr)); }
	std::string compound_exception::message() const
	{
		std::string result;
		for (std::size_t i = 0; auto &ptr : exceptions)
		{
			result.append(1, '[').append(std::to_string(i++)).append("] ");
			try
			{
				std::rethrow_exception(ptr);
			}
			catch (std::exception &e)
			{
				result.append("what(): \"").append(e.what()).append("\"\n");
			}
			catch (...)
			{
				result.append("Non `std::exception`-derived exception\n");
			}
		}
		return result;
	}
}	 // namespace sek