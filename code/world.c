#include "core.h"
#include "platform.h"
#include "str.h"
#include "client.h"
#include "world.h"
#include "event.h"

void Worlds_SaveAll(bool join) {
	for(int32_t i = 0; i < MAX_WORLDS; i++) {
		World world = Worlds_List[i];

		if(i < MAX_WORLDS && world) {
			if(World_Save(world) && join)
				Thread_Join(world->thread);
		}
	}
}

World World_Create(const char* name) {
	World tmp = Memory_Alloc(1, sizeof(struct world));
	WorldInfo wi = Memory_Alloc(1, sizeof(struct worldInfo));

	tmp->name = String_AllocCopy(name);
	tmp->saveDone = true;
	tmp->id = -1;
	tmp->info = wi;

	/*
	** Устанавливаем дефолтные значения
	** согласно документации по CPE.
	*/
	wi->props[PROP_SIDEBLOCK] = 7;
	wi->props[PROP_EDGEBLOCK] = 8;
	wi->props[PROP_FOGDIST] = 0;
	wi->props[PROP_SPDCLOUDS] = 256;
	wi->props[PROP_SPDWEATHER] = 256;
	wi->props[PROP_FADEWEATHER] = 128;
	wi->props[PROP_EXPFOG] = 0;
	wi->props[PROP_SIDEOFFSET] = -2;

	for(int i = 0; i < WORLD_COLORS_COUNT; i++) {
		wi->colors[i] = -1;
	}

	return tmp;
}

bool World_Add(World world) {
	if(world->id == -1) {
		for(int32_t i = 0; i < MAX_WORLDS; i++) {
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

World World_GetByName(const char* name) {
	for(int32_t i = 0; i < MAX_WORLDS; i++) {
		World world = Worlds_List[i];
		if(world && String_CaselessCompare(world->name, name))
			return world;
	}
	return NULL;
}

World World_GetByID(int32_t id) {
	return id < MAX_WORLDS ? Worlds_List[id] : NULL;
}

void World_SetDimensions(World world, uint16_t width, uint16_t height, uint16_t length) {
	WorldInfo wi = world->info;
	wi->width = width;
	wi->width = height;
	wi->length = length;
}

bool World_SetProperty(World world, uint8_t property, int32_t value) {
	if(property > WORLD_PROPS_COUNT) return false;

	world->modified = true;
	world->info->props[property] = value;
	world->info->modval |= MV_PROPS;
	world->info->modprop |= 2 ^ property;
	return true;
}

int32_t World_GetProperty(World world, uint8_t property) {
	if(property > WORLD_PROPS_COUNT) return 0;
	return world->info->props[property];
}

bool World_SetTexturePack(World world, const char* url) {
	if(String_CaselessCompare(world->info->texturepack, url)) {
		return true;
	}
	world->modified = true;
	world->info->modval |= MV_TEXPACK;
	if(!url || String_Length(url) > 64) {
		world->info->texturepack[0] = '\0';
		return true;
	}
	if(!String_Copy(world->info->texturepack, 65, url)) {
		world->info->texturepack[0] = '\0';
		return false;
	}
	return true;
}

const char* World_GetTexturePack(World world) {
	return world->info->texturepack;
}

bool World_SetWeather(World world, Weather type) {
	if(type > 2) return false;

	world->info->wt = type;
	world->modified = true;
	world->info->modval |= MV_WEATHER;
	Event_Call(EVT_ONWEATHER, world);
	return true;
}

bool World_SetColor(World world, uint8_t type, int16_t r, int16_t g, int16_t b) {
	if(type > WORLD_COLORS_COUNT) return false;
	int16_t* colors = &world->info->colors[type * 3];
	world->info->modval |= MV_COLORS;
	world->modified = true;
	colors[0] = r;
	colors[1] = g;
	colors[2] = b;
	Event_Call(EVT_ONCOLOR, world);
	return true;
}

int16_t* World_GetColor(World world, uint8_t type) {
	if(type > WORLD_COLORS_COUNT) return false;
	return &world->info->colors[type * 3];
}

void World_UpdateClients(World world) {
	Clients_UpdateWorldInfo(world);
	world->info->modval = 0;
}

Weather World_GetWeather(World world) {
	return world->info->wt;
}

void World_AllocBlockArray(World world) {
	if(world->data) Memory_Free(world->data);

	WorldInfo wi = world->info;
	uint16_t dx = wi->width,
	dy = wi->height,
	dz = wi->length;

	world->size = 4 + dx * dy * dz;
	BlockID* data = Memory_Alloc(world->size, sizeof(BlockID));
	*(uint32_t*)data = htonl(world->size - 4);
	world->data = data;
}

void World_Free(World world) {
	if(world->data) Memory_Free(world->data);
	WorldInfo wi = world->info;
	if(wi) Memory_Free(wi);
	if(world->id != -1) Worlds_List[world->id] = NULL;
	Memory_Free(world);
}

bool _WriteData(FILE* fp, uint8_t dataType, void* ptr, int32_t size) {
	if(!File_Write(&dataType, 1, 1, fp))
		return false;
	if(ptr && !File_Write(ptr, size, 1, fp))
		return false;
	return true;
}

bool World_WriteInfo(World world, FILE* fp) {
	int32_t magic = WORLD_MAGIC;
	if(!File_Write((char*)&magic, 4, 1, fp)) {
		Error_PrintSys(false);
		return false;
	}
	return _WriteData(fp, DT_DIM, &world->info->width, 6) &&
	_WriteData(fp, DT_SV, &world->info->spawnVec, sizeof(struct vector)) &&
	_WriteData(fp, DT_SA, &world->info->spawnAng, sizeof(struct angle)) &&
	_WriteData(fp, DT_WT, &world->info->wt, sizeof(Weather)) &&
	_WriteData(fp, DT_PROPS, world->info->props, 4 * WORLD_PROPS_COUNT) &&
	_WriteData(fp, DT_COLORS, world->info->colors, 2 * WORLD_COLORS_COUNT) &&
	_WriteData(fp, DT_END, NULL, 0);
}

bool World_ReadInfo(World world, FILE* fp) {
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
				if(File_Read(&world->info->width, 6, 1, fp) != 1)
					return false;
				break;
			case DT_SV:
				if(File_Read(&world->info->spawnVec, sizeof(struct vector), 1, fp) != 1)
					return false;
				break;
			case DT_SA:
				if(File_Read(&world->info->spawnAng, sizeof(struct angle), 1, fp) != 1)
					return false;
				break;
			case DT_WT:
				if(File_Read(&world->info->wt, sizeof(Weather), 1, fp) != 1)
					return false;
				break;
			case DT_PROPS:
				if(File_Read(world->info->props, 4 * WORLD_PROPS_COUNT, 1, fp) != 1)
					return false;
				break;
			case DT_COLORS:
				if(File_Read(world->info->colors, 2 * WORLD_COLORS_COUNT, 1, fp) != 1)
					return false;
				break;
			case DT_END:
				return true;
			default:
				Error_PrintF2(ET_SERVER, EC_FILECORR, false, world->name);
				return false;
		}
	}

	return false;
}

static TRET wSaveThread(TARG param) {
	World world = param;

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
	int32_t ret;

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

bool World_Save(World world) {
	if(!world->modified) {
		world->saveDone = true;
		return true;
	}
	if(world->saveDone) {
		world->saveDone = false;
		world->thread = Thread_Create(wSaveThread, world);
		return true;
	}
	return world->thread != NULL;
}

bool World_Load(World world) {
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
	int32_t ret;

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

uint32_t World_GetOffset(World world, SVec* pos) {
	WorldInfo wi = world->info;
	uint16_t dx = wi->width, dy = wi->height, dz = wi->length;

	if(pos->x > dx || pos->y > dy || pos->z > dz) return 0;
	return pos->z * dz + pos->y * (dx * dy) + pos->x + 4;
}

bool World_SetBlock(World world, SVec* pos, BlockID id) {
	uint32_t offset = World_GetOffset(world, pos);

	if(offset > 3 && offset < world->size) {
		world->data[offset] = id;
		world->modified = true;
	} else
		return false;

	return true;
}

BlockID World_GetBlock(World world, SVec* pos) {
	uint32_t offset = World_GetOffset(world, pos);

	if(offset > 3 && offset < world->size)
		return world->data[offset];
	else
		return 0;
}

void World_Tick(World world) {
	if(world->thread && world->saveDone) {
		Thread_Close(world->thread);
		world->thread = NULL;
		if(world->saveUnload) World_Free(world);
	}
}
