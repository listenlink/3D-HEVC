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

/** \file     TAppEncTop.cpp
    \brief    Encoder application class
*/

#include <list>
#include <fstream>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <assert.h>
#include <iomanip>

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

#if NH_MV
  m_vps = new TComVPS; 
#else
  m_iFrameRcvd = 0;
#endif
  m_totalBytes = 0;
  m_essentialBytes = 0;
}

TAppEncTop::~TAppEncTop()
{
#if NH_MV
  if (m_vps)
  {
   delete m_vps; 
  };
#endif

}

Void TAppEncTop::xInitLibCfg()
{
#if NH_MV
  TComVPS& vps = (*m_vps);   
#else
  TComVPS vps;
#endif
  
#if NH_3D
  vps.createCamPars(m_iNumberOfViews);  
#endif

#if NH_3D_DLT
  TComDLT dlt = TComDLT();
#endif

#if NH_MV
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
  vps.setMaxLayersMinus1( m_numberOfLayers - 1);
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
  vps.setMaxTLayers                                               ( m_maxTempLayer );
  if (m_maxTempLayer == 1)
  {
    vps.setTemporalNestingFlag(true);
  }
  vps.setMaxLayers                                                ( 1 );
  for(Int i = 0; i < MAX_TLAYER; i++)
  {
    vps.setNumReorderPics                                         ( m_numReorderPics[i], i );
    vps.setMaxDecPicBuffering                                     ( m_maxDecPicBuffering[i], i );
  }
#endif
#if NH_MV
  xSetTimingInfo           ( vps );
  xSetHrdParameters        ( vps ); 
  xSetLayerIds             ( vps );   
  xSetDimensionIdAndLength ( vps );
  xSetDependencies         ( vps );
  xSetRepFormat            ( vps ); 
  xSetProfileTierLevel     ( vps ); 
  xSetLayerSets            ( vps ); 
  xSetDpbSize              ( vps ); 
  xSetVPSVUI               ( vps ); 
#if NH_3D
  xSetCamPara              ( vps ); 
  m_ivPicLists.setVPS      ( &vps );
#endif
#if NH_3D_DLT
  xDeriveDltArray          ( vps, &dlt );
#endif
  if ( m_targetEncLayerIdList.size() == 0 )
  {
    for (Int i = 0; i < m_numberOfLayers; i++ )
    {
      m_targetEncLayerIdList.push_back( vps.getLayerIdInNuh( i ) );
    }
  }
  for( Int i = (Int) m_targetEncLayerIdList.size()-1 ; i >= 0 ; i--)
  {
    Int iNuhLayerId = m_targetEncLayerIdList[i]; 
    Bool allRefLayersPresent = true; 
    for( Int j = 0; j < vps.getNumRefLayers( iNuhLayerId ); j++)
    {
      allRefLayersPresent = allRefLayersPresent && xLayerIdInTargetEncLayerIdList( vps.getIdRefLayer( iNuhLayerId, j) );
    }
    if ( !allRefLayersPresent )
    {
      printf("\nCannot encode layer with nuh_layer_id equal to %d since not all reference layers are in TargetEncLayerIdList\n", iNuhLayerId);
      m_targetEncLayerIdList.erase( m_targetEncLayerIdList.begin() + i  );
    }
  }

  if ( m_outputVpsInfo )
  {  
    vps.printScalabilityId();
    vps.printLayerDependencies();
    vps.printLayerSets(); 
    vps.printPTL(); 
  }

#if NH_3D
  // Set 3d tool parameters
  for (Int d = 0; d < 2; d++)
  {  
    m_sps3dExtension.setIvMvPredFlag          ( d, m_ivMvPredFlag[d]       );
    m_sps3dExtension.setIvMvScalingFlag       ( d, m_ivMvScalingFlag[d]    );
    if (d == 0 )
    {    
      m_sps3dExtension.setLog2SubPbSizeMinus3   ( d, m_log2SubPbSizeMinus3   );
      m_sps3dExtension.setIvResPredFlag         ( d, m_ivResPredFlag         );
      m_sps3dExtension.setDepthRefinementFlag   ( d, m_depthRefinementFlag   );
      m_sps3dExtension.setViewSynthesisPredFlag ( d, m_viewSynthesisPredFlag );
      m_sps3dExtension.setDepthBasedBlkPartFlag ( d, m_depthBasedBlkPartFlag );
    }
    else
    {    
      m_sps3dExtension.setMpiFlag               ( d, m_mpiFlag               );
      m_sps3dExtension.setLog2MpiSubPbSizeMinus3( d, m_log2MpiSubPbSizeMinus3);
      m_sps3dExtension.setIntraContourFlag      ( d, m_intraContourFlag      );
      m_sps3dExtension.setIntraSdcWedgeFlag     ( d, m_intraSdcFlag || m_intraWedgeFlag     );
      m_sps3dExtension.setQtPredFlag            ( d, m_qtPredFlag            );
      m_sps3dExtension.setInterSdcFlag          ( d, m_interSdcFlag          );
      m_sps3dExtension.setDepthIntraSkipFlag    ( d, m_depthIntraSkipFlag    );  
    }
  }
#endif


  /// Create encoders and set profiles profiles
  for(Int layerIdInVps = 0; layerIdInVps < m_numberOfLayers; layerIdInVps++)
  {
    m_frameRcvd                 .push_back(0);
    m_acTEncTopList             .push_back(new TEncTop); 
    m_acTVideoIOYuvInputFileList.push_back(new TVideoIOYuv);
    m_acTVideoIOYuvReconFileList.push_back(new TVideoIOYuv);
#if NH_3D    
    Int profileIdc = -1; 
    for (Int olsIdx = 0; olsIdx < vps.getNumOutputLayerSets(); olsIdx++ )
    {   
      Int lsIdx = vps.olsIdxToLsIdx( olsIdx );
      for(Int i = 0; i < vps.getNumLayersInIdList( lsIdx ); i++ )
      {
        if( vps.getLayerIdInNuh( layerIdInVps) == vps.getLayerSetLayerIdList(lsIdx, i) )
        {
          Int ptlIdx = vps.getProfileTierLevelIdx( olsIdx, i );
          if ( ptlIdx != -1 )
          {
            Int curProfileIdc = vps.getPTL(ptlIdx)->getGeneralPTL()->getProfileIdc(); 
            if (profileIdc == -1)   
            {
              profileIdc = curProfileIdc; 
            }
            else
            {   
              if ( profileIdc != curProfileIdc )
              {              
                fprintf(stderr, "Error: ProfileIdc for layer with index %d in VPS not equal in all OLSs. \n", layerIdInVps );
                exit(EXIT_FAILURE);
              }
            }
          }
        }
      }
    }

    if (profileIdc == -1 )
    {
      fprintf(stderr, "Error: No profile given for layer with index %d in VPS not equal in all OLS. \n", layerIdInVps );
      exit(EXIT_FAILURE);
    }
    m_acTEncTopList[ layerIdInVps ]->setProfileIdc( profileIdc ); 
#endif
  }


  for(Int layerIdInVps = 0; layerIdInVps < m_numberOfLayers; layerIdInVps++)
  {
    m_cListPicYuvRec            .push_back(new TComList<TComPicYuv*>) ;
    m_ivPicLists.push_back( m_acTEncTopList[ layerIdInVps ]->getListPic()  ); 
    TEncTop& m_cTEncTop = *m_acTEncTopList[ layerIdInVps ];  // It is not a member, but this name helps avoiding code duplication !!!

    Int layerId = vps.getLayerIdInNuh( layerIdInVps );
    m_cTEncTop.setLayerIdInVps                 ( layerIdInVps ); 
    m_cTEncTop.setLayerId                      ( layerId );    
    m_cTEncTop.setViewId                       ( vps.getViewId      (  layerId ) );
    m_cTEncTop.setViewIndex                    ( vps.getViewIndex   (  layerId ) );
#if NH_3D
    Bool isDepth = ( vps.getDepthId     ( layerId ) != 0 ) ;
    m_cTEncTop.setIsDepth                      ( isDepth );
    //====== Camera Parameters =========
    m_cTEncTop.setCameraParameters             ( &m_cCameraData );     
#if NH_3D_VSO
    //====== VSO =========
    m_cTEncTop.setRenderModelParameters        ( &m_cRenModStrParser ); 
    m_cTEncTop.setForceLambdaScaleVSO          ( isDepth ? m_bForceLambdaScaleVSO : false );
    m_cTEncTop.setLambdaScaleVSO               ( isDepth ? m_dLambdaScaleVSO      : 1     );
    m_cTEncTop.setVSOMode                      ( isDepth ? m_uiVSOMode            : 0     );

    m_cTEncTop.setAllowNegDist                 ( isDepth ? m_bAllowNegDist        : false );

    // SAIT_VSO_EST_A0033
    m_cTEncTop.setUseEstimatedVSD              ( isDepth ? m_bUseEstimatedVSD     : false );

    // LGE_WVSO_A0119
    m_cTEncTop.setUseWVSO                      ( isDepth ? m_bUseWVSO             : false );   
    m_cTEncTop.setVSOWeight                    ( isDepth ? m_iVSOWeight           : 0     );
    m_cTEncTop.setVSDWeight                    ( isDepth ? m_iVSDWeight           : 0     );
    m_cTEncTop.setDWeight                      ( isDepth ? m_iDWeight             : 0     );
#endif // H_3D_VSO
#if H_3D_IC
    m_cTEncTop.setUseIC                        ( vps.getViewIndex( layerId ) == 0 || isDepth ? false : m_abUseIC );
    m_cTEncTop.setUseICLowLatencyEnc           ( m_bUseLowLatencyICEnc );
#endif

    
    m_cTEncTop.setUseDMM                       ( isDepth ? m_intraWedgeFlag   : false );
    m_cTEncTop.setUseSDC                       ( isDepth ? m_intraSdcFlag     : false );
    m_cTEncTop.setUseDLT                       ( isDepth ? m_useDLT   : false );
    m_cTEncTop.setUseQTL                       ( isDepth ? m_bUseQTL  : false );
    m_cTEncTop.setSps3dExtension               ( m_sps3dExtension );
#endif // H_3D

    m_cTEncTop.setIvPicLists                   ( &m_ivPicLists ); 
#endif  // NH_MV
  m_cTEncTop.setVPS(&vps);

#if NH_3D_DLT
  m_cTEncTop.setDLT(dlt);
#endif

#if NH_MV
  m_cTEncTop.setProfile                                           ( m_profiles[0]);
  m_cTEncTop.setLevel                                             ( m_levelTier[0], m_level[0] );
#else
  m_cTEncTop.setProfile                                           ( m_profile);
  m_cTEncTop.setLevel                                             ( m_levelTier, m_level);
#endif
  m_cTEncTop.setProgressiveSourceFlag                             ( m_progressiveSourceFlag);
  m_cTEncTop.setInterlacedSourceFlag                              ( m_interlacedSourceFlag);
  m_cTEncTop.setNonPackedConstraintFlag                           ( m_nonPackedConstraintFlag);
  m_cTEncTop.setFrameOnlyConstraintFlag                           ( m_frameOnlyConstraintFlag);
  m_cTEncTop.setBitDepthConstraintValue                           ( m_bitDepthConstraint );
  m_cTEncTop.setChromaFormatConstraintValue                       ( m_chromaFormatConstraint );
  m_cTEncTop.setIntraConstraintFlag                               ( m_intraConstraintFlag );
  m_cTEncTop.setOnePictureOnlyConstraintFlag                      ( m_onePictureOnlyConstraintFlag );
  m_cTEncTop.setLowerBitRateConstraintFlag                        ( m_lowerBitRateConstraintFlag );

  m_cTEncTop.setPrintMSEBasedSequencePSNR                         ( m_printMSEBasedSequencePSNR);
  m_cTEncTop.setPrintFrameMSE                                     ( m_printFrameMSE);
  m_cTEncTop.setPrintSequenceMSE                                  ( m_printSequenceMSE);
  m_cTEncTop.setCabacZeroWordPaddingEnabled                       ( m_cabacZeroWordPaddingEnabled );

  m_cTEncTop.setFrameRate                                         ( m_iFrameRate );
  m_cTEncTop.setFrameSkip                                         ( m_FrameSkip );
  m_cTEncTop.setSourceWidth                                       ( m_iSourceWidth );
  m_cTEncTop.setSourceHeight                                      ( m_iSourceHeight );
  m_cTEncTop.setConformanceWindow                                 ( m_confWinLeft, m_confWinRight, m_confWinTop, m_confWinBottom );
  m_cTEncTop.setFramesToBeEncoded                                 ( m_framesToBeEncoded );

  //====== Coding Structure ========
#if NH_MV
  m_cTEncTop.setIntraPeriod                                       ( m_iIntraPeriod[ layerIdInVps ] );
#else
  m_cTEncTop.setIntraPeriod                                       ( m_iIntraPeriod );
#endif
  m_cTEncTop.setDecodingRefreshType                               ( m_iDecodingRefreshType );
  m_cTEncTop.setGOPSize                                           ( m_iGOPSize );
#if NH_MV
  m_cTEncTop.setGopList                                           ( m_GOPListMvc[layerIdInVps] );
  m_cTEncTop.setExtraRPSs                                         ( m_extraRPSsMvc[layerIdInVps] );
  for(Int i = 0; i < MAX_TLAYER; i++)
  {
    m_cTEncTop.setNumReorderPics                                  ( m_numReorderPicsMvc[layerIdInVps][i], i );
    m_cTEncTop.setMaxDecPicBuffering                              ( m_maxDecPicBufferingMvc[layerIdInVps][i], i );
  }
#else
  m_cTEncTop.setGopList                                           ( m_GOPList );
  m_cTEncTop.setExtraRPSs                                         ( m_extraRPSs );
  for(Int i = 0; i < MAX_TLAYER; i++)
  {
    m_cTEncTop.setNumReorderPics                                  ( m_numReorderPics[i], i );
    m_cTEncTop.setMaxDecPicBuffering                              ( m_maxDecPicBuffering[i], i );
  }
#endif
  for( UInt uiLoop = 0; uiLoop < MAX_TLAYER; ++uiLoop )
  {
    m_cTEncTop.setLambdaModifier                                  ( uiLoop, m_adLambdaModifier[ uiLoop ] );
  }
#if NH_MV
  m_cTEncTop.setQP                                                ( m_iQP[layerIdInVps] );
#else
  m_cTEncTop.setQP                                                ( m_iQP );
#endif

  m_cTEncTop.setPad                                               ( m_aiPad );

#if NH_MV
  m_cTEncTop.setMaxTempLayer                                      ( m_maxTempLayerMvc[layerIdInVps] );
#else
  m_cTEncTop.setMaxTempLayer                                      ( m_maxTempLayer );
#endif
  m_cTEncTop.setUseAMP( m_enableAMP );

  //===== Slice ========

  //====== Loop/Deblock Filter ========
#if NH_MV
  m_cTEncTop.setLoopFilterDisable                                 ( m_bLoopFilterDisable[layerIdInVps]);
#else
  m_cTEncTop.setLoopFilterDisable                                 ( m_bLoopFilterDisable       );
#endif
  m_cTEncTop.setLoopFilterOffsetInPPS                             ( m_loopFilterOffsetInPPS );
  m_cTEncTop.setLoopFilterBetaOffset                              ( m_loopFilterBetaOffsetDiv2  );
  m_cTEncTop.setLoopFilterTcOffset                                ( m_loopFilterTcOffsetDiv2    );
  m_cTEncTop.setDeblockingFilterMetric                            ( m_DeblockingFilterMetric );

  //====== Motion search ========
  m_cTEncTop.setDisableIntraPUsInInterSlices                      ( m_bDisableIntraPUsInInterSlices );
  m_cTEncTop.setFastSearch                                        ( m_iFastSearch  );
  m_cTEncTop.setSearchRange                                       ( m_iSearchRange );
  m_cTEncTop.setBipredSearchRange                                 ( m_bipredSearchRange );
  m_cTEncTop.setClipForBiPredMeEnabled                            ( m_bClipForBiPredMeEnabled );
  m_cTEncTop.setFastMEAssumingSmootherMVEnabled                   ( m_bFastMEAssumingSmootherMVEnabled );

#if NH_MV
  m_cTEncTop.setUseDisparitySearchRangeRestriction                ( m_bUseDisparitySearchRangeRestriction );
  m_cTEncTop.setVerticalDisparitySearchRange                      ( m_iVerticalDisparitySearchRange );
#endif
  //====== Quality control ========
  m_cTEncTop.setMaxDeltaQP                                        ( m_iMaxDeltaQP  );
  m_cTEncTop.setMaxCuDQPDepth                                     ( m_iMaxCuDQPDepth  );
  m_cTEncTop.setDiffCuChromaQpOffsetDepth                         ( m_diffCuChromaQpOffsetDepth );
  m_cTEncTop.setChromaCbQpOffset                                  ( m_cbQpOffset     );
  m_cTEncTop.setChromaCrQpOffset                                  ( m_crQpOffset  );

#if NH_3D
  m_cTEncTop.setChromaFormatIdc                                   ( isDepth ? CHROMA_400 : m_chromaFormatIDC );
#else
  m_cTEncTop.setChromaFormatIdc                                   ( m_chromaFormatIDC  );
#endif

#if ADAPTIVE_QP_SELECTION
  m_cTEncTop.setUseAdaptQpSelect                                  ( m_bUseAdaptQpSelect   );
#endif

  m_cTEncTop.setUseAdaptiveQP                                     ( m_bUseAdaptiveQP  );
  m_cTEncTop.setQPAdaptationRange                                 ( m_iQPAdaptationRange );
  m_cTEncTop.setExtendedPrecisionProcessingFlag                   ( m_extendedPrecisionProcessingFlag );
  m_cTEncTop.setHighPrecisionOffsetsEnabledFlag                   ( m_highPrecisionOffsetsEnabledFlag );
  //====== Tool list ========
  m_cTEncTop.setDeltaQpRD                                         ( m_uiDeltaQpRD  );
  m_cTEncTop.setUseASR                                            ( m_bUseASR      );
  m_cTEncTop.setUseHADME                                          ( m_bUseHADME    );
#if NH_MV
  m_cTEncTop.setdQPs                                              ( m_aidQP[layerIdInVps]   );
#else
  m_cTEncTop.setdQPs                                              ( m_aidQP        );
#endif
  m_cTEncTop.setUseRDOQ                                           ( m_useRDOQ     );
  m_cTEncTop.setUseRDOQTS                                         ( m_useRDOQTS   );
#if T0196_SELECTIVE_RDOQ
  m_cTEncTop.setUseSelectiveRDOQ                                  ( m_useSelectiveRDOQ );
#endif
  m_cTEncTop.setRDpenalty                                         ( m_rdPenalty );
  m_cTEncTop.setMaxCUWidth                                        ( m_uiMaxCUWidth );
  m_cTEncTop.setMaxCUHeight                                       ( m_uiMaxCUHeight );
  m_cTEncTop.setMaxTotalCUDepth                                   ( m_uiMaxTotalCUDepth );
  m_cTEncTop.setLog2DiffMaxMinCodingBlockSize                     ( m_uiLog2DiffMaxMinCodingBlockSize );
  m_cTEncTop.setQuadtreeTULog2MaxSize                             ( m_uiQuadtreeTULog2MaxSize );
  m_cTEncTop.setQuadtreeTULog2MinSize                             ( m_uiQuadtreeTULog2MinSize );
  m_cTEncTop.setQuadtreeTUMaxDepthInter                           ( m_uiQuadtreeTUMaxDepthInter );
  m_cTEncTop.setQuadtreeTUMaxDepthIntra                           ( m_uiQuadtreeTUMaxDepthIntra );
  m_cTEncTop.setUseFastEnc                                        ( m_bUseFastEnc  );
  m_cTEncTop.setUseEarlyCU                                        ( m_bUseEarlyCU  );
  m_cTEncTop.setUseFastDecisionForMerge                           ( m_useFastDecisionForMerge  );
  m_cTEncTop.setUseCbfFastMode                                    ( m_bUseCbfFastMode  );
  m_cTEncTop.setUseEarlySkipDetection                             ( m_useEarlySkipDetection );
  m_cTEncTop.setCrossComponentPredictionEnabledFlag               ( m_crossComponentPredictionEnabledFlag );
  m_cTEncTop.setUseReconBasedCrossCPredictionEstimate             ( m_reconBasedCrossCPredictionEstimate );
#if NH_MV
  m_cTEncTop.setLog2SaoOffsetScale                                ( CHANNEL_TYPE_LUMA  , m_log2SaoOffsetScale[layerIdInVps][CHANNEL_TYPE_LUMA]   );
  m_cTEncTop.setLog2SaoOffsetScale                                ( CHANNEL_TYPE_CHROMA, m_log2SaoOffsetScale[layerIdInVps][CHANNEL_TYPE_CHROMA] );
#else
  m_cTEncTop.setLog2SaoOffsetScale                                ( CHANNEL_TYPE_LUMA  , m_log2SaoOffsetScale[CHANNEL_TYPE_LUMA]   );
  m_cTEncTop.setLog2SaoOffsetScale                                ( CHANNEL_TYPE_CHROMA, m_log2SaoOffsetScale[CHANNEL_TYPE_CHROMA] );
#endif
  m_cTEncTop.setUseTransformSkip                                  ( m_useTransformSkip      );
  m_cTEncTop.setUseTransformSkipFast                              ( m_useTransformSkipFast  );
  m_cTEncTop.setTransformSkipRotationEnabledFlag                  ( m_transformSkipRotationEnabledFlag );
  m_cTEncTop.setTransformSkipContextEnabledFlag                   ( m_transformSkipContextEnabledFlag   );
  m_cTEncTop.setPersistentRiceAdaptationEnabledFlag               ( m_persistentRiceAdaptationEnabledFlag );
  m_cTEncTop.setCabacBypassAlignmentEnabledFlag                   ( m_cabacBypassAlignmentEnabledFlag );
  m_cTEncTop.setLog2MaxTransformSkipBlockSize                     ( m_log2MaxTransformSkipBlockSize  );
  for (UInt signallingModeIndex = 0; signallingModeIndex < NUMBER_OF_RDPCM_SIGNALLING_MODES; signallingModeIndex++)
  {
    m_cTEncTop.setRdpcmEnabledFlag                                ( RDPCMSignallingMode(signallingModeIndex), m_rdpcmEnabledFlag[signallingModeIndex]);
  }
  m_cTEncTop.setUseConstrainedIntraPred                           ( m_bUseConstrainedIntraPred );
  m_cTEncTop.setFastUDIUseMPMEnabled                              ( m_bFastUDIUseMPMEnabled );
  m_cTEncTop.setFastMEForGenBLowDelayEnabled                      ( m_bFastMEForGenBLowDelayEnabled );
  m_cTEncTop.setUseBLambdaForNonKeyLowDelayPictures               ( m_bUseBLambdaForNonKeyLowDelayPictures );
  m_cTEncTop.setPCMLog2MinSize                                    ( m_uiPCMLog2MinSize);
  m_cTEncTop.setUsePCM                                            ( m_usePCM );

  // set internal bit-depth and constants
  for (UInt channelType = 0; channelType < MAX_NUM_CHANNEL_TYPE; channelType++)
  {
    m_cTEncTop.setBitDepth((ChannelType)channelType, m_internalBitDepth[channelType]);
    m_cTEncTop.setPCMBitDepth((ChannelType)channelType, m_bPCMInputBitDepthFlag ? m_MSBExtendedBitDepth[channelType] : m_internalBitDepth[channelType]);
  }

  m_cTEncTop.setPCMLog2MaxSize                                    ( m_pcmLog2MaxSize);
  m_cTEncTop.setMaxNumMergeCand                                   ( m_maxNumMergeCand );


  //====== Weighted Prediction ========
  m_cTEncTop.setUseWP                                             ( m_useWeightedPred      );
  m_cTEncTop.setWPBiPred                                          ( m_useWeightedBiPred   );
  //====== Parallel Merge Estimation ========
  m_cTEncTop.setLog2ParallelMergeLevelMinus2                      ( m_log2ParallelMergeLevel - 2 );

  //====== Slice ========
  m_cTEncTop.setSliceMode                                         ( (SliceConstraint) m_sliceMode );
  m_cTEncTop.setSliceArgument                                     ( m_sliceArgument            );

  //====== Dependent Slice ========
  m_cTEncTop.setSliceSegmentMode                                  (  (SliceConstraint) m_sliceSegmentMode );
  m_cTEncTop.setSliceSegmentArgument                              ( m_sliceSegmentArgument     );

  if(m_sliceMode == NO_SLICES )
  {
    m_bLFCrossSliceBoundaryFlag = true;
  }
  m_cTEncTop.setLFCrossSliceBoundaryFlag                          ( m_bLFCrossSliceBoundaryFlag );
#if NH_MV
  m_cTEncTop.setUseSAO ( m_bUseSAO[layerIdInVps] );
#else
  m_cTEncTop.setUseSAO                                            ( m_bUseSAO );
#endif
  m_cTEncTop.setTestSAODisableAtPictureLevel                      ( m_bTestSAODisableAtPictureLevel );
  m_cTEncTop.setSaoEncodingRate                                   ( m_saoEncodingRate );
  m_cTEncTop.setSaoEncodingRateChroma                             ( m_saoEncodingRateChroma );
  m_cTEncTop.setMaxNumOffsetsPerPic                               ( m_maxNumOffsetsPerPic);

  m_cTEncTop.setSaoCtuBoundary                                    ( m_saoCtuBoundary);
  m_cTEncTop.setPCMInputBitDepthFlag                              ( m_bPCMInputBitDepthFlag);
  m_cTEncTop.setPCMFilterDisableFlag                              ( m_bPCMFilterDisableFlag);

  m_cTEncTop.setIntraSmoothingDisabledFlag                        (!m_enableIntraReferenceSmoothing );
  m_cTEncTop.setDecodedPictureHashSEIEnabled                      ( m_decodedPictureHashSEIEnabled );
  m_cTEncTop.setRecoveryPointSEIEnabled                           ( m_recoveryPointSEIEnabled );
  m_cTEncTop.setBufferingPeriodSEIEnabled                         ( m_bufferingPeriodSEIEnabled );
  m_cTEncTop.setPictureTimingSEIEnabled                           ( m_pictureTimingSEIEnabled );
  m_cTEncTop.setToneMappingInfoSEIEnabled                         ( m_toneMappingInfoSEIEnabled );
  m_cTEncTop.setTMISEIToneMapId                                   ( m_toneMapId );
  m_cTEncTop.setTMISEIToneMapCancelFlag                           ( m_toneMapCancelFlag );
  m_cTEncTop.setTMISEIToneMapPersistenceFlag                      ( m_toneMapPersistenceFlag );
  m_cTEncTop.setTMISEICodedDataBitDepth                           ( m_toneMapCodedDataBitDepth );
  m_cTEncTop.setTMISEITargetBitDepth                              ( m_toneMapTargetBitDepth );
  m_cTEncTop.setTMISEIModelID                                     ( m_toneMapModelId );
  m_cTEncTop.setTMISEIMinValue                                    ( m_toneMapMinValue );
  m_cTEncTop.setTMISEIMaxValue                                    ( m_toneMapMaxValue );
  m_cTEncTop.setTMISEISigmoidMidpoint                             ( m_sigmoidMidpoint );
  m_cTEncTop.setTMISEISigmoidWidth                                ( m_sigmoidWidth );
  m_cTEncTop.setTMISEIStartOfCodedInterva                         ( m_startOfCodedInterval );
  m_cTEncTop.setTMISEINumPivots                                   ( m_numPivots );
  m_cTEncTop.setTMISEICodedPivotValue                             ( m_codedPivotValue );
  m_cTEncTop.setTMISEITargetPivotValue                            ( m_targetPivotValue );
  m_cTEncTop.setTMISEICameraIsoSpeedIdc                           ( m_cameraIsoSpeedIdc );
  m_cTEncTop.setTMISEICameraIsoSpeedValue                         ( m_cameraIsoSpeedValue );
  m_cTEncTop.setTMISEIExposureIndexIdc                            ( m_exposureIndexIdc );
  m_cTEncTop.setTMISEIExposureIndexValue                          ( m_exposureIndexValue );
  m_cTEncTop.setTMISEIExposureCompensationValueSignFlag           ( m_exposureCompensationValueSignFlag );
  m_cTEncTop.setTMISEIExposureCompensationValueNumerator          ( m_exposureCompensationValueNumerator );
  m_cTEncTop.setTMISEIExposureCompensationValueDenomIdc           ( m_exposureCompensationValueDenomIdc );
  m_cTEncTop.setTMISEIRefScreenLuminanceWhite                     ( m_refScreenLuminanceWhite );
  m_cTEncTop.setTMISEIExtendedRangeWhiteLevel                     ( m_extendedRangeWhiteLevel );
  m_cTEncTop.setTMISEINominalBlackLevelLumaCodeValue              ( m_nominalBlackLevelLumaCodeValue );
  m_cTEncTop.setTMISEINominalWhiteLevelLumaCodeValue              ( m_nominalWhiteLevelLumaCodeValue );
  m_cTEncTop.setTMISEIExtendedWhiteLevelLumaCodeValue             ( m_extendedWhiteLevelLumaCodeValue );
  m_cTEncTop.setChromaSamplingFilterHintEnabled                   ( m_chromaSamplingFilterSEIenabled );
  m_cTEncTop.setChromaSamplingHorFilterIdc                        ( m_chromaSamplingHorFilterIdc );
  m_cTEncTop.setChromaSamplingVerFilterIdc                        ( m_chromaSamplingVerFilterIdc );
  m_cTEncTop.setFramePackingArrangementSEIEnabled                 ( m_framePackingSEIEnabled );
  m_cTEncTop.setFramePackingArrangementSEIType                    ( m_framePackingSEIType );
  m_cTEncTop.setFramePackingArrangementSEIId                      ( m_framePackingSEIId );
  m_cTEncTop.setFramePackingArrangementSEIQuincunx                ( m_framePackingSEIQuincunx );
  m_cTEncTop.setFramePackingArrangementSEIInterpretation          ( m_framePackingSEIInterpretation );
  m_cTEncTop.setSegmentedRectFramePackingArrangementSEIEnabled    ( m_segmentedRectFramePackingSEIEnabled );
  m_cTEncTop.setSegmentedRectFramePackingArrangementSEICancel     ( m_segmentedRectFramePackingSEICancel );
  m_cTEncTop.setSegmentedRectFramePackingArrangementSEIType       ( m_segmentedRectFramePackingSEIType );
  m_cTEncTop.setSegmentedRectFramePackingArrangementSEIPersistence( m_segmentedRectFramePackingSEIPersistence );
  m_cTEncTop.setDisplayOrientationSEIAngle                        ( m_displayOrientationSEIAngle );
  m_cTEncTop.setTemporalLevel0IndexSEIEnabled                     ( m_temporalLevel0IndexSEIEnabled );
  m_cTEncTop.setGradualDecodingRefreshInfoEnabled                 ( m_gradualDecodingRefreshInfoEnabled );
  m_cTEncTop.setNoDisplaySEITLayer                                ( m_noDisplaySEITLayer );
  m_cTEncTop.setDecodingUnitInfoSEIEnabled                        ( m_decodingUnitInfoSEIEnabled );
  m_cTEncTop.setSOPDescriptionSEIEnabled                          ( m_SOPDescriptionSEIEnabled );
  m_cTEncTop.setScalableNestingSEIEnabled                         ( m_scalableNestingSEIEnabled );
#if NH_MV
  m_cTEncTop.setSubBitstreamPropSEIEnabled                        ( m_subBistreamPropSEIEnabled );
  if( m_subBistreamPropSEIEnabled )                               
  {                                                               
    m_cTEncTop.setNumAdditionalSubStreams                         ( m_sbPropNumAdditionalSubStreams );
    m_cTEncTop.setSubBitstreamMode                                ( m_sbPropSubBitstreamMode );
    m_cTEncTop.setOutputLayerSetIdxToVps                          ( m_sbPropOutputLayerSetIdxToVps );
    m_cTEncTop.setHighestSublayerId                               ( m_sbPropHighestSublayerId );
    m_cTEncTop.setAvgBitRate                                      ( m_sbPropAvgBitRate );
    m_cTEncTop.setMaxBitRate                                      ( m_sbPropMaxBitRate );
  }
#endif

  m_cTEncTop.setTMCTSSEIEnabled                                   ( m_tmctsSEIEnabled );
  m_cTEncTop.setTimeCodeSEIEnabled                                ( m_timeCodeSEIEnabled );
  m_cTEncTop.setNumberOfTimeSets                                  ( m_timeCodeSEINumTs );
  for(Int i = 0; i < m_timeCodeSEINumTs; i++)
  {
    m_cTEncTop.setTimeSet(m_timeSetArray[i], i);
  }
  m_cTEncTop.setKneeSEIEnabled                                    ( m_kneeSEIEnabled );
  m_cTEncTop.setKneeSEIId                                         ( m_kneeSEIId );
  m_cTEncTop.setKneeSEICancelFlag                                 ( m_kneeSEICancelFlag );
  m_cTEncTop.setKneeSEIPersistenceFlag                            ( m_kneeSEIPersistenceFlag );
  m_cTEncTop.setKneeSEIInputDrange                                ( m_kneeSEIInputDrange );
  m_cTEncTop.setKneeSEIInputDispLuminance                         ( m_kneeSEIInputDispLuminance );
  m_cTEncTop.setKneeSEIOutputDrange                               ( m_kneeSEIOutputDrange );
  m_cTEncTop.setKneeSEIOutputDispLuminance                        ( m_kneeSEIOutputDispLuminance );
  m_cTEncTop.setKneeSEINumKneePointsMinus1                        ( m_kneeSEINumKneePointsMinus1 );
  m_cTEncTop.setKneeSEIInputKneePoint                             ( m_kneeSEIInputKneePoint );
  m_cTEncTop.setKneeSEIOutputKneePoint                            ( m_kneeSEIOutputKneePoint );
  m_cTEncTop.setMasteringDisplaySEI                               ( m_masteringDisplay );

  m_cTEncTop.setTileUniformSpacingFlag                            ( m_tileUniformSpacingFlag );
  m_cTEncTop.setNumColumnsMinus1                                  ( m_numTileColumnsMinus1 );
  m_cTEncTop.setNumRowsMinus1                                     ( m_numTileRowsMinus1 );
  if(!m_tileUniformSpacingFlag)
  {
    m_cTEncTop.setColumnWidth                                     ( m_tileColumnWidth );
    m_cTEncTop.setRowHeight                                       ( m_tileRowHeight );
  }
  m_cTEncTop.xCheckGSParameters();
  Int uiTilesCount = (m_numTileRowsMinus1+1) * (m_numTileColumnsMinus1+1);
  if(uiTilesCount == 1)
  {
    m_bLFCrossTileBoundaryFlag = true;
  }
  m_cTEncTop.setLFCrossTileBoundaryFlag                           ( m_bLFCrossTileBoundaryFlag );
  m_cTEncTop.setWaveFrontSynchro                                  ( m_iWaveFrontSynchro );
  m_cTEncTop.setTMVPModeId                                        ( m_TMVPModeId );
  m_cTEncTop.setUseScalingListId                                  ( m_useScalingListId  );
  m_cTEncTop.setScalingListFile                                   ( m_scalingListFile   );
  m_cTEncTop.setSignHideFlag                                      ( m_signHideFlag);
#if KWU_RC_VIEWRC_E0227 || KWU_RC_MADPRED_E0227
  if(!m_cTEncTop.getIsDepth())    //only for texture
  {
    m_cTEncTop.setUseRateCtrl                                     ( m_RCEnableRateControl );
  }                                                          
  else                                                       
  {                                                          
    m_cTEncTop.setUseRateCtrl                                     ( 0 );
  }
#else
  m_cTEncTop.setUseRateCtrl                                       ( m_RCEnableRateControl );
#endif
#if !KWU_RC_VIEWRC_E0227
  m_cTEncTop.setTargetBitrate                                     ( m_RCTargetBitrate );
#endif
  m_cTEncTop.setKeepHierBit                                       ( m_RCKeepHierarchicalBit );
  m_cTEncTop.setLCULevelRC                                        ( m_RCLCULevelRC );
  m_cTEncTop.setUseLCUSeparateModel                               ( m_RCUseLCUSeparateModel );
  m_cTEncTop.setInitialQP                                         ( m_RCInitialQP );
  m_cTEncTop.setForceIntraQP                                      ( m_RCForceIntraQP );
#if KWU_RC_MADPRED_E0227
  if(m_cTEncTop.getUseRateCtrl() && !m_cTEncTop.getIsDepth())
  {
    m_cTEncTop.setUseDepthMADPred(layerIdInVps ? m_depthMADPred       : 0);
    if(m_cTEncTop.getUseDepthMADPred())
    {
      m_cTEncTop.setCamParam(&m_cCameraData);
    }
  }
#endif
#if KWU_RC_VIEWRC_E0227
  if(m_cTEncTop.getUseRateCtrl() && !m_cTEncTop.getIsDepth())
  {
    m_cTEncTop.setUseViewWiseRateCtrl(m_viewWiseRateCtrl);
    if(m_iNumberOfViews == 1)
    {
      if(m_viewWiseRateCtrl)
      {
        m_cTEncTop.setTargetBitrate(m_viewTargetBits[layerIdInVps>>1]);
      }
      else
      {
        m_cTEncTop.setTargetBitrate       ( m_RCTargetBitrate );
      }
    }
    else
    {
      if(m_viewWiseRateCtrl)
      {
        m_cTEncTop.setTargetBitrate(m_viewTargetBits[layerIdInVps>>1]);
      }
      else
      {
        if(m_iNumberOfViews == 2)
        {
          if(m_cTEncTop.getViewId() == 0)
          {
            m_cTEncTop.setTargetBitrate              ( (m_RCTargetBitrate*80)/100 );
          }
          else if(m_cTEncTop.getViewId() == 1)
          {
            m_cTEncTop.setTargetBitrate              ( (m_RCTargetBitrate*20)/100 );
          }
        }
        else if(m_iNumberOfViews == 3)
        {
          if(m_cTEncTop.getViewId() == 0)
          {
            m_cTEncTop.setTargetBitrate              ( (m_RCTargetBitrate*66)/100 );
          }
          else if(m_cTEncTop.getViewId() == 1)
          {
            m_cTEncTop.setTargetBitrate              ( (m_RCTargetBitrate*17)/100 );
          }
          else if(m_cTEncTop.getViewId() == 2)
          {
            m_cTEncTop.setTargetBitrate              ( (m_RCTargetBitrate*17)/100 );
          }
        }
        else
        {
          m_cTEncTop.setTargetBitrate              ( m_RCTargetBitrate );
        }
      }
    }
  }
#endif
  m_cTEncTop.setTransquantBypassEnableFlag                        ( m_TransquantBypassEnableFlag );
  m_cTEncTop.setCUTransquantBypassFlagForceValue                  ( m_CUTransquantBypassFlagForce );
  m_cTEncTop.setCostMode                                          ( m_costMode );
  m_cTEncTop.setUseRecalculateQPAccordingToLambda                 ( m_recalculateQPAccordingToLambda );
  m_cTEncTop.setUseStrongIntraSmoothing                           ( m_useStrongIntraSmoothing );
  m_cTEncTop.setActiveParameterSetsSEIEnabled                     ( m_activeParameterSetsSEIEnabled );
  m_cTEncTop.setVuiParametersPresentFlag                          ( m_vuiParametersPresentFlag );
  m_cTEncTop.setAspectRatioInfoPresentFlag                        ( m_aspectRatioInfoPresentFlag);
  m_cTEncTop.setAspectRatioIdc                                    ( m_aspectRatioIdc );
  m_cTEncTop.setSarWidth                                          ( m_sarWidth );
  m_cTEncTop.setSarHeight                                         ( m_sarHeight );
  m_cTEncTop.setOverscanInfoPresentFlag                           ( m_overscanInfoPresentFlag );
  m_cTEncTop.setOverscanAppropriateFlag                           ( m_overscanAppropriateFlag );
  m_cTEncTop.setVideoSignalTypePresentFlag                        ( m_videoSignalTypePresentFlag );
  m_cTEncTop.setVideoFormat                                       ( m_videoFormat );
  m_cTEncTop.setVideoFullRangeFlag                                ( m_videoFullRangeFlag );
  m_cTEncTop.setColourDescriptionPresentFlag                      ( m_colourDescriptionPresentFlag );
  m_cTEncTop.setColourPrimaries                                   ( m_colourPrimaries );
  m_cTEncTop.setTransferCharacteristics                           ( m_transferCharacteristics );
  m_cTEncTop.setMatrixCoefficients                                ( m_matrixCoefficients );
  m_cTEncTop.setChromaLocInfoPresentFlag                          ( m_chromaLocInfoPresentFlag );
  m_cTEncTop.setChromaSampleLocTypeTopField                       ( m_chromaSampleLocTypeTopField );
  m_cTEncTop.setChromaSampleLocTypeBottomField                    ( m_chromaSampleLocTypeBottomField );
  m_cTEncTop.setNeutralChromaIndicationFlag                       ( m_neutralChromaIndicationFlag );
  m_cTEncTop.setDefaultDisplayWindow                              ( m_defDispWinLeftOffset, m_defDispWinRightOffset, m_defDispWinTopOffset, m_defDispWinBottomOffset );
  m_cTEncTop.setFrameFieldInfoPresentFlag                         ( m_frameFieldInfoPresentFlag );
  m_cTEncTop.setPocProportionalToTimingFlag                       ( m_pocProportionalToTimingFlag );
  m_cTEncTop.setNumTicksPocDiffOneMinus1                          ( m_numTicksPocDiffOneMinus1    );
  m_cTEncTop.setBitstreamRestrictionFlag                          ( m_bitstreamRestrictionFlag );
  m_cTEncTop.setTilesFixedStructureFlag                           ( m_tilesFixedStructureFlag );
  m_cTEncTop.setMotionVectorsOverPicBoundariesFlag                ( m_motionVectorsOverPicBoundariesFlag );
  m_cTEncTop.setMinSpatialSegmentationIdc                         ( m_minSpatialSegmentationIdc );
  m_cTEncTop.setMaxBytesPerPicDenom                               ( m_maxBytesPerPicDenom );
  m_cTEncTop.setMaxBitsPerMinCuDenom                              ( m_maxBitsPerMinCuDenom );
  m_cTEncTop.setLog2MaxMvLengthHorizontal                         ( m_log2MaxMvLengthHorizontal );
  m_cTEncTop.setLog2MaxMvLengthVertical                           ( m_log2MaxMvLengthVertical );
  m_cTEncTop.setEfficientFieldIRAPEnabled                         ( m_bEfficientFieldIRAPEnabled );
  m_cTEncTop.setHarmonizeGopFirstFieldCoupleEnabled               ( m_bHarmonizeGopFirstFieldCoupleEnabled );

  m_cTEncTop.setSummaryOutFilename                                ( m_summaryOutFilename );
  m_cTEncTop.setSummaryPicFilenameBase                            ( m_summaryPicFilenameBase );
  m_cTEncTop.setSummaryVerboseness                                ( m_summaryVerboseness );

#if NH_MV
  }
#endif
#if NH_3D_VSO
  if ( m_bUseVSO )
  {
    if ( m_uiVSOMode == 4 )
    {
#if H_3D_VSO_EARLY_SKIP
      m_cRendererModel.create( m_cRenModStrParser.getNumOfBaseViews(), m_cRenModStrParser.getNumOfModels(), m_iSourceWidth, m_uiMaxCUHeight , LOG2_DISP_PREC_LUT, 0, m_bVSOEarlySkip );
#else
      m_cRendererModel.create( m_cRenModStrParser.getNumOfBaseViews(), m_cRenModStrParser.getNumOfModels(), m_iSourceWidth, m_uiMaxCUHeight , LOG2_DISP_PREC_LUT, 0 );
#endif
      for ( Int layer = 0; layer < m_numberOfLayers ; layer++ )
      {
        TEncTop* pcEncTop =  m_acTEncTopList[ layer ]; 
        Int iViewNum      = pcEncTop->getViewIndex(); 
        Int iContent      = pcEncTop->getIsDepth() ? 1 : 0; 
        Int iNumOfModels  = m_cRenModStrParser.getNumOfModelsForView(iViewNum, iContent);

        Bool bUseVSO      = (iNumOfModels != 0);

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
    else
    {
      AOT(true);
    }
  }
#endif
}

Void TAppEncTop::xCreateLib()
{
#if NH_MV
  // initialize global variables
  initROM();
#if NH_3D_DMM
  initWedgeLists( true );
#endif

  for( Int layer=0; layer < m_numberOfLayers; layer++)
  {
    m_acTVideoIOYuvInputFileList[layer]->open( m_pchInputFileList[layer],     false, m_inputBitDepth, m_MSBExtendedBitDepth, m_internalBitDepth );  // read  mode
    m_acTVideoIOYuvInputFileList[layer]->skipFrames( m_FrameSkip, m_iSourceWidth - m_aiPad[0], m_iSourceHeight - m_aiPad[1], m_InputChromaFormatIDC);

    if (m_pchReconFileList[layer])
    {
      m_acTVideoIOYuvReconFileList[layer]->open( m_pchReconFileList[layer], true, m_outputBitDepth, m_outputBitDepth, m_internalBitDepth);  // write mode
    }
    m_acTEncTopList[layer]->create();
  }
#else
  // Video I/O
  m_cTVideoIOYuvInputFile.open( m_pchInputFile,     false, m_inputBitDepth, m_MSBExtendedBitDepth, m_internalBitDepth );  // read  mode
  m_cTVideoIOYuvInputFile.skipFrames(m_FrameSkip, m_iSourceWidth - m_aiPad[0], m_iSourceHeight - m_aiPad[1], m_InputChromaFormatIDC);

  if (m_pchReconFile)
  {
    m_cTVideoIOYuvReconFile.open(m_pchReconFile, true, m_outputBitDepth, m_outputBitDepth, m_internalBitDepth);  // write mode
  }

  // Neo Decoder
  m_cTEncTop.create();
#endif
}

Void TAppEncTop::xDestroyLib()
{
#if NH_MV
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
    delete m_cListPicYuvRec[layer] ; 
    m_cListPicYuvRec[layer] = NULL;
  }
#else
  // Video I/O
  m_cTVideoIOYuvInputFile.close();
  m_cTVideoIOYuvReconFile.close();

  // Neo Decoder
  m_cTEncTop.destroy();
#endif
}

Void TAppEncTop::xInitLib(Bool isFieldCoding)
{
#if NH_MV
  for(Int layer=0; layer<m_numberOfLayers; layer++)
  {
#if KWU_RC_MADPRED_E0227
    m_acTEncTopList[layer]->init( isFieldCoding, this );
#else
    m_acTEncTopList[layer]->init( isFieldCoding );
#endif
  }
#else
  m_cTEncTop.init(isFieldCoding);
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

#if !NH_3D
  TComPicYuv*       pcPicYuvOrg = new TComPicYuv;
#endif
  TComPicYuv*       pcPicYuvRec = NULL;

  // initialize internal class & member variables
  xInitLibCfg();
  xCreateLib();
  xInitLib(m_isField);

  printChromaFormat();

  // main encoder loop
#if NH_MV
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
  const InputColourSpaceConversion ipCSC  =  m_inputColourSpaceConvert;
  const InputColourSpaceConversion snrCSC = (!m_snrInternalColourSpace) ? m_inputColourSpaceConvert : IPCOLOURSPACE_UNCHANGED;

  list<AccessUnit> outputAccessUnits; ///< list of access units to write out.  is populated by the encoding process
  
#if NH_3D
  TComPicYuv* picYuvOrg[2]; 
  TComPicYuv  picYuvTrueOrg[2]; 
  for (Int d = 0; d < 2 ; d++)
  {
    picYuvOrg[d] = new TComPicYuv;
    picYuvOrg[d]   ->create( m_iSourceWidth, m_isField ? m_iSourceHeightOrg : m_iSourceHeight, ( d > 0 ) ? CHROMA_400 : m_chromaFormatIDC, m_uiMaxCUWidth, m_uiMaxCUHeight, m_uiMaxTotalCUDepth, true );
    picYuvTrueOrg[d].create( m_iSourceWidth, m_isField ? m_iSourceHeightOrg : m_iSourceHeight, ( d > 0 ) ? CHROMA_400 : m_chromaFormatIDC, m_uiMaxCUWidth, m_uiMaxCUHeight, m_uiMaxTotalCUDepth, true );
  }
#else
  TComPicYuv cPicYuvTrueOrg;

  // allocate original YUV buffer
  if( m_isField )
  {
    pcPicYuvOrg->create  ( m_iSourceWidth, m_iSourceHeightOrg, m_chromaFormatIDC, m_uiMaxCUWidth, m_uiMaxCUHeight, m_uiMaxTotalCUDepth, true );
    cPicYuvTrueOrg.create(m_iSourceWidth, m_iSourceHeightOrg, m_chromaFormatIDC, m_uiMaxCUWidth, m_uiMaxCUHeight, m_uiMaxTotalCUDepth, true);
  }
  else
  {
    pcPicYuvOrg->create  ( m_iSourceWidth, m_iSourceHeight, m_chromaFormatIDC, m_uiMaxCUWidth, m_uiMaxCUHeight, m_uiMaxTotalCUDepth, true );
    cPicYuvTrueOrg.create(m_iSourceWidth, m_iSourceHeight, m_chromaFormatIDC, m_uiMaxCUWidth, m_uiMaxCUHeight, m_uiMaxTotalCUDepth, true );
  }
#endif
#if NH_MV
  while ( (m_targetEncLayerIdList.size() != 0 ) && !allEos )
  {
    for(Int layer=0; layer < m_numberOfLayers; layer++ )
    {
#if NH_3D
      TComPicYuv* pcPicYuvOrg    =  picYuvOrg    [ m_depthFlag[layer] ];
      TComPicYuv& cPicYuvTrueOrg =  picYuvTrueOrg[ m_depthFlag[layer] ];
#endif
      if (!xLayerIdInTargetEncLayerIdList( m_vps->getLayerIdInNuh( layer ) ))
      {
        continue; 
      }

      Int frmCnt = 0;
      while ( !eos[layer] && !(frmCnt == gopSize))
      {
        // get buffers
        xGetBuffer(pcPicYuvRec, layer);

        // read input YUV file        
        m_acTVideoIOYuvInputFileList[layer]->read      ( pcPicYuvOrg, &cPicYuvTrueOrg, ipCSC, m_aiPad, m_InputChromaFormatIDC );
        m_acTEncTopList             [layer]->initNewPic( pcPicYuvOrg );

        // increase number of received frames
        m_frameRcvd[layer]++;
        
        frmCnt++;

        eos[layer] = (m_frameRcvd[layer] == m_framesToBeEncoded);
        allEos = allEos||eos[layer];

        // if end of file (which is only detected on a read failure) flush the encoder of any queued pictures
        if (m_acTVideoIOYuvInputFileList[layer]->isEof())
        {
          flush          [layer] = true;
          eos            [layer] = true;
          m_frameRcvd    [layer]--;
          m_acTEncTopList[layer]->setFramesToBeEncoded(m_frameRcvd[layer]);
        }
      }
    }
    for ( Int gopId=0; gopId < gopSize; gopId++ )
    {
#if NH_3D
      UInt iNextPoc = m_acTEncTopList[0] -> getFrameId( gopId );
      if ( iNextPoc < m_framesToBeEncoded )
      {
        m_cCameraData.update( iNextPoc );
      }
#endif
      for(Int layer=0; layer < m_numberOfLayers; layer++ )
      {
#if NH_3D
        TComPicYuv* pcPicYuvOrg    =  picYuvOrg    [ m_depthFlag[layer] ];
        TComPicYuv& cPicYuvTrueOrg =  picYuvTrueOrg[ m_depthFlag[layer] ];
#endif
        if (!xLayerIdInTargetEncLayerIdList( m_vps->getLayerIdInNuh( layer ) ))
        {
          continue; 
        }

#if NH_3D_VSO        
          if( m_bUseVSO && m_bUseEstimatedVSD && iNextPoc < m_framesToBeEncoded )
          {
            m_cCameraData.setDispCoeff( iNextPoc, m_acTEncTopList[layer]->getViewIndex() );
            m_acTEncTopList[layer]  ->setDispCoeff( m_cCameraData.getDispCoeff() );
          }
#endif

        Int   iNumEncoded = 0;

        // call encoding function for one frame                               
        m_acTEncTopList[layer]->encode( eos[layer], flush[layer] ? 0 : pcPicYuvOrg, flush[layer] ? 0 : &cPicYuvTrueOrg, snrCSC, *m_cListPicYuvRec[layer], outputAccessUnits, iNumEncoded, gopId );        
        xWriteOutput(bitstreamFile, iNumEncoded, outputAccessUnits, layer);
        outputAccessUnits.clear();
      }
    }

    gopSize = maxGopSize;
  }
  for(Int layer=0; layer < m_numberOfLayers; layer++ )
  {
    if (!xLayerIdInTargetEncLayerIdList( m_vps->getLayerIdInNuh( layer ) ))
    {
      continue; 
    }    
    m_acTEncTopList[layer]->printSummary(m_isField);
  }
#else

  while ( !bEos )
  {
    // get buffers
    xGetBuffer(pcPicYuvRec);

    // read input YUV file
    m_cTVideoIOYuvInputFile.read( pcPicYuvOrg, &cPicYuvTrueOrg, ipCSC, m_aiPad, m_InputChromaFormatIDC, m_bClipInputVideoToRec709Range );

    // increase number of received frames
    m_iFrameRcvd++;

    bEos = (m_isField && (m_iFrameRcvd == (m_framesToBeEncoded >> 1) )) || ( !m_isField && (m_iFrameRcvd == m_framesToBeEncoded) );

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
    if ( m_isField )
    {
      m_cTEncTop.encode( bEos, flush ? 0 : pcPicYuvOrg, flush ? 0 : &cPicYuvTrueOrg, snrCSC, m_cListPicYuvRec, outputAccessUnits, iNumEncoded, m_isTopFieldFirst );
    }
    else
    {
      m_cTEncTop.encode( bEos, flush ? 0 : pcPicYuvOrg, flush ? 0 : &cPicYuvTrueOrg, snrCSC, m_cListPicYuvRec, outputAccessUnits, iNumEncoded );
    }

    // write bistream to file if necessary
    if ( iNumEncoded > 0 )
    {
      xWriteOutput(bitstreamFile, iNumEncoded, outputAccessUnits);
      outputAccessUnits.clear();
    }
  }

  m_cTEncTop.printSummary(m_isField);
#endif

#if NH_3D
  // delete original YUV buffer
  for (Int d = 0; d < 2; d++)
  {
    picYuvOrg[d]->destroy();
    delete picYuvOrg[d];
    picYuvOrg[d] = NULL;

    picYuvTrueOrg[d].destroy();
  }
#else
  // delete original YUV buffer
  pcPicYuvOrg->destroy();
  delete pcPicYuvOrg;
  pcPicYuvOrg = NULL;
#endif

#if !NH_MV
  // delete used buffers in encoder class
  m_cTEncTop.deletePicBuffer();
#endif
#if !NH_3D
  cPicYuvTrueOrg.destroy();
#endif

  // delete buffers & classes
  xDeleteBuffer();
  xDestroyLib();

  printRateSummary();

#if H_3D_REN_MAX_DEV_OUT
  Double dMaxDispDiff = m_cCameraData.getMaxShiftDeviation(); 

  if ( !(dMaxDispDiff < 0) )
  {  
    printf("\n Max. possible shift error: %12.3f samples.\n", dMaxDispDiff );
  }
#endif

  return;
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
#if NH_MV
Void TAppEncTop::xGetBuffer( TComPicYuv*& rpcPicYuvRec, UInt layer)
#else
Void TAppEncTop::xGetBuffer( TComPicYuv*& rpcPicYuvRec)
#endif
{
  assert( m_iGOPSize > 0 );

  // org. buffer
#if NH_MV
  if ( m_cListPicYuvRec[layer]->size() == (UInt)m_iGOPSize )
  {
    rpcPicYuvRec = m_cListPicYuvRec[layer]->popFront();
#else
  if ( m_cListPicYuvRec.size() >= (UInt)m_iGOPSize ) // buffer will be 1 element longer when using field coding, to maintain first field whilst processing second.
  {
    rpcPicYuvRec = m_cListPicYuvRec.popFront();
#endif
  }
  else
  {
    rpcPicYuvRec = new TComPicYuv;
#if NH_3D
    rpcPicYuvRec->create( m_iSourceWidth, m_iSourceHeight, m_depthFlag[layer] > 0 ? CHROMA_400 : m_chromaFormatIDC, m_uiMaxCUWidth, m_uiMaxCUHeight, m_uiMaxTotalCUDepth, true );
#else
    rpcPicYuvRec->create( m_iSourceWidth, m_iSourceHeight, m_chromaFormatIDC, m_uiMaxCUWidth, m_uiMaxCUHeight, m_uiMaxTotalCUDepth, true );
#endif

  }
#if NH_MV
  m_cListPicYuvRec[layer]->pushBack( rpcPicYuvRec );
#else
  m_cListPicYuvRec.pushBack( rpcPicYuvRec );
#endif
}

Void TAppEncTop::xDeleteBuffer( )
{
#if NH_MV
  for(Int layer=0; layer<m_cListPicYuvRec.size(); layer++)
  {
    if(m_cListPicYuvRec[layer])
    {
      TComList<TComPicYuv*>::iterator iterPicYuvRec  = m_cListPicYuvRec[layer]->begin();
      Int iSize = Int( m_cListPicYuvRec[layer]->size() );
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
#if NH_MV
    }
  }
#endif  

}

/** 
  Write access units to output file.
  \param bitstreamFile  target bitstream file
  \param iNumEncoded    number of encoded frames
  \param accessUnits    list of access units to be written
 */
#if NH_MV
Void TAppEncTop::xWriteOutput(std::ostream& bitstreamFile, Int iNumEncoded, std::list<AccessUnit>& accessUnits, UInt layerIdx)
#else
Void TAppEncTop::xWriteOutput(std::ostream& bitstreamFile, Int iNumEncoded, const std::list<AccessUnit>& accessUnits)
#endif
{
  const InputColourSpaceConversion ipCSC = (!m_outputInternalColourSpace) ? m_inputColourSpaceConvert : IPCOLOURSPACE_UNCHANGED;

  if (m_isField)
  {
    //Reinterlace fields
    Int i;
#if NH_MV
    if( iNumEncoded > 0 )
    {
      TComList<TComPicYuv*>::iterator iterPicYuvRec = m_cListPicYuvRec[layerIdx]->end();
#else
    TComList<TComPicYuv*>::iterator iterPicYuvRec = m_cListPicYuvRec.end();
    list<AccessUnit>::const_iterator iterBitstream = accessUnits.begin();
#endif

    for ( i = 0; i < iNumEncoded; i++ )
    {
      --iterPicYuvRec;
    }

    for ( i = 0; i < iNumEncoded/2; i++ )
    {
      TComPicYuv*  pcPicYuvRecTop  = *(iterPicYuvRec++);
      TComPicYuv*  pcPicYuvRecBottom  = *(iterPicYuvRec++);

#if NH_MV
      if (m_pchReconFileList[layerIdx])
      {
#if NH_3D
        m_acTVideoIOYuvReconFileList[layerIdx]->write( pcPicYuvRecTop, pcPicYuvRecBottom, ipCSC, m_confWinLeft, m_confWinRight, m_confWinTop, m_confWinBottom, m_depth420OutputFlag && m_depthFlag[layerIdx ] ? CHROMA_420 : NUM_CHROMA_FORMAT, m_isTopFieldFirst );
#else
        m_acTVideoIOYuvReconFileList[layerIdx]->write( pcPicYuvRecTop, pcPicYuvRecBottom, ipCSC, m_confWinLeft, m_confWinRight, m_confWinTop, m_confWinBottom, NUM_CHROMA_FORMAT, m_isTopFieldFirst );
#endif
      }
    }
  }

  if( ! accessUnits.empty() )
  {
    list<AccessUnit>::iterator aUIter;
    for( aUIter = accessUnits.begin(); aUIter != accessUnits.end(); aUIter++ )
    {
      const vector<UInt>& stats = writeAnnexB(bitstreamFile, *aUIter);
      rateStatsAccum(*aUIter, stats);
    }
  }
#else
      if (m_pchReconFile)
      {
        m_cTVideoIOYuvReconFile.write( pcPicYuvRecTop, pcPicYuvRecBottom, ipCSC, m_confWinLeft, m_confWinRight, m_confWinTop, m_confWinBottom, NUM_CHROMA_FORMAT, m_isTopFieldFirst );
      }

      const AccessUnit& auTop = *(iterBitstream++);
      const vector<UInt>& statsTop = writeAnnexB(bitstreamFile, auTop);
      rateStatsAccum(auTop, statsTop);

      const AccessUnit& auBottom = *(iterBitstream++);
      const vector<UInt>& statsBottom = writeAnnexB(bitstreamFile, auBottom);
      rateStatsAccum(auBottom, statsBottom);
    }
#endif
  }
  else
  {
    Int i;
#if NH_MV
    if( iNumEncoded > 0 )
    {
      TComList<TComPicYuv*>::iterator iterPicYuvRec = m_cListPicYuvRec[layerIdx]->end();
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
#if NH_MV
      if (m_pchReconFileList[layerIdx])
      {
#if NH_3D
        m_acTVideoIOYuvReconFileList[layerIdx]->write( pcPicYuvRec, ipCSC, m_confWinLeft, m_confWinRight, m_confWinTop, m_confWinBottom, m_depth420OutputFlag && m_depthFlag[layerIdx ] ? CHROMA_420 : NUM_CHROMA_FORMAT  );
#else
        m_acTVideoIOYuvReconFileList[layerIdx]->write( pcPicYuvRec, ipCSC, m_confWinLeft, m_confWinRight, m_confWinTop, m_confWinBottom );
#endif

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
        m_cTVideoIOYuvReconFile.write( pcPicYuvRec, ipCSC, m_confWinLeft, m_confWinRight, m_confWinTop, m_confWinBottom,
            NUM_CHROMA_FORMAT, m_bClipOutputVideoToRec709Range  );
      }

      const AccessUnit& au = *(iterBitstream++);
      const vector<UInt>& stats = writeAnnexB(bitstreamFile, au);
      rateStatsAccum(au, stats);
    }
#endif
  }
}

/**
 *
 */
Void TAppEncTop::rateStatsAccum(const AccessUnit& au, const std::vector<UInt>& annexBsizes)
{
  AccessUnit::const_iterator it_au = au.begin();
  vector<UInt>::const_iterator it_stats = annexBsizes.begin();

  for (; it_au != au.end(); it_au++, it_stats++)
  {
    switch ((*it_au)->m_nalUnitType)
    {
    case NAL_UNIT_CODED_SLICE_TRAIL_R:
    case NAL_UNIT_CODED_SLICE_TRAIL_N:
    case NAL_UNIT_CODED_SLICE_TSA_R:
    case NAL_UNIT_CODED_SLICE_TSA_N:
    case NAL_UNIT_CODED_SLICE_STSA_R:
    case NAL_UNIT_CODED_SLICE_STSA_N:
    case NAL_UNIT_CODED_SLICE_BLA_W_LP:
    case NAL_UNIT_CODED_SLICE_BLA_W_RADL:
    case NAL_UNIT_CODED_SLICE_BLA_N_LP:
    case NAL_UNIT_CODED_SLICE_IDR_W_RADL:
    case NAL_UNIT_CODED_SLICE_IDR_N_LP:
    case NAL_UNIT_CODED_SLICE_CRA:
    case NAL_UNIT_CODED_SLICE_RADL_N:
    case NAL_UNIT_CODED_SLICE_RADL_R:
    case NAL_UNIT_CODED_SLICE_RASL_N:
    case NAL_UNIT_CODED_SLICE_RASL_R:
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

Void TAppEncTop::printRateSummary()
{
#if NH_MV
  Double time = (Double) m_frameRcvd[0] / m_iFrameRate;
  printf("\n");
#else
  Double time = (Double) m_iFrameRcvd / m_iFrameRate;
#endif
  printf("Bytes written to file: %u (%.3f kbps)\n", m_totalBytes, 0.008 * m_totalBytes / time);
  if (m_summaryVerboseness > 0)
  {
  printf("Bytes for SPS/PPS/Slice (Incl. Annex B): %u (%.3f kbps)\n", m_essentialBytes, 0.008 * m_essentialBytes / time);
  }
}

Void TAppEncTop::printChromaFormat()
{
  std::cout << std::setw(43) << "Input ChromaFormatIDC = ";
  switch (m_InputChromaFormatIDC)
  {
  case CHROMA_400:  std::cout << "  4:0:0"; break;
  case CHROMA_420:  std::cout << "  4:2:0"; break;
  case CHROMA_422:  std::cout << "  4:2:2"; break;
  case CHROMA_444:  std::cout << "  4:4:4"; break;
  default:
    std::cerr << "Invalid";
    exit(1);
  }
  std::cout << std::endl;

#if NH_MV
  for (Int i = 0; i < m_numberOfLayers; i++)
  {
    std::cout << "Layer " << i << std::setw( 43 - (i > 9 ? 6 : 7) ) << "Internal ChromaFormatIDC = ";
    switch (m_acTEncTopList[i]->getChromaFormatIdc())
#else
    std::cout << std::setw(43) << "Output (internal) ChromaFormatIDC = ";
    switch (m_cTEncTop.getChromaFormatIdc())
#endif
    {
    case CHROMA_400:  std::cout << "  4:0:0"; break;
    case CHROMA_420:  std::cout << "  4:2:0"; break;
    case CHROMA_422:  std::cout << "  4:2:2"; break;
    case CHROMA_444:  std::cout << "  4:4:4"; break;
    default:
      std::cerr << "Invalid";
      exit(1);
    }
#if NH_MV
    std::cout << std::endl;
  }
#endif
  std::cout << "\n" << std::endl;
}

#if NH_3D_DLT
Void TAppEncTop::xAnalyzeInputBaseDepth(UInt layer, UInt uiNumFrames, TComVPS* vps, TComDLT* dlt)
{
  TComPicYuv*       pcDepthPicYuvOrg = new TComPicYuv;
  TComPicYuv*       pcDepthPicYuvTrueOrg = new TComPicYuv;
  // allocate original YUV buffer
  pcDepthPicYuvOrg->create( m_iSourceWidth, m_iSourceHeight, CHROMA_420, m_uiMaxCUWidth, m_uiMaxCUHeight, m_uiMaxTotalCUDepth, false );
  pcDepthPicYuvTrueOrg->create( m_iSourceWidth, m_iSourceHeight, CHROMA_420, m_uiMaxCUWidth, m_uiMaxCUHeight, m_uiMaxTotalCUDepth, false );
  
  TVideoIOYuv* depthVideoFile = new TVideoIOYuv;
  
  UInt uiMaxDepthValue = ((1 << m_inputBitDepth[CHANNEL_TYPE_LUMA])-1);
  
  std::vector<Bool> abValidDepths(256, false);
  
  depthVideoFile->open( m_pchInputFileList[layer], false, m_inputBitDepth, m_MSBExtendedBitDepth, m_internalBitDepth );
  
  Int iHeight   = pcDepthPicYuvOrg->getHeight(COMPONENT_Y);
  Int iWidth    = pcDepthPicYuvOrg->getWidth(COMPONENT_Y);
  Int iStride   = pcDepthPicYuvOrg->getStride(COMPONENT_Y);
  
  Pel* pInDM    = pcDepthPicYuvOrg->getAddr(COMPONENT_Y);
  
  for(Int uiFrame=0; uiFrame < uiNumFrames; uiFrame++ )
  {
    depthVideoFile->read( pcDepthPicYuvOrg, pcDepthPicYuvTrueOrg, IPCOLOURSPACE_UNCHANGED, m_aiPad, m_InputChromaFormatIDC, m_bClipInputVideoToRec709Range );
    
    // check all pixel values
    for (Int i=0; i<iHeight; i++)
    {
      Int rowOffset = i*iStride;
      
      for (Int j=0; j<iWidth; j++)
      {
        Pel depthValue = pInDM[rowOffset+j];
        abValidDepths[depthValue] = true;
      }
    }
  }
  
  depthVideoFile->close();
  delete depthVideoFile; 
  
  pcDepthPicYuvOrg->destroy();
  delete pcDepthPicYuvOrg;
  pcDepthPicYuvTrueOrg->destroy();
  delete pcDepthPicYuvTrueOrg;
  
  // convert boolean array to idx2Depth LUT
  std::vector<Int> aiIdx2DepthValue(256, 0);
  Int iNumDepthValues = 0;
  for(Int p=0; p<=uiMaxDepthValue; p++)
  {
    if( abValidDepths[p] == true)
    {
      aiIdx2DepthValue[iNumDepthValues++] = p;
    }
  }
  
  if( uiNumFrames == 0 || gCeilLog2(iNumDepthValues) == m_inputBitDepth[CHANNEL_TYPE_LUMA] )
  {
    dlt->setUseDLTFlag(layer, false);
  }
  
  // assign LUT
  if( dlt->getUseDLTFlag(layer) )
  {
    dlt->setDepthLUTs(layer, aiIdx2DepthValue, iNumDepthValues);
  }
}
#endif

#if NH_MV
Void TAppEncTop::xSetDimensionIdAndLength( TComVPS& vps )
{   
  vps.setScalabilityMaskFlag( m_scalabilityMask ); 
  for( Int dim = 0; dim < m_dimIds.size(); dim++ )
  {
    vps.setDimensionIdLen( dim, m_dimensionIdLen[ dim ] );
    for( Int layer = 0; layer <= vps.getMaxLayersMinus1(); layer++ )
    {        
      vps.setDimensionId( layer, dim, m_dimIds[ dim ][ layer ] );        
    }  
  }

  vps.initNumViews(); 
  Int maxViewId = xGetMax( m_viewId ); 

  Int viewIdLen = gCeilLog2( maxViewId + 1 ); 
  const Int maxViewIdLen = ( 1 << 4 ) - 1; 
  assert( viewIdLen <= maxViewIdLen ); 
  vps.setViewIdLen( viewIdLen ); 
  for (Int i = 0; i < m_iNumberOfViews; i++)
  {
    vps.setViewIdVal( i, m_viewId[ i] ); 
  }

  assert( m_iNumberOfViews == vps.getNumViews() ); 


#if NH_3D
  vps.initViewCompLayer( ); 
#endif
}

Void TAppEncTop::xSetDependencies( TComVPS& vps )
{
  // Direct dependency flags + dependency types
  for( Int depLayer = 1; depLayer < MAX_NUM_LAYERS; depLayer++ )
  {
    for( Int refLayer = 0; refLayer < MAX_NUM_LAYERS; refLayer++ )
    {
      vps.setDirectDependencyFlag( depLayer, refLayer, false); 
      vps.setDirectDependencyType( depLayer, refLayer,    -1 ); 
    }
  }

  Int  defaultDirectDependencyType = -1; 
  Bool defaultDirectDependencyFlag = false; 

  Int directDepTypeLenMinus2 = 0;  
  for( Int depLayer = 1; depLayer < m_numberOfLayers; depLayer++ )
  {
    Int numRefLayers = (Int) m_directRefLayers[depLayer].size(); 
    assert(  numRefLayers == (Int) m_dependencyTypes[depLayer].size() ); 
    for( Int i = 0; i < numRefLayers; i++ )
    {
      Int refLayer = m_directRefLayers[depLayer][i]; 
      vps.setDirectDependencyFlag( depLayer, refLayer, true); 
      Int curDirectDependencyType = m_dependencyTypes[depLayer][i]; 
      directDepTypeLenMinus2 = std::max( directDepTypeLenMinus2, gCeilLog2( curDirectDependencyType + 1  ) - 2 );  
      if ( defaultDirectDependencyType != -1 )    
      {
        defaultDirectDependencyFlag = defaultDirectDependencyFlag && (curDirectDependencyType == defaultDirectDependencyType );         
      }
      else
      {
        defaultDirectDependencyType = curDirectDependencyType; 
        defaultDirectDependencyFlag = true; 
      }
      
      vps.setDirectDependencyType( depLayer, refLayer, curDirectDependencyType);       
    }
  }

  vps.setDefaultDirectDependencyFlag( defaultDirectDependencyFlag );       
  vps.setDefaultDirectDependencyType( defaultDirectDependencyFlag ? defaultDirectDependencyType : -1 );       

  assert( directDepTypeLenMinus2 <= 1 ); 
  vps.setDirectDepTypeLenMinus2( directDepTypeLenMinus2 ); 


  vps.setRefLayers(); 

  // Max sub layers, + presence flag
  Bool subLayersMaxMinus1PresentFlag = false; 
  for (Int curLayerIdInVps = 0; curLayerIdInVps < m_numberOfLayers; curLayerIdInVps++ )
  {    
    Int curSubLayersMaxMinus1 = 0; 
    for( Int i = 0; i < getGOPSize(); i++ ) 
    {
      GOPEntry geCur =  m_GOPListMvc[curLayerIdInVps][i];
      curSubLayersMaxMinus1 = std::max( curSubLayersMaxMinus1, geCur.m_temporalId ); 
    }  

    vps.setSubLayersVpsMaxMinus1( curLayerIdInVps, curSubLayersMaxMinus1 ); 
    subLayersMaxMinus1PresentFlag = subLayersMaxMinus1PresentFlag || ( curSubLayersMaxMinus1 != vps.getMaxSubLayersMinus1() );
  }

  vps.setVpsSubLayersMaxMinus1PresentFlag( subLayersMaxMinus1PresentFlag ); 

  // Max temporal id for inter layer reference pictures
  for ( Int refLayerIdInVps = 0; refLayerIdInVps < m_numberOfLayers; refLayerIdInVps++)
  {
    Int refLayerIdInNuh = vps.getLayerIdInNuh( refLayerIdInVps );
    for ( Int curLayerIdInVps = 1; curLayerIdInVps < m_numberOfLayers; curLayerIdInVps++)
    {
      Int curLayerIdInNuh = vps.getLayerIdInNuh( curLayerIdInVps );      
      Int maxTid = -1; 
#if NH_3D
      if ( vps.getDirectDependencyFlag( curLayerIdInVps, refLayerIdInVps ) )
      {
        if ( m_depthFlag[ curLayerIdInVps] == m_depthFlag[ refLayerIdInVps ] )
        {
#endif
          for( Int i = 0; i < ( getGOPSize() + 1); i++ ) 
          {        
            GOPEntry geCur =  m_GOPListMvc[curLayerIdInVps][( i < getGOPSize()  ? i : MAX_GOP )];
            GOPEntry geRef =  m_GOPListMvc[refLayerIdInVps][( i < getGOPSize()  ? i : MAX_GOP )];
            for (Int j = 0; j < geCur.m_numActiveRefLayerPics; j++)
            {
#if NH_3D
              if ( vps.getIdRefListLayer( curLayerIdInNuh, geCur.m_interLayerPredLayerIdc[ j ] ) == refLayerIdInNuh )
#else
              if ( vps.getIdDirectRefLayer( curLayerIdInNuh, geCur.m_interLayerPredLayerIdc[ j ] ) == refLayerIdInNuh )
#endif
              {
                Bool refLayerZero   = ( i == getGOPSize() ) && ( refLayerIdInVps == 0 );
                maxTid = std::max( maxTid, refLayerZero ? 0 : geRef.m_temporalId ); 
              }
            }
          }              
#if NH_3D
        }
        else
        {        
          if( m_depthFlag[ curLayerIdInVps ] && ( m_mpiFlag|| m_qtPredFlag || m_intraContourFlag ) ) 
          {          
            Int nuhLayerIdTex = vps.getLayerIdInNuh( vps.getViewIndex( curLayerIdInNuh ), false ); 
            if ( nuhLayerIdTex == refLayerIdInNuh )
            {
              for( Int i = 0; i < ( getGOPSize() + 1); i++ ) 
              {        
                GOPEntry geCur =  m_GOPListMvc[curLayerIdInVps][( i < getGOPSize()  ? i : MAX_GOP )];
                GOPEntry geRef =  m_GOPListMvc[refLayerIdInVps][( i < getGOPSize()  ? i : MAX_GOP )];
                if ( geCur.m_interCompPredFlag )
                {
                  Bool refLayerZero   = ( i == getGOPSize() ) && ( refLayerIdInVps == 0 );
                  maxTid = std::max( maxTid, refLayerZero ? 0 : geRef.m_temporalId ); 
                }
              }
            }
          }
          if( !m_depthFlag[ curLayerIdInVps ] && vps.getNumRefListLayers( curLayerIdInNuh) > 0  && ( m_depthRefinementFlag || m_viewSynthesisPredFlag || m_depthBasedBlkPartFlag ) ) 
          {             
            for( Int i = 0; i < ( getGOPSize() + 1); i++ ) 
            {        
              GOPEntry geCur =  m_GOPListMvc[curLayerIdInVps][( i < getGOPSize()  ? i : MAX_GOP )];
              GOPEntry geRef =  m_GOPListMvc[refLayerIdInVps][( i < getGOPSize()  ? i : MAX_GOP )];

              if ( geCur.m_interCompPredFlag )
              {
                for (Int j = 0; j < geCur.m_numActiveRefLayerPics; j++ )
                {
                  Int nuhLayerIdDep = vps.getLayerIdInNuh( vps.getViewIndex( vps.getIdRefListLayer( curLayerIdInNuh, geCur.m_interLayerPredLayerIdc[j] ) ), true ); 
                  if ( nuhLayerIdDep == refLayerIdInNuh )
                  {
                    Bool refLayerZero   = ( i == getGOPSize() ) && ( refLayerIdInVps == 0 );
                    maxTid = std::max( maxTid, refLayerZero ? 0 : geRef.m_temporalId ); 
                  }
                }
              }
            }
          }        
        }
      } // if ( vps.getDirectDependencyFlag( curLayerIdInVps, refLayerIdInVps ) ) 
      vps.setMaxTidIlRefPicsPlus1( refLayerIdInVps, curLayerIdInVps, maxTid + 1 );
#endif
    }  // Loop curLayerIdInVps
  } // Loop refLayerIdInVps

  // Max temporal id for inter layer reference pictures presence flag
  Bool maxTidRefPresentFlag = false;   
  for ( Int refLayerIdInVps = 0; refLayerIdInVps < m_numberOfLayers; refLayerIdInVps++)
  {
    for ( Int curLayerIdInVps = 1; curLayerIdInVps < m_numberOfLayers; curLayerIdInVps++)
    {
        maxTidRefPresentFlag = maxTidRefPresentFlag || ( vps.getMaxTidIlRefPicsPlus1( refLayerIdInVps, curLayerIdInVps ) != 7 );    
    }
  }
  vps.setMaxTidRefPresentFlag( maxTidRefPresentFlag );


  // Max one active ref layer flag
  Bool maxOneActiveRefLayerFlag = true;  
  for ( Int layerIdInVps = 1; layerIdInVps < m_numberOfLayers && maxOneActiveRefLayerFlag; layerIdInVps++)
  {
    for( Int i = 0; i < ( getGOPSize() + 1) && maxOneActiveRefLayerFlag; i++ ) 
    {        
      GOPEntry ge =  m_GOPListMvc[layerIdInVps][ ( i < getGOPSize()  ? i : MAX_GOP ) ]; 
      maxOneActiveRefLayerFlag =  maxOneActiveRefLayerFlag && (ge.m_numActiveRefLayerPics <= 1); 
    }            
  }

  vps.setMaxOneActiveRefLayerFlag( maxOneActiveRefLayerFlag );
  
  // Poc Lsb Not Present Flag
  for ( Int layerIdInVps = 1; layerIdInVps < m_numberOfLayers; layerIdInVps++)
  {
    if ( m_directRefLayers[ layerIdInVps ].size() == 0 ) 
    {    
      vps.setPocLsbNotPresentFlag( layerIdInVps,  true );  
    }
  }  
  
  // All Ref layers active flag
  Bool allRefLayersActiveFlag = true; 
  for ( Int layerIdInVps = 1; layerIdInVps < m_numberOfLayers && allRefLayersActiveFlag; layerIdInVps++)
  {    
    Int layerIdInNuh = vps.getLayerIdInNuh( layerIdInVps ); 
    for( Int i = 0; i < ( getGOPSize() + 1) && allRefLayersActiveFlag; i++ ) 
    {        
      GOPEntry ge =  m_GOPListMvc[layerIdInVps][ ( i < getGOPSize()  ? i : MAX_GOP ) ]; 
      Int tId = ge.m_temporalId;  // Should be equal for all layers. 
      
      // check if all reference layers when allRefLayerActiveFlag is equal to 1 are reference layer pictures specified in the gop entry
#if NH_3D
      for (Int k = 0; k < vps.getNumRefListLayers( layerIdInNuh ) && allRefLayersActiveFlag; k++ )
      {
        Int refLayerIdInVps = vps.getLayerIdInVps( vps.getIdRefListLayer( layerIdInNuh , k ) ); 
#else
      for (Int k = 0; k < vps.getNumDirectRefLayers( layerIdInNuh ) && allRefLayersActiveFlag; k++ )
      {
        Int refLayerIdInVps = vps.getLayerIdInVps( vps.getIdDirectRefLayer( layerIdInNuh , k ) ); 
#endif
        if ( vps.getSubLayersVpsMaxMinus1(refLayerIdInVps) >= tId  && ( tId == 0 || vps.getMaxTidIlRefPicsPlus1(refLayerIdInVps,layerIdInVps) > tId )  )
        {
          Bool gopEntryFoundFlag = false; 
          for( Int l = 0; l < ge.m_numActiveRefLayerPics && !gopEntryFoundFlag; l++ )
          {
            gopEntryFoundFlag = gopEntryFoundFlag || ( ge.m_interLayerPredLayerIdc[l] == k ); 
          }          
          allRefLayersActiveFlag = allRefLayersActiveFlag && gopEntryFoundFlag;  
        }        
      }

      // check if all inter layer reference pictures specified in the gop entry are valid reference layer pictures when allRefLayerActiveFlag is equal to 1 
      // (Should actually always be true) 
      Bool maxTidIlRefAndSubLayerMaxVaildFlag = true; 
      for( Int l = 0; l < ge.m_numActiveRefLayerPics; l++ )
      {   
        Bool referenceLayerFoundFlag = false; 
#if NH_3D
        for (Int k = 0; k < vps.getNumRefListLayers( layerIdInNuh ); k++ )
        {
          Int refLayerIdInVps = vps.getLayerIdInVps( vps.getIdRefListLayer( layerIdInNuh, k) );
#else
        for (Int k = 0; k < vps.getNumDirectRefLayers( layerIdInNuh ); k++ )
        {
          Int refLayerIdInVps = vps.getLayerIdInVps( vps.getIdDirectRefLayer( layerIdInNuh, k) );
#endif
          if ( vps.getSubLayersVpsMaxMinus1(refLayerIdInVps) >= tId  && ( tId == 0 || vps.getMaxTidIlRefPicsPlus1(refLayerIdInVps,layerIdInVps) > tId )  )
          {          
            referenceLayerFoundFlag = referenceLayerFoundFlag || ( ge.m_interLayerPredLayerIdc[l] == k ); 
          }          
        }
       maxTidIlRefAndSubLayerMaxVaildFlag = maxTidIlRefAndSubLayerMaxVaildFlag && referenceLayerFoundFlag;  
      }
      assert ( maxTidIlRefAndSubLayerMaxVaildFlag ); // Something wrong with MaxTidIlRefPicsPlus1 or SubLayersVpsMaxMinus1
    }            
  }

  vps.setAllRefLayersActiveFlag( allRefLayersActiveFlag );
}; 


GOPEntry* TAppEncTop::xGetGopEntry( Int layerIdInVps, Int poc )
{
  GOPEntry* geFound = NULL; 
  for( Int i = 0; i < ( getGOPSize() + 1) && geFound == NULL ; i++ ) 
  {
    GOPEntry* ge = &(m_GOPListMvc[layerIdInVps][ ( i < getGOPSize()  ? i : MAX_GOP ) ]);
    if ( ge->m_POC == poc )
    {
      geFound = ge;       
    }
  }
  assert( geFound != NULL ); 
  return geFound; 
}


Void TAppEncTop::xSetTimingInfo( TComVPS& vps )
{
  vps.getTimingInfo()->setTimingInfoPresentFlag( false );
}

Void TAppEncTop::xSetHrdParameters( TComVPS& vps )
{
  vps.createHrdParamBuffer();
  for( Int i = 0; i < vps.getNumHrdParameters(); i++ )
  {
    vps.setHrdOpSetIdx( 0, i );
    vps.setCprmsPresentFlag( false, i );
  }
}

Void TAppEncTop::xSetLayerIds( TComVPS& vps )
{
  vps.setSplittingFlag     ( m_splittingFlag );

  Bool nuhLayerIdPresentFlag = false; 
  

  vps.setVpsMaxLayerId( xGetMax( m_layerIdInNuh ) ); 

  for (Int i = 0; i < m_numberOfLayers; i++)
  {
    nuhLayerIdPresentFlag = nuhLayerIdPresentFlag || ( m_layerIdInNuh[i] != i ); 
  }

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
  {
    maxVec = max( vec[i], maxVec ); 
  }
  return maxVec;
}

Void TAppEncTop::xSetProfileTierLevel( TComVPS& vps )
{ 

  // SET PTL
  assert( m_profiles.size() == m_level.size() && m_profiles.size() == m_levelTier.size() ); 
  vps.setVpsNumProfileTierLevelMinus1( (Int) m_profiles.size() - 1 );
  for ( Int ptlIdx = 0; ptlIdx <= vps.getVpsNumProfileTierLevelMinus1(); ptlIdx++ )
  {
    if ( ptlIdx > 1 )
    {
      Bool vpsProfilePresentFlag = ( m_profiles[ptlIdx] != m_profiles[ptlIdx - 1] )
        || ( m_inblFlag[ptlIdx ] != m_inblFlag[ptlIdx - 1] ); 
      vps.setVpsProfilePresentFlag( ptlIdx, vpsProfilePresentFlag ); 
    }

    xSetProfileTierLevel( vps, ptlIdx, -1, m_profiles[ptlIdx], m_level[ptlIdx], 
      m_levelTier[ ptlIdx ], m_progressiveSourceFlag, m_interlacedSourceFlag,
      m_nonPackedConstraintFlag, m_frameOnlyConstraintFlag,  m_inblFlag[ptlIdx] );     
  }  
}

Void TAppEncTop::xSetProfileTierLevel(TComVPS& vps, Int profileTierLevelIdx, Int subLayer, Profile::Name profile, Level::Name level, Level::Tier tier, Bool progressiveSourceFlag, Bool interlacedSourceFlag, Bool nonPackedConstraintFlag, Bool frameOnlyConstraintFlag, Bool inbldFlag)
{
  TComPTL* ptlStruct = vps.getPTL( profileTierLevelIdx );    
  assert( ptlStruct != NULL ); 

  ProfileTierLevel* ptl; 
  if ( subLayer == -1 )
  {
    ptl = ptlStruct->getGeneralPTL();
  }
  else
  {
    ptl = ptlStruct->getSubLayerPTL(  subLayer );
  }

  assert( ptl != NULL );

  ptl->setProfileIdc( profile );
  ptl->setTierFlag  ( tier    );
  ptl->setLevelIdc  ( level   );
  ptl->setProfileCompatibilityFlag( profile, true );
  ptl->setInbldFlag( inbldFlag );

  switch ( profile )
  {
  case Profile::MAIN:
    break; 
  case Profile::MULTIVIEWMAIN:
#if NH_3D
  case Profile::MAIN3D:
#endif
    ptl->setMax12bitConstraintFlag      ( true  ); 
    ptl->setMax12bitConstraintFlag      ( true  );
    ptl->setMax10bitConstraintFlag      ( true  );
    ptl->setMax8bitConstraintFlag       ( true  );
    ptl->setMax422chromaConstraintFlag  ( true  );
    ptl->setMax420chromaConstraintFlag  ( true  );
    ptl->setMaxMonochromeConstraintFlag ( false );
    ptl->setIntraConstraintFlag         ( false ); 
    ptl->setOnePictureOnlyConstraintFlag( false );
    ptl->setLowerBitRateConstraintFlag  ( true  );        
    break; 
  default:
    assert( 0 ); // other profiles currently not supported
    break; 
  }
}

Void TAppEncTop::xSetRepFormat( TComVPS& vps )
{

  Bool anyDepth = false; 
#if NH_3D
  for ( Int i = 0; i < m_numberOfLayers; i++ )
  {
    vps.setVpsRepFormatIdx( i, m_depthFlag[ i ] ? 1 : 0 );
    anyDepth = anyDepth || m_depthFlag[ i ];
  }
#endif

  vps.setRepFormatIdxPresentFlag( anyDepth ); 
  vps.setVpsNumRepFormatsMinus1 ( anyDepth ? 1 : 0     ); 


  std::vector<TComRepFormat> repFormat;
  repFormat.resize( vps.getVpsNumRepFormatsMinus1() + 1 ); 
  for ( Int j = 0; j <= vps.getVpsNumRepFormatsMinus1(); j++ )
  {           
    repFormat[j].setBitDepthVpsChromaMinus8   ( m_internalBitDepth[CHANNEL_TYPE_LUMA  ] - 8 ); 
    repFormat[j].setBitDepthVpsLumaMinus8     ( m_internalBitDepth[CHANNEL_TYPE_CHROMA] - 8 );
    repFormat[j].setChromaFormatVpsIdc        ( j == 1 ? CHROMA_400 :  CHROMA_420 );
    repFormat[j].setPicHeightVpsInLumaSamples ( m_iSourceHeight );
    repFormat[j].setPicWidthVpsInLumaSamples  ( m_iSourceWidth  );    
    repFormat[j].setChromaAndBitDepthVpsPresentFlag( true );    
    // ToDo not supported yet. 
    //repFormat->setSeparateColourPlaneVpsFlag( );

    repFormat[j].setConformanceWindowVpsFlag( true );
    repFormat[j].setConfWinVpsLeftOffset    ( m_confWinLeft   / TComSPS::getWinUnitX( repFormat[j].getChromaFormatVpsIdc() ) );
    repFormat[j].setConfWinVpsRightOffset   ( m_confWinRight  / TComSPS::getWinUnitX( repFormat[j].getChromaFormatVpsIdc() )  );
    repFormat[j].setConfWinVpsTopOffset     ( m_confWinTop    / TComSPS::getWinUnitY( repFormat[j].getChromaFormatVpsIdc() )  );
    repFormat[j].setConfWinVpsBottomOffset  ( m_confWinBottom / TComSPS::getWinUnitY( repFormat[j].getChromaFormatVpsIdc() ) ); 
  }

  vps.setRepFormat( repFormat );

}

Void TAppEncTop::xSetDpbSize                ( TComVPS& vps )
{
  // These settings need to be verified

  TComDpbSize dpbSize;   
  dpbSize.init( vps.getNumOutputLayerSets(), vps.getVpsMaxLayerId() + 1, vps.getMaxSubLayersMinus1() + 1 ) ;
  

  for( Int i = 0; i < vps.getNumOutputLayerSets(); i++ )
  {  
    Int currLsIdx = vps.olsIdxToLsIdx( i ); 
    Bool subLayerFlagInfoPresentFlag = false; 

    for( Int j = 0; j  <=  vps.getMaxSubLayersInLayerSetMinus1( currLsIdx ); j++ )
    {   
      Bool subLayerDpbInfoPresentFlag = false; 
      for( Int k = 0; k < vps.getNumLayersInIdList( currLsIdx ); k++ )   
      {
        Int layerIdInVps = vps.getLayerIdInVps( vps.getLayerSetLayerIdList( currLsIdx, k ) );
        if ( vps.getNecessaryLayerFlag( i,k ) && ( vps.getVpsBaseLayerInternalFlag() || vps.getLayerSetLayerIdList( currLsIdx, k ) != 0 ) )
        {       
          dpbSize.setMaxVpsDecPicBufferingMinus1( i, k, j, m_maxDecPicBufferingMvc[ layerIdInVps ][ j ] - 1 );
          if ( j > 0 )
          {
            subLayerDpbInfoPresentFlag = subLayerDpbInfoPresentFlag || ( dpbSize.getMaxVpsDecPicBufferingMinus1( i, k, j ) != dpbSize.getMaxVpsDecPicBufferingMinus1( i, k, j - 1 ) );
          }
        }
        else
        {
          if (vps.getNecessaryLayerFlag(i,k) && j == 0 && k == 0 )
          {          
            dpbSize.setMaxVpsDecPicBufferingMinus1(i, k ,j, 0 ); 
          }
        }
      }        

      Int maxNumReorderPics = MIN_INT;
      for ( Int idx = 0; idx < vps.getNumLayersInIdList( currLsIdx ); idx++ )
      {
        if (vps.getNecessaryLayerFlag(i, idx ))
        {        
          Int layerIdInVps = vps.getLayerIdInVps( vps.getLayerSetLayerIdList(currLsIdx, idx) );        
          maxNumReorderPics = std::max( maxNumReorderPics, m_numReorderPicsMvc[ layerIdInVps ][ j ] ); 
        }
      }
      assert( maxNumReorderPics != MIN_INT ); 

      dpbSize.setMaxVpsNumReorderPics( i, j, maxNumReorderPics );
      if ( j > 0 )
      {
        subLayerDpbInfoPresentFlag = subLayerDpbInfoPresentFlag || ( dpbSize.getMaxVpsNumReorderPics( i, j ) != dpbSize.getMaxVpsNumReorderPics( i, j - 1 ) );
      }

      // To Be Done !
      // dpbSize.setMaxVpsLatencyIncreasePlus1( i, j, xx );
      if ( j > 0 )
      {
        subLayerDpbInfoPresentFlag = subLayerDpbInfoPresentFlag || ( dpbSize.getMaxVpsLatencyIncreasePlus1( i, j ) != dpbSize.getMaxVpsLatencyIncreasePlus1( i, j - 1  ) );
      }

      if( j > 0 )  
      {
        dpbSize.setSubLayerDpbInfoPresentFlag( i, j, subLayerDpbInfoPresentFlag );
        subLayerFlagInfoPresentFlag = subLayerFlagInfoPresentFlag || subLayerDpbInfoPresentFlag; 
      }       
    }  
    dpbSize.setSubLayerFlagInfoPresentFlag( i, subLayerFlagInfoPresentFlag ); 
  }  
  vps.setDpbSize( dpbSize ); 
}

Void TAppEncTop::xSetLayerSets( TComVPS& vps )
{   
  // Layer sets
  vps.setVpsNumLayerSetsMinus1   ( m_vpsNumLayerSets - 1 ); 
    
  for (Int lsIdx = 0; lsIdx < m_vpsNumLayerSets; lsIdx++ )
  {
    for( Int layerId = 0; layerId < MAX_NUM_LAYER_IDS; layerId++ )
    {
      vps.setLayerIdIncludedFlag( false, lsIdx, layerId ); 
    }
    for ( Int i = 0; i < m_layerIdsInSets[lsIdx].size(); i++)
    {       
      vps.setLayerIdIncludedFlag( true, lsIdx, vps.getLayerIdInNuh( m_layerIdsInSets[lsIdx][i] ) ); 
    } 
  }
  vps.deriveLayerSetLayerIdList(); 

  Int numAddOuputLayerSets = (Int) m_outputLayerSetIdx.size(); 
  // Additional output layer sets + profileLevelTierIdx
  vps.setDefaultOutputLayerIdc      ( m_defaultOutputLayerIdc );   
  if( vps.getNumIndependentLayers() == 0 && m_numAddLayerSets > 0  )
  {
    fprintf( stderr, "\nWarning: Ignoring additional layer sets since NumIndependentLayers is equal to 0.\n");            
  }
  else
  {
    vps.setNumAddLayerSets( m_numAddLayerSets ); 
    if ( m_highestLayerIdxPlus1.size() < vps.getNumAddLayerSets() ) 
    {
      fprintf(stderr, "\nError: Number of highestLayerIdxPlus1 parameters must be greater than or equal to NumAddLayerSets\n");
      exit(EXIT_FAILURE);
    }

    for (Int i = 0; i < vps.getNumAddLayerSets(); i++)
    {
      if ( m_highestLayerIdxPlus1[ i ].size() < vps.getNumIndependentLayers() ) 
      {
        fprintf(stderr, "Error: Number of elements in highestLayerIdxPlus1[ %d ] parameters must be greater than or equal to NumIndependentLayers(= %d)\n", i, vps.getNumIndependentLayers());
        exit(EXIT_FAILURE);
      }

      for (Int j = 1; j < vps.getNumIndependentLayers(); j++)
      {
        if ( m_highestLayerIdxPlus1[ i ][ j ]  < 0 || m_highestLayerIdxPlus1[ i ][ j ] > vps.getNumLayersInTreePartition( j ) ) 
        {
          fprintf(stderr, "Error: highestLayerIdxPlus1[ %d ][ %d ] shall be in the range of 0 to NumLayersInTreePartition[ %d ] (= %d ), inclusive. \n", i, j, j, vps.getNumLayersInTreePartition( j ) );
          exit(EXIT_FAILURE);
        }
        vps.setHighestLayerIdxPlus1( i, j, m_highestLayerIdxPlus1[ i ][ j ] ); 
      }
      vps.deriveAddLayerSetLayerIdList( i );
    }        
  }  
  vps.setNumAddOlss                 ( numAddOuputLayerSets          ); 
  vps.initTargetLayerIdLists(); 

  for (Int olsIdx = 0; olsIdx < vps.getNumLayerSets() + numAddOuputLayerSets; olsIdx++)
  {
    Int addOutLsIdx = olsIdx - vps.getNumLayerSets();     
    vps.setLayerSetIdxForOlsMinus1( olsIdx, ( ( addOutLsIdx < 0 ) ?  olsIdx  : m_outputLayerSetIdx[ addOutLsIdx ] ) - 1 ); 

    Int lsIdx = vps.olsIdxToLsIdx( olsIdx );
    if (vps.getDefaultOutputLayerIdc() == 2 || addOutLsIdx >= 0 )
    { 
      for ( Int i = 0; i < vps.getNumLayersInIdList( lsIdx ); i++)
      {
        vps.setOutputLayerFlag( olsIdx, i, ( olsIdx == 0 && i == 0 ) ? vps.inferOutputLayerFlag(olsIdx, i ) : false ); // This is a software only fix for a bug in the spec. In spec outputLayerFlag neither present nor inferred for this case !
      }

      std::vector<Int>& outLayerIdList = ( addOutLsIdx >= 0 ) ? m_layerIdsInAddOutputLayerSet[addOutLsIdx] : m_layerIdsInDefOutputLayerSet[olsIdx]; 

      Bool outputLayerInLayerSetFlag = false; 
      for (Int j = 0; j < outLayerIdList.size(); j++)
      {   
        for ( Int i = 0; i < vps.getNumLayersInIdList( lsIdx ); i++)
        {
          if ( vps.getLayerSetLayerIdList( lsIdx, i ) == outLayerIdList[ j ] )
          {
            vps.setOutputLayerFlag( olsIdx, i, true );       
            outputLayerInLayerSetFlag = true; 
            break; 
          }
        }
        if ( !outputLayerInLayerSetFlag )
        {
          fprintf(stderr, "Error: Output layer %d in output layer set %d not in corresponding layer set %d \n", outLayerIdList[ j ], olsIdx , lsIdx );
          exit(EXIT_FAILURE);
        }
      }
    }
    else
    {
      for ( Int i = 0; i < vps.getNumLayersInIdList( lsIdx ); i++)
      {
        vps.setOutputLayerFlag( olsIdx, i, vps.inferOutputLayerFlag( olsIdx, i ) );       
      }
    }

    vps.deriveNecessaryLayerFlags( olsIdx ); 
    vps.deriveTargetLayerIdList(  olsIdx ); 

    // SET profile_tier_level_index. 
    if ( olsIdx == 0 )
    {   
      vps.setProfileTierLevelIdx( 0, 0 , vps.getMaxLayersMinus1() > 0 ? 1 : 0 ); 
    }
    else
    {
      if( (Int) m_profileTierLevelIdx[ olsIdx ].size() < vps.getNumLayersInIdList( lsIdx ) )
      {
        fprintf( stderr, "Warning: Not enough profileTierLevelIdx values given for the %d-th OLS. Inferring default values.\n", olsIdx ); 
      }
      for (Int j = 0; j < vps.getNumLayersInIdList( lsIdx ); j++)
      {
        if( j < (Int) m_profileTierLevelIdx[ olsIdx ].size() )
        {
          vps.setProfileTierLevelIdx(olsIdx, j, m_profileTierLevelIdx[olsIdx][j] );
          if( !vps.getNecessaryLayerFlag(olsIdx,j) && m_profileTierLevelIdx[ olsIdx ][ j ] != -1 )
          {
            fprintf( stderr, "Warning: The %d-th layer in the %d-th OLS is not necessary such that profileTierLevelIdx[%d][%d] will be ignored. Set value to -1 to suppress warning.\n", j,olsIdx,olsIdx,j ); 
          }          
        }
        else if ( vps.getNecessaryLayerFlag(olsIdx,j) )
        {
          // setting default values
          if ( j == 0 || vps.getVpsNumProfileTierLevelMinus1() < 1 )
          {
            // set base layer as default
            vps.setProfileTierLevelIdx(olsIdx, j, 1 );
          }
          else
          {
            // set VpsProfileTierLevel[2] as default
            vps.setProfileTierLevelIdx(olsIdx, j, 2 ); 
          }
        }
      }
    }
   
    if ( vps.getNumOutputLayersInOutputLayerSet( olsIdx ) == 1 && 
        vps.getNumDirectRefLayers( vps.getOlsHighestOutputLayerId( olsIdx ) ) )
    {   
      vps.setAltOutputLayerFlag( olsIdx , m_altOutputLayerFlag[ olsIdx ]);
    }
    else
    {
      vps.setAltOutputLayerFlag( olsIdx , false );
      if ( m_altOutputLayerFlag[ olsIdx ] )
      {
        printf( "\nWarning: Ignoring AltOutputLayerFlag for output layer set %d, since more than one output layer or no dependent layers.\n", olsIdx );            
      }
    }
  }
}

Void TAppEncTop::xSetVPSVUI( TComVPS& vps )
{
  vps.setVpsVuiPresentFlag( m_vpsVuiPresentFlag ); 

  TComVPSVUI vpsVui;
  vpsVui.init(vps.getNumAddLayerSets(),vps.getMaxSubLayersMinus1() + 1, vps.getMaxLayersMinus1() + 1 );

  if ( m_vpsVuiPresentFlag )
  {
    // All this stuff could actually be derived by the encoder, 
    // however preliminary setting it from input parameters

    vpsVui.setCrossLayerPicTypeAlignedFlag( m_crossLayerPicTypeAlignedFlag );
    vpsVui.setCrossLayerIrapAlignedFlag   ( m_crossLayerIrapAlignedFlag    );
    vpsVui.setAllLayersIdrAlignedFlag     ( m_allLayersIdrAlignedFlag      );
    vpsVui.setBitRatePresentVpsFlag( m_bitRatePresentVpsFlag );
    vpsVui.setPicRatePresentVpsFlag( m_picRatePresentVpsFlag );

    if( vpsVui.getBitRatePresentVpsFlag( )  ||  vpsVui.getPicRatePresentVpsFlag( ) )
    {
      for( Int i = 0; i  <  vps.getNumLayerSets(); i++ )
      {
        for( Int j = 0; j  <=  vps.getMaxTLayers(); j++ ) 
        {
          if( vpsVui.getBitRatePresentVpsFlag( ) && m_bitRatePresentFlag[i].size() > j )
          {
            vpsVui.setBitRatePresentFlag( i, j, m_bitRatePresentFlag[i][j] );            
          }
          if( vpsVui.getPicRatePresentVpsFlag( ) && m_picRatePresentFlag[i].size() > j   )
          {
            vpsVui.setPicRatePresentFlag( i, j, m_picRatePresentFlag[i][j] );
          }
          if( vpsVui.getBitRatePresentFlag( i, j )  && m_avgBitRate[i].size() > j )
          {
            vpsVui.setAvgBitRate( i, j, m_avgBitRate[i][j] );          
          }
          if( vpsVui.getBitRatePresentFlag( i, j )  && m_maxBitRate[i].size() > j )
          {
            vpsVui.setMaxBitRate( i, j, m_maxBitRate[i][j] );
          }
          if( vpsVui.getPicRatePresentFlag( i, j ) && m_constantPicRateIdc[i].size() > j )
          {
            vpsVui.setConstantPicRateIdc( i, j, m_constantPicRateIdc[i][j] );
          }
          if( vpsVui.getPicRatePresentFlag( i, j ) && m_avgPicRate[i].size() > j )
          {
            vpsVui.setAvgPicRate( i, j, m_avgPicRate[i][j] );
          }
        }
      }
    }

    vpsVui.setTilesNotInUseFlag( m_tilesNotInUseFlag );

    if( !vpsVui.getTilesNotInUseFlag() ) 
    {      
      for( Int i = 0; i  <=  vps.getMaxLayersMinus1(); i++ )
      {
        vpsVui.setTilesInUseFlag( i, m_tilesInUseFlag[ i ] );
        if( vpsVui.getTilesInUseFlag( i ) )  
        {
          vpsVui.setLoopFilterNotAcrossTilesFlag( i, m_loopFilterNotAcrossTilesFlag[ i ] );
        }
      }  

      for( Int i = 1; i  <=  vps.getMaxLayersMinus1(); i++ )  
      {
        for( Int j = 0; j < vps.getNumDirectRefLayers( vps.getLayerIdInNuh( i ) ) ; j++ )
        {  
          Int layerIdx = vps.getLayerIdInVps( vps.getIdDirectRefLayer(vps.getLayerIdInNuh( i ) , j  ));  
          if( vpsVui.getTilesInUseFlag( i )  &&  vpsVui.getTilesInUseFlag( layerIdx ) )  
          {
            vpsVui.setTileBoundariesAlignedFlag( i, j, m_tileBoundariesAlignedFlag[i][j] );
          }
        }  
      }
    }  

    vpsVui.setWppNotInUseFlag( m_wppNotInUseFlag );

    if( !vpsVui.getWppNotInUseFlag( ) )
    {
      for( Int i = 1; i  <=  vps.getMaxLayersMinus1(); i++ )  
      {
        vpsVui.setWppInUseFlag( i, m_wppInUseFlag[ i ]);
      }
    }

  vpsVui.setSingleLayerForNonIrapFlag( m_singleLayerForNonIrapFlag );
  vpsVui.setHigherLayerIrapSkipFlag( m_higherLayerIrapSkipFlag );

    vpsVui.setIlpRestrictedRefLayersFlag( m_ilpRestrictedRefLayersFlag );

    if( vpsVui.getIlpRestrictedRefLayersFlag( ) )
    {
      for( Int i = 1; i  <=  vps.getMaxLayersMinus1(); i++ )
      {
        for( Int j = 0; j < vps.getNumDirectRefLayers( vps.getLayerIdInNuh( i ) ); j++ )
        {
          if ( m_minSpatialSegmentOffsetPlus1[i].size() > j )
          {        
            vpsVui.setMinSpatialSegmentOffsetPlus1( i, j, m_minSpatialSegmentOffsetPlus1[i][j] );
          }
          if( vpsVui.getMinSpatialSegmentOffsetPlus1( i, j ) > 0 )
          {
            if ( m_ctuBasedOffsetEnabledFlag[i].size() > j )
            {        
              vpsVui.setCtuBasedOffsetEnabledFlag( i, j, m_ctuBasedOffsetEnabledFlag[i][j] );
            }
            if( vpsVui.getCtuBasedOffsetEnabledFlag( i, j ) )
            {
              if ( m_minHorizontalCtuOffsetPlus1[i].size() > j )
              {
                vpsVui.setMinHorizontalCtuOffsetPlus1( i, j, m_minHorizontalCtuOffsetPlus1[i][j] );
              }
            }
          }
        }
      }
    }      
    vpsVui.setVideoSignalInfoIdxPresentFlag( true ); 
    vpsVui.setVpsNumVideoSignalInfoMinus1  ( 0    );     

    std::vector<TComVideoSignalInfo> videoSignalInfos;
    videoSignalInfos.resize( vpsVui.getVpsNumVideoSignalInfoMinus1() + 1 );

    videoSignalInfos[0].setColourPrimariesVps        ( m_colourPrimaries ); 
    videoSignalInfos[0].setMatrixCoeffsVps           ( m_matrixCoefficients ); 
    videoSignalInfos[0].setTransferCharacteristicsVps( m_transferCharacteristics ); 
    videoSignalInfos[0].setVideoVpsFormat            ( m_videoFormat ); 
    videoSignalInfos[0].setVideoFullRangeVpsFlag     ( m_videoFullRangeFlag );  

    vpsVui.setVideoSignalInfo( videoSignalInfos );       

    for (Int i = 0; i < m_numberOfLayers; i++)
    {      
      vpsVui.setVpsVideoSignalInfoIdx( i, 0 ); 
    }
    vpsVui.setVpsVuiBspHrdPresentFlag( false ); // TBD
  }
  else
  {
    //Default inference when not present.
    vpsVui.setCrossLayerIrapAlignedFlag   ( false   );
  }
  vps.setVPSVUI( vpsVui ); 
}

#if NH_3D
Void TAppEncTop::xSetCamPara                ( TComVPS& vps )
{
  vps.setCpPrecision( m_cCameraData.getCamParsCodedPrecision()); 

  for ( Int n = 1; n < vps.getNumViews(); n++ )
  {  
    Int i      = vps.getViewOIdxList( n ); 
    Int iInVps = vps.getVoiInVps    ( i ); 
    vps.setNumCp( iInVps,  n);   

    if ( vps.getNumCp( iInVps ) > 0 )
    {
      vps.setCpInSliceSegmentHeaderFlag( iInVps, m_cCameraData.getVaryingCameraParameters() );

      for( Int m = 0; m < vps.getNumCp( iInVps ); m++ )
      {
        vps.setCpRefVoi( iInVps, m, vps.getViewOIdxList( m ) ); 
        if( !vps.getCpInSliceSegmentHeaderFlag( iInVps ) ) 
        {
          Int j = vps.getCpRefVoi( iInVps, m );
          Int jInVps = vps.getVoiInVps( j );         

          vps.setVpsCpScale   ( iInVps, jInVps, m_cCameraData.getCodedScale() [ jInVps ][ iInVps ] ) ;
          vps.setVpsCpInvScale( iInVps, jInVps, m_cCameraData.getCodedScale() [ iInVps ][ jInVps ] ) ;
          vps.setVpsCpOff     ( iInVps, jInVps, m_cCameraData.getCodedOffset()[ jInVps ][ iInVps ] ) ;
          vps.setVpsCpInvOff  ( iInVps, jInVps, m_cCameraData.getCodedOffset()[ iInVps ][ jInVps ] ) ;
        }
      }
    }
  }
  vps.deriveCpPresentFlag(); 
}
#endif


Bool TAppEncTop::xLayerIdInTargetEncLayerIdList(Int nuhLayerId)
{
  return  ( std::find(m_targetEncLayerIdList.begin(), m_targetEncLayerIdList.end(), nuhLayerId) != m_targetEncLayerIdList.end()) ;
}


#endif


#if NH_3D_DLT
Void TAppEncTop::xDeriveDltArray( TComVPS& vps, TComDLT* dlt )
{
  Int  iNumDepthViews  = 0;
  Bool bDltPresentFlag = false;

  for ( Int layer = 0; layer <= vps.getMaxLayersMinus1(); layer++ )
  {
    Bool isDepth = ( vps.getDepthId( layer ) == 1 );

    if ( isDepth )
    {
      iNumDepthViews++;
    }

    dlt->setUseDLTFlag( layer , isDepth && m_useDLT );
    if( dlt->getUseDLTFlag( layer ) )
    {
      xAnalyzeInputBaseDepth(layer, max(m_iIntraPeriod[layer], 24), &vps, dlt);
      bDltPresentFlag = bDltPresentFlag || dlt->getUseDLTFlag(layer);
      dlt->setInterViewDltPredEnableFlag(layer, (dlt->getUseDLTFlag(layer) && (layer>1)));
      
      // ----------------------------- determine whether to use bit-map -----------------------------
      Bool bDltBitMapRepFlag       = false;
      UInt uiNumBitsNonBitMap      = 0;
      UInt uiNumBitsBitMap         = 0;
      
      UInt uiMaxDiff               = 0;
      UInt uiMinDiff               = MAX_INT;
      UInt uiLengthMinDiff         = 0;
      UInt uiLengthDltDiffMinusMin = 0;
      
      std::vector<Int> aiIdx2DepthValue_coded(256, 0);
      UInt uiNumDepthValues_coded = 0;
      
      uiNumDepthValues_coded = dlt->getNumDepthValues(layer);
      for( UInt ui = 0; ui<uiNumDepthValues_coded; ui++ )
      {
        aiIdx2DepthValue_coded[ui] = dlt->idx2DepthValue(layer, ui);
      }
      
      if( dlt->getInterViewDltPredEnableFlag( layer ) )
      {
        AOF( vps.getDepthId( 1 ) == 1 );
        AOF( layer > 1 );
        // assumes ref layer id to be 1
        std::vector<Int> piRefDLT = dlt->idx2DepthValue( 1 );
        UInt uiRefNum = dlt->getNumDepthValues( 1 );
        dlt->getDeltaDLT(layer, piRefDLT, uiRefNum, aiIdx2DepthValue_coded, uiNumDepthValues_coded);
      }
      
      std::vector<UInt> puiDltDiffValues(uiNumDepthValues_coded, 0);
      
      for (UInt d = 1; d < uiNumDepthValues_coded; d++)
      {
        puiDltDiffValues[d] = aiIdx2DepthValue_coded[d] - aiIdx2DepthValue_coded[d-1];
        
        if ( uiMaxDiff < puiDltDiffValues[d] )
        {
          uiMaxDiff = puiDltDiffValues[d];
        }
        
        if ( uiMinDiff > puiDltDiffValues[d] )
        {
          uiMinDiff = puiDltDiffValues[d];
        }
      }
      
      // counting bits
      // diff coding branch
      uiNumBitsNonBitMap += 8;                          // u(v) bits for num_depth_values_in_dlt[layerId] (i.e. num_entry[ layerId ])
      
      if ( uiNumDepthValues_coded > 1 )
      {
        uiNumBitsNonBitMap += 8;                        // u(v) bits for max_diff[ layerId ]
      }
      
      if ( uiNumDepthValues_coded > 2 )
      {
        uiLengthMinDiff    = (UInt) gCeilLog2(uiMaxDiff + 1);
        uiNumBitsNonBitMap += uiLengthMinDiff;          // u(v)  bits for min_diff[ layerId ]
      }
      
      uiNumBitsNonBitMap += 8;                          // u(v) bits for dlt_depth_value0[ layerId ]
      
      if (uiMaxDiff > uiMinDiff)
      {
        uiLengthDltDiffMinusMin = (UInt) gCeilLog2(uiMaxDiff - uiMinDiff + 1);
        uiNumBitsNonBitMap += uiLengthDltDiffMinusMin * (uiNumDepthValues_coded - 1);  // u(v) bits for dlt_depth_value_diff_minus_min[ layerId ][ j ]
      }
      
      // bit map branch
      uiNumBitsBitMap = 1 << m_inputBitDepth[CHANNEL_TYPE_LUMA];
      
      // determine bDltBitMapFlag
      bDltBitMapRepFlag = (uiNumBitsBitMap > uiNumBitsNonBitMap) ? false : true;
      
      dlt->setUseBitmapRep(layer, bDltBitMapRepFlag);
    }
  }

  dlt->setDltPresentFlag( bDltPresentFlag );
  dlt->setNumDepthViews ( iNumDepthViews  );
  dlt->setDepthViewBitDepth( m_inputBitDepth[CHANNEL_TYPE_LUMA] );
}
#endif
//! \}
