#include "core.h"
#include "platform.h"
#include "str.h"
#include "block.h"
#include "client.h"
#include "server.h"
#include "packets.h"
#include "event.h"
#include "heartbeat.h"
#include "lang.h"

static AssocType headAssocType = NULL;

static AssocType AGetType(uint16_t type) {
	AssocType ptr = headAssocType;

	while(ptr) {
		if(ptr->type == type) break;
		ptr = ptr->next;
	}

	return ptr;
}

static AssocNode AGetNode(Client client, uint16_t type) {
	AssocNode ptr = client->headNode;

	while(ptr) {
		if(ptr->type == type) break;
		ptr = ptr->next;
	}

	return ptr;
}

uint16_t Assoc_NewType() {
	AssocType tptr = Memory_Alloc(1, sizeof(struct _AssocType));
	if(headAssocType) {
		uint16_t type = 0;
		AssocType tptr2 = headAssocType;

		while(tptr2) {
			type = min(type, tptr2->type);
			tptr2 = tptr2->next;
		}

		tptr->type = type++;
		headAssocType->next = tptr;
	}
	tptr->prev = headAssocType;
	headAssocType = tptr;
	return tptr->type;
}

bool Assoc_DelType(uint16_t type, bool freeData) {
	AssocType tptr = AGetType(type);
	if(!tptr) return false;

	for(ClientID id = 0; id < MAX_CLIENTS; id++) {
		if(Clients_List[id])
			Assoc_Remove(Clients_List[id], type, freeData);
	}

	if(tptr->next)
		tptr->next->prev = tptr->prev;
	if(tptr->prev)
		tptr->prev->next = tptr->next;
	Memory_Free((void*)tptr);
	return true;
}

bool Assoc_Set(Client client, uint16_t type, void* ptr) {
	if(AGetNode(client, type)) return false;
	AssocNode nptr = AGetNode(client, type);
	if(!nptr) {
		nptr = Memory_Alloc(1, sizeof(struct _AssocNode));
		nptr->type = type;
	}
	nptr->dataptr = ptr;
	nptr->prev = client->headNode;
	if(client->headNode) client->headNode->next = nptr;
	client->headNode = nptr;
	return true;
}

void* Assoc_GetPtr(Client client, uint16_t type) {
	AssocNode nptr = AGetNode(client, type);
	if(nptr) return nptr->dataptr;
	return NULL;
}

bool Assoc_Remove(Client client, uint16_t type, bool freeData) {
	AssocNode nptr = AGetNode(client, type);
	if(!nptr) return false;
	if(nptr->next)
		nptr->next->prev = nptr->prev;
	if(nptr->prev)
	nptr->prev->next = nptr->next;
	if(freeData) Memory_Free(nptr->dataptr);
	Memory_Free((void*)nptr);
	return true;
}

uint8_t Clients_GetCount(int32_t state) {
	uint8_t count = 0;
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

Client Client_New(Socket fd, uint32_t addr) {
	Client tmp = Memory_Alloc(1, sizeof(struct client));
	tmp->id = 0xFF;
	tmp->sock = fd;
	tmp->addr = addr;
	tmp->mutex = Mutex_Create();
	tmp->rdbuf = Memory_Alloc(134, 1);
	tmp->wrbuf = Memory_Alloc(2048, 1);
	return tmp;
}

bool Client_Add(Client client) {
	int8_t maxplayers = Config_GetInt8(Server_Config, CFG_MAXPLAYERS_KEY);
	for(ClientID i = 0; i < min(maxplayers, MAX_CLIENTS); i++) {
		if(!Clients_List[i]) {
			client->id = i;
			client->thread = Thread_Create(Client_ThreadProc, client);
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
	if(!cpd || !cpd->skinURL)
		return Client_GetName(client);
	return cpd->skinURL;
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

int16_t Client_GetModel(Client client) {
	if(!client->cpeData) return 0;
	return client->cpeData->model;
}

BlockID Client_GetHeldBlock(Client client) {
	if(!client->cpeData) return BLOCK_AIR;
	return client->cpeData->heldBlock;
}

int32_t Client_GetExtVer(Client client, uint32_t extCRC32) {
	CPEData cpd = client->cpeData;
	if(!cpd) return false;

	CPEExt ptr = cpd->firstExtension;
	while(ptr) {
		if(ptr->crc32 == extCRC32) return ptr->version;
		ptr = ptr->next;
	}
	return false;
}

bool Client_Despawn(Client client) {
	PlayerData pd = client->playerData;
	if(!pd || !pd->spawned) return false;
	pd->spawned = false;
	Packet_WriteDespawn(Client_Broadcast, client);
	Event_Call(EVT_ONDESPAWN, (void*)client);
	return true;
}

bool Client_ChangeWorld(Client client, World world) {
	if(Client_IsInWorld(client, world)) return false;

	Client_Despawn(client);
	PlayerData pd = client->playerData;
	pd->position = world->info->spawnVec;
	pd->angle = world->info->spawnAng;
	if(!Client_SendMap(client, world)) {
		Client_Kick(client, Lang_Get(LANG_KICKMAPFAIL));
		return false;
	}
	return true;
}

void Client_UpdateWorldInfo(Client client, World world, bool updateAll) {
	/*
	** Нет смысла пыжиться в попытках
	** поменять значения клиенту, если
	** он не поддерживает CPE вообще.
	*/
	if(!client->cpeData) return;
	WorldInfo wi = world->info;
	uint8_t modval = wi->modval;
	uint16_t modprop = wi->modprop;
	uint8_t modclr = wi->modclr;

	if(updateAll || modval & MV_COLORS) {
		for(uint8_t color = 0; color < WORLD_COLORS_COUNT; color++) {
			if(updateAll || modclr & (2 ^ color))
				Client_SetEnvColor(client, color, World_GetEnvColor(world, color));
		}
	}
	if(updateAll || modval & MV_TEXPACK)
		Client_SetTexturePack(client, wi->texturepack);
	if(updateAll || modval & MV_PROPS) {
		for(uint8_t prop = 0; prop < WORLD_PROPS_COUNT; prop++) {
			if(updateAll || modprop & (2 ^ prop))
				Client_SetEnvProperty(client, prop, World_GetProperty(world, prop));
		}
	}
	if(updateAll || modval & MV_WEATHER)
		Client_SetWeather(client, wi->wt);
}

static uint32_t copyMessagePart(const char* message, char* part, uint32_t i, char* color) {
	if(*message == '\0') return 0;

	if(i > 0) {
		*part++ = '>';
		*part++ = ' ';
	}

	if(*color > 0) {
		*part++ = '&';
		*part++ = *color;
	}

	uint32_t len = min(60, (uint32_t)String_Length(message));
	if(message[len - 1] == '&' && ISHEX(message[len])) --len;

	for(uint32_t j = 0; j < len; j++) {
		char prevsym = (*part++ = *message++);
		char nextsym = *message;
		if(nextsym == '\0' || nextsym == '\n') break;
		if(prevsym == '&' && ISHEX(nextsym)) *color = nextsym;
	}

	*part = '\0';
	return len;
}

bool Client_MakeSelection(Client client, uint8_t id, SVec* start, SVec* end, Color4* color) {
	if(Client_GetExtVer(client, EXT_CUBOID)) {
		CPEPacket_WriteMakeSelection(client, id, start, end, color);
		return true;
	}
	return false;
}

bool Client_RemoveSelection(Client client, uint8_t id) {
	if(Client_GetExtVer(client, EXT_CUBOID)) {
		CPEPacket_WriteRemoveSelection(client, id);
		return true;
	}
	return false;
}

void Client_Chat(Client client, MessageType type, const char* message) {
	uint32_t msgLen = (uint32_t)String_Length(message);

	if(msgLen > 62 && type == CPE_CHAT) {
		char color = 0, part[65] = {0};
		uint32_t parts = (msgLen / 60) + 1;
		for(uint32_t i = 0; i < parts; i++) {
			uint32_t len = copyMessagePart(message, part, i, &color);
			if(len > 0) {
				Packet_WriteChat(client, type, part);
				message += len;
			}
		}
		return;
	}

	Packet_WriteChat(client, type, message);
}

static void HandlePacket(Client client, char* data, Packet packet, bool extended) {
	bool ret = false;

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

static uint16_t GetPacketSizeFor(Packet packet, Client client, bool* extended) {
	uint16_t packetSize = packet->size;
	bool _extended = *extended;
	if(packet->haveCPEImp) {
		_extended = Client_GetExtVer(client, packet->extCRC32) == packet->extVersion;
		if(_extended) packetSize = packet->extSize;
	}
	return packetSize;
}

static void PacketReceiverWs(Client client) {
	Packet packet;
	bool extended;
	uint16_t packetSize, recvSize;
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
	bool extended;
	uint16_t packetSize;
	uint8_t packetId;

	if(Socket_Receive(client->sock, (char*)&packetId, 1, 0) == 1) {
		packet = Packet_Get(packetId);
		if(!packet) {
			Client_Kick(client, Lang_Get(LANG_KICKPACKETREAD));
			return;
		}

		packetSize = GetPacketSizeFor(packet, client, &extended);

		if(packetSize > 0) {
			int32_t len = Socket_Receive(client->sock, client->rdbuf, packetSize, 0);

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

TRET Client_MapThreadProc(TARG param) {
	Client client = param;
	if(client->closed) return 0;

	uint8_t* data = (uint8_t*)client->wrbuf;
	PlayerData pd = client->playerData;

	World world = pd->world;
	uint8_t* mapData = world->data;
	int32_t mapSize = world->size;

	*data++ = 0x03;
	uint16_t* len = (uint16_t*)data++;
	uint8_t* out = ++data;

	int32_t ret, windowBits = 31;
	z_stream stream = {0};

	Mutex_Lock(client->mutex);
	if(Client_GetExtVer(client, EXT_FASTMAP)) {
		windowBits = -15;
		mapData += 4;
		mapSize -= 4;
	}

	if((ret = deflateInit2(
		&stream,
		1,
		Z_DEFLATED,
		windowBits,
		8,
		Z_DEFAULT_STRATEGY)) != Z_OK) {
		pd->state = STATE_WLOADERR;
		return 0;
	}

	stream.avail_in = mapSize;
	stream.next_in = mapData;

	do {
		stream.next_out = out;
		stream.avail_out = 1024;

		if((ret = deflate(&stream, Z_FINISH)) == Z_STREAM_ERROR) {
			pd->state = STATE_WLOADERR;
			goto end;
		}

		*len = htons(1024 - (uint16_t)stream.avail_out);
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
		Packet_WriteLvlFin(client);
		Client_Spawn(client);
	}

	return 0;
}

void Client_Init(void) {
	Client_Broadcast = Memory_Alloc(1, sizeof(struct client));
	Client_Broadcast->wrbuf = Memory_Alloc(2048, 1);
	Client_Broadcast->mutex = Mutex_Create();
}

bool Client_IsInGame(Client client) {
	if(!client->playerData) return false;
	return client->playerData->state == STATE_INGAME;
}

bool Client_IsInSameWorld(Client client, Client other) {
	if(!client->playerData || !other->playerData) return false;
	return client->playerData->world == other->playerData->world;
}

bool Client_IsInWorld(Client client, World world) {
	if(!client->playerData) return false;
	return client->playerData->world == world;
}

bool Client_IsOP(Client client) {
	PlayerData pd = client->playerData;
	return pd ? pd->isOP : false;
}

//TODO: ClassiCube auth
bool Client_CheckAuth(Client client) {
	return Heartbeat_CheckKey(client);
}

bool Client_SetBlock(Client client, SVec* pos, BlockID id) {
	if(client->playerData->state != STATE_INGAME) return false;
	Packet_WriteSetBlock(client, pos, id);
	return true;
}

bool Client_SetEnvProperty(Client client, uint8_t property, int32_t value) {
	if(Client_GetExtVer(client, EXT_MAPASPECT)) {
		CPEPacket_WriteMapProperty(client, property, value);
		return true;
	}
	return false;
}

bool Client_SetEnvColor(Client client, uint8_t type, Color3* color) {
	if(Client_GetExtVer(client, EXT_MAPASPECT)) {
		CPEPacket_WriteEnvColor(client, type, color);
		return true;
	}
	return false;
}

bool Client_SetTexturePack(Client client, const char* url) {
	if(Client_GetExtVer(client, EXT_MAPASPECT)) {
		CPEPacket_WriteTexturePack(client, url);
		return true;
	}
	return false;
}

bool Client_SetWeather(Client client, Weather type) {
	if(Client_GetExtVer(client, EXT_WEATHER)) {
		CPEPacket_WriteWeatherType(client, type);
		return true;
	}
	return false;
}

bool Client_SetInvOrder(Client client, Order order, BlockID block) {
	if(!Block_IsValid(block)) return false;

	if(Client_GetExtVer(client, EXT_INVORDER)) {
		CPEPacket_WriteInventoryOrder(client, order, block);
		return true;
	}
	return false;
}

bool Client_SetHeld(Client client, BlockID block, bool canChange) {
	if(!Block_IsValid(block)) return false;
	if(Client_GetExtVer(client, EXT_HELDBLOCK)) {
		CPEPacket_WriteHoldThis(client, block, canChange);
		return true;
	}
	return false;
}

bool Client_SetHotbar(Client client, Order pos, BlockID block) {
	if(!Block_IsValid(block) || pos > 8) return false;
	if(Client_GetExtVer(client, EXT_SETHOTBAR)) {
		CPEPacket_WriteSetHotBar(client, pos, block);
		return true;
	}
	return false;
}

bool Client_SetBlockPerm(Client client, BlockID block, bool allowPlace, bool allowDestroy) {
	if(!Block_IsValid(block)) return false;
	if(Client_GetExtVer(client, EXT_BLOCKPERM)) {
		CPEPacket_WriteBlockPerm(client, block, allowPlace, allowDestroy);
		return true;
	}
	return false;
}

bool Client_SetModel(Client client, int16_t model) {
	if(!client->cpeData) return false;
	if(!CPE_CheckModel(model)) return false;
	client->cpeData->model = model;

	for(ClientID i = 0; i < MAX_CLIENTS; i++) {
		Client other = Clients_List[i];
		if(!other || !Client_GetExtVer(other, EXT_CHANGEMODEL)) continue;
		CPEPacket_WriteSetModel(other, other == client ? 0xFF : client->id, model);
	}
	return true;
}

bool Client_SetModelStr(Client client, const char* model) {
	return Client_SetModel(client, CPE_GetModelNum(model));
}

bool Client_SetHacks(Client client) {
	if(Client_GetExtVer(client, EXT_HACKCTRL)) {
		CPEPacket_WriteHackControl(client, client->cpeData->hacks);
		return true;
	}
	return false;
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

	if(client->thread) Thread_Close(client->thread);

	if(client->mapThread) Thread_Close(client->mapThread);

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

int32_t Client_Send(Client client, int32_t len) {
	if(client->closed) return 0;
	if(client == Client_Broadcast) {
		for(ClientID i = 0; i < MAX_CLIENTS; i++) {
			Client bClient = Clients_List[i];

			if(bClient && !bClient->closed) {
				Mutex_Lock(bClient->mutex);
				if(bClient->websock)
					WsClient_SendHeader(bClient->websock, 0x02, (uint16_t)len);
				Socket_Send(bClient->sock, client->wrbuf, len);
				Mutex_Unlock(bClient->mutex);
			}
		}
		return len;
	}

	if(client->websock)
		WsClient_SendHeader(client->websock, 0x02, (uint16_t)len);
	return Socket_Send(client->sock, client->wrbuf, len);
}

/*
** Эта функция понадобилась ибо я не смог
** придумать как это всё уместить внутри
** Client_Spawn. Как-нибудь придумать,
** что с этим можно сделать.
*/

static void SendSpawnPacket(Client client, Client other) {
	int32_t extlist_ver = Client_GetExtVer(client, EXT_PLAYERLIST);

	if(extlist_ver == 2) {
		if(client->playerData->firstSpawn || other->playerData->firstSpawn)
			CPEPacket_WriteAddName(client, other);
		CPEPacket_WriteAddEntity2(client, other);
	} else { // TODO: ExtPlayerList ver. 1 support
		Packet_WriteSpawn(client, other);
	}
}

bool Client_Spawn(Client client) {
	PlayerData pd = client->playerData;
	if(pd->spawned) return false;
	World world = pd->world;
	Client_UpdateWorldInfo(client, world, true);

	for(ClientID i = 0; i < MAX_CLIENTS; i++) {
		Client other = Clients_List[i];
		if(other && Client_IsInSameWorld(client, other)) {
			SendSpawnPacket(other, client);

			if(other->cpeData && Client_GetExtVer(other, EXT_CHANGEMODEL))
				CPEPacket_WriteSetModel(other, other == client ? 0xFF : client->id, Client_GetModel(client));

			if(client != other) {
				SendSpawnPacket(client, other);

				if(other->cpeData && Client_GetExtVer(client, EXT_CHANGEMODEL))
					CPEPacket_WriteSetModel(client, other->id, Client_GetModel(other));
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

bool Client_SendMap(Client client, World world) {
	if(client->mapThread) return false;
	PlayerData pd = client->playerData;
	pd->world = world;
	pd->state = STATE_MOTD;
	Packet_WriteLvlInit(client);
	client->mapThread = Thread_Create(Client_MapThreadProc, client);
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

	if(client->ppstm < 1000) {
		client->ppstm += Server_Delta;
	} else {
		if(client->pps > MAX_CLIENT_PPS) {
			Client_Kick(client, Lang_Get(LANG_KICKPACKETSPAM));
			return;
		}
		client->pps = 0;
		client->ppstm = 0;
	}

	if(!pd) return;
	switch (pd->state) {
		case STATE_WLOADDONE:
			pd->state = STATE_INGAME;
			Thread_Close(client->mapThread);
			client->mapThread = NULL;
			break;
		case STATE_WLOADERR:
			Client_Kick(client, Lang_Get(LANG_KICKMAPFAIL));
			break;
	}
}
