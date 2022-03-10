#include "core.h"
#include "platform.h"
#include "cserror.h"
#include "str.h"

void *Memory_Alloc(cs_size num, cs_size size) {
	void *ptr = Memory_TryAlloc(num, size);
	if(!ptr) {
		Error_PrintSys(true);
	}
	return ptr;
}

void *Memory_Realloc(void *oldptr, cs_size new) {
	void *newptr = Memory_TryRealloc(oldptr, new);
	if(!newptr) {
		Error_PrintSys(true);
	}
	return newptr;
}

void Memory_Copy(void *dst, const void *src, cs_size count) {
	cs_byte *u8dst = (cs_byte *)dst,
	*u8src = (cs_byte *)src;
	while(count--) *u8dst++ = *u8src++;
}

void Memory_Fill(void *dst, cs_size count, cs_byte val) {
	cs_byte *u8dst = (cs_byte *)dst;
	while(count--) *u8dst++ = val;
}

cs_file File_Open(cs_str path, cs_str mode) {
	return fopen(path, mode);
}

cs_size File_Read(void *ptr, cs_size size, cs_size count, cs_file fp) {
	return fread(ptr, size, count, fp);
}

cs_int32 File_ReadLine(cs_file fp, cs_char *line, cs_int32 len) {
	cs_char *sym;

	for(sym = line; (sym - line) < len; sym++) {
		cs_int32 ch = File_GetChar(fp);
		if(ch == EOF || ch == '\n') {
			*sym = '\0';
			return (cs_int32)(sym - line);
		} else if(ch != '\r')
			*sym = (cs_char)ch;
	}

	*sym = '\0';
	return -1;
}

cs_size File_Write(const void *ptr, cs_size size, cs_size count, cs_file fp) {
	return fwrite(ptr, size, count, fp);
}

cs_int32 File_GetChar(cs_file fp) {
	return fgetc(fp);
}

cs_int32 File_Error(cs_file fp) {
	return ferror(fp);
}

cs_int32 File_WriteFormat(cs_file fp, cs_str fmt, ...) {
	va_list args;
	va_start(args, fmt);
	cs_int32 len = vfprintf(fp, fmt, args);
	va_end(args);

	return len;
}

cs_bool File_Flush(cs_file fp) {
	return fflush(fp) == 0;
}

cs_int32 File_Seek(cs_file fp, long offset, cs_int32 origin) {
	return fseek(fp, offset, origin);
}

cs_bool File_Close(cs_file fp) {
	return fclose(fp) != 0;
}

cs_int32 Socket_SetAddr(struct sockaddr_in *ssa, cs_str ip, cs_uint16 port) {
	ssa->sin_family = AF_INET;
	ssa->sin_port = htons(port);
	return inet_pton(AF_INET, ip, &ssa->sin_addr.s_addr);
}

cs_bool Socket_SetAddrGuess(struct sockaddr_in *ssa, cs_str host, cs_uint16 port) {
	cs_int32 ret;
	if((ret = Socket_SetAddr(ssa, host, port)) == 0) {
		struct addrinfo *addr;
		struct addrinfo hints = {0};
		hints.ai_family = AF_INET;
		hints.ai_socktype = 0;
		hints.ai_protocol = 0;

		cs_char strport[6];
		String_FormatBuf(strport, 6, "%d", port);
		if((ret = getaddrinfo(host, strport, &hints, &addr)) == 0) {
			*ssa = *(struct sockaddr_in *)addr->ai_addr;
			freeaddrinfo(addr);
			return true;
		}
	}
	return ret == 1;
}

Socket Socket_New(void) {
	return socket(AF_INET, SOCK_STREAM, 0);
}

cs_bool Socket_Bind(Socket sock, struct sockaddr_in *addr) {
#if defined(CORE_USE_UNIX)
	if(setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &(cs_int32){1}, 4) < 0) {
		return false;
	}
#elif defined(CORE_USE_WINDOWS)
	if(setsockopt(sock, SOL_SOCKET, SO_DONTLINGER, (void*)&(cs_int32){0}, 4) < 0) {
		return false;
	}
#endif

	if(bind(sock, (const struct sockaddr *)addr, sizeof(struct sockaddr_in)) < 0) {
		return false;
	}

	if(listen(sock, SOMAXCONN) < 0) {
		return false;
	}

	return true;
}

cs_bool Socket_Connect(Socket sock, struct sockaddr_in *addr) {
	socklen_t len = sizeof(struct sockaddr_in);
	return connect(sock, (struct sockaddr *)addr, len) == 0;
}

Socket Socket_Accept(Socket sock, struct sockaddr_in *addr) {
	socklen_t len = sizeof(struct sockaddr_in);
	return accept(sock, (struct sockaddr *)addr, &len);
}

cs_int32 Socket_Receive(Socket sock, cs_char *buf, cs_int32 len, cs_int32 flags) {
	return recv(sock, buf, len, MSG_NOSIGNAL | MSG_DONTWAIT | flags);
}

cs_bool Socket_ReceiveLine(Socket sock, cs_char *line, cs_int32 blen, cs_int32 *recv) {
	cs_int32 start_len = blen;
	cs_char sym = 0;

	if(*recv > 0) {
		blen -= *recv;
		if(blen < 1) {
			*recv = -1;
			return false;
		} 
		line += *recv;
	}

	while(blen > 1) {
		if(Socket_Receive(sock, &sym, 1, 0) == 1) {
			if(sym == '\n') {
				*recv = start_len - blen;
				*line = '\0';
				return true;
			} else if(sym != '\r') {
				*line++ = sym;
				blen -= 1;
			}
		} else {
			if(Socket_IsFatal()) {
				*recv = -1;
				return false;
			}

			break;
		}
	}

	*recv = start_len - blen;
	return false;
}

/* Это пиздец, товарищи. Я понятия не имею, почему
 * иногда при грязном закрытии сокета, send продолжает
 * бесконечно возвращать EWOULDBLOCK. Пришлось нашаманить
 * ту хуйню, которую вы сейчас видите. До лучших времён
 * это говно останется здесь. По хорошему, при ошибке в
 * send, сетевой поток должен продолжать исполнять свой
 * цикл в нормальном порядке и после полного тика с чтением
 * данных из этого сокета и прочими приколами, если оказалось,
 * что клиент жив, то повторить отправку данных с места
 * ошибки. Если же мёртв, то послать его куда подальше и
 * ничего ему не отправлять. Поскольку такой подход требует
 * ещё больше переписать код отправки пакетов я решил
 * накостылять этой процедуре, чтобы оно хоть как-то работало
 * сейчас. В общем, не обессудьте, когда-нибудь я это всё
 * перепишу и оно начнёт работать так, как задумано душевно
 * больными людьми, создавшими сие чудо, протокол TCP/IP. Но
 * сегодня у меня нет ни времени, ни желания этим заниматься,
 * так как на первый взгляд это всё требует общего буфера для
 * сокета, который сможет в себе хранить все неотправленные
 * данные, так как при той же отправке мира они будут копиться с
 * ебанутой скоростью. Также встаёт вопрос с вебсокет соедиениями,
 * они тоже должны быть способны обрабатывать кейсы, когда send
 * начинает страдать хуйнёй.
 */
cs_bool Socket_Send(Socket sock, const cs_char *buf, cs_int32 len) {
	cs_uint32 offset = 0;
	cs_uint32 attempts = 100;

	while(attempts > 0) {
		cs_int32 sent = send(sock, buf + offset, len, MSG_NOSIGNAL);
		if(sent > 0) {
			len -= sent;
			offset += sent;
			if(len == 0) return true;
		} else if(sent < 0 && Socket_IsFatal()) {
			return false;
		} else if(sent == 0)
			return false;

		Thread_Sleep(8);
		attempts--;
	}

	return false;
}

cs_bool Socket_Shutdown(Socket sock, cs_int32 how) {
	return shutdown(sock, how) == 0;
}

cs_bool Directory_Ensure(cs_str path) {
	if(Directory_Exists(path)) return true;
	return Directory_Create(path);
}

cs_bool DLib_LoadAll(cs_str lib[], cs_str symlist[], void **ctx) {
	ctx[0] = NULL;
	for(cs_uint32 i = 0; lib[i]; i++)
		if(DLib_Load(lib[i], ctx)) break;
	if(ctx[0] == NULL) return false;

	for(cs_uint32 i = 0; symlist[i]; i++)
		if(!DLib_GetSym(ctx[0], symlist[i], &ctx[i + 1])) {
			DLib_Unload(ctx[0]);
			ctx[0] = NULL;
			return false;
		}

	return true;
}

cs_error Thread_GetError(void) {
	return errno;
}
