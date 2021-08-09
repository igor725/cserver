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

#define HBEAT_URL "/server/heartbeat/?name=%s&port=%d&users=%d&max=%d&salt=%s&public=%s&web=true&software=%s"
#define PLAY_URL "http://www.classicube.net/server/play/"
#define PLAY_URL_LEN 38

cs_bool PlayURL_OK = false;
cs_char Secret[90] = {0};
cs_uint32 Delay = 5000;

static void NewSecret(cs_uint32 length) {
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
		Secret[i] = (cs_char)Random_Range(&secrnd, min, max);
	}
	Secret[length] = '\0';

	cs_file sfile = File_Open("secret.txt", "w");
	if(sfile) {
		File_Write("#Remove this file if you want to generate new secret key.\n", 58, 1, sfile);
		File_Write("#This key used by the heartbeat as server's \"salt\" for user authentication check.\n", 82, 1, sfile);
		File_Write(Secret, length, 1, sfile);
		File_Close(sfile);
	}
}

cs_str reserved = "!*'();:@&=+$,/?#[]%";
static void TrimReserved(cs_char *name, cs_int32 len) {
	for(cs_int32 i = 0; i < len; i++) {
		cs_char sym = name[i];
		if(sym == '\0') break;
		if(sym == ' ') name[i] = '+';
		if(String_LastChar(reserved, sym)) name[i] = '.';
	}
}

static void DoRequest(void) {
	if(*Secret == '\0') NewSecret(32);
	cs_char reqstr[512], name[65], rsp[1024];
	String_Copy(name, 65, Config_GetStrByKey(Server_Config, CFG_SERVERNAME_KEY));
	TrimReserved(name, 65);

	cs_uint16 port = (cs_uint16)Config_GetInt16ByKey(Server_Config, CFG_SERVERPORT_KEY);
	cs_bool public = Config_GetBoolByKey(Server_Config, CFG_HEARTBEAT_PUBLIC_KEY);
	cs_byte max = (cs_byte)Config_GetInt8ByKey(Server_Config, CFG_MAXPLAYERS_KEY);
	cs_byte count = Clients_GetCount(STATE_INGAME);
	String_FormatBuf(reqstr, 512, HBEAT_URL,
		name, port, count,
		max, Secret,
		public ? "true" : "false",
		SOFTWARE_NAME "%2F" GIT_COMMIT_SHA
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

static const cs_char hexchars[] = "0123456789abcdef";

cs_bool Heartbeat_CheckKey(Client *client) {
	if(*Secret == '\0') return true;
	cs_str key = client->playerData->key;
	cs_str name =  client->playerData->name;

	MD5_CTX ctx;
	cs_byte hash[16];
	cs_char hash_hex[16 * 2 + 1];

	MD5_Init(&ctx);
	MD5_Update(&ctx, Secret, (cs_ulong)String_Length(Secret));
	MD5_Update(&ctx, name, (cs_ulong)String_Length(name));
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
		Thread_Sleep(Delay);
		if(!Server_Active) break;
	}
	return 0;
}

void Heartbeat_Start(cs_uint32 delay) {
	cs_file sfile = File_Open("secret.txt", "r");
	if(sfile) {
		do {
			if(!File_ReadLine(sfile, Secret, 90))
				break;
		} while(*Secret == '#');
		File_Close(sfile);
	}
	Delay = delay * 1000;
	Thread_Create(HeartbeatThread, NULL, true);
}
