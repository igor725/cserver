#include "core.h"
#include "hash.h"
#include <zlib.h>

// TODO: Own CRC32 implementation
cs_int32 CRC32_Gen(cs_byte *data, cs_uint32 len) {
	return crc32(0, data, len);
}

#if defined(WINDOWS)
HCRYPTPROV hCryptProvider = 0;

INL static cs_bool InitAlg(HASH_CTX *ctx, ALG_ID alg) {
	return (cs_bool)CryptCreateHash(hCryptProvider, alg, 0, 0, &ctx->hash);
}

INL static cs_bool UpdateHash(HASH_CTX *ctx, const void *data, cs_ulong len) {
	return (cs_bool)CryptHashData(ctx->hash, data, len, 0);
}

INL static cs_bool FinalHash(void *hash, HASH_CTX *ctx) {
	return CryptGetHashParam(ctx->hash, HP_HASHVAL, hash, &ctx->hashLen, 0) &&
	CryptDestroyHash(ctx->hash);
}

cs_bool Hash_Init(void) {
	return (cs_bool)CryptAcquireContextA(&hCryptProvider, NULL, MS_DEF_PROV, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT);
}

void Hash_Uninit(void) {
	if(hCryptProvider) CryptReleaseContext(hCryptProvider, 0);
}

cs_bool SHA1_Init(SHA_CTX *ctx) {
	ctx->hashLen = 20;
	return InitAlg(ctx, CALG_SHA1);
}

cs_bool SHA1_Update(SHA_CTX *ctx, const void *data, cs_ulong len) {
	return UpdateHash(ctx, data, len);
}

cs_bool SHA1_Final(cs_byte *hash, SHA_CTX *ctx) {
	return FinalHash(hash, ctx);
}

cs_bool MD5_Init(MD5_CTX *ctx) {
	ctx->hashLen = 16;
	return InitAlg(ctx, CALG_MD5);
}

cs_bool MD5_Update(MD5_CTX *ctx, const void *data, cs_ulong len) {
	return UpdateHash(ctx, data, len);
}

cs_bool MD5_Final(cs_byte *hash, MD5_CTX *ctx) {
	return FinalHash(hash, ctx);
}
#elif defined(UNIX)
cs_bool Hash_Init(void) {return true;}
void Hash_Uninit(void) {}
#endif
