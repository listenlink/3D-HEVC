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
  m_cYuvDepthOnVsp.destroy();
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
#if H_3D_VSP
    m_cYuvDepthOnVsp.create( g_uiMaxCUWidth, g_uiMaxCUHeight );
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
  m_uiaShift[0] = 0;
  for( Int i = 1; i < 64; i++ )
  {
    m_uiaShift[i] = ( (1 << 15) + i/2 ) / i;
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
#if QC_DIM_DELTADC_UNIFY_F0132 || HHI_DIM_PREDSAMP_FIX_F0171
  piMask = pcCU->getPattern()->getAdiOrgBuf( iWidth, iHeight, m_piYuvExt ); // no filtering
#else
  if( isDmmMode ) piMask = pcCU->getPattern()->getAdiOrgBuf( iWidth, iHeight, m_piYuvExt ); // no filtering for DMM
  else            piMask = pcCU->getPattern()->getPredictorPtr( 0, g_aucConvertToBit[ iWidth ] + 2, m_piYuvExt );
#endif
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
#if QC_DIM_DELTADC_UNIFY_F0132
    if( isDmmMode || isRbcMode)
#else
    if( isDmmMode )
#endif
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
#if H_3D_DIM_RBC && !QC_DIM_DELTADC_UNIFY_F0132
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
    if ( pcCU->getVSPFlag(uiPartAddr) == 0)
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
      {
        xPredInterUniVSP( pcCU, uiPartAddr, iWidth, iHeight, REF_PIC_LIST_0, pcYuvPred );
      }
      else
      {
        xPredInterBiVSP ( pcCU, uiPartAddr, iWidth, iHeight, pcYuvPred );
      }
    }
#endif
    return;
  }

  for ( iPartIdx = 0; iPartIdx < pcCU->getNumPartInter(); iPartIdx++ )
  {
    pcCU->getPartIndexAndSize( iPartIdx, uiPartAddr, iWidth, iHeight );

#if H_3D_VSP
    if ( pcCU->getVSPFlag(uiPartAddr) == 0 )
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
      {
        xPredInterUniVSP( pcCU, uiPartAddr, iWidth, iHeight, REF_PIC_LIST_0, pcYuvPred );
      }
      else
      {
        xPredInterBiVSP ( pcCU, uiPartAddr, iWidth, iHeight, pcYuvPred );
      }
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
    bICFlag = bICFlag && (iWidth > 8);
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
  Int       depthRefViewIdx = pcCU->getDvInfo(uiPartAddr).m_aVIdxCan;
#if H_3D_FCO_VSP_DONBDV_E0163
  TComPic* pRefPicBaseDepth = 0;
  Bool     bIsCurrDepthCoded = false;
  pRefPicBaseDepth  = pcCU->getSlice()->getIvPic( true, pcCU->getSlice()->getViewIndex() );
  if ( pRefPicBaseDepth->getPicYuvRec() != NULL  ) 
  {
    bIsCurrDepthCoded = true;
  }
  else 
  {
    pRefPicBaseDepth = pcCU->getSlice()->getIvPic (true, depthRefViewIdx );
  }
#else
  TComPic* pRefPicBaseDepth = pcCU->getSlice()->getIvPic (true, depthRefViewIdx );
#endif
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
  TComMv cDv  = pcCU->getDvInfo(uiPartAddr).m_acNBDV;
  pcCU->clipMv(cDv);

#if H_3D_FCO_VSP_DONBDV_E0163
  if ( bIsCurrDepthCoded )
  {
      cDv.setZero();
  }
#endif
  // fetch virtual depth map
  pcBaseViewDepthPicYuv->extendPicBorder();
  xGetVirtualDepth( pcCU, pcBaseViewDepthPicYuv, &cDv, uiPartAddr, iWidth, iHeight, &m_cYuvDepthOnVsp );
  // sub-PU based compensation
  xPredInterLumaBlkFromDM   ( pcCU, pcBaseViewTxtPicYuv, &m_cYuvDepthOnVsp, pShiftLUT, &cDv, uiPartAddr, iWidth, iHeight, pcCU->getSlice()->getIsDepth(), rpcYuvPred, bi );
  xPredInterChromaBlkFromDM ( pcCU, pcBaseViewTxtPicYuv, &m_cYuvDepthOnVsp, pShiftLUT, &cDv, uiPartAddr, iWidth, iHeight, pcCU->getSlice()->getIsDepth(), rpcYuvPred, bi );
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
    {
      continue;
    }
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
#if H_3D_IC
    m_if.filterHorLuma( ref, refStride, dst, dstStride, width, height, xFrac,       !bi || bICFlag
#else
    m_if.filterHorLuma( ref, refStride, dst, dstStride, width, height, xFrac,       !bi 
#endif
#if H_3D_ARP
    , filterType
#endif
      );
  }
  else if ( xFrac == 0 )
  {
#if H_3D_IC
    m_if.filterVerLuma( ref, refStride, dst, dstStride, width, height, yFrac, true, !bi || bICFlag
#else
    m_if.filterVerLuma( ref, refStride, dst, dstStride, width, height, yFrac, true, !bi 
#endif
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
#if H_3D_IC
    m_if.filterVerLuma(tmp + (halfFilterSize-1)*tmpStride, tmpStride, dst, dstStride, width, height,              yFrac, false, !bi || bICFlag
#else
    m_if.filterVerLuma(tmp + (halfFilterSize-1)*tmpStride, tmpStride, dst, dstStride, width, height,              yFrac, false, !bi
#endif
#if H_3D_ARP
    , filterType
#endif 
      );    
  }

#if H_3D_IC
  if( bICFlag )
  {
    Int a, b, i, j;
    const Int iShift = IC_CONST_SHIFT;

    xGetLLSICPrediction( cu, mv, refPic, a, b, TEXT_LUMA );


    for ( i = 0; i < height; i++ )
    {
      for ( j = 0; j < width; j++ )
      {
          dst[j] = Clip3( 0, ( 1 << g_bitDepthY ) - 1, ( ( a*dst[j] ) >> iShift ) + b );
      }
      dst += dstStride;
    }

    if(bi)
    {
      Pel *dst2      = dstPic->getLumaAddr( partAddr );
      Int shift = IF_INTERNAL_PREC - g_bitDepthY;
      for (i = 0; i < height; i++)
      {
        for (j = 0; j < width; j++)
        {
          Short val = dst2[j] << shift;
          dst2[j] = val - (Short)IF_INTERNAL_OFFS;
        }
        dst2 += dstStride;
      }
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
#if H_3D_IC
    m_if.filterHorChroma(refCb, refStride, dstCb,  dstStride, cxWidth, cxHeight, xFrac, !bi || bICFlag
#else
    m_if.filterHorChroma(refCb, refStride, dstCb,  dstStride, cxWidth, cxHeight, xFrac, !bi
#endif
#if H_3D_ARP
    , filterType
#endif
    );    
#if H_3D_IC
    m_if.filterHorChroma(refCr, refStride, dstCr,  dstStride, cxWidth, cxHeight, xFrac, !bi || bICFlag
#else
    m_if.filterHorChroma(refCr, refStride, dstCr,  dstStride, cxWidth, cxHeight, xFrac, !bi
#endif
#if H_3D_ARP
    , filterType
#endif
    );
  }
  else if ( xFrac == 0 )
  {
#if H_3D_IC
    m_if.filterVerChroma(refCb, refStride, dstCb, dstStride, cxWidth, cxHeight, yFrac, true, !bi || bICFlag
#else
    m_if.filterVerChroma(refCb, refStride, dstCb, dstStride, cxWidth, cxHeight, yFrac, true, !bi
#endif
#if H_3D_ARP
    , filterType
#endif
    );
#if H_3D_IC
    m_if.filterVerChroma(refCr, refStride, dstCr, dstStride, cxWidth, cxHeight, yFrac, true, !bi || bICFlag
#else
    m_if.filterVerChroma(refCr, refStride, dstCr, dstStride, cxWidth, cxHeight, yFrac, true, !bi
#endif
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
#if H_3D_IC
    m_if.filterVerChroma(extY  + (halfFilterSize-1)*extStride, extStride, dstCb, dstStride, cxWidth, cxHeight  , yFrac, false, !bi || bICFlag
#else
    m_if.filterVerChroma(extY  + (halfFilterSize-1)*extStride, extStride, dstCb, dstStride, cxWidth, cxHeight  , yFrac, false, !bi
#endif
#if H_3D_ARP
    , filterType
#endif 
      );
    
    m_if.filterHorChroma(refCr - (halfFilterSize-1)*refStride, refStride, extY,  extStride, cxWidth, cxHeight+filterSize-1, xFrac, false
#if H_3D_ARP
    , filterType
#endif 
      );
#if H_3D_IC
    m_if.filterVerChroma(extY  + (halfFilterSize-1)*extStride, extStride, dstCr, dstStride, cxWidth, cxHeight  , yFrac, false, !bi || bICFlag
#else
    m_if.filterVerChroma(extY  + (halfFilterSize-1)*extStride, extStride, dstCr, dstStride, cxWidth, cxHeight  , yFrac, false, !bi
#endif
#if H_3D_ARP
    , filterType
#endif 
      );    
  }

#if H_3D_IC
  if( bICFlag )
  {
    Int a, b, i, j;
    const Int iShift = IC_CONST_SHIFT;
    xGetLLSICPrediction( cu, mv, refPic, a, b, TEXT_CHROMA_U ); // Cb
    for ( i = 0; i < cxHeight; i++ )
    {
      for ( j = 0; j < cxWidth; j++ )
      {
          dstCb[j] = Clip3(  0, ( 1 << g_bitDepthC ) - 1, ( ( a*dstCb[j] ) >> iShift ) + b );
      }
      dstCb += dstStride;
    }
    xGetLLSICPrediction( cu, mv, refPic, a, b, TEXT_CHROMA_V ); // Cr
    for ( i = 0; i < cxHeight; i++ )
    {
      for ( j = 0; j < cxWidth; j++ )
      {
          dstCr[j] = Clip3( 0, ( 1 << g_bitDepthC ) - 1, ( ( a*dstCr[j] ) >> iShift ) + b );
      }
      dstCr += dstStride;
    }

    if(bi)
    {
      Pel* dstCb2 = dstPic->getCbAddr( partAddr );
      Pel* dstCr2 = dstPic->getCrAddr( partAddr );
      Int shift = IF_INTERNAL_PREC - g_bitDepthC;
      for (i = 0; i < cxHeight; i++)
      {
        for (j = 0; j < cxWidth; j++)
        {
          Short val = dstCb2[j] << shift;
          dstCb2[j] = val - (Short)IF_INTERNAL_OFFS;

          val = dstCr2[j] << shift;
          dstCr2[j] = val - (Short)IF_INTERNAL_OFFS;
        }
        dstCb2 += dstStride;
        dstCr2 += dstStride;
      }
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
  Int leftColumn[MAX_CU_SIZE+1], topRow[MAX_CU_SIZE+1], bottomRow[MAX_CU_SIZE], rightColumn[MAX_CU_SIZE];
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


/** Function for deriving LM illumination compensation.
 */
Void TComPrediction::xGetLLSICPrediction( TComDataCU* pcCU, TComMv *pMv, TComPicYuv *pRefPic, Int &a, Int &b, TextType eType )
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
  Int precShift = std::max(0, (( eType == TEXT_LUMA ) ? g_bitDepthY : g_bitDepthC) - 12);

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

    for( j = 0; j < uiWidth; j+=2 )
    {
      x += pRef[j];
      y += pRec[j];
      xx += (pRef[j] * pRef[j])>>precShift;
      xy += (pRef[j] * pRec[j])>>precShift;
    }
    iCountShift += g_aucConvertToBit[ uiWidth ] + 1;
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

    for( i = 0; i < uiHeight; i+=2 )
    {
      x += pRef[0];
      y += pRec[0];

      xx += (pRef[0] * pRef[0])>>precShift;
      xy += (pRef[0] * pRec[0])>>precShift;

      pRef += iRefStride*2;
      pRec += iRecStride*2;
    }
    iCountShift += iCountShift > 0 ? 1 : ( g_aucConvertToBit[ uiWidth ] + 1 );
  }

  xy += xx >> IC_REG_COST_SHIFT;
  xx += xx >> IC_REG_COST_SHIFT;
  Int a1 = ( xy << iCountShift ) - ((y * x) >> precShift);
  Int a2 = ( xx << iCountShift ) - ((x * x) >> precShift);
  const Int iShift = IC_CONST_SHIFT;
  {
    {
      const Int iShiftA2 = 6;
      const Int iAccuracyShift = 15;

      Int iScaleShiftA2 = 0;
      Int iScaleShiftA1 = 0;
      Int a1s = a1;
      Int a2s = a2;

      a1 = Clip3(0, 2*a2, a1);
      iScaleShiftA2 = GetMSB( abs( a2 ) ) - iShiftA2;
      iScaleShiftA1 = iScaleShiftA2 - IC_SHIFT_DIFF;

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

      a = a1s * m_uiaShift[ a2s ];
      a = a >> iScaleShiftA;
      b = (  y - ( ( a * x ) >> iShift ) + ( 1 << ( iCountShift - 1 ) ) ) >> iCountShift;
    }
  }   
}
#endif

#if H_3D_VSP
// not fully support iRatioTxtPerDepth* != 1
Void TComPrediction::xGetVirtualDepth( TComDataCU *cu, TComPicYuv *picRefDepth, TComMv *mv, UInt partAddr, Int width, Int height, TComYuv *yuvDepth, Int ratioTxtPerDepthX, Int ratioTxtPerDepthY )
{
  Int nTxtPerDepthX = H_3D_VSP_BLOCKSIZE;
  Int nTxtPerDepthY = H_3D_VSP_BLOCKSIZE;

  Int refDepStride = picRefDepth->getStride();

  Int refDepOffset  = ( (mv->getHor()+2) >> 2 ) + ( (mv->getVer()+2) >> 2 ) * refDepStride;
  Pel *refDepth     = picRefDepth->getLumaAddr( cu->getAddr(), cu->getZorderIdxInCU() + partAddr );

  if( ratioTxtPerDepthX!=1 || ratioTxtPerDepthY!=1 )
  {
    Int posX, posY;
    refDepth    = picRefDepth->getLumaAddr( );
    cu->getPic()->getPicYuvRec()->getTopLeftSamplePos( cu->getAddr(), cu->getZorderIdxInCU() + partAddr, posX, posY ); // top-left position in texture
    posX /= ratioTxtPerDepthX; // texture position -> depth postion
    posY /= ratioTxtPerDepthY;
    refDepOffset += posX + posY * refDepStride;

    width  /= ratioTxtPerDepthX; // texture size -> depth size
    height /= ratioTxtPerDepthY;
  }

  refDepth += refDepOffset;

  Int depStride = yuvDepth->getStride();
  Pel *depth = yuvDepth->getLumaAddr();

  if( width<8 || height<8 )
  { // no split
    Int rightOffset = width - 1;
    Int depStrideBlock = depStride * nTxtPerDepthY;
    Pel *refDepthTop = refDepth;
    Pel *refDepthBot = refDepthTop + (height-1)*refDepStride;

    Pel maxDepth = refDepthTop[0] > refDepthBot[0] ? refDepthTop[0] : refDepthBot[0];
    if( maxDepth < refDepthTop[rightOffset] ) { maxDepth = refDepthTop[rightOffset]; }
    if( maxDepth < refDepthBot[rightOffset] ) { maxDepth = refDepthBot[rightOffset]; }

    for( Int sY=0; sY<height; sY+=nTxtPerDepthY )
    {
      for( Int sX=0; sX<width; sX+=nTxtPerDepthX )
      {
        depth[sX] = maxDepth;
      }
      depth += depStrideBlock;
    }
  }
  else
  { // split to 4x8, or 8x4
    Int blocksize    = 8;
    Int subblocksize = 4;
    Int depStrideBlock = depStride * blocksize;
    Pel *depthTmp = NULL;
    Int depStrideTmp = depStride * nTxtPerDepthY;
    Int offset[4] = { 0, subblocksize-1, subblocksize, blocksize-1 };
    Pel *refDepthTmp[4] = { NULL, NULL, NULL, NULL };
    Pel repDepth4x8[2] = {0, 0};
    Pel repDepth8x4[2] = {0, 0};

    Int refDepStrideBlock    = refDepStride * blocksize;
    Int refDepStrideSubBlock = refDepStride * subblocksize;

    refDepthTmp[0] = refDepth;
    refDepthTmp[2] = refDepthTmp[0] + refDepStrideSubBlock;
    refDepthTmp[1] = refDepthTmp[2] - refDepStride;
    refDepthTmp[3] = refDepthTmp[1] + refDepStrideSubBlock;

    for( Int y=0; y<height; y+=blocksize )
    {
      for( Int x=0; x<width; x+=blocksize )
      {
        Bool ULvsBR = false, URvsBL = false;

        ULvsBR = refDepthTmp[0][x+offset[0]] < refDepthTmp[3][x+offset[3]];
        URvsBL = refDepthTmp[0][x+offset[3]] < refDepthTmp[3][x+offset[0]];

        if( ULvsBR ^ URvsBL )
        { // 4x8
          repDepth4x8[0] = refDepthTmp[0][x+offset[0]] > refDepthTmp[0][x+offset[1]] ? refDepthTmp[0][x+offset[0]] : refDepthTmp[0][x+offset[1]];
          if( repDepth4x8[0] < refDepthTmp[3][x+offset[0]] )
          {
            repDepth4x8[0] = refDepthTmp[3][x+offset[0]];
          }
          if( repDepth4x8[0] < refDepthTmp[3][x+offset[1]] )
          {
            repDepth4x8[0] = refDepthTmp[3][x+offset[1]];
          }
          repDepth4x8[1] = refDepthTmp[0][x+offset[2]] > refDepthTmp[0][x+offset[3]] ? refDepthTmp[0][x+offset[2]] : refDepthTmp[0][x+offset[3]];
          if( repDepth4x8[1] < refDepthTmp[3][x+offset[2]] )
          {
            repDepth4x8[1] = refDepthTmp[3][x+offset[2]];
          }
          if( repDepth4x8[1] < refDepthTmp[3][x+offset[3]] )
          {
            repDepth4x8[1] = refDepthTmp[3][x+offset[3]];
          }

          depthTmp = &depth[x];
          for( Int sY=0; sY<blocksize; sY+=nTxtPerDepthY )
          {
            for( Int sX=0; sX<subblocksize; sX+=nTxtPerDepthX )
            {
              depthTmp[sX] = repDepth4x8[0];
            }
            depthTmp += depStrideTmp;
          }
          depthTmp = &depth[x+subblocksize];
          for( Int sY=0; sY<blocksize; sY+=nTxtPerDepthY )
          {
            for( Int sX=0; sX<subblocksize; sX+=nTxtPerDepthX )
            {
              depthTmp[sX] = repDepth4x8[1];
            }
            depthTmp += depStrideTmp;
          }
        }
        else
        { // 8x4
          repDepth8x4[0] = refDepthTmp[0][x+offset[0]] > refDepthTmp[0][x+offset[3]] ? refDepthTmp[0][x+offset[0]] : refDepthTmp[0][x+offset[3]];
          if( repDepth8x4[0] < refDepthTmp[1][x+offset[0]] )
          {
            repDepth8x4[0] = refDepthTmp[1][x+offset[0]];
          }
          if( repDepth8x4[0] < refDepthTmp[1][x+offset[3]] )
          {
            repDepth8x4[0] = refDepthTmp[1][x+offset[3]];
          }
          repDepth8x4[1] = refDepthTmp[2][x+offset[0]] > refDepthTmp[2][x+offset[3]] ? refDepthTmp[2][x+offset[0]] : refDepthTmp[2][x+offset[3]];
          if( repDepth8x4[1] < refDepthTmp[3][x+offset[0]] )
          {
            repDepth8x4[1] = refDepthTmp[3][x+offset[0]];
          }
          if( repDepth8x4[1] < refDepthTmp[3][x+offset[3]] )
          {
            repDepth8x4[1] = refDepthTmp[3][x+offset[3]];
          }
          
          depthTmp = &depth[x];
          for( Int sY=0; sY<subblocksize; sY+=nTxtPerDepthY )
          {
            for( Int sX=0; sX<blocksize; sX+=nTxtPerDepthX )
            {
              depthTmp[sX] = repDepth8x4[0];
            }
            depthTmp += depStrideTmp;
          }
          for( Int sY=0; sY<subblocksize; sY+=nTxtPerDepthY )
          {
            for( Int sX=0; sX<blocksize; sX+=nTxtPerDepthX )
            {
              depthTmp[sX] = repDepth8x4[1];
            }
            depthTmp += depStrideTmp;
          }
        }
      }
      refDepthTmp[0] += refDepStrideBlock;
      refDepthTmp[1] += refDepStrideBlock;
      refDepthTmp[2] += refDepStrideBlock;
      refDepthTmp[3] += refDepStrideBlock;
      depth       += depStrideBlock;
    }
  }


}

Void TComPrediction::xPredInterLumaBlkFromDM( TComDataCU *cu, TComPicYuv *picRef, TComYuv *yuvDepth, Int* shiftLUT, TComMv *mv, UInt partAddr, Int width, Int height, Bool isDepth, TComYuv *&yuvDst, Bool isBi )
{
  Int nTxtPerDepthX = H_3D_VSP_BLOCKSIZE;
  Int nTxtPerDepthY = H_3D_VSP_BLOCKSIZE;
  
  Int refStride = picRef->getStride();
  Int dstStride = yuvDst->getStride();
  Int depStride = yuvDepth->getStride();
  Int refStrideBlock = refStride  * nTxtPerDepthY;
  Int dstStrideBlock = dstStride * nTxtPerDepthY;
  Int depStrideBlock = depStride * nTxtPerDepthY;

  Pel *ref    = picRef->getLumaAddr( cu->getAddr(), cu->getZorderIdxInCU() + partAddr );
  Pel *dst    = yuvDst->getLumaAddr(partAddr);
  Pel *depth  = yuvDepth->getLumaAddr();

#if H_3D_VSP_BLOCKSIZE == 1
#if H_3D_VSP_CONSTRAINED
  //get LUT based horizontal reference range
  Int range = xGetConstrainedSize(width, height);

  // The minimum depth value
  Int minRelativePos = MAX_INT;
  Int maxRelativePos = MIN_INT;

  Pel* depthTemp, *depthInitial=depth;
  for (Int yTxt = 0; yTxt < height; yTxt++)
  {
    for (Int xTxt = 0; xTxt < width; xTxt++)
    {
      if (depthPosX+xTxt < widthDepth)
      {
        depthTemp = depthInitial + xTxt;
      }
      else
      {
        depthTemp = depthInitial + (widthDepth - depthPosX - 1);
      }

      Int disparity = shiftLUT[ *depthTemp ]; // << iShiftPrec;
      Int disparityInt = disparity >> 2;

      if( disparity <= 0)
      {
        if (minRelativePos > disparityInt+xTxt)
        {
          minRelativePos = disparityInt+xTxt;
        }
      }
      else
      {
        if (maxRelativePos < disparityInt+xTxt)
        {
          maxRelativePos = disparityInt+xTxt;
        }
      }
    }
    if (depthPosY+yTxt < heightDepth)
    {
      depthInitial = depthInitial + depStride;
    }
  }

  Int disparity_tmp = shiftLUT[ *depth ]; // << iShiftPrec;
  if (disparity_tmp <= 0)
  {
    maxRelativePos = minRelativePos + range -1 ;
  }
  else
  {
    minRelativePos = maxRelativePos - range +1 ;
  }
#endif
#endif // H_3D_VSP_BLOCKSIZE == 1

  TComMv dv(0, 0);

  for ( Int yTxt = 0; yTxt < height; yTxt += nTxtPerDepthY )
  {
    for ( Int xTxt = 0; xTxt < width; xTxt += nTxtPerDepthX )
    {
      Pel repDepth = depth[ xTxt ];
      assert( repDepth >= 0 && repDepth <= 255 );

      Int disparity = shiftLUT[ repDepth ]; // remove << iShiftPrec ??
      Int xFrac = disparity & 0x3;

      dv.setHor( disparity );
      cu->clipMv( dv );

      Int refOffset = xTxt + (dv.getHor() >> 2);
      
#if H_3D_VSP_CONSTRAINED
      if(refOffset<minRelativePos || refOffset>maxRelativePos)
      {
        xFrac = 0;
      }
      refOffset = Clip3(minRelativePos, maxRelativePos, refOffset);
#endif

      assert( ref[refOffset] >= 0 && ref[refOffset]<= 255 );
      m_if.filterHorLuma( &ref[refOffset], refStride, &dst[xTxt], dstStride, nTxtPerDepthX, nTxtPerDepthY, xFrac, !isBi );
    }
    ref   += refStrideBlock;
    dst   += dstStrideBlock;
    depth += depStrideBlock;
  }

}

Void TComPrediction::xPredInterChromaBlkFromDM  ( TComDataCU *cu, TComPicYuv *picRef, TComYuv *yuvDepth, Int* shiftLUT, TComMv *mv, UInt partAddr, Int width, Int height, Bool isDepth, TComYuv *&yuvDst, Bool isBi )
{
#if (H_3D_VSP_BLOCKSIZE==1)
  Int nTxtPerDepthX = 1;
  Int nTxtPerDepthY = 1;
#else
  Int nTxtPerDepthX = H_3D_VSP_BLOCKSIZE >> 1;
  Int nTxtPerDepthY = H_3D_VSP_BLOCKSIZE >> 1;
#endif

  Int refStride = picRef->getCStride();
  Int dstStride = yuvDst->getCStride();
  Int depStride = yuvDepth->getStride();
  Int refStrideBlock = refStride * nTxtPerDepthY;
  Int dstStrideBlock = dstStride * nTxtPerDepthY;
  Int depStrideBlock = depStride * (nTxtPerDepthY<<1);

  Pel *refCb  = picRef->getCbAddr( cu->getAddr(), cu->getZorderIdxInCU() + partAddr );
  Pel *refCr  = picRef->getCrAddr( cu->getAddr(), cu->getZorderIdxInCU() + partAddr );
  Pel *dstCb  = yuvDst->getCbAddr(partAddr);
  Pel *dstCr  = yuvDst->getCrAddr(partAddr);
  Pel *depth  = yuvDepth->getLumaAddr();

#if H_3D_VSP_BLOCKSIZE == 1
#if H_3D_VSP_CONSTRAINED
  //get LUT based horizontal reference range
  Int range = xGetConstrainedSize(width, height, false);

  // The minimum depth value
  Int minRelativePos = MAX_INT;
  Int maxRelativePos = MIN_INT;

  Int depthTmp;
  for (Int yTxt=0; yTxt<height; yTxt++)
  {
    for (Int xTxt=0; xTxt<width; xTxt++)
    {
      depthTmp = m_pDepthBlock[xTxt+yTxt*width];
      Int disparity = shiftLUT[ depthTmp ]; // << iShiftPrec;
      Int disparityInt = disparity >> 3;//in chroma resolution

      if (disparityInt < 0)
      {
        if (minRelativePos > disparityInt+xTxt)
        {
          minRelativePos = disparityInt+xTxt;
        }
      }
      else
      {
        if (maxRelativePos < disparityInt+xTxt)
        {
          maxRelativePos = disparityInt+xTxt;
        }
      }
    }
  }

  depthTmp = m_pDepthBlock[0];
  Int disparity_tmp = shiftLUT[ depthTmp ]; // << iShiftPrec;
  if ( disparity_tmp < 0 )
  {
    maxRelativePos = minRelativePos + range - 1;
  }
  else
  {
    minRelativePos = maxRelativePos - range + 1;
  }

#endif // H_3D_VSP_CONSTRAINED
#endif // H_3D_VSP_BLOCKSIZE == 1

  TComMv dv(0, 0);
  // luma size -> chroma size
  height >>= 1;
  width  >>= 1;

  for ( Int yTxt = 0; yTxt < height; yTxt += nTxtPerDepthY )
  {
    for ( Int xTxt = 0; xTxt < width; xTxt += nTxtPerDepthX )
    {
      Pel repDepth = depth[ xTxt<<1 ];
      assert( repDepth >= 0 && repDepth <= 255 );

      Int disparity = shiftLUT[ repDepth ]; // remove << iShiftPrec;
      Int xFrac = disparity & 0x7;
      
      dv.setHor( disparity );
      cu->clipMv( dv );

      Int refOffset = xTxt + (dv.getHor() >> 3);

#if H_3D_VSP_CONSTRAINED
      if(refOffset<minRelativePos || refOffset>maxRelativePos)
      {
        xFrac = 0;
      }
      refOffset = Clip3(minRelativePos, maxRelativePos, refOffset);
#endif

      assert( refCb[refOffset] >= 0 && refCb[refOffset]<= 255 );
      assert( refCr[refOffset] >= 0 && refCr[refOffset]<= 255 );

      m_if.filterHorChroma( &refCb[refOffset], refStride, &dstCb[xTxt], dstStride, nTxtPerDepthX, nTxtPerDepthY, xFrac, !isBi );
      m_if.filterHorChroma( &refCr[refOffset], refStride, &dstCr[xTxt], dstStride, nTxtPerDepthX, nTxtPerDepthY, xFrac, !isBi );
    }
    refCb += refStrideBlock;
    refCr += refStrideBlock;
    dstCb += dstStrideBlock;
    dstCr += dstStrideBlock;
    depth += depStrideBlock;
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
UInt TComPrediction::xPredWedgeFromTex( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiWidth, UInt uiHeight, UInt intraTabIdx )
{
  TComPic*      pcPicTex = pcCU->getSlice()->getTexturePic();
  assert( pcPicTex != NULL );
  TComDataCU*   pcColTexCU = pcPicTex->getCU(pcCU->getAddr());
  UInt          uiTexPartIdx = pcCU->getZorderIdxInCU() + uiAbsPartIdx;
  Int           uiColTexIntraDir = pcColTexCU->isIntra( uiTexPartIdx ) ? pcColTexCU->getLumaIntraDir( uiTexPartIdx ) : 255;

  assert( uiColTexIntraDir > DC_IDX && uiColTexIntraDir < 35 );
  return g_aauiWdgLstM3[g_aucConvertToBit[uiWidth]][uiColTexIntraDir-2].at(intraTabIdx);
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

  Int cuMaxLog2Size = g_aucConvertToBit[g_uiMaxCUWidth]+2;   // 
  iDC = iDC >> (cuMaxLog2Size - pcCU->getDepth(0))*2;        //  iDC /= (uiWidth*uiHeight);

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
Void TComPrediction::analyzeSegmentsSDC( Pel* pOrig, UInt uiStride, UInt uiSize, Pel* rpSegMeans, UInt uiNumSegments, Bool* pMask, UInt uiMaskStride
                                         ,UInt uiIntraMode
                                         ,Bool orgDC
                                        )
{
  Int iSumDepth[2];
  memset(iSumDepth, 0, sizeof(Int)*2);
  Int iSumPix[2];
  memset(iSumPix, 0, sizeof(Int)*2);
  
  if (orgDC == false)
  {
    if ( getDimType(uiIntraMode) == DMM1_IDX )
    {
      UChar ucSegmentLT = pMask[0];
      UChar ucSegmentRT = pMask[uiSize-1];
      UChar ucSegmentLB = pMask[uiMaskStride * (uiSize-1)]; 
      UChar ucSegmentRB = pMask[uiMaskStride * (uiSize-1) + (uiSize-1)]; 

      rpSegMeans[ucSegmentLT] = pOrig[0];
      rpSegMeans[ucSegmentRT] = pOrig[uiSize-1];
      rpSegMeans[ucSegmentLB] = pOrig[uiStride * (uiSize-1) ];
      rpSegMeans[ucSegmentRB] = pOrig[uiStride * (uiSize-1) + (uiSize-1) ];
    }
    else if (uiIntraMode == PLANAR_IDX)
    {
      Pel* pLeftTop = pOrig;
      Pel* pRightTop = pOrig + (uiSize-1);
      Pel* pLeftBottom = (pOrig+ (uiStride*(uiSize-1)));
      Pel* pRightBottom = (pOrig+ (uiStride*(uiSize-1)) + (uiSize-1));

      rpSegMeans[0] = (*pLeftTop + *pRightTop + *pLeftBottom + *pRightBottom + 2)>>2;
    }
    return;
  }

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
