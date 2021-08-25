#include "core.h"
#include "str.h"
#include "log.h"
#include "platform.h"
#include "server.h"
#include "csmath.h"
#include "heartbeat.h"
#include "http.h"
#include "lang.h"
#include "hash.h"

struct _HBKeyCheck {
	heartbeatKeyChecker func;
};

AListField *headHeartbeat = NULL, *headKeyChecker = NULL;

INL static void TrimReserved(cs_char *name, cs_int32 len) {
	for(cs_int32 i = 0; i < len; i++) {
		cs_char sym = name[i];
		if(sym == '\0') break;
		if(sym == ' ') name[i] = '+';
		if(String_FirstChar("!*'();:@&=+$,/?#[]%", sym)) name[i] = '.';
	}
}

INL static cs_bool DoRequest(Heartbeat *hb) {
	if(*hb->secretkey == '\0') return false;
	cs_char reqstr[512], name[65], rsp[1024];
	String_Copy(name, 65, Config_GetStrByKey(Server_Config, CFG_SERVERNAME_KEY));
	TrimReserved(name, 65);

	cs_uint16 port = (cs_uint16)Config_GetInt16ByKey(Server_Config, CFG_SERVERPORT_KEY);
	cs_byte max = (cs_byte)Config_GetInt8ByKey(Server_Config, CFG_MAXPLAYERS_KEY);
	cs_byte count = Clients_GetCount(STATE_INGAME);
	String_FormatBuf(reqstr, 512, hb->templ,
		name, port, count,
		max, hb->secretkey,
		hb->isPublic ? "true" : "false",
		SOFTWARE_NAME "%2F" GIT_COMMIT_SHA
	);

	Http h;
	Memory_Zero(&h, sizeof(Http));
	h.secure = hb->isSecure;

	if(Http_Open(&h, hb->domain)) {
		if(Http_Request(&h, reqstr)) {
			if(Http_ReadResponse(&h, rsp, 1024)) {
				if(String_CaselessCompare2(rsp, hb->playURL, String_Length(hb->playURL))) {
					if(!hb->isPlayURLok) {
						Log_Info(Lang_Get(Lang_ConGrp, 3), rsp);
						hb->isPlayURLok = true;
					}
				} else {
					Log_Error(Lang_Get(Lang_ErrGrp, 3), rsp);
					return false;
				}
			} else {
				Log_Error(Lang_Get(Lang_ErrGrp, 3), "Empty server response");
				return false;
			}
		} else {
			Log_Error(Lang_Get(Lang_ErrGrp, 3), "HTTP request failed");
			return false;
		}
	} else {
		Log_Error(Lang_Get(Lang_ErrGrp, 3), "Can't open HTTP connection");
		return false;
	}

	Http_Cleanup(&h);
	return true;
}

static const cs_char hexchars[] = "0123456789abcdef";

cs_bool Heartbeat_VanillaKeyChecker(cs_str secret, Client *client) {
	cs_str key = client->playerData->key;
	cs_str name =  client->playerData->name;

	MD5_CTX ctx;
	cs_byte hash[16];
	cs_char hash_hex[33];

	if(MD5_Init(&ctx)) {
		MD5_Update(&ctx, secret, (cs_ulong)String_Length(secret));
		MD5_Update(&ctx, name, (cs_ulong)String_Length(name));
		MD5_Final(hash, &ctx);
	} else {
		Log_Error("VanillaKeyChecker: MD5_Init() returned false, can't check user key validity.");
		return false;
	}

	for(cs_int32 i = 0; i < 16; i++) {
		cs_byte b = hash[i];
		hash_hex[i * 2] = hexchars[b >> 4];
		hash_hex[i * 2 + 1] = hexchars[b & 0xF];
	}

	return String_CaselessCompare2(hash_hex, key, 32);
}

THREAD_FUNC(HeartbeatThread) {
	Heartbeat *hb = (Heartbeat *)param;

	while(hb->isOnline) {
		DoRequest(hb);
		Thread_Sleep(hb->delay);
		if(!Server_Active) break;
	}

	if(hb->freeAtEnd) Memory_Free((void *)hb);
	return 0;
}

void Heartbeat_NewSecret(Heartbeat *hb, cs_uint32 length) {
	RNGState secrnd;
	Random_SeedFromTime(&secrnd);

	for(cs_uint32 i = 0; i < length; i++) {
		cs_int32 min, max;
		switch(Random_Range(&secrnd, 0, 3)) {
			case 0:
				min = 48;
				max = 57;
				break;
			case 1:
				min = 65;
				max = 90;
				break;
			default:
				min = 97;
				max = 122;
				break;
		}
		hb->secretkey[i] = (cs_char)Random_Range(&secrnd, min, max);
	}
	hb->secretkey[length] = '\0';

	cs_file sfile = File_Open(hb->secretfile, "w");
	if(sfile) {
		File_Write("#Remove this file if you want to generate new secret key.\n", 58, 1, sfile);
		File_Write("#This key used by the heartbeat as server's \"salt\" for user authentication check.\n", 82, 1, sfile);
		File_Write(hb->secretkey, length, 1, sfile);
		File_Close(sfile);
	}
}

void Heartbeat_AddKeyChecker(heartbeatKeyChecker checker) {
	AList_AddField(&headKeyChecker, (void *)checker);
}

cs_bool Heartbeat_RemoveKeyChecker(heartbeatKeyChecker checker) {
	AListField *tmp;
	List_Iter(tmp, headKeyChecker) {
		if(tmp->value.ptr == (void *)checker) {
			AList_Remove(&headHeartbeat, tmp);
			return true;
		}
	}
	return false;
}

cs_bool Heartbeat_Validate(Client *client) {
	if(!headKeyChecker || !headHeartbeat) return true;
	AListField *kc;
	cs_bool keyvalid = false;

	List_Iter(kc, headKeyChecker) {
		if(keyvalid) break;
		AListField *hb;
		List_Iter(hb, headHeartbeat) {
			if(keyvalid) break;
			Heartbeat *tmp = (Heartbeat *)hb->value.ptr;
			if(*tmp->secretkey != '\0')
				keyvalid = ((struct _HBKeyCheck *)&kc->value.ptr)->func(tmp->secretkey, client);
		}
	}

	return keyvalid;
}

cs_bool Heartbeat_Add(Heartbeat *hb) {
	if(!hb->templ || !hb->playURL || !hb->domain
	|| !hb->secretfile || hb->delay < 1000) return false;
	hb->isPlayURLok = false;
	hb->isOnline = true;

	cs_file sfile = File_Open(hb->secretfile, "r");
	if(sfile) {
		do {
			if(!File_ReadLine(sfile, hb->secretkey, 90))
				break;
		} while(*hb->secretkey == '#');
		File_Close(sfile);
	} else
		Heartbeat_NewSecret(hb, 32);

	hb->thread = Thread_Create(HeartbeatThread, (void *)hb, false);
	AList_AddField(&headHeartbeat, (void *)hb);
	return true;
}

cs_bool Heartbeat_Remove(Heartbeat *hb) {
	hb->isOnline = false;
	AListField *tmp;
	List_Iter(tmp, headHeartbeat) {
		if((Heartbeat *)tmp->value.ptr == hb) {
			AList_Remove(&headHeartbeat, tmp);
			return true;
		}
	}

	return false;
}