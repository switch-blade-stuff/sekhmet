//
// Created by switchblade on 30/05/22.
//

#pragma once

namespace sek::detail
{
	typedef int native_openmode;
}

#ifdef SEK_OS_WIN
#include "win/native_util.hpp"
#elif defined(SEK_OS_UNIX)
#include "unix/native_util.hpp"
#endif