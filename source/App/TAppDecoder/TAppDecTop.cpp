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

/** \file     TAppDecTop.cpp
    \brief    Decoder application class
*/

#include <list>
#include <vector>
#include <stdio.h>
#include <fcntl.h>
#include <assert.h>

#include "TAppDecTop.h"
#include "TLibDecoder/AnnexBread.h"
#include "TLibDecoder/NALread.h"

//! \ingroup TAppDecoder
//! \{

// ====================================================================================================================
// Constructor / destructor / initialization / destroy
// ====================================================================================================================

TAppDecTop::TAppDecTop()
{
  ::memset (m_abDecFlag, 0, sizeof (m_abDecFlag));
  m_useDepth = false;
  m_pScaleOffsetFile  = 0;
}

Void TAppDecTop::create()
{
}

Void TAppDecTop::destroy()
{
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
#if VIDYO_VPS_INTEGRATION|QC_MVHEVC_B0046
  increaseNumberOfViews( 0, 0, 0 );
#else
  increaseNumberOfViews( 1 );
#endif
  
#if FLEX_CODING_ORDER_M23723
  Int iDepthViewIdx = 0;
  Int iTextureViewIdx=0;
  Bool firstFrame=1;
  Bool viewIdZero=true;
  Int fcoIndex=0;  //when the current frame is not first frame,use FCO_index stand for viewDepth. 
#endif

#if SONY_COLPIC_AVAILABILITY
  m_tDecTop[0]->setViewOrderIdx(0);
#endif
  Int                 viewDepthId = 0;
  Int                 previousViewDepthId  = 0;
  UInt                uiPOC[MAX_VIEW_NUM*2];
  TComList<TComPic*>* pcListPic[MAX_VIEW_NUM*2];
  Bool                newPicture[MAX_VIEW_NUM*2];
  Bool                previousPictureDecoded = false;
  for( Int i = 0; i < MAX_VIEW_NUM*2; i++ )
  {
    uiPOC[i] = 0;
    pcListPic[i] = NULL;
    newPicture[i] = false;
#if FLEX_CODING_ORDER_M23723
    m_fcoOrder[i]=NULL;
#endif

  }

  ifstream bitstreamFile(m_pchBitstreamFile, ifstream::in | ifstream::binary);
  if (!bitstreamFile)
  {
    fprintf(stderr, "\nfailed to open bitstream file `%s' for reading\n", m_pchBitstreamFile);
    exit(EXIT_FAILURE);
  }

  if( m_pchScaleOffsetFile ) 
  { 
    m_pScaleOffsetFile = ::fopen( m_pchScaleOffsetFile, "wt" ); 
    AOF( m_pScaleOffsetFile ); 
  }
  m_cCamParsCollector.init( m_pScaleOffsetFile );

  InputByteStream bytestream(bitstreamFile);

  while (!!bitstreamFile)
  {
    /* location serves to work around a design fault in the decoder, whereby
     * the process of reading a new slice that is the first slice of a new frame
     * requires the TDecTop::decode() method to be called again with the same
     * nal unit. */
    streampos location = bitstreamFile.tellg();
    AnnexBStats stats = AnnexBStats();
    vector<uint8_t> nalUnit;
    InputNALUnit nalu;
    byteStreamNALUnit(bytestream, nalUnit, stats);

    // call actual decoding function
    if (nalUnit.empty())
    {
      /* this can happen if the following occur:
       *  - empty input file
       *  - two back-to-back start_code_prefixes
       *  - start_code_prefix immediately followed by EOF
       */
      fprintf(stderr, "Warning: Attempt to decode an empty NAL unit\n");
    }
    else
    {
      read(nalu, nalUnit);
#if QC_MVHEVC_B0046
    viewDepthId = nalu.m_layerId;
    Int depth = 0;
    Int viewId = viewDepthId;
#else
#if VIDYO_VPS_INTEGRATION
      Int viewId = 0;
      Int depth = 0;
      
      if(nalu.m_nalUnitType != NAL_UNIT_VPS || nalu.m_layerId)
      {
        // code assumes that the first nal unit is VPS
        // currently, this is a hack that requires non-first VPSs have non-zero layer_id
        viewId = getVPSAccess()->getActiveVPS()->getViewId(nalu.m_layerId);
        depth = getVPSAccess()->getActiveVPS()->getDepthFlag(nalu.m_layerId);
      }
#if FLEX_CODING_ORDER_M23723
      if (viewId>0)
      {
        viewIdZero=false;
      }
      if (viewIdZero==false&&viewId==0)
      {
        firstFrame=0; //if viewId has been more than zero and now it set to zero again, we can see that it is not the first view
      }
      if (firstFrame)
      { // if the current view is first frame, we set the viewDepthId as texture plus depth and get the FCO order 
        viewDepthId = iDepthViewIdx+iTextureViewIdx;
        m_fcoViewDepthId=viewDepthId;
      }
      else
      {//if current view is not first frame, we set the viewDepthId depended on the FCO order
        viewDepthId=0;
        if (depth)
        {
          for (fcoIndex=0;fcoIndex<2*MAX_VIEW_NUM;fcoIndex++ )
          {
            if (m_fcoOrder[fcoIndex]=='D')
            {
              if (viewId==viewDepthId)
                break;
              else
                viewDepthId++;
            }
          }
        }
        else
        {
          for (fcoIndex=0;fcoIndex<2*MAX_VIEW_NUM;fcoIndex++ )
          {
            if (m_fcoOrder[fcoIndex]=='T')
            {
              if (viewId==viewDepthId)
                break;
              else
                viewDepthId++;
            }
          }
        }

        viewDepthId=fcoIndex;

      }


#else
       viewDepthId = nalu.m_layerId;   // coding order T0D0T1D1T2D2
#endif
    
#else
      Int viewId = nalu.m_viewId;
      Int depth = nalu.m_isDepth ? 1 : 0;
#if FLEX_CODING_ORDER_M23723
      if (viewId>0)
      {
        viewIdZero=false;
      }
      if (viewIdZero==false&&viewId==0)
      {
        firstFrame=0;
      }
      if (firstFrame)
      {
        viewDepthId = iDepthViewIdx+iTextureViewIdx;
        m_fcoViewDepthId=viewDepthId;
      }
      else
      {
        viewDepthId=0;
        if (depth)
        {
          for (fcoIndex=0;fcoIndex<2*MAX_VIEW_NUM;fcoIndex++ )
          {
            if (m_fcoOrder[fcoIndex]=='D')
            {
              if (viewId==viewDepthId)
                break;
              else
                viewDepthId++;
            }
          }
        }
        else
        {
          for (fcoIndex=0;fcoIndex<2*MAX_VIEW_NUM;fcoIndex++ )
          {
            if (m_fcoOrder[fcoIndex]=='T')
            {
              if (viewId==viewDepthId)
                break;
              else
                viewDepthId++;
            }
          }
        }
        viewDepthId=fcoIndex;
      }
#else
  viewDepthId = viewId * 2 + depth;   // coding order T0D0T1D1T2D2
#endif
#endif
#endif     
      newPicture[viewDepthId] = false;
      if( viewDepthId >= m_tDecTop.size() )      
      {
#if VIDYO_VPS_INTEGRATION|QC_MVHEVC_B0046
        increaseNumberOfViews( viewDepthId, viewId, depth );
#else
        increaseNumberOfViews( viewDepthId +1 );
#endif   
      }
      if(m_iMaxTemporalLayer >= 0 && nalu.m_temporalId > m_iMaxTemporalLayer)
      {
        previousPictureDecoded = false; 
      }
      if(m_tDecTop.size() > 1 && (viewDepthId != previousViewDepthId) && previousPictureDecoded )
      {
        m_tDecTop[previousViewDepthId]->executeDeblockAndAlf(uiPOC[previousViewDepthId], pcListPic[previousViewDepthId], m_iSkipFrame, m_pocLastDisplay[previousViewDepthId]);
      } 
      if( ( viewDepthId == 0 && (viewDepthId != previousViewDepthId) ) || m_tDecTop.size() == 1 )
      {
#if HHI_INTER_VIEW_RESIDUAL_PRED
        for( Int i = 0; i < m_tDecTop.size(); i++ )
        {
          m_tDecTop[i]->deleteExtraPicBuffers( uiPOC[i] );
        }
#endif
        for( Int i = 0; i < m_tDecTop.size(); i++ )
        {
          m_tDecTop[i]->compressMotion( uiPOC[i] );
        }
      }   
      if( !(m_iMaxTemporalLayer >= 0 && nalu.m_temporalId > m_iMaxTemporalLayer) )
      {
#if QC_MVHEVC_B0046
        if(viewDepthId && m_tDecTop[viewDepthId]->m_bFirstNal== false)
        {
          m_tDecTop[viewDepthId]->m_bFirstNal = true;
          ParameterSetManagerDecoder* pDecV0 = m_tDecTop[0]->xGetParaSetDec();
          m_tDecTop[viewDepthId]->xCopyVPS(pDecV0->getPrefetchedVPS(0));
          m_tDecTop[viewDepthId]->xCopySPS(pDecV0->getPrefetchedSPS(0));
          m_tDecTop[viewDepthId]->xCopyPPS(pDecV0->getPrefetchedPPS(0));
        }
#endif
        newPicture[viewDepthId] = m_tDecTop[viewDepthId]->decode(nalu, m_iSkipFrame, m_pocLastDisplay[viewDepthId]);
        if (newPicture[viewDepthId])
        {
          bitstreamFile.clear();
          /* location points to the current nalunit payload[1] due to the
           * need for the annexB parser to read three extra bytes.
           * [1] except for the first NAL unit in the file
           *     (but bNewPicture doesn't happen then) */
          bitstreamFile.seekg(location-streamoff(3));
          bytestream.reset();
        }
        if( nalu.isSlice() )
        {
          previousPictureDecoded = true;
#if FLEX_CODING_ORDER_M23723
        if (firstFrame)
        {
            if (depth)
            {
                iDepthViewIdx++;
                m_fcoOrder[viewDepthId]='D';
            }
            else
           {
                iTextureViewIdx++;
                m_fcoOrder[viewDepthId]='T';
           }
          }

#endif
        }
      }
    }
    if( ( (newPicture[viewDepthId] || !bitstreamFile) && m_tDecTop.size() == 1) || (!bitstreamFile && previousPictureDecoded == true) )  
    {
      m_tDecTop[viewDepthId]->executeDeblockAndAlf(uiPOC[viewDepthId], pcListPic[viewDepthId], m_iSkipFrame, m_pocLastDisplay[viewDepthId]);
    }
    if( pcListPic[viewDepthId] )
    {
#if QC_REM_IDV_B0046
      Int iviewId = m_tDecTop[viewDepthId]->getViewId();
      if( newPicture[viewDepthId] && (nalu.m_nalUnitType == NAL_UNIT_CODED_SLICE_IDR || ((nalu.m_nalUnitType == NAL_UNIT_CODED_SLICE_IDR && iviewId) && m_tDecTop[viewDepthId]->getNalUnitTypeBaseView() == NAL_UNIT_CODED_SLICE_IDR)) )
#else
      if( newPicture[viewDepthId] && (nalu.m_nalUnitType == NAL_UNIT_CODED_SLICE_IDR || (nalu.m_nalUnitType == NAL_UNIT_CODED_SLICE_IDV && m_tDecTop[viewDepthId]->getNalUnitTypeBaseView() == NAL_UNIT_CODED_SLICE_IDR)) )
#endif
      {
        xFlushOutput( pcListPic[viewDepthId], viewDepthId );
      }
      // write reconstruction to file
      if(newPicture[viewDepthId])
      {
#if H0567_DPB_PARAMETERS_PER_TEMPORAL_LAYER
        xWriteOutput( pcListPic[viewDepthId], viewDepthId, nalu.m_temporalId );
#else
        xWriteOutput( pcListPic[viewDepthId], viewDepthId );
#endif
      }
    }
    previousViewDepthId = viewDepthId;
  } 
  if( m_cCamParsCollector.isInitialized() )
  {
    m_cCamParsCollector.setSlice( 0 );
  }
  // last frame
  for( Int viewDepthIdx = 0; viewDepthIdx < m_tDecTop.size(); viewDepthIdx++ )
  {
    xFlushOutput( pcListPic[viewDepthIdx], viewDepthIdx );
  }  
  xDestroyDecLib();
}

// ====================================================================================================================
// Protected member functions
// ====================================================================================================================

Void TAppDecTop::xDestroyDecLib()
{

  for(Int viewDepthIdx=0; viewDepthIdx<m_tVideoIOYuvReconFile.size() ; viewDepthIdx++)
  {
    if( m_tVideoIOYuvReconFile[viewDepthIdx] )
    {
      m_tVideoIOYuvReconFile[viewDepthIdx]->close();
      delete m_tVideoIOYuvReconFile[viewDepthIdx]; 
      m_tVideoIOYuvReconFile[viewDepthIdx] = NULL ;
    }
  }

  for(Int viewDepthIdx=0; viewDepthIdx<m_tDecTop.size() ; viewDepthIdx++)
  {
    if( m_tDecTop[viewDepthIdx] )
    {
      if( !m_useDepth && (viewDepthIdx % 2 == 1) )
      {
      }
      else
      {
        m_tDecTop[viewDepthIdx]->deletePicBuffer();
        m_tDecTop[viewDepthIdx]->destroy() ;
      }
#if QC_MVHEVC_B0046
      if(viewDepthIdx)
      {
         //Call clear function to remove the record, which has been freed during viewDepthIdx = 0 case.
        m_tDecTop[viewDepthIdx]->xGetParaSetDec()->clearSPS();
        m_tDecTop[viewDepthIdx]->xGetParaSetDec()->clearVPS();
        m_tDecTop[viewDepthIdx]->xGetParaSetDec()->clearPPS();
      }
#endif
      delete m_tDecTop[viewDepthIdx] ; 
      m_tDecTop[viewDepthIdx] = NULL ;
    }
  }

  m_cCamParsCollector.uninit();
  if( m_pScaleOffsetFile ) 
  { 
    ::fclose( m_pScaleOffsetFile ); 
  }
}

#if H0567_DPB_PARAMETERS_PER_TEMPORAL_LAYER
Void TAppDecTop::xWriteOutput( TComList<TComPic*>* pcListPic, Int viewDepthId, UInt tId )
#else
Void TAppDecTop::xWriteOutput( TComList<TComPic*>* pcListPic, Int viewDepthId )
#endif
{
  TComList<TComPic*>::iterator iterPic   = pcListPic->begin();
  Int not_displayed = 0;

  while (iterPic != pcListPic->end())
  {
    TComPic* pcPic = *(iterPic);
    if(pcPic->getOutputMark() && pcPic->getPOC() > m_pocLastDisplay[viewDepthId])
    {
       not_displayed++;
    }
    iterPic++;
  }
  iterPic   = pcListPic->begin();
  
  while (iterPic != pcListPic->end())
  {
    TComPic* pcPic = *(iterPic);
#if PIC_CROPPING
    TComSPS *sps = pcPic->getSlice(0)->getSPS();
#endif
    
#if H0567_DPB_PARAMETERS_PER_TEMPORAL_LAYER
    if ( pcPic->getOutputMark() && (not_displayed >  pcPic->getSlice(0)->getSPS()->getNumReorderPics(tId) && pcPic->getPOC() > m_pocLastDisplay[viewDepthId]))
#else
    if ( pcPic->getOutputMark() && (not_displayed >  pcPic->getSlice(0)->getSPS()->getNumReorderFrames() && pcPic->getPOC() > m_pocLastDisplay[viewDepthId]))
#endif
    {
      // write to file
       not_displayed--;
      if ( m_pchReconFile )
      {
#if PIC_CROPPING
        m_tVideoIOYuvReconFile[viewDepthId]->write( pcPic->getPicYuvRec(), sps->getPicCropLeftOffset(), sps->getPicCropRightOffset(), sps->getPicCropTopOffset(), sps->getPicCropBottomOffset() );
#else
        m_tVideoIOYuvReconFile[viewDepthId]->write( pcPic->getPicYuvRec(), pcPic->getSlice(0)->getSPS()->getPad() );
#endif
      }
      
      // update POC of display order
      m_pocLastDisplay[viewDepthId] = pcPic->getPOC();
      
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
      pcPic->setOutputMark(false);
    }
    
    iterPic++;
  }
}

/** \param pcListPic list of pictures to be written to file
    \todo            DYN_REF_FREE should be revised
 */
Void TAppDecTop::xFlushOutput( TComList<TComPic*>* pcListPic, Int viewDepthId )
{
  if(!pcListPic)
  {
    return;
  } 
  TComList<TComPic*>::iterator iterPic   = pcListPic->begin();

  iterPic   = pcListPic->begin();
  
  while (iterPic != pcListPic->end())
  {
    TComPic* pcPic = *(iterPic);
#if PIC_CROPPING
    TComSPS *sps = pcPic->getSlice(0)->getSPS();
#endif

    if ( pcPic->getOutputMark() )
    {
      // write to file
      if ( m_pchReconFile )
      {
#if PIC_CROPPING
        m_tVideoIOYuvReconFile[viewDepthId]->write( pcPic->getPicYuvRec(), sps->getPicCropLeftOffset(), sps->getPicCropRightOffset(), sps->getPicCropTopOffset(), sps->getPicCropBottomOffset() );
#else
        m_tVideoIOYuvReconFile[viewDepthId]->write( pcPic->getPicYuvRec(), pcPic->getSlice(0)->getSPS()->getPad() );
#endif
      }
      
      // update POC of display order
      m_pocLastDisplay[viewDepthId] = pcPic->getPOC();
      
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
      pcPic->setOutputMark(false);
    }
    
    iterPic++;
  }
  pcListPic->clear();
  m_pocLastDisplay[viewDepthId] = -MAX_INT;
}
#if VIDYO_VPS_INTEGRATION|QC_MVHEVC_B0046
Void  TAppDecTop::increaseNumberOfViews  ( UInt layerId, UInt viewId, UInt isDepth )
#else
Void  TAppDecTop::increaseNumberOfViews  ( Int newNumberOfViewDepth )
#endif
{
#if VIDYO_VPS_INTEGRATION|QC_MVHEVC_B0046
  Int newNumberOfViewDepth = layerId + 1;
#endif
  if ( m_outputBitDepth == 0 )
  {
    m_outputBitDepth = g_uiBitDepth + g_uiBitIncrement;
  }
#if !VIDYO_VPS_INTEGRATION&!QC_MVHEVC_B0046
  Int viewId = (newNumberOfViewDepth-1)>>1;   // coding order T0D0T1D1T2D2
  Bool isDepth = ((newNumberOfViewDepth % 2) == 0);  // coding order T0D0T1D1T2D2
#endif
  if( isDepth )
    m_useDepth = true;

  if ( m_pchReconFile )
  { 
    while( m_tVideoIOYuvReconFile.size() < newNumberOfViewDepth)
    {
      m_tVideoIOYuvReconFile.push_back(new TVideoIOYuv);
      Char buffer[4];
#if VIDYO_VPS_INTEGRATION|QC_MVHEVC_B0046
      sprintf(buffer,"_%i", viewId );
#else
      sprintf(buffer,"_%i", (Int)(m_tVideoIOYuvReconFile.size()-1) / 2 );
#endif
      Char* nextFilename = NULL;
#if VIDYO_VPS_INTEGRATION|QC_MVHEVC_B0046
      if( isDepth)
#else
      if( (m_tVideoIOYuvReconFile.size() % 2) == 0 )
#endif
      {
        Char* pchTempFilename = NULL;
        xAppendToFileNameEnd( m_pchReconFile, "_depth", pchTempFilename);
        xAppendToFileNameEnd( pchTempFilename, buffer, nextFilename);
        free ( pchTempFilename );
      }
      else
      {
        xAppendToFileNameEnd( m_pchReconFile, buffer, nextFilename);
      }
#if !VIDYO_VPS_INTEGRATION&!QC_MVHEVC_B0046
      if( isDepth || ( !isDepth && (m_tVideoIOYuvReconFile.size() % 2) == 1 ) )
#endif
      {
        m_tVideoIOYuvReconFile.back()->open( nextFilename, true, m_outputBitDepth, g_uiBitDepth + g_uiBitIncrement );
      }
      free ( nextFilename );
    }
  }

  while( m_pocLastDisplay.size() < newNumberOfViewDepth )
  {
    m_pocLastDisplay.push_back(-MAX_INT+m_iSkipFrame);
  }
  while( m_tDecTop.size() < newNumberOfViewDepth)
  {
    m_tDecTop.push_back(new TDecTop);
#if !VIDYO_VPS_INTEGRATION&!QC_MVHEVC_B0046
    if( isDepth || ( !isDepth && (m_tVideoIOYuvReconFile.size() % 2) == 1 ) )
    {
#endif
      m_tDecTop.back()->create();
      m_tDecTop.back()->init( this, newNumberOfViewDepth == 1);
      m_tDecTop.back()->setViewId( viewId );
      m_tDecTop.back()->setIsDepth( isDepth );
      m_tDecTop.back()->setPictureDigestEnabled(m_pictureDigestEnabled);
      m_tDecTop.back()->setCamParsCollector( &m_cCamParsCollector );
#if !VIDYO_VPS_INTEGRATION&!QC_MVHEVC_B0046
    }
#endif
  }
}

TDecTop* TAppDecTop::getTDecTop( Int viewId, Bool isDepth )
{ 
#if FLEX_CODING_ORDER_M23723
  Int viewnumber=0;
  Int i=0;
  Bool fcoFlag=0;
  if (viewId>m_fcoViewDepthId)
  {
    return NULL;
  }
  else
  {
    if (isDepth)
   {
      for ( i=0; i<=m_fcoViewDepthId;i++)
      {
         if (m_fcoOrder[i]=='D')
         {
           if (viewnumber==viewId)
           {
             fcoFlag=1;
             break;
           }
           else
             viewnumber++;
         }
      }
    }
    else
    {
      for ( i=0; i<=m_fcoViewDepthId;i++)
      {
        if (m_fcoOrder[i]=='T')
        {
          if (viewnumber==viewId)
          {
            fcoFlag=1;
            break;
          }
          else
            viewnumber++;
        }
      }
    }
    if (fcoFlag)
    {
      return m_tDecTop[i];
    }
    else
      return NULL;
    
  }

    // coding order T0D0T1D1T2D2
#else
  return m_tDecTop[(isDepth ? 1 : 0) + viewId * 2];  // coding order T0D0T1D1T2D2
#endif
 
} 

std::vector<TComPic*> TAppDecTop::getInterViewRefPics( Int viewId, Int poc, Bool isDepth, TComSPS* sps )
{
  std::vector<TComPic*> apcRefPics( sps->getNumberOfUsableInterViewRefs(), (TComPic*)NULL );
  for( Int k = 0; k < sps->getNumberOfUsableInterViewRefs(); k++ )
  {
    TComPic* pcRefPic = xGetPicFromView( sps->getUsableInterViewRef( k ) + viewId, poc, isDepth );
    assert( pcRefPic != NULL );
    apcRefPics[k] = pcRefPic;
  }
  return apcRefPics;
}

TComPic* TAppDecTop::xGetPicFromView( Int viewId, Int poc, Bool isDepth )
{
  assert( ( viewId >= 0 ) );

#if FLEX_CODING_ORDER_M23723
if (getTDecTop(viewId,isDepth))
{
   TComList<TComPic*>* apcListPic = getTDecTop( viewId, isDepth )->getListPic();
   TComPic* pcPic = NULL;
   for( TComList<TComPic*>::iterator it=apcListPic->begin(); it!=apcListPic->end(); it++ )
   {
     if( (*it)->getPOC() == poc )
     {
       pcPic = *it;
       break;
     }
   }
   return pcPic;
}
else
  return NULL;
#else

  TComList<TComPic*>* apcListPic = getTDecTop( viewId, isDepth )->getListPic();
  TComPic* pcPic = NULL;
  for( TComList<TComPic*>::iterator it=apcListPic->begin(); it!=apcListPic->end(); it++ )
  {
    if( (*it)->getPOC() == poc )
    {
      pcPic = *it;
      break;
    }
  }
  return pcPic;
#endif
}
//! \}
