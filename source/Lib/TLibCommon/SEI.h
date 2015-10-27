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

#ifndef __SEI__
#define __SEI__

#pragma once
#include <list>
#include <vector>
#include <cstring>

#include "CommonDef.h"
#include "libmd5/MD5.h"


#if NH_MV_SEI
#include "TAppCommon/program_options_lite.h"
using namespace std;
namespace po = df::program_options_lite;
#endif

//! \ingroup TLibCommon
//! \{
class TComSPS;
#if NH_MV_SEI
class TComSlice;
class SEIScalableNesting;
#endif

/**
 * Abstract class representing an SEI message with lightweight RTTI.
 */
class SEI
{
public:
  enum PayloadType
  {
    BUFFERING_PERIOD                     = 0,
    PICTURE_TIMING                       = 1,
    PAN_SCAN_RECT                        = 2,
    FILLER_PAYLOAD                       = 3,
    USER_DATA_REGISTERED_ITU_T_T35       = 4,
    USER_DATA_UNREGISTERED               = 5,
    RECOVERY_POINT                       = 6,
    SCENE_INFO                           = 9,
    FULL_FRAME_SNAPSHOT                  = 15,
    PROGRESSIVE_REFINEMENT_SEGMENT_START = 16,
    PROGRESSIVE_REFINEMENT_SEGMENT_END   = 17,
    FILM_GRAIN_CHARACTERISTICS           = 19,
    POST_FILTER_HINT                     = 22,
    TONE_MAPPING_INFO                    = 23,
    FRAME_PACKING                        = 45,
    DISPLAY_ORIENTATION                  = 47,
    SOP_DESCRIPTION                      = 128,
    ACTIVE_PARAMETER_SETS                = 129,
    DECODING_UNIT_INFO                   = 130,
    TEMPORAL_LEVEL0_INDEX                = 131,
    DECODED_PICTURE_HASH                 = 132,
    SCALABLE_NESTING                     = 133,
    REGION_REFRESH_INFO                  = 134,
    NO_DISPLAY                           = 135,
    TIME_CODE                            = 136,
    MASTERING_DISPLAY_COLOUR_VOLUME      = 137,
    SEGM_RECT_FRAME_PACKING              = 138,
    TEMP_MOTION_CONSTRAINED_TILE_SETS    = 139,
    CHROMA_SAMPLING_FILTER_HINT          = 140,
    KNEE_FUNCTION_INFO                   = 141
#if NH_MV_SEI
    ,COLOUR_REMAPPING_INFO                    = 142,
    DEINTERLACED_FIELD_IDENTIFICATION         = 143,
    LAYERS_NOT_PRESENT                        = 160,
    INTER_LAYER_CONSTRAINED_TILE_SETS         = 161,
    BSP_NESTING                               = 162,
    BSP_INITIAL_ARRIVAL_TIME                  = 163,
    SUB_BITSTREAM_PROPERTY                    = 164,
    ALPHA_CHANNEL_INFO                        = 165,
    OVERLAY_INFO                              = 166,
    TEMPORAL_MV_PREDICTION_CONSTRAINTS        = 167,
    FRAME_FIELD_INFO                          = 168,
    THREE_DIMENSIONAL_REFERENCE_DISPLAYS_INFO = 176,
    DEPTH_REPRESENTATION_INFO                 = 177,
    MULTIVIEW_SCENE_INFO                      = 178,
    MULTIVIEW_ACQUISITION_INFO                = 179,
    MULTIVIEW_VIEW_POSITION                   = 180
#if NH_3D
    ,ALTERNATIVE_DEPTH_INFO                    = 181
#endif
#endif

  };

  SEI();

  virtual ~SEI() {}
  virtual SEI*       getCopy( ) const;
  static const Char *getSEIMessageString(SEI::PayloadType payloadType );
  virtual PayloadType payloadType() const = 0;

#if NH_MV_SEI
  static SEI*        getNewSEIMessage         ( SEI::PayloadType payloadType );
  Bool               insertSei                ( Int curLayerId, Int curPoc, Int curTid, Int curNaluType ) const;


  virtual Void       setupFromSlice           ( const TComSlice* slice );
  virtual Void       setupFromCfgFile         ( const Char* cfgFile );
  virtual Bool       checkCfg                 ( const TComSlice* slice   );

  Void               xPrintCfgErrorIntro();
  Void               xCheckCfgRange           ( Bool& wrongConfig, Int val, Int minVal, Int maxVal, const Char* seName );
  Void               xCheckCfg                ( Bool& wrongConfig, Bool cond, const Char* errStr );
  Void               xAddGeneralOpts          ( po::Options &opts, IntAry1d defAppLayerIds, IntAry1d defAppPocs, IntAry1d defAppTids, IntAry1d defAppVclNaluTypes,
                                                Int defSeiNaluId, Int defPositionInSeiNalu, Bool defModifyByEncoder );
    // Filters where to insert SEI in the bitstream.
  // When the respected vector is empty, all layersIds, POCs, Tids, and Nalu types are used.
  IntAry1d                       m_applicableLayerIds;
  IntAry1d                       m_applicablePocs;
  IntAry1d                       m_applicableTids;
  IntAry1d                       m_applicableVclNaluTypes;

  Int                            m_payloadType;              // Payload type
  Int                            m_seiNaluId;                // Identifies to which NAL unit  the SEI is added.
  Int                            m_positionInSeiNalu;        // Identifies the order within the NAL unit
  Bool                           m_modifyByEncoder;          // Don't use the SEI cfg-file, but let let the encoder setup the NALU.

  SEIScalableNesting*            m_scalNestSeiContThisSei;   // Pointer to scalable nesting SEI containing the SEI. When NULL, the SEI is not nested.
#endif
};

static const UInt ISO_IEC_11578_LEN=16;

class SEIuserDataUnregistered : public SEI
{
public:
  PayloadType payloadType() const { return USER_DATA_UNREGISTERED; }

  SEIuserDataUnregistered()
    : userData(0)
    {}

  virtual ~SEIuserDataUnregistered()
  {
    delete userData;
  }

  UChar uuid_iso_iec_11578[ISO_IEC_11578_LEN];
  UInt  userDataLength;
  UChar *userData;
};

class SEIDecodedPictureHash : public SEI
{
public:
  PayloadType payloadType() const { return DECODED_PICTURE_HASH; }

  SEIDecodedPictureHash() {}
  virtual ~SEIDecodedPictureHash() {}

  enum Method
  {
    MD5,
    CRC,
    CHECKSUM,
    RESERVED,
  } method;

  TComPictureHash m_pictureHash;
};

class SEIActiveParameterSets : public SEI
{
public:
  PayloadType payloadType() const { return ACTIVE_PARAMETER_SETS; }

  SEIActiveParameterSets()
    : activeVPSId            (0)
    , m_selfContainedCvsFlag (false)
    , m_noParameterSetUpdateFlag (false)
    , numSpsIdsMinus1        (0)
  {}
  virtual ~SEIActiveParameterSets() {}

  Int activeVPSId;
  Bool m_selfContainedCvsFlag;
  Bool m_noParameterSetUpdateFlag;
  Int numSpsIdsMinus1;
  std::vector<Int> activeSeqParameterSetId;
};

class SEIBufferingPeriod : public SEI
{
public:
  PayloadType payloadType() const { return BUFFERING_PERIOD; }
  void copyTo (SEIBufferingPeriod& target);

  SEIBufferingPeriod()
  : m_bpSeqParameterSetId (0)
  , m_rapCpbParamsPresentFlag (false)
  , m_cpbDelayOffset      (0)
  , m_dpbDelayOffset      (0)
  {
    ::memset(m_initialCpbRemovalDelay, 0, sizeof(m_initialCpbRemovalDelay));
    ::memset(m_initialCpbRemovalDelayOffset, 0, sizeof(m_initialCpbRemovalDelayOffset));
    ::memset(m_initialAltCpbRemovalDelay, 0, sizeof(m_initialAltCpbRemovalDelay));
    ::memset(m_initialAltCpbRemovalDelayOffset, 0, sizeof(m_initialAltCpbRemovalDelayOffset));
  }
  virtual ~SEIBufferingPeriod() {}

  UInt m_bpSeqParameterSetId;
  Bool m_rapCpbParamsPresentFlag;
  UInt m_cpbDelayOffset;
  UInt m_dpbDelayOffset;
  UInt m_initialCpbRemovalDelay         [MAX_CPB_CNT][2];
  UInt m_initialCpbRemovalDelayOffset   [MAX_CPB_CNT][2];
  UInt m_initialAltCpbRemovalDelay      [MAX_CPB_CNT][2];
  UInt m_initialAltCpbRemovalDelayOffset[MAX_CPB_CNT][2];
  Bool m_concatenationFlag;
  UInt m_auCpbRemovalDelayDelta;
};
class SEIPictureTiming : public SEI
{
public:
  PayloadType payloadType() const { return PICTURE_TIMING; }
  void copyTo (SEIPictureTiming& target);

  SEIPictureTiming()
  : m_picStruct               (0)
  , m_sourceScanType          (0)
  , m_duplicateFlag           (false)
  , m_picDpbOutputDuDelay     (0)
  {}
  virtual ~SEIPictureTiming()
  {
  }

  UInt  m_picStruct;
  UInt  m_sourceScanType;
  Bool  m_duplicateFlag;

  UInt  m_auCpbRemovalDelay;
  UInt  m_picDpbOutputDelay;
  UInt  m_picDpbOutputDuDelay;
  UInt  m_numDecodingUnitsMinus1;
  Bool  m_duCommonCpbRemovalDelayFlag;
  UInt  m_duCommonCpbRemovalDelayMinus1;
  std::vector<UInt> m_numNalusInDuMinus1;
  std::vector<UInt> m_duCpbRemovalDelayMinus1;
};

class SEIDecodingUnitInfo : public SEI
{
public:
  PayloadType payloadType() const { return DECODING_UNIT_INFO; }

  SEIDecodingUnitInfo()
    : m_decodingUnitIdx(0)
    , m_duSptCpbRemovalDelay(0)
    , m_dpbOutputDuDelayPresentFlag(false)
    , m_picSptDpbOutputDuDelay(0)
  {}
  virtual ~SEIDecodingUnitInfo() {}
  Int m_decodingUnitIdx;
  Int m_duSptCpbRemovalDelay;
  Bool m_dpbOutputDuDelayPresentFlag;
  Int m_picSptDpbOutputDuDelay;
};

class SEIRecoveryPoint : public SEI
{
public:
  PayloadType payloadType() const { return RECOVERY_POINT; }

  SEIRecoveryPoint() {}
  virtual ~SEIRecoveryPoint() {}

  Int  m_recoveryPocCnt;
  Bool m_exactMatchingFlag;
  Bool m_brokenLinkFlag;
};

class SEIFramePacking : public SEI
{
public:
  PayloadType payloadType() const { return FRAME_PACKING; }

  SEIFramePacking() {}
  virtual ~SEIFramePacking() {}

  Int  m_arrangementId;
  Bool m_arrangementCancelFlag;
  Int  m_arrangementType;
  Bool m_quincunxSamplingFlag;
  Int  m_contentInterpretationType;
  Bool m_spatialFlippingFlag;
  Bool m_frame0FlippedFlag;
  Bool m_fieldViewsFlag;
  Bool m_currentFrameIsFrame0Flag;
  Bool m_frame0SelfContainedFlag;
  Bool m_frame1SelfContainedFlag;
  Int  m_frame0GridPositionX;
  Int  m_frame0GridPositionY;
  Int  m_frame1GridPositionX;
  Int  m_frame1GridPositionY;
  Int  m_arrangementReservedByte;
  Bool m_arrangementPersistenceFlag;
  Bool m_upsampledAspectRatio;
};

class SEISegmentedRectFramePacking : public SEI
{
public:
  PayloadType payloadType() const { return SEGM_RECT_FRAME_PACKING; }

  SEISegmentedRectFramePacking() {}
  virtual ~SEISegmentedRectFramePacking() {}

  Bool m_arrangementCancelFlag;
  Int  m_contentInterpretationType;
  Bool m_arrangementPersistenceFlag;
};

class SEIDisplayOrientation : public SEI
{
public:
  PayloadType payloadType() const { return DISPLAY_ORIENTATION; }

  SEIDisplayOrientation()
    : cancelFlag(true)
    , persistenceFlag(0)
    , extensionFlag(false)
    {}
  virtual ~SEIDisplayOrientation() {}

  Bool cancelFlag;
  Bool horFlip;
  Bool verFlip;

  UInt anticlockwiseRotation;
  Bool persistenceFlag;
  Bool extensionFlag;
};

class SEITemporalLevel0Index : public SEI
{
public:
  PayloadType payloadType() const { return TEMPORAL_LEVEL0_INDEX; }

  SEITemporalLevel0Index()
    : tl0Idx(0)
    , rapIdx(0)
    {}
  virtual ~SEITemporalLevel0Index() {}

  UInt tl0Idx;
  UInt rapIdx;
};

class SEIGradualDecodingRefreshInfo : public SEI
{
public:
  PayloadType payloadType() const { return REGION_REFRESH_INFO; }

  SEIGradualDecodingRefreshInfo()
    : m_gdrForegroundFlag(0)
  {}
  virtual ~SEIGradualDecodingRefreshInfo() {}

  Bool m_gdrForegroundFlag;
};

class SEINoDisplay : public SEI
{
public:
  PayloadType payloadType() const { return NO_DISPLAY; }

  SEINoDisplay()
    : m_noDisplay(false)
  {}
  virtual ~SEINoDisplay() {}

  Bool m_noDisplay;
};

class SEISOPDescription : public SEI
{
public:
  PayloadType payloadType() const { return SOP_DESCRIPTION; }

  SEISOPDescription() {}
  virtual ~SEISOPDescription() {}

  UInt m_sopSeqParameterSetId;
  UInt m_numPicsInSopMinus1;

  UInt m_sopDescVclNaluType[MAX_NUM_PICS_IN_SOP];
  UInt m_sopDescTemporalId[MAX_NUM_PICS_IN_SOP];
  UInt m_sopDescStRpsIdx[MAX_NUM_PICS_IN_SOP];
  Int m_sopDescPocDelta[MAX_NUM_PICS_IN_SOP];
};

class SEIToneMappingInfo : public SEI
{
public:
  PayloadType payloadType() const { return TONE_MAPPING_INFO; }
  SEIToneMappingInfo() {}
  virtual ~SEIToneMappingInfo() {}

  Int    m_toneMapId;
  Bool   m_toneMapCancelFlag;
  Bool   m_toneMapPersistenceFlag;
  Int    m_codedDataBitDepth;
  Int    m_targetBitDepth;
  Int    m_modelId;
  Int    m_minValue;
  Int    m_maxValue;
  Int    m_sigmoidMidpoint;
  Int    m_sigmoidWidth;
  std::vector<Int> m_startOfCodedInterval;
  Int    m_numPivots;
  std::vector<Int> m_codedPivotValue;
  std::vector<Int> m_targetPivotValue;
  Int    m_cameraIsoSpeedIdc;
  Int    m_cameraIsoSpeedValue;
  Int    m_exposureIndexIdc;
  Int    m_exposureIndexValue;
  Bool   m_exposureCompensationValueSignFlag;
  Int    m_exposureCompensationValueNumerator;
  Int    m_exposureCompensationValueDenomIdc;
  Int    m_refScreenLuminanceWhite;
  Int    m_extendedRangeWhiteLevel;
  Int    m_nominalBlackLevelLumaCodeValue;
  Int    m_nominalWhiteLevelLumaCodeValue;
  Int    m_extendedWhiteLevelLumaCodeValue;
};

class SEIKneeFunctionInfo : public SEI
{
public:
  PayloadType payloadType() const { return KNEE_FUNCTION_INFO; }
  SEIKneeFunctionInfo() {}
  virtual ~SEIKneeFunctionInfo() {}

  Int   m_kneeId;
  Bool  m_kneeCancelFlag;
  Bool  m_kneePersistenceFlag;
  Int   m_kneeInputDrange;
  Int   m_kneeInputDispLuminance;
  Int   m_kneeOutputDrange;
  Int   m_kneeOutputDispLuminance;
  Int   m_kneeNumKneePointsMinus1;
  std::vector<Int> m_kneeInputKneePoint;
  std::vector<Int> m_kneeOutputKneePoint;
};

class SEIChromaSamplingFilterHint : public SEI
{
public:
  PayloadType payloadType() const {return CHROMA_SAMPLING_FILTER_HINT;}
  SEIChromaSamplingFilterHint() {}
  virtual ~SEIChromaSamplingFilterHint() {
    if(m_verChromaFilterIdc == 1)
    {
      for(Int i = 0; i < m_numVerticalFilters; i ++)
      {
        free(m_verFilterCoeff[i]);
      }
      free(m_verFilterCoeff);
      free(m_verTapLengthMinus1);
    }
    if(m_horChromaFilterIdc == 1)
    {
      for(Int i = 0; i < m_numHorizontalFilters; i ++)
      {
        free(m_horFilterCoeff[i]);
      }
      free(m_horFilterCoeff);
      free(m_horTapLengthMinus1);
    }
  }

  Int   m_verChromaFilterIdc;
  Int   m_horChromaFilterIdc;
  Bool  m_verFilteringProcessFlag;
  Int   m_targetFormatIdc;
  Bool  m_perfectReconstructionFlag;
  Int   m_numVerticalFilters;
  Int*  m_verTapLengthMinus1;
  Int** m_verFilterCoeff;
  Int   m_numHorizontalFilters;
  Int*  m_horTapLengthMinus1;
  Int** m_horFilterCoeff;
};

class SEIMasteringDisplayColourVolume : public SEI
{
public:
    PayloadType payloadType() const { return MASTERING_DISPLAY_COLOUR_VOLUME; }
    SEIMasteringDisplayColourVolume() {}
    virtual ~SEIMasteringDisplayColourVolume(){}

    TComSEIMasteringDisplay values;
};

#if NH_MV
#if !NH_MV_SEI
class SEISubBitstreamProperty : public SEI
{
public:
  PayloadType payloadType() const { return SUB_BITSTREAM_PROPERTY; }

  SEISubBitstreamProperty():   m_activeVpsId(-1), m_numAdditionalSubStreams(0) {}
  virtual ~SEISubBitstreamProperty() {}

  Int  m_activeVpsId;
  Int  m_numAdditionalSubStreams;
  std::vector<Int>  m_subBitstreamMode;
  std::vector<Int>  m_outputLayerSetIdxToVps;
  std::vector<Int>  m_highestSublayerId;
  std::vector<Int>  m_avgBitRate;
  std::vector<Int>  m_maxBitRate;
};
#endif
#endif

typedef std::list<SEI*> SEIMessages;

/// output a selection of SEI messages by payload type. Ownership stays in original message list.
SEIMessages getSeisByType(SEIMessages &seiList, SEI::PayloadType seiType);

/// remove a selection of SEI messages by payload type from the original list and return them in a new list.
SEIMessages extractSeisByType(SEIMessages &seiList, SEI::PayloadType seiType);

/// delete list of SEI messages (freeing the referenced objects)
Void deleteSEIs (SEIMessages &seiList);

class SEIScalableNesting : public SEI
{
public:
  PayloadType payloadType() const { return SCALABLE_NESTING; }

  SEIScalableNesting() {}

  virtual ~SEIScalableNesting()
  {
    deleteSEIs(m_nestedSEIs);
  }

  Bool  m_bitStreamSubsetFlag;
  Bool  m_nestingOpFlag;
  Bool  m_defaultOpFlag;                             //value valid if m_nestingOpFlag != 0
  UInt  m_nestingNumOpsMinus1;                       // -"-
  UInt  m_nestingMaxTemporalIdPlus1[MAX_TLAYER];     // -"-
  UInt  m_nestingOpIdx[MAX_NESTING_NUM_OPS];         // -"-

  Bool  m_allLayersFlag;                             //value valid if m_nestingOpFlag == 0
  UInt  m_nestingNoOpMaxTemporalIdPlus1;             //value valid if m_nestingOpFlag == 0 and m_allLayersFlag == 0
  UInt  m_nestingNumLayersMinus1;                    //value valid if m_nestingOpFlag == 0 and m_allLayersFlag == 0
  UChar m_nestingLayerId[MAX_NESTING_NUM_LAYER];     //value valid if m_nestingOpFlag == 0 and m_allLayersFlag == 0. This can e.g. be a static array of 64 UChar values

  SEIMessages m_nestedSEIs;
};

class SEITimeCode : public SEI
{
public:
  PayloadType payloadType() const { return TIME_CODE; }
  SEITimeCode() {}
  virtual ~SEITimeCode(){}

  UInt numClockTs;
  TComSEITimeSet timeSetArray[MAX_TIMECODE_SEI_SETS];
};

//definition according to P1005_v1;
class SEITempMotionConstrainedTileSets: public SEI
{
  struct TileSetData
  {
    protected:
      std::vector<Int> m_top_left_tile_index;  //[tileSetIdx][tileIdx];
      std::vector<Int> m_bottom_right_tile_index;

    public:
      Int     m_mcts_id;
      Bool    m_display_tile_set_flag;
      Int     m_num_tile_rects_in_set; //_minus1;
      Bool    m_exact_sample_value_match_flag;
      Bool    m_mcts_tier_level_idc_present_flag;
      Bool    m_mcts_tier_flag;
      Int     m_mcts_level_idc;

      Void setNumberOfTileRects(const Int number)
      {
        m_top_left_tile_index    .resize(number);
        m_bottom_right_tile_index.resize(number);
      }

      Int  getNumberOfTileRects() const
      {
        assert(m_top_left_tile_index.size() == m_bottom_right_tile_index.size());
        return Int(m_top_left_tile_index.size());
      }

            Int &topLeftTileIndex    (const Int tileRectIndex)       { return m_top_left_tile_index    [tileRectIndex]; }
            Int &bottomRightTileIndex(const Int tileRectIndex)       { return m_bottom_right_tile_index[tileRectIndex]; }
      const Int &topLeftTileIndex    (const Int tileRectIndex) const { return m_top_left_tile_index    [tileRectIndex]; }
      const Int &bottomRightTileIndex(const Int tileRectIndex) const { return m_bottom_right_tile_index[tileRectIndex]; }
  };

protected:
  std::vector<TileSetData> m_tile_set_data;

public:

  Bool    m_mc_all_tiles_exact_sample_value_match_flag;
  Bool    m_each_tile_one_tile_set_flag;
  Bool    m_limited_tile_set_display_flag;
  Bool    m_max_mcs_tier_level_idc_present_flag;
  Bool    m_max_mcts_tier_flag;
  Int     m_max_mcts_level_idc;

  PayloadType payloadType() const { return TEMP_MOTION_CONSTRAINED_TILE_SETS; }

  Void setNumberOfTileSets(const Int number)       { m_tile_set_data.resize(number);     }
  Int  getNumberOfTileSets()                 const { return Int(m_tile_set_data.size()); }

        TileSetData &tileSetData (const Int index)       { return m_tile_set_data[index]; }
  const TileSetData &tileSetData (const Int index) const { return m_tile_set_data[index]; }

};

#if NH_MV_SEI
#if NH_MV_LAYERS_NOT_PRESENT_SEI
class SEILayersNotPresent : public SEI
{
public:
  PayloadType payloadType( ) const { return LAYERS_NOT_PRESENT; }
  SEILayersNotPresent ( ) { };
  ~SEILayersNotPresent( ) { };
  SEI* getCopy( ) const { return new SEILayersNotPresent(*this); };

  Void setupFromCfgFile( const Char*      cfgFile );
  Bool checkCfg        ( const TComSlice* slice   );

  Int       m_lnpSeiActiveVpsId;
  UInt      m_lnpSeiMaxLayers;
  BoolAry1d m_layerNotPresentFlag;

  Void resizeDimI( Int sizeDimI )
  {
    m_layerNotPresentFlag.resize( sizeDimI );
  }
};
#endif

class SEIInterLayerConstrainedTileSets : public SEI
{
public:
  PayloadType payloadType( ) const { return INTER_LAYER_CONSTRAINED_TILE_SETS; }
  SEIInterLayerConstrainedTileSets ( ) { };
  ~SEIInterLayerConstrainedTileSets( ) { };
  SEI* getCopy( ) const { return new SEIInterLayerConstrainedTileSets(*this); };

  Void setupFromCfgFile( const Char*      cfgFile );
  Bool checkCfg        ( const TComSlice* slice   );

  Bool      m_ilAllTilesExactSampleValueMatchFlag;
  Bool      m_ilOneTilePerTileSetFlag;
  Int       m_ilNumSetsInMessageMinus1;
  Bool      m_skippedTileSetPresentFlag;
  IntAry1d  m_ilctsId;
  IntAry1d  m_ilNumTileRectsInSetMinus1;
  IntAry2d  m_ilTopLeftTileIndex;
  IntAry2d  m_ilBottomRightTileIndex;
  IntAry1d  m_ilcIdc;
  BoolAry1d m_ilExactSampleValueMatchFlag;
  Int       m_allTilesIlcIdc;

  Void      resizeDimI( Int sizeDimI )
  {
    m_ilctsId                    .resize( sizeDimI );
    m_ilNumTileRectsInSetMinus1  .resize( sizeDimI );
    m_ilTopLeftTileIndex         .resize( sizeDimI );
    m_ilBottomRightTileIndex     .resize( sizeDimI );
    m_ilcIdc                     .resize( sizeDimI );
    m_ilExactSampleValueMatchFlag.resize( sizeDimI );
  }

  Void      resizeDimJ( Int i, Int sizeDimJ )
  {
    m_ilTopLeftTileIndex    [i].resize( sizeDimJ );
    m_ilBottomRightTileIndex[i].resize( sizeDimJ );
  }

};

#if NH_MV_TBD
class SEIBspNesting : public SEI
{
public:
  PayloadType payloadType( ) const { return BSP_NESTING; }
  SEIBspNesting ( ) { };
  ~SEIBspNesting( ) { };
  SEI* getCopy( ) const { return new SEIBspNesting(*this); };

  Void setupFromCfgFile( const Char*      cfgFile );
  Void setupFromSlice  ( const TComSlice* slice   );
  Bool checkCfg        ( const TComSlice* slice   );

  Int       m_seiOlsIdx;
  Int       m_seiPartitioningSchemeIdx;
  Int       m_bspIdx;
  Int       m_bspNestingZeroBit;
  Int       m_numSeisInBspMinus1;
};

class SEIBspInitialArrivalTime : public SEI
{
public:
  PayloadType payloadType( ) const { return BSP_INITIAL_ARRIVAL_TIME; }
  SEIBspInitialArrivalTime ( ) { };
  ~SEIBspInitialArrivalTime( ) { };
  SEI* getCopy( ) const { return new SEIBspInitialArrivalTime(*this); };

  Void setupFromCfgFile( const Char*      cfgFile );
  Void setupFromSlice  ( const TComSlice* slice   );
  Bool checkCfg        ( const TComSlice* slice   );

  IntAry1d  m_nalInitialArrivalDelay;
  IntAry1d  m_vclInitialArrivalDelay;
};
#endif

class SEISubBitstreamProperty : public SEI
{
public:
  PayloadType payloadType( ) const { return SUB_BITSTREAM_PROPERTY; }
  SEISubBitstreamProperty ( ) { };
  ~SEISubBitstreamProperty( ) { };
  SEI* getCopy( ) const { return new SEISubBitstreamProperty(*this); };

  Void setupFromCfgFile( const Char*      cfgFile );
  Bool checkCfg        ( const TComSlice* slice   );
  Void resizeArrays    ( );

  Int       m_sbPropertyActiveVpsId;
  Int       m_numAdditionalSubStreamsMinus1;
  IntAry1d  m_subBitstreamMode;
  IntAry1d  m_olsIdxToVps;
  IntAry1d  m_highestSublayerId;
  IntAry1d  m_avgSbPropertyBitRate;
  IntAry1d  m_maxSbPropertyBitRate;
};

class SEIAlphaChannelInfo : public SEI
{
public:
  PayloadType payloadType( ) const { return ALPHA_CHANNEL_INFO; }
  SEIAlphaChannelInfo ( ) { };
  ~SEIAlphaChannelInfo( ) { };
  SEI* getCopy( ) const { return new SEIAlphaChannelInfo(*this); };

  Void setupFromCfgFile( const Char*      cfgFile );
  Bool checkCfg        ( const TComSlice* slice   );

  Bool      m_alphaChannelCancelFlag;
  Int       m_alphaChannelUseIdc;
  Int       m_alphaChannelBitDepthMinus8;
  Int       m_alphaTransparentValue;
  Int       m_alphaOpaqueValue;
  Bool      m_alphaChannelIncrFlag;
  Bool      m_alphaChannelClipFlag;
  Bool      m_alphaChannelClipTypeFlag;
};

class SEIOverlayInfo : public SEI
{
public:
  PayloadType payloadType( ) const { return OVERLAY_INFO; }
  SEIOverlayInfo ( );
  ~SEIOverlayInfo( ) { };
  SEI* getCopy( ) const { return new SEIOverlayInfo(*this); };

  Void setupFromCfgFile( const Char*      cfgFile );
  Bool checkCfg        ( const TComSlice* slice   );

  const Int m_numOverlaysMax;
  const Int m_numOverlayElementsMax;
  const Int m_numStringBytesMax;  //incl. null termination byte

  Bool      m_overlayInfoCancelFlag;
  Int       m_overlayContentAuxIdMinus128;
  Int       m_overlayLabelAuxIdMinus128;
  Int       m_overlayAlphaAuxIdMinus128;
  Int       m_overlayElementLabelValueLengthMinus8;
  Int       m_numOverlaysMinus1;
  IntAry1d  m_overlayIdx;
  BoolAry1d m_languageOverlayPresentFlag;
  IntAry1d  m_overlayContentLayerId;
  BoolAry1d m_overlayLabelPresentFlag;
  IntAry1d  m_overlayLabelLayerId;
  BoolAry1d m_overlayAlphaPresentFlag;
  IntAry1d  m_overlayAlphaLayerId;
  IntAry1d  m_numOverlayElementsMinus1;
  IntAry2d  m_overlayElementLabelMin;
  IntAry2d  m_overlayElementLabelMax;
  StringAry1d  m_overlayLanguage;
  StringAry1d  m_overlayName;
  StringAry2d  m_overlayElementName;
  Bool      m_overlayInfoPersistenceFlag;
};

class SEITemporalMvPredictionConstraints : public SEI
{
public:
  PayloadType payloadType( ) const { return TEMPORAL_MV_PREDICTION_CONSTRAINTS; }
  SEITemporalMvPredictionConstraints ( ) { };
  ~SEITemporalMvPredictionConstraints( ) { };
  SEI* getCopy( ) const { return new SEITemporalMvPredictionConstraints(*this); };

  Void setupFromCfgFile( const Char*      cfgFile );
  Bool checkCfg        ( const TComSlice* slice   );

  Bool      m_prevPicsNotUsedFlag;
  Bool      m_noIntraLayerColPicFlag;
};

#if NH_MV_SEI_TBD
class SEIFrameFieldInfo : public SEI
{
public:
  PayloadType payloadType( ) const { return FRAME_FIELD_INFO; }
  SEIFrameFieldInfo ( ) { };
  ~SEIFrameFieldInfo( ) { };
  SEI* getCopy( ) const { return new SEIFrameFieldInfo(*this); };

  Void setupFromCfgFile( const Char*      cfgFile );
  Void setupFromSlice  ( const TComSlice* slice   );
  Bool checkCfg        ( const TComSlice* slice   );

  Int       m_ffinfoPicStruct;
  Int       m_ffinfoSourceScanType;
  Bool      m_ffinfoDuplicateFlag;
};
#endif

class SEIThreeDimensionalReferenceDisplaysInfo : public SEI
{
public:
  PayloadType payloadType( ) const { return THREE_DIMENSIONAL_REFERENCE_DISPLAYS_INFO; }
  SEIThreeDimensionalReferenceDisplaysInfo ( ) { };
  ~SEIThreeDimensionalReferenceDisplaysInfo( ) { };
  SEI* getCopy( ) const { return new SEIThreeDimensionalReferenceDisplaysInfo(*this); };

  Void setupFromCfgFile( const Char*      cfgFile );
  Bool checkCfg        ( const TComSlice* slice   );

  Int getNumRefDisplaysMinus1( ) const
  {
    return m_numRefDisplaysMinus1;
  }

  Int       m_precRefDisplayWidth;
  Bool      m_refViewingDistanceFlag;
  Int       m_precRefViewingDist;
  Int       m_numRefDisplaysMinus1;
  IntAry1d  m_leftViewId;
  IntAry1d  m_rightViewId;
  IntAry1d  m_exponentRefDisplayWidth;
  IntAry1d  m_mantissaRefDisplayWidth;
  IntAry1d  m_exponentRefViewingDistance;
  IntAry1d  m_mantissaRefViewingDistance;
  BoolAry1d m_additionalShiftPresentFlag;
  IntAry1d  m_numSampleShiftPlus512;
  Bool      m_threeDimensionalReferenceDisplaysExtensionFlag;

  Void resizeArrays( )
  {
    Int numReferenceDiaplays = getNumRefDisplaysMinus1() + 1;

    m_leftViewId    .resize( numReferenceDiaplays );
    m_rightViewId   .resize( numReferenceDiaplays );
    m_exponentRefDisplayWidth      .resize( numReferenceDiaplays );
    m_mantissaRefDisplayWidth      .resize( numReferenceDiaplays );
    m_exponentRefViewingDistance   .resize( numReferenceDiaplays );
    m_mantissaRefViewingDistance   .resize( numReferenceDiaplays );
    m_additionalShiftPresentFlag   .resize( numReferenceDiaplays );
    m_numSampleShiftPlus512        .resize( numReferenceDiaplays );
  }

  UInt getMantissaReferenceDisplayWidthLen  ( Int i ) const ;
  UInt getMantissaReferenceViewingDistanceLen  ( Int i ) const ;
private:
  UInt xGetSyntaxElementLen( Int expo, Int prec, Int val ) const;
};

#if SEI_DRI_F0169
class SEIDepthRepresentationInfo : public SEI
{
    public:
        PayloadType payloadType( ) const { return DEPTH_REPRESENTATION_INFO; }
        SEIDepthRepresentationInfo ( )
        {
            m_currLayerID=-1;
        };
        ~SEIDepthRepresentationInfo( ) { };
        SEI* getCopy( ) const { return new SEIDepthRepresentationInfo(*this); };

        Void setupFromCfgFile( const Char*      cfgFile );
        Void setupFromSlice  ( const TComSlice* slice   );
        Bool checkCfg        ( const TComSlice* slice   );
        Void clear()
        {
            int i;
            m_zNearFlag.clear();
            m_zFarFlag.clear();
            m_dMinFlag.clear();
            m_dMaxFlag.clear();

            for(i=0;i<m_zNear.size();i++)
                m_zNear[i].clear();
            m_zNear.clear();

            for(i=0;i<m_zFar.size();i++)
                m_zFar[i].clear();
            m_zFar.clear();

            for(i=0;i<m_dMin.size();i++)
                m_dMin[i].clear();
            m_dMin.clear();

            for(i=0;i<m_dMax.size();i++)
                m_dMax[i].clear();
            m_dMax.clear();

            for(i=0;i<m_depthRepresentationType.size();i++)
                m_depthRepresentationType[i].clear();
            m_depthRepresentationType.clear();

            for(i=0;i<m_disparityRefViewId.size();i++)
                m_disparityRefViewId[i].clear();
            m_disparityRefViewId.clear();

            for(i=0;i<m_depthNonlinearRepresentationNumMinus1.size();i++)
                m_depthNonlinearRepresentationNumMinus1[i].clear();
            m_depthNonlinearRepresentationNumMinus1.clear();

            for(i=0;i<m_depth_nonlinear_representation_model.size();i++)
                m_depth_nonlinear_representation_model[i].clear();
            m_depth_nonlinear_representation_model.clear();

        }
        Int m_currLayerID;
        BoolAry1d      m_zNearFlag;
        BoolAry1d      m_zFarFlag;
        BoolAry1d      m_dMinFlag;
        BoolAry1d      m_dMaxFlag;
        BoolAry2d      m_depthRepresentationInfoSeiPresentFlag;
        std::vector< std::vector<Double> > m_zNear,m_zFar,m_dMin,m_dMax;

        IntAry2d       m_depthRepresentationType;
        IntAry2d       m_disparityRefViewId;
        IntAry2d       m_depthNonlinearRepresentationNumMinus1;
        IntAry2d       m_depth_nonlinear_representation_model;
};
#endif

class SEIMultiviewSceneInfo : public SEI
{
public:
  PayloadType payloadType( ) const { return MULTIVIEW_SCENE_INFO; }
  SEIMultiviewSceneInfo ( ) { };
  ~SEIMultiviewSceneInfo( ) { };
  SEI* getCopy( ) const { return new SEIMultiviewSceneInfo(*this); };

  Void setupFromCfgFile( const Char*      cfgFile );
  Bool checkCfg        ( const TComSlice* slice   );

  Int       m_minDisparity;
  Int       m_maxDisparityRange;
};


class SEIMultiviewAcquisitionInfo : public SEI
{
public:
  PayloadType payloadType( ) const { return MULTIVIEW_ACQUISITION_INFO; }
  SEIMultiviewAcquisitionInfo ( ) { };
  ~SEIMultiviewAcquisitionInfo( ) { };
  SEI* getCopy( ) const { return new SEIMultiviewAcquisitionInfo(*this); };

  Void setupFromCfgFile( const Char*      cfgFile );
  Bool checkCfg        ( const TComSlice* slice   );

  Int getNumViewsMinus1( ) const
  {
    Int numViewsMinus1;
    if( m_scalNestSeiContThisSei != NULL )
    {
      numViewsMinus1 = m_scalNestSeiContThisSei->m_nestingNumLayersMinus1;
    }
    else
    {
      numViewsMinus1 = 0;
    }
    return numViewsMinus1;
  }

  Void resizeArrays( )
  {
    Int numViews = getNumViewsMinus1() + 1;
    m_signFocalLengthX       .resize( numViews );
    m_exponentFocalLengthX   .resize( numViews );
    m_mantissaFocalLengthX   .resize( numViews );
    m_signFocalLengthY       .resize( numViews );
    m_exponentFocalLengthY   .resize( numViews );
    m_mantissaFocalLengthY   .resize( numViews );
    m_signPrincipalPointX    .resize( numViews );
    m_exponentPrincipalPointX.resize( numViews );
    m_mantissaPrincipalPointX.resize( numViews );
    m_signPrincipalPointY    .resize( numViews );
    m_exponentPrincipalPointY.resize( numViews );
    m_mantissaPrincipalPointY.resize( numViews );
    m_signSkewFactor         .resize( numViews );
    m_exponentSkewFactor     .resize( numViews );
    m_mantissaSkewFactor     .resize( numViews );

    m_signR                  .resize( numViews );
    m_exponentR              .resize( numViews );
    m_mantissaR              .resize( numViews );
    m_signT                  .resize( numViews );
    m_exponentT              .resize( numViews );
    m_mantissaT              .resize( numViews );

    for( Int i = 0; i  < numViews ; i++ )
    {
      m_signR    [i].resize( 3 );
      m_exponentR[i].resize( 3 );
      m_mantissaR[i].resize( 3 );
      m_signT    [i].resize( 3 );
      m_exponentT[i].resize( 3 );
      m_mantissaT[i].resize( 3 );

      for (Int j = 0; j < 3; j++)
      {
        m_signR    [i][j].resize( 3 );
        m_exponentR[i][j].resize( 3 );
        m_mantissaR[i][j].resize( 3 );
      }
    }
  }

  UInt getMantissaFocalLengthXLen   ( Int i ) const ;
  UInt getMantissaFocalLengthYLen   ( Int i ) const ;
  UInt getMantissaPrincipalPointXLen( Int i ) const ;
  UInt getMantissaPrincipalPointYLen( Int i ) const ;
  UInt getMantissaSkewFactorLen     ( Int i ) const ;
  UInt getMantissaRLen              ( Int i, Int j, Int k ) const ;
  UInt getMantissaTLen              ( Int i, Int j )        const ;

  Bool      m_intrinsicParamFlag;
  Bool      m_extrinsicParamFlag;
  Bool      m_intrinsicParamsEqualFlag;
  Int       m_precFocalLength;
  Int       m_precPrincipalPoint;
  Int       m_precSkewFactor;
  BoolAry1d m_signFocalLengthX;
  IntAry1d  m_exponentFocalLengthX;
  IntAry1d  m_mantissaFocalLengthX;
  BoolAry1d m_signFocalLengthY;
  IntAry1d  m_exponentFocalLengthY;
  IntAry1d  m_mantissaFocalLengthY;
  BoolAry1d m_signPrincipalPointX;
  IntAry1d  m_exponentPrincipalPointX;
  IntAry1d  m_mantissaPrincipalPointX;
  BoolAry1d m_signPrincipalPointY;
  IntAry1d  m_exponentPrincipalPointY;
  IntAry1d  m_mantissaPrincipalPointY;
  BoolAry1d m_signSkewFactor;
  IntAry1d  m_exponentSkewFactor;
  IntAry1d  m_mantissaSkewFactor;
  Int       m_precRotationParam;
  Int       m_precTranslationParam;
  BoolAry3d m_signR;
  IntAry3d  m_exponentR;
  IntAry3d  m_mantissaR;
  BoolAry2d m_signT;
  IntAry2d  m_exponentT;
  IntAry2d  m_mantissaT;
private:
  UInt xGetSyntaxElementLen( Int expo, Int prec, Int val ) const;
};



class SEIMultiviewViewPosition : public SEI
{
public:
  PayloadType payloadType( ) const { return MULTIVIEW_VIEW_POSITION; }
  SEIMultiviewViewPosition ( ) { };
  ~SEIMultiviewViewPosition( ) { };
  SEI* getCopy( ) const { return new SEIMultiviewViewPosition(*this); };

  Void setupFromCfgFile( const Char*      cfgFile );
  Void setupFromSlice  ( const TComSlice* slice   );
  Bool checkCfg        ( const TComSlice* slice   );

  Int       m_numViewsMinus1;
  IntAry1d  m_viewPosition;
};

#if NH_3D
class SEIAlternativeDepthInfo : public SEI
{
public:
  PayloadType payloadType( ) const { return ALTERNATIVE_DEPTH_INFO; }
  SEIAlternativeDepthInfo ( ) { };
  ~SEIAlternativeDepthInfo( ) { };
  SEI* getCopy( ) const { return new SEIAlternativeDepthInfo(*this); };

  Void setupFromCfgFile( const Char*      cfgFile );
  Bool checkCfg        ( const TComSlice* slice   );

  UInt getManGvdFocalLengthXLen       ( Int i, Int j ) const;
  UInt getManGvdFocalLengthYLen       ( Int i, Int j ) const;
  UInt getManGvdPrincipalPointXLen    ( Int i, Int j ) const;
  UInt getManGvdPrincipalPointYLen    ( Int i, Int j ) const;
  //UInt getManGvdRLen                  ( Int i, int j ) const;
  UInt getManGvdTXLen                 ( Int i, Int j ) const;
  UInt xGetSyntaxElementLen           ( Int expo, Int prec, Int val ) const;

  Void resizeArrays( )
  {
    const Int numViews = 3; // getNumConstituentViewsGvdMinus1() + 1;

    m_signGvdZNearFlag.resize(3);
    m_expGvdZNear.resize(3);
    m_manLenGvdZNearMinus1.resize(3);
    m_manGvdZNear.resize(3);
    m_signGvdZFarFlag.resize(3);
    m_expGvdZFar.resize(3);
    m_manLenGvdZFarMinus1.resize(3);
    m_manGvdZFar.resize(3);

    m_signGvdFocalLengthX.resize(3);
    m_expGvdFocalLengthX.resize(3);
    m_manGvdFocalLengthX.resize(3);
    m_signGvdFocalLengthY.resize(3);
    m_expGvdFocalLengthY.resize(3);
    m_manGvdFocalLengthY.resize(3);
    m_signGvdPrincipalPointX.resize(3);
    m_expGvdPrincipalPointX.resize(3);
    m_manGvdPrincipalPointX.resize(3);
    m_signGvdPrincipalPointY.resize(3);
    m_expGvdPrincipalPointY.resize(3);
    m_manGvdPrincipalPointY.resize(3);

    m_signGvdR00.resize(3);
    m_expGvdR00.resize(3);
    m_manGvdR00.resize(3);
    m_signGvdR01.resize(3);
    m_expGvdR01.resize(3);
    m_manGvdR01.resize(3);
    m_signGvdR02.resize(3);
    m_expGvdR02.resize(3);
    m_manGvdR02.resize(3);
    m_signGvdR10.resize(3);
    m_expGvdR10.resize(3);
    m_manGvdR10.resize(3);
    m_signGvdR11.resize(3);
    m_expGvdR11.resize(3);
    m_manGvdR11.resize(3);
    m_signGvdR12.resize(3);
    m_expGvdR12.resize(3);
    m_manGvdR12.resize(3);
    m_signGvdR20.resize(3);
    m_expGvdR20.resize(3);
    m_manGvdR20.resize(3);
    m_signGvdR21.resize(3);
    m_expGvdR21.resize(3);
    m_manGvdR21.resize(3);
    m_signGvdR22.resize(3);
    m_expGvdR22.resize(3);
    m_manGvdR22.resize(3);

    m_signGvdTX.resize(3);
    m_expGvdTX.resize(3);
    m_manGvdTX.resize(3);

    for( Int i = 0; i < numViews; i++ )
    {
      m_signGvdZNearFlag[i].resize(3);
      m_expGvdZNear[i].resize(3);
      m_manLenGvdZNearMinus1[i].resize(3);
      m_manGvdZNear[i].resize(3);
      m_signGvdZFarFlag[i].resize(3);
      m_expGvdZFar[i].resize(3);
      m_manLenGvdZFarMinus1[i].resize(3);
      m_manGvdZFar[i].resize(3);

      m_signGvdFocalLengthX[i].resize(3);
      m_expGvdFocalLengthX[i].resize(3);
      m_manGvdFocalLengthX[i].resize(3);
      m_signGvdFocalLengthY[i].resize(3);
      m_expGvdFocalLengthY[i].resize(3);
      m_manGvdFocalLengthY[i].resize(3);
      m_signGvdPrincipalPointX[i].resize(3);
      m_expGvdPrincipalPointX[i].resize(3);
      m_manGvdPrincipalPointX[i].resize(3);
      m_signGvdPrincipalPointY[i].resize(3);
      m_expGvdPrincipalPointY[i].resize(3);
      m_manGvdPrincipalPointY[i].resize(3);

      m_signGvdR00[i].resize(3);
      m_expGvdR00[i].resize(3);
      m_manGvdR00[i].resize(3);
      m_signGvdR01[i].resize(3);
      m_expGvdR01[i].resize(3);
      m_manGvdR01[i].resize(3);
      m_signGvdR02[i].resize(3);
      m_expGvdR02[i].resize(3);
      m_manGvdR02[i].resize(3);
      m_signGvdR10[i].resize(3);
      m_expGvdR10[i].resize(3);
      m_manGvdR10[i].resize(3);
      m_signGvdR11[i].resize(3);
      m_expGvdR11[i].resize(3);
      m_manGvdR11[i].resize(3);
      m_signGvdR12[i].resize(3);
      m_expGvdR12[i].resize(3);
      m_manGvdR12[i].resize(3);
      m_signGvdR20[i].resize(3);
      m_expGvdR20[i].resize(3);
      m_manGvdR20[i].resize(3);
      m_signGvdR21[i].resize(3);
      m_expGvdR21[i].resize(3);
      m_manGvdR21[i].resize(3);
      m_signGvdR22[i].resize(3);
      m_expGvdR22[i].resize(3);
      m_manGvdR22[i].resize(3);

      m_signGvdTX[i].resize(3);
      m_expGvdTX[i].resize(3);
      m_manGvdTX[i].resize(3);
    }

  }

  Bool      m_alternativeDepthInfoCancelFlag;
  Int       m_depthType;
  Int       m_numConstituentViewsGvdMinus1;
  Bool      m_depthPresentGvdFlag;
  Bool      m_zGvdFlag;
  Bool      m_intrinsicParamGvdFlag;
  Bool      m_rotationGvdFlag;
  Bool      m_translationGvdFlag;
  BoolAry2d m_signGvdZNearFlag;
  IntAry2d  m_expGvdZNear;
  IntAry2d  m_manLenGvdZNearMinus1;
  IntAry2d  m_manGvdZNear;
  BoolAry2d m_signGvdZFarFlag;
  IntAry2d  m_expGvdZFar;
  IntAry2d  m_manLenGvdZFarMinus1;
  IntAry2d  m_manGvdZFar;
  Int       m_precGvdFocalLength;
  Int       m_precGvdPrincipalPoint;
  Int       m_precGvdRotationParam;
  Int       m_precGvdTranslationParam;
  BoolAry2d m_signGvdFocalLengthX;
  IntAry2d  m_expGvdFocalLengthX;
  IntAry2d  m_manGvdFocalLengthX;
  BoolAry2d m_signGvdFocalLengthY;
  IntAry2d  m_expGvdFocalLengthY;
  IntAry2d  m_manGvdFocalLengthY;
  BoolAry2d m_signGvdPrincipalPointX;
  IntAry2d  m_expGvdPrincipalPointX;
  IntAry2d  m_manGvdPrincipalPointX;
  BoolAry2d m_signGvdPrincipalPointY;
  IntAry2d  m_expGvdPrincipalPointY;
  IntAry2d  m_manGvdPrincipalPointY;

  BoolAry2d m_signGvdR00;
  IntAry2d  m_expGvdR00;
  IntAry2d  m_manGvdR00;
  BoolAry2d m_signGvdR01;
  IntAry2d  m_expGvdR01;
  IntAry2d  m_manGvdR01;
  BoolAry2d m_signGvdR02;
  IntAry2d  m_expGvdR02;
  IntAry2d  m_manGvdR02;
  BoolAry2d m_signGvdR10;
  IntAry2d  m_expGvdR10;
  IntAry2d  m_manGvdR10;
  BoolAry2d m_signGvdR11;
  IntAry2d  m_expGvdR11;
  IntAry2d  m_manGvdR11;
  BoolAry2d m_signGvdR12;
  IntAry2d  m_expGvdR12;
  IntAry2d  m_manGvdR12;
  BoolAry2d m_signGvdR20;
  IntAry2d  m_expGvdR20;
  IntAry2d  m_manGvdR20;
  BoolAry2d m_signGvdR21;
  IntAry2d  m_expGvdR21;
  IntAry2d  m_manGvdR21;
  BoolAry2d m_signGvdR22;
  IntAry2d  m_expGvdR22;
  IntAry2d  m_manGvdR22;

  BoolAry2d m_signGvdTX;
  IntAry2d  m_expGvdTX;
  IntAry2d  m_manGvdTX;

  Int       m_minOffsetXInt;
  Int       m_minOffsetXFrac;
  Int       m_maxOffsetXInt;
  Int       m_maxOffsetXFrac;
  Bool      m_offsetYPresentFlag;
  Int       m_minOffsetYInt;
  Int       m_minOffsetYFrac;
  Int       m_maxOffsetYInt;
  Int       m_maxOffsetYFrac;
  Bool      m_warpMapSizePresentFlag;
  Int       m_warpMapWidthMinus2;
  Int       m_warpMapHeightMinus2;
};

#endif
#endif

#endif
//! \}
