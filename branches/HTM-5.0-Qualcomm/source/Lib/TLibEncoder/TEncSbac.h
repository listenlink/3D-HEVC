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

/** \file     TEncSbac.h
    \brief    Context-adaptive entropy encoder class (header)
*/

#ifndef __TENCSBAC__
#define __TENCSBAC__

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "TLibCommon/TComBitStream.h"
#include "TLibCommon/ContextTables.h"
#include "TLibCommon/ContextModel.h"
#include "TLibCommon/ContextModel3DBuffer.h"
#include "TEncEntropy.h"
#include "TEncBinCoder.h"
#include "TEncBinCoderCABAC.h"
#if FAST_BIT_EST
#include "TEncBinCoderCABACCounter.h"
#endif

class TEncTop;

//! \ingroup TLibEncoder
//! \{

// ====================================================================================================================
// Class definition
// ====================================================================================================================

/// SBAC encoder class
class TEncSbac : public TEncEntropyIf
{
public:
  TEncSbac();
  virtual ~TEncSbac();
  
  Void  init                   ( TEncBinIf* p )  { m_pcBinIf = p; }
  Void  uninit                 ()                { m_pcBinIf = 0; }
  
  //  Virtual list
  Void  resetEntropy           ();
#if CABAC_INIT_FLAG
  Void  determineCabacInitIdx  ();
#endif
  Void  setBitstream           ( TComBitIf* p )  { m_pcBitIf = p; m_pcBinIf->init( p ); }
  Void  setSlice               ( TComSlice* p )  { m_pcSlice = p;                       }
  
  Bool  getAlfCtrl             ()                         { return m_bAlfCtrl;          }
  UInt  getMaxAlfCtrlDepth     ()                         { return m_uiMaxAlfCtrlDepth; }
  Void  setAlfCtrl             ( Bool bAlfCtrl          ) { m_bAlfCtrl          = bAlfCtrl;          }
  Void  setMaxAlfCtrlDepth     ( UInt uiMaxAlfCtrlDepth ) { m_uiMaxAlfCtrlDepth = uiMaxAlfCtrlDepth; }
  
  // SBAC RD
  Void  resetCoeffCost         ()                { m_uiCoeffCost = 0;  }
  UInt  getCoeffCost           ()                { return  m_uiCoeffCost;  }
  
  Void  load                   ( TEncSbac* pScr  );
  Void  loadIntraDirModeLuma   ( TEncSbac* pScr  );
  Void  store                  ( TEncSbac* pDest );
  Void  loadContexts           ( TEncSbac* pScr  );
  Void  resetBits              ()                { m_pcBinIf->resetBits(); m_pcBitIf->resetBits(); }
  UInt  getNumberOfWrittenBits ()                { return m_pcBinIf->getNumWrittenBits(); }
  //--SBAC RD

#if VIDYO_VPS_INTEGRATION|QC_MVHEVC_B0046
  Void  codeVPS                 ( TComVPS* pcVPS );
#endif
  
#if HHI_MPI
  Void  codeSPS                 ( TComSPS* pcSPS, Bool bIsDepth );
#else
  Void  codeSPS                 ( TComSPS* pcSPS     );
#endif
  Void  codePPS                 ( TComPPS* pcPPS     );
  void codeSEI(const SEI&);
  Void  codeSliceHeader         ( TComSlice* pcSlice );
  Void codeTileMarkerFlag(TComSlice* pcSlice) {printf("Not supported\n"); assert(0); exit(1);}
#if TILES_WPP_ENTRY_POINT_SIGNALLING
  Void  codeTilesWPPEntryPoint( TComSlice* pSlice );
#else
  Void  codeSliceHeaderSubstreamTable( TComSlice* pcSlice );
#endif
  Void  codeTerminatingBit      ( UInt uilsLast      );
  Void  codeSliceFinish         ();
#if OL_FLUSH
  Void  codeFlush               ();
  Void  encodeStart             ();
#endif
  
  Void  codeAlfFlag       ( UInt uiCode );
  Void  codeAlfUvlc       ( UInt uiCode );
  Void  codeAlfSvlc       ( Int  uiCode );
  Void  codeAlfCtrlDepth  ();
#if LCU_SYNTAX_ALF
  Void codeAPSAlflag(UInt uiCode) {assert (0);  return;}
  Void codeAlfFixedLengthIdx( UInt idx, UInt maxValue){ assert (0);  return;}
#endif

  Void codeAlfCtrlFlag       ( UInt uiSymbol );
  Void  codeApsExtensionFlag () { assert (0); return; };
  Void  codeSaoFlag       ( UInt uiCode );
  Void  codeSaoUvlc       ( UInt uiCode );
  Void  codeSaoSvlc       ( Int  uiCode );
#if SAO_UNIT_INTERLEAVING
  Void  codeSaoRun        ( UInt  uiCode, UInt uiMaxValue  ) {;}
  Void  codeSaoMergeLeft  ( UInt  uiCode, UInt uiCompIdx );
  Void  codeSaoMergeUp    ( UInt  uiCode);
  Void  codeSaoTypeIdx    ( UInt  uiCode);
  Void  codeSaoUflc       ( UInt  uiCode);
#endif
  Void  codeScalingList      ( TComScalingList* scalingList     ){ assert (0);  return;};
  
#if RWTH_SDC_DLT_B0036
  Void codeSDCFlag          ( TComDataCU* pcCU, UInt uiAbsPartIdx );
  Void codeSDCResidualData  ( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiSegment );
  Void codeSDCPredMode      ( TComDataCU* pcCU, UInt uiAbsPartIdx );
#endif

private:
  Void  xWriteUnarySymbol    ( UInt uiSymbol, ContextModel* pcSCModel, Int iOffset );
  Void  xWriteUnaryMaxSymbol ( UInt uiSymbol, ContextModel* pcSCModel, Int iOffset, UInt uiMaxSymbol );
  Void  xWriteEpExGolomb     ( UInt uiSymbol, UInt uiCount );
  Void  xWriteGoRiceExGolomb ( UInt uiSymbol, UInt &ruiGoRiceParam );
  Void  xWriteTerminatingBit ( UInt uiBit );
  
  Void  xCopyFrom            ( TEncSbac* pSrc );
  Void  xCopyContextsFrom    ( TEncSbac* pSrc );  
  
  Void codeAPSInitInfo(TComAPS* pcAPS) {printf("Not supported in codeAPSInitInfo()\n"); assert(0); exit(1);}
  Void codeFinish     (Bool bEnd)      { m_pcBinIf->encodeFlush(bEnd); }  //<! flush bits when CABAC termination
  Void codeDFFlag( UInt uiCode, const Char *pSymbolName )       {printf("Not supported in codeDFFlag()\n"); assert(0); exit(1);};
  Void codeDFSvlc( Int iCode, const Char *pSymbolName )         {printf("Not supported in codeDFSvlc()\n"); assert(0); exit(1);};

#if HHI_DMM_WEDGE_INTRA || HHI_DMM_PRED_TEX
  Void  xWriteExGolombLevel  ( UInt uiSymbol, ContextModel& rcSCModel  );
#endif
#if HHI_DMM_WEDGE_INTRA
  Void  xCodeWedgeFullInfo          ( TComDataCU* pcCU, UInt uiAbsPartIdx );
  Void  xCodeWedgeFullDeltaInfo     ( TComDataCU* pcCU, UInt uiAbsPartIdx );

  Void  xCodeWedgePredDirInfo       ( TComDataCU* pcCU, UInt uiAbsPartIdx );
  Void  xCodeWedgePredDirDeltaInfo  ( TComDataCU* pcCU, UInt uiAbsPartIdx );
#endif
#if HHI_DMM_PRED_TEX
  Void  xCodeWedgePredTexDeltaInfo  ( TComDataCU* pcCU, UInt uiAbsPartIdx );
  Void  xCodeContourPredTexDeltaInfo( TComDataCU* pcCU, UInt uiAbsPartIdx );
#endif
#if LGE_EDGE_INTRA_A0070
  Void  xCodeEdgeIntraInfo( TComDataCU* pcCU, UInt uiPartIdx );
#endif

protected:
  TComBitIf*    m_pcBitIf;
  TComSlice*    m_pcSlice;
  TEncBinIf*    m_pcBinIf;
  Bool          m_bAlfCtrl;
  
  //SBAC RD
  UInt          m_uiCoeffCost;
  
  // Adaptive loop filter
  UInt          m_uiMaxAlfCtrlDepth;
  Int           m_iSliceGranularity; //!< slice granularity
  //--Adaptive loop filter
  
public:

  /// set slice granularity
  Void setSliceGranularity(Int iSliceGranularity)  {m_iSliceGranularity = iSliceGranularity;}

  /// get slice granularity
  Int  getSliceGranularity()                       {return m_iSliceGranularity;             }
  Void codeAlfCtrlFlag   ( TComDataCU* pcCU, UInt uiAbsPartIdx );
  Void codeSkipFlag      ( TComDataCU* pcCU, UInt uiAbsPartIdx );
#if LGE_ILLUCOMP_B0045
  Void codeICFlag        ( TComDataCU* pcCU, UInt uiAbsPartIdx );
#endif
  Void codeMergeFlag     ( TComDataCU* pcCU, UInt uiAbsPartIdx );
  Void codeMergeIndex    ( TComDataCU* pcCU, UInt uiAbsPartIdx );
#if HHI_INTER_VIEW_RESIDUAL_PRED
  Void codeResPredFlag   ( TComDataCU* pcCU, UInt uiAbsPartIdx );
#endif
  Void codeSplitFlag     ( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth );
#if HHI_INTER_VIEW_MOTION_PRED
  Void codeMVPIdx        ( TComDataCU* pcCU, UInt uiAbsPartIdx, RefPicList eRefList, Int iNum );
#else
  Void codeMVPIdx        ( TComDataCU* pcCU, UInt uiAbsPartIdx, RefPicList eRefList );
#endif
  
  Void codePartSize      ( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth );
  Void codePredMode      ( TComDataCU* pcCU, UInt uiAbsPartIdx );
#if BURST_IPCM
  Void codeIPCMInfo      ( TComDataCU* pcCU, UInt uiAbsPartIdx, Int numIPCM, Bool firstIPCMFlag);
#else
  Void codeIPCMInfo      ( TComDataCU* pcCU, UInt uiAbsPartIdx );
#endif
  Void codeTransformSubdivFlag ( UInt uiSymbol, UInt uiCtx );
  Void codeQtCbf               ( TComDataCU* pcCU, UInt uiAbsPartIdx, TextType eType, UInt uiTrDepth );
  Void codeQtRootCbf           ( TComDataCU* pcCU, UInt uiAbsPartIdx );
  
  Void codeIntraDirLumaAng     ( TComDataCU* pcCU, UInt uiAbsPartIdx );
  
  Void codeIntraDirChroma      ( TComDataCU* pcCU, UInt uiAbsPartIdx );
  Void codeInterDir            ( TComDataCU* pcCU, UInt uiAbsPartIdx );
  Void codeRefFrmIdx           ( TComDataCU* pcCU, UInt uiAbsPartIdx, RefPicList eRefList );
  Void codeMvd                 ( TComDataCU* pcCU, UInt uiAbsPartIdx, RefPicList eRefList );
  
  Void codeDeltaQP             ( TComDataCU* pcCU, UInt uiAbsPartIdx );
  
  Void codeLastSignificantXY ( UInt uiPosX, UInt uiPosY, Int width, Int height, TextType eTType, UInt uiScanIdx );
  Void codeCoeffNxN            ( TComDataCU* pcCU, TCoeff* pcCoef, UInt uiAbsPartIdx, UInt uiWidth, UInt uiHeight, UInt uiDepth, TextType eTType );
  
  // -------------------------------------------------------------------------------------------------------------------
  // for RD-optimizatioon
  // -------------------------------------------------------------------------------------------------------------------
  
  Void estBit               (estBitsSbacStruct* pcEstBitsSbac, Int width, Int height, TextType eTType);
  Void estCBFBit                     ( estBitsSbacStruct* pcEstBitsSbac, UInt uiCTXIdx, TextType eTType );
  Void estSignificantCoeffGroupMapBit( estBitsSbacStruct* pcEstBitsSbac, UInt uiCTXIdx, TextType eTType );
  Void estSignificantMapBit          ( estBitsSbacStruct* pcEstBitsSbac, Int width, Int height, TextType eTType );
  Void estSignificantCoefficientsBit ( estBitsSbacStruct* pcEstBitsSbac, UInt uiCTXIdx, TextType eTType );
  
  Void updateContextTables           ( SliceType eSliceType, Int iQp, Bool bExecuteFinish=true  );
  Void updateContextTables           ( SliceType eSliceType, Int iQp  ) { this->updateContextTables( eSliceType, iQp, true); };
  Void writeTileMarker               ( UInt uiTileIdx, UInt uiBitsUsed );

  
  TEncBinIf* getEncBinIf()  { return m_pcBinIf; }
private:
  UInt                 m_uiLastQp;
  
  ContextModel         m_contextModels[MAX_NUM_CTX_MOD];
  Int                  m_numContextModels;
  ContextModel3DBuffer m_cCUSplitFlagSCModel;
  ContextModel3DBuffer m_cCUSkipFlagSCModel;
#if LGE_ILLUCOMP_B0045
  ContextModel3DBuffer m_cCUICFlagSCModel;
#endif
  ContextModel3DBuffer m_cCUMergeFlagExtSCModel;
  ContextModel3DBuffer m_cCUMergeIdxExtSCModel;
#if HHI_INTER_VIEW_RESIDUAL_PRED
  ContextModel3DBuffer m_cResPredFlagSCModel;
#endif
  ContextModel3DBuffer m_cCUPartSizeSCModel;
  ContextModel3DBuffer m_cCUPredModeSCModel;
  ContextModel3DBuffer m_cCUAlfCtrlFlagSCModel;
  ContextModel3DBuffer m_cCUIntraPredSCModel;
  ContextModel3DBuffer m_cCUChromaPredSCModel;
  ContextModel3DBuffer m_cCUDeltaQpSCModel;
  ContextModel3DBuffer m_cCUInterDirSCModel;
  ContextModel3DBuffer m_cCURefPicSCModel;
  ContextModel3DBuffer m_cCUMvdSCModel;
  ContextModel3DBuffer m_cCUQtCbfSCModel;
  ContextModel3DBuffer m_cCUTransSubdivFlagSCModel;
  ContextModel3DBuffer m_cCUQtRootCbfSCModel;
  
  ContextModel3DBuffer m_cCUSigCoeffGroupSCModel;
  ContextModel3DBuffer m_cCUSigSCModel;
  ContextModel3DBuffer m_cCuCtxLastX;
  ContextModel3DBuffer m_cCuCtxLastY;
  ContextModel3DBuffer m_cCUOneSCModel;
  ContextModel3DBuffer m_cCUAbsSCModel;
  
  ContextModel3DBuffer m_cMVPIdxSCModel;
  
  ContextModel3DBuffer m_cALFFlagSCModel;
  ContextModel3DBuffer m_cALFUvlcSCModel;
  ContextModel3DBuffer m_cALFSvlcSCModel;
#if AMP_CTX
  ContextModel3DBuffer m_cCUAMPSCModel;
#else
  ContextModel3DBuffer m_cCUXPosiSCModel;
  ContextModel3DBuffer m_cCUYPosiSCModel;
#endif
  ContextModel3DBuffer m_cSaoFlagSCModel;
  ContextModel3DBuffer m_cSaoUvlcSCModel;
  ContextModel3DBuffer m_cSaoSvlcSCModel;
#if SAO_UNIT_INTERLEAVING
  ContextModel3DBuffer m_cSaoMergeLeftSCModel;
  ContextModel3DBuffer m_cSaoMergeUpSCModel;
  ContextModel3DBuffer m_cSaoTypeIdxSCModel;
#endif

#if HHI_DMM_WEDGE_INTRA || HHI_DMM_PRED_TEX
  ContextModel3DBuffer m_cDmmFlagSCModel;
  ContextModel3DBuffer m_cDmmModeSCModel;
  ContextModel3DBuffer m_cDmmDataSCModel;
#endif
#if LGE_EDGE_INTRA_A0070
  ContextModel3DBuffer m_cEdgeIntraSCModel;
#if LGE_EDGE_INTRA_DELTA_DC
  ContextModel3DBuffer m_cEdgeIntraDeltaDCSCModel;
#endif
#endif
  
#if RWTH_SDC_DLT_B0036
  ContextModel3DBuffer m_cSDCFlagSCModel;
  
  ContextModel3DBuffer m_cSDCResidualFlagSCModel;
  ContextModel3DBuffer m_cSDCResidualSignFlagSCModel;
  ContextModel3DBuffer m_cSDCResidualSCModel;
  
  ContextModel3DBuffer m_cSDCPredModeSCModel;
#endif
};

//! \}

#endif // !defined(AFX_TENCSBAC_H__DDA7CDC4_EDE3_4015_9D32_2156249C82AA__INCLUDED_)
