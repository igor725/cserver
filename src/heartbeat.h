#ifndef HEARTBEAT_H
#define HEARTBEAT_H
#include "core.h"
#include "client.h"

struct _Heartbeat;
typedef cs_bool(*heartbeatKeyChecker)(cs_str secret, Client *client);

typedef struct _Heartbeat {
	cs_str domain, playURL,
	templ, secretfile;
	cs_char secretkey[90];
	cs_bool isPublic, isSecure,
	isPlayURLok, isOnline, freeAtEnd;
	cs_uint16 delay;
	Thread thread;
} Heartbeat;

API cs_bool Heartbeat_VanillaKeyChecker(cs_str secret, Client *client);
API void Heartbeat_AddKeyChecker(heartbeatKeyChecker checker);
API void Heartbeat_NewSecret(Heartbeat *hb, cs_uint32 length);
API cs_bool Heartbeat_Validate(Client *client);
API cs_bool Heartbeat_Add(Heartbeat *hb);
API cs_bool Heartbeat_Remove(Heartbeat *hb);
#endif // HEARTBEAT_H
