#include "core.h"
#include "str.h"
#include "platform.h"
#include "http.h"
#include "server.h"
#include "svmath.h"
#include "heartbeat.h"

#define HBEAT_URL "/server/heartbeat/?name=%s&port=%d&users=%d&max=%d&salt=%s&public=false&web=true"

uint32_t Heartbeat_Delay = 5000, Timer = 0;
const char* Heartbeat_URL = NULL;
char Secret[17] = {0};

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
	const char* serverName = Config_GetStr(Server_Config, "server-name");
	int serverPort = Config_GetInt(Server_Config, "server-port");
	String_FormatBuf(path, 128, HBEAT_URL, serverName, serverPort, 0, 127, Secret);

	SOCKET fd = Socket_New();
	req.sock = fd;

	HttpRequest_SetHost(&req, "classicube.net", 80);
	HttpRequest_SetPath(&req, path);
	HttpRequest_SetHeaderStr(&req, "Pragma", "no-cache");
	HttpRequest_SetHeaderStr(&req, "Connection", "close");

	if(HttpRequest_Perform(&req, &resp)) {
		if(resp.bodysize > 0) {
			Log_Info("Body: %d, %s", resp.bodysize, resp.body);
		} else {
			while(Socket_ReceiveLine(fd, path, 128)) {
				Log_Info(path);
			}
		}
	}

	Socket_Close(fd);
	HttpRequest_Cleanup(&req);
	HttpResponse_Cleanup(&resp);
}

void Heartbeat_Tick(void) {
	if(!Heartbeat_Enabled) return;

	if(Timer <= 0) {
		// DoRequest();
		Timer = Heartbeat_Delay;
	}

	Timer -= Server_Delta;
}
