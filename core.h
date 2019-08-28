#ifndef CORE_H
#define CORE_H

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define _CRT_SECURE_NO_WARNINGS
#include <winsock2.h>
#include <ws2tcpip.h>
#define ZLIB_WINAPI
#else
#include <errno.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

#define INVALID_SOCKET -1
#define SD_SEND SHUT_WR
typedef int SOCKET;
#define GetLastError() (errno)
#define min(a, b) (((a)<(b))?(a):(b))
#define max(a, b) (((a)>(b))?(a):(b))
#define Sleep(ms) (usleep(ms * 1000))
#endif

#define true  1
#define false 0

typedef unsigned short ushort;
typedef unsigned char  uchar;
typedef int            bool;
typedef unsigned int   uint;
typedef unsigned char  BlockID;
typedef unsigned char  ClientID;
typedef unsigned char  Order;

#include <stdio.h>
#include <stdlib.h>
#include "platform.h"

#define DELIM " "
#define SOFTWARE_NAME "C-Server"
#define SOFTWARE_VERSION "0.1"
#define CHATLINE "<%s>: %s"

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
	char* name;
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
