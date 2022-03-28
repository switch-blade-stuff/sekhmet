//
// Created by switchblade on 2021-11-06.
//

#pragma once

/* Platform-dependant definitions. */

#if defined(_WIN32) || defined(_WIN64)
#define SEK_OS_WIN
#endif

#ifdef __unix__
#define SEK_OS_UNIX
#endif

#if defined(__linux__) || defined(linux) || defined(__linux)
#define SEK_OS_LINUX
#endif

#ifdef __APPLE__
#define SEK_OS_APPLE
#ifndef SEK_OS_UNIX
#define SEK_OS_UNIX
#endif
#endif

#ifdef _MSC_VER

#define SEK_PRETTY_FUNC __FUNCSIG__
#define SEK_NO_VLA

#define SEK_FORCE_INLINE __forceinline
#define SEK_NEVER_INLINE __declspec(noinline)
#define SEK_VECTORCALL __vectorcall

#elif defined(__clang__) || defined(__GNUC__)

#define SEK_PRETTY_FUNC __PRETTY_FUNCTION__

#define SEK_FORCE_INLINE __attribute__((always_inline)) inline
#define SEK_NEVER_INLINE __attribute__((noinline))
#define SEK_VECTORCALL

#else

#define SEK_COUNTER 0

#endif

#define SEK_FUNC __FUNCTION__
#define SEK_FILE __FILE__
#define SEK_LINE __LINE__
#define SEK_COUNTER __COUNTER__

#ifdef SEK_OS_WIN

#define SEK_PATH_SEPARATOR '\\'

#if defined(_MSC_VER)
#define SEK_API_EXPORT __declspec(dllexport)
#define SEK_API_IMPORT __declspec(dllimport)
#elif defined(__clang__) || defined(__GNUC__)
#define SEK_API_EXPORT __attribute__(dllexport)
#define SEK_API_IMPORT __attribute__(dllimport)
#endif

#ifdef SEK_IMPORT_LIBRARY
#define SEK_API SEK_API_IMPORT
#else
#define SEK_API SEK_API_EXPORT
#endif

#else

#define SEK_PATH_SEPARATOR '/'
#define SEK_API

#endif

#ifdef SEK_OS_UNIX
#include <unistd.h>
#endif

#ifdef _POSIX_VERSION
#define SEK_OS_POSIX
#endif

#ifdef SEK_OS_POSIX
#include <sys/types.h>
#endif

#ifndef SSIZE_MAX
#if SIZE_MAX > INT32_MAX
typedef int64_t ssize_t;
#define SSIZE_MAX INT64_MAX
#else
typedef int32_t ssize_t;
#define SSIZE_MAX INT32_MAX
#endif
#endif