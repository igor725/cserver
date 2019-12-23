#include "core.h"
#include "platform.h"
#include "str.h"
#include "http.h"

/*
** Позаимствовано здеся:
** https://github.com/j-ulrich/http-status-codes-cpp/blob/master/HttpStatusCodes_C.h
*/

const char* HttpCode_GetReason(cs_int32 code) {
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

		default: return NULL;
	}
}

static HTTPHeader* FindHeader(HTTPHeader* hdr, const char* key) {
	HTTPHeader* ptr = hdr;
	while(ptr) {
		if(String_CaselessCompare(ptr->key, key)) return ptr;
		ptr = ptr->next;
	}
	return NULL;
}

static HTTPHeader* AllocHeader(const char* key) {
	HTTPHeader* hdr = Memory_Alloc(1, sizeof(HTTPHeader));
	hdr->key = String_AllocCopy(key);
	return hdr;
}

static void EmptyHeader(HTTPHeader* hdr) {
	if(hdr->type == HDRT_STR && hdr->value.vchar)
		Memory_Free((void*)hdr->value.vchar);
	hdr->value.vchar = NULL;
	hdr->type = 0;
}

static void FreeHeader(HTTPHeader* hdr) {
	Memory_Free((void*)hdr->key);
	EmptyHeader(hdr);
	Memory_Free(hdr);
}

HTTPHeader* HttpRequest_GetHeader(HTTPRequest* req, const char* key) {
	HTTPHeader* hdr = FindHeader(req->header, key);
	if(!hdr) {
		hdr = AllocHeader(key);
		if(req->header) {
			HTTPHeader* ptr = req->header;
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

void HttpRequest_SetHeaderStr(HTTPRequest* req, const char* key, const char* value) {
	HTTPHeader* hdr = HttpRequest_GetHeader(req, key);
	EmptyHeader(hdr);
	hdr->type = HDRT_STR;
	hdr->value.vchar = String_AllocCopy(value);
}

const char* HttpRequest_GetHeaderStr(HTTPRequest* req, const char* key) {
	HTTPHeader* hdr = FindHeader(req->header, key);
	if(hdr && hdr->type == HDRT_STR) return hdr->value.vchar;
	return NULL;
}

void HttpRequest_SetHeaderInt(HTTPRequest* req, const char* key, cs_int32 value) {
	HTTPHeader* hdr = HttpRequest_GetHeader(req, key);
	EmptyHeader(hdr);
	hdr->type = HDRT_INT;
	hdr->value.vint = value;
}

cs_int32 HttpRequest_GetHeaderInt(HTTPRequest* req, const char* key) {
	HTTPHeader* hdr = FindHeader(req->header, key);
	if(hdr && hdr->type == HDRT_INT) return hdr->value.vint;
	return 0;
}

void HttpRequest_SetHeader(HTTPRequest* resp, const char* key, const char* value) {
	char first = *value;
	if(first >= '0' && first <= '9') {
		HttpRequest_SetHeaderInt(resp, key, String_ToInt(value));
		return;
	}
	if(first >= ' ' && first <= '~') {
		HttpRequest_SetHeaderStr(resp, key, value);
	}
}

void HttpRequest_SetHost(HTTPRequest* req, const char* host, cs_uint16 port) {
	char hostfmt[26];
	Socket_SetAddrGuess(&req->addr, host, port);
	if(port != 80)
		String_FormatBuf(hostfmt, 26, "www.%s:%d", host, port);
	else
		String_FormatBuf(hostfmt, 26, "www.%s", host);
	HttpRequest_SetHeaderStr(req, "Host", hostfmt);
}

void HttpRequest_SetPath(HTTPRequest* req, const char* path) {
	if(req->path) Memory_Free((void*)req->path);
	/*
	** TODO: Придумать как запилить здесь кодирование url,
	** по началу можно и не полное, а просто заменять пробелы
	** на плюсы, думаю этого будет достаточно какое-то время.
	*/
	req->path = String_AllocCopy(path);
}

static cs_int32 SendLine(Socket sock, char* line) {
	cs_uint32 len = (cs_uint32)String_Length(line);
	return Socket_Send(sock, line, len);
}

static void SendHeaders(Socket sock, HTTPHeader* hdr, char* line) {
	while(hdr) {
		switch(hdr->type) {
			case HDRT_INT:
				String_FormatBuf(line, 1024, "%s: %d\r\n", hdr->key, hdr->value.vint);
				break;
			case HDRT_STR:
				String_FormatBuf(line, 1024, "%s: %s\r\n", hdr->key, hdr->value.vchar);
				break;
			default:
				break;
		}
		SendLine(sock, line);
		hdr = hdr->next;
	}

	String_Copy(line, 1024, "\r\n");
	SendLine(sock, line);
}

cs_bool HttpRequest_Read(HTTPRequest* req, Socket sock) {
	char line[1024];
	if(!Socket_ReceiveLine(sock, line, 1023)) {
		req->error = HTTP_ERR_INVALID_REQUEST;
		return false;
	}

	if(!String_CaselessCompare2(line, "GET ", 4)) {
		req->error = HTTP_ERR_INVALID_REQUEST;
		return false;
	}

	char* path_start = line + 4;
	char* path_end = (char*)String_LastChar(path_start, ' ');
	if(!path_end) {
		req->error = HTTP_ERR_INVALID_REQUEST;
		return false;
	}
	*path_end = '\0';
	req->path = String_AllocCopy(path_start);

	if(!String_CaselessCompare2(++path_end, "HTTP/1.1", 8)) {
		req->error = HTTP_ERR_INVALID_VERSION;
		return false;
	}

	while(Socket_ReceiveLine(sock, line, 1023)) {
		char* value = (char*)String_FirstChar(line, ':');
		if(value) {
			*value++ = '\0';
			HttpRequest_SetHeader(req, line, ++value);
		}
	}

	return true;
}

cs_bool HttpRequest_Perform(HTTPRequest* req, HTTPResponse* resp) {
	char line[1024];
	if(!Socket_Connect(req->sock, &req->addr)) {
		req->error = HTTP_ERROR_CONNECTION_FAILED;
		return false;
	}
	String_FormatBuf(line, 1024, "GET %s HTTP/1.1\r\n", req->path);
	SendLine(req->sock, line);
	SendHeaders(req->sock, req->header, line);
	if(!HttpResponse_Read(resp, req->sock)) {
		req->error = HTTP_ERR_RESPONSE_READ;
		return false;
	}
	return true;
}

void HttpRequest_Cleanup(HTTPRequest* req) {
	if(req->path) Memory_Free((void*)req->path);
	if(req->header) {
		HTTPHeader* ptr = req->header;

		while(ptr) {
			FreeHeader(ptr);
			ptr = ptr->next;
		}
	}
}

HTTPHeader* HttpResponse_GetHeader(HTTPResponse* resp, const char* key) {
	HTTPHeader* hdr = FindHeader(resp->header, key);
	if(!hdr) {
		hdr = AllocHeader(key);
		if(resp->header) {
			HTTPHeader* ptr = resp->header;
			while(ptr)
				if(ptr->next)
					ptr = ptr->next;
				else
					break;
			ptr->next = hdr;
		} else resp->header = hdr;
	}
	return hdr;
}

void HttpResponse_SetHeaderStr(HTTPResponse* resp, const char* key, const char* value) {
	HTTPHeader* hdr = HttpResponse_GetHeader(resp, key);
	EmptyHeader(hdr);
	hdr->type = HDRT_STR;
	hdr->value.vchar = String_AllocCopy(value);
}

const char* HttpResponse_GetHeaderStr(HTTPResponse* resp, const char* key) {
	HTTPHeader* hdr = FindHeader(resp->header, key);
	if(hdr && hdr->type == HDRT_STR) return hdr->value.vchar;
	return NULL;
}

void HttpResponse_SetHeaderInt(HTTPResponse* resp, const char* key, cs_int32 value) {
	HTTPHeader* hdr = HttpResponse_GetHeader(resp, key);
	EmptyHeader(hdr);
	hdr->type = HDRT_INT;
	hdr->value.vint = value;
}

cs_int32 HttpResponse_GetHeaderInt(HTTPResponse* resp, const char* key) {
	HTTPHeader* hdr = FindHeader(resp->header, key);
	if(hdr && hdr->type == HDRT_INT) return hdr->value.vint;
	return 0;
}

void HttpResponse_SetHeader(HTTPResponse* resp, const char* key, const char* value) {
	char first = *value;
	if(first >= '0' && first <= '9') {
		HttpResponse_SetHeaderInt(resp, key, String_ToInt(value));
		return;
	}
	if(first >= ' ' && first <= '~') {
		HttpResponse_SetHeaderStr(resp, key, value);
	}
}

void HttpResponse_SetBody(HTTPResponse* resp, char* body, cs_int32 size) {
	HttpResponse_SetHeaderInt(resp, "Content-Length", size);
	resp->bodysize = size;
	resp->body = body;
}

cs_bool HttpResponse_SendTo(HTTPResponse* resp, Socket sock) {
	char line[1024];
	const char* phrase = HttpCode_GetReason(resp->code);
	if(!phrase) {
		resp->error = HTTP_ERR_INVALID_CODE;
		return false;
	}
	String_FormatBuf(line, 1024, "HTTP/1.1 %d %s\r\n", resp->code, phrase);
	SendLine(sock, line);
	SendHeaders(sock, resp->header, line);
	if(resp->body)
		Socket_Send(sock, resp->body, resp->bodysize);

	return true;
}

cs_bool HttpResponse_Read(HTTPResponse* resp, Socket sock) {
	char line[1024];
	if(Socket_ReceiveLine(sock, line, 1023)) {
		if(!String_CaselessCompare2(line, "HTTP/1.1 ", 9)) {
			resp->error = HTTP_ERR_INVALID_VERSION;
			return false;
		}
	} else {
		resp->error = HTTP_ERR_INVALID_VERSION;
		return false;
	}

	const char* code_start = String_FirstChar(line, ' ');
	if(!code_start) {
		resp->error = HTTP_ERR_INVALID_REQUEST;
		return false;
	}
	char* code_end = (char*)++code_start;
	while(*code_end >= '0' && *code_end <= '9') ++code_end;
	*code_end = '\0';
	if(code_start == code_end) {
		resp->error = HTTP_ERR_INVALID_REQUEST;
		return false;
	}
	cs_int32 code = String_ToInt(code_start);

	if(code < 100 || code > 599) {
		resp->error = HTTP_ERR_INVALID_CODE;
		return false;
	}

	resp->code = code;
	while(Socket_ReceiveLine(sock, line, 1023)) {
		char* value = (char*)String_FirstChar(line, ':');
		if(value) {
			*value++ = '\0';
			HttpResponse_SetHeader(resp, line, ++value);
		}
	}

	cs_int32 len = HttpResponse_GetHeaderInt(resp, "Content-Length");
	if(len > 0) {
		if(len <= 131072) {
			resp->bodysize = len;
			resp->body = Memory_Alloc(1, len + 1);
			if(Socket_Receive(sock, resp->body, len, 0) != len) {
				resp->error = HTTP_ERR_BODY_RECV_FAIL;
				return false;
			}
			return true;
		} else {
			resp->error = HTTP_ERR_TOO_BIG_BODY;
			return false;
		}
	}

	const char* enc = HttpResponse_GetHeaderStr(resp, "Transfer-Encoding");
	if(enc && String_CaselessCompare(enc, "chunked")) {
		char hex[12];
		while(Socket_ReceiveLine(sock, hex, 12)) {
			if(*hex == '0') break;
			cs_int32 clen = String_HexToInt(hex);
			if(clen == 0) {
				resp->error = HTTP_ERR_BODY_RECV_FAIL;
				return false;
			}
			cs_int32 oldbodysize = resp->bodysize;
			if(!resp->body) {
				resp->body = Memory_Alloc(1, clen);
				if(Socket_Receive(sock, resp->body, clen, 0) != clen) {
					resp->error = HTTP_ERR_BODY_RECV_FAIL;
					return false;
				}
			}
			resp->bodysize += clen;
			if(resp->bodysize > 134217728) {
				resp->error = HTTP_ERR_TOO_BIG_BODY;
				return false;
			}
			if(oldbodysize > 0) {
				resp->body = Memory_Realloc(resp->body, oldbodysize, resp->bodysize);
				if(Socket_Receive(sock, resp->body + oldbodysize, clen, 0) != clen) {
					resp->error = HTTP_ERR_BODY_RECV_FAIL;
					return false;
				}
			}
		}
	}
	return true;
}

void HttpResponse_Cleanup(HTTPResponse* resp) {
	if(resp->header) {
		HTTPHeader* ptr = resp->header;

		while(ptr) {
			FreeHeader(ptr);
			ptr = ptr->next;
		}
	}

	resp->code = 0;
	resp->error = 0;
	resp->bodysize = 0;
	resp->body = NULL;
	resp->header = NULL;
}
