/* The copyright in this software is being made available under the BSD
 * License, included below. This software may be subject to other third party
 * and contributor rights, including patent rights, and no such rights are
 * granted under this license.  
 *
 * Copyright (c) 2010-2013, ITU/ISO/IEC
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
#if H_3D_ARP
  TComYuv   m_acYuvPredBase[2];
#endif
  TComYuv m_filteredBlock[4][4];
  TComYuv m_filteredBlockTmp[4];
  
  TComInterpolationFilter m_if;
  
  Pel*   m_pLumaRecBuffer;       ///< array for downsampled reconstructed luma sample 
  Int    m_iLumaRecStride;       ///< stride of #m_pLumaRecBuffer array
#if H_3D_IC
  UInt   m_uiaShift[ 64 ];       // Table for multiplication to substitue of division operation
#endif

#if H_3D_VSP
  Int*   m_pDepthBlock;         ///< Store a depth block, local variable, to prevent memory allocation every time
#if H_3D_VSP_CONSTRAINED
  Int  xGetConstrainedSize(Int nPbW, Int nPbH, Bool bLuma = true);
#endif
  TComYuv   m_cYuvDepthOnVsp;
#endif

  Void xPredIntraAng            (Int bitDepth, Int* pSrc, Int srcStride, Pel*& rpDst, Int dstStride, UInt width, UInt height, UInt dirMode, Bool blkAboveAvailable, Bool blkLeftAvailable, Bool bFilter );
  Void xPredIntraPlanar         ( Int* pSrc, Int srcStride, Pel* rpDst, Int dstStride, UInt width, UInt height );
  
  // motion compensation functions
#if H_3D_ARP
  Void xPredInterUniARP         ( TComDataCU* pcCU,                          UInt uiPartAddr,               Int iWidth, Int iHeight, RefPicList eRefPicList, TComYuv*& rpcYuvPred, Bool bi=false, TComMvField * pNewMvFiled = NULL );
  Void xPredInterUniARPviewRef( TComDataCU* pcCU, UInt uiPartAddr, Int iWidth, Int iHeight, RefPicList eRefPicList, TComYuv*& rpcYuvPred, Bool bi, TComMvField * pNewMvFiled = NULL );
#endif
  Void xPredInterUni            ( TComDataCU* pcCU,                          UInt uiPartAddr,               Int iWidth, Int iHeight, RefPicList eRefPicList, TComYuv*& rpcYuvPred, Bool bi=false          );
  Void xPredInterBi             ( TComDataCU* pcCU,                          UInt uiPartAddr,               Int iWidth, Int iHeight,                         TComYuv*& rpcYuvPred );
#if H_3D_VSP
  Void xPredInterUniVSP         ( TComDataCU* pcCU,                          UInt uiPartAddr,               Int iWidth, Int iHeight, RefPicList eRefPicList, TComYuv*& rpcYuvPred, Bool bi=false          );
  Void xPredInterBiVSP          ( TComDataCU* pcCU,                          UInt uiPartAddr,               Int iWidth, Int iHeight,                         TComYuv*& rpcYuvPred );
#endif

  Void xPredInterLumaBlk  ( TComDataCU *cu, TComPicYuv *refPic, UInt partAddr, TComMv *mv, Int width, Int height, TComYuv *&dstPic, Bool bi
#if H_3D_ARP
    , Bool filterType = false
#endif
#if H_3D_IC
    , Bool bICFlag    = false
#endif
    );
  Void xPredInterChromaBlk( TComDataCU *cu, TComPicYuv *refPic, UInt partAddr, TComMv *mv, Int width, Int height, TComYuv *&dstPic, Bool bi

#if H_3D_ARP
    , Bool filterType = false
#endif
#if H_3D_IC
    , Bool bICFlag    = false
#endif
    );

#if H_3D_VSP
#if NTT_STORE_SPDV_VSP_G0148
  Void xPredInterUniSubPU        ( TComDataCU *cu, UInt uiPartAddr, Int iWidth, Int iHeight, RefPicList eRefPicList, TComYuv*& rpcYuvPred, Bool bi, Int widthSubPU=4, Int heightSubPU=4 );
#else
  Void xGetVirtualDepth           ( TComDataCU *cu, TComPicYuv *picRefDepth, TComMv *dv, UInt partAddr, Int width, Int height, TComYuv *yuvDepth, Int &vspSize, Int txtPerDepthX=1, Int txtPerDepthY=1 );
  Void xPredInterLumaBlkFromDM    ( TComDataCU *cu, TComPicYuv *picRef, TComYuv *yuvDepth, Int* shiftLUT, TComMv *mv, UInt partAddr, Int width, Int height, Bool isDepth, TComYuv *&pcYuvDst, Bool isBi, Int vspSize);
  Void xPredInterChromaBlkFromDM  ( TComDataCU *cu, TComPicYuv *picRef, TComYuv *yuvDepth, Int* shiftLUT, TComMv *mv, UInt partAddr, Int width, Int height, Bool isDepth, TComYuv *&pcYuvDst, Bool isBi, Int vspSize);
#endif
#endif

  Void xWeightedAverage         ( TComYuv* pcYuvSrc0, TComYuv* pcYuvSrc1, Int iRefIdx0, Int iRefIdx1, UInt uiPartAddr, Int iWidth, Int iHeight, TComYuv*& rpcYuvDst );
  
  Void xGetLLSPrediction ( TComPattern* pcPattern, Int* pSrc0, Int iSrcStride, Pel* pDst0, Int iDstStride, UInt uiWidth, UInt uiHeight, UInt uiExt0 );
#if H_3D_IC
  Void xGetLLSICPrediction( TComDataCU* pcCU, TComMv *pMv, TComPicYuv *pRefPic, Int &a, Int &b, TextType eType );
#endif
  Void xDCPredFiltering( Int* pSrc, Int iSrcStride, Pel*& rpDst, Int iDstStride, Int iWidth, Int iHeight );
  Bool xCheckIdenticalMotion    ( TComDataCU* pcCU, UInt PartAddr);
#if H_3D_SPIVMP
  Bool xCheckTwoSPMotion ( TComDataCU* pcCU, UInt PartAddr0, UInt PartAddr1 );
  Void xGetSubPUAddrAndMerge(TComDataCU* pcCU, UInt uiPartAddr, Int iSPWidth, Int iSPHeight, Int iNumSPInOneLine, Int iNumSP, UInt* uiMergedSPW, UInt* uiMergedSPH, UInt* uiSPAddr );
#endif
#if H_3D_DIM
  // depth intra functions
  Void xPredBiSegDCs            ( Int* ptrSrc, UInt srcStride, Bool* biSegPattern, Int patternStride, Pel& predDC1, Pel& predDC2 );
  Void xAssignBiSegDCs          ( Pel* ptrDst, UInt dstStride, Bool* biSegPattern, Int patternStride, Pel   valDC1, Pel   valDC2 );
#if H_3D_DIM_DMM
  UInt xPredWedgeFromTex        ( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiWidth, UInt uiHeight, UInt intraTabIdx );
  Void xPredContourFromTex      ( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiWidth, UInt uiHeight, TComWedgelet* pcContourWedge );
  Void xCopyTextureLumaBlock    ( TComDataCU* pcCU, UInt uiAbsPartIdx, Pel* piDestBlockY, UInt uiWidth, UInt uiHeight );
#endif
#endif

public:
  TComPrediction();
  virtual ~TComPrediction();
  
  Void    initTempBuff();
  
  // inter
  Void motionCompensation         ( TComDataCU*  pcCU, TComYuv* pcYuvPred, RefPicList eRefPicList = REF_PIC_LIST_X, Int iPartIdx = -1 );
  
  // motion vector prediction
  Void getMvPredAMVP              ( TComDataCU* pcCU, UInt uiPartIdx, UInt uiPartAddr, RefPicList eRefPicList, TComMv& rcMvPred );
  
  // Angular Intra
  Void predIntraLumaAng           ( TComPattern* pcTComPattern, UInt uiDirMode, Pel* piPred, UInt uiStride, Int iWidth, Int iHeight, Bool bAbove, Bool bLeft );
  Void predIntraChromaAng         ( Int* piSrc, UInt uiDirMode, Pel* piPred, UInt uiStride, Int iWidth, Int iHeight, Bool bAbove, Bool bLeft );
  
#if H_3D_DIM
  // Depth intra
  Void predIntraLumaDepth         ( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiIntraMode, Pel* piPred, UInt uiStride, Int iWidth, Int iHeight, Bool bFastEnc = false );
#if H_3D_DIM_SDC
  Void analyzeSegmentsSDC         ( Pel* pOrig, UInt uiStride, UInt uiSize, Pel* rpSegMeans, UInt uiNumSegments, Bool* pMask, UInt uiMaskStride
                                    ,UInt uiIntraMode 
                                    ,Bool orgDC=false
    );
#endif
#endif
  
#if H_3D_DBBP
  PartSize      getPartitionSizeFromDepth(Pel* pDepthPels, UInt uiDepthStride, UInt uiSize);
  Bool          getSegmentMaskFromDepth( Pel* pDepthPels, UInt uiDepthStride, UInt uiWidth, UInt uiHeight, Bool* pMask );
  Void          combineSegmentsWithMask( TComYuv* pInYuv[2], TComYuv* pOutYuv, Bool* pMask, UInt uiWidth, UInt uiHeight, UInt uiPartAddr = 0 );
#endif

  Pel  predIntraGetPredValDC      ( Int* pSrc, Int iSrcStride, UInt iWidth, UInt iHeight, Bool bAbove, Bool bLeft );
  
  Int* getPredicBuf()             { return m_piYuvExt;      }
  Int  getPredicBufWidth()        { return m_iYuvExtStride; }
  Int  getPredicBufHeight()       { return m_iYuvExtHeight; }

};

//! \}

#endif // __TCOMPREDICTION__
