#ifndef CORE_H
#define CORE_H
typedef unsigned short ushort;
typedef unsigned char  uchar;
typedef int            bool;
typedef unsigned int   uint;
#define true  1
#define false 0

#define DELIM " "
#define SOFTWARE_NAME "C-Server"
#define SOFTWARE_VERSION "0.1"

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
