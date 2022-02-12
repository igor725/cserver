#ifndef CLIENT_H
#define CLIENT_H
#include "core.h"
#include "list.h"
#include "block.h"
#include "vector.h"
#include "world.h"
#include "websocket.h"
#include "compr.h"
#include <stdarg.h>

typedef enum _EMesgType {
	MESSAGE_TYPE_CHAT, // Сообщение в чате
	MESSAGE_TYPE_STATUS1, // Правый верхний угол
	MESSAGE_TYPE_STATUS2,
	MESSAGE_TYPE_STATUS3,
	MESSAGE_TYPE_BRIGHT1 = 11, // Правый нижний угол
	MESSAGE_TYPE_BRIGHT2,
	MESSAGE_TYPE_BRIGHT3,
	MESSAGE_TYPE_ANNOUNCE = 100 // Сообщение в середине экрана
} EMesgType;

typedef enum _EPlayerState {
	PLAYER_STATE_INITIAL, // Игрок только подключился
	PLAYER_STATE_MOTD, // Игрок получает карту
	PLAYER_STATE_INGAME // Игрок находится в игре
} EPlayerState;


#define PCU_NONE BIT(0) // Ни одно из CPE-значений игрока не изменилось
#define PCU_GROUP BIT(1) // Была обновлена группа игрока
#define PCU_MODEL BIT(2) // Была изменена модель игрока
#define PCU_SKIN BIT(3) // Был изменён скин игрока
#define PCU_ENTPROP BIT(5) // Модель игрока была повёрнута

typedef struct _CGroup {
	cs_int16 id;
	cs_byte rank;
	cs_char name[65];
	AListField *field;
} CGroup;

typedef struct _CPEHacks {
	cs_bool flying, noclip, speeding,
	spawnControl, tpv;
	cs_int16 jumpHeight;
} CPEHacks;

typedef struct _CPEData {
	CPEExt *headExtension; // Список дополнений клиента
	cs_char appName[65]; // Название игрового клиента
	cs_char skin[65]; // Скин игрока, может быть NULL [ExtPlayerList]
	cs_char *message; // Используется для получения длинных сообщений [LongerMessages]
	BlockID heldBlock; // Выбранный игроком блок в данный момент [HeldBlock]
	cs_int8 updates; // Обновлённые значения игрока
	cs_bool hideDisplayName, // Будет ли ник игрока скрыт [ExtPlayerList]
	pingStarted; // Начат ли процесс пингования [TwoWayPing]
	cs_int16 _extCount, // Переменная используется при получении списка дополнений
	model, // Текущая модель игрока [ChangeModel]
	group; // Текущая группа игрока [ExtPlayerList]
	cs_uint16 pingData; // Данные, цепляемые к пинг-запросу
	cs_uint32 pingTime; // Сам пинг, в миллисекундах
	cs_float pingAvgTime; // Средний пинг, в миллисекундах
	cs_uint32 _pingAvgSize; // Количество значений в текущем среднем пинга
	cs_uint64 pingStart; // Время начала пинг-запроса
	cs_int32 rotation[3]; // Вращение модели игрока в градусах [EntityProperty]
	cs_uint16 clickDist; // Расстояние клика игрока
} CPEData;

typedef struct _PlayerData {
	cs_char key[65]; // Ключ, полученный от игрока
	cs_char name[65]; // Имя игрока
	World *world, *reqWorldChange; // Мир, в котором игрок обитает
	Vec position; // Позиция игрока
	Ang angle; // Угол вращения игрока
	EPlayerState state; // Текущее состояние игрока
	cs_bool isOP, // Является ли игрок оператором
	spawned, // Заспавнен ли игрок
	firstSpawn; // Был лы этот спавн первым с момента захода на сервер
} PlayerData;

typedef struct _Client {
	Waitable *waitend; // Ожидание завершения потока клиента
	cs_bool closed; // В случае значения true сервер прекращает общение с клиентом
	Socket sock; // Файловый дескриптор сокета клиента
	ClientID id; // Используется в качестве entityid
	cs_ulong pps, // Количество пакетов, отправленных игроком за секунду
	ppstm, // Таймер для счётчика пакетов
	addr; // ipv4 адрес клиента
	Compr compr; // Штука для сжатия карты
	CPEData *cpeData; // В случае vanilla клиента эта структура не создаётся
	PlayerData *playerData; // Создаётся при получении hanshake пакета
	KListField *headNode; // Последняя созданная ассоциативная нода у клиента
	WebSock *websock; // Создаётся, если клиент был определён как браузерный
	Mutex *mutex; // Мьютекс записи, на время отправки пакета клиенту он лочится
	cs_char rdbuf[134], // Буфер для получения пакетов от клиента
	wrbuf[2048]; // Буфер для отправки пакетов клиенту
} Client;

void Client_Tick(Client *client);
void Client_Free(Client *client);

NOINL cs_int32 Client_Send(Client *client, cs_int32 len);
NOINL cs_bool Client_BulkBlockUpdate(Client *client, BulkBlockUpdate *bbu);
NOINL cs_bool Client_DefineBlock(Client *client, BlockDef *block);
NOINL cs_bool Client_UndefineBlock(Client *client, BlockID id);

API CGroup *Group_Add(cs_int16 gid, cs_str gname, cs_byte grank);
API CGroup *Group_GetByID(cs_int16 gid);
API cs_bool Group_Remove(cs_int16 gid);

API cs_byte Clients_GetCount(EPlayerState state);
API void Clients_KickAll(cs_str reason);

API cs_bool Client_ChangeWorld(Client *client, World *world);
API void Client_Chat(Client *client, EMesgType type, cs_str message);
API void Client_Kick(Client *client, cs_str reason);
API void Client_KickFormat(Client *client, cs_str fmtreason, ...);
API void Client_UpdateWorldInfo(Client *client, World *world, cs_bool updateAll);
API cs_bool Client_Update(Client *client);
API cs_bool Client_SendHacks(Client *client, CPEHacks *hacks);
API cs_bool Client_MakeSelection(Client *client, cs_byte id, SVec *start, SVec *end, Color4* color);
API cs_bool Client_RemoveSelection(Client *client, cs_byte id);
API cs_bool Client_TeleportTo(Client *client, Vec *pos, Ang *ang);
API cs_bool Client_TeleportToSpawn(Client *client);
API cs_bool Client_CheckState(Client *client, EPlayerState state);

API cs_bool Client_IsLocal(Client *client);
API cs_bool Client_IsInSameWorld(Client *client, Client *other);
API cs_bool Client_IsInWorld(Client *client, World *world);
API cs_bool Client_IsOP(Client *client);
API cs_bool Client_IsFirstSpawn(Client *client);

API cs_bool Client_SetWeather(Client *client, cs_int8 type);
API cs_bool Client_SetInvOrder(Client *client, cs_byte order, BlockID block);
API cs_bool Client_SetServerIdent(Client *client, cs_str name, cs_str motd);
API cs_bool Client_SetEnvProperty(Client *client, cs_byte property, cs_int32 value);
API cs_bool Client_SetEnvColor(Client *client, cs_byte type, Color3* color);
API cs_bool Client_SetTexturePack(Client *client, cs_str url);
API cs_bool Client_AddTextColor(Client *client, Color4* color, cs_char code);
API cs_bool Client_SetBlock(Client *client, SVec *pos, BlockID id);
API cs_bool Client_SetModel(Client *client, cs_int16 model);
API cs_bool Client_SetModelStr(Client *client, cs_str model);
API cs_bool Client_SetBlockPerm(Client *client, BlockID block, cs_bool allowPlace, cs_bool allowDestroy);
API cs_bool Client_SetHeldBlock(Client *client, BlockID block, cs_bool canChange);
API cs_bool Client_SetClickDistance(Client *client, cs_uint16 dist);
API cs_bool Client_SetHotkey(Client *client, cs_str action, cs_int32 keycode, cs_int8 keymod);
API cs_bool Client_SetHotbar(Client *client, cs_byte pos, BlockID block);
API cs_bool Client_SetSkin(Client *client, cs_str skin);
API cs_bool Client_SetSpawn(Client *client, Vec *pos, Ang *ang);
API cs_bool Client_SetOP(Client *client, cs_bool state);
API cs_bool Client_SetVelocity(Client *client, Vec *velocity, cs_bool mode);
API cs_bool Client_SetRotation(Client *client, cs_byte axis, cs_int32 value);
API cs_bool Client_SetGroup(Client *client, cs_int16 gid);
API cs_bool Client_RegisterParticle(Client *client, CustomParticle *e);
API cs_bool Client_SpawnParticle(Client *client, cs_byte id, Vec *pos, Vec *origin);

API cs_str Client_GetName(Client *client);
API cs_str Client_GetAppName(Client *client);
API cs_str Client_GetKey(Client *client);
API cs_str Client_GetSkin(Client *client);
API ClientID Client_GetID(Client *client);
API Client *Client_GetByID(ClientID id);
API Client *Client_GetByName(cs_str name);
API World *Client_GetWorld(Client *client);
API BlockID Client_GetStandBlock(Client *client);
API cs_int8 Client_GetFluidLevel(Client *client, BlockID *fluid);
API cs_int16 Client_GetModel(Client *client);
API BlockID Client_GetHeldBlock(Client *client);
API cs_uint16 Client_GetClickDistance(Client *client);
API cs_float Client_GetClickDistanceInBlocks(Client *client);
API cs_bool Client_GetPosition(Client *client, Vec *pos, Ang *ang);
API cs_int32 Client_GetExtVer(Client *client, cs_uint32 exthash);
API cs_uint32 Client_GetAddr(Client *client);
API cs_int32 Client_GetPing(Client *client);
API cs_float Client_GetAvgPing(Client *client);
API CGroup *Client_GetGroup(Client *client);
API cs_int16 Client_GetGroupID(Client *client);

API cs_bool Client_Spawn(Client *client);
API cs_bool Client_Despawn(Client *client);

VAR Client *Broadcast;
VAR Client *Clients_List[MAX_CLIENTS];
#endif // CLIENT_H
