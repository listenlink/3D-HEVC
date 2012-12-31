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

/** \file     TEncSearch.h
    \brief    encoder search class (header)
*/

#ifndef __TENCSEARCH__
#define __TENCSEARCH__

// Include files
#include "TLibCommon/TComYuv.h"
#include "TLibCommon/TComMotionInfo.h"
#include "TLibCommon/TComPattern.h"
#include "TLibCommon/TComPrediction.h"
#include "TLibCommon/TComTrQuant.h"
#include "TLibCommon/TComPic.h"
#include "TEncEntropy.h"
#include "TEncSbac.h"
#include "TEncCfg.h"

//! \ingroup TLibEncoder
//! \{

class TEncCu;

// ====================================================================================================================
// Class definition
// ====================================================================================================================

/// encoder search class
class TEncSearch : public TComPrediction
{
private:
  TCoeff**        m_ppcQTTempCoeffY;
  TCoeff**        m_ppcQTTempCoeffCb;
  TCoeff**        m_ppcQTTempCoeffCr;
  TCoeff*         m_pcQTTempCoeffY;
  TCoeff*         m_pcQTTempCoeffCb;
  TCoeff*         m_pcQTTempCoeffCr;
#if ADAPTIVE_QP_SELECTION
  Int**           m_ppcQTTempArlCoeffY;
  Int**           m_ppcQTTempArlCoeffCb;
  Int**           m_ppcQTTempArlCoeffCr;
  Int*            m_pcQTTempArlCoeffY;
  Int*            m_pcQTTempArlCoeffCb;
  Int*            m_pcQTTempArlCoeffCr;
#endif
  UChar*          m_puhQTTempTrIdx;
  UChar*          m_puhQTTempCbf[3];
  
  TComYuv*        m_pcQTTempTComYuv;
  TComYuv         m_tmpYuvPred; // To be used in xGetInterPredictionError() to avoid constant memory allocation/deallocation
protected:
  // interface to option
  TEncCfg*        m_pcEncCfg;
  
  // interface to classes
  TComTrQuant*    m_pcTrQuant;
  TComRdCost*     m_pcRdCost;
  TEncEntropy*    m_pcEntropyCoder;
  
  // ME parameters
  Int             m_iSearchRange;
  Int             m_bipredSearchRange; // Search range for bi-prediction
#if DV_V_RESTRICTION_B0037
  Bool            m_bUseDisparitySearchRangeRestriction;
  Int             m_iVerticalDisparitySearchRange;
#endif
  Int             m_iFastSearch;
  Int             m_aaiAdaptSR[2][33];
  TComMv          m_cSrchRngLT;
  TComMv          m_cSrchRngRB;
  TComMv          m_acMvPredictors[3];
  
  // RD computation
  TEncSbac***     m_pppcRDSbacCoder;
  TEncSbac*       m_pcRDGoOnSbacCoder;
  Bool            m_bUseSBACRD;
  DistParam       m_cDistParam;
  
  // Misc.
  Pel*            m_pTempPel;
  UInt*           m_puiDFilter;
  Int             m_iMaxDeltaQP;
  

#if HHI_VSO
  TComYuv         m_cYuvRecTemp; 
#endif
  
  // AMVP cost computation
#if HHI_INTER_VIEW_MOTION_PRED
  UInt            m_auiMVPIdxCost[AMVP_MAX_NUM_CANDS+2][AMVP_MAX_NUM_CANDS+2]; //th array bounds
#else
  // UInt            m_auiMVPIdxCost[AMVP_MAX_NUM_CANDS+1][AMVP_MAX_NUM_CANDS];
  UInt            m_auiMVPIdxCost[AMVP_MAX_NUM_CANDS+1][AMVP_MAX_NUM_CANDS+1]; //th array bounds
#endif
  
public:
  TEncSearch();
  virtual ~TEncSearch();
  
  Void init(  TEncCfg*      pcEncCfg,
            TComTrQuant*  pcTrQuant,
            Int           iSearchRange,
            Int           bipredSearchRange,
#if DV_V_RESTRICTION_B0037
            Bool          bUseDisparitySearchRangeRestriction,
            Int           iVerticalDisparitySearchRange,
#endif
            Int           iFastSearch,
            Int           iMaxDeltaQP,
            TEncEntropy*  pcEntropyCoder,
            TComRdCost*   pcRdCost,
            TEncSbac***   pppcRDSbacCoder,
            TEncSbac*     pcRDGoOnSbacCoder );
  
protected:
  
  /// sub-function for motion vector refinement used in fractional-pel accuracy
  UInt  xPatternRefinement( TComPattern* pcPatternKey,
                           TComMv baseRefMv,
                           Int iFrac, TComMv& rcMvFrac
#if VSP_MV_ZERO
                          ,Bool bIsVsp
#endif
                          );
  
  typedef struct
  {
    Pel*  piRefY;
    Int   iYStride;
    Int   iBestX;
    Int   iBestY;
    UInt  uiBestRound;
    UInt  uiBestDistance;
    UInt  uiBestSad;
    UChar ucPointNr;
  } IntTZSearchStruct;
  
  // sub-functions for ME
  __inline Void xTZSearchHelp         ( TComPattern* pcPatternKey, IntTZSearchStruct& rcStruct, const Int iSearchX, const Int iSearchY, const UChar ucPointNr, const UInt uiDistance );
  __inline Void xTZ2PointSearch       ( TComPattern* pcPatternKey, IntTZSearchStruct& rcStrukt, TComMv* pcMvSrchRngLT, TComMv* pcMvSrchRngRB );
  __inline Void xTZ8PointSquareSearch ( TComPattern* pcPatternKey, IntTZSearchStruct& rcStrukt, TComMv* pcMvSrchRngLT, TComMv* pcMvSrchRngRB, const Int iStartX, const Int iStartY, const Int iDist );
  __inline Void xTZ8PointDiamondSearch( TComPattern* pcPatternKey, IntTZSearchStruct& rcStrukt, TComMv* pcMvSrchRngLT, TComMv* pcMvSrchRngRB, const Int iStartX, const Int iStartY, const Int iDist );
  
  Void xGetInterPredictionError( TComDataCU* pcCU, TComYuv* pcYuvOrg, Int iPartIdx, UInt& ruiSAD, Bool Hadamard );

public:
  Void  preestChromaPredMode    ( TComDataCU* pcCU, 
                                  TComYuv*    pcOrgYuv, 
                                  TComYuv*    pcPredYuv );
  Void  estIntraPredQT          ( TComDataCU* pcCU, 
                                  TComYuv*    pcOrgYuv, 
                                  TComYuv*    pcPredYuv, 
                                  TComYuv*    pcResiYuv, 
                                  TComYuv*    pcRecoYuv,
                                  Dist&       ruiDistC,
                                  Bool        bLumaOnly );
  Void  estIntraPredChromaQT    ( TComDataCU* pcCU, 
                                  TComYuv*    pcOrgYuv, 
                                  TComYuv*    pcPredYuv, 
                                  TComYuv*    pcResiYuv, 
                                  TComYuv*    pcRecoYuv,
                                  Dist        uiPreCalcDistC );
  
  
  /// encoder estimation - inter prediction (non-skip)
  Void predInterSearch          ( TComDataCU* pcCU,
                                  TComYuv*    pcOrgYuv,
#if LG_RESTRICTEDRESPRED_M24766
                                  TComYuv*    rpcResiPredYuv,
#endif
                                  TComYuv*&   rpcPredYuv,
                                  TComYuv*&   rpcResiYuv,
                                  TComYuv*&   rpcRecoYuv,
                                  Bool        bUseRes = false
#if AMP_MRG
                                 ,Bool        bUseMRG = false
#endif
#if FORCE_REF_VSP==1
                                 ,Bool        bForceRefVsp = false
#endif
                                );
  
#if HHI_INTER_VIEW_RESIDUAL_PRED
  /// encode residual and compute rd-cost for inter mode
  Void encodeResAndCalcRdInterCU( TComDataCU* pcCU,
                                  TComYuv*    pcYuvOrg,
                                  TComYuv*    pcYuvPred,
                                  TComYuv*&   rpcYuvResi,
                                  TComYuv*&   rpcYuvResiBest,
                                  TComYuv*&   rpcYuvRec,
                                  TComYuv*&   rpcYuvResPrd,
                                  Bool        bSkipRes );
#else
  /// encode residual and compute rd-cost for inter mode
  Void encodeResAndCalcRdInterCU( TComDataCU* pcCU,
                                  TComYuv*    pcYuvOrg,
                                  TComYuv*    pcYuvPred,
                                  TComYuv*&   rpcYuvResi,
                                  TComYuv*&   rpcYuvResiBest,
                                  TComYuv*&   rpcYuvRec,
                                  Bool        bSkipRes );
#endif
  /// set ME search range
  Void setAdaptiveSearchRange   ( Int iDir, Int iRefIdx, Int iSearchRange) { m_aaiAdaptSR[iDir][iRefIdx] = iSearchRange; }
  
  Void xEncPCM    (TComDataCU* pcCU, UInt uiAbsPartIdx, Pel* piOrg, Pel* piPCM, Pel* piPred, Pel* piResi, Pel* piReco, UInt uiStride, UInt uiWidth, UInt uiHeight, TextType eText);
  Void IPCMSearch (TComDataCU* pcCU, TComYuv* pcOrgYuv, TComYuv*& rpcPredYuv, TComYuv*& rpcResiYuv, TComYuv*& rpcRecoYuv );
protected:
  
  // -------------------------------------------------------------------------------------------------------------------
  // Intra search
  // -------------------------------------------------------------------------------------------------------------------
  
  Void  xEncSubdivCbfQT           ( TComDataCU*  pcCU,
                                    UInt         uiTrDepth,
                                    UInt         uiAbsPartIdx,
                                    Bool         bLuma,
                                    Bool         bChroma );
  Void  xEncCoeffQT               ( TComDataCU*  pcCU,
                                    UInt         uiTrDepth,
                                    UInt         uiAbsPartIdx,
                                    TextType     eTextType,
                                    Bool         bRealCoeff );
  Void  xEncIntraHeader           ( TComDataCU*  pcCU,
                                    UInt         uiTrDepth,
                                    UInt         uiAbsPartIdx,
                                    Bool         bLuma,
                                    Bool         bChroma );
  UInt  xGetIntraBitsQT           ( TComDataCU*  pcCU,
                                    UInt         uiTrDepth,
                                    UInt         uiAbsPartIdx,
                                    Bool         bLuma,
                                    Bool         bChroma,
                                    Bool         bRealCoeff );
  
  Void  xIntraCodingLumaBlk       ( TComDataCU*  pcCU,
                                    UInt         uiTrDepth,
                                    UInt         uiAbsPartIdx,
                                    TComYuv*     pcOrgYuv, 
                                    TComYuv*     pcPredYuv, 
                                    TComYuv*     pcResiYuv, 
                                    Dist&        ruiDist 
#if LG_ZEROINTRADEPTHRESI_A0087
                                   ,Bool        bZeroResi = false
#endif
                                   );
  Void  xIntraCodingChromaBlk     ( TComDataCU*  pcCU,
                                    UInt         uiTrDepth,
                                    UInt         uiAbsPartIdx,
                                    TComYuv*     pcOrgYuv, 
                                    TComYuv*     pcPredYuv, 
                                    TComYuv*     pcResiYuv, 
                                    Dist&        ruiDist,
                                    UInt         uiChromaId );
  Void  xRecurIntraCodingQT       ( TComDataCU*  pcCU, 
                                    UInt         uiTrDepth,
                                    UInt         uiAbsPartIdx, 
                                    Bool         bLumaOnly,
                                    TComYuv*     pcOrgYuv, 
                                    TComYuv*     pcPredYuv, 
                                    TComYuv*     pcResiYuv, 
                                    Dist&        ruiDistY,
                                    Dist&        ruiDistC,
#if HHI_RQT_INTRA_SPEEDUP
                                    Bool         bCheckFirst,
#endif
                                    Double&      dRDCost 
#if LG_ZEROINTRADEPTHRESI_A0087
                                   ,Bool         bZeroResi = false
#endif
                                  );
  
  Void  xSetIntraResultQT         ( TComDataCU*  pcCU,
                                    UInt         uiTrDepth,
                                    UInt         uiAbsPartIdx,
                                    Bool         bLumaOnly,
                                    TComYuv*     pcRecoYuv );
  
  Void  xRecurIntraChromaCodingQT ( TComDataCU*  pcCU, 
                                    UInt         uiTrDepth,
                                    UInt         uiAbsPartIdx, 
                                    TComYuv*     pcOrgYuv, 
                                    TComYuv*     pcPredYuv, 
                                    TComYuv*     pcResiYuv, 
                                    Dist&        ruiDist );
  Void  xSetIntraResultChromaQT   ( TComDataCU*  pcCU,
                                    UInt         uiTrDepth,
                                    UInt         uiAbsPartIdx,
                                    TComYuv*     pcRecoYuv );
  
#if RWTH_SDC_DLT_B0036
  Void  xAnalyzeSegmentsSDC       ( Pel* pOrig,
                                   UInt uiStride,
                                   UInt uiSize,
                                   Pel* rpSegMeans,
                                   UInt uiNumSegments,
                                   Bool* pMask,
                                   UInt uiMaskStride );
  
  Void  xIntraCodingSDC           ( TComDataCU* pcCU, UInt uiAbsPartIdx, TComYuv* pcOrgYuv, TComYuv* pcPredYuv, Dist& ruiDist, Double& dRDCost, Bool bResidual );
#endif
  
  // -------------------------------------------------------------------------------------------------------------------
  // DMM intra search
  // -------------------------------------------------------------------------------------------------------------------

#if HHI_DMM_WEDGE_INTRA || HHI_DMM_PRED_TEX
  Bool predIntraLumaDMMAvailable  ( UInt           uiMode, 
                                    UInt           uiWidth, 
#if HHI_DMM_PRED_TEX && FLEX_CODING_ORDER_M23723
                                    UInt         uiHeight, 
                                    Bool         bDMMAvailable34 );
#else
                                    UInt         uiHeight );
#endif

  Void xGetWedgeDeltaDCsMinDist   ( TComWedgelet*  pcWedgelet, 
                                    TComDataCU*    pcCU, 
                                    UInt           uiAbsPtIdx, 
                                    Pel*           piOrig, 
                                    Pel*           piPredic, 
                                    UInt           uiStride, 
                                    UInt           uiWidth, 
                                    UInt           uiHeight, 
                                    Int&           riDeltaDC1, 
                                    Int&           riDeltaDC2, 
                                    Bool           bAboveAvail, 
                                    Bool           bLeftAvail );
#endif

#if LGE_EDGE_INTRA_A0070
  Bool  xEdgePartition       ( TComDataCU* pcCU, UInt uiPartIdx, Bool bPU4x4 );
  Bool  xCheckTerminatedEdge ( Bool* pbEdge, Int iX, Int iY, Int iWidth, Int iHeight );
  Bool  xConstructChainCode  ( TComDataCU* pcCU, UInt uiPartIdx, Bool bPU4x4 );
#if LGE_EDGE_INTRA_DELTA_DC
  Void  xAssignEdgeIntraDeltaDCs( TComDataCU* pcCU, UInt uiAbsPartIdx, Pel* piOrig, UInt uiStride, Pel* piPredic, UInt uiWidth, UInt uiHeight );
#endif
#endif

#if HHI_DMM_WEDGE_INTRA
  Void findWedgeFullMinDist       ( TComDataCU*    pcCU, 
                                    UInt           uiAbsPtIdx,
                                    Pel*           piOrig,
                                    Pel*           piPredic,
                                    UInt           uiStride,
                                    UInt           uiWidth,
                                    UInt           uiHeight,
                                    UInt&          ruiTabIdx,
                                    Int&           riDeltaDC1,
                                    Int&           riDeltaDC2,
                                    Bool           bAboveAvail,
                                    Bool           bLeftAvail );
  Void findWedgePredDirMinDist    ( TComDataCU*    pcCU, 
                                    UInt           uiAbsPtIdx,
                                    Pel*           piOrig,
                                    Pel*           piPredic,
                                    UInt           uiStride,
                                    UInt           uiWidth,
                                    UInt           uiHeight,
                                    UInt&          ruiTabIdx,
                                    Int&           riWedgeDeltaEnd,
                                    Int&           riDeltaDC1,
                                    Int&           riDeltaDC2,
                                    Bool           bAboveAvail,
                                    Bool           bLeftAvail );
  Void xSearchWedgeFullMinDist    ( TComDataCU*    pcCU, 
                                    UInt           uiAbsPtIdx, 
                                    WedgeList*     pacWedgeList, 
                                    Pel*           piRef, 
                                    UInt           uiRefStride, 
                                    UInt           uiWidth, 
                                    UInt           uiHeight, 
                                    UInt&          ruiTabIdx, 
                                    Dist&          riDist );
#if HHIQC_DMMFASTSEARCH_B0039
  Void xSearchWedgeFullMinDistFast( TComDataCU*    pcCU, 
                                    UInt           uiAbsPtIdx, 
                                    WedgeNodeList* pacWedgeNodeList, 
                                    WedgeList*     pacWedgeList, 
                                    Pel*           piRef, 
                                    UInt           uiRefStride, 
                                    UInt           uiWidth, 
                                    UInt           uiHeight, 
                                    UInt&          ruiTabIdx, 
                                    Dist&          riDist );
#endif
  Void xSearchWedgePredDirMinDist ( TComDataCU*    pcCU, 
                                    UInt           uiAbsPtIdx, 
                                    WedgeList*     pacWedgeList, 
                                    Pel*           piRef, 
                                    UInt           uiRefStride, 
                                    UInt           uiWidth, 
                                    UInt           uiHeight, 
                                    UInt&          ruiTabIdx, 
                                    Int&           riWedgeDeltaEnd );
#endif
#if HHI_DMM_PRED_TEX
  Void findWedgeTexMinDist        ( TComDataCU*    pcCU, 
                                    UInt           uiAbsPtIdx,
                                    Pel*           piOrig,
                                    Pel*           piPredic,
                                    UInt           uiStride,
                                    UInt           uiWidth,
                                    UInt           uiHeight,
                                    UInt&          ruiTabIdx,
                                    Int&           riDeltaDC1,
                                    Int&           riDeltaDC2,
                                    Bool           bAboveAvail,
                                    Bool           bLeftAvail );
  Void findContourPredTex         ( TComDataCU*    pcCU, 
                                    UInt           uiAbsPtIdx,
                                    Pel*           piOrig,
                                    Pel*           piPredic,
                                    UInt           uiStride,
                                    UInt           uiWidth,
                                    UInt           uiHeight,
                                    Int&           riDeltaDC1,
                                    Int&           riDeltaDC2,
                                    Bool           bAboveAvail,
                                    Bool           bLeftAvail );
#endif

  // -------------------------------------------------------------------------------------------------------------------
  // Inter search (AMP)
  // -------------------------------------------------------------------------------------------------------------------
  
  Void xEstimateMvPredAMVP        ( TComDataCU* pcCU,
                                    TComYuv*    pcOrgYuv,
                                    UInt        uiPartIdx,
                                    RefPicList  eRefPicList,
                                    Int         iRefIdx,
                                    TComMv&     rcMvPred,
                                    Bool        bFilled = false
                                  #if H0111_MVD_L1_ZERO
                                  , UInt*       puiDistBiP = NULL
                                  #endif
                                  #if ZERO_MVD_EST
                                  , UInt*       puiDist = NULL
                                  #endif
                                     );
  
  Void xCheckBestMVP              ( TComDataCU* pcCU,
                                    RefPicList  eRefPicList,
                                    TComMv      cMv,
                                    TComMv&     rcMvPred,
                                    Int&        riMVPIdx,
                                    UInt&       ruiBits,
                                    UInt&       ruiCost );
  
  UInt xGetTemplateCost           ( TComDataCU* pcCU,
                                    UInt        uiPartIdx,
                                    UInt        uiPartAddr,
                                    TComYuv*    pcOrgYuv,
                                    TComYuv*    pcTemplateCand,
                                    TComMv      cMvCand,
                                    Int         iMVPIdx,
                                    Int         iMVPNum,
                                    RefPicList  eRefPicList,
                                    Int         iRefIdx,
                                    Int         iSizeX,
                                    Int         iSizeY
                                  #if ZERO_MVD_EST
                                  , UInt&       ruiDist
                                  #endif
                                   );
  
  
  Void xCopyAMVPInfo              ( AMVPInfo*   pSrc, AMVPInfo* pDst );
  UInt xGetMvpIdxBits             ( Int iIdx, Int iNum );
  Void xGetBlkBits                ( PartSize  eCUMode, Bool bPSlice, Int iPartIdx,  UInt uiLastMode, UInt uiBlkBit[3]);
  
  Void xMergeEstimation           ( TComDataCU*     pcCU,
                                    TComYuv*        pcYuvOrg,
#if LG_RESTRICTEDRESPRED_M24766
                                    TComYuv*        rpcResiPredYuv, 
#endif
                                    Int             iPartIdx,
                                    UInt&           uiInterDir,
                                    TComMvField*    pacMvField,
                                    UInt&           uiMergeIndex,
                                    UInt&           ruiCost
#if CU_BASED_MRG_CAND_LIST
                                  , TComMvField* cMvFieldNeighbours,  
                                    UChar* uhInterDirNeighbours,
                                    Int& numValidMergeCand
#endif
                                   );
  // -------------------------------------------------------------------------------------------------------------------
  // motion estimation
  // -------------------------------------------------------------------------------------------------------------------
  
  Void xMotionEstimation          ( TComDataCU*   pcCU,
                                    TComYuv*      pcYuvOrg,
                                    Int           iPartIdx,
                                    RefPicList    eRefPicList,
                                    TComMv*       pcMvPred,
                                    Int           iRefIdxPred,
                                    TComMv&       rcMv,
#if VSP_MV_ZERO
                                    Bool          bIsVsp,
#endif
                                    UInt&         ruiBits,
                                    UInt&         ruiCost,
                                    Bool          bBi = false  );
  
  Void xTZSearch                  ( TComDataCU*   pcCU,
                                    TComPattern*  pcPatternKey,
                                    Pel*          piRefY,
                                    Int           iRefStride,
                                    TComMv*       pcMvSrchRngLT,
                                    TComMv*       pcMvSrchRngRB,
                                    TComMv&       rcMv,
                                    UInt&         ruiSAD );
  
#if DV_V_RESTRICTION_B0037
  Void xSetSearchRange            ( TComDataCU*   pcCU,
                                    TComMv&       cMvPred,
                                    Int           iSrchRng,
                                    TComMv&       rcMvSrchRngLT,
                                    TComMv&       rcMvSrchRngRB,
                                    Bool          bDispSrchRngRst,
                                    Int           iDispVerSrchRng );
#else
  Void xSetSearchRange            ( TComDataCU*   pcCU,
                                    TComMv&       cMvPred,
                                    Int           iSrchRng,
                                    TComMv&       rcMvSrchRngLT,
                                    TComMv&       rcMvSrchRngRB );
#endif
  
  Void xPatternSearchFast         ( TComDataCU*   pcCU,
                                    TComPattern*  pcPatternKey,
                                    Pel*          piRefY,
                                    Int           iRefStride,
                                    TComMv*       pcMvSrchRngLT,
                                    TComMv*       pcMvSrchRngRB,
                                    TComMv&       rcMv,
                                    UInt&         ruiSAD );
  
  Void xPatternSearch             ( TComPattern*  pcPatternKey,
                                    Pel*          piRefY,
                                    Int           iRefStride,
                                    TComMv*       pcMvSrchRngLT,
                                    TComMv*       pcMvSrchRngRB,
                                    TComMv&       rcMv,
                                    UInt&         ruiSAD );
  
  Void xPatternSearchFracDIF      ( TComDataCU*   pcCU,
                                    TComPattern*  pcPatternKey,
                                    Pel*          piRefY,
                                    Int           iRefStride,
                                    TComMv*       pcMvInt,
                                    TComMv&       rcMvHalf,
                                    TComMv&       rcMvQter,
                                    UInt&         ruiCost 
                                   ,Bool biPred
#if VSP_MV_ZERO
                                   ,Bool bIsVsp
#endif
                                   );
  
  Void xExtDIFUpSamplingH( TComPattern* pcPattern, Bool biPred  );
  Void xExtDIFUpSamplingQ( TComPattern* pcPatternKey, TComMv halfPelRef, Bool biPred );
  
  // -------------------------------------------------------------------------------------------------------------------
  // T & Q & Q-1 & T-1
  // -------------------------------------------------------------------------------------------------------------------
  
  Void xEncodeResidualQT( TComDataCU* pcCU, UInt uiAbsPartIdx, const UInt uiDepth, Bool bSubdivAndCbf, TextType eType );
#if IBDI_DISTORTION || HHI_VSO
  Void xEstimateResidualQT( TComDataCU* pcCU, UInt uiQuadrant, UInt uiAbsPartIdx, UInt absTUPartIdx,TComYuv* pcOrg, TComYuv* pcPred, TComYuv* pcResi, const UInt uiDepth, Double &rdCost, UInt &ruiBits, Dist &ruiDist, Dist *puiZeroDist );
#else
  Void xEstimateResidualQT( TComDataCU* pcCU, UInt uiQuadrant, UInt uiAbsPartIdx, UInt absTUPartIdx                                , TComYuv* pcResi, const UInt uiDepth, Double &rdCost, UInt &ruiBits, Dist &ruiDist, Dist *puiZeroDist );
#endif
  Void xSetResidualQTData( TComDataCU* pcCU, UInt uiQuadrant, UInt uiAbsPartIdx,UInt absTUPartIdx, TComYuv* pcResi, UInt uiDepth, Bool bSpatial );
  
  UInt  xModeBitsIntra ( TComDataCU* pcCU, UInt uiMode, UInt uiPU, UInt uiPartOffset, UInt uiDepth, UInt uiInitTrDepth );
  UInt  xUpdateCandList( UInt uiMode, Double uiCost, UInt uiFastCandNum, UInt * CandModeList, Double * CandCostList );
  
  // -------------------------------------------------------------------------------------------------------------------
  // compute symbol bits
  // -------------------------------------------------------------------------------------------------------------------
  
  Void xAddSymbolBitsInter        ( TComDataCU*   pcCU,
                                   UInt          uiQp,
                                   UInt          uiTrMode,
                                   UInt&         ruiBits,
                                   TComYuv*&     rpcYuvRec,
                                   TComYuv*      pcYuvPred,
                                   TComYuv*&     rpcYuvResi );
  
  Void  setWpScalingDistParam( TComDataCU* pcCU, Int iRefIdx, RefPicList eRefPicListCur );
  inline  Void  setDistParamComp( UInt uiComp )  { m_cDistParam.uiComp = uiComp; }
  
};// END CLASS DEFINITION TEncSearch

//! \}

#endif // __TENCSEARCH__
