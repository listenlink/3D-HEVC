/* The copyright in this software is being made available under the BSD
 * License, included below. This software may be subject to other third party
 * and contributor rights, including patent rights, and no such rights are
 * granted under this license.  
 *
 * Copyright (c) 2010-2012, ITU/ISO/IEC
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
  m_totalBytes = 0;
  m_essentialBytes = 0;
}

TAppEncTop::~TAppEncTop()
{
}

Void TAppEncTop::xInitLibCfg()
{
#if VIDYO_VPS_INTEGRATION
  UInt layerId = 0;
  // TODO: fix the assumption here that the temporal structures are all equal across all layers???
  m_cVPS.setMaxTLayers( m_maxTempLayer[0] );
  m_cVPS.setMaxLayers( m_iNumberOfViews * (m_bUsingDepthMaps ? 2:1) );
  for(Int i = 0; i < MAX_TLAYER; i++)
  {
    m_cVPS.setNumReorderPics( m_numReorderPics[0][i], i );
    m_cVPS.setMaxDecPicBuffering( m_maxDecPicBuffering[0][i], i );
  }
#endif
  
  for(Int iViewIdx=0; iViewIdx<m_iNumberOfViews; iViewIdx++)
  {
    m_frameRcvd.push_back(0);
    m_acTEncTopList.push_back(new TEncTop); 
    m_acTVideoIOYuvInputFileList.push_back(new TVideoIOYuv);
    m_acTVideoIOYuvReconFileList.push_back(new TVideoIOYuv);
    m_picYuvRec.push_back(new TComList<TComPicYuv*>) ;

    m_acTEncTopList[iViewIdx]->setFrameRate                    ( m_iFrameRate );
    m_acTEncTopList[iViewIdx]->setFrameSkip                    ( m_FrameSkip );
    m_acTEncTopList[iViewIdx]->setSourceWidth                  ( m_iSourceWidth );
    m_acTEncTopList[iViewIdx]->setSourceHeight                 ( m_iSourceHeight );
#if PIC_CROPPING
    m_acTEncTopList[iViewIdx]->setCroppingMode                 ( m_croppingMode );
    m_acTEncTopList[iViewIdx]->setCropLeft                     ( m_cropLeft );
    m_acTEncTopList[iViewIdx]->setCropRight                    ( m_cropRight );
    m_acTEncTopList[iViewIdx]->setCropTop                      ( m_cropTop );
    m_acTEncTopList[iViewIdx]->setCropBottom                   ( m_cropBottom );
#endif
    m_acTEncTopList[iViewIdx]->setFrameToBeEncoded             ( m_iFrameToBeEncoded );
    m_acTEncTopList[iViewIdx]->setViewId                       ( iViewIdx );
    m_acTEncTopList[iViewIdx]->setIsDepth                      ( false );
    m_acTEncTopList[iViewIdx]->setViewOrderIdx                 ( m_cCameraData.getViewOrderIndex()[ iViewIdx ] );
#if VIDYO_VPS_INTEGRATION
    layerId = iViewIdx * (m_bUsingDepthMaps ? 2:1);
    m_acTEncTopList[iViewIdx]->setLayerId                      ( layerId );
    m_cVPS.setDepthFlag                                        ( false, layerId );
    m_cVPS.setViewId                                           ( iViewIdx, layerId );
    m_cVPS.setViewOrderIdx                                     ( m_cCameraData.getViewOrderIndex()[ iViewIdx ], layerId );
    // TODO: set correct dependentFlag and dependentLayer
    m_cVPS.setDependentFlag                                    ( iViewIdx ? true:false, layerId );
    m_cVPS.setDependentLayer                                   ( layerId - (m_bUsingDepthMaps ? 2:1), layerId );
#endif
    
    m_acTEncTopList[iViewIdx]->setCamParPrecision              ( m_cCameraData.getCamParsCodedPrecision  () );
    m_acTEncTopList[iViewIdx]->setCamParInSliceHeader          ( m_cCameraData.getVaryingCameraParameters() );
    m_acTEncTopList[iViewIdx]->setCodedScale                   ( m_cCameraData.getCodedScale             () );
    m_acTEncTopList[iViewIdx]->setCodedOffset                  ( m_cCameraData.getCodedOffset            () );

  //====== Coding Structure ========
    m_acTEncTopList[iViewIdx]->setIntraPeriod                  ( m_iIntraPeriod );
    m_acTEncTopList[iViewIdx]->setDecodingRefreshType          ( m_iDecodingRefreshType );
    m_acTEncTopList[iViewIdx]->setGOPSize                      ( m_iGOPSize );
    m_acTEncTopList[iViewIdx]->setGopList                      ( m_GOPListsMvc[iViewIdx] );
    m_acTEncTopList[iViewIdx]->setExtraRPSs                    ( m_extraRPSs[iViewIdx] );
#if H0567_DPB_PARAMETERS_PER_TEMPORAL_LAYER
    for(Int i = 0; i < MAX_TLAYER; i++)
    {
      m_acTEncTopList[iViewIdx]->setNumReorderPics             ( m_numReorderPics[iViewIdx][i], i );
      m_acTEncTopList[iViewIdx]->setMaxDecPicBuffering         ( m_maxDecPicBuffering[iViewIdx][i], i );
    }
#else
    m_acTEncTopList[iViewIdx]->setNumReorderFrames             ( m_numReorderFrames );
    m_acTEncTopList[iViewIdx]->setMaxNumberOfReferencePictures ( m_maxNumberOfReferencePictures );
#endif
    for( UInt uiLoop = 0; uiLoop < MAX_TLAYER; ++uiLoop )
    {
      m_acTEncTopList[iViewIdx]->setLambdaModifier( uiLoop, m_adLambdaModifier[ uiLoop ] );
    }
    m_acTEncTopList[iViewIdx]->setQP                           ( m_aiQP[0] );
  
    m_acTEncTopList[iViewIdx]->setTemporalLayerQPOffset        ( m_aiTLayerQPOffset );
    m_acTEncTopList[iViewIdx]->setPad                          ( m_aiPad );
    
#if H0566_TLA
    m_acTEncTopList[iViewIdx]->setMaxTempLayer                 ( m_maxTempLayer[iViewIdx] );
#else
    m_acTEncTopList[iViewIdx]->setTLayering                    ( m_bTLayering );
    m_acTEncTopList[iViewIdx]->setTLayerSwitchingFlag          ( m_abTLayerSwitchingFlag );
#endif

    m_acTEncTopList[iViewIdx]->setDisInter4x4                  ( m_bDisInter4x4);
  
    m_acTEncTopList[iViewIdx]->setUseNSQT( m_enableNSQT );
    m_acTEncTopList[iViewIdx]->setUseAMP( m_enableAMP );
  
  //===== Slice ========
  
  //====== Loop/Deblock Filter ========
    m_acTEncTopList[iViewIdx]->setLoopFilterDisable            ( m_abLoopFilterDisable[0]       );
    m_acTEncTopList[iViewIdx]->setLoopFilterOffsetInAPS        ( m_loopFilterOffsetInAPS );
    m_acTEncTopList[iViewIdx]->setLoopFilterBetaOffset         ( m_loopFilterBetaOffsetDiv2  );
    m_acTEncTopList[iViewIdx]->setLoopFilterTcOffset           ( m_loopFilterTcOffsetDiv2    );
#if DBL_CONTROL
    m_acTEncTopList[iViewIdx]->setDeblockingFilterControlPresent( m_DeblockingFilterControlPresent);
#endif

  //====== Motion search ========
    m_acTEncTopList[iViewIdx]->setFastSearch                   ( m_iFastSearch  );
    m_acTEncTopList[iViewIdx]->setSearchRange                  ( m_iSearchRange );
    m_acTEncTopList[iViewIdx]->setBipredSearchRange            ( m_bipredSearchRange );

  //====== Quality control ========
    m_acTEncTopList[iViewIdx]->setMaxDeltaQP                   ( m_iMaxDeltaQP  );
    m_acTEncTopList[iViewIdx]->setMaxCuDQPDepth                ( m_iMaxCuDQPDepth  );

    m_acTEncTopList[iViewIdx]->setChromaQpOffset               ( m_iChromaQpOffset     );
    m_acTEncTopList[iViewIdx]->setChromaQpOffset2nd            ( m_iChromaQpOffset2nd  );

#if ADAPTIVE_QP_SELECTION
    m_acTEncTopList[iViewIdx]->setUseAdaptQpSelect             ( m_bUseAdaptQpSelect   );
#endif

#if LOSSLESS_CODING
    Int lowestQP;
#if H0736_AVC_STYLE_QP_RANGE
    lowestQP =  - ( (Int)(6*(g_uiBitDepth + g_uiBitIncrement - 8)) );
#else
    lowestQP = 0;
#endif
    if ((m_iMaxDeltaQP == 0 ) && (m_aiQP[0] == lowestQP) && (m_useLossless == true))
    {
      m_bUseAdaptiveQP = false;
    }
#endif

    m_acTEncTopList[iViewIdx]->setUseAdaptiveQP                ( m_bUseAdaptiveQP  );
    m_acTEncTopList[iViewIdx]->setQPAdaptationRange            ( m_iQPAdaptationRange );
  
#if HHI_VSO
    //====== VSO =========
    m_acTEncTopList[iViewIdx]->setForceLambdaScaleVSO          ( false );
    m_acTEncTopList[iViewIdx]->setLambdaScaleVSO               ( 1     );
    m_acTEncTopList[iViewIdx]->setVSOMode                      ( 0     );
    m_acTEncTopList[iViewIdx]->setUseVSO                       ( false ); 
#if SAIT_VSO_EST_A0033
    m_acTEncTopList[iViewIdx]->setUseEstimatedVSD              ( false );
#endif
#if LGE_WVSO_A0119
    m_acTEncTopList[iViewIdx]->setWVSO												 ( false ); 
#endif
#endif

#if DEPTH_MAP_GENERATION
    m_acTEncTopList[iViewIdx]->setPredDepthMapGeneration       ( m_uiPredDepthMapGeneration );
    m_acTEncTopList[iViewIdx]->setPdmPrecision                 ( (UInt)m_cCameraData.getPdmPrecision     () );
    m_acTEncTopList[iViewIdx]->setPdmScaleNomDelta             (       m_cCameraData.getPdmScaleNomDelta () );
    m_acTEncTopList[iViewIdx]->setPdmOffset                    (       m_cCameraData.getPdmOffset        () );
#endif
#if HHI_INTER_VIEW_MOTION_PRED
    m_acTEncTopList[iViewIdx]->setMultiviewMvPredMode          ( m_uiMultiviewMvPredMode );
    m_acTEncTopList[iViewIdx]->setMultiviewMvRegMode           ( iViewIdx ? m_uiMultiviewMvRegMode       : 0   );
    m_acTEncTopList[iViewIdx]->setMultiviewMvRegLambdaScale    ( iViewIdx ? m_dMultiviewMvRegLambdaScale : 0.0 );
#endif
#if HHI_INTER_VIEW_RESIDUAL_PRED
    m_acTEncTopList[iViewIdx]->setMultiviewResPredMode         ( m_uiMultiviewResPredMode );
#endif

  //====== Tool list ========
    m_acTEncTopList[iViewIdx]->setUseSBACRD                    ( m_bUseSBACRD   );
    m_acTEncTopList[iViewIdx]->setDeltaQpRD                    ( m_uiDeltaQpRD  );
    m_acTEncTopList[iViewIdx]->setUseASR                       ( m_bUseASR      );
    m_acTEncTopList[iViewIdx]->setUseHADME                     ( m_bUseHADME    );
    m_acTEncTopList[iViewIdx]->setUseALF                       ( m_abUseALF[0]  );
    m_acTEncTopList[iViewIdx]->setALFEncodePassReduction       ( m_iALFEncodePassReduction );
#if LOSSLESS_CODING
    m_acTEncTopList[iViewIdx]->setUseLossless                  ( m_useLossless );
#endif
    m_acTEncTopList[iViewIdx]->setALFMaxNumberFilters          ( m_iALFMaxNumberFilters ) ;

    m_acTEncTopList[iViewIdx]->setUseLComb                     ( m_bUseLComb    );
    m_acTEncTopList[iViewIdx]->setLCMod                        ( m_bLCMod         );
    m_acTEncTopList[iViewIdx]->setdQPs                         ( m_aidQP        );
    m_acTEncTopList[iViewIdx]->setUseRDOQ                      ( m_abUseRDOQ[0] );
#if !PIC_CROPPING
    m_acTEncTopList[iViewIdx]->setUsePAD                       ( m_bUsePAD      );
#endif
    m_acTEncTopList[iViewIdx]->setQuadtreeTULog2MaxSize        ( m_uiQuadtreeTULog2MaxSize );
    m_acTEncTopList[iViewIdx]->setQuadtreeTULog2MinSize        ( m_uiQuadtreeTULog2MinSize );
    m_acTEncTopList[iViewIdx]->setQuadtreeTUMaxDepthInter      ( m_uiQuadtreeTUMaxDepthInter );
    m_acTEncTopList[iViewIdx]->setQuadtreeTUMaxDepthIntra      ( m_uiQuadtreeTUMaxDepthIntra );
    m_acTEncTopList[iViewIdx]->setUseFastEnc                   ( m_bUseFastEnc  );
    m_acTEncTopList[iViewIdx]->setUseEarlyCU                   ( m_bUseEarlyCU  ); 
#if FAST_DECISION_FOR_MRG_RD_COST
    m_acTEncTopList[iViewIdx]->setUseFastDecisionForMerge      ( m_useFastDecisionForMerge  );
#endif
    m_acTEncTopList[iViewIdx]->setUseCbfFastMode               ( m_bUseCbfFastMode  );
#if HHI_INTERVIEW_SKIP
    m_acTEncTopList[iViewIdx]->setInterViewSkip            ( iViewIdx>0 ? m_bInterViewSkip : false );
#if HHI_INTERVIEW_SKIP_LAMBDA_SCALE
    m_acTEncTopList[iViewIdx]->setInterViewSkipLambdaScale ( iViewIdx>0 ? (UInt)m_dInterViewSkipLambdaScale : 1 );
#endif
#endif
    m_acTEncTopList[iViewIdx]->setUseLMChroma                  ( m_bUseLMChroma );
    m_acTEncTopList[iViewIdx]->setUseConstrainedIntraPred      ( m_bUseConstrainedIntraPred );
    m_acTEncTopList[iViewIdx]->setPCMLog2MinSize               ( m_uiPCMLog2MinSize);
    m_acTEncTopList[iViewIdx]->setUsePCM                       ( m_usePCM );
    m_acTEncTopList[iViewIdx]->setPCMLog2MaxSize               ( m_pcmLog2MaxSize);

  //====== Weighted Prediction ========
    m_acTEncTopList[iViewIdx]->setUseWP                        ( m_bUseWeightPred      );
    m_acTEncTopList[iViewIdx]->setWPBiPredIdc                  ( m_uiBiPredIdc         );
  //====== Slice ========
    m_acTEncTopList[iViewIdx]->setSliceMode                    ( m_iSliceMode                );
    m_acTEncTopList[iViewIdx]->setSliceArgument                ( m_iSliceArgument            );

  //====== Entropy Slice ========
    m_acTEncTopList[iViewIdx]->setEntropySliceMode             ( m_iEntropySliceMode         );
    m_acTEncTopList[iViewIdx]->setEntropySliceArgument         ( m_iEntropySliceArgument     );
    int iNumPartInCU = 1<<(m_uiMaxCUDepth<<1);
    if(m_iEntropySliceMode==SHARP_FIXED_NUMBER_OF_LCU_IN_ENTROPY_SLICE)
    {
      m_acTEncTopList[iViewIdx]->setEntropySliceArgument ( m_iEntropySliceArgument * ( iNumPartInCU >> ( m_iSliceGranularity << 1 ) ) );
    }
    if(m_iSliceMode==AD_HOC_SLICES_FIXED_NUMBER_OF_LCU_IN_SLICE)
    {
      m_acTEncTopList[iViewIdx]->setSliceArgument ( m_iSliceArgument * ( iNumPartInCU >> ( m_iSliceGranularity << 1 ) ) );
    }
#if FIXED_NUMBER_OF_TILES_SLICE_MODE
    if(m_iSliceMode==AD_HOC_SLICES_FIXED_NUMBER_OF_TILES_IN_SLICE)
    {
      m_acTEncTopList[iViewIdx]->setSliceArgument ( m_iSliceArgument );
    }
#endif
    m_acTEncTopList[iViewIdx]->setSliceGranularity        ( m_iSliceGranularity         );
    if(m_iSliceMode == 0 )
    {
      m_bLFCrossSliceBoundaryFlag = true;
    }
    m_acTEncTopList[iViewIdx]->setLFCrossSliceBoundaryFlag( m_bLFCrossSliceBoundaryFlag );
    m_acTEncTopList[iViewIdx]->setUseSAO               ( m_abUseSAO[0]     );
#if SAO_UNIT_INTERLEAVING
    m_acTEncTopList[iViewIdx]->setMaxNumOffsetsPerPic (m_maxNumOffsetsPerPic);
    m_acTEncTopList[iViewIdx]->setSaoInterleavingFlag (m_saoInterleavingFlag);
#endif
    m_acTEncTopList[iViewIdx]->setPCMInputBitDepthFlag  ( m_bPCMInputBitDepthFlag); 
    m_acTEncTopList[iViewIdx]->setPCMFilterDisableFlag  ( m_bPCMFilterDisableFlag); 

    m_acTEncTopList[iViewIdx]->setPictureDigestEnabled(m_pictureDigestEnabled);

    m_acTEncTopList[iViewIdx]->setColumnRowInfoPresent       ( m_iColumnRowInfoPresent );
    m_acTEncTopList[iViewIdx]->setUniformSpacingIdr          ( m_iUniformSpacingIdr );
#if !REMOVE_TILE_DEPENDENCE
    m_acTEncTopList[iViewIdx]->setTileBoundaryIndependenceIdr( m_iTileBoundaryIndependenceIdr );
#endif
    m_acTEncTopList[iViewIdx]->setNumColumnsMinus1           ( m_iNumColumnsMinus1 );
    m_acTEncTopList[iViewIdx]->setNumRowsMinus1              ( m_iNumRowsMinus1 );
    if(m_iUniformSpacingIdr==0)
    {
      m_acTEncTopList[iViewIdx]->setColumnWidth              ( m_pchColumnWidth );
      m_acTEncTopList[iViewIdx]->setRowHeight                ( m_pchRowHeight );
    }
    m_acTEncTopList[iViewIdx]->xCheckGSParameters();
    m_acTEncTopList[iViewIdx]->setTileLocationInSliceHeaderFlag ( m_iTileLocationInSliceHeaderFlag );
    m_acTEncTopList[iViewIdx]->setTileMarkerFlag              ( m_iTileMarkerFlag );
    m_acTEncTopList[iViewIdx]->setMaxTileMarkerEntryPoints    ( m_iMaxTileMarkerEntryPoints );
  
    Int uiTilesCount          = (m_iNumRowsMinus1+1) * (m_iNumColumnsMinus1+1);
    m_dMaxTileMarkerOffset    = ((Double)uiTilesCount) / m_iMaxTileMarkerEntryPoints;
    m_acTEncTopList[iViewIdx]->setMaxTileMarkerOffset         ( m_dMaxTileMarkerOffset );
    m_acTEncTopList[iViewIdx]->setTileBehaviorControlPresentFlag( m_iTileBehaviorControlPresentFlag );
#if !REMOVE_TILE_DEPENDENCE
    if(m_iTileBoundaryIndependenceIdr == 0 || uiTilesCount == 1)
#else
    if(uiTilesCount == 1)
#endif
    {
      m_bLFCrossTileBoundaryFlag = true; 
    }
    m_acTEncTopList[iViewIdx]->setLFCrossTileBoundaryFlag( m_bLFCrossTileBoundaryFlag );
    m_acTEncTopList[iViewIdx]->setWaveFrontSynchro           ( m_iWaveFrontSynchro );
    m_acTEncTopList[iViewIdx]->setWaveFrontFlush             ( m_iWaveFrontFlush );
    m_acTEncTopList[iViewIdx]->setWaveFrontSubstreams        ( m_iWaveFrontSubstreams );
    m_acTEncTopList[iViewIdx]->setEnableTMVP ( m_enableTMVP );
    m_acTEncTopList[iViewIdx]->setUseScalingListId           ( m_useScalingListId  );
    m_acTEncTopList[iViewIdx]->setScalingListFile            ( m_scalingListFile   );
#if MULTIBITS_DATA_HIDING
    m_acTEncTopList[iViewIdx]->setSignHideFlag(m_signHideFlag);
    m_acTEncTopList[iViewIdx]->setTSIG(m_signHidingThreshold);
#endif

#if LCU_SYNTAX_ALF
    if(uiTilesCount > 1)
    {
      m_bALFParamInSlice = false;
      m_bALFPicBasedEncode = true;
    }
    m_acTEncTopList[iViewIdx]->setALFParamInSlice              ( m_bALFParamInSlice);
    m_acTEncTopList[iViewIdx]->setALFPicBasedEncode            ( m_bALFPicBasedEncode);
#endif

    //====== Depth tools ========
#if HHI_DMM_WEDGE_INTRA || HHI_DMM_PRED_TEX
    m_acTEncTopList[iViewIdx]->setUseDMM                     ( false );
#endif
#if HHI_MPI
    m_acTEncTopList[iViewIdx]->setUseMVI( false );
#endif
  }
  if( m_bUsingDepthMaps )
  {
    for(Int iViewIdx=0; iViewIdx<m_iNumberOfViews; iViewIdx++)
    {
      m_depthFrameRcvd.push_back(0);
      m_acTEncDepthTopList.push_back(new TEncTop); 
      m_acTVideoIOYuvDepthInputFileList.push_back(new TVideoIOYuv);
      m_acTVideoIOYuvDepthReconFileList.push_back(new TVideoIOYuv);
      m_picYuvDepthRec.push_back(new TComList<TComPicYuv*>) ;

      m_acTEncDepthTopList[iViewIdx]->setFrameRate                    ( m_iFrameRate );
      m_acTEncDepthTopList[iViewIdx]->setFrameSkip                    ( m_FrameSkip );
      m_acTEncDepthTopList[iViewIdx]->setSourceWidth                  ( m_iSourceWidth );
      m_acTEncDepthTopList[iViewIdx]->setSourceHeight                 ( m_iSourceHeight );
#if PIC_CROPPING
      m_acTEncDepthTopList[iViewIdx]->setCroppingMode                 ( m_croppingMode );
      m_acTEncDepthTopList[iViewIdx]->setCropLeft                     ( m_cropLeft );
      m_acTEncDepthTopList[iViewIdx]->setCropRight                    ( m_cropRight );
      m_acTEncDepthTopList[iViewIdx]->setCropTop                      ( m_cropTop );
      m_acTEncDepthTopList[iViewIdx]->setCropBottom                   ( m_cropBottom );
#endif
      m_acTEncDepthTopList[iViewIdx]->setFrameToBeEncoded             ( m_iFrameToBeEncoded );
      m_acTEncDepthTopList[iViewIdx]->setViewId                       ( iViewIdx );
      m_acTEncDepthTopList[iViewIdx]->setIsDepth                      ( true );
      m_acTEncDepthTopList[iViewIdx]->setViewOrderIdx                 ( m_cCameraData.getViewOrderIndex()[ iViewIdx ] );
#if VIDYO_VPS_INTEGRATION
      layerId = iViewIdx * 2 + 1;
      m_acTEncDepthTopList[iViewIdx]->setLayerId                      ( layerId );
      m_cVPS.setDepthFlag                                             ( true, layerId );
      m_cVPS.setViewId                                                ( iViewIdx, layerId );
      m_cVPS.setViewOrderIdx                                          ( m_cCameraData.getViewOrderIndex()[ iViewIdx ], layerId );
      m_cVPS.setDependentFlag                                         ( true, layerId );
      m_cVPS.setDependentLayer                                        ( layerId-1, layerId);
#endif
      m_acTEncDepthTopList[iViewIdx]->setCamParPrecision              ( 0 );
      m_acTEncDepthTopList[iViewIdx]->setCamParInSliceHeader          ( false );
      m_acTEncDepthTopList[iViewIdx]->setCodedScale                   ( 0 );
      m_acTEncDepthTopList[iViewIdx]->setCodedOffset                  ( 0 );

      //====== Coding Structure ========
      m_acTEncDepthTopList[iViewIdx]->setIntraPeriod                  ( m_iIntraPeriod );
      m_acTEncDepthTopList[iViewIdx]->setDecodingRefreshType          ( m_iDecodingRefreshType );
      m_acTEncDepthTopList[iViewIdx]->setGOPSize                      ( m_iGOPSize );
      m_acTEncDepthTopList[iViewIdx]->setGopList                      ( m_GOPListsMvc[iViewIdx] );
      m_acTEncDepthTopList[iViewIdx]->setExtraRPSs                    ( m_extraRPSs[iViewIdx] );
#if H0567_DPB_PARAMETERS_PER_TEMPORAL_LAYER
      for(Int i = 0; i < MAX_TLAYER; i++)
      {
        m_acTEncDepthTopList[iViewIdx]->setNumReorderPics             ( m_numReorderPics[iViewIdx][i], i );
        m_acTEncDepthTopList[iViewIdx]->setMaxDecPicBuffering         ( m_maxDecPicBuffering[iViewIdx][i], i );
      }
#else
      m_acTEncDepthTopList[iViewIdx]->setNumReorderFrames             ( m_numReorderFrames );
      m_acTEncDepthTopList[iViewIdx]->setMaxNumberOfReferencePictures ( m_maxNumberOfReferencePictures );
#endif
      for( UInt uiLoop = 0; uiLoop < MAX_TLAYER; ++uiLoop )
      {
        m_acTEncDepthTopList[iViewIdx]->setLambdaModifier( uiLoop, m_adLambdaModifier[ uiLoop ] );
      }
      m_acTEncDepthTopList[iViewIdx]->setQP                           ( m_aiQP[1] );

      m_acTEncDepthTopList[iViewIdx]->setTemporalLayerQPOffset        ( m_aiTLayerQPOffset );
      m_acTEncDepthTopList[iViewIdx]->setPad                          ( m_aiPad );

#if H0566_TLA
      m_acTEncDepthTopList[iViewIdx]->setMaxTempLayer                 ( m_maxTempLayer[iViewIdx] );
#else
      m_acTEncDepthTopList[iViewIdx]->setTLayering                    ( m_bTLayering );
      m_acTEncDepthTopList[iViewIdx]->setTLayerSwitchingFlag          ( m_abTLayerSwitchingFlag );
#endif

      m_acTEncDepthTopList[iViewIdx]->setDisInter4x4                  ( m_bDisInter4x4);

      m_acTEncDepthTopList[iViewIdx]->setUseNSQT( m_enableNSQT );
      m_acTEncDepthTopList[iViewIdx]->setUseAMP( m_enableAMP );

      //===== Slice ========

      //====== Loop/Deblock Filter ========
      m_acTEncDepthTopList[iViewIdx]->setLoopFilterDisable            ( m_abLoopFilterDisable[1]   );
      m_acTEncDepthTopList[iViewIdx]->setLoopFilterOffsetInAPS        ( m_loopFilterOffsetInAPS );
      m_acTEncDepthTopList[iViewIdx]->setLoopFilterBetaOffset         ( m_loopFilterBetaOffsetDiv2  );
      m_acTEncDepthTopList[iViewIdx]->setLoopFilterTcOffset           ( m_loopFilterTcOffsetDiv2    );
#if DBL_CONTROL
      m_acTEncDepthTopList[iViewIdx]->setDeblockingFilterControlPresent( m_DeblockingFilterControlPresent);
#endif

      //====== Motion search ========
      m_acTEncDepthTopList[iViewIdx]->setFastSearch                   ( m_iFastSearch  );
      m_acTEncDepthTopList[iViewIdx]->setSearchRange                  ( m_iSearchRange );
      m_acTEncDepthTopList[iViewIdx]->setBipredSearchRange            ( m_bipredSearchRange );

      //====== Quality control ========
      m_acTEncDepthTopList[iViewIdx]->setMaxDeltaQP                   ( m_iMaxDeltaQP  );
      m_acTEncDepthTopList[iViewIdx]->setMaxCuDQPDepth                ( m_iMaxCuDQPDepth  );

      m_acTEncDepthTopList[iViewIdx]->setChromaQpOffset               ( m_iChromaQpOffset     );
      m_acTEncDepthTopList[iViewIdx]->setChromaQpOffset2nd            ( m_iChromaQpOffset2nd  );

#if ADAPTIVE_QP_SELECTION
      m_acTEncDepthTopList[iViewIdx]->setUseAdaptQpSelect             ( m_bUseAdaptQpSelect   );
#endif

      m_acTEncDepthTopList[iViewIdx]->setUseAdaptiveQP                ( m_bUseAdaptiveQP  );
      m_acTEncDepthTopList[iViewIdx]->setQPAdaptationRange            ( m_iQPAdaptationRange );

      //====== Tool list ========
      m_acTEncDepthTopList[iViewIdx]->setUseSBACRD                    ( m_bUseSBACRD   );
      m_acTEncDepthTopList[iViewIdx]->setDeltaQpRD                    ( m_uiDeltaQpRD  );
      m_acTEncDepthTopList[iViewIdx]->setUseASR                       ( m_bUseASR      );
      m_acTEncDepthTopList[iViewIdx]->setUseHADME                     ( m_bUseHADME    );
      m_acTEncDepthTopList[iViewIdx]->setUseALF                       ( m_abUseALF[1]  );
      m_acTEncDepthTopList[iViewIdx]->setALFEncodePassReduction       ( m_iALFEncodePassReduction );
#if LOSSLESS_CODING
      m_acTEncDepthTopList[iViewIdx]->setUseLossless                  ( m_useLossless );
#endif
      m_acTEncDepthTopList[iViewIdx]->setALFMaxNumberFilters          ( m_iALFMaxNumberFilters ) ;

      m_acTEncDepthTopList[iViewIdx]->setUseLComb                     ( m_bUseLComb    );
      m_acTEncDepthTopList[iViewIdx]->setLCMod                        ( m_bLCMod       );
      m_acTEncDepthTopList[iViewIdx]->setdQPs                         ( m_aidQPdepth   );
      m_acTEncDepthTopList[iViewIdx]->setUseRDOQ                      ( m_abUseRDOQ[1] );
#if !PIC_CROPPING
      m_acTEncDepthTopList[iViewIdx]->setUsePAD                       ( m_bUsePAD      );
#endif
      m_acTEncDepthTopList[iViewIdx]->setQuadtreeTULog2MaxSize        ( m_uiQuadtreeTULog2MaxSize );
      m_acTEncDepthTopList[iViewIdx]->setQuadtreeTULog2MinSize        ( m_uiQuadtreeTULog2MinSize );
      m_acTEncDepthTopList[iViewIdx]->setQuadtreeTUMaxDepthInter      ( m_uiQuadtreeTUMaxDepthInter );
      m_acTEncDepthTopList[iViewIdx]->setQuadtreeTUMaxDepthIntra      ( m_uiQuadtreeTUMaxDepthIntra );
      m_acTEncDepthTopList[iViewIdx]->setUseFastEnc                   ( m_bUseFastEnc  );
      m_acTEncDepthTopList[iViewIdx]->setUseEarlyCU                   ( m_bUseEarlyCU  ); 
#if FAST_DECISION_FOR_MRG_RD_COST
      m_acTEncDepthTopList[iViewIdx]->setUseFastDecisionForMerge      ( m_useFastDecisionForMerge  );
#endif
      m_acTEncDepthTopList[iViewIdx]->setUseCbfFastMode               ( m_bUseCbfFastMode  );
#if HHI_INTERVIEW_SKIP
      m_acTEncDepthTopList[iViewIdx]->setInterViewSkip            ( 0 );
#if HHI_INTERVIEW_SKIP_LAMBDA_SCALE
      m_acTEncDepthTopList[iViewIdx]->setInterViewSkipLambdaScale ( 1 );
#endif
#endif
      m_acTEncDepthTopList[iViewIdx]->setUseLMChroma                  ( m_bUseLMChroma );
      m_acTEncDepthTopList[iViewIdx]->setUseConstrainedIntraPred      ( m_bUseConstrainedIntraPred );
      m_acTEncDepthTopList[iViewIdx]->setPCMLog2MinSize               ( m_uiPCMLog2MinSize);
      m_acTEncDepthTopList[iViewIdx]->setUsePCM                       ( m_usePCM );
      m_acTEncDepthTopList[iViewIdx]->setPCMLog2MaxSize               ( m_pcmLog2MaxSize);
      //====== VSO ========
#if HHI_VSO
      m_acTEncDepthTopList[iViewIdx]->setUseVSO                       ( m_bUseVSO      ); //GT: might be enabled/disabled later for VSO Mode 4
      m_acTEncDepthTopList[iViewIdx]->setForceLambdaScaleVSO          ( m_bForceLambdaScaleVSO );
      m_acTEncDepthTopList[iViewIdx]->setLambdaScaleVSO               ( m_dLambdaScaleVSO );
#if HHI_VSO_DIST_INT
      m_acTEncDepthTopList[iViewIdx]->setAllowNegDist                 ( m_bAllowNegDist );
#endif
      m_acTEncDepthTopList[iViewIdx]->setVSOMode                      ( m_uiVSOMode );

#if SAIT_VSO_EST_A0033
      m_acTEncDepthTopList[iViewIdx]->setUseEstimatedVSD              ( m_bUseEstimatedVSD );
#endif
#if LGE_WVSO_A0119
      m_acTEncDepthTopList[iViewIdx]->setWVSO                          ( m_bWVSO      );
#endif
#endif

#if DEPTH_MAP_GENERATION
      m_acTEncDepthTopList[iViewIdx]->setPredDepthMapGeneration       ( 0 );
#endif
#if HHI_INTER_VIEW_MOTION_PRED
      m_acTEncDepthTopList[iViewIdx]->setMultiviewMvPredMode          ( 0 );
      m_acTEncDepthTopList[iViewIdx]->setMultiviewMvRegMode           ( 0 );
      m_acTEncDepthTopList[iViewIdx]->setMultiviewMvRegLambdaScale    ( 0.0 );
#endif
#if HHI_INTER_VIEW_RESIDUAL_PRED
      m_acTEncDepthTopList[iViewIdx]->setMultiviewResPredMode         ( 0 );
#endif

      //====== Weighted Prediction ========
      m_acTEncDepthTopList[iViewIdx]->setUseWP                        ( m_bUseWeightPred      );
      m_acTEncDepthTopList[iViewIdx]->setWPBiPredIdc                  ( m_uiBiPredIdc         );
      //====== Slice ========
      m_acTEncDepthTopList[iViewIdx]->setSliceMode                    ( m_iSliceMode                );
      m_acTEncDepthTopList[iViewIdx]->setSliceArgument                ( m_iSliceArgument            );

      //====== Entropy Slice ========
      m_acTEncDepthTopList[iViewIdx]->setEntropySliceMode             ( m_iEntropySliceMode         );
      m_acTEncDepthTopList[iViewIdx]->setEntropySliceArgument         ( m_iEntropySliceArgument     );
      int iNumPartInCU = 1<<(m_uiMaxCUDepth<<1);
      if(m_iEntropySliceMode==SHARP_FIXED_NUMBER_OF_LCU_IN_ENTROPY_SLICE)
      {
        m_acTEncDepthTopList[iViewIdx]->setEntropySliceArgument ( m_iEntropySliceArgument * ( iNumPartInCU >> ( m_iSliceGranularity << 1 ) ) );
      }
      if(m_iSliceMode==AD_HOC_SLICES_FIXED_NUMBER_OF_LCU_IN_SLICE)
      {
        m_acTEncDepthTopList[iViewIdx]->setSliceArgument ( m_iSliceArgument * ( iNumPartInCU >> ( m_iSliceGranularity << 1 ) ) );
      }
#if FIXED_NUMBER_OF_TILES_SLICE_MODE
      if(m_iSliceMode==AD_HOC_SLICES_FIXED_NUMBER_OF_TILES_IN_SLICE)
      {
        m_acTEncDepthTopList[iViewIdx]->setSliceArgument ( m_iSliceArgument );
      }
#endif
      m_acTEncDepthTopList[iViewIdx]->setSliceGranularity        ( m_iSliceGranularity         );
      if(m_iSliceMode == 0 )
      {
        m_bLFCrossSliceBoundaryFlag = true;
      }
      m_acTEncDepthTopList[iViewIdx]->setLFCrossSliceBoundaryFlag( m_bLFCrossSliceBoundaryFlag );
      m_acTEncDepthTopList[iViewIdx]->setUseSAO               ( m_abUseSAO[1]     );
#if SAO_UNIT_INTERLEAVING
      m_acTEncDepthTopList[iViewIdx]->setMaxNumOffsetsPerPic (m_maxNumOffsetsPerPic);
      m_acTEncDepthTopList[iViewIdx]->setSaoInterleavingFlag (m_saoInterleavingFlag);
#endif
      m_acTEncDepthTopList[iViewIdx]->setPCMInputBitDepthFlag  ( m_bPCMInputBitDepthFlag); 
      m_acTEncDepthTopList[iViewIdx]->setPCMFilterDisableFlag  ( m_bPCMFilterDisableFlag); 

      m_acTEncDepthTopList[iViewIdx]->setPictureDigestEnabled(m_pictureDigestEnabled);

      m_acTEncDepthTopList[iViewIdx]->setColumnRowInfoPresent       ( m_iColumnRowInfoPresent );
      m_acTEncDepthTopList[iViewIdx]->setUniformSpacingIdr          ( m_iUniformSpacingIdr );
#if !REMOVE_TILE_DEPENDENCE
      m_acTEncDepthTopList[iViewIdx]->setTileBoundaryIndependenceIdr( m_iTileBoundaryIndependenceIdr );
#endif
      m_acTEncDepthTopList[iViewIdx]->setNumColumnsMinus1           ( m_iNumColumnsMinus1 );
      m_acTEncDepthTopList[iViewIdx]->setNumRowsMinus1              ( m_iNumRowsMinus1 );
      if(m_iUniformSpacingIdr==0)
      {
        m_acTEncDepthTopList[iViewIdx]->setColumnWidth              ( m_pchColumnWidth );
        m_acTEncDepthTopList[iViewIdx]->setRowHeight                ( m_pchRowHeight );
      }
      m_acTEncDepthTopList[iViewIdx]->xCheckGSParameters();
      m_acTEncDepthTopList[iViewIdx]->setTileLocationInSliceHeaderFlag ( m_iTileLocationInSliceHeaderFlag );
      m_acTEncDepthTopList[iViewIdx]->setTileMarkerFlag              ( m_iTileMarkerFlag );
      m_acTEncDepthTopList[iViewIdx]->setMaxTileMarkerEntryPoints    ( m_iMaxTileMarkerEntryPoints );

      Int uiTilesCount          = (m_iNumRowsMinus1+1) * (m_iNumColumnsMinus1+1);
      m_dMaxTileMarkerOffset    = ((Double)uiTilesCount) / m_iMaxTileMarkerEntryPoints;
      m_acTEncDepthTopList[iViewIdx]->setMaxTileMarkerOffset         ( m_dMaxTileMarkerOffset );
      m_acTEncDepthTopList[iViewIdx]->setTileBehaviorControlPresentFlag( m_iTileBehaviorControlPresentFlag );
#if !REMOVE_TILE_DEPENDENCE
      if(m_iTileBoundaryIndependenceIdr == 0 || uiTilesCount == 1)
#else
      if(uiTilesCount == 1)
#endif
      {
        m_bLFCrossTileBoundaryFlag = true; 
      }
      m_acTEncDepthTopList[iViewIdx]->setLFCrossTileBoundaryFlag( m_bLFCrossTileBoundaryFlag );
      m_acTEncDepthTopList[iViewIdx]->setWaveFrontSynchro           ( m_iWaveFrontSynchro );
      m_acTEncDepthTopList[iViewIdx]->setWaveFrontFlush             ( m_iWaveFrontFlush );
      m_acTEncDepthTopList[iViewIdx]->setWaveFrontSubstreams        ( m_iWaveFrontSubstreams );
      m_acTEncDepthTopList[iViewIdx]->setEnableTMVP ( m_enableTMVP );
      m_acTEncDepthTopList[iViewIdx]->setUseScalingListId           ( m_useScalingListId  );
      m_acTEncDepthTopList[iViewIdx]->setScalingListFile            ( m_scalingListFile   );
#if MULTIBITS_DATA_HIDING
      m_acTEncDepthTopList[iViewIdx]->setSignHideFlag(m_signHideFlag);
      m_acTEncDepthTopList[iViewIdx]->setTSIG(m_signHidingThreshold);
#endif

#if LCU_SYNTAX_ALF
      if(uiTilesCount > 1)
      {
        m_bALFParamInSlice = false;
        m_bALFPicBasedEncode = true;
      }
      m_acTEncDepthTopList[iViewIdx]->setALFParamInSlice              ( m_bALFParamInSlice);
      m_acTEncDepthTopList[iViewIdx]->setALFPicBasedEncode            ( m_bALFPicBasedEncode);
#endif

  //====== Depth tools ========
#if HHI_DMM_WEDGE_INTRA || HHI_DMM_PRED_TEX
    m_acTEncDepthTopList[iViewIdx]->setUseDMM                     ( m_bUseDMM );
#endif
#if HHI_MPI
     m_acTEncDepthTopList[iViewIdx]->setUseMVI( m_bUseMVI );
#endif
    }
  }

#if HHI_INTER_VIEW_MOTION_PRED
  else if( m_uiMultiviewMvRegMode )
  {
    for(Int iViewIdx=0; iViewIdx<m_iNumberOfViews; iViewIdx++)
    {
      m_acTVideoIOYuvDepthInputFileList.push_back( new TVideoIOYuv );
    }
  }
#endif

#if HHI_VSO
  if ( m_bUseVSO )
  {
    if ( m_uiVSOMode == 4 )
    {
#if HHI_VSO_SPEEDUP_A0033
#if LGE_VSO_EARLY_SKIP_A0093
      m_cRendererModel.create( m_cRenModStrParser.getNumOfBaseViews(), m_cRenModStrParser.getNumOfModels(), m_iSourceWidth, g_uiMaxCUHeight , LOG2_DISP_PREC_LUT, 0, m_bVSOEarlySkip );
#else
      m_cRendererModel.create( m_cRenModStrParser.getNumOfBaseViews(), m_cRenModStrParser.getNumOfModels(), m_iSourceWidth, g_uiMaxCUHeight , LOG2_DISP_PREC_LUT, 0 );
#endif
#else
      m_cRendererModel.create( m_cRenModStrParser.getNumOfBaseViews(), m_cRenModStrParser.getNumOfModels(), m_iSourceWidth, m_iSourceHeight, LOG2_DISP_PREC_LUT, 0 );
#endif

      for ( Int iViewNum = 0; iViewNum < m_iNumberOfViews; iViewNum++ )
      {
        for (Int iContent = 0; iContent < 2; iContent++ )
        {
          Int iNumOfModels   = m_cRenModStrParser.getNumOfModelsForView(iViewNum, iContent);
          Bool bUseVSO = (iNumOfModels != 0);

          TEncTop* pcEncTop = ( iContent == 0 ) ? m_acTEncTopList[iViewNum] : m_acTEncDepthTopList[iViewNum];
          pcEncTop->setUseVSO( bUseVSO );
          pcEncTop->getRdCost()->setRenModel( bUseVSO ? &m_cRendererModel : NULL );

          for (Int iCurModel = 0; iCurModel < iNumOfModels; iCurModel++ )
          {
            Int iModelNum; Int iLeftViewNum; Int iRightViewNum; Int iDump; Int iOrgRefNum; Int iBlendMode;

            m_cRenModStrParser.getSingleModelData  ( iViewNum, iContent, iCurModel, iModelNum, iBlendMode, iLeftViewNum, iRightViewNum, iOrgRefNum, iDump ) ;
            m_cRendererModel  .createSingleModel   ( iViewNum, iContent, iModelNum, iLeftViewNum, iRightViewNum, (iOrgRefNum != -1), iBlendMode );
          }
        }
      }
    }
    else
    {
      AOT(true);
    }
#if LGE_WVSO_A0119 
    for ( Int iViewNum = 0; iViewNum < m_iNumberOfViews; iViewNum++ )
    {
      for (Int iContent = 0; iContent < 2; iContent++ )
      {
        TEncTop* pcEncTop = ( iContent == 0 ) ? m_acTEncTopList[iViewNum] : m_acTEncDepthTopList[iViewNum]; 
        pcEncTop->setWVSO( m_bWVSO );
        pcEncTop->setVSOWeight( m_iVSOWeight );
        pcEncTop->setVSDWeight( m_iVSDWeight );
        pcEncTop->setDWeight( m_iDWeight );
      }
    }
#endif
  }
#endif

#if HHI_INTERVIEW_SKIP
  m_cUsedPelsRenderer.init(m_iSourceWidth, m_iSourceHeight, true, 0, LOG2_DISP_PREC_LUT, true, 0, 0, 0, 0, 0, 6, 4, 1, 0, 6 );
#endif

}

Void TAppEncTop::xCreateLib()
{
  for(Int iViewIdx=0; iViewIdx<m_iNumberOfViews; iViewIdx++)
  {
    m_acTVideoIOYuvInputFileList[iViewIdx]->open( m_pchInputFileList[iViewIdx],     false, m_uiInputBitDepth, m_uiInternalBitDepth );  // read  mode
    m_acTVideoIOYuvInputFileList[iViewIdx]->skipFrames(m_FrameSkip, m_iSourceWidth - m_aiPad[0], m_iSourceHeight - m_aiPad[1]);

    if (m_pchReconFileList[iViewIdx])
      m_acTVideoIOYuvReconFileList[iViewIdx]->open(m_pchReconFileList[iViewIdx], true, m_uiOutputBitDepth, m_uiInternalBitDepth);  // write mode
    m_acTEncTopList[iViewIdx]->create();
    if( m_bUsingDepthMaps )
    {
      m_acTVideoIOYuvDepthInputFileList[iViewIdx]->open( m_pchDepthInputFileList[iViewIdx],     false, m_uiInputBitDepth, m_uiInternalBitDepth );  // read  mode
      m_acTVideoIOYuvDepthInputFileList[iViewIdx]->skipFrames(m_FrameSkip, m_iSourceWidth - m_aiPad[0], m_iSourceHeight - m_aiPad[1]);

      if (m_pchDepthReconFileList[iViewIdx])
        m_acTVideoIOYuvDepthReconFileList[iViewIdx]->open(m_pchDepthReconFileList[iViewIdx], true, m_uiOutputBitDepth, m_uiInternalBitDepth);  // write mode
      m_acTEncDepthTopList[iViewIdx]->create();
    }
#if HHI_INTER_VIEW_MOTION_PRED
    else if( m_uiMultiviewMvRegMode )
    {
      m_acTVideoIOYuvDepthInputFileList[iViewIdx]->open( m_pchDepthInputFileList[iViewIdx],     false, m_uiInputBitDepth, m_uiInternalBitDepth );  // read  mode
      m_acTVideoIOYuvDepthInputFileList[iViewIdx]->skipFrames(m_FrameSkip, m_iSourceWidth, m_iSourceHeight);
    }
#endif
  }
}

Void TAppEncTop::xDestroyLib()
{
  // Video I/O
  for(Int iViewIdx=0; iViewIdx<m_iNumberOfViews; iViewIdx++)
  {
    m_acTVideoIOYuvInputFileList[iViewIdx]->close();
    m_acTVideoIOYuvReconFileList[iViewIdx]->close();
    delete m_acTVideoIOYuvInputFileList[iViewIdx] ; 
    m_acTVideoIOYuvInputFileList[iViewIdx] = NULL;
    delete m_acTVideoIOYuvReconFileList[iViewIdx] ; 
    m_acTVideoIOYuvReconFileList[iViewIdx] = NULL;
    m_acTEncTopList[iViewIdx]->deletePicBuffer();
    m_acTEncTopList[iViewIdx]->destroy();
    delete m_acTEncTopList[iViewIdx] ; 
    m_acTEncTopList[iViewIdx] = NULL;
    delete m_picYuvRec[iViewIdx] ; 
    m_picYuvRec[iViewIdx] = NULL;

    if( iViewIdx < Int( m_acTVideoIOYuvDepthInputFileList.size() ) && m_acTVideoIOYuvDepthInputFileList[iViewIdx] )
    {
      m_acTVideoIOYuvDepthInputFileList[iViewIdx]->close( );
      delete m_acTVideoIOYuvDepthInputFileList[iViewIdx];
      m_acTVideoIOYuvDepthInputFileList[iViewIdx] = NULL;
    }
    if( iViewIdx < Int( m_acTVideoIOYuvDepthReconFileList.size() ) && m_acTVideoIOYuvDepthReconFileList[iViewIdx] )
    {
      m_acTVideoIOYuvDepthReconFileList[iViewIdx]->close () ;
      delete m_acTVideoIOYuvDepthReconFileList[iViewIdx];
      m_acTVideoIOYuvDepthReconFileList[iViewIdx] = NULL;
    }
    if( iViewIdx < Int( m_acTEncDepthTopList.size() ) && m_acTEncDepthTopList[iViewIdx] )
    {
      m_acTEncDepthTopList[iViewIdx]->deletePicBuffer();
      m_acTEncDepthTopList[iViewIdx]->destroy();
      delete m_acTEncDepthTopList[iViewIdx];
      m_acTEncDepthTopList[iViewIdx] = NULL;
    }
    if( iViewIdx < Int( m_picYuvDepthRec.size() ) && m_picYuvDepthRec[iViewIdx] )
    {
      delete m_picYuvDepthRec[iViewIdx] ; 
      m_picYuvDepthRec[iViewIdx] = NULL;
    }
  }
}

Void TAppEncTop::xInitLib()
{
  for(Int iViewIdx=0; iViewIdx<m_iNumberOfViews; iViewIdx++)
  {
    m_acTEncTopList[iViewIdx]->init( this );
  }
  for(Int iViewIdx=0; iViewIdx<m_iNumberOfViews; iViewIdx++)
  {
    m_acTEncTopList[iViewIdx]->setTEncTopList( &m_acTEncTopList  );
  }
  if ( m_bUsingDepthMaps )
  {
    for(Int iViewIdx=0; iViewIdx<m_iNumberOfViews; iViewIdx++)
    {
      m_acTEncDepthTopList[iViewIdx]->init( this );
    }
    for(Int iViewIdx=0; iViewIdx<m_iNumberOfViews; iViewIdx++)
    {
      m_acTEncDepthTopList[iViewIdx]->setTEncTopList( &m_acTEncDepthTopList  );
    }
  }
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
  TComPicYuv*       pcDepthPicYuvOrg = new TComPicYuv;
  TComPicYuv*       pcPdmDepthOrg    = new TComPicYuv;
  TComPicYuv*       pcPicYuvRec = NULL;
  TComPicYuv*       pcDepthPicYuvRec = NULL;
  
  // initialize internal class & member variables
  xInitLibCfg();
  xCreateLib();
  xInitLib();
  
  // main encoder loop
  Bool  allEos = false;
  std::vector<Bool>  eos ;
  std::vector<Bool>  depthEos ;
  Int maxGopSize = 0;
  Int gopSize = 1;
  
  list<AccessUnit> outputAccessUnits; ///< list of access units to write out.  is populated by the encoding process
  maxGopSize = (std::max)(maxGopSize, m_acTEncTopList[0]->getGOPSize());   

  for(Int iViewIdx=0; iViewIdx < m_iNumberOfViews; iViewIdx++ )
  {
    eos.push_back( false );
    depthEos.push_back( false );
  }

  // allocate original YUV buffer
  pcPicYuvOrg->create( m_iSourceWidth, m_iSourceHeight, m_uiMaxCUWidth, m_uiMaxCUHeight, m_uiMaxCUDepth );
  pcDepthPicYuvOrg->create( m_iSourceWidth, m_iSourceHeight, m_uiMaxCUWidth, m_uiMaxCUHeight, m_uiMaxCUDepth );

#if HHI_INTER_VIEW_MOTION_PRED
  if( m_uiMultiviewMvRegMode )
  {
    pcPdmDepthOrg->create( m_iSourceWidth, m_iSourceHeight, m_uiMaxCUWidth, m_uiMaxCUHeight, m_uiMaxCUDepth );
  }
#endif

  while ( !allEos )
  {
    for(Int iViewIdx=0; iViewIdx < m_iNumberOfViews; iViewIdx++ )
    {
      Int frmCnt = 0;
      while ( !eos[iViewIdx] && !(frmCnt == gopSize))
      {
        // get buffers
        xGetBuffer(pcPicYuvRec, iViewIdx, false);

        // read input YUV file
        m_acTVideoIOYuvInputFileList[iViewIdx]->read( pcPicYuvOrg, m_aiPad );
      
#if HHI_INTER_VIEW_MOTION_PRED
        if( m_uiMultiviewMvRegMode && iViewIdx )
        {
          m_acTVideoIOYuvDepthInputFileList[iViewIdx]->read( pcPdmDepthOrg, m_aiPad, m_bUsingDepthMaps );
        }
#endif

#if HHI_INTER_VIEW_MOTION_PRED
        m_acTEncTopList[iViewIdx]->initNewPic( pcPicYuvOrg, ( m_uiMultiviewMvRegMode && iViewIdx ? pcPdmDepthOrg : 0 ) );
#else
        m_acTEncTopList[iViewIdx]->initNewPic( pcPicYuvOrg );
#endif

        // increase number of received frames
        m_frameRcvd[iViewIdx]++;
        frmCnt++;
        // check end of file
        eos[iViewIdx] = ( m_acTVideoIOYuvInputFileList[iViewIdx]->isEof() == 1 ?   true : false  );
        eos[iViewIdx] = ( m_frameRcvd[iViewIdx] == m_iFrameToBeEncoded ?    true : eos[iViewIdx]   );
        allEos = allEos|eos[iViewIdx] ; 
      }
      if( m_bUsingDepthMaps )
      {
        Int frmCntDepth = 0;
        while ( !depthEos[iViewIdx] && !(frmCntDepth == gopSize))
        {
          // get buffers
          xGetBuffer(pcDepthPicYuvRec, iViewIdx, true);

          // read input YUV file
          m_acTVideoIOYuvDepthInputFileList[iViewIdx]->read( pcDepthPicYuvOrg, m_aiPad );

          m_acTEncDepthTopList[iViewIdx]->initNewPic( pcDepthPicYuvOrg );

          // increase number of received frames
          m_depthFrameRcvd[iViewIdx]++;
          frmCntDepth++;
          // check end of file
          depthEos[iViewIdx] = ( m_acTVideoIOYuvDepthInputFileList[iViewIdx]->isEof() == 1 ?   true : false  );
          depthEos[iViewIdx] = ( m_depthFrameRcvd[iViewIdx] == m_iFrameToBeEncoded ?    true : depthEos[iViewIdx]   );
          allEos = allEos|depthEos[iViewIdx] ; 
        }
      }
    }
    for ( Int gopId=0; gopId < gopSize; gopId++ )
    {
      Int  iNumEncoded = 0;
      UInt iNextPoc = m_acTEncTopList[0] -> getFrameId( gopId );
      if ( iNextPoc < m_iFrameToBeEncoded )
      {
      m_cCameraData.update( iNextPoc );
      }
      for(Int iViewIdx=0; iViewIdx < m_iNumberOfViews; iViewIdx++ )
      {
#if SAIT_VSO_EST_A0033
        if( m_bUseVSO && iNextPoc < m_iFrameToBeEncoded )
        {
          m_cCameraData.xSetDispCoeff( iNextPoc, iViewIdx );
          m_acTEncDepthTopList[iViewIdx]->setDispCoeff( m_cCameraData.getDispCoeff() );
        }
#endif
        iNumEncoded = 0;
        // call encoding function for one frame
        m_acTEncTopList[iViewIdx]->encode( eos[iViewIdx], pcPicYuvOrg, *m_picYuvRec[iViewIdx], outputAccessUnits, iNumEncoded, gopId );
        xWriteOutput(bitstreamFile, iNumEncoded, outputAccessUnits, iViewIdx, false);
        outputAccessUnits.clear();
        if( m_bUsingDepthMaps )
        {
          Int  iNumDepthEncoded = 0;
          // call encoding function for one depth frame
          m_acTEncDepthTopList[iViewIdx]->encode( depthEos[iViewIdx], pcDepthPicYuvOrg, *m_picYuvDepthRec[iViewIdx], outputAccessUnits, iNumDepthEncoded, gopId );
          xWriteOutput(bitstreamFile, iNumDepthEncoded, outputAccessUnits, iViewIdx, true);
          outputAccessUnits.clear();
        }
      }
#if HHI_INTERVIEW_SKIP || HHI_INTER_VIEW_MOTION_PRED || HHI_INTER_VIEW_RESIDUAL_PRED
      for( Int iViewIdx = 0; iViewIdx < m_iNumberOfViews; iViewIdx++ )
      {
        if( iViewIdx < (Int)m_acTEncTopList.size() && m_acTEncTopList[iViewIdx] )
        {
          m_acTEncTopList[iViewIdx]->deleteExtraPicBuffers( m_acTEncTopList[iViewIdx]->getGOPEncoder()->getPocLastCoded() );
        }
        if( iViewIdx < (Int)m_acTEncDepthTopList.size() && m_acTEncDepthTopList[iViewIdx] )
        {
          m_acTEncDepthTopList[iViewIdx]->deleteExtraPicBuffers( m_acTEncTopList[iViewIdx]->getGOPEncoder()->getPocLastCoded() );
        }
      }
#endif
      for(Int iViewIdx=0; iViewIdx < m_iNumberOfViews; iViewIdx++ )
      {
        m_acTEncTopList[iViewIdx]->compressMotion( m_acTEncTopList[iViewIdx]->getGOPEncoder()->getPocLastCoded() );
        if( m_bUsingDepthMaps )
        {
          m_acTEncDepthTopList[iViewIdx]->compressMotion( m_acTEncDepthTopList[iViewIdx]->getGOPEncoder()->getPocLastCoded() );
        }
      }
    }
    gopSize = maxGopSize;
  }
  // delete original YUV buffer
  pcPicYuvOrg->destroy();
  delete pcPicYuvOrg;
  pcPicYuvOrg = NULL;
  pcDepthPicYuvOrg->destroy();
  delete pcDepthPicYuvOrg;
  pcDepthPicYuvOrg = NULL;
  
  for(Int iViewIdx=0; iViewIdx < m_iNumberOfViews; iViewIdx++ )
  {
    m_acTEncTopList[iViewIdx]->printOutSummary(m_acTEncTopList[iViewIdx]->getNumAllPicCoded());
    if ( m_bUsingDepthMaps )
    {
      m_acTEncDepthTopList[iViewIdx]->printOutSummary(m_acTEncDepthTopList[iViewIdx]->getNumAllPicCoded());
    }
  }

  // delete buffers & classes
  xDeleteBuffer();
  xDestroyLib();

  return;
}

TEncTop*  TAppEncTop::getTEncTop( Int viewId, Bool isDepth )    
{ 
  if ( isDepth )  
  {
    return m_acTEncDepthTopList[viewId]; 
  } 
  else
  { 
    return m_acTEncTopList[viewId]; 
  } 
}


// ====================================================================================================================
// Protected member functions
// ====================================================================================================================

/**
 - application has picture buffer list with size of GOP
 - picture buffer list acts as ring buffer
 - end of the list has the latest picture
 .
 */
Void TAppEncTop::xGetBuffer( TComPicYuv*& rpcPicYuvRec, Int iViewIdx, Bool isDepth)
{
  assert( m_iGOPSize > 0 );
  
  if( !isDepth )
  {
    if ( m_picYuvRec[iViewIdx]->size() == (UInt)m_iGOPSize )
    {
      rpcPicYuvRec = m_picYuvRec[iViewIdx]->popFront();
    }
    else
    {
      rpcPicYuvRec = new TComPicYuv; 
      rpcPicYuvRec->create( m_iSourceWidth, m_iSourceHeight, m_uiMaxCUWidth, m_uiMaxCUHeight, m_uiMaxCUDepth );
    }
    m_picYuvRec[iViewIdx]->pushBack( rpcPicYuvRec );
  }
  else
  {
    if ( m_picYuvDepthRec[iViewIdx]->size() == (UInt)m_iGOPSize )
    {
      rpcPicYuvRec = m_picYuvDepthRec[iViewIdx]->popFront();
    }
    else
    {
      rpcPicYuvRec = new TComPicYuv; 
      rpcPicYuvRec->create( m_iSourceWidth, m_iSourceHeight, m_uiMaxCUWidth, m_uiMaxCUHeight, m_uiMaxCUDepth );
    }
    m_picYuvDepthRec[iViewIdx]->pushBack( rpcPicYuvRec );
  }
}

Void TAppEncTop::xDeleteBuffer( )
{
  for(Int iViewIdx=0; iViewIdx<m_picYuvRec.size(); iViewIdx++)
  {
    if(m_picYuvRec[iViewIdx])
    {
      TComList<TComPicYuv*>::iterator iterPicYuvRec  = m_picYuvRec[iViewIdx]->begin();
      Int iSize = Int( m_picYuvRec[iViewIdx]->size() );
      for ( Int i = 0; i < iSize; i++ )
      {
        TComPicYuv*  pcPicYuvRec  = *(iterPicYuvRec++);
        if(pcPicYuvRec)
        {
          pcPicYuvRec->destroy();
          delete pcPicYuvRec; 
          pcPicYuvRec = NULL;
        }
      }
    } 
  }
  for(Int iViewIdx=0; iViewIdx<m_picYuvDepthRec.size(); iViewIdx++)
  {
    if(m_picYuvDepthRec[iViewIdx])
    {
      TComList<TComPicYuv*>::iterator iterPicYuvRec  = m_picYuvDepthRec[iViewIdx]->begin();
      Int iSize = Int( m_picYuvDepthRec[iViewIdx]->size() );
      for ( Int i = 0; i < iSize; i++ )
      {
        TComPicYuv*  pcPicYuvRec  = *(iterPicYuvRec++);
        if(pcPicYuvRec)
        {
          pcPicYuvRec->destroy();
          delete pcPicYuvRec; 
          pcPicYuvRec = NULL;
        }
      } 
    }
  }
}

/** \param iNumEncoded  number of encoded frames
 */
Void TAppEncTop::xWriteOutput(std::ostream& bitstreamFile, Int iNumEncoded, std::list<AccessUnit>& accessUnits, Int iViewIdx, Bool isDepth )
{
  Int i;
  
  if( iNumEncoded > 0 )
  {
    TComList<TComPicYuv*>::iterator iterPicYuvRec = !isDepth ? m_picYuvRec[iViewIdx]->end() : m_picYuvDepthRec[iViewIdx]->end();
  
    for ( i = 0; i < iNumEncoded; i++ )
    {
      --iterPicYuvRec;
    }
  
    for ( i = 0; i < iNumEncoded; i++ )
    {
      TComPicYuv*  pcPicYuvRec  = *(iterPicYuvRec++);
      if( !isDepth )
      {
        if (m_pchReconFileList[iViewIdx])
        {
#if PIC_CROPPING
          m_acTVideoIOYuvReconFileList[iViewIdx]->write( pcPicYuvRec, m_cropLeft, m_cropRight, m_cropTop, m_cropBottom );
#else
          m_acTVideoIOYuvReconFileList[iViewIdx]->write( pcPicYuvRec, m_aiPad );
#endif
        }
      }
      else
      {
        if (m_pchDepthReconFileList[iViewIdx])
        {
#if PIC_CROPPING
          m_acTVideoIOYuvDepthReconFileList[iViewIdx]->write( pcPicYuvRec, m_cropLeft, m_cropRight, m_cropTop, m_cropBottom );
#else
          m_acTVideoIOYuvDepthReconFileList[iViewIdx]->write( pcPicYuvRec, m_aiPad );
#endif
        }
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
}


TComPicYuv* TAppEncTop::xGetPicYuvFromView( Int iViewIdx, Int iPoc, Bool bDepth, Bool bRecon )
{
  TComPic*    pcPic = xGetPicFromView( iViewIdx, iPoc, bDepth);
  TComPicYuv* pcPicYuv = NULL;

  if (pcPic != NULL)
  {
    if( bRecon )
    {
      if ( pcPic->getReconMark() )
      {
        pcPicYuv = pcPic->getPicYuvRec();
      }
    }
    else
    {
      pcPicYuv = pcPic->getPicYuvOrg();
    }
  };

  return pcPicYuv;
};

/**
 *
 */
void TAppEncTop::rateStatsAccum(const AccessUnit& au, const std::vector<unsigned>& annexBsizes)
{
  AccessUnit::const_iterator it_au = au.begin();
  vector<unsigned>::const_iterator it_stats = annexBsizes.begin();

  for (; it_au != au.end(); it_au++, it_stats++)
  {
    switch ((*it_au)->m_nalUnitType)
    {
    case NAL_UNIT_CODED_SLICE:
#if H0566_TLA
    case NAL_UNIT_CODED_SLICE_IDV:
    case NAL_UNIT_CODED_SLICE_TLA:
    case NAL_UNIT_CODED_SLICE_CRA:
#else
    case NAL_UNIT_CODED_SLICE_DATAPART_A:
    case NAL_UNIT_CODED_SLICE_DATAPART_B:
    case NAL_UNIT_CODED_SLICE_CDR:
#endif
    case NAL_UNIT_CODED_SLICE_IDR:
    case NAL_UNIT_SPS:
    case NAL_UNIT_PPS:
#if VIDYO_VPS_INTEGRATION
    case NAL_UNIT_VPS:
#endif
      m_essentialBytes += *it_stats;
      break;
    default:
      break;
    }

    m_totalBytes += *it_stats;
  }
}

#if HHI_INTERVIEW_SKIP
Void TAppEncTop::getUsedPelsMap( Int iViewIdx, Int iPoc, TComPicYuv* pcPicYuvUsedSplsMap )
{
  AOT( iViewIdx <= 0);
  AOT( iViewIdx >= m_iNumberOfViews );
  AOF( m_cCameraData.getCurFrameId() == iPoc );
  Int iViewSIdx      = m_cCameraData.getBaseId2SortedId()[iViewIdx];
  Int iFirstViewSIdx = m_cCameraData.getBaseId2SortedId()[0];

  AOT( iViewSIdx == iFirstViewSIdx );

  Bool bFirstIsLeft = (iFirstViewSIdx < iViewSIdx);

  m_cUsedPelsRenderer.setShiftLUTs(
      m_cCameraData.getBaseViewShiftLUTD()[0][iViewIdx],
      m_cCameraData.getBaseViewShiftLUTI()[0][iViewIdx],
      m_cCameraData.getBaseViewShiftLUTI()[0][iViewIdx],
      m_cCameraData.getBaseViewShiftLUTD()[0][iViewIdx],
      m_cCameraData.getBaseViewShiftLUTI()[0][iViewIdx],
      m_cCameraData.getBaseViewShiftLUTI()[0][iViewIdx],
      -1
      );

  TComPicYuv* pcPicYuvDepth = xGetPicYuvFromView(0, iPoc, true, true );
  AOF( pcPicYuvDepth);

  m_cUsedPelsRenderer.getUsedSamplesMap( pcPicYuvDepth, pcPicYuvUsedSplsMap, bFirstIsLeft );
}
#endif
#if HHI_VSO
#if HHI_VSO_SPEEDUP_A0033
Void TAppEncTop::setupRenModel( Int iPoc, Int iEncViewIdx, Int iEncContent, Int iHorOffset )
{
  m_cRendererModel.setHorOffset( iHorOffset ); 
#else
Void TAppEncTop::setupRenModel( Int iPoc, Int iEncViewIdx, Int iEncContent )
{
#endif
  Int iEncViewSIdx = m_cCameraData.getBaseId2SortedId()[ iEncViewIdx ];

  // setup base views
  Int iNumOfBV = m_cRenModStrParser.getNumOfBaseViewsForView( iEncViewSIdx, iEncContent );

  for (Int iCurView = 0; iCurView < iNumOfBV; iCurView++ )
  {
    Int iBaseViewSIdx;
    Int iVideoDistMode;
    Int iDepthDistMode;

    m_cRenModStrParser.getBaseViewData( iEncViewSIdx, iEncContent, iCurView, iBaseViewSIdx, iVideoDistMode, iDepthDistMode );

    AOT( iVideoDistMode < 0 || iVideoDistMode > 2 );

    Int iBaseViewIdx = m_cCameraData.getBaseSortedId2Id()[ iBaseViewSIdx ];

    TComPicYuv* pcPicYuvVideoRec  = xGetPicYuvFromView( iBaseViewIdx, iPoc, false, true  );
    TComPicYuv* pcPicYuvDepthRec  = xGetPicYuvFromView( iBaseViewIdx, iPoc, true , true  );
    TComPicYuv* pcPicYuvVideoOrg  = xGetPicYuvFromView( iBaseViewIdx, iPoc, false, false );
    TComPicYuv* pcPicYuvDepthOrg  = xGetPicYuvFromView( iBaseViewIdx, iPoc, true , false );

    TComPicYuv* pcPicYuvVideoRef  = ( iVideoDistMode == 2 ) ? pcPicYuvVideoOrg  : NULL;
    TComPicYuv* pcPicYuvDepthRef  = ( iDepthDistMode == 2 ) ? pcPicYuvDepthOrg  : NULL;

    TComPicYuv* pcPicYuvVideoTest = ( iVideoDistMode == 0 ) ? pcPicYuvVideoOrg  : pcPicYuvVideoRec;
    TComPicYuv* pcPicYuvDepthTest = ( iDepthDistMode == 0 ) ? pcPicYuvDepthOrg  : pcPicYuvDepthRec;

    AOT( (iVideoDistMode == 2) != (pcPicYuvVideoRef != NULL) );
    AOT( (iDepthDistMode == 2) != (pcPicYuvDepthRef != NULL) );
    AOT( pcPicYuvDepthTest == NULL );
    AOT( pcPicYuvVideoTest == NULL );

    m_cRendererModel.setBaseView( iBaseViewSIdx, pcPicYuvVideoTest, pcPicYuvDepthTest, pcPicYuvVideoRef, pcPicYuvDepthRef );
  }

  m_cRendererModel.setErrorMode( iEncViewSIdx, iEncContent, 0 );
  // setup virtual views
  Int iNumOfSV  = m_cRenModStrParser.getNumOfModelsForView( iEncViewSIdx, iEncContent );
  for (Int iCurView = 0; iCurView < iNumOfSV; iCurView++ )
  {
    Int iOrgRefBaseViewSIdx;
    Int iLeftBaseViewSIdx;
    Int iRightBaseViewSIdx;
    Int iSynthViewRelNum;
    Int iModelNum;
    Int iBlendMode;
    m_cRenModStrParser.getSingleModelData(iEncViewSIdx, iEncContent, iCurView, iModelNum, iBlendMode,iLeftBaseViewSIdx, iRightBaseViewSIdx, iOrgRefBaseViewSIdx, iSynthViewRelNum );

    Int iLeftBaseViewIdx    = -1;
    Int iRightBaseViewIdx   = -1;

    TComPicYuv* pcPicYuvOrgRef  = NULL;
    Int**      ppiShiftLUTLeft  = NULL;
    Int**      ppiShiftLUTRight = NULL;
    Int**      ppiBaseShiftLUTLeft  = NULL;
    Int**      ppiBaseShiftLUTRight = NULL;


    Int        iDistToLeft      = -1;

    Int iSynthViewIdx = m_cCameraData.synthRelNum2Idx( iSynthViewRelNum );

    if ( iLeftBaseViewSIdx != -1 )
    {
      iLeftBaseViewIdx   = m_cCameraData.getBaseSortedId2Id()   [ iLeftBaseViewSIdx ];
      ppiShiftLUTLeft    = m_cCameraData.getSynthViewShiftLUTI()[ iLeftBaseViewIdx  ][ iSynthViewIdx  ];
    }

    if ( iRightBaseViewSIdx != -1 )
    {
      iRightBaseViewIdx  = m_cCameraData.getBaseSortedId2Id()   [iRightBaseViewSIdx ];
      ppiShiftLUTRight   = m_cCameraData.getSynthViewShiftLUTI()[ iRightBaseViewIdx ][ iSynthViewIdx ];
    }

    if ( iRightBaseViewSIdx != -1 && iLeftBaseViewSIdx != -1 )
    {
      iDistToLeft    = m_cCameraData.getRelDistLeft(  iSynthViewIdx , iLeftBaseViewIdx, iRightBaseViewIdx);
      ppiBaseShiftLUTLeft  = m_cCameraData.getBaseViewShiftLUTI() [ iLeftBaseViewIdx  ][ iRightBaseViewIdx ];
      ppiBaseShiftLUTRight = m_cCameraData.getBaseViewShiftLUTI() [ iRightBaseViewIdx ][ iLeftBaseViewIdx  ];

    }

    if ( iOrgRefBaseViewSIdx != -1 )
    {
      pcPicYuvOrgRef = xGetPicYuvFromView( m_cCameraData.getBaseSortedId2Id()[ iOrgRefBaseViewSIdx ] , iPoc, false, false );
      AOF ( pcPicYuvOrgRef );
    }

    m_cRendererModel.setSingleModel( iModelNum, ppiShiftLUTLeft, ppiBaseShiftLUTLeft, ppiShiftLUTRight, ppiBaseShiftLUTRight, iDistToLeft, pcPicYuvOrgRef );
  }
}
#endif



/*
void TAppEncTop::printRateSummary()
{
  double time = (double) m_iFrameRcvd / m_iFrameRate;
  printf("Bytes written to file: %u (%.3f kbps)\n", m_totalBytes, 0.008 * m_totalBytes / time);
#if VERBOSE_RATE
  printf("Bytes for SPS/PPS/Slice (Incl. Annex B): %u (%.3f kbps)\n", m_essentialBytes, 0.008 * m_essentialBytes / time);
#endif
}
*/

std::vector<TComPic*> TAppEncTop::getInterViewRefPics( Int viewId, Int poc, Bool isDepth, TComSPS* sps )
{
  std::vector<TComPic*> apcRefPics( sps->getNumberOfUsableInterViewRefs(), (TComPic*)NULL );
  for( Int k = 0; k < sps->getNumberOfUsableInterViewRefs(); k++ )
  {
    TComPic* pcRefPic = xGetPicFromView( sps->getUsableInterViewRef( k ) + viewId, poc, isDepth );
    assert( pcRefPic != NULL );
    apcRefPics[k] = pcRefPic;
  }
  return apcRefPics;
}

TComPic* TAppEncTop::xGetPicFromView( Int viewIdx, Int poc, Bool isDepth )
{
  assert( ( viewIdx >= 0 ) && ( viewIdx < m_iNumberOfViews ) );

  TComList<TComPic*>* apcListPic = (isDepth ?  m_acTEncDepthTopList[viewIdx] : m_acTEncTopList[viewIdx])->getListPic() ;

  TComPic* pcPic = NULL;
  for(TComList<TComPic*>::iterator it=apcListPic->begin(); it!=apcListPic->end(); it++)
  {
    if( (*it)->getPOC() == poc )
    {
      pcPic = *it ;
      break ;
    }
  }

  return pcPic;
};

//! \}
