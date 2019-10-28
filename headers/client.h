#ifndef CLIENT_H
#define CLIENT_H
#include "world.h"
#include "websocket.h"

enum playerStates {
	STATE_MOTD, // Игрок получает карту
	STATE_WLOADDONE, // Карта была успешно получена
	STATE_WLOADERR, // Ошибка при получении карты
	STATE_INGAME // Игрок находится в игре
};

typedef struct cpeExt {
	const char* name; // Название дополнения
	int version; // Его версия
	uint32_t crc32; // crc32 хеш названия дополнения
	struct cpeExt*  next; // Следующее по списку дополнение
} *EXT;

typedef struct cpeHacks {
	bool flying, noclip, speeding;
	bool spawnControl, tpv;
	short jumpHeight;
} *HACKS;

typedef struct cpeData {
	const char* appName; // Название игрового клиента
	EXT firstExtension; // Начало списка дополнений клиента
	HACKS hacks; // Структура с значениями чит-параметров для клиента
	char* message; // Используется для получения длинных сообщений
	BlockID heldBlock; // Выбранный игроком блок в данный момент
	short _extCount; // Переменная используется при получении списка дополнений
	int16_t model; // Текущая модель игрока
	bool pingStarted; // Начат ли процесс пингования
	uint16_t pingData; // Данные, цепляемые к пинг-запросу
	uint64_t pingStart; // Время начала пинг-запроса
	uint32_t pingTime; // Сам пинг, в миллисекундах
} *CPEDATA;

typedef struct playerData {
	int state; // Текущее состояние игрока
	const char* key; // Ключ, полученный от игрока
	const char* name; // Имя игрока
	WORLD world; // Мир, в котором игрок обитает
	ANGLE* angle; // Угол вращения игрока
	VECTOR* position; // Позиция игрока
	bool isOP; // Является ли игрок оператором
	bool spawned; // Заспавнен ли игрок
} *PLAYERDATA;

typedef struct client {
	ClientID id; // Используется в качестве entityid
	CPEDATA cpeData; // В случае vanilla клиента эта структура не создаётся
	PLAYERDATA playerData; // Создаётся при получении hanshake пакета
	WSCLIENT websock; // Создаётся, если клиент был определён как браузерный
	MUTEX* mutex; // Мьютекс записи, на время отправки пакета клиенту он лочится
	THREAD thread; // Основной поток клиента, целью которого является чтение пакетов
	THREAD mapThread; // Поток отправки карты игроку
	bool closed; // В случае значения true сервер прекращает общение с клиентом и удаляет его
	uint32_t addr; // ipv4 адрес клиента
	SOCKET sock; // Файловый дескриптор сокета клиента
	char* rdbuf; // Буфер для получения пакетов от клиента
	char* wrbuf; // Буфер для отправки пакетов клиенту
	uint32_t pps; // Количество пакетов, отправленных игроком за секунду
	uint32_t ppstm; // Таймер для счётчика пакетов
} *CLIENT;

void Client_SetPos(CLIENT client, VECTOR* vec, ANGLE* ang);
void Client_UpdatePositions(CLIENT client);
int  Client_Send(CLIENT client, int len);
void Client_HandshakeStage2(CLIENT client);
bool Client_CheckAuth(CLIENT client);
TRET Client_ThreadProc(TARG param);
void Client_Free(CLIENT client);
void Client_Tick(CLIENT client);
CLIENT Client_New(SOCKET fd, uint32_t addr);
bool Client_Add(CLIENT client);
void Client_Init(void);

API uint8_t Clients_GetCount(int state);
API void Clients_KickAll(const char* reason);

API bool Client_ChangeWorld(CLIENT client, WORLD world);
API void Client_Chat(CLIENT client, MessageType type, const char* message);
API void Client_Kick(CLIENT client, const char* reason);
API bool Client_SendMap(CLIENT client, WORLD world);

API bool Client_IsSupportExt(CLIENT client, uint32_t extCRC32, int extVer);
API bool Client_IsInSameWorld(CLIENT client, CLIENT other);
API bool Client_IsInWorld(CLIENT client, WORLD world);
API bool Client_IsInGame(CLIENT client);
API bool Client_IsOP(CLIENT client);

API bool Client_SetWeather(CLIENT client, Weather type);
API bool Client_SetInvOrder(CLIENT client, Order order, BlockID block);
API bool Client_SetProperty(CLIENT client, uint8_t property, int value);
API bool Client_SetTexturePack(CLIENT client, const char* url);
API bool Client_SetBlock(CLIENT client, short x, short y, short z, BlockID id);
API bool Client_SetModel(CLIENT client, int16_t model);
API bool Client_SetModelStr(CLIENT client, const char* model);
API bool Client_SetBlockPerm(CLIENT client, BlockID block, bool allowPlace, bool allowDestroy);
API bool Client_SetHeld(CLIENT client, BlockID block, bool canChange);
API bool Client_SetHotbar(CLIENT client, Order pos, BlockID block);
API bool Client_SetHacks(CLIENT client);

API const char* Client_GetName(CLIENT client);
API const char* Client_GetAppName(CLIENT client);
API CLIENT Client_GetByID(ClientID id);
API CLIENT Client_GetByName(const char* name);
API int16_t Client_GetModel(CLIENT client);

API bool Client_Spawn(CLIENT client);
API bool Client_Despawn(CLIENT client);

VAR CLIENT Client_Broadcast;
VAR CLIENT Clients_List[MAX_CLIENTS];
#endif
