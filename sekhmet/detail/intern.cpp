//
// Created by switchblade on 10/05/22.
//

#include "intern.hpp"

namespace sek
{
	template class SEK_API_EXPORT basic_intern_pool<char>;
	template class SEK_API_EXPORT basic_intern_pool<wchar_t>;
	template class SEK_API_EXPORT basic_interned_string<char>;
	template class SEK_API_EXPORT basic_interned_string<wchar_t>;
}	 // namespace sek