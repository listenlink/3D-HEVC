/* The copyright in this software is being made available under the BSD
 * License, included below. This software may be subject to other third party
 * and contributor rights, including patent rights, and no such rights are
 * granted under this license.
 *
 * Copyright (c) 2010-2011, ISO/IEC
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
 *  * Neither the name of the ISO/IEC nor the names of its contributors may
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
#include <stdio.h>
#include <fcntl.h>
#include <assert.h>

#include "TAppEncTop.h"

// ====================================================================================================================
// Constructor / destructor / initialization / destroy
// ====================================================================================================================

TAppEncTop::TAppEncTop()
{
#if HHI_VSO
  m_iLastFramePutInERViewBuffer = -1;
#endif
}

TAppEncTop::~TAppEncTop()
{
}


Void TAppEncTop::xInitLibCfg()
{
  for(Int iViewIdx=0; iViewIdx<m_iNumberOfViews; iViewIdx++)
  {
    m_iFrameRcvdVector.push_back(0) ;
    m_acTEncTopList.push_back(new TEncTop);

    m_acTVideoIOYuvInputFileList.push_back(new TVideoIOYuv);

    m_acTVideoIOYuvReconFileList.push_back(new TVideoIOYuv);
    m_cListPicYuvRecList.push_back(new TComList<TComPicYuv*>) ;
    m_aiNextPocToDump.push_back( 0 );
    m_cListPicYuvRecMap.push_back( std::map<PicOrderCnt,TComPicYuv*>() );
    m_acTEncTopList[iViewIdx]->setFrameRate                    ( m_iFrameRate );
    m_acTEncTopList[iViewIdx]->setFrameSkip                    ( m_FrameSkip );
    m_acTEncTopList[iViewIdx]->setSourceWidth                  ( m_iSourceWidth );
    m_acTEncTopList[iViewIdx]->setSourceHeight                 ( m_iSourceHeight );
    m_acTEncTopList[iViewIdx]->setFrameToBeEncoded             ( m_iFrameToBeEncoded );

  //====== Coding Structure ========
#if DCM_DECODING_REFRESH
    m_acTEncTopList[iViewIdx]->setDecodingRefreshType          ( m_iDecodingRefreshType );
#endif
    m_acTEncTopList[iViewIdx]->setCPSSize                      ( m_uiCodedPictureStoreSize );
    m_acTEncTopList[iViewIdx]->setGOPSize                      ( m_iGOPSize );
    m_acTEncTopList[iViewIdx]->setRateGOPSize                  ( m_iRateGOPSize );

    m_acTEncTopList[iViewIdx]->setSeqStructure                              ( m_cInputFormatString );

    m_acTEncTopList[iViewIdx]->setQP                           ( m_aiQP[0] );

    m_acTEncTopList[iViewIdx]->setTemporalLayerQPOffset        ( m_aiTLayerQPOffset );
    m_acTEncTopList[iViewIdx]->setPad                          ( m_aiPad );

    //===== Slice ========

    //====== Entropy Coding ========
    m_acTEncTopList[iViewIdx]->setSymbolMode                   ( m_iSymbolMode );

    //====== Loop/Deblock Filter ========
    m_acTEncTopList[iViewIdx]->setLoopFilterDisable            ( m_abLoopFilterDisable[0]   );
    m_acTEncTopList[iViewIdx]->setLoopFilterAlphaC0Offset      ( m_iLoopFilterAlphaC0Offset );
    m_acTEncTopList[iViewIdx]->setLoopFilterBetaOffset         ( m_iLoopFilterBetaOffset    );

    //====== Motion search ========
    m_acTEncTopList[iViewIdx]->setFastSearch                   ( m_iFastSearch  );
    m_acTEncTopList[iViewIdx]->setSearchRange                  ( m_iSearchRange );
    m_acTEncTopList[iViewIdx]->setBipredSearchRange            ( m_bipredSearchRange );
    m_acTEncTopList[iViewIdx]->setMaxDeltaQP                   ( m_iMaxDeltaQP  );

#if HHI_VSO
    //====== VSO =========
    m_acTEncTopList[iViewIdx]->setForceLambdaScaleVSO          ( false );
    m_acTEncTopList[iViewIdx]->setLambdaScaleVSO               ( 1     );
    m_acTEncTopList[iViewIdx]->setVSOMode                      ( 0     );
#endif

    //====== Tool list ========
    m_acTEncTopList[iViewIdx]->setUseSBACRD                    ( m_bUseSBACRD   );
    m_acTEncTopList[iViewIdx]->setDeltaQpRD                    ( m_uiDeltaQpRD  );
    m_acTEncTopList[iViewIdx]->setUseASR                       ( m_bUseASR      );
    m_acTEncTopList[iViewIdx]->setUseHADME                     ( m_bUseHADME    );
    m_acTEncTopList[iViewIdx]->setUseALF                       ( m_abUseALF[0]  );
#if MQT_ALF_NPASS
    m_acTEncTopList[iViewIdx]->setALFEncodePassReduction       ( m_iALFEncodePassReduction );
#endif
#if DCM_COMB_LIST
    m_acTEncTopList[iViewIdx]->setUseLComb                     ( m_bUseLComb    );
    m_acTEncTopList[iViewIdx]->setLCMod                        ( m_bLCMod         );
#endif
    m_acTEncTopList[iViewIdx]->setdQPs                         ( m_aidQP        );
    m_acTEncTopList[iViewIdx]->setUseRDOQ                      ( m_abUseRDOQ[0] );
    m_acTEncTopList[iViewIdx]->setUseLDC                       ( m_bUseLDC      );
    m_acTEncTopList[iViewIdx]->setUsePAD                       ( m_bUsePAD      );
    m_acTEncTopList[iViewIdx]->setQuadtreeTULog2MaxSize        ( m_uiQuadtreeTULog2MaxSize );
    m_acTEncTopList[iViewIdx]->setQuadtreeTULog2MinSize        ( m_uiQuadtreeTULog2MinSize );
    m_acTEncTopList[iViewIdx]->setQuadtreeTUMaxDepthInter      ( m_uiQuadtreeTUMaxDepthInter );
    m_acTEncTopList[iViewIdx]->setQuadtreeTUMaxDepthIntra      ( m_uiQuadtreeTUMaxDepthIntra );
    m_acTEncTopList[iViewIdx]->setUseFastEnc                   ( m_bUseFastEnc  );

#if HHI_VSO
    m_acTEncTopList[iViewIdx]->setUseVSO                       ( false ); //GT: might be enabled later for VSO Mode 4
#endif

    m_acTEncTopList[iViewIdx]->setViewId                       ( (UInt)iViewIdx );
    m_acTEncTopList[iViewIdx]->setViewOrderIdx                 ( m_cCameraData.getViewOrderIndex()[ iViewIdx ] );
    m_acTEncTopList[iViewIdx]->setIsDepth                      ( false );
    m_acTEncTopList[iViewIdx]->setCamParPrecision              ( m_cCameraData.getCamParsCodedPrecision  () );
    m_acTEncTopList[iViewIdx]->setCamParInSliceHeader          ( m_cCameraData.getVaryingCameraParameters() );
    m_acTEncTopList[iViewIdx]->setCodedScale                   ( m_cCameraData.getCodedScale             () );
    m_acTEncTopList[iViewIdx]->setCodedOffset                  ( m_cCameraData.getCodedOffset            () );
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


#if HHI_INTERVIEW_SKIP
    m_acTEncTopList[iViewIdx]->setInterViewSkip            ( iViewIdx ? m_uiInterViewSkip : 0 );
#if HHI_INTERVIEW_SKIP_LAMBDA_SCALE
    m_acTEncTopList[iViewIdx]->setInterViewSkipLambdaScale ( iViewIdx ? (Int)m_dInterViewSkipLambdaScale : 1 );
#endif
#endif
    m_acTEncTopList[iViewIdx]->setUseMRG                       ( m_bUseMRG      ); // SOPH:

#if LM_CHROMA
    m_acTEncTopList[iViewIdx]->setUseLMChroma                  ( m_bUseLMChroma );
#endif

#if HHI_RMP_SWITCH
    m_acTEncTopList[iViewIdx]->setUseRMP                     ( m_bUseRMP );
#endif
#ifdef ROUNDING_CONTROL_BIPRED
    m_acTEncTopList[iViewIdx]->setUseRoundingControlBipred(m_useRoundingControlBipred);
#endif
#if HHI_DMM_WEDGE_INTRA || HHI_DMM_PRED_TEX
    m_acTEncTopList[iViewIdx]->setUseDMM( false );
#endif
#if CONSTRAINED_INTRA_PRED
    m_acTEncTopList[iViewIdx]->setUseConstrainedIntraPred      ( m_bUseConstrainedIntraPred );
#endif
#ifdef WEIGHT_PRED
    //====== Weighted Prediction ========
    m_acTEncTopList[iViewIdx]->setUseWP                         ( m_bUseWeightPred      );
    m_acTEncTopList[iViewIdx]->setWPBiPredIdc                   ( m_uiBiPredIdc         );
#endif
    //====== Slice ========
    m_acTEncTopList[iViewIdx]->setSliceMode               ( m_iSliceMode                );
    m_acTEncTopList[iViewIdx]->setSliceArgument           ( m_iSliceArgument            );

    //====== Entropy Slice ========
    m_acTEncTopList[iViewIdx]->setEntropySliceMode        ( m_iEntropySliceMode         );
    m_acTEncTopList[iViewIdx]->setEntropySliceArgument    ( m_iEntropySliceArgument     );
#if MTK_NONCROSS_INLOOP_FILTER
    if(m_iSliceMode == 0 )
    {
      m_bLFCrossSliceBoundaryFlag = true;
    }
    m_acTEncTopList[iViewIdx]->setLFCrossSliceBoundaryFlag( m_bLFCrossSliceBoundaryFlag );
#endif
#if MTK_SAO
    m_acTEncTopList[iViewIdx]->setUseSAO               ( m_abUseSAO[0]     );
#endif
#if HHI_MPI
    m_acTEncTopList[iViewIdx]->setUseMVI( false );
#endif

    m_acTEncTopList[iViewIdx]->setPictureDigestEnabled(m_pictureDigestEnabled);
    m_acTEncTopList[iViewIdx]->setQpChangeFrame( m_iQpChangeFrame );
    m_acTEncTopList[iViewIdx]->setQpChangeOffsetVideo( m_iQpChangeOffsetVideo );
    m_acTEncTopList[iViewIdx]->setQpChangeOffsetDepth( m_iQpChangeOffsetDepth );
  }
  if( m_bUsingDepthMaps )
  {

#if HHI_VSO
    for (Int iViewIdx=0; iViewIdx<m_iNumberOfExternalRefs; iViewIdx++)
    {
        m_acTVideoIOYuvERFileList.push_back(new TVideoIOYuv);
    }
#endif

    for(Int iViewIdx=0; iViewIdx<m_iNumberOfViews; iViewIdx++)
    {
      m_iDepthFrameRcvdVector.push_back(0) ;
      m_acTEncDepthTopList.push_back(new TEncTop);

      m_acTVideoIOYuvDepthInputFileList.push_back(new TVideoIOYuv);

      m_acTVideoIOYuvDepthReconFileList.push_back(new TVideoIOYuv);
      m_cListPicYuvDepthRecList.push_back(new TComList<TComPicYuv*>) ;
      m_aiNextDepthPocToDump.push_back( 0 );
      m_cListPicYuvDepthRecMap.push_back( std::map<PicOrderCnt,TComPicYuv*>() );
      m_acTEncDepthTopList[iViewIdx]->setFrameRate                    ( m_iFrameRate );
      m_acTEncDepthTopList[iViewIdx]->setFrameSkip                    ( m_FrameSkip );
      m_acTEncDepthTopList[iViewIdx]->setSourceWidth                  ( m_iSourceWidth );
      m_acTEncDepthTopList[iViewIdx]->setSourceHeight                 ( m_iSourceHeight );
      m_acTEncDepthTopList[iViewIdx]->setFrameToBeEncoded             ( m_iFrameToBeEncoded );

      //====== Coding Structure ========
#if DCM_DECODING_REFRESH
      m_acTEncDepthTopList[iViewIdx]->setDecodingRefreshType          ( m_iDecodingRefreshType );
#endif
      m_acTEncDepthTopList[iViewIdx]->setCPSSize                      ( m_uiCodedPictureStoreSize );
      m_acTEncDepthTopList[iViewIdx]->setGOPSize                      ( m_iGOPSize );
      m_acTEncDepthTopList[iViewIdx]->setRateGOPSize                  ( m_iRateGOPSize );

      m_acTEncDepthTopList[iViewIdx]->setSeqStructure                              ( m_cInputFormatString );

      m_acTEncDepthTopList[iViewIdx]->setQP                           ( m_aiQP[1] );

      m_acTEncDepthTopList[iViewIdx]->setTemporalLayerQPOffset        ( m_aiTLayerQPOffset );
      m_acTEncDepthTopList[iViewIdx]->setPad                          ( m_aiPad );

      //===== Slice ========

      //====== Entropy Coding ========
      m_acTEncDepthTopList[iViewIdx]->setSymbolMode                   ( m_iSymbolMode );

      //====== Loop/Deblock Filter ========
      m_acTEncDepthTopList[iViewIdx]->setLoopFilterDisable            ( m_abLoopFilterDisable[1]   );
      m_acTEncDepthTopList[iViewIdx]->setLoopFilterAlphaC0Offset      ( m_iLoopFilterAlphaC0Offset );
      m_acTEncDepthTopList[iViewIdx]->setLoopFilterBetaOffset         ( m_iLoopFilterBetaOffset    );

      //====== Motion search ========
      m_acTEncDepthTopList[iViewIdx]->setFastSearch                   ( m_iFastSearch  );
      m_acTEncDepthTopList[iViewIdx]->setSearchRange                  ( m_iSearchRange );
      m_acTEncDepthTopList[iViewIdx]->setBipredSearchRange            ( m_bipredSearchRange );
      m_acTEncDepthTopList[iViewIdx]->setMaxDeltaQP                   ( m_iMaxDeltaQP  );

      //====== Tool list ========
      m_acTEncDepthTopList[iViewIdx]->setUseSBACRD                    ( m_bUseSBACRD   );
      m_acTEncDepthTopList[iViewIdx]->setDeltaQpRD                    ( m_uiDeltaQpRD  );
      m_acTEncDepthTopList[iViewIdx]->setUseASR                       ( m_bUseASR      );
      m_acTEncDepthTopList[iViewIdx]->setUseHADME                     ( m_bUseHADME    );
      m_acTEncDepthTopList[iViewIdx]->setUseALF                       ( m_abUseALF[1]  );
#if MQT_ALF_NPASS
      m_acTEncDepthTopList[iViewIdx]->setALFEncodePassReduction       ( m_iALFEncodePassReduction );
#endif
#if DCM_COMB_LIST
      m_acTEncDepthTopList[iViewIdx]->setUseLComb                     ( m_bUseLComb    );
      m_acTEncDepthTopList[iViewIdx]->setLCMod                        ( m_bLCMod         );
#endif
      m_acTEncDepthTopList[iViewIdx]->setdQPs                         ( m_aidQP        );
      m_acTEncDepthTopList[iViewIdx]->setUseRDOQ                      ( m_abUseRDOQ[1] );
      m_acTEncDepthTopList[iViewIdx]->setUseLDC                       ( m_bUseLDC      );
      m_acTEncDepthTopList[iViewIdx]->setUsePAD                       ( m_bUsePAD      );
      m_acTEncDepthTopList[iViewIdx]->setQuadtreeTULog2MaxSize        ( m_uiQuadtreeTULog2MaxSize );
      m_acTEncDepthTopList[iViewIdx]->setQuadtreeTULog2MinSize        ( m_uiQuadtreeTULog2MinSize );
      m_acTEncDepthTopList[iViewIdx]->setQuadtreeTUMaxDepthInter      ( m_uiQuadtreeTUMaxDepthInter );
      m_acTEncDepthTopList[iViewIdx]->setQuadtreeTUMaxDepthIntra      ( m_uiQuadtreeTUMaxDepthIntra );
      m_acTEncDepthTopList[iViewIdx]->setUseFastEnc                   ( m_bUseFastEnc  );

#if HHI_VSO
      m_acTEncDepthTopList[iViewIdx]->setUseVSO                       ( m_bUseVSO      ); //GT: might be enabled/disabled later for VSO Mode 4
      m_acTEncDepthTopList[iViewIdx]->setForceLambdaScaleVSO          ( m_bForceLambdaScaleVSO );
      m_acTEncDepthTopList[iViewIdx]->setLambdaScaleVSO               ( m_dLambdaScaleVSO );
#if HHI_VSO_DIST_INT
      m_acTEncDepthTopList[iViewIdx]->setAllowNegDist                 ( m_bAllowNegDist );
#endif
      m_acTEncDepthTopList[iViewIdx]->setVSOMode                      ( m_uiVSOMode );
#endif

      m_acTEncDepthTopList[iViewIdx]->setViewId                       ( (UInt)iViewIdx );
      m_acTEncDepthTopList[iViewIdx]->setViewOrderIdx                 ( m_cCameraData.getViewOrderIndex()[ iViewIdx ] );
      m_acTEncDepthTopList[iViewIdx]->setIsDepth                      ( true );
      m_acTEncDepthTopList[iViewIdx]->setCamParPrecision              ( 0 );
      m_acTEncDepthTopList[iViewIdx]->setCamParInSliceHeader          ( false );
      m_acTEncDepthTopList[iViewIdx]->setCodedScale                   ( 0 );
      m_acTEncDepthTopList[iViewIdx]->setCodedOffset                  ( 0 );
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

#if HHI_INTERVIEW_SKIP
      m_acTEncDepthTopList[iViewIdx]->setInterViewSkip            ( 0 );
#if HHI_INTERVIEW_SKIP_LAMBDA_SCALE
      m_acTEncDepthTopList[iViewIdx]->setInterViewSkipLambdaScale ( 1 );
#endif
#endif
      m_acTEncDepthTopList[iViewIdx]->setUseMRG                       ( m_bUseMRG      ); // SOPH:

#if LM_CHROMA
      m_acTEncDepthTopList[iViewIdx]->setUseLMChroma                  ( m_bUseLMChroma );
#endif

#if HHI_RMP_SWITCH
      m_acTEncDepthTopList[iViewIdx]->setUseRMP                     ( m_bUseRMP );
#endif
#ifdef ROUNDING_CONTROL_BIPRED
      m_acTEncDepthTopList[iViewIdx]->setUseRoundingControlBipred(m_useRoundingControlBipred);
#endif
#if HHI_DMM_WEDGE_INTRA || HHI_DMM_PRED_TEX
      m_acTEncDepthTopList[iViewIdx]->setUseDMM( m_bUseDMM );
#endif
#if CONSTRAINED_INTRA_PRED
      m_acTEncDepthTopList[iViewIdx]->setUseConstrainedIntraPred      ( m_bUseConstrainedIntraPred );
#endif
#ifdef WEIGHT_PRED
    //====== Weighted Prediction ========
      m_acTEncDepthTopList[iViewIdx]->setUseWP                         ( m_bUseWeightPred      );
      m_acTEncDepthTopList[iViewIdx]->setWPBiPredIdc                   ( m_uiBiPredIdc         );
#endif
      //====== Slice ========
      m_acTEncDepthTopList[iViewIdx]->setSliceMode               ( m_iSliceMode                );
      m_acTEncDepthTopList[iViewIdx]->setSliceArgument           ( m_iSliceArgument            );

      //====== Entropy Slice ========
      m_acTEncDepthTopList[iViewIdx]->setEntropySliceMode        ( m_iEntropySliceMode         );
      m_acTEncDepthTopList[iViewIdx]->setEntropySliceArgument    ( m_iEntropySliceArgument     );
#if MTK_NONCROSS_INLOOP_FILTER
      if(m_iSliceMode == 0 )
      {
        m_bLFCrossSliceBoundaryFlag = true;
      }
      m_acTEncDepthTopList[iViewIdx]->setLFCrossSliceBoundaryFlag( m_bLFCrossSliceBoundaryFlag );
#endif
#if MTK_SAO
      m_acTEncDepthTopList[iViewIdx]->setUseSAO               ( m_abUseSAO[1]     );
#endif
#if HHI_MPI
      m_acTEncDepthTopList[iViewIdx]->setUseMVI( m_bUseMVI );
#endif

      m_acTEncDepthTopList[iViewIdx]->setPictureDigestEnabled(m_pictureDigestEnabled);

      m_acTEncDepthTopList[iViewIdx]->setQpChangeFrame( m_iQpChangeFrame );
      m_acTEncDepthTopList[iViewIdx]->setQpChangeOffsetVideo( m_iQpChangeOffsetVideo );
      m_acTEncDepthTopList[iViewIdx]->setQpChangeOffsetDepth( m_iQpChangeOffsetDepth );
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
      m_cRendererModel.create( m_cRenModStrParser.getNumOfBaseViews(), m_cRenModStrParser.getNumOfModels(), m_iSourceWidth, m_iSourceHeight, LOG2_DISP_PREC_LUT, 0 );

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
      m_cRendererTop.init(m_iSourceWidth, m_iSourceHeight,true,0,0,true, 0,0,0,0,0,0,0,1,0,0 );  //GT: simplest configuration
    }
  }
#endif

#if POZNAN_SYNTH
  //m_cAvailabilityRenderer.init(m_iSourceWidth, m_iSourceHeight,true,0,0,true, 0,0,0,0,0,0,0,1,0,0 );  //GT: simplest configuration
  m_cAvailabilityRenderer.init(m_iSourceWidth, m_iSourceHeight,true,0,LOG2_DISP_PREC_LUT,true, 0,0,0,0,0,6,4,1,0,6 );  //GT: simplest configuration
#endif

#if HHI_INTERVIEW_SKIP
  m_cUsedPelsRenderer.init(m_iSourceWidth, m_iSourceHeight, true, 0, LOG2_DISP_PREC_LUT, true, 0, 0, 0, 0, 0, 6, 4, 1, 0, 6 );
#endif
}

Void TAppEncTop::xCreateLib()
{
  // Video I/O
  m_cTVideoIOBitsFile.openBits( m_pchBitstreamFile, true  );  // write mode

  for(Int iViewIdx=0; iViewIdx<m_iNumberOfViews; iViewIdx++)
  {
    m_acTVideoIOYuvInputFileList[iViewIdx]->open( m_pchInputFileList[iViewIdx],     false, m_uiInputBitDepth, m_uiInternalBitDepth );  // read  mode
    m_acTVideoIOYuvInputFileList[iViewIdx]->skipFrames(m_FrameSkip, m_iSourceWidth, m_iSourceHeight);
    m_acTVideoIOYuvReconFileList[iViewIdx]->open( m_pchReconFileList[iViewIdx],     true, m_uiOutputBitDepth, m_uiInternalBitDepth);  // write mode

    // Neo Decoder
    m_acTEncTopList[iViewIdx]->create();

    if( m_bUsingDepthMaps )
    {
      m_acTVideoIOYuvDepthInputFileList[iViewIdx]->open( m_pchDepthInputFileList[iViewIdx],     false, m_uiInputBitDepth, m_uiInternalBitDepth );  // read  mode
      m_acTVideoIOYuvDepthInputFileList[iViewIdx]->skipFrames(m_FrameSkip, m_iSourceWidth, m_iSourceHeight);

      m_acTVideoIOYuvDepthReconFileList[iViewIdx]->open( m_pchDepthReconFileList[iViewIdx],     true, m_uiOutputBitDepth, m_uiInternalBitDepth);  // write mode

      // Neo Decoder
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

#if HHI_VSO
  for(Int iViewIdx=0; iViewIdx < m_iNumberOfExternalRefs; iViewIdx++)
  {
    m_acTVideoIOYuvERFileList[iViewIdx]->open( m_pchERRefFileList[iViewIdx], false, m_uiInputBitDepth, m_uiInternalBitDepth ); // read mode
  }
#endif
}

Void TAppEncTop::xDestroyLib()
{

  m_cTVideoIOBitsFile.closeBits();

#if HHI_VSO
  for ( Int iViewIdx = 0; iViewIdx < m_iNumberOfExternalRefs; iViewIdx++ )
  {
    m_acTVideoIOYuvERFileList[iViewIdx]->close();
    delete m_acTVideoIOYuvERFileList[iViewIdx];
    m_acTVideoIOYuvERFileList[iViewIdx] = 0;
  };
#endif

  for(Int iViewIdx=0; iViewIdx<m_iNumberOfViews; iViewIdx++)
  {
    m_acTVideoIOYuvInputFileList[iViewIdx]->close();
    m_acTVideoIOYuvReconFileList[iViewIdx]->close();

    delete m_acTVideoIOYuvInputFileList[iViewIdx] ; m_acTVideoIOYuvInputFileList[iViewIdx] = NULL;
    delete m_acTVideoIOYuvReconFileList[iViewIdx] ; m_acTVideoIOYuvReconFileList[iViewIdx] = NULL;

    delete m_cListPicYuvRecList[iViewIdx] ; m_cListPicYuvRecList[iViewIdx] = NULL;

    m_acTEncTopList[iViewIdx]->destroy();
    delete m_acTEncTopList[iViewIdx] ; m_acTEncTopList[iViewIdx] = NULL;

    if( iViewIdx < Int( m_acTVideoIOYuvDepthInputFileList.size() ) && m_acTVideoIOYuvDepthInputFileList[iViewIdx] )
    {
      m_acTVideoIOYuvDepthInputFileList[iViewIdx]->close( );
      delete m_acTVideoIOYuvDepthInputFileList[iViewIdx] ;
    }
    if( iViewIdx < Int( m_acTVideoIOYuvDepthReconFileList.size() ) && m_acTVideoIOYuvDepthReconFileList[iViewIdx] )
    {
      m_acTVideoIOYuvDepthReconFileList[iViewIdx]->close () ;
      delete m_acTVideoIOYuvDepthReconFileList[iViewIdx];
    }
    if( iViewIdx < Int( m_acTEncDepthTopList.size() ) && m_acTEncDepthTopList[iViewIdx] )
    {
      m_acTEncDepthTopList[iViewIdx]->destroy();
      delete m_acTEncDepthTopList[iViewIdx];
    }
    if( iViewIdx < Int( m_cListPicYuvDepthRecList.size() ) && m_cListPicYuvDepthRecList[iViewIdx] )
    {
      delete m_cListPicYuvDepthRecList[iViewIdx ];
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
//GT PRE LOAD ENC BUFFER
Void TAppEncTop::encode()
{
  TComPicYuv*       pcPicYuvOrg      = new TComPicYuv;
  TComPicYuv*       pcDepthPicYuvOrg = new TComPicYuv;
  TComPicYuv*       pcPdmDepthOrg    = new TComPicYuv;
  TComPicYuv*       pcPicYuvRec      = NULL;
  //TComPicYuv*       pcDepthPicYuvRec = NULL;

  // initialize internal class & member variables
  xInitLibCfg();
  xCreateLib();
  xInitLib();

  // main encoder loop

  //GT: setup and init Bools for Eos and Continue Reading
  Bool  bAllEos = false;
  Bool  bAllContinueReadingPics;
  std::vector<Bool>  bEos ;
  std::vector<Bool>  bContinueReadingPics ;

  Bool  bAllDepthEos = false;
  Bool  bAllContinueReadingDepthPics;
  std::vector<Bool>  bDepthEos ;
  std::vector<Bool>  bContinueReadingDepthPics ;

  for(Int iViewIdx=0; iViewIdx < m_iNumberOfViews; iViewIdx++ )
  {
    bEos.push_back( false ) ;
    bContinueReadingPics.push_back( true );
    if( m_bUsingDepthMaps)
    {
      bDepthEos.push_back( false ) ;
      bContinueReadingDepthPics.push_back( true ) ;
    }
  }
  // allocate original YUV buffer
  pcPicYuvOrg->create( m_iSourceWidth, m_iSourceHeight, m_uiMaxCUWidth, m_uiMaxCUHeight, m_uiMaxCUDepth );
  if( m_bUsingDepthMaps)
  {
    pcDepthPicYuvOrg->create( m_iSourceWidth, m_iSourceHeight, m_uiMaxCUWidth, m_uiMaxCUHeight, m_uiMaxCUDepth );
  }
#if HHI_INTER_VIEW_MOTION_PRED
  if( m_uiMultiviewMvRegMode )
  {
    pcPdmDepthOrg->create( m_iSourceWidth, m_iSourceHeight, m_uiMaxCUWidth, m_uiMaxCUHeight, m_uiMaxCUDepth );
  }
#endif

  TComBitstream*    pcBitstream = new TComBitstream;
  pcBitstream->create( (m_iSourceWidth * m_iSourceHeight * 3) >> 1 ) ; //GT: is size reasonable ??

  while ( !(bAllEos&& bAllContinueReadingPics) )
  {
    bAllContinueReadingPics = false;
    bAllContinueReadingDepthPics = false;

    //GT: Read all Buffers
    for(Int iViewIdx=0; iViewIdx < m_iNumberOfViews; iViewIdx++ ) //GT; store frames first
    {
      if (!bEos[iViewIdx] && bContinueReadingPics[iViewIdx] ) //GT: read frames to buffer
      {
        // get buffers
        xGetBuffer( pcPicYuvRec, iViewIdx, m_cListPicYuvRecList ); // ringbuffer of size gopsize -> m_cListPicYuvRec, m_cListBitstream
        // read input YUV file
        m_acTVideoIOYuvInputFileList[iViewIdx]->read( pcPicYuvOrg, m_aiPad  ) ;
        bEos[iViewIdx] = ( m_acTVideoIOYuvInputFileList[iViewIdx]->isEof() == 1 ?   true : false  );             //GT: End of File
        bEos[iViewIdx] = ( m_iFrameRcvdVector[iViewIdx] == (m_iFrameToBeEncoded - 1) ?    true : bEos[iViewIdx]   );   //GT: FramesToBeEncoded Reached
        bAllEos = bAllEos|bEos[iViewIdx] ;

#if HHI_INTER_VIEW_MOTION_PRED
        if( m_uiMultiviewMvRegMode && iViewIdx )
        {
          m_acTVideoIOYuvDepthInputFileList[iViewIdx]->read( pcPdmDepthOrg, m_aiPad, m_bUsingDepthMaps );
        }
#endif

        // increase number of received frames
        m_iFrameRcvdVector[iViewIdx]++ ;
      }

#if HHI_INTER_VIEW_MOTION_PRED
      m_acTEncTopList[iViewIdx]->receivePic( bEos[iViewIdx],  pcPicYuvOrg, m_cListPicYuvRecList[iViewIdx]->back(), ( m_uiMultiviewMvRegMode && iViewIdx ? pcPdmDepthOrg : 0 ) );
#else
      m_acTEncTopList[iViewIdx]->receivePic( bEos[iViewIdx],  pcPicYuvOrg, m_cListPicYuvRecList[iViewIdx]->back(), 0 );
#endif

      if( m_bUsingDepthMaps )
      {
        if (!bDepthEos[iViewIdx] && bContinueReadingDepthPics[iViewIdx] )
        {
          // get buffers
          xGetBuffer( pcPicYuvRec, iViewIdx, m_cListPicYuvDepthRecList ); // ringbuffer of size gopsize -> m_cListPicYuvRec, m_cListBitstream
          // read input YUV file
          m_acTVideoIOYuvDepthInputFileList[iViewIdx]->read( pcDepthPicYuvOrg, m_aiPad  ) ;
          bDepthEos[iViewIdx] = ( m_acTVideoIOYuvDepthInputFileList[iViewIdx]->isEof() == 1 ?   true : false  );
          bDepthEos[iViewIdx] = ( m_iDepthFrameRcvdVector[iViewIdx] == (m_iFrameToBeEncoded - 1) ?    true : bDepthEos[iViewIdx]   );
          bAllDepthEos = bAllDepthEos|bDepthEos[iViewIdx] ;
          // increase number of received frames
          m_iDepthFrameRcvdVector[iViewIdx]++ ;
        }
        m_acTEncDepthTopList[iViewIdx]->receivePic( bDepthEos[iViewIdx],  pcDepthPicYuvOrg, m_cListPicYuvDepthRecList[iViewIdx]->back() );
      }
    }

    //===== store current POC =====
    Bool  bCurrPocCoded = m_acTEncTopList[ 0 ]->currentPocWillBeCoded();
    Int   iCurrPoc      = m_acTEncTopList[ 0 ]->getNextFrameId();

    //===== update camera parameters =====
    if( bCurrPocCoded )
    {
      m_cCameraData.update( (UInt)iCurrPoc );
    }

#if HHI_VSO    
    if ( m_bUseVSO && ( m_uiVSOMode != 4) )
    {
      //GT: Read external reference pics or render references
      xStoreVSORefPicsInBuffer();       //GT;
    }
#endif

    //GT: Encode
    for(Int iViewIdx=0; iViewIdx < m_iNumberOfViews; iViewIdx++ )     // Start encoding
    {
#if POZNAN_SYNTH
      xStoreSynthPicsInBuffer(iViewIdx,false);
#endif
      bool bThisViewContinueReadingPics = bContinueReadingPics[iViewIdx];
      m_acTEncTopList[iViewIdx]->encode( bEos[iViewIdx], m_cListPicYuvRecMap[iViewIdx], pcBitstream, bThisViewContinueReadingPics );
      bContinueReadingPics[iViewIdx]=bThisViewContinueReadingPics;
      bAllContinueReadingPics = bAllContinueReadingPics||bContinueReadingPics[iViewIdx];

      if(pcBitstream->getNumberOfWrittenBits()!=0)
      {
        m_cTVideoIOBitsFile.writeBits( pcBitstream );
      }
      pcBitstream->resetBits(); //GT: also done later in ....
      pcBitstream->rewindStreamPacket( );
      // write bistream to file if necessary
      xWriteOutput( iViewIdx ); //GT: Write Reconfiles (when gop is complete?)

      if( m_bUsingDepthMaps )
      {
#if POZNAN_SYNTH
        xStoreSynthPicsInBuffer(iViewIdx,true);
#endif
        bool bThisViewContinueReadingDepthPics = bContinueReadingDepthPics[iViewIdx];
        m_acTEncDepthTopList[iViewIdx]->encode( bDepthEos[iViewIdx], m_cListPicYuvDepthRecMap[iViewIdx], pcBitstream, bThisViewContinueReadingDepthPics );
        bContinueReadingDepthPics[iViewIdx]=bThisViewContinueReadingDepthPics;

        bAllContinueReadingDepthPics = bAllContinueReadingDepthPics||bContinueReadingDepthPics[iViewIdx];
        if(pcBitstream->getNumberOfWrittenBits()!=0)
        {
          m_cTVideoIOBitsFile.writeBits( pcBitstream );
        }
        pcBitstream->resetBits();
        pcBitstream->rewindStreamPacket( );
        // write bistream to file if necessary
        xWriteOutput( iViewIdx, true );
      }
    }

    // delete extra picture buffers
    if( bCurrPocCoded )
    {
      for( Int iViewIdx = 0; iViewIdx < m_iNumberOfViews; iViewIdx++ )
      {
        if( iViewIdx < (Int)m_acTEncTopList.size() && m_acTEncTopList[iViewIdx] )
        {
          m_acTEncTopList[iViewIdx]->deleteExtraPicBuffers( iCurrPoc );
        }
        if( iViewIdx < (Int)m_acTEncDepthTopList.size() && m_acTEncDepthTopList[iViewIdx] )
        {
          m_acTEncDepthTopList[iViewIdx]->deleteExtraPicBuffers( iCurrPoc );
        }
      }
    }

#if AMVP_BUFFERCOMPRESS
    // compress motion for entire access
    if( bCurrPocCoded )
    {
      for( Int iViewIdx = 0; iViewIdx < m_iNumberOfViews; iViewIdx++ )
      {
        if( iViewIdx < (Int)m_acTEncTopList.size() && m_acTEncTopList[iViewIdx] )
        {
          m_acTEncTopList[iViewIdx]->compressMotion( iCurrPoc );
        }
        if( iViewIdx < (Int)m_acTEncDepthTopList.size() && m_acTEncDepthTopList[iViewIdx] )
        {
          m_acTEncDepthTopList[iViewIdx]->compressMotion( iCurrPoc );
        }
      }
    }
#endif
  }

  // write output
  for(Int iViewIdx=0; iViewIdx < m_iNumberOfViews; iViewIdx++ )
  {
    m_acTEncTopList[iViewIdx]->printOutSummary(m_acTEncTopList[iViewIdx]->getNumAllPicCoded());
    if ( m_bUsingDepthMaps )
    {
      m_acTEncDepthTopList[iViewIdx]->printOutSummary(m_acTEncDepthTopList[iViewIdx]->getNumAllPicCoded());
    }
  }

  // delete original YUV buffer
  pcPicYuvOrg->destroy();
  delete pcPicYuvOrg;
  pcPicYuvOrg = NULL;

  // valgrind
  if( m_bUsingDepthMaps)
  {
    pcDepthPicYuvOrg->destroy();
  }
  delete pcDepthPicYuvOrg ;
  pcDepthPicYuvOrg = NULL ;

  pcBitstream->destroy();
  delete pcBitstream;
  pcBitstream = NULL ;


  // delete used buffers in encoder class
  for(Int iViewIdx =0; iViewIdx < m_iNumberOfViews; iViewIdx++)
  {
    m_acTEncTopList[iViewIdx]->deletePicBuffer() ;
  }

  if( m_bUsingDepthMaps)
  {
    for(Int iViewIdx =0; iViewIdx < m_iNumberOfViews; iViewIdx++)
    {
      m_acTEncDepthTopList[iViewIdx]->deletePicBuffer() ;
    }
  }

  if ( pcPdmDepthOrg )
  {
    pcPdmDepthOrg->destroy();
    delete pcPdmDepthOrg;
    pcPdmDepthOrg = NULL;
  }

  // delete buffers & classes
  xDeleteBuffer();
  xDestroyLib();
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
Void TAppEncTop::xGetBuffer( TComPicYuv*& rpcPicYuvRec, Int iViewIdx, std::vector< TComList<TComPicYuv*>*>& racBuffer )
{
  if ( m_uiCodedPictureStoreSize   == 0 )
    {
      if (racBuffer[iViewIdx]->size() == 0)
      {
        rpcPicYuvRec = new TComPicYuv;
        rpcPicYuvRec->create( m_iSourceWidth, m_iSourceHeight, m_uiMaxCUWidth, m_uiMaxCUHeight, m_uiMaxCUDepth );

        racBuffer[iViewIdx]->pushBack( rpcPicYuvRec );
      }
      rpcPicYuvRec = racBuffer[iViewIdx]->popFront(); //GT why? only one in list ?? A: only to get rpcPicYuvRec
      racBuffer[iViewIdx]->pushBack( rpcPicYuvRec );
      return;
    }

    // org. buffer
  if ( racBuffer[iViewIdx]->size() == (UInt)m_uiCodedPictureStoreSize )
    {
      rpcPicYuvRec = racBuffer[iViewIdx]->popFront();
    }
    else
    {
      rpcPicYuvRec = new TComPicYuv;
      rpcPicYuvRec->create( m_iSourceWidth, m_iSourceHeight, m_uiMaxCUWidth, m_uiMaxCUHeight, m_uiMaxCUDepth );
    }
    racBuffer[iViewIdx]->pushBack( rpcPicYuvRec );
}

Void TAppEncTop::xDeleteBuffer( )
{
  TComList<TComBitstream*>::iterator iterBitstream = m_cListBitstream.begin();

  Int iSize = Int( m_cListBitstream.size() );

  for ( Int i = 0; i < iSize; i++ )
  {
    TComBitstream* pcBitstream = *(iterBitstream++);

    pcBitstream->destroy();

    delete pcBitstream; pcBitstream = NULL;
  }

  for(Int iViewIdx=0; iViewIdx<m_iNumberOfViews; iViewIdx++)
  {
    TComList<TComPicYuv*>::iterator iterPicYuvRec  = m_cListPicYuvRecList[iViewIdx]->begin();

    iSize = Int( m_cListPicYuvRecList[iViewIdx]->size() );

    for ( Int i = 0; i < iSize; i++ )
    {
        TComPicYuv*  pcPicYuvRec  = *(iterPicYuvRec++);
        pcPicYuvRec->destroy();
        delete pcPicYuvRec; pcPicYuvRec = NULL;
    }
  }
  if( m_bUsingDepthMaps)
  {
    for(Int iViewIdx=0; iViewIdx<m_iNumberOfViews; iViewIdx++)
    {
      TComList<TComPicYuv*>::iterator iterPicYuvRec  = m_cListPicYuvDepthRecList[iViewIdx]->begin();

      iSize = Int( m_cListPicYuvDepthRecList[iViewIdx]->size() );

      for ( Int i = 0; i < iSize; i++ )
      {
          TComPicYuv*  pcPicYuvRec  = *(iterPicYuvRec++);
          pcPicYuvRec->destroy();
          delete pcPicYuvRec; pcPicYuvRec = NULL;
      }
    }
  }

  // Delete ERFiles

#if HHI_VSO
  std::map< Int,vector<TComPicYuv*> >::iterator iterMapPicExtRefView = m_cMapPicExtRefView.begin();
  while ( iterMapPicExtRefView != m_cMapPicExtRefView.end() )
  {
    for ( UInt uiViewNumber = 0; uiViewNumber < iterMapPicExtRefView->second.size(); uiViewNumber++ )
    {
      if ( iterMapPicExtRefView->second[uiViewNumber] )
      {
        iterMapPicExtRefView->second[uiViewNumber]->destroy();
        delete iterMapPicExtRefView->second[uiViewNumber];
      }
    }
    iterMapPicExtRefView++;
  }
#endif
}

/** \param iNumEncoded  number of encoded frames
 */
Void TAppEncTop::xWriteOutput( Int iViewIdx, Bool isDepth )
{
  std::map<PicOrderCnt, TComPicYuv*> &rcMap = ( isDepth ? m_cListPicYuvDepthRecMap          : m_cListPicYuvRecMap          )[iViewIdx];
  PicOrderCnt  &riNextPocToDump             = ( isDepth ? m_aiNextDepthPocToDump            : m_aiNextPocToDump            )[iViewIdx];
  TVideoIOYuv* &rpcTVideoIOYuvReconFile     = ( isDepth ? m_acTVideoIOYuvDepthReconFileList : m_acTVideoIOYuvReconFileList )[iViewIdx];
  std::map<PicOrderCnt, TComPicYuv*>::iterator i;

  while( ! rcMap.empty() && ( i = rcMap.begin() )->first == riNextPocToDump )
  {
    riNextPocToDump++;
    rpcTVideoIOYuvReconFile->write( i->second, m_aiPad );
    rcMap.erase( i );
  }
}

// GT FIX
std::vector<TComPic*> TAppEncTop::getSpatialRefPics( Int iViewIdx, Int iPoc, Bool bIsDepthCoder )
{
  std::vector<TComPic*> apcRefPics( iViewIdx, (TComPic*)NULL );
  for( int iRefViewIdx = 0; iRefViewIdx < iViewIdx; iRefViewIdx++ )
  {
// GT FIX
    TComPic* pcRefPic = xGetPicFromView(iRefViewIdx, iPoc, bIsDepthCoder);

    assert( pcRefPic != NULL );
    apcRefPics[iRefViewIdx] = pcRefPic;
  }
  return apcRefPics;
}


TComPic* TAppEncTop::xGetPicFromView( Int iViewIdx, Int iPoc, bool bDepth )
{
  assert( ( iViewIdx >= 0) && ( iViewIdx < m_iNumberOfViews ) );

  TComPic* pcPic = 0;
  TComList<TComPic*>* apcListPic;

  apcListPic = (bDepth ?  m_acTEncDepthTopList[iViewIdx] : m_acTEncTopList[iViewIdx])->getListPic() ;

  for(TComList<TComPic*>::iterator it=apcListPic->begin(); it!=apcListPic->end(); it++)
  {
    if( (*it)->getPOC() == iPoc )
    {
      pcPic = *it ;
      break ;
    }
  }

  return pcPic;
};

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

#if HHI_VSO
Void TAppEncTop::xSetBasePicYuv( Int iViewIdx, Int iPoc, TComMVDRefData* pcRefInfo, InterViewReference eView, bool bDepth )
{

  if ( ( iViewIdx < 0) || ( iViewIdx >= m_iNumberOfViews ) )
    return;

  if ( !m_bUsingDepthMaps && bDepth )
    return;

  TComPic* pcPic = xGetPicFromView( iViewIdx, iPoc, bDepth);

  if (pcPic == 0)
    return;

  pcRefInfo->setPicYuvBaseView(eView, bDepth, pcPic->getPicYuvOrg(),  pcPic->getReconMark() ? pcPic->getPicYuvRec() : NULL );

};

Void TAppEncTop::setMVDPic( Int iViewIdx, Int iPoc, TComMVDRefData* pcMVDRefData )
{
  AOF( iViewIdx >= 0);
  AOF( iViewIdx <  m_iNumberOfViews  );

  xSetBasePicYuv(iViewIdx - 1, iPoc, pcMVDRefData, PREVVIEW, false);
  xSetBasePicYuv(iViewIdx    , iPoc, pcMVDRefData, CURRVIEW, false );
  xSetBasePicYuv(iViewIdx + 1, iPoc, pcMVDRefData, NEXTVIEW, false );

  if ( m_bUsingDepthMaps )
  {
    xSetBasePicYuv(iViewIdx - 1, iPoc, pcMVDRefData, PREVVIEW, true );
    xSetBasePicYuv(iViewIdx    , iPoc, pcMVDRefData, CURRVIEW, true );
    xSetBasePicYuv(iViewIdx + 1, iPoc, pcMVDRefData, NEXTVIEW, true );
  }


  xSetBaseLUT       (iViewIdx, iViewIdx-1  , pcMVDRefData, PREVVIEW );
  xSetBaseLUT       (iViewIdx, iViewIdx+1  , pcMVDRefData, NEXTVIEW );


  if ( m_bUseVSO && m_uiVSOMode != 4)
  {
    xSetERPicYuvs               (iViewIdx, iPoc, pcMVDRefData);
    pcMVDRefData->setShiftLUTsERView(m_cCameraData.getSynthViewShiftLUTD()[iViewIdx],  m_cCameraData.getSynthViewShiftLUTI()[iViewIdx] );
    pcMVDRefData->setRefViewInd     (m_aaiBaseViewRefInd[iViewIdx], m_aaiERViewRefInd[iViewIdx], m_aaiERViewRefLutInd[iViewIdx]);
  }
};


Void TAppEncTop::xSetBaseLUT( Int iViewIdxSource, Int iViewIdxTarget, TComMVDRefData* pcRefInfo, InterViewReference eView )
{
  if ( ( iViewIdxSource < 0) || ( iViewIdxSource >= m_iNumberOfViews )||( iViewIdxTarget < 0) || ( iViewIdxTarget >= m_iNumberOfViews ) )
    return;
  assert( abs( iViewIdxTarget - iViewIdxSource ) <= 1 );  //GT; Not supported yet
  pcRefInfo->setShiftLUTsBaseView(eView, m_cCameraData.getBaseViewShiftLUTD()[iViewIdxSource][iViewIdxTarget],m_cCameraData.getBaseViewShiftLUTI()[iViewIdxSource][iViewIdxTarget] );
};

Void TAppEncTop::xSetERPicYuvs( Int iViewIdx, Int iPoc, TComMVDRefData* pcReferenceInfo )
{
  std::vector<TComPicYuv*> apcExtRefViews;

  std::map< Int, vector<TComPicYuv*> >::iterator cMapIt;
  cMapIt = m_cMapPicExtRefView.find(iPoc);

  assert(  cMapIt != m_cMapPicExtRefView.end() );

  pcReferenceInfo->setPicYuvERViews( cMapIt->second );
}

Void TAppEncTop::xStoreVSORefPicsInBuffer()
{
  // X-Check if all Encoders have received the same number of pics
  Int iNumRcvd = m_iFrameRcvdVector[0];
  for ( UInt uiViewNumber = 0; uiViewNumber < m_iNumberOfViews; uiViewNumber ++ )
  {
    assert( ( m_iFrameRcvdVector[uiViewNumber] == iNumRcvd ) && ( m_iDepthFrameRcvdVector[uiViewNumber] == iNumRcvd ) ); //GT; if assert here, the encoder instances are not synchronized any more, re-think this function and the ERView Buffer!!
  };

  Int iCurPoc = iNumRcvd - 1;

  if ( iCurPoc <= m_iLastFramePutInERViewBuffer )
  {
    return;
  }

  std::vector<TComPicYuv*> apcExtRefViewVec;

  Int iNumberOfReferenceViews = 0;
  if (m_iNumberOfExternalRefs != 0)
  {
    m_aaiERViewRefLutInd = m_aaiERViewRefInd;
    // Insert Rendered Views form File

    iNumberOfReferenceViews = m_iNumberOfExternalRefs;

    for ( UInt uiViewNumber = 0; uiViewNumber < iNumberOfReferenceViews; uiViewNumber++ )
    {
      TComPicYuv* pcPicYuvERView = new TComPicYuv;
      pcPicYuvERView->create( m_iSourceWidth, m_iSourceHeight, m_uiMaxCUWidth, m_uiMaxCUHeight, m_uiMaxCUDepth );

      m_acTVideoIOYuvERFileList[uiViewNumber]->read( pcPicYuvERView, m_aiPad  ) ;

      apcExtRefViewVec.push_back( pcPicYuvERView );
    }
  }
  else
  { //Render Views

    for ( UInt uiViewNumber = 0; uiViewNumber < m_iNumberOfViews; uiViewNumber++ )
    {
      m_aaiERViewRefInd   [uiViewNumber].clear();
      m_aaiERViewRefLutInd[uiViewNumber].clear();
    }

    iNumberOfReferenceViews = 0;
    for ( UInt iSynthViewIdx = 0; iSynthViewIdx < m_cCameraData.getSynthViewNumbers().size(); iSynthViewIdx++ )
    {
      // Get Left and right view
      Int  iLeftViewIdx  = -1;
      Int  iRightViewIdx = -1;
      Bool bIsBaseView;

      Int iRelDistToLeft;
      m_cCameraData.getLeftRightBaseView( iSynthViewIdx, iLeftViewIdx, iRightViewIdx, iRelDistToLeft, bIsBaseView );

      if  ((iLeftViewIdx == -1) || (iRightViewIdx == -1))
      {
        std::cerr << "Left or right View not given." << endl;
        exit(EXIT_FAILURE);
      }

      m_cRendererTop.setShiftLUTs(
        m_cCameraData.getSynthViewShiftLUTD()[iLeftViewIdx] [iSynthViewIdx],
        m_cCameraData.getSynthViewShiftLUTI()[iLeftViewIdx] [iSynthViewIdx],
        m_cCameraData.getBaseViewShiftLUTI ()[iLeftViewIdx] [iRightViewIdx],
        m_cCameraData.getSynthViewShiftLUTD()[iRightViewIdx][iSynthViewIdx],
        m_cCameraData.getSynthViewShiftLUTI()[iRightViewIdx][iSynthViewIdx],
        m_cCameraData.getBaseViewShiftLUTI ()[iRightViewIdx][iLeftViewIdx],
        iRelDistToLeft
      );
      if ( bIsBaseView ) continue;

      // Render from left
      TComPicYuv* pcPicYuvERView = new TComPicYuv;
      pcPicYuvERView->create( m_iSourceWidth, m_iSourceHeight, m_uiMaxCUWidth, m_uiMaxCUHeight, m_uiMaxCUDepth );
      m_cRendererTop.extrapolateView( xGetPicFromView( iLeftViewIdx, iCurPoc, false )->getPicYuvOrg(), xGetPicFromView( iLeftViewIdx, iCurPoc, true )->getPicYuvOrg(), pcPicYuvERView, true );

      apcExtRefViewVec.push_back( pcPicYuvERView );

      m_aaiERViewRefInd   [ iLeftViewIdx].push_back( iNumberOfReferenceViews );
      m_aaiERViewRefLutInd[ iLeftViewIdx].push_back( iSynthViewIdx );
      iNumberOfReferenceViews++;

      //Render from right
      pcPicYuvERView = new TComPicYuv;
      pcPicYuvERView->create( m_iSourceWidth, m_iSourceHeight, m_uiMaxCUWidth, m_uiMaxCUHeight, m_uiMaxCUDepth );

      m_cRendererTop.extrapolateView( xGetPicFromView( iRightViewIdx, iCurPoc, false )->getPicYuvOrg(), xGetPicFromView( iRightViewIdx, iCurPoc, true )->getPicYuvOrg(), pcPicYuvERView, false );
      apcExtRefViewVec.push_back( pcPicYuvERView );

      m_aaiERViewRefInd   [ iRightViewIdx].push_back( iNumberOfReferenceViews );
      m_aaiERViewRefLutInd[ iRightViewIdx].push_back( iSynthViewIdx );
      iNumberOfReferenceViews++;
    }
  }

  m_iLastFramePutInERViewBuffer++;
  m_cMapPicExtRefView.insert( std::make_pair( m_iLastFramePutInERViewBuffer, apcExtRefViewVec ) );

  if ( m_cMapPicExtRefView.size() >  (UInt)m_iGOPSize + 1 )
  {
    for ( UInt uiViewNumber = 0; uiViewNumber < (UInt) m_cMapPicExtRefView.begin()->second.size(); uiViewNumber++ )
    {
      if ( m_cMapPicExtRefView.begin()->second[uiViewNumber] )
      {
        m_cMapPicExtRefView.begin()->second[uiViewNumber]->destroy();
        delete m_cMapPicExtRefView.begin()->second[uiViewNumber];
      }
    }
    m_cMapPicExtRefView.erase ( m_cMapPicExtRefView.begin() );
  }
}
#endif

#if POZNAN_SYNTH
Void TAppEncTop::xStoreSynthPicsInBuffer(Int iCoddedViewIdx,Bool bDepth)
{
  Int iCurPoc;
  if(bDepth)
  {
    iCurPoc = m_acTEncDepthTopList[ iCoddedViewIdx ]->getNextFrameId();
    if (!(m_acTEncDepthTopList[ iCoddedViewIdx ]->currentPocWillBeCoded())) return;
  }
  else
  {
    iCurPoc = m_acTEncTopList[ iCoddedViewIdx ]->getNextFrameId();
    if (!(m_acTEncTopList[ iCoddedViewIdx ]->currentPocWillBeCoded())) return;
  }
  
  Int iNumberOfReferenceViews = 0;
  UInt iSynthViewIdx;
  // Get Left and right view
  Int  iLeftViewIdx  = -1;
  Int  iRightViewIdx = -1;
  Int  iNearestViewIdx = -1;
  Bool bIsBaseView;
  Bool bRenderFromLeft;

  Int iRelDistToLeft;
  if(iCoddedViewIdx==0) //First on View Coded List
  {
    //TComPic* pcPic = xGetPicFromView( iCoddedViewIdx, iCurPoc, false );
    return;
  }
  m_cCameraData.getNearestBaseView(iCoddedViewIdx, iNearestViewIdx, iRelDistToLeft, bRenderFromLeft);

  m_cAvailabilityRenderer.setShiftLUTs(
    m_cCameraData.getBaseViewShiftLUTD()[iNearestViewIdx][iCoddedViewIdx],
    m_cCameraData.getBaseViewShiftLUTI()[iNearestViewIdx][iCoddedViewIdx],
    m_cCameraData.getBaseViewShiftLUTI()[iNearestViewIdx][iCoddedViewIdx],
    m_cCameraData.getBaseViewShiftLUTD()[iNearestViewIdx][iCoddedViewIdx],//right
    m_cCameraData.getBaseViewShiftLUTI()[iNearestViewIdx][iCoddedViewIdx],
    m_cCameraData.getBaseViewShiftLUTI()[iNearestViewIdx][iCoddedViewIdx],
    iRelDistToLeft
  );
    

  TComPicYuv* pcPicYuvERView = new TComPicYuv;
  pcPicYuvERView->create( m_iSourceWidth, m_iSourceHeight, m_uiMaxCUWidth, m_uiMaxCUHeight, m_uiMaxCUDepth );

  TComPic* pcPic = xGetPicFromView( iCoddedViewIdx, iCurPoc, bDepth );
  pcPic->addSynthesisBuffer();
  pcPic->addAvailabilityBuffer();
  TComPicYuv* pcPicYuvSynthView = pcPic->getPicYuvSynth();
  TComPicYuv* pcPicYuvAvailView = pcPic->getPicYuvAvail();
  
  //m_cAvailabilityRenderer.extrapolateAvailabilityView( xGetPicFromView( iNearestViewIdx, iCurPoc, false )->getPicYuvRec(), xGetPicFromView( iNearestViewIdx, iCurPoc, true )->getPicYuvRec(), pcPicYuvERView, pcPicYuvAvailView, bRenderFromLeft );
  m_cAvailabilityRenderer.extrapolateAvailabilityView( xGetPicFromView( iNearestViewIdx, iCurPoc, bDepth )->getPicYuvRec(), xGetPicFromView( iNearestViewIdx, iCurPoc, true )->getPicYuvRec(), pcPicYuvSynthView, pcPicYuvAvailView, bRenderFromLeft );
      
  pcPicYuvAvailView->setBorderExtension( false );//Needed??
  pcPicYuvAvailView->extendPicBorder();//Needed??

  pcPicYuvSynthView->setBorderExtension( false );//Needed??
  pcPicYuvSynthView->extendPicBorder();//Needed??

  //TComPic* pcPicDepth = xGetPicFromView( iCoddedViewIdx, iCurPoc, true );
  //pcPicDepth->addAvailabilityBuffer();
  //pcPicDepth->addSynthesisBuffer();
  //pcPicYuvAvailView->copyToPic(pcPicDepth->getPicYuvAvail());
      
#if POZNAN_OUTPUT_AVAILABLE_MAP
  {
  Char acFilenameBase[1024];
  ::sprintf( acFilenameBase,  "Available_%s_%s_V%d.yuv", (bDepth?"Depth":"Tex"),( false ? "Dec" : "Enc" ),iCoddedViewIdx );
  pcPicYuvAvailView->dump(acFilenameBase, iCurPoc!=0);
  }
#endif
#if POZNAN_OUTPUT_SYNTH
  {
  Char acFilenameBase[1024];
  ::sprintf( acFilenameBase,  "Synth_%s_%s_V%d.yuv", (bDepth?"Depth":"Tex"),( false ? "Dec" : "Enc" ),iCoddedViewIdx );
  pcPicYuvERView->dump(acFilenameBase, iCurPoc!=0);
  }
#endif

      //Usun pcPicYuvERView i inne bufforki
}
#endif

#if HHI_INTERVIEW_SKIP
Void TAppEncTop::getUsedPelsMap( Int iViewIdx, Int iPoc, TComPicYuv* pcPicYuvUsedSplsMap )
{
  AOT( iViewIdx <= 0);
  AOT( iViewIdx >= m_iNumberOfViews );
  AOF( m_uiInterViewSkip != 0 );
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
Void TAppEncTop::setupRenModel( Int iPoc, Int iEncViewIdx, Int iEncContent )
{
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

