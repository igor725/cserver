#include "core.h"
#include "platform.h"
#include "str.h"
#include "world.h"
#include "event.h"
#include "list.h"
#include "compr.h"
#include "client.h"

enum _EWorldDataItems {
	WDAT_DIMENSIONS,
	WDAT_SPAWNVEC,
	WDAT_SPAWNANG,
	WDAT_WEATHER,
	WDAT_ENVPROPS,
	WDAT_ENVCOLORS,
	WDAT_TEXTURE,
	WDAT_GENSEED,

	DT_END = 0xFF
};

AListField *World_Head = NULL;
World *World_Main = NULL;

World *World_Create(cs_str name) {
	if(!String_IsSafe(name)) return NULL;
	World *tmp = Memory_Alloc(1, sizeof(World));
	tmp->name = String_AllocCopy(name);
	tmp->taskann = Waitable_Create();
	tmp->mtx = Mutex_Create();

	/*
	 * Устанавливаем дефолтные значения
	 * согласно документации по CPE.
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
	World_WaitProcessFinish(world, WORLD_PROC_ALL);
	World_Lock(world, 0);
	AListField *tmp;
	List_Iter(tmp, World_Head) {
		if(tmp->value.ptr == world) {
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

cs_bool World_IsModified(World *world) {
	return ISSET(world->flags, WORLD_FLAG_MODIFIED);
}

cs_bool World_IsInMemory(World *world) {
	return ISSET(world->flags, WORLD_FLAG_INMEMORY);
}

cs_bool World_IsReadyToPlay(World *world) {
	return world->wdata.ptr != NULL &&
	ISSET(world->flags, WORLD_FLAG_ALLOCATED);
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

void World_SetSpawn(World *world, Vec *svec, Ang *sang) {
	if(svec) world->info.spawnVec = *svec;
	if(sang) world->info.spawnAng = *sang;
	if((svec || sang) && !ISSET(world->flags, WORLD_FLAG_MODIGNORE))
		world->flags |= WORLD_FLAG_MODIFIED;
}

cs_bool World_SetDimensions(World *world, const SVec *dims) {
	if (ISSET(world->flags, WORLD_FLAG_ALLOCATED)) {
		const SVec* currdim = &world->info.dimensions;
		return dims->x == currdim->x && dims->y == currdim->y && dims->z == currdim->z;
	}

	if(dims->x < 1 || dims->y < 1 || dims->z < 1) return false;
	cs_uint32 size = (cs_uint32)dims->x * (cs_uint32)dims->y;
	if((WORLD_MAX_SIZE / size) < (cs_uint32)dims->z) return false;
	size *= (cs_uint32)dims->z;
	world->info.dimensions = *dims;
	world->wdata.size = size;
	return true;
}

cs_bool World_SetEnvProp(World *world, EProp prop, cs_int32 value) {
	if(prop < WORLD_PROPS_COUNT) {
		if(!ISSET(world->flags, WORLD_FLAG_MODIGNORE))
			world->flags |= WORLD_FLAG_MODIFIED;
		world->info.props[prop] = value;
		world->info.modval |= CPE_WMODVAL_PROPS;
		world->info.modprop |= 2 ^ prop;
		return true;
	}

	return false;
}

cs_int32 World_GetEnvProp(World *world, EProp prop) {
	return prop < WORLD_PROPS_COUNT ? world->info.props[prop] : 0;
}

cs_bool World_SetTexturePack(World *world, cs_str url) {
	if(String_CaselessCompare(world->info.texturepack, url))
		return true;
	if(!ISSET(world->flags, WORLD_FLAG_MODIGNORE))
		world->flags |= WORLD_FLAG_MODIFIED;
	world->info.modval |= CPE_WMODVAL_TEXPACK;
	if(!url || String_Length(url) > 64) {
		world->info.texturepack[0] = '\0';
		return url == NULL;
	}
	if(!String_Copy(world->info.texturepack, MAX_STR_LEN, url)) {
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
	if(!ISSET(world->flags, WORLD_FLAG_MODIGNORE))
		world->flags |= WORLD_FLAG_MODIFIED;
	world->info.weatherType = type;
	world->info.modval |= CPE_WMODVAL_WEATHER;
	return true;
}

cs_bool World_SetEnvColor(World *world, EColor type, Color3* color) {
	if(type < WORLD_COLORS_COUNT) {
		if(!ISSET(world->flags, WORLD_FLAG_MODIGNORE))
			world->flags |= WORLD_FLAG_MODIFIED;
		world->info.modval |= CPE_WMODVAL_COLORS;
		world->info.modclr |= (1 << type);
		world->info.colors[type] = *color;
		return true;
	}

	return false;
}

void World_SetSeed(World *world, cs_uint32 seed) {
	if(!ISSET(world->flags, WORLD_FLAG_MODIGNORE))
		world->flags |= WORLD_FLAG_MODIFIED;

	world->info.seed = seed;
}

cs_bool World_FinishEnvUpdate(World *world) {
	preWorldEnvUpdate ev = {
		.world = world,
		.values = world->info.modval,
		.props = world->info.modprop,
		.colors = world->info.modclr
	};

	if(Event_Call(EVT_PREWORLDENVUPDATE, &ev)) {
		for(ClientID i = 0; i < MAX_CLIENTS; i++) {
			Client *client = Clients_List[i];
			if(client && Client_IsInWorld(client, world))
				Client_UpdateWorldInfo(client, world, false);
		}

		world->info.modclr = 0x00;
		world->info.modprop = 0x00;
		world->info.modval = CPE_WMODVAL_NONE;
		return true;
	}

	return false;
}

cs_byte World_CountPlayers(World *world) {
	cs_byte count = 0;
	for(ClientID i = 0; i < MAX_CLIENTS; i++) {
		Client *client = Clients_List[i];
		if(!client) continue;
		if(!Client_IsInWorld(client, world)) continue;
		if(Client_IsBot(client)) continue;
		count++;
	}
	return count;
}

cs_bool World_GetEnvColor(World *world, EColor type, Color3 *dst) {
	if(type < WORLD_COLORS_COUNT) {
		*dst = world->info.colors[type];
		return true;
	}
	return false;
}

EWeather World_GetWeather(World *world) {
	return world->info.weatherType;
}

cs_uint32 World_GetSeed(World *world) {
	return world->info.seed;
}

void World_AllocBlockArray(World *world) {
	if (world->flags & WORLD_FLAG_ALLOCATED) {
		Log_Warn("Double World(%p) reallocation!");
		return;
	}
	void *data = Memory_Alloc(world->wdata.size + 4, 1);
	*(cs_uint32 *)data = htonl(world->wdata.size);
	world->wdata.ptr = data;
	world->wdata.blocks = (BlockID *)data + 4;
	world->flags |= WORLD_FLAG_ALLOCATED;
}

cs_bool World_CleanBlockArray(World *world) {
	if(World_IsReadyToPlay(world)) {
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
	if(world->mtx) Mutex_Free(world->mtx);
	if(world->taskann) Waitable_Free(world->taskann);
	if(world->name) Memory_Free((void *)world->name);
	Memory_Free(world);
}

NOINL static cs_bool WriteWData(cs_file fp, cs_byte dataType, void *ptr, cs_int32 size) {
	return File_Write(&dataType, 1, 1, fp) && (size > 0 ? File_Write(ptr, size, 1, fp) : true);
}

#define _WriteWData(F, T, V) WriteWData(F, T, &V, sizeof(V))

INL static cs_bool WriteInfo(World *world, cs_file fp) {
	return File_Write((void *)&(cs_int32){WORLD_MAGIC}, 4, 1, fp) &&
	_WriteWData(fp, WDAT_DIMENSIONS, world->info.dimensions) &&
	_WriteWData(fp, WDAT_SPAWNVEC, world->info.spawnVec) &&
	_WriteWData(fp, WDAT_SPAWNANG, world->info.spawnAng) &&
	_WriteWData(fp, WDAT_WEATHER, world->info.weatherType) &&
	_WriteWData(fp, WDAT_ENVPROPS, world->info.props) &&
	_WriteWData(fp, WDAT_ENVCOLORS, world->info.colors) &&
	_WriteWData(fp, WDAT_TEXTURE, world->info.texturepack) &&
	_WriteWData(fp, WDAT_GENSEED, world->info.seed) &&
	WriteWData(fp, DT_END, NULL, 0);
}

#define ReadWData(F, P) File_Read(&P, sizeof(P), 1, F)

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
			case WDAT_DIMENSIONS:
				if(ReadWData(fp, dims) != 1)
					return false;
				if(!World_SetDimensions(world, &dims))
					return false;
				break;
			case WDAT_SPAWNVEC:
				if(ReadWData(fp, world->info.spawnVec) != 1)
					return false;
				break;
			case WDAT_SPAWNANG:
				if(ReadWData(fp, world->info.spawnAng) != 1)
					return false;
				break;
			case WDAT_WEATHER:
				if(ReadWData(fp, world->info.weatherType) != 1)
					return false;
				break;
			case WDAT_ENVPROPS:
				if(ReadWData(fp, world->info.props) != 1)
					return false;
				break;
			case WDAT_ENVCOLORS:
				if(ReadWData(fp, world->info.colors) != 1)
					return false;
				break;
			case WDAT_TEXTURE:
				if(ReadWData(fp, world->info.texturepack) != 1)
					return false;
				break;
			case WDAT_GENSEED:
				if(ReadWData(fp, world->info.seed) != 1)
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
		World_FinishProcess(world, WORLD_PROC_SAVING);
		return 0;
	}

	cs_byte *wdata = World_GetBlockArray(world, &wsize);
	Compr_SetInBuffer(&world->compr, wdata, wsize);

	cs_char path[MAX_PATH_LEN], tmpname[MAX_PATH_LEN], out[CHUNK_SIZE];
	String_FormatBuf(path, MAX_PATH_LEN, "worlds" PATH_DELIM "%s.cws", world->name);
	String_FormatBuf(tmpname, MAX_PATH_LEN, "worlds" PATH_DELIM "%s.tmp", world->name);

	Directory_Ensure("worlds");
	cs_file fp = File_Open(tmpname, "wb");
	if(!fp) {
		world->error.code = WORLD_ERROR_IOFAIL;
		world->error.extra = WORLD_EXTRA_IO_OPEN;
		Compr_Reset(&world->compr);
		World_FinishProcess(world, WORLD_PROC_SAVING);
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
	} else {
		world->error.code = WORLD_ERROR_IOFAIL;
		world->error.extra = WORLD_EXTRA_IO_WRITE;
		Compr_Reset(&world->compr);
		File_Close(fp);
		World_FinishProcess(world, WORLD_PROC_SAVING);
		return 0;
	}

	File_Close(fp);
	Compr_Reset(&world->compr);
	if(compr_ok && !File_Rename(tmpname, path)) {
		world->error.code = WORLD_ERROR_IOFAIL;
		world->error.extra = WORLD_EXTRA_IO_RENAME;
		World_FinishProcess(world, WORLD_PROC_SAVING);
		return 0;
	}

	world->error.code = WORLD_ERROR_SUCCESS;
	world->error.extra = WORLD_EXTRA_NOINFO;
	World_FinishProcess(world, WORLD_PROC_SAVING);
	return 0;
}

cs_bool World_Lock(World *world, cs_ulong timeout) {
	Mutex_Lock(world->mtx);
	return true;
}

void World_Unlock(World *world) {
	Mutex_Unlock(world->mtx);
}

void World_StartProcess(World *world, WorldProcs process) {
	Mutex_Lock(world->mtx);
	++world->prCount[process];
	world->processes |= BIT(process);
	Mutex_Unlock(world->mtx);
}

void World_FinishProcess(World *world, WorldProcs process) {
	Mutex_Lock(world->mtx);
	if ((--world->prCount[process]) == 0) {
		world->processes &= ~BIT(process);
		Waitable_Signal(world->taskann);
	}
	Mutex_Unlock(world->mtx);
}

void World_WaitProcessFinish(World *world, WorldProcs process) {
	if (process == WORLD_PROC_ALL) {
		while (world->processes)
			Waitable_Wait(world->taskann);
		return;
	}
	while (world->processes & BIT(process))
		Waitable_Wait(world->taskann);
}

void World_SetInMemory(World *world, cs_bool state) {
	if(state)
		world->flags |= WORLD_FLAG_INMEMORY;
	else
		world->flags &= ~WORLD_FLAG_INMEMORY;
}

void World_SetIgnoreModifications(World *world, cs_bool state) {
	if(state)
		world->flags |= WORLD_FLAG_MODIGNORE;
	else
		world->flags &= ~WORLD_FLAG_MODIGNORE;
}

cs_bool World_Save(World *world) {
	if(World_IsInMemory(world)) {
		world->error.code = WORLD_ERROR_INMEMORY;
		world->error.extra = WORLD_EXTRA_NOINFO;
		return false;
	}

	if(!World_IsModified(world))
		return World_IsReadyToPlay(world);

	World_WaitProcessFinish(world, WORLD_PROC_ALL);
	World_StartProcess(world, WORLD_PROC_SAVING);
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
		World_FinishProcess(world, WORLD_PROC_LOADING);
		return 0;
	}

	cs_char path[256];
	String_FormatBuf(path, 256, "worlds" PATH_DELIM "%s.cws", world->name);

	cs_file fp = File_Open(path, "rb");
	if(!fp) {
		world->error.code = WORLD_ERROR_IOFAIL;
		world->error.extra = WORLD_EXTRA_IO_OPEN;
		Compr_Reset(&world->compr);
		World_FinishProcess(world, WORLD_PROC_LOADING);
		return 0;
	}

	if(ReadInfo(world, fp)) {
		cs_uint32 wsize = 0;
		cs_byte in[CHUNK_SIZE];
		if (!ISSET(world->flags, WORLD_FLAG_ALLOCATED))
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
	} else {
		world->error.code = WORLD_ERROR_INFOREAD;
		world->error.extra = WORLD_EXTRA_NOINFO;
	}

	if(fp) File_Close(fp);
	Compr_Reset(&world->compr);
	if(world->error.code == WORLD_ERROR_SUCCESS)
		Event_Call(EVT_ONWORLDSTATUSCHANGE, world);
	World_FinishProcess(world, WORLD_PROC_LOADING);
	return 0;
}

cs_bool World_Load(World *world) {
	if(World_IsInMemory(world)) {
		world->error.code = WORLD_ERROR_INMEMORY;
		world->error.extra = WORLD_EXTRA_NOINFO;
		return false;
	}

	World_WaitProcessFinish(world, WORLD_PROC_ALL);
	World_StartProcess(world, WORLD_PROC_LOADING);
	Thread_Create(WorldLoadThread, world, false);
	return true;
}

void World_FreeBlockArray(World *world) {
	if(world->wdata.size) {
		Memory_Free(world->wdata.ptr);
		world->wdata.size = 0;
		world->wdata.ptr = world->wdata.blocks = NULL;
	}
	world->flags &= ~WORLD_FLAG_ALLOCATED;
}

void World_Unload(World *world) {
	World_WaitProcessFinish(world, WORLD_PROC_ALL);
	World_FreeBlockArray(world);
	Event_Call(EVT_ONWORLDSTATUSCHANGE, world);
}

cs_str World_GetName(World *world) {
	return world->name;
}

void World_GetSpawn(World *world, Vec *svec, Ang *sang) {
	if(svec) *svec = world->info.spawnVec;
	if(sang) *sang = world->info.spawnAng;
}

void World_GetDimensions(World *world, SVec *dims) {
	if(dims) *dims = world->info.dimensions;
}

cs_uint32 World_GetOffset(World *world, SVec *pos) {
	if(pos->x < 0 || pos->y < 0 || pos->z < 0) return WORLD_INVALID_OFFSET;
	if(pos->x >= world->info.dimensions.x ||
	pos->y >= world->info.dimensions.y ||
	pos->z >= world->info.dimensions.z) return WORLD_INVALID_OFFSET;
	cs_uint32 offset = ((cs_uint32)pos->y * (cs_uint32)world->info.dimensions.z
	+ (cs_uint32)pos->z) * (cs_uint32)world->info.dimensions.x + (cs_uint32)pos->x;
	return offset < world->wdata.size ? offset : WORLD_INVALID_OFFSET;
}

cs_bool World_SetBlockO(World *world, cs_uint32 offset, BlockID id) {
	if(world->wdata.size <= offset) return false;
	world->wdata.blocks[offset] = id;
	if(!ISSET(world->flags, WORLD_FLAG_MODIGNORE))
		world->flags |= WORLD_FLAG_MODIFIED;
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
