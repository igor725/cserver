#include "core.h"
#include "str.h"
#include "list.h"
#include "platform.h"
#include "block.h"
#include "growingbuffer.h"
#include "protocol.h"
#include "client.h"
#include "event.h"
#include "strstor.h"
#include "compr.h"
#include "world.h"
#include "websock.h"

Client *Broadcast = NULL;
Client *Clients_List[MAX_CLIENTS] = {0};
static AListField *headCGroup = NULL;

CGroup *Group_Add(cs_int16 gid, cs_str gname, cs_byte grank) {
	CGroup *gptr = Group_GetByID(gid);
	if(!gptr) {
		gptr = Memory_Alloc(1, sizeof(CGroup));
		gptr->id = gid;
		gptr->field = AList_AddField(&headCGroup, gptr);
	}

	String_Copy(gptr->name, 65, gname);
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

	AList_Remove(&headCGroup, cg->field);
	Memory_Free(cg);
	return true;
}

static void CubeNormalize(SVec *s, SVec *e) {
	cs_int16 tmp, *a = (cs_int16 *)s, *b = (cs_int16 *)e;
	for(int i = 0; i < 3; i++) {
		if(a[i] < b[i]) {
			tmp = a[i];
			a[i] = b[i];
			b[i] = tmp;
		}
		a[i]++;
	}
}

void Cuboid_SetPositions(CPECuboid *cub, SVec start, SVec end) {
	cub->pos[0] = start, cub->pos[1] = end;
	CubeNormalize(&cub->pos[0], &cub->pos[1]);
}

void Cuboid_SetColor(CPECuboid *cub, Color4 color) {
	cub->color = color;
}

cs_uint32 Cuboid_GetSize(CPECuboid *cub) {
	return (cub->pos[0].x - cub->pos[1].x) *
	(cub->pos[0].y - cub->pos[1].y) *
	(cub->pos[0].z - cub->pos[1].z);
}

void Cuboid_GetPositions(CPECuboid *cub, SVec *start, SVec *end) {
	if(start) *start = cub->pos[0];
	if(end) *end = cub->pos[1];
}

cs_byte Clients_GetCount(EPlayerState state) {
	cs_byte count = 0;
	for(ClientID i = 0; i < MAX_CLIENTS; i++) {
		Client *client = Clients_List[i];
		if(client) {
			if(!Client_CheckState(client, state)) continue;
			if(Client_IsBot(client)) continue;
			count++;
		}
	}
	return count;
}

static ClientID FindFreeID(void) {
	for(ClientID i = 0; i < MAX_CLIENTS; i++)
		if(!Clients_List[i]) return i;

	return CLIENT_SELF;
}

Client *Client_NewBot(void) {
	ClientID botid = FindFreeID();
	if(botid == CLIENT_SELF)
		return NULL;
	
	Client *client = Memory_Alloc(1, sizeof(Client));
	client->playerData = Memory_Alloc(1, sizeof(PlayerData));
	client->cpeData = Memory_Alloc(1, sizeof(CPEData));
	client->playerData->state = PLAYER_STATE_INGAME;
	client->waitend = Waitable_Create();
	Waitable_Signal(client->waitend);
	client->mutex = Mutex_Create();
	client->sock = INVALID_SOCKET;
	client->cpeData->model = 256;
	client->id = botid;

	Clients_List[botid] = client;
	return client;
}

cs_bool Client_IsBot(Client *bot) {
	return bot->sock == INVALID_SOCKET;
}

void Client_Lock(Client *client) {
	Mutex_Lock(client->mutex);
}

void Client_Unlock(Client *client) {
	Mutex_Unlock(client->mutex);
}

cs_str Client_GetName(Client *client) {
	if(!client->playerData) return Sstor_Get("NONAME");
	return client->playerData->name;
}

cs_str Client_GetDisplayName(Client *client) {
	if(!client->playerData) return Sstor_Get("NONAME");
	return client->playerData->displayname;
}

cs_str Client_GetKey(Client *client) {
	if(!client->playerData) return Sstor_Get("CL_NOKEY");
	return client->playerData->key;
}

cs_str Client_GetAppName(Client *client) {
	if(!client->cpeData) return Sstor_Get("CL_VANILLA");
	return client->cpeData->appName;
}

cs_str Client_GetSkin(Client *client) {
	if(!client->cpeData || *client->cpeData->skin == '\0')
		return Client_GetName(client);
	return client->cpeData->skin;
}

Client *Client_GetByName(cs_str name) {
	for(ClientID i = 0; i < MAX_CLIENTS; i++) {
		Client *client = Clients_List[i];
		if(client && client->playerData && String_CaselessCompare(client->playerData->name, name))
			return client;
	}
	return NULL;
}

Client *Client_GetByID(ClientID id) {
	return id >= 0 && id < MAX_CLIENTS ? Clients_List[id] : NULL;
}

ClientID Client_GetID(Client *client) {
	return client->id;
}

World *Client_GetWorld(Client *client) {
	if(!client->playerData) return NULL;
	return client->playerData->world;
}

BlockID Client_GetStandBlock(Client *client) {
	if(!client->playerData) return 0;
	SVec tpos; SVec_Copy(tpos, client->playerData->position);
	if(tpos.x < 0 || tpos.y < 0 || tpos.z < 0) return 0;
	tpos.y -= 2;

	return World_GetBlock(client->playerData->world, &tpos);
}

cs_int8 Client_GetFluidLevel(Client *client, BlockID *fluid) {
	if(!client->playerData) return 0;
	SVec tpos; SVec_Copy(tpos, client->playerData->position);
	if(tpos.x < 0 || tpos.y < 0 || tpos.z < 0) return 0;

	BlockID id;
	cs_byte level = 2;

	test_wtrlevel:
	if((id = World_GetBlock(client->playerData->world, &tpos)) > 7 && id < 12) {
		if(fluid) *fluid = id;
		return level;
	}

	if(level > 1) {
		level--;
		tpos.y--;
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

cs_uint16 Client_GetClickDistance(Client *client) {
	if(!client->cpeData) return 160;
	return client->cpeData->clickDist;
}

cs_float Client_GetClickDistanceInBlocks(Client *client) {
	if(!client->cpeData) return 5.0f;
	return client->cpeData->clickDist / 32.0f;
}

cs_int32 Client_GetExtVer(Client *client, cs_uint32 exthash) {
	if(Client_IsBot(client)) return true;
	if(!client->cpeData) return false;

	CPEExt *ptr = client->cpeData->headExtension;
	while(ptr) {
		if(ptr->hash == exthash) return ptr->version;
		ptr = ptr->next;
	}

	return false;
}

cs_bool Client_Despawn(Client *client) {
	if(!client->playerData || !client->playerData->spawned)
		return false;
	client->playerData->spawned = false;
	for(cs_uint32 i = 0; i < MAX_CLIENTS; i++) {
		Client *other = Clients_List[i];
		if(other) Vanilla_WriteDespawn(other, client);
	}
	Event_Call(EVT_ONDESPAWN, client);
	return true;
}

cs_bool Client_ChangeWorld(Client *client, World *world) {
	if(Client_IsBot(client)) {
		Client_Despawn(client);
		client->playerData->world = world;
		Client_Spawn(client);
		return true;
	}
	if(!world || Client_CheckState(client, PLAYER_STATE_MOTD))
		return false;

	Client_Despawn(client);
	client->playerData->reqWorldChange = world;
	return true;
}

void Client_UpdateWorldInfo(Client *client, World *world, cs_bool updateAll) {
	if(!client->cpeData) return;

	if(Client_GetExtVer(client, EXT_MAPASPECT)) {
		if(updateAll || world->info.modval & MV_COLORS) {
			for(cs_byte color = 0; color < WORLD_COLORS_COUNT; color++) {
				if(updateAll || world->info.modclr & (1 << color))
					CPE_WriteEnvColor(client, color, World_GetEnvColor(world, color));
			}
		}
		if(updateAll || world->info.modval & MV_TEXPACK)
			CPE_WriteTexturePack(client, world->info.texturepack);
		if(updateAll || world->info.modval & MV_PROPS) {
			for(cs_byte prop = 0; prop < WORLD_PROPS_COUNT; prop++) {
				if(updateAll || world->info.modprop & (1 << prop))
					CPE_WriteMapProperty(client, prop, World_GetEnvProp(world, prop));
			}
		}
	} else {
		switch(Client_GetExtVer(client, EXT_MAPPROPS)) {
			case 1:
				CPE_WriteSetMapAppearanceV1(
					client, world->info.texturepack,
					(cs_byte)World_GetEnvProp(world, 0),
					(cs_byte)World_GetEnvProp(world, 1),
					(cs_int16)World_GetEnvProp(world, 2)
				);
				break;

			case 2:
				CPE_WriteSetMapAppearanceV2(
					client, world->info.texturepack,
					(cs_byte)World_GetEnvProp(world, 0),
					(cs_byte)World_GetEnvProp(world, 1),
					(cs_int16)World_GetEnvProp(world, 2),
					(cs_int16)World_GetEnvProp(world, 3),
					(cs_int16)World_GetEnvProp(world, 4)
				);
				break;
		}
	}

	if(updateAll || world->info.modval & MV_WEATHER)
		Client_SetWeather(client, world->info.weatherType);
}

CPECuboid *Client_NewSelection(Client *client) {
	if(Client_GetExtVer(client, EXT_CUBOID)) {
		for(cs_byte i = 0; i < 16; i++) {
			CPECuboid *cub = &client->cpeData->cuboids[i];
			if(cub->used) continue;
			cub->used = true;
			cub->id = i;
			return cub;
		}
	}

	return NULL;
}

cs_bool Client_UpdateSelection(Client *client, CPECuboid *cub) {
	if(Client_GetExtVer(client, EXT_CUBOID)) {
		CPE_WriteMakeSelection(client, cub);
		return true;
	}

	return false;
}

cs_bool Client_RemoveSelection(Client *client, CPECuboid *cub) {
	if(Client_GetExtVer(client, EXT_CUBOID)) {
		CPE_WriteRemoveSelection(client, cub->id);
		cub->used = false;
		return true;
	}
	return false;
}

cs_bool Client_TeleportTo(Client *client, Vec *pos, Ang *ang) {
	if(Client_CheckState(client, PLAYER_STATE_INGAME)) {
		Vanilla_WriteTeleport(client, pos, ang);
		client->playerData->position = *pos;
		client->playerData->angle = *ang;
		if(Client_IsBot(client)) {
			for(ClientID i = 0; i < MAX_CLIENTS; i++) {
				Client *other = Clients_List[i];
				if(!other || Client_IsBot(other)) continue;
				if(!Client_CheckState(other, PLAYER_STATE_INGAME)) continue;
				if(!Client_IsInSameWorld(other, client)) continue;
				Vanilla_WritePosAndOrient(other, client);
			}
		}
		return true;
	}
	return false;
}

cs_bool Client_TeleportToSpawn(Client *client) {
	if(!client->playerData || !client->playerData->world) return false;
	return Client_TeleportTo(client,
		&client->playerData->world->info.spawnVec,
		&client->playerData->world->info.spawnAng
	);
}

INL static cs_uint32 CopyMessagePart(cs_str msg, cs_char *part, cs_uint32 i, cs_char *color) {
	if(*msg == '\0') return 0;
	cs_uint32 maxlen = 64;

	if(i > 0) {
		*part++ = '>';
		*part++ = ' ';
		maxlen -= 2;
	}

	if(*color > 0) {
		*part++ = '&';
		*part++ = *color;
		maxlen -= 2;
	}

	cs_uint32 len = min(maxlen, (cs_uint32)String_Length(msg));
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

void Client_Chat(Client *client, EMesgType type, cs_str message) {
	cs_uint32 msgLen = (cs_uint32)String_Length(message);

	if(msgLen > 62 && type == MESSAGE_TYPE_CHAT) {
		cs_char color = 0, part[65] = {0};
		cs_uint32 parts = (msgLen / 60) + 1;
		for(cs_uint32 i = 0; i < parts; i++) {
			cs_uint32 len = CopyMessagePart(message, part, i, &color);
			if(len > 0) {
				Vanilla_WriteChat(client, type, part);
				message += len;
			}
		}
		return;
	}

	Vanilla_WriteChat(client, type, message);
}

NOINL static void HandlePacket(Client *client, cs_char *data, Packet *packet, cs_bool extended) {
	cs_bool ret = false;

	if(extended)
		if(packet->extHandler)
			ret = packet->extHandler(client, data);
		else
			ret = packet->handler(client, data);
	else
		if(packet->handler)
			ret = packet->handler(client, data);

	if(!ret)
		Client_KickFormat(client, Sstor_Get("KICK_PERR_UNEXP"), packet->id);
	else client->pps += 1;
}

INL static cs_uint16 GetPacketSizeFor(Packet *packet, Client *client, cs_bool *extended) {
	if(packet->haveCPEImp) {
		*extended = Client_GetExtVer(client, packet->exthash) == packet->extVersion;
		if(*extended) return packet->extSize;
	}
	return packet->size;
}

cs_bool Client_CheckState(Client *client, EPlayerState state) {
	if(!client->playerData) return state == PLAYER_STATE_INITIAL;
	return client->playerData->state == state;
}

cs_bool Client_IsClosed(Client *client) {
	return client->closed;
}

static const struct _subnet {
	cs_ulong net;
	cs_ulong mask;
} localnets[] = {
	{0x0000007f, 0x000000FF},
	{0x0000000A, 0x000000FF},
	{0x000010AC, 0x00000FFF},
	{0x0000A8C0, 0x0000FFFF},
	
	{0x00000000, 0x00000000}
};

cs_bool Client_IsLocal(Client *client) {
	if(Client_IsBot(client)) return true;

	for(const struct _subnet *s = localnets; s->mask; s++) {
		if((client->addr & s->mask) == s->net)
			return true;
	}

	return false;
}

cs_bool Client_IsInSameWorld(Client *client, Client *other) {
	return Client_GetWorld(client) == Client_GetWorld(other);
}

cs_bool Client_IsInWorld(Client *client, World *world) {
	return Client_GetWorld(client) == world;
}

cs_bool Client_IsOP(Client *client) {
	return client->playerData ? client->playerData->isOP : false;
}

cs_bool Client_IsFirstSpawn(Client *client) {
	return client->playerData ? client->playerData->firstSpawn : true;
}

void Client_SetBlock(Client *client, SVec *pos, BlockID id) {
	if(Client_GetExtVer(client, EXT_CUSTOMBLOCKS) < 1 ||
	(Client_GetExtVer(client, EXT_BLOCKDEF) || Client_GetExtVer(client, EXT_BLOCKDEF2)) < 1)
		id = Block_GetFallbackFor(Client_GetWorld(client), id);

	Vanilla_WriteSetBlock(client, pos, id);
}

cs_bool Client_SetEnvProperty(Client *client, cs_byte property, cs_int32 value) {
	if(Client_GetExtVer(client, EXT_MAPASPECT)) {
		CPE_WriteMapProperty(client, property, value);
		return true;
	}
	return false;
}

cs_bool Client_SetEnvColor(Client *client, cs_byte type, Color3* color) {
	if(Client_GetExtVer(client, EXT_ENVCOLOR)) {
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

cs_bool Client_SetDisplayName(Client *client, cs_str name) {
	if(!client->playerData) return false;
	if(String_Copy(client->playerData->displayname, 65, name))
		client->cpeData->updates |= PCU_NAME;
	return false;
}

cs_bool Client_SetWeather(Client *client, cs_int8 type) {
	if(Client_GetExtVer(client, EXT_WEATHER)) {
		CPE_WriteWeatherType(client, type);
		return true;
	}
	return false;
}

cs_bool Client_SetInvOrder(Client *client, cs_byte order, BlockID block) {
	if(Client_GetExtVer(client, EXT_INVORDER)) {
		CPE_WriteInventoryOrder(client, order, block);
		return true;
	}
	return false;
}

cs_bool Client_SetServerIdent(Client *client, cs_str name, cs_str motd) {
	if(Client_CheckState(client, PLAYER_STATE_INGAME) && !Client_GetExtVer(client, EXT_INSTANTMOTD))
		return false;
	Vanilla_WriteServerIdent(client, name, motd);
	return true;
}

cs_bool Client_SetHeldBlock(Client *client, BlockID block, cs_bool preventChange) {
	if(Client_GetExtVer(client, EXT_HELDBLOCK)) {
		CPE_WriteHoldThis(client, block, preventChange);
		return true;
	}
	return false;
}

cs_bool Client_SetClickDistance(Client *client, cs_uint16 dist) {
	if(Client_GetExtVer(client, EXT_CLICKDIST)) {
		CPE_WriteClickDistance(client, dist);
		client->cpeData->clickDist = dist;
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

cs_bool Client_SetHotbar(Client *client, cs_byte pos, BlockID block) {
	if(pos < 9 && Client_GetExtVer(client, EXT_SETHOTBAR)) {
		CPE_WriteSetHotBar(client, pos, block);
		return true;
	}
	return false;
}

cs_bool Client_SetBlockPerm(Client *client, BlockID block, cs_bool allowPlace, cs_bool allowDestroy) {
	if(Client_GetExtVer(client, EXT_BLOCKPERM)) {
		CPE_WriteBlockPerm(client, block, allowPlace, allowDestroy);
		return true;
	}
	return false;
}

cs_bool Client_SetModel(Client *client, cs_int16 model) {
	if(!client->cpeData || !CPE_CheckModel(client, model)) return false;
	client->cpeData->model = model;
	client->cpeData->updates |= PCU_MODEL;
	return true;
}

cs_bool Client_SetSkin(Client *client, cs_str skin) {
	if(!client->cpeData) return false;
	String_Copy(client->cpeData->skin, 65, skin);
	client->cpeData->updates |= PCU_ENTITY;
	return true;
}

GrowingBuffer *Client_GetBuffer(Client *client) {
	return &client->gb;
}

cs_uint32 Client_GetAddr(Client *client) {
	return client->addr;
}

cs_int32 Client_GetPing(Client *client) {
	if(Client_GetExtVer(client, EXT_TWOWAYPING)) {
		return client->cpeData->pingTime;
	}
	return -1;
}

cs_float Client_GetAvgPing(Client *client) {
	if(Client_GetExtVer(client, EXT_TWOWAYPING)) {
		return client->cpeData->pingAvgTime;
	}
	return -1.0f;
}

cs_bool Client_GetPosition(Client *client, Vec *pos, Ang *ang) {
	if(client->playerData) {
		if(pos) *pos = client->playerData->position;
		if(ang) *ang = client->playerData->angle;
		return true;
	}
	return false;
}

cs_bool Client_SetOP(Client *client, cs_bool state) {
	if(!client->playerData) return false;
	client->playerData->isOP = state;
	Event_Call(EVT_ONUSERTYPECHANGE, client);
	// if(Client_CheckState(client, PLAYER_STATE_INGAME))
	// 	Vanilla_WriteUserType(client, state ? 0x64 : 0x00);
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

cs_bool Client_RegisterParticle(Client *client, CustomParticle *e) {
	if(Client_GetExtVer(client, EXT_PARTICLE)) {
		CPE_WriteDefineEffect(client, e);
		return true;
	}
	return false;
}

cs_bool Client_SpawnParticle(Client *client, cs_byte id, Vec *pos, Vec *origin) {
	if(Client_GetExtVer(client, EXT_PARTICLE)) {
		CPE_WriteSpawnEffect(client, id, pos, origin);
		return true;
	}
	return false;
}

cs_bool Client_SendPluginMessage(Client *client, cs_byte channel, cs_str message) {
	if(Client_GetExtVer(client, EXT_PLUGINMESSAGE)) {
		CPE_WritePluginMessage(client, channel, message);
		return true;
	}
	return false;
}

cs_bool Client_SetRotation(Client *client, cs_byte axis, cs_int32 value) {
	if(axis < 2 || !client->cpeData) return false;
	client->cpeData->rotation[axis] = value;
	client->cpeData->updates |= PCU_ENTPROP;
	return true;
}

cs_bool Client_SetModelStr(Client *client, cs_str model) {
	return Client_SetModel(client, CPE_GetModelNum(model));
}

cs_bool Client_SetGroup(Client *client, cs_int16 gid) {
	if(!client->cpeData) return false;
	client->cpeData->group = gid;
	client->cpeData->updates |= PCU_NAME;
	return true;
}

cs_bool Client_SendHacks(Client *client, CPEHacks *hacks) {
	if(Client_GetExtVer(client, EXT_HACKCTRL)) {
		CPE_WriteHackControl(client, hacks);
		return true;
	}
	return false;
}

void Client_BulkBlockUpdate(Client *client, BulkBlockUpdate *bbu) {
	if(Client_GetExtVer(client, EXT_BULKUPDATE)) {
		CPE_WriteBulkBlockUpdate(client, bbu);
	} else {
		World *world = Client_GetWorld(client);
		SVec d = {0, 0, 0}, p = {0, 0, 0};
		World_GetDimensions(world, &d);
		cs_uint32 *offsets = (cs_uint32 *)bbu->data.offsets;
		for(cs_uint32 i = 0; i < bbu->data.count; i++) {
			cs_uint32 offset = ntohl(offsets[i]);
			p.x = (cs_int16)(offset % (cs_uint32)d.x);
			p.y = (cs_int16)((offset / (cs_uint32)d.x) / (cs_uint32)d.z);
			p.z = (cs_int16)((offset / (cs_uint32)d.x) % (cs_uint32)d.z);
			Vanilla_WriteSetBlock(client, &p, bbu->data.ids[i]);
		}
	}
}

cs_bool Client_DefineBlock(Client *client, BlockID id, BlockDef *block) {
	if(block->flags & BDF_UNDEFINED) return false;
	if(block->flags & BDF_EXTENDED) {
		if(Client_GetExtVer(client, EXT_BLOCKDEF2)) {
			CPE_WriteDefineExBlock(client, id, block);
			return true;
		}
	} else {
		if(Client_GetExtVer(client, EXT_BLOCKDEF)) {
			CPE_WriteDefineBlock(client, id, block);
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
		CPE_WriteAddTextColor(client, color, code);
		return true;
	}
	return false;
}

cs_bool Client_Update(Client *client) {
	if(!client->cpeData || client->cpeData->updates == PCU_NONE) return false;

	for(ClientID id = 0; id < MAX_CLIENTS; id++) {
		Client *other = Clients_List[id];
		if(other) {
			cs_bool hasplsupport = Client_GetExtVer(other, EXT_PLAYERLIST) == 2,
			hassmsupport = Client_GetExtVer(other, EXT_CHANGEMODEL) == 1,
			hasentprop = Client_GetExtVer(other, EXT_ENTPROP) == 1,
			isinsameworld = Client_IsInSameWorld(client, other);
			if(client->cpeData->updates & PCU_NAME && hasplsupport) {
				client->cpeData->updates |= PCU_ENTITY;
				CPE_WriteAddName(other, client);
			}
			if(isinsameworld) {
				if(client->cpeData->updates & PCU_MODEL && hassmsupport)
					CPE_WriteSetModel(other, client);
				if(client->cpeData->updates & PCU_ENTITY && hasplsupport)
					CPE_WriteAddEntity2(other, client);
				if(client->cpeData->updates & PCU_ENTPROP && hasentprop)
					for(cs_int8 i = 0; i < 3; i++) {
						CPE_WriteSetEntityProperty(other, client, i, client->cpeData->rotation[i]);
					}
			}
		}
	}

	client->cpeData->updates = PCU_NONE;
	return true;
}

void Client_Free(Client *client) {
	if(client->waitend) {
		Waitable_Free(client->waitend);
		client->waitend = NULL;
	}
	if(client->mutex) {
		Mutex_Free(client->mutex);
		client->mutex = NULL;
	}
	if(client->websock) {
		Memory_Free(client->websock);
		client->websock = NULL;
	}
	if(client->playerData) {
		Memory_Free(client->playerData);
		client->playerData = NULL;
	}
	if(client->kickReason) {
		Memory_Free((void *)client->kickReason);
		client->kickReason = false;
	}

	if(client->cpeData) {
		CPEExt *prev, *ptr = client->cpeData->headExtension;

		while(ptr) {
			prev = ptr;
			ptr = ptr->next;
			Memory_Free((void *)prev->name);
			Memory_Free(prev);
		}

		if(client->cpeData->message) {
			Memory_Free(client->cpeData->message);
			client->cpeData->message = NULL;
		}

		Memory_Free(client->cpeData);
		client->cpeData = NULL;
	}

	while(client->headNode) {
		Memory_Free(client->headNode->value.ptr);
		KList_Remove(&client->headNode, client->headNode);
	}

	GrowingBuffer_Cleanup(&client->gb);
	Compr_Cleanup(&client->compr);
	Memory_Free(client);
}

cs_bool Client_RawSend(Client *client, cs_char *buf, cs_int32 len) {
	if(client->sock == (Socket)-1 || client->closed || len < 1) return false;

	if(client == Broadcast) {
		for(ClientID i = 0; i < MAX_CLIENTS; i++) {
			Client *bClient = Clients_List[i];

			if(bClient && !bClient->closed) {
				Mutex_Lock(bClient->mutex);
				Client_RawSend(bClient, buf, len);
				Mutex_Unlock(bClient->mutex);
			}
		}

		return true;
	}

	if(client->websock) {
		if(!WebSock_SendFrame(client->websock, 0x02, buf, (cs_uint16)len)) {
			client->closed = true;
			return false;
		}
	} else {
		if(Socket_Send(client->sock, buf, len) != len) {
			client->closed = true;
			return false;
		}
	}

	return true;
}

void Client_SetNoFlush(Client *client, cs_bool state) {
	client->noflush = state;
	if(!state) {
		Mutex_Lock(client->mutex);
		Client_FlushBuffer(client);
		Mutex_Unlock(client->mutex);
	}
}

cs_bool Client_SendAnytimeData(Client *client, cs_int32 size) {
	cs_char *data = GrowingBuffer_GetCurrentPoint(&client->gb);
	return Client_RawSend(client, data, size);
}

cs_bool Client_FlushBuffer(Client *client) {
	if(client->noflush) return 0;
	cs_uint32 size = 0;
	cs_char *data = GrowingBuffer_PopFullData(&client->gb, &size);
	return Client_RawSend(client, data, size);
}

INL static void PacketReceiverWs(Client *client) {
	cs_byte packetId;
	Packet *packet;
	cs_bool extended = false;
	cs_uint16 packetSize, recvSize;
	cs_char *data = client->rdbuf;

	if(WebSock_ReceiveFrame(client->websock)) {
		if(client->websock->opcode == 0x08) {
			client->closed = true;
			return;
		}

		recvSize = client->websock->plen - 1;
		packet_handle:
		packetId = *data++;
		packet = Packet_Get(packetId);
		if(!packet) {
			Client_KickFormat(client, Sstor_Get("KICK_PERR_NOHANDLER"), packetId);
			return;
		}

		packetSize = GetPacketSizeFor(packet, client, &extended);

		if(packetSize <= recvSize) {
			HandlePacket(client, data, packet, extended);
			if(recvSize > packetSize && !client->closed) {
				data += packetSize;
				recvSize -= packetSize + 1;
				goto packet_handle;
			}

			return;
		} else Client_Kick(client, Sstor_Get("KICK_PERR_WS"));
	} else if(client->websock->error != WS_ERROR_CONTINUE) {
		Client_KickFormat(client, WebSock_GetError(client->websock));
		client->closed = true;
	}
}

INL static void PacketReceiverRaw(Client *client) {
	Packet *packet;
	cs_uint16 packetSize;
	cs_byte packetId = 0xFF;
	cs_bool extended = false;
	cs_error ecode = 0;
	cs_int32 len = 0;

	if((len = Socket_Receive(client->sock, (cs_char *)&packetId, 1, 0)) > 0) {
		packet = Packet_Get(packetId);
		if(!packet) {
			Client_KickFormat(client, Sstor_Get("KICK_PERR_NOHANDLER"), packetId);
			return;
		}

		packetSize = GetPacketSizeFor(packet, client, &extended);

		if(packetSize > 0) {
			cs_uint32 offset = 0;
			while(!client->closed) {
				len = Socket_Receive(client->sock, client->rdbuf + offset, packetSize - offset, 0);

				if(len > 0) {
					offset += len;

					if(offset == packetSize) {
						HandlePacket(client, client->rdbuf, packet, extended);
						break;
					}
				} else if(len < 0 && ((ecode = Socket_GetError()) != EAGAIN && ecode > 0)) {
					Client_KickFormat(client, Sstor_Get("KICK_NERR"), ecode);
				} else if(len == 0) client->closed = true;
			}
		}
	} else if(len < 0 && (ecode = Socket_GetError()) != EAGAIN && ecode > 0) {
		Client_KickFormat(client, Sstor_Get("KICK_NERR"), ecode);
	} else if(len == 0) client->closed = true;
}

NOINL static void SendWorld(Client *client, World *world) {
	if(!World_IsReadyToPlay(world)) {
		if(!World_Load(world)) {
			Client_Kick(client, Sstor_Get("KICK_INT"));
			return;
		}
	}

	World_StartTask(world);
	cs_bool hasfastmap = Client_GetExtVer(client, EXT_FASTMAP) == 1;
	cs_bool isfallbackneeded = Client_GetExtVer(client, EXT_BLOCKDEF) < 1 ||
	Client_GetExtVer(client, EXT_BLOCKDEF2) < 1 ||
	Client_GetExtVer(client, EXT_CUSTOMBLOCKS) < 1;
	cs_byte *map = NULL;
	cs_uint32 wsize = 0;
	cs_bool compr_ok;

	if(hasfastmap) {
		compr_ok = Compr_Init(&client->compr, COMPR_TYPE_DEFLATE);
		if(compr_ok) {
			map = World_GetBlockArray(world, &wsize);
			CPE_WriteFastMapInit(client, wsize);
		}
	} else {
		compr_ok = Compr_Init(&client->compr, COMPR_TYPE_GZIP);
		if(compr_ok) {
			map = World_GetData(world, &wsize);
			Vanilla_WriteLvlInit(client);
		}
	}

	if(compr_ok) {
		cs_byte indata[10240] = {0}, data[1029] = {0x03};
		cs_uint16 *len = (cs_uint16 *)(data + 1);
		cs_byte *cmpdata = data + 3;
		cs_byte *progr = data + 1028;
		cs_uint32 sent = 0;

		if(Client_GetExtVer(client, EXT_BLOCKDEF)) {
			World *oldworld = client->playerData->world;
			for(cs_uint16 id = 0; id < 256; id++) {
				BlockDef *newbdef = Block_GetDefinition(world, (BlockID)id);
				if(oldworld) {
					BlockDef *unbdef = Block_GetDefinition(oldworld, (BlockID)id);
					if(unbdef) Client_UndefineBlock(client, (BlockID)id);
				}
				if(newbdef) Client_DefineBlock(client, (BlockID)id, newbdef);
			}
		}

		client->playerData->world = world;
		client->playerData->state = PLAYER_STATE_MOTD;
		client->playerData->position = world->info.spawnVec;
		client->playerData->angle = world->info.spawnAng;

		while(compr_ok && !client->closed && !Compr_IsInState(&client->compr, COMPR_STATE_DONE)) {
			cs_uint32 avail = min(wsize - sent, 10240);
			if(avail > 0) {
				Memory_Copy(indata, map + sent, avail);
				for(cs_uint32 i = 0; isfallbackneeded && i < avail; i++)
					indata[i] = Block_GetFallbackFor(world, indata[i]);
				Compr_SetInBuffer(&client->compr, indata, avail);
				sent += avail;
			}

			do {
				Mutex_Lock(client->mutex);
				Compr_SetOutBuffer(&client->compr, cmpdata, 1024);
				if((compr_ok = Compr_Update(&client->compr)) == true) {
					if(!client->closed && client->compr.written > 0) {
						*len = htons((cs_uint16)client->compr.written);
						*progr = (cs_byte)(((cs_float)sent / wsize) * 100);
						compr_ok = Client_RawSend(client, (cs_char *)data, 1028);
					}
				}
				Mutex_Unlock(client->mutex);
			} while(compr_ok && Compr_IsInState(&client->compr, COMPR_STATE_INPROCESS));
		}

		if(compr_ok) {
			Vanilla_WriteLvlFin(client, &world->info.dimensions);
			Mutex_Lock(client->mutex);
			client->playerData->state = PLAYER_STATE_INGAME;
			Client_FlushBuffer(client);
			Mutex_Unlock(client->mutex);
			Client_Spawn(client);
		} else
			Client_KickFormat(client, Sstor_Get("KICK_ZERR"), Compr_GetLastError(&client->compr));

		Compr_Reset(&client->compr);
	} else
		Client_KickFormat(client, Sstor_Get("KICK_ZERR"), Compr_GetLastError(&client->compr));

	World_EndTask(world);
}

void Client_Tick(Client *client) {
	if(client->websock)
		PacketReceiverWs(client);
	else
		PacketReceiverRaw(client);

	if(client->playerData && client->playerData->reqWorldChange) {
		SendWorld(client, client->playerData->reqWorldChange);
		client->playerData->reqWorldChange = NULL;
	}
}

INL static void SendSpawnPacket(Client *client, Client *other) {
	if(Client_GetExtVer(client, EXT_PLAYERLIST))
		CPE_WriteAddEntity2(client, other);
	else
		Vanilla_WriteSpawn(client, other);
}

cs_bool Client_Spawn(Client *client) {
	if(client->closed || !client->playerData || client->playerData->spawned)
		return false;

	onSpawn evt = {
		.client = client,
		.position = &client->playerData->position,
		.angle = &client->playerData->angle
	};
	Event_Call(EVT_ONSPAWN, &evt);
	// Vanilla_WriteUserType(client, client->playerData->isOP ? 0x64 : 0x00);
	Client_UpdateWorldInfo(client, client->playerData->world, true);
	if(client->cpeData) client->cpeData->updates = PCU_NONE;

	for(ClientID i = 0; i < MAX_CLIENTS; i++) {
		Client *other = Clients_List[i];
		if(!other || !Client_CheckState(other, PLAYER_STATE_INGAME)) continue;

		if(client->playerData->firstSpawn) {
			if(Client_GetExtVer(other, EXT_PLAYERLIST))
				CPE_WriteAddName(other, client);
			if(Client_GetExtVer(client, EXT_PLAYERLIST) && client != other)
				CPE_WriteAddName(client, other);
		}

		if(Client_IsInSameWorld(client, other)) {
			SendSpawnPacket(other, client);

			if(Client_GetExtVer(other, EXT_CHANGEMODEL))
				CPE_WriteSetModel(other, client);

			if(client != other && !Client_IsBot(client)) {
				SendSpawnPacket(client, other);

				if(Client_GetExtVer(client, EXT_CHANGEMODEL))
					CPE_WriteSetModel(client, other);
			}
		}
	}

	client->playerData->spawned = true;
	client->playerData->firstSpawn = false;
	return true;
}

cs_str Client_GetDisconnectReason(Client *client) {
	return client->kickReason ? client->kickReason : "Disconnect";
}

void Client_Kick(Client *client, cs_str reason) {
	if(Client_IsBot(client)) {
		client->closed = true;
		return;
	}
	if(client->kickReason) return;
	if(!reason) reason = Sstor_Get("KICK_NOREASON");
	Vanilla_WriteKick(client, reason);
	Socket_Shutdown(client->sock, SD_SEND);
	client->kickReason = String_AllocCopy(reason);
}

void Client_KickFormat(Client *client, cs_str fmtreason, ...) {
	char kickreason[65];
	va_list args;
	va_start(args, fmtreason);
	if(String_FormatBufVararg(kickreason, 65, fmtreason, &args))
		Client_Kick(client, kickreason);
	else
		Client_Kick(client, fmtreason);
	va_end(args);
}
