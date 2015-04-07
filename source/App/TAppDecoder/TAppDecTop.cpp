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
#if H_MV
  for (Int i = 0; i < MAX_NUM_LAYER_IDS; i++) 
  {
    m_layerIdToDecIdx[i] = -1; 
    m_layerInitilizedFlags[i] = false; 
  }
#endif
#if H_3D
    m_pScaleOffsetFile  = 0;
#endif

#if H_MV
    m_markedForOutput = false; 
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
#if H_3D
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
#endif
  InputByteStream bytestream(bitstreamFile);

  // create & initialize internal classes
  xCreateDecLib();
  xInitDecLib  ();
#if !H_MV
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
  Bool loopFiltered      = false;

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
    vector<uint8_t> nalUnit;
    InputNALUnit nalu;
    byteStreamNALUnit(bytestream, nalUnit, stats);

    // call actual decoding function
    Bool bNewPicture = false;
#if H_MV
    Bool newSliceDiffPoc   = false;
    Bool newSliceDiffLayer = false;
    Bool sliceSkippedFlag  = false; 
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
      if( (m_iMaxTemporalLayer >= 0 && nalu.m_temporalId > m_iMaxTemporalLayer) 
          || !isNaluWithinTargetDecLayerIdSet(&nalu)
          || nalu.m_layerId > MAX_NUM_LAYER_IDS-1
          || (nalu.m_nalUnitType == NAL_UNIT_VPS && nalu.m_layerId > 0)           
          || (nalu.m_nalUnitType == NAL_UNIT_EOB && nalu.m_layerId > 0)    
          || (nalu.m_nalUnitType == NAL_UNIT_EOS && nalu.m_layerId > 0)    
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
        Int decIdx     = xGetDecoderIdx( nalu.m_layerId , true );      
        newSliceDiffLayer = nalu.isSlice() && ( nalu.m_layerId != layerIdCurrPic ) && !firstSlice;
        newSliceDiffPoc   = m_tDecTop[decIdx]->decode(nalu, m_iSkipFrame, m_pocLastDisplay[decIdx], newSliceDiffLayer, sliceSkippedFlag );
        // decode function only returns true when all of the following conditions are true
        // - poc in particular layer changes
        // - nalu does not belong to first slice in layer
        // - nalu.isSlice() == true      

        if ( nalu.m_nalUnitType == NAL_UNIT_VPS )
        {
          m_vps = m_tDecTop[decIdx]->getPrefetchedVPS(); 
          if ( m_targetDecLayerIdSetFileEmpty )
          {
            TComVPS* vps = m_vps; 
            if ( m_targetOptLayerSetIdx == -1 )
            {
              // Not normative! Corresponds to specification by "External Means". (Should be set equal to 0, when no external means available. ) 
              m_targetOptLayerSetIdx = vps->getVpsNumLayerSetsMinus1(); 
            }

            for (Int dI = 0; dI < m_numDecoders; dI++ )
            {
              m_tDecTop[decIdx]->setTargetOptLayerSetIdx( m_targetOptLayerSetIdx ); 
#if H_3D_ANNEX_SELECTION_FIX
              m_tDecTop[decIdx]->setProfileIdc( ); 
#endif
            }

            if ( m_targetOptLayerSetIdx < 0 || m_targetOptLayerSetIdx >= vps->getNumOutputLayerSets() )
            {
              fprintf(stderr, "\ntarget output layer set index must be in the range of 0 to %d, inclusive \n", vps->getNumOutputLayerSets() - 1 );            
              exit(EXIT_FAILURE);
            }
            m_targetDecLayerIdSet = vps->getTargetDecLayerIdList( m_targetOptLayerSetIdx ); 
          }
          if (m_outputVpsInfo )
          {
            m_vps->printScalabilityId();
            m_vps->printLayerDependencies();
            m_vps->printLayerSets();
            m_vps->printPTL(); 
          }
        }
#if H_3D
        if (nalu.m_nalUnitType == NAL_UNIT_VPS )
        {                  
          m_cCamParsCollector.init( m_pScaleOffsetFile, m_tDecTop[decIdx]->getPrefetchedVPS() );
        }       
#endif
        bNewPicture       = ( newSliceDiffLayer || newSliceDiffPoc ) && !sliceSkippedFlag; 
        if ( nalu.isSlice() && firstSlice && !sliceSkippedFlag )        
        {
          layerIdCurrPic = nalu.m_layerId; 
          pocCurrPic     = m_tDecTop[decIdx]->getCurrPoc(); 
          decIdxCurrPic  = decIdx; 
          firstSlice     = false; 
        }

        if ( bNewPicture || !bitstreamFile )
        { 
          layerIdLastPic    = layerIdCurrPic; 
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
          bitstreamFile.seekg(location-streamoff(3));
          bytestream.reset();
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
    if (bNewPicture || !bitstreamFile || nalu.m_nalUnitType == NAL_UNIT_EOS )
    {
      if (!loopFiltered || bitstreamFile)
      {
#if H_MV
        assert( decIdxLastPic != -1 ); 
        m_tDecTop[decIdxLastPic]->endPicDecoding(poc, pcListPic, m_targetDecLayerIdSet );
        xMarkForOutput( allLayersDecoded, poc, layerIdLastPic ); 
#else
        m_cTDecTop.executeLoopFilters(poc, pcListPic);
#endif
      }
      loopFiltered = (nalu.m_nalUnitType == NAL_UNIT_EOS);
    }
#if !FIX_WRITING_OUTPUT
#if SETTING_NO_OUT_PIC_PRIOR
    if (bNewPicture && m_cTDecTop.getIsNoOutputPriorPics())
    {
      m_cTDecTop.checkNoOutputPriorPics( pcListPic );
    }
#endif
#endif
#if H_3D
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
#if H_MV
      if ( m_pchReconFiles[decIdxLastPic] && !m_reconOpen[decIdxLastPic] )
#else
      if ( m_pchReconFile && !openedReconFile  )
#endif
      {
        if (!m_outputBitDepthY) { m_outputBitDepthY = g_bitDepthY; }
        if (!m_outputBitDepthC) { m_outputBitDepthC = g_bitDepthC; }

#if H_MV
        m_tVideoIOYuvReconFile[decIdxLastPic]->open( m_pchReconFiles[decIdxLastPic], true, m_outputBitDepthY, m_outputBitDepthC, g_bitDepthY, g_bitDepthC ); // write mode
        m_reconOpen[decIdxLastPic] = true;
      }
#if FIX_WRITING_OUTPUT
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
#if SETTING_NO_OUT_PIC_PRIOR
      if ( (bNewPicture || nalu.m_nalUnitType == NAL_UNIT_CODED_SLICE_CRA) && m_tDecTop[decIdxLastPic]->getNoOutputPriorPicsFlag() )
      {
        m_tDecTop[decIdxLastPic]->checkNoOutputPriorPics( pcListPic );
        m_tDecTop[decIdxLastPic]->setNoOutputPriorPicsFlag (false);
      }
#endif
#endif
      if ( bNewPicture && newSliceDiffPoc && 
#else
        m_cTVideoIOYuvReconFile.open( m_pchReconFile, true, m_outputBitDepthY, m_outputBitDepthC, g_bitDepthY, g_bitDepthC ); // write mode
        openedReconFile  = true;
      }
#if FIX_WRITING_OUTPUT
      // write reconstruction to file
      if( bNewPicture )
      {
        xWriteOutput( pcListPic, nalu.m_temporalId );
      }
#if SETTING_NO_OUT_PIC_PRIOR
      if ( (bNewPicture || nalu.m_nalUnitType == NAL_UNIT_CODED_SLICE_CRA) && m_cTDecTop.getNoOutputPriorPicsFlag() )
      {
        m_cTDecTop.checkNoOutputPriorPics( pcListPic );
        m_cTDecTop.setNoOutputPriorPicsFlag (false);
      }
#endif
#endif
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
      if (nalu.m_nalUnitType == NAL_UNIT_EOS)
      {
#if H_MV
#if FIX_OUTPUT_EOS
        xWriteOutput( pcListPic, decIdxLastPic, nalu.m_temporalId );
#else
        xFlushOutput( pcListPic, decIdxLastPic );
#endif
#else
#if FIX_OUTPUT_EOS
        xWriteOutput( pcListPic, nalu.m_temporalId );
#else
        xFlushOutput( pcListPic );
#endif

#endif
      }
      // write reconstruction to file -- for additional bumping as defined in C.5.2.3 
#if H_MV
      // Above comment seems to be wrong
#endif
#if FIX_WRITING_OUTPUT
      if(!bNewPicture && nalu.m_nalUnitType >= NAL_UNIT_CODED_SLICE_TRAIL_N && nalu.m_nalUnitType <= NAL_UNIT_RESERVED_VCL31)
#else
      if(bNewPicture)
#endif
      {
#if H_MV        
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
#if H_MV
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
#if H_3D_DIM_DMM
  initWedgeLists();
#endif
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

  if (pcListPic->empty())
  {
    return;
  }

  TComList<TComPic*>::iterator iterPic   = pcListPic->begin();
  Int numPicsNotYetDisplayed = 0;
  Int dpbFullness = 0;
#if H_MV
  TComSPS* activeSPS = m_tDecTop[ decIdx ]->getActiveSPS();
#else
  TComSPS* activeSPS = m_cTDecTop.getActiveSPS();
#endif
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
#if H_MV
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
  iterPic   = pcListPic->begin();
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
      
#if H_MV
      if ( pcPicTop->getOutputMark() && (numPicsNotYetDisplayed >  pcPicTop->getNumReorderPics(tId) && !(pcPicTop->getPOC()%2) && pcPicBottom->getPOC() == pcPicTop->getPOC()+1)
          && pcPicBottom->getOutputMark() && (numPicsNotYetDisplayed >  pcPicBottom->getNumReorderPics(tId) && (pcPicTop->getPOC() == m_pocLastDisplay[decIdx]+1 || m_pocLastDisplay[decIdx]<0)))
#else
      if ( pcPicTop->getOutputMark() && pcPicBottom->getOutputMark() &&
          (numPicsNotYetDisplayed >  numReorderPicsHighestTid || dpbFullness > maxDecPicBufferingHighestTid) &&
          (!(pcPicTop->getPOC()%2) && pcPicBottom->getPOC() == pcPicTop->getPOC()+1) &&
          (pcPicTop->getPOC() == m_iPOCLastDisplay+1 || m_iPOCLastDisplay < 0))
#endif
      {
        // write to file
        numPicsNotYetDisplayed = numPicsNotYetDisplayed-2;
#if H_MV
      if ( m_pchReconFiles[decIdx] )
#else
        if ( m_pchReconFile )
#endif
        {
          const Window &conf = pcPicTop->getConformanceWindow();
          const Window &defDisp = m_respectDefDispWindow ? pcPicTop->getDefDisplayWindow() : Window();

          const Bool isTff = pcPicTop->isTopField();
#if H_MV
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
                                        conf.getWindowLeftOffset() + defDisp.getWindowLeftOffset(),
                                        conf.getWindowRightOffset() + defDisp.getWindowRightOffset(),
                                        conf.getWindowTopOffset() + defDisp.getWindowTopOffset(),
                                        conf.getWindowBottomOffset() + defDisp.getWindowBottomOffset(), isTff );
        }
        
        // update POC of display order
#if H_MV
        m_pocLastDisplay[decIdx] = pcPic->getPOC();
#else
        m_iPOCLastDisplay = pcPicBottom->getPOC();
#endif
        
        // erase non-referenced picture in the reference picture list after display
        if ( !pcPicTop->getSlice(0)->isReferenced() && pcPicTop->getReconMark() == true )
        {
#if !DYN_REF_FREE
          pcPicTop->setReconMark(false);
          
          // mark it should be extended later
          pcPicTop->getPicYuvRec()->setBorderExtension( false );
          
#else
          pcPicTop->destroy();
          pcListPic->erase( iterPic );
          iterPic = pcListPic->begin(); // to the beginning, non-efficient way, have to be revised!
          continue;
#endif
        }
        if ( !pcPicBottom->getSlice(0)->isReferenced() && pcPicBottom->getReconMark() == true )
        {
#if !DYN_REF_FREE
          pcPicBottom->setReconMark(false);
          
          // mark it should be extended later
          pcPicBottom->getPicYuvRec()->setBorderExtension( false );
          
#else
          pcPicBottom->destroy();
          pcListPic->erase( iterPic );
          iterPic = pcListPic->begin(); // to the beginning, non-efficient way, have to be revised!
          continue;
#endif
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

#if H_MV
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
#if H_MV
      if ( m_pchReconFiles[decIdx] )
#else
        if ( m_pchReconFile )
#endif
        {
          const Window &conf = pcPic->getConformanceWindow();
          const Window &defDisp = m_respectDefDispWindow ? pcPic->getDefDisplayWindow() : Window();
#if H_MV
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
#if H_MV
        pcPic->setPicOutputFlag(false);
#endif
      }
      
      iterPic++;
    }
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
#if H_MV
      if ( m_pchReconFiles[decIdx] )
#else
        if ( m_pchReconFile )
#endif
        {
          const Window &conf = pcPicTop->getConformanceWindow();
          const Window &defDisp = m_respectDefDispWindow ? pcPicTop->getDefDisplayWindow() : Window();
          const Bool isTff = pcPicTop->isTopField();
#if H_MV
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
                                        conf.getWindowLeftOffset() + defDisp.getWindowLeftOffset(),
                                        conf.getWindowRightOffset() + defDisp.getWindowRightOffset(),
                                        conf.getWindowTopOffset() + defDisp.getWindowTopOffset(),
                                        conf.getWindowBottomOffset() + defDisp.getWindowBottomOffset(), isTff );
        }
        
        // update POC of display order
#if H_MV
      m_pocLastDisplay[decIdx] = pcPic->getPOC();
#else
      m_iPOCLastDisplay = pcPicBottom->getPOC();
#endif        
        // erase non-referenced picture in the reference picture list after display
        if ( !pcPicTop->getSlice(0)->isReferenced() && pcPicTop->getReconMark() == true )
        {
#if !DYN_REF_FREE
          pcPicTop->setReconMark(false);
          
          // mark it should be extended later
          pcPicTop->getPicYuvRec()->setBorderExtension( false );
          
#else
          pcPicTop->destroy();
          pcListPic->erase( iterPic );
          iterPic = pcListPic->begin(); // to the beginning, non-efficient way, have to be revised!
          continue;
#endif
        }
        if ( !pcPicBottom->getSlice(0)->isReferenced() && pcPicBottom->getReconMark() == true )
        {
#if !DYN_REF_FREE
          pcPicBottom->setReconMark(false);
          
          // mark it should be extended later
          pcPicBottom->getPicYuvRec()->setBorderExtension( false );
          
#else
          pcPicBottom->destroy();
          pcListPic->erase( iterPic );
          iterPic = pcListPic->begin(); // to the beginning, non-efficient way, have to be revised!
          continue;
#endif
        }
        pcPicTop->setOutputMark(false);
        pcPicBottom->setOutputMark(false);
        
#if !DYN_REF_FREE
        if(pcPicTop)
        {
          pcPicTop->destroy();
          delete pcPicTop;
          pcPicTop = NULL;
        }
#endif
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
#if H_MV
      if ( m_pchReconFiles[decIdx] )
#else
        if ( m_pchReconFile )
#endif
        {
          const Window &conf = pcPic->getConformanceWindow();
          const Window &defDisp = m_respectDefDispWindow ? pcPic->getDefDisplayWindow() : Window();
#if H_MV
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
#if H_MV
        pcPic->setPicOutputFlag(false);
#endif
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
    m_tDecTop[ decIdx ]->setTargetOptLayerSetIdx( m_targetOptLayerSetIdx );    
#if H_3D_ANNEX_SELECTION_FIX
    m_tDecTop[ decIdx ]->setProfileIdc           ( );    
#endif

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
