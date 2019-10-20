#include "core.h"
#include "platform.h"
#include "str.h"
#include "http.h"
#include "server.h"

// #define HTTP_TEST
#ifndef HTTP_TEST
int main(int argc, char** argv) {
	if(argc < 2 || !String_CaselessCompare(argv[1], "nochdir")) {
		const char* path = String_AllocCopy(argv[0]);
		char* lastSlash = (char*)String_LastChar(path, PATH_DELIM);
		if(lastSlash) {
			*lastSlash = '\0';
			Directory_SetCurrentDir(path);
		}
		Memory_Free((char*)path);
	}

	Server_InitialWork();
	Server_StartLoop();
	Server_Stop();
	return 0;
}
#else
int main() {
	Socket_Init();
	struct httpRequest req = {0};
	struct httpResponse resp = {0};
	HttpRequest_SetHost(&req, "example.com", 80);
	HttpRequest_SetPath(&req, "/");

	if(HttpRequest_Perform(&req, &resp)) {
		Log_Info("Resp code: %d", resp.code);
		if(resp.body) {
			Log_Info("Received body size: %d", resp.bodysize);
			Log_Info(resp.body);
		}
		return 0;
	}

	Log_Info("HttpRequest_Perform finished with error: %d", resp.error);
	return 0;
}
#endif
