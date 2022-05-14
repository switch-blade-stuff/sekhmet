//
// Created by switchblade on 10/05/22.
//

#include "intern.hpp"

namespace sek
{
	template<>
	basic_intern_pool<char> &basic_intern_pool<char>::global()
	{
		static basic_intern_pool instance;
		return instance;
	}
	template<>
	basic_intern_pool<wchar_t> &basic_intern_pool<wchar_t>::global()
	{
		static basic_intern_pool instance;
		return instance;
	}
}	 // namespace sek