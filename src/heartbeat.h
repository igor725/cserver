#ifndef HEARTBEAT_H
#define HEARTBEAT_H
#include "core.h"
#include "client.h"

struct _Heartbeat;
typedef cs_bool(*heartbeatKeyChecker)(struct _Heartbeat *hb, Client *client);

typedef struct _Heartbeat {
	cs_str domain, playURL,
	template, secretfile;
	cs_char secretkey[90];
	heartbeatKeyChecker validate;
	cs_bool isPublic, isSecure, isPlayURLok;
	cs_uint16 delay;
} Heartbeat;

API cs_bool Heartbeat_VanillaKeyChecker(Heartbeat *hb, Client *client);
API void Heartbeat_NewSecret(Heartbeat *hb, cs_uint32 length);
API cs_bool Heartbeat_Run(Heartbeat *hb);
#endif // HEARTBEAT_H
