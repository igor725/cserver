#ifndef CORE_H
#define CORE_H
#if defined(_WIN32)
#define WINDOWS
#define PATH_DELIM "\\"
#define DLIB_EXT "dll"
#define _CRT_SECURE_NO_WARNINGS
#define WIN32_LEAN_AND_MEAN

#define INL inline
#define NOINL __declspec(noinline)
#ifndef PLUGIN_BUILD
#define API __declspec(dllexport, noinline)
#define VAR __declspec(dllexport) extern
#else
#define API __declspec(dllimport)
#define VAR __declspec(dllimport) extern
#define EXP __declspec(dllexport) extern
#define EXPF __declspec(dllexport)
#endif // PLUGIN_BUILD

#if _MSC_VER
typedef signed __int8 cs_int8;
typedef signed __int16 cs_int16;
typedef signed __int32 cs_int32;
typedef signed __int64 cs_int64;
typedef unsigned __int8 cs_byte;
typedef unsigned __int16 cs_uint16;
typedef unsigned __int32 cs_uint32;
typedef unsigned __int64 cs_uint64;
#ifdef _WIN64
typedef unsigned __int64 cs_uintptr, cs_size;
#else
typedef unsigned int cs_uintptr, cs_size;
#endif // _WIN64
#else
#define CSTYPES_ERROR
#endif // _MSC_VER
#elif defined(__unix__)
#define UNIX
#define _GNU_SOURCE
#define PATH_DELIM "/"
#define DLIB_EXT "so"

#define INL inline
#define NOINL __attribute__((noinline))
#ifndef PLUGIN_BUILD
#define API __attribute__((__visibility__("default"), noinline))
#define VAR __attribute__((__visibility__("default"))) extern
#else
#define API
#define VAR extern
#define EXP __attribute__((__visibility__("default"))) extern
#define EXPF __attribute__((__visibility__("default"), noinline))
#endif // PLUGIN_BUILD

#define min(a, b) (((a)<(b))?(a):(b))
#define max(a, b) (((a)>(b))?(a):(b))
#define MAX_PATH  PATH_MAX

#ifdef __INT8_TYPE__
typedef __INT8_TYPE__ cs_int8;
typedef __INT16_TYPE__ cs_int16;
typedef __INT32_TYPE__ cs_int32;
typedef __INT64_TYPE__ cs_int64;
typedef __UINT8_TYPE__ cs_byte;
typedef __UINT16_TYPE__ cs_uint16;
typedef __UINT32_TYPE__ cs_uint32;
typedef __UINT64_TYPE__ cs_uint64;
typedef __UINTPTR_TYPE__ cs_uintptr, cs_size;
#else
#define CSTYPES_ERROR
#endif //__INT8_TYPE__
#else
#error Unknown OS
#endif // OS defines

#ifdef CSTYPES_ERROR
typedef signed char cs_int8;
typedef signed short cs_int16;
typedef signed int cs_int32;
typedef signed long long cs_int64;
typedef unsigned char cs_byte;
typedef unsigned short cs_uint16;
typedef unsigned int cs_uint32;
typedef unsigned long long cs_uint64;
typedef unsigned long cs_uintptr, cs_size;
#endif

#ifndef true
#define true  1
#define false 0
#endif

#ifndef NULL
#define NULL ((void *)0)
#endif

#ifndef MANUAL_BACKENDS
#if defined(WINDOWS)
#define HTTP_USE_WININET_BACKEND
#define HASH_USE_WINCRYPT_BACKEND
#elif defined(UNIX)
#define HTTP_USE_CURL_BACKEND
#define HASH_USE_CRYPTO_BACKEND
#endif
#endif

typedef char cs_char;
typedef unsigned char cs_uchar;
typedef unsigned long cs_ulong;
typedef float cs_float;
typedef const cs_char *cs_str;
typedef cs_byte cs_bool;
typedef cs_byte BlockID;
typedef cs_int8 ClientID;
typedef struct {
	cs_int16 r, g, b, a;
} Color4;

typedef struct {
	cs_int16 r, g, b;
} Color3;

typedef struct _CPEExt {
	cs_str name; // Название дополнения
	cs_int32 version; // Его версия
	cs_uint32 hash; // crc32 хеш названия дополнения
	struct _CPEExt *next; // Следующее дополнение
} CPEExt;

typedef struct _CustomParticle {
	cs_byte id;
	struct TextureRec {cs_byte U1, V1, U2, V2;} rec;
	Color3 tintCol;
	cs_byte frameCount, particleCount, collideFlags;
	cs_bool fullBright;
	cs_float size, sizeVariation, spread, speed,
	gravity, baseLifetime, lifetimeVariation;
} CustomParticle;

#define PLUGIN_API_NUM 1
#define MAX_PLUGINS 64
#define	MAX_CMD_OUT 1024
#define MAX_CLIENT_PPS 128
#define MAX_CLIENTS 127

#ifndef GIT_COMMIT_SHA
#define GIT_COMMIT_SHA "0000000"
#endif

#ifdef PLUGIN_BUILD
EXP cs_bool Plugin_Load(void);
EXP cs_bool Plugin_Unload(cs_bool force);
EXP cs_int32 Plugin_ApiVer, Plugin_Version;
#define Plugin_SetVersion(ver) cs_int32 Plugin_ApiVer = PLUGIN_API_NUM, Plugin_Version = ver;
#else
#define SOFTWARE_NAME "C-Server"
#define SOFTWARE_FULLNAME SOFTWARE_NAME "/" GIT_COMMIT_SHA
#define TICKS_PER_SECOND 60
#define MAINCFG "server.cfg"
#define WORLD_MAGIC 0x54414457
#endif

#define ISHEX(ch) ((ch > '/' && ch < ':') || (ch > '@' && ch < 'G') || (ch > '`' && ch < 'g'))
#define BIT(b) (1U << b)
#endif
