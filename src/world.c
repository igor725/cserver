#include "core.h"
#include "platform.h"
#include "str.h"
#include "world.h"
#include "event.h"
#include "list.h"
#include "compr.h"
#include "block.h"
#include "client.h"

enum _EWorldDataItems {
	DT_DIM,
	DT_SV,
	DT_SA,
	DT_WT,
	DT_PROPS,
	DT_COLORS,
	DT_TEXPACK,

	DT_END = 0xFF
};

AListField *World_Head = NULL;
World *World_Main = NULL;

World *World_Create(cs_str name) {
	World *tmp = Memory_Alloc(1, sizeof(World));
	tmp->name = String_AllocCopy(name);
	tmp->sem = Semaphore_Create(1, 1);
	tmp->taskw = Waitable_Create();
	Waitable_Signal(tmp->taskw);

	/*
	** Устанавливаем дефолтные значения
	** согласно документации по CPE.
	*/
	cs_int32 *props = tmp->info.props;
	props[WORLD_PROP_SIDEBLOCK] = BLOCK_BEDROCK;
	props[WORLD_PROP_EDGEBLOCK] = BLOCK_WATER;
	props[WORLD_PROP_CLOUDSLEVEL] = 256;
	props[WORLD_PROP_FOGDIST] = 0;
	props[WORLD_PROP_SPDCLOUDS] = 256;
	props[WORLD_PROP_SPDWEATHER] = 256;
	props[WORLD_PROP_FADEWEATHER] = 128;
	props[WORLD_PROP_EXPFOG] = 0;
	props[WORLD_PROP_SIDEOFFSET] = -2;

	Color3* colors = tmp->info.colors;
	for(int i = 0; i < WORLD_COLORS_COUNT; i++) {
		colors[i].r = -1;
		colors[i].g = -1;
		colors[i].b = -1;
	}

	return tmp;
}

void World_Add(World *world) {
	if(AList_AddField(&World_Head, world)) {
		if(!World_Main) World_Main = world;
		Event_Call(EVT_ONWORLDADDED, world);
	}
}

cs_bool World_Remove(World *world) {
	if(world == World_Main) return false;
	World_WaitAllTasks(world);
	World_Lock(world, 0);
	AListField *tmp;
	List_Iter(tmp, World_Head) {
		if(AList_GetValue(tmp).ptr == world) {
			AList_Remove(&World_Head, tmp);
			break;
		}
	}
	Event_Call(EVT_ONWORLDREMOVED, world);
	World_Unload(world);
	World_Unlock(world);
	World_Free(world);
	return true;
}

cs_bool World_IsInMemory(World *world) {
	return world->inmemory;
}

cs_bool World_IsReadyToPlay(World *world) {
	return world->wdata.ptr != NULL && world->loaded;
}

World *World_GetByName(cs_str name) {
	AListField *tmp;
	List_Iter(tmp, World_Head) {
		World *world = (World *)tmp->value.ptr;
		if(world && String_CaselessCompare(world->name, name))
			return world;
	}
	return NULL;
}

void World_SetDimensions(World *world, const SVec *dims) {
	world->info.dimensions = *dims;
	world->wdata.size = (cs_uint32)dims->x * (cs_uint32)dims->y * (cs_uint32)dims->z;
}

cs_bool World_SetEnvProp(World *world, EProp property, cs_int32 value) {
	if(property > WORLD_PROPS_COUNT) return false;
	world->modified = true;
	world->info.props[property] = value;
	world->info.modval |= MV_PROPS;
	world->info.modprop |= 2 ^ property;
	return true;
}

cs_int32 World_GetEnvProp(World *world, EProp property) {
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
		return url == NULL;
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

cs_bool World_SetWeather(World *world, EWeather type) {
	if(type > WORLD_WEATHER_SNOW) return false;
	world->info.weatherType = type;
	world->modified = true;
	world->info.modval |= MV_WEATHER;
	Event_Call(EVT_ONWEATHER, world);
	return true;
}

cs_bool World_SetEnvColor(World *world, EColors type, Color3* color) {
	if(type > WORLD_COLORS_COUNT) return false;
	world->info.modval |= MV_COLORS;
	world->info.modclr |= (1 << type);
	world->modified = true;
	world->info.colors[type] = *color;
	Event_Call(EVT_ONCOLOR, world);
	return true;
}

void World_FinishEnvUpdate(World *world) {
	for(ClientID i = 0; i < MAX_CLIENTS; i++) {
		Client *client = Clients_List[i];
		if(client && Client_IsInWorld(client, world))
			Client_UpdateWorldInfo(client, world, false);
	}
	world->info.modclr = 0x00;
	world->info.modprop = 0x00;
	world->info.modval = MV_NONE;
}

cs_byte World_CountPlayers(World *world) {
	cs_byte count = 0;
	for(ClientID i = 0; i < MAX_CLIENTS; i++) {
		Client *client = Clients_List[i];
		if(client && Client_IsInWorld(client, world))
			count++;
	}
	return count;
}

Color3* World_GetEnvColor(World *world, EColors type) {
	if(type > WORLD_COLORS_COUNT) return false;
	return &world->info.colors[type];
}

EWeather World_GetWeather(World *world) {
	return world->info.weatherType;
}

void World_AllocBlockArray(World *world) {
	void *data = Memory_Alloc(world->wdata.size + 4, 1);
	*(cs_uint32 *)data = htonl(world->wdata.size);
	world->wdata.ptr = data;
	world->wdata.blocks = (BlockID *)data + 4;
	world->loaded = true;
}

cs_bool World_CleanBlockArray(World *world) {
	if(world->loaded && world->wdata.blocks) {
		Memory_Fill(world->wdata.blocks, world->wdata.size, 0);
		return true;
	}

	return false;
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
	while(world->headNode) {
		Memory_Free(world->headNode->value.ptr);
		KList_Remove(&world->headNode, world->headNode);
	}
	Compr_Cleanup(&world->compr);
	World_FreeBlockArray(world);
	Semaphore_Free(world->sem);
	Waitable_Free(world->taskw);
	if(world->name) Memory_Free((void *)world->name);
	Memory_Free(world);
}

NOINL static cs_bool WriteWData(cs_file fp, cs_byte dataType, void *ptr, cs_int32 size) {
	return File_Write(&dataType, 1, 1, fp) && (size > 0 ? File_Write(ptr, size, 1, fp) : true);
}

INL static cs_bool WriteInfo(World *world, cs_file fp) {
	if(!File_Write((cs_char *)&(cs_int32){WORLD_MAGIC}, 4, 1, fp)) {
		world->error.code = WORLD_ERROR_IOFAIL;
		world->error.extra = WORLD_EXTRA_IO_WRITE;
		return false;
	}
	return WriteWData(fp, DT_DIM, &world->info.dimensions, sizeof(SVec)) &&
	WriteWData(fp, DT_SV, &world->info.spawnVec, sizeof(Vec)) &&
	WriteWData(fp, DT_SA, &world->info.spawnAng, sizeof(Ang)) &&
	WriteWData(fp, DT_WT, &world->info.weatherType, 1) &&
	WriteWData(fp, DT_PROPS, world->info.props, 4 * WORLD_PROPS_COUNT) &&
	WriteWData(fp, DT_COLORS, world->info.colors, sizeof(Color3) * WORLD_COLORS_COUNT) &&
	WriteWData(fp, DT_TEXPACK, world->info.texturepack, sizeof(world->info.texturepack)) &&
	WriteWData(fp, DT_END, NULL, 0);
}

static cs_bool ReadInfo(World *world, cs_file fp) {
	cs_byte id = 0;
	cs_uint32 magic = 0;
	if(!File_Read(&magic, 4, 1, fp))
		return false;

	if(WORLD_MAGIC != magic) {
		world->error.code = WORLD_ERROR_INFOREAD;
		return false;
	}

	SVec dims;
	while(File_Read(&id, 1, 1, fp) == 1) {
		switch (id) {
			case DT_DIM:
				if(File_Read(&dims, sizeof(SVec), 1, fp) != 1)
					return false;
				World_SetDimensions(world, &dims);
				break;
			case DT_SV:
				if(File_Read(&world->info.spawnVec, sizeof(Vec), 1, fp) != 1)
					return false;
				break;
			case DT_SA:
				if(File_Read(&world->info.spawnAng, sizeof(Ang), 1, fp) != 1)
					return false;
				break;
			case DT_WT:
				if(File_Read(&world->info.weatherType, 1, 1, fp) != 1)
					return false;
				break;
			case DT_PROPS:
				if(File_Read(world->info.props, 4 * WORLD_PROPS_COUNT, 1, fp) != 1)
					return false;
				break;
			case DT_COLORS:
				if(File_Read(world->info.colors, sizeof(Color3) * WORLD_COLORS_COUNT, 1, fp) != 1)
					return false;
				break;
			case DT_TEXPACK:
				if(File_Read(world->info.texturepack, sizeof(world->info.texturepack), 1, fp) != 1)
					return false;
				break;
			case DT_END:
				return true;
			default:
				world->error.code = WORLD_ERROR_INFOREAD;
				world->error.extra = WORLD_EXTRA_UNKNOWN_DATA_TYPE;
				return false;
		}
	}

	return false;
}

#define CHUNK_SIZE 16384

THREAD_FUNC(WorldSaveThread) {
	World *world = (World *)param;
	cs_uint32 wsize = 0;
	cs_bool compr_ok;

	if((compr_ok = Compr_Init(&world->compr, COMPR_TYPE_GZIP)) == false) {
		world->error.code = WORLD_ERROR_COMPR;
		world->error.extra = WORLD_EXTRA_COMPR_INIT;
		World_Unlock(world);
		return 0;
	}

	cs_byte *wdata = World_GetBlockArray(world, &wsize);
	Compr_SetInBuffer(&world->compr, wdata, wsize);

	cs_char path[256], tmpname[256], out[CHUNK_SIZE];
	String_FormatBuf(path, 256, "worlds" PATH_DELIM "%s.cws", world->name);
	String_FormatBuf(tmpname, 256, "worlds" PATH_DELIM "%s.tmp", world->name);

	cs_file fp = File_Open(tmpname, "wb");
	if(!fp) {
		world->error.code = WORLD_ERROR_IOFAIL;
		world->error.extra = WORLD_EXTRA_IO_OPEN;
		Compr_Reset(&world->compr);
		World_Unlock(world);
		return 0;
	}

	if((compr_ok = WriteInfo(world, fp)) == true) {
		do {
			Compr_SetOutBuffer(&world->compr, out, CHUNK_SIZE);
			if((compr_ok = Compr_Update(&world->compr)) == true) {
				if(File_Write(out, 1, world->compr.written, fp) != world->compr.written) {
					world->error.code = WORLD_ERROR_IOFAIL;
					world->error.extra = WORLD_EXTRA_IO_WRITE;
					compr_ok = false;
					break;
				}
			} else break;
		} while(world->compr.state != COMPR_STATE_DONE);
	}

	File_Close(fp);
	Compr_Reset(&world->compr);
	if(compr_ok && !File_Rename(tmpname, path)) {
		world->error.code = WORLD_ERROR_IOFAIL;
		world->error.extra = WORLD_EXTRA_IO_RENAME;
		World_Unlock(world);
		return 0;
	}

	world->error.code = WORLD_ERROR_SUCCESS;
	world->error.extra = WORLD_EXTRA_NOINFO;
	World_Unlock(world);
	return 0;
}

cs_bool World_Lock(World *world, cs_ulong timeout) {
	if(timeout > 0 && !Semaphore_TryWait(world->sem, timeout))
		return false;
	else Semaphore_Wait(world->sem);

	return true;
}

void World_Unlock(World *world) {
	Semaphore_Post(world->sem);
}

void World_StartTask(World *world) {
	World_Lock(world, 0);
	if(!world->taskc++)
		Waitable_Reset(world->taskw);
	World_Unlock(world);
}

void World_EndTask(World *world) {
	World_Lock(world, 0);
	if(--world->taskc == 0)
		Waitable_Signal(world->taskw);
	World_Unlock(world);
}

void World_WaitAllTasks(World *world) {
	Waitable_Wait(world->taskw);
}

void World_SetInMemory(World *world, cs_bool state) {
	world->inmemory = state;
}

cs_bool World_Save(World *world) {
	if(world->inmemory) {
		world->error.code = WORLD_ERROR_INMEMORY;
		world->error.extra = WORLD_EXTRA_NOINFO;
		return false;
	}
	if(!world->modified)
		return world->loaded;
	World_Lock(world, 0);
	Thread_Create(WorldSaveThread, world, true);
	return true;
}

cs_bool World_HasError(World *world) {
	return world->error.code != WORLD_ERROR_SUCCESS;
}

EWorldError World_PopError(World *world, EWorldExtra *extra) {
	if(extra) *extra = world->error.extra;
	EWorldError code = world->error.code;
	world->error.code = WORLD_ERROR_SUCCESS;
	world->error.extra = WORLD_EXTRA_NOINFO;
	return code;
}

THREAD_FUNC(WorldLoadThread) {
	World *world = (World *)param;
	cs_bool compr_ok;

	if((compr_ok = Compr_Init(&world->compr, COMPR_TYPE_UNGZIP)) == false) {
		world->error.code = WORLD_ERROR_COMPR;
		world->error.extra = WORLD_EXTRA_COMPR_INIT;
		World_Unlock(world);
		return 0;
	}

	cs_char path[256];
	String_FormatBuf(path, 256, "worlds" PATH_DELIM "%s.cws", world->name);

	cs_file fp = File_Open(path, "rb");
	if(!fp) {
		world->error.code = WORLD_ERROR_IOFAIL;
		world->error.extra = WORLD_EXTRA_IO_OPEN;
		Compr_Reset(&world->compr);
		World_Unlock(world);
		return 0;
	}

	if(ReadInfo(world, fp)) {
		cs_uint32 wsize = 0;
		cs_byte in[CHUNK_SIZE];
		World_AllocBlockArray(world);
		cs_byte *data = World_GetBlockArray(world, &wsize);
		Compr_SetOutBuffer(&world->compr, data, wsize);
		cs_uint32 indatasize;
		while((indatasize = (cs_uint32)File_Read(in, 1, CHUNK_SIZE, fp)) > 0) {
			Compr_SetInBuffer(&world->compr, in, indatasize);
			if((compr_ok = Compr_Update(&world->compr)) == false) {
				world->error.code = WORLD_ERROR_COMPR;
				world->error.extra = WORLD_EXTRA_COMPR_PROC;
				break;
			}
		}
		world->error.code = WORLD_ERROR_SUCCESS;
		world->error.extra = WORLD_EXTRA_NOINFO;
	}

	if(fp) File_Close(fp);
	Compr_Reset(&world->compr);
	Event_Call(EVT_ONWORLDLOADED, world);
	World_Unlock(world);
	return 0;
}

cs_bool World_Load(World *world) {
	if(world->inmemory) {
		world->error.code = WORLD_ERROR_INMEMORY;
		world->error.extra = WORLD_EXTRA_NOINFO;
		return false;
	}
	if(world->loaded)
		return false;
	World_Lock(world, 0);
	Thread_Create(WorldLoadThread, world, false);
	return true;
}

void World_FreeBlockArray(World *world) {
	if(world->wdata.size) {
		Memory_Free(world->wdata.ptr);
		world->wdata.size = 0;
		world->wdata.ptr = world->wdata.blocks = NULL;
	}
	world->loaded = false;
}

void World_Unload(World *world) {
	World_WaitAllTasks(world);
	World_FreeBlockArray(world);
	Event_Call(EVT_ONWORLDUNLOADED, world);
}

cs_str World_GetName(World *world) {
	return world->name;
}

void World_GetDimensions(World *world, SVec *dims) {
	if(dims) *dims = world->info.dimensions;
}

cs_uint32 World_GetOffset(World *world, SVec *pos) {
	if(pos->x < 0 || pos->y < 0 || pos->z < 0) return (cs_uint32)-1;
	if(pos->x >= world->info.dimensions.x ||
	pos->y >= world->info.dimensions.y ||
	pos->z >= world->info.dimensions.z) return (cs_uint32)-1;
	cs_uint32 offset = ((cs_uint32)pos->y * (cs_uint32)world->info.dimensions.z
	+ (cs_uint32)pos->z) * (cs_uint32)world->info.dimensions.x + (cs_uint32)pos->x;
	return offset < world->wdata.size ? offset : (cs_uint32)-1;
}

cs_bool World_SetBlockO(World *world, cs_uint32 offset, BlockID id) {
	if(world->wdata.size <= offset) return false;
	world->wdata.blocks[offset] = id;
	world->modified = true;
	return true;
}

cs_bool World_SetBlock(World *world, SVec *pos, BlockID id) {
	return World_SetBlockO(world, World_GetOffset(world, pos), id);
}

BlockID World_GetBlockO(World *world, cs_uint32 offset) {
	if(offset >= world->wdata.size) return (BlockID)-1;
	return world->wdata.blocks[offset];
}

BlockID World_GetBlock(World *world, SVec *pos) {
	return World_GetBlockO(world, World_GetOffset(world, pos));
}
