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
#if RExt__DECODER_DEBUG_BIT_STATISTICS
#include "TLibCommon/TComCodingStatistics.h"
#endif

//! \ingroup TAppDecoder
//! \{

// ====================================================================================================================
// Constructor / destructor / initialization / destroy
// ====================================================================================================================

TAppDecTop::TAppDecTop()
#if !NH_MV
: m_iPOCLastDisplay(-MAX_INT)
#else
: m_numDecoders( 0 )
#endif
{
#if NH_MV
  for (Int i = 0; i < MAX_NUM_LAYER_IDS; i++) 
  {
    m_layerIdToDecIdx[i] = -1; 
    m_layerInitilizedFlags[i] = false; 
  }
#endif
#if NH_3D
    m_pScaleOffsetFile  = 0;
#endif

#if NH_MV
    m_markedForOutput = false; 
#endif

}

Void TAppDecTop::create()
{
}

Void TAppDecTop::destroy()
{
#if NH_MV
  // destroy internal classes
  xDestroyDecLib();
#endif

  if (m_pchBitstreamFile)
  {
    free (m_pchBitstreamFile);
    m_pchBitstreamFile = NULL;
  }
#if NH_MV
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
#if NH_3D
  if (m_pchScaleOffsetFile)
  {
    free (m_pchScaleOffsetFile);
    m_pchScaleOffsetFile = NULL; 
  }
#endif
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
#if NH_MV
  poc = -1; 
#endif
  TComList<TComPic*>* pcListPic = NULL;

  ifstream bitstreamFile(m_pchBitstreamFile, ifstream::in | ifstream::binary);
  if (!bitstreamFile)
  {
    fprintf(stderr, "\nfailed to open bitstream file `%s' for reading\n", m_pchBitstreamFile);
    exit(EXIT_FAILURE);
  }
#if NH_3D
  if( m_pchScaleOffsetFile ) 
  { 
    m_pScaleOffsetFile = ::fopen( m_pchScaleOffsetFile, "wt" ); 
    AOF( m_pScaleOffsetFile ); 
  }
#endif

  InputByteStream bytestream(bitstreamFile);

  if (!m_outputDecodedSEIMessagesFilename.empty() && m_outputDecodedSEIMessagesFilename!="-")
  {
    m_seiMessageFileStream.open(m_outputDecodedSEIMessagesFilename.c_str(), std::ios::out);
    if (!m_seiMessageFileStream.is_open() || !m_seiMessageFileStream.good())
    {
      fprintf(stderr, "\nUnable to open file `%s' for writing decoded SEI messages\n", m_outputDecodedSEIMessagesFilename.c_str());
      exit(EXIT_FAILURE);
    }
  }

  // create & initialize internal classes
  xCreateDecLib();
  xInitDecLib  ();
#if !NH_MV
  m_iPOCLastDisplay += m_iSkipFrame;      // set the last displayed POC correctly for skip forward.

  // main decoder loop
  Bool openedReconFile = false; // reconstruction file not yet opened. (must be performed after SPS is seen)
#else

  Int  pocCurrPic        = -MAX_INT;     
  Int  pocLastPic        = -MAX_INT;   

  Int  layerIdLastPic    = -MAX_INT; 
  Int  layerIdCurrPic    = 0; 

  Int  decIdxLastPic     = 0; 
  Int  decIdxCurrPic     = 0; 

  Bool firstSlice        = true; 
#endif
  Bool loopFiltered = false;

  while (!!bitstreamFile)
  {
    /* location serves to work around a design fault in the decoder, whereby
     * the process of reading a new slice that is the first slice of a new frame
     * requires the TDecTop::decode() method to be called again with the same
     * nal unit. */
#if RExt__DECODER_DEBUG_BIT_STATISTICS
    TComCodingStatistics::TComCodingStatisticsData backupStats(TComCodingStatistics::GetStatistics());
    streampos location = bitstreamFile.tellg() - streampos(bytestream.GetNumBufferedBytes());
#else
    streampos location = bitstreamFile.tellg();
#endif
#if NH_MV
#if ENC_DEC_TRACE
    Int64 symCount = g_nSymbolCounter;
#endif
#endif
    AnnexBStats stats = AnnexBStats();

    InputNALUnit nalu;
    byteStreamNALUnit(bytestream, nalu.getBitstream().getFifo(), stats);

    // call actual decoding function
    Bool bNewPicture = false;
#if NH_MV
    Bool newSliceDiffPoc   = false;
    Bool newSliceDiffLayer = false;
    Bool sliceSkippedFlag  = false; 
    Bool allLayersDecoded  = false;     
#endif
    if (nalu.getBitstream().getFifo().empty())
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
      read(nalu);
#if NH_MV      
      if( (m_iMaxTemporalLayer >= 0 && nalu.m_temporalId > m_iMaxTemporalLayer) 
          || !isNaluWithinTargetDecLayerIdSet(&nalu)
          || nalu.m_nuhLayerId > MAX_NUM_LAYER_IDS-1
          || (nalu.m_nalUnitType == NAL_UNIT_VPS && nalu.m_nuhLayerId > 0)           
          || (nalu.m_nalUnitType == NAL_UNIT_EOB && nalu.m_nuhLayerId > 0)              
         ) 
      {
        bNewPicture = false;
        if ( !bitstreamFile )
        {
          decIdxLastPic     = decIdxCurrPic; 
        }
      }
      else
      { 
        Int decIdx     = xGetDecoderIdx( nalu.m_nuhLayerId , true );      
        newSliceDiffLayer = nalu.isSlice() && ( nalu.m_nuhLayerId != layerIdCurrPic ) && !firstSlice;
        newSliceDiffPoc   = m_tDecTop[decIdx]->decode(nalu, m_iSkipFrame, m_pocLastDisplay[decIdx], newSliceDiffLayer, sliceSkippedFlag );
        // decode function only returns true when all of the following conditions are true
        // - poc in particular layer changes
        // - nalu does not belong to first slice in layer
        // - nalu.isSlice() == true      

        bNewPicture       = ( newSliceDiffLayer || newSliceDiffPoc ) && !sliceSkippedFlag; 
        if ( nalu.isSlice() && firstSlice && !sliceSkippedFlag )        
        {
          layerIdCurrPic = nalu.m_nuhLayerId; 
          pocCurrPic     = m_tDecTop[decIdx]->getCurrPoc(); 
          decIdxCurrPic  = decIdx; 
          firstSlice     = false; 

          /// Use VPS activated by the first slice to determine OLS
          m_vps = m_tDecTop[decIdx]->getActiveVPS( );
          if ( m_targetDecLayerIdSetFileEmpty )
          {            
            if ( m_targetOptLayerSetIdx == -1 )
            {
              m_targetOptLayerSetIdx = m_tDecTop[decIdx]->getTargetOlsIdx(); 
            }
            else
            {
              assert( m_tDecTop[decIdx]->getTargetOlsIdx() == m_targetOptLayerSetIdx );
            }

            if ( m_targetOptLayerSetIdx < 0 || m_targetOptLayerSetIdx >= m_vps->getNumOutputLayerSets() )
            {
              fprintf(stderr, "\ntarget output layer set index must be in the range of 0 to %d, inclusive \n", m_vps->getNumOutputLayerSets() - 1 );            
              exit(EXIT_FAILURE);
            }
            m_targetDecLayerIdSet = m_vps->getTargetDecLayerIdList( m_targetOptLayerSetIdx ); 
          }

          if (m_outputVpsInfo )
          {
            m_vps->printScalabilityId();
            m_vps->printLayerDependencies();
            m_vps->printLayerSets();
            m_vps->printPTL(); 
          }
        }        

        if ( bNewPicture || !bitstreamFile || nalu.m_nalUnitType == NAL_UNIT_EOS )
        { 
          layerIdLastPic    = layerIdCurrPic; 
          layerIdCurrPic    = nalu.m_nuhLayerId; 
          pocLastPic        = pocCurrPic; 
          pocCurrPic        = m_tDecTop[decIdx]->getCurrPoc(); 
          decIdxLastPic     = decIdxCurrPic; 
          decIdxCurrPic     = decIdx; 
          allLayersDecoded = ( pocCurrPic != pocLastPic ) && ( nalu.m_nalUnitType != NAL_UNIT_EOS );
        }
#else
      if( (m_iMaxTemporalLayer >= 0 && nalu.m_temporalId > m_iMaxTemporalLayer) || !isNaluWithinTargetDecLayerIdSet(&nalu)  )
      {
        bNewPicture = false;
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
#if RExt__DECODER_DEBUG_BIT_STATISTICS
          bitstreamFile.seekg(location);
          bytestream.reset();
          TComCodingStatistics::SetStatistics(backupStats);
#else
          bitstreamFile.seekg(location-streamoff(3));
          bytestream.reset();
#endif
#if H_MV_ENC_DEC_TRAC
#if ENC_DEC_TRACE
          const Bool resetCounter = false; 
          if ( resetCounter )
          {
            g_nSymbolCounter  = symCount; // Only reset counter SH becomes traced twice
          }
          else
          {
            g_disableHLSTrace = true;     // Tracing of second parsing of SH is not carried out
          }      
#endif
#endif
        }
      }
    }

    if ( (bNewPicture || !bitstreamFile || nalu.m_nalUnitType == NAL_UNIT_EOS) &&
#if NH_MV      
      !m_tDecTop[decIdxLastPic]->getFirstSliceInSequence () )
#else
      !m_cTDecTop.getFirstSliceInSequence () )
#endif

    {
      if (!loopFiltered || bitstreamFile)
      {
#if NH_MV
        assert( decIdxLastPic != -1 ); 
        m_tDecTop[decIdxLastPic]->endPicDecoding(poc, pcListPic, m_targetDecLayerIdSet );
        xMarkForOutput( allLayersDecoded, poc, layerIdLastPic ); 
#else
        m_cTDecTop.executeLoopFilters(poc, pcListPic);
#endif
      }
      loopFiltered = (nalu.m_nalUnitType == NAL_UNIT_EOS);
      if (nalu.m_nalUnitType == NAL_UNIT_EOS)
      {
#if NH_MV      
        m_tDecTop[decIdxLastPic]->setFirstSliceInSequence(true);
#else
        m_cTDecTop.setFirstSliceInSequence(true);
#endif
      }
    }
    else if ( (bNewPicture || !bitstreamFile || nalu.m_nalUnitType == NAL_UNIT_EOS ) &&
#if NH_MV      
              m_tDecTop[decIdxLastPic]->getFirstSliceInSequence () ) 
#else
              m_cTDecTop.getFirstSliceInSequence () ) 
#endif
    {
#if NH_MV      
      m_tDecTop[decIdxLastPic]->setFirstSliceInPicture (true);
#else
      m_cTDecTop.setFirstSliceInPicture (true);
#endif
    }

#if NH_3D
    if ( allLayersDecoded || !bitstreamFile )
    {
      for( Int dI = 0; dI < m_numDecoders; dI++ )
      {
        TComPic* picLastCoded = m_ivPicLists.getPic( m_tDecTop[dI]->getLayerId(), pocLastPic );
        assert( picLastCoded != NULL );        
        picLastCoded->compressMotion(1);
      }
    }
#endif

    if( pcListPic )
    {
#if NH_MV
      if ( m_pchReconFiles[decIdxLastPic] && !m_reconOpen[decIdxLastPic] )
#else
      if ( m_pchReconFile && !openedReconFile )
#endif
      {
        const BitDepths &bitDepths=pcListPic->front()->getPicSym()->getSPS().getBitDepths(); // use bit depths of first reconstructed picture.
        for (UInt channelType = 0; channelType < MAX_NUM_CHANNEL_TYPE; channelType++)
        {
          if (m_outputBitDepth[channelType] == 0)
          {
            m_outputBitDepth[channelType] = bitDepths.recon[channelType];
          }
        }
#if NH_MV
        m_tVideoIOYuvReconFile[decIdxLastPic]->open( m_pchReconFiles[decIdxLastPic], true, m_outputBitDepth, m_outputBitDepth, bitDepths.recon ); // write mode
        m_reconOpen[decIdxLastPic] = true;
      }
      // write reconstruction to file
      if( bNewPicture )
      {
        // Bumping after picture has been decoded
#if ENC_DEC_TRACE
        g_bJustDoIt = true;  
        writeToTraceFile( "Bumping after decoding \n", g_decTracePicOutput  );         
        g_bJustDoIt = false;  
#endif
        xWriteOutput( pcListPic, decIdxLastPic, nalu.m_temporalId );
      }
      if ( (bNewPicture || nalu.m_nalUnitType == NAL_UNIT_CODED_SLICE_CRA) && m_tDecTop[decIdxLastPic]->getNoOutputPriorPicsFlag() )
      {
        m_tDecTop[decIdxLastPic]->checkNoOutputPriorPics( pcListPic );
        m_tDecTop[decIdxLastPic]->setNoOutputPriorPicsFlag (false);
      }

      if ( bNewPicture && newSliceDiffPoc && 
#else
        m_cTVideoIOYuvReconFile.open( m_pchReconFile, true, m_outputBitDepth, m_outputBitDepth, bitDepths.recon ); // write mode
        openedReconFile = true;
      }
      // write reconstruction to file
      if( bNewPicture )
      {
        xWriteOutput( pcListPic, nalu.m_temporalId );
      }
      if ( (bNewPicture || nalu.m_nalUnitType == NAL_UNIT_CODED_SLICE_CRA) && m_cTDecTop.getNoOutputPriorPicsFlag() )
      {
        m_cTDecTop.checkNoOutputPriorPics( pcListPic );
        m_cTDecTop.setNoOutputPriorPicsFlag (false);
      }

      if ( bNewPicture &&
#endif
           (   nalu.m_nalUnitType == NAL_UNIT_CODED_SLICE_IDR_W_RADL
            || nalu.m_nalUnitType == NAL_UNIT_CODED_SLICE_IDR_N_LP
            || nalu.m_nalUnitType == NAL_UNIT_CODED_SLICE_BLA_N_LP
            || nalu.m_nalUnitType == NAL_UNIT_CODED_SLICE_BLA_W_RADL
            || nalu.m_nalUnitType == NAL_UNIT_CODED_SLICE_BLA_W_LP ) )
      {
#if NH_MV
        xFlushOutput( pcListPic, decIdxLastPic );
#else
        xFlushOutput( pcListPic );
#endif
      }
      if (nalu.m_nalUnitType == NAL_UNIT_EOS)
      {
#if NH_MV
        xWriteOutput( pcListPic, decIdxCurrPic, nalu.m_temporalId );
#else
        xWriteOutput( pcListPic, nalu.m_temporalId );
#endif
#if NH_MV
        m_tDecTop[decIdxCurrPic]->setFirstSliceInPicture (false);
#else
        m_cTDecTop.setFirstSliceInPicture (false);
#endif
      }
      // write reconstruction to file -- for additional bumping as defined in C.5.2.3
#if NH_MV
      // Above comment seems to be wrong
#endif
      if(!bNewPicture && nalu.m_nalUnitType >= NAL_UNIT_CODED_SLICE_TRAIL_N && nalu.m_nalUnitType <= NAL_UNIT_RESERVED_VCL31)
      {
#if NH_MV        
        // Bumping after reference picture set has been applied (here after first vcl nalu. 
#if ENC_DEC_TRACE
        g_bJustDoIt = true;  
        writeToTraceFile( "Bumping after reference picture set has been applied \n", g_decTracePicOutput  );         
        g_bJustDoIt = false;  
#endif

        xWriteOutput( m_tDecTop[decIdxCurrPic]->getListPic(), decIdxCurrPic, nalu.m_temporalId );
#else
        xWriteOutput( pcListPic, nalu.m_temporalId );
#endif
      }
    }
  }
#if NH_MV
#if NH_3D
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
  xFlushOutput( pcListPic );
  // delete buffers
  m_cTDecTop.deletePicBuffer();
  // destroy internal classes
  xDestroyDecLib();
#endif
}

// ====================================================================================================================
// Protected member functions
// ====================================================================================================================

Void TAppDecTop::xCreateDecLib()
{
#if NH_MV
  // initialize global variables
  initROM();  
#if NH_3D_DMM
  initWedgeLists();
#endif
#else
  // create decoder class
  m_cTDecTop.create();
#endif
}

Void TAppDecTop::xDestroyDecLib()
{
#if NH_MV
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
#if NH_3D
  m_cCamParsCollector.uninit();
  if( m_pScaleOffsetFile ) 
  { 
    ::fclose( m_pScaleOffsetFile ); 
  }
#endif
}

Void TAppDecTop::xInitDecLib()
{

#if NH_3D
  m_cCamParsCollector.setCodeScaleOffsetFile( m_pScaleOffsetFile );
#endif
#if !NH_MV
  // initialize decoder class
  m_cTDecTop.init();
  m_cTDecTop.setDecodedPictureHashSEIEnabled(m_decodedPictureHashSEIEnabled);
#if O0043_BEST_EFFORT_DECODING
  m_cTDecTop.setForceDecodeBitDepth(m_forceDecodeBitDepth);
#endif
  if (!m_outputDecodedSEIMessagesFilename.empty())
  {
    std::ostream &os=m_seiMessageFileStream.is_open() ? m_seiMessageFileStream : std::cout;
    m_cTDecTop.setDecodedSEIMessageOutputStream(&os);
  }
#endif
}

/** \param pcListPic list of pictures to be written to file
    \param tId       temporal sub-layer ID
 */
#if NH_MV
Void TAppDecTop::xWriteOutput( TComList<TComPic*>* pcListPic, Int decIdx, Int tId )
#else
Void TAppDecTop::xWriteOutput( TComList<TComPic*>* pcListPic, UInt tId )
#endif
{
  if (pcListPic->empty())
  {
    return;
  }

  TComList<TComPic*>::iterator iterPic   = pcListPic->begin();
  Int numPicsNotYetDisplayed = 0;
  Int dpbFullness = 0;
  const TComSPS* activeSPS = &(pcListPic->front()->getPicSym()->getSPS());

  UInt numReorderPicsHighestTid;
  UInt maxDecPicBufferingHighestTid;
  UInt maxNrSublayers = activeSPS->getMaxTLayers();

  if(m_iMaxTemporalLayer == -1 || m_iMaxTemporalLayer >= maxNrSublayers)
  {
    numReorderPicsHighestTid = activeSPS->getNumReorderPics(maxNrSublayers-1);
    maxDecPicBufferingHighestTid =  activeSPS->getMaxDecPicBuffering(maxNrSublayers-1); 
  }
  else
  {
    numReorderPicsHighestTid = activeSPS->getNumReorderPics(m_iMaxTemporalLayer);
    maxDecPicBufferingHighestTid = activeSPS->getMaxDecPicBuffering(m_iMaxTemporalLayer); 
  }

  while (iterPic != pcListPic->end())
  {
    TComPic* pcPic = *(iterPic);
#if NH_MV
    if(pcPic->getOutputMark() && pcPic->getPOC() > m_pocLastDisplay[decIdx])
#else
    if(pcPic->getOutputMark() && pcPic->getPOC() > m_iPOCLastDisplay)
#endif
    {
       numPicsNotYetDisplayed++;
      dpbFullness++;
    }
    else if(pcPic->getSlice( 0 )->isReferenced())
    {
      dpbFullness++;
    }
    iterPic++;
  }

  iterPic = pcListPic->begin();

  if (numPicsNotYetDisplayed>2)
  {
    iterPic++;
  }

  TComPic* pcPic = *(iterPic);
  if (numPicsNotYetDisplayed>2 && pcPic->isField()) //Field Decoding
  {
    TComList<TComPic*>::iterator endPic   = pcListPic->end();
    endPic--;
    iterPic   = pcListPic->begin();
    while (iterPic != endPic)
    {
      TComPic* pcPicTop = *(iterPic);
      iterPic++;
      TComPic* pcPicBottom = *(iterPic);

#if NH_MV
      if ( pcPicTop->getOutputMark() && pcPicBottom->getOutputMark() &&
        (numPicsNotYetDisplayed >  numReorderPicsHighestTid || dpbFullness > maxDecPicBufferingHighestTid) &&
        (!(pcPicTop->getPOC()%2) && pcPicBottom->getPOC() == pcPicTop->getPOC()+1) &&
        (pcPicTop->getPOC() == m_pocLastDisplay[decIdx]+1 || m_pocLastDisplay[decIdx] < 0))
#else
      if ( pcPicTop->getOutputMark() && pcPicBottom->getOutputMark() &&
          (numPicsNotYetDisplayed >  numReorderPicsHighestTid || dpbFullness > maxDecPicBufferingHighestTid) &&
          (!(pcPicTop->getPOC()%2) && pcPicBottom->getPOC() == pcPicTop->getPOC()+1) &&
          (pcPicTop->getPOC() == m_iPOCLastDisplay+1 || m_iPOCLastDisplay < 0))
#endif
      {
        // write to file
        numPicsNotYetDisplayed = numPicsNotYetDisplayed-2;
#if NH_MV
      if ( m_pchReconFiles[decIdx] )
#else
        if ( m_pchReconFile )
#endif
        {
          const Window &conf = pcPicTop->getConformanceWindow();
          const Window  defDisp = m_respectDefDispWindow ? pcPicTop->getDefDisplayWindow() : Window();
          const Bool isTff = pcPicTop->isTopField();

          Bool display = true;
          if( m_decodedNoDisplaySEIEnabled )
          {
            SEIMessages noDisplay = getSeisByType(pcPic->getSEIs(), SEI::NO_DISPLAY );
            const SEINoDisplay *nd = ( noDisplay.size() > 0 ) ? (SEINoDisplay*) *(noDisplay.begin()) : NULL;
            if( (nd != NULL) && nd->m_noDisplay )
            {
              display = false;
            }
          }

          if (display)
          {
#if NH_MV
        assert( conf   .getScaledFlag() );
        assert( defDisp.getScaledFlag() );
#if ENC_DEC_TRACE 
        g_bJustDoIt = true;  
        writeToTraceFile( "OutputPic Poc"   , pcPic->getPOC    (), g_decTracePicOutput  ); 
        writeToTraceFile( "OutputPic LayerId", pcPic->getLayerId(), g_decTracePicOutput );         
        g_bJustDoIt = false;  
#endif
        m_tVideoIOYuvReconFile[decIdx]->write( pcPicTop->getPicYuvRec(), pcPicBottom->getPicYuvRec(),
#else
        m_cTVideoIOYuvReconFile.write( pcPicTop->getPicYuvRec(), pcPicBottom->getPicYuvRec(),
#endif
                                           m_outputColourSpaceConvert,
                                           conf.getWindowLeftOffset() + defDisp.getWindowLeftOffset(),
                                           conf.getWindowRightOffset() + defDisp.getWindowRightOffset(),
                                           conf.getWindowTopOffset() + defDisp.getWindowTopOffset(),
#if NH_3D
                                           conf.getWindowBottomOffset() + defDisp.getWindowBottomOffset(), m_depth420OutputFlag && pcPicTop->getIsDepth() ? CHROMA_420 : NUM_CHROMA_FORMAT, isTff );
#else
                                           conf.getWindowBottomOffset() + defDisp.getWindowBottomOffset(), NUM_CHROMA_FORMAT, isTff );
#endif
          }
        }

        // update POC of display order
#if NH_MV
        m_pocLastDisplay[decIdx] = pcPic->getPOC();
#else
        m_iPOCLastDisplay = pcPicBottom->getPOC();
#endif

        // erase non-referenced picture in the reference picture list after display
        if ( !pcPicTop->getSlice(0)->isReferenced() && pcPicTop->getReconMark() == true )
        {
          pcPicTop->setReconMark(false);

          // mark it should be extended later
          pcPicTop->getPicYuvRec()->setBorderExtension( false );
        }
        if ( !pcPicBottom->getSlice(0)->isReferenced() && pcPicBottom->getReconMark() == true )
        {
          pcPicBottom->setReconMark(false);

          // mark it should be extended later
          pcPicBottom->getPicYuvRec()->setBorderExtension( false );
        }
        pcPicTop->setOutputMark(false);
        pcPicBottom->setOutputMark(false);
      }
    }
  }
  else if (!pcPic->isField()) //Frame Decoding
  {
    iterPic = pcListPic->begin();

    while (iterPic != pcListPic->end())
    {
      pcPic = *(iterPic);

#if NH_MV
      if(pcPic->getOutputMark() && pcPic->getPOC() > m_pocLastDisplay[decIdx] &&
        (numPicsNotYetDisplayed >  numReorderPicsHighestTid || dpbFullness > maxDecPicBufferingHighestTid))
#else      
      if(pcPic->getOutputMark() && pcPic->getPOC() > m_iPOCLastDisplay &&
        (numPicsNotYetDisplayed >  numReorderPicsHighestTid || dpbFullness > maxDecPicBufferingHighestTid))
#endif
      {
        // write to file
         numPicsNotYetDisplayed--;
        if(pcPic->getSlice(0)->isReferenced() == false)
        {
          dpbFullness--;
        }
#if NH_MV
      if ( m_pchReconFiles[decIdx] )
#else
        if ( m_pchReconFile )
#endif
        {
          const Window &conf    = pcPic->getConformanceWindow();
          const Window defDisp = m_respectDefDispWindow ? pcPic->getDefDisplayWindow() : Window();
#if NH_MV
        assert( conf   .getScaledFlag() );
        assert( defDisp.getScaledFlag() );
#if ENC_DEC_TRACE
        g_bJustDoIt = true;  
        writeToTraceFile( "OutputPic Poc"   , pcPic->getPOC    (), g_decTracePicOutput  ); 
        writeToTraceFile( "OutputPic LayerId", pcPic->getLayerId(), g_decTracePicOutput );         
        g_bJustDoIt = false; 
#endif
        m_tVideoIOYuvReconFile[decIdx]->write( pcPic->getPicYuvRec(),
#else
          m_cTVideoIOYuvReconFile.write( pcPic->getPicYuvRec(),
#endif
                                         m_outputColourSpaceConvert,
                                         conf.getWindowLeftOffset() + defDisp.getWindowLeftOffset(),
                                         conf.getWindowRightOffset() + defDisp.getWindowRightOffset(),
                                         conf.getWindowTopOffset() + defDisp.getWindowTopOffset(),
                                         conf.getWindowBottomOffset() + defDisp.getWindowBottomOffset(), 
#if NH_3D
                                          m_depth420OutputFlag && pcPic->getIsDepth() ? CHROMA_420 : NUM_CHROMA_FORMAT,
#else
                                          NUM_CHROMA_FORMAT,
#endif
           m_bClipOutputVideoToRec709Range   );
        }

        // update POC of display order
#if NH_MV
        m_pocLastDisplay[decIdx] = pcPic->getPOC();
#else
        m_iPOCLastDisplay = pcPic->getPOC();
#endif

        // erase non-referenced picture in the reference picture list after display
        if ( !pcPic->getSlice(0)->isReferenced() && pcPic->getReconMark() == true )
        {
          pcPic->setReconMark(false);

          // mark it should be extended later
          pcPic->getPicYuvRec()->setBorderExtension( false );
        }
        pcPic->setOutputMark(false);
#if NH_MV
        pcPic->setPicOutputFlag(false);
#endif
      }

      iterPic++;
    }
  }
}

/** \param pcListPic list of pictures to be written to file
 */
#if NH_MV
Void TAppDecTop::xFlushOutput( TComList<TComPic*>* pcListPic, Int decIdx )
#else
Void TAppDecTop::xFlushOutput( TComList<TComPic*>* pcListPic )
#endif
{
  if(!pcListPic || pcListPic->empty())
  {
    return;
  }
  TComList<TComPic*>::iterator iterPic   = pcListPic->begin();

  iterPic   = pcListPic->begin();
  TComPic* pcPic = *(iterPic);

  if (pcPic->isField()) //Field Decoding
  {
    TComList<TComPic*>::iterator endPic   = pcListPic->end();
    endPic--;
    TComPic *pcPicTop, *pcPicBottom = NULL;
    while (iterPic != endPic)
    {
      pcPicTop = *(iterPic);
      iterPic++;
      pcPicBottom = *(iterPic);

      if ( pcPicTop->getOutputMark() && pcPicBottom->getOutputMark() && !(pcPicTop->getPOC()%2) && (pcPicBottom->getPOC() == pcPicTop->getPOC()+1) )
      {
        // write to file
#if NH_MV
      if ( m_pchReconFiles[decIdx] )
#else
        if ( m_pchReconFile )
#endif
        {
          const Window &conf = pcPicTop->getConformanceWindow();
          const Window  defDisp = m_respectDefDispWindow ? pcPicTop->getDefDisplayWindow() : Window();
          const Bool isTff = pcPicTop->isTopField();
#if NH_MV
        assert( conf   .getScaledFlag() );
        assert( defDisp.getScaledFlag() );
#if ENC_DEC_TRACE
        g_bJustDoIt = true;  
        writeToTraceFile( "OutputPic Poc"   , pcPic->getPOC    (), g_decTracePicOutput  ); 
        writeToTraceFile( "OutputPic LayerId", pcPic->getLayerId(), g_decTracePicOutput );         
        g_bJustDoIt = false;  
#endif
        m_tVideoIOYuvReconFile[decIdx]->write( pcPicTop->getPicYuvRec(), pcPicBottom->getPicYuvRec(),
#else
          m_cTVideoIOYuvReconFile.write( pcPicTop->getPicYuvRec(), pcPicBottom->getPicYuvRec(),
#endif
                                         m_outputColourSpaceConvert,
                                         conf.getWindowLeftOffset() + defDisp.getWindowLeftOffset(),
                                         conf.getWindowRightOffset() + defDisp.getWindowRightOffset(),
                                         conf.getWindowTopOffset() + defDisp.getWindowTopOffset(),
#if NH_3D
                                         conf.getWindowBottomOffset() + defDisp.getWindowBottomOffset(), m_depth420OutputFlag && pcPicTop->getIsDepth() ? CHROMA_420 : NUM_CHROMA_FORMAT, isTff );
#else
                                         conf.getWindowBottomOffset() + defDisp.getWindowBottomOffset(), NUM_CHROMA_FORMAT, isTff );
#endif
        }

        // update POC of display order
#if NH_MV
      m_pocLastDisplay[decIdx] = pcPic->getPOC();
#else
        m_iPOCLastDisplay = pcPicBottom->getPOC();
#endif        
        // erase non-referenced picture in the reference picture list after display
        if ( !pcPicTop->getSlice(0)->isReferenced() && pcPicTop->getReconMark() == true )
        {
          pcPicTop->setReconMark(false);

          // mark it should be extended later
          pcPicTop->getPicYuvRec()->setBorderExtension( false );
        }
        if ( !pcPicBottom->getSlice(0)->isReferenced() && pcPicBottom->getReconMark() == true )
        {
          pcPicBottom->setReconMark(false);

          // mark it should be extended later
          pcPicBottom->getPicYuvRec()->setBorderExtension( false );
        }
        pcPicTop->setOutputMark(false);
        pcPicBottom->setOutputMark(false);

        if(pcPicTop)
        {
          pcPicTop->destroy();
          delete pcPicTop;
          pcPicTop = NULL;
        }
      }
    }
    if(pcPicBottom)
    {
      pcPicBottom->destroy();
      delete pcPicBottom;
      pcPicBottom = NULL;
    }
  }
  else //Frame decoding
  {
    while (iterPic != pcListPic->end())
    {
      pcPic = *(iterPic);

      if ( pcPic->getOutputMark() )
      {
        // write to file
#if NH_MV
      if ( m_pchReconFiles[decIdx] )
#else
        if ( m_pchReconFile )
#endif
        {
          const Window &conf    = pcPic->getConformanceWindow();
          const Window  defDisp = m_respectDefDispWindow ? pcPic->getDefDisplayWindow() : Window();
#if NH_MV
        assert( conf   .getScaledFlag() );
        assert( defDisp.getScaledFlag() );
#if ENC_DEC_TRACE
        g_bJustDoIt = true;  
        writeToTraceFile( "OutputPic Poc"   , pcPic->getPOC    (), g_decTracePicOutput  ); 
        writeToTraceFile( "OutputPic LayerId", pcPic->getLayerId(), g_decTracePicOutput );         
        g_bJustDoIt = false;  
#endif
        m_tVideoIOYuvReconFile[decIdx]->write( pcPic->getPicYuvRec(),
#else
          m_cTVideoIOYuvReconFile.write( pcPic->getPicYuvRec(),
#endif
                                         m_outputColourSpaceConvert,
                                         conf.getWindowLeftOffset() + defDisp.getWindowLeftOffset(),
                                         conf.getWindowRightOffset() + defDisp.getWindowRightOffset(),
                                         conf.getWindowTopOffset() + defDisp.getWindowTopOffset(),
                                         conf.getWindowBottomOffset() + defDisp.getWindowBottomOffset(), 
#if NH_3D
                                         m_depth420OutputFlag && pcPic->getIsDepth() ? CHROMA_420 : NUM_CHROMA_FORMAT
#else
                                         NUM_CHROMA_FORMAT
#endif
                                         , m_bClipOutputVideoToRec709Range);
        }

        // update POC of display order
#if NH_MV
      m_pocLastDisplay[decIdx] = pcPic->getPOC();
#else
        m_iPOCLastDisplay = pcPic->getPOC();
#endif

        // erase non-referenced picture in the reference picture list after display
        if ( !pcPic->getSlice(0)->isReferenced() && pcPic->getReconMark() == true )
        {
          pcPic->setReconMark(false);

          // mark it should be extended later
          pcPic->getPicYuvRec()->setBorderExtension( false );
        }
        pcPic->setOutputMark(false);
#if NH_MV
        pcPic->setPicOutputFlag(false);
#endif
      }
#if !NH_MV
      if(pcPic != NULL)
      {
        pcPic->destroy();
        delete pcPic;
        pcPic = NULL;
      }
#endif
      iterPic++;
    }
  }
#if NH_MV
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
#if NH_MV
    if ( nalu->m_nuhLayerId == (*it) )
#else
    if ( nalu->m_nuhLayerId == (*it) )
#endif
    {
      return true;
    }
  }
  return false;
}

#if NH_MV
Int TAppDecTop::xGetDecoderIdx( Int layerId, Bool createFlag /*= false */ )
{
  Int decIdx = -1; 

  if ( layerId > MAX_NUM_LAYER_IDS-1 )  
  {
    return decIdx; 
  }

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
    m_tDecTop[ decIdx ]->init( );
    m_tDecTop[ decIdx ]->setLayerId( layerId );
    m_tDecTop[ decIdx ]->setDecodedPictureHashSEIEnabled(m_decodedPictureHashSEIEnabled);
    m_tDecTop[ decIdx ]->setIvPicLists( &m_ivPicLists ); 
    m_tDecTop[ decIdx ]->setLayerInitilizedFlags( m_layerInitilizedFlags );
    m_tDecTop[ decIdx ]->setTargetOlsIdx( m_targetOptLayerSetIdx );    
#if O0043_BEST_EFFORT_DECODING
    m_cTDecTop[ decIdx ]->setForceDecodeBitDepth(m_forceDecodeBitDepth);
#endif
    if (!m_outputDecodedSEIMessagesFilename.empty())
    {
      std::ostream &os=m_seiMessageFileStream.is_open() ? m_seiMessageFileStream : std::cout;
      m_tDecTop[ decIdx ]->setDecodedSEIMessageOutputStream(&os);
    }
#if NH_3D
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

Void TAppDecTop::xMarkForOutput( Bool allLayersDecoded, Int pocLastPic, Int layerIdLastPic )
{  
  vector<Int> targetOptLayerIdList = m_vps->getTargetOptLayerIdList( m_targetOptLayerSetIdx );

  if (m_vps->getAltOutputLayerFlagVar( m_targetOptLayerSetIdx ) )
  {
    assert( targetOptLayerIdList.size() == 1 ); 
    Int targetLayerId = targetOptLayerIdList[0];     

    TComPic* curPic = m_ivPicLists.getPic( layerIdLastPic, pocLastPic );
    assert( curPic != NULL );

    if ( layerIdLastPic == targetLayerId )
    {
      if ( curPic->getPicOutputFlag() )
      {
        curPic->setOutputMark( true );
      }
      else
      {        
        xMarkAltOutPic( targetLayerId, pocLastPic ); 
      }
      m_markedForOutput = true; 
    }
    else if ( ( layerIdLastPic > targetLayerId || allLayersDecoded ) && !m_markedForOutput )
    {
      xMarkAltOutPic( targetLayerId, pocLastPic );
    }

    if ( allLayersDecoded )
    {
      m_markedForOutput = false; 
    }
  }
  else
  { 
    for( Int dI = 0; dI < m_numDecoders; dI++ )
    {      
      Int layerId = m_tDecTop[dI]->getLayerId(); 
      TComPic* curPic = m_ivPicLists.getPic( layerId, pocLastPic );
      if ( curPic != NULL )
      {
        if ( curPic->getReconMark() )
        {
          Bool isTargetOptLayer = std::find(targetOptLayerIdList.begin(), targetOptLayerIdList.end(), layerId) != targetOptLayerIdList.end();
          curPic->setOutputMark( isTargetOptLayer ? curPic->getPicOutputFlag() : false ); 
        }
      }
    }
  }
}

Void TAppDecTop::xMarkAltOutPic( Int targetOutputLayer, Int pocLastPic )
{
  Int optLayerIdxInVps = m_vps->getLayerIdInNuh( targetOutputLayer ); 
  Int highestNuhLayerId = -1; 
  TComPic* picWithHighestNuhLayerId = NULL; 
  for (Int dIdx = 0; dIdx < m_numDecoders; dIdx++)
  {
    Int curLayerId = m_tDecTop[dIdx]->getLayerId();
    Int curLayerIdxInVps = m_vps->getLayerIdInNuh( curLayerId  ); 
    if ( m_vps->getDependencyFlag(optLayerIdxInVps, curLayerIdxInVps ) )
    {
      TComPic* curPic = m_ivPicLists.getPic( curLayerId, pocLastPic ); 
      if (curPic != NULL)
      {
        if (curPic->getReconMark() && curPic->getPicOutputFlag() )
        {
          curPic->setOutputMark   ( false ); 
          curPic->setPicOutputFlag( false ); 
          if ( curLayerId > highestNuhLayerId)
          {
            highestNuhLayerId = curLayerId ; 
            picWithHighestNuhLayerId = curPic; 
          }            
        }
      }
    }
  }
  if ( picWithHighestNuhLayerId != NULL )
  {
    picWithHighestNuhLayerId->setPicOutputFlag(true); 
    picWithHighestNuhLayerId->setOutputMark   (true); 
  }
}

#endif
//! \}
