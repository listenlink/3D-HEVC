/* The copyright in this software is being made available under the BSD
 * License, included below. This software may be subject to other third party
 * and contributor rights, including patent rights, and no such rights are
 * granted under this license.  
 *
 * Copyright (c) 2010-2013, ITU/ISO/IEC
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

/** \file     TAppEncTop.cpp
    \brief    Encoder application class
*/

#include <list>
#include <fstream>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <assert.h>

#include "TAppEncTop.h"
#include "TLibEncoder/AnnexBwrite.h"

using namespace std;

//! \ingroup TAppEncoder
//! \{

// ====================================================================================================================
// Constructor / destructor / initialization / destroy
// ====================================================================================================================

TAppEncTop::TAppEncTop()
{
#if !H_MV
  m_iFrameRcvd = 0;
#endif
  m_totalBytes = 0;
  m_essentialBytes = 0;
}

TAppEncTop::~TAppEncTop()
{
}

Void TAppEncTop::xInitLibCfg()
{
  TComVPS vps;
  
#if H_MV
  Int maxTempLayer = -1; 
  for (Int j = 0; j < m_numberOfLayers; j++)
  {
    maxTempLayer = max( m_maxTempLayerMvc[ j ], maxTempLayer ); 
  }

  vps.setMaxTLayers                       ( maxTempLayer );
  if ( maxTempLayer )
  {
    vps.setTemporalNestingFlag(true);
  }
  vps.setMaxLayers( m_numberOfLayers );
  for(Int i = 0; i < MAX_TLAYER; i++)
  {
    Int maxNumReOrderPics  = 0; 
    Int maxDecPicBuffering = 0;
    for (Int j = 0; j < m_numberOfLayers; j++)
    {
      maxNumReOrderPics  = max( maxNumReOrderPics,  m_numReorderPicsMvc    [ j ][ i ] );     
      maxDecPicBuffering = max( maxDecPicBuffering, m_maxDecPicBufferingMvc[ j ][ i ] );     
    }

    vps.setNumReorderPics                 ( maxNumReOrderPics  ,i );
    vps.setMaxDecPicBuffering             ( maxDecPicBuffering ,i );
  }
#else
  vps.setMaxTLayers                       ( m_maxTempLayer );
  if (m_maxTempLayer == 1)
  {
    vps.setTemporalNestingFlag(true);
  }
  vps.setMaxLayers                        ( 1 );
  for(Int i = 0; i < MAX_TLAYER; i++)
  {
    vps.setNumReorderPics                 ( m_numReorderPics[i], i );
    vps.setMaxDecPicBuffering             ( m_maxDecPicBuffering[i], i );
  }
#endif

#if H_MV
  xSetLayerIds             ( vps );   
  xSetDimensionIdAndLength ( vps );
  xSetDirectDependencyFlags( vps );

  for(Int layer = 0; layer < m_numberOfLayers; layer++)
  {
    m_frameRcvd                 .push_back(0);
    m_acTEncTopList             .push_back(new TEncTop); 
    m_acTVideoIOYuvInputFileList.push_back(new TVideoIOYuv);
    m_acTVideoIOYuvReconFileList.push_back(new TVideoIOYuv);
    m_picYuvRec                 .push_back(new TComList<TComPicYuv*>) ;

    m_ivPicLists.push_back( m_acTEncTopList[ layer ]->getListPic()  ); 
    TEncTop& m_cTEncTop = *m_acTEncTopList[ layer ];  // It is not a member, but this name helps avoiding code duplication !!!
    
    m_cTEncTop.setLayerIdInVps( layer ); 
    m_cTEncTop.setLayerId     ( vps.getLayerIdInNuh( layer ) );    
    m_cTEncTop.setViewId      ( vps.getViewId      ( layer ) );
#if H_3D
    m_cTEncTop.setIsDepth     ( vps.getDepthId     ( layer ) != 0 );
#endif
    m_cTEncTop.setIvPicLists  ( &m_ivPicLists ); 
#endif

  m_cTEncTop.setVPS(&vps);

  m_cTEncTop.setProfile(m_profile);
  m_cTEncTop.setLevel(m_levelTier, m_level);

#if L0046_CONSTRAINT_FLAGS
  m_cTEncTop.setProgressiveSourceFlag(m_progressiveSourceFlag);
  m_cTEncTop.setInterlacedSourceFlag(m_interlacedSourceFlag);
  m_cTEncTop.setNonPackedConstraintFlag(m_nonPackedConstraintFlag);
  m_cTEncTop.setFrameOnlyConstraintFlag(m_frameOnlyConstraintFlag);
#endif
  
  m_cTEncTop.setFrameRate                    ( m_iFrameRate );
  m_cTEncTop.setFrameSkip                    ( m_FrameSkip );
  m_cTEncTop.setSourceWidth                  ( m_iSourceWidth );
  m_cTEncTop.setSourceHeight                 ( m_iSourceHeight );
  m_cTEncTop.setConformanceWindow            ( m_confLeft, m_confRight, m_confTop, m_confBottom );
  m_cTEncTop.setFramesToBeEncoded            ( m_framesToBeEncoded );
  
  //====== Coding Structure ========
  m_cTEncTop.setIntraPeriod                  ( m_iIntraPeriod );
  m_cTEncTop.setDecodingRefreshType          ( m_iDecodingRefreshType );
  m_cTEncTop.setGOPSize                      ( m_iGOPSize );
#if H_MV
  m_cTEncTop.setGopList                      ( m_GOPListMvc[layer] );
  m_cTEncTop.setExtraRPSs                    ( m_extraRPSsMvc[layer] );
  for(Int i = 0; i < MAX_TLAYER; i++)
  {
    m_cTEncTop.setNumReorderPics             ( m_numReorderPicsMvc[layer][i], i );
    m_cTEncTop.setMaxDecPicBuffering         ( m_maxDecPicBufferingMvc[layer][i], i );
  }
#else
  m_cTEncTop.setGopList                      ( m_GOPList );
  m_cTEncTop.setExtraRPSs                    ( m_extraRPSs );
  for(Int i = 0; i < MAX_TLAYER; i++)
  {
    m_cTEncTop.setNumReorderPics             ( m_numReorderPics[i], i );
    m_cTEncTop.setMaxDecPicBuffering         ( m_maxDecPicBuffering[i], i );
  }
#endif
  for( UInt uiLoop = 0; uiLoop < MAX_TLAYER; ++uiLoop )
  {
    m_cTEncTop.setLambdaModifier( uiLoop, m_adLambdaModifier[ uiLoop ] );
  }
#if H_MV
  m_cTEncTop.setQP                           ( m_iQP[layer] );
#else
  m_cTEncTop.setQP                           ( m_iQP );
#endif
  m_cTEncTop.setPad                          ( m_aiPad );
    
#if H_MV
  m_cTEncTop.setMaxTempLayer                 ( m_maxTempLayerMvc[layer] );
#else
  m_cTEncTop.setMaxTempLayer                 ( m_maxTempLayer );
#endif
  m_cTEncTop.setUseAMP( m_enableAMP );
  
  //===== Slice ========
  
  //====== Loop/Deblock Filter ========
#if H_MV
  m_cTEncTop.setLoopFilterDisable            ( m_bLoopFilterDisable[layer]);
#else
  m_cTEncTop.setLoopFilterDisable            ( m_bLoopFilterDisable       );
#endif
  m_cTEncTop.setLoopFilterOffsetInPPS        ( m_loopFilterOffsetInPPS );
  m_cTEncTop.setLoopFilterBetaOffset         ( m_loopFilterBetaOffsetDiv2  );
  m_cTEncTop.setLoopFilterTcOffset           ( m_loopFilterTcOffsetDiv2    );
  m_cTEncTop.setDeblockingFilterControlPresent( m_DeblockingFilterControlPresent);

  //====== Motion search ========
  m_cTEncTop.setFastSearch                   ( m_iFastSearch  );
  m_cTEncTop.setSearchRange                  ( m_iSearchRange );
  m_cTEncTop.setBipredSearchRange            ( m_bipredSearchRange );

  //====== Quality control ========
  m_cTEncTop.setMaxDeltaQP                   ( m_iMaxDeltaQP  );
  m_cTEncTop.setMaxCuDQPDepth                ( m_iMaxCuDQPDepth  );

  m_cTEncTop.setChromaCbQpOffset               ( m_cbQpOffset     );
  m_cTEncTop.setChromaCrQpOffset            ( m_crQpOffset  );

#if ADAPTIVE_QP_SELECTION
  m_cTEncTop.setUseAdaptQpSelect             ( m_bUseAdaptQpSelect   );
#endif

  Int lowestQP;
  lowestQP =  - 6*(g_bitDepthY - 8); // XXX: check
#if H_MV
  if ((m_iMaxDeltaQP == 0 ) && (m_iQP[layer] == lowestQP) && (m_useLossless == true))
#else
  if ((m_iMaxDeltaQP == 0 ) && (m_iQP == lowestQP) && (m_useLossless == true))
#endif
  {
    m_bUseAdaptiveQP = false;
  }
  m_cTEncTop.setUseAdaptiveQP                ( m_bUseAdaptiveQP  );
  m_cTEncTop.setQPAdaptationRange            ( m_iQPAdaptationRange );
  
  //====== Tool list ========
  m_cTEncTop.setUseSBACRD                    ( m_bUseSBACRD   );
  m_cTEncTop.setDeltaQpRD                    ( m_uiDeltaQpRD  );
  m_cTEncTop.setUseASR                       ( m_bUseASR      );
  m_cTEncTop.setUseHADME                     ( m_bUseHADME    );
  m_cTEncTop.setUseLossless                  ( m_useLossless );
  m_cTEncTop.setUseLComb                     ( m_bUseLComb    );
#if H_MV
  m_cTEncTop.setdQPs                         ( m_aidQP[layer]   );
#else
  m_cTEncTop.setdQPs                         ( m_aidQP        );
#endif
  m_cTEncTop.setUseRDOQ                      ( m_useRDOQ     );
  m_cTEncTop.setUseRDOQTS                    ( m_useRDOQTS   );
#if L0232_RD_PENALTY
  m_cTEncTop.setRDpenalty                 ( m_rdPenalty );
#endif
  m_cTEncTop.setQuadtreeTULog2MaxSize        ( m_uiQuadtreeTULog2MaxSize );
  m_cTEncTop.setQuadtreeTULog2MinSize        ( m_uiQuadtreeTULog2MinSize );
  m_cTEncTop.setQuadtreeTUMaxDepthInter      ( m_uiQuadtreeTUMaxDepthInter );
  m_cTEncTop.setQuadtreeTUMaxDepthIntra      ( m_uiQuadtreeTUMaxDepthIntra );
  m_cTEncTop.setUseFastEnc                   ( m_bUseFastEnc  );
  m_cTEncTop.setUseEarlyCU                   ( m_bUseEarlyCU  ); 
  m_cTEncTop.setUseFastDecisionForMerge      ( m_useFastDecisionForMerge  );
  m_cTEncTop.setUseCbfFastMode            ( m_bUseCbfFastMode  );
  m_cTEncTop.setUseEarlySkipDetection            ( m_useEarlySkipDetection );

  m_cTEncTop.setUseTransformSkip             ( m_useTransformSkip      );
  m_cTEncTop.setUseTransformSkipFast         ( m_useTransformSkipFast  );
  m_cTEncTop.setUseConstrainedIntraPred      ( m_bUseConstrainedIntraPred );
  m_cTEncTop.setPCMLog2MinSize          ( m_uiPCMLog2MinSize);
  m_cTEncTop.setUsePCM                       ( m_usePCM );
  m_cTEncTop.setPCMLog2MaxSize               ( m_pcmLog2MaxSize);
  m_cTEncTop.setMaxNumMergeCand              ( m_maxNumMergeCand );
  

  //====== Weighted Prediction ========
  m_cTEncTop.setUseWP                   ( m_useWeightedPred      );
  m_cTEncTop.setWPBiPred                ( m_useWeightedBiPred   );
  //====== Parallel Merge Estimation ========
  m_cTEncTop.setLog2ParallelMergeLevelMinus2 ( m_log2ParallelMergeLevel - 2 );

  //====== Slice ========
  m_cTEncTop.setSliceMode               ( m_sliceMode                );
  m_cTEncTop.setSliceArgument           ( m_sliceArgument            );

  //====== Dependent Slice ========
  m_cTEncTop.setSliceSegmentMode        ( m_sliceSegmentMode         );
  m_cTEncTop.setSliceSegmentArgument    ( m_sliceSegmentArgument     );
  Int iNumPartInCU = 1<<(m_uiMaxCUDepth<<1);
  if(m_sliceSegmentMode==FIXED_NUMBER_OF_LCU)
  {
    m_cTEncTop.setSliceSegmentArgument ( m_sliceSegmentArgument * iNumPartInCU );
  }
  if(m_sliceMode==FIXED_NUMBER_OF_LCU)
  {
    m_cTEncTop.setSliceArgument ( m_sliceArgument * iNumPartInCU );
  }
  if(m_sliceMode==FIXED_NUMBER_OF_TILES)
  {
    m_cTEncTop.setSliceArgument ( m_sliceArgument );
  }
  
  if(m_sliceMode == 0 )
  {
    m_bLFCrossSliceBoundaryFlag = true;
  }
  m_cTEncTop.setLFCrossSliceBoundaryFlag( m_bLFCrossSliceBoundaryFlag );
#if H_MV
  m_cTEncTop.setUseSAO ( m_bUseSAO[layer] );
#else
  m_cTEncTop.setUseSAO ( m_bUseSAO );
#endif
  m_cTEncTop.setMaxNumOffsetsPerPic (m_maxNumOffsetsPerPic);

  m_cTEncTop.setSaoLcuBoundary (m_saoLcuBoundary);
  m_cTEncTop.setSaoLcuBasedOptimization (m_saoLcuBasedOptimization);
  m_cTEncTop.setPCMInputBitDepthFlag  ( m_bPCMInputBitDepthFlag); 
  m_cTEncTop.setPCMFilterDisableFlag  ( m_bPCMFilterDisableFlag); 

  m_cTEncTop.setDecodedPictureHashSEIEnabled(m_decodedPictureHashSEIEnabled);
  m_cTEncTop.setRecoveryPointSEIEnabled( m_recoveryPointSEIEnabled );
  m_cTEncTop.setBufferingPeriodSEIEnabled( m_bufferingPeriodSEIEnabled );
  m_cTEncTop.setPictureTimingSEIEnabled( m_pictureTimingSEIEnabled );
  m_cTEncTop.setFramePackingArrangementSEIEnabled( m_framePackingSEIEnabled );
  m_cTEncTop.setFramePackingArrangementSEIType( m_framePackingSEIType );
  m_cTEncTop.setFramePackingArrangementSEIId( m_framePackingSEIId );
  m_cTEncTop.setFramePackingArrangementSEIQuincunx( m_framePackingSEIQuincunx );
  m_cTEncTop.setFramePackingArrangementSEIInterpretation( m_framePackingSEIInterpretation );
  m_cTEncTop.setDisplayOrientationSEIAngle( m_displayOrientationSEIAngle );
  m_cTEncTop.setTemporalLevel0IndexSEIEnabled( m_temporalLevel0IndexSEIEnabled );
  m_cTEncTop.setGradualDecodingRefreshInfoEnabled( m_gradualDecodingRefreshInfoEnabled );
  m_cTEncTop.setDecodingUnitInfoSEIEnabled( m_decodingUnitInfoSEIEnabled );
  m_cTEncTop.setUniformSpacingIdr          ( m_iUniformSpacingIdr );
  m_cTEncTop.setNumColumnsMinus1           ( m_iNumColumnsMinus1 );
  m_cTEncTop.setNumRowsMinus1              ( m_iNumRowsMinus1 );
  if(m_iUniformSpacingIdr==0)
  {
    m_cTEncTop.setColumnWidth              ( m_pColumnWidth );
    m_cTEncTop.setRowHeight                ( m_pRowHeight );
  }
  m_cTEncTop.xCheckGSParameters();
  Int uiTilesCount          = (m_iNumRowsMinus1+1) * (m_iNumColumnsMinus1+1);
  if(uiTilesCount == 1)
  {
    m_bLFCrossTileBoundaryFlag = true; 
  }
  m_cTEncTop.setLFCrossTileBoundaryFlag( m_bLFCrossTileBoundaryFlag );
  m_cTEncTop.setWaveFrontSynchro           ( m_iWaveFrontSynchro );
  m_cTEncTop.setWaveFrontSubstreams        ( m_iWaveFrontSubstreams );
  m_cTEncTop.setTMVPModeId ( m_TMVPModeId );
  m_cTEncTop.setUseScalingListId           ( m_useScalingListId  );
  m_cTEncTop.setScalingListFile            ( m_scalingListFile   );
  m_cTEncTop.setSignHideFlag(m_signHideFlag);
#if RATE_CONTROL_LAMBDA_DOMAIN
  m_cTEncTop.setUseRateCtrl         ( m_RCEnableRateControl );
  m_cTEncTop.setTargetBitrate       ( m_RCTargetBitrate );
  m_cTEncTop.setKeepHierBit         ( m_RCKeepHierarchicalBit );
  m_cTEncTop.setLCULevelRC          ( m_RCLCULevelRC );
  m_cTEncTop.setUseLCUSeparateModel ( m_RCUseLCUSeparateModel );
  m_cTEncTop.setInitialQP           ( m_RCInitialQP );
  m_cTEncTop.setForceIntraQP        ( m_RCForceIntraQP );
#else
  m_cTEncTop.setUseRateCtrl     ( m_enableRateCtrl);
  m_cTEncTop.setTargetBitrate   ( m_targetBitrate);
  m_cTEncTop.setNumLCUInUnit    ( m_numLCUInUnit);
#endif
  m_cTEncTop.setTransquantBypassEnableFlag(m_TransquantBypassEnableFlag);
  m_cTEncTop.setCUTransquantBypassFlagValue(m_CUTransquantBypassFlagValue);
  m_cTEncTop.setUseRecalculateQPAccordingToLambda( m_recalculateQPAccordingToLambda );
  m_cTEncTop.setUseStrongIntraSmoothing( m_useStrongIntraSmoothing );
  m_cTEncTop.setActiveParameterSetsSEIEnabled ( m_activeParameterSetsSEIEnabled ); 
  m_cTEncTop.setVuiParametersPresentFlag( m_vuiParametersPresentFlag );
  m_cTEncTop.setAspectRatioIdc( m_aspectRatioIdc );
  m_cTEncTop.setSarWidth( m_sarWidth );
  m_cTEncTop.setSarHeight( m_sarHeight );
  m_cTEncTop.setOverscanInfoPresentFlag( m_overscanInfoPresentFlag );
  m_cTEncTop.setOverscanAppropriateFlag( m_overscanAppropriateFlag );
  m_cTEncTop.setVideoSignalTypePresentFlag( m_videoSignalTypePresentFlag );
  m_cTEncTop.setVideoFormat( m_videoFormat );
  m_cTEncTop.setVideoFullRangeFlag( m_videoFullRangeFlag );
  m_cTEncTop.setColourDescriptionPresentFlag( m_colourDescriptionPresentFlag );
  m_cTEncTop.setColourPrimaries( m_colourPrimaries );
  m_cTEncTop.setTransferCharacteristics( m_transferCharacteristics );
  m_cTEncTop.setMatrixCoefficients( m_matrixCoefficients );
  m_cTEncTop.setChromaLocInfoPresentFlag( m_chromaLocInfoPresentFlag );
  m_cTEncTop.setChromaSampleLocTypeTopField( m_chromaSampleLocTypeTopField );
  m_cTEncTop.setChromaSampleLocTypeBottomField( m_chromaSampleLocTypeBottomField );
  m_cTEncTop.setNeutralChromaIndicationFlag( m_neutralChromaIndicationFlag );
  m_cTEncTop.setDefaultDisplayWindow( m_defDispWinLeftOffset, m_defDispWinRightOffset, m_defDispWinTopOffset, m_defDispWinBottomOffset );
  m_cTEncTop.setFrameFieldInfoPresentFlag( m_frameFieldInfoPresentFlag );
  m_cTEncTop.setPocProportionalToTimingFlag( m_pocProportionalToTimingFlag );
  m_cTEncTop.setNumTicksPocDiffOneMinus1   ( m_numTicksPocDiffOneMinus1    );
  m_cTEncTop.setBitstreamRestrictionFlag( m_bitstreamRestrictionFlag );
  m_cTEncTop.setTilesFixedStructureFlag( m_tilesFixedStructureFlag );
  m_cTEncTop.setMotionVectorsOverPicBoundariesFlag( m_motionVectorsOverPicBoundariesFlag );
  m_cTEncTop.setMinSpatialSegmentationIdc( m_minSpatialSegmentationIdc );
  m_cTEncTop.setMaxBytesPerPicDenom( m_maxBytesPerPicDenom );
  m_cTEncTop.setMaxBitsPerMinCuDenom( m_maxBitsPerMinCuDenom );
  m_cTEncTop.setLog2MaxMvLengthHorizontal( m_log2MaxMvLengthHorizontal );
  m_cTEncTop.setLog2MaxMvLengthVertical( m_log2MaxMvLengthVertical );
#if SIGNAL_BITRATE_PICRATE_IN_VPS
  TComBitRatePicRateInfo *bitRatePicRateInfo = m_cTEncTop.getVPS()->getBitratePicrateInfo();
  // The number of bit rate/pic rate have to equal to number of sub-layers.
  if(m_bitRatePicRateMaxTLayers)
  {
    assert(m_bitRatePicRateMaxTLayers == m_cTEncTop.getVPS()->getMaxTLayers());
  }
  for(Int i = 0; i < m_bitRatePicRateMaxTLayers; i++)
  {
    bitRatePicRateInfo->setBitRateInfoPresentFlag( i, m_bitRateInfoPresentFlag[i] );
    if( bitRatePicRateInfo->getBitRateInfoPresentFlag(i) )
    {
      bitRatePicRateInfo->setAvgBitRate(i, m_avgBitRate[i]);
      bitRatePicRateInfo->setMaxBitRate(i, m_maxBitRate[i]);
    }
  }
  for(Int i = 0; i < m_bitRatePicRateMaxTLayers; i++)
  {
    bitRatePicRateInfo->setPicRateInfoPresentFlag( i, m_picRateInfoPresentFlag[i] );
    if( bitRatePicRateInfo->getPicRateInfoPresentFlag(i) )
    {
      bitRatePicRateInfo->setAvgPicRate     (i, m_avgPicRate[i]);
      bitRatePicRateInfo->setConstantPicRateIdc(i, m_constantPicRateIdc[i]);
    }
  }
#endif

#if H_MV
  }
#endif

}

Void TAppEncTop::xCreateLib()
{
#if H_MV
  // initialize global variables
  initROM();

  for( Int layer=0; layer < m_numberOfLayers; layer++)
  {
    m_acTVideoIOYuvInputFileList[layer]->open( m_pchInputFileList[layer],     false, m_inputBitDepthY, m_inputBitDepthC, m_internalBitDepthY, m_internalBitDepthC );  // read  mode
    m_acTVideoIOYuvInputFileList[layer]->skipFrames( m_FrameSkip, m_iSourceWidth - m_aiPad[0], m_iSourceHeight - m_aiPad[1]);

    if (m_pchReconFileList[layer])
    {
      m_acTVideoIOYuvReconFileList[layer]->open( m_pchReconFileList[layer], true, m_outputBitDepthY, m_outputBitDepthC, m_internalBitDepthY, m_internalBitDepthC);  // write mode
    }
    m_acTEncTopList[layer]->create();
  }
#else
  // Video I/O
  m_cTVideoIOYuvInputFile.open( m_pchInputFile,     false, m_inputBitDepthY, m_inputBitDepthC, m_internalBitDepthY, m_internalBitDepthC );  // read  mode
  m_cTVideoIOYuvInputFile.skipFrames(m_FrameSkip, m_iSourceWidth - m_aiPad[0], m_iSourceHeight - m_aiPad[1]);

  if (m_pchReconFile)
    m_cTVideoIOYuvReconFile.open(m_pchReconFile, true, m_outputBitDepthY, m_outputBitDepthC, m_internalBitDepthY, m_internalBitDepthC);  // write mode
  
  // Neo Decoder
  m_cTEncTop.create();
#endif
}

Void TAppEncTop::xDestroyLib()
{
#if H_MV
  // destroy ROM
  destroyROM();

  for(Int layer=0; layer<m_numberOfLayers; layer++)
  {
    m_acTVideoIOYuvInputFileList[layer]->close();
    m_acTVideoIOYuvReconFileList[layer]->close();
    delete m_acTVideoIOYuvInputFileList[layer] ; 
    m_acTVideoIOYuvInputFileList[layer] = NULL;
    delete m_acTVideoIOYuvReconFileList[layer] ; 
    m_acTVideoIOYuvReconFileList[layer] = NULL;
    m_acTEncTopList[layer]->deletePicBuffer();
    m_acTEncTopList[layer]->destroy();
    delete m_acTEncTopList[layer] ; 
    m_acTEncTopList[layer] = NULL;
    delete m_picYuvRec[layer] ; 
    m_picYuvRec[layer] = NULL;
  }
#else
  // Video I/O
  m_cTVideoIOYuvInputFile.close();
  m_cTVideoIOYuvReconFile.close();
  
  // Neo Decoder
  m_cTEncTop.destroy();
#endif
}

Void TAppEncTop::xInitLib()
{
#if H_MV
  for(Int layer=0; layer<m_numberOfLayers; layer++)
  {
    m_acTEncTopList[layer]->init( );
  }
#else
  m_cTEncTop.init();
#endif
}

// ====================================================================================================================
// Public member functions
// ====================================================================================================================

/**
 - create internal class
 - initialize internal variable
 - until the end of input YUV file, call encoding function in TEncTop class
 - delete allocated buffers
 - destroy internal class
 .
 */
Void TAppEncTop::encode()
{
  fstream bitstreamFile(m_pchBitstreamFile, fstream::binary | fstream::out);
  if (!bitstreamFile)
  {
    fprintf(stderr, "\nfailed to open bitstream file `%s' for writing\n", m_pchBitstreamFile);
    exit(EXIT_FAILURE);
  }

  TComPicYuv*       pcPicYuvOrg = new TComPicYuv;
  TComPicYuv*       pcPicYuvRec = NULL;
  
  // initialize internal class & member variables
  xInitLibCfg();
  xCreateLib();
  xInitLib();
  
  // main encoder loop
#if H_MV
  Bool  allEos = false;
  std::vector<Bool>  eos ;
  std::vector<Bool>  flush ;  
  
  Int gopSize    = 1;
  Int maxGopSize = 0;
  maxGopSize = (std::max)(maxGopSize, m_acTEncTopList[0]->getGOPSize());  
  
  for(Int layer=0; layer < m_numberOfLayers; layer++ )
  {
    eos  .push_back( false );
    flush.push_back( false );
  }
#else
  Int   iNumEncoded = 0;
  Bool  bEos = false;
#endif
 
  list<AccessUnit> outputAccessUnits; ///< list of access units to write out.  is populated by the encoding process

  // allocate original YUV buffer
  pcPicYuvOrg->create( m_iSourceWidth, m_iSourceHeight, m_uiMaxCUWidth, m_uiMaxCUHeight, m_uiMaxCUDepth );
  
#if H_MV
  while ( !allEos )
  {
    for(Int layer=0; layer < m_numberOfLayers; layer++ )
    {
      Int frmCnt = 0;
      while ( !eos[layer] && !(frmCnt == gopSize))
      {
        // get buffers
        xGetBuffer(pcPicYuvRec, layer);

        // read input YUV file
        m_acTVideoIOYuvInputFileList[layer]->read      ( pcPicYuvOrg, m_aiPad );
        m_acTEncTopList             [layer]->initNewPic( pcPicYuvOrg );

        // increase number of received frames
        m_frameRcvd[layer]++;
        
        frmCnt++;

        eos[layer] = (m_frameRcvd[layer] == m_framesToBeEncoded);
        allEos = allEos|eos[layer];

        // if end of file (which is only detected on a read failure) flush the encoder of any queued pictures
        if (m_acTVideoIOYuvInputFileList[layer]->isEof())
        {
          flush          [layer] = true;
          eos            [layer]   = true;
          m_frameRcvd    [layer]--;
          m_acTEncTopList[layer]->setFramesToBeEncoded(m_frameRcvd[layer]);
        }
      }
    }
    for ( Int gopId=0; gopId < gopSize; gopId++ )
    {
      for(Int layer=0; layer < m_numberOfLayers; layer++ )
      {
        Int   iNumEncoded = 0;

        // call encoding function for one frame          
        m_acTEncTopList[layer]->encode( eos[layer], flush[layer] ? 0 : pcPicYuvOrg, *m_picYuvRec[layer], outputAccessUnits, iNumEncoded, gopId );        
        xWriteOutput(bitstreamFile, iNumEncoded, outputAccessUnits, layer);
        outputAccessUnits.clear();
      }

    }
    gopSize = maxGopSize;
  }
  for(Int layer=0; layer < m_numberOfLayers; layer++ )
  {
    m_acTEncTopList[layer]->printSummary( m_acTEncTopList[layer]->getNumAllPicCoded() );
  }
#else
  while ( !bEos )
  {
    // get buffers
    xGetBuffer(pcPicYuvRec);

    // read input YUV file
    m_cTVideoIOYuvInputFile.read( pcPicYuvOrg, m_aiPad );

    // increase number of received frames
    m_iFrameRcvd++;

    bEos = (m_iFrameRcvd == m_framesToBeEncoded);

    Bool flush = 0;
    // if end of file (which is only detected on a read failure) flush the encoder of any queued pictures
    if (m_cTVideoIOYuvInputFile.isEof())
    {
      flush = true;
      bEos = true;
      m_iFrameRcvd--;
      m_cTEncTop.setFramesToBeEncoded(m_iFrameRcvd);
    }

    // call encoding function for one frame
    m_cTEncTop.encode( bEos, flush ? 0 : pcPicYuvOrg, m_cListPicYuvRec, outputAccessUnits, iNumEncoded );
    
    // write bistream to file if necessary
    if ( iNumEncoded > 0 )
    {
      xWriteOutput(bitstreamFile, iNumEncoded, outputAccessUnits);
      outputAccessUnits.clear();
    }
  }

  m_cTEncTop.printSummary();
#endif

  // delete original YUV buffer
  pcPicYuvOrg->destroy();
  delete pcPicYuvOrg;
  pcPicYuvOrg = NULL;
  
#if !H_MV
  // delete used buffers in encoder class
  m_cTEncTop.deletePicBuffer();
#endif

  // delete buffers & classes
  xDeleteBuffer();
  xDestroyLib();
  
  printRateSummary();

  return;
}

#if H_3D
TEncTop* TAppEncTop::getTEncTopView( Int viewId, Bool isDepth )
{
  TEncTop* encoder = NULL;
  for( Int layer = 0; layer < m_acTEncTopList.size(); layer++ )
  {
    if( m_acTEncTopList[layer]->getViewId()  == viewId &&
        m_acTEncTopList[layer]->getIsDepth() == isDepth )
    {
      encoder = m_acTEncTopList[layer];
      break;
    }
  }
  return encoder;
}
#endif

// ====================================================================================================================
// Protected member functions
// ====================================================================================================================

/**
 - application has picture buffer list with size of GOP
 - picture buffer list acts as ring buffer
 - end of the list has the latest picture
 .
 */

#if H_MV
Void TAppEncTop::xGetBuffer( TComPicYuv*& rpcPicYuvRec, UInt layer)
#else
Void TAppEncTop::xGetBuffer( TComPicYuv*& rpcPicYuvRec)
#endif
{
  assert( m_iGOPSize > 0 );
  
  // org. buffer
#if H_MV
  if ( m_picYuvRec[layer]->size() == (UInt)m_iGOPSize )
  {
    rpcPicYuvRec = m_picYuvRec[layer]->popFront();
#else
  if ( m_cListPicYuvRec.size() == (UInt)m_iGOPSize )
  {
    rpcPicYuvRec = m_cListPicYuvRec.popFront();
#endif

  }
  else
  {
    rpcPicYuvRec = new TComPicYuv;
    
    rpcPicYuvRec->create( m_iSourceWidth, m_iSourceHeight, m_uiMaxCUWidth, m_uiMaxCUHeight, m_uiMaxCUDepth );

  }

#if H_MV
  m_picYuvRec[layer]->pushBack( rpcPicYuvRec );
#else
  m_cListPicYuvRec.pushBack( rpcPicYuvRec );
#endif
}

Void TAppEncTop::xDeleteBuffer( )
{

#if H_MV
  for(Int layer=0; layer<m_picYuvRec.size(); layer++)
  {
    if(m_picYuvRec[layer])
    {
      TComList<TComPicYuv*>::iterator iterPicYuvRec  = m_picYuvRec[layer]->begin();
      Int iSize = Int( m_picYuvRec[layer]->size() );
#else
  TComList<TComPicYuv*>::iterator iterPicYuvRec  = m_cListPicYuvRec.begin();
  
  Int iSize = Int( m_cListPicYuvRec.size() );
#endif

  for ( Int i = 0; i < iSize; i++ )
  {
    TComPicYuv*  pcPicYuvRec  = *(iterPicYuvRec++);
    pcPicYuvRec->destroy();
    delete pcPicYuvRec; pcPicYuvRec = NULL;
  }

#if H_MV
    }
  }
#endif  

}

/** \param iNumEncoded  number of encoded frames
 */
#if H_MV
Void TAppEncTop::xWriteOutput(std::ostream& bitstreamFile, Int iNumEncoded, std::list<AccessUnit>& accessUnits, UInt layerId)
#else
Void TAppEncTop::xWriteOutput(std::ostream& bitstreamFile, Int iNumEncoded, const std::list<AccessUnit>& accessUnits)
#endif
{
  Int i;
  
#if H_MV
  if( iNumEncoded > 0 )
  {
    TComList<TComPicYuv*>::iterator iterPicYuvRec = m_picYuvRec[layerId]->end();
#else
  TComList<TComPicYuv*>::iterator iterPicYuvRec = m_cListPicYuvRec.end();
  list<AccessUnit>::const_iterator iterBitstream = accessUnits.begin();
#endif

  for ( i = 0; i < iNumEncoded; i++ )
  {
    --iterPicYuvRec;
  }
  
  for ( i = 0; i < iNumEncoded; i++ )
  {
    TComPicYuv*  pcPicYuvRec  = *(iterPicYuvRec++);

#if H_MV
      if (m_pchReconFileList[layerId])
      {
        m_acTVideoIOYuvReconFileList[layerId]->write( pcPicYuvRec, m_confLeft, m_confRight, m_confTop, m_confBottom );
      }
    }
  }
  if( ! accessUnits.empty() )
  {
    list<AccessUnit>::iterator aUIter;
    for( aUIter = accessUnits.begin(); aUIter != accessUnits.end(); aUIter++ )
    {
      const vector<unsigned>& stats = writeAnnexB(bitstreamFile, *aUIter);
      rateStatsAccum(*aUIter, stats);
    }
  }
#else
    if (m_pchReconFile)
    {
      m_cTVideoIOYuvReconFile.write( pcPicYuvRec, m_confLeft, m_confRight, m_confTop, m_confBottom );
    }

    const AccessUnit& au = *(iterBitstream++);
    const vector<UInt>& stats = writeAnnexB(bitstreamFile, au);
    rateStatsAccum(au, stats);
  }
#endif

}

/**
 *
 */
void TAppEncTop::rateStatsAccum(const AccessUnit& au, const std::vector<UInt>& annexBsizes)
{
  AccessUnit::const_iterator it_au = au.begin();
  vector<UInt>::const_iterator it_stats = annexBsizes.begin();

  for (; it_au != au.end(); it_au++, it_stats++)
  {
    switch ((*it_au)->m_nalUnitType)
    {
    case NAL_UNIT_CODED_SLICE_TRAIL_R:
    case NAL_UNIT_CODED_SLICE_TRAIL_N:
    case NAL_UNIT_CODED_SLICE_TLA:
    case NAL_UNIT_CODED_SLICE_TSA_N:
    case NAL_UNIT_CODED_SLICE_STSA_R:
    case NAL_UNIT_CODED_SLICE_STSA_N:
    case NAL_UNIT_CODED_SLICE_BLA:
    case NAL_UNIT_CODED_SLICE_BLANT:
    case NAL_UNIT_CODED_SLICE_BLA_N_LP:
    case NAL_UNIT_CODED_SLICE_IDR:
    case NAL_UNIT_CODED_SLICE_IDR_N_LP:
    case NAL_UNIT_CODED_SLICE_CRA:
    case NAL_UNIT_CODED_SLICE_RADL_N:
    case NAL_UNIT_CODED_SLICE_DLP:
    case NAL_UNIT_CODED_SLICE_RASL_N:
    case NAL_UNIT_CODED_SLICE_TFD:
    case NAL_UNIT_VPS:
    case NAL_UNIT_SPS:
    case NAL_UNIT_PPS:
      m_essentialBytes += *it_stats;
      break;
    default:
      break;
    }

    m_totalBytes += *it_stats;
  }
}

void TAppEncTop::printRateSummary()
{

#if H_MV
  Double time = (Double) m_frameRcvd[0] / m_iFrameRate;
  printf("\n");
#else
  Double time = (Double) m_iFrameRcvd / m_iFrameRate;
#endif

  printf("Bytes written to file: %u (%.3f kbps)\n", m_totalBytes, 0.008 * m_totalBytes / time);
#if VERBOSE_RATE
  printf("Bytes for SPS/PPS/Slice (Incl. Annex B): %u (%.3f kbps)\n", m_essentialBytes, 0.008 * m_essentialBytes / time);
#endif
}

#if H_MV
Void TAppEncTop::xSetDimensionIdAndLength( TComVPS& vps )
{   
  vps.setScalabilityMask( m_scalabilityMask ); 
  for( Int dim = 0; dim < m_dimIds.size(); dim++ )
  {
      vps.setDimensionIdLen( dim, m_dimensionIdLen[ dim ] );
      for( Int layer = 1; layer < vps.getMaxLayers(); layer++ )
      {        
        vps.setDimensionId( layer, dim, m_dimIds[ dim ][ layer ] );        
      }  
  }
}

Void TAppEncTop::xSetDirectDependencyFlags( TComVPS& vps )
{
  for( Int layer = 0; layer < m_numberOfLayers; layer++ )
  {
    if( m_GOPListMvc[layer][MAX_GOP].m_POC == -1 )
    {
      continue;
    }
    for( Int i = 0; i < getGOPSize()+1; i++ ) 
    {
      GOPEntry ge = ( i < getGOPSize() ) ? m_GOPListMvc[layer][i] : m_GOPListMvc[layer][MAX_GOP];
      for( Int j = 0; j < ge.m_numInterViewRefPics; j++ )
      {
        Int interLayerRef = layer + ge.m_interViewRefs[j];
        vps.setDirectDependencyFlag( layer, interLayerRef, true );
      }
    }
  }

  vps.checkVPSExtensionSyntax(); 
  vps.calcIvRefLayers();
}

Void TAppEncTop::xSetLayerIds( TComVPS& vps )
{
  vps.setSplittingFlag     ( m_splittingFlag );

  Bool nuhLayerIdPresentFlag = !( m_layerIdInNuh.size() == 1 ); 
  Int  maxNuhLayerId = nuhLayerIdPresentFlag ? xGetMax( m_layerIdInNuh ) : ( m_numberOfLayers - 1 ) ; 

  vps.setMaxNuhLayerId( maxNuhLayerId ); 
  vps.setVpsNuhLayerIdPresentFlag( nuhLayerIdPresentFlag ); 

  for (Int layer = 0; layer < m_numberOfLayers; layer++ )
  {
    vps.setLayerIdInNuh( layer, nuhLayerIdPresentFlag ? m_layerIdInNuh[ layer ] : layer ); 
    vps.setLayerIdInVps( vps.getLayerIdInNuh( layer ), layer ); 
  }
}

Int TAppEncTop::xGetMax( std::vector<Int>& vec )
{
  Int maxVec = 0; 
  for ( Int i = 0; i < vec.size(); i++)    
    maxVec = max( vec[i], maxVec ); 
  return maxVec;
}
#endif

//! \}