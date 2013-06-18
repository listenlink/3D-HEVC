/* The copyright in this software is being made available under the BSD
 * License, included below. This software may be subject to other third party
 * and contributor rights, including patent rights, and no such rights are
 * granted under this license.
 *
 * Copyright (c) 2010-2011, ISO/IEC
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
 *  * Neither the name of the ISO/IEC nor the names of its contributors may
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



/** \file     TComDepthMapGenerator.cpp
    \brief    depth map generator class
*/



#include "CommonDef.h"
#include "TComDepthMapGenerator.h"



TComDepthMapGenerator::TComDepthMapGenerator()
{
  m_bCreated            = false;
  m_bInit               = false;
  m_bDecoder            = false;
  m_pcPrediction        = 0;
  m_pcSPSAccess         = 0;
  m_pcAUPicAccess       = 0;

  m_uiSubSampExpX       = 0;
  m_uiSubSampExpY       = 0;
}

TComDepthMapGenerator::~TComDepthMapGenerator()
{
  destroy ();
  uninit  ();
}

Void
TComDepthMapGenerator::create( Bool bDecoder, UInt uiPicWidth, UInt uiPicHeight, UInt uiMaxCUDepth, UInt uiMaxCUWidth, UInt uiMaxCUHeight, UInt uiOrgBitDepth, UInt uiSubSampExpX, UInt uiSubSampExpY )
{
  destroy();
  m_bDecoder            = bDecoder;

  m_uiSubSampExpX       = uiSubSampExpX;
  m_uiSubSampExpY       = uiSubSampExpY;
  m_bCreated    = true;
}

Void
TComDepthMapGenerator::destroy()
{
  if( m_bCreated )
  {
    m_bCreated    = false;

    m_uiSubSampExpX       = 0;
    m_uiSubSampExpY       = 0;
    m_bDecoder            = false;
  }
}


Void
TComDepthMapGenerator::init( TComPrediction* pcPrediction, TComVPSAccess* pcVPSAccess, TComSPSAccess* pcSPSAccess, TComAUPicAccess* pcAUPicAccess )
{
  AOF( pcPrediction  );
  AOF( pcSPSAccess   );
  AOF( pcAUPicAccess );
  uninit();
  m_pcPrediction  = pcPrediction;
  m_pcVPSAccess   = pcVPSAccess;
  m_pcSPSAccess   = pcSPSAccess;
  m_pcAUPicAccess = pcAUPicAccess;
  m_bInit         = true;
}

Void
TComDepthMapGenerator::uninit()
{
  if( m_bInit )
  {
    m_bInit         = false;
    m_pcPrediction  = 0;
    m_pcSPSAccess   = 0;
    m_pcAUPicAccess = 0;
  }
}

Void  
TComDepthMapGenerator::initViewComponent( TComPic* pcPic )
{
  AOF  ( m_bCreated && m_bInit );
  AOF  ( pcPic );
  AOT  ( pcPic->getSPS()->getViewIndex() && !pcPic->getSPS()->isDepth() && pcPic->getPOC() && pcPic->getSPS()->getPredDepthMapGeneration() != m_pcSPSAccess->getPdm() );
  m_bPDMAvailable = false;
  m_uiCurrViewIndex  = pcPic->getSPS()->getViewIndex();

  // update SPS list and AU pic list and set depth map generator in SPS
  m_pcVPSAccess  ->addVPS( pcPic->getVPS() );
  m_pcSPSAccess  ->addSPS( pcPic->getSPS() );
  m_pcAUPicAccess->addPic( pcPic );
  pcPic->getSPS()->setDepthMapGenerator( this );

  // check whether we have depth data or don't use pred depth prediction
  ROFVS( pcPic->getSPS()->getViewIndex() );
  ROTVS( pcPic->getSPS()->isDepth  () );
  ROFVS( m_pcSPSAccess->getPdm     () ); 
}


#if H_3D_IV_MERGE
Bool
TComDepthMapGenerator::getPdmCandidate(TComDataCU* pcCU, UInt uiPartIdx, RefPicList eRefPicList, Int iRefIdx, Int* paiPdmRefIdx, TComMv* pacPdmMv, DisInfo* pDInfo, Int* iPdm, Bool bMerge )
{
  AOF  ( m_bCreated && m_bInit );
  TComSlice*    pcSlice     = pcCU->getSlice ();
  TComSPS*      pcSPS       = pcSlice->getSPS();
  AOF  ( pcSPS->getViewIndex() == m_uiCurrViewIndex );

  TComPic*      pcRefPic    = pcSlice->getRefPic( eRefPicList, iRefIdx );
  UInt          uiRefViewId = pcRefPic->getSPS()->getViewIndex();
  Bool          bInterview  = ( uiRefViewId < m_uiCurrViewIndex );
  Bool          bPdmIView   = ( ( pcSPS->getMultiviewMvPredMode() & PDM_USE_FOR_IVIEW ) == PDM_USE_FOR_IVIEW );
  Bool          bPdmInter   = ( ( pcSPS->getMultiviewMvPredMode() & PDM_USE_FOR_INTER ) == PDM_USE_FOR_INTER );
  Bool          bPdmMerge   = ( ( pcSPS->getMultiviewMvPredMode() & PDM_USE_FOR_MERGE ) == PDM_USE_FOR_MERGE );
  if(!bMerge)
  {
    ROTRS( ( bInterview && !bMerge ) && !bPdmIView, false );
    ROTRS( (!bInterview && !bMerge ) && !bPdmInter, false );
    ROTRS(                  bMerge   && !bPdmMerge, false );
  }
  else
    ROTRS( !bPdmMerge, 0 );


  Bool abPdmAvailable[4] = {false, false, false, false};

  Int iValid = 0;
  Int iViewIndex = 0;
  for( UInt uiBIndex = 0; uiBIndex < m_uiCurrViewIndex && iValid==0; uiBIndex++ )
  {

    UInt        uiBaseIndex    = uiBIndex;
    TComPic*    pcBasePic   = m_pcAUPicAccess->getPic( uiBaseIndex );
    for( Int iRefListId = 0; iRefListId < 2 && iValid==0; iRefListId++ )
    {
      RefPicList  eRefPicListTest = RefPicList( iRefListId );
      Int         iNumRefPics = pcSlice->getNumRefIdx( eRefPicListTest ) ;
      for( Int iRefIndex = 0; iRefIndex < iNumRefPics; iRefIndex++ )
      { 
        if(pcBasePic->getPOC() == pcSlice->getRefPic( eRefPicListTest, iRefIndex )->getPOC() 
          && pcBasePic->getViewIndex() == pcSlice->getRefPic( eRefPicListTest, iRefIndex )->getViewIndex())
        {
          iValid=1;
          iViewIndex = uiBaseIndex;
          break;
        }
      }
    }
  }
  if (iValid == 0)
    return false;

  //--- get base CU/PU and check prediction mode ---
  TComPic*    pcBasePic   = m_pcAUPicAccess->getPic( iViewIndex );
  TComPicYuv* pcBaseRec   = pcBasePic->getPicYuvRec   ();
  if(bMerge || !bInterview)
  {
    Int  iCurrPosX, iCurrPosY;
    UInt          uiPartAddr;
    Int           iWidth;
    Int           iHeight;

    pcCU->getPartIndexAndSize( uiPartIdx, uiPartAddr, iWidth, iHeight );
    pcBaseRec->getTopLeftSamplePos( pcCU->getAddr(), pcCU->getZorderIdxInCU() + uiPartAddr, iCurrPosX, iCurrPosY );
    iCurrPosX  += ( ( iWidth  - 1 ) >> 1 );
    iCurrPosY  += ( ( iHeight - 1 ) >> 1 );

    Int         iBasePosX   = Clip3( 0, pcBaseRec->getWidth () - 1, iCurrPosX + ( (pDInfo->m_acNBDV.getHor() + 2 ) >> 2 ) );
    Int         iBasePosY   = Clip3( 0, pcBaseRec->getHeight() - 1, iCurrPosY + ( (pDInfo->m_acNBDV.getVer() + 2 ) >> 2 )); 
    Int         iBaseCUAddr;
    Int         iBaseAbsPartIdx;
    pcBaseRec->getCUAddrAndPartIdx( iBasePosX , iBasePosY , iBaseCUAddr, iBaseAbsPartIdx );

    TComDataCU* pcBaseCU    = pcBasePic->getCU( iBaseCUAddr );
    if(!( pcBaseCU->getPredictionMode( iBaseAbsPartIdx ) == MODE_INTRA ))
    {
      for( UInt uiCurrRefListId = 0; uiCurrRefListId < 2; uiCurrRefListId++ )
      {
        RefPicList  eCurrRefPicList = RefPicList( uiCurrRefListId );
        if(!bMerge && eCurrRefPicList != eRefPicList)
          continue;
        Bool bLoop_stop = false;
        for(Int iLoop = 0; iLoop < 2 && !bLoop_stop; ++iLoop)
        {
          RefPicList eBaseRefPicList = (iLoop ==1)? RefPicList( 1 -  uiCurrRefListId ) : RefPicList( uiCurrRefListId );
          TComMvField cBaseMvField;
          pcBaseCU->getMvField( pcBaseCU, iBaseAbsPartIdx, eBaseRefPicList, cBaseMvField );
          Int         iBaseRefIdx     = cBaseMvField.getRefIdx();
          if (iBaseRefIdx >= 0)
          {
            Int iBaseRefPOC = pcBaseCU->getSlice()->getRefPOC(eBaseRefPicList, iBaseRefIdx);
            if (iBaseRefPOC != pcSlice->getPOC())    
            {
              for (Int iPdmRefIdx = (bMerge?0: iRefIdx); iPdmRefIdx < (bMerge? pcSlice->getNumRefIdx( eCurrRefPicList ): (iRefIdx+1)); iPdmRefIdx++)
              {
                if (iBaseRefPOC == pcSlice->getRefPOC(eCurrRefPicList, iPdmRefIdx))
                {
                  abPdmAvailable[ uiCurrRefListId ] = true;
                  TComMv cMv(cBaseMvField.getHor(), cBaseMvField.getVer());

                  //if( bMerge )
                  //{
                    //cMv.m_bDvMcp = true;
                    //cMv.m_iDvMcpDispX = pDInfo->m_acMvCand[0].getHor();
                  //}

                  pcCU->clipMv( cMv );
                  if(bMerge)
                  {
                    paiPdmRefIdx  [ uiCurrRefListId ] = iPdmRefIdx;
                    pacPdmMv      [ uiCurrRefListId ] = cMv;
                    bLoop_stop = true;
                    break;
                  }else
                  {
                    pacPdmMv  [0] = cMv;
                    return true;
                  }
                }
              }
            }
          }
        }
      }
    }
    if( bMerge )
      iPdm[0] = ( abPdmAvailable[0] ? 1 : 0 ) + ( abPdmAvailable[1] ? 2 : 0 );
  }
  if(bMerge || bInterview)
  {
    for( Int iRefListId = 0; iRefListId < 2 ; iRefListId++ )
    {
      RefPicList  eRefPicListDMV       = RefPicList( iRefListId );
      Int         iNumRefPics       = pcSlice->getNumRefIdx( eRefPicListDMV );
      for( Int iPdmRefIdx = (bMerge ? 0: iRefIdx); iPdmRefIdx < (bMerge ? iNumRefPics: (iRefIdx+1) ); iPdmRefIdx++ )
      {
        if( pcSlice->getRefPOC( eRefPicListDMV, iPdmRefIdx ) == pcSlice->getPOC())
        {
          abPdmAvailable[ iRefListId+2 ] = true;
          paiPdmRefIdx  [ iRefListId+2 ] = iPdmRefIdx;

          TComMv cMv = pDInfo->m_acNBDV; 
          cMv.setVer(0);

          pcCU->clipMv( cMv );
          pacPdmMv      [ iRefListId + 2] = cMv;
          if(bMerge)
            break;
          else
          {
            pacPdmMv [0] = cMv;
            return true;
          }
        }
      }
    }
    iPdm[1] = ( abPdmAvailable[2] ? 1 : 0 ) + ( abPdmAvailable[3] ? 2 : 0 );
  }
  return false;
}
#endif

