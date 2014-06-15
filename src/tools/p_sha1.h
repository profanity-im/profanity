/* public api for steve reid's public domain SHA-1 implementation */
/* this file is in the public domain */

/** @file
 *  SHA-1 hash API.
 */

#ifndef __P_SHA1_H
#define __P_SHA1_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint32_t state[5];
    uint32_t count[2];
    uint8_t  buffer[64];
} P_SHA1_CTX;

#define P_SHA1_DIGEST_SIZE 20

void P_SHA1_Init(P_SHA1_CTX* context);
void P_SHA1_Update(P_SHA1_CTX* context, const uint8_t* data, const size_t len);
void P_SHA1_Final(P_SHA1_CTX* context, uint8_t digest[P_SHA1_DIGEST_SIZE]);

#ifdef __cplusplus
}
#endif

#endif /* __P_SHA1_H */
