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

/** \file     TComPrediction.cpp
    \brief    prediction class
*/

#include <memory.h>
#include "TComPrediction.h"
#include "TComPic.h"
#include "TComTU.h"

//! \ingroup TLibCommon
//! \{

// ====================================================================================================================
// Tables
// ====================================================================================================================

const UChar TComPrediction::m_aucIntraFilter[MAX_NUM_CHANNEL_TYPE][MAX_INTRA_FILTER_DEPTHS] =
{
  { // Luma
    10, //4x4
    7, //8x8
    1, //16x16
    0, //32x32
    10, //64x64
  },
  { // Chroma
    10, //4xn
    7, //8xn
    1, //16xn
    0, //32xn
    10, //64xn
  }

};

// ====================================================================================================================
// Constructor / destructor / initialize
// ====================================================================================================================

TComPrediction::TComPrediction()
: m_pLumaRecBuffer(0)
, m_iLumaRecStride(0)
{
  for(UInt ch=0; ch<MAX_NUM_COMPONENT; ch++)
  {
    for(UInt buf=0; buf<2; buf++)
    {
      m_piYuvExt[ch][buf] = NULL;
    }
  }
#if NH_3D_VSP
  m_pDepthBlock = (Int*) malloc(MAX_NUM_PART_IDXS_IN_CTU_WIDTH*MAX_NUM_PART_IDXS_IN_CTU_WIDTH*sizeof(Int));
  if (m_pDepthBlock == NULL)
  {
      printf("ERROR: UKTGHU, No memory allocated.\n");
  }
#endif

}

TComPrediction::~TComPrediction()
{
#if NH_3D_VSP
  if (m_pDepthBlock != NULL)
  {
    free(m_pDepthBlock);
  }
  m_cYuvDepthOnVsp.destroy();
#endif

  destroy();
}

Void TComPrediction::destroy()
{
  for(UInt ch=0; ch<MAX_NUM_COMPONENT; ch++)
  {
    for(UInt buf=0; buf<NUM_PRED_BUF; buf++)
    {
      delete [] m_piYuvExt[ch][buf];
      m_piYuvExt[ch][buf] = NULL;
    }
  }

  for(UInt i=0; i<NUM_REF_PIC_LIST_01; i++)
  {
    m_acYuvPred[i].destroy();
  }

  m_cYuvPredTemp.destroy();

#if NH_3D_ARP
  m_acYuvPredBase[0].destroy();
  m_acYuvPredBase[1].destroy();
#endif
  if( m_pLumaRecBuffer )
  {
    delete [] m_pLumaRecBuffer;
    m_pLumaRecBuffer = 0;
  }
  m_iLumaRecStride = 0;

  for (UInt i = 0; i < LUMA_INTERPOLATION_FILTER_SUB_SAMPLE_POSITIONS; i++)
  {
    for (UInt j = 0; j < LUMA_INTERPOLATION_FILTER_SUB_SAMPLE_POSITIONS; j++)
    {
      m_filteredBlock[i][j].destroy();
    }
    m_filteredBlockTmp[i].destroy();
  }
}

Void TComPrediction::initTempBuff(ChromaFormat chromaFormatIDC)
{
  // if it has been initialised before, but the chroma format has changed, release the memory and start again.
  if( m_piYuvExt[COMPONENT_Y][PRED_BUF_UNFILTERED] != NULL && m_cYuvPredTemp.getChromaFormat()!=chromaFormatIDC)
  {
    destroy();
  }

  if( m_piYuvExt[COMPONENT_Y][PRED_BUF_UNFILTERED] == NULL ) // check if first is null (in which case, nothing initialised yet)
  {
    Int extWidth  = MAX_CU_SIZE + 16;
    Int extHeight = MAX_CU_SIZE + 1;

    for (UInt i = 0; i < LUMA_INTERPOLATION_FILTER_SUB_SAMPLE_POSITIONS; i++)
    {
      m_filteredBlockTmp[i].create(extWidth, extHeight + 7, chromaFormatIDC);
      for (UInt j = 0; j < LUMA_INTERPOLATION_FILTER_SUB_SAMPLE_POSITIONS; j++)
      {
        m_filteredBlock[i][j].create(extWidth, extHeight, chromaFormatIDC);
      }
    }

    m_iYuvExtSize = (MAX_CU_SIZE*2+1) * (MAX_CU_SIZE*2+1);
    for(UInt ch=0; ch<MAX_NUM_COMPONENT; ch++)
    {
      for(UInt buf=0; buf<NUM_PRED_BUF; buf++)
      {
        m_piYuvExt[ch][buf] = new Pel[ m_iYuvExtSize ];
      }
    }

    // new structure
    for(UInt i=0; i<NUM_REF_PIC_LIST_01; i++)
    {
      m_acYuvPred[i] .create( MAX_CU_SIZE, MAX_CU_SIZE, chromaFormatIDC );
    }

    m_cYuvPredTemp.create( MAX_CU_SIZE, MAX_CU_SIZE, chromaFormatIDC );
#if NH_3D_ARP
    m_acYuvPredBase[0] .create( MAX_CU_SIZE, MAX_CU_SIZE, chromaFormatIDC );
    m_acYuvPredBase[1] .create( MAX_CU_SIZE, MAX_CU_SIZE, chromaFormatIDC );
#endif
#if NH_3D_VSP
    m_cYuvDepthOnVsp.create( MAX_CU_SIZE, MAX_CU_SIZE, chromaFormatIDC );
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
#if NH_3D_IC
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
//NOTE: Bit-Limit - 25-bit source
Pel TComPrediction::predIntraGetPredValDC( const Pel* pSrc, Int iSrcStride, UInt iWidth, UInt iHeight)
{
  assert(iWidth > 0 && iHeight > 0);
  Int iInd, iSum = 0;
  Pel pDcVal;

  for (iInd = 0;iInd < iWidth;iInd++)
  {
    iSum += pSrc[iInd-iSrcStride];
  }
  for (iInd = 0;iInd < iHeight;iInd++)
  {
    iSum += pSrc[iInd*iSrcStride-1];
  }

  pDcVal = (iSum + iWidth) / (iWidth + iHeight);

  return pDcVal;
}

// Function for deriving the angular Intra predictions

/** Function for deriving the simplified angular intra predictions.
 * \param bitDepth           bit depth
 * \param pSrc               pointer to reconstructed sample array
 * \param srcStride          the stride of the reconstructed sample array
 * \param pTrueDst           reference to pointer for the prediction sample array
 * \param dstStrideTrue      the stride of the prediction sample array
 * \param uiWidth            the width of the block
 * \param uiHeight           the height of the block
 * \param channelType        type of pel array (luma/chroma)
 * \param format             chroma format
 * \param dirMode            the intra prediction mode index
 * \param blkAboveAvailable  boolean indication if the block above is available
 * \param blkLeftAvailable   boolean indication if the block to the left is available
 * \param bEnableEdgeFilters indication whether to enable edge filters
 *
 * This function derives the prediction samples for the angular mode based on the prediction direction indicated by
 * the prediction mode index. The prediction direction is given by the displacement of the bottom row of the block and
 * the reference row above the block in the case of vertical prediction or displacement of the rightmost column
 * of the block and reference column left from the block in the case of the horizontal prediction. The displacement
 * is signalled at 1/32 pixel accuracy. When projection of the predicted pixel falls inbetween reference samples,
 * the predicted value for the pixel is linearly interpolated from the reference samples. All reference samples are taken
 * from the extended main reference.
 */
//NOTE: Bit-Limit - 25-bit source
Void TComPrediction::xPredIntraAng(       Int bitDepth,
                                    const Pel* pSrc,     Int srcStride,
                                          Pel* pTrueDst, Int dstStrideTrue,
                                          UInt uiWidth, UInt uiHeight, ChannelType channelType,
                                          UInt dirMode, const Bool bEnableEdgeFilters
                                  )
{
  Int width=Int(uiWidth);
  Int height=Int(uiHeight);

  // Map the mode index to main prediction direction and angle
  assert( dirMode != PLANAR_IDX ); //no planar
  const Bool modeDC        = dirMode==DC_IDX;

  // Do the DC prediction
  if (modeDC)
  {
    const Pel dcval = predIntraGetPredValDC(pSrc, srcStride, width, height);

    for (Int y=height;y>0;y--, pTrueDst+=dstStrideTrue)
    {
      for (Int x=0; x<width;) // width is always a multiple of 4.
      {
        pTrueDst[x++] = dcval;
      }
    }
  }
  else // Do angular predictions
  {
    const Bool       bIsModeVer         = (dirMode >= 18);
    const Int        intraPredAngleMode = (bIsModeVer) ? (Int)dirMode - VER_IDX :  -((Int)dirMode - HOR_IDX);
    const Int        absAngMode         = abs(intraPredAngleMode);
    const Int        signAng            = intraPredAngleMode < 0 ? -1 : 1;
    const Bool       edgeFilter         = bEnableEdgeFilters && isLuma(channelType) && (width <= MAXIMUM_INTRA_FILTERED_WIDTH) && (height <= MAXIMUM_INTRA_FILTERED_HEIGHT);

    // Set bitshifts and scale the angle parameter to block size
    static const Int angTable[9]    = {0,    2,    5,   9,  13,  17,  21,  26,  32};
    static const Int invAngTable[9] = {0, 4096, 1638, 910, 630, 482, 390, 315, 256}; // (256 * 32) / Angle
    Int invAngle                    = invAngTable[absAngMode];
    Int absAng                      = angTable[absAngMode];
    Int intraPredAngle              = signAng * absAng;

    Pel* refMain;
    Pel* refSide;

    Pel  refAbove[2*MAX_CU_SIZE+1];
    Pel  refLeft[2*MAX_CU_SIZE+1];

    // Initialize the Main and Left reference array.
    if (intraPredAngle < 0)
    {
      const Int refMainOffsetPreScale = (bIsModeVer ? height : width ) - 1;
      const Int refMainOffset         = height - 1;
      for (Int x=0;x<width+1;x++)
      {
        refAbove[x+refMainOffset] = pSrc[x-srcStride-1];
      }
      for (Int y=0;y<height+1;y++)
      {
        refLeft[y+refMainOffset] = pSrc[(y-1)*srcStride-1];
      }
      refMain = (bIsModeVer ? refAbove : refLeft)  + refMainOffset;
      refSide = (bIsModeVer ? refLeft  : refAbove) + refMainOffset;

      // Extend the Main reference to the left.
      Int invAngleSum    = 128;       // rounding for (shift by 8)
      for (Int k=-1; k>(refMainOffsetPreScale+1)*intraPredAngle>>5; k--)
      {
        invAngleSum += invAngle;
        refMain[k] = refSide[invAngleSum>>8];
      }
    }
    else
    {
      for (Int x=0;x<2*width+1;x++)
      {
        refAbove[x] = pSrc[x-srcStride-1];
      }
      for (Int y=0;y<2*height+1;y++)
      {
        refLeft[y] = pSrc[(y-1)*srcStride-1];
      }
      refMain = bIsModeVer ? refAbove : refLeft ;
      refSide = bIsModeVer ? refLeft  : refAbove;
    }

    // swap width/height if we are doing a horizontal mode:
    Pel tempArray[MAX_CU_SIZE*MAX_CU_SIZE];
    const Int dstStride = bIsModeVer ? dstStrideTrue : MAX_CU_SIZE;
    Pel *pDst = bIsModeVer ? pTrueDst : tempArray;
    if (!bIsModeVer)
    {
      std::swap(width, height);
    }

    if (intraPredAngle == 0)  // pure vertical or pure horizontal
    {
      for (Int y=0;y<height;y++)
      {
        for (Int x=0;x<width;x++)
        {
          pDst[y*dstStride+x] = refMain[x+1];
        }
      }

      if (edgeFilter)
      {
        for (Int y=0;y<height;y++)
        {
          pDst[y*dstStride] = Clip3 (0, ((1 << bitDepth) - 1), pDst[y*dstStride] + (( refSide[y+1] - refSide[0] ) >> 1) );
        }
      }
    }
    else
    {
      Pel *pDsty=pDst;

      for (Int y=0, deltaPos=intraPredAngle; y<height; y++, deltaPos+=intraPredAngle, pDsty+=dstStride)
      {
        const Int deltaInt   = deltaPos >> 5;
        const Int deltaFract = deltaPos & (32 - 1);

        if (deltaFract)
        {
          // Do linear filtering
          const Pel *pRM=refMain+deltaInt+1;
          Int lastRefMainPel=*pRM++;
          for (Int x=0;x<width;pRM++,x++)
          {
            Int thisRefMainPel=*pRM;
            pDsty[x+0] = (Pel) ( ((32-deltaFract)*lastRefMainPel + deltaFract*thisRefMainPel +16) >> 5 );
            lastRefMainPel=thisRefMainPel;
          }
        }
        else
        {
          // Just copy the integer samples
          for (Int x=0;x<width; x++)
          {
            pDsty[x] = refMain[x+deltaInt+1];
          }
        }
      }
    }

    // Flip the block if this is the horizontal mode
    if (!bIsModeVer)
    {
      for (Int y=0; y<height; y++)
      {
        for (Int x=0; x<width; x++)
        {
          pTrueDst[x*dstStrideTrue] = pDst[x];
        }
        pTrueDst++;
        pDst+=dstStride;
      }
    }
  }
}

Void TComPrediction::predIntraAng( const ComponentID compID, UInt uiDirMode, Pel* piOrg /* Will be null for decoding */, UInt uiOrgStride, Pel* piPred, UInt uiStride, TComTU &rTu, const Bool bUseFilteredPredSamples, const Bool bUseLosslessDPCM )
{
  const ChannelType    channelType = toChannelType(compID);
  const TComRectangle &rect        = rTu.getRect(isLuma(compID) ? COMPONENT_Y : COMPONENT_Cb);
  const Int            iWidth      = rect.width;
  const Int            iHeight     = rect.height;

  assert( g_aucConvertToBit[ iWidth ] >= 0 ); //   4x  4
  assert( g_aucConvertToBit[ iWidth ] <= 5 ); // 128x128
  //assert( iWidth == iHeight  );

        Pel *pDst = piPred;

  // get starting pixel in block
  const Int sw = (2 * iWidth + 1);

  if ( bUseLosslessDPCM )
  {
    const Pel *ptrSrc = getPredictorPtr( compID, false );
    // Sample Adaptive intra-Prediction (SAP)
    if (uiDirMode==HOR_IDX)
    {
      // left column filled with reference samples
      // remaining columns filled with piOrg data (if available).
      for(Int y=0; y<iHeight; y++)
      {
        piPred[y*uiStride+0] = ptrSrc[(y+1)*sw];
      }
      if (piOrg!=0)
      {
        piPred+=1; // miss off first column
        for(Int y=0; y<iHeight; y++, piPred+=uiStride, piOrg+=uiOrgStride)
        {
          memcpy(piPred, piOrg, (iWidth-1)*sizeof(Pel));
        }
      }
    }
    else // VER_IDX
    {
      // top row filled with reference samples
      // remaining rows filled with piOrd data (if available)
      for(Int x=0; x<iWidth; x++)
      {
        piPred[x] = ptrSrc[x+1];
      }
      if (piOrg!=0)
      {
        piPred+=uiStride; // miss off the first row
        for(Int y=1; y<iHeight; y++, piPred+=uiStride, piOrg+=uiOrgStride)
        {
          memcpy(piPred, piOrg, iWidth*sizeof(Pel));
        }
      }
    }
  }
  else
  {
    const Pel *ptrSrc = getPredictorPtr( compID, bUseFilteredPredSamples );

    if ( uiDirMode == PLANAR_IDX )
    {
      xPredIntraPlanar( ptrSrc+sw+1, sw, pDst, uiStride, iWidth, iHeight );
    }
    else
    {
      // Create the prediction
            TComDataCU *const pcCU              = rTu.getCU();
      const UInt              uiAbsPartIdx      = rTu.GetAbsPartIdxTU();
      const Bool              enableEdgeFilters = !(pcCU->isRDPCMEnabled(uiAbsPartIdx) && pcCU->getCUTransquantBypass(uiAbsPartIdx));
#if O0043_BEST_EFFORT_DECODING
      const Int channelsBitDepthForPrediction = rTu.getCU()->getSlice()->getSPS()->getStreamBitDepth(channelType);
#else
      const Int channelsBitDepthForPrediction = rTu.getCU()->getSlice()->getSPS()->getBitDepth(channelType);
#endif
      xPredIntraAng( channelsBitDepthForPrediction, ptrSrc+sw+1, sw, pDst, uiStride, iWidth, iHeight, channelType, uiDirMode, enableEdgeFilters );

      if( uiDirMode == DC_IDX )
      {
        xDCPredFiltering( ptrSrc+sw+1, sw, pDst, uiStride, iWidth, iHeight, channelType );
      }
    }
  }

}

#if NH_3D_DMM
Void TComPrediction::predIntraLumaDmm( TComDataCU* pcCU, UInt uiAbsPartIdx, DmmID dmmType, Pel* piPred, UInt uiStride, Int iWidth, Int iHeight )
{
  assert( iWidth == iHeight  );
  assert( iWidth >= DMM_MIN_SIZE && iWidth <= DMM_MAX_SIZE );
#if NH_3D_SDC_INTRA
  assert( !pcCU->getSDCFlag( uiAbsPartIdx ) );
#endif

  // get partition
  Bool* biSegPattern  = new Bool[ (UInt)(iWidth*iHeight) ];
  UInt  patternStride = (UInt)iWidth;
  switch( dmmType )
  {
  case( DMM1_IDX ): { (getWedgeListScaled( (UInt)iWidth )->at( pcCU->getDmm1WedgeTabIdx( uiAbsPartIdx ) )).getPatternScaledCopy( (UInt)iWidth, biSegPattern ); } break;
  case( DMM4_IDX ): { predContourFromTex( pcCU, uiAbsPartIdx, iWidth, iHeight, biSegPattern );                                                                 } break;
  default: assert(0);
  }

  // get predicted partition values
  Pel predDC1 = 0, predDC2 = 0;
  predBiSegDCs( pcCU, uiAbsPartIdx, iWidth, iHeight, biSegPattern, patternStride, predDC1, predDC2 );

  // set segment values with deltaDC offsets
  Pel segDC1 = 0, segDC2 = 0;
  Pel deltaDC1 = pcCU->getDmmDeltaDC( dmmType, 0, uiAbsPartIdx );
  Pel deltaDC2 = pcCU->getDmmDeltaDC( dmmType, 1, uiAbsPartIdx );
#if NH_3D_DLT
  segDC1 = pcCU->getSlice()->getPPS()->getDLT()->idx2DepthValue( pcCU->getSlice()->getLayerIdInVps(), pcCU->getSlice()->getPPS()->getDLT()->depthValue2idx( pcCU->getSlice()->getLayerIdInVps(), predDC1 ) + deltaDC1 );
  segDC2 = pcCU->getSlice()->getPPS()->getDLT()->idx2DepthValue( pcCU->getSlice()->getLayerIdInVps(), pcCU->getSlice()->getPPS()->getDLT()->depthValue2idx( pcCU->getSlice()->getLayerIdInVps(), predDC2 ) + deltaDC2 );
#else
  segDC1 = ClipBD( predDC1 + deltaDC1, pcCU->getSlice()->getSPS()->getBitDepth(CHANNEL_TYPE_LUMA) );
  segDC2 = ClipBD( predDC2 + deltaDC2, pcCU->getSlice()->getSPS()->getBitDepth(CHANNEL_TYPE_LUMA) );
#endif

  // set prediction signal
  Pel* pDst = piPred;
  assignBiSegDCs( pDst, uiStride, biSegPattern, patternStride, segDC1, segDC2 );
  
  delete[] biSegPattern;
}
#endif

/** Check for identical motion in both motion vector direction of a bi-directional predicted CU
  * \returns true, if motion vectors and reference pictures match
 */
Bool TComPrediction::xCheckIdenticalMotion ( TComDataCU* pcCU, UInt PartAddr )
{
  if( pcCU->getSlice()->isInterB() && !pcCU->getSlice()->getPPS()->getWPBiPred() )
  {
    if( pcCU->getCUMvField(REF_PIC_LIST_0)->getRefIdx(PartAddr) >= 0 && pcCU->getCUMvField(REF_PIC_LIST_1)->getRefIdx(PartAddr) >= 0)
    {
      Int RefPOCL0 = pcCU->getSlice()->getRefPic(REF_PIC_LIST_0, pcCU->getCUMvField(REF_PIC_LIST_0)->getRefIdx(PartAddr))->getPOC();
      Int RefPOCL1 = pcCU->getSlice()->getRefPic(REF_PIC_LIST_1, pcCU->getCUMvField(REF_PIC_LIST_1)->getRefIdx(PartAddr))->getPOC();
#if NH_MV_FIX_TICKET_106
#if NH_MV
      Int layerIdL0 = pcCU->getSlice()->getRefPic(REF_PIC_LIST_0, pcCU->getCUMvField(REF_PIC_LIST_0)->getRefIdx(PartAddr))->getLayerId();
      Int layerIdL1 = pcCU->getSlice()->getRefPic(REF_PIC_LIST_1, pcCU->getCUMvField(REF_PIC_LIST_1)->getRefIdx(PartAddr))->getLayerId();
#if NH_3D_ARP
      if(!pcCU->getARPW(PartAddr) && RefPOCL0 == RefPOCL1 && layerIdL0 == layerIdL1 && pcCU->getCUMvField(REF_PIC_LIST_0)->getMv(PartAddr) == pcCU->getCUMvField(REF_PIC_LIST_1)->getMv(PartAddr))
#else
      if(RefPOCL0 == RefPOCL1 && layerIdL0 == layerIdL1 && pcCU->getCUMvField(REF_PIC_LIST_0)->getMv(PartAddr) == pcCU->getCUMvField(REF_PIC_LIST_1)->getMv(PartAddr))
#endif
#else
      if(RefPOCL0 == RefPOCL1 && pcCU->getCUMvField(REF_PIC_LIST_0)->getMv(PartAddr) == pcCU->getCUMvField(REF_PIC_LIST_1)->getMv(PartAddr))
#endif
#else
#if NH_3D_ARP
      if(!pcCU->getARPW(PartAddr) && RefPOCL0 == RefPOCL1 && pcCU->getCUMvField(REF_PIC_LIST_0)->getMv(PartAddr) == pcCU->getCUMvField(REF_PIC_LIST_1)->getMv(PartAddr))
#else
      if(RefPOCL0 == RefPOCL1 && pcCU->getCUMvField(REF_PIC_LIST_0)->getMv(PartAddr) == pcCU->getCUMvField(REF_PIC_LIST_1)->getMv(PartAddr))
#endif
#endif
      {
        return true;
      }
    }
  }
  return false;
}

#if NH_3D_SPIVMP
Void TComPrediction::xGetSubPUAddrAndMerge(TComDataCU* pcCU, UInt uiPartAddr, Int iSPWidth, Int iSPHeight, Int iNumSPInOneLine, Int iNumSP, UInt* uiMergedSPW, UInt* uiMergedSPH, UInt* uiSPAddr )
{
  for (Int i = 0; i < iNumSP; i++)
  {
    uiMergedSPW[i] = iSPWidth;
    uiMergedSPH[i] = iSPHeight;
    pcCU->getSPAbsPartIdx(uiPartAddr, iSPWidth, iSPHeight, i, iNumSPInOneLine, uiSPAddr[i]);
  }
#if NH_3D_ARP
  if( pcCU->getARPW( uiPartAddr ) != 0 )
  {
    return;
  }
#endif

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

#if NH_3D_DBBP
PartSize TComPrediction::getPartitionSizeFromDepth(Pel* pDepthPels, UInt uiDepthStride, UInt uiSize, TComDataCU*& pcCU)
{
  const TComSPS* sps = pcCU->getSlice()->getSPS();
  UInt uiMaxCUWidth = sps->getMaxCUWidth();
  UInt uiMaxCUHeight = sps->getMaxCUHeight();
  
  // find virtual partitioning for this CU based on depth block
  // segmentation of texture block --> mask IDs
  Pel*  pDepthBlockStart      = pDepthPels;

  // first compute average of depth block for thresholding
  Int iSumDepth = 0;
  Int iSubSample = 4;
  Int iPictureWidth = pcCU->getSlice()->getIvPic (true, pcCU->getDvInfo(0).m_aVIdxCan)->getPicYuvRec()->getWidth(COMPONENT_Y);
  Int iPictureHeight = pcCU->getSlice()->getIvPic (true, pcCU->getDvInfo(0).m_aVIdxCan)->getPicYuvRec()->getHeight(COMPONENT_Y);
  TComMv cDv = pcCU->getSlice()->getDepthRefinementFlag(  ) ? pcCU->getDvInfo(0).m_acDoNBDV : pcCU->getDvInfo(0).m_acNBDV;
  if( pcCU->getSlice()->getDepthRefinementFlag(  ) )
  {
    cDv.setVer(0);
  }
  Int iBlkX = ( pcCU->getCtuRsAddr() % pcCU->getSlice()->getIvPic (true, pcCU->getDvInfo(0).m_aVIdxCan)->getFrameWidthInCtus() ) * uiMaxCUWidth  + g_auiRasterToPelX[ g_auiZscanToRaster[ pcCU->getZorderIdxInCtu() ] ]+ ((cDv.getHor()+2)>>2);
  Int iBlkY = ( pcCU->getCtuRsAddr() / pcCU->getSlice()->getIvPic (true, pcCU->getDvInfo(0).m_aVIdxCan)->getFrameWidthInCtus() ) * uiMaxCUHeight + g_auiRasterToPelY[ g_auiZscanToRaster[ pcCU->getZorderIdxInCtu() ] ]+ ((cDv.getVer()+2)>>2);
  
  UInt t=0;

  for (Int y=0; y<uiSize; y+=iSubSample)
  {
    for (Int x=0; x<uiSize; x+=iSubSample)
    {
      if (iBlkX+x>iPictureWidth)
      {
        Int depthPel = pDepthPels[t];
        iSumDepth += depthPel;
      } 
      else
      {
        Int depthPel = pDepthPels[x];
        t=x;
        iSumDepth += depthPel;
      }
    }

    // next row
    if (!(iBlkY+y+4>iPictureHeight))
    {
      pDepthPels += uiDepthStride*iSubSample;
    }
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
      Int depthPel = 0;
      if (iBlkX+x>iPictureWidth)
      {
        depthPel = pDepthPels[t];
      }
      else
      { 
        depthPel = pDepthPels[x];
        t=x;
      }

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
    if (!(iBlkY+y+4>iPictureHeight))
    {
      pDepthPels += uiDepthStride*iSubSample;
    }
  }

  PartSize matchedPartSize = NUMBER_OF_PART_SIZES;

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

  AOF( matchedPartSize != NUMBER_OF_PART_SIZES );

  return matchedPartSize;
}

Bool TComPrediction::getSegmentMaskFromDepth( Pel* pDepthPels, UInt uiDepthStride, UInt uiWidth, UInt uiHeight, Bool* pMask, TComDataCU*& pcCU)
{
  const TComSPS* sps = pcCU->getSlice()->getSPS();
  UInt uiMaxCUWidth = sps->getMaxCUWidth();
  UInt uiMaxCUHeight = sps->getMaxCUHeight();
  
  // segmentation of texture block --> mask IDs
  Pel*  pDepthBlockStart      = pDepthPels;

  // first compute average of depth block for thresholding
  Int iSumDepth = 0;
  Int uiMinDepth = MAX_INT;
  Int uiMaxDepth = 0;
  uiMinDepth = pDepthPels[ 0 ];
  uiMaxDepth = pDepthPels[ 0 ];
  iSumDepth  = pDepthPels[ 0 ];
  
  Int iPictureWidth = pcCU->getSlice()->getIvPic (true, pcCU->getDvInfo(0).m_aVIdxCan)->getPicYuvRec()->getWidth(COMPONENT_Y);
  Int iPictureHeight = pcCU->getSlice()->getIvPic (true, pcCU->getDvInfo(0).m_aVIdxCan)->getPicYuvRec()->getHeight(COMPONENT_Y);  
  TComMv cDv = pcCU->getSlice()->getDepthRefinementFlag(  ) ? pcCU->getDvInfo(0).m_acDoNBDV : pcCU->getDvInfo(0).m_acNBDV;
  if( pcCU->getSlice()->getDepthRefinementFlag(  ) )
  {
    cDv.setVer(0);
  }
  Int iBlkX = ( pcCU->getCtuRsAddr() % pcCU->getSlice()->getIvPic (true, pcCU->getDvInfo(0).m_aVIdxCan)->getFrameWidthInCtus() ) * uiMaxCUWidth  + g_auiRasterToPelX[ g_auiZscanToRaster[ pcCU->getZorderIdxInCtu() ] ]+ ((cDv.getHor()+2)>>2);
  Int iBlkY = ( pcCU->getCtuRsAddr() / pcCU->getSlice()->getIvPic (true, pcCU->getDvInfo(0).m_aVIdxCan)->getFrameWidthInCtus() ) * uiMaxCUHeight + g_auiRasterToPelY[ g_auiZscanToRaster[ pcCU->getZorderIdxInCtu() ] ]+ ((cDv.getVer()+2)>>2);
  if (iBlkX>(Int)(iPictureWidth - uiWidth))
  {
    iSumDepth += pDepthPels[ iPictureWidth - iBlkX - 1 ];
    uiMinDepth = std::min( uiMinDepth, (Int)pDepthPels[ iPictureWidth - iBlkX - 1 ]);
    uiMaxDepth = std::max( uiMaxDepth, (Int)pDepthPels[ iPictureWidth - iBlkX - 1 ]);
  }
  else
  {
    iSumDepth += pDepthPels[ uiWidth - 1 ];
    uiMinDepth = std::min( uiMinDepth, (Int)pDepthPels[ uiWidth - 1 ]);
    uiMaxDepth = std::max( uiMaxDepth, (Int)pDepthPels[ uiWidth - 1 ]);
  }
  if (iBlkY>(Int)(iPictureHeight - uiHeight))
  {
    iSumDepth += pDepthPels[ uiDepthStride * (iPictureHeight - iBlkY - 1) ];
    uiMinDepth = std::min( uiMinDepth, (Int)pDepthPels[ uiDepthStride * (iPictureHeight - iBlkY - 1) ]);
    uiMaxDepth = std::max( uiMaxDepth, (Int)pDepthPels[ uiDepthStride * (iPictureHeight - iBlkY - 1) ]);
  }
  else
  {
    iSumDepth += pDepthPels[ uiDepthStride * (uiHeight - 1) ];
    uiMinDepth = std::min( uiMinDepth, (Int)pDepthPels[ uiDepthStride * (uiHeight - 1) ]);
    uiMaxDepth = std::max( uiMaxDepth, (Int)pDepthPels[ uiDepthStride * (uiHeight - 1) ]);
  }
  if (iBlkY>(Int)(iPictureHeight - uiHeight) && iBlkX>(Int)(iPictureWidth - uiWidth))
  {
    iSumDepth += pDepthPels[ uiDepthStride * (iPictureHeight - iBlkY - 1) + iPictureWidth - iBlkX - 1 ];
    uiMinDepth = std::min( uiMinDepth, (Int)pDepthPels[ uiDepthStride * (iPictureHeight - iBlkY - 1) + iPictureWidth - iBlkX - 1 ]);
    uiMaxDepth = std::max( uiMaxDepth, (Int)pDepthPels[ uiDepthStride * (iPictureHeight - iBlkY - 1) + iPictureWidth - iBlkX - 1 ]);
  }
  else if (iBlkY>(Int)(iPictureHeight - uiHeight))
  {
    iSumDepth += pDepthPels[ uiDepthStride * (iPictureHeight - iBlkY - 1) + uiWidth - 1 ];
    uiMinDepth = std::min( uiMinDepth, (Int)pDepthPels[ uiDepthStride * (iPictureHeight - iBlkY - 1) + uiWidth - 1 ]);
    uiMaxDepth = std::max( uiMaxDepth, (Int)pDepthPels[ uiDepthStride * (iPictureHeight - iBlkY - 1) + uiWidth - 1 ]);
  }
  else if (iBlkX>(Int)(iPictureWidth - uiWidth))
  {
    iSumDepth += pDepthPels[ uiDepthStride * (uiHeight - 1) + iPictureWidth - iBlkX - 1 ];
    uiMinDepth = std::min( uiMinDepth, (Int)pDepthPels[ uiDepthStride * (uiHeight - 1) + iPictureWidth - iBlkX - 1 ]);
    uiMaxDepth = std::max( uiMaxDepth, (Int)pDepthPels[ uiDepthStride * (uiHeight - 1) + iPictureWidth - iBlkX - 1 ]);
  }
  else
  {
    iSumDepth += pDepthPels[ uiDepthStride * (uiHeight - 1) + uiWidth - 1 ];
    uiMinDepth = std::min( uiMinDepth, (Int)pDepthPels[ uiDepthStride * (uiHeight - 1) + uiWidth - 1 ]);
    uiMaxDepth = std::max( uiMaxDepth, (Int)pDepthPels[ uiDepthStride * (uiHeight - 1) + uiWidth - 1 ]);
  }

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
  UInt t=0;
  UInt uiSumPix[2] = {0,0};
  for (Int y=0; y<uiHeight; y++)
  {
    for (Int x=0; x<uiHeight; x++)
    {
      Int depthPel = 0;
      if (iBlkX+x>iPictureWidth)
      {
        depthPel = pDepthPels[t];
      }
      else
      {
        depthPel = pDepthPels[x];
        t=x;
      }

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
    if (!(iBlkY+y+1>iPictureHeight))
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

Void TComPrediction::combineSegmentsWithMask( TComYuv* pInYuv[2], TComYuv* pOutYuv, Bool* pMask, UInt uiWidth, UInt uiHeight, UInt uiPartAddr, UInt partSize, Int bitDepthY )
{
  Pel*  piSrc[2]    = {pInYuv[0]->getAddr(COMPONENT_Y, uiPartAddr), pInYuv[1]->getAddr(COMPONENT_Y, uiPartAddr)};
  UInt  uiSrcStride = pInYuv[0]->getStride(COMPONENT_Y);
  Pel*  piDst       = pOutYuv->getAddr(COMPONENT_Y, uiPartAddr);
  UInt  uiDstStride = pOutYuv->getStride(COMPONENT_Y);
  
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
        
        piDst[x] = (l!=r) ? ClipBD( Pel(( left + (tmpTar[y*uiWidth+x] << 1) + right ) >> 2 ), bitDepthY) : tmpTar[y*uiWidth+x];
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
        
        piDst[x] = (t!=b) ? ClipBD( Pel(( top + (tmpTar[y*uiWidth+x] << 1) + bottom ) >> 2 ), bitDepthY) : tmpTar[y*uiWidth+x];
      }
      piDst     += uiDstStride;
    }
  }

  if ( tmpTar    ) 
  { 
    xFree(tmpTar);             
    tmpTar        = NULL; 
  }
  
  // now combine chroma
  Pel*  piSrcU[2]       = { pInYuv[0]->getAddr(COMPONENT_Cb, uiPartAddr), pInYuv[1]->getAddr(COMPONENT_Cb, uiPartAddr) };
  Pel*  piSrcV[2]       = { pInYuv[0]->getAddr(COMPONENT_Cr, uiPartAddr), pInYuv[1]->getAddr(COMPONENT_Cr, uiPartAddr) };
  UInt  uiSrcStrideC    = pInYuv[0]->getStride(COMPONENT_Cb);
  Pel*  piDstU          = pOutYuv->getAddr(COMPONENT_Cb, uiPartAddr);
  Pel*  piDstV          = pOutYuv->getAddr(COMPONENT_Cr, uiPartAddr);
  UInt  uiDstStrideC    = pOutYuv->getStride(COMPONENT_Cb);
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
          filSrcU = ClipBD( Pel(( leftU + (tmpTarU[y*uiWidthC+x] << 1) + rightU ) >> 2 ), bitDepthY);
          filSrcV = ClipBD( Pel(( leftV + (tmpTarV[y*uiWidthC+x] << 1) + rightV ) >> 2 ), bitDepthY);
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
          filSrcU = ClipBD( Pel(( topU + (tmpTarU[y*uiWidthC+x] << 1) + bottomU ) >> 2 ), bitDepthY);
          filSrcV = ClipBD( Pel(( topV + (tmpTarV[y*uiWidthC+x] << 1) + bottomV ) >> 2 ), bitDepthY);
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

  if( tmpTarU )
  {
    xFree(tmpTarU);
    tmpTarU        = NULL;
  }
  if ( tmpTarV    ) 
  {
    xFree(tmpTarV);
    tmpTarV        = NULL; 
  }
}
#endif

Void TComPrediction::motionCompensation ( TComDataCU* pcCU, TComYuv* pcYuvPred, RefPicList eRefPicList, Int iPartIdx )
{
  Int         iWidth;
  Int         iHeight;
  UInt        uiPartAddr;
  const TComSlice *pSlice    = pcCU->getSlice();
  const SliceType  sliceType = pSlice->getSliceType();
  const TComPPS   &pps       = *(pSlice->getPPS());

  if ( iPartIdx >= 0 )
  {
    pcCU->getPartIndexAndSize( iPartIdx, uiPartAddr, iWidth, iHeight );
#if NH_3D_VSP
    if ( pcCU->getVSPFlag(uiPartAddr) == 0)
    {
#endif
     if ( eRefPicList != REF_PIC_LIST_X )
     {
      if( (sliceType == P_SLICE && pps.getUseWP()) || (sliceType == B_SLICE && pps.getWPBiPred()))
      {
        xPredInterUni (pcCU, uiPartAddr, iWidth, iHeight, eRefPicList, pcYuvPred, true );
        xWeightedPredictionUni( pcCU, pcYuvPred, uiPartAddr, iWidth, iHeight, eRefPicList, pcYuvPred );
      }
      else
      {
        xPredInterUni (pcCU, uiPartAddr, iWidth, iHeight, eRefPicList, pcYuvPred );
      }
     }
     else
     {
#if NH_3D_SPIVMP
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
#if NH_3D_SPIVMP
        }
#endif
      }
#if NH_3D_VSP
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

#if NH_3D_VSP
    if ( pcCU->getVSPFlag(uiPartAddr) == 0 )
    {
#endif
    if ( eRefPicList != REF_PIC_LIST_X )
    {
      if( (sliceType == P_SLICE && pps.getUseWP()) || (sliceType == B_SLICE && pps.getWPBiPred()))
      {
        xPredInterUni (pcCU, uiPartAddr, iWidth, iHeight, eRefPicList, pcYuvPred, true );
        xWeightedPredictionUni( pcCU, pcYuvPred, uiPartAddr, iWidth, iHeight, eRefPicList, pcYuvPred );
      }
      else
      {
        xPredInterUni (pcCU, uiPartAddr, iWidth, iHeight, eRefPicList, pcYuvPred );
      }
    }
    else
    {
#if NH_3D_SPIVMP
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
#if NH_3D_SPIVMP
      }
#endif
    }
#if NH_3D_VSP
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

Void TComPrediction::xPredInterUni ( TComDataCU* pcCU, UInt uiPartAddr, Int iWidth, Int iHeight, RefPicList eRefPicList, TComYuv* pcYuvPred, Bool bi )
{
  Int         iRefIdx     = pcCU->getCUMvField( eRefPicList )->getRefIdx( uiPartAddr );           assert (iRefIdx >= 0);
  TComMv      cMv         = pcCU->getCUMvField( eRefPicList )->getMv( uiPartAddr );
  pcCU->clipMv(cMv);

#if ENC_DEC_TRACE && H_MV_ENC_DEC_TRAC
  if ( g_traceMotionInfoBeforUniPred  )
  {
    std::cout << "RefPic POC     : " << pcCU->getSlice()->getRefPic( eRefPicList, iRefIdx )->getPOC()     << std::endl; 
    std::cout << "RefPic Layer Id: " << pcCU->getSlice()->getRefPic( eRefPicList, iRefIdx )->getLayerId() << std::endl; 
    std::cout << "RefIdx         : " << iRefIdx                                                           << std::endl; 
    std::cout << "RefPIcList     : " << eRefPicList                                                        << std::endl; 
  }
#endif

#if NH_MV
  pcCU->checkMvVertRest(cMv, eRefPicList, iRefIdx );
#endif
#if NH_3D_ARP
  if(  pcCU->getARPW( uiPartAddr ) > 0 )
  {
    if( pcCU->getSlice()->getRefPic( eRefPicList, iRefIdx )->getPOC()== pcCU->getSlice()->getPOC() )
    {
      xPredInterUniARPviewRef( pcCU , uiPartAddr , iWidth , iHeight , eRefPicList , pcYuvPred , bi ); 
    }
    else
    {
      xPredInterUniARP( pcCU, uiPartAddr, iWidth, iHeight, eRefPicList, pcYuvPred, bi );
    }      
  }
  else
  {
#endif

    for (UInt comp=COMPONENT_Y; comp<pcYuvPred->getNumberValidComponents(); comp++)
    {
      const ComponentID compID=ComponentID(comp);
#if NH_3D_IC
      Bool bICFlag = pcCU->getICFlag( uiPartAddr ) && ( pcCU->getSlice()->getRefPic( eRefPicList, iRefIdx )->getViewIndex() != pcCU->getSlice()->getViewIndex() ) && ( isLuma(compID) || (iWidth > 8) );
      xPredInterBlk(compID,  pcCU, pcCU->getSlice()->getRefPic( eRefPicList, iRefIdx )->getPicYuvRec(), uiPartAddr, &cMv, iWidth, iHeight, pcYuvPred, bi, pcCU->getSlice()->getSPS()->getBitDepth(toChannelType(compID))
#if NH_3D_ARP
        , false
#endif
        , bICFlag );
#else
      xPredInterBlk  (compID,  pcCU, pcCU->getSlice()->getRefPic( eRefPicList, iRefIdx )->getPicYuvRec(), uiPartAddr, &cMv, iWidth, iHeight, pcYuvPred, bi, pcCU->getSlice()->getSPS()->getBitDepth(toChannelType(compID)) );
#endif
    }
#if NH_3D_ARP
  }
#endif
}

#if NH_3D_VSP
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
  UInt numPartsInLine       = pcCU->getPic()->getNumPartInCtuWidth();
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

      xPredInterBlk( COMPONENT_Y,  pcCU, pcCU->getSlice()->getRefPic( eRefPicList, refIdx )->getPicYuvRec(), partAddrSubPU, &cMv, widthSubPU, heightSubPU, rpcYuvPred, bi, pcCU->getSlice()->getSPS()->getBitDepth(CHANNEL_TYPE_LUMA) );
      xPredInterBlk( COMPONENT_Cb, pcCU, pcCU->getSlice()->getRefPic( eRefPicList, refIdx )->getPicYuvRec(), partAddrSubPU, &cMv, widthSubPU, heightSubPU, rpcYuvPred, bi, pcCU->getSlice()->getSPS()->getBitDepth(CHANNEL_TYPE_CHROMA) );
      xPredInterBlk( COMPONENT_Cr, pcCU, pcCU->getSlice()->getRefPic( eRefPicList, refIdx )->getPicYuvRec(), partAddrSubPU, &cMv, widthSubPU, heightSubPU, rpcYuvPred, bi, pcCU->getSlice()->getSPS()->getBitDepth(CHANNEL_TYPE_CHROMA) );
    }
  }
}
#endif

#if NH_3D_ARP
//temporal ARP
Void TComPrediction::xPredInterUniARP( TComDataCU* pcCU, UInt uiPartAddr, Int iWidth, Int iHeight, RefPicList eRefPicList, TComYuv*& rpcYuvPred, Bool bi )
{
  Int         iRefIdx      = pcCU->getCUMvField( eRefPicList )->getRefIdx( uiPartAddr );           
  TComMv      cMv          = pcCU->getCUMvField( eRefPicList )->getMv( uiPartAddr );
  Bool        bTobeScaled  = false;
  TComPic* pcPicYuvBaseCol = NULL;
  TComPic* pcPicYuvBaseRef = NULL;

#if NH_3D_NBDV
  DisInfo cDistparity;
  cDistparity.m_acNBDV = pcCU->getDvInfo(0).m_acNBDV;
  cDistparity.m_aVIdxCan = pcCU->getDvInfo(uiPartAddr).m_aVIdxCan;
#else
  assert(0); // ARP can be applied only when a DV is available
#endif
  UChar dW = pcCU->getARPW ( uiPartAddr );

    Int arpRefIdx = pcCU->getSlice()->getFirstTRefIdx(eRefPicList);
  if (arpRefIdx < 0 || !pcCU->getSlice()->getArpRefPicAvailable( eRefPicList, cDistparity.m_aVIdxCan))
    {
      dW = 0;
      bTobeScaled = false;
    }
    else
    {
    if( arpRefIdx != iRefIdx )
    {
      bTobeScaled = true;
    }
    pcPicYuvBaseCol =  pcCU->getSlice()->getBaseViewRefPic( pcCU->getSlice()->getPOC(),                              cDistparity.m_aVIdxCan );
    pcPicYuvBaseRef =  pcCU->getSlice()->getBaseViewRefPic( pcCU->getSlice()->getRefPic( eRefPicList, arpRefIdx )->getPOC(), cDistparity.m_aVIdxCan );
    }

    if(bTobeScaled)
    {     
      Int iCurrPOC    = pcCU->getSlice()->getPOC();
      Int iColRefPOC  = pcCU->getSlice()->getRefPOC( eRefPicList, iRefIdx );
      Int iCurrRefPOC = pcCU->getSlice()->getRefPOC( eRefPicList, arpRefIdx );
      Int iScale      = pcCU->xGetDistScaleFactor(iCurrPOC, iCurrRefPOC, iCurrPOC, iColRefPOC);
      if ( iScale != 4096 )
      {
        cMv = cMv.scaleMv( iScale );
      }
      iRefIdx = arpRefIdx;
    }

  pcCU->clipMv(cMv);
  TComPicYuv* pcPicYuvRef = pcCU->getSlice()->getRefPic( eRefPicList, iRefIdx )->getPicYuvRec();

  for (UInt comp=COMPONENT_Y; comp< rpcYuvPred->getNumberValidComponents(); comp++)
  {
    const ComponentID compID=ComponentID(comp);
    xPredInterBlk  ( compID,  pcCU, pcPicYuvRef, uiPartAddr, &cMv, iWidth, iHeight, rpcYuvPred, bi || ( dW > 0 ), pcCU->getSlice()->getSPS()->getBitDepth(toChannelType(compID)), true );
  }

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
    TComMv cNBDV = cDistparity.m_acNBDV;
    pcCU->clipMv( cNBDV );
    
    TComPicYuv* pcPicYuvBaseColRec = pcPicYuvBaseCol->getPicYuvRec();
    TComPicYuv* pcPicYuvBaseRefRec = pcPicYuvBaseRef->getPicYuvRec();
    
    UInt uiCompNum = ( iWidth > 8 ) ? 3: 1;
    for (UInt comp=COMPONENT_Y; comp< uiCompNum; comp++)
    {
      const ComponentID compID=ComponentID(comp);
      xPredInterBlk  ( compID,  pcCU, pcPicYuvBaseColRec, uiPartAddr, &cNBDV, iWidth, iHeight, pYuvB0, true, pcCU->getSlice()->getSPS()->getBitDepth(toChannelType(compID)), true );
      xPredInterBlk  ( compID,  pcCU, pcPicYuvBaseRefRec, uiPartAddr, &cMVwithDisparity, iWidth, iHeight, pYuvB1, true, pcCU->getSlice()->getSPS()->getBitDepth(toChannelType(compID)), true );
    }    
    
    pYuvB0->subtractARP( pYuvB0 , pYuvB1 , uiPartAddr , iWidth , iHeight );

    if( 2 == dW )
    {
      pYuvB0->multiplyARP( uiPartAddr , iWidth , iHeight , dW );
    }
    rpcYuvPred->addARP( rpcYuvPred , pYuvB0 , uiPartAddr , iWidth , iHeight , !bi, pcCU->getSlice()->getSPS()->getBitDepths() );
  }
}

Bool TComPrediction::xCheckBiInterviewARP( TComDataCU* pcCU, UInt uiPartAddr, Int iWidth, Int iHeight, RefPicList eBaseRefPicList, TComPic*& pcPicYuvCurrTRef, TComMv& cBaseTMV, Int& iCurrTRefPoc )
{
  Int         iRefIdx       = pcCU->getCUMvField( eBaseRefPicList )->getRefIdx( uiPartAddr );
  TComMv      cDMv          = pcCU->getCUMvField( eBaseRefPicList )->getMv( uiPartAddr );
  TComPic* pcPicYuvBaseCol  = pcCU->getSlice()->getRefPic( eBaseRefPicList, iRefIdx );  
  Int irefPUX = pcCU->getCUPelX() + g_auiRasterToPelX[g_auiZscanToRaster[uiPartAddr]] + iWidth/2  + ((cDMv.getHor() + 2)>>2);
  Int irefPUY = pcCU->getCUPelY() + g_auiRasterToPelY[g_auiZscanToRaster[uiPartAddr]] + iHeight/2 + ((cDMv.getVer() + 2)>>2);

  irefPUX = (Int)Clip3<Int>(0, pcCU->getSlice()->getSPS()-> getPicWidthInLumaSamples()-1, irefPUX);
  irefPUY = (Int)Clip3<Int>(0, pcCU->getSlice()->getSPS()->getPicHeightInLumaSamples()-1, irefPUY);  

  Int uiLCUAddr,uiAbsPartAddr;
  pcPicYuvBaseCol->getCUAddrAndPartIdx( irefPUX, irefPUY, uiLCUAddr, uiAbsPartAddr);
  TComDataCU *pColCU = pcPicYuvBaseCol->getCtu( uiLCUAddr );

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

//inter-view ARP
Void TComPrediction::xPredInterUniARPviewRef( TComDataCU* pcCU, UInt uiPartAddr, Int iWidth, Int iHeight, RefPicList eRefPicList, TComYuv*& rpcYuvPred, Bool bi )
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

  pcCU->clipMv(cTempDMv);

  Int irefPUX = pcCU->getCUPelX() + g_auiRasterToPelX[g_auiZscanToRaster[uiPartAddr]] + iWidth/2  + ((cDMv.getHor() + 2)>>2);
  Int irefPUY = pcCU->getCUPelY() + g_auiRasterToPelY[g_auiZscanToRaster[uiPartAddr]] + iHeight/2 + ((cDMv.getVer() + 2)>>2);

  irefPUX = (Int)Clip3<Int>(0, pcCU->getSlice()->getSPS()-> getPicWidthInLumaSamples()-1, irefPUX);
  irefPUY = (Int)Clip3<Int>(0, pcCU->getSlice()->getSPS()->getPicHeightInLumaSamples()-1, irefPUY);  
  
  Int uiLCUAddr,uiAbsPartAddr;
  pcPicYuvBaseCol->getCUAddrAndPartIdx( irefPUX, irefPUY, uiLCUAddr, uiAbsPartAddr);
  TComDataCU *pColCU = pcPicYuvBaseCol->getCtu( uiLCUAddr );
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
      if( iRef != -1 && pcCU->getSlice()->getArpRefPicAvailable( eRefPicListCurr, pcPicYuvBaseCol->getViewIndex()))
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

  for (UInt comp=COMPONENT_Y; comp< rpcYuvPred->getNumberValidComponents(); comp++)
  {
    const ComponentID compID=ComponentID(comp);
    xPredInterBlk  ( compID,  pcCU, pcYuvBaseCol, uiPartAddr, &cTempDMv, iWidth, iHeight, rpcYuvPred, bi || ( dW > 0 && bTMVAvai ), pcCU->getSlice()->getSPS()->getBitDepth(toChannelType(compID)), bTMVAvai );
  }

  if( dW > 0 ) 
  {
    assert ( bTMVAvai );
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

    UInt uiCompNum = ( iWidth > 8 ) ? 3: 1;
    for (UInt comp=COMPONENT_Y; comp< uiCompNum; comp++)
    {
      const ComponentID compID=ComponentID(comp);
      xPredInterBlk  ( compID,  pcCU, pcYuvCurrTref, uiPartAddr, &cBaseTMV, iWidth, iHeight, pYuvCurrTRef, true, pcCU->getSlice()->getSPS()->getBitDepth(toChannelType(compID)), true );
      xPredInterBlk  ( compID,  pcCU, pcYuvBaseTref, uiPartAddr, &cTempMv, iWidth, iHeight, pYuvBaseTRef, true, pcCU->getSlice()->getSPS()->getBitDepth(toChannelType(compID)), true );
    }

    pYuvCurrTRef->subtractARP( pYuvCurrTRef , pYuvBaseTRef , uiPartAddr , iWidth , iHeight );  
    if(dW == 2)
    {
      pYuvCurrTRef->multiplyARP( uiPartAddr , iWidth , iHeight , dW );
    }
    rpcYuvPred->addARP( rpcYuvPred , pYuvCurrTRef , uiPartAddr , iWidth , iHeight , !bi, pcCU->getSlice()->getSPS()->getBitDepths() ); 
  }
}
#endif

Void TComPrediction::xPredInterBi ( TComDataCU* pcCU, UInt uiPartAddr, Int iWidth, Int iHeight, TComYuv* pcYuvPred )
{
  TComYuv* pcMbYuv;
  Int      iRefIdx[NUM_REF_PIC_LIST_01] = {-1, -1};

  for ( UInt refList = 0; refList < NUM_REF_PIC_LIST_01; refList++ )
  {
    RefPicList eRefPicList = (refList ? REF_PIC_LIST_1 : REF_PIC_LIST_0);
    iRefIdx[refList] = pcCU->getCUMvField( eRefPicList )->getRefIdx( uiPartAddr );

    if ( iRefIdx[refList] < 0 )
    {
      continue;
    }

    assert( iRefIdx[refList] < pcCU->getSlice()->getNumRefIdx(eRefPicList) );

    pcMbYuv = &m_acYuvPred[refList];
    if( pcCU->getCUMvField( REF_PIC_LIST_0 )->getRefIdx( uiPartAddr ) >= 0 && pcCU->getCUMvField( REF_PIC_LIST_1 )->getRefIdx( uiPartAddr ) >= 0 )
    {
      xPredInterUni ( pcCU, uiPartAddr, iWidth, iHeight, eRefPicList, pcMbYuv, true );
    }
    else
    {
      if ( ( pcCU->getSlice()->getPPS()->getUseWP()       && pcCU->getSlice()->getSliceType() == P_SLICE ) ||
           ( pcCU->getSlice()->getPPS()->getWPBiPred()    && pcCU->getSlice()->getSliceType() == B_SLICE ) )
      {
        xPredInterUni ( pcCU, uiPartAddr, iWidth, iHeight, eRefPicList, pcMbYuv, true );
      }
      else
      {
        xPredInterUni ( pcCU, uiPartAddr, iWidth, iHeight, eRefPicList, pcMbYuv );
      }
    }
  }

  if ( pcCU->getSlice()->getPPS()->getWPBiPred()    && pcCU->getSlice()->getSliceType() == B_SLICE  )
  {
    xWeightedPredictionBi( pcCU, &m_acYuvPred[REF_PIC_LIST_0], &m_acYuvPred[REF_PIC_LIST_1], iRefIdx[REF_PIC_LIST_0], iRefIdx[REF_PIC_LIST_1], uiPartAddr, iWidth, iHeight, pcYuvPred );
  }
  else if ( pcCU->getSlice()->getPPS()->getUseWP() && pcCU->getSlice()->getSliceType() == P_SLICE )
  {
    xWeightedPredictionUni( pcCU, &m_acYuvPred[REF_PIC_LIST_0], uiPartAddr, iWidth, iHeight, REF_PIC_LIST_0, pcYuvPred );
  }
  else
  {
    xWeightedAverage( &m_acYuvPred[REF_PIC_LIST_0], &m_acYuvPred[REF_PIC_LIST_1], iRefIdx[REF_PIC_LIST_0], iRefIdx[REF_PIC_LIST_1], uiPartAddr, iWidth, iHeight, pcYuvPred, pcCU->getSlice()->getSPS()->getBitDepths() );
  }
}

#if NH_3D_VSP
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

  xWeightedAverage( &m_acYuvPred[0], &m_acYuvPred[1], iRefIdx[0], iRefIdx[1], uiPartAddr, iWidth, iHeight, rpcYuvPred, pcCU->getSlice()->getSPS()->getBitDepths() );
}
#endif

/**
 * \brief Generate motion-compensated block
 *
 * \param compID     Colour component ID
 * \param cu         Pointer to current CU
 * \param refPic     Pointer to reference picture
 * \param partAddr   Address of block within CU
 * \param mv         Motion vector
 * \param width      Width of block
 * \param height     Height of block
 * \param dstPic     Pointer to destination picture
 * \param bi         Flag indicating whether bipred is used
 * \param  bitDepth  Bit depth
 */


#if NH_3D
Void TComPrediction::xPredInterBlk(const ComponentID compID, TComDataCU *cu, TComPicYuv *refPic, UInt partAddr, TComMv *mv, Int width, Int height, TComYuv *dstPic, Bool bi, const Int bitDepth 
#if NH_3D_ARP
    , Bool filterType
#endif
#if NH_3D_IC
    , Bool bICFlag
#endif
)
#else
Void TComPrediction::xPredInterBlk(const ComponentID compID, TComDataCU *cu, TComPicYuv *refPic, UInt partAddr, TComMv *mv, Int width, Int height, TComYuv *dstPic, Bool bi, const Int bitDepth )
#endif
{
#if NH_MV
  assert( refPic->getBorderExtension() ); 
#endif
  Int     refStride  = refPic->getStride(compID);
  Int     dstStride  = dstPic->getStride(compID);
  Int shiftHor=(2+refPic->getComponentScaleX(compID));
  Int shiftVer=(2+refPic->getComponentScaleY(compID));

  Int     refOffset  = (mv->getHor() >> shiftHor) + (mv->getVer() >> shiftVer) * refStride;

  Pel*    ref     = refPic->getAddr(compID, cu->getCtuRsAddr(), cu->getZorderIdxInCtu() + partAddr ) + refOffset;

  Pel*    dst = dstPic->getAddr( compID, partAddr );

  Int     xFrac  = mv->getHor() & ((1<<shiftHor)-1);
  Int     yFrac  = mv->getVer() & ((1<<shiftVer)-1);

#if NH_3D_INTEGER_MV_DEPTH
  if( cu->getSlice()->getIsDepth() )
  {
    refOffset = mv->getHor() + mv->getVer() * refStride;
    ref       = refPic->getAddr(compID, cu->getCtuRsAddr(), cu->getZorderIdxInCtu() + partAddr ) + refOffset;
    xFrac     = 0;
    yFrac     = 0;
  }
#endif

  UInt    cxWidth  = width  >> refPic->getComponentScaleX(compID);
  UInt    cxHeight = height >> refPic->getComponentScaleY(compID);

  const ChromaFormat chFmt = cu->getPic()->getChromaFormat();

  if ( yFrac == 0 )
  {
#if NH_3D
    m_if.filterHor(compID, ref, refStride, dst,  dstStride, cxWidth, cxHeight, xFrac, !bi 
#if NH_3D_IC
     || bICFlag
#endif
    , chFmt, bitDepth
#if NH_3D_ARP 
    , filterType
#endif 
);
#else
    m_if.filterHor(compID, ref, refStride, dst,  dstStride, cxWidth, cxHeight, xFrac, !bi, chFmt, bitDepth );
#endif
  }
  else if ( xFrac == 0 )
  {
#if NH_3D
  m_if.filterVer(compID, ref, refStride, dst, dstStride, cxWidth, cxHeight, yFrac, true, !bi 
#if NH_3D_IC
    || bICFlag
#endif
    , chFmt, bitDepth
#if NH_3D_ARP
    , filterType
#endif
);
#else
    m_if.filterVer(compID, ref, refStride, dst, dstStride, cxWidth, cxHeight, yFrac, true, !bi, chFmt, bitDepth );

#endif
  }
  else
  {
    Int   tmpStride = m_filteredBlockTmp[0].getStride(compID);
    Pel*  tmp       = m_filteredBlockTmp[0].getAddr(compID);

    const Int vFilterSize = isLuma(compID) ? NTAPS_LUMA : NTAPS_CHROMA;

#if NH_3D_ARP 
    m_if.filterHor(compID, ref - ((vFilterSize>>1) -1)*refStride, refStride, tmp, tmpStride, cxWidth, cxHeight+vFilterSize-1, xFrac, false,      chFmt, bitDepth, filterType );
#else
    m_if.filterHor(compID, ref - ((vFilterSize>>1) -1)*refStride, refStride, tmp, tmpStride, cxWidth, cxHeight+vFilterSize-1, xFrac, false,      chFmt, bitDepth );
#endif 

#if NH_3D
    m_if.filterVer(compID, tmp + ((vFilterSize>>1) -1)*tmpStride, tmpStride, dst, dstStride, cxWidth, cxHeight,               yFrac, false, !bi
#if NH_3D_IC
    || bICFlag
#endif
    , chFmt, bitDepth
#if NH_3D_ARP
    , filterType
#endif 
);
#else
    m_if.filterVer(compID, tmp + ((vFilterSize>>1) -1)*tmpStride, tmpStride, dst, dstStride, cxWidth, cxHeight,               yFrac, false, !bi, chFmt, bitDepth );
#endif
  }

#if NH_3D_IC
  if( bICFlag )
  {
    Int a, b, i, j;
    const Int iShift = IC_CONST_SHIFT;
    Pel *dst2 = dst;

    xGetLLSICPrediction( compID, cu, mv, refPic, a, b, bitDepth );

    for ( i = 0; i < cxHeight; i++ )
    {
      for ( j = 0; j < cxWidth; j++ )
      {
        dst[j] = Clip3( 0, ( 1 << bitDepth ) - 1, ( ( a*dst[j] ) >> iShift ) + b );
      }
      dst += dstStride;
    }

    if(bi)
    {
      Int shift = IF_INTERNAL_PREC - bitDepth;
      for (i = 0; i < cxHeight; i++)
      {
        for (j = 0; j < cxWidth; j++)
        {
          Pel val = dst2[j] << shift;
          dst2[j] = val - (Pel)IF_INTERNAL_OFFS;
        }
        dst2 += dstStride;
      }
    }
  }
#endif

}

Void TComPrediction::xWeightedAverage( TComYuv* pcYuvSrc0, TComYuv* pcYuvSrc1, Int iRefIdx0, Int iRefIdx1, UInt uiPartIdx, Int iWidth, Int iHeight, TComYuv* pcYuvDst, const BitDepths &clipBitDepths )
{
  if( iRefIdx0 >= 0 && iRefIdx1 >= 0 )
  {
    pcYuvDst->addAvg( pcYuvSrc0, pcYuvSrc1, uiPartIdx, iWidth, iHeight, clipBitDepths );
  }
  else if ( iRefIdx0 >= 0 && iRefIdx1 <  0 )
  {
    pcYuvSrc0->copyPartToPartYuv( pcYuvDst, uiPartIdx, iWidth, iHeight );
  }
  else if ( iRefIdx0 <  0 && iRefIdx1 >= 0 )
  {
    pcYuvSrc1->copyPartToPartYuv( pcYuvDst, uiPartIdx, iWidth, iHeight );
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
 * \param pSrc        pointer to reconstructed sample array
 * \param srcStride   the stride of the reconstructed sample array
 * \param rpDst       reference to pointer for the prediction sample array
 * \param dstStride   the stride of the prediction sample array
 * \param width       the width of the block
 * \param height      the height of the block
 * \param channelType type of pel array (luma, chroma)
 * \param format      chroma format
 *
 * This function derives the prediction samples for planar mode (intra coding).
 */
//NOTE: Bit-Limit - 24-bit source
Void TComPrediction::xPredIntraPlanar( const Pel* pSrc, Int srcStride, Pel* rpDst, Int dstStride, UInt width, UInt height )
{
  assert(width <= height);

  Int leftColumn[MAX_CU_SIZE+1], topRow[MAX_CU_SIZE+1], bottomRow[MAX_CU_SIZE], rightColumn[MAX_CU_SIZE];
  UInt shift1Dhor = g_aucConvertToBit[ width ] + 2;
  UInt shift1Dver = g_aucConvertToBit[ height ] + 2;

  // Get left and above reference column and row
  for(Int k=0;k<width+1;k++)
  {
    topRow[k] = pSrc[k-srcStride];
  }

  for (Int k=0; k < height+1; k++)
  {
    leftColumn[k] = pSrc[k*srcStride-1];
  }

  // Prepare intermediate variables used in interpolation
  Int bottomLeft = leftColumn[height];
  Int topRight   = topRow[width];

  for(Int k=0;k<width;k++)
  {
    bottomRow[k]  = bottomLeft - topRow[k];
    topRow[k]     <<= shift1Dver;
  }

  for(Int k=0;k<height;k++)
  {
    rightColumn[k]  = topRight - leftColumn[k];
    leftColumn[k]   <<= shift1Dhor;
  }

  const UInt topRowShift = 0;

  // Generate prediction signal
  for (Int y=0;y<height;y++)
  {
    Int horPred = leftColumn[y] + width;
    for (Int x=0;x<width;x++)
    {
      horPred += rightColumn[y];
      topRow[x] += bottomRow[x];

      Int vertPred = ((topRow[x] + topRowShift)>>topRowShift);
      rpDst[y*dstStride+x] = ( horPred + vertPred ) >> (shift1Dhor+1);
    }
  }
}

/** Function for filtering intra DC predictor.
 * \param pSrc pointer to reconstructed sample array
 * \param iSrcStride the stride of the reconstructed sample array
 * \param pDst reference to pointer for the prediction sample array
 * \param iDstStride the stride of the prediction sample array
 * \param iWidth the width of the block
 * \param iHeight the height of the block
 * \param channelType type of pel array (luma, chroma)
 *
 * This function performs filtering left and top edges of the prediction samples for DC mode (intra coding).
 */
Void TComPrediction::xDCPredFiltering( const Pel* pSrc, Int iSrcStride, Pel* pDst, Int iDstStride, Int iWidth, Int iHeight, ChannelType channelType )
{
  Int x, y, iDstStride2, iSrcStride2;

  if (isLuma(channelType) && (iWidth <= MAXIMUM_INTRA_FILTERED_WIDTH) && (iHeight <= MAXIMUM_INTRA_FILTERED_HEIGHT))
  {
    //top-left
    pDst[0] = (Pel)((pSrc[-iSrcStride] + pSrc[-1] + 2 * pDst[0] + 2) >> 2);

    //top row (vertical filter)
    for ( x = 1; x < iWidth; x++ )
    {
      pDst[x] = (Pel)((pSrc[x - iSrcStride] +  3 * pDst[x] + 2) >> 2);
    }

    //left column (horizontal filter)
    for ( y = 1, iDstStride2 = iDstStride, iSrcStride2 = iSrcStride-1; y < iHeight; y++, iDstStride2+=iDstStride, iSrcStride2+=iSrcStride )
    {
      pDst[iDstStride2] = (Pel)((pSrc[iSrcStride2] + 3 * pDst[iDstStride2] + 2) >> 2);
    }
  }

  return;
}

/* Static member function */
Bool TComPrediction::UseDPCMForFirstPassIntraEstimation(TComTU &rTu, const UInt uiDirMode)
{
  return (rTu.getCU()->isRDPCMEnabled(rTu.GetAbsPartIdxTU()) ) &&
          rTu.getCU()->getCUTransquantBypass(rTu.GetAbsPartIdxTU()) &&
          (uiDirMode==HOR_IDX || uiDirMode==VER_IDX);
}
#if NH_3D_IC
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
Void TComPrediction::xGetLLSICPrediction( const ComponentID compID, TComDataCU* pcCU, TComMv *pMv, TComPicYuv *pRefPic, Int &a, Int &b, const Int bitDepth )
{
  TComPicYuv *pRecPic = pcCU->getPic()->getPicYuvRec();
  Pel *pRec = NULL, *pRef = NULL;
  UInt uiWidth, uiHeight, uiTmpPartIdx;
  Int iRecStride = pRecPic->getStride(compID);
  Int iRefStride = pRefPic->getStride(compID);
  Int iRefOffset, iHor, iVer;
  iHor = pcCU->getSlice()->getIsDepth() ? pMv->getHor() : ( ( pMv->getHor() + 2 ) >> 2 );
  iVer = pcCU->getSlice()->getIsDepth() ? pMv->getVer() : ( ( pMv->getVer() + 2 ) >> 2 );
  if( !isLuma(compID) )
  {
    iHor = pcCU->getSlice()->getIsDepth() ? ( ( pMv->getHor() + 1 ) >> 1 ) : ( ( pMv->getHor() + 4 ) >> 3 );
    iVer = pcCU->getSlice()->getIsDepth() ? ( ( pMv->getVer() + 1 ) >> 1 ) : ( ( pMv->getVer() + 4 ) >> 3 );
  }
  uiWidth  = pcCU->getWidth( 0 ) >> pRefPic->getComponentScaleX(compID);
  uiHeight = pcCU->getHeight( 0 ) >> pRefPic->getComponentScaleY(compID);

  Int i, j, iCountShift = 0;

  // LLS parameters estimation

  Int x = 0, y = 0, xx = 0, xy = 0;
  Int precShift = std::max(0, bitDepth - 12);

  UInt partAddr = 0;
  if( pcCU->getPUAbove( uiTmpPartIdx, pcCU->getZorderIdxInCtu() ) )
  {
    iRefOffset = iHor + iVer * iRefStride - iRefStride;
    pRef = pRefPic->getAddr(compID, pcCU->getCtuRsAddr(), pcCU->getZorderIdxInCtu() + partAddr ) + iRefOffset;
    pRec = pRecPic->getAddr(compID, pcCU->getCtuRsAddr(), pcCU->getZorderIdxInCtu() + partAddr ) - iRecStride;
    for( j = 0; j < uiWidth; j+=2 )
    {
      x += pRef[j];
      y += pRec[j];
      if( isLuma(compID) )
      {
        xx += (pRef[j] * pRef[j])>>precShift;
        xy += (pRef[j] * pRec[j])>>precShift;
      }
    }
    iCountShift += g_aucConvertToBit[ uiWidth ] + 1;
  }

  if( pcCU->getPULeft( uiTmpPartIdx, pcCU->getZorderIdxInCtu() ) )
  {
    iRefOffset = iHor + iVer * iRefStride - 1;
    pRef = pRefPic->getAddr(compID, pcCU->getCtuRsAddr(), pcCU->getZorderIdxInCtu() + partAddr ) + iRefOffset;
    pRec = pRecPic->getAddr(compID, pcCU->getCtuRsAddr(), pcCU->getZorderIdxInCtu() + partAddr ) - 1;
    for( i = 0; i < uiHeight; i+=2 )
    {
      x += pRef[0];
      y += pRec[0];
      if( isLuma(compID) )
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

  if( !isLuma(compID) )
  {
    a = ( 1 << IC_CONST_SHIFT );
    b = (  y - x + ( 1 << ( iCountShift - 1 ) ) ) >> iCountShift;
  }
  else
  {
  xy += xx >> IC_REG_COST_SHIFT;
  xx += xx >> IC_REG_COST_SHIFT;
  Int a1 = ( xy << iCountShift ) - ((y * x) >> precShift);
  Int a2 = ( xx << iCountShift ) - ((x * x) >> precShift);
  const Int iShift = IC_CONST_SHIFT;
      const Int iShiftA2 = 6;
      const Int iAccuracyShift = 15;

      Int iScaleShiftA2 = 0;
      Int iScaleShiftA1 = 0;
    Int a1s;
    Int a2s;

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
#endif

#if NH_3D_SDC_INTRA
Void TComPrediction::predConstantSDC( Pel* ptrSrc, UInt srcStride, UInt uiSize, Pel& predDC )
{
  Pel* pLeftTop     =  ptrSrc;
  Pel* pRightTop    =  ptrSrc                          + (uiSize-1);
  Pel* pLeftBottom  = (ptrSrc+ (srcStride*(uiSize-1))              );
  Pel* pRightBottom = (ptrSrc+ (srcStride*(uiSize-1))  + (uiSize-1));
  predDC = (*pLeftTop + *pRightTop + *pLeftBottom + *pRightBottom + 2)>>2;
}
#endif // NH_3D_SDC_INTRA

#if NH_3D_DMM
Void TComPrediction::predContourFromTex( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiWidth, UInt uiHeight, Bool* segPattern )
{
  // get copy of co-located texture luma block
  TComYuv cTempYuv;
  cTempYuv.create( uiWidth, uiHeight, CHROMA_400 ); 
  cTempYuv.clear();
  Pel* piRefBlkY = cTempYuv.getAddr( COMPONENT_Y );
  TComPicYuv* pcPicYuvRef = pcCU->getSlice()->getTexturePic()->getPicYuvRec();
  assert( pcPicYuvRef != NULL );
  Int  iRefStride = pcPicYuvRef->getStride( COMPONENT_Y );
  Pel* piRefY     = pcPicYuvRef->getAddr  ( COMPONENT_Y, pcCU->getCtuRsAddr(), pcCU->getZorderIdxInCtu() + uiAbsPartIdx );

  for( Int y = 0; y < uiHeight; y++ )
  {
    ::memcpy(piRefBlkY, piRefY, sizeof(Pel)*uiWidth);
    piRefBlkY += uiWidth;
    piRefY += iRefStride;
  }


  // find contour for texture luma block
  piRefBlkY = cTempYuv.getAddr( COMPONENT_Y );
  UInt iDC = 0;
  iDC  = piRefBlkY[ 0 ];
  iDC += piRefBlkY[ uiWidth - 1 ];
  iDC += piRefBlkY[ uiWidth * (uiHeight - 1) ];
  iDC += piRefBlkY[ uiWidth * (uiHeight - 1) + uiWidth - 1 ];
  iDC = iDC >> 2;

  piRefBlkY = cTempYuv.getAddr( COMPONENT_Y );
  for( UInt k = 0; k < (uiWidth*uiHeight); k++ ) 
  { 
    segPattern[k] = (piRefBlkY[k] > iDC) ? true : false;
  }

  cTempYuv.destroy();
}

Void TComPrediction::predBiSegDCs( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiWidth, UInt uiHeight, Bool* biSegPattern, Int patternStride, Pel& predDC1, Pel& predDC2 )
{
  assert( biSegPattern );
  const Pel *piMask = getPredictorPtr( COMPONENT_Y, false );
  assert( piMask );
  Int srcStride = 2*uiWidth + 1;
  const Pel *ptrSrc = piMask+srcStride+1;

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

Void TComPrediction::assignBiSegDCs( Pel* ptrDst, UInt dstStride, Bool* biSegPattern, Int patternStride, Pel valDC1, Pel valDC2 )
{
  assert( biSegPattern );
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
#endif
//! \}
