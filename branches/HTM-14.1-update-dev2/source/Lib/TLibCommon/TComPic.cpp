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
, m_pcPicYuvPred                          (NULL)
, m_pcPicYuvResi                          (NULL)
, m_bReconstructed                        (false)
, m_bNeededForOutput                      (false)
#if NH_MV
, m_bPicOutputFlag                        (false)
#endif
, m_uiCurrSliceIdx                        (0)
, m_bCheckLTMSB                           (false)
#if NH_MV
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
  for(UInt i=0; i<NUM_PIC_YUV; i++)
  {
    m_apcPicYuv[i]      = NULL;
  }
#if NH_3D_QTLPC
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

Void TComPic::create( const TComSPS &sps, const TComPPS &pps, const Bool bIsVirtual)
{
  const ChromaFormat chromaFormatIDC = sps.getChromaFormatIdc();
  const Int          iWidth          = sps.getPicWidthInLumaSamples();
  const Int          iHeight         = sps.getPicHeightInLumaSamples();
  const UInt         uiMaxCuWidth    = sps.getMaxCUWidth();
  const UInt         uiMaxCuHeight   = sps.getMaxCUHeight();
  const UInt         uiMaxDepth      = sps.getMaxTotalCUDepth();

  m_picSym.create( sps, pps, uiMaxDepth );
  if (!bIsVirtual)
  {
    m_apcPicYuv[PIC_YUV_ORG    ]   = new TComPicYuv;  m_apcPicYuv[PIC_YUV_ORG     ]->create( iWidth, iHeight, chromaFormatIDC, uiMaxCuWidth, uiMaxCuHeight, uiMaxDepth, true );
    m_apcPicYuv[PIC_YUV_TRUE_ORG]  = new TComPicYuv;  m_apcPicYuv[PIC_YUV_TRUE_ORG]->create( iWidth, iHeight, chromaFormatIDC, uiMaxCuWidth, uiMaxCuHeight, uiMaxDepth, true );
  }
  m_apcPicYuv[PIC_YUV_REC]  = new TComPicYuv;  m_apcPicYuv[PIC_YUV_REC]->create( iWidth, iHeight, chromaFormatIDC, uiMaxCuWidth, uiMaxCuHeight, uiMaxDepth, true );

  // there are no SEI messages associated with this picture initially
  if (m_SEIs.size() > 0)
  {
    deleteSEIs (m_SEIs);
  }
  m_bUsedByCurr = false;

#if H_3D_FCO
/* initialize the texture to depth reference status */
  for (int j=0; j<2; j++)
  {
      for (int i=0; i<MAX_NUM_REF; i++)
      {
          m_aiTexToDepRef[j][i] = -1;
      }
  }
#endif

}

Void TComPic::destroy()
{
  m_picSym.destroy();

  for(UInt i=0; i<NUM_PIC_YUV; i++)
  {
    if (m_apcPicYuv[i])
    {
      m_apcPicYuv[i]->destroy();
      delete m_apcPicYuv[i];
      m_apcPicYuv[i]  = NULL;
    }
  }

  deleteSEIs(m_SEIs);
}
#if NH_3D
Void TComPic::compressMotion(Int scale)
#else
Void TComPic::compressMotion()
#endif
{
  TComPicSym* pPicSym = getPicSym();
  for ( UInt uiCUAddr = 0; uiCUAddr < pPicSym->getNumberOfCtusInFrame(); uiCUAddr++ )
  {
    TComDataCU* pCtu = pPicSym->getCtu(uiCUAddr);
#if NH_3D
    pCtu->compressMV(scale); 
#else
    pCtu->compressMV();
#endif
    
  }
}

Bool  TComPic::getSAOMergeAvailability(Int currAddr, Int mergeAddr)
{
  Bool mergeCtbInSliceSeg = (mergeAddr >= getPicSym()->getCtuTsToRsAddrMap(getCtu(currAddr)->getSlice()->getSliceCurStartCtuTsAddr()));
  Bool mergeCtbInTile     = (getPicSym()->getTileIdxMap(mergeAddr) == getPicSym()->getTileIdxMap(currAddr));
  return (mergeCtbInSliceSeg && mergeCtbInTile);
}

UInt TComPic::getSubstreamForCtuAddr(const UInt ctuAddr, const Bool bAddressInRaster, TComSlice *pcSlice)
{
  UInt subStrm;
  const bool bWPPEnabled=pcSlice->getPPS()->getEntropyCodingSyncEnabledFlag();
  const TComPicSym &picSym            = *(getPicSym());

  if ((bWPPEnabled && picSym.getFrameHeightInCtus()>1) || (picSym.getNumTiles()>1)) // wavefronts, and possibly tiles being used.
  {
    if (bWPPEnabled)
    {
      const UInt ctuRsAddr                = bAddressInRaster?ctuAddr : picSym.getCtuTsToRsAddrMap(ctuAddr);
      const UInt frameWidthInCtus         = picSym.getFrameWidthInCtus();
      const UInt tileIndex                = picSym.getTileIdxMap(ctuRsAddr);
      const UInt numTileColumns           = (picSym.getNumTileColumnsMinus1()+1);
      const TComTile *pTile               = picSym.getTComTile(tileIndex);
      const UInt firstCtuRsAddrOfTile     = pTile->getFirstCtuRsAddr();
      const UInt tileYInCtus              = firstCtuRsAddrOfTile / frameWidthInCtus;
      // independent tiles => substreams are "per tile"
      const UInt ctuLine                  = ctuRsAddr / frameWidthInCtus;
      const UInt startingSubstreamForTile =(tileYInCtus*numTileColumns) + (pTile->getTileHeightInCtus()*(tileIndex%numTileColumns));
      subStrm = startingSubstreamForTile + (ctuLine - tileYInCtus);
    }
    else
    {
      const UInt ctuRsAddr                = bAddressInRaster?ctuAddr : picSym.getCtuTsToRsAddrMap(ctuAddr);
      const UInt tileIndex                = picSym.getTileIdxMap(ctuRsAddr);
      subStrm=tileIndex;
    }
  }
  else
  {
    // dependent tiles => substreams are "per frame".
    subStrm = 0;
  }
  return subStrm;
}

#if NH_MV
Void TComPic::print( Bool legend )
{
  if ( legend )
    std::cout  << std::endl << "LId"        << "\t" << "POC"   << "\t" << "Rec"          << "\t" << "Ref"                       << "\t" << "LT"            <<  "\t" << "OutMark" <<  "\t" << "OutFlag" << std::endl;
  else
    std::cout  << getLayerId() << "\t" << getPOC()<< "\t" << getReconMark() << "\t" << getSlice(0)->isReferenced() << "\t" << getIsLongTerm() << "\t" << getOutputMark() << "\t" << getSlice(0)->getPicOutputFlag() <<std::endl;
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

#if NH_3D
TComPic* TComPicLists::getPic( Int viewIndex, Bool depthFlag, Int poc )
{
  return getPic   ( m_vps->getLayerIdInNuh( viewIndex, depthFlag ), poc );
}
#endif
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

#if NH_3D
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
#endif
#endif // NH_MV

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
