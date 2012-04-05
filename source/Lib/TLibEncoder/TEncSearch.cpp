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


/** \file     TEncSearch.cpp
 \brief    encoder search class
 */

#include "../TLibCommon/TypeDef.h"
#include "../TLibCommon/TComMotionInfo.h"
#include "TEncSearch.h"

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
  m_puhQTTempTrIdx   = NULL;
  m_puhQTTempCbf[0] = m_puhQTTempCbf[1] = m_puhQTTempCbf[2] = NULL;
  m_pcQTTempTComYuv  = NULL;
  m_pcEncCfg = NULL;
  m_pcEntropyCoder = NULL;
  m_pTempPel = NULL;
#ifdef WEIGHT_PRED
  setWpScalingDistParam( NULL, -1, -1, REF_PIC_LIST_X );
#endif
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
      m_pcQTTempTComYuv[ui].destroy();
    }
  }
  delete[] m_ppcQTTempCoeffY;
  delete[] m_ppcQTTempCoeffCb;
  delete[] m_ppcQTTempCoeffCr;
  delete[] m_pcQTTempCoeffY;
  delete[] m_pcQTTempCoeffCb;
  delete[] m_pcQTTempCoeffCr;
  delete[] m_puhQTTempTrIdx;
  delete[] m_puhQTTempCbf[0];
  delete[] m_puhQTTempCbf[1];
  delete[] m_puhQTTempCbf[2];
  delete[] m_pcQTTempTComYuv;
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
  m_pcRdCost->initRateDistortionModel( m_iSearchRange << 2 );

  for( Int iNum = 0; iNum < AMVP_MAX_NUM_CANDS+1; iNum++)
  {
    for( Int iIdx = 0; iIdx < AMVP_MAX_NUM_CANDS; iIdx++)
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
    m_pcQTTempTComYuv[ui].create( g_uiMaxCUWidth, g_uiMaxCUHeight );
  }
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
#ifdef WEIGHT_PRED
  setDistParamComp(0);  // Y component
#endif

  // distortion
  uiSad = m_cDistParam.DistFunc( &m_cDistParam );

  // motion cost
  uiSad += m_pcRdCost->getCost( iSearchX, iSearchY );

  // regularization cost
  if( m_pcRdCost->useMultiviewReg() )
  {
    uiSad += m_pcRdCost->getMultiviewRegCost( iSearchX, iSearchY );
  }

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

#ifdef ROUNDING_CONTROL_BIPRED
UInt TEncSearch::xPatternRefinement_Bi    ( TComPattern* pcPatternKey, Pel* piRef, Int iRefStride, Int iIntStep, Int iFrac, TComMv& rcMvFrac, Pel* pcRef2, Bool bRound, Bool bInterview )
{
  UInt  uiDist;
  UInt  uiDistBest  = MAX_UINT;
  UInt  uiDirecBest = 0;

  Pel*  piRefPos;

  m_pcRdCost->setDistParam_Bi( pcPatternKey, piRef, iRefStride, iIntStep, m_cDistParam, m_pcEncCfg->getUseHADME() );

  TComMv* pcMvRefine = (iFrac == 2 ? s_acMvRefineH : s_acMvRefineQ);

  for (UInt i = 0; i < 9; i++)
  {
    TComMv cMvTest = pcMvRefine[i];
    cMvTest += rcMvFrac;
    piRefPos = piRef + (pcMvRefine[i].getHor() + iRefStride * pcMvRefine[i].getVer()) * iFrac;
    m_cDistParam.pCur = piRefPos;
    uiDist = m_cDistParam.DistFuncRnd( &m_cDistParam, pcRef2, bRound );
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
#endif

//<--

UInt TEncSearch::xPatternRefinement( TComPattern* pcPatternKey, Pel* piRef, Int iRefStride, Int iIntStep, Int iFrac, TComMv& rcMvFrac )
{
  UInt  uiDist;
  UInt  uiDistBest  = MAX_UINT;
  UInt  uiDirecBest = 0;

  Pel*  piRefPos;
  m_pcRdCost->setDistParam( pcPatternKey, piRef, iRefStride, iIntStep, m_cDistParam, m_pcEncCfg->getUseHADME() );

  TComMv* pcMvRefine = (iFrac == 2 ? s_acMvRefineH : s_acMvRefineQ);

  for (UInt i = 0; i < 9; i++)
  {
    TComMv cMvTest = pcMvRefine[i];
    cMvTest += rcMvFrac;
    piRefPos = piRef + (pcMvRefine[i].getHor() + iRefStride * pcMvRefine[i].getVer()) * iFrac;
    m_cDistParam.pCur = piRefPos;
#ifdef WEIGHT_PRED
    setDistParamComp(0);  // Y component
#endif
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

#if CAVLC_RQT_CBP
  if(pcCU->getSlice()->getSymbolMode() == 0)
  {
    if(bLuma && bChroma)
    {
      m_pcEntropyCoder->m_pcEntropyCoderIf->codeCbfTrdiv( pcCU, uiAbsPartIdx, uiFullDepth);
    }
    else
    {
      UInt uiFlagPattern = m_pcEntropyCoder->m_pcEntropyCoderIf->xGetFlagPattern(pcCU, uiAbsPartIdx, uiFullDepth);
      if(bLuma)
      {
        if(uiTrDepth==0 || pcCU->getCbf(uiAbsPartIdx, TEXT_LUMA, uiTrDepth-1))
          m_pcEntropyCoder->encodeQtCbf( pcCU, uiAbsPartIdx, TEXT_LUMA, uiTrDepth);
        if(uiFlagPattern & 0x01)
          m_pcEntropyCoder->encodeTransformSubdivFlag( uiSubdiv, uiFullDepth );
      }
      else if(bChroma)
      {
        if( uiLog2TrafoSize > pcCU->getSlice()->getSPS()->getQuadtreeTULog2MinSize() )
        {
          if(uiTrDepth==0 || pcCU->getCbf(uiAbsPartIdx, TEXT_CHROMA_U, uiTrDepth-1))
            m_pcEntropyCoder->encodeQtCbf( pcCU, uiAbsPartIdx, TEXT_CHROMA_U, uiTrDepth);
          if(uiTrDepth==0 || pcCU->getCbf(uiAbsPartIdx, TEXT_CHROMA_V, uiTrDepth-1))
            m_pcEntropyCoder->encodeQtCbf( pcCU, uiAbsPartIdx, TEXT_CHROMA_V, uiTrDepth);
        }
        if(uiFlagPattern & 0x01)
          m_pcEntropyCoder->encodeTransformSubdivFlag( uiSubdiv, uiFullDepth );
      }
    }
  }
#endif

#if CAVLC_RQT_CBP
  if(pcCU->getSlice()->getSymbolMode())
  {
#endif
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
#if CAVLC_RQT_CBP
  }
#endif

  if( uiSubdiv )
  {
    UInt uiQPartNum = pcCU->getPic()->getNumPartInCU() >> ( ( uiFullDepth + 1 ) << 1 );
    for( UInt uiPart = 0; uiPart < 4; uiPart++ )
    {
      xEncSubdivCbfQT( pcCU, uiTrDepth + 1, uiAbsPartIdx + uiPart * uiQPartNum, bLuma, bChroma );
    }
    return;
  }

  if(pcCU->getSlice()->getSymbolMode())
  {
    //===== Cbfs =====
    if( bLuma )
    {
      m_pcEntropyCoder->encodeQtCbf( pcCU, uiAbsPartIdx, TEXT_LUMA,     uiTrMode );
    }
    if( bChroma )
    {
      Bool bCodeChroma = true;
      if( uiLog2TrafoSize == pcCU->getSlice()->getSPS()->getQuadtreeTULog2MinSize() )
      {
        assert( uiTrDepth > 0 );
        UInt uiQPDiv = pcCU->getPic()->getNumPartInCU() >> ( ( pcCU->getDepth( 0 ) + uiTrDepth - 1 ) << 1 );
        bCodeChroma  = ( ( uiAbsPartIdx % uiQPDiv ) == 0 );
      }
      if( bCodeChroma )
      {
        m_pcEntropyCoder->encodeQtCbf( pcCU, uiAbsPartIdx, TEXT_CHROMA_U, uiTrDepth );
        m_pcEntropyCoder->encodeQtCbf( pcCU, uiAbsPartIdx, TEXT_CHROMA_V, uiTrDepth );
      }
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
#if !CAVLC_RQT_CBP
    if ( pcCU->getSlice()->getSymbolMode() || pcCU->getCbf( uiAbsPartIdx, eTextType, uiTrDepth ))
    {
      if(pcCU->getSlice()->getSymbolMode() == 0)
      {
        if( eTextType == TEXT_LUMA || uiLog2TrafoSize-1 > pcCU->getSlice()->getSPS()->getQuadtreeTULog2MinSize() )
          m_pcEntropyCoder->m_pcEntropyCoderIf->codeBlockCbf(pcCU, uiAbsPartIdx, eTextType, uiTrDepth + 1, uiQPartNum, true);
      }
#endif
      for( UInt uiPart = 0; uiPart < 4; uiPart++ )
      {
        xEncCoeffQT( pcCU, uiTrDepth + 1, uiAbsPartIdx + uiPart * uiQPartNum, eTextType, bRealCoeff );
      }
#if !CAVLC_RQT_CBP
    }
#endif
    return;
  }

  if( eTextType != TEXT_LUMA && uiLog2TrafoSize == pcCU->getSlice()->getSPS()->getQuadtreeTULog2MinSize() )
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

  m_pcEntropyCoder->encodeCoeffNxN( pcCU, pcCoeff, uiAbsPartIdx, uiWidth, uiHeight, uiFullDepth, eTextType, false );
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
                            Bool         bRealCoeff /* just for test */
                            )
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
                                Dist&       ruiDist
                                )
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

#if HHI_DMM_WEDGE_INTRA || HHI_DMM_PRED_TEX
  if( uiLumaPredMode > MAX_MODE_ID_INTRA_DIR )
  {
    predIntraLumaDMM( pcCU, uiAbsPartIdx, uiLumaPredMode, piPred, uiStride, uiWidth, uiHeight, bAboveAvail, bLeftAvail, true );
  }
  else
#endif
  {
    //===== get prediction signal =====
    predIntraLumaAng( pcCU->getPattern(), uiLumaPredMode, piPred, uiStride, uiWidth, uiHeight, pcCU, bAboveAvail, bLeftAvail );
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
  //--- init rate estimation arrays for RDOQ ---
  if( m_pcEncCfg->getUseRDOQ() )
  {
    m_pcEntropyCoder->estimateBit( m_pcTrQuant->m_pcEstBitsSbac, uiWidth, TEXT_LUMA );
  }
  //--- transform and quantization ---
  UInt uiAbsSum = 0;
  pcCU       ->setTrIdxSubParts ( uiTrDepth, uiAbsPartIdx, uiFullDepth );
#if POZNAN_TEXTURE_TU_DELTA_QP_ACCORDING_TO_DEPTH
  m_pcTrQuant->setQPforQuant    ( pcCU->getQP( 0 ) + pcCU->getQpOffsetForTextCU(uiAbsPartIdx, true), !pcCU->getSlice()->getDepth(), pcCU->getSlice()->getSliceType(), TEXT_LUMA );
#else
  m_pcTrQuant->setQPforQuant    ( pcCU->getQP( 0 ), !pcCU->getSlice()->getDepth(), pcCU->getSlice()->getSliceType(), TEXT_LUMA );
#endif
  m_pcTrQuant->transformNxN     ( pcCU, piResi, uiStride, pcCoeff, uiWidth, uiHeight, uiAbsSum, TEXT_LUMA, uiAbsPartIdx );

  //--- set coded block flag ---
  pcCU->setCbfSubParts          ( ( uiAbsSum ? 1 : 0 ) << uiTrDepth, TEXT_LUMA, uiAbsPartIdx, uiFullDepth );
  //--- inverse transform ---
  if( uiAbsSum )
  {
#if INTRA_DST_TYPE_7
    m_pcTrQuant->invtransformNxN( TEXT_LUMA,pcCU->getLumaIntraDir( uiAbsPartIdx ), piResi, uiStride, pcCoeff, uiWidth, uiHeight );
#else
    m_pcTrQuant->invtransformNxN( piResi, uiStride, pcCoeff, uiWidth, uiHeight );
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
  if( uiLog2TrSize == pcCU->getSlice()->getSPS()->getQuadtreeTULog2MinSize() )
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
  Pel*      piRecQt           = ( uiChromaId > 0 ? m_pcQTTempTComYuv[ uiQTLayer ].getCrAddr( uiAbsPartIdx ) : m_pcQTTempTComYuv[ uiQTLayer ].getCbAddr( uiAbsPartIdx ) );
  UInt      uiRecQtStride     = m_pcQTTempTComYuv[ uiQTLayer ].getCStride();

  UInt      uiZOrder          = pcCU->getZorderIdxInCU() + uiAbsPartIdx;
  Pel*      piRecIPred        = ( uiChromaId > 0 ? pcCU->getPic()->getPicYuvRec()->getCrAddr( pcCU->getAddr(), uiZOrder ) : pcCU->getPic()->getPicYuvRec()->getCbAddr( pcCU->getAddr(), uiZOrder ) );
  UInt      uiRecIPredStride  = pcCU->getPic()->getPicYuvRec()->getCStride();

  //===== update chroma mode =====
#if !LM_CHROMA
  if( uiChromaPredMode == 4 )
  {
    uiChromaPredMode          = pcCU->getLumaIntraDir( 0 );
  }
#endif

  //===== init availability pattern =====
  Bool  bAboveAvail = false;
  Bool  bLeftAvail  = false;
  pcCU->getPattern()->initPattern         ( pcCU, uiTrDepth, uiAbsPartIdx );
  pcCU->getPattern()->initAdiPatternChroma( pcCU, uiAbsPartIdx, uiTrDepth, m_piYuvExt, m_iYuvExtStride, m_iYuvExtHeight, bAboveAvail, bLeftAvail );
  Int*  pPatChroma  = ( uiChromaId > 0 ? pcCU->getPattern()->getAdiCrBuf( uiWidth, uiHeight, m_piYuvExt ) : pcCU->getPattern()->getAdiCbBuf( uiWidth, uiHeight, m_piYuvExt ) );

  //===== get prediction signal =====
#if LM_CHROMA
  if(pcCU->getSlice()->getSPS()->getUseLMChroma() && uiChromaPredMode == 3)
  {
    predLMIntraChroma( pcCU->getPattern(), pPatChroma, piPred, uiStride, uiWidth, uiHeight, uiChromaId );
  }
  else
  {
    if( uiChromaPredMode == 4 )
    {
      uiChromaPredMode          = pcCU->getLumaIntraDir( 0 );
    }
  predIntraChromaAng( pcCU->getPattern(), pPatChroma, uiChromaPredMode, piPred, uiStride, uiWidth, uiHeight, pcCU, bAboveAvail, bLeftAvail );
  }
#else // LM_CHROMA
  predIntraChromaAng( pcCU->getPattern(), pPatChroma, uiChromaPredMode, piPred, uiStride, uiWidth, uiHeight, pcCU, bAboveAvail, bLeftAvail );
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
  {
    //--- init rate estimation arrays for RDOQ ---
    if( m_pcEncCfg->getUseRDOQ() )
    {
      m_pcEntropyCoder->estimateBit( m_pcTrQuant->m_pcEstBitsSbac, uiWidth, eText );
    }
    //--- transform and quantization ---
    UInt uiAbsSum = 0;
#if POZNAN_TEXTURE_TU_DELTA_QP_ACCORDING_TO_DEPTH
  m_pcTrQuant->setQPforQuant       ( pcCU->getQP( 0 ) + pcCU->getQpOffsetForTextCU(uiAbsPartIdx, true), !pcCU->getSlice()->getDepth(), pcCU->getSlice()->getSliceType(), TEXT_CHROMA );
#else
    m_pcTrQuant->setQPforQuant     ( pcCU->getQP( 0 ), !pcCU->getSlice()->getDepth(), pcCU->getSlice()->getSliceType(), TEXT_CHROMA );
#endif
    m_pcTrQuant->transformNxN      ( pcCU, piResi, uiStride, pcCoeff, uiWidth, uiHeight, uiAbsSum, eText, uiAbsPartIdx );
    //--- set coded block flag ---
    pcCU->setCbfSubParts           ( ( uiAbsSum ? 1 : 0 ) << uiOrgTrDepth, eText, uiAbsPartIdx, pcCU->getDepth(0) + uiTrDepth );
    //--- inverse transform ---
    if( uiAbsSum )
    {
#if INTRA_DST_TYPE_7
    m_pcTrQuant->invtransformNxN( TEXT_CHROMA, REG_DCT, piResi, uiStride, pcCoeff, uiWidth, uiHeight );
#else
    m_pcTrQuant->invtransformNxN( piResi, uiStride, pcCoeff, uiWidth, uiHeight );
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
  ruiDist += m_pcRdCost->getDistPart( piReco, uiStride, piOrg, uiStride, uiWidth, uiHeight );
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
  if( pcCU->getLumaIntraDir( uiAbsPartIdx ) > MAX_MODE_ID_INTRA_DIR )
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
    {
      dSingleCost = m_pcRdCost->calcRdCost( uiSingleBits, uiSingleDistY + uiSingleDistC );
    }
#else
    dSingleCost = m_pcRdCost->calcRdCost( uiSingleBits, uiSingleDistY + uiSingleDistC );
#endif
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
    {
      dSplitCost       = m_pcRdCost->calcRdCost( uiSplitBits, uiSplitDistY + uiSplitDistC );
    }
#else
    dSplitCost       = m_pcRdCost->calcRdCost( uiSplitBits, uiSplitDistY + uiSplitDistC );
#endif

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
  }

  if( bCheckSplit )
  {
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
    if( !bLumaOnly && uiLog2TrSize == pcCU->getSlice()->getSPS()->getQuadtreeTULog2MinSize() )
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
    if( uiLog2TrSize == pcCU->getSlice()->getSPS()->getQuadtreeTULog2MinSize() )
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
    UInt  uiSAD  = m_pcRdCost->calcHAD( piOrgU, uiStride, piPredU, uiStride, uiWidth, uiHeight );  //GT: change metric here?
    uiSAD       += m_pcRdCost->calcHAD( piOrgV, uiStride, piPredV, uiStride, uiWidth, uiHeight );  //GT: change metric here?
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
  Dist    uiOverallDistY = 0;
  Dist    uiOverallDistC = 0;
  UInt    CandNum;
  UInt    CandModeList[ FAST_UDI_MAX_RDMODE_NUM ];
  Double  CandCostList[ FAST_UDI_MAX_RDMODE_NUM ];
  UInt    uiFastCandNum=g_aucIntraModeNumFast[ uiWidthBit ];

  //===== set QP and clear Cbf =====
  pcCU->setQPSubParts( pcCU->getSlice()->getSliceQp(), 0, uiDepth );

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
    UInt uiMaxMode     = g_aucIntraModeNumAng[uiWidthBit];
#if ADD_PLANAR_MODE
    uiMaxMode += 1;
#endif
#if HHI_DMM_WEDGE_INTRA || HHI_DMM_PRED_TEX
    Bool bTestDmm = false;
    if ( m_pcEncCfg->isDepthCoder() && m_pcEncCfg->getUseDMM() )
      bTestDmm = true;
#endif
    UInt uiMaxModeFast = g_aucIntraModeNumFast[ uiWidthBit ];
    Pel* piOrg         = pcOrgYuv ->getLumaAddr( uiPU, uiWidth );
    Pel* piPred        = pcPredYuv->getLumaAddr( uiPU, uiWidth );
    UInt uiStride      = pcPredYuv->getStride();

    if ( uiFastCandNum != uiMaxMode ) uiMaxModeFast = 0;
    for( Int i=0; i < uiFastCandNum; i++ )
    {
      CandCostList[ i ] = MAX_DOUBLE;
    }
    CandNum = 0;

#if ADD_PLANAR_MODE
    UInt uiHdModeList[NUM_INTRA_MODE];
    uiHdModeList[0] = PLANAR_IDX;
    for( Int i=1; i < uiMaxMode; i++) uiHdModeList[i] = i-1;

    for( Int iMode = Int(uiMaxModeFast); iMode < Int(uiMaxMode); iMode++ )
    {
      UInt uiMode = uiHdModeList[iMode];
#if (!REFERENCE_SAMPLE_PADDING)
      if ( !predIntraLumaDirAvailable( uiMode, uiWidthBit, bAboveAvail, bLeftAvail ) )
        continue;
#endif
      predIntraLumaAng( pcCU->getPattern(), uiMode, piPred, uiStride, uiWidth, uiHeight, pcCU, bAboveAvail, bLeftAvail );
#else
    for( UInt uiMode = uiMaxModeFast; uiMode < uiMaxMode; uiMode++ )
    {
#if (!REFERENCE_SAMPLE_PADDING)
      if ( !predIntraLumaDirAvailable( uiMode, uiWidthBit, bAboveAvail, bLeftAvail ) )
        continue;
#endif

      predIntraLumaAng( pcCU->getPattern(), uiMode, piPred, uiStride, uiWidth, uiHeight, pcCU, bAboveAvail, bLeftAvail );
#endif
      // use hadamard transform here
      Dist uiSad;
#if HHI_VSO
      if ( m_pcRdCost->getUseVSO() )
      {
        Bool bSad = !m_pcRdCost->getUseRenModel();
        uiSad = m_pcRdCost->getDistVS(pcCU, uiPartOffset, piPred, uiStride, piOrg, uiStride, uiWidth, uiHeight, bSad, 0 );
      }
      else
      {
        uiSad = (Dist) m_pcRdCost->calcHAD( piOrg, uiStride, piPred, uiStride, uiWidth, uiHeight );
      }
#else
        uiSad = (Dist) m_pcRdCost->calcHAD( piOrg, uiStride, piPred, uiStride, uiWidth, uiHeight );
#endif

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

      CandNum += xUpdateCandList( uiMode, cost, uiFastCandNum, CandModeList, CandCostList );
#if HHI_DMM_WEDGE_INTRA || HHI_DMM_PRED_TEX
      if ( bTestDmm ) bTestDmm = uiSad ? true : false;
#endif
    }
    UInt uiRdModeList[FAST_UDI_MAX_RDMODE_NUM];
    UInt uiNewMaxMode;
    UInt uiMinMode = 0;

    if(uiFastCandNum!=uiMaxMode)
    {
      uiNewMaxMode = Min( uiFastCandNum, CandNum );
      for( Int i = 0; i < uiNewMaxMode; i++)
      {
        uiRdModeList[i] = CandModeList[i];
      }
#if FAST_UDI_USE_MPM
#if MTK_DCM_MPM
      Int uiPreds[2] = {-1, -1};
      Int numCand = pcCU->getIntraDirLumaPredictor(uiPartOffset, uiPreds);

      for( Int j=0; j < numCand; j++)
      {
        Bool mostProbableModeIncluded = false;
        Int mostProbableMode = uiPreds[j];
#if ADD_PLANAR_MODE
      if (mostProbableMode == 2)
      {
        mostProbableMode = PLANAR_IDX;
      }
#endif
        for( Int i=0; i < uiNewMaxMode; i++)
        {
          mostProbableModeIncluded |= (mostProbableMode == uiRdModeList[i]);
        }
        if (!mostProbableModeIncluded)
        {
          uiRdModeList[uiNewMaxMode++] = mostProbableMode;
        }
      }
#else
      Int mostProbableMode = pcCU->getMostProbableIntraDirLuma( uiPartOffset );
#if ADD_PLANAR_MODE
      if (mostProbableMode == 2)
      {
        mostProbableMode = PLANAR_IDX;
      }
#endif
      Bool mostProbableModeIncluded = false;
      for( Int i=0; i < uiNewMaxMode; i++)
      {
        mostProbableModeIncluded |= (mostProbableMode == uiRdModeList[i]);
      }
      if (!mostProbableModeIncluded)
      {
        uiRdModeList[uiNewMaxMode++] = mostProbableMode;
      }
#endif
#endif
    }
    else
    {
      uiNewMaxMode = uiMaxMode;
#if ADD_PLANAR_MODE
      uiRdModeList[0] = PLANAR_IDX;
      for( Int i=1; i < uiNewMaxMode; i++) uiRdModeList[i] = i-1;
#else
      for( Int i=0; i < uiNewMaxMode; i++) uiRdModeList[i] = i;
#endif
    }

#if HHI_DMM_WEDGE_INTRA || HHI_DMM_PRED_TEX
    if( m_pcEncCfg->isDepthCoder() && uiWidth >= 4 && uiWidth < 64 && m_pcEncCfg->getUseDMM() && bTestDmm && uiWidth == uiHeight )
    {
#if HHI_DMM_WEDGE_INTRA
      UInt uiTabIdx  = 0;
      Int  iDeltaDC1 = 0;
      Int  iDeltaDC2 = 0;
      findWedgeFullMinDist( pcCU, uiPartOffset, piOrg, piPred, uiStride, uiWidth, uiHeight, uiTabIdx, iDeltaDC1, iDeltaDC2, bAboveAvail, bLeftAvail, WedgeDist_SAD );
      pcCU->setWedgeFullTabIdxSubParts  ( uiTabIdx,  uiPartOffset, uiDepth + uiInitTrDepth );
      pcCU->setWedgeFullDeltaDC1SubParts( iDeltaDC1, uiPartOffset, uiDepth + uiInitTrDepth );
      pcCU->setWedgeFullDeltaDC2SubParts( iDeltaDC2, uiPartOffset, uiDepth + uiInitTrDepth );

      uiRdModeList[ uiNewMaxMode++ ] = DMM_WEDGE_FULL_IDX;
      uiRdModeList[ uiNewMaxMode++ ] = DMM_WEDGE_FULL_D_IDX;

      if ( uiWidth > 4 )
      {
        Int  iWedgeDeltaEnd = 0;

        iDeltaDC1 = 0;
        iDeltaDC2 = 0;

        findWedgePredDirMinDist( pcCU, uiPartOffset, piOrg, piPred, uiStride, uiWidth, uiHeight, uiTabIdx, iWedgeDeltaEnd, iDeltaDC1, iDeltaDC2, bAboveAvail, bLeftAvail, WedgeDist_SAD );
        pcCU->setWedgePredDirTabIdxSubParts  ( uiTabIdx,       uiPartOffset, uiDepth + uiInitTrDepth );
        pcCU->setWedgePredDirDeltaEndSubParts( iWedgeDeltaEnd, uiPartOffset, uiDepth + uiInitTrDepth );
        pcCU->setWedgePredDirDeltaDC1SubParts( iDeltaDC1,      uiPartOffset, uiDepth + uiInitTrDepth );
        pcCU->setWedgePredDirDeltaDC2SubParts( iDeltaDC2,      uiPartOffset, uiDepth + uiInitTrDepth );

        uiRdModeList[ uiNewMaxMode++ ] = DMM_WEDGE_PREDDIR_IDX;
        uiRdModeList[ uiNewMaxMode++ ] = DMM_WEDGE_PREDDIR_D_IDX;
      }
#endif
#if HHI_DMM_PRED_TEX
#if FLEX_CODING_ORDER
      if ( pcCU->getSlice()->getSPS()->getUseDMM34() )
      {
#endif
      TComYuv cTempYuv; cTempYuv.create( uiWidth, uiHeight ); cTempYuv.clear();
      Pel* piTempY      = cTempYuv.getLumaAddr();

      fillTexturePicTempBlock( pcCU, uiPartOffset, piTempY, uiWidth, uiHeight );

      piTempY = cTempYuv.getLumaAddr();

      UInt uiTexTabIdx  = 0;
      Int  iTexDeltaDC1 = 0;
      Int  iTexDeltaDC2 = 0;
      findWedgeTexMinDist( pcCU, uiPartOffset, piOrg, piPred, uiStride, uiWidth, uiHeight, uiTexTabIdx, iTexDeltaDC1, iTexDeltaDC2, bAboveAvail, bLeftAvail, WedgeDist_SAD, piTempY ); 
      pcCU->setWedgePredTexTabIdxSubParts  ( uiTexTabIdx,  uiPartOffset, uiDepth + uiInitTrDepth );
      pcCU->setWedgePredTexDeltaDC1SubParts( iTexDeltaDC1, uiPartOffset, uiDepth + uiInitTrDepth );
      pcCU->setWedgePredTexDeltaDC2SubParts( iTexDeltaDC2, uiPartOffset, uiDepth + uiInitTrDepth );

      uiRdModeList[ uiNewMaxMode++ ] = DMM_WEDGE_PREDTEX_IDX;
      uiRdModeList[ uiNewMaxMode++ ] = DMM_WEDGE_PREDTEX_D_IDX;

      if ( uiWidth > 4 )
      {
        piTempY = cTempYuv.getLumaAddr();

        iTexDeltaDC1 = 0;
        iTexDeltaDC2 = 0;

        findContourPredTex( pcCU, uiPartOffset, piOrg, piPred, uiStride, uiWidth, uiHeight, iTexDeltaDC1, iTexDeltaDC2, bAboveAvail, bLeftAvail, piTempY );
        pcCU->setContourPredTexDeltaDC1SubParts( iTexDeltaDC1, uiPartOffset, uiDepth + uiInitTrDepth );
        pcCU->setContourPredTexDeltaDC2SubParts( iTexDeltaDC2, uiPartOffset, uiDepth + uiInitTrDepth );

        uiRdModeList[ uiNewMaxMode++ ] = DMM_CONTOUR_PREDTEX_IDX;
        uiRdModeList[ uiNewMaxMode++ ] = DMM_CONTOUR_PREDTEX_D_IDX;
      }

      cTempYuv.destroy();
#if FLEX_CODING_ORDER
      }
#endif
#endif
    }
#endif
    //===== check modes (using r-d costs) =====
#if HHI_RQT_INTRA_SPEEDUP_MOD
    UInt   uiSecondBestMode  = MAX_UINT;
    Double dSecondBestPUCost = MAX_DOUBLE;
#endif

    UInt    uiBestPUMode  = 0;
    Dist    uiBestPUDistY = 0;
    Dist    uiBestPUDistC = 0;
    Double  dBestPUCost   = MAX_DOUBLE;
    for( UInt uiMode = uiMinMode; uiMode < uiNewMaxMode; uiMode++ )
    {
      // set luma prediction mode
      UInt uiOrgMode = uiRdModeList[uiMode];

#if (!REFERENCE_SAMPLE_PADDING)
#if HHI_DMM_WEDGE_INTRA || HHI_DMM_PRED_TEX
      if ( !predIntraLumaDirAvailable( uiOrgMode, uiWidthBit, bAboveAvail, bLeftAvail, uiWidth, uiHeight, pcCU, uiPartOffset ) )
        continue;
#else
      if ( !predIntraLumaDirAvailable( uiOrgMode, uiWidthBit, bAboveAvail, bLeftAvail ) )
        continue;
#endif
#else
#if HHI_DMM_WEDGE_INTRA || HHI_DMM_PRED_TEX
#if HHI_DMM_PRED_TEX && FLEX_CODING_ORDER
      if( m_pcEncCfg->isDepthCoder() && !predIntraLumaDMMAvailable( uiOrgMode, uiWidth, uiHeight, pcCU->getSlice()->getSPS()->getUseDMM34() ) )
#else
      if( m_pcEncCfg->isDepthCoder() && !predIntraLumaDMMAvailable( uiOrgMode, uiWidth, uiHeight ) )
#endif
        continue;
#endif
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

#if (!REFERENCE_SAMPLE_PADDING)
#if HHI_DMM_WEDGE_INTRA || HHI_DMM_PRED_TEX
      if ( !predIntraLumaDirAvailable( uiOrgMode, uiWidthBit, bAboveAvail, bLeftAvail, uiWidth, uiHeight, pcCU, uiPartOffset ) )
        continue;
#else
      if ( !predIntraLumaDirAvailable( uiOrgMode, uiWidthBit, bAboveAvail, bLeftAvail ) )
        continue;
#endif
#else
#if HHI_DMM_WEDGE_INTRA || HHI_DMM_PRED_TEX
#if HHI_DMM_PRED_TEX && FLEX_CODING_ORDER
      if( m_pcEncCfg->isDepthCoder() && !predIntraLumaDMMAvailable( uiOrgMode, uiWidth, uiHeight, pcCU->getSlice()->getSPS()->getUseDMM34() ) )
#else
      if( m_pcEncCfg->isDepthCoder() && !predIntraLumaDMMAvailable( uiOrgMode, uiWidth, uiHeight ) )
#endif
        continue;
#endif
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
      if( !bLumaOnly && uiLog2TrSize == pcCU->getSlice()->getSPS()->getQuadtreeTULog2MinSize() )
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
#if ADD_PLANAR_MODE
  UInt  uiModeList[6];
  uiModeList[0] = PLANAR_IDX;
  for( Int i = 0; i < 5; i++ )
  {
    uiModeList[i+1] = i;
  }
  UInt uiLumaMode = pcCU->getLumaIntraDir(0);
#else
  UInt  uiModeList[5];
  for( Int i = 0; i < 4; i++ )
  {
    uiModeList[i] = i;
  }

  uiModeList[4]   = pcCU->getLumaIntraDir(0);
#endif

  UInt  uiMinMode = 0;
#if CHROMA_CODEWORD
#if ADD_PLANAR_MODE
  UInt  uiMaxMode = 6;

#if LM_CHROMA
  UInt  uiIgnore;
  if(pcCU->getSlice()->getSPS()->getUseLMChroma())
  {
    uiIgnore = ( ( (uiLumaMode != PLANAR_IDX) && (uiLumaMode >= 3) ) ? uiMaxMode : uiLumaMode );
  }
  else
  {
    uiIgnore = ( ( (uiLumaMode != PLANAR_IDX) && (uiLumaMode >= 4) ) ? uiMaxMode : uiLumaMode );
  }
#else
  UInt  uiIgnore = ( ( (uiLumaMode != PLANAR_IDX) && (uiLumaMode >= 4) ) ? uiMaxMode : uiLumaMode );
#endif

#else
  UInt  uiMaxMode = 5;

#if LM_CHROMA
  UInt  uiIgnore;
  if(pcCU->getSlice()->getSPS()->getUseLMChroma())
  {
    uiIgnore = (uiModeList[4] >= 0 && uiModeList[4] < 3) ? uiModeList[4] : 6;
  }
  else
  {
    uiIgnore = (uiModeList[4] >= 0 && uiModeList[4] < 4) ? uiModeList[4] : 6;
  }
#else
  UInt  uiIgnore = (uiModeList[4] < 4) ? uiModeList[4] : 6;
#endif

#endif
#else
#if ADD_PLANAR_MODE
  UInt  uiMaxMode = ( ( (uiLumaMode != PLANAR_IDX) && (uiLumaMode >= 4) ) ? 6 : 5 );
#else
  UInt  uiMaxMode = ( uiModeList[4] >= 4 ? 5 : 4 );
#endif
#endif

  //----- check chroma modes -----
  for( UInt uiMode = uiMinMode; uiMode < uiMaxMode; uiMode++ )
  {
#if CHROMA_CODEWORD
#if ADD_PLANAR_MODE
    if ( uiModeList[uiMode] == uiIgnore )
#else
    if (uiMode == uiIgnore)
#endif
    {
      continue;
    }
#endif
#if HHI_DMM_WEDGE_INTRA || HHI_DMM_PRED_TEX
#if ADD_PLANAR_MODE
    if ( uiModeList[uiMode] == 4 && pcCU->getLumaIntraDir(0) > MAX_MODE_ID_INTRA_DIR )
    {
      continue;
    }
#else
    if ( uiMode == 4 && pcCU->getLumaIntraDir(0) > MAX_MODE_ID_INTRA_DIR )
    {
      continue;
    }
#endif
#endif
    //----- restore context models -----
    if( m_bUseSBACRD )
    {
      m_pcRDGoOnSbacCoder->load( m_pppcRDSbacCoder[uiDepth][CI_CURR_BEST] );
    }

    //----- chroma coding -----
    Dist    uiDist = 0;
#if ADD_PLANAR_MODE
    pcCU->setChromIntraDirSubParts  ( uiModeList[uiMode], 0, uiDepth );
#else
    pcCU->setChromIntraDirSubParts  ( uiMode, 0, uiDepth );
#endif
    xRecurIntraChromaCodingQT       ( pcCU,   0, 0, pcOrgYuv, pcPredYuv, pcResiYuv, uiDist );
    UInt    uiBits = xGetIntraBitsQT( pcCU,   0, 0, false, true, false );
    Double  dCost  = m_pcRdCost->calcRdCost( uiBits, uiDist );

    //----- compare -----
    if( dCost < dBestCost )
    {
      dBestCost   = dCost;
      uiBestDist  = uiDist;
#if ADD_PLANAR_MODE
      uiBestMode  = uiModeList[uiMode];
#else
      uiBestMode  = uiMode;
#endif
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

#if (!REFERENCE_SAMPLE_PADDING)
#if HHI_DMM_WEDGE_INTRA || HHI_DMM_PRED_TEX
Bool TEncSearch::predIntraLumaDirAvailable( UInt uiMode, UInt uiWidthBit, Bool bAboveAvail, Bool bLeftAvail, UInt uiWidth, UInt uiHeight, TComDataCU* pcCU, UInt uiAbsPartIdx   )
#else
Bool TEncSearch::predIntraLumaDirAvailable( UInt uiMode, UInt uiWidthBit, Bool bAboveAvail, Bool bLeftAvail )
#endif
{
  Bool bDirAvailable;
#if HHI_DMM_WEDGE_INTRA || HHI_DMM_PRED_TEX
  if( uiMode > MAX_MODE_ID_INTRA_DIR )
  {
    return predIntraLumaDMMAvailable( uiMode, bAboveAvail, bLeftAvail, uiWidth, uiHeight );
  }
  else
#endif
  {
    bDirAvailable = true;
    UInt uiNewMode = g_aucAngIntraModeOrder[uiMode];
    if ( uiNewMode > 0 && ( ( (!bAboveAvail) && uiNewMode < 18 ) || ( (!bLeftAvail) && uiNewMode > 17 ) ) )
    {
      bDirAvailable = false;
    }
  }

  return bDirAvailable;
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
                                      Bool         bAbove,
                                      Bool         bLeft,
                                      WedgeDist    eWedgeDist
                                      )
{
  assert( uiWidth >= DMM_WEDGEMODEL_MIN_SIZE && uiWidth <= DMM_WEDGEMODEL_MAX_SIZE );

  WedgeList* pacWedgeList = &g_aacWedgeLists[(g_aucConvertToBit[uiWidth])];
  xSearchWedgeFullMinDist( pcCU, uiAbsPtIdx, pacWedgeList, piOrig, uiStride, uiWidth, uiHeight, ruiTabIdx );

  TComWedgelet* pcBestWedgelet = &(pacWedgeList->at(ruiTabIdx));
  xGetWedgeDeltaDCsMinDist( pcBestWedgelet, pcCU, uiAbsPtIdx, piOrig, piPredic, uiStride, uiWidth, uiHeight, riDeltaDC1, riDeltaDC2, bAbove, bLeft );
}

Void TEncSearch::xSearchWedgeFullMinDist( TComDataCU* pcCU, UInt uiAbsPtIdx, WedgeList* pacWedgeList, Pel* piRef, UInt uiRefStride, UInt uiWidth, UInt uiHeight, UInt& ruiTabIdx )
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
      uiActDist = m_pcRdCost->getDistPart( piPred, uiPredStride, piRef, uiRefStride, uiWidth, uiHeight, DF_SAD );
    }
#else
    uiActDist = m_pcRdCost->getDistPart( piPred, uiPredStride, piRef, uiRefStride, uiWidth, uiHeight, DF_SAD );
#endif

    if( uiActDist < uiBestDist || uiBestDist == RDO_DIST_MAX )
    {
      uiBestDist   = uiActDist;
      uiBestTabIdx = uiIdx;
    }
  }
  ruiTabIdx = uiBestTabIdx;

  cPredYuv.destroy();
  return;
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
                                         Bool         bAbove,
                                         Bool         bLeft,
                                         WedgeDist    eWedgeDist )
{
  assert( uiWidth >= DMM_WEDGEMODEL_MIN_SIZE && uiWidth <= DMM_WEDGEMODEL_MAX_SIZE );
  WedgeList* pacWedgeList = &g_aacWedgeLists[(g_aucConvertToBit[uiWidth])];

  ruiTabIdx       = 0;
  riWedgeDeltaEnd = 0;

  xSearchWedgePredDirMinDist( pcCU, uiAbsPtIdx, pacWedgeList, piOrig, uiStride, uiWidth, uiHeight, ruiTabIdx, riWedgeDeltaEnd );

  TComWedgelet* pcBestWedgelet = &(pacWedgeList->at(ruiTabIdx));
  xGetWedgeDeltaDCsMinDist( pcBestWedgelet, pcCU, uiAbsPtIdx, piOrig, piPredic, uiStride, uiWidth, uiHeight, riDeltaDC1, riDeltaDC2, bAbove, bLeft );
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
      uiActDist = m_pcRdCost->getDistPart( piPred, uiPredStride, piRef, uiRefStride, uiWidth, uiHeight, DF_SAD );
    }
#else
    uiActDist = m_pcRdCost->getDistPart( piPred, uiPredStride, piRef, uiRefStride, uiWidth, uiHeight, DF_SAD );
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
                                      Bool         bAbove, 
                                      Bool         bLeft, 
                                      WedgeDist    eWedgeDist,
                                      Pel*         piTextureRef
                                    )
  {
  assert( uiWidth >= DMM_WEDGEMODEL_MIN_SIZE && uiWidth <= DMM_WEDGEMODEL_MAX_SIZE );
  WedgeList* pacWedgeList = &g_aacWedgeLists[(g_aucConvertToBit[uiWidth])];

  ruiTabIdx = getBestWedgeFromText( pcCU, uiAbsPtIdx, uiWidth, uiHeight, eWedgeDist, piTextureRef );

  TComWedgelet* pcBestWedgelet = &(pacWedgeList->at(ruiTabIdx));
  xGetWedgeDeltaDCsMinDist( pcBestWedgelet, pcCU, uiAbsPtIdx, piOrig, piPredic, uiStride, uiWidth, uiHeight, riDeltaDC1, riDeltaDC2, bAbove, bLeft );
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
                                     Bool         bAbove,
                                     Bool         bLeft,
                                     Pel*         piTextureRef )
{
  // get contour pattern
  TComWedgelet* pcContourWedge = new TComWedgelet( uiWidth, uiHeight );
  getBestContourFromText( pcCU, uiAbsPtIdx, uiWidth, uiHeight, pcContourWedge, piTextureRef );

  xGetWedgeDeltaDCsMinDist( pcContourWedge, pcCU, uiAbsPtIdx, piOrig, piPredic, uiStride, uiWidth, uiHeight, riDeltaDC1, riDeltaDC2, bAbove, bLeft );

  pcContourWedge->destroy();
  delete pcContourWedge;
}
#endif
#if HHI_DMM_WEDGE_INTRA || HHI_DMM_PRED_TEX
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
                                           Bool          bAbove,
                                           Bool          bLeft )
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
  getWedgePredDCs( pcWedgelet, piMask, iMaskStride, iPredDC1, iPredDC2, bAbove, bLeft );

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

#if HHI_DMM_PRED_TEX && FLEX_CODING_ORDER
Bool TEncSearch::predIntraLumaDMMAvailable( UInt uiMode, UInt uiWidth, UInt uiHeight, Bool bDMMAvailable34 )
#else
Bool TEncSearch::predIntraLumaDMMAvailable( UInt uiMode, UInt uiWidth, UInt uiHeight )
#endif
{
  if( uiMode <= MAX_MODE_ID_INTRA_DIR ) return true;

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
#if FLEX_CODING_ORDER
    if ( !bDMMAvailable34 )
    {
      bDMMAvailable = false;
    }
#endif
  }

#endif

  return bDMMAvailable;
}

Void TEncSearch::xDeltaDCQuantScaleDown( TComDataCU*  pcCU, Int& riDeltaDC )
{
  Int  iSign  = riDeltaDC < 0 ? -1 : 1;
  UInt uiAbs  = abs( riDeltaDC );

  Int riB = 0;
  Int iQp = pcCU->getQP(0);
  Int iMax = ( 1<<( g_uiBitDepth + g_uiBitIncrement - 1) );
  Double dStepSize = Clip3( 1, iMax, pow( 2.0, iQp/10.0 + g_dDeltaDCsQuantOffset ) );

  riB = roftoi( uiAbs / dStepSize );

  riDeltaDC = riB * iSign;
  return;
}
#endif


Void TEncSearch::xGetInterPredictionError( TComDataCU* pcCU, TComYuv* pcYuvOrg, Int iPartIdx, UInt& ruiErr, Bool bHadamard )
{
  TComYuv cYuvPred;
  cYuvPred.create( pcYuvOrg->getWidth(), pcYuvOrg->getHeight() );

#ifdef WEIGHT_PRED
  UInt uiAbsPartIdx = 0;
  Int iWidth = 0;
  Int iHeight = 0;
  Int iRefIdx[2];
  pcCU->getPartIndexAndSize( iPartIdx, uiAbsPartIdx, iWidth, iHeight );

  iRefIdx[0] = pcCU->getCUMvField( REF_PIC_LIST_0 )->getRefIdx( uiAbsPartIdx );
  iRefIdx[1] = pcCU->getCUMvField( REF_PIC_LIST_1 )->getRefIdx( uiAbsPartIdx );
  if ( iRefIdx[0]>=0 && iRefIdx[1]<1 )
    setWpScalingDistParam( pcCU, iRefIdx[0], iRefIdx[1], REF_PIC_LIST_0);
  else
    setWpScalingDistParam( pcCU, iRefIdx[0], iRefIdx[1], REF_PIC_LIST_1);

  motionCompensation( pcCU, &cYuvPred, REF_PIC_LIST_X, iPartIdx );
#else
  motionCompensation( pcCU, &cYuvPred, REF_PIC_LIST_X, iPartIdx );

  UInt uiAbsPartIdx = 0;
  Int iWidth = 0;
  Int iHeight = 0;
  pcCU->getPartIndexAndSize( iPartIdx, uiAbsPartIdx, iWidth, iHeight );
#endif

  DistParam cDistParam;
#ifdef WEIGHT_PRED
  cDistParam.applyWeight = false;
#endif
  m_pcRdCost->setDistParam( cDistParam,
                            pcYuvOrg->getLumaAddr( uiAbsPartIdx ), pcYuvOrg->getStride(),
                            cYuvPred .getLumaAddr( uiAbsPartIdx ), cYuvPred .getStride(),
                            iWidth, iHeight, m_pcEncCfg->getUseHADME() );
  ruiErr = cDistParam.DistFunc( &cDistParam );

  cYuvPred.destroy();
}

#if POZNAN_DBMP
Void TEncSearch::xGetInterPredictionError_DBMP( TComDataCU* pcCU, TComYuv* pcYuvOrg, Int iPartIdx, UInt& ruiErr, Bool bHadamard )
{
  TComYuv cYuvPred;
  cYuvPred.create( pcYuvOrg->getWidth(), pcYuvOrg->getHeight() );

#ifdef WEIGHT_PRED
  UInt uiAbsPartIdx = 0;
  Int iWidth = 0;
  Int iHeight = 0;
  //Int iRefIdx[2];
  pcCU->getPartIndexAndSize( iPartIdx, uiAbsPartIdx, iWidth, iHeight );

//???????????????????????????????????????????????????????????????????????????????????
  //iRefIdx[0] = pcCU->getCUMvField( REF_PIC_LIST_0 )->getRefIdx( uiAbsPartIdx );
  //iRefIdx[1] = pcCU->getCUMvField( REF_PIC_LIST_1 )->getRefIdx( uiAbsPartIdx );
  //if ( iRefIdx[0]>=0 && iRefIdx[1]<1 )
  //  setWpScalingDistParam( pcCU, iRefIdx[0], iRefIdx[1], REF_PIC_LIST_0);
  //else
  //  setWpScalingDistParam( pcCU, iRefIdx[0], iRefIdx[1], REF_PIC_LIST_1);
  setWpScalingDistParam( pcCU, -1, -1, REF_PIC_LIST_X);//???
//???????????????????????????????????????????????????????????????????????????????????
  motionCompensation_DBMP( pcCU, &cYuvPred, REF_PIC_LIST_X, iPartIdx );
#else
  motionCompensation_DBMP( pcCU, &cYuvPred, REF_PIC_LIST_X, iPartIdx );

  UInt uiAbsPartIdx = 0;
  Int iWidth = 0;
  Int iHeight = 0;
  pcCU->getPartIndexAndSize( iPartIdx, uiAbsPartIdx, iWidth, iHeight );
#endif

  DistParam cDistParam;
#ifdef WEIGHT_PRED
  cDistParam.applyWeight = false;
#endif
  m_pcRdCost->setDistParam( cDistParam, 
                            pcYuvOrg->getLumaAddr( uiAbsPartIdx ), pcYuvOrg->getStride(), 
                            cYuvPred .getLumaAddr( uiAbsPartIdx ), cYuvPred .getStride(), 
                            iWidth, iHeight, m_pcEncCfg->getUseHADME() );
  ruiErr = cDistParam.DistFunc( &cDistParam );

  cYuvPred.destroy();
}
#endif

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
Void TEncSearch::xMergeEstimation( TComDataCU* pcCU, TComYuv* pcYuvOrg, Int iPUIdx, UInt& uiInterDir, TComMvField* pacMvField, UInt& uiMergeIndex, UInt& ruiCost, UInt& ruiBits, UChar* puhNeighCands,Bool& bValid )
{
  TComMvField  cMvFieldNeighbours[MRG_MAX_NUM_CANDS << 1]; // double length for mv of both lists
  UChar uhInterDirNeighbours[MRG_MAX_NUM_CANDS];
  UInt uiNeighbourCandIdx[MRG_MAX_NUM_CANDS]; //MVs with same idx => same cand

  for( UInt ui = 0; ui < MRG_MAX_NUM_CANDS; ++ui )
  {
    uhInterDirNeighbours[ui] = 0;
    uiNeighbourCandIdx[ui] = 0;
  }

  UInt uiAbsPartIdx = 0;
  Int iWidth = 0;
  Int iHeight = 0;

  pcCU->getPartIndexAndSize( iPUIdx, uiAbsPartIdx, iWidth, iHeight );
  UInt uiDepth = pcCU->getDepth( uiAbsPartIdx );
  pcCU->getInterMergeCandidates( uiAbsPartIdx, iPUIdx, uiDepth, cMvFieldNeighbours,uhInterDirNeighbours, uiNeighbourCandIdx );

  UInt uiNumCand = 0;
  for( UInt uiMergeCand = 0; uiMergeCand < MRG_MAX_NUM_CANDS; ++uiMergeCand )
  {
    if( uiNeighbourCandIdx[uiMergeCand] == ( uiMergeCand + 1 ) )
    {
      uiNumCand++;
    }
  }

  UInt uiBestSAD = MAX_UINT;
  UInt uiBestBitCost = MAX_UINT;
  UInt uiBestBits = MAX_UINT;

  ruiCost = MAX_UINT;
  ruiBits = MAX_UINT;

  bValid = false;

  for( UInt uiMergeCand = 0; uiMergeCand < MRG_MAX_NUM_CANDS; ++uiMergeCand )
  {
    if( uiNeighbourCandIdx[uiMergeCand] == ( uiMergeCand + 1 ) )
    {
      bValid = true;
      UInt uiCostCand = MAX_UINT;
      UInt uiBitsCand = 0;

      PartSize ePartSize = pcCU->getPartitionSize( 0 );

#if POZNAN_DBMP_CALC_PRED_DATA
	  if(uiMergeCand==POZNAN_DBMP_MRG_CAND)
	  {
		pcCU->getCUMvField2nd( REF_PIC_LIST_0 )->setAllMvField( cMvFieldNeighbours[0 + 2*uiMergeCand].getMv(), cMvFieldNeighbours[0 + 2*uiMergeCand].getRefIdx(), ePartSize, uiAbsPartIdx, iPUIdx, 0 );
		pcCU->getCUMvField2nd( REF_PIC_LIST_1 )->setAllMvField( cMvFieldNeighbours[1 + 2*uiMergeCand].getMv(), cMvFieldNeighbours[1 + 2*uiMergeCand].getRefIdx(), ePartSize, uiAbsPartIdx, iPUIdx, 0 );
	  }
	  else
#endif
	  {
      pcCU->getCUMvField(REF_PIC_LIST_0)->setAllMvField( cMvFieldNeighbours[0 + 2*uiMergeCand].getMv(), cMvFieldNeighbours[0 + 2*uiMergeCand].getRefIdx(), ePartSize, uiAbsPartIdx, iPUIdx, 0 );
      pcCU->getCUMvField(REF_PIC_LIST_1)->setAllMvField( cMvFieldNeighbours[1 + 2*uiMergeCand].getMv(), cMvFieldNeighbours[1 + 2*uiMergeCand].getRefIdx(), ePartSize, uiAbsPartIdx, iPUIdx, 0 );
	  }

#if POZNAN_DBMP
	  if(uiMergeCand==POZNAN_DBMP_MRG_CAND)
		xGetInterPredictionError_DBMP( pcCU, pcYuvOrg, iPUIdx, uiCostCand, m_pcEncCfg->getUseHADME() );
	  else
      xGetInterPredictionError( pcCU, pcYuvOrg, iPUIdx, uiCostCand, m_pcEncCfg->getUseHADME() );
#else
      xGetInterPredictionError( pcCU, pcYuvOrg, iPUIdx, uiCostCand, m_pcEncCfg->getUseHADME() );
#endif

      if( uiNumCand == 1 )
      {
        uiBitsCand = 1;
      }
      else
      {
        UInt uiMergeCandIdx = uiMergeCand;
#if POZNAN_DBMP
		if(pcCU->getSlice()->getMP()->isDBMPEnabled())
		{
			if(uiMergeCand == POZNAN_DBMP_MRG_CAND) uiMergeCandIdx = POZNAN_DBMP_MERGE_POS;
			else if(uiMergeCand >= POZNAN_DBMP_MERGE_POS) uiMergeCandIdx++;
		}
#endif

        if( uiMergeCandIdx == 0 || uiNumCand == 2 )
        {
          uiBitsCand = 2;
        }
        else if( uiMergeCandIdx == 1 || uiNumCand == 3 )
        {
          uiBitsCand = 3;
        }
        else if( uiMergeCandIdx == 2 || uiNumCand == 4 )
        {
          uiBitsCand = 4;
        }
		else if( uiMergeCandIdx == 3 || uiNumCand == 5 )
        {
          uiBitsCand = 5;
        }
        else
        {
          uiBitsCand = 6;
        }
      }

      if ( uiCostCand < ruiCost )
      {
        ruiCost = uiCostCand;
        ruiBits = uiBitsCand;
        pacMvField[0] = cMvFieldNeighbours[0 + 2*uiMergeCand];
        pacMvField[1] = cMvFieldNeighbours[1 + 2*uiMergeCand];

#if POZNAN_DBMP_CALC_PRED_DATA
		if(uiMergeCand==POZNAN_DBMP_MRG_CAND)
		{
			TComCUMvField* pcDBMPPredMvField;

			pcDBMPPredMvField = pcCU->getSlice()->getMP()->getDBMPPredMVField(REF_PIC_LIST_0);
			pcDBMPPredMvField->setMv(pcCU->getCUMvField(REF_PIC_LIST_0)->getMv(uiAbsPartIdx),0);
			pcDBMPPredMvField->setRefIdx(pcCU->getCUMvField(REF_PIC_LIST_0)->getRefIdx(uiAbsPartIdx),0);

			pcDBMPPredMvField = pcCU->getSlice()->getMP()->getDBMPPredMVField(REF_PIC_LIST_1);
			pcDBMPPredMvField->setMv(pcCU->getCUMvField(REF_PIC_LIST_1)->getMv(uiAbsPartIdx),0);
			pcDBMPPredMvField->setRefIdx(pcCU->getCUMvField(REF_PIC_LIST_1)->getRefIdx(uiAbsPartIdx),0);
		}
#endif

        uiInterDir = uhInterDirNeighbours[uiMergeCand];
        uiMergeIndex = uiMergeCand;
        for( UInt ui = 0; ui < MRG_MAX_NUM_CANDS; ui++ )
        {
          UChar uhNeighCand = uiNeighbourCandIdx[ui];
          puhNeighCands[ui] = uhNeighCand;
        }

        uiBestSAD = uiCostCand;
        uiBestBitCost = m_pcRdCost->getCost( uiBitsCand );
        uiBestBits = uiBitsCand;
      }
    }
  }
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
Void TEncSearch::predInterSearch( TComDataCU* pcCU, TComYuv* pcOrgYuv, TComYuv*& rpcPredYuv, TComYuv*& rpcResiYuv, TComYuv*& rpcRecoYuv, Bool bUseRes )
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

#ifdef WEIGHT_PRED
  Int           iRefIdx[2]={0,0}; //If un-initialized, may cause SEGV in bi-directional prediction iterative stage.
#else
  Int           iRefIdx[2];
#endif
  Int           iRefIdxBi[2];

  UInt          uiPartAddr;
  Int           iRoiWidth, iRoiHeight;

  UInt          uiMbBits[3] = {1, 1, 0};

  UInt          uiLastMode = 0;
  Int           iRefStart, iRefEnd;

  PartSize      ePartSize = pcCU->getPartitionSize( 0 );

#if ZERO_MVD_EST
  Int           aiZeroMvdMvpIdx[2] = {-1, -1};
  Int           aiZeroMvdRefIdx[2] = {0, 0};
  Int           iZeroMvdDir = -1;
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

#if DCM_COMB_LIST
    UInt          uiCostTempL0[MAX_NUM_REF];
    for (Int iNumRef=0; iNumRef < MAX_NUM_REF; iNumRef++) uiCostTempL0[iNumRef] = MAX_UINT;
#endif

    xGetBlkBits( ePartSize, pcCU->getSlice()->isInterP(), iPartIdx, uiLastMode, uiMbBits);

    pcCU->getPartIndexAndSize( iPartIdx, uiPartAddr, iRoiWidth, iRoiHeight );

#if PART_MRG
    Bool bTestNormalMC = true;
    if (pcCU->getWidth( 0 ) > 8 && iNumPart == 2 && iPartIdx == 0)
      bTestNormalMC = false;
    if (bTestNormalMC)
    {
#endif

    //  Uni-directional prediction
    for ( Int iRefList = 0; iRefList < iNumPredDir; iRefList++ )
    {
      RefPicList  eRefPicList = ( iRefList ? REF_PIC_LIST_1 : REF_PIC_LIST_0 );

      for ( Int iRefIdxTemp = 0; iRefIdxTemp < pcCU->getSlice()->getNumRefIdx(eRefPicList); iRefIdxTemp++ )
      {
#ifdef WEIGHT_PRED
        if ( eRefPicList == REF_PIC_LIST_0 ) setWpScalingDistParam( pcCU, iRefIdxTemp, -1 , eRefPicList);
        if ( eRefPicList == REF_PIC_LIST_1 ) setWpScalingDistParam( pcCU, -1, iRefIdxTemp , eRefPicList);
#endif
        uiBitsTemp = uiMbBits[iRefList];
        if ( pcCU->getSlice()->getNumRefIdx(eRefPicList) > 1 )
        {
          uiBitsTemp += iRefIdxTemp+1;
          if ( iRefIdxTemp == pcCU->getSlice()->getNumRefIdx(eRefPicList)-1 ) uiBitsTemp--;
        }
#if ZERO_MVD_EST
        xEstimateMvPredAMVP( pcCU, pcOrgYuv, iPartIdx, eRefPicList, iRefIdxTemp, cMvPred[iRefList][iRefIdxTemp], false, &uiZeroMvdDistTemp);
#else
        xEstimateMvPredAMVP( pcCU, pcOrgYuv, iPartIdx, eRefPicList, iRefIdxTemp, cMvPred[iRefList][iRefIdxTemp]);
#endif
        aaiMvpIdx[iRefList][iRefIdxTemp] = pcCU->getMVPIdx(eRefPicList, uiPartAddr);
        aaiMvpNum[iRefList][iRefIdxTemp] = pcCU->getMVPNum(eRefPicList, uiPartAddr);

        uiBitsTemp += m_auiMVPIdxCost[aaiMvpIdx[iRefList][iRefIdxTemp]][aaiMvpNum[iRefList][iRefIdxTemp]];
#if ZERO_MVD_EST
#if DCM_COMB_LIST
        if ((iRefList != 1 || !pcCU->getSlice()->getNoBackPredFlag()) &&
            (pcCU->getSlice()->getNumRefIdx(REF_PIC_LIST_C) <= 0 || pcCU->getSlice()->getRefIdxOfLC(eRefPicList, iRefIdxTemp)>=0))
#endif
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
#if DCM_COMB_LIST
        if ( pcCU->getSlice()->getSPS()->getUseLDC() || pcCU->getSlice()->getNumRefIdx(REF_PIC_LIST_C) > 0)
#else
        if ( pcCU->getSlice()->getSPS()->getUseLDC() )
#endif
        {
#if DCM_COMB_LIST
          if ( iRefList && ( (pcCU->getSlice()->getSPS()->getUseLDC() && (iRefIdxTemp != iRefIdx[0])) || pcCU->getSlice()->getNoBackPredFlag() || (pcCU->getSlice()->getNumRefIdx(REF_PIC_LIST_C) > 0 && !pcCU->getSlice()->getNoBackPredFlag() && pcCU->getSlice()->getRefIdxOfL0FromRefIdxOfL1(iRefIdxTemp)>=0 ) ) )
#else
          if ( iRefList && ( iRefIdxTemp != iRefIdx[0] || pcCU->getSlice()->getNoBackPredFlag() ) )
#endif
            {
#if DCM_COMB_LIST
              if (pcCU->getSlice()->getNumRefIdx(REF_PIC_LIST_C) > 0 && !pcCU->getSlice()->getNoBackPredFlag())
              {
                uiCostTemp = uiCostTempL0[pcCU->getSlice()->getRefIdxOfL0FromRefIdxOfL1(iRefIdxTemp)];
              }
              else
              {
                uiCostTemp = MAX_UINT;
              }
#else
              uiCostTemp = MAX_UINT;
#endif
#if DCM_COMB_LIST
              if ( pcCU->getSlice()->getNoBackPredFlag() || pcCU->getSlice()->getSPS()->getUseLDC() )
#else
              if ( pcCU->getSlice()->getNoBackPredFlag() )
#endif
              {
                cMvTemp[1][iRefIdxTemp] = cMvTemp[0][iRefIdxTemp];
              }
#if DCM_COMB_LIST
              else
              {
                cMvTemp[1][iRefIdxTemp] = cMvTemp[0][pcCU->getSlice()->getRefIdxOfL0FromRefIdxOfL1(iRefIdxTemp)];
              }
#endif
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

#if DCM_COMB_LIST
        if(pcCU->getSlice()->getNumRefIdx(REF_PIC_LIST_C) > 0 && !pcCU->getSlice()->getNoBackPredFlag())
        {
          if(iRefList==REF_PIC_LIST_0)
          {
            uiCostTempL0[iRefIdxTemp] = uiCostTemp;
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
#endif

        if ( ( iRefList == 0 && uiCostTemp < uiCost[iRefList] ) ||
            ( iRefList == 1 &&  pcCU->getSlice()->getNoBackPredFlag() && iRefIdxTemp == iRefIdx[0] ) ||
#if DCM_COMB_LIST
            ( iRefList == 1 && (pcCU->getSlice()->getNumRefIdx(REF_PIC_LIST_C) > 0) && (iRefIdxTemp==0 || iRefIdxTemp == iRefIdx[0]) && !pcCU->getSlice()->getNoBackPredFlag() && (iRefIdxTemp == pcCU->getSlice()->getRefIdxOfL0FromRefIdxOfL1(iRefIdxTemp)) ) ||
#endif
            ( iRefList == 1 && !pcCU->getSlice()->getNoBackPredFlag() && uiCostTemp < uiCost[iRefList] ) )
          {
            uiCost[iRefList] = uiCostTemp;
            uiBits[iRefList] = uiBitsTemp; // storing for bi-prediction

            // set motion
            cMv[iRefList]     = cMvTemp[iRefList][iRefIdxTemp];
            iRefIdx[iRefList] = iRefIdxTemp;
            pcCU->getCUMvField(eRefPicList)->setAllMvField( cMv[iRefList], iRefIdx[iRefList], ePartSize, uiPartAddr, iPartIdx, 0 );

            // storing list 1 prediction signal for iterative bi-directional prediction
            if ( eRefPicList == REF_PIC_LIST_1 )
            {
              TComYuv*  pcYuvPred = &m_acYuvPred[iRefList];
              motionCompensation ( pcCU, pcYuvPred, eRefPicList, iPartIdx );
            }
#if DCM_COMB_LIST
            if ( (pcCU->getSlice()->getNoBackPredFlag() || (pcCU->getSlice()->getNumRefIdx(REF_PIC_LIST_C) > 0 && pcCU->getSlice()->getRefIdxOfL0FromRefIdxOfL1(0)==0 )) && eRefPicList == REF_PIC_LIST_0 )
#else
            if ( pcCU->getSlice()->getNoBackPredFlag() && eRefPicList == REF_PIC_LIST_0 )
#endif
            {
              TComYuv*  pcYuvPred = &m_acYuvPred[iRefList];
              motionCompensation ( pcCU, pcYuvPred, eRefPicList, iPartIdx );
            }
          }
      }
    }
    //  Bi-directional prediction
    if ( pcCU->getSlice()->isInterB() )
    {

      cMvBi[0] = cMv[0];            cMvBi[1] = cMv[1];
      iRefIdxBi[0] = iRefIdx[0];    iRefIdxBi[1] = iRefIdx[1];

      ::memcpy(cMvPredBi, cMvPred, sizeof(cMvPred));
      ::memcpy(aaiMvpIdxBi, aaiMvpIdx, sizeof(aaiMvpIdx));

      UInt uiMotBits[2] = { uiBits[0] - uiMbBits[0], uiBits[1] - uiMbBits[1] };
      uiBits[2] = uiMbBits[2] + uiMotBits[0] + uiMotBits[1];

      // 4-times iteration (default)
      Int iNumIter = 4;

      // fast encoder setting: only one iteration
      if ( m_pcEncCfg->getUseFastEnc() )
      {
        iNumIter = 1;
      }

      for ( Int iIter = 0; iIter < iNumIter; iIter++ )
      {

        Int         iRefList    = iIter % 2;
#if DCM_COMB_LIST
        if ( m_pcEncCfg->getUseFastEnc() && (pcCU->getSlice()->getNoBackPredFlag() || (pcCU->getSlice()->getNumRefIdx(REF_PIC_LIST_C) > 0 && pcCU->getSlice()->getRefIdxOfL0FromRefIdxOfL1(0)==0 )) )
#else
        if ( m_pcEncCfg->getUseFastEnc() && pcCU->getSlice()->getNoBackPredFlag() )
#endif
        {
          iRefList = 1;
        }
        RefPicList  eRefPicList = ( iRefList ? REF_PIC_LIST_1 : REF_PIC_LIST_0 );

        Bool bChanged = false;

#if GPB_SIMPLE
        if ( pcCU->getSlice()->getSPS()->getUseLDC() && iRefList )
        {
          iRefStart = iRefIdxBi[1-iRefList];
          iRefEnd   = iRefIdxBi[1-iRefList];
        }
        else
        {
          iRefStart = 0;
          iRefEnd   = pcCU->getSlice()->getNumRefIdx(eRefPicList)-1;
        }
#else
        iRefStart = 0;
        iRefEnd   = pcCU->getSlice()->getNumRefIdx(eRefPicList)-1;
#endif

#if DCM_COMB_LIST
        if ( m_pcEncCfg->getUseFastEnc() && (pcCU->getSlice()->getNoBackPredFlag() || (pcCU->getSlice()->getNumRefIdx(REF_PIC_LIST_C) > 0 && pcCU->getSlice()->getRefIdxOfL0FromRefIdxOfL1(0)==0 )) )
#else
        if ( m_pcEncCfg->getUseFastEnc() && pcCU->getSlice()->getNoBackPredFlag() )
#endif
        {
          iRefStart = 0;
          iRefEnd   = pcCU->getSlice()->getNumRefIdx(eRefPicList)-1;
        }

        for ( Int iRefIdxTemp = iRefStart; iRefIdxTemp <= iRefEnd; iRefIdxTemp++ )
        {
#ifdef WEIGHT_PRED
          if ( eRefPicList == REF_PIC_LIST_0 ) setWpScalingDistParam( pcCU, iRefIdxTemp, -1 , eRefPicList);
          if ( eRefPicList == REF_PIC_LIST_1 ) setWpScalingDistParam( pcCU, -1, iRefIdxTemp , eRefPicList);
#endif
          uiBitsTemp = uiMbBits[2] + uiMotBits[1-iRefList];
          if ( pcCU->getSlice()->getNumRefIdx(eRefPicList) > 1 )
          {
            uiBitsTemp += iRefIdxTemp+1;
            if ( iRefIdxTemp == pcCU->getSlice()->getNumRefIdx(eRefPicList)-1 ) uiBitsTemp--;
          }

          uiBitsTemp += m_auiMVPIdxCost[aaiMvpIdxBi[iRefList][iRefIdxTemp]][aaiMvpNum[iRefList][iRefIdxTemp]];
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

            //  Set motion
            pcCU->getCUMvField( eRefPicList )->setAllMvField( cMvBi[iRefList], iRefIdxBi[iRefList], ePartSize, uiPartAddr, iPartIdx, 0 );

            TComYuv* pcYuvPred = &m_acYuvPred[iRefList];
            motionCompensation( pcCU, pcYuvPred, eRefPicList, iPartIdx );
          }
        } // for loop-iRefIdxTemp

        if ( !bChanged )
        {
          if ( uiCostBi <= uiCost[0] && uiCostBi <= uiCost[1] && pcCU->getAMVPMode(uiPartAddr) == AM_EXPL )
          {
            xCopyAMVPInfo(&aacAMVPInfo[0][iRefIdxBi[0]], pcCU->getCUMvField(REF_PIC_LIST_0)->getAMVPInfo());
            xCheckBestMVP(pcCU, REF_PIC_LIST_0, cMvBi[0], cMvPredBi[0][iRefIdxBi[0]], aaiMvpIdxBi[0][iRefIdxBi[0]], uiBits[2], uiCostBi);
            xCopyAMVPInfo(&aacAMVPInfo[1][iRefIdxBi[1]], pcCU->getCUMvField(REF_PIC_LIST_1)->getAMVPInfo());
            xCheckBestMVP(pcCU, REF_PIC_LIST_1, cMvBi[1], cMvPredBi[1][iRefIdxBi[1]], aaiMvpIdxBi[1][iRefIdxBi[1]], uiBits[2], uiCostBi);
          }
          break;
        }
      } // for loop-iter
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
#if PART_MRG
    } //end if bTestNormalMC
#endif
    //  Clear Motion Field
    {
      pcCU->getCUMvField(REF_PIC_LIST_0)->setAllMvField( cMvZero, NOT_VALID, ePartSize, uiPartAddr, iPartIdx, 0 );
      pcCU->getCUMvField(REF_PIC_LIST_1)->setAllMvField( cMvZero, NOT_VALID, ePartSize, uiPartAddr, iPartIdx, 0 );
    }
    pcCU->getCUMvField(REF_PIC_LIST_0)->setAllMvd    ( cMvZero,            ePartSize, uiPartAddr, iPartIdx, 0 );
    pcCU->getCUMvField(REF_PIC_LIST_1)->setAllMvd    ( cMvZero,            ePartSize, uiPartAddr, iPartIdx, 0 );

    pcCU->setMVPIdxSubParts( -1, REF_PIC_LIST_0, uiPartAddr, iPartIdx, pcCU->getDepth(uiPartAddr));
    pcCU->setMVPNumSubParts( -1, REF_PIC_LIST_0, uiPartAddr, iPartIdx, pcCU->getDepth(uiPartAddr));
    pcCU->setMVPIdxSubParts( -1, REF_PIC_LIST_1, uiPartAddr, iPartIdx, pcCU->getDepth(uiPartAddr));
    pcCU->setMVPNumSubParts( -1, REF_PIC_LIST_1, uiPartAddr, iPartIdx, pcCU->getDepth(uiPartAddr));

    UInt uiMEBits = 0;
    // Set Motion Field_
#if DCM_COMB_LIST
    if ( pcCU->getSlice()->getNoBackPredFlag() || (pcCU->getSlice()->getNumRefIdx(REF_PIC_LIST_C) > 0 && pcCU->getSlice()->getRefIdxOfL0FromRefIdxOfL1(0)==0 ) )
#else
    if ( pcCU->getSlice()->getNoBackPredFlag() )
#endif
    {
      uiCost[1] = MAX_UINT;
    }
#if PART_MRG
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
        pcCU->getCUMvField(REF_PIC_LIST_0)->setAllMvField( cMvBi[0], iRefIdxBi[0], ePartSize, uiPartAddr, iPartIdx, 0 );
        pcCU->getCUMvField(REF_PIC_LIST_1)->setAllMvField( cMvBi[1], iRefIdxBi[1], ePartSize, uiPartAddr, iPartIdx, 0 );
      }
      {
        TempMv = cMvBi[0] - cMvPredBi[0][iRefIdxBi[0]];
        pcCU->getCUMvField(REF_PIC_LIST_0)->setAllMvd    ( TempMv,                 ePartSize, uiPartAddr, iPartIdx, 0 );
#if DCM_SIMPLIFIED_MVP==0
        if (pcCU->clearMVPCand(TempMv, &aacAMVPInfo[0][iRefIdxBi[0]]))
        {
          aaiMvpIdxBi[0][iRefIdxBi[0]] = pcCU->searchMVPIdx(cMvPredBi[0][iRefIdxBi[0]], &aacAMVPInfo[0][iRefIdxBi[0]]);
          aaiMvpNum[0][iRefIdxBi[0]] = aacAMVPInfo[0][iRefIdxBi[0]].iN;
        }
#endif
      }
      {
        TempMv = cMvBi[1] - cMvPredBi[1][iRefIdxBi[1]];
        pcCU->getCUMvField(REF_PIC_LIST_1)->setAllMvd    ( TempMv,                 ePartSize, uiPartAddr, iPartIdx, 0 );
#if DCM_SIMPLIFIED_MVP==0
        if (pcCU->clearMVPCand(TempMv, &aacAMVPInfo[1][iRefIdxBi[1]]))
        {
          aaiMvpIdxBi[1][iRefIdxBi[1]] = pcCU->searchMVPIdx(cMvPredBi[1][iRefIdxBi[1]], &aacAMVPInfo[1][iRefIdxBi[1]]);
          aaiMvpNum[1][iRefIdxBi[1]] = aacAMVPInfo[1][iRefIdxBi[1]].iN;
        }
#endif
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
      pcCU->getCUMvField(REF_PIC_LIST_0)->setAllMvField( cMv[0],   iRefIdx[0],   ePartSize, uiPartAddr, iPartIdx, 0 );

      {
        TempMv = cMv[0] - cMvPred[0][iRefIdx[0]];
        pcCU->getCUMvField(REF_PIC_LIST_0)->setAllMvd    ( TempMv,                 ePartSize, uiPartAddr, iPartIdx, 0 );
#if DCM_SIMPLIFIED_MVP==0
        if (pcCU->clearMVPCand(TempMv, &aacAMVPInfo[0][iRefIdx[0]]))
        {
          aaiMvpIdx[0][iRefIdx[0]] = pcCU->searchMVPIdx(cMvPred[0][iRefIdx[0]], &aacAMVPInfo[0][iRefIdx[0]]);
          aaiMvpNum[0][iRefIdx[0]] = aacAMVPInfo[0][iRefIdx[0]].iN;
        }
#endif
      }
      pcCU->setInterDirSubParts( 1, uiPartAddr, iPartIdx, pcCU->getDepth(0) );

      pcCU->setMVPIdxSubParts( aaiMvpIdx[0][iRefIdx[0]], REF_PIC_LIST_0, uiPartAddr, iPartIdx, pcCU->getDepth(uiPartAddr));
      pcCU->setMVPNumSubParts( aaiMvpNum[0][iRefIdx[0]], REF_PIC_LIST_0, uiPartAddr, iPartIdx, pcCU->getDepth(uiPartAddr));

      uiMEBits = uiBits[0];
    }
    else
    {
      uiLastMode = 1;
      pcCU->getCUMvField(REF_PIC_LIST_1)->setAllMvField( cMv[1],   iRefIdx[1],   ePartSize, uiPartAddr, iPartIdx, 0 );

      {
        TempMv = cMv[1] - cMvPred[1][iRefIdx[1]];
        pcCU->getCUMvField(REF_PIC_LIST_1)->setAllMvd    ( TempMv,                 ePartSize, uiPartAddr, iPartIdx, 0 );
#if DCM_SIMPLIFIED_MVP==0
        if (pcCU->clearMVPCand(TempMv, &aacAMVPInfo[1][iRefIdx[1]]))
        {
          aaiMvpIdx[1][iRefIdx[1]] = pcCU->searchMVPIdx(cMvPred[1][iRefIdx[1]], &aacAMVPInfo[1][iRefIdx[1]]);
          aaiMvpNum[1][iRefIdx[1]] = aacAMVPInfo[1][iRefIdx[1]].iN;
        }
#endif
      }
      pcCU->setInterDirSubParts( 2, uiPartAddr, iPartIdx, pcCU->getDepth(0) );

      pcCU->setMVPIdxSubParts( aaiMvpIdx[1][iRefIdx[1]], REF_PIC_LIST_1, uiPartAddr, iPartIdx, pcCU->getDepth(uiPartAddr));
      pcCU->setMVPNumSubParts( aaiMvpNum[1][iRefIdx[1]], REF_PIC_LIST_1, uiPartAddr, iPartIdx, pcCU->getDepth(uiPartAddr));

      uiMEBits = uiBits[1];
    }
#if PART_MRG
    } // end if bTestNormalMC
#endif

    if ( pcCU->getSlice()->getSPS()->getUseMRG() && pcCU->getPartitionSize( uiPartAddr ) == SIZE_2Nx2N )
    {
      UChar       uhInterDirNeighbours[MRG_MAX_NUM_CANDS];
      UInt        uiNeighbourCandIdx  [MRG_MAX_NUM_CANDS];
      TComMvField cMvFieldNeighbours  [MRG_MAX_NUM_CANDS << 1]; // double length for mv of both lists
      for( UInt ui = 0; ui < MRG_MAX_NUM_CANDS; ui++ )
      {
        uhInterDirNeighbours[ui] = 0;
        uiNeighbourCandIdx  [ui] = 0;
      }
      pcCU->getInterMergeCandidates( uiPartAddr, iPartIdx, pcCU->getDepth( uiPartAddr ), cMvFieldNeighbours, uhInterDirNeighbours, uiNeighbourCandIdx );
      for( UInt ui = 0; ui < MRG_MAX_NUM_CANDS; ui++ )
      {
        pcCU->setNeighbourCandIdxSubParts( ui, uiNeighbourCandIdx[ui], uiPartAddr, iPartIdx, pcCU->getDepth( uiPartAddr ) );
      }
    }

    if ( pcCU->getSlice()->getSPS()->getUseMRG() && pcCU->getPartitionSize( uiPartAddr ) != SIZE_2Nx2N )
    {
      UInt uiMRGInterDir = 0;
      TComMvField cMRGMvField[2];
      UInt uiMRGIndex = 0;

      UInt uiMEInterDir = 0;
      TComMvField cMEMvField[2];

      m_pcRdCost->getMotionCost( 1, 0 );
#if PART_MRG
      // calculate ME cost
      UInt uiMEError = MAX_UINT;
      UInt uiMECost = MAX_UINT;

      if (bTestNormalMC)
      {
        xGetInterPredictionError( pcCU, pcOrgYuv, iPartIdx, uiMEError, m_pcEncCfg->getUseHADME() );
        uiMECost = uiMEError + m_pcRdCost->getCost( uiMEBits );
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
      UInt uiMRGError = MAX_UINT;
      UInt uiMRGBits = MAX_UINT;
      Bool bMergeValid = false;
      UChar ucNeighCand[MRG_MAX_NUM_CANDS];
      for( UInt ui = 0; ui < MRG_MAX_NUM_CANDS; ui++ )
      {
        ucNeighCand[ui] = 0;
      }
      xMergeEstimation( pcCU, pcOrgYuv, iPartIdx, uiMRGInterDir, cMRGMvField, uiMRGIndex, uiMRGError, uiMRGBits, ucNeighCand, bMergeValid );
      UInt uiMRGCost = uiMRGError + m_pcRdCost->getCost( uiMRGBits );

      for( UInt ui = 0; ui < MRG_MAX_NUM_CANDS; ui++ )
      {
        pcCU->setNeighbourCandIdxSubParts( ui, ucNeighCand[ui], uiPartAddr, iPartIdx, pcCU->getDepth( uiPartAddr ) );
      }
      if ( bMergeValid && uiMRGCost < uiMECost )
      {
        // set Merge result
        pcCU->setMergeFlagSubParts ( true,          uiPartAddr, iPartIdx, pcCU->getDepth( uiPartAddr ) );
        pcCU->setMergeIndexSubParts( uiMRGIndex,    uiPartAddr, iPartIdx, pcCU->getDepth( uiPartAddr ) );
        pcCU->setInterDirSubParts  ( uiMRGInterDir, uiPartAddr, iPartIdx, pcCU->getDepth( uiPartAddr ) );

#if POZNAN_DBMP_CALC_PRED_DATA
		if(uiMRGIndex==POZNAN_DBMP_MRG_CAND)
		{
			TComCUMvField* pcDBMPPredMvField;
			pcDBMPPredMvField = pcCU->getSlice()->getMP()->getDBMPPredMVField(REF_PIC_LIST_0);
			pcCU->getCUMvField( REF_PIC_LIST_0 )->setAllMvField( pcDBMPPredMvField->getMv(0), pcDBMPPredMvField->getRefIdx(0), ePartSize, uiPartAddr, iPartIdx, 0 );			
			pcDBMPPredMvField = pcCU->getSlice()->getMP()->getDBMPPredMVField(REF_PIC_LIST_1);
			pcCU->getCUMvField( REF_PIC_LIST_1 )->setAllMvField( pcDBMPPredMvField->getMv(0), pcDBMPPredMvField->getRefIdx(0), ePartSize, uiPartAddr, iPartIdx, 0 );

			pcCU->getCUMvField2nd( REF_PIC_LIST_0 )->setAllMvField( cMRGMvField[0].getMv(), cMRGMvField[0].getRefIdx(), ePartSize, uiPartAddr, iPartIdx, 0 );
			pcCU->getCUMvField2nd( REF_PIC_LIST_1 )->setAllMvField( cMRGMvField[1].getMv(), cMRGMvField[1].getRefIdx(), ePartSize, uiPartAddr, iPartIdx, 0 );
		}
		else
#endif
        {
          pcCU->getCUMvField( REF_PIC_LIST_0 )->setAllMvField( cMRGMvField[0].getMv(), cMRGMvField[0].getRefIdx(), ePartSize, uiPartAddr, iPartIdx, 0 );
          pcCU->getCUMvField( REF_PIC_LIST_1 )->setAllMvField( cMRGMvField[1].getMv(), cMRGMvField[1].getRefIdx(), ePartSize, uiPartAddr, iPartIdx, 0 );
        }

        pcCU->getCUMvField(REF_PIC_LIST_0)->setAllMvd    ( cMvZero,            ePartSize, uiPartAddr, iPartIdx, 0 );
        pcCU->getCUMvField(REF_PIC_LIST_1)->setAllMvd    ( cMvZero,            ePartSize, uiPartAddr, iPartIdx, 0 );
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
          pcCU->getCUMvField( REF_PIC_LIST_0 )->setAllMvField( cMEMvField[0].getMv(), cMEMvField[0].getRefIdx(), ePartSize, uiPartAddr, iPartIdx, 0 );
          pcCU->getCUMvField( REF_PIC_LIST_1 )->setAllMvField( cMEMvField[1].getMv(), cMEMvField[1].getRefIdx(), ePartSize, uiPartAddr, iPartIdx, 0 );
        }
      }
#if PART_MRG
      if (!bTestNormalMC && !bMergeValid)
      {
        assert(pcCU->getWidth( 0 ) > 8 && iNumPart == 2 && iPartIdx == 0);
        return;
      }
#endif
    }

    //  MC
    motionCompensation ( pcCU, rpcPredYuv, REF_PIC_LIST_X, iPartIdx );

  } //  end of for ( Int iPartIdx = 0; iPartIdx < iNumPart; iPartIdx++ )

#ifdef WEIGHT_PRED
  setWpScalingDistParam( pcCU, -1, -1, REF_PIC_LIST_X );
#endif
  return;
}

// AMVP
#if ZERO_MVD_EST
Void TEncSearch::xEstimateMvPredAMVP( TComDataCU* pcCU, TComYuv* pcOrgYuv, UInt uiPartIdx, RefPicList eRefPicList, Int iRefIdx, TComMv& rcMvPred, Bool bFilled, UInt* puiDist )
#else
Void TEncSearch::xEstimateMvPredAMVP( TComDataCU* pcCU, TComYuv* pcOrgYuv, UInt uiPartIdx, RefPicList eRefPicList, Int iRefIdx, TComMv& rcMvPred, Bool bFilled )
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
#if ZERO_MVD_EST
      uiTmpCost = xGetTemplateCost( pcCU, uiPartIdx, uiPartAddr, pcOrgYuv, &m_cYuvPredTemp, pcAMVPInfo->m_acMvCand[i], i, pcAMVPInfo->iN, eRefPicList, iRefIdx, iRoiWidth, iRoiHeight, uiDist );
#else
      uiTmpCost = xGetTemplateCost( pcCU, uiPartIdx, uiPartAddr, pcOrgYuv, &m_cYuvPredTemp, pcAMVPInfo->m_acMvCand[i], i, pcAMVPInfo->iN, eRefPicList, iRefIdx, iRoiWidth, iRoiHeight);
#endif
      if ( uiBestCost > uiTmpCost )
      {
        uiBestCost = uiTmpCost;
        cBestMv   = pcAMVPInfo->m_acMvCand[i];
        iBestIdx  = i;
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
  else if ( eCUMode == SIZE_2NxN )
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
  else if ( eCUMode == SIZE_Nx2N )
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

  m_pcRdCost->setPredictor( rcMvPred );
  Int iOrgMvBits  = m_pcRdCost->getBits(cMv.getHor(), cMv.getVer());
  Int iBestMvBits = iOrgMvBits;

  for (Int iMVPIdx = 0; iMVPIdx < pcAMVPInfo->iN; iMVPIdx++)
  {
    if (iMVPIdx == riMVPIdx) continue;

    m_pcRdCost->setPredictor( pcAMVPInfo->m_acMvCand[iMVPIdx] );

    Int iMvBits = m_pcRdCost->getBits(cMv.getHor(), cMv.getVer());

    if (iMvBits < iBestMvBits)
    {
      iBestMvBits = iMvBits;
      iBestMVPIdx = iMVPIdx;
    }
  }

  if (iBestMVPIdx != riMVPIdx)  //if changed
  {
    rcMvPred = pcAMVPInfo->m_acMvCand[iBestMVPIdx];

    iOrgMvBits  += m_auiMVPIdxCost[riMVPIdx][pcAMVPInfo->iN];
    iBestMvBits += m_auiMVPIdxCost[iBestMVPIdx][pcAMVPInfo->iN];

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

  // prediction pattern
  xPredInterLumaBlk( pcCU, pcPicYuvRef, uiPartAddr, &cMvCand, iSizeX, iSizeY, pcTemplateCand );

#ifdef WEIGHT_PRED
  if ( pcCU->getSlice()->getPPS()->getUseWP() )
  {
    PartSize  ePartSize = pcCU->getPartitionSize( 0 );
    pcCU->getCUMvField(eRefPicList)->setAllMvField( cMvCand, iRefIdx, ePartSize, uiPartAddr, uiPartIdx, 0 );
#if 0
    TComMvField mvField;
    mvField.setMvField( cMvCand, iRefIdx);
    pcCU->getCUMvField(eRefPicList)->setAllMvField( mvField, ePartSize, uiPartAddr, 0 );
#endif
    xWeightedPredictionUni( pcCU, pcTemplateCand, uiPartAddr, iSizeX, iSizeY, eRefPicList, pcTemplateCand, uiPartIdx );
  }
#endif

  // calc distortion
#if ZERO_MVD_EST
  m_pcRdCost->getMotionCost( 1, 0 );
  DistParam cDistParam;
  m_pcRdCost->setDistParam( cDistParam,
                            pcOrgYuv->getLumaAddr(uiPartAddr), pcOrgYuv->getStride(),
                            pcTemplateCand->getLumaAddr(uiPartAddr), pcTemplateCand->getStride(),
                            iSizeX, iSizeY, m_pcEncCfg->getUseHADME() );
  // MW: check VSO here
  ruiDist = cDistParam.DistFunc( &cDistParam );
  uiCost = ruiDist + m_pcRdCost->getCost( m_auiMVPIdxCost[iMVPIdx][iMVPNum] );
#else
#if HHI_VSO
  if ( false /*m_pcRdCost->getUseVSO()*/ ) // GT: currently disabled
  {
    uiCost = m_pcRdCost->getDistVS  ( pcCU, uiPartAddr,  pcTemplateCand->getLumaAddr(uiPartAddr), pcTemplateCand->getStride(), pcOrgYuv->getLumaAddr(uiPartAddr), pcOrgYuv->getStride(), iSizeX, iSizeY, true, 0);
    uiCost = (UInt) m_pcRdCost->calcRdCostVSO( m_auiMVPIdxCost[iMVPIdx][iMVPNum], uiCost, false, DF_SAD );
  }
  else
#endif
  {
    uiCost = m_pcRdCost->getDistPart( pcTemplateCand->getLumaAddr(uiPartAddr), pcTemplateCand->getStride(), pcOrgYuv->getLumaAddr(uiPartAddr), pcOrgYuv->getStride(), iSizeX, iSizeY, DF_SAD );
    uiCost = (UInt) m_pcRdCost->calcRdCost( m_auiMVPIdxCost[iMVPIdx][iMVPNum], uiCost, false, DF_SAD );
  }
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
#ifdef ROUNDING_CONTROL_BIPRED
  Pel           pRefBufY[16384];  // 128x128
#endif
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

#ifdef ROUNDING_CONTROL_BIPRED
    Int y;
    //Int x;
    Pel *pRefY = pcYuvOther->getLumaAddr(uiPartAddr);
    Int iRefStride = pcYuvOther->getStride();

    // copy the MC block into pRefBufY
    for( y = 0; y < iRoiHeight; y++)
    {
      memcpy(pRefBufY+y*iRoiWidth,pRefY,sizeof(Pel)*iRoiWidth);
      pRefY += iRefStride;
    }
#else
    pcYuv->removeHighFreq( pcYuvOther, uiPartAddr, iRoiWidth, iRoiHeight );
#endif

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
  if( pcCU->getSlice()->getSPS()->isDepth() )
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

  //  Do integer search
#ifdef ROUNDING_CONTROL_BIPRED
  if( bBi )
  {
    xPatternSearch_Bi      ( pcPatternKey, piRefY, iRefStride, &cMvSrchRngLT, &cMvSrchRngRB, rcMv, ruiCost, pRefBufY, pcCU->getSlice()->isRounding() );
  }
  else
  {
    if ( !m_iFastSearch)
    {
      xPatternSearch      ( pcPatternKey, piRefY, iRefStride, &cMvSrchRngLT, &cMvSrchRngRB, rcMv, ruiCost );
    }
    else
    {
      rcMv = *pcMvPred;
      xPatternSearchFast  ( pcCU, pcPatternKey, piRefY, iRefStride, &cMvSrchRngLT, &cMvSrchRngRB, rcMv, ruiCost );
    }
  }
#else
  if ( !m_iFastSearch || bBi )
  {
    xPatternSearch      ( pcPatternKey, piRefY, iRefStride, &cMvSrchRngLT, &cMvSrchRngRB, rcMv, ruiCost );
  }
  else
  {
    rcMv = ( m_pcRdCost->useMultiviewReg() ? m_pcRdCost->getMultiviewOrgMvPred() : *pcMvPred );
    xPatternSearchFast  ( pcCU, pcPatternKey, piRefY, iRefStride, &cMvSrchRngLT, &cMvSrchRngRB, rcMv, ruiCost );
  }
#endif


  m_pcRdCost->getMotionCost( 1, 0 );
#if HHI_FULL_PEL_DEPTH_MAP_MV_ACC
  if( ! pcCU->getSlice()->getSPS()->isDepth() )
  {
#endif
  m_pcRdCost->setCostScale ( 1 );

#ifdef ROUNDING_CONTROL_BIPRED
  if( bBi )
  {
    Bool bRound =  pcCU->getSlice()->isRounding() ;
    xPatternSearchFracDIF_Bi( pcCU, pcPatternKey, piRefY, iRefStride, &rcMv, cMvHalf, cMvQter, ruiCost, pRefBufY, bRound );
  }
  else
#endif
  {
    xPatternSearchFracDIF( pcCU, pcPatternKey, piRefY, iRefStride, &rcMv, cMvHalf, cMvQter, ruiCost );
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
  if( pcCU->getSlice()->getSPS()->isDepth() )
    ruiCost += m_pcRdCost->getCost( uiMvBits );
#endif

  ruiBits   += uiMvBits;
  ruiCost       = (UInt)( floor( fWeight * ( (Double)ruiCost - (Double)m_pcRdCost->getCost( uiMvBits ) ) ) + (Double)m_pcRdCost->getCost( ruiBits ) );
}


Void TEncSearch::xSetSearchRange ( TComDataCU* pcCU, TComMv& cMvPred, Int iSrchRng, TComMv& rcMvSrchRngLT, TComMv& rcMvSrchRngRB )
{
  Int  iMvShift = 2;
#if HHI_FULL_PEL_DEPTH_MAP_MV_ACC
  if( pcCU->getSlice()->getSPS()->isDepth() )
    iMvShift = 0;
#endif
  pcCU->clipMv( cMvPred );

  rcMvSrchRngLT.setHor( cMvPred.getHor() - (iSrchRng << iMvShift) );
  rcMvSrchRngLT.setVer( cMvPred.getVer() - (iSrchRng << iMvShift) );

  rcMvSrchRngRB.setHor( cMvPred.getHor() + (iSrchRng << iMvShift) );
  rcMvSrchRngRB.setVer( cMvPred.getVer() + (iSrchRng << iMvShift) );

  pcCU->clipMv        ( rcMvSrchRngLT );
  pcCU->clipMv        ( rcMvSrchRngRB );

  rcMvSrchRngLT >>= iMvShift;
  rcMvSrchRngRB >>= iMvShift;
}

#ifdef ROUNDING_CONTROL_BIPRED
Void TEncSearch::xPatternSearch_Bi( TComPattern* pcPatternKey, Pel* piRefY, Int iRefStride, TComMv* pcMvSrchRngLT, TComMv* pcMvSrchRngRB, TComMv& rcMv, UInt& ruiSAD, Pel* pcRefY2, Bool bRound )
{
  Int   iSrchRngHorLeft   = pcMvSrchRngLT->getHor();
  Int   iSrchRngHorRight  = pcMvSrchRngRB->getHor();
  Int   iSrchRngVerTop    = pcMvSrchRngLT->getVer();
  Int   iSrchRngVerBottom = pcMvSrchRngRB->getVer();

  UInt  uiSad;
  Dist  uiSadBest         = RDO_DIST_MAX;
  Int   iBestX = 0;
  Int   iBestY = 0;

  Pel*  piRefSrch;

  //-- jclee for using the SAD function pointer
  m_pcRdCost->setDistParam_Bi( pcPatternKey, piRefY, iRefStride,  m_cDistParam);

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
      uiSad = m_cDistParam.DistFuncRnd( &m_cDistParam, pcRefY2, bRound );

      // motion cost
      uiSad += m_pcRdCost->getCost( x, y );

      // regularization cost
      if( m_pcRdCost->useMultiviewReg() )
      {
        uiSad += m_pcRdCost->getMultiviewRegCost( x, y );
      }

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
#endif

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
#ifdef WEIGHT_PRED
      setDistParamComp(0);
#endif
      uiSad = m_cDistParam.DistFunc( &m_cDistParam );

      // motion cost
      uiSad += m_pcRdCost->getCost( x, y );

      // regularization cost
      if( m_pcRdCost->useMultiviewReg() )
      {
        uiSad += m_pcRdCost->getMultiviewRegCost( x, y );
      }

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
  if( ! pcCU->getSlice()->getSPS()->isDepth() )
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
      if( ! pcCU->getSlice()->getSPS()->isDepth() )
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


#ifdef ROUNDING_CONTROL_BIPRED
Void TEncSearch::xPatternSearchFracDIF_Bi( TComDataCU* pcCU, TComPattern* pcPatternKey, Pel* piRefY, Int iRefStride, TComMv* pcMvInt, TComMv& rcMvHalf, TComMv& rcMvQter, UInt& ruiCost, Pel* piRefY2, Bool bRound )
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
  Pel*  piRef;
  iRefStride  = m_cYuvExt.getStride();

  //  Half-pel refinement
  xExtDIFUpSamplingH ( &cPatternRoi, &m_cYuvExt );
  piRef = m_cYuvExt.getLumaAddr() + ((iRefStride + 4) << 2);

  rcMvHalf = *pcMvInt;   rcMvHalf <<= 1;    // for mv-cost
  ruiCost = xPatternRefinement_Bi( pcPatternKey, piRef, iRefStride, 4, 2, rcMvHalf, piRefY2, bRound );

  m_pcRdCost->setCostScale( 0 );

  //  Quater-pel refinement
  Pel*  piSrcPel = cPatternRoi.getROIY() + (rcMvHalf.getHor() >> 1) + cPatternRoi.getPatternLStride() * (rcMvHalf.getVer() >> 1);
  Int*  piSrc    = m_piYuvExt  + ((m_iYuvExtStride + 4) << 2) + (rcMvHalf.getHor() << 1) + m_iYuvExtStride * (rcMvHalf.getVer() << 1);
  piRef += (rcMvHalf.getHor() << 1) + iRefStride * (rcMvHalf.getVer() << 1);
  xExtDIFUpSamplingQ ( pcPatternKey, piRef, iRefStride, piSrcPel, cPatternRoi.getPatternLStride(), piSrc, m_iYuvExtStride, m_puiDFilter[rcMvHalf.getHor()+rcMvHalf.getVer()*3] );

  rcMvQter = *pcMvInt;   rcMvQter <<= 1;    // for mv-cost
  rcMvQter += rcMvHalf;  rcMvQter <<= 1;
  ruiCost = xPatternRefinement_Bi( pcPatternKey, piRef, iRefStride, 4, 1, rcMvQter, piRefY2, bRound );
}

#endif

Void TEncSearch::xPatternSearchFracDIF( TComDataCU* pcCU, TComPattern* pcPatternKey, Pel* piRefY, Int iRefStride, TComMv* pcMvInt, TComMv& rcMvHalf, TComMv& rcMvQter, UInt& ruiCost )
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
  Pel*  piRef;
  iRefStride  = m_cYuvExt.getStride();

  //  Half-pel refinement
  xExtDIFUpSamplingH ( &cPatternRoi, &m_cYuvExt );
  piRef = m_cYuvExt.getLumaAddr() + ((iRefStride + 4) << 2);

  rcMvHalf = *pcMvInt;   rcMvHalf <<= 1;    // for mv-cost
  ruiCost = xPatternRefinement( pcPatternKey, piRef, iRefStride, 4, 2, rcMvHalf   );

  m_pcRdCost->setCostScale( 0 );

  //  Quater-pel refinement
  Pel*  piSrcPel = cPatternRoi.getROIY() + (rcMvHalf.getHor() >> 1) + cPatternRoi.getPatternLStride() * (rcMvHalf.getVer() >> 1);
  Int*  piSrc    = m_piYuvExt  + ((m_iYuvExtStride + 4) << 2) + (rcMvHalf.getHor() << 1) + m_iYuvExtStride * (rcMvHalf.getVer() << 1);
  piRef += (rcMvHalf.getHor() << 1) + iRefStride * (rcMvHalf.getVer() << 1);
  xExtDIFUpSamplingQ ( pcPatternKey, piRef, iRefStride, piSrcPel, cPatternRoi.getPatternLStride(), piSrc, m_iYuvExtStride, m_puiDFilter[rcMvHalf.getHor()+rcMvHalf.getVer()*3] );

  rcMvQter = *pcMvInt;   rcMvQter <<= 1;    // for mv-cost
  rcMvQter += rcMvHalf;  rcMvQter <<= 1;
  ruiCost = xPatternRefinement( pcPatternKey, piRef, iRefStride, 4, 1, rcMvQter );
}

Void TEncSearch::predInterSkipSearch( TComDataCU* pcCU, TComYuv* pcOrgYuv, TComYuv*& rpcPredYuv, TComYuv*& rpcResiYuv, TComYuv*& rpcRecoYuv )
{
  SliceType eSliceType = pcCU->getSlice()->getSliceType();
  if ( eSliceType == I_SLICE )
    return;

  if ( eSliceType == P_SLICE && pcCU->getSlice()->getNumRefIdx( REF_PIC_LIST_0 ) > 0 )
  {
    pcCU->setInterDirSubParts( 1, 0, 0, pcCU->getDepth( 0 ) );

    TComMv cMv;
    TComMv cZeroMv;
    xEstimateMvPredAMVP( pcCU, pcOrgYuv, 0, REF_PIC_LIST_0, 0, cMv, (pcCU->getCUMvField( REF_PIC_LIST_0 )->getAMVPInfo()->iN > 0?  true:false) );
    pcCU->getCUMvField( REF_PIC_LIST_0 )->setAllMvField( cMv, 0, SIZE_2Nx2N, 0, 0, 0 );
    pcCU->getCUMvField( REF_PIC_LIST_0 )->setAllMvd    ( cZeroMv, SIZE_2Nx2N, 0, 0, 0 );  //unnecessary
    pcCU->getCUMvField( REF_PIC_LIST_1 )->setAllMvField( cZeroMv, NOT_VALID, SIZE_2Nx2N, 0, 0, 0 );  //unnecessary
    pcCU->getCUMvField( REF_PIC_LIST_1 )->setAllMvd    ( cZeroMv, SIZE_2Nx2N, 0, 0, 0 );  //unnecessary

    pcCU->setMVPIdxSubParts( -1, REF_PIC_LIST_1, 0, 0, pcCU->getDepth(0));
    pcCU->setMVPNumSubParts( -1, REF_PIC_LIST_1, 0, 0, pcCU->getDepth(0));

    //  Motion compensation
    motionCompensation ( pcCU, rpcPredYuv, REF_PIC_LIST_0 );

  }
  else if ( eSliceType == B_SLICE &&
           pcCU->getSlice()->getNumRefIdx( REF_PIC_LIST_0 ) > 0 &&
           pcCU->getSlice()->getNumRefIdx( REF_PIC_LIST_1 ) > 0  )
  {
    TComMv cMv;
    TComMv cZeroMv;

    if (pcCU->getInterDir(0)!=2)
    {
      xEstimateMvPredAMVP( pcCU, pcOrgYuv, 0, REF_PIC_LIST_0, 0, cMv, (pcCU->getCUMvField( REF_PIC_LIST_0 )->getAMVPInfo()->iN > 0?  true:false) );
      pcCU->getCUMvField( REF_PIC_LIST_0 )->setAllMvField( cMv, 0, SIZE_2Nx2N, 0, 0, 0 );
      pcCU->getCUMvField( REF_PIC_LIST_0 )->setAllMvd    ( cZeroMv, SIZE_2Nx2N, 0, 0, 0 ); //unnecessary
    }

    if (pcCU->getInterDir(0)!=1)
    {
      xEstimateMvPredAMVP( pcCU, pcOrgYuv, 0, REF_PIC_LIST_1, 0, cMv, (pcCU->getCUMvField( REF_PIC_LIST_1 )->getAMVPInfo()->iN > 0?  true:false) );
      pcCU->getCUMvField( REF_PIC_LIST_1 )->setAllMvField( cMv, 0, SIZE_2Nx2N, 0, 0, 0 );
      pcCU->getCUMvField( REF_PIC_LIST_1 )->setAllMvd    ( cZeroMv, SIZE_2Nx2N, 0, 0, 0 );  //unnecessary
    }

    motionCompensation ( pcCU, rpcPredYuv );

  }
  else
  {
    assert( 0 );
  }

  return;
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
Void TEncSearch::encodeResAndCalcRdInterCU( TComDataCU* pcCU, TComYuv* pcYuvOrg, TComYuv* pcYuvPred, TComYuv*& rpcYuvResi, TComYuv*& rpcYuvResiBest, TComYuv*& rpcYuvRec, TComYuv*& rpcYuvResPrd, Bool bSkipRes )
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

  //  No residual coding : SKIP mode
  if ( ePredMode == MODE_SKIP && bSkipRes )
  {
    rpcYuvResi->clear();

    pcYuvPred->copyToPartYuv( rpcYuvRec, 0 );

#if HHI_INTER_VIEW_RESIDUAL_PRED
    // add residual prediction
    if( pcCU->getResPredFlag( 0 ) )
    {
      rpcYuvRec->add( rpcYuvResPrd, uiWidth, uiHeight );
      rpcYuvRec->clip( uiWidth, uiHeight );
    }
#endif

#if HHI_VSO    
    if ( m_pcRdCost->getUseVSO() )
    {
      uiDistortion = m_pcRdCost->getDistVS( pcCU, 0, rpcYuvRec->getLumaAddr(), rpcYuvRec->getStride(),  pcYuvOrg->getLumaAddr(), pcYuvOrg->getStride(),  uiWidth,      uiHeight     , false, 0 );
    }
    else
#endif
    {
      uiDistortion = m_pcRdCost->getDistPart( rpcYuvRec->getLumaAddr(), rpcYuvRec->getStride(),  pcYuvOrg->getLumaAddr(), pcYuvOrg->getStride(),  uiWidth,      uiHeight      )
      + m_pcRdCost->getDistPart( rpcYuvRec->getCbAddr(),   rpcYuvRec->getCStride(), pcYuvOrg->getCbAddr(),   pcYuvOrg->getCStride(), uiWidth >> 1, uiHeight >> 1 )
      + m_pcRdCost->getDistPart( rpcYuvRec->getCrAddr(),   rpcYuvRec->getCStride(), pcYuvOrg->getCrAddr(),   pcYuvOrg->getCStride(), uiWidth >> 1, uiHeight >> 1 );
    }
    

    if( m_bUseSBACRD )
      m_pcRDGoOnSbacCoder->load(m_pppcRDSbacCoder[pcCU->getDepth(0)][CI_CURR_BEST]);

    m_pcEntropyCoder->resetBits();
#if HHI_MPI
    if( pcCU->getTextureModeDepth( 0 ) == -1 )
    {
#endif
    m_pcEntropyCoder->encodeSkipFlag(pcCU, 0, true);
#if HHI_MRG_SKIP
    m_pcEntropyCoder->encodeMergeIndex( pcCU, 0, 0, true );
#else
    if ( pcCU->getSlice()->getNumRefIdx( REF_PIC_LIST_0 ) > 0 ) //if ( ref. frame list0 has at least 1 entry )
    {
      m_pcEntropyCoder->encodeMVPIdx( pcCU, 0, REF_PIC_LIST_0);
    }
    if ( pcCU->getSlice()->getNumRefIdx( REF_PIC_LIST_1 ) > 0 ) //if ( ref. frame list1 has at least 1 entry )
    {
      m_pcEntropyCoder->encodeMVPIdx( pcCU, 0, REF_PIC_LIST_1);
    }
#endif
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
      pcCU->getTotalCost() = m_pcRdCost->calcRdCost( uiBits, uiDistortion );
    }

    if( m_bUseSBACRD )
      m_pcRDGoOnSbacCoder->store(m_pppcRDSbacCoder[pcCU->getDepth(0)][CI_TEMP_BEST]);

    pcCU->setCbfSubParts( 0, 0, 0, 0, pcCU->getDepth( 0 ) );
    pcCU->setTrIdxSubParts( 0, 0, pcCU->getDepth(0) );

#if HHI_VSO
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
  UInt    uiQp, uiQpBest = 0, uiQpMin, uiQpMax;
  Double  dCost, dCostBest = MAX_DOUBLE;

  UInt uiBestTrMode = 0;

  UInt uiTrLevel = 0;
  if( (pcCU->getWidth(0) > pcCU->getSlice()->getSPS()->getMaxTrSize()) )
  {
    while( pcCU->getWidth(0) > (pcCU->getSlice()->getSPS()->getMaxTrSize()<<uiTrLevel) ) uiTrLevel++;
  }
  UInt uiMaxTrMode = pcCU->getSlice()->getSPS()->getMaxTrDepth() + uiTrLevel;

  while((uiWidth>>uiMaxTrMode) < (g_uiMaxCUWidth>>g_uiMaxCUDepth)) uiMaxTrMode--;

  uiQpMin      = bHighPass ? Min( MAX_QP, Max( MIN_QP, pcCU->getQP(0) - m_iMaxDeltaQP ) ) : pcCU->getQP( 0 );
  uiQpMax      = bHighPass ? Min( MAX_QP, Max( MIN_QP, pcCU->getQP(0) + m_iMaxDeltaQP ) ) : pcCU->getQP( 0 );

  pcYuvPred->copyToPartYuv( rpcYuvRec, 0 );

#if HHI_INTER_VIEW_RESIDUAL_PRED
  // add residual prediction
  if( pcCU->getResPredFlag( 0 ) )
  {
    rpcYuvRec->add( rpcYuvResPrd, uiWidth, uiHeight );
  }
#endif
#if HHI_INTERVIEW_SKIP
  if( bSkipRes)
  {
    rpcYuvResi->clear() ;
  }
  else
  {
    rpcYuvResi->subtract( pcYuvOrg, pcYuvPred, 0, uiWidth );
  }
#else
  rpcYuvResi->subtract( pcYuvOrg, pcYuvPred, 0, uiWidth );
#endif

  for ( uiQp = uiQpMin; uiQp <= uiQpMax; uiQp++ )
  {
    pcCU->setQPSubParts( uiQp, 0, pcCU->getDepth(0) );
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

    xEstimateResidualQT( pcCU, 0, 0, pcYuvOrg, rpcYuvRec, rpcYuvResi,  pcCU->getDepth(0), dCost, uiBits, uiDistortion, &uiZeroDistortion );

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
      xSetResidualQTData( pcCU, 0, NULL, pcCU->getDepth(0), false );
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
        xSetResidualQTData( pcCU, 0, rpcYuvResiBest, pcCU->getDepth(0), true );
      }

      if( uiQpMin != uiQpMax && uiQp != uiQpMax )
      {
        const UInt uiQPartNum = pcCU->getPic()->getNumPartInCU() >> (pcCU->getDepth(0) << 1);
        ::memcpy( m_puhQTTempTrIdx, pcCU->getTransformIdx(),        uiQPartNum * sizeof(UChar) );
        ::memcpy( m_puhQTTempCbf[0], pcCU->getCbf( TEXT_LUMA ),     uiQPartNum * sizeof(UChar) );
        ::memcpy( m_puhQTTempCbf[1], pcCU->getCbf( TEXT_CHROMA_U ), uiQPartNum * sizeof(UChar) );
        ::memcpy( m_puhQTTempCbf[2], pcCU->getCbf( TEXT_CHROMA_V ), uiQPartNum * sizeof(UChar) );
        ::memcpy( m_pcQTTempCoeffY,  pcCU->getCoeffY(),  uiWidth * uiHeight * sizeof( TCoeff )      );
        ::memcpy( m_pcQTTempCoeffCb, pcCU->getCoeffCb(), uiWidth * uiHeight * sizeof( TCoeff ) >> 2 );
        ::memcpy( m_pcQTTempCoeffCr, pcCU->getCoeffCr(), uiWidth * uiHeight * sizeof( TCoeff ) >> 2 );
      }
      uiBitsBest       = uiBits;
      uiDistortionBest = uiDistortion;
      dCostBest        = dCost;
      uiQpBest         = uiQp;

      if( m_bUseSBACRD )
      {
        m_pcRDGoOnSbacCoder->store( m_pppcRDSbacCoder[ pcCU->getDepth( 0 ) ][ CI_TEMP_BEST ] );
      }
    }

#if HHI_VSO
    // reset Model
    if( m_pcRdCost->getUseRenModel() )
    {
      Pel*  piSrc       = pcYuvOrg->getLumaAddr();
      UInt  uiSrcStride = pcYuvOrg->getStride();
      m_pcRdCost->setRenModelData( pcCU, 0, piSrc, uiSrcStride, uiWidth, uiHeight );
    }
#endif
  }

  assert ( dCostBest != MAX_DOUBLE );

  if( uiQpMin != uiQpMax && uiQpBest != uiQpMax )
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
  }
  rpcYuvRec->addClip ( rpcYuvRec, rpcYuvResiBest, 0, uiWidth );

  // update with clipped distortion and cost (qp estimation loop uses unclipped values)

#if HHI_VSO
  if ( m_pcRdCost->getUseVSO() )
  {
    uiDistortionBest = m_pcRdCost->getDistVS  ( pcCU, 0, rpcYuvRec->getLumaAddr(), rpcYuvRec->getStride(),  pcYuvOrg->getLumaAddr(), pcYuvOrg->getStride(),  uiWidth,      uiHeight, false, 0    );
  }
  else
#endif
  {
    uiDistortionBest = m_pcRdCost->getDistPart( rpcYuvRec->getLumaAddr(), rpcYuvRec->getStride(),  pcYuvOrg->getLumaAddr(), pcYuvOrg->getStride(),  uiWidth,      uiHeight      )
    + m_pcRdCost->getDistPart( rpcYuvRec->getCbAddr(),   rpcYuvRec->getCStride(), pcYuvOrg->getCbAddr(),   pcYuvOrg->getCStride(), uiWidth >> 1, uiHeight >> 1 )
    + m_pcRdCost->getDistPart( rpcYuvRec->getCrAddr(),   rpcYuvRec->getCStride(), pcYuvOrg->getCrAddr(),   pcYuvOrg->getCStride(), uiWidth >> 1, uiHeight >> 1 );
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
    uiBestTrMode = 0;
    pcCU->setCbfSubParts( 0, 0, 0, 0, pcCU->getDepth( 0 ) );
  }

  pcCU->setQPSubParts( uiQpBest, 0, pcCU->getDepth(0) );

  // set Model
#if HHI_VSO
  if( m_pcRdCost->getUseRenModel() )
  {
    Pel*  piSrc       = rpcYuvRec->getLumaAddr();
    UInt  uiSrcStride = rpcYuvRec->getStride();
    m_pcRdCost->setRenModelData( pcCU, 0, piSrc, uiSrcStride, uiWidth, uiHeight );
  }
#endif
}

Void TEncSearch::xEstimateResidualQT( TComDataCU* pcCU, UInt uiQuadrant, UInt uiAbsPartIdx, TComYuv* pcOrg, TComYuv* pcPred, TComYuv* pcResi, const UInt uiDepth, Double &rdCost, UInt &ruiBits, Dist &ruiDist, Dist *puiZeroDist )
{
  const UInt uiTrMode = uiDepth - pcCU->getDepth( 0 );

  assert( pcCU->getDepth( 0 ) == pcCU->getDepth( uiAbsPartIdx ) );
  const UInt uiLog2TrSize = g_aucConvertToBit[pcCU->getSlice()->getSPS()->getMaxCUWidth() >> uiDepth]+2;

#if HHI_RQT_FORCE_SPLIT_ACC2_PU
#if HHI_RQT_FORCE_SPLIT_NxN
  const Bool bNxNOK = pcCU->getPartitionSize( 0 ) == SIZE_NxN && uiTrMode > 0;
#else
  const Bool bNxNOK = pcCU->getPartitionSize( 0 ) == SIZE_NxN;
#endif
#if HHI_RQT_FORCE_SPLIT_RECT
  const Bool bSymmetricOK  = pcCU->getPartitionSize( 0 ) >= SIZE_2NxN  && pcCU->getPartitionSize( 0 ) < SIZE_NxN   && uiTrMode > 0;
#else
  const Bool bSymmetricOK  = pcCU->getPartitionSize( 0 ) >= SIZE_2NxN  && pcCU->getPartitionSize( 0 ) < SIZE_NxN;
#endif
  const Bool bNoForceSplit = pcCU->getPartitionSize( 0 ) == SIZE_2Nx2N || bNxNOK || bSymmetricOK;
  const Bool bCheckFull    = bNoForceSplit && ( uiLog2TrSize <= pcCU->getSlice()->getSPS()->getQuadtreeTULog2MaxSize() );
#else
  const Bool bCheckFull    = ( uiLog2TrSize <= pcCU->getSlice()->getSPS()->getQuadtreeTULog2MaxSize() );
#endif

  const Bool bCheckSplit  = ( uiLog2TrSize >  pcCU->getQuadtreeTULog2MinSizeInCU(uiAbsPartIdx) );

  assert( bCheckFull || bCheckSplit );

  Bool  bCodeChroma   = true;
  UInt  uiTrModeC     = uiTrMode;
  UInt  uiLog2TrSizeC = uiLog2TrSize-1;
  if( uiLog2TrSize == pcCU->getSlice()->getSPS()->getQuadtreeTULog2MinSize() )
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

    pcCU->setTrIdxSubParts( uiDepth - pcCU->getDepth( 0 ), uiAbsPartIdx, uiDepth );
    if (m_pcEncCfg->getUseRDOQ())
    {
      m_pcEntropyCoder->estimateBit(m_pcTrQuant->m_pcEstBitsSbac, 1<< uiLog2TrSize, TEXT_LUMA );
    }
#if POZNAN_TEXTURE_TU_DELTA_QP_ACCORDING_TO_DEPTH
    m_pcTrQuant->setQPforQuant( pcCU->getQP( 0 ) + pcCU->getQpOffsetForTextCU(uiAbsPartIdx, false), false, pcCU->getSlice()->getSliceType(), TEXT_LUMA );
#else
    m_pcTrQuant->setQPforQuant( pcCU->getQP( 0 ), false, pcCU->getSlice()->getSliceType(), TEXT_LUMA );
#endif
    m_pcTrQuant->transformNxN( pcCU, pcResi->getLumaAddr( uiAbsPartIdx ), pcResi->getStride (), pcCoeffCurrY, 1<< uiLog2TrSize,    1<< uiLog2TrSize,    uiAbsSumY, TEXT_LUMA,     uiAbsPartIdx );

    pcCU->setCbfSubParts( uiAbsSumY ? uiSetCbf : 0, TEXT_LUMA, uiAbsPartIdx, uiDepth );

    if( bCodeChroma )
    {
      if (m_pcEncCfg->getUseRDOQ())
      {
        m_pcEntropyCoder->estimateBit(m_pcTrQuant->m_pcEstBitsSbac, 1<<uiLog2TrSizeC, TEXT_CHROMA );
      }
#if POZNAN_TEXTURE_TU_DELTA_QP_ACCORDING_TO_DEPTH
      m_pcTrQuant->setQPforQuant( pcCU->getQP( 0 ) + pcCU->getQpOffsetForTextCU(uiAbsPartIdx, false), false, pcCU->getSlice()->getSliceType(), TEXT_CHROMA );
#else
      m_pcTrQuant->setQPforQuant( pcCU->getQP( 0 ), false, pcCU->getSlice()->getSliceType(), TEXT_CHROMA );
#endif
      m_pcTrQuant->transformNxN( pcCU, pcResi->getCbAddr( uiAbsPartIdx ), pcResi->getCStride(), pcCoeffCurrU, 1<<uiLog2TrSizeC, 1<<uiLog2TrSizeC, uiAbsSumU, TEXT_CHROMA_U, uiAbsPartIdx );
      m_pcTrQuant->transformNxN( pcCU, pcResi->getCrAddr( uiAbsPartIdx ), pcResi->getCStride(), pcCoeffCurrV, 1<<uiLog2TrSizeC, 1<<uiLog2TrSizeC, uiAbsSumV, TEXT_CHROMA_V, uiAbsPartIdx );
      pcCU->setCbfSubParts( uiAbsSumU ? uiSetCbf : 0, TEXT_CHROMA_U, uiAbsPartIdx, pcCU->getDepth(0)+uiTrModeC );
      pcCU->setCbfSubParts( uiAbsSumV ? uiSetCbf : 0, TEXT_CHROMA_V, uiAbsPartIdx, pcCU->getDepth(0)+uiTrModeC );
    }

    m_pcEntropyCoder->resetBits();

    if (pcCU->getSlice()->getSymbolMode())
      m_pcEntropyCoder->encodeQtCbf( pcCU, uiAbsPartIdx, TEXT_LUMA,     uiTrMode );

    m_pcEntropyCoder->encodeCoeffNxN( pcCU, pcCoeffCurrY, uiAbsPartIdx, 1<< uiLog2TrSize,    1<< uiLog2TrSize,    uiDepth, TEXT_LUMA,     false );
    const UInt uiSingleBitsY = m_pcEntropyCoder->getNumberOfWrittenBits();

    UInt uiSingleBitsU = 0;
    UInt uiSingleBitsV = 0;
    if( bCodeChroma )
    {
      if (pcCU->getSlice()->getSymbolMode())
        m_pcEntropyCoder->encodeQtCbf   ( pcCU, uiAbsPartIdx, TEXT_CHROMA_U, uiTrMode );
      m_pcEntropyCoder->encodeCoeffNxN( pcCU, pcCoeffCurrU, uiAbsPartIdx, 1<<uiLog2TrSizeC, 1<<uiLog2TrSizeC, uiDepth, TEXT_CHROMA_U, false );
      uiSingleBitsU = m_pcEntropyCoder->getNumberOfWrittenBits() - uiSingleBitsY;

      if (pcCU->getSlice()->getSymbolMode())
        m_pcEntropyCoder->encodeQtCbf   ( pcCU, uiAbsPartIdx, TEXT_CHROMA_V, uiTrMode );
      m_pcEntropyCoder->encodeCoeffNxN( pcCU, pcCoeffCurrV, uiAbsPartIdx, 1<<uiLog2TrSizeC, 1<<uiLog2TrSizeC, uiDepth, TEXT_CHROMA_V, false );
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
      uiDistY = m_pcRdCost->getDistPart( pcPred->getLumaAddr( uiAbsPartIdx ), pcPred->getStride(), pcOrg->getLumaAddr( uiAbsPartIdx ), pcOrg->getStride(), 1<< uiLog2TrSize, 1<< uiLog2TrSize);
#else
      uiDistY = m_pcRdCost->getDistPart( m_pTempPel, 1<< uiLog2TrSize, pcResi->getLumaAddr( uiAbsPartIdx ), pcResi->getStride(), 1<< uiLog2TrSize, 1<< uiLog2TrSize ); // initialized with zero residual distortion
#endif
    }


    if ( puiZeroDist )
    {
      *puiZeroDist += uiDistY;
    }
    if( uiAbsSumY )
    {
      Pel *pcResiCurrY = m_pcQTTempTComYuv[uiQTTempAccessLayer].getLumaAddr( uiAbsPartIdx );
#if POZNAN_TEXTURE_TU_DELTA_QP_ACCORDING_TO_DEPTH
      m_pcTrQuant->setQPforQuant( pcCU->getQP( 0 ) + pcCU->getQpOffsetForTextCU(uiAbsPartIdx, false),  false, pcCU->getSlice()->getSliceType(), TEXT_LUMA );
#else
      m_pcTrQuant->setQPforQuant( pcCU->getQP( 0 ), false, pcCU->getSlice()->getSliceType(), TEXT_LUMA );
#endif
#if INTRA_DST_TYPE_7 // Inside Inter Encoder Search. So use conventional DCT.
    m_pcTrQuant->invtransformNxN( TEXT_LUMA,REG_DCT, pcResiCurrY, m_pcQTTempTComYuv[uiQTTempAccessLayer].getStride(),  pcCoeffCurrY, 1<< uiLog2TrSize,    1<< uiLog2TrSize );//this is for inter mode only
#else
    m_pcTrQuant->invtransformNxN( pcResiCurrY, m_pcQTTempTComYuv[uiQTTempAccessLayer].getStride(),  pcCoeffCurrY, 1<< uiLog2TrSize,    1<< uiLog2TrSize );
#endif

      Dist uiNonzeroDistY;

#if HHI_VSO      
      if ( m_pcRdCost->getUseVSO() )
      {
        m_cYuvRecTemp.addClipPartLuma( &m_pcQTTempTComYuv[uiQTTempAccessLayer], pcPred, uiAbsPartIdx, 1<< uiLog2TrSize  );
        uiNonzeroDistY = m_pcRdCost->getDistVS( pcCU, uiAbsPartIdx, m_cYuvRecTemp.getLumaAddr(uiAbsPartIdx), m_cYuvRecTemp.getStride(),
                                                pcOrg->getLumaAddr( uiAbsPartIdx ), pcOrg->getStride(), 1<< uiLog2TrSize,   1<< uiLog2TrSize, false, 0 );
      }
      else
#endif
      {
        uiNonzeroDistY = m_pcRdCost->getDistPart( m_pcQTTempTComYuv[uiQTTempAccessLayer].getLumaAddr( uiAbsPartIdx ), m_pcQTTempTComYuv[uiQTTempAccessLayer].getStride(),
                                                  pcResi->getLumaAddr( uiAbsPartIdx ), pcResi->getStride(), 1<< uiLog2TrSize,    1<< uiLog2TrSize );
      }

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
    }

    if( !uiAbsSumY )
    {
      Pel *pcPtr =  m_pcQTTempTComYuv[uiQTTempAccessLayer].getLumaAddr( uiAbsPartIdx );
      const UInt uiStride = m_pcQTTempTComYuv[uiQTTempAccessLayer].getStride();
      for( UInt uiY = 0; uiY < 1<< uiLog2TrSize; ++uiY )
      {
        ::memset( pcPtr, 0, sizeof(Pel) << uiLog2TrSize );
        pcPtr += uiStride;
      }
    }

    Dist uiDistU = 0;
    Dist uiDistV = 0;
    if( bCodeChroma )
    {
#if IBDI_DISTORTION
      uiDistU = m_pcRdCost->getDistPart( pcPred->getCbAddr( uiAbsPartIdx ), pcPred->getCStride(), pcOrg->getCbAddr( uiAbsPartIdx ), pcOrg->getCStride(), 1<< uiLog2TrSizeC, 1<< uiLog2TrSizeC);
#else
      uiDistU = m_pcRdCost->getDistPart( m_pTempPel, 1<<uiLog2TrSizeC, pcResi->getCbAddr( uiAbsPartIdx ), pcResi->getCStride(), 1<<uiLog2TrSizeC, 1<<uiLog2TrSizeC ); // initialized with zero residual destortion
#endif
      if ( puiZeroDist )
      {
        *puiZeroDist += uiDistU;
      }
      if( uiAbsSumU )
      {
        Pel *pcResiCurrU = m_pcQTTempTComYuv[uiQTTempAccessLayer].getCbAddr( uiAbsPartIdx );
#if POZNAN_TEXTURE_TU_DELTA_QP_ACCORDING_TO_DEPTH
        m_pcTrQuant->setQPforQuant( pcCU->getQP( 0 ) + pcCU->getQpOffsetForTextCU(uiAbsPartIdx, false), false, pcCU->getSlice()->getSliceType(), TEXT_CHROMA );
#else
        m_pcTrQuant->setQPforQuant( pcCU->getQP( 0 ), false, pcCU->getSlice()->getSliceType(), TEXT_CHROMA );
#endif
#if INTRA_DST_TYPE_7  // Inside Inter Encoder Search. So use conventional DCT.
        m_pcTrQuant->invtransformNxN( TEXT_CHROMA,REG_DCT, pcResiCurrU, m_pcQTTempTComYuv[uiQTTempAccessLayer].getCStride(), pcCoeffCurrU, 1<<uiLog2TrSizeC, 1<<uiLog2TrSizeC);
#else
        m_pcTrQuant->invtransformNxN( pcResiCurrU, m_pcQTTempTComYuv[uiQTTempAccessLayer].getCStride(), pcCoeffCurrU, 1<<uiLog2TrSizeC, 1<<uiLog2TrSizeC );
#endif
        const UInt uiNonzeroDistU = m_pcRdCost->getDistPart( m_pcQTTempTComYuv[uiQTTempAccessLayer].getCbAddr( uiAbsPartIdx ), m_pcQTTempTComYuv[uiQTTempAccessLayer].getCStride(),
                                                            pcResi->getCbAddr( uiAbsPartIdx ), pcResi->getCStride(), 1<<uiLog2TrSizeC, 1<<uiLog2TrSizeC );
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
      if( !uiAbsSumU )
      {
        Pel *pcPtr =  m_pcQTTempTComYuv[uiQTTempAccessLayer].getCbAddr( uiAbsPartIdx );
        const UInt uiStride = m_pcQTTempTComYuv[uiQTTempAccessLayer].getCStride();
        for( UInt uiY = 0; uiY < 1<<uiLog2TrSizeC; ++uiY )
        {
          ::memset( pcPtr, 0, sizeof(Pel) << uiLog2TrSizeC );
          pcPtr += uiStride;
        }
      }

#if IBDI_DISTORTION
      uiDistV = m_pcRdCost->getDistPart( pcPred->getCrAddr( uiAbsPartIdx ), pcPred->getCStride(), pcOrg->getCrAddr( uiAbsPartIdx ), pcOrg->getCStride(), 1<< uiLog2TrSizeC, 1<< uiLog2TrSizeC);
#else
      uiDistV = m_pcRdCost->getDistPart( m_pTempPel, 1<<uiLog2TrSizeC, pcResi->getCrAddr( uiAbsPartIdx ), pcResi->getCStride(), 1<<uiLog2TrSizeC, 1<<uiLog2TrSizeC ); // initialized with zero residual destortion
#endif
      if ( puiZeroDist )
      {
        *puiZeroDist += uiDistV;
      }
      if( uiAbsSumV )
      {
        Pel *pcResiCurrV = m_pcQTTempTComYuv[uiQTTempAccessLayer].getCrAddr  ( uiAbsPartIdx );
        if( !uiAbsSumU )
        {
#if POZNAN_TEXTURE_TU_DELTA_QP_ACCORDING_TO_DEPTH
          m_pcTrQuant->setQPforQuant( pcCU->getQP( 0 ) + pcCU->getQpOffsetForTextCU(uiAbsPartIdx, false), false, pcCU->getSlice()->getSliceType(), TEXT_CHROMA );
#else
          m_pcTrQuant->setQPforQuant( pcCU->getQP( 0 ), false, pcCU->getSlice()->getSliceType(), TEXT_CHROMA );
#endif
        }
#if INTRA_DST_TYPE_7   // Inside Inter Encoder Search. So use conventional DCT.
        m_pcTrQuant->invtransformNxN( TEXT_CHROMA,REG_DCT, pcResiCurrV, m_pcQTTempTComYuv[uiQTTempAccessLayer].getCStride(), pcCoeffCurrV, 1<<uiLog2TrSizeC, 1<<uiLog2TrSizeC );
#else
        m_pcTrQuant->invtransformNxN( pcResiCurrV, m_pcQTTempTComYuv[uiQTTempAccessLayer].getCStride(), pcCoeffCurrV, 1<<uiLog2TrSizeC, 1<<uiLog2TrSizeC );
#endif
        const UInt uiNonzeroDistV = m_pcRdCost->getDistPart( m_pcQTTempTComYuv[uiQTTempAccessLayer].getCrAddr( uiAbsPartIdx ), m_pcQTTempTComYuv[uiQTTempAccessLayer].getCStride(),
                                                            pcResi->getCrAddr( uiAbsPartIdx ), pcResi->getCStride(), 1<<uiLog2TrSizeC, 1<<uiLog2TrSizeC );
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
      if( !uiAbsSumV )
      {
        Pel *pcPtr =  m_pcQTTempTComYuv[uiQTTempAccessLayer].getCrAddr( uiAbsPartIdx );
        const UInt uiStride = m_pcQTTempTComYuv[uiQTTempAccessLayer].getCStride();
        for( UInt uiY = 0; uiY < 1<<uiLog2TrSizeC; ++uiY )
        {
          ::memset( pcPtr, 0, sizeof(Pel) << uiLog2TrSizeC );
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

#if CAVLC_RQT_CBP
    if (pcCU->getSlice()->getSymbolMode())
    {
#endif
    if( uiLog2TrSize > pcCU->getQuadtreeTULog2MinSizeInCU(uiAbsPartIdx) )
    {
      m_pcEntropyCoder->encodeTransformSubdivFlag( 0, uiDepth );
    }
#if CAVLC_RQT_CBP
    }
#endif

    if (pcCU->getSlice()->getSymbolMode())
    {
      if( bCodeChroma )
      {
        m_pcEntropyCoder->encodeQtCbf( pcCU, uiAbsPartIdx, TEXT_CHROMA_U, uiTrMode );
        m_pcEntropyCoder->encodeQtCbf( pcCU, uiAbsPartIdx, TEXT_CHROMA_V, uiTrMode );
      }

      m_pcEntropyCoder->encodeQtCbf( pcCU, uiAbsPartIdx, TEXT_LUMA,     uiTrMode );
    }

    m_pcEntropyCoder->encodeCoeffNxN( pcCU, pcCoeffCurrY, uiAbsPartIdx, 1<< uiLog2TrSize,    1<< uiLog2TrSize,    uiDepth, TEXT_LUMA,     false );

    if( bCodeChroma )
    {
      m_pcEntropyCoder->encodeCoeffNxN( pcCU, pcCoeffCurrU, uiAbsPartIdx, 1<<uiLog2TrSizeC, 1<<uiLog2TrSizeC, uiDepth, TEXT_CHROMA_U, false );
      m_pcEntropyCoder->encodeCoeffNxN( pcCU, pcCoeffCurrV, uiAbsPartIdx, 1<<uiLog2TrSizeC, 1<<uiLog2TrSizeC, uiDepth, TEXT_CHROMA_V, false );
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
  }

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
      xEstimateResidualQT( pcCU, ui, uiAbsPartIdx + ui * uiQPartNumSubdiv, pcOrg, pcPred, pcResi, uiDepth + 1, dSubdivCost, uiSubdivBits, uiSubdivDist, bCheckFull ? NULL : puiZeroDist );
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
  if ( m_pcRdCost->getUseRenModel() )
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

#if CAVLC_RQT_CBP
  if(pcCU->getSlice()->getSymbolMode() == 0 )
  {
    if( bSubdivAndCbf && uiCurrTrMode != 0)
      m_pcEntropyCoder->m_pcEntropyCoderIf->codeCbfTrdiv( pcCU, uiAbsPartIdx, uiDepth );
  }
#endif

#if CAVLC_RQT_CBP
  if(pcCU->getSlice()->getSymbolMode())
  {
#endif
  if( bSubdivAndCbf && uiLog2TrSize <= pcCU->getSlice()->getSPS()->getQuadtreeTULog2MaxSize() && uiLog2TrSize > pcCU->getQuadtreeTULog2MinSizeInCU(uiAbsPartIdx) )
  {
    m_pcEntropyCoder->encodeTransformSubdivFlag( bSubdiv, uiDepth );
  }
#if CAVLC_RQT_CBP
  }
#endif

  if (pcCU->getSlice()->getSymbolMode())
  {
    assert( pcCU->getPredictionMode(uiAbsPartIdx) != MODE_INTRA );
    if( bSubdivAndCbf && uiLog2TrSize <= pcCU->getSlice()->getSPS()->getQuadtreeTULog2MaxSize() )
    {
      const Bool bFirstCbfOfCU = uiLog2TrSize == pcCU->getSlice()->getSPS()->getQuadtreeTULog2MaxSize() || uiCurrTrMode == 0;
      if( bFirstCbfOfCU || uiLog2TrSize > pcCU->getSlice()->getSPS()->getQuadtreeTULog2MinSize() )
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
      else if( uiLog2TrSize == pcCU->getSlice()->getSPS()->getQuadtreeTULog2MinSize() )
      {
        assert( pcCU->getCbf( uiAbsPartIdx, TEXT_CHROMA_U, uiCurrTrMode ) == pcCU->getCbf( uiAbsPartIdx, TEXT_CHROMA_U, uiCurrTrMode - 1 ) );
        assert( pcCU->getCbf( uiAbsPartIdx, TEXT_CHROMA_V, uiCurrTrMode ) == pcCU->getCbf( uiAbsPartIdx, TEXT_CHROMA_V, uiCurrTrMode - 1 ) );
      }
    }
  }

  if( !bSubdiv )
  {
    const UInt uiNumCoeffPerAbsPartIdxIncrement = pcCU->getSlice()->getSPS()->getMaxCUWidth() * pcCU->getSlice()->getSPS()->getMaxCUHeight() >> ( pcCU->getSlice()->getSPS()->getMaxCUDepth() << 1 );
    assert( 16 == uiNumCoeffPerAbsPartIdxIncrement ); // check
    const UInt uiQTTempAccessLayer = pcCU->getSlice()->getSPS()->getQuadtreeTULog2MaxSize() - uiLog2TrSize;
    TCoeff *pcCoeffCurrY = m_ppcQTTempCoeffY [uiQTTempAccessLayer] +  uiNumCoeffPerAbsPartIdxIncrement * uiAbsPartIdx;
    TCoeff *pcCoeffCurrU = m_ppcQTTempCoeffCb[uiQTTempAccessLayer] + (uiNumCoeffPerAbsPartIdxIncrement * uiAbsPartIdx>>2);
    TCoeff *pcCoeffCurrV = m_ppcQTTempCoeffCr[uiQTTempAccessLayer] + (uiNumCoeffPerAbsPartIdxIncrement * uiAbsPartIdx>>2);

    Bool  bCodeChroma   = true;
    UInt  uiTrModeC     = uiTrMode;
    UInt  uiLog2TrSizeC = uiLog2TrSize-1;
    if( uiLog2TrSize == pcCU->getSlice()->getSPS()->getQuadtreeTULog2MinSize() )
    {
      uiLog2TrSizeC++;
      uiTrModeC    --;
      UInt  uiQPDiv = pcCU->getPic()->getNumPartInCU() >> ( ( pcCU->getDepth( 0 ) + uiTrModeC ) << 1 );
      bCodeChroma   = ( ( uiAbsPartIdx % uiQPDiv ) == 0 );
    }

    if( bSubdivAndCbf )
    {
      if (pcCU->getSlice()->getSymbolMode())
      {
        m_pcEntropyCoder->encodeQtCbf( pcCU, uiAbsPartIdx, TEXT_LUMA,     uiTrMode );
      }
    }
    else
    {
      if( eType == TEXT_LUMA     && pcCU->getCbf( uiAbsPartIdx, TEXT_LUMA,     uiTrMode ) )
      {
        m_pcEntropyCoder->encodeCoeffNxN( pcCU, pcCoeffCurrY, uiAbsPartIdx, 1<< uiLog2TrSize,    1<< uiLog2TrSize,    uiDepth, TEXT_LUMA,     false );
      }
      if( bCodeChroma )
      {
        if( eType == TEXT_CHROMA_U && pcCU->getCbf( uiAbsPartIdx, TEXT_CHROMA_U, uiTrMode ) )
        {
          m_pcEntropyCoder->encodeCoeffNxN( pcCU, pcCoeffCurrU, uiAbsPartIdx, 1<<uiLog2TrSizeC, 1<<uiLog2TrSizeC, uiDepth, TEXT_CHROMA_U, false );
        }
        if( eType == TEXT_CHROMA_V && pcCU->getCbf( uiAbsPartIdx, TEXT_CHROMA_V, uiTrMode ) )
        {
          m_pcEntropyCoder->encodeCoeffNxN( pcCU, pcCoeffCurrV, uiAbsPartIdx, 1<<uiLog2TrSizeC, 1<<uiLog2TrSizeC, uiDepth, TEXT_CHROMA_V, false );
        }
      }
    }
  }
  else
  {
    if( bSubdivAndCbf || pcCU->getCbf( uiAbsPartIdx, eType, uiCurrTrMode ) )
    {
      const UInt uiQPartNumSubdiv = pcCU->getPic()->getNumPartInCU() >> ((uiDepth + 1 ) << 1);
#if !CAVLC_RQT_CBP
      if(pcCU->getSlice()->getSymbolMode() == 0)
      {
        if( !bSubdivAndCbf && (eType == TEXT_LUMA || uiLog2TrSize-1 > pcCU->getSlice()->getSPS()->getQuadtreeTULog2MinSize()) )
          m_pcEntropyCoder->m_pcEntropyCoderIf->codeBlockCbf(pcCU, uiAbsPartIdx, eType, uiCurrTrMode + 1, uiQPartNumSubdiv, true);
      }
#endif
      for( UInt ui = 0; ui < 4; ++ui )
      {
        xEncodeResidualQT( pcCU, uiAbsPartIdx + ui * uiQPartNumSubdiv, uiDepth + 1, bSubdivAndCbf, eType );
      }
    }
  }
}

Void TEncSearch::xSetResidualQTData( TComDataCU* pcCU, UInt uiAbsPartIdx, TComYuv* pcResi, UInt uiDepth, Bool bSpatial )
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
    if( uiLog2TrSize == pcCU->getSlice()->getSPS()->getQuadtreeTULog2MinSize() )
    {
      uiLog2TrSizeC++;
      uiTrModeC    --;
      UInt  uiQPDiv = pcCU->getPic()->getNumPartInCU() >> ( ( pcCU->getDepth( 0 ) + uiTrModeC ) << 1 );
      bCodeChroma   = ( ( uiAbsPartIdx % uiQPDiv ) == 0 );
    }

    if( bSpatial )
    {
      m_pcQTTempTComYuv[uiQTTempAccessLayer].copyPartToPartLuma    ( pcResi, uiAbsPartIdx, 1<<uiLog2TrSize , 1<<uiLog2TrSize  );
      if( bCodeChroma )
      {
        m_pcQTTempTComYuv[uiQTTempAccessLayer].copyPartToPartChroma( pcResi, uiAbsPartIdx, 1<<uiLog2TrSizeC, 1<<uiLog2TrSizeC );
      }
    }
    else
    {
      UInt    uiNumCoeffPerAbsPartIdxIncrement = pcCU->getSlice()->getSPS()->getMaxCUWidth() * pcCU->getSlice()->getSPS()->getMaxCUHeight() >> ( pcCU->getSlice()->getSPS()->getMaxCUDepth() << 1 );
      UInt    uiNumCoeffY = ( 1 << ( uiLog2TrSize << 1 ) );
      TCoeff* pcCoeffSrcY = m_ppcQTTempCoeffY [uiQTTempAccessLayer] +  uiNumCoeffPerAbsPartIdxIncrement * uiAbsPartIdx;
      TCoeff* pcCoeffDstY = pcCU->getCoeffY() + uiNumCoeffPerAbsPartIdxIncrement * uiAbsPartIdx;
      ::memcpy( pcCoeffDstY, pcCoeffSrcY, sizeof( TCoeff ) * uiNumCoeffY );
      if( bCodeChroma )
      {
        UInt    uiNumCoeffC = ( 1 << ( uiLog2TrSizeC << 1 ) );
        TCoeff* pcCoeffSrcU = m_ppcQTTempCoeffCb[uiQTTempAccessLayer] + (uiNumCoeffPerAbsPartIdxIncrement * uiAbsPartIdx>>2);
        TCoeff* pcCoeffSrcV = m_ppcQTTempCoeffCr[uiQTTempAccessLayer] + (uiNumCoeffPerAbsPartIdxIncrement * uiAbsPartIdx>>2);
        TCoeff* pcCoeffDstU = pcCU->getCoeffCb() + (uiNumCoeffPerAbsPartIdxIncrement * uiAbsPartIdx>>2);
        TCoeff* pcCoeffDstV = pcCU->getCoeffCr() + (uiNumCoeffPerAbsPartIdxIncrement * uiAbsPartIdx>>2);
        ::memcpy( pcCoeffDstU, pcCoeffSrcU, sizeof( TCoeff ) * uiNumCoeffC );
        ::memcpy( pcCoeffDstV, pcCoeffSrcV, sizeof( TCoeff ) * uiNumCoeffC );
      }
    }
  }
  else
  {
    const UInt uiQPartNumSubdiv = pcCU->getPic()->getNumPartInCU() >> ((uiDepth + 1 ) << 1);
    for( UInt ui = 0; ui < 4; ++ui )
    {
      xSetResidualQTData( pcCU, uiAbsPartIdx + ui * uiQPartNumSubdiv, pcResi, uiDepth + 1, bSpatial );
    }
  }
}

UInt TEncSearch::xModeBitsIntra( TComDataCU* pcCU, UInt uiMode, UInt uiPU, UInt uiPartOffset, UInt uiDepth, UInt uiInitTrDepth )
{
  if( m_bUseSBACRD )
  {
    m_pcRDGoOnSbacCoder->load( m_pppcRDSbacCoder[uiDepth][CI_CURR_BEST] );
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
    ruiBits = m_pcEntropyCoder->getNumberOfWrittenBits();

    m_pcEntropyCoder->resetBits();
#if HHI_MRG_SKIP
    m_pcEntropyCoder->encodeMergeIndex(pcCU, 0, 0, true);
#else
    if ( pcCU->getSlice()->getNumRefIdx( REF_PIC_LIST_0 ) > 0 ) //if ( ref. frame list0 has at least 1 entry )
    {
      m_pcEntropyCoder->encodeMVPIdx( pcCU, 0, REF_PIC_LIST_0);
    }
    if ( pcCU->getSlice()->getNumRefIdx( REF_PIC_LIST_1 ) > 0 ) //if ( ref. frame list1 has at least 1 entry )
    {
      m_pcEntropyCoder->encodeMVPIdx( pcCU, 0, REF_PIC_LIST_1);
    }
#endif
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
#if HHI_MRG_SKIP
      if (pcCU->getPredictionMode(0) == MODE_SKIP)
      {
        pcCU->setPredModeSubParts( MODE_INTER, 0, pcCU->getDepth(0) );
      }
#endif
      m_pcEntropyCoder->encodePredMode( pcCU, 0, true );
      m_pcEntropyCoder->encodePartSize( pcCU, 0, pcCU->getDepth(0), true );
      m_pcEntropyCoder->encodePredInfo( pcCU, 0, true );

#if HHI_INTER_VIEW_RESIDUAL_PRED
      m_pcEntropyCoder->encodeResPredFlag( pcCU, 0, 0, true );
#endif
#if HHI_MPI
    }
#endif
    m_pcEntropyCoder->encodeCoeff   ( pcCU, 0, pcCU->getDepth(0), pcCU->getWidth(0), pcCU->getHeight(0) );

    ruiBits += m_pcEntropyCoder->getNumberOfWrittenBits();
  }
}


Void TEncSearch::xExtDIFUpSamplingH ( TComPattern* pcPattern, TComYuv* pcYuvExt  )
{
  Int   x, y;

  Int   iWidth      = pcPattern->getROIYWidth();
  Int   iHeight     = pcPattern->getROIYHeight();

  Int   iPatStride  = pcPattern->getPatternLStride();
  Int   iExtStride  = pcYuvExt ->getStride();

  Int*  piSrcY;
  Int*  piDstY;
  Pel*  piDstYPel;
  Pel*  piSrcYPel;

  //  Copy integer-pel
  piSrcYPel = pcPattern->getROIY() - 4 - iPatStride;
  piDstY    = m_piYuvExt;//pcYuvExt->getLumaAddr();
  piDstYPel = pcYuvExt->getLumaAddr();
  for ( y = 0; y < iHeight + 2; y++ )
  {
    for ( x = 0; x < iWidth + 8; x++ )
    {
      piDstYPel[x << 2] = piSrcYPel[x];
    }
    piSrcYPel +=  iPatStride;
    piDstY    += (m_iYuvExtStride << 2);
    piDstYPel += (iExtStride      << 2);
  }

  //  Half-pel NORM. : vertical
  piSrcYPel = pcPattern->getROIY()    - iPatStride - 4;
  piDstY    = m_piYuvExt              + (m_iYuvExtStride<<1);
  piDstYPel = pcYuvExt->getLumaAddr() + (iExtStride<<1);
  xCTI_FilterHalfVer     (piSrcYPel, iPatStride,     1, iWidth + 8, iHeight + 1, m_iYuvExtStride<<2, 4, piDstY, iExtStride<<2, piDstYPel);

  //  Half-pel interpolation : horizontal
  piSrcYPel = pcPattern->getROIY()   -  iPatStride - 1;
  piDstYPel = pcYuvExt->getLumaAddr() + 14;
  xCTI_FilterHalfHor (piSrcYPel, iPatStride,     1,  iWidth + 1, iHeight + 1,  iExtStride<<2, 4, piDstYPel);

  //  Half-pel interpolation : center
  piSrcY    = m_piYuvExt              + (m_iYuvExtStride<<1) + (3 << 2);
  piDstYPel = pcYuvExt->getLumaAddr() + (iExtStride<<1)      + 14;
  xCTI_FilterHalfHor       (piSrcY, m_iYuvExtStride<<2, 4, iWidth + 1, iHeight + 1,iExtStride<<2, 4, piDstYPel);

}

Void TEncSearch::xExtDIFUpSamplingQ   ( TComPattern* pcPatternKey, Pel* piDst, Int iDstStride, Pel* piSrcPel, Int iSrcPelStride, Int* piSrc, Int iSrcStride, UInt uiFilter )
{
  Int   x, y;

  Int   iWidth      = pcPatternKey->getROIYWidth();
  Int   iHeight     = pcPatternKey->getROIYHeight();

  Int*  piSrcY;
  Int*  piDstY;
  Pel*  piDstYPel;
  Pel*  piSrcYPel;

  Int iSrcStride4 = (iSrcStride<<2);
  Int iDstStride4 = (iDstStride<<2);

  switch (uiFilter)
  {
    case 0:
    {
      //  Quater-pel interpolation : vertical
      piSrcYPel = piSrcPel - 3;
      piDstY    = piSrc - 14 - iSrcStride;
      xCTI_FilterQuarter0Ver(piSrcYPel, iSrcPelStride, 1, iWidth + 7, iHeight, iSrcStride4, 4, piDstY);

      piSrcYPel = piSrcPel - 3;
      piDstY    = piSrc - 14 + iSrcStride;
      xCTI_FilterQuarter1Ver(piSrcYPel, iSrcPelStride, 1, iWidth + 7, iHeight, iSrcStride4, 4, piDstY);
      // Above three pixels
      piSrcY    = piSrc-2 - iSrcStride;
      piDstYPel = piDst-1 - iDstStride;
      xCTI_FilterQuarter0Hor(piSrcY, iSrcStride4, 4, iWidth, iHeight, iDstStride4, 4, piDstYPel);

      piSrcY    = piSrc-2 - iSrcStride;
      piDstYPel = piDst   - iDstStride;;
      xCTI_FilterHalfHor(piSrcY, iSrcStride4, 4, iWidth, iHeight, iDstStride4, 4, piDstYPel);

      piSrcY    = piSrc-2 - iSrcStride;
      piDstYPel = piDst+1 - iDstStride;
      xCTI_FilterQuarter1Hor(piSrcY, iSrcStride4, 4, iWidth, iHeight, iDstStride4, 4, piDstYPel);

      // Middle two pixels
      piSrcY    = piSrc-2;
      piDstYPel = piDst-1;
      xCTI_FilterQuarter0Hor(piSrcY, iSrcStride4, 4, iWidth, iHeight, iDstStride4, 4, piDstYPel);

      piSrcY    = piSrc-2;
      piDstYPel = piDst+1;
      xCTI_FilterQuarter1Hor(piSrcY, iSrcStride4, 4, iWidth, iHeight, iDstStride4, 4, piDstYPel);

      // Below three pixels
      piSrcY    = piSrc-2 + iSrcStride;
      piDstYPel = piDst-1 + iDstStride;
      xCTI_FilterQuarter0Hor(piSrcY, iSrcStride4, 4, iWidth, iHeight, iDstStride4, 4, piDstYPel);

      piSrcY    = piSrc-2 + iSrcStride;
      piDstYPel = piDst   + iDstStride;;
      xCTI_FilterHalfHor(piSrcY, iSrcStride4, 4, iWidth, iHeight, iDstStride4, 4, piDstYPel);

      piSrcY    = piSrc-2 + iSrcStride;
      piDstYPel = piDst+1 + iDstStride;
      xCTI_FilterQuarter1Hor(piSrcY, iSrcStride4, 4, iWidth, iHeight, iDstStride4, 4, piDstYPel);
      break;
    }
    case 1:
    {
      //  Quater-pel interpolation : vertical
      piSrcYPel = piSrcPel - 4;
      piDstY    = piSrc-16 - iSrcStride;
      xCTI_FilterQuarter0Ver(piSrcYPel, iSrcPelStride, 1, iWidth + 8, iHeight, iSrcStride4, 4, piDstY);

      piSrcYPel = piSrcPel - 4;
      piDstY    = piSrc-16 + iSrcStride;
      xCTI_FilterQuarter1Ver(piSrcYPel, iSrcPelStride, 1, iWidth + 8, iHeight, iSrcStride4, 4, piDstY);
      // Left three pixels
      piSrcY    = piSrc-4 - iSrcStride;
      piDstYPel = piDst-1 - iDstStride;
      xCTI_FilterQuarter1Hor(piSrcY, iSrcStride4, 4, iWidth, iHeight, iDstStride4, 4, piDstYPel);

      piSrcY    = piSrc-4;
      piDstYPel = piDst-1;
      xCTI_FilterQuarter1Hor(piSrcY, iSrcStride4, 4, iWidth, iHeight, iDstStride4, 4, piDstYPel);

      piSrcY    = piSrc-4 + iSrcStride;
      piDstYPel = piDst-1 + iDstStride;
      xCTI_FilterQuarter1Hor(piSrcY, iSrcStride4, 4, iWidth, iHeight, iDstStride4, 4, piDstYPel);

      // Middle two pixels
      piSrcY    = piSrc - iSrcStride;
      piDstYPel = piDst - iDstStride;
      Int iSrcStride2 = (iSrcStride<<1);
      Int iDstStride2 = (iDstStride<<1);

      for (y=0; y < iHeight*2; y++)
      {
        for (x=0; x < iWidth; x++)
        {
          piDstYPel[x*4] = Clip( (piSrcY[x*4] +  32) >>  6 );
        }
        piSrcY+=iSrcStride2;
        piDstYPel+=iDstStride2;
      }

      // Right three pixels
      piSrcY    = piSrc   - iSrcStride;
      piDstYPel = piDst+1 - iDstStride;
      xCTI_FilterQuarter0Hor(piSrcY, iSrcStride4, 4, iWidth, iHeight, iDstStride4, 4, piDstYPel);

      piSrcY    = piSrc;
      piDstYPel = piDst+1;
      xCTI_FilterQuarter0Hor(piSrcY, iSrcStride4, 4, iWidth, iHeight, iDstStride4, 4, piDstYPel);

      piSrcY    = piSrc   + iSrcStride;
      piDstYPel = piDst+1 + iDstStride;
      xCTI_FilterQuarter0Hor(piSrcY, iSrcStride4, 4, iWidth, iHeight, iDstStride4, 4, piDstYPel);

      break;
    }
    case 2:
    {
      //  Quater-pel interpolation : vertical
      piSrcYPel = piSrcPel - 3 - iSrcPelStride;;
      piDstY    = piSrc - 14 - iSrcStride;
      xCTI_FilterQuarter1Ver(piSrcYPel, iSrcPelStride, 1, iWidth + 7, iHeight, iSrcStride4, 4, piDstY);

      piSrcYPel = piSrcPel - 3;
      piDstY    = piSrc - 14 + iSrcStride;
      xCTI_FilterQuarter0Ver(piSrcYPel, iSrcPelStride, 1, iWidth + 7, iHeight, iSrcStride4, 4, piDstY);
      // Above three pixels
      piSrcY    = piSrc-2 - iSrcStride;
      piDstYPel = piDst-1 - iDstStride;
      xCTI_FilterQuarter0Hor(piSrcY, iSrcStride4, 4, iWidth, iHeight, iDstStride4, 4, piDstYPel);

      piSrcY    = piSrc-2 - iSrcStride;
      piDstYPel = piDst   - iDstStride;;
      xCTI_FilterHalfHor(piSrcY, iSrcStride4, 4, iWidth, iHeight, iDstStride4, 4, piDstYPel);

      piSrcY    = piSrc-2 - iSrcStride;
      piDstYPel = piDst+1 - iDstStride;
      xCTI_FilterQuarter1Hor(piSrcY, iSrcStride4, 4, iWidth, iHeight, iDstStride4, 4, piDstYPel);

      // Middle two pixels
      piDstYPel = piDst - 1;
      xCTI_FilterQuarter0Hor(piSrcPel, iSrcPelStride, 1, iWidth, iHeight, iDstStride4, 4, piDstYPel);

      piDstYPel = piDst + 1;
      xCTI_FilterQuarter1Hor(piSrcPel, iSrcPelStride, 1, iWidth, iHeight, iDstStride4, 4, piDstYPel);

      // Below three pixels
      piSrcY    = piSrc-2 + iSrcStride;
      piDstYPel = piDst-1 + iDstStride;
      xCTI_FilterQuarter0Hor(piSrcY, iSrcStride4, 4, iWidth, iHeight, iDstStride4, 4, piDstYPel);

      piSrcY    = piSrc-2 + iSrcStride;
      piDstYPel = piDst   + iDstStride;;
      xCTI_FilterHalfHor(piSrcY, iSrcStride4, 4, iWidth, iHeight, iDstStride4, 4, piDstYPel);

      piSrcY    = piSrc-2 + iSrcStride;
      piDstYPel = piDst+1 + iDstStride;
      xCTI_FilterQuarter1Hor(piSrcY, iSrcStride4, 4, iWidth, iHeight, iDstStride4, 4, piDstYPel);
      break;
    }
    case 3:
    {
      //  Quater-pel interpolation : vertical
      piSrcYPel = piSrcPel-4 - iSrcPelStride;
      piDstY    = piSrc-16 - iSrcStride;
      xCTI_FilterQuarter1Ver(piSrcYPel, iSrcPelStride, 1, iWidth + 8, iHeight, iSrcStride4, 4, piDstY);

      piSrcYPel = piSrcPel-4;
      piDstY    = piSrc-16 + iSrcStride;
      xCTI_FilterQuarter0Ver(piSrcYPel, iSrcPelStride, 1, iWidth + 8, iHeight, iSrcStride4, 4, piDstY);
      // Left three pixels
      piSrcY    = piSrc-4 - iSrcStride;
      piDstYPel = piDst-1 - iDstStride;
      xCTI_FilterQuarter1Hor(piSrcY, iSrcStride4, 4, iWidth, iHeight, iDstStride4, 4, piDstYPel);

      piSrcYPel = piSrcPel-1;
      piDstYPel = piDst-1;
      xCTI_FilterQuarter1Hor(piSrcYPel, iSrcPelStride, 1, iWidth, iHeight, iDstStride4, 4, piDstYPel);

      piSrcY    = piSrc-4 + iSrcStride;
      piDstYPel = piDst-1 + iDstStride;
      xCTI_FilterQuarter1Hor(piSrcY, iSrcStride4, 4, iWidth, iHeight, iDstStride4, 4, piDstYPel);

      // Middle two pixels
      piSrcY    = piSrc - iSrcStride;
      piDstYPel = piDst - iDstStride;
      Int iSrcStride2 = (iSrcStride<<1);
      Int iDstStride2 = (iDstStride<<1);

      for (y=0; y < iHeight*2; y++)
      {
        for (x=0; x < iWidth; x++)
        {
          piDstYPel[x*4] = Clip( (piSrcY[x*4] + 32) >>  6 );
        }
        piSrcY+=iSrcStride2;
        piDstYPel+=iDstStride2;
      }

      // Right three pixels
      piSrcY    = piSrc   - iSrcStride;
      piDstYPel = piDst+1 - iDstStride;
      xCTI_FilterQuarter0Hor(piSrcY, iSrcStride4, 4, iWidth, iHeight, iDstStride4, 4, piDstYPel);

      piDstYPel = piDst+1;
      xCTI_FilterQuarter0Hor(piSrcPel, iSrcPelStride, 1, iWidth, iHeight, iDstStride4, 4, piDstYPel);

      piSrcY    = piSrc   + iSrcStride;
      piDstYPel = piDst+1 + iDstStride;
      xCTI_FilterQuarter0Hor(piSrcY, iSrcStride4, 4, iWidth, iHeight, iDstStride4, 4, piDstYPel);

      break;
    }
    default:
    {
      assert(0);
    }
  }
}
#ifdef WEIGHT_PRED
Void  TEncSearch::setWpScalingDistParam( TComDataCU* pcCU, Int iRefIdx0, Int iRefIdx1 , RefPicList eRefPicListCur )
{
  if ( iRefIdx0<0 && iRefIdx1<0 )
  {
    m_cDistParam.applyWeight = false;
    return;
  }

  TComSlice       *pcSlice  = pcCU->getSlice();
  TComPPS         *pps      = pcCU->getSlice()->getPPS();
  wpScalingParam  *wp0 , *wp1;

  m_cDistParam.applyWeight = ( pcSlice->getSliceType()==P_SLICE && pps->getUseWP() ) || ( pcSlice->getSliceType()==B_SLICE && pps->getWPBiPredIdc() ) ;

  if ( !m_cDistParam.applyWeight ) return;

  getWpScaling( pcCU, iRefIdx0, iRefIdx1, wp0 , wp1 );

  if ( iRefIdx0 < 0 ) wp0 = NULL;
  if ( iRefIdx1 < 0 ) wp1 = NULL;

  m_cDistParam.wpCur  = NULL;
  m_cDistParam.wpRef  = NULL;

  if ( eRefPicListCur == REF_PIC_LIST_0 )
  {
    m_cDistParam.wpCur = wp0;
    m_cDistParam.wpRef = wp1;
  }
  else
  {
    m_cDistParam.wpCur = wp1;
    m_cDistParam.wpRef = wp0;
  }
}
#endif

