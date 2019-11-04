#include "core.h"
#include "str.h"
#include "log.h"
#include "platform.h"
#include "block.h"
#include "client.h"
#include "server.h"
#include "protocol.h"
#include "event.h"
#include "heartbeat.h"
#include "lang.h"
#include <zlib.h>

static AssocType headAssocType = NULL;
static CGroup headCGroup = NULL;

static AssocType AGetType(cs_uint16 type) {
	AssocType ptr = headAssocType;

	while(ptr) {
		if(ptr->type == type) break;
		ptr = ptr->next;
	}

	return ptr;
}

static AssocNode AGetNode(Client client, cs_uint16 type) {
	AssocNode ptr = client->headNode;

	while(ptr) {
		if(ptr->type == type) break;
		ptr = ptr->next;
	}

	return ptr;
}

cs_uint16 Assoc_NewType() {
	AssocType tptr = Memory_Alloc(1, sizeof(struct _AssocType));
	if(headAssocType) {
		cs_uint16 type = 0;
		AssocType tptr2 = headAssocType;

		while(tptr2) {
			type = min(type, tptr2->type);
			tptr2 = tptr2->next;
		}

		tptr->type = type++;
		headAssocType->prev = tptr;
	}
	tptr->next = headAssocType;
	headAssocType = tptr;
	return tptr->type;
}

cs_bool Assoc_DelType(cs_uint16 type, cs_bool freeData) {
	AssocType tptr = AGetType(type);
	if(!tptr) return false;

	for(ClientID id = 0; id < MAX_CLIENTS; id++) {
		if(Clients_List[id])
			Assoc_Remove(Clients_List[id], type, freeData);
	}

	if(tptr->next)
		tptr->next->prev = tptr->next;
	if(tptr->prev)
		tptr->prev->next = tptr->prev;
	Memory_Free((void*)tptr);
	return true;
}

cs_bool Assoc_Set(Client client, cs_uint16 type, void* ptr) {
	if(AGetNode(client, type)) return false;
	AssocNode nptr = AGetNode(client, type);
	if(!nptr) {
		nptr = Memory_Alloc(1, sizeof(struct _AssocNode));
		nptr->type = type;
	}
	nptr->dataptr = ptr;
	nptr->next = client->headNode;
	if(client->headNode) client->headNode->prev = nptr;
	client->headNode = nptr;
	return true;
}

void* Assoc_GetPtr(Client client, cs_uint16 type) {
	AssocNode nptr = AGetNode(client, type);
	if(nptr) return nptr->dataptr;
	return NULL;
}

cs_bool Assoc_Remove(Client client, cs_uint16 type, cs_bool freeData) {
	AssocNode nptr = AGetNode(client, type);
	if(!nptr) return false;
	if(nptr->next)
		nptr->next->prev = nptr->next;
	if(nptr->prev)
		nptr->prev->next = nptr->prev;
	if(freeData) Memory_Free(nptr->dataptr);
	Memory_Free((void*)nptr);
	return true;
}

CGroup Group_Add(cs_int16 gid, const char* gname, cs_uint8 grank) {
	CGroup gptr = Group_GetByID(gid);
	if(!gptr) {
		gptr = Memory_Alloc(1, sizeof(struct _CGroup));
		gptr->id = gid;
		gptr->next = headCGroup;
		if(headCGroup) headCGroup->prev = gptr;
		headCGroup = gptr;
	}

	if(gptr->name) Memory_Free((void*)gptr->name);
	gptr->name = String_AllocCopy(gname);
	gptr->rank = grank;
	return gptr;
}

CGroup Group_GetByID(cs_int16 id) {
	CGroup gptr = headCGroup;

	while(gptr) {
		if(gptr->id == id) break;
		gptr = gptr->next;
	}

	return gptr;
}

cs_bool Group_Remove(cs_int16 gid) {
	CGroup cg = Group_GetByID(gid);
	if(!cg) return false;

	for(ClientID id = 0; id < MAX_CLIENTS; id++) {
		Client client = Clients_List[id];
		if(client && Client_GetGroupID(client) == gid)
			Client_SetGroup(client, -1);
	}

	if(cg->next)
		cg->next->prev = cg->prev;
	if(cg->prev)
		cg->prev->next = cg->next;

	Memory_Free((void*)cg->name);
	Memory_Free(cg);
	return true;
}

cs_uint8 Clients_GetCount(cs_int32 state) {
	cs_uint8 count = 0;
	for(ClientID i = 0; i < MAX_CLIENTS; i++) {
		Client client = Clients_List[i];
		if(!client) continue;
		PlayerData pd = client->playerData;
		if(pd && pd->state == state) count++;
	}
	return count;
}

void Clients_UpdateWorldInfo(World world) {
	for(ClientID i = 0; i < MAX_CLIENTS; i++) {
		Client cl = Clients_List[i];
		if(cl && Client_IsInWorld(cl, world))
			Client_UpdateWorldInfo(cl, world, false);
	}
}

void Clients_KickAll(const char* reason) {
	for(ClientID i = 0; i < MAX_CLIENTS; i++) {
		Client client = Clients_List[i];
		if(client) Client_Kick(client, reason);
	}
}

Client Client_New(Socket fd, cs_uint32 addr) {
	Client tmp = Memory_Alloc(1, sizeof(struct client));
	tmp->id = 0xFF;
	tmp->sock = fd;
	tmp->addr = addr;
	tmp->mutex = Mutex_Create();
	tmp->rdbuf = Memory_Alloc(134, 1);
	tmp->wrbuf = Memory_Alloc(2048, 1);
	return tmp;
}

cs_bool Client_Add(Client client) {
	cs_int8 maxplayers = Config_GetInt8(Server_Config, CFG_MAXPLAYERS_KEY);
	for(ClientID i = 0; i < min(maxplayers, MAX_CLIENTS); i++) {
		if(!Clients_List[i]) {
			client->id = i;
			Thread_Create(Client_ThreadProc, client, true);
			Clients_List[i] = client;
			return true;
		}
	}
	return false;
}

const char* Client_GetName(Client client) {
	return client->playerData->name;
}

const char* Client_GetAppName(Client client) {
	if(!client->cpeData) return Lang_Get(LANG_CPEVANILLA);
	return client->cpeData->appName;
}

const char* Client_GetSkin(Client client) {
	CPEData cpd = client->cpeData;
	if(!cpd || !cpd->skin)
		return Client_GetName(client);
	return cpd->skin;
}

Client Client_GetByName(const char* name) {
	for(ClientID i = 0; i < MAX_CLIENTS; i++) {
		Client client = Clients_List[i];
		if(!client) continue;
		PlayerData pd = client->playerData;
		if(pd && String_CaselessCompare(pd->name, name))
			return client;
	}
	return NULL;
}

Client Client_GetByID(ClientID id) {
	return id < MAX_CLIENTS ? Clients_List[id] : NULL;
}

static struct _CGroup dgroup = {-1, "", 0, NULL, NULL};

CGroup Client_GetGroup(Client client) {
	if(!client->cpeData) return &dgroup;
	CGroup gptr = Group_GetByID(client->cpeData->group);
	return !gptr ? &dgroup : gptr;
}

cs_int16 Client_GetGroupID(Client client) {
	if(!client->cpeData) return -1;
	return client->cpeData->group;
}

cs_int16 Client_GetModel(Client client) {
	if(!client->cpeData) return 256;
	return client->cpeData->model;
}

BlockID Client_GetHeldBlock(Client client) {
	if(!client->cpeData) return BLOCK_AIR;
	return client->cpeData->heldBlock;
}

cs_int32 Client_GetExtVer(Client client, cs_uint32 extCRC32) {
	CPEData cpd = client->cpeData;
	if(!cpd) return false;

	CPEExt ptr = cpd->firstExtension;
	while(ptr) {
		if(ptr->crc32 == extCRC32) return ptr->version;
		ptr = ptr->next;
	}
	return false;
}

cs_bool Client_Despawn(Client client) {
	PlayerData pd = client->playerData;
	if(!pd || !pd->spawned) return false;
	pd->spawned = false;
	Packet_WriteDespawn(Client_Broadcast, client);
	Event_Call(EVT_ONDESPAWN, (void*)client);
	return true;
}

static TRET wSendThread(TARG param) {
	Client client = param;
	if(client->closed) return 0;
	PlayerData pd = client->playerData;
	World world = pd->world;

	if(world->process == WP_LOADING) {
		Client_Chat(client, MT_CHAT, Lang_Get(LANG_INFWWAIT));
		Waitable_Wait(world->wait);
	}

	if(!world->loaded) {
		Client_Kick(client, Lang_Get(LANG_KICKMAPFAIL));
		return 0;
	}

	Packet_WriteLvlInit(client);
	Mutex_Lock(client->mutex);
	cs_uint8* data = (cs_uint8*)client->wrbuf;
	cs_uint8* worldData = world->data;
	cs_int32 worldSize = world->size;

	*data++ = 0x03;
	cs_uint16* len = (cs_uint16*)data++;
	cs_uint8* out = ++data;

	cs_int32 ret, windBits = 31;
	z_stream stream = {0};

	if(Client_GetExtVer(client, EXT_FASTMAP)) {
		windBits = -15;
		worldData += 4;
		worldSize -= 4;
	}

	if((ret = deflateInit2(
		&stream,
		1,
		Z_DEFLATED,
		windBits,
		8,
		Z_DEFAULT_STRATEGY)) != Z_OK) {
		pd->state = STATE_WLOADERR;
		return 0;
	}

	stream.avail_in = worldSize;
	stream.next_in = worldData;

	do {
		stream.next_out = out;
		stream.avail_out = 1024;

		if((ret = deflate(&stream, Z_FINISH)) == Z_STREAM_ERROR) {
			pd->state = STATE_WLOADERR;
			goto end;
		}

		*len = htons(1024 - (cs_uint16)stream.avail_out);
		if(client->closed || !Client_Send(client, 1028)) {
			pd->state = STATE_WLOADERR;
			goto end;
		}
	} while(stream.avail_out == 0);
	pd->state = STATE_WLOADDONE;

	end:
	deflateEnd(&stream);
	Mutex_Unlock(client->mutex);
	if(pd->state == STATE_WLOADDONE) {
		pd->state = STATE_INGAME;
		pd->position = world->info->spawnVec;
		pd->angle = world->info->spawnAng;
		Packet_WriteLvlFin(client, &world->info->dimensions);
		Client_Spawn(client);
	}

	return 0;
}

cs_bool Client_ChangeWorld(Client client, World world) {
	if(Client_IsInWorld(client, world)) return false;
	PlayerData pd = client->playerData;

	if(pd->state != STATE_INITIAL && pd->state != STATE_INGAME) {
		Client_Kick(client, Lang_Get(LANG_KICKMAPFAIL));
		return false;
	}

	Client_Despawn(client);
	pd->world = world;
	pd->state = STATE_MOTD;
	if(!world->loaded) World_Load(world);
	Thread_Create(wSendThread, client, true);
	return true;
}

void Client_UpdateWorldInfo(Client client, World world, cs_bool updateAll) {
	/*
	** Нет смысла пыжиться в попытках
	** поменять значения клиенту, если
	** он не поддерживает CPE вообще.
	*/
	if(!client->cpeData) return;
	WorldInfo wi = world->info;
	cs_uint8 modval = wi->modval;
	cs_uint16 modprop = wi->modprop;
	cs_uint8 modclr = wi->modclr;

	if(updateAll || modval & MV_COLORS) {
		for(cs_uint8 color = 0; color < WORLD_COLORS_COUNT; color++) {
			if(updateAll || modclr & (2 ^ color))
				Client_SetEnvColor(client, color, World_GetEnvColor(world, color));
		}
	}
	if(updateAll || modval & MV_TEXPACK)
		Client_SetTexturePack(client, wi->texturepack);
	if(updateAll || modval & MV_PROPS) {
		for(cs_uint8 prop = 0; prop < WORLD_PROPS_COUNT; prop++) {
			if(updateAll || modprop & (2 ^ prop))
				Client_SetEnvProperty(client, prop, World_GetProperty(world, prop));
		}
	}
	if(updateAll || modval & MV_WEATHER)
		Client_SetWeather(client, wi->wt);
}

static cs_uint32 copyMessagePart(const char* message, char* part, cs_uint32 i, char* color) {
	if(*message == '\0') return 0;

	if(i > 0) {
		*part++ = '>';
		*part++ = ' ';
	}

	if(*color > 0) {
		*part++ = '&';
		*part++ = *color;
	}

	cs_uint32 len = min(60, (cs_uint32)String_Length(message));
	if(message[len - 1] == '&' && ISHEX(message[len])) --len;

	for(cs_uint32 j = 0; j < len; j++) {
		char prevsym = (*part++ = *message++);
		char nextsym = *message;
		if(nextsym == '\0' || nextsym == '\n') break;
		if(prevsym == '&' && ISHEX(nextsym)) *color = nextsym;
	}

	*part = '\0';
	return len;
}

cs_bool Client_MakeSelection(Client client, cs_uint8 id, SVec* start, SVec* end, Color4* color) {
	if(Client_GetExtVer(client, EXT_CUBOID)) {
		CPEPacket_WriteMakeSelection(client, id, start, end, color);
		return true;
	}
	return false;
}

cs_bool Client_RemoveSelection(Client client, cs_uint8 id) {
	if(Client_GetExtVer(client, EXT_CUBOID)) {
		CPEPacket_WriteRemoveSelection(client, id);
		return true;
	}
	return false;
}

void Client_Chat(Client client, MessageType type, const char* message) {
	cs_uint32 msgLen = (cs_uint32)String_Length(message);

	if(msgLen > 62 && type == MT_CHAT) {
		char color = 0, part[65] = {0};
		cs_uint32 parts = (msgLen / 60) + 1;
		for(cs_uint32 i = 0; i < parts; i++) {
			cs_uint32 len = copyMessagePart(message, part, i, &color);
			if(len > 0) {
				Packet_WriteChat(client, type, part);
				message += len;
			}
		}
		return;
	}

	Packet_WriteChat(client, type, message);
}

static void HandlePacket(Client client, char* data, Packet packet, cs_bool extended) {
	cs_bool ret = false;

	if(extended)
		if(packet->cpeHandler)
			ret = packet->cpeHandler(client, data);
		else
			ret = packet->handler(client, data);
	else
		if(packet->handler)
			ret = packet->handler(client, data);

	if(!ret)
		Client_Kick(client, Lang_Get(LANG_KICKPACKETREAD));
	else
		client->pps += 1;
}

static cs_uint16 GetPacketSizeFor(Packet packet, Client client, cs_bool* extended) {
	cs_uint16 packetSize = packet->size;
	cs_bool _extended = *extended;
	if(packet->haveCPEImp) {
		_extended = Client_GetExtVer(client, packet->extCRC32) == packet->extVersion;
		if(_extended) packetSize = packet->extSize;
	}
	return packetSize;
}

static void PacketReceiverWs(Client client) {
	Packet packet;
	cs_bool extended;
	cs_uint16 packetSize, recvSize;
	WsClient ws = client->websock;
	char* data = client->rdbuf;

	if(WsClient_ReceiveFrame(ws)) {
		if(ws->opcode == 0x08) {
			client->closed = true;
			return;
		}

		recvSize = ws->plen - 1;
		handlePacket:
		packet = Packet_Get(*data++);
		if(!packet) {
			Client_Kick(client, Lang_Get(LANG_KICKPACKETREAD));
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
				goto handlePacket;
			}

			return;
		} else
			Client_Kick(client, Lang_Get(LANG_KICKPACKETREAD));
	} else
		client->closed = true;
}

static void PacketReceiverRaw(Client client) {
	Packet packet;
	cs_bool extended;
	cs_uint16 packetSize;
	cs_uint8 packetId;

	if(Socket_Receive(client->sock, (char*)&packetId, 1, 0) == 1) {
		packet = Packet_Get(packetId);
		if(!packet) {
			Client_Kick(client, Lang_Get(LANG_KICKPACKETREAD));
			return;
		}

		packetSize = GetPacketSizeFor(packet, client, &extended);

		if(packetSize > 0) {
			cs_int32 len = Socket_Receive(client->sock, client->rdbuf, packetSize, 0);

			if(packetSize == len)
				HandlePacket(client, client->rdbuf, packet, extended);
			else
				client->closed = true;
		}
	} else
		client->closed = true;
}

TRET Client_ThreadProc(TARG param) {
	Client client = param;

	while(!client->closed) {
		if(client->websock)
			PacketReceiverWs(client);
		else
			PacketReceiverRaw(client);
	}

	return 0;
}

void Client_Init(void) {
	Client_Broadcast = Memory_Alloc(1, sizeof(struct client));
	Client_Broadcast->wrbuf = Memory_Alloc(2048, 1);
	Client_Broadcast->mutex = Mutex_Create();
}

cs_bool Client_IsInGame(Client client) {
	if(!client->playerData) return false;
	return client->playerData->state == STATE_INGAME;
}

cs_bool Client_IsInSameWorld(Client client, Client other) {
	if(!client->playerData || !other->playerData) return false;
	return client->playerData->world == other->playerData->world;
}

cs_bool Client_IsInWorld(Client client, World world) {
	if(!client->playerData) return false;
	return client->playerData->world == world;
}

cs_bool Client_IsOP(Client client) {
	PlayerData pd = client->playerData;
	return pd ? pd->isOP : false;
}

cs_bool Client_CheckAuth(Client client) {
	return Heartbeat_CheckKey(client);
}

cs_bool Client_SetBlock(Client client, SVec* pos, BlockID id) {
	if(client->playerData->state != STATE_INGAME) return false;
	Packet_WriteSetBlock(client, pos, id);
	return true;
}

cs_bool Client_SetEnvProperty(Client client, cs_uint8 property, cs_int32 value) {
	if(Client_GetExtVer(client, EXT_MAPASPECT)) {
		CPEPacket_WriteMapProperty(client, property, value);
		return true;
	}
	return false;
}

cs_bool Client_SetEnvColor(Client client, cs_uint8 type, Color3* color) {
	if(Client_GetExtVer(client, EXT_MAPASPECT)) {
		CPEPacket_WriteEnvColor(client, type, color);
		return true;
	}
	return false;
}

cs_bool Client_SetTexturePack(Client client, const char* url) {
	if(Client_GetExtVer(client, EXT_MAPASPECT)) {
		CPEPacket_WriteTexturePack(client, url);
		return true;
	}
	return false;
}

cs_bool Client_SetWeather(Client client, Weather type) {
	if(Client_GetExtVer(client, EXT_WEATHER)) {
		CPEPacket_WriteWeatherType(client, type);
		return true;
	}
	return false;
}

cs_bool Client_SetInvOrder(Client client, Order order, BlockID block) {
	if(!Block_IsValid(block)) return false;

	if(Client_GetExtVer(client, EXT_INVORDER)) {
		CPEPacket_WriteInventoryOrder(client, order, block);
		return true;
	}
	return false;
}

cs_bool Client_SetHeld(Client client, BlockID block, cs_bool canChange) {
	if(!Block_IsValid(block)) return false;
	if(Client_GetExtVer(client, EXT_HELDBLOCK)) {
		CPEPacket_WriteHoldThis(client, block, canChange);
		return true;
	}
	return false;
}

cs_bool Client_SetHotbar(Client client, Order pos, BlockID block) {
	if(!Block_IsValid(block) || pos > 8) return false;
	if(Client_GetExtVer(client, EXT_SETHOTBAR)) {
		CPEPacket_WriteSetHotBar(client, pos, block);
		return true;
	}
	return false;
}

cs_bool Client_SetBlockPerm(Client client, BlockID block, cs_bool allowPlace, cs_bool allowDestroy) {
	if(!Block_IsValid(block)) return false;
	if(Client_GetExtVer(client, EXT_BLOCKPERM)) {
		CPEPacket_WriteBlockPerm(client, block, allowPlace, allowDestroy);
		return true;
	}
	return false;
}

cs_bool Client_SetModel(Client client, cs_int16 model) {
	if(!client->cpeData) return false;
	if(!CPE_CheckModel(model)) return false;
	client->cpeData->model = model;

	for(ClientID i = 0; i < MAX_CLIENTS; i++) {
		Client other = Clients_List[i];
		if(!other || !Client_GetExtVer(other, EXT_CHANGEMODEL)) continue;
		CPEPacket_WriteSetModel(other, client);
	}
	return true;
}

cs_bool Client_SetSkin(Client client, const char* skin) {
	CPEData cpd = client->cpeData;
	if(!cpd) return false;
	if(cpd->skin)
		Memory_Free((void*)cpd->skin);
	cpd->skin = String_AllocCopy(skin);

	for(ClientID i = 0; i < MAX_CLIENTS; i++) {
		Client other = Clients_List[i];
		if(!other || Client_GetExtVer(other, EXT_PLAYERLIST) != 2) continue;
		CPEPacket_WriteAddEntity2(other, client);
	}
	return true;
}

cs_bool Client_SetModelStr(Client client, const char* model) {
	return Client_SetModel(client, CPE_GetModelNum(model));
}

cs_bool Client_SetGroup(Client client, cs_int16 gid) {
	if(!client->playerData || !client->cpeData)
		return false;
	client->cpeData->group = gid;
	if(!client->playerData->firstSpawn)
		Client_UpdateGroup(client);
	return true;
}

cs_bool Client_UpdateHacks(Client client) {
	if(Client_GetExtVer(client, EXT_HACKCTRL)) {
		CPEPacket_WriteHackControl(client, client->cpeData->hacks);
		return true;
	}
	return false;
}

void Client_UpdateGroup(Client client) {
	for(ClientID id = 0; id < MAX_CLIENTS; id++) {
		Client other = Clients_List[id];
		if(other)
			CPEPacket_WriteAddName(other, client);
	}
}

static void SocketWaitClose(Client client) {
	Socket_Shutdown(client->sock, SD_SEND);
	while(Socket_Receive(client->sock, client->rdbuf, 131, 0) > 0) {}
	Socket_Close(client->sock);
}

void Client_Free(Client client) {
	if(client->id != 0xFF)
		Clients_List[client->id] = NULL;

	if(client->mutex) Mutex_Free(client->mutex);

	if(client->websock) Memory_Free(client->websock);

	PlayerData pd = client->playerData;

	if(pd) {
		Memory_Free((void*)pd->name);
		Memory_Free((void*)pd->key);
		Memory_Free(pd);
	}

	CPEData cpd = client->cpeData;

	if(cpd) {
		CPEExt prev, ptr = cpd->firstExtension;

		while(ptr) {
			prev = ptr;
			ptr = ptr->next;
			Memory_Free((void*)prev->name);
			Memory_Free(prev);
		}

		if(cpd->hacks) Memory_Free(cpd->hacks);
		if(cpd->message) Memory_Free(cpd->message);
		if(cpd->appName) Memory_Free((void*)cpd->appName);
		Memory_Free(cpd);
	}

	SocketWaitClose(client);
	Memory_Free(client->rdbuf);
	Memory_Free(client->wrbuf);
	Memory_Free(client);
}

cs_int32 Client_Send(Client client, cs_int32 len) {
	if(client->closed) return 0;
	if(client == Client_Broadcast) {
		for(ClientID i = 0; i < MAX_CLIENTS; i++) {
			Client bClient = Clients_List[i];

			if(bClient && !bClient->closed) {
				Mutex_Lock(bClient->mutex);
				if(bClient->websock)
					WsClient_SendHeader(bClient->websock, 0x02, (cs_uint16)len);
				Socket_Send(bClient->sock, client->wrbuf, len);
				Mutex_Unlock(bClient->mutex);
			}
		}
		return len;
	}

	if(client->websock)
		WsClient_SendHeader(client->websock, 0x02, (cs_uint16)len);
	return Socket_Send(client->sock, client->wrbuf, len);
}

/*
** Эта функция понадобилась ибо я не смог
** придумать как это всё уместить внутри
** Client_Spawn. Как-нибудь придумать,
** что с этим можно сделать.
*/

static void SendSpawnPacket(Client client, Client other) {
	cs_int32 extlist_ver = Client_GetExtVer(client, EXT_PLAYERLIST);

	if(extlist_ver == 2) {
		CPEPacket_WriteAddEntity2(client, other);
	} else { // TODO: ExtPlayerList ver. 1 support
		Packet_WriteSpawn(client, other);
	}
}

cs_bool Client_Spawn(Client client) {
	PlayerData pd = client->playerData;
	if(pd->spawned) return false;

	Client_UpdateWorldInfo(client, pd->world, true);

	for(ClientID i = 0; i < MAX_CLIENTS; i++) {
		Client other = Clients_List[i];
		if(!other) continue;

		if(pd->firstSpawn) {
			if(Client_GetExtVer(other, EXT_PLAYERLIST))
				CPEPacket_WriteAddName(other, client);
			if(Client_GetExtVer(client, EXT_PLAYERLIST) && client != other)
				CPEPacket_WriteAddName(client, other);
		}

		if(Client_IsInSameWorld(client, other)) {
			SendSpawnPacket(other, client);

			if(Client_GetExtVer(other, EXT_CHANGEMODEL))
				CPEPacket_WriteSetModel(other, client);

			if(client != other) {
				SendSpawnPacket(client, other);

				if(Client_GetExtVer(client, EXT_CHANGEMODEL))
					CPEPacket_WriteSetModel(client, other);
			}
		}
	}

	pd->spawned = true;
	Event_Call(EVT_ONSPAWN, (void*)client);
	if(pd->firstSpawn) { // TODO: Перенести это куда-нибудь
		const char* name = Client_GetName(client);
		const char* appname = Client_GetAppName(client);
		Log_Info(Lang_Get(LANG_SVPLCONN), name, appname);
		pd->firstSpawn = false;
	}
	return true;
}

void Client_HandshakeStage2(Client client) {
	Client_ChangeWorld(client, Worlds_List[0]);
}

void Client_Kick(Client client, const char* reason) {
	if(client->closed) return;
	if(!reason) reason = Lang_Get(LANG_KICKNOREASON);
	Packet_WriteKick(client, reason);
	client->closed = true;
	/*
	** Этот вызов нужен, чтобы корректно завершить
	** сокет клиента после кика, если цикл сервера
	** в основом потоке уже не работает.
	*/
	if(!Server_Active) Client_Tick(client);
}

void Client_UpdatePositions(Client client) {
	for(ClientID i = 0; i < MAX_CLIENTS; i++) {
		Client other = Clients_List[i];
		if(other && client != other && Client_IsInGame(other) && Client_IsInSameWorld(client, other))
			Packet_WritePosAndOrient(other, client);
	}
}

void Client_Tick(Client client) {
	PlayerData pd = client->playerData;
	if(client->closed) {
		if(pd && pd->state > STATE_WLOADDONE) {
			for(int i = 0; i < MAX_CLIENTS; i++) {
				Client other = Clients_List[i];
				if(other && Client_GetExtVer(other, EXT_PLAYERLIST))
					CPEPacket_WriteRemoveName(other, client);
			}
			Event_Call(EVT_ONDISCONNECT, (void*)client);
			Log_Info(Lang_Get(LANG_SVPLDISCONN), Client_GetName(client));
		}
		Client_Despawn(client);
		Client_Free(client);
		return;
	}

	client->ppstm += Server_Delta;
	if(client->ppstm > 1000) {
		if(client->pps > MAX_CLIENT_PPS) {
			Client_Kick(client, Lang_Get(LANG_KICKPACKETSPAM));
			return;
		}
		client->pps = 0;
		client->ppstm = 0;
	}

	if(pd && pd->state == STATE_WLOADERR)
		Client_Kick(client, Lang_Get(LANG_KICKMAPFAIL));
}
