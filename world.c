#include "core.h"
#include <stdio.h>
#include <zlib.h>
#include "world.h"

WORLD* World_Create(char* name, ushort dx, ushort dy, ushort dz) {
	WORLD* tmp = calloc(1, sizeof(struct world));

	tmp->name = name;
	tmp->size = dx * dy * dz + 4;
	tmp->dimensions[0] = dx;
	tmp->dimensions[1] = dy;
	tmp->dimensions[2] = dz;

	BlockID* data = calloc(tmp->size, sizeof(BlockID));
	*(uint*)data = htonl(tmp->size - 4);
	tmp->data = data;

	return tmp;
}

void World_Destroy(WORLD* world) {
	if(world->data)
		free(world->data);
	free(world);
}

void World_GenerateFlat(WORLD* world) {
	ushort dx = world->dimensions[0],
	dy = world->dimensions[1],
	dz = world->dimensions[2];

	int offset = dx * dz * (dy / 2 - 1);
	memset(world->data + 4, 3, offset);
	memset(world->data + 4 + offset, 2, dx * dz);
	world->spawnVec.x = dx / 2;
	world->spawnVec.y = dy / 2;
	world->spawnVec.z = dz / 2;
}

int World_WriteInfo(WORLD* world, FILE* fp) {
	return true;
}

int World_ReadInfo(WORLD* world, FILE* fp) {
	return true;
}

int World_Save(WORLD* world) {
	char name[256];
	sprintf(name, "%s.cws", world->name);
	FILE* fp;
	if((fp = fopen(name, "wb")) == NULL) {
		Error_SetCode(ET_IO, 0, "fopen");
		return false;
	}

	if(!World_WriteInfo(world, fp))
		return false;

	z_stream stream = {0};
	uchar out[1024];
	int ret;

	if((ret = deflateInit(&stream, 4)) != Z_OK) {
		Error_SetCode(ET_ZLIB, ret, "deflateInit");
		return false;
	}

	stream.avail_in = world->size;
	stream.next_in = (uchar*)world->data;

	do {
		stream.next_out = out;
		stream.avail_out = 1024;

		if((ret = deflate(&stream, Z_FINISH)) == Z_STREAM_ERROR) {
			Error_SetCode(ET_ZLIB, ret, "deflate");
			return false;
		}
		uint nb = 1024 - stream.avail_out;

		if(fwrite(out, 1, 1024 - stream.avail_out, fp) != nb || ferror(fp)){

			deflateEnd(&stream);
			return false;
		}
	} while(stream.avail_out == 0);

	fclose(fp);
	Error_SetSuccess();
	return true;
}

int World_Load(WORLD* world) {
	char name[256];
	sprintf(name, "%s.cws", world->name);
	FILE* fp;
	if((fp = fopen(name, "rb")) == NULL) {
		Error_SetCode(ET_IO, 0, "fopen");
		return false;
	}

	if(!World_ReadInfo(world, fp))
		return false;

	z_stream stream = {0};
	uchar in[1024];
	int ret;

	if((ret = inflateInit(&stream)) != Z_OK) {
		Error_SetCode(ET_ZLIB, ret, "inflateInit");
		return false;
	}

	stream.next_out = (uchar*)world->data;

	do {
		stream.avail_in = fread(in, 1, 1024, fp);
		if(ferror(fp)) {
			Error_SetCode(ET_IO, 0, "fread");
			inflateEnd(&stream);
			return false;
		}

		if(stream.avail_in == 0)
			break;

		stream.next_in = in;

		do {
			stream.avail_out = 1024;
			ret = inflate(&stream, Z_NO_FLUSH);
			if(ret == Z_NEED_DICT || ret == Z_DATA_ERROR || ret == Z_MEM_ERROR) {
				Error_SetCode(ET_ZLIB, ret, "inflate");
				inflateEnd(&stream);
				return false;
			}
		} while(stream.avail_out == 0);
	} while(ret != Z_STREAM_END);

	fclose(fp);
	Error_SetSuccess();
	return true;
}

uint World_GetOffset(WORLD* world, ushort x, ushort y, ushort z) {
	int dx = world->dimensions[0], dy = world->dimensions[1], dz = world->dimensions[2];
	if(x > dx || y > dy || z > dz)
		return 0;

	return z * dz + y * (dx * dy) + x + 4;
}

bool World_SetBlock(WORLD* world, ushort x, ushort y, ushort z, BlockID id) {
	int offset = World_GetOffset(world, x, y, z);

	if(offset > 3)
		world->data[offset] = (char)id;
	else
		return false;

	return true;
}

BlockID World_GetBlock(WORLD* world, ushort x, ushort y, ushort z) {
	int offset = World_GetOffset(world, x, y, z);

	if(offset > 3)
		return world->data[offset];
	else
		return 0;
}
