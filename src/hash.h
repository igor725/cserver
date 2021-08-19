#ifndef HASH_H
#define HASH_H
#include "core.h"

cs_bool Hash_Init(void);
void Hash_Uninit(void);

#if defined(WINDOWS)
#include <windows.h>
#include <wincrypt.h>

typedef struct _WinHash {
	HCRYPTHASH hash;
	cs_ulong hashLen;
} SHA_CTX, MD5_CTX, HASH_CTX;

cs_bool SHA1_Init(SHA_CTX *ctx);
cs_bool SHA1_Update(SHA_CTX *ctx, const void *data, cs_ulong len);
cs_bool SHA1_Final(cs_byte *hash, SHA_CTX *ctx);

cs_bool MD5_Init(MD5_CTX *ctx);
cs_bool MD5_Update(MD5_CTX *ctx, const void *data, cs_ulong len);
cs_bool MD5_Final(cs_byte *hash, MD5_CTX *ctx);
#elif defined(UNIX)
#include <openssl/md5.h>
#include <openssl/sha.h>
#endif
#endif // HASH_H
