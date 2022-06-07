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
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 * ============================================================================
 *
 * Created by switchblade on 03/05/22
 */

#include "logger.hpp"

#include <cstdio>

namespace sek::engine
{
	static logger make_logger(log_level level)
	{
		logger result{level};
		result.on_log() += +[](std::string_view msg) { fputs(msg.data(), stdout); };
		return result;
	}

	template<>
	template<>
	std::atomic<logger *> &logger::global_ptr<log_level::info>()
	{
		static auto instance = make_logger(log_level::info);
		static std::atomic<logger *> ptr = &instance;
		return ptr;
	}
	template<>
	template<>
	std::atomic<logger *> &logger::global_ptr<log_level::warn>()
	{
		static auto instance = make_logger(log_level::warn);
		static std::atomic<logger *> ptr = &instance;
		return ptr;
	}
	template<>
	template<>
	std::atomic<logger *> &logger::global_ptr<log_level::error>()
	{
		static auto instance = make_logger(log_level::error);
		static std::atomic<logger *> ptr = &instance;
		return ptr;
	}
	template<>
	template<>
	std::atomic<logger *> &logger::global_ptr<log_level::fatal>()
	{
		static auto instance = make_logger(log_level::fatal);
		static std::atomic<logger *> ptr = &instance;
		return ptr;
	}
}	 // namespace sek