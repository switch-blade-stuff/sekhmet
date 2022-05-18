//
// Created by switchblade on 03/05/22.
//

#include "logger.hpp"

#include <cstdio>

namespace sek
{
	static logger make_logger(log_level level)
	{
		logger result{level};
		result.log_event += +[](std::string_view msg) { fputs(msg.data(), stdout); };
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