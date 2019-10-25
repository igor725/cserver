#include "core.h"
#include "platform.h"
#include "str.h"
#include "client.h"
#include "world.h"
#include "event.h"

void Worlds_SaveAll(bool join) {
	for(int i = 0; i < MAX_WORLDS; i++) {
		WORLD world = Worlds_List[i];

		if(i < MAX_WORLDS && world) {
			if(World_Save(world) && join)
				Thread_Join(world->thread);
		}
	}
}

WORLD World_Create(const char* name) {
	WORLD tmp = Memory_Alloc(1, sizeof(struct world));
	WORLDINFO wi = Memory_Alloc(1, sizeof(struct worldInfo));
	tmp->name = String_AllocCopy(name);
	tmp->saveDone = true;
	tmp->id = -1;
	tmp->info = wi;


	wi->dim = Memory_Alloc(1, sizeof(struct worldDims));
	wi->spawnVec = Memory_Alloc(1, sizeof(struct vector));
	wi->spawnAng = Memory_Alloc(1, sizeof(struct angle));

	wi->props[PROP_SIDEBLOCK] = 7;
	wi->props[PROP_EDGEBLOCK] = 8;
	wi->props[PROP_FOGDIST] = 0;
	wi->props[PROP_SPDCLOUDS] = 256;
	wi->props[PROP_SPDWEATHER] = 256;
	wi->props[PROP_FADEWEATHER] = 128;
	wi->props[PROP_EXPFOG] = 0;
	wi->props[PROP_SIDEOFFSET] = -2;

	return tmp;
}

bool World_Add(WORLD world) {
	if(world->id == -1) {
		for(int i = 0; i < MAX_WORLDS; i++) {
			if(!Worlds_List[i]) {
				world->id = i;
				Worlds_List[i] = world;
				return true;
			}
		}
		return false;
	}

	if(world->id > MAX_WORLDS) return false;
	Worlds_List[world->id] = world;
	return true;
}

WORLD World_GetByName(const char* name) {
	for(int i = 0; i < MAX_WORLDS; i++) {
		WORLD world = Worlds_List[i];
		if(world && String_CaselessCompare(world->name, name))
			return world;
	}
	return NULL;
}

WORLD World_GetByID(int id) {
	return id < MAX_WORLDS ? Worlds_List[id] : NULL;
}

void World_SetDimensions(WORLD world, uint16_t width, uint16_t height, uint16_t length) {
	WORLDDIMS wd = world->info->dim;
	wd->width = width;
	wd->height = height;
	wd->length = length;
}

bool World_SetProperty(WORLD world, uint8_t property, int value) {
	if(property > WORLD_PROPS_COUNT) return false;

	world->info->props[property] = value;
	for(ClientID i = 0; i < MAX_CLIENTS; i++) {
		CLIENT client = Clients_List[i];
		if(client && Client_IsInWorld(client, world))
			Client_SetProperty(client, property, value);
	}

	return true;
}

int World_GetProperty(WORLD world, uint8_t property) {
	if(property > WORLD_PROPS_COUNT) return 0;
	return world->info->props[property];
}

bool World_SetTexturePack(WORLD world, const char* url) {
	if(!url || String_Length(url) > 64) {
		world->info->texturepack[0] = '\0';
		return true;
	}
	if(String_Copy(world->info->texturepack, 65, url)) {
		world->info->texturepack[0] = '\0';
		return true;
	}
	return false;
}

const char* World_GetTexturePack(WORLD world) {
	return world->info->texturepack;
}

bool World_SetWeather(WORLD world, Weather type) {
	if(type > 2) return false;

	world->info->wt = type;
	Event_Call(EVT_ONWEATHER, world);
	for(ClientID i = 0; i < MAX_CLIENTS; i++) {
		CLIENT client = Clients_List[i];
		if(client && Client_IsInWorld(client, world))
			Client_SetWeather(client, type);
	}
	return true;
}

Weather World_GetWeather(WORLD world) {
	return world->info->wt;
}

void World_AllocBlockArray(WORLD world) {
	if(world->data) Memory_Free(world->data);

	WORLDINFO wi = world->info;
	uint16_t dx = wi->dim->width,
	dy = wi->dim->height,
	dz = wi->dim->length;

	world->size = 4 + dx * dy * dz;
	BlockID* data = Memory_Alloc(world->size, sizeof(BlockID));
	*(uint32_t*)data = htonl(world->size - 4);
	world->data = data;
}

void World_Free(WORLD world) {
	if(world->data) Memory_Free(world->data);
	WORLDINFO wi = world->info;
	if(wi) {
		Memory_Free(wi->spawnVec);
		Memory_Free(wi->spawnAng);
		Memory_Free(wi->dim);
		Memory_Free(wi);
	}
	if(world->id != -1) Worlds_List[world->id] = NULL;
	Memory_Free(world);
}

bool _WriteData(FILE* fp, uint8_t dataType, void* ptr, int size) {
	if(!File_Write(&dataType, 1, 1, fp))
		return false;
	if(ptr && !File_Write(ptr, size, 1, fp))
		return false;
	return true;
}

bool World_WriteInfo(WORLD world, FILE* fp) {
	int magic = WORLD_MAGIC;
	if(!File_Write((char*)&magic, 4, 1, fp)) {
		Error_PrintSys(false);
		return false;
	}
	return _WriteData(fp, DT_DIM, world->info->dim, sizeof(struct worldDims)) &&
	_WriteData(fp, DT_SV, world->info->spawnVec, sizeof(struct vector)) &&
	_WriteData(fp, DT_SA, world->info->spawnAng, sizeof(struct angle)) &&
	_WriteData(fp, DT_WT, &world->info->wt, sizeof(Weather)) &&
	_WriteData(fp, DT_PROPS, world->info->props, sizeof(int) * WORLD_PROPS_COUNT) &&
	_WriteData(fp, DT_END, NULL, 0);
}

bool World_ReadInfo(WORLD world, FILE* fp) {
	uint8_t id = 0;
	uint32_t magic = 0;
	if(!File_Read(&magic, 4, 1, fp))
		return false;

	if(WORLD_MAGIC != magic) {
		Error_Print2(ET_SERVER, EC_MAGIC, true);
		return false;
	}

	while(File_Read(&id, 1, 1, fp) == 1) {
		switch (id) {
			case DT_DIM:
				if(File_Read(world->info->dim, sizeof(struct worldDims), 1, fp) != 1)
					return false;
				break;
			case DT_SV:
				if(File_Read(world->info->spawnVec, sizeof(struct vector), 1, fp) != 1)
					return false;
				break;
			case DT_SA:
				if(File_Read(world->info->spawnAng, sizeof(struct angle), 1, fp) != 1)
					return false;
				break;
			case DT_WT:
				if(File_Read(&world->info->wt, sizeof(Weather), 1, fp) != 1)
					return false;
				break;
			case DT_PROPS:
				if(File_Read(world->info->props, sizeof(int) * WORLD_PROPS_COUNT, 1, fp) != 1)
					return false;
				break;
			case DT_END:
				return true;
			default:
				Error_PrintF2(ET_SERVER, EC_WIUNKID, false, world->name);
				return false;
		}
	}

	return false;
}

static TRET wSaveThread(TARG param) {
	WORLD world = (WORLD)param;

	char path[256];
	char tmpname[256];
	String_FormatBuf(path, 256, "worlds/%s", world->name);
	String_FormatBuf(tmpname, 256, "worlds/%s.tmp", world->name);

	FILE* fp = File_Open(tmpname, "wb");
	if(!fp) return false;

	if(!World_WriteInfo(world, fp)) {
		File_Close(fp);
		return false;
	}

	z_stream stream = {0};
	uint8_t out[1024];
	int ret;

	if((ret = deflateInit(&stream, Z_BEST_COMPRESSION)) != Z_OK) {
		Error_Print2(ET_ZLIB, ret, false);
		return false;
	}

	stream.avail_in = world->size;
	stream.next_in = (uint8_t*)world->data;

	do {
		stream.next_out = out;
		stream.avail_out = 1024;

		if((ret = deflate(&stream, Z_FINISH)) == Z_STREAM_ERROR) {
			Error_Print2(ET_ZLIB, ret, false);
			return false;
		}

		if(!File_Write(out, 1, 1024 - stream.avail_out, fp)){
			deflateEnd(&stream);
			return false;
		}
	} while(stream.avail_out == 0);

	File_Close(fp);
	File_Rename(tmpname, path);
	world->saveDone = true;
	return 0;
}

bool World_Save(WORLD world) {
	if(world->saveDone) {
		world->saveDone = false;
		world->thread = Thread_Create(wSaveThread, world);
		return true;
	}
	return world->thread != NULL;
}

bool World_Load(WORLD world) {
	char path[256];
	String_FormatBuf(path, 256, "worlds/%s", world->name);

	FILE* fp = File_Open(path, "rb");
	if(!fp) return false;

	if(!World_ReadInfo(world, fp)) {
		File_Close(fp);
		return false;
	}

	World_AllocBlockArray(world);

	z_stream stream = {0};
	uint8_t in[1024];
	int ret;

	if((ret = inflateInit(&stream)) != Z_OK) {
		Error_Print2(ET_ZLIB, ret, false);
		return false;
	}

	stream.next_out = (uint8_t*)world->data;

	do {
		stream.avail_in = (uint32_t)File_Read(in, 1, 1024, fp);
		if(File_Error(fp)) {
			inflateEnd(&stream);
			return false;
		}

		if(stream.avail_in == 0) break;
		stream.next_in = in;

		do {
			stream.avail_out = 1024;
			if((ret = inflate(&stream, Z_NO_FLUSH)) == Z_NEED_DICT || ret == Z_DATA_ERROR || ret == Z_MEM_ERROR) {
				Error_Print2(ET_ZLIB, ret, false);
				inflateEnd(&stream);
				return false;
			}
		} while(stream.avail_out == 0);
	} while(ret != Z_STREAM_END);

	File_Close(fp);
	return true;
}

uint32_t World_GetOffset(WORLD world, uint16_t x, uint16_t y, uint16_t z) {
	WORLDDIMS wd = world->info->dim;
	uint16_t dx = wd->width, dy = wd->height, dz = wd->length;

	if(x > dx || y > dy || z > dz) return 0;
	return z * dz + y * (dx * dy) + x + 4;
}

bool World_SetBlock(WORLD world, uint16_t x, uint16_t y, uint16_t z, BlockID id) {
	uint32_t offset = World_GetOffset(world, x, y, z);

	if(offset > 3 && offset < world->size)
		world->data[offset] = id;
	else
		return false;

	return true;
}

BlockID World_GetBlock(WORLD world, uint16_t x, uint16_t y, uint16_t z) {
	uint32_t offset = World_GetOffset(world, x, y, z);

	if(offset > 3 && offset < world->size)
		return world->data[offset];
	else
		return 0;
}

void World_Tick(WORLD world) {
	if(world->thread && world->saveDone) {
		Thread_Close(world->thread);
		world->thread = NULL;
		if(world->saveUnload) World_Free(world);
	}
}
