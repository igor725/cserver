#include "core.h"
#include "http.h"

static HTTPHDR FindHeader(HTTPHDR hdr, const char* key) {
	while(hdr) {
		if(String_CaselessCompare(hdr->key, key)) return hdr;
		hdr = hdr->next;
	}
	return NULL;
}

static HTTPHDR AllocHeader(const char* key) {
	HTTPHDR hdr = Memory_Alloc(1, sizeof(struct httpHeader));
	hdr->key = String_AllocCopy(key);
	return hdr;
}

static void EmptyHeader(HTTPHDR hdr) {
	if(hdr->type == HDRT_STR && hdr->value.vchar)
		Memory_Free((void*)hdr->value.vchar);
	hdr->value.vchar = NULL;
	hdr->type = 0;
}

HTTPHDR HttpRequest_GetHeader(HTTPREQ req, const char* key) {
	HTTPHDR hdr = FindHeader(req->header, key);
	if(!hdr) {
		hdr = AllocHeader(key);
		if(req->header) {
			HTTPHDR ptr = req->header;
			while(ptr)
				if(ptr->next)
					ptr = ptr->next;
				else
					break;
			ptr->next = hdr;
		} else req->header = hdr;
	}
	return hdr;
}

void HttpRequest_SetHeaderStr(HTTPREQ req, const char* key, const char* value) {
	HTTPHDR hdr = HttpRequest_GetHeader(req, key);
	EmptyHeader(hdr);
	hdr->type = HDRT_STR;
	hdr->value.vchar = String_AllocCopy(value);
}

void HttpRequest_SetHeaderInt(HTTPREQ req, const char* key, int value) {
	HTTPHDR hdr = HttpRequest_GetHeader(req, key);
	EmptyHeader(hdr);
	hdr->type = HDRT_INT;
	hdr->value.vint = value;
}

void HttpRequest_SetHost(HTTPREQ req, const char* host, uint16_t port) {
	char hostfmt[22];
	// Socket_SetAddrHost(&req->addr, host, port);
	if(port != 80)
		String_FormatBuf(hostfmt, 22, "%s:%d", host, port);
	else
		String_Copy(hostfmt, 22, host);
	HttpRequest_SetHeaderStr(req, "Host", hostfmt);
}

void HttpRequest_SetPath(HTTPREQ req, const char* path) {
	if(req->path) Memory_Free((void*)req->path);
	req->path = String_AllocCopy(path);
}

static int SendLine(SOCKET sock, char* line) {
	uint32_t len = (uint32_t)String_Length(line);
	return Socket_Send(sock, line, len);
}

void HttpRequest_Perform(HTTPREQ req, HTTPRESP resp) {
	char line[1024];
	req->sock = Socket_New();
	Socket_Connect(req->sock, &req->addr);
	String_FormatBuf(line, 1024, "GET %s HTTP/1.1\r\n", req->path);
	SendLine(req->sock, line);
	HTTPHDR hdr = req->header;
	while(hdr) {
		switch(hdr->type) {
			case HDRT_INT:
				String_FormatBuf(line, 1024, "%s: %d\r\n", hdr->key, hdr->value.vint);
				break;
			case HDRT_STR:
				String_FormatBuf(line, 1024, "%s: %s\r\n", hdr->key, hdr->value.vchar);
				break;
			default:
				return;
		}
		SendLine(req->sock, line);
		hdr = hdr->next;
	}
	String_Copy(line, 1024, "\r\n");
	SendLine(req->sock, line);
	resp->readonly = true;

	if(Socket_ReceiveLine(req->sock, line, 1024)) {
		if(!String_CaselessCompare2(line, "HTTP/1.1 ", 9)) {
			resp->error = HTTP_ERR_INVALID_VERSION;
			return;
		}

		const char* code_start = String_FirstChar(line, ' ');
		char* code_end = (char*)code_start++;
		while((*code_end >= '0') <= '9') ++code_end;
		*code_end = '\0';
		int code = String_ToInt(code_start);

		if(code < 100 || code > 599) {
			resp->error = HTTP_ERR_INVALID_CODE;
			return;
		}

		resp->code = code;
	}
}

void HttpRequest_Cleanup(HTTPREQ req) {
	if(req->path) Memory_Free((void*)req->path);
	if(req->header) {
		HTTPHDR ptr = req->header;

		while(ptr) {
			EmptyHeader(ptr);
			Memory_Free(ptr);
			ptr = ptr->next;
		}
	}
}
