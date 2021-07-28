#include "core.h"
#include "str.h"
#include "log.h"
#include "list.h"
#include "platform.h"
#include "block.h"
#include "server.h"
#include "protocol.h"
#include "client.h"
#include "event.h"
#include "heartbeat.h"
#include "lang.h"
#include <zlib.h>

AListField *headAssocType = NULL,
*headCGroup = NULL;

static AListField *AGetType(cs_uint16 type) {
	AListField *ptr = NULL;

	List_Iter(ptr, headAssocType) {
		if(ptr->value.num16 == type) return ptr;
	}

	return NULL;
}

static KListField *AGetNode(Client *client, cs_uint16 type) {
	KListField *ptr = NULL;

	List_Iter(ptr, client->headNode) {
		if(ptr->key.num16 == type) return ptr;
	}

	return NULL;
}

cs_uint16 Assoc_NewType(void) {
	cs_uint16 next_id = headAssocType ? headAssocType->value.num16 + 1 : 0;
	AList_AddField(&headAssocType, NULL)->value.num16 = next_id;
	return next_id;
}

cs_bool Assoc_DelType(cs_uint16 type, cs_bool freeData) {
	AListField *tptr = AGetType(type);
	if(!tptr) return false;

	for(ClientID id = 0; id < MAX_CLIENTS; id++) {
		if(Clients_List[id])
			Assoc_Remove(Clients_List[id], type, freeData);
	}

	AList_Remove(&headAssocType, tptr);
	return true;
}

cs_bool Assoc_Set(Client *client, cs_uint16 type, void *ptr) {
	if(AGetNode(client, type)) return false;
	KListField *nptr = AGetNode(client, type);
	if(!nptr) nptr = KList_Add(&client->headNode, NULL, NULL);
	nptr->key.num16 = type;
	nptr->value.ptr = ptr;
	return true;
}

void *Assoc_GetPtr(Client *client, cs_uint16 type) {
	KListField *nptr = AGetNode(client, type);
	if(nptr) return nptr->value.ptr;
	return NULL;
}

cs_bool Assoc_Remove(Client *client, cs_uint16 type, cs_bool freeData) {
	KListField *nptr = AGetNode(client, type);
	if(!nptr) return false;
	if(freeData) Memory_Free(nptr->value.ptr);
	KList_Remove(&client->headNode, nptr);
	return true;
}

CGroup *Group_Add(cs_int16 gid, cs_str gname, cs_byte grank) {
	CGroup *gptr = Group_GetByID(gid);
	if(!gptr) {
		gptr = Memory_Alloc(1, sizeof(CGroup));
		gptr->id = gid;
		gptr->field = AList_AddField(&headCGroup, gptr);
	}

	if(gptr->name) Memory_Free((void *)gptr->name);
	gptr->name = String_AllocCopy(gname);
	gptr->rank = grank;
	return gptr;
}

CGroup *Group_GetByID(cs_int16 gid) {
	CGroup *gptr = NULL;
	AListField *lptr = NULL;

	List_Iter(lptr, headCGroup) {
		CGroup *tmp = lptr->value.ptr;
		if(tmp->id == gid) {
			gptr = tmp;
			break;
		}
	}

	return gptr;
}

cs_bool Group_Remove(cs_int16 gid) {
	CGroup *cg = Group_GetByID(gid);
	if(!cg) return false;

	for(ClientID id = 0; id < MAX_CLIENTS; id++) {
		Client *client = Clients_List[id];
		if(client && Client_GetGroupID(client) == gid)
			Client_SetGroup(client, -1);
	}

	Memory_Free((void *)cg->name);
	Memory_Free(cg);
	AList_Remove(&headCGroup, cg->field);

	return true;
}

cs_byte Clients_GetCount(cs_int32 state) {
	cs_byte count = 0;
	for(ClientID i = 0; i < MAX_CLIENTS; i++) {
		Client *client = Clients_List[i];
		if(!client) continue;
		PlayerData *pd = client->playerData;
		if(pd && pd->state == state) count++;
	}
	return count;
}

void Clients_UpdateWorldInfo(World *world) {
	for(ClientID i = 0; i < MAX_CLIENTS; i++) {
		Client *cl = Clients_List[i];
		if(cl && Client_IsInWorld(cl, world))
			Client_UpdateWorldInfo(cl, world, false);
	}
}

void Clients_KickAll(cs_str reason) {
	for(ClientID i = 0; i < MAX_CLIENTS; i++) {
		Client *client = Clients_List[i];
		if(client) Client_Kick(client, reason);
	}
}

Client *Client_New(Socket fd, cs_uint32 addr) {
	Client *tmp = Memory_Alloc(1, sizeof(Client));
	tmp->sock = fd;
	tmp->addr = addr;
	tmp->id = CLIENT_SELF;
	tmp->mutex = Mutex_Create();
	tmp->rdbuf = Memory_Alloc(134, 1);
	tmp->wrbuf = Memory_Alloc(2048, 1);
	return tmp;
}

cs_str Client_GetName(Client *client) {
	if(!client->playerData) return "unnamed";
	return client->playerData->name;
}

cs_str Client_GetAppName(Client *client) {
	if(!client->cpeData) return "vanilla client";
	return client->cpeData->appName;
}

cs_str Client_GetSkin(Client *client) {
	CPEData *cpd = client->cpeData;
	if(!cpd || !cpd->skin)
		return Client_GetName(client);
	return cpd->skin;
}

Client *Client_GetByName(cs_str name) {
	for(ClientID i = 0; i < MAX_CLIENTS; i++) {
		Client *client = Clients_List[i];
		if(!client) continue;
		PlayerData *pd = client->playerData;
		if(pd && String_CaselessCompare(pd->name, name))
			return client;
	}
	return NULL;
}

Client *Client_GetByID(ClientID id) {
	if(id < 0) return NULL;
	return id < MAX_CLIENTS ? Clients_List[id] : NULL;
}

World *Client_GetWorld(Client *client) {
	if(!client->playerData) return NULL;
	return client->playerData->world;
}

cs_int8 Client_GetFluidLevel(Client *client) {
	PlayerData *pd = client->playerData;
	World *world = pd->world;
	SVec tpos; SVec_Copy(tpos, pd->position)

	BlockID id;
	cs_byte level = 2;

	test_wtrlevel:
	if((id = World_GetBlock(world, &tpos)) > 7 && id < 12) {
		return level;
	}

	if(level > 1) {
		level--;
		tpos.y -= 1;
		goto test_wtrlevel;
	}

	return 0;
}

static CGroup dgroup = {-1, 0, "", NULL};

CGroup *Client_GetGroup(Client *client) {
	if(!client->cpeData) return &dgroup;
	CGroup *gptr = Group_GetByID(client->cpeData->group);
	return !gptr ? &dgroup : gptr;
}

cs_int16 Client_GetGroupID(Client *client) {
	if(!client->cpeData) return -1;
	return client->cpeData->group;
}

cs_int16 Client_GetModel(Client *client) {
	if(!client->cpeData) return 256;
	return client->cpeData->model;
}

BlockID Client_GetHeldBlock(Client *client) {
	if(!client->cpeData) return BLOCK_AIR;
	return client->cpeData->heldBlock;
}

cs_int32 Client_GetExtVer(Client *client, cs_uint32 exthash) {
	CPEData *cpd = client->cpeData;
	if(!cpd) return false;

	CPEExt *ptr = cpd->headExtension;
	while(ptr) {
		if(ptr->hash == exthash) return ptr->version;
		ptr = ptr->next;
	}
	return false;
}

cs_bool Client_Despawn(Client *client) {
	PlayerData *pd = client->playerData;
	if(!pd || !pd->spawned) return false;
	pd->spawned = false;
	Client *other;
	for(cs_uint32 i = 0; (other = Clients_List[i]) != NULL; i++)
		Vanilla_WriteDespawn(other, client);
	Event_Call(EVT_ONDESPAWN, client);
	return true;
}

#define CHUNK_SIZE 1024

THREAD_FUNC(WorldSendThread) {
	Client *client = (Client *)param;
	if(client->closed) return 0;
	PlayerData *pd = client->playerData;
	World *world = pd->world;

	if(world->process == WP_LOADING)
		Waitable_Wait(world->wait);

	if(!world->loaded) {
		Client_Kick(client, Lang_Get(Lang_KickGrp, 6));
		return 0;
	}

	Vanilla_WriteLvlInit(client, World_GetBlockArraySize(world));
	Mutex_Lock(client->mutex);
	cs_byte *data = (cs_byte *)client->wrbuf;

	*data++ = 0x03;
	cs_uint16 *len = (cs_uint16 *)data++;
	Bytef *out = ++data;

	cs_int32 ret, wndBits;
	z_stream stream = {0};
	stream.zalloc = Z_NULL;
	stream.zfree = Z_NULL;
	stream.opaque = Z_NULL;

	if(Client_GetExtVer(client, EXT_FASTMAP)) {
		stream.next_in = World_GetBlockArray(world, &stream.avail_in);
		wndBits = -15;
	} else {
		stream.next_in = World_GetData(world, &stream.avail_in);
		wndBits = 31;
	}

	if((ret = deflateInit2(
		&stream,
		Z_DEFAULT_COMPRESSION,
		Z_DEFLATED,
		wndBits,
		8,
		Z_DEFAULT_STRATEGY)) != Z_OK) {
			Log_Error("deflateInit2 error: %s", zError(ret));
		pd->state = STATE_WLOADERR;
		return 0;
	}

	cs_byte zstate = 0;
	do {
		stream.next_out = out;
		stream.avail_out = CHUNK_SIZE;

		if((ret = deflate(&stream, zstate == 0 ? Z_NO_FLUSH : Z_FINISH)) == Z_STREAM_ERROR) {
			pd->state = STATE_WLOADERR;
			goto world_send_end;
		}

		if(stream.avail_out == CHUNK_SIZE) {
			if(zstate == 1)
				zstate = 2;
			else
				zstate = 1;
		} else {
			*len = htons(CHUNK_SIZE - (cs_uint16)stream.avail_out);
			if(client->closed || !Client_Send(client, CHUNK_SIZE + 4)) {
				pd->state = STATE_WLOADERR;
				goto world_send_end;
			}
		}
	} while(zstate != 2);
	pd->state = STATE_WLOADDONE;

	world_send_end:
	deflateEnd(&stream);
	Mutex_Unlock(client->mutex);
	if(pd->state == STATE_WLOADDONE) {
		pd->state = STATE_INGAME;
		pd->position = world->info.spawnVec;
		pd->angle = world->info.spawnAng;
		Event_Call(EVT_PRELVLFIN, client);
		if(Client_GetExtVer(client, EXT_BLOCKDEF)) {
			for(BlockID id = 0; id < 255; id++) {
				BlockDef *bdef = Block_GetDefinition(id);
				if(bdef) Client_DefineBlock(client, bdef);
			}
		}
		Vanilla_WriteLvlFin(client, &world->info.dimensions);
		Client_Spawn(client);
	} else
		Client_Kick(client, Lang_Get(Lang_KickGrp, 6));

	return 0;
}

cs_bool Client_ChangeWorld(Client *client, World *world) {
	if(Client_IsInWorld(client, world)) return false;
	PlayerData *pd = client->playerData;

	if(pd->state != STATE_INITIAL && pd->state != STATE_INGAME) {
		Client_Kick(client, Lang_Get(Lang_KickGrp, 6));
		return false;
	}

	Client_Despawn(client);
	pd->world = world;
	pd->state = STATE_MOTD;
	if(!world->loaded) World_Load(world);
	client->thread[1] = Thread_Create(WorldSendThread, client, false);
	return true;
}

void Client_UpdateWorldInfo(Client *client, World *world, cs_bool updateAll) {
	/*
	** Нет смысла пыжиться в попытках
	** поменять значения клиенту, если
	** он не поддерживает CPE вообще.
	*/
	if(!client->cpeData) return;
	WorldInfo *wi = &world->info;
	cs_byte modval = wi->modval,
	modclr = wi->modclr;
	cs_uint16 modprop = wi->modprop;

	if(updateAll || modval & MV_COLORS) {
		for(cs_byte color = 0; color < WORLD_COLORS_COUNT; color++) {
			if(updateAll || modclr & (2 ^ color))
				Client_SetEnvColor(client, color, World_GetEnvColor(world, color));
		}
	}
	if(updateAll || modval & MV_TEXPACK)
		Client_SetTexturePack(client, wi->texturepack);
	if(updateAll || modval & MV_PROPS) {
		for(cs_byte prop = 0; prop < WORLD_PROPS_COUNT; prop++) {
			if(updateAll || modprop & (2 ^ prop))
				Client_SetEnvProperty(client, prop, World_GetProperty(world, prop));
		}
	}
	if(updateAll || modval & MV_WEATHER)
		Client_SetWeather(client, wi->weatherType);
}

cs_bool Client_MakeSelection(Client *client, cs_byte id, SVec *start, SVec *end, Color4* color) {
	if(Client_GetExtVer(client, EXT_CUBOID)) {
		CPE_WriteMakeSelection(client, id, start, end, color);
		return true;
	}
	return false;
}

cs_bool Client_RemoveSelection(Client *client, cs_byte id) {
	if(Client_GetExtVer(client, EXT_CUBOID)) {
		CPE_WriteRemoveSelection(client, id);
		return true;
	}
	return false;
}

cs_bool Client_TeleportTo(Client *client, Vec *pos, Ang *ang) {
	if(Client_IsInGame(client)) {
		Vanilla_WriteTeleport(client, pos, ang);
		return true;
	}
	return false;
}

static cs_uint32 copyMessagePart(cs_str msg, cs_char *part, cs_uint32 i, cs_char *color) {
	if(*msg == '\0') return 0;

	if(i > 0) {
		*part++ = '>';
		*part++ = ' ';
	}

	if(*color > 0) {
		*part++ = '&';
		*part++ = *color;
	}

	cs_uint32 len = min(60, (cs_uint32)String_Length(msg));
	if(msg[len - 1] == '&' && ISHEX(msg[len])) --len;

	for(cs_uint32 j = 0; j < len; j++) {
		cs_char prevsym = *msg++,
		nextsym = *msg;

		if(prevsym != '\r') *part++ = prevsym;
		if(nextsym == '\0' || nextsym == '\n') break;
		if(prevsym == '&' && ISHEX(nextsym)) *color = nextsym;
	}

	*part = '\0';
	return len;
}

void Client_Chat(Client *client, cs_byte type, cs_str message) {
	cs_uint32 msgLen = (cs_uint32)String_Length(message);

	if(msgLen > 62 && type == MT_CHAT) {
		cs_char color = 0, part[65] = {0};
		cs_uint32 parts = (msgLen / 60) + 1;
		for(cs_uint32 i = 0; i < parts; i++) {
			cs_uint32 len = copyMessagePart(message, part, i, &color);
			if(len > 0) {
				Vanilla_WriteChat(client, type, part);
				message += len;
			}
		}
		return;
	}

	Vanilla_WriteChat(client, type, message);
}

static void HandlePacket(Client *client, cs_char *data, Packet *packet, cs_bool extended) {
	cs_bool ret = false;

	if(extended)
		if(packet->cpeHandler)
			ret = packet->cpeHandler(client, data);
		else
			ret = packet->handler(client, data);
	else
		if(packet->handler)
			ret = packet->handler(client, data);

	if(!ret) {
		Log_Error(Lang_Get(Lang_ErrGrp, 2), packet->id, client->id);
		Client_Kick(client, Lang_Get(Lang_KickGrp, 7));
	} else
		client->pps += 1;
}

static cs_uint16 GetPacketSizeFor(Packet *packet, Client *client, cs_bool *extended) {
	cs_uint16 packetSize = packet->size;
	if(packet->haveCPEImp) {
		*extended = Client_GetExtVer(client, packet->exthash) == packet->extVersion;
		if(*extended) packetSize = packet->extSize;
	}
	return packetSize;
}

void Client_Init(void) {
	Broadcast = Memory_Alloc(1, sizeof(Client));
	Broadcast->wrbuf = Memory_Alloc(2048, 1);
	Broadcast->mutex = Mutex_Create();
}

cs_bool Client_IsInGame(Client *client) {
	if(!client->playerData) return false;
	return client->playerData->state == STATE_INGAME;
}

cs_bool Client_IsInSameWorld(Client *client, Client *other) {
	return Client_GetWorld(client) == Client_GetWorld(other);
}

cs_bool Client_IsInWorld(Client *client, World *world) {
	return Client_GetWorld(client) == world;
}

cs_bool Client_IsOP(Client *client) {
	PlayerData *pd = client->playerData;
	return pd ? pd->isOP : false;
}

cs_bool Client_CheckAuth(Client *client) {
	return Heartbeat_CheckKey(client);
}

cs_bool Client_SetBlock(Client *client, SVec *pos, BlockID id) {
	if(client->playerData->state != STATE_INGAME) return false;
	Vanilla_WriteSetBlock(client, pos, id);
	return true;
}

cs_bool Client_SetEnvProperty(Client *client, cs_byte property, cs_int32 value) {
	if(Client_GetExtVer(client, EXT_MAPASPECT)) {
		CPE_WriteMapProperty(client, property, value);
		return true;
	}
	return false;
}

cs_bool Client_SetEnvColor(Client *client, cs_byte type, Color3* color) {
	if(Client_GetExtVer(client, EXT_MAPASPECT)) {
		CPE_WriteEnvColor(client, type, color);
		return true;
	}
	return false;
}

cs_bool Client_SetTexturePack(Client *client, cs_str url) {
	if(Client_GetExtVer(client, EXT_MAPASPECT)) {
		CPE_WriteTexturePack(client, url);
		return true;
	}
	return false;
}

cs_bool Client_SetWeather(Client *client, cs_int8 type) {
	if(Client_GetExtVer(client, EXT_WEATHER)) {
		CPE_WriteWeatherType(client, type);
		return true;
	}
	return false;
}

cs_bool Client_SetInvOrder(Client *client, Order order, BlockID block) {
	if(!Block_IsValid(block)) return false;

	if(Client_GetExtVer(client, EXT_INVORDER)) {
		CPE_WriteInventoryOrder(client, order, block);
		return true;
	}
	return false;
}

cs_bool Client_SetHeld(Client *client, BlockID block, cs_bool canChange) {
	if(!Block_IsValid(block)) return false;
	if(Client_GetExtVer(client, EXT_HELDBLOCK)) {
		CPE_WriteHoldThis(client, block, canChange);
		return true;
	}
	return false;
}

cs_bool Client_SetHotkey(Client *client, cs_str action, cs_int32 keycode, cs_int8 keymod) {
	if(Client_GetExtVer(client, EXT_TEXTHOTKEY)) {
		CPE_WriteSetHotKey(client, action, keycode, keymod);
		return true;
	}
	return false;
}

cs_bool Client_SetHotbar(Client *client, Order pos, BlockID block) {
	if(!Block_IsValid(block) || pos > 8) return false;
	if(Client_GetExtVer(client, EXT_SETHOTBAR)) {
		CPE_WriteSetHotBar(client, pos, block);
		return true;
	}
	return false;
}

cs_bool Client_SetBlockPerm(Client *client, BlockID block, cs_bool allowPlace, cs_bool allowDestroy) {
	if(!Block_IsValid(block)) return false;
	if(Client_GetExtVer(client, EXT_BLOCKPERM)) {
		CPE_WriteBlockPerm(client, block, allowPlace, allowDestroy);
		return true;
	}
	return false;
}

cs_bool Client_SetModel(Client *client, cs_int16 model) {
	CPEData *cpd = client->cpeData;
	if(!cpd) return false;
	if(!CPE_CheckModel(model)) return false;
	cpd->model = model;
	cpd->updates |= PCU_MODEL;
	return true;
}

cs_bool Client_SetSkin(Client *client, cs_str skin) {
	CPEData *cpd = client->cpeData;
	if(!cpd) return false;
	if(cpd->skin)
		Memory_Free((void *)cpd->skin);
	cpd->skin = String_AllocCopy(skin);
	cpd->updates |= PCU_SKIN;
	return true;
}

cs_bool Client_SetSpawn(Client *client, Vec *pos, Ang *ang) {
	if(Client_GetExtVer(client, EXT_SETSPAWN)) {
		CPE_WriteSetSpawnPoint(client, pos, ang);
		return true;
	}
	return false;
}

cs_bool Client_SetVelocity(Client *client, Vec *velocity, cs_bool mode) {
	if(Client_GetExtVer(client, EXT_VELCTRL)) {
		CPE_WriteVelocityControl(client, velocity, mode);
		return true;
	}
	return false;
}

cs_bool Client_SetRotation(Client *client, cs_byte axis, cs_int32 value) {
	if(axis > 2) return false;
	CPEData *cpd = client->cpeData;
	if(!cpd) return false;
	cpd->rotation[axis] = value;
	cpd->updates |= PCU_ENTPROP;
	return true;
}

cs_bool Client_SetModelStr(Client *client, cs_str model) {
	return Client_SetModel(client, CPE_GetModelNum(model));
}

cs_bool Client_SetGroup(Client *client, cs_int16 gid) {
	CPEData *cpd = client->cpeData;
	PlayerData *pd = client->playerData;
	if(!pd || !cpd)
		return false;
	cpd->group = gid;
	cpd->updates |= PCU_GROUP;
	return true;
}

cs_bool Client_SendHacks(Client *client, CPEHacks *hacks) {
	if(Client_GetExtVer(client, EXT_HACKCTRL)) {
		CPE_WriteHackControl(client, hacks);
		return true;
	}
	return false;
}

cs_bool Client_BulkBlockUpdate(Client *client, BulkBlockUpdate *bbu) {
	if(Client_GetExtVer(client, EXT_BULKUPDATE)) {
		CPE_WriteBulkBlockUpdate(client, bbu);
		return true;
	}
	return false;
}

cs_bool Client_DefineBlock(Client *client, BlockDef *block) {
	if(block->flags & BDF_UNDEFINED) return false;
	if(block->flags & BDF_EXTENDED) {
		if(Client_GetExtVer(client, EXT_BLOCKDEF2)) {
			CPE_WriteDefineExBlock(client, block);
			return true;
		}
	} else {
		if(Client_GetExtVer(client, EXT_BLOCKDEF)) {
			CPE_WriteDefineBlock(client, block);
			return true;
		}
	}

	return false;
}

cs_bool Client_UndefineBlock(Client *client, BlockID id) {
	if(Client_GetExtVer(client, EXT_BLOCKDEF)) {
		CPE_WriteUndefineBlock(client, id);
		return true;
	}
	return false;
}

cs_bool Client_AddTextColor(Client *client, Color4* color, cs_char code) {
	if(Client_GetExtVer(client, EXT_TEXTCOLORS)) {
		CPE_WriteSetTextColor(client, color, code);
		return true;
	}
	return false;
}

cs_bool Client_Update(Client *client) {
	CPEData *cpd = client->cpeData;
	if(!cpd) return false;
	cs_byte updates = cpd->updates;
	if(updates == PCU_NONE) return false;
	cpd->updates = PCU_NONE;

	for(ClientID id = 0; id < MAX_CLIENTS; id++) {
		Client *other = Clients_List[id];
		if(other) {
			if(updates & PCU_GROUP)
				CPE_WriteAddName(other, client);
			if(updates & PCU_MODEL)
				CPE_WriteSetModel(other, client);
			if(updates & PCU_SKIN)
				CPE_WriteAddEntity2(other, client);
			if(updates & PCU_ENTPROP)
				for(cs_int8 i = 0; i < 3; i++) {
					CPE_WriteSetEntityProperty(other, client, i, cpd->rotation[i]);
				}
		}
	}

	return true;
}

void Client_Free(Client *client) {
	if(client->thread[0])
		Thread_Join(client->thread[0]);

	if(client->id >= 0)
		Clients_List[client->id] = NULL;

	if(client->mutex) Mutex_Free(client->mutex);
	if(client->websock) Memory_Free(client->websock);
	if(client->rdbuf) Memory_Free(client->rdbuf);
	if(client->wrbuf) Memory_Free(client->wrbuf);

	PlayerData *pd = client->playerData;

	if(pd) {
		Memory_Free((void *)pd->name);
		Memory_Free((void *)pd->key);
		Memory_Free(pd);
	}

	CPEData *cpd = client->cpeData;

	if(cpd) {
		CPEExt *prev, *ptr = cpd->headExtension;

		while(ptr) {
			prev = ptr;
			ptr = ptr->next;
			Memory_Free((void *)prev->name);
			Memory_Free(prev);
		}

		if(cpd->message) Memory_Free(cpd->message);
		if(cpd->appName) Memory_Free((void *)cpd->appName);
		Memory_Free(cpd);
	}

	Socket_Shutdown(client->sock, SD_SEND);
	Socket_Close(client->sock);

	Memory_Free(client);
}

cs_int32 Client_Send(Client *client, cs_int32 len) {
	if(client->closed) return 0;
	if(client == Broadcast) {
		for(ClientID i = 0; i < MAX_CLIENTS; i++) {
			Client *bClient = Clients_List[i];

			if(bClient && !bClient->closed) {
				Mutex_Lock(bClient->mutex);
				if(bClient->websock)
					WebSock_SendFrame(bClient->websock, 0x02, client->wrbuf, (cs_uint16)len);
				else
					Socket_Send(client->sock, client->wrbuf, len);
				Mutex_Unlock(bClient->mutex);
			}
		}
		return len;
	}

	if(client->websock)
		return WebSock_SendFrame(client->websock, 0x02, client->wrbuf, (cs_uint16)len);
	else
		return Socket_Send(client->sock, client->wrbuf, len);
}

static void PacketReceiverWs(Client *client) {
	cs_byte packetId;
	Packet *packet;
	cs_bool extended;
	cs_uint16 packetSize, recvSize;
	WebSock *ws = client->websock;
	cs_char *data = client->rdbuf;

	if(WebSock_ReceiveFrame(ws)) {
		if(ws->opcode == 0x08) {
			client->closed = true;
			return;
		}

		recvSize = ws->plen - 1;
		packet_handle:
		packetId = *data++;
		packet = Packet_Get(packetId);
		if(!packet) {
			Log_Error(Lang_Get(Lang_ErrGrp, 2), packetId, client->id);
			Client_Kick(client, Lang_Get(Lang_KickGrp, 7));
			return;
		}

		packetSize = GetPacketSizeFor(packet, client, &extended);

		if(packetSize <= recvSize) {
			HandlePacket(client, data, packet, extended);
			/*
			** Каждую ~секунду к фрейму с пакетом 0x08 (Teleport)
			** приклеивается пакет 0x2B (TwoWayPing) и поскольку
			** не исключено, что таких приклеиваний может быть
			** много, пришлось использовать goto для обработки
			** всех пакетов, входящих в фрейм.
			*/
			if(recvSize > packetSize) {
				data += packetSize;
				recvSize -= packetSize + 1;
				goto packet_handle;
			}

			return;
		} else
			Client_Kick(client, Lang_Get(Lang_KickGrp, 7));
	} else
		client->closed = true;
}

static void PacketReceiverRaw(Client *client) {
	Packet *packet;
	cs_bool extended;
	cs_uint16 packetSize;
	cs_byte packetId;

	if(Socket_Receive(client->sock, (cs_char *)&packetId, 1, MSG_WAITALL) == 1) {
		packet = Packet_Get(packetId);
		if(!packet) {
			Log_Error(Lang_Get(Lang_ErrGrp, 2), packetId, client->id);
			Client_Kick(client, Lang_Get(Lang_KickGrp, 7));
			return;
		}

		packetSize = GetPacketSizeFor(packet, client, &extended);

		if(packetSize > 0) {
			cs_int32 len = Socket_Receive(client->sock, client->rdbuf, packetSize, MSG_WAITALL);

			if(packetSize == len)
				HandlePacket(client, client->rdbuf, packet, extended);
			else
				client->closed = true;
		}
	} else
		client->closed = true;
}

THREAD_FUNC(ClientThread) {
	Client *client = (Client *)param;

	while(!client->closed) {
		if(client->websock)
			PacketReceiverWs(client);
		else
			PacketReceiverRaw(client);

		if(client->thread[1]) {
			Thread_Join(client->thread[1]);
			client->thread[1] = NULL;
		}
	}

	return 0;
}

cs_bool Client_Add(Client *client) {
	cs_int8 maxplayers = Config_GetInt8ByKey(Server_Config, CFG_MAXPLAYERS_KEY);
	for(ClientID i = 0; i < min(maxplayers, MAX_CLIENTS); i++) {
		if(!Clients_List[i]) {
			client->id = i;
			client->thread[0] = Thread_Create(ClientThread, client, false);
			Clients_List[i] = client;
			return true;
		}
	}
	return false;
}

static void SendSpawnPacket(Client *client, Client *other) {
	if(Client_GetExtVer(client, EXT_PLAYERLIST))
		CPE_WriteAddEntity2(client, other);
	else
		Vanilla_WriteSpawn(client, other);
}

cs_bool Client_Spawn(Client *client) {
	PlayerData *pd = client->playerData;
	if(pd->spawned) return false;

	Client_UpdateWorldInfo(client, pd->world, true);

	for(ClientID i = 0; i < MAX_CLIENTS; i++) {
		Client *other = Clients_List[i];
		if(!other) continue;

		if(pd->firstSpawn) {
			if(Client_GetExtVer(other, EXT_PLAYERLIST))
				CPE_WriteAddName(other, client);
			if(Client_GetExtVer(client, EXT_PLAYERLIST) && client != other)
				CPE_WriteAddName(client, other);
		}

		if(Client_IsInSameWorld(client, other)) {
			SendSpawnPacket(other, client);

			if(Client_GetExtVer(other, EXT_CHANGEMODEL))
				CPE_WriteSetModel(other, client);

			if(client != other) {
				SendSpawnPacket(client, other);

				if(Client_GetExtVer(client, EXT_CHANGEMODEL))
					CPE_WriteSetModel(client, other);
			}
		}
	}

	pd->spawned = true;
	Event_Call(EVT_ONSPAWN, client);
	return true;
}

void Client_Kick(Client *client, cs_str reason) {
	if(client->closed) return;
	if(!reason) reason = Lang_Get(Lang_KickGrp, 0);
	Vanilla_WriteKick(client, reason);
	client->closed = true;

	/*
	** Этот вызов нужен, чтобы корректно завершить
	** сокет клиента после кика, если цикл сервера
	** в основом потоке уже не работает.
	*/
	if(!Server_Active) Client_Tick(client, 0);
}

void Client_Tick(Client *client, cs_int32 delta) {
	PlayerData *pd = client->playerData;
	if(client->closed) {
		if(pd && pd->state > STATE_WLOADDONE) {
			for(int i = 0; i < MAX_CLIENTS; i++) {
				Client *other = Clients_List[i];
				if(other && Client_GetExtVer(other, EXT_PLAYERLIST))
					CPE_WriteRemoveName(other, client);
			}
			Event_Call(EVT_ONDISCONNECT, client);
		}
		Client_Despawn(client);
		Client_Free(client);
		return;
	}

	client->ppstm += delta;
	if(client->ppstm > 1000) {
		if(client->pps > MAX_CLIENT_PPS) {
			Client_Kick(client, Lang_Get(Lang_KickGrp, 9));
			return;
		}
		client->pps = 0;
		client->ppstm = 0;
	}
}
