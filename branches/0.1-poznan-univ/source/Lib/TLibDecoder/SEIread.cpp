

#include "../TLibCommon/TComBitStream.h"
#include "../TLibCommon/SEI.h"
#include "SEIread.h"

static void parseSEIuserDataUnregistered(TComBitstream& bs, SEIuserDataUnregistered &sei, unsigned payloadSize);
static void parseSEIpictureDigest(TComBitstream& bs, SEIpictureDigest& sei, unsigned payloadSize);

/**
 * unmarshal a single SEI message from bitstream @bs
 */
void parseSEImessage(TComBitstream& bs, SEImessages& seis)
{
  unsigned payloadType = 0;
  for (unsigned char byte = 0xff; 0xff == byte; )
  {
    payloadType += byte = bs.read(8);
  }

  unsigned payloadSize = 0;
  for (unsigned char byte = 0xff; 0xff == byte; )
  {
    payloadSize += byte = bs.read(8);
  }

  switch (payloadType)
  {
  case SEI::USER_DATA_UNREGISTERED:
    seis.user_data_unregistered = new SEIuserDataUnregistered;
    parseSEIuserDataUnregistered(bs, *seis.user_data_unregistered, payloadSize);
    break;
  case SEI::PICTURE_DIGEST:
    seis.picture_digest = new SEIpictureDigest;
    parseSEIpictureDigest(bs, *seis.picture_digest, payloadSize);
    break;
  default:
    assert(!"Unhandled SEI message");
  }
}

/**
 * parse bitstream @bs and unpack a user_data_unregistered SEI message
 * of @payloasSize bytes into @sei.
 */
static void parseSEIuserDataUnregistered(TComBitstream& bs, SEIuserDataUnregistered &sei, unsigned payloadSize)
{
  assert(payloadSize >= 16);
  for (unsigned i = 0; i < 16; i++)
  {
    sei.uuid_iso_iec_11578[i] = bs.read(8);
  }

  sei.userDataLength = payloadSize - 16;
  if (!sei.userDataLength)
  {
    sei.userData = 0;
    return;
  }

  sei.userData = new unsigned char[sei.userDataLength];
  for (unsigned i = 0; i < sei.userDataLength; i++)
  {
    sei.userData[i] = bs.read(8);
  }
}

/**
 * parse bitstream @bs and unpack a picture_digest SEI message
 * of @payloadSize bytes into @sei.
 */
static void parseSEIpictureDigest(TComBitstream& bs, SEIpictureDigest& sei, unsigned payloadSize)
{
  assert(payloadSize >= 17);
  sei.method = static_cast<SEIpictureDigest::Method>(bs.read(8));
  assert(SEIpictureDigest::MD5 == sei.method);
  for (unsigned i = 0; i < 16; i++)
  {
    sei.digest[i] = bs.read(8);
  }
}

