#include "core.h"
#include "hash.h"
#include "platform.h"
#include <zlib.h>

// TODO: Own CRC32 implementation
cs_int32 CRC32_Gen(const cs_byte *data, cs_uint32 len) {
	return crc32(0, data, len);
}

#if defined(WINDOWS)
HCRYPTPROV hCryptProvider = 0;

struct _CryptLib {
	void *lib;
	BOOL(*AcquireContext)(HCRYPTPROV *, cs_str, cs_str, cs_ulong, cs_ulong);
	BOOL(*ReleaseContext)(HCRYPTPROV, cs_ulong);
	BOOL(*CreateHash)(HCRYPTPROV, ALG_ID, HCRYPTKEY, cs_ulong, HCRYPTHASH *);
	BOOL(*HashData)(HCRYPTHASH, const cs_byte *, cs_ulong, cs_ulong);
	BOOL(*GetHashParam)(HCRYPTHASH, cs_ulong, cs_byte *, cs_ulong *, cs_ulong);
	BOOL(*DestroyHash)(HCRYPTHASH);
} Crypt;

INL static cs_bool InitBackend(void) {
	if(!Crypt.lib) {
		if(!(DLib_Load("advapi32.dll", &Crypt.lib) &&
			DLib_GetSym(Crypt.lib, "CryptAcquireContextA", &Crypt.AcquireContext) &&
			DLib_GetSym(Crypt.lib, "CryptReleaseContext", &Crypt.ReleaseContext) &&
			DLib_GetSym(Crypt.lib, "CryptCreateHash", &Crypt.CreateHash) &&
			DLib_GetSym(Crypt.lib, "CryptHashData", &Crypt.HashData) &&
			DLib_GetSym(Crypt.lib, "CryptGetHashParam", &Crypt.GetHashParam) &&
			DLib_GetSym(Crypt.lib, "CryptDestroyHash", &Crypt.DestroyHash)
		)) return false;
	} else if(hCryptProvider) return true;
	return (cs_bool)Crypt.AcquireContext(&hCryptProvider, NULL, MS_DEF_PROV, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT);
}

void Hash_Uninit(void) {
	if(!Crypt.lib) return;
	if(hCryptProvider) Crypt.ReleaseContext(hCryptProvider, 0);
	Crypt.AcquireContext = NULL;
	Crypt.ReleaseContext = NULL;
	Crypt.CreateHash = NULL;
	Crypt.HashData = NULL;
	Crypt.GetHashParam = NULL;
	Crypt.DestroyHash = NULL;
	DLib_Unload(Crypt.lib);
	Crypt.lib = NULL;
}

INL static cs_bool InitAlg(HASH_CTX *ctx, ALG_ID alg) {
	if(!Crypt.lib && !InitBackend()) return false;
	return (cs_bool)Crypt.CreateHash(hCryptProvider, alg, 0, 0, &ctx->hash);
}

INL static cs_bool UpdateHash(HASH_CTX *ctx, const void *data, cs_ulong len) {
	if(!Crypt.lib) return false;
	return (cs_bool)Crypt.HashData(ctx->hash, data, len, 0);
}

INL static cs_bool FinalHash(void *hash, HASH_CTX *ctx) {
	if(!Crypt.lib) return false;
	return Crypt.GetHashParam(ctx->hash, HP_HASHVAL, hash, &ctx->hashLen, 0) &&
	Crypt.DestroyHash(ctx->hash);
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
struct _CryptLib {
	void *lib;

	cs_bool(*SHA1Init)(SHA_CTX *);
	cs_bool(*SHA1Update)(SHA_CTX *, const void *data, cs_ulong len);
	cs_bool(*SHA1Final)(cs_byte *, SHA_CTX *);

	cs_bool(*MD5Init)(MD5_CTX *);
	cs_bool(*MD5Update)(MD5_CTX *, const void *data, cs_ulong len);
	cs_bool(*MD5Final)(cs_byte *, MD5_CTX *);
} Crypt;

cs_bool Hash_Init(void) {
	if(Crypt.lib)
		return true;
	else {
		if(!((DLib_Load("libcrypto.so", &Crypt.lib) ||
			DLib_Load("libcrypto.so.1.1", &Crypt.lib)) &&
			DLib_GetSym(Crypt.lib, "SHA1_Init", &Crypt.SHA1Init) &&
			DLib_GetSym(Crypt.lib, "SHA1_Update", &Crypt.SHA1Update) &&
			DLib_GetSym(Crypt.lib, "SHA1_Final", &Crypt.SHA1Final) &&
			DLib_GetSym(Crypt.lib, "MD5_Init", &Crypt.MD5Init) &&
			DLib_GetSym(Crypt.lib, "MD5_Update", &Crypt.MD5Update) &&
			DLib_GetSym(Crypt.lib, "MD5_Final", &Crypt.MD5Final)
		)) return false;
	}
	return true;
}

void Hash_Uninit(void) {
	if(!Crypt.lib) return;
	Crypt.SHA1Init = NULL;
	Crypt.SHA1Update = NULL;
	Crypt.SHA1Final = NULL;
	Crypt.MD5Init = NULL;
	Crypt.MD5Update = NULL;
	Crypt.MD5Final = NULL;
	DLib_Unload(Crypt.lib);
	Crypt.lib = NULL;
}

cs_bool SHA1_Init(SHA_CTX *ctx) {
	if(!Crypt.SHA1Init && !Hash_Init()) return false;
	return Crypt.SHA1Init(ctx);
}

cs_bool SHA1_Update(SHA_CTX *ctx, const void *data, cs_ulong len) {
	if(!Crypt.SHA1Update) return false;
	return Crypt.SHA1Update(ctx, data, len);
}

cs_bool SHA1_Final(cs_byte *hash, SHA_CTX *ctx) {
	if(!Crypt.SHA1Final) return false;
	return Crypt.SHA1Final(hash, ctx);
}

cs_bool MD5_Init(MD5_CTX *ctx) {
	if(!Crypt.MD5Init && !Hash_Init()) return false;
	return Crypt.MD5Init(ctx);
}

cs_bool MD5_Update(MD5_CTX *ctx, const void *data, cs_ulong len) {
	if(!Crypt.MD5Update) return false;
	return Crypt.MD5Update(ctx, data, len);
}

cs_bool MD5_Final(cs_byte *hash, MD5_CTX *ctx) {
	if(!Crypt.MD5Final) return false;
	return Crypt.MD5Final(hash, ctx);
}
#endif
