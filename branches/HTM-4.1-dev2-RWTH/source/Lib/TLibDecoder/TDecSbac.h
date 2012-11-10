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

class SEImessages;

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
#if OL_FLUSH
  Void decodeFlush();
#endif

#if CABAC_INIT_FLAG
  Void  resetEntropy (TComSlice* pSlice );
#else
  Void  resetEntropywithQPandInitIDC ( Int  iQp, Int iID);
  Void  resetEntropy                 ( Int  iQp, Int iID      ) { resetEntropywithQPandInitIDC(iQp, iID);                                      }
  Void  resetEntropy                 ( TComSlice* pcSlice     ) { resetEntropywithQPandInitIDC(pcSlice->getSliceQp(), pcSlice->getCABACinitIDC());}
#endif
  Void  setBitstream              ( TComInputBitstream* p  ) { m_pcBitstream = p; m_pcTDecBinIf->init( p ); }
  
#if VIDYO_VPS_INTEGRATION
  Void  parseVPS                  ( TComVPS* pcVPS )  {}
#endif
#if HHI_MPI
  Void  parseSPS                  ( TComSPS* pcSPS, Bool bIsDepth ) {}
#else
  Void  parseSPS                  ( TComSPS* pcSPS         ) {}
#endif
#if TILES_OR_ENTROPY_SYNC_IDC  
  Void  parsePPS                  ( TComPPS* pcPPS, ParameterSetManagerDecoder *parameterSet         ) {}
#else
  Void  parsePPS                  ( TComPPS* pcPPS         ) {}
#endif
  Void  parseAPS                  ( TComAPS* pAPS          ) {}
  void parseSEI(SEImessages&) {}

#if LCU_SYNTAX_ALF
  Void  parseSliceHeader          ( TComSlice*& rpcSlice, ParameterSetManagerDecoder *parameterSetManager, AlfCUCtrlInfo &alfCUCtrl, AlfParamSet& alfParamSet) {}
#else
  Void  parseSliceHeader          ( TComSlice*& rpcSlice, ParameterSetManagerDecoder *parameterSetManager, AlfCUCtrlInfo &alfCUCtrl ) {}
#endif

  Void  parseTerminatingBit       ( UInt& ruiBit );
#if HHI_INTER_VIEW_MOTION_PRED
  Void  parseMVPIdx               ( Int& riMVPIdx, Int iNumAMVPCands );
#else
  Void  parseMVPIdx               ( Int& riMVPIdx          );
#endif
  
#if SAO_UNIT_INTERLEAVING
  Void  parseSaoUvlc              ( UInt& ruiVal           );
  Void  parseSaoSvlc              ( Int&  riVal            );
  Void  parseSaoMergeLeft         ( UInt&  ruiVal, UInt uiCompIdx   );
  Void  parseSaoMergeUp           ( UInt&  ruiVal  );
  Void  parseSaoTypeIdx           ( UInt&  ruiVal  );
  Void  parseSaoUflc              ( UInt& ruiVal           );
  Void  parseSaoOneLcuInterleaving(Int rx, Int ry, SAOParam* pSaoParam, TComDataCU* pcCU, Int iCUAddrInSlice, Int iCUAddrUpInSlice, Bool bLFCrossSliceBoundaryFlag);
  Void  parseSaoOffset            (SaoLcuParam* psSaoLcuParam);
#endif
  
#if RWTH_SDC_DLT_B0036
  Void parseSDCFlag    ( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth );
  Void parseSDCPredMode    ( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth );
  Void parseSDCResidualData     ( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth, UInt uiPart );
#endif
private:
  Void  xReadUnarySymbol    ( UInt& ruiSymbol, ContextModel* pcSCModel, Int iOffset );
  Void  xReadUnaryMaxSymbol ( UInt& ruiSymbol, ContextModel* pcSCModel, Int iOffset, UInt uiMaxSymbol );
  Void  xReadEpExGolomb     ( UInt& ruiSymbol, UInt uiCount );
  Void  xReadGoRiceExGolomb ( UInt &ruiSymbol, UInt &ruiGoRiceParam );
#if HHI_DMM_WEDGE_INTRA || HHI_DMM_PRED_TEX
  Void  xReadExGolombLevel  ( UInt& ruiSymbol, ContextModel& rcSCModel  );
#endif
#if HHI_DMM_WEDGE_INTRA
  Void xParseWedgeFullInfo          ( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth );
  Void xParseWedgeFullDeltaInfo     ( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth );

  Void xParseWedgePredDirInfo       ( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth );
  Void xParseWedgePredDirDeltaInfo  ( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth );
#endif
#if HHI_DMM_PRED_TEX
  Void xParseWedgePredTexDeltaInfo  ( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth );
  Void xParseContourPredTexDeltaInfo( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth );
#endif
  
#if LGE_EDGE_INTRA
  Void xParseEdgeIntraInfo ( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth );
#endif
  
private:
  TComInputBitstream* m_pcBitstream;
  TDecBinIf*        m_pcTDecBinIf;
  
  Int           m_iSliceGranularity; //!< slice granularity

public:
  Void parseAlfCtrlFlag   ( UInt &ruiAlfCtrlFlag );

  /// set slice granularity
  Void setSliceGranularity(Int iSliceGranularity)  {m_iSliceGranularity = iSliceGranularity;}

  /// get slice granularity
  Int  getSliceGranularity()                       {return m_iSliceGranularity;             }

  Void parseSkipFlag      ( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth );
#if LGE_ILLUCOMP_B0045
  Void parseICFlag        ( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth );
#endif
  Void parseSplitFlag     ( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth );
  Void parseMergeFlag     ( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth, UInt uiPUIdx );
  Void parseMergeIndex    ( TComDataCU* pcCU, UInt& ruiMergeIndex, UInt uiAbsPartIdx, UInt uiDepth );
#if HHI_INTER_VIEW_RESIDUAL_PRED
  Void parseResPredFlag   ( TComDataCU* pcCU, Bool& rbResPredFlag, UInt uiAbsPartIdx, UInt uiDepth );
#endif
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
  
  Void parseIPCMInfo      ( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth);

  Void parseLastSignificantXY( UInt& uiPosLastX, UInt& uiPosLastY, Int width, Int height, TextType eTType, UInt uiScanIdx );
  Void parseCoeffNxN      ( TComDataCU* pcCU, TCoeff* pcCoef, UInt uiAbsPartIdx, UInt uiWidth, UInt uiHeight, UInt uiDepth, TextType eTType );
  
  Void readTileMarker   ( UInt& uiTileIdx, UInt uiBitsUsed );
  Void updateContextTables( SliceType eSliceType, Int iQp );

  Void  parseScalingList ( TComScalingList* scalingList ) {}

private:
  UInt m_uiLastDQpNonZero;
  UInt m_uiLastQp;
  
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
#if LGE_EDGE_INTRA
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

#endif // !defined(AFX_TDECSBAC_H__CFCAAA19_8110_47F4_9A16_810C4B5499D5__INCLUDED_)
