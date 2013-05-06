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

/** \file     TDecSbac.cpp
    \brief    Context-adaptive entropy decoder class
*/

#include "TDecSbac.h"

#if RWTH_SDC_DLT_B0036
#define GetNumDepthValues()     (pcCU->getSlice()->getSPS()->getNumDepthValues())
#define GetBitsPerDepthValue()  (pcCU->getSlice()->getSPS()->getBitsPerDepthValue())
#endif

//! \ingroup TLibDecoder
//! \{

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

TDecSbac::TDecSbac() 
// new structure here
: m_pcBitstream               ( 0 )
, m_pcTDecBinIf               ( NULL )
, m_numContextModels          ( 0 )
, m_cCUSplitFlagSCModel       ( 1,             1,               NUM_SPLIT_FLAG_CTX            , m_contextModels + m_numContextModels, m_numContextModels )
, m_cCUSkipFlagSCModel        ( 1,             1,               NUM_SKIP_FLAG_CTX             , m_contextModels + m_numContextModels, m_numContextModels)
#if LGE_ILLUCOMP_B0045
, m_cCUICFlagSCModel          ( 1,             1,               NUM_IC_FLAG_CTX               , m_contextModels + m_numContextModels, m_numContextModels)
#endif
, m_cCUMergeFlagExtSCModel    ( 1,             1,               NUM_MERGE_FLAG_EXT_CTX        , m_contextModels + m_numContextModels, m_numContextModels)
, m_cCUMergeIdxExtSCModel     ( 1,             1,               NUM_MERGE_IDX_EXT_CTX         , m_contextModels + m_numContextModels, m_numContextModels)
#if H3D_IVRP
, m_cResPredFlagSCModel       ( 1,             1,               NUM_RES_PRED_FLAG_CTX         , m_contextModels + m_numContextModels, m_numContextModels)
#endif
#if QC_ARP_D0177
, m_cCUPUARPW                 ( 1,             1,               NUM_ARPW_CTX                 , m_contextModels + m_numContextModels, m_numContextModels)
#endif
, m_cCUPartSizeSCModel        ( 1,             1,               NUM_PART_SIZE_CTX             , m_contextModels + m_numContextModels, m_numContextModels)
, m_cCUPredModeSCModel        ( 1,             1,               NUM_PRED_MODE_CTX             , m_contextModels + m_numContextModels, m_numContextModels)
, m_cCUAlfCtrlFlagSCModel     ( 1,             1,               NUM_ALF_CTRL_FLAG_CTX         , m_contextModels + m_numContextModels, m_numContextModels)
, m_cCUIntraPredSCModel       ( 1,             1,               NUM_ADI_CTX                   , m_contextModels + m_numContextModels, m_numContextModels)
, m_cCUChromaPredSCModel      ( 1,             1,               NUM_CHROMA_PRED_CTX           , m_contextModels + m_numContextModels, m_numContextModels)
, m_cCUDeltaQpSCModel         ( 1,             1,               NUM_DELTA_QP_CTX              , m_contextModels + m_numContextModels, m_numContextModels)
, m_cCUInterDirSCModel        ( 1,             1,               NUM_INTER_DIR_CTX             , m_contextModels + m_numContextModels, m_numContextModels)
, m_cCURefPicSCModel          ( 1,             1,               NUM_REF_NO_CTX                , m_contextModels + m_numContextModels, m_numContextModels)
, m_cCUMvdSCModel             ( 1,             1,               NUM_MV_RES_CTX                , m_contextModels + m_numContextModels, m_numContextModels)
, m_cCUQtCbfSCModel           ( 1,             2,               NUM_QT_CBF_CTX                , m_contextModels + m_numContextModels, m_numContextModels)
, m_cCUTransSubdivFlagSCModel ( 1,             1,               NUM_TRANS_SUBDIV_FLAG_CTX     , m_contextModels + m_numContextModels, m_numContextModels)
, m_cCUQtRootCbfSCModel       ( 1,             1,               NUM_QT_ROOT_CBF_CTX           , m_contextModels + m_numContextModels, m_numContextModels)
, m_cCUSigCoeffGroupSCModel   ( 1,             2,               NUM_SIG_CG_FLAG_CTX           , m_contextModels + m_numContextModels, m_numContextModels)
, m_cCUSigSCModel             ( 1,             1,               NUM_SIG_FLAG_CTX              , m_contextModels + m_numContextModels, m_numContextModels)
, m_cCuCtxLastX               ( 1,             2,               NUM_CTX_LAST_FLAG_XY          , m_contextModels + m_numContextModels, m_numContextModels)
, m_cCuCtxLastY               ( 1,             2,               NUM_CTX_LAST_FLAG_XY          , m_contextModels + m_numContextModels, m_numContextModels)
, m_cCUOneSCModel             ( 1,             1,               NUM_ONE_FLAG_CTX              , m_contextModels + m_numContextModels, m_numContextModels)
, m_cCUAbsSCModel             ( 1,             1,               NUM_ABS_FLAG_CTX              , m_contextModels + m_numContextModels, m_numContextModels)
, m_cMVPIdxSCModel            ( 1,             1,               NUM_MVP_IDX_CTX               , m_contextModels + m_numContextModels, m_numContextModels)
, m_cALFFlagSCModel           ( 1,             1,               NUM_ALF_FLAG_CTX              , m_contextModels + m_numContextModels, m_numContextModels)
, m_cALFUvlcSCModel           ( 1,             1,               NUM_ALF_UVLC_CTX              , m_contextModels + m_numContextModels, m_numContextModels)
, m_cALFSvlcSCModel           ( 1,             1,               NUM_ALF_SVLC_CTX              , m_contextModels + m_numContextModels, m_numContextModels)
, m_cCUAMPSCModel             ( 1,             1,               NUM_CU_AMP_CTX                , m_contextModels + m_numContextModels, m_numContextModels)
, m_cSaoFlagSCModel           ( 1,             1,               NUM_SAO_FLAG_CTX              , m_contextModels + m_numContextModels, m_numContextModels)
, m_cSaoUvlcSCModel           ( 1,             1,               NUM_SAO_UVLC_CTX              , m_contextModels + m_numContextModels, m_numContextModels)
, m_cSaoSvlcSCModel           ( 1,             1,               NUM_SAO_SVLC_CTX              , m_contextModels + m_numContextModels, m_numContextModels)
, m_cSaoMergeLeftSCModel      ( 1,             1,               NUM_SAO_MERGE_LEFT_FLAG_CTX   , m_contextModels + m_numContextModels, m_numContextModels)
, m_cSaoMergeUpSCModel        ( 1,             1,               NUM_SAO_MERGE_UP_FLAG_CTX     , m_contextModels + m_numContextModels, m_numContextModels)
, m_cSaoTypeIdxSCModel        ( 1,             1,               NUM_SAO_TYPE_IDX_CTX          , m_contextModels + m_numContextModels, m_numContextModels)
#if HHI_DMM_WEDGE_INTRA || HHI_DMM_PRED_TEX
, m_cDmmFlagSCModel           ( 1,             1,               NUM_DMM_FLAG_CTX              , m_contextModels + m_numContextModels, m_numContextModels)
, m_cDmmModeSCModel           ( 1,             1,               NUM_DMM_MODE_CTX              , m_contextModels + m_numContextModels, m_numContextModels)
, m_cDmmDataSCModel           ( 1,             1,               NUM_DMM_DATA_CTX              , m_contextModels + m_numContextModels, m_numContextModels)
#endif
#if LGE_EDGE_INTRA_A0070
, m_cEdgeIntraSCModel         ( 1,             1,               NUM_EDGE_INTRA_CTX            , m_contextModels + m_numContextModels, m_numContextModels)
#if LGE_EDGE_INTRA_DELTA_DC
, m_cEdgeIntraDeltaDCSCModel  ( 1,             1,               NUM_EDGE_INTRA_DELTA_DC_CTX   , m_contextModels + m_numContextModels, m_numContextModels)
#endif
#endif
#if RWTH_SDC_DLT_B0036
, m_cSDCFlagSCModel             ( 1,             1,                 SDC_NUM_FLAG_CTX           , m_contextModels + m_numContextModels, m_numContextModels)
, m_cSDCResidualFlagSCModel     ( 1,             2,  SDC_NUM_RESIDUAL_FLAG_CTX  , m_contextModels + m_numContextModels, m_numContextModels)
, m_cSDCResidualSignFlagSCModel ( 1,             2,  SDC_NUM_SIGN_FLAG_CTX      , m_contextModels + m_numContextModels, m_numContextModels)
, m_cSDCResidualSCModel         ( 1,             2,  SDC_NUM_RESIDUAL_CTX       , m_contextModels + m_numContextModels, m_numContextModels)
, m_cSDCPredModeSCModel             ( 1,             3,                 SDC_NUM_PRED_MODE_CTX     , m_contextModels + m_numContextModels, m_numContextModels)
#endif
{
  assert( m_numContextModels <= MAX_NUM_CTX_MOD );
  m_iSliceGranularity = 0;
}

TDecSbac::~TDecSbac()
{
}

// ====================================================================================================================
// Public member functions
// ====================================================================================================================

#if CABAC_INIT_FLAG
Void TDecSbac::resetEntropy(TComSlice* pSlice)
{
  SliceType sliceType  = pSlice->getSliceType();
  Int       qp         = pSlice->getSliceQp();

  if (pSlice->getPPS()->getCabacInitPresentFlag() && pSlice->getCabacInitFlag())
  {
    switch (sliceType)
    {
    case P_SLICE:           // change initialization table to B_SLICE initialization
      sliceType = B_SLICE; 
      break;
    case B_SLICE:           // change initialization table to P_SLICE initialization
      sliceType = P_SLICE; 
      break;
    default     :           // should not occur
      assert(0);
    }
  }

#else
Void TDecSbac::resetEntropywithQPandInitIDC (Int  qp, Int iID)
{
  SliceType sliceType = (SliceType)iID;
#endif  

  m_cCUSplitFlagSCModel.initBuffer       ( sliceType, qp, (UChar*)INIT_SPLIT_FLAG );
  m_cCUSkipFlagSCModel.initBuffer        ( sliceType, qp, (UChar*)INIT_SKIP_FLAG );
#if LGE_ILLUCOMP_B0045
  m_cCUICFlagSCModel.initBuffer          ( sliceType, qp, (UChar*)INIT_IC_FLAG );
#endif
  m_cCUMergeFlagExtSCModel.initBuffer    ( sliceType, qp, (UChar*)INIT_MERGE_FLAG_EXT );
  m_cCUMergeIdxExtSCModel.initBuffer     ( sliceType, qp, (UChar*)INIT_MERGE_IDX_EXT );
#if H3D_IVRP
  m_cResPredFlagSCModel.initBuffer       ( sliceType, qp, (UChar*)INIT_RES_PRED_FLAG );
#endif
#if QC_ARP_D0177
  m_cCUPUARPW.initBuffer                ( sliceType, qp, (UChar*)INIT_ARPW );
#endif
  m_cCUAlfCtrlFlagSCModel.initBuffer     ( sliceType, qp, (UChar*)INIT_ALF_CTRL_FLAG );
  m_cCUPartSizeSCModel.initBuffer        ( sliceType, qp, (UChar*)INIT_PART_SIZE );
  m_cCUAMPSCModel.initBuffer             ( sliceType, qp, (UChar*)INIT_CU_AMP_POS );
  m_cCUPredModeSCModel.initBuffer        ( sliceType, qp, (UChar*)INIT_PRED_MODE );
  m_cCUIntraPredSCModel.initBuffer       ( sliceType, qp, (UChar*)INIT_INTRA_PRED_MODE );
  m_cCUChromaPredSCModel.initBuffer      ( sliceType, qp, (UChar*)INIT_CHROMA_PRED_MODE );
  m_cCUInterDirSCModel.initBuffer        ( sliceType, qp, (UChar*)INIT_INTER_DIR );
  m_cCUMvdSCModel.initBuffer             ( sliceType, qp, (UChar*)INIT_MVD );
  m_cCURefPicSCModel.initBuffer          ( sliceType, qp, (UChar*)INIT_REF_PIC );
  m_cCUDeltaQpSCModel.initBuffer         ( sliceType, qp, (UChar*)INIT_DQP );
  m_cCUQtCbfSCModel.initBuffer           ( sliceType, qp, (UChar*)INIT_QT_CBF );
  m_cCUQtRootCbfSCModel.initBuffer       ( sliceType, qp, (UChar*)INIT_QT_ROOT_CBF );
  m_cCUSigCoeffGroupSCModel.initBuffer   ( sliceType, qp, (UChar*)INIT_SIG_CG_FLAG );
  m_cCUSigSCModel.initBuffer             ( sliceType, qp, (UChar*)INIT_SIG_FLAG );
  m_cCuCtxLastX.initBuffer               ( sliceType, qp, (UChar*)INIT_LAST );
  m_cCuCtxLastY.initBuffer               ( sliceType, qp, (UChar*)INIT_LAST );
  m_cCUOneSCModel.initBuffer             ( sliceType, qp, (UChar*)INIT_ONE_FLAG );
  m_cCUAbsSCModel.initBuffer             ( sliceType, qp, (UChar*)INIT_ABS_FLAG );
  m_cMVPIdxSCModel.initBuffer            ( sliceType, qp, (UChar*)INIT_MVP_IDX );
  m_cALFFlagSCModel.initBuffer           ( sliceType, qp, (UChar*)INIT_ALF_FLAG );
  m_cALFUvlcSCModel.initBuffer           ( sliceType, qp, (UChar*)INIT_ALF_UVLC );
  m_cALFSvlcSCModel.initBuffer           ( sliceType, qp, (UChar*)INIT_ALF_SVLC );
  m_cSaoFlagSCModel.initBuffer           ( sliceType, qp, (UChar*)INIT_SAO_FLAG );
  m_cSaoUvlcSCModel.initBuffer           ( sliceType, qp, (UChar*)INIT_SAO_UVLC );
  m_cSaoSvlcSCModel.initBuffer           ( sliceType, qp, (UChar*)INIT_SAO_SVLC );
  m_cSaoMergeLeftSCModel.initBuffer      ( sliceType, qp, (UChar*)INIT_SAO_MERGE_LEFT_FLAG );
  m_cSaoMergeUpSCModel.initBuffer        ( sliceType, qp, (UChar*)INIT_SAO_MERGE_UP_FLAG );
  m_cSaoTypeIdxSCModel.initBuffer        ( sliceType, qp, (UChar*)INIT_SAO_TYPE_IDX );

  m_cCUTransSubdivFlagSCModel.initBuffer ( sliceType, qp, (UChar*)INIT_TRANS_SUBDIV_FLAG );
#if LGE_EDGE_INTRA_A0070
  m_cEdgeIntraSCModel.initBuffer         ( sliceType, qp, (UChar*)INIT_EDGE_INTRA );
#if LGE_EDGE_INTRA_DELTA_DC
  m_cEdgeIntraDeltaDCSCModel.initBuffer  ( sliceType, qp, (UChar*)INIT_EDGE_INTRA_DELTA_DC );
#endif
#endif
  m_uiLastDQpNonZero  = 0;
#if HHI_DMM_WEDGE_INTRA || HHI_DMM_PRED_TEX
  m_cDmmFlagSCModel.initBuffer           ( sliceType, qp, (UChar*)INIT_DMM_FLAG );
  m_cDmmModeSCModel.initBuffer           ( sliceType, qp, (UChar*)INIT_DMM_MODE );
  m_cDmmDataSCModel.initBuffer           ( sliceType, qp, (UChar*)INIT_DMM_DATA );
#endif
#if RWTH_SDC_DLT_B0036
  m_cSDCFlagSCModel.initBuffer              ( sliceType, qp, (UChar*)INIT_SDC_FLAG );
  m_cSDCResidualFlagSCModel.initBuffer      ( sliceType, qp, (UChar*)INIT_SDC_RESIDUAL_FLAG );
  m_cSDCResidualSCModel.initBuffer          ( sliceType, qp, (UChar*)INIT_SDC_RESIDUAL );
  m_cSDCResidualSignFlagSCModel.initBuffer  ( sliceType, qp, (UChar*)INIT_SDC_SIGN_FLAG );
  m_cSDCPredModeSCModel.initBuffer              ( sliceType, qp, (UChar*)INIT_SDC_PRED_MODE );
#endif
  
  // new structure
  m_uiLastQp          = qp;
  
  m_pcTDecBinIf->start();
}

/** The function does the following: Read out terminate bit. Flush CABAC. Byte-align for next tile.
 *  Intialize CABAC states. Start CABAC.
 */
Void TDecSbac::updateContextTables( SliceType eSliceType, Int iQp )
{
  UInt uiBit;
  m_pcTDecBinIf->decodeBinTrm(uiBit);
  m_pcTDecBinIf->finish();  
#if !OL_FLUSH_ALIGN
  // Account for misaligned CABAC.
  Int iCABACReadAhead = m_pcTDecBinIf->getBitsReadAhead();
  iCABACReadAhead--;
  Int iStreamBits = 8-m_pcBitstream->getNumBitsUntilByteAligned();
  if (iCABACReadAhead >= iStreamBits)
  {
    // Misaligned CABAC has read into the 1st byte of the next tile.
    // Back up a byte prior to alignment.
    m_pcBitstream->backupByte();
  }
#endif
  m_pcBitstream->readOutTrailingBits();
  m_cCUSplitFlagSCModel.initBuffer       ( eSliceType, iQp, (UChar*)INIT_SPLIT_FLAG );
  m_cCUSkipFlagSCModel.initBuffer        ( eSliceType, iQp, (UChar*)INIT_SKIP_FLAG );
#if LGE_ILLUCOMP_B0045
  m_cCUICFlagSCModel.initBuffer          ( eSliceType, iQp, (UChar*)INIT_IC_FLAG );
#endif
  m_cCUMergeFlagExtSCModel.initBuffer    ( eSliceType, iQp, (UChar*)INIT_MERGE_FLAG_EXT );
  m_cCUMergeIdxExtSCModel.initBuffer     ( eSliceType, iQp, (UChar*)INIT_MERGE_IDX_EXT );
#if H3D_IVRP
  m_cResPredFlagSCModel.initBuffer       ( eSliceType, iQp, (UChar*)INIT_RES_PRED_FLAG );
#endif
#if QC_ARP_D0177
  m_cCUPUARPW.initBuffer                 ( eSliceType, iQp, (UChar*)INIT_ARPW );
#endif
  m_cCUAlfCtrlFlagSCModel.initBuffer     ( eSliceType, iQp, (UChar*)INIT_ALF_CTRL_FLAG );
  m_cCUPartSizeSCModel.initBuffer        ( eSliceType, iQp, (UChar*)INIT_PART_SIZE );
  m_cCUAMPSCModel.initBuffer             ( eSliceType, iQp, (UChar*)INIT_CU_AMP_POS );
  m_cCUPredModeSCModel.initBuffer        ( eSliceType, iQp, (UChar*)INIT_PRED_MODE );
  m_cCUIntraPredSCModel.initBuffer       ( eSliceType, iQp, (UChar*)INIT_INTRA_PRED_MODE );
  m_cCUChromaPredSCModel.initBuffer      ( eSliceType, iQp, (UChar*)INIT_CHROMA_PRED_MODE );
  m_cCUInterDirSCModel.initBuffer        ( eSliceType, iQp, (UChar*)INIT_INTER_DIR );
  m_cCUMvdSCModel.initBuffer             ( eSliceType, iQp, (UChar*)INIT_MVD );
  m_cCURefPicSCModel.initBuffer          ( eSliceType, iQp, (UChar*)INIT_REF_PIC );
  m_cCUDeltaQpSCModel.initBuffer         ( eSliceType, iQp, (UChar*)INIT_DQP );
  m_cCUQtCbfSCModel.initBuffer           ( eSliceType, iQp, (UChar*)INIT_QT_CBF );
  m_cCUQtRootCbfSCModel.initBuffer       ( eSliceType, iQp, (UChar*)INIT_QT_ROOT_CBF );
  m_cCUSigCoeffGroupSCModel.initBuffer   ( eSliceType, iQp, (UChar*)INIT_SIG_CG_FLAG );
  m_cCUSigSCModel.initBuffer             ( eSliceType, iQp, (UChar*)INIT_SIG_FLAG );
  m_cCuCtxLastX.initBuffer               ( eSliceType, iQp, (UChar*)INIT_LAST );
  m_cCuCtxLastY.initBuffer               ( eSliceType, iQp, (UChar*)INIT_LAST );
  m_cCUOneSCModel.initBuffer             ( eSliceType, iQp, (UChar*)INIT_ONE_FLAG );
  m_cCUAbsSCModel.initBuffer             ( eSliceType, iQp, (UChar*)INIT_ABS_FLAG );
  m_cMVPIdxSCModel.initBuffer            ( eSliceType, iQp, (UChar*)INIT_MVP_IDX );
  m_cALFFlagSCModel.initBuffer           ( eSliceType, iQp, (UChar*)INIT_ALF_FLAG );
  m_cALFUvlcSCModel.initBuffer           ( eSliceType, iQp, (UChar*)INIT_ALF_UVLC );
  m_cALFSvlcSCModel.initBuffer           ( eSliceType, iQp, (UChar*)INIT_ALF_SVLC );
  m_cSaoFlagSCModel.initBuffer           ( eSliceType, iQp, (UChar*)INIT_SAO_FLAG );
  m_cSaoUvlcSCModel.initBuffer           ( eSliceType, iQp, (UChar*)INIT_SAO_UVLC );
  m_cSaoSvlcSCModel.initBuffer           ( eSliceType, iQp, (UChar*)INIT_SAO_SVLC );
  m_cSaoMergeLeftSCModel.initBuffer      ( eSliceType, iQp, (UChar*)INIT_SAO_MERGE_LEFT_FLAG );
  m_cSaoMergeUpSCModel.initBuffer        ( eSliceType, iQp, (UChar*)INIT_SAO_MERGE_UP_FLAG );
  m_cSaoTypeIdxSCModel.initBuffer        ( eSliceType, iQp, (UChar*)INIT_SAO_TYPE_IDX );
  m_cCUTransSubdivFlagSCModel.initBuffer ( eSliceType, iQp, (UChar*)INIT_TRANS_SUBDIV_FLAG );
#if HHI_DMM_WEDGE_INTRA || HHI_DMM_PRED_TEX
  m_cDmmFlagSCModel.initBuffer           ( eSliceType, iQp, (UChar*)INIT_DMM_FLAG );
  m_cDmmModeSCModel.initBuffer           ( eSliceType, iQp, (UChar*)INIT_DMM_MODE );
  m_cDmmDataSCModel.initBuffer           ( eSliceType, iQp, (UChar*)INIT_DMM_DATA );
#endif
#if RWTH_SDC_DLT_B0036
  m_cSDCFlagSCModel.initBuffer              ( eSliceType, iQp, (UChar*)INIT_SDC_FLAG );
  m_cSDCResidualFlagSCModel.initBuffer      ( eSliceType, iQp, (UChar*)INIT_SDC_RESIDUAL_FLAG );
  m_cSDCResidualSCModel.initBuffer          ( eSliceType, iQp, (UChar*)INIT_SDC_RESIDUAL );
  m_cSDCResidualSignFlagSCModel.initBuffer  ( eSliceType, iQp, (UChar*)INIT_SDC_SIGN_FLAG );
  m_cSDCPredModeSCModel.initBuffer              ( eSliceType, iQp, (UChar*)INIT_SDC_PRED_MODE );
#endif

  m_pcTDecBinIf->start();
}

Void TDecSbac::readTileMarker( UInt& uiTileIdx, UInt uiBitsUsed )
{
  UInt uiSymbol;
  uiTileIdx = 0;
  for (Int iShift=uiBitsUsed-1; iShift>=0; iShift--)
  {
    m_pcTDecBinIf->decodeBinEP ( uiSymbol );
    if (uiSymbol)
    {
      uiTileIdx |= (1<<iShift);
    }
  }
}

Void TDecSbac::parseTerminatingBit( UInt& ruiBit )
{
  m_pcTDecBinIf->decodeBinTrm( ruiBit );
}


Void TDecSbac::xReadUnaryMaxSymbol( UInt& ruiSymbol, ContextModel* pcSCModel, Int iOffset, UInt uiMaxSymbol )
{
  if (uiMaxSymbol == 0)
  {
    ruiSymbol = 0;
    return;
  }
  
  m_pcTDecBinIf->decodeBin( ruiSymbol, pcSCModel[0] );
  
  if( ruiSymbol == 0 || uiMaxSymbol == 1 )
  {
    return;
  }
  
  UInt uiSymbol = 0;
  UInt uiCont;
  
  do
  {
    m_pcTDecBinIf->decodeBin( uiCont, pcSCModel[ iOffset ] );
    uiSymbol++;
  }
  while( uiCont && ( uiSymbol < uiMaxSymbol - 1 ) );
  
  if( uiCont && ( uiSymbol == uiMaxSymbol - 1 ) )
  {
    uiSymbol++;
  }
  
  ruiSymbol = uiSymbol;
}

Void TDecSbac::xReadEpExGolomb( UInt& ruiSymbol, UInt uiCount )
{
  UInt uiSymbol = 0;
  UInt uiBit = 1;
  
  while( uiBit )
  {
    m_pcTDecBinIf->decodeBinEP( uiBit );
    uiSymbol += uiBit << uiCount++;
  }
  
  if ( --uiCount )
  {
    UInt bins;
    m_pcTDecBinIf->decodeBinsEP( bins, uiCount );
    uiSymbol += bins;
  }
  
  ruiSymbol = uiSymbol;
}

Void TDecSbac::xReadUnarySymbol( UInt& ruiSymbol, ContextModel* pcSCModel, Int iOffset )
{
  m_pcTDecBinIf->decodeBin( ruiSymbol, pcSCModel[0] );
  
  if( !ruiSymbol )
  {
    return;
  }
  
  UInt uiSymbol = 0;
  UInt uiCont;
  
  do
  {
    m_pcTDecBinIf->decodeBin( uiCont, pcSCModel[ iOffset ] );
    uiSymbol++;
  }
  while( uiCont );
  
  ruiSymbol = uiSymbol;
}

/** Parsing of coeff_abs_level_minus3
 * \param ruiSymbol reference to coeff_abs_level_minus3
 * \param ruiGoRiceParam reference to Rice parameter
 * \returns Void
 */
Void TDecSbac::xReadGoRiceExGolomb( UInt &ruiSymbol, UInt &ruiGoRiceParam )
{
  Bool bExGolomb    = false;
  UInt uiCodeWord   = 0;
  UInt uiQuotient   = 0;
  UInt uiRemainder  = 0;
  UInt uiMaxVlc     = g_auiGoRiceRange[ ruiGoRiceParam ];
  UInt uiMaxPreLen  = g_auiGoRicePrefixLen[ ruiGoRiceParam ];

  do
  {
    uiQuotient++;
    m_pcTDecBinIf->decodeBinEP( uiCodeWord );
  }
  while( uiCodeWord && uiQuotient < uiMaxPreLen );

  uiCodeWord  = 1 - uiCodeWord;
  uiQuotient -= uiCodeWord;

  if ( ruiGoRiceParam > 0 )
  {
    m_pcTDecBinIf->decodeBinsEP( uiRemainder, ruiGoRiceParam );    
  }

  ruiSymbol      = uiRemainder + ( uiQuotient << ruiGoRiceParam );
  bExGolomb      = ruiSymbol == ( uiMaxVlc + 1 );

  if( bExGolomb )
  {
    xReadEpExGolomb( uiCodeWord, 0 );
    ruiSymbol += uiCodeWord;
  }

  ruiGoRiceParam = g_aauiGoRiceUpdate[ ruiGoRiceParam ][ min<UInt>( ruiSymbol, 23 ) ];

  return;
}


/** Parse I_PCM information. 
 * \param pcCU
 * \param uiAbsPartIdx 
 * \param uiDepth
 * \returns Void
 *
 * If I_PCM flag indicates that the CU is I_PCM, parse its PCM alignment bits and codes. 
 */
Void TDecSbac::parseIPCMInfo ( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth )
{
  UInt uiSymbol;
  Int numSubseqIPCM = 0;
  Bool readPCMSampleFlag = false;

  if(pcCU->getNumSucIPCM() > 0) 
  {
    readPCMSampleFlag = true;
  }
  else
  {
    m_pcTDecBinIf->decodeBinTrm(uiSymbol);

    if (uiSymbol)
    {
      readPCMSampleFlag = true;
      m_pcTDecBinIf->decodeNumSubseqIPCM(numSubseqIPCM);
      pcCU->setNumSucIPCM(numSubseqIPCM + 1);
      m_pcTDecBinIf->decodePCMAlignBits();
    }
  }

  if (readPCMSampleFlag == true)
  {
    Bool bIpcmFlag = true;


    pcCU->setPartSizeSubParts  ( SIZE_2Nx2N, uiAbsPartIdx, uiDepth );
    pcCU->setSizeSubParts      ( g_uiMaxCUWidth>>uiDepth, g_uiMaxCUHeight>>uiDepth, uiAbsPartIdx, uiDepth );
    pcCU->setIPCMFlagSubParts  ( bIpcmFlag, uiAbsPartIdx, uiDepth );

    UInt uiMinCoeffSize = pcCU->getPic()->getMinCUWidth()*pcCU->getPic()->getMinCUHeight();
    UInt uiLumaOffset   = uiMinCoeffSize*uiAbsPartIdx;
    UInt uiChromaOffset = uiLumaOffset>>2;

    Pel* piPCMSample;
    UInt uiWidth;
    UInt uiHeight;
    UInt uiSampleBits;
    UInt uiX, uiY;

    piPCMSample = pcCU->getPCMSampleY() + uiLumaOffset;
    uiWidth = pcCU->getWidth(uiAbsPartIdx);
    uiHeight = pcCU->getHeight(uiAbsPartIdx);
    uiSampleBits = pcCU->getSlice()->getSPS()->getPCMBitDepthLuma();

    for(uiY = 0; uiY < uiHeight; uiY++)
    {
      for(uiX = 0; uiX < uiWidth; uiX++)
      {
        UInt uiSample;
        m_pcTDecBinIf->xReadPCMCode(uiSampleBits, uiSample);
        piPCMSample[uiX] = uiSample;
      }
      piPCMSample += uiWidth;
    }

    piPCMSample = pcCU->getPCMSampleCb() + uiChromaOffset;
    uiWidth = pcCU->getWidth(uiAbsPartIdx)/2;
    uiHeight = pcCU->getHeight(uiAbsPartIdx)/2;
    uiSampleBits = pcCU->getSlice()->getSPS()->getPCMBitDepthChroma();

    for(uiY = 0; uiY < uiHeight; uiY++)
    {
      for(uiX = 0; uiX < uiWidth; uiX++)
      {
        UInt uiSample;
        m_pcTDecBinIf->xReadPCMCode(uiSampleBits, uiSample);
        piPCMSample[uiX] = uiSample;
      }
      piPCMSample += uiWidth;
    }

    piPCMSample = pcCU->getPCMSampleCr() + uiChromaOffset;
    uiWidth = pcCU->getWidth(uiAbsPartIdx)/2;
    uiHeight = pcCU->getHeight(uiAbsPartIdx)/2;
    uiSampleBits = pcCU->getSlice()->getSPS()->getPCMBitDepthChroma();

    for(uiY = 0; uiY < uiHeight; uiY++)
    {
      for(uiX = 0; uiX < uiWidth; uiX++)
      {
        UInt uiSample;
        m_pcTDecBinIf->xReadPCMCode(uiSampleBits, uiSample);
        piPCMSample[uiX] = uiSample;
      }
      piPCMSample += uiWidth;
    }

    pcCU->setNumSucIPCM( pcCU->getNumSucIPCM() - 1);
    if(pcCU->getNumSucIPCM() == 0)
    {
      m_pcTDecBinIf->resetBac();
    }
  }
}

/** parse skip flag
 * \param pcCU
 * \param uiAbsPartIdx 
 * \param uiDepth
 * \returns Void
 */
Void TDecSbac::parseSkipFlag( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth )
{
  if( pcCU->getSlice()->isIntra() )
  {
    return;
  }
  
  UInt uiSymbol = 0;
  UInt uiCtxSkip = pcCU->getCtxSkipFlag( uiAbsPartIdx );
  m_pcTDecBinIf->decodeBin( uiSymbol, m_cCUSkipFlagSCModel.get( 0, 0, uiCtxSkip ) );
  DTRACE_CABAC_VL( g_nSymbolCounter++ );
  DTRACE_CABAC_T( "\tSkipFlag" );
  DTRACE_CABAC_T( "\tuiCtxSkip: ");
  DTRACE_CABAC_V( uiCtxSkip );
  DTRACE_CABAC_T( "\tuiSymbol: ");
  DTRACE_CABAC_V( uiSymbol );
  DTRACE_CABAC_T( "\n");
  
  if( uiSymbol )
  {
    pcCU->setPredModeSubParts( MODE_SKIP,  uiAbsPartIdx, uiDepth );
    pcCU->setPartSizeSubParts( SIZE_2Nx2N, uiAbsPartIdx, uiDepth );
    pcCU->setSizeSubParts( g_uiMaxCUWidth>>uiDepth, g_uiMaxCUHeight>>uiDepth, uiAbsPartIdx, uiDepth );
    pcCU->setMergeFlagSubParts( true , uiAbsPartIdx, 0, uiDepth );
  }
}

#if LGE_ILLUCOMP_B0045
/** parse illumination compensation flag
 * \param pcCU
 * \param uiAbsPartIdx 
 * \param uiDepth
 * \returns Void
 */
Void TDecSbac::parseICFlag( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth )
{ 
  UInt uiSymbol = 0;
  UInt uiCtxIC = pcCU->getCtxICFlag( uiAbsPartIdx );
  m_pcTDecBinIf->decodeBin( uiSymbol, m_cCUICFlagSCModel.get( 0, 0, uiCtxIC ) );
  DTRACE_CABAC_VL( g_nSymbolCounter++ );
  DTRACE_CABAC_T( "\tICFlag" );
  DTRACE_CABAC_T( "\tuiCtxIC: ");
  DTRACE_CABAC_V( uiCtxIC );
  DTRACE_CABAC_T( "\tuiSymbol: ");
  DTRACE_CABAC_V( uiSymbol );
  DTRACE_CABAC_T( "\n");
  
  pcCU->setICFlagSubParts( uiSymbol ? true : false , uiAbsPartIdx, 0, uiDepth );
}
#endif


/** parse merge flag
 * \param pcCU
 * \param uiAbsPartIdx 
 * \param uiDepth
 * \param uiPUIdx
 * \returns Void
 */
Void TDecSbac::parseMergeFlag ( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth, UInt uiPUIdx )
{
  UInt uiSymbol;
  m_pcTDecBinIf->decodeBin( uiSymbol, *m_cCUMergeFlagExtSCModel.get( 0 ) );
  pcCU->setMergeFlagSubParts( uiSymbol ? true : false, uiAbsPartIdx, uiPUIdx, uiDepth );

  DTRACE_CABAC_VL( g_nSymbolCounter++ );
  DTRACE_CABAC_T( "\tMergeFlag: " );
  DTRACE_CABAC_V( uiSymbol );
  DTRACE_CABAC_T( "\tAddress: " );
  DTRACE_CABAC_V( pcCU->getAddr() );
  DTRACE_CABAC_T( "\tuiAbsPartIdx: " );
  DTRACE_CABAC_V( uiAbsPartIdx );
  DTRACE_CABAC_T( "\n" );
}

Void TDecSbac::parseMergeIndex ( TComDataCU* pcCU, UInt& ruiMergeIndex, UInt uiAbsPartIdx, UInt uiDepth )
{
  UInt uiNumCand = MRG_MAX_NUM_CANDS;
  UInt uiUnaryIdx = 0;
  uiNumCand = pcCU->getSlice()->getMaxNumMergeCand();
#if HHI_MPI
  const Bool bMVIAvailable = pcCU->getSlice()->getSPS()->getUseMVI() && pcCU->getSlice()->getSliceType() != I_SLICE;
  const UInt uiMviMergePos = bMVIAvailable ? HHI_MPI_MERGE_POS : uiNumCand;
#endif
  if ( uiNumCand > 1 )
  {
    for( ; uiUnaryIdx < uiNumCand - 1; ++uiUnaryIdx )
    {
      UInt uiSymbol = 0;
      if ( uiUnaryIdx==0 )
      {
        m_pcTDecBinIf->decodeBin( uiSymbol, m_cCUMergeIdxExtSCModel.get( 0, 0, 0 ) );
      }
      else
      {
        m_pcTDecBinIf->decodeBinEP( uiSymbol );
      }
      if( uiSymbol == 0 )
      {
        break;
      }
    }
  }
  ruiMergeIndex = uiUnaryIdx;

  DTRACE_CABAC_VL( g_nSymbolCounter++ )
  DTRACE_CABAC_T( "\tparseMergeIndex()" )
  DTRACE_CABAC_T( "\tuiMRGIdx= " )
  DTRACE_CABAC_V( ruiMergeIndex )
  DTRACE_CABAC_T( "\n" )
#if HHI_MPI
  if( ruiMergeIndex > uiMviMergePos )
  {
    assert( bMVIAvailable );
    ruiMergeIndex--;
  }
  else if( ruiMergeIndex == uiMviMergePos )
  {
    assert( bMVIAvailable );
    pcCU->setTextureModeDepthSubParts( uiDepth, uiAbsPartIdx, uiDepth );
  }
#endif
}

#if H3D_IVRP
Void
TDecSbac::parseResPredFlag( TComDataCU* pcCU, Bool& rbResPredFlag, UInt uiAbsPartIdx, UInt uiDepth )
{
  UInt uiCtx    = pcCU->getCtxResPredFlag( uiAbsPartIdx );
  UInt uiSymbol = 0;
  m_pcTDecBinIf->decodeBin( uiSymbol, m_cResPredFlagSCModel.get( 0, 0, uiCtx ) );
  rbResPredFlag = ( uiSymbol != 0 );
}
#endif

#if H3D_IVMP
Void TDecSbac::parseMVPIdx      ( Int& riMVPIdx, Int iNumAMVPCands )
{
  UInt uiSymbol;
  xReadUnaryMaxSymbol(uiSymbol, m_cMVPIdxSCModel.get(0), 1, iNumAMVPCands-1);
  riMVPIdx = uiSymbol;
}
#else
Void TDecSbac::parseMVPIdx      ( Int& riMVPIdx )
{
  UInt uiSymbol;
  xReadUnaryMaxSymbol(uiSymbol, m_cMVPIdxSCModel.get(0), 1, AMVP_MAX_NUM_CANDS-1);
  riMVPIdx = uiSymbol;
}
#endif

Void TDecSbac::parseSplitFlag     ( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth )
{
  if( uiDepth == g_uiMaxCUDepth - g_uiAddCUDepth )
  {
    pcCU->setDepthSubParts( uiDepth, uiAbsPartIdx );
    return;
  }
  
  UInt uiSymbol;

#if H3D_QTL
  Bool bParseSplitFlag    = true;

  TComSPS *sps            = pcCU->getPic()->getSlice(0)->getSPS();
  TComPic *pcTexture      = pcCU->getSlice()->getTexturePic();
  Bool bDepthMapDetect    = (pcTexture != NULL);
  Bool bIntraSliceDetect  = (pcCU->getSlice()->getSliceType() == I_SLICE);

  Bool rapPic     = (pcCU->getSlice()->getNalUnitType() == NAL_UNIT_CODED_SLICE_IDR || pcCU->getSlice()->getNalUnitType() == NAL_UNIT_CODED_SLICE_CRA);
  if(bDepthMapDetect && !bIntraSliceDetect && !rapPic && sps->getUseQTLPC())
  {
    TComDataCU *pcTextureCU = pcTexture->getCU(pcCU->getAddr());
    assert(pcTextureCU->getDepth(uiAbsPartIdx) >= uiDepth);
    bParseSplitFlag         = (pcTextureCU->getDepth(uiAbsPartIdx) > uiDepth);
  }

  if(bParseSplitFlag)
  {
#endif
    m_pcTDecBinIf->decodeBin( uiSymbol, m_cCUSplitFlagSCModel.get( 0, 0, pcCU->getCtxSplitFlag( uiAbsPartIdx, uiDepth ) ) );
    DTRACE_CABAC_VL( g_nSymbolCounter++ )
    DTRACE_CABAC_T( "\tSplitFlag\n" )
#if H3D_QTL
  }
  else
    uiSymbol = 0;
#endif

  pcCU->setDepthSubParts( uiDepth + uiSymbol, uiAbsPartIdx );
  
  return;
}

/** parse partition size
 * \param pcCU
 * \param uiAbsPartIdx 
 * \param uiDepth
 * \returns Void
 */
Void TDecSbac::parsePartSize( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth )
{
  UInt uiSymbol, uiMode = 0;
  PartSize eMode;

#if H3D_QTL
  Bool bParsePartSize    = true;
  TComSPS *sps           = pcCU->getPic()->getSlice(0)->getSPS();
  TComPic *pcTexture     = pcCU->getSlice()->getTexturePic();
  Bool bDepthMapDetect   = (pcTexture != NULL);
  Bool bIntraSliceDetect = (pcCU->getSlice()->getSliceType() == I_SLICE);

  Bool rapPic     = (pcCU->getSlice()->getNalUnitType() == NAL_UNIT_CODED_SLICE_IDR || pcCU->getSlice()->getNalUnitType() == NAL_UNIT_CODED_SLICE_CRA);
  if(bDepthMapDetect && !bIntraSliceDetect && !rapPic && sps->getUseQTLPC())
  {
    TComDataCU *pcTextureCU = pcTexture->getCU(pcCU->getAddr());
    assert(pcTextureCU->getDepth(uiAbsPartIdx) >= uiDepth);
    if (pcTextureCU->getDepth(uiAbsPartIdx) == uiDepth && pcTextureCU->getPartitionSize( uiAbsPartIdx ) != SIZE_NxN)
    {
      bParsePartSize = false;
      eMode          = SIZE_2Nx2N;
    }
  }
#endif
  
  if ( pcCU->isIntra( uiAbsPartIdx ) )
  {
#if H3D_QTL
    if(bParsePartSize)
    {
#endif
      uiSymbol = 1;
      if( uiDepth == g_uiMaxCUDepth - g_uiAddCUDepth )
      {
        m_pcTDecBinIf->decodeBin( uiSymbol, m_cCUPartSizeSCModel.get( 0, 0, 0) );
      }
      eMode = uiSymbol ? SIZE_2Nx2N : SIZE_NxN;
#if H3D_QTL
    }
#endif
    UInt uiTrLevel = 0;    
    UInt uiWidthInBit  = g_aucConvertToBit[pcCU->getWidth(uiAbsPartIdx)]+2;
    UInt uiTrSizeInBit = g_aucConvertToBit[pcCU->getSlice()->getSPS()->getMaxTrSize()]+2;
    uiTrLevel          = uiWidthInBit >= uiTrSizeInBit ? uiWidthInBit - uiTrSizeInBit : 0;
    if( eMode == SIZE_NxN )
    {
      pcCU->setTrIdxSubParts( 1+uiTrLevel, uiAbsPartIdx, uiDepth );
    }
    else
    {
      pcCU->setTrIdxSubParts( uiTrLevel, uiAbsPartIdx, uiDepth );
    }
  }
  else
  {
#if H3D_QTL
    if(bParsePartSize)
    {
#endif
      UInt uiMaxNumBits = 2;
      if( uiDepth == g_uiMaxCUDepth - g_uiAddCUDepth && !( pcCU->getSlice()->getSPS()->getDisInter4x4() && (g_uiMaxCUWidth>>uiDepth) == 8 && (g_uiMaxCUHeight>>uiDepth) == 8 ) )
      {
        uiMaxNumBits ++;
      }
      for ( UInt ui = 0; ui < uiMaxNumBits; ui++ )
      {
        m_pcTDecBinIf->decodeBin( uiSymbol, m_cCUPartSizeSCModel.get( 0, 0, ui) );
        if ( uiSymbol )
        {
          break;
        }
        uiMode++;
      }
      eMode = (PartSize) uiMode;
      if ( pcCU->getSlice()->getSPS()->getAMPAcc( uiDepth ) )
      {
        if (eMode == SIZE_2NxN)
        {
            m_pcTDecBinIf->decodeBin(uiSymbol, m_cCUAMPSCModel.get( 0, 0, 0 ));
          if (uiSymbol == 0)
          {
            m_pcTDecBinIf->decodeBinEP(uiSymbol);
            eMode = (uiSymbol == 0? SIZE_2NxnU : SIZE_2NxnD);
          }
        }
        else if (eMode == SIZE_Nx2N)
        {
          m_pcTDecBinIf->decodeBin(uiSymbol, m_cCUAMPSCModel.get( 0, 0, 0 ));
          if (uiSymbol == 0)
          {
            m_pcTDecBinIf->decodeBinEP(uiSymbol);
            eMode = (uiSymbol == 0? SIZE_nLx2N : SIZE_nRx2N);
          }
        }
      }
#if H3D_QTL
    }
#endif
  }
  pcCU->setPartSizeSubParts( eMode, uiAbsPartIdx, uiDepth );
  pcCU->setSizeSubParts( g_uiMaxCUWidth>>uiDepth, g_uiMaxCUHeight>>uiDepth, uiAbsPartIdx, uiDepth );
}

/** parse prediction mode
 * \param pcCU
 * \param uiAbsPartIdx 
 * \param uiDepth
 * \returns Void
 */
Void TDecSbac::parsePredMode( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth )
{
  if( pcCU->getSlice()->isIntra() )
  {
    pcCU->setPredModeSubParts( MODE_INTRA, uiAbsPartIdx, uiDepth );
    return;
  }
  
  UInt uiSymbol;
  Int  iPredMode = MODE_INTER;
  m_pcTDecBinIf->decodeBin( uiSymbol, m_cCUPredModeSCModel.get( 0, 0, 0 ) );
  iPredMode += uiSymbol;
  pcCU->setPredModeSubParts( (PredMode)iPredMode, uiAbsPartIdx, uiDepth );
}
  
Void TDecSbac::parseIntraDirLumaAng  ( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth )
{
  UInt uiSymbol;
  Int  intraPredMode;

#if HHI_DMM_WEDGE_INTRA || HHI_DMM_PRED_TEX
  UInt uiFlag = 0;
  if( pcCU->getSlice()->getSPS()->getUseDMM() && (g_uiMaxCUWidth>>uiDepth) <= DMM_WEDGEMODEL_MAX_SIZE )
  {
    m_pcTDecBinIf->decodeBin( uiFlag, m_cDmmFlagSCModel.get(0, 0, 0) );
  }
  if( uiFlag )
  {
    UInt uiDMMode;

#if HHI_DMM_WEDGE_INTRA && HHI_DMM_PRED_TEX
    m_pcTDecBinIf->decodeBin( uiSymbol, m_cDmmModeSCModel.get(0, 0, 0) ); uiDMMode  = uiSymbol;
    m_pcTDecBinIf->decodeBin( uiSymbol, m_cDmmModeSCModel.get(0, 0, 0) ); uiDMMode |= uiSymbol << 1;
    if ( pcCU->getPartitionSize( uiAbsPartIdx ) != SIZE_NxN && g_uiMaxCUWidth>>uiDepth > 4 )
    {
      m_pcTDecBinIf->decodeBin( uiSymbol, m_cDmmModeSCModel.get(0, 0, 0) ); uiDMMode |= uiSymbol << 2;
    }
#else
    m_pcTDecBinIf->decodeBin( uiSymbol, m_cDmmModeSCModel.get(0, 0, 0) ); uiDMMode  = uiSymbol;
    if ( pcCU->getPartitionSize( uiAbsPartIdx ) != SIZE_NxN && g_uiMaxCUWidth>>uiDepth > 4 )
    {
      m_pcTDecBinIf->decodeBin( uiSymbol, m_cDmmModeSCModel.get(0, 0, 0) ); uiDMMode |= uiSymbol << 1;
    }
#endif
    intraPredMode = uiDMMode + NUM_INTRA_MODE;

#if HHI_DMM_WEDGE_INTRA
    if( intraPredMode == DMM_WEDGE_FULL_IDX )          { xParseWedgeFullInfo          ( pcCU, uiAbsPartIdx, uiDepth ); }
    if( intraPredMode == DMM_WEDGE_FULL_D_IDX )        { xParseWedgeFullDeltaInfo     ( pcCU, uiAbsPartIdx, uiDepth ); }
    if( intraPredMode == DMM_WEDGE_PREDDIR_IDX )       { xParseWedgePredDirInfo       ( pcCU, uiAbsPartIdx, uiDepth ); }
    if( intraPredMode == DMM_WEDGE_PREDDIR_D_IDX )     { xParseWedgePredDirDeltaInfo  ( pcCU, uiAbsPartIdx, uiDepth ); }
#endif
#if HHI_DMM_PRED_TEX
    if( intraPredMode == DMM_WEDGE_PREDTEX_D_IDX )     { xParseWedgePredTexDeltaInfo  ( pcCU, uiAbsPartIdx, uiDepth ); }
#if LGE_DMM3_SIMP_C0044
    if( intraPredMode == DMM_WEDGE_PREDTEX_IDX )       { xParseWedgePredTexInfo       ( pcCU, uiAbsPartIdx, uiDepth ); }
#endif
    if( intraPredMode == DMM_CONTOUR_PREDTEX_D_IDX )   { xParseContourPredTexDeltaInfo( pcCU, uiAbsPartIdx, uiDepth ); }
#endif
  }
  else
  {
#endif

#if LGE_EDGE_INTRA_A0070
    Bool bCodeEdgeIntra = false;
    if( pcCU->getSlice()->getSPS()->isDepth() )
    {
      UInt uiPUWidth = pcCU->getWidth( uiAbsPartIdx ) >> (pcCU->getPartitionSize( uiAbsPartIdx ) == SIZE_NxN ? 1 : 0);
      if( uiPUWidth <= LGE_EDGE_INTRA_MAX_SIZE && uiPUWidth >= LGE_EDGE_INTRA_MIN_SIZE )
        bCodeEdgeIntra = true;
    }
#endif

    Int uiPreds[3] = {-1, -1, -1};
    Int uiPredNum = pcCU->getIntraDirLumaPredictor(uiAbsPartIdx, uiPreds);  
#if LGE_EDGE_INTRA_A0070
    UInt uiCheckBit = 0;
#endif

    m_pcTDecBinIf->decodeBin( uiSymbol, m_cCUIntraPredSCModel.get( 0, 0, 0) );

    if ( uiSymbol )
    {
      m_pcTDecBinIf->decodeBinEP( uiSymbol );
      if (uiSymbol)
      {
        m_pcTDecBinIf->decodeBinEP( uiSymbol );
        uiSymbol++;
      }
      intraPredMode = uiPreds[uiSymbol];
    }
    else
    {
      intraPredMode = 0;


      m_pcTDecBinIf->decodeBinsEP( uiSymbol, 5 );
#if LGE_EDGE_INTRA_A0070
      if (bCodeEdgeIntra)
      {
        if (uiSymbol==31)
        {
          m_pcTDecBinIf->decodeBinsEP(uiCheckBit,1);
          if (uiCheckBit)
            uiSymbol = EDGE_INTRA_IDX;
        }
      }
#endif
      intraPredMode = uiSymbol;

      //postponed sorting of MPMs (only in remaining branch)
      if (uiPreds[0] > uiPreds[1])
      { 
        std::swap(uiPreds[0], uiPreds[1]); 
      }
      if (uiPreds[0] > uiPreds[2])
      {
        std::swap(uiPreds[0], uiPreds[2]);
      }
      if (uiPreds[1] > uiPreds[2])
      {
        std::swap(uiPreds[1], uiPreds[2]);
      }
#if LGE_EDGE_INTRA_A0070
      if ( intraPredMode != EDGE_INTRA_IDX)
      {
#endif
        for ( Int i = 0; i < uiPredNum; i++ )
        {
          intraPredMode += ( intraPredMode >= uiPreds[i] );
        }
#if LGE_EDGE_INTRA_A0070
      }
#endif
    }

#if LGE_EDGE_INTRA_A0070
    if( intraPredMode == EDGE_INTRA_IDX )
    {
      xParseEdgeIntraInfo( pcCU, uiAbsPartIdx, uiDepth );
#if LGE_EDGE_INTRA_DELTA_DC
      m_pcTDecBinIf->decodeBin( uiSymbol, m_cEdgeIntraDeltaDCSCModel.get(0, 0, 0) );
      if( uiSymbol )
      {
        intraPredMode = EDGE_INTRA_DELTA_IDX;
        Int iDeltaDC0;
        Int iDeltaDC1;

        xReadExGolombLevel( (UInt &) iDeltaDC0, m_cEdgeIntraDeltaDCSCModel.get(0, 0, 1) );
        if( iDeltaDC0 != 0 )
        {
          UInt uiSign;
          m_pcTDecBinIf->decodeBinEP( uiSign );
          if ( uiSign )
          {
            iDeltaDC0 = -iDeltaDC0;
          }
        }
        xReadExGolombLevel( (UInt &) iDeltaDC1, m_cEdgeIntraDeltaDCSCModel.get(0, 0, 1) );
        if( iDeltaDC1 != 0 )
        {
          UInt uiSign;
          m_pcTDecBinIf->decodeBinEP( uiSign );
          if ( uiSign )
          {
            iDeltaDC1 = -iDeltaDC1;
          }
        }

        pcCU->setEdgeDeltaDC0( uiAbsPartIdx, iDeltaDC0 );
        pcCU->setEdgeDeltaDC1( uiAbsPartIdx, iDeltaDC1 );
      }
#endif
    }
#endif

#if HHI_DMM_WEDGE_INTRA || HHI_DMM_PRED_TEX
  }
#endif

  pcCU->setLumaIntraDirSubParts( (UChar)intraPredMode, uiAbsPartIdx, uiDepth );
}

Void TDecSbac::parseIntraDirChroma( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth )
{
  UInt uiSymbol;

  m_pcTDecBinIf->decodeBin( uiSymbol, m_cCUChromaPredSCModel.get( 0, 0, 0 ) );

  if( uiSymbol == 0 )
  {
    uiSymbol = DM_CHROMA_IDX;
  } 
  else 
  {
    if( pcCU->getSlice()->getSPS()->getUseLMChroma() )
    {
      m_pcTDecBinIf->decodeBin( uiSymbol, m_cCUChromaPredSCModel.get( 0, 0, 1 ) );
    }
    else
    {
      uiSymbol = 1;
    }

    if( uiSymbol == 0 )
    {
      uiSymbol = LM_CHROMA_IDX;
    } 
    else
    {
      UInt uiIPredMode;
      m_pcTDecBinIf->decodeBinsEP( uiIPredMode, 2 );
      UInt uiAllowedChromaDir[ NUM_CHROMA_MODE ];
      pcCU->getAllowedChromaDir( uiAbsPartIdx, uiAllowedChromaDir );
      uiSymbol = uiAllowedChromaDir[ uiIPredMode ];
    }
  }
  pcCU->setChromIntraDirSubParts( uiSymbol, uiAbsPartIdx, uiDepth );
  return;
}

Void TDecSbac::parseInterDir( TComDataCU* pcCU, UInt& ruiInterDir, UInt uiAbsPartIdx, UInt uiDepth )
{
  UInt uiSymbol;
  const UInt uiCtx = pcCU->getCtxInterDir( uiAbsPartIdx );
  ContextModel *pCtx = m_cCUInterDirSCModel.get( 0 );
  m_pcTDecBinIf->decodeBin( uiSymbol, *( pCtx + uiCtx ) );

  if( uiSymbol )
  {
    uiSymbol = 2;
  }

  uiSymbol++;
  ruiInterDir = uiSymbol;
  return;
}

Void TDecSbac::parseRefFrmIdx( TComDataCU* pcCU, Int& riRefFrmIdx, UInt uiAbsPartIdx, UInt uiDepth, RefPicList eRefList )
{
  UInt uiSymbol;

  if(pcCU->getSlice()->getNumRefIdx(REF_PIC_LIST_C ) > 0 && eRefList==REF_PIC_LIST_C)
  {
    ContextModel *pCtx = m_cCURefPicSCModel.get( 0 );
    m_pcTDecBinIf->decodeBin( uiSymbol, *pCtx );

    if( uiSymbol )
    {
      xReadUnaryMaxSymbol( uiSymbol, pCtx + 1, 1, pcCU->getSlice()->getNumRefIdx( REF_PIC_LIST_C )-2 );
      uiSymbol++;
    }
    riRefFrmIdx = uiSymbol;
  }
  else
  {
    ContextModel *pCtx = m_cCURefPicSCModel.get( 0 );
    m_pcTDecBinIf->decodeBin( uiSymbol, *pCtx );

    if( uiSymbol )
    {
      xReadUnaryMaxSymbol( uiSymbol, pCtx + 1, 1, pcCU->getSlice()->getNumRefIdx( eRefList )-2 );
      uiSymbol++;
    }
    riRefFrmIdx = uiSymbol;
  }

  return;
}

Void TDecSbac::parseMvd( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiPartIdx, UInt uiDepth, RefPicList eRefList )
{
  UInt uiSymbol;
  UInt uiHorAbs;
  UInt uiVerAbs;
  UInt uiHorSign = 0;
  UInt uiVerSign = 0;
  ContextModel *pCtx = m_cCUMvdSCModel.get( 0 );

  if(pcCU->getSlice()->getMvdL1ZeroFlag() && eRefList == REF_PIC_LIST_1 && pcCU->getInterDir(uiAbsPartIdx)==3)
  {
    uiHorAbs=0;
    uiVerAbs=0;
  }
  else
  {

    m_pcTDecBinIf->decodeBin( uiHorAbs, *pCtx );
    m_pcTDecBinIf->decodeBin( uiVerAbs, *pCtx );

    const Bool bHorAbsGr0 = uiHorAbs != 0;
    const Bool bVerAbsGr0 = uiVerAbs != 0;
    pCtx++;

    if( bHorAbsGr0 )
    {
      m_pcTDecBinIf->decodeBin( uiSymbol, *pCtx );
      uiHorAbs += uiSymbol;
    }

    if( bVerAbsGr0 )
    {
      m_pcTDecBinIf->decodeBin( uiSymbol, *pCtx );
      uiVerAbs += uiSymbol;
    }

    if( bHorAbsGr0 )
    {
      if( 2 == uiHorAbs )
      {
        xReadEpExGolomb( uiSymbol, 1 );
        uiHorAbs += uiSymbol;
      }

      m_pcTDecBinIf->decodeBinEP( uiHorSign );
    }

    if( bVerAbsGr0 )
    {
      if( 2 == uiVerAbs )
      {
        xReadEpExGolomb( uiSymbol, 1 );
        uiVerAbs += uiSymbol;
      }

      m_pcTDecBinIf->decodeBinEP( uiVerSign );
    }

  }

  const TComMv cMv( uiHorSign ? -Int( uiHorAbs ): uiHorAbs, uiVerSign ? -Int( uiVerAbs ) : uiVerAbs );
  pcCU->getCUMvField( eRefList )->setAllMvd( cMv, pcCU->getPartitionSize( uiAbsPartIdx ), uiAbsPartIdx, uiDepth, uiPartIdx );
  return;
}


Void TDecSbac::parseTransformSubdivFlag( UInt& ruiSubdivFlag, UInt uiLog2TransformBlockSize )
{
  m_pcTDecBinIf->decodeBin( ruiSubdivFlag, m_cCUTransSubdivFlagSCModel.get( 0, 0, uiLog2TransformBlockSize ) );
  DTRACE_CABAC_VL( g_nSymbolCounter++ )
  DTRACE_CABAC_T( "\tparseTransformSubdivFlag()" )
  DTRACE_CABAC_T( "\tsymbol=" )
  DTRACE_CABAC_V( ruiSubdivFlag )
  DTRACE_CABAC_T( "\tctx=" )
  DTRACE_CABAC_V( uiLog2TransformBlockSize )
  DTRACE_CABAC_T( "\n" )
}

Void TDecSbac::parseQtRootCbf( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth, UInt& uiQtRootCbf )
{
  UInt uiSymbol;
  const UInt uiCtx = 0;
  m_pcTDecBinIf->decodeBin( uiSymbol , m_cCUQtRootCbfSCModel.get( 0, 0, uiCtx ) );
  DTRACE_CABAC_VL( g_nSymbolCounter++ )
  DTRACE_CABAC_T( "\tparseQtRootCbf()" )
  DTRACE_CABAC_T( "\tsymbol=" )
  DTRACE_CABAC_V( uiSymbol )
  DTRACE_CABAC_T( "\tctx=" )
  DTRACE_CABAC_V( uiCtx )
  DTRACE_CABAC_T( "\tuiAbsPartIdx=" )
  DTRACE_CABAC_V( uiAbsPartIdx )
  DTRACE_CABAC_T( "\n" )
  
  uiQtRootCbf = uiSymbol;
}

Void TDecSbac::parseDeltaQP( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth )
{
  Int qp;
  UInt uiDQp;
  Int  iDQp;
  
  m_pcTDecBinIf->decodeBin( uiDQp, m_cCUDeltaQpSCModel.get( 0, 0, 0 ) );
  
  if ( uiDQp == 0 )
  {
    qp = pcCU->getRefQP(uiAbsPartIdx);
  }
  else
  {
    UInt uiSign;
    Int qpBdOffsetY = pcCU->getSlice()->getSPS()->getQpBDOffsetY();
    m_pcTDecBinIf->decodeBinEP(uiSign);

    UInt uiMaxAbsDQpMinus1 = 24 + (qpBdOffsetY/2) + (uiSign);
    UInt uiAbsDQpMinus1;
    xReadUnaryMaxSymbol (uiAbsDQpMinus1,  &m_cCUDeltaQpSCModel.get( 0, 0, 1 ), 1, uiMaxAbsDQpMinus1);

    iDQp = uiAbsDQpMinus1 + 1;

    if(uiSign)
    {
      iDQp = -iDQp;
    }

    qp = (((Int) pcCU->getRefQP( uiAbsPartIdx ) + iDQp + 52 + 2*qpBdOffsetY )%(52+qpBdOffsetY)) - qpBdOffsetY;
  }
  
  UInt uiAbsQpCUPartIdx = (uiAbsPartIdx>>(8-(pcCU->getSlice()->getPPS()->getMaxCuDQPDepth()<<1)))<<(8-(pcCU->getSlice()->getPPS()->getMaxCuDQPDepth()<<1)) ;
  UInt uiQpCUDepth =   min(uiDepth,pcCU->getSlice()->getPPS()->getMaxCuDQPDepth()) ;
  pcCU->setQPSubParts( qp, uiAbsQpCUPartIdx, uiQpCUDepth );
}

Void TDecSbac::parseQtCbf( TComDataCU* pcCU, UInt uiAbsPartIdx, TextType eType, UInt uiTrDepth, UInt uiDepth )
{
  UInt uiSymbol;
  const UInt uiCtx = pcCU->getCtxQtCbf( uiAbsPartIdx, eType, uiTrDepth );
  m_pcTDecBinIf->decodeBin( uiSymbol , m_cCUQtCbfSCModel.get( 0, eType ? TEXT_CHROMA: eType, uiCtx ) );
  
  DTRACE_CABAC_VL( g_nSymbolCounter++ )
  DTRACE_CABAC_T( "\tparseQtCbf()" )
  DTRACE_CABAC_T( "\tsymbol=" )
  DTRACE_CABAC_V( uiSymbol )
  DTRACE_CABAC_T( "\tctx=" )
  DTRACE_CABAC_V( uiCtx )
  DTRACE_CABAC_T( "\tetype=" )
  DTRACE_CABAC_V( eType )
  DTRACE_CABAC_T( "\tuiAbsPartIdx=" )
  DTRACE_CABAC_V( uiAbsPartIdx )
  DTRACE_CABAC_T( "\n" )
  
  pcCU->setCbfSubParts( uiSymbol << uiTrDepth, eType, uiAbsPartIdx, uiDepth );
}

/** Parse (X,Y) position of the last significant coefficient
 * \param uiPosLastX reference to X component of last coefficient
 * \param uiPosLastY reference to Y component of last coefficient
 * \param width  Block width
 * \param height Block height
 * \param eTType plane type / luminance or chrominance
 * \param uiScanIdx scan type (zig-zag, hor, ver)
 *
 * This method decodes the X and Y component within a block of the last significant coefficient.
 */
Void TDecSbac::parseLastSignificantXY( UInt& uiPosLastX, UInt& uiPosLastY, Int width, Int height, TextType eTType, UInt uiScanIdx )
{
  UInt uiLast;
  ContextModel *pCtxX = m_cCuCtxLastX.get( 0, eTType );
  ContextModel *pCtxY = m_cCuCtxLastY.get( 0, eTType );

  // posX
  Int widthCtx = eTType ? 4 : width;
  const UInt *puiCtxIdxX = g_uiLastCtx + ( g_aucConvertToBit[ widthCtx ] * ( g_aucConvertToBit[ widthCtx ] + 3 ) );
  for( uiPosLastX = 0; uiPosLastX < g_uiGroupIdx[ width - 1 ]; uiPosLastX++ )
  {
    if ( eTType  )
    {
      m_pcTDecBinIf->decodeBin( uiLast, *( pCtxX + (uiPosLastX>>g_aucConvertToBit[ width ])  ) );
    }
    else
    {
      m_pcTDecBinIf->decodeBin( uiLast, *( pCtxX + puiCtxIdxX[ uiPosLastX ] ) );
    }
    if( !uiLast )
    {
      break;
    }
  }

  // posY
  Int heightCtx = eTType? 4 : height;
  const UInt *puiCtxIdxY = g_uiLastCtx + ( g_aucConvertToBit[ heightCtx ] * ( g_aucConvertToBit[ heightCtx ] + 3 ) );
  for( uiPosLastY = 0; uiPosLastY < g_uiGroupIdx[ height - 1 ]; uiPosLastY++ )
  {
    if (eTType)
    {
      m_pcTDecBinIf->decodeBin( uiLast, *( pCtxY + (uiPosLastY>>g_aucConvertToBit[ height ]) ) );
    }
    else
    {
      m_pcTDecBinIf->decodeBin( uiLast, *( pCtxY + puiCtxIdxY[ uiPosLastY ] ) );
    }
    if( !uiLast )
    {
      break;
    }
  }
  if ( uiPosLastX > 3 )
  {
    UInt uiTemp  = 0;
    UInt uiCount = ( uiPosLastX - 2 ) >> 1;
    for ( Int i = uiCount - 1; i >= 0; i-- )
    {
      m_pcTDecBinIf->decodeBinEP( uiLast );
      uiTemp += uiLast << i;
    }
    uiPosLastX = g_uiMinInGroup[ uiPosLastX ] + uiTemp;
  }
  if ( uiPosLastY > 3 )
  {
    UInt uiTemp  = 0;
    UInt uiCount = ( uiPosLastY - 2 ) >> 1;
    for ( Int i = uiCount - 1; i >= 0; i-- )
    {
      m_pcTDecBinIf->decodeBinEP( uiLast );
      uiTemp += uiLast << i;
    }
    uiPosLastY = g_uiMinInGroup[ uiPosLastY ] + uiTemp;
  }
  
  if( uiScanIdx == SCAN_VER )
  {
    swap( uiPosLastX, uiPosLastY );
  }
}

Void TDecSbac::parseCoeffNxN( TComDataCU* pcCU, TCoeff* pcCoef, UInt uiAbsPartIdx, UInt uiWidth, UInt uiHeight, UInt uiDepth, TextType eTType )
{
  DTRACE_CABAC_VL( g_nSymbolCounter++ )
  DTRACE_CABAC_T( "\tparseCoeffNxN()\teType=" )
  DTRACE_CABAC_V( eTType )
  DTRACE_CABAC_T( "\twidth=" )
  DTRACE_CABAC_V( uiWidth )
  DTRACE_CABAC_T( "\theight=" )
  DTRACE_CABAC_V( uiHeight )
  DTRACE_CABAC_T( "\tdepth=" )
  DTRACE_CABAC_V( uiDepth )
  DTRACE_CABAC_T( "\tabspartidx=" )
  DTRACE_CABAC_V( uiAbsPartIdx )
  DTRACE_CABAC_T( "\ttoCU-X=" )
  DTRACE_CABAC_V( pcCU->getCUPelX() )
  DTRACE_CABAC_T( "\ttoCU-Y=" )
  DTRACE_CABAC_V( pcCU->getCUPelY() )
  DTRACE_CABAC_T( "\tCU-addr=" )
  DTRACE_CABAC_V(  pcCU->getAddr() )
  DTRACE_CABAC_T( "\tinCU-X=" )
  DTRACE_CABAC_V( g_auiRasterToPelX[ g_auiZscanToRaster[uiAbsPartIdx] ] )
  DTRACE_CABAC_T( "\tinCU-Y=" )
  DTRACE_CABAC_V( g_auiRasterToPelY[ g_auiZscanToRaster[uiAbsPartIdx] ] )
  DTRACE_CABAC_T( "\tpredmode=" )
  DTRACE_CABAC_V(  pcCU->getPredictionMode( uiAbsPartIdx ) )
  DTRACE_CABAC_T( "\n" )
  
  if( uiWidth > pcCU->getSlice()->getSPS()->getMaxTrSize() )
  {
    uiWidth  = pcCU->getSlice()->getSPS()->getMaxTrSize();
    uiHeight = pcCU->getSlice()->getSPS()->getMaxTrSize();
  }
  
  eTType = eTType == TEXT_LUMA ? TEXT_LUMA : ( eTType == TEXT_NONE ? TEXT_NONE : TEXT_CHROMA );
  
  //----- parse significance map -----
  const UInt  uiLog2BlockSize   = g_aucConvertToBit[ uiWidth ] + 2;
  const UInt  uiMaxNumCoeff     = uiWidth * uiHeight;
  const UInt  uiMaxNumCoeffM1   = uiMaxNumCoeff - 1;
  UInt uiScanIdx = pcCU->getCoefScanIdx(uiAbsPartIdx, uiWidth, eTType==TEXT_LUMA, pcCU->isIntra(uiAbsPartIdx));
  int blockType = uiLog2BlockSize;
  if (uiWidth != uiHeight)
  {
    uiScanIdx = SCAN_DIAG;
    blockType = 4;
  }
  
  //===== decode last significant =====
  UInt uiPosLastX, uiPosLastY;
  parseLastSignificantXY( uiPosLastX, uiPosLastY, uiWidth, uiHeight, eTType, uiScanIdx );
  UInt uiBlkPosLast      = uiPosLastX + (uiPosLastY<<uiLog2BlockSize);
  pcCoef[ uiBlkPosLast ] = 1;

  //===== decode significance flags =====
  UInt uiScanPosLast   = uiBlkPosLast;
  if (uiScanIdx == SCAN_ZIGZAG)
  {
    // Map zigzag to diagonal scan
    uiScanIdx = SCAN_DIAG;
  }
  const UInt * scan;
  if (uiWidth == uiHeight)
  {
    scan = g_auiSigLastScan[ uiScanIdx ][ uiLog2BlockSize-1 ];
  }
  else
  {
    scan = g_sigScanNSQT[ uiLog2BlockSize - 2 ];
  }
  for( uiScanPosLast = 0; uiScanPosLast < uiMaxNumCoeffM1; uiScanPosLast++ )
  {
    UInt uiBlkPos = scan[ uiScanPosLast ];
    if( uiBlkPosLast == uiBlkPos )
    {
      break;
    }
  }

  ContextModel * const baseCoeffGroupCtx = m_cCUSigCoeffGroupSCModel.get( 0, eTType );
  ContextModel * const baseCtx = (eTType==TEXT_LUMA) ? m_cCUSigSCModel.get( 0, 0 ) : m_cCUSigSCModel.get( 0, 0 ) + NUM_SIG_FLAG_CTX_LUMA;

  const Int  iLastScanSet      = uiScanPosLast >> LOG2_SCAN_SET_SIZE;
  UInt uiNumOne                = 0;
  UInt uiGoRiceParam           = 0;

  UInt const tsig = pcCU->getSlice()->getPPS()->getTSIG();
#if LOSSLESS_CODING
  Bool beValid; 
  if (pcCU->isLosslessCoded(uiAbsPartIdx))
  {
    beValid = false;
  }
  else 
  {
    beValid = pcCU->getSlice()->getPPS()->getSignHideFlag() > 0;
  }
#else
  Bool beValid = pcCU->getSlice()->getPPS()->getSignHideFlag() > 0;
#endif
  UInt absSum = 0;

  UInt uiSigCoeffGroupFlag[ MLS_GRP_NUM ];
  ::memset( uiSigCoeffGroupFlag, 0, sizeof(UInt) * MLS_GRP_NUM );
  const UInt uiNumBlkSide = uiWidth >> (MLS_CG_SIZE >> 1);
  const UInt * scanCG;
  if (uiWidth == uiHeight)
  {
    scanCG = g_auiSigLastScan[ uiScanIdx ][ uiLog2BlockSize > 3 ? uiLog2BlockSize-2-1 : 0  ];    
    if( uiLog2BlockSize == 3 )
    {
      scanCG = g_sigLastScan8x8[ uiScanIdx ];
    }
    else if( uiLog2BlockSize == 5 )
    {
      scanCG = g_sigLastScanCG32x32;
    }
  }
  else
  {
    scanCG = g_sigCGScanNSQT[ uiLog2BlockSize - 2 ];
  }
  Int  iScanPosSig             = (Int) uiScanPosLast;
  for( Int iSubSet = iLastScanSet; iSubSet >= 0; iSubSet-- )
  {
    Int  iSubPos     = iSubSet << LOG2_SCAN_SET_SIZE;
    uiGoRiceParam    = 0;
    Int numNonZero = 0;
    
    Int lastNZPosInCG = -1, firstNZPosInCG = SCAN_SET_SIZE;

    Int pos[SCAN_SET_SIZE];
    if( iScanPosSig == (Int) uiScanPosLast )
    {
      lastNZPosInCG  = iScanPosSig;
      firstNZPosInCG = iScanPosSig;
      iScanPosSig--;
      pos[ numNonZero ] = uiBlkPosLast;
      numNonZero = 1;
    }

      // decode significant_coeffgroup_flag
      Int iCGBlkPos = scanCG[ iSubSet ];
      Int iCGPosY   = iCGBlkPos / uiNumBlkSide;
      Int iCGPosX   = iCGBlkPos - (iCGPosY * uiNumBlkSide);
      if( uiWidth == 8 && uiHeight == 8 && (uiScanIdx == SCAN_HOR || uiScanIdx == SCAN_VER) )
      {
        iCGPosY = (uiScanIdx == SCAN_HOR ? iCGBlkPos : 0);
        iCGPosX = (uiScanIdx == SCAN_VER ? iCGBlkPos : 0);
      }
      if( iSubSet == iLastScanSet || iSubSet == 0)
      {
        uiSigCoeffGroupFlag[ iCGBlkPos ] = 1;
      }
      else
      {
          UInt uiSigCoeffGroup;
          UInt uiCtxSig  = TComTrQuant::getSigCoeffGroupCtxInc( uiSigCoeffGroupFlag, iCGPosX, iCGPosY, uiScanIdx, uiWidth, uiHeight );
          m_pcTDecBinIf->decodeBin( uiSigCoeffGroup, baseCoeffGroupCtx[ uiCtxSig ] );
          uiSigCoeffGroupFlag[ iCGBlkPos ] = uiSigCoeffGroup;
      }

      // decode significant_coeff_flag
      UInt uiBlkPos, uiPosY, uiPosX, uiSig, uiCtxSig;
      for( ; iScanPosSig >= iSubPos; iScanPosSig-- )
      {
        uiBlkPos  = scan[ iScanPosSig ];
        uiPosY    = uiBlkPos >> uiLog2BlockSize;
        uiPosX    = uiBlkPos - ( uiPosY << uiLog2BlockSize );
        uiSig     = 0;
        
        if( uiSigCoeffGroupFlag[ iCGBlkPos ] )
        {
          if( iScanPosSig > iSubPos || iSubSet == 0  || numNonZero )
          {
            uiCtxSig  = TComTrQuant::getSigCtxInc( pcCoef, uiPosX, uiPosY, blockType, uiWidth, uiHeight, eTType );
            m_pcTDecBinIf->decodeBin( uiSig, baseCtx[ uiCtxSig ] );
          }
          else
          {
            uiSig = 1;
          }
        }
        pcCoef[ uiBlkPos ] = uiSig;
        if( uiSig )
        {
          pos[ numNonZero ] = uiBlkPos;
          numNonZero ++;
          if( lastNZPosInCG == -1 )
          {
            lastNZPosInCG = iScanPosSig;
          }
          firstNZPosInCG = iScanPosSig;
        }
      }

    
    if( numNonZero )
    {
      Bool signHidden = ( lastNZPosInCG - firstNZPosInCG >= (Int)tsig );
      absSum = 0;

      UInt c1 = 1;
      UInt uiCtxSet    = (iSubSet > 0 && eTType==TEXT_LUMA) ? 2 : 0;
      UInt uiBin;
      
      if( uiNumOne > 0 )
      {
        uiCtxSet++;
      }
      
      uiNumOne       >>= 1;
      ContextModel *baseCtxMod = ( eTType==TEXT_LUMA ) ? m_cCUOneSCModel.get( 0, 0 ) + 4 * uiCtxSet : m_cCUOneSCModel.get( 0, 0 ) + NUM_ONE_FLAG_CTX_LUMA + 4 * uiCtxSet;
      Int absCoeff[SCAN_SET_SIZE];

      for ( Int i = 0; i < numNonZero; i++) absCoeff[i] = 1;   
      Int numC1Flag = min(numNonZero, C1FLAG_NUMBER);
      Int firstC2FlagIdx = -1;

      for( Int idx = 0; idx < numC1Flag; idx++ )
      {
        m_pcTDecBinIf->decodeBin( uiBin, baseCtxMod[c1] );
        if( uiBin == 1 )
        {
          c1 = 0;
          if (firstC2FlagIdx == -1)
          {
            firstC2FlagIdx = idx;
          }
        }
        else if( (c1 < 3) && (c1 > 0) )
        {
          c1++;
        }
        absCoeff[ idx ] = uiBin + 1;
      }
      
      if (c1 == 0)
      {
        baseCtxMod = ( eTType==TEXT_LUMA ) ? m_cCUAbsSCModel.get( 0, 0 ) + uiCtxSet : m_cCUAbsSCModel.get( 0, 0 ) + NUM_ABS_FLAG_CTX_LUMA + uiCtxSet;
        if ( firstC2FlagIdx != -1)
        {
          m_pcTDecBinIf->decodeBin( uiBin, baseCtxMod[0] ); 
          absCoeff[ firstC2FlagIdx ] = uiBin + 2;
        }
      }

      UInt coeffSigns;
      if ( signHidden && beValid )
      {
        m_pcTDecBinIf->decodeBinsEP( coeffSigns, numNonZero-1 );
        coeffSigns <<= 32 - (numNonZero-1);
      }
      else
      {
        m_pcTDecBinIf->decodeBinsEP( coeffSigns, numNonZero );
        coeffSigns <<= 32 - numNonZero;
      }
      
      Int iFirstCoeff2 = 1;    
      if (c1 == 0 || numNonZero > C1FLAG_NUMBER)
      {
        for( Int idx = 0; idx < numNonZero; idx++ )
        {
          UInt baseLevel  = (idx < C1FLAG_NUMBER)? (2 + iFirstCoeff2) : 1;

          if( absCoeff[ idx ] == baseLevel)
          {
            UInt uiLevel;
            xReadGoRiceExGolomb( uiLevel, uiGoRiceParam );
            absCoeff[ idx ] = uiLevel + baseLevel;
          }

          if(absCoeff[ idx ] >= 2)  
          {
            iFirstCoeff2 = 0;
            uiNumOne++;
          }
        }
      }

      for( Int idx = 0; idx < numNonZero; idx++ )
      {
        Int blkPos = pos[ idx ];
        // Signs applied later.
        pcCoef[ blkPos ] = absCoeff[ idx ];
        absSum += absCoeff[ idx ];

        if ( idx == numNonZero-1 && signHidden && beValid )
        {
          // Infer sign of 1st element.
          if (absSum&0x1)
            pcCoef[ blkPos ] = -pcCoef[ blkPos ];
        }
        else
        {
          Int sign = static_cast<Int>( coeffSigns ) >> 31;
          pcCoef[ blkPos ] = ( pcCoef[ blkPos ] ^ sign ) - sign;
          coeffSigns <<= 1;
        }
      }
    }
    else
    {
      uiNumOne >>= 1;
    }
  }
  
  return;
}


Void TDecSbac::parseSaoUvlc (UInt& ruiVal)
{
  UInt uiCode;
  Int  i;

  m_pcTDecBinIf->decodeBin( uiCode, m_cSaoUvlcSCModel.get( 0, 0, 0 ) );
  if ( uiCode == 0 )
  {
    ruiVal = 0;
    return;
  }

  i=1;
  while (1)
  {
    m_pcTDecBinIf->decodeBin( uiCode, m_cSaoUvlcSCModel.get( 0, 0, 1 ) );
    if ( uiCode == 0 ) break;
    i++;
  }

  ruiVal = i;
}

Void TDecSbac::parseSaoSvlc (Int&  riVal)
{
  UInt uiCode;
  Int  iSign;
  Int  i;

  m_pcTDecBinIf->decodeBin( uiCode, m_cSaoSvlcSCModel.get( 0, 0, 0 ) );

  if ( uiCode == 0 )
  {
    riVal = 0;
    return;
  }

  // read sign
  m_pcTDecBinIf->decodeBin( uiCode, m_cSaoSvlcSCModel.get( 0, 0, 1 ) );

  if ( uiCode == 0 )
  {
    iSign =  1;
  }
  else
  {
    iSign = -1;
  }

  // read magnitude
  i=1;
  while (1)
  {
    m_pcTDecBinIf->decodeBin( uiCode, m_cSaoSvlcSCModel.get( 0, 0, 2 ) );
    if ( uiCode == 0 ) break;
    i++;
  }

  riVal = i*iSign;
}

Void TDecSbac::parseSaoUflc (UInt&  riVal)
{
  UInt uiSymbol;
  riVal = 0;
  for (Int i=0;i<5;i++)
  {
    m_pcTDecBinIf->decodeBinEP ( uiSymbol );
    if (uiSymbol)
    {
      riVal |= (1<<i);
    }
  }
}
Void TDecSbac::parseSaoMergeLeft (UInt&  ruiVal, UInt uiCompIdx)
{
  UInt uiCode;
  m_pcTDecBinIf->decodeBin( uiCode, m_cSaoMergeLeftSCModel.get( 0, 0, uiCompIdx ) );
  ruiVal = (Int)uiCode;
}

Void TDecSbac::parseSaoMergeUp (UInt&  ruiVal)
{
  UInt uiCode;
  m_pcTDecBinIf->decodeBin( uiCode, m_cSaoMergeUpSCModel.get( 0, 0, 0 ) );
  ruiVal = (Int)uiCode;
}
Void TDecSbac::parseSaoTypeIdx (UInt&  ruiVal)
{
  UInt uiCode;
  Int  i;
  m_pcTDecBinIf->decodeBin( uiCode, m_cSaoTypeIdxSCModel.get( 0, 0, 0 ) );
  if ( uiCode == 0 )
  {
    ruiVal = 0;
    return;
  }
  i=1;
  while (1)
  {
    m_pcTDecBinIf->decodeBin( uiCode, m_cSaoTypeIdxSCModel.get( 0, 0, 1 ) );
    if ( uiCode == 0 ) break;
    i++;
  }
  ruiVal = i;
}

inline Void copySaoOneLcuParam(SaoLcuParam* psDst,  SaoLcuParam* psSrc)
{
  Int i;
  psDst->partIdx = psSrc->partIdx;
  psDst->typeIdx    = psSrc->typeIdx;
  if (psDst->typeIdx != -1)
  {
    if (psDst->typeIdx == SAO_BO)
    {
      psDst->bandPosition = psSrc->bandPosition ;
    }
    else
    {
      psDst->bandPosition = 0;
    }
    psDst->length  = psSrc->length;
    for (i=0;i<psDst->length;i++)
    {
      psDst->offset[i] = psSrc->offset[i];
    }
  }
  else
  {
    psDst->length  = 0;
    for (i=0;i<SAO_BO_LEN;i++)
    {
      psDst->offset[i] = 0;
    }
  }
}
Void TDecSbac::parseSaoOffset(SaoLcuParam* psSaoLcuParam)
{
  UInt uiSymbol;
  Int iSymbol;
  static Int iTypeLength[MAX_NUM_SAO_TYPE] = {
    SAO_EO_LEN,
    SAO_EO_LEN,
    SAO_EO_LEN,
    SAO_EO_LEN,
    SAO_BO_LEN
  }; 

  parseSaoTypeIdx(uiSymbol);
  psSaoLcuParam->typeIdx = (Int)uiSymbol - 1;
  if (uiSymbol)
  {
    psSaoLcuParam->length = iTypeLength[psSaoLcuParam->typeIdx];
    if( psSaoLcuParam->typeIdx == SAO_BO )
    {
      // Parse Left Band Index
      parseSaoUflc( uiSymbol );
      psSaoLcuParam->bandPosition = uiSymbol;
      for(Int i=0; i< psSaoLcuParam->length; i++)
      {
        parseSaoSvlc(iSymbol);
        psSaoLcuParam->offset[i] = iSymbol;
      }   
    }
    else if( psSaoLcuParam->typeIdx < 4 )
    {
      parseSaoUvlc(uiSymbol); psSaoLcuParam->offset[0] = uiSymbol;
      parseSaoUvlc(uiSymbol); psSaoLcuParam->offset[1] = uiSymbol;
      parseSaoUvlc(uiSymbol); psSaoLcuParam->offset[2] = -(Int)uiSymbol;
      parseSaoUvlc(uiSymbol); psSaoLcuParam->offset[3] = -(Int)uiSymbol;
    }
  }
  else
  {
    psSaoLcuParam->length = 0;
  }
}

Void TDecSbac::parseSaoOneLcuInterleaving(Int rx, Int ry, SAOParam* pSaoParam, TComDataCU* pcCU, Int iCUAddrInSlice, Int iCUAddrUpInSlice, Bool bLFCrossSliceBoundaryFlag)
{
  Int iAddr = pcCU->getAddr();
  UInt uiSymbol;
  for (Int iCompIdx=0; iCompIdx<3; iCompIdx++)
  {
    pSaoParam->saoLcuParam[iCompIdx][iAddr].mergeUpFlag    = 0;
    pSaoParam->saoLcuParam[iCompIdx][iAddr].mergeLeftFlag  = 0;
    pSaoParam->saoLcuParam[iCompIdx][iAddr].bandPosition   = 0;
    pSaoParam->saoLcuParam[iCompIdx][iAddr].typeIdx        = -1;
    pSaoParam->saoLcuParam[iCompIdx][iAddr].offset[0]     = 0;
    pSaoParam->saoLcuParam[iCompIdx][iAddr].offset[1]     = 0;
    pSaoParam->saoLcuParam[iCompIdx][iAddr].offset[2]     = 0;
    pSaoParam->saoLcuParam[iCompIdx][iAddr].offset[3]     = 0;

    if (pSaoParam->bSaoFlag[iCompIdx])
    {
      if (rx>0 && iCUAddrInSlice!=0)
      {
        parseSaoMergeLeft(uiSymbol,iCompIdx); pSaoParam->saoLcuParam[iCompIdx][iAddr].mergeLeftFlag = (Int)uiSymbol;
      }
      else
      {
        pSaoParam->saoLcuParam[iCompIdx][iAddr].mergeLeftFlag = 0;
      }

      if (pSaoParam->saoLcuParam[iCompIdx][iAddr].mergeLeftFlag==0)
      {
        if ((ry > 0) && (iCUAddrUpInSlice>0||bLFCrossSliceBoundaryFlag))
        {
          parseSaoMergeUp(uiSymbol);  pSaoParam->saoLcuParam[iCompIdx][iAddr].mergeUpFlag = uiSymbol;
        }
        else
        {
          pSaoParam->saoLcuParam[iCompIdx][iAddr].mergeUpFlag = 0;
        }
        if (!pSaoParam->saoLcuParam[iCompIdx][iAddr].mergeUpFlag)
        {
          parseSaoOffset(&(pSaoParam->saoLcuParam[iCompIdx][iAddr]));
        }
        else
        {
          copySaoOneLcuParam(&pSaoParam->saoLcuParam[iCompIdx][iAddr], &pSaoParam->saoLcuParam[iCompIdx][iAddr-pSaoParam->numCuInWidth]);
        }
      }
      else
      {
        copySaoOneLcuParam(&pSaoParam->saoLcuParam[iCompIdx][iAddr],  &pSaoParam->saoLcuParam[iCompIdx][iAddr-1]);
      }
    }
    else
    {
      pSaoParam->saoLcuParam[iCompIdx][iAddr].typeIdx = -1;
      pSaoParam->saoLcuParam[iCompIdx][iAddr].bandPosition = 0;
    }
  }
}

/**
 - Initialize our contexts from the nominated source.
 .
 \param pSrc Contexts to be copied.
 */
Void TDecSbac::xCopyContextsFrom( TDecSbac* pSrc )
{
  memcpy(m_contextModels, pSrc->m_contextModels, m_numContextModels*sizeof(m_contextModels[0]));
}

Void TDecSbac::xCopyFrom( TDecSbac* pSrc )
{
  m_pcTDecBinIf->copyState( pSrc->m_pcTDecBinIf );

  m_uiLastQp           = pSrc->m_uiLastQp;
  xCopyContextsFrom( pSrc );

}

Void TDecSbac::load ( TDecSbac* pScr )
{
  xCopyFrom(pScr);
}

Void TDecSbac::loadContexts ( TDecSbac* pScr )
{
  xCopyContextsFrom(pScr);
}

Void TDecSbac::decodeFlush ( )
{
  UInt uiBit;
  m_pcTDecBinIf->decodeBinTrm(uiBit);
  m_pcTDecBinIf->flush();

}

#if HHI_DMM_WEDGE_INTRA || HHI_DMM_PRED_TEX || (LGE_EDGE_INTRA_A0070 && LGE_EDGE_INTRA_DELTA_DC)
Void TDecSbac::xReadExGolombLevel( UInt& ruiSymbol, ContextModel& rcSCModel  )
{
  UInt uiSymbol;
  UInt uiCount = 0;
  do
  {
    m_pcTDecBinIf->decodeBin( uiSymbol, rcSCModel );
    uiCount++;
  }
  while( uiSymbol && ( uiCount != 13 ) );

  ruiSymbol = uiCount - 1;

  if( uiSymbol )
  {
    xReadEpExGolomb( uiSymbol, 0 );
    ruiSymbol += uiSymbol + 1;
  }

  return;
}
#endif
#if HHI_DMM_WEDGE_INTRA
Void TDecSbac::xParseWedgeFullInfo( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth )
{
  Int iIntraIdx = pcCU->getIntraSizeIdx(uiAbsPartIdx);
  Int iBits = g_aucWedgeFullBitsListIdx[iIntraIdx];

  UInt uiSymbol, uiTabIdx = 0;
  for ( Int i = 0; i < iBits; i++ )
  {
    m_pcTDecBinIf->decodeBin( uiSymbol, m_cDmmDataSCModel.get(0, 0, 0) );
    uiTabIdx += ( uiSymbol && i == 0 ) ? 1 : 0;
    uiTabIdx += ( uiSymbol && i == 1 ) ? 2 : 0;
    uiTabIdx += ( uiSymbol && i == 2 ) ? 4 : 0;
    uiTabIdx += ( uiSymbol && i == 3 ) ? 8 : 0;
    uiTabIdx += ( uiSymbol && i == 4 ) ? 16 : 0;
    uiTabIdx += ( uiSymbol && i == 5 ) ? 32 : 0;
    uiTabIdx += ( uiSymbol && i == 6 ) ? 64 : 0;
    uiTabIdx += ( uiSymbol && i == 7 ) ? 128 : 0;
    uiTabIdx += ( uiSymbol && i == 8 ) ? 256 : 0;
    uiTabIdx += ( uiSymbol && i == 9 ) ? 512 : 0;
    uiTabIdx += ( uiSymbol && i == 10 ) ? 1024 : 0;
    uiTabIdx += ( uiSymbol && i == 11 ) ? 2048 : 0;
    uiTabIdx += ( uiSymbol && i == 12 ) ? 4096 : 0;
  }

  pcCU->setWedgeFullTabIdxSubParts( uiTabIdx, uiAbsPartIdx, uiDepth );
}

Void TDecSbac::xParseWedgeFullDeltaInfo( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth )
{
  Int iIntraIdx = pcCU->getIntraSizeIdx(uiAbsPartIdx);
  Int iBits = g_aucWedgeFullBitsListIdx[iIntraIdx];

  UInt uiSymbol, uiTabIdx = 0;
  for ( Int i = 0; i < iBits; i++ )
  {
    m_pcTDecBinIf->decodeBin( uiSymbol, m_cDmmDataSCModel.get(0, 0, 0) );
    uiTabIdx += ( uiSymbol && i == 0 ) ? 1 : 0;
    uiTabIdx += ( uiSymbol && i == 1 ) ? 2 : 0;
    uiTabIdx += ( uiSymbol && i == 2 ) ? 4 : 0;
    uiTabIdx += ( uiSymbol && i == 3 ) ? 8 : 0;
    uiTabIdx += ( uiSymbol && i == 4 ) ? 16 : 0;
    uiTabIdx += ( uiSymbol && i == 5 ) ? 32 : 0;
    uiTabIdx += ( uiSymbol && i == 6 ) ? 64 : 0;
    uiTabIdx += ( uiSymbol && i == 7 ) ? 128 : 0;
    uiTabIdx += ( uiSymbol && i == 8 ) ? 256 : 0;
    uiTabIdx += ( uiSymbol && i == 9 ) ? 512 : 0;
    uiTabIdx += ( uiSymbol && i == 10 ) ? 1024 : 0;
    uiTabIdx += ( uiSymbol && i == 11 ) ? 2048 : 0;
    uiTabIdx += ( uiSymbol && i == 12 ) ? 4096 : 0;
  }

  pcCU->setWedgeFullTabIdxSubParts( uiTabIdx, uiAbsPartIdx, uiDepth );

  UInt uiDC1, uiDC2;
  xReadExGolombLevel( uiDC1, m_cDmmDataSCModel.get(0, 0, 1) );
  Int iDC1 = uiDC1;
  if ( uiDC1 )
  {
    UInt uiSign;
    m_pcTDecBinIf->decodeBinEP( uiSign );
    if ( uiSign )
    {
      iDC1 = -iDC1;
    }
  }
  xReadExGolombLevel( uiDC2, m_cDmmDataSCModel.get(0, 0, 1) );
  Int iDC2 = uiDC2;
  if ( uiDC2 )
  {
    UInt uiSign;
    m_pcTDecBinIf->decodeBinEP( uiSign );
    if ( uiSign )
    {
      iDC2 = -iDC2;
    }
  }

  pcCU->setWedgeFullDeltaDC1SubParts( iDC1, uiAbsPartIdx, uiDepth );
  pcCU->setWedgeFullDeltaDC2SubParts( iDC2, uiAbsPartIdx, uiDepth );
}

Void TDecSbac::xParseWedgePredDirInfo( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth )
{
  if( DMM_WEDGE_PREDDIR_DELTAEND_MAX > 0 )
  {
    UInt uiDeltaEnd = 0;
    m_pcTDecBinIf->decodeBin( uiDeltaEnd, m_cDmmDataSCModel.get(0, 0, 2) );

    Int iDeltaEnd;
    if( uiDeltaEnd != 0 )
    {
      UInt uiAbsValMinus1;
      UInt uiSymbol;
      m_pcTDecBinIf->decodeBin( uiSymbol, m_cDmmDataSCModel.get(0, 0, 2) ); uiAbsValMinus1  = uiSymbol;
      m_pcTDecBinIf->decodeBin( uiSymbol, m_cDmmDataSCModel.get(0, 0, 2) ); uiAbsValMinus1 |= uiSymbol << 1;
      uiDeltaEnd = uiAbsValMinus1 + 1;

      iDeltaEnd = uiDeltaEnd;
      UInt uiSign;
      m_pcTDecBinIf->decodeBinEP( uiSign );
      if( uiSign )
      {
        iDeltaEnd = -iDeltaEnd;
      }
    }
    else
    {
      iDeltaEnd = 0;
    }
    pcCU->setWedgePredDirDeltaEndSubParts( iDeltaEnd, uiAbsPartIdx, uiDepth );
  }
}

Void TDecSbac::xParseWedgePredDirDeltaInfo( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth )
{
  if( DMM_WEDGE_PREDDIR_DELTAEND_MAX > 0 )
  {
    UInt uiDeltaEnd = 0;
    m_pcTDecBinIf->decodeBin( uiDeltaEnd, m_cDmmDataSCModel.get(0, 0, 2) );

    Int iDeltaEnd;
    if( uiDeltaEnd != 0 )
    {
      UInt uiAbsValMinus1;
      UInt uiSymbol;
      m_pcTDecBinIf->decodeBin( uiSymbol, m_cDmmDataSCModel.get(0, 0, 2) ); uiAbsValMinus1  = uiSymbol;
      m_pcTDecBinIf->decodeBin( uiSymbol, m_cDmmDataSCModel.get(0, 0, 2) ); uiAbsValMinus1 |= uiSymbol << 1;
      uiDeltaEnd = uiAbsValMinus1 + 1;

      iDeltaEnd = uiDeltaEnd;
      UInt uiSign;
      m_pcTDecBinIf->decodeBinEP( uiSign );
      if( uiSign )
      {
        iDeltaEnd = -iDeltaEnd;
      }
    }
    else
    {
      iDeltaEnd = 0;
    }

    pcCU->setWedgePredDirDeltaEndSubParts( iDeltaEnd, uiAbsPartIdx, uiDepth );
  }

  UInt uiDC1, uiDC2;
  xReadExGolombLevel( uiDC1, m_cDmmDataSCModel.get(0, 0, 1) );
  Int iDC1 = uiDC1;
  if ( uiDC1 )
  {
    UInt uiSign;
    m_pcTDecBinIf->decodeBinEP( uiSign );
    if ( uiSign )
    {
      iDC1 = -iDC1;
    }
  }
  xReadExGolombLevel( uiDC2, m_cDmmDataSCModel.get(0, 0, 1) );
  Int iDC2 = uiDC2;
  if ( uiDC2 )
  {
    UInt uiSign;
    m_pcTDecBinIf->decodeBinEP( uiSign );
    if ( uiSign )
    {
      iDC2 = -iDC2;
    }
  }

  pcCU->setWedgePredDirDeltaDC1SubParts( iDC1, uiAbsPartIdx, uiDepth );
  pcCU->setWedgePredDirDeltaDC2SubParts( iDC2, uiAbsPartIdx, uiDepth );
}
#endif
#if HHI_DMM_PRED_TEX
#if LGE_DMM3_SIMP_C0044
Void TDecSbac::xParseWedgePredTexInfo( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth )
{
  Int iIntraIdx = pcCU->getIntraSizeIdx(uiAbsPartIdx);
  Int iBits = g_aucWedgeTexPredBitsListIdx[iIntraIdx];

  UInt uiSymbol, uiTabIdx = 0;
  for ( Int i = 0; i < iBits; i++ )
  {
    m_pcTDecBinIf->decodeBin( uiSymbol, m_cDmmDataSCModel.get(0, 0, 3) );
    uiTabIdx += ( uiSymbol && i == 0 ) ? 1 : 0;
    uiTabIdx += ( uiSymbol && i == 1 ) ? 2 : 0;
    uiTabIdx += ( uiSymbol && i == 2 ) ? 4 : 0;
    uiTabIdx += ( uiSymbol && i == 3 ) ? 8 : 0;
    uiTabIdx += ( uiSymbol && i == 4 ) ? 16 : 0;
    uiTabIdx += ( uiSymbol && i == 5 ) ? 32 : 0;
    uiTabIdx += ( uiSymbol && i == 6 ) ? 64 : 0;
    uiTabIdx += ( uiSymbol && i == 7 ) ? 128 : 0;
    uiTabIdx += ( uiSymbol && i == 8 ) ? 256 : 0;
    uiTabIdx += ( uiSymbol && i == 9 ) ? 512 : 0;
    uiTabIdx += ( uiSymbol && i == 10 ) ? 1024 : 0;
  }

  pcCU->setWedgePredTexIntraTabIdxSubParts( uiTabIdx, uiAbsPartIdx, uiDepth );
}
#endif

Void TDecSbac::xParseWedgePredTexDeltaInfo( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth )
{
#if LGE_DMM3_SIMP_C0044
  xParseWedgePredTexInfo( pcCU, uiAbsPartIdx, uiDepth );
#endif
  UInt uiDC1, uiDC2;
  xReadExGolombLevel( uiDC1, m_cDmmDataSCModel.get(0, 0, 1) );
  Int iDC1 = uiDC1;
  if ( uiDC1 )
  {
    UInt uiSign;
    m_pcTDecBinIf->decodeBinEP( uiSign );
    if ( uiSign )
    {
      iDC1 = -iDC1;
    }
  }
  xReadExGolombLevel( uiDC2, m_cDmmDataSCModel.get(0, 0, 1) );
  Int iDC2 = uiDC2;
  if ( uiDC2 )
  {
    UInt uiSign;
    m_pcTDecBinIf->decodeBinEP( uiSign );
    if ( uiSign )
    {
      iDC2 = -iDC2;
    }
  }

  pcCU->setWedgePredTexDeltaDC1SubParts( iDC1, uiAbsPartIdx, uiDepth );
  pcCU->setWedgePredTexDeltaDC2SubParts( iDC2, uiAbsPartIdx, uiDepth );
}

Void TDecSbac::xParseContourPredTexDeltaInfo( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth )
{
  UInt uiDC1, uiDC2;
  xReadExGolombLevel( uiDC1, m_cDmmDataSCModel.get(0, 0, 1) );
  Int iDC1 = uiDC1;
  if ( uiDC1 )
  {
    UInt uiSign;
    m_pcTDecBinIf->decodeBinEP( uiSign );
    if ( uiSign )
    {
      iDC1 = -iDC1;
    }
  }
  xReadExGolombLevel( uiDC2, m_cDmmDataSCModel.get(0, 0, 1) );
  Int iDC2 = uiDC2;
  if ( uiDC2 )
  {
    UInt uiSign;
    m_pcTDecBinIf->decodeBinEP( uiSign );
    if ( uiSign )
    {
      iDC2 = -iDC2;
    }
  }

  pcCU->setContourPredTexDeltaDC1SubParts( iDC1, uiAbsPartIdx, uiDepth );
  pcCU->setContourPredTexDeltaDC2SubParts( iDC2, uiAbsPartIdx, uiDepth );
}
#endif
#if QC_ARP_D0177
Void TDecSbac::parseARPW( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth )
{
  UInt nMaxW = pcCU->getSlice()->getARPStepNum() - 1;
  UInt nW = 0;
  if( nMaxW > 0 )
  {
    UInt nOffset = pcCU->getCTXARPWFlag(uiAbsPartIdx);    
    UInt uiCode = 0;
    m_pcTDecBinIf->decodeBin( uiCode , m_cCUPUARPW.get( 0, 0, 0 + nOffset ) );
    nW = uiCode;
    if( nW == 1 )   
    {
      m_pcTDecBinIf->decodeBin( uiCode , m_cCUPUARPW.get( 0, 0, 3 ) );
      nW += ( uiCode == 1 );
    }
  }
  pcCU->setARPWSubParts( ( UChar )( nW ) , uiAbsPartIdx, uiDepth );  
}
#endif
#if LGE_EDGE_INTRA_A0070
Void TDecSbac::xParseEdgeIntraInfo( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth )
{
  UInt uiSymbol = 0;

  // 1. Top(0) or Left(1)
  UChar ucLeft;
  m_pcTDecBinIf->decodeBinEP( uiSymbol );
  ucLeft = uiSymbol;

  // 2. Start position (lowest bit first)
  UChar ucStart = 0;
  for( UInt ui = 0; ui < 6 - uiDepth; ui++ )
  {
    m_pcTDecBinIf->decodeBinEP( uiSymbol );
    ucStart |= (uiSymbol << ui);
  }

  // 3. Number of edges
  UChar ucMax = 0;
  for( UInt ui = 0; ui < 7 - uiDepth; ui++ )
  {
    m_pcTDecBinIf->decodeBinEP( uiSymbol );
    ucMax |= (uiSymbol << ui);
  }
  ucMax++; // +1

  // 4. Edges
  UChar* pucSymbolList = (UChar*) xMalloc( UChar, 256 * LGE_EDGE_INTRA_MAX_EDGE_NUM_PER_4x4 );
  UInt uiCtxEdgeIntra = pcCU->getCtxEdgeIntra( uiAbsPartIdx );
  for( Int iPtr = 0; iPtr < ucMax; iPtr++ )
  {
    UChar ucEdge = 0;
    UInt  uiReorderEdge = 0;
    // Left-friendly direction
    // 0 (   0deg) => 0
    // 1 (  45deg) => 10
    // 2 ( -45deg) => 110
    // 3 (  90deg) => 1110
    // 4 ( -90deg) => 11110
    // 5 ( 135deg) => 111110
    // 6 (-135deg) => 111111
    // Right-friendly direction
    // 0 (   0deg) => 0
    // 1 ( -45deg) => 10
    // 2 (  45deg) => 110
    // 3 ( -90deg) => 1110
    // 4 (  90deg) => 11110
    // 5 (-135deg) => 111110
    // 6 ( 135deg) => 111111
    // refer to a paper "An efficient chain code with Huffman coding"
    for( UInt ui = 0; ui < 6; ui++ )
    {
      m_pcTDecBinIf->decodeBin( uiSymbol, m_cEdgeIntraSCModel.get( 0, 0, uiCtxEdgeIntra ) );
      ucEdge <<= 1;
      ucEdge |= uiSymbol;
      if( uiSymbol == 0 )
        break;
    }

    switch( ucEdge )
    {
    case 0 :  // "0"
      uiReorderEdge = 0;
      break;
    case 2 :  // "10"
      uiReorderEdge = 1;
      break;
    case 6 :  // "110"
      uiReorderEdge = 2;
      break;
    case 14 : // "1110"
      uiReorderEdge = 3;
      break;
    case 30 : // "11110"
      uiReorderEdge = 4;
      break;
    case 62 : // "111110"
      uiReorderEdge = 5;
      break;
    case 63 : // "111111"
      uiReorderEdge = 6;
      break;
    default :
      printf("parseIntraEdgeChain: error (unknown code %d)\n",ucEdge);
      assert(false);
      break;
    }
    pucSymbolList[iPtr] = uiReorderEdge;
  }
  /////////////////////
  // Edge Reconstruction
  Bool* pbRegion = pcCU->getEdgePartition( uiAbsPartIdx );
  pcCU->reconPartition( uiAbsPartIdx, uiDepth, ucLeft == 1, ucStart, ucMax, pucSymbolList, pbRegion );
  xFree( pucSymbolList );
}
#endif
  
#if RWTH_SDC_DLT_B0036
Void TDecSbac::parseSDCFlag( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth )
{
  assert( pcCU->getSlice()->getSPS()->isDepth() );
  
  UInt uiSymbol = 0;
  UInt uiCtxSDCFlag = pcCU->getCtxSDCFlag( uiAbsPartIdx );
  m_pcTDecBinIf->decodeBin( uiSymbol, m_cSDCFlagSCModel.get( 0, 0, uiCtxSDCFlag ) );
  
  if( uiSymbol == 1 )
  {
    pcCU->setPartSizeSubParts(SIZE_2Nx2N, uiAbsPartIdx, uiDepth);
    
    pcCU->setSDCFlagSubParts( true, uiAbsPartIdx, 0, uiDepth);
    pcCU->setTrIdxSubParts(0, uiAbsPartIdx, uiDepth);
    pcCU->setCbfSubParts(1, 1, 1, uiAbsPartIdx, uiDepth);
  }
}

Void TDecSbac::parseSDCPredMode( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth )
{
  assert( pcCU->getSlice()->getSPS()->isDepth() );
  assert( pcCU->getPartitionSize(uiAbsPartIdx) == SIZE_2Nx2N );
  
  assert( pcCU->getSDCFlag(uiAbsPartIdx) );
  
  UInt uiCtx            = 0;
  
  UInt uiMPModeIdx      = 0;
  
  for(Int i=0; i<RWTH_SDC_NUM_PRED_MODES-1; i++)
  {
    UInt uiIsMostProb = 0;
    m_pcTDecBinIf->decodeBin( uiIsMostProb, m_cSDCPredModeSCModel.get( 0, i, uiCtx ) );
    
    if ( uiIsMostProb == 1 )
      break;
    
    // else: get next most probable pred mode
    uiMPModeIdx = (uiMPModeIdx+1)%RWTH_SDC_NUM_PRED_MODES;
  }
  
  Int intraPredMode = g_auiSDCPredModes[uiMPModeIdx];
  
#if HHI_DMM_WEDGE_INTRA
  if( intraPredMode == DMM_WEDGE_FULL_IDX )          { xParseWedgeFullInfo          ( pcCU, uiAbsPartIdx, uiDepth ); }
  if( intraPredMode == DMM_WEDGE_PREDDIR_IDX )       { xParseWedgePredDirInfo       ( pcCU, uiAbsPartIdx, uiDepth ); }
#endif
  
  pcCU->setLumaIntraDirSubParts((UChar)intraPredMode, uiAbsPartIdx, uiDepth);
}

Void TDecSbac::parseSDCResidualData ( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth, UInt uiSegment )
{
  assert( pcCU->getSlice()->getSPS()->isDepth() );
  assert( pcCU->getSDCFlag(uiAbsPartIdx) );
  assert( uiSegment < 2 );
  
  UInt uiResidual = 0;
  UInt uiBit      = 0;
  UInt uiAbsIdx   = 0;
  UInt uiSign     = 0;
  Int  iIdx       = 0;
  
  UInt uiMaxResidualBits  = GetBitsPerDepthValue();
  assert( uiMaxResidualBits <= g_uiBitDepth );
  
  m_pcTDecBinIf->decodeBin(uiResidual, m_cSDCResidualFlagSCModel.get( 0, uiSegment, 0 ) );
  
  if (uiResidual)
  {
    // decode residual sign bit
    m_pcTDecBinIf->decodeBin(uiSign, m_cSDCResidualSignFlagSCModel.get( 0, uiSegment, 0 ) );
    
    // decode residual magnitude
    for (Int i=0; i<uiMaxResidualBits; i++)
    {
      m_pcTDecBinIf->decodeBin(uiBit, m_cSDCResidualSCModel.get( 0, uiSegment, i ) );
      uiAbsIdx |= uiBit << i;
    }
    
    uiAbsIdx += 1;
    iIdx =(Int)(uiSign ? -1 : 1)*uiAbsIdx;
  }
  
  pcCU->setSDCSegmentDCOffset(iIdx, uiSegment, uiAbsPartIdx);
}
#endif

//! \}
