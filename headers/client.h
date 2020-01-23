#ifndef CLIENT_H
#define CLIENT_H
#include "block.h"
#include "vector.h"
#include "world.h"
#include "websocket.h"

enum {
	MT_CHAT, // Сообщение в чате
	MT_STATUS1, // Правый верхний угол
	MT_STATUS2,
	MT_STATUS3,
	MT_BRIGHT1 = 11, // Правый нижний угол
	MT_BRIGHT2,
	MT_BRIGHT3,
	MT_ANNOUNCE = 100 // Сообщение в середине экрана
};

enum {
	STATE_INITIAL, // Игрок только подключился
	STATE_MOTD, // Игрок получает карту
	STATE_WLOADDONE, // Карта была успешно получена
	STATE_WLOADERR, // Ошибка при получении карты
	STATE_INGAME // Игрок находится в игре
};

enum {
	PCU_NONE = 0, // Ни одно из CPE-значений игрока не изменилось
	PCU_GROUP = 2, // Была обновлена группа игрока
	PCU_MODEL = 4, // Была изменена модель игрока
	PCU_SKIN = 8, // Был изменён скин игрока
	PCU_ENTPROP = 16 // Модель игрока была повёрнута
};

enum {
	ROT_X = 0, // Вращение модели по оси X
	ROT_Y = 1,
	ROT_Z = 2,
};

typedef struct _AssocType {
	cs_uint16 type; // Сам тип, регистрируемый структурой
	struct _AssocType* prev; // Предыдущий тип
	struct _AssocType* next; // Следующий тип
} AssocType;

typedef struct _AssocNode {
	cs_uint16 type; // Тип ноды
	void* dataptr; // Поинтер, который ей присвоен
	struct _AssocNode* prev; // Предыдущая нода
	struct _AssocNode* next; // Следующая нода
} AssocNode;

typedef struct _CGroup {
	cs_int16 id;
	cs_str name;
	cs_uint8 rank;
	struct _CGroup* prev; // Предыдущая группа
	struct _CGroup* next; // Следующая группа
} CGroup;

typedef struct {
	cs_bool flying, noclip, speeding;
	cs_bool spawnControl, tpv;
	cs_int16 jumpHeight;
} CPEHacks;

typedef struct {
	CPEExt headExtension; // Список дополнений клиента
	cs_str appName; // Название игрового клиента
	cs_str skin; // Скин игрока, может быть NULL [ExtPlayerList]
	cs_bool hideDisplayName; // Будет ли ник игрока скрыт [ExtPlayerList]
	cs_int32 rotation[3]; // Вращение модели игрока в градусах [EntityProperty]
	char* message; // Используется для получения длинных сообщений [LongerMessages]
	BlockID heldBlock; // Выбранный игроком блок в данный момент [HeldBlock]
	cs_int16 _extCount; // Переменная используется при получении списка дополнений
	cs_int16 model; // Текущая модель игрока [ChangeModel]
	cs_int16 group; // Текущая группа игрока [ExtPlayerList]
	cs_bool pingStarted; // Начат ли процесс пингования [TwoWayPing]
	cs_uint16 pingData; // Данные, цепляемые к пинг-запросу
	cs_uint64 pingStart; // Время начала пинг-запроса
	cs_uint32 pingTime; // Сам пинг, в миллисекундах
	cs_int8 updates; // Обновлённые значения игрока
} CPEData;

typedef struct {
	cs_int32 state; // Текущее состояние игрока
	cs_str key; // Ключ, полученный от игрока
	cs_str name; // Имя игрока
	World* world; // Мир, в котором игрок обитает
	Vec position; // Позиция игрока
	Ang angle; // Угол вращения игрока
	cs_bool isOP; // Является ли игрок оператором
	cs_bool spawned; // Заспавнен ли игрок
	cs_bool firstSpawn; // Был лы этот спавн первым с момента захода на сервер
} PlayerData;

typedef struct {
	ClientID id; // Используется в качестве entityid
	void* thread[2]; // Потоки клиента
	CPEData* cpeData; // В случае vanilla клиента эта структура не создаётся
	PlayerData* playerData; // Создаётся при получении hanshake пакета
	AssocNode* headNode; // Последняя созданная ассоциативная нода у клиента
	WsClient* websock; // Создаётся, если клиент был определён как браузерный
	Mutex* mutex; // Мьютекс записи, на время отправки пакета клиенту он лочится
	cs_bool closed; // В случае значения true сервер прекращает общение с клиентом и удаляет его
	cs_uint32 addr; // ipv4 адрес клиента
	Socket sock; // Файловый дескриптор сокета клиента
	char* rdbuf; // Буфер для получения пакетов от клиента
	char* wrbuf; // Буфер для отправки пакетов клиенту
	cs_uint32 pps; // Количество пакетов, отправленных игроком за секунду
	cs_uint32 ppstm; // Таймер для счётчика пакетов
} Client;

cs_int32 Client_Send(Client* client, cs_int32 len);
void Client_HandshakeStage2(Client* client);
cs_bool Client_CheckAuth(Client* client);
void Client_Free(Client* client);
void Client_Tick(Client* client, cs_int32 delta);
Client* Client_New(Socket fd, cs_uint32 addr);
cs_bool Client_Add(Client* client);
void Client_Init(void);
cs_bool Client_BulkBlockUpdate(Client* client, BulkBlockUpdate* bbu);
cs_bool Client_DefineBlock(Client* client, BlockDef* block);
cs_bool Client_UndefineBlock(Client* client, BlockID id);

API cs_uint16 Assoc_NewType();
API cs_bool Assoc_DelType(cs_uint16 type, cs_bool freeData);
API cs_bool Assoc_Set(Client* client, cs_uint16 type, void* ptr);
API void* Assoc_GetPtr(Client* client, cs_uint16 type);
API cs_bool Assoc_Remove(Client* client, cs_uint16 type, cs_bool freeData);

API CGroup* Group_Add(cs_int16 gid, cs_str gname, cs_uint8 grank);
API CGroup* Group_GetByID(cs_int16 gid);
API cs_bool Group_Remove(cs_int16 gid);

API cs_uint8 Clients_GetCount(cs_int32 state);
API void Clients_KickAll(cs_str reason);
API void Clients_UpdateWorldInfo(World* world);

API cs_bool Client_ChangeWorld(Client* client, World* world);
API void Client_Chat(Client* client, cs_uint8 type, cs_str message);
API void Client_Kick(Client* client, cs_str reason);
API void Client_UpdateWorldInfo(Client* client, World* world, cs_bool updateAll);
API cs_bool Client_Update(Client* client);
API cs_bool Client_SendHacks(Client* client, CPEHacks* hacks);
API cs_bool Client_MakeSelection(Client* client, cs_uint8 id, SVec* start, SVec* end, Color4* color);
API cs_bool Client_RemoveSelection(Client* client, cs_uint8 id);
API cs_bool Client_TeleportTo(Client* client, Vec* pos, Ang* ang);

API cs_bool Client_IsInSameWorld(Client* client, Client* other);
API cs_bool Client_IsInWorld(Client* client, World* world);
API cs_bool Client_IsInGame(Client* client);
API cs_bool Client_IsOP(Client* client);

API cs_bool Client_SetWeather(Client* client, cs_int8 type);
API cs_bool Client_SetInvOrder(Client* client, Order order, BlockID block);
API cs_bool Client_SetEnvProperty(Client* client, cs_uint8 property, cs_int32 value);
API cs_bool Client_SetEnvColor(Client* client, cs_uint8 type, Color3* color);
API cs_bool Client_SetTexturePack(Client* client, cs_str url);
API cs_bool Client_AddTextColor(Client* client, Color4* color, char code);
API cs_bool Client_SetBlock(Client* client, SVec* pos, BlockID id);
API cs_bool Client_SetModel(Client* client, cs_int16 model);
API cs_bool Client_SetModelStr(Client* client, cs_str model);
API cs_bool Client_SetBlockPerm(Client* client, BlockID block, cs_bool allowPlace, cs_bool allowDestroy);
API cs_bool Client_SetHeld(Client* client, BlockID block, cs_bool canChange);
API cs_bool Client_SetHotkey(Client* client, cs_str action, cs_int32 keycode, cs_int8 keymod);
API cs_bool Client_SetHotbar(Client* client, Order pos, BlockID block);
API cs_bool Client_SetSkin(Client* client, cs_str skin);
API cs_bool Client_SetSpawn(Client* client, Vec* pos, Ang* ang);
API cs_bool Client_SetVelocity(Client* client, Vec* velocity, cs_bool mode);
API cs_bool Client_SetRotation(Client* client, cs_uint8 axis, cs_int32 value);
API cs_bool Client_SetGroup(Client* client, cs_int16 gid);

API cs_str Client_GetName(Client* client);
API cs_str Client_GetAppName(Client* client);
API cs_str Client_GetSkin(Client* client);
API Client* Client_GetByID(ClientID id);
API Client* Client_GetByName(cs_str name);
API World* Client_GetWorld(Client* client);
API cs_int8 Client_GetFluidLevel(Client* client);
API cs_int16 Client_GetModel(Client* client);
API BlockID Client_GetHeldBlock(Client* client);
API cs_int32 Client_GetExtVer(Client* client, cs_uint32 extCRC32);
API CGroup* Client_GetGroup(Client* client);
API cs_int16 Client_GetGroupID(Client* client);

API cs_bool Client_Spawn(Client* client);
API cs_bool Client_Despawn(Client* client);

VAR Client* Broadcast;
VAR Client* Clients_List[MAX_CLIENTS];
#endif // CLIENT_H
