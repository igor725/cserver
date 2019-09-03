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
#elif defined(__unix__)
#define _GNU_SOURCE
#include <errno.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/time.h>
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
#endif

#define true  1
#define false 0

typedef unsigned short ushort;
typedef unsigned char  uchar;
typedef unsigned int   uint;
typedef int            bool;
typedef uchar          Order;
typedef uchar          BlockID;
typedef uchar          ClientID;

#include <stdlib.h>
#include <stdio.h>
#include "platform.h"
#include "error.h"
#include "str.h"
#include "log.h"

#define SOFTWARE_NAME "C-Server"
#define SOFTWARE_VERSION "0.1"
#define CHATLINE "<%s>: %s"
#define MAINCFG "server.cfg"

#define DEFAULT_NAME "Server name"
#define DEFAULT_MOTD "Server MOTD"

#define CPE_CHAT      0
#define CPE_STATUS1   1
#define CPE_STATUS2   2
#define CPE_STATUS3   3
#define CPE_BRIGHT1   11
#define CPE_BRIGHT2   12
#define CPE_BRIGHT3   13
#define CPE_ANNOUNCE  100

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
