#ifndef CORE_H
#define CORE_H
#if defined(_WIN32)
#pragma warning(disable:4706)
#pragma warning(disable:4100)
#define WINDOWS
#define WIN32_LEAN_AND_MEAN
#define _CRT_SECURE_NO_WARNINGS
#include <winsock2.h>
#include <ws2tcpip.h>
#define ZLIB_WINAPI
#ifndef CPLUGIN
#define API __declspec(dllexport, noinline)
#define VAR __declspec(dllexport)
#else
#define API __declspec(dllimport)
#define VAR __declspec(dllimport)
#define EXP __declspec(dllexport)
#endif
#elif defined(__unix__)
#define _GNU_SOURCE
#include <errno.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <dlfcn.h>
#include <pthread.h>
#include <dirent.h>

typedef int SOCKET;
#define min(a, b) (((a)<(b))?(a):(b))
#define max(a, b) (((a)>(b))?(a):(b))
#define Sleep(ms) (usleep(ms * 1000))
#define GetLastError() (errno)
#define INVALID_SOCKET -1
#define POSIX
#define SD_SEND   SHUT_WR
#define MAX_PATH  PATH_MAX
#ifndef CPLUGIN
#define API __attribute__((visibility("default"), noinline))
#define VAR __attribute__((visibility("default")))
#else
#define API
#define VAR
#define EXP __attribute__((__visibility__("default")))
#endif
#endif

#ifndef true
#define true  1
#define false 0
#endif

typedef unsigned short ushort;
typedef ushort*        ushortp;
typedef unsigned char  uchar;
typedef unsigned int   uint;
typedef int            bool;
typedef uchar          Order;
typedef uchar          BlockID;
typedef uchar          ClientID;
typedef unsigned char  Weather;
typedef unsigned char  MessageType;

#include <stdlib.h>
#include <stdio.h>
#include <zlib.h>
#include "platform.h"
#include "error.h"
#include "block.h"
#include "str.h"
#include "log.h"

#define SOFTWARE_NAME "C-Server"
#define SOFTWARE_VERSION "0.1"
#define CHATLINE "<%s>: %s"
#define MAINCFG "server.cfg"

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

typedef struct ext {
	const char* name;
	int   version;
	struct ext*  next;
} EXT;

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
