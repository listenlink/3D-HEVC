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

/** \file     TEncSbac.cpp
    \brief    SBAC encoder class
*/

#include "TEncTop.h"
#include "TEncSbac.h"

#include <map>
#include <algorithm>

#if RWTH_SDC_DLT_B0036
#define GetNumDepthValues()     (pcCU->getSlice()->getSPS()->getNumDepthValues())
#define GetBitsPerDepthValue()  (pcCU->getSlice()->getSPS()->getBitsPerDepthValue())
#endif

//! \ingroup TLibEncoder
//! \{

// ====================================================================================================================
// Constructor / destructor / create / destroy
// ====================================================================================================================

TEncSbac::TEncSbac()
// new structure here
: m_pcBitIf                   ( NULL )
, m_pcSlice                   ( NULL )
, m_pcBinIf                   ( NULL )
, m_bAlfCtrl                  ( false )
, m_uiCoeffCost               ( 0 )
, m_uiMaxAlfCtrlDepth         ( 0 )
, m_numContextModels          ( 0 )
, m_cCUSplitFlagSCModel       ( 1,             1,               NUM_SPLIT_FLAG_CTX            , m_contextModels + m_numContextModels, m_numContextModels )
, m_cCUSkipFlagSCModel        ( 1,             1,               NUM_SKIP_FLAG_CTX             , m_contextModels + m_numContextModels, m_numContextModels)
#if LGE_ILLUCOMP_B0045
, m_cCUICFlagSCModel          ( 1,             1,               NUM_IC_FLAG_CTX               , m_contextModels + m_numContextModels, m_numContextModels)
#endif
, m_cCUMergeFlagExtSCModel    ( 1,             1,               NUM_MERGE_FLAG_EXT_CTX        , m_contextModels + m_numContextModels, m_numContextModels)
, m_cCUMergeIdxExtSCModel     ( 1,             1,               NUM_MERGE_IDX_EXT_CTX         , m_contextModels + m_numContextModels, m_numContextModels)
#if HHI_INTER_VIEW_RESIDUAL_PRED
, m_cResPredFlagSCModel       ( 1,             1,               NUM_RES_PRED_FLAG_CTX         , m_contextModels + m_numContextModels, m_numContextModels)
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
#if AMP_CTX
, m_cCUAMPSCModel             ( 1,             1,               NUM_CU_AMP_CTX                , m_contextModels + m_numContextModels, m_numContextModels)
#else
, m_cCUXPosiSCModel           ( 1,             1,               NUM_CU_X_POS_CTX              , m_contextModels + m_numContextModels, m_numContextModels)
, m_cCUYPosiSCModel           ( 1,             1,               NUM_CU_Y_POS_CTX              , m_contextModels + m_numContextModels, m_numContextModels)
#endif
, m_cSaoFlagSCModel           ( 1,             1,               NUM_SAO_FLAG_CTX              , m_contextModels + m_numContextModels, m_numContextModels)
, m_cSaoUvlcSCModel           ( 1,             1,               NUM_SAO_UVLC_CTX              , m_contextModels + m_numContextModels, m_numContextModels)
, m_cSaoSvlcSCModel           ( 1,             1,               NUM_SAO_SVLC_CTX              , m_contextModels + m_numContextModels, m_numContextModels)
#if SAO_UNIT_INTERLEAVING
, m_cSaoMergeLeftSCModel      ( 1,             1,               NUM_SAO_MERGE_LEFT_FLAG_CTX   , m_contextModels + m_numContextModels, m_numContextModels)
, m_cSaoMergeUpSCModel        ( 1,             1,               NUM_SAO_MERGE_UP_FLAG_CTX     , m_contextModels + m_numContextModels, m_numContextModels)
, m_cSaoTypeIdxSCModel        ( 1,             1,               NUM_SAO_TYPE_IDX_CTX          , m_contextModels + m_numContextModels, m_numContextModels)
#endif
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

TEncSbac::~TEncSbac()
{
}

// ====================================================================================================================
// Public member functions
// ====================================================================================================================

Void TEncSbac::resetEntropy           ()
{
  Int  iQp              = m_pcSlice->getSliceQp();
  SliceType eSliceType  = m_pcSlice->getSliceType();
  
#if CABAC_INIT_FLAG
  Int  encCABACTableIdx = m_pcSlice->getPPS()->getEncCABACTableIdx();
  if (!m_pcSlice->isIntra() && (encCABACTableIdx==B_SLICE || encCABACTableIdx==P_SLICE) && m_pcSlice->getPPS()->getCabacInitPresentFlag())
  {
    eSliceType = (SliceType) encCABACTableIdx;
  }
#endif

  m_cCUSplitFlagSCModel.initBuffer       ( eSliceType, iQp, (UChar*)INIT_SPLIT_FLAG );
  
  m_cCUSkipFlagSCModel.initBuffer        ( eSliceType, iQp, (UChar*)INIT_SKIP_FLAG );
#if LGE_ILLUCOMP_B0045
  m_cCUICFlagSCModel.initBuffer          ( eSliceType, iQp, (UChar*)INIT_IC_FLAG );
#endif
  m_cCUAlfCtrlFlagSCModel.initBuffer     ( eSliceType, iQp, (UChar*)INIT_ALF_CTRL_FLAG );
  m_cCUMergeFlagExtSCModel.initBuffer    ( eSliceType, iQp, (UChar*)INIT_MERGE_FLAG_EXT);
  m_cCUMergeIdxExtSCModel.initBuffer     ( eSliceType, iQp, (UChar*)INIT_MERGE_IDX_EXT);
#if HHI_INTER_VIEW_RESIDUAL_PRED
  m_cResPredFlagSCModel.initBuffer       ( eSliceType, iQp, (UChar*)INIT_RES_PRED_FLAG );
#endif
  m_cCUPartSizeSCModel.initBuffer        ( eSliceType, iQp, (UChar*)INIT_PART_SIZE );
#if AMP_CTX
  m_cCUAMPSCModel.initBuffer             ( eSliceType, iQp, (UChar*)INIT_CU_AMP_POS );
#else
  m_cCUXPosiSCModel.initBuffer           ( eSliceType, iQp, (UChar*)INIT_CU_X_POS );
  m_cCUYPosiSCModel.initBuffer           ( eSliceType, iQp, (UChar*)INIT_CU_Y_POS );
#endif
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
  m_cCUTransSubdivFlagSCModel.initBuffer ( eSliceType, iQp, (UChar*)INIT_TRANS_SUBDIV_FLAG );
  m_cSaoFlagSCModel.initBuffer           ( eSliceType, iQp, (UChar*)INIT_SAO_FLAG );
  m_cSaoUvlcSCModel.initBuffer           ( eSliceType, iQp, (UChar*)INIT_SAO_UVLC );
  m_cSaoSvlcSCModel.initBuffer           ( eSliceType, iQp, (UChar*)INIT_SAO_SVLC );
#if SAO_UNIT_INTERLEAVING
  m_cSaoMergeLeftSCModel.initBuffer      ( eSliceType, iQp, (UChar*)INIT_SAO_MERGE_LEFT_FLAG );
  m_cSaoMergeUpSCModel.initBuffer        ( eSliceType, iQp, (UChar*)INIT_SAO_MERGE_UP_FLAG );
  m_cSaoTypeIdxSCModel.initBuffer        ( eSliceType, iQp, (UChar*)INIT_SAO_TYPE_IDX );
#endif
#if HHI_DMM_WEDGE_INTRA || HHI_DMM_PRED_TEX
  m_cDmmFlagSCModel.initBuffer           ( eSliceType, iQp, (UChar*)INIT_DMM_FLAG );
  m_cDmmModeSCModel.initBuffer           ( eSliceType, iQp, (UChar*)INIT_DMM_MODE );
  m_cDmmDataSCModel.initBuffer           ( eSliceType, iQp, (UChar*)INIT_DMM_DATA );
#endif
#if LGE_EDGE_INTRA_A0070
  m_cEdgeIntraSCModel.initBuffer         ( eSliceType, iQp, (UChar*)INIT_EDGE_INTRA );
#if LGE_EDGE_INTRA_DELTA_DC
  m_cEdgeIntraDeltaDCSCModel.initBuffer  ( eSliceType, iQp, (UChar*)INIT_EDGE_INTRA_DELTA_DC );
#endif
#endif
#if RWTH_SDC_DLT_B0036
  m_cSDCFlagSCModel.initBuffer              ( eSliceType, iQp, (UChar*)INIT_SDC_FLAG );
  m_cSDCResidualFlagSCModel.initBuffer      ( eSliceType, iQp, (UChar*)INIT_SDC_RESIDUAL_FLAG );
  m_cSDCResidualSCModel.initBuffer          ( eSliceType, iQp, (UChar*)INIT_SDC_RESIDUAL );
  m_cSDCResidualSignFlagSCModel.initBuffer  ( eSliceType, iQp, (UChar*)INIT_SDC_SIGN_FLAG );
  m_cSDCPredModeSCModel.initBuffer              ( eSliceType, iQp, (UChar*)INIT_SDC_PRED_MODE );
#endif

  // new structure
  m_uiLastQp = iQp;
  
  m_pcBinIf->start();
  
  return;
}

#if CABAC_INIT_FLAG
/** The function does the following: 
 * If current slice type is P/B then it determines the distance of initialisation type 1 and 2 from the current CABAC states and 
 * stores the index of the closest table.  This index is used for the next P/B slice when cabac_init_present_flag is true.
 */
Void TEncSbac::determineCabacInitIdx()
{
  Int  qp              = m_pcSlice->getSliceQp();

  if (!m_pcSlice->isIntra())
  {
    SliceType aSliceTypeChoices[] = {B_SLICE, P_SLICE};

    UInt bestCost             = MAX_UINT;
    SliceType bestSliceType   = aSliceTypeChoices[0];
    for (UInt idx=0; idx<2; idx++)
    {
      UInt curCost          = 0;
      SliceType curSliceType  = aSliceTypeChoices[idx];

      curCost  = m_cCUSplitFlagSCModel.calcCost       ( curSliceType, qp, (UChar*)INIT_SPLIT_FLAG );
      curCost += m_cCUSkipFlagSCModel.calcCost        ( curSliceType, qp, (UChar*)INIT_SKIP_FLAG );
#if LGE_ILLUCOMP_B0045
      curCost += m_cCUICFlagSCModel.calcCost          ( curSliceType, qp, (UChar*)INIT_IC_FLAG );
#endif
      curCost += m_cCUAlfCtrlFlagSCModel.calcCost     ( curSliceType, qp, (UChar*)INIT_ALF_CTRL_FLAG );
      curCost += m_cCUMergeFlagExtSCModel.calcCost    ( curSliceType, qp, (UChar*)INIT_MERGE_FLAG_EXT);
      curCost += m_cCUMergeIdxExtSCModel.calcCost     ( curSliceType, qp, (UChar*)INIT_MERGE_IDX_EXT);
#if HHI_INTER_VIEW_RESIDUAL_PRED
      curCost += m_cResPredFlagSCModel.calcCost       ( curSliceType, qp, (UChar*)INIT_RES_PRED_FLAG);
#endif
      curCost += m_cCUPartSizeSCModel.calcCost        ( curSliceType, qp, (UChar*)INIT_PART_SIZE );
#if AMP_CTX
      curCost += m_cCUAMPSCModel.calcCost             ( curSliceType, qp, (UChar*)INIT_CU_AMP_POS );
#else
      curCost += m_cCUXPosiSCModel.calcCost           ( curSliceType, qp, (UChar*)INIT_CU_X_POS );
      curCost += m_cCUYPosiSCModel.calcCost           ( curSliceType, qp, (UChar*)INIT_CU_Y_POS );
#endif
      curCost += m_cCUPredModeSCModel.calcCost        ( curSliceType, qp, (UChar*)INIT_PRED_MODE );
      curCost += m_cCUIntraPredSCModel.calcCost       ( curSliceType, qp, (UChar*)INIT_INTRA_PRED_MODE );
      curCost += m_cCUChromaPredSCModel.calcCost      ( curSliceType, qp, (UChar*)INIT_CHROMA_PRED_MODE );
      curCost += m_cCUInterDirSCModel.calcCost        ( curSliceType, qp, (UChar*)INIT_INTER_DIR );
      curCost += m_cCUMvdSCModel.calcCost             ( curSliceType, qp, (UChar*)INIT_MVD );
      curCost += m_cCURefPicSCModel.calcCost          ( curSliceType, qp, (UChar*)INIT_REF_PIC );
      curCost += m_cCUDeltaQpSCModel.calcCost         ( curSliceType, qp, (UChar*)INIT_DQP );
      curCost += m_cCUQtCbfSCModel.calcCost           ( curSliceType, qp, (UChar*)INIT_QT_CBF );
      curCost += m_cCUQtRootCbfSCModel.calcCost       ( curSliceType, qp, (UChar*)INIT_QT_ROOT_CBF );
      curCost += m_cCUSigCoeffGroupSCModel.calcCost   ( curSliceType, qp, (UChar*)INIT_SIG_CG_FLAG );
      curCost += m_cCUSigSCModel.calcCost             ( curSliceType, qp, (UChar*)INIT_SIG_FLAG );
      curCost += m_cCuCtxLastX.calcCost               ( curSliceType, qp, (UChar*)INIT_LAST );
      curCost += m_cCuCtxLastY.calcCost               ( curSliceType, qp, (UChar*)INIT_LAST );
      curCost += m_cCUOneSCModel.calcCost             ( curSliceType, qp, (UChar*)INIT_ONE_FLAG );
      curCost += m_cCUAbsSCModel.calcCost             ( curSliceType, qp, (UChar*)INIT_ABS_FLAG );
      curCost += m_cMVPIdxSCModel.calcCost            ( curSliceType, qp, (UChar*)INIT_MVP_IDX );
      curCost += m_cALFFlagSCModel.calcCost           ( curSliceType, qp, (UChar*)INIT_ALF_FLAG );
      curCost += m_cALFUvlcSCModel.calcCost           ( curSliceType, qp, (UChar*)INIT_ALF_UVLC );
      curCost += m_cALFSvlcSCModel.calcCost           ( curSliceType, qp, (UChar*)INIT_ALF_SVLC );
      curCost += m_cCUTransSubdivFlagSCModel.calcCost ( curSliceType, qp, (UChar*)INIT_TRANS_SUBDIV_FLAG );
      curCost += m_cSaoFlagSCModel.calcCost           ( curSliceType, qp, (UChar*)INIT_SAO_FLAG );
      curCost += m_cSaoUvlcSCModel.calcCost           ( curSliceType, qp, (UChar*)INIT_SAO_UVLC );
      curCost += m_cSaoSvlcSCModel.calcCost           ( curSliceType, qp, (UChar*)INIT_SAO_SVLC );
#if SAO_UNIT_INTERLEAVING
      curCost += m_cSaoMergeLeftSCModel.calcCost      ( curSliceType, qp, (UChar*)INIT_SAO_MERGE_LEFT_FLAG );
      curCost += m_cSaoMergeUpSCModel.calcCost        ( curSliceType, qp, (UChar*)INIT_SAO_MERGE_UP_FLAG );
      curCost += m_cSaoTypeIdxSCModel.calcCost        ( curSliceType, qp, (UChar*)INIT_SAO_TYPE_IDX );
#endif

      if (curCost < bestCost)
      {
        bestSliceType = curSliceType;
        bestCost      = curCost;
      }
    }
    m_pcSlice->getPPS()->setEncCABACTableIdx( bestSliceType );
  }
  else
  {
    m_pcSlice->getPPS()->setEncCABACTableIdx( I_SLICE );
  }  

  #if CABAC_INIT_FLAG && FIX_POZNAN_CABAC_INIT_FLAG
    m_pcSlice->getPPS()->setEncPrevPOC( m_pcSlice->getPOC() );
  #endif
}
#endif


/** The function does the followng: Write out terminate bit. Flush CABAC. Intialize CABAC states. Start CABAC.
 */
Void TEncSbac::updateContextTables( SliceType eSliceType, Int iQp, Bool bExecuteFinish )
{
  m_pcBinIf->encodeBinTrm(1);
  if (bExecuteFinish) m_pcBinIf->finish();
  m_cCUSplitFlagSCModel.initBuffer       ( eSliceType, iQp, (UChar*)INIT_SPLIT_FLAG );
  
  m_cCUSkipFlagSCModel.initBuffer        ( eSliceType, iQp, (UChar*)INIT_SKIP_FLAG );
#if LGE_ILLUCOMP_B0045
  m_cCUICFlagSCModel.initBuffer          ( eSliceType, iQp, (UChar*)INIT_IC_FLAG );
#endif
  m_cCUAlfCtrlFlagSCModel.initBuffer     ( eSliceType, iQp, (UChar*)INIT_ALF_CTRL_FLAG );
  m_cCUMergeFlagExtSCModel.initBuffer    ( eSliceType, iQp, (UChar*)INIT_MERGE_FLAG_EXT);
  m_cCUMergeIdxExtSCModel.initBuffer     ( eSliceType, iQp, (UChar*)INIT_MERGE_IDX_EXT);
#if HHI_INTER_VIEW_RESIDUAL_PRED
  m_cResPredFlagSCModel.initBuffer       ( eSliceType, iQp, (UChar*)INIT_RES_PRED_FLAG );
#endif
  m_cCUPartSizeSCModel.initBuffer        ( eSliceType, iQp, (UChar*)INIT_PART_SIZE );
#if AMP_CTX
  m_cCUAMPSCModel.initBuffer             ( eSliceType, iQp, (UChar*)INIT_CU_AMP_POS );
#else
  m_cCUXPosiSCModel.initBuffer           ( eSliceType, iQp, (UChar*)INIT_CU_X_POS );
  m_cCUYPosiSCModel.initBuffer           ( eSliceType, iQp, (UChar*)INIT_CU_Y_POS );
#endif
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
  m_cCUTransSubdivFlagSCModel.initBuffer ( eSliceType, iQp, (UChar*)INIT_TRANS_SUBDIV_FLAG );
  m_cSaoFlagSCModel.initBuffer           ( eSliceType, iQp, (UChar*)INIT_SAO_FLAG );
  m_cSaoUvlcSCModel.initBuffer           ( eSliceType, iQp, (UChar*)INIT_SAO_UVLC );
  m_cSaoSvlcSCModel.initBuffer           ( eSliceType, iQp, (UChar*)INIT_SAO_SVLC );
#if SAO_UNIT_INTERLEAVING
  m_cSaoMergeLeftSCModel.initBuffer      ( eSliceType, iQp, (UChar*)INIT_SAO_MERGE_LEFT_FLAG );
  m_cSaoMergeUpSCModel.initBuffer        ( eSliceType, iQp, (UChar*)INIT_SAO_MERGE_UP_FLAG );
  m_cSaoTypeIdxSCModel.initBuffer        ( eSliceType, iQp, (UChar*)INIT_SAO_TYPE_IDX );
#endif
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
  
  m_pcBinIf->start();
}

Void TEncSbac::writeTileMarker( UInt uiTileIdx, UInt uiBitsUsed )
{
  for (Int iShift=uiBitsUsed-1; iShift>=0; iShift--)
  {
    m_pcBinIf->encodeBinEP ( (uiTileIdx & (1 << iShift)) >> iShift );
  }
}

void TEncSbac::codeSEI(const SEI&)
{
  assert(0);
}

#if VIDYO_VPS_INTEGRATION|QC_MVHEVC_B0046
Void TEncSbac::codeVPS( TComVPS* pcVPS )
{
  assert (0);
  return;
}
#endif

#if HHI_MPI
Void TEncSbac::codeSPS( TComSPS* pcSPS, Bool bIsDepth )
#else
Void TEncSbac::codeSPS( TComSPS* pcSPS )
#endif
{
  assert (0);
  return;
}

Void TEncSbac::codePPS( TComPPS* pcPPS )
{
  assert (0);
  return;
}

Void TEncSbac::codeSliceHeader( TComSlice* pcSlice )
{
  assert (0);
  return;
}

#if TILES_WPP_ENTRY_POINT_SIGNALLING
Void TEncSbac::codeTilesWPPEntryPoint( TComSlice* pSlice )
{
  assert (0);
  return;
}
#else
Void TEncSbac::codeSliceHeaderSubstreamTable( TComSlice* pcSlice )
{
  assert (0);
}
#endif

Void TEncSbac::codeTerminatingBit( UInt uilsLast )
{
  m_pcBinIf->encodeBinTrm( uilsLast );
}

Void TEncSbac::codeSliceFinish()
{
  m_pcBinIf->finish();
}

#if OL_FLUSH
Void TEncSbac::codeFlush()
{
  m_pcBinIf->flush();
}

Void TEncSbac::encodeStart()
{
  m_pcBinIf->start();
}
#endif

Void TEncSbac::xWriteUnarySymbol( UInt uiSymbol, ContextModel* pcSCModel, Int iOffset )
{
  m_pcBinIf->encodeBin( uiSymbol ? 1 : 0, pcSCModel[0] );
  
  if( 0 == uiSymbol)
  {
    return;
  }
  
  while( uiSymbol-- )
  {
    m_pcBinIf->encodeBin( uiSymbol ? 1 : 0, pcSCModel[ iOffset ] );
  }
  
  return;
}

Void TEncSbac::xWriteUnaryMaxSymbol( UInt uiSymbol, ContextModel* pcSCModel, Int iOffset, UInt uiMaxSymbol )
{
  if (uiMaxSymbol == 0)
  {
    return;
  }
  
  m_pcBinIf->encodeBin( uiSymbol ? 1 : 0, pcSCModel[ 0 ] );
  
  if ( uiSymbol == 0 )
  {
    return;
  }
  
  Bool bCodeLast = ( uiMaxSymbol > uiSymbol );
  
  while( --uiSymbol )
  {
    m_pcBinIf->encodeBin( 1, pcSCModel[ iOffset ] );
  }
  if( bCodeLast )
  {
    m_pcBinIf->encodeBin( 0, pcSCModel[ iOffset ] );
  }
  
  return;
}

Void TEncSbac::xWriteEpExGolomb( UInt uiSymbol, UInt uiCount )
{
  UInt bins = 0;
  Int numBins = 0;
  
  while( uiSymbol >= (UInt)(1<<uiCount) )
  {
    bins = 2 * bins + 1;
    numBins++;
    uiSymbol -= 1 << uiCount;
    uiCount  ++;
  }
  bins = 2 * bins + 0;
  numBins++;
  
  bins = (bins << uiCount) | uiSymbol;
  numBins += uiCount;
  
  assert( numBins <= 32 );
  m_pcBinIf->encodeBinsEP( bins, numBins );
}

/** Coding of coeff_abs_level_minus3
 * \param uiSymbol value of coeff_abs_level_minus3
 * \param ruiGoRiceParam reference to Rice parameter
 * \returns Void
 */
Void TEncSbac::xWriteGoRiceExGolomb( UInt uiSymbol, UInt &ruiGoRiceParam )
{
  UInt uiMaxVlc     = g_auiGoRiceRange[ ruiGoRiceParam ];
  Bool bExGolomb    = ( uiSymbol > uiMaxVlc );
  UInt uiCodeWord   = min<UInt>( uiSymbol, ( uiMaxVlc + 1 ) );
  UInt uiQuotient   = uiCodeWord >> ruiGoRiceParam;
  UInt uiMaxPreLen  = g_auiGoRicePrefixLen[ ruiGoRiceParam ];
  
  UInt binValues;
  Int numBins;
  
  if ( uiQuotient >= uiMaxPreLen )
  {
    numBins = uiMaxPreLen;
    binValues = ( 1 << numBins ) - 1;
  }
  else
  {
    numBins = uiQuotient + 1;
    binValues = ( 1 << numBins ) - 2;
  }
  
  m_pcBinIf->encodeBinsEP( ( binValues << ruiGoRiceParam ) + uiCodeWord - ( uiQuotient << ruiGoRiceParam ), numBins + ruiGoRiceParam );
  
#if EIGHT_BITS_RICE_CODE
  ruiGoRiceParam = g_aauiGoRiceUpdate[ ruiGoRiceParam ][ min<UInt>( uiSymbol, 23 ) ];
#else
  ruiGoRiceParam = g_aauiGoRiceUpdate[ ruiGoRiceParam ][ min<UInt>( uiSymbol, 15 ) ];
#endif
  
  if( bExGolomb )
  {
    uiSymbol -= uiMaxVlc + 1;
    xWriteEpExGolomb( uiSymbol, 0 );
  }
}

// SBAC RD
Void  TEncSbac::load ( TEncSbac* pSrc)
{
  this->xCopyFrom(pSrc);
}

Void  TEncSbac::loadIntraDirModeLuma( TEncSbac* pSrc)
{
  m_pcBinIf->copyState( pSrc->m_pcBinIf );
  
  this->m_cCUIntraPredSCModel      .copyFrom( &pSrc->m_cCUIntraPredSCModel       );
}


Void  TEncSbac::store( TEncSbac* pDest)
{
  pDest->xCopyFrom( this );
}


Void TEncSbac::xCopyFrom( TEncSbac* pSrc )
{
  m_pcBinIf->copyState( pSrc->m_pcBinIf );
  
  this->m_uiCoeffCost = pSrc->m_uiCoeffCost;
  this->m_uiLastQp    = pSrc->m_uiLastQp;
  
  memcpy( m_contextModels, pSrc->m_contextModels, m_numContextModels * sizeof( ContextModel ) );
}

#if HHI_INTER_VIEW_MOTION_PRED
Void TEncSbac::codeMVPIdx ( TComDataCU* pcCU, UInt uiAbsPartIdx, RefPicList eRefList, Int iNum )
#else
Void TEncSbac::codeMVPIdx ( TComDataCU* pcCU, UInt uiAbsPartIdx, RefPicList eRefList )
#endif
{
  Int iSymbol = pcCU->getMVPIdx(eRefList, uiAbsPartIdx);
#if HHI_INTER_VIEW_MOTION_PRED
#else
  Int iNum = AMVP_MAX_NUM_CANDS;
#endif

  xWriteUnaryMaxSymbol(iSymbol, m_cMVPIdxSCModel.get(0), 1, iNum-1);
}

Void TEncSbac::codePartSize( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth )
{
  PartSize eSize         = pcCU->getPartitionSize( uiAbsPartIdx );

#if OL_QTLIMIT_PREDCODING_B0068
  TComSPS *sps           = pcCU->getPic()->getSlice(0)->getSPS();
  TComPic *pcTexture     = pcCU->getSlice()->getTexturePic();
  Bool bDepthMapDetect   = (pcTexture != NULL);
  Bool bIntraSliceDetect = (pcCU->getSlice()->getSliceType() == I_SLICE);
 
#if HHI_QTLPC_RAU_OFF_C0160
  Bool rapPic     = (pcCU->getSlice()->getNalUnitType() == NAL_UNIT_CODED_SLICE_IDR || pcCU->getSlice()->getNalUnitType() == NAL_UNIT_CODED_SLICE_CRA);
  if(bDepthMapDetect && !bIntraSliceDetect && !rapPic && sps->getUseQTLPC() && pcCU->getPic()->getReduceBitsFlag())
#else
  if(bDepthMapDetect && !bIntraSliceDetect && sps->getUseQTLPC() && pcCU->getPic()->getReduceBitsFlag())
#endif
  {
    TComDataCU *pcTextureCU = pcTexture->getCU(pcCU->getAddr());
    UInt uiCUIdx            = (pcCU->getZorderIdxInCU() == 0) ? uiAbsPartIdx : pcCU->getZorderIdxInCU();
    assert(pcTextureCU->getDepth(uiCUIdx) >= uiDepth);
    if (pcTextureCU->getDepth(uiCUIdx) == uiDepth && pcTextureCU->getPartitionSize( uiCUIdx ) != SIZE_NxN)
    {
      assert( eSize == SIZE_2Nx2N );
      return;
    }
  }
#endif

  if ( pcCU->isIntra( uiAbsPartIdx ) )
  {
    if( uiDepth == g_uiMaxCUDepth - g_uiAddCUDepth )
    {
      m_pcBinIf->encodeBin( eSize == SIZE_2Nx2N? 1 : 0, m_cCUPartSizeSCModel.get( 0, 0, 0 ) );
    }
    return;
  }
  
  switch(eSize)
  {
  case SIZE_2Nx2N:
    {
      m_pcBinIf->encodeBin( 1, m_cCUPartSizeSCModel.get( 0, 0, 0) );
      break;
    }
  case SIZE_2NxN:
  case SIZE_2NxnU:
  case SIZE_2NxnD:
    {
      m_pcBinIf->encodeBin( 0, m_cCUPartSizeSCModel.get( 0, 0, 0) );
      m_pcBinIf->encodeBin( 1, m_cCUPartSizeSCModel.get( 0, 0, 1) );
      if ( pcCU->getSlice()->getSPS()->getAMPAcc( uiDepth ) )
      {
        if (eSize == SIZE_2NxN)
        {
#if AMP_CTX           
          m_pcBinIf->encodeBin(1, m_cCUAMPSCModel.get( 0, 0, 0 ));
#else
          m_pcBinIf->encodeBin(1, m_cCUYPosiSCModel.get( 0, 0, 0 ));
#endif
        }
        else
        {
#if AMP_CTX
          m_pcBinIf->encodeBin(0, m_cCUAMPSCModel.get( 0, 0, 0 ));          
          m_pcBinIf->encodeBinEP((eSize == SIZE_2NxnU? 0: 1));
#else
          m_pcBinIf->encodeBin(0, m_cCUYPosiSCModel.get( 0, 0, 0 ));
          m_pcBinIf->encodeBin((eSize == SIZE_2NxnU? 0: 1), m_cCUYPosiSCModel.get( 0, 0, 1 ));
#endif
        }
      }
      break;
    }
  case SIZE_Nx2N:
  case SIZE_nLx2N:
  case SIZE_nRx2N:
    {
      m_pcBinIf->encodeBin( 0, m_cCUPartSizeSCModel.get( 0, 0, 0) );
      m_pcBinIf->encodeBin( 0, m_cCUPartSizeSCModel.get( 0, 0, 1) );
      if( uiDepth == g_uiMaxCUDepth - g_uiAddCUDepth && !( pcCU->getSlice()->getSPS()->getDisInter4x4() && pcCU->getWidth(uiAbsPartIdx) == 8 && pcCU->getHeight(uiAbsPartIdx) == 8 ) )
      {
        m_pcBinIf->encodeBin( 1, m_cCUPartSizeSCModel.get( 0, 0, 2) );
      }
      if ( pcCU->getSlice()->getSPS()->getAMPAcc( uiDepth ) )
      {
        if (eSize == SIZE_Nx2N)
        {
#if AMP_CTX
          m_pcBinIf->encodeBin(1, m_cCUAMPSCModel.get( 0, 0, 0 ));
#else
          m_pcBinIf->encodeBin(1, m_cCUXPosiSCModel.get( 0, 0, 0 ));
#endif
        }
        else
        {
#if AMP_CTX
          m_pcBinIf->encodeBin(0, m_cCUAMPSCModel.get( 0, 0, 0 ));
          m_pcBinIf->encodeBinEP((eSize == SIZE_nLx2N? 0: 1));
#else
          m_pcBinIf->encodeBin(0, m_cCUXPosiSCModel.get( 0, 0, 0 ));
          m_pcBinIf->encodeBin((eSize == SIZE_nLx2N? 0: 1), m_cCUXPosiSCModel.get( 0, 0, 1 ));
#endif
        }
      }
      break;
    }
  case SIZE_NxN:
    {
      if( uiDepth == g_uiMaxCUDepth - g_uiAddCUDepth && !( pcCU->getSlice()->getSPS()->getDisInter4x4() && pcCU->getWidth(uiAbsPartIdx) == 8 && pcCU->getHeight(uiAbsPartIdx) == 8 ) )
      {
        m_pcBinIf->encodeBin( 0, m_cCUPartSizeSCModel.get( 0, 0, 0) );
        m_pcBinIf->encodeBin( 0, m_cCUPartSizeSCModel.get( 0, 0, 1) );
        m_pcBinIf->encodeBin( 0, m_cCUPartSizeSCModel.get( 0, 0, 2) );
      }
      break;
    }
  default:
    {
      assert(0);
    }
  }
}

/** code prediction mode
 * \param pcCU
 * \param uiAbsPartIdx  
 * \returns Void
 */
Void TEncSbac::codePredMode( TComDataCU* pcCU, UInt uiAbsPartIdx )
{
#if RWTH_SDC_DLT_B0036
  if ( pcCU->getSlice()->isIntra() )
  {
    assert( pcCU->isIntra(uiAbsPartIdx) );
    return;
  }
#endif
  
  // get context function is here
  Int iPredMode = pcCU->getPredictionMode( uiAbsPartIdx );
  m_pcBinIf->encodeBin( iPredMode == MODE_INTER ? 0 : 1, m_cCUPredModeSCModel.get( 0, 0, 0 ) );
}

Void TEncSbac::codeAlfCtrlFlag( TComDataCU* pcCU, UInt uiAbsPartIdx )
{
  if (!m_bAlfCtrl)
    return;
  
  if( pcCU->getDepth(uiAbsPartIdx) > m_uiMaxAlfCtrlDepth && !pcCU->isFirstAbsZorderIdxInDepth(uiAbsPartIdx, m_uiMaxAlfCtrlDepth))
  {
    return;
  }
  
  const UInt uiSymbol = pcCU->getAlfCtrlFlag( uiAbsPartIdx ) ? 1 : 0;
  m_pcBinIf->encodeBin( uiSymbol, *m_cCUAlfCtrlFlagSCModel.get( 0 ) );
}

Void TEncSbac::codeAlfCtrlDepth()
{
  if (!m_bAlfCtrl)
    return;
  
  UInt uiDepth = m_uiMaxAlfCtrlDepth;
  xWriteUnaryMaxSymbol(uiDepth, m_cALFUvlcSCModel.get(0), 1, g_uiMaxCUDepth-1);
}

/** code skip flag
 * \param pcCU
 * \param uiAbsPartIdx 
 * \returns Void
 */
Void TEncSbac::codeSkipFlag( TComDataCU* pcCU, UInt uiAbsPartIdx )
{
  // get context function is here
  UInt uiSymbol = pcCU->isSkipped( uiAbsPartIdx ) ? 1 : 0;
  UInt uiCtxSkip = pcCU->getCtxSkipFlag( uiAbsPartIdx ) ;
  m_pcBinIf->encodeBin( uiSymbol, m_cCUSkipFlagSCModel.get( 0, 0, uiCtxSkip ) );
  DTRACE_CABAC_VL( g_nSymbolCounter++ );
  DTRACE_CABAC_T( "\tSkipFlag" );
  DTRACE_CABAC_T( "\tuiCtxSkip: ");
  DTRACE_CABAC_V( uiCtxSkip );
  DTRACE_CABAC_T( "\tuiSymbol: ");
  DTRACE_CABAC_V( uiSymbol );
  DTRACE_CABAC_T( "\n");
}

#if LGE_ILLUCOMP_B0045
/** code Illumination Compensation flag
 * \param pcCU
 * \param uiAbsPartIdx 
 * \returns Void
 */
Void TEncSbac::codeICFlag( TComDataCU* pcCU, UInt uiAbsPartIdx )
{
  // get context function is here
  UInt uiSymbol = pcCU->getICFlag( uiAbsPartIdx ) ? 1 : 0;
  UInt uiCtxIC  = pcCU->getCtxICFlag( uiAbsPartIdx ) ;
  m_pcBinIf->encodeBin( uiSymbol, m_cCUICFlagSCModel.get( 0, 0, uiCtxIC ) );
  DTRACE_CABAC_VL( g_nSymbolCounter++ );
  DTRACE_CABAC_T( "\tICFlag" );
  DTRACE_CABAC_T( "\tuiCtxIC: ");
  DTRACE_CABAC_V( uiCtxIC );
  DTRACE_CABAC_T( "\tuiSymbol: ");
  DTRACE_CABAC_V( uiSymbol );
  DTRACE_CABAC_T( "\n");
}
#endif

/** code merge flag
 * \param pcCU
 * \param uiAbsPartIdx 
 * \returns Void
 */
Void TEncSbac::codeMergeFlag( TComDataCU* pcCU, UInt uiAbsPartIdx )
{
  const UInt uiSymbol = pcCU->getMergeFlag( uiAbsPartIdx ) ? 1 : 0;
  m_pcBinIf->encodeBin( uiSymbol, *m_cCUMergeFlagExtSCModel.get( 0 ) );

  DTRACE_CABAC_VL( g_nSymbolCounter++ );
  DTRACE_CABAC_T( "\tMergeFlag: " );
  DTRACE_CABAC_V( uiSymbol );
  DTRACE_CABAC_T( "\tAddress: " );
  DTRACE_CABAC_V( pcCU->getAddr() );
  DTRACE_CABAC_T( "\tuiAbsPartIdx: " );
  DTRACE_CABAC_V( uiAbsPartIdx );
  DTRACE_CABAC_T( "\n" );
}

/** code merge index
 * \param pcCU
 * \param uiAbsPartIdx 
 * \returns Void
 */
Void TEncSbac::codeMergeIndex( TComDataCU* pcCU, UInt uiAbsPartIdx )
{
  UInt uiNumCand = MRG_MAX_NUM_CANDS;
#if !MRG_IDX_CTX_RED
  UInt auiCtx[4] = { 0, 1, 2, 3 };
#endif
  UInt uiUnaryIdx = pcCU->getMergeIndex( uiAbsPartIdx );
  uiNumCand = pcCU->getSlice()->getMaxNumMergeCand();
#if HHI_MPI
  const Bool bMVIAvailable = pcCU->getSlice()->getSPS()->getUseMVI() && pcCU->getSlice()->getSliceType() != I_SLICE;
  if( bMVIAvailable )
  {
    const Bool bUseMVI = pcCU->getTextureModeDepth( uiAbsPartIdx ) != -1;
    if( bUseMVI )
    {
      uiUnaryIdx = (UInt)HHI_MPI_MERGE_POS;
    }
    else if( (Int)uiUnaryIdx >= (Int)HHI_MPI_MERGE_POS )
    {
      uiUnaryIdx++;
    }
  }
#endif
  if ( uiNumCand > 1 )
  {
    for( UInt ui = 0; ui < uiNumCand - 1; ++ui )
    {
      const UInt uiSymbol = ui == uiUnaryIdx ? 0 : 1;
#if MRG_IDX_CTX_RED
      if ( ui==0 )
      {
        m_pcBinIf->encodeBin( uiSymbol, m_cCUMergeIdxExtSCModel.get( 0, 0, 0 ) );
      }
      else
      {
        m_pcBinIf->encodeBinEP( uiSymbol );
      }
#else
      m_pcBinIf->encodeBin( uiSymbol, m_cCUMergeIdxExtSCModel.get( 0, 0, auiCtx[ui] ) );
#endif
      if( uiSymbol == 0 )
      {
        break;
      }
    }
  }
  DTRACE_CABAC_VL( g_nSymbolCounter++ );
  DTRACE_CABAC_T( "\tparseMergeIndex()" );
  DTRACE_CABAC_T( "\tuiMRGIdx= " );
  DTRACE_CABAC_V( pcCU->getMergeIndex( uiAbsPartIdx ) );
  DTRACE_CABAC_T( "\n" );
}

#if HHI_INTER_VIEW_RESIDUAL_PRED
Void
TEncSbac::codeResPredFlag( TComDataCU* pcCU, UInt uiAbsPartIdx )
{
  UInt  uiCtx     = pcCU->getCtxResPredFlag( uiAbsPartIdx );
  UInt  uiSymbol  = ( pcCU->getResPredFlag( uiAbsPartIdx ) ? 1 : 0 );
  m_pcBinIf->encodeBin( uiSymbol, m_cResPredFlagSCModel.get( 0, 0, uiCtx ) );
}
#endif

Void TEncSbac::codeSplitFlag   ( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth )
{
  if( uiDepth == g_uiMaxCUDepth - g_uiAddCUDepth )
    return;
  
  UInt uiCtx           = pcCU->getCtxSplitFlag( uiAbsPartIdx, uiDepth );
  UInt uiCurrSplitFlag = ( pcCU->getDepth( uiAbsPartIdx ) > uiDepth ) ? 1 : 0;
  
  assert( uiCtx < 3 );

#if OL_QTLIMIT_PREDCODING_B0068
  Bool bCodeSplitFlag    = true;

  TComSPS *sps           = pcCU->getPic()->getSlice(0)->getSPS();
  TComPic *pcTexture     = pcCU->getSlice()->getTexturePic();
  Bool bDepthMapDetect   = (pcTexture != NULL);
  Bool bIntraSliceDetect = (pcCU->getSlice()->getSliceType() == I_SLICE);

#if HHI_QTLPC_RAU_OFF_C0160
  Bool rapPic     = (pcCU->getSlice()->getNalUnitType() == NAL_UNIT_CODED_SLICE_IDR || pcCU->getSlice()->getNalUnitType() == NAL_UNIT_CODED_SLICE_CRA);
  if(bDepthMapDetect && !bIntraSliceDetect && !rapPic && sps->getUseQTLPC() && pcCU->getPic()->getReduceBitsFlag())
#else
  if(bDepthMapDetect && !bIntraSliceDetect && sps->getUseQTLPC() && pcCU->getPic()->getReduceBitsFlag())
#endif
  {
    TComDataCU *pcTextureCU = pcTexture->getCU(pcCU->getAddr());
    UInt uiCUIdx            = (pcCU->getZorderIdxInCU() == 0) ? uiAbsPartIdx : pcCU->getZorderIdxInCU();
    assert(pcTextureCU->getDepth(uiCUIdx) >= uiDepth);
    bCodeSplitFlag          = (pcTextureCU->getDepth(uiCUIdx) > uiDepth);
  }

  if(!bCodeSplitFlag)
  {
    assert(uiCurrSplitFlag == 0);
    return;
  }
#endif

  m_pcBinIf->encodeBin( uiCurrSplitFlag, m_cCUSplitFlagSCModel.get( 0, 0, uiCtx ) );
  DTRACE_CABAC_VL( g_nSymbolCounter++ )
  DTRACE_CABAC_T( "\tSplitFlag\n" )
  return;
}

Void TEncSbac::codeTransformSubdivFlag( UInt uiSymbol, UInt uiCtx )
{
  m_pcBinIf->encodeBin( uiSymbol, m_cCUTransSubdivFlagSCModel.get( 0, 0, uiCtx ) );
  DTRACE_CABAC_VL( g_nSymbolCounter++ )
  DTRACE_CABAC_T( "\tparseTransformSubdivFlag()" )
  DTRACE_CABAC_T( "\tsymbol=" )
  DTRACE_CABAC_V( uiSymbol )
  DTRACE_CABAC_T( "\tctx=" )
  DTRACE_CABAC_V( uiCtx )
  DTRACE_CABAC_T( "\n" )
}

#if LGE_EDGE_INTRA_A0070
Void TEncSbac::xCodeEdgeIntraInfo( TComDataCU* pcCU, UInt uiPartIdx )
{
  UInt   uiDepth        = pcCU->getDepth( uiPartIdx ) + (pcCU->getPartitionSize( uiPartIdx ) == SIZE_NxN ? 1 : 0);
  UInt   uiCtxEdgeIntra = pcCU->getCtxEdgeIntra( uiPartIdx );
  UChar* pucSymbolList  = pcCU->getEdgeCode( uiPartIdx );
  UChar  ucEdgeNumber   = pcCU->getEdgeNumber( uiPartIdx );
  Bool   bLeft          = pcCU->getEdgeLeftFirst( uiPartIdx );
  UChar  ucStart        = pcCU->getEdgeStartPos( uiPartIdx );
  UInt   uiSymbol;

  // 1. Top(0) or Left(1)
  uiSymbol = (bLeft == false) ? 0 : 1;
  m_pcBinIf->encodeBinEP( uiSymbol );

  // 2. Start position (lowest bit first)
  uiSymbol = ucStart;
  for( UInt ui = 6; ui > uiDepth; ui-- ) // 64(0)->6, 32(1)->5, 16(2)->4, 8(3)->3, 4(4)->2
  {
    m_pcBinIf->encodeBinEP( uiSymbol & 0x1 );
    uiSymbol >>= 1;
  }

  // 3. Number of edges
  uiSymbol = ucEdgeNumber > 0 ? ucEdgeNumber - 1 : 0;
  for( UInt ui = 7; ui > uiDepth; ui-- ) // 64(0)->7, 32(1)->6, 16(2)->5, 8(3)->4, 4(4)->3
  {
    m_pcBinIf->encodeBinEP( uiSymbol & 0x1 );
    uiSymbol >>= 1;
  }

  if(uiSymbol != 0)
  {
    printf(" ucEdgeNumber %d at depth %d\n",ucEdgeNumber, uiDepth);
    assert(false);
  }

  // 4. Edges
  for( Int iPtr2 = 0; iPtr2 < ucEdgeNumber; iPtr2++ )
  {
    UInt uiReorderSymbol = pucSymbolList[iPtr2];

    //printf ("Ptr = %d, Symbol = %d\n", iPtr2, uiSymbol);

    // Left-friendly direction (Top start)
    // 0 (   0deg) => 0
    // 1 (  45deg) => 10
    // 2 ( -45deg) => 110
    // 3 (  90deg) => 1110
    // 4 ( -90deg) => 11110
    // 5 ( 135deg) => 111110
    // 6 (-135deg) => 111111
    // Right-friendly direction (Left start)
    // 0 (   0deg) => 0
    // 2 ( -45deg) => 10
    // 1 (  45deg) => 110
    // 4 ( -90deg) => 1110
    // 3 (  90deg) => 11110
    // 6 (-135deg) => 111110
    // 5 ( 135deg) => 111111

    // refer to a paper "An efficient chain code with Huffman coding"

    for( UInt ui = 0; ui < uiReorderSymbol; ui++ )
    {
      m_pcBinIf->encodeBin( 1, m_cEdgeIntraSCModel.get( 0, 0, uiCtxEdgeIntra ) );
    }

    if( uiReorderSymbol != 6 )
      m_pcBinIf->encodeBin( 0, m_cEdgeIntraSCModel.get( 0, 0, uiCtxEdgeIntra ) );
  }
}
#endif

Void TEncSbac::codeIntraDirLumaAng( TComDataCU* pcCU, UInt uiAbsPartIdx )
{
  UInt uiDir         = pcCU->getLumaIntraDir( uiAbsPartIdx );

#if HHI_DMM_WEDGE_INTRA || HHI_DMM_PRED_TEX
  if( pcCU->getSlice()->getSPS()->getUseDMM() && pcCU->getWidth( uiAbsPartIdx ) <= DMM_WEDGEMODEL_MAX_SIZE )
  {
#if LGE_EDGE_INTRA_A0070
    m_pcBinIf->encodeBin( uiDir >= NUM_INTRA_MODE && uiDir < EDGE_INTRA_IDX, m_cDmmFlagSCModel.get(0, 0, 0) );
#else
    m_pcBinIf->encodeBin( uiDir >= NUM_INTRA_MODE, m_cDmmFlagSCModel.get(0, 0, 0) );
#endif
  }
#if LGE_EDGE_INTRA_A0070
  if( uiDir >= NUM_INTRA_MODE && uiDir < EDGE_INTRA_IDX )
#else
  if( uiDir >= NUM_INTRA_MODE )
#endif
  {
    assert( pcCU->getWidth( uiAbsPartIdx ) <= DMM_WEDGEMODEL_MAX_SIZE );
    UInt uiDMMode = uiDir - NUM_INTRA_MODE;

#if HHI_DMM_WEDGE_INTRA && HHI_DMM_PRED_TEX
    m_pcBinIf->encodeBin( (uiDMMode & 0x01),      m_cDmmModeSCModel.get(0, 0, 0) );
    m_pcBinIf->encodeBin( (uiDMMode & 0x02) >> 1, m_cDmmModeSCModel.get(0, 0, 0) );

    if( pcCU->getPartitionSize( uiAbsPartIdx ) != SIZE_NxN && pcCU->getWidth( uiAbsPartIdx ) > 4 )
    {
      m_pcBinIf->encodeBin( (uiDMMode & 0x04) >> 2, m_cDmmModeSCModel.get(0, 0, 0) );
    }
#else
    m_pcBinIf->encodeBin( (uiDMMode & 0x01),      m_cDmmModeSCModel.get(0, 0, 0) );

    if( pcCU->getPartitionSize( uiAbsPartIdx ) != SIZE_NxN && pcCU->getWidth( uiAbsPartIdx ) > 4 )
    {
      m_pcBinIf->encodeBin( (uiDMMode & 0x02) >> 1, m_cDmmModeSCModel.get(0, 0, 0) );
    }
#endif
#if HHI_DMM_WEDGE_INTRA
    if( uiDir == DMM_WEDGE_FULL_IDX )          { xCodeWedgeFullInfo          ( pcCU, uiAbsPartIdx ); }
    if( uiDir == DMM_WEDGE_FULL_D_IDX )        { xCodeWedgeFullDeltaInfo     ( pcCU, uiAbsPartIdx ); }
    if( uiDir == DMM_WEDGE_PREDDIR_IDX )       { xCodeWedgePredDirInfo       ( pcCU, uiAbsPartIdx ); }
    if( uiDir == DMM_WEDGE_PREDDIR_D_IDX )     { xCodeWedgePredDirDeltaInfo  ( pcCU, uiAbsPartIdx ); }
#endif
#if HHI_DMM_PRED_TEX

#if FLEX_CODING_ORDER_M23723
    if ( !pcCU->getSlice()->getSPS()->getUseDMM34() )
    {
      assert( uiDir != DMM_WEDGE_PREDTEX_D_IDX );
      assert( uiDir != DMM_CONTOUR_PREDTEX_D_IDX );
    }
#endif

    if( uiDir == DMM_WEDGE_PREDTEX_D_IDX )     { xCodeWedgePredTexDeltaInfo  ( pcCU, uiAbsPartIdx ); }
    if( uiDir == DMM_CONTOUR_PREDTEX_D_IDX )   { xCodeContourPredTexDeltaInfo( pcCU, uiAbsPartIdx ); }
#endif
  }
  else
#if LGE_EDGE_INTRA_A0070
    if ( uiDir >= EDGE_INTRA_IDX)
    {
      m_pcBinIf->encodeBin( 0, m_cCUIntraPredSCModel.get( 0, 0, 0 ) );
      m_pcBinIf->encodeBinsEP( 63, 6 );
      xCodeEdgeIntraInfo( pcCU, uiAbsPartIdx );
#if LGE_EDGE_INTRA_DELTA_DC
      m_pcBinIf->encodeBin( (uiDir == EDGE_INTRA_DELTA_IDX), m_cEdgeIntraDeltaDCSCModel.get(0, 0, 0) );
      if( uiDir == EDGE_INTRA_DELTA_IDX )
      {
        Int iDeltaDC0 = pcCU->getEdgeDeltaDC0( uiAbsPartIdx );
        Int iDeltaDC1 = pcCU->getEdgeDeltaDC1( uiAbsPartIdx );

        xWriteExGolombLevel( UInt( abs( iDeltaDC0 ) ), m_cEdgeIntraDeltaDCSCModel.get(0, 0, 1) );
        if ( iDeltaDC0 != 0 )
        {
          UInt uiSign = iDeltaDC0 > 0 ? 0 : 1;
          m_pcBinIf->encodeBinEP( uiSign );
        }
        xWriteExGolombLevel( UInt( abs( iDeltaDC1 ) ), m_cEdgeIntraDeltaDCSCModel.get(0, 0, 1) );
        if ( iDeltaDC1 != 0 )
        {
          UInt uiSign = iDeltaDC1 > 0 ? 0 : 1;
          m_pcBinIf->encodeBinEP( uiSign );
        }
      }
#endif
    }
    else
#endif // LGE_EDGE_INTRA
  {
#endif
#if !LOGI_INTRA_NAME_3MPM
  Int iIntraIdx = pcCU->getIntraSizeIdx(uiAbsPartIdx);
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
  
#if LOGI_INTRA_NAME_3MPM
  Int uiPreds[3] = {-1, -1, -1};
#else
  Int uiPreds[2] = {-1, -1};
#endif
  Int uiPredNum = pcCU->getIntraDirLumaPredictor(uiAbsPartIdx, uiPreds);  

  Int uiPredIdx = -1;

  for(UInt i = 0; i < uiPredNum; i++)
  {
    if(uiDir == uiPreds[i])
    {
      uiPredIdx = i;
    }
  }
 
  if(uiPredIdx != -1)
  {
    m_pcBinIf->encodeBin( 1, m_cCUIntraPredSCModel.get( 0, 0, 0 ) );
#if LOGI_INTRA_NAME_3MPM
    m_pcBinIf->encodeBinEP( uiPredIdx ? 1 : 0 );
    if (uiPredIdx)
    {
      m_pcBinIf->encodeBinEP( uiPredIdx-1 );
    }
#else
    m_pcBinIf->encodeBinEP( uiPredIdx );
#endif
  }
  else
  {
    m_pcBinIf->encodeBin( 0, m_cCUIntraPredSCModel.get( 0, 0, 0 ) );
  
#if LOGI_INTRA_NAME_3MPM
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
#endif

    for(Int i = (uiPredNum - 1); i >= 0; i--)
    {
      uiDir = uiDir > uiPreds[i] ? uiDir - 1 : uiDir;
    }

#if LOGI_INTRA_NAME_3MPM
    m_pcBinIf->encodeBinsEP( uiDir, 5 );
#if LGE_EDGE_INTRA_A0070
  if (bCodeEdgeIntra)
    if (uiDir == 31) m_pcBinIf->encodeBinsEP(0,1);
#endif
#else
    if ( uiDir < 31 )
    {
      m_pcBinIf->encodeBinsEP( uiDir, g_aucIntraModeBitsAng[ iIntraIdx ] - 1 );
    }
    else
    {
      m_pcBinIf->encodeBinsEP( 31, 5 );
      m_pcBinIf->encodeBinEP( uiDir - 31 );
    }
#endif
   }
#if HHI_DMM_WEDGE_INTRA || HHI_DMM_PRED_TEX
  }
#endif
  return;
}

Void TEncSbac::codeIntraDirChroma( TComDataCU* pcCU, UInt uiAbsPartIdx )
{
  UInt uiIntraDirChroma = pcCU->getChromaIntraDir( uiAbsPartIdx );

  if( uiIntraDirChroma == DM_CHROMA_IDX ) 
  {
    m_pcBinIf->encodeBin( 0, m_cCUChromaPredSCModel.get( 0, 0, 0 ) );
  } 
  else if( uiIntraDirChroma == LM_CHROMA_IDX )
  {
    m_pcBinIf->encodeBin( 1, m_cCUChromaPredSCModel.get( 0, 0, 0 ) );
    m_pcBinIf->encodeBin( 0, m_cCUChromaPredSCModel.get( 0, 0, 1 ) );
  }
  else
  { 
    UInt uiAllowedChromaDir[ NUM_CHROMA_MODE ];
    pcCU->getAllowedChromaDir( uiAbsPartIdx, uiAllowedChromaDir );

    for( Int i = 0; i < NUM_CHROMA_MODE - 2; i++ )
    {
      if( uiIntraDirChroma == uiAllowedChromaDir[i] )
      {
        uiIntraDirChroma = i;
        break;
      }
    }
    m_pcBinIf->encodeBin( 1, m_cCUChromaPredSCModel.get( 0, 0, 0 ) );

    if (pcCU->getSlice()->getSPS()->getUseLMChroma())
    {
      m_pcBinIf->encodeBin( 1, m_cCUChromaPredSCModel.get( 0, 0, 1 ));
    }
#if  CHROMA_MODE_CODING
    m_pcBinIf->encodeBinsEP( uiIntraDirChroma, 2 );
#else
    xWriteUnaryMaxSymbol( uiIntraDirChroma, m_cCUChromaPredSCModel.get( 0, 0 ) + 1, 0, 3 );
#endif
  }
  return;
}

Void TEncSbac::codeInterDir( TComDataCU* pcCU, UInt uiAbsPartIdx )
{
  const UInt uiInterDir = pcCU->getInterDir( uiAbsPartIdx ) - 1;
  const UInt uiCtx      = pcCU->getCtxInterDir( uiAbsPartIdx );
  ContextModel *pCtx    = m_cCUInterDirSCModel.get( 0 );
  m_pcBinIf->encodeBin( uiInterDir == 2 ? 1 : 0, *( pCtx + uiCtx ) );

  return;
}

Void TEncSbac::codeRefFrmIdx( TComDataCU* pcCU, UInt uiAbsPartIdx, RefPicList eRefList )
{
  if ( pcCU->getSlice()->getNumRefIdx(REF_PIC_LIST_C) > 0 && pcCU->getInterDir( uiAbsPartIdx ) != 3)
  {
    Int iRefFrame = pcCU->getSlice()->getRefIdxOfLC(eRefList, pcCU->getCUMvField( eRefList )->getRefIdx( uiAbsPartIdx ));

    ContextModel *pCtx = m_cCURefPicSCModel.get( 0 );
    m_pcBinIf->encodeBin( ( iRefFrame == 0 ? 0 : 1 ), *pCtx );

    if( iRefFrame > 0 )
    {
      xWriteUnaryMaxSymbol( iRefFrame - 1, pCtx + 1, 1, pcCU->getSlice()->getNumRefIdx( REF_PIC_LIST_C )-2 );
    }
  }
  else
  {
    Int iRefFrame = pcCU->getCUMvField( eRefList )->getRefIdx( uiAbsPartIdx );
    ContextModel *pCtx = m_cCURefPicSCModel.get( 0 );
    m_pcBinIf->encodeBin( ( iRefFrame == 0 ? 0 : 1 ), *pCtx );
    
    if( iRefFrame > 0 )
    {
      xWriteUnaryMaxSymbol( iRefFrame - 1, pCtx + 1, 1, pcCU->getSlice()->getNumRefIdx( eRefList )-2 );
    }
  }
  return;
}

Void TEncSbac::codeMvd( TComDataCU* pcCU, UInt uiAbsPartIdx, RefPicList eRefList )
{
#if H0111_MVD_L1_ZERO
  if(pcCU->getSlice()->getMvdL1ZeroFlag() && eRefList == REF_PIC_LIST_1 && pcCU->getInterDir(uiAbsPartIdx)==3)
  {
    return;
  }
#endif

  const TComCUMvField* pcCUMvField = pcCU->getCUMvField( eRefList );
  const Int iHor = pcCUMvField->getMvd( uiAbsPartIdx ).getHor();
  const Int iVer = pcCUMvField->getMvd( uiAbsPartIdx ).getVer();
  ContextModel* pCtx = m_cCUMvdSCModel.get( 0 );

  m_pcBinIf->encodeBin( iHor != 0 ? 1 : 0, *pCtx );
  m_pcBinIf->encodeBin( iVer != 0 ? 1 : 0, *pCtx );

  const Bool bHorAbsGr0 = iHor != 0;
  const Bool bVerAbsGr0 = iVer != 0;
  const UInt uiHorAbs   = 0 > iHor ? -iHor : iHor;
  const UInt uiVerAbs   = 0 > iVer ? -iVer : iVer;
  pCtx++;

  if( bHorAbsGr0 )
  {
    m_pcBinIf->encodeBin( uiHorAbs > 1 ? 1 : 0, *pCtx );
  }

  if( bVerAbsGr0 )
  {
    m_pcBinIf->encodeBin( uiVerAbs > 1 ? 1 : 0, *pCtx );
  }

  if( bHorAbsGr0 )
  {
    if( uiHorAbs > 1 )
    {
      xWriteEpExGolomb( uiHorAbs-2, 1 );
    }

    m_pcBinIf->encodeBinEP( 0 > iHor ? 1 : 0 );
  }

  if( bVerAbsGr0 )
  {
    if( uiVerAbs > 1 )
    {
      xWriteEpExGolomb( uiVerAbs-2, 1 );
    }

    m_pcBinIf->encodeBinEP( 0 > iVer ? 1 : 0 );
  }
  
  return;
}

Void TEncSbac::codeDeltaQP( TComDataCU* pcCU, UInt uiAbsPartIdx )
{
  Int iDQp  = pcCU->getQP( uiAbsPartIdx ) - pcCU->getRefQP( uiAbsPartIdx );
  
#if H0736_AVC_STYLE_QP_RANGE
  Int qpBdOffsetY =  pcCU->getSlice()->getSPS()->getQpBDOffsetY();
  iDQp = (iDQp + 78 + qpBdOffsetY + (qpBdOffsetY/2)) % (52 + qpBdOffsetY) - 26 - (qpBdOffsetY/2);
#else
#if LOSSLESS_CODING
  if(pcCU->getSlice()->getSPS()->getUseLossless())
  {
    if(iDQp > 25)
    {
      iDQp = iDQp - 52;
    }
    if(iDQp < -26)
    {
      iDQp = iDQp + 52;
    }
  }
#endif
#endif

  if ( iDQp == 0 )
  {
    m_pcBinIf->encodeBin( 0, m_cCUDeltaQpSCModel.get( 0, 0, 0 ) );
  }
  else
  {
    m_pcBinIf->encodeBin( 1, m_cCUDeltaQpSCModel.get( 0, 0, 0 ) );
   
    UInt uiSign = (iDQp > 0 ? 0 : 1);
#if !H0736_AVC_STYLE_QP_RANGE
    UInt uiQpBdOffsetY = 6*(g_uiBitIncrement + g_uiBitDepth - 8);
#endif

    m_pcBinIf->encodeBinEP(uiSign);

#if H0736_AVC_STYLE_QP_RANGE
    assert(iDQp >= -(26+(qpBdOffsetY/2)));
    assert(iDQp <=  (25+(qpBdOffsetY/2)));

    UInt uiMaxAbsDQpMinus1 = 24 + (qpBdOffsetY/2) + (uiSign);
#else
    assert(iDQp >= -(26+(Int)(uiQpBdOffsetY/2)));
    assert(iDQp <=  (25+(Int)(uiQpBdOffsetY/2)));

    UInt uiMaxAbsDQpMinus1 = 24 + (uiQpBdOffsetY/2) + (uiSign);
#endif
    UInt uiAbsDQpMinus1 = (UInt)((iDQp > 0)? iDQp  : (-iDQp)) - 1;
    xWriteUnaryMaxSymbol( uiAbsDQpMinus1, &m_cCUDeltaQpSCModel.get( 0, 0, 1 ), 1, uiMaxAbsDQpMinus1);
  }
  
  return;
}

Void TEncSbac::codeQtCbf( TComDataCU* pcCU, UInt uiAbsPartIdx, TextType eType, UInt uiTrDepth )
{
  UInt uiCbf = pcCU->getCbf     ( uiAbsPartIdx, eType, uiTrDepth );
  UInt uiCtx = pcCU->getCtxQtCbf( uiAbsPartIdx, eType, uiTrDepth );
  m_pcBinIf->encodeBin( uiCbf , m_cCUQtCbfSCModel.get( 0, eType ? TEXT_CHROMA : eType, uiCtx ) );
  DTRACE_CABAC_VL( g_nSymbolCounter++ )
  DTRACE_CABAC_T( "\tparseQtCbf()" )
  DTRACE_CABAC_T( "\tsymbol=" )
  DTRACE_CABAC_V( uiCbf )
  DTRACE_CABAC_T( "\tctx=" )
  DTRACE_CABAC_V( uiCtx )
  DTRACE_CABAC_T( "\tetype=" )
  DTRACE_CABAC_V( eType )
  DTRACE_CABAC_T( "\tuiAbsPartIdx=" )
  DTRACE_CABAC_V( uiAbsPartIdx )
  DTRACE_CABAC_T( "\n" )
}

#if BURST_IPCM
/** Code I_PCM information. 
 * \param pcCU pointer to CU
 * \param uiAbsPartIdx CU index
 * \param numIPCM the number of succesive IPCM blocks with the same size 
 * \param firstIPCMFlag 
 * \returns Void
 */
Void TEncSbac::codeIPCMInfo( TComDataCU* pcCU, UInt uiAbsPartIdx, Int numIPCM, Bool firstIPCMFlag)
#else
/** Code I_PCM information. 
 * \param pcCU pointer to CU
 * \param uiAbsPartIdx CU index
 * \returns Void
 *
 * If I_PCM flag indicates that the CU is I_PCM, code its PCM alignment bits and codes.  
 */
Void TEncSbac::codeIPCMInfo( TComDataCU* pcCU, UInt uiAbsPartIdx)
#endif
{
  UInt uiIPCM = (pcCU->getIPCMFlag(uiAbsPartIdx) == true)? 1 : 0;

#if BURST_IPCM
  Bool writePCMSampleFlag = pcCU->getIPCMFlag(uiAbsPartIdx);

  if( uiIPCM == 0 || firstIPCMFlag)
  {
    m_pcBinIf->encodeBinTrm (uiIPCM);

    if ( firstIPCMFlag )
    {
      m_pcBinIf->encodeNumSubseqIPCM( numIPCM - 1 );
      m_pcBinIf->encodePCMAlignBits();
    }
  }
#else
  m_pcBinIf->encodeBinTrm (uiIPCM);
#endif

#if BURST_IPCM
  if (writePCMSampleFlag)
#else
  if (uiIPCM)
#endif
  {
#if !BURST_IPCM
    m_pcBinIf->encodePCMAlignBits();
#endif
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
        UInt uiSample = piPCMSample[uiX];

        m_pcBinIf->xWritePCMCode(uiSample, uiSampleBits);
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
        UInt uiSample = piPCMSample[uiX];

        m_pcBinIf->xWritePCMCode(uiSample, uiSampleBits);
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
        UInt uiSample = piPCMSample[uiX];

        m_pcBinIf->xWritePCMCode(uiSample, uiSampleBits);
      }
      piPCMSample += uiWidth;
    }
#if BURST_IPCM
    numIPCM--;
    if(numIPCM == 0)
    {
      m_pcBinIf->resetBac();
    }
#else
    m_pcBinIf->resetBac();
#endif
  }
}

Void TEncSbac::codeQtRootCbf( TComDataCU* pcCU, UInt uiAbsPartIdx )
{
  UInt uiCbf = pcCU->getQtRootCbf( uiAbsPartIdx );
  UInt uiCtx = 0;
  m_pcBinIf->encodeBin( uiCbf , m_cCUQtRootCbfSCModel.get( 0, 0, uiCtx ) );
  DTRACE_CABAC_VL( g_nSymbolCounter++ )
  DTRACE_CABAC_T( "\tparseQtRootCbf()" )
  DTRACE_CABAC_T( "\tsymbol=" )
  DTRACE_CABAC_V( uiCbf )
  DTRACE_CABAC_T( "\tctx=" )
  DTRACE_CABAC_V( uiCtx )
  DTRACE_CABAC_T( "\tuiAbsPartIdx=" )
  DTRACE_CABAC_V( uiAbsPartIdx )
  DTRACE_CABAC_T( "\n" )
}

/** Encode (X,Y) position of the last significant coefficient
 * \param uiPosX X component of last coefficient
 * \param uiPosY Y component of last coefficient
 * \param width  Block width
 * \param height Block height
 * \param eTType plane type / luminance or chrominance
 * \param uiScanIdx scan type (zig-zag, hor, ver)
 * This method encodes the X and Y component within a block of the last significant coefficient.
 */
Void TEncSbac::codeLastSignificantXY( UInt uiPosX, UInt uiPosY, Int width, Int height, TextType eTType, UInt uiScanIdx )
{  
  // swap
  if( uiScanIdx == SCAN_VER )
  {
    swap( uiPosX, uiPosY );
  }

  UInt uiCtxLast;
  ContextModel *pCtxX = m_cCuCtxLastX.get( 0, eTType );
  ContextModel *pCtxY = m_cCuCtxLastY.get( 0, eTType );
  UInt uiGroupIdxX    = g_uiGroupIdx[ uiPosX ];
  UInt uiGroupIdxY    = g_uiGroupIdx[ uiPosY ];

  // posX
#if LAST_CTX_REDUCTION
  Int widthCtx = eTType? 4: width;
  const UInt *puiCtxIdxX = g_uiLastCtx + ( g_aucConvertToBit[ widthCtx ] * ( g_aucConvertToBit[ widthCtx ] + 3 ) );
#else
  const UInt *puiCtxIdxX = g_uiLastCtx + ( g_aucConvertToBit[ width ] * ( g_aucConvertToBit[ width ] + 3 ) );
#endif
  for( uiCtxLast = 0; uiCtxLast < uiGroupIdxX; uiCtxLast++ )
  {
#if LAST_CTX_REDUCTION
    if (eTType)
    {
      m_pcBinIf->encodeBin( 1, *( pCtxX + (uiCtxLast>>g_aucConvertToBit[ width ]) ) );
    }
    else
    {
#endif
      m_pcBinIf->encodeBin( 1, *( pCtxX + puiCtxIdxX[ uiCtxLast ] ) );
#if LAST_CTX_REDUCTION
    }
#endif
  }
  if( uiGroupIdxX < g_uiGroupIdx[ width - 1 ])
  {
#if LAST_CTX_REDUCTION
    if ( eTType )
    {
      m_pcBinIf->encodeBin( 0, *( pCtxX + (uiCtxLast>>g_aucConvertToBit[ width ]) ) );
    }
    else
    {
#endif
      m_pcBinIf->encodeBin( 0, *( pCtxX + puiCtxIdxX[ uiCtxLast ] ) );
#if LAST_CTX_REDUCTION
    }
#endif
  }

  // posY
#if LAST_CTX_REDUCTION
  Int heightCtx = eTType? 4: height;
  const UInt *puiCtxIdxY = g_uiLastCtx + ( g_aucConvertToBit[ heightCtx ] * ( g_aucConvertToBit[ heightCtx ] + 3 ) );
#else
  const UInt *puiCtxIdxY = g_uiLastCtx + ( g_aucConvertToBit[ height ] * ( g_aucConvertToBit[ height ] + 3 ) );
#endif
  for( uiCtxLast = 0; uiCtxLast < uiGroupIdxY; uiCtxLast++ )
  {
#if LAST_CTX_REDUCTION
    if (eTType)
    {
      m_pcBinIf->encodeBin( 1, *( pCtxY + (uiCtxLast>>g_aucConvertToBit[ height ])));
    }
    else
    {
#endif
      m_pcBinIf->encodeBin( 1, *( pCtxY + puiCtxIdxY[ uiCtxLast ] ) );
#if LAST_CTX_REDUCTION
    }
#endif
  }
  if( uiGroupIdxY < g_uiGroupIdx[ height - 1 ])
  {
#if LAST_CTX_REDUCTION
    if (eTType)
    {
      m_pcBinIf->encodeBin( 0, *( pCtxY + (uiCtxLast>>g_aucConvertToBit[ height ]) ) );
    }
    else
    {
#endif
      m_pcBinIf->encodeBin( 0, *( pCtxY + puiCtxIdxY[ uiCtxLast ] ) );
#if LAST_CTX_REDUCTION
    }
#endif
    }
  if ( uiGroupIdxX > 3 )
  {      
    UInt uiCount = ( uiGroupIdxX - 2 ) >> 1;
    uiPosX       = uiPosX - g_uiMinInGroup[ uiGroupIdxX ];
    for (Int i = uiCount - 1 ; i >= 0; i-- )
    {
      m_pcBinIf->encodeBinEP( ( uiPosX >> i ) & 1 );
    }
  }
  if ( uiGroupIdxY > 3 )
  {      
    UInt uiCount = ( uiGroupIdxY - 2 ) >> 1;
    uiPosY       = uiPosY - g_uiMinInGroup[ uiGroupIdxY ];
    for ( Int i = uiCount - 1 ; i >= 0; i-- )
    {
      m_pcBinIf->encodeBinEP( ( uiPosY >> i ) & 1 );
    }
  }
}

Void TEncSbac::codeCoeffNxN( TComDataCU* pcCU, TCoeff* pcCoef, UInt uiAbsPartIdx, UInt uiWidth, UInt uiHeight, UInt uiDepth, TextType eTType )
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

  if( uiWidth > m_pcSlice->getSPS()->getMaxTrSize() )
  {
    uiWidth  = m_pcSlice->getSPS()->getMaxTrSize();
    uiHeight = m_pcSlice->getSPS()->getMaxTrSize();
  }
  
  UInt uiNumSig = 0;
  
  // compute number of significant coefficients
  uiNumSig = TEncEntropy::countNonZeroCoeffs(pcCoef, uiWidth * uiHeight);
  
  if ( uiNumSig == 0 )
    return;
  
  eTType = eTType == TEXT_LUMA ? TEXT_LUMA : ( eTType == TEXT_NONE ? TEXT_NONE : TEXT_CHROMA );
  
  //----- encode significance map -----
  const UInt   uiLog2BlockSize   = g_aucConvertToBit[ uiWidth ] + 2;
  UInt uiScanIdx = pcCU->getCoefScanIdx(uiAbsPartIdx, uiWidth, eTType==TEXT_LUMA, pcCU->isIntra(uiAbsPartIdx));
  if (uiScanIdx == SCAN_ZIGZAG)
  {
    // Map zigzag to diagonal scan
    uiScanIdx = SCAN_DIAG;
  }
  Int blockType = uiLog2BlockSize;
  if (uiWidth != uiHeight)
  {
    uiScanIdx = SCAN_DIAG;
    blockType = 4;
  }
  
  const UInt * scan;
  if (uiWidth == uiHeight)
  {
    scan = g_auiSigLastScan[ uiScanIdx ][ uiLog2BlockSize - 1 ];
  }
  else
  {
    scan = g_sigScanNSQT[ uiLog2BlockSize - 2 ];
  }
  
#if MULTIBITS_DATA_HIDING
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
#endif

  // Find position of last coefficient
  Int scanPosLast = -1;
  Int posLast;

  const UInt * scanCG;
  if (uiWidth == uiHeight)
  {
    scanCG = g_auiSigLastScan[ uiScanIdx ][ uiLog2BlockSize > 3 ? uiLog2BlockSize-2-1 : 0 ];
#if MULTILEVEL_SIGMAP_EXT
    if( uiLog2BlockSize == 3 )
    {
      scanCG = g_sigLastScan8x8[ uiScanIdx ];
    }
    else if( uiLog2BlockSize == 5 )
    {
      scanCG = g_sigLastScanCG32x32;
    }
#endif
  }
  else
  {
    scanCG = g_sigCGScanNSQT[ uiLog2BlockSize - 2 ];
  }
  UInt uiSigCoeffGroupFlag[ MLS_GRP_NUM ];
  static const UInt uiShift = MLS_CG_SIZE >> 1;
  const UInt uiNumBlkSide = uiWidth >> uiShift;

#if !MULTILEVEL_SIGMAP_EXT
  if( blockType > 3 )
  {
#endif
    ::memset( uiSigCoeffGroupFlag, 0, sizeof(UInt) * MLS_GRP_NUM );

    do
    {
      posLast = scan[ ++scanPosLast ];

      // get L1 sig map
      UInt uiPosY    = posLast >> uiLog2BlockSize;
      UInt uiPosX    = posLast - ( uiPosY << uiLog2BlockSize );
      UInt uiBlkIdx  = uiNumBlkSide * (uiPosY >> uiShift) + (uiPosX >> uiShift);
#if MULTILEVEL_SIGMAP_EXT
      if( uiWidth == 8 && uiHeight == 8 && (uiScanIdx == SCAN_HOR || uiScanIdx == SCAN_VER) )
      {
        if( uiScanIdx == SCAN_HOR )
        {
          uiBlkIdx = uiPosY >> 1;
        }
        else if( uiScanIdx == SCAN_VER )
        {
          uiBlkIdx = uiPosX >> 1;
        }
      }
#endif
      if( pcCoef[ posLast ] )
      {
        uiSigCoeffGroupFlag[ uiBlkIdx ] = 1;
      }

      uiNumSig -= ( pcCoef[ posLast ] != 0 );
    }
    while ( uiNumSig > 0 );
#if !MULTILEVEL_SIGMAP_EXT
  }
  else
  {
  
  do
  {
    posLast = scan[ ++scanPosLast ];
    uiNumSig -= ( pcCoef[ posLast ] != 0 );
  }
  while ( uiNumSig > 0 );

  }
#endif

  // Code position of last coefficient
  Int posLastY = posLast >> uiLog2BlockSize;
  Int posLastX = posLast - ( posLastY << uiLog2BlockSize );
  codeLastSignificantXY(posLastX, posLastY, uiWidth, uiHeight, eTType, uiScanIdx);
  
  //===== code significance flag =====
  ContextModel * const baseCoeffGroupCtx = m_cCUSigCoeffGroupSCModel.get( 0, eTType );
  ContextModel * const baseCtx = (eTType==TEXT_LUMA) ? m_cCUSigSCModel.get( 0, 0 ) : m_cCUSigSCModel.get( 0, 0 ) + NUM_SIG_FLAG_CTX_LUMA;


  const Int  iLastScanSet      = scanPosLast >> LOG2_SCAN_SET_SIZE;
  UInt uiNumOne                = 0;
  UInt uiGoRiceParam           = 0;
  Int  iScanPosSig             = scanPosLast;

  for( Int iSubSet = iLastScanSet; iSubSet >= 0; iSubSet-- )
  {
    Int numNonZero = 0;
    Int  iSubPos     = iSubSet << LOG2_SCAN_SET_SIZE;
    uiGoRiceParam    = 0;
    Int absCoeff[16];
    UInt coeffSigns = 0;

#if MULTIBITS_DATA_HIDING
    Int lastNZPosInCG = -1, firstNZPosInCG = SCAN_SET_SIZE;
#endif

    if( iScanPosSig == scanPosLast )
    {
      absCoeff[ 0 ] = abs( pcCoef[ posLast ] );
      coeffSigns    = ( pcCoef[ posLast ] < 0 );
      numNonZero    = 1;
#if MULTIBITS_DATA_HIDING
      lastNZPosInCG  = iScanPosSig;
      firstNZPosInCG = iScanPosSig;
#endif
      iScanPosSig--;
    }

#if !MULTILEVEL_SIGMAP_EXT
    if( blockType > 3 )
    {
#endif
      // encode significant_coeffgroup_flag
      Int iCGBlkPos = scanCG[ iSubSet ];
      Int iCGPosY   = iCGBlkPos / uiNumBlkSide;
      Int iCGPosX   = iCGBlkPos - (iCGPosY * uiNumBlkSide);
#if MULTILEVEL_SIGMAP_EXT
      if( uiWidth == 8 && uiHeight == 8 && (uiScanIdx == SCAN_HOR || uiScanIdx == SCAN_VER) )
      {
        iCGPosY = (uiScanIdx == SCAN_HOR ? iCGBlkPos : 0);
        iCGPosX = (uiScanIdx == SCAN_VER ? iCGBlkPos : 0);
      }
#endif
#if !REMOVE_INFER_SIGGRP
      Bool bInferredCGFlag = false;
#endif
#if REMOVE_INFER_SIGGRP
      if( iSubSet == iLastScanSet || iSubSet == 0)
#else      
      if( iSubSet == iLastScanSet )
#endif
      {
        uiSigCoeffGroupFlag[ iCGBlkPos ] = 1;
      }
      else
      {
#if !REMOVE_INFER_SIGGRP
#if MULTILEVEL_SIGMAP_EXT
        if( !TComTrQuant::bothCGNeighboursOne( uiSigCoeffGroupFlag, iCGPosX, iCGPosY, uiScanIdx, uiWidth, uiHeight ) && ( iSubSet ) )
#else
        if( !TComTrQuant::bothCGNeighboursOne( uiSigCoeffGroupFlag, iCGPosX, iCGPosY, uiWidth, uiHeight ) && ( iSubSet ) )
#endif
        {
#endif
          UInt uiSigCoeffGroup   = (uiSigCoeffGroupFlag[ iCGBlkPos ] != 0);
#if MULTILEVEL_SIGMAP_EXT
          UInt uiCtxSig  = TComTrQuant::getSigCoeffGroupCtxInc( uiSigCoeffGroupFlag, iCGPosX, iCGPosY, uiScanIdx, uiWidth, uiHeight );
#else
          UInt uiCtxSig  = TComTrQuant::getSigCoeffGroupCtxInc( uiSigCoeffGroupFlag, iCGPosX, iCGPosY, uiWidth, uiHeight );
#endif
          m_pcBinIf->encodeBin( uiSigCoeffGroup, baseCoeffGroupCtx[ uiCtxSig ] );
#if !REMOVE_INFER_SIGGRP
        }
        else
        {
          uiSigCoeffGroupFlag[ iCGBlkPos ] = 1;
          bInferredCGFlag = true;
        }
#endif
      }
      
      // encode significant_coeff_flag
      if( uiSigCoeffGroupFlag[ iCGBlkPos ] )
      {
        UInt uiBlkPos, uiPosY, uiPosX, uiSig, uiCtxSig;
        for( ; iScanPosSig >= iSubPos; iScanPosSig-- )
        {
          uiBlkPos  = scan[ iScanPosSig ]; 
          uiPosY    = uiBlkPos >> uiLog2BlockSize;
          uiPosX    = uiBlkPos - ( uiPosY << uiLog2BlockSize );
          uiSig     = (pcCoef[ uiBlkPos ] != 0);
#if REMOVE_INFER_SIGGRP
          if( iScanPosSig > iSubPos || iSubSet == 0 || numNonZero )
#else
          if( iScanPosSig > iSubPos || bInferredCGFlag || numNonZero )
#endif
          {
            uiCtxSig  = TComTrQuant::getSigCtxInc( pcCoef, uiPosX, uiPosY, blockType, uiWidth, uiHeight, eTType );
            m_pcBinIf->encodeBin( uiSig, baseCtx[ uiCtxSig ] );
          }
          if( uiSig )
          {
            absCoeff[ numNonZero ] = abs( pcCoef[ uiBlkPos ] );
            coeffSigns = 2 * coeffSigns + ( pcCoef[ uiBlkPos ] < 0 );
            numNonZero++;
#if MULTIBITS_DATA_HIDING
            if( lastNZPosInCG == -1 )
            {
              lastNZPosInCG = iScanPosSig;
            }
            firstNZPosInCG = iScanPosSig;
#endif
          }
        }
      }
      else
      {
        iScanPosSig = iSubPos - 1;
      }
#if !MULTILEVEL_SIGMAP_EXT
    } 
    else
    {

    for( ; iScanPosSig >= iSubPos; iScanPosSig-- )
    {
      UInt  uiBlkPos  = scan[ iScanPosSig ]; 
      UInt  uiPosY    = uiBlkPos >> uiLog2BlockSize;
      UInt  uiPosX    = uiBlkPos - ( uiPosY << uiLog2BlockSize );
      UInt  uiSig     = 0; 
      if( pcCoef[ uiBlkPos ] != 0 )
      {
        uiSig = 1;
        absCoeff[ numNonZero ] = abs( pcCoef[ uiBlkPos ] );
        coeffSigns = 2 * coeffSigns + ( pcCoef[ uiBlkPos ] < 0 );
        numNonZero++;
#if MULTIBITS_DATA_HIDING
        if( lastNZPosInCG == -1 )
        {
          lastNZPosInCG = iScanPosSig;
        }
        firstNZPosInCG = iScanPosSig;
#endif
      }      
      UInt  uiCtxSig  = TComTrQuant::getSigCtxInc( pcCoef, uiPosX, uiPosY, blockType, uiWidth, uiHeight, eTType );
      m_pcBinIf->encodeBin( uiSig, baseCtx[ uiCtxSig ] );
    }

    }
#endif

    if( numNonZero > 0 )
    {
#if MULTIBITS_DATA_HIDING
      Bool signHidden = ( lastNZPosInCG - firstNZPosInCG >= (Int)tsig );
#endif  // MULTIBITS_DATA_HIDING

      UInt c1 = 1;
#if !RESTRICT_GR1GR2FLAG_NUMBER
      UInt c2 = 0;
#endif
#if LEVEL_CTX_LUMA_RED
      UInt uiCtxSet = (iSubSet > 0 && eTType==TEXT_LUMA) ? 2 : 0;
#else
      UInt uiCtxSet = (iSubSet > 0 && eTType==TEXT_LUMA) ? 3 : 0;
#endif
      
      if( uiNumOne > 0 )
      {
        uiCtxSet++;
#if !LEVEL_CTX_LUMA_RED
        if( uiNumOne > 3 && eTType==TEXT_LUMA)
        {
          uiCtxSet++;
        }
#endif
      }
      
      uiNumOne       >>= 1;
      ContextModel *baseCtxMod = ( eTType==TEXT_LUMA ) ? m_cCUOneSCModel.get( 0, 0 ) + 4 * uiCtxSet : m_cCUOneSCModel.get( 0, 0 ) + NUM_ONE_FLAG_CTX_LUMA + 4 * uiCtxSet;
      
#if RESTRICT_GR1GR2FLAG_NUMBER
      Int numC1Flag = min(numNonZero, C1FLAG_NUMBER);
      Int firstC2FlagIdx = -1;
      for( Int idx = 0; idx < numC1Flag; idx++ )
#else
      for ( Int idx = 0; idx < numNonZero; idx++ )
#endif
      {
        UInt uiSymbol = absCoeff[ idx ] > 1;
        m_pcBinIf->encodeBin( uiSymbol, baseCtxMod[c1] );
        if( uiSymbol )
        {
          c1 = 0;

#if RESTRICT_GR1GR2FLAG_NUMBER
          if (firstC2FlagIdx == -1)
          {
            firstC2FlagIdx = idx;
          }
#endif
        }
        else if( (c1 < 3) && (c1 > 0) )
        {
          c1++;
        }
      }
      
      if (c1 == 0)
      {

#if RESTRICT_GR1GR2FLAG_NUMBER
        baseCtxMod = ( eTType==TEXT_LUMA ) ? m_cCUAbsSCModel.get( 0, 0 ) + uiCtxSet : m_cCUAbsSCModel.get( 0, 0 ) + NUM_ABS_FLAG_CTX_LUMA + uiCtxSet;
        if ( firstC2FlagIdx != -1)
        {
          UInt symbol = absCoeff[ firstC2FlagIdx ] > 2;
          m_pcBinIf->encodeBin( symbol, baseCtxMod[0] );
        }
#else    
        baseCtxMod = ( eTType==TEXT_LUMA ) ? m_cCUAbsSCModel.get( 0, 0 ) + 3 * uiCtxSet : m_cCUAbsSCModel.get( 0, 0 ) + NUM_ABS_FLAG_CTX_LUMA + 3 * uiCtxSet;
        for ( Int idx = 0; idx < numNonZero; idx++ )
        {
          if( absCoeff[ idx ] > 1 )
          {
            UInt symbol = absCoeff[ idx ] > 2;
            m_pcBinIf->encodeBin( symbol, baseCtxMod[c2] );
            c2 += (c2 < 2);
            uiNumOne++;
          }
        }
#endif
      }
      
#if MULTIBITS_DATA_HIDING
      if( beValid && signHidden )
      {
        m_pcBinIf->encodeBinsEP( (coeffSigns >> 1), numNonZero-1 );
      }
      else
      {
        m_pcBinIf->encodeBinsEP( coeffSigns, numNonZero );
      }
#else
      m_pcBinIf->encodeBinsEP( coeffSigns, numNonZero );
#endif
      
#if RESTRICT_GR1GR2FLAG_NUMBER
      Int iFirstCoeff2 = 1;    
      if (c1 == 0 || numNonZero > C1FLAG_NUMBER)
#else
      if (c1 == 0)
#endif
      {
        for ( Int idx = 0; idx < numNonZero; idx++ )
        {
#if RESTRICT_GR1GR2FLAG_NUMBER
          UInt baseLevel  = (idx < C1FLAG_NUMBER)? (2 + iFirstCoeff2 ) : 1;

          if( absCoeff[ idx ] >= baseLevel)
          {
            xWriteGoRiceExGolomb( absCoeff[ idx ] - baseLevel, uiGoRiceParam ); 
          }
          if(absCoeff[ idx ] >= 2)  
          {
            iFirstCoeff2 = 0;
            uiNumOne++;
          }
#else
          if ( absCoeff[ idx ] > 2 )
          {
            xWriteGoRiceExGolomb( absCoeff[ idx ]  - 3, uiGoRiceParam );            
          }
#endif
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

Void TEncSbac::codeAlfFlag       ( UInt uiCode )
{
  UInt uiSymbol = ( ( uiCode == 0 ) ? 0 : 1 );
  m_pcBinIf->encodeBin( uiSymbol, m_cALFFlagSCModel.get( 0, 0, 0 ) );
}

Void TEncSbac::codeAlfCtrlFlag( UInt uiSymbol )
{
  m_pcBinIf->encodeBin( uiSymbol, m_cCUAlfCtrlFlagSCModel.get( 0, 0, 0) );
}

Void TEncSbac::codeAlfUvlc       ( UInt uiCode )
{
  Int i;
  
  if ( uiCode == 0 )
  {
    m_pcBinIf->encodeBin( 0, m_cALFUvlcSCModel.get( 0, 0, 0 ) );
  }
  else
  {
    m_pcBinIf->encodeBin( 1, m_cALFUvlcSCModel.get( 0, 0, 0 ) );
    for ( i=0; i<uiCode-1; i++ )
    {
      m_pcBinIf->encodeBin( 1, m_cALFUvlcSCModel.get( 0, 0, 1 ) );
    }
    m_pcBinIf->encodeBin( 0, m_cALFUvlcSCModel.get( 0, 0, 1 ) );
  }
}

Void TEncSbac::codeAlfSvlc       ( Int iCode )
{
  Int i;
  
  if ( iCode == 0 )
  {
    m_pcBinIf->encodeBin( 0, m_cALFSvlcSCModel.get( 0, 0, 0 ) );
  }
  else
  {
    m_pcBinIf->encodeBin( 1, m_cALFSvlcSCModel.get( 0, 0, 0 ) );
    
    // write sign
    if ( iCode > 0 )
    {
      m_pcBinIf->encodeBin( 0, m_cALFSvlcSCModel.get( 0, 0, 1 ) );
    }
    else
    {
      m_pcBinIf->encodeBin( 1, m_cALFSvlcSCModel.get( 0, 0, 1 ) );
      iCode = -iCode;
    }
    
    // write magnitude
    for ( i=0; i<iCode-1; i++ )
    {
      m_pcBinIf->encodeBin( 1, m_cALFSvlcSCModel.get( 0, 0, 2 ) );
    }
    m_pcBinIf->encodeBin( 0, m_cALFSvlcSCModel.get( 0, 0, 2 ) );
  }
}

Void TEncSbac::codeSaoFlag       ( UInt uiCode )
{
  UInt uiSymbol = ( ( uiCode == 0 ) ? 0 : 1 );
  m_pcBinIf->encodeBin( uiSymbol, m_cSaoFlagSCModel.get( 0, 0, 0 ) );
}

Void TEncSbac::codeSaoUvlc       ( UInt uiCode )
{
  Int i;

  if ( uiCode == 0 )
  {
    m_pcBinIf->encodeBin( 0, m_cSaoUvlcSCModel.get( 0, 0, 0 ) );
  }
  else
  {
    m_pcBinIf->encodeBin( 1, m_cSaoUvlcSCModel.get( 0, 0, 0 ) );
    for ( i=0; i<uiCode-1; i++ )
    {
      m_pcBinIf->encodeBin( 1, m_cSaoUvlcSCModel.get( 0, 0, 1 ) );
    }
    m_pcBinIf->encodeBin( 0, m_cSaoUvlcSCModel.get( 0, 0, 1 ) );
  }
}

Void TEncSbac::codeSaoSvlc       ( Int iCode )
{
  Int i;

  if ( iCode == 0 )
  {
    m_pcBinIf->encodeBin( 0, m_cSaoSvlcSCModel.get( 0, 0, 0 ) );
  }
  else
  {
    m_pcBinIf->encodeBin( 1, m_cSaoSvlcSCModel.get( 0, 0, 0 ) );

    // write sign
    if ( iCode > 0 )
    {
      m_pcBinIf->encodeBin( 0, m_cSaoSvlcSCModel.get( 0, 0, 1 ) );
    }
    else
    {
      m_pcBinIf->encodeBin( 1, m_cSaoSvlcSCModel.get( 0, 0, 1 ) );
      iCode = -iCode;
    }

    // write magnitude
    for ( i=0; i<iCode-1; i++ )
    {
      m_pcBinIf->encodeBin( 1, m_cSaoSvlcSCModel.get( 0, 0, 2 ) );
    }
    m_pcBinIf->encodeBin( 0, m_cSaoSvlcSCModel.get( 0, 0, 2 ) );
  }
}
#if SAO_UNIT_INTERLEAVING
/** Code SAO band position 
 * \param uiCode
 */
Void TEncSbac::codeSaoUflc       ( UInt uiCode )
{
  for (Int i=0;i<5;i++)
  {
    m_pcBinIf->encodeBinEP ( (uiCode>>i) &0x01 );
  }
}
/** Code SAO merge left flag 
 * \param uiCode
 * \param uiCompIdx
 */
Void TEncSbac::codeSaoMergeLeft       ( UInt uiCode, UInt uiCompIdx )
{
  if (uiCode == 0)
  {
    m_pcBinIf->encodeBin(0,  m_cSaoMergeLeftSCModel.get( 0, 0, uiCompIdx ));
  }
  else
  {
    m_pcBinIf->encodeBin(1,  m_cSaoMergeLeftSCModel.get( 0, 0, uiCompIdx ));
  }
}
/** Code SAO merge up flag 
 * \param uiCode
 */
Void TEncSbac::codeSaoMergeUp       ( UInt uiCode)
{
  if (uiCode == 0)
  {
    m_pcBinIf->encodeBin(0,  m_cSaoMergeUpSCModel.get( 0, 0, 0 ));
  }
  else
  {
    m_pcBinIf->encodeBin(1,  m_cSaoMergeUpSCModel.get( 0, 0, 0 ));
  }
}
/** Code SAO type index 
 * \param uiCode
 */
Void TEncSbac::codeSaoTypeIdx       ( UInt uiCode)
{
  Int i;
  if ( uiCode == 0 )
  {
    m_pcBinIf->encodeBin( 0, m_cSaoTypeIdxSCModel.get( 0, 0, 0 ) );
  }
  else
  {
    m_pcBinIf->encodeBin( 1, m_cSaoTypeIdxSCModel.get( 0, 0, 0 ) );
    for ( i=0; i<uiCode-1; i++ )
    {
      m_pcBinIf->encodeBin( 1, m_cSaoTypeIdxSCModel.get( 0, 0, 1 ) );
    }
    m_pcBinIf->encodeBin( 0, m_cSaoTypeIdxSCModel.get( 0, 0, 1 ) );
  }
}
#endif
/*!
 ****************************************************************************
 * \brief
 *   estimate bit cost for CBP, significant map and significant coefficients
 ****************************************************************************
 */
Void TEncSbac::estBit( estBitsSbacStruct* pcEstBitsSbac, Int width, Int height, TextType eTType )
{
  estCBFBit( pcEstBitsSbac, 0, eTType );

  estSignificantCoeffGroupMapBit( pcEstBitsSbac, 0, eTType );
  
  // encode significance map
  estSignificantMapBit( pcEstBitsSbac, width, height, eTType );
  
  // encode significant coefficients
  estSignificantCoefficientsBit( pcEstBitsSbac, 0, eTType );
}

/*!
 ****************************************************************************
 * \brief
 *    estimate bit cost for each CBP bit
 ****************************************************************************
 */
Void TEncSbac::estCBFBit( estBitsSbacStruct* pcEstBitsSbac, UInt uiCTXIdx, TextType eTType )
{
  ContextModel *pCtx = m_cCUQtCbfSCModel.get( 0 );

  for( UInt uiCtxInc = 0; uiCtxInc < 3*NUM_QT_CBF_CTX; uiCtxInc++ )
  {
    pcEstBitsSbac->blockCbpBits[ uiCtxInc ][ 0 ] = pCtx[ uiCtxInc ].getEntropyBits( 0 );
    pcEstBitsSbac->blockCbpBits[ uiCtxInc ][ 1 ] = pCtx[ uiCtxInc ].getEntropyBits( 1 );
  }

  pCtx = m_cCUQtRootCbfSCModel.get( 0 );
  
  for( UInt uiCtxInc = 0; uiCtxInc < 4; uiCtxInc++ )
  {
    pcEstBitsSbac->blockRootCbpBits[ uiCtxInc ][ 0 ] = pCtx[ uiCtxInc ].getEntropyBits( 0 );
    pcEstBitsSbac->blockRootCbpBits[ uiCtxInc ][ 1 ] = pCtx[ uiCtxInc ].getEntropyBits( 1 );
  }
}


/*!
 ****************************************************************************
 * \brief
 *    estimate SAMBAC bit cost for significant coefficient group map
 ****************************************************************************
 */
Void TEncSbac::estSignificantCoeffGroupMapBit( estBitsSbacStruct* pcEstBitsSbac, UInt uiCTXIdx, TextType eTType )
{
  Int firstCtx = 0, numCtx = NUM_SIG_CG_FLAG_CTX;

  for ( Int ctxIdx = firstCtx; ctxIdx < firstCtx + numCtx; ctxIdx++ )
  {
    for( UInt uiBin = 0; uiBin < 2; uiBin++ )
    {
      pcEstBitsSbac->significantCoeffGroupBits[ ctxIdx ][ uiBin ] = m_cCUSigCoeffGroupSCModel.get(  0, eTType, ctxIdx ).getEntropyBits( uiBin );
    }
  }
}


/*!
 ****************************************************************************
 * \brief
 *    estimate SAMBAC bit cost for significant coefficient map
 ****************************************************************************
 */
Void TEncSbac::estSignificantMapBit( estBitsSbacStruct* pcEstBitsSbac, Int width, Int height, TextType eTType )
{
  Int firstCtx = 0, numCtx = (eTType == TEXT_LUMA) ? 9 : 6;
  if (std::max(width, height) >= 16)
  {
    firstCtx = (eTType == TEXT_LUMA) ? 20 : 17;
    numCtx = (eTType == TEXT_LUMA) ? 7 : 4;    
  }
  else if (width == 8)
  {
    firstCtx = (eTType == TEXT_LUMA) ? 9 : 6;
    numCtx = 11;
  }
  
  if (eTType == TEXT_LUMA )
  {
    for ( Int ctxIdx = firstCtx; ctxIdx < firstCtx + numCtx; ctxIdx++ )
    {
      for( UInt uiBin = 0; uiBin < 2; uiBin++ )
      {
        pcEstBitsSbac->significantBits[ ctxIdx ][ uiBin ] = m_cCUSigSCModel.get(  0, 0, ctxIdx ).getEntropyBits( uiBin );
      }
    }
  }
  else
  {
    for ( Int ctxIdx = firstCtx; ctxIdx < firstCtx + numCtx; ctxIdx++ )
    {
      for( UInt uiBin = 0; uiBin < 2; uiBin++ )
      {
        pcEstBitsSbac->significantBits[ ctxIdx ][ uiBin ] = m_cCUSigSCModel.get(  0, 0, NUM_SIG_FLAG_CTX_LUMA + ctxIdx ).getEntropyBits( uiBin );
      }
    }
  }
  Int iBitsX = 0, iBitsY = 0;
  const UInt *puiCtxIdx;
  Int ctx;
#if LAST_CTX_REDUCTION
  Int widthCtx = eTType? 4 : width;
  puiCtxIdx = g_uiLastCtx + (g_aucConvertToBit[ widthCtx ]*(g_aucConvertToBit[ widthCtx ]+3));
#else  
  puiCtxIdx = g_uiLastCtx + (g_aucConvertToBit[ width ]*(g_aucConvertToBit[ width ]+3));
#endif
  ContextModel *pCtxX      = m_cCuCtxLastX.get( 0, eTType );
  for (ctx = 0; ctx < g_uiGroupIdx[ width - 1 ]; ctx++)
  {
    Int ctxOffset = puiCtxIdx[ ctx ];
#if LAST_CTX_REDUCTION
    if (eTType)
    {
      Int ctxOffsetC =  ctx>>g_aucConvertToBit[ width ];
      pcEstBitsSbac->lastXBits[ ctx ] = iBitsX + pCtxX[ ctxOffsetC ].getEntropyBits( 0 );
      iBitsX += pCtxX[ ctxOffsetC].getEntropyBits( 1 );
    }
    else
    {
#endif
      pcEstBitsSbac->lastXBits[ ctx ] = iBitsX + pCtxX[ ctxOffset ].getEntropyBits( 0 );
      iBitsX += pCtxX[ ctxOffset ].getEntropyBits( 1 );
#if LAST_CTX_REDUCTION
    }
#endif
    }
  pcEstBitsSbac->lastXBits[ctx] = iBitsX;

#if LAST_CTX_REDUCTION
  Int heightCtx = eTType? 4 : height;
  puiCtxIdx = g_uiLastCtx + (g_aucConvertToBit[ heightCtx ]*(g_aucConvertToBit[ heightCtx ]+3));
#else
  puiCtxIdx = g_uiLastCtx + (g_aucConvertToBit[ height ]*(g_aucConvertToBit[ height ]+3));
#endif
  ContextModel *pCtxY      = m_cCuCtxLastY.get( 0, eTType );
  for (ctx = 0; ctx < g_uiGroupIdx[ height - 1 ]; ctx++)
  {
    Int ctxOffset = puiCtxIdx[ ctx ];
#if LAST_CTX_REDUCTION
    if (eTType)
    {
      Int ctxOffsetC =  ctx>>g_aucConvertToBit[ height ];
      pcEstBitsSbac->lastYBits[ ctx ] = iBitsY + pCtxY[ ctxOffsetC ].getEntropyBits( 0 );
      iBitsY += pCtxY[ctxOffsetC].getEntropyBits( 1 );
    }
    else
    {
#endif
      pcEstBitsSbac->lastYBits[ ctx ] = iBitsY + pCtxY[ ctxOffset ].getEntropyBits( 0 );
      iBitsY += pCtxY[ ctxOffset ].getEntropyBits( 1 );
#if LAST_CTX_REDUCTION
    }
#endif
    }
  pcEstBitsSbac->lastYBits[ctx] = iBitsY;
}

/*!
 ****************************************************************************
 * \brief
 *    estimate bit cost of significant coefficient
 ****************************************************************************
 */
Void TEncSbac::estSignificantCoefficientsBit( estBitsSbacStruct* pcEstBitsSbac, UInt uiCTXIdx, TextType eTType )
{
  if (eTType==TEXT_LUMA)
  {
    ContextModel *ctxOne = m_cCUOneSCModel.get(0, 0);
    ContextModel *ctxAbs = m_cCUAbsSCModel.get(0, 0);

    for (Int ctxIdx = 0; ctxIdx < NUM_ONE_FLAG_CTX_LUMA; ctxIdx++)
    {
      pcEstBitsSbac->m_greaterOneBits[ ctxIdx ][ 0 ] = ctxOne[ ctxIdx ].getEntropyBits( 0 );
      pcEstBitsSbac->m_greaterOneBits[ ctxIdx ][ 1 ] = ctxOne[ ctxIdx ].getEntropyBits( 1 );    
    }

    for (Int ctxIdx = 0; ctxIdx < NUM_ABS_FLAG_CTX_LUMA; ctxIdx++)
    {
      pcEstBitsSbac->m_levelAbsBits[ ctxIdx ][ 0 ] = ctxAbs[ ctxIdx ].getEntropyBits( 0 );
      pcEstBitsSbac->m_levelAbsBits[ ctxIdx ][ 1 ] = ctxAbs[ ctxIdx ].getEntropyBits( 1 );    
    }
  }
  else
  {
    ContextModel *ctxOne = m_cCUOneSCModel.get(0, 0) + NUM_ONE_FLAG_CTX_LUMA;
    ContextModel *ctxAbs = m_cCUAbsSCModel.get(0, 0) + NUM_ABS_FLAG_CTX_LUMA;

    for (Int ctxIdx = 0; ctxIdx < NUM_ONE_FLAG_CTX_CHROMA; ctxIdx++)
    {
      pcEstBitsSbac->m_greaterOneBits[ ctxIdx ][ 0 ] = ctxOne[ ctxIdx ].getEntropyBits( 0 );
      pcEstBitsSbac->m_greaterOneBits[ ctxIdx ][ 1 ] = ctxOne[ ctxIdx ].getEntropyBits( 1 );    
    }

    for (Int ctxIdx = 0; ctxIdx < NUM_ABS_FLAG_CTX_CHROMA; ctxIdx++)
    {
      pcEstBitsSbac->m_levelAbsBits[ ctxIdx ][ 0 ] = ctxAbs[ ctxIdx ].getEntropyBits( 0 );
      pcEstBitsSbac->m_levelAbsBits[ ctxIdx ][ 1 ] = ctxAbs[ ctxIdx ].getEntropyBits( 1 );    
    }
  }
}

/**
 - Initialize our context information from the nominated source.
 .
 \param pSrc From where to copy context information.
 */
Void TEncSbac::xCopyContextsFrom( TEncSbac* pSrc )
{  
  memcpy(m_contextModels, pSrc->m_contextModels, m_numContextModels*sizeof(m_contextModels[0]));
}

Void  TEncSbac::loadContexts ( TEncSbac* pScr)
{
  this->xCopyContextsFrom(pScr);
}

#if HHI_DMM_WEDGE_INTRA || HHI_DMM_PRED_TEX || (LGE_EDGE_INTRA_A0070 && LGE_EDGE_INTRA_DELTA_DC)
Void TEncSbac::xWriteExGolombLevel( UInt uiSymbol, ContextModel& rcSCModel  )
{
  if( uiSymbol )
  {
    m_pcBinIf->encodeBin( 1, rcSCModel );
    UInt uiCount = 0;
    Bool bNoExGo = (uiSymbol < 13);

    while( --uiSymbol && ++uiCount < 13 )
    {
      m_pcBinIf->encodeBin( 1, rcSCModel );
    }
    if( bNoExGo )
    {
      m_pcBinIf->encodeBin( 0, rcSCModel );
    }
    else
    {
      xWriteEpExGolomb( uiSymbol, 0 );
    }
  }
  else
  {
    m_pcBinIf->encodeBin( 0, rcSCModel );
  }

  return;
}
#endif
#if HHI_DMM_WEDGE_INTRA
Void TEncSbac::xCodeWedgeFullInfo( TComDataCU* pcCU, UInt uiAbsPartIdx )
{
  Int iIntraIdx = pcCU->getIntraSizeIdx(uiAbsPartIdx);
  Int iBits = g_aucWedgeFullBitsListIdx[iIntraIdx];

  UInt uiTabIdx = pcCU->getWedgeFullTabIdx( uiAbsPartIdx );

  for ( Int i = 0; i < iBits; i++ )
  {
    m_pcBinIf->encodeBin( ( uiTabIdx >> i ) & 1, m_cDmmDataSCModel.get(0, 0, 0) );
  }
}

Void TEncSbac::xCodeWedgeFullDeltaInfo( TComDataCU* pcCU, UInt uiAbsPartIdx )
{
  Int iIntraIdx = pcCU->getIntraSizeIdx(uiAbsPartIdx);
  Int iBits = g_aucWedgeFullBitsListIdx[iIntraIdx];

  UInt uiTabIdx = pcCU->getWedgeFullTabIdx( uiAbsPartIdx );

  for ( Int i = 0; i < iBits; i++ )
  {
    m_pcBinIf->encodeBin( ( uiTabIdx >> i ) & 1, m_cDmmDataSCModel.get(0, 0, 0) );
  }

  Int iDeltaDC1 = pcCU->getWedgeFullDeltaDC1( uiAbsPartIdx );
  Int iDeltaDC2 = pcCU->getWedgeFullDeltaDC2( uiAbsPartIdx );

  xWriteExGolombLevel( UInt( abs( iDeltaDC1 ) ), m_cDmmDataSCModel.get(0, 0, 1) );
  if ( iDeltaDC1 != 0 )
  {
    UInt uiSign = iDeltaDC1 > 0 ? 0 : 1;
    m_pcBinIf->encodeBinEP( uiSign );
  }
  xWriteExGolombLevel( UInt( abs( iDeltaDC2 ) ), m_cDmmDataSCModel.get(0, 0, 1) );
  if ( iDeltaDC2 != 0 )
  {
    UInt uiSign = iDeltaDC2 > 0 ? 0 : 1;
    m_pcBinIf->encodeBinEP( uiSign );
  }
}

Void TEncSbac::xCodeWedgePredDirInfo( TComDataCU* pcCU, UInt uiAbsPartIdx )
{
  if( DMM_WEDGE_PREDDIR_DELTAEND_MAX > 0 )
  {
    Int iDeltaEnd = pcCU->getWedgePredDirDeltaEnd( uiAbsPartIdx );
    m_pcBinIf->encodeBin( (iDeltaEnd!=0), m_cDmmDataSCModel.get(0, 0, 2) );

    if( iDeltaEnd != 0 )
    {
      UInt uiAbsValMinus1 = abs(iDeltaEnd)-1;
      m_pcBinIf->encodeBin( (uiAbsValMinus1 & 0x01),      m_cDmmDataSCModel.get(0, 0, 2) );
      m_pcBinIf->encodeBin( (uiAbsValMinus1 & 0x02) >> 1, m_cDmmDataSCModel.get(0, 0, 2) );

      UInt uiSign = iDeltaEnd > 0 ? 0 : 1;
      m_pcBinIf->encodeBinEP( uiSign );
    }
  }
}

Void TEncSbac::xCodeWedgePredDirDeltaInfo( TComDataCU* pcCU, UInt uiAbsPartIdx )
{
  if( DMM_WEDGE_PREDDIR_DELTAEND_MAX > 0 )
  {
    Int iDeltaEnd = pcCU->getWedgePredDirDeltaEnd( uiAbsPartIdx );
    m_pcBinIf->encodeBin( (iDeltaEnd!=0), m_cDmmDataSCModel.get(0, 0, 2) );

    if( iDeltaEnd != 0 )
    {
      UInt uiAbsValMinus1 = abs(iDeltaEnd)-1;
      m_pcBinIf->encodeBin( (uiAbsValMinus1 & 0x01),      m_cDmmDataSCModel.get(0, 0, 2) );
      m_pcBinIf->encodeBin( (uiAbsValMinus1 & 0x02) >> 1, m_cDmmDataSCModel.get(0, 0, 2) );

      UInt uiSign = iDeltaEnd > 0 ? 0 : 1;
      m_pcBinIf->encodeBinEP( uiSign );
    }
  }

  Int iDeltaDC1 = pcCU->getWedgePredDirDeltaDC1( uiAbsPartIdx );
  Int iDeltaDC2 = pcCU->getWedgePredDirDeltaDC2( uiAbsPartIdx );

  xWriteExGolombLevel( UInt( abs( iDeltaDC1 ) ), m_cDmmDataSCModel.get(0, 0, 1) );
  if ( iDeltaDC1 != 0 )
  {
    UInt uiSign = iDeltaDC1 > 0 ? 0 : 1;
    m_pcBinIf->encodeBinEP( uiSign );
  }
  xWriteExGolombLevel( UInt( abs( iDeltaDC2 ) ), m_cDmmDataSCModel.get(0, 0, 1) );
  if ( iDeltaDC2 != 0 )
  {
    UInt uiSign = iDeltaDC2 > 0 ? 0 : 1;
    m_pcBinIf->encodeBinEP( uiSign );
  }
}
#endif
#if HHI_DMM_PRED_TEX
Void TEncSbac::xCodeWedgePredTexDeltaInfo( TComDataCU* pcCU, UInt uiAbsPartIdx )
{
  Int iDeltaDC1 = pcCU->getWedgePredTexDeltaDC1( uiAbsPartIdx );
  Int iDeltaDC2 = pcCU->getWedgePredTexDeltaDC2( uiAbsPartIdx );

  xWriteExGolombLevel( UInt( abs( iDeltaDC1 ) ), m_cDmmDataSCModel.get(0, 0, 1) );
  if ( iDeltaDC1 != 0 )
  {
    UInt uiSign = iDeltaDC1 > 0 ? 0 : 1;
    m_pcBinIf->encodeBinEP( uiSign );
  }
  xWriteExGolombLevel( UInt( abs( iDeltaDC2 ) ), m_cDmmDataSCModel.get(0, 0, 1) );
  if ( iDeltaDC2 != 0 )
  {
    UInt uiSign = iDeltaDC2 > 0 ? 0 : 1;
    m_pcBinIf->encodeBinEP( uiSign );
  }
}

Void TEncSbac::xCodeContourPredTexDeltaInfo( TComDataCU* pcCU, UInt uiAbsPartIdx )
{
  Int iDeltaDC1 = pcCU->getContourPredTexDeltaDC1( uiAbsPartIdx );
  Int iDeltaDC2 = pcCU->getContourPredTexDeltaDC2( uiAbsPartIdx );

  xWriteExGolombLevel( UInt( abs( iDeltaDC1 ) ), m_cDmmDataSCModel.get(0, 0, 1) );
  if ( iDeltaDC1 != 0 )
  {
    UInt uiSign = iDeltaDC1 > 0 ? 0 : 1;
    m_pcBinIf->encodeBinEP( uiSign );
  }
  xWriteExGolombLevel( UInt( abs( iDeltaDC2 ) ), m_cDmmDataSCModel.get(0, 0, 1) );
  if ( iDeltaDC2 != 0 )
  {
    UInt uiSign = iDeltaDC2 > 0 ? 0 : 1;
    m_pcBinIf->encodeBinEP( uiSign );
  }
}
#endif

#if RWTH_SDC_DLT_B0036
Void TEncSbac::codeSDCPredMode( TComDataCU* pcCU, UInt uiAbsPartIdx )
{
  assert( pcCU->getSlice()->getSPS()->isDepth() );
  assert( pcCU->getPartitionSize(uiAbsPartIdx) == SIZE_2Nx2N );
  assert( pcCU->getSDCFlag(uiAbsPartIdx) );
  
  UInt uiPredMode     = pcCU->getLumaIntraDir(uiAbsPartIdx);
  UInt uiCtx          = 0;
  
  UInt uiMPModeIdx    = 0;
  
  for(Int i=0; i<RWTH_SDC_NUM_PRED_MODES-1; i++)
  {
    UInt uiBit = (uiPredMode == g_auiSDCPredModes[uiMPModeIdx]) ? 1 : 0;
    m_pcBinIf->encodeBin( uiBit, m_cSDCPredModeSCModel.get( 0, i, uiCtx ) );
    
    // if mode is most probable mode, we are done here
    if ( uiBit == 1 )
      break;
    
    // else: get next most probable pred mode
    uiMPModeIdx = (uiMPModeIdx+1)%RWTH_SDC_NUM_PRED_MODES;
  }
  
#if HHI_DMM_WEDGE_INTRA
  if( uiPredMode == DMM_WEDGE_FULL_IDX )          { xCodeWedgeFullInfo          ( pcCU, uiAbsPartIdx ); }
  if( uiPredMode == DMM_WEDGE_PREDDIR_IDX )       { xCodeWedgePredDirInfo       ( pcCU, uiAbsPartIdx ); }
#endif
  
  AOF(uiPredMode == g_auiSDCPredModes[uiMPModeIdx]);
}

Void TEncSbac::codeSDCFlag( TComDataCU* pcCU, UInt uiAbsPartIdx )
{
  // get context function is here
  UInt uiSymbol = pcCU->getSDCFlag( uiAbsPartIdx ) ? 1 : 0;
  UInt uiCtxSDCFlag = pcCU->getCtxSDCFlag( uiAbsPartIdx );
  m_pcBinIf->encodeBin( uiSymbol, m_cSDCFlagSCModel.get( 0, 0, uiCtxSDCFlag ) );
  
}

Void TEncSbac::codeSDCResidualData ( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiSegment )
{
  assert( pcCU->getSlice()->getSPS()->isDepth() );
  assert( pcCU->getPartitionSize(uiAbsPartIdx) == SIZE_2Nx2N );
  assert( pcCU->getSDCFlag(uiAbsPartIdx) );
  assert( uiSegment < 2 );
  
  Pel segmentDCOffset = pcCU->getSDCSegmentDCOffset(uiSegment, uiAbsPartIdx);
  
  UInt uiResidual = segmentDCOffset == 0 ? 0 : 1;
  UInt uiSign     = segmentDCOffset < 0 ? 1 : 0;
  UInt uiAbsIdx   = abs(segmentDCOffset);
  UInt uiBit = 0;
  
  UInt uiMaxResidualBits  = GetBitsPerDepthValue();
  assert( uiMaxResidualBits <= g_uiBitDepth );
  
  // residual flag
  m_pcBinIf->encodeBin( uiResidual, m_cSDCResidualFlagSCModel.get( 0, uiSegment, 0 ) ); //TODO depthmap: more sophisticated context selection
  
  if (uiResidual)
  {
    // encode sign bit of residual
    m_pcBinIf->encodeBin( uiSign, m_cSDCResidualSignFlagSCModel.get( 0, uiSegment, 0 ) ); //TODO depthmap: more sophisticated context selection
        
    assert(uiAbsIdx < GetNumDepthValues());
    
    // encode residual magnitude
    uiAbsIdx -= 1;
    for (Int i=0; i<uiMaxResidualBits; i++)
    {
      uiBit = (uiAbsIdx & (1<<i))>>i;
      
      m_pcBinIf->encodeBin( uiBit, m_cSDCResidualSCModel.get( 0, uiSegment, i ) ); //TODO depthmap: more sophisticated context selection
    }
    
  }
}
#endif
//! \}
