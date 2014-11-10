/* The copyright in this software is being made available under the BSD
 * License, included below. This software may be subject to other third party
 * and contributor rights, including patent rights, and no such rights are
 * granted under this license.  
 *
* Copyright (c) 2010-2014, ITU/ISO/IEC
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
#include "TLibCommon/ContextTables.h"
#include "TLibCommon/ContextModel.h"
#include "TLibCommon/ContextModel3DBuffer.h"

//! \ingroup TLibDecoder
//! \{

// ====================================================================================================================
// Class definition
// ====================================================================================================================

/// SBAC decoder class
class TDecSbac : public TDecEntropyIf
{
public:
  TDecSbac();
  virtual ~TDecSbac();
  
  Void  init                      ( TDecBinIf* p )    { m_pcTDecBinIf = p; }
  Void  uninit                    (              )    { m_pcTDecBinIf = 0; }
  
  Void load                          ( TDecSbac* pScr );
  Void loadContexts                  ( TDecSbac* pScr );
  Void xCopyFrom           ( TDecSbac* pSrc );
  Void xCopyContextsFrom       ( TDecSbac* pSrc );

  Void  resetEntropy (TComSlice* pSlice );
  Void  setBitstream              ( TComInputBitstream* p  ) { m_pcBitstream = p; m_pcTDecBinIf->init( p ); }
  Void  parseVPS                  ( TComVPS* /*pcVPS*/ ) {}
#if HHI_TOOL_PARAMETERS_I2_J0107
  Void  parseSPS                  ( TComSPS* /*pcSPS*/ ) {}
#else
#if H_3D
  Void  parseSPS                  ( TComSPS* /*pcSPS*/ , Int /*viewIndex*/, Bool /*depthFlag*/ ) {}
#else
  Void  parseSPS                  ( TComSPS* /*pcSPS*/ ) {}
#endif
#endif
#if H_3D
  Void  parsePPS                  ( TComPPS* /*pcPPS*/, TComVPS* /*pcVPS*/ ) {}
#else
  Void  parsePPS                  ( TComPPS* /*pcPPS*/ ) {}
#endif

#if H_MV
  Void  parseSliceHeader          ( TComSlice*& /*rpcSlice*/, ParameterSetManagerDecoder* /*parameterSetManager*/, Int targetOlsIdx ) {}
#else
  Void  parseSliceHeader          ( TComSlice*& /*rpcSlice*/, ParameterSetManagerDecoder* /*parameterSetManager*/ ) {}
#endif
  Void  parseTerminatingBit       ( UInt& ruiBit );
  Void  parseMVPIdx               ( Int& riMVPIdx          );
  Void  parseSaoMaxUvlc           ( UInt& val, UInt maxSymbol );
  Void  parseSaoMerge         ( UInt&  ruiVal   );
  Void  parseSaoTypeIdx           ( UInt&  ruiVal  );
  Void  parseSaoUflc              ( UInt uiLength, UInt& ruiVal     );
  Void parseSAOBlkParam (SAOBlkParam& saoBlkParam, Bool* sliceEnabled, Bool leftMergeAvail, Bool aboveMergeAvail);
  Void parseSaoSign(UInt& val);
private:
  Void  xReadUnarySymbol    ( UInt& ruiSymbol, ContextModel* pcSCModel, Int iOffset );
  Void  xReadUnaryMaxSymbol ( UInt& ruiSymbol, ContextModel* pcSCModel, Int iOffset, UInt uiMaxSymbol );
  Void  xReadEpExGolomb     ( UInt& ruiSymbol, UInt uiCount );
  Void  xReadCoefRemainExGolomb ( UInt &rSymbol, UInt &rParam );
#if H_3D_DIM
  Void  xReadExGolombLevel   ( UInt& ruiSymbol, ContextModel& rcSCModel  );
  Void  xParseDimDeltaDC     ( Pel& rValDeltaDC, UInt uiNumSeg );
#if H_3D_DIM_DMM
  Void  xParseDmm1WedgeIdx   ( UInt& ruiTabIdx, Int iNumBit );
#endif
#if H_3D_DIM_SDC
  Void  xParseSDCResidualData     ( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth, UInt uiPart );
#endif
#endif
#if H_3D_INTER_SDC
  Void  parseDeltaDC         ( TComDataCU* pcCU, UInt absPartIdx, UInt depth );
  Void  parseSDCFlag         ( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth );
#endif
#if H_3D_DBBP
  Void parseDBBPFlag        ( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth );
#endif
private:
  TComInputBitstream* m_pcBitstream;
  TDecBinIf*        m_pcTDecBinIf;
 
public:
  
  Void parseSkipFlag      ( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth );
#if H_3D_SINGLE_DEPTH  
  Void parseSingleDepthMode      ( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth );
#endif  
  Void parseCUTransquantBypassFlag( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth );
  Void parseSplitFlag     ( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth );
  Void parseMergeFlag     ( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth, UInt uiPUIdx );
  Void parseMergeIndex    ( TComDataCU* pcCU, UInt& ruiMergeIndex );
#if H_3D_ARP
  Void parseARPW          ( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth );
#endif
#if H_3D_IC
  Void parseICFlag        ( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth );
#endif
  Void parsePartSize      ( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth );
  Void parsePredMode      ( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth );
  
  Void parseIntraDirLumaAng( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth );
  
  Void parseIntraDirChroma( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth );
  
#if H_3D_DIM
  Void parseIntraDepth     ( TComDataCU* pcCU, UInt absPartIdx, UInt depth );
  Void parseIntraDepthMode ( TComDataCU* pcCU, UInt absPartIdx, UInt depth );
#endif

  Void parseInterDir      ( TComDataCU* pcCU, UInt& ruiInterDir, UInt uiAbsPartIdx );
  Void parseRefFrmIdx     ( TComDataCU* pcCU, Int& riRefFrmIdx, RefPicList eRefList );
  Void parseMvd           ( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiPartIdx, UInt uiDepth, RefPicList eRefList );
  
  Void parseTransformSubdivFlag( UInt& ruiSubdivFlag, UInt uiLog2TransformBlockSize );
  Void parseQtCbf         ( TComDataCU* pcCU, UInt uiAbsPartIdx, TextType eType, UInt uiTrDepth, UInt uiDepth );
  Void parseQtRootCbf     ( UInt uiAbsPartIdx, UInt& uiQtRootCbf );
  
  Void parseDeltaQP       ( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth );
  
  Void parseIPCMInfo      ( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth);

  Void parseLastSignificantXY( UInt& uiPosLastX, UInt& uiPosLastY, Int width, Int height, TextType eTType, UInt uiScanIdx );
  Void parseCoeffNxN      ( TComDataCU* pcCU, TCoeff* pcCoef, UInt uiAbsPartIdx, UInt uiWidth, UInt uiHeight, UInt uiDepth, TextType eTType );
  Void parseTransformSkipFlags ( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt width, UInt height, UInt uiDepth, TextType eTType);

  Void updateContextTables( SliceType eSliceType, Int iQp );

  Void  parseScalingList ( TComScalingList* /*scalingList*/ ) {}

private:
  UInt m_uiLastDQpNonZero;
  UInt m_uiLastQp;
  
  ContextModel         m_contextModels[MAX_NUM_CTX_MOD];
  Int                  m_numContextModels;
  ContextModel3DBuffer m_cCUSplitFlagSCModel;
  ContextModel3DBuffer m_cCUSkipFlagSCModel;
#if H_3D_SINGLE_DEPTH
  ContextModel3DBuffer m_cCUSingleDepthFlagSCModel;
  ContextModel3DBuffer m_cSingleDepthValueSCModel;
#endif
  ContextModel3DBuffer m_cCUMergeFlagExtSCModel;
  ContextModel3DBuffer m_cCUMergeIdxExtSCModel;
#if H_3D_ARP
  ContextModel3DBuffer m_cCUPUARPWSCModel;
#endif
#if H_3D_IC
  ContextModel3DBuffer m_cCUICFlagSCModel;
#endif
  ContextModel3DBuffer m_cCUPartSizeSCModel;
  ContextModel3DBuffer m_cCUPredModeSCModel;
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
  
  ContextModel3DBuffer m_cSaoMergeSCModel;
  ContextModel3DBuffer m_cSaoTypeIdxSCModel;
  ContextModel3DBuffer m_cTransformSkipSCModel;
  ContextModel3DBuffer m_CUTransquantBypassFlagSCModel;

#if H_3D_DIM
  ContextModel3DBuffer m_cDepthIntraModeSCModel;
  ContextModel3DBuffer m_cDdcFlagSCModel;
  ContextModel3DBuffer m_cDdcDataSCModel;
  ContextModel3DBuffer m_cAngleFlagSCModel;
#if H_3D_DIM_SDC  
  ContextModel3DBuffer m_cSDCResidualFlagSCModel;
  ContextModel3DBuffer m_cSDCResidualSCModel;
#endif
#endif
#if H_3D_DIM_SDC  
  ContextModel3DBuffer m_cSDCFlagSCModel;
#endif
#if H_3D_DBBP
  ContextModel3DBuffer m_cDBBPFlagSCModel;
#endif
};

//! \}

#endif // !defined(AFX_TDECSBAC_H__CFCAAA19_8110_47F4_9A16_810C4B5499D5__INCLUDED_)
