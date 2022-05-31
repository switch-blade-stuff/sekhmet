//
// Created by switchblade on 30/05/22.
//

#include "native_file.hpp"

#ifdef SEK_OS_WIN
#include "win/native_file_handle.cpp"
#elif defined(SEK_OS_UNIX)
#include "unix/native_file_handle.cpp"
#endif