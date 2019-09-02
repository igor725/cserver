#include "core.h"
#include <zlib.h>
#include "world.h"

WORLD* World_Create(const char* name) {
	WORLD* tmp = (WORLD*)Memory_Alloc(1, sizeof(WORLD));

	tmp->name = String_AllocCopy(name);
	tmp->info = (WORLDINFO*)Memory_Alloc(1, sizeof(WORLDINFO));
	tmp->info->dim = (WORLDDIMS*)Memory_Alloc(1, sizeof(WORLDDIMS));
	tmp->info->spawnVec = (VECTOR*)Memory_Alloc(1, sizeof(VECTOR));
	tmp->info->spawnAng = (ANGLE*)Memory_Alloc(1, sizeof(ANGLE));
	return tmp;
}

WORLD* World_FindByName(const char* name) {
	for(int i = 0; i < MAX_WORLDS; i++) {
		WORLD* world = worlds[i];
		if(!world) continue;
		if(String_CaselessCompare(world->name, name))
			return world;
	}
	return NULL;
}

void World_SetDimensions(WORLD* world, ushort width, ushort height, ushort length) {
	WORLDDIMS* wd = world->info->dim;
	wd->width = width;
	wd->height = height;
	wd->length = length;
}

void World_AllocBlockArray(WORLD* world) {
	if(world->data)
		Memory_Free(world->data);

	WORLDINFO* wi = world->info;
	ushort dx = wi->dim->width,
	dy = wi->dim->height,
	dz = wi->dim->length;

	world->size = 4 + dx * dy * dz;
	BlockID* data = (BlockID*)Memory_Alloc(world->size, sizeof(BlockID));
	*(uint*)data = htonl(world->size - 4);
	world->data = data;
}

void World_Destroy(WORLD* world) {
	if(world->data)
		Memory_Free(world->data);
	if(world->info)
		Memory_Free(world->info);
	Memory_Free(world);
}

bool _WriteData(FILE* fp, uchar dataType, void* ptr, int size) {
	if(!File_Write(&dataType, 1, 1, fp))
		return false;
	if(ptr && !File_Write(ptr, size, 1, fp))
		return false;
	return true;
}

bool World_WriteInfo(WORLD* world, FILE* fp) {
	int magic = WORLD_MAGIC;
	if(!File_Write((char*)&magic, 4, 1, fp))
		return false;
	return _WriteData(fp, DT_DIM, world->info->dim, sizeof(WORLDDIMS)) &&
	_WriteData(fp, DT_SV, world->info->spawnVec, sizeof(VECTOR)) &&
	_WriteData(fp, DT_SA, world->info->spawnAng, sizeof(ANGLE)) &&
	_WriteData(fp, DT_END, NULL, 0);
}

bool World_ReadInfo(WORLD* world, FILE* fp) {
	uchar id = 0;
	uint magic = 0;
	if(!File_Read(&magic, 4, 1, fp))
		return false;

	if(WORLD_MAGIC != magic) {
		Error_Set(ET_SERVER, EC_MAGIC);
		return false;
	}

	while(File_Read(&id, 1, 1, fp) == 1) {
		switch (id) {
			case DT_DIM:
				if(File_Read(world->info->dim, sizeof(WORLDDIMS), 1, fp) != 1)
					return false;
				break;
			case DT_SV:
				if(File_Read(world->info->spawnVec, sizeof(VECTOR), 1, fp) != 1)
					return false;
				break;
			case DT_SA:
				if(File_Read(world->info->spawnAng, sizeof(ANGLE), 1, fp) != 1)
					return false;
				break;
			case DT_END:
				return true;
			default:
				Error_Set(ET_SERVER, EC_WIUNKID);
				return false;
		}
	}

	return false;
}

int World_Save(WORLD* world) {
	FILE* fp;
	char tmpname[256];
	String_FormatBuf(tmpname, 256, "%s.tmp", world->name);

	if(!(fp = File_Open(tmpname, "wb")))
		return false;

	if(!World_WriteInfo(world, fp)) {
		File_Close(fp);
		return false;
	}

	z_stream stream = {0};
	uchar out[1024];
	int ret;

	if((ret = deflateInit(&stream, 4)) != Z_OK) {
		Error_Set(ET_ZLIB, ret);
		return false;
	}

	stream.avail_in = world->size;
	stream.next_in = (uchar*)world->data;

	do {
		stream.next_out = out;
		stream.avail_out = 1024;

		if((ret = deflate(&stream, Z_FINISH)) == Z_STREAM_ERROR) {
			Error_Set(ET_ZLIB, ret);
			return false;
		}

		if(!File_Write(out, 1, 1024 - stream.avail_out, fp)){
			deflateEnd(&stream);
			return false;
		}
	} while(stream.avail_out == 0);

	File_Close(fp);
	return File_Rename(tmpname, world->name);
}

int World_Load(WORLD* world) {
	FILE* fp;
	if(!(fp = File_Open(world->name, "rb")))
		return false;

	if(!World_ReadInfo(world, fp)) {
		File_Close(fp);
		return false;
	} else {
		World_AllocBlockArray(world);
	}

	z_stream stream = {0};
	uchar in[1024];
	int ret;

	if((ret = inflateInit(&stream)) != Z_OK) {
		Error_Set(ET_ZLIB, ret);
		return false;
	}

	stream.next_out = (uchar*)world->data;

	do {
		stream.avail_in = (uint)File_Read(in, 1, 1024, fp);
		if(File_Error(fp)) {
			inflateEnd(&stream);
			return false;
		}

		if(stream.avail_in == 0)
			break;

		stream.next_in = in;

		do {
			stream.avail_out = 1024;
			if((ret = inflate(&stream, Z_NO_FLUSH)) == Z_NEED_DICT || ret == Z_DATA_ERROR || ret == Z_MEM_ERROR) {
				Error_Set(ET_ZLIB, ret);
				inflateEnd(&stream);
				return false;
			}
		} while(stream.avail_out == 0);
	} while(ret != Z_STREAM_END);

	File_Close(fp);
	return true;
}

uint World_GetOffset(WORLD* world, ushort x, ushort y, ushort z) {
	WORLDDIMS* wd = world->info->dim;
	ushort dx = wd->width, dy = wd->height, dz = wd->length;

	if(x > dx || y > dy || z > dz)
		return 0;

	return z * dz + y * (dx * dy) + x + 4;
}

bool World_SetBlock(WORLD* world, ushort x, ushort y, ushort z, BlockID id) {
	uint offset = World_GetOffset(world, x, y, z);

	if(offset > 3 && offset < world->size)
		world->data[offset] = id;
	else
		return false;

	return true;
}

BlockID World_GetBlock(WORLD* world, ushort x, ushort y, ushort z) {
	uint offset = World_GetOffset(world, x, y, z);

	if(offset > 3 && offset < world->size)
		return world->data[offset];
	else
		return 0;
}
