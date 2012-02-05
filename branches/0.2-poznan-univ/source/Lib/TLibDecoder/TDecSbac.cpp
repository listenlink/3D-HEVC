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



/** \file     TDecSbac.cpp
    \brief    Context-adaptive entropy decoder class
*/

#include "TDecSbac.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

TDecSbac::TDecSbac() 
// new structure here
: m_pcBitstream               ( 0 )
, m_pcTDecBinIf               ( NULL )
, m_bAlfCtrl                  ( false )
, m_uiMaxAlfCtrlDepth         ( 0 )
, m_cCUSkipFlagSCModel        ( 1,             1,               NUM_SKIP_FLAG_CTX             )
, m_cCUSplitFlagSCModel       ( 1,             1,               NUM_SPLIT_FLAG_CTX            )
, m_cCUMergeFlagExtSCModel    ( 1,             1,               NUM_MERGE_FLAG_EXT_CTX        )
, m_cCUMergeIdxExtSCModel     ( 1,             1,               NUM_MERGE_IDX_EXT_CTX         )
, m_cCUMVMergeIdxExtSCModel   ( 1,             1,               NUM_MV_MERGE_IDX_EXT_CTX      )
, m_cResPredFlagSCModel       ( 1,             1,               NUM_RES_PRED_FLAG_CTX         )
, m_cCUAlfCtrlFlagSCModel     ( 1,             1,               NUM_ALF_CTRL_FLAG_CTX         )
, m_cCUPartSizeSCModel        ( 1,             1,               NUM_PART_SIZE_CTX             )
, m_cCUPredModeSCModel        ( 1,             1,               NUM_PRED_MODE_CTX             )
, m_cCUIntraPredSCModel       ( 1,             1,               NUM_ADI_CTX                   )
#if ADD_PLANAR_MODE
, m_cPlanarFlagSCModel        ( 1,             1,               NUM_PLANARFLAG_CTX            )
#endif
, m_cCUChromaPredSCModel      ( 1,             1,               NUM_CHROMA_PRED_CTX           )
, m_cCUInterDirSCModel        ( 1,             1,               NUM_INTER_DIR_CTX             )
, m_cCURefPicSCModel          ( 1,             1,               NUM_REF_NO_CTX                )
, m_cCUMvdSCModel             ( 1,             2,               NUM_MV_RES_CTX                )
, m_cCUTransSubdivFlagSCModel ( 1,             1,               NUM_TRANS_SUBDIV_FLAG_CTX     )
, m_cCUQtRootCbfSCModel       ( 1,             1,               NUM_QT_ROOT_CBF_CTX           )
, m_cCUDeltaQpSCModel         ( 1,             1,               NUM_DELTA_QP_CTX              )
, m_cCUQtCbfSCModel           ( 1,             3,               NUM_QT_CBF_CTX                )
#if SIMPLE_CONTEXT_SIG
, m_cCUSigSCModel             ( 4,             2,               NUM_SIG_FLAG_CTX              ) 
#else
, m_cCUSigSCModel             ( MAX_CU_DEPTH,  2,               NUM_SIG_FLAG_CTX              )
#endif
#if PCP_SIGMAP_SIMPLE_LAST
, m_cCuCtxLastX               ( 1,             2,               NUM_CTX_LAST_FLAG_XY          )
, m_cCuCtxLastY               ( 1,             2,               NUM_CTX_LAST_FLAG_XY          )
#else
, m_cCULastSCModel            ( MAX_CU_DEPTH,  2,               NUM_LAST_FLAG_CTX             )
#endif
, m_cCUOneSCModel             ( 1,             2,               NUM_ONE_FLAG_CTX              )
, m_cCUAbsSCModel             ( 1,             2,               NUM_ABS_FLAG_CTX              )
, m_cMVPIdxSCModel            ( 1,             1,               NUM_MVP_IDX_CTX               )
, m_cALFFlagSCModel           ( 1,             1,               NUM_ALF_FLAG_CTX              )
, m_cALFUvlcSCModel           ( 1,             1,               NUM_ALF_UVLC_CTX              )
, m_cALFSvlcSCModel           ( 1,             1,               NUM_ALF_SVLC_CTX              )
#if MTK_SAO
, m_cAOFlagSCModel            ( 1,             1,               NUM_AO_FLAG_CTX              )
, m_cAOUvlcSCModel            ( 1,             1,               NUM_AO_UVLC_CTX              )
, m_cAOSvlcSCModel            ( 1,             1,               NUM_AO_SVLC_CTX              )
#endif
, m_cViewIdxSCModel           ( 1,             1,               NUM_VIEW_IDX_CTX              )
#if HHI_DMM_WEDGE_INTRA || HHI_DMM_PRED_TEX
, m_cIntraDMMSCModel          ( 1,             1,               NUM_DMM_CTX                   )
, m_cIntraWedgeSCModel        ( 1,             1,               NUM_WEDGE_CTX                 )
#endif
{
}

TDecSbac::~TDecSbac()
{
}

// ====================================================================================================================
// Public member functions
// ====================================================================================================================

Void TDecSbac::resetEntropy          (TComSlice* pcSlice)
{
  Int  iQp              = pcSlice->getSliceQp();
  SliceType eSliceType  = pcSlice->getSliceType();
  
  m_cCUSplitFlagSCModel.initBuffer       ( eSliceType, iQp, (Short*)INIT_SPLIT_FLAG );
  m_cCUSkipFlagSCModel.initBuffer        ( eSliceType, iQp, (Short*)INIT_SKIP_FLAG );
  m_cCUMergeFlagExtSCModel.initBuffer    ( eSliceType, iQp, (Short*)INIT_MERGE_FLAG_EXT );
  m_cCUMergeIdxExtSCModel.initBuffer     ( eSliceType, iQp, (Short*)INIT_MERGE_IDX_EXT );
  m_cCUMVMergeIdxExtSCModel.initBuffer   ( eSliceType, iQp, (Short*)INIT_MV_MERGE_IDX_EXT );
  m_cResPredFlagSCModel.initBuffer       ( eSliceType, iQp, (Short*)INIT_RES_PRED_FLAG );
  m_cCUAlfCtrlFlagSCModel.initBuffer     ( eSliceType, iQp, (Short*)INIT_ALF_CTRL_FLAG );
  m_cCUPartSizeSCModel.initBuffer        ( eSliceType, iQp, (Short*)INIT_PART_SIZE );
  m_cCUPredModeSCModel.initBuffer        ( eSliceType, iQp, (Short*)INIT_PRED_MODE );
  m_cCUIntraPredSCModel.initBuffer       ( eSliceType, iQp, (Short*)INIT_INTRA_PRED_MODE );
#if ADD_PLANAR_MODE
  m_cPlanarFlagSCModel.initBuffer        ( eSliceType, iQp, (Short*)INIT_PLANARFLAG );
#endif
  m_cCUChromaPredSCModel.initBuffer      ( eSliceType, iQp, (Short*)INIT_CHROMA_PRED_MODE );
  m_cCUInterDirSCModel.initBuffer        ( eSliceType, iQp, (Short*)INIT_INTER_DIR );
  m_cCUMvdSCModel.initBuffer             ( eSliceType, iQp, (Short*)INIT_MVD );
  m_cCURefPicSCModel.initBuffer          ( eSliceType, iQp, (Short*)INIT_REF_PIC );
  m_cCUDeltaQpSCModel.initBuffer         ( eSliceType, iQp, (Short*)INIT_DQP );
  m_cCUQtCbfSCModel.initBuffer           ( eSliceType, iQp, (Short*)INIT_QT_CBF );
  m_cCUQtRootCbfSCModel.initBuffer       ( eSliceType, iQp, (Short*)INIT_QT_ROOT_CBF );
  m_cCUSigSCModel.initBuffer             ( eSliceType, iQp, (Short*)INIT_SIG_FLAG );
#if PCP_SIGMAP_SIMPLE_LAST
  m_cCuCtxLastX.initBuffer               ( eSliceType, iQp, (Short*)INIT_LAST_X );
  m_cCuCtxLastY.initBuffer               ( eSliceType, iQp, (Short*)INIT_LAST_Y );
#else
  m_cCULastSCModel.initBuffer            ( eSliceType, iQp, (Short*)INIT_LAST_FLAG );
#endif
  m_cCUOneSCModel.initBuffer             ( eSliceType, iQp, (Short*)INIT_ONE_FLAG );
  m_cCUAbsSCModel.initBuffer             ( eSliceType, iQp, (Short*)INIT_ABS_FLAG );
  m_cMVPIdxSCModel.initBuffer            ( eSliceType, iQp, (Short*)INIT_MVP_IDX );
  m_cALFFlagSCModel.initBuffer           ( eSliceType, iQp, (Short*)INIT_ALF_FLAG );
  m_cALFUvlcSCModel.initBuffer           ( eSliceType, iQp, (Short*)INIT_ALF_UVLC );
  m_cALFSvlcSCModel.initBuffer           ( eSliceType, iQp, (Short*)INIT_ALF_SVLC );
#if MTK_SAO
  m_cAOFlagSCModel.initBuffer           ( eSliceType, iQp, (Short*)INIT_AO_FLAG );
  m_cAOUvlcSCModel.initBuffer           ( eSliceType, iQp, (Short*)INIT_AO_UVLC );
  m_cAOSvlcSCModel.initBuffer           ( eSliceType, iQp, (Short*)INIT_AO_SVLC );
#endif
  m_cCUTransSubdivFlagSCModel.initBuffer ( eSliceType, iQp, (Short*)INIT_TRANS_SUBDIV_FLAG );
  m_cViewIdxSCModel.initBuffer           ( eSliceType, iQp, (Short*)INIT_VIEW_IDX );
#if HHI_DMM_WEDGE_INTRA || HHI_DMM_PRED_TEX
  m_cIntraDMMSCModel.initBuffer          ( eSliceType, iQp, (Short*)INIT_INTRA_DMM );
  m_cIntraWedgeSCModel.initBuffer        ( eSliceType, iQp, (Short*)INIT_INTRA_WEDGELET );
#endif
  
  m_uiLastDQpNonZero  = 0;
  
  // new structure
  m_uiLastQp          = iQp;
  
  m_pcTDecBinIf->start();
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

#if MVD_CTX
/** Decode a motion vector difference
 * \param riMvdComp motion vector difference
 * \param uiAbsSumL motion vector difference of left PU
 * \param uiAbsSumA motion vector difference of above PU
 * \param uiCtx index for context set based on vertical or horizontal component
 */
Void TDecSbac::xReadMvd( Int& riMvdComp, UInt uiAbsSumL, UInt uiAbsSumA, UInt uiCtx )
#else
Void TDecSbac::xReadMvd( Int& riMvdComp, UInt uiAbsSum, UInt uiCtx )
#endif
{
  UInt uiLocalCtx = 0;

#if MVD_CTX
  uiLocalCtx += (uiAbsSumA>16) ? 1 : 0;
  uiLocalCtx += (uiAbsSumL>16) ? 1 : 0;
#else
  if( uiAbsSum >= 3 )
  {
    uiLocalCtx += ( uiAbsSum > 32) ? 2 : 1;
  }
#endif

  riMvdComp = 0;
  
  UInt uiSymbol;
  m_pcTDecBinIf->decodeBin( uiSymbol, m_cCUMvdSCModel.get( 0, uiCtx, uiLocalCtx ) );
  
  if (!uiSymbol)
  {
    return;
  }
  
  xReadExGolombMvd( uiSymbol, &m_cCUMvdSCModel.get( 0, uiCtx, 3 ), 3 );
  uiSymbol++;
  
  UInt uiSign;
  m_pcTDecBinIf->decodeBinEP( uiSign );
  
  riMvdComp = ( 0 != uiSign ) ? -(Int)uiSymbol : (Int)uiSymbol;
  
  return;
}

Void TDecSbac::xReadExGolombMvd( UInt& ruiSymbol, ContextModel* pcSCModel, UInt uiMaxBin )
{
  UInt uiSymbol;
  
  m_pcTDecBinIf->decodeBin( ruiSymbol, pcSCModel[0] );
  
  if( !ruiSymbol )
  {
    return;
  }
  
  m_pcTDecBinIf->decodeBin( uiSymbol, pcSCModel[1] );
  
  ruiSymbol = 1;
  
  if( !uiSymbol )
  {
    return;
  }
  
  pcSCModel += 2;
  UInt uiCount = 2;
  
  do
  {
    if( uiMaxBin == uiCount )
    {
      pcSCModel++;
    }
    m_pcTDecBinIf->decodeBin( uiSymbol, *pcSCModel );
    uiCount++;
  }
  while( uiSymbol && ( uiCount != 8 ) );
  
  ruiSymbol = uiCount - 1;
  
  if( uiSymbol )
  {
    xReadEpExGolomb( uiSymbol, 3 );
    ruiSymbol += uiSymbol + 1;
  }
  
  return;
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
  
  uiCount--;
  while( uiCount-- )
  {
    m_pcTDecBinIf->decodeBinEP( uiBit );
    uiSymbol += uiBit << uiCount;
  }
  
  ruiSymbol = uiSymbol;
  
  return;
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

#if E253
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

  for( UInt ui = 0; ui < ruiGoRiceParam; ui++ )
  {
    m_pcTDecBinIf->decodeBinEP( uiCodeWord );
    if( uiCodeWord )
    {
      uiRemainder += 1 << ui;
    }
  }

  ruiSymbol      = uiRemainder + ( uiQuotient << ruiGoRiceParam );
  bExGolomb      = ruiSymbol == ( uiMaxVlc + 1 );

  if( bExGolomb )
  {
    xReadEpExGolomb( uiCodeWord, 0 );
    ruiSymbol += uiCodeWord;
  }

  ruiGoRiceParam = g_aauiGoRiceUpdate[ ruiGoRiceParam ][ min<UInt>( ruiSymbol, 15 ) ];

  return;
}
#if HHI_DMM_WEDGE_INTRA || HHI_DMM_PRED_TEX
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
#else
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

Void TDecSbac::parseAlfCtrlDepth( UInt& ruiAlfCtrlDepth )
{
  UInt uiSymbol;
  xReadUnaryMaxSymbol( uiSymbol, m_cALFUvlcSCModel.get( 0 ), 1, g_uiMaxCUDepth - 1 );
  ruiAlfCtrlDepth = uiSymbol;
}

Void TDecSbac::parseAlfCtrlFlag( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth )
{
  if( !m_bAlfCtrl )
  {
    return;
  }
  
  if( uiDepth > m_uiMaxAlfCtrlDepth && !pcCU->isFirstAbsZorderIdxInDepth( uiAbsPartIdx, m_uiMaxAlfCtrlDepth ) )
  {
    return;
  }
  
  UInt uiSymbol;
  m_pcTDecBinIf->decodeBin( uiSymbol, m_cCUAlfCtrlFlagSCModel.get( 0, 0, pcCU->getCtxAlfCtrlFlag( uiAbsPartIdx ) ) );
  
  if( uiDepth > m_uiMaxAlfCtrlDepth )
  {
    pcCU->setAlfCtrlFlagSubParts( uiSymbol, uiAbsPartIdx, m_uiMaxAlfCtrlDepth );
  }
  else
  {
    pcCU->setAlfCtrlFlagSubParts( uiSymbol, uiAbsPartIdx, uiDepth );
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
  DTRACE_CABAC_V( g_nSymbolCounter++ );
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
    
#if HHI_MRG_SKIP
    pcCU->setMergeFlagSubParts( true , uiAbsPartIdx, 0, uiDepth );
#else
    TComMv cZeroMv(0,0);
    pcCU->getCUMvField( REF_PIC_LIST_0 )->setAllMvd    ( cZeroMv, SIZE_2Nx2N, uiAbsPartIdx, 0, uiDepth );
    pcCU->getCUMvField( REF_PIC_LIST_1 )->setAllMvd    ( cZeroMv, SIZE_2Nx2N, uiAbsPartIdx, 0, uiDepth );
    
    pcCU->setTrIdxSubParts( 0, uiAbsPartIdx, uiDepth );
    pcCU->setCbfSubParts  ( 0, 0, 0, uiAbsPartIdx, uiDepth );
    
    if ( pcCU->getSlice()->isInterP() )
    {
      pcCU->setInterDirSubParts( 1, uiAbsPartIdx, 0, uiDepth );
      
      if ( pcCU->getSlice()->getNumRefIdx( REF_PIC_LIST_0 ) > 0 )
        pcCU->getCUMvField( REF_PIC_LIST_0 )->setAllRefIdx(  0, SIZE_2Nx2N, uiAbsPartIdx, 0, uiDepth );
      if ( pcCU->getSlice()->getNumRefIdx( REF_PIC_LIST_1 ) > 0 )
        pcCU->getCUMvField( REF_PIC_LIST_1 )->setAllRefIdx( NOT_VALID, SIZE_2Nx2N, uiAbsPartIdx, 0, uiDepth );
    }
    else
    {
      pcCU->setInterDirSubParts( 3, uiAbsPartIdx, 0, uiDepth );
      
      if ( pcCU->getSlice()->getNumRefIdx( REF_PIC_LIST_0 ) > 0 )
        pcCU->getCUMvField( REF_PIC_LIST_0 )->setAllRefIdx(  0, SIZE_2Nx2N, uiAbsPartIdx, 0, uiDepth );
      if ( pcCU->getSlice()->getNumRefIdx( REF_PIC_LIST_1 ) > 0 )
        pcCU->getCUMvField( REF_PIC_LIST_1 )->setAllRefIdx( 0, SIZE_2Nx2N, uiAbsPartIdx, 0, uiDepth );
    }
#endif // HHI_MRG_SKIP
  }
}

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
  UInt uiCtx = 0;
#if CHANGE_MERGE_CONTEXT
  uiCtx = pcCU->getCtxMergeFlag( uiAbsPartIdx );
#else
  for(UInt uiIter = 0; uiIter < MRG_MAX_NUM_CANDS; uiIter++ )
  {
    if( pcCU->getNeighbourCandIdx( uiIter, uiAbsPartIdx ) == uiIter + 1 )
    {
      if( uiIter == 0 )
      {
        uiCtx++;
      }
      else if( uiIter == 1 )
      {
        uiCtx++;
      }
    }
  }
#endif
  m_pcTDecBinIf->decodeBin( uiSymbol, m_cCUMergeFlagExtSCModel.get( 0, 0, uiCtx ) );
  pcCU->setMergeFlagSubParts( uiSymbol ? true : false, uiAbsPartIdx, uiPUIdx, uiDepth );

  DTRACE_CABAC_V( g_nSymbolCounter++ );
  DTRACE_CABAC_T( "\tMergeFlag: " );
  DTRACE_CABAC_V( uiSymbol );
  DTRACE_CABAC_T( "\tAddress: " );
  DTRACE_CABAC_V( pcCU->getAddr() );
  DTRACE_CABAC_T( "\tuiAbsPartIdx: " );
  DTRACE_CABAC_V( uiAbsPartIdx );
  for( UInt ui = 0; ui < MRG_MAX_NUM_CANDS; ui++ )
  {
    DTRACE_CABAC_T( "\tNumMrgCand: " );
    DTRACE_CABAC_V( ui );
    DTRACE_CABAC_T( "\t==\t" );
    DTRACE_CABAC_V( UInt( pcCU->getNeighbourCandIdx( ui, uiAbsPartIdx ) ) );
  }
  DTRACE_CABAC_T( "\n" );
}


#if HHI_INTER_VIEW_MOTION_PRED || HHI_MPI || POZNAN_EIVD
Void TDecSbac::parseMergeIndexMV( TComDataCU* pcCU, UInt& ruiMergeIndex, UInt uiAbsPartIdx, UInt uiDepth )
{
#if HHI_MPI
  const Bool bMVIAvailable = pcCU->getSlice()->getSPS()->getUseMVI() && pcCU->getSlice()->getSliceType() != I_SLICE;
  const UInt uiMviMergePos = bMVIAvailable ? HHI_MPI_MERGE_POS : MRG_MAX_NUM_CANDS;
#endif
#if POZNAN_EIVD
  const Bool bEIVDAvailable = pcCU->getSlice()->getMP()->isEIVDEnabled();
  Bool bEIVDModifyMergeIdx = true;
#endif
  //--- set number of candidates and availability ---
  Bool  abAvailable[ MRG_MAX_NUM_CANDS ];
  UInt  uiNumCand = 0;
  for( UInt uiIdx = 0; uiIdx < MRG_MAX_NUM_CANDS; uiIdx++ )
  {
#if HHI_MPI
    if( uiIdx == uiMviMergePos )
    {
      abAvailable[ uiIdx ] = true;
      uiNumCand++;
    }
    else if( uiIdx > uiMviMergePos )
    {
      if( ( abAvailable[ uiIdx ] = ( pcCU->getNeighbourCandIdx( uiIdx - 1, uiAbsPartIdx ) == uiIdx ) ) )
      {
        uiNumCand++;
      }
    }
    else
#endif
    if( ( abAvailable[ uiIdx ] = ( pcCU->getNeighbourCandIdx( uiIdx, uiAbsPartIdx ) == uiIdx + 1 ) ) )
    {
      uiNumCand++;
    }
  }

#if POZNAN_EIVD
  if(bEIVDAvailable)
  {
    UInt uiEIVDMergePos = POZNAN_EIVD_MERGE_POS;
#if HHI_MPI
	if(bMVIAvailable && uiEIVDMergePos>=uiMviMergePos) uiEIVDMergePos++;
#endif
	for( Int iIdx = MRG_MAX_NUM_CANDS-2; iIdx >= (Int)uiEIVDMergePos; iIdx-- )
	{
		Int iWrIdx = iIdx+1;
#if HHI_MPI
		if(bMVIAvailable) 
		{
			if(iIdx==(Int)uiMviMergePos) continue;
			if(iWrIdx==(Int)uiMviMergePos) iWrIdx++;
		}
#endif
		if(iWrIdx>=MRG_MAX_NUM_CANDS) continue;

		abAvailable[ iWrIdx ] = abAvailable[ iIdx ];
	}
	abAvailable[ uiEIVDMergePos ] = ( pcCU->getNeighbourCandIdx( POZNAN_EIVD_MRG_CAND, uiAbsPartIdx ) == POZNAN_EIVD_MRG_CAND + 1 );
	
	uiNumCand = 0; for( UInt uiIdx = 0; uiIdx < MRG_MAX_NUM_CANDS; uiIdx++ ) if(abAvailable[ uiIdx ]) uiNumCand++;
  }
#endif

  AOF( uiNumCand > 1 );

  //--- determine contexts ---
  AOF( NUM_MV_MERGE_IDX_EXT_CTX >= MRG_MAX_NUM_CANDS - 1 );
  UInt  auiCtx[ MRG_MAX_NUM_CANDS - 1 ];
  for( UInt uiIdx = 0; uiIdx < MRG_MAX_NUM_CANDS - 1; uiIdx++ )
  {
    auiCtx[ uiIdx ] = MRG_MAX_NUM_CANDS;
    if( uiNumCand > uiIdx + 1 )
    {
      Bool bAvComb    = false;
      for( UInt uiCId = uiIdx; uiCId < MRG_MAX_NUM_CANDS - 1; uiCId++ )
      {
        bAvComb = ( bAvComb || abAvailable[ uiCId ] );
        if( abAvailable[ uiCId + 1 ] && bAvComb )
        {
          auiCtx[ uiIdx ] = uiCId;
          break;
        }
      }
      AOT( auiCtx[ uiIdx ] == MRG_MAX_NUM_CANDS );
    }
  }

  //--- parse unary index ---
  UInt uiUnaryIdx = 0;
  for( ; uiUnaryIdx < uiNumCand - 1; uiUnaryIdx++ )
  {
    UInt uiSymbol = 0;
    m_pcTDecBinIf->decodeBin( uiSymbol, m_cCUMVMergeIdxExtSCModel.get( 0, 0, auiCtx[uiUnaryIdx] ) );
    if( uiSymbol == 0 )
    {
      break;
    }
  }

  //--- determine merge index ---
  ruiMergeIndex = uiUnaryIdx;
  for( UInt uiIdx = 0; uiIdx <= ruiMergeIndex; uiIdx++ )
  {
    if( ! abAvailable[ uiIdx ] )
    {
      ruiMergeIndex++;
    }
  }

#if HHI_MPI
  if( ruiMergeIndex > uiMviMergePos )
  {
    ruiMergeIndex--;
  }
  else if( ruiMergeIndex == uiMviMergePos )
  {
    pcCU->setTextureModeDepthSubParts( uiDepth, uiAbsPartIdx, uiDepth );
#if POZNAN_EIVD
	bEIVDModifyMergeIdx = false;
#endif
  }
#endif

#if POZNAN_EIVD
  if(bEIVDAvailable && bEIVDModifyMergeIdx)
  {
	if(ruiMergeIndex==POZNAN_EIVD_MERGE_POS) ruiMergeIndex = POZNAN_EIVD_MRG_CAND;
	else if(ruiMergeIndex>POZNAN_EIVD_MERGE_POS) ruiMergeIndex--;
  }
#endif

  //--- trace ---
  DTRACE_CABAC_V( g_nSymbolCounter++ );
  DTRACE_CABAC_T( "\tparseMergeIndex():" );
  DTRACE_CABAC_T( "   MRGIdx = " );
  DTRACE_CABAC_V( ruiMergeIndex );
  DTRACE_CABAC_T( "   UnaryIdx = " );
  DTRACE_CABAC_V( uiUnaryIdx );
  DTRACE_CABAC_T( "   NumCand = " );
  DTRACE_CABAC_V( uiNumCand );
  DTRACE_CABAC_T( "   Availibility = " );
  for( UInt uiIdx = 0; uiIdx < MRG_MAX_NUM_CANDS; uiIdx++ )
  {
    DTRACE_CABAC_V( abAvailable[ uiIdx ] ? 1 : 0 );
  }
  DTRACE_CABAC_T( "   Contexts = " );
  for( UInt uiIdx = 0; uiIdx < MRG_MAX_NUM_CANDS - 1; uiIdx++ )
  {
    DTRACE_CABAC_V( auiCtx[ uiIdx ] );
  }
  DTRACE_CABAC_T( "\n" );
}
#endif


Void TDecSbac::parseMergeIndex ( TComDataCU* pcCU, UInt& ruiMergeIndex, UInt uiAbsPartIdx, UInt uiDepth )
{
#if HHI_INTER_VIEW_MOTION_PRED || HHI_MPI || POZNAN_EIVD
  if(
#if HHI_INTER_VIEW_MOTION_PRED
      ( pcCU->getSlice()->getSPS()->getViewId() > 0 && ( pcCU->getSlice()->getSPS()->getMultiviewMvPredMode() & PDM_USE_FOR_MERGE ) == PDM_USE_FOR_MERGE ) ||
#endif
#if HHI_MPI
      ( pcCU->getSlice()->getSPS()->getUseMVI() && pcCU->getSlice()->getSliceType() != I_SLICE && pcCU->getPartitionSize( uiAbsPartIdx ) == SIZE_2Nx2N ) ||
#endif
#if	POZNAN_EIVD  
	  ( pcCU->getSlice()->getMP()->isEIVDEnabled() ) ||
#endif
      0
    )
  {
    parseMergeIndexMV( pcCU, ruiMergeIndex, uiAbsPartIdx, uiDepth );
    return;
  }
#endif

  Bool bLeftInvolved = false;
  Bool bAboveInvolved = false;
  Bool bCollocatedInvolved = false;
  Bool bCornerInvolved = false;
  Bool bCornerBLInvolved = false;
  UInt uiNumCand = 0;
  for( UInt uiIter = 0; uiIter < MRG_MAX_NUM_CANDS; ++uiIter )
  {
    if( pcCU->getNeighbourCandIdx( uiIter, uiAbsPartIdx ) == uiIter + 1 )
    {
      uiNumCand++;
      if( uiIter == 0 )
      {
        bLeftInvolved = true;
      }
      else if( uiIter == 1 )
      {
        bAboveInvolved = true;
      }
      else if( uiIter == 2 )
      {
        bCollocatedInvolved = true;
      }
      else if( uiIter == 3 )
      {
        bCornerInvolved = true;
      }
      else if( uiIter == 4 )
      {
        bCornerBLInvolved = true;
      }
    }
  }
  assert( uiNumCand > 1 );

  UInt auiCtx[4] = { 0, 0, 0, 3 };
  if( bLeftInvolved && bAboveInvolved )
  {
    auiCtx[0] = 0;
  }
  else if( bLeftInvolved || bAboveInvolved )
  {
    auiCtx[0] = bCollocatedInvolved ? 1 : 2;
  }
  else
  {
    auiCtx[0] = bCollocatedInvolved ? 2 : 3;
  }

  if( uiNumCand >= 3 )
  {
    if( bAboveInvolved )
    {
      auiCtx[1] = bCollocatedInvolved ? 1 : 2;
    }
    else
    {
      auiCtx[1] = bCollocatedInvolved ? 2 : 3;
    }
  }

  if( uiNumCand >= 4 )
  {
    auiCtx[2] =  bCollocatedInvolved ? 2 : 3;
  }

  UInt uiUnaryIdx = 0;
  for( ; uiUnaryIdx < uiNumCand - 1; ++uiUnaryIdx )
  {
    UInt uiSymbol = 0;
    m_pcTDecBinIf->decodeBin( uiSymbol, m_cCUMergeIdxExtSCModel.get( 0, 0, auiCtx[uiUnaryIdx] ) );
    if( uiSymbol == 0 )
    {
      break;
    }
  }

  if( !bLeftInvolved )
  {
    ++uiUnaryIdx;
  }
  if( !bAboveInvolved && uiUnaryIdx >= 1 )
  {
    ++uiUnaryIdx;
  }
  if( !bCollocatedInvolved && uiUnaryIdx >= 2 )
  {
    ++uiUnaryIdx;
  }
  if( !bCornerInvolved && uiUnaryIdx >= 3 )
  {
    ++uiUnaryIdx;
  }

  ruiMergeIndex = uiUnaryIdx;

  DTRACE_CABAC_V( g_nSymbolCounter++ )
  DTRACE_CABAC_T( "\tparseMergeIndex()" )
  DTRACE_CABAC_T( "\tuiMRGIdx= " )
  DTRACE_CABAC_V( ruiMergeIndex )
  DTRACE_CABAC_T( "\tuiNumCand= " )
  DTRACE_CABAC_V( uiNumCand )
  DTRACE_CABAC_T( "\tbLeftInvolved= " )
  DTRACE_CABAC_V( bLeftInvolved )
  DTRACE_CABAC_T( "\tbAboveInvolved= " )
  DTRACE_CABAC_V( bAboveInvolved )
  DTRACE_CABAC_T( "\tbCollocatedInvolved= " )
  DTRACE_CABAC_V( bCollocatedInvolved )
  DTRACE_CABAC_T( "\tbCornerRTInvolved= " )
  DTRACE_CABAC_V( bCornerInvolved )
  DTRACE_CABAC_T( "\tbCornerBLInvolved= " )
  DTRACE_CABAC_V( bCornerBLInvolved )
  DTRACE_CABAC_T( "\n" )
}


Void 
TDecSbac::parseResPredFlag( TComDataCU* pcCU, Bool& rbResPredFlag, UInt uiAbsPartIdx, UInt uiDepth )
{
  UInt uiCtx    = 0;
  UInt uiSymbol = 0;
  m_pcTDecBinIf->decodeBin( uiSymbol, m_cResPredFlagSCModel.get( 0, 0, uiCtx ) );
  rbResPredFlag = ( uiSymbol != 0 );
}


Void TDecSbac::parseMVPIdx      ( TComDataCU* pcCU, Int& riMVPIdx, Int iMVPNum, UInt uiAbsPartIdx, UInt uiDepth, RefPicList eRefList )
{
  UInt uiSymbol;
  xReadUnaryMaxSymbol(uiSymbol, m_cMVPIdxSCModel.get(0), 1, iMVPNum-1);

  riMVPIdx = uiSymbol;
}

Void TDecSbac::parseSplitFlag     ( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth )
{
  if( uiDepth == g_uiMaxCUDepth - g_uiAddCUDepth )
  {
    pcCU->setDepthSubParts( uiDepth, uiAbsPartIdx );
    return;
  }
  
  UInt uiSymbol;
  m_pcTDecBinIf->decodeBin( uiSymbol, m_cCUSplitFlagSCModel.get( 0, 0, pcCU->getCtxSplitFlag( uiAbsPartIdx, uiDepth ) ) );
  DTRACE_CABAC_V( g_nSymbolCounter++ )
  DTRACE_CABAC_T( "\tSplitFlag\n" )
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
  
  if ( pcCU->isIntra( uiAbsPartIdx ) )
  {
#if MTK_DISABLE_INTRA_NxN_SPLIT
    uiSymbol = 1;
    if( uiDepth == g_uiMaxCUDepth - g_uiAddCUDepth )
#endif
    {
      m_pcTDecBinIf->decodeBin( uiSymbol, m_cCUPartSizeSCModel.get( 0, 0, 0) );
    }
    eMode = uiSymbol ? SIZE_2Nx2N : SIZE_NxN;
  }
  else
  {
#if HHI_RMP_SWITCH
    if ( !pcCU->getSlice()->getSPS()->getUseRMP())
    {
      m_pcTDecBinIf->decodeBin( uiSymbol, m_cCUPartSizeSCModel.get( 0, 0, 0) );
      if( uiSymbol )
        uiMode = 0;
      else
        uiMode = 3;
    }
    else
#endif
    {
      UInt uiMaxNumBits = 3;
      for ( UInt ui = 0; ui < uiMaxNumBits; ui++ )
      {
        m_pcTDecBinIf->decodeBin( uiSymbol, m_cCUPartSizeSCModel.get( 0, 0, ui) );
        if ( uiSymbol )
        {
          break;
        }
        uiMode++;
      }
    }
    eMode = (PartSize) uiMode;
    
    if (pcCU->getSlice()->isInterB() && uiMode == 3)
    {
#if HHI_DISABLE_INTER_NxN_SPLIT
      uiSymbol = 0;
      if( uiDepth == g_uiMaxCUDepth - g_uiAddCUDepth )
#endif
      {
        m_pcTDecBinIf->decodeBin( uiSymbol, m_cCUPartSizeSCModel.get( 0, 0, 3) );
      }
      
      if (uiSymbol == 0)
      {
        pcCU->setPredModeSubParts( MODE_INTRA, uiAbsPartIdx, uiDepth );
#if MTK_DISABLE_INTRA_NxN_SPLIT
        if( uiDepth == g_uiMaxCUDepth - g_uiAddCUDepth )
#endif
        {
          m_pcTDecBinIf->decodeBin( uiSymbol, m_cCUPartSizeSCModel.get( 0, 0, 4) );
        }
        if (uiSymbol == 0)
          eMode = SIZE_2Nx2N;
      }
    }
  }
  
  pcCU->setPartSizeSubParts( eMode, uiAbsPartIdx, uiDepth );
  pcCU->setSizeSubParts( g_uiMaxCUWidth>>uiDepth, g_uiMaxCUHeight>>uiDepth, uiAbsPartIdx, uiDepth );
  
  UInt uiTrLevel = 0;
  
  UInt uiWidthInBit  = g_aucConvertToBit[pcCU->getWidth(uiAbsPartIdx)]+2;
  UInt uiTrSizeInBit = g_aucConvertToBit[pcCU->getSlice()->getSPS()->getMaxTrSize()]+2;
  uiTrLevel          = uiWidthInBit >= uiTrSizeInBit ? uiWidthInBit - uiTrSizeInBit : 0;
  
  if( pcCU->getPredictionMode(uiAbsPartIdx) == MODE_INTRA )
  {
    if( pcCU->getPartitionSize( uiAbsPartIdx ) == SIZE_NxN )
    {
      pcCU->setTrIdxSubParts( 1+uiTrLevel, uiAbsPartIdx, uiDepth );
    }
    else
    {
      pcCU->setTrIdxSubParts( uiTrLevel, uiAbsPartIdx, uiDepth );
    }
  }
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
  if ( pcCU->getSlice()->isInterB() )
  {
    pcCU->setPredModeSubParts( (PredMode)iPredMode, uiAbsPartIdx, uiDepth );
    return;
  }
  m_pcTDecBinIf->decodeBin( uiSymbol, m_cCUPredModeSCModel.get( 0, 0, 1 ) );
  iPredMode += uiSymbol;
  pcCU->setPredModeSubParts( (PredMode)iPredMode, uiAbsPartIdx, uiDepth );
}

#if HHI_DMM_WEDGE_INTRA
Void TDecSbac::xParseWedgeFullInfo( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth )
{
  Int iIntraIdx = pcCU->getIntraSizeIdx(uiAbsPartIdx);
  Int iBits = g_aucWedgeFullBitsListIdx[iIntraIdx];

  UInt uiSymbol, uiTabIdx = 0;
  for ( Int i = 0; i < iBits; i++ )
  {
    m_pcTDecBinIf->decodeBin( uiSymbol, m_cIntraWedgeSCModel.get(0, 0, 0) );
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
    m_pcTDecBinIf->decodeBin( uiSymbol, m_cIntraWedgeSCModel.get(0, 0, 0) );
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
  xReadExGolombLevel( uiDC1, m_cIntraWedgeSCModel.get(0, 0, 1) );
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
  xReadExGolombLevel( uiDC2, m_cIntraWedgeSCModel.get(0, 0, 1) );
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
    m_pcTDecBinIf->decodeBin( uiDeltaEnd, m_cIntraWedgeSCModel.get(0, 0, 3) );

    Int iDeltaEnd;
    if( uiDeltaEnd != 0 )
    {
      UInt uiAbsValMinus1;
      UInt uiSymbol;
      m_pcTDecBinIf->decodeBin( uiSymbol, m_cIntraWedgeSCModel.get(0, 0, 3) ); uiAbsValMinus1  = uiSymbol;
      m_pcTDecBinIf->decodeBin( uiSymbol, m_cIntraWedgeSCModel.get(0, 0, 3) ); uiAbsValMinus1 |= uiSymbol << 1;
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
    m_pcTDecBinIf->decodeBin( uiDeltaEnd, m_cIntraWedgeSCModel.get(0, 0, 3) );

    Int iDeltaEnd;
    if( uiDeltaEnd != 0 )
    {
      UInt uiAbsValMinus1;
      UInt uiSymbol;
      m_pcTDecBinIf->decodeBin( uiSymbol, m_cIntraWedgeSCModel.get(0, 0, 3) ); uiAbsValMinus1  = uiSymbol;
      m_pcTDecBinIf->decodeBin( uiSymbol, m_cIntraWedgeSCModel.get(0, 0, 3) ); uiAbsValMinus1 |= uiSymbol << 1;
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
  xReadExGolombLevel( uiDC1, m_cIntraWedgeSCModel.get(0, 0, 1) );
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
  xReadExGolombLevel( uiDC2, m_cIntraWedgeSCModel.get(0, 0, 1) );
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
Void TDecSbac::xParseWedgePredTexDeltaInfo( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth )
{
  UInt uiDC1, uiDC2;
  xReadExGolombLevel( uiDC1, m_cIntraWedgeSCModel.get(0, 0, 1) );
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
  xReadExGolombLevel( uiDC2, m_cIntraWedgeSCModel.get(0, 0, 1) );
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
  xReadExGolombLevel( uiDC1, m_cIntraWedgeSCModel.get(0, 0, 1) );
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
  xReadExGolombLevel( uiDC2, m_cIntraWedgeSCModel.get(0, 0, 1) );
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

#if MTK_DCM_MPM
Void TDecSbac::parseIntraDirLumaAng( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth )
{
  Int  uiIPredMode;
  UInt uiSymbol;
  
#if HHI_DMM_WEDGE_INTRA || HHI_DMM_PRED_TEX
  UInt uiFlag = 0;
  if ( pcCU->getSlice()->getSPS()->isDepth() && pcCU->getSlice()->getSPS()->getUseDMM() && g_uiMaxCUWidth>>uiDepth < 64 )
  {
    m_pcTDecBinIf->decodeBin( uiFlag, m_cIntraDMMSCModel.get(0, 0, 0) );
  }
  if ( uiFlag )
  {
    UInt uiDMMode;

#if HHI_DMM_WEDGE_INTRA && HHI_DMM_PRED_TEX
    m_pcTDecBinIf->decodeBin( uiSymbol, m_cIntraDMMSCModel.get(0, 0, 1) ); uiDMMode  = uiSymbol;
    m_pcTDecBinIf->decodeBin( uiSymbol, m_cIntraDMMSCModel.get(0, 0, 1) ); uiDMMode |= uiSymbol << 1;
    if ( pcCU->getPartitionSize( uiAbsPartIdx ) != SIZE_NxN && g_uiMaxCUWidth>>uiDepth > 4 )
    {
      m_pcTDecBinIf->decodeBin( uiSymbol, m_cIntraDMMSCModel.get(0, 0, 1) ); uiDMMode |= uiSymbol << 2;
    }
#else
        m_pcTDecBinIf->decodeBin( uiSymbol, m_cIntraDMMSCModel.get(0, 0, 1) ); uiDMMode  = uiSymbol;
      if ( pcCU->getPartitionSize( uiAbsPartIdx ) != SIZE_NxN && g_uiMaxCUWidth>>uiDepth > 4 )
      {
      m_pcTDecBinIf->decodeBin( uiSymbol, m_cIntraDMMSCModel.get(0, 0, 1) ); uiDMMode |= uiSymbol << 1;
    }
#endif
    uiIPredMode = uiDMMode + MAX_MODE_ID_INTRA_DIR + 1;

#if HHI_DMM_WEDGE_INTRA
    if( uiIPredMode == DMM_WEDGE_FULL_IDX )          { xParseWedgeFullInfo          ( pcCU, uiAbsPartIdx, uiDepth ); }
    if( uiIPredMode == DMM_WEDGE_FULL_D_IDX )        { xParseWedgeFullDeltaInfo     ( pcCU, uiAbsPartIdx, uiDepth ); }
    if( uiIPredMode == DMM_WEDGE_PREDDIR_IDX )       { xParseWedgePredDirInfo       ( pcCU, uiAbsPartIdx, uiDepth ); }
    if( uiIPredMode == DMM_WEDGE_PREDDIR_D_IDX )     { xParseWedgePredDirDeltaInfo  ( pcCU, uiAbsPartIdx, uiDepth ); }
#endif
#if HHI_DMM_PRED_TEX
    if( uiIPredMode == DMM_WEDGE_PREDTEX_D_IDX )     { xParseWedgePredTexDeltaInfo  ( pcCU, uiAbsPartIdx, uiDepth ); }
    if( uiIPredMode == DMM_CONTOUR_PREDTEX_D_IDX )   { xParseContourPredTexDeltaInfo( pcCU, uiAbsPartIdx, uiDepth ); }
#endif
  }
  else
#endif
  {
    Int iIntraIdx = pcCU->getIntraSizeIdx(uiAbsPartIdx);
   
    Int uiPreds[2] = {-1, -1};
    Int uiPredNum = pcCU->getIntraDirLumaPredictor(uiAbsPartIdx, uiPreds);  

    m_pcTDecBinIf->decodeBin( uiSymbol, m_cCUIntraPredSCModel.get( 0, 0, 0) );
   
    if ( uiSymbol )
    {
      if(uiPredNum == 1)   
      {
        uiIPredMode = uiPreds[0];
      }
      else 
      {
        m_pcTDecBinIf->decodeBin( uiSymbol, m_cCUIntraPredSCModel.get( 0, 0, 2) );
        uiIPredMode = uiPreds[uiSymbol];
      }
    }
    else
    {
      if ( g_aucIntraModeBitsAng[iIntraIdx] < 6 )
      {
        m_pcTDecBinIf->decodeBin( uiSymbol, m_cCUIntraPredSCModel.get( 0, 0, 1 ) ); uiIPredMode  = uiSymbol;
        if ( g_aucIntraModeBitsAng[iIntraIdx] > 2 ) { m_pcTDecBinIf->decodeBin( uiSymbol, m_cCUIntraPredSCModel.get( 0, 0, 1 ) ); uiIPredMode |= uiSymbol << 1; }
        if ( g_aucIntraModeBitsAng[iIntraIdx] > 3 ) { m_pcTDecBinIf->decodeBin( uiSymbol, m_cCUIntraPredSCModel.get( 0, 0, 1 ) ); uiIPredMode |= uiSymbol << 2; }
        if ( g_aucIntraModeBitsAng[iIntraIdx] > 4 ) { m_pcTDecBinIf->decodeBin( uiSymbol, m_cCUIntraPredSCModel.get( 0, 0, 1 ) ); uiIPredMode |= uiSymbol << 3; }
      }
      else
      {
        m_pcTDecBinIf->decodeBin( uiSymbol, m_cCUIntraPredSCModel.get( 0, 0, 1 ) ); uiIPredMode  = uiSymbol;
        m_pcTDecBinIf->decodeBin( uiSymbol, m_cCUIntraPredSCModel.get( 0, 0, 1 ) ); uiIPredMode |= uiSymbol << 1;
        m_pcTDecBinIf->decodeBin( uiSymbol, m_cCUIntraPredSCModel.get( 0, 0, 1 ) ); uiIPredMode |= uiSymbol << 2;
        m_pcTDecBinIf->decodeBin( uiSymbol, m_cCUIntraPredSCModel.get( 0, 0, 1 ) ); uiIPredMode |= uiSymbol << 3;
        m_pcTDecBinIf->decodeBin( uiSymbol, m_cCUIntraPredSCModel.get( 0, 0, 1 ) ); uiIPredMode |= uiSymbol << 4;

        if (uiIPredMode == 31)
        { // Escape coding for the last two modes
          m_pcTDecBinIf->decodeBin( uiSymbol, m_cCUIntraPredSCModel.get( 0, 0, 1 ) );
          uiIPredMode = uiSymbol ? 32 : 31;
        }
      }

      for(UInt i = 0; i < uiPredNum; i++)
      {
        if(uiIPredMode >= uiPreds[i]) {  uiIPredMode ++; }
      }
    }
#if ADD_PLANAR_MODE
    if (uiIPredMode == 2)
    {
      UInt planarFlag;
      m_pcTDecBinIf->decodeBin( planarFlag, m_cPlanarFlagSCModel.get(0,0,0) );
      if ( planarFlag )
      {
        uiIPredMode = PLANAR_IDX;
      }
    }
#endif
  }

  pcCU->setLumaIntraDirSubParts( (UChar)uiIPredMode, uiAbsPartIdx, uiDepth );
}
#else
Void TDecSbac::parseIntraDirLumaAng  ( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth )
{
  Int  uiIPredMode;
  UInt uiSymbol;

#if HHI_DMM_WEDGE_INTRA || HHI_DMM_PRED_TEX
  UInt uiFlag = 0;
  if ( pcCU->getSlice()->getSPS()->isDepth() && pcCU->getSlice()->getSPS()->getUseDMM() && g_uiMaxCUWidth>>uiDepth < 64 )
  {
    m_pcTDecBinIf->decodeBin( uiFlag, m_cIntraDMMSCModel.get(0, 0, 0) );
  }
  if ( uiFlag )
  {
    UInt uiDMMode;

#if HHI_DMM_WEDGE_INTRA && HHI_DMM_PRED_TEX
    m_pcTDecBinIf->decodeBin( uiSymbol, m_cIntraDMMSCModel.get(0, 0, 1) ); uiDMMode  = uiSymbol;
    m_pcTDecBinIf->decodeBin( uiSymbol, m_cIntraDMMSCModel.get(0, 0, 1) ); uiDMMode |= uiSymbol << 1;
      if ( pcCU->getPartitionSize( uiAbsPartIdx ) != SIZE_NxN && g_uiMaxCUWidth>>uiDepth > 4 )
      {
      m_pcTDecBinIf->decodeBin( uiSymbol, m_cIntraDMMSCModel.get(0, 0, 1) ); uiDMMode |= uiSymbol << 2;
        }
#else
      m_pcTDecBinIf->decodeBin( uiSymbol, m_cIntraDMMSCModel.get(0, 0, 1) ); uiDMMode  = uiSymbol;
      if ( pcCU->getPartitionSize( uiAbsPartIdx ) != SIZE_NxN && g_uiMaxCUWidth>>uiDepth > 4 )
      {
      m_pcTDecBinIf->decodeBin( uiSymbol, m_cIntraDMMSCModel.get(0, 0, 1) ); uiDMMode |= uiSymbol << 1;
    }
#endif
    uiIPredMode = g_aucAdditionalIntraModeList[uiDMMode];

#if HHI_DMM_WEDGE_INTRA
    if( uiIPredMode == DMM_WEDGE_FULL_IDX )          { xParseWedgeFullInfo          ( pcCU, uiAbsPartIdx, uiDepth ); }
    if( uiIPredMode == DMM_WEDGE_FULL_D_IDX )        { xParseWedgeFullDeltaInfo     ( pcCU, uiAbsPartIdx, uiDepth ); }
    if( uiIPredMode == DMM_WEDGE_PREDDIR_IDX )       { xParseWedgePredDirInfo       ( pcCU, uiAbsPartIdx, uiDepth ); }
    if( uiIPredMode == DMM_WEDGE_PREDDIR_D_IDX )     { xParseWedgePredDirDeltaInfo  ( pcCU, uiAbsPartIdx, uiDepth ); }
#endif
#if HHI_DMM_PRED_TEX
    if( uiIPredMode == DMM_WEDGE_PREDTEX_D_IDX )     { xParseWedgePredTexDeltaInfo  ( pcCU, uiAbsPartIdx, uiDepth ); }
    if( uiIPredMode == DMM_CONTOUR_PREDTEX_D_IDX )   { xParseContourPredTexDeltaInfo( pcCU, uiAbsPartIdx, uiDepth ); }
#endif
  }
  else
#endif
  {
  Int  iMostProbable = pcCU->getMostProbableIntraDirLuma( uiAbsPartIdx );
  
  m_pcTDecBinIf->decodeBin( uiSymbol, m_cCUIntraPredSCModel.get( 0, 0, 0) );
  
  if ( uiSymbol )
    uiIPredMode = iMostProbable;
  else
  {
    Int iIntraIdx = pcCU->getIntraSizeIdx(uiAbsPartIdx);
    if ( g_aucIntraModeBitsAng[iIntraIdx] < 6 )
    {
      m_pcTDecBinIf->decodeBin( uiSymbol, m_cCUIntraPredSCModel.get( 0, 0, 1 ) ); uiIPredMode  = uiSymbol;
      if ( g_aucIntraModeBitsAng[iIntraIdx] > 2 ) { m_pcTDecBinIf->decodeBin( uiSymbol, m_cCUIntraPredSCModel.get( 0, 0, 1 ) ); uiIPredMode |= uiSymbol << 1; }
      if ( g_aucIntraModeBitsAng[iIntraIdx] > 3 ) { m_pcTDecBinIf->decodeBin( uiSymbol, m_cCUIntraPredSCModel.get( 0, 0, 1 ) ); uiIPredMode |= uiSymbol << 2; }
      if ( g_aucIntraModeBitsAng[iIntraIdx] > 4 ) { m_pcTDecBinIf->decodeBin( uiSymbol, m_cCUIntraPredSCModel.get( 0, 0, 1 ) ); uiIPredMode |= uiSymbol << 3; }
    }
    else
    {
      m_pcTDecBinIf->decodeBin( uiSymbol, m_cCUIntraPredSCModel.get( 0, 0, 1 ) ); uiIPredMode  = uiSymbol;
      m_pcTDecBinIf->decodeBin( uiSymbol, m_cCUIntraPredSCModel.get( 0, 0, 1 ) ); uiIPredMode |= uiSymbol << 1;
      m_pcTDecBinIf->decodeBin( uiSymbol, m_cCUIntraPredSCModel.get( 0, 0, 1 ) ); uiIPredMode |= uiSymbol << 2;
      m_pcTDecBinIf->decodeBin( uiSymbol, m_cCUIntraPredSCModel.get( 0, 0, 1 ) ); uiIPredMode |= uiSymbol << 3;
      m_pcTDecBinIf->decodeBin( uiSymbol, m_cCUIntraPredSCModel.get( 0, 0, 1 ) ); uiIPredMode |= uiSymbol << 4;
      
      if (uiIPredMode == 31)
      { // Escape coding for the last two modes
        m_pcTDecBinIf->decodeBin( uiSymbol, m_cCUIntraPredSCModel.get( 0, 0, 1 ) );
        uiIPredMode = uiSymbol ? 32 : 31;
      }
    }
    
    if (uiIPredMode >= iMostProbable)
      uiIPredMode++;
  }
  
#if ADD_PLANAR_MODE
  if (uiIPredMode == 2)
  {
    UInt planarFlag;
    m_pcTDecBinIf->decodeBin( planarFlag, m_cPlanarFlagSCModel.get(0,0,0) );
    if ( planarFlag )
    {
      uiIPredMode = PLANAR_IDX;
    }
  }
#endif
  }

  pcCU->setLumaIntraDirSubParts( (UChar)uiIPredMode, uiAbsPartIdx, uiDepth );
}
#endif
Void TDecSbac::parseIntraDirChroma( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth )
{
  UInt uiSymbol;
  
#if CHROMA_CODEWORD
  UInt uiMode = pcCU->getLumaIntraDir(uiAbsPartIdx);
#if ADD_PLANAR_MODE
  if ( (uiMode == 2 ) || (uiMode == PLANAR_IDX) )
  {
    uiMode = 4;
  }
#endif

#if LM_CHROMA
  Int  iMaxMode = pcCU->getSlice()->getSPS()->getUseLMChroma() ? 3 : 4;
  Int  iMax = uiMode < iMaxMode ? 2 : 3; 
  
  m_pcTDecBinIf->decodeBin( uiSymbol, m_cCUChromaPredSCModel.get( 0, 0, pcCU->getCtxIntraDirChroma( uiAbsPartIdx ) ) );
  
  if ( uiSymbol )
  {
    xReadUnaryMaxSymbol( uiSymbol, m_cCUChromaPredSCModel.get( 0, 0 ) + 3, 0, iMax );
    uiSymbol++;
  }
  
  //switch codeword
  if (uiSymbol == 0)
  {
    uiSymbol = 4;
  } 
  else if (uiSymbol == 1 && pcCU->getSlice()->getSPS()->getUseLMChroma())
  {
    uiSymbol = 3;
  }
  else 
  {
#if CHROMA_CODEWORD_SWITCH 
    uiSymbol = ChromaMapping[iMax-2][uiSymbol];
#endif

    if (pcCU->getSlice()->getSPS()->getUseLMChroma())
       uiSymbol --;

    if (uiSymbol <= uiMode)
       uiSymbol --;
  }
#else // <-- LM_CHROMA
  Int  iMax = uiMode < 4 ? 2 : 3;
  
  m_pcTDecBinIf->decodeBin( uiSymbol, m_cCUChromaPredSCModel.get( 0, 0, pcCU->getCtxIntraDirChroma( uiAbsPartIdx ) ) );
  
  if ( uiSymbol )
  {
    xReadUnaryMaxSymbol( uiSymbol, m_cCUChromaPredSCModel.get( 0, 0 ) + 3, 0, iMax );
    uiSymbol++;
  }
  
  //switch codeword
  if (uiSymbol == 0)
  {
    uiSymbol = 4;
  } 
#if CHROMA_CODEWORD_SWITCH 
  else
  {
    uiSymbol = ChromaMapping[iMax-2][uiSymbol];
    if (uiSymbol <= uiMode)
    {
      uiSymbol --;
    }
  }
#else
  else if (uiSymbol <= uiMode)
  {
    uiSymbol --;
  }
#endif
#endif // <-- LM_CHROMA

#else // CHROMA_CODEWORD
  m_pcTDecBinIf->decodeBin( uiSymbol, m_cCUChromaPredSCModel.get( 0, 0, pcCU->getCtxIntraDirChroma( uiAbsPartIdx ) ) );
  
  if ( uiSymbol )
  {
    xReadUnaryMaxSymbol( uiSymbol, m_cCUChromaPredSCModel.get( 0, 0 ) + 3, 0, 3 );
    uiSymbol++;
  }
#endif
  
#if ADD_PLANAR_MODE
  if (uiSymbol == 2)
  {
#if CHROMA_CODEWORD
    uiMode = pcCU->getLumaIntraDir(uiAbsPartIdx);
    if (uiMode == 2)
    {
      uiSymbol = PLANAR_IDX;
    }
    else if (uiMode != PLANAR_IDX)
#endif
    {
      UInt planarFlag;
      m_pcTDecBinIf->decodeBin( planarFlag, m_cPlanarFlagSCModel.get(0,0,1) );
      if ( planarFlag )
      {
        uiSymbol = PLANAR_IDX;
      }
    }
  }
#endif
  pcCU->setChromIntraDirSubParts( uiSymbol, uiAbsPartIdx, uiDepth );
  
  return;
}

Void TDecSbac::parseInterDir( TComDataCU* pcCU, UInt& ruiInterDir, UInt uiAbsPartIdx, UInt uiDepth )
{
  UInt uiSymbol;
  UInt uiCtx = pcCU->getCtxInterDir( uiAbsPartIdx );
  
  m_pcTDecBinIf->decodeBin( uiSymbol, m_cCUInterDirSCModel.get( 0, 0, uiCtx ) );
  
  if ( uiSymbol )
  {
    uiSymbol = 2;
  }
#if DCM_COMB_LIST
  else if(pcCU->getSlice()->getNumRefIdx(REF_PIC_LIST_C) > 0)
  {
    uiSymbol = 0;
  }
#endif
  else if ( pcCU->getSlice()->getNoBackPredFlag() )
  {
    uiSymbol = 0;
  }
  else
  {
    m_pcTDecBinIf->decodeBin( uiSymbol, m_cCUInterDirSCModel.get( 0, 0, 3 ) );
  }
  uiSymbol++;
  ruiInterDir = uiSymbol;
  return;
}

Void TDecSbac::parseRefFrmIdx( TComDataCU* pcCU, Int& riRefFrmIdx, UInt uiAbsPartIdx, UInt uiDepth, RefPicList eRefList )
{
  UInt uiSymbol;

#if DCM_COMB_LIST
  if(pcCU->getSlice()->getNumRefIdx(REF_PIC_LIST_C ) > 0 && eRefList==REF_PIC_LIST_C)
  {
    UInt uiCtx;

    uiCtx = pcCU->getCtxRefIdx( uiAbsPartIdx, RefPicList(pcCU->getSlice()->getListIdFromIdxOfLC(0)) );

    m_pcTDecBinIf->decodeBin ( uiSymbol, m_cCURefPicSCModel.get( 0, 0, uiCtx ) );

    if ( uiSymbol )
    {
      xReadUnaryMaxSymbol( uiSymbol, &m_cCURefPicSCModel.get( 0, 0, 4 ), 1, pcCU->getSlice()->getNumRefIdx( REF_PIC_LIST_C )-2 );

      uiSymbol++;
    }

    riRefFrmIdx = uiSymbol;
  }
  else
  {
#endif

  UInt uiCtx = pcCU->getCtxRefIdx( uiAbsPartIdx, eRefList );
  
  m_pcTDecBinIf->decodeBin ( uiSymbol, m_cCURefPicSCModel.get( 0, 0, uiCtx ) );
  if ( uiSymbol )
  {
    xReadUnaryMaxSymbol( uiSymbol, &m_cCURefPicSCModel.get( 0, 0, 4 ), 1, pcCU->getSlice()->getNumRefIdx( eRefList )-2 );
    
    uiSymbol++;
  }
  riRefFrmIdx = uiSymbol;

#if DCM_COMB_LIST
  }
#endif

  return;
}

Void TDecSbac::parseViewIdx(Int &riViewIdx)
{
  assert(0) ; // not implemented yet
  UInt uiSymbol;
  xReadUnaryMaxSymbol(uiSymbol, &m_cViewIdxSCModel.get(0,0,4), 1, MAX_NUMBER_VIEWS);
  riViewIdx = uiSymbol;
}

Void TDecSbac::parseMvd( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiPartIdx, UInt uiDepth, RefPicList eRefList )
{
  Int iHor, iVer;
  UInt uiAbsPartIdxL, uiAbsPartIdxA;
#if MVD_CTX
  Int iHorPredL, iVerPredL;
  Int iHorPredA, iVerPredA;
#else
  Int iHorPred, iVerPred;
#endif

  TComDataCU* pcCUL   = pcCU->getPULeft ( uiAbsPartIdxL, pcCU->getZorderIdxInCU() + uiAbsPartIdx );
  TComDataCU* pcCUA   = pcCU->getPUAbove( uiAbsPartIdxA, pcCU->getZorderIdxInCU() + uiAbsPartIdx );
  
  TComCUMvField* pcCUMvFieldL = ( pcCUL == NULL || pcCUL->isIntra( uiAbsPartIdxL ) ) ? NULL : pcCUL->getCUMvField( eRefList );
  TComCUMvField* pcCUMvFieldA = ( pcCUA == NULL || pcCUA->isIntra( uiAbsPartIdxA ) ) ? NULL : pcCUA->getCUMvField( eRefList );

#if MVD_CTX
  iHorPredL = ( (pcCUMvFieldL == NULL) ? 0 : pcCUMvFieldL->getMvd( uiAbsPartIdxL ).getAbsHor() );
  iVerPredL = ( (pcCUMvFieldL == NULL) ? 0 : pcCUMvFieldL->getMvd( uiAbsPartIdxL ).getAbsVer() );
  iHorPredA = ( (pcCUMvFieldA == NULL) ? 0 : pcCUMvFieldA->getMvd( uiAbsPartIdxA ).getAbsHor() );
  iVerPredA = ( (pcCUMvFieldA == NULL) ? 0 : pcCUMvFieldA->getMvd( uiAbsPartIdxA ).getAbsVer() );
#else
  iHorPred = ( (pcCUMvFieldL == NULL) ? 0 : pcCUMvFieldL->getMvd( uiAbsPartIdxL ).getAbsHor() ) +
  ( (pcCUMvFieldA == NULL) ? 0 : pcCUMvFieldA->getMvd( uiAbsPartIdxA ).getAbsHor() );
  iVerPred = ( (pcCUMvFieldL == NULL) ? 0 : pcCUMvFieldL->getMvd( uiAbsPartIdxL ).getAbsVer() ) +
  ( (pcCUMvFieldA == NULL) ? 0 : pcCUMvFieldA->getMvd( uiAbsPartIdxA ).getAbsVer() );
#endif

  TComMv cTmpMv( 0, 0 );
  pcCU->getCUMvField( eRefList )->setAllMv( cTmpMv, pcCU->getPartitionSize( uiAbsPartIdx ), uiAbsPartIdx, uiPartIdx, uiDepth );
  
#if MVD_CTX
  xReadMvd( iHor, iHorPredL, iHorPredA, 0 );
  xReadMvd( iVer, iVerPredL, iVerPredA, 1 );
#else
  xReadMvd( iHor, iHorPred, 0 );
  xReadMvd( iVer, iVerPred, 1 );
#endif  

  // set mvd
  TComMv cMv( iHor, iVer );
  pcCU->getCUMvField( eRefList )->setAllMvd( cMv, pcCU->getPartitionSize( uiAbsPartIdx ), uiAbsPartIdx, uiPartIdx, uiDepth );
  
  return;
}


Void TDecSbac::parseTransformSubdivFlag( UInt& ruiSubdivFlag, UInt uiLog2TransformBlockSize )
{
  m_pcTDecBinIf->decodeBin( ruiSubdivFlag, m_cCUTransSubdivFlagSCModel.get( 0, 0, uiLog2TransformBlockSize ) );
  DTRACE_CABAC_V( g_nSymbolCounter++ )
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
  const UInt uiCtx = pcCU->getCtxQtRootCbf( uiAbsPartIdx );
  m_pcTDecBinIf->decodeBin( uiSymbol , m_cCUQtRootCbfSCModel.get( 0, 0, uiCtx ) );
  DTRACE_CABAC_V( g_nSymbolCounter++ )
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
  UInt uiDQp;
  Int  iDQp;
  
  m_pcTDecBinIf->decodeBin( uiDQp, m_cCUDeltaQpSCModel.get( 0, 0, 0 ) );
  
  if ( uiDQp == 0 )
  {
    uiDQp = pcCU->getSlice()->getSliceQp();
  }
  else
  {
    xReadUnarySymbol( uiDQp, &m_cCUDeltaQpSCModel.get( 0, 0, 2 ), 1 );
    iDQp = ( uiDQp + 2 ) / 2;
    
    if ( uiDQp & 1 )
    {
      iDQp = -iDQp;
    }
    uiDQp = pcCU->getSlice()->getSliceQp() + iDQp;
  }
  
  pcCU->setQPSubParts( uiDQp, uiAbsPartIdx, uiDepth );
}

Void TDecSbac::parseQtCbf( TComDataCU* pcCU, UInt uiAbsPartIdx, TextType eType, UInt uiTrDepth, UInt uiDepth )
{
  UInt uiSymbol;
  const UInt uiCtx = pcCU->getCtxQtCbf( uiAbsPartIdx, eType, uiTrDepth );
  m_pcTDecBinIf->decodeBin( uiSymbol , m_cCUQtCbfSCModel.get( 0, eType ? eType - 1: eType, uiCtx ) );
  
  DTRACE_CABAC_V( g_nSymbolCounter++ )
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


#if PCP_SIGMAP_SIMPLE_LAST
/** Parse (X,Y) position of the last significant coefficient
 * \param uiPosLastX reference to X component of last coefficient
 * \param uiPosLastY reference to Y component of last coefficient
 * \param uiWidth block width
 * \param eTType plane type / luminance or chrominance
 * \param uiCTXIdx block size context
 * \param uiScanIdx scan type (zig-zag, hor, ver)
 * \returns Void
 * This method decodes the X and Y component within a block of the last significant coefficient.
 */
__inline Void TDecSbac::parseLastSignificantXY( UInt& uiPosLastX, UInt& uiPosLastY, const UInt uiWidth, const TextType eTType, const UInt uiCTXIdx, const UInt uiScanIdx )
{
  UInt uiLast;
  const UInt uiCtxOffset = g_uiCtxXYOffset[uiCTXIdx];

  for(uiPosLastX=0; uiPosLastX<uiWidth-1; uiPosLastX++)
  {
    m_pcTDecBinIf->decodeBin( uiLast, m_cCuCtxLastX.get( 0, eTType, uiCtxOffset + g_uiCtxXY[uiPosLastX] ) );
    if(uiLast)
    {
      break;
    }
  }

  for(uiPosLastY=0; uiPosLastY<uiWidth-1; uiPosLastY++)
  {
    m_pcTDecBinIf->decodeBin( uiLast, m_cCuCtxLastY.get( 0, eTType, uiCtxOffset + g_uiCtxXY[uiPosLastY] ) );
    if(uiLast)
    {
      break;
    }
  }

  if( uiScanIdx == SCAN_VER )
  {
    swap( uiPosLastX, uiPosLastY );
  }
}
#endif

Void TDecSbac::parseCoeffNxN( TComDataCU* pcCU, TCoeff* pcCoef, UInt uiAbsPartIdx, UInt uiWidth, UInt uiHeight, UInt uiDepth, TextType eTType )
{
  DTRACE_CABAC_V( g_nSymbolCounter++ )
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
  
  UInt uiCTXIdx;
  
  switch(uiWidth)
  {
    case  2: uiCTXIdx = 6; break;
    case  4: uiCTXIdx = 5; break;
    case  8: uiCTXIdx = 4; break;
    case 16: uiCTXIdx = 3; break;
    case 32: uiCTXIdx = 2; break;
    case 64: uiCTXIdx = 1; break;
    default: uiCTXIdx = 0; break;
  }
  
  eTType = eTType == TEXT_LUMA ? TEXT_LUMA : ( eTType == TEXT_NONE ? TEXT_NONE : TEXT_CHROMA );
  
  //----- parse significance map -----
  const UInt  uiLog2BlockSize   = g_aucConvertToBit[ uiWidth ] + 2;
  const UInt  uiMaxNumCoeff     = 1 << ( uiLog2BlockSize << 1 );
  const UInt  uiMaxNumCoeffM1   = uiMaxNumCoeff - 1;
  const UInt  uiNum4x4Blk       = max<UInt>( 1, uiMaxNumCoeff >> 4 );
#if !PCP_SIGMAP_SIMPLE_LAST
  bool        bLastReceived     = false;
#endif
#if QC_MDCS
  const UInt uiScanIdx = pcCU->getCoefScanIdx(uiAbsPartIdx, uiWidth, eTType==TEXT_LUMA, pcCU->isIntra(uiAbsPartIdx));
#endif //QC_MDCS
  
#if PCP_SIGMAP_SIMPLE_LAST
    //===== decode last significant =====
    UInt uiPosLastX, uiPosLastY;
    parseLastSignificantXY( uiPosLastX, uiPosLastY, uiWidth, eTType, uiCTXIdx, uiScanIdx );
    UInt uiBlkPosLast      = uiPosLastX + (uiPosLastY<<uiLog2BlockSize);
    pcCoef[ uiBlkPosLast ] = 1;

    //===== decode significance flags =====
    for( UInt uiScanPos = 0; uiScanPos < uiMaxNumCoeffM1; uiScanPos++ )
    {
#if QC_MDCS
      UInt uiBlkPos = g_auiSigLastScan[uiScanIdx][uiLog2BlockSize-1][uiScanPos]; 
#else
      UInt  uiBlkPos  = g_auiFrameScanXY[ uiLog2BlockSize-1 ][ uiScanPos ];
#endif //QC_MDCS
      if( uiBlkPosLast == uiBlkPos )
      {
        break;
      }
      UInt  uiPosY    = uiBlkPos >> uiLog2BlockSize;
      UInt  uiPosX    = uiBlkPos - ( uiPosY << uiLog2BlockSize );
      UInt  uiSig     = 0;
      UInt  uiCtxSig  = TComTrQuant::getSigCtxInc( pcCoef, uiPosX, uiPosY, uiLog2BlockSize, uiWidth );
#if SIMPLE_CONTEXT_SIG
      if( uiCtxSig < 4 || eTType )
      {
        m_pcTDecBinIf->decodeBin( uiSig, m_cCUSigSCModel.get( uiCTXIdx-2, eTType, uiCtxSig ) );
      }
      else
      {
        m_pcTDecBinIf->decodeBin( uiSig, m_cCUSigSCModel.get( uiCTXIdx-2 ? uiCTXIdx-2 : 1 , eTType, uiCtxSig ) );
      }
#else
      m_pcTDecBinIf->decodeBin( uiSig, m_cCUSigSCModel.get( uiCTXIdx, eTType, uiCtxSig ) );
#endif
      pcCoef[ uiBlkPos ] = uiSig;
    }

#else
  for( UInt uiScanPos = 0; uiScanPos < uiMaxNumCoeffM1; uiScanPos++ )
  {
#if QC_MDCS
    UInt uiBlkPos = g_auiSigLastScan[uiScanIdx][uiLog2BlockSize-1][uiScanPos]; 
#else
    UInt  uiBlkPos  = g_auiFrameScanXY[ uiLog2BlockSize-1 ][ uiScanPos ];
#endif //QC_MDCS
    UInt  uiPosY    = uiBlkPos >> uiLog2BlockSize;
    UInt  uiPosX    = uiBlkPos - ( uiPosY << uiLog2BlockSize );
    
    //===== code significance flag =====
    UInt  uiSig     = 0;
    UInt  uiCtxSig  = TComTrQuant::getSigCtxInc( pcCoef, uiPosX, uiPosY, uiLog2BlockSize, uiWidth );
#if SIMPLE_CONTEXT_SIG
      if( uiCtxSig < 4 || eTType )
      {
        m_pcTDecBinIf->decodeBin( uiSig, m_cCUSigSCModel.get( uiCTXIdx-2, eTType, uiCtxSig ) );
      }
      else
      {
        m_pcTDecBinIf->decodeBin( uiSig, m_cCUSigSCModel.get( uiCTXIdx-2 ? uiCTXIdx-2 : 1 , eTType, uiCtxSig ) );
      }
#else
    m_pcTDecBinIf->decodeBin( uiSig, m_cCUSigSCModel.get( uiCTXIdx, eTType, uiCtxSig ) );
#endif
    if( uiSig )
    {
      pcCoef[ uiBlkPos ] = 1;
      
      //===== code last flag =====
      UInt  uiLast     = 0;
      UInt  uiCtxLast  = TComTrQuant::getLastCtxInc( uiPosX, uiPosY, uiLog2BlockSize );
      m_pcTDecBinIf->decodeBin( uiLast, m_cCULastSCModel.get( uiCTXIdx, eTType, uiCtxLast ) );
      
      if( uiLast )
      {
        bLastReceived = true;
        break;
      }
    }
  }
  if( !bLastReceived )
  {
    pcCoef[ uiMaxNumCoeffM1 ] = 1;
  }
#endif
  
  /*
   * Sign and bin0 PCP (Section 3.2 and 3.3 of JCTVC-B088)
   */
  Int  c1, c2;
  UInt uiSign;
  UInt uiLevel;
#if E253
  UInt uiGoRiceParam = 0;
#endif

  if( uiNum4x4Blk > 1 )
  {
    Bool b1stBlk  = true;
    UInt uiNumOne = 0;
    
    for( UInt uiSubBlk = 0; uiSubBlk < uiNum4x4Blk; uiSubBlk++ )
    {
      UInt uiCtxSet    = 0;
      UInt uiSubNumSig = 0;
      UInt uiSubPosX   = 0;
      UInt uiSubPosY   = 0;
#if E253
      uiGoRiceParam    = 0;
#endif

      uiSubPosX = g_auiFrameScanX[ g_aucConvertToBit[ uiWidth ] - 1 ][ uiSubBlk ] << 2;
      uiSubPosY = g_auiFrameScanY[ g_aucConvertToBit[ uiWidth ] - 1 ][ uiSubBlk ] << 2;
      
      TCoeff* piCurr = &pcCoef[ uiSubPosX + uiSubPosY * uiWidth ];
      
      for( UInt uiY = 0; uiY < 4; uiY++ )
      {
        for( UInt uiX = 0; uiX < 4; uiX++ )
        {
          if( piCurr[ uiX ] )
          {
            uiSubNumSig++;
          }
        }
        piCurr += uiWidth;
      }
      
      if( uiSubNumSig > 0 )
      {
        c1 = 1;
        c2 = 0;
        
        if( b1stBlk )
        {
          b1stBlk  = false;
          uiCtxSet = 5;
        }
        else
        {
          uiCtxSet = ( uiNumOne >> 2 ) + 1;
          uiNumOne = 0;
        }
        
        for( UInt uiScanPos = 0; uiScanPos < 16; uiScanPos++ )
        {
          UInt  uiBlkPos  = g_auiFrameScanXY[ 1 ][ 15 - uiScanPos ];
          UInt  uiPosY    = uiBlkPos >> 2;
          UInt  uiPosX    = uiBlkPos - ( uiPosY << 2 );
          UInt  uiIndex   = ( ( uiSubPosY + uiPosY ) << uiLog2BlockSize ) + uiSubPosX + uiPosX;
          
          uiLevel = pcCoef[ uiIndex ];
          
          if( uiLevel )
          {
            UInt uiCtx = min<UInt>(c1, 4);
            m_pcTDecBinIf->decodeBin( uiLevel, m_cCUOneSCModel.get( 0, eTType, ( uiCtxSet << 2 ) + uiCtxSet + uiCtx ) );
            if( uiLevel == 1 )
            {
              c1      = 0;
              uiLevel = 2;
            }
            else if( c1 )
            {
              c1++;
              uiLevel++;
            }
            else
            {
              uiLevel++;
            }
            pcCoef[ uiIndex ] = uiLevel;
          }
        }
        
        for( UInt uiScanPos = 0; uiScanPos < 16; uiScanPos++ )
        {
          UInt  uiBlkPos  = g_auiFrameScanXY[ 1 ][ 15 - uiScanPos ];
          UInt  uiPosY    = uiBlkPos >> 2;
          UInt  uiPosX    = uiBlkPos - ( uiPosY << 2 );
          UInt  uiIndex   = ( ( uiSubPosY + uiPosY ) << uiLog2BlockSize ) + uiSubPosX + uiPosX;
          
          uiLevel = pcCoef[ uiIndex ];
          
          if( uiLevel )
          {
            if( uiLevel == 2 )
            {
              UInt uiCtx = min<UInt>(c2, 4);
              c2++;
              uiNumOne++;
#if E253
              m_pcTDecBinIf->decodeBin( uiLevel, m_cCUAbsSCModel.get( 0, eTType, ( uiCtxSet << 2 ) + uiCtxSet + uiCtx ) );

              if( uiLevel )
              {
                xReadGoRiceExGolomb( uiLevel, uiGoRiceParam );
                uiLevel += 3;
              }
              else
              {
                uiLevel = 2;
              }
#else
              xReadExGolombLevel( uiLevel, m_cCUAbsSCModel.get( 0, eTType, ( uiCtxSet << 2 ) + uiCtxSet + uiCtx ) );
              uiLevel += 2;
#endif
            }
            pcCoef[ uiIndex ] = uiLevel;
          }
        }
        
        for( UInt uiScanPos = 0; uiScanPos < 16; uiScanPos++ )
        {
          UInt  uiBlkPos  = g_auiFrameScanXY[ 1 ][ 15 - uiScanPos ];
          UInt  uiPosY    = uiBlkPos >> 2;
          UInt  uiPosX    = uiBlkPos - ( uiPosY << 2 );
          UInt  uiIndex   = (uiSubPosY + uiPosY) * uiWidth + uiSubPosX + uiPosX;
          
          uiLevel = pcCoef[ uiIndex ];
          
          if( uiLevel )
          {
            m_pcTDecBinIf->decodeBinEP( uiSign );
            pcCoef[ uiIndex ] = ( uiSign ? -(Int)uiLevel : (Int)uiLevel );
          }
        }
      }
    }
  }
  else
  {
    c1 = 1;
    c2 = 0;
    
    for( UInt uiScanPos = 0; uiScanPos < 16; uiScanPos++ )
    {
      UInt uiIndex = g_auiFrameScanXY[ 1 ][ 15 - uiScanPos ];
      uiLevel = pcCoef[ uiIndex ];
      
      if( uiLevel )
      {
        UInt uiCtx = min<UInt>(c1, 4);
        m_pcTDecBinIf->decodeBin( uiLevel, m_cCUOneSCModel.get( 0, eTType, uiCtx ) );
        if( uiLevel == 1 )
        {
          c1      = 0;
          uiLevel = 2;
        }
        else if( c1 )
        {
          c1++;
          uiLevel++;
        }
        else
        {
          uiLevel++;
        }
        pcCoef[ uiIndex ] = uiLevel;
      }
    }
    
    for( UInt uiScanPos = 0; uiScanPos < 16; uiScanPos++ )
    {
      UInt uiIndex = g_auiFrameScanXY[ 1 ][ 15 - uiScanPos ];
      uiLevel = pcCoef[ uiIndex ];
      
      if( uiLevel )
      {
        if( uiLevel == 2 )
        {
          UInt uiCtx = min<UInt>(c2, 4);
          c2++;
#if E253
          m_pcTDecBinIf->decodeBin( uiLevel, m_cCUAbsSCModel.get( 0, eTType, uiCtx ) );

          if( uiLevel )
          {
            xReadGoRiceExGolomb( uiLevel, uiGoRiceParam );
            uiLevel += 3;
          }
          else
          {
            uiLevel = 2;
          }
#else
          xReadExGolombLevel( uiLevel, m_cCUAbsSCModel.get( 0, eTType, uiCtx ) );
          uiLevel += 2;
#endif
        }
        pcCoef[ uiIndex ] = uiLevel;
      }
    }
    
    for( UInt uiScanPos = 0; uiScanPos < 16; uiScanPos++ )
    {
      UInt uiIndex = g_auiFrameScanXY[ 1 ][ 15 - uiScanPos ];
      uiLevel = pcCoef[ uiIndex ];
      
      if( uiLevel )
      {
        m_pcTDecBinIf->decodeBinEP( uiSign );
        pcCoef[ uiIndex ] = ( uiSign ? -(Int)uiLevel : (Int)uiLevel );
      }
    }
  }
  
  return;
}

Void TDecSbac::parseAlfFlag (UInt& ruiVal)
{
  UInt uiSymbol;
  m_pcTDecBinIf->decodeBin( uiSymbol, m_cALFFlagSCModel.get( 0, 0, 0 ) );
  
  ruiVal = uiSymbol;
}

#if TSB_ALF_HEADER
Void TDecSbac::parseAlfFlagNum( UInt& ruiVal, UInt minValue, UInt depth )
{
  UInt uiLength = 0;
  UInt maxValue = (minValue << (depth*2));
  UInt temp = maxValue - minValue;
  for(UInt i=0; i<32; i++)
  {
    if(temp&0x1)
    {
      uiLength = i+1;
    }
    temp = (temp >> 1);
  }
  ruiVal = 0;
  UInt uiBit;
  if(uiLength)
  {
    while( uiLength-- )
    {
      m_pcTDecBinIf->decodeBinEP( uiBit );
      ruiVal += uiBit << uiLength;
    }
  }
  else
  {
    ruiVal = 0;
  }
  ruiVal += minValue;
}

Void TDecSbac::parseAlfCtrlFlag( UInt &ruiAlfCtrlFlag )
{
  UInt uiSymbol;
  m_pcTDecBinIf->decodeBin( uiSymbol, m_cCUAlfCtrlFlagSCModel.get( 0, 0, 0 ) );
  ruiAlfCtrlFlag = uiSymbol;
}
#endif

Void TDecSbac::parseAlfUvlc (UInt& ruiVal)
{
  UInt uiCode;
  Int  i;
  
  m_pcTDecBinIf->decodeBin( uiCode, m_cALFUvlcSCModel.get( 0, 0, 0 ) );
  if ( uiCode == 0 )
  {
    ruiVal = 0;
    return;
  }
  
  i=1;
  while (1)
  {
    m_pcTDecBinIf->decodeBin( uiCode, m_cALFUvlcSCModel.get( 0, 0, 1 ) );
    if ( uiCode == 0 ) break;
    i++;
  }
  
  ruiVal = i;
}

Void TDecSbac::parseAlfSvlc (Int&  riVal)
{
  UInt uiCode;
  Int  iSign;
  Int  i;
  
  m_pcTDecBinIf->decodeBin( uiCode, m_cALFSvlcSCModel.get( 0, 0, 0 ) );
  
  if ( uiCode == 0 )
  {
    riVal = 0;
    return;
  }
  
  // read sign
  m_pcTDecBinIf->decodeBin( uiCode, m_cALFSvlcSCModel.get( 0, 0, 1 ) );
  
  if ( uiCode == 0 ) iSign =  1;
  else               iSign = -1;
  
  // read magnitude
  i=1;
  while (1)
  {
    m_pcTDecBinIf->decodeBin( uiCode, m_cALFSvlcSCModel.get( 0, 0, 2 ) );
    if ( uiCode == 0 ) break;
    i++;
  }
  
  riVal = i*iSign;
}

#if MTK_SAO
Void TDecSbac::parseAoFlag (UInt& ruiVal)
{
  UInt uiSymbol;
  m_pcTDecBinIf->decodeBin( uiSymbol, m_cAOFlagSCModel.get( 0, 0, 0 ) );

  ruiVal = uiSymbol;
}

Void TDecSbac::parseAoUvlc (UInt& ruiVal)
{
  UInt uiCode;
  Int  i;

  m_pcTDecBinIf->decodeBin( uiCode, m_cAOUvlcSCModel.get( 0, 0, 0 ) );
  if ( uiCode == 0 )
  {
    ruiVal = 0;
    return;
  }

  i=1;
  while (1)
  {
    m_pcTDecBinIf->decodeBin( uiCode, m_cAOUvlcSCModel.get( 0, 0, 1 ) );
    if ( uiCode == 0 ) break;
    i++;
  }

  ruiVal = i;
}

Void TDecSbac::parseAoSvlc (Int&  riVal)
{
  UInt uiCode;
  Int  iSign;
  Int  i;

  m_pcTDecBinIf->decodeBin( uiCode, m_cAOSvlcSCModel.get( 0, 0, 0 ) );

  if ( uiCode == 0 )
  {
    riVal = 0;
    return;
  }

  // read sign
  m_pcTDecBinIf->decodeBin( uiCode, m_cAOSvlcSCModel.get( 0, 0, 1 ) );

  if ( uiCode == 0 ) iSign =  1;
  else               iSign = -1;

  // read magnitude
  i=1;
  while (1)
  {
    m_pcTDecBinIf->decodeBin( uiCode, m_cAOSvlcSCModel.get( 0, 0, 2 ) );
    if ( uiCode == 0 ) break;
    i++;
  }

  riVal = i*iSign;
}
#endif
