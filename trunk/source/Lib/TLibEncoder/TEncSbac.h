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
  Void  resetEntropy           (const TComSlice *pSlice);
  SliceType determineCabacInitIdx  (const TComSlice *pSlice);
  Void  setBitstream           ( TComBitIf* p )  { m_pcBitIf = p; m_pcBinIf->init( p ); }

  Void  load                   ( const TEncSbac* pSrc  );
  Void  loadIntraDirMode       ( const TEncSbac* pScr, const ChannelType chType  );
#if NH_3D_DMM
  Void  loadIntraDepthDmm      ( const TEncSbac* pSrc );
#endif
  Void  store                  ( TEncSbac* pDest ) const;
  Void  loadContexts           ( const TEncSbac* pSrc  );
  Void  resetBits              ()                { m_pcBinIf->resetBits(); m_pcBitIf->resetBits(); }
  UInt  getNumberOfWrittenBits ()                { return m_pcBinIf->getNumWrittenBits(); }
  //--SBAC RD

  Void  codeVPS                ( const TComVPS* pcVPS );
  Void  codeSPS                ( const TComSPS* pcSPS     );
  Void  codePPS                ( const TComPPS* pcPPS     );
  Void  codeSliceHeader        ( TComSlice* pcSlice );
  Void  codeTilesWPPEntryPoint ( TComSlice* pSlice );
  Void  codeTerminatingBit     ( UInt uilsLast      );
  Void  codeSliceFinish        ();
  Void  codeSaoMaxUvlc       ( UInt code, UInt maxSymbol );
  Void  codeSaoMerge         ( UInt  uiCode );
  Void  codeSaoTypeIdx       ( UInt  uiCode);
  Void  codeSaoUflc          ( UInt uiLength, UInt  uiCode );
  Void  codeSAOSign          ( UInt  uiCode);  //<! code SAO offset sign

  Void codeSAOOffsetParam(ComponentID compIdx, SAOOffset& ctbParam, Bool sliceEnabled, const Int channelBitDepth);
  Void codeSAOBlkParam(SAOBlkParam& saoBlkParam, const BitDepths &bitDepths
                    , Bool* sliceEnabled
                    , Bool leftMergeAvail
                    , Bool aboveMergeAvail
                    , Bool onlyEstMergeInfo = false
                    );

private:
  Void  xWriteUnarySymbol    ( UInt uiSymbol, ContextModel* pcSCModel, Int iOffset );
  Void  xWriteUnaryMaxSymbol ( UInt uiSymbol, ContextModel* pcSCModel, Int iOffset, UInt uiMaxSymbol );
  Void  xWriteEpExGolomb     ( UInt uiSymbol, UInt uiCount );
  Void  xWriteCoefRemainExGolomb ( UInt symbol, UInt &rParam, const Bool useLimitedPrefixLength, const Int maxLog2TrDynamicRange );
#if NH_3D_DMM || NH_3D_SDC_INTRA || NH_3D_SDC_INTER
  Void  xWriteExGolombLevelDdc( UInt uiSymbol );
  Void  xCodeDeltaDC         ( Pel valDeltaDC, UInt uiNumSeg );
#endif
#if NH_3D_DMM
  Void  xCodeIntraDepthMode  ( TComDataCU* pcCU, UInt absPartIdx );
  Void  xCodeDmmData         ( TComDataCU* pcCU, UInt absPartIdx );
  Void  xCodeDmm1WedgeIdx    ( UInt uiTabIdx, Int iNumBit );
#endif


  Void  xCopyFrom            ( const TEncSbac* pSrc );
  Void  xCopyContextsFrom    ( const TEncSbac* pSrc );

protected:
  TComBitIf*    m_pcBitIf;
  TEncBinIf*    m_pcBinIf;

  //--Adaptive loop filter

public:
  Void codeCUTransquantBypassFlag( TComDataCU* pcCU, UInt uiAbsPartIdx );
  Void codeSkipFlag      ( TComDataCU* pcCU, UInt uiAbsPartIdx );
#if NH_3D_DIS
  Void codeDIS           ( TComDataCU* pcCU, UInt uiAbsPartIdx );
#endif
  Void codeMergeFlag     ( TComDataCU* pcCU, UInt uiAbsPartIdx );
  Void codeMergeIndex    ( TComDataCU* pcCU, UInt uiAbsPartIdx );
#if NH_3D_ARP
  Void codeARPW          ( TComDataCU* pcCU, UInt uiAbsPartIdx );
#endif
#if NH_3D_IC
  Void codeICFlag        ( TComDataCU* pcCU, UInt uiAbsPartIdx );
#endif
#if NH_3D_DMM || NH_3D_SDC_INTRA || NH_3D_SDC_INTER
  Void  codeDeltaDC      ( TComDataCU* pcCU, UInt absPartIdx );
#endif
#if NH_3D_SDC_INTRA || NH_3D_SDC_INTER
  Void codeSDCFlag       ( TComDataCU* pcCU, UInt uiAbsPartIdx );
#endif
#if NH_3D_DBBP
  Void codeDBBPFlag      ( TComDataCU* pcCU, UInt uiAbsPartIdx );
#endif
  Void codeSplitFlag     ( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth );
  Void codeMVPIdx        ( TComDataCU* pcCU, UInt uiAbsPartIdx, RefPicList eRefList );

  Void codePartSize      ( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth );
  Void codePredMode      ( TComDataCU* pcCU, UInt uiAbsPartIdx );
  Void codeIPCMInfo      ( TComDataCU* pcCU, UInt uiAbsPartIdx );
  Void codeTransformSubdivFlag ( UInt uiSymbol, UInt uiCtx );
  Void codeQtCbf               ( TComTU & rTu, const ComponentID compID, const Bool lowestLevel );
  Void codeQtRootCbf           ( TComDataCU* pcCU, UInt uiAbsPartIdx );
  Void codeQtCbfZero           ( TComTU &rTu, const ChannelType chType );
  Void codeQtRootCbfZero       ( );
  Void codeIntraDirLumaAng     ( TComDataCU* pcCU, UInt absPartIdx, Bool isMultiple);
  Void codeIntraDirChroma      ( TComDataCU* pcCU, UInt uiAbsPartIdx );
  Void codeInterDir            ( TComDataCU* pcCU, UInt uiAbsPartIdx );
  Void codeRefFrmIdx           ( TComDataCU* pcCU, UInt uiAbsPartIdx, RefPicList eRefList );
  Void codeMvd                 ( TComDataCU* pcCU, UInt uiAbsPartIdx, RefPicList eRefList );

  Void codeCrossComponentPrediction( TComTU &rTu, ComponentID compID );

  Void codeDeltaQP             ( TComDataCU* pcCU, UInt uiAbsPartIdx );
  Void codeChromaQpAdjustment  ( TComDataCU* cu, UInt absPartIdx );

  Void codeLastSignificantXY ( UInt uiPosX, UInt uiPosY, Int width, Int height, ComponentID component, UInt uiScanIdx );
  Void codeCoeffNxN            ( TComTU &rTu, TCoeff* pcCoef, const ComponentID compID );
  Void codeTransformSkipFlags ( TComTU &rTu, ComponentID component );

  // -------------------------------------------------------------------------------------------------------------------
  // for RD-optimizatioon
  // -------------------------------------------------------------------------------------------------------------------

  Void estBit               (estBitsSbacStruct* pcEstBitsSbac, Int width, Int height, ChannelType chType);
  Void estCBFBit                     ( estBitsSbacStruct* pcEstBitsSbac );
  Void estSignificantCoeffGroupMapBit( estBitsSbacStruct* pcEstBitsSbac, ChannelType chType );
  Void estSignificantMapBit          ( estBitsSbacStruct* pcEstBitsSbac, Int width, Int height, ChannelType chType );
  Void estLastSignificantPositionBit ( estBitsSbacStruct* pcEstBitsSbac, Int width, Int height, ChannelType chType );
  Void estSignificantCoefficientsBit ( estBitsSbacStruct* pcEstBitsSbac, ChannelType chType );

  Void codeExplicitRdpcmMode            ( TComTU &rTu, const ComponentID compID );


  TEncBinIf* getEncBinIf()  { return m_pcBinIf; }
private:
  ContextModel         m_contextModels[MAX_NUM_CTX_MOD];
  Int                  m_numContextModels;
  ContextModel3DBuffer m_cCUSplitFlagSCModel;
  ContextModel3DBuffer m_cCUSkipFlagSCModel;
#if NH_3D_DIS
  ContextModel3DBuffer m_cCUDISFlagSCModel;
  ContextModel3DBuffer m_cCUDISTypeSCModel;
#endif
  ContextModel3DBuffer m_cCUMergeFlagExtSCModel;
  ContextModel3DBuffer m_cCUMergeIdxExtSCModel;
#if NH_3D_ARP
  ContextModel3DBuffer m_cCUPUARPWSCModel;
#endif
#if NH_3D_IC
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
  ContextModel3DBuffer m_explicitRdpcmFlagSCModel;
  ContextModel3DBuffer m_explicitRdpcmDirSCModel;
  ContextModel3DBuffer m_cCrossComponentPredictionSCModel;

  ContextModel3DBuffer m_ChromaQpAdjFlagSCModel;
  ContextModel3DBuffer m_ChromaQpAdjIdcSCModel;

#if NH_3D_DMM
  ContextModel3DBuffer m_cNotDmmFlagSCModel;
  ContextModel3DBuffer m_cDmmModeSCModel;
#endif
#if NH_3D_DMM || NH_3D_SDC_INTRA || NH_3D_SDC_INTER
  ContextModel3DBuffer m_cDdcDataSCModel;
  ContextModel3DBuffer m_cSDCFlagSCModel;
#endif
#if NH_3D_SDC_INTRA  
  ContextModel3DBuffer m_cSDCResidualFlagSCModel;
  ContextModel3DBuffer m_cSDCResidualSCModel;
  ContextModel3DBuffer m_cDdcFlagSCModel;
#endif
#if NH_3D_DBBP
  ContextModel3DBuffer m_cDBBPFlagSCModel;
#endif

  UInt m_golombRiceAdaptationStatistics[RExt__GOLOMB_RICE_ADAPTATION_STATISTICS_SETS];
};

//! \}

#endif // !defined(AFX_TENCSBAC_H__DDA7CDC4_EDE3_4015_9D32_2156249C82AA__INCLUDED_)
