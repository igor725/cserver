#ifndef CLIENTTYPES_H
#define CLIENTTYPES_H
#include "core.h"
#include "vector.h"
#include "types/cpe.h"
#include "types/list.h"
#include "types/world.h"
#include "types/compr.h"
#include "types/websock.h"
#include "types/netbuffer.h"
#include "types/protocol.h"

#define CLIENT_SELF      (ClientID)-1
#define CLIENT_BROADCAST (Client *)NULL

typedef enum _EMesgType {
	MESSAGE_TYPE_CHAT, // Сообщение в чате
	MESSAGE_TYPE_STATUS1, // Правый верхний угол
	MESSAGE_TYPE_STATUS2,
	MESSAGE_TYPE_STATUS3,
	MESSAGE_TYPE_BRIGHT1 = 11, // Правый нижний угол
	MESSAGE_TYPE_BRIGHT2,
	MESSAGE_TYPE_BRIGHT3,
	MESSAGE_TYPE_ANNOUNCE = 100, // Сообщение в середине экрана
	MESSAGE_TYPE_BIGANNOUNCE,
	MESSAGE_TYPE_SMALLANNOUNCE
} EMesgType;

typedef enum _EClientState {
	CLIENT_STATE_INITIAL, // Игрок только подключился
	CLIENT_STATE_MOTD, // Игрок получает карту
	CLIENT_STATE_INGAME // Игрок находится в игре
} EClientState;

typedef struct _PlayerData {
	cs_char key[65]; // Ключ, полученный от игрока
	cs_char name[65]; // Имя игрока
	cs_char displayname[65]; // Отображаемое имя игрока
	World *world; // Мир, в котором игрок обитает
	Vec position; // Позиция игрока
	Ang angle; // Угол вращения игрока
	cs_bool isOP; // Является ли игрок оператором
	cs_bool spawned; // Заспавнен ли игрок
	cs_bool firstSpawn; // Был лы этот спавн первым с момента захода на сервер
} PlayerData;

typedef struct _PacketData {
	Packet *packet;
	cs_uint16 psize;
	cs_bool isExtended;
} PacketData;

typedef struct _MapData {
	Compr compr; // Штука для сжатия карты
	World *world; // Передаваемая карта
	BlockID *cbptr; // Указатель на место, с которого отправляем карту
	cs_uint32 size; // Размер карты в байтах
	cs_uint32 sent; // Количество отправленных байт
	cs_bool fback; // Нужно ли заменять кастомные блоки
	cs_bool fastmap; // Есть ли дополнение FastMap
} MapData;

typedef struct _Client {
	ClientID id; // Используется в качестве entityid
	EClientState state; // Текущее состояние игрока
	cs_str kickReason; // Причина кика, если имеется
	cs_uint64 lastmsg; // Временная метка последнего полученного от клиента сообщения
	cs_ulong addr; // ipv4 адрес клиента
	NetBuffer netbuf; // Прикол для обмена данными
	PacketData packetData; // Стейт получения пакета от клиента
	MapData mapData; // Стейт отправки карты клиенту
	CPEData cpeData; // CPE-информация о клиенте
	PlayerData playerData; // Создаётся при получении hanshake пакета
	KListField *headNode; // Последняя созданная ассоциативная нода у клиента
	WebSock *websock; // Создаётся, если клиент был определён как браузерный
	Mutex *mutex; // Мьютекс записи, на время отправки пакета клиенту он лочится
} Client;
#endif
