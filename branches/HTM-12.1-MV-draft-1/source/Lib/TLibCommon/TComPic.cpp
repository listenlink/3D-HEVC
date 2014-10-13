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
, m_apcPicSym                             (NULL)
, m_pcPicYuvPred                          (NULL)
, m_pcPicYuvResi                          (NULL)
, m_bReconstructed                        (false)
, m_bNeededForOutput                      (false)
#if H_MV
, m_bPicOutputFlag                        (false)
#endif
, m_uiCurrSliceIdx                        (0)
, m_bCheckLTMSB                           (false)
#if H_MV
, m_layerId                               (0)
, m_viewId                                (0)
#endif
{
  m_apcPicYuv[0]      = NULL;
  m_apcPicYuv[1]      = NULL;
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
Void TComPic::compressMotion()
{
  TComPicSym* pPicSym = getPicSym(); 
  for ( UInt uiCUAddr = 0; uiCUAddr < pPicSym->getFrameHeightInCU()*pPicSym->getFrameWidthInCU(); uiCUAddr++ )
  {
    TComDataCU* pcCU = pPicSym->getCU(uiCUAddr);
    pcCU->compressMV(); 
  } 
}

Bool  TComPic::getSAOMergeAvailability(Int currAddr, Int mergeAddr)
{
  Bool mergeCtbInSliceSeg = (mergeAddr >= getPicSym()->getCUOrderMap(getCU(currAddr)->getSlice()->getSliceCurStartCUAddr()/getNumPartInCU()));
  Bool mergeCtbInTile     = (getPicSym()->getTileIdxMap(mergeAddr) == getPicSym()->getTileIdxMap(currAddr));
  return (mergeCtbInSliceSeg && mergeCtbInTile);
}

#if H_MV
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

#endif // H_MV

//! \}
