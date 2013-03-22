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

/** \file     TComPrediction.h
    \brief    prediction class (header)
*/

#ifndef __TCOMPREDICTION__
#define __TCOMPREDICTION__


// Include files
#include "TComPic.h"
#include "TComMotionInfo.h"
#include "TComPattern.h"
#include "TComTrQuant.h"
#include "TComInterpolationFilter.h"
#include "TComWeightPrediction.h"

#if HHI_DMM_WEDGE_INTRA || HHI_DMM_PRED_TEX
#include <math.h>
#endif

//! \ingroup TLibCommon
//! \{

// ====================================================================================================================
// Class definition
// ====================================================================================================================

/// prediction class
class TComPrediction : public TComWeightPrediction
{
protected:
  Int*      m_piYuvExt;
  Int       m_iYuvExtStride;
  Int       m_iYuvExtHeight;
  
  TComYuv   m_acYuvPred[2];
  TComYuv   m_cYuvPredTemp;
  TComYuv m_filteredBlock[4][4];
  TComYuv m_filteredBlockTmp[4];
  
  TComInterpolationFilter m_if;
  
  Pel*   m_pLumaRecBuffer;       ///< array for downsampled reconstructed luma sample 
  Int    m_iLumaRecStride;       ///< stride of #m_pLumaRecBuffer array
  UInt   m_uiaShift[ 63 ];       // Table for multiplication to substitue of division operation

#if MERL_VSP_C0152
  Int*   m_pDepth; ///< Local variable, to store a depth block, just to prevent allocate memory every time
#endif

  Void xPredIntraAng            ( Int* pSrc, Int srcStride, Pel*& rpDst, Int dstStride, UInt width, UInt height, UInt dirMode, Bool blkAboveAvailable, Bool blkLeftAvailable, Bool bFilter );
  Void xPredIntraPlanar         ( Int* pSrc, Int srcStride, Pel* rpDst, Int dstStride, UInt width, UInt height );
  
  // motion compensation functions
#if DEPTH_MAP_GENERATION
  Void xPredInterUni            ( TComDataCU* pcCU,                          UInt uiPartAddr,               Int iWidth, Int iHeight, RefPicList eRefPicList, TComYuv*& rpcYuvPred, Int iPartIdx, Bool bPrdDepthMap, UInt uiSubSampExpX = 0, UInt uiSubSampExpY = 0, Bool bi=false );
#else
  Void xPredInterUni            ( TComDataCU* pcCU,                          UInt uiPartAddr,               Int iWidth, Int iHeight, RefPicList eRefPicList, TComYuv*& rpcYuvPred, Int iPartIdx, Bool bi=false          );
#endif

#if DEPTH_MAP_GENERATION
  Void xPredInterBi             ( TComDataCU* pcCU,                          UInt uiPartAddr,               Int iWidth, Int iHeight, UInt uiSubSampExpX, UInt uiSubSampExpY, TComYuv*& rpcYuvPred, Int iPartIdx, Bool bPrdDepthMap );
  Void xPredInterPrdDepthMap    ( TComDataCU* pcCU, TComPicYuv* pcPicYuvRef, UInt uiPartAddr, TComMv* pcMv, Int iWidth, Int iHeight, UInt uiSubSampExpX, UInt uiSubSampExpY, TComYuv*& rpcYuv, UInt uiRShift, UInt uiOffset );
#else
  Void xPredInterBi             ( TComDataCU* pcCU,                          UInt uiPartAddr,               Int iWidth, Int iHeight,                         TComYuv*& rpcYuvPred, Int iPartIdx          );
  Void xPredInterPrdDepthMap    ( TComDataCU* pcCU, TComPicYuv* pcPicYuvRef, UInt uiPartAddr, TComMv* pcMv, Int iWidth, Int iHeight,                         TComYuv*& rpcYuv, UInt uiRShift, UInt uiOffset );
#endif
#if MERL_VSP_C0152
  Void xPredInterUniBWVSP         ( TComDataCU* pcCU,                          UInt uiPartAddr, UInt uiAbsPartIdx,               Int iWidth, Int iHeight, RefPicList eRefPicList, TComYuv*& rpcYuvPred, Int iPartIdx, Bool bPrdDepthMap, UInt uiSubSampExpX, UInt uiSubSampExpY, Bool bi=false );
  Void xPredInterBiBWVSP          ( TComDataCU* pcCU,                          UInt uiPartAddr, UInt uiAbsPartIdx,               Int iWidth, Int iHeight, UInt uiSubSampExpX, UInt uiSubSampExpY, TComYuv*& rpcYuvPred, Int iPartIdx, Bool bPrdDepthMap );
#endif

#if DEPTH_MAP_GENERATION
  Void xWeightedAveragePdm      ( TComDataCU* pcCU, TComYuv* pcYuvSrc0, TComYuv* pcYuvSrc1, Int iRefIdx0, Int iRefIdx1, UInt uiPartAddr, Int iWidth, Int iHeight, TComYuv*& rpcYuvDst, UInt uiSubSampExpX, UInt uiSubSampExpY );
#endif

#if LGE_ILLUCOMP_B0045
  Void xPredInterLumaBlk  ( TComDataCU *cu, TComPicYuv *refPic, UInt partAddr, TComMv *mv, Int width, Int height, TComYuv *&dstPic, Bool bi, Bool bICFlag = false );
  Void xPredInterChromaBlk( TComDataCU *cu, TComPicYuv *refPic, UInt partAddr, TComMv *mv, Int width, Int height, TComYuv *&dstPic, Bool bi, Bool bICFlag = false );
#else
  Void xPredInterLumaBlk  ( TComDataCU *cu, TComPicYuv *refPic, UInt partAddr, TComMv *mv, Int width, Int height, TComYuv *&dstPic, Bool bi );
  Void xPredInterChromaBlk( TComDataCU *cu, TComPicYuv *refPic, UInt partAddr, TComMv *mv, Int width, Int height, TComYuv *&dstPic, Bool bi );
#endif
  Void xWeightedAverage         ( TComDataCU* pcCU, TComYuv* pcYuvSrc0, TComYuv* pcYuvSrc1, Int iRefIdx0, Int iRefIdx1, UInt uiPartAddr, Int iWidth, Int iHeight, TComYuv*& rpcYuvDst );

#if MERL_VSP_C0152
  Void xPredInterLumaBlkFromDM    ( TComPicYuv *refPic, TComPicYuv *pPicBaseDepth, Int* pShiftLUT, Int iShiftPrec, TComMv *mv, UInt partAddr,Int posX, Int posY, Int size_x, Int size_y, Bool isDepth, Int vspIdx
                                  , TComYuv *&dstPic );
  Void xPredInterChromaBlkFromDM  ( TComPicYuv *refPic, TComPicYuv *pPicBaseDepth, Int* pShiftLUT, Int iShiftPrec, TComMv *mv, UInt partAddr,Int posX, Int posY, Int size_x, Int size_y, Bool isDepth, Int vspIdx
                                  , TComYuv *&dstPic );
#endif
  Void xGetLLSPrediction ( TComPattern* pcPattern, Int* pSrc0, Int iSrcStride, Pel* pDst0, Int iDstStride, UInt uiWidth, UInt uiHeight, UInt uiExt0 );
#if LGE_ILLUCOMP_B0045
  Void xGetLLSICPrediction(TComDataCU* pcCU, TComMv *pMv, TComPicYuv *pRefPic, Int &a, Int &b, Int &iShift);
  Void xGetLLSICPredictionChroma(TComDataCU* pcCU, TComMv *pMv, TComPicYuv *pRefPic, Int &a, Int &b, Int &iShift, Int iChromaId);
#endif
  Void xDCPredFiltering( Int* pSrc, Int iSrcStride, Pel*& rpDst, Int iDstStride, Int iWidth, Int iHeight );
  Bool xCheckIdenticalMotion    ( TComDataCU* pcCU, UInt PartAddr);

#if HHI_DMM_WEDGE_INTRA
  Void xPredIntraWedgeFull       ( TComDataCU* pcCU, UInt uiAbsPartIdx, Pel* piPred, UInt uiStride, Int iWidth, Int iHeight, Bool bAbove, Bool bLeft, Bool bEncoder, Bool bDelta, UInt uiTabIdx, Int iDeltaDC1 = 0, Int iDeltaDC2 = 0 );
  Void xPredIntraWedgeDir        ( TComDataCU* pcCU, UInt uiAbsPartIdx, Pel* piPred, UInt uiStride, Int iWidth, Int iHeight, Bool bAbove, Bool bLeft, Bool bEncoder, Bool bDelta, Int iWedgeDeltaEnd, Int iDeltaDC1 = 0, Int iDeltaDC2 = 0 );
  Void xGetBlockOffset           ( TComDataCU* pcCU, UInt uiAbsPartIdx, TComDataCU* pcRefCU, UInt uiRefAbsPartIdx, UInt& ruiOffsetX, UInt& ruiOffsetY );
  Bool xGetWedgeIntraDirPredData ( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiBlockSize, Int& riSlopeX, Int& riSlopeY, UInt& ruiStartPosX, UInt& ruiStartPosY );
  Void xGetWedgeIntraDirStartEnd ( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiBlockSize, Int iDeltaX, Int iDeltaY, UInt uiPMSPosX, UInt uiPMSPosY, UChar& ruhXs, UChar& ruhYs, UChar& ruhXe, UChar& ruhYe, Int iDeltaEnd = 0 );
#endif
#if HHI_DMM_PRED_TEX
  Void xPredIntraWedgeTex        ( TComDataCU* pcCU, UInt uiAbsPartIdx, Pel* piPred, UInt uiStride, Int iWidth, Int iHeight, Bool bAbove, Bool bLeft, Bool bEncoder, Bool bDelta, Int iDeltaDC1 = 0, Int iDeltaDC2 = 0 );
  Void xPredIntraContourTex      ( TComDataCU* pcCU, UInt uiAbsPartIdx, Pel* piPred, UInt uiStride, Int iWidth, Int iHeight, Bool bAbove, Bool bLeft, Bool bEncoder, Bool bDelta, Int iDeltaDC1 = 0, Int iDeltaDC2 = 0 );
#endif
#if HHI_DMM_WEDGE_INTRA || HHI_DMM_PRED_TEX
  Void xDeltaDCQuantScaleUp      ( TComDataCU* pcCU, Int& riDeltaDC );
  Void xDeltaDCQuantScaleDown    ( TComDataCU* pcCU, Int& riDeltaDC );
#endif

#if LGE_EDGE_INTRA_A0070
  Pel  xGetNearestNeighbor  ( Int x, Int y, Int* pSrc, Int srcStride, Int iWidth, Int iHeight, Bool* bpRegion );
  Void xPredIntraEdge       ( TComDataCU* pcCU, UInt uiAbsPartIdx, Int iWidth, Int iHeight, Int* pSrc, Int srcStride, Pel*& rpDst, Int dstStride, Bool bDelta = false );
#endif

public:
  TComPrediction();
  virtual ~TComPrediction();
  
  Void    initTempBuff();
  
  // inter
#if DEPTH_MAP_GENERATION
  Void motionCompensation         ( TComDataCU*  pcCU, TComYuv* pcYuvPred, RefPicList eRefPicList = REF_PIC_LIST_X, Int iPartIdx = -1, Bool bPrdDepthMap = false, UInt uiSubSampExpX = 0, UInt uiSubSampExpY = 0 );
#else
  Void motionCompensation         ( TComDataCU*  pcCU, TComYuv* pcYuvPred, RefPicList eRefPicList = REF_PIC_LIST_X, Int iPartIdx = -1 );
#endif

#if MERL_VSP_C0152
   Void motionCompensationBWVSP   ( TComDataCU*  pcCU, TComYuv* pcYuvPred, UInt uiAbsPartIdx, RefPicList eRefPicList = REF_PIC_LIST_X, Int iPartIdx = -1, Bool bPrdDepthMap = false, UInt uiSubSampExpX = 0, UInt uiSubSampExpY = 0 );
#endif

  // motion vector prediction
  Void getMvPredAMVP              ( TComDataCU* pcCU, UInt uiPartIdx, UInt uiPartAddr, RefPicList eRefPicList, Int iRefIdx, TComMv& rcMvPred );
  
  // Angular Intra
  Void predIntraLumaAng           ( TComPattern* pcTComPattern, UInt uiDirMode, Pel* piPred, UInt uiStride, Int iWidth, Int iHeight,  TComDataCU* pcCU, Bool bAbove, Bool bLeft );
  Void predIntraChromaAng         ( TComPattern* pcTComPattern, Int* piSrc, UInt uiDirMode, Pel* piPred, UInt uiStride, Int iWidth, Int iHeight, TComDataCU* pcCU, Bool bAbove, Bool bLeft );
  
  Pel  predIntraGetPredValDC      ( Int* pSrc, Int iSrcStride, UInt iWidth, UInt iHeight, Bool bAbove, Bool bLeft );
  
  Int* getPredicBuf()             { return m_piYuvExt;      }
  Int  getPredicBufWidth()        { return m_iYuvExtStride; }
  Int  getPredicBufHeight()       { return m_iYuvExtHeight; }

  Void predLMIntraChroma( TComPattern* pcPattern, Int* piSrc, Pel* pPred, UInt uiPredStride, UInt uiCWidth, UInt uiCHeight, UInt uiChromaId );
  Void getLumaRecPixels  ( TComPattern* pcPattern, UInt uiWidth0, UInt uiHeight0 );

#if HHI_DMM_WEDGE_INTRA || HHI_DMM_PRED_TEX
  Void  predIntraLumaDMM        ( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiMode, Pel* piPred, UInt uiStride, Int iWidth, Int iHeight, Bool bAbove, Bool bLeft, Bool bEncoder );

  Void  getWedgePredDCs         ( TComWedgelet* pcWedgelet, Int* piMask, Int iMaskStride, Int& riPredDC1, Int& riPredDC2, Bool bAbove, Bool bLeft );
  Void  calcWedgeDCs            ( TComWedgelet* pcWedgelet, Pel* piOrig, UInt uiStride, Int& riDC1, Int& riDC2 );
  Void  assignWedgeDCs2Pred     ( TComWedgelet* pcWedgelet, Pel* piPred,  UInt uiStride, Int   iDC1, Int   iDC2 );
#endif
#if HHI_DMM_PRED_TEX
  Void  getBestContourFromTex   ( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiWidth, UInt uiHeight, TComWedgelet* pcContourWedge );
  UInt  getBestWedgeFromTex     ( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiWidth, UInt uiHeight );
  Void  copyTextureLumaBlock    ( TComDataCU* pcCU, UInt uiAbsPartIdx, Pel* piDestBlockY, UInt uiWidth, UInt uiHeight );
#endif
#if HHI_DMM_WEDGE_INTRA
  UInt  getBestContinueWedge    ( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiWidth, UInt uiHeight, Int iDeltaEnd = 0 );
  Bool  getWedgePatternIdx      ( WedgeRefList* pcWedgeRefList, UInt& ruiTabIdx, UChar uhXs, UChar uhYs, UChar uhXe, UChar uhYe );
#endif
#if LGE_EDGE_INTRA_A0070
  Void predIntraLumaEdge          ( TComDataCU* pcCU, TComPattern* pcTComPattern, UInt uiAbsPartIdx, Int iWidth, Int iHeight, Pel* piPred, UInt uiStride, Bool bDelta = false );
#endif

  // simplified intra pred for "virtual" depth maps
  Void  predIntraDepthAng ( TComPattern* pcTComPattern, UInt uiDirMode, Pel* piPred, UInt uiStride, Int iWidth, Int iHeight );
  Int   xGetDCDepth       ( Int* pSrc, Int iDelta, Int iBlkSize );
  Int   xGetDCValDepth    ( Int iVal1, Int iVal2, Int iVal3, Int iVal4 );
  Void  xPredIntraAngDepth( Int* pSrc, Int srcStride, Pel* pDst, Int dstStride, UInt width, UInt height, UInt dirMode );

};

//! \}

#endif // __TCOMPREDICTION__
