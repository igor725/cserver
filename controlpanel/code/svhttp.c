#include <core.h>
#include <minizip/unzip.h>

#include "svhttp.h"
#include "sha1.h"

SOCKET httpServer = INVALID_SOCKET;
MUTEX* zMutex;
unzFile zData;

/*
** Позаимствовано здеся:
** https://github.com/j-ulrich/http-status-codes-cpp/blob/master/HttpStatusCodes_C.h
*/

static const char* getReasonPhrase(int code) {
	switch (code) {
		case 100: return "Continue";
		case 101: return "Switching Protocols";
		case 102: return "Processing";
		case 103: return "Early Hints";

		case 200: return "OK";
		case 201: return "Created";
		case 202: return "Accepted";
		case 203: return "Non-Authoritative Information";
		case 204: return "No Content";
		case 205: return "Reset Content";
		case 206: return "Partial Content";
		case 207: return "Multi-Status";
		case 208: return "Already Reported";
		case 226: return "IM Used";

		case 300: return "Multiple Choices";
		case 301: return "Moved Permanently";
		case 302: return "Found";
		case 303: return "See Other";
		case 304: return "Not Modified";
		case 305: return "Use Proxy";
		case 307: return "Temporary Redirect";
		case 308: return "Permanent Redirect";

		case 400: return "Bad Request";
		case 401: return "Unauthorized";
		case 402: return "Payment Required";
		case 403: return "Forbidden";
		case 404: return "Not Found";
		case 405: return "Method Not Allowed";
		case 406: return "Not Acceptable";
		case 407: return "Proxy Authentication Required";
		case 408: return "Request Timeout";
		case 409: return "Conflict";
		case 410: return "Gone";
		case 411: return "Length Required";
		case 412: return "Precondition Failed";
		case 413: return "Payload Too Large";
		case 414: return "URI Too Long";
		case 415: return "Unsupported Media Type";
		case 416: return "Range Not Satisfiable";
		case 417: return "Expectation Failed";
		case 418: return "I'm a teapot";
		case 422: return "Unprocessable Entity";
		case 423: return "Locked";
		case 424: return "Failed Dependency";
		case 426: return "Upgrade Required";
		case 428: return "Precondition Required";
		case 429: return "Too Many Requests";
		case 431: return "Request Header Fields Too Large";
		case 451: return "Unavailable For Legal Reasons";

		case 500: return "Internal Server Error";
		case 501: return "Not Implemented";
		case 502: return "Bad Gateway";
		case 503: return "Service Unavailable";
		case 504: return "Gateway Time-out";
		case 505: return "HTTP Version Not Supported";
		case 506: return "Variant Also Negotiates";
		case 507: return "Insufficient Storage";
		case 508: return "Loop Detected";
		case 510: return "Not Extended";
		case 511: return "Network Authentication Required";

		default: return 0;
	}
}

static const WEBHEADER defaultHeaders[] = {
	{"Server", HTTP_SERVER_NAME},
	{"Allow", "GET"},
	{NULL, NULL}
};

static int sendBuffer(WEBCLIENT wcl, size_t len) {
	return send(wcl->sock, wcl->buffer, (uint32_t)len, 0);
}

static void writeHTTPCode(WEBCLIENT wcl) {
	const char* phrase = getReasonPhrase(wcl->respCode);
	String_FormatBuf(wcl->buffer, HTTP_BUFFER_LEN, "HTTP/1.1 %d %s\r\n", wcl->respCode, phrase);
	sendBuffer(wcl, String_Length(wcl->buffer));
}

static void writeHTTPHeader(WEBCLIENT wcl, const char* key, const char* value) {
	String_FormatBuf(wcl->buffer, HTTP_BUFFER_LEN, "%s: %s\r\n", key, value);
	sendBuffer(wcl, String_Length(wcl->buffer));
}

static void writeDefaultHTTPHeaders(WEBCLIENT wcl) {
	for(const WEBHEADER* hdr = defaultHeaders; hdr->key; hdr++) {
		writeHTTPHeader(wcl, hdr->key, hdr->value);
	}
}

static void writeHTTPBody(WEBCLIENT wcl) {
	if(!wcl->wsUpgrade) {
		if(wcl->respBody && wcl->respLength == 0)
			wcl->respLength = String_Length(wcl->respBody);

		char size[8];
		String_FormatBuf(size, 8, "%d", wcl->respLength);
		writeHTTPHeader(wcl, "Content-Length", size);
	}

	String_Copy(wcl->buffer, HTTP_BUFFER_LEN, "\r\n");
	sendBuffer(wcl, 2);

	if(!wcl->wsUpgrade) {
		if(wcl->respBody && wcl->respLength > 0) {
			String_Append(wcl->buffer, HTTP_BUFFER_LEN, wcl->respBody);
		}
		sendBuffer(wcl, wcl->respLength + 2);
	}
}

static bool SendZippedFile(WEBCLIENT wcl, const char* name) {
	unz_file_info info;
	char size[8];
	uint32_t done = 0;

	Mutex_Lock(zMutex);
	if(unzLocateFile(zData, name, false) == UNZ_OK) {
		unzOpenCurrentFile(zData);
		unzGetCurrentFileInfo(zData, &info, NULL, 0, NULL, 0, NULL, 0);
		writeHTTPCode(wcl);
		writeDefaultHTTPHeaders(wcl);
		String_FormatBuf(size, 8, "%d", info.uncompressed_size);
		writeHTTPHeader(wcl, "Content-Length", size);
		writeHTTPHeader(wcl, "Content-Type", "text/html");
		String_Copy(wcl->buffer, HTTP_BUFFER_LEN, "\r\n");
		sendBuffer(wcl, 2);

		while(done != info.uncompressed_size) {
			uint32_t blocksz = min(1024, info.uncompressed_size - done);
			if(unzReadCurrentFile(zData, wcl->buffer, blocksz) > 0) {
				sendBuffer(wcl, blocksz);
				done += blocksz;
			}
		}

		unzCloseCurrentFile(zData);
		Mutex_Unlock(zMutex);
		return true;
	}

	Mutex_Unlock(zMutex);
	return false;
}

const char b64chars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

static char* SHA1toB64(uint8_t* in) {
	char* out = (char*)in + 20;
	out[28] = '\0';

	for (int i = 0, j = 0; i < 20; i += 3, j += 4) {
		int v = in[i];
		v = i + 1 < 20 ? v << 8 | in[i + 1] : v << 8;
		v = i + 2 < 20 ? v << 8 | in[i + 2] : v << 8;

		out[j] = b64chars[(v >> 18) & 0x3F];
		out[j + 1] = b64chars[(v >> 12) & 0x3F];
		if (i + 1 < 20) {
			out[j + 2] = b64chars[(v >> 6) & 0x3F];
		} else {
			out[j + 2] = '=';
		}
		if (i + 2 < 20) {
			out[j + 3] = b64chars[v & 0x3F];
		} else {
			out[j + 3] = '=';
		}
	}

	return out;
}

static void GenerateResponse(WEBCLIENT wcl) {
	writeHTTPCode(wcl);
	writeDefaultHTTPHeaders(wcl);

	if(wcl->wsUpgrade) {
		char acceptKey[64];
		String_Copy(acceptKey, 64, wcl->wsKey);
		String_Append(acceptKey, 64, GUID);

		SHA1_CTX ctx;
		SHA1Init(&ctx);
		SHA1Update(&ctx, (uint8_t*)acceptKey, (uint32_t)String_Length(acceptKey));
		SHA1Final((uint8_t*)acceptKey, &ctx);

		writeHTTPHeader(wcl, "Sec-WebSocket-Protocol", CPL_WSPROTO);
		writeHTTPHeader(wcl, "Sec-WebSocket-Accept", SHA1toB64((uint8_t*)acceptKey));
		writeHTTPHeader(wcl, "Upgrade", "websocket");
		writeHTTPHeader(wcl, "Connection", "Upgrade");
	}

	writeHTTPBody(wcl);
}

static void freeWsClient(WEBCLIENT wcl) {
	if(wcl->request) Memory_Free((void*)wcl->request);
	if(wcl->wsKey) Memory_Free((void*)wcl->wsKey);
	if(wcl->wsFrame) WebSocket_FreeFrame(wcl->wsFrame);
	Memory_Free(wcl);
}

static void wclClose(WEBCLIENT wcl) {
	shutdown(wcl->sock, SD_SEND);
	Socket_Close(wcl->sock);
	freeWsClient(wcl);
}

static char* ReadSockUntil(WEBCLIENT wcl, size_t len, char sym) {
	char* tmp = wcl->buffer;

	do {
		int ret = recv(wcl->sock, tmp, 1, 0);
		if(ret > 0)
			len -= ret;
		else
			break;
	} while(len > 0 && *tmp != sym && (*tmp == '\r' ? tmp : tmp++));

	*tmp = '\0';
	return wcl->buffer;
}

static void wclSetError(WEBCLIENT wcl, int code, const char* error) {
	wcl->respCode = code;
	wcl->respBody = error;
	wcl->wsUpgrade = false;
}

static void ReadHeader(WEBCLIENT wcl, const char* value) {
	const char* key = wcl->buffer;

	if(String_CaselessCompare(key, "Upgrade")) {
		if(String_FindSubstr(value, "websocket")) {
			wcl->wsUpgrade = true;
		}
	} else if(String_CaselessCompare(key, "Sec-WebSocket-Key")) {
		wcl->wsKey = String_AllocCopy(value);
	} else if(String_CaselessCompare(key, "Sec-WebSocket-Protocol")) {
		if(!String_FindSubstr(value, CPL_WSPROTO)) {
			wclSetError(wcl, 400, "Unknown websocket protocol.");
		}
	} else if(String_CaselessCompare(key, "Sec-WebSocket-Version")) {
		wcl->wsVersion = String_ToInt(value);
		if(wcl->wsVersion != 13) {
			wclSetError(wcl, 400, "Invalid WebSocket-Version.");
		}
	}
}

static void sendFrame(WEBCLIENT wcl, const char* data, uint32_t dlen, char opcode) {
	uint32_t len = WebSocket_Encode(wcl->buffer, 1024, data, dlen, opcode);
	if(len) sendBuffer(wcl, len);
}

static bool HandleWebSocketFrame(WEBCLIENT wcl) {
	WSFRAME ws = wcl->wsFrame;
	if(ws->opcode == 0x08) return true;

	return false;
}

/*
** Пока что параметры URL нам не пригодятся
** от слова совсем. Я, например, не могу
** придумать, где их можно было бы
** применить вместо вебсокет-сообщений.
** Так что пусть весь остальной код считает,
** что этих параметров нет, даже если они
** есть.
*/
static char* TrimParams(char* str) {
	char* tmp = str;
	while(*str++ != '\0') {
		if(*str == '?') {
			*str = '\0';
			break;
		}
	}
	return tmp;
}

static void HandleGetRequest(WEBCLIENT wcl, char* buffer) {
	wcl->request = String_AllocCopy(TrimParams(ReadSockUntil(wcl, 512, ' ')));
	char* httpver = ReadSockUntil(wcl, HTTP_BUFFER_LEN, '\n');

	if(!String_CaselessCompare(httpver, "HTTP/1.1")) {
		wclSetError(wcl, 505, "Invalid HTTP version");
		return;
	}

	while(wcl->respCode < 400) {
		char* value = ReadSockUntil(wcl, HTTP_BUFFER_LEN, '\n');
		if(!String_Length(value)) break;

		while(*value != ':') ++value;
		*value = '\0'; value += 2;
		ReadHeader(wcl, value);
	}

	if(wcl->wsUpgrade) {
		wcl->wsFrame = Memory_Alloc(1, sizeof(struct wsFrame));
		WebSocket_Setup(wcl->wsFrame, wcl->sock);
		wcl->respCode = 101;
		GenerateResponse(wcl);

		while(1) {
			if(WebSocket_ReceiveFrame(wcl->wsFrame)) {
				if(!HandleWebSocketFrame(wcl)) continue;
			}
			break;
		}
	} else {
		const char* filename = "index.html";
		if(!String_CaselessCompare(wcl->request, "/")) {
			filename = wcl->request + 1;
		}
		if(!SendZippedFile(wcl, filename)) {
			wclSetError(wcl, 404, "Specified file not found in " CPL_ZIP);
			GenerateResponse(wcl);
		}
	}
}

static TRET ClientThreadProc(TARG param) {
	WEBCLIENT wcl = Memory_Alloc(1, sizeof(struct webClient));
	wcl->respCode = 200;
	wcl->sock = (SOCKET)param;

	int ret = recv(wcl->sock, wcl->buffer, 4, 0);
	if(ret && String_CaselessCompare(wcl->buffer, "get "))
		HandleGetRequest(wcl, wcl->buffer);

	wclClose(wcl);
	return 0;
}

static TRET AcceptThreadProc(TARG param) {
	struct sockaddr_in caddr;
	socklen_t caddrsz = sizeof(caddr);

	while(1) {
		SOCKET client = accept(httpServer, (struct sockaddr*)&caddr, &caddrsz);

		if(client != INVALID_SOCKET) {
			Thread_Create(ClientThreadProc, (TARG)client);
		} else
			break;
	}

	Http_CloseServer();
	return 0;
}

bool Http_StartServer(const char* ip, uint16_t port) {
	zData = unzOpen(CPL_ZIP);
	zMutex = Mutex_Create();
	if(!zData) {
		Log_Error("Can't open " CPL_ZIP);
		return false;
	}

	httpServer = Socket_Bind(ip, port);
	if(httpServer != INVALID_SOCKET) {
		Thread_Create(AcceptThreadProc, NULL);
		return true;
	}
	return false;
}

bool Http_CloseServer() {
	if(zMutex) Mutex_Free(zMutex);
	if(zData) unzClose(zData);
	if(httpServer != INVALID_SOCKET) {
		Socket_Close(httpServer);
		return true;
	}
	return false;
}
