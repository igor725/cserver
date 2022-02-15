#ifndef CLIENTTYPES_H
#define CLIENTTYPES_H
#include "core.h"
#include "vector.h"
#include "types/cpe.h"
#include "types/list.h"
#include "types/world.h"
#include "types/compr.h"
#include "types/websock.h"
#include "types/growingbuffer.h"

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

typedef struct _PlayerData {
	cs_char key[65]; // Ключ, полученный от игрока
	cs_char name[65]; // Имя игрока
	World *world, *reqWorldChange; // Мир, в котором игрок обитает
	Vec position; // Позиция игрока
	Ang angle; // Угол вращения игрока
	EPlayerState state; // Текущее состояние игрока
	cs_bool isOP; // Является ли игрок оператором
	cs_bool spawned; // Заспавнен ли игрок
	cs_bool firstSpawn; // Был лы этот спавн первым с момента захода на сервер
} PlayerData;

typedef struct _Client {
	Waitable *waitend; // Ожидание завершения потока клиента
	cs_bool closed; // В случае значения true сервер прекращает общение с клиентом
	cs_bool noflush; // Приостанавливает отправку пакетов клиенту, что приводит к заполнению буфера
	Socket sock; // Файловый дескриптор сокета клиента
	ClientID id; // Используется в качестве entityid
	cs_ulong pps; // Количество пакетов, отправленных игроком за секунду
	cs_ulong ppstm; // Таймер для счётчика пакетов
	cs_ulong addr; // ipv4 адрес клиента
	Compr compr; // Штука для сжатия карты
	CPEData *cpeData; // В случае vanilla клиента эта структура не создаётся
	PlayerData *playerData; // Создаётся при получении hanshake пакета
	KListField *headNode; // Последняя созданная ассоциативная нода у клиента
	WebSock *websock; // Создаётся, если клиент был определён как браузерный
	Mutex *mutex; // Мьютекс записи, на время отправки пакета клиенту он лочится
	cs_char rdbuf[134]; // Буфер для получения пакетов от клиента
	GrowingBuffer gb; // Буфер отправки пакетов клиенту
} Client;
#endif
