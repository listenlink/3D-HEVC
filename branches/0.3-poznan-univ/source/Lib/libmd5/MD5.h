
#pragma once
#include "libmd5.h"

class MD5 {
public:
  /**
   * initialize digest state
   */
  MD5()
  {
    MD5Init(&m_state);
  }

  /**
   * compute digest over @buf of length @len.
   * multiple calls may extend the digest over more data.
   */
  void update(unsigned char *buf, unsigned len)
  {
    MD5Update(&m_state, buf, len);
  }

  /**
   * flush any outstanding MD5 data, write the digest into @digest.
   */
  void finalize(unsigned char digest[16])
  {
    MD5Final(digest, &m_state);
  }

private:
  context_md5_t m_state;
};

/**
 * Produce an ascii(hex) representation of the 128bit @digest.
 *
 * Returns: a statically allocated null-terminated string.  DO NOT FREE.
 */
inline const char*
digestToString(unsigned char digest[16])
{
  const char* hex = "0123456789abcdef";
  static char string[33];
  for (int i = 0; i < 16; i++)
  {
    string[i*2+0] = hex[digest[i] >> 4];
    string[i*2+1] = hex[digest[i] & 0xf];
  }
  return string;
}

