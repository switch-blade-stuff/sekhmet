//
// Created by switchblade on 2022-04-07.
//

#include "filemap.hpp"

#ifdef SEK_OS_WIN
#include "win/filemap_handle.cpp"
#elif defined(SEK_OS_UNIX)
#include "unix/filemap_handle.cpp"
#endif