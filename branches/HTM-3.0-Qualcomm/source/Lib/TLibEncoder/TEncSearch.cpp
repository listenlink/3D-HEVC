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

/** \file     TEncSearch.cpp
 \brief    encoder search class
 */

#include "TLibCommon/TypeDef.h"
#include "TLibCommon/TComRom.h"
#include "TLibCommon/TComMotionInfo.h"
#include "TEncSearch.h"
#include <math.h>

//! \ingroup TLibEncoder
//! \{

static TComMv s_acMvRefineH[9] =
{
  TComMv(  0,  0 ), // 0
  TComMv(  0, -1 ), // 1
  TComMv(  0,  1 ), // 2
  TComMv( -1,  0 ), // 3
  TComMv(  1,  0 ), // 4
  TComMv( -1, -1 ), // 5
  TComMv(  1, -1 ), // 6
  TComMv( -1,  1 ), // 7
  TComMv(  1,  1 )  // 8
};

static TComMv s_acMvRefineQ[9] =
{
  TComMv(  0,  0 ), // 0
  TComMv(  0, -1 ), // 1
  TComMv(  0,  1 ), // 2
  TComMv( -1, -1 ), // 5
  TComMv(  1, -1 ), // 6
  TComMv( -1,  0 ), // 3
  TComMv(  1,  0 ), // 4
  TComMv( -1,  1 ), // 7
  TComMv(  1,  1 )  // 8
};

static UInt s_auiDFilter[9] =
{
  0, 1, 0,
  2, 3, 2,
  0, 1, 0
};

TEncSearch::TEncSearch()
{
  m_ppcQTTempCoeffY  = NULL;
  m_ppcQTTempCoeffCb = NULL;
  m_ppcQTTempCoeffCr = NULL;
  m_pcQTTempCoeffY   = NULL;
  m_pcQTTempCoeffCb  = NULL;
  m_pcQTTempCoeffCr  = NULL;
#if ADAPTIVE_QP_SELECTION
  m_ppcQTTempArlCoeffY  = NULL;
  m_ppcQTTempArlCoeffCb = NULL;
  m_ppcQTTempArlCoeffCr = NULL;
  m_pcQTTempArlCoeffY   = NULL;
  m_pcQTTempArlCoeffCb  = NULL;
  m_pcQTTempArlCoeffCr  = NULL;
#endif
  m_puhQTTempTrIdx   = NULL;
  m_puhQTTempCbf[0] = m_puhQTTempCbf[1] = m_puhQTTempCbf[2] = NULL;
  m_pcQTTempTComYuv  = NULL;
  m_pcEncCfg = NULL;
  m_pcEntropyCoder = NULL;
  m_pTempPel = NULL;

  setWpScalingDistParam( NULL, -1, REF_PIC_LIST_X );
}

TEncSearch::~TEncSearch()
{
  if ( m_pTempPel )
  {
    delete [] m_pTempPel;
    m_pTempPel = NULL;
  }
  
  if ( m_pcEncCfg )
  {
    const UInt uiNumLayersAllocated = m_pcEncCfg->getQuadtreeTULog2MaxSize()-m_pcEncCfg->getQuadtreeTULog2MinSize()+1;
    for( UInt ui = 0; ui < uiNumLayersAllocated; ++ui )
    {
      delete[] m_ppcQTTempCoeffY[ui];
      delete[] m_ppcQTTempCoeffCb[ui];
      delete[] m_ppcQTTempCoeffCr[ui];
#if ADAPTIVE_QP_SELECTION
      delete[] m_ppcQTTempArlCoeffY[ui];
      delete[] m_ppcQTTempArlCoeffCb[ui];
      delete[] m_ppcQTTempArlCoeffCr[ui];
#endif
      m_pcQTTempTComYuv[ui].destroy();
    }
  }
  delete[] m_ppcQTTempCoeffY;
  delete[] m_ppcQTTempCoeffCb;
  delete[] m_ppcQTTempCoeffCr;
  delete[] m_pcQTTempCoeffY;
  delete[] m_pcQTTempCoeffCb;
  delete[] m_pcQTTempCoeffCr;
#if ADAPTIVE_QP_SELECTION
  delete[] m_ppcQTTempArlCoeffY;
  delete[] m_ppcQTTempArlCoeffCb;
  delete[] m_ppcQTTempArlCoeffCr;
  delete[] m_pcQTTempArlCoeffY;
  delete[] m_pcQTTempArlCoeffCb;
  delete[] m_pcQTTempArlCoeffCr;
#endif
  delete[] m_puhQTTempTrIdx;
  delete[] m_puhQTTempCbf[0];
  delete[] m_puhQTTempCbf[1];
  delete[] m_puhQTTempCbf[2];
  delete[] m_pcQTTempTComYuv;
  
  m_tmpYuvPred.destroy();
}

void TEncSearch::init(TEncCfg*      pcEncCfg,
                      TComTrQuant*  pcTrQuant,
                      Int           iSearchRange,
                      Int           bipredSearchRange,
                      Int           iFastSearch,
                      Int           iMaxDeltaQP,
                      TEncEntropy*  pcEntropyCoder,
                      TComRdCost*   pcRdCost,
                      TEncSbac*** pppcRDSbacCoder,
                      TEncSbac*   pcRDGoOnSbacCoder
                      )
{
  m_pcEncCfg             = pcEncCfg;
  m_pcTrQuant            = pcTrQuant;
  m_iSearchRange         = iSearchRange;
  m_bipredSearchRange    = bipredSearchRange;
  m_iFastSearch          = iFastSearch;
  m_iMaxDeltaQP          = iMaxDeltaQP;
  m_pcEntropyCoder       = pcEntropyCoder;
  m_pcRdCost             = pcRdCost;
  
  m_pppcRDSbacCoder     = pppcRDSbacCoder;
  m_pcRDGoOnSbacCoder   = pcRDGoOnSbacCoder;
  
  m_bUseSBACRD          = pppcRDSbacCoder ? true : false;
  
  for (Int iDir = 0; iDir < 2; iDir++)
  {
    for (Int iRefIdx = 0; iRefIdx < 33; iRefIdx++)
    {
      m_aaiAdaptSR[iDir][iRefIdx] = iSearchRange;
    }
  }
  
  m_puiDFilter = s_auiDFilter + 4;
  
  // initialize motion cost
#if !FIX203
  m_pcRdCost->initRateDistortionModel( m_iSearchRange << 2 );
#endif
  
#if HHI_INTER_VIEW_MOTION_PRED
  const Int iNumAMVPCands = AMVP_MAX_NUM_CANDS + 1;
  for( Int iNum = 0; iNum < iNumAMVPCands+1; iNum++)
  {
    for( Int iIdx = 0; iIdx < iNumAMVPCands; iIdx++)
#else
  for( Int iNum = 0; iNum < AMVP_MAX_NUM_CANDS+1; iNum++)
  {
    for( Int iIdx = 0; iIdx < AMVP_MAX_NUM_CANDS; iIdx++)
#endif
    {
      if (iIdx < iNum)
        m_auiMVPIdxCost[iIdx][iNum] = xGetMvpIdxBits(iIdx, iNum);
      else
        m_auiMVPIdxCost[iIdx][iNum] = MAX_INT;
    }
  }
  
  initTempBuff();
  
  m_pTempPel = new Pel[g_uiMaxCUWidth*g_uiMaxCUHeight];
  
  const UInt uiNumLayersToAllocate = pcEncCfg->getQuadtreeTULog2MaxSize()-pcEncCfg->getQuadtreeTULog2MinSize()+1;
  m_ppcQTTempCoeffY  = new TCoeff*[uiNumLayersToAllocate];
  m_ppcQTTempCoeffCb = new TCoeff*[uiNumLayersToAllocate];
  m_ppcQTTempCoeffCr = new TCoeff*[uiNumLayersToAllocate];
  m_pcQTTempCoeffY   = new TCoeff [g_uiMaxCUWidth*g_uiMaxCUHeight   ];
  m_pcQTTempCoeffCb  = new TCoeff [g_uiMaxCUWidth*g_uiMaxCUHeight>>2];
  m_pcQTTempCoeffCr  = new TCoeff [g_uiMaxCUWidth*g_uiMaxCUHeight>>2];
#if ADAPTIVE_QP_SELECTION
  m_ppcQTTempArlCoeffY  = new Int*[uiNumLayersToAllocate];
  m_ppcQTTempArlCoeffCb = new Int*[uiNumLayersToAllocate];
  m_ppcQTTempArlCoeffCr = new Int*[uiNumLayersToAllocate];
  m_pcQTTempArlCoeffY   = new Int [g_uiMaxCUWidth*g_uiMaxCUHeight   ];
  m_pcQTTempArlCoeffCb  = new Int [g_uiMaxCUWidth*g_uiMaxCUHeight>>2];
  m_pcQTTempArlCoeffCr  = new Int [g_uiMaxCUWidth*g_uiMaxCUHeight>>2];
#endif
  
  const UInt uiNumPartitions = 1<<(g_uiMaxCUDepth<<1);
  m_puhQTTempTrIdx   = new UChar  [uiNumPartitions];
  m_puhQTTempCbf[0]  = new UChar  [uiNumPartitions];
  m_puhQTTempCbf[1]  = new UChar  [uiNumPartitions];
  m_puhQTTempCbf[2]  = new UChar  [uiNumPartitions];
  m_pcQTTempTComYuv  = new TComYuv[uiNumLayersToAllocate];
  for( UInt ui = 0; ui < uiNumLayersToAllocate; ++ui )
  {
    m_ppcQTTempCoeffY[ui]  = new TCoeff[g_uiMaxCUWidth*g_uiMaxCUHeight   ];
    m_ppcQTTempCoeffCb[ui] = new TCoeff[g_uiMaxCUWidth*g_uiMaxCUHeight>>2];
    m_ppcQTTempCoeffCr[ui] = new TCoeff[g_uiMaxCUWidth*g_uiMaxCUHeight>>2];
#if ADAPTIVE_QP_SELECTION
    m_ppcQTTempArlCoeffY[ui]  = new Int[g_uiMaxCUWidth*g_uiMaxCUHeight   ];
    m_ppcQTTempArlCoeffCb[ui] = new Int[g_uiMaxCUWidth*g_uiMaxCUHeight>>2];
    m_ppcQTTempArlCoeffCr[ui] = new Int[g_uiMaxCUWidth*g_uiMaxCUHeight>>2];
#endif
    m_pcQTTempTComYuv[ui].create( g_uiMaxCUWidth, g_uiMaxCUHeight );
  }
  
  m_tmpYuvPred.create(MAX_CU_SIZE, MAX_CU_SIZE);
}

#if FASTME_SMOOTHER_MV
#define FIRSTSEARCHSTOP     1
#else
#define FIRSTSEARCHSTOP     0
#endif

#define TZ_SEARCH_CONFIGURATION                                                                                 \
const Int  iRaster                  = 5;  /* TZ soll von aussen ?ergeben werden */                            \
const Bool bTestOtherPredictedMV    = 0;                                                                      \
const Bool bTestZeroVector          = 1;                                                                      \
const Bool bTestZeroVectorStart     = 0;                                                                      \
const Bool bTestZeroVectorStop      = 0;                                                                      \
const Bool bFirstSearchDiamond      = 1;  /* 1 = xTZ8PointDiamondSearch   0 = xTZ8PointSquareSearch */        \
const Bool bFirstSearchStop         = FIRSTSEARCHSTOP;                                                        \
const UInt uiFirstSearchRounds      = 3;  /* first search stop X rounds after best match (must be >=1) */     \
const Bool bEnableRasterSearch      = 1;                                                                      \
const Bool bAlwaysRasterSearch      = 0;  /* ===== 1: BETTER but factor 2 slower ===== */                     \
const Bool bRasterRefinementEnable  = 0;  /* enable either raster refinement or star refinement */            \
const Bool bRasterRefinementDiamond = 0;  /* 1 = xTZ8PointDiamondSearch   0 = xTZ8PointSquareSearch */        \
const Bool bStarRefinementEnable    = 1;  /* enable either star refinement or raster refinement */            \
const Bool bStarRefinementDiamond   = 1;  /* 1 = xTZ8PointDiamondSearch   0 = xTZ8PointSquareSearch */        \
const Bool bStarRefinementStop      = 0;                                                                      \
const UInt uiStarRefinementRounds   = 2;  /* star refinement stop X rounds after best match (must be >=1) */  \


__inline Void TEncSearch::xTZSearchHelp( TComPattern* pcPatternKey, IntTZSearchStruct& rcStruct, const Int iSearchX, const Int iSearchY, const UChar ucPointNr, const UInt uiDistance )
{
  UInt  uiSad;
  
  Pel*  piRefSrch;
  
  piRefSrch = rcStruct.piRefY + iSearchY * rcStruct.iYStride + iSearchX;
  
  //-- jclee for using the SAD function pointer
  m_pcRdCost->setDistParam( pcPatternKey, piRefSrch, rcStruct.iYStride,  m_cDistParam );
  
  // fast encoder decision: use subsampled SAD when rows > 8 for integer ME
  if ( m_pcEncCfg->getUseFastEnc() )
  {
    if ( m_cDistParam.iRows > 8 )
    {
      m_cDistParam.iSubShift = 1;
    }
  }

  setDistParamComp(0);  // Y component

  // distortion
  uiSad = m_cDistParam.DistFunc( &m_cDistParam );
  
  // motion cost
  uiSad += m_pcRdCost->getCost( iSearchX, iSearchY );
  
  if( uiSad < rcStruct.uiBestSad )
  {
    rcStruct.uiBestSad      = uiSad;
    rcStruct.iBestX         = iSearchX;
    rcStruct.iBestY         = iSearchY;
    rcStruct.uiBestDistance = uiDistance;
    rcStruct.uiBestRound    = 0;
    rcStruct.ucPointNr      = ucPointNr;
  }
}

__inline Void TEncSearch::xTZ2PointSearch( TComPattern* pcPatternKey, IntTZSearchStruct& rcStruct, TComMv* pcMvSrchRngLT, TComMv* pcMvSrchRngRB )
{
  Int   iSrchRngHorLeft   = pcMvSrchRngLT->getHor();
  Int   iSrchRngHorRight  = pcMvSrchRngRB->getHor();
  Int   iSrchRngVerTop    = pcMvSrchRngLT->getVer();
  Int   iSrchRngVerBottom = pcMvSrchRngRB->getVer();
  
  // 2 point search,                   //   1 2 3
  // check only the 2 untested points  //   4 0 5
  // around the start point            //   6 7 8
  Int iStartX = rcStruct.iBestX;
  Int iStartY = rcStruct.iBestY;
  switch( rcStruct.ucPointNr )
  {
    case 1:
    {
      if ( (iStartX - 1) >= iSrchRngHorLeft )
      {
        xTZSearchHelp( pcPatternKey, rcStruct, iStartX - 1, iStartY, 0, 2 );
      }
      if ( (iStartY - 1) >= iSrchRngVerTop )
      {
        xTZSearchHelp( pcPatternKey, rcStruct, iStartX, iStartY - 1, 0, 2 );
      }
    }
      break;
    case 2:
    {
      if ( (iStartY - 1) >= iSrchRngVerTop )
      {
        if ( (iStartX - 1) >= iSrchRngHorLeft )
        {
          xTZSearchHelp( pcPatternKey, rcStruct, iStartX - 1, iStartY - 1, 0, 2 );
        }
        if ( (iStartX + 1) <= iSrchRngHorRight )
        {
          xTZSearchHelp( pcPatternKey, rcStruct, iStartX + 1, iStartY - 1, 0, 2 );
        }
      }
    }
      break;
    case 3:
    {
      if ( (iStartY - 1) >= iSrchRngVerTop )
      {
        xTZSearchHelp( pcPatternKey, rcStruct, iStartX, iStartY - 1, 0, 2 );
      }
      if ( (iStartX + 1) <= iSrchRngHorRight )
      {
        xTZSearchHelp( pcPatternKey, rcStruct, iStartX + 1, iStartY, 0, 2 );
      }
    }
      break;
    case 4:
    {
      if ( (iStartX - 1) >= iSrchRngHorLeft )
      {
        if ( (iStartY + 1) <= iSrchRngVerBottom )
        {
          xTZSearchHelp( pcPatternKey, rcStruct, iStartX - 1, iStartY + 1, 0, 2 );
        }
        if ( (iStartY - 1) >= iSrchRngVerTop )
        {
          xTZSearchHelp( pcPatternKey, rcStruct, iStartX - 1, iStartY - 1, 0, 2 );
        }
      }
    }
      break;
    case 5:
    {
      if ( (iStartX + 1) <= iSrchRngHorRight )
      {
        if ( (iStartY - 1) >= iSrchRngVerTop )
        {
          xTZSearchHelp( pcPatternKey, rcStruct, iStartX + 1, iStartY - 1, 0, 2 );
        }
        if ( (iStartY + 1) <= iSrchRngVerBottom )
        {
          xTZSearchHelp( pcPatternKey, rcStruct, iStartX + 1, iStartY + 1, 0, 2 );
        }
      }
    }
      break;
    case 6:
    {
      if ( (iStartX - 1) >= iSrchRngHorLeft )
      {
        xTZSearchHelp( pcPatternKey, rcStruct, iStartX - 1, iStartY , 0, 2 );
      }
      if ( (iStartY + 1) <= iSrchRngVerBottom )
      {
        xTZSearchHelp( pcPatternKey, rcStruct, iStartX, iStartY + 1, 0, 2 );
      }
    }
      break;
    case 7:
    {
      if ( (iStartY + 1) <= iSrchRngVerBottom )
      {
        if ( (iStartX - 1) >= iSrchRngHorLeft )
        {
          xTZSearchHelp( pcPatternKey, rcStruct, iStartX - 1, iStartY + 1, 0, 2 );
        }
        if ( (iStartX + 1) <= iSrchRngHorRight )
        {
          xTZSearchHelp( pcPatternKey, rcStruct, iStartX + 1, iStartY + 1, 0, 2 );
        }
      }
    }
      break;
    case 8:
    {
      if ( (iStartX + 1) <= iSrchRngHorRight )
      {
        xTZSearchHelp( pcPatternKey, rcStruct, iStartX + 1, iStartY, 0, 2 );
      }
      if ( (iStartY + 1) <= iSrchRngVerBottom )
      {
        xTZSearchHelp( pcPatternKey, rcStruct, iStartX, iStartY + 1, 0, 2 );
      }
    }
      break;
    default:
    {
      assert( false );
    }
      break;
  } // switch( rcStruct.ucPointNr )
}

__inline Void TEncSearch::xTZ8PointSquareSearch( TComPattern* pcPatternKey, IntTZSearchStruct& rcStruct, TComMv* pcMvSrchRngLT, TComMv* pcMvSrchRngRB, const Int iStartX, const Int iStartY, const Int iDist )
{
  Int   iSrchRngHorLeft   = pcMvSrchRngLT->getHor();
  Int   iSrchRngHorRight  = pcMvSrchRngRB->getHor();
  Int   iSrchRngVerTop    = pcMvSrchRngLT->getVer();
  Int   iSrchRngVerBottom = pcMvSrchRngRB->getVer();
  
  // 8 point search,                   //   1 2 3
  // search around the start point     //   4 0 5
  // with the required  distance       //   6 7 8
  assert( iDist != 0 );
  const Int iTop        = iStartY - iDist;
  const Int iBottom     = iStartY + iDist;
  const Int iLeft       = iStartX - iDist;
  const Int iRight      = iStartX + iDist;
  rcStruct.uiBestRound += 1;
  
  if ( iTop >= iSrchRngVerTop ) // check top
  {
    if ( iLeft >= iSrchRngHorLeft ) // check top left
    {
      xTZSearchHelp( pcPatternKey, rcStruct, iLeft, iTop, 1, iDist );
    }
    // top middle
    xTZSearchHelp( pcPatternKey, rcStruct, iStartX, iTop, 2, iDist );
    
    if ( iRight <= iSrchRngHorRight ) // check top right
    {
      xTZSearchHelp( pcPatternKey, rcStruct, iRight, iTop, 3, iDist );
    }
  } // check top
  if ( iLeft >= iSrchRngHorLeft ) // check middle left
  {
    xTZSearchHelp( pcPatternKey, rcStruct, iLeft, iStartY, 4, iDist );
  }
  if ( iRight <= iSrchRngHorRight ) // check middle right
  {
    xTZSearchHelp( pcPatternKey, rcStruct, iRight, iStartY, 5, iDist );
  }
  if ( iBottom <= iSrchRngVerBottom ) // check bottom
  {
    if ( iLeft >= iSrchRngHorLeft ) // check bottom left
    {
      xTZSearchHelp( pcPatternKey, rcStruct, iLeft, iBottom, 6, iDist );
    }
    // check bottom middle
    xTZSearchHelp( pcPatternKey, rcStruct, iStartX, iBottom, 7, iDist );
    
    if ( iRight <= iSrchRngHorRight ) // check bottom right
    {
      xTZSearchHelp( pcPatternKey, rcStruct, iRight, iBottom, 8, iDist );
    }
  } // check bottom
}

__inline Void TEncSearch::xTZ8PointDiamondSearch( TComPattern* pcPatternKey, IntTZSearchStruct& rcStruct, TComMv* pcMvSrchRngLT, TComMv* pcMvSrchRngRB, const Int iStartX, const Int iStartY, const Int iDist )
{
  Int   iSrchRngHorLeft   = pcMvSrchRngLT->getHor();
  Int   iSrchRngHorRight  = pcMvSrchRngRB->getHor();
  Int   iSrchRngVerTop    = pcMvSrchRngLT->getVer();
  Int   iSrchRngVerBottom = pcMvSrchRngRB->getVer();
  
  // 8 point search,                   //   1 2 3
  // search around the start point     //   4 0 5
  // with the required  distance       //   6 7 8
  assert ( iDist != 0 );
  const Int iTop        = iStartY - iDist;
  const Int iBottom     = iStartY + iDist;
  const Int iLeft       = iStartX - iDist;
  const Int iRight      = iStartX + iDist;
  rcStruct.uiBestRound += 1;
  
  if ( iDist == 1 ) // iDist == 1
  {
    if ( iTop >= iSrchRngVerTop ) // check top
    {
      xTZSearchHelp( pcPatternKey, rcStruct, iStartX, iTop, 2, iDist );
    }
    if ( iLeft >= iSrchRngHorLeft ) // check middle left
    {
      xTZSearchHelp( pcPatternKey, rcStruct, iLeft, iStartY, 4, iDist );
    }
    if ( iRight <= iSrchRngHorRight ) // check middle right
    {
      xTZSearchHelp( pcPatternKey, rcStruct, iRight, iStartY, 5, iDist );
    }
    if ( iBottom <= iSrchRngVerBottom ) // check bottom
    {
      xTZSearchHelp( pcPatternKey, rcStruct, iStartX, iBottom, 7, iDist );
    }
  }
  else // if (iDist != 1)
  {
    if ( iDist <= 8 )
    {
      const Int iTop_2      = iStartY - (iDist>>1);
      const Int iBottom_2   = iStartY + (iDist>>1);
      const Int iLeft_2     = iStartX - (iDist>>1);
      const Int iRight_2    = iStartX + (iDist>>1);
      
      if (  iTop >= iSrchRngVerTop && iLeft >= iSrchRngHorLeft &&
          iRight <= iSrchRngHorRight && iBottom <= iSrchRngVerBottom ) // check border
      {
        xTZSearchHelp( pcPatternKey, rcStruct, iStartX,  iTop,      2, iDist    );
        xTZSearchHelp( pcPatternKey, rcStruct, iLeft_2,  iTop_2,    1, iDist>>1 );
        xTZSearchHelp( pcPatternKey, rcStruct, iRight_2, iTop_2,    3, iDist>>1 );
        xTZSearchHelp( pcPatternKey, rcStruct, iLeft,    iStartY,   4, iDist    );
        xTZSearchHelp( pcPatternKey, rcStruct, iRight,   iStartY,   5, iDist    );
        xTZSearchHelp( pcPatternKey, rcStruct, iLeft_2,  iBottom_2, 6, iDist>>1 );
        xTZSearchHelp( pcPatternKey, rcStruct, iRight_2, iBottom_2, 8, iDist>>1 );
        xTZSearchHelp( pcPatternKey, rcStruct, iStartX,  iBottom,   7, iDist    );
      }
      else // check border
      {
        if ( iTop >= iSrchRngVerTop ) // check top
        {
          xTZSearchHelp( pcPatternKey, rcStruct, iStartX, iTop, 2, iDist );
        }
        if ( iTop_2 >= iSrchRngVerTop ) // check half top
        {
          if ( iLeft_2 >= iSrchRngHorLeft ) // check half left
          {
            xTZSearchHelp( pcPatternKey, rcStruct, iLeft_2, iTop_2, 1, (iDist>>1) );
          }
          if ( iRight_2 <= iSrchRngHorRight ) // check half right
          {
            xTZSearchHelp( pcPatternKey, rcStruct, iRight_2, iTop_2, 3, (iDist>>1) );
          }
        } // check half top
        if ( iLeft >= iSrchRngHorLeft ) // check left
        {
          xTZSearchHelp( pcPatternKey, rcStruct, iLeft, iStartY, 4, iDist );
        }
        if ( iRight <= iSrchRngHorRight ) // check right
        {
          xTZSearchHelp( pcPatternKey, rcStruct, iRight, iStartY, 5, iDist );
        }
        if ( iBottom_2 <= iSrchRngVerBottom ) // check half bottom
        {
          if ( iLeft_2 >= iSrchRngHorLeft ) // check half left
          {
            xTZSearchHelp( pcPatternKey, rcStruct, iLeft_2, iBottom_2, 6, (iDist>>1) );
          }
          if ( iRight_2 <= iSrchRngHorRight ) // check half right
          {
            xTZSearchHelp( pcPatternKey, rcStruct, iRight_2, iBottom_2, 8, (iDist>>1) );
          }
        } // check half bottom
        if ( iBottom <= iSrchRngVerBottom ) // check bottom
        {
          xTZSearchHelp( pcPatternKey, rcStruct, iStartX, iBottom, 7, iDist );
        }
      } // check border
    }
    else // iDist > 8
    {
      if ( iTop >= iSrchRngVerTop && iLeft >= iSrchRngHorLeft &&
          iRight <= iSrchRngHorRight && iBottom <= iSrchRngVerBottom ) // check border
      {
        xTZSearchHelp( pcPatternKey, rcStruct, iStartX, iTop,    0, iDist );
        xTZSearchHelp( pcPatternKey, rcStruct, iLeft,   iStartY, 0, iDist );
        xTZSearchHelp( pcPatternKey, rcStruct, iRight,  iStartY, 0, iDist );
        xTZSearchHelp( pcPatternKey, rcStruct, iStartX, iBottom, 0, iDist );
        for ( Int index = 1; index < 4; index++ )
        {
          Int iPosYT = iTop    + ((iDist>>2) * index);
          Int iPosYB = iBottom - ((iDist>>2) * index);
          Int iPosXL = iStartX - ((iDist>>2) * index);
          Int iPosXR = iStartX + ((iDist>>2) * index);
          xTZSearchHelp( pcPatternKey, rcStruct, iPosXL, iPosYT, 0, iDist );
          xTZSearchHelp( pcPatternKey, rcStruct, iPosXR, iPosYT, 0, iDist );
          xTZSearchHelp( pcPatternKey, rcStruct, iPosXL, iPosYB, 0, iDist );
          xTZSearchHelp( pcPatternKey, rcStruct, iPosXR, iPosYB, 0, iDist );
        }
      }
      else // check border
      {
        if ( iTop >= iSrchRngVerTop ) // check top
        {
          xTZSearchHelp( pcPatternKey, rcStruct, iStartX, iTop, 0, iDist );
        }
        if ( iLeft >= iSrchRngHorLeft ) // check left
        {
          xTZSearchHelp( pcPatternKey, rcStruct, iLeft, iStartY, 0, iDist );
        }
        if ( iRight <= iSrchRngHorRight ) // check right
        {
          xTZSearchHelp( pcPatternKey, rcStruct, iRight, iStartY, 0, iDist );
        }
        if ( iBottom <= iSrchRngVerBottom ) // check bottom
        {
          xTZSearchHelp( pcPatternKey, rcStruct, iStartX, iBottom, 0, iDist );
        }
        for ( Int index = 1; index < 4; index++ )
        {
          Int iPosYT = iTop    + ((iDist>>2) * index);
          Int iPosYB = iBottom - ((iDist>>2) * index);
          Int iPosXL = iStartX - ((iDist>>2) * index);
          Int iPosXR = iStartX + ((iDist>>2) * index);
          
          if ( iPosYT >= iSrchRngVerTop ) // check top
          {
            if ( iPosXL >= iSrchRngHorLeft ) // check left
            {
              xTZSearchHelp( pcPatternKey, rcStruct, iPosXL, iPosYT, 0, iDist );
            }
            if ( iPosXR <= iSrchRngHorRight ) // check right
            {
              xTZSearchHelp( pcPatternKey, rcStruct, iPosXR, iPosYT, 0, iDist );
            }
          } // check top
          if ( iPosYB <= iSrchRngVerBottom ) // check bottom
          {
            if ( iPosXL >= iSrchRngHorLeft ) // check left
            {
              xTZSearchHelp( pcPatternKey, rcStruct, iPosXL, iPosYB, 0, iDist );
            }
            if ( iPosXR <= iSrchRngHorRight ) // check right
            {
              xTZSearchHelp( pcPatternKey, rcStruct, iPosXR, iPosYB, 0, iDist );
            }
          } // check bottom
        } // for ...
      } // check border
    } // iDist <= 8
  } // iDist == 1
}

//<--

UInt TEncSearch::xPatternRefinement( TComPattern* pcPatternKey,
                                    TComMv baseRefMv,
                                    Int iFrac, TComMv& rcMvFrac )
{
  UInt  uiDist;
  UInt  uiDistBest  = MAX_UINT;
  UInt  uiDirecBest = 0;
  
  Pel*  piRefPos;
  Int iRefStride = m_filteredBlock[0][0].getStride();
#if NS_HAD
  m_pcRdCost->setDistParam( pcPatternKey, m_filteredBlock[0][0].getLumaAddr(), iRefStride, 1, m_cDistParam, m_pcEncCfg->getUseHADME(), m_pcEncCfg->getUseNSQT() );
#else
  m_pcRdCost->setDistParam( pcPatternKey, m_filteredBlock[0][0].getLumaAddr(), iRefStride, 1, m_cDistParam, m_pcEncCfg->getUseHADME() );
#endif
  
  TComMv* pcMvRefine = (iFrac == 2 ? s_acMvRefineH : s_acMvRefineQ);
  
  for (UInt i = 0; i < 9; i++)
  {
    TComMv cMvTest = pcMvRefine[i];
    cMvTest += baseRefMv;
    
    Int horVal = cMvTest.getHor() * iFrac;
    Int verVal = cMvTest.getVer() * iFrac;
    piRefPos = m_filteredBlock[ verVal & 3 ][ horVal & 3 ].getLumaAddr();
    if ( horVal == 2 && ( verVal & 1 ) == 0 )
      piRefPos += 1;
    if ( ( horVal & 1 ) == 0 && verVal == 2 )
      piRefPos += iRefStride;
    cMvTest = pcMvRefine[i];
    cMvTest += rcMvFrac;

    setDistParamComp(0);  // Y component

    m_cDistParam.pCur = piRefPos;
    uiDist = m_cDistParam.DistFunc( &m_cDistParam );
    uiDist += m_pcRdCost->getCost( cMvTest.getHor(), cMvTest.getVer() );
    
    if ( uiDist < uiDistBest )
    {
      uiDistBest  = uiDist;
      uiDirecBest = i;
    }
  }
  
  rcMvFrac = pcMvRefine[uiDirecBest];
  
  return uiDistBest;
}

Void
TEncSearch::xEncSubdivCbfQT( TComDataCU*  pcCU,
                            UInt         uiTrDepth,
                            UInt         uiAbsPartIdx,
                            Bool         bLuma,
                            Bool         bChroma )
{
  UInt  uiFullDepth     = pcCU->getDepth(0) + uiTrDepth;
  UInt  uiTrMode        = pcCU->getTransformIdx( uiAbsPartIdx );
  UInt  uiSubdiv        = ( uiTrMode > uiTrDepth ? 1 : 0 );
  UInt  uiLog2TrafoSize = g_aucConvertToBit[pcCU->getSlice()->getSPS()->getMaxCUWidth()] + 2 - uiFullDepth;

  {
    if( pcCU->getPredictionMode(0) == MODE_INTRA && pcCU->getPartitionSize(0) == SIZE_NxN && uiTrDepth == 0 )
    {
      assert( uiSubdiv );
    }
    else if( uiLog2TrafoSize > pcCU->getSlice()->getSPS()->getQuadtreeTULog2MaxSize() )
    {
      assert( uiSubdiv );
    }
    else if( uiLog2TrafoSize == pcCU->getSlice()->getSPS()->getQuadtreeTULog2MinSize() )
    {
      assert( !uiSubdiv );
    }
    else if( uiLog2TrafoSize == pcCU->getQuadtreeTULog2MinSizeInCU(uiAbsPartIdx) )
    {
      assert( !uiSubdiv );
    }
    else
    {
      assert( uiLog2TrafoSize > pcCU->getQuadtreeTULog2MinSizeInCU(uiAbsPartIdx) );
      if( bLuma )
      {
        m_pcEntropyCoder->encodeTransformSubdivFlag( uiSubdiv, uiFullDepth );
      }
    }
  }
  
  if ( bChroma )
  {
    if( uiLog2TrafoSize > 2 )
    {
      if( uiTrDepth==0 || pcCU->getCbf( uiAbsPartIdx, TEXT_CHROMA_U, uiTrDepth-1 ) )
        m_pcEntropyCoder->encodeQtCbf( pcCU, uiAbsPartIdx, TEXT_CHROMA_U, uiTrDepth );
      if( uiTrDepth==0 || pcCU->getCbf( uiAbsPartIdx, TEXT_CHROMA_V, uiTrDepth-1 ) )
        m_pcEntropyCoder->encodeQtCbf( pcCU, uiAbsPartIdx, TEXT_CHROMA_V, uiTrDepth );
    }
  }

  if( uiSubdiv )
  {
    UInt uiQPartNum = pcCU->getPic()->getNumPartInCU() >> ( ( uiFullDepth + 1 ) << 1 );
    for( UInt uiPart = 0; uiPart < 4; uiPart++ )
    {
      xEncSubdivCbfQT( pcCU, uiTrDepth + 1, uiAbsPartIdx + uiPart * uiQPartNum, bLuma, bChroma );
    }
    return;
  }
  
  {
    //===== Cbfs =====
    if( bLuma )
    {
      m_pcEntropyCoder->encodeQtCbf( pcCU, uiAbsPartIdx, TEXT_LUMA,     uiTrMode );
    }
  }
}


Void
TEncSearch::xEncCoeffQT( TComDataCU*  pcCU,
                        UInt         uiTrDepth,
                        UInt         uiAbsPartIdx,
                        TextType     eTextType,
                        Bool         bRealCoeff )
{
  UInt  uiFullDepth     = pcCU->getDepth(0) + uiTrDepth;
  UInt  uiTrMode        = pcCU->getTransformIdx( uiAbsPartIdx );
  UInt  uiSubdiv        = ( uiTrMode > uiTrDepth ? 1 : 0 );
  UInt  uiLog2TrafoSize = g_aucConvertToBit[pcCU->getSlice()->getSPS()->getMaxCUWidth()] + 2 - uiFullDepth;
  UInt  uiChroma        = ( eTextType != TEXT_LUMA ? 1 : 0 );
  
  if( uiSubdiv )
  {
    UInt uiQPartNum = pcCU->getPic()->getNumPartInCU() >> ( ( uiFullDepth + 1 ) << 1 );
      for( UInt uiPart = 0; uiPart < 4; uiPart++ )
      {
        xEncCoeffQT( pcCU, uiTrDepth + 1, uiAbsPartIdx + uiPart * uiQPartNum, eTextType, bRealCoeff );
      }
    return;
  }
  
  if( eTextType != TEXT_LUMA && uiLog2TrafoSize == 2 )
  {
    assert( uiTrDepth > 0 );
    uiTrDepth--;
    UInt uiQPDiv = pcCU->getPic()->getNumPartInCU() >> ( ( pcCU->getDepth( 0 ) + uiTrDepth ) << 1 );
    Bool bFirstQ = ( ( uiAbsPartIdx % uiQPDiv ) == 0 );
    if( !bFirstQ )
    {
      return;
    }
  }
  
  //===== coefficients =====
  UInt    uiWidth         = pcCU->getWidth  ( 0 ) >> ( uiTrDepth + uiChroma );
  UInt    uiHeight        = pcCU->getHeight ( 0 ) >> ( uiTrDepth + uiChroma );
  UInt    uiCoeffOffset   = ( pcCU->getPic()->getMinCUWidth() * pcCU->getPic()->getMinCUHeight() * uiAbsPartIdx ) >> ( uiChroma << 1 );
  UInt    uiQTLayer       = pcCU->getSlice()->getSPS()->getQuadtreeTULog2MaxSize() - uiLog2TrafoSize;
  TCoeff* pcCoeff         = 0;
  switch( eTextType )
  {
    case TEXT_LUMA:     pcCoeff = ( bRealCoeff ? pcCU->getCoeffY () : m_ppcQTTempCoeffY [uiQTLayer] );  break;
    case TEXT_CHROMA_U: pcCoeff = ( bRealCoeff ? pcCU->getCoeffCb() : m_ppcQTTempCoeffCb[uiQTLayer] );  break;
    case TEXT_CHROMA_V: pcCoeff = ( bRealCoeff ? pcCU->getCoeffCr() : m_ppcQTTempCoeffCr[uiQTLayer] );  break;
    default:            assert(0);
  }
  pcCoeff += uiCoeffOffset;
  
  m_pcEntropyCoder->encodeCoeffNxN( pcCU, pcCoeff, uiAbsPartIdx, uiWidth, uiHeight, uiFullDepth, eTextType );
}


Void
TEncSearch::xEncIntraHeader( TComDataCU*  pcCU,
                            UInt         uiTrDepth,
                            UInt         uiAbsPartIdx,
                            Bool         bLuma,
                            Bool         bChroma )
{
  if( bLuma )
  {
    // CU header
    if( uiAbsPartIdx == 0 )
    {
      if( !pcCU->getSlice()->isIntra() )
      {
        m_pcEntropyCoder->encodeSkipFlag( pcCU, 0, true );
        m_pcEntropyCoder->encodePredMode( pcCU, 0, true );
      }
      
      m_pcEntropyCoder  ->encodePartSize( pcCU, 0, pcCU->getDepth(0), true );

      if (pcCU->isIntra(0) && pcCU->getPartitionSize(0) == SIZE_2Nx2N )
      {
        m_pcEntropyCoder->encodeIPCMInfo( pcCU, 0, true );

        if ( pcCU->getIPCMFlag (0))
        {
          return;
        }
      }
    }
    // luma prediction mode
    if( pcCU->getPartitionSize(0) == SIZE_2Nx2N )
    {
      if( uiAbsPartIdx == 0 )
      {
        m_pcEntropyCoder->encodeIntraDirModeLuma ( pcCU, 0 );
      }
    }
    else
    {
      UInt uiQNumParts = pcCU->getTotalNumPart() >> 2;
      if( uiTrDepth == 0 )
      {
        assert( uiAbsPartIdx == 0 );
        for( UInt uiPart = 0; uiPart < 4; uiPart++ )
        {
          m_pcEntropyCoder->encodeIntraDirModeLuma ( pcCU, uiPart * uiQNumParts );
        }
      }
      else if( ( uiAbsPartIdx % uiQNumParts ) == 0 )
      {
        m_pcEntropyCoder->encodeIntraDirModeLuma ( pcCU, uiAbsPartIdx );
      }
    }
  }
  if( bChroma )
  {
    // chroma prediction mode
    if( uiAbsPartIdx == 0 )
    {
      m_pcEntropyCoder->encodeIntraDirModeChroma( pcCU, 0, true );
    }
  }
}


UInt
TEncSearch::xGetIntraBitsQT( TComDataCU*  pcCU,
                            UInt         uiTrDepth,
                            UInt         uiAbsPartIdx,
                            Bool         bLuma,
                            Bool         bChroma,
                            Bool         bRealCoeff /* just for test */ )
{
  m_pcEntropyCoder->resetBits();
  xEncIntraHeader ( pcCU, uiTrDepth, uiAbsPartIdx, bLuma, bChroma );
  xEncSubdivCbfQT ( pcCU, uiTrDepth, uiAbsPartIdx, bLuma, bChroma );
  
  if( bLuma )
  {
    xEncCoeffQT   ( pcCU, uiTrDepth, uiAbsPartIdx, TEXT_LUMA,      bRealCoeff );
  }
  if( bChroma )
  {
    xEncCoeffQT   ( pcCU, uiTrDepth, uiAbsPartIdx, TEXT_CHROMA_U,  bRealCoeff );
    xEncCoeffQT   ( pcCU, uiTrDepth, uiAbsPartIdx, TEXT_CHROMA_V,  bRealCoeff );
  }
  UInt   uiBits = m_pcEntropyCoder->getNumberOfWrittenBits();
  return uiBits;
}



Void
TEncSearch::xIntraCodingLumaBlk( TComDataCU* pcCU,
                                UInt        uiTrDepth,
                                UInt        uiAbsPartIdx,
                                TComYuv*    pcOrgYuv, 
                                TComYuv*    pcPredYuv, 
                                TComYuv*    pcResiYuv, 
                                Dist&       ruiDist )
{
  UInt    uiLumaPredMode    = pcCU     ->getLumaIntraDir     ( uiAbsPartIdx );
  UInt    uiFullDepth       = pcCU     ->getDepth   ( 0 )  + uiTrDepth;
  UInt    uiWidth           = pcCU     ->getWidth   ( 0 ) >> uiTrDepth;
  UInt    uiHeight          = pcCU     ->getHeight  ( 0 ) >> uiTrDepth;
  UInt    uiStride          = pcOrgYuv ->getStride  ();
  Pel*    piOrg             = pcOrgYuv ->getLumaAddr( uiAbsPartIdx );
  Pel*    piPred            = pcPredYuv->getLumaAddr( uiAbsPartIdx );
  Pel*    piResi            = pcResiYuv->getLumaAddr( uiAbsPartIdx );
  Pel*    piReco            = pcPredYuv->getLumaAddr( uiAbsPartIdx );
  
  UInt    uiLog2TrSize      = g_aucConvertToBit[ pcCU->getSlice()->getSPS()->getMaxCUWidth() >> uiFullDepth ] + 2;
  UInt    uiQTLayer         = pcCU->getSlice()->getSPS()->getQuadtreeTULog2MaxSize() - uiLog2TrSize;
  UInt    uiNumCoeffPerInc  = pcCU->getSlice()->getSPS()->getMaxCUWidth() * pcCU->getSlice()->getSPS()->getMaxCUHeight() >> ( pcCU->getSlice()->getSPS()->getMaxCUDepth() << 1 );
  TCoeff* pcCoeff           = m_ppcQTTempCoeffY[ uiQTLayer ] + uiNumCoeffPerInc * uiAbsPartIdx;
#if ADAPTIVE_QP_SELECTION
  Int*    pcArlCoeff        = m_ppcQTTempArlCoeffY[ uiQTLayer ] + uiNumCoeffPerInc * uiAbsPartIdx;
#endif
  Pel*    piRecQt           = m_pcQTTempTComYuv[ uiQTLayer ].getLumaAddr( uiAbsPartIdx );
  UInt    uiRecQtStride     = m_pcQTTempTComYuv[ uiQTLayer ].getStride  ();
  
  UInt    uiZOrder          = pcCU->getZorderIdxInCU() + uiAbsPartIdx;
  Pel*    piRecIPred        = pcCU->getPic()->getPicYuvRec()->getLumaAddr( pcCU->getAddr(), uiZOrder );
  UInt    uiRecIPredStride  = pcCU->getPic()->getPicYuvRec()->getStride  ();
  
  //===== init availability pattern =====
  Bool  bAboveAvail = false;
  Bool  bLeftAvail  = false;
  pcCU->getPattern()->initPattern   ( pcCU, uiTrDepth, uiAbsPartIdx );
  pcCU->getPattern()->initAdiPattern( pcCU, uiAbsPartIdx, uiTrDepth, m_piYuvExt, m_iYuvExtStride, m_iYuvExtHeight, bAboveAvail, bLeftAvail );
  
  //===== get prediction signal =====
#if HHI_DMM_WEDGE_INTRA || HHI_DMM_PRED_TEX
  if( uiLumaPredMode >= NUM_INTRA_MODE )
  {
    predIntraLumaDMM( pcCU, uiAbsPartIdx, uiLumaPredMode, piPred, uiStride, uiWidth, uiHeight, bAboveAvail, bLeftAvail, true );
  }
  else
  {
#endif
  predIntraLumaAng( pcCU->getPattern(), uiLumaPredMode, piPred, uiStride, uiWidth, uiHeight, pcCU, bAboveAvail, bLeftAvail );
#if HHI_DMM_WEDGE_INTRA || HHI_DMM_PRED_TEX
  }
#endif
  
  //===== get residual signal =====
  {
    // get residual
    Pel*  pOrg    = piOrg;
    Pel*  pPred   = piPred;
    Pel*  pResi   = piResi;
    for( UInt uiY = 0; uiY < uiHeight; uiY++ )
    {
      for( UInt uiX = 0; uiX < uiWidth; uiX++ )
      {
        pResi[ uiX ] = pOrg[ uiX ] - pPred[ uiX ];
      }
      pOrg  += uiStride;
      pResi += uiStride;
      pPred += uiStride;
    }
  }
  
  //===== transform and quantization =====
  //--- init rate estimation arrays for RDOQ ---
  if( m_pcEncCfg->getUseRDOQ() )
  {
    m_pcEntropyCoder->estimateBit( m_pcTrQuant->m_pcEstBitsSbac, uiWidth, uiWidth, TEXT_LUMA );
  }
  //--- transform and quantization ---
  UInt uiAbsSum = 0;
  pcCU       ->setTrIdxSubParts ( uiTrDepth, uiAbsPartIdx, uiFullDepth );

#if H0736_AVC_STYLE_QP_RANGE
  m_pcTrQuant->setQPforQuant    ( pcCU->getQP( 0 ), !pcCU->getSlice()->getDepth(), pcCU->getSlice()->getSliceType(), TEXT_LUMA, pcCU->getSlice()->getSPS()->getQpBDOffsetY(), 0 );
#else
  m_pcTrQuant->setQPforQuant    ( pcCU->getQP( 0 ), !pcCU->getSlice()->getDepth(), pcCU->getSlice()->getSliceType(), TEXT_LUMA, 0 );
#endif

#if RDOQ_CHROMA_LAMBDA 
  m_pcTrQuant->selectLambda     (TEXT_LUMA);  
#endif
  m_pcTrQuant->transformNxN     ( pcCU, piResi, uiStride, pcCoeff, 
#if ADAPTIVE_QP_SELECTION
                                 pcArlCoeff, 
#endif
                                 uiWidth, uiHeight, uiAbsSum, TEXT_LUMA, uiAbsPartIdx );
  
  //--- set coded block flag ---
  pcCU->setCbfSubParts          ( ( uiAbsSum ? 1 : 0 ) << uiTrDepth, TEXT_LUMA, uiAbsPartIdx, uiFullDepth );
  //--- inverse transform ---
  if( uiAbsSum )
  {
    Int scalingListType = 0 + g_eTTable[(Int)TEXT_LUMA];
    assert(scalingListType < 6);
#if LOSSLESS_CODING
    m_pcTrQuant->invtransformNxN( pcCU, TEXT_LUMA,pcCU->getLumaIntraDir( uiAbsPartIdx ), piResi, uiStride, pcCoeff, uiWidth, uiHeight, scalingListType );
#else
    m_pcTrQuant->invtransformNxN( TEXT_LUMA,pcCU->getLumaIntraDir( uiAbsPartIdx ), piResi, uiStride, pcCoeff, uiWidth, uiHeight, scalingListType );
#endif
  }
  else
  {
    Pel* pResi = piResi;
    memset( pcCoeff, 0, sizeof( TCoeff ) * uiWidth * uiHeight );
    for( UInt uiY = 0; uiY < uiHeight; uiY++ )
    {
      memset( pResi, 0, sizeof( Pel ) * uiWidth );
      pResi += uiStride;
    }
  }
  
  //===== reconstruction =====
  {
    Pel* pPred      = piPred;
    Pel* pResi      = piResi;
    Pel* pReco      = piReco;
    Pel* pRecQt     = piRecQt;
    Pel* pRecIPred  = piRecIPred;
    for( UInt uiY = 0; uiY < uiHeight; uiY++ )
    {
      for( UInt uiX = 0; uiX < uiWidth; uiX++ )
      {
        pReco    [ uiX ] = Clip( pPred[ uiX ] + pResi[ uiX ] );
        pRecQt   [ uiX ] = pReco[ uiX ];
        pRecIPred[ uiX ] = pReco[ uiX ];
      }
      pPred     += uiStride;
      pResi     += uiStride;
      pReco     += uiStride;
      pRecQt    += uiRecQtStride;
      pRecIPred += uiRecIPredStride;
    }
  }
  
  //===== update distortion =====
#if HHI_VSO
  if ( m_pcRdCost->getUseVSO() )
  {
    ruiDist += m_pcRdCost->getDistVS  ( pcCU, uiAbsPartIdx, piReco, uiStride, piOrg, uiStride, uiWidth, uiHeight, false, 0 );
  }
  else
#endif
  {
  ruiDist += m_pcRdCost->getDistPart( piReco, uiStride, piOrg, uiStride, uiWidth, uiHeight );
}
}


Void
TEncSearch::xIntraCodingChromaBlk( TComDataCU* pcCU,
                                  UInt        uiTrDepth,
                                  UInt        uiAbsPartIdx,
                                  TComYuv*    pcOrgYuv, 
                                  TComYuv*    pcPredYuv, 
                                  TComYuv*    pcResiYuv, 
                                  Dist&       ruiDist,
                                  UInt        uiChromaId )
{
  UInt uiOrgTrDepth = uiTrDepth;
  UInt uiFullDepth  = pcCU->getDepth( 0 ) + uiTrDepth;
  UInt uiLog2TrSize = g_aucConvertToBit[ pcCU->getSlice()->getSPS()->getMaxCUWidth() >> uiFullDepth ] + 2;
  if( uiLog2TrSize == 2 )
  {
    assert( uiTrDepth > 0 );
    uiTrDepth--;
    UInt uiQPDiv = pcCU->getPic()->getNumPartInCU() >> ( ( pcCU->getDepth( 0 ) + uiTrDepth ) << 1 );
    Bool bFirstQ = ( ( uiAbsPartIdx % uiQPDiv ) == 0 );
    if( !bFirstQ )
    {
      return;
    }
  }
  
  TextType  eText             = ( uiChromaId > 0 ? TEXT_CHROMA_V : TEXT_CHROMA_U );
  UInt      uiChromaPredMode  = pcCU     ->getChromaIntraDir( uiAbsPartIdx );
  UInt      uiWidth           = pcCU     ->getWidth   ( 0 ) >> ( uiTrDepth + 1 );
  UInt      uiHeight          = pcCU     ->getHeight  ( 0 ) >> ( uiTrDepth + 1 );
  UInt      uiStride          = pcOrgYuv ->getCStride ();
  Pel*      piOrg             = ( uiChromaId > 0 ? pcOrgYuv ->getCrAddr( uiAbsPartIdx ) : pcOrgYuv ->getCbAddr( uiAbsPartIdx ) );
  Pel*      piPred            = ( uiChromaId > 0 ? pcPredYuv->getCrAddr( uiAbsPartIdx ) : pcPredYuv->getCbAddr( uiAbsPartIdx ) );
  Pel*      piResi            = ( uiChromaId > 0 ? pcResiYuv->getCrAddr( uiAbsPartIdx ) : pcResiYuv->getCbAddr( uiAbsPartIdx ) );
  Pel*      piReco            = ( uiChromaId > 0 ? pcPredYuv->getCrAddr( uiAbsPartIdx ) : pcPredYuv->getCbAddr( uiAbsPartIdx ) );
  
  UInt      uiQTLayer         = pcCU->getSlice()->getSPS()->getQuadtreeTULog2MaxSize() - uiLog2TrSize;
  UInt      uiNumCoeffPerInc  = ( pcCU->getSlice()->getSPS()->getMaxCUWidth() * pcCU->getSlice()->getSPS()->getMaxCUHeight() >> ( pcCU->getSlice()->getSPS()->getMaxCUDepth() << 1 ) ) >> 2;
  TCoeff*   pcCoeff           = ( uiChromaId > 0 ? m_ppcQTTempCoeffCr[ uiQTLayer ] : m_ppcQTTempCoeffCb[ uiQTLayer ] ) + uiNumCoeffPerInc * uiAbsPartIdx;
#if ADAPTIVE_QP_SELECTION
  Int*      pcArlCoeff        = ( uiChromaId > 0 ? m_ppcQTTempArlCoeffCr[ uiQTLayer ] : m_ppcQTTempArlCoeffCb[ uiQTLayer ] ) + uiNumCoeffPerInc * uiAbsPartIdx;
#endif
  Pel*      piRecQt           = ( uiChromaId > 0 ? m_pcQTTempTComYuv[ uiQTLayer ].getCrAddr( uiAbsPartIdx ) : m_pcQTTempTComYuv[ uiQTLayer ].getCbAddr( uiAbsPartIdx ) );
  UInt      uiRecQtStride     = m_pcQTTempTComYuv[ uiQTLayer ].getCStride();
  
  UInt      uiZOrder          = pcCU->getZorderIdxInCU() + uiAbsPartIdx;
  Pel*      piRecIPred        = ( uiChromaId > 0 ? pcCU->getPic()->getPicYuvRec()->getCrAddr( pcCU->getAddr(), uiZOrder ) : pcCU->getPic()->getPicYuvRec()->getCbAddr( pcCU->getAddr(), uiZOrder ) );
  UInt      uiRecIPredStride  = pcCU->getPic()->getPicYuvRec()->getCStride();
  
  //===== update chroma mode =====
  if( uiChromaPredMode == DM_CHROMA_IDX )
  {
    uiChromaPredMode          = pcCU->getLumaIntraDir( 0 );
#if HHI_DMM_WEDGE_INTRA || HHI_DMM_PRED_TEX
    mapDMMtoIntraMode( uiChromaPredMode );
#endif
  }
  
  //===== init availability pattern =====
  Bool  bAboveAvail = false;
  Bool  bLeftAvail  = false;
  pcCU->getPattern()->initPattern         ( pcCU, uiTrDepth, uiAbsPartIdx );

  if( uiChromaPredMode == LM_CHROMA_IDX && uiChromaId == 0 )
  {
    pcCU->getPattern()->initAdiPattern( pcCU, uiAbsPartIdx, uiTrDepth, m_piYuvExt, m_iYuvExtStride, m_iYuvExtHeight, bAboveAvail, bLeftAvail, true );
    getLumaRecPixels( pcCU->getPattern(), uiWidth, uiHeight );
  }
  
  pcCU->getPattern()->initAdiPatternChroma( pcCU, uiAbsPartIdx, uiTrDepth, m_piYuvExt, m_iYuvExtStride, m_iYuvExtHeight, bAboveAvail, bLeftAvail );
  Int*  pPatChroma  = ( uiChromaId > 0 ? pcCU->getPattern()->getAdiCrBuf( uiWidth, uiHeight, m_piYuvExt ) : pcCU->getPattern()->getAdiCbBuf( uiWidth, uiHeight, m_piYuvExt ) );
  
  //===== get prediction signal =====
  if( uiChromaPredMode == LM_CHROMA_IDX )
  {
    predLMIntraChroma( pcCU->getPattern(), pPatChroma, piPred, uiStride, uiWidth, uiHeight, uiChromaId );
  }
  else
  {
    predIntraChromaAng( pcCU->getPattern(), pPatChroma, uiChromaPredMode, piPred, uiStride, uiWidth, uiHeight, pcCU, bAboveAvail, bLeftAvail );  
  }
  
  //===== get residual signal =====
  {
    // get residual
    Pel*  pOrg    = piOrg;
    Pel*  pPred   = piPred;
    Pel*  pResi   = piResi;
    for( UInt uiY = 0; uiY < uiHeight; uiY++ )
    {
      for( UInt uiX = 0; uiX < uiWidth; uiX++ )
      {
        pResi[ uiX ] = pOrg[ uiX ] - pPred[ uiX ];
      }
      pOrg  += uiStride;
      pResi += uiStride;
      pPred += uiStride;
    }
  }
  
  //===== transform and quantization =====
  {
    //--- init rate estimation arrays for RDOQ ---
    if( m_pcEncCfg->getUseRDOQ() )
    {
      m_pcEntropyCoder->estimateBit( m_pcTrQuant->m_pcEstBitsSbac, uiWidth, uiWidth, eText );
    }
    //--- transform and quantization ---
    UInt uiAbsSum = 0;

#if H0736_AVC_STYLE_QP_RANGE
    if(eText == TEXT_CHROMA_U)
    {
      m_pcTrQuant->setQPforQuant     ( pcCU->getQP( 0 ), !pcCU->getSlice()->getDepth(), pcCU->getSlice()->getSliceType(), TEXT_CHROMA, pcCU->getSlice()->getSPS()->getQpBDOffsetC(), pcCU->getSlice()->getPPS()->getChromaQpOffset() );
    }
    else
    {
      m_pcTrQuant->setQPforQuant     ( pcCU->getQP( 0 ), !pcCU->getSlice()->getDepth(), pcCU->getSlice()->getSliceType(), TEXT_CHROMA, pcCU->getSlice()->getSPS()->getQpBDOffsetC(), pcCU->getSlice()->getPPS()->getChromaQpOffset2nd() );
    }
#else
    if(eText == TEXT_CHROMA_U)
      m_pcTrQuant->setQPforQuant     ( pcCU->getQP( 0 ), !pcCU->getSlice()->getDepth(), pcCU->getSlice()->getSliceType(), TEXT_CHROMA, pcCU->getSlice()->getPPS()->getChromaQpOffset() );
    else
      m_pcTrQuant->setQPforQuant     ( pcCU->getQP( 0 ), !pcCU->getSlice()->getDepth(), pcCU->getSlice()->getSliceType(), TEXT_CHROMA, pcCU->getSlice()->getPPS()->getChromaQpOffset2nd() );
#endif

#if RDOQ_CHROMA_LAMBDA 
    m_pcTrQuant->selectLambda      (TEXT_CHROMA);  
#endif
    m_pcTrQuant->transformNxN      ( pcCU, piResi, uiStride, pcCoeff, 
#if ADAPTIVE_QP_SELECTION
                                     pcArlCoeff, 
#endif
                                     uiWidth, uiHeight, uiAbsSum, eText, uiAbsPartIdx );
    //--- set coded block flag ---
    pcCU->setCbfSubParts           ( ( uiAbsSum ? 1 : 0 ) << uiOrgTrDepth, eText, uiAbsPartIdx, pcCU->getDepth(0) + uiTrDepth );
    //--- inverse transform ---
    if( uiAbsSum )
    {
      Int scalingListType = 0 + g_eTTable[(Int)eText];
      assert(scalingListType < 6);
#if LOSSLESS_CODING
      m_pcTrQuant->invtransformNxN( pcCU, TEXT_CHROMA, REG_DCT, piResi, uiStride, pcCoeff, uiWidth, uiHeight, scalingListType );
#else
      m_pcTrQuant->invtransformNxN( TEXT_CHROMA, REG_DCT, piResi, uiStride, pcCoeff, uiWidth, uiHeight, scalingListType );
#endif
    }
    else
    {
      Pel* pResi = piResi;
      memset( pcCoeff, 0, sizeof( TCoeff ) * uiWidth * uiHeight );
      for( UInt uiY = 0; uiY < uiHeight; uiY++ )
      {
        memset( pResi, 0, sizeof( Pel ) * uiWidth );
        pResi += uiStride;
      }
    }
  }
  
  //===== reconstruction =====
  {
    Pel* pPred      = piPred;
    Pel* pResi      = piResi;
    Pel* pReco      = piReco;
    Pel* pRecQt     = piRecQt;
    Pel* pRecIPred  = piRecIPred;
    for( UInt uiY = 0; uiY < uiHeight; uiY++ )
    {
      for( UInt uiX = 0; uiX < uiWidth; uiX++ )
      {
        pReco    [ uiX ] = Clip( pPred[ uiX ] + pResi[ uiX ] );
        pRecQt   [ uiX ] = pReco[ uiX ];
        pRecIPred[ uiX ] = pReco[ uiX ];
      }
      pPred     += uiStride;
      pResi     += uiStride;
      pReco     += uiStride;
      pRecQt    += uiRecQtStride;
      pRecIPred += uiRecIPredStride;
    }
  }
  
  //===== update distortion =====
#if WEIGHTED_CHROMA_DISTORTION
  ruiDist += m_pcRdCost->getDistPart( piReco, uiStride, piOrg, uiStride, uiWidth, uiHeight, true );
#else
  ruiDist += m_pcRdCost->getDistPart( piReco, uiStride, piOrg, uiStride, uiWidth, uiHeight );
#endif
}



Void 
TEncSearch::xRecurIntraCodingQT( TComDataCU*  pcCU, 
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
                                Double&      dRDCost )
{
  UInt    uiFullDepth   = pcCU->getDepth( 0 ) +  uiTrDepth;
  UInt    uiLog2TrSize  = g_aucConvertToBit[ pcCU->getSlice()->getSPS()->getMaxCUWidth() >> uiFullDepth ] + 2;
  Bool    bCheckFull    = ( uiLog2TrSize  <= pcCU->getSlice()->getSPS()->getQuadtreeTULog2MaxSize() );
  Bool    bCheckSplit   = ( uiLog2TrSize  >  pcCU->getQuadtreeTULog2MinSizeInCU(uiAbsPartIdx) );
  
#if HHI_RQT_INTRA_SPEEDUP
  if( bCheckFirst && bCheckFull )
  {
    bCheckSplit = false;
  }
#endif
#if HHI_DMM_WEDGE_INTRA || HHI_DMM_PRED_TEX
  if( pcCU->getLumaIntraDir( uiAbsPartIdx ) >= NUM_INTRA_MODE )
  {
    bCheckSplit = false;
  }
#endif
  Double  dSingleCost   = MAX_DOUBLE;
  Dist    uiSingleDistY = 0;
  Dist    uiSingleDistC = 0;
  UInt    uiSingleCbfY  = 0;
  UInt    uiSingleCbfU  = 0;
  UInt    uiSingleCbfV  = 0;
  
  if( bCheckFull )
  {
    //----- store original entropy coding status -----
    if( m_bUseSBACRD && bCheckSplit )
    {
      m_pcRDGoOnSbacCoder->store( m_pppcRDSbacCoder[ uiFullDepth ][ CI_QT_TRAFO_ROOT ] );
    }
    //----- code luma block with given intra prediction mode and store Cbf-----
    dSingleCost   = 0.0;
    xIntraCodingLumaBlk( pcCU, uiTrDepth, uiAbsPartIdx, pcOrgYuv, pcPredYuv, pcResiYuv, uiSingleDistY ); 
    if( bCheckSplit )
    {
      uiSingleCbfY = pcCU->getCbf( uiAbsPartIdx, TEXT_LUMA, uiTrDepth );
    }
    //----- code chroma blocks with given intra prediction mode and store Cbf-----
    if( !bLumaOnly )
    {
      xIntraCodingChromaBlk ( pcCU, uiTrDepth, uiAbsPartIdx, pcOrgYuv, pcPredYuv, pcResiYuv, uiSingleDistC, 0 ); 
      xIntraCodingChromaBlk ( pcCU, uiTrDepth, uiAbsPartIdx, pcOrgYuv, pcPredYuv, pcResiYuv, uiSingleDistC, 1 ); 
      if( bCheckSplit )
      {
        uiSingleCbfU = pcCU->getCbf( uiAbsPartIdx, TEXT_CHROMA_U, uiTrDepth );
        uiSingleCbfV = pcCU->getCbf( uiAbsPartIdx, TEXT_CHROMA_V, uiTrDepth );
      }
    }
    //----- determine rate and r-d cost -----
    UInt uiSingleBits = xGetIntraBitsQT( pcCU, uiTrDepth, uiAbsPartIdx, true, !bLumaOnly, false );

#if HHI_VSO
    if ( m_pcRdCost->getUseLambdaScaleVSO())
    {
      dSingleCost = m_pcRdCost->calcRdCostVSO( uiSingleBits, uiSingleDistY + uiSingleDistC );
    }
    else
#endif
    {
    dSingleCost       = m_pcRdCost->calcRdCost( uiSingleBits, uiSingleDistY + uiSingleDistC );
  }
  }
  
  if( bCheckSplit )
  {
    //----- store full entropy coding status, load original entropy coding status -----
    if( m_bUseSBACRD )
    {
      if( bCheckFull )
      {
        m_pcRDGoOnSbacCoder->store( m_pppcRDSbacCoder[ uiFullDepth ][ CI_QT_TRAFO_TEST ] );
        m_pcRDGoOnSbacCoder->load ( m_pppcRDSbacCoder[ uiFullDepth ][ CI_QT_TRAFO_ROOT ] );
      }
      else
      {
        m_pcRDGoOnSbacCoder->store( m_pppcRDSbacCoder[ uiFullDepth ][ CI_QT_TRAFO_ROOT ] );
      }
    }
    //----- code splitted block -----
    Double  dSplitCost      = 0.0;
    Dist    uiSplitDistY    = 0;
    Dist    uiSplitDistC    = 0;
    UInt    uiQPartsDiv     = pcCU->getPic()->getNumPartInCU() >> ( ( uiFullDepth + 1 ) << 1 );
    UInt    uiAbsPartIdxSub = uiAbsPartIdx;

    UInt    uiSplitCbfY = 0;
    UInt    uiSplitCbfU = 0;
    UInt    uiSplitCbfV = 0;

    for( UInt uiPart = 0; uiPart < 4; uiPart++, uiAbsPartIdxSub += uiQPartsDiv )
    {
#if HHI_RQT_INTRA_SPEEDUP
      xRecurIntraCodingQT( pcCU, uiTrDepth + 1, uiAbsPartIdxSub, bLumaOnly, pcOrgYuv, pcPredYuv, pcResiYuv, uiSplitDistY, uiSplitDistC, bCheckFirst, dSplitCost );
#else
      xRecurIntraCodingQT( pcCU, uiTrDepth + 1, uiAbsPartIdxSub, bLumaOnly, pcOrgYuv, pcPredYuv, pcResiYuv, uiSplitDistY, uiSplitDistC, dSplitCost );
#endif

      uiSplitCbfY |= pcCU->getCbf( uiAbsPartIdxSub, TEXT_LUMA, uiTrDepth + 1 );
      if(!bLumaOnly)
      {
        uiSplitCbfU |= pcCU->getCbf( uiAbsPartIdxSub, TEXT_CHROMA_U, uiTrDepth + 1 );
        uiSplitCbfV |= pcCU->getCbf( uiAbsPartIdxSub, TEXT_CHROMA_V, uiTrDepth + 1 );
      }
    }

    for( UInt uiOffs = 0; uiOffs < 4 * uiQPartsDiv; uiOffs++ )
    {
      pcCU->getCbf( TEXT_LUMA )[ uiAbsPartIdx + uiOffs ] |= ( uiSplitCbfY << uiTrDepth );
    }
    if( !bLumaOnly )
    {
      for( UInt uiOffs = 0; uiOffs < 4 * uiQPartsDiv; uiOffs++ )
      {
        pcCU->getCbf( TEXT_CHROMA_U )[ uiAbsPartIdx + uiOffs ] |= ( uiSplitCbfU << uiTrDepth );
        pcCU->getCbf( TEXT_CHROMA_V )[ uiAbsPartIdx + uiOffs ] |= ( uiSplitCbfV << uiTrDepth );
      }
    }
    //----- restore context states -----
    if( m_bUseSBACRD )
    {
      m_pcRDGoOnSbacCoder->load ( m_pppcRDSbacCoder[ uiFullDepth ][ CI_QT_TRAFO_ROOT ] );
    }
    //----- determine rate and r-d cost -----
    UInt uiSplitBits = xGetIntraBitsQT( pcCU, uiTrDepth, uiAbsPartIdx, true, !bLumaOnly, false );
#if HHI_VSO
    if( m_pcRdCost->getUseLambdaScaleVSO() )
    {
      dSplitCost = m_pcRdCost->calcRdCostVSO( uiSplitBits, uiSplitDistY + uiSplitDistC );
    }
    else
#endif
    {
    dSplitCost       = m_pcRdCost->calcRdCost( uiSplitBits, uiSplitDistY + uiSplitDistC );
    }
    
    //===== compare and set best =====
    if( dSplitCost < dSingleCost )
    {
      //--- update cost ---
      ruiDistY += uiSplitDistY;
      ruiDistC += uiSplitDistC;
      dRDCost  += dSplitCost;
      return;
    }
    //----- set entropy coding status -----
    if( m_bUseSBACRD )
    {
      m_pcRDGoOnSbacCoder->load ( m_pppcRDSbacCoder[ uiFullDepth ][ CI_QT_TRAFO_TEST ] );
    }
    
    //--- set transform index and Cbf values ---
    pcCU->setTrIdxSubParts( uiTrDepth, uiAbsPartIdx, uiFullDepth );
    pcCU->setCbfSubParts  ( uiSingleCbfY << uiTrDepth, TEXT_LUMA, uiAbsPartIdx, uiFullDepth );
    if( !bLumaOnly )
    {
      pcCU->setCbfSubParts( uiSingleCbfU << uiTrDepth, TEXT_CHROMA_U, uiAbsPartIdx, uiFullDepth );
      pcCU->setCbfSubParts( uiSingleCbfV << uiTrDepth, TEXT_CHROMA_V, uiAbsPartIdx, uiFullDepth );
    }
    
    //--- set reconstruction for next intra prediction blocks ---
    UInt  uiWidth     = pcCU->getWidth ( 0 ) >> uiTrDepth;
    UInt  uiHeight    = pcCU->getHeight( 0 ) >> uiTrDepth;
    UInt  uiQTLayer   = pcCU->getSlice()->getSPS()->getQuadtreeTULog2MaxSize() - uiLog2TrSize;
    UInt  uiZOrder    = pcCU->getZorderIdxInCU() + uiAbsPartIdx;
    Pel*  piSrc       = m_pcQTTempTComYuv[ uiQTLayer ].getLumaAddr( uiAbsPartIdx );
    UInt  uiSrcStride = m_pcQTTempTComYuv[ uiQTLayer ].getStride  ();
    Pel*  piDes       = pcCU->getPic()->getPicYuvRec()->getLumaAddr( pcCU->getAddr(), uiZOrder );
    UInt  uiDesStride = pcCU->getPic()->getPicYuvRec()->getStride  ();
    for( UInt uiY = 0; uiY < uiHeight; uiY++, piSrc += uiSrcStride, piDes += uiDesStride )
    {
      for( UInt uiX = 0; uiX < uiWidth; uiX++ )
      {
        piDes[ uiX ] = piSrc[ uiX ];
      }
    }
    if( !bLumaOnly )
    {
      uiWidth   >>= 1;
      uiHeight  >>= 1;
      piSrc       = m_pcQTTempTComYuv[ uiQTLayer ].getCbAddr  ( uiAbsPartIdx );
      uiSrcStride = m_pcQTTempTComYuv[ uiQTLayer ].getCStride ();
      piDes       = pcCU->getPic()->getPicYuvRec()->getCbAddr ( pcCU->getAddr(), uiZOrder );
      uiDesStride = pcCU->getPic()->getPicYuvRec()->getCStride();
      for( UInt uiY = 0; uiY < uiHeight; uiY++, piSrc += uiSrcStride, piDes += uiDesStride )
      {
        for( UInt uiX = 0; uiX < uiWidth; uiX++ )
        {
          piDes[ uiX ] = piSrc[ uiX ];
        }
      }
      piSrc       = m_pcQTTempTComYuv[ uiQTLayer ].getCrAddr  ( uiAbsPartIdx );
      piDes       = pcCU->getPic()->getPicYuvRec()->getCrAddr ( pcCU->getAddr(), uiZOrder );
      for( UInt uiY = 0; uiY < uiHeight; uiY++, piSrc += uiSrcStride, piDes += uiDesStride )
      {
        for( UInt uiX = 0; uiX < uiWidth; uiX++ )
        {
          piDes[ uiX ] = piSrc[ uiX ];
        }
      }
    }
  }

#if HHI_VSO
  if ( m_pcRdCost->getUseRenModel() && bCheckFull )
  {
    UInt  uiWidth     = pcCU->getWidth ( 0 ) >> uiTrDepth;
    UInt  uiHeight    = pcCU->getHeight( 0 ) >> uiTrDepth;
    UInt  uiQTLayer   = pcCU->getSlice()->getSPS()->getQuadtreeTULog2MaxSize() - uiLog2TrSize;
    Pel*  piSrc       = m_pcQTTempTComYuv[ uiQTLayer ].getLumaAddr( uiAbsPartIdx );
    UInt  uiSrcStride = m_pcQTTempTComYuv[ uiQTLayer ].getStride  ();

    m_pcRdCost->setRenModelData( pcCU, uiAbsPartIdx, piSrc, (Int) uiSrcStride, (Int) uiWidth, (Int) uiHeight );
  }
#endif

  ruiDistY += uiSingleDistY;
  ruiDistC += uiSingleDistC;
  dRDCost  += dSingleCost;
}


Void
TEncSearch::xSetIntraResultQT( TComDataCU* pcCU,
                              UInt        uiTrDepth,
                              UInt        uiAbsPartIdx,
                              Bool        bLumaOnly,
                              TComYuv*    pcRecoYuv )
{
  UInt uiFullDepth  = pcCU->getDepth(0) + uiTrDepth;
  UInt uiTrMode     = pcCU->getTransformIdx( uiAbsPartIdx );
  if(  uiTrMode == uiTrDepth )
  {
    UInt uiLog2TrSize = g_aucConvertToBit[ pcCU->getSlice()->getSPS()->getMaxCUWidth() >> uiFullDepth ] + 2;
    UInt uiQTLayer    = pcCU->getSlice()->getSPS()->getQuadtreeTULog2MaxSize() - uiLog2TrSize;
    
    Bool bSkipChroma  = false;
    Bool bChromaSame  = false;
    if( !bLumaOnly && uiLog2TrSize == 2 )
    {
      assert( uiTrDepth > 0 );
      UInt uiQPDiv = pcCU->getPic()->getNumPartInCU() >> ( ( pcCU->getDepth( 0 ) + uiTrDepth - 1 ) << 1 );
      bSkipChroma  = ( ( uiAbsPartIdx % uiQPDiv ) != 0 );
      bChromaSame  = true;
    }
    
    //===== copy transform coefficients =====
    UInt uiNumCoeffY    = ( pcCU->getSlice()->getSPS()->getMaxCUWidth() * pcCU->getSlice()->getSPS()->getMaxCUHeight() ) >> ( uiFullDepth << 1 );
    UInt uiNumCoeffIncY = ( pcCU->getSlice()->getSPS()->getMaxCUWidth() * pcCU->getSlice()->getSPS()->getMaxCUHeight() ) >> ( pcCU->getSlice()->getSPS()->getMaxCUDepth() << 1 );
    TCoeff* pcCoeffSrcY = m_ppcQTTempCoeffY [ uiQTLayer ] + ( uiNumCoeffIncY * uiAbsPartIdx );
    TCoeff* pcCoeffDstY = pcCU->getCoeffY ()              + ( uiNumCoeffIncY * uiAbsPartIdx );
    ::memcpy( pcCoeffDstY, pcCoeffSrcY, sizeof( TCoeff ) * uiNumCoeffY );
#if ADAPTIVE_QP_SELECTION
    Int* pcArlCoeffSrcY = m_ppcQTTempArlCoeffY [ uiQTLayer ] + ( uiNumCoeffIncY * uiAbsPartIdx );
    Int* pcArlCoeffDstY = pcCU->getArlCoeffY ()              + ( uiNumCoeffIncY * uiAbsPartIdx );
    ::memcpy( pcArlCoeffDstY, pcArlCoeffSrcY, sizeof( Int ) * uiNumCoeffY );
#endif
    if( !bLumaOnly && !bSkipChroma )
    {
      UInt uiNumCoeffC    = ( bChromaSame ? uiNumCoeffY    : uiNumCoeffY    >> 2 );
      UInt uiNumCoeffIncC = uiNumCoeffIncY >> 2;
      TCoeff* pcCoeffSrcU = m_ppcQTTempCoeffCb[ uiQTLayer ] + ( uiNumCoeffIncC * uiAbsPartIdx );
      TCoeff* pcCoeffSrcV = m_ppcQTTempCoeffCr[ uiQTLayer ] + ( uiNumCoeffIncC * uiAbsPartIdx );
      TCoeff* pcCoeffDstU = pcCU->getCoeffCb()              + ( uiNumCoeffIncC * uiAbsPartIdx );
      TCoeff* pcCoeffDstV = pcCU->getCoeffCr()              + ( uiNumCoeffIncC * uiAbsPartIdx );
      ::memcpy( pcCoeffDstU, pcCoeffSrcU, sizeof( TCoeff ) * uiNumCoeffC );
      ::memcpy( pcCoeffDstV, pcCoeffSrcV, sizeof( TCoeff ) * uiNumCoeffC );
#if ADAPTIVE_QP_SELECTION
      Int* pcArlCoeffSrcU = m_ppcQTTempArlCoeffCb[ uiQTLayer ] + ( uiNumCoeffIncC * uiAbsPartIdx );
      Int* pcArlCoeffSrcV = m_ppcQTTempArlCoeffCr[ uiQTLayer ] + ( uiNumCoeffIncC * uiAbsPartIdx );
      Int* pcArlCoeffDstU = pcCU->getArlCoeffCb()              + ( uiNumCoeffIncC * uiAbsPartIdx );
      Int* pcArlCoeffDstV = pcCU->getArlCoeffCr()              + ( uiNumCoeffIncC * uiAbsPartIdx );
      ::memcpy( pcArlCoeffDstU, pcArlCoeffSrcU, sizeof( Int ) * uiNumCoeffC );
      ::memcpy( pcArlCoeffDstV, pcArlCoeffSrcV, sizeof( Int ) * uiNumCoeffC );
#endif
    }
    
    //===== copy reconstruction =====
    m_pcQTTempTComYuv[ uiQTLayer ].copyPartToPartLuma( pcRecoYuv, uiAbsPartIdx, 1 << uiLog2TrSize, 1 << uiLog2TrSize );
    if( !bLumaOnly && !bSkipChroma )
    {
      UInt uiLog2TrSizeChroma = ( bChromaSame ? uiLog2TrSize : uiLog2TrSize - 1 );
      m_pcQTTempTComYuv[ uiQTLayer ].copyPartToPartChroma( pcRecoYuv, uiAbsPartIdx, 1 << uiLog2TrSizeChroma, 1 << uiLog2TrSizeChroma );
    }
  }
  else
  {
    UInt uiNumQPart  = pcCU->getPic()->getNumPartInCU() >> ( ( uiFullDepth + 1 ) << 1 );
    for( UInt uiPart = 0; uiPart < 4; uiPart++ )
    {
      xSetIntraResultQT( pcCU, uiTrDepth + 1, uiAbsPartIdx + uiPart * uiNumQPart, bLumaOnly, pcRecoYuv );
    }
  }
}



Void 
TEncSearch::xRecurIntraChromaCodingQT( TComDataCU*  pcCU, 
                                      UInt         uiTrDepth,
                                      UInt         uiAbsPartIdx, 
                                      TComYuv*     pcOrgYuv, 
                                      TComYuv*     pcPredYuv, 
                                      TComYuv*     pcResiYuv, 
                                      Dist&        ruiDist )
{
  UInt uiFullDepth = pcCU->getDepth( 0 ) +  uiTrDepth;
  UInt uiTrMode    = pcCU->getTransformIdx( uiAbsPartIdx );
  if(  uiTrMode == uiTrDepth )
  {
    xIntraCodingChromaBlk( pcCU, uiTrDepth, uiAbsPartIdx, pcOrgYuv, pcPredYuv, pcResiYuv, ruiDist, 0 ); 
    xIntraCodingChromaBlk( pcCU, uiTrDepth, uiAbsPartIdx, pcOrgYuv, pcPredYuv, pcResiYuv, ruiDist, 1 ); 
  }
  else
  {
    UInt uiSplitCbfU     = 0;
    UInt uiSplitCbfV     = 0;
    UInt uiQPartsDiv     = pcCU->getPic()->getNumPartInCU() >> ( ( uiFullDepth + 1 ) << 1 );
    UInt uiAbsPartIdxSub = uiAbsPartIdx;
    for( UInt uiPart = 0; uiPart < 4; uiPart++, uiAbsPartIdxSub += uiQPartsDiv )
    {
      xRecurIntraChromaCodingQT( pcCU, uiTrDepth + 1, uiAbsPartIdxSub, pcOrgYuv, pcPredYuv, pcResiYuv, ruiDist );
      uiSplitCbfU |= pcCU->getCbf( uiAbsPartIdxSub, TEXT_CHROMA_U, uiTrDepth + 1 );
      uiSplitCbfV |= pcCU->getCbf( uiAbsPartIdxSub, TEXT_CHROMA_V, uiTrDepth + 1 );
    }
    for( UInt uiOffs = 0; uiOffs < 4 * uiQPartsDiv; uiOffs++ )
    {
      pcCU->getCbf( TEXT_CHROMA_U )[ uiAbsPartIdx + uiOffs ] |= ( uiSplitCbfU << uiTrDepth );
      pcCU->getCbf( TEXT_CHROMA_V )[ uiAbsPartIdx + uiOffs ] |= ( uiSplitCbfV << uiTrDepth );
    }
  }
}

Void
TEncSearch::xSetIntraResultChromaQT( TComDataCU* pcCU,
                                    UInt        uiTrDepth,
                                    UInt        uiAbsPartIdx,
                                    TComYuv*    pcRecoYuv )
{
  UInt uiFullDepth  = pcCU->getDepth(0) + uiTrDepth;
  UInt uiTrMode     = pcCU->getTransformIdx( uiAbsPartIdx );
  if(  uiTrMode == uiTrDepth )
  {
    UInt uiLog2TrSize = g_aucConvertToBit[ pcCU->getSlice()->getSPS()->getMaxCUWidth() >> uiFullDepth ] + 2;
    UInt uiQTLayer    = pcCU->getSlice()->getSPS()->getQuadtreeTULog2MaxSize() - uiLog2TrSize;
    
    Bool bChromaSame  = false;
    if( uiLog2TrSize == 2 )
    {
      assert( uiTrDepth > 0 );
      UInt uiQPDiv = pcCU->getPic()->getNumPartInCU() >> ( ( pcCU->getDepth( 0 ) + uiTrDepth - 1 ) << 1 );
      if( ( uiAbsPartIdx % uiQPDiv ) != 0 )
      {
        return;
      }
      bChromaSame     = true;
    }
    
    //===== copy transform coefficients =====
    UInt uiNumCoeffC    = ( pcCU->getSlice()->getSPS()->getMaxCUWidth() * pcCU->getSlice()->getSPS()->getMaxCUHeight() ) >> ( uiFullDepth << 1 );
    if( !bChromaSame )
    {
      uiNumCoeffC     >>= 2;
    }
    UInt uiNumCoeffIncC = ( pcCU->getSlice()->getSPS()->getMaxCUWidth() * pcCU->getSlice()->getSPS()->getMaxCUHeight() ) >> ( ( pcCU->getSlice()->getSPS()->getMaxCUDepth() << 1 ) + 2 );
    TCoeff* pcCoeffSrcU = m_ppcQTTempCoeffCb[ uiQTLayer ] + ( uiNumCoeffIncC * uiAbsPartIdx );
    TCoeff* pcCoeffSrcV = m_ppcQTTempCoeffCr[ uiQTLayer ] + ( uiNumCoeffIncC * uiAbsPartIdx );
    TCoeff* pcCoeffDstU = pcCU->getCoeffCb()              + ( uiNumCoeffIncC * uiAbsPartIdx );
    TCoeff* pcCoeffDstV = pcCU->getCoeffCr()              + ( uiNumCoeffIncC * uiAbsPartIdx );
    ::memcpy( pcCoeffDstU, pcCoeffSrcU, sizeof( TCoeff ) * uiNumCoeffC );
    ::memcpy( pcCoeffDstV, pcCoeffSrcV, sizeof( TCoeff ) * uiNumCoeffC );
#if ADAPTIVE_QP_SELECTION    
    Int* pcArlCoeffSrcU = m_ppcQTTempArlCoeffCb[ uiQTLayer ] + ( uiNumCoeffIncC * uiAbsPartIdx );
    Int* pcArlCoeffSrcV = m_ppcQTTempArlCoeffCr[ uiQTLayer ] + ( uiNumCoeffIncC * uiAbsPartIdx );
    Int* pcArlCoeffDstU = pcCU->getArlCoeffCb()              + ( uiNumCoeffIncC * uiAbsPartIdx );
    Int* pcArlCoeffDstV = pcCU->getArlCoeffCr()              + ( uiNumCoeffIncC * uiAbsPartIdx );
    ::memcpy( pcArlCoeffDstU, pcArlCoeffSrcU, sizeof( Int ) * uiNumCoeffC );
    ::memcpy( pcArlCoeffDstV, pcArlCoeffSrcV, sizeof( Int ) * uiNumCoeffC );
#endif
    
    //===== copy reconstruction =====
    UInt uiLog2TrSizeChroma = ( bChromaSame ? uiLog2TrSize : uiLog2TrSize - 1 );
    m_pcQTTempTComYuv[ uiQTLayer ].copyPartToPartChroma( pcRecoYuv, uiAbsPartIdx, 1 << uiLog2TrSizeChroma, 1 << uiLog2TrSizeChroma );
  }
  else
  {
    UInt uiNumQPart  = pcCU->getPic()->getNumPartInCU() >> ( ( uiFullDepth + 1 ) << 1 );
    for( UInt uiPart = 0; uiPart < 4; uiPart++ )
    {
      xSetIntraResultChromaQT( pcCU, uiTrDepth + 1, uiAbsPartIdx + uiPart * uiNumQPart, pcRecoYuv );
    }
  }
}


Void 
TEncSearch::preestChromaPredMode( TComDataCU* pcCU, 
                                 TComYuv*    pcOrgYuv, 
                                 TComYuv*    pcPredYuv )
{
  UInt  uiWidth     = pcCU->getWidth ( 0 ) >> 1;
  UInt  uiHeight    = pcCU->getHeight( 0 ) >> 1;
  UInt  uiStride    = pcOrgYuv ->getCStride();
  Pel*  piOrgU      = pcOrgYuv ->getCbAddr ( 0 );
  Pel*  piOrgV      = pcOrgYuv ->getCrAddr ( 0 );
  Pel*  piPredU     = pcPredYuv->getCbAddr ( 0 );
  Pel*  piPredV     = pcPredYuv->getCrAddr ( 0 );
  
  //===== init pattern =====
  Bool  bAboveAvail = false;
  Bool  bLeftAvail  = false;
  pcCU->getPattern()->initPattern         ( pcCU, 0, 0 );
  pcCU->getPattern()->initAdiPatternChroma( pcCU, 0, 0, m_piYuvExt, m_iYuvExtStride, m_iYuvExtHeight, bAboveAvail, bLeftAvail );
  Int*  pPatChromaU = pcCU->getPattern()->getAdiCbBuf( uiWidth, uiHeight, m_piYuvExt );
  Int*  pPatChromaV = pcCU->getPattern()->getAdiCrBuf( uiWidth, uiHeight, m_piYuvExt );
  
  //===== get best prediction modes (using SAD) =====
  UInt  uiMinMode   = 0;
  UInt  uiMaxMode   = 4;
  UInt  uiBestMode  = MAX_UINT;
  UInt  uiMinSAD    = MAX_UINT;
  for( UInt uiMode  = uiMinMode; uiMode < uiMaxMode; uiMode++ )
  {
    //--- get prediction ---
    predIntraChromaAng( pcCU->getPattern(), pPatChromaU, uiMode, piPredU, uiStride, uiWidth, uiHeight, pcCU, bAboveAvail, bLeftAvail );
    predIntraChromaAng( pcCU->getPattern(), pPatChromaV, uiMode, piPredV, uiStride, uiWidth, uiHeight, pcCU, bAboveAvail, bLeftAvail );
    
    //--- get SAD ---
    UInt  uiSAD  = m_pcRdCost->calcHAD( piOrgU, uiStride, piPredU, uiStride, uiWidth, uiHeight );
    uiSAD       += m_pcRdCost->calcHAD( piOrgV, uiStride, piPredV, uiStride, uiWidth, uiHeight );
    //--- check ---
    if( uiSAD < uiMinSAD )
    {
      uiMinSAD   = uiSAD;
      uiBestMode = uiMode;
    }
  }
  
  //===== set chroma pred mode =====
  pcCU->setChromIntraDirSubParts( uiBestMode, 0, pcCU->getDepth( 0 ) );
}

Void 
TEncSearch::estIntraPredQT( TComDataCU* pcCU, 
                           TComYuv*    pcOrgYuv, 
                           TComYuv*    pcPredYuv, 
                           TComYuv*    pcResiYuv, 
                           TComYuv*    pcRecoYuv,
                           Dist&       ruiDistC,
                           Bool        bLumaOnly )
{
  UInt    uiDepth        = pcCU->getDepth(0);
  UInt    uiNumPU        = pcCU->getNumPartInter();
  UInt    uiInitTrDepth  = pcCU->getPartitionSize(0) == SIZE_2Nx2N ? 0 : 1;
  UInt    uiWidth        = pcCU->getWidth (0) >> uiInitTrDepth;
  UInt    uiHeight       = pcCU->getHeight(0) >> uiInitTrDepth;
  UInt    uiQNumParts    = pcCU->getTotalNumPart() >> 2;
  UInt    uiWidthBit     = pcCU->getIntraSizeIdx(0);
  UInt    uiOverallDistY = 0;
  UInt    uiOverallDistC = 0;
  UInt    CandNum;
  Double  CandCostList[ FAST_UDI_MAX_RDMODE_NUM ];
  
  //===== set QP and clear Cbf =====
  if ( pcCU->getSlice()->getPPS()->getUseDQP() == true)
  {
    pcCU->setQPSubParts( pcCU->getQP(0), 0, uiDepth );
  }
  else
  {
    pcCU->setQPSubParts( pcCU->getSlice()->getSliceQp(), 0, uiDepth );
  }
  
  //===== loop over partitions =====
  UInt uiPartOffset = 0;
  for( UInt uiPU = 0; uiPU < uiNumPU; uiPU++, uiPartOffset += uiQNumParts )
  {
    //===== init pattern for luma prediction =====
    Bool bAboveAvail = false;
    Bool bLeftAvail  = false;
    pcCU->getPattern()->initPattern   ( pcCU, uiInitTrDepth, uiPartOffset );
    pcCU->getPattern()->initAdiPattern( pcCU, uiPartOffset, uiInitTrDepth, m_piYuvExt, m_iYuvExtStride, m_iYuvExtHeight, bAboveAvail, bLeftAvail );
    
    //===== determine set of modes to be tested (using prediction signal only) =====
#if LOGI_INTRA_NAME_3MPM
    Int numModesAvailable     = 35; //total number of Intra modes
#else
    Int numModesAvailable     = g_aucIntraModeNumAng[uiWidthBit];
#endif
    Pel* piOrg         = pcOrgYuv ->getLumaAddr( uiPU, uiWidth );
    Pel* piPred        = pcPredYuv->getLumaAddr( uiPU, uiWidth );
    UInt uiStride      = pcPredYuv->getStride();
    UInt uiRdModeList[FAST_UDI_MAX_RDMODE_NUM];
    Int numModesForFullRD = g_aucIntraModeNumFast[ uiWidthBit ];
    
#if HHI_DMM_WEDGE_INTRA || HHI_DMM_PRED_TEX
    Bool bTestDmm = ( m_pcEncCfg->getUseDMM() );
#endif

    Bool doFastSearch = (numModesForFullRD != numModesAvailable);
    if (doFastSearch)
    {
      assert(numModesForFullRD < numModesAvailable);

      for( Int i=0; i < numModesForFullRD; i++ ) 
      {
        CandCostList[ i ] = MAX_DOUBLE;
      }
      CandNum = 0;
      
      for( Int modeIdx = 0; modeIdx < numModesAvailable; modeIdx++ )
      {
        UInt uiMode = modeIdx;

        predIntraLumaAng( pcCU->getPattern(), uiMode, piPred, uiStride, uiWidth, uiHeight, pcCU, bAboveAvail, bLeftAvail );
        
        // use hadamard transform here
      Dist uiSad;
#if HHI_VSO
      if ( m_pcRdCost->getUseVSO() )
      {
        Bool bSad = !m_pcRdCost->getUseRenModel();
        uiSad = m_pcRdCost->getDistVS(pcCU, uiPartOffset, piPred, uiStride, piOrg, uiStride, uiWidth, uiHeight, bSad, 0 );
      }
      else
#endif
      {
        uiSad = (Dist) m_pcRdCost->calcHAD( piOrg, uiStride, piPred, uiStride, uiWidth, uiHeight );
      }
        
        UInt   iModeBits = xModeBitsIntra( pcCU, uiMode, uiPU, uiPartOffset, uiDepth, uiInitTrDepth );

      Double dLambda;
#if HHI_VSO
      if ( m_pcRdCost->getUseLambdaScaleVSO() )
      {
        dLambda = m_pcRdCost->getUseRenModel() ? m_pcRdCost->getLambdaVSO() : m_pcRdCost->getSqrtLambdaVSO();
        //GT: Sad is SSE for VSO4
      }
      else
      {
        dLambda = m_pcRdCost->getSqrtLambda();
      }
#else
      dLambda = m_pcRdCost->getSqrtLambda();
#endif

      Double cost = (Double)uiSad + (Double)iModeBits *  dLambda;
        
        CandNum += xUpdateCandList( uiMode, cost, numModesForFullRD, uiRdModeList, CandCostList );

#if HHI_DMM_WEDGE_INTRA || HHI_DMM_PRED_TEX
        if( bTestDmm ) bTestDmm = uiSad ? true : false;
#endif
      }
    
#if FAST_UDI_USE_MPM
#if LOGI_INTRA_NAME_3MPM
      Int uiPreds[3] = {-1, -1, -1};
#else
      Int uiPreds[2] = {-1, -1};
#endif
      Int iMode = -1;
      Int numCand = pcCU->getIntraDirLumaPredictor( uiPartOffset, uiPreds, &iMode );
#if LOGI_INTRA_NAME_3MPM
      if( iMode >= 0 )
      {
        numCand = iMode;
      }
#else
      if( iMode >= 0 )
      {
        numCand = 1;
        uiPreds[0] = iMode;
      }
#endif
      
      for( Int j=0; j < numCand; j++)

      {
        Bool mostProbableModeIncluded = false;
        Int mostProbableMode = uiPreds[j];
        
        for( Int i=0; i < numModesForFullRD; i++)
        {
          mostProbableModeIncluded |= (mostProbableMode == uiRdModeList[i]);
        }
        if (!mostProbableModeIncluded)
        {
          uiRdModeList[numModesForFullRD++] = mostProbableMode;
        }
      }
#endif // FAST_UDI_USE_MPM
    }
    else
    {
      for( Int i=0; i < numModesForFullRD; i++)
      {
        uiRdModeList[i] = i;
      }
    }
    
#if HHI_DMM_WEDGE_INTRA || HHI_DMM_PRED_TEX
    if( m_pcEncCfg->getUseDMM() && bTestDmm && uiWidth >= DMM_WEDGEMODEL_MIN_SIZE && uiWidth <= DMM_WEDGEMODEL_MAX_SIZE && uiWidth == uiHeight )
    {
#if HHI_DMM_WEDGE_INTRA
      UInt uiTabIdx  = 0;
      Int  iDeltaDC1 = 0;
      Int  iDeltaDC2 = 0;
      findWedgeFullMinDist( pcCU, uiPartOffset, piOrg, piPred, uiStride, uiWidth, uiHeight, uiTabIdx, iDeltaDC1, iDeltaDC2, bAboveAvail, bLeftAvail );
      pcCU->setWedgeFullTabIdxSubParts  ( uiTabIdx,  uiPartOffset, uiDepth + uiInitTrDepth );
      pcCU->setWedgeFullDeltaDC1SubParts( iDeltaDC1, uiPartOffset, uiDepth + uiInitTrDepth );
      pcCU->setWedgeFullDeltaDC2SubParts( iDeltaDC2, uiPartOffset, uiDepth + uiInitTrDepth );

      uiRdModeList[ numModesForFullRD++ ] = DMM_WEDGE_FULL_IDX;
      uiRdModeList[ numModesForFullRD++ ] = DMM_WEDGE_FULL_D_IDX;

      if ( uiWidth > 4 )
      {
        Int  iWedgeDeltaEnd = 0;

        iDeltaDC1 = 0;
        iDeltaDC2 = 0;

        findWedgePredDirMinDist( pcCU, uiPartOffset, piOrg, piPred, uiStride, uiWidth, uiHeight, uiTabIdx, iWedgeDeltaEnd, iDeltaDC1, iDeltaDC2, bAboveAvail, bLeftAvail );
        pcCU->setWedgePredDirTabIdxSubParts  ( uiTabIdx,       uiPartOffset, uiDepth + uiInitTrDepth );
        pcCU->setWedgePredDirDeltaEndSubParts( iWedgeDeltaEnd, uiPartOffset, uiDepth + uiInitTrDepth );
        pcCU->setWedgePredDirDeltaDC1SubParts( iDeltaDC1,      uiPartOffset, uiDepth + uiInitTrDepth );
        pcCU->setWedgePredDirDeltaDC2SubParts( iDeltaDC2,      uiPartOffset, uiDepth + uiInitTrDepth );

        uiRdModeList[ numModesForFullRD++ ] = DMM_WEDGE_PREDDIR_IDX;
        uiRdModeList[ numModesForFullRD++ ] = DMM_WEDGE_PREDDIR_D_IDX;
      }
#endif
#if HHI_DMM_PRED_TEX
      UInt uiTexTabIdx  = 0;
      Int  iTexDeltaDC1 = 0;
      Int  iTexDeltaDC2 = 0;
      findWedgeTexMinDist( pcCU, uiPartOffset, piOrg, piPred, uiStride, uiWidth, uiHeight, uiTexTabIdx, iTexDeltaDC1, iTexDeltaDC2, bAboveAvail, bLeftAvail ); 
      pcCU->setWedgePredTexTabIdxSubParts  ( uiTexTabIdx,  uiPartOffset, uiDepth + uiInitTrDepth );
      pcCU->setWedgePredTexDeltaDC1SubParts( iTexDeltaDC1, uiPartOffset, uiDepth + uiInitTrDepth );
      pcCU->setWedgePredTexDeltaDC2SubParts( iTexDeltaDC2, uiPartOffset, uiDepth + uiInitTrDepth );

      uiRdModeList[ numModesForFullRD++ ] = DMM_WEDGE_PREDTEX_IDX;
      uiRdModeList[ numModesForFullRD++ ] = DMM_WEDGE_PREDTEX_D_IDX;

      if ( uiWidth > 4 )
      {
        iTexDeltaDC1 = 0;
        iTexDeltaDC2 = 0;

        findContourPredTex( pcCU, uiPartOffset, piOrg, piPred, uiStride, uiWidth, uiHeight, iTexDeltaDC1, iTexDeltaDC2, bAboveAvail, bLeftAvail );
        pcCU->setContourPredTexDeltaDC1SubParts( iTexDeltaDC1, uiPartOffset, uiDepth + uiInitTrDepth );
        pcCU->setContourPredTexDeltaDC2SubParts( iTexDeltaDC2, uiPartOffset, uiDepth + uiInitTrDepth );

        uiRdModeList[ numModesForFullRD++ ] = DMM_CONTOUR_PREDTEX_IDX;
        uiRdModeList[ numModesForFullRD++ ] = DMM_CONTOUR_PREDTEX_D_IDX;
      }
#endif
    }
#endif

    //===== check modes (using r-d costs) =====
#if HHI_RQT_INTRA_SPEEDUP_MOD
    UInt   uiSecondBestMode  = MAX_UINT;
    Double dSecondBestPUCost = MAX_DOUBLE;
#endif
    
    UInt    uiBestPUMode  = 0;
    UInt    uiBestPUDistY = 0;
    UInt    uiBestPUDistC = 0;
    Double  dBestPUCost   = MAX_DOUBLE;
    for( UInt uiMode = 0; uiMode < numModesForFullRD; uiMode++ )
    {
      // set luma prediction mode
      UInt uiOrgMode = uiRdModeList[uiMode];
      
#if HHI_DMM_WEDGE_INTRA || HHI_DMM_PRED_TEX
      if( m_pcEncCfg->getIsDepth() && !predIntraLumaDMMAvailable( uiOrgMode, uiWidth, uiHeight ) )
      {
        continue;
      }
#endif

      pcCU->setLumaIntraDirSubParts ( uiOrgMode, uiPartOffset, uiDepth + uiInitTrDepth );
      
      // set context models
      if( m_bUseSBACRD )
      {
        m_pcRDGoOnSbacCoder->load( m_pppcRDSbacCoder[uiDepth][CI_CURR_BEST] );
      }
      
      // determine residual for partition
      Dist   uiPUDistY = 0;
      Dist   uiPUDistC = 0;
      Double dPUCost   = 0.0;


      // reset Model
#if HHI_VSO
      if( m_pcRdCost->getUseRenModel() )
      {
        m_pcRdCost->setRenModelData( pcCU, uiPartOffset, piOrg, uiStride, uiWidth, uiHeight );
      }
#endif

#if HHI_RQT_INTRA_SPEEDUP
      xRecurIntraCodingQT( pcCU, uiInitTrDepth, uiPartOffset, bLumaOnly, pcOrgYuv, pcPredYuv, pcResiYuv, uiPUDistY, uiPUDistC, true, dPUCost );
#else
      xRecurIntraCodingQT( pcCU, uiInitTrDepth, uiPartOffset, bLumaOnly, pcOrgYuv, pcPredYuv, pcResiYuv, uiPUDistY, uiPUDistC, dPUCost );
#endif
      
      // check r-d cost
      if( dPUCost < dBestPUCost )
      {
#if HHI_RQT_INTRA_SPEEDUP_MOD
        uiSecondBestMode  = uiBestPUMode;
        dSecondBestPUCost = dBestPUCost;
#endif
        uiBestPUMode  = uiOrgMode;
        uiBestPUDistY = uiPUDistY;
        uiBestPUDistC = uiPUDistC;
        dBestPUCost   = dPUCost;
        
        xSetIntraResultQT( pcCU, uiInitTrDepth, uiPartOffset, bLumaOnly, pcRecoYuv );
        
        UInt uiQPartNum = pcCU->getPic()->getNumPartInCU() >> ( ( pcCU->getDepth(0) + uiInitTrDepth ) << 1 );
        ::memcpy( m_puhQTTempTrIdx,  pcCU->getTransformIdx()       + uiPartOffset, uiQPartNum * sizeof( UChar ) );
        ::memcpy( m_puhQTTempCbf[0], pcCU->getCbf( TEXT_LUMA     ) + uiPartOffset, uiQPartNum * sizeof( UChar ) );
        ::memcpy( m_puhQTTempCbf[1], pcCU->getCbf( TEXT_CHROMA_U ) + uiPartOffset, uiQPartNum * sizeof( UChar ) );
        ::memcpy( m_puhQTTempCbf[2], pcCU->getCbf( TEXT_CHROMA_V ) + uiPartOffset, uiQPartNum * sizeof( UChar ) );
        
      }
#if HHI_RQT_INTRA_SPEEDUP_MOD
      else if( dPUCost < dSecondBestPUCost )
      {
        uiSecondBestMode  = uiOrgMode;
        dSecondBestPUCost = dPUCost;
      }
#endif
    } // Mode loop
    
#if HHI_RQT_INTRA_SPEEDUP
#if HHI_RQT_INTRA_SPEEDUP_MOD
    for( UInt ui =0; ui < 2; ++ui )
#endif
    {
#if HHI_RQT_INTRA_SPEEDUP_MOD
      UInt uiOrgMode   = ui ? uiSecondBestMode  : uiBestPUMode;
      if( uiOrgMode == MAX_UINT )
      {
        break;
      }
#else
      UInt uiOrgMode = uiBestPUMode;
#endif
      
      pcCU->setLumaIntraDirSubParts ( uiOrgMode, uiPartOffset, uiDepth + uiInitTrDepth );
      
      // set context models
      if( m_bUseSBACRD )
      {
        m_pcRDGoOnSbacCoder->load( m_pppcRDSbacCoder[uiDepth][CI_CURR_BEST] );
      }
      
      // determine residual for partition
      Dist   uiPUDistY = 0;
      Dist   uiPUDistC = 0;
      Double dPUCost   = 0.0;

#if HHI_VSO
      // reset Model
      if( m_pcRdCost->getUseRenModel() )
      {
        m_pcRdCost->setRenModelData( pcCU, uiPartOffset, piOrg, uiStride, uiWidth, uiHeight );
      }
#endif

      xRecurIntraCodingQT( pcCU, uiInitTrDepth, uiPartOffset, bLumaOnly, pcOrgYuv, pcPredYuv, pcResiYuv, uiPUDistY, uiPUDistC, false, dPUCost );
      
      // check r-d cost
      if( dPUCost < dBestPUCost )
      {
        uiBestPUMode  = uiOrgMode;
        uiBestPUDistY = uiPUDistY;
        uiBestPUDistC = uiPUDistC;
        dBestPUCost   = dPUCost;
        
        xSetIntraResultQT( pcCU, uiInitTrDepth, uiPartOffset, bLumaOnly, pcRecoYuv );
        
        UInt uiQPartNum = pcCU->getPic()->getNumPartInCU() >> ( ( pcCU->getDepth(0) + uiInitTrDepth ) << 1 );
        ::memcpy( m_puhQTTempTrIdx,  pcCU->getTransformIdx()       + uiPartOffset, uiQPartNum * sizeof( UChar ) );
        ::memcpy( m_puhQTTempCbf[0], pcCU->getCbf( TEXT_LUMA     ) + uiPartOffset, uiQPartNum * sizeof( UChar ) );
        ::memcpy( m_puhQTTempCbf[1], pcCU->getCbf( TEXT_CHROMA_U ) + uiPartOffset, uiQPartNum * sizeof( UChar ) );
        ::memcpy( m_puhQTTempCbf[2], pcCU->getCbf( TEXT_CHROMA_V ) + uiPartOffset, uiQPartNum * sizeof( UChar ) );
        
      }
    } // Mode loop
#endif
    
    //--- update overall distortion ---
    uiOverallDistY += uiBestPUDistY;
    uiOverallDistC += uiBestPUDistC;
    
    //--- update transform index and cbf ---
    UInt uiQPartNum = pcCU->getPic()->getNumPartInCU() >> ( ( pcCU->getDepth(0) + uiInitTrDepth ) << 1 );
    ::memcpy( pcCU->getTransformIdx()       + uiPartOffset, m_puhQTTempTrIdx,  uiQPartNum * sizeof( UChar ) );
    ::memcpy( pcCU->getCbf( TEXT_LUMA     ) + uiPartOffset, m_puhQTTempCbf[0], uiQPartNum * sizeof( UChar ) );
    ::memcpy( pcCU->getCbf( TEXT_CHROMA_U ) + uiPartOffset, m_puhQTTempCbf[1], uiQPartNum * sizeof( UChar ) );
    ::memcpy( pcCU->getCbf( TEXT_CHROMA_V ) + uiPartOffset, m_puhQTTempCbf[2], uiQPartNum * sizeof( UChar ) );
    
    //--- set reconstruction for next intra prediction blocks ---
    if( uiPU != uiNumPU - 1 )
    {
      Bool bSkipChroma  = false;
      Bool bChromaSame  = false;
      UInt uiLog2TrSize = g_aucConvertToBit[ pcCU->getSlice()->getSPS()->getMaxCUWidth() >> ( pcCU->getDepth(0) + uiInitTrDepth ) ] + 2;
      if( !bLumaOnly && uiLog2TrSize == 2 )
      {
        assert( uiInitTrDepth  > 0 );
        bSkipChroma  = ( uiPU != 0 );
        bChromaSame  = true;
      }
      
      UInt    uiCompWidth   = pcCU->getWidth ( 0 ) >> uiInitTrDepth;
      UInt    uiCompHeight  = pcCU->getHeight( 0 ) >> uiInitTrDepth;
      UInt    uiZOrder      = pcCU->getZorderIdxInCU() + uiPartOffset;
      Pel*    piDes         = pcCU->getPic()->getPicYuvRec()->getLumaAddr( pcCU->getAddr(), uiZOrder );
      UInt    uiDesStride   = pcCU->getPic()->getPicYuvRec()->getStride();
      Pel*    piSrc         = pcRecoYuv->getLumaAddr( uiPartOffset );
      UInt    uiSrcStride   = pcRecoYuv->getStride();
      for( UInt uiY = 0; uiY < uiCompHeight; uiY++, piSrc += uiSrcStride, piDes += uiDesStride )
      {
        for( UInt uiX = 0; uiX < uiCompWidth; uiX++ )
        {
          piDes[ uiX ] = piSrc[ uiX ];
        }
      }

#if HHI_VSO
      // set model
      if( m_pcRdCost->getUseRenModel() )
      {
        piSrc = pcRecoYuv->getLumaAddr( uiPartOffset );
        m_pcRdCost->setRenModelData( pcCU, uiPartOffset, piSrc, uiSrcStride, uiCompWidth, uiCompHeight);
      }
#endif

      if( !bLumaOnly && !bSkipChroma )
      {
        if( !bChromaSame )
        {
          uiCompWidth   >>= 1;
          uiCompHeight  >>= 1;
        }
        piDes         = pcCU->getPic()->getPicYuvRec()->getCbAddr( pcCU->getAddr(), uiZOrder );
        uiDesStride   = pcCU->getPic()->getPicYuvRec()->getCStride();
        piSrc         = pcRecoYuv->getCbAddr( uiPartOffset );
        uiSrcStride   = pcRecoYuv->getCStride();
        for( UInt uiY = 0; uiY < uiCompHeight; uiY++, piSrc += uiSrcStride, piDes += uiDesStride )
        {
          for( UInt uiX = 0; uiX < uiCompWidth; uiX++ )
          {
            piDes[ uiX ] = piSrc[ uiX ];
          }
        }
        piDes         = pcCU->getPic()->getPicYuvRec()->getCrAddr( pcCU->getAddr(), uiZOrder );
        piSrc         = pcRecoYuv->getCrAddr( uiPartOffset );
        for( UInt uiY = 0; uiY < uiCompHeight; uiY++, piSrc += uiSrcStride, piDes += uiDesStride )
        {
          for( UInt uiX = 0; uiX < uiCompWidth; uiX++ )
          {
            piDes[ uiX ] = piSrc[ uiX ];
          }
        }
      }
    }
    
    //=== update PU data ====
    pcCU->setLumaIntraDirSubParts     ( uiBestPUMode, uiPartOffset, uiDepth + uiInitTrDepth );
    pcCU->copyToPic                   ( uiDepth, uiPU, uiInitTrDepth );
  } // PU loop
  
  
  if( uiNumPU > 1 )
  { // set Cbf for all blocks
    UInt uiCombCbfY = 0;
    UInt uiCombCbfU = 0;
    UInt uiCombCbfV = 0;
    UInt uiPartIdx  = 0;
    for( UInt uiPart = 0; uiPart < 4; uiPart++, uiPartIdx += uiQNumParts )
    {
      uiCombCbfY |= pcCU->getCbf( uiPartIdx, TEXT_LUMA,     1 );
      uiCombCbfU |= pcCU->getCbf( uiPartIdx, TEXT_CHROMA_U, 1 );
      uiCombCbfV |= pcCU->getCbf( uiPartIdx, TEXT_CHROMA_V, 1 );
    }
    for( UInt uiOffs = 0; uiOffs < 4 * uiQNumParts; uiOffs++ )
    {
      pcCU->getCbf( TEXT_LUMA     )[ uiOffs ] |= uiCombCbfY;
      pcCU->getCbf( TEXT_CHROMA_U )[ uiOffs ] |= uiCombCbfU;
      pcCU->getCbf( TEXT_CHROMA_V )[ uiOffs ] |= uiCombCbfV;
    }
  }
  
  //===== reset context models =====
  if(m_bUseSBACRD)
  {
    m_pcRDGoOnSbacCoder->load(m_pppcRDSbacCoder[uiDepth][CI_CURR_BEST]);
  }
  
  //===== set distortion (rate and r-d costs are determined later) =====
  ruiDistC                   = uiOverallDistC;
  pcCU->getTotalDistortion() = uiOverallDistY + uiOverallDistC;
}



Void 
TEncSearch::estIntraPredChromaQT( TComDataCU* pcCU, 
                                 TComYuv*    pcOrgYuv, 
                                 TComYuv*    pcPredYuv, 
                                 TComYuv*    pcResiYuv, 
                                 TComYuv*    pcRecoYuv,
                                 Dist        uiPreCalcDistC )
{
  UInt    uiDepth     = pcCU->getDepth(0);
  UInt    uiBestMode  = 0;
  Dist    uiBestDist  = 0;
  Double  dBestCost   = MAX_DOUBLE;
  
  //----- init mode list -----
  UInt  uiMinMode = 0;
  UInt  uiModeList[ NUM_CHROMA_MODE ];
  pcCU->getAllowedChromaDir( 0, uiModeList );
  UInt  uiMaxMode = NUM_CHROMA_MODE;

  //----- check chroma modes -----
  for( UInt uiMode = uiMinMode; uiMode < uiMaxMode; uiMode++ )
  {
    if ( !pcCU->getSlice()->getSPS()->getUseLMChroma() && uiModeList[uiMode] == LM_CHROMA_IDX )
    {
      continue;
    }
    //----- restore context models -----
    if( m_bUseSBACRD )
    {
      m_pcRDGoOnSbacCoder->load( m_pppcRDSbacCoder[uiDepth][CI_CURR_BEST] );
    }
    
    //----- chroma coding -----
    Dist    uiDist = 0;
    pcCU->setChromIntraDirSubParts  ( uiModeList[uiMode], 0, uiDepth );
    xRecurIntraChromaCodingQT       ( pcCU,   0, 0, pcOrgYuv, pcPredYuv, pcResiYuv, uiDist );
    UInt    uiBits = xGetIntraBitsQT( pcCU,   0, 0, false, true, false );
    Double  dCost  = m_pcRdCost->calcRdCost( uiBits, uiDist );
    
    //----- compare -----
    if( dCost < dBestCost )
    {
      dBestCost   = dCost;
      uiBestDist  = uiDist;
      uiBestMode  = uiModeList[uiMode];
      UInt  uiQPN = pcCU->getPic()->getNumPartInCU() >> ( uiDepth << 1 );
      xSetIntraResultChromaQT( pcCU, 0, 0, pcRecoYuv );
      ::memcpy( m_puhQTTempCbf[1], pcCU->getCbf( TEXT_CHROMA_U ), uiQPN * sizeof( UChar ) );
      ::memcpy( m_puhQTTempCbf[2], pcCU->getCbf( TEXT_CHROMA_V ), uiQPN * sizeof( UChar ) );
    }
  }
  
  //----- set data -----
  UInt  uiQPN = pcCU->getPic()->getNumPartInCU() >> ( uiDepth << 1 );
  ::memcpy( pcCU->getCbf( TEXT_CHROMA_U ), m_puhQTTempCbf[1], uiQPN * sizeof( UChar ) );
  ::memcpy( pcCU->getCbf( TEXT_CHROMA_V ), m_puhQTTempCbf[2], uiQPN * sizeof( UChar ) );
  pcCU->setChromIntraDirSubParts( uiBestMode, 0, uiDepth );
  pcCU->getTotalDistortion      () += uiBestDist - uiPreCalcDistC;
  
  //----- restore context models -----
  if( m_bUseSBACRD )
  {
    m_pcRDGoOnSbacCoder->load( m_pppcRDSbacCoder[uiDepth][CI_CURR_BEST] );
  }
}

/** Function for encoding and reconstructing luma/chroma samples of a PCM mode CU.
 * \param pcCU pointer to current CU
 * \param uiAbsPartIdx part index
 * \param piOrg pointer to original sample arrays
 * \param piPCM pointer to PCM code arrays
 * \param piPred pointer to prediction signal arrays
 * \param piResi pointer to residual signal arrays
 * \param piReco pointer to reconstructed sample arrays
 * \param uiStride stride of the original/prediction/residual sample arrays
 * \param uiWidth block width
 * \param uiHeight block height
 * \param ttText texture component type
 * \returns Void
 */
Void TEncSearch::xEncPCM (TComDataCU* pcCU, UInt uiAbsPartIdx, Pel* piOrg, Pel* piPCM, Pel* piPred, Pel* piResi, Pel* piReco, UInt uiStride, UInt uiWidth, UInt uiHeight, TextType eText )
{
  UInt uiX, uiY;
  UInt uiReconStride;
  Pel* pOrg  = piOrg;
  Pel* pPCM  = piPCM;
  Pel* pPred = piPred;
  Pel* pResi = piResi;
  Pel* pReco = piReco;
  Pel* pRecoPic;
  UInt uiInternalBitDepth = g_uiBitDepth + g_uiBitIncrement;
  UInt uiPCMBitDepth;

  if( eText == TEXT_LUMA)
  {
    uiReconStride = pcCU->getPic()->getPicYuvRec()->getStride();
    pRecoPic      = pcCU->getPic()->getPicYuvRec()->getLumaAddr(pcCU->getAddr(), pcCU->getZorderIdxInCU()+uiAbsPartIdx);
    uiPCMBitDepth = pcCU->getSlice()->getSPS()->getPCMBitDepthLuma();
  }
  else
  {
    uiReconStride = pcCU->getPic()->getPicYuvRec()->getCStride();

    if( eText == TEXT_CHROMA_U )
    {
      pRecoPic = pcCU->getPic()->getPicYuvRec()->getCbAddr(pcCU->getAddr(), pcCU->getZorderIdxInCU()+uiAbsPartIdx);
    }
    else
    {
      pRecoPic = pcCU->getPic()->getPicYuvRec()->getCrAddr(pcCU->getAddr(), pcCU->getZorderIdxInCU()+uiAbsPartIdx);
    }
    uiPCMBitDepth = pcCU->getSlice()->getSPS()->getPCMBitDepthChroma();
  }

  // Reset pred and residual
  for( uiY = 0; uiY < uiHeight; uiY++ )
  {
    for( uiX = 0; uiX < uiWidth; uiX++ )
    {
      pPred[uiX] = 0;
      pResi[uiX] = 0;
    }
    pPred += uiStride;
    pResi += uiStride;
  }

  // Encode
  for( uiY = 0; uiY < uiHeight; uiY++ )
  {
    for( uiX = 0; uiX < uiWidth; uiX++ )
    {
      pPCM[uiX] = (pOrg[uiX]>>(uiInternalBitDepth - uiPCMBitDepth));
    }
    pPCM += uiWidth;
    pOrg += uiStride;
  }

  pPCM  = piPCM;

  // Reconstruction
  for( uiY = 0; uiY < uiHeight; uiY++ )
  {
    for( uiX = 0; uiX < uiWidth; uiX++ )
    {
      pReco   [uiX] = (pPCM[uiX]<<(uiInternalBitDepth - uiPCMBitDepth));
      pRecoPic[uiX] = pReco[uiX];
    }
    pPCM += uiWidth;
    pReco += uiStride;
    pRecoPic += uiReconStride;
  }
}

/**  Function for PCM mode estimation.
 * \param pcCU
 * \param pcOrgYuv
 * \param rpcPredYuv
 * \param rpcResiYuv
 * \param rpcRecoYuv
 * \returns Void
 */
Void TEncSearch::IPCMSearch( TComDataCU* pcCU, TComYuv* pcOrgYuv, TComYuv*& rpcPredYuv, TComYuv*& rpcResiYuv, TComYuv*& rpcRecoYuv )
{
  UInt   uiDepth        = pcCU->getDepth(0);
  UInt   uiWidth        = pcCU->getWidth(0);
  UInt   uiHeight       = pcCU->getHeight(0);
  UInt   uiStride       = rpcPredYuv->getStride();
  UInt   uiStrideC      = rpcPredYuv->getCStride();
  UInt   uiWidthC       = uiWidth  >> 1;
  UInt   uiHeightC      = uiHeight >> 1;
  Dist   uiDistortion = 0;
  UInt   uiBits;

  Double dCost;

  Pel*    pOrig;
  Pel*    pResi;
  Pel*    pReco;
  Pel*    pPred;
  Pel*    pPCM;

  UInt uiAbsPartIdx = 0;

  UInt uiMinCoeffSize = pcCU->getPic()->getMinCUWidth()*pcCU->getPic()->getMinCUHeight();
  UInt uiLumaOffset   = uiMinCoeffSize*uiAbsPartIdx;
  UInt uiChromaOffset = uiLumaOffset>>2;

  // Luminance
  pOrig    = pcOrgYuv->getLumaAddr(0, uiWidth);
  pResi    = rpcResiYuv->getLumaAddr(0, uiWidth);
  pPred    = rpcPredYuv->getLumaAddr(0, uiWidth);
  pReco    = rpcRecoYuv->getLumaAddr(0, uiWidth);
  pPCM     = pcCU->getPCMSampleY() + uiLumaOffset;

  xEncPCM ( pcCU, 0, pOrig, pPCM, pPred, pResi, pReco, uiStride, uiWidth, uiHeight, TEXT_LUMA );

  // Chroma U
  pOrig    = pcOrgYuv->getCbAddr();
  pResi    = rpcResiYuv->getCbAddr();
  pPred    = rpcPredYuv->getCbAddr();
  pReco    = rpcRecoYuv->getCbAddr();
  pPCM     = pcCU->getPCMSampleCb() + uiChromaOffset;

  xEncPCM ( pcCU, 0, pOrig, pPCM, pPred, pResi, pReco, uiStrideC, uiWidthC, uiHeightC, TEXT_CHROMA_U );

  // Chroma V
  pOrig    = pcOrgYuv->getCrAddr();
  pResi    = rpcResiYuv->getCrAddr();
  pPred    = rpcPredYuv->getCrAddr();
  pReco    = rpcRecoYuv->getCrAddr();
  pPCM     = pcCU->getPCMSampleCr() + uiChromaOffset;

  xEncPCM ( pcCU, 0, pOrig, pPCM, pPred, pResi, pReco, uiStrideC, uiWidthC, uiHeightC, TEXT_CHROMA_V );

  m_pcEntropyCoder->resetBits();
  xEncIntraHeader ( pcCU, uiDepth, uiAbsPartIdx, true, false);
  uiBits = m_pcEntropyCoder->getNumberOfWrittenBits();

#if HHI_VSO
  if( m_pcRdCost->getUseLambdaScaleVSO() )
  {
    dCost =  m_pcRdCost->calcRdCostVSO( uiBits, uiDistortion );
  }
  else
#endif
  {
  dCost = m_pcRdCost->calcRdCost( uiBits, uiDistortion );
  }

  if(m_bUseSBACRD)
  {
    m_pcRDGoOnSbacCoder->load(m_pppcRDSbacCoder[uiDepth][CI_CURR_BEST]);
  }

  pcCU->getTotalBits()       = uiBits;
  pcCU->getTotalCost()       = dCost;
  pcCU->getTotalDistortion() = uiDistortion;

  pcCU->copyToPic(uiDepth, 0, 0);
}

Void TEncSearch::xGetInterPredictionError( TComDataCU* pcCU, TComYuv* pcYuvOrg, Int iPartIdx, UInt& ruiErr, Bool bHadamard )
{
  motionCompensation( pcCU, &m_tmpYuvPred, REF_PIC_LIST_X, iPartIdx );

  UInt uiAbsPartIdx = 0;
  Int iWidth = 0;
  Int iHeight = 0;
  pcCU->getPartIndexAndSize( iPartIdx, uiAbsPartIdx, iWidth, iHeight );

  DistParam cDistParam;

  cDistParam.bApplyWeight = false;

  m_pcRdCost->setDistParam( cDistParam, 
                            pcYuvOrg->getLumaAddr( uiAbsPartIdx ), pcYuvOrg->getStride(), 
                            m_tmpYuvPred .getLumaAddr( uiAbsPartIdx ), m_tmpYuvPred .getStride(), 
#if NS_HAD
                            iWidth, iHeight, m_pcEncCfg->getUseHADME(), m_pcEncCfg->getUseNSQT() );
#else
                            iWidth, iHeight, m_pcEncCfg->getUseHADME() );
#endif
  ruiErr = cDistParam.DistFunc( &cDistParam );
}

/** estimation of best merge coding
 * \param pcCU
 * \param pcYuvOrg
 * \param iPUIdx
 * \param uiInterDir
 * \param pacMvField
 * \param uiMergeIndex
 * \param ruiCost
 * \param ruiBits
 * \param puhNeighCands
 * \param bValid 
 * \returns Void
 */
#if CU_BASED_MRG_CAND_LIST
Void TEncSearch::xMergeEstimation( TComDataCU* pcCU, TComYuv* pcYuvOrg, Int iPUIdx, UInt& uiInterDir, TComMvField* pacMvField, UInt& uiMergeIndex, UInt& ruiCost, TComMvField* cMvFieldNeighbours, UChar* uhInterDirNeighbours, Int& numValidMergeCand )
#else
#if LG_RESTRICTEDRESPRED_M24766
Void TEncSearch::xMergeEstimation( TComDataCU* pcCU, TComYuv* pcYuvOrg, TComYuv* rpcResiPredYuv, Int iPUIdx, UInt& uiInterDir, TComMvField* pacMvField, UInt& uiMergeIndex, UInt& ruiCost )
#else
Void TEncSearch::xMergeEstimation( TComDataCU* pcCU, TComYuv* pcYuvOrg, Int iPUIdx, UInt& uiInterDir, TComMvField* pacMvField, UInt& uiMergeIndex, UInt& ruiCost )
#endif
#endif
{
#if !CU_BASED_MRG_CAND_LIST
#if HHI_INTER_VIEW_MOTION_PRED
  TComMvField  cMvFieldNeighbours[MRG_MAX_NUM_CANDS_MEM << 1]; // double length for mv of both lists
  UChar uhInterDirNeighbours[MRG_MAX_NUM_CANDS_MEM];
  Int numValidMergeCand = 0;
  for( UInt ui = 0; ui < MRG_MAX_NUM_CANDS_MEM; ++ui )
#else
  TComMvField  cMvFieldNeighbours[MRG_MAX_NUM_CANDS << 1]; // double length for mv of both lists
  UChar uhInterDirNeighbours[MRG_MAX_NUM_CANDS];
  Int numValidMergeCand = 0;
  for( UInt ui = 0; ui < MRG_MAX_NUM_CANDS; ++ui )
#endif
  {
    uhInterDirNeighbours[ui] = 0;
  }
#endif

  UInt uiAbsPartIdx = 0;
  Int iWidth = 0;
  Int iHeight = 0; 

  pcCU->getPartIndexAndSize( iPUIdx, uiAbsPartIdx, iWidth, iHeight );
  UInt uiDepth = pcCU->getDepth( uiAbsPartIdx );
#if CU_BASED_MRG_CAND_LIST
  PartSize partSize = pcCU->getPartitionSize( 0 );
  if ( pcCU->getSlice()->getPPS()->getLog2ParallelMergeLevelMinus2() && partSize != SIZE_2Nx2N && pcCU->getWidth( 0 ) <= 8 )
  {
    pcCU->setPartSizeSubParts( SIZE_2Nx2N, 0, uiDepth );
    if ( iPUIdx == 0 )
    {
      pcCU->getInterMergeCandidates( 0, 0, uiDepth, cMvFieldNeighbours,uhInterDirNeighbours, numValidMergeCand );
    }
    pcCU->setPartSizeSubParts( partSize, 0, uiDepth );
  }
  else
  {
    pcCU->getInterMergeCandidates( uiAbsPartIdx, iPUIdx, uiDepth, cMvFieldNeighbours, uhInterDirNeighbours, numValidMergeCand );
  }
#else
  pcCU->getInterMergeCandidates( uiAbsPartIdx, iPUIdx, uiDepth, cMvFieldNeighbours,uhInterDirNeighbours, numValidMergeCand );
#endif

#if HHI_INTER_VIEW_MOTION_PRED
  const int maxNumMergeCand = MRG_MAX_NUM_CANDS_SIGNALED + ( pcCU->getSlice()->getSPS()->getMultiviewMvPredMode() ? 1 : 0 );
#endif
#if LG_RESTRICTEDRESPRED_M24766
  Int iPUResiPredShift[4];
  Int iLastAddResiShift = -1000;
#endif
  ruiCost = MAX_UINT;
  for( UInt uiMergeCand = 0; uiMergeCand < numValidMergeCand; ++uiMergeCand )
  {
    {
      UInt uiCostCand = MAX_UINT;
      UInt uiBitsCand = 0;
      
      PartSize ePartSize = pcCU->getPartitionSize( 0 );

      pcCU->getCUMvField(REF_PIC_LIST_0)->setAllMvField( cMvFieldNeighbours[0 + 2*uiMergeCand], ePartSize, uiAbsPartIdx, 0, iPUIdx );
      pcCU->getCUMvField(REF_PIC_LIST_1)->setAllMvField( cMvFieldNeighbours[1 + 2*uiMergeCand], ePartSize, uiAbsPartIdx, 0, iPUIdx );
#if LG_RESTRICTEDRESPRED_M24766
	  Int iAddResiShift;
	  UInt uiPartAddr;
	  Int iRoiWidth, iRoiHeight;

	  pcCU->getPartIndexAndSize( iPUIdx, uiPartAddr, iRoiWidth, iRoiHeight );
	  iAddResiShift = pcCU->getResiPredMode(uiPartAddr);
	  iAddResiShift = (pcCU->getSlice()->getPPS()->getUseWP() || pcCU->getInterDir(uiPartAddr) != 3) ? (iAddResiShift >= 0 ? 0 : -1) : (iAddResiShift >= 0 ? 1 - iAddResiShift : -1);

	  if( pcCU->getResPredFlag( 0 ))
	  { // subtract residual prediction from original in motion search
		  if(iLastAddResiShift != iAddResiShift)
		  {
			  //add subtracted residual last time
			  if(iLastAddResiShift >= 0)
			  {
				  iPUResiPredShift[0] = iPUResiPredShift[1] = iPUResiPredShift[2] = iPUResiPredShift[3] = iLastAddResiShift;
				  pcYuvOrg->add(iPUResiPredShift, ePartSize, rpcResiPredYuv, pcCU->getWidth( 0 ), pcCU->getHeight( 0 ));
			  }
			  //subtract residual
			  if(iAddResiShift >= 0)
			  {
				  iPUResiPredShift[0] = iPUResiPredShift[1] = iPUResiPredShift[2] = iPUResiPredShift[3] = iAddResiShift;
				  pcYuvOrg->add(iPUResiPredShift, ePartSize, rpcResiPredYuv, pcCU->getWidth( 0 ), pcCU->getHeight( 0 ), true );
			  }
			  iLastAddResiShift = iAddResiShift;
		  }
	  }
#endif
      xGetInterPredictionError( pcCU, pcYuvOrg, iPUIdx, uiCostCand, m_pcEncCfg->getUseHADME() );
      uiBitsCand = uiMergeCand + 1;
#if HHI_INTER_VIEW_MOTION_PRED
      if (uiMergeCand == maxNumMergeCand - 1 )
#else
      if (uiMergeCand == MRG_MAX_NUM_CANDS_SIGNALED -1)
#endif
      {
         uiBitsCand--;
      }
      uiCostCand = uiCostCand + m_pcRdCost->getCost( uiBitsCand );
      if ( uiCostCand < ruiCost )
      {
        ruiCost = uiCostCand;
        pacMvField[0] = cMvFieldNeighbours[0 + 2*uiMergeCand];
        pacMvField[1] = cMvFieldNeighbours[1 + 2*uiMergeCand];
        uiInterDir = uhInterDirNeighbours[uiMergeCand];
        uiMergeIndex = uiMergeCand;
      }
    }
  }
#if LG_RESTRICTEDRESPRED_M24766
  if( pcCU->getResPredFlag( 0 ) && iLastAddResiShift >= 0)
  {
	  iPUResiPredShift[0] = iPUResiPredShift[1] = iPUResiPredShift[2] = iPUResiPredShift[3] = iLastAddResiShift;
	  pcYuvOrg->add(iPUResiPredShift, pcCU->getPartitionSize(0), rpcResiPredYuv, pcCU->getWidth( 0 ), pcCU->getHeight( 0 ));
  }
#endif
}

/** search of the best candidate for inter prediction
 * \param pcCU
 * \param pcOrgYuv
 * \param rpcPredYuv
 * \param rpcResiYuv
 * \param rpcRecoYuv
 * \param bUseRes
 * \returns Void
 */
#if AMP_MRG
#if LG_RESTRICTEDRESPRED_M24766
Void TEncSearch::predInterSearch( TComDataCU* pcCU, TComYuv* pcOrgYuv, TComYuv* rpcResiPredYuv, TComYuv*& rpcPredYuv, TComYuv*& rpcResiYuv, TComYuv*& rpcRecoYuv, Bool bUseRes, Bool bUseMRG )
#else
Void TEncSearch::predInterSearch( TComDataCU* pcCU, TComYuv* pcOrgYuv, TComYuv*& rpcPredYuv, TComYuv*& rpcResiYuv, TComYuv*& rpcRecoYuv, Bool bUseRes, Bool bUseMRG )
#endif
#else
Void TEncSearch::predInterSearch( TComDataCU* pcCU, TComYuv* pcOrgYuv, TComYuv*& rpcPredYuv, TComYuv*& rpcResiYuv, TComYuv*& rpcRecoYuv, Bool bUseRes )
#endif
{
  m_acYuvPred[0].clear();
  m_acYuvPred[1].clear();
  m_cYuvPredTemp.clear();
  rpcPredYuv->clear();
  
  if ( !bUseRes )
  {
    rpcResiYuv->clear();
  }
  
  rpcRecoYuv->clear();
  
  TComMv        cMvSrchRngLT;
  TComMv        cMvSrchRngRB;
  
  TComMv        cMvZero;
  TComMv        TempMv; //kolya
  
  TComMv        cMv[2];
  TComMv        cMvBi[2];
  TComMv        cMvTemp[2][33];
  
  Int           iNumPart    = pcCU->getNumPartInter();
  Int           iNumPredDir = pcCU->getSlice()->isInterP() ? 1 : 2;
  
  TComMv        cMvPred[2][33];
  
  TComMv        cMvPredBi[2][33];
  Int           aaiMvpIdxBi[2][33];
  
  Int           aaiMvpIdx[2][33];
  Int           aaiMvpNum[2][33];
  
  AMVPInfo aacAMVPInfo[2][33];
  
  Int           iRefIdx[2]={0,0}; //If un-initialized, may cause SEGV in bi-directional prediction iterative stage.
  Int           iRefIdxBi[2];
  
  UInt          uiPartAddr;
  Int           iRoiWidth, iRoiHeight;
  
  UInt          uiMbBits[3] = {1, 1, 0};
  
  UInt          uiLastMode = 0;
  Int           iRefStart, iRefEnd;
  
  PartSize      ePartSize = pcCU->getPartitionSize( 0 );

#if H0111_MVD_L1_ZERO
  Int           bestBiPRefIdxL1 = 0;
  Int           bestBiPMvpL1 = 0;
  UInt          biPDistTemp = MAX_INT;
#endif

#if ZERO_MVD_EST
  Int           aiZeroMvdMvpIdx[2] = {-1, -1};
  Int           aiZeroMvdRefIdx[2] = {0, 0};
  Int           iZeroMvdDir = -1;
#endif

#if CU_BASED_MRG_CAND_LIST
#if HHI_INTER_VIEW_MOTION_PRED
  TComMvField cMvFieldNeighbours[MRG_MAX_NUM_CANDS_MEM << 1]; // double length for mv of both lists
  UChar uhInterDirNeighbours[MRG_MAX_NUM_CANDS_MEM];
  Int numValidMergeCand = 0 ;
#else
  TComMvField cMvFieldNeighbours[MRG_MAX_NUM_CANDS << 1]; // double length for mv of both lists
  UChar uhInterDirNeighbours[MRG_MAX_NUM_CANDS];
  Int numValidMergeCand = 0 ;
#endif 
#endif 

#if HHI_INTER_VIEW_MOTION_PRED
  Int iNumAMVPCands = AMVP_MAX_NUM_CANDS + ( pcCU->getSlice()->getSPS()->getMultiviewMvPredMode() ? 1 : 0 );
#endif

  for ( Int iPartIdx = 0; iPartIdx < iNumPart; iPartIdx++ )
  {
    UInt          uiCost[2] = { MAX_UINT, MAX_UINT };
    UInt          uiCostBi  =   MAX_UINT;
    UInt          uiCostTemp;
    
    UInt          uiBits[3];
    UInt          uiBitsTemp;
#if ZERO_MVD_EST
    UInt          uiZeroMvdCost = MAX_UINT;
    UInt          uiZeroMvdCostTemp;
    UInt          uiZeroMvdBitsTemp;
    UInt          uiZeroMvdDistTemp = MAX_UINT;
    UInt          auiZeroMvdBits[3];
#endif
#if H0111_MVD_L1_ZERO
    UInt          bestBiPDist = MAX_INT;
#endif

    UInt          uiCostTempL0[MAX_NUM_REF];
    for (Int iNumRef=0; iNumRef < MAX_NUM_REF; iNumRef++) uiCostTempL0[iNumRef] = MAX_UINT;
    UInt          uiBitsTempL0[MAX_NUM_REF];
#if LG_RESTRICTEDRESPRED_M24766
	Int iPUResiPredShift[4] = {0, 0, 0, 0};
#endif
    xGetBlkBits( ePartSize, pcCU->getSlice()->isInterP(), iPartIdx, uiLastMode, uiMbBits);
    
    pcCU->getPartIndexAndSize( iPartIdx, uiPartAddr, iRoiWidth, iRoiHeight );
    
#if AMP_MRG
    Bool bTestNormalMC = true;
    
    if ( bUseMRG && pcCU->getWidth( 0 ) > 8 && iNumPart == 2 )
    {
      bTestNormalMC = false;
    }
    
    if (bTestNormalMC)
    {
#endif
#if LG_RESTRICTEDRESPRED_M24766
		Bool bLastResiFlag = false;
#endif
    //  Uni-directional prediction
    for ( Int iRefList = 0; iRefList < iNumPredDir; iRefList++ )
    {
      RefPicList  eRefPicList = ( iRefList ? REF_PIC_LIST_1 : REF_PIC_LIST_0 );
      
      for ( Int iRefIdxTemp = 0; iRefIdxTemp < pcCU->getSlice()->getNumRefIdx(eRefPicList); iRefIdxTemp++ )
      {
#if LG_RESTRICTEDRESPRED_M24766
		  if( pcCU->getResPredFlag( 0 ))
		  {
			  if(pcCU->getSlice()->getViewId() == pcCU->getSlice()->getRefViewId(eRefPicList, iRefIdxTemp))
			  { // subtract residual prediction from original in motion search
				  if(!bLastResiFlag)
					  pcOrgYuv->add(iPUResiPredShift, pcCU->getPartitionSize(0), rpcResiPredYuv, pcCU->getWidth( 0 ), pcCU->getHeight( 0 ), true );
				  bLastResiFlag = true;
			  }
			  else
			  {
				  if(bLastResiFlag)
					  pcOrgYuv->add(iPUResiPredShift, pcCU->getPartitionSize(0), rpcResiPredYuv, pcCU->getWidth( 0 ), pcCU->getHeight( 0 ));
				  bLastResiFlag = false;
			  }
		  }
#endif
        uiBitsTemp = uiMbBits[iRefList];
        if ( pcCU->getSlice()->getNumRefIdx(eRefPicList) > 1 )
        {
          uiBitsTemp += iRefIdxTemp+1;
          if ( iRefIdxTemp == pcCU->getSlice()->getNumRefIdx(eRefPicList)-1 ) uiBitsTemp--;
        }
#if H0111_MVD_L1_ZERO
#if ZERO_MVD_EST
        xEstimateMvPredAMVP( pcCU, pcOrgYuv, iPartIdx, eRefPicList, iRefIdxTemp, cMvPred[iRefList][iRefIdxTemp], false, &biPDistTemp, &uiZeroMvdDistTemp);
#else
        xEstimateMvPredAMVP( pcCU, pcOrgYuv, iPartIdx, eRefPicList, iRefIdxTemp, cMvPred[iRefList][iRefIdxTemp], false, &biPDistTemp);
#endif
#else
#if ZERO_MVD_EST
        xEstimateMvPredAMVP( pcCU, pcOrgYuv, iPartIdx, eRefPicList, iRefIdxTemp, cMvPred[iRefList][iRefIdxTemp], false, &uiZeroMvdDistTemp);
#else
        xEstimateMvPredAMVP( pcCU, pcOrgYuv, iPartIdx, eRefPicList, iRefIdxTemp, cMvPred[iRefList][iRefIdxTemp]);
#endif
#endif
        aaiMvpIdx[iRefList][iRefIdxTemp] = pcCU->getMVPIdx(eRefPicList, uiPartAddr);
        aaiMvpNum[iRefList][iRefIdxTemp] = pcCU->getMVPNum(eRefPicList, uiPartAddr);
        
#if H0111_MVD_L1_ZERO
        if(pcCU->getSlice()->getMvdL1ZeroFlag() && iRefList==1 && biPDistTemp < bestBiPDist)
        {
          bestBiPDist = biPDistTemp;
          bestBiPMvpL1 = aaiMvpIdx[iRefList][iRefIdxTemp];
          bestBiPRefIdxL1 = iRefIdxTemp;
        }
#endif

#if HHI_INTER_VIEW_MOTION_PRED
        uiBitsTemp += m_auiMVPIdxCost[aaiMvpIdx[iRefList][iRefIdxTemp]][iNumAMVPCands];
#else
        uiBitsTemp += m_auiMVPIdxCost[aaiMvpIdx[iRefList][iRefIdxTemp]][AMVP_MAX_NUM_CANDS];
#endif
#if ZERO_MVD_EST
        if ((iRefList != 1 || !pcCU->getSlice()->getNoBackPredFlag()) &&
            (pcCU->getSlice()->getNumRefIdx(REF_PIC_LIST_C) <= 0 || pcCU->getSlice()->getRefIdxOfLC(eRefPicList, iRefIdxTemp)>=0))
        {
          uiZeroMvdBitsTemp = uiBitsTemp;
          uiZeroMvdBitsTemp += 2; //zero mvd bits

          m_pcRdCost->getMotionCost( 1, 0 );
          uiZeroMvdCostTemp = uiZeroMvdDistTemp + m_pcRdCost->getCost(uiZeroMvdBitsTemp);

          if (uiZeroMvdCostTemp < uiZeroMvdCost)
          {
            uiZeroMvdCost = uiZeroMvdCostTemp;
            iZeroMvdDir = iRefList + 1;
            aiZeroMvdRefIdx[iRefList] = iRefIdxTemp;
            aiZeroMvdMvpIdx[iRefList] = aaiMvpIdx[iRefList][iRefIdxTemp];
            auiZeroMvdBits[iRefList] = uiZeroMvdBitsTemp;
          }          
        }
#endif
        
#if GPB_SIMPLE_UNI
        if ( pcCU->getSlice()->getNumRefIdx(REF_PIC_LIST_C) > 0)
        {
          if ( iRefList && ( pcCU->getSlice()->getNoBackPredFlag() || (pcCU->getSlice()->getNumRefIdx(REF_PIC_LIST_C) > 0 && !pcCU->getSlice()->getNoBackPredFlag() && pcCU->getSlice()->getRefIdxOfL0FromRefIdxOfL1(iRefIdxTemp)>=0 ) ) )
            {
              if ( pcCU->getSlice()->getNoBackPredFlag() )
              {
                cMvTemp[1][iRefIdxTemp] = cMvTemp[0][iRefIdxTemp];
                uiCostTemp = uiCostTempL0[iRefIdxTemp];
                /*first subtract the bit-rate part of the cost of the other list*/
                uiCostTemp -= m_pcRdCost->getCost( uiBitsTempL0[iRefIdxTemp] );
              }
              else
              {
                cMvTemp[1][iRefIdxTemp] = cMvTemp[0][pcCU->getSlice()->getRefIdxOfL0FromRefIdxOfL1(iRefIdxTemp)]; 
                uiCostTemp = uiCostTempL0[pcCU->getSlice()->getRefIdxOfL0FromRefIdxOfL1(iRefIdxTemp)];
                /*first subtract the bit-rate part of the cost of the other list*/
                uiCostTemp -= m_pcRdCost->getCost( uiBitsTempL0[pcCU->getSlice()->getRefIdxOfL0FromRefIdxOfL1(iRefIdxTemp)] );
              }
              /*correct the bit-rate part of the current ref*/
              m_pcRdCost->setPredictor  ( cMvPred[iRefList][iRefIdxTemp] );
              uiBitsTemp += m_pcRdCost->getBits( cMvTemp[1][iRefIdxTemp].getHor(), cMvTemp[1][iRefIdxTemp].getVer() );
              /*calculate the correct cost*/
              uiCostTemp += m_pcRdCost->getCost( uiBitsTemp );
            }
            else
            {
              xMotionEstimation ( pcCU, pcOrgYuv, iPartIdx, eRefPicList, &cMvPred[iRefList][iRefIdxTemp], iRefIdxTemp, cMvTemp[iRefList][iRefIdxTemp], uiBitsTemp, uiCostTemp );
            }
        }
        else
        {
          if (iRefList && pcCU->getSlice()->getNoBackPredFlag())
          {
            uiCostTemp = MAX_UINT;
            cMvTemp[1][iRefIdxTemp] = cMvTemp[0][iRefIdxTemp];
          }
          else
          { 
            xMotionEstimation ( pcCU, pcOrgYuv, iPartIdx, eRefPicList, &cMvPred[iRefList][iRefIdxTemp], iRefIdxTemp, cMvTemp[iRefList][iRefIdxTemp], uiBitsTemp, uiCostTemp );
          }        
        }
#else
        xMotionEstimation ( pcCU, pcOrgYuv, iPartIdx, eRefPicList, &cMvPred[iRefList][iRefIdxTemp], iRefIdxTemp, cMvTemp[iRefList][iRefIdxTemp], uiBitsTemp, uiCostTemp );
#endif
        xCopyAMVPInfo(pcCU->getCUMvField(eRefPicList)->getAMVPInfo(), &aacAMVPInfo[iRefList][iRefIdxTemp]); // must always be done ( also when AMVP_MODE = AM_NONE )
        if ( pcCU->getAMVPMode(uiPartAddr) == AM_EXPL )
        {          
          xCheckBestMVP(pcCU, eRefPicList, cMvTemp[iRefList][iRefIdxTemp], cMvPred[iRefList][iRefIdxTemp], aaiMvpIdx[iRefList][iRefIdxTemp], uiBitsTemp, uiCostTemp);
        }

        if(pcCU->getSlice()->getNumRefIdx(REF_PIC_LIST_C) > 0 && !pcCU->getSlice()->getNoBackPredFlag())
        {
          if(iRefList==REF_PIC_LIST_0)
          {
            uiCostTempL0[iRefIdxTemp] = uiCostTemp;
            uiBitsTempL0[iRefIdxTemp] = uiBitsTemp;
            if(pcCU->getSlice()->getRefIdxOfLC(REF_PIC_LIST_0, iRefIdxTemp)<0)
            {
              uiCostTemp = MAX_UINT;
            }
          }
          else
          {
            if(pcCU->getSlice()->getRefIdxOfLC(REF_PIC_LIST_1, iRefIdxTemp)<0)
            {
              uiCostTemp = MAX_UINT;
            }           
          }
        }

        if ( ( iRefList == 0 && uiCostTemp < uiCost[iRefList] ) ||
            ( iRefList == 1 &&  pcCU->getSlice()->getNoBackPredFlag() && iRefIdxTemp == iRefIdx[0] ) ||
            ( iRefList == 1 && (pcCU->getSlice()->getNumRefIdx(REF_PIC_LIST_C) > 0) && (iRefIdxTemp==0 || iRefIdxTemp == iRefIdx[0]) && !pcCU->getSlice()->getNoBackPredFlag() && (iRefIdxTemp == pcCU->getSlice()->getRefIdxOfL0FromRefIdxOfL1(iRefIdxTemp)) ) ||
            ( iRefList == 1 && !pcCU->getSlice()->getNoBackPredFlag() && uiCostTemp < uiCost[iRefList] ) )
          {
            uiCost[iRefList] = uiCostTemp;
            uiBits[iRefList] = uiBitsTemp; // storing for bi-prediction
            
            // set motion
            cMv[iRefList]     = cMvTemp[iRefList][iRefIdxTemp];
            iRefIdx[iRefList] = iRefIdxTemp;
            pcCU->getCUMvField(eRefPicList)->setAllMv( cMv[iRefList], ePartSize, uiPartAddr, 0, iPartIdx );
            pcCU->getCUMvField(eRefPicList)->setAllRefIdx( iRefIdx[iRefList], ePartSize, uiPartAddr, 0, iPartIdx );

#if H0111_MVD_L1_ZERO
            if(!pcCU->getSlice()->getMvdL1ZeroFlag())
            {
#endif
              // storing list 1 prediction signal for iterative bi-directional prediction
              if ( eRefPicList == REF_PIC_LIST_1 )
              {
                TComYuv*  pcYuvPred = &m_acYuvPred[iRefList];
                motionCompensation ( pcCU, pcYuvPred, eRefPicList, iPartIdx );
              }
              if ( (pcCU->getSlice()->getNoBackPredFlag() || (pcCU->getSlice()->getNumRefIdx(REF_PIC_LIST_C) > 0 && pcCU->getSlice()->getRefIdxOfL0FromRefIdxOfL1(0)==0 )) && eRefPicList == REF_PIC_LIST_0 )
              {
                TComYuv*  pcYuvPred = &m_acYuvPred[iRefList];
                motionCompensation ( pcCU, pcYuvPred, eRefPicList, iPartIdx );
              }
#if H0111_MVD_L1_ZERO
            }
#endif
          }
      }
    }
#if LG_RESTRICTEDRESPRED_M24766
	if( pcCU->getResPredFlag( 0 ) && bLastResiFlag)
	{ // subtract residual prediction from original in motion search
		pcOrgYuv->add(iPUResiPredShift, pcCU->getPartitionSize(0), rpcResiPredYuv, pcCU->getWidth( 0 ), pcCU->getHeight( 0 ));
	}
#endif
    //  Bi-directional prediction
    if ( pcCU->getSlice()->isInterB() )
    {
#if LG_RESTRICTEDRESPRED_M24766
		Int iLastAddResiShift = -1000;
#endif
      cMvBi[0] = cMv[0];            cMvBi[1] = cMv[1];
      iRefIdxBi[0] = iRefIdx[0];    iRefIdxBi[1] = iRefIdx[1];
      
      ::memcpy(cMvPredBi, cMvPred, sizeof(cMvPred));
      ::memcpy(aaiMvpIdxBi, aaiMvpIdx, sizeof(aaiMvpIdx));
      
#if H0111_MVD_L1_ZERO
      UInt uiMotBits[2];

      if(pcCU->getSlice()->getMvdL1ZeroFlag())
      {
        xCopyAMVPInfo(&aacAMVPInfo[1][bestBiPRefIdxL1], pcCU->getCUMvField(REF_PIC_LIST_1)->getAMVPInfo());
        pcCU->setMVPIdxSubParts( bestBiPMvpL1, REF_PIC_LIST_1, uiPartAddr, iPartIdx, pcCU->getDepth(uiPartAddr));
        aaiMvpIdxBi[1][bestBiPRefIdxL1] = bestBiPMvpL1;
        cMvPredBi[1][bestBiPRefIdxL1]   = pcCU->getCUMvField(REF_PIC_LIST_1)->getAMVPInfo()->m_acMvCand[bestBiPMvpL1];

        cMvBi[1] = cMvPredBi[1][bestBiPRefIdxL1];
        iRefIdxBi[1] = bestBiPRefIdxL1;
        pcCU->getCUMvField( REF_PIC_LIST_1 )->setAllMv( cMvBi[1], ePartSize, uiPartAddr, 0, iPartIdx );
        pcCU->getCUMvField( REF_PIC_LIST_1 )->setAllRefIdx( iRefIdxBi[1], ePartSize, uiPartAddr, 0, iPartIdx );
        TComYuv* pcYuvPred = &m_acYuvPred[1];
        motionCompensation( pcCU, pcYuvPred, REF_PIC_LIST_1, iPartIdx );

        uiMotBits[0] = uiBits[0] - uiMbBits[0];
        uiMotBits[1] = uiMbBits[1];

        if ( pcCU->getSlice()->getNumRefIdx(REF_PIC_LIST_1) > 1 )
        {
          uiMotBits[1] += bestBiPRefIdxL1+1;
          if ( bestBiPRefIdxL1 == pcCU->getSlice()->getNumRefIdx(REF_PIC_LIST_1)-1 ) uiMotBits[1]--;
        }

#if HHI_INTER_VIEW_MOTION_PRED
        uiMotBits[1] += m_auiMVPIdxCost[aaiMvpIdxBi[1][bestBiPRefIdxL1]][iNumAMVPCands];
#else
        uiMotBits[1] += m_auiMVPIdxCost[aaiMvpIdxBi[1][bestBiPRefIdxL1]][AMVP_MAX_NUM_CANDS];
#endif

        uiBits[2] = uiMbBits[2] + uiMotBits[0] + uiMotBits[1];

        cMvTemp[1][bestBiPRefIdxL1] = cMvBi[1];
      }
      else
      {
        uiMotBits[0] = uiBits[0] - uiMbBits[0];
        uiMotBits[1] = uiBits[1] - uiMbBits[1];
        uiBits[2] = uiMbBits[2] + uiMotBits[0] + uiMotBits[1];
      }
#else    
      UInt uiMotBits[2] = { uiBits[0] - uiMbBits[0], uiBits[1] - uiMbBits[1] };
      uiBits[2] = uiMbBits[2] + uiMotBits[0] + uiMotBits[1];
#endif      

      // 4-times iteration (default)
      Int iNumIter = 4;
      
      // fast encoder setting: only one iteration
#if H0111_MVD_L1_ZERO
      if ( m_pcEncCfg->getUseFastEnc() || pcCU->getSlice()->getMvdL1ZeroFlag())
#else
      if ( m_pcEncCfg->getUseFastEnc() )
#endif
      {
        iNumIter = 1;
      }
      
      for ( Int iIter = 0; iIter < iNumIter; iIter++ )
      {
        
        Int         iRefList    = iIter % 2;
        if ( m_pcEncCfg->getUseFastEnc() && (pcCU->getSlice()->getNoBackPredFlag() || (pcCU->getSlice()->getNumRefIdx(REF_PIC_LIST_C) > 0 && pcCU->getSlice()->getRefIdxOfL0FromRefIdxOfL1(0)==0 )) )
        {
          iRefList = 1;
        }
        RefPicList  eRefPicList = ( iRefList ? REF_PIC_LIST_1 : REF_PIC_LIST_0 );

#if H0111_MVD_L1_ZERO
        if(pcCU->getSlice()->getMvdL1ZeroFlag())
        {
          iRefList = 0;
          eRefPicList = REF_PIC_LIST_0;
        }
#endif

        Bool bChanged = false;
        
        iRefStart = 0;
        iRefEnd   = pcCU->getSlice()->getNumRefIdx(eRefPicList)-1;
        
        for ( Int iRefIdxTemp = iRefStart; iRefIdxTemp <= iRefEnd; iRefIdxTemp++ )
        {
          uiBitsTemp = uiMbBits[2] + uiMotBits[1-iRefList];
          if ( pcCU->getSlice()->getNumRefIdx(eRefPicList) > 1 )
          {
            uiBitsTemp += iRefIdxTemp+1;
            if ( iRefIdxTemp == pcCU->getSlice()->getNumRefIdx(eRefPicList)-1 ) uiBitsTemp--;
          }
#if HHI_INTER_VIEW_MOTION_PRED
          uiBitsTemp += m_auiMVPIdxCost[aaiMvpIdxBi[iRefList][iRefIdxTemp]][iNumAMVPCands];
#else
          uiBitsTemp += m_auiMVPIdxCost[aaiMvpIdxBi[iRefList][iRefIdxTemp]][AMVP_MAX_NUM_CANDS];
#endif
#if LG_RESTRICTEDRESPRED_M24766
		  Int iAddResiShift = -1, iPredFrom = 0;
		  Int iBestRefIdx = pcCU->getCUMvField(eRefPicList == REF_PIC_LIST_0 ? REF_PIC_LIST_1 : REF_PIC_LIST_0)->getRefIdx(uiPartAddr);

		  iPredFrom = iBestRefIdx >= 0 ? 3 : 1;
		  if(iBestRefIdx >= 0 && pcCU->getSlice()->getViewId() == pcCU->getSlice()->getRefViewId(eRefPicList == REF_PIC_LIST_0 ? REF_PIC_LIST_1 : REF_PIC_LIST_0, iBestRefIdx))
			  iAddResiShift++;
		  if(pcCU->getSlice()->getViewId() == pcCU->getSlice()->getRefViewId(eRefPicList, iRefIdxTemp))
			  iAddResiShift++;
		  iAddResiShift = (pcCU->getSlice()->getPPS()->getUseWP() || iPredFrom != 3) ? (iAddResiShift >= 0 ? 0 : -1) : (iAddResiShift >= 0 ? 1-iAddResiShift : -1);

		  if( pcCU->getResPredFlag( 0 ) )
		  {
			  if(iLastAddResiShift != iAddResiShift)
			  {
				  //add substracted residual last time
				  if(iLastAddResiShift >= 0 )
				  {
					  iPUResiPredShift[0] = iPUResiPredShift[1] = iPUResiPredShift[2] = iPUResiPredShift[3] = iLastAddResiShift;
					  pcOrgYuv->add(iPUResiPredShift, pcCU->getPartitionSize(0), rpcResiPredYuv, pcCU->getWidth( 0 ), pcCU->getHeight( 0 ));
				  }
				  //substract residual
				  if(iAddResiShift >= 0)
				  {
					  iPUResiPredShift[0] = iPUResiPredShift[1] = iPUResiPredShift[2] = iPUResiPredShift[3] = iAddResiShift;
					  pcOrgYuv->add(iPUResiPredShift, pcCU->getPartitionSize(0), rpcResiPredYuv, pcCU->getWidth( 0 ), pcCU->getHeight( 0 ), true );
				  }
				  iLastAddResiShift = iAddResiShift;
			  }
		  }
#endif
          // call ME
          xMotionEstimation ( pcCU, pcOrgYuv, iPartIdx, eRefPicList, &cMvPredBi[iRefList][iRefIdxTemp], iRefIdxTemp, cMvTemp[iRefList][iRefIdxTemp], uiBitsTemp, uiCostTemp, true );
          if ( pcCU->getAMVPMode(uiPartAddr) == AM_EXPL )
          {
            xCopyAMVPInfo(&aacAMVPInfo[iRefList][iRefIdxTemp], pcCU->getCUMvField(eRefPicList)->getAMVPInfo());
            xCheckBestMVP(pcCU, eRefPicList, cMvTemp[iRefList][iRefIdxTemp], cMvPredBi[iRefList][iRefIdxTemp], aaiMvpIdxBi[iRefList][iRefIdxTemp], uiBitsTemp, uiCostTemp);
          }
          
          if ( uiCostTemp < uiCostBi )
          {
            bChanged = true;
            
            cMvBi[iRefList]     = cMvTemp[iRefList][iRefIdxTemp];
            iRefIdxBi[iRefList] = iRefIdxTemp;
            
            uiCostBi            = uiCostTemp;
            uiMotBits[iRefList] = uiBitsTemp - uiMbBits[2] - uiMotBits[1-iRefList];
            uiBits[2]           = uiBitsTemp;
            
#if H0111_MVD_L1_ZERO
            if(iNumIter!=1)
            {
#endif
              //  Set motion
              pcCU->getCUMvField( eRefPicList )->setAllMv( cMvBi[iRefList], ePartSize, uiPartAddr, 0, iPartIdx );
              pcCU->getCUMvField( eRefPicList )->setAllRefIdx( iRefIdxBi[iRefList], ePartSize, uiPartAddr, 0, iPartIdx );

              TComYuv* pcYuvPred = &m_acYuvPred[iRefList];
              motionCompensation( pcCU, pcYuvPred, eRefPicList, iPartIdx );
#if H0111_MVD_L1_ZERO
            }
#endif
          }
        } // for loop-iRefIdxTemp
        
        if ( !bChanged )
        {
          if ( uiCostBi <= uiCost[0] && uiCostBi <= uiCost[1] && pcCU->getAMVPMode(uiPartAddr) == AM_EXPL )
          {
            xCopyAMVPInfo(&aacAMVPInfo[0][iRefIdxBi[0]], pcCU->getCUMvField(REF_PIC_LIST_0)->getAMVPInfo());
            xCheckBestMVP(pcCU, REF_PIC_LIST_0, cMvBi[0], cMvPredBi[0][iRefIdxBi[0]], aaiMvpIdxBi[0][iRefIdxBi[0]], uiBits[2], uiCostBi);
#if H0111_MVD_L1_ZERO
            if(!pcCU->getSlice()->getMvdL1ZeroFlag())
            {
#endif
              xCopyAMVPInfo(&aacAMVPInfo[1][iRefIdxBi[1]], pcCU->getCUMvField(REF_PIC_LIST_1)->getAMVPInfo());
              xCheckBestMVP(pcCU, REF_PIC_LIST_1, cMvBi[1], cMvPredBi[1][iRefIdxBi[1]], aaiMvpIdxBi[1][iRefIdxBi[1]], uiBits[2], uiCostBi);
#if H0111_MVD_L1_ZERO
            }
#endif
          }
          break;
        }
      } // for loop-iter
#if LG_RESTRICTEDRESPRED_M24766
	  if( pcCU->getResPredFlag( 0 ) && iLastAddResiShift >= 0)
	  {
		  iPUResiPredShift[0] = iPUResiPredShift[1] = iPUResiPredShift[2] = iPUResiPredShift[3] = iLastAddResiShift;
		  pcOrgYuv->add(iPUResiPredShift, pcCU->getPartitionSize(0), rpcResiPredYuv, pcCU->getWidth( 0 ), pcCU->getHeight( 0 ));
	  }
#endif
    } // if (B_SLICE)
#if ZERO_MVD_EST
    if ( pcCU->getSlice()->isInterB() )
    {
      m_pcRdCost->getMotionCost( 1, 0 );

      for ( Int iL0RefIdxTemp = 0; iL0RefIdxTemp <= pcCU->getSlice()->getNumRefIdx(REF_PIC_LIST_0)-1; iL0RefIdxTemp++ )
      for ( Int iL1RefIdxTemp = 0; iL1RefIdxTemp <= pcCU->getSlice()->getNumRefIdx(REF_PIC_LIST_1)-1; iL1RefIdxTemp++ )
      {
        UInt uiRefIdxBitsTemp = 0;
        if ( pcCU->getSlice()->getNumRefIdx(REF_PIC_LIST_0) > 1 )
        {
          uiRefIdxBitsTemp += iL0RefIdxTemp+1;
          if ( iL0RefIdxTemp == pcCU->getSlice()->getNumRefIdx(REF_PIC_LIST_0)-1 ) uiRefIdxBitsTemp--;
        }
        if ( pcCU->getSlice()->getNumRefIdx(REF_PIC_LIST_1) > 1 )
        {
          uiRefIdxBitsTemp += iL1RefIdxTemp+1;
          if ( iL1RefIdxTemp == pcCU->getSlice()->getNumRefIdx(REF_PIC_LIST_1)-1 ) uiRefIdxBitsTemp--;
        }

        Int iL0MVPIdx = 0;
        Int iL1MVPIdx = 0;

        for (iL0MVPIdx = 0; iL0MVPIdx < aaiMvpNum[0][iL0RefIdxTemp]; iL0MVPIdx++)
        {
          for (iL1MVPIdx = 0; iL1MVPIdx < aaiMvpNum[1][iL1RefIdxTemp]; iL1MVPIdx++)
          {
            uiZeroMvdBitsTemp = uiRefIdxBitsTemp;
            uiZeroMvdBitsTemp += uiMbBits[2];
            uiZeroMvdBitsTemp += m_auiMVPIdxCost[iL0MVPIdx][aaiMvpNum[0][iL0RefIdxTemp]] + m_auiMVPIdxCost[iL1MVPIdx][aaiMvpNum[1][iL1RefIdxTemp]];
            uiZeroMvdBitsTemp += 4; //zero mvd for both directions
            pcCU->getCUMvField( REF_PIC_LIST_0 )->setAllMvField( aacAMVPInfo[0][iL0RefIdxTemp].m_acMvCand[iL0MVPIdx], iL0RefIdxTemp, ePartSize, uiPartAddr, iPartIdx, 0 );
            pcCU->getCUMvField( REF_PIC_LIST_1 )->setAllMvField( aacAMVPInfo[1][iL1RefIdxTemp].m_acMvCand[iL1MVPIdx], iL1RefIdxTemp, ePartSize, uiPartAddr, iPartIdx, 0 );
  
            xGetInterPredictionError( pcCU, pcOrgYuv, iPartIdx, uiZeroMvdDistTemp, m_pcEncCfg->getUseHADME() );
            uiZeroMvdCostTemp = uiZeroMvdDistTemp + m_pcRdCost->getCost( uiZeroMvdBitsTemp );
            if (uiZeroMvdCostTemp < uiZeroMvdCost)
            {
              uiZeroMvdCost = uiZeroMvdCostTemp;
              iZeroMvdDir = 3;
              aiZeroMvdMvpIdx[0] = iL0MVPIdx;
              aiZeroMvdMvpIdx[1] = iL1MVPIdx;
              aiZeroMvdRefIdx[0] = iL0RefIdxTemp;
              aiZeroMvdRefIdx[1] = iL1RefIdxTemp;
              auiZeroMvdBits[2] = uiZeroMvdBitsTemp;
            }
          }
        }
      }
    }
#endif

#if AMP_MRG
    } //end if bTestNormalMC
#endif
    //  Clear Motion Field
    pcCU->getCUMvField(REF_PIC_LIST_0)->setAllMvField( TComMvField(), ePartSize, uiPartAddr, 0, iPartIdx );
    pcCU->getCUMvField(REF_PIC_LIST_1)->setAllMvField( TComMvField(), ePartSize, uiPartAddr, 0, iPartIdx );
    pcCU->getCUMvField(REF_PIC_LIST_0)->setAllMvd    ( cMvZero,       ePartSize, uiPartAddr, 0, iPartIdx );
    pcCU->getCUMvField(REF_PIC_LIST_1)->setAllMvd    ( cMvZero,       ePartSize, uiPartAddr, 0, iPartIdx );

    pcCU->setMVPIdxSubParts( -1, REF_PIC_LIST_0, uiPartAddr, iPartIdx, pcCU->getDepth(uiPartAddr));
    pcCU->setMVPNumSubParts( -1, REF_PIC_LIST_0, uiPartAddr, iPartIdx, pcCU->getDepth(uiPartAddr));
    pcCU->setMVPIdxSubParts( -1, REF_PIC_LIST_1, uiPartAddr, iPartIdx, pcCU->getDepth(uiPartAddr));
    pcCU->setMVPNumSubParts( -1, REF_PIC_LIST_1, uiPartAddr, iPartIdx, pcCU->getDepth(uiPartAddr));
    
    UInt uiMEBits = 0;
    // Set Motion Field_
    if ( pcCU->getSlice()->getNoBackPredFlag() || (pcCU->getSlice()->getNumRefIdx(REF_PIC_LIST_C) > 0 && pcCU->getSlice()->getRefIdxOfL0FromRefIdxOfL1(0)==0 ) )
    {
      uiCost[1] = MAX_UINT;
    }
#if AMP_MRG
    if (bTestNormalMC)
    {
#endif
#if ZERO_MVD_EST
    if (uiZeroMvdCost <= uiCostBi && uiZeroMvdCost <= uiCost[0] && uiZeroMvdCost <= uiCost[1])
    {
      if (iZeroMvdDir == 3)
      {
        uiLastMode = 2;

        pcCU->getCUMvField(REF_PIC_LIST_0)->setAllMvField( aacAMVPInfo[0][aiZeroMvdRefIdx[0]].m_acMvCand[aiZeroMvdMvpIdx[0]], aiZeroMvdRefIdx[0], ePartSize, uiPartAddr, iPartIdx, 0 );
        pcCU->getCUMvField(REF_PIC_LIST_1)->setAllMvField( aacAMVPInfo[1][aiZeroMvdRefIdx[1]].m_acMvCand[aiZeroMvdMvpIdx[1]], aiZeroMvdRefIdx[1], ePartSize, uiPartAddr, iPartIdx, 0 );
  
        pcCU->setInterDirSubParts( 3, uiPartAddr, iPartIdx, pcCU->getDepth(0) );
        
        pcCU->setMVPIdxSubParts( aiZeroMvdMvpIdx[0], REF_PIC_LIST_0, uiPartAddr, iPartIdx, pcCU->getDepth(uiPartAddr));
        pcCU->setMVPNumSubParts( aaiMvpNum[0][aiZeroMvdRefIdx[0]], REF_PIC_LIST_0, uiPartAddr, iPartIdx, pcCU->getDepth(uiPartAddr));
        pcCU->setMVPIdxSubParts( aiZeroMvdMvpIdx[1], REF_PIC_LIST_1, uiPartAddr, iPartIdx, pcCU->getDepth(uiPartAddr));
        pcCU->setMVPNumSubParts( aaiMvpNum[1][aiZeroMvdRefIdx[1]], REF_PIC_LIST_1, uiPartAddr, iPartIdx, pcCU->getDepth(uiPartAddr));
        uiMEBits = auiZeroMvdBits[2];
      }
      else if (iZeroMvdDir == 1)
      {        
        uiLastMode = 0;

        pcCU->getCUMvField(REF_PIC_LIST_0)->setAllMvField( aacAMVPInfo[0][aiZeroMvdRefIdx[0]].m_acMvCand[aiZeroMvdMvpIdx[0]], aiZeroMvdRefIdx[0], ePartSize, uiPartAddr, iPartIdx, 0 );

        pcCU->setInterDirSubParts( 1, uiPartAddr, iPartIdx, pcCU->getDepth(0) );
        
        pcCU->setMVPIdxSubParts( aiZeroMvdMvpIdx[0], REF_PIC_LIST_0, uiPartAddr, iPartIdx, pcCU->getDepth(uiPartAddr));
        pcCU->setMVPNumSubParts( aaiMvpNum[0][aiZeroMvdRefIdx[0]], REF_PIC_LIST_0, uiPartAddr, iPartIdx, pcCU->getDepth(uiPartAddr));
        uiMEBits = auiZeroMvdBits[0];
      }
      else if (iZeroMvdDir == 2)
      {
        uiLastMode = 1;

        pcCU->getCUMvField(REF_PIC_LIST_1)->setAllMvField( aacAMVPInfo[1][aiZeroMvdRefIdx[1]].m_acMvCand[aiZeroMvdMvpIdx[1]], aiZeroMvdRefIdx[1], ePartSize, uiPartAddr, iPartIdx, 0 );

        pcCU->setInterDirSubParts( 2, uiPartAddr, iPartIdx, pcCU->getDepth(0) );
        
        pcCU->setMVPIdxSubParts( aiZeroMvdMvpIdx[1], REF_PIC_LIST_1, uiPartAddr, iPartIdx, pcCU->getDepth(uiPartAddr));
        pcCU->setMVPNumSubParts( aaiMvpNum[1][aiZeroMvdRefIdx[1]], REF_PIC_LIST_1, uiPartAddr, iPartIdx, pcCU->getDepth(uiPartAddr));
        uiMEBits = auiZeroMvdBits[1];
      }
      else
      {
        assert(0);
      }
    }
    else
#endif
    if ( uiCostBi <= uiCost[0] && uiCostBi <= uiCost[1])
    {
      uiLastMode = 2;
      {
            pcCU->getCUMvField(REF_PIC_LIST_0)->setAllMv( cMvBi[0], ePartSize, uiPartAddr, 0, iPartIdx );
            pcCU->getCUMvField(REF_PIC_LIST_0)->setAllRefIdx( iRefIdxBi[0], ePartSize, uiPartAddr, 0, iPartIdx );
            pcCU->getCUMvField(REF_PIC_LIST_1)->setAllMv( cMvBi[1], ePartSize, uiPartAddr, 0, iPartIdx );
            pcCU->getCUMvField(REF_PIC_LIST_1)->setAllRefIdx( iRefIdxBi[1], ePartSize, uiPartAddr, 0, iPartIdx );
      }
      {
        TempMv = cMvBi[0] - cMvPredBi[0][iRefIdxBi[0]];
            pcCU->getCUMvField(REF_PIC_LIST_0)->setAllMvd    ( TempMv,                 ePartSize, uiPartAddr, 0, iPartIdx );
      }
      {
        TempMv = cMvBi[1] - cMvPredBi[1][iRefIdxBi[1]];
            pcCU->getCUMvField(REF_PIC_LIST_1)->setAllMvd    ( TempMv,                 ePartSize, uiPartAddr, 0, iPartIdx );
      }
      
      pcCU->setInterDirSubParts( 3, uiPartAddr, iPartIdx, pcCU->getDepth(0) );
      
      pcCU->setMVPIdxSubParts( aaiMvpIdxBi[0][iRefIdxBi[0]], REF_PIC_LIST_0, uiPartAddr, iPartIdx, pcCU->getDepth(uiPartAddr));
      pcCU->setMVPNumSubParts( aaiMvpNum[0][iRefIdxBi[0]], REF_PIC_LIST_0, uiPartAddr, iPartIdx, pcCU->getDepth(uiPartAddr));
      pcCU->setMVPIdxSubParts( aaiMvpIdxBi[1][iRefIdxBi[1]], REF_PIC_LIST_1, uiPartAddr, iPartIdx, pcCU->getDepth(uiPartAddr));
      pcCU->setMVPNumSubParts( aaiMvpNum[1][iRefIdxBi[1]], REF_PIC_LIST_1, uiPartAddr, iPartIdx, pcCU->getDepth(uiPartAddr));

      uiMEBits = uiBits[2];
    }
    else if ( uiCost[0] <= uiCost[1] )
    {
      uiLastMode = 0;
          pcCU->getCUMvField(REF_PIC_LIST_0)->setAllMv( cMv[0], ePartSize, uiPartAddr, 0, iPartIdx );
          pcCU->getCUMvField(REF_PIC_LIST_0)->setAllRefIdx( iRefIdx[0], ePartSize, uiPartAddr, 0, iPartIdx );
      {
        TempMv = cMv[0] - cMvPred[0][iRefIdx[0]];
            pcCU->getCUMvField(REF_PIC_LIST_0)->setAllMvd    ( TempMv,                 ePartSize, uiPartAddr, 0, iPartIdx );
      }
      pcCU->setInterDirSubParts( 1, uiPartAddr, iPartIdx, pcCU->getDepth(0) );
      
      pcCU->setMVPIdxSubParts( aaiMvpIdx[0][iRefIdx[0]], REF_PIC_LIST_0, uiPartAddr, iPartIdx, pcCU->getDepth(uiPartAddr));
      pcCU->setMVPNumSubParts( aaiMvpNum[0][iRefIdx[0]], REF_PIC_LIST_0, uiPartAddr, iPartIdx, pcCU->getDepth(uiPartAddr));

      uiMEBits = uiBits[0];
    }
    else
    {
      uiLastMode = 1;
          pcCU->getCUMvField(REF_PIC_LIST_1)->setAllMv( cMv[1], ePartSize, uiPartAddr, 0, iPartIdx );
          pcCU->getCUMvField(REF_PIC_LIST_1)->setAllRefIdx( iRefIdx[1], ePartSize, uiPartAddr, 0, iPartIdx );
      {
        TempMv = cMv[1] - cMvPred[1][iRefIdx[1]];
            pcCU->getCUMvField(REF_PIC_LIST_1)->setAllMvd    ( TempMv,                 ePartSize, uiPartAddr, 0, iPartIdx );
      }
      pcCU->setInterDirSubParts( 2, uiPartAddr, iPartIdx, pcCU->getDepth(0) );
      
      pcCU->setMVPIdxSubParts( aaiMvpIdx[1][iRefIdx[1]], REF_PIC_LIST_1, uiPartAddr, iPartIdx, pcCU->getDepth(uiPartAddr));
      pcCU->setMVPNumSubParts( aaiMvpNum[1][iRefIdx[1]], REF_PIC_LIST_1, uiPartAddr, iPartIdx, pcCU->getDepth(uiPartAddr));

      uiMEBits = uiBits[1];
    }
#if AMP_MRG
    } // end if bTestNormalMC
#endif

    if ( pcCU->getPartitionSize( uiPartAddr ) != SIZE_2Nx2N )
    {
      UInt uiMRGInterDir = 0;     
      TComMvField cMRGMvField[2];
      UInt uiMRGIndex = 0;

      UInt uiMEInterDir = 0;
      TComMvField cMEMvField[2];

      m_pcRdCost->getMotionCost( 1, 0 );
#if AMP_MRG
      // calculate ME cost
      UInt uiMEError = MAX_UINT;
      UInt uiMECost = MAX_UINT;

      if (bTestNormalMC)
      {
#if LG_RESTRICTEDRESPRED_M24766
		  Int iAddResiShift = pcCU->getResiPredMode(uiPartAddr);
		  iPUResiPredShift[0] = iPUResiPredShift[1] = iPUResiPredShift[2] = iPUResiPredShift[3] = \
			  (pcCU->getSlice()->getPPS()->getUseWP() || pcCU->getInterDir(uiPartAddr) != 3)? (iAddResiShift >= 0 ? 0 : -1) : (iAddResiShift >= 0 ? 1-iAddResiShift : -1);
		  if(pcCU->getResPredFlag(0) && iAddResiShift >= 0)
		  {
			  pcOrgYuv->add(iPUResiPredShift, pcCU->getPartitionSize(0), rpcResiPredYuv, pcCU->getWidth( 0 ), pcCU->getHeight( 0 ), true);
		  }
#endif
        xGetInterPredictionError( pcCU, pcOrgYuv, iPartIdx, uiMEError, m_pcEncCfg->getUseHADME() );
        uiMECost = uiMEError + m_pcRdCost->getCost( uiMEBits );
#if LG_RESTRICTEDRESPRED_M24766
		if(pcCU->getResPredFlag(0) && iAddResiShift >= 0)
		{
			pcOrgYuv->add(iPUResiPredShift, pcCU->getPartitionSize(0), rpcResiPredYuv, pcCU->getWidth( 0 ), pcCU->getHeight( 0 ));
		}
#endif
      }
#else
      // calculate ME cost
      UInt uiMEError = MAX_UINT;
      xGetInterPredictionError( pcCU, pcOrgYuv, iPartIdx, uiMEError, m_pcEncCfg->getUseHADME() );
      UInt uiMECost = uiMEError + m_pcRdCost->getCost( uiMEBits );
#endif 
      // save ME result.
      uiMEInterDir = pcCU->getInterDir( uiPartAddr );
      pcCU->getMvField( pcCU, uiPartAddr, REF_PIC_LIST_0, cMEMvField[0] );
      pcCU->getMvField( pcCU, uiPartAddr, REF_PIC_LIST_1, cMEMvField[1] );

      // find Merge result
      UInt uiMRGCost = MAX_UINT;
#if CU_BASED_MRG_CAND_LIST
      xMergeEstimation( pcCU, pcOrgYuv, iPartIdx, uiMRGInterDir, cMRGMvField, uiMRGIndex, uiMRGCost, cMvFieldNeighbours, uhInterDirNeighbours, numValidMergeCand);
#else
#if LG_RESTRICTEDRESPRED_M24766
      xMergeEstimation( pcCU, pcOrgYuv, rpcResiPredYuv, iPartIdx, uiMRGInterDir, cMRGMvField, uiMRGIndex, uiMRGCost );
#else
	  xMergeEstimation( pcCU, pcOrgYuv, iPartIdx, uiMRGInterDir, cMRGMvField, uiMRGIndex, uiMRGCost );
#endif
#endif
      if ( uiMRGCost < uiMECost )
      {
        // set Merge result
        pcCU->setMergeFlagSubParts ( true,          uiPartAddr, iPartIdx, pcCU->getDepth( uiPartAddr ) );
        pcCU->setMergeIndexSubParts( uiMRGIndex,    uiPartAddr, iPartIdx, pcCU->getDepth( uiPartAddr ) );
        pcCU->setInterDirSubParts  ( uiMRGInterDir, uiPartAddr, iPartIdx, pcCU->getDepth( uiPartAddr ) );
        {
          pcCU->getCUMvField( REF_PIC_LIST_0 )->setAllMvField( cMRGMvField[0], ePartSize, uiPartAddr, 0, iPartIdx );
          pcCU->getCUMvField( REF_PIC_LIST_1 )->setAllMvField( cMRGMvField[1], ePartSize, uiPartAddr, 0, iPartIdx );
        }

        pcCU->getCUMvField(REF_PIC_LIST_0)->setAllMvd    ( cMvZero,            ePartSize, uiPartAddr, 0, iPartIdx );
        pcCU->getCUMvField(REF_PIC_LIST_1)->setAllMvd    ( cMvZero,            ePartSize, uiPartAddr, 0, iPartIdx );

        pcCU->setMVPIdxSubParts( -1, REF_PIC_LIST_0, uiPartAddr, iPartIdx, pcCU->getDepth(uiPartAddr));
        pcCU->setMVPNumSubParts( -1, REF_PIC_LIST_0, uiPartAddr, iPartIdx, pcCU->getDepth(uiPartAddr));
        pcCU->setMVPIdxSubParts( -1, REF_PIC_LIST_1, uiPartAddr, iPartIdx, pcCU->getDepth(uiPartAddr));
        pcCU->setMVPNumSubParts( -1, REF_PIC_LIST_1, uiPartAddr, iPartIdx, pcCU->getDepth(uiPartAddr));
      }
      else
      {
        // set ME result
        pcCU->setMergeFlagSubParts( false,        uiPartAddr, iPartIdx, pcCU->getDepth( uiPartAddr ) );
        pcCU->setInterDirSubParts ( uiMEInterDir, uiPartAddr, iPartIdx, pcCU->getDepth( uiPartAddr ) );
        {
          pcCU->getCUMvField( REF_PIC_LIST_0 )->setAllMvField( cMEMvField[0], ePartSize, uiPartAddr, 0, iPartIdx );
          pcCU->getCUMvField( REF_PIC_LIST_1 )->setAllMvField( cMEMvField[1], ePartSize, uiPartAddr, 0, iPartIdx );
        }
      }
    }

    //  MC
    motionCompensation ( pcCU, rpcPredYuv, REF_PIC_LIST_X, iPartIdx );
    
  } //  end of for ( Int iPartIdx = 0; iPartIdx < iNumPart; iPartIdx++ )

  setWpScalingDistParam( pcCU, -1, REF_PIC_LIST_X );

  return;
}

// AMVP
#if H0111_MVD_L1_ZERO
#if ZERO_MVD_EST
Void TEncSearch::xEstimateMvPredAMVP( TComDataCU* pcCU, TComYuv* pcOrgYuv, UInt uiPartIdx, RefPicList eRefPicList, Int iRefIdx, TComMv& rcMvPred, Bool bFilled, UInt* puiDistBiP, UInt* puiDist  )
#else
Void TEncSearch::xEstimateMvPredAMVP( TComDataCU* pcCU, TComYuv* pcOrgYuv, UInt uiPartIdx, RefPicList eRefPicList, Int iRefIdx, TComMv& rcMvPred, Bool bFilled, UInt* puiDistBiP )
#endif
#else
#if ZERO_MVD_EST
Void TEncSearch::xEstimateMvPredAMVP( TComDataCU* pcCU, TComYuv* pcOrgYuv, UInt uiPartIdx, RefPicList eRefPicList, Int iRefIdx, TComMv& rcMvPred, Bool bFilled, UInt* puiDist )
#else
Void TEncSearch::xEstimateMvPredAMVP( TComDataCU* pcCU, TComYuv* pcOrgYuv, UInt uiPartIdx, RefPicList eRefPicList, Int iRefIdx, TComMv& rcMvPred, Bool bFilled )
#endif
#endif
{
  AMVPInfo* pcAMVPInfo = pcCU->getCUMvField(eRefPicList)->getAMVPInfo();
  
  TComMv  cBestMv;
  Int     iBestIdx = 0;
  TComMv  cZeroMv;
  TComMv  cMvPred;
  UInt    uiBestCost = MAX_INT;
  UInt    uiPartAddr = 0;
  Int     iRoiWidth, iRoiHeight;
  Int     i;
  
  pcCU->getPartIndexAndSize( uiPartIdx, uiPartAddr, iRoiWidth, iRoiHeight );
  // Fill the MV Candidates
  if (!bFilled)
  {
    pcCU->fillMvpCand( uiPartIdx, uiPartAddr, eRefPicList, iRefIdx, pcAMVPInfo );
  }
  
  // initialize Mvp index & Mvp
  iBestIdx = 0;
  cBestMv  = pcAMVPInfo->m_acMvCand[0];
#if !ZERO_MVD_EST
  if( pcCU->getAMVPMode(uiPartAddr) == AM_NONE || (pcAMVPInfo->iN <= 1 && pcCU->getAMVPMode(uiPartAddr) == AM_EXPL) )
  {
    rcMvPred = cBestMv;
    
    pcCU->setMVPIdxSubParts( iBestIdx, eRefPicList, uiPartAddr, uiPartIdx, pcCU->getDepth(uiPartAddr));
    pcCU->setMVPNumSubParts( pcAMVPInfo->iN, eRefPicList, uiPartAddr, uiPartIdx, pcCU->getDepth(uiPartAddr));

#if H0111_MVD_L1_ZERO
    if(pcCU->getSlice()->getMvdL1ZeroFlag() && eRefPicList==REF_PIC_LIST_1)
    {
#if HHI_INTER_VIEW_MOTION_PRED
      Int iNumAMVPCands = AMVP_MAX_NUM_CANDS + ( pcCU->getSlice()->getSPS()->getMultiviewMvPredMode() ? 1 : 0 );
#if ZERO_MVD_EST
      (*puiDistBiP) = xGetTemplateCost( pcCU, uiPartIdx, uiPartAddr, pcOrgYuv, &m_cYuvPredTemp, rcMvPred, 0, iNumAMVPCands, eRefPicList, iRefIdx, iRoiWidth, iRoiHeight, uiDist );
#else
      (*puiDistBiP) = xGetTemplateCost( pcCU, uiPartIdx, uiPartAddr, pcOrgYuv, &m_cYuvPredTemp, rcMvPred, 0, iNumAMVPCands, eRefPicList, iRefIdx, iRoiWidth, iRoiHeight);
#endif
#else
#if ZERO_MVD_EST
      (*puiDistBiP) = xGetTemplateCost( pcCU, uiPartIdx, uiPartAddr, pcOrgYuv, &m_cYuvPredTemp, rcMvPred, 0, AMVP_MAX_NUM_CANDS, eRefPicList, iRefIdx, iRoiWidth, iRoiHeight, uiDist );
#else
      (*puiDistBiP) = xGetTemplateCost( pcCU, uiPartIdx, uiPartAddr, pcOrgYuv, &m_cYuvPredTemp, rcMvPred, 0, AMVP_MAX_NUM_CANDS, eRefPicList, iRefIdx, iRoiWidth, iRoiHeight);
#endif
#endif
    }
#endif
    return;
  }
#endif  
  if (pcCU->getAMVPMode(uiPartAddr) == AM_EXPL && bFilled)
  {
    assert(pcCU->getMVPIdx(eRefPicList,uiPartAddr) >= 0);
    rcMvPred = pcAMVPInfo->m_acMvCand[pcCU->getMVPIdx(eRefPicList,uiPartAddr)];
    return;
  }
  
  if (pcCU->getAMVPMode(uiPartAddr) == AM_EXPL)
  {
    m_cYuvPredTemp.clear();
#if ZERO_MVD_EST
    UInt uiDist;
#endif
    //-- Check Minimum Cost.
    for ( i = 0 ; i < pcAMVPInfo->iN; i++)
    {
      UInt uiTmpCost;
#if HHI_INTER_VIEW_MOTION_PRED
      Int iNumAMVPCands = AMVP_MAX_NUM_CANDS + ( pcCU->getSlice()->getSPS()->getMultiviewMvPredMode() ? 1 : 0 );
#if ZERO_MVD_EST
      uiTmpCost = xGetTemplateCost( pcCU, uiPartIdx, uiPartAddr, pcOrgYuv, &m_cYuvPredTemp, pcAMVPInfo->m_acMvCand[i], i, iNumAMVPCands, eRefPicList, iRefIdx, iRoiWidth, iRoiHeight, uiDist );
#else
      uiTmpCost = xGetTemplateCost( pcCU, uiPartIdx, uiPartAddr, pcOrgYuv, &m_cYuvPredTemp, pcAMVPInfo->m_acMvCand[i], i, iNumAMVPCands, eRefPicList, iRefIdx, iRoiWidth, iRoiHeight);
#endif
#else
#if ZERO_MVD_EST
      uiTmpCost = xGetTemplateCost( pcCU, uiPartIdx, uiPartAddr, pcOrgYuv, &m_cYuvPredTemp, pcAMVPInfo->m_acMvCand[i], i, AMVP_MAX_NUM_CANDS, eRefPicList, iRefIdx, iRoiWidth, iRoiHeight, uiDist );
#else
      uiTmpCost = xGetTemplateCost( pcCU, uiPartIdx, uiPartAddr, pcOrgYuv, &m_cYuvPredTemp, pcAMVPInfo->m_acMvCand[i], i, AMVP_MAX_NUM_CANDS, eRefPicList, iRefIdx, iRoiWidth, iRoiHeight);
#endif      
#endif
      if ( uiBestCost > uiTmpCost )
      {
        uiBestCost = uiTmpCost;
        cBestMv   = pcAMVPInfo->m_acMvCand[i];
        iBestIdx  = i;
        #if H0111_MVD_L1_ZERO
        (*puiDistBiP) = uiTmpCost;
        #endif
        #if ZERO_MVD_EST
        (*puiDist) = uiDist;
        #endif
      }
    }
    
    m_cYuvPredTemp.clear();
  }
  
  // Setting Best MVP
  rcMvPred = cBestMv;
  pcCU->setMVPIdxSubParts( iBestIdx, eRefPicList, uiPartAddr, uiPartIdx, pcCU->getDepth(uiPartAddr));
  pcCU->setMVPNumSubParts( pcAMVPInfo->iN, eRefPicList, uiPartAddr, uiPartIdx, pcCU->getDepth(uiPartAddr));
  return;
}

UInt TEncSearch::xGetMvpIdxBits(Int iIdx, Int iNum)
{
  assert(iIdx >= 0 && iNum >= 0 && iIdx < iNum);
  
  if (iNum == 1)
    return 0;
  
  UInt uiLength = 1;
  Int iTemp = iIdx;
  if ( iTemp == 0 )
  {
    return uiLength;
  }
  
  Bool bCodeLast = ( iNum-1 > iTemp );
  
  uiLength += (iTemp-1);
  
  if( bCodeLast )
  {
    uiLength++;
  }
  
  return uiLength;
}

Void TEncSearch::xGetBlkBits( PartSize eCUMode, Bool bPSlice, Int iPartIdx, UInt uiLastMode, UInt uiBlkBit[3])
{
  if ( eCUMode == SIZE_2Nx2N )
  {
    uiBlkBit[0] = (! bPSlice) ? 3 : 1;
    uiBlkBit[1] = 3;
    uiBlkBit[2] = 5;
  }
  else if ( (eCUMode == SIZE_2NxN || eCUMode == SIZE_2NxnU) || eCUMode == SIZE_2NxnD )
  {
    UInt aauiMbBits[2][3][3] = { { {0,0,3}, {0,0,0}, {0,0,0} } , { {5,7,7}, {7,5,7}, {9-3,9-3,9-3} } };
    if ( bPSlice )
    {
      uiBlkBit[0] = 3;
      uiBlkBit[1] = 0;
      uiBlkBit[2] = 0;
    }
    else
    {
      ::memcpy( uiBlkBit, aauiMbBits[iPartIdx][uiLastMode], 3*sizeof(UInt) );
    }
  }
  else if ( (eCUMode == SIZE_Nx2N || eCUMode == SIZE_nLx2N) || eCUMode == SIZE_nRx2N )
  {
    UInt aauiMbBits[2][3][3] = { { {0,2,3}, {0,0,0}, {0,0,0} } , { {5,7,7}, {7-2,7-2,9-2}, {9-3,9-3,9-3} } };
    if ( bPSlice )
    {
      uiBlkBit[0] = 3;
      uiBlkBit[1] = 0;
      uiBlkBit[2] = 0;
    }
    else
    {
      ::memcpy( uiBlkBit, aauiMbBits[iPartIdx][uiLastMode], 3*sizeof(UInt) );
    }
  }
  else if ( eCUMode == SIZE_NxN )
  {
    uiBlkBit[0] = (! bPSlice) ? 3 : 1;
    uiBlkBit[1] = 3;
    uiBlkBit[2] = 5;
  }
  else
  {
    printf("Wrong!\n");
    assert( 0 );
  }
}

Void TEncSearch::xCopyAMVPInfo (AMVPInfo* pSrc, AMVPInfo* pDst)
{
  pDst->iN = pSrc->iN;
  for (Int i = 0; i < pSrc->iN; i++)
  {
    pDst->m_acMvCand[i] = pSrc->m_acMvCand[i];
  }
}

Void TEncSearch::xCheckBestMVP ( TComDataCU* pcCU, RefPicList eRefPicList, TComMv cMv, TComMv& rcMvPred, Int& riMVPIdx, UInt& ruiBits, UInt& ruiCost )
{
  AMVPInfo* pcAMVPInfo = pcCU->getCUMvField(eRefPicList)->getAMVPInfo();
  
  assert(pcAMVPInfo->m_acMvCand[riMVPIdx] == rcMvPred);
  
  if (pcAMVPInfo->iN < 2) return;
  
  m_pcRdCost->getMotionCost( 1, 0 );
  m_pcRdCost->setCostScale ( 0    );
  
  Int iBestMVPIdx = riMVPIdx;
  
#if HHI_INTER_VIEW_MOTION_PRED
  Int iNumAMVPCands = AMVP_MAX_NUM_CANDS + ( pcCU->getSlice()->getSPS()->getMultiviewMvPredMode() ? 1 : 0 );
#endif
  
  m_pcRdCost->setPredictor( rcMvPred );
  Int iOrgMvBits  = m_pcRdCost->getBits(cMv.getHor(), cMv.getVer());
#if HHI_INTER_VIEW_MOTION_PRED
  iOrgMvBits += m_auiMVPIdxCost[riMVPIdx][iNumAMVPCands];
#else
  iOrgMvBits += m_auiMVPIdxCost[riMVPIdx][AMVP_MAX_NUM_CANDS];
#endif
  Int iBestMvBits = iOrgMvBits;
  
  for (Int iMVPIdx = 0; iMVPIdx < pcAMVPInfo->iN; iMVPIdx++)
  {
    if (iMVPIdx == riMVPIdx) continue;
    
    m_pcRdCost->setPredictor( pcAMVPInfo->m_acMvCand[iMVPIdx] );
    
    Int iMvBits = m_pcRdCost->getBits(cMv.getHor(), cMv.getVer());
#if HHI_INTER_VIEW_MOTION_PRED
    iMvBits += m_auiMVPIdxCost[iMVPIdx][iNumAMVPCands];
#else
    iMvBits += m_auiMVPIdxCost[iMVPIdx][AMVP_MAX_NUM_CANDS];
#endif
    
    if (iMvBits < iBestMvBits)
    {
      iBestMvBits = iMvBits;
      iBestMVPIdx = iMVPIdx;
    }
  }
  
  if (iBestMVPIdx != riMVPIdx)  //if changed
  {
    rcMvPred = pcAMVPInfo->m_acMvCand[iBestMVPIdx];
    
    riMVPIdx = iBestMVPIdx;
    UInt uiOrgBits = ruiBits;
    ruiBits = uiOrgBits - iOrgMvBits + iBestMvBits;
    ruiCost = (ruiCost - m_pcRdCost->getCost( uiOrgBits ))  + m_pcRdCost->getCost( ruiBits );
  }
}

UInt TEncSearch::xGetTemplateCost( TComDataCU* pcCU,
                                  UInt        uiPartIdx,
                                  UInt      uiPartAddr,
                                  TComYuv*    pcOrgYuv,
                                  TComYuv*    pcTemplateCand,
                                  TComMv      cMvCand,
                                  Int         iMVPIdx,
                                  Int     iMVPNum,
                                  RefPicList  eRefPicList,
                                  Int         iRefIdx,
                                  Int         iSizeX,
                                  Int         iSizeY
                               #if ZERO_MVD_EST
                                , UInt&       ruiDist
                               #endif
                                  )
{
  UInt uiCost  = MAX_INT;
  
  TComPicYuv* pcPicYuvRef = pcCU->getSlice()->getRefPic( eRefPicList, iRefIdx )->getPicYuvRec();
  
  pcCU->clipMv( cMvCand );

#if HHI_FULL_PEL_DEPTH_MAP_MV_ACC
  if( pcCU->getSlice()->getIsDepth() )
    cMvCand <<= 2;
#endif
  // prediction pattern
  if ( pcCU->getSlice()->getPPS()->getUseWP() && pcCU->getSlice()->getSliceType()==P_SLICE )
  {
    xPredInterLumaBlk( pcCU, pcPicYuvRef, uiPartAddr, &cMvCand, iSizeX, iSizeY, pcTemplateCand, true );
  }
  else
  {
    xPredInterLumaBlk( pcCU, pcPicYuvRef, uiPartAddr, &cMvCand, iSizeX, iSizeY, pcTemplateCand, false );
  }

  if ( pcCU->getSlice()->getPPS()->getUseWP() && pcCU->getSlice()->getSliceType()==P_SLICE )
  {
    xWeightedPredictionUni( pcCU, pcTemplateCand, uiPartAddr, iSizeX, iSizeY, eRefPicList, pcTemplateCand, uiPartIdx, iRefIdx );
  }

  // calc distortion
#if ZERO_MVD_EST
  m_pcRdCost->getMotionCost( 1, 0 );
  DistParam cDistParam;
  m_pcRdCost->setDistParam( cDistParam, 
                            pcOrgYuv->getLumaAddr(uiPartAddr), pcOrgYuv->getStride(), 
                            pcTemplateCand->getLumaAddr(uiPartAddr), pcTemplateCand->getStride(), 
#if NS_HAD
                            iSizeX, iSizeY, m_pcEncCfg->getUseHADME(), m_pcEncCfg->getUseNSQT() );
#else
                            iSizeX, iSizeY, m_pcEncCfg->getUseHADME() );
#endif
  ruiDist = cDistParam.DistFunc( &cDistParam );
  uiCost = ruiDist + m_pcRdCost->getCost( m_auiMVPIdxCost[iMVPIdx][iMVPNum] );
#else
// GT: CONSIDER ADDING VSO HERE
#if WEIGHTED_CHROMA_DISTORTION
  uiCost = m_pcRdCost->getDistPart( pcTemplateCand->getLumaAddr(uiPartAddr), pcTemplateCand->getStride(), pcOrgYuv->getLumaAddr(uiPartAddr), pcOrgYuv->getStride(), iSizeX, iSizeY, false, DF_SAD );
#else
  uiCost = m_pcRdCost->getDistPart( pcTemplateCand->getLumaAddr(uiPartAddr), pcTemplateCand->getStride(), pcOrgYuv->getLumaAddr(uiPartAddr), pcOrgYuv->getStride(), iSizeX, iSizeY, DF_SAD );
#endif
  uiCost = (UInt) m_pcRdCost->calcRdCost( m_auiMVPIdxCost[iMVPIdx][iMVPNum], uiCost, false, DF_SAD );
#endif
  return uiCost;
}

Void TEncSearch::xMotionEstimation( TComDataCU* pcCU, TComYuv* pcYuvOrg, Int iPartIdx, RefPicList eRefPicList, TComMv* pcMvPred, Int iRefIdxPred, TComMv& rcMv, UInt& ruiBits, UInt& ruiCost, Bool bBi  )
{
  UInt          uiPartAddr;
  Int           iRoiWidth;
  Int           iRoiHeight;
  
  TComMv        cMvHalf, cMvQter;
  TComMv        cMvSrchRngLT;
  TComMv        cMvSrchRngRB;
  
  TComYuv*      pcYuv = pcYuvOrg;
  m_iSearchRange = m_aaiAdaptSR[eRefPicList][iRefIdxPred];
  
  Int           iSrchRng      = ( bBi ? m_bipredSearchRange : m_iSearchRange );
  TComPattern*  pcPatternKey  = pcCU->getPattern        ();
  
  Double        fWeight       = 1.0;
  
  pcCU->getPartIndexAndSize( iPartIdx, uiPartAddr, iRoiWidth, iRoiHeight );
  
  if ( bBi )
  {
    TComYuv*  pcYuvOther = &m_acYuvPred[1-(Int)eRefPicList];
    pcYuv                = &m_cYuvPredTemp;
    
    pcYuvOrg->copyPartToPartYuv( pcYuv, uiPartAddr, iRoiWidth, iRoiHeight );
    
    pcYuv->removeHighFreq( pcYuvOther, uiPartAddr, iRoiWidth, iRoiHeight );
    
    fWeight = 0.5;
  }
  
  //  Search key pattern initialization
  pcPatternKey->initPattern( pcYuv->getLumaAddr( uiPartAddr ),
                            pcYuv->getCbAddr  ( uiPartAddr ),
                            pcYuv->getCrAddr  ( uiPartAddr ),
                            iRoiWidth,
                            iRoiHeight,
                            pcYuv->getStride(),
                            0, 0, 0, 0 );
  
  Pel*        piRefY      = pcCU->getSlice()->getRefPic( eRefPicList, iRefIdxPred )->getPicYuvRec()->getLumaAddr( pcCU->getAddr(), pcCU->getZorderIdxInCU() + uiPartAddr );
  Int         iRefStride  = pcCU->getSlice()->getRefPic( eRefPicList, iRefIdxPred )->getPicYuvRec()->getStride();
  
  TComMv      cMvPred = *pcMvPred;
  
  if ( bBi )  xSetSearchRange   ( pcCU, rcMv   , iSrchRng, cMvSrchRngLT, cMvSrchRngRB );
  else        xSetSearchRange   ( pcCU, cMvPred, iSrchRng, cMvSrchRngLT, cMvSrchRngRB );
  
  m_pcRdCost->getMotionCost ( 1, 0 );
  
  m_pcRdCost->setPredictor  ( *pcMvPred );
#if HHI_FULL_PEL_DEPTH_MAP_MV_ACC
  if( pcCU->getSlice()->getIsDepth() )
    m_pcRdCost->setCostScale  ( 0 );
  else
#endif
  m_pcRdCost->setCostScale  ( 2 );

#if HHI_INTER_VIEW_MOTION_PRED
  { // init inter-view regularization
    TComMv  cOrgDepthMapMv;
    Bool    bMultiviewReg = pcCU->getIViewOrgDepthMvPred( iPartIdx, eRefPicList, iRefIdxPred, cOrgDepthMapMv );
#if HHI_FULL_PEL_DEPTH_MAP_MV_ACC
    if( bMultiviewReg && pcCU->getSlice()->getSPS()->isDepth() )
    {
      cOrgDepthMapMv += TComMv( 2, 2 );
      cOrgDepthMapMv >>= 2;
    }
#endif
    m_pcRdCost->setMultiviewReg( bMultiviewReg ? &cOrgDepthMapMv : 0 );
    if( bMultiviewReg && !bBi )
    {
      xSetSearchRange( pcCU, cOrgDepthMapMv, iSrchRng, cMvSrchRngLT, cMvSrchRngRB );
    }
  }
#endif

  setWpScalingDistParam( pcCU, iRefIdxPred, eRefPicList );
  //  Do integer search
  if ( !m_iFastSearch || bBi )
  {
    xPatternSearch      ( pcPatternKey, piRefY, iRefStride, &cMvSrchRngLT, &cMvSrchRngRB, rcMv, ruiCost );
  }
  else
  {
    rcMv = *pcMvPred;
    xPatternSearchFast  ( pcCU, pcPatternKey, piRefY, iRefStride, &cMvSrchRngLT, &cMvSrchRngRB, rcMv, ruiCost );
  }
  
  m_pcRdCost->getMotionCost( 1, 0 );
#if HHI_FULL_PEL_DEPTH_MAP_MV_ACC
  if( ! pcCU->getSlice()->getIsDepth() )
  {
#endif
  m_pcRdCost->setCostScale ( 1 );
  
  {
    xPatternSearchFracDIF( pcCU, pcPatternKey, piRefY, iRefStride, &rcMv, cMvHalf, cMvQter, ruiCost
                          ,bBi
                          );
  }
  
  
  
  m_pcRdCost->setCostScale( 0 );
  rcMv <<= 2;
  rcMv += (cMvHalf <<= 1);
  rcMv +=  cMvQter;
#if HHI_FULL_PEL_DEPTH_MAP_MV_ACC
  }
#endif
  
  UInt uiMvBits = m_pcRdCost->getBits( rcMv.getHor(), rcMv.getVer() );
#if HHI_FULL_PEL_DEPTH_MAP_MV_ACC
  if( pcCU->getSlice()->getIsDepth() )
    ruiCost += m_pcRdCost->getCost( uiMvBits );
#endif
  
  ruiBits      += uiMvBits;
  ruiCost       = (UInt)( floor( fWeight * ( (Double)ruiCost - (Double)m_pcRdCost->getCost( uiMvBits ) ) ) + (Double)m_pcRdCost->getCost( ruiBits ) );
}


Void TEncSearch::xSetSearchRange ( TComDataCU* pcCU, TComMv& cMvPred, Int iSrchRng, TComMv& rcMvSrchRngLT, TComMv& rcMvSrchRngRB )
{
  Int  iMvShift = 2;
#if HHI_FULL_PEL_DEPTH_MAP_MV_ACC
  if( pcCU->getSlice()->getIsDepth() )
    iMvShift = 0;
#endif
  TComMv cTmpMvPred = cMvPred;
  pcCU->clipMv( cTmpMvPred );

  rcMvSrchRngLT.setHor( cTmpMvPred.getHor() - (iSrchRng << iMvShift) );
  rcMvSrchRngLT.setVer( cTmpMvPred.getVer() - (iSrchRng << iMvShift) );
  
  rcMvSrchRngRB.setHor( cTmpMvPred.getHor() + (iSrchRng << iMvShift) );
  rcMvSrchRngRB.setVer( cTmpMvPred.getVer() + (iSrchRng << iMvShift) );
  pcCU->clipMv        ( rcMvSrchRngLT );
  pcCU->clipMv        ( rcMvSrchRngRB );
  
  rcMvSrchRngLT >>= iMvShift;
  rcMvSrchRngRB >>= iMvShift;
}

Void TEncSearch::xPatternSearch( TComPattern* pcPatternKey, Pel* piRefY, Int iRefStride, TComMv* pcMvSrchRngLT, TComMv* pcMvSrchRngRB, TComMv& rcMv, UInt& ruiSAD )
{
  Int   iSrchRngHorLeft   = pcMvSrchRngLT->getHor();
  Int   iSrchRngHorRight  = pcMvSrchRngRB->getHor();
  Int   iSrchRngVerTop    = pcMvSrchRngLT->getVer();
  Int   iSrchRngVerBottom = pcMvSrchRngRB->getVer();
  
  UInt  uiSad;
  UInt  uiSadBest         = MAX_UINT;
  Int   iBestX = 0;
  Int   iBestY = 0;
  
  Pel*  piRefSrch;
  
  //-- jclee for using the SAD function pointer
  m_pcRdCost->setDistParam( pcPatternKey, piRefY, iRefStride,  m_cDistParam );
  
  // fast encoder decision: use subsampled SAD for integer ME
  if ( m_pcEncCfg->getUseFastEnc() )
  {
    if ( m_cDistParam.iRows > 8 )
    {
      m_cDistParam.iSubShift = 1;
    }
  }
  
  piRefY += (iSrchRngVerTop * iRefStride);
  for ( Int y = iSrchRngVerTop; y <= iSrchRngVerBottom; y++ )
  {
    for ( Int x = iSrchRngHorLeft; x <= iSrchRngHorRight; x++ )
    {
      //  find min. distortion position
      piRefSrch = piRefY + x;
      m_cDistParam.pCur = piRefSrch;

      setDistParamComp(0);

      uiSad = m_cDistParam.DistFunc( &m_cDistParam );
      
      // motion cost
      uiSad += m_pcRdCost->getCost( x, y );
      
      if ( uiSad < uiSadBest )
      {
        uiSadBest = uiSad;
        iBestX    = x;
        iBestY    = y;
      }
    }
    piRefY += iRefStride;
  }
  
  rcMv.set( iBestX, iBestY );
  
  ruiSAD = uiSadBest - m_pcRdCost->getCost( iBestX, iBestY );
  return;
}

Void TEncSearch::xPatternSearchFast( TComDataCU* pcCU, TComPattern* pcPatternKey, Pel* piRefY, Int iRefStride, TComMv* pcMvSrchRngLT, TComMv* pcMvSrchRngRB, TComMv& rcMv, UInt& ruiSAD )
{
  pcCU->getMvPredLeft       ( m_acMvPredictors[0] );
  pcCU->getMvPredAbove      ( m_acMvPredictors[1] );
  pcCU->getMvPredAboveRight ( m_acMvPredictors[2] );
  
  switch ( m_iFastSearch )
  {
    case 1:
      xTZSearch( pcCU, pcPatternKey, piRefY, iRefStride, pcMvSrchRngLT, pcMvSrchRngRB, rcMv, ruiSAD );
      break;
      
    default:
      break;
  }
}

Void TEncSearch::xTZSearch( TComDataCU* pcCU, TComPattern* pcPatternKey, Pel* piRefY, Int iRefStride, TComMv* pcMvSrchRngLT, TComMv* pcMvSrchRngRB, TComMv& rcMv, UInt& ruiSAD )
{
  Int   iSrchRngHorLeft   = pcMvSrchRngLT->getHor();
  Int   iSrchRngHorRight  = pcMvSrchRngRB->getHor();
  Int   iSrchRngVerTop    = pcMvSrchRngLT->getVer();
  Int   iSrchRngVerBottom = pcMvSrchRngRB->getVer();
  
  TZ_SEARCH_CONFIGURATION
  
  UInt uiSearchRange = m_iSearchRange;
  pcCU->clipMv( rcMv );
#if HHI_FULL_PEL_DEPTH_MAP_MV_ACC
  if( ! pcCU->getSlice()->getIsDepth() )
#endif
  rcMv >>= 2;
  // init TZSearchStruct
  IntTZSearchStruct cStruct;
  cStruct.iYStride    = iRefStride;
  cStruct.piRefY      = piRefY;
  cStruct.uiBestSad   = MAX_UINT;
  
  // set rcMv (Median predictor) as start point and as best point
  xTZSearchHelp( pcPatternKey, cStruct, rcMv.getHor(), rcMv.getVer(), 0, 0 );
  
  // test whether one of PRED_A, PRED_B, PRED_C MV is better start point than Median predictor
  if ( bTestOtherPredictedMV )
  {
    for ( UInt index = 0; index < 3; index++ )
    {
      TComMv cMv = m_acMvPredictors[index];
      pcCU->clipMv( cMv );
#if HHI_FULL_PEL_DEPTH_MAP_MV_ACC
      if( ! pcCU->getSlice()->getIsDepth() )
#endif
      cMv >>= 2;
      xTZSearchHelp( pcPatternKey, cStruct, cMv.getHor(), cMv.getVer(), 0, 0 );
    }
  }
  
  // test whether zero Mv is better start point than Median predictor
  if ( bTestZeroVector )
  {
    xTZSearchHelp( pcPatternKey, cStruct, 0, 0, 0, 0 );
  }
  
  // start search
  Int  iDist = 0;
  Int  iStartX = cStruct.iBestX;
  Int  iStartY = cStruct.iBestY;
  
  // first search
  for ( iDist = 1; iDist <= (Int)uiSearchRange; iDist*=2 )
  {
    if ( bFirstSearchDiamond == 1 )
    {
      xTZ8PointDiamondSearch ( pcPatternKey, cStruct, pcMvSrchRngLT, pcMvSrchRngRB, iStartX, iStartY, iDist );
    }
    else
    {
      xTZ8PointSquareSearch  ( pcPatternKey, cStruct, pcMvSrchRngLT, pcMvSrchRngRB, iStartX, iStartY, iDist );
    }
    
    if ( bFirstSearchStop && ( cStruct.uiBestRound >= uiFirstSearchRounds ) ) // stop criterion
    {
      break;
    }
  }
  
  // test whether zero Mv is a better start point than Median predictor
  if ( bTestZeroVectorStart && ((cStruct.iBestX != 0) || (cStruct.iBestY != 0)) )
  {
    xTZSearchHelp( pcPatternKey, cStruct, 0, 0, 0, 0 );
    if ( (cStruct.iBestX == 0) && (cStruct.iBestY == 0) )
    {
      // test its neighborhood
      for ( iDist = 1; iDist <= (Int)uiSearchRange; iDist*=2 )
      {
        xTZ8PointDiamondSearch( pcPatternKey, cStruct, pcMvSrchRngLT, pcMvSrchRngRB, 0, 0, iDist );
        if ( bTestZeroVectorStop && (cStruct.uiBestRound > 0) ) // stop criterion
        {
          break;
        }
      }
    }
  }
  
  // calculate only 2 missing points instead 8 points if cStruct.uiBestDistance == 1
  if ( cStruct.uiBestDistance == 1 )
  {
    cStruct.uiBestDistance = 0;
    xTZ2PointSearch( pcPatternKey, cStruct, pcMvSrchRngLT, pcMvSrchRngRB );
  }
  
  // raster search if distance is too big
  if ( bEnableRasterSearch && ( ((Int)(cStruct.uiBestDistance) > iRaster) || bAlwaysRasterSearch ) )
  {
    cStruct.uiBestDistance = iRaster;
    for ( iStartY = iSrchRngVerTop; iStartY <= iSrchRngVerBottom; iStartY += iRaster )
    {
      for ( iStartX = iSrchRngHorLeft; iStartX <= iSrchRngHorRight; iStartX += iRaster )
      {
        xTZSearchHelp( pcPatternKey, cStruct, iStartX, iStartY, 0, iRaster );
      }
    }
  }
  
  // raster refinement
  if ( bRasterRefinementEnable && cStruct.uiBestDistance > 0 )
  {
    while ( cStruct.uiBestDistance > 0 )
    {
      iStartX = cStruct.iBestX;
      iStartY = cStruct.iBestY;
      if ( cStruct.uiBestDistance > 1 )
      {
        iDist = cStruct.uiBestDistance >>= 1;
        if ( bRasterRefinementDiamond == 1 )
        {
          xTZ8PointDiamondSearch ( pcPatternKey, cStruct, pcMvSrchRngLT, pcMvSrchRngRB, iStartX, iStartY, iDist );
        }
        else
        {
          xTZ8PointSquareSearch  ( pcPatternKey, cStruct, pcMvSrchRngLT, pcMvSrchRngRB, iStartX, iStartY, iDist );
        }
      }
      
      // calculate only 2 missing points instead 8 points if cStruct.uiBestDistance == 1
      if ( cStruct.uiBestDistance == 1 )
      {
        cStruct.uiBestDistance = 0;
        if ( cStruct.ucPointNr != 0 )
        {
          xTZ2PointSearch( pcPatternKey, cStruct, pcMvSrchRngLT, pcMvSrchRngRB );
        }
      }
    }
  }
  
  // start refinement
  if ( bStarRefinementEnable && cStruct.uiBestDistance > 0 )
  {
    while ( cStruct.uiBestDistance > 0 )
    {
      iStartX = cStruct.iBestX;
      iStartY = cStruct.iBestY;
      cStruct.uiBestDistance = 0;
      cStruct.ucPointNr = 0;
      for ( iDist = 1; iDist < (Int)uiSearchRange + 1; iDist*=2 )
      {
        if ( bStarRefinementDiamond == 1 )
        {
          xTZ8PointDiamondSearch ( pcPatternKey, cStruct, pcMvSrchRngLT, pcMvSrchRngRB, iStartX, iStartY, iDist );
        }
        else
        {
          xTZ8PointSquareSearch  ( pcPatternKey, cStruct, pcMvSrchRngLT, pcMvSrchRngRB, iStartX, iStartY, iDist );
        }
        if ( bStarRefinementStop && (cStruct.uiBestRound >= uiStarRefinementRounds) ) // stop criterion
        {
          break;
        }
      }
      
      // calculate only 2 missing points instead 8 points if cStrukt.uiBestDistance == 1
      if ( cStruct.uiBestDistance == 1 )
      {
        cStruct.uiBestDistance = 0;
        if ( cStruct.ucPointNr != 0 )
        {
          xTZ2PointSearch( pcPatternKey, cStruct, pcMvSrchRngLT, pcMvSrchRngRB );
        }
      }
    }
  }
  
  // write out best match
  rcMv.set( cStruct.iBestX, cStruct.iBestY );
  ruiSAD = cStruct.uiBestSad - m_pcRdCost->getCost( cStruct.iBestX, cStruct.iBestY );
}

Void TEncSearch::xPatternSearchFracDIF(TComDataCU* pcCU,
                                       TComPattern* pcPatternKey,
                                       Pel* piRefY,
                                       Int iRefStride,
                                       TComMv* pcMvInt,
                                       TComMv& rcMvHalf,
                                       TComMv& rcMvQter,
                                       UInt& ruiCost
                                       ,Bool biPred
                                       )
{
  //  Reference pattern initialization (integer scale)
  TComPattern cPatternRoi;
  Int         iOffset    = pcMvInt->getHor() + pcMvInt->getVer() * iRefStride;
  cPatternRoi.initPattern( piRefY +  iOffset,
                          NULL,
                          NULL,
                          pcPatternKey->getROIYWidth(),
                          pcPatternKey->getROIYHeight(),
                          iRefStride,
                          0, 0, 0, 0 );
  
  //  Half-pel refinement
  xExtDIFUpSamplingH ( &cPatternRoi, biPred );
  
  rcMvHalf = *pcMvInt;   rcMvHalf <<= 1;    // for mv-cost
  TComMv baseRefMv(0, 0);
  ruiCost = xPatternRefinement( pcPatternKey, baseRefMv, 2, rcMvHalf   );
  
  m_pcRdCost->setCostScale( 0 );
  
  xExtDIFUpSamplingQ ( &cPatternRoi, rcMvHalf, biPred );
  baseRefMv = rcMvHalf;
  baseRefMv <<= 1;
  
  rcMvQter = *pcMvInt;   rcMvQter <<= 1;    // for mv-cost
  rcMvQter += rcMvHalf;  rcMvQter <<= 1;
  ruiCost = xPatternRefinement( pcPatternKey, baseRefMv, 1, rcMvQter );
}

/** encode residual and calculate rate-distortion for a CU block
 * \param pcCU
 * \param pcYuvOrg
 * \param pcYuvPred
 * \param rpcYuvResi
 * \param rpcYuvResiBest
 * \param rpcYuvRec
 * \param bSkipRes
 * \returns Void
 */
#if HHI_INTER_VIEW_RESIDUAL_PRED
Void TEncSearch::encodeResAndCalcRdInterCU( TComDataCU* pcCU, TComYuv* pcYuvOrg, TComYuv* pcYuvPred, TComYuv*& rpcYuvResi, TComYuv*& rpcYuvResiBest, TComYuv*& rpcYuvRec, TComYuv*& rpcYuvResPrd, Bool bSkipRes )
#else
Void TEncSearch::encodeResAndCalcRdInterCU( TComDataCU* pcCU, TComYuv* pcYuvOrg, TComYuv* pcYuvPred, TComYuv*& rpcYuvResi, TComYuv*& rpcYuvResiBest, TComYuv*& rpcYuvRec, Bool bSkipRes )
#endif
{
  if ( pcCU->isIntra(0) )
  {
    return;
  }
  
  PredMode  ePredMode    = pcCU->getPredictionMode( 0 );
  Bool      bHighPass    = pcCU->getSlice()->getDepth() ? true : false;
  UInt      uiBits       = 0, uiBitsBest = 0;
  Dist      uiDistortion = 0, uiDistortionBest = 0;
  
  UInt      uiWidth      = pcCU->getWidth ( 0 );
  UInt      uiHeight     = pcCU->getHeight( 0 );
#if LG_RESTRICTEDRESPRED_M24766
  Int       iPUResiPredShift[4];
#endif
  //  No residual coding : SKIP mode
  if ( ePredMode == MODE_SKIP && bSkipRes )
  {
    rpcYuvResi->clear();
    
    pcYuvPred->copyToPartYuv( rpcYuvRec, 0 );
    
#if HHI_INTER_VIEW_RESIDUAL_PRED
    // add residual prediction
    if( pcCU->getResPredFlag( 0 ) )
    {
#if LG_RESTRICTEDRESPRED_M24766
		pcCU->getPUResiPredShift(iPUResiPredShift, 0);
		rpcYuvRec->add(iPUResiPredShift, pcCU->getPartitionSize(0), rpcYuvResPrd, uiWidth, uiHeight );
#else
      rpcYuvRec->add( rpcYuvResPrd, uiWidth, uiHeight );
#endif
      rpcYuvRec->clip( uiWidth, uiHeight );
    }
#endif

#if HHI_VSO    
    if ( m_pcRdCost->getUseVSO() )
    {
      uiDistortion = m_pcRdCost->getDistVS( pcCU, 0, rpcYuvRec->getLumaAddr(), rpcYuvRec->getStride(),  pcYuvOrg->getLumaAddr(), pcYuvOrg->getStride(),  uiWidth,      uiHeight     , false, 0 );
	}
    else    
    {
#endif
#if WEIGHTED_CHROMA_DISTORTION
    uiDistortion = m_pcRdCost->getDistPart( rpcYuvRec->getLumaAddr(), rpcYuvRec->getStride(),  pcYuvOrg->getLumaAddr(), pcYuvOrg->getStride(),  uiWidth,      uiHeight      )
    + m_pcRdCost->getDistPart( rpcYuvRec->getCbAddr(),   rpcYuvRec->getCStride(), pcYuvOrg->getCbAddr(),   pcYuvOrg->getCStride(), uiWidth >> 1, uiHeight >> 1, true )
    + m_pcRdCost->getDistPart( rpcYuvRec->getCrAddr(),   rpcYuvRec->getCStride(), pcYuvOrg->getCrAddr(),   pcYuvOrg->getCStride(), uiWidth >> 1, uiHeight >> 1, true );
#else
    uiDistortion = m_pcRdCost->getDistPart( rpcYuvRec->getLumaAddr(), rpcYuvRec->getStride(),  pcYuvOrg->getLumaAddr(), pcYuvOrg->getStride(),  uiWidth,      uiHeight      )
    + m_pcRdCost->getDistPart( rpcYuvRec->getCbAddr(),   rpcYuvRec->getCStride(), pcYuvOrg->getCbAddr(),   pcYuvOrg->getCStride(), uiWidth >> 1, uiHeight >> 1 )
    + m_pcRdCost->getDistPart( rpcYuvRec->getCrAddr(),   rpcYuvRec->getCStride(), pcYuvOrg->getCrAddr(),   pcYuvOrg->getCStride(), uiWidth >> 1, uiHeight >> 1 );
#endif
#if HHI_VSO    
    }
#endif

    if( m_bUseSBACRD )
      m_pcRDGoOnSbacCoder->load(m_pppcRDSbacCoder[pcCU->getDepth(0)][CI_CURR_BEST]);
    
    m_pcEntropyCoder->resetBits();
#if HHI_MPI
    if( pcCU->getTextureModeDepth( 0 ) == -1 )
    {
#endif
    m_pcEntropyCoder->encodeSkipFlag(pcCU, 0, true);
    m_pcEntropyCoder->encodeMergeIndex( pcCU, 0, 0, true );
#if HHI_INTER_VIEW_RESIDUAL_PRED
    m_pcEntropyCoder->encodeResPredFlag( pcCU, 0, 0, true );
#endif
#if HHI_MPI
    }
#endif
    
    uiBits = m_pcEntropyCoder->getNumberOfWrittenBits();
    pcCU->getTotalBits()       = uiBits;
    pcCU->getTotalDistortion() = uiDistortion;

#if HHI_VSO
    if ( m_pcRdCost->getUseLambdaScaleVSO() )
    {
      pcCU->getTotalCost() = m_pcRdCost->calcRdCostVSO( uiBits, uiDistortion );
    }
    else
#endif
    {
    pcCU->getTotalCost()       = m_pcRdCost->calcRdCost( uiBits, uiDistortion );
    }
    
    if( m_bUseSBACRD )
      m_pcRDGoOnSbacCoder->store(m_pppcRDSbacCoder[pcCU->getDepth(0)][CI_TEMP_BEST]);
    
    pcCU->setCbfSubParts( 0, 0, 0, 0, pcCU->getDepth( 0 ) );
    pcCU->setTrIdxSubParts( 0, 0, pcCU->getDepth(0) );
    
#if HHI_VSO // necessary? 
    // set Model
    if( m_pcRdCost->getUseRenModel() )
    {
      Pel*  piSrc       = rpcYuvRec->getLumaAddr();
      UInt  uiSrcStride = rpcYuvRec->getStride();
      m_pcRdCost->setRenModelData( pcCU, 0, piSrc, uiSrcStride, uiWidth, uiHeight );
    }
#endif

    return;
  }
  
  //  Residual coding.
#if H0736_AVC_STYLE_QP_RANGE
  Int    qp, qpBest = 0, qpMin, qpMax;
#else
  UInt    uiQp, uiQpBest = 0, uiQpMin, uiQpMax;
#endif
  Double  dCost, dCostBest = MAX_DOUBLE;
  
  UInt uiTrLevel = 0;
  if( (pcCU->getWidth(0) > pcCU->getSlice()->getSPS()->getMaxTrSize()) )
  {
    while( pcCU->getWidth(0) > (pcCU->getSlice()->getSPS()->getMaxTrSize()<<uiTrLevel) ) uiTrLevel++;
  }
  UInt uiMaxTrMode = pcCU->getSlice()->getSPS()->getMaxTrDepth() + uiTrLevel;
  
  while((uiWidth>>uiMaxTrMode) < (g_uiMaxCUWidth>>g_uiMaxCUDepth)) uiMaxTrMode--;
  
#if H0736_AVC_STYLE_QP_RANGE
  qpMin =  bHighPass ? Clip3( -pcCU->getSlice()->getSPS()->getQpBDOffsetY(), MAX_QP, pcCU->getQP(0) - m_iMaxDeltaQP ) : pcCU->getQP( 0 );
  qpMax =  bHighPass ? Clip3( -pcCU->getSlice()->getSPS()->getQpBDOffsetY(), MAX_QP, pcCU->getQP(0) + m_iMaxDeltaQP ) : pcCU->getQP( 0 );
#else
  uiQpMin      = bHighPass ? min( MAX_QP, max( MIN_QP, pcCU->getQP(0) - m_iMaxDeltaQP ) ) : pcCU->getQP( 0 );
  uiQpMax      = bHighPass ? min( MAX_QP, max( MIN_QP, pcCU->getQP(0) + m_iMaxDeltaQP ) ) : pcCU->getQP( 0 );
#endif

  #if HHI_INTERVIEW_SKIP
  if( bSkipRes)
  {
    rpcYuvResi->clear() ;
  }
  else
  {
#if LG_RESTRICTEDRESPRED_M24766
	  iPUResiPredShift[0] = iPUResiPredShift[1] = iPUResiPredShift[2] = iPUResiPredShift[3] = 0;
	  rpcYuvResi->subtract(iPUResiPredShift, pcCU->getPartitionSize(0), pcYuvOrg, pcYuvPred, 0, uiWidth );
#else
	  rpcYuvResi->subtract( pcYuvOrg, pcYuvPred, 0, uiWidth );
#endif
#if HHI_INTER_VIEW_RESIDUAL_PRED
    // subtract residual prediction
    if( pcCU->getResPredFlag( 0 ) )
    {
#if LG_RESTRICTEDRESPRED_M24766
		pcCU->getPUResiPredShift(iPUResiPredShift, 0);
		rpcYuvResi->subtract(iPUResiPredShift, pcCU->getPartitionSize(0), rpcYuvResi, rpcYuvResPrd, 0, uiWidth );
#else
      rpcYuvResi->subtract( rpcYuvResi, rpcYuvResPrd, 0, uiWidth );
#endif
    }
#endif
  }
#else
  rpcYuvResi->subtract( pcYuvOrg, pcYuvPred, 0, uiWidth );
#if HHI_INTER_VIEW_RESIDUAL_PRED
  // add residual prediction
  if( pcCU->getResPredFlag( 0 ) )
  {
    rpcYuvResi->subtract( rpcYuvResi, rpcYuvResPrd, uiWidth, uiHeight );
  }
#endif
#endif

#if H0736_AVC_STYLE_QP_RANGE
  for ( qp = qpMin; qp <= qpMax; qp++ )
#else
  for ( uiQp = uiQpMin; uiQp <= uiQpMax; uiQp++ )
#endif
  {
    dCost = 0.;
    uiBits = 0;
    uiDistortion = 0;
    if( m_bUseSBACRD )
    {
      m_pcRDGoOnSbacCoder->load( m_pppcRDSbacCoder[ pcCU->getDepth( 0 ) ][ CI_CURR_BEST ] );
    }
    
    Dist uiZeroDistortion = 0;
#if HHI_VSO
    if ( m_pcRdCost->getUseVSO() )
    {
      m_cYuvRecTemp.create( pcYuvPred->getWidth(), pcYuvPred->getHeight()  );
    }
#endif
#if IBDI_DISTORTION || HHI_VSO
    xEstimateResidualQT( pcCU, 0, 0, 0, pcYuvOrg, pcYuvPred, rpcYuvResi,  pcCU->getDepth(0), dCost, uiBits, uiDistortion, &uiZeroDistortion );
#else
    xEstimateResidualQT( pcCU, 0, 0, 0, rpcYuvResi,  pcCU->getDepth(0), dCost, uiBits, uiDistortion, &uiZeroDistortion );
#endif
    

#if HHI_VSO
    if ( m_pcRdCost->getUseVSO() )
    {
      m_cYuvRecTemp.destroy();
    }
#endif

    double dZeroCost;
#if HHI_VSO
    if( m_pcRdCost->getUseLambdaScaleVSO() )
    {
      dZeroCost = m_pcRdCost->calcRdCostVSO( 0, uiZeroDistortion );
    }
    else
#endif
    {
      dZeroCost = m_pcRdCost->calcRdCost( 0, uiZeroDistortion );
    }

#if LOSSLESS_CODING
    if(pcCU->isLosslessCoded( 0 ))
    {  
      dZeroCost = dCost + 1;
    }
#endif
    if ( dZeroCost < dCost )
    {
      dCost        = dZeroCost;
      uiBits       = 0;
      uiDistortion = uiZeroDistortion;
      
      const UInt uiQPartNum = pcCU->getPic()->getNumPartInCU() >> (pcCU->getDepth(0) << 1);
      ::memset( pcCU->getTransformIdx()      , 0, uiQPartNum * sizeof(UChar) );
      ::memset( pcCU->getCbf( TEXT_LUMA )    , 0, uiQPartNum * sizeof(UChar) );
      ::memset( pcCU->getCbf( TEXT_CHROMA_U ), 0, uiQPartNum * sizeof(UChar) );
      ::memset( pcCU->getCbf( TEXT_CHROMA_V ), 0, uiQPartNum * sizeof(UChar) );
      ::memset( pcCU->getCoeffY()            , 0, uiWidth * uiHeight * sizeof( TCoeff )      );
      ::memset( pcCU->getCoeffCb()           , 0, uiWidth * uiHeight * sizeof( TCoeff ) >> 2 );
      ::memset( pcCU->getCoeffCr()           , 0, uiWidth * uiHeight * sizeof( TCoeff ) >> 2 );
    }
    else
    {
      xSetResidualQTData( pcCU, 0, 0, 0, NULL, pcCU->getDepth(0), false );
    }
    
    if( m_bUseSBACRD )
    {
      m_pcRDGoOnSbacCoder->load( m_pppcRDSbacCoder[pcCU->getDepth(0)][CI_CURR_BEST] );
    }
#if 0 // check
    {
      m_pcEntropyCoder->resetBits();
      m_pcEntropyCoder->encodeCoeff( pcCU, 0, pcCU->getDepth(0), pcCU->getWidth(0), pcCU->getHeight(0) );
      const UInt uiBitsForCoeff = m_pcEntropyCoder->getNumberOfWrittenBits();
      if( m_bUseSBACRD )
      {
        m_pcRDGoOnSbacCoder->load( m_pppcRDSbacCoder[pcCU->getDepth(0)][CI_CURR_BEST] );
      }
      if( uiBitsForCoeff != uiBits )
        assert( 0 );
    }
#endif
    uiBits = 0;
    {
      TComYuv *pDummy = NULL;
      xAddSymbolBitsInter( pcCU, 0, 0, uiBits, pDummy, NULL, pDummy );
    }
    
    Double dExactCost;
#if HHI_VSO
    if( m_pcRdCost->getUseLambdaScaleVSO() )
    {
      dExactCost = m_pcRdCost->calcRdCostVSO( uiBits, uiDistortion );;
    }
    else
#endif
    {
      dExactCost = m_pcRdCost->calcRdCost( uiBits, uiDistortion );
    }
    
    dCost = dExactCost;
    
    if ( dCost < dCostBest )
    {
      if ( !pcCU->getQtRootCbf( 0 ) )
      {
        rpcYuvResiBest->clear();
      }
      else
      {
        xSetResidualQTData( pcCU, 0, 0, 0, rpcYuvResiBest, pcCU->getDepth(0), true );
      }
      
#if H0736_AVC_STYLE_QP_RANGE
      if( qpMin != qpMax && qp != qpMax )
#else
      if( uiQpMin != uiQpMax && uiQp != uiQpMax )
#endif
      {
        const UInt uiQPartNum = pcCU->getPic()->getNumPartInCU() >> (pcCU->getDepth(0) << 1);
        ::memcpy( m_puhQTTempTrIdx, pcCU->getTransformIdx(),        uiQPartNum * sizeof(UChar) );
        ::memcpy( m_puhQTTempCbf[0], pcCU->getCbf( TEXT_LUMA ),     uiQPartNum * sizeof(UChar) );
        ::memcpy( m_puhQTTempCbf[1], pcCU->getCbf( TEXT_CHROMA_U ), uiQPartNum * sizeof(UChar) );
        ::memcpy( m_puhQTTempCbf[2], pcCU->getCbf( TEXT_CHROMA_V ), uiQPartNum * sizeof(UChar) );
        ::memcpy( m_pcQTTempCoeffY,  pcCU->getCoeffY(),  uiWidth * uiHeight * sizeof( TCoeff )      );
        ::memcpy( m_pcQTTempCoeffCb, pcCU->getCoeffCb(), uiWidth * uiHeight * sizeof( TCoeff ) >> 2 );
        ::memcpy( m_pcQTTempCoeffCr, pcCU->getCoeffCr(), uiWidth * uiHeight * sizeof( TCoeff ) >> 2 );
#if ADAPTIVE_QP_SELECTION
        ::memcpy( m_pcQTTempArlCoeffY,  pcCU->getArlCoeffY(),  uiWidth * uiHeight * sizeof( Int )      );
        ::memcpy( m_pcQTTempArlCoeffCb, pcCU->getArlCoeffCb(), uiWidth * uiHeight * sizeof( Int ) >> 2 );
        ::memcpy( m_pcQTTempArlCoeffCr, pcCU->getArlCoeffCr(), uiWidth * uiHeight * sizeof( Int ) >> 2 );
#endif
      }
      uiBitsBest       = uiBits;
      uiDistortionBest = uiDistortion;
      dCostBest        = dCost;
#if H0736_AVC_STYLE_QP_RANGE
      qpBest           = qp;
#else
      uiQpBest         = uiQp;
#endif      
      if( m_bUseSBACRD )
      {
        m_pcRDGoOnSbacCoder->store( m_pppcRDSbacCoder[ pcCU->getDepth( 0 ) ][ CI_TEMP_BEST ] );
      }
    }

#if HHI_VSO
    // GT: reset Model, only fordQP necessary??
    if( m_pcRdCost->getUseRenModel() )
    {
      Pel*  piSrc       = pcYuvOrg->getLumaAddr();
      UInt  uiSrcStride = pcYuvOrg->getStride();
      m_pcRdCost->setRenModelData( pcCU, 0, piSrc, uiSrcStride, uiWidth, uiHeight );
    }
#endif
  }
  
  assert ( dCostBest != MAX_DOUBLE );
  
#if H0736_AVC_STYLE_QP_RANGE
  if( qpMin != qpMax && qpBest != qpMax )
#else
  if( uiQpMin != uiQpMax && uiQpBest != uiQpMax )
#endif
  {
    if( m_bUseSBACRD )
    {
      assert( 0 ); // check
      m_pcRDGoOnSbacCoder->load( m_pppcRDSbacCoder[ pcCU->getDepth( 0 ) ][ CI_TEMP_BEST ] );
    }
    // copy best cbf and trIdx to pcCU
    const UInt uiQPartNum = pcCU->getPic()->getNumPartInCU() >> (pcCU->getDepth(0) << 1);
    ::memcpy( pcCU->getTransformIdx(),       m_puhQTTempTrIdx,  uiQPartNum * sizeof(UChar) );
    ::memcpy( pcCU->getCbf( TEXT_LUMA ),     m_puhQTTempCbf[0], uiQPartNum * sizeof(UChar) );
    ::memcpy( pcCU->getCbf( TEXT_CHROMA_U ), m_puhQTTempCbf[1], uiQPartNum * sizeof(UChar) );
    ::memcpy( pcCU->getCbf( TEXT_CHROMA_V ), m_puhQTTempCbf[2], uiQPartNum * sizeof(UChar) );
    ::memcpy( pcCU->getCoeffY(),  m_pcQTTempCoeffY,  uiWidth * uiHeight * sizeof( TCoeff )      );
    ::memcpy( pcCU->getCoeffCb(), m_pcQTTempCoeffCb, uiWidth * uiHeight * sizeof( TCoeff ) >> 2 );
    ::memcpy( pcCU->getCoeffCr(), m_pcQTTempCoeffCr, uiWidth * uiHeight * sizeof( TCoeff ) >> 2 );
#if ADAPTIVE_QP_SELECTION
    ::memcpy( pcCU->getArlCoeffY(),  m_pcQTTempArlCoeffY,  uiWidth * uiHeight * sizeof( Int )      );
    ::memcpy( pcCU->getArlCoeffCb(), m_pcQTTempArlCoeffCb, uiWidth * uiHeight * sizeof( Int ) >> 2 );
    ::memcpy( pcCU->getArlCoeffCr(), m_pcQTTempArlCoeffCr, uiWidth * uiHeight * sizeof( Int ) >> 2 );
#endif
  }
#if HHI_INTER_VIEW_RESIDUAL_PRED
  // add residual prediction
  if( pcCU->getResPredFlag( 0 ) )
  {
    pcYuvPred->copyToPartYuv( rpcYuvRec, 0 );
#if LG_RESTRICTEDRESPRED_M24766
	pcCU->getPUResiPredShift(iPUResiPredShift, 0);
	rpcYuvRec->add(iPUResiPredShift, pcCU->getPartitionSize(0), rpcYuvResPrd,   uiWidth, uiHeight );
	iPUResiPredShift[0] = iPUResiPredShift[1] = iPUResiPredShift[2] = iPUResiPredShift[3] = 0;
    rpcYuvRec->add(iPUResiPredShift, pcCU->getPartitionSize(0), rpcYuvResiBest, uiWidth, uiHeight );
#else
	rpcYuvRec->add( rpcYuvResPrd,   uiWidth, uiHeight );
	rpcYuvRec->add( rpcYuvResiBest, uiWidth, uiHeight );
#endif
    rpcYuvRec->clip( uiWidth, uiHeight );
  }
  else
#endif
  rpcYuvRec->addClip ( pcYuvPred, rpcYuvResiBest, 0, uiWidth );
  
  // update with clipped distortion and cost (qp estimation loop uses unclipped values)

#if HHI_VSO // GT: might be removed since VSO already provided clipped distortion
  if ( m_pcRdCost->getUseVSO() )
  {
    uiDistortionBest = m_pcRdCost->getDistVS  ( pcCU, 0, rpcYuvRec->getLumaAddr(), rpcYuvRec->getStride(),  pcYuvOrg->getLumaAddr(), pcYuvOrg->getStride(),  uiWidth,      uiHeight, false, 0    );
  }
  else
#endif
{
#if WEIGHTED_CHROMA_DISTORTION
  uiDistortionBest = m_pcRdCost->getDistPart( rpcYuvRec->getLumaAddr(), rpcYuvRec->getStride(),  pcYuvOrg->getLumaAddr(), pcYuvOrg->getStride(),  uiWidth,      uiHeight      )
  + m_pcRdCost->getDistPart( rpcYuvRec->getCbAddr(),   rpcYuvRec->getCStride(), pcYuvOrg->getCbAddr(),   pcYuvOrg->getCStride(), uiWidth >> 1, uiHeight >> 1, true )
  + m_pcRdCost->getDistPart( rpcYuvRec->getCrAddr(),   rpcYuvRec->getCStride(), pcYuvOrg->getCrAddr(),   pcYuvOrg->getCStride(), uiWidth >> 1, uiHeight >> 1, true );
#else
  uiDistortionBest = m_pcRdCost->getDistPart( rpcYuvRec->getLumaAddr(), rpcYuvRec->getStride(),  pcYuvOrg->getLumaAddr(), pcYuvOrg->getStride(),  uiWidth,      uiHeight      )
  + m_pcRdCost->getDistPart( rpcYuvRec->getCbAddr(),   rpcYuvRec->getCStride(), pcYuvOrg->getCbAddr(),   pcYuvOrg->getCStride(), uiWidth >> 1, uiHeight >> 1 )
  + m_pcRdCost->getDistPart( rpcYuvRec->getCrAddr(),   rpcYuvRec->getCStride(), pcYuvOrg->getCrAddr(),   pcYuvOrg->getCStride(), uiWidth >> 1, uiHeight >> 1 );
#endif
}
#if HHI_VSO
  if ( m_pcRdCost->getUseLambdaScaleVSO() )
  {
    dCostBest = m_pcRdCost->calcRdCostVSO( uiBitsBest, uiDistortionBest );
  }
  else
#endif
  {
  dCostBest = m_pcRdCost->calcRdCost( uiBitsBest, uiDistortionBest );
  }
  
  pcCU->getTotalBits()       = uiBitsBest;
  pcCU->getTotalDistortion() = uiDistortionBest;
  pcCU->getTotalCost()       = dCostBest;
  
  if ( pcCU->isSkipped(0) )
  {
    pcCU->setCbfSubParts( 0, 0, 0, 0, pcCU->getDepth( 0 ) );
  }
  
#if H0736_AVC_STYLE_QP_RANGE
  pcCU->setQPSubParts( qpBest, 0, pcCU->getDepth(0) );
#else
  pcCU->setQPSubParts( uiQpBest, 0, pcCU->getDepth(0) );
#endif

  // set Model
#if HHI_VSO // necessary??
  if( m_pcRdCost->getUseRenModel() )
  {
    Pel*  piSrc       = rpcYuvRec->getLumaAddr();
    UInt  uiSrcStride = rpcYuvRec->getStride();
    m_pcRdCost->setRenModelData( pcCU, 0, piSrc, uiSrcStride, uiWidth, uiHeight );
}
#endif

}

#if IBDI_DISTORTION || HHI_VSO
Void TEncSearch::xEstimateResidualQT( TComDataCU* pcCU, UInt uiQuadrant, UInt uiAbsPartIdx, UInt absTUPartIdx, TComYuv* pcOrg, TComYuv* pcPred, TComYuv* pcResi, const UInt uiDepth, Double &rdCost, UInt &ruiBits, Dist &ruiDist, Dist *puiZeroDist )
#else
Void TEncSearch::xEstimateResidualQT( TComDataCU* pcCU, UInt uiQuadrant, UInt uiAbsPartIdx, UInt absTUPartIdx, TComYuv* pcResi, const UInt uiDepth, Double &rdCost, UInt &ruiBits, Dist &ruiDist, Dist *puiZeroDist )
#endif
{
  const UInt uiTrMode = uiDepth - pcCU->getDepth( 0 );
  
  assert( pcCU->getDepth( 0 ) == pcCU->getDepth( uiAbsPartIdx ) );
  const UInt uiLog2TrSize = g_aucConvertToBit[pcCU->getSlice()->getSPS()->getMaxCUWidth() >> uiDepth]+2;
  
#if G519_TU_AMP_NSQT_HARMONIZATION
  UInt SplitFlag = ((pcCU->getSlice()->getSPS()->getQuadtreeTUMaxDepthInter() == 1) && pcCU->getPredictionMode(uiAbsPartIdx) == MODE_INTER && ( pcCU->getPartitionSize(uiAbsPartIdx) != SIZE_2Nx2N ));
#else
  UInt SplitFlag = ((pcCU->getSlice()->getSPS()->getQuadtreeTUMaxDepthInter() == 1) && pcCU->getPredictionMode(uiAbsPartIdx) == MODE_INTER && ( pcCU->getPartitionSize(uiAbsPartIdx) == SIZE_2NxN || pcCU->getPartitionSize(uiAbsPartIdx) == SIZE_Nx2N));
#endif
  Bool bCheckFull;
  if ( SplitFlag && uiDepth == pcCU->getDepth(uiAbsPartIdx) && ( uiLog2TrSize >  pcCU->getQuadtreeTULog2MinSizeInCU(uiAbsPartIdx) ) )
     bCheckFull = false;
  else
     bCheckFull =  ( uiLog2TrSize <= pcCU->getSlice()->getSPS()->getQuadtreeTULog2MaxSize() );

  const Bool bCheckSplit  = ( uiLog2TrSize >  pcCU->getQuadtreeTULog2MinSizeInCU(uiAbsPartIdx) );
  
  assert( bCheckFull || bCheckSplit );
  
  Bool  bCodeChroma   = true;
  UInt  uiTrModeC     = uiTrMode;
  UInt  uiLog2TrSizeC = uiLog2TrSize-1;
  if( uiLog2TrSize == 2 )
  {
    uiLog2TrSizeC++;
    uiTrModeC    --;
    UInt  uiQPDiv = pcCU->getPic()->getNumPartInCU() >> ( ( pcCU->getDepth( 0 ) + uiTrModeC ) << 1 );
    bCodeChroma   = ( ( uiAbsPartIdx % uiQPDiv ) == 0 );
  }
  
  const UInt uiSetCbf = 1 << uiTrMode;
  // code full block
  Double dSingleCost = MAX_DOUBLE;
  UInt uiSingleBits = 0;
  Dist uiSingleDist = 0;
  UInt uiAbsSumY = 0, uiAbsSumU = 0, uiAbsSumV = 0;
  
  if( m_bUseSBACRD )
  {
    m_pcRDGoOnSbacCoder->store( m_pppcRDSbacCoder[ uiDepth ][ CI_QT_TRAFO_ROOT ] );
  }
  
  if( bCheckFull )
  {
    const UInt uiNumCoeffPerAbsPartIdxIncrement = pcCU->getSlice()->getSPS()->getMaxCUWidth() * pcCU->getSlice()->getSPS()->getMaxCUHeight() >> ( pcCU->getSlice()->getSPS()->getMaxCUDepth() << 1 );
    const UInt uiQTTempAccessLayer = pcCU->getSlice()->getSPS()->getQuadtreeTULog2MaxSize() - uiLog2TrSize;
    TCoeff *pcCoeffCurrY = m_ppcQTTempCoeffY [uiQTTempAccessLayer] +  uiNumCoeffPerAbsPartIdxIncrement * uiAbsPartIdx;
    TCoeff *pcCoeffCurrU = m_ppcQTTempCoeffCb[uiQTTempAccessLayer] + (uiNumCoeffPerAbsPartIdxIncrement * uiAbsPartIdx>>2);
    TCoeff *pcCoeffCurrV = m_ppcQTTempCoeffCr[uiQTTempAccessLayer] + (uiNumCoeffPerAbsPartIdxIncrement * uiAbsPartIdx>>2);
#if ADAPTIVE_QP_SELECTION    
    Int *pcArlCoeffCurrY = m_ppcQTTempArlCoeffY [uiQTTempAccessLayer] +  uiNumCoeffPerAbsPartIdxIncrement * uiAbsPartIdx;
    Int *pcArlCoeffCurrU = m_ppcQTTempArlCoeffCb[uiQTTempAccessLayer] + (uiNumCoeffPerAbsPartIdxIncrement * uiAbsPartIdx>>2);
    Int *pcArlCoeffCurrV = m_ppcQTTempArlCoeffCr[uiQTTempAccessLayer] + (uiNumCoeffPerAbsPartIdxIncrement * uiAbsPartIdx>>2);   
#endif
    
    Int trWidth = 0, trHeight = 0, trWidthC = 0, trHeightC = 0;
    UInt absTUPartIdxC = uiAbsPartIdx;

    trWidth  = trHeight  = 1 << uiLog2TrSize;
    trWidthC = trHeightC = 1 <<uiLog2TrSizeC;
    pcCU->getNSQTSize ( uiTrMode, uiAbsPartIdx, trWidth, trHeight );
    pcCU->getNSQTSize ( uiTrModeC, uiAbsPartIdx, trWidthC, trHeightC );

    if( bCodeChroma && pcCU->useNonSquareTrans( uiTrMode, uiAbsPartIdx ) && !( uiLog2TrSizeC  == pcCU->getSlice()->getSPS()->getQuadtreeTULog2MinSize() && uiTrModeC == 1 ) )
    {  
      absTUPartIdxC = pcCU->getNSAddrChroma( uiLog2TrSizeC, uiTrModeC, uiQuadrant, absTUPartIdx );
    }
    pcCU->setTrIdxSubParts( uiDepth - pcCU->getDepth( 0 ), uiAbsPartIdx, uiDepth );
    if (m_pcEncCfg->getUseRDOQ())
    {
      m_pcEntropyCoder->estimateBit(m_pcTrQuant->m_pcEstBitsSbac, trWidth, trHeight, TEXT_LUMA );        
    }

#if H0736_AVC_STYLE_QP_RANGE
    m_pcTrQuant->setQPforQuant( pcCU->getQP( 0 ), false, pcCU->getSlice()->getSliceType(), TEXT_LUMA, pcCU->getSlice()->getSPS()->getQpBDOffsetY(), 0 );
#else
    m_pcTrQuant->setQPforQuant( pcCU->getQP( 0 ), false, pcCU->getSlice()->getSliceType(), TEXT_LUMA, 0 );
#endif

#if RDOQ_CHROMA_LAMBDA 
    m_pcTrQuant->selectLambda(TEXT_LUMA);  
#endif
    m_pcTrQuant->transformNxN( pcCU, pcResi->getLumaAddr( absTUPartIdx ), pcResi->getStride (), pcCoeffCurrY, 
#if ADAPTIVE_QP_SELECTION
                                 pcArlCoeffCurrY, 
#endif      
                                 trWidth,   trHeight,    uiAbsSumY, TEXT_LUMA,     uiAbsPartIdx );
    
    pcCU->setCbfSubParts( uiAbsSumY ? uiSetCbf : 0, TEXT_LUMA, uiAbsPartIdx, uiDepth );
    
    if( bCodeChroma )
    {
      if (m_pcEncCfg->getUseRDOQ())
      {
        m_pcEntropyCoder->estimateBit(m_pcTrQuant->m_pcEstBitsSbac, trWidthC, trHeightC, TEXT_CHROMA );          
      }

#if H0736_AVC_STYLE_QP_RANGE
      m_pcTrQuant->setQPforQuant( pcCU->getQP( 0 ), false, pcCU->getSlice()->getSliceType(), TEXT_CHROMA, pcCU->getSlice()->getSPS()->getQpBDOffsetC(), pcCU->getSlice()->getPPS()->getChromaQpOffset() );
#else
      m_pcTrQuant->setQPforQuant( pcCU->getQP( 0 ), false, pcCU->getSlice()->getSliceType(), TEXT_CHROMA, pcCU->getSlice()->getPPS()->getChromaQpOffset() );
#endif

#if RDOQ_CHROMA_LAMBDA 
      m_pcTrQuant->selectLambda(TEXT_CHROMA); 
#endif

      m_pcTrQuant->transformNxN( pcCU, pcResi->getCbAddr(absTUPartIdxC), pcResi->getCStride(), pcCoeffCurrU, 
#if ADAPTIVE_QP_SELECTION
                                 pcArlCoeffCurrU, 
#endif        
                                 trWidthC, trHeightC, uiAbsSumU, TEXT_CHROMA_U, uiAbsPartIdx );
#if H0736_AVC_STYLE_QP_RANGE
      m_pcTrQuant->setQPforQuant( pcCU->getQP( 0 ), false, pcCU->getSlice()->getSliceType(), TEXT_CHROMA, pcCU->getSlice()->getSPS()->getQpBDOffsetC(), pcCU->getSlice()->getPPS()->getChromaQpOffset2nd() );
#else
      m_pcTrQuant->setQPforQuant( pcCU->getQP( 0 ), false, pcCU->getSlice()->getSliceType(), TEXT_CHROMA, pcCU->getSlice()->getPPS()->getChromaQpOffset2nd() );
#endif
      m_pcTrQuant->transformNxN( pcCU, pcResi->getCrAddr(absTUPartIdxC), pcResi->getCStride(), pcCoeffCurrV, 
#if ADAPTIVE_QP_SELECTION
                                 pcArlCoeffCurrV, 
#endif        
                                 trWidthC, trHeightC, uiAbsSumV, TEXT_CHROMA_V, uiAbsPartIdx );

      pcCU->setCbfSubParts( uiAbsSumU ? uiSetCbf : 0, TEXT_CHROMA_U, uiAbsPartIdx, pcCU->getDepth(0)+uiTrModeC );
      pcCU->setCbfSubParts( uiAbsSumV ? uiSetCbf : 0, TEXT_CHROMA_V, uiAbsPartIdx, pcCU->getDepth(0)+uiTrModeC );
    }
    
    m_pcEntropyCoder->resetBits();
    
    {
      m_pcEntropyCoder->encodeQtCbf( pcCU, uiAbsPartIdx, TEXT_LUMA,     uiTrMode );
    }
    
    m_pcEntropyCoder->encodeCoeffNxN( pcCU, pcCoeffCurrY, uiAbsPartIdx,  trWidth,  trHeight,    uiDepth, TEXT_LUMA );
    const UInt uiSingleBitsY = m_pcEntropyCoder->getNumberOfWrittenBits();
    
    UInt uiSingleBitsU = 0;
    UInt uiSingleBitsV = 0;
    if( bCodeChroma )
    {
      {
        m_pcEntropyCoder->encodeQtCbf   ( pcCU, uiAbsPartIdx, TEXT_CHROMA_U, uiTrMode );
      }
      m_pcEntropyCoder->encodeCoeffNxN( pcCU, pcCoeffCurrU, uiAbsPartIdx, trWidthC, trHeightC, uiDepth, TEXT_CHROMA_U );
      uiSingleBitsU = m_pcEntropyCoder->getNumberOfWrittenBits() - uiSingleBitsY;
      
      {
        m_pcEntropyCoder->encodeQtCbf   ( pcCU, uiAbsPartIdx, TEXT_CHROMA_V, uiTrMode );
      }
      m_pcEntropyCoder->encodeCoeffNxN( pcCU, pcCoeffCurrV, uiAbsPartIdx, trWidthC, trHeightC, uiDepth, TEXT_CHROMA_V );
      uiSingleBitsV = m_pcEntropyCoder->getNumberOfWrittenBits() - ( uiSingleBitsY + uiSingleBitsU );
    }
    
    const UInt uiNumSamplesLuma = 1 << (uiLog2TrSize<<1);
    const UInt uiNumSamplesChro = 1 << (uiLog2TrSizeC<<1);
    
    Dist uiDistY;

//GT VSO

    // GT Fix: Not necessary for VSO, however used for chroma later, irrelevant except from valgrind error message
    ::memset( m_pTempPel, 0, sizeof( Pel ) * uiNumSamplesLuma ); // not necessary needed for inside of recursion (only at the beginning)
    
#if HHI_VSO
    if ( m_pcRdCost->getUseVSO() )
    {
      uiDistY = m_pcRdCost->getDistVS  ( pcCU, uiAbsPartIdx, pcPred->getLumaAddr( uiAbsPartIdx ), pcPred->getStride(), pcOrg->getLumaAddr( uiAbsPartIdx), pcOrg->getStride(), 1<< uiLog2TrSize, 1<< uiLog2TrSize, false, 0 ); // initialized with zero residual distortion
    }
    else
#endif
    {
#if IBDI_DISTORTION
     uiDistY = m_pcRdCost->getDistPart( pcPred->getLumaAddr( absTUPartIdx ), pcPred->getStride(), pcOrg->getLumaAddr( absTUPartIdx), pcOrg->getStride(), trWidth, trHeight);
#else
     uiDistY = m_pcRdCost->getDistPart( m_pTempPel, trWidth, pcResi->getLumaAddr( absTUPartIdx ), pcResi->getStride(), trWidth, trHeight ); // initialized with zero residual destortion
#endif
    }


    if ( puiZeroDist )
    {
      *puiZeroDist += uiDistY;
    }
    if( uiAbsSumY )
    {
      Pel *pcResiCurrY = m_pcQTTempTComYuv[ uiQTTempAccessLayer ].getLumaAddr( absTUPartIdx );

#if H0736_AVC_STYLE_QP_RANGE
      m_pcTrQuant->setQPforQuant( pcCU->getQP( 0 ), false, pcCU->getSlice()->getSliceType(), TEXT_LUMA, pcCU->getSlice()->getSPS()->getQpBDOffsetY(), 0 );
#else
      m_pcTrQuant->setQPforQuant( pcCU->getQP( 0 ), false, pcCU->getSlice()->getSliceType(), TEXT_LUMA, 0 );
#endif

      Int scalingListType = 3 + g_eTTable[(Int)TEXT_LUMA];
      assert(scalingListType < 6);     
#if LOSSLESS_CODING
      m_pcTrQuant->invtransformNxN( pcCU, TEXT_LUMA,REG_DCT, pcResiCurrY, m_pcQTTempTComYuv[uiQTTempAccessLayer].getStride(),  pcCoeffCurrY, trWidth, trHeight, scalingListType );//this is for inter mode only
#else     
      m_pcTrQuant->invtransformNxN( TEXT_LUMA,REG_DCT, pcResiCurrY, m_pcQTTempTComYuv[uiQTTempAccessLayer].getStride(),  pcCoeffCurrY, trWidth, trHeight, scalingListType );//this is for inter mode only
#endif
      
      Dist uiNonzeroDistY;

#if HHI_VSO      
      if ( m_pcRdCost->getUseVSO() )
      {
        static int iCount = 1; 
        iCount++; 
        m_cYuvRecTemp.addClipPartLuma( &m_pcQTTempTComYuv[uiQTTempAccessLayer], pcPred, uiAbsPartIdx, 1<< uiLog2TrSize  );
        uiNonzeroDistY = m_pcRdCost->getDistVS( pcCU, uiAbsPartIdx, m_cYuvRecTemp.getLumaAddr(uiAbsPartIdx), m_cYuvRecTemp.getStride(),
                                                pcOrg->getLumaAddr( uiAbsPartIdx ), pcOrg->getStride(), 1<< uiLog2TrSize,   1<< uiLog2TrSize, false, 0 );
      }
      else
#endif
      {
        uiNonzeroDistY = m_pcRdCost->getDistPart( m_pcQTTempTComYuv[uiQTTempAccessLayer].getLumaAddr( absTUPartIdx ), m_pcQTTempTComYuv[uiQTTempAccessLayer].getStride(),
          pcResi->getLumaAddr( absTUPartIdx ), pcResi->getStride(), trWidth,trHeight );
      }

#if LOSSLESS_CODING
      if (pcCU->isLosslessCoded(0)) 
      {
        uiDistY = uiNonzeroDistY;
      }
      else
      {

      Double singleCostY;
      Double nullCostY;

#if HHI_VSO      
      if ( m_pcRdCost->getUseLambdaScaleVSO())
      {
        singleCostY = m_pcRdCost->calcRdCostVSO( uiSingleBitsY, uiNonzeroDistY );
        nullCostY   = m_pcRdCost->calcRdCostVSO( 0, uiDistY );
      }
      else
#endif
      {
        singleCostY = m_pcRdCost->calcRdCost( uiSingleBitsY, uiNonzeroDistY );
        nullCostY   = m_pcRdCost->calcRdCost( 0, uiDistY );
      }
        if( nullCostY < singleCostY )  
        {    
          uiAbsSumY = 0;
          ::memset( pcCoeffCurrY, 0, sizeof( TCoeff ) * uiNumSamplesLuma );
        }
        else
        {
          uiDistY = uiNonzeroDistY;
        }
      }
#else
      Double dSingleCostY;
      Double dNullCostY;

#if HHI_VSO      
      if ( m_pcRdCost->getUseLambdaScaleVSO())
      {
        dSingleCostY = m_pcRdCost->calcRdCostVSO( uiSingleBitsY, uiNonzeroDistY );
        dNullCostY   = m_pcRdCost->calcRdCostVSO( 0, uiDistY );
      }
      else
#endif
      {
        dSingleCostY = m_pcRdCost->calcRdCost( uiSingleBitsY, uiNonzeroDistY );
        dNullCostY   = m_pcRdCost->calcRdCost( 0, uiDistY );
      }
      if( dNullCostY < dSingleCostY )
      {
        uiAbsSumY = 0;
        ::memset( pcCoeffCurrY, 0, sizeof( TCoeff ) * uiNumSamplesLuma );
      }
      else
      {
        uiDistY = uiNonzeroDistY;
      }
#endif
    }
    
    if( !uiAbsSumY )
    {
      Pel *pcPtr =  m_pcQTTempTComYuv[uiQTTempAccessLayer].getLumaAddr( absTUPartIdx );
      const UInt uiStride = m_pcQTTempTComYuv[uiQTTempAccessLayer].getStride();
      for( UInt uiY = 0; uiY < trHeight; ++uiY )
      {
        ::memset( pcPtr, 0, sizeof( Pel ) * trWidth );
        pcPtr += uiStride;
      } 
    }
    
    UInt uiDistU = 0;
    UInt uiDistV = 0;
    if( bCodeChroma )
    {
#if IBDI_DISTORTION
      uiDistU = m_pcRdCost->getDistPart( pcPred->getCbAddr( absTUPartIdxC ), pcPred->getCStride(), pcOrg->getCbAddr( absTUPartIdxC ), pcOrg->getCStride(), trWidthC, trHeightC
#if WEIGHTED_CHROMA_DISTORTION
                                          , true
#endif
                                          );
#else
      uiDistU = m_pcRdCost->getDistPart( m_pTempPel, trWidthC, pcResi->getCbAddr( absTUPartIdxC ), pcResi->getCStride(), trWidthC, trHeightC
#if WEIGHTED_CHROMA_DISTORTION
                                          , true
#endif
                                          ); // initialized with zero residual destortion
#endif
      if ( puiZeroDist )
      {
        *puiZeroDist += uiDistU;
      }
      if( uiAbsSumU )
      {
        Pel *pcResiCurrU = m_pcQTTempTComYuv[uiQTTempAccessLayer].getCbAddr( absTUPartIdxC );

#if H0736_AVC_STYLE_QP_RANGE
        m_pcTrQuant->setQPforQuant( pcCU->getQP( 0 ), false, pcCU->getSlice()->getSliceType(), TEXT_CHROMA, pcCU->getSlice()->getSPS()->getQpBDOffsetC(), pcCU->getSlice()->getPPS()->getChromaQpOffset() );
#else
        m_pcTrQuant->setQPforQuant( pcCU->getQP( 0 ), false, pcCU->getSlice()->getSliceType(), TEXT_CHROMA, pcCU->getSlice()->getPPS()->getChromaQpOffset() );
#endif

        Int scalingListType = 3 + g_eTTable[(Int)TEXT_CHROMA_U];
        assert(scalingListType < 6);
#if LOSSLESS_CODING
        m_pcTrQuant->invtransformNxN( pcCU, TEXT_CHROMA,REG_DCT, pcResiCurrU, m_pcQTTempTComYuv[uiQTTempAccessLayer].getCStride(), pcCoeffCurrU, trWidthC, trHeightC, scalingListType  );
#else
        m_pcTrQuant->invtransformNxN( TEXT_CHROMA,REG_DCT, pcResiCurrU, m_pcQTTempTComYuv[uiQTTempAccessLayer].getCStride(), pcCoeffCurrU, trWidthC, trHeightC, scalingListType );
#endif       
        
        const UInt uiNonzeroDistU = m_pcRdCost->getDistPart( m_pcQTTempTComYuv[uiQTTempAccessLayer].getCbAddr( absTUPartIdxC), m_pcQTTempTComYuv[uiQTTempAccessLayer].getCStride(),
          pcResi->getCbAddr( absTUPartIdxC), pcResi->getCStride(), trWidthC, trHeightC
#if WEIGHTED_CHROMA_DISTORTION
          , true
#endif
          );

#if LOSSLESS_CODING
        if(pcCU->isLosslessCoded(0))  
        {
          uiDistU = uiNonzeroDistU;
        }
        else
        {
          const Double dSingleCostU = m_pcRdCost->calcRdCost( uiSingleBitsU, uiNonzeroDistU );
          const Double dNullCostU   = m_pcRdCost->calcRdCost( 0, uiDistU );
          if( dNullCostU < dSingleCostU )
          {
            uiAbsSumU = 0;
            ::memset( pcCoeffCurrU, 0, sizeof( TCoeff ) * uiNumSamplesChro );
          }
          else
          {
            uiDistU = uiNonzeroDistU;
          }
        }
#else
        const Double dSingleCostU = m_pcRdCost->calcRdCost( uiSingleBitsU, uiNonzeroDistU );
        const Double dNullCostU   = m_pcRdCost->calcRdCost( 0, uiDistU );
        if( dNullCostU < dSingleCostU )
        {
          uiAbsSumU = 0;
          ::memset( pcCoeffCurrU, 0, sizeof( TCoeff ) * uiNumSamplesChro );
        }
        else
        {
          uiDistU = uiNonzeroDistU;
        }
#endif
      }
      if( !uiAbsSumU )
      {
        Pel *pcPtr =  m_pcQTTempTComYuv[uiQTTempAccessLayer].getCbAddr( absTUPartIdxC );
          const UInt uiStride = m_pcQTTempTComYuv[uiQTTempAccessLayer].getCStride();
        for( UInt uiY = 0; uiY < trHeightC; ++uiY )
        {
          ::memset( pcPtr, 0, sizeof(Pel) * trWidthC );
          pcPtr += uiStride;
        }
      }
      
#if IBDI_DISTORTION
      uiDistV = m_pcRdCost->getDistPart( pcPred->getCrAddr( absTUPartIdxC ), pcPred->getCStride(), pcOrg->getCrAddr( absTUPartIdxC ), pcOrg->getCStride(), trWidthC, trHeightC
#if WEIGHTED_CHROMA_DISTORTION
                                          , true
#endif
                                          );
#else
      uiDistV = m_pcRdCost->getDistPart( m_pTempPel, trWidthC, pcResi->getCrAddr( absTUPartIdxC), pcResi->getCStride(), trWidthC, trHeightC
#if WEIGHTED_CHROMA_DISTORTION
                                          , true
#endif
                                          ); // initialized with zero residual destortion
#endif
      if ( puiZeroDist )
      {
        *puiZeroDist += uiDistV;
      }
      if( uiAbsSumV )
      {
        Pel *pcResiCurrV = m_pcQTTempTComYuv[uiQTTempAccessLayer].getCrAddr( absTUPartIdxC );
        if( !uiAbsSumU )
        {
#if H0736_AVC_STYLE_QP_RANGE
          m_pcTrQuant->setQPforQuant( pcCU->getQP( 0 ), false, pcCU->getSlice()->getSliceType(), TEXT_CHROMA, pcCU->getSlice()->getSPS()->getQpBDOffsetC(), pcCU->getSlice()->getPPS()->getChromaQpOffset2nd() );
#else
          m_pcTrQuant->setQPforQuant( pcCU->getQP( 0 ), false, pcCU->getSlice()->getSliceType(), TEXT_CHROMA, pcCU->getSlice()->getPPS()->getChromaQpOffset2nd() );
#endif
        }
        Int scalingListType = 3 + g_eTTable[(Int)TEXT_CHROMA_V];
        assert(scalingListType < 6);
#if LOSSLESS_CODING
        m_pcTrQuant->invtransformNxN( pcCU, TEXT_CHROMA,REG_DCT, pcResiCurrV, m_pcQTTempTComYuv[uiQTTempAccessLayer].getCStride(), pcCoeffCurrV, trWidthC, trHeightC, scalingListType );
#else
        m_pcTrQuant->invtransformNxN( TEXT_CHROMA,REG_DCT, pcResiCurrV, m_pcQTTempTComYuv[uiQTTempAccessLayer].getCStride(), pcCoeffCurrV, trWidthC, trHeightC, scalingListType );
#endif
        
        const UInt uiNonzeroDistV = m_pcRdCost->getDistPart( m_pcQTTempTComYuv[uiQTTempAccessLayer].getCrAddr( absTUPartIdxC ), m_pcQTTempTComYuv[uiQTTempAccessLayer].getCStride(),
          pcResi->getCrAddr( absTUPartIdxC ), pcResi->getCStride(), trWidthC, trHeightC
#if WEIGHTED_CHROMA_DISTORTION
                                                   , true
#endif
                                                   );
#if LOSSLESS_CODING
        if (pcCU->isLosslessCoded(0)) 
        {
          uiDistV = uiNonzeroDistV;
        }
        else
        {
          const Double dSingleCostV = m_pcRdCost->calcRdCost( uiSingleBitsV, uiNonzeroDistV );
          const Double dNullCostV   = m_pcRdCost->calcRdCost( 0, uiDistV );
          if( dNullCostV < dSingleCostV )
          {
            uiAbsSumV = 0;
            ::memset( pcCoeffCurrV, 0, sizeof( TCoeff ) * uiNumSamplesChro );
          }
          else
          {
            uiDistV = uiNonzeroDistV;
          }
        }
#else
        const Double dSingleCostV = m_pcRdCost->calcRdCost( uiSingleBitsV, uiNonzeroDistV );
        const Double dNullCostV   = m_pcRdCost->calcRdCost( 0, uiDistV );
        if( dNullCostV < dSingleCostV )
        {
          uiAbsSumV = 0;
          ::memset( pcCoeffCurrV, 0, sizeof( TCoeff ) * uiNumSamplesChro );
        }
        else
        {
          uiDistV = uiNonzeroDistV;
        }
#endif
      }
      if( !uiAbsSumV )
      {
        Pel *pcPtr =  m_pcQTTempTComYuv[uiQTTempAccessLayer].getCrAddr( absTUPartIdxC );
        const UInt uiStride = m_pcQTTempTComYuv[uiQTTempAccessLayer].getCStride();
        for( UInt uiY = 0; uiY < trHeightC; ++uiY )
        {   
          ::memset( pcPtr, 0, sizeof(Pel) * trWidthC );
          pcPtr += uiStride;
        }
      }
    }
    pcCU->setCbfSubParts( uiAbsSumY ? uiSetCbf : 0, TEXT_LUMA, uiAbsPartIdx, uiDepth );
    if( bCodeChroma )
    {
      pcCU->setCbfSubParts( uiAbsSumU ? uiSetCbf : 0, TEXT_CHROMA_U, uiAbsPartIdx, pcCU->getDepth(0)+uiTrModeC );
      pcCU->setCbfSubParts( uiAbsSumV ? uiSetCbf : 0, TEXT_CHROMA_V, uiAbsPartIdx, pcCU->getDepth(0)+uiTrModeC );
    }

    if( m_bUseSBACRD )
    {
      m_pcRDGoOnSbacCoder->load( m_pppcRDSbacCoder[ uiDepth ][ CI_QT_TRAFO_ROOT ] );
    }

    m_pcEntropyCoder->resetBits();

    {
      if( uiLog2TrSize > pcCU->getQuadtreeTULog2MinSizeInCU(uiAbsPartIdx) )
      {
        m_pcEntropyCoder->encodeTransformSubdivFlag( 0, uiDepth );
      }
    }

    {
      if( bCodeChroma )
      {
        m_pcEntropyCoder->encodeQtCbf( pcCU, uiAbsPartIdx, TEXT_CHROMA_U, uiTrMode );
        m_pcEntropyCoder->encodeQtCbf( pcCU, uiAbsPartIdx, TEXT_CHROMA_V, uiTrMode );
      }

      m_pcEntropyCoder->encodeQtCbf( pcCU, uiAbsPartIdx, TEXT_LUMA,     uiTrMode );
    }

    m_pcEntropyCoder->encodeCoeffNxN( pcCU, pcCoeffCurrY, uiAbsPartIdx, trWidth, trHeight,    uiDepth, TEXT_LUMA );

    if( bCodeChroma )
    {
      m_pcEntropyCoder->encodeCoeffNxN( pcCU, pcCoeffCurrU, uiAbsPartIdx, trWidthC, trHeightC, uiDepth, TEXT_CHROMA_U );
      m_pcEntropyCoder->encodeCoeffNxN( pcCU, pcCoeffCurrV, uiAbsPartIdx, trWidthC, trHeightC, uiDepth, TEXT_CHROMA_V );
    }

    uiSingleBits = m_pcEntropyCoder->getNumberOfWrittenBits();

    uiSingleDist = uiDistY + uiDistU + uiDistV;
#if HHI_VSO
    if ( m_pcRdCost->getUseLambdaScaleVSO())
    {
      dSingleCost = m_pcRdCost->calcRdCostVSO( uiSingleBits, uiSingleDist );
    }
    else
#endif
    {
    dSingleCost = m_pcRdCost->calcRdCost( uiSingleBits, uiSingleDist );
  }  
  } // CHECK FULL
  
  // code sub-blocks
  if( bCheckSplit )
  {
    if( m_bUseSBACRD && bCheckFull )
    {
      m_pcRDGoOnSbacCoder->store( m_pppcRDSbacCoder[ uiDepth ][ CI_QT_TRAFO_TEST ] );
      m_pcRDGoOnSbacCoder->load ( m_pppcRDSbacCoder[ uiDepth ][ CI_QT_TRAFO_ROOT ] );
    }
    Dist uiSubdivDist = 0;
    UInt uiSubdivBits = 0;
    Double dSubdivCost = 0.0;
    
    const UInt uiQPartNumSubdiv = pcCU->getPic()->getNumPartInCU() >> ((uiDepth + 1 ) << 1);
    for( UInt ui = 0; ui < 4; ++ui )
    {
      UInt nsAddr = 0;
      nsAddr = pcCU->getNSAbsPartIdx( uiLog2TrSize - 1, uiAbsPartIdx + ui * uiQPartNumSubdiv, absTUPartIdx, ui, uiTrMode + 1 );
#if IBDI_DISTORTION || HHI_VSO
      xEstimateResidualQT( pcCU, ui, uiAbsPartIdx + ui * uiQPartNumSubdiv, nsAddr, pcOrg, pcPred, pcResi, uiDepth + 1, dSubdivCost, uiSubdivBits, uiSubdivDist, bCheckFull ? NULL : puiZeroDist );
#else
      xEstimateResidualQT( pcCU, ui, uiAbsPartIdx + ui * uiQPartNumSubdiv, nsAddr, pcResi, uiDepth + 1, dSubdivCost, uiSubdivBits, uiSubdivDist, bCheckFull ? NULL : puiZeroDist );
#endif
    }
    
    UInt uiYCbf = 0;
    UInt uiUCbf = 0;
    UInt uiVCbf = 0;
    for( UInt ui = 0; ui < 4; ++ui )
    {
      uiYCbf |= pcCU->getCbf( uiAbsPartIdx + ui * uiQPartNumSubdiv, TEXT_LUMA,     uiTrMode + 1 );
      uiUCbf |= pcCU->getCbf( uiAbsPartIdx + ui * uiQPartNumSubdiv, TEXT_CHROMA_U, uiTrMode + 1 );
      uiVCbf |= pcCU->getCbf( uiAbsPartIdx + ui * uiQPartNumSubdiv, TEXT_CHROMA_V, uiTrMode + 1 );
    }
    for( UInt ui = 0; ui < 4 * uiQPartNumSubdiv; ++ui )
    {
      pcCU->getCbf( TEXT_LUMA     )[uiAbsPartIdx + ui] |= uiYCbf << uiTrMode;
      pcCU->getCbf( TEXT_CHROMA_U )[uiAbsPartIdx + ui] |= uiUCbf << uiTrMode;
      pcCU->getCbf( TEXT_CHROMA_V )[uiAbsPartIdx + ui] |= uiVCbf << uiTrMode;
    }
    
    if( m_bUseSBACRD )
    {
      m_pcRDGoOnSbacCoder->load( m_pppcRDSbacCoder[ uiDepth ][ CI_QT_TRAFO_ROOT ] );
    }
    m_pcEntropyCoder->resetBits();
    
    {
      xEncodeResidualQT( pcCU, uiAbsPartIdx, uiDepth, true,  TEXT_LUMA );
      xEncodeResidualQT( pcCU, uiAbsPartIdx, uiDepth, false, TEXT_LUMA );
      xEncodeResidualQT( pcCU, uiAbsPartIdx, uiDepth, false, TEXT_CHROMA_U );
      xEncodeResidualQT( pcCU, uiAbsPartIdx, uiDepth, false, TEXT_CHROMA_V );
    }
    
    uiSubdivBits = m_pcEntropyCoder->getNumberOfWrittenBits();

#if HHI_VSO
    if ( m_pcRdCost->getUseLambdaScaleVSO())
    {
      dSubdivCost  = m_pcRdCost->calcRdCostVSO( uiSubdivBits, uiSubdivDist );
    }
    else
#endif
    {
    dSubdivCost  = m_pcRdCost->calcRdCost( uiSubdivBits, uiSubdivDist );
    }
    
    if( uiYCbf || uiUCbf || uiVCbf || !bCheckFull )
    {
      if( dSubdivCost < dSingleCost )
      {
        rdCost += dSubdivCost;
        ruiBits += uiSubdivBits;
        ruiDist += uiSubdivDist;
        return;
      }
    }
    assert( bCheckFull );
    if( m_bUseSBACRD )
    {
      m_pcRDGoOnSbacCoder->load( m_pppcRDSbacCoder[ uiDepth ][ CI_QT_TRAFO_TEST ] );
    }
  }

#if HHI_VSO
  if ( m_pcRdCost->getUseRenModel() ) //Only done if not split ( see return above )
  {
    UInt  uiWidth     = 1<< uiLog2TrSize;
    UInt  uiHeight    = 1<< uiLog2TrSize;

    Pel*  piSrc;
    UInt  uiSrcStride;

    if ( uiAbsSumY )
    {
      UInt  uiQTLayer   = pcCU->getSlice()->getSPS()->getQuadtreeTULog2MaxSize() - uiLog2TrSize;
      m_cYuvRecTemp.addClipPartLuma( &m_pcQTTempTComYuv[uiQTLayer], pcPred, uiAbsPartIdx, 1<< uiLog2TrSize  );
      piSrc       = m_cYuvRecTemp.getLumaAddr( uiAbsPartIdx );
      uiSrcStride = m_cYuvRecTemp.getStride  ();
    }
    else
    {
      piSrc       = pcPred->getLumaAddr( uiAbsPartIdx );
      uiSrcStride = pcPred->getStride  ();
    }

    m_pcRdCost->setRenModelData( pcCU, uiAbsPartIdx, piSrc, (Int) uiSrcStride, (Int) uiWidth, (Int) uiHeight );
  }
#endif

  rdCost += dSingleCost;
  ruiBits += uiSingleBits;
  ruiDist += uiSingleDist;
  
  pcCU->setTrIdxSubParts( uiTrMode, uiAbsPartIdx, uiDepth );
  
  pcCU->setCbfSubParts( uiAbsSumY ? uiSetCbf : 0, TEXT_LUMA, uiAbsPartIdx, uiDepth );
  if( bCodeChroma )
  {
    pcCU->setCbfSubParts( uiAbsSumU ? uiSetCbf : 0, TEXT_CHROMA_U, uiAbsPartIdx, pcCU->getDepth(0)+uiTrModeC );
    pcCU->setCbfSubParts( uiAbsSumV ? uiSetCbf : 0, TEXT_CHROMA_V, uiAbsPartIdx, pcCU->getDepth(0)+uiTrModeC );
  }
}

Void TEncSearch::xEncodeResidualQT( TComDataCU* pcCU, UInt uiAbsPartIdx, const UInt uiDepth, Bool bSubdivAndCbf, TextType eType )
{
  assert( pcCU->getDepth( 0 ) == pcCU->getDepth( uiAbsPartIdx ) );
  const UInt uiCurrTrMode = uiDepth - pcCU->getDepth( 0 );
  const UInt uiTrMode = pcCU->getTransformIdx( uiAbsPartIdx );
  
  const Bool bSubdiv = uiCurrTrMode != uiTrMode;
  
  const UInt uiLog2TrSize = g_aucConvertToBit[pcCU->getSlice()->getSPS()->getMaxCUWidth() >> uiDepth]+2;

  {
    if( bSubdivAndCbf && uiLog2TrSize <= pcCU->getSlice()->getSPS()->getQuadtreeTULog2MaxSize() && uiLog2TrSize > pcCU->getQuadtreeTULog2MinSizeInCU(uiAbsPartIdx) )
    {
      m_pcEntropyCoder->encodeTransformSubdivFlag( bSubdiv, uiDepth );
    }
  }

  {
    assert( pcCU->getPredictionMode(uiAbsPartIdx) != MODE_INTRA );
    if( bSubdivAndCbf && uiLog2TrSize <= pcCU->getSlice()->getSPS()->getQuadtreeTULog2MaxSize() )
    {
      const Bool bFirstCbfOfCU = uiLog2TrSize == pcCU->getSlice()->getSPS()->getQuadtreeTULog2MaxSize() || uiCurrTrMode == 0;

      if( bFirstCbfOfCU || uiLog2TrSize > 2 )
      {
        if( bFirstCbfOfCU || pcCU->getCbf( uiAbsPartIdx, TEXT_CHROMA_U, uiCurrTrMode - 1 ) )
        {
          m_pcEntropyCoder->encodeQtCbf( pcCU, uiAbsPartIdx, TEXT_CHROMA_U, uiCurrTrMode );
        }
        if( bFirstCbfOfCU || pcCU->getCbf( uiAbsPartIdx, TEXT_CHROMA_V, uiCurrTrMode - 1 ) )
        {
          m_pcEntropyCoder->encodeQtCbf( pcCU, uiAbsPartIdx, TEXT_CHROMA_V, uiCurrTrMode );
        }
      }
      else if( uiLog2TrSize == 2 )
      {
        assert( pcCU->getCbf( uiAbsPartIdx, TEXT_CHROMA_U, uiCurrTrMode ) == pcCU->getCbf( uiAbsPartIdx, TEXT_CHROMA_U, uiCurrTrMode - 1 ) );
        assert( pcCU->getCbf( uiAbsPartIdx, TEXT_CHROMA_V, uiCurrTrMode ) == pcCU->getCbf( uiAbsPartIdx, TEXT_CHROMA_V, uiCurrTrMode - 1 ) );
      }
    }
  }
  
  if( !bSubdiv )
  {
    const UInt uiNumCoeffPerAbsPartIdxIncrement = pcCU->getSlice()->getSPS()->getMaxCUWidth() * pcCU->getSlice()->getSPS()->getMaxCUHeight() >> ( pcCU->getSlice()->getSPS()->getMaxCUDepth() << 1 );
    //assert( 16 == uiNumCoeffPerAbsPartIdxIncrement ); // check
    const UInt uiQTTempAccessLayer = pcCU->getSlice()->getSPS()->getQuadtreeTULog2MaxSize() - uiLog2TrSize;
    TCoeff *pcCoeffCurrY = m_ppcQTTempCoeffY [uiQTTempAccessLayer] +  uiNumCoeffPerAbsPartIdxIncrement * uiAbsPartIdx;
    TCoeff *pcCoeffCurrU = m_ppcQTTempCoeffCb[uiQTTempAccessLayer] + (uiNumCoeffPerAbsPartIdxIncrement * uiAbsPartIdx>>2);
    TCoeff *pcCoeffCurrV = m_ppcQTTempCoeffCr[uiQTTempAccessLayer] + (uiNumCoeffPerAbsPartIdxIncrement * uiAbsPartIdx>>2);
    
    Bool  bCodeChroma   = true;
    UInt  uiTrModeC     = uiTrMode;
    UInt  uiLog2TrSizeC = uiLog2TrSize-1;
    if( uiLog2TrSize == 2 )
    {
      uiLog2TrSizeC++;
      uiTrModeC    --;
      UInt  uiQPDiv = pcCU->getPic()->getNumPartInCU() >> ( ( pcCU->getDepth( 0 ) + uiTrModeC ) << 1 );
      bCodeChroma   = ( ( uiAbsPartIdx % uiQPDiv ) == 0 );
    }
    
    if( bSubdivAndCbf )
    {
      {
        m_pcEntropyCoder->encodeQtCbf( pcCU, uiAbsPartIdx, TEXT_LUMA,     uiTrMode );
      }
    }
    else
    {
      if( eType == TEXT_LUMA     && pcCU->getCbf( uiAbsPartIdx, TEXT_LUMA,     uiTrMode ) )
      {
        Int trWidth  = 1 << uiLog2TrSize;
        Int trHeight = 1 << uiLog2TrSize;
        pcCU->getNSQTSize( uiTrMode, uiAbsPartIdx, trWidth, trHeight );
        m_pcEntropyCoder->encodeCoeffNxN( pcCU, pcCoeffCurrY, uiAbsPartIdx, trWidth, trHeight,    uiDepth, TEXT_LUMA );
      }
      if( bCodeChroma )
      {
        Int trWidth  = 1 << uiLog2TrSizeC;
        Int trHeight = 1 << uiLog2TrSizeC;
        pcCU->getNSQTSize( uiTrMode, uiAbsPartIdx, trWidth, trHeight );
        if( eType == TEXT_CHROMA_U && pcCU->getCbf( uiAbsPartIdx, TEXT_CHROMA_U, uiTrMode ) )
        {
          m_pcEntropyCoder->encodeCoeffNxN( pcCU, pcCoeffCurrU, uiAbsPartIdx, trWidth, trHeight, uiDepth, TEXT_CHROMA_U );
        }
        if( eType == TEXT_CHROMA_V && pcCU->getCbf( uiAbsPartIdx, TEXT_CHROMA_V, uiTrMode ) )
        {
          m_pcEntropyCoder->encodeCoeffNxN( pcCU, pcCoeffCurrV, uiAbsPartIdx, trWidth, trHeight, uiDepth, TEXT_CHROMA_V );
        }
      }
    }
  }
  else
  {
    if( bSubdivAndCbf || pcCU->getCbf( uiAbsPartIdx, eType, uiCurrTrMode ) )
    {
      const UInt uiQPartNumSubdiv = pcCU->getPic()->getNumPartInCU() >> ((uiDepth + 1 ) << 1);
      for( UInt ui = 0; ui < 4; ++ui )
      {
        xEncodeResidualQT( pcCU, uiAbsPartIdx + ui * uiQPartNumSubdiv, uiDepth + 1, bSubdivAndCbf, eType );
      }
    }
  }
}

Void TEncSearch::xSetResidualQTData( TComDataCU* pcCU, UInt uiQuadrant, UInt uiAbsPartIdx, UInt absTUPartIdx, TComYuv* pcResi, UInt uiDepth, Bool bSpatial )
{
  assert( pcCU->getDepth( 0 ) == pcCU->getDepth( uiAbsPartIdx ) );
  const UInt uiCurrTrMode = uiDepth - pcCU->getDepth( 0 );
  const UInt uiTrMode = pcCU->getTransformIdx( uiAbsPartIdx );

  if( uiCurrTrMode == uiTrMode )
  {
    const UInt uiLog2TrSize = g_aucConvertToBit[pcCU->getSlice()->getSPS()->getMaxCUWidth() >> uiDepth]+2;
    const UInt uiQTTempAccessLayer = pcCU->getSlice()->getSPS()->getQuadtreeTULog2MaxSize() - uiLog2TrSize;

    Bool  bCodeChroma   = true;
    UInt  uiTrModeC     = uiTrMode;
    UInt  uiLog2TrSizeC = uiLog2TrSize-1;
    if( uiLog2TrSize == 2 )
    {
      uiLog2TrSizeC++;
      uiTrModeC    --;
      UInt  uiQPDiv = pcCU->getPic()->getNumPartInCU() >> ( ( pcCU->getDepth( 0 ) + uiTrModeC ) << 1 );
      bCodeChroma   = ( ( uiAbsPartIdx % uiQPDiv ) == 0 );
    }

    if( bSpatial )
    {      
      Int trWidth  = 1 << uiLog2TrSize;
      Int trHeight = 1 << uiLog2TrSize;
      pcCU->getNSQTSize( uiTrMode, uiAbsPartIdx, trWidth, trHeight );
      m_pcQTTempTComYuv[uiQTTempAccessLayer].copyPartToPartLuma    ( pcResi, absTUPartIdx, trWidth , trHeight );

      if( bCodeChroma )
      {
        Int trWidthC  = 1 << uiLog2TrSizeC;
        Int trHeightC = 1 << uiLog2TrSizeC;
        UInt absTUPartIdxC = absTUPartIdx;
        pcCU->getNSQTSize( uiTrModeC, uiAbsPartIdx, trWidthC, trHeightC );

        if( pcCU->useNonSquareTrans( uiTrModeC, uiAbsPartIdx ) && !( uiLog2TrSizeC  == pcCU->getSlice()->getSPS()->getQuadtreeTULog2MinSize() && uiTrModeC == 1 ) )
        {          
          absTUPartIdxC = pcCU->getNSAddrChroma( uiLog2TrSizeC, uiTrModeC, uiQuadrant, absTUPartIdx );
          m_pcQTTempTComYuv[uiQTTempAccessLayer].copyPartToPartChroma( pcResi, absTUPartIdxC, trWidthC, trHeightC );
        }
        else
          m_pcQTTempTComYuv[uiQTTempAccessLayer].copyPartToPartChroma( pcResi, uiAbsPartIdx, 1 << uiLog2TrSizeC, 1 << uiLog2TrSizeC );
      }
    }
    else
    {
      UInt    uiNumCoeffPerAbsPartIdxIncrement = pcCU->getSlice()->getSPS()->getMaxCUWidth() * pcCU->getSlice()->getSPS()->getMaxCUHeight() >> ( pcCU->getSlice()->getSPS()->getMaxCUDepth() << 1 );
      UInt    uiNumCoeffY = ( 1 << ( uiLog2TrSize << 1 ) );
      TCoeff* pcCoeffSrcY = m_ppcQTTempCoeffY [uiQTTempAccessLayer] +  uiNumCoeffPerAbsPartIdxIncrement * uiAbsPartIdx;
      TCoeff* pcCoeffDstY = pcCU->getCoeffY() + uiNumCoeffPerAbsPartIdxIncrement * uiAbsPartIdx;
      ::memcpy( pcCoeffDstY, pcCoeffSrcY, sizeof( TCoeff ) * uiNumCoeffY );
#if ADAPTIVE_QP_SELECTION
      Int* pcArlCoeffSrcY = m_ppcQTTempArlCoeffY [uiQTTempAccessLayer] +  uiNumCoeffPerAbsPartIdxIncrement * uiAbsPartIdx;
      Int* pcArlCoeffDstY = pcCU->getArlCoeffY() + uiNumCoeffPerAbsPartIdxIncrement * uiAbsPartIdx;
      ::memcpy( pcArlCoeffDstY, pcArlCoeffSrcY, sizeof( Int ) * uiNumCoeffY );
#endif
      if( bCodeChroma )
      {
        UInt    uiNumCoeffC = ( 1 << ( uiLog2TrSizeC << 1 ) );
        TCoeff* pcCoeffSrcU = m_ppcQTTempCoeffCb[uiQTTempAccessLayer] + (uiNumCoeffPerAbsPartIdxIncrement * uiAbsPartIdx>>2);
        TCoeff* pcCoeffSrcV = m_ppcQTTempCoeffCr[uiQTTempAccessLayer] + (uiNumCoeffPerAbsPartIdxIncrement * uiAbsPartIdx>>2);
        TCoeff* pcCoeffDstU = pcCU->getCoeffCb() + (uiNumCoeffPerAbsPartIdxIncrement * uiAbsPartIdx>>2);
        TCoeff* pcCoeffDstV = pcCU->getCoeffCr() + (uiNumCoeffPerAbsPartIdxIncrement * uiAbsPartIdx>>2);
        ::memcpy( pcCoeffDstU, pcCoeffSrcU, sizeof( TCoeff ) * uiNumCoeffC );
        ::memcpy( pcCoeffDstV, pcCoeffSrcV, sizeof( TCoeff ) * uiNumCoeffC );
#if ADAPTIVE_QP_SELECTION
        Int* pcArlCoeffSrcU = m_ppcQTTempArlCoeffCb[uiQTTempAccessLayer] + (uiNumCoeffPerAbsPartIdxIncrement * uiAbsPartIdx>>2);
        Int* pcArlCoeffSrcV = m_ppcQTTempArlCoeffCr[uiQTTempAccessLayer] + (uiNumCoeffPerAbsPartIdxIncrement * uiAbsPartIdx>>2);
        Int* pcArlCoeffDstU = pcCU->getArlCoeffCb() + (uiNumCoeffPerAbsPartIdxIncrement * uiAbsPartIdx>>2);
        Int* pcArlCoeffDstV = pcCU->getArlCoeffCr() + (uiNumCoeffPerAbsPartIdxIncrement * uiAbsPartIdx>>2);
        ::memcpy( pcArlCoeffDstU, pcArlCoeffSrcU, sizeof( Int ) * uiNumCoeffC );
        ::memcpy( pcArlCoeffDstV, pcArlCoeffSrcV, sizeof( Int ) * uiNumCoeffC );
#endif
      }
    }
  }
  else
  {
    const UInt uiQPartNumSubdiv = pcCU->getPic()->getNumPartInCU() >> ((uiDepth + 1 ) << 1);
    const UInt uiLog2TrSize = g_aucConvertToBit[pcCU->getSlice()->getSPS()->getMaxCUWidth() >> uiDepth] + 2;
    for( UInt ui = 0; ui < 4; ++ui )
    {
      UInt nsAddr = 0;
      nsAddr = pcCU->getNSAbsPartIdx( uiLog2TrSize-1, uiAbsPartIdx + ui * uiQPartNumSubdiv, absTUPartIdx, ui, uiCurrTrMode + 1);
      xSetResidualQTData( pcCU, ui, uiAbsPartIdx + ui * uiQPartNumSubdiv, nsAddr, pcResi, uiDepth + 1, bSpatial );
    }
  }
}

UInt TEncSearch::xModeBitsIntra( TComDataCU* pcCU, UInt uiMode, UInt uiPU, UInt uiPartOffset, UInt uiDepth, UInt uiInitTrDepth )
{
  if( m_bUseSBACRD )
  {
    // Reload only contexts required for coding intra mode information
    m_pcRDGoOnSbacCoder->loadIntraDirModeLuma( m_pppcRDSbacCoder[uiDepth][CI_CURR_BEST] );
  }
  
  pcCU->setLumaIntraDirSubParts ( uiMode, uiPartOffset, uiDepth + uiInitTrDepth );
  
  m_pcEntropyCoder->resetBits();
  m_pcEntropyCoder->encodeIntraDirModeLuma ( pcCU, uiPartOffset);
  
  return m_pcEntropyCoder->getNumberOfWrittenBits();
}

UInt TEncSearch::xUpdateCandList( UInt uiMode, Double uiCost, UInt uiFastCandNum, UInt * CandModeList, Double * CandCostList )
{
  UInt i;
  UInt shift=0;
  
  while ( shift<uiFastCandNum && uiCost<CandCostList[ uiFastCandNum-1-shift ] ) shift++;
  
  if( shift!=0 )
  {
    for(i=1; i<shift; i++)
    {
      CandModeList[ uiFastCandNum-i ] = CandModeList[ uiFastCandNum-1-i ];
      CandCostList[ uiFastCandNum-i ] = CandCostList[ uiFastCandNum-1-i ];
    }
    CandModeList[ uiFastCandNum-shift ] = uiMode;
    CandCostList[ uiFastCandNum-shift ] = uiCost;
    return 1;
  }
  
  return 0;
}

/** add inter-prediction syntax elements for a CU block
 * \param pcCU
 * \param uiQp
 * \param uiTrMode
 * \param ruiBits
 * \param rpcYuvRec
 * \param pcYuvPred
 * \param rpcYuvResi
 * \returns Void
 */
Void  TEncSearch::xAddSymbolBitsInter( TComDataCU* pcCU, UInt uiQp, UInt uiTrMode, UInt& ruiBits, TComYuv*& rpcYuvRec, TComYuv*pcYuvPred, TComYuv*& rpcYuvResi )
{
  if ( pcCU->isSkipped( 0 ) )
  {
#if HHI_MPI
    if( pcCU->getTextureModeDepth( 0 ) != -1 )
    {
      return;
    }
#endif
    m_pcEntropyCoder->resetBits();
    m_pcEntropyCoder->encodeSkipFlag(pcCU, 0, true);
    m_pcEntropyCoder->encodeMergeIndex(pcCU, 0, 0, true);
#if HHI_INTER_VIEW_RESIDUAL_PRED
    m_pcEntropyCoder->encodeResPredFlag( pcCU, 0, 0, true );
#endif
    ruiBits += m_pcEntropyCoder->getNumberOfWrittenBits();
  }
  else
  {
    m_pcEntropyCoder->resetBits();
#if HHI_MPI
    if( pcCU->getTextureModeDepth( 0 ) == -1 )
    {
#endif
    m_pcEntropyCoder->encodeSkipFlag ( pcCU, 0, true );
    if (pcCU->getPredictionMode(0) == MODE_SKIP)
    {
      pcCU->setPredModeSubParts( MODE_INTER, 0, pcCU->getDepth(0) );
    }
    m_pcEntropyCoder->encodePredMode( pcCU, 0, true );
    m_pcEntropyCoder->encodePartSize( pcCU, 0, pcCU->getDepth(0), true );
    m_pcEntropyCoder->encodePredInfo( pcCU, 0, true );
#if HHI_INTER_VIEW_RESIDUAL_PRED
    m_pcEntropyCoder->encodeResPredFlag( pcCU, 0, 0, true );
#endif
#if HHI_MPI
    }
#endif
    Bool bDummy = false;
    m_pcEntropyCoder->encodeCoeff   ( pcCU, 0, pcCU->getDepth(0), pcCU->getWidth(0), pcCU->getHeight(0), bDummy );
    
    ruiBits += m_pcEntropyCoder->getNumberOfWrittenBits();
  }
}

/**
 * \brief Generate half-sample interpolated block
 *
 * \param pattern Reference picture ROI
 * \param biPred    Flag indicating whether block is for biprediction
 */
Void TEncSearch::xExtDIFUpSamplingH( TComPattern* pattern, Bool biPred )
{
  Int width      = pattern->getROIYWidth();
  Int height     = pattern->getROIYHeight();
  Int srcStride  = pattern->getPatternLStride();
  
  Int intStride = m_filteredBlockTmp[0].getStride();
  Int dstStride = m_filteredBlock[0][0].getStride();
  Short *intPtr;
  Short *dstPtr;
  Int filterSize = NTAPS_LUMA;
  Int halfFilterSize = (filterSize>>1);
  Pel *srcPtr = pattern->getROIY() - halfFilterSize*srcStride - 1;
  
  m_if.filterHorLuma(srcPtr, srcStride, m_filteredBlockTmp[0].getLumaAddr(), intStride, width+1, height+filterSize, 0, false);
  m_if.filterHorLuma(srcPtr, srcStride, m_filteredBlockTmp[2].getLumaAddr(), intStride, width+1, height+filterSize, 2, false);
  
  intPtr = m_filteredBlockTmp[0].getLumaAddr() + halfFilterSize * intStride + 1;  
  dstPtr = m_filteredBlock[0][0].getLumaAddr();
  m_if.filterVerLuma(intPtr, intStride, dstPtr, dstStride, width+0, height+0, 0, false, true);
  
  intPtr = m_filteredBlockTmp[0].getLumaAddr() + (halfFilterSize-1) * intStride + 1;  
  dstPtr = m_filteredBlock[2][0].getLumaAddr();
  m_if.filterVerLuma(intPtr, intStride, dstPtr, dstStride, width+0, height+1, 2, false, true);
  
  intPtr = m_filteredBlockTmp[2].getLumaAddr() + halfFilterSize * intStride;
  dstPtr = m_filteredBlock[0][2].getLumaAddr();
  m_if.filterVerLuma(intPtr, intStride, dstPtr, dstStride, width+1, height+0, 0, false, true);
  
  intPtr = m_filteredBlockTmp[2].getLumaAddr() + (halfFilterSize-1) * intStride;
  dstPtr = m_filteredBlock[2][2].getLumaAddr();
  m_if.filterVerLuma(intPtr, intStride, dstPtr, dstStride, width+1, height+1, 2, false, true);
}

/**
 * \brief Generate quarter-sample interpolated blocks
 *
 * \param pattern    Reference picture ROI
 * \param halfPelRef Half-pel mv
 * \param biPred     Flag indicating whether block is for biprediction
 */
Void TEncSearch::xExtDIFUpSamplingQ( TComPattern* pattern, TComMv halfPelRef, Bool biPred )
{
  Int width      = pattern->getROIYWidth();
  Int height     = pattern->getROIYHeight();
  Int srcStride  = pattern->getPatternLStride();
  
  Pel *srcPtr;
  Int intStride = m_filteredBlockTmp[0].getStride();
  Int dstStride = m_filteredBlock[0][0].getStride();
  Short *intPtr;
  Short *dstPtr;
  Int filterSize = NTAPS_LUMA;
  
  Int halfFilterSize = (filterSize>>1);

  Int extHeight = (halfPelRef.getVer() == 0) ? height + filterSize : height + filterSize-1;
  
  // Horizontal filter 1/4
  srcPtr = pattern->getROIY() - halfFilterSize * srcStride - 1;
  intPtr = m_filteredBlockTmp[1].getLumaAddr();
  if (halfPelRef.getVer() > 0)
  {
    srcPtr += srcStride;
  }
  if (halfPelRef.getHor() >= 0)
  {
    srcPtr += 1;
  }
  m_if.filterHorLuma(srcPtr, srcStride, intPtr, intStride, width, extHeight, 1, false);
  
  // Horizontal filter 3/4
  srcPtr = pattern->getROIY() - halfFilterSize*srcStride - 1;
  intPtr = m_filteredBlockTmp[3].getLumaAddr();
  if (halfPelRef.getVer() > 0)
  {
    srcPtr += srcStride;
  }
  if (halfPelRef.getHor() > 0)
  {
    srcPtr += 1;
  }
  m_if.filterHorLuma(srcPtr, srcStride, intPtr, intStride, width, extHeight, 3, false);        
  
  // Generate @ 1,1
  intPtr = m_filteredBlockTmp[1].getLumaAddr() + (halfFilterSize-1) * intStride;
  dstPtr = m_filteredBlock[1][1].getLumaAddr();
  if (halfPelRef.getVer() == 0)
  {
    intPtr += intStride;
  }
  m_if.filterVerLuma(intPtr, intStride, dstPtr, dstStride, width, height, 1, false, true);
  
  // Generate @ 3,1
  intPtr = m_filteredBlockTmp[1].getLumaAddr() + (halfFilterSize-1) * intStride;
  dstPtr = m_filteredBlock[3][1].getLumaAddr();
  m_if.filterVerLuma(intPtr, intStride, dstPtr, dstStride, width, height, 3, false, true);
  
  if (halfPelRef.getVer() != 0)
  {
    // Generate @ 2,1
    intPtr = m_filteredBlockTmp[1].getLumaAddr() + (halfFilterSize-1) * intStride;
    dstPtr = m_filteredBlock[2][1].getLumaAddr();
    if (halfPelRef.getVer() == 0)
    {
      intPtr += intStride;
    }
    m_if.filterVerLuma(intPtr, intStride, dstPtr, dstStride, width, height, 2, false, true);
    
    // Generate @ 2,3
    intPtr = m_filteredBlockTmp[3].getLumaAddr() + (halfFilterSize-1) * intStride;
    dstPtr = m_filteredBlock[2][3].getLumaAddr();
    if (halfPelRef.getVer() == 0)
    {
      intPtr += intStride;
    }
    m_if.filterVerLuma(intPtr, intStride, dstPtr, dstStride, width, height, 2, false, true);
  }
  else
  {
    // Generate @ 0,1
    intPtr = m_filteredBlockTmp[1].getLumaAddr() + halfFilterSize * intStride;
    dstPtr = m_filteredBlock[0][1].getLumaAddr();
    m_if.filterVerLuma(intPtr, intStride, dstPtr, dstStride, width, height, 0, false, true);
    
    // Generate @ 0,3
    intPtr = m_filteredBlockTmp[3].getLumaAddr() + halfFilterSize * intStride;
    dstPtr = m_filteredBlock[0][3].getLumaAddr();
    m_if.filterVerLuma(intPtr, intStride, dstPtr, dstStride, width, height, 0, false, true);
  }
  
  if (halfPelRef.getHor() != 0)
  {
    // Generate @ 1,2
    intPtr = m_filteredBlockTmp[2].getLumaAddr() + (halfFilterSize-1) * intStride;
    dstPtr = m_filteredBlock[1][2].getLumaAddr();
    if (halfPelRef.getHor() > 0)
    {
      intPtr += 1;
    }
    if (halfPelRef.getVer() >= 0)
    {
      intPtr += intStride;
    }
    m_if.filterVerLuma(intPtr, intStride, dstPtr, dstStride, width, height, 1, false, true);
    
    // Generate @ 3,2
    intPtr = m_filteredBlockTmp[2].getLumaAddr() + (halfFilterSize-1) * intStride;
    dstPtr = m_filteredBlock[3][2].getLumaAddr();
    if (halfPelRef.getHor() > 0)
    {
      intPtr += 1;
    }
    if (halfPelRef.getVer() > 0)
    {
      intPtr += intStride;
    }
    m_if.filterVerLuma(intPtr, intStride, dstPtr, dstStride, width, height, 3, false, true);  
  }
  else
  {
    // Generate @ 1,0
    intPtr = m_filteredBlockTmp[0].getLumaAddr() + (halfFilterSize-1) * intStride + 1;
    dstPtr = m_filteredBlock[1][0].getLumaAddr();
    if (halfPelRef.getVer() >= 0)
    {
      intPtr += intStride;
    }
    m_if.filterVerLuma(intPtr, intStride, dstPtr, dstStride, width, height, 1, false, true);
    
    // Generate @ 3,0
    intPtr = m_filteredBlockTmp[0].getLumaAddr() + (halfFilterSize-1) * intStride + 1;
    dstPtr = m_filteredBlock[3][0].getLumaAddr();
    if (halfPelRef.getVer() > 0)
    {
      intPtr += intStride;
    }
    m_if.filterVerLuma(intPtr, intStride, dstPtr, dstStride, width, height, 3, false, true);
  }
  
  // Generate @ 1,3
  intPtr = m_filteredBlockTmp[3].getLumaAddr() + (halfFilterSize-1) * intStride;
  dstPtr = m_filteredBlock[1][3].getLumaAddr();
  if (halfPelRef.getVer() == 0)
  {
    intPtr += intStride;
  }
  m_if.filterVerLuma(intPtr, intStride, dstPtr, dstStride, width, height, 1, false, true);
  
  // Generate @ 3,3
  intPtr = m_filteredBlockTmp[3].getLumaAddr() + (halfFilterSize-1) * intStride;
  dstPtr = m_filteredBlock[3][3].getLumaAddr();
  m_if.filterVerLuma(intPtr, intStride, dstPtr, dstStride, width, height, 3, false, true);
}

/** set wp tables
 * \param TComDataCU* pcCU
 * \param iRefIdx
 * \param eRefPicListCur
 * \returns Void
 */
Void  TEncSearch::setWpScalingDistParam( TComDataCU* pcCU, Int iRefIdx, RefPicList eRefPicListCur )
{
  if ( iRefIdx<0 )
  {
    m_cDistParam.bApplyWeight = false;
    return;
  }

  TComSlice       *pcSlice  = pcCU->getSlice();
  TComPPS         *pps      = pcCU->getSlice()->getPPS();
  wpScalingParam  *wp0 , *wp1;

  m_cDistParam.bApplyWeight = ( pcSlice->getSliceType()==P_SLICE && pps->getUseWP() ) || ( pcSlice->getSliceType()==B_SLICE && pps->getWPBiPredIdc() ) ;

  if ( !m_cDistParam.bApplyWeight ) return;

  Int iRefIdx0 = ( eRefPicListCur == REF_PIC_LIST_0 ) ? iRefIdx : (-1);
  Int iRefIdx1 = ( eRefPicListCur == REF_PIC_LIST_1 ) ? iRefIdx : (-1);

  getWpScaling( pcCU, iRefIdx0, iRefIdx1, wp0 , wp1 );

  if ( iRefIdx0 < 0 ) wp0 = NULL;
  if ( iRefIdx1 < 0 ) wp1 = NULL;

  m_cDistParam.wpCur  = NULL;

  if ( eRefPicListCur == REF_PIC_LIST_0 )
  {
    m_cDistParam.wpCur = wp0;
  }
  else
  {
    m_cDistParam.wpCur = wp1;
  }
}

#if HHI_DMM_WEDGE_INTRA || HHI_DMM_PRED_TEX
Bool TEncSearch::predIntraLumaDMMAvailable( UInt uiMode, UInt uiWidth, UInt uiHeight )
{
  if( uiMode < NUM_INTRA_MODE ) return true;

  Bool bDMMAvailable = m_pcEncCfg->getUseDMM();

#if HHI_DMM_WEDGE_INTRA
  if( uiMode == DMM_WEDGE_FULL_IDX        ||
      uiMode == DMM_WEDGE_FULL_D_IDX      ||
      uiMode == DMM_WEDGE_PREDDIR_IDX     ||
      uiMode == DMM_WEDGE_PREDDIR_D_IDX )
  {
    if( (uiWidth != uiHeight) || (uiWidth < DMM_WEDGEMODEL_MIN_SIZE) || (uiWidth > DMM_WEDGEMODEL_MAX_SIZE) || ( ( uiMode == DMM_WEDGE_PREDDIR_IDX || uiMode == DMM_WEDGE_PREDDIR_D_IDX ) && uiWidth == 4 ) )
    {
      bDMMAvailable = false;
    }
  }
#endif
#if HHI_DMM_PRED_TEX
  if( uiMode == DMM_WEDGE_PREDTEX_IDX     ||
      uiMode == DMM_WEDGE_PREDTEX_D_IDX   ||
      uiMode == DMM_CONTOUR_PREDTEX_IDX   ||
      uiMode == DMM_CONTOUR_PREDTEX_D_IDX )
  {
    if( (uiWidth != uiHeight) || (uiWidth < DMM_WEDGEMODEL_MIN_SIZE) || (uiWidth > DMM_WEDGEMODEL_MAX_SIZE) || ( ( uiMode == DMM_CONTOUR_PREDTEX_IDX || uiMode == DMM_CONTOUR_PREDTEX_D_IDX ) && uiWidth == 4 ) )
    {
      bDMMAvailable = false;
    }
  }
#endif

  return bDMMAvailable;
}

Void TEncSearch::xGetWedgeDeltaDCsMinDist( TComWedgelet* pcWedgelet,
                                           TComDataCU*   pcCU,
                                           UInt          uiAbsPtIdx,
                                           Pel*          piOrig,
                                           Pel*          piPredic,
                                           UInt          uiStride,
                                           UInt          uiWidth,
                                           UInt          uiHeight,
                                           Int&          riDeltaDC1,
                                           Int&          riDeltaDC2,
                                           Bool          bAboveAvail,
                                           Bool          bLeftAvail )
{
  Int iDC1 = 0;
  Int iDC2 = 0;
  calcWedgeDCs       ( pcWedgelet, piOrig,   uiStride, iDC1, iDC2 );
  assignWedgeDCs2Pred( pcWedgelet, piPredic, uiStride, iDC1, iDC2 );

  Int iPredDC1 = 0;
  Int iPredDC2 = 0;
  Int* piMask = pcCU->getPattern()->getAdiOrgBuf( uiWidth, uiHeight, m_piYuvExt );
  Int iMaskStride = ( uiWidth<<1 ) + 1;
  piMask += iMaskStride+1;
  getWedgePredDCs( pcWedgelet, piMask, iMaskStride, iPredDC1, iPredDC2, bAboveAvail, bLeftAvail );

  riDeltaDC1 = iDC1 - iPredDC1;
  riDeltaDC2 = iDC2 - iPredDC2;

#if HHI_VSO
  if( m_pcRdCost->getUseVSO() )
  {
    Int iFullDeltaDC1 = riDeltaDC1;
    Int iFullDeltaDC2 = riDeltaDC2;

    xDeltaDCQuantScaleDown( pcCU, iFullDeltaDC1 );
    xDeltaDCQuantScaleDown( pcCU, iFullDeltaDC2 );

    Dist uiBestDist     = RDO_DIST_MAX;
    UInt  uiBestQStepDC1 = 0;
    UInt  uiBestQStepDC2 = 0;

    UInt uiDeltaDC1Max = abs(iFullDeltaDC1);
    UInt uiDeltaDC2Max = abs(iFullDeltaDC2);

    //VSO Level delta DC check range extension
    uiDeltaDC1Max += (uiDeltaDC1Max>>1);
    uiDeltaDC2Max += (uiDeltaDC2Max>>1);

    for( UInt uiQStepDC1 = 1; uiQStepDC1 <= uiDeltaDC1Max; uiQStepDC1++  )
    {
      Int iLevelDeltaDC1 = (Int)(uiQStepDC1) * (Int)(( iFullDeltaDC1 < 0 ) ? -1 : 1);
      xDeltaDCQuantScaleUp( pcCU, iLevelDeltaDC1 );

      Int iTestDC1 = Clip( iPredDC1 + iLevelDeltaDC1 );
      for( UInt uiQStepDC2 = 1; uiQStepDC2 <= uiDeltaDC2Max; uiQStepDC2++  )
      {
        Int iLevelDeltaDC2 = (Int)(uiQStepDC2) * (Int)(( iFullDeltaDC2 < 0 ) ? -1 : 1);
        xDeltaDCQuantScaleUp( pcCU, iLevelDeltaDC2 );

        Int iTestDC2 = Clip( iPredDC2 + iLevelDeltaDC2 );

        assignWedgeDCs2Pred( pcWedgelet, piPredic, uiStride, iTestDC1, iTestDC2 );

        Dist uiActDist = m_pcRdCost->getDistVS( pcCU, 0, piPredic, uiStride,  piOrig, uiStride, uiWidth, uiHeight, false, 0 );
        if( uiActDist < uiBestDist || uiBestDist == RDO_DIST_MAX )
        {
          uiBestDist     = uiActDist;
          uiBestQStepDC1 = uiQStepDC1;
          uiBestQStepDC2 = uiQStepDC2;
        }
      }
    }

    iFullDeltaDC1 = (Int)(uiBestQStepDC1) * (Int)(( iFullDeltaDC1 < 0 ) ? -1 : 1);
    iFullDeltaDC2 = (Int)(uiBestQStepDC2) * (Int)(( iFullDeltaDC2 < 0 ) ? -1 : 1);
    xDeltaDCQuantScaleUp( pcCU, iFullDeltaDC1 );
    xDeltaDCQuantScaleUp( pcCU, iFullDeltaDC2 );
    riDeltaDC1 = iFullDeltaDC1;
    riDeltaDC2 = iFullDeltaDC2;
  }
#endif

  xDeltaDCQuantScaleDown( pcCU, riDeltaDC1 );
  xDeltaDCQuantScaleDown( pcCU, riDeltaDC2 );
}
#endif
#if HHI_DMM_WEDGE_INTRA
Void TEncSearch::findWedgeFullMinDist( TComDataCU*  pcCU,
                                       UInt         uiAbsPtIdx,
                                       Pel*         piOrig,
                                       Pel*         piPredic,
                                       UInt         uiStride,
                                       UInt         uiWidth,
                                       UInt         uiHeight,
                                       UInt&        ruiTabIdx,
                                       Int&         riDeltaDC1,
                                       Int&         riDeltaDC2,
                                       Bool         bAboveAvail,
                                       Bool         bLeftAvail )
{
  assert( uiWidth >= DMM_WEDGEMODEL_MIN_SIZE && uiWidth <= DMM_WEDGEMODEL_MAX_SIZE );

  WedgeList* pacWedgeList = &g_aacWedgeLists[(g_aucConvertToBit[uiWidth])];
  Dist iDist = RDO_DIST_MAX;
  xSearchWedgeFullMinDist( pcCU, uiAbsPtIdx, pacWedgeList, piOrig, uiStride, uiWidth, uiHeight, ruiTabIdx, iDist );

  TComWedgelet* pcBestWedgelet = &(pacWedgeList->at(ruiTabIdx));
  xGetWedgeDeltaDCsMinDist( pcBestWedgelet, pcCU, uiAbsPtIdx, piOrig, piPredic, uiStride, uiWidth, uiHeight, riDeltaDC1, riDeltaDC2, bAboveAvail, bLeftAvail );
}

Void TEncSearch::findWedgePredDirMinDist( TComDataCU*  pcCU,
                                          UInt         uiAbsPtIdx,
                                          Pel*         piOrig,
                                          Pel*         piPredic,
                                          UInt         uiStride,
                                          UInt         uiWidth,
                                          UInt         uiHeight,
                                          UInt&        ruiTabIdx,
                                          Int&         riWedgeDeltaEnd,
                                          Int&         riDeltaDC1,
                                          Int&         riDeltaDC2,
                                          Bool         bAboveAvail,
                                          Bool         bLeftAvail )
{
  assert( uiWidth >= DMM_WEDGEMODEL_MIN_SIZE && uiWidth <= DMM_WEDGEMODEL_MAX_SIZE );
  WedgeList* pacWedgeList = &g_aacWedgeLists[(g_aucConvertToBit[uiWidth])];

  ruiTabIdx       = 0;
  riWedgeDeltaEnd = 0;

  xSearchWedgePredDirMinDist( pcCU, uiAbsPtIdx, pacWedgeList, piOrig, uiStride, uiWidth, uiHeight, ruiTabIdx, riWedgeDeltaEnd );

  TComWedgelet* pcBestWedgelet = &(pacWedgeList->at(ruiTabIdx));
  xGetWedgeDeltaDCsMinDist( pcBestWedgelet, pcCU, uiAbsPtIdx, piOrig, piPredic, uiStride, uiWidth, uiHeight, riDeltaDC1, riDeltaDC2, bAboveAvail, bLeftAvail );
}

Void TEncSearch::xSearchWedgeFullMinDist( TComDataCU* pcCU, UInt uiAbsPtIdx, WedgeList* pacWedgeList, Pel* piRef, UInt uiRefStride, UInt uiWidth, UInt uiHeight, UInt& ruiTabIdx, Dist& riDist )
{
  ruiTabIdx = 0;

  // local pred buffer
  TComYuv cPredYuv;
  cPredYuv.create( uiWidth, uiHeight );
  cPredYuv.clear();

  UInt uiPredStride = cPredYuv.getStride();
  Pel* piPred       = cPredYuv.getLumaAddr();

  Int  iDC1 = 0;
  Int  iDC2 = 0;
  // regular wedge search
  Dist uiBestDist   = RDO_DIST_MAX;
  UInt uiBestTabIdx = 0;

  for( UInt uiIdx = 0; uiIdx < pacWedgeList->size(); uiIdx++ )
  {
    calcWedgeDCs       ( &(pacWedgeList->at(uiIdx)), piRef,  uiRefStride,  iDC1, iDC2 );
    assignWedgeDCs2Pred( &(pacWedgeList->at(uiIdx)), piPred, uiPredStride, iDC1, iDC2 );

    Dist uiActDist = RDO_DIST_MAX;
#if HHI_VSO
    if( m_pcRdCost->getUseVSO() )
    {
      uiActDist = m_pcRdCost->getDistVS( pcCU, 0, piPred, uiPredStride, piRef, uiRefStride, uiWidth, uiHeight, false, 0 );
    }
    else
    {
      uiActDist = m_pcRdCost->getDistPart( piPred, uiPredStride, piRef, uiRefStride, uiWidth, uiHeight, false, DF_SAD );
    }
#else
    uiActDist = m_pcRdCost->getDistPart( piPred, uiPredStride, piRef, uiRefStride, uiWidth, uiHeight, false, DF_SAD );
#endif

    if( uiActDist < uiBestDist || uiBestDist == RDO_DIST_MAX )
    {
      uiBestDist   = uiActDist;
      uiBestTabIdx = uiIdx;
    }
  }
  ruiTabIdx = uiBestTabIdx;
  riDist    = uiBestDist;

  cPredYuv.destroy();
  return;
}

Void TEncSearch::xSearchWedgePredDirMinDist( TComDataCU* pcCU, UInt uiAbsPtIdx, WedgeList* pacWedgeList, Pel* piRef, UInt uiRefStride, UInt uiWidth, UInt uiHeight, UInt& ruiTabIdx, Int& riWedgeDeltaEnd )
{
  ruiTabIdx       = 0;
  riWedgeDeltaEnd = 0;

  // local pred buffer
  TComYuv cPredYuv;
  cPredYuv.create( uiWidth, uiHeight );
  cPredYuv.clear();

  UInt uiPredStride = cPredYuv.getStride();
  Pel* piPred       = cPredYuv.getLumaAddr();

  Int  iDC1 = 0;
  Int  iDC2 = 0;

  // regular wedge search
  Dist uiBestDist    = RDO_DIST_MAX;
  UInt uiBestTabIdx  = 0;
  Int  iBestDeltaEnd = 0;

  UInt uiIdx = 0;
  for( Int iTestDeltaEnd = -DMM_WEDGE_PREDDIR_DELTAEND_MAX; iTestDeltaEnd <= DMM_WEDGE_PREDDIR_DELTAEND_MAX; iTestDeltaEnd++ )
  {
    uiIdx = getBestContinueWedge( pcCU, uiAbsPtIdx, uiWidth, uiHeight, iTestDeltaEnd );
    calcWedgeDCs       ( &(pacWedgeList->at(uiIdx)), piRef,  uiRefStride,  iDC1, iDC2 );
    assignWedgeDCs2Pred( &(pacWedgeList->at(uiIdx)), piPred, uiPredStride, iDC1, iDC2 );

    Dist uiActDist = RDO_DIST_MAX;
#if HHI_VSO
    if( m_pcRdCost->getUseVSO() )
    {
      uiActDist = m_pcRdCost->getDistVS( pcCU, 0, piPred, uiPredStride, piRef, uiRefStride, uiWidth, uiHeight, false, 0 );
    }
    else
    {
      uiActDist = m_pcRdCost->getDistPart( piPred, uiPredStride, piRef, uiRefStride, uiWidth, uiHeight, false, DF_SAD );
    }
#else
    uiActDist = m_pcRdCost->getDistPart( piPred, uiPredStride, piRef, uiRefStride, uiWidth, uiHeight, false, DF_SAD );
#endif

    if( uiActDist < uiBestDist || uiBestDist == RDO_DIST_MAX )
    {
      uiBestDist    = uiActDist;
      uiBestTabIdx  = uiIdx;
      iBestDeltaEnd = iTestDeltaEnd;
    }
    else if( uiIdx == uiBestTabIdx && abs(iTestDeltaEnd) < abs(iBestDeltaEnd) )
    {
      iBestDeltaEnd = iTestDeltaEnd;
    }
  }

  ruiTabIdx       = uiBestTabIdx;
  riWedgeDeltaEnd = iBestDeltaEnd;

  cPredYuv.destroy();
  return;
}
#endif
#if HHI_DMM_PRED_TEX
Void TEncSearch::findWedgeTexMinDist( TComDataCU*  pcCU, 
                                      UInt         uiAbsPtIdx, 
                                      Pel*         piOrig, 
                                      Pel*         piPredic, 
                                      UInt         uiStride, 
                                      UInt         uiWidth, 
                                      UInt         uiHeight, 
                                      UInt&        ruiTabIdx, 
                                      Int&         riDeltaDC1, 
                                      Int&         riDeltaDC2, 
                                      Bool         bAboveAvail, 
                                      Bool         bLeftAvail )
{
  assert( uiWidth >= DMM_WEDGEMODEL_MIN_SIZE && uiWidth <= DMM_WEDGEMODEL_MAX_SIZE );
  WedgeList* pacWedgeList = &g_aacWedgeLists[(g_aucConvertToBit[uiWidth])];

  ruiTabIdx = getBestWedgeFromTex( pcCU, uiAbsPtIdx, uiWidth, uiHeight );

  TComWedgelet* pcBestWedgelet = &(pacWedgeList->at(ruiTabIdx));
  xGetWedgeDeltaDCsMinDist( pcBestWedgelet, pcCU, uiAbsPtIdx, piOrig, piPredic, uiStride, uiWidth, uiHeight, riDeltaDC1, riDeltaDC2, bAboveAvail, bLeftAvail );
}

Void TEncSearch::findContourPredTex( TComDataCU*  pcCU,
                                     UInt         uiAbsPtIdx,
                                     Pel*         piOrig,
                                     Pel*         piPredic,
                                     UInt         uiStride,
                                     UInt         uiWidth,
                                     UInt         uiHeight,
                                     Int&         riDeltaDC1,
                                     Int&         riDeltaDC2,
                                     Bool         bAboveAvail,
                                     Bool         bLeftAvail )
{
  // get contour pattern
  TComWedgelet* pcContourWedge = new TComWedgelet( uiWidth, uiHeight );
  getBestContourFromTex( pcCU, uiAbsPtIdx, uiWidth, uiHeight, pcContourWedge );

  xGetWedgeDeltaDCsMinDist( pcContourWedge, pcCU, uiAbsPtIdx, piOrig, piPredic, uiStride, uiWidth, uiHeight, riDeltaDC1, riDeltaDC2, bAboveAvail, bLeftAvail );

  pcContourWedge->destroy();
  delete pcContourWedge;
}
#endif
//! \}