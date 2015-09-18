/* The copyright in this software is being made available under the BSD
 * License, included below. This software may be subject to other third party
 * and contributor rights, including patent rights, and no such rights are
 * granted under this license.
 *
 * Copyright (c) 2010-2015, ITU/ISO/IEC
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *  * Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *  * Neither the name of the ITU/ISO/IEC nor the names of its contributors may
 *    be used to endorse or promote products derived from this software without
 *    specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

/**
 \file     SEIread.cpp
 \brief    reading funtionality for SEI messages
 */

#include "TLibCommon/CommonDef.h"
#include "TLibCommon/TComBitStream.h"
#include "TLibCommon/SEI.h"
#include "TLibCommon/TComSlice.h"
#include "SyntaxElementParser.h"
#include "SEIread.h"
#include "TLibCommon/TComPicYuv.h"
#include <iomanip>


//! \ingroup TLibDecoder
//! \{


#if ENC_DEC_TRACE
Void  xTraceSEIHeader()
{
  fprintf( g_hTrace, "=========== SEI message ===========\n");
}

Void  xTraceSEIMessageType(SEI::PayloadType payloadType)
{
  fprintf( g_hTrace, "=========== %s SEI message ===========\n", SEI::getSEIMessageString(payloadType));
}
#endif

Void SEIReader::sei_read_code(std::ostream *pOS, UInt uiLength, UInt& ruiCode, const Char *pSymbolName)
{
  READ_CODE(uiLength, ruiCode, pSymbolName);
  if (pOS)
  {
    (*pOS) << "  " << std::setw(55) << pSymbolName << ": " << ruiCode << "\n";
  }
}

Void SEIReader::sei_read_uvlc(std::ostream *pOS, UInt& ruiCode, const Char *pSymbolName)
{
  READ_UVLC(ruiCode, pSymbolName);
  if (pOS)
  {
    (*pOS) << "  " << std::setw(55) << pSymbolName << ": " << ruiCode << "\n";
  }
}

Void SEIReader::sei_read_svlc(std::ostream *pOS, Int& ruiCode, const Char *pSymbolName)
{
  READ_SVLC(ruiCode, pSymbolName);
  if (pOS)
  {
    (*pOS) << "  " << std::setw(55) << pSymbolName << ": " << ruiCode << "\n";
  }
}

Void SEIReader::sei_read_flag(std::ostream *pOS, UInt& ruiCode, const Char *pSymbolName)
{
  READ_FLAG(ruiCode, pSymbolName);
  if (pOS)
  {
    (*pOS) << "  " << std::setw(55) << pSymbolName << ": " << (ruiCode?1:0) << "\n";
  }
}

#if NH_MV_SEI
inline Void SEIReader::output_sei_message_header(SEI &sei, std::ostream *pDecodedMessageOutputStream, UInt payloadSize)
#else
static inline Void output_sei_message_header(SEI &sei, std::ostream *pDecodedMessageOutputStream, UInt payloadSize)
#endif
{
  if (pDecodedMessageOutputStream)
  {
    std::string seiMessageHdr(SEI::getSEIMessageString(sei.payloadType())); seiMessageHdr+=" SEI message";
    (*pDecodedMessageOutputStream) << std::setfill('-') << std::setw(seiMessageHdr.size()) << "-" << std::setfill(' ') << "\n" << seiMessageHdr << " (" << payloadSize << " bytes)"<< "\n";
#if NH_MV_SEI
    (*pDecodedMessageOutputStream) << std::setfill(' ') << "LayerId: " << m_layerId << std::setw(2) << " Picture: " << m_decOrder << std::setw( 5 ) << std::endl; 
#endif
  }
}

#undef READ_CODE
#undef READ_SVLC
#undef READ_UVLC
#undef READ_FLAG


/**
 * unmarshal a single SEI message from bitstream bs
 */
#if NH_MV_LAYERS_NOT_PRESENT_SEI
Void SEIReader::parseSEImessage(TComInputBitstream* bs, SEIMessages& seis, const NalUnitType nalUnitType, const TComVPS *vps, const TComSPS *sps, std::ostream *pDecodedMessageOutputStream)
#else
Void SEIReader::parseSEImessage(TComInputBitstream* bs, SEIMessages& seis, const NalUnitType nalUnitType, const TComSPS *sps, std::ostream *pDecodedMessageOutputStream)
#endif
{
  setBitstream(bs);

  assert(!m_pcBitstream->getNumBitsUntilByteAligned());
  do
  {
#if NH_MV_LAYERS_NOT_PRESENT_SEI
    xReadSEImessage(seis, nalUnitType, vps, sps, pDecodedMessageOutputStream);
#else
    xReadSEImessage(seis, nalUnitType, sps, pDecodedMessageOutputStream);
#endif
    /* SEI messages are an integer number of bytes, something has failed
    * in the parsing if bitstream not byte-aligned */
    assert(!m_pcBitstream->getNumBitsUntilByteAligned());
  }
  while (m_pcBitstream->getNumBitsLeft() > 8);

  xReadRbspTrailingBits();
}

#if NH_MV_LAYERS_NOT_PRESENT_SEI
Void SEIReader::xReadSEImessage(SEIMessages& seis, const NalUnitType nalUnitType, const TComVPS *vps, const TComSPS *sps, std::ostream *pDecodedMessageOutputStream)
#else
Void SEIReader::xReadSEImessage(SEIMessages& seis, const NalUnitType nalUnitType, const TComSPS *sps, std::ostream *pDecodedMessageOutputStream)
#endif
{
#if ENC_DEC_TRACE
  xTraceSEIHeader();
#endif
  Int payloadType = 0;
  UInt val = 0;

  do
  {
    sei_read_code(NULL, 8, val, "payload_type");
    payloadType += val;
  } while (val==0xFF);

  UInt payloadSize = 0;
  do
  {
    sei_read_code(NULL, 8, val, "payload_size");
    payloadSize += val;
  } while (val==0xFF);

#if ENC_DEC_TRACE
  xTraceSEIMessageType((SEI::PayloadType)payloadType);
#endif

  /* extract the payload for this single SEI message.
   * This allows greater safety in erroneous parsing of an SEI message
   * from affecting subsequent messages.
   * After parsing the payload, bs needs to be restored as the primary
   * bitstream.
   */
  TComInputBitstream *bs = getBitstream();
  setBitstream(bs->extractSubstream(payloadSize * 8));

  SEI *sei = NULL;

  if(nalUnitType == NAL_UNIT_PREFIX_SEI)
  {
    switch (payloadType)
    {
    case SEI::USER_DATA_UNREGISTERED:
      sei = new SEIuserDataUnregistered;
      xParseSEIuserDataUnregistered((SEIuserDataUnregistered&) *sei, payloadSize, pDecodedMessageOutputStream);
      break;
    case SEI::ACTIVE_PARAMETER_SETS:
      sei = new SEIActiveParameterSets;
      xParseSEIActiveParameterSets((SEIActiveParameterSets&) *sei, payloadSize, pDecodedMessageOutputStream);
      break;
    case SEI::DECODING_UNIT_INFO:
      if (!sps)
      {
        printf ("Warning: Found Decoding unit SEI message, but no active SPS is available. Ignoring.");
      }
      else
      {
        sei = new SEIDecodingUnitInfo;
        xParseSEIDecodingUnitInfo((SEIDecodingUnitInfo&) *sei, payloadSize, sps, pDecodedMessageOutputStream);
      }
      break;
    case SEI::BUFFERING_PERIOD:
      if (!sps)
      {
        printf ("Warning: Found Buffering period SEI message, but no active SPS is available. Ignoring.");
      }
      else
      {
        sei = new SEIBufferingPeriod;
        xParseSEIBufferingPeriod((SEIBufferingPeriod&) *sei, payloadSize, sps, pDecodedMessageOutputStream);
      }
      break;
    case SEI::PICTURE_TIMING:
      if (!sps)
      {
        printf ("Warning: Found Picture timing SEI message, but no active SPS is available. Ignoring.");
      }
      else
      {
        sei = new SEIPictureTiming;
        xParseSEIPictureTiming((SEIPictureTiming&)*sei, payloadSize, sps, pDecodedMessageOutputStream);
      }
      break;
    case SEI::RECOVERY_POINT:
      sei = new SEIRecoveryPoint;
      xParseSEIRecoveryPoint((SEIRecoveryPoint&) *sei, payloadSize, pDecodedMessageOutputStream);
      break;
    case SEI::FRAME_PACKING:
      sei = new SEIFramePacking;
      xParseSEIFramePacking((SEIFramePacking&) *sei, payloadSize, pDecodedMessageOutputStream);
      break;
    case SEI::SEGM_RECT_FRAME_PACKING:
      sei = new SEISegmentedRectFramePacking;
      xParseSEISegmentedRectFramePacking((SEISegmentedRectFramePacking&) *sei, payloadSize, pDecodedMessageOutputStream);
      break;
    case SEI::DISPLAY_ORIENTATION:
      sei = new SEIDisplayOrientation;
      xParseSEIDisplayOrientation((SEIDisplayOrientation&) *sei, payloadSize, pDecodedMessageOutputStream);
      break;
    case SEI::TEMPORAL_LEVEL0_INDEX:
      sei = new SEITemporalLevel0Index;
      xParseSEITemporalLevel0Index((SEITemporalLevel0Index&) *sei, payloadSize, pDecodedMessageOutputStream);
      break;
    case SEI::REGION_REFRESH_INFO:
      sei = new SEIGradualDecodingRefreshInfo;
      xParseSEIRegionRefreshInfo((SEIGradualDecodingRefreshInfo&) *sei, payloadSize, pDecodedMessageOutputStream);
      break;
    case SEI::NO_DISPLAY:
      sei = new SEINoDisplay;
      xParseSEINoDisplay((SEINoDisplay&) *sei, payloadSize, pDecodedMessageOutputStream);
      break;
    case SEI::TONE_MAPPING_INFO:
      sei = new SEIToneMappingInfo;
      xParseSEIToneMappingInfo((SEIToneMappingInfo&) *sei, payloadSize, pDecodedMessageOutputStream);
      break;
    case SEI::SOP_DESCRIPTION:
      sei = new SEISOPDescription;
      xParseSEISOPDescription((SEISOPDescription&) *sei, payloadSize, pDecodedMessageOutputStream);
      break;
    case SEI::SCALABLE_NESTING:
      sei = new SEIScalableNesting;
#if NH_MV_LAYERS_NOT_PRESENT_SEI
      xParseSEIScalableNesting((SEIScalableNesting&) *sei, nalUnitType, payloadSize, vps, sps, pDecodedMessageOutputStream);
#else
      xParseSEIScalableNesting((SEIScalableNesting&) *sei, nalUnitType, payloadSize, sps, pDecodedMessageOutputStream);
#endif
      break;
    case SEI::TEMP_MOTION_CONSTRAINED_TILE_SETS:
      sei = new SEITempMotionConstrainedTileSets;
      xParseSEITempMotionConstraintsTileSets((SEITempMotionConstrainedTileSets&) *sei, payloadSize, pDecodedMessageOutputStream);
      break;
    case SEI::TIME_CODE:
      sei = new SEITimeCode;
      xParseSEITimeCode((SEITimeCode&) *sei, payloadSize, pDecodedMessageOutputStream);
      break;
    case SEI::CHROMA_SAMPLING_FILTER_HINT:
      sei = new SEIChromaSamplingFilterHint;
      xParseSEIChromaSamplingFilterHint((SEIChromaSamplingFilterHint&) *sei, payloadSize/*, sps*/, pDecodedMessageOutputStream);
      //}
      break;
    case SEI::KNEE_FUNCTION_INFO:
      sei = new SEIKneeFunctionInfo;
      xParseSEIKneeFunctionInfo((SEIKneeFunctionInfo&) *sei, payloadSize, pDecodedMessageOutputStream);
      break;
    case SEI::MASTERING_DISPLAY_COLOUR_VOLUME:
      sei = new SEIMasteringDisplayColourVolume;
      xParseSEIMasteringDisplayColourVolume((SEIMasteringDisplayColourVolume&) *sei, payloadSize, pDecodedMessageOutputStream);
      break;
#if !NH_MV_SEI
    case SEI::SUB_BITSTREAM_PROPERTY:
      sei = new SEISubBitstreamProperty;
      xParseSEISubBitstreamProperty((SEISubBitstreamProperty&) *sei, payloadSize, pDecodedMessageOutputStream );
      break;
#else
#if NH_MV_LAYERS_NOT_PRESENT_SEI
    case SEI::LAYERS_NOT_PRESENT:
      if (!vps)
      {
        printf ("Warning: Found Layers not present SEI message, but no active VPS is available. Ignoring.");
      }
      else
      {
        sei = new SEILayersNotPresent;
        xParseSEILayersNotPresent((SEILayersNotPresent&) *sei, payloadSize, vps, pDecodedMessageOutputStream);
      }
      break;
#endif
    case SEI::INTER_LAYER_CONSTRAINED_TILE_SETS:
      sei = new SEIInterLayerConstrainedTileSets;
      xParseSEIInterLayerConstrainedTileSets((SEIInterLayerConstrainedTileSets&) *sei, payloadSize, pDecodedMessageOutputStream );
      break;
#if NH_MV_TBD
    case SEI::BSP_NESTING:
      sei = new SEIBspNesting;
      xParseSEIBspNesting((SEIBspNesting&) *sei, payloadSize, pDecodedMessageOutputStream );
      break;
    case SEI::BSP_INITIAL_ARRIVAL_TIME:
      sei = new SEIBspInitialArrivalTime;
      xParseSEIBspInitialArrivalTime((SEIBspInitialArrivalTime&) *sei, payloadSize, pDecodedMessageOutputStream );
      break;
#endif
    case SEI::SUB_BITSTREAM_PROPERTY:
      sei = new SEISubBitstreamProperty;
      xParseSEISubBitstreamProperty((SEISubBitstreamProperty&) *sei, payloadSize, pDecodedMessageOutputStream );
      break;
#if NH_MV_SEI_TBD
    case SEI::ALPHA_CHANNEL_INFO:
      sei = new SEIAlphaChannelInfo;
      xParseSEIAlphaChannelInfo((SEIAlphaChannelInfo&) *sei, payloadSize, pDecodedMessageOutputStream );
      break;
    case SEI::OVERLAY_INFO:
      sei = new SEIOverlayInfo;
      xParseSEIOverlayInfo((SEIOverlayInfo&) *sei, payloadSize, pDecodedMessageOutputStream );
      break;
#endif
    case SEI::TEMPORAL_MV_PREDICTION_CONSTRAINTS:
      sei = new SEITemporalMvPredictionConstraints;
      xParseSEITemporalMvPredictionConstraints((SEITemporalMvPredictionConstraints&) *sei, payloadSize, pDecodedMessageOutputStream );
      break;
#if NH_MV_SEI_TBD
    case SEI::FRAME_FIELD_INFO:
      sei = new SEIFrameFieldInfo;
      xParseSEIFrameFieldInfo((SEIFrameFieldInfo&) *sei, payloadSize, pDecodedMessageOutputStream );
      break;
    case SEI::THREE_DIMENSIONAL_REFERENCE_DISPLAYS_INFO:
      sei = new SEIThreeDimensionalReferenceDisplaysInfo;
      xParseSEIThreeDimensionalReferenceDisplaysInfo((SEIThreeDimensionalReferenceDisplaysInfo&) *sei, payloadSize, pDecodedMessageOutputStream );
      break;
    case SEI::DEPTH_REPRESENTATION_INFO:
      sei = new SEIDepthRepresentationInfo;
      xParseSEIDepthRepresentationInfo((SEIDepthRepresentationInfo&) *sei, payloadSize, pDecodedMessageOutputStream );
      break;
#endif
    case SEI::MULTIVIEW_SCENE_INFO:
      sei = new SEIMultiviewSceneInfo;
      xParseSEIMultiviewSceneInfo((SEIMultiviewSceneInfo&) *sei, payloadSize, pDecodedMessageOutputStream );
      break;

    case SEI::MULTIVIEW_ACQUISITION_INFO:
      sei = new SEIMultiviewAcquisitionInfo;
      xParseSEIMultiviewAcquisitionInfo((SEIMultiviewAcquisitionInfo&) *sei, payloadSize, pDecodedMessageOutputStream );
      break;

    case SEI::MULTIVIEW_VIEW_POSITION:
      sei = new SEIMultiviewViewPosition;
      xParseSEIMultiviewViewPosition((SEIMultiviewViewPosition&) *sei, payloadSize, pDecodedMessageOutputStream );
      break;
#if NH_MV_TBD
    case SEI::ALTERNATIVE_DEPTH_INFO:
      sei = new SEIAlternativeDepthInfo;
      xParseSEIAlternativeDepthInfo((SEIAlternativeDepthInfo&) *sei, payloadSize, pDecodedMessageOutputStream );
      break;
#endif
#endif
    default:
      for (UInt i = 0; i < payloadSize; i++)
      {
        UInt seiByte;
        sei_read_code (NULL, 8, seiByte, "unknown prefix SEI payload byte");
      }
      printf ("Unknown prefix SEI message (payloadType = %d) was found!\n", payloadType);
      if (pDecodedMessageOutputStream)
      {
        (*pDecodedMessageOutputStream) << "Unknown prefix SEI message (payloadType = " << payloadType << ") was found!\n";
      }
      break;
    }
  }
  else
  {
    switch (payloadType)
    {
      case SEI::USER_DATA_UNREGISTERED:
        sei = new SEIuserDataUnregistered;
        xParseSEIuserDataUnregistered((SEIuserDataUnregistered&) *sei, payloadSize, pDecodedMessageOutputStream);
        break;
      case SEI::DECODED_PICTURE_HASH:
        sei = new SEIDecodedPictureHash;
        xParseSEIDecodedPictureHash((SEIDecodedPictureHash&) *sei, payloadSize, pDecodedMessageOutputStream);
        break;
      default:
        for (UInt i = 0; i < payloadSize; i++)
        {
          UInt seiByte;
          sei_read_code( NULL, 8, seiByte, "unknown suffix SEI payload byte");
        }
        printf ("Unknown suffix SEI message (payloadType = %d) was found!\n", payloadType);
        if (pDecodedMessageOutputStream)
        {
          (*pDecodedMessageOutputStream) << "Unknown suffix SEI message (payloadType = " << payloadType << ") was found!\n";
        }
        break;
    }
  }

  if (sei != NULL)
  {
    seis.push_back(sei);
  }

  /* By definition the underlying bitstream terminates in a byte-aligned manner.
   * 1. Extract all bar the last MIN(bitsremaining,nine) bits as reserved_payload_extension_data
   * 2. Examine the final 8 bits to determine the payload_bit_equal_to_one marker
   * 3. Extract the remainingreserved_payload_extension_data bits.
   *
   * If there are fewer than 9 bits available, extract them.
   */
  Int payloadBitsRemaining = getBitstream()->getNumBitsLeft();
  if (payloadBitsRemaining) /* more_data_in_payload() */
  {
    for (; payloadBitsRemaining > 9; payloadBitsRemaining--)
    {
      UInt reservedPayloadExtensionData;
      sei_read_code ( pDecodedMessageOutputStream, 1, reservedPayloadExtensionData, "reserved_payload_extension_data");
    }

    /* 2 */
    Int finalBits = getBitstream()->peekBits(payloadBitsRemaining);
    Int finalPayloadBits = 0;
    for (Int mask = 0xff; finalBits & (mask >> finalPayloadBits); finalPayloadBits++)
    {
      continue;
    }

    /* 3 */
    for (; payloadBitsRemaining > 9 - finalPayloadBits; payloadBitsRemaining--)
    {
      UInt reservedPayloadExtensionData;
      sei_read_flag ( 0, reservedPayloadExtensionData, "reserved_payload_extension_data");
    }

    UInt dummy;
    sei_read_flag( 0, dummy, "payload_bit_equal_to_one"); payloadBitsRemaining--;
    while (payloadBitsRemaining)
    {
      sei_read_flag( 0, dummy, "payload_bit_equal_to_zero"); payloadBitsRemaining--;
    }
  }

  /* restore primary bitstream for sei_message */
  delete getBitstream();
  setBitstream(bs);
}

/**
 * parse bitstream bs and unpack a user_data_unregistered SEI message
 * of payloasSize bytes into sei.
 */

Void SEIReader::xParseSEIuserDataUnregistered(SEIuserDataUnregistered &sei, UInt payloadSize, std::ostream *pDecodedMessageOutputStream)
{
  assert(payloadSize >= ISO_IEC_11578_LEN);
  UInt val;
  output_sei_message_header(sei, pDecodedMessageOutputStream, payloadSize);

  for (UInt i = 0; i < ISO_IEC_11578_LEN; i++)
  {
    sei_read_code( pDecodedMessageOutputStream, 8, val, "uuid_iso_iec_11578");
    sei.uuid_iso_iec_11578[i] = val;
  }

  sei.userDataLength = payloadSize - ISO_IEC_11578_LEN;
  if (!sei.userDataLength)
  {
    sei.userData = 0;
    return;
  }

  sei.userData = new UChar[sei.userDataLength];
  for (UInt i = 0; i < sei.userDataLength; i++)
  {
    sei_read_code( NULL, 8, val, "user_data_payload_byte" );
    sei.userData[i] = val;
  }
  if (pDecodedMessageOutputStream)
  {
    (*pDecodedMessageOutputStream) << "  User data payload size: " << sei.userDataLength << "\n";
  }
}

/**
 * parse bitstream bs and unpack a decoded picture hash SEI message
 * of payloadSize bytes into sei.
 */
Void SEIReader::xParseSEIDecodedPictureHash(SEIDecodedPictureHash& sei, UInt payloadSize, std::ostream *pDecodedMessageOutputStream)
{
  UInt bytesRead = 0;
  output_sei_message_header(sei, pDecodedMessageOutputStream, payloadSize);

  UInt val;
  sei_read_code( pDecodedMessageOutputStream, 8, val, "hash_type");
  sei.method = static_cast<SEIDecodedPictureHash::Method>(val); bytesRead++;

  const Char *traceString="\0";
  switch (sei.method)
  {
    case SEIDecodedPictureHash::MD5: traceString="picture_md5"; break;
    case SEIDecodedPictureHash::CRC: traceString="picture_crc"; break;
    case SEIDecodedPictureHash::CHECKSUM: traceString="picture_checksum"; break;
    default: assert(false); break;
  }

  if (pDecodedMessageOutputStream)
  {
    (*pDecodedMessageOutputStream) << "  " << std::setw(55) << traceString << ": " << std::hex << std::setfill('0');
  }

  sei.m_pictureHash.hash.clear();
  for(;bytesRead < payloadSize; bytesRead++)
  {
    sei_read_code( NULL, 8, val, traceString);
    sei.m_pictureHash.hash.push_back((UChar)val);
    if (pDecodedMessageOutputStream)
    {
      (*pDecodedMessageOutputStream) << std::setw(2) << val;
    }
  }

  if (pDecodedMessageOutputStream)
  {
    (*pDecodedMessageOutputStream) << std::dec << std::setfill(' ') << "\n";
  }
}

Void SEIReader::xParseSEIActiveParameterSets(SEIActiveParameterSets& sei, UInt payloadSize, std::ostream *pDecodedMessageOutputStream)
{
  UInt val; 
  output_sei_message_header(sei, pDecodedMessageOutputStream, payloadSize);

  sei_read_code( pDecodedMessageOutputStream, 4, val, "active_video_parameter_set_id");   sei.activeVPSId = val;
  sei_read_flag( pDecodedMessageOutputStream,    val, "self_contained_cvs_flag");         sei.m_selfContainedCvsFlag     = (val != 0);
  sei_read_flag( pDecodedMessageOutputStream,    val, "no_parameter_set_update_flag");    sei.m_noParameterSetUpdateFlag = (val != 0);
  sei_read_uvlc( pDecodedMessageOutputStream,    val, "num_sps_ids_minus1");              sei.numSpsIdsMinus1 = val;

  sei.activeSeqParameterSetId.resize(sei.numSpsIdsMinus1 + 1);
  for (Int i=0; i < (sei.numSpsIdsMinus1 + 1); i++)
  {
    sei_read_uvlc( pDecodedMessageOutputStream, val, "active_seq_parameter_set_id[i]");    sei.activeSeqParameterSetId[i] = val;
  }
}

Void SEIReader::xParseSEIDecodingUnitInfo(SEIDecodingUnitInfo& sei, UInt payloadSize, const TComSPS *sps, std::ostream *pDecodedMessageOutputStream)
{
  UInt val;
  output_sei_message_header(sei, pDecodedMessageOutputStream, payloadSize);
  sei_read_uvlc( pDecodedMessageOutputStream, val, "decoding_unit_idx");
  sei.m_decodingUnitIdx = val;

  const TComVUI *vui = sps->getVuiParameters();
  if(vui->getHrdParameters()->getSubPicCpbParamsInPicTimingSEIFlag())
  {
    sei_read_code( pDecodedMessageOutputStream, ( vui->getHrdParameters()->getDuCpbRemovalDelayLengthMinus1() + 1 ), val, "du_spt_cpb_removal_delay_increment");
    sei.m_duSptCpbRemovalDelay = val;
  }
  else
  {
    sei.m_duSptCpbRemovalDelay = 0;
  }
  sei_read_flag( pDecodedMessageOutputStream, val, "dpb_output_du_delay_present_flag"); sei.m_dpbOutputDuDelayPresentFlag = (val != 0);
  if(sei.m_dpbOutputDuDelayPresentFlag)
  {
    sei_read_code( pDecodedMessageOutputStream, vui->getHrdParameters()->getDpbOutputDelayDuLengthMinus1() + 1, val, "pic_spt_dpb_output_du_delay");
    sei.m_picSptDpbOutputDuDelay = val;
  }
}

Void SEIReader::xParseSEIBufferingPeriod(SEIBufferingPeriod& sei, UInt payloadSize, const TComSPS *sps, std::ostream *pDecodedMessageOutputStream)
{
  Int i, nalOrVcl;
  UInt code;

  const TComVUI *pVUI = sps->getVuiParameters();
  const TComHRD *pHRD = pVUI->getHrdParameters();

  output_sei_message_header(sei, pDecodedMessageOutputStream, payloadSize);

  sei_read_uvlc( pDecodedMessageOutputStream, code, "bp_seq_parameter_set_id" );                         sei.m_bpSeqParameterSetId     = code;
  if( !pHRD->getSubPicCpbParamsPresentFlag() )
  {
    sei_read_flag( pDecodedMessageOutputStream, code, "irap_cpb_params_present_flag" );                   sei.m_rapCpbParamsPresentFlag = code;
  }
  if( sei.m_rapCpbParamsPresentFlag )
  {
    sei_read_code( pDecodedMessageOutputStream, pHRD->getCpbRemovalDelayLengthMinus1() + 1, code, "cpb_delay_offset" );      sei.m_cpbDelayOffset = code;
    sei_read_code( pDecodedMessageOutputStream, pHRD->getDpbOutputDelayLengthMinus1()  + 1, code, "dpb_delay_offset" );      sei.m_dpbDelayOffset = code;
  }

  //read splicing flag and cpb_removal_delay_delta
  sei_read_flag( pDecodedMessageOutputStream, code, "concatenation_flag");
  sei.m_concatenationFlag = code;
  sei_read_code( pDecodedMessageOutputStream, ( pHRD->getCpbRemovalDelayLengthMinus1() + 1 ), code, "au_cpb_removal_delay_delta_minus1" );
  sei.m_auCpbRemovalDelayDelta = code + 1;

  for( nalOrVcl = 0; nalOrVcl < 2; nalOrVcl ++ )
  {
    if( ( ( nalOrVcl == 0 ) && ( pHRD->getNalHrdParametersPresentFlag() ) ) ||
        ( ( nalOrVcl == 1 ) && ( pHRD->getVclHrdParametersPresentFlag() ) ) )
    {
      for( i = 0; i < ( pHRD->getCpbCntMinus1( 0 ) + 1 ); i ++ )
      {
        sei_read_code( pDecodedMessageOutputStream, ( pHRD->getInitialCpbRemovalDelayLengthMinus1() + 1 ) , code, nalOrVcl?"vcl_initial_cpb_removal_delay":"nal_initial_cpb_removal_delay" );
        sei.m_initialCpbRemovalDelay[i][nalOrVcl] = code;
        sei_read_code( pDecodedMessageOutputStream, ( pHRD->getInitialCpbRemovalDelayLengthMinus1() + 1 ) , code, nalOrVcl?"vcl_initial_cpb_removal_offset":"vcl_initial_cpb_removal_offset" );
        sei.m_initialCpbRemovalDelayOffset[i][nalOrVcl] = code;
        if( pHRD->getSubPicCpbParamsPresentFlag() || sei.m_rapCpbParamsPresentFlag )
        {
          sei_read_code( pDecodedMessageOutputStream, ( pHRD->getInitialCpbRemovalDelayLengthMinus1() + 1 ) , code, nalOrVcl?"vcl_initial_alt_cpb_removal_delay":"vcl_initial_alt_cpb_removal_delay" );
          sei.m_initialAltCpbRemovalDelay[i][nalOrVcl] = code;
          sei_read_code( pDecodedMessageOutputStream, ( pHRD->getInitialCpbRemovalDelayLengthMinus1() + 1 ) , code, nalOrVcl?"vcl_initial_alt_cpb_removal_offset":"vcl_initial_alt_cpb_removal_offset" );
          sei.m_initialAltCpbRemovalDelayOffset[i][nalOrVcl] = code;
        }
      }
    }
  }
}

Void SEIReader::xParseSEIPictureTiming(SEIPictureTiming& sei, UInt payloadSize, const TComSPS *sps, std::ostream *pDecodedMessageOutputStream)
{
  Int i;
  UInt code;

  const TComVUI *vui = sps->getVuiParameters();
  const TComHRD *hrd = vui->getHrdParameters();
  output_sei_message_header(sei, pDecodedMessageOutputStream, payloadSize);

  if( vui->getFrameFieldInfoPresentFlag() )
  {
    sei_read_code( pDecodedMessageOutputStream, 4, code, "pic_struct" );             sei.m_picStruct            = code;
    sei_read_code( pDecodedMessageOutputStream, 2, code, "source_scan_type" );       sei.m_sourceScanType       = code;
    sei_read_flag( pDecodedMessageOutputStream,    code, "duplicate_flag" );         sei.m_duplicateFlag        = (code == 1);
  }

  if( hrd->getCpbDpbDelaysPresentFlag())
  {
    sei_read_code( pDecodedMessageOutputStream, ( hrd->getCpbRemovalDelayLengthMinus1() + 1 ), code, "au_cpb_removal_delay_minus1" );
    sei.m_auCpbRemovalDelay = code + 1;
    sei_read_code( pDecodedMessageOutputStream, ( hrd->getDpbOutputDelayLengthMinus1() + 1 ), code, "pic_dpb_output_delay" );
    sei.m_picDpbOutputDelay = code;

    if(hrd->getSubPicCpbParamsPresentFlag())
    {
      sei_read_code( pDecodedMessageOutputStream, hrd->getDpbOutputDelayDuLengthMinus1()+1, code, "pic_dpb_output_du_delay" );
      sei.m_picDpbOutputDuDelay = code;
    }

    if( hrd->getSubPicCpbParamsPresentFlag() && hrd->getSubPicCpbParamsInPicTimingSEIFlag() )
    {
      sei_read_uvlc( pDecodedMessageOutputStream, code, "num_decoding_units_minus1");
      sei.m_numDecodingUnitsMinus1 = code;
      sei_read_flag( pDecodedMessageOutputStream, code, "du_common_cpb_removal_delay_flag" );
      sei.m_duCommonCpbRemovalDelayFlag = code;
      if( sei.m_duCommonCpbRemovalDelayFlag )
      {
        sei_read_code( pDecodedMessageOutputStream, ( hrd->getDuCpbRemovalDelayLengthMinus1() + 1 ), code, "du_common_cpb_removal_delay_increment_minus1" );
        sei.m_duCommonCpbRemovalDelayMinus1 = code;
      }
      sei.m_numNalusInDuMinus1.resize(sei.m_numDecodingUnitsMinus1 + 1 );
      sei.m_duCpbRemovalDelayMinus1.resize( sei.m_numDecodingUnitsMinus1 + 1 );

      for( i = 0; i <= sei.m_numDecodingUnitsMinus1; i ++ )
      {
        sei_read_uvlc( pDecodedMessageOutputStream, code, "num_nalus_in_du_minus1[i]");
        sei.m_numNalusInDuMinus1[ i ] = code;
        if( ( !sei.m_duCommonCpbRemovalDelayFlag ) && ( i < sei.m_numDecodingUnitsMinus1 ) )
        {
          sei_read_code( pDecodedMessageOutputStream, ( hrd->getDuCpbRemovalDelayLengthMinus1() + 1 ), code, "du_cpb_removal_delay_minus1[i]" );
          sei.m_duCpbRemovalDelayMinus1[ i ] = code;
        }
      }
    }
  }
}

Void SEIReader::xParseSEIRecoveryPoint(SEIRecoveryPoint& sei, UInt payloadSize, std::ostream *pDecodedMessageOutputStream)
{
  Int  iCode;
  UInt uiCode;
  output_sei_message_header(sei, pDecodedMessageOutputStream, payloadSize);

  sei_read_svlc( pDecodedMessageOutputStream, iCode,  "recovery_poc_cnt" );      sei.m_recoveryPocCnt     = iCode;
  sei_read_flag( pDecodedMessageOutputStream, uiCode, "exact_matching_flag" );   sei.m_exactMatchingFlag  = uiCode;
  sei_read_flag( pDecodedMessageOutputStream, uiCode, "broken_link_flag" );      sei.m_brokenLinkFlag     = uiCode;
}

Void SEIReader::xParseSEIFramePacking(SEIFramePacking& sei, UInt payloadSize, std::ostream *pDecodedMessageOutputStream)
{
  UInt val;
  output_sei_message_header(sei, pDecodedMessageOutputStream, payloadSize);

  sei_read_uvlc( pDecodedMessageOutputStream, val, "frame_packing_arrangement_id" );                 sei.m_arrangementId = val;
  sei_read_flag( pDecodedMessageOutputStream, val, "frame_packing_arrangement_cancel_flag" );        sei.m_arrangementCancelFlag = val;

  if ( !sei.m_arrangementCancelFlag )
  {
    sei_read_code( pDecodedMessageOutputStream, 7, val, "frame_packing_arrangement_type" );          sei.m_arrangementType = val;
    assert((sei.m_arrangementType > 2) && (sei.m_arrangementType < 6) );

    sei_read_flag( pDecodedMessageOutputStream, val, "quincunx_sampling_flag" );                     sei.m_quincunxSamplingFlag = val;

    sei_read_code( pDecodedMessageOutputStream, 6, val, "content_interpretation_type" );             sei.m_contentInterpretationType = val;
    sei_read_flag( pDecodedMessageOutputStream, val, "spatial_flipping_flag" );                      sei.m_spatialFlippingFlag = val;
    sei_read_flag( pDecodedMessageOutputStream, val, "frame0_flipped_flag" );                        sei.m_frame0FlippedFlag = val;
    sei_read_flag( pDecodedMessageOutputStream, val, "field_views_flag" );                           sei.m_fieldViewsFlag = val;
    sei_read_flag( pDecodedMessageOutputStream, val, "current_frame_is_frame0_flag" );               sei.m_currentFrameIsFrame0Flag = val;
    sei_read_flag( pDecodedMessageOutputStream, val, "frame0_self_contained_flag" );                 sei.m_frame0SelfContainedFlag = val;
    sei_read_flag( pDecodedMessageOutputStream, val, "frame1_self_contained_flag" );                 sei.m_frame1SelfContainedFlag = val;

    if ( sei.m_quincunxSamplingFlag == 0 && sei.m_arrangementType != 5)
    {
      sei_read_code( pDecodedMessageOutputStream, 4, val, "frame0_grid_position_x" );                sei.m_frame0GridPositionX = val;
      sei_read_code( pDecodedMessageOutputStream, 4, val, "frame0_grid_position_y" );                sei.m_frame0GridPositionY = val;
      sei_read_code( pDecodedMessageOutputStream, 4, val, "frame1_grid_position_x" );                sei.m_frame1GridPositionX = val;
      sei_read_code( pDecodedMessageOutputStream, 4, val, "frame1_grid_position_y" );                sei.m_frame1GridPositionY = val;
    }

    sei_read_code( pDecodedMessageOutputStream, 8, val, "frame_packing_arrangement_reserved_byte" );   sei.m_arrangementReservedByte = val;
    sei_read_flag( pDecodedMessageOutputStream, val,  "frame_packing_arrangement_persistence_flag" );  sei.m_arrangementPersistenceFlag = (val != 0);
  }
  sei_read_flag( pDecodedMessageOutputStream, val, "upsampled_aspect_ratio_flag" );                  sei.m_upsampledAspectRatio = val;
}

Void SEIReader::xParseSEISegmentedRectFramePacking(SEISegmentedRectFramePacking& sei, UInt payloadSize, std::ostream *pDecodedMessageOutputStream)
{
  UInt val;
  output_sei_message_header(sei, pDecodedMessageOutputStream, payloadSize);
  sei_read_flag( pDecodedMessageOutputStream, val,       "segmented_rect_frame_packing_arrangement_cancel_flag" );       sei.m_arrangementCancelFlag            = val;
  if( !sei.m_arrangementCancelFlag )
  {
    sei_read_code( pDecodedMessageOutputStream, 2, val, "segmented_rect_content_interpretation_type" );                sei.m_contentInterpretationType = val;
    sei_read_flag( pDecodedMessageOutputStream, val,     "segmented_rect_frame_packing_arrangement_persistence" );                              sei.m_arrangementPersistenceFlag               = val;
  }
}

Void SEIReader::xParseSEIDisplayOrientation(SEIDisplayOrientation& sei, UInt payloadSize, std::ostream *pDecodedMessageOutputStream)
{
  UInt val;
  output_sei_message_header(sei, pDecodedMessageOutputStream, payloadSize);
  sei_read_flag( pDecodedMessageOutputStream, val,       "display_orientation_cancel_flag" );       sei.cancelFlag            = val;
  if( !sei.cancelFlag )
  {
    sei_read_flag( pDecodedMessageOutputStream, val,     "hor_flip" );                              sei.horFlip               = val;
    sei_read_flag( pDecodedMessageOutputStream, val,     "ver_flip" );                              sei.verFlip               = val;
    sei_read_code( pDecodedMessageOutputStream, 16, val, "anticlockwise_rotation" );                sei.anticlockwiseRotation = val;
    sei_read_flag( pDecodedMessageOutputStream, val,     "display_orientation_persistence_flag" );  sei.persistenceFlag       = val;
  }
}

Void SEIReader::xParseSEITemporalLevel0Index(SEITemporalLevel0Index& sei, UInt payloadSize, std::ostream *pDecodedMessageOutputStream)
{
  UInt val;
  output_sei_message_header(sei, pDecodedMessageOutputStream, payloadSize);
  sei_read_code( pDecodedMessageOutputStream, 8, val, "temporal_sub_layer_zero_idx" );  sei.tl0Idx = val;
  sei_read_code( pDecodedMessageOutputStream, 8, val, "irap_pic_id" );  sei.rapIdx = val;
}

Void SEIReader::xParseSEIRegionRefreshInfo(SEIGradualDecodingRefreshInfo& sei, UInt payloadSize, std::ostream *pDecodedMessageOutputStream)
{
  UInt val;
  output_sei_message_header(sei, pDecodedMessageOutputStream, payloadSize);
  sei_read_flag( pDecodedMessageOutputStream, val, "refreshed_region_flag" ); sei.m_gdrForegroundFlag = val ? 1 : 0;
}

Void SEIReader::xParseSEINoDisplay(SEINoDisplay& sei, UInt payloadSize, std::ostream *pDecodedMessageOutputStream)
{
  output_sei_message_header(sei, pDecodedMessageOutputStream, payloadSize);
  sei.m_noDisplay = true;
}

Void SEIReader::xParseSEIToneMappingInfo(SEIToneMappingInfo& sei, UInt payloadSize, std::ostream *pDecodedMessageOutputStream)
{
  Int i;
  UInt val;
  output_sei_message_header(sei, pDecodedMessageOutputStream, payloadSize);
  sei_read_uvlc( pDecodedMessageOutputStream, val, "tone_map_id" );                         sei.m_toneMapId = val;
  sei_read_flag( pDecodedMessageOutputStream, val, "tone_map_cancel_flag" );                sei.m_toneMapCancelFlag = val;

  if ( !sei.m_toneMapCancelFlag )
  {
    sei_read_flag( pDecodedMessageOutputStream, val, "tone_map_persistence_flag" );         sei.m_toneMapPersistenceFlag = val;
    sei_read_code( pDecodedMessageOutputStream, 8, val, "coded_data_bit_depth" );           sei.m_codedDataBitDepth = val;
    sei_read_code( pDecodedMessageOutputStream, 8, val, "target_bit_depth" );               sei.m_targetBitDepth = val;
    sei_read_uvlc( pDecodedMessageOutputStream, val, "tone_map_model_id" );                 sei.m_modelId = val;
    switch(sei.m_modelId)
    {
    case 0:
      {
        sei_read_code( pDecodedMessageOutputStream, 32, val, "min_value" );                 sei.m_minValue = val;
        sei_read_code( pDecodedMessageOutputStream, 32, val, "max_value" );                 sei.m_maxValue = val;
        break;
      }
    case 1:
      {
        sei_read_code( pDecodedMessageOutputStream, 32, val, "sigmoid_midpoint" );          sei.m_sigmoidMidpoint = val;
        sei_read_code( pDecodedMessageOutputStream, 32, val, "sigmoid_width" );             sei.m_sigmoidWidth = val;
        break;
      }
    case 2:
      {
        UInt num = 1u << sei.m_targetBitDepth;
        sei.m_startOfCodedInterval.resize(num+1);
        for(i = 0; i < num; i++)
        {
          sei_read_code( pDecodedMessageOutputStream, ((( sei.m_codedDataBitDepth + 7 ) >> 3 ) << 3), val, "start_of_coded_interval[i]" );
          sei.m_startOfCodedInterval[i] = val;
        }
        sei.m_startOfCodedInterval[num] = 1u << sei.m_codedDataBitDepth;
        break;
      }
    case 3:
      {
        sei_read_code( pDecodedMessageOutputStream, 16, val,  "num_pivots" );                       sei.m_numPivots = val;
        sei.m_codedPivotValue.resize(sei.m_numPivots);
        sei.m_targetPivotValue.resize(sei.m_numPivots);
        for(i = 0; i < sei.m_numPivots; i++ )
        {
          sei_read_code( pDecodedMessageOutputStream, ((( sei.m_codedDataBitDepth + 7 ) >> 3 ) << 3), val, "coded_pivot_value[i]" );
          sei.m_codedPivotValue[i] = val;
          sei_read_code( pDecodedMessageOutputStream, ((( sei.m_targetBitDepth + 7 ) >> 3 ) << 3),    val, "target_pivot_value[i]" );
          sei.m_targetPivotValue[i] = val;
        }
        break;
      }
    case 4:
      {
        sei_read_code( pDecodedMessageOutputStream, 8, val, "camera_iso_speed_idc" );                     sei.m_cameraIsoSpeedIdc = val;
        if( sei.m_cameraIsoSpeedIdc == 255) //Extended_ISO
        {
          sei_read_code( pDecodedMessageOutputStream, 32,   val,   "camera_iso_speed_value" );            sei.m_cameraIsoSpeedValue = val;
        }
        sei_read_code( pDecodedMessageOutputStream, 8, val, "exposure_index_idc" );                       sei.m_exposureIndexIdc = val;
        if( sei.m_exposureIndexIdc == 255) //Extended_ISO
        {
          sei_read_code( pDecodedMessageOutputStream, 32,   val,   "exposure_index_value" );              sei.m_exposureIndexValue = val;
        }
        sei_read_flag( pDecodedMessageOutputStream, val, "exposure_compensation_value_sign_flag" );       sei.m_exposureCompensationValueSignFlag = val;
        sei_read_code( pDecodedMessageOutputStream, 16, val, "exposure_compensation_value_numerator" );   sei.m_exposureCompensationValueNumerator = val;
        sei_read_code( pDecodedMessageOutputStream, 16, val, "exposure_compensation_value_denom_idc" );   sei.m_exposureCompensationValueDenomIdc = val;
        sei_read_code( pDecodedMessageOutputStream, 32, val, "ref_screen_luminance_white" );              sei.m_refScreenLuminanceWhite = val;
        sei_read_code( pDecodedMessageOutputStream, 32, val, "extended_range_white_level" );              sei.m_extendedRangeWhiteLevel = val;
        sei_read_code( pDecodedMessageOutputStream, 16, val, "nominal_black_level_code_value" );          sei.m_nominalBlackLevelLumaCodeValue = val;
        sei_read_code( pDecodedMessageOutputStream, 16, val, "nominal_white_level_code_value" );          sei.m_nominalWhiteLevelLumaCodeValue= val;
        sei_read_code( pDecodedMessageOutputStream, 16, val, "extended_white_level_code_value" );         sei.m_extendedWhiteLevelLumaCodeValue = val;
        break;
      }
    default:
      {
        assert(!"Undefined SEIToneMapModelId");
        break;
      }
    }//switch model id
  }// if(!sei.m_toneMapCancelFlag)
}

Void SEIReader::xParseSEISOPDescription(SEISOPDescription &sei, UInt payloadSize, std::ostream *pDecodedMessageOutputStream)
{
  Int iCode;
  UInt uiCode;
  output_sei_message_header(sei, pDecodedMessageOutputStream, payloadSize);

  sei_read_uvlc( pDecodedMessageOutputStream, uiCode,           "sop_seq_parameter_set_id"            ); sei.m_sopSeqParameterSetId = uiCode;
  sei_read_uvlc( pDecodedMessageOutputStream, uiCode,           "num_pics_in_sop_minus1"              ); sei.m_numPicsInSopMinus1 = uiCode;
  for (UInt i = 0; i <= sei.m_numPicsInSopMinus1; i++)
  {
    sei_read_code( pDecodedMessageOutputStream, 6, uiCode,                     "sop_vcl_nut[i]" );  sei.m_sopDescVclNaluType[i] = uiCode;
    sei_read_code( pDecodedMessageOutputStream, 3, sei.m_sopDescTemporalId[i], "sop_temporal_id[i]"   );  sei.m_sopDescTemporalId[i] = uiCode;
    if (sei.m_sopDescVclNaluType[i] != NAL_UNIT_CODED_SLICE_IDR_W_RADL && sei.m_sopDescVclNaluType[i] != NAL_UNIT_CODED_SLICE_IDR_N_LP)
    {
      sei_read_uvlc( pDecodedMessageOutputStream, sei.m_sopDescStRpsIdx[i],    "sop_short_term_rps_idx[i]"    ); sei.m_sopDescStRpsIdx[i] = uiCode;
    }
    if (i > 0)
    {
      sei_read_svlc( pDecodedMessageOutputStream, iCode,                       "sop_poc_delta[i]"     ); sei.m_sopDescPocDelta[i] = iCode;
    }
  }
}

#if NH_MV_LAYERS_NOT_PRESENT_SEI
Void SEIReader::xParseSEIScalableNesting(SEIScalableNesting& sei, const NalUnitType nalUnitType, UInt payloadSize, const TComVPS *vps, const TComSPS *sps, std::ostream *pDecodedMessageOutputStream)
#else
Void SEIReader::xParseSEIScalableNesting(SEIScalableNesting& sei, const NalUnitType nalUnitType, UInt payloadSize, const TComSPS *sps, std::ostream *pDecodedMessageOutputStream)
#endif
{
  UInt uiCode;
  SEIMessages seis;
  output_sei_message_header(sei, pDecodedMessageOutputStream, payloadSize);

  sei_read_flag( pDecodedMessageOutputStream, uiCode,            "bitstream_subset_flag"         ); sei.m_bitStreamSubsetFlag = uiCode;
  sei_read_flag( pDecodedMessageOutputStream, uiCode,            "nesting_op_flag"               ); sei.m_nestingOpFlag = uiCode;
  if (sei.m_nestingOpFlag)
  {
    sei_read_flag( pDecodedMessageOutputStream, uiCode,            "default_op_flag"               ); sei.m_defaultOpFlag = uiCode;
    sei_read_uvlc( pDecodedMessageOutputStream, uiCode,            "nesting_num_ops_minus1"        ); sei.m_nestingNumOpsMinus1 = uiCode;
    for (UInt i = sei.m_defaultOpFlag; i <= sei.m_nestingNumOpsMinus1; i++)
    {
      sei_read_code( pDecodedMessageOutputStream, 3,        uiCode,  "nesting_max_temporal_id_plus1[i]"   ); sei.m_nestingMaxTemporalIdPlus1[i] = uiCode;
      sei_read_uvlc( pDecodedMessageOutputStream, uiCode,            "nesting_op_idx[i]"                  ); sei.m_nestingOpIdx[i] = uiCode;
    }
  }
  else
  {
    sei_read_flag( pDecodedMessageOutputStream, uiCode,            "all_layers_flag"               ); sei.m_allLayersFlag       = uiCode;
    if (!sei.m_allLayersFlag)
    {
      sei_read_code( pDecodedMessageOutputStream, 3,        uiCode,  "nesting_no_op_max_temporal_id_plus1"  ); sei.m_nestingNoOpMaxTemporalIdPlus1 = uiCode;
      sei_read_uvlc( pDecodedMessageOutputStream, uiCode,            "nesting_num_layers_minus1"            ); sei.m_nestingNumLayersMinus1        = uiCode;
      for (UInt i = 0; i <= sei.m_nestingNumLayersMinus1; i++)
      {
        sei_read_code( pDecodedMessageOutputStream, 6,           uiCode,     "nesting_layer_id[i]"      ); sei.m_nestingLayerId[i]   = uiCode;
      }
    }
  }

  // byte alignment
  while ( m_pcBitstream->getNumBitsRead() % 8 != 0 )
  {
    UInt code;
    sei_read_flag( pDecodedMessageOutputStream, code, "nesting_zero_bit" );
  }

  // read nested SEI messages
  do
  {
#if NH_MV_LAYERS_NOT_PRESENT_SEI
    xReadSEImessage(sei.m_nestedSEIs, nalUnitType, vps, sps, pDecodedMessageOutputStream);
#else
    xReadSEImessage(sei.m_nestedSEIs, nalUnitType, sps, pDecodedMessageOutputStream);
#endif
  } while (m_pcBitstream->getNumBitsLeft() > 8);

  if (pDecodedMessageOutputStream)
  {
    (*pDecodedMessageOutputStream) << "End of scalable nesting SEI message\n";
  }
}

#if NH_MV
#if !NH_MV_SEI
Void SEIReader::xParseSEISubBitstreamProperty(SEISubBitstreamProperty &sei, UInt payloadSize, std::ostream *pDecodedMessageOutputStream )
{
  UInt code;
  output_sei_message_header(sei, pDecodedMessageOutputStream, payloadSize);
  sei_read_code( pDecodedMessageOutputStream, 4, code, "active_vps_id" );                      sei.m_activeVpsId = code;
  sei_read_uvlc( pDecodedMessageOutputStream, code, "num_additional_sub_streams_minus1" );     sei.m_numAdditionalSubStreams = code + 1;

  xResizeSubBitstreamPropertySeiArrays(sei);
  for( Int i = 0; i < sei.m_numAdditionalSubStreams; i++ )
  {
    sei_read_code( pDecodedMessageOutputStream,   2, code, "sub_bitstream_mode[i]"           ); sei.m_subBitstreamMode[i] = code;
    sei_read_uvlc( pDecodedMessageOutputStream,  code, "output_layer_set_idx_to_vps[i]"      ); sei.m_outputLayerSetIdxToVps[i] = code;
    sei_read_code( pDecodedMessageOutputStream,   3, code, "highest_sub_layer_id[i]"         ); sei.m_highestSublayerId[i] = code;
    sei_read_code( pDecodedMessageOutputStream,  16, code, "avg_bit_rate[i]"                 ); sei.m_avgBitRate[i] = code;
    sei_read_code( pDecodedMessageOutputStream,  16, code, "max_bit_rate[i]"                 ); sei.m_maxBitRate[i] = code;
  }  
}

Void SEIReader::xResizeSubBitstreamPropertySeiArrays(SEISubBitstreamProperty &sei)
{
  sei.m_subBitstreamMode.resize( sei.m_numAdditionalSubStreams );
  sei.m_outputLayerSetIdxToVps.resize( sei.m_numAdditionalSubStreams );
  sei.m_highestSublayerId.resize( sei.m_numAdditionalSubStreams );
  sei.m_avgBitRate.resize( sei.m_numAdditionalSubStreams );
  sei.m_maxBitRate.resize( sei.m_numAdditionalSubStreams );
}
#endif
#endif


Void SEIReader::xParseSEITempMotionConstraintsTileSets(SEITempMotionConstrainedTileSets& sei, UInt payloadSize, std::ostream *pDecodedMessageOutputStream)
{
  UInt code;
  output_sei_message_header(sei, pDecodedMessageOutputStream, payloadSize);
  sei_read_flag( pDecodedMessageOutputStream, code, "mc_all_tiles_exact_sample_value_match_flag");  sei.m_mc_all_tiles_exact_sample_value_match_flag = (code != 0);
  sei_read_flag( pDecodedMessageOutputStream, code, "each_tile_one_tile_set_flag");                 sei.m_each_tile_one_tile_set_flag                = (code != 0);

  if(!sei.m_each_tile_one_tile_set_flag)
  {
    sei_read_flag( pDecodedMessageOutputStream, code, "limited_tile_set_display_flag");  sei.m_limited_tile_set_display_flag = (code != 0);
    sei_read_uvlc( pDecodedMessageOutputStream, code, "num_sets_in_message_minus1");     sei.setNumberOfTileSets(code + 1);

    if(sei.getNumberOfTileSets() != 0)
    {
      for(Int i = 0; i < sei.getNumberOfTileSets(); i++)
      {
        sei_read_uvlc( pDecodedMessageOutputStream, code, "mcts_id");  sei.tileSetData(i).m_mcts_id = code;

        if(sei.m_limited_tile_set_display_flag)
        {
          sei_read_flag( pDecodedMessageOutputStream, code, "display_tile_set_flag");  sei.tileSetData(i).m_display_tile_set_flag = (code != 1);
        }

        sei_read_uvlc( pDecodedMessageOutputStream, code, "num_tile_rects_in_set_minus1");  sei.tileSetData(i).setNumberOfTileRects(code + 1);

        for(Int j=0; j<sei.tileSetData(i).getNumberOfTileRects(); j++)
        {
          sei_read_uvlc( pDecodedMessageOutputStream, code, "top_left_tile_index");      sei.tileSetData(i).topLeftTileIndex(j)     = code;
          sei_read_uvlc( pDecodedMessageOutputStream, code, "bottom_right_tile_index");  sei.tileSetData(i).bottomRightTileIndex(j) = code;
        }

        if(!sei.m_mc_all_tiles_exact_sample_value_match_flag)
        {
          sei_read_flag( pDecodedMessageOutputStream, code, "exact_sample_value_match_flag");   sei.tileSetData(i).m_exact_sample_value_match_flag    = (code != 0);
        }
        sei_read_flag( pDecodedMessageOutputStream, code, "mcts_tier_level_idc_present_flag");  sei.tileSetData(i).m_mcts_tier_level_idc_present_flag = (code != 0);

        if(sei.tileSetData(i).m_mcts_tier_level_idc_present_flag)
        {
          sei_read_flag( pDecodedMessageOutputStream, code,    "mcts_tier_flag"); sei.tileSetData(i).m_mcts_tier_flag = (code != 0);
          sei_read_code( pDecodedMessageOutputStream, 8, code, "mcts_level_idc"); sei.tileSetData(i).m_mcts_level_idc =  code;
        }
      }
    }
  }
  else
  {
    sei_read_flag( pDecodedMessageOutputStream, code, "max_mcs_tier_level_idc_present_flag");  sei.m_max_mcs_tier_level_idc_present_flag = code;
    if(sei.m_max_mcs_tier_level_idc_present_flag)
    {
      sei_read_flag( pDecodedMessageOutputStream, code, "max_mcts_tier_flag");  sei.m_max_mcts_tier_flag = code;
      sei_read_code( pDecodedMessageOutputStream, 8, code, "max_mcts_level_idc"); sei.m_max_mcts_level_idc = code;
    }
  }
}

Void SEIReader::xParseSEITimeCode(SEITimeCode& sei, UInt payloadSize, std::ostream *pDecodedMessageOutputStream)
{
  UInt code;
  output_sei_message_header(sei, pDecodedMessageOutputStream, payloadSize);
  sei_read_code( pDecodedMessageOutputStream, 2, code, "num_clock_ts"); sei.numClockTs = code;
  for(Int i = 0; i < sei.numClockTs; i++)
  {
    TComSEITimeSet currentTimeSet;
    sei_read_flag( pDecodedMessageOutputStream, code, "clock_time_stamp_flag[i]"); currentTimeSet.clockTimeStampFlag = code;
    if(currentTimeSet.clockTimeStampFlag)
    {
      sei_read_flag( pDecodedMessageOutputStream, code, "nuit_field_based_flag"); currentTimeSet.numUnitFieldBasedFlag = code;
      sei_read_code( pDecodedMessageOutputStream, 5, code, "counting_type"); currentTimeSet.countingType = code;
      sei_read_flag( pDecodedMessageOutputStream, code, "full_timestamp_flag"); currentTimeSet.fullTimeStampFlag = code;
      sei_read_flag( pDecodedMessageOutputStream, code, "discontinuity_flag"); currentTimeSet.discontinuityFlag = code;
      sei_read_flag( pDecodedMessageOutputStream, code, "cnt_dropped_flag"); currentTimeSet.cntDroppedFlag = code;
      sei_read_code( pDecodedMessageOutputStream, 9, code, "n_frames"); currentTimeSet.numberOfFrames = code;
      if(currentTimeSet.fullTimeStampFlag)
      {
        sei_read_code( pDecodedMessageOutputStream, 6, code, "seconds_value"); currentTimeSet.secondsValue = code;
        sei_read_code( pDecodedMessageOutputStream, 6, code, "minutes_value"); currentTimeSet.minutesValue = code;
        sei_read_code( pDecodedMessageOutputStream, 5, code, "hours_value"); currentTimeSet.hoursValue = code;
      }
      else
      {
        sei_read_flag( pDecodedMessageOutputStream, code, "seconds_flag"); currentTimeSet.secondsFlag = code;
        if(currentTimeSet.secondsFlag)
        {
          sei_read_code( pDecodedMessageOutputStream, 6, code, "seconds_value"); currentTimeSet.secondsValue = code;
          sei_read_flag( pDecodedMessageOutputStream, code, "minutes_flag"); currentTimeSet.minutesFlag = code;
          if(currentTimeSet.minutesFlag)
          {
            sei_read_code( pDecodedMessageOutputStream, 6, code, "minutes_value"); currentTimeSet.minutesValue = code;
            sei_read_flag( pDecodedMessageOutputStream, code, "hours_flag"); currentTimeSet.hoursFlag = code;
            if(currentTimeSet.hoursFlag)
            {
              sei_read_code( pDecodedMessageOutputStream, 5, code, "hours_value"); currentTimeSet.hoursValue = code;
            }
          }
        }
      }
      sei_read_code( pDecodedMessageOutputStream, 5, code, "time_offset_length"); currentTimeSet.timeOffsetLength = code;
      if(currentTimeSet.timeOffsetLength > 0)
      {
        sei_read_code( pDecodedMessageOutputStream, currentTimeSet.timeOffsetLength, code, "time_offset_value");
        if((code & (1 << (currentTimeSet.timeOffsetLength-1))) == 0)
        {
          currentTimeSet.timeOffsetValue = code;
        }
        else
        {
          code &= (1<< (currentTimeSet.timeOffsetLength-1)) - 1;
          currentTimeSet.timeOffsetValue = ~code + 1;
        }
      }
    }
    sei.timeSetArray[i] = currentTimeSet;
  }
}

Void SEIReader::xParseSEIChromaSamplingFilterHint(SEIChromaSamplingFilterHint& sei, UInt payloadSize/*, TComSPS* sps*/, std::ostream *pDecodedMessageOutputStream)
{
  UInt uiCode;
  output_sei_message_header(sei, pDecodedMessageOutputStream, payloadSize);

  sei_read_code( pDecodedMessageOutputStream, 8, uiCode, "ver_chroma_filter_idc"); sei.m_verChromaFilterIdc = uiCode;
  sei_read_code( pDecodedMessageOutputStream, 8, uiCode, "hor_chroma_filter_idc"); sei.m_horChromaFilterIdc = uiCode;
  sei_read_flag( pDecodedMessageOutputStream, uiCode, "ver_filtering_process_flag"); sei.m_verFilteringProcessFlag = uiCode;
  if(sei.m_verChromaFilterIdc == 1 || sei.m_horChromaFilterIdc == 1)
  {
    sei_read_uvlc( pDecodedMessageOutputStream, uiCode, "target_format_idc"); sei.m_targetFormatIdc = uiCode;
    if(sei.m_verChromaFilterIdc == 1)
    {
      sei_read_uvlc( pDecodedMessageOutputStream, uiCode, "num_vertical_filters"); sei.m_numVerticalFilters = uiCode;
      if(sei.m_numVerticalFilters > 0)
      {
        sei.m_verTapLengthMinus1 = (Int*)malloc(sei.m_numVerticalFilters * sizeof(Int));
        sei.m_verFilterCoeff = (Int**)malloc(sei.m_numVerticalFilters * sizeof(Int*));
        for(Int i = 0; i < sei.m_numVerticalFilters; i ++)
        {
          sei_read_uvlc( pDecodedMessageOutputStream, uiCode, "ver_tap_length_minus_1"); sei.m_verTapLengthMinus1[i] = uiCode;
          sei.m_verFilterCoeff[i] = (Int*)malloc(sei.m_verTapLengthMinus1[i] * sizeof(Int));
          for(Int j = 0; j < sei.m_verTapLengthMinus1[i]; j ++)
          {
            sei_read_svlc( pDecodedMessageOutputStream, sei.m_verFilterCoeff[i][j], "ver_filter_coeff");
          }
        }
      }
    }
    if(sei.m_horChromaFilterIdc == 1)
    {
      sei_read_uvlc( pDecodedMessageOutputStream, uiCode, "num_horizontal_filters"); sei.m_numHorizontalFilters = uiCode;
      if(sei.m_numHorizontalFilters  > 0)
      {
        sei.m_horTapLengthMinus1 = (Int*)malloc(sei.m_numHorizontalFilters * sizeof(Int));
        sei.m_horFilterCoeff = (Int**)malloc(sei.m_numHorizontalFilters * sizeof(Int*));
        for(Int i = 0; i < sei.m_numHorizontalFilters; i ++)
        {
          sei_read_uvlc( pDecodedMessageOutputStream, uiCode, "hor_tap_length_minus_1"); sei.m_horTapLengthMinus1[i] = uiCode;
          sei.m_horFilterCoeff[i] = (Int*)malloc(sei.m_horTapLengthMinus1[i] * sizeof(Int));
          for(Int j = 0; j < sei.m_horTapLengthMinus1[i]; j ++)
          {
            sei_read_svlc( pDecodedMessageOutputStream, sei.m_horFilterCoeff[i][j], "hor_filter_coeff");
          }
        }
      }
    }
  }
}

Void SEIReader::xParseSEIKneeFunctionInfo(SEIKneeFunctionInfo& sei, UInt payloadSize, std::ostream *pDecodedMessageOutputStream)
{
  Int i;
  UInt val;
  output_sei_message_header(sei, pDecodedMessageOutputStream, payloadSize);

  sei_read_uvlc( pDecodedMessageOutputStream, val, "knee_function_id" );                   sei.m_kneeId = val;
  sei_read_flag( pDecodedMessageOutputStream, val, "knee_function_cancel_flag" );          sei.m_kneeCancelFlag = val;
  if ( !sei.m_kneeCancelFlag )
  {
    sei_read_flag( pDecodedMessageOutputStream, val, "knee_function_persistence_flag" );   sei.m_kneePersistenceFlag = val;
    sei_read_code( pDecodedMessageOutputStream, 32, val, "input_d_range" );                sei.m_kneeInputDrange = val;
    sei_read_code( pDecodedMessageOutputStream, 32, val, "input_disp_luminance" );         sei.m_kneeInputDispLuminance = val;
    sei_read_code( pDecodedMessageOutputStream, 32, val, "output_d_range" );               sei.m_kneeOutputDrange = val;
    sei_read_code( pDecodedMessageOutputStream, 32, val, "output_disp_luminance" );        sei.m_kneeOutputDispLuminance = val;
    sei_read_uvlc( pDecodedMessageOutputStream, val, "num_knee_points_minus1" );           sei.m_kneeNumKneePointsMinus1 = val;
    assert( sei.m_kneeNumKneePointsMinus1 > 0 );
    sei.m_kneeInputKneePoint.resize(sei.m_kneeNumKneePointsMinus1+1);
    sei.m_kneeOutputKneePoint.resize(sei.m_kneeNumKneePointsMinus1+1);
    for(i = 0; i <= sei.m_kneeNumKneePointsMinus1; i++ )
    {
      sei_read_code( pDecodedMessageOutputStream, 10, val, "input_knee_point" );           sei.m_kneeInputKneePoint[i] = val;
      sei_read_code( pDecodedMessageOutputStream, 10, val, "output_knee_point" );          sei.m_kneeOutputKneePoint[i] = val;
    }
  }
}

Void SEIReader::xParseSEIMasteringDisplayColourVolume(SEIMasteringDisplayColourVolume& sei, UInt payloadSize, std::ostream *pDecodedMessageOutputStream)
{
  UInt code;
  output_sei_message_header(sei, pDecodedMessageOutputStream, payloadSize);

  sei_read_code( pDecodedMessageOutputStream, 16, code, "display_primaries_x[0]" ); sei.values.primaries[0][0] = code;
  sei_read_code( pDecodedMessageOutputStream, 16, code, "display_primaries_y[0]" ); sei.values.primaries[0][1] = code;

  sei_read_code( pDecodedMessageOutputStream, 16, code, "display_primaries_x[1]" ); sei.values.primaries[1][0] = code;
  sei_read_code( pDecodedMessageOutputStream, 16, code, "display_primaries_y[1]" ); sei.values.primaries[1][1] = code;

  sei_read_code( pDecodedMessageOutputStream, 16, code, "display_primaries_x[2]" ); sei.values.primaries[2][0] = code;
  sei_read_code( pDecodedMessageOutputStream, 16, code, "display_primaries_y[2]" ); sei.values.primaries[2][1] = code;


  sei_read_code( pDecodedMessageOutputStream, 16, code, "white_point_x" ); sei.values.whitePoint[0] = code;
  sei_read_code( pDecodedMessageOutputStream, 16, code, "white_point_y" ); sei.values.whitePoint[1] = code;

  sei_read_code( pDecodedMessageOutputStream, 32, code, "max_display_mastering_luminance" ); sei.values.maxLuminance = code;
  sei_read_code( pDecodedMessageOutputStream, 32, code, "min_display_mastering_luminance" ); sei.values.minLuminance = code;
}

#if NH_MV_LAYERS_NOT_PRESENT_SEI
Void SEIReader::xParseSEILayersNotPresent(SEILayersNotPresent &sei, UInt payloadSize, const TComVPS *vps, std::ostream *pDecodedMessageOutputStream)
{
  UInt code;
  UInt i = 0;

  output_sei_message_header(sei, pDecodedMessageOutputStream, payloadSize);
  sei_read_code( pDecodedMessageOutputStream, 4, code, "lnp_sei_active_vps_id" ); sei.m_lnpSeiActiveVpsId = code;
  assert(vps->getVPSId() == sei.m_lnpSeiActiveVpsId);

  sei.m_lnpSeiMaxLayers = vps->getMaxLayersMinus1() + 1;
  sei.resizeDimI(sei.m_lnpSeiMaxLayers);
  for (; i < sei.m_lnpSeiMaxLayers; i++)
  {
    sei_read_flag( pDecodedMessageOutputStream, code, "layer_not_present_flag" ); 
    sei.m_layerNotPresentFlag[i] = (code == 1);
  }
};
#endif

Void SEIReader::xParseSEIInterLayerConstrainedTileSets(SEIInterLayerConstrainedTileSets& sei, UInt payloadSize, std::ostream *pDecodedMessageOutputStream)
{
  UInt code;
  output_sei_message_header(sei, pDecodedMessageOutputStream, payloadSize);

  sei_read_flag( pDecodedMessageOutputStream, code, "il_all_tiles_exact_sample_value_match_flag" ); sei.m_ilAllTilesExactSampleValueMatchFlag = (code == 1);
  sei_read_flag( pDecodedMessageOutputStream, code, "il_one_tile_per_tile_set_flag"              ); sei.m_ilOneTilePerTileSetFlag             = (code == 1);
  if( !sei.m_ilOneTilePerTileSetFlag )
  {
    sei_read_uvlc( pDecodedMessageOutputStream, code, "il_num_sets_in_message_minus1" ); sei.m_ilNumSetsInMessageMinus1 = code;
    if( sei.m_ilNumSetsInMessageMinus1 )
    {
      sei_read_flag( pDecodedMessageOutputStream, code, "skipped_tile_set_present_flag" ); sei.m_skippedTileSetPresentFlag = (code == 1);
    }
    Int numSignificantSets = sei.m_ilNumSetsInMessageMinus1 - sei.m_skippedTileSetPresentFlag + 1;
    
    sei.resizeDimI( numSignificantSets );
    for( Int i = 0; i < numSignificantSets; i++ )
    {
      sei_read_uvlc( pDecodedMessageOutputStream, code, "ilcts_id"                        ); sei.m_ilctsId                  [i] = code;
      sei_read_uvlc( pDecodedMessageOutputStream, code, "il_num_tile_rects_in_set_minus1" ); sei.m_ilNumTileRectsInSetMinus1[i] = code;
      
      sei.resizeDimJ( i, sei.m_ilNumTileRectsInSetMinus1[ i ] + 1 );
      for( Int j = 0; j  <=  sei.m_ilNumTileRectsInSetMinus1[ i ]; j++ )
      {
        sei_read_uvlc( pDecodedMessageOutputStream, code, "il_top_left_tile_index"     ); sei.m_ilTopLeftTileIndex    [i][j] = code;
        sei_read_uvlc( pDecodedMessageOutputStream, code, "il_bottom_right_tile_index" ); sei.m_ilBottomRightTileIndex[i][j] = code;
      }
      sei_read_code( pDecodedMessageOutputStream, 2, code, "ilc_idc" ); sei.m_ilcIdc[i] = code;
      if ( !sei.m_ilAllTilesExactSampleValueMatchFlag )
      {
        sei_read_flag( pDecodedMessageOutputStream, code, "il_exact_sample_value_match_flag" ); sei.m_ilExactSampleValueMatchFlag[i] = (code == 1);
      }
    }
  }
  else
  {
    sei_read_code( pDecodedMessageOutputStream, 2, code, "all_tiles_ilc_idc" ); sei.m_allTilesIlcIdc = code;
  }
};

#if NH_MV_SEI_TBD
Void SEIReader::xParseSEIBspNesting(SEIBspNesting& sei, UInt payloadSize, std::ostream *pDecodedMessageOutputStream)
{
  UInt code;
  output_sei_message_header(sei, pDecodedMessageOutputStream, payloadSize);

  sei_read_uvlc( pDecodedMessageOutputStream, code, "sei_ols_idx" ); sei.m_seiOlsIdx = code;
  sei_read_uvlc( pDecodedMessageOutputStream, code, "sei_partitioning_scheme_idx" ); sei.m_seiPartitioningSchemeIdx = code;
  sei_read_uvlc( pDecodedMessageOutputStream, code, "bsp_idx" ); sei.m_bspIdx = code;
  while( !ByteaLigned(() ) );
  {
    sei_read_code( pDecodedMessageOutputStream, *equalto0*/u1, code, "bsp_nesting_zero_bit" ); sei.m_bspNestingZeroBit = code;
  }
  sei_read_uvlc( pDecodedMessageOutputStream, code, "num_seis_in_bsp_minus1" ); sei.m_numSeisInBspMinus1 = code;
  for( Int i = 0; i  <=  NumSeisInBspMinus1( ); i++ )
  {
    SeiMessage(() );
  }
};

Void SEIReader::xParseSEIBspInitialArrivalTime(SEIBspInitialArrivalTime& sei, UInt payloadSize, std::ostream *pDecodedMessageOutputStream)
{
  UInt code;
  output_sei_message_header(sei, pDecodedMessageOutputStream, payloadSize);

  psIdx = SeiPartitioningSchemeIdx();
  if( nalInitialArrivalDelayPresent )
  {
    for( Int i = 0; i < BspSchedCnt( SeiOlsIdx(), psIdx, MaxTemporalId( 0 ) ); i++ )
    {
      sei_read_code( pDecodedMessageOutputStream, getNalInitialArrivalDelayLen ), code, "nal_initial_arrival_delay" ); sei.m_nalInitialArrivalDelay[i] = code;
    }
  }
  if( vclInitialArrivalDelayPresent )
  {
    for( Int i = 0; i < BspSchedCnt( SeiOlsIdx(), psIdx, MaxTemporalId( 0 ) ); i++ )
    {
      sei_read_code( pDecodedMessageOutputStream, getVclInitialArrivalDelayLen ), code, "vcl_initial_arrival_delay" ); sei.m_vclInitialArrivalDelay[i] = code;
    }
  }
};
#endif

Void SEIReader::xParseSEISubBitstreamProperty(SEISubBitstreamProperty& sei, UInt payloadSize, std::ostream *pDecodedMessageOutputStream)
{
  UInt code;
  output_sei_message_header(sei, pDecodedMessageOutputStream, payloadSize);

  sei_read_code( pDecodedMessageOutputStream, 4, code, "sb_property_active_vps_id" ); sei.m_sbPropertyActiveVpsId = code;
  sei_read_uvlc( pDecodedMessageOutputStream, code, "num_additional_sub_streams_minus1" ); sei.m_numAdditionalSubStreamsMinus1 = code;
  sei.resizeArrays( ); 
  for( Int i = 0; i  <=  sei.m_numAdditionalSubStreamsMinus1; i++ )
  {
    sei_read_code( pDecodedMessageOutputStream, 2, code, "sub_bitstream_mode" ); sei.m_subBitstreamMode[i] = code;
    sei_read_uvlc( pDecodedMessageOutputStream, code, "ols_idx_to_vps" ); sei.m_olsIdxToVps[i] = code;
    sei_read_code( pDecodedMessageOutputStream, 3, code, "highest_sublayer_id" ); sei.m_highestSublayerId[i] = code;
    sei_read_code( pDecodedMessageOutputStream, 16, code, "avg_sb_property_bit_rate" ); sei.m_avgSbPropertyBitRate[i] = code;
    sei_read_code( pDecodedMessageOutputStream, 16, code, "max_sb_property_bit_rate" ); sei.m_maxSbPropertyBitRate[i] = code;
  }
};

#if NH_MV_SEI_TBD
Void SEIReader::xParseSEIAlphaChannelInfo(SEIAlphaChannelInfo& sei, UInt payloadSize, std::ostream *pDecodedMessageOutputStream)
{
  UInt code;
  output_sei_message_header(sei, pDecodedMessageOutputStream, payloadSize);

  sei_read_flag( pDecodedMessageOutputStream, code, "alpha_channel_cancel_flag" ); sei.m_alphaChannelCancelFlag = (code == 1);
  if( !sei.m_alphaChannelCancelFlag )
  {
    sei_read_code( pDecodedMessageOutputStream, 3, code, "alpha_channel_use_idc" ); sei.m_alphaChannelUseIdc = code;
    sei_read_code( pDecodedMessageOutputStream, 3, code, "alpha_channel_bit_depth_minus8" ); sei.m_alphaChannelBitDepthMinus8 = code;
    sei_read_code( pDecodedMessageOutputStream, getAlphaTransparentValueLen ), code, "alpha_transparent_value" ); sei.m_alphaTransparentValue = code;
    sei_read_code( pDecodedMessageOutputStream, getAlphaOpaqueValueLen ), code, "alpha_opaque_value" ); sei.m_alphaOpaqueValue = code;
    sei_read_flag( pDecodedMessageOutputStream, code, "alpha_channel_incr_flag" ); sei.m_alphaChannelIncrFlag = (code == 1);
    sei_read_flag( pDecodedMessageOutputStream, code, "alpha_channel_clip_flag" ); sei.m_alphaChannelClipFlag = (code == 1);
    if( sei.m_alphaChannelClipFlag )
    {
      sei_read_flag( pDecodedMessageOutputStream, code, "alpha_channel_clip_type_flag" ); sei.m_alphaChannelClipTypeFlag = (code == 1);
    }
  }
};

Void SEIReader::xParseSEIOverlayInfo(SEIOverlayInfo& sei, UInt payloadSize, std::ostream *pDecodedMessageOutputStream)
{
  UInt code;
  output_sei_message_header(sei, pDecodedMessageOutputStream, payloadSize);

  sei_read_flag( pDecodedMessageOutputStream, code, "overlay_info_cancel_flag" ); sei.m_overlayInfoCancelFlag = (code == 1);
  if( !sei.m_overlayInfoCancelFlag )
  {
    sei_read_uvlc( pDecodedMessageOutputStream, code, "overlay_content_aux_id_minus128" ); sei.m_overlayContentAuxIdMinus128 = code;
    sei_read_uvlc( pDecodedMessageOutputStream, code, "overlay_label_aux_id_minus128" ); sei.m_overlayLabelAuxIdMinus128 = code;
    sei_read_uvlc( pDecodedMessageOutputStream, code, "overlay_alpha_aux_id_minus128" ); sei.m_overlayAlphaAuxIdMinus128 = code;
    sei_read_uvlc( pDecodedMessageOutputStream, code, "overlay_element_label_value_length_minus8" ); sei.m_overlayElementLabelValueLengthMinus8 = code;
    sei_read_uvlc( pDecodedMessageOutputStream, code, "num_overlays_minus1" ); sei.m_numOverlaysMinus1 = code;
    for( Int i = 0; i  <=  NumOverlaysMinus1( ); i++ )
    {
      sei_read_uvlc( pDecodedMessageOutputStream, code, "overlay_idx" ); sei.m_overlayIdx[i] = code;
      sei_read_flag( pDecodedMessageOutputStream, code, "language_overlay_present_flag" ); sei.m_languageOverlayPresentFlag[i] = (code == 1);
      sei_read_code( pDecodedMessageOutputStream, 6, code, "overlay_content_layer_id" ); sei.m_overlayContentLayerId[i] = code;
      sei_read_flag( pDecodedMessageOutputStream, code, "overlay_label_present_flag" ); sei.m_overlayLabelPresentFlag[i] = (code == 1);
      if( sei.m_overlayLabelPresentFlag( i ) )
      {
        sei_read_code( pDecodedMessageOutputStream, 6, code, "overlay_label_layer_id" ); sei.m_overlayLabelLayerId[i] = code;
      }
      sei_read_flag( pDecodedMessageOutputStream, code, "overlay_alpha_present_flag" ); sei.m_overlayAlphaPresentFlag[i] = (code == 1);
      if( sei.m_overlayAlphaPresentFlag( i ) )
      {
        sei_read_code( pDecodedMessageOutputStream, 6, code, "overlay_alpha_layer_id" ); sei.m_overlayAlphaLayerId[i] = code;
      }
      if( sei.m_overlayLabelPresentFlag( i ) )
      {
        sei_read_uvlc( pDecodedMessageOutputStream, code, "num_overlay_elements_minus1" ); sei.m_numOverlayElementsMinus1[i] = code;
        for( Int j = 0; j  <=  sei.m_numOverlayElementsMinus1( i ); j++ )
        {
          sei_read_code( pDecodedMessageOutputStream, getOverlayElementLabelMinLen ), code, "overlay_element_label_min" ); sei.m_overlayElementLabelMin[i][j] = code;
          sei_read_code( pDecodedMessageOutputStream, getOverlayElementLabelMaxLen ), code, "overlay_element_label_max" ); sei.m_overlayElementLabelMax[i][j] = code;
        }
      }
    }
    while( !ByteaLigned(() ) );
    {
      sei_read_code( pDecodedMessageOutputStream, *equalto0*/f1, code, "overlay_zero_bit" ); sei.m_overlayZeroBit = code;
    }
    for( Int i = 0; i  <=  NumOverlaysMinus1( ); i++ )
    {
      if( sei.m_languageOverlayPresentFlag( i ) )
      {
        sei_read_code( pDecodedMessageOutputStream, tv, code, "overlay_language" ); sei.m_overlayLanguage[i] = code;
      }
      sei_read_code( pDecodedMessageOutputStream, tv, code, "overlay_name" ); sei.m_overlayName[i] = code;
      if( sei.m_overlayLabelPresentFlag( i ) )
      {
        for( Int j = 0; j  <=  sei.m_numOverlayElementsMinus1( i ); j++ )
        {
          sei_read_code( pDecodedMessageOutputStream, tv, code, "overlay_element_name" ); sei.m_overlayElementName[i][j] = code;
        }
      }
    }
    sei_read_flag( pDecodedMessageOutputStream, code, "overlay_info_persistence_flag" ); sei.m_overlayInfoPersistenceFlag = (code == 1);
  }
};
#endif

Void SEIReader::xParseSEITemporalMvPredictionConstraints(SEITemporalMvPredictionConstraints& sei, UInt payloadSize, std::ostream *pDecodedMessageOutputStream)
{
  UInt code;
  output_sei_message_header(sei, pDecodedMessageOutputStream, payloadSize);

  sei_read_flag( pDecodedMessageOutputStream, code, "prev_pics_not_used_flag"     ); sei.m_prevPicsNotUsedFlag    = (code == 1);
  sei_read_flag( pDecodedMessageOutputStream, code, "no_intra_layer_col_pic_flag" ); sei.m_noIntraLayerColPicFlag = (code == 1);
};

#if NH_MV_SEI_TBD
Void SEIReader::xParseSEIFrameFieldInfo(SEIFrameFieldInfo& sei, UInt payloadSize, std::ostream *pDecodedMessageOutputStream)
{
  UInt code;
  output_sei_message_header(sei, pDecodedMessageOutputStream, payloadSize);

  sei_read_code( pDecodedMessageOutputStream, 4, code, "ffinfo_pic_struct" ); sei.m_ffinfoPicStruct = code;
  sei_read_code( pDecodedMessageOutputStream, 2, code, "ffinfo_source_scan_type" ); sei.m_ffinfoSourceScanType = code;
  sei_read_flag( pDecodedMessageOutputStream, code, "ffinfo_duplicate_flag" ); sei.m_ffinfoDuplicateFlag = (code == 1);
};

Void SEIReader::xParseSEIThreeDimensionalReferenceDisplaysInfo(SEIThreeDimensionalReferenceDisplaysInfo& sei, UInt payloadSize, std::ostream *pDecodedMessageOutputStream)
{
  UInt code;
  output_sei_message_header(sei, pDecodedMessageOutputStream, payloadSize);

  sei_read_uvlc( pDecodedMessageOutputStream, code, "prec_ref_display_width" ); sei.m_precRefDisplayWidth = code;
  sei_read_flag( pDecodedMessageOutputStream, code, "ref_viewing_distance_flag" ); sei.m_refViewingDistanceFlag = (code == 1);
  if( sei.m_refViewingDistanceFlag )
  {
    sei_read_uvlc( pDecodedMessageOutputStream, code, "prec_ref_viewing_dist" ); sei.m_precRefViewingDist = code;
  }
  sei_read_uvlc( pDecodedMessageOutputStream, code, "num_ref_displays_minus1" ); sei.m_numRefDisplaysMinus1 = code;
  for( Int i = 0; i  <=  NumRefDisplaysMinus1( ); i++ )
  {
    sei_read_uvlc( pDecodedMessageOutputStream, code, "left_view_id" ); sei.m_leftViewId[i] = code;
    sei_read_uvlc( pDecodedMessageOutputStream, code, "right_view_id" ); sei.m_rightViewId[i] = code;
    sei_read_code( pDecodedMessageOutputStream, 6, code, "exponent_ref_display_width" ); sei.m_exponentRefDisplayWidth[i] = code;
    sei_read_code( pDecodedMessageOutputStream, getMantissaRefDisplayWidthLen ), code, "mantissa_ref_display_width" ); sei.m_mantissaRefDisplayWidth[i] = code;
    if( sei.m_refViewingDistanceFlag )
    {
      sei_read_code( pDecodedMessageOutputStream, 6, code, "exponent_ref_viewing_distance" ); sei.m_exponentRefViewingDistance[i] = code;
      sei_read_code( pDecodedMessageOutputStream, getMantissaRefViewingDistanceLen ), code, "mantissa_ref_viewing_distance" ); sei.m_mantissaRefViewingDistance[i] = code;
    }
    sei_read_flag( pDecodedMessageOutputStream, code, "additional_shift_present_flag" ); sei.m_additionalShiftPresentFlag[i] = (code == 1);
    if( sei.m_additionalShiftPresentFlag( i ) )
    {
      sei_read_code( pDecodedMessageOutputStream, 10, code, "num_sample_shift_plus512" ); sei.m_numSampleShiftPlus512[i] = code;
    }
  }
  sei_read_flag( pDecodedMessageOutputStream, code, "three_dimensional_reference_displays_extension_flag" ); sei.m_threeDimensionalReferenceDisplaysExtensionFlag = (code == 1);
};

Void SEIReader::xParseSEIDepthRepresentationInfo(SEIDepthRepresentationInfo& sei, UInt payloadSize, std::ostream *pDecodedMessageOutputStream)
{
  UInt code;
  output_sei_message_header(sei, pDecodedMessageOutputStream, payloadSize);

  sei_read_flag( pDecodedMessageOutputStream, code, "z_near_flag" ); sei.m_zNearFlag = (code == 1);
  sei_read_flag( pDecodedMessageOutputStream, code, "z_far_flag" ); sei.m_zFarFlag = (code == 1);
  sei_read_flag( pDecodedMessageOutputStream, code, "d_min_flag" ); sei.m_dMinFlag = (code == 1);
  sei_read_flag( pDecodedMessageOutputStream, code, "d_max_flag" ); sei.m_dMaxFlag = (code == 1);
  sei_read_uvlc( pDecodedMessageOutputStream, code, "depth_representation_type" ); sei.m_depthRepresentationType = code;
  if( sei.m_dMinFlag  | |  sei.m_dMaxFlag )
  {
    sei_read_uvlc( pDecodedMessageOutputStream, code, "disparity_ref_view_id" ); sei.m_disparityRefViewId = code;
  }
  if( sei.m_zNearFlag )
  {
    DepthRepInfoElement(() ZNearSign, ZNearExp, ZNearMantissa, ZNearManLen );
  }
  if( sei.m_zFarFlag )
  {
    DepthRepInfoElement(() ZFarSign, ZFarExp, ZFarMantissa, ZFarManLen );
  }
  if( sei.m_dMinFlag )
  {
    DepthRepInfoElement(() DMinSign, DMinExp, DMinMantissa, DMinManLen );
  }
  if( sei.m_dMaxFlag )
  {
    DepthRepInfoElement(() DMaxSign, DMaxExp, DMaxMantissa, DMaxManLen );
  }
  if( sei.m_depthRepresentationType  ==  3 )
  {
    sei_read_uvlc( pDecodedMessageOutputStream, code, "depth_nonlinear_representation_num_minus1" ); sei.m_depthNonlinearRepresentationNumMinus1 = code;
    for( Int i = 1; i  <=  sei.m_depthNonlinearRepresentationNumMinus1 + 1; i++ )
    {
      DepthNonlinearRepresentationModel( i );
    }
  }
};

Void SEIReader::xParseSEIDepthRepInfoElement(SEIDepthRepInfoElement& sei, UInt payloadSize, std::ostream *pDecodedMessageOutputStream)
{
  UInt code;
  output_sei_message_header(sei, pDecodedMessageOutputStream, payloadSize);

  sei_read_flag( pDecodedMessageOutputStream, code, "da_sign_flag" ); sei.m_daSignFlag = (code == 1);
  sei_read_code( pDecodedMessageOutputStream, 7, code, "da_exponent" ); sei.m_daExponent = code;
  sei_read_code( pDecodedMessageOutputStream, 5, code, "da_mantissa_len_minus1" ); sei.m_daMantissaLenMinus1 = code;
  sei_read_code( pDecodedMessageOutputStream, getDaMantissaLen ), code, "da_mantissa" ); sei.m_daMantissa = code;
};
#endif
Void SEIReader::xParseSEIMultiviewSceneInfo(SEIMultiviewSceneInfo& sei, UInt payloadSize, std::ostream *pDecodedMessageOutputStream)
{
  UInt  code;
  Int  sCode; 
  output_sei_message_header(sei, pDecodedMessageOutputStream, payloadSize);

  sei_read_svlc( pDecodedMessageOutputStream, sCode, "min_disparity" )      ; sei.m_minDisparity      = sCode;
  sei_read_uvlc( pDecodedMessageOutputStream, code , "max_disparity_range" ); sei.m_maxDisparityRange = code;
};

Void SEIReader::xParseSEIMultiviewAcquisitionInfo(SEIMultiviewAcquisitionInfo& sei, UInt payloadSize, std::ostream *pDecodedMessageOutputStream)
{
  UInt code;
  output_sei_message_header(sei, pDecodedMessageOutputStream, payloadSize);

  sei.resizeArrays( );
  sei_read_flag( pDecodedMessageOutputStream, code, "intrinsic_param_flag" ); sei.m_intrinsicParamFlag = (code == 1);
  sei_read_flag( pDecodedMessageOutputStream, code, "extrinsic_param_flag" ); sei.m_extrinsicParamFlag = (code == 1);
  if( sei.m_intrinsicParamFlag )
  {
    sei_read_flag( pDecodedMessageOutputStream, code, "intrinsic_params_equal_flag" ); sei.m_intrinsicParamsEqualFlag = (code == 1);
    sei_read_uvlc( pDecodedMessageOutputStream, code, "prec_focal_length"           ); sei.m_precFocalLength          =  code      ;
    sei_read_uvlc( pDecodedMessageOutputStream, code, "prec_principal_point"        ); sei.m_precPrincipalPoint       =  code      ;
    sei_read_uvlc( pDecodedMessageOutputStream, code, "prec_skew_factor"            ); sei.m_precSkewFactor           =  code      ;

    for( Int i = 0; i  <=  ( sei.m_intrinsicParamsEqualFlag ? 0 : sei.getNumViewsMinus1() ); i++ )
    {      
      sei_read_flag( pDecodedMessageOutputStream,                                         code, "sign_focal_length_x"        ); sei.m_signFocalLengthX       [i] = (code == 1);
      sei_read_code( pDecodedMessageOutputStream, 6,                                      code, "exponent_focal_length_x"    ); sei.m_exponentFocalLengthX   [i] =  code      ;
      sei_read_code( pDecodedMessageOutputStream, sei.getMantissaFocalLengthXLen   ( i ), code, "mantissa_focal_length_x"    ); sei.m_mantissaFocalLengthX   [i] =  code      ;
      sei_read_flag( pDecodedMessageOutputStream,                                         code, "sign_focal_length_y"        ); sei.m_signFocalLengthY       [i] = (code == 1);
      sei_read_code( pDecodedMessageOutputStream, 6,                                      code, "exponent_focal_length_y"    ); sei.m_exponentFocalLengthY   [i] =  code      ;
      sei_read_code( pDecodedMessageOutputStream, sei.getMantissaFocalLengthYLen   ( i ), code, "mantissa_focal_length_y"    ); sei.m_mantissaFocalLengthY   [i] =  code      ;
      sei_read_flag( pDecodedMessageOutputStream,                                         code, "sign_principal_point_x"     ); sei.m_signPrincipalPointX    [i] = (code == 1);
      sei_read_code( pDecodedMessageOutputStream, 6,                                      code, "exponent_principal_point_x" ); sei.m_exponentPrincipalPointX[i] =  code      ;
      sei_read_code( pDecodedMessageOutputStream, sei.getMantissaPrincipalPointXLen( i ), code, "mantissa_principal_point_x" ); sei.m_mantissaPrincipalPointX[i] =  code      ;
      sei_read_flag( pDecodedMessageOutputStream,                                         code, "sign_principal_point_y"     ); sei.m_signPrincipalPointY    [i] = (code == 1);
      sei_read_code( pDecodedMessageOutputStream, 6,                                      code, "exponent_principal_point_y" ); sei.m_exponentPrincipalPointY[i] =  code      ;
      sei_read_code( pDecodedMessageOutputStream, sei.getMantissaPrincipalPointYLen( i ), code, "mantissa_principal_point_y" ); sei.m_mantissaPrincipalPointY[i] =  code      ;
      sei_read_flag( pDecodedMessageOutputStream,                                         code, "sign_skew_factor"           ); sei.m_signSkewFactor         [i] = (code == 1);
      sei_read_code( pDecodedMessageOutputStream, 6,                                      code, "exponent_skew_factor"       ); sei.m_exponentSkewFactor     [i] =  code      ;
      sei_read_code( pDecodedMessageOutputStream, sei.getMantissaSkewFactorLen     ( i ), code, "mantissa_skew_factor"       ); sei.m_mantissaSkewFactor     [i] =  code      ;
    }
  }
  if( sei.m_extrinsicParamFlag )
  {
    sei_read_uvlc( pDecodedMessageOutputStream, code, "prec_rotation_param"    ); sei.m_precRotationParam    = code;
    sei_read_uvlc( pDecodedMessageOutputStream, code, "prec_translation_param" ); sei.m_precTranslationParam = code;

    for( Int i = 0; i  <=  sei.getNumViewsMinus1(); i++ )
    {
      for( Int j = 0; j  <=  2; j++ )  /* row */
      {
        for( Int k = 0; k  <=  2; k++ )  /* column */
        {
          sei_read_flag( pDecodedMessageOutputStream,                                 code, "sign_r"     ); sei.m_signR    [i][j][k] = (code == 1);
          sei_read_code( pDecodedMessageOutputStream, 6,                              code, "exponent_r" ); sei.m_exponentR[i][j][k] =  code      ;
          sei_read_code( pDecodedMessageOutputStream, sei.getMantissaRLen( i, j, k ), code, "mantissa_r" ); sei.m_mantissaR[i][j][k] =  code      ;
        }
        sei_read_flag( pDecodedMessageOutputStream,                              code, "sign_t"     ); sei.m_signT    [i][j] = (code == 1);
        sei_read_code( pDecodedMessageOutputStream, 6,                           code, "exponent_t" ); sei.m_exponentT[i][j] =  code      ;
        sei_read_code( pDecodedMessageOutputStream, sei.getMantissaTLen( i, j ), code, "mantissa_t" ); sei.m_mantissaT[i][j] =  code      ;
      }
    }
  }
};



Void SEIReader::xParseSEIMultiviewViewPosition(SEIMultiviewViewPosition& sei, UInt payloadSize, std::ostream *pDecodedMessageOutputStream)
{
  UInt code;
  output_sei_message_header(sei, pDecodedMessageOutputStream, payloadSize);

  sei_read_uvlc( pDecodedMessageOutputStream, code, "num_views_minus1" ); sei.m_numViewsMinus1 = code;
  sei.m_viewPosition.resize( sei.m_numViewsMinus1 + 1 ); 
  for( Int i = 0; i  <=  sei.m_numViewsMinus1; i++ )
  {
    sei_read_uvlc( pDecodedMessageOutputStream, code, "view_position" ); sei.m_viewPosition[i] = code;
  }
};

#if NH_MV_SEI_TBD
Void SEIReader::xParseSEIAlternativeDepthInfo(SEIAlternativeDepthInfo& sei, UInt payloadSize, std::ostream *pDecodedMessageOutputStream)
{
  UInt code;
  output_sei_message_header(sei, pDecodedMessageOutputStream, payloadSize);

  sei_read_flag( pDecodedMessageOutputStream, code, "alternative_depth_info_cancel_flag" ); sei.m_alternativeDepthInfoCancelFlag = (code == 1);
  if( sei.m_alternativeDepthInfoCancelFlag  ==  0 )
  {
    sei_read_code( pDecodedMessageOutputStream, 2, code, "depth_type" ); sei.m_depthType = code;
    if( sei.m_depthType  ==  0 )
    {
      sei_read_uvlc( pDecodedMessageOutputStream, code, "num_constituent_views_gvd_minus1" ); sei.m_numConstituentViewsGvdMinus1 = code;
      sei_read_flag( pDecodedMessageOutputStream, code, "depth_present_gvd_flag" ); sei.m_depthPresentGvdFlag = (code == 1);
      sei_read_flag( pDecodedMessageOutputStream, code, "z_gvd_flag" ); sei.m_zGvdFlag = (code == 1);
      sei_read_flag( pDecodedMessageOutputStream, code, "intrinsic_param_gvd_flag" ); sei.m_intrinsicParamGvdFlag = (code == 1);
      sei_read_flag( pDecodedMessageOutputStream, code, "rotation_gvd_flag" ); sei.m_rotationGvdFlag = (code == 1);
      sei_read_flag( pDecodedMessageOutputStream, code, "translation_gvd_flag" ); sei.m_translationGvdFlag = (code == 1);
      if( sei.m_zGvdFlag )
      {
        for( Int i = 0; i  <=  sei.m_numConstituentViewsGvdMinus1 + 1; i++ )
        {
          sei_read_flag( pDecodedMessageOutputStream, code, "sign_gvd_z_near_flag" ); sei.m_signGvdZNearFlag[i] = (code == 1);
          sei_read_code( pDecodedMessageOutputStream, 7, code, "exp_gvd_z_near" ); sei.m_expGvdZNear[i] = code;
          sei_read_code( pDecodedMessageOutputStream, 5, code, "man_len_gvd_z_near_minus1" ); sei.m_manLenGvdZNearMinus1[i] = code;
          sei_read_code( pDecodedMessageOutputStream, getManGvdZNearLen ), code, "man_gvd_z_near" ); sei.m_manGvdZNear[i] = code;
          sei_read_flag( pDecodedMessageOutputStream, code, "sign_gvd_z_far_flag" ); sei.m_signGvdZFarFlag[i] = (code == 1);
          sei_read_code( pDecodedMessageOutputStream, 7, code, "exp_gvd_z_far" ); sei.m_expGvdZFar[i] = code;
          sei_read_code( pDecodedMessageOutputStream, 5, code, "man_len_gvd_z_far_minus1" ); sei.m_manLenGvdZFarMinus1[i] = code;
          sei_read_code( pDecodedMessageOutputStream, getManGvdZFarLen ), code, "man_gvd_z_far" ); sei.m_manGvdZFar[i] = code;
        }
      }
      if( sei.m_intrinsicParamGvdFlag )
      {
        sei_read_uvlc( pDecodedMessageOutputStream, code, "prec_gvd_focal_length" ); sei.m_precGvdFocalLength = code;
        sei_read_uvlc( pDecodedMessageOutputStream, code, "prec_gvd_principal_point" ); sei.m_precGvdPrincipalPoint = code;
      }
      if( sei.m_rotationGvdFlag )
      {
        sei_read_uvlc( pDecodedMessageOutputStream, code, "prec_gvd_rotation_param" ); sei.m_precGvdRotationParam = code;
      }
      if( sei.m_translationGvdFlag )
      {
        sei_read_uvlc( pDecodedMessageOutputStream, code, "prec_gvd_translation_param" ); sei.m_precGvdTranslationParam = code;
      }
      for( Int i = 0; i  <=  sei.m_numConstituentViewsGvdMinus1 + 1; i++ )
      {
        if( sei.m_intrinsicParamGvdFlag )
        {
          sei_read_flag( pDecodedMessageOutputStream, code, "sign_gvd_focal_length_x" ); sei.m_signGvdFocalLengthX[i] = (code == 1);
          sei_read_code( pDecodedMessageOutputStream, 6, code, "exp_gvd_focal_length_x" ); sei.m_expGvdFocalLengthX[i] = code;
          sei_read_code( pDecodedMessageOutputStream, getManGvdFocalLengthXLen ), code, "man_gvd_focal_length_x" ); sei.m_manGvdFocalLengthX[i] = code;
          sei_read_flag( pDecodedMessageOutputStream, code, "sign_gvd_focal_length_y" ); sei.m_signGvdFocalLengthY[i] = (code == 1);
          sei_read_code( pDecodedMessageOutputStream, 6, code, "exp_gvd_focal_length_y" ); sei.m_expGvdFocalLengthY[i] = code;
          sei_read_code( pDecodedMessageOutputStream, getManGvdFocalLengthYLen ), code, "man_gvd_focal_length_y" ); sei.m_manGvdFocalLengthY[i] = code;
          sei_read_flag( pDecodedMessageOutputStream, code, "sign_gvd_principal_point_x" ); sei.m_signGvdPrincipalPointX[i] = (code == 1);
          sei_read_code( pDecodedMessageOutputStream, 6, code, "exp_gvd_principal_point_x" ); sei.m_expGvdPrincipalPointX[i] = code;
          sei_read_code( pDecodedMessageOutputStream, getManGvdPrincipalPointXLen ), code, "man_gvd_principal_point_x" ); sei.m_manGvdPrincipalPointX[i] = code;
          sei_read_flag( pDecodedMessageOutputStream, code, "sign_gvd_principal_point_y" ); sei.m_signGvdPrincipalPointY[i] = (code == 1);
          sei_read_code( pDecodedMessageOutputStream, 6, code, "exp_gvd_principal_point_y" ); sei.m_expGvdPrincipalPointY[i] = code;
          sei_read_code( pDecodedMessageOutputStream, getManGvdPrincipalPointYLen ), code, "man_gvd_principal_point_y" ); sei.m_manGvdPrincipalPointY[i] = code;
        }
        if( sei.m_rotationGvdFlag )
        {
          for( Int j = 10; j  <=  3; j++ ) /* row */
          {
            for( Int k = 10; k  <=  3; k++ )  /* column */
            {
              sei_read_flag( pDecodedMessageOutputStream, code, "sign_gvd_r" ); sei.m_signGvdR[i][j][k] = (code == 1);
              sei_read_code( pDecodedMessageOutputStream, 6, code, "exp_gvd_r" ); sei.m_expGvdR[i][j][k] = code;
              sei_read_code( pDecodedMessageOutputStream, getManGvdRLen ), code, "man_gvd_r" ); sei.m_manGvdR[i][j][k] = code;
            }
          }
        }
        if( sei.m_translationGvdFlag )
        {
          sei_read_flag( pDecodedMessageOutputStream, code, "sign_gvd_t_x" ); sei.m_signGvdTX[i] = (code == 1);
          sei_read_code( pDecodedMessageOutputStream, 6, code, "exp_gvd_t_x" ); sei.m_expGvdTX[i] = code;
          sei_read_code( pDecodedMessageOutputStream, getManGvdTXLen ), code, "man_gvd_t_x" ); sei.m_manGvdTX[i] = code;
        }
      }
    }
    if( sei.m_depthType  ==  1 )
    {
      sei_read_svlc( pDecodedMessageOutputStream, code, "min_offset_x_int" ); sei.m_minOffsetXInt = code;
      sei_read_code( pDecodedMessageOutputStream, 8, code, "min_offset_x_frac" ); sei.m_minOffsetXFrac = code;
      sei_read_svlc( pDecodedMessageOutputStream, code, "max_offset_x_int" ); sei.m_maxOffsetXInt = code;
      sei_read_code( pDecodedMessageOutputStream, 8, code, "max_offset_x_frac" ); sei.m_maxOffsetXFrac = code;
      sei_read_flag( pDecodedMessageOutputStream, code, "offset_y_present_flag" ); sei.m_offsetYPresentFlag = (code == 1);
      if( sei.m_offsetYPresentFlag )
      {
        sei_read_svlc( pDecodedMessageOutputStream, code, "min_offset_y_int" ); sei.m_minOffsetYInt = code;
        sei_read_code( pDecodedMessageOutputStream, 8, code, "min_offset_y_frac" ); sei.m_minOffsetYFrac = code;
        sei_read_svlc( pDecodedMessageOutputStream, code, "max_offset_y_int" ); sei.m_maxOffsetYInt = code;
        sei_read_code( pDecodedMessageOutputStream, 8, code, "max_offset_y_frac" ); sei.m_maxOffsetYFrac = code;
      }
      sei_read_flag( pDecodedMessageOutputStream, code, "warp_map_size_present_flag" ); sei.m_warpMapSizePresentFlag = (code == 1);
      if( sei.m_warpMapSizePresentFlag )
      {
        sei_read_uvlc( pDecodedMessageOutputStream, code, "warp_map_width_minus2" ); sei.m_warpMapWidthMinus2 = code;
        sei_read_uvlc( pDecodedMessageOutputStream, code, "warp_map_height_minus2" ); sei.m_warpMapHeightMinus2 = code;
      }
    }
  }
};
#endif

//! \}
