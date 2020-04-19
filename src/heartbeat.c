#include "core.h"
#include "str.h"
#include "log.h"
#include "platform.h"
#include "server.h"
#include "random.h"
#include "heartbeat.h"
#include "http.h"
#include "lang.h"
#include "hash.h"

#define HBEAT_URL "/server/heartbeat/?name=%s&port=%d&users=%d&max=%d&salt=%s&public=%s&web=true&software=%s"
#define PLAY_URL "http://www.classicube.net/server/play/"
#define PLAY_URL_LEN 38

cs_bool PlayURL_OK = false;
char Secret[17] = {0};
cs_uint32 Delay = 5000;

static void NewSecret(void) {
	RNGState secrnd;
	Random_SeedFromTime(&secrnd);
	for(cs_int32 i = 0; i < 17; i++) {
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
		Secret[i] = (char)Random_Range(&secrnd, min, max);
	}

	FILE *sfile = File_Open("secret.txt", "w");
	if(sfile) {
		File_Write("#Remove this file if you want to generate new secret key.\n", 58, 1, sfile);
		File_Write(Secret, 16, 1, sfile);
		File_Close(sfile);
	}
}

cs_str reserved = "!*'();:@&=+$,/?#[]%";
static void TrimReserved(char *name, cs_int32 len) {
	for(cs_int32 i = 0; i < len; i++) {
		char sym = name[i];
		if(sym == '\0') break;
		if(sym == ' ') name[i] = '+';
		if(String_LastChar(reserved, sym)) name[i] = '.';
	}
}

static void DoRequest() {
	if(*Secret == '\0') NewSecret();
	char reqstr[512], name[65], rsp[1024];
	String_Copy(name, 65, Config_GetStrByKey(Server_Config, CFG_SERVERNAME_KEY));
	TrimReserved(name, 65);

	cs_uint16 port = Config_GetInt16ByKey(Server_Config, CFG_SERVERPORT_KEY);
	cs_bool public = Config_GetBoolByKey(Server_Config, CFG_HEARTBEAT_PUBLIC_KEY);
	cs_byte max = Config_GetInt8ByKey(Server_Config, CFG_MAXPLAYERS_KEY);
	cs_byte count = Clients_GetCount(STATE_INGAME);
	String_FormatBuf(reqstr, 512, HBEAT_URL,
		name, port, count,
		max, Secret,
		public ? "true" : "false",
		SOFTWARE_NAME "%%47" GIT_COMMIT_SHA
	);

	Http h;
	Memory_Zero(&h, sizeof(Http));
	h.secure = true;

	if(Http_Open(&h, "classicube.net")) {
		if(Http_Request(&h, reqstr)) {
			if(Http_ReadResponse(&h, rsp, 1024)) {
				if(String_CaselessCompare2(rsp, PLAY_URL, PLAY_URL_LEN)) {
					if(!PlayURL_OK) {
						Log_Info(Lang_Get(Lang_ConGrp, 3), rsp);
						PlayURL_OK = true;
					}
				} else
					Log_Error(Lang_Get(Lang_ErrGrp, 3), rsp);
			} else
				Log_Error(Lang_Get(Lang_ErrGrp, 3), "Empty server response");
		} else
			Log_Error(Lang_Get(Lang_ErrGrp, 3), "HTTP request failed");
	} else
		Log_Error(Lang_Get(Lang_ErrGrp, 3), "Can't open HTTP connection");
	Http_Cleanup(&h);
}

static const char hexchars[] = "0123456789abcdef";

cs_bool Heartbeat_CheckKey(Client *client) {
	if(*Secret == '\0') return true;
	cs_str key = client->playerData->key;
	cs_str name =  client->playerData->name;

	MD5_CTX ctx;
	cs_byte hash[16];
	char hash_hex[16 * 2 + 1];

	MD5_Init(&ctx);
	MD5_Update(&ctx, Secret, String_Length(Secret));
	MD5_Update(&ctx, name, String_Length(name));
	MD5_Final(hash, &ctx);

	for(cs_int32 i = 0; i < 16; i++) {
		cs_byte b = hash[i];
		hash_hex[i * 2] = hexchars[b >> 4];
		hash_hex[i * 2 + 1] = hexchars[b & 0xF];
	}

	hash_hex[16 * 2] = '\0';
	return String_CaselessCompare(hash_hex, key);
}

THREAD_FUNC(HeartbeatThread) {
	(void)param;
	while(true) {
		DoRequest();
		Sleep(Delay);
		if(!Server_Active) break;
	}
	return 0;
}

void Heartbeat_Start(cs_uint32 delay) {
	FILE *sfile = File_Open("secret.txt", "r");
	if(sfile) {
		File_Seek(sfile, 59, SEEK_SET);
		File_Read(Secret, 16, 1, sfile);
		File_Close(sfile);
	}
	Delay = delay * 1000;
	Thread_Create(HeartbeatThread, NULL, true);
}
