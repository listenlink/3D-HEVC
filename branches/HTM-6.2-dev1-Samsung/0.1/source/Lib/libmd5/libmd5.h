
#pragma once
#include <stdint.h>

typedef struct _context_md5_t {
  uint32_t buf[4];
  uint32_t bits[2];
  unsigned char in[64];
} context_md5_t;

#ifdef __cplusplus
extern "C" {
#endif
void MD5Init(context_md5_t *ctx);
void MD5Update(context_md5_t *ctx, unsigned char *buf, unsigned len);
void MD5Final(unsigned char digest[16], context_md5_t *ctx);
#ifdef __cplusplus
}
#endif
