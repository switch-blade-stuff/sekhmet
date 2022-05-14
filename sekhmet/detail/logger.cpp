//
// Created by switchblade on 03/05/22.
//

#include "logger.hpp"

#include <cstdio>

namespace sek
{
	template<>
	std::atomic<logger *> &logger::info_ptr()
	{
		static auto instance = []()
		{
			logger result{info_cat};
			result += delegate{+[](std::string_view msg) { fputs(msg.data(), stdout); }};
			return result;
		}();
		static std::atomic<logger *> ptr = &instance;
		return ptr;
	}
	template<>
	std::atomic<logger *> &logger::warn_ptr()
	{
		static auto instance = []()
		{
			logger result{warn_cat};
			result += delegate{+[](std::string_view msg) { fputs(msg.data(), stdout); }};
			return result;
		}();
		static std::atomic<logger *> ptr = &instance;
		return ptr;
	}
	template<>
	std::atomic<logger *> &logger::error_ptr()
	{
		static auto instance = []()
		{
			logger result{error_cat};
			result += delegate{+[](std::string_view msg) { fputs(msg.data(), stderr); }};
			return result;
		}();
		static std::atomic<logger *> ptr = &instance;
		return ptr;
	}
	template<>
	std::atomic<logger *> &logger::fatal_ptr()
	{
		static auto instance = []()
		{
			logger result{fatal_cat};
			result += delegate{+[](std::string_view msg) { fputs(msg.data(), stderr); }};
			return result;
		}();
		static std::atomic<logger *> ptr = &instance;
		return ptr;
	}
}	 // namespace sek