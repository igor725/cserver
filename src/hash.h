#ifndef HASH_H
#define HASH_H
#include "core.h"

#ifndef CORE_BUILD_PLUGIN
	void Hash_Uninit(void);
#endif

#if defined(HASH_USE_WINCRYPT_BACKEND)
// Start of <wincrypt.h>
#	define HP_HASHVAL 0x0002
#	define MS_DEF_PROV "Microsoft Base Cryptographic Provider v1.0"
#	define CRYPT_VERIFYCONTEXT 0xF0000000
#	define PROV_RSA_FULL 1
#	define ALG_SID_MD5 3
#	define ALG_SID_SHA1 4
#	define ALG_CLASS_HASH (4 << 13)
#	define CALG_MD5 (ALG_CLASS_HASH | ALG_SID_MD5)
#	define CALG_SHA1 (ALG_CLASS_HASH | ALG_SID_SHA1)
	typedef cs_uintptr HCRYPTHASH, HCRYPTPROV, HCRYPTKEY;
	typedef cs_uint32 ALG_ID;
	typedef cs_int32 BOOL;
// End of <wincrypt.h>

	typedef struct _WinHash {
		HCRYPTHASH hash;
		cs_ulong hashLen;
	} SHA_CTX, MD5_CTX, HASH_CTX;
#elif defined(HASH_USE_CRYPTO_BACKEND)
// Start of <openssl/md5.h>
#	define MD5_LONG unsigned int
#	define MD5_CBLOCK      64
#	define MD5_LBLOCK      (MD5_CBLOCK/4)

	typedef struct MD5state_st {
		MD5_LONG A, B, C, D;
		MD5_LONG Nl, Nh;
		MD5_LONG data[MD5_LBLOCK];
		unsigned int num;
	} MD5_CTX;
// End of <openssl/md5.h>

// Start of <openssl/sha.h>
#	define SHA_LONG unsigned int

#	define SHA_LBLOCK      16
#	define SHA_CBLOCK      (SHA_LBLOCK*4)
#	define SHA_LAST_BLOCK  (SHA_CBLOCK-8)

	typedef struct SHAstate_st {
		SHA_LONG h0, h1, h2, h3, h4;
		SHA_LONG Nl, Nh;
		SHA_LONG data[SHA_LBLOCK];
		unsigned int num;
	} SHA_CTX;
// End of <openssl/sha.h>
#else
#	error No сryptographic backend selected
#endif

API cs_bool SHA1_Start(SHA_CTX *ctx);
API cs_bool SHA1_PushData(SHA_CTX *ctx, const void *data, cs_ulong len);
API cs_bool SHA1_End(cs_byte *hash, SHA_CTX *ctx);

API cs_bool MD5_Start(MD5_CTX *ctx);
API cs_bool MD5_PushData(MD5_CTX *ctx, const void *data, cs_ulong len);
API cs_bool MD5_End(cs_byte *hash, MD5_CTX *ctx);
#endif // HASH_H
