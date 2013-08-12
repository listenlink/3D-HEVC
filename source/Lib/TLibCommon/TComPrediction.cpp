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
, m_iLumaRecStride(0)
{
  m_piYuvExt = NULL;
#if H_3D_VSP
  m_pDepthBlock = (Int*) malloc(MAX_NUM_SPU_W*MAX_NUM_SPU_W*sizeof(Int));
  if (m_pDepthBlock == NULL)
      printf("ERROR: UKTGHU, No memory allocated.\n");
#endif
}

TComPrediction::~TComPrediction()
{
#if H_3D_VSP
  if (m_pDepthBlock != NULL)
      free(m_pDepthBlock);
#endif

  delete[] m_piYuvExt;

  m_acYuvPred[0].destroy();
  m_acYuvPred[1].destroy();

  m_cYuvPredTemp.destroy();

#if H_3D_ARP
  m_acYuvPredBase[0].destroy();
  m_acYuvPredBase[1].destroy();
#endif
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
    Int extWidth  = MAX_CU_SIZE + 16; 
    Int extHeight = MAX_CU_SIZE + 1;
    Int i, j;
    for (i = 0; i < 4; i++)
    {
      m_filteredBlockTmp[i].create(extWidth, extHeight + 7);
      for (j = 0; j < 4; j++)
      {
        m_filteredBlock[i][j].create(extWidth, extHeight);
      }
    }
    m_iYuvExtHeight  = ((MAX_CU_SIZE + 2) << 4);
    m_iYuvExtStride = ((MAX_CU_SIZE  + 8) << 4);
    m_piYuvExt = new Int[ m_iYuvExtStride * m_iYuvExtHeight ];

    // new structure
    m_acYuvPred[0] .create( MAX_CU_SIZE, MAX_CU_SIZE );
    m_acYuvPred[1] .create( MAX_CU_SIZE, MAX_CU_SIZE );

    m_cYuvPredTemp.create( MAX_CU_SIZE, MAX_CU_SIZE );
#if H_3D_ARP
    m_acYuvPredBase[0] .create( g_uiMaxCUWidth, g_uiMaxCUHeight );
    m_acYuvPredBase[1] .create( g_uiMaxCUWidth, g_uiMaxCUHeight );
#endif
  }

  if (m_iLumaRecStride != (MAX_CU_SIZE>>1) + 1)
  {
    m_iLumaRecStride =  (MAX_CU_SIZE>>1) + 1;
    if (!m_pLumaRecBuffer)
    {
      m_pLumaRecBuffer = new Pel[ m_iLumaRecStride * m_iLumaRecStride ];
    }
  }
#if H_3D_IC
  for( Int i = 1; i < 64; i++ )
  {
    m_uiaShift[i-1] = ( (1 << 15) + i/2 ) / i;
  }
#endif
}

// ====================================================================================================================
// Public member functions
// ====================================================================================================================

// Function for calculating DC value of the reference samples used in Intra prediction
Pel TComPrediction::predIntraGetPredValDC( Int* pSrc, Int iSrcStride, UInt iWidth, UInt iHeight, Bool bAbove, Bool bLeft )
{
  assert(iWidth > 0 && iHeight > 0);
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
Void TComPrediction::xPredIntraAng(Int bitDepth, Int* pSrc, Int srcStride, Pel*& rpDst, Int dstStride, UInt width, UInt height, UInt dirMode, Bool blkAboveAvailable, Bool blkLeftAvailable, Bool bFilter )
{
  Int k,l;
  Int blkSize        = width;
  Pel* pDst          = rpDst;

  // Map the mode index to main prediction direction and angle
  assert( dirMode > 0 ); //no planar
  Bool modeDC        = dirMode < 2;
  Bool modeHor       = !modeDC && (dirMode < 18);
  Bool modeVer       = !modeDC && !modeHor;
  Int intraPredAngle = modeVer ? (Int)dirMode - VER_IDX : modeHor ? -((Int)dirMode - HOR_IDX) : 0;
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
          pDst[k*dstStride] = Clip3(0, (1<<bitDepth)-1, pDst[k*dstStride] + (( refSide[k+1] - refSide[0] ) >> 1) );
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

Void TComPrediction::predIntraLumaAng(TComPattern* pcTComPattern, UInt uiDirMode, Pel* piPred, UInt uiStride, Int iWidth, Int iHeight, Bool bAbove, Bool bLeft )
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
    if ( (iWidth > 16) || (iHeight > 16) )
    {
      xPredIntraAng(g_bitDepthY, ptrSrc+sw+1, sw, pDst, uiStride, iWidth, iHeight, uiDirMode, bAbove, bLeft, false );
    }
    else
    {
      xPredIntraAng(g_bitDepthY, ptrSrc+sw+1, sw, pDst, uiStride, iWidth, iHeight, uiDirMode, bAbove, bLeft, true );

      if( (uiDirMode == DC_IDX ) && bAbove && bLeft )
      {
        xDCPredFiltering( ptrSrc+sw+1, sw, pDst, uiStride, iWidth, iHeight);
      }
    }
  }
}

// Angular chroma
Void TComPrediction::predIntraChromaAng( Int* piSrc, UInt uiDirMode, Pel* piPred, UInt uiStride, Int iWidth, Int iHeight, Bool bAbove, Bool bLeft )
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
    xPredIntraAng(g_bitDepthC, ptrSrc+sw+1, sw, pDst, uiStride, iWidth, iHeight, uiDirMode, bAbove, bLeft, false );
  }
}

#if H_3D_DIM
Void TComPrediction::predIntraLumaDepth( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiIntraMode, Pel* piPred, UInt uiStride, Int iWidth, Int iHeight, Bool bFastEnc )
{
  assert( iWidth == iHeight  );
  assert( iWidth >= DIM_MIN_SIZE && iWidth <= DIM_MAX_SIZE );
  assert( isDimMode( uiIntraMode ) );

  UInt dimType    = getDimType  ( uiIntraMode );
  Bool dimDeltaDC = isDimDeltaDC( uiIntraMode );    
  Bool isDmmMode  = (dimType <  DMM_NUM_TYPE);
  Bool isRbcMode  = (dimType == RBC_IDX);

  Bool* biSegPattern  = NULL;
  UInt  patternStride = 0;

  // get partiton
#if H_3D_DIM_DMM
  TComWedgelet* dmmSegmentation = NULL;
  if( isDmmMode )
  {
    switch( dimType )
    {
    case( DMM1_IDX ): 
      {
        dmmSegmentation = &(g_dmmWedgeLists[ g_aucConvertToBit[iWidth] ][ pcCU->getDmmWedgeTabIdx( dimType, uiAbsPartIdx ) ]);
      } break;
    case( DMM2_IDX ):
      {
        UInt uiTabIdx = 0;
        if( bFastEnc ) { uiTabIdx = pcCU->getDmmWedgeTabIdx( dimType, uiAbsPartIdx ); }
        else
        {
          uiTabIdx = xPredWedgeFromIntra( pcCU, uiAbsPartIdx, iWidth, iHeight, pcCU->getDmm2DeltaEnd( uiAbsPartIdx ) );
          pcCU->setDmmWedgeTabIdxSubParts( uiTabIdx, dimType, uiAbsPartIdx, (pcCU->getDepth(0) + (pcCU->getPartitionSize(0) == SIZE_2Nx2N ? 0 : 1)) );
        }
        dmmSegmentation = &(g_dmmWedgeLists[ g_aucConvertToBit[iWidth] ][ uiTabIdx ]);
      } break;
    case( DMM3_IDX ): 
      {
        UInt uiTabIdx = 0;
        if( bFastEnc ) { uiTabIdx = pcCU->getDmmWedgeTabIdx( dimType, uiAbsPartIdx ); }
        else
        {
          uiTabIdx = xPredWedgeFromTex( pcCU, uiAbsPartIdx, iWidth, iHeight, pcCU->getDmm3IntraTabIdx( uiAbsPartIdx ) );
          pcCU->setDmmWedgeTabIdxSubParts( uiTabIdx, dimType, uiAbsPartIdx, (pcCU->getDepth(0) + (pcCU->getPartitionSize(0) == SIZE_2Nx2N ? 0 : 1)) );
        }
        dmmSegmentation = &(g_dmmWedgeLists[ g_aucConvertToBit[iWidth] ][ uiTabIdx ]);
      } break;
    case( DMM4_IDX ): 
      {
        dmmSegmentation = new TComWedgelet( iWidth, iHeight );
        xPredContourFromTex( pcCU, uiAbsPartIdx, iWidth, iHeight, dmmSegmentation );
      } break;
    default: assert(0);
    }
    assert( dmmSegmentation );
    biSegPattern  = dmmSegmentation->getPattern();
    patternStride = dmmSegmentation->getStride ();
  }
#endif
#if H_3D_DIM_RBC
  if( isRbcMode )
  {
    biSegPattern  = pcCU->getEdgePartition( uiAbsPartIdx );
    patternStride = iWidth;
  }
#endif

  // get predicted partition values
  assert( biSegPattern );
  Int* piMask = NULL;
  if( isDmmMode ) piMask = pcCU->getPattern()->getAdiOrgBuf( iWidth, iHeight, m_piYuvExt ); // no filtering for DMM
  else            piMask = pcCU->getPattern()->getPredictorPtr( 0, g_aucConvertToBit[ iWidth ] + 2, m_piYuvExt );
  assert( piMask );
  Int maskStride = 2*iWidth + 1;  
  Int* ptrSrc = piMask+maskStride+1;
  Pel predDC1 = 0; Pel predDC2 = 0;
  xPredBiSegDCs( ptrSrc, maskStride, biSegPattern, patternStride, predDC1, predDC2 );

  // set segment values with deltaDC offsets
  Pel segDC1 = 0;
  Pel segDC2 = 0;
  if( dimDeltaDC )
  {
    Pel deltaDC1 = pcCU->getDimDeltaDC( dimType, 0, uiAbsPartIdx );
    Pel deltaDC2 = pcCU->getDimDeltaDC( dimType, 1, uiAbsPartIdx );
#if H_3D_DIM_DMM
    if( isDmmMode )
    {
#if H_3D_DIM_DLT
      segDC1 = pcCU->getSlice()->getVPS()->idx2DepthValue( pcCU->getSlice()->getLayerIdInVps(), pcCU->getSlice()->getVPS()->depthValue2idx( pcCU->getSlice()->getLayerIdInVps(), predDC1 ) + deltaDC1 );
      segDC2 = pcCU->getSlice()->getVPS()->idx2DepthValue( pcCU->getSlice()->getLayerIdInVps(), pcCU->getSlice()->getVPS()->depthValue2idx( pcCU->getSlice()->getLayerIdInVps(), predDC2 ) + deltaDC2 );
#else
      segDC1 = ClipY( predDC1 + deltaDC1 );
      segDC2 = ClipY( predDC2 + deltaDC2 );
#endif
    }
#endif
#if H_3D_DIM_RBC
    if( isRbcMode )
    {
      xDeltaDCQuantScaleUp( pcCU, deltaDC1 );
      xDeltaDCQuantScaleUp( pcCU, deltaDC2 );
      segDC1 = ClipY( predDC1 + deltaDC1 );
      segDC2 = ClipY( predDC2 + deltaDC2 );
    }
#endif
  }
  else
  {
    segDC1 = predDC1;
    segDC2 = predDC2;
  }

  // set prediction signal
  Pel* pDst = piPred;
  xAssignBiSegDCs( pDst, uiStride, biSegPattern, patternStride, segDC1, segDC2 );

#if H_3D_DIM_DMM
  if( dimType == DMM4_IDX ) { dmmSegmentation->destroy(); delete dmmSegmentation; }
#endif
}
#endif

/** Function for checking identical motion.
 * \param TComDataCU* pcCU
 * \param UInt PartAddr
 */
Bool TComPrediction::xCheckIdenticalMotion ( TComDataCU* pcCU, UInt PartAddr )
{
  if( pcCU->getSlice()->isInterB() && !pcCU->getSlice()->getPPS()->getWPBiPred() )
  {
    if( pcCU->getCUMvField(REF_PIC_LIST_0)->getRefIdx(PartAddr) >= 0 && pcCU->getCUMvField(REF_PIC_LIST_1)->getRefIdx(PartAddr) >= 0)
    {
      Int RefPOCL0 = pcCU->getSlice()->getRefPic(REF_PIC_LIST_0, pcCU->getCUMvField(REF_PIC_LIST_0)->getRefIdx(PartAddr))->getPOC();
      Int RefPOCL1 = pcCU->getSlice()->getRefPic(REF_PIC_LIST_1, pcCU->getCUMvField(REF_PIC_LIST_1)->getRefIdx(PartAddr))->getPOC();
      if(RefPOCL0 == RefPOCL1 && pcCU->getCUMvField(REF_PIC_LIST_0)->getMv(PartAddr) == pcCU->getCUMvField(REF_PIC_LIST_1)->getMv(PartAddr))
      {
        return true;
      }
    }
  }
  return false;
}


Void TComPrediction::motionCompensation ( TComDataCU* pcCU, TComYuv* pcYuvPred, RefPicList eRefPicList, Int iPartIdx )
{
  Int         iWidth;
  Int         iHeight;
  UInt        uiPartAddr;

  if ( iPartIdx >= 0 )
  {
    pcCU->getPartIndexAndSize( iPartIdx, uiPartAddr, iWidth, iHeight );
#if H_3D_VSP
    if ( 0 == pcCU->getVSPFlag(uiPartAddr) )
    {
#endif
      if ( eRefPicList != REF_PIC_LIST_X )
      {
        if( pcCU->getSlice()->getPPS()->getUseWP())
        {
          xPredInterUni (pcCU, uiPartAddr, iWidth, iHeight, eRefPicList, pcYuvPred, true );
        }
        else
        {
          xPredInterUni (pcCU, uiPartAddr, iWidth, iHeight, eRefPicList, pcYuvPred );
        }
        if ( pcCU->getSlice()->getPPS()->getUseWP() )
        {
          xWeightedPredictionUni( pcCU, pcYuvPred, uiPartAddr, iWidth, iHeight, eRefPicList, pcYuvPred );
        }
      }
      else
      {
        if ( xCheckIdenticalMotion( pcCU, uiPartAddr ) )
        {
          xPredInterUni (pcCU, uiPartAddr, iWidth, iHeight, REF_PIC_LIST_0, pcYuvPred );
        }
        else
        {
          xPredInterBi  (pcCU, uiPartAddr, iWidth, iHeight, pcYuvPred );
        }
      }
#if H_3D_VSP
    }
    else
    {
      if ( xCheckIdenticalMotion( pcCU, uiPartAddr ) )
        xPredInterUniVSP( pcCU, uiPartAddr, iWidth, iHeight, REF_PIC_LIST_0, pcYuvPred );
      else
        xPredInterBiVSP ( pcCU, uiPartAddr, iWidth, iHeight, pcYuvPred );
    }
#endif
    return;
  }

  for ( iPartIdx = 0; iPartIdx < pcCU->getNumPartInter(); iPartIdx++ )
  {
    pcCU->getPartIndexAndSize( iPartIdx, uiPartAddr, iWidth, iHeight );

#if H_3D_VSP
    if ( 0 == pcCU->getVSPFlag(uiPartAddr) )
    {
#endif
      if ( eRefPicList != REF_PIC_LIST_X )
      {
        if( pcCU->getSlice()->getPPS()->getUseWP())
        {
          xPredInterUni (pcCU, uiPartAddr, iWidth, iHeight, eRefPicList, pcYuvPred, true );
        }
        else
        {
          xPredInterUni (pcCU, uiPartAddr, iWidth, iHeight, eRefPicList, pcYuvPred );
        }
        if ( pcCU->getSlice()->getPPS()->getUseWP() )
        {
          xWeightedPredictionUni( pcCU, pcYuvPred, uiPartAddr, iWidth, iHeight, eRefPicList, pcYuvPred );
        }
      }
      else
      {
        if ( xCheckIdenticalMotion( pcCU, uiPartAddr ) )
        {
          xPredInterUni (pcCU, uiPartAddr, iWidth, iHeight, REF_PIC_LIST_0, pcYuvPred );
        }
        else
        {
          xPredInterBi  (pcCU, uiPartAddr, iWidth, iHeight, pcYuvPred );
        }
      }
#if H_3D_VSP
    }
    else
    {
      if ( xCheckIdenticalMotion( pcCU, uiPartAddr ) )
        xPredInterUniVSP( pcCU, uiPartAddr, iWidth, iHeight, REF_PIC_LIST_0, pcYuvPred );
      else
        xPredInterBiVSP ( pcCU, uiPartAddr, iWidth, iHeight, pcYuvPred );
    }
#endif
  }
  return;
}

Void TComPrediction::xPredInterUni ( TComDataCU* pcCU, UInt uiPartAddr, Int iWidth, Int iHeight, RefPicList eRefPicList, TComYuv*& rpcYuvPred, Bool bi )
{
  Int         iRefIdx     = pcCU->getCUMvField( eRefPicList )->getRefIdx( uiPartAddr );           assert (iRefIdx >= 0);
  TComMv      cMv         = pcCU->getCUMvField( eRefPicList )->getMv( uiPartAddr );
  pcCU->clipMv(cMv);
#if H_3D_ARP
  if(  pcCU->getARPW( uiPartAddr ) > 0 
    && pcCU->getPartitionSize(uiPartAddr)==SIZE_2Nx2N 
    && pcCU->getSlice()->getRefPic( eRefPicList, iRefIdx )->getPOC()!= pcCU->getSlice()->getPOC() 
    )
  {
    xPredInterUniARP( pcCU, uiPartAddr, iWidth, iHeight, eRefPicList, rpcYuvPred, bi );
  }
  else
  {
#endif
#if H_3D_IC
    Bool bICFlag = pcCU->getICFlag( uiPartAddr ) && ( pcCU->getSlice()->getRefPic( eRefPicList, iRefIdx )->getViewIndex() != pcCU->getSlice()->getViewIndex() );
    xPredInterLumaBlk  ( pcCU, pcCU->getSlice()->getRefPic( eRefPicList, iRefIdx )->getPicYuvRec(), uiPartAddr, &cMv, iWidth, iHeight, rpcYuvPred, bi
#if H_3D_ARP
      , false
#endif
      , bICFlag );
    xPredInterChromaBlk( pcCU, pcCU->getSlice()->getRefPic( eRefPicList, iRefIdx )->getPicYuvRec(), uiPartAddr, &cMv, iWidth, iHeight, rpcYuvPred, bi
#if H_3D_ARP
      , false
#endif
      , bICFlag );
#else
  xPredInterLumaBlk  ( pcCU, pcCU->getSlice()->getRefPic( eRefPicList, iRefIdx )->getPicYuvRec(), uiPartAddr, &cMv, iWidth, iHeight, rpcYuvPred, bi );
  xPredInterChromaBlk( pcCU, pcCU->getSlice()->getRefPic( eRefPicList, iRefIdx )->getPicYuvRec(), uiPartAddr, &cMv, iWidth, iHeight, rpcYuvPred, bi );
#endif
#if H_3D_ARP
  }
#endif
}

#if H_3D_VSP
Void TComPrediction::xPredInterUniVSP( TComDataCU* pcCU, UInt uiPartAddr, Int iWidth, Int iHeight, RefPicList eRefPicList, TComYuv*& rpcYuvPred, Bool bi )
{
  // Get depth reference
#if MTK_VSP_FIX_E0172 
  Int vspDir = pcCU->getVSPDir( uiPartAddr );
  RefPicList privateRefPicList = (vspDir == 0) ? REF_PIC_LIST_0 : REF_PIC_LIST_1;
  Int privateRefIdx = pcCU->getCUMvField( privateRefPicList )->getRefIdx( uiPartAddr );
  Int depthRefViewIdx = pcCU->getSlice()->getRefPic(privateRefPicList, privateRefIdx)->getViewIndex();
#else
  Int depthRefViewIdx = pcCU->getDvInfo(uiPartAddr).m_aVIdxCan;
#endif
  TComPic* pRefPicBaseDepth = pcCU->getSlice()->getIvPic (true, depthRefViewIdx );
  assert(pRefPicBaseDepth != NULL);
  TComPicYuv* pcBaseViewDepthPicYuv = pRefPicBaseDepth->getPicYuvRec();
  assert(pcBaseViewDepthPicYuv != NULL);

  // Get texture reference
  Int iRefIdx = pcCU->getCUMvField( eRefPicList )->getRefIdx( uiPartAddr );
  assert(iRefIdx >= 0);
  TComPic* pRefPicBaseTxt = pcCU->getSlice()->getRefPic( eRefPicList, iRefIdx );
  TComPicYuv* pcBaseViewTxtPicYuv = pRefPicBaseTxt->getPicYuvRec();
  assert(pcBaseViewTxtPicYuv != NULL);

  // Initialize LUT according to the reference viewIdx
  Int txtRefViewIdx = pRefPicBaseTxt->getViewIndex();
  Int* pShiftLUT    = pcCU->getSlice()->getDepthToDisparityB( txtRefViewIdx );
  assert( txtRefViewIdx < pcCU->getSlice()->getViewIndex() );

  // Do compensation
#if MTK_VSP_FIX_E0172 
  TComMv cDv  = pcCU->getCUMvField( privateRefPicList )->getMv( uiPartAddr );
#else
  TComMv cDv  = pcCU->getCUMvField( eRefPicList )->getMv( uiPartAddr ); // cDv is the disparity vector derived from the neighbors
#endif
  pcCU->clipMv(cDv);
  UInt uiAbsPartIdx = pcCU->getZorderIdxInCU();
  Int iBlkX = ( pcCU->getAddr() % pRefPicBaseDepth->getFrameWidthInCU() ) * g_uiMaxCUWidth  + g_auiRasterToPelX[ g_auiZscanToRaster[ uiAbsPartIdx ] ];
  Int iBlkY = ( pcCU->getAddr() / pRefPicBaseDepth->getFrameWidthInCU() ) * g_uiMaxCUHeight + g_auiRasterToPelY[ g_auiZscanToRaster[ uiAbsPartIdx ] ];
  xPredInterLumaBlkFromDM  ( pcBaseViewTxtPicYuv, pcBaseViewDepthPicYuv, pShiftLUT, &cDv, uiPartAddr, iBlkX,    iBlkY,    iWidth,    iHeight,    pcCU->getSlice()->getIsDepth(), rpcYuvPred, bi );
  xPredInterChromaBlkFromDM( pcBaseViewTxtPicYuv, pcBaseViewDepthPicYuv, pShiftLUT, &cDv, uiPartAddr, iBlkX>>1, iBlkY>>1, iWidth>>1, iHeight>>1, pcCU->getSlice()->getIsDepth(), rpcYuvPred, bi );
}
#endif

#if H_3D_ARP
Void TComPrediction::xPredInterUniARP( TComDataCU* pcCU, UInt uiPartAddr, Int iWidth, Int iHeight, RefPicList eRefPicList, TComYuv*& rpcYuvPred, Bool bi, TComMvField * pNewMvFiled )
{
  Int         iRefIdx      = pNewMvFiled ? pNewMvFiled->getRefIdx() : pcCU->getCUMvField( eRefPicList )->getRefIdx( uiPartAddr );           
  TComMv      cMv          = pNewMvFiled ? pNewMvFiled->getMv()     : pcCU->getCUMvField( eRefPicList )->getMv( uiPartAddr );
  Bool        bTobeScaled  = false;
  TComPic* pcPicYuvBaseCol = NULL;
  TComPic* pcPicYuvBaseRef = NULL;

#if H_3D_NBDV
  DisInfo cDistparity;
  cDistparity.bDV           = pcCU->getDvInfo(uiPartAddr).bDV;
  if( cDistparity.bDV )
  {
    cDistparity.m_acNBDV = pcCU->getDvInfo(0).m_acNBDV;
    assert(pcCU->getDvInfo(uiPartAddr).bDV ==  pcCU->getDvInfo(0).bDV);
    cDistparity.m_aVIdxCan = pcCU->getDvInfo(uiPartAddr).m_aVIdxCan;
  }
#else
  assert(0); // ARP can be applied only when a DV is available
#endif

  UChar dW = cDistparity.bDV ? pcCU->getARPW ( uiPartAddr ) : 0;

  if( cDistparity.bDV ) 
  {
    if( dW > 0 && pcCU->getSlice()->getRefPic( eRefPicList, 0 )->getPOC()!= pcCU->getSlice()->getPOC() )
    {
      bTobeScaled = true;
    }

    pcPicYuvBaseCol =  pcCU->getSlice()->getBaseViewRefPic( pcCU->getSlice()->getPOC(),                              cDistparity.m_aVIdxCan );
    pcPicYuvBaseRef =  pcCU->getSlice()->getBaseViewRefPic( pcCU->getSlice()->getRefPic( eRefPicList, 0 )->getPOC(), cDistparity.m_aVIdxCan );
    
    if( ( !pcPicYuvBaseCol || pcPicYuvBaseCol->getPOC() != pcCU->getSlice()->getPOC() ) || ( !pcPicYuvBaseRef || pcPicYuvBaseRef->getPOC() != pcCU->getSlice()->getRefPic( eRefPicList, 0 )->getPOC() ) )
    {
      dW = 0;
      bTobeScaled = false;
    }
    else
    {
      assert( pcPicYuvBaseCol->getPOC() == pcCU->getSlice()->getPOC() && pcPicYuvBaseRef->getPOC() == pcCU->getSlice()->getRefPic( eRefPicList, 0 )->getPOC() );
    }

    if(bTobeScaled)
    {     
      Int iCurrPOC    = pcCU->getSlice()->getPOC();
      Int iColRefPOC  = pcCU->getSlice()->getRefPOC( eRefPicList, iRefIdx );
      Int iCurrRefPOC = pcCU->getSlice()->getRefPOC( eRefPicList,  0);
      Int iScale = pcCU-> xGetDistScaleFactor(iCurrPOC, iCurrRefPOC, iCurrPOC, iColRefPOC);
      if ( iScale != 4096 )
      {
        cMv = cMv.scaleMv( iScale );
      }
      iRefIdx = 0;
    }
  }

  pcCU->clipMv(cMv);
  TComPicYuv* pcPicYuvRef = pcCU->getSlice()->getRefPic( eRefPicList, iRefIdx )->getPicYuvRec();
  xPredInterLumaBlk  ( pcCU, pcPicYuvRef, uiPartAddr, &cMv, iWidth, iHeight, rpcYuvPred, bi, true );
  xPredInterChromaBlk( pcCU, pcPicYuvRef, uiPartAddr, &cMv, iWidth, iHeight, rpcYuvPred, bi, true );

  if( dW > 0 )
  {
    TComYuv * pYuvB0 = &m_acYuvPredBase[0];
    TComYuv * pYuvB1  = &m_acYuvPredBase[1];

    TComMv cMVwithDisparity = cMv + cDistparity.m_acNBDV;
    pcCU->clipMv(cMVwithDisparity);

    assert ( cDistparity.bDV );

    pcPicYuvRef = pcPicYuvBaseCol->getPicYuvRec();
    xPredInterLumaBlk  ( pcCU, pcPicYuvRef, uiPartAddr, &cDistparity.m_acNBDV, iWidth, iHeight, pYuvB0, bi, true );
    xPredInterChromaBlk( pcCU, pcPicYuvRef, uiPartAddr, &cDistparity.m_acNBDV, iWidth, iHeight, pYuvB0, bi, true );
    
    pcPicYuvRef = pcPicYuvBaseRef->getPicYuvRec();
    xPredInterLumaBlk  ( pcCU, pcPicYuvRef, uiPartAddr, &cMVwithDisparity, iWidth, iHeight, pYuvB1, bi, true );
    xPredInterChromaBlk( pcCU, pcPicYuvRef, uiPartAddr, &cMVwithDisparity, iWidth, iHeight, pYuvB1, bi, true );

    pYuvB0->subtractARP( pYuvB0 , pYuvB1 , uiPartAddr , iWidth , iHeight );

    if( 2 == dW )
    {
      pYuvB0->multiplyARP( uiPartAddr , iWidth , iHeight , dW );
    }
    rpcYuvPred->addARP( rpcYuvPred , pYuvB0 , uiPartAddr , iWidth , iHeight , !bi );
  }
}
#endif

Void TComPrediction::xPredInterBi ( TComDataCU* pcCU, UInt uiPartAddr, Int iWidth, Int iHeight, TComYuv*& rpcYuvPred )
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
      xPredInterUni ( pcCU, uiPartAddr, iWidth, iHeight, eRefPicList, pcMbYuv, true );
    }
    else
    {
      if ( ( pcCU->getSlice()->getPPS()->getUseWP()       && pcCU->getSlice()->getSliceType() == P_SLICE ) || 
           ( pcCU->getSlice()->getPPS()->getWPBiPred() && pcCU->getSlice()->getSliceType() == B_SLICE ) )
      {
        xPredInterUni ( pcCU, uiPartAddr, iWidth, iHeight, eRefPicList, pcMbYuv, true );
      }
      else
      {
        xPredInterUni ( pcCU, uiPartAddr, iWidth, iHeight, eRefPicList, pcMbYuv );
      }
    }
  }

  if ( pcCU->getSlice()->getPPS()->getWPBiPred() && pcCU->getSlice()->getSliceType() == B_SLICE  )
  {
    xWeightedPredictionBi( pcCU, &m_acYuvPred[0], &m_acYuvPred[1], iRefIdx[0], iRefIdx[1], uiPartAddr, iWidth, iHeight, rpcYuvPred );
  }  
  else if ( pcCU->getSlice()->getPPS()->getUseWP() && pcCU->getSlice()->getSliceType() == P_SLICE )
  {
    xWeightedPredictionUni( pcCU, &m_acYuvPred[0], uiPartAddr, iWidth, iHeight, REF_PIC_LIST_0, rpcYuvPred ); 
  }
  else
  {
    xWeightedAverage( &m_acYuvPred[0], &m_acYuvPred[1], iRefIdx[0], iRefIdx[1], uiPartAddr, iWidth, iHeight, rpcYuvPred );
  }
}

#if H_3D_VSP

Void TComPrediction::xPredInterBiVSP( TComDataCU* pcCU, UInt uiPartAddr, Int iWidth, Int iHeight, TComYuv*& rpcYuvPred )
{
  TComYuv* pcMbYuv;
  Int      iRefIdx[2] = {-1, -1};
  Bool     bi = (pcCU->getCUMvField( REF_PIC_LIST_0 )->getRefIdx( uiPartAddr ) >= 0 && pcCU->getCUMvField( REF_PIC_LIST_1 )->getRefIdx( uiPartAddr ) >= 0);

  for ( Int iRefList = 0; iRefList < 2; iRefList++ )
  {
    RefPicList eRefPicList = RefPicList(iRefList);
    iRefIdx[iRefList] = pcCU->getCUMvField( eRefPicList )->getRefIdx( uiPartAddr );

    if ( iRefIdx[iRefList] < 0 )
      continue;
    assert( iRefIdx[iRefList] < pcCU->getSlice()->getNumRefIdx(eRefPicList) );

    pcMbYuv = &m_acYuvPred[iRefList];
    xPredInterUniVSP ( pcCU, uiPartAddr, iWidth, iHeight, eRefPicList, pcMbYuv, bi );
  }

  xWeightedAverage( &m_acYuvPred[0], &m_acYuvPred[1], iRefIdx[0], iRefIdx[1], uiPartAddr, iWidth, iHeight, rpcYuvPred );
}

#endif

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
Void TComPrediction::xPredInterLumaBlk( TComDataCU *cu, TComPicYuv *refPic, UInt partAddr, TComMv *mv, Int width, Int height, TComYuv *&dstPic, Bool bi 
#if H_3D_ARP
    , Bool filterType
#endif
#if H_3D_IC
    , Bool bICFlag
#endif
  )
{
  Int refStride = refPic->getStride();  
  Int refOffset = ( mv->getHor() >> 2 ) + ( mv->getVer() >> 2 ) * refStride;
  Pel *ref      = refPic->getLumaAddr( cu->getAddr(), cu->getZorderIdxInCU() + partAddr ) + refOffset;
  
  Int dstStride = dstPic->getStride();
  Pel *dst      = dstPic->getLumaAddr( partAddr );
  
  Int xFrac = mv->getHor() & 0x3;
  Int yFrac = mv->getVer() & 0x3;

#if H_3D_IC
  if( cu->getSlice()->getIsDepth() )
  {
    refOffset = mv->getHor() + mv->getVer() * refStride;
    ref       = refPic->getLumaAddr( cu->getAddr(), cu->getZorderIdxInCU() + partAddr ) + refOffset;
    xFrac     = 0;
    yFrac     = 0;
  }
#endif
  if ( yFrac == 0 )
  {
    m_if.filterHorLuma( ref, refStride, dst, dstStride, width, height, xFrac,       !bi 
#if H_3D_ARP
    , filterType
#endif
      );
  }
  else if ( xFrac == 0 )
  {
    m_if.filterVerLuma( ref, refStride, dst, dstStride, width, height, yFrac, true, !bi 
#if H_3D_ARP
    , filterType
#endif
      );
  }
  else
  {
    Int tmpStride = m_filteredBlockTmp[0].getStride();
    Short *tmp    = m_filteredBlockTmp[0].getLumaAddr();

    Int filterSize = NTAPS_LUMA;
    Int halfFilterSize = ( filterSize >> 1 );

    m_if.filterHorLuma(ref - (halfFilterSize-1)*refStride, refStride, tmp, tmpStride, width, height+filterSize-1, xFrac, false     
#if H_3D_ARP 
    , filterType
#endif 
      );
    m_if.filterVerLuma(tmp + (halfFilterSize-1)*tmpStride, tmpStride, dst, dstStride, width, height,              yFrac, false, !bi
#if H_3D_ARP
    , filterType
#endif 
      );    
  }

#if H_3D_IC
  if( bICFlag )
  {
    Int a, b, iShift, i, j;

    xGetLLSICPrediction( cu, mv, refPic, a, b, iShift, TEXT_LUMA );

    for ( i = 0; i < height; i++ )
    {
      for ( j = 0; j < width; j++ )
      {
        if( bi )
        {
          Int iIFshift = IF_INTERNAL_PREC - g_bitDepthY;
          dst[j] = ( ( a*dst[j] + a*IF_INTERNAL_OFFS ) >> iShift ) + b*( 1 << iIFshift ) - IF_INTERNAL_OFFS;
        }
        else
          dst[j] = Clip3( 0, ( 1 << g_bitDepthY ) - 1, ( ( a*dst[j] ) >> iShift ) + b );
      }
      dst += dstStride;
    }
  }
#endif
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
Void TComPrediction::xPredInterChromaBlk( TComDataCU *cu, TComPicYuv *refPic, UInt partAddr, TComMv *mv, Int width, Int height, TComYuv *&dstPic, Bool bi 
#if H_3D_ARP
    , Bool filterType
#endif
#if H_3D_IC
    , Bool bICFlag
#endif
  )
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
    m_if.filterHorChroma(refCb, refStride, dstCb,  dstStride, cxWidth, cxHeight, xFrac, !bi
#if H_3D_ARP
    , filterType
#endif
    );    
    m_if.filterHorChroma(refCr, refStride, dstCr,  dstStride, cxWidth, cxHeight, xFrac, !bi
#if H_3D_ARP
    , filterType
#endif
    );
  }
  else if ( xFrac == 0 )
  {
    m_if.filterVerChroma(refCb, refStride, dstCb, dstStride, cxWidth, cxHeight, yFrac, true, !bi
#if H_3D_ARP
    , filterType
#endif
    );
    m_if.filterVerChroma(refCr, refStride, dstCr, dstStride, cxWidth, cxHeight, yFrac, true, !bi
#if H_3D_ARP
    , filterType
#endif
    );
  }
  else
  {
    m_if.filterHorChroma(refCb - (halfFilterSize-1)*refStride, refStride, extY,  extStride, cxWidth, cxHeight+filterSize-1, xFrac, false
#if H_3D_ARP
    , filterType
#endif  
      );
    m_if.filterVerChroma(extY  + (halfFilterSize-1)*extStride, extStride, dstCb, dstStride, cxWidth, cxHeight  , yFrac, false, !bi
#if H_3D_ARP
    , filterType
#endif 
      );
    
    m_if.filterHorChroma(refCr - (halfFilterSize-1)*refStride, refStride, extY,  extStride, cxWidth, cxHeight+filterSize-1, xFrac, false
#if H_3D_ARP
    , filterType
#endif 
      );
    m_if.filterVerChroma(extY  + (halfFilterSize-1)*extStride, extStride, dstCr, dstStride, cxWidth, cxHeight  , yFrac, false, !bi
#if H_3D_ARP
    , filterType
#endif 
      );    
  }

#if H_3D_IC
  if( bICFlag )
  {
    Int a, b, iShift, i, j;
    xGetLLSICPrediction( cu, mv, refPic, a, b, iShift, TEXT_CHROMA_U ); // Cb
    for ( i = 0; i < cxHeight; i++ )
    {
      for ( j = 0; j < cxWidth; j++ )
      {
        if( bi )
        {
          Int iIFshift = IF_INTERNAL_PREC - g_bitDepthC;
          dstCb[j] = ( ( a*dstCb[j] + a*IF_INTERNAL_OFFS ) >> iShift ) + b*( 1<<iIFshift ) - IF_INTERNAL_OFFS;
        }
        else
          dstCb[j] = Clip3(  0, ( 1 << g_bitDepthC ) - 1, ( ( a*dstCb[j] ) >> iShift ) + b );
      }
      dstCb += dstStride;
    }
    xGetLLSICPrediction( cu, mv, refPic, a, b, iShift, TEXT_CHROMA_V ); // Cr
    for ( i = 0; i < cxHeight; i++ )
    {
      for ( j = 0; j < cxWidth; j++ )
      {
        if( bi )
        {
          Int iIFshift = IF_INTERNAL_PREC - g_bitDepthC;
          dstCr[j] = ( ( a*dstCr[j] + a*IF_INTERNAL_OFFS ) >> iShift ) + b*( 1<<iIFshift ) - IF_INTERNAL_OFFS;
        }
        else
          dstCr[j] = Clip3( 0, ( 1 << g_bitDepthC ) - 1, ( ( a*dstCr[j] ) >> iShift ) + b );
      }
      dstCr += dstStride;
    }
  }
#endif
}

Void TComPrediction::xWeightedAverage( TComYuv* pcYuvSrc0, TComYuv* pcYuvSrc1, Int iRefIdx0, Int iRefIdx1, UInt uiPartIdx, Int iWidth, Int iHeight, TComYuv*& rpcYuvDst )
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
Void TComPrediction::getMvPredAMVP( TComDataCU* pcCU, UInt uiPartIdx, UInt uiPartAddr, RefPicList eRefPicList, TComMv& rcMvPred )
{
  AMVPInfo* pcAMVPInfo = pcCU->getCUMvField(eRefPicList)->getAMVPInfo();
  if( pcAMVPInfo->iN <= 1 )
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
#if H_3D_IC
/** Function for deriving the position of first non-zero binary bit of a value
 * \param x input value
 *
 * This function derives the position of first non-zero binary bit of a value
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

/** Function for deriving LM illumination compensation.
 */
Void TComPrediction::xGetLLSICPrediction( TComDataCU* pcCU, TComMv *pMv, TComPicYuv *pRefPic, Int &a, Int &b, Int &iShift, TextType eType )
{
  TComPicYuv *pRecPic = pcCU->getPic()->getPicYuvRec();
  Pel *pRec = NULL, *pRef = NULL;
  UInt uiWidth, uiHeight, uiTmpPartIdx;
  Int iRecStride = ( eType == TEXT_LUMA ) ? pRecPic->getStride() : pRecPic->getCStride();
  Int iRefStride = ( eType == TEXT_LUMA ) ? pRefPic->getStride() : pRefPic->getCStride();
  Int iCUPelX, iCUPelY, iRefX, iRefY, iRefOffset, iHor, iVer;

  iCUPelX = pcCU->getCUPelX() + g_auiRasterToPelX[g_auiZscanToRaster[pcCU->getZorderIdxInCU()]];
  iCUPelY = pcCU->getCUPelY() + g_auiRasterToPelY[g_auiZscanToRaster[pcCU->getZorderIdxInCU()]];
  iHor = pcCU->getSlice()->getIsDepth() ? pMv->getHor() : ( ( pMv->getHor() + 2 ) >> 2 );
  iVer = pcCU->getSlice()->getIsDepth() ? pMv->getVer() : ( ( pMv->getVer() + 2 ) >> 2 );
  iRefX   = iCUPelX + iHor;
  iRefY   = iCUPelY + iVer;
  if( eType != TEXT_LUMA )
  {
    iHor = pcCU->getSlice()->getIsDepth() ? ( ( pMv->getHor() + 1 ) >> 1 ) : ( ( pMv->getHor() + 4 ) >> 3 );
    iVer = pcCU->getSlice()->getIsDepth() ? ( ( pMv->getVer() + 1 ) >> 1 ) : ( ( pMv->getVer() + 4 ) >> 3 );
  }
  uiWidth  = ( eType == TEXT_LUMA ) ? pcCU->getWidth( 0 )  : ( pcCU->getWidth( 0 )  >> 1 );
  uiHeight = ( eType == TEXT_LUMA ) ? pcCU->getHeight( 0 ) : ( pcCU->getHeight( 0 ) >> 1 );

  Int i, j, iCountShift = 0;

  // LLS parameters estimation -->

  Int x = 0, y = 0, xx = 0, xy = 0;

  if( pcCU->getPUAbove( uiTmpPartIdx, pcCU->getZorderIdxInCU() ) && iCUPelY > 0 && iRefY > 0 )
  {
    iRefOffset = iHor + iVer * iRefStride - iRefStride;
    if( eType == TEXT_LUMA )
    {
      pRef = pRefPic->getLumaAddr( pcCU->getAddr(), pcCU->getZorderIdxInCU() ) + iRefOffset;
      pRec = pRecPic->getLumaAddr( pcCU->getAddr(), pcCU->getZorderIdxInCU() ) - iRecStride;
    }
    else if( eType == TEXT_CHROMA_U )
    {
      pRef = pRefPic->getCbAddr( pcCU->getAddr(), pcCU->getZorderIdxInCU() ) + iRefOffset;
      pRec = pRecPic->getCbAddr( pcCU->getAddr(), pcCU->getZorderIdxInCU() ) - iRecStride;
    }
    else
    {
      assert( eType == TEXT_CHROMA_V );
      pRef = pRefPic->getCrAddr( pcCU->getAddr(), pcCU->getZorderIdxInCU() ) + iRefOffset;
      pRec = pRecPic->getCrAddr( pcCU->getAddr(), pcCU->getZorderIdxInCU() ) - iRecStride;
    }

    for( j = 0; j < uiWidth; j++ )
    {
      x += pRef[j];
      y += pRec[j];
      xx += pRef[j] * pRef[j];
      xy += pRef[j] * pRec[j];
    }
    iCountShift += g_aucConvertToBit[ uiWidth ] + 2;
  }


  if( pcCU->getPULeft( uiTmpPartIdx, pcCU->getZorderIdxInCU() ) && iCUPelX > 0 && iRefX > 0 )
  {
    iRefOffset = iHor + iVer * iRefStride - 1;
    if( eType == TEXT_LUMA )
    {
      pRef = pRefPic->getLumaAddr( pcCU->getAddr(), pcCU->getZorderIdxInCU() ) + iRefOffset;
      pRec = pRecPic->getLumaAddr( pcCU->getAddr(), pcCU->getZorderIdxInCU() ) - 1;
    }
    else if( eType == TEXT_CHROMA_U )
    {
      pRef = pRefPic->getCbAddr( pcCU->getAddr(), pcCU->getZorderIdxInCU() ) + iRefOffset;
      pRec = pRecPic->getCbAddr( pcCU->getAddr(), pcCU->getZorderIdxInCU() ) - 1;
    }
    else
    {
      assert( eType == TEXT_CHROMA_V );
      pRef = pRefPic->getCrAddr( pcCU->getAddr(), pcCU->getZorderIdxInCU() ) + iRefOffset;
      pRec = pRecPic->getCrAddr( pcCU->getAddr(), pcCU->getZorderIdxInCU() ) - 1;
    }

    for( i = 0; i < uiHeight; i++ )
    {
      x += pRef[0];
      y += pRec[0];
      xx += pRef[0] * pRef[0];
      xy += pRef[0] * pRec[0];

      pRef += iRefStride;
      pRec += iRecStride;
    }
    iCountShift += iCountShift > 0 ? 1 : ( g_aucConvertToBit[ uiWidth ] + 2 );
  }

  Int iTempShift = ( ( eType == TEXT_LUMA ) ? g_bitDepthY : g_bitDepthC ) + g_aucConvertToBit[ uiWidth ] + 3 - 15;

  if( iTempShift > 0 )
  {
    x  = ( x +  ( 1 << ( iTempShift - 1 ) ) ) >> iTempShift;
    y  = ( y +  ( 1 << ( iTempShift - 1 ) ) ) >> iTempShift;
    xx = ( xx + ( 1 << ( iTempShift - 1 ) ) ) >> iTempShift;
    xy = ( xy + ( 1 << ( iTempShift - 1 ) ) ) >> iTempShift;
    iCountShift -= iTempShift;
  }

  iShift = 13;

  if( iCountShift == 0 )
  {
    a = 1;
    b = 0;
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

      a = Clip3( -( 1 << 15 ), ( 1 << 15 ) - 1, a ); 

      Int minA = -(1 << (6));
      Int maxA = (1 << 6) - 1;
      if( a <= maxA && a >= minA )
      {
        // do nothing
      }
      else
      {
        Short n = CountLeadingZerosOnes( a );
        a = a >> (9-n);
        iShift -= (9-n);
      }

      b = (  y - ( ( a * x ) >> iShift ) + ( 1 << ( iCountShift - 1 ) ) ) >> iCountShift;
    }
  }   
}
#endif

#if H_3D_VSP
// Input:
// refPic: Ref picture. Full picture, with padding
// posX, posY:     PU position, texture
// sizeX, sizeY: PU size
// partAddr: z-order index
// dv: disparity vector. derived from neighboring blocks
//
// Output: dstPic, PU predictor 64x64
Void TComPrediction::xPredInterLumaBlkFromDM( TComPicYuv *refPic, TComPicYuv *pPicBaseDepth, Int* pShiftLUT, TComMv* dv, UInt partAddr,Int posX, Int posY
                                            , Int sizeX, Int sizeY, Bool isDepth, TComYuv *&dstPic, Bool bi )
{
  Int widthLuma;
  Int heightLuma;

  if (isDepth)
  {
    widthLuma   =  pPicBaseDepth->getWidth();
    heightLuma  =  pPicBaseDepth->getHeight();
  }
  else
  {
    widthLuma   =  refPic->getWidth();
    heightLuma  =  refPic->getHeight();
  }

#if H_3D_VSP_BLOCKSIZE != 1
  Int widthDepth  = pPicBaseDepth->getWidth();
  Int heightDepth = pPicBaseDepth->getHeight();
#endif

#if H_3D_VSP_CONSTRAINED
  Int widthDepth  = pPicBaseDepth->getWidth();
  Int heightDepth = pPicBaseDepth->getHeight();
#endif

  Int nTxtPerDepthX = widthLuma  / ( pPicBaseDepth->getWidth() );  // texture pixel # per depth pixel
  Int nTxtPerDepthY = heightLuma / ( pPicBaseDepth->getHeight() );

  Int refStride = refPic->getStride();
  Int dstStride = dstPic->getStride();
  Int depStride =  pPicBaseDepth->getStride();
  Int depthPosX = Clip3(0,   widthLuma - sizeX,  (posX/nTxtPerDepthX) + ((dv->getHor()+2)>>2));
  Int depthPosY = Clip3(0,   heightLuma- sizeY,  (posY/nTxtPerDepthY) + ((dv->getVer()+2)>>2));
  Pel *ref    = refPic->getLumaAddr() + posX + posY * refStride;
  Pel *dst    = dstPic->getLumaAddr(partAddr);
  Pel *depth  = pPicBaseDepth->getLumaAddr() + depthPosX + depthPosY * depStride;

#if H_3D_VSP_BLOCKSIZE != 1
#if H_3D_VSP_BLOCKSIZE == 2
  Int  dW = sizeX>>1;
  Int  dH = sizeY>>1;
#endif
#if H_3D_VSP_BLOCKSIZE == 4
  Int  dW = sizeX>>2;
  Int  dH = sizeY>>2;
#endif
  {
    Pel* depthi = depth;
    for (Int j = 0; j < dH; j++)
    {
      for (Int i = 0; i < dW; i++)
      {
        Pel* depthTmp;
#if H_3D_VSP_BLOCKSIZE == 2
        if (depthPosX + (i<<1) < widthDepth)
          depthTmp = depthi + (i << 1);
        else
          depthTmp = depthi + (widthDepth - depthPosX - 1);
#endif
#if H_3D_VSP_BLOCKSIZE == 4
        if (depthPosX + (i<<2) < widthDepth)
          depthTmp = depthi + (i << 2);
        else
          depthTmp = depthi + (widthDepth - depthPosX - 1);
#endif
        Int maxV = 0;
        for (Int blockj = 0; blockj < H_3D_VSP_BLOCKSIZE; blockj+=(H_3D_VSP_BLOCKSIZE-1))
        {
          Int iX = 0;
          for (Int blocki = 0; blocki < H_3D_VSP_BLOCKSIZE; blocki+=(H_3D_VSP_BLOCKSIZE-1))
          {
            if (maxV < depthTmp[iX])
              maxV = depthTmp[iX];
#if H_3D_VSP_BLOCKSIZE == 2
            if (depthPosX + (i<<1) + blocki < widthDepth - 1)
#else // H_3D_VSP_BLOCKSIZE == 4
            if (depthPosX + (i<<2) + blocki < widthDepth - 1)
#endif
              iX = (H_3D_VSP_BLOCKSIZE-1);
          }
#if H_3D_VSP_BLOCKSIZE == 2
          if (depthPosY + (j<<1) + blockj < heightDepth - 1)
#else // H_3D_VSP_BLOCKSIZE == 4
          if (depthPosY + (j<<2) + blockj < heightDepth - 1)
#endif
            depthTmp += depStride * (H_3D_VSP_BLOCKSIZE-1);
        }
        m_pDepthBlock[i+j*dW] = maxV;
      } // end of i < dW
#if H_3D_VSP_BLOCKSIZE == 2
      if (depthPosY + ((j+1)<<1) < heightDepth)
        depthi += (depStride << 1);
      else
        depthi  = depth + (heightDepth-depthPosY-1)*depStride;
#endif
#if H_3D_VSP_BLOCKSIZE == 4
      if (depthPosY + ((j+1)<<2) < heightDepth) // heightDepth-1
        depthi += (depStride << 2);
      else
        depthi  = depth + (heightDepth-depthPosY-1)*depStride; // the last line
#endif
    }
  }
#endif // H_3D_VSP_BLOCKSIZE != 1

#if H_3D_VSP_BLOCKSIZE == 1
#if H_3D_VSP_CONSTRAINED
  //get LUT based horizontal reference range
  Int range = xGetConstrainedSize(sizeX, sizeY);

  // The minimum depth value
  Int minRelativePos = MAX_INT;
  Int maxRelativePos = MIN_INT;

  Pel* depthTemp, *depthInitial=depth;
  for (Int yTxt = 0; yTxt < sizeY; yTxt++)
  {
    for (Int xTxt = 0; xTxt < sizeX; xTxt++)
    {
      if (depthPosX+xTxt < widthDepth)
        depthTemp = depthInitial + xTxt;
      else
        depthTemp = depthInitial + (widthDepth - depthPosX - 1);

      Int disparity = pShiftLUT[ *depthTemp ]; // << iShiftPrec;
      Int disparityInt = disparity >> 2;

      if( disparity <= 0)
      {
        if (minRelativePos > disparityInt+xTxt)
            minRelativePos = disparityInt+xTxt;
      }
      else
      {
        if (maxRelativePos < disparityInt+xTxt)
            maxRelativePos = disparityInt+xTxt;
      }
    }
    if (depthPosY+yTxt < heightDepth)
      depthInitial = depthInitial + depStride;
  }

  Int disparity_tmp = pShiftLUT[ *depth ]; // << iShiftPrec;
  if (disparity_tmp <= 0)
    maxRelativePos = minRelativePos + range -1 ;
  else
    minRelativePos = maxRelativePos - range +1 ;
#endif
#endif // H_3D_VSP_BLOCKSIZE == 1

#if H_3D_VSP_BLOCKSIZE != 1
  Int yDepth = 0;
#endif
  for ( Int yTxt = 0; yTxt < sizeY; yTxt += nTxtPerDepthY )
  {
    for ( Int xTxt = 0, xDepth = 0; xTxt < sizeX; xTxt += nTxtPerDepthX, xDepth++ )
    {
      Pel repDepth = 0; // to store the depth value used for warping
#if H_3D_VSP_BLOCKSIZE == 1
      repDepth = depth[xDepth];
#endif
#if H_3D_VSP_BLOCKSIZE == 2
      repDepth = m_pDepthBlock[(xTxt>>1) + (yTxt>>1)*dW];
#endif
#if H_3D_VSP_BLOCKSIZE == 4
      repDepth = m_pDepthBlock[(xTxt>>2) + (yTxt>>2)*dW];
#endif

      assert( repDepth >= 0 && repDepth <= 255 );
      Int disparity = pShiftLUT[ repDepth ]; // remove << iShiftPrec ??
      Int refOffset = xTxt + (disparity >> 2);
      Int xFrac = disparity & 0x3;
#if H_3D_VSP_CONSTRAINED
      if(refOffset<minRelativePos || refOffset>maxRelativePos)
        xFrac = 0;
      refOffset = Clip3(minRelativePos, maxRelativePos, refOffset);
#endif
      Int absX  = posX + refOffset;

      if (xFrac == 0)
        absX = Clip3(0, widthLuma-1, absX);
      else
        absX = Clip3(4, widthLuma-5, absX);

      refOffset = absX - posX;

      assert( ref[refOffset] >= 0 && ref[refOffset]<= 255 );
      m_if.filterHorLuma( &ref[refOffset], refStride, &dst[xTxt], dstStride, nTxtPerDepthX, nTxtPerDepthY, xFrac, !bi );
    }
    ref   += refStride*nTxtPerDepthY;
    dst   += dstStride*nTxtPerDepthY;
    depth += depStride;
#if H_3D_VSP_BLOCKSIZE != 1
    yDepth++;
#endif

  }
}

Void TComPrediction::xPredInterChromaBlkFromDM ( TComPicYuv *refPic, TComPicYuv *pPicBaseDepth, Int* pShiftLUT, TComMv*dv, UInt partAddr, Int posX, Int posY
                                               , Int sizeX, Int sizeY, Bool isDepth, TComYuv *&dstPic, Bool bi)
{
  Int refStride = refPic->getCStride();
  Int dstStride = dstPic->getCStride();
  Int depStride = pPicBaseDepth->getStride();

  Int widthChroma, heightChroma;
  if( isDepth)
  {
     widthChroma   = pPicBaseDepth->getWidth()>>1;
     heightChroma  = pPicBaseDepth->getHeight()>>1;
  }
  else
  {
     widthChroma   = refPic->getWidth()>>1;
     heightChroma  = refPic->getHeight()>>1;
  }

  // Below is only for Texture chroma component

  Int widthDepth  = pPicBaseDepth->getWidth();
  Int heightDepth = pPicBaseDepth->getHeight();

  Int nTxtPerDepthX, nTxtPerDepthY;  // Number of texture samples per one depth sample
  Int nDepthPerTxtX, nDepthPerTxtY;  // Number of depth samples per one texture sample

  Int depthPosX;  // Starting position in depth image
  Int depthPosY;

  if ( widthChroma > widthDepth )
  {
    nTxtPerDepthX = widthChroma / widthDepth;
    nDepthPerTxtX = 1;
    depthPosX = posX / nTxtPerDepthX + ((dv->getHor()+2)>>2);
  }
  else
  {
    nTxtPerDepthX = 1;
    nDepthPerTxtX = widthDepth / widthChroma;
    depthPosX = posX * nDepthPerTxtX + ((dv->getHor()+2)>>2);
  }
  depthPosX = Clip3(0, widthDepth - (sizeX<<1), depthPosX);
  if ( heightChroma > heightDepth )
  {
    nTxtPerDepthY = heightChroma / heightDepth;
    nDepthPerTxtY = 1;
    depthPosY = posY / nTxtPerDepthY + ((dv->getVer()+2)>>2);
  }
  else
  {
    nTxtPerDepthY = 1;
    nDepthPerTxtY = heightDepth / heightChroma;
    depthPosY = posY * nDepthPerTxtY + ((dv->getVer()+2)>>2);
  }
  depthPosY = Clip3(0, heightDepth - (sizeY<<1), depthPosY);

  Pel *refCb  = refPic->getCbAddr() + posX + posY * refStride;
  Pel *refCr  = refPic->getCrAddr() + posX + posY * refStride;
  Pel *dstCb  = dstPic->getCbAddr(partAddr);
  Pel *dstCr  = dstPic->getCrAddr(partAddr);
  Pel *depth  = pPicBaseDepth->getLumaAddr() + depthPosX + depthPosY * depStride;  // move the pointer to the current depth pixel position

  Int refStrideBlock = refStride * nTxtPerDepthY;
  Int dstStrideBlock = dstStride * nTxtPerDepthY;
  Int depStrideBlock = depStride * nDepthPerTxtY;

  if ( widthChroma > widthDepth ) // We assume
  {
    assert( heightChroma > heightDepth );
    printf("This branch should never been reached.\n");
    exit(0);
  }
  else
  {
#if H_3D_VSP_BLOCKSIZE == 1
  Int  dW = sizeX;
  Int  dH = sizeY;
  Int  sW = 2; // search window size
  Int  sH = 2;
#endif
#if H_3D_VSP_BLOCKSIZE == 2
  Int  dW = sizeX;
  Int  dH = sizeY;
  Int  sW = 2; // search window size
  Int  sH = 2;
#endif
#if H_3D_VSP_BLOCKSIZE == 4
  Int  dW = sizeX>>1;
  Int  dH = sizeY>>1;
  Int  sW = 4; // search window size
  Int  sH = 4;
#endif

  {
    Pel* depthi = depth;
    for (Int j = 0; j < dH; j++)
    {
      for (Int i = 0; i < dW; i++)
      {
        Pel* depthTmp;
#if H_3D_VSP_BLOCKSIZE == 1
        depthTmp = depthi + (i << 1);
#endif
#if H_3D_VSP_BLOCKSIZE == 2
        if (depthPosX + (i<<1) < widthDepth)
          depthTmp = depthi + (i << 1);
        else
          depthTmp = depthi + (widthDepth - depthPosX - 1);
#endif
#if H_3D_VSP_BLOCKSIZE == 4
        if (depthPosX + (i<<2) < widthDepth)
          depthTmp = depthi + (i << 2);
        else
          depthTmp = depthi + (widthDepth - depthPosX - 1);
#endif
        Int maxV = 0;
        for (Int blockj = 0; blockj < sH; blockj+=(sH-1))
        {
          Int iX = 0;
          for (Int blocki = 0; blocki < sW; blocki+=(sW-1))
          {
            if (maxV < depthTmp[iX])
              maxV = depthTmp[iX];
            if (depthPosX + i*sW + blocki < widthDepth - 1)
                iX = (sW-1);
          }
          if (depthPosY + j*sH + blockj < heightDepth - 1)
                depthTmp += depStride * (sH-1);
        }
        m_pDepthBlock[i+j*dW] = maxV;
      } // end of i < dW
#if H_3D_VSP_BLOCKSIZE == 1
      if (depthPosY + ((j+1)<<1) < heightDepth)
        depthi += (depStride << 1);
      else
        depthi  = depth + (heightDepth-1)*depStride;
#endif
#if H_3D_VSP_BLOCKSIZE == 2
      if (depthPosY + ((j+1)<<1) < heightDepth)
        depthi += (depStride << 1);
      else
        depthi  = depth + (heightDepth-depthPosY-1)*depStride;
#endif
#if H_3D_VSP_BLOCKSIZE == 4
      if (depthPosY + ((j+1)<<2) < heightDepth) // heightDepth-1
        depthi += (depStride << 2);
      else
        depthi  = depth + (heightDepth-depthPosY-1)*depStride; // the last line
#endif
    }
  }


#if H_3D_VSP_BLOCKSIZE == 1
#if H_3D_VSP_CONSTRAINED
  //get LUT based horizontal reference range
  Int range = xGetConstrainedSize(sizeX, sizeY, false);

  // The minimum depth value
  Int minRelativePos = MAX_INT;
  Int maxRelativePos = MIN_INT;

  Int depthTmp;
  for (Int yTxt=0; yTxt<sizeY; yTxt++)
  {
    for (Int xTxt=0; xTxt<sizeX; xTxt++)
    {
      depthTmp = m_pDepthBlock[xTxt+yTxt*dW];
      Int disparity = pShiftLUT[ depthTmp ]; // << iShiftPrec;
      Int disparityInt = disparity >> 3;//in chroma resolution

      if (disparityInt < 0)
      {
        if (minRelativePos > disparityInt+xTxt)
            minRelativePos = disparityInt+xTxt;
      }
      else
      {
        if (maxRelativePos < disparityInt+xTxt)
            maxRelativePos = disparityInt+xTxt;
      }
    }
  }

  depthTmp = m_pDepthBlock[0];
  Int disparity_tmp = pShiftLUT[ depthTmp ]; // << iShiftPrec;
  if ( disparity_tmp < 0 )
    maxRelativePos = minRelativePos + range - 1;
  else
    minRelativePos = maxRelativePos - range + 1;

#endif // H_3D_VSP_CONSTRAINED
#endif // H_3D_VSP_BLOCKSIZE == 1

    // (sizeX, sizeY) is Chroma block size
    for ( Int yTxt = 0, yDepth = 0; yTxt < sizeY; yTxt += nTxtPerDepthY, yDepth += nDepthPerTxtY )
    {
      for ( Int xTxt = 0, xDepth = 0; xTxt < sizeX; xTxt += nTxtPerDepthX, xDepth += nDepthPerTxtX )
      {
        Pel repDepth = 0; // to store the depth value used for warping
#if H_3D_VSP_BLOCKSIZE == 1
        repDepth = m_pDepthBlock[(xTxt) + (yTxt)*dW];
#endif
#if H_3D_VSP_BLOCKSIZE == 2
        repDepth = m_pDepthBlock[(xTxt) + (yTxt)*dW];
#endif
#if H_3D_VSP_BLOCKSIZE == 4
        repDepth = m_pDepthBlock[(xTxt>>1) + (yTxt>>1)*dW];
#endif

      // calculate the offset in the reference picture
        Int disparity = pShiftLUT[ repDepth ]; // Remove << iShiftPrec;
        Int refOffset = xTxt + (disparity >> 3); // in integer pixel in chroma image
        Int xFrac = disparity & 0x7;
#if H_3D_VSP_CONSTRAINED
        if(refOffset < minRelativePos || refOffset > maxRelativePos)
          xFrac = 0;
        refOffset = Clip3(minRelativePos, maxRelativePos, refOffset);
#endif
        Int absX  = posX + refOffset;

        if (xFrac == 0)
          absX = Clip3(0, widthChroma-1, absX);
        else
          absX = Clip3(4, widthChroma-5, absX);

        refOffset = absX - posX;

        assert( refCb[refOffset] >= 0 && refCb[refOffset]<= 255 );
        assert( refCr[refOffset] >= 0 && refCr[refOffset]<= 255 );
        m_if.filterHorChroma(&refCb[refOffset], refStride, &dstCb[xTxt],  dstStride, nTxtPerDepthX, nTxtPerDepthY, xFrac, !bi);
        m_if.filterHorChroma(&refCr[refOffset], refStride, &dstCr[xTxt],  dstStride, nTxtPerDepthX, nTxtPerDepthY, xFrac, !bi);
      }
      refCb += refStrideBlock;
      refCr += refStrideBlock;
      dstCb += dstStrideBlock;
      dstCr += dstStrideBlock;
      depth += depStrideBlock;
    }
  }

}

#if H_3D_VSP_CONSTRAINED
Int TComPrediction::xGetConstrainedSize(Int nPbW, Int nPbH, Bool bLuma)
{
  Int iSize = 0;
  if (bLuma)
  {
    Int iArea = (nPbW+7) * (nPbH+7);
    Int iAlpha = iArea / nPbH - nPbW - 7;
    iSize = iAlpha + nPbW;
  }
  else // chroma
  {
    Int iArea = (nPbW+2) * (nPbH+2);
    Int iAlpha = iArea / nPbH - nPbW - 4;
    iSize = iAlpha + nPbW;
  }
  return iSize;
}
#endif // H_3D_VSP_CONSTRAINED

#endif // H_3D_VSP

#if H_3D_DIM
Void TComPrediction::xPredBiSegDCs( Int* ptrSrc, UInt srcStride, Bool* biSegPattern, Int patternStride, Pel& predDC1, Pel& predDC2 )
{
  Int  refDC1, refDC2;
  const Int  iTR = (   patternStride - 1        ) - srcStride;
  const Int  iTM = ( ( patternStride - 1 ) >> 1 ) - srcStride;
  const Int  iLB = (   patternStride - 1        ) * srcStride - 1;
  const Int  iLM = ( ( patternStride - 1 ) >> 1 ) * srcStride - 1;

  Bool bL = ( biSegPattern[0] != biSegPattern[(patternStride-1)*patternStride] );
  Bool bT = ( biSegPattern[0] != biSegPattern[(patternStride-1)]               );

  if( bL == bT )
  {
    refDC1 = bL ? ( ptrSrc[iTR] + ptrSrc[iLB] )>>1 : 1<<( g_bitDepthY - 1 );
    refDC2 =      ( ptrSrc[ -1] + ptrSrc[-(Int)srcStride] )>>1;
  }
  else
  {
    refDC1 = bL ? ptrSrc[iLB] : ptrSrc[iTR];
    refDC2 = bL ? ptrSrc[iTM] : ptrSrc[iLM];
  }

  predDC1 = biSegPattern[0] ? refDC1 : refDC2;
  predDC2 = biSegPattern[0] ? refDC2 : refDC1;
}

Void TComPrediction::xAssignBiSegDCs( Pel* ptrDst, UInt dstStride, Bool* biSegPattern, Int patternStride, Pel valDC1, Pel valDC2 )
{
  if( dstStride == patternStride )
  {
    for( UInt k = 0; k < (patternStride * patternStride); k++ )
    {
      if( true == biSegPattern[k] ) { ptrDst[k] = valDC2; }
      else                          { ptrDst[k] = valDC1; }
    }
  }
  else
  {
    Pel* piTemp = ptrDst;
    for( UInt uiY = 0; uiY < patternStride; uiY++ )
    {
      for( UInt uiX = 0; uiX < patternStride; uiX++ )
      {
        if( true == biSegPattern[uiX] ) { piTemp[uiX] = valDC2; }
        else                            { piTemp[uiX] = valDC1; }
      }
      piTemp       += dstStride;
      biSegPattern += patternStride;
    }
  }
}

#if H_3D_DIM_DMM
UInt TComPrediction::xPredWedgeFromIntra( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiWidth, UInt uiHeight, Int iDeltaEnd )
{
  UInt uiThisBlockSize = uiWidth;

  TComDataCU* pcTempCU;
  UInt        uiTempPartIdx;
  // 1st: try continue above wedgelet
  pcTempCU = pcCU->getPUAbove( uiTempPartIdx, pcCU->getZorderIdxInCU() + uiAbsPartIdx );
  if( pcTempCU && isDimMode( pcTempCU->getLumaIntraDir( uiTempPartIdx ) ) )
  {
    UInt dimType =  getDimType( pcTempCU->getLumaIntraDir( uiTempPartIdx ) );
    if( DMM1_IDX == dimType || DMM2_IDX == dimType || DMM3_IDX == dimType )
    {
      // get offset between current and reference block
      UInt uiOffsetX = 0, uiOffsetY = 0;
      xGetBlockOffset( pcCU, uiAbsPartIdx, pcTempCU, uiTempPartIdx, uiOffsetX, uiOffsetY );

      // get reference wedgelet
      WedgeList* pacWedgeList = &g_dmmWedgeLists[(g_aucConvertToBit[(pcTempCU->getWidth( uiTempPartIdx )>>((pcTempCU->getPartitionSize( uiTempPartIdx ) == SIZE_NxN) ? 1 : 0))])];
      TComWedgelet* pcRefWedgelet = &(pacWedgeList->at( pcTempCU->getDmmWedgeTabIdx( dimType, uiTempPartIdx ) ) );

      // find wedgelet, if direction is suitable for continue wedge
      if( pcRefWedgelet->checkPredDirAbovePossible( uiThisBlockSize, uiOffsetX ) )
      {
        UChar uhContD_Xs, uhContD_Ys, uhContD_Xe, uhContD_Ye;
        pcRefWedgelet->getPredDirStartEndAbove( uhContD_Xs, uhContD_Ys, uhContD_Xe, uhContD_Ye, uiThisBlockSize, uiOffsetX, iDeltaEnd );
        return xGetWedgePatternIdx( uiThisBlockSize, uhContD_Xs, uhContD_Ys, uhContD_Xe, uhContD_Ye );
      }
    }
  }

  // 2nd: try continue left wedglelet
  pcTempCU = pcCU->getPULeft( uiTempPartIdx, pcCU->getZorderIdxInCU() + uiAbsPartIdx );
  if( pcTempCU && isDimMode( pcTempCU->getLumaIntraDir( uiTempPartIdx ) ) )
  {
    UInt dimType = getDimType( pcTempCU->getLumaIntraDir( uiTempPartIdx ) );
    if( DMM1_IDX == dimType || DMM2_IDX == dimType || DMM3_IDX == dimType )
    {
      // get offset between current and reference block
      UInt uiOffsetX = 0, uiOffsetY = 0;
      xGetBlockOffset( pcCU, uiAbsPartIdx, pcTempCU, uiTempPartIdx, uiOffsetX, uiOffsetY );

      // get reference wedgelet
      WedgeList* pacWedgeList = &g_dmmWedgeLists[(g_aucConvertToBit[(pcTempCU->getWidth( uiTempPartIdx )>>((pcTempCU->getPartitionSize( uiTempPartIdx ) == SIZE_NxN) ? 1 : 0))])];
      TComWedgelet* pcRefWedgelet = &(pacWedgeList->at( pcTempCU->getDmmWedgeTabIdx( dimType, uiTempPartIdx ) ) );

      // find wedgelet, if direction is suitable for continue wedge
      if( pcRefWedgelet->checkPredDirLeftPossible( uiThisBlockSize, uiOffsetY ) )
      {
        UChar uhContD_Xs, uhContD_Ys, uhContD_Xe, uhContD_Ye;
        pcRefWedgelet->getPredDirStartEndLeft( uhContD_Xs, uhContD_Ys, uhContD_Xe, uhContD_Ye, uiThisBlockSize, uiOffsetY, iDeltaEnd );
        return xGetWedgePatternIdx( uiThisBlockSize, uhContD_Xs, uhContD_Ys, uhContD_Xe, uhContD_Ye );
      }
    }
  }

  // 3rd: (default) make wedglet from intra dir and max slope point
  Int iSlopeX = 0, iSlopeY = 0;
  UInt uiStartPosX = 0, uiStartPosY = 0;
  if( xGetWedgeIntraDirPredData( pcCU, uiAbsPartIdx, uiThisBlockSize, iSlopeX, iSlopeY, uiStartPosX, uiStartPosY ) )
  {
    UChar uhContD_Xs, uhContD_Ys, uhContD_Xe, uhContD_Ye;
    xGetWedgeIntraDirStartEnd( pcCU, uiAbsPartIdx, uiThisBlockSize, iSlopeX, iSlopeY, uiStartPosX, uiStartPosY, uhContD_Xs, uhContD_Ys, uhContD_Xe, uhContD_Ye, iDeltaEnd );
    return xGetWedgePatternIdx( uiThisBlockSize, uhContD_Xs, uhContD_Ys, uhContD_Xe, uhContD_Ye );
  }

  return 0;
}

UInt TComPrediction::xPredWedgeFromTex( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiWidth, UInt uiHeight, UInt intraTabIdx )
{
  TComPic*      pcPicTex = pcCU->getSlice()->getTexturePic();
  assert( pcPicTex != NULL );
  TComDataCU*   pcColTexCU = pcPicTex->getCU(pcCU->getAddr());
  UInt          uiTexPartIdx = pcCU->getZorderIdxInCU() + uiAbsPartIdx;
  Int           uiColTexIntraDir = pcColTexCU->isIntra( uiTexPartIdx ) ? pcColTexCU->getLumaIntraDir( uiTexPartIdx ) : 255;

  if( uiColTexIntraDir > DC_IDX && uiColTexIntraDir < 35 ) { return g_aauiWdgLstM3[g_aucConvertToBit[uiWidth]][uiColTexIntraDir-2].at(intraTabIdx); }
  else                                                     { return g_dmmWedgeNodeLists[(g_aucConvertToBit[uiWidth])].at(intraTabIdx).getPatternIdx(); }
}

Void TComPrediction::xPredContourFromTex( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiWidth, UInt uiHeight, TComWedgelet* pcContourWedge )
{
  pcContourWedge->clear();

  // get copy of co-located texture luma block
  TComYuv cTempYuv;
  cTempYuv.create( uiWidth, uiHeight ); 
  cTempYuv.clear();
  Pel* piRefBlkY = cTempYuv.getLumaAddr();
  xCopyTextureLumaBlock( pcCU, uiAbsPartIdx, piRefBlkY, uiWidth, uiHeight );
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


Void TComPrediction::xCopyTextureLumaBlock( TComDataCU* pcCU, UInt uiAbsPartIdx, Pel* piDestBlockY, UInt uiWidth, UInt uiHeight )
{
  TComPicYuv* pcPicYuvRef = pcCU->getSlice()->getTexturePic()->getPicYuvRec();
  assert( pcPicYuvRef != NULL );
  Int         iRefStride = pcPicYuvRef->getStride();
  Pel*        piRefY = pcPicYuvRef->getLumaAddr( pcCU->getAddr(), pcCU->getZorderIdxInCU() + uiAbsPartIdx );

  for ( Int y = 0; y < uiHeight; y++ )
  {
    ::memcpy(piDestBlockY, piRefY, sizeof(Pel)*uiWidth);
    piDestBlockY += uiWidth;
    piRefY += iRefStride;
  }
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
  riSlopeX = 0, riSlopeY = 0, ruiStartPosX = 0, ruiStartPosY = 0;

  // 1st step: get wedge start point (max. slope)
  Int* piSource = pcCU->getPattern()->getAdiOrgBuf( uiBlockSize, uiBlockSize, m_piYuvExt );
  Int iSourceStride = ( uiBlockSize<<1 ) + 1;

  UInt uiSlopeMaxAbove = 0, uiPosSlopeMaxAbove = 0;
  for( UInt uiPosHor = 0; uiPosHor < (uiBlockSize-1); uiPosHor++ )
  {
    if( abs( piSource[uiPosHor+1] - piSource[uiPosHor] ) > uiSlopeMaxAbove )
    {
      uiSlopeMaxAbove = abs( piSource[uiPosHor+1] - piSource[uiPosHor] );
      uiPosSlopeMaxAbove = uiPosHor;
    }
  }

  UInt uiSlopeMaxLeft = 0, uiPosSlopeMaxLeft = 0;
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

  ruiStartPosX = ( uiSlopeMaxAbove >  uiSlopeMaxLeft  ) ? uiPosSlopeMaxAbove : 0;
  ruiStartPosY = ( uiSlopeMaxLeft  >= uiSlopeMaxAbove ) ? uiPosSlopeMaxLeft  : 0;

  // 2nd step: derive wedge direction
  Int uiPreds[3] = {-1, -1, -1};
  Int iMode = -1;
  Int iPredNum = pcCU->getIntraDirLumaPredictor( uiAbsPartIdx, uiPreds, &iMode );  

  UInt uiDirMode = 0;
  if( iMode >= 0 ) { iPredNum = iMode; }
  if( iPredNum == 1 ) { uiDirMode = uiPreds[0]; }
  if( iPredNum == 2 ) { uiDirMode = uiPreds[1]; }

  if( uiDirMode < 2 ) { return false; } // no planar & DC

  Bool modeHor       = (uiDirMode < 18);
  Bool modeVer       = !modeHor;
  Int intraPredAngle = modeVer ? (Int)uiDirMode - VER_IDX : modeHor ? -((Int)uiDirMode - HOR_IDX) : 0;
  Int absAng         = abs(intraPredAngle);
  Int signAng        = intraPredAngle < 0 ? -1 : 1;
  Int angTable[9]    = {0,2,5,9,13,17,21,26,32};
  absAng             = angTable[absAng];
  intraPredAngle     = signAng * absAng;

  // 3rd step: set slope for direction
  if( modeHor )
  {
    riSlopeX = ( intraPredAngle > 0 ) ?            -32 :              32;
    riSlopeY = ( intraPredAngle > 0 ) ? intraPredAngle : -intraPredAngle;
  }
  else if( modeVer )
  {
    riSlopeX = ( intraPredAngle > 0 ) ? intraPredAngle : -intraPredAngle;
    riSlopeY = ( intraPredAngle > 0 ) ?            -32 :              32;
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
  WedgeResolution eWedgeRes = g_dmmWedgeResolution[(UInt)g_aucConvertToBit[uiBlockSize]];
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

UInt TComPrediction::xGetWedgePatternIdx( UInt uiBlockSize, UChar uhXs, UChar uhYs, UChar uhXe, UChar uhYe )
{
  WedgeRefList* pcWedgeRefList = &g_dmmWedgeRefLists[(g_aucConvertToBit[uiBlockSize])];
  for( UInt uiIdx = 0; uiIdx < pcWedgeRefList->size(); uiIdx++ )
  {
    TComWedgeRef* pcTestWedgeRef = &(pcWedgeRefList->at(uiIdx));
    if( pcTestWedgeRef->getStartX() == uhXs && pcTestWedgeRef->getStartY() == uhYs && pcTestWedgeRef->getEndX() == uhXe && pcTestWedgeRef->getEndY() == uhYe )
    {
      return pcTestWedgeRef->getRefIdx();
    }
  }
  return 0;
}
#endif
#if H_3D_DIM_RBC
Void TComPrediction::xDeltaDCQuantScaleUp( TComDataCU* pcCU, Pel& rDeltaDC )
{
  Int  iSign  = rDeltaDC < 0 ? -1 : 1;
  UInt uiAbs  = abs( rDeltaDC );

  Int iQp = pcCU->getQP(0);
  Double dMax = (Double)( 1<<( g_bitDepthY - 1 ) );
  Double dStepSize = Clip3( 1.0, dMax, pow( 2.0, iQp/10.0 - 2.0 ) );

  rDeltaDC = iSign * roftoi( uiAbs * dStepSize );
  return;
}

Void TComPrediction::xDeltaDCQuantScaleDown( TComDataCU*  pcCU, Pel& rDeltaDC )
{
  Int  iSign  = rDeltaDC < 0 ? -1 : 1;
  UInt uiAbs  = abs( rDeltaDC );

  Int iQp = pcCU->getQP(0);
  Double dMax = (Double)( 1<<( g_bitDepthY - 1 ) );
  Double dStepSize = Clip3( 1.0, dMax, pow( 2.0, iQp/10.0 - 2.0 ) );

  rDeltaDC = iSign * roftoi( uiAbs / dStepSize );
  return;
}
#endif
#if H_3D_DIM_SDC
Void TComPrediction::analyzeSegmentsSDC( Pel* pOrig, UInt uiStride, UInt uiSize, Pel* rpSegMeans, UInt uiNumSegments, Bool* pMask, UInt uiMaskStride )
{
  Int iSumDepth[2];
  memset(iSumDepth, 0, sizeof(Int)*2);
  Int iSumPix[2];
  memset(iSumPix, 0, sizeof(Int)*2);
  
  Int subSamplePix;
  if ( uiSize == 64 || uiSize == 32 )
  {
    subSamplePix = 2;
  }
  else
  {
    subSamplePix = 1;
  }
  for (Int y=0; y<uiSize; y+=subSamplePix)
  {
    for (Int x=0; x<uiSize; x+=subSamplePix)
    {
      UChar ucSegment = pMask?(UChar)pMask[x]:0;
      assert( ucSegment < uiNumSegments );
      
      iSumDepth[ucSegment] += pOrig[x];
      iSumPix[ucSegment]   += 1;
    }
    
    pOrig  += uiStride*subSamplePix;
    pMask  += uiMaskStride*subSamplePix;
  }
  
  // compute mean for each segment
  for( UChar ucSeg = 0; ucSeg < uiNumSegments; ucSeg++ )
  {
    if( iSumPix[ucSeg] > 0 )
      rpSegMeans[ucSeg] = iSumDepth[ucSeg] / iSumPix[ucSeg];
    else
      rpSegMeans[ucSeg] = 0;  // this happens for zero-segments
  }
}
#endif // H_3D_DIM_SDC
#endif
//! \}
