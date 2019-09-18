#include <core.h>

#include "http.h"
#include "sha1.h"
#include "b64.h"

SOCKET httpServer;

/*
** Позаимствовано здеся:
** https://github.com/j-ulrich/http-status-codes-cpp/blob/master/HttpStatusCodes_C.h
*/

static const char* getReasonPhrase(int code) {
	switch (code)
	{
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

static void writeHTTPCode(WEBCLIENT* wcl) {
	const char* phrase = getReasonPhrase(wcl->respCode);
	String_FormatBuf(wcl->buffer, 1024, "HTTP/1.1 %d %s\r\n", wcl->respCode, phrase);
	send(wcl->sock, wcl->buffer, String_Length(wcl->buffer), 0);
}

static void writeHTTPHeader(WEBCLIENT* wcl, const char* key, const char* value) {
	String_FormatBuf(wcl->buffer, 1024, "%s: %s\r\n", key, value);
	send(wcl->sock, wcl->buffer, String_Length(wcl->buffer), 0);
}

static void writeDefaultHTTPHeaders(WEBCLIENT* wcl) {
	for(const WEBHEADER* hdr = defaultHeaders; hdr ->key; hdr++) {
		writeHTTPHeader(wcl, hdr->key, hdr->value);
	}
}

static void writeHTTPBody(WEBCLIENT* wcl) {
	if(wcl->respBody && wcl->respLength == 0)
		wcl->respLength = String_Length(wcl->respBody);

	char size[8];
	String_FormatBuf(size, 8, "%d", wcl->respLength);
	writeHTTPHeader(wcl, "Content-Length", size);
	String_Copy(wcl->buffer, 1024, "\r\n");

	if(wcl->respBody && wcl->respLength > 0) {
		String_Append(wcl->buffer, 1024, wcl->respBody);
	}

	send(wcl->sock, wcl->buffer, wcl->respLength + 2, 0);
}

static void GenerateResponse(WEBCLIENT* wcl) {
	writeHTTPCode(wcl);
	writeDefaultHTTPHeaders(wcl);
	if(wcl->wsUpgrade) {
		char acceptKey[64] = {0};
		String_Append(acceptKey, 64, wcl->wsKey);
		String_Append(acceptKey, 64, GUID);

		SHA1_CTX ctx;
		SHA1Init(&ctx);
		SHA1Update(&ctx, acceptKey, String_Length(acceptKey));
		SHA1Final(acceptKey, &ctx);
		const char* b64acceptKey = b64_encode(acceptKey, 20);

		writeHTTPHeader(wcl, "Sec-WebSocket-Protocol", CPL_WSPROTO);
		writeHTTPHeader(wcl, "Sec-WebSocket-Accept", b64acceptKey);
		writeHTTPHeader(wcl, "Upgrade", "websocket");
		writeHTTPHeader(wcl, "Connection", "Upgrade");

		Memory_Free((void*)b64acceptKey);
	}
	writeHTTPBody(wcl);
}

static void freeWsClient(WEBCLIENT* wcl) {
	if(wcl->request)
		Memory_Free((void*)wcl->request);
	if(wcl->wsKey)
		Memory_Free((void*)wcl->wsKey);

	Memory_Free(wcl);
}

static void wclClose(WEBCLIENT* wcl) {
	shutdown(wcl->sock, SD_SEND);
	Socket_Close(wcl->sock);
	freeWsClient(wcl);
}

static char* ReadSockUntil(WEBCLIENT* wcl, size_t len, char sym) {
	char* tmp = wcl->buffer;

	do {
		int ret = recv(wcl->sock, tmp, 1, 0);
		if(ret > 0)
			len -= ret;
		else
			break;
	}
	while(len > 0 && *tmp != sym && (*tmp == '\r' ? tmp : tmp++));

	*tmp = '\0';
	return wcl->buffer;
}

static void wclSetError(WEBCLIENT* wcl, int code, const char* error) {
	wcl->respCode = code;
	wcl->respBody = error;
	wcl->wsUpgrade = false;
}

static void ReadHeader(WEBCLIENT* wcl, const char* value) {
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

static void Handle_GetRequest(WEBCLIENT* wcl, char* buffer) {
	wcl->request = String_AllocCopy(ReadSockUntil(wcl, 512, ' '));
	char* httpver = ReadSockUntil(wcl, 1024, '\n');

	while(wcl->respCode < 400) {
		char* value = ReadSockUntil(wcl, 1024, '\n');
		if(!String_Length(value)) break;

		while(*value != ':') ++value;
		*value = '\0'; value += 2;
		ReadHeader(wcl, value);
	}

	if(wcl->wsUpgrade) {
		wcl->wsFrame = (WSFRAME*)Memory_Alloc(1, sizeof(WSFRAME));
		WebSocket_Setup(wcl->wsFrame, wcl->sock);
		wcl->respCode = 101;
	}

	GenerateResponse(wcl);
}

static TRET ClientThreadProc(TARG param) {
	WEBCLIENT* wcl = (WEBCLIENT*)Memory_Alloc(1, sizeof(WEBCLIENT));
	wcl->respCode = 200;
	wcl->sock = (SOCKET)param;

	int ret = recv(wcl->sock, wcl->buffer, 4, 0);
	if(ret && String_CaselessCompare(wcl->buffer, "get ")) {
		Handle_GetRequest(wcl, wcl->buffer);
	}

	wclClose(wcl);
	return 0;
}

static TRET AcceptThreadProc(TARG param) {
	struct sockaddr_in caddr;
	socklen_t caddrsz = sizeof caddr;

	while(1) {
		SOCKET client = accept(httpServer, (struct sockaddr*)&caddr, &caddrsz);

		if(client != INVALID_SOCKET) {
			Thread_Create(ClientThreadProc, (TARG)client);
		} else
			break;
	}

	Socket_Close(httpServer);
	return 0;
}

bool Http_StartServer(const char* ip, ushort port) {
	httpServer = Socket_Bind(ip, port);
	if(httpServer != INVALID_SOCKET) {
		Thread_Create(AcceptThreadProc, NULL);
		return true;
	}
	return false;
}
