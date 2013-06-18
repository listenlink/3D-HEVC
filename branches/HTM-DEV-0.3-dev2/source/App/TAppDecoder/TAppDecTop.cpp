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
#if !H_MV
: m_iPOCLastDisplay(-MAX_INT)
#else
: m_numDecoders( 0 )
#endif
{
  ::memset (m_abDecFlag, 0, sizeof (m_abDecFlag));
#if H_MV
  for (Int i = 0; i < MAX_NUM_LAYER_IDS; i++) m_layerIdToDecIdx[i] = -1; 
#endif
#if H_3D
    m_pScaleOffsetFile  = 0;
#endif
}

Void TAppDecTop::create()
{
}

Void TAppDecTop::destroy()
{
  if (m_pchBitstreamFile)
  {
    free (m_pchBitstreamFile);
    m_pchBitstreamFile = NULL;
  }
#if H_MV
  for (Int decIdx = 0; decIdx < m_numDecoders; decIdx++)
  {
    if (m_pchReconFiles[decIdx])
    {
      free (m_pchReconFiles[decIdx]);
      m_pchReconFiles[decIdx] = NULL;
    }
  }
#endif
  if (m_pchReconFile)
  {
    free (m_pchReconFile);
    m_pchReconFile = NULL;
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
  Int                 poc;
#if H_MV
  poc = -1; 
#endif
  TComList<TComPic*>* pcListPic = NULL;

  ifstream bitstreamFile(m_pchBitstreamFile, ifstream::in | ifstream::binary);
  if (!bitstreamFile)
  {
    fprintf(stderr, "\nfailed to open bitstream file `%s' for reading\n", m_pchBitstreamFile);
    exit(EXIT_FAILURE);
  }

#if H_3D
  if( m_pchScaleOffsetFile ) 
  { 
    m_pScaleOffsetFile = ::fopen( m_pchScaleOffsetFile, "wt" ); 
    AOF( m_pScaleOffsetFile ); 
  }
  m_cCamParsCollector.init( m_pScaleOffsetFile );
#endif
  InputByteStream bytestream(bitstreamFile);

  // create & initialize internal classes
  xCreateDecLib();
  xInitDecLib  ();
#if !H_MV
  m_iPOCLastDisplay += m_iSkipFrame;      // set the last displayed POC correctly for skip forward.

  // main decoder loop
  Bool recon_opened = false; // reconstruction file not yet opened. (must be performed after SPS is seen)
#else
  Int  pocCurrPic        = -MAX_INT;     
  Int  pocLastPic        = -MAX_INT;   

  Int  layerIdCurrPic    = 0; 

  Int  decIdxLastPic     = 0; 
  Int  decIdxCurrPic     = 0; 

  Bool firstSlice        = true; 
#endif
 
  while (!!bitstreamFile)
  {
    /* location serves to work around a design fault in the decoder, whereby
     * the process of reading a new slice that is the first slice of a new frame
     * requires the TDecTop::decode() method to be called again with the same
     * nal unit. */
    streampos location = bitstreamFile.tellg();
#if H_MV
#if ENC_DEC_TRACE
    Int64 symCount = g_nSymbolCounter;
#endif
#endif
    AnnexBStats stats = AnnexBStats();
#if !H_MV
    Bool bPreviousPictureDecoded = false;
#endif

    vector<uint8_t> nalUnit;
    InputNALUnit nalu;
    byteStreamNALUnit(bytestream, nalUnit, stats);

    // call actual decoding function
    Bool bNewPicture = false;
#if H_MV
    Bool newSliceDiffPoc   = false;
    Bool newSliceDiffLayer = false;
    Bool allLayersDecoded  = false;     
#endif
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
#if H_MV      
      Int decIdx     = xGetDecoderIdx( nalu.m_layerId , true );
      
      if( (m_iMaxTemporalLayer >= 0 && nalu.m_temporalId > m_iMaxTemporalLayer) || !isNaluWithinTargetDecLayerIdSet(&nalu) )
      {
        bNewPicture = false;
      }
      else
      { 
        newSliceDiffLayer = nalu.isSlice() && ( nalu.m_layerId != layerIdCurrPic ) && !firstSlice;
        newSliceDiffPoc   = m_tDecTop[decIdx]->decode(nalu, m_iSkipFrame, m_pocLastDisplay[decIdx], newSliceDiffLayer );
        // decode function only returns true when all of the following conditions are true
        // - poc in particular layer changes
        // - nalu does not belong to first slice in layer
        // - nalu.isSlice() == true      

        bNewPicture       = newSliceDiffLayer || newSliceDiffPoc; 

        if ( nalu.isSlice() && firstSlice )
        {
          layerIdCurrPic = nalu.m_layerId; 
          pocCurrPic     = m_tDecTop[decIdx]->getCurrPoc(); 
          decIdxCurrPic  = decIdx; 
          firstSlice     = false; 
        }

        if ( bNewPicture || !bitstreamFile )
        { 
          layerIdCurrPic    = nalu.m_layerId; 
          
          pocLastPic        = pocCurrPic; 
          pocCurrPic        = m_tDecTop[decIdx]->getCurrPoc(); 
          
          decIdxLastPic     = decIdxCurrPic; 
          decIdxCurrPic     = decIdx; 

          allLayersDecoded = ( pocCurrPic != pocLastPic );
        }

        
#else
      if( (m_iMaxTemporalLayer >= 0 && nalu.m_temporalId > m_iMaxTemporalLayer) || !isNaluWithinTargetDecLayerIdSet(&nalu)  )
      {
        if(bPreviousPictureDecoded)
        {
          bNewPicture = true;
          bPreviousPictureDecoded = false;
        }
        else
        {
          bNewPicture = false;
        }
      }
      else
      {
        bNewPicture = m_cTDecTop.decode(nalu, m_iSkipFrame, m_iPOCLastDisplay);
#endif
        if (bNewPicture)
        {
          bitstreamFile.clear();
          /* location points to the current nalunit payload[1] due to the
           * need for the annexB parser to read three extra bytes.
           * [1] except for the first NAL unit in the file
           *     (but bNewPicture doesn't happen then) */
          bitstreamFile.seekg(location-streamoff(3));
          bytestream.reset();
#if H_MV
#if ENC_DEC_TRACE
          g_nSymbolCounter = symCount;
#endif
#endif
        }
#if !H_MV
        bPreviousPictureDecoded = true; 
#endif
      }
    }
    if (bNewPicture || !bitstreamFile)
    {
#if H_MV
      assert( decIdxLastPic != -1 ); 
      m_tDecTop[decIdxLastPic]->endPicDecoding(poc, pcListPic, m_targetDecLayerIdSet );
#else
      m_cTDecTop.executeLoopFilters(poc, pcListPic);
#endif
    }
#if H_3D
    if ( allLayersDecoded || !bitstreamFile )
    {
      for( Int dI = 0; dI < m_numDecoders; dI++ )
      {
        TComPic* picLastCoded = m_ivPicLists.getPic( m_tDecTop[dI]->getLayerId(), pocLastPic );
        assert( picLastCoded != NULL );        
        picLastCoded->compressMotion();         
      }
    }
#endif

    if( pcListPic )
    {
#if H_MV
      if ( m_pchReconFiles[decIdxLastPic] && !m_reconOpen[decIdxLastPic] )
#else
      if ( m_pchReconFile && !recon_opened )
#endif
      {
        if (!m_outputBitDepthY) { m_outputBitDepthY = g_bitDepthY; }
        if (!m_outputBitDepthC) { m_outputBitDepthC = g_bitDepthC; }

#if H_MV
        m_tVideoIOYuvReconFile[decIdxLastPic]->open( m_pchReconFiles[decIdxLastPic], true, m_outputBitDepthY, m_outputBitDepthC, g_bitDepthY, g_bitDepthC ); // write mode
        m_reconOpen[decIdxLastPic] = true;
      }
      if ( bNewPicture && newSliceDiffPoc && 
#else
        m_cTVideoIOYuvReconFile.open( m_pchReconFile, true, m_outputBitDepthY, m_outputBitDepthC, g_bitDepthY, g_bitDepthC ); // write mode
        recon_opened = true;
      }
      if ( bNewPicture && 
#endif
           (   nalu.m_nalUnitType == NAL_UNIT_CODED_SLICE_IDR_W_RADL
            || nalu.m_nalUnitType == NAL_UNIT_CODED_SLICE_IDR_N_LP
            || nalu.m_nalUnitType == NAL_UNIT_CODED_SLICE_BLA_N_LP
            || nalu.m_nalUnitType == NAL_UNIT_CODED_SLICE_BLA_W_RADL
            || nalu.m_nalUnitType == NAL_UNIT_CODED_SLICE_BLA_W_LP ) )
      {
#if H_MV
        xFlushOutput( pcListPic, decIdxLastPic );
#else
        xFlushOutput( pcListPic );
#endif
      }
      // write reconstruction to file
      if(bNewPicture)
      {
#if H_MV
        xWriteOutput( pcListPic, decIdxLastPic, nalu.m_temporalId );
      }
    }
  }

#if H_3D
  if( m_cCamParsCollector.isInitialized() )
  {
    m_cCamParsCollector.setSlice( 0 );
  }
#endif
  for(UInt decIdx = 0; decIdx < m_numDecoders; decIdx++)
  {
    xFlushOutput( m_tDecTop[decIdx]->getListPic(), decIdx );
  }
#else
        xWriteOutput( pcListPic, nalu.m_temporalId );
      }
    }
  }
  
  xFlushOutput( pcListPic );
  // delete buffers
  m_cTDecTop.deletePicBuffer();
#endif
     
  // destroy internal classes
  xDestroyDecLib();
}

// ====================================================================================================================
// Protected member functions
// ====================================================================================================================

Void TAppDecTop::xCreateDecLib()
{
#if H_MV
  // initialize global variables
  initROM();  
#else
  // create decoder class
  m_cTDecTop.create();
#endif
}

Void TAppDecTop::xDestroyDecLib()
{
#if H_MV
  // destroy ROM
  destroyROM();

  for(Int decIdx = 0; decIdx < m_numDecoders ; decIdx++)
  {
    if( m_tVideoIOYuvReconFile[decIdx] )
    {
      m_tVideoIOYuvReconFile[decIdx]->close();
      delete m_tVideoIOYuvReconFile[decIdx]; 
      m_tVideoIOYuvReconFile[decIdx] = NULL ;
    }

    if( m_tDecTop[decIdx] )
    {
      m_tDecTop[decIdx]->deletePicBuffer();
      m_tDecTop[decIdx]->destroy() ;
    }
    delete m_tDecTop[decIdx] ; 
    m_tDecTop[decIdx] = NULL ;
  }
#else
  if ( m_pchReconFile )
  {
    m_cTVideoIOYuvReconFile. close();
  }
  
  // destroy decoder class
  m_cTDecTop.destroy();
#endif
#if H_3D
  m_cCamParsCollector.uninit();
  if( m_pScaleOffsetFile ) 
  { 
    ::fclose( m_pScaleOffsetFile ); 
  }
#endif
}

Void TAppDecTop::xInitDecLib()
{
#if !H_MV
  // initialize decoder class
  m_cTDecTop.init();
  m_cTDecTop.setDecodedPictureHashSEIEnabled(m_decodedPictureHashSEIEnabled);
#endif
}

/** \param pcListPic list of pictures to be written to file
    \todo            DYN_REF_FREE should be revised
 */
#if H_MV
Void TAppDecTop::xWriteOutput( TComList<TComPic*>* pcListPic, Int decIdx, Int tId )
#else
Void TAppDecTop::xWriteOutput( TComList<TComPic*>* pcListPic, UInt tId )
#endif
{
  TComList<TComPic*>::iterator iterPic   = pcListPic->begin();
  Int not_displayed = 0;

  while (iterPic != pcListPic->end())
  {
    TComPic* pcPic = *(iterPic);
#if H_MV
    if(pcPic->getOutputMark() && pcPic->getPOC() > m_pocLastDisplay[decIdx])
#else
    if(pcPic->getOutputMark() && pcPic->getPOC() > m_iPOCLastDisplay)
#endif
    {
       not_displayed++;
    }
    iterPic++;
  }
  iterPic   = pcListPic->begin();
  
  while (iterPic != pcListPic->end())
  {
    TComPic* pcPic = *(iterPic);
    
#if H_MV
    if ( pcPic->getOutputMark() && (not_displayed >  pcPic->getNumReorderPics(tId) && pcPic->getPOC() > m_pocLastDisplay[decIdx]))
#else
    if ( pcPic->getOutputMark() && (not_displayed >  pcPic->getNumReorderPics(tId) && pcPic->getPOC() > m_iPOCLastDisplay))
#endif
    {
      // write to file
       not_displayed--;
#if H_MV
      if ( m_pchReconFiles[decIdx] )
#else
      if ( m_pchReconFile )
#endif
      {
        const Window &conf = pcPic->getConformanceWindow();
        const Window &defDisp = m_respectDefDispWindow ? pcPic->getDefDisplayWindow() : Window();
#if H_MV
        m_tVideoIOYuvReconFile[decIdx]->write( pcPic->getPicYuvRec(),
#else
        m_cTVideoIOYuvReconFile.write( pcPic->getPicYuvRec(),
#endif
                                       conf.getWindowLeftOffset() + defDisp.getWindowLeftOffset(),
                                       conf.getWindowRightOffset() + defDisp.getWindowRightOffset(),
                                       conf.getWindowTopOffset() + defDisp.getWindowTopOffset(),
                                       conf.getWindowBottomOffset() + defDisp.getWindowBottomOffset() );
      }
      
      // update POC of display order
#if H_MV
      m_pocLastDisplay[decIdx] = pcPic->getPOC();
#else
      m_iPOCLastDisplay = pcPic->getPOC();
#endif
      
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
#if H_MV
Void TAppDecTop::xFlushOutput( TComList<TComPic*>* pcListPic, Int decIdx )
#else
Void TAppDecTop::xFlushOutput( TComList<TComPic*>* pcListPic )
#endif
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

    if ( pcPic->getOutputMark() )
    {
      // write to file
#if H_MV
      if ( m_pchReconFiles[decIdx] )
#else
      if ( m_pchReconFile )
#endif
      {
        const Window &conf = pcPic->getConformanceWindow();
        const Window &defDisp = m_respectDefDispWindow ? pcPic->getDefDisplayWindow() : Window();
#if H_MV
        m_tVideoIOYuvReconFile[decIdx]->write( pcPic->getPicYuvRec(),
#else
        m_cTVideoIOYuvReconFile.write( pcPic->getPicYuvRec(),
#endif
                                       conf.getWindowLeftOffset() + defDisp.getWindowLeftOffset(),
                                       conf.getWindowRightOffset() + defDisp.getWindowRightOffset(),
                                       conf.getWindowTopOffset() + defDisp.getWindowTopOffset(),
                                       conf.getWindowBottomOffset() + defDisp.getWindowBottomOffset() );
      }
      
      // update POC of display order
#if H_MV
      m_pocLastDisplay[decIdx] = pcPic->getPOC();
#else
      m_iPOCLastDisplay = pcPic->getPOC();
#endif
      
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
#if !H_MV
#if !DYN_REF_FREE
    if(pcPic)
    {
      pcPic->destroy();
      delete pcPic;
      pcPic = NULL;
    }
#endif 
#endif
    iterPic++;
  }
#if H_MV
  m_pocLastDisplay[decIdx] = -MAX_INT;
#else
  pcListPic->clear();
  m_iPOCLastDisplay = -MAX_INT;
#endif
}

/** \param nalu Input nalu to check whether its LayerId is within targetDecLayerIdSet
 */
Bool TAppDecTop::isNaluWithinTargetDecLayerIdSet( InputNALUnit* nalu )
{
  if ( m_targetDecLayerIdSet.size() == 0 ) // By default, the set is empty, meaning all LayerIds are allowed
  {
    return true;
  }
  for (std::vector<Int>::iterator it = m_targetDecLayerIdSet.begin(); it != m_targetDecLayerIdSet.end(); it++)
  {
#if H_MV
    if ( nalu->m_layerId == (*it) )
#else
    if ( nalu->m_reservedZero6Bits == (*it) )
#endif
    {
      return true;
    }
  }
  return false;
}

#if H_MV
Int TAppDecTop::xGetDecoderIdx( Int layerId, Bool createFlag /*= false */ )
{
  Int decIdx = -1; 
  if ( m_layerIdToDecIdx[ layerId ] != -1 ) 
  {      
    decIdx = m_layerIdToDecIdx[ layerId ]; 
  }
  else
  {      
    assert ( createFlag ); 
    assert( m_numDecoders < MAX_NUM_LAYERS ); 

    decIdx = m_numDecoders; 

    // Init decoder
    m_tDecTop[ decIdx ] =  new TDecTop;
    m_tDecTop[ decIdx ]->create();
#if H_3D_IV_MERGE
    m_tDecTop[ decIdx ]->init(this );
#else
    m_tDecTop[ decIdx ]->init( );
#endif
    m_tDecTop[ decIdx ]->setLayerId( layerId );
    m_tDecTop[ decIdx ]->setDecodedPictureHashSEIEnabled(m_decodedPictureHashSEIEnabled);
    m_tDecTop[ decIdx ]->setIvPicLists( &m_ivPicLists ); 
#if H_3D
   m_tDecTop[ decIdx ]->setCamParsCollector( &m_cCamParsCollector );
#endif

    // append pic list of new decoder to PicLists 
    assert( m_ivPicLists.size() == m_numDecoders );
    m_ivPicLists.push_back( m_tDecTop[ decIdx ]->getListPic() );

    // create recon file related stuff      
    Char* pchTempFilename = NULL;
    if ( m_pchReconFile )
    {      
      Char buffer[4];      
      sprintf(buffer,"_%i", layerId );
      assert ( m_pchReconFile ); 
      xAppendToFileNameEnd( m_pchReconFile , buffer, pchTempFilename );
      assert( m_pchReconFiles.size() == m_numDecoders );
    }

    m_pchReconFiles.push_back( pchTempFilename );   

    m_tVideoIOYuvReconFile[ decIdx ] = new TVideoIOYuv;
    m_reconOpen           [ decIdx ] = false;

    // set others 
    m_pocLastDisplay      [ decIdx ] = -MAX_INT;
    m_layerIdToDecIdx     [ layerId ] = decIdx; 

    m_numDecoders++; 
  };
  return decIdx;
}
#endif
#if H_3D_IV_MERGE
TComPic* TAppDecTop::xGetPicFromView( Int viewIdx, Int poc, Bool isDepth )
{
  assert( ( viewIdx >= 0 ) );

  TComList<TComPic*>* apcListPic = m_tDecTop[ (isDepth ? 1 : 0) + viewIdx * 2 ]->getListPic();

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
#endif

//! \}
