/*
 * Created by switchblade on 03/05/22
 */

#include "logger.hpp"

#include <cstdio>

namespace sek::engine
{
	template<typename C>
	static basic_logger<C> make_logger(const auto &lvl_long, const auto &lvl_short)
	{
		constexpr static auto print_format = static_string_cast<C>("{}\n");
		using string_type = typename basic_logger<C>::string_type;

		basic_logger<C> result{lvl_long, lvl_short};
		result.on_log() += [](const string_type &msg) { fmt::print(print_format.data(), msg); };
		return result;
	}

	template<>
	basic_logger<char> &basic_logger<char>::info()
	{
		static auto instance = make_logger<char>("INFO", "I");
		return instance;
	}
	template<>
	basic_logger<char> &basic_logger<char>::warn()
	{
		static auto instance = make_logger<char>("WARN", "W");
		return instance;
	}
	template<>
	basic_logger<char> &basic_logger<char>::error()
	{
		static auto instance = make_logger<char>("ERROR", "E");
		return instance;
	}
	template<>
	basic_logger<char> &basic_logger<char>::fatal()
	{
		static auto instance = make_logger<char>("FATAL", "F");
		return instance;
	}

	template class SEK_API_EXPORT basic_logger<char>;

	template<>
	basic_logger<wchar_t> &basic_logger<wchar_t>::info()
	{
		static auto instance = make_logger<wchar_t>(L"INFO", L"I");
		return instance;
	}
	template<>
	basic_logger<wchar_t> &basic_logger<wchar_t>::warn()
	{
		static auto instance = make_logger<wchar_t>(L"WARN", L"W");
		return instance;
	}
	template<>
	basic_logger<wchar_t> &basic_logger<wchar_t>::error()
	{
		static auto instance = make_logger<wchar_t>(L"ERROR", L"E");
		return instance;
	}
	template<>
	basic_logger<wchar_t> &basic_logger<wchar_t>::fatal()
	{
		static auto instance = make_logger<wchar_t>(L"FATAL", L"F");
		return instance;
	}

	template class SEK_API_EXPORT basic_logger<wchar_t>;
}	 // namespace sek::engine