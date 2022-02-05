#include "core.h"
#include "hash.h"
#include "platform.h"

#if defined(HASH_USE_WINCRYPT_BACKEND)
HCRYPTPROV hCryptProvider = 0;

static cs_str csymlist[] = {
	"CryptAcquireContextA", "CryptReleaseContext",
	"CryptCreateHash", "CryptHashData",
	"CryptGetHashParam", "CryptDestroyHash",
	NULL
};

static struct _CryptLib {
	void *lib;

	BOOL(*AcquireContext)(HCRYPTPROV *, cs_str, cs_str, cs_ulong, cs_ulong);
	BOOL(*ReleaseContext)(HCRYPTPROV, cs_ulong);
	BOOL(*CreateHash)(HCRYPTPROV, ALG_ID, HCRYPTKEY, cs_ulong, HCRYPTHASH *);
	BOOL(*HashData)(HCRYPTHASH, const cs_byte *, cs_ulong, cs_ulong);
	BOOL(*GetHashParam)(HCRYPTHASH, cs_ulong, cs_byte *, cs_ulong *, cs_ulong);
	BOOL(*DestroyHash)(HCRYPTHASH);
} Crypt;

INL static cs_bool InitBackend(void) {
	if(!Crypt.lib && !DLib_LoadAll(DLib_List("advapi32.dll"), csymlist, (void **)&Crypt))
		return false;

	if(!hCryptProvider)
		return (cs_bool)Crypt.AcquireContext(
			&hCryptProvider, NULL,
			MS_DEF_PROV, PROV_RSA_FULL,
			CRYPT_VERIFYCONTEXT
		);
	
	return true;
}

void Hash_Uninit(void) {
	if(hCryptProvider) Crypt.ReleaseContext(hCryptProvider, 0);
	DLib_Unload(Crypt.lib);
	Memory_Zero(&Crypt, sizeof(Crypt));
}

static cs_bool InitAlg(HASH_CTX *ctx, ALG_ID alg) {
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
#elif defined(HASH_USE_CRYPTO_BACKEND)
static cs_str csymlist[] = {
	"SHA1_Init", "SHA1_Update", "SHA1_Final",
	"MD5_Init", "MD5_Update", "MD5_Final",
	NULL
};

static struct _CryptLib {
	void *lib;

	cs_bool(*SHA1Init)(SHA_CTX *);
	cs_bool(*SHA1Update)(SHA_CTX *, const void *data, cs_ulong len);
	cs_bool(*SHA1Final)(cs_byte *, SHA_CTX *);

	cs_bool(*MD5Init)(MD5_CTX *);
	cs_bool(*MD5Update)(MD5_CTX *, const void *data, cs_ulong len);
	cs_bool(*MD5Final)(cs_byte *, MD5_CTX *);
} Crypt;

static cs_str cryptodll[] = {
#if defined(WINDOWS)
	"crypto.dll", "libeay32.dll",
#elif defined(UNIX)
	"libcrypto.so", "libcrypto.so.1.1",
#else
#error This file wants to be hacked
#endif
	NULL
};

cs_bool Hash_Init(void) {
	return Crypt.lib != NULL || DLib_LoadAll(cryptodll, csymlist, (void **)&Crypt);
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
	if(!Crypt.lib && !Hash_Init()) return false;
	return Crypt.SHA1Init(ctx);
}

cs_bool SHA1_Update(SHA_CTX *ctx, const void *data, cs_ulong len) {
	if(!Crypt.lib) return false;
	return Crypt.SHA1Update(ctx, data, len);
}

cs_bool SHA1_Final(cs_byte *hash, SHA_CTX *ctx) {
	if(!Crypt.lib) return false;
	return Crypt.SHA1Final(hash, ctx);
}

cs_bool MD5_Init(MD5_CTX *ctx) {
	if(!Crypt.lib && !Hash_Init()) return false;
	return Crypt.MD5Init(ctx);
}

cs_bool MD5_Update(MD5_CTX *ctx, const void *data, cs_ulong len) {
	if(!Crypt.lib) return false;
	return Crypt.MD5Update(ctx, data, len);
}

cs_bool MD5_Final(cs_byte *hash, MD5_CTX *ctx) {
	if(!Crypt.lib) return false;
	return Crypt.MD5Final(hash, ctx);
}
#else
#error This file wants to be hacked
#endif
