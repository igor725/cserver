/**
 * @file core.h
 * @author igor725 (gvaldovigor@gmail.com)
 * @brief Файл с основными дефайнами сервера, тут
 * определены типы, макросы, макро-функции и всякие
 * другие приколы, необходимые для работы сервера.
 * Файл этот желательно подключать в каждом плагине
 * и использовать те типы, которые определены в нём.
 * 
 * @copyright Copyright (c) 2022
 * 
 */
#ifndef CORE_H
#define CORE_H

// Определяем, где мы компилимся
#ifndef CORE_MANUAL_BACKENDS
#	if defined(__MINGW32__)
#		define CORE_USE_WINDOWS
#		define CORE_USE_WINDOWS_PATHS
#		define CORE_USE_UNIX_DEFINES
#		define CORE_USE_UNIX_TYPES
#		define HTTP_USE_WININET_BACKEND
#		define HASH_USE_WINCRYPT_BACKEND
#	elif defined(_WIN32)
#		define CORE_USE_WINDOWS
#		define CORE_USE_WINDOWS_PATHS
#		define CORE_USE_WINDOWS_DEFINES
#		define CORE_USE_WINDOWS_TYPES
#		define HTTP_USE_WININET_BACKEND
#		define HASH_USE_WINCRYPT_BACKEND
#	elif defined(__APPLE__)
#		define CORE_USE_UNIX
#		define CORE_USE_DARWIN
#		define CORE_USE_DARWIN_PATHS
#		define CORE_USE_UNIX_DEFINES
#		define CORE_USE_UNIX_TYPES
#		define HTTP_USE_CURL_BACKEND
#		define HASH_USE_CRYPTO_BACKEND
#	elif defined(__ANDROID__)
#		define CORE_USE_UNIX
#		define CORE_USE_UNIX_PATHS
#		define CORE_USE_UNIX_DEFINES
#		define CORE_USE_UNIX_TYPES
#		define HTTP_USE_CURL_BACKEND
#		define HASH_USE_CRYPTO_BACKEND
#	elif defined(__linux__)
#		define CORE_USE_UNIX
#		define CORE_USE_LINUX
#		define CORE_USE_UNIX_PATHS
#		define CORE_USE_UNIX_DEFINES
#		define CORE_USE_UNIX_TYPES
#		define HTTP_USE_CURL_BACKEND
#		define HASH_USE_CRYPTO_BACKEND
#	elif defined(__unix__)
#		define CORE_USE_UNIX
#		define CORE_USE_UNIX_PATHS
#		define CORE_USE_UNIX_DEFINES
#		define CORE_USE_UNIX_TYPES
#		define HTTP_USE_CURL_BACKEND
#		define HASH_USE_CRYPTO_BACKEND
#	endif
#endif

#if defined(CORE_USE_WINDOWS_PATHS)
#	define PATH_DELIM "\\"
#	define DLIB_EXT "dll"
#	define _CRT_SECURE_NO_WARNINGS
#	define WIN32_LEAN_AND_MEAN
#elif defined(CORE_USE_UNIX_PATHS)
#	define PATH_DELIM "/"
#	define DLIB_EXT "so"
#elif defined(CORE_USE_DARWIN_PATHS)
#	define PATH_DELIM "/"
#	define DLIB_EXT "dylib"
#	define PATH_MAX 260 // TODO: Убрать это отсюдова
#else
#	error This file wants to be hacked
#endif // CORE_USE_*_PATHS

#if defined(CORE_USE_WINDOWS_DEFINES)
#	define NOINL __declspec(noinline)
#	ifndef CORE_BUILD_PLUGIN
#		define API __declspec(dllexport, noinline)
#		define VAR __declspec(dllexport)
#	else
#		define API __declspec(dllimport) extern
#		define VAR __declspec(dllimport) extern
#		define EXP __declspec(dllexport)
#		define EXPF __declspec(dllexport)
#	endif
#elif defined(CORE_USE_UNIX_DEFINES)
#	define NOINL __attribute__((noinline))
#	ifndef CORE_BUILD_PLUGIN
#		ifdef _WIN32
#			define API __attribute__((dllexport, noinline))
#			define VAR __attribute__((dllexport)) extern
#		else
#			define API __attribute__((__visibility__("default"), noinline))
#			define VAR __attribute__((__visibility__("default"))) extern
#		endif
#	else
#		ifdef _WIN32
#			define API __attribute__((dllimport)) extern
#			define VAR __attribute__((dllimport)) extern
#			define EXP __attribute__((dllexport)) extern
#			define EXPF __attribute__((dllexport, noinline))
#		else
#			define API extern
#			define VAR extern
#			define EXP __attribute__((__visibility__("default"))) extern
#			define EXPF __attribute__((__visibility__("default"), noinline))
#		endif
#	endif

#	define min(a, b) (((a)<(b))?(a):(b))
#	define max(a, b) (((a)>(b))?(a):(b))
#	define MAX_PATH  PATH_MAX
#endif // CORE_USE_*_TYPES

#if defined(CORE_USE_WINDOWS_TYPES)
#	if _MSC_VER
		typedef signed __int8 cs_int8;
		typedef signed __int16 cs_int16;
		typedef signed __int32 cs_int32;
		typedef signed __int64 cs_int64;
		typedef unsigned __int8 cs_byte;
		typedef unsigned __int16 cs_uint16;
		typedef unsigned __int32 cs_uint32;
		typedef unsigned __int64 cs_uint64;
#		ifdef _WIN64
			typedef unsigned __int64 cs_uintptr, cs_size;
#		else
			typedef unsigned int cs_uintptr, cs_size;
#		endif
#	else
#		error CORE_USE_WINDOWS_TYPES can be used only with MSVC.
#	endif
#elif defined(CORE_USE_UNIX_TYPES)
#	ifdef __INT8_TYPE__
		typedef __INT8_TYPE__ cs_int8;
		typedef __INT16_TYPE__ cs_int16;
		typedef __INT32_TYPE__ cs_int32;
		typedef __INT64_TYPE__ cs_int64;
		typedef __UINT8_TYPE__ cs_byte;
		typedef __UINT16_TYPE__ cs_uint16;
		typedef __UINT32_TYPE__ cs_uint32;
		typedef __UINT64_TYPE__ cs_uint64;
		typedef __UINTPTR_TYPE__ cs_uintptr, cs_size;
#	else
#		error No __INT8_TYPE__ found.
#	endif
#elif defined(CORE_USE_GENERIC_TYPES)
	typedef signed char cs_int8;
	typedef signed short cs_int16;
	typedef signed int cs_int32;
	typedef signed long long cs_int64;
	typedef unsigned char cs_byte;
	typedef unsigned short cs_uint16;
	typedef unsigned int cs_uint32;
	typedef unsigned long long cs_uint64;
	typedef unsigned long cs_uintptr, cs_size;
#else
#	error C types cannot be detected
#endif

typedef char cs_char;
typedef unsigned char cs_uchar;
typedef unsigned long cs_ulong;
typedef long cs_long;
typedef float cs_float;
typedef double cs_double;
typedef const cs_char *cs_str;
typedef cs_byte cs_bool;
typedef cs_byte BlockID;
typedef cs_byte ClientID;

#define true  1
#define false 0
#define NULL ((void *)0)
#define INL inline

#define PLUGIN_API_NUM 1
#define MAX_PLUGINS 64
#define	MAX_CMD_OUT 1024
#define MAX_CLIENTS 254
#define MAX_STR_LEN 65

#ifndef GIT_COMMIT_TAG
#	define GIT_COMMIT_TAG "unknown"
#endif

#ifndef CORE_BUILD_PLUGIN
#	define SOFTWARE_NAME "CServer"
#	define SOFTWARE_FULLNAME SOFTWARE_NAME "/" GIT_COMMIT_TAG
#	define TICKS_PER_SECOND (1000 / 64)
#	define MAINCFG "server.cfg"
#	define WORLD_MAGIC 0x54414457
#endif

#define ISHEX(ch) ((ch > '/' && ch < ':') || (ch > '@' && ch < 'G') || (ch > '`' && ch < 'g'))
#define ISNUM(ch) (ch >= '0' && ch <= '9')
#define ISSET(v, f) (((v) & (f)) == f)
#define BIT(b) (1U << b)
#endif
