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



/** \file     TDecSbac.h
    \brief    SBAC decoder class (header)
*/

#ifndef __TDECSBAC__
#define __TDECSBAC__


#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


#include "TDecEntropy.h"
#include "TDecBinCoder.h"
#include "../TLibCommon/ContextTables.h"
#include "../TLibCommon/ContextModel.h"
#include "../TLibCommon/ContextModel3DBuffer.h"

// ====================================================================================================================
// Class definition
// ====================================================================================================================

class SEImessages;

/// SBAC decoder class
class TDecSbac : public TDecEntropyIf
{
public:
  TDecSbac();
  virtual ~TDecSbac();
  
  Void  init                      ( TDecBinIf* p )    { m_pcTDecBinIf = p; }
  Void  uninit                    (              )    { m_pcTDecBinIf = 0; }
  
  Void  resetEntropy              ( TComSlice* pcSlice     );
  Void  setBitstream              ( TComBitstream* p       ) { m_pcBitstream = p; m_pcTDecBinIf->init( p ); }
  
  Void  setAlfCtrl                ( Bool bAlfCtrl          ) { m_bAlfCtrl = bAlfCtrl;                   }
  Void  setMaxAlfCtrlDepth        ( UInt uiMaxAlfCtrlDepth ) { m_uiMaxAlfCtrlDepth = uiMaxAlfCtrlDepth; }
  
  Void  parseNalUnitHeader    ( NalUnitType& eNalUnitType, UInt& TemporalId, Bool& bOutputFlag ) {}
  
  Void  parseSPS                  ( TComSPS* pcSPS         ) {}
  Void  parsePPS                  ( TComPPS* pcPPS         ) {}
  void parseSEI(SEImessages&) {}
  Void  parseSliceHeader          ( TComSlice*& rpcSlice   ) {}
  Void  parseTerminatingBit       ( UInt& ruiBit );
  Void  parseMVPIdx               ( TComDataCU* pcCU, Int& riMVPIdx, Int iMVPNum, UInt uiAbsPartIdx, UInt uiDepth, RefPicList eRefList );
  
  Void  parseAlfFlag              ( UInt& ruiVal           );
  Void  parseAlfUvlc              ( UInt& ruiVal           );
  Void  parseAlfSvlc              ( Int&  riVal            );
  Void  parseAlfCtrlDepth         ( UInt& ruiAlfCtrlDepth  );
#if MTK_SAO
  Void  parseAoFlag              ( UInt& ruiVal           );
  Void  parseAoUvlc              ( UInt& ruiVal           );
  Void  parseAoSvlc              ( Int&  riVal            );
#endif
private:
  Void  xReadUnarySymbol    ( UInt& ruiSymbol, ContextModel* pcSCModel, Int iOffset );
  Void  xReadUnaryMaxSymbol ( UInt& ruiSymbol, ContextModel* pcSCModel, Int iOffset, UInt uiMaxSymbol );
  Void  xReadEpExGolomb     ( UInt& ruiSymbol, UInt uiCount );
#if E253
  Void  xReadGoRiceExGolomb ( UInt &ruiSymbol, UInt &ruiGoRiceParam );
#if HHI_DMM_WEDGE_INTRA || HHI_DMM_PRED_TEX
  Void  xReadExGolombLevel  ( UInt& ruiSymbol, ContextModel& rcSCModel  );
#endif
#else
  Void  xReadExGolombLevel  ( UInt& ruiSymbol, ContextModel& rcSCModel  );
#endif
  
#if MVD_CTX
  Void  xReadMvd            ( Int& riMvdComp, UInt uiAbsSumL, UInt uiAbsSumA, UInt uiCtx );
#else
  Void  xReadMvd            ( Int& riMvdComp, UInt uiAbsSum, UInt uiCtx );
#endif

  Void  xReadExGolombMvd    ( UInt& ruiSymbol, ContextModel* pcSCModel, UInt uiMaxBin );
  
#if HHI_DMM_WEDGE_INTRA
  Void xParseWedgeFullInfo  ( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth );
  Void xParseWedgeFullDeltaInfo     ( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth );

  Void xParseWedgePredDirInfo       ( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth );
  Void xParseWedgePredDirDeltaInfo  ( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth );
#endif
#if HHI_DMM_PRED_TEX
  Void xParseWedgePredTexDeltaInfo  ( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth );
  Void xParseContourPredTexDeltaInfo( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth );
#endif
  
private:
  TComBitstream*    m_pcBitstream;
  TDecBinIf*        m_pcTDecBinIf;
  
  Bool m_bAlfCtrl;
  UInt m_uiMaxAlfCtrlDepth;
  
public:
  Void parseAlfCtrlFlag   ( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth );
#if TSB_ALF_HEADER
  Void parseAlfFlagNum    ( UInt& ruiVal, UInt minValue, UInt depth );
  Void parseAlfCtrlFlag   ( UInt &ruiAlfCtrlFlag );
#endif
  
  Void parseSkipFlag      ( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth );
  Void parseSplitFlag     ( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth );
  Void parseMergeFlag     ( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth, UInt uiPUIdx );
  Void parseMergeIndex    ( TComDataCU* pcCU, UInt& ruiMergeIndex, UInt uiAbsPartIdx, UInt uiDepth );
#if HHI_INTER_VIEW_MOTION_PRED || HHI_MPI || POZNAN_DBMP
  Void parseMergeIndexMV  ( TComDataCU* pcCU, UInt& ruiMergeIndex, UInt uiAbsPartIdx, UInt uiDepth );
#endif
  Void parseResPredFlag   ( TComDataCU* pcCU, Bool& rbResPredFlag, UInt uiAbsPartIdx, UInt uiDepth );
  Void parsePartSize      ( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth );
  Void parsePredMode      ( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth );
  
  Void parseIntraDirLumaAng( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth );
  
  Void parseIntraDirChroma( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth );
  
  Void parseInterDir      ( TComDataCU* pcCU, UInt& ruiInterDir, UInt uiAbsPartIdx, UInt uiDepth );
  Void parseRefFrmIdx     ( TComDataCU* pcCU, Int& riRefFrmIdx, UInt uiAbsPartIdx, UInt uiDepth, RefPicList eRefList );
  Void parseMvd           ( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiPartIdx, UInt uiDepth, RefPicList eRefList );
  
  Void parseTransformSubdivFlag( UInt& ruiSubdivFlag, UInt uiLog2TransformBlockSize );
  Void parseQtCbf         ( TComDataCU* pcCU, UInt uiAbsPartIdx, TextType eType, UInt uiTrDepth, UInt uiDepth );
  Void parseQtRootCbf     ( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth, UInt& uiQtRootCbf );
  
  Void parseDeltaQP       ( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth );
  
  Void parseCbf           ( TComDataCU* pcCU, UInt uiAbsPartIdx, TextType eType, UInt uiTrDepth, UInt uiDepth ) {}
  Void parseBlockCbf      ( TComDataCU* pcCU, UInt uiAbsPartIdx, TextType eType, UInt uiTrDepth, UInt uiDepth, UInt uiQPartNum ) {}
  
#if CAVLC_RQT_CBP
  Void parseCbfTrdiv      ( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiTrDepth, UInt uiDepth, UInt& uiSubdiv ) {}
#endif

#if PCP_SIGMAP_SIMPLE_LAST
  __inline Void parseLastSignificantXY( UInt& uiPosLastX, UInt& uiPosLastY, const UInt uiWidth, const TextType eTType, const UInt uiCTXIdx, const UInt uiScanIdx );
#endif
  Void parseCoeffNxN      ( TComDataCU* pcCU, TCoeff* pcCoef, UInt uiAbsPartIdx, UInt uiWidth, UInt uiHeight, UInt uiDepth, TextType eTType );
  Void parseViewIdx       (Int &riViewIdx );
  
private:
  UInt m_uiLastDQpNonZero;
  UInt m_uiLastQp;
  
  ContextModel3DBuffer m_cCUSkipFlagSCModel;
  ContextModel3DBuffer m_cCUSplitFlagSCModel;
  ContextModel3DBuffer m_cCUMergeFlagExtSCModel;
  ContextModel3DBuffer m_cCUMergeIdxExtSCModel;
  ContextModel3DBuffer m_cCUMVMergeIdxExtSCModel;
  ContextModel3DBuffer m_cResPredFlagSCModel;
  ContextModel3DBuffer m_cCUAlfCtrlFlagSCModel;
  ContextModel3DBuffer m_cCUPartSizeSCModel;
  ContextModel3DBuffer m_cCUPredModeSCModel;
  
  ContextModel3DBuffer m_cCUIntraPredSCModel;
#if ADD_PLANAR_MODE
  ContextModel3DBuffer m_cPlanarFlagSCModel;
#endif
  ContextModel3DBuffer m_cCUChromaPredSCModel;
  ContextModel3DBuffer m_cCUInterDirSCModel;
  ContextModel3DBuffer m_cCURefPicSCModel;
  ContextModel3DBuffer m_cCUMvdSCModel;
  
  ContextModel3DBuffer m_cCUTransSubdivFlagSCModel;
  ContextModel3DBuffer m_cCUQtRootCbfSCModel;
  ContextModel3DBuffer m_cCUDeltaQpSCModel;
  
  ContextModel3DBuffer m_cCUQtCbfSCModel;
  
  ContextModel3DBuffer m_cCUSigSCModel;
#if PCP_SIGMAP_SIMPLE_LAST
  ContextModel3DBuffer m_cCuCtxLastX;
  ContextModel3DBuffer m_cCuCtxLastY;
#else  
  ContextModel3DBuffer m_cCULastSCModel;
#endif
  ContextModel3DBuffer m_cCUOneSCModel;
  ContextModel3DBuffer m_cCUAbsSCModel;
  
  ContextModel3DBuffer m_cMVPIdxSCModel;
  
  ContextModel3DBuffer m_cALFFlagSCModel;
  ContextModel3DBuffer m_cALFUvlcSCModel;
  ContextModel3DBuffer m_cALFSvlcSCModel;
#if MTK_SAO
  ContextModel3DBuffer m_cAOFlagSCModel;
  ContextModel3DBuffer m_cAOUvlcSCModel;
  ContextModel3DBuffer m_cAOSvlcSCModel;
#endif
  ContextModel3DBuffer m_cViewIdxSCModel;
#if HHI_DMM_WEDGE_INTRA || HHI_DMM_PRED_TEX
  ContextModel3DBuffer m_cIntraDMMSCModel;
  ContextModel3DBuffer m_cIntraWedgeSCModel;
#endif
};

#endif // !defined(AFX_TDECSBAC_H__CFCAAA19_8110_47F4_9A16_810C4B5499D5__INCLUDED_)
