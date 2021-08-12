#include "core.h"
#include "platform.h"
#include "error.h"
#include "str.h"
#include "server.h"
#include "world.h"
#include "event.h"
#include <zlib.h>

World *Worlds_List[MAX_WORLDS] = {0};

void Worlds_SaveAll(cs_bool join, cs_bool unload) {
	for(cs_int32 i = 0; i < MAX_WORLDS; i++) {
		World *world = Worlds_List[i];

		if(i < MAX_WORLDS && world) {
			if(World_Save(world, unload) && join) {
				Waitable_Wait(world->wait);
				if(!Server_Active) {
					World_Free(world);
				}
			}
		}
	}
}

World *World_Create(cs_str name) {
	World *tmp = Memory_Alloc(1, sizeof(World));

	tmp->name = String_AllocCopy(name);
	tmp->wait = Waitable_Create();
	tmp->process = WP_NOPROC;
	tmp->id = -1;

	/*
	** Устанавливаем дефолтные значения
	** согласно документации по CPE.
	*/
	cs_int32 *props = tmp->info.props;
	props[PROP_SIDEBLOCK] = 7;
	props[PROP_EDGEBLOCK] = 8;
	props[PROP_FOGDIST] = 0;
	props[PROP_SPDCLOUDS] = 256;
	props[PROP_SPDWEATHER] = 256;
	props[PROP_FADEWEATHER] = 128;
	props[PROP_EXPFOG] = 0;
	props[PROP_SIDEOFFSET] = -2;

	Color3* colors = tmp->info.colors;
	for(int i = 0; i < WORLD_COLORS_COUNT; i++) {
		colors[i].r = -1;
		colors[i].g = -1;
		colors[i].b = -1;
	}

	return tmp;
}

cs_bool World_Add(World *world) {
	if(world->id == -1) {
		for(WorldID i = 0; i < MAX_WORLDS; i++) {
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

cs_bool World_IsReadyToPlay(World *world) {
	return world->wdata.ptr != NULL &&
	world->process == WP_NOPROC && world->loaded;
}

World *World_GetByName(cs_str name) {
	for(WorldID i = 0; i < MAX_WORLDS; i++) {
		World *world = Worlds_List[i];
		if(world && String_CaselessCompare(world->name, name))
			return world;
	}
	return NULL;
}

World *World_GetByID(WorldID id) {
	return id < MAX_WORLDS ? Worlds_List[id] : NULL;
}

void World_SetDimensions(World *world, const SVec *dims) {
	world->info.dimensions = *dims;
	world->wdata.size = dims->x * dims->y * dims->z;
}

cs_bool World_SetProperty(World *world, cs_byte property, cs_int32 value) {
	if(property > WORLD_PROPS_COUNT) return false;

	world->modified = true;
	world->info.props[property] = value;
	world->info.modval |= MV_PROPS;
	world->info.modprop |= 2 ^ property;
	return true;
}

cs_int32 World_GetProperty(World *world, cs_byte property) {
	if(property > WORLD_PROPS_COUNT) return 0;
	return world->info.props[property];
}

cs_bool World_SetTexturePack(World *world, cs_str url) {
	if(String_CaselessCompare(world->info.texturepack, url))
		return true;
	world->modified = true;
	world->info.modval |= MV_TEXPACK;
	if(!url || String_Length(url) > 64) {
		world->info.texturepack[0] = '\0';
		return true;
	}
	if(!String_Copy(world->info.texturepack, 65, url)) {
		world->info.texturepack[0] = '\0';
		return false;
	}
	return true;
}

cs_str World_GetTexturePack(World *world) {
	return world->info.texturepack;
}

cs_bool World_SetWeather(World *world, cs_int8 type) {
	if(type > 2) return false;
	world->info.weatherType = type;
	world->modified = true;
	world->info.modval |= MV_WEATHER;
	Event_Call(EVT_ONWEATHER, world);
	return true;
}

cs_bool World_SetEnvColor(World *world, cs_byte type, Color3* color) {
	if(type > WORLD_COLORS_COUNT) return false;
	world->info.modval |= MV_COLORS;
	world->modified = true;
	world->info.colors[type * 3] = *color;
	Event_Call(EVT_ONCOLOR, world);
	return true;
}

Color3* World_GetEnvColor(World *world, cs_byte type) {
	if(type > WORLD_COLORS_COUNT) return false;
	return &world->info.colors[type];
}

cs_int8 World_GetWeather(World *world) {
	return world->info.weatherType;
}

void World_AllocBlockArray(World *world) {
	void *data = Memory_Alloc(world->wdata.size + 4, 1);
	*(cs_uint32 *)data = htonl(world->wdata.size);
	world->wdata.ptr = data;
	world->wdata.blocks = (BlockID *)data + 4;
	world->loaded = true;
}

BlockID *World_GetBlockArray(World *world, cs_uint32 *size) {
	if(size) *size = World_GetBlockArraySize(world);
	return world->wdata.blocks;
}

void *World_GetData(World *world, cs_uint32 *size) {
	if(size) *size = world->wdata.size + 4;
	return world->wdata.ptr;
}

cs_uint32 World_GetBlockArraySize(World *world) {
	return world->wdata.size;
}

void World_Free(World *world) {
	Waitable_Free(world->wait);
	if(world->id != -1) Worlds_List[world->id] = NULL;
	Memory_Free(world);
}

static cs_bool WriteWData(cs_file fp, cs_byte dataType, void *ptr, cs_int32 size) {
	return File_Write(&dataType, 1, 1, fp) && (size > 0 ? File_Write(ptr, size, 1, fp) : true);
}

static cs_bool WriteInfo(World *world, cs_file fp) {
	cs_int32 magic = WORLD_MAGIC;
	if(!File_Write((cs_char *)&magic, 4, 1, fp)) {
		Error_PrintSys(false);
		return false;
	}
	WorldInfo *wi = &world->info;
	return WriteWData(fp, DT_DIM, &wi->dimensions, sizeof(SVec)) &&
	WriteWData(fp, DT_SV, &wi->spawnVec, sizeof(Vec)) &&
	WriteWData(fp, DT_SA, &wi->spawnAng, sizeof(Ang)) &&
	WriteWData(fp, DT_WT, &wi->weatherType, 1) &&
	WriteWData(fp, DT_PROPS, wi->props, 4 * WORLD_PROPS_COUNT) &&
	WriteWData(fp, DT_COLORS, wi->colors, sizeof(Color3) * WORLD_COLORS_COUNT) &&
	WriteWData(fp, DT_END, NULL, 0);
}

static cs_bool ReadInfo(World *world, cs_file fp) {
	cs_byte id = 0;
	cs_uint32 magic = 0;
	if(!File_Read(&magic, 4, 1, fp))
		return false;

	if(WORLD_MAGIC != magic) {
		ERROR_PRINT(ET_SERVER, EC_MAGIC, true);
		return false;
	}

	SVec dims;
	WorldInfo *wi = &world->info;
	while(File_Read(&id, 1, 1, fp) == 1) {
		switch (id) {
			case DT_DIM:
				if(File_Read(&dims, sizeof(SVec), 1, fp) != 1)
					return false;
				World_SetDimensions(world, &dims);
				break;
			case DT_SV:
				if(File_Read(&wi->spawnVec, sizeof(Vec), 1, fp) != 1)
					return false;
				break;
			case DT_SA:
				if(File_Read(&wi->spawnAng, sizeof(Ang), 1, fp) != 1)
					return false;
				break;
			case DT_WT:
				if(File_Read(&wi->weatherType, 1, 1, fp) != 1)
					return false;
				break;
			case DT_PROPS:
				if(File_Read(wi->props, 4 * WORLD_PROPS_COUNT, 1, fp) != 1)
					return false;
				break;
			case DT_COLORS:
				if(File_Read(wi->colors, sizeof(Color3) * WORLD_COLORS_COUNT, 1, fp) != 1)
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

#define CHUNK_SIZE 16384

THREAD_FUNC(WorldSaveThread) {
	World *world = (World *)param;
	cs_bool succ = false;
	cs_char path[256];
	cs_char tmpname[256];
	String_FormatBuf(path, 256, "worlds" PATH_DELIM "%s", world->name);
	String_FormatBuf(tmpname, 256, "worlds" PATH_DELIM "%s.tmp", world->name);

	cs_file fp = File_Open(tmpname, "wb");
	if(!fp) {
		Error_PrintSys(false);
		goto world_save_end;
	}

	if(!WriteInfo(world, fp))
		goto world_save_end;

	Bytef out[CHUNK_SIZE];
	cs_int32 ret;
	z_stream stream = {
		.zalloc = Z_NULL,
		.zfree = Z_NULL,
		.opaque = Z_NULL
	};

	if((ret = deflateInit2(
		&stream,
		Z_DEFAULT_COMPRESSION,
		Z_DEFLATED,
		31,
		8,
		Z_DEFAULT_STRATEGY
	)) != Z_OK) {
		ERROR_PRINT(ET_ZLIB, ret, false);
		goto world_save_end;
	}

	stream.next_in = World_GetBlockArray(world, &stream.avail_in);

	cs_byte zstate = 0;
	do {
		stream.next_out = out;
		stream.avail_out = CHUNK_SIZE;

		if((ret = deflate(&stream, zstate == 0 ? Z_NO_FLUSH : Z_FINISH)) == Z_STREAM_ERROR) {
			ERROR_PRINT(ET_ZLIB, ret, false);
			goto world_save_end;
		}

		if(stream.avail_out == CHUNK_SIZE) {
			if(zstate == 1)
				zstate = 2;
			else
				zstate = 1;
		} else {
			if(!File_Write(out, 1, CHUNK_SIZE - stream.avail_out, fp)) {
				Error_PrintSys(false);
				goto world_save_end;
			}
		}
	} while(zstate != 2);
	succ = (stream.avail_in == 0);

	world_save_end:
	File_Close(fp);
	deflateEnd(&stream);
	world->process = WP_NOPROC;
	if(succ)
		File_Rename(tmpname, path);
	Waitable_Signal(world->wait);
	if(world->saveUnload)
		World_Unload(world);
	return 0;
}

cs_bool World_Save(World *world, cs_bool unload) {
	if(world->process != WP_NOPROC || !world->modified || !world->loaded)
		return world->process == WP_SAVING;
	world->process = WP_SAVING;
	world->saveUnload = unload;
	Waitable_Reset(world->wait);
	Thread_Create(WorldSaveThread, world, true);
	return true;
}

THREAD_FUNC(WorldLoadThread) {
	World *world = (World *)param;
	cs_bool error = true;
	cs_char path[256];
	String_FormatBuf(path, 256, "worlds" PATH_DELIM "%s", world->name);

	cs_file fp = File_Open(path, "rb");
	if(!fp) {
		Error_PrintSys(false);
		goto world_load_done;
	}

	if(!ReadInfo(world, fp))
		goto world_load_done;

	World_AllocBlockArray(world);

	cs_int32 ret;
	Bytef in[CHUNK_SIZE];
	z_stream stream = {
		.zalloc = Z_NULL,
		.zfree = Z_NULL,
		.opaque = Z_NULL
	};

	if((ret = inflateInit2(&stream, 31)) != Z_OK) {
		ERROR_PRINT(ET_ZLIB, ret, false);
		goto world_load_done;
	}

	stream.next_out = World_GetBlockArray(world, NULL);

	do {
		stream.avail_in = (uLongf)File_Read(in, 1, CHUNK_SIZE, fp);
		if(File_Error(fp)) {
			Error_PrintSys(false);
			goto world_load_done;
		}

		if(stream.avail_in == 0) break;
		stream.next_in = in;

		do {
			stream.avail_out = CHUNK_SIZE;
			if((ret = inflate(&stream, Z_NO_FLUSH)) == Z_NEED_DICT || ret == Z_DATA_ERROR || ret == Z_MEM_ERROR) {
				ERROR_PRINT(ET_ZLIB, ret, false);
				goto world_load_done;
			}
		} while(stream.avail_out == 0);
	} while(ret != Z_STREAM_END);
	error = false;

	world_load_done:
	if(fp) File_Close(fp);
	inflateEnd(&stream);
	world->process = WP_NOPROC;
	world->saveUnload = false;
	if(error)
		World_Unload(world);
	Waitable_Signal(world->wait);
	return 0;
}

cs_bool World_Load(World *world) {
	if(world->loaded) return false;
	if(world->process != WP_NOPROC)
		return world->process == WP_LOADING;
	world->process = WP_LOADING;
	Waitable_Reset(world->wait);
	Thread_Create(WorldLoadThread, world, true);
	return true;
}

void World_Unload(World *world) {
	if(world->process != WP_NOPROC)
		Waitable_Wait(world->wait);
	if(world->wdata.size) {
		Memory_Free(world->wdata.ptr);
		world->wdata.size = 0;
		world->wdata.ptr = world->wdata.blocks = NULL;
	}
	world->loaded = false;
}

cs_str World_GetName(World *world) {
	if(!world->name) return "unnamed";
	return world->name;
}

cs_uint32 World_GetOffset(World *world, SVec *pos) {
	if(pos->x < 0 || pos->y < 0 || pos->z < 0) return 0;
	cs_uint32 offset = ((cs_uint32)pos->y * (cs_uint32)world->info.dimensions.z
	+ (cs_uint32)pos->z) * (cs_uint32)world->info.dimensions.x + (cs_uint32)pos->x;
	if(offset > world->wdata.size) return world->wdata.size;
	return offset;
}

cs_bool World_SetBlockO(World *world, cs_uint32 offset, BlockID id) {
	world->wdata.blocks[offset] = id;
	world->modified = true;
	return true;
}

cs_bool World_SetBlock(World *world, SVec *pos, BlockID id) {
	cs_uint32 offset = World_GetOffset(world, pos);
	return World_SetBlockO(world, offset, id);
}

BlockID World_GetBlock(World *world, SVec *pos) {
	cs_int32 offset = World_GetOffset(world, pos);
	return world->wdata.blocks[offset];
}
