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

/** \file     TComWeightPrediction.h
    \brief    weighting prediction class (header)
*/

// Include files
#include "TComSlice.h"
#include "TComWeightPrediction.h"
#include "TComInterpolationFilter.h"

// ====================================================================================================================
// Class definition
// ====================================================================================================================
TComWeightPrediction::TComWeightPrediction()
{
}

/** weighted averaging for bi-pred
 * \param TComYuv* pcYuvSrc0
 * \param TComYuv* pcYuvSrc1
 * \param iPartUnitIdx
 * \param iWidth
 * \param iHeight
 * \param wpScalingParam *wp0
 * \param wpScalingParam *wp1
 * \param TComYuv* rpcYuvDst
 * \returns Void
 */
Void TComWeightPrediction::addWeightBi( TComYuv* pcYuvSrc0, TComYuv* pcYuvSrc1, UInt iPartUnitIdx, UInt iWidth, UInt iHeight, wpScalingParam *wp0, wpScalingParam *wp1, TComYuv* rpcYuvDst, Bool bRound )
{
  Int x, y;

  Pel* pSrcY0  = pcYuvSrc0->getLumaAddr( iPartUnitIdx );
  Pel* pSrcU0  = pcYuvSrc0->getCbAddr  ( iPartUnitIdx );
  Pel* pSrcV0  = pcYuvSrc0->getCrAddr  ( iPartUnitIdx );
  
  Pel* pSrcY1  = pcYuvSrc1->getLumaAddr( iPartUnitIdx );
  Pel* pSrcU1  = pcYuvSrc1->getCbAddr  ( iPartUnitIdx );
  Pel* pSrcV1  = pcYuvSrc1->getCrAddr  ( iPartUnitIdx );
  
  Pel* pDstY   = rpcYuvDst->getLumaAddr( iPartUnitIdx );
  Pel* pDstU   = rpcYuvDst->getCbAddr  ( iPartUnitIdx );
  Pel* pDstV   = rpcYuvDst->getCrAddr  ( iPartUnitIdx );
  
  // Luma : --------------------------------------------
  Int w0      = wp0[0].w;
  Int offset  = wp0[0].offset;
  Int shiftNum = IF_INTERNAL_PREC - ( g_uiBitDepth + g_uiBitIncrement );
  Int shift   = wp0[0].shift + shiftNum;
  Int round   = (1<<(shift-1)) * bRound;
  Int w1      = wp1[0].w;

  UInt  iSrc0Stride = pcYuvSrc0->getStride();
  UInt  iSrc1Stride = pcYuvSrc1->getStride();
  UInt  iDstStride  = rpcYuvDst->getStride();
  for ( y = iHeight-1; y >= 0; y-- )
  {
    for ( x = iWidth-1; x >= 0; )
    {
      // note: luma min width is 4
      pDstY[x] = weightBidir(w0,pSrcY0[x], w1,pSrcY1[x], round, shift, offset); x--;
      pDstY[x] = weightBidir(w0,pSrcY0[x], w1,pSrcY1[x], round, shift, offset); x--;
      pDstY[x] = weightBidir(w0,pSrcY0[x], w1,pSrcY1[x], round, shift, offset); x--;
      pDstY[x] = weightBidir(w0,pSrcY0[x], w1,pSrcY1[x], round, shift, offset); x--;
    }
    pSrcY0 += iSrc0Stride;
    pSrcY1 += iSrc1Stride;
    pDstY  += iDstStride;
  }

  
  // Chroma U : --------------------------------------------
  w0      = wp0[1].w;
  offset  = wp0[1].offset;
  shift   = wp0[1].shift + shiftNum;
  round   = (1<<(shift-1));
  w1      = wp1[1].w;

  iSrc0Stride = pcYuvSrc0->getCStride();
  iSrc1Stride = pcYuvSrc1->getCStride();
  iDstStride  = rpcYuvDst->getCStride();
  
  iWidth  >>=1;
  iHeight >>=1;
  
  for ( y = iHeight-1; y >= 0; y-- )
  {
    for ( x = iWidth-1; x >= 0; )
    {
      // note: chroma min width is 2
      pDstU[x] = weightBidir(w0,pSrcU0[x], w1,pSrcU1[x], round, shift, offset); x--;
      pDstU[x] = weightBidir(w0,pSrcU0[x], w1,pSrcU1[x], round, shift, offset); x--;
    }
    pSrcU0 += iSrc0Stride;
    pSrcU1 += iSrc1Stride;
    pDstU  += iDstStride;
  }

  // Chroma V : --------------------------------------------
  w0      = wp0[2].w;
  offset  = wp0[2].offset;
  shift   = wp0[2].shift + shiftNum;
  round   = (1<<(shift-1));
  w1      = wp1[2].w;

  for ( y = iHeight-1; y >= 0; y-- )
  {
    for ( x = iWidth-1; x >= 0; )
    {
      // note: chroma min width is 2
      pDstV[x] = weightBidir(w0,pSrcV0[x], w1,pSrcV1[x], round, shift, offset); x--;
      pDstV[x] = weightBidir(w0,pSrcV0[x], w1,pSrcV1[x], round, shift, offset); x--;
    }
    pSrcV0 += iSrc0Stride;
    pSrcV1 += iSrc1Stride;
    pDstV  += iDstStride;
  }
}

/** weighted averaging for uni-pred
 * \param TComYuv* pcYuvSrc0
 * \param iPartUnitIdx
 * \param iWidth
 * \param iHeight
 * \param wpScalingParam *wp0
 * \param TComYuv* rpcYuvDst
 * \returns Void
 */
Void TComWeightPrediction::addWeightUni( TComYuv* pcYuvSrc0, UInt iPartUnitIdx, UInt iWidth, UInt iHeight, wpScalingParam *wp0, TComYuv* rpcYuvDst )
{
  Int x, y;
  
  Pel* pSrcY0  = pcYuvSrc0->getLumaAddr( iPartUnitIdx );
  Pel* pSrcU0  = pcYuvSrc0->getCbAddr  ( iPartUnitIdx );
  Pel* pSrcV0  = pcYuvSrc0->getCrAddr  ( iPartUnitIdx );
  
  Pel* pDstY   = rpcYuvDst->getLumaAddr( iPartUnitIdx );
  Pel* pDstU   = rpcYuvDst->getCbAddr  ( iPartUnitIdx );
  Pel* pDstV   = rpcYuvDst->getCrAddr  ( iPartUnitIdx );
  
  // Luma : --------------------------------------------
  Int w0      = wp0[0].w;
  Int offset  = wp0[0].offset;
  Int shiftNum = IF_INTERNAL_PREC - ( g_uiBitDepth + g_uiBitIncrement );
  Int shift   = wp0[0].shift + shiftNum;
  Int round   = (1<<(shift-1));
  UInt  iSrc0Stride = pcYuvSrc0->getStride();
  UInt  iDstStride  = rpcYuvDst->getStride();
  
  for ( y = iHeight-1; y >= 0; y-- )
  {
    for ( x = iWidth-1; x >= 0; )
    {
      // note: luma min width is 4
      pDstY[x] = weightUnidir(w0,pSrcY0[x], round, shift, offset); x--;
      pDstY[x] = weightUnidir(w0,pSrcY0[x], round, shift, offset); x--;
      pDstY[x] = weightUnidir(w0,pSrcY0[x], round, shift, offset); x--;
      pDstY[x] = weightUnidir(w0,pSrcY0[x], round, shift, offset); x--;
    }
    pSrcY0 += iSrc0Stride;
    pDstY  += iDstStride;
  }
  
  // Chroma U : --------------------------------------------
  w0      = wp0[1].w;
  offset  = wp0[1].offset;
  shift   = wp0[1].shift + shiftNum;
  round   = (1<<(shift-1));

  iSrc0Stride = pcYuvSrc0->getCStride();
  iDstStride  = rpcYuvDst->getCStride();
  
  iWidth  >>=1;
  iHeight >>=1;
  
  for ( y = iHeight-1; y >= 0; y-- )
  {
    for ( x = iWidth-1; x >= 0; )
    {
      // note: chroma min width is 2
      pDstU[x] = weightUnidir(w0,pSrcU0[x], round, shift, offset); x--;
      pDstU[x] = weightUnidir(w0,pSrcU0[x], round, shift, offset); x--;
    }
    pSrcU0 += iSrc0Stride;
    pDstU  += iDstStride;
  }

  // Chroma V : --------------------------------------------
  w0      = wp0[2].w;
  offset  = wp0[2].offset;
  shift   = wp0[2].shift + shiftNum;
  round   = (1<<(shift-1));

  for ( y = iHeight-1; y >= 0; y-- )
  {
    for ( x = iWidth-1; x >= 0; )
    {
      // note: chroma min width is 2
      pDstV[x] = weightUnidir(w0,pSrcV0[x], round, shift, offset); x--;
      pDstV[x] = weightUnidir(w0,pSrcV0[x], round, shift, offset); x--;
    }
    pSrcV0 += iSrc0Stride;
    pDstV  += iDstStride;
  }
}

//=======================================================
//  getWpScaling()
//=======================================================
/** derivation of wp tables
 * \param TComDataCU* pcCU
 * \param iRefIdx0
 * \param iRefIdx1
 * \param wpScalingParam *&wp0
 * \param wpScalingParam *&wp1
 * \param ibdi
 * \returns Void
 */
Void TComWeightPrediction::getWpScaling( TComDataCU* pcCU, Int iRefIdx0, Int iRefIdx1, wpScalingParam *&wp0, wpScalingParam *&wp1, Int ibdi)
{
  TComSlice*      pcSlice       = pcCU->getSlice();
  TComPPS*        pps           = pcCU->getSlice()->getPPS();
  UInt            uiWPBiPredIdc = pps->getWPBiPredIdc();
  wpScalingParam* pwp;
  Bool            bBiDir        = (iRefIdx0>=0 && iRefIdx1>=0);
  Bool            bUniDir       = !bBiDir;

  m_ibdi = ibdi;

  if ( bUniDir || (uiWPBiPredIdc==1) )
  { // explicit --------------------
    if ( iRefIdx0 >= 0 )
    {
      pcSlice->getWpScaling(REF_PIC_LIST_0, iRefIdx0, wp0);
    }
    if ( iRefIdx1 >= 0 )
    {
      pcSlice->getWpScaling(REF_PIC_LIST_1, iRefIdx1, wp1);
    }
  }
  else if ( uiWPBiPredIdc == 2 )
  { // implicit --------------------
    Int poc0    = pcSlice->getRefPOC(REF_PIC_LIST_0, iRefIdx0);
    Int poc1    = pcSlice->getRefPOC(REF_PIC_LIST_1, iRefIdx1);
    Int pocCur  = pcSlice->getPOC();
    Int td      = Clip3(-128,127,poc1 - poc0);
    Int tb      = Clip3(-128,127,pocCur - poc0);
    Int tx      = ( td != 0 ) ? ( ( 16384 + abs( td / 2 ) ) / td ) : (0);
    Int DistScaleFactor = Clip3( -1024, 1023, ( tb * tx + 32 ) >> 6 );

    Bool  implicitScale=true;
    if ( poc1==poc0 || (DistScaleFactor>>2)<(-64) || (DistScaleFactor>>2)>128 )
    {
      implicitScale = false;
    }

    for ( int e=0 ; e<2 ; e++ )
    {
      pwp = (e==0) ? m_wp0 : m_wp1;
      for ( int k=0 ; k<3 ; k++ ) // 3 components: Y,U,V
      {   
        pwp->uiLog2WeightDenom = 5;  pwp->iOffset = 0;  pwp->bPresentFlag = true;  
        if ( implicitScale )
        {
          pwp->iWeight = (e==0) ? (64 - (DistScaleFactor >> 2)) : (DistScaleFactor >> 2);
        }
        else
        {
          pwp->iWeight = 32;
        }
        pwp++;
      }

    }

    wp0 = m_wp0;
    wp1 = m_wp1;
  }
  else
  {
    assert(0);
  }

  if ( iRefIdx0 < 0 )
  {
    wp0 = NULL;
  }
  if ( iRefIdx1 < 0 )
  {
    wp1 = NULL;
  }

  if ( bBiDir )
  { // Bi-Dir case
    for ( int yuv=0 ; yuv<3 ; yuv++ )
    {
      wp0[yuv].w      = wp0[yuv].iWeight;
      wp0[yuv].o      = wp0[yuv].iOffset * (1 << (m_ibdi-8));
      wp1[yuv].w      = wp1[yuv].iWeight;
      wp1[yuv].o      = wp1[yuv].iOffset * (1 << (m_ibdi-8));
      wp0[yuv].offset = wp0[yuv].o + wp1[yuv].o;
      wp0[yuv].shift  = wp0[yuv].uiLog2WeightDenom + 1;
      wp0[yuv].round  = (1 << wp0[yuv].uiLog2WeightDenom);
      wp1[yuv].offset = wp0[yuv].offset;
      wp1[yuv].shift  = wp0[yuv].shift;
      wp1[yuv].round  = wp0[yuv].round;
    }
  }
  else
  {  // Unidir
    pwp = (iRefIdx0>=0) ? wp0 : wp1 ;
    if ( uiWPBiPredIdc == 2 )
    {
      for ( int yuv=0 ; yuv<3 ; yuv++ )
      {
        pwp[yuv].w      = 1;
        pwp[yuv].offset = 0;
        pwp[yuv].shift  = 0;
        pwp[yuv].round  = 0;
      }
    }
    else {
      for ( int yuv=0 ; yuv<3 ; yuv++ )
      {
        pwp[yuv].w      = pwp[yuv].iWeight;
        pwp[yuv].offset = pwp[yuv].iOffset * (1 << (m_ibdi-8));
        pwp[yuv].shift  = pwp[yuv].uiLog2WeightDenom;
        pwp[yuv].round  = (pwp[yuv].uiLog2WeightDenom>=1) ? (1 << (pwp[yuv].uiLog2WeightDenom-1)) : (0);
      }
    }
  }
}

/** weighted prediction for bi-pred
 * \param TComDataCU* pcCU
 * \param TComYuv* pcYuvSrc0
 * \param TComYuv* pcYuvSrc1
 * \param iRefIdx0
 * \param iRefIdx1
 * \param uiPartIdx
 * \param iWidth
 * \param iHeight
 * \param TComYuv* rpcYuvDst
 * \returns Void
 */
Void TComWeightPrediction::xWeightedPredictionBi( TComDataCU* pcCU, TComYuv* pcYuvSrc0, TComYuv* pcYuvSrc1, Int iRefIdx0, Int iRefIdx1, UInt uiPartIdx, Int iWidth, Int iHeight, TComYuv* rpcYuvDst )
{
  wpScalingParam  *pwp0, *pwp1;
  TComPPS         *pps = pcCU->getSlice()->getPPS();

  assert( pps->getWPBiPredIdc() != 0 );

  Int ibdi = (g_uiBitDepth+g_uiBitIncrement);
  getWpScaling(pcCU, iRefIdx0, iRefIdx1, pwp0, pwp1, ibdi);

  if( iRefIdx0 >= 0 && iRefIdx1 >= 0 )
  {
    addWeightBi(pcYuvSrc0, pcYuvSrc1, uiPartIdx, iWidth, iHeight, pwp0, pwp1, rpcYuvDst );
  }
  else if ( iRefIdx0 >= 0 && iRefIdx1 <  0 )
  {
    addWeightUni( pcYuvSrc0, uiPartIdx, iWidth, iHeight, pwp0, rpcYuvDst );
  }
  else if ( iRefIdx0 <  0 && iRefIdx1 >= 0 )
  {
    addWeightUni( pcYuvSrc1, uiPartIdx, iWidth, iHeight, pwp1, rpcYuvDst );
  }
  else
  {
    assert (0);
  }
}

/** weighted prediction for uni-pred
 * \param TComDataCU* pcCU
 * \param TComYuv* pcYuvSrc
 * \param uiPartAddr
 * \param iWidth
 * \param iHeight
 * \param eRefPicList
 * \param TComYuv*& rpcYuvPred
 * \param iPartIdx
 * \param iRefIdx
 * \returns Void
 */
Void TComWeightPrediction::xWeightedPredictionUni( TComDataCU* pcCU, TComYuv* pcYuvSrc, UInt uiPartAddr, Int iWidth, Int iHeight, RefPicList eRefPicList, TComYuv*& rpcYuvPred, Int iPartIdx, Int iRefIdx)
{ 
  wpScalingParam  *pwp, *pwpTmp;
  if ( iRefIdx < 0 )
  {
    iRefIdx   = pcCU->getCUMvField( eRefPicList )->getRefIdx( uiPartAddr );
  }
  assert (iRefIdx >= 0);

  Int ibdi = (g_uiBitDepth+g_uiBitIncrement);

  if ( eRefPicList == REF_PIC_LIST_0 )
  {
    getWpScaling(pcCU, iRefIdx, -1, pwp, pwpTmp, ibdi);
  }
  else
  {
    getWpScaling(pcCU, -1, iRefIdx, pwpTmp, pwp, ibdi);
  }
  addWeightUni( pcYuvSrc, uiPartAddr, iWidth, iHeight, pwp, rpcYuvPred );
}


