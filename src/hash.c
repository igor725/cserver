/*
** Этот файл компилируется ТОЛЬКО из под винды,
** линуксовый билд-скрипт его не трогает.
** Окна предоставляют своё криптографическое
** API, так что разумнее будет юзать его,
** нежели пихать огромную libcrypto в
** сервер и радоваться. Данный модуль повторяет
** функции хеширования md5 и sha1 из libcrypto.
*/
#include "core.h"
#ifdef WINDOWS
#include "hash.h"

static cs_bool HashInit(HASH_CTX *ctx, cs_int32 type) {
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

static cs_bool HashUpdate(HASH_CTX *ctx, const void *data, cs_size len) {
	if(!CryptHashData(ctx->hash, data, (cs_uint32)len, 0)) {
		CryptReleaseContext(ctx->provider, 0);
		CryptDestroyHash(ctx->hash);
		return false;
	}
	return true;
}

static cs_bool HashFinal(void *hash, HASH_CTX *ctx) {
	CryptGetHashParam(ctx->hash, HP_HASHVAL, hash, &ctx->hashLen, 0);
	CryptReleaseContext(ctx->provider, 0);
	CryptDestroyHash(ctx->hash);
	return false;
}

cs_bool SHA1_Init(SHA_CTX *ctx) {
	ctx->hashLen = 20;
	return HashInit(ctx, CALG_SHA1);
}

cs_bool SHA1_Update(SHA_CTX *ctx, const void *data, cs_size len) {
	return HashUpdate(ctx, data, len);
}

cs_bool SHA1_Final(cs_byte *hash, SHA_CTX *ctx) {
	return HashFinal(hash, ctx);
}

cs_bool MD5_Init(MD5_CTX *ctx) {
	ctx->hashLen = 16;
	return HashInit(ctx, CALG_MD5);
}

cs_bool MD5_Update(MD5_CTX *ctx, const void *data, cs_size len) {
	return HashUpdate(ctx, data, len);
}

cs_bool MD5_Final(cs_byte *hash, MD5_CTX *ctx) {
	return HashFinal(hash, ctx);
}
#endif
