#ifndef CORE_H
#define CORE_H
#if defined(_WIN32)
#define WINDOWS
#define PATH_DELIM "\\"
#define DLIB_EXT "dll"
#define _CRT_SECURE_NO_WARNINGS
#define WIN32_LEAN_AND_MEAN
#ifndef PLUGIN_BUILD
#define API __declspec(dllexport, noinline)
#define VAR __declspec(dllexport)
#else
#define API __declspec(dllimport)
#define VAR __declspec(dllimport)
#define EXP __declspec(dllexport)
#endif // PLUGIN_BUILD

typedef __int8 cs_int8;
typedef __int16 cs_int16;
typedef __int32 cs_int32;
typedef __int64 cs_int64;
typedef unsigned __int8 cs_byte;
typedef unsigned __int16 cs_uint16;
typedef unsigned __int32 cs_uint32;
typedef unsigned __int64 cs_uint64;
#ifdef _WIN64
typedef unsigned __int64 cs_uintptr;
typedef unsigned __int64 cs_size;
#else
typedef unsigned int cs_uintptr;
typedef unsigned int cs_size;
#endif // _WIN64
#elif defined(__unix__)
#define POSIX
#define PATH_DELIM "/"
#define DLIB_EXT "so"

#ifndef PLUGIN_BUILD
#define API __attribute__((visibility("default"), noinline))
#define VAR __attribute__((visibility("default")))
#else
#define API
#define VAR
#define EXP __attribute__((__visibility__("default")))
#endif // PLUGIN_BUILD

#define min(a, b) (((a)<(b))?(a):(b))
#define max(a, b) (((a)>(b))?(a):(b))
#define INVALID_SOCKET -1
#define SD_SEND   SHUT_WR
#define MAX_PATH  PATH_MAX

typedef __INT8_TYPE__ cs_int8;
typedef __INT16_TYPE__ cs_int16;
typedef __INT32_TYPE__ cs_int32;
typedef __INT64_TYPE__ cs_int64;
typedef __UINT8_TYPE__ cs_byte;
typedef __UINT16_TYPE__ cs_uint16;
typedef __UINT32_TYPE__ cs_uint32;
typedef __UINT64_TYPE__ cs_uint64;
typedef __UINTPTR_TYPE__ cs_uintptr;
typedef __SIZE_TYPE__ cs_size;
#else
#error Unknown OS
#endif // OS defines

#define true  1
#define false 0

#ifndef NULL
#define NULL ((void *)0)
#endif

typedef char cs_char;
typedef unsigned char cs_uchar;
typedef unsigned long cs_ulong;
typedef float cs_float;
typedef const cs_char *cs_str;
typedef cs_byte cs_bool;
typedef cs_byte Order;
typedef cs_byte BlockID;
typedef cs_int8 ClientID;
typedef cs_int16 WorldID;
// TODO: Придумать, как пернести это чудо в protocol.h
typedef struct _CPEExt {
	cs_str name; // Название дополнения
	cs_int32 version; // Его версия
	cs_uint32 hash; // crc32 хеш названия дополнения
	struct _CPEExt *next; // Следующее дополнение
} CPEExt;

#ifdef PLUGIN_BUILD
EXP cs_bool Plugin_Load(void);
EXP cs_bool Plugin_Unload(void);
EXP extern cs_int32 Plugin_ApiVer, Plugin_Version;
#define Plugin_SetVersion(ver) cs_int32 Plugin_ApiVer = PLUGIN_API_NUM, Plugin_Version = ver;
#endif

#define SOFTWARE_NAME "C-Server"
#define SOFTWARE_FULLNAME SOFTWARE_NAME "/" GIT_COMMIT_SHA
#define CHATLINE "<%s>: %s"
#define MAINCFG "server.cfg"
#define WORLD_MAGIC 0x54414457
#define PLUGIN_API_NUM 1

#define MAX_PLUGINS 64
#define	MAX_CMD_OUT 1024
#define MAX_CLIENT_PPS 128
#define MAX_CFG_LEN 128
#define MAX_CLIENTS 127
#define MAX_PACKETS 256
#define MAX_WORLDS 256
#define MAX_EVENTS 128

#define ISHEX(ch) ((ch > '/' && ch < ':') || (ch > '@' && ch < 'G') || (ch > '`' && ch < 'g'))
#define MODE(b) Lang_Get(Lang_SwGrp, b > 0)
#define BIT(b) (1U << b)

typedef struct {
	cs_int16 r, g, b, a;
} Color4;

typedef struct {
	cs_int16 r, g, b;
} Color3;
#endif
