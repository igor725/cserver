#include "core.h"
#include "error.h"
#include <zlib.h>
#include "world.h"

WORLD* World_Create(char* name) {
	WORLD* tmp = calloc(1, sizeof(struct world));

	tmp->name = name;
	tmp->info = calloc(1, sizeof(struct worldInfo));
	tmp->info->dim = calloc(1, sizeof(struct worldDims));
	tmp->info->spawnVec = calloc(1, sizeof(struct vector));
	tmp->info->spawnAng = calloc(1, sizeof(struct angle));
	return tmp;
}

void World_SetDimensions(WORLD* world, ushort width, ushort height, ushort length) {
	WORLDDIMS* wd = world->info->dim;
	wd->width = width;
	wd->height = height;
	wd->length = length;
}

void World_AllocBlockArray(WORLD* world) {
	if(world->data)
		free(world->data);

	WORLDINFO* wi = world->info;
	ushort dx = wi->dim->width,
	dy = wi->dim->height,
	dz = wi->dim->length;

	world->size = 4 + dx * dy * dz;
	BlockID* data = calloc(world->size, sizeof(BlockID));
	world->data = data;
}

void World_Destroy(WORLD* world) {
	if(world->data)
		free(world->data);
	if(world->info)
		free(world->info);
	free(world);
}

void World_GenerateFlat(WORLD* world) {
	WORLDINFO* wi = world->info;
	ushort dx = wi->dim->width,
	dy = wi->dim->height,
	dz = wi->dim->length;

	BlockID* data = world->data + 4;
	int dirtEnd = dx * dz * (dy / 2 - 1);
	for(int i = 0; i < dirtEnd + dx * dz; i++) {
		if(i < dirtEnd)
			data[i] = 3;
		else
			data[i] = 2;
	}
	wi->spawnVec->x = (float)dx / 2;
	wi->spawnVec->y = (float)dy / 2;
	wi->spawnVec->z = (float)dz / 2;
}

void _WriteData(FILE* fp, uchar dataType, void* ptr, int size) {
	if(!fwrite(&dataType, 1, 1, fp))
		return;
	if(ptr && !fwrite(ptr, size, 1, fp))
		return;
}

bool World_WriteInfo(WORLD* world, FILE* fp) {
	int magic = WORLD_MAGIC;
	if(fwrite((char*)&magic, 4, 1, fp) != 1)
		return false;
	_WriteData(fp, DT_DIM, world->info->dim, sizeof(struct worldDims));
	_WriteData(fp, DT_SV, world->info->spawnVec, sizeof(struct vector));
	_WriteData(fp, DT_SA, world->info->spawnAng, sizeof(struct angle));
	_WriteData(fp, DT_END, NULL, 0);
	return true;
}

bool World_ReadInfo(WORLD* world, FILE* fp) {
	uchar id = 0;
	uint magic = 0;
	fread(&magic, 4, 1, fp);
	if(WORLD_MAGIC != magic) {
		Error_Set(ET_SERVER, EC_MAGIC);
		return false;
	}

	while(fread(&id, 1, 1, fp) == 1 && id != DT_END) {
		switch (id) {
			case DT_DIM:
				if(fread(world->info->dim, sizeof(struct worldDims), 1, fp) != 1)
					return false;
				break;
			case DT_SV:
				if(fread(world->info->spawnVec, sizeof(struct vector), 1, fp) != 1)
					return false;
				break;
			case DT_SA:
				if(fread(world->info->spawnAng, sizeof(struct angle), 1, fp) != 1)
					return false;
				break;
			default:
				return false;
		}
	}

	return true;
}

int World_Save(WORLD* world) {
	char name[256];
	sprintf(name, "%s.cws", world->name);
	FILE* fp;
	if(!(fp = fopen(name, "wb"))) {
		Error_Set(ET_WIN, GetLastError());
		return false;
	}

	if(!World_WriteInfo(world, fp)) {
		fclose(fp);
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
		uint nb = 1024 - stream.avail_out;

		if(fwrite(out, 1, 1024 - stream.avail_out, fp) != nb || ferror(fp)){
			deflateEnd(&stream);
			Error_Set(ET_WIN, 0);
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
	if(!(fp = fopen(name, "rb"))) {
		Error_Set(ET_WIN, GetLastError());
		return false;
	}

	if(!World_ReadInfo(world, fp)) {
		fclose(fp);
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
		stream.avail_in = (uint)fread(in, 1, 1024, fp);
		if(ferror(fp)) {
			Error_Set(ET_WIN, GetLastError());
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
				Error_Set(ET_ZLIB, ret);
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
	WORLDDIMS* wd = world->info->dim;
	ushort dx = wd->width, dy = wd->height, dz = wd->length;

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
