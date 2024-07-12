#include "core.h"
#include "str.h"
#include "log.h"
#include "platform.h"
#include "server.h"
#include "csmath.h"
#include "cserror.h"
#include "heartbeat.h"
#include "http.h"
#include "hash.h"
#include "strstor.h"
#include "list.h"
#include "client.h"
#include "config.h"

#define REQUEST "%s?name=%s&port=%u&users=%d&max=%u&salt=%s&public=%s&web=true&software=%s&version=7"

static AListField *headHeartbeat = NULL;
static Mutex *gLock = NULL;
static cs_bool inited = false;

static void NewSecret(Heartbeat *self) {
	RNGState secrnd;
	Random_SeedFromTime(&secrnd);

	for(cs_uint32 i = 0; i < HEARTBEAT_SECRET_LENGTH; i++) {
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
		self->secret[i] = (cs_char)Random_Range(&secrnd, min, max);
	}
	self->secret[HEARTBEAT_SECRET_LENGTH - 1] = '\0';
	self->keychanged = true;
}

INL static void TrimReserved(cs_char *name) {
	for(cs_int32 i = 0; name[i] != '\0'; i++) {
		if(name[i] == ' ') name[i] = '+';
		else if(String_FirstChar("!*'();:@&=+$,/?#%", name[i])) name[i] = '.';
	}
}

static cs_file openHeartbeatSecret(Heartbeat *self, cs_str mode) {
	cs_char path[MAX_PATH_LEN];
	if(!String_FormatBuf(path, MAX_PATH_LEN, "secrets/%s.txt", self->domain)) {
		Error_PrintSys(false);
		return NULL;
	}

	return File_Open(path, mode);
}

INL static void searchForSecretKey(Heartbeat *self) {
	Mutex_Lock(gLock);
	self->keychanged = false;
	cs_file sfile = openHeartbeatSecret(self, "r");
	if(sfile) {
		cs_int32 len = File_ReadLine(sfile, self->secret, HEARTBEAT_SECRET_LENGTH);
		if(len == 0) NewSecret(self);
		File_Close(sfile);
	} else NewSecret(self);
	Mutex_Unlock(gLock);
}

static const cs_char hexchars[] = "0123456789abcdef";

static cs_bool VanillaKeyChecker(cs_str secret, Client *client) {
	cs_str name =  Client_GetName(client);
	cs_str key = Client_GetKey(client);

	MD5_CTX ctx;
	cs_byte hash[16];
	cs_char hash_hex[33];

	if(MD5_Start(&ctx)) {
		MD5_PushData(&ctx, secret, (cs_ulong)String_Length(secret));
		MD5_PushData(&ctx, name, (cs_ulong)String_Length(name));
		MD5_End(hash, &ctx);
	} else {
		Log_Error(Sstor_Get("HBEAT_KEYCHECK_ERR"));
		return false;
	}

	for(cs_int32 i = 0; i < 16; i++) {
		cs_byte b = hash[i];
		hash_hex[i * 2] = hexchars[b >> 4];
		hash_hex[i * 2 + 1] = hexchars[b & 0xF];
	}

	return String_CaselessCompare2(hash_hex, key, 32);
}

static void MakeHeartbeatRequest(Heartbeat *self) {
	cs_char reqstr[512], name[MAX_STR_LEN], rsp[1024];
	String_Copy(name, MAX_STR_LEN, Config_GetStrByKey(Server_Config, CFG_SERVERNAME_KEY));
	TrimReserved(name);

	cs_int32 port = (cs_int32)Config_GetIntByKey(Server_Config, CFG_SERVERPORT_KEY);
	cs_int16 max = (cs_int16)Config_GetIntByKey(Server_Config, CFG_MAXPLAYERS_KEY);
	cs_byte count = Clients_GetCount(CLIENT_STATE_INGAME);

	if(String_FormatBuf(reqstr, 512, REQUEST,
		self->reqpath, name, port, count,
		max, self->secret, self->ispublic ? "True" : "False",
		SOFTWARE_NAME "%2F" GIT_COMMIT_TAG
	) == -1) {
		Log_Info(Sstor_Get("HBEAT_ERR"), "String_FormatBuf failed");
		return;
	}

	Http h = {
		.secure = self->issecure
	};

	if(!Http_Open(&h, self->domain)) {
		Log_Error(Sstor_Get("HBEAT_ERR"), Sstor_Get("HBEAT_ERR_CF"));
		goto httpend;
	}

	if(!Http_Request(&h, reqstr)) {
		Log_Error(Sstor_Get("HBEAT_ERR"), Sstor_Get("HBEAT_ERR_HF"));
		goto httpend;
	}

	if(Http_ReadResponse(&h, rsp, 1024) == 0) {
		Log_Error(Sstor_Get("HBEAT_ERR"), Sstor_Get("HBEAT_ERR_ER"));
		goto httpend;
	}

	if(!self->isannounced) {
		cs_char *url = String_FindSubstr(rsp, self->playurl);
		if(url) {
			Log_Info(Sstor_Get("HBEAT_URL"), url);
			self->isannounced = true;
		}
	}

	httpend:
	Http_Cleanup(&h);
	return;
}

THREAD_FUNC(HbeatThread) {
	Heartbeat *self = (Heartbeat *)param;
	cs_uint16 delay = 0;

	while(self->started && Server_Active) {
		if(delay == 0) {
			MakeHeartbeatRequest(self);
			delay = self->delay;
		} else {
			Thread_Sleep(50);
			delay -= 50;
		}
	}

	Waitable_Signal(self->isdone);
	return 0;
}

Heartbeat *Heartbeat_New(void) {
	return Memory_Alloc(1, sizeof(Heartbeat));
}

cs_bool Heartbeat_SetDomain(Heartbeat *self, cs_str domain) {
	if(self->started) return false;
	if(self->domain) Memory_Free((void *)self->domain);
	self->domain = String_AllocCopy(domain);
	return true;
}

cs_bool Heartbeat_SetRequestPath(Heartbeat *self, cs_str path) {
	if(self->started) return false;
	if(self->reqpath) Memory_Free((void *)self->reqpath);
	self->reqpath = String_AllocCopy(path);
	return true;
}

cs_bool Heartbeat_SetPlayURL(Heartbeat *self, cs_str url) {
	if(self->started) return false;
	if(self->playurl) Memory_Free((void *)self->playurl);
	self->playurl = String_AllocCopy(url);
	return true;
}

void Heartbeat_SetPublic(Heartbeat *self, cs_bool state) {
	self->ispublic = state;
}

void Heartbeat_SetDelay(Heartbeat *self, cs_uint16 delay) {
	self->delay = delay;
}

cs_bool Heartbeat_SetKeyChecker(Heartbeat *self, heartbeatKeyChecker func) {
	if(self->started) return false;
	self->checker = func;
	return true;
}

cs_bool Heartbeat_Run(Heartbeat *self) {
	if(self->started || !self->playurl || !self->domain || !self->reqpath)
		return false;

	if(!inited) {
		gLock = Mutex_Create();
		inited = true;
	}

	Mutex_Lock(gLock);
	AListField *kc;
	List_Iter(kc, headHeartbeat) {
		Heartbeat *other = kc->value.ptr;
		if(String_CaselessCompare(self->domain, other->domain))
			return false;
	}

	if(!self->checker)
		self->checker = VanillaKeyChecker;

	if(*self->secret == '\0')
		searchForSecretKey(self);

	self->started = true;
	self->isdone = Waitable_Create();
	Thread_Create(HbeatThread, self, true);
	AList_AddField(&headHeartbeat, self);
	Mutex_Unlock(gLock);
	return true;
}

static void freeMemory(Heartbeat *self) {
	if(self->started) {
		self->started = false;
		Waitable_Wait(self->isdone);
	}

	if(self->domain) Memory_Free((void *)self->domain);
	if(self->reqpath) Memory_Free((void *)self->reqpath);
	if(self->playurl) Memory_Free((void *)self->playurl);
	Waitable_Free(self->isdone);
	Memory_Free(self);
}

static void saveHeartbeatKey(Heartbeat *self) {
	if(self->keychanged) {
		cs_file sfile = openHeartbeatSecret(self, "w");
		if(!sfile) {
			Error_PrintSys(false);
			return;
		}
		File_Write(self->secret, String_Length(self->secret), 1, sfile);
		File_Write("\r\n", 2, 1, sfile);
		File_Close(sfile);
		self->keychanged = false;
	}
}

void Heartbeat_Close(Heartbeat *self) {
	saveHeartbeatKey(self);
	freeMemory(self);
	AListField *kc;
	Mutex_Lock(gLock);
	List_Iter(kc, headHeartbeat) {
		if(kc->value.ptr == self) {
			AList_Remove(&headHeartbeat, kc);
			break;
		}
	}
	Mutex_Unlock(gLock);
}

cs_bool Heartbeat_Validate(Client *client) {
	if(!headHeartbeat || Client_IsLocal(client)) return true;

	AListField *kc;
	List_Iter(kc, headHeartbeat) {
		Heartbeat *self = (Heartbeat *)kc->value.ptr;
		if(self->checker(self->secret, client)) return true;
	}

	return false;
}

void Heartbeat_StopAll(void) {
	if(!inited) return;
	Mutex_Lock(gLock);
	while(headHeartbeat != NULL) {
		saveHeartbeatKey(headHeartbeat->value.ptr);
		freeMemory(headHeartbeat->value.ptr);
		AList_Remove(&headHeartbeat, headHeartbeat);
	}
	Mutex_Unlock(gLock);
	Mutex_Free(gLock);
	inited = false;
}
