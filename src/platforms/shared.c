#include "core.h"
#include "platform.h"
#include "cserror.h"
#include "str.h"

void *Memory_Alloc(cs_size num, cs_size size) {
	void *ptr = Memory_TryAlloc(num, size);
	if(!ptr) Error_PrintSys(true);
	return ptr;
}

void *Memory_Realloc(void *oldptr, cs_size new) {
	void *newptr = Memory_TryRealloc(oldptr, new);
	if(!newptr) Error_PrintSys(true);
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

cs_bool Memory_Compare(const cs_byte *src1, const cs_byte *src2, cs_size len) {
	if(!src1 || !src2) return false;

	while(len--)
		if(*src1++ != *src2++) return false;

	return true;
}

cs_file File_Open(cs_str path, cs_str mode) {
	return fopen(path, mode);
}

cs_size File_Read(void *ptr, cs_size size, cs_size count, cs_file fp) {
	return fread(ptr, size, count, fp);
}

cs_int32 File_ReadLine(cs_file fp, cs_char *line, cs_size len) {
	cs_char *sym;

	for(sym = line; (cs_size)(sym - line) < len; sym++) {
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

cs_bool File_IsEnd(cs_file fp) {
	return feof(fp) > 0;
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

cs_long File_Seek(cs_file fp, cs_long offset, cs_int32 origin) {
	if(fseek(fp, offset, origin) != 0) return -1;
	return ftell(fp);
}

cs_bool File_Close(cs_file fp) {
	return fclose(fp) != 0;
}

static const struct _subnet {
	cs_ulong net;
	cs_ulong mask;
} localnets[] = {
	{0x0000007f, 0x000000FF},
	{0x0000000A, 0x000000FF},
	{0x000010AC, 0x00000FFF},
	{0x0000A8C0, 0x0000FFFF},

	{0x00000000, 0x00000000}
};

cs_bool Socket_IsLocal(cs_ulong addr) {
	for(const struct _subnet *s = localnets; s->mask; s++) {
		if((addr & s->mask) == s->net)
			return true;
	}

	return false;
}

cs_bool Socket_SetAddrGuess(struct sockaddr_in *ssa, cs_str host, cs_uint16 port) {
	cs_int32 ret;

	if((ret = Socket_SetAddr(ssa, host, port)) == 0) {
		struct addrinfo *addr;
		struct addrinfo hints = {
			.ai_family = AF_INET,
			.ai_socktype = 0,
			.ai_protocol = 0
		};

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
	return connect(sock, (struct sockaddr *)addr, sizeof(struct sockaddr_in)) == 0;
}

Socket Socket_Accept(Socket sock, struct sockaddr_in *addr) {
	return accept(sock, (struct sockaddr *)addr, &(socklen_t){sizeof(struct sockaddr_in)});
}

cs_int32 Socket_Receive(Socket sock, cs_char *buf, cs_int32 len, cs_int32 flags) {
	return recv(sock, buf, len, MSG_NOSIGNAL | MSG_DONTWAIT | flags);
}

cs_int32 Socket_Send(Socket sock, const cs_char *buf, cs_int32 len) {
	return (cs_int32)send(sock, buf, len, MSG_NOSIGNAL);
}

cs_bool Socket_Shutdown(Socket sock, cs_int32 how) {
	return shutdown(sock, how) == 0;
}

cs_bool Directory_Ensure(cs_str path) {
	return Directory_Exists(path) || Directory_Create(path);
}

cs_bool DLib_LoadAll(cs_str const lib[], cs_str const symlist[], void **ctx) {
	ctx[0] = NULL;
	while(*lib && !DLib_Load(*lib++, ctx));

	for(cs_int32 i = 0; ctx[0] && symlist[i]; i++)
		if(!DLib_GetSym(ctx[0], symlist[i], &ctx[i + 1])) {
			DLib_Unload(ctx[0]);
			ctx[0] = NULL;
		}

	return ctx[0] != NULL;
}

cs_error Thread_GetError(void) {
	return errno;
}
