#include "core.h"
#include "str.h"
#include "log.h"
#include "block.h"
#include "client.h"
#include "event.h"
#include "server.h"
#include "protocol.h"
#include "platform.h"
#include "command.h"
#include "lang.h"
#include <zlib.h>

cs_uint16 extensionsCount;
CPEExt *headExtension;

void Proto_WriteString(cs_char **dataptr, cs_str string) {
	cs_size size = 0;
	if(string)
		size = min(String_Length(string), 64);
	for(cs_byte i = 0; i < 64; i++)
		*(*dataptr)++ = i < size ? string[i] : ' ';
}

void Proto_WriteFlVec(cs_char **dataptr, const Vec *vec) {
	cs_char *data = *dataptr;
	*(cs_int32 *)data = htonl((cs_int32)(vec->x * 32)); data += 4;
	*(cs_int32 *)data = htonl((cs_int32)(vec->y * 32)); data += 4;
	*(cs_int32 *)data = htonl((cs_int32)(vec->z * 32)); data += 4;
	*dataptr = data;
}

void Proto_WriteFlSVec(cs_char **dataptr, const Vec *vec) {
	cs_char *data = *dataptr;
	*(cs_int16 *)data = htons((cs_int16)(vec->x * 32)); data += 2;
	*(cs_int16 *)data = htons((cs_int16)(vec->y * 32)); data += 2;
	*(cs_int16 *)data = htons((cs_int16)(vec->z * 32)); data += 2;
	*dataptr = data;
}

void Proto_WriteSVec(cs_char **dataptr, const SVec *vec) {
	cs_char *data = *dataptr;
	*(cs_int16 *)data = htons(vec->x); data += 2;
	*(cs_int16 *)data = htons(vec->y); data += 2;
	*(cs_int16 *)data = htons(vec->z); data += 2;
	*dataptr = data;
}

void Proto_WriteAng(cs_char **dataptr, const Ang *ang) {
	cs_char *data = *dataptr;
	*(cs_byte *)data++ = (cs_byte)((ang->yaw / 360) * 256);
	*(cs_byte *)data++ = (cs_byte)((ang->pitch / 360) * 256);
	*dataptr = data;
}

void Proto_WriteColor3(cs_char **dataptr, const Color3* color) {
	cs_char *data = *dataptr;
	*(cs_int16 *)data = htons(color->r); data += 2;
	*(cs_int16 *)data = htons(color->g); data += 2;
	*(cs_int16 *)data = htons(color->b); data += 2;
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

NOINL static cs_uint32 WriteExtEntityPos(cs_char **dataptr, Vec *vec, Ang *ang, cs_bool extended) {
	if(extended)
		Proto_WriteFlVec(dataptr, vec);
	else
		Proto_WriteFlSVec(dataptr, vec);
	Proto_WriteAng(dataptr, ang);
	return extended ? 12 : 6;
}

cs_uint32 Proto_WriteClientPos(cs_char *data, Client *client, cs_bool extended) {
	PlayerData *pd = client->playerData;
	return WriteExtEntityPos(&data, &pd->position, &pd->angle, extended);
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
	ang->yaw = (((cs_float)*data++) / 256) * 360;
	ang->pitch = (((cs_float)*data++) / 256) * 360;
	*dataptr = data;
}

void Proto_ReadFlSVec(cs_char **dataptr, Vec *vec) {
	cs_char *data = *dataptr;
	vec->x = (cs_float)ntohs(*(cs_int16 *)data) / 32; data += 2;
	vec->y = (cs_float)ntohs(*(cs_int16 *)data) / 32; data += 2;
	vec->z = (cs_float)ntohs(*(cs_int16 *)data) / 32; data += 2;
	*dataptr = data;
}

void Proto_ReadFlVec(cs_char **dataptr, Vec *vec) {
	cs_char *data = *dataptr;
	vec->x = (cs_float)ntohl(*(cs_int32 *)data) / 32; data += 4;
	vec->y = (cs_float)ntohl(*(cs_int32 *)data) / 32; data += 4;
	vec->z = (cs_float)ntohl(*(cs_int32 *)data) / 32; data += 4;
	*dataptr = data;
}

cs_bool Proto_ReadClientPos(Client *client, cs_char *data) {
	PlayerData *cpd = client->playerData;
	Vec *vec = &cpd->position, newVec;
	Ang *ang = &cpd->angle, newAng;
	cs_bool changed = false;

	if(Client_GetExtVer(client, EXT_ENTPOS))
		Proto_ReadFlVec(&data, &newVec);
	else
		Proto_ReadFlSVec(&data, &newVec);

	Proto_ReadAng(&data, &newAng);

	if(newVec.x != vec->x || newVec.y != vec->y || newVec.z != vec->z) {
		cpd->position = newVec;
		Event_Call(EVT_ONMOVE, client);
		changed = true;
	}

	if(newAng.yaw != ang->yaw || newAng.pitch != ang->pitch) {
		cpd->angle = newAng;
		Event_Call(EVT_ONROTATE, client);
		changed = true;
	}

	return changed;
}

void Vanilla_WriteServerIdent(Client *client, cs_str name, cs_str motd) {
	PacketWriter_Start(client);

	*data++ = 0x00;
	*data++ = 0x07;
	Proto_WriteString(&data, name);
	Proto_WriteString(&data, motd);
	*data = 0x00;

	PacketWriter_End(client, 131);
}

void Vanilla_WriteLvlInit(Client *client) {
	PacketWriter_Start(client);

	*data++ = 0x02;

	PacketWriter_End(client, 1);
}

void Vanilla_WriteLvlFin(Client *client, SVec *dims) {
	PacketWriter_Start(client);

	*data++ = 0x04;
	Proto_WriteSVec(&data, dims);

	PacketWriter_End(client, 7);
}

void Vanilla_WriteSetBlock(Client *client, SVec *pos, BlockID block) {
	PacketWriter_Start(client);

	*data++ = 0x06;
	Proto_WriteSVec(&data, pos);
	*data++ = block;

	PacketWriter_End(client, 8);
}

void Vanilla_WriteSpawn(Client *client, Client *other) {
	PacketWriter_Start(client);

	*data++ = 0x07;
	*data++ = client == other ? CLIENT_SELF : other->id;
	Proto_WriteString(&data, other->playerData->name);
	cs_bool extended = Client_GetExtVer(client, EXT_ENTPOS) != 0;
	cs_uint32 len = Proto_WriteClientPos(data, other, extended);

	PacketWriter_End(client, 68 + len);
}

void Vanilla_WriteTeleport(Client *client, Vec *pos, Ang *ang) {
	PacketWriter_Start(client);

	*data++ = 0x08;
	*data++ = CLIENT_SELF;
	cs_bool extended = Client_GetExtVer(client, EXT_ENTPOS) != 0;
	cs_uint32 len = WriteExtEntityPos(&data, pos, ang, extended);

	PacketWriter_End(client, 4 + len);
}

void Vanilla_WritePosAndOrient(Client *client, Client *other) {
	PacketWriter_Start(client);

	*data++ = 0x08;
	*data++ = client == other ? CLIENT_SELF : other->id;
	cs_bool extended = Client_GetExtVer(client, EXT_ENTPOS) != 0;
	cs_uint32 len = Proto_WriteClientPos(data, other, extended);

	PacketWriter_End(client, 4 + len);
}

void Vanilla_WriteDespawn(Client *client, Client *other) {
	PacketWriter_Start(client);

	*data++ = 0x0C;
	*data++ = client == other ? CLIENT_SELF : other->id;

	PacketWriter_End(client, 2);
}

void Vanilla_WriteChat(Client *client, cs_byte type, cs_str mesg) {
	PacketWriter_Start(client);
	if(client == Broadcast) {
		for(ClientID i = 0; i < MAX_CLIENTS; i++) {
			Client *tg = Clients_List[i];
			if(tg) Vanilla_WriteChat(tg, type, mesg);
		}
		PacketWriter_Stop(client);
		return;
	}

	cs_char mesg_out[64] = {0};
	String_Copy(mesg_out, 64, mesg);

	if(!Client_GetExtVer(client, EXT_CP437)) {
		for(cs_int32 i = 0; i < 64; i++) {
			if(mesg_out[i] == '\0') break;
			if(mesg_out[i] < ' ' || mesg_out[i] > '~')
				mesg_out[i] = '?';
		}
	}

	*data++ = 0x0D;
	*data++ = type;
	Proto_WriteString(&data, mesg_out);

	PacketWriter_End(client, 66);
}

void Vanilla_WriteKick(Client *client, cs_str reason) {
	PacketWriter_Start(client);

	*data++ = 0x0E;
	Proto_WriteString(&data, reason);

	PacketWriter_End(client, 65);
}

cs_bool Handler_Handshake(Client *client, cs_char *data) {
	if(client->playerData) return false;
	if(*data++ != 0x07) {
		Client_Kick(client, Lang_Get(Lang_KickGrp, 2));
		return true;
	}

	client->playerData = Memory_Alloc(1, sizeof(PlayerData));
	client->playerData->firstSpawn = true;
	if(client->addr == htonl(INADDR_LOOPBACK) && Config_GetBoolByKey(Server_Config, CFG_LOCALOP_KEY))
		client->playerData->isOP = true;

	if(!Proto_ReadString(&data, &client->playerData->name)) return false;
	if(!Proto_ReadString(&data, &client->playerData->key)) return false;

	for(ClientID i = 0; i < MAX_CLIENTS; i++) {
		Client *other = Clients_List[i];
		if(!other || !other->playerData || other == client) continue;
		if(String_CaselessCompare(client->playerData->name, other->playerData->name)) {
			Client_Kick(client, Lang_Get(Lang_KickGrp, 3));
			return true;
		}
	}

	if(Client_CheckAuth(client)) {
		Client_SetServerIdent(client,
			Config_GetStrByKey(Server_Config, CFG_SERVERNAME_KEY),
			Config_GetStrByKey(Server_Config, CFG_SERVERMOTD_KEY)
		);
	} else {
		Client_Kick(client, Lang_Get(Lang_KickGrp, 4));
		return true;
	}

	if(*data == 0x42) {
		client->cpeData = Memory_Alloc(1, sizeof(CPEData));
		client->cpeData->model = 256; // Humanoid model id

		CPE_WriteInfo(client);
		CPEExt *ptr = headExtension;
		while(ptr) {
			CPE_WriteExtEntry(client, ptr);
			ptr = ptr->next;
		}
	} else {
		Event_Call(EVT_ONHANDSHAKEDONE, client);
		Client_ChangeWorld(client, (World *)World_Head->value.ptr);
	}

	return true;
}

static void UpdateBlock(World *world, SVec *pos, BlockID block) {
	for(ClientID i = 0; i < MAX_CLIENTS; i++) {
		Client *client = Clients_List[i];
		if(client && Client_IsInGame(client) && Client_IsInWorld(client, world))
			Vanilla_WriteSetBlock(client, pos, block);
	}
}

cs_bool Handler_SetBlock(Client *client, cs_char *data) {
	ValidateClientState(client, STATE_INGAME, false)

	World *world = Client_GetWorld(client);
	if(!world) return false;

	SVec pos;
	Proto_ReadSVec(&data, &pos);
	cs_byte mode = *data++;
	BlockID block = *data;

	switch(mode) {
		case 0x01:
			if(!Block_IsValid(block)) {
				Client_Kick(client, Lang_Get(Lang_KickGrp, 8));
				return false;
			}
			if(Event_OnBlockPlace(client, mode, &pos, &block)) {
				World_SetBlock(world, &pos, block);
				UpdateBlock(world, &pos, block);
			} else
				Vanilla_WriteSetBlock(client, &pos, World_GetBlock(world, &pos));
			break;
		case 0x00:
			block = BLOCK_AIR;
			if(Event_OnBlockPlace(client, mode, &pos, &block)) {
				World_SetBlock(world, &pos, block);
				UpdateBlock(world, &pos, block);
			} else
				Vanilla_WriteSetBlock(client, &pos, World_GetBlock(world, &pos));
			break;
	}

	return true;
}

cs_bool Handler_PosAndOrient(Client *client, cs_char *data) {
	ValidateClientState(client, STATE_INGAME, false)

	CPEData *cpd = client->cpeData;
	BlockID cb = *data++;

	if(cpd && cpd->heldBlock != cb) {
		Event_OnHeldBlockChange(client, cpd->heldBlock, cb);
		cpd->heldBlock = cb;
	}

	if(Proto_ReadClientPos(client, data)) {
		for(ClientID i = 0; i < MAX_CLIENTS; i++) {
			Client *other = Clients_List[i];
			if(other && client != other && Client_IsInGame(other) && Client_IsInSameWorld(client, other))
				Vanilla_WritePosAndOrient(other, client);
		}
	}
	return true;
}

cs_bool Handler_Message(Client *client, cs_char *data) {
	ValidateClientState(client, STATE_INGAME, true)

	cs_char message[65],
	*messptr = message;
	cs_byte type = 0,
	partial = *data++,
	len = Proto_ReadStringNoAlloc(&data, message);
	if(len == 0) return false;

	for(cs_byte i = 0; i < len; i++) {
		if(message[i] == '%' && ISHEX(message[i + 1]))
			message[i] = '&';
	}

	CPEData *cpd = client->cpeData;
	if(cpd && Client_GetExtVer(client, EXT_LONGMSG)) {
		if(!cpd->message) cpd->message = Memory_Alloc(1, 193);
		if(String_Append(cpd->message, 193, message) && partial == 1) return true;
		messptr = cpd->message;
	}

	if(Event_OnMessage(client, messptr, &type)) {
		cs_char formatted[320];
		String_FormatBuf(formatted, 320, "<%s>: %s", Client_GetName(client), messptr);

		if(*messptr == '/') {
			if(!Command_Handle(messptr, client))
				Vanilla_WriteChat(client, type, Lang_Get(Lang_CmdGrp, 3));
		} else
			Client_Chat(Broadcast, type, formatted);

		Log_Chat(formatted);
	}

	if(messptr != message) *messptr = '\0';

	return true;
}

/*
** Врайтеры и хендлеры
** CPE протокола
*/
#define MODELS_COUNT 14

static cs_str validModelNames[MODELS_COUNT] = {
	"humanoid",
	"chicken",
	"creeper",
	"pig",
	"sheep",
	"skeleton",
	"sheep",
	"sheep_nofur",
	"skeleton",
	"spider",
	"zombie",
	"head",
	"sit",
	"chibi"
};

cs_bool CPE_CheckModel(cs_int16 model) {
	if(model < 256) return Block_IsValid((BlockID)model);
	return model - 256 < MODELS_COUNT;
}

cs_int16 CPE_GetModelNum(cs_str model) {
	cs_int16 modelnum = -1;
	for(cs_int16 i = 0; validModelNames[i]; i++) {
		cs_str cmdl = validModelNames[i];
		if(String_CaselessCompare(model, cmdl)) {
			modelnum = i + 256;
			break;
		}
	}
	if(modelnum == -1) {
		cs_int32 tmp = String_ToInt(model);
		if(tmp < 0 || tmp > 255)
			modelnum = 256;
		else
			modelnum = (cs_int16)tmp;
	}

	return modelnum;
}

cs_str CPE_GetModelStr(cs_int16 num) {
	return num >= 0 && num < MODELS_COUNT ? validModelNames[num] : NULL;
}

void CPE_WriteInfo(Client *client) {
	PacketWriter_Start(client);

	*data++ = 0x10;
	Proto_WriteString(&data, SOFTWARE_FULLNAME);
	*(cs_uint16 *)data = htons(extensionsCount);

	PacketWriter_End(client, 67);
}

void CPE_WriteExtEntry(Client *client, CPEExt *ext) {
	PacketWriter_Start(client);

	*data++ = 0x11;
	Proto_WriteString(&data, ext->name);
	*(cs_uint32 *)data = htonl(ext->version);

	PacketWriter_End(client, 69);
}

void CPE_WriteClickDistance(Client *client, cs_int16 dist) {
	PacketWriter_Start(client);

	*data++ = 0x12;
	*(cs_int16 *)data = dist;

	PacketWriter_End(client, 3);
}

// 0x13 - CustomBlocksSupportLevel

void CPE_WriteHoldThis(Client *client, BlockID block, cs_bool preventChange) {
	PacketWriter_Start(client);

	*data++ = 0x14;
	*data++ = block;
	*data   = (cs_char)preventChange;

	PacketWriter_End(client, 3);
}

void CPE_WriteSetHotKey(Client *client, cs_str action, cs_int32 keycode, cs_int8 keymod) {
	PacketWriter_Start(client);

	*data++ = 0x15;
	Proto_WriteString(&data, NULL); // Label
	Proto_WriteString(&data, action);
	*(cs_int32 *)data = htonl(keycode); data += 4;
	*data = keymod;

	PacketWriter_End(client, 134);
}

void CPE_WriteAddName(Client *client, Client *other) {
	PacketWriter_Start(client);

	*data = 0x16; data += 2;
	*data++ = client == other ? CLIENT_SELF : other->id;
	Proto_WriteString(&data, Client_GetName(other));
	Proto_WriteString(&data, Client_GetName(other));
	CGroup *group = Client_GetGroup(other);
	Proto_WriteString(&data, group->name);
	*data = group->rank;

	PacketWriter_End(client, 196);
}

void CPE_WriteAddEntity2(Client *client, Client *other) {
	PacketWriter_Start(client);

	*data++ = 0x21;
	*data++ = client == other ? CLIENT_SELF : other->id;
	if(other->cpeData && other->cpeData->hideDisplayName)
		Proto_WriteString(&data, NULL);
	else
		Proto_WriteString(&data, Client_GetName(other));
	Proto_WriteString(&data, Client_GetSkin(other));
	cs_bool extended = Client_GetExtVer(client, EXT_ENTPOS) != 0;
	cs_uint32 len = Proto_WriteClientPos(data, other, extended);

	PacketWriter_End(client, 132 + len);
}

void CPE_WriteRemoveName(Client *client, Client *other) {
	PacketWriter_Start(client);

	*data = 0x18; data += 2;
	*data = client == other ? CLIENT_SELF : other->id;

	PacketWriter_End(client, 3);
}

void CPE_WriteEnvColor(Client *client, cs_byte type, Color3* col) {
	PacketWriter_Start(client);

	*data++ = 0x19;
	*data++ = type;
	Proto_WriteColor3(&data, col);

	PacketWriter_End(client, 8);
}

void CPE_WriteMakeSelection(Client *client, cs_byte id, SVec *start, SVec *end, Color4* color) {
	PacketWriter_Start(client);

	*data++ = 0x1A;
	*data++ = id;
	Proto_WriteString(&data, NULL); // Label
	Proto_WriteSVec(&data, start);
	Proto_WriteSVec(&data, end);
	Proto_WriteColor4(&data, color);

	PacketWriter_End(client, 86);
}

void CPE_WriteRemoveSelection(Client *client, cs_byte id) {
	PacketWriter_Start(client);

	*data++ = 0x1B;
	*data   = id;

	PacketWriter_End(client, 2);
}

void CPE_WriteBlockPerm(Client *client, BlockID id, cs_bool allowPlace, cs_bool allowDestroy) {
	PacketWriter_Start(client);

	*data++ = 0x1C;
	*data++ = id;
	*data++ = (cs_char)allowPlace;
	*data   = (cs_char)allowDestroy;

	PacketWriter_End(client, 4);
}

void CPE_WriteSetModel(Client *client, Client *other) {
	PacketWriter_Start(client);

	*data++ = 0x1D;
	*data++ = client == other ? CLIENT_SELF : other->id;
	cs_int16 model = Client_GetModel(other);
	if(model < 256) {
		cs_char modelname[4];
		String_FormatBuf(modelname, 4, "%d", model);
		Proto_WriteString(&data, modelname);
	} else
		Proto_WriteString(&data, CPE_GetModelStr(model - 256));

	PacketWriter_End(client, 66);
}

void CPE_WriteSetMapAppearanceV1(Client *client, cs_str tex, cs_byte side, cs_byte edge, cs_int16 sidelvl) {
	PacketWriter_Start(client);

	*data++ = 0x1E;
	Proto_WriteString(&data, tex);
	*data++ = side;
	*data++ = edge;
	*(cs_uint16 *)data = htons(sidelvl);

	PacketWriter_End(client, 69);
}

void CPE_WriteSetMapAppearanceV2(Client *client, cs_str tex, cs_byte side, cs_byte edge, cs_int16 sidelvl, cs_int16 cllvl, cs_int16 maxview) {
	PacketWriter_Start(client);

	*data++ = 0x1E;
	Proto_WriteString(&data, tex);
	*data++ = side;
	*data++ = edge;
	*(cs_uint16 *)data = htons(sidelvl); data += 2;
	*(cs_uint16 *)data = htons(cllvl); data += 2;
	*(cs_uint16 *)data = htons(maxview);

	PacketWriter_End(client, 73);
}

void CPE_WriteWeatherType(Client *client, cs_int8 type) {
	PacketWriter_Start(client);

	*data++ = 0x1F;
	*data   = type;

	PacketWriter_End(client, 2);
}

void CPE_WriteHackControl(Client *client, CPEHacks *hacks) {
	PacketWriter_Start(client);

	*data++ = 0x20;
	*data++ = (cs_char)hacks->flying;
	*data++ = (cs_char)hacks->noclip;
	*data++ = (cs_char)hacks->speeding;
	*data++ = (cs_char)hacks->spawnControl;
	*data++ = (cs_char)hacks->tpv;
	*(cs_int16 *)data = htons(hacks->jumpHeight);

	PacketWriter_End(client, 8);
}

void CPE_WriteDefineBlock(Client *client, BlockDef *block) {
	PacketWriter_Start(client);

	*data++ = 0x23;
	*data++ = block->id;
	Proto_WriteString(&data, block->name);
	*(struct _BlockParams *)data = block->params.nonext;

	PacketWriter_End(client, 80);
}

void CPE_WriteUndefineBlock(Client *client, BlockID id) {
	PacketWriter_Start(client);

	*data++ = 0x24;
	*data = id;

	PacketWriter_End(client, 2);
}

void CPE_WriteDefineExBlock(Client *client, BlockDef *block) {
	PacketWriter_Start(client);

	*data++ = 0x25;
	*data++ = block->id;
	Proto_WriteString(&data, block->name);
	*(struct _BlockParamsExt *)data = block->params.ext;

	PacketWriter_End(client, 88);
}

void CPE_WriteBulkBlockUpdate(Client *client, BulkBlockUpdate *bbu) {
	PacketWriter_Start(client);

	*data++ = 0x26;
	*(struct _BBUData *)data = bbu->data;

	PacketWriter_End(client, 1282);
}

void CPE_WriteFastMapInit(Client *client, cs_uint32 size) {
	PacketWriter_Start(client);

	*data++ = 0x02;
	*(cs_uint32 *)data = htonl(size);

	PacketWriter_End(client, 5);
}

void CPE_WriteAddTextColor(Client *client, Color4* color, cs_char code) {
	PacketWriter_Start(client);

	*data++ = 0x27;
	Proto_WriteByteColor4(&data, color);
	*data = code;

	PacketWriter_End(client, 6);
}

void CPE_WriteTexturePack(Client *client, cs_str url) {
	PacketWriter_Start(client);

	*data++ = 0x28;
	Proto_WriteString(&data, url);

	PacketWriter_End(client, 65);
}

void CPE_WriteMapProperty(Client *client, cs_byte property, cs_int32 value) {
	PacketWriter_Start(client);

	*data++ = 0x29;
	*data++ = property;
	*(cs_int32 *)data = htonl(value);

	PacketWriter_End(client, 6);
}

void CPE_WriteSetEntityProperty(Client *client, Client *other, cs_int8 type, cs_int32 value) {
	PacketWriter_Start(client);

	*data++ = 0x2A;
	*data++ = client == other ? CLIENT_SELF : other->id;
	*data++ = type;
	*(cs_int32 *)data = htonl(value);

	PacketWriter_End(client, 7);
}

void CPE_WriteTwoWayPing(Client *client, cs_byte direction, cs_int16 num) {
	PacketWriter_Start(client);

	*data++ = 0x2B;
	*data++ = direction;
	*(cs_uint16 *)data = num;

	PacketWriter_End(client, 4);
}

void CPE_WriteInventoryOrder(Client *client, cs_byte order, BlockID block) {
	PacketWriter_Start(client);

	*data++ = 0x2C;
	*data++ = block;
	*data   = order;

	PacketWriter_End(client, 3);
}

void CPE_WriteSetHotBar(Client *client, cs_byte order, BlockID block) {
	PacketWriter_Start(client);

	*data++ = 0x2D;
	*data++ = block;
	*data   = order;

	PacketWriter_End(client, 3);
}

void CPE_WriteSetSpawnPoint(Client *client, Vec *pos, Ang *ang) {
	PacketWriter_Start(client);

	cs_int32 len;

	*data++ = 0x2E;
	if(Client_GetExtVer(client, EXT_ENTPOS)) {
		Proto_WriteFlVec(&data, pos);
		len = 15;
	} else {
		Proto_WriteFlSVec(&data, pos);
		len = 9;
	}
	Proto_WriteAng(&data, ang);

	PacketWriter_End(client, len);
}

void CPE_WriteVelocityControl(Client *client, Vec *velocity, cs_bool mode) {
	PacketWriter_Start(client);

	*data++ = 0x2F;
	Proto_WriteFlVec(&data, velocity);
	*(cs_uint32 *)data = mode ? 0x01010101 : 0x00000000; // Why not?

	PacketWriter_End(client, 16);
}

void CPE_WriteDefineEffect(Client *client, CustomParticle *e) {
	PacketWriter_Start(client);

	*data++ = 0x30;
	*(struct TextureRec *)data = e->rec;
	*data++ = e->frameCount;
	*data++ = e->particleCount;
	*data++ = (cs_byte)(e->size * 32.0f);
	*(cs_uint32 *)data = htonl((cs_uint32)(e->sizeVariation * 1000.0f)); data += 4;
	*(cs_uint16 *)data = htons((cs_uint16)(e->spread * 32.0f)); data += 2;
	*(cs_uint32 *)data = htonl((cs_uint32)(e->speed * 10000.0f)); data += 4;
	*(cs_uint32 *)data = htonl((cs_uint32)(e->gravity * 10000.0f)); data += 4;
	*(cs_uint32 *)data = htonl((cs_uint32)(e->baseLifetime * 10000.0f)); data += 4;
	*(cs_uint32 *)data = htonl((cs_uint32)(e->lifetimeVariation * 10000.0f)); data += 4;
	*data++ = e->collideFlags;
	*data = e->fullBright;

	PacketWriter_End(client, 36);
}

void CPE_WriteSpawnEffect(Client *client, cs_byte id, Vec *pos, Vec *origin) {
	PacketWriter_Start(client);

	*data++ = 0x31;
	*data++ = id;
	Proto_WriteFlVec(&data, pos);
	Proto_WriteFlVec(&data, origin);

	PacketWriter_End(client, 26);
}

cs_bool CPEHandler_ExtInfo(Client *client, cs_char *data) {
	ValidateCpeClient(client, false)
	ValidateClientState(client, STATE_INITIAL, false)

	if(!Proto_ReadString(&data, &client->cpeData->appName)) return false;
	client->cpeData->_extCount = ntohs(*(cs_uint16 *)data);
	return true;
}

cs_bool CPEHandler_ExtEntry(Client *client, cs_char *data) {
	ValidateCpeClient(client, false)
	ValidateClientState(client, STATE_INITIAL, false)

	CPEData *cpd = client->cpeData;
	CPEExt *tmp = Memory_Alloc(1, sizeof(struct _CPEExt));
	if(!Proto_ReadString(&data, &tmp->name)) {
		Memory_Free(tmp);
		return false;
	}

	tmp->version = ntohl(*(cs_int32 *)data);
	if(tmp->version < 1) {
		Memory_Free((void *)tmp->name);
		Memory_Free(tmp);
		return false;
	}

	tmp->hash = crc32(0, (cs_byte*)tmp->name, (cs_uint32)String_Length(tmp->name));
	tmp->next = cpd->headExtension;
	cpd->headExtension = tmp;

	if(--cpd->_extCount == 0) {
		Event_Call(EVT_ONHANDSHAKEDONE, client);
		Client_ChangeWorld(client, (World *)World_Head->value.ptr);
	}

	return true;
}

cs_bool CPEHandler_PlayerClick(Client *client, cs_char *data) {
	ValidateCpeClient(client, false)
	ValidateClientState(client, STATE_INGAME, false)

	cs_char button = *data++, action = *data++;
	cs_int16 yaw = ntohs(*(cs_int16 *)data); data += 2;
	cs_int16 pitch = ntohs(*(cs_int16 *)data); data += 2;
	ClientID tgID = *data++;
	SVec tgBlockPos;
	Proto_ReadSVec(&data, &tgBlockPos);
	cs_char tgBlockFace = *data;

	Event_OnClick(
		client, button,
		action, yaw,
		pitch, tgID,
		&tgBlockPos,
		tgBlockFace
	);

	return true;
}

cs_bool CPEHandler_TwoWayPing(Client *client, cs_char *data) {
	ValidateCpeClient(client, false)

	CPEData *cpd = client->cpeData;
	cs_byte pingDirection = *data++;
	cs_uint16 pingData = *(cs_uint16 *)data;

	if(pingDirection == 0) {
		CPE_WriteTwoWayPing(client, 0, pingData);
		if(!cpd->pingStarted) {
			CPE_WriteTwoWayPing(client, 1, cpd->pingData++);
			cpd->pingStarted = true;
			cpd->pingStart = Time_GetMSec();
		}
		return true;
	} else if(pingDirection == 1) {
		if(cpd->pingStarted) {
			cpd->pingStarted = false;
			if(cpd->pingData == pingData)
				cpd->pingTime = (cs_uint32)((Time_GetMSec() - cpd->pingStart) / 2);
			return true;
		}
	}
	return false;
}

Packet *packetsList[256];

void Packet_Register(cs_byte id, cs_uint16 size, packetHandler handler) {
	Packet *tmp = (Packet *)Memory_Alloc(1, sizeof(Packet));
	tmp->id = id;
	tmp->size = size;
	tmp->handler = handler;
	packetsList[id] = tmp;
}

void Packet_RegisterCPE(cs_byte id, cs_uint32 hash, cs_int32 ver, cs_uint16 size, packetHandler handler) {
	Packet *tmp = packetsList[id];
	tmp->exthash = hash;
	tmp->extVersion = ver;
	tmp->cpeHandler = handler;
	tmp->extSize = size;
	tmp->haveCPEImp = true;
}

void Packet_RegisterExtension(cs_str name, cs_int32 version) {
	CPEExt *tmp = Memory_Alloc(1, sizeof(struct _CPEExt));
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
	// {"CustomBlocks", 1},
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
	// {"CustomModels", 2},

	{NULL, 0}
};

Packet *Packet_Get(cs_byte id) {
	return packetsList[id];
}

void Packet_RegisterDefault(void) {
	Packet_Register(0x00, 130, Handler_Handshake);
	Packet_Register(0x05,   8, Handler_SetBlock);
	Packet_Register(0x08,   9, Handler_PosAndOrient);
	Packet_Register(0x0D,  65, Handler_Message);

	const struct extReg *ext;
	for(ext = serverExtensions; ext->name; ext++) {
		Packet_RegisterExtension(ext->name, ext->version);
	}

	Packet_Register(0x10, 66, CPEHandler_ExtInfo);
	Packet_Register(0x11, 68, CPEHandler_ExtEntry);
	Packet_Register(0x2B,  3, CPEHandler_TwoWayPing);
	Packet_Register(0x22, 14, CPEHandler_PlayerClick);
	Packet_RegisterCPE(0x08, EXT_ENTPOS, 1, 15, NULL);
}
