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

/** \file     TComPic.cpp
    \brief    picture class
*/

#include "TComPic.h"
#include "SEI.h"

//! \ingroup TLibCommon
//! \{

// ====================================================================================================================
// Constructor / destructor / create / destroy
// ====================================================================================================================

TComPic::TComPic()
{
  m_uiTLayer          = 0;

  m_apcPicSym         = NULL;
  m_apcPicYuv[0]      = NULL;
  m_apcPicYuv[1]      = NULL;
#if DEPTH_MAP_GENERATION
  m_pcPredDepthMap    = NULL;
#if PDM_REMOVE_DEPENDENCE
  m_pcPredDepthMap_temp    = NULL;
#endif
#endif
#if FCO_DVP_REFINE_C0132_C0170
  m_bDepthCoded       = false;
  m_pcRecDepthMap     = NULL;
#endif
#if H3D_IVMP
  m_pcOrgDepthMap     = NULL;
#endif
#if H3D_IVRP
  m_pcResidual        = NULL;
#endif
  m_pcPicYuvPred      = NULL;
  m_pcPicYuvResi      = NULL;
  m_bIsLongTerm       = false;
  m_bReconstructed    = false;
  m_usedForTMVP       = true;
  m_bNeededForOutput  = false;
  m_pSliceSUMap       = NULL;
  m_uiCurrSliceIdx    = 0; 
#if HHI_INTERVIEW_SKIP
  m_pcUsedPelsMap     = NULL;
#endif
#if INTER_VIEW_VECTOR_SCALING_C0115
  m_iViewOrderIdx     = 0;    // will be changed to view_id
#endif
  m_aaiCodedScale     = 0;
  m_aaiCodedOffset    = 0;
#if H3D_QTL
  m_bReduceBitsQTL    = 0;
#endif
#if H3D_NBDV
  m_bRapCheck = false;
  m_eRapRefList = REF_PIC_LIST_0;
  m_uiRapRefIdx = 0;
#endif

}

TComPic::~TComPic()
{
}

Void TComPic::create( Int iWidth, Int iHeight, UInt uiMaxWidth, UInt uiMaxHeight, UInt uiMaxDepth, Bool bIsVirtual )
{
  m_apcPicSym     = new TComPicSym;  m_apcPicSym   ->create( iWidth, iHeight, uiMaxWidth, uiMaxHeight, uiMaxDepth );
  if (!bIsVirtual)
  {
    m_apcPicYuv[0]  = new TComPicYuv;  m_apcPicYuv[0]->create( iWidth, iHeight, uiMaxWidth, uiMaxHeight, uiMaxDepth );
  }
  m_apcPicYuv[1]  = new TComPicYuv;  m_apcPicYuv[1]->create( iWidth, iHeight, uiMaxWidth, uiMaxHeight, uiMaxDepth );
  
  /* there are no SEI messages associated with this picture initially */
  m_SEIs = NULL;
  m_bUsedByCurr = false;
  return;
}

Void TComPic::destroy()
{
  if (m_apcPicSym)
  {
    m_apcPicSym->destroy();
    delete m_apcPicSym;
    m_apcPicSym = NULL;
  }
  
  if (m_apcPicYuv[0])
  {
    m_apcPicYuv[0]->destroy();
    delete m_apcPicYuv[0];
    m_apcPicYuv[0]  = NULL;
  }
  
  if (m_apcPicYuv[1])
  {
    m_apcPicYuv[1]->destroy();
    delete m_apcPicYuv[1];
    m_apcPicYuv[1]  = NULL;
  }
#if HHI_INTERVIEW_SKIP
  if( m_pcUsedPelsMap )
  {
    m_pcUsedPelsMap->destroy();
    delete m_pcUsedPelsMap;
    m_pcUsedPelsMap = NULL;
  }
#endif
#if DEPTH_MAP_GENERATION
  if( m_pcPredDepthMap )
  {
    m_pcPredDepthMap->destroy();
    delete m_pcPredDepthMap;
    m_pcPredDepthMap = NULL;
  }
#if PDM_REMOVE_DEPENDENCE
  if( m_pcPredDepthMap_temp )         //  estimated depth map
  {
    m_pcPredDepthMap_temp->destroy();
    delete m_pcPredDepthMap_temp;
    m_pcPredDepthMap_temp = NULL;
  }                     
#endif
#endif
#if H3D_IVMP
  if( m_pcOrgDepthMap )
  {
    m_pcOrgDepthMap->destroy();
    delete m_pcOrgDepthMap;
    m_pcOrgDepthMap = NULL;
  }
#endif
#if H3D_IVRP
  if( m_pcResidual )
  {
    m_pcResidual->destroy();
    delete m_pcResidual;
    m_pcResidual = NULL;
  }
#endif
  delete m_SEIs;
}

Void TComPic::compressMotion()
{
  TComPicSym* pPicSym = getPicSym(); 
  for ( UInt uiCUAddr = 0; uiCUAddr < pPicSym->getFrameHeightInCU()*pPicSym->getFrameWidthInCU(); uiCUAddr++ )
  {
    TComDataCU* pcCU = pPicSym->getCU(uiCUAddr);
    pcCU->compressMV(); 
  } 
}

#if DEPTH_MAP_GENERATION
Void
TComPic::addPrdDepthMapBuffer( UInt uiSubSampExpX, UInt uiSubSampExpY )
{
  AOT( m_pcPredDepthMap );
  AOF( m_apcPicYuv[1]   );
  Int   iWidth        = m_apcPicYuv[1]->getWidth      ();
  Int   iHeight       = m_apcPicYuv[1]->getHeight     ();
  UInt  uiMaxCuWidth  = m_apcPicYuv[1]->getMaxCuWidth ();
  UInt  uiMaxCuHeight = m_apcPicYuv[1]->getMaxCuHeight();
  UInt  uiMaxCuDepth  = m_apcPicYuv[1]->getMaxCuDepth ();
  m_pcPredDepthMap    = new TComPicYuv;
  m_pcPredDepthMap    ->create( iWidth >> uiSubSampExpX, iHeight >> uiSubSampExpY, uiMaxCuWidth >> uiSubSampExpX, uiMaxCuHeight >> uiSubSampExpY, uiMaxCuDepth );
#if PDM_REMOVE_DEPENDENCE
  m_pcPredDepthMap_temp    = new TComPicYuv;
  m_pcPredDepthMap_temp    ->create( iWidth >> uiSubSampExpX, iHeight >> uiSubSampExpY, uiMaxCuWidth >> uiSubSampExpX, uiMaxCuHeight >> uiSubSampExpY, uiMaxCuDepth );
#endif
}
#endif

#if H3D_IVMP
Void
TComPic::addOrgDepthMapBuffer()
{
  AOT( m_pcOrgDepthMap );
  AOF( m_apcPicYuv[1]  );
  Int   iWidth        = m_apcPicYuv[1]->getWidth      ();
  Int   iHeight       = m_apcPicYuv[1]->getHeight     ();
  UInt  uiMaxCuWidth  = m_apcPicYuv[1]->getMaxCuWidth ();
  UInt  uiMaxCuHeight = m_apcPicYuv[1]->getMaxCuHeight();
  UInt  uiMaxCuDepth  = m_apcPicYuv[1]->getMaxCuDepth ();
  m_pcOrgDepthMap     = new TComPicYuv;
  m_pcOrgDepthMap     ->create( iWidth, iHeight, uiMaxCuWidth, uiMaxCuHeight, uiMaxCuDepth );
}
#endif

#if H3D_IVRP
Void
TComPic::addResidualBuffer()
{
  AOT( m_pcResidual   );
  AOF( m_apcPicYuv[1] );
  Int   iWidth        = m_apcPicYuv[1]->getWidth      ();
  Int   iHeight       = m_apcPicYuv[1]->getHeight     ();
  UInt  uiMaxCuWidth  = m_apcPicYuv[1]->getMaxCuWidth ();
  UInt  uiMaxCuHeight = m_apcPicYuv[1]->getMaxCuHeight();
  UInt  uiMaxCuDepth  = m_apcPicYuv[1]->getMaxCuDepth ();
  m_pcResidual        = new TComPicYuv;
  m_pcResidual        ->create( iWidth, iHeight, uiMaxCuWidth, uiMaxCuHeight, uiMaxCuDepth );
}
#endif

#if DEPTH_MAP_GENERATION
Void
TComPic::removePrdDepthMapBuffer()
{
  if( m_pcPredDepthMap )
  {
    m_pcPredDepthMap->destroy();
    delete m_pcPredDepthMap;
    m_pcPredDepthMap = NULL;
  }
#if PDM_REMOVE_DEPENDENCE
  if(m_pcPredDepthMap_temp)
  {
    m_pcPredDepthMap_temp->destroy();
    delete m_pcPredDepthMap_temp;
    m_pcPredDepthMap_temp = NULL;
  }
#endif
}
#endif

#if H3D_IVMP
Void
TComPic::removeOrgDepthMapBuffer()
{
  if( m_pcOrgDepthMap )
  {
    m_pcOrgDepthMap->destroy();
    delete m_pcOrgDepthMap;
    m_pcOrgDepthMap = NULL;
  }
}
#endif

#if H3D_IVRP
Void
TComPic::removeResidualBuffer()
{
  if( m_pcResidual )
  {
    m_pcResidual->destroy();
    delete m_pcResidual;
    m_pcResidual = NULL;
  }
}
#endif

/** Create non-deblocked filter information
 * \param pSliceStartAddress array for storing slice start addresses
 * \param numSlices number of slices in picture
 * \param sliceGranularityDepth slice granularity 
 * \param bNDBFilterCrossSliceBoundary cross-slice-boundary in-loop filtering; true for "cross".
 * \param numTiles number of tiles in picture
 * \param bNDBFilterCrossTileBoundary cross-tile-boundary in-loop filtering; true for "cross".
 */
Void TComPic::createNonDBFilterInfo(UInt* pSliceStartAddress, Int numSlices, Int sliceGranularityDepth
                                    ,Bool bNDBFilterCrossSliceBoundary
                                    ,Int numTiles
                                    ,Bool bNDBFilterCrossTileBoundary)
{
  UInt maxNumSUInLCU = getNumPartInCU();
  UInt numLCUInPic   = getNumCUsInFrame();
  UInt picWidth      = getSlice(0)->getSPS()->getPicWidthInLumaSamples();
  UInt picHeight     = getSlice(0)->getSPS()->getPicHeightInLumaSamples();
  Int  numLCUsInPicWidth = getFrameWidthInCU();
  Int  numLCUsInPicHeight= getFrameHeightInCU();
  UInt maxNumSUInLCUWidth = getNumPartInWidth();
  UInt maxNumSUInLCUHeight= getNumPartInHeight();

  m_bIndependentSliceBoundaryForNDBFilter = (bNDBFilterCrossSliceBoundary)?(false):((numSlices > 1)?(true):(false)) ;
  m_sliceGranularityForNDBFilter = sliceGranularityDepth;
  m_bIndependentTileBoundaryForNDBFilter  = (bNDBFilterCrossTileBoundary)?(false) :((numTiles > 1)?(true):(false));

  m_pbValidSlice = new Bool[numSlices];
  for(Int s=0; s< numSlices; s++)
  {
    m_pbValidSlice[s] = true;
  }
  m_pSliceSUMap = new Int[maxNumSUInLCU * numLCUInPic];

  //initialization
  for(UInt i=0; i< (maxNumSUInLCU * numLCUInPic); i++ )
  {
    m_pSliceSUMap[i] = -1;
  }
  for( UInt CUAddr = 0; CUAddr < numLCUInPic ; CUAddr++ )
  {
    TComDataCU* pcCU = getCU( CUAddr );
    pcCU->setSliceSUMap(m_pSliceSUMap + (CUAddr* maxNumSUInLCU)); 
    pcCU->getNDBFilterBlocks()->clear();
  }
  m_vSliceCUDataLink.clear();

  m_vSliceCUDataLink.resize(numSlices);

  UInt startAddr, endAddr, firstCUInStartLCU, startLCU, endLCU, lastCUInEndLCU, uiAddr;
  UInt LPelX, TPelY, LCUX, LCUY;
  UInt currSU;
  UInt startSU, endSU;

  for(Int s=0; s< numSlices; s++)
  {
    //1st step: decide the real start address
    startAddr = pSliceStartAddress[s];
    endAddr   = pSliceStartAddress[s+1] -1;

    startLCU            = startAddr / maxNumSUInLCU;
    firstCUInStartLCU   = startAddr % maxNumSUInLCU;

    endLCU              = endAddr   / maxNumSUInLCU;
    lastCUInEndLCU      = endAddr   % maxNumSUInLCU;   

    uiAddr = m_apcPicSym->getCUOrderMap(startLCU);

    LCUX      = getCU(uiAddr)->getCUPelX();
    LCUY      = getCU(uiAddr)->getCUPelY();
    LPelX     = LCUX + g_auiRasterToPelX[ g_auiZscanToRaster[firstCUInStartLCU] ];
    TPelY     = LCUY + g_auiRasterToPelY[ g_auiZscanToRaster[firstCUInStartLCU] ];
    currSU    = firstCUInStartLCU;

    Bool bMoveToNextLCU = false;
    Bool bSliceInOneLCU = (startLCU == endLCU);

    while(!( LPelX < picWidth ) || !( TPelY < picHeight ))
    {
      currSU ++;

      if(bSliceInOneLCU)
      {
        if(currSU > lastCUInEndLCU)
        {
          m_pbValidSlice[s] = false;
          break;
        }
      }

      if(currSU >= maxNumSUInLCU )
      {
        bMoveToNextLCU = true;
        break;
      }

      LPelX = LCUX + g_auiRasterToPelX[ g_auiZscanToRaster[currSU] ];
      TPelY = LCUY + g_auiRasterToPelY[ g_auiZscanToRaster[currSU] ];

    }


    if(!m_pbValidSlice[s])
    {
      continue;
    }

    if(currSU != firstCUInStartLCU)
    {
      if(!bMoveToNextLCU)
      {
        firstCUInStartLCU = currSU;
      }
      else
      {
        startLCU++;
        firstCUInStartLCU = 0;
        assert( startLCU < getNumCUsInFrame());
      }
      assert(startLCU*maxNumSUInLCU + firstCUInStartLCU < endAddr);
    }


    //2nd step: assign NonDBFilterInfo to each processing block
    for(UInt i= startLCU; i <= endLCU; i++)
    {
      startSU = (i == startLCU)?(firstCUInStartLCU):(0);
      endSU   = (i == endLCU  )?(lastCUInEndLCU   ):(maxNumSUInLCU -1);

      uiAddr = m_apcPicSym->getCUOrderMap(i);
      Int iTileID= m_apcPicSym->getTileIdxMap(uiAddr);

      TComDataCU* pcCU = getCU(uiAddr);
      m_vSliceCUDataLink[s].push_back(pcCU);

      createNonDBFilterInfoLCU(iTileID, s, pcCU, startSU, endSU, m_sliceGranularityForNDBFilter, picWidth, picHeight);
    }
  }

  //step 3: border availability
  for(Int s=0; s< numSlices; s++)
  {
    if(!m_pbValidSlice[s])
    {
      continue;
    }

    for(Int i=0; i< m_vSliceCUDataLink[s].size(); i++)
    {
      TComDataCU* pcCU = m_vSliceCUDataLink[s][i];
      uiAddr = pcCU->getAddr();

      if(pcCU->getPic()==0)
      {
        continue;
      }
      Int iTileID= m_apcPicSym->getTileIdxMap(uiAddr);
      Bool bTopTileBoundary = false, bDownTileBoundary= false, bLeftTileBoundary= false, bRightTileBoundary= false;

      if(m_bIndependentTileBoundaryForNDBFilter)
      {
        //left
        if( uiAddr % numLCUsInPicWidth != 0)
        {
          bLeftTileBoundary = ( m_apcPicSym->getTileIdxMap(uiAddr -1) != iTileID )?true:false;
        }
        //right
        if( (uiAddr % numLCUsInPicWidth) != (numLCUsInPicWidth -1) )
        {
          bRightTileBoundary = ( m_apcPicSym->getTileIdxMap(uiAddr +1) != iTileID)?true:false;
        }
        //top
        if( uiAddr >= numLCUsInPicWidth)
        {
          bTopTileBoundary = (m_apcPicSym->getTileIdxMap(uiAddr - numLCUsInPicWidth) !=  iTileID )?true:false;
        }
        //down
        if( uiAddr + numLCUsInPicWidth < numLCUInPic )
        {
          bDownTileBoundary = (m_apcPicSym->getTileIdxMap(uiAddr + numLCUsInPicWidth) != iTileID)?true:false;
        }

      }

      pcCU->setNDBFilterBlockBorderAvailability(numLCUsInPicWidth, numLCUsInPicHeight, maxNumSUInLCUWidth, maxNumSUInLCUHeight,picWidth, picHeight
        ,m_bIndependentSliceBoundaryForNDBFilter
        ,bTopTileBoundary, bDownTileBoundary, bLeftTileBoundary, bRightTileBoundary
        ,m_bIndependentTileBoundaryForNDBFilter);

    }

  }

  if( m_bIndependentSliceBoundaryForNDBFilter || m_bIndependentTileBoundaryForNDBFilter)
  {
    m_pNDBFilterYuvTmp = new TComPicYuv();
    m_pNDBFilterYuvTmp->create(picWidth, picHeight, g_uiMaxCUWidth, g_uiMaxCUHeight, g_uiMaxCUDepth);
  }
}
#if H3D_NBDV
Bool TComPic::getDisCandRefPictures(Int iColPOC)
{
  UInt uiTempLayerCurr=7;
  TComSlice* currSlice = getCurrSlice();
  UInt iPOCCurr=currSlice->getPOC();
  UInt iPOCDiff = 255;
  Bool  bRAP=false;
  Bool bCheck = false;
  Int MaxRef = currSlice->getNumRefIdx(RefPicList(0));
  RefPicList eRefPicList = REF_PIC_LIST_0 ;
  if(currSlice->isInterB())
  {
    if(currSlice->getNumRefIdx(RefPicList(0))< currSlice->getNumRefIdx(RefPicList(1)))
      MaxRef = currSlice->getNumRefIdx(RefPicList(1));
  }
  for(Int lpRef = 0; lpRef < MaxRef; lpRef++)
  {
    for(Int lpNr = 0; lpNr < (currSlice->isInterB() ? 2: 1); lpNr ++)
    {
      eRefPicList = RefPicList(0);
      if(currSlice->isInterB())
        eRefPicList = RefPicList(lpNr==0 ? (currSlice->getColDir()): (1-currSlice->getColDir()));
      if(iColPOC == currSlice->getRefPOC(eRefPicList, lpRef))
        continue;
      if(lpRef >= currSlice->getNumRefIdx(eRefPicList)||(currSlice->getViewId() != currSlice->getRefPic( eRefPicList, lpRef)->getViewId()))
        continue;
      Int iTempPoc = currSlice->getRefPic(eRefPicList, lpRef)->getPOC();
      UInt uiTempLayer = currSlice->getRefPic(eRefPicList, lpRef)->getCurrSlice()->getTLayer(); 
      Int iTempDiff = (iTempPoc > iPOCCurr) ? (iTempPoc - iPOCCurr): (iPOCCurr - iTempPoc);
#if QC_REM_IDV_B0046
      TComSlice* refSlice = currSlice->getRefPic(eRefPicList, lpRef)->getCurrSlice();
      bRAP = (refSlice->getSPS()->getViewId() && (refSlice->getNalUnitType() == NAL_UNIT_CODED_SLICE_IDR || refSlice->getNalUnitType() == NAL_UNIT_CODED_SLICE_CRA))? 1: 0;      
#else
      bRAP = (currSlice->getRefPic(eRefPicList, lpRef)->getCurrSlice()->getNalUnitType() == NAL_UNIT_CODED_SLICE_IDV? 1:0);
#endif
      if( bRAP)
      {
         bCheck = true;
         this->setRapRefIdx(lpRef);
         this->setRapRefList(eRefPicList);
         return bCheck;
      }
      if(uiTempLayerCurr > uiTempLayer)
      {
        bCheck = true;
        if(uiTempLayerCurr == uiTempLayer)
        {
          if(iPOCDiff > iTempDiff)
          {
            iPOCDiff=iTempDiff;
            if(iPOCDiff < 255)
            {
              this->setRapRefIdx(lpRef);
              this->setRapRefList(eRefPicList);
            }
          }
        }
        else
        {
          iPOCDiff=iTempDiff;
          uiTempLayerCurr = uiTempLayer;
          this->setRapRefIdx(lpRef);
          this->setRapRefList(eRefPicList);
        } 
      }
    }
  }
  return bCheck;
}
#endif

#if HHI_INTERVIEW_SKIP
Void TComPic::addUsedPelsMapBuffer()
{
  AOT( m_pcUsedPelsMap );
  AOF( m_apcPicYuv[1]  );
  Int   iWidth        = m_apcPicYuv[1]->getWidth      ();
  Int   iHeight       = m_apcPicYuv[1]->getHeight     ();
  UInt  uiMaxCuWidth  = m_apcPicSym->getMaxCUWidth();
  UInt  uiMaxCuHeight = m_apcPicSym->getMaxCUHeight();
  UInt  uiMaxCuDepth  = m_apcPicSym->getMaxCUDepth ();
  m_pcUsedPelsMap     = new TComPicYuv;
  m_pcUsedPelsMap     ->create( iWidth, iHeight, uiMaxCuWidth, uiMaxCuHeight, uiMaxCuDepth );

}
Void
TComPic::removeUsedPelsMapBuffer()
{
  if( m_pcUsedPelsMap )
  {
    m_pcUsedPelsMap->destroy();
    delete m_pcUsedPelsMap;
    m_pcUsedPelsMap = NULL;
  }
}
#endif

/** Create non-deblocked filter information for LCU
 * \param tileID tile index
 * \param sliceID slice index
 * \param pcCU CU data pointer
 * \param startSU start SU index in LCU
 * \param endSU end SU index in LCU
 * \param sliceGranularyDepth slice granularity
 * \param picWidth picture width
 * \param picHeight picture height
 */
Void TComPic::createNonDBFilterInfoLCU(Int tileID, Int sliceID, TComDataCU* pcCU, UInt startSU, UInt endSU, Int sliceGranularyDepth, UInt picWidth, UInt picHeight)
{
  UInt LCUX          = pcCU->getCUPelX();
  UInt LCUY          = pcCU->getCUPelY();
  Int* pCUSliceMap    = pcCU->getSliceSUMap();
  UInt maxNumSUInLCU = getNumPartInCU();
  UInt maxNumSUInSGU = maxNumSUInLCU >> (sliceGranularyDepth << 1);
  UInt maxNumSUInLCUWidth = getNumPartInWidth();
  UInt LPelX, TPelY;
  UInt currSU;


  //get the number of valid NBFilterBLock
  currSU   = startSU;
  while(currSU <= endSU)
  {
    LPelX = LCUX + g_auiRasterToPelX[ g_auiZscanToRaster[currSU] ];
    TPelY = LCUY + g_auiRasterToPelY[ g_auiZscanToRaster[currSU] ];

    while(!( LPelX < picWidth ) || !( TPelY < picHeight ))
    {
      currSU += maxNumSUInSGU;
      if(currSU >= maxNumSUInLCU || currSU > endSU)
      {
        break;
      }
      LPelX = LCUX + g_auiRasterToPelX[ g_auiZscanToRaster[currSU] ];
      TPelY = LCUY + g_auiRasterToPelY[ g_auiZscanToRaster[currSU] ];
    }

    if(currSU >= maxNumSUInLCU || currSU > endSU)
    {
      break;
    }

    NDBFBlockInfo NDBFBlock;

    NDBFBlock.tileID  = tileID;
    NDBFBlock.sliceID = sliceID;
    NDBFBlock.posY    = TPelY;
    NDBFBlock.posX    = LPelX;
    NDBFBlock.startSU = currSU;

    UInt uiLastValidSU  = currSU;
    UInt uiIdx, uiLPelX_su, uiTPelY_su;
    for(uiIdx = currSU; uiIdx < currSU + maxNumSUInSGU; uiIdx++)
    {
      if(uiIdx > endSU)
      {
        break;        
      }
      uiLPelX_su   = LCUX + g_auiRasterToPelX[ g_auiZscanToRaster[uiIdx] ];
      uiTPelY_su   = LCUY + g_auiRasterToPelY[ g_auiZscanToRaster[uiIdx] ];
      if( !(uiLPelX_su < picWidth ) || !( uiTPelY_su < picHeight ))
      {
        continue;
      }
      pCUSliceMap[uiIdx] = sliceID;
      uiLastValidSU = uiIdx;
    }
    NDBFBlock.endSU = uiLastValidSU;

    UInt rTLSU = g_auiZscanToRaster[ NDBFBlock.startSU ];
    UInt rBRSU = g_auiZscanToRaster[ NDBFBlock.endSU   ];
    NDBFBlock.widthSU  = (rBRSU % maxNumSUInLCUWidth) - (rTLSU % maxNumSUInLCUWidth)+ 1;
    NDBFBlock.heightSU = (UInt)(rBRSU / maxNumSUInLCUWidth) - (UInt)(rTLSU / maxNumSUInLCUWidth)+ 1;
    NDBFBlock.width    = NDBFBlock.widthSU  * getMinCUWidth();
    NDBFBlock.height   = NDBFBlock.heightSU * getMinCUHeight();

    pcCU->getNDBFilterBlocks()->push_back(NDBFBlock);

    currSU += maxNumSUInSGU;
  }

}

/** destroy non-deblocked filter information for LCU
 */
Void TComPic::destroyNonDBFilterInfo()
{
  if(m_pbValidSlice != NULL)
  {
    delete[] m_pbValidSlice;
    m_pbValidSlice = NULL;
  }

  if(m_pSliceSUMap != NULL)
  {
    delete[] m_pSliceSUMap;
    m_pSliceSUMap = NULL;
  }
  for( UInt CUAddr = 0; CUAddr < getNumCUsInFrame() ; CUAddr++ )
  {
    TComDataCU* pcCU = getCU( CUAddr );
    pcCU->getNDBFilterBlocks()->clear();
  }

  if( m_bIndependentSliceBoundaryForNDBFilter || m_bIndependentTileBoundaryForNDBFilter)
  {
    m_pNDBFilterYuvTmp->destroy();
    delete m_pNDBFilterYuvTmp;
    m_pNDBFilterYuvTmp = NULL;
  }

}


//! \}
