/* The copyright in this software is being made available under the BSD
 * License, included below. This software may be subject to other third party
 * and contributor rights, including patent rights, and no such rights are
 * granted under this license.  
 *
* Copyright (c) 2010-2014, ITU/ISO/IEC
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
Void TComPrediction::predIntraLumaDepth( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiIntraMode, Pel* piPred, UInt uiStride, Int iWidth, Int iHeight, Bool bFastEnc, TComWedgelet* dmm4Segmentation  )
{
  assert( iWidth == iHeight  );
  assert( iWidth >= DIM_MIN_SIZE && iWidth <= DIM_MAX_SIZE );
  assert( isDimMode( uiIntraMode ) );

  UInt dimType    = getDimType  ( uiIntraMode );
  Bool isDmmMode  = (dimType <  DMM_NUM_TYPE);

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
        dmmSegmentation = pcCU->isDMM1UpscaleMode((UInt)iWidth) ? 
            &(g_dmmWedgeLists[ g_aucConvertToBit[pcCU->getDMM1BasePatternWidth((UInt)iWidth)] ][ pcCU->getDmmWedgeTabIdx( dimType, uiAbsPartIdx ) ]) : 
            &(g_dmmWedgeLists[ g_aucConvertToBit[iWidth] ][ pcCU->getDmmWedgeTabIdx( dimType, uiAbsPartIdx ) ]);
      } break;
    case( DMM4_IDX ): 
      {
        if( dmm4Segmentation == NULL )
        { 
          dmmSegmentation = new TComWedgelet( iWidth, iHeight );
          xPredContourFromTex( pcCU, uiAbsPartIdx, iWidth, iHeight, dmmSegmentation );
        }
        else
        {
          xPredContourFromTex( pcCU, uiAbsPartIdx, iWidth, iHeight, dmm4Segmentation );
          dmmSegmentation = dmm4Segmentation;
        }
      } break;
    default: assert(0);
    }
    assert( dmmSegmentation );
    if( dimType == DMM1_IDX && pcCU->isDMM1UpscaleMode((UInt)iWidth) ) 
    {
        biSegPattern = dmmSegmentation->getScaledPattern((UInt)iWidth);
        patternStride = iWidth;
    } 
    else 
    { 
        biSegPattern  = dmmSegmentation->getPattern();
        patternStride = dmmSegmentation->getStride ();
    }
  }
#endif

  // get predicted partition values
  assert( biSegPattern );
  Int* piMask = NULL;
  piMask = pcCU->getPattern()->getAdiOrgBuf( iWidth, iHeight, m_piYuvExt ); // no filtering
  assert( piMask );
  Int maskStride = 2*iWidth + 1;  
  Int* ptrSrc = piMask+maskStride+1;
  Pel predDC1 = 0; Pel predDC2 = 0;
  xPredBiSegDCs( ptrSrc, maskStride, biSegPattern, patternStride, predDC1, predDC2 );

  // set segment values with deltaDC offsets
  Pel segDC1 = 0;
  Pel segDC2 = 0;
  if( !pcCU->getSDCFlag( uiAbsPartIdx ) )
  {
    Pel deltaDC1 = pcCU->getDimDeltaDC( dimType, 0, uiAbsPartIdx );
    Pel deltaDC2 = pcCU->getDimDeltaDC( dimType, 1, uiAbsPartIdx );
#if H_3D_DIM_DMM
    if( isDmmMode )
    {
#if H_3D_DIM_DLT
      segDC1 = pcCU->getSlice()->getPPS()->getDLT()->idx2DepthValue( pcCU->getSlice()->getLayerIdInVps(), pcCU->getSlice()->getPPS()->getDLT()->depthValue2idx( pcCU->getSlice()->getLayerIdInVps(), predDC1 ) + deltaDC1 );
      segDC2 = pcCU->getSlice()->getPPS()->getDLT()->idx2DepthValue( pcCU->getSlice()->getLayerIdInVps(), pcCU->getSlice()->getPPS()->getDLT()->depthValue2idx( pcCU->getSlice()->getLayerIdInVps(), predDC2 ) + deltaDC2 );
#else
      segDC1 = ClipY( predDC1 + deltaDC1 );
      segDC2 = ClipY( predDC2 + deltaDC2 );
#endif
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
  pcCU->setDmmPredictor(segDC1, 0);
  pcCU->setDmmPredictor(segDC2, 1);

#if H_3D_DIM_DMM
  if( dimType == DMM4_IDX && dmm4Segmentation == NULL ) { dmmSegmentation->destroy(); delete dmmSegmentation; }
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
#if H_3D_ARP
      if(!pcCU->getARPW(PartAddr) && RefPOCL0 == RefPOCL1 && pcCU->getCUMvField(REF_PIC_LIST_0)->getMv(PartAddr) == pcCU->getCUMvField(REF_PIC_LIST_1)->getMv(PartAddr))
#else
      if(RefPOCL0 == RefPOCL1 && pcCU->getCUMvField(REF_PIC_LIST_0)->getMv(PartAddr) == pcCU->getCUMvField(REF_PIC_LIST_1)->getMv(PartAddr))
#endif
      {
        return true;
      }
    }
  }
  return false;
}

#if H_3D_SPIVMP
Void TComPrediction::xGetSubPUAddrAndMerge(TComDataCU* pcCU, UInt uiPartAddr, Int iSPWidth, Int iSPHeight, Int iNumSPInOneLine, Int iNumSP, UInt* uiMergedSPW, UInt* uiMergedSPH, UInt* uiSPAddr )
{
  for (Int i = 0; i < iNumSP; i++)
  {
    uiMergedSPW[i] = iSPWidth;
    uiMergedSPH[i] = iSPHeight;
    pcCU->getSPAbsPartIdx(uiPartAddr, iSPWidth, iSPHeight, i, iNumSPInOneLine, uiSPAddr[i]);
  }
  if( pcCU->getARPW( uiPartAddr ) != 0 )
  {
    return;
  }

  // horizontal sub-PU merge
  for (Int i=0; i<iNumSP; i++)
  {
    if (i % iNumSPInOneLine == iNumSPInOneLine - 1 || uiMergedSPW[i]==0 || uiMergedSPH[i]==0)
    {
      continue;
    }
    for (Int j=i+1; j<i+iNumSPInOneLine-i%iNumSPInOneLine; j++)
    {
      if (xCheckTwoSPMotion(pcCU, uiSPAddr[i], uiSPAddr[j]))
      {
        uiMergedSPW[i] += iSPWidth;
        uiMergedSPW[j] = uiMergedSPH[j] = 0;
      }
      else
      {
        break;
      }
    }
  }
  //vertical sub-PU merge
  for (Int i=0; i<iNumSP-iNumSPInOneLine; i++)
  {
    if (uiMergedSPW[i]==0 || uiMergedSPH[i]==0)
    {
      continue;
    }
    for (Int j=i+iNumSPInOneLine; j<iNumSP; j+=iNumSPInOneLine)
    {
      if (xCheckTwoSPMotion(pcCU, uiSPAddr[i], uiSPAddr[j]) && uiMergedSPW[i]==uiMergedSPW[j])
      {
        uiMergedSPH[i] += iSPHeight;
        uiMergedSPH[j] = uiMergedSPW[j] = 0;
      }
      else
      {
        break;
      }
    }
  }
}

Bool TComPrediction::xCheckTwoSPMotion ( TComDataCU* pcCU, UInt PartAddr0, UInt PartAddr1 )
{
  if( pcCU->getCUMvField(REF_PIC_LIST_0)->getRefIdx(PartAddr0) != pcCU->getCUMvField(REF_PIC_LIST_0)->getRefIdx(PartAddr1))
  {
    return false;
  }
  if( pcCU->getCUMvField(REF_PIC_LIST_1)->getRefIdx(PartAddr0) != pcCU->getCUMvField(REF_PIC_LIST_1)->getRefIdx(PartAddr1))
  {
    return false;
  }

  if (pcCU->getCUMvField(REF_PIC_LIST_0)->getRefIdx(PartAddr0) >= 0)
  {
    if (pcCU->getCUMvField(REF_PIC_LIST_0)->getMv(PartAddr0) != pcCU->getCUMvField(REF_PIC_LIST_0)->getMv(PartAddr1))
    {
      return false;
    }
  }

  if (pcCU->getCUMvField(REF_PIC_LIST_1)->getRefIdx(PartAddr0) >= 0)
  {
    if (pcCU->getCUMvField(REF_PIC_LIST_1)->getMv(PartAddr0) != pcCU->getCUMvField(REF_PIC_LIST_1)->getMv(PartAddr1))
    {
      return false;
    }
  }
  return true;
}
#endif

#if H_3D_DBBP
PartSize TComPrediction::getPartitionSizeFromDepth(Pel* pDepthPels, UInt uiDepthStride, UInt uiSize)
{
  // find virtual partitioning for this CU based on depth block
  // segmentation of texture block --> mask IDs
  Pel*  pDepthBlockStart      = pDepthPels;
  
  // first compute average of depth block for thresholding
  Int iSumDepth = 0;
  Int iSubSample = 4;
  for (Int y=0; y<uiSize; y+=iSubSample)
  {
    for (Int x=0; x<uiSize; x+=iSubSample)
    {
      Int depthPel = pDepthPels[x];
      
      iSumDepth += depthPel;
    }
    
    // next row
    pDepthPels += uiDepthStride*iSubSample;
  }
  
  Int iSizeInBits = g_aucConvertToBit[uiSize] - g_aucConvertToBit[iSubSample];  // respect sub-sampling factor
  Int iMean = iSumDepth >> iSizeInBits*2;       // iMean /= (uiSize*uiSize);
  
  // start again for segmentation
  pDepthPels = pDepthBlockStart;
  
  // start mapping process
  Int matchedPartSum[2][2] = {{0,0},{0,0}}; // counter for each part size and boolean option
  PartSize virtualPartSizes[2] = { SIZE_Nx2N, SIZE_2NxN};
  
  UInt uiHalfSize = uiSize>>1;
  for (Int y=0; y<uiSize; y+=iSubSample)
  {
    for (Int x=0; x<uiSize; x+=iSubSample)
    {
      Int depthPel = pDepthPels[x];
      
      // decide which segment this pixel belongs to
      Int ucSegment = (Int)(depthPel>iMean);
      
      // Matched Filter to find optimal (conventional) partitioning
      
      // SIZE_Nx2N
      if(x<uiHalfSize)  // left
      {
        matchedPartSum[0][ucSegment]++;
      }
      else  // right
      {
        matchedPartSum[0][1-ucSegment]++;
      }
      
      // SIZE_2NxN
      if(y<uiHalfSize)  // top
      {
        matchedPartSum[1][ucSegment]++;
      }
      else  // bottom
      {
        matchedPartSum[1][1-ucSegment]++;
      }
    }
    
    // next row
    pDepthPels += uiDepthStride*iSubSample;
  }
  
  PartSize matchedPartSize = SIZE_NONE;
  
  Int iMaxMatchSum = 0;
  for(Int p=0; p<2; p++)  // loop over partition
  {
    for( Int b=0; b<=1; b++ ) // loop over boolean options
    {
      if(matchedPartSum[p][b] > iMaxMatchSum)
      {
        iMaxMatchSum = matchedPartSum[p][b];
        matchedPartSize = virtualPartSizes[p];
      }
    }
  }
  
  AOF( matchedPartSize != SIZE_NONE );
  
  return matchedPartSize;
}

Bool TComPrediction::getSegmentMaskFromDepth( Pel* pDepthPels, UInt uiDepthStride, UInt uiWidth, UInt uiHeight, Bool* pMask )
{
  // segmentation of texture block --> mask IDs
  Pel*  pDepthBlockStart      = pDepthPels;
  
  // first compute average of depth block for thresholding
  Int iSumDepth = 0;
  Int uiMinDepth = MAX_INT;
  Int uiMaxDepth = 0;

  iSumDepth  = pDepthPels[ 0 ];
  iSumDepth += pDepthPels[ uiWidth - 1 ];
  iSumDepth += pDepthPels[ uiDepthStride * (uiHeight - 1) ];
  iSumDepth += pDepthPels[ uiDepthStride * (uiHeight - 1) + uiWidth - 1 ];

  uiMinDepth = pDepthPels[ 0 ];
  uiMinDepth = std::min( uiMinDepth, (Int)pDepthPels[ uiWidth - 1 ]);
  uiMinDepth = std::min( uiMinDepth, (Int)pDepthPels[ uiDepthStride * (uiHeight - 1) ]);
  uiMinDepth = std::min( uiMinDepth, (Int)pDepthPels[ uiDepthStride * (uiHeight - 1) + uiWidth - 1 ]);

  uiMaxDepth = pDepthPels[ 0 ];
  uiMaxDepth = std::max( uiMaxDepth, (Int)pDepthPels[ uiWidth - 1 ]);
  uiMaxDepth = std::max( uiMaxDepth, (Int)pDepthPels[ uiDepthStride * (uiHeight - 1) ]);
  uiMaxDepth = std::max( uiMaxDepth, (Int)pDepthPels[ uiDepthStride * (uiHeight - 1) + uiWidth - 1 ]);

  
  // don't generate mask for blocks with small depth range (encoder decision)
  if( uiMaxDepth - uiMinDepth < 10 )
  {
    return false;
  }
  
  AOF(uiWidth==uiHeight);
  Int iMean = iSumDepth >> 2;
  
  // start again for segmentation
  pDepthPels = pDepthBlockStart;
  
  Bool bInvertMask = pDepthPels[0]>iMean; // top-left segment needs to be mapped to partIdx 0
  
  // generate mask
  UInt uiSumPix[2] = {0,0};
  for (Int y=0; y<uiHeight; y++)
  {
    for (Int x=0; x<uiHeight; x++)
    {
      Int depthPel = pDepthPels[x];
      
      // decide which segment this pixel belongs to
      Int ucSegment = (Int)(depthPel>iMean);
      
      if( bInvertMask )
      {
        ucSegment = 1-ucSegment;
      }
      
      // count pixels for each segment
      uiSumPix[ucSegment]++;
      
      // set mask value
      pMask[x] = (Bool)ucSegment;
    }
    
    // next row
    pDepthPels += uiDepthStride;
    pMask += MAX_CU_SIZE;
  }
  
  // don't generate valid mask for tiny segments (encoder decision)
  // each segment needs to cover at least 1/8th of block
  UInt uiMinPixPerSegment = (uiWidth*uiHeight) >> 3;
  if( !( uiSumPix[0] > uiMinPixPerSegment && uiSumPix[1] > uiMinPixPerSegment ) )
  {
    return false;
  }
  
  // all good
  return true;
}

Void TComPrediction::combineSegmentsWithMask( TComYuv* pInYuv[2], TComYuv* pOutYuv, Bool* pMask, UInt uiWidth, UInt uiHeight, UInt uiPartAddr, UInt partSize )
{
  Pel*  piSrc[2]    = {pInYuv[0]->getLumaAddr(uiPartAddr), pInYuv[1]->getLumaAddr(uiPartAddr)};
  UInt  uiSrcStride = pInYuv[0]->getStride();
  Pel*  piDst       = pOutYuv->getLumaAddr(uiPartAddr);
  UInt  uiDstStride = pOutYuv->getStride();
  
  UInt  uiMaskStride= MAX_CU_SIZE;
  Pel* tmpTar = 0;
  tmpTar = (Pel *)xMalloc(Pel, uiWidth*uiHeight);
  
  // backup pointer
  Bool* pMaskStart = pMask;
  
  // combine luma first
  for (Int y=0; y<uiHeight; y++)
  {
    for (Int x=0; x<uiWidth; x++)
    {
      UChar ucSegment = (UChar)pMask[x];
      AOF( ucSegment < 2 );
      
      // filtering
      tmpTar[y*uiWidth+x] = piSrc[ucSegment][x];
    }
    
    piSrc[0]  += uiSrcStride;
    piSrc[1]  += uiSrcStride;
    pMask     += uiMaskStride;
  }
  
  if (partSize == SIZE_Nx2N)
  {
    for (Int y=0; y<uiHeight; y++)
    {
      for (Int x=0; x<uiWidth; x++)
      {
        Bool l = (x==0)?pMaskStart[y*uiMaskStride+x]:pMaskStart[y*uiMaskStride+x-1];
        Bool r = (x==uiWidth-1)?pMaskStart[y*uiMaskStride+x]:pMaskStart[y*uiMaskStride+x+1];
        
        Pel left, right;
        left   = (x==0)          ? tmpTar[y*uiWidth+x] : tmpTar[y*uiWidth+x-1];
        right  = (x==uiWidth-1)  ? tmpTar[y*uiWidth+x] : tmpTar[y*uiWidth+x+1];
        
        piDst[x] = (l!=r) ? ClipY( Pel(( left + (tmpTar[y*uiWidth+x] << 1) + right ) >> 2 )) : tmpTar[y*uiWidth+x]; 
      }
      piDst     += uiDstStride;
    }
  }
  else // SIZE_2NxN
  {
    for (Int y=0; y<uiHeight; y++)
    {
      for (Int x=0; x<uiWidth; x++)
      {
        Bool t = (y==0)?pMaskStart[y*uiMaskStride+x]:pMaskStart[(y-1)*uiMaskStride+x];
        Bool b = (y==uiHeight-1)?pMaskStart[y*uiMaskStride+x]:pMaskStart[(y+1)*uiMaskStride+x];
        
        Pel top, bottom;
        top    = (y==0)          ? tmpTar[y*uiWidth+x] : tmpTar[(y-1)*uiWidth+x];
        bottom = (y==uiHeight-1) ? tmpTar[y*uiWidth+x] : tmpTar[(y+1)*uiWidth+x];
        
        piDst[x] = (t!=b) ? ClipY( Pel(( top + (tmpTar[y*uiWidth+x] << 1) + bottom ) >> 2 )) : tmpTar[y*uiWidth+x];
      }
      piDst     += uiDstStride;
    }
  }

  if ( tmpTar    ) { xFree(tmpTar);             tmpTar        = NULL; }
  
  // now combine chroma
  Pel*  piSrcU[2]       = { pInYuv[0]->getCbAddr(uiPartAddr), pInYuv[1]->getCbAddr(uiPartAddr) };
  Pel*  piSrcV[2]       = { pInYuv[0]->getCrAddr(uiPartAddr), pInYuv[1]->getCrAddr(uiPartAddr) };
  UInt  uiSrcStrideC    = pInYuv[0]->getCStride();
  Pel*  piDstU          = pOutYuv->getCbAddr(uiPartAddr);
  Pel*  piDstV          = pOutYuv->getCrAddr(uiPartAddr);
  UInt  uiDstStrideC    = pOutYuv->getCStride();
  UInt  uiWidthC        = uiWidth >> 1;
  UInt  uiHeightC       = uiHeight >> 1;
  Pel  filSrcU = 0, filSrcV = 0;
  Pel* tmpTarU = 0, *tmpTarV = 0;
  tmpTarU = (Pel *)xMalloc(Pel, uiWidthC*uiHeightC);
  tmpTarV = (Pel *)xMalloc(Pel, uiWidthC*uiHeightC);
  pMask = pMaskStart;
  
  for (Int y=0; y<uiHeightC; y++)
  {
    for (Int x=0; x<uiWidthC; x++)
    {
      UChar ucSegment = (UChar)pMask[x*2];
      AOF( ucSegment < 2 );
      
      // filtering
      tmpTarU[y*uiWidthC+x] = piSrcU[ucSegment][x];
      tmpTarV[y*uiWidthC+x] = piSrcV[ucSegment][x];
    }
    
    piSrcU[0]   += uiSrcStrideC;
    piSrcU[1]   += uiSrcStrideC;
    piSrcV[0]   += uiSrcStrideC;
    piSrcV[1]   += uiSrcStrideC;
    pMask       += 2*uiMaskStride;
  }

  if (partSize == SIZE_Nx2N)
  {
    for (Int y=0; y<uiHeightC; y++)
    {
      for (Int x=0; x<uiWidthC; x++)
      {
        Bool l = (x==0)?pMaskStart[y*2*uiMaskStride+x*2]:pMaskStart[y*2*uiMaskStride+(x-1)*2];
        Bool r = (x==uiWidthC-1)?pMaskStart[y*2*uiMaskStride+x*2]:pMaskStart[y*2*uiMaskStride+(x+1)*2];

        Pel leftU, rightU;
        leftU   = (x==0)           ? tmpTarU[y*uiWidthC+x] : tmpTarU[y*uiWidthC+x-1];
        rightU  = (x==uiWidthC-1)  ? tmpTarU[y*uiWidthC+x] : tmpTarU[y*uiWidthC+x+1];
        Pel leftV, rightV;
        leftV   = (x==0)           ? tmpTarV[y*uiWidthC+x] : tmpTarV[y*uiWidthC+x-1];
        rightV  = (x==uiWidthC-1)  ? tmpTarV[y*uiWidthC+x] : tmpTarV[y*uiWidthC+x+1];

        if (l!=r)
        {
          filSrcU = ClipC( Pel(( leftU + (tmpTarU[y*uiWidthC+x] << 1) + rightU ) >> 2 ));
          filSrcV = ClipC( Pel(( leftV + (tmpTarV[y*uiWidthC+x] << 1) + rightV ) >> 2 ));
        }
        else
        {
          filSrcU = tmpTarU[y*uiWidthC+x];
          filSrcV = tmpTarV[y*uiWidthC+x];
        }
        piDstU[x] = filSrcU;
        piDstV[x] = filSrcV;
      }
      piDstU      += uiDstStrideC;
      piDstV      += uiDstStrideC;
    }
  }
  else
  {
    for (Int y=0; y<uiHeightC; y++)
    {
      for (Int x=0; x<uiWidthC; x++)
      {
        Bool t = (y==0)?pMaskStart[y*2*uiMaskStride+x*2]:pMaskStart[(y-1)*2*uiMaskStride+x*2];
        Bool b = (y==uiHeightC-1)?pMaskStart[y*2*uiMaskStride+x*2]:pMaskStart[(y+1)*2*uiMaskStride+x*2];

        Pel topU, bottomU;
        topU    = (y==0)           ? tmpTarU[y*uiWidthC+x] : tmpTarU[(y-1)*uiWidthC+x];
        bottomU = (y==uiHeightC-1) ? tmpTarU[y*uiWidthC+x] : tmpTarU[(y+1)*uiWidthC+x];
        Pel topV, bottomV;
        topV    = (y==0)           ? tmpTarV[y*uiWidthC+x] : tmpTarV[(y-1)*uiWidthC+x];
        bottomV = (y==uiHeightC-1) ? tmpTarV[y*uiWidthC+x] : tmpTarV[(y+1)*uiWidthC+x];

        if (t!=b)
        {
          filSrcU = ClipC( Pel(( topU + (tmpTarU[y*uiWidthC+x] << 1) + bottomU ) >> 2 ));
          filSrcV = ClipC( Pel(( topV + (tmpTarV[y*uiWidthC+x] << 1) + bottomV ) >> 2 ));
        }
        else
        {
          filSrcU = tmpTarU[y*uiWidthC+x];
          filSrcV = tmpTarV[y*uiWidthC+x];
        }
        piDstU[x] = filSrcU;
        piDstV[x] = filSrcV;
      }
      piDstU      += uiDstStrideC;
      piDstV      += uiDstStrideC;
    }
  }

  if ( tmpTarU    ) { xFree(tmpTarU);             tmpTarU        = NULL; }
  if ( tmpTarV    ) { xFree(tmpTarV);             tmpTarV        = NULL; }
}
#endif

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
#if H_3D_SPIVMP
        if ( pcCU->getSPIVMPFlag(uiPartAddr)!=0)  
        {
          Int iNumSPInOneLine, iNumSP, iSPWidth, iSPHeight;

          pcCU->getSPPara(iWidth, iHeight, iNumSP, iNumSPInOneLine, iSPWidth, iSPHeight);

          UInt uiW[256], uiH[256];
          UInt uiSPAddr[256];

          xGetSubPUAddrAndMerge(pcCU, uiPartAddr, iSPWidth, iSPHeight, iNumSPInOneLine, iNumSP, uiW, uiH, uiSPAddr);

          //MC
          for (Int i = 0; i < iNumSP; i++)
          {
            if (uiW[i]==0 || uiH[i]==0)
            {
              continue;
            }
            if( xCheckIdenticalMotion( pcCU, uiSPAddr[i] ))
            {
              xPredInterUni (pcCU, uiSPAddr[i], uiW[i], uiH[i], REF_PIC_LIST_0, pcYuvPred );
            }
            else
            {
              xPredInterBi  (pcCU, uiSPAddr[i], uiW[i], uiH[i], pcYuvPred);
            }
          }
        }
        else
        {
#endif
          if ( xCheckIdenticalMotion( pcCU, uiPartAddr ) )
          {
            xPredInterUni (pcCU, uiPartAddr, iWidth, iHeight, REF_PIC_LIST_0, pcYuvPred );
          }
          else
          {
            xPredInterBi  (pcCU, uiPartAddr, iWidth, iHeight, pcYuvPred );
          }
#if H_3D_SPIVMP
        }
#endif
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

  for ( iPartIdx = 0; iPartIdx < pcCU->getNumPartitions(); iPartIdx++ )
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
#if H_3D_SPIVMP
       if (pcCU->getSPIVMPFlag(uiPartAddr)!=0)  
      {
        Int iNumSPInOneLine, iNumSP, iSPWidth, iSPHeight;

        pcCU->getSPPara(iWidth, iHeight, iNumSP, iNumSPInOneLine, iSPWidth, iSPHeight);

        UInt uiW[256], uiH[256];
        UInt uiSPAddr[256];

        xGetSubPUAddrAndMerge(pcCU, uiPartAddr, iSPWidth, iSPHeight, iNumSPInOneLine, iNumSP, uiW, uiH, uiSPAddr);
        //MC
        for (Int i = 0; i < iNumSP; i++)
        {
          if (uiW[i]==0 || uiH[i]==0)
          {
            continue;
          }
          if( xCheckIdenticalMotion( pcCU, uiSPAddr[i] ))
          {
            xPredInterUni (pcCU, uiSPAddr[i], uiW[i], uiH[i], REF_PIC_LIST_0, pcYuvPred );
          }
          else
          {
            xPredInterBi  (pcCU, uiSPAddr[i], uiW[i], uiH[i], pcYuvPred);
          }
        }
      }
      else
      {
#endif
        if ( xCheckIdenticalMotion( pcCU, uiPartAddr ) )
        {
          xPredInterUni (pcCU, uiPartAddr, iWidth, iHeight, REF_PIC_LIST_0, pcYuvPred );
        }
        else
        {
          xPredInterBi  (pcCU, uiPartAddr, iWidth, iHeight, pcYuvPred );
        }
#if H_3D_SPIVMP
       }
#endif
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
  if(pcCU->getARPW( uiPartAddr ) > 0  && pcCU->getSlice()->getRefPic( eRefPicList, iRefIdx )->getPOC()== pcCU->getSlice()->getPOC())
  {
    xPredInterUniARPviewRef( pcCU , uiPartAddr , iWidth , iHeight , eRefPicList , rpcYuvPred , bi );
  }
  else
  {
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
  }
#endif
}

#if H_3D_VSP
Void TComPrediction::xPredInterUniVSP( TComDataCU* pcCU, UInt uiPartAddr, Int iWidth, Int iHeight, RefPicList eRefPicList, TComYuv*& rpcYuvPred, Bool bi )
{
  Int vspSize = pcCU->getVSPFlag( uiPartAddr ) >> 1;

  Int widthSubPU, heightSubPU;
  if (vspSize)
  {
    widthSubPU  = 8;
    heightSubPU = 4;
  }
  else
  {
    widthSubPU  = 4;
    heightSubPU = 8;
  }
  xPredInterUniSubPU( pcCU, uiPartAddr, iWidth, iHeight, eRefPicList, rpcYuvPred, bi, widthSubPU, heightSubPU );
}

Void TComPrediction::xPredInterUniSubPU( TComDataCU* pcCU, UInt uiPartAddr, Int iWidth, Int iHeight, RefPicList eRefPicList, TComYuv*& rpcYuvPred, Bool bi, Int widthSubPU, Int heightSubPU )
{
  UInt numPartsInLine       = pcCU->getPic()->getNumPartInWidth();
  UInt horiNumPartsInSubPU  = widthSubPU >> 2;
  UInt vertNumPartsInSubPU  = (heightSubPU >> 2) * numPartsInLine;

  UInt partAddrRasterLine = g_auiZscanToRaster[ uiPartAddr ];

  for( Int posY=0; posY<iHeight; posY+=heightSubPU, partAddrRasterLine+=vertNumPartsInSubPU )
  {
    UInt partAddrRasterSubPU = partAddrRasterLine;
    for( Int posX=0; posX<iWidth; posX+=widthSubPU, partAddrRasterSubPU+=horiNumPartsInSubPU )
    {
      UInt    partAddrSubPU = g_auiRasterToZscan[ partAddrRasterSubPU ];
      Int     refIdx        = pcCU->getCUMvField( eRefPicList )->getRefIdx( partAddrSubPU );           assert (refIdx >= 0);
      TComMv  cMv           = pcCU->getCUMvField( eRefPicList )->getMv( partAddrSubPU );
      pcCU->clipMv(cMv);

      xPredInterLumaBlk  ( pcCU, pcCU->getSlice()->getRefPic( eRefPicList, refIdx )->getPicYuvRec(), partAddrSubPU, &cMv, widthSubPU, heightSubPU, rpcYuvPred, bi );
      xPredInterChromaBlk( pcCU, pcCU->getSlice()->getRefPic( eRefPicList, refIdx )->getPicYuvRec(), partAddrSubPU, &cMv, widthSubPU, heightSubPU, rpcYuvPred, bi );

    }
  }
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
    Int arpRefIdx = pcCU->getSlice()->getFirstTRefIdx(eRefPicList);
    if( dW > 0 && pcCU->getSlice()->getRefPic( eRefPicList, arpRefIdx )->getPOC()!= pcCU->getSlice()->getPOC() )
    {
      bTobeScaled = true;
    }

    pcPicYuvBaseCol =  pcCU->getSlice()->getBaseViewRefPic( pcCU->getSlice()->getPOC(),                              cDistparity.m_aVIdxCan );

    pcPicYuvBaseRef =  pcCU->getSlice()->getBaseViewRefPic( pcCU->getSlice()->getRefPic( eRefPicList, arpRefIdx )->getPOC(), cDistparity.m_aVIdxCan );

    if (!pcCU->getSlice()->getArpRefPicAvailable( eRefPicList, cDistparity.m_aVIdxCan))
    {
      dW = 0;
      bTobeScaled = false;
    }
    else
    {
      assert( pcPicYuvBaseCol->getPOC() == pcCU->getSlice()->getPOC() && pcPicYuvBaseRef->getPOC() == pcCU->getSlice()->getRefPic( eRefPicList, arpRefIdx )->getPOC() );
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
  xPredInterLumaBlk  ( pcCU, pcPicYuvRef, uiPartAddr, &cMv, iWidth, iHeight, rpcYuvPred, bi || ( dW > 0 ), true );
  xPredInterChromaBlk( pcCU, pcPicYuvRef, uiPartAddr, &cMv, iWidth, iHeight, rpcYuvPred, bi || ( dW > 0 ), true );

  if( dW > 0 )
  {
    TComYuv * pYuvB0 = &m_acYuvPredBase[0];
    TComYuv * pYuvB1  = &m_acYuvPredBase[1];

    TComMv cMVwithDisparity = cMv + cDistparity.m_acNBDV;
    pcCU->clipMv(cMVwithDisparity);
    if (iWidth <= 8)
    {
      pYuvB0->clear(); pYuvB1->clear();
    }

    assert ( cDistparity.bDV );
    
    TComMv cNBDV = cDistparity.m_acNBDV;
    pcCU->clipMv( cNBDV );
    
    pcPicYuvRef = pcPicYuvBaseCol->getPicYuvRec();
    xPredInterLumaBlk  ( pcCU, pcPicYuvRef, uiPartAddr, &cNBDV, iWidth, iHeight, pYuvB0, true, true );
    if (iWidth > 8)
      xPredInterChromaBlk( pcCU, pcPicYuvRef, uiPartAddr, &cNBDV, iWidth, iHeight, pYuvB0, true, true );
    
    pcPicYuvRef = pcPicYuvBaseRef->getPicYuvRec();
    xPredInterLumaBlk  ( pcCU, pcPicYuvRef, uiPartAddr, &cMVwithDisparity, iWidth, iHeight, pYuvB1, true, true );
  
    if (iWidth > 8)
      xPredInterChromaBlk( pcCU, pcPicYuvRef, uiPartAddr, &cMVwithDisparity, iWidth, iHeight, pYuvB1, true, true );
    
    pYuvB0->subtractARP( pYuvB0 , pYuvB1 , uiPartAddr , iWidth , iHeight );

    if( 2 == dW )
    {
      pYuvB0->multiplyARP( uiPartAddr , iWidth , iHeight , dW );
    }
    rpcYuvPred->addARP( rpcYuvPred , pYuvB0 , uiPartAddr , iWidth , iHeight , !bi );
  }
}

Bool TComPrediction::xCheckBiInterviewARP( TComDataCU* pcCU, UInt uiPartAddr, Int iWidth, Int iHeight, RefPicList eBaseRefPicList, TComPic*& pcPicYuvCurrTRef, TComMv& cBaseTMV, Int& iCurrTRefPoc )
{
  Int         iRefIdx       = pcCU->getCUMvField( eBaseRefPicList )->getRefIdx( uiPartAddr );
  TComMv      cDMv          = pcCU->getCUMvField( eBaseRefPicList )->getMv( uiPartAddr );
  TComPic* pcPicYuvBaseCol  = pcCU->getSlice()->getRefPic( eBaseRefPicList, iRefIdx );  
  TComPicYuv* pcYuvBaseCol  = pcPicYuvBaseCol->getPicYuvRec();
  Int uiLCUAddr,uiAbsPartAddr;
  Int irefPUX = pcCU->getCUPelX() + g_auiRasterToPelX[g_auiZscanToRaster[uiPartAddr]] + iWidth/2  + ((cDMv.getHor() + 2)>>2);
  Int irefPUY = pcCU->getCUPelY() + g_auiRasterToPelY[g_auiZscanToRaster[uiPartAddr]] + iHeight/2 + ((cDMv.getVer() + 2)>>2);

  irefPUX = (Int)Clip3<Int>(0, pcCU->getSlice()->getSPS()-> getPicWidthInLumaSamples()-1, irefPUX);
  irefPUY = (Int)Clip3<Int>(0, pcCU->getSlice()->getSPS()->getPicHeightInLumaSamples()-1, irefPUY);  
  pcYuvBaseCol->getCUAddrAndPartIdx( irefPUX, irefPUY, uiLCUAddr, uiAbsPartAddr);
  TComDataCU *pColCU = pcPicYuvBaseCol->getCU( uiLCUAddr );

  TComPic* pcPicYuvBaseTRef = NULL;
  pcPicYuvCurrTRef = NULL;

  //If there is available motion in base reference list, use it
  if(!pColCU->isIntra(uiAbsPartAddr))
  {
    for(Int iList = 0; iList < (pColCU->getSlice()->isInterB() ? 2: 1); iList ++)
    {
      RefPicList eRefPicListCurr = RefPicList(iList);
      Int iRef = pColCU->getCUMvField(eRefPicListCurr)->getRefIdx(uiAbsPartAddr);
      if( iRef != -1)
      {
        pcPicYuvBaseTRef = pColCU->getSlice()->getRefPic(eRefPicListCurr, iRef);  
        Int  iCurrPOC    = pColCU->getSlice()->getPOC();
        Int  iCurrRefPOC = pcPicYuvBaseTRef->getPOC();
        Int  iCurrRef    = pcCU->getSlice()->getFirstTRefIdx(eRefPicListCurr);

        if( iCurrRef >= 0 && iCurrPOC != iCurrRefPOC)
        {
          pcPicYuvCurrTRef =  pcCU->getSlice()->getRefPic(eRefPicListCurr,iCurrRef);  
          Int iTargetPOC = pcPicYuvCurrTRef->getPOC();
          pcPicYuvBaseTRef =  pcCU->getSlice()->getBaseViewRefPic(iTargetPOC,  pcPicYuvBaseCol->getViewIndex() );  
          if(pcPicYuvBaseTRef)
          {
            cBaseTMV = pColCU->getCUMvField(eRefPicListCurr)->getMv(uiAbsPartAddr);
            Int iScale = pcCU-> xGetDistScaleFactor(iCurrPOC, iTargetPOC, iCurrPOC, iCurrRefPOC);
            if ( iScale != 4096 )
            {
              cBaseTMV = cBaseTMV.scaleMv( iScale );
            }
            iCurrTRefPoc = iTargetPOC;
            return true;
          }
        }
      }
    }
  }

  //If there is no available motion in base reference list, use ( 0, 0 )
  if( pcCU->getSlice()->getFirstTRefIdx( eBaseRefPicList ) >= 0 )
  {
    cBaseTMV.set( 0, 0 );
    pcPicYuvCurrTRef = pcCU->getSlice()->getRefPic( eBaseRefPicList,  pcCU->getSlice()->getFirstTRefIdx( eBaseRefPicList ) );
    iCurrTRefPoc = pcPicYuvCurrTRef->getPOC();
    return true;
  }

  return false;
}

Void TComPrediction::xPredInterUniARPviewRef( TComDataCU* pcCU, UInt uiPartAddr, Int iWidth, Int iHeight, RefPicList eRefPicList, TComYuv*& rpcYuvPred, Bool bi, TComMvField * pNewMvFiled )
{
  Int         iRefIdx       = pcCU->getCUMvField( eRefPicList )->getRefIdx( uiPartAddr );           
  TComMv      cDMv          = pcCU->getCUMvField( eRefPicList )->getMv( uiPartAddr );
  TComMv      cTempDMv      = cDMv;
  UChar       dW            = pcCU->getARPW ( uiPartAddr );

  TComPic* pcPicYuvBaseTRef = NULL;
  TComPic* pcPicYuvCurrTRef = NULL;
  TComPic* pcPicYuvBaseCol  = pcCU->getSlice()->getRefPic( eRefPicList, iRefIdx );  
  TComPicYuv* pcYuvBaseCol  = pcPicYuvBaseCol->getPicYuvRec();   
  Bool bTMVAvai = false;     
  TComMv cBaseTMV;
  if( pNewMvFiled )
  {
    iRefIdx = pNewMvFiled->getRefIdx(); 
    cDMv = pNewMvFiled->getMv();
  }
  pcCU->clipMv(cTempDMv);

  assert(dW > 0);
  if (!pcCU->getSlice()->getArpRefPicAvailable( eRefPicList, pcPicYuvBaseCol->getViewIndex()))
  {
    dW = 0;
  }
  Int uiLCUAddr,uiAbsPartAddr;
  Int irefPUX = pcCU->getCUPelX() + g_auiRasterToPelX[g_auiZscanToRaster[uiPartAddr]] + iWidth/2  + ((cDMv.getHor() + 2)>>2);
  Int irefPUY = pcCU->getCUPelY() + g_auiRasterToPelY[g_auiZscanToRaster[uiPartAddr]] + iHeight/2 + ((cDMv.getVer() + 2)>>2);

  irefPUX = (Int)Clip3<Int>(0, pcCU->getSlice()->getSPS()-> getPicWidthInLumaSamples()-1, irefPUX);
  irefPUY = (Int)Clip3<Int>(0, pcCU->getSlice()->getSPS()->getPicHeightInLumaSamples()-1, irefPUY);  
  pcYuvBaseCol->getCUAddrAndPartIdx( irefPUX, irefPUY, uiLCUAddr, uiAbsPartAddr);
  TComDataCU *pColCU = pcPicYuvBaseCol->getCU( uiLCUAddr );
  if( pcCU->getSlice()->isInterB() && !pcCU->getSlice()->getIsDepth() )
  {
    RefPicList eOtherRefList = ( eRefPicList == REF_PIC_LIST_0 ) ? REF_PIC_LIST_1 : REF_PIC_LIST_0;
    Int iOtherRefIdx = pcCU->getCUMvField( eOtherRefList )->getRefIdx( uiPartAddr );
    //The other prediction direction is temporal ARP
    if( iOtherRefIdx >= 0 && pcCU->getSlice()->getViewIndex() == pcCU->getSlice()->getRefPic( eOtherRefList, iOtherRefIdx )->getViewIndex() )
    {
      bTMVAvai = true;
      pcPicYuvBaseTRef = pcCU->getSlice()->getRefPic( eOtherRefList, iOtherRefIdx );
      Int  iCurrPOC    = pcCU->getSlice()->getPOC();
      Int  iCurrRefPOC = pcPicYuvBaseTRef->getPOC();
      Int  iCurrRef    = pcCU->getSlice()->getFirstTRefIdx( eOtherRefList );
      
      if( iCurrRef >= 0 )
      {
        pcPicYuvCurrTRef =  pcCU->getSlice()->getRefPic( eOtherRefList,iCurrRef );  
        Int iTargetPOC = pcPicYuvCurrTRef->getPOC();
        pcPicYuvBaseTRef =  pcCU->getSlice()->getBaseViewRefPic( iTargetPOC,  pcPicYuvBaseCol->getViewIndex() );
        if( pcPicYuvBaseTRef )
        {
          cBaseTMV = pcCU->getCUMvField( eOtherRefList )->getMv( uiPartAddr );
          Int iScale = pcCU-> xGetDistScaleFactor( iCurrPOC, iTargetPOC, iCurrPOC, iCurrRefPOC );
          if ( iScale != 4096 )
          {
            cBaseTMV = cBaseTMV.scaleMv( iScale );
          }
        }
        else
        {
          dW = 0;
        }
      }
      else
      {
        dW = 0;
      }
    }

    //Both prediction directions are inter-view ARP
    if ( iOtherRefIdx >= 0 && !bTMVAvai )
    {
      RefPicList eBaseList = REF_PIC_LIST_0;
      Int iCurrTRefPoc;
      bTMVAvai = ( eBaseList != eRefPicList ) && ( pcCU->getSlice()->getViewIndex() != pcCU->getSlice()->getRefPic( eOtherRefList, iOtherRefIdx )->getViewIndex() );

      if ( bTMVAvai )
      {
        if( xCheckBiInterviewARP( pcCU, uiPartAddr, iWidth, iHeight, eBaseList, pcPicYuvCurrTRef, cBaseTMV, iCurrTRefPoc ) )
        {
          pcPicYuvBaseTRef = pcCU->getSlice()->getBaseViewRefPic( iCurrTRefPoc,  pcPicYuvBaseCol->getViewIndex() );
          if ( pcPicYuvBaseTRef == NULL )
          {
            dW = 0;
          }
        }
        else
        {
          dW = 0;
        }
      }
    }
  }

  if( !pColCU->isIntra( uiAbsPartAddr ) && !bTMVAvai )
  {
    TComMvField puMVField;
    for(Int iList = 0; iList < (pColCU->getSlice()->isInterB() ? 2: 1) && !bTMVAvai; iList ++)
    {
      RefPicList eRefPicListCurr = RefPicList(iList);
      Int iRef = pColCU->getCUMvField(eRefPicListCurr)->getRefIdx(uiAbsPartAddr);
      if( iRef != -1)
      {
        pcPicYuvBaseTRef = pColCU->getSlice()->getRefPic(eRefPicListCurr, iRef);  
        Int  iCurrPOC    = pColCU->getSlice()->getPOC();
        Int  iCurrRefPOC = pcPicYuvBaseTRef->getPOC();
        Int  iCurrRef    = pcCU->getSlice()->getFirstTRefIdx(eRefPicListCurr);
        if (iCurrRef >= 0 && iCurrRefPOC != iCurrPOC)
        {
          pcPicYuvCurrTRef =  pcCU->getSlice()->getRefPic(eRefPicListCurr,iCurrRef);  
          Int iTargetPOC = pcPicYuvCurrTRef->getPOC();
          {
            pcPicYuvBaseTRef =  pcCU->getSlice()->getBaseViewRefPic(iTargetPOC,  pcPicYuvBaseCol->getViewIndex() );  
            if(pcPicYuvBaseTRef)
            {
              cBaseTMV = pColCU->getCUMvField(eRefPicListCurr)->getMv(uiAbsPartAddr);
              Int iScale = pcCU-> xGetDistScaleFactor(iCurrPOC, iTargetPOC, iCurrPOC, iCurrRefPOC);
              if ( iScale != 4096 )
                cBaseTMV = cBaseTMV.scaleMv( iScale );                  
              bTMVAvai = true;
              break;
            }
          }
        }
      }
    }
  }
  if (bTMVAvai == false)
  { 
    bTMVAvai = true;
    cBaseTMV.set(0, 0);
    pcPicYuvBaseTRef =  pColCU->getSlice()->getRefPic(eRefPicList,  pcCU->getSlice()->getFirstTRefIdx(eRefPicList));  
    pcPicYuvCurrTRef =  pcCU->getSlice()->getRefPic  (eRefPicList,  pcCU->getSlice()->getFirstTRefIdx(eRefPicList));      
  }

  xPredInterLumaBlk  ( pcCU, pcYuvBaseCol, uiPartAddr, &cTempDMv, iWidth, iHeight, rpcYuvPred, bi || ( dW > 0 && bTMVAvai ),        bTMVAvai);
  xPredInterChromaBlk( pcCU, pcYuvBaseCol, uiPartAddr, &cTempDMv, iWidth, iHeight, rpcYuvPred, bi || ( dW > 0 && bTMVAvai ),        bTMVAvai);

  if( dW > 0 && bTMVAvai ) 
  {
    TComYuv*    pYuvCurrTRef    = &m_acYuvPredBase[0];
    TComYuv*    pYuvBaseTRef    = &m_acYuvPredBase[1];
    TComPicYuv* pcYuvCurrTref   = pcPicYuvCurrTRef->getPicYuvRec();        
    TComPicYuv* pcYuvBaseTref   = pcPicYuvBaseTRef->getPicYuvRec();  
    TComMv      cTempMv         = cDMv + cBaseTMV;

    pcCU->clipMv(cBaseTMV);
    pcCU->clipMv(cTempMv);

    if (iWidth <= 8)
    {
      pYuvCurrTRef->clear(); pYuvBaseTRef->clear();
    }
    xPredInterLumaBlk  ( pcCU, pcYuvCurrTref, uiPartAddr, &cBaseTMV, iWidth, iHeight, pYuvCurrTRef, true,   true);

    if (iWidth > 8)
      xPredInterChromaBlk( pcCU, pcYuvCurrTref, uiPartAddr, &cBaseTMV, iWidth, iHeight, pYuvCurrTRef, true,   true);

    xPredInterLumaBlk  ( pcCU, pcYuvBaseTref, uiPartAddr, &cTempMv,  iWidth, iHeight, pYuvBaseTRef, true,   true); 

    if (iWidth > 8)
      xPredInterChromaBlk( pcCU, pcYuvBaseTref, uiPartAddr, &cTempMv,  iWidth, iHeight, pYuvBaseTRef, true,   true); 

    pYuvCurrTRef->subtractARP( pYuvCurrTRef , pYuvBaseTRef , uiPartAddr , iWidth , iHeight );  
    if(dW == 2)
    {
      pYuvCurrTRef->multiplyARP( uiPartAddr , iWidth , iHeight , dW );
    }
    rpcYuvPred->addARP( rpcYuvPred , pYuvCurrTRef , uiPartAddr , iWidth , iHeight , !bi ); 
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
  Int iRefOffset, iHor, iVer;
  iHor = pcCU->getSlice()->getIsDepth() ? pMv->getHor() : ( ( pMv->getHor() + 2 ) >> 2 );
  iVer = pcCU->getSlice()->getIsDepth() ? pMv->getVer() : ( ( pMv->getVer() + 2 ) >> 2 );
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

  if( pcCU->getPUAbove( uiTmpPartIdx, pcCU->getZorderIdxInCU() ) )
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
      if ( eType == TEXT_LUMA )
      {
        xx += (pRef[j] * pRef[j])>>precShift;
        xy += (pRef[j] * pRec[j])>>precShift;
      }
    }
    iCountShift += g_aucConvertToBit[ uiWidth ] + 1;
  }

  if( pcCU->getPULeft( uiTmpPartIdx, pcCU->getZorderIdxInCU() ) )
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
      if ( eType == TEXT_LUMA )
      {
        xx += (pRef[0] * pRef[0])>>precShift;
        xy += (pRef[0] * pRec[0])>>precShift;
      }
      pRef += iRefStride*2;
      pRec += iRecStride*2;
    }
    iCountShift += iCountShift > 0 ? 1 : ( g_aucConvertToBit[ uiWidth ] + 1 );
  }

  if( iCountShift == 0 )
  {
    a = ( 1 << IC_CONST_SHIFT );
    b = 0;
    return;
  }

  if (  eType != TEXT_LUMA )
  {
    a = 32;
    b = (  y - x + ( 1 << ( iCountShift - 1 ) ) ) >> iCountShift;
  }
  else
  {
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
}
#endif

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
    const Int  iTRR = ( patternStride * 2 - 1  ) - srcStride; 
    const Int  iLBB = ( patternStride * 2 - 1  ) * srcStride - 1;
    refDC1 = bL ? ( ptrSrc[iTR] + ptrSrc[iLB] )>>1 : (abs(ptrSrc[iTRR] - ptrSrc[-(Int)srcStride]) > abs(ptrSrc[iLBB] - ptrSrc[ -1]) ? ptrSrc[iTRR] : ptrSrc[iLBB]);
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

  iDC  = piRefBlkY[ 0 ];
  iDC += piRefBlkY[ uiWidth - 1 ];
  iDC += piRefBlkY[ uiWidth * (uiHeight - 1) ];
  iDC += piRefBlkY[ uiWidth * (uiHeight - 1) + uiWidth - 1 ];
  iDC = iDC >> 2;

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
  for( Int i = 0; i < uiNumSegments; i++ )
  {
    rpSegMeans[i] = 0;
  }
  if (orgDC == false)
  {
    Pel* pLeftTop = pOrig;
    Pel* pRightTop = pOrig + (uiSize-1);
    Pel* pLeftBottom = (pOrig+ (uiStride*(uiSize-1)));
    Pel* pRightBottom = (pOrig+ (uiStride*(uiSize-1)) + (uiSize-1));

    rpSegMeans[0] = (*pLeftTop + *pRightTop + *pLeftBottom + *pRightBottom + 2)>>2;
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
