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

/** \file     TComPrediction.cpp
    \brief    prediction class
*/

#include <memory.h>
#include "TComPrediction.h"

//! \ingroup TLibCommon
//! \{

// ====================================================================================================================
// Constructor / destructor / initialize
// ====================================================================================================================

TComPrediction::TComPrediction()
: m_pLumaRecBuffer(0)
{
  m_piYuvExt = NULL;
}

TComPrediction::~TComPrediction()
{
  
  delete[] m_piYuvExt;

  m_acYuvPred[0].destroy();
  m_acYuvPred[1].destroy();

  m_cYuvPredTemp.destroy();

  if( m_pLumaRecBuffer )
  {
    delete [] m_pLumaRecBuffer;
  }
  
  Int i, j;
  for (i = 0; i < 4; i++)
  {
    for (j = 0; j < 4; j++)
    {
      m_filteredBlock[i][j].destroy();
    }
    m_filteredBlockTmp[i].destroy();
  }
}

Void TComPrediction::initTempBuff()
{
  if( m_piYuvExt == NULL )
  {
    Int extWidth  = g_uiMaxCUWidth + 16; 
    Int extHeight = g_uiMaxCUHeight + 1;
    Int i, j;
    for (i = 0; i < 4; i++)
    {
      m_filteredBlockTmp[i].create(extWidth, extHeight + 7);
      for (j = 0; j < 4; j++)
      {
        m_filteredBlock[i][j].create(extWidth, extHeight);
      }
    }
    m_iYuvExtHeight  = ((g_uiMaxCUHeight + 2) << 4);
    m_iYuvExtStride = ((g_uiMaxCUWidth  + 8) << 4);
    m_piYuvExt = new Int[ m_iYuvExtStride * m_iYuvExtHeight ];

    // new structure
    m_acYuvPred[0] .create( g_uiMaxCUWidth, g_uiMaxCUHeight );
    m_acYuvPred[1] .create( g_uiMaxCUWidth, g_uiMaxCUHeight );

    m_cYuvPredTemp.create( g_uiMaxCUWidth, g_uiMaxCUHeight );
  }

  m_iLumaRecStride =  (g_uiMaxCUWidth>>1) + 1;
  m_pLumaRecBuffer = new Pel[ m_iLumaRecStride * m_iLumaRecStride ];

  for( Int i = 1; i < 64; i++ )
  {
    m_uiaShift[i-1] = ( (1 << 15) + i/2 ) / i;
  }
}

// ====================================================================================================================
// Public member functions
// ====================================================================================================================

// Function for calculating DC value of the reference samples used in Intra prediction
Pel TComPrediction::predIntraGetPredValDC( Int* pSrc, Int iSrcStride, UInt iWidth, UInt iHeight, Bool bAbove, Bool bLeft )
{
  Int iInd, iSum = 0;
  Pel pDcVal;

  if (bAbove)
  {
    for (iInd = 0;iInd < iWidth;iInd++)
    {
      iSum += pSrc[iInd-iSrcStride];
    }
  }
  if (bLeft)
  {
    for (iInd = 0;iInd < iHeight;iInd++)
    {
      iSum += pSrc[iInd*iSrcStride-1];
    }
  }

  if (bAbove && bLeft)
  {
    pDcVal = (iSum + iWidth) / (iWidth + iHeight);
  }
  else if (bAbove)
  {
    pDcVal = (iSum + iWidth/2) / iWidth;
  }
  else if (bLeft)
  {
    pDcVal = (iSum + iHeight/2) / iHeight;
  }
  else
  {
    pDcVal = pSrc[-1]; // Default DC value already calculated and placed in the prediction array if no neighbors are available
  }
  
  return pDcVal;
}

// Function for deriving the angular Intra predictions

/** Function for deriving the simplified angular intra predictions.
 * \param pSrc pointer to reconstructed sample array
 * \param srcStride the stride of the reconstructed sample array
 * \param rpDst reference to pointer for the prediction sample array
 * \param dstStride the stride of the prediction sample array
 * \param width the width of the block
 * \param height the height of the block
 * \param dirMode the intra prediction mode index
 * \param blkAboveAvailable boolean indication if the block above is available
 * \param blkLeftAvailable boolean indication if the block to the left is available
 *
 * This function derives the prediction samples for the angular mode based on the prediction direction indicated by
 * the prediction mode index. The prediction direction is given by the displacement of the bottom row of the block and
 * the reference row above the block in the case of vertical prediction or displacement of the rightmost column
 * of the block and reference column left from the block in the case of the horizontal prediction. The displacement
 * is signalled at 1/32 pixel accuracy. When projection of the predicted pixel falls inbetween reference samples,
 * the predicted value for the pixel is linearly interpolated from the reference samples. All reference samples are taken
 * from the extended main reference.
 */
Void TComPrediction::xPredIntraAng( Int* pSrc, Int srcStride, Pel*& rpDst, Int dstStride, UInt width, UInt height, UInt dirMode, Bool blkAboveAvailable, Bool blkLeftAvailable, Bool bFilter )
{
  Int k,l;
  Int blkSize        = width;
  Pel* pDst          = rpDst;

  // Map the mode index to main prediction direction and angle
#if LOGI_INTRA_NAME_3MPM
  assert( dirMode > 0 ); //no planar
  Bool modeDC        = dirMode < 2;
  Bool modeHor       = !modeDC && (dirMode < 18);
  Bool modeVer       = !modeDC && !modeHor;
  Int intraPredAngle = modeVer ? (Int)dirMode - VER_IDX : modeHor ? -((Int)dirMode - HOR_IDX) : 0;
#else
  Bool modeDC        = dirMode == 0;
  Bool modeVer       = !modeDC && (dirMode < 18);
  Bool modeHor       = !modeDC && !modeVer;
  Int intraPredAngle = modeVer ? dirMode - 9 : modeHor ? dirMode - 25 : 0;
#endif
  Int absAng         = abs(intraPredAngle);
  Int signAng        = intraPredAngle < 0 ? -1 : 1;

  // Set bitshifts and scale the angle parameter to block size
  Int angTable[9]    = {0,    2,    5,   9,  13,  17,  21,  26,  32};
  Int invAngTable[9] = {0, 4096, 1638, 910, 630, 482, 390, 315, 256}; // (256 * 32) / Angle
  Int invAngle       = invAngTable[absAng];
  absAng             = angTable[absAng];
  intraPredAngle     = signAng * absAng;

  // Do the DC prediction
  if (modeDC)
  {
    Pel dcval = predIntraGetPredValDC(pSrc, srcStride, width, height, blkAboveAvailable, blkLeftAvailable);

    for (k=0;k<blkSize;k++)
    {
      for (l=0;l<blkSize;l++)
      {
        pDst[k*dstStride+l] = dcval;
      }
    }
  }

  // Do angular predictions
  else
  {
    Pel* refMain;
    Pel* refSide;
    Pel  refAbove[2*MAX_CU_SIZE+1];
    Pel  refLeft[2*MAX_CU_SIZE+1];

    // Initialise the Main and Left reference array.
    if (intraPredAngle < 0)
    {
      for (k=0;k<blkSize+1;k++)
      {
        refAbove[k+blkSize-1] = pSrc[k-srcStride-1];
      }
      for (k=0;k<blkSize+1;k++)
      {
        refLeft[k+blkSize-1] = pSrc[(k-1)*srcStride-1];
      }
      refMain = (modeVer ? refAbove : refLeft) + (blkSize-1);
      refSide = (modeVer ? refLeft : refAbove) + (blkSize-1);

      // Extend the Main reference to the left.
      Int invAngleSum    = 128;       // rounding for (shift by 8)
      for (k=-1; k>blkSize*intraPredAngle>>5; k--)
      {
        invAngleSum += invAngle;
        refMain[k] = refSide[invAngleSum>>8];
      }
    }
    else
    {
      for (k=0;k<2*blkSize+1;k++)
      {
        refAbove[k] = pSrc[k-srcStride-1];
      }
      for (k=0;k<2*blkSize+1;k++)
      {
        refLeft[k] = pSrc[(k-1)*srcStride-1];
      }
      refMain = modeVer ? refAbove : refLeft;
      refSide = modeVer ? refLeft  : refAbove;
    }

    if (intraPredAngle == 0)
    {
      for (k=0;k<blkSize;k++)
      {
        for (l=0;l<blkSize;l++)
        {
          pDst[k*dstStride+l] = refMain[l+1];
        }
      }

      if ( bFilter )
      {
        for (k=0;k<blkSize;k++)
        {
#if REMOVE_DIV_OPERATION
          pDst[k*dstStride] = Clip ( pDst[k*dstStride] + (( refSide[k+1] - refSide[0] ) >> 1) );
#else
          pDst[k*dstStride] = Clip ( pDst[k*dstStride] + ( refSide[k+1] - refSide[0] ) / 2 );
#endif
        }
      }
    }
    else
    {
      Int deltaPos=0;
      Int deltaInt;
      Int deltaFract;
      Int refMainIndex;

      for (k=0;k<blkSize;k++)
      {
        deltaPos += intraPredAngle;
        deltaInt   = deltaPos >> 5;
        deltaFract = deltaPos & (32 - 1);

        if (deltaFract)
        {
          // Do linear filtering
          for (l=0;l<blkSize;l++)
          {
            refMainIndex        = l+deltaInt+1;
            pDst[k*dstStride+l] = (Pel) ( ((32-deltaFract)*refMain[refMainIndex]+deltaFract*refMain[refMainIndex+1]+16) >> 5 );
          }
        }
        else
        {
          // Just copy the integer samples
          for (l=0;l<blkSize;l++)
          {
            pDst[k*dstStride+l] = refMain[l+deltaInt+1];
          }
        }
      }
    }

    // Flip the block if this is the horizontal mode
    if (modeHor)
    {
      Pel  tmp;
      for (k=0;k<blkSize-1;k++)
      {
        for (l=k+1;l<blkSize;l++)
        {
          tmp                 = pDst[k*dstStride+l];
          pDst[k*dstStride+l] = pDst[l*dstStride+k];
          pDst[l*dstStride+k] = tmp;
        }
      }
    }
  }
}

Void TComPrediction::predIntraLumaAng(TComPattern* pcTComPattern, UInt uiDirMode, Pel* piPred, UInt uiStride, Int iWidth, Int iHeight,  TComDataCU* pcCU, Bool bAbove, Bool bLeft )
{
  Pel *pDst = piPred;
  Int *ptrSrc;

  assert( g_aucConvertToBit[ iWidth ] >= 0 ); //   4x  4
  assert( g_aucConvertToBit[ iWidth ] <= 5 ); // 128x128
  assert( iWidth == iHeight  );

  ptrSrc = pcTComPattern->getPredictorPtr( uiDirMode, g_aucConvertToBit[ iWidth ] + 2, m_piYuvExt );

  // get starting pixel in block
  Int sw = 2 * iWidth + 1;

  // Create the prediction
  if ( uiDirMode == PLANAR_IDX )
  {
    xPredIntraPlanar( ptrSrc+sw+1, sw, pDst, uiStride, iWidth, iHeight );
  }
  else
  {
#if LOGI_INTRA_NAME_3MPM
    xPredIntraAng( ptrSrc+sw+1, sw, pDst, uiStride, iWidth, iHeight, uiDirMode, bAbove, bLeft, true );
#else
    xPredIntraAng( ptrSrc+sw+1, sw, pDst, uiStride, iWidth, iHeight, g_aucAngIntraModeOrder[ uiDirMode ], bAbove, bLeft, true );
#endif

    if( (uiDirMode == DC_IDX ) && bAbove && bLeft )
    {
      xDCPredFiltering( ptrSrc+sw+1, sw, pDst, uiStride, iWidth, iHeight);
    }
  }
}

// Angular chroma
Void TComPrediction::predIntraChromaAng( TComPattern* pcTComPattern, Int* piSrc, UInt uiDirMode, Pel* piPred, UInt uiStride, Int iWidth, Int iHeight, TComDataCU* pcCU, Bool bAbove, Bool bLeft )
{
  Pel *pDst = piPred;
  Int *ptrSrc = piSrc;

  // get starting pixel in block
  Int sw = 2 * iWidth + 1;

  if ( uiDirMode == PLANAR_IDX )
  {
    xPredIntraPlanar( ptrSrc+sw+1, sw, pDst, uiStride, iWidth, iHeight );
  }
  else
  {
    // Create the prediction
#if LOGI_INTRA_NAME_3MPM
    xPredIntraAng( ptrSrc+sw+1, sw, pDst, uiStride, iWidth, iHeight, uiDirMode, bAbove, bLeft, false );
#else
    xPredIntraAng( ptrSrc+sw+1, sw, pDst, uiStride, iWidth, iHeight, g_aucAngIntraModeOrder[ uiDirMode ], bAbove, bLeft, false );
#endif
  }
}

/** Function for checking identical motion.
 * \param TComDataCU* pcCU
 * \param UInt PartAddr
 */
Bool TComPrediction::xCheckIdenticalMotion ( TComDataCU* pcCU, UInt PartAddr )
{
  if( pcCU->getSlice()->isInterB() && pcCU->getSlice()->getPPS()->getWPBiPredIdc() == 0 )
  {
    if( pcCU->getCUMvField(REF_PIC_LIST_0)->getRefIdx(PartAddr) >= 0 && pcCU->getCUMvField(REF_PIC_LIST_1)->getRefIdx(PartAddr) >= 0)
    {
      Int RefPOCL0    = pcCU->getSlice()->getRefPic(REF_PIC_LIST_0, pcCU->getCUMvField(REF_PIC_LIST_0)->getRefIdx(PartAddr))->getPOC();
      Int RefViewIdL0 = pcCU->getSlice()->getRefPic(REF_PIC_LIST_0, pcCU->getCUMvField(REF_PIC_LIST_0)->getRefIdx(PartAddr))->getViewId();
      Int RefPOCL1    = pcCU->getSlice()->getRefPic(REF_PIC_LIST_1, pcCU->getCUMvField(REF_PIC_LIST_1)->getRefIdx(PartAddr))->getPOC();
      Int RefViewIdL1 = pcCU->getSlice()->getRefPic(REF_PIC_LIST_1, pcCU->getCUMvField(REF_PIC_LIST_1)->getRefIdx(PartAddr))->getViewId();
      if(RefPOCL0 == RefPOCL1 && RefViewIdL0 == RefViewIdL1 && pcCU->getCUMvField(REF_PIC_LIST_0)->getMv(PartAddr) == pcCU->getCUMvField(REF_PIC_LIST_1)->getMv(PartAddr))
      {
        return true;
      }
    }
  }
  return false;
}

#if DEPTH_MAP_GENERATION
Void TComPrediction::motionCompensation( TComDataCU* pcCU, TComYuv* pcYuvPred, RefPicList eRefPicList, Int iPartIdx, Bool bPrdDepthMap, UInt uiSubSampExpX, UInt uiSubSampExpY )
#else
Void TComPrediction::motionCompensation ( TComDataCU* pcCU, TComYuv* pcYuvPred, RefPicList eRefPicList, Int iPartIdx )
#endif
{
  Int         iWidth;
  Int         iHeight;
  UInt        uiPartAddr;

  if ( iPartIdx >= 0 )
  {
    pcCU->getPartIndexAndSize( iPartIdx, uiPartAddr, iWidth, iHeight );

#if DEPTH_MAP_GENERATION
    if( bPrdDepthMap )
    {
      iWidth  >>= uiSubSampExpX;
      iHeight >>= uiSubSampExpY;
    }
#endif

    if ( eRefPicList != REF_PIC_LIST_X )
    {
      if( pcCU->getSlice()->getPPS()->getUseWP())
      {
#if DEPTH_MAP_GENERATION
        xPredInterUni (pcCU, uiPartAddr, iWidth, iHeight, eRefPicList, pcYuvPred, iPartIdx, bPrdDepthMap, uiSubSampExpX, uiSubSampExpY, true );
#else
        xPredInterUni (pcCU, uiPartAddr, iWidth, iHeight, eRefPicList, pcYuvPred, iPartIdx, true );
#endif
      }
      else
      {
#if DEPTH_MAP_GENERATION
        xPredInterUni (pcCU, uiPartAddr, iWidth, iHeight, eRefPicList, pcYuvPred, iPartIdx, bPrdDepthMap, uiSubSampExpX, uiSubSampExpY, false );
#else
        xPredInterUni (pcCU, uiPartAddr, iWidth, iHeight, eRefPicList, pcYuvPred, iPartIdx, false );
#endif
      }
      if ( pcCU->getSlice()->getPPS()->getUseWP() )
      {
        xWeightedPredictionUni( pcCU, pcYuvPred, uiPartAddr, iWidth, iHeight, eRefPicList, pcYuvPred, iPartIdx );
      }
    }
    else
    {
#if DEPTH_MAP_GENERATION
      if( xCheckIdenticalMotion( pcCU, uiPartAddr ) && !bPrdDepthMap )
#else
      if ( xCheckIdenticalMotion( pcCU, uiPartAddr ) )
#endif
      {
#if DEPTH_MAP_GENERATION
        xPredInterUni (pcCU, uiPartAddr, iWidth, iHeight, REF_PIC_LIST_0, pcYuvPred, iPartIdx, bPrdDepthMap, uiSubSampExpX, uiSubSampExpY, false );
#else
        xPredInterUni (pcCU, uiPartAddr, iWidth, iHeight, REF_PIC_LIST_0, pcYuvPred, iPartIdx, false );
#endif
      }
      else
      {
#if DEPTH_MAP_GENERATION
        xPredInterBi  (pcCU, uiPartAddr, iWidth, iHeight, uiSubSampExpX, uiSubSampExpY, pcYuvPred, iPartIdx, bPrdDepthMap );
#else
        xPredInterBi  (pcCU, uiPartAddr, iWidth, iHeight, pcYuvPred, iPartIdx );
#endif
      }
    }
    return;
  }

  for ( iPartIdx = 0; iPartIdx < pcCU->getNumPartInter(); iPartIdx++ )
  {
    pcCU->getPartIndexAndSize( iPartIdx, uiPartAddr, iWidth, iHeight );

#if DEPTH_MAP_GENERATION
    if( bPrdDepthMap )
    {
      iWidth  >>= uiSubSampExpX;
      iHeight >>= uiSubSampExpY;
    }
#endif

    if ( eRefPicList != REF_PIC_LIST_X )
    {
      if( pcCU->getSlice()->getPPS()->getUseWP())
      {
#if DEPTH_MAP_GENERATION
        xPredInterUni (pcCU, uiPartAddr, iWidth, iHeight, eRefPicList, pcYuvPred, iPartIdx, bPrdDepthMap, uiSubSampExpX, uiSubSampExpY, true );
#else
        xPredInterUni (pcCU, uiPartAddr, iWidth, iHeight, eRefPicList, pcYuvPred, iPartIdx, true );
#endif   
      }
      else
      {
#if DEPTH_MAP_GENERATION
        xPredInterUni (pcCU, uiPartAddr, iWidth, iHeight, eRefPicList, pcYuvPred, iPartIdx, bPrdDepthMap, uiSubSampExpX, uiSubSampExpY, false );
#else
        xPredInterUni (pcCU, uiPartAddr, iWidth, iHeight, eRefPicList, pcYuvPred, iPartIdx, false );
#endif   
      }
#if DEPTH_MAP_GENERATION
      xPredInterUni (pcCU, uiPartAddr, iWidth, iHeight, eRefPicList, pcYuvPred, iPartIdx, bPrdDepthMap, uiSubSampExpX, uiSubSampExpY, false );
#else
      xPredInterUni (pcCU, uiPartAddr, iWidth, iHeight, eRefPicList, pcYuvPred, iPartIdx, false );
#endif  
      if ( pcCU->getSlice()->getPPS()->getUseWP() )
      {
        xWeightedPredictionUni( pcCU, pcYuvPred, uiPartAddr, iWidth, iHeight, eRefPicList, pcYuvPred, iPartIdx );
      }
    }
    else
    {
      if ( xCheckIdenticalMotion( pcCU, uiPartAddr ) )
      {
#if DEPTH_MAP_GENERATION
        xPredInterUni (pcCU, uiPartAddr, iWidth, iHeight, REF_PIC_LIST_0, pcYuvPred, iPartIdx, bPrdDepthMap, uiSubSampExpX, uiSubSampExpY, false );
#else
        xPredInterUni (pcCU, uiPartAddr, iWidth, iHeight, REF_PIC_LIST_0, pcYuvPred, iPartIdx, false );
#endif 
      }
      else
      {
#if DEPTH_MAP_GENERATION
        xPredInterBi  (pcCU, uiPartAddr, iWidth, iHeight, uiSubSampExpX, uiSubSampExpY, pcYuvPred, iPartIdx, bPrdDepthMap );
#else
        xPredInterBi  (pcCU, uiPartAddr, iWidth, iHeight, pcYuvPred, iPartIdx );
#endif
      }
    }
  }
  return;
}



#if DEPTH_MAP_GENERATION
Void TComPrediction::xPredInterUni ( TComDataCU* pcCU, UInt uiPartAddr, Int iWidth, Int iHeight, RefPicList eRefPicList, TComYuv*& rpcYuvPred, Int iPartIdx, Bool bPrdDepthMap, UInt uiSubSampExpX, UInt uiSubSampExpY, Bool bi )
#else
Void TComPrediction::xPredInterUni ( TComDataCU* pcCU, UInt uiPartAddr, Int iWidth, Int iHeight, RefPicList eRefPicList, TComYuv*& rpcYuvPred, Int iPartIdx, Bool bi )
#endif
{
  Int         iRefIdx     = pcCU->getCUMvField( eRefPicList )->getRefIdx( uiPartAddr );           assert (iRefIdx >= 0);
  TComMv      cMv         = pcCU->getCUMvField( eRefPicList )->getMv( uiPartAddr );
  pcCU->clipMv(cMv);

#if DEPTH_MAP_GENERATION
  if( bPrdDepthMap )
  {
    UInt uiRShift = 0;
#if PDM_REMOVE_DEPENDENCE
	if(pcCU->getPic()->getStoredPDMforV2()==1)
	    xPredInterPrdDepthMap( pcCU, pcCU->getSlice()->getRefPic( eRefPicList, iRefIdx )->getPredDepthMapTemp(), uiPartAddr, &cMv, iWidth, iHeight, uiSubSampExpX, uiSubSampExpY, rpcYuvPred, uiRShift, 0 );
	else
#endif
    xPredInterPrdDepthMap( pcCU, pcCU->getSlice()->getRefPic( eRefPicList, iRefIdx )->getPredDepthMap(), uiPartAddr, &cMv, iWidth, iHeight, uiSubSampExpX, uiSubSampExpY, rpcYuvPred, uiRShift, 0 );
    return;
  }
#endif

#if HHI_FULL_PEL_DEPTH_MAP_MV_ACC
  if( pcCU->getSlice()->getSPS()->isDepth() )
  {
    UInt uiRShift = ( bi ? 14-g_uiBitDepth-g_uiBitIncrement : 0 );
    UInt uiOffset = bi ? IF_INTERNAL_OFFS : 0;
#if DEPTH_MAP_GENERATION
    xPredInterPrdDepthMap( pcCU, pcCU->getSlice()->getRefPic( eRefPicList, iRefIdx )->getPicYuvRec(), uiPartAddr, &cMv, iWidth, iHeight, 0, 0, rpcYuvPred, uiRShift, uiOffset );
#else
    xPredInterPrdDepthMap( pcCU, pcCU->getSlice()->getRefPic( eRefPicList, iRefIdx )->getPicYuvRec(), uiPartAddr, &cMv, iWidth, iHeight, rpcYuvPred, uiRShift, uiOffset );
#endif
  }
  else
  {
#endif
  xPredInterLumaBlk  ( pcCU, pcCU->getSlice()->getRefPic( eRefPicList, iRefIdx )->getPicYuvRec(), uiPartAddr, &cMv, iWidth, iHeight, rpcYuvPred, bi );
#if HHI_FULL_PEL_DEPTH_MAP_MV_ACC
  }
#endif
  xPredInterChromaBlk( pcCU, pcCU->getSlice()->getRefPic( eRefPicList, iRefIdx )->getPicYuvRec(), uiPartAddr, &cMv, iWidth, iHeight, rpcYuvPred, bi );
}


#if DEPTH_MAP_GENERATION
Void TComPrediction::xPredInterBi ( TComDataCU* pcCU, UInt uiPartAddr, Int iWidth, Int iHeight, UInt uiSubSampExpX, UInt uiSubSampExpY, TComYuv*& rpcYuvPred, Int iPartIdx, Bool bPrdDepthMap )
#else
Void TComPrediction::xPredInterBi ( TComDataCU* pcCU, UInt uiPartAddr, Int iWidth, Int iHeight, TComYuv*& rpcYuvPred, Int iPartIdx )
#endif
{
  TComYuv* pcMbYuv;
  Int      iRefIdx[2] = {-1, -1};

  for ( Int iRefList = 0; iRefList < 2; iRefList++ )
  {
    RefPicList eRefPicList = (iRefList ? REF_PIC_LIST_1 : REF_PIC_LIST_0);
    iRefIdx[iRefList] = pcCU->getCUMvField( eRefPicList )->getRefIdx( uiPartAddr );

    if ( iRefIdx[iRefList] < 0 )
    {
      continue;
    }

    assert( iRefIdx[iRefList] < pcCU->getSlice()->getNumRefIdx(eRefPicList) );

    pcMbYuv = &m_acYuvPred[iRefList];
    if( pcCU->getCUMvField( REF_PIC_LIST_0 )->getRefIdx( uiPartAddr ) >= 0 && pcCU->getCUMvField( REF_PIC_LIST_1 )->getRefIdx( uiPartAddr ) >= 0 )
    {
#if DEPTH_MAP_GENERATION
      xPredInterUni ( pcCU, uiPartAddr, iWidth, iHeight, eRefPicList, pcMbYuv, iPartIdx, bPrdDepthMap, uiSubSampExpX, uiSubSampExpY, true );
#else
      xPredInterUni ( pcCU, uiPartAddr, iWidth, iHeight, eRefPicList, pcMbYuv, iPartIdx, true );
#endif
    }
    else
    {
      if ( pcCU->getSlice()->getPPS()->getWPBiPredIdc() )
      {
#if DEPTH_MAP_GENERATION
        xPredInterUni ( pcCU, uiPartAddr, iWidth, iHeight, eRefPicList, pcMbYuv, iPartIdx, bPrdDepthMap, uiSubSampExpX, uiSubSampExpY, true );
#else
        xPredInterUni ( pcCU, uiPartAddr, iWidth, iHeight, eRefPicList, pcMbYuv, iPartIdx, true );
#endif
      }
      else
      {
#if DEPTH_MAP_GENERATION
        xPredInterUni ( pcCU, uiPartAddr, iWidth, iHeight, eRefPicList, pcMbYuv, iPartIdx, bPrdDepthMap, uiSubSampExpX, uiSubSampExpY, false );
#else
        xPredInterUni ( pcCU, uiPartAddr, iWidth, iHeight, eRefPicList, pcMbYuv, iPartIdx, false );
#endif
      }
    }
  }

  if ( pcCU->getSlice()->getPPS()->getWPBiPredIdc() )
  {
    xWeightedPredictionBi( pcCU, &m_acYuvPred[0], &m_acYuvPred[1], iRefIdx[0], iRefIdx[1], uiPartAddr, iWidth, iHeight, rpcYuvPred );
  }
  else
  {
#if DEPTH_MAP_GENERATION
    if ( bPrdDepthMap )
    {
      xWeightedAveragePdm( pcCU, &m_acYuvPred[0], &m_acYuvPred[1], iRefIdx[0], iRefIdx[1], uiPartAddr, iWidth, iHeight, rpcYuvPred, uiSubSampExpX, uiSubSampExpY );
    }
    else
    {
    xWeightedAverage( pcCU, &m_acYuvPred[0], &m_acYuvPred[1], iRefIdx[0], iRefIdx[1], uiPartAddr, iWidth, iHeight, rpcYuvPred );
  }
#else
    xWeightedAverage( pcCU, &m_acYuvPred[0], &m_acYuvPred[1], iRefIdx[0], iRefIdx[1], uiPartAddr, iWidth, iHeight, rpcYuvPred );
#endif
  }
}

Void 
#if DEPTH_MAP_GENERATION
TComPrediction::xPredInterPrdDepthMap( TComDataCU* pcCU, TComPicYuv* pcPicYuvRef, UInt uiPartAddr, TComMv* pcMv, Int iWidth, Int iHeight, UInt uiSubSampExpX, UInt uiSubSampExpY, TComYuv*& rpcYuv, UInt uiRShift, UInt uiOffset )
#else
TComPrediction::xPredInterPrdDepthMap( TComDataCU* pcCU, TComPicYuv* pcPicYuvRef, UInt uiPartAddr, TComMv* pcMv, Int iWidth, Int iHeight, TComYuv*& rpcYuv, UInt uiRShift, UInt uiOffset )
#endif
{
#if DEPTH_MAP_GENERATION
  Int     iShiftX     = 2 + uiSubSampExpX;
  Int     iShiftY     = 2 + uiSubSampExpY;
  Int     iAddX       = ( 1 << iShiftX ) >> 1;
  Int     iAddY       = ( 1 << iShiftY ) >> 1;
  Int     iHor        = ( pcMv->getHor() + iAddX ) >> iShiftX;
  Int     iVer        = ( pcMv->getVer() + iAddY ) >> iShiftY;
#if HHI_FULL_PEL_DEPTH_MAP_MV_ACC
  if( pcCU->getSlice()->getSPS()->isDepth() )
  {
    iHor = pcMv->getHor();
    iVer = pcMv->getVer();
  }
#endif
  Int     iRefStride  = pcPicYuvRef->getStride();
  Int     iDstStride  = rpcYuv->getStride();
  Int     iRefOffset  = iHor + iVer * iRefStride;
#else
  Int     iFPelMask   = ~3;
  Int     iRefStride  = pcPicYuvRef->getStride();
  Int     iDstStride  = rpcYuv->getStride();
  Int     iHor        = ( pcMv->getHor() + 2 ) & iFPelMask;
  Int     iVer        = ( pcMv->getVer() + 2 ) & iFPelMask;
#if HHI_FULL_PEL_DEPTH_MAP_MV_ACC
  if( pcCU->getSlice()->getSPS()->isDepth() )
  {
    iHor = pcMv->getHor() * 4;
    iVer = pcMv->getVer() * 4;
}
#endif
  Int     ixFrac      = iHor & 0x3;
  Int     iyFrac      = iVer & 0x3;
  Int     iRefOffset  = ( iHor >> 2 ) + ( iVer >> 2 ) * iRefStride;
#endif

  Pel*    piRefY      = pcPicYuvRef->getLumaAddr( pcCU->getAddr(), pcCU->getZorderIdxInCU() + uiPartAddr ) + iRefOffset;
  Pel*    piDstY      = rpcYuv->getLumaAddr( uiPartAddr );

  for( Int y = 0; y < iHeight; y++, piDstY += iDstStride, piRefY += iRefStride )
  {
    for( Int x = 0; x < iWidth; x++ )
    {
      piDstY[ x ] = ( piRefY[ x ] << uiRShift ) - uiOffset;
    }
  }
}


/**
 * \brief Generate motion-compensated luma block
 *
 * \param cu       Pointer to current CU
 * \param refPic   Pointer to reference picture
 * \param partAddr Address of block within CU
 * \param mv       Motion vector
 * \param width    Width of block
 * \param height   Height of block
 * \param dstPic   Pointer to destination picture
 * \param bi       Flag indicating whether bipred is used
 */
Void TComPrediction::xPredInterLumaBlk( TComDataCU *cu, TComPicYuv *refPic, UInt partAddr, TComMv *mv, Int width, Int height, TComYuv *&dstPic, Bool bi )
{
  Int refStride = refPic->getStride();  
  Int refOffset = ( mv->getHor() >> 2 ) + ( mv->getVer() >> 2 ) * refStride;
  Pel *ref      = refPic->getLumaAddr( cu->getAddr(), cu->getZorderIdxInCU() + partAddr ) + refOffset;
  
  Int dstStride = dstPic->getStride();
  Pel *dst      = dstPic->getLumaAddr( partAddr );
  
  Int xFrac = mv->getHor() & 0x3;
  Int yFrac = mv->getVer() & 0x3;

#if HHI_FULL_PEL_DEPTH_MAP_MV_ACC
  assert( ! cu->getSlice()->getIsDepth() || ( xFrac == 0 && yFrac == 0 ) );
#endif

  if ( yFrac == 0 )
  {
    m_if.filterHorLuma( ref, refStride, dst, dstStride, width, height, xFrac,       !bi );
  }
  else if ( xFrac == 0 )
  {
    m_if.filterVerLuma( ref, refStride, dst, dstStride, width, height, yFrac, true, !bi );
  }
  else
  {
    Int tmpStride = m_filteredBlockTmp[0].getStride();
    Short *tmp    = m_filteredBlockTmp[0].getLumaAddr();

    Int filterSize = NTAPS_LUMA;
    Int halfFilterSize = ( filterSize >> 1 );

    m_if.filterHorLuma(ref - (halfFilterSize-1)*refStride, refStride, tmp, tmpStride, width, height+filterSize-1, xFrac, false     );
    m_if.filterVerLuma(tmp + (halfFilterSize-1)*tmpStride, tmpStride, dst, dstStride, width, height,              yFrac, false, !bi);    
  }
}

/**
 * \brief Generate motion-compensated chroma block
 *
 * \param cu       Pointer to current CU
 * \param refPic   Pointer to reference picture
 * \param partAddr Address of block within CU
 * \param mv       Motion vector
 * \param width    Width of block
 * \param height   Height of block
 * \param dstPic   Pointer to destination picture
 * \param bi       Flag indicating whether bipred is used
 */
Void TComPrediction::xPredInterChromaBlk( TComDataCU *cu, TComPicYuv *refPic, UInt partAddr, TComMv *mv, Int width, Int height, TComYuv *&dstPic, Bool bi )
{
  Int     refStride  = refPic->getCStride();
  Int     dstStride  = dstPic->getCStride();
  
  Int     refOffset  = (mv->getHor() >> 3) + (mv->getVer() >> 3) * refStride;
  
  Pel*    refCb     = refPic->getCbAddr( cu->getAddr(), cu->getZorderIdxInCU() + partAddr ) + refOffset;
  Pel*    refCr     = refPic->getCrAddr( cu->getAddr(), cu->getZorderIdxInCU() + partAddr ) + refOffset;
  
  Pel* dstCb = dstPic->getCbAddr( partAddr );
  Pel* dstCr = dstPic->getCrAddr( partAddr );
  
  Int     xFrac  = mv->getHor() & 0x7;
  Int     yFrac  = mv->getVer() & 0x7;
  UInt    cxWidth  = width  >> 1;
  UInt    cxHeight = height >> 1;
  
  Int     extStride = m_filteredBlockTmp[0].getStride();
  Short*  extY      = m_filteredBlockTmp[0].getLumaAddr();
  
  Int filterSize = NTAPS_CHROMA;
  
  Int halfFilterSize = (filterSize>>1);
  
  if ( yFrac == 0 )
  {
    m_if.filterHorChroma(refCb, refStride, dstCb,  dstStride, cxWidth, cxHeight, xFrac, !bi);    
    m_if.filterHorChroma(refCr, refStride, dstCr,  dstStride, cxWidth, cxHeight, xFrac, !bi);    
  }
  else if ( xFrac == 0 )
  {
    m_if.filterVerChroma(refCb, refStride, dstCb, dstStride, cxWidth, cxHeight, yFrac, true, !bi);    
    m_if.filterVerChroma(refCr, refStride, dstCr, dstStride, cxWidth, cxHeight, yFrac, true, !bi);    
  }
  else
  {
    m_if.filterHorChroma(refCb - (halfFilterSize-1)*refStride, refStride, extY,  extStride, cxWidth, cxHeight+filterSize-1, xFrac, false);
    m_if.filterVerChroma(extY  + (halfFilterSize-1)*extStride, extStride, dstCb, dstStride, cxWidth, cxHeight  , yFrac, false, !bi);
    
    m_if.filterHorChroma(refCr - (halfFilterSize-1)*refStride, refStride, extY,  extStride, cxWidth, cxHeight+filterSize-1, xFrac, false);
    m_if.filterVerChroma(extY  + (halfFilterSize-1)*extStride, extStride, dstCr, dstStride, cxWidth, cxHeight  , yFrac, false, !bi);    
  }
}

#if DEPTH_MAP_GENERATION
Void TComPrediction::xWeightedAveragePdm( TComDataCU* pcCU, TComYuv* pcYuvSrc0, TComYuv* pcYuvSrc1, Int iRefIdx0, Int iRefIdx1, UInt uiPartIdx, Int iWidth, Int iHeight, TComYuv*& rpcYuvDst, UInt uiSubSampExpX, UInt uiSubSampExpY )
{
  if( iRefIdx0 >= 0 && iRefIdx1 >= 0 )
  {
    rpcYuvDst->addAvgPdm( pcYuvSrc0, pcYuvSrc1, uiPartIdx, iWidth, iHeight, uiSubSampExpX, uiSubSampExpY );
  }
  else if ( iRefIdx0 >= 0 && iRefIdx1 <  0 )
  {
    pcYuvSrc0->copyPartToPartYuvPdm( rpcYuvDst, uiPartIdx, iWidth, iHeight, uiSubSampExpX, uiSubSampExpY );
  }
  else if ( iRefIdx0 <  0 && iRefIdx1 >= 0 )
  {
    pcYuvSrc1->copyPartToPartYuvPdm( rpcYuvDst, uiPartIdx, iWidth, iHeight, uiSubSampExpX, uiSubSampExpY );
  }
  else
  {
    assert (0);
  }
}
#endif

Void TComPrediction::xWeightedAverage( TComDataCU* pcCU, TComYuv* pcYuvSrc0, TComYuv* pcYuvSrc1, Int iRefIdx0, Int iRefIdx1, UInt uiPartIdx, Int iWidth, Int iHeight, TComYuv*& rpcYuvDst )
{
  if( iRefIdx0 >= 0 && iRefIdx1 >= 0 )
  {
    rpcYuvDst->addAvg( pcYuvSrc0, pcYuvSrc1, uiPartIdx, iWidth, iHeight );
  }
  else if ( iRefIdx0 >= 0 && iRefIdx1 <  0 )
  {
    pcYuvSrc0->copyPartToPartYuv( rpcYuvDst, uiPartIdx, iWidth, iHeight );
  }
  else if ( iRefIdx0 <  0 && iRefIdx1 >= 0 )
  {
    pcYuvSrc1->copyPartToPartYuv( rpcYuvDst, uiPartIdx, iWidth, iHeight );
  }
}

// AMVP
Void TComPrediction::getMvPredAMVP( TComDataCU* pcCU, UInt uiPartIdx, UInt uiPartAddr, RefPicList eRefPicList, Int iRefIdx, TComMv& rcMvPred )
{
  AMVPInfo* pcAMVPInfo = pcCU->getCUMvField(eRefPicList)->getAMVPInfo();

  if( pcCU->getAMVPMode(uiPartAddr) == AM_NONE || (pcAMVPInfo->iN <= 1 && pcCU->getAMVPMode(uiPartAddr) == AM_EXPL) )
  {
    rcMvPred = pcAMVPInfo->m_acMvCand[0];

    pcCU->setMVPIdxSubParts( 0, eRefPicList, uiPartAddr, uiPartIdx, pcCU->getDepth(uiPartAddr));
    pcCU->setMVPNumSubParts( pcAMVPInfo->iN, eRefPicList, uiPartAddr, uiPartIdx, pcCU->getDepth(uiPartAddr));
    return;
  }

  assert(pcCU->getMVPIdx(eRefPicList,uiPartAddr) >= 0);
  rcMvPred = pcAMVPInfo->m_acMvCand[pcCU->getMVPIdx(eRefPicList,uiPartAddr)];
  return;
}

/** Function for deriving planar intra prediction.
 * \param pSrc pointer to reconstructed sample array
 * \param srcStride the stride of the reconstructed sample array
 * \param rpDst reference to pointer for the prediction sample array
 * \param dstStride the stride of the prediction sample array
 * \param width the width of the block
 * \param height the height of the block
 *
 * This function derives the prediction samples for planar mode (intra coding).
 */
Void TComPrediction::xPredIntraPlanar( Int* pSrc, Int srcStride, Pel* rpDst, Int dstStride, UInt width, UInt height )
{
  assert(width == height);

  Int k, l, bottomLeft, topRight;
  Int horPred;
  Int leftColumn[MAX_CU_SIZE], topRow[MAX_CU_SIZE], bottomRow[MAX_CU_SIZE], rightColumn[MAX_CU_SIZE];
  UInt blkSize = width;
  UInt offset2D = width;
  UInt shift1D = g_aucConvertToBit[ width ] + 2;
  UInt shift2D = shift1D + 1;

  // Get left and above reference column and row
  for(k=0;k<blkSize+1;k++)
  {
    topRow[k] = pSrc[k-srcStride];
    leftColumn[k] = pSrc[k*srcStride-1];
  }

  // Prepare intermediate variables used in interpolation
  bottomLeft = leftColumn[blkSize];
  topRight   = topRow[blkSize];
  for (k=0;k<blkSize;k++)
  {
    bottomRow[k]   = bottomLeft - topRow[k];
    rightColumn[k] = topRight   - leftColumn[k];
    topRow[k]      <<= shift1D;
    leftColumn[k]  <<= shift1D;
  }

  // Generate prediction signal
  for (k=0;k<blkSize;k++)
  {
    horPred = leftColumn[k] + offset2D;
    for (l=0;l<blkSize;l++)
    {
      horPred += rightColumn[k];
      topRow[l] += bottomRow[l];
      rpDst[k*dstStride+l] = ( (horPred + topRow[l]) >> shift2D );
    }
  }
}

/** Function for deriving chroma LM intra prediction.
 * \param pcPattern pointer to neighbouring pixel access pattern
 * \param piSrc pointer to reconstructed chroma sample array
 * \param pPred pointer for the prediction sample array
 * \param uiPredStride the stride of the prediction sample array
 * \param uiCWidth the width of the chroma block
 * \param uiCHeight the height of the chroma block
 * \param uiChromaId boolean indication of chroma component
 *
 * This function derives the prediction samples for chroma LM mode (chroma intra coding)
 */
Void TComPrediction::predLMIntraChroma( TComPattern* pcPattern, Int* piSrc, Pel* pPred, UInt uiPredStride, UInt uiCWidth, UInt uiCHeight, UInt uiChromaId )
{
  UInt uiWidth  = 2 * uiCWidth;

  xGetLLSPrediction( pcPattern, piSrc+uiWidth+2, uiWidth+1, pPred, uiPredStride, uiCWidth, uiCHeight, 1 );  
}

/** Function for deriving downsampled luma sample of current chroma block and its above, left causal pixel
 * \param pcPattern pointer to neighbouring pixel access pattern
 * \param uiCWidth the width of the chroma block
 * \param uiCHeight the height of the chroma block
 *
 * This function derives downsampled luma sample of current chroma block and its above, left causal pixel
 */
Void TComPrediction::getLumaRecPixels( TComPattern* pcPattern, UInt uiCWidth, UInt uiCHeight )
{
  UInt uiWidth  = 2 * uiCWidth;
  UInt uiHeight = 2 * uiCHeight;  

  Pel* pRecSrc = pcPattern->getROIY();
  Pel* pDst0 = m_pLumaRecBuffer + m_iLumaRecStride + 1;

  Int iRecSrcStride = pcPattern->getPatternLStride();
  Int iRecSrcStride2 = iRecSrcStride << 1;
  Int iDstStride = m_iLumaRecStride;
  Int iSrcStride = ( max( uiWidth, uiHeight ) << 1 ) + 1;

  Int* ptrSrc = pcPattern->getAdiOrgBuf( uiWidth, uiHeight, m_piYuvExt );

  // initial pointers
  Pel* pDst = pDst0 - 1 - iDstStride;  
  Int* piSrc = ptrSrc;

  // top left corner downsampled from ADI buffer
  // don't need this point

  // top row downsampled from ADI buffer
  pDst++;     
  piSrc ++;
  for (Int i = 0; i < uiCWidth; i++)
  {
    pDst[i] = ((piSrc[2*i] * 2 ) + piSrc[2*i - 1] + piSrc[2*i + 1] + 2) >> 2;
  }

  // left column downsampled from ADI buffer
  pDst = pDst0 - 1; 
  piSrc = ptrSrc + iSrcStride;
  for (Int j = 0; j < uiCHeight; j++)
  {
    pDst[0] = ( piSrc[0] + piSrc[iSrcStride] ) >> 1;
    piSrc += iSrcStride << 1; 
    pDst += iDstStride;    
  }

  // inner part from reconstructed picture buffer
  for( Int j = 0; j < uiCHeight; j++ )
  {
    for (Int i = 0; i < uiCWidth; i++)
    {
      pDst0[i] = (pRecSrc[2*i] + pRecSrc[2*i + iRecSrcStride]) >> 1;
    }

    pDst0 += iDstStride;
    pRecSrc += iRecSrcStride2;
  }
}

/** Function for deriving the positon of first non-zero binary bit of a value
 * \param x input value
 *
 * This function derives the positon of first non-zero binary bit of a value
 */
Int GetMSB( UInt x )
{
  Int iMSB = 0, bits = ( sizeof( Int ) << 3 ), y = 1;

  while( x > 1 )
  {
    bits >>= 1;
    y = x >> bits;

    if( y )
    {
      x = y;
      iMSB += bits;
    }
  }

  iMSB+=y;

  return iMSB;
}

/** Function for counting leading number of zeros/ones
 * \param x input value
 \ This function counts leading number of zeros for positive numbers and
 \ leading number of ones for negative numbers. This can be implemented in
 \ single instructure cycle on many processors.
 */

Short CountLeadingZerosOnes (Short x)
{
  Short clz;
  Short i;

  if(x == 0)
  {
    clz = 0;
  }
  else
  {
    if (x == -1)
    {
      clz = 15;
    }
    else
    {
      if(x < 0)
      {
        x = ~x;
      }
      clz = 15;
      for(i = 0;i < 15;++i)
      {
        if(x) 
        {
          clz --;
        }
        x = x >> 1;
      }
    }
  }
  return clz;
}

/** Function for deriving LM intra prediction.
 * \param pcPattern pointer to neighbouring pixel access pattern
 * \param pSrc0 pointer to reconstructed chroma sample array
 * \param iSrcStride the stride of reconstructed chroma sample array
 * \param pDst0 reference to pointer for the prediction sample array
 * \param iDstStride the stride of the prediction sample array
 * \param uiWidth the width of the chroma block
 * \param uiHeight the height of the chroma block
 * \param uiExt0 line number of neiggboirng pixels for calculating LM model parameter, default value is 1
 *
 * This function derives the prediction samples for chroma LM mode (chroma intra coding)
 */
Void TComPrediction::xGetLLSPrediction( TComPattern* pcPattern, Int* pSrc0, Int iSrcStride, Pel* pDst0, Int iDstStride, UInt uiWidth, UInt uiHeight, UInt uiExt0 )
{

  Pel  *pDst, *pLuma;
  Int  *pSrc;

  Int  iLumaStride = m_iLumaRecStride;
  Pel* pLuma0 = m_pLumaRecBuffer + uiExt0 * iLumaStride + uiExt0;

  Int i, j, iCountShift = 0;

  UInt uiExt = uiExt0;

  // LLS parameters estimation -->

  Int x = 0, y = 0, xx = 0, xy = 0;

  pSrc  = pSrc0  - iSrcStride;
  pLuma = pLuma0 - iLumaStride;

  for( j = 0; j < uiWidth; j++ )
  {
    x += pLuma[j];
    y += pSrc[j];
    xx += pLuma[j] * pLuma[j];
    xy += pLuma[j] * pSrc[j];
  }
  iCountShift += g_aucConvertToBit[ uiWidth ] + 2;

  pSrc  = pSrc0 - uiExt;
  pLuma = pLuma0 - uiExt;

  for( i = 0; i < uiHeight; i++ )
  {
    x += pLuma[0];
    y += pSrc[0];
    xx += pLuma[0] * pLuma[0];
    xy += pLuma[0] * pSrc[0];

    pSrc  += iSrcStride;
    pLuma += iLumaStride;
  }
  iCountShift += iCountShift > 0 ? 1 : ( g_aucConvertToBit[ uiWidth ] + 2 );

  Int iTempShift = ( g_uiBitDepth + g_uiBitIncrement ) + g_aucConvertToBit[ uiWidth ] + 3 - 15;

  if(iTempShift > 0)
  {
    x  = ( x +  ( 1 << ( iTempShift - 1 ) ) ) >> iTempShift;
    y  = ( y +  ( 1 << ( iTempShift - 1 ) ) ) >> iTempShift;
    xx = ( xx + ( 1 << ( iTempShift - 1 ) ) ) >> iTempShift;
    xy = ( xy + ( 1 << ( iTempShift - 1 ) ) ) >> iTempShift;
    iCountShift -= iTempShift;
  }

  Int a, b, iShift = 13;

  if( iCountShift == 0 )
  {
    a = 0;
    b = 1 << (g_uiBitDepth + g_uiBitIncrement - 1);
    iShift = 0;
  }
  else
  {
    Int a1 = ( xy << iCountShift ) - y * x;
    Int a2 = ( xx << iCountShift ) - x * x;              

    {
      const Int iShiftA2 = 6;
      const Int iShiftA1 = 15;
      const Int iAccuracyShift = 15;

      Int iScaleShiftA2 = 0;
      Int iScaleShiftA1 = 0;
      Int a1s = a1;
      Int a2s = a2;

      iScaleShiftA1 = GetMSB( abs( a1 ) ) - iShiftA1;
      iScaleShiftA2 = GetMSB( abs( a2 ) ) - iShiftA2;  

      if( iScaleShiftA1 < 0 )
      {
        iScaleShiftA1 = 0;
      }
      
      if( iScaleShiftA2 < 0 )
      {
        iScaleShiftA2 = 0;
      }
      
      Int iScaleShiftA = iScaleShiftA2 + iAccuracyShift - iShift - iScaleShiftA1;

      a2s = a2 >> iScaleShiftA2;

      a1s = a1 >> iScaleShiftA1;

      if (a2s >= 1)
      {
        a = a1s * m_uiaShift[ a2s - 1];
      }
      else
      {
        a = 0;
      }
      
      if( iScaleShiftA < 0 )
      {
        a = a << -iScaleShiftA;
      }
      else
      {
        a = a >> iScaleShiftA;
      }
      
       a = Clip3(-( 1 << 15 ), ( 1 << 15 ) - 1, a); 
     
      Int minA = -(1 << (6));
      Int maxA = (1 << 6) - 1;
      if( a <= maxA && a >= minA )
      {
        // do nothing
      }
      else
      {
        Short n = CountLeadingZerosOnes(a);
        a = a >> (9-n);
        iShift -= (9-n);
      }

      b = (  y - ( ( a * x ) >> iShift ) + ( 1 << ( iCountShift - 1 ) ) ) >> iCountShift;
    }
  }   

  // <-- end of LLS parameters estimation

  // get prediction -->
  uiExt = uiExt0;
  pLuma = pLuma0;
  pDst = pDst0;

  for( i = 0; i < uiHeight; i++ )
  {
    for( j = 0; j < uiWidth; j++ )
    {
      pDst[j] = Clip( ( ( a * pLuma[j] ) >> iShift ) + b );
    }
    
    pDst  += iDstStride;
    pLuma += iLumaStride;
  }
  // <-- end of get prediction

}

/** Function for filtering intra DC predictor.
 * \param pSrc pointer to reconstructed sample array
 * \param iSrcStride the stride of the reconstructed sample array
 * \param rpDst reference to pointer for the prediction sample array
 * \param iDstStride the stride of the prediction sample array
 * \param iWidth the width of the block
 * \param iHeight the height of the block
 *
 * This function performs filtering left and top edges of the prediction samples for DC mode (intra coding).
 */
Void TComPrediction::xDCPredFiltering( Int* pSrc, Int iSrcStride, Pel*& rpDst, Int iDstStride, Int iWidth, Int iHeight )
{
  Pel* pDst = rpDst;
  Int x, y, iDstStride2, iSrcStride2;

  // boundary pixels processing
  pDst[0] = (Pel)((pSrc[-iSrcStride] + pSrc[-1] + 2 * pDst[0] + 2) >> 2);

  for ( x = 1; x < iWidth; x++ )
  {
    pDst[x] = (Pel)((pSrc[x - iSrcStride] +  3 * pDst[x] + 2) >> 2);
  }

  for ( y = 1, iDstStride2 = iDstStride, iSrcStride2 = iSrcStride-1; y < iHeight; y++, iDstStride2+=iDstStride, iSrcStride2+=iSrcStride )
  {
    pDst[iDstStride2] = (Pel)((pSrc[iSrcStride2] + 3 * pDst[iDstStride2] + 2) >> 2);
  }

  return;
}

#if HHI_DMM_WEDGE_INTRA || HHI_DMM_PRED_TEX
Void TComPrediction::predIntraLumaDMM( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiMode, Pel* piPred, UInt uiStride, Int iWidth, Int iHeight, Bool bAbove, Bool bLeft, Bool bEncoder )
{
#if HHI_DMM_WEDGE_INTRA
  if( uiMode == DMM_WEDGE_FULL_IDX        ) { xPredIntraWedgeFull ( pcCU, uiAbsPartIdx, piPred, uiStride, iWidth, iHeight, bAbove, bLeft, bEncoder, false, pcCU->getWedgeFullTabIdx ( uiAbsPartIdx ) ); }
  if( uiMode == DMM_WEDGE_FULL_D_IDX      ) { xPredIntraWedgeFull ( pcCU, uiAbsPartIdx, piPred, uiStride, iWidth, iHeight, bAbove, bLeft, bEncoder, true,  pcCU->getWedgeFullTabIdx( uiAbsPartIdx ), pcCU->getWedgeFullDeltaDC1( uiAbsPartIdx ), pcCU->getWedgeFullDeltaDC2( uiAbsPartIdx ) ); }
  if( uiMode == DMM_WEDGE_PREDDIR_IDX     ) { xPredIntraWedgeDir  ( pcCU, uiAbsPartIdx, piPred, uiStride, iWidth, iHeight, bAbove, bLeft, bEncoder, false, pcCU->getWedgePredDirDeltaEnd( uiAbsPartIdx ) ); }
  if( uiMode == DMM_WEDGE_PREDDIR_D_IDX   ) { xPredIntraWedgeDir  ( pcCU, uiAbsPartIdx, piPred, uiStride, iWidth, iHeight, bAbove, bLeft, bEncoder, true,  pcCU->getWedgePredDirDeltaEnd( uiAbsPartIdx ), pcCU->getWedgePredDirDeltaDC1( uiAbsPartIdx ), pcCU->getWedgePredDirDeltaDC2( uiAbsPartIdx ) ); }
#endif
#if HHI_DMM_PRED_TEX
  if( uiMode == DMM_WEDGE_PREDTEX_IDX     ) { xPredIntraWedgeTex  ( pcCU, uiAbsPartIdx, piPred, uiStride, iWidth, iHeight, bAbove, bLeft, bEncoder, false ); }
  if( uiMode == DMM_WEDGE_PREDTEX_D_IDX   ) { xPredIntraWedgeTex  ( pcCU, uiAbsPartIdx, piPred, uiStride, iWidth, iHeight, bAbove, bLeft, bEncoder, true, pcCU->getWedgePredTexDeltaDC1( uiAbsPartIdx ), pcCU->getWedgePredTexDeltaDC2( uiAbsPartIdx ) ); }
  if( uiMode == DMM_CONTOUR_PREDTEX_IDX   ) { xPredIntraContourTex( pcCU, uiAbsPartIdx, piPred, uiStride, iWidth, iHeight, bAbove, bLeft, bEncoder, false ); }
  if( uiMode == DMM_CONTOUR_PREDTEX_D_IDX ) { xPredIntraContourTex( pcCU, uiAbsPartIdx, piPred, uiStride, iWidth, iHeight, bAbove, bLeft, bEncoder, true, pcCU->getContourPredTexDeltaDC1( uiAbsPartIdx ), pcCU->getContourPredTexDeltaDC2( uiAbsPartIdx ) ); }
#endif
}

Void TComPrediction::getWedgePredDCs( TComWedgelet* pcWedgelet, Int* piMask, Int iMaskStride, Int& riPredDC1, Int& riPredDC2, Bool bAbove, Bool bLeft )
{
  riPredDC1 = ( 1<<( g_uiBitDepth + g_uiBitIncrement - 1) ); //pred val, if no neighbors are available
  riPredDC2 = ( 1<<( g_uiBitDepth + g_uiBitIncrement - 1) );

  if( !bAbove && !bLeft ) { return; }

  UInt uiNumSmpDC1 = 0, uiNumSmpDC2 = 0;
  Int iPredDC1 = 0, iPredDC2 = 0;

  Bool* pabWedgePattern = pcWedgelet->getPattern();
  UInt  uiWedgeStride   = pcWedgelet->getStride();

  if( bAbove )
  {
    for( Int k = 0; k < pcWedgelet->getWidth(); k++ )
    {
      if( true == pabWedgePattern[k] )
      {
        iPredDC2 += piMask[k-iMaskStride];
        uiNumSmpDC2++;
      }
      else
      {
        iPredDC1 += piMask[k-iMaskStride];
        uiNumSmpDC1++;
      }
    }
  }
  if( bLeft )
  {
    for( Int k = 0; k < pcWedgelet->getHeight(); k++ )
    {
      if( true == pabWedgePattern[k*uiWedgeStride] )
      {
        iPredDC2 += piMask[k*iMaskStride-1];
        uiNumSmpDC2++;
      } 
      else
      {
        iPredDC1 += piMask[k*iMaskStride-1];
        uiNumSmpDC1++;
      }
    }
  }

  if( uiNumSmpDC1 > 0 )
  {
    iPredDC1 /= uiNumSmpDC1;
    riPredDC1 = iPredDC1;
  }
  if( uiNumSmpDC2 > 0 )
  {
    iPredDC2 /= uiNumSmpDC2;
    riPredDC2 = iPredDC2;
  }
}

Void TComPrediction::calcWedgeDCs( TComWedgelet* pcWedgelet, Pel* piOrig, UInt uiStride, Int& riDC1, Int& riDC2 )
{
  UInt uiDC1 = 0;
  UInt uiDC2 = 0;
  UInt uiNumPixDC1 = 0, uiNumPixDC2 = 0;
  Bool* pabWedgePattern = pcWedgelet->getPattern();
  if( uiStride == pcWedgelet->getStride() )
  {
    for( UInt k = 0; k < (pcWedgelet->getWidth() * pcWedgelet->getHeight()); k++ )
    {
      if( true == pabWedgePattern[k] ) 
      {
        uiDC2 += piOrig[k];
        uiNumPixDC2++;
      }
      else
      {
        uiDC1 += piOrig[k];
        uiNumPixDC1++;
      }
    }
  }
  else
  {
    Pel* piTemp = piOrig;
    UInt uiWedgeStride = pcWedgelet->getStride();
    for( UInt uiY = 0; uiY < pcWedgelet->getHeight(); uiY++ )
    {
      for( UInt uiX = 0; uiX < pcWedgelet->getWidth(); uiX++ )
      {
        if( true == pabWedgePattern[uiX] ) 
        {
          uiDC2 += piTemp[uiX];
          uiNumPixDC2++;
        }
        else
        {
          uiDC1 += piTemp[uiX];
          uiNumPixDC1++;
        }
      }
      piTemp          += uiStride;
      pabWedgePattern += uiWedgeStride;
    }
  }

  if( uiNumPixDC1 > 0 ) { riDC1 = uiDC1 / uiNumPixDC1; }
  else                  { riDC1 = ( 1<<( g_uiBitDepth + g_uiBitIncrement - 1) ); }

  if( uiNumPixDC2 > 0 ) { riDC2 = uiDC2 / uiNumPixDC2; }
  else                  { riDC2 = ( 1<<( g_uiBitDepth + g_uiBitIncrement - 1) ); }
}

Void TComPrediction::assignWedgeDCs2Pred( TComWedgelet* pcWedgelet, Pel* piPred, UInt uiStride, Int iDC1, Int iDC2 )
{
  Bool* pabWedgePattern = pcWedgelet->getPattern();

  if( uiStride == pcWedgelet->getStride() )
  {
    for( UInt k = 0; k < (pcWedgelet->getWidth() * pcWedgelet->getHeight()); k++ )
    {
      if( true == pabWedgePattern[k] ) 
      {
        piPred[k] = iDC2;
      }
      else
      {
        piPred[k] = iDC1;
      }
    }
  }
  else
  {
    Pel* piTemp = piPred;
    UInt uiWedgeStride = pcWedgelet->getStride();
    for( UInt uiY = 0; uiY < pcWedgelet->getHeight(); uiY++ )
    {
      for( UInt uiX = 0; uiX < pcWedgelet->getWidth(); uiX++ )
      {
        if( true == pabWedgePattern[uiX] ) 
        {
          piTemp[uiX] = iDC2;
        }
        else
        {
          piTemp[uiX] = iDC1;
        }
      }
      piTemp          += uiStride;
      pabWedgePattern += uiWedgeStride;
    }
  }
}

Void TComPrediction::xDeltaDCQuantScaleUp( TComDataCU* pcCU, Int& riDeltaDC )
{
  Int  iSign  = riDeltaDC < 0 ? -1 : 1;
  UInt uiAbs  = abs( riDeltaDC );

  Int iQp = pcCU->getQP(0);
  Double dMax = (Double)( 1<<( g_uiBitDepth + g_uiBitIncrement - 1) );
  Double dStepSize = Clip3( 1.0, dMax, pow( 2.0, iQp/10.0 + g_iDeltaDCsQuantOffset ) );

  riDeltaDC = iSign * roftoi( uiAbs * dStepSize );
  return;
}

Void TComPrediction::xDeltaDCQuantScaleDown( TComDataCU*  pcCU, Int& riDeltaDC )
{
  Int  iSign  = riDeltaDC < 0 ? -1 : 1;
  UInt uiAbs  = abs( riDeltaDC );

  Int iQp = pcCU->getQP(0);
  Double dMax = (Double)( 1<<( g_uiBitDepth + g_uiBitIncrement - 1) );
  Double dStepSize = Clip3( 1.0, dMax, pow( 2.0, iQp/10.0 + g_iDeltaDCsQuantOffset ) );

  riDeltaDC = iSign * roftoi( uiAbs / dStepSize );
  return;
}
#endif

#if HHI_DMM_PRED_TEX
Void TComPrediction::getBestContourFromTex( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiWidth, UInt uiHeight, TComWedgelet* pcContourWedge )
{
  pcContourWedge->clear();

  // get copy of co-located texture luma block
  TComYuv cTempYuv;
  cTempYuv.create( uiWidth, uiHeight ); 
  cTempYuv.clear();
  Pel* piRefBlkY = cTempYuv.getLumaAddr();
  copyTextureLumaBlock( pcCU, uiAbsPartIdx, piRefBlkY, uiWidth, uiHeight );
  piRefBlkY = cTempYuv.getLumaAddr();

  // find contour for texture luma block
  UInt iDC = 0;
  for( UInt k = 0; k < (uiWidth*uiHeight); k++ ) 
  { 
    iDC += piRefBlkY[k]; 
  }
  iDC /= (uiWidth*uiHeight);
  piRefBlkY = cTempYuv.getLumaAddr();

  Bool* pabContourPattern = pcContourWedge->getPattern();
  for( UInt k = 0; k < (uiWidth*uiHeight); k++ ) 
  { 
    pabContourPattern[k] = (piRefBlkY[k] > iDC) ? true : false;
  }

  cTempYuv.destroy();
}

UInt TComPrediction::getBestWedgeFromTex( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiWidth, UInt uiHeight )
{
  assert( uiWidth >= DMM_WEDGEMODEL_MIN_SIZE && uiWidth <= DMM_WEDGEMODEL_MAX_SIZE );
  WedgeList* pacWedgeList = &g_aacWedgeLists[(g_aucConvertToBit[uiWidth])];

  // get copy of co-located texture luma block
  TComYuv cTempYuv; 
  cTempYuv.create( uiWidth, uiHeight ); 
  cTempYuv.clear();
  Pel* piRefBlkY = cTempYuv.getLumaAddr();

  copyTextureLumaBlock( pcCU, uiAbsPartIdx, piRefBlkY, uiWidth, uiHeight );
  piRefBlkY = cTempYuv.getLumaAddr();

  // local pred buffer
  TComYuv cPredYuv; 
  cPredYuv.create( uiWidth, uiHeight ); 
  cPredYuv.clear();
  Pel* piPred = cPredYuv.getLumaAddr();

  UInt uiPredStride = cPredYuv.getStride();

  // regular wedge search
  TComWedgeDist cWedgeDist;
  UInt uiBestDist = MAX_UINT;
  UInt uiBestTabIdx = 0;
  Int  iDC1 = 0;
  Int  iDC2 = 0;

  for( UInt uiIdx = 0; uiIdx < pacWedgeList->size(); uiIdx++ )
  {
    calcWedgeDCs       ( &(pacWedgeList->at(uiIdx)), piRefBlkY, uiWidth,      iDC1, iDC2 );
    assignWedgeDCs2Pred( &(pacWedgeList->at(uiIdx)), piPred,    uiPredStride, iDC1, iDC2 );

    UInt uiActDist = cWedgeDist.getDistPart( piPred, uiPredStride, piRefBlkY, uiWidth, uiWidth, uiHeight, WedgeDist_SAD );

    if( uiActDist < uiBestDist || uiBestDist == MAX_UINT )
    {
      uiBestDist   = uiActDist;
      uiBestTabIdx = uiIdx;
    }
  }

  cPredYuv.destroy();
  cTempYuv.destroy();
  return uiBestTabIdx;
}

Void TComPrediction::copyTextureLumaBlock( TComDataCU* pcCU, UInt uiAbsPartIdx, Pel* piDestBlockY, UInt uiWidth, UInt uiHeight )
{
  TComPicYuv* pcPicYuvRef = pcCU->getSlice()->getTexturePic()->getPicYuvRec();
  Int         iRefStride = pcPicYuvRef->getStride();
  Pel*        piRefY;

  piRefY = pcPicYuvRef->getLumaAddr( pcCU->getAddr(), pcCU->getZorderIdxInCU() + uiAbsPartIdx );

  for ( Int y = 0; y < uiHeight; y++ )
  {
    ::memcpy(piDestBlockY, piRefY, sizeof(Pel)*uiWidth);
//    ::memset(piDestBlockY, 128, sizeof(Pel)*uiWidth);
    piDestBlockY += uiWidth;
    piRefY += iRefStride;
  }
}

Void TComPrediction::xPredIntraWedgeTex( TComDataCU* pcCU, UInt uiAbsPartIdx, Pel* piPred, UInt uiStride, Int iWidth, Int iHeight, Bool bAbove, Bool bLeft, Bool bEncoder, Bool bDelta, Int iDeltaDC1, Int iDeltaDC2 )
{
  assert( iWidth >= DMM_WEDGEMODEL_MIN_SIZE && iWidth <= DMM_WEDGEMODEL_MAX_SIZE );
  WedgeList* pacWedgeList = &g_aacWedgeLists[(g_aucConvertToBit[iWidth])];

  // get wedge pattern
  UInt uiTextureWedgeTabIdx = 0;
  if( bEncoder ) 
  {
    // encoder: load stored wedge pattern from CU
    uiTextureWedgeTabIdx = pcCU->getWedgePredTexTabIdx( uiAbsPartIdx );
  }
  else
  {
    // decoder: get and store wedge pattern in CU
    uiTextureWedgeTabIdx = getBestWedgeFromTex( pcCU, uiAbsPartIdx, (UInt)iWidth, (UInt)iHeight );

    UInt uiDepth = (pcCU->getDepth(0)) + (pcCU->getPartitionSize(0) == SIZE_2Nx2N ? 0 : 1);
    pcCU->setWedgePredTexTabIdxSubParts( uiTextureWedgeTabIdx, uiAbsPartIdx, uiDepth );
  }
  TComWedgelet* pcWedgelet = &(pacWedgeList->at(uiTextureWedgeTabIdx));

  // get wedge pred DCs
  Int iPredDC1 = 0;
  Int iPredDC2 = 0;
  Int* piMask = pcCU->getPattern()->getAdiOrgBuf( iWidth, iHeight, m_piYuvExt );
  Int iMaskStride = ( iWidth<<1 ) + 1;
  piMask += iMaskStride+1;
  getWedgePredDCs( pcWedgelet, piMask, iMaskStride, iPredDC1, iPredDC2, bAbove, bLeft );

  if( bDelta ) 
  {
    xDeltaDCQuantScaleUp( pcCU, iDeltaDC1 );
    xDeltaDCQuantScaleUp( pcCU, iDeltaDC2 );
  }

  // assign wedge pred DCs to prediction
  if( bDelta ) { assignWedgeDCs2Pred( pcWedgelet, piPred, uiStride, Clip ( iPredDC1+iDeltaDC1 ), Clip( iPredDC2+iDeltaDC2 ) ); }
  else         { assignWedgeDCs2Pred( pcWedgelet, piPred, uiStride,        iPredDC1,                   iPredDC2           ); }
}

Void TComPrediction::xPredIntraContourTex( TComDataCU* pcCU, UInt uiAbsPartIdx, Pel* piPred, UInt uiStride, Int iWidth, Int iHeight, Bool bAbove, Bool bLeft, Bool bEncoder, Bool bDelta, Int iDeltaDC1, Int iDeltaDC2 )
{
  // get contour pattern
  TComWedgelet* pcContourWedge = new TComWedgelet( iWidth, iHeight );
  getBestContourFromTex( pcCU, uiAbsPartIdx, (UInt)iWidth, (UInt)iHeight, pcContourWedge );

  // get wedge pred DCs
  Int iPredDC1 = 0;
  Int iPredDC2 = 0;
  Int* piMask = pcCU->getPattern()->getAdiOrgBuf( iWidth, iHeight, m_piYuvExt );
  Int iMaskStride = ( iWidth<<1 ) + 1;
  piMask += iMaskStride+1;
  getWedgePredDCs( pcContourWedge, piMask, iMaskStride, iPredDC1, iPredDC2, bAbove, bLeft );

  if( bDelta ) 
  {
    xDeltaDCQuantScaleUp( pcCU, iDeltaDC1 );
    xDeltaDCQuantScaleUp( pcCU, iDeltaDC2 );
  }

  // assign wedge pred DCs to prediction
  if( bDelta ) { assignWedgeDCs2Pred( pcContourWedge, piPred, uiStride, Clip ( iPredDC1+iDeltaDC1 ), Clip( iPredDC2+iDeltaDC2 ) ); }
  else         { assignWedgeDCs2Pred( pcContourWedge, piPred, uiStride,        iPredDC1,                   iPredDC2           ); }

  pcContourWedge->destroy();
  delete pcContourWedge;
}
#endif

#if HHI_DMM_WEDGE_INTRA
UInt TComPrediction::getBestContinueWedge( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiWidth, UInt uiHeight, Int iDeltaEnd )
{
  UInt uiThisBlockSize = uiWidth;
  assert( uiThisBlockSize >= DMM_WEDGEMODEL_MIN_SIZE && uiThisBlockSize <= DMM_WEDGEMODEL_MAX_SIZE );
  WedgeRefList* pacContDWedgeRefList = &g_aacWedgeRefLists[(g_aucConvertToBit[uiThisBlockSize])];

  UInt uiPredDirWedgeTabIdx = 0;
  TComDataCU* pcTempCU;
  UInt        uiTempPartIdx;
  // 1st: try continue above wedgelet
  pcTempCU = pcCU->getPUAbove( uiTempPartIdx, pcCU->getZorderIdxInCU() + uiAbsPartIdx );
  if( pcTempCU )
  {
    UChar uhLumaIntraDir = pcTempCU->getLumaIntraDir( uiTempPartIdx );
    if( DMM_WEDGE_FULL_IDX      == uhLumaIntraDir || 
        DMM_WEDGE_FULL_D_IDX    == uhLumaIntraDir || 
        DMM_WEDGE_PREDDIR_IDX   == uhLumaIntraDir || 
        DMM_WEDGE_PREDDIR_D_IDX == uhLumaIntraDir
#if HHI_DMM_PRED_TEX
        ||
        DMM_WEDGE_PREDTEX_IDX   == uhLumaIntraDir ||
        DMM_WEDGE_PREDTEX_D_IDX == uhLumaIntraDir    
#endif
      )
    {
      UInt uiRefWedgeSize = (UInt)g_aucIntraSizeIdxToWedgeSize[pcTempCU->getIntraSizeIdx( uiTempPartIdx )];
      WedgeList* pacWedgeList = &g_aacWedgeLists[(g_aucConvertToBit[uiRefWedgeSize])];

      // get offset between current and reference block
      UInt uiOffsetX = 0;
      UInt uiOffsetY = 0;
      xGetBlockOffset( pcCU, uiAbsPartIdx, pcTempCU, uiTempPartIdx, uiOffsetX, uiOffsetY );

      // get reference wedgelet
      UInt uiRefWedgeTabIdx = 0;
      switch( uhLumaIntraDir )
      {
      case( DMM_WEDGE_FULL_IDX      ): { uiRefWedgeTabIdx = pcTempCU->getWedgeFullTabIdx   ( uiTempPartIdx ); } break;
      case( DMM_WEDGE_FULL_D_IDX    ): { uiRefWedgeTabIdx = pcTempCU->getWedgeFullTabIdx   ( uiTempPartIdx ); } break;
      case( DMM_WEDGE_PREDDIR_IDX   ): { uiRefWedgeTabIdx = pcTempCU->getWedgePredDirTabIdx( uiTempPartIdx ); } break;
      case( DMM_WEDGE_PREDDIR_D_IDX ): { uiRefWedgeTabIdx = pcTempCU->getWedgePredDirTabIdx( uiTempPartIdx ); } break;
#if HHI_DMM_PRED_TEX
      case( DMM_WEDGE_PREDTEX_IDX   ): { uiRefWedgeTabIdx = pcTempCU->getWedgePredTexTabIdx( uiTempPartIdx ); } break;
      case( DMM_WEDGE_PREDTEX_D_IDX ): { uiRefWedgeTabIdx = pcTempCU->getWedgePredTexTabIdx( uiTempPartIdx ); } break;
#endif
      default: { assert( 0 ); return uiPredDirWedgeTabIdx; }
      }
      TComWedgelet* pcRefWedgelet;
      pcRefWedgelet = &(pacWedgeList->at( uiRefWedgeTabIdx ));

      // find reference wedgelet, if direction is suitable for continue wedge
      if( pcRefWedgelet->checkPredDirAbovePossible( uiThisBlockSize, uiOffsetX ) )
      {
        UChar uhContD_Xs, uhContD_Ys, uhContD_Xe, uhContD_Ye;
        pcRefWedgelet->getPredDirStartEndAbove( uhContD_Xs, uhContD_Ys, uhContD_Xe, uhContD_Ye, uiThisBlockSize, uiOffsetX, iDeltaEnd );
        getWedgePatternIdx( pacContDWedgeRefList, uiPredDirWedgeTabIdx, uhContD_Xs, uhContD_Ys, uhContD_Xe, uhContD_Ye );
        return uiPredDirWedgeTabIdx;
      }
    }
  }

  // 2nd: try continue left wedglelet
  pcTempCU = pcCU->getPULeft( uiTempPartIdx, pcCU->getZorderIdxInCU() + uiAbsPartIdx );
  if( pcTempCU )
  {
    UChar uhLumaIntraDir = pcTempCU->getLumaIntraDir( uiTempPartIdx );
    if( DMM_WEDGE_FULL_IDX      == uhLumaIntraDir || 
        DMM_WEDGE_FULL_D_IDX    == uhLumaIntraDir || 
        DMM_WEDGE_PREDDIR_IDX   == uhLumaIntraDir || 
        DMM_WEDGE_PREDDIR_D_IDX == uhLumaIntraDir
#if HHI_DMM_PRED_TEX
        ||
        DMM_WEDGE_PREDTEX_IDX   == uhLumaIntraDir ||
        DMM_WEDGE_PREDTEX_D_IDX == uhLumaIntraDir    
#endif
      )
    {
      UInt uiRefWedgeSize = (UInt)g_aucIntraSizeIdxToWedgeSize[pcTempCU->getIntraSizeIdx( uiTempPartIdx )];
      WedgeList* pacWedgeList = &g_aacWedgeLists[(g_aucConvertToBit[uiRefWedgeSize])];

      // get offset between current and reference block
      UInt uiOffsetX = 0;
      UInt uiOffsetY = 0;
      xGetBlockOffset( pcCU, uiAbsPartIdx, pcTempCU, uiTempPartIdx, uiOffsetX, uiOffsetY );

      // get reference wedgelet
      UInt uiRefWedgeTabIdx = 0;
      switch( uhLumaIntraDir )
      {
      case( DMM_WEDGE_FULL_IDX      ): { uiRefWedgeTabIdx = pcTempCU->getWedgeFullTabIdx   ( uiTempPartIdx ); } break;
      case( DMM_WEDGE_FULL_D_IDX    ): { uiRefWedgeTabIdx = pcTempCU->getWedgeFullTabIdx   ( uiTempPartIdx ); } break;
      case( DMM_WEDGE_PREDDIR_IDX   ): { uiRefWedgeTabIdx = pcTempCU->getWedgePredDirTabIdx( uiTempPartIdx ); } break;
      case( DMM_WEDGE_PREDDIR_D_IDX ): { uiRefWedgeTabIdx = pcTempCU->getWedgePredDirTabIdx( uiTempPartIdx ); } break;
#if HHI_DMM_PRED_TEX
      case( DMM_WEDGE_PREDTEX_IDX   ): { uiRefWedgeTabIdx = pcTempCU->getWedgePredTexTabIdx( uiTempPartIdx ); } break;
      case( DMM_WEDGE_PREDTEX_D_IDX ): { uiRefWedgeTabIdx = pcTempCU->getWedgePredTexTabIdx( uiTempPartIdx ); } break;
#endif
      default: { assert( 0 ); return uiPredDirWedgeTabIdx; }
      }
      TComWedgelet* pcRefWedgelet;
      pcRefWedgelet = &(pacWedgeList->at( uiRefWedgeTabIdx ));

      // find reference wedgelet, if direction is suitable for continue wedge
      if( pcRefWedgelet->checkPredDirLeftPossible( uiThisBlockSize, uiOffsetY ) )
      {
        UChar uhContD_Xs, uhContD_Ys, uhContD_Xe, uhContD_Ye;
        pcRefWedgelet->getPredDirStartEndLeft( uhContD_Xs, uhContD_Ys, uhContD_Xe, uhContD_Ye, uiThisBlockSize, uiOffsetY, iDeltaEnd );
        getWedgePatternIdx( pacContDWedgeRefList, uiPredDirWedgeTabIdx, uhContD_Xs, uhContD_Ys, uhContD_Xe, uhContD_Ye );
        return uiPredDirWedgeTabIdx;
      }
    }
  }

  // 3rd: (default) make wedglet from intra dir and max slope point
  Int iSlopeX = 0;
  Int iSlopeY = 0;
  UInt uiStartPosX = 0;
  UInt uiStartPosY = 0;
  if( xGetWedgeIntraDirPredData( pcCU, uiAbsPartIdx, uiThisBlockSize, iSlopeX, iSlopeY, uiStartPosX, uiStartPosY ) )
  {
    UChar uhContD_Xs, uhContD_Ys, uhContD_Xe, uhContD_Ye;
    xGetWedgeIntraDirStartEnd( pcCU, uiAbsPartIdx, uiThisBlockSize, iSlopeX, iSlopeY, uiStartPosX, uiStartPosY, uhContD_Xs, uhContD_Ys, uhContD_Xe, uhContD_Ye, iDeltaEnd );
    getWedgePatternIdx( pacContDWedgeRefList, uiPredDirWedgeTabIdx, uhContD_Xs, uhContD_Ys, uhContD_Xe, uhContD_Ye );
    return uiPredDirWedgeTabIdx;
  }

  return uiPredDirWedgeTabIdx;
}

Bool TComPrediction::getWedgePatternIdx( WedgeRefList* pcWedgeRefList, UInt& ruiTabIdx, UChar uhXs, UChar uhYs, UChar uhXe, UChar uhYe )
{
  ruiTabIdx = 0;

  for( UInt uiIdx = 0; uiIdx < pcWedgeRefList->size(); uiIdx++ )
  {
    TComWedgeRef* pcTestWedgeRef = &(pcWedgeRefList->at(uiIdx));

    if( pcTestWedgeRef->getStartX() == uhXs &&
      pcTestWedgeRef->getStartY() == uhYs &&
      pcTestWedgeRef->getEndX()   == uhXe &&
      pcTestWedgeRef->getEndY()   == uhYe    )
    {
      ruiTabIdx = pcTestWedgeRef->getRefIdx();
      return true;
    }
  }

  return false;
}

Void TComPrediction::xPredIntraWedgeFull( TComDataCU* pcCU, UInt uiAbsPartIdx, Pel* piPred, UInt uiStride, Int iWidth, Int iHeight, Bool bAbove, Bool bLeft, Bool bEncoder, Bool bDelta, UInt uiTabIdx, Int iDeltaDC1, Int iDeltaDC2 )
{
  assert( iWidth >= DMM_WEDGEMODEL_MIN_SIZE && iWidth <= DMM_WEDGEMODEL_MAX_SIZE );
  WedgeList* pacWedgeList = &g_aacWedgeLists[(g_aucConvertToBit[iWidth])];
  TComWedgelet* pcWedgelet = &(pacWedgeList->at(uiTabIdx));

  // get wedge pred DCs
  Int iPredDC1 = 0;
  Int iPredDC2 = 0;

  Int* piMask = pcCU->getPattern()->getAdiOrgBuf( iWidth, iHeight, m_piYuvExt );
  Int iMaskStride = ( iWidth<<1 ) + 1;
  piMask += iMaskStride+1;
  getWedgePredDCs( pcWedgelet, piMask, iMaskStride, iPredDC1, iPredDC2, bAbove, bLeft );

  if( bDelta ) 
  {
    xDeltaDCQuantScaleUp( pcCU, iDeltaDC1 );
    xDeltaDCQuantScaleUp( pcCU, iDeltaDC2 );
  }

  // assign wedge pred DCs to prediction
  if( bDelta ) { assignWedgeDCs2Pred( pcWedgelet, piPred, uiStride, Clip( iPredDC1+iDeltaDC1 ), Clip( iPredDC2+iDeltaDC2 ) ); }
  else         { assignWedgeDCs2Pred( pcWedgelet, piPred, uiStride, iPredDC1,           iPredDC2           ); }
}

Void TComPrediction::xPredIntraWedgeDir( TComDataCU* pcCU, UInt uiAbsPartIdx, Pel* piPred, UInt uiStride, Int iWidth, Int iHeight, Bool bAbove, Bool bLeft, Bool bEncoder, Bool bDelta, Int iWedgeDeltaEnd, Int iDeltaDC1, Int iDeltaDC2 )
{
  assert( iWidth >= DMM_WEDGEMODEL_MIN_SIZE && iWidth <= DMM_WEDGEMODEL_MAX_SIZE );
  WedgeList* pacWedgeList = &g_aacWedgeLists[(g_aucConvertToBit[iWidth])];

  // get wedge pattern
  UInt uiDirWedgeTabIdx = 0;
  if( bEncoder )
  {
    // encoder: load stored wedge pattern from CU
    uiDirWedgeTabIdx = pcCU->getWedgePredDirTabIdx( uiAbsPartIdx );
  }
  else
  {
    uiDirWedgeTabIdx = getBestContinueWedge( pcCU, uiAbsPartIdx, iWidth, iHeight, iWedgeDeltaEnd );

    UInt uiDepth = (pcCU->getDepth(0)) + (pcCU->getPartitionSize(0) == SIZE_2Nx2N ? 0 : 1);
    pcCU->setWedgePredDirTabIdxSubParts( uiDirWedgeTabIdx, uiAbsPartIdx, uiDepth );
  }
  TComWedgelet* pcWedgelet = &(pacWedgeList->at(uiDirWedgeTabIdx));

  // get wedge pred DCs
  Int iPredDC1 = 0;
  Int iPredDC2 = 0;

  Int* piMask = pcCU->getPattern()->getAdiOrgBuf( iWidth, iHeight, m_piYuvExt );
  Int iMaskStride = ( iWidth<<1 ) + 1;
  piMask += iMaskStride+1;
  getWedgePredDCs( pcWedgelet, piMask, iMaskStride, iPredDC1, iPredDC2, bAbove, bLeft );

  if( bDelta ) 
  {
    xDeltaDCQuantScaleUp( pcCU, iDeltaDC1 );
    xDeltaDCQuantScaleUp( pcCU, iDeltaDC2 );
  }

  // assign wedge pred DCs to prediction
  if( bDelta ) { assignWedgeDCs2Pred( pcWedgelet, piPred, uiStride, Clip( iPredDC1+iDeltaDC1 ), Clip( iPredDC2+iDeltaDC2 ) ); }
  else         { assignWedgeDCs2Pred( pcWedgelet, piPred, uiStride,       iPredDC1,                   iPredDC2             ); }
}

Void TComPrediction::xGetBlockOffset( TComDataCU* pcCU, UInt uiAbsPartIdx, TComDataCU* pcRefCU, UInt uiRefAbsPartIdx, UInt& ruiOffsetX, UInt& ruiOffsetY )
{
  ruiOffsetX = 0;
  ruiOffsetY = 0;

  // get offset between current and above/left block
  UInt uiThisOriginX = pcCU->getCUPelX() + g_auiRasterToPelX[ g_auiZscanToRaster[uiAbsPartIdx] ];
  UInt uiThisOriginY = pcCU->getCUPelY() + g_auiRasterToPelY[ g_auiZscanToRaster[uiAbsPartIdx] ];

  UInt uiNumPartInRefCU = pcRefCU->getTotalNumPart();
  UInt uiMaxDepthRefCU = 0;
  while( uiNumPartInRefCU > 1 )
  {
    uiNumPartInRefCU >>= 2;
    uiMaxDepthRefCU++;
  }

  UInt uiDepthRefPU = (pcRefCU->getDepth(uiRefAbsPartIdx)) + (pcRefCU->getPartitionSize(uiRefAbsPartIdx) == SIZE_2Nx2N ? 0 : 1);
  UInt uiShifts = (uiMaxDepthRefCU - uiDepthRefPU)*2;
  UInt uiRefBlockOriginPartIdx = (uiRefAbsPartIdx>>uiShifts)<<uiShifts;

  UInt uiRefOriginX = pcRefCU->getCUPelX() + g_auiRasterToPelX[ g_auiZscanToRaster[uiRefBlockOriginPartIdx] ];
  UInt uiRefOriginY = pcRefCU->getCUPelY() + g_auiRasterToPelY[ g_auiZscanToRaster[uiRefBlockOriginPartIdx] ];

  if( (uiThisOriginX - uiRefOriginX) > 0 ) { ruiOffsetX = (UInt)(uiThisOriginX - uiRefOriginX); }
  if( (uiThisOriginY - uiRefOriginY) > 0 ) { ruiOffsetY = (UInt)(uiThisOriginY - uiRefOriginY); }
}

Bool TComPrediction::xGetWedgeIntraDirPredData( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiBlockSize, Int& riSlopeX, Int& riSlopeY, UInt& ruiStartPosX, UInt& ruiStartPosY )
{
  riSlopeX     = 0;
  riSlopeY     = 0;
  ruiStartPosX = 0;
  ruiStartPosY = 0;

  // 1st step: get wedge start point (max. slope)
  Int* piSource = pcCU->getPattern()->getAdiOrgBuf( uiBlockSize, uiBlockSize, m_piYuvExt );
  Int iSourceStride = ( uiBlockSize<<1 ) + 1;

  UInt uiSlopeMaxAbove = 0;
  UInt uiPosSlopeMaxAbove = 0;
  for( UInt uiPosHor = 0; uiPosHor < (uiBlockSize-1); uiPosHor++ )
  {
    if( abs( piSource[uiPosHor+1] - piSource[uiPosHor] ) > uiSlopeMaxAbove )
    {
      uiSlopeMaxAbove = abs( piSource[uiPosHor+1] - piSource[uiPosHor] );
      uiPosSlopeMaxAbove = uiPosHor;
    }
  }

  UInt uiSlopeMaxLeft = 0;
  UInt uiPosSlopeMaxLeft = 0;
  for( UInt uiPosVer = 0; uiPosVer < (uiBlockSize-1); uiPosVer++ )
  {
    if( abs( piSource[(uiPosVer+1)*iSourceStride] - piSource[uiPosVer*iSourceStride] ) > uiSlopeMaxLeft )
    {
      uiSlopeMaxLeft = abs( piSource[(uiPosVer+1)*iSourceStride] - piSource[uiPosVer*iSourceStride] );
      uiPosSlopeMaxLeft = uiPosVer;
    }
  }

  if( uiSlopeMaxAbove == 0 && uiSlopeMaxLeft == 0 ) 
  { 
    return false; 
  }

  if( uiSlopeMaxAbove > uiSlopeMaxLeft )
  {
    ruiStartPosX = uiPosSlopeMaxAbove;
    ruiStartPosY = 0;
  }
  else
  {
    ruiStartPosX = 0;
    ruiStartPosY = uiPosSlopeMaxLeft;
  }

  // 2nd step: derive wedge direction
#if LOGI_INTRA_NAME_3MPM
  Int uiPreds[3] = {-1, -1, -1};
#else
  Int uiPreds[2] = {-1, -1};
#endif
  Int iMode = -1;
  Int iPredNum = pcCU->getIntraDirLumaPredictor( uiAbsPartIdx, uiPreds, &iMode );  

  UInt uiDirMode = 0;
#if LOGI_INTRA_NAME_3MPM
  if( iMode >= 0 ) { iPredNum = iMode; }
  if( iPredNum == 1 ) { uiDirMode = uiPreds[0]; }
  if( iPredNum == 2 ) { uiDirMode = uiPreds[1]; }

  if( uiDirMode < 2 ) { return false; } // no planar & DC

  Bool modeHor       = (uiDirMode < 18);
  Bool modeVer       = !modeHor;
  Int intraPredAngle = modeVer ? (Int)uiDirMode - VER_IDX : modeHor ? -((Int)uiDirMode - HOR_IDX) : 0;
#else
  if( iPredNum == 1 ) { uiDirMode = g_aucAngIntraModeOrder[uiPreds[0]]; }
  if( iPredNum == 2 ) { uiDirMode = g_aucAngIntraModeOrder[uiPreds[1]]; }

  if( uiDirMode == 0 ) {  return false; } // no DC

  Bool modeVer       = (uiDirMode < 18);
  Bool modeHor       = !modeVer;
  Int intraPredAngle = modeVer ? uiDirMode - 9 : modeHor ? uiDirMode - 25 : 0;
#endif
  Int absAng         = abs(intraPredAngle);
  Int signAng        = intraPredAngle < 0 ? -1 : 1;
  Int angTable[9]    = {0,2,5,9,13,17,21,26,32};
  absAng             = angTable[absAng];
  intraPredAngle     = signAng * absAng;

  // 3rd step: set slope for direction
  if( modeHor )
  {
    if( intraPredAngle > 0 )
    {
      riSlopeX = -32;
      riSlopeY = intraPredAngle;
    }
    else
    {
      riSlopeX = 32;
      riSlopeY = -intraPredAngle;
    }
  }
  else if( modeVer )
  {
    if( intraPredAngle > 0 )
    {
      riSlopeX = intraPredAngle;
      riSlopeY = -32;
    }
    else
    {
      riSlopeX = -intraPredAngle;
      riSlopeY = 32;
    }
  }

  return true;
}

Void TComPrediction::xGetWedgeIntraDirStartEnd( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiBlockSize, Int iDeltaX, Int iDeltaY, UInt uiPMSPosX, UInt uiPMSPosY, UChar& ruhXs, UChar& ruhYs, UChar& ruhXe, UChar& ruhYe, Int iDeltaEnd )
{
  ruhXs = 0;
  ruhYs = 0;
  ruhXe = 0;
  ruhYe = 0;

  // scaling of start pos and block size to wedge resolution
  UInt uiScaledStartPosX = 0;
  UInt uiScaledStartPosY = 0;
  UInt uiScaledBlockSize = 0;
  WedgeResolution eWedgeRes = g_aeWedgeResolutionList[(UInt)g_aucConvertToBit[uiBlockSize]];
  switch( eWedgeRes )
  {
  case( DOUBLE_PEL ): { uiScaledStartPosX = (uiPMSPosX>>1); uiScaledStartPosY = (uiPMSPosY>>1); uiScaledBlockSize = (uiBlockSize>>1); break; }
  case(   FULL_PEL ): { uiScaledStartPosX =  uiPMSPosX;     uiScaledStartPosY =  uiPMSPosY;     uiScaledBlockSize =  uiBlockSize;     break; }
  case(   HALF_PEL ): { uiScaledStartPosX = (uiPMSPosX<<1); uiScaledStartPosY = (uiPMSPosY<<1); uiScaledBlockSize = (uiBlockSize<<1); break; }
  }
  Int iMaxPos = (Int)uiScaledBlockSize - 1;

  // case above
  if( uiScaledStartPosX > 0 && uiScaledStartPosY == 0 )
  {
    ruhXs = (UChar)uiScaledStartPosX;
    ruhYs = 0;

    if( iDeltaY == 0 )
    {
      if( iDeltaX < 0 )
      {
        ruhXe = 0;
        ruhYe = (UChar)std::min( std::max( iDeltaEnd, 0 ), iMaxPos );
        return;
      }
      else
      {
        ruhXe = (UChar)iMaxPos;
        ruhYe = (UChar)std::min( std::max( -iDeltaEnd, 0 ), iMaxPos );
        std::swap( ruhXs, ruhXe );
        std::swap( ruhYs, ruhYe );
        return;
      }
    }

    // regular case
    Int iVirtualEndX = (Int)ruhXs + roftoi( (Double)iMaxPos * ((Double)iDeltaX / (Double)iDeltaY) );

    if( iVirtualEndX < 0 )
    {
      Int iYe = roftoi( (Double)(0 - (Int)ruhXs) * ((Double)iDeltaY / (Double)iDeltaX) ) + iDeltaEnd;
      if( iYe < (Int)uiScaledBlockSize )
      {
        ruhXe = 0;
        ruhYe = (UChar)std::max( iYe, 0 );
        return;
      }
      else
      {
        ruhXe = (UChar)std::min( (iYe - iMaxPos), iMaxPos );
        ruhYe = (UChar)iMaxPos;
        return;
      }
    }
    else if( iVirtualEndX > iMaxPos )
    {
      Int iYe = roftoi( (Double)(iMaxPos - (Int)ruhXs) * ((Double)iDeltaY / (Double)iDeltaX) ) - iDeltaEnd;
      if( iYe < (Int)uiScaledBlockSize )
      {
        ruhXe = (UChar)iMaxPos;
        ruhYe = (UChar)std::max( iYe, 0 );
        std::swap( ruhXs, ruhXe );
        std::swap( ruhYs, ruhYe );
        return;
      }
      else
      {
        ruhXe = (UChar)std::max( (iMaxPos - (iYe - iMaxPos)), 0 );
        ruhYe = (UChar)iMaxPos;
        return;
      }
    }
    else
    {
      Int iXe = iVirtualEndX + iDeltaEnd;
      if( iXe < 0 )
      {
        ruhXe = 0;
        ruhYe = (UChar)std::max( (iMaxPos + iXe), 0 );
        return;
      }
      else if( iXe > iMaxPos )
      {
        ruhXe = (UChar)iMaxPos;
        ruhYe = (UChar)std::max( (iMaxPos - (iXe - iMaxPos)), 0 );
        std::swap( ruhXs, ruhXe );
        std::swap( ruhYs, ruhYe );
        return;
      }
      else
      {
        ruhXe = (UChar)iXe;
        ruhYe = (UChar)iMaxPos;
        return;
      }
    }
  }

  // case left
  if( uiScaledStartPosY > 0 && uiScaledStartPosX == 0 )
  {
    ruhXs = 0;
    ruhYs = (UChar)uiScaledStartPosY;

    if( iDeltaX == 0 )
    {
      if( iDeltaY < 0 )
      {
        ruhXe = (UChar)std::min( std::max( -iDeltaEnd, 0 ), iMaxPos );
        ruhYe = 0;
        std::swap( ruhXs, ruhXe );
        std::swap( ruhYs, ruhYe );
        return;
      }
      else
      {
        ruhXe = (UChar)std::min( std::max( iDeltaEnd, 0 ), iMaxPos );
        ruhYe = (UChar)iMaxPos;
        return; 
      }
    }

    // regular case
    Int iVirtualEndY = (Int)ruhYs + roftoi( (Double)iMaxPos * ((Double)iDeltaY / (Double)iDeltaX) );

    if( iVirtualEndY < 0 )
    {
      Int iXe = roftoi( (Double)(0 - (Int)ruhYs ) * ((Double)iDeltaX / (Double)iDeltaY) ) - iDeltaEnd;
      if( iXe < (Int)uiScaledBlockSize )
      {
        ruhXe = (UChar)std::max( iXe, 0 );
        ruhYe = 0;
        std::swap( ruhXs, ruhXe );
        std::swap( ruhYs, ruhYe );
        return;
      }
      else
      {
        ruhXe = (UChar)iMaxPos;
        ruhYe = (UChar)std::min( (iXe - iMaxPos), iMaxPos );
        std::swap( ruhXs, ruhXe );
        std::swap( ruhYs, ruhYe );
        return;
      }
    }
    else if( iVirtualEndY > (uiScaledBlockSize-1) )
    {
      Int iXe = roftoi( (Double)((Int)(uiScaledBlockSize-1) - (Int)ruhYs ) * ((Double)iDeltaX / (Double)iDeltaY) ) + iDeltaEnd;
      if( iXe < (Int)uiScaledBlockSize )
      {
        ruhXe = (UChar)std::max( iXe, 0 );
        ruhYe = (UChar)(uiScaledBlockSize-1);
        return;
      }
      else
      {
        ruhXe = (UChar)iMaxPos;
        ruhYe = (UChar)std::max( (iMaxPos - (iXe - iMaxPos)), 0 );
        std::swap( ruhXs, ruhXe );
        std::swap( ruhYs, ruhYe );
        return;
      }
    }
    else
    {
      Int iYe = iVirtualEndY - iDeltaEnd;
      if( iYe < 0 )
      {
        ruhXe = (UChar)std::max( (iMaxPos + iYe), 0 );
        ruhYe = 0;
        std::swap( ruhXs, ruhXe );
        std::swap( ruhYs, ruhYe );
        return;
      }
      else if( iYe > iMaxPos )
      {
        ruhXe = (UChar)std::max( (iMaxPos - (iYe - iMaxPos)), 0 );
        ruhYe = (UChar)iMaxPos;
        return;
      }
      else
      {
        ruhXe = (UChar)iMaxPos;
        ruhYe = (UChar)iYe;
        std::swap( ruhXs, ruhXe );
        std::swap( ruhYs, ruhYe );
        return;
      }
    }
  }

  // case origin
  if( uiScaledStartPosX == 0 && uiScaledStartPosY == 0 )
  {
    if( iDeltaX*iDeltaY < 0 )
    {
      return;
    }

    ruhXs = 0;
    ruhYs = 0;

    if( iDeltaY == 0 )
    {
      ruhXe = (UChar)iMaxPos;
      ruhYe = 0;
      std::swap( ruhXs, ruhXe );
      std::swap( ruhYs, ruhYe );
      return;
    }

    if( iDeltaX == 0 )
    {
      ruhXe = 0;
      ruhYe = (UChar)iMaxPos;
      return;
    }

    Int iVirtualEndX = (Int)ruhXs + roftoi( (Double)iMaxPos * ((Double)iDeltaX / (Double)iDeltaY) );

    if( iVirtualEndX > iMaxPos )
    {
      Int iYe = roftoi( (Double)((Int)iMaxPos - (Int)ruhXs) * ((Double)iDeltaY / (Double)iDeltaX) ) - iDeltaEnd;
      if( iYe < (Int)uiScaledBlockSize )
      {
        ruhXe = (UChar)(uiScaledBlockSize-1);
        ruhYe = (UChar)std::max( iYe, 0 );
        std::swap( ruhXs, ruhXe );
        std::swap( ruhYs, ruhYe );
        return;
      }
      else
      {
        ruhXe = (UChar)std::max( (iMaxPos - (iYe - iMaxPos)), 0 );
        ruhYe = (UChar)(uiScaledBlockSize-1);
        return;
      }
    }
    else
    {
      Int iXe = iVirtualEndX + iDeltaEnd;
      if( iXe < 0 )
      {
        ruhXe = 0;
        ruhYe = (UChar)std::max( (iMaxPos + iXe), 0 );
        return;
      }
      else if( iXe > iMaxPos )
      {
        ruhXe = (UChar)(uiScaledBlockSize-1);
        ruhYe = (UChar)std::max( (iMaxPos - (iXe - iMaxPos)), 0 );
        std::swap( ruhXs, ruhXe );
        std::swap( ruhYs, ruhYe );
        return;
      }
      else
      {
        ruhXe = (UChar)iXe;
        ruhYe = (UChar)(uiScaledBlockSize-1);
        return;
      }
    }
  }
}
#endif

Void 
TComPrediction::predIntraDepthAng(TComPattern* pcTComPattern, UInt uiDirMode, Pel* piPred, UInt uiStride, Int iWidth, Int iHeight )
{
  Pel*  pDst    = piPred;
  Int*  ptrSrc  = pcTComPattern->getAdiOrgBuf( iWidth, iHeight, m_piYuvExt );
  Int   sw      = ( iWidth<<1 ) + 1;
#if !LOGI_INTRA_NAME_3MPM
  uiDirMode     = g_aucAngIntraModeOrder[ uiDirMode ];
#endif
  xPredIntraAngDepth( ptrSrc+sw+1, sw, pDst, uiStride, iWidth, iHeight, uiDirMode );
}

Int
TComPrediction::xGetDCDepth( Int* pSrc, Int iDelta, Int iBlkSize )
{
  Int iDC    = PDM_UNDEFINED_DEPTH;
  Int iSum   = 0;
  Int iNum   = 0;
  for( Int k = 0; k < iBlkSize; k++, pSrc += iDelta )
  {
    if( *pSrc != PDM_UNDEFINED_DEPTH )
    {
      iSum += *pSrc;
      iNum ++;
    }
  }
  if( iNum )
  {
    iDC = ( iSum + ( iNum >> 1 ) ) / iNum;
  }
  return iDC;
}

Int
TComPrediction::xGetDCValDepth( Int iVal1, Int iVal2, Int iVal3, Int iVal4 )
{
  if     ( iVal1 != PDM_UNDEFINED_DEPTH )   return iVal1;
  else if( iVal2 != PDM_UNDEFINED_DEPTH )   return iVal2;
  else if( iVal3 != PDM_UNDEFINED_DEPTH )   return iVal3;
  return   iVal4;
}

Void 
TComPrediction::xPredIntraAngDepth( Int* pSrc, Int srcStride, Pel* pDst, Int dstStride, UInt width, UInt height, UInt dirMode )
{
  AOF( width == height );
  Int blkSize       = width;
  Int iDCAbove      = xGetDCDepth( pSrc - srcStride,                               1, blkSize );
  Int iDCAboveRight = xGetDCDepth( pSrc - srcStride + blkSize,                     1, blkSize );
  Int iDCLeft       = xGetDCDepth( pSrc -         1,                       srcStride, blkSize );
  Int iDCBelowLeft  = xGetDCDepth( pSrc -         1 + blkSize * srcStride, srcStride, blkSize );
  Int iWgt, iDC1, iDC2;
  if( dirMode < 2 ) // 1..2
  {
    iDC1  = xGetDCValDepth( iDCAbove, iDCAboveRight, iDCLeft,  iDCBelowLeft  );
    iDC2  = xGetDCValDepth( iDCLeft,  iDCBelowLeft,  iDCAbove, iDCAboveRight );
    iWgt  = 8;
  }
  else if( dirMode < 11 ) // 3..10
  {
    iDC1  = xGetDCValDepth( iDCLeft,  iDCBelowLeft,  iDCAbove, iDCAboveRight );
    iDC2  = xGetDCValDepth( iDCBelowLeft,  iDCLeft,  iDCAbove, iDCAboveRight );
    iWgt  = 6 + dirMode; 
  }
  else if( dirMode < 27 ) // 11..26
  {
    iDC1  = xGetDCValDepth( iDCAbove, iDCAboveRight, iDCLeft,  iDCBelowLeft  );
    iDC2  = xGetDCValDepth( iDCLeft,  iDCBelowLeft,  iDCAbove, iDCAboveRight );
    iWgt  = dirMode - 10;
  }
  else if( dirMode < 35 ) // 27..34
  {
    iDC1  = xGetDCValDepth( iDCAbove, iDCAboveRight, iDCLeft,  iDCBelowLeft  );
    iDC2  = xGetDCValDepth( iDCAboveRight, iDCAbove, iDCLeft,  iDCBelowLeft  );
    iWgt  = 42 - dirMode;
  }
  else // (wedgelet -> use simple DC prediction
  {
    iDC1  = xGetDCValDepth( iDCAbove, iDCAboveRight, iDCLeft,  iDCBelowLeft  );
    iDC2  = xGetDCValDepth( iDCLeft,  iDCBelowLeft,  iDCAbove, iDCAboveRight );
    iWgt  = 8;
  }
  Int iWgt2   = 16 - iWgt;
  Int iDCVal  = ( iWgt * iDC1 + iWgt2 * iDC2 + 8 ) >> 4;

  // set depth
  for( Int iY = 0; iY < blkSize; iY++, pDst += dstStride )
  {
    for( Int iX = 0; iX < blkSize; iX++ )
    {
      pDst[ iX ] = iDCVal;
    }
  }
}

//! \}