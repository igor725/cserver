#ifndef CLIENT_H
#define CLIENT_H
#include "vector.h"
#include "world.h"
#include "websocket.h"

enum messageTypes {
	MT_CHAT, // Сообщение в чате
	MT_STATUS1, // Правый верхний угол
	MT_STATUS2,
	MT_STATUS3,
	MT_BRIGHT1 = 11, // Правый нижний угол
	MT_BRIGHT2,
	MT_BRIGHT3,
	MT_ANNOUNCE = 100 // Сообщение в середине экрана
};

enum playerStates {
	STATE_INITIAL, // Игрок только подключился
	STATE_MOTD, // Игрок получает карту
	STATE_WLOADDONE, // Карта была успешно получена
	STATE_WLOADERR, // Ошибка при получении карты
	STATE_INGAME // Игрок находится в игре
};

typedef struct _AssocType {
	uint16_t type; // Сам тип, регистрируемый структурой
	struct _AssocType* prev; // Предыдущий тип
	struct _AssocType* next; // Следующий тип
} *AssocType;

typedef struct _AssocNode {
	uint16_t type; // Тип ноды
	void* dataptr; // Поинтер, который ей присвоен
	struct _AssocNode* prev; // Предыдущая нода
	struct _AssocNode* next; // Следующая нода
} *AssocNode;

typedef struct cpeExt {
	const char* name; // Название дополнения
	int32_t version; // Его версия
	uint32_t crc32; // crc32 хеш названия дополнения
	struct cpeExt* next; // Следующее дополнение
} *CPEExt;

typedef struct cpeHacks {
	bool flying, noclip, speeding;
	bool spawnControl, tpv;
	int16_t jumpHeight;
} *Hacks;

typedef struct cpeData {
	const char* appName; // Название игрового клиента
	const char* skinURL; // URL скина игрока, может быть NULL
	CPEExt firstExtension; // Начало списка дополнений клиента
	Hacks hacks; // Структура с значениями чит-параметров для клиента
	char* message; // Используется для получения длинных сообщений
	BlockID heldBlock; // Выбранный игроком блок в данный момент
	int16_t _extCount; // Переменная используется при получении списка дополнений
	int16_t model; // Текущая модель игрока
	bool pingStarted; // Начат ли процесс пингования
	uint16_t pingData; // Данные, цепляемые к пинг-запросу
	uint64_t pingStart; // Время начала пинг-запроса
	uint32_t pingTime; // Сам пинг, в миллисекундах
} *CPEData;

typedef struct playerData {
	int32_t state; // Текущее состояние игрока
	const char* key; // Ключ, полученный от игрока
	const char* name; // Имя игрока
	World world; // Мир, в котором игрок обитает
	Vec position; // Позиция игрока
	Ang angle; // Угол вращения игрока
	bool isOP; // Является ли игрок оператором
	bool spawned; // Заспавнен ли игрок
	bool firstSpawn; // Был лы этот спавн первым с момента захода на сервер
} *PlayerData;

typedef struct client {
	ClientID id; // Используется в качестве entityid
	CPEData cpeData; // В случае vanilla клиента эта структура не создаётся
	PlayerData playerData; // Создаётся при получении hanshake пакета
	AssocNode headNode; // Последняя созданная ассоциативная нода у клиента
	WsClient websock; // Создаётся, если клиент был определён как браузерный
	Mutex* mutex; // Мьютекс записи, на время отправки пакета клиенту он лочится
	bool closed; // В случае значения true сервер прекращает общение с клиентом и удаляет его
	uint32_t addr; // ipv4 адрес клиента
	Socket sock; // Файловый дескриптор сокета клиента
	char* rdbuf; // Буфер для получения пакетов от клиента
	char* wrbuf; // Буфер для отправки пакетов клиенту
	uint32_t pps; // Количество пакетов, отправленных игроком за секунду
	uint32_t ppstm; // Таймер для счётчика пакетов
} *Client;

void Client_UpdatePositions(Client client);
int32_t Client_Send(Client client, int32_t len);
void Client_HandshakeStage2(Client client);
bool Client_CheckAuth(Client client);
TRET Client_ThreadProc(TARG param);
void Client_Free(Client client);
void Client_Tick(Client client);
Client Client_New(Socket fd, uint32_t addr);
bool Client_Add(Client client);
void Client_Init(void);

API uint16_t Assoc_NewType();
API bool Assoc_DelType(uint16_t type, bool freeData);
API bool Assoc_Set(Client client, uint16_t type, void* ptr);
API void* Assoc_GetPtr(Client client, uint16_t type);
API bool Assoc_Remove(Client client, uint16_t type, bool freeData);

API uint8_t Clients_GetCount(int32_t state);
API void Clients_KickAll(const char* reason);
API void Clients_UpdateWorldInfo(World world);

API bool Client_ChangeWorld(Client client, World world);
API void Client_Chat(Client client, MessageType type, const char* message);
API void Client_Kick(Client client, const char* reason);
API bool Client_SendMap(Client client, World world);
API void Client_UpdateWorldInfo(Client client, World world, bool updateAll);
API bool Client_UpdateHacks(Client client);
API bool Client_MakeSelection(Client client, uint8_t id, SVec* start, SVec* end, Color4* color);
API bool Client_RemoveSelection(Client client, uint8_t id);

API bool Client_IsInSameWorld(Client client, Client other);
API bool Client_IsInWorld(Client client, World world);
API bool Client_IsInGame(Client client);
API bool Client_IsOP(Client client);

API bool Client_SetWeather(Client client, Weather type);
API bool Client_SetInvOrder(Client client, Order order, BlockID block);
API bool Client_SetEnvProperty(Client client, uint8_t property, int32_t value);
API bool Client_SetEnvColor(Client client, uint8_t type, Color3* color);
API bool Client_SetTexturePack(Client client, const char* url);
API bool Client_SetBlock(Client client, SVec* pos, BlockID id);
API bool Client_SetModel(Client client, int16_t model);
API bool Client_SetModelStr(Client client, const char* model);
API bool Client_SetBlockPerm(Client client, BlockID block, bool allowPlace, bool allowDestroy);
API bool Client_SetHeld(Client client, BlockID block, bool canChange);
API bool Client_SetHotbar(Client client, Order pos, BlockID block);

API const char* Client_GetName(Client client);
API const char* Client_GetAppName(Client client);
API const char* Client_GetSkin(Client client);
API Client Client_GetByID(ClientID id);
API Client Client_GetByName(const char* name);
API int16_t Client_GetModel(Client client);
API BlockID Client_GetHeldBlock(Client client);
API int32_t Client_GetExtVer(Client client, uint32_t extCRC32);

API bool Client_Spawn(Client client);
API bool Client_Despawn(Client client);

VAR Client Client_Broadcast;
VAR Client Clients_List[MAX_CLIENTS];
#endif
