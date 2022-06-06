#include "core.h"
#include "str.h"
#include "list.h"
#include "platform.h"
#include "block.h"
#include "netbuffer.h"
#include "protocol.h"
#include "client.h"
#include "event.h"
#include "strstor.h"
#include "compr.h"
#include "world.h"
#include "websock.h"
#include "groups.h"
#include "cpe.h"

Client *Clients_List[MAX_CLIENTS] = {NULL};

cs_byte Clients_GetCount(EClientState state) {
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

void Client_Init(Client *client, Socket fd, cs_ulong addr) {
	client->addr = addr;
	client->mutex = Mutex_Create();
	client->cpeData.model = 256;
	client->cpeData.clickDist = 160;
	NetBuffer_Init(&client->netbuf, fd);
	String_Copy(client->cpeData.appName, MAX_STR_LEN, "Vanilla client");
}

Client *Client_NewBot(void) {
	ClientID botid = FindFreeID();
	if(botid == CLIENT_SELF)
		return NULL;

	Client *client = Memory_Alloc(1, sizeof(Client));
	Client_Init(client, INVALID_SOCKET, 0);
	client->state = CLIENT_STATE_INGAME;
	client->id = botid;

	Clients_List[botid] = client;
	return client;
}

cs_bool Client_IsBot(Client *client) {
	return !NetBuffer_IsValid(&client->netbuf);
}

void Client_Lock(Client *client) {
	Mutex_Lock(client->mutex);
}

void Client_Unlock(Client *client) {
	Mutex_Unlock(client->mutex);
}

cs_str Client_GetName(Client *client) {
	return client->playerData.name;
}

cs_str Client_GetDisplayName(Client *client) {
	return client->playerData.displayname;
}

cs_str Client_GetKey(Client *client) {
	return client->playerData.key;
}

cs_str Client_GetAppName(Client *client) {
	return client->cpeData.appName;
}

cs_str Client_GetSkin(Client *client) {
	if(*client->cpeData.skin == '\0')
		return Client_GetName(client);
	return client->cpeData.skin;
}

Client *Client_GetByName(cs_str name) {
	for(ClientID i = 0; i < MAX_CLIENTS; i++) {
		Client *client = Clients_List[i];
		if(client && String_CaselessCompare(client->playerData.name, name))
			return client;
	}
	return NULL;
}

Client *Client_GetByID(ClientID id) {
	return id < MAX_CLIENTS ? Clients_List[id] : NULL;
}

ClientID Client_GetID(Client *client) {
	return client->id;
}

World *Client_GetWorld(Client *client) {
	return client->playerData.world;
}

BlockID Client_GetStandBlock(Client *client) {
	if(!client->playerData.world) return 0;
	SVec tpos; SVec_Copy(tpos, client->playerData.position);
	if(tpos.x < 0 || tpos.y < 0 || tpos.z < 0) return 0;
	tpos.y -= 2;

	BlockID id;
	for(cs_int16 x = -1; x < 2; x++) {
		for(cs_int16 z = -1; z < 2; z++) {
			SVec btest = tpos;
			btest.x += x; btest.z += z;
			id = World_GetBlock(client->playerData.world, &btest);
			if(id != BLOCK_AIR) {
				if(x != 0 || z != 0) {
					btest.y += 1;
					if(World_GetBlock(client->playerData.world, &btest))
						continue;
				}

				return id;
			}
		}
	}

	return BLOCK_AIR;
}

cs_int8 Client_GetFluidLevel(Client *client, BlockID *fluid) {
	if(!client->playerData.world) return 0;
	SVec tpos; SVec_Copy(tpos, client->playerData.position);
	if(tpos.x < 0 || tpos.y < 0 || tpos.z < 0) return 0;

	BlockID id;
	cs_byte level = 2;

	test_wtrlevel:
	if((id = World_GetBlock(client->playerData.world, &tpos)) > 7 && id < 12) {
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

static CGroup dgroup = {0, ""};

CGroup *Client_GetGroup(Client *client) {
	CGroup *gptr = Groups_GetByID(client->cpeData.group);
	return !gptr ? &dgroup : gptr;
}

cs_uintptr Client_GetGroupID(Client *client) {
	return client->cpeData.group;
}

cs_int16 Client_GetModel(Client *client) {
	return client->cpeData.model;
}

BlockID Client_GetHeldBlock(Client *client) {
	return client->cpeData.heldBlock;
}

cs_uint16 Client_GetClickDistance(Client *client) {
	return client->cpeData.clickDist;
}

cs_float Client_GetClickDistanceInBlocks(Client *client) {
	return client->cpeData.clickDist / 32.0f;
}

cs_int32 Client_GetExtVer(Client *client, cs_ulong exthash) {
	if(Client_IsBot(client)) return false;

	for(cs_int16 i = 0; i < client->cpeData.extensions.count; i++)
		if(client->cpeData.extensions.list[i].hash == exthash)
			return client->cpeData.extensions.list[i].version;

	return 0;
}

cs_bool Client_Despawn(Client *client) {
	if(!client->playerData.spawned)
		return false;
	client->playerData.spawned = false;
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
		client->playerData.world = world;
		Client_Spawn(client);
		return true;
	}

	if(client->mapData.world) return false;

	Client_Despawn(client);
	client->mapData.world = world;
	client->state = CLIENT_STATE_MOTD;
	client->playerData.position = world->info.spawnVec;
	client->playerData.angle = world->info.spawnAng;
	return true;
}

void Client_UpdateWorldInfo(Client *client, World *world, cs_bool updateAll) {
	if(Client_IsBot(client)) return;

	if(Client_GetExtVer(client, EXT_MAPASPECT)) {
		if(updateAll || world->info.modval & MV_COLORS) {
			for(cs_byte color = 0; color < WORLD_COLORS_COUNT; color++) {
				if(updateAll || world->info.modclr & (1 << color)) {
					Color3 ecol;
					if(World_GetEnvColor(world, color, &ecol))
						CPE_WriteEnvColor(client, color, &ecol);
				}
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
		CPEAppearance apps = {
			.texture = world->info.texturepack,
			.side = (BlockID)World_GetEnvProp(world, 0),
			.edge = (BlockID)World_GetEnvProp(world, 1),
			.sidelvl = (cs_int16)World_GetEnvProp(world, 2),
			.cloudlvl = (cs_int16)World_GetEnvProp(world, 3),
			.maxdist = (cs_int16)World_GetEnvProp(world, 4)
		};

		cs_int32 ver = Client_GetExtVer(client, EXT_MAPPROPS);
		if(ver > 0) CPE_WriteSetMapAppearance(client, ver, &apps);
	}

	if(updateAll || world->info.modval & MV_WEATHER)
		Client_SetWeather(client, world->info.weatherType);
}

CPECuboid *Client_NewSelection(Client *client) {
	if(Client_GetExtVer(client, EXT_CUBOID)) {
		for(cs_byte i = 0; i < CPE_MAX_CUBOIDS; i++) {
			CPECuboid *cub = &client->cpeData.cuboids[i];
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
		if(&client->cpeData.cuboids[cub->id] != cub)
			return false;
		CPE_WriteMakeSelection(client, cub);
		return true;
	}

	return false;
}

cs_bool Client_RemoveSelection(Client *client, CPECuboid *cub) {
	if(Client_GetExtVer(client, EXT_CUBOID)) {
		if(&client->cpeData.cuboids[cub->id] != cub)
			return false;
		CPE_WriteRemoveSelection(client, cub->id);
		cub->used = false;
		return true;
	}
	return false;
}

cs_bool Client_TeleportTo(Client *client, Vec *pos, Ang *ang) {
	if(Client_CheckState(client, CLIENT_STATE_INGAME)) {
		Vanilla_WriteTeleport(client, pos, ang);
		client->playerData.position = *pos;
		client->playerData.angle = *ang;
		if(Client_IsBot(client)) {
			for(ClientID i = 0; i < MAX_CLIENTS; i++) {
				Client *other = Clients_List[i];
				if(!other || Client_IsBot(other)) continue;
				if(!Client_CheckState(other, CLIENT_STATE_INGAME)) continue;
				if(!Client_IsInSameWorld(other, client)) continue;
				Vanilla_WritePosAndOrient(other, client);
			}
		}
		return true;
	}
	return false;
}

cs_bool Client_TeleportToSpawn(Client *client) {
	if(!client->playerData.world) return false;
	return Client_TeleportTo(client,
		&client->playerData.world->info.spawnVec,
		&client->playerData.world->info.spawnAng
	);
}

INL static cs_uint32 CopyMessagePart(cs_str msg, cs_char *part, cs_uint32 i, cs_char *color, cs_bool sanitize) {
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

		if(prevsym != '\r' && prevsym != '\n') {
			if(sanitize)
				*part++ = prevsym < ' ' || prevsym > '~' ? '?' : prevsym;
			else
				*part++ = prevsym;
		}
		if(nextsym == '\0') break;
		if(nextsym == '\n') {len = (j + 2); break;}
		if(prevsym == '&' && ISHEX(nextsym)) *color = nextsym;
	}

	*part = '\0';
	return len;
}

void Client_Chat(Client *client, EMesgType type, cs_str message) {
	if(client && Client_IsBot(client)) return;

	if(client && type == MESSAGE_TYPE_CHAT) {
		cs_char color = 0, part[MAX_STR_LEN] = {0};
		cs_bool next = false;
		while(*message != '\0') {
			cs_uint32 len = CopyMessagePart(message, part, next, &color, !client->cpeData.markedAsCPE);
			if(len > 0) {
				if((!next && *part != '\0') || (next && *(part + 2) != '\0'))
					Vanilla_WriteChat(client, type, part);
				message += len;
				next = true;
			}
		}

		return;
	}

	if(client == CLIENT_BROADCAST) {
		for(ClientID i = 0; i < MAX_CLIENTS; i++) {
			Client *bclient = Clients_List[i];
			if(bclient) Client_Chat(bclient, type, message);
		}

		return;
	}

	Vanilla_WriteChat(client, type, message);
}

cs_bool Client_CheckState(Client *client, EClientState state) {
	return client->state == state;
}

cs_bool Client_IsClosed(Client *client) {
	return !NetBuffer_IsAlive(&client->netbuf);
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
	return client->playerData.isOP;
}

cs_bool Client_IsSpawned(Client *client) {
	return client->playerData.spawned;
}

cs_bool Client_IsFirstSpawn(Client *client) {
	return client->playerData.firstSpawn;
}

void Client_SetBlock(Client *client, SVec *pos, BlockID id) {
	if(Client_GetExtVer(client, EXT_CUSTOMBLOCKS) < 1 ||
	(Client_GetExtVer(client, EXT_BLOCKDEF) || Client_GetExtVer(client, EXT_BLOCKDEF2)) < 1)
		id = Block_GetFallbackFor(Client_GetWorld(client), id);

	Vanilla_WriteSetBlock(client, pos, id);
}

cs_bool Client_SetEnvProperty(Client *client, EProp property, cs_int32 value) {
	if(Client_GetExtVer(client, EXT_MAPASPECT)) {
		CPE_WriteMapProperty(client, (cs_byte)property, value);
		return true;
	}
	return false;
}

cs_bool Client_SetEnvColor(Client *client, EColor type, Color3* color) {
	if(Client_GetExtVer(client, EXT_ENVCOLOR)) {
		CPE_WriteEnvColor(client, (cs_byte)type, color);
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
	if(String_Copy(client->playerData.displayname, MAX_STR_LEN, name))
		client->cpeData.updates |= PCU_NAME;
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
	if(Client_CheckState(client, CLIENT_STATE_INGAME) && !Client_GetExtVer(client, EXT_INSTANTMOTD))
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
		client->cpeData.clickDist = dist;
		return true;
	}
	return false;
}

cs_bool Client_SetHotkey(Client *client, cs_str action, ELWJGLKey keycode, ELWJGLMod keymod) {
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
	if(!CPE_CheckModel(client, model)) return false;
	client->cpeData.model = model;
	client->cpeData.updates |= PCU_MODEL;
	return true;
}

cs_bool Client_SetSkin(Client *client, cs_str skin) {
	String_Copy(client->cpeData.skin, MAX_STR_LEN, skin);
	client->cpeData.updates |= PCU_ENTITY;
	return true;
}

EClientState Client_GetState(Client *client) {
	return client->state;
}

cs_uint32 Client_GetAddr(Client *client) {
	return client->addr;
}

cs_int32 Client_GetPing(Client *client) {
	return client->cpeData.pingTime;
}

cs_float Client_GetAvgPing(Client *client) {
	return client->cpeData.pingAvgTime;
}

void Client_GetPosition(Client *client, Vec *pos, Ang *ang) {
	if(pos) *pos = client->playerData.position;
	if(ang) *ang = client->playerData.angle;
}

cs_bool Client_SetOP(Client *client, cs_bool state) {
	if(Client_IsBot(client)) return false;
	client->playerData.isOP = state;
	Event_Call(EVT_ONUSERTYPECHANGE, client);
	// if(Client_CheckState(client, CLIENT_STATE_INGAME))
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

cs_bool Client_SetVelocity(Client *client, Vec *velocity, cs_byte mode) {
	if(Client_GetExtVer(client, EXT_VELCTRL)) {
		CPE_WriteVelocityControl(client, velocity, mode);
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

cs_bool Client_UndefineModel(Client *client, cs_byte id) {
	if(Client_GetExtVer(client, EXT_CUSTOMMODELS)) {
		CPE_WriteUndefineModel(client, id);
		return true;
	}
	return false;
}

cs_bool Client_SetProp(Client *client, EEntProp prop, cs_int32 value) {
	if(prop >= ENTITY_PROP_COUNT) return false;
	client->cpeData.props[prop] = value;
	client->cpeData.updates |= PCU_ENTPROP;
	return true;
}

cs_bool Client_SetModelStr(Client *client, cs_str model) {
	return Client_SetModel(client, CPE_GetModelNum(model));
}

cs_bool Client_SetGroup(Client *client, cs_uintptr gid) {
	if(!Groups_GetByID(gid)) return false;
	client->cpeData.group = gid;
	client->cpeData.updates |= PCU_NAME;
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

INL static cs_bool SendCPEEntity(Client *client, Client *other) {
	cs_int32 ver = Client_GetExtVer(client, EXT_PLAYERLIST);
	if(!ver) return false;
	CPE_WriteAddEntity(client, ver, other);
	return true;
}

INL static void PushClientName(Client *client, Client *other) {
	if(!Client_IsBot(other))
		CPE_WriteAddName(client, other);
}

cs_bool Client_Update(Client *client) {
	if(client->cpeData.updates == PCU_NONE) return false;

	for(ClientID id = 0; id < MAX_CLIENTS; id++) {
		Client *other = Clients_List[id];
		if(other) {
			cs_bool hasplsupport = Client_GetExtVer(other, EXT_PLAYERLIST) == 2,
			hassmsupport = Client_GetExtVer(other, EXT_CHANGEMODEL) == 1,
			hasentprop = Client_GetExtVer(other, EXT_ENTPROP) == 1;
			if(client->cpeData.updates & PCU_NAME && hasplsupport) {
				client->cpeData.updates |= PCU_ENTITY;
				PushClientName(other, client);
			}
			if(Client_IsInSameWorld(client, other)) {
				if(client->cpeData.updates & PCU_ENTITY && hasplsupport) {
					client->cpeData.updates |= PCU_MODEL;
					SendCPEEntity(other, client);
				}
				if(client->cpeData.updates & PCU_MODEL && hassmsupport)
					CPE_WriteSetModel(other, client);
				if(client->cpeData.updates & PCU_ENTPROP && hasentprop)
					for(EEntProp i = 0; i < ENTITY_PROP_COUNT; i++)
						CPE_WriteSetEntityProperty(other, client, i, client->cpeData.props[i]);
			}
		}
	}

	client->cpeData.updates = PCU_NONE;
	return true;
}

cs_char *Client_StartRaw(Client *client, cs_uint32 msize) {
	if(!NetBuffer_IsAlive(&client->netbuf) || Client_IsBot(client)) return NULL;
	return NetBuffer_StartWrite(&client->netbuf, msize);
}

cs_bool Client_EndRaw(Client *client, cs_uint32 psize) {
	return NetBuffer_EndWrite(&client->netbuf, psize);
}

void Client_Free(Client *client) {
	if(client->mutex) {
		Mutex_Free(client->mutex);
		client->mutex = NULL;
	}
	if(client->websock) {
		Memory_Free(client->websock);
		client->websock = NULL;
	}
	if(client->kickReason) {
		Memory_Free((void *)client->kickReason);
		client->kickReason = false;
	}
	if(client->cpeData.extensions.list) {
		Memory_Free((void *)client->cpeData.extensions.list);
		client->cpeData.extensions.list = NULL;
	}

	while(client->headNode) {
		Memory_Free(client->headNode->value.ptr);
		KList_Remove(&client->headNode, client->headNode);
	}

	NetBuffer_ForceClose(&client->netbuf);
	Compr_Cleanup(&client->mapData.compr);
	Memory_Free(client);
}

NOINL static void HandlePacket(Client *client, cs_char *data) {
	packetHandler phand = NULL;

	if(client->packetData.isExtended)
		if(client->packetData.packet->extHandler)
			phand = (packetHandler)client->packetData.packet->extHandler;
		else
			phand = (packetHandler)client->packetData.packet->handler;
	else
		if(client->packetData.packet->handler)
			phand = (packetHandler)client->packetData.packet->handler;

	if(phand(client, data) == false)
		Client_KickFormat(client, Sstor_Get("KICK_PERR_UNEXP"), client->packetData.packet->id);
}

INL static cs_uint16 GetPacketSizeFor(Packet *packet, Client *client, cs_bool *extended) {
	if(packet->haveCPEImp) {
		*extended = Client_GetExtVer(client, packet->exthash) == packet->extVersion;
		if(*extended) return packet->extSize;
	}
	return packet->size;
}

INL static void PacketReceiverWs(Client *client) {
	wsrecvmark:
	if(WebSock_Tick(client->websock, &client->netbuf)) {
		if(client->websock->opcode == 0x08) {
			NetBuffer_ForceClose(&client->netbuf);
			return;
		}

		cs_char *data = client->websock->payload;
		cs_uint16 availpay = client->websock->paylen;
		PacketData *pdata = &client->packetData;

		while(availpay > 0) {
			EPacketID packetId = (EPacketID)*data++;
			pdata->packet = Packet_Get(packetId);
			if(!pdata->packet) {
				Client_KickFormat(client, Sstor_Get("KICK_PERR_NOHANDLER"), packetId);
				return;
			}
			pdata->psize = GetPacketSizeFor(pdata->packet, client, &pdata->isExtended);
			availpay -= 1;

			if(availpay >= pdata->psize) {
				HandlePacket(client, data);
				client->lastmsg = Time_GetMSec();
				availpay -= pdata->psize;
				data += pdata->psize;
			}
		}

		goto wsrecvmark;
	} else if(client->websock->error != WEBSOCK_ERROR_CONTINUE) {
		if(client->websock->error == WEBSOCK_ERROR_SOCKET)
			NetBuffer_ForceClose(&client->netbuf);
		else
			Client_KickFormat(client, WebSock_GetError(client->websock));
	}
}

INL static void PacketReceiverRaw(Client *client) {
	PacketData *pdata = &client->packetData;
	recvmark:

	if(!pdata->packet) {
		if(NetBuffer_AvailRead(&client->netbuf) >= 1) {
			EPacketID packetId = (EPacketID)*NetBuffer_Read(&client->netbuf, 1);
			pdata->packet = Packet_Get(packetId);
			if(!pdata->packet) {
				Client_KickFormat(client, Sstor_Get("KICK_PERR_NOHANDLER"), packetId);
				return;
			}

			pdata->psize = GetPacketSizeFor(pdata->packet, client, &pdata->isExtended);
		} else return;
	}

	if(NetBuffer_AvailRead(&client->netbuf) >= pdata->psize) {
		HandlePacket(client, NetBuffer_Read(&client->netbuf, pdata->psize));
		client->lastmsg = Time_GetMSec();
		pdata->packet = NULL;
		pdata->psize = 0;
		goto recvmark;
	}
}

NOINL static cs_bool SendWorldTick(Client *client) {
	MapData *md = &client->mapData;

	if(!World_IsReadyToPlay(md->world)) {
		if(!World_Load(md->world)) {
			Client_Kick(client, Sstor_Get("KICK_INT"));
			return true;
		}
	}

	if(md->sent == 0) { // Передача только началась
		World_StartTask(md->world);
		if(md->world != client->playerData.world) {
			if(Client_GetExtVer(client, EXT_BLOCKDEF)) {
				World *oldworld = client->playerData.world;
				for(cs_uint16 id = 0; id < 256; id++) {
					BlockDef *newbdef = Block_GetDefinition(md->world, (BlockID)id);
					if(oldworld) {
						BlockDef *unbdef = Block_GetDefinition(oldworld, (BlockID)id);
						if(unbdef) Client_UndefineBlock(client, (BlockID)id);
					}
					if(newbdef) Client_DefineBlock(client, (BlockID)id, newbdef);
				}
			}
		}

		md->fastmap = Client_GetExtVer(client, EXT_FASTMAP) == 1;
		md->fback = Client_GetExtVer(client, EXT_BLOCKDEF) < 1 ||
		Client_GetExtVer(client, EXT_BLOCKDEF2) < 1 ||
		Client_GetExtVer(client, EXT_CUSTOMBLOCKS) < 1;
		if(md->fastmap) {
			if(Compr_Init(&md->compr, COMPR_TYPE_DEFLATE)) {
				md->cbptr = World_GetBlockArray(md->world, &md->size);
				CPE_WriteFastMapInit(client, md->size);
			} else goto mapfail;
		} else {
			if(Compr_Init(&md->compr, COMPR_TYPE_GZIP)) {
				md->cbptr = World_GetData(md->world, &md->size);
				Vanilla_WriteLvlInit(client);
			} else goto mapfail;
		}
	}

	if(md->sent <= md->size) {
		cs_uint64 markStart = Time_GetMSec();
		cs_byte indata[10240] = {0};

		while(NetBuffer_IsAlive(&client->netbuf)) {
			cs_uint32 avail;
			comprstep:
			avail = min(md->size - md->sent, 10240);
			if(avail > 0) {
				if(md->fback) {
					for(cs_uint32 i = 0; i < avail; i++) {
						cs_uint32 offset = md->sent + i;
						if(offset < 4 && !md->fastmap)
							indata[i] = md->cbptr[offset];
						else
							indata[i] = Block_GetFallbackFor(md->world, md->cbptr[offset]);
					}
				} else
					Memory_Copy(indata, md->cbptr + md->sent, avail);
				Compr_SetInBuffer(&md->compr, indata, avail);
				md->sent += avail;
			}

			Mutex_Lock(client->mutex);
			while(true) {
				cs_byte *data = (cs_byte *)NetBuffer_StartWrite(&client->netbuf, 1030);
				Compr_SetOutBuffer(&md->compr, data + 3, 1024);
				*data = 0x03;

				if(Compr_Update(&md->compr)) {
					if(md->compr.written > 0) {
						*((cs_uint16 *)(data + 1)) = htons((cs_uint16)md->compr.written);
						*(data + 1028) = (cs_byte)(((cs_float)md->sent / md->size) * 100);
						if(!NetBuffer_EndWrite(&client->netbuf, 1028)) {
							Mutex_Unlock(client->mutex);
							goto mapend;
						}
					} else break;
				} else {
					Mutex_Unlock(client->mutex);
					goto mapfail;
				}
			}
			Mutex_Unlock(client->mutex);

			if(Compr_IsInState(&md->compr, COMPR_STATE_DONE))
				break;

			// Не даём серверу слишком долго сжимать карту для клиента
			// Поле taskc хранит в себе количество подключающихся в данный момент клиентов к данному миру
			if(Time_GetMSec() - markStart < (TICKS_PER_SECOND / md->world->taskc))
				goto comprstep;
			else
				return false;
		}

		if(!NetBuffer_IsAlive(&client->netbuf)) {
			goto mapfail;
		} else {
			Vanilla_WriteLvlFin(client, &md->world->info.dimensions);
			client->playerData.world = md->world;
			client->state = CLIENT_STATE_INGAME;
			Client_Spawn(client);
			goto mapend;
		}
	}

	mapfail:
	Client_KickFormat(client, Sstor_Get("KICK_ZERR"), Compr_GetLastError(&md->compr));

	mapend:
	World_EndTask(md->world);
	Compr_Reset(&md->compr);
	md->world = NULL;
	md->sent = 0;
	md->size = 0;
	return true;
}

void Client_Tick(Client *client) {
	if(client->websock)
		PacketReceiverWs(client);
	else
		PacketReceiverRaw(client);

	if(client->mapData.world) {
		if(SendWorldTick(client))
			client->mapData.world = NULL;
	}
}

INL static void SendSpawnPacket(Client *client, Client *other) {
	if(!SendCPEEntity(client, other))
		Vanilla_WriteSpawn(client, other);
}

cs_bool Client_Spawn(Client *client) {
	if(client->playerData.spawned)
		return false;

	onSpawn evt = {
		.client = client,
		.position = &client->playerData.position,
		.angle = &client->playerData.angle,
		.updateenv = true
	};
	Event_Call(EVT_ONSPAWN, &evt);
	// Vanilla_WriteUserType(client, client->playerData.isOP ? 0x64 : 0x00);
	if(evt.updateenv) Client_UpdateWorldInfo(client, client->playerData.world, true);
	client->cpeData.updates = PCU_NONE;

	for(ClientID i = 0; i < MAX_CLIENTS; i++) {
		Client *other = Clients_List[i];
		if(!other || !Client_CheckState(other, CLIENT_STATE_INGAME)) continue;

		if(client->playerData.firstSpawn) {
			if(Client_GetExtVer(other, EXT_PLAYERLIST))
				PushClientName(other, client);
			if(Client_GetExtVer(client, EXT_PLAYERLIST) && client != other)
				PushClientName(client, other);
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

	client->playerData.spawned = true;
	client->playerData.firstSpawn = false;
	return true;
}

cs_str Client_GetDisconnectReason(Client *client) {
	return client->kickReason ? client->kickReason : "Disconnect";
}

void Client_Kick(Client *client, cs_str reason) {
	if(Client_IsBot(client)) {
		NetBuffer_ForceClose(&client->netbuf);
		return;
	}
	if(client->kickReason) return;
	if(!reason) reason = Sstor_Get("KICK_NOREASON");
	Vanilla_WriteKick(client, reason);
	client->kickReason = String_AllocCopy(reason);
	NetBuffer_Shutdown(&client->netbuf);
}

void Client_KickFormat(Client *client, cs_str fmtreason, ...) {
	char kickreason[MAX_STR_LEN];
	va_list args;
	va_start(args, fmtreason);
	if(String_FormatBufVararg(kickreason, MAX_STR_LEN, fmtreason, &args))
		Client_Kick(client, kickreason);
	else
		Client_Kick(client, fmtreason);
	va_end(args);
}
