#ifndef CORE_H
#define CORE_H
#if defined(_WIN32)
#  pragma warning(disable:4100)
#  define WINDOWS
#  define WIN32_LEAN_AND_MEAN
#  define _CRT_SECURE_NO_WARNINGS
#  include <winsock2.h>
#  include <ws2tcpip.h>

#  define ZLIB_WINAPI
#  ifndef CPLUGIN
#    define API __declspec(dllexport, noinline)
#    define VAR __declspec(dllexport)
#  else
#    define API __declspec(dllimport)
#    define VAR __declspec(dllimport)
#    define EXP __declspec(dllexport)
#  endif

typedef signed __int64 int64;
typedef unsigned __int64 uint64;
#elif defined(__unix__)
#  define POSIX
#  define _GNU_SOURCE
#  include <errno.h>
#  include <sys/socket.h>
#  include <sys/stat.h>
#  include <sys/time.h>
#  include <sys/types.h>
#  include <arpa/inet.h>
#  include <unistd.h>
#  include <dlfcn.h>
#  include <pthread.h>
#  include <dirent.h>

#  ifndef CPLUGIN
#    define API __attribute__((visibility("default"), noinline))
#    define VAR __attribute__((visibility("default")))
#  else
#    define API
#    define VAR
#    define EXP __attribute__((__visibility__("default")))
#  endif

#  define min(a, b) (((a)<(b))?(a):(b))
#  define max(a, b) (((a)>(b))?(a):(b))
#  define Sleep(ms) (usleep(ms * 1000))
#  define GetLastError() (errno)
#  define INVALID_SOCKET -1
#  define SD_SEND   SHUT_WR
#  define MAX_PATH  PATH_MAX

typedef int SOCKET;
typedef unsigned __INT64_TYPE__ uint64;
typedef signed __INT64_TYPE__ int64;
#else
#  error Unknown OS
#endif

#ifndef true
#  define true  1
#  define false 0
#endif

#include <stdint.h>
#include <stdio.h>
#include <zlib.h>

typedef uint32_t bool;
typedef uint8_t  Order;
typedef uint8_t  BlockID;
typedef uint8_t  ClientID;
typedef uint8_t  Weather;
typedef uint8_t  MessageType;

#include "platform.h"
#include "error.h"
#include "block.h"
#include "str.h"
#include "log.h"

#define SOFTWARE_NAME "C-Server"
#define SOFTWARE_VERSION "0.1"
#define CHATLINE "<%s>: %s"
#define MAINCFG "server.cfg"

#define MAX_CLIENTS 128
#define MAX_WORLDS  128
#define MAX_PACKETS 256
#define MAX_EVENTS  64
#define EVENT_TYPES 12

#define CFG_STRLEN 128

#define WORLD_MAGIC 0x54414457

#define DEFAULT_NAME "Server name"
#define DEFAULT_MOTD "Server MOTD"

enum messageTypes {
	CPE_CHAT,
	CPE_STATUS1,
	CPE_STATUS2,
	CPE_STATUS3,
	CPE_BRIGHT1 = 11,
	CPE_BRIGHT2,
	CPE_BRIGHT3,
	CPE_ANNOUNCE = 100
};

#define ISHEX(ch) ((ch > '/' && ch < ':') || (ch > '@' && ch < 'G') || (ch > '`' && ch < 'g'))

typedef struct cpeExt {
	const char* name;
	int version;
	struct cpeExt*  next;
} *EXT;

typedef struct vector {
	float x;
	float y;
	float z;
} VECTOR;

typedef struct angle {
	float yaw;
	float pitch;
} ANGLE;
#endif
