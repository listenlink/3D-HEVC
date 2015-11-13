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

/** \file     TComPrediction.h
    \brief    prediction class (header)
*/

#ifndef __TCOMPREDICTION__
#define __TCOMPREDICTION__


// Include files
#include "TComYuv.h"
#include "TComInterpolationFilter.h"
#include "TComWeightPrediction.h"

#if NH_3D_ARP
#include "TComPic.h"
#endif
// forward declaration
class TComMv;
class TComTU; 

//! \ingroup TLibCommon
//! \{

// ====================================================================================================================
// Class definition
// ====================================================================================================================

/// prediction class
typedef enum PRED_BUF_E
{
  PRED_BUF_UNFILTERED=0,
  PRED_BUF_FILTERED=1,
  NUM_PRED_BUF=2
} PRED_BUF;

static const UInt MAX_INTRA_FILTER_DEPTHS=5;

class TComPrediction : public TComWeightPrediction
{
private:
  static const UChar m_aucIntraFilter[MAX_NUM_CHANNEL_TYPE][MAX_INTRA_FILTER_DEPTHS];

protected:
  Pel*      m_piYuvExt[MAX_NUM_COMPONENT][NUM_PRED_BUF];
  Int       m_iYuvExtSize;

  TComYuv   m_acYuvPred[NUM_REF_PIC_LIST_01];
  TComYuv   m_cYuvPredTemp;
#if NH_3D_ARP
  TComYuv   m_acYuvPredBase[2];
#endif
  TComYuv m_filteredBlock[LUMA_INTERPOLATION_FILTER_SUB_SAMPLE_POSITIONS][LUMA_INTERPOLATION_FILTER_SUB_SAMPLE_POSITIONS];
  TComYuv m_filteredBlockTmp[LUMA_INTERPOLATION_FILTER_SUB_SAMPLE_POSITIONS];

  TComInterpolationFilter m_if;

  Pel*   m_pLumaRecBuffer;       ///< array for downsampled reconstructed luma sample
  Int    m_iLumaRecStride;       ///< stride of #m_pLumaRecBuffer array
#if NH_3D_IC
  UInt   m_uiaShift[ 64 ];       // Table for multiplication to substitue of division operation
#endif

#if NH_3D_VSP
  Int*    m_pDepthBlock;         ///< Store a depth block, local variable, to prevent memory allocation every time
  TComYuv m_cYuvDepthOnVsp;
#endif
  Void xPredIntraAng            ( Int bitDepth, const Pel* pSrc, Int srcStride, Pel* pDst, Int dstStride, UInt width, UInt height, ChannelType channelType, UInt dirMode, const Bool bEnableEdgeFilters );
  Void xPredIntraPlanar         ( const Pel* pSrc, Int srcStride, Pel* rpDst, Int dstStride, UInt width, UInt height );

  // motion compensation functions
#if NH_3D_ARP
  Void xPredInterUniARP         ( TComDataCU* pcCU,                          UInt uiPartAddr,               Int iWidth, Int iHeight, RefPicList eRefPicList, TComYuv*& rpcYuvPred, Bool bi );
  Void xPredInterUniARPviewRef  ( TComDataCU* pcCU,                          UInt uiPartAddr,               Int iWidth, Int iHeight, RefPicList eRefPicList, TComYuv*& rpcYuvPred, Bool bi );
  Bool xCheckBiInterviewARP     ( TComDataCU* pcCU, UInt uiPartAddr, Int iWidth, Int iHeight, RefPicList eBaseRefPicList, TComPic*& pcPicYuvCurrTRef, TComMv& cBaseTMV, Int& iCurrTRefPoc );
#endif

  Void xPredInterUni            ( TComDataCU* pcCU,                          UInt uiPartAddr,               Int iWidth, Int iHeight, RefPicList eRefPicList, TComYuv* pcYuvPred, Bool bi=false          );
  Void xPredInterBi             ( TComDataCU* pcCU,                          UInt uiPartAddr,               Int iWidth, Int iHeight,                         TComYuv* pcYuvPred          );
#if NH_3D_VSP
  Void xPredInterUniVSP         ( TComDataCU* pcCU,                          UInt uiPartAddr,               Int iWidth, Int iHeight, RefPicList eRefPicList, TComYuv*& rpcYuvPred, Bool bi=false          );
  Void xPredInterBiVSP          ( TComDataCU* pcCU,                          UInt uiPartAddr,               Int iWidth, Int iHeight,                         TComYuv*& rpcYuvPred );
#endif

#if NH_3D
  Void xPredInterBlk(const ComponentID compID, TComDataCU *cu, TComPicYuv *refPic, UInt partAddr, TComMv *mv, Int width, Int height, TComYuv *dstPic, Bool bi, const Int bitDepth
#if NH_3D_ARP
    , Bool filterType = false
#endif
#if NH_3D_IC
    , Bool bICFlag    = false
#endif
 );
#else
  Void xPredInterBlk(const ComponentID compID, TComDataCU *cu, TComPicYuv *refPic, UInt partAddr, TComMv *mv, Int width, Int height, TComYuv *dstPic, Bool bi, const Int bitDepth );
#endif
#if NH_3D_VSP
  Void xPredInterUniSubPU        ( TComDataCU *cu, UInt uiPartAddr, Int iWidth, Int iHeight, RefPicList eRefPicList, TComYuv*& rpcYuvPred, Bool bi, Int widthSubPU=4, Int heightSubPU=4 );
#endif

  Void xWeightedAverage         ( TComYuv* pcYuvSrc0, TComYuv* pcYuvSrc1, Int iRefIdx0, Int iRefIdx1, UInt uiPartAddr, Int iWidth, Int iHeight, TComYuv* pcYuvDst, const BitDepths &clipBitDepths  );

  Void xGetLLSPrediction ( const Pel* pSrc0, Int iSrcStride, Pel* pDst0, Int iDstStride, UInt uiWidth, UInt uiHeight, UInt uiExt0, const ChromaFormat chFmt  DEBUG_STRING_FN_DECLARE(sDebug) );
#if NH_3D_IC
  Void xGetLLSICPrediction( const ComponentID compID, TComDataCU* pcCU, TComMv *pMv, TComPicYuv *pRefPic, Int &a, Int &b, const Int bitDepth);
#endif
  Void xDCPredFiltering( const Pel* pSrc, Int iSrcStride, Pel* pDst, Int iDstStride, Int iWidth, Int iHeight, ChannelType channelType );
  Bool xCheckIdenticalMotion    ( TComDataCU* pcCU, UInt PartAddr);
#if NH_3D_SPIVMP
  Bool xCheckTwoSPMotion ( TComDataCU* pcCU, UInt PartAddr0, UInt PartAddr1 );
  Void xGetSubPUAddrAndMerge(TComDataCU* pcCU, UInt uiPartAddr, Int iSPWidth, Int iSPHeight, Int iNumSPInOneLine, Int iNumSP, UInt* uiMergedSPW, UInt* uiMergedSPH, UInt* uiSPAddr );
#endif

  Void destroy();

public:
  TComPrediction();
  virtual ~TComPrediction();

  Void    initTempBuff(ChromaFormat chromaFormatIDC);

  ChromaFormat getChromaFormat() const { return m_cYuvPredTemp.getChromaFormat(); }

  // inter
  Void motionCompensation         ( TComDataCU*  pcCU, TComYuv* pcYuvPred, RefPicList eRefPicList = REF_PIC_LIST_X, Int iPartIdx = -1 );

  // motion vector prediction
  Void getMvPredAMVP              ( TComDataCU* pcCU, UInt uiPartIdx, UInt uiPartAddr, RefPicList eRefPicList, TComMv& rcMvPred );

  // Angular Intra
  Void predIntraAng               ( const ComponentID compID, UInt uiDirMode, Pel *piOrg /* Will be null for decoding */, UInt uiOrgStride, Pel* piPred, UInt uiStride, TComTU &rTu, const Bool bUseFilteredPredSamples, const Bool bUseLosslessDPCM = false );

  Pel  predIntraGetPredValDC      ( const Pel* pSrc, Int iSrcStride, UInt iWidth, UInt iHeight);
#if NH_3D_DMM
  Void predIntraLumaDmm           ( TComDataCU* pcCU, UInt uiAbsPartIdx, DmmID dmmType, Pel* piPred, UInt uiStride, Int iWidth, Int iHeight );
  Void predContourFromTex         ( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiWidth, UInt uiHeight, Bool* segPattern );
  Void predBiSegDCs               ( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiWidth, UInt uiHeight, Bool* biSegPattern, Int patternStride, Pel& predDC1, Pel& predDC2 );
  Void assignBiSegDCs             ( Pel* ptrDst, UInt dstStride, Bool* biSegPattern, Int patternStride, Pel valDC1, Pel valDC2 );
#endif
#if NH_3D_SDC_INTRA
  Void predConstantSDC            ( Pel* ptrSrc, UInt srcStride, UInt uiSize, Pel& predDC );
#endif
  
#if NH_3D_DBBP
  PartSize      getPartitionSizeFromDepth(Pel* pDepthPels, UInt uiDepthStride, UInt uiSize, TComDataCU*& pcCU);
  Bool          getSegmentMaskFromDepth( Pel* pDepthPels, UInt uiDepthStride, UInt uiWidth, UInt uiHeight, Bool* pMask, TComDataCU*& pcCU);
  Void          combineSegmentsWithMask( TComYuv* pInYuv[2], TComYuv* pOutYuv, Bool* pMask, UInt uiWidth, UInt uiHeight, UInt uiPartAddr, UInt partSize, Int bitDepthY );
#endif
#if NH_3D
  Pel  predIntraGetPredValDC      ( const Pel* pSrc, Int iSrcStride, UInt iWidth, UInt iHeight, Bool bAbove, Bool bLeft );
#endif
  Pel*  getPredictorPtr           ( const ComponentID compID, const Bool bUseFilteredPredictions )
  {
    return m_piYuvExt[compID][bUseFilteredPredictions?PRED_BUF_FILTERED:PRED_BUF_UNFILTERED];
  }

  // This function is actually still in TComPattern.cpp
  /// set parameters from CU data for accessing intra data
  Void initIntraPatternChType ( TComTU &rTu,
                              const ComponentID compID, const Bool bFilterRefSamples
                              DEBUG_STRING_FN_DECLARE(sDebug)
                              );

  static Bool filteringIntraReferenceSamples(const ComponentID compID, UInt uiDirMode, UInt uiTuChWidth, UInt uiTuChHeight, const ChromaFormat chFmt, const Bool intraReferenceSmoothingDisabled);

  static Bool UseDPCMForFirstPassIntraEstimation(TComTU &rTu, const UInt uiDirMode);
};

//! \}

#endif // __TCOMPREDICTION__
