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



/** \file     TAppDecTop.cpp
    \brief    Decoder application class
*/

#include <list>
#include <stdio.h>
#include <fcntl.h>
#include <assert.h>

#include "TAppDecTop.h"

// ====================================================================================================================
// Local constants
// ====================================================================================================================

/// initial bitstream buffer size
/// should be large enough for parsing SPS
/// resized as a function of picture size after parsing SPS
#define BITS_BUF_SIZE 65536

// ====================================================================================================================
// Constructor / destructor / initialization / destroy
// ====================================================================================================================

TAppDecTop::TAppDecTop()
{
  ::memset (m_abDecFlag, 0, sizeof (m_abDecFlag));
  m_bUsingDepth = false;
//  m_iPOCLastDisplay  = -1;
  m_pScaleOffsetFile  = 0;
}

Void TAppDecTop::create()
{
  m_apcBitstream  = new TComBitstream;
  
  m_apcBitstream->create( BITS_BUF_SIZE );
}

Void TAppDecTop::destroy()
{
  if ( m_apcBitstream )
  {
    m_apcBitstream->destroy();
    delete m_apcBitstream;
    m_apcBitstream = NULL;
  }
  if( m_pchBitstreamFile )
  {
    free(m_pchBitstreamFile);
  }
  if( m_pchReconFile )
  {
    free(m_pchReconFile);
  }
}

// ====================================================================================================================
// Public member functions
// ====================================================================================================================

/**
 - create internal class
 - initialize internal class
 - until the end of the bitstream, call decoding function in TDecTop class
 - delete allocated buffers
 - destroy internal class
 .
 */
Void TAppDecTop::decode()
{
  TComBitstream*      pcBitstream = m_apcBitstream;
  UInt                uiPOC;
  TComList<TComPic*>* pcListPic;
  Bool bFirstSliceDecoded = true;

  // create & initialize internal classes
  xCreateDecLib();
  xInitDecLib  ();
#if DCM_SKIP_DECODING_FRAMES
//  m_iPOCLastDisplay += m_iSkipFrame;      // set the last displayed POC correctly for skip forward.
#endif

  // main decoder loop
  Bool  bEos        = false;
  Bool resizedBitstreamBuffer = false;
  
  Bool bIsDepth = false;
  Int iViewIdx = 0;
  TComSPS cComSPS ;
  NalUnitType eNalUnitType;

  
  while ( !bEos )
  {
    streampos  lLocation = m_cTVideoIOBitstreamFile.getFileLocation();
    bEos                 = m_cTVideoIOBitstreamFile.readBits( pcBitstream );
    if (bEos)
    {
      //if (!bFirstSliceDecoded) m_cTDecTop.decode( bEos, pcBitstream, uiPOC, pcListPic, eNalUnitType, cComSPS, m_iSkipFrame, m_iPOCLastDisplay);
      if( bIsDepth )
      {
        if (!bFirstSliceDecoded) m_acTDecDepthTopList[iViewIdx]->decode( bEos, pcBitstream, uiPOC, pcListPic, eNalUnitType, cComSPS, m_iSkipFrame, m_aiDepthPOCLastDisplayList[iViewIdx] );
        m_acTDecDepthTopList[iViewIdx]->executeDeblockAndAlf( bEos, pcBitstream, uiPOC, pcListPic, m_iSkipFrame, m_aiDepthPOCLastDisplayList[iViewIdx]);
      }
      else
      {
        if (!bFirstSliceDecoded) m_acTDecTopList[iViewIdx]->decode( bEos, pcBitstream, uiPOC, pcListPic, eNalUnitType, cComSPS, m_iSkipFrame, m_aiPOCLastDisplayList[iViewIdx] );
        m_acTDecTopList[iViewIdx]->executeDeblockAndAlf( bEos, pcBitstream, uiPOC, pcListPic, m_iSkipFrame, m_aiPOCLastDisplayList[iViewIdx]);
      }
      if( pcListPic )
      {
        // write reconstuction to file
        xWriteOutput( pcListPic );
      }
      break;
    }
    
    // call actual decoding function
#if DCM_SKIP_DECODING_FRAMES
    Bool bNewPicture;
    if( bIsDepth )
      bNewPicture = m_acTDecDepthTopList[iViewIdx]->decode( bEos, pcBitstream, uiPOC, pcListPic, eNalUnitType, cComSPS, m_iSkipFrame, m_aiDepthPOCLastDisplayList[iViewIdx] );
    else
      bNewPicture = m_acTDecTopList[iViewIdx]->decode( bEos, pcBitstream, uiPOC, pcListPic, eNalUnitType, cComSPS, m_iSkipFrame, m_aiPOCLastDisplayList[iViewIdx] );
    bFirstSliceDecoded   = true;

    if( eNalUnitType == NAL_UNIT_SPS )
    {
#if POZNAN_SYNTH
      if(cComSPS.getViewId()==0 && !cComSPS.isDepth()) // it should be called at first view at the begining of the stream
        initRenderer(cComSPS);
#endif
      if( cComSPS.isDepth() && (m_bUsingDepth==false) )  // expected not using depth, but bitstream are using depth
      {                                                     // know from sps
        assert( cComSPS.getViewId() == 0 && iViewIdx == 0 && !bIsDepth );
        startUsingDepth() ;
      }
      if( cComSPS.isDepth() && !bIsDepth )
      {
        assert( cComSPS.getViewId() == iViewIdx );
        m_acTDecDepthTopList[iViewIdx]->setSPS(cComSPS);
      }
      else if( cComSPS.getViewId() >= m_acTVideoIOYuvReconFileList.size() ) // expecting iViewIdx, but got cComSPS.getViewIdx()
      {
        assert( cComSPS.getViewId() == m_acTVideoIOYuvReconFileList.size() );
        assert( !cComSPS.isDepth() );
        increaseNumberOfViews(cComSPS.getViewId()+1);
        m_acTDecTopList[cComSPS.getViewId()]->setSPS(cComSPS);
      }
      bEos = m_cTVideoIOBitstreamFile.readBits( pcBitstream );
      assert( !bEos);
      if( cComSPS.isDepth() )
        m_acTDecDepthTopList[cComSPS.getViewId()]->decode( bEos, pcBitstream, uiPOC, pcListPic, eNalUnitType, cComSPS, m_iSkipFrame, m_aiDepthPOCLastDisplayList[cComSPS.getViewId()]); // decode PPS
      else
        m_acTDecTopList[cComSPS.getViewId()]->decode( bEos, pcBitstream, uiPOC, pcListPic, eNalUnitType, cComSPS, m_iSkipFrame, m_aiPOCLastDisplayList[cComSPS.getViewId()]); // decode PPS
      assert( eNalUnitType == NAL_UNIT_PPS );
    }
    assert( eNalUnitType != NAL_UNIT_SEI ); // not yet supported for MVC
    if (bNewPicture)
    {
      if( bIsDepth )
        m_acTDecDepthTopList[iViewIdx]->executeDeblockAndAlf( bEos, pcBitstream, uiPOC, pcListPic, m_iSkipFrame, m_aiDepthPOCLastDisplayList[iViewIdx]);
      else
        m_acTDecTopList[iViewIdx]->executeDeblockAndAlf( bEos, pcBitstream, uiPOC, pcListPic, m_iSkipFrame, m_aiPOCLastDisplayList[iViewIdx]);
      if (!m_cTVideoIOBitstreamFile.good()) m_cTVideoIOBitstreamFile.clear();
      m_cTVideoIOBitstreamFile.setFileLocation( lLocation );
      bFirstSliceDecoded = false;

      if( m_bUsingDepth && !bIsDepth )
      {
        bIsDepth = true;
      }
      else
      {
        bIsDepth = false;
        if( iViewIdx<m_acTDecTopList.size()-1)
        {
          iViewIdx++ ;
        }
        else
        {
          iViewIdx = 0;

          // end of access unit: delete extra pic buffers
          Int iNumViews = (Int)m_acTVideoIOYuvReconFileList.size();
          for( Int iVId = 0; iVId < iNumViews; iVId++ )
          {
            if( iVId < (Int)m_acTDecTopList.size() &&  m_acTDecTopList[iVId] )
            {
              m_acTDecTopList[iVId]->deleteExtraPicBuffers( (Int)uiPOC );
            }
            if( iVId < (Int)m_acTDecDepthTopList.size() && m_acTDecDepthTopList[iVId] )
            {
              m_acTDecDepthTopList[iVId]->deleteExtraPicBuffers( (Int)uiPOC );
            }
          }

#if AMVP_BUFFERCOMPRESS
          // compress motion for entire access unit
          for( Int iVId = 0; iVId < iNumViews; iVId++ )
          {
            if( iVId < (Int)m_acTDecTopList.size() &&  m_acTDecTopList[iVId] )
            {
              m_acTDecTopList[iVId]->compressMotion( (Int)uiPOC );
            }
            if( iVId < (Int)m_acTDecDepthTopList.size() && m_acTDecDepthTopList[iVId] )
            {
              m_acTDecDepthTopList[iVId]->compressMotion( (Int)uiPOC );
            }
          }
#endif
        }
      }
    }
#else
#error
    m_cTDecTop.decode( bEos, pcBitstream, uiPOC, pcListPic );
#endif

    
    if (!resizedBitstreamBuffer)
    {
      TComSPS *sps = m_acTDecTopList[0]->getSPS();
      if (sps)
      {
        pcBitstream->destroy();
        pcBitstream->create(sps->getWidth() * sps->getHeight() * 2);
        resizedBitstreamBuffer = true;
      }
    }
    
    if( pcListPic )
    {
      // write reconstuction to file
      xWriteOutput( pcListPic );
    }
  }
  
  // delete buffers
  for(Int i=0; i<m_acTDecTopList.size(); i++)
    m_acTDecTopList[i]->deletePicBuffer();

  if (m_bUsingDepth)
  {
    for(Int i=0; i<m_acTDecDepthTopList.size(); i++)
      m_acTDecDepthTopList[i]->deletePicBuffer();
  }
  
  // destroy internal classes
  xDestroyDecLib();
}

// ====================================================================================================================
// Protected member functions
// ====================================================================================================================

Void TAppDecTop::xCreateDecLib()
{
  // open bitstream file
  m_cTVideoIOBitstreamFile.openBits( m_pchBitstreamFile, false);  // read mode

  // create decoder class
//  m_cTDecTop.create();
  m_acTDecTopList.push_back(new TDecTop) ;// at least one decoder
  m_acTDecTopList[0]->create() ;

  m_aiPOCLastDisplayList.push_back(-1+m_iSkipFrame) ;

  if( m_pchScaleOffsetFile ) 
  { 
    m_pScaleOffsetFile = ::fopen( m_pchScaleOffsetFile, "wt" ); 
    AOF( m_pScaleOffsetFile ); 
  }
  m_cCamParsCollector.init( m_pScaleOffsetFile );
}

Void TAppDecTop::xDestroyDecLib()
{
  // close bitstream file
  m_cTVideoIOBitstreamFile.closeBits();

  for(Int iViewIdx=0; iViewIdx<m_acTVideoIOYuvReconFileList.size() ; iViewIdx++)
  {
    m_acTVideoIOYuvReconFileList[iViewIdx]->close();
    delete m_acTVideoIOYuvReconFileList[iViewIdx]; m_acTVideoIOYuvReconFileList[iViewIdx] = NULL ;
  }

  // destroy decoder class
//  m_cTDecTop.destroy();
  for(Int iViewIdx=0; iViewIdx<m_acTDecTopList.size() ; iViewIdx++)
  {
    m_acTDecTopList[iViewIdx]->destroy() ;
    delete m_acTDecTopList[iViewIdx] ; m_acTDecTopList[iViewIdx] = NULL ;
  }

  for(Int iViewIdx=0; iViewIdx<m_acTVideoIOYuvDepthReconFileList.size() ; iViewIdx++)
  {
    m_acTVideoIOYuvDepthReconFileList[iViewIdx]->close();
    delete m_acTVideoIOYuvDepthReconFileList[iViewIdx]; m_acTVideoIOYuvDepthReconFileList[iViewIdx] = NULL ;
  }

  for(Int iViewIdx=0; iViewIdx<m_acTDecDepthTopList.size() ; iViewIdx++)
  {
    m_acTDecDepthTopList[iViewIdx]->destroy() ;
    delete m_acTDecDepthTopList[iViewIdx] ; m_acTDecDepthTopList[iViewIdx] = NULL ;
  }

  m_cCamParsCollector.uninit();
  if( m_pScaleOffsetFile ) 
  { 
    ::fclose( m_pScaleOffsetFile ); 
  }
}

Void TAppDecTop::xInitDecLib()
{
  // initialize decoder class
  m_acTDecTopList[0]->init( this );
  m_acTDecTopList[0]->setViewIdx(0);
  m_acTDecTopList[0]->setPictureDigestEnabled(m_pictureDigestEnabled);
  m_acTDecTopList[0]->setCamParsCollector( &m_cCamParsCollector );
}

/** \param pcListPic list of pictures to be written to file
    \param bFirst    first picture?
    \todo            DYN_REF_FREE should be revised
 */
Void TAppDecTop::xWriteOutput( TComList<TComPic*>* pcListPic )
{
  TComList<TComPic*>::iterator iterPic   = pcListPic->begin();

  while (iterPic != pcListPic->end())
  {
    TComPic* pcPic = *(iterPic);
    Int iViewIdx = pcPic->getViewIdx();
    Int bIsDepth = pcPic->getSlice(0)->getSPS()->isDepth() ;

    if (!bIsDepth)
    {
      if( m_acTVideoIOYuvReconFileList.size() < iViewIdx+1 )
            increaseNumberOfViews( iViewIdx+1 ) ;

      if ( pcPic->getReconMark() && pcPic->getPOC() == (m_aiPOCLastDisplayList[iViewIdx] + 1) )
      {
        // write to file
        if ( m_pchReconFile )
        {
          m_acTVideoIOYuvReconFileList[iViewIdx]->write( pcPic->getPicYuvRec(), pcPic->getSlice(0)->getSPS()->getPad() );
        }

        // update POC of display order
        m_aiPOCLastDisplayList[iViewIdx] = pcPic->getPOC();

        // erase non-referenced picture in the reference picture list after display
        if ( !pcPic->getSlice(0)->isReferenced() && pcPic->getReconMark() == true )
        {
  #if !DYN_REF_FREE
          pcPic->setReconMark(false);

          // mark it should be extended later
          pcPic->getPicYuvRec()->setBorderExtension( false );

  #else
          pcPic->destroy();
          pcListPic->erase( iterPic );
          iterPic = pcListPic->begin(); // to the beginning, non-efficient way, have to be revised!
          continue;
  #endif
        }
      }
    } /// end !bIsDepth
    else
    {
      if( m_acTVideoIOYuvDepthReconFileList.size() < iViewIdx+1 )
             increaseNumberOfViews( iViewIdx+1 ) ;

      if ( pcPic->getReconMark() && pcPic->getPOC() == (m_aiDepthPOCLastDisplayList[iViewIdx] + 1) )
      {
        // write to file
        if ( m_pchReconFile )
        {
          m_acTVideoIOYuvDepthReconFileList[iViewIdx]->write( pcPic->getPicYuvRec(), pcPic->getSlice(0)->getSPS()->getPad() );
        }

        // update POC of display order
        m_aiDepthPOCLastDisplayList[iViewIdx] = pcPic->getPOC();

        // erase non-referenced picture in the reference picture list after display
        if ( !pcPic->getSlice(0)->isReferenced() && pcPic->getReconMark() == true )
        {
  #if !DYN_REF_FREE
          pcPic->setReconMark(false);

          // mark it should be extended later
          pcPic->getPicYuvRec()->setBorderExtension( false );

  #else
          pcPic->destroy();
          pcListPic->erase( iterPic );
          iterPic = pcListPic->begin(); // to the beginning, non-efficient way, have to be revised!
          continue;
  #endif
        }
      }
    } // end bIsDepth

    iterPic++;
  }
}

Void TAppDecTop::startUsingDepth()
{
  m_bUsingDepth = true ;
  increaseNumberOfViews( (Int)m_acTVideoIOYuvReconFileList.size() );
}

Void  TAppDecTop::increaseNumberOfViews  (Int iNewNumberOfViews)
{
  while( m_acTVideoIOYuvReconFileList.size() < iNewNumberOfViews)
  {
    m_acTVideoIOYuvReconFileList.push_back(new TVideoIOYuv);

// GT FIX
    Char cBuffer[4]  ;
    sprintf(cBuffer,"_%i",(Int)m_acTVideoIOYuvReconFileList.size()-1 );
    Char* pchNextFilename;
    xAppendToFileNameEnd( m_pchReconFile, cBuffer, pchNextFilename);
// GT FIX END
    if ( m_outputBitDepth == 0 )
      m_outputBitDepth = g_uiBitDepth + g_uiBitIncrement;
    m_acTVideoIOYuvReconFileList.back()->open( pchNextFilename, true, m_outputBitDepth, g_uiBitDepth + g_uiBitIncrement );
    free (pchNextFilename);
  }

  while( m_aiPOCLastDisplayList.size() < iNewNumberOfViews )
    m_aiPOCLastDisplayList.push_back(-1+m_iSkipFrame) ;

  while( m_acTDecTopList.size() < iNewNumberOfViews)
  {
    m_acTDecTopList.push_back(new TDecTop) ;// at least one decoder
    m_acTDecTopList.back()->create() ;
    m_acTDecTopList.back()->init( this, false );
    m_acTDecTopList.back()->setViewIdx((Int)m_acTDecTopList.size()-1);
    m_acTDecTopList.back()->setPictureDigestEnabled(m_pictureDigestEnabled);
    m_acTDecTopList.back()->setCamParsCollector( &m_cCamParsCollector );
  }
  if( m_bUsingDepth )
  {
    while( m_acTVideoIOYuvDepthReconFileList.size() < iNewNumberOfViews  )
    {
      m_acTVideoIOYuvDepthReconFileList.push_back(new TVideoIOYuv);
// GT FIX
      Char* pchTempFilename = NULL;
      xAppendToFileNameEnd( m_pchReconFile, "_depth", pchTempFilename);
      Char cBuffer[4]  ;
      sprintf(cBuffer,"_%i",(Int)m_acTVideoIOYuvDepthReconFileList.size()-1 );
      Char* pchDepthFilename = NULL;
      xAppendToFileNameEnd( pchTempFilename, cBuffer, pchDepthFilename);
// GT FIX END
      if ( m_outputBitDepth == 0 )
        m_outputBitDepth = g_uiBitDepth + g_uiBitIncrement;
      m_acTVideoIOYuvDepthReconFileList.back()->open( pchDepthFilename, true, m_outputBitDepth, g_uiBitDepth + g_uiBitIncrement );
      free (pchTempFilename);
      free( pchDepthFilename );
    }
    while( m_aiDepthPOCLastDisplayList.size() < iNewNumberOfViews )
      m_aiDepthPOCLastDisplayList.push_back(-1+m_iSkipFrame) ;
    while( m_acTDecDepthTopList.size() < iNewNumberOfViews)
    {
      m_acTDecDepthTopList.push_back(new TDecTop) ;// at least one decoder
      m_acTDecDepthTopList.back()->create() ;
      m_acTDecDepthTopList.back()->init( this, false );
      m_acTDecDepthTopList.back()->setViewIdx((Int)m_acTDecTopList.size()-1);
      m_acTDecDepthTopList.back()->setPictureDigestEnabled(m_pictureDigestEnabled);
      m_acTDecDepthTopList.back()->setToDepth( true );
      m_acTDecDepthTopList.back()->setCamParsCollector( &m_cCamParsCollector );
    }
  }
}


// GT FIX
std::vector<TComPic*> TAppDecTop::getSpatialRefPics( Int iViewIdx, Int iPoc, Bool bIsDepth ) // only for mvc functionality yet
{
  std::vector<TComPic*> apcRefPics( iViewIdx, (TComPic*)NULL );
  for( int iRefViewIdx = 0; iRefViewIdx < iViewIdx; iRefViewIdx++ )
  {
    TComPic* pcRefPic = getPicFromView( iRefViewIdx, iPoc, bIsDepth );
    assert( pcRefPic != NULL );
    apcRefPics[iRefViewIdx] = pcRefPic;
  }
  return apcRefPics;
}

TComPic* TAppDecTop::getPicFromView( Int iViewIdx, Int iPoc, bool bIsDepth )
{
  TComList<TComPic*>* apcListPic = (bIsDepth ? m_acTDecDepthTopList[iViewIdx] : m_acTDecTopList[iViewIdx])->getListPic();
  TComPic* pcRefPic = NULL;
  for( TComList<TComPic*>::iterator it=apcListPic->begin(); it!=apcListPic->end(); it++ )
  {
    if( (*it)->getPOC() == iPoc )
    {
      pcRefPic = *it;
      break;
    }
  }
  return pcRefPic;
}

#if POZNAN_SYNTH
Void TAppDecTop::initRenderer(TComSPS &cComSPS)
{
  m_cAvailabilityRenderer.init(cComSPS.getWidth(), cComSPS.getHeight(),true,0,LOG2_DISP_PREC_LUT,true, 0,0,0,0,0,6,4,1,0,6 );  //GT: simplest configuration
}
//*
Void TAppDecTop::storeSynthPicsInBuffer(Int iCoddedViewIdx,Int iCoddedViewOrderIdx, Int iCurPoc, Bool bDepth)
{
  Int  iLeftViewIdx  = -1;
  Int  iRightViewIdx = -1;
  Int  iNearestViewIdx = -1;
  Bool bIsBaseView;
  Bool bRenderFromLeft;

  Int iRelDistToLeft = 128;
  if(iCoddedViewIdx==0) //First on View Coded List
  {
    TComPic* pcPic = getPicFromView( iCoddedViewIdx, iCurPoc, false );
    return;
  }
  iNearestViewIdx = 0;
  bRenderFromLeft = iCoddedViewOrderIdx>0?true:false;
  //m_cCamParsCollector.getNearestBaseView(iCoddedViewIdx, iNearestViewIdx, iRelDistToLeft, bRenderFromLeft);

  m_cAvailabilityRenderer.setShiftLUTs(
    m_cCamParsCollector.getBaseViewShiftLUTD()[iNearestViewIdx][iCoddedViewIdx],
    m_cCamParsCollector.getBaseViewShiftLUTI()[iNearestViewIdx][iCoddedViewIdx],
    m_cCamParsCollector.getBaseViewShiftLUTI()[iNearestViewIdx][iCoddedViewIdx],
    m_cCamParsCollector.getBaseViewShiftLUTD()[iNearestViewIdx][iCoddedViewIdx],//right
    m_cCamParsCollector.getBaseViewShiftLUTI()[iNearestViewIdx][iCoddedViewIdx],
    m_cCamParsCollector.getBaseViewShiftLUTI()[iNearestViewIdx][iCoddedViewIdx],
    iRelDistToLeft
  );

  TComPic* pcPic = getPicFromView( iCoddedViewIdx, iCurPoc, bDepth );

  TComPicYuv* pcPicYuvSynthView = pcPic->getPicYuvSynth();
  TComPicYuv* pcPicYuvAvailView = pcPic->getPicYuvAvail();
  if(!pcPicYuvSynthView)
  {
    pcPic->addSynthesisBuffer();
    pcPicYuvSynthView = pcPic->getPicYuvSynth();
  }
  if(!pcPicYuvAvailView)
  {
    pcPic->addAvailabilityBuffer();
    pcPicYuvAvailView = pcPic->getPicYuvAvail();
  }

  //m_cAvailabilityRenderer.extrapolateAvailabilityView( xGetPicFromView( iNearestViewIdx, iCurPoc, false )->getPicYuvRec(), xGetPicFromView( iNearestViewIdx, iCurPoc, true )->getPicYuvRec(), pcPicYuvERView, pcPicYuvAvailView, bRenderFromLeft );
  m_cAvailabilityRenderer.extrapolateAvailabilityView( getPicFromView( iNearestViewIdx, iCurPoc, false )->getPicYuvRec(), getPicFromView( iNearestViewIdx, iCurPoc, true )->getPicYuvRec(), pcPicYuvSynthView, pcPicYuvAvailView, bRenderFromLeft );

  pcPicYuvAvailView->setBorderExtension( false );//Needed??
  pcPicYuvAvailView->extendPicBorder();//Needed??

#if POZNAN_OUTPUT_AVAILABLE_MAP
  {
  Char acFilenameBase[1024];
  ::sprintf( acFilenameBase,  "Available_%s_%s_V%d.yuv", (bDepth ? "Depth":"Tex"),( true ? "Dec" : "Enc" ), iCoddedViewIdx);
  pcPicYuvAvailView->dump(acFilenameBase, iCurPoc!=0);
  }
#endif
#if POZNAN_OUTPUT_SYNTH
  {
  Char acFilenameBase[1024];
  ::sprintf( acFilenameBase,  "Synth_%s_%s_V%d.yuv", (bDepth ? "Depth":"Tex"),( true ? "Dec" : "Enc" ), iCoddedViewIdx );
  pcPicYuvSynthView->dump(acFilenameBase, iCurPoc!=0);
  }
#endif
  
}
#endif
