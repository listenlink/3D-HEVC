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
#include "TComPredFilter.h"
#ifdef WEIGHT_PRED
  #include "TComWeightPrediction.h"
#endif

// ====================================================================================================================
// Class definition
// ====================================================================================================================

/// prediction class
class TComPrediction : public TComPredFilter
#ifdef WEIGHT_PRED
  , public TComWeightPrediction
#endif
{
protected:
  Int*      m_piYuvExt;
  Int       m_iYuvExtStride;
  Int       m_iYuvExtHeight;
  
  TComYuv   m_acYuvPred[2];
  TComYuv   m_cYuvPredTemp;
  TComYuv   m_cYuvExt;
  
#if LM_CHROMA
  Pel*   m_pLumaRecBuffer;       // array for downsampled reconstructed luma sample 
  Int    m_iLumaRecStride;
  UInt   m_uiaShift[ 65 ];       // Table for multiplication to substitue of division operation
#endif

  Void xPredIntraAng            ( Int* pSrc, Int srcStride, Pel*& rpDst, Int dstStride, UInt width, UInt height, UInt dirMode, Bool blkAboveAvailable, Bool blkLeftAvailable );
#if ADD_PLANAR_MODE
#if REFERENCE_SAMPLE_PADDING
  Void xPredIntraPlanar         ( Int* pSrc, Int srcStride, Pel*& rpDst, Int dstStride, UInt width, UInt height );
#else
  Void xPredIntraPlanar         ( Int* pSrc, Int srcStride, Pel*& rpDst, Int dstStride, UInt width, UInt height, Bool blkAboveAvailable, Bool blkLeftAvailable );
#endif
#endif
  
  // motion compensation functions
#if HIGH_ACCURACY_BI
#if DEPTH_MAP_GENERATION
  Void xPredInterUni            ( TComDataCU* pcCU,                          UInt uiPartAddr,               Int iWidth, Int iHeight, RefPicList eRefPicList, TComYuv*& rpcYuvPred, Int iPartIdx, Bool bPrdDepthMap, Bool bi=false );
#else
  Void xPredInterUni            ( TComDataCU* pcCU,                          UInt uiPartAddr,               Int iWidth, Int iHeight, RefPicList eRefPicList, TComYuv*& rpcYuvPred, Int iPartIdx, Bool bi=false );
#endif
#else
#if DEPTH_MAP_GENERATION
  Void xPredInterUni            ( TComDataCU* pcCU,                          UInt uiPartAddr,               Int iWidth, Int iHeight, RefPicList eRefPicList, TComYuv*& rpcYuvPred, Int iPartIdx, Bool bPrdDepthMap );
#else
  Void xPredInterUni            ( TComDataCU* pcCU,                          UInt uiPartAddr,               Int iWidth, Int iHeight, RefPicList eRefPicList, TComYuv*& rpcYuvPred, Int iPartIdx );
#endif
#endif
  Void xPredInterBi             ( TComDataCU* pcCU,                          UInt uiPartAddr,               Int iWidth, Int iHeight,                         TComYuv*& rpcYuvPred, Int iPartIdx, Bool bPrdDepthMap );
  Void xPredInterPrdDepthMap    ( TComDataCU* pcCU, TComPicYuv* pcPicYuvRef, UInt uiPartAddr, TComMv* pcMv, Int iWidth, Int iHeight,                         TComYuv*& rpcYuv, UInt uiRShift, UInt uiFilterMode ); // 0:std, 1:bilin, 2:nearest neighbour
  Void xPredInterLumaBlk        ( TComDataCU* pcCU, TComPicYuv* pcPicYuvRef, UInt uiPartAddr, TComMv* pcMv, Int iWidth, Int iHeight,                         TComYuv*& rpcYuv );
  Void xPredInterChromaBlk      ( TComDataCU* pcCU, TComPicYuv* pcPicYuvRef, UInt uiPartAddr, TComMv* pcMv, Int iWidth, Int iHeight,                         TComYuv*& rpcYuv                            );
  Void xWeightedAverage         ( TComDataCU* pcCU, TComYuv* pcYuvSrc0, TComYuv* pcYuvSrc1, Int iRefIdx0, Int iRefIdx1, UInt uiPartAddr, Int iWidth, Int iHeight, TComYuv*& rpcYuvDst );
  Void xDCTIF_FilterC ( Pel*  piRefC, Int iRefStride,Pel*  piDstC,Int iDstStride,Int iWidth, Int iHeight,Int iMVyFrac,Int iMVxFrac);

#if HIGH_ACCURACY_BI
  Void xPredInterLumaBlk_ha        ( TComDataCU* pcCU, TComPicYuv* pcPicYuvRef, UInt uiPartAddr, TComMv* pcMv, Int iWidth, Int iHeight,                         TComYuv*& rpcYuv );
  Void xPredInterChromaBlk_ha      ( TComDataCU* pcCU, TComPicYuv* pcPicYuvRef, UInt uiPartAddr, TComMv* pcMv, Int iWidth, Int iHeight,                         TComYuv*& rpcYuv                            );
  Void xDCTIF_FilterC_ha ( Pel*  piRefC, Int iRefStride,Pel*  piDstC,Int iDstStride,Int iWidth, Int iHeight,Int iMVyFrac,Int iMVxFrac);
#endif

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
#endif

#if LM_CHROMA
  Void xGetRecPixels     ( TComPattern* pcPattern, Pel* pRecSrc, Int iRecSrcStride, Pel* pDst0, Int iDstStride, UInt uiWidth0, UInt uiHeight0 );   
  Void xGetLLSPrediction ( TComPattern* pcPattern, Int* pSrc0, Int iSrcStride, Pel* pDst0, Int iDstStride, UInt uiWidth, UInt uiHeight, UInt uiExt0 );
#endif

#if MN_DC_PRED_FILTER
  Void xDCPredFiltering( Int* pSrc, Int iSrcStride, Pel*& rpDst, Int iDstStride, Int iWidth, Int iHeight );
#endif

public:
  TComPrediction();
  virtual ~TComPrediction();
  
  Void    initTempBuff();
  
  // inter
  Void motionCompensation         ( TComDataCU*  pcCU, TComYuv* pcYuvPred, RefPicList eRefPicList = REF_PIC_LIST_X, Int iPartIdx = -1, Bool bPrdDepthMap = false );
  
  // motion vector prediction
  Void getMvPredAMVP              ( TComDataCU* pcCU, UInt uiPartIdx, UInt uiPartAddr, RefPicList eRefPicList, Int iRefIdx, TComMv& rcMvPred );
  
  // Angular Intra
  Void predIntraLumaAng           ( TComPattern* pcTComPattern, UInt uiDirMode, Pel* piPred, UInt uiStride, Int iWidth, Int iHeight,  TComDataCU* pcCU, Bool bAbove, Bool bLeft );
  Void predIntraChromaAng         ( TComPattern* pcTComPattern, Int* piSrc, UInt uiDirMode, Pel* piPred, UInt uiStride, Int iWidth, Int iHeight, TComDataCU* pcCU, Bool bAbove, Bool bLeft );
  
#if HHI_DMM_WEDGE_INTRA || HHI_DMM_PRED_TEX
  Void  predIntraLumaDMM         ( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiMode, Pel* piPred, UInt uiStride, Int iWidth, Int iHeight, Bool bAbove, Bool bLeft, Bool bEncoder );

  Void  getWedgePredDCs         ( TComWedgelet* pcWedgelet, Int* piMask, Int iMaskStride, Int& riPredDC1, Int& riPredDC2, Bool bAbove, Bool bLeft );
  Void  calcWedgeDCs             ( TComWedgelet* pcWedgelet, Pel* piOrig, UInt uiStride, Int& riDC1, Int& riDC2 );
  Void  assignWedgeDCs2Pred      ( TComWedgelet* pcWedgelet, Pel* piPred,  UInt uiStride, Int   iDC1, Int   iDC2 );
#endif
#if HHI_DMM_PRED_TEX
  Void  getBestContourFromText   ( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiWidth, UInt uiHeight, TComWedgelet* pcContourWedge, Pel* piTextureBlock = NULL );
  UInt  getBestWedgeFromText     ( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiWidth, UInt uiHeight, WedgeDist eWedgeDist = WedgeDist_SAD, Pel* piTextureBlock = NULL );
  Void  fillTexturePicTempBlock  ( TComDataCU* pcCU, UInt uiAbsPartIdx, Pel* piTempBlockY, UInt uiWidth, UInt uiHeight );
#endif
#if HHI_DMM_WEDGE_INTRA
  UInt  getBestContinueWedge     ( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiWidth, UInt uiHeight, Int iDeltaEnd = 0 );
  Bool  getWedgeListIdx          ( WedgeList* pcWedgeList, WedgeRefList* pcWedgeRefList, UInt& ruiTabIdx, UChar uhXs, UChar uhYs, UChar uhXe, UChar uhYe );
#endif
  
  Pel  predIntraGetPredValDC      ( Int* pSrc, Int iSrcStride, UInt iWidth, UInt iHeight, Bool bAbove, Bool bLeft );
  
  Int* getPredicBuf()             { return m_piYuvExt;      }
  Int  getPredicBufWidth()        { return m_iYuvExtStride; }
  Int  getPredicBufHeight()       { return m_iYuvExtHeight; }

#if LM_CHROMA
  Void predLMIntraChroma( TComPattern* pcPattern, Int* piSrc, Pel* pPred, UInt uiPredStride, UInt uiCWidth, UInt uiCHeight, UInt uiChromaId );
#endif

  // simplified intra pred for "virtual" depth maps
  Void  predIntraDepthAng ( TComPattern* pcTComPattern, UInt uiDirMode, Pel* piPred, UInt uiStride, Int iWidth, Int iHeight );
  Int   xGetDCDepth       ( Int* pSrc, Int iDelta, Int iBlkSize );
  Int   xGetDCValDepth    ( Int iVal1, Int iVal2, Int iVal3, Int iVal4 );
  Void  xPredIntraAngDepth( Int* pSrc, Int srcStride, Pel* pDst, Int dstStride, UInt width, UInt height, UInt dirMode );
};

#if HHI_DMM_WEDGE_INTRA || HHI_DMM_PRED_TEX
// ====================================================================================================================
// Class definition TComWedgeDist
// ====================================================================================================================
class TComWedgeDist
{
private:
  // for distortion
  Int                     m_iBlkWidth;
  Int                     m_iBlkHeight;

  FpDistFunc              m_afpDistortFunc[8]; // [eDFunc]
#ifdef ROUNDING_CONTROL_BIPRED
  FpDistFuncRnd           m_afpDistortFuncRnd[4];
#endif

public:
  TComWedgeDist();
  virtual ~TComWedgeDist();

  // Distortion Functions
  Void    init();

  Void    setDistParam( UInt uiBlkWidth, UInt uiBlkHeight, WedgeDist eWDist, DistParam& rcDistParam );
  Void    setDistParam( TComPattern* pcPatternKey, Pel* piRefY, Int iRefStride,            DistParam& rcDistParam );
  Void    setDistParam( TComPattern* pcPatternKey, Pel* piRefY, Int iRefStride, Int iStep, DistParam& rcDistParam, Bool bHADME=false );
  Void    setDistParam( DistParam& rcDP, Pel* p1, Int iStride1, Pel* p2, Int iStride2, Int iWidth, Int iHeight, Bool bHadamard = false );

#ifdef ROUNDING_CONTROL_BIPRED
  Void    setDistParam_Bi( TComPattern* pcPatternKey, Pel* piRefY, Int iRefStride,            DistParam& rcDistParam );
  Void    setDistParam_Bi( TComPattern* pcPatternKey, Pel* piRefY, Int iRefStride, Int iStep, DistParam& rcDistParam, Bool bHADME=false );
#endif

private:

  //   static UInt xGetSAD           ( DistParam* pcDtParam );
  static UInt xGetSAD4          ( DistParam* pcDtParam );
  static UInt xGetSAD8          ( DistParam* pcDtParam );
  static UInt xGetSAD16         ( DistParam* pcDtParam );
  static UInt xGetSAD32         ( DistParam* pcDtParam );
  //   static UInt xGetSAD64         ( DistParam* pcDtParam );

  static UInt xGetSSE4          ( DistParam* pcDtParam );
  static UInt xGetSSE8          ( DistParam* pcDtParam );
  static UInt xGetSSE16         ( DistParam* pcDtParam );
  static UInt xGetSSE32         ( DistParam* pcDtParam );

#ifdef ROUNDING_CONTROL_BIPRED
  //   static UInt xGetSAD           ( DistParam* pcDtParam, Pel* pRefY, Bool bRound );
  static UInt xGetSAD4          ( DistParam* pcDtParam, Pel* pRefY, Bool bRound );
  static UInt xGetSAD8          ( DistParam* pcDtParam, Pel* pRefY, Bool bRound );
  static UInt xGetSAD16         ( DistParam* pcDtParam, Pel* pRefY, Bool bRound );
  static UInt xGetSAD32         ( DistParam* pcDtParam, Pel* pRefY, Bool bRound );
  //   static UInt xGetSAD64         ( DistParam* pcDtParam, Pel* pRefY, Bool bRound );
#endif

public:
  UInt   getDistPart( Pel* piCur, Int iCurStride,  Pel* piOrg, Int iOrgStride, UInt uiBlkWidth, UInt uiBlkHeight, WedgeDist eWDist = WedgeDist_SAD );

};// END CLASS DEFINITION TComWedgeDist
#endif

#endif // __TCOMPREDICTION__

