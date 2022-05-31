#include "core.h"
#include "str.h"
#include "log.h"
#include "block.h"
#include "client.h"
#include "event.h"
#include "server.h"
#include "netbuffer.h"
#include "protocol.h"
#include "platform.h"
#include "command.h"
#include "heartbeat.h"
#include "strstor.h"
#include "compr.h"
#include "vector.h"
#include "world.h"
#include "config.h"
#include "cpe.h"

#define ValidateClientState(client, st, ret) \
if(!Client_CheckState(client, st)) return ret

#define ValidateCpeClient(client, ret) \
if(client->extData.type != CLIENT_TYPE_CPE) return ret

#define PacketWriter_Start(cl, msz) \
if(!NetBuffer_IsAlive(&cl->netbuf) || Client_IsBot(cl)) return; \
Mutex_Lock(cl->mutex); \
cs_char *data = NetBuffer_StartWrite(&cl->netbuf, msz); \
cs_char *start = data; \
if(data == NULL) { \
	NetBuffer_ForceClose(&cl->netbuf); \
	Mutex_Unlock(cl->mutex); \
	return; \
}

#define PacketWriter_End(cl) \
NetBuffer_EndWrite(&cl->netbuf, (cs_uint32)(data - start)); \
Mutex_Unlock(cl->mutex);

static cs_uint16 extensionsCount = 0;
static CPESvExt *headExtension = NULL;

void Proto_WriteString(cs_char **dataptr, cs_str string) {
	cs_size size = 0;
	if(string)
		size = min(String_Length(string), 64);
	for(cs_byte i = 0; i < 64; i++)
		*(*dataptr)++ = i < size ? string[i] : ' ';
}

void Proto_WriteFlVec(cs_char **dataptr, const Vec *vec) {
	cs_char *data = *dataptr;
	*(cs_int32 *)data = htonl((cs_uint32)(vec->x * 32)); data += 4;
	*(cs_int32 *)data = htonl((cs_uint32)(vec->y * 32)); data += 4;
	*(cs_int32 *)data = htonl((cs_uint32)(vec->z * 32)); data += 4;
	*dataptr = data;
}

void Proto_WriteFlSVec(cs_char **dataptr, const Vec *vec) {
	cs_char *data = *dataptr;
	*(cs_int16 *)data = htons((cs_uint16)(vec->x * 32)); data += 2;
	*(cs_int16 *)data = htons((cs_uint16)(vec->y * 32)); data += 2;
	*(cs_int16 *)data = htons((cs_uint16)(vec->z * 32)); data += 2;
	*dataptr = data;
}

void Proto_WriteSVec(cs_char **dataptr, const SVec *vec) {
	cs_char *data = *dataptr;
	*(cs_int16 *)data = htons((cs_uint16)vec->x); data += 2;
	*(cs_int16 *)data = htons((cs_uint16)vec->y); data += 2;
	*(cs_int16 *)data = htons((cs_uint16)vec->z); data += 2;
	*dataptr = data;
}

void Proto_WriteAng(cs_char **dataptr, const Ang *ang) {
	cs_char *data = *dataptr;
	*(cs_byte *)data++ = (cs_byte)((ang->yaw) * 256.0f / 360.0f);
	*(cs_byte *)data++ = (cs_byte)((ang->pitch) * 256.0f / 360.0f);
	*dataptr = data;
}

void Proto_WriteColor3(cs_char **dataptr, const Color3* color) {
	cs_char *data = *dataptr;
	*(cs_int16 *)data = htons((cs_uint16)color->r); data += 2;
	*(cs_int16 *)data = htons((cs_uint16)color->g); data += 2;
	*(cs_int16 *)data = htons((cs_uint16)color->b); data += 2;
	*dataptr = data;
}

void Proto_WriteColor4(cs_char **dataptr, const Color4* color) {
	cs_char *data = *dataptr;
	*(cs_int16 *)data = (cs_int16)htons(color->r); data += 2;
	*(cs_int16 *)data = (cs_int16)htons(color->g); data += 2;
	*(cs_int16 *)data = (cs_int16)htons(color->b); data += 2;
	*(cs_int16 *)data = (cs_int16)htons(color->a); data += 2;
	*dataptr = data;
}

void Proto_WriteByteColor3(cs_char **dataptr, const Color3* color) {
	cs_char *data = *dataptr;
	*data++ = (cs_int8)color->r;
	*data++ = (cs_int8)color->g;
	*data++ = (cs_int8)color->b;
	*dataptr = data;
}

void Proto_WriteByteColor4(cs_char **dataptr, const Color4* color) {
	cs_char *data = *dataptr;
	*data++ = (cs_char)color->r;
	*data++ = (cs_char)color->g;
	*data++ = (cs_char)color->b;
	*data++ = (cs_char)color->a;
	*dataptr = data;
}

void Proto_WriteFloat(cs_char **dataptr, cs_float num) {
	union {
		cs_float num;
		cs_uint32 numi;
	} fi;
	fi.num = num;
	*(cs_uint32 *)*dataptr = htonl(fi.numi);
	*dataptr += sizeof(fi);
}

NOINL static void WriteExtEntityPos(cs_char **dataptr, Vec *vec, Ang *ang, cs_bool extended, cs_bool sub) {
	Vec avec = *vec;
	if(sub) avec.y -= 0.66f;

	if(extended)
		Proto_WriteFlVec(dataptr, &avec);
	else
		Proto_WriteFlSVec(dataptr, &avec);
	Proto_WriteAng(dataptr, ang);
}

cs_byte Proto_ReadString(cs_char **dataptr, cs_str *dstptr) {
	cs_str data = *dataptr;
	*dataptr += 64;
	cs_byte end;

	for(end = 64; end > 0; end--)
		if(data[end - 1] != ' ') break;

	if(end > 0) {
		cs_char *str = Memory_Alloc(end + 1, 1);
		Memory_Copy(str, data, end);
		str[end] = '\0';
		if(*dstptr) Memory_Free((void *)*dstptr);
		*dstptr = str;
	};

	return end;
}

cs_byte Proto_ReadStringNoAlloc(cs_char **dataptr, cs_char *dst) {
	cs_str data = *dataptr;
	*dataptr += 64;
	cs_byte end;

	for(end = 64; end > 0; end--)
		if(data[end - 1] != ' ') break;

	if(end > 0) {
		Memory_Copy(dst, data, end);
		dst[end] = '\0';
	}

	return end;
}

void Proto_ReadSVec(cs_char **dataptr, SVec *vec) {
	cs_char *data = *dataptr;
	vec->x = ntohs(*(cs_int16 *)data); data += 2;
	vec->y = ntohs(*(cs_int16 *)data); data += 2;
	vec->z = ntohs(*(cs_int16 *)data); data += 2;
	*dataptr = data;
}

void Proto_ReadAng(cs_char **dataptr, Ang *ang) {
	cs_char *data = *dataptr;
	ang->yaw = (*data++) * 360.0f / 256.0f;
	ang->pitch = (*data++) * 360.0f / 256.0f;
	*dataptr = data;
}

void Proto_ReadFlSVec(cs_char **dataptr, Vec *vec) {
	cs_char *data = *dataptr;
	vec->x = (cs_int16)ntohs(*(cs_uint16 *)data) / 32.0f; data += 2;
	vec->y = (cs_int16)ntohs(*(cs_uint16 *)data) / 32.0f; data += 2;
	vec->z = (cs_int16)ntohs(*(cs_uint16 *)data) / 32.0f; data += 2;
	*dataptr = data;
}

void Proto_ReadFlVec(cs_char **dataptr, Vec *vec) {
	cs_char *data = *dataptr;
	vec->x = (cs_int32)ntohl(*(cs_uint32 *)data) / 32.0f; data += 4;
	vec->y = (cs_int32)ntohl(*(cs_uint32 *)data) / 32.0f; data += 4;
	vec->z = (cs_int32)ntohl(*(cs_uint32 *)data) / 32.0f; data += 4;
	*dataptr = data;
}

void Vanilla_WriteServerIdent(Client *client, cs_str name, cs_str motd) {
	PacketWriter_Start(client, 131);

	*data++ = PACKET_IDENTIFICATION;
	*data++ = PROTOCOL_VERSION;
	Proto_WriteString(&data, name);
	Proto_WriteString(&data, motd);
	*data++ = 0x00;

	PacketWriter_End(client);
}

void Vanilla_WriteLvlInit(Client *client) {
	PacketWriter_Start(client, 1);

	*data++ = PACKET_LEVELINIT;

	PacketWriter_End(client);
}

void Vanilla_WriteLvlFin(Client *client, SVec *dims) {
	PacketWriter_Start(client, 7);

	*data++ = PACKET_LEVELFINAL;
	Proto_WriteSVec(&data, dims);

	PacketWriter_End(client);
}

void Vanilla_WriteSetBlock(Client *client, SVec *pos, BlockID block) {
	PacketWriter_Start(client, 8);

	*data++ = PACKET_SETBLOCK_SERVER;
	Proto_WriteSVec(&data, pos);
	*data++ = block;

	PacketWriter_End(client);
}

void Vanilla_WriteSpawn(Client *client, Client *other) {
	PacketWriter_Start(client, 80);

	*data++ = PACKET_ENTITYSPAWN;
	*data++ = client == other ? CLIENT_SELF : other->id;
	Proto_WriteString(&data, other->playerData.name);
	WriteExtEntityPos(
		&data,
		&other->playerData.position,
		&other->playerData.angle,
		Client_GetExtVer(client, EXT_ENTPOS) > 0,
		client == other
	);

	PacketWriter_End(client);
}

void Vanilla_WriteTeleport(Client *client, Vec *pos, Ang *ang) {
	PacketWriter_Start(client, 18);

	*data++ = PACKET_ENTITYTELEPORT;
	*data++ = CLIENT_SELF;
	WriteExtEntityPos(
		&data, pos, ang,
		Client_GetExtVer(client, EXT_ENTPOS) > 0,
		true
	);

	PacketWriter_End(client);
}

void Vanilla_WritePosAndOrient(Client *client, Client *other) {
	PacketWriter_Start(client, 18);

	*data++ = PACKET_ENTITYTELEPORT;
	*data++ = client == other ? CLIENT_SELF : other->id;
	WriteExtEntityPos(
		&data,
		&other->playerData.position,
		&other->playerData.angle,
		Client_GetExtVer(client, EXT_ENTPOS) > 0,
		client == other
	);

	PacketWriter_End(client);
}

void Vanilla_WriteDespawn(Client *client, Client *other) {
	PacketWriter_Start(client, 2);

	*data++ = PACKET_ENTITYDESPAWN;
	*data++ = client == other ? CLIENT_SELF : other->id;

	PacketWriter_End(client);
}

void Vanilla_WriteChat(Client *client, EMesgType type, cs_str mesg) {
	PacketWriter_Start(client, 66);

	*data++ = PACKET_SENDMESSAGE;
	*data++ = (cs_byte)type;
	Proto_WriteString(&data, mesg);

	PacketWriter_End(client);
}

void Vanilla_WriteKick(Client *client, cs_str reason) {
	PacketWriter_Start(client, 65);

	*data++ = PACKET_ENTITYREMOVE;
	Proto_WriteString(&data, reason);

	PacketWriter_End(client);
}

void Vanilla_WriteUserType(Client *client, cs_byte type) {
	PacketWriter_Start(client, 2);

	*data++ = PACKET_USERUPDATE;
	*data++ = type;

	PacketWriter_End(client);
}

static cs_bool FinishHandshake(Client *client) {
	cs_int32 extVer = Client_GetExtVer(client, EXT_CUSTOMMODELS);
	cs_bool hasParts = Client_GetExtVer(client, EXT_CUSTOMPARTS) > 0;
	for(cs_int16 i = 0; i < max(CPE_MODELS_COUNT, CPE_PARTICLES_COUNT); i++) {
		CPE_SendModel(client, extVer, (cs_byte)i);
		if(hasParts) CPE_SendParticle(client, (cs_byte)i);
	}

	onHandshakeDone evt = {
		.client = client,
		.world = World_Main
	};

	return Event_Call(EVT_ONHANDSHAKEDONE, &evt) &&
	Client_ChangeWorld(client, evt.world);
}

cs_bool Handler_Handshake(Client *client, cs_char *data) {
	if(*client->playerData.name != '\0') return false;
	cs_byte protoVer = *data++;
	if(protoVer != PROTOCOL_VERSION) {
		Client_KickFormat(client, Sstor_Get("KICK_PROTOVER"), PROTOCOL_VERSION, protoVer);
		return true;
	}

	client->playerData.firstSpawn = true;
	if(Client_IsLocal(client) && Config_GetBoolByKey(Server_Config, CFG_LOCALOP_KEY))
		client->playerData.isOP = true;

	if(!Proto_ReadStringNoAlloc(&data, client->playerData.name)) return false;
	if(Config_GetBoolByKey(Server_Config, CFG_SANITIZE_KEY)) {
		for(cs_char *c = client->playerData.name; *c != '\0'; c++) {
			if((*c < '0' || *c > '9') && (*c < 'A' || *c > 'Z') && (*c < 'a' || *c > 'z') && *c != '_') {
				Client_Kick(client, Sstor_Get("KICK_NAMECHARS"));
				return true;
			}
		}
	}

	if(!Proto_ReadStringNoAlloc(&data, client->playerData.key)) return false;
	String_Copy(client->playerData.displayname, 65, client->playerData.name);
	if(*data == 0x42) client->extData.type = CLIENT_TYPE_CPE;

	for(ClientID i = 0; i < MAX_CLIENTS; i++) {
		Client *other = Clients_List[i];
		if(!other || other == client) continue;
		if(String_CaselessCompare(client->playerData.name, other->playerData.name)) {
			Client_Kick(client, Sstor_Get("KICK_NAMEINUSE"));
			return true;
		}
	}

	if(Heartbeat_Validate(client)) {
		preHandshakeDone params = {
			.client = client
		};
		String_Copy(params.name, 65, Config_GetStrByKey(Server_Config, CFG_SERVERNAME_KEY));
		String_Copy(params.motd, 65, Config_GetStrByKey(Server_Config, CFG_SERVERMOTD_KEY));
		Event_Call(EVT_PREHANDSHAKEDONE, &params);
		Client_SetServerIdent(client, params.name, params.motd);
	} else {
		Client_Kick(client, Sstor_Get("KICK_AUTHFAIL"));
		return true;
	}

	if(client->extData.type == CLIENT_TYPE_CPE) {
		CPE_WriteInfo(client);
		CPESvExt *ptr = headExtension;
		while(ptr) {
			CPE_WriteExtEntry(client, ptr);
			ptr = ptr->next;
		}
	} else return FinishHandshake(client);

	return true;
}

INL static void UpdateBlock(World *world, SVec *pos, BlockID block) {
	for(ClientID i = 0; i < MAX_CLIENTS; i++) {
		Client *client = Clients_List[i];
		if(client && Client_IsInWorld(client, world))
			Client_SetBlock(client, pos, block);
	}
}

cs_bool Handler_SetBlock(Client *client, cs_char *data) {
	ValidateClientState(client, CLIENT_STATE_INGAME, true);

	World *world = Client_GetWorld(client);
	if(!world || !World_IsReadyToPlay(world)) return false;

	onBlockPlace params;
	params.client = client;
	Proto_ReadSVec(&data, &params.pos);
	params.mode = (ESetBlockMode)*data++;
	params.id = *data;

	switch(params.mode) {
		case SETBLOCK_MODE_CREATE:
			if(!Block_IsValid(world, params.id)) {
				Client_KickFormat(client, Sstor_Get("KICK_UNKBID"), params.id);
				return false;
			}
			if(Event_Call(EVT_ONBLOCKPLACE, &params) && World_SetBlock(world, &params.pos, params.id))
				UpdateBlock(world, &params.pos, params.id);
			else
				Vanilla_WriteSetBlock(client, &params.pos, World_GetBlock(world, &params.pos));
			break;
		case SETBLOCK_MODE_DESTROY:
			params.id = BLOCK_AIR;
			if(Event_Call(EVT_ONBLOCKPLACE, &params) && World_SetBlock(world, &params.pos, params.id))
				UpdateBlock(world, &params.pos, params.id);
			else
				Vanilla_WriteSetBlock(client, &params.pos, World_GetBlock(world, &params.pos));
			break;
		
		default:
			return false;
	}

	return true;
}

INL static cs_bool ReadClientPos(Client *client, cs_char *data) {
	Vec *vec = &client->playerData.position, newVec;
	Ang *ang = &client->playerData.angle, newAng;
	cs_bool changed = false;

	if(Client_GetExtVer(client, EXT_ENTPOS))
		Proto_ReadFlVec(&data, &newVec);
	else
		Proto_ReadFlSVec(&data, &newVec);

	Proto_ReadAng(&data, &newAng);

	if(!Vec_Compare(&newVec, vec)) {
		client->playerData.position = newVec;
		Event_Call(EVT_ONMOVE, client);
		changed = true;
	}

	if(!Ang_Compare(&newAng, ang)) {
		client->playerData.angle = newAng;
		Event_Call(EVT_ONROTATE, client);
		changed = true;
	}

	return changed;
}

cs_bool Handler_PosAndOrient(Client *client, cs_char *data) {
	ValidateClientState(client, CLIENT_STATE_INGAME, true);

	BlockID cb = *data++;
	if(Client_GetExtVer(client, EXT_HELDBLOCK) == 1) {
		if(client->extData.heldBlock != cb) {
			onHeldBlockChange params = {
				.client = client,
				.prev = client->extData.heldBlock,
				.curr = cb
			};
			Event_Call(EVT_ONHELDBLOCKCHNG, &params);
			client->extData.heldBlock = cb;
		}
	}

	if(ReadClientPos(client, data)) {
		for(ClientID i = 0; i < MAX_CLIENTS; i++) {
			Client *other = Clients_List[i];
			if(other && client != other && Client_CheckState(other, CLIENT_STATE_INGAME) && Client_IsInSameWorld(client, other))
				Vanilla_WritePosAndOrient(other, client);
		}
	}
	return true;
}

static cs_bool isUrl(cs_char *mesg, cs_byte ls, cs_byte en) {
	if((en - ls) < 7) return false;

	while((en - ls) > 0 && mesg[ls] == '&') { ls += 2; en -= 2; }

	if(mesg[ls] != 'h' || mesg[ls + 1] != 't' || mesg[ls + 2] != 't' || mesg[ls + 3] != 'p')
		return false;

	ls += 4; en -= 4;
	if(mesg[ls] == 's') { ls++; en--; }

	return (en - ls) >= 3 && mesg[ls] == ':' && mesg[ls + 1] == '/' && mesg[ls + 2] == '/';
}

static void convertColors(cs_char *message, cs_byte len) {
	cs_byte last_space = 0;
	for(cs_byte i = 0; i < len; i++) {
		if(message[i] == ' ') last_space = i;
		if(message[i] == '%' && message[i + 1] != '\0') {
			if(!isUrl(message, last_space, i))
				message[i] = '&';
		}
	}
}

cs_bool Handler_Message(Client *client, cs_char *data) {
	ValidateClientState(client, CLIENT_STATE_INGAME, true);

	onMessage params = {
		.client = client,
		.type = MESSAGE_TYPE_CHAT
	};
	cs_byte partial = *data++,
	len = Proto_ReadStringNoAlloc(&data, params.message);
	if(len == 0) return false;
	convertColors(params.message, len);

	if(Client_GetExtVer(client, EXT_LONGMSG)) {
		if(String_Append(client->extData.message, CPE_MAX_EXTMESG_LEN, params.message) && partial == 1) return true;
		String_Copy(params.message, CPE_MAX_EXTMESG_LEN, client->extData.message);
	}

	if(*params.message == '/') {
		if(!Command_Handle(params.message, client))
			Vanilla_WriteChat(client, MESSAGE_TYPE_CHAT, Sstor_Get("CMD_UNK"));
	} else {
		if(Event_Call(EVT_ONMESSAGE, &params)) {
			cs_char formatted[320] = {0};

			if(String_FormatBuf(formatted, 320, "<%s&f>: %s", Client_GetDisplayName(client), params.message))
				Client_Chat(CLIENT_BROADCAST, params.type, formatted);
		}
	}

	/*
	 * Нужно для очистки буфера частичных
	 * сообщений дополнения LongerMessages
	 */
	*client->extData.message = '\0';
	return true;
}

/*
 * Врайтеры и хендлеры
 * CPE протокола
 */

void CPE_WriteInfo(Client *client) {
	PacketWriter_Start(client, 67);

	*data++ = PACKET_EXTINFO;
	Proto_WriteString(&data, SOFTWARE_FULLNAME);
	*(cs_uint16 *)data = htons(extensionsCount);
	data += 2;

	PacketWriter_End(client);
}

void CPE_WriteExtEntry(Client *client, CPESvExt *ext) {
	PacketWriter_Start(client, 69);

	*data++ = PACKET_EXTENTRY;
	Proto_WriteString(&data, ext->name);
	*(cs_uint32 *)data = htonl(ext->version);
	data += 4;

	PacketWriter_End(client);
}

void CPE_WriteClickDistance(Client *client, cs_uint16 dist) {
	PacketWriter_Start(client, 3);

	*data++ = PACKET_CLICKDIST;
	*(cs_uint16 *)data = htons(dist);
	data += 2;

	PacketWriter_End(client);
}

void CPE_CustomBlockSupportLevel(Client *client, cs_byte level) {
	PacketWriter_Start(client, 2);

	*data++ = PACKET_CUSTOMBLOCKSLVL;
	*data++ = level;

	PacketWriter_End(client);
}

void CPE_WriteHoldThis(Client *client, BlockID block, cs_bool preventChange) {
	PacketWriter_Start(client, 3);

	*data++ = PACKET_HOLDTHIS;
	*data++ = block;
	*data++ = (cs_char)preventChange;

	PacketWriter_End(client);
}

void CPE_WriteSetHotKey(Client *client, cs_str action, ELWJGLKey keycode, ELWJGLMod keymod) {
	PacketWriter_Start(client, 134);

	*data++ = PACKET_SETHOTKEY;
	Proto_WriteString(&data, NULL); // Label
	Proto_WriteString(&data, action);
	*(cs_uint32 *)data = htonl((cs_uint32)keycode); data += 4;
	*data++ = (cs_byte)keymod;

	PacketWriter_End(client);
}

void CPE_WriteAddName(Client *client, Client *other) {
	PacketWriter_Start(client, 196);

	*data = PACKET_NAMEADD; data += 2;
	*data++ = client == other ? CLIENT_SELF : other->id;
	Proto_WriteString(&data, Client_GetName(other));
	Proto_WriteString(&data, Client_GetDisplayName(other));
	CGroup *group = Client_GetGroup(other);
	Proto_WriteString(&data, group->name);
	*data++ = group->rank;

	PacketWriter_End(client);
}

void CPE_WriteAddEntity_v1(Client *client, Client *other) {
	PacketWriter_Start(client, 130);

	*data++ = PACKET_EXTENTITYADDv1;
	*data++ = client == other ? CLIENT_SELF : other->id;
	Proto_WriteString(&data, Client_GetDisplayName(other));
	Proto_WriteString(&data, Client_GetSkin(other));

	PacketWriter_End(client);
}

void CPE_WriteAddEntity_v2(Client *client, Client *other) {
	PacketWriter_Start(client, 144);

	*data++ = PACKET_EXTENTITYADDv2;
	*data++ = client == other ? CLIENT_SELF : other->id;
	Proto_WriteString(&data, Client_GetDisplayName(other));
	Proto_WriteString(&data, Client_GetSkin(other));
	WriteExtEntityPos(
		&data,
		&other->playerData.position,
		&other->playerData.angle,
		Client_GetExtVer(client, EXT_ENTPOS) > 0,
		client == other
	);

	PacketWriter_End(client);
}

void CPE_WriteRemoveName(Client *client, Client *other) {
	PacketWriter_Start(client, 3);

	*data = PACKET_NAMEREMOVE; data += 2;
	*data++ = client == other ? CLIENT_SELF : other->id;

	PacketWriter_End(client);
}

void CPE_WriteEnvColor(Client *client, cs_byte type, Color3* col) {
	PacketWriter_Start(client, 8);

	*data++ = PACKET_SETENVCOLOR;
	*data++ = type;
	Proto_WriteColor3(&data, col);

	PacketWriter_End(client);
}

void CPE_WriteMakeSelection(Client *client, CPECuboid *cub) {
	PacketWriter_Start(client, 86);

	*data++ = PACKET_SELECTIONADD;
	*data++ = cub->id;
	Proto_WriteString(&data, NULL); // Клиент игнорирует Label
	Proto_WriteSVec(&data, &cub->pos[0]);
	Proto_WriteSVec(&data, &cub->pos[1]);
	Proto_WriteColor4(&data, &cub->color);

	PacketWriter_End(client);
}

void CPE_WriteRemoveSelection(Client *client, cs_byte id) {
	PacketWriter_Start(client, 2);

	*data++ = PACKET_SELECTIONREMOVE;
	*data++ = id;

	PacketWriter_End(client);
}

void CPE_WriteBlockPerm(Client *client, BlockID id, cs_bool allowPlace, cs_bool allowDestroy) {
	PacketWriter_Start(client, 4);

	*data++ = PACKET_SETBLOCKPERMISSION;
	*data++ = id;
	*data++ = (cs_char)allowPlace;
	*data++ = (cs_char)allowDestroy;

	PacketWriter_End(client);
}

void CPE_WriteSetModel(Client *client, Client *other) {
	PacketWriter_Start(client, 66);

	*data++ = PACKET_ENTITYMODEL;
	*data++ = client == other ? CLIENT_SELF : other->id;

	cs_char model[64] = {0};
	if(CPE_GetModelStr(Client_GetModel(other), model, 64))
		Proto_WriteString(&data, model);
	else
		Proto_WriteString(&data, CPE_GetDefaultModelName());

	PacketWriter_End(client);
}

void CPE_WriteSetMapAppearanceV1(Client *client, cs_str tex, cs_byte side, cs_byte edge, cs_int16 sidelvl) {
	PacketWriter_Start(client, 69);

	*data++ = PACKET_SETMAPAPPEARANCE;
	Proto_WriteString(&data, tex);
	*data++ = side;
	*data++ = edge;
	*(cs_uint16 *)data = htons(sidelvl);
	data += 2;

	PacketWriter_End(client);
}

void CPE_WriteSetMapAppearanceV2(Client *client, cs_str tex, cs_byte side, cs_byte edge, cs_int16 sidelvl, cs_int16 cllvl, cs_int16 maxview) {
	PacketWriter_Start(client, 73);

	*data++ = PACKET_SETMAPAPPEARANCE;
	Proto_WriteString(&data, tex);
	*data++ = side;
	*data++ = edge;
	*(cs_uint16 *)data = htons(sidelvl); data += 2;
	*(cs_uint16 *)data = htons(cllvl); data += 2;
	*(cs_uint16 *)data = htons(maxview); data += 2;

	PacketWriter_End(client);
}

void CPE_WriteWeatherType(Client *client, cs_int8 type) {
	PacketWriter_Start(client, 2);

	*data++ = PACKET_SETENVWEATHER;
	*data++ = type;

	PacketWriter_End(client);
}

void CPE_WriteHackControl(Client *client, CPEHacks *hacks) {
	PacketWriter_Start(client, 8);

	*data++ = PACKET_UPDATEHACKS;
	*data++ = (cs_char)hacks->flying;
	*data++ = (cs_char)hacks->noclip;
	*data++ = (cs_char)hacks->speeding;
	*data++ = (cs_char)hacks->spawnControl;
	*data++ = (cs_char)hacks->tpv;
	*(cs_int16 *)data = htons(hacks->jumpHeight);
	data += 2;

	PacketWriter_End(client);
}

void CPE_WriteDefineBlock(Client *client, BlockID id, BlockDef *block) {
	PacketWriter_Start(client, 80);

	*data++ = PACKET_DEFINEBLOCK;
	*data++ = id;
	Proto_WriteString(&data, block->name);
	*(struct _BlockParams *)data = block->params.nonext;
	data += sizeof(block->params.nonext);

	PacketWriter_End(client);
}

void CPE_WriteUndefineBlock(Client *client, BlockID id) {
	PacketWriter_Start(client, 2);

	*data++ = PACKET_UNDEFINEBLOCK;
	*data++ = id;

	PacketWriter_End(client);
}

void CPE_WriteDefineExBlock(Client *client, BlockID id, BlockDef *block) {
	PacketWriter_Start(client, 88);

	*data++ = PACKET_DEFINEBLOCKEXT;
	*data++ = id;
	Proto_WriteString(&data, block->name);
	*(struct _BlockParamsExt *)data = block->params.ext;
	data += sizeof(block->params.ext);

	PacketWriter_End(client);
}

void CPE_WriteBulkBlockUpdate(Client *client, BulkBlockUpdate *bbu) {
	PacketWriter_Start(client, 1282);

	*data++ = PACKET_BULKBLOCKUPDATE;
	*(struct _BBUData *)data = bbu->data;
	// Клиенту нужно количество блоков, не индекс последнего
	(*(struct _BBUData *)data).count--;
	data += sizeof(bbu->data);

	PacketWriter_End(client);
}

void CPE_WriteFastMapInit(Client *client, cs_uint32 size) {
	PacketWriter_Start(client, 5);

	*data++ = PACKET_LEVELINIT;
	*(cs_uint32 *)data = htonl(size);
	data += 4;

	PacketWriter_End(client);
}

void CPE_WriteAddTextColor(Client *client, Color4* color, cs_char code) {
	PacketWriter_Start(client, 6);

	*data++ = PACKET_ADDTEXTCOLOR;
	Proto_WriteByteColor4(&data, color);
	*data++ = code;

	PacketWriter_End(client);
}

void CPE_WriteTexturePack(Client *client, cs_str url) {
	PacketWriter_Start(client, 65);

	*data++ = PACKET_ENVSETPACKURL;
	Proto_WriteString(&data, url);

	PacketWriter_End(client);
}

void CPE_WriteMapProperty(Client *client, cs_byte property, cs_int32 value) {
	PacketWriter_Start(client, 6);

	*data++ = PACKET_SETENVPROP;
	*data++ = property;
	*(cs_int32 *)data = htonl(value);
	data += 4;

	PacketWriter_End(client);
}

void CPE_WriteSetEntityProperty(Client *client, Client *other, EEntProp type, cs_int32 value) {
	PacketWriter_Start(client, 7);

	*data++ = PACKET_ENTITYPROPERTY;
	*data++ = client == other ? CLIENT_SELF : other->id;
	*data++ = (cs_byte)type;
	*(cs_int32 *)data = htonl(value);
	data += 4;

	PacketWriter_End(client);
}

void CPE_WriteTwoWayPing(Client *client, cs_byte direction, cs_int16 num) {
	PacketWriter_Start(client, 4);

	*data++ = PACKET_TWOWAYPING;
	*data++ = direction;
	*(cs_uint16 *)data = num;
	data += 2;

	PacketWriter_End(client);
}

void CPE_WriteInventoryOrder(Client *client, cs_byte order, BlockID block) {
	PacketWriter_Start(client, 3);

	*data++ = PACKET_SETINVORDER;
	*data++ = block;
	*data++ = order;

	PacketWriter_End(client);
}

void CPE_WriteSetHotBar(Client *client, cs_byte order, BlockID block) {
	PacketWriter_Start(client, 3);

	*data++ = PACKET_SETHOTBAR;
	*data++ = block;
	*data++ = order;

	PacketWriter_End(client);
}

void CPE_WriteSetSpawnPoint(Client *client, Vec *pos, Ang *ang) {
	PacketWriter_Start(client, 15);

	*data++ = PACKET_SETSPAWNPOINT;
	if(Client_GetExtVer(client, EXT_ENTPOS))
		Proto_WriteFlVec(&data, pos);
	else
		Proto_WriteFlSVec(&data, pos);
	Proto_WriteAng(&data, ang);

	PacketWriter_End(client);
}

void CPE_WriteVelocityControl(Client *client, Vec *velocity, cs_byte mode) {
	PacketWriter_Start(client, 16);

	*data++ = PACKET_SETVELOCITY;
	*(cs_int32 *)data = htonl((cs_int32)(velocity->x * 10000.0f)); data += 4;
	*(cs_int32 *)data = htonl((cs_int32)(velocity->y * 10000.0f)); data += 4;
	*(cs_int32 *)data = htonl((cs_int32)(velocity->z * 10000.0f)); data += 4;
	for(cs_int8 i = 2; i >= 0; i--) *data++ = (mode >> i) & 1;

	PacketWriter_End(client);
}

void CPE_WriteDefineEffect(Client *client, cs_byte id, CPEParticle *e) {
	PacketWriter_Start(client, 36);

	*data++ = PACKET_DEFINEEFFECT;
	*data++ = id;
	*(UVCoordsB *)data = e->rec; data += sizeof(UVCoordsB);
	*data++ = (cs_byte)e->tintCol.r;
	*data++ = (cs_byte)e->tintCol.g;
	*data++ = (cs_byte)e->tintCol.b;
	*data++ = e->frameCount;
	*data++ = e->particleCount;
	*data++ = e->size;
	*(cs_uint32 *)data = htonl((cs_uint32)(e->sizeVariation * 10000.0f)); data += 4;
	*(cs_uint16 *)data = htons((cs_uint16)(e->spread * 32.0f)); data += 2;
	*(cs_uint32 *)data = htonl((cs_uint32)(e->speed * 10000.0f)); data += 4;
	*(cs_uint32 *)data = htonl((cs_uint32)(e->gravity * 10000.0f)); data += 4;
	*(cs_uint32 *)data = htonl((cs_uint32)(e->baseLifetime * 10000.0f)); data += 4;
	*(cs_uint32 *)data = htonl((cs_uint32)(e->lifetimeVariation * 10000.0f)); data += 4;
	*data++ = e->collideFlags;
	*data++ = e->fullBright;

	PacketWriter_End(client);
}

void CPE_WriteSpawnEffect(Client *client, cs_byte id, Vec *pos, Vec *origin) {
	PacketWriter_Start(client, 26);

	*data++ = PACKET_SPAWNEFFECT;
	*data++ = id;
	Proto_WriteFlVec(&data, pos);
	Proto_WriteFlVec(&data, origin);

	PacketWriter_End(client);
}

void CPE_WriteDefineModel(Client *client, cs_byte id, CPEModel *model) {
	PacketWriter_Start(client, 116);

	*data++ = PACKET_DEFINEMODEL;
	*data++ = id;
	Proto_WriteString(&data, model->name);
	*data++ = model->flags;

	Proto_WriteFloat(&data, model->nameY);
	Proto_WriteFloat(&data, model->eyeY);

	Proto_WriteFloat(&data, model->collideBox.x);
	Proto_WriteFloat(&data, model->collideBox.y);
	Proto_WriteFloat(&data, model->collideBox.z);

	Proto_WriteFloat(&data, model->clickMin.x);
	Proto_WriteFloat(&data, model->clickMin.y);
	Proto_WriteFloat(&data, model->clickMin.z);

	Proto_WriteFloat(&data, model->clickMax.x);
	Proto_WriteFloat(&data, model->clickMax.y);
	Proto_WriteFloat(&data, model->clickMax.z);

	*(cs_uint16 *)data = htons(model->uScale); data += 2;
	*(cs_uint16 *)data = htons(model->vScale); data += 2;
	*data++ = model->partsCount;

	PacketWriter_End(client);
}

void CPE_WriteDefineModelPart(Client *client, cs_int32 ver, cs_byte id, CPEModelPart *part) {
	PacketWriter_Start(client, 167);

	*data++ = PACKET_DEFINEMODELPART;
	*data++ = id;

	Proto_WriteFloat(&data, part->minCoords.x);
	Proto_WriteFloat(&data, part->minCoords.y);
	Proto_WriteFloat(&data, part->minCoords.z);

	Proto_WriteFloat(&data, part->maxCoords.x);
	Proto_WriteFloat(&data, part->maxCoords.y);
	Proto_WriteFloat(&data, part->maxCoords.z);

	for(cs_int32 i = 0; i < 6; i++) {
		*(cs_uint16 *)data = htons(part->UVs[i].U1); data += 2;
		*(cs_uint16 *)data = htons(part->UVs[i].V1); data += 2;
		*(cs_uint16 *)data = htons(part->UVs[i].U2); data += 2;
		*(cs_uint16 *)data = htons(part->UVs[i].V2); data += 2;
	}

	Proto_WriteFloat(&data, part->rotOrigin.x);
	Proto_WriteFloat(&data, part->rotOrigin.y);
	Proto_WriteFloat(&data, part->rotOrigin.z);

	Proto_WriteFloat(&data, part->rotAngles.x);
	Proto_WriteFloat(&data, part->rotAngles.y);
	Proto_WriteFloat(&data, part->rotAngles.z);

	// Отправка анимаций происходит только если версия CustomModels равна 2
	for(cs_int32 i = 0; i < 4 && ver == 2; i++) {
		*data++ = part->anims[i].flags;

		Proto_WriteFloat(&data, part->anims[i].a);
		Proto_WriteFloat(&data, part->anims[i].b);
		Proto_WriteFloat(&data, part->anims[i].c);
		Proto_WriteFloat(&data, part->anims[i].d);
	}

	*data++ = part->flags;

	PacketWriter_End(client);
}

void CPE_WriteUndefineModel(Client *client, cs_byte id) {
	PacketWriter_Start(client, 2);

	*data++ = PACKET_UNDEFINEMODEL;
	*data++ = id;

	PacketWriter_End(client);
}

void CPE_WritePluginMessage(Client *client, cs_byte channel, cs_str message) {
	PacketWriter_Start(client, 66);

	*data++ = PACKET_PLUGINMESSAGE;
	*data++ = channel;
	Proto_WriteString(&data, message);

	PacketWriter_End(client);
}

cs_bool CPEHandler_ExtInfo(Client *client, cs_char *data) {
	ValidateCpeClient(client, false);
	ValidateClientState(client, CLIENT_STATE_MOTD, false);

	if(!Proto_ReadStringNoAlloc(&data, client->extData.appName)) {
		String_Copy(client->extData.appName, 65, "(unknown)");
		return true;
	}
	client->extData.extensions.count = ntohs(*(cs_uint16 *)data);
	if(client->extData.extensions.count > 512) return false;
	client->extData.extensions.list = Memory_TryAlloc(
		client->extData.extensions.count,
		sizeof(struct _CPEClExt)
	);
	return client->extData.extensions.list != NULL;
}

cs_bool CPEHandler_ExtEntry(Client *client, cs_char *data) {
	ValidateCpeClient(client, false);
	ValidateClientState(client, CLIENT_STATE_MOTD, false);
	struct _CPEClExts *exts = &client->extData.extensions;
	if(exts->current == exts->count) return false;
	cs_char tempname[65];
	if(!Proto_ReadStringNoAlloc(&data, tempname))
		return false;

	CPEClExt *ext = &exts->list[exts->current++];
	ext->version = ntohl(*(cs_int32 *)data);
	if(ext->version < 1) return false;
	ext->hash = Compr_CRC32((cs_byte*)tempname, (cs_uint32)String_Length(tempname));

	if(exts->current == exts->count) {
		if(Client_GetExtVer(client, EXT_CUSTOMBLOCKS)) {
			CPE_CustomBlockSupportLevel(client, 1);
			return true;
		}

		return FinishHandshake(client);
	}

	return true;
}

cs_bool CPEHandler_SetCBVer(Client *client, cs_char *data) {
	ValidateCpeClient(client, false);
	ValidateClientState(client, CLIENT_STATE_MOTD, false);
	if(Client_GetExtVer(client, EXT_CUSTOMBLOCKS) < 1) return false;
	client->extData.cbLevel = *data;
	return FinishHandshake(client);
}

cs_bool CPEHandler_PlayerClick(Client *client, cs_char *data) {
	if(Client_GetExtVer(client, EXT_PLAYERCLICK) < 1) return false;
	ValidateClientState(client, CLIENT_STATE_INGAME, true);

	onPlayerClick params;
	params.client = client;
	params.button = *data++;
	params.action = *data++;
	params.angle.yaw = (cs_float)((cs_int16)ntohs(*(cs_uint16 *)data)) / 32767.0f * 360.0f; data += 2;
	params.angle.pitch = (cs_float)((cs_int16)ntohs(*(cs_uint16 *)data)) / 32767.0f * 360.0f; data += 2;
	params.tgid = *data++;
	Proto_ReadSVec(&data, &params.tgpos);
	params.tgface = *data;
	Event_Call(EVT_ONCLICK, &params);

	return true;
}

cs_bool CPEHandler_TwoWayPing(Client *client, cs_char *data) {
	if(!Client_CheckState(client, CLIENT_STATE_INGAME)) return true;
	if(Client_GetExtVer(client, EXT_TWOWAYPING) < 1) return false;

	cs_byte pingDirection = *data++;
	cs_uint16 pingData = *(cs_uint16 *)data;

	if(pingDirection == 0) {
		CPE_WriteTwoWayPing(client, 0, pingData);
		if(!client->extData.pingStarted) {
			CPE_WriteTwoWayPing(client, 1, ++client->extData.pingData);
			client->extData.pingStarted = true;
			client->extData.pingStart = Time_GetMSec();
		}

		return true;
	} else if(pingDirection == 1) {
		if(client->extData.pingStarted) {
			client->extData.pingStarted = false;
			if(client->extData.pingData == pingData) {
				client->extData.pingTime = (cs_uint32)((Time_GetMSec() - client->extData.pingStart) / 2);
				client->extData.pingAvgTime = (client->extData._pingAvgSize * client->extData.pingAvgTime +
				(cs_float)client->extData.pingTime) / (client->extData._pingAvgSize + 1);
				client->extData._pingAvgSize += 1;
			}

			return true;
		}
	}

	return false;
}

cs_bool CPEHandler_PluginMessage(Client *client, cs_char *data) {
	if(Client_GetExtVer(client, EXT_PLUGINMESSAGE) < 1) return false;

	onPluginMessage pmesg = {
		.client = client,
		.channel = *data++
	};

	Proto_ReadStringNoAlloc(&data, pmesg.message);
	return Event_Call(EVT_ONPLUGINMESSAGE, &pmesg);
}

static Packet *packetsList[256] = {NULL};

cs_bool Packet_Register(EPacketID id, cs_uint16 size, packetHandler handler) {
	if(packetsList[id]) return false;

	Packet *tmp = (Packet *)Memory_Alloc(1, sizeof(Packet));
	tmp->id = id;
	tmp->size = size;
	tmp->handler = (void *)handler;
	packetsList[id] = tmp;
	
	return true;
}

cs_bool Packet_SetCPEHandler(EPacketID id, cs_uint32 hash, cs_int32 ver, cs_uint16 size, packetHandler handler) {
	Packet *tmp = packetsList[id];
	if(!tmp || tmp->exthash) return false;

	tmp->exthash = hash;
	tmp->extVersion = ver;
	tmp->extHandler = (void *)handler;
	tmp->extSize = size;
	tmp->haveCPEImp = true;

	return true;
}

void CPE_RegisterServerExtension(cs_str name, cs_int32 version) {
	CPESvExt *tmp = Memory_Alloc(1, sizeof(struct _CPESvExt));
	tmp->name = name;
	tmp->version = version;
	tmp->next = headExtension;
	headExtension = tmp;
	++extensionsCount;
}

static const struct extReg {
	cs_str name;
	cs_int32 version;
} serverExtensions[] = {
	{"ClickDistance", 1},
	{"CustomBlocks", 1},
	{"HeldBlock", 1},
	{"EmoteFix", 1},
	{"TextHotKey", 1},
	{"ExtPlayerList", 2},
	{"EnvColors", 1},
	{"SelectionCuboid", 1},
	{"BlockPermissions", 1},
	{"ChangeModel", 1},
	{"EnvMapAppearance", 2},
	{"EnvWeatherType", 1},
	{"HackControl", 1},
	{"MessageTypes", 1},
	{"PlayerClick", 1},
	{"LongerMessages", 1},
	{"FullCP437", 1},
	{"BlockDefinitions", 1},
	{"BlockDefinitionsExt", 2},
	{"BulkBlockUpdate", 1},
	{"TextColors", 1},
	{"EnvMapAspect", 1},
	{"EntityProperty", 1},
	{"ExtEntityPositions", 1},
	{"TwoWayPing", 1},
	{"InventoryOrder", 1},
	// {"ExtendedBlocks", 1},
	{"FastMap", 1},
	// {"ExtendedTextures", 1},
	{"SetHotbar", 1},
	{"SetSpawnpoint", 1},
	{"VelocityControl", 1},
	{"CustomParticles", 1},
	{"CustomModels", 2},
	{"PluginMessages", 1},

	{NULL, 0}
};

Packet *Packet_Get(EPacketID id) {
	return packetsList[id];
}

void Packet_RegisterDefault(void) {
	(void)Packet_Register(PACKET_IDENTIFICATION,  130, Handler_Handshake);
	(void)Packet_Register(PACKET_SETBLOCK_CLIENT, 8, Handler_SetBlock);
	(void)Packet_Register(PACKET_ENTITYTELEPORT,  9, Handler_PosAndOrient);
	(void)Packet_Register(PACKET_SENDMESSAGE,     65, Handler_Message);

	const struct extReg *ext;
	for(ext = serverExtensions; ext->name; ext++)
		CPE_RegisterServerExtension(ext->name, ext->version);

	(void)Packet_Register(PACKET_EXTINFO,         66, CPEHandler_ExtInfo);
	(void)Packet_Register(PACKET_EXTENTRY,        68, CPEHandler_ExtEntry);
	(void)Packet_Register(PACKET_CUSTOMBLOCKSLVL, 1, CPEHandler_SetCBVer);
	(void)Packet_Register(PACKET_TWOWAYPING,      3, CPEHandler_TwoWayPing);
	(void)Packet_Register(PACKET_PLAYERCLICKED,   14, CPEHandler_PlayerClick);
	(void)Packet_Register(PACKET_PLUGINMESSAGE,   65, CPEHandler_PluginMessage);
	(void)Packet_SetCPEHandler(PACKET_ENTITYTELEPORT, EXT_ENTPOS, 1, 15, NULL);
}

void Packet_UnregisterAll(void) {
	while(headExtension != NULL) {
		CPESvExt *tmp = headExtension;
		headExtension = headExtension->next;
		Memory_Free(tmp);
	}

	for(cs_int32 i = 0; i < 255; i++) {
		Packet *packet = packetsList[i];
		if(packet) {
			Memory_Free(packet);
			packetsList[i] = NULL;
		}
	}
}
