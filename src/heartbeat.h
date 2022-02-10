#ifndef HEARTBEAT_H
#define HEARTBEAT_H
#include "core.h"
#include "client.h"

typedef cs_bool(*heartbeatKeyChecker)(cs_str secret, Client *client);

typedef struct _Heartbeat {
	heartbeatKeyChecker checker;
	cs_str domain, playurl;
	Waitable *isdone;
	cs_bool started,
	ispublic, issecure,
	isannounced;
	cs_uint16 delay;
} Heartbeat;

// typedef struct _Heartbeat {
// 	cs_str domain, playURL,
// 	templ, secretfile;
// 	cs_char secretkey[90];
// 	cs_bool isPublic, isSecure,
// 	isPlayURLok, isOnline, freeAtEnd;
// 	cs_uint16 delay;
// 	Thread thread;
// } Heartbeat;

API Heartbeat *Heartbeat_New(void);
API cs_bool Heartbeat_SetDomain(Heartbeat *self, cs_str url);
API cs_bool Heartbeat_SetPlayURL(Heartbeat *self, cs_str url);
API cs_bool Heartbeat_SetKeyChecker(Heartbeat *self, heartbeatKeyChecker func);
API void Heartbeat_SetPublic(Heartbeat *self, cs_bool state);
API void Heartbeat_SetDelay(Heartbeat *self, cs_uint16 delay);
API cs_bool Heartbeat_Run(Heartbeat *self);
cs_bool Heartbeat_Validate(Client *client);
void Heartbeat_StopAll(void);
// API void Heartbeat_AddKeyChecker(heartbeatKeyChecker checker);
// API void Heartbeat_NewSecret(Heartbeat *hb, cs_uint32 length);
// API cs_bool Heartbeat_Validate(Client *client);
// API cs_bool Heartbeat_Add(Heartbeat *hb);
// API cs_bool Heartbeat_Remove(Heartbeat *hb);
#endif // HEARTBEAT_H
