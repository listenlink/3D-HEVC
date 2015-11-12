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

/** \file     SEI.cpp
    \brief    helper functions for SEI handling
*/

#include "CommonDef.h"
#include "SEI.h"
#if NH_MV
#include "TComSlice.h"
#endif

SEIMessages getSeisByType(SEIMessages &seiList, SEI::PayloadType seiType)
{
  SEIMessages result;

  for (SEIMessages::iterator it=seiList.begin(); it!=seiList.end(); it++)
  {
    if ((*it)->payloadType() == seiType)
    {
      result.push_back(*it);
    }
  }
  return result;
}

SEIMessages extractSeisByType(SEIMessages &seiList, SEI::PayloadType seiType)
{
  SEIMessages result;

  SEIMessages::iterator it=seiList.begin();
  while ( it!=seiList.end() )
  {
    if ((*it)->payloadType() == seiType)
    {
      result.push_back(*it);
      it = seiList.erase(it);
    }
    else
    {
      it++;
    }
  }
  return result;
}


Void deleteSEIs (SEIMessages &seiList)
{
  for (SEIMessages::iterator it=seiList.begin(); it!=seiList.end(); it++)
  {
    delete (*it);
  }
  seiList.clear();
}

void SEIBufferingPeriod::copyTo (SEIBufferingPeriod& target)
{
  target.m_bpSeqParameterSetId = m_bpSeqParameterSetId;
  target.m_rapCpbParamsPresentFlag = m_rapCpbParamsPresentFlag;
  target.m_cpbDelayOffset = m_cpbDelayOffset;
  target.m_dpbDelayOffset = m_dpbDelayOffset;
  target.m_concatenationFlag = m_concatenationFlag;
  target.m_auCpbRemovalDelayDelta = m_auCpbRemovalDelayDelta;
  ::memcpy(target.m_initialCpbRemovalDelay, m_initialCpbRemovalDelay, sizeof(m_initialCpbRemovalDelay));
  ::memcpy(target.m_initialCpbRemovalDelayOffset, m_initialCpbRemovalDelayOffset, sizeof(m_initialCpbRemovalDelayOffset));
  ::memcpy(target.m_initialAltCpbRemovalDelay, m_initialAltCpbRemovalDelay, sizeof(m_initialAltCpbRemovalDelay));
  ::memcpy(target.m_initialAltCpbRemovalDelayOffset, m_initialAltCpbRemovalDelayOffset, sizeof(m_initialAltCpbRemovalDelayOffset));
}

void SEIPictureTiming::copyTo (SEIPictureTiming& target)
{
  target.m_picStruct = m_picStruct;
  target.m_sourceScanType = m_sourceScanType;
  target.m_duplicateFlag = m_duplicateFlag;

  target.m_auCpbRemovalDelay = m_auCpbRemovalDelay;
  target.m_picDpbOutputDelay = m_picDpbOutputDelay;
  target.m_picDpbOutputDuDelay = m_picDpbOutputDuDelay;
  target.m_numDecodingUnitsMinus1 = m_numDecodingUnitsMinus1;
  target.m_duCommonCpbRemovalDelayFlag = m_duCommonCpbRemovalDelayFlag;
  target.m_duCommonCpbRemovalDelayMinus1 = m_duCommonCpbRemovalDelayMinus1;

  target.m_numNalusInDuMinus1 = m_numNalusInDuMinus1;
  target.m_duCpbRemovalDelayMinus1 = m_duCpbRemovalDelayMinus1;
}

// Static member
const TChar *SEI::getSEIMessageString(SEI::PayloadType payloadType)
{
  switch (payloadType)
  {
    case SEI::BUFFERING_PERIOD:                     return "Buffering period";
    case SEI::PICTURE_TIMING:                       return "Picture timing";
    case SEI::PAN_SCAN_RECT:                        return "Pan-scan rectangle";                   // not currently decoded
    case SEI::FILLER_PAYLOAD:                       return "Filler payload";                       // not currently decoded
    case SEI::USER_DATA_REGISTERED_ITU_T_T35:       return "User data registered";                 // not currently decoded
    case SEI::USER_DATA_UNREGISTERED:               return "User data unregistered";
    case SEI::RECOVERY_POINT:                       return "Recovery point";
    case SEI::SCENE_INFO:                           return "Scene information";                    // not currently decoded
    case SEI::FULL_FRAME_SNAPSHOT:                  return "Picture snapshot";                     // not currently decoded
    case SEI::PROGRESSIVE_REFINEMENT_SEGMENT_START: return "Progressive refinement segment start"; // not currently decoded
    case SEI::PROGRESSIVE_REFINEMENT_SEGMENT_END:   return "Progressive refinement segment end";   // not currently decoded
    case SEI::FILM_GRAIN_CHARACTERISTICS:           return "Film grain characteristics";           // not currently decoded
    case SEI::POST_FILTER_HINT:                     return "Post filter hint";                     // not currently decoded
    case SEI::TONE_MAPPING_INFO:                    return "Tone mapping information";
    case SEI::KNEE_FUNCTION_INFO:                   return "Knee function information";
    case SEI::FRAME_PACKING:                        return "Frame packing arrangement";
    case SEI::DISPLAY_ORIENTATION:                  return "Display orientation";
    case SEI::SOP_DESCRIPTION:                      return "Structure of pictures information";
    case SEI::ACTIVE_PARAMETER_SETS:                return "Active parameter sets";
    case SEI::DECODING_UNIT_INFO:                   return "Decoding unit information";
    case SEI::TEMPORAL_LEVEL0_INDEX:                return "Temporal sub-layer zero index";
    case SEI::DECODED_PICTURE_HASH:                 return "Decoded picture hash";
    case SEI::SCALABLE_NESTING:                     return "Scalable nesting";
    case SEI::REGION_REFRESH_INFO:                  return "Region refresh information";
    case SEI::NO_DISPLAY:                           return "No display";
    case SEI::TIME_CODE:                            return "Time code";
    case SEI::MASTERING_DISPLAY_COLOUR_VOLUME:      return "Mastering display colour volume";
    case SEI::SEGM_RECT_FRAME_PACKING:              return "Segmented rectangular frame packing arrangement";
    case SEI::TEMP_MOTION_CONSTRAINED_TILE_SETS:    return "Temporal motion constrained tile sets";
    case SEI::CHROMA_RESAMPLING_FILTER_HINT:        return "Chroma sampling filter hint";
    case SEI::COLOUR_REMAPPING_INFO:                return "Colour remapping info";

#if NH_MV
    case SEI::DEINTERLACED_FIELD_IDENTIFICATION:         return "Deinterlaced field identification";
    case SEI::LAYERS_NOT_PRESENT:                        return "Layers not present";
    case SEI::INTER_LAYER_CONSTRAINED_TILE_SETS:         return "Inter-layer constrained tile sets";
    case SEI::BSP_NESTING:                               return "Bitstream partition nesting";
    case SEI::BSP_INITIAL_ARRIVAL_TIME:                  return "Bitstream partition initial arrival time";
    case SEI::SUB_BITSTREAM_PROPERTY:                    return "Sub-bitstream property";
    case SEI::ALPHA_CHANNEL_INFO:                        return "Alpha channel information";
    case SEI::OVERLAY_INFO:                              return "Overlay information"  ;
    case SEI::TEMPORAL_MV_PREDICTION_CONSTRAINTS:        return "Temporal motion vector prediction constraints";
    case SEI::FRAME_FIELD_INFO:                          return "Frame-field information";
    case SEI::THREE_DIMENSIONAL_REFERENCE_DISPLAYS_INFO: return "3D reference displays information";
    case SEI::DEPTH_REPRESENTATION_INFO:                 return "Depth representation information";
    case SEI::MULTIVIEW_SCENE_INFO:                      return "Multiview scene information";
    case SEI::MULTIVIEW_ACQUISITION_INFO:                return "Multiview acquisition information";
    case SEI::MULTIVIEW_VIEW_POSITION:                   return "Multiview view position";
#endif
    default:                                        return "Unknown";
  }
}

#if NH_MV
SEI::SEI()
{
  m_scalNestSeiContThisSei = NULL;
}

SEI* SEI::getNewSEIMessage(SEI::PayloadType payloadType)
{
  switch (payloadType)
  {
#if NH_MV_SEI_TBD
    //////////////////////////////////////////////////////////////////////////
    // TBD: Modify version 1 SEIs to use the same interfaces as Annex GFI SEI messages.
    //////////////////////////////////////////////////////////////////////////

    case SEI::BUFFERING_PERIOD:                     return new SEIBufferingPeriod;
    case SEI::PICTURE_TIMING:                       return new SEIPictureTiming;
    case SEI::PAN_SCAN_RECT:                        return new SEIPanScanRectangle;                    // not currently decoded
    case SEI::FILLER_PAYLOAD:                       return new SEIFillerPaylod;                       // not currently decoded
    case SEI::USER_DATA_REGISTERED_ITU_T_T35:       return new SEIUserDataRegistered;                 // not currently decoded
    case SEI::USER_DATA_UNREGISTERED:               return new SEIuserDataUnregistered;
    case SEI::RECOVERY_POINT:                       return new SEIRecoveryPoint;
    case SEI::SCENE_INFO:                           return new SEISceneInformation;                    // not currently decoded
    case SEI::FULL_FRAME_SNAPSHOT:                  return new SEIPictureSnapshot;                     // not currently decoded
    case SEI::PROGRESSIVE_REFINEMENT_SEGMENT_START: return new SEIProgressiveRefinementSegmentStart;   // not currently decoded
    case SEI::PROGRESSIVE_REFINEMENT_SEGMENT_END:   return new SEIProgressiveRefinementSegmentEnd;     // not currently decoded
    case SEI::FILM_GRAIN_CHARACTERISTICS:           return new SEIFilmGrainCharacteristics;            // not currently decoded
    case SEI::POST_FILTER_HINT:                     return new SEIPostFilterHint;                      // not currently decoded
    case SEI::TONE_MAPPING_INFO:                    return new SEIToneMappingInfo;
    case SEI::KNEE_FUNCTION_INFO:                   return new SEIKneeFunctionInfo;
    case SEI::FRAME_PACKING:                        return new SEIFramePacking;
    case SEI::DISPLAY_ORIENTATION:                  return new SEIDisplayOrientation;
    case SEI::SOP_DESCRIPTION:                      return new SEISOPDescription;
    case SEI::ACTIVE_PARAMETER_SETS:                return new SEIActiveParameterSets;
    case SEI::DECODING_UNIT_INFO:                   return new SEIDecodingUnitInfo;
    case SEI::TEMPORAL_LEVEL0_INDEX:                return new SEITemporalLevel0Index
    case SEI::DECODED_PICTURE_HASH:                 return new SEIDecodedPictureHash;
    case SEI::SCALABLE_NESTING:                     return new SEIScalableNesting;
    case SEI::REGION_REFRESH_INFO:                  return new SEIRegionRefreshInfo;
    case SEI::NO_DISPLAY:                           return new SEINoDisplay;
    case SEI::TIME_CODE:                            return new SEITimeCode;
    case SEI::MASTERING_DISPLAY_COLOUR_VOLUME:      return new SEIMasteringDisplayColourVolume;
    case SEI::SEGM_RECT_FRAME_PACKING:              return new SEISegmentedRectFramePacking;
    case SEI::TEMP_MOTION_CONSTRAINED_TILE_SETS:    return new SEITempMotionConstrainedTileSets;
    case SEI::CHROMA_SAMPLING_FILTER_HINT:          return new SEIChromaSamplingFilterHint
#endif
  case SEI::LAYERS_NOT_PRESENT                    :               return new SEILayersNotPresent;
  case SEI::INTER_LAYER_CONSTRAINED_TILE_SETS     :               return new SEIInterLayerConstrainedTileSets;
#if NH_MV_SEI_TBD
  case SEI::BSP_NESTING                           :               return new SEIBspNesting;
  case SEI::BSP_INITIAL_ARRIVAL_TIME              :               return new SEIBspInitialArrivalTime;
#endif
  case SEI::SUB_BITSTREAM_PROPERTY                :               return new SEISubBitstreamProperty;
  case SEI::ALPHA_CHANNEL_INFO                    :               return new SEIAlphaChannelInfo;
  case SEI::OVERLAY_INFO                          :               return new SEIOverlayInfo;
  case SEI::TEMPORAL_MV_PREDICTION_CONSTRAINTS    :               return new SEITemporalMvPredictionConstraints;
#if NH_MV_SEI_TBD
  case SEI::FRAME_FIELD_INFO                      :               return new SEIFrameFieldInfo;
#endif
  case SEI::THREE_DIMENSIONAL_REFERENCE_DISPLAYS_INFO:            return new SEIThreeDimensionalReferenceDisplaysInfo;
  case SEI::DEPTH_REPRESENTATION_INFO             :               return new SEIDepthRepresentationInfo;
  case SEI::MULTIVIEW_SCENE_INFO                  :               return new SEIMultiviewSceneInfo;
  case SEI::MULTIVIEW_ACQUISITION_INFO            :               return new SEIMultiviewAcquisitionInfo;
  case SEI::MULTIVIEW_VIEW_POSITION               :               return new SEIMultiviewViewPosition;
  default:                                        assert( false ); return NULL;
  }
}

Void SEI::setupFromSlice  ( const TComSlice* slice )
{
  xPrintCfgErrorIntro();
  std::cout << getSEIMessageString( payloadType() ) << "Setup by the encoder is currently not possible. Using the default SEI cfg-file." << std::endl;
}

SEI* SEI::getCopy() const
{
  assert( 0 );
  return NULL;
}

Void SEI::setupFromCfgFile( const TChar* cfgFile )
{
  assert( false );
}

Bool SEI::insertSei( Int curLayerId, Int curPoc, Int curTid, Int curNaluType ) const
{
  Bool insertSeiHere = true;
  if( !m_applicableLayerIds.empty() )
  {
    insertSeiHere = insertSeiHere && ( std::find( m_applicableLayerIds.begin(), m_applicableLayerIds.end(), curLayerId) != m_applicableLayerIds.end() ) ;
  }
  if( !m_applicablePocs     .empty() )
  {
    insertSeiHere = insertSeiHere && ( std::find( m_applicablePocs    .begin(), m_applicablePocs    .end(), curPoc    ) != m_applicablePocs    .end() ) ;
  }
  if( !m_applicableTids     .empty() )
  {
    insertSeiHere = insertSeiHere && ( std::find( m_applicableTids    .begin(), m_applicableTids    .end(), curTid    ) != m_applicableTids    .end() ) ;
  }
  if( !m_applicableVclNaluTypes.empty() )
  {
    insertSeiHere = insertSeiHere && ( std::find( m_applicableVclNaluTypes.begin(), m_applicableVclNaluTypes.end(), curNaluType) != m_applicableVclNaluTypes.end() ) ;
  }
  return insertSeiHere;
}

Bool SEI::checkCfg( const TComSlice* slice )
{
  assert( false );
  return true;
}

Void SEI::xPrintCfgErrorIntro()
{
  std::cout << "Error in configuration of " << getSEIMessageString( payloadType() ) << " SEI: ";
}

Void SEI::xCheckCfgRange( Bool& wrongConfig, Int val, Int minVal, Int maxVal, const TChar* seName )
{
  if ( val < minVal || val > maxVal  )
  {
    xPrintCfgErrorIntro();
    std::cout << "The value of " << seName << "shall be in the range of " << minVal << " to " << maxVal << ", inclusive." << std::endl;
    wrongConfig = true;
  }
}

Void SEI::xAddGeneralOpts(po::Options &opts, IntAry1d defAppLayerIds, IntAry1d deftApplicablePocs,
                                            IntAry1d defAppTids,     IntAry1d defAppVclNaluTypes,
                                            Int defSeiNaluId, Int defPositionInSeiNalu, Bool defModifyByEncoder)
{
  opts.addOptions()
    ("PayloadType"            , m_payloadType            , -1                    , "Payload Type"                                                         )
    ("ApplicableLayerIds"     , m_applicableLayerIds     , defAppLayerIds        , "LayerIds      of layers   to which the SEI is added. (all when empty)")
    ("ApplicablePocs"         , m_applicablePocs         , deftApplicablePocs    , "POCs          of pictures to which the SEI is added. (all when empty)")
    ("ApplicableTids"         , m_applicableTids         , defAppTids            , "TIds          of pictures to which the SEI is added. (all when empty)")
    ("ApplicableVclNaluTypes" , m_applicableVclNaluTypes , defAppVclNaluTypes    , "NaluUnitTypes of picture  to which the SEI is added. (all when empty)")
    ("SeiNaluId"              , m_seiNaluId              , defSeiNaluId          , "Identifies to which NAL unit  the SEI is added." )
    ("PositionInSeiNalu"      , m_positionInSeiNalu      , defPositionInSeiNalu  , "Identifies the position within the NAL unit.")
    ("ModifyByEncoder"        , m_modifyByEncoder        , defModifyByEncoder    , "0: Use payload as specified in cfg file   1: Modify SEI by encoder");
}

Void SEI::xCheckCfg( Bool& wrongConfig, Bool cond, const TChar* errStr )
{
  if ( !cond  )
  {
    xPrintCfgErrorIntro();
    std::cout << errStr << std::endl;
    wrongConfig = true;
  }
}

Void SEILayersNotPresent::setupFromCfgFile(const TChar* cfgFile)
{
  // Set default values
  IntAry1d defAppLayerIds, defAppPocs, defAppTids, defAppVclNaluTypes;

  // TBD: Add default values for which layers, POCS, Tids or Nalu types the SEI should be send.
  // This SEI message doesn't need to be added by default to any Layer / POC / NAL Unit / T Layer. Not sure if empty is right.
  defAppLayerIds    .empty( );
  defAppPocs        .empty( );
  defAppTids        .empty( );
  defAppVclNaluTypes.empty( );

  Int      defSeiNaluId                  = 0;
  Int      defPositionInSeiNalu          = 0;
  Bool     defModifyByEncoder            = false;   //0: Use payload as specified in cfg file   1: Modify SEI by encoder

  // Setup config file options
  po::Options opts;
  xAddGeneralOpts( opts , defAppLayerIds, defAppPocs, defAppTids, defAppVclNaluTypes, defSeiNaluId, defPositionInSeiNalu, defModifyByEncoder );

  opts.addOptions()
    ("LnpSeiActiveVpsId"              , m_lnpSeiActiveVpsId                , 0                              , "LnpSeiActiveVpsId"                )   //Why?
    ("LayerNotPresentFlag"            , m_layerNotPresentFlag              , BoolAry1d(1,0)                 , "LayerNotPresentFlag"              )
    ;

  po::setDefaults(opts);

  // Parse the cfg file
  po::ErrorReporter err;
  po::parseConfigFile( opts, cfgFile, err );
  m_lnpSeiMaxLayers = (UInt) m_layerNotPresentFlag.size();
};

  Bool SEILayersNotPresent::checkCfg( const TComSlice* slice )
  {
  // Check config values
    Bool wrongConfig = false;
//
    const TComVPS* vps = slice->getVPS();
//  // TBD: Add constraints on presence of SEI here.
    xCheckCfg     ( wrongConfig, m_lnpSeiActiveVpsId == vps->getVPSId(), "The value of lnp_sei_active_vps_id shall be equal to the value of vps_video_parameter_set_id of the active VPS for the VCL NAL units of the access unit containing the SEI message." );
    xCheckCfg     ( wrongConfig, m_lnpSeiMaxLayers == vps->getMaxLayersMinus1(), "The number of LayerNotPresent flags shall be equal to vpsMaxLayersMinus1." );


    for (Int i = 0; i < vps->getMaxLayersMinus1(); i++)
    {
      if ( m_layerNotPresentFlag[ i ] && i < vps->getMaxLayersMinus1() )
      {
        for (Int j = 0; j < vps->getNumPredictedLayers( vps->getLayerIdInNuh( j ) - 1 ); j++ )
        {
          xCheckCfg     ( wrongConfig, m_layerNotPresentFlag[ vps->getLayerIdInVps( vps->getIdPredictedLayer( vps->getLayerIdInNuh(i),j) )], "When layer_not_present_flag[ i ] is equal to 1 and i is less than MaxLayersMinus1, layer_not_present_flag[ LayerIdxInVps[ IdPredictedLayer[ layer_id_in_nuh[ i ] ][ j ] ] ] shall be equal to 1 for all values of j in the range of 0 to NumPredictedLayers[ layer_id_in_nuh[ i ] ] - 1, inclusive." );
        }
      }
    }

      return wrongConfig;
  };



Void SEIInterLayerConstrainedTileSets::setupFromCfgFile(const TChar* cfgFile)
{
  // Set default values
  IntAry1d defAppLayerIds, defAppPocs, defAppTids, defAppVclNaluTypes;

  // TBD: Add default values for which layers, POCS, Tids or Nalu types the SEI should be send.
  defAppLayerIds    .empty( );
  defAppPocs        .push_back( 0 );
  defAppTids        .empty( );
  defAppVclNaluTypes.empty( );

  Int      defSeiNaluId                  = 0;
  Int      defPositionInSeiNalu          = 0;
  Bool     defModifyByEncoder            = false;

  // Setup config file options
  po::Options opts;
  xAddGeneralOpts( opts , defAppLayerIds, defAppPocs, defAppTids, defAppVclNaluTypes, defSeiNaluId, defPositionInSeiNalu, defModifyByEncoder );

  const Int maxNumTileInSet = 100;

  opts.addOptions()
    ("IlAllTilesExactSampleValueMatchFlag", m_ilAllTilesExactSampleValueMatchFlag, false                    , "IlAllTilesExactSampleValueMatchFlag")
    ("IlOneTilePerTileSetFlag"        , m_ilOneTilePerTileSetFlag          , false                          , "IlOneTilePerTileSetFlag"          )
    ("IlNumSetsInMessageMinus1"       , m_ilNumSetsInMessageMinus1         , 0                              , "IlNumSetsInMessageMinus1"         )
    ("SkippedTileSetPresentFlag"      , m_skippedTileSetPresentFlag        , false                          , "SkippedTileSetPresentFlag"        )
    ("IlctsId"                        , m_ilctsId                          , IntAry1d (256,0)               , "IlctsId"                          )
    ("IlNumTileRectsInSetMinus1"      , m_ilNumTileRectsInSetMinus1        , IntAry1d (256,0)               , "IlNumTileRectsInSetMinus1"        )
    ("IlTopLeftTileIndex_%d"          , m_ilTopLeftTileIndex               , IntAry1d (maxNumTileInSet,0), 256, "IlTopLeftTileIndex"               )
    ("IlBottomRightTileIndex_%d"      , m_ilBottomRightTileIndex           , IntAry1d (maxNumTileInSet,0), 256, "IlBottomRightTileIndex"           )
    ("IlcIdc"                         , m_ilcIdc                           , IntAry1d (256,0)               , "IlcIdc"                           )
    ("IlExactSampleValueMatchFlag"    , m_ilExactSampleValueMatchFlag      , BoolAry1d(256,0)               , "IlExactSampleValueMatchFlag"      )
    ("AllTilesIlcIdc"                 , m_allTilesIlcIdc                   , 0                              , "AllTilesIlcIdc"                   )
    ;

  po::setDefaults(opts);

  // Parse the cfg file
  po::ErrorReporter err;
  po::parseConfigFile( opts, cfgFile, err );
};

Bool SEIInterLayerConstrainedTileSets::checkCfg( const TComSlice* slice )
{
  // Check config values
  Bool wrongConfig = false;
  const TComPPS* pps = slice->getPPS();

  // Currently only the active PPS checked.
  xCheckCfg     ( wrongConfig, pps->getTilesEnabledFlag() , "The inter-layer constrained tile sets SEI message shall not be present for the layer with nuh_layer_id equal to targetLayerId when tiles_enabled_flag is equal to 0 for any PPS that is active for the pictures of the CLVS of the layer with nuh_layer_id equal to targetLayerId." );

  if ( m_ilOneTilePerTileSetFlag )
  {
    xCheckCfg( wrongConfig, ( pps->getNumTileColumnsMinus1() + 1 ) *  ( pps->getNumTileRowsMinus1() + 1 ) <= 256, "It is a requirement of bitstream conformance that when il_one_tile_per_tile_set_flag is equal to 1, the value of ( num_tile_columns_minus1 + 1 ) * ( num_tile_rows_minus1 + 1 ) shall be less than or equal to 256."    );
  }
  Int numSignificantSets = m_ilNumSetsInMessageMinus1 - m_skippedTileSetPresentFlag + 1;

  for (Int i = 0 ; i < numSignificantSets; i++)
  {
    xCheckCfgRange( wrongConfig, m_ilctsId[i]                         , 0 , (1 << 30) - 1, "ilcts_id"                         );
  }

  return wrongConfig;
};
#if NH_MV_SEI_TBD

Void SEIBspNesting::setupFromSlice  ( const TComSlice* slice )
{
  sei.m_seiOlsIdx =  TBD ;
  sei.m_seiPartitioningSchemeIdx =  TBD ;
  sei.m_bspIdx =  TBD ;
  while( !ByteaLigned(() ) );
  {
    sei.m_bspNestingZeroBit =  TBD ;
  }
  sei.m_numSeisInBspMinus1 =  TBD ;
  for( Int i = 0; i  <=  NumSeisInBspMinus1( ); i++ )
  {
    SeiMessage(() );
  }
};

Void SEIBspNesting::setupFromCfgFile(const TChar* cfgFile)
{
  // Set default values
  IntAry1d defAppLayerIds, defAppPocs, defAppTids, defAppVclNaluTypes;

  // TBD: Add default values for which layers, POCS, Tids or Nalu types the SEI should be send.
  defAppLayerIds    .push_back( TBD );
  defAppPocs        .push_back( TBD );
  defAppTids        .push_back( TBD );
  defAppVclNaluTypes.push_back( TBD );

  Int      defSeiNaluId                  = 0;
  Int      defPositionInSeiNalu          = 0;
  Bool     defModifyByEncoder            = TBD;

  // Setup config file options
  po::Options opts;
  xAddGeneralOpts( opts , defAppLayerIds, defAppPocs, defAppTids, defAppVclNaluTypes, defSeiNaluId, defPositionInSeiNalu, defModifyByEncoder );

  opts.addOptions()
    ("SeiOlsIdx"                      , m_seiOlsIdx                        , 0                              , "SeiOlsIdx"                        )
    ("SeiPartitioningSchemeIdx"       , m_seiPartitioningSchemeIdx         , 0                              , "SeiPartitioningSchemeIdx"         )
    ("BspIdx"                         , m_bspIdx                           , 0                              , "BspIdx"                           )
    ("BspNestingZeroBit"              , m_bspNestingZeroBit                , 0                              , "BspNestingZeroBit"                )
    ("NumSeisInBspMinus1"             , m_numSeisInBspMinus1               , 0                              , "NumSeisInBspMinus1"               )
    ;

  po::setDefaults(opts);

  // Parse the cfg file
  po::ErrorReporter err;
  po::parseConfigFile( opts, cfgFile, err );
};

Bool SEIBspNesting::checkCfg( const TComSlice* slice )
{
  // Check config values
  Bool wrongConfig = false;

  // TBD: Add constraints on presence of SEI here.
  xCheckCfg     ( wrongConfig, TBD , "TBD" );
  xCheckCfg     ( wrongConfig, TBD , "TBD" );

  // TBD: Modify constraints according to the SEI semantics.
  xCheckCfgRange( wrongConfig, m_seiOlsIdx                      , MINVAL , MAXVAL, "sei_ols_idx"          );
  xCheckCfgRange( wrongConfig, m_seiPartitioningSchemeIdx       , MINVAL , MAXVAL, "sei_partitioning_scheme_idx"       );
  xCheckCfgRange( wrongConfig, m_bspIdx                         , MINVAL , MAXVAL, "bsp_idx"              );
  xCheckCfgRange( wrongConfig, m_bspNestingZeroBit              , MINVAL , MAXVAL, "bsp_nesting_zero_bit ");
  xCheckCfgRange( wrongConfig, m_numSeisInBspMinus1             , MINVAL , MAXVAL, "num_seis_in_bsp_minus1"           );

  return wrongConfig;

};

Void SEIBspInitialArrivalTime::setupFromSlice  ( const TComSlice* slice )
{
  psIdx = SeiPartitioningSchemeIdx();
  if( nalInitialArrivalDelayPresent )
  {
    for( Int i = 0; i < BspSchedCnt( SeiOlsIdx(), psIdx, MaxTemporalId( 0 ) ); i++ )
    {
      sei.m_nalInitialArrivalDelay[i] =  TBD ;
    }
  }
  if( vclInitialArrivalDelayPresent )
  {
    for( Int i = 0; i < BspSchedCnt( SeiOlsIdx(), psIdx, MaxTemporalId( 0 ) ); i++ )
    {
      sei.m_vclInitialArrivalDelay[i] =  TBD ;
    }
  }
};

Void SEIBspInitialArrivalTime::setupFromCfgFile(const TChar* cfgFile)
{
  // Set default values
  IntAry1d defAppLayerIds, defAppPocs, defAppTids, defAppVclNaluTypes;

  // TBD: Add default values for which layers, POCS, Tids or Nalu types the SEI should be send.
  defAppLayerIds    .push_back( TBD );
  defAppPocs        .push_back( TBD );
  defAppTids        .push_back( TBD );
  defAppVclNaluTypes.push_back( TBD );

  Int      defSeiNaluId                  = 0;
  Int      defPositionInSeiNalu          = 0;
  Bool     defModifyByEncoder            = TBD;

  // Setup config file options
  po::Options opts;
  xAddGeneralOpts( opts , defAppLayerIds, defAppPocs, defAppTids, defAppVclNaluTypes, defSeiNaluId, defPositionInSeiNalu, defModifyByEncoder );

  opts.addOptions()
    ("NalInitialArrivalDelay"         , m_nalInitialArrivalDelay           , IntAry1d (1,0)                 , "NalInitialArrivalDelay"           )
    ("VclInitialArrivalDelay"         , m_vclInitialArrivalDelay           , IntAry1d (1,0)                 , "VclInitialArrivalDelay"           )
    ;

  po::setDefaults(opts);

  // Parse the cfg file
  po::ErrorReporter err;
  po::parseConfigFile( opts, cfgFile, err );
};

Bool SEIBspInitialArrivalTime::checkCfg( const TComSlice* slice )
{
  // Check config values
  Bool wrongConfig = false;

  // TBD: Add constraints on presence of SEI here.
  xCheckCfg     ( wrongConfig, TBD , "TBD" );
  xCheckCfg     ( wrongConfig, TBD , "TBD" );

  // TBD: Modify constraints according to the SEI semantics.
  xCheckCfgRange( wrongConfig, m_nalInitialArrivalDelay[i]      , MINVAL , MAXVAL, "nal_initial_arrival_delay"        );
  xCheckCfgRange( wrongConfig, m_vclInitialArrivalDelay[i]      , MINVAL , MAXVAL, "vcl_initial_arrival_delay"        );

  return wrongConfig;

};
#endif

Void SEISubBitstreamProperty::setupFromCfgFile(const TChar* cfgFile)
{
  // Set default values
  IntAry1d defAppLayerIds, defAppPocs, defAppTids, defAppVclNaluTypes;

  // TBD: Add default values for which layers, POCS, Tids or Nalu types the SEI should be send.
  defAppLayerIds    .push_back( 0 );
  defAppPocs        .push_back( 0 );
  defAppTids        .push_back( 0 );
  defAppVclNaluTypes = IRAP_NAL_UNIT_TYPES;

  Int      defSeiNaluId                  = 0;
  Int      defPositionInSeiNalu          = 0;
  Bool     defModifyByEncoder            = false;

  // Setup config file options
  po::Options opts;
  xAddGeneralOpts( opts , defAppLayerIds, defAppPocs, defAppTids, defAppVclNaluTypes, defSeiNaluId, defPositionInSeiNalu, defModifyByEncoder );

  opts.addOptions()
    ("SbPropertyActiveVpsId"          , m_sbPropertyActiveVpsId            , 0                              , "SbPropertyActiveVpsId"            )
    ("NumAdditionalSubStreamsMinus1"  , m_numAdditionalSubStreamsMinus1    , 0                              , "NumAdditionalSubStreamsMinus1"    )
    ("SubBitstreamMode"               , m_subBitstreamMode                 , IntAry1d (1,0)                 , "SubBitstreamMode"                 )
    ("OlsIdxToVps"                    , m_olsIdxToVps                      , IntAry1d (1,0)                 , "OlsIdxToVps"                      )
    ("HighestSublayerId"              , m_highestSublayerId                , IntAry1d (1,0)                 , "HighestSublayerId"                )
    ("AvgSbPropertyBitRate"           , m_avgSbPropertyBitRate             , IntAry1d (1,0)                 , "AvgSbPropertyBitRate"             )
    ("MaxSbPropertyBitRate"           , m_maxSbPropertyBitRate             , IntAry1d (1,0)                 , "MaxSbPropertyBitRate"             )
    ;

  po::setDefaults(opts);

  // Parse the cfg file
  po::ErrorReporter err;
  po::parseConfigFile( opts, cfgFile, err );
};

Bool SEISubBitstreamProperty::checkCfg( const TComSlice* slice )
{
  // Check config values
  Bool wrongConfig = false;

  // For the current encoder, the initial IRAP access unit has always POC zero.
  xCheckCfg     ( wrongConfig, slice->isIRAP() && (slice->getPOC() == 0), "When present, the sub-bitstream property SEI message shall be associated with an initial IRAP access unit and the information provided by the SEI messages applies to the bitstream corresponding to the CVS containing the associated initial IRAP access unit.");

  Bool sizeNotCorrect =
    (    ( m_numAdditionalSubStreamsMinus1 + 1 ) != m_subBitstreamMode    .size() )
    || ( ( m_numAdditionalSubStreamsMinus1 + 1 ) != m_olsIdxToVps         .size() )
    || ( ( m_numAdditionalSubStreamsMinus1 + 1 ) != m_highestSublayerId   .size() )
    || ( ( m_numAdditionalSubStreamsMinus1 + 1 ) != m_avgSbPropertyBitRate.size() )
    || ( ( m_numAdditionalSubStreamsMinus1 + 1 ) != m_maxSbPropertyBitRate.size() );

  xCheckCfg( wrongConfig, !sizeNotCorrect, "Some parameters of some sub-bitstream not provided." );
  xCheckCfg( wrongConfig, slice->getVPS()->getVPSId() == m_sbPropertyActiveVpsId, "The value of sb_property_active_vps_id shall be equal to the value of vps_video_parameter_set_id of the active VPS referred to by the VCL NAL units of the associated access unit." );

  xCheckCfgRange( wrongConfig, m_numAdditionalSubStreamsMinus1  , 0 , (1 << 10) - 1 , "num_additional_sub_streams_minus1");

  if ( !sizeNotCorrect )
  {
    for (Int i = 0; i <= m_numAdditionalSubStreamsMinus1; i++ )
    {
      xCheckCfgRange( wrongConfig, m_subBitstreamMode[i]    , 0 , 1                                          , "sub_bitstream_mode" );
      xCheckCfgRange( wrongConfig, m_olsIdxToVps[i]         , 0 , slice->getVPS()->getNumOutputLayerSets()-1 , "ols_idx_to_vps"     );
    }
  }
  return wrongConfig;
};

Void SEISubBitstreamProperty::resizeArrays( )
{
  m_subBitstreamMode    .resize( m_numAdditionalSubStreamsMinus1 + 1);
  m_olsIdxToVps         .resize( m_numAdditionalSubStreamsMinus1 + 1);
  m_highestSublayerId   .resize( m_numAdditionalSubStreamsMinus1 + 1);
  m_avgSbPropertyBitRate.resize( m_numAdditionalSubStreamsMinus1 + 1);
  m_maxSbPropertyBitRate.resize( m_numAdditionalSubStreamsMinus1 + 1);
}

Void SEIAlphaChannelInfo::setupFromCfgFile(const TChar* cfgFile)
{
  // Set default values
  IntAry1d defAppLayerIds, defAppPocs, defAppTids, defAppVclNaluTypes;

  // TBD: Add default values for which layers, POCS, Tids or Nalu types the SEI should be send.
  defAppLayerIds    .clear();
  defAppPocs        .push_back( 0 );
  defAppTids        .push_back( 0 );
  defAppVclNaluTypes = IDR_NAL_UNIT_TYPES;

  Int      defSeiNaluId                  = 0;
  Int      defPositionInSeiNalu          = 0;
  Bool     defModifyByEncoder            = false;

  // Setup config file options
  po::Options opts;
  xAddGeneralOpts( opts , defAppLayerIds, defAppPocs, defAppTids, defAppVclNaluTypes, defSeiNaluId, defPositionInSeiNalu, defModifyByEncoder );

  opts.addOptions()
    ("AlphaChannelCancelFlag"         , m_alphaChannelCancelFlag           , false                          , "AlphaChannelCancelFlag"           )
    ("AlphaChannelUseIdc"             , m_alphaChannelUseIdc               , 0                              , "AlphaChannelUseIdc"               )
    ("AlphaChannelBitDepthMinus8"     , m_alphaChannelBitDepthMinus8       , 0                              , "AlphaChannelBitDepthMinus8"       )
    ("AlphaTransparentValue"          , m_alphaTransparentValue            , 0                              , "AlphaTransparentValue"            )
    ("AlphaOpaqueValue"               , m_alphaOpaqueValue                 , 255                            , "AlphaOpaqueValue"                 )
    ("AlphaChannelIncrFlag"           , m_alphaChannelIncrFlag             , false                          , "AlphaChannelIncrFlag"             )
    ("AlphaChannelClipFlag"           , m_alphaChannelClipFlag             , false                          , "AlphaChannelClipFlag"             )
    ("AlphaChannelClipTypeFlag"       , m_alphaChannelClipTypeFlag         , false                          , "AlphaChannelClipTypeFlag"         )
    ;

  po::setDefaults(opts);

  // Parse the cfg file
  po::ErrorReporter err;
  po::parseConfigFile( opts, cfgFile, err );

};

Bool SEIAlphaChannelInfo::checkCfg( const TComSlice* slice )
{
  // Check config values
  Bool wrongConfig = false;

  int maxInterpretationValue = (1 << (m_alphaChannelBitDepthMinus8+9)) - 1;
  xCheckCfgRange( wrongConfig, m_alphaChannelCancelFlag         , 0 , 1, "alpha_channel_cancel_flag"        );
  xCheckCfgRange( wrongConfig, m_alphaChannelUseIdc             , 0 , 7, "alpha_channel_use_idc");
  xCheckCfgRange( wrongConfig, m_alphaChannelBitDepthMinus8     , 0 , 7, "alpha_channel_bit_depth_minus8"   );
  xCheckCfgRange( wrongConfig, m_alphaTransparentValue          , 0 , maxInterpretationValue, "alpha_transparent_value"          );
  xCheckCfgRange( wrongConfig, m_alphaOpaqueValue               , 0 , maxInterpretationValue, "alpha_opaque_value"   );
  xCheckCfgRange( wrongConfig, m_alphaChannelIncrFlag           , 0 , 1, "alpha_channel_incr_flag"          );
  xCheckCfgRange( wrongConfig, m_alphaChannelClipFlag           , 0 , 1, "alpha_channel_clip_flag"          );
  xCheckCfgRange( wrongConfig, m_alphaChannelClipTypeFlag       , 0 , 1, "alpha_channel_clip_type_flag"     );

  return wrongConfig;

};

SEIOverlayInfo::SEIOverlayInfo ( )
  : m_numOverlaysMax(16)
  , m_numOverlayElementsMax(256)
  , m_numStringBytesMax(256) //incl. null termination byte
{ };

Void SEIOverlayInfo::setupFromCfgFile(const TChar* cfgFile)
{
  // Set default values
  IntAry1d defAppLayerIds, defAppPocs, defAppTids, defAppVclNaluTypes;

  // Add default values for which layers, POCS, Tids or Nalu types the SEI should be send.
  defAppLayerIds    .clear();
  defAppPocs        .push_back( 0 );
  defAppTids        .push_back( 0 );
  defAppVclNaluTypes = IDR_NAL_UNIT_TYPES;

  Int      defSeiNaluId                  = 0;
  Int      defPositionInSeiNalu          = 0;
  Bool     defModifyByEncoder            = false;

  // Setup config file options
  po::Options opts;
  xAddGeneralOpts( opts , defAppLayerIds, defAppPocs, defAppTids, defAppVclNaluTypes, defSeiNaluId, defPositionInSeiNalu, defModifyByEncoder );

  opts.addOptions()
    ("OverlayInfoCancelFlag"          , m_overlayInfoCancelFlag            , false                           , "OverlayInfoCancelFlag"            )
    ("OverlayContentAuxIdMinus128"    , m_overlayContentAuxIdMinus128      , 0                               , "OverlayContentAuxIdMinus128"      )
    ("OverlayLabelAuxIdMinus128"      , m_overlayLabelAuxIdMinus128        , 0                               , "OverlayLabelAuxIdMinus128"        )
    ("OverlayAlphaAuxIdMinus128"      , m_overlayAlphaAuxIdMinus128        , 0                               , "OverlayAlphaAuxIdMinus128"        )
    ("OverlayElementLabelValueLengthMinus8", m_overlayElementLabelValueLengthMinus8, 0                       , "OverlayElementLabelValueLengthMinus8")
    ("NumOverlaysMinus1"              , m_numOverlaysMinus1                , 0                               , "NumOverlaysMinus1"                )
    ("OverlayIdx"                     , m_overlayIdx                       , IntAry1d (16,0)                 , "OverlayIdx"                       )
    ("LanguageOverlayPresentFlag"     , m_languageOverlayPresentFlag       , BoolAry1d(16,0)                 , "LanguageOverlayPresentFlag"       )
    ("OverlayContentLayerId"          , m_overlayContentLayerId            , IntAry1d (16,0)                 , "OverlayContentLayerId"            )
    ("OverlayLabelPresentFlag"        , m_overlayLabelPresentFlag          , BoolAry1d(16,0)                 , "OverlayLabelPresentFlag"          )
    ("OverlayLabelLayerId"            , m_overlayLabelLayerId              , IntAry1d (16,0)                 , "OverlayLabelLayerId"              )
    ("OverlayAlphaPresentFlag"        , m_overlayAlphaPresentFlag          , BoolAry1d(16,0)                 , "OverlayAlphaPresentFlag"          )
    ("OverlayAlphaLayerId"            , m_overlayAlphaLayerId              , IntAry1d (16,0)                 , "OverlayAlphaLayerId"              )
    ("NumOverlayElementsMinus1"       , m_numOverlayElementsMinus1         , IntAry1d (16,0)                 , "NumOverlayElementsMinus1"         )
    ("OverlayElementLabelMin_%d"      , m_overlayElementLabelMin           , IntAry1d (256,0) ,16            , "OverlayElementLabelMin"           )
    ("OverlayElementLabelMax_%d"      , m_overlayElementLabelMax           , IntAry1d (256,0) ,16            , "OverlayElementLabelMax"           )
    ("OverlayLanguage_%d"             , m_overlayLanguage                  , std::string(""), 16             , "OverlayLanguage"                  )
    ("OverlayName_%d"                 , m_overlayName                      , std::string(""), 16             , "OverlayName"                      )
    ("OverlayElementName_%d_%d"       , m_overlayElementName               , std::string(""), 256 ,16        , "OverlayElementName"               )
    ("OverlayInfoPersistenceFlag"     , m_overlayInfoPersistenceFlag       , false                           , "OverlayInfoPersistenceFlag"       )
    ;

  po::setDefaults(opts);

  // Parse the cfg file
  po::ErrorReporter err;
  po::parseConfigFile( opts, cfgFile, err );
};


Bool SEIOverlayInfo::checkCfg( const TComSlice* slice )
{
  // Check config values
  Bool wrongConfig = false;

  xCheckCfgRange( wrongConfig, m_overlayInfoCancelFlag          , 0 ,   1, "overlay_info_cancel_flag"         );
  xCheckCfgRange( wrongConfig, m_overlayContentAuxIdMinus128    , 0 ,  31, "overlay_content_aux_id_minus128"  );
  xCheckCfgRange( wrongConfig, m_overlayLabelAuxIdMinus128      , 0 ,  31, "overlay_label_aux_id_minus128"    );
  xCheckCfgRange( wrongConfig, m_overlayAlphaAuxIdMinus128      , 0 ,  31, "overlay_alpha_aux_id_minus128"    );
  xCheckCfgRange( wrongConfig, m_numOverlaysMinus1              , 0 ,  m_numOverlaysMax-1, "num_overlays_minus1"  );
  for (Int i=0 ; i<=m_numOverlaysMinus1 ; ++i)
  {
    xCheckCfgRange( wrongConfig, m_overlayIdx[i]                  , 0 , 255, "overlay_idx"          );
    xCheckCfgRange( wrongConfig, m_languageOverlayPresentFlag[i]  , 0 ,   1, "language_overlay_present_flag"    );
    xCheckCfgRange( wrongConfig, m_overlayLabelPresentFlag[i]     , 0 ,   1, "overlay_label_present_flag"       );
    xCheckCfgRange( wrongConfig, m_overlayAlphaPresentFlag[i]     , 0 ,   1, "overlay_alpha_present_flag"       );
    xCheckCfgRange( wrongConfig, m_overlayContentLayerId[i]       , 0 ,   63, "overlay_content_layer_id"    );
    xCheckCfgRange( wrongConfig, m_overlayLabelLayerId[i]         , 0 ,   63, "overlay_label_layer_id"    );
    xCheckCfgRange( wrongConfig, m_overlayAlphaLayerId[i]         , 0 ,   63, "overlay_alpha_layer_id"    );
    xCheckCfgRange( wrongConfig, m_numOverlayElementsMinus1[i]    , 0 , m_numOverlayElementsMax-1, "num_overlay_elements_minus1"       );
    for (Int j=0 ; j<=m_numOverlayElementsMinus1[i] ; ++j)
    {
      Int maxLabelMinMaxValue = ( 1 << ( m_overlayElementLabelValueLengthMinus8 + 8 ) )-1;
      xCheckCfgRange( wrongConfig, m_overlayElementLabelMin[i][j] , 0 , maxLabelMinMaxValue , "overlay_element_label_min"    );
      xCheckCfgRange( wrongConfig, m_overlayElementLabelMax[i][j] , 0 , maxLabelMinMaxValue , "overlay_element_label_max"    );
    }
  }
  xCheckCfgRange( wrongConfig, m_overlayInfoPersistenceFlag     , 0 ,   1, "overlay_info_persistence_flag"    );

  return wrongConfig;

};


Void SEITemporalMvPredictionConstraints::setupFromCfgFile(const TChar* cfgFile)
{
  // Set default values
  IntAry1d defAppLayerIds, defAppPocs, defAppTids, defAppVclNaluTypes;

  // TBD: Add default values for which layers, POCS, Tids or Nalu types the SEI should be send.
  defAppLayerIds    .clear    (   );
  defAppPocs        .push_back( 0 );
  defAppTids        .push_back( 0 );
  defAppVclNaluTypes.clear    (   );

  Int      defSeiNaluId                  = 0;
  Int      defPositionInSeiNalu          = 0;
  Bool     defModifyByEncoder            = false;

  // Setup config file options
  po::Options opts;
  xAddGeneralOpts( opts , defAppLayerIds, defAppPocs, defAppTids, defAppVclNaluTypes, defSeiNaluId, defPositionInSeiNalu, defModifyByEncoder );

  opts.addOptions()
    ("PrevPicsNotUsedFlag"   , m_prevPicsNotUsedFlag   , false, "PrevPicsNotUsedFlag"    )
    ("NoIntraLayerColPicFlag", m_noIntraLayerColPicFlag, false, "NoIntraLayerColPicFlag" )
    ;

  po::setDefaults(opts);

  // Parse the cfg file
  po::ErrorReporter err;
  po::parseConfigFile( opts, cfgFile, err );
};

Bool SEITemporalMvPredictionConstraints::checkCfg( const TComSlice* slice )
{
  // Check config values
  Bool wrongConfig = false;

  xCheckCfg     ( wrongConfig, slice->getTemporalId() == 0 , "The temporal motion vector prediction constraints SEI message may be present in an access unit with TemporalId equal to 0 and shall not be present in an access unit with TemporalId greater than 0." );

  return wrongConfig;
};

#if NH_MV_SEI_TBD
Void SEIFrameFieldInfo::setupFromCfgFile(const TChar* cfgFile)
{
  // Set default values
  IntAry1d defAppLayerIds, defAppPocs, defAppTids, defAppVclNaluTypes;

  // TBD: Add default values for which layers, POCS, Tids or Nalu types the SEI should be send.
  defAppLayerIds    .push_back( TBD );
  defAppPocs        .push_back( TBD );
  defAppTids        .push_back( TBD );
  defAppVclNaluTypes.push_back( TBD );

  Int      defSeiNaluId                  = 0;
  Int      defPositionInSeiNalu          = 0;
  Bool     defModifyByEncoder            = false;

  // Setup config file options
  po::Options opts;
  xAddGeneralOpts( opts , defAppLayerIds, defAppPocs, defAppTids, defAppVclNaluTypes, defSeiNaluId, defPositionInSeiNalu, defModifyByEncoder );

  opts.addOptions()
    ("FfinfoPicStruct"     , m_ffinfoPicStruct     , 0     , "FfinfoPicStruct"     )
    ("FfinfoSourceScanType", m_ffinfoSourceScanType, 0     , "FfinfoSourceScanType")
    ("FfinfoDuplicateFlag" , m_ffinfoDuplicateFlag , false , "FfinfoDuplicateFlag" )
    ;

  po::setDefaults(opts);

  // Parse the cfg file
  po::ErrorReporter err;
  po::parseConfigFile( opts, cfgFile, err );
};


Bool SEIFrameFieldInfo::checkCfg( const TComSlice* slice )
{
  // Check config values
  Bool wrongConfig = false;

  // TBD: Add constraints on presence of SEI here.
  xCheckCfg     ( wrongConfig, TBD , "TBD" );
  xCheckCfg     ( wrongConfig, TBD , "TBD" );

  // TBD: Modify constraints according to the SEI semantics.
  xCheckCfgRange( wrongConfig, m_ffinfoPicStruct                , MINVAL , MAXVAL, "ffinfo_pic_struct"                );
  xCheckCfgRange( wrongConfig, m_ffinfoSourceScanType           , MINVAL , MAXVAL, "ffinfo_source_scan_type"          );
  xCheckCfgRange( wrongConfig, m_ffinfoDuplicateFlag            , MINVAL , MAXVAL, "ffinfo_duplicate_flag"            );

  return wrongConfig;

};
#endif

Void SEIThreeDimensionalReferenceDisplaysInfo::setupFromCfgFile(const TChar* cfgFile)
{
  // Set default values
  IntAry1d defAppLayerIds, defAppPocs, defAppTids, defAppVclNaluTypes;

  // Default values for which layers, POCS, Tids or Nalu types the SEI should be sent.
  defAppLayerIds      .push_back( 0 );
  defAppPocs          .push_back( 0 );
  defAppTids          .push_back( 0 );
  defAppVclNaluTypes = IRAP_NAL_UNIT_TYPES;

  Int      defSeiNaluId                  = 0;
  Int      defPositionInSeiNalu          = 0;
  Bool     defModifyByEncoder            = 0;

  // Setup config file options
  po::Options opts;
  xAddGeneralOpts( opts , defAppLayerIds, defAppPocs, defAppTids, defAppVclNaluTypes, defSeiNaluId, defPositionInSeiNalu, defModifyByEncoder );

  opts.addOptions()
    ("PrecRefDisplayWidth"            , m_precRefDisplayWidth              , 0                              , "PrecRefDisplayWidth"              )
    ("RefViewingDistanceFlag"         , m_refViewingDistanceFlag           , false                          , "RefViewingDistanceFlag"           )
    ("PrecRefViewingDist"             , m_precRefViewingDist               , 0                              , "PrecRefViewingDist"               )
    ("NumRefDisplaysMinus1"           , m_numRefDisplaysMinus1             , 0                              , "NumRefDisplaysMinus1"             )
    ("LeftViewId"                     , m_leftViewId                       , IntAry1d (1,0)                 , "LeftViewId"                       )
    ("RightViewId"                    , m_rightViewId                      , IntAry1d (1,0)                 , "RightViewId"                      )
    ("ExponentRefDisplayWidth"        , m_exponentRefDisplayWidth          , IntAry1d (1,0)                 , "ExponentRefDisplayWidth"          )
    ("MantissaRefDisplayWidth"        , m_mantissaRefDisplayWidth          , IntAry1d (1,0)                 , "MantissaRefDisplayWidth"          )
    ("ExponentRefViewingDistance"     , m_exponentRefViewingDistance       , IntAry1d (1,0)                 , "ExponentRefViewingDistance"       )
    ("MantissaRefViewingDistance"     , m_mantissaRefViewingDistance       , IntAry1d (1,0)                 , "MantissaRefViewingDistance"       )
    ("AdditionalShiftPresentFlag"     , m_additionalShiftPresentFlag       , BoolAry1d(1,0)                 , "AdditionalShiftPresentFlag"       )
    ("NumSampleShiftPlus512"          , m_numSampleShiftPlus512            , IntAry1d (1,0)                 , "NumSampleShiftPlus512"            )
    ("ThreeDimensionalReferenceDisplaysExtensionFlag", m_threeDimensionalReferenceDisplaysExtensionFlag, false                          , "ThreeDimensionalReferenceDisplaysExtensionFlag")
    ;

  po::setDefaults(opts);

  // Parse the cfg file
  po::ErrorReporter err;
  po::parseConfigFile( opts, cfgFile, err );
};


UInt SEIThreeDimensionalReferenceDisplaysInfo::getMantissaReferenceDisplayWidthLen( Int i ) const
{
  return xGetSyntaxElementLen( m_exponentRefDisplayWidth[i], m_precRefDisplayWidth, m_mantissaRefDisplayWidth[ i ] );
}

UInt SEIThreeDimensionalReferenceDisplaysInfo::getMantissaReferenceViewingDistanceLen( Int i ) const
{
  return xGetSyntaxElementLen( m_exponentRefViewingDistance[i], m_precRefViewingDist, m_mantissaRefViewingDistance[ i ] );
}

UInt SEIThreeDimensionalReferenceDisplaysInfo::xGetSyntaxElementLen( Int expo, Int prec, Int val ) const
{
  UInt len;
  if( expo == 0 )
  {
    len = std::max(0, prec - 30 );
  }
  else
  {
    len = std::max( 0, expo + prec - 31 );
  }

  assert( val >= 0 );
  assert( val <= ( ( 1 << len )- 1) );
  return len;
}

Bool SEIThreeDimensionalReferenceDisplaysInfo::checkCfg( const TComSlice* slice )
{
  // Check config values
  Bool wrongConfig = false;

  // The 3D reference display SEI should preferably be sent along with the multiview acquisition SEI. For now the multiview acquisition SEI is restricted to POC = 0, so 3D reference displays SEI is restricted to POC = 0 as well.
  xCheckCfg     ( wrongConfig, slice->isIRAP() && (slice->getPOC() == 0)  , "The 3D reference displays SEI message currently is associated with an access unit that contains an IRAP picture." );

  xCheckCfgRange( wrongConfig, m_precRefDisplayWidth            , 0 , 31, "prec_ref_display_width"  );
  xCheckCfgRange( wrongConfig, m_refViewingDistanceFlag         , 0 , 1, "ref_viewing_distance_flag");
  xCheckCfgRange( wrongConfig, m_precRefViewingDist             , 0 , 31, "prec_ref_viewing_dist"   );
  xCheckCfgRange( wrongConfig, m_numRefDisplaysMinus1           , 0 , 31, "num_ref_displays_minus1" );

  for (Int i = 0; i <= getNumRefDisplaysMinus1(); i++ )
  {
    xCheckCfgRange( wrongConfig, m_exponentRefDisplayWidth[i]     , 0 , 62, "exponent_ref_display_width"   );
    xCheckCfgRange( wrongConfig, m_exponentRefViewingDistance[i]  , 0 , 62, "exponent_ref_viewing_distance");
    xCheckCfgRange( wrongConfig, m_additionalShiftPresentFlag[i]  , 0 , 1, "additional_shift_present_flag" );
    xCheckCfgRange( wrongConfig, m_numSampleShiftPlus512[i]       , 0 , 1023, "num_sample_shift_plus512"   );
  }
  xCheckCfgRange( wrongConfig, m_threeDimensionalReferenceDisplaysExtensionFlag, 0 , 1, "three_dimensional_reference_displays_extension_flag");

  return wrongConfig;

};

Void SEIDepthRepresentationInfo::setupFromSlice  ( const TComSlice* slice )
{

    m_currLayerID=slice->getLayerIdInVps();
};

Void SEIDepthRepresentationInfo::setupFromCfgFile(const TChar* cfgFile)
{
  // Set default values
  IntAry1d defAppLayerIds, defAppPocs, defAppTids, defAppVclNaluTypes;

  // TBD: Add default values for which layers, POCS, Tids or Nalu types the SEI should be send.
  //defAppLayerIds    .push_back( TBD );
  defAppPocs        .push_back( 0 );
  //defAppTids        .push_back( TBD );
  //defAppVclNaluTypes.push_back( TBD );

  Int      defSeiNaluId                  = 0;
  Int      defPositionInSeiNalu          = 0;
  Bool     defModifyByEncoder            = true;

  // Setup config file options
  po::Options opts;

  xAddGeneralOpts( opts , defAppLayerIds, defAppPocs, defAppTids, defAppVclNaluTypes, defSeiNaluId, defPositionInSeiNalu, defModifyByEncoder );

  opts.addOptions()
    ("ZNear_%d"                      , m_zNear               , std::vector<double>(0,0)       , MAX_NUM_LAYERS , "ZNear"           )
    ("ZFar_%d"                       , m_zFar                , std::vector<double>(0,0)       , MAX_NUM_LAYERS , "ZFar"            )
    ("DMin_%d"                       , m_dMin                , std::vector<double>(0,0)       , MAX_NUM_LAYERS , "DMin"            )
    ("DMax_%d"                       , m_dMax                , std::vector<double>(0,0)       , MAX_NUM_LAYERS , "DMax"            )
    ("DepthRepresentationInfoSeiPresentFlag_%d",  m_depthRepresentationInfoSeiPresentFlag, BoolAry1d(1,0), MAX_NUM_LAYERS, "DepthRepresentationInfoSeiPresentFlag")
    ("DepthRepresentationType_%d"        , m_depthRepresentationType          , IntAry1d(0,0), MAX_NUM_LAYERS,  "DepthRepresentationType"        )
    ("DisparityRefViewId_%d"             , m_disparityRefViewId               ,  IntAry1d(0,0), MAX_NUM_LAYERS,  "DisparityRefViewId"             )
    ("DepthNonlinearRepresentationNumMinus1_%d", m_depthNonlinearRepresentationNumMinus1, IntAry1d(0,0), MAX_NUM_LAYERS, "DepthNonlinearRepresentationNumMinus1")
    ("DepthNonlinearRepresentationModel_%d"    , m_depth_nonlinear_representation_model ,   IntAry1d(0,0), MAX_NUM_LAYERS, "DepthNonlinearRepresentationModel") ;


  po::setDefaults(opts);

  // Parse the cfg file
  po::ErrorReporter err;
  po::parseConfigFile( opts, cfgFile, err );


  for(int i=0;i<MAX_NUM_LAYERS;i++)
  {
    if (m_zNear[i].size()>0)
    {
      m_zNearFlag.push_back(true);
    }
    else
    {
      m_zNearFlag.push_back(false);
    }

    if (m_zFar[i].size()>0)
    {
      m_zFarFlag.push_back(true);
    }
    else
    {
      m_zFarFlag.push_back(false);
    }

    if (m_dMin[i].size()>0)
    {
      m_dMinFlag.push_back(true);
    }
    else
    {
      m_dMinFlag.push_back(false);
    }

    if (m_dMax[i].size()>0)
    {
      m_dMaxFlag.push_back(true);
    }
    else
    {
      m_dMaxFlag.push_back(false);
    }


    if (m_depthRepresentationInfoSeiPresentFlag[i][0])
    {
      if ( m_depthRepresentationType[i].size()<=0 )
      {
        printf("DepthRepresentationType_%d must be present for layer %d\n",i,i );
        return;
      }

      if (  m_depthRepresentationType[i][0]<0 )
      {
        printf("DepthRepresentationType_%d must be equal to or greater than 0\n",i );
        return;
      }

      if (m_dMinFlag[i] || m_dMaxFlag[i])
      {
        if (m_disparityRefViewId[i].size()<=0)
        {
          printf("DisparityRefViewId_%d must be present for layer %d\n",i,i );
          assert(false);
          return;
        }
        if (m_disparityRefViewId[i][0]<0)
        {
          printf("DisparityRefViewId_%d must be equal to or greater than 0\n",i );
          assert(false);
          return;
        }
      }

      if (m_depthRepresentationType[i][0]==3)
      {
        if (m_depthNonlinearRepresentationNumMinus1[i].size()<=0)
        {
          printf("DepthNonlinearRepresentationNumMinus1_%d must be present for layer %d\n",i,i );
          assert(false);
          return;
        }
        if (m_depthNonlinearRepresentationNumMinus1[i][0]<0)
        {
          printf("DepthNonlinearRepresentationNumMinus1_%d must be equal to or greater than 0\n",i );
          assert(false);
          return;
        }

        if (m_depth_nonlinear_representation_model[i].size() != m_depthNonlinearRepresentationNumMinus1[i][0]+1)
        {
          printf("the number of values in Depth_nonlinear_representation_model must be equal to DepthNonlinearRepresentationNumMinus1+1 in layer %d\n",i );
          assert(false);
          return;
        }
      }
    }
  }

  assert(m_zNearFlag.size()==MAX_NUM_LAYERS);
  assert(m_zFarFlag.size()==MAX_NUM_LAYERS);
  assert(m_dMinFlag.size()==MAX_NUM_LAYERS);
  assert(m_dMaxFlag.size()==MAX_NUM_LAYERS);
}

Bool SEIDepthRepresentationInfo::checkCfg( const TComSlice* slice )
{
    // Check config values
    Bool wrongConfig = false;
    assert(m_currLayerID>=0);

    if (m_depthRepresentationInfoSeiPresentFlag[m_currLayerID][0]==false)
    {
        printf("DepthRepresentationInfoSeiPresentFlag_%d should be equal to 1 when  ApplicableLayerIds is empty or ApplicableLayerIds contains  %d\n",m_currLayerID,slice->getLayerId());
        assert(false);
    }
    // TBD: Add constraints on presence of SEI here.
    xCheckCfg     ( wrongConfig, m_depthRepresentationType[m_currLayerID][0] >=0 , "depth_representation_type must be equal to or greater than 0" );
    if ( m_dMaxFlag[m_currLayerID] || m_dMinFlag[m_currLayerID])
    {
        xCheckCfg( wrongConfig , m_disparityRefViewId[m_currLayerID][0]>=0, "disparity_ref_view_id must be equal to or greater than 0 when d_min or d_max are present");
    }

    if (m_depthRepresentationType[m_currLayerID][0]==3)
    {
        xCheckCfg(wrongConfig , m_depthNonlinearRepresentationNumMinus1[m_currLayerID][0]>=0, "depth_nonlinear_representation_num_minus1 must be greater than or equal to 0");

        if (m_depthNonlinearRepresentationNumMinus1[m_currLayerID][0]>=0)
        {
            xCheckCfg( wrongConfig , m_depthNonlinearRepresentationNumMinus1[m_currLayerID][0]+1 == m_depth_nonlinear_representation_model[m_currLayerID].size() ,"the number of values in depth_nonlinear_representation_model must be equal to depth_nonlinear_representation_num_minus1+1");
        }

    }

    return wrongConfig;
}


Void SEIMultiviewSceneInfo::setupFromCfgFile(const TChar* cfgFile)
{
  // Set default values
  IntAry1d defAppLayerIds, defAppPocs, defAppTids, defAppVclNaluTypes;

  // TBD: Add default values for which layers, POCS, Tids or Nalu types the SEI should be send.
  defAppLayerIds      .clear();
  defAppPocs          .clear();
  defAppTids          .push_back( 0 );
  defAppVclNaluTypes = IRAP_NAL_UNIT_TYPES;

  Int      defSeiNaluId                  = 0;
  Int      defPositionInSeiNalu          = 0;
  Bool     defModifyByEncoder            = false;

  // Setup config file options
  po::Options opts;
  xAddGeneralOpts( opts , defAppLayerIds, defAppPocs, defAppTids, defAppVclNaluTypes, defSeiNaluId, defPositionInSeiNalu, defModifyByEncoder );

  opts.addOptions()
    ("MinDisparity"                   , m_minDisparity                     , 0                              , "MinDisparity"                     )
    ("MaxDisparityRange"              , m_maxDisparityRange                , 0                              , "MaxDisparityRange"                )
    ;

  po::setDefaults(opts);

  // Parse the cfg file
  po::ErrorReporter err;
  po::parseConfigFile( opts, cfgFile, err );
};


Bool SEIMultiviewSceneInfo::checkCfg( const TComSlice* slice )
{
  // Check config values
  Bool wrongConfig = false;

  xCheckCfg     ( wrongConfig, slice->isIRAP(), "When present, the multiview scene information SEI message shall be associated with an IRAP access unit." );

  xCheckCfgRange( wrongConfig, m_minDisparity              , -1024 , 1023, "min_disparity"                    );
  xCheckCfgRange( wrongConfig, m_maxDisparityRange         ,     0 , 2047, "max_disparity_range"              );

  return wrongConfig;

};

Void SEIMultiviewAcquisitionInfo::setupFromCfgFile(const TChar* cfgFile)
{
  // Set default values
  IntAry1d defAppLayerIds, defAppPocs, defAppTids, defAppVclNaluTypes;

  defAppLayerIds    .clear();
  defAppPocs        .push_back( 0 );
  defAppTids        .push_back( 0 );
  defAppVclNaluTypes = IDR_NAL_UNIT_TYPES;


  Int      defSeiNaluId                  = 0;
  Int      defPositionInSeiNalu          = 0;
  Bool     defModifyByEncoder            = false;

  // Setup config file options
  po::Options opts;
  xAddGeneralOpts( opts , defAppLayerIds, defAppPocs, defAppTids, defAppVclNaluTypes, defSeiNaluId, defPositionInSeiNalu, defModifyByEncoder );

  opts.addOptions()
    ("IntrinsicParamFlag"               , m_intrinsicParamFlag               , false                              , "IntrinsicParamFlag"               )
    ("ExtrinsicParamFlag"               , m_extrinsicParamFlag               , false                              , "ExtrinsicParamFlag"               )
    ("IntrinsicParamsEqualFlag"         , m_intrinsicParamsEqualFlag         , false                              , "IntrinsicParamsEqualFlag"         )
    ("PrecFocalLength"                  , m_precFocalLength                  , 0                                  , "PrecFocalLength"                  )
    ("PrecPrincipalPoint"               , m_precPrincipalPoint               , 0                                  , "PrecPrincipalPoint"               )
    ("PrecSkewFactor"                   , m_precSkewFactor                   , 0                                  , "PrecSkewFactor"                   )
    ("SignFocalLengthX"                 , m_signFocalLengthX                 , BoolAry1d(1,0)                     , "SignFocalLengthX"                 )
    ("ExponentFocalLengthX"             , m_exponentFocalLengthX             , IntAry1d (1,0)                     , "ExponentFocalLengthX"             )
    ("MantissaFocalLengthX"             , m_mantissaFocalLengthX             , IntAry1d (1,0)                     , "MantissaFocalLengthX"             )
    ("SignFocalLengthY"                 , m_signFocalLengthY                 , BoolAry1d(1,0)                     , "SignFocalLengthY"                 )
    ("ExponentFocalLengthY"             , m_exponentFocalLengthY             , IntAry1d (1,0)                     , "ExponentFocalLengthY"             )
    ("MantissaFocalLengthY"             , m_mantissaFocalLengthY             , IntAry1d (1,0)                     , "MantissaFocalLengthY"             )
    ("SignPrincipalPointX"              , m_signPrincipalPointX              , BoolAry1d(1,0)                     , "SignPrincipalPointX"              )
    ("ExponentPrincipalPointX"          , m_exponentPrincipalPointX          , IntAry1d (1,0)                     , "ExponentPrincipalPointX"          )
    ("MantissaPrincipalPointX"          , m_mantissaPrincipalPointX          , IntAry1d (1,0)                     , "MantissaPrincipalPointX"          )
    ("SignPrincipalPointY"              , m_signPrincipalPointY              , BoolAry1d(1,0)                     , "SignPrincipalPointY"              )
    ("ExponentPrincipalPointY"          , m_exponentPrincipalPointY          , IntAry1d (1,0)                     , "ExponentPrincipalPointY"          )
    ("MantissaPrincipalPointY"          , m_mantissaPrincipalPointY          , IntAry1d (1,0)                     , "MantissaPrincipalPointY"          )
    ("SignSkewFactor"                   , m_signSkewFactor                   , BoolAry1d(1,0)                     , "SignSkewFactor"                   )
    ("ExponentSkewFactor"               , m_exponentSkewFactor               , IntAry1d (1,0)                     , "ExponentSkewFactor"               )
    ("MantissaSkewFactor"               , m_mantissaSkewFactor               , IntAry1d (1,0)                     , "MantissaSkewFactor"               )
    ("PrecRotationParam"                , m_precRotationParam                , 0                                  , "PrecRotationParam"                )
    ("PrecTranslationParam"             , m_precTranslationParam             , 0                                  , "PrecTranslationParam"             )
    ("SignR_%d_%d"                      , m_signR                            , BoolAry1d(3,0) ,MAX_NUM_LAYERS ,3  , "SignR"                            )
    ("ExponentR_%d_%d"                  , m_exponentR                        , IntAry1d (3,0) ,MAX_NUM_LAYERS ,3  , "ExponentR"                        )
    ("MantissaR_%d_%d"                  , m_mantissaR                        , IntAry1d (3,0) ,MAX_NUM_LAYERS ,3  , "MantissaR"                        )
    ("SignT_%d"                         , m_signT                            , BoolAry1d(3,0) ,MAX_NUM_LAYERS     , "SignT"                            )
    ("ExponentT_%d"                     , m_exponentT                        , IntAry1d (3,0) ,MAX_NUM_LAYERS     , "ExponentT"                        )
    ("MantissaT_%d"                     , m_mantissaT                        , IntAry1d (3,0) ,MAX_NUM_LAYERS     , "MantissaT"                        )
    ;

  po::setDefaults(opts);

  // Parse the cfg file
  po::ErrorReporter err;
  po::parseConfigFile( opts, cfgFile, err );
};

UInt SEIMultiviewAcquisitionInfo::getMantissaFocalLengthXLen( Int i ) const
{
  return xGetSyntaxElementLen( m_exponentFocalLengthX[i], m_precFocalLength, m_mantissaFocalLengthX[ i ] );
}

Bool SEIMultiviewAcquisitionInfo::checkCfg( const TComSlice* slice )
{
  // Check config values
  Bool wrongConfig = false;

  // Currently the encoder starts with POC 0 for all layers. The condition on POC 0 should be changed, when this changes.
  xCheckCfg     ( wrongConfig, slice->isIRAP() && (slice->getPOC() == 0)  , "When present, the multiview acquisition information SEI message that applies to the current layer shall be included in an access unit that contains an IRAP picture that is the first picture of a CLVS of the current layer." );

  xCheckCfgRange( wrongConfig, m_precFocalLength         , 0, 31, "prec_focal_length"         );
  xCheckCfgRange( wrongConfig, m_precPrincipalPoint      , 0, 31, "prec_principle_point"      );
  xCheckCfgRange( wrongConfig, m_precSkewFactor          , 0, 31, "prec_skew_factor"          );

  for (Int i = 0; i <= getNumViewsMinus1(); i++ )
  {
    xCheckCfgRange( wrongConfig, m_exponentFocalLengthX    [ i ], 0, 62, "exponent_focal_length_x"   );
    xCheckCfgRange( wrongConfig, m_exponentFocalLengthY    [ i ], 0, 62, "exponent_focal_length_y"   );
    xCheckCfgRange( wrongConfig, m_exponentPrincipalPointX [ i ], 0, 62, "exponent_principal_point_x");
    xCheckCfgRange( wrongConfig, m_exponentPrincipalPointY [ i ], 0, 62, "exponent_principal_point_y");
    xCheckCfgRange( wrongConfig, m_exponentSkewFactor      [ i ], 0, 62, "exponent_skew_factor"      );
  }

  xCheckCfgRange( wrongConfig, m_precRotationParam       , 0, 31, "prec_focal_length"         );
  xCheckCfgRange( wrongConfig, m_precTranslationParam    , 0, 31, "prec_focal_length"         );

  for (Int i = 0; i <= getNumViewsMinus1(); i++ )
  {
    for (Int j = 0; j <= 2; j++)
    {
      xCheckCfgRange( wrongConfig, m_exponentT[i][j]     , 0, 62, "exponent_skew_factor"      );
      for (Int k = 0; k <= 2; k++ )
      {
        xCheckCfgRange( wrongConfig, m_exponentR[i][j][k], 0, 62, "exponent_principal_point_y");
      }
    }
  }

  return wrongConfig;

};

UInt SEIMultiviewAcquisitionInfo::getMantissaFocalLengthYLen( Int i ) const
{
  return xGetSyntaxElementLen( m_exponentFocalLengthY[i], m_precFocalLength, m_mantissaFocalLengthY[ i ]  );
}


UInt SEIMultiviewAcquisitionInfo::getMantissaPrincipalPointXLen( Int i ) const
{
  return xGetSyntaxElementLen( m_exponentPrincipalPointX[i], m_precPrincipalPoint, m_mantissaPrincipalPointX[ i ]  );
}

UInt SEIMultiviewAcquisitionInfo::getMantissaPrincipalPointYLen( Int i ) const
{
  return xGetSyntaxElementLen( m_exponentPrincipalPointY[i], m_precPrincipalPoint, m_mantissaPrincipalPointY[ i ] );
}

UInt SEIMultiviewAcquisitionInfo::getMantissaSkewFactorLen( Int i ) const
{
  return xGetSyntaxElementLen( m_exponentSkewFactor[ i ], m_precSkewFactor, m_mantissaSkewFactor[ i ] );
}

UInt SEIMultiviewAcquisitionInfo::getMantissaRLen( Int i, Int j, Int k ) const
{
  return xGetSyntaxElementLen( m_exponentR[ i ][ j ][ k ], m_precRotationParam, m_mantissaR[ i ][ j] [ k ] );
}

UInt SEIMultiviewAcquisitionInfo::getMantissaTLen( Int i, Int j ) const
{
  return xGetSyntaxElementLen( m_exponentT[ i ][ j ], m_precTranslationParam, m_mantissaT[ i ][ j ] );
}
UInt SEIMultiviewAcquisitionInfo::xGetSyntaxElementLen( Int expo, Int prec, Int val ) const
{
  UInt len;
  if( expo == 0 )
  {
    len = std::max(0, prec - 30 );
  }
  else
  {
    len = std::max( 0, expo + prec - 31 );
  }

  assert( val >= 0 );
  assert( val <= ( ( 1 << len )- 1) );
  return len;
}

Void SEIMultiviewViewPosition::setupFromSlice  ( const TComSlice* slice )
{
  const TComVPS* vps = slice->getVPS();
  m_numViewsMinus1 = vps->getNumViews() - 1;
  m_viewPosition.resize( m_numViewsMinus1 + 1 );
  for (Int i = 0; i <= m_numViewsMinus1; i++ )
  {
    // Assuming that view ids indicate the position
    m_viewPosition[i] = vps->getViewIdVal( i );
  }
}

Void SEIMultiviewViewPosition::setupFromCfgFile(const TChar* cfgFile)
{
  // Set default values
  IntAry1d defAppLayerIds, defAppPocs, defAppTids, defAppVclNaluTypes;

  defAppLayerIds    .push_back( 0 );
  defAppPocs        .push_back( 0 );
  defAppTids        .push_back( 0 );
  defAppVclNaluTypes = IDR_NAL_UNIT_TYPES;

  Int      defSeiNaluId                  = 0;
  Int      defPositionInSeiNalu          = 0;
  Bool     defModifyByEncoder            = true;

  // Setup config file options
  po::Options opts;
  xAddGeneralOpts( opts , defAppLayerIds, defAppPocs, defAppTids, defAppVclNaluTypes, defSeiNaluId, defPositionInSeiNalu, defModifyByEncoder );

  opts.addOptions()
    ("NumViewsMinus1"         , m_numViewsMinus1                          , 0                       , "NumViewsMinus1")
    ("ViewPosition"           , m_viewPosition                            , IntAry1d (1,0)          , "ViewPosition"  );
  ;

  po::setDefaults(opts);

  // Parse the cfg file
  po::ErrorReporter err;
  po::parseConfigFile( opts, cfgFile, err );
};

Bool SEIMultiviewViewPosition::checkCfg( const TComSlice* slice )
{
  // Check config values
  Bool wrongConfig = false;

  // TBD: Add constraints on presence of SEI here.
  xCheckCfg     ( wrongConfig, slice->isIRAP() , "When present, the multiview view position SEI message shall be associated with an IRAP access unit."  );

  // TBD: Modify constraints according to the SEI semantics.
  xCheckCfgRange( wrongConfig, m_numViewsMinus1                 , 0 , 62, "num_views_minus1");
  for(Int i = 0; i <= m_numViewsMinus1; i++)
  {
    xCheckCfgRange( wrongConfig, m_viewPosition[i]                , 0 , 62, "view_position");
  }

  return wrongConfig;

};



#endif
