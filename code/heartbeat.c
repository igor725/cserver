#include "core.h"
#include "str.h"
#include "platform.h"
#include "http.h"
#include "server.h"
#include "svmath.h"
#include "heartbeat.h"
#include "client.h"

#define HBEAT_URL "/server/heartbeat/?name=%s&port=%d&users=%d&max=%d&salt=%s&public=%s&web=true&software=%s"
#define PLAY_URL "http://www.classicube.net/server/play/"
#define PLAY_URL_LEN 38

const char* SoftwareName = SOFTWARE_NAME "/" SOFTWARE_VERSION;
uint32_t Delay = 5000;
char Secret[17] = {0};
THREAD Thread;

static void NewSecret(void) {
	RNGState secrnd;
	Random_Seed(&secrnd, (int)Time_GetMSec());
	for(int i = 0; i < 16; i++) {
		int min, max;
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
}

static void DoRequest() {
	if(*Secret == '\0') NewSecret();
	struct httpRequest req = {0};
	struct httpResponse resp = {0};
	char path[128] = {0};
	const char* serverName = Config_GetStr(Server_Config, CFG_SERVERNAME_KEY);
	int serverPort = Config_GetInt(Server_Config, CFG_SERVERPORT_KEY);
	bool serverPublic = Config_GetBool(Server_Config, CFG_HEARTBEAT_PUBLIC_KEY);
	uint8_t serverMax = Config_GetInt8(Server_Config, CFG_MAXPLAYERS_KEY);
	uint8_t count = Clients_GetCount(STATE_INGAME);
	String_FormatBuf(path, 128, HBEAT_URL, serverName, serverPort, count, serverMax, Secret, serverPublic ? "true" : "false", SoftwareName);

	SOCKET fd = Socket_New();
	req.sock = fd;

	HttpRequest_SetHost(&req, "classicube.net", 80);
	HttpRequest_SetPath(&req, path);
	HttpRequest_SetHeaderStr(&req, "Pragma", "no-cache");
	HttpRequest_SetHeaderStr(&req, "Connection", "close");
	HttpRequest_SetHeaderStr(&req, "User-Agent", SOFTWARE_FULLNAME);

	if(HttpRequest_Perform(&req, &resp)) {
		if(!Heartbeat_URL && resp.body) {
			if(String_CaselessCompare2(resp.body, PLAY_URL, PLAY_URL_LEN)) {
				Heartbeat_URL = String_AllocCopy(resp.body);
				Log_Info("Server play URL: %s", resp.body);
			}
		}
	}

	Socket_Close(fd);
	HttpRequest_Cleanup(&req);
	HttpResponse_Cleanup(&resp);
}

static TRET HeartbeatThreadProc(TARG param) {
	(void)param;
	while(Server_Active) {
		DoRequest();
		Sleep(Delay);
	}
	return 0;
}

void Heartbeat_Start(uint32_t delay) {
	Delay = delay * 1000;
	Thread = Thread_Create(HeartbeatThreadProc, NULL);
}

void Heartbeat_Close(void) {
	if(Thread_IsValid(Thread)) Thread_Close(Thread);
}
