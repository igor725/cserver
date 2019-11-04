#ifndef HASH_H
#define HASH_H

#if defined(WINDOWS)
#include <wincrypt.h>

typedef struct {
	HCRYPTPROV provider;
	HCRYPTHASH hash;
	unsigned long hashLen;
} SHA_CTX, MD5_CTX, HASH_CTX;

bool SHA1_Init(SHA_CTX* ctx);
bool SHA1_Update(SHA_CTX* ctx, const void* data, cs_size len);
bool SHA1_Final(cs_uint8* hash, SHA_CTX* ctx);

bool MD5_Init(MD5_CTX* ctx);
bool MD5_Update(MD5_CTX* ctx, const void* data, cs_size len);
bool MD5_Final(cs_uint8* hash, MD5_CTX* ctx);
#elif defined(POSIX)
#  include <openssl/md5.h>
#  include <openssl/sha.h>
#endif

#endif
