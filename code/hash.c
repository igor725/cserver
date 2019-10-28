/*
	Этот файл компилируется ТОЛЬКО из под винды,
	линуксовый билд-скрипт его не трогает.
	Окна предоставляют своё криптографическое
	API, так что разумнее будет юзать его,
	нежели пихать огромную libcrypto в
	сервер и радоваться. Данный модуль повторяет
	функции хеширования md5 и sha1 из libcrypto.
*/
#include "core.h"
#ifdef WINDOWS
#include "hash.h"

static bool HashInit(HASH_CTX* ctx, int type) {
	if(!CryptAcquireContext(
		&ctx->provider,
		NULL,
		NULL,
		PROV_RSA_FULL,
		CRYPT_VERIFYCONTEXT
	)) return false;

	if(!CryptCreateHash(ctx->provider, type, 0, 0, &ctx->hash)) {
		CryptReleaseContext(ctx->provider, 0);
		return false;
	}

	return true;
}

static bool HashUpdate(HASH_CTX* ctx, const void* data, size_t len) {
	if(!CryptHashData(ctx->hash, data, (uint32_t)len, 0)) {
		CryptReleaseContext(ctx->provider, 0);
		CryptDestroyHash(ctx->hash);
		return false;
	}
	return true;
}

static bool HashFinal(void* hash, HASH_CTX* ctx) {
	CryptGetHashParam(ctx->hash, HP_HASHVAL, hash, &ctx->hashLen, 0);
	CryptReleaseContext(ctx->provider, 0);
	CryptDestroyHash(ctx->hash);
	return false;
}

bool SHA1_Init(SHA_CTX* ctx) {
	ctx->hashLen = 20;
	return HashInit(ctx, CALG_SHA1);
}

bool SHA1_Update(SHA_CTX* ctx, const void* data, size_t len) {
	return HashUpdate(ctx, data, len);
}

bool SHA1_Final(uint8_t* hash, SHA_CTX* ctx) {
	return HashFinal(hash, ctx);
}

bool MD5_Init(MD5_CTX* ctx) {
	ctx->hashLen = 16;
	return HashInit(ctx, CALG_MD5);
}

bool MD5_Update(MD5_CTX* ctx, const void* data, size_t len) {
	return HashUpdate(ctx, data, len);
}

bool MD5_Final(uint8_t* hash, MD5_CTX* ctx) {
	return HashFinal(hash, ctx);
}
#endif
