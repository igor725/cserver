/* ================ sha1.h ================ */
/*
SHA-1 in C
By Steve Reid <steve@edmweb.com>
100% Public Domain
*/
#ifndef _SHA1
#define _SHA1
#define HASH_SIZE 20
typedef struct {
    uint32_t state[5];
    uint32_t count[2];
    uint8_t buffer[64];
} SHA1_CTX;

void SHA1Transform(uint32_t state[5], const uint8_t buffer[64]);
void SHA1Init(SHA1_CTX* context);
void SHA1Update(SHA1_CTX* context, const uint8_t* data, uint32_t len);
void SHA1Final(uint8_t digest[HASH_SIZE], SHA1_CTX* context);
#endif
