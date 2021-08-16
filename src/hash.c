#include "core.h"
#ifdef WINDOWS
#include "hash.h"

INL static cs_bool HashInit(HASH_CTX *ctx, ALG_ID alg) {
	return CryptAcquireContext(&ctx->prov, NULL, NULL, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT) &&
	CryptCreateHash(ctx->prov, alg, 0, 0, &ctx->hash);
}

INL static cs_bool HashUpdate(HASH_CTX *ctx, const void *data, cs_ulong len) {
	return (cs_bool)CryptHashData(ctx->hash, data, len, 0);
}

INL static cs_bool HashFinal(void *hash, HASH_CTX *ctx) {
	return CryptGetHashParam(ctx->hash, HP_HASHVAL, hash, &ctx->hashLen, 0) &&
	CryptReleaseContext(ctx->prov, 0) && CryptDestroyHash(ctx->hash);
}

cs_bool SHA1_Init(SHA_CTX *ctx) {
	ctx->hashLen = 20;
	return HashInit(ctx, CALG_SHA1);
}

cs_bool SHA1_Update(SHA_CTX *ctx, const void *data, cs_ulong len) {
	return HashUpdate(ctx, data, len);
}

cs_bool SHA1_Final(cs_byte *hash, SHA_CTX *ctx) {
	return HashFinal(hash, ctx);
}

cs_bool MD5_Init(MD5_CTX *ctx) {
	ctx->hashLen = 16;
	return HashInit(ctx, CALG_MD5);
}

cs_bool MD5_Update(MD5_CTX *ctx, const void *data, cs_ulong len) {
	return HashUpdate(ctx, data, len);
}

cs_bool MD5_Final(cs_byte *hash, MD5_CTX *ctx) {
	return HashFinal(hash, ctx);
}
#endif
