

#include "../TLibCommon/TComBitCounter.h"
#include "../TLibCommon/TComBitStream.h"
#include "../TLibCommon/SEI.h"
#include "SEIwrite.h"

static void writeSEIuserDataUnregistered(TComBitIf& bs, const SEIuserDataUnregistered &sei);
static void writeSEIpictureDigest(TComBitIf& bs, const SEIpictureDigest& sei);

void writeSEIpayloadData(TComBitIf& bs, const SEI& sei)
{
  switch (sei.payloadType())
  {
  case SEI::USER_DATA_UNREGISTERED:
    writeSEIuserDataUnregistered(bs, *static_cast<const SEIuserDataUnregistered*>(&sei));
    break;
  case SEI::PICTURE_DIGEST:
    writeSEIpictureDigest(bs, *static_cast<const SEIpictureDigest*>(&sei));
    break;
  default:
    assert(!"Unhandled SEI message");
  }
}

/**
 * marshal a single SEI message @sei, storing the marshalled representation
 * in bitstream @bs.
 */
void writeSEImessage(TComBitIf& bs, const SEI& sei)
{
  /* calculate how large the payload data is */
  /* TODO: this would be far nicer if it used vectored buffers */
  TComBitCounter bs_count;
  bs_count.resetBits();
  writeSEIpayloadData(bs_count, sei);
  unsigned payload_data_num_bits = bs_count.getNumberOfWrittenBits();
  assert(0 == payload_data_num_bits % 8);

  unsigned payloadType = sei.payloadType();
  for (; payloadType >= 0xff; payloadType -= 0xff)
    bs.write(0xff, 8);
  bs.write(payloadType, 8);

  unsigned payloadSize = payload_data_num_bits/8;
  for (; payloadSize >= 0xff; payloadSize -= 0xff)
    bs.write(0xff, 8);
  bs.write(payloadSize, 8);

  /* payloadData */
  writeSEIpayloadData(bs, sei);
}

/**
 * marshal a user_data_unregistered SEI message @sei, storing the marshalled
 * representation in bitstream @bs.
 */
static void writeSEIuserDataUnregistered(TComBitIf& bs, const SEIuserDataUnregistered &sei)
{
  for (unsigned i = 0; i < 16; i++)
  {
    bs.write(sei.uuid_iso_iec_11578[i], 8);
  }

  for (unsigned i = 0; i < sei.userDataLength; i++)
  {
    bs.write(sei.userData[i], 8);
  }
}

/**
 * marshal a picture_digest SEI message, storing the marshalled
 * representation in bitstream @bs.
 */
static void writeSEIpictureDigest(TComBitIf& bs, const SEIpictureDigest& sei)
{
  bs.write(sei.method, 8);
  for (unsigned i = 0; i < 16; i++)
  {
    bs.write(sei.digest[i], 8);
  }
}
