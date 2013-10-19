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
: m_uiTLayer                              (0)
, m_bUsedByCurr                           (false)
, m_bIsLongTerm                           (false)
, m_bIsUsedAsLongTerm                     (false)
, m_apcPicSym                             (NULL)
, m_pcPicYuvPred                          (NULL)
, m_pcPicYuvResi                          (NULL)
, m_bReconstructed                        (false)
, m_bNeededForOutput                      (false)
, m_uiCurrSliceIdx                        (0)
, m_pSliceSUMap                           (NULL)
, m_pbValidSlice                          (NULL)
, m_sliceGranularityForNDBFilter          (0)
, m_bIndependentSliceBoundaryForNDBFilter (false)
, m_bIndependentTileBoundaryForNDBFilter  (false)
, m_pNDBFilterYuvTmp                      (NULL)
, m_bCheckLTMSB                           (false)
#if H_MV
, m_layerId                               (0)
, m_viewId                                (0)
#if H_3D
, m_viewIndex                             (0)
, m_isDepth                               (false)
, m_aaiCodedScale                         (0)
, m_aaiCodedOffset                        (0)
#endif
#endif
{
  m_apcPicYuv[0]      = NULL;
  m_apcPicYuv[1]      = NULL;
#if H_3D_QTLPC
  m_bReduceBitsQTL    = 0;
#endif
#if H_3D_NBDV
  m_iNumDdvCandPics   = 0;
  m_eRapRefList       = REF_PIC_LIST_0;
  m_uiRapRefIdx       = 0;
#endif
}

TComPic::~TComPic()
{
}

Void TComPic::create( Int iWidth, Int iHeight, UInt uiMaxWidth, UInt uiMaxHeight, UInt uiMaxDepth, Window &conformanceWindow, Window &defaultDisplayWindow,
                      Int *numReorderPics, Bool bIsVirtual)

{
  m_apcPicSym     = new TComPicSym;  m_apcPicSym   ->create( iWidth, iHeight, uiMaxWidth, uiMaxHeight, uiMaxDepth );
  if (!bIsVirtual)
  {
    m_apcPicYuv[0]  = new TComPicYuv;  m_apcPicYuv[0]->create( iWidth, iHeight, uiMaxWidth, uiMaxHeight, uiMaxDepth );
  }
  m_apcPicYuv[1]  = new TComPicYuv;  m_apcPicYuv[1]->create( iWidth, iHeight, uiMaxWidth, uiMaxHeight, uiMaxDepth );
  
  // there are no SEI messages associated with this picture initially
  if (m_SEIs.size() > 0)
  {
    deleteSEIs (m_SEIs);
  }
  m_bUsedByCurr = false;

  /* store conformance window parameters with picture */
  m_conformanceWindow = conformanceWindow;
  
  /* store display window parameters with picture */
  m_defaultDisplayWindow = defaultDisplayWindow;

  /* store number of reorder pics with picture */
  memcpy(m_numReorderPics, numReorderPics, MAX_TLAYER*sizeof(Int));

  /* initialize the texture to depth reference status */
#if H_3D_FCO
  for (int j=0; j<2; j++)
  {
      for (int i=0; i<MAX_NUM_REF; i++)
      {
          m_aiTexToDepRef[j][i] = -1;
      }
  }
#endif

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
  
  deleteSEIs(m_SEIs);
}
#if H_3D
Void TComPic::compressMotion(Int scale)
#else
Void TComPic::compressMotion()
#endif
{
  TComPicSym* pPicSym = getPicSym(); 
  for ( UInt uiCUAddr = 0; uiCUAddr < pPicSym->getFrameHeightInCU()*pPicSym->getFrameWidthInCU(); uiCUAddr++ )
  {
    TComDataCU* pcCU = pPicSym->getCU(uiCUAddr);
#if H_3D
    pcCU->compressMV(scale); 
#else
    pcCU->compressMV(); 
#endif
  } 
}

/** Create non-deblocked filter information
 * \param pSliceStartAddress array for storing slice start addresses
 * \param numSlices number of slices in picture
 * \param sliceGranularityDepth slice granularity 
 * \param bNDBFilterCrossSliceBoundary cross-slice-boundary in-loop filtering; true for "cross".
 * \param numTiles number of tiles in picture
 * \param bNDBFilterCrossTileBoundary cross-tile-boundary in-loop filtering; true for "cross".
 */
Void TComPic::createNonDBFilterInfo(std::vector<Int> sliceStartAddress, Int sliceGranularityDepth
                                    ,std::vector<Bool>* LFCrossSliceBoundary
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
  Int  numSlices = (Int) sliceStartAddress.size() - 1;
  m_bIndependentSliceBoundaryForNDBFilter = false;
  if(numSlices > 1)
  {
    for(Int s=0; s< numSlices; s++)
    {
      if((*LFCrossSliceBoundary)[s] == false)
      {
        m_bIndependentSliceBoundaryForNDBFilter = true;
      }
    }
  }
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
    startAddr = sliceStartAddress[s];
    endAddr   = sliceStartAddress[s+1] -1;

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
        , *LFCrossSliceBoundary
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
#if H_MV
Void TComPic::print( Bool legend )
{
  if ( legend )
    std::cout  << "LId"        << "\t" << "POC"   << "\t" << "Rec"          << "\t" << "Ref"                       << "\t" << "LT"            << std::endl;
  else
    std::cout  << getLayerId() << "\t" << getPOC()<< "\t" << getReconMark() << "\t" << getSlice(0)->isReferenced() << "\t" << getIsLongTerm() << std::endl;
}

TComPic* TComPicLists::getPic( Int layerIdInNuh, Int poc )
{
  TComPic* pcPic = NULL;
  for(TComList<TComList<TComPic*>*>::iterator itL = m_lists.begin(); ( itL != m_lists.end() && pcPic == NULL ); itL++)
  {    
    for(TComList<TComPic*>::iterator itP=(*itL)->begin(); ( itP!=(*itL)->end() && pcPic == NULL ); itP++)
    {
      TComPic* currPic = (*itP); 
      if ( ( currPic->getPOC() == poc ) && ( currPic->getLayerId() == layerIdInNuh ) )
      {
        pcPic = currPic ;      
      }
    }
  }
  return pcPic;
}

#if H_3D
TComPic* TComPicLists::getPic( Int viewIndex, Bool depthFlag, Int poc )
{
  return getPic   ( m_vps->getLayerIdInNuh( viewIndex, depthFlag ), poc );
}

Void TComPicLists::print()
{
  Bool first = true;     
  for(TComList<TComList<TComPic*>*>::iterator itL = m_lists.begin(); ( itL != m_lists.end() ); itL++)
  {    
    for(TComList<TComPic*>::iterator itP=(*itL)->begin(); ( itP!=(*itL)->end() ); itP++)
    {
      if ( first )
      {
        (*itP)->print( true );       
        first = false; 
      }
      (*itP)->print( false );       
    }
  }
}

TComPicYuv* TComPicLists::getPicYuv( Int layerIdInNuh, Int poc, Bool reconFlag )
{
  TComPic*    pcPic = getPic( layerIdInNuh, poc );
  TComPicYuv* pcPicYuv = NULL;

  if (pcPic != NULL)
  {
    if( reconFlag )
    {
      if ( pcPic->getReconMark() )
      {
        pcPicYuv = pcPic->getPicYuvRec();
      }
    }
    else
    {
      pcPicYuv = pcPic->getPicYuvOrg();
    }
  };

  return pcPicYuv;
}

TComPicYuv* TComPicLists::getPicYuv( Int viewIndex, Bool depthFlag, Int poc, Bool recon )
{  
  Int layerIdInNuh = m_vps->getLayerIdInNuh( viewIndex, depthFlag ); 
  return getPicYuv( layerIdInNuh, poc, recon );
}
#if H_3D_ARP
TComList<TComPic*>* TComPicLists::getPicList( Int layerIdInNuh )
{
  TComList<TComList<TComPic*>*>::iterator itL = m_lists.begin();
  Int iLayer = 0;

  assert( layerIdInNuh < m_lists.size() );

  while( iLayer != layerIdInNuh )
  {
    itL++;
    iLayer++;
  }

  return *itL;
}
#endif
#endif // H_3D
#endif // H_MV

#if H_3D_NBDV 
Int TComPic::getDisCandRefPictures( Int iColPOC )
{
  UInt       uiTempLayerCurr = 7;
  TComSlice* currSlice       = getSlice(getCurrSliceIdx());
  UInt       numDdvCandPics  = 0;

  if ( !currSlice->getEnableTMVPFlag() )
    return numDdvCandPics;

  numDdvCandPics += 1;

  UInt pocCurr = currSlice->getPOC();
  UInt pocDiff = 255;

  for(UInt lpNr = 0; lpNr < (currSlice->isInterB() ? 2: 1); lpNr ++)
  {
    UInt x = lpNr ? currSlice->getColFromL0Flag() : 1 - currSlice->getColFromL0Flag();

    for (UInt i = 0; i < currSlice->getNumRefIdx(RefPicList(x)); i++)
    {
      if(currSlice->getViewIndex() == currSlice->getRefPic((RefPicList)x, i)->getViewIndex() 
        && (x == currSlice->getColFromL0Flag()||currSlice->getRefPOC((RefPicList)x, i)!= iColPOC) && numDdvCandPics!=2)
      {
        TComSlice* refSlice    = currSlice->getRefPic((RefPicList)x, i)->getSlice(getCurrSliceIdx());
        Bool       bRAP        = (refSlice->getViewIndex() && refSlice->isIRAP())? 1: 0; 
        UInt       uiTempLayer = currSlice->getRefPic((RefPicList)x, i)->getSlice(getCurrSliceIdx())->getTLayer(); 
        
        if( bRAP )
        {
          this->setRapRefIdx(i);
          this->setRapRefList((RefPicList)x);
          numDdvCandPics = 2;

          return numDdvCandPics;
        }
        else if (uiTempLayerCurr > uiTempLayer) 
        {
           uiTempLayerCurr = uiTempLayer; 
        }
      }
    }
  }

  UInt z   = -1; // GT: Added to make code compile needs to be checked!
  UInt idx = 0;
  
  for(UInt lpNr = 0; lpNr < (currSlice->isInterB() ? 2: 1); lpNr ++)
  {
    UInt x = lpNr? currSlice->getColFromL0Flag() : 1-currSlice->getColFromL0Flag();
    
    for (UInt i = 0; i < currSlice->getNumRefIdx(RefPicList(x)); i++)
    {
      Int iTempPoc = currSlice->getRefPic((RefPicList)x, i)->getPOC();
      Int iTempDiff = (iTempPoc > pocCurr) ? (iTempPoc - pocCurr): (pocCurr - iTempPoc);
      
      if(currSlice->getViewIndex() == currSlice->getRefPic((RefPicList)x, i)->getViewIndex() &&  (x == currSlice->getColFromL0Flag()||currSlice->getRefPOC((RefPicList)x, i)!= iColPOC) 
        && currSlice->getRefPic((RefPicList)x, i)->getSlice(getCurrSliceIdx())->getTLayer() == uiTempLayerCurr && pocDiff > iTempDiff)
      {
        pocDiff = iTempDiff;
        z       = x;
        idx     = i;
      }
    }
  }

  if( pocDiff < 255 )
  {
    this->setRapRefIdx(idx);
    this->setRapRefList((RefPicList) z );
    numDdvCandPics = 2;
  }

  return numDdvCandPics;
}

Void TComPic::checkTemporalIVRef()
{
  TComSlice* currSlice = getSlice(getCurrSliceIdx());
  const Int numCandPics = this->getNumDdvCandPics();
  for(Int curCandPic = 0; curCandPic < numCandPics; curCandPic++)
  {
    RefPicList eCurRefPicList   = REF_PIC_LIST_0 ;
    Int        curCandPicRefIdx = 0;
    if( curCandPic == 0 ) 
    { 
      eCurRefPicList   = RefPicList(currSlice->isInterB() ? 1-currSlice->getColFromL0Flag() : 0);
      curCandPicRefIdx = currSlice->getColRefIdx();
    }
    else                 
    {
      eCurRefPicList   = this->getRapRefList();
      curCandPicRefIdx = this->getRapRefIdx();
    }
    TComPic* pcCandColPic = currSlice->getRefPic( eCurRefPicList, curCandPicRefIdx);
    TComSlice* pcCandColSlice = pcCandColPic->getSlice(0);// currently only support single slice

    if(!pcCandColSlice->isIntra())
    {
      for( Int iColRefDir = 0; iColRefDir < (pcCandColSlice->isInterB() ? 2: 1); iColRefDir ++ )
      {
        for( Int iColRefIdx =0; iColRefIdx < pcCandColSlice->getNumRefIdx(( RefPicList )iColRefDir ); iColRefIdx++)
        {
          m_abTIVRINCurrRL[curCandPic][iColRefDir][iColRefIdx] = false;
          Int iColViewIdx    = pcCandColSlice->getViewIndex();
          Int iColRefViewIdx = pcCandColSlice->getRefPic( ( RefPicList )iColRefDir, iColRefIdx)->getViewIndex();
          if(iColViewIdx == iColRefViewIdx)
            continue;

          for(Int iCurrRefDir = 0;(iCurrRefDir < (currSlice->isInterB() ? 2: 1)) && (m_abTIVRINCurrRL[curCandPic][iColRefDir][iColRefIdx] == false ); iCurrRefDir++)
          {
            for( Int iCurrRefIdx =0; iCurrRefIdx < currSlice->getNumRefIdx(( RefPicList )iCurrRefDir ); iCurrRefIdx++)
            {
              if( currSlice->getRefPic( ( RefPicList )iCurrRefDir, iCurrRefIdx )->getViewIndex() == iColRefViewIdx )
              {  
                m_abTIVRINCurrRL[curCandPic][iColRefDir][iColRefIdx] = true;
                break;
              }
            }
          }
        }
      }
    }
  }
}
Bool TComPic::isTempIVRefValid(Int currCandPic, Int iColRefDir, Int iColRefIdx)
{
  return m_abTIVRINCurrRL[currCandPic][iColRefDir][iColRefIdx];
}

Void TComPic::checkTextureRef(  )
{
  TComSlice* pcCurrSlice = getSlice(getCurrSliceIdx());
  TComPic* pcTextPic = pcCurrSlice->getTexturePic();
#if H_3D_FCO
  if ( pcTextPic )
  {
#endif

  TComSlice* pcTextSlice = pcTextPic->getSlice(0); // currently only support single slice

  for( Int iTextRefDir = 0; (iTextRefDir < (pcTextSlice->isInterB()? 2:1) ) && !pcTextSlice->isIntra(); iTextRefDir ++ )
  {
    for( Int iTextRefIdx =0; iTextRefIdx<pcTextSlice->getNumRefIdx(( RefPicList )iTextRefDir ); iTextRefIdx++)
    {
      Int iTextRefPOC    = pcTextSlice->getRefPOC( ( RefPicList )iTextRefDir, iTextRefIdx);
      Int iTextRefViewId = pcTextSlice->getRefPic( ( RefPicList )iTextRefDir, iTextRefIdx)->getViewIndex();
      m_aiTexToDepRef[iTextRefDir][iTextRefIdx] = -1;
      Int iCurrRefDir = iTextRefDir;
      for( Int iCurrRefIdx =0; ( iCurrRefIdx<pcCurrSlice->getNumRefIdx(( RefPicList )iCurrRefDir ) ) && ( m_aiTexToDepRef[iTextRefDir][iTextRefIdx] < 0 ) ; iCurrRefIdx++)
      {
        if( pcCurrSlice->getRefPOC( ( RefPicList )iCurrRefDir, iCurrRefIdx ) == iTextRefPOC && 
          pcCurrSlice->getRefPic( ( RefPicList )iCurrRefDir, iCurrRefIdx)->getViewIndex() == iTextRefViewId )
        {  
          m_aiTexToDepRef[iTextRefDir][iTextRefIdx] = iCurrRefIdx;
        }
      }
    }

  }
#if H_3D_FCO
  }
#endif

}

Int TComPic::isTextRefValid(Int iTextRefDir, Int iTextRefIdx)
{
  return m_aiTexToDepRef[iTextRefDir][iTextRefIdx];
}
#endif
//! \}
