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
 ,m_pcSeiColourRemappingInfoPrevious(NULL)
{
#if NH_MV
  for (Int i = 0; i < MAX_NUM_LAYER_IDS; i++)
  {
    m_layerIdToDecIdx                     [i] = -1;
    m_layerInitilizedFlag                 [i] = false;
    m_eosInLayer                          [i] = false;
    m_firstPicInLayerDecodedFlag          [i] = false;
    m_pocDecrementedInDpbFlag             [i] = false;
    m_decodingOrder                       [i] = 0;
    m_lastPresentPocResetIdc              [i] = MIN_INT;
    m_firstPicInPocResettingPeriodReceived[i] = true;
    m_noRaslOutputFlagAssocIrap           [i] = false;
  }

  m_curPic                          = NULL;
  m_vps                             = NULL;
  m_sps                             = NULL;
  m_pps                             = NULL;
  m_initilizedFromVPS               = false;
  m_firstSliceInBitstream           = true;
  m_newVpsActivatedbyCurAu          = false;
  m_newVpsActivatedbyCurPic         = false;
  m_handleCraAsBlaFlag              = false;
  m_handleCraAsBlaFlagSetByExtMeans = false;
  m_noClrasOutputFlag               = false;
  m_noClrasOutputFlagSetByExtMeans  = false;
  m_layerResetFlag                  = false;
  m_totalNumofPicsReceived          = 0;
  m_cvsStartFound                   = false;
#endif
}

Void TAppDecTop::create()
{
#if NH_MV
#if ENC_DEC_TRACE
  if ( g_hTrace == NULL )
  {
    g_hTrace = fopen( "TraceDec.txt", "wb" );
    g_bJustDoIt = g_bEncDecTraceDisable;
    g_nSymbolCounter = 0;
  }
#endif
#endif
}

Void TAppDecTop::destroy()
{
#if NH_MV
  // destroy internal classes
  xDestroyDecLib();
#endif
  m_bitstreamFileName.clear();
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
  m_reconFileName.clear();
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
#if NH_MV
Void TAppDecTop::decode( Int num )
{
  m_targetOptLayerSetIdx = m_targetOptLayerSetInd[ num ]; 
  // create & initialize internal classes
  xInitFileIO  ();
  xCreateDecLib();
  xInitDecLib  ();

  InputByteStream bytestream(m_bitstreamFile);

  while ( m_bitstreamFile )
  {
    AnnexBStats stats = AnnexBStats();
    InputNALUnit nalu;

    byteStreamNALUnit(bytestream, nalu.getBitstream().getFifo(), stats);

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

      if ( m_printReceivedNalus )
      {
        std::cout << "Received NAL unit: ";
        nalu.print();
        std::cout << std::endl;
      }

      if ( xExtractAndRewrite( &nalu )
          && nalu.m_nuhLayerId <= MAX_NUM_LAYER_IDS-1
          && !(nalu.m_nalUnitType == NAL_UNIT_VPS && nalu.m_nuhLayerId > 0)
          && !(nalu.m_nalUnitType == NAL_UNIT_EOB && nalu.m_nuhLayerId > 0)
         )
      {
        xGetDecoderIdx( nalu.m_nuhLayerId , true );
        if( nalu.isSlice() )
        {
          xProcessVclNalu   ( nalu );
        }
        else
        {
          xProcessNonVclNalu( nalu );
        }
      }
    }
  }
 xTerminateDecoding();
}
#endif

#if !NH_MV
Void TAppDecTop::decode( )
{
  Int                 poc;
  TComList<TComPic*>* pcListPic = NULL;

  ifstream bitstreamFile(m_bitstreamFileName.c_str(), ifstream::in | ifstream::binary);
  if (!bitstreamFile)
  {
    fprintf(stderr, "\nfailed to open bitstream file `%s' for reading\n", m_bitstreamFileName.c_str());
    exit(EXIT_FAILURE);
  }

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

  m_iPOCLastDisplay += m_iSkipFrame;      // set the last displayed POC correctly for skip forward.

  // clear contents of colour-remap-information-SEI output file
  if (!m_colourRemapSEIFileName.empty())
  {
    std::ofstream ofile(m_colourRemapSEIFileName.c_str());
    if (!ofile.good() || !ofile.is_open())
    {
      fprintf(stderr, "\nUnable to open file '%s' for writing colour-remap-information-SEI video\n", m_colourRemapSEIFileName.c_str());
      exit(EXIT_FAILURE);
    }
  }

  // main decoder loop
  Bool openedReconFile = false; // reconstruction file not yet opened. (must be performed after SPS is seen)
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
    AnnexBStats stats = AnnexBStats();

    InputNALUnit nalu;
    byteStreamNALUnit(bytestream, nalu.getBitstream().getFifo(), stats);

    // call actual decoding function
    Bool bNewPicture = false;
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

      if( (m_iMaxTemporalLayer >= 0 && nalu.m_temporalId > m_iMaxTemporalLayer) || !isNaluWithinTargetDecLayerIdSet(&nalu)  )
      {
        bNewPicture = false;
      }
      else
      {
        bNewPicture = m_cTDecTop.decode(nalu, m_iSkipFrame, m_iPOCLastDisplay);
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
        }
      }
    }

    if ( (bNewPicture || !bitstreamFile || nalu.m_nalUnitType == NAL_UNIT_EOS) &&

      !m_cTDecTop.getFirstSliceInSequence () )

    {
      if (!loopFiltered || bitstreamFile)
      {
        m_cTDecTop.executeLoopFilters(poc, pcListPic);
      }
      loopFiltered = (nalu.m_nalUnitType == NAL_UNIT_EOS);
      if (nalu.m_nalUnitType == NAL_UNIT_EOS)
      {
        m_cTDecTop.setFirstSliceInSequence(true);
      }
    }
    else if ( (bNewPicture || !bitstreamFile || nalu.m_nalUnitType == NAL_UNIT_EOS ) &&
              m_cTDecTop.getFirstSliceInSequence () )
    {
      m_cTDecTop.setFirstSliceInPicture (true);
   }

    if( pcListPic )
    {
      if ( (!m_reconFileName.empty()) && (!openedReconFile) )
      {
        const BitDepths &bitDepths=pcListPic->front()->getPicSym()->getSPS().getBitDepths(); // use bit depths of first reconstructed picture.
        for (UInt channelType = 0; channelType < MAX_NUM_CHANNEL_TYPE; channelType++)
        {
          if (m_outputBitDepth[channelType] == 0)
          {
            m_outputBitDepth[channelType] = bitDepths.recon[channelType];
          }
        }

        m_cTVideoIOYuvReconFile.open( m_reconFileName, true, m_outputBitDepth, m_outputBitDepth, bitDepths.recon ); // write mode
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
           (   nalu.m_nalUnitType == NAL_UNIT_CODED_SLICE_IDR_W_RADL
            || nalu.m_nalUnitType == NAL_UNIT_CODED_SLICE_IDR_N_LP
            || nalu.m_nalUnitType == NAL_UNIT_CODED_SLICE_BLA_N_LP
            || nalu.m_nalUnitType == NAL_UNIT_CODED_SLICE_BLA_W_RADL
            || nalu.m_nalUnitType == NAL_UNIT_CODED_SLICE_BLA_W_LP ) )
      {

        xFlushOutput( pcListPic );
      }
      if (nalu.m_nalUnitType == NAL_UNIT_EOS)
      {

        xWriteOutput( pcListPic, nalu.m_temporalId );
        m_cTDecTop.setFirstSliceInPicture (false);
      }
      // write reconstruction to file -- for additional bumping as defined in C.5.2.3
      if(!bNewPicture && nalu.m_nalUnitType >= NAL_UNIT_CODED_SLICE_TRAIL_N && nalu.m_nalUnitType <= NAL_UNIT_RESERVED_VCL31)
      {
        xWriteOutput( pcListPic, nalu.m_temporalId );
      }
    }
  }

  xFlushOutput( pcListPic );
  // delete buffers
  m_cTDecTop.deletePicBuffer();
  // destroy internal classes
  xDestroyDecLib();
}
#endif

// ====================================================================================================================
// Protected member functions
// ====================================================================================================================

Void TAppDecTop::xCreateDecLib()
{
#if NH_MV
  // initialize global variables
  initROM();
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
#if !NH_MV
      m_tDecTop[decIdx]->deletePicBuffer();
#endif
      m_tDecTop[decIdx]->destroy() ;
    }
    delete m_tDecTop[decIdx] ;
    m_tDecTop[decIdx] = NULL ;
  }
#else
  if ( !m_reconFileName.empty() )
  {
    m_cTVideoIOYuvReconFile. close();
  }

  // destroy decoder class
  m_cTDecTop.destroy();
#endif
  if (m_pcSeiColourRemappingInfoPrevious != NULL)
  {
    delete m_pcSeiColourRemappingInfoPrevious;
    m_pcSeiColourRemappingInfoPrevious = NULL;
  }
}

Void TAppDecTop::xInitDecLib()
{

#if NH_MV
  m_dpb.setPrintPicOutput(m_printPicOutput);
#else
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
  if (m_pcSeiColourRemappingInfoPrevious != NULL)
  {
    delete m_pcSeiColourRemappingInfoPrevious;
    m_pcSeiColourRemappingInfoPrevious = NULL;
  }
}


#if !NH_MV
/** \param pcListPic list of pictures to be written to file
    \param tId       temporal sub-layer ID
 */
Void TAppDecTop::xWriteOutput( TComList<TComPic*>* pcListPic, UInt tId )
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
    if(pcPic->getOutputMark() && pcPic->getPOC() > m_iPOCLastDisplay)
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

      if ( pcPicTop->getOutputMark() && pcPicBottom->getOutputMark() &&
          (numPicsNotYetDisplayed >  numReorderPicsHighestTid || dpbFullness > maxDecPicBufferingHighestTid) &&
          (!(pcPicTop->getPOC()%2) && pcPicBottom->getPOC() == pcPicTop->getPOC()+1) &&
          (pcPicTop->getPOC() == m_iPOCLastDisplay+1 || m_iPOCLastDisplay < 0))
      {
        // write to file
        numPicsNotYetDisplayed = numPicsNotYetDisplayed-2;
        if ( !m_reconFileName.empty() )
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
        m_cTVideoIOYuvReconFile.write( pcPicTop->getPicYuvRec(), pcPicBottom->getPicYuvRec(),
                                           m_outputColourSpaceConvert,
                                           conf.getWindowLeftOffset() + defDisp.getWindowLeftOffset(),
                                           conf.getWindowRightOffset() + defDisp.getWindowRightOffset(),
                                           conf.getWindowTopOffset() + defDisp.getWindowTopOffset(),
                                           conf.getWindowBottomOffset() + defDisp.getWindowBottomOffset(), NUM_CHROMA_FORMAT, isTff );
          }
        }

        // update POC of display order
        m_iPOCLastDisplay = pcPicBottom->getPOC();

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

      if(pcPic->getOutputMark() && pcPic->getPOC() > m_iPOCLastDisplay &&
        (numPicsNotYetDisplayed >  numReorderPicsHighestTid || dpbFullness > maxDecPicBufferingHighestTid))
      {
        // write to file
         numPicsNotYetDisplayed--;
        if(pcPic->getSlice(0)->isReferenced() == false)
        {
          dpbFullness--;
        }
        if ( !m_reconFileName.empty() )
        {
          const Window &conf    = pcPic->getConformanceWindow();
          const Window defDisp = m_respectDefDispWindow ? pcPic->getDefDisplayWindow() : Window();

          m_cTVideoIOYuvReconFile.write( pcPic->getPicYuvRec(),
                                         m_outputColourSpaceConvert,
                                         conf.getWindowLeftOffset() + defDisp.getWindowLeftOffset(),
                                         conf.getWindowRightOffset() + defDisp.getWindowRightOffset(),
                                         conf.getWindowTopOffset() + defDisp.getWindowTopOffset(),
                                         conf.getWindowBottomOffset() + defDisp.getWindowBottomOffset(),
                                         NUM_CHROMA_FORMAT, m_bClipOutputVideoToRec709Range  );
        }

        if (!m_colourRemapSEIFileName.empty())
        {
          xOutputColourRemapPic(pcPic);
        }

        // update POC of display order
        m_iPOCLastDisplay = pcPic->getPOC();

        // erase non-referenced picture in the reference picture list after display
        if ( !pcPic->getSlice(0)->isReferenced() && pcPic->getReconMark() == true )
        {
          pcPic->setReconMark(false);

          // mark it should be extended later
          pcPic->getPicYuvRec()->setBorderExtension( false );
        }
        pcPic->setOutputMark(false);
      }

      iterPic++;
    }
  }
}

/** \param pcListPic list of pictures to be written to file
 */
Void TAppDecTop::xFlushOutput( TComList<TComPic*>* pcListPic )
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

        if ( !m_reconFileName.empty() )
        {
          const Window &conf = pcPicTop->getConformanceWindow();
          const Window  defDisp = m_respectDefDispWindow ? pcPicTop->getDefDisplayWindow() : Window();
          const Bool isTff = pcPicTop->isTopField();
          m_cTVideoIOYuvReconFile.write( pcPicTop->getPicYuvRec(), pcPicBottom->getPicYuvRec(),
                                         m_outputColourSpaceConvert,
                                         conf.getWindowLeftOffset() + defDisp.getWindowLeftOffset(),
                                         conf.getWindowRightOffset() + defDisp.getWindowRightOffset(),
                                         conf.getWindowTopOffset() + defDisp.getWindowTopOffset(),
                                         conf.getWindowBottomOffset() + defDisp.getWindowBottomOffset(), NUM_CHROMA_FORMAT, isTff );
        }

        // update POC of display order
        m_iPOCLastDisplay = pcPicBottom->getPOC();

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
        if ( !m_reconFileName.empty() )
        {
          const Window &conf    = pcPic->getConformanceWindow();
          const Window  defDisp = m_respectDefDispWindow ? pcPic->getDefDisplayWindow() : Window();
          m_cTVideoIOYuvReconFile.write( pcPic->getPicYuvRec(),
                                         m_outputColourSpaceConvert,
                                         conf.getWindowLeftOffset() + defDisp.getWindowLeftOffset(),
                                         conf.getWindowRightOffset() + defDisp.getWindowRightOffset(),
                                         conf.getWindowTopOffset() + defDisp.getWindowTopOffset(),
                                         conf.getWindowBottomOffset() + defDisp.getWindowBottomOffset(),
                                         NUM_CHROMA_FORMAT, m_bClipOutputVideoToRec709Range );
        }

        if (!m_colourRemapSEIFileName.empty())
        {
          xOutputColourRemapPic(pcPic);
        }

        // update POC of display order
        m_iPOCLastDisplay = pcPic->getPOC();
        // erase non-referenced picture in the reference picture list after display
        if ( !pcPic->getSlice(0)->isReferenced() && pcPic->getReconMark() == true )
        {
          pcPic->setReconMark(false);

          // mark it should be extended later
          pcPic->getPicYuvRec()->setBorderExtension( false );
        }
        pcPic->setOutputMark(false);
      }
      if(pcPic != NULL)
      {
        pcPic->destroy();
        delete pcPic;
        pcPic = NULL;
      }
      iterPic++;
    }
  }
  pcListPic->clear();
  m_iPOCLastDisplay = -MAX_INT;
}
#endif
/** \param nalu Input nalu to check whether its LayerId is within targetDecLayerIdSet
 */
#if NH_MV
Bool TAppDecTop::xIsNaluInTargetDecLayerIdSet( InputNALUnit* nalu )
#else
Bool TAppDecTop::isNaluWithinTargetDecLayerIdSet( InputNALUnit* nalu )
#endif
{
  if ( m_targetDecLayerIdSet.size() == 0 ) // By default, the set is empty, meaning all LayerIds are allowed
  {
    return true;
  }
  for (std::vector<Int>::iterator it = m_targetDecLayerIdSet.begin(); it != m_targetDecLayerIdSet.end(); it++)
  {
    if ( nalu->m_nuhLayerId == (*it) )
    {
      return true;
    }
  }
  return false;
}

#if NH_MV

Bool TAppDecTop::xExtractAndRewrite( InputNALUnit* nalu )
{
  Bool naluInSubStream;
  if ( !m_initilizedFromVPS )
  {
    naluInSubStream = true; // No active VPS yet. Wait for slice activating one.
  }
  else
  {
    if ( m_decProcCvsg == CLAUSE_8 )
    {
      // 8.1.2->clause 10

      // sub-bitstream extraction process as specified in clause 10
      naluInSubStream = true;
      if ( nalu->m_temporalId > m_highestTid || !xIsNaluInTargetDecLayerIdSet(nalu) )
      {
        naluInSubStream = false;
      }
    }
    else
    {
      // F.8.1.2
      Int targetDecLayerSetIdx = m_vps->olsIdxToLsIdx( m_targetOptLayerSetIdx );
      if ( targetDecLayerSetIdx <= m_vps->getVpsNumLayerSetsMinus1() && m_vps->getVpsBaseLayerInternalFlag() )
      {
        // - If TargetDecLayerSetIdx is less than or equal to vps_num_layer_sets_minus1 and
        //   vps_base_layer_internal_flag is equal to 1, the following applies:
        //   - The sub-bitstream extraction process as specified in clause 10 is applied with the CVSG, HighestTid and
        //     TargetDecLayerIdList as inputs, and the output is assigned to a bitstream referred to as BitstreamToDecode.

        naluInSubStream = true;
        if ( nalu->m_temporalId > m_highestTid || !xIsNaluInTargetDecLayerIdSet(nalu) )
        {
          naluInSubStream = false;
        }

      }
      else if ( targetDecLayerSetIdx <= m_vps->getVpsNumLayerSetsMinus1() && !m_vps->getVpsBaseLayerInternalFlag() )
      {
        // - Otherwise, if TargetDecLayerSetIdx is less than or equal to vps_num_layer_sets_minus1 and vps_base_layer_internal_flag
        //   is equal to 0, the following applies:
        //   - The sub-bitstream extraction process as specified in clause F.10.1 is applied with the CVSG, HighestTid and
        //     TargetDecLayerIdList as inputs, and the output is assigned to a bitstream referred to as BitstreamToDecode.

        naluInSubStream = true;
        if ( nalu->m_temporalId > m_highestTid || !xIsNaluInTargetDecLayerIdSet(nalu) )
        {
          naluInSubStream = false;
        }
      }
      else if ( targetDecLayerSetIdx > m_vps->getVpsNumLayerSetsMinus1() && m_vps->getNumLayersInIdList( targetDecLayerSetIdx ) == 1 )
      {
        // - Otherwise, if TargetDecLayerSetIdx is greater than vps_num_layer_sets_minus1 and
        //   NumLayersInIdList[ TargetDecLayerSetIdx ] is equal to 1, the following applies:
        //   - The independent non-base layer rewriting process of clause F.10.2 is applied with the CVSG, HighestTid and
        //     TargetDecLayerIdList[ 0 ] as inputs, and the output is assigned to a bitstream referred to as BitstreamToDecode.

        Int assingedBaseLayerId = m_targetDecLayerIdSet[0];
        naluInSubStream = true;

        if ( nalu->m_nalUnitType != NAL_UNIT_SPS &&
          nalu->m_nalUnitType != NAL_UNIT_PPS &&
          nalu->m_nalUnitType != NAL_UNIT_EOB &&
          nalu->m_nuhLayerId != assingedBaseLayerId )
        {
          naluInSubStream = false;
        }

        if ( ( nalu->m_nalUnitType == NAL_UNIT_SPS || nalu->m_nalUnitType != NAL_UNIT_PPS ) &&
          !( nalu->m_nuhLayerId == 0 || nalu->m_nuhLayerId == assingedBaseLayerId ) )
        {
          naluInSubStream = false;
        }

        if ( nalu->m_nalUnitType == NAL_UNIT_VPS )
        {
          naluInSubStream = false;
        }

        if ( nalu->m_temporalId > m_highestTid )
        {
          naluInSubStream = false;
        }
                
        // For now, don't do the layer id change here, but change smallest layer id. 
        // To be verified.         
        //if ( naluInSubStream )
        //{
        //  nalu->m_nuhLayerId = 0;
        //}
      }
      else
      {
        // - Otherwise, the following applies:
        //   - The sub-bitstream extraction process as specified in clause F.10.3 is applied with the CVSG, HighestTid and
        //     TargetDecLayerIdList as inputs, and the output is assigned to a bitstream referred to as BitstreamToDecode.

        naluInSubStream = true;

        if ( nalu->m_nalUnitType != NAL_UNIT_VPS &&
          nalu->m_nalUnitType != NAL_UNIT_SPS &&
          nalu->m_nalUnitType != NAL_UNIT_PPS &&
          nalu->m_nalUnitType != NAL_UNIT_EOS &&
          nalu->m_nalUnitType != NAL_UNIT_EOB &&
          !xIsNaluInTargetDecLayerIdSet(nalu) )
        {
          naluInSubStream = false;
        }

        if ( ( nalu->m_nalUnitType == NAL_UNIT_VPS ||
          nalu->m_nalUnitType == NAL_UNIT_SPS ||
          nalu->m_nalUnitType == NAL_UNIT_PPS ||
          nalu->m_nalUnitType == NAL_UNIT_EOS    ) &&
          !( nalu->m_nuhLayerId == 0 || xIsNaluInTargetDecLayerIdSet(nalu) )
          )
        {
          naluInSubStream = false;
        }

        if ( nalu->m_temporalId > m_highestTid )
        {
          naluInSubStream = false;
        }
        // TBD: vps_base_layer_available_flag in each VPS is set equal to 0.
      }
    }
  }

  return naluInSubStream;
}

Void TAppDecTop::xProcessVclNalu( InputNALUnit nalu )
{
  TDecTop* dec  = xGetDecoder( nalu );

  // Decode slice header of slice of current or new picture.
  dec->decodeSliceHeader( nalu );

  // Check if slice belongs to current or new picture
  Bool sliceIsFirstOfNewPicture = dec->getFirstSliceSegementInPicFlag();

  if ( !xIsSkipVclNalu( nalu, sliceIsFirstOfNewPicture ) )
  {
    if ( sliceIsFirstOfNewPicture )
    {
      xDetectNewPocResettingPeriod( nalu );
      Bool sliceIsFirstOfNewAU = xDetectNewAu( nalu );
      xFinalizePreviousPictures ( sliceIsFirstOfNewAU );
      xDecodeFirstSliceOfPicture( nalu, sliceIsFirstOfNewAU );
    }
    else
    {
      xDecodeFollowSliceOfPicture( nalu );
    }
  }
}

Bool TAppDecTop::xIsSkipVclNalu( InputNALUnit& nalu, Bool isFirstSliceOfPic )
{

  TDecTop* dec = xGetDecoder( nalu ); 

  m_handleCraAsBlaFlagSetByExtMeans = false;
  Bool skipNalu = false;

  if ( isFirstSliceOfPic )
  {
    m_totalNumofPicsReceived++;
  }

  if (!m_cvsStartFound )
  {
    // Skip as specified by decoder option. (Not normative!)
    if ( m_totalNumofPicsReceived <= m_iSkipFrame )
    {
      skipNalu = true;
      if ( isFirstSliceOfPic )
      {
        std::cout << "Layer " << std::setfill(' ') << std::setw(2) << nalu.m_nuhLayerId
          << "   POC    ? Skipping picture." << std::setw(5) << m_totalNumofPicsReceived - 1 << std::endl;
      }
    }
    else
    {      
      if ( dec->getIsInOwnTargetDecLayerIdList() )
      {
        // Search for initial IRAP access unit
        Bool canBeSliceOfInitialIrapAu = nalu.isIrap() && ( dec->decProcClause8() || nalu.m_nuhLayerId == dec->getSmallestLayerId() );

        if ( !canBeSliceOfInitialIrapAu )
        {
          skipNalu = true;
          if ( isFirstSliceOfPic )
          {
            std::cout << "Layer " << std::setfill(' ') << std::setw(2) << nalu.m_nuhLayerId
              << "   POC    ? Not an initial IRAP AU. Skipping picture." << std::setw(5) << m_totalNumofPicsReceived - 1 << std::endl;
          }
        }
        else
        {
          m_cvsStartFound = true;
          if( nalu.isCra() )
          {
            // Ensure that NoRaslOutputFlag of picture is equal to 1.
            m_handleCraAsBlaFlagSetByExtMeans = true;
            m_handleCraAsBlaFlag = true;
          }
        }
      }
      else
      {
        skipNalu = true; 
      }
    }
  }
  else
  {
    assert( dec->getIsInOwnTargetDecLayerIdList() ); 
  }
  return skipNalu;
}

Void TAppDecTop::xProcessNonVclNalu( InputNALUnit nalu )
{
  xGetDecoder(nalu)->decodeNonVclNalu( nalu );

  if (  nalu.m_nalUnitType == NAL_UNIT_EOS )
  {
    m_eosInLayer[ nalu.m_nuhLayerId ] = true;
  }
}
Void TAppDecTop::xTerminateDecoding()
{
    xFinalizePic( true );
    xFinalizeAU ( );

   xFlushOutput(); 
   m_dpb.emptyAllSubDpbs();
}


//////////////////////////////
/// Process slices
//////////////////////////////

Void TAppDecTop::xDecodeFirstSliceOfPicture( InputNALUnit nalu, Bool sliceIsFirstOfNewAu )
{
  TDecTop* dec = xGetDecoder(nalu);
  // Initialize from VPS of first slice

  // Get current SPS and PPS
  TComSlice* slicePilot = dec->getSlicePilot(); 
  m_vps = slicePilot->getVPS(); 
  m_sps = slicePilot->getSPS(); 
  m_pps = slicePilot->getPPS(); 

  /// Use VPS activated by the first slice to initialized decoding
  if( !m_initilizedFromVPS )
  {
    xF811GeneralDecProc( nalu );
    m_initilizedFromVPS = true;
    m_newVpsActivatedbyCurAu  = true; //TBD
    m_newVpsActivatedbyCurPic = true;
  }

  // Create new sub-DBP if not existing
  m_dpb.getSubDpb( nalu.m_nuhLayerId, true );

  // Create a new picture initialize and make it the current picture
  assert( m_curPic == NULL );
  m_curPic = new TComPic;

  m_curPic->create(*m_sps, *m_pps, true);

  m_curPic->setLayerId                      ( nalu.m_nuhLayerId );
  m_curPic->setDecodingOrder                ( m_decodingOrder[ nalu.m_nuhLayerId ]);
  m_curPic->setIsFstPicOfAllLayOfPocResetPer( m_newPicIsFstPicOfAllLayOfPocResetPer );
  m_curPic->setIsPocResettingPic            ( m_newPicIsPocResettingPic );
  m_curPic->setActivatesNewVps              ( m_newVpsActivatedbyCurPic );

  dec     ->activatePSsAndInitPicOrSlice( m_curPic );

  m_decodingOrder[ nalu.m_nuhLayerId ]++;

  // Insert pic to current AU
  // ( There is also a "current AU in the DBP", however the DBP AU will include the current
  //   picture only after it is inserted to the DBP )
  m_curAu.addPic(  m_curPic, true );

  // Invoke Claus 8 and Annex F decoding process for a picture (only parts before POC derivation ).
  xPicDecoding( START_PIC, sliceIsFirstOfNewAu );

  if (m_decProcCvsg == ANNEX_F )
  {
    // Do output before POC derivation
    xF13522OutputAndRemOfPicsFromDpb( true );
  }

  // Decode POC and apply reference picture set
  dec->setDecProcPocAndRps( m_decProcPocAndRps );
  dec->decodePocAndRps( );

  // Do output after POC and RPS derivation
  if (m_decProcCvsg == CLAUSE_8 )
  {
    xC522OutputAndRemOfPicsFromDpb( );
  }
  else if (m_decProcCvsg == ANNEX_F )
  {
    xF13522OutputAndRemOfPicsFromDpb( false );
  }
  else
  {
    assert(false);
  }

  // Generate unavailable reference pictures
  dec->genUnavailableRefPics( );

  // decode first slice segment
  dec->decodeSliceSegment( nalu );

  m_firstSliceInBitstream = false;
}

Void TAppDecTop::xDecodeFollowSliceOfPicture( InputNALUnit nalu )
{
  // decode following segment
    TDecTop* dec = xGetDecoder( nalu ); 
    dec->activatePSsAndInitPicOrSlice( NULL );
    dec->decodeSliceSegment          ( nalu );
}

Void TAppDecTop::xFinalizePreviousPictures( Bool sliceIsFirstOfNewAU )
{
  Bool curPicIsLastInAu = sliceIsFirstOfNewAU && (m_curPic != NULL);
  // When slice belongs to new picture, finalize current picture.
  if( m_curPic != NULL )
  {
    xFinalizePic( curPicIsLastInAu );

    if (m_curPic->isIrap() )
    {
      m_noRaslOutputFlagAssocIrap[ m_curPic->getLayerId() ] = m_curPic->getNoRaslOutputFlag();
    }

    m_tDecTop[ xGetDecoderIdx( m_curPic->getLayerId() )]->finalizePic();
    m_curPic->getPicYuvRec()->extendPicBorder(); 
    m_newVpsActivatedbyCurPic = false;
  }

  // When slice belongs to new AU, finalize current AU.
  if ( curPicIsLastInAu && !m_curAu.empty() )
  {
    xFinalizeAU( );
    m_newVpsActivatedbyCurAu = false;
    m_curAu.clear();
  }
  m_curPic = NULL;
}


Void TAppDecTop::xFinalizePic(Bool curPicIsLastInAu )
{
  if ( m_curPic != NULL )
  {
    m_tDecTop[ xGetDecoderIdx(m_curPic->getLayerId() ) ]->executeLoopFilters( ); // 8.7

    // Invoke Claus 8 and F.8 decoding process for a picture (only parts after POC derivation )
    xPicDecoding(FINALIZE_PIC, curPicIsLastInAu );

    if( m_decProcCvsg == CLAUSE_8 )
    {
      xC523PicDecMarkAddBumpAndStor    ( );
    }
    else if ( m_decProcCvsg == ANNEX_F )
    {
      xF13523PicDecMarkAddBumpAndStor  ( curPicIsLastInAu );
    }
  }
}

Void TAppDecTop::xFinalizeAU()
{
}


Void TAppDecTop::xF811GeneralDecProc( InputNALUnit nalu )
{
  ////////////////////////////////////////////////////////////////////////////////
  // F.8.1 General decoding process
  ////////////////////////////////////////////////////////////////////////////////

  // The following applies at the beginning of decoding a CVSG, after activating the VPS RBSP that is active for
  // the entire CVSG and before decoding any VCL NAL units of the CVSG:

  if ( !m_vps->getVpsExtensionFlag() )
  {
    //-   If vps_extension( ) is not present in the active VPS or a decoding process specified in this annex is not in use,
    //   clause 8.1.2 is invoked with the CVSG as input.
    x812CvsgDecodingProcess( xGetDecoderIdx( nalu.m_nuhLayerId ) );
    m_decProcCvsg = CLAUSE_8;
  }
  else
  {
    // - Otherwise (vps_extension( ) is present in the active VPS and a decoding process specified in this annex is in use),
    xF812CvsgDecodingProcess( xGetDecoderIdx( nalu.m_nuhLayerId ) );
    xF13521InitDpb();
    m_decProcCvsg = ANNEX_F;
  }

  if ( m_printVpsInfo  && ( m_decProcCvsg == ANNEX_F ) && ( m_targetOptLayerSetIdx == m_targetOptLayerSetInd[ 0 ] ) )
  {
    m_vps->printScalabilityId();
    m_vps->printLayerDependencies();
    m_vps->printLayerSets();
    m_vps->printPTL();
  }
}

Void   TAppDecTop::xPicDecoding( DecProcPart curPart, Bool picPosInAuIndication )
{
  if ( m_decProcCvsg == CLAUSE_8 )
  {
    // F.8.1.1 -> 8.1.2 -> 8.1.3
    x813decProcForCodPicWithLIdZero  ( curPart );
  }
  else if ( m_decProcCvsg == ANNEX_F )
  {
    if ( m_targetOptLayerSetIdx == 0  )
    {
      // F.8.1.1 -> F.8.1.2 -> 8.1.3
      x813decProcForCodPicWithLIdZero  ( curPart );
    }
    else
    {
      // F.8.1.1 -> F.8.1.2 -> F.8.1.3
      xF813ComDecProcForACodedPic( curPart, picPosInAuIndication );
    }
  }
  else
  {
    assert( false );
  }
}

Void TAppDecTop::x812CvsgDecodingProcess( Int decIdx )
{
  ///////////////////////////////////////////////////////////////////////////////////////
  //  8.1.2 CVSG decoding process
  ///////////////////////////////////////////////////////////////////////////////////////

  // The layer identifier list TargetDecLayerIdList, which specifies the list of nuh_layer_id values,
  // in increasing order of nuh_layer_id values, of the NAL units to be decoded, is specified as follows:

  Bool externalMeansToSetTargetDecLayerIdList = !m_targetDecLayerIdSetFileEmpty;

  if ( externalMeansToSetTargetDecLayerIdList )
  {
    // - If some external means, not specified in this Specification, is available to set TargetDecLayerIdList,
    //   TargetDecLayerIdList is set by the external means.
    assert( !m_targetDecLayerIdSet.empty() ); // Already done when parsing cfg ile
  }
  else
  {
    //-  Otherwise, TargetDecLayerIdList contains only one nuh_layer_id value that is equal to 0.
    m_targetDecLayerIdSet.clear();
    m_targetDecLayerIdSet.push_back( 0 );
  }

  // The variable HighestTid, which identifies the highest temporal sub-layer to be decoded, is specified as follows:
  Bool externalMeansSetHighestTid = ( m_iMaxTemporalLayer != -1 );
  if ( externalMeansSetHighestTid )
  {
    //- If some external means, not specified in this Specification, is available to set HighestTid,
    //  HighestTid is set by the external means.
    m_highestTid = m_iMaxTemporalLayer;
  }
  else
  {
    //-  Otherwise, HighestTid is set equal to sps_max_sub_layers_minus1.
    m_highestTid = m_sps->getSpsMaxSubLayersMinus1();
  }

  //The variable SubPicHrdFlag is specified as follows:
  //- If the decoding process is invoked in a bitstream conformance test as specified in clause C.1, SubPicHrdFlag is set as specified in clause C.1.
  //- Otherwise, SubPicHrdFlag is set equal to ( SubPicHrdPreferredFlag  &&  sub_pic_hrd_params_present_flag ).

  // The sub-bitstream extraction process as specified in clause 10 is applied with the CVSG, HighestTid and TargetDecLayerIdList as inputs, and the output is assigned to a bitstream referred to as BitstreamToDecode.
}

Void TAppDecTop::x813decProcForCodPicWithLIdZero( DecProcPart curPart )
{
  ////////////////////////////////////////////////////////////////////////////////
  // 8.1.3 Decoding process for a coded picture with nuh_layer_id equal to 0.
  ////////////////////////////////////////////////////////////////////////////////

  if ( curPart == START_PIC )
  {
    Int nuhLayerId = m_curPic->getLayerId();

    if ( ( m_curPic->isBla() && m_curPic->getSlice(0)->getNalUnitType() == NAL_UNIT_CODED_SLICE_BLA_W_LP ) || m_curPic->isCra() )
    {
      // Not needed.
      // When the current picture is a BLA picture that has nal_unit_type equal to BLA_W_LP or is a CRA picture, the following applies:
      // -  If some external means not specified in this Specification is available to set the variable UseAltCpbParamsFlag to a value, UseAltCpbParamsFlag is set equal to the value provided by the external means.
      // -  Otherwise, the value of UseAltCpbParamsFlag is set equal to 0.
    }

    if ( m_curPic->isIrap() )
    {
      // When the current picture is an IRAP picture, the following applies:
      if ( m_curPic->isIdr() || m_curPic->isBla() || m_curPic->getDecodingOrder() == 0 || m_eosInLayer[ nuhLayerId ] )
      {
        // -  If the current picture is an IDR picture, a BLA picture, the first picture in the bitstream in decoding order,
        //    or the first picture that follows an end of sequence NAL unit in decoding order, the variable NoRaslOutputFlag
        //    is set equal to 1.

        m_curPic->setNoRaslOutputFlag( true );
      }
      else if ( m_handleCraAsBlaFlagSetByExtMeans )
      {
        // -  Otherwise, if some external means not specified in this Specification is available to set the variable HandleCraAsBlaFlag
        //    to a value for the current picture, the variable HandleCraAsBlaFlag is set equal to the value provided by
        //    the external means and the variable NoRaslOutputFlag is set equal to HandleCraAsBlaFlag.

        m_curPic->setNoRaslOutputFlag( m_handleCraAsBlaFlag );
      }
      else
      {
        // -  Otherwise, the variable HandleCraAsBlaFlag is set equal to 0 and the variable NoRaslOutputFlag is set equal to 0.
        m_handleCraAsBlaFlag = false;
        m_curPic->setNoRaslOutputFlag( false );
      }
    }

    m_decProcPocAndRps = CLAUSE_8;
  }
  else if ( curPart == FINALIZE_PIC )
  {
    // -  PicOutputFlag is set as follows:
    if (m_curPic->isRasl() && m_noRaslOutputFlagAssocIrap[ m_curPic->getLayerId() ] )
    {
      // -  If the current picture is a RASL picture and NoRaslOutputFlag of the associated IRAP picture is equal to 1,
      //    PicOutputFlag is set equal to 0.
      m_curPic->setPicOutputFlag( false );
    }
    else
    {
      // -  Otherwise, PicOutputFlag is set equal to pic_output_flag.
      m_curPic->setPicOutputFlag( m_curPic->getSlice(0)->getPicOutputFlag() );
    }

    // 4.  After all slices of the current picture have been decoded, the decoded picture is marked as "used for short-term reference".
    m_curPic->markAsUsedForShortTermReference();
  }
}

// F.8.1.2
Void TAppDecTop::xF812CvsgDecodingProcess( Int decIdx )
{
  ///////////////////////////////////////////////////////////////////////////////////////
  //  F.8.1.2 CVSG decoding process
 ///////////////////////////////////////////////////////////////////////////////////////

  // The variable TargetOlsIdx, which specifies the index to the list of the OLSs
  // specified by the VPS, of the target OLS, is specified as follows:


  // If some external means, not specified in this Specification, is available to set
  // TargetOlsIdx, TargetOlsIdx is set by the external means.

  // For this decoder the TargetOlsIdx is always set by external means:
  // When m_targetOptLayerSetIdx is equal to -1,  TargetOlsIdx is set to getVpsNumLayerSetsMinus1 (which has already been done in TDecTop, since needed there for parsing)
  // Otherwise m_targetOptLayerSetIdx is used directly.

  if ( m_targetOptLayerSetIdx == -1 )
  {
    m_targetOptLayerSetIdx = m_tDecTop[decIdx]->getTargetOlsIdx();
  }
  else
  {
    assert( m_tDecTop[decIdx]->getTargetOlsIdx() == m_targetOptLayerSetIdx );
  }

  m_targetDecLayerSetIdx = m_vps->olsIdxToLsIdx( m_targetOptLayerSetIdx );
  m_targetDecLayerIdSet  = m_vps->getTargetDecLayerIdList( m_targetOptLayerSetIdx );


  if ( m_targetOptLayerSetIdx < 0 || m_targetOptLayerSetIdx >= m_vps->getNumOutputLayerSets() )
  {
    fprintf(stderr, "\ntarget output layer set index must be in the range of 0 to %d, inclusive \n", m_vps->getNumOutputLayerSets() - 1 );
    exit(EXIT_FAILURE);
  }

  if ( !m_vps->getVpsBaseLayerAvailableFlag() )
  {
    if(  m_targetDecLayerSetIdx < m_vps->getFirstAddLayerSetIdx() || m_targetDecLayerSetIdx > m_vps->getLastAddLayerSetIdx() )
    {
      // When vps_base_layer_available_flag is equal to 0, OlsIdxToLsIdx[ TargetOlsIdx ] shall be in the range of FirstAddLayerSetIdx to LastAddLayerSetIdx, inclusive.
      fprintf(stderr, "\nvps_base_layer_available_flag is equal to 0, OlsIdxToLsIdx[ TargetOlsIdx ] shall be in the range of %d to %d, inclusive \n", m_vps->getFirstAddLayerSetIdx(), m_vps->getLastAddLayerSetIdx() );
      exit(EXIT_FAILURE);
    }
  }

  if ( !m_vps->getVpsBaseLayerInternalFlag() && m_targetOptLayerSetIdx <= 0 )
  {
    // When vps_base_layer_internal_flag is equal to 0, TargetOlsIdx shall be greater than 0.
    fprintf(stderr, "\nvps_base_layer_internal_flag is equal to 0, TargetOlsIdx shall be greater than 0.\n" );
    exit(EXIT_FAILURE);
  }

  // The variable HighestTid, which identifies the highest temporal sub-layer to be decoded,
  // is specified as follows:

  Bool externalMeansSetHighestTid = ( m_iMaxTemporalLayer != -1 );
  if ( externalMeansSetHighestTid )
  {
    m_highestTid = m_iMaxTemporalLayer;
  }
  else
  {
    m_highestTid = m_sps->getSpsMaxSubLayersMinus1();
  }

  // The variable SubPicHrdPreferredFlag is either specified by external means, or when not specified by external means, set equal to 0.
  // The variable SubPicHrdFlag is specified as follows:
  // -  If the decoding process is invoked in a bitstream conformance test as specified in clause F.13.1, SubPicHrdFlag is set as specified in clause F.13.1.
  // -  Otherwise, SubPicHrdFlag is set equal to ( SubPicHrdPreferredFlag  &&  sub_pic_hrd_params_present_flag ), where sub_pic_hrd_params_present_flag is found in any hrd_parameters( ) syntax structure that applies to at least one bitstream partition of the output layer set idenified by TargetOlsIdx.
  // -  TBD in case that needed some when.

  // A bitstream to be decoded, BitstreamToDecode, is specified as follows:
  //   - Extraction is done in xExtractAndRewrite();

  //   - SmallestLayerId is already derived in  TDecTop:: initFromActiveVps, since required for SH parsing.
  m_smallestLayerId = m_tDecTop[decIdx]->getSmallestLayerId();

  // When vps_base_layer_internal_flag is equal to 0, vps_base_layer_available_flag is equal to 1,
  // and TargetDecLayerSetIdx is in the range of 0 to vps_num_layer_sets_minus1, inclusive,
  // the following applies:
  if ( !m_vps->getVpsBaseLayerInternalFlag() && m_vps->getVpsBaseLayerAvailableFlag() &&
       ( m_targetDecLayerSetIdx >= 0 && m_targetDecLayerSetIdx <= m_vps->getVpsNumLayerSetsMinus1() ) )
  {

    // TBD: The size of the sub-DPB for the layer with nuh_layer_id equal to 0 is set equal to 1.

    // TBD: The values of pic_width_in_luma_samples, pic_height_in_luma_samples, chroma_format_idc,
    // separate_colour_plane_flag, bit_depth_luma_minus8, bit_depth_chroma_minus8, conf_win_left_offset,
    // conf_win_right_offset, conf_win_top_offset, and conf_win_bottom_offset for decoded pictures
    // with nuh_layer_id equal to 0 are set equal to the values of pic_width_vps_in_luma_samples,
    // pic_height_vps_in_luma_samples, chroma_format_vps_idc, separate_colour_plane_vps_flag,
    // bit_depth_vps_luma_minus8, bit_depth_vps_chroma_minus8, conf_win_vps_left_offset,
    // conf_win_vps_right_offset, conf_win_vps_top_offset, and conf_win_vps_bottom_offset respectively,
    // of the vps_rep_format_idx[ 0 ]-th rep_format( ) syntax structure in the active VPS.

    // The variable BaseLayerOutputFlag is set equal to ( TargetOptLayerIdList[ 0 ]  = =  0 ).

    m_baseLayerOutputFlag = ( m_vps->getTargetOptLayerIdList( m_targetOptLayerSetIdx )[ 0 ] == 0 );

    // NOTE - The BaseLayerOutputFlag for each access unit is to be sent by an external means to
    // the base layer decoder for controlling the output of base layer decoded pictures.
    // BaseLayerOutputFlag equal to 1 indicates that the base layer is an output layer.
    // BaseLayerOutputFlag equal to 0 indicates that the base layer is not an output layer.

    // The variable LayerInitializedFlag[ i ] is set equal to 0 for all values of i from 0
    // to vps_max_layer_id, inclusive, and the variable FirstPicInLayerDecodedFlag[ i ] is set
    // equal to 0 for all values of i from 0 to vps_max_layer_id, inclusive.

    for (Int i = 0; i <= m_vps->getVpsMaxLayerId(); i++ )
    {
      m_layerInitilizedFlag       [ i ] = false;
      m_firstPicInLayerDecodedFlag[ i ] = false;
    }
  }
}

Void TAppDecTop::xC522OutputAndRemOfPicsFromDpb( )
{
  ////////////////////////////////////////////////////////////////////////////////
  // C.5.2.2 Output and removal of pictures from the DPB
  ////////////////////////////////////////////////////////////////////////////////

//  The output and removal of pictures from the DPB before the decoding of the current picture
//  (but after parsing the slice header of the first slice of the current picture) happens instantaneously
//  when the first decoding unit of the access unit containing the current picture is removed from the CPB and proceeds as follows:
//  - The decoding process for RPS as specified in clause 8.3.2 is invoked.

  Int nuhLayerId = m_curPic->getLayerId();
  const TComSPS* sps = m_curPic->getSlice(0)->getSPS();
  assert( nuhLayerId == 0 );

  if( ( m_curPic->isIrap() && m_curPic->getNoRaslOutputFlag() == 1) && m_curPic->getDecodingOrder() != 0 )
  {
    // - If the current picture is an IRAP picture with NoRaslOutputFlag equal to 1 that is not picture 0,
    //   the following ordered steps are applied:

    // 1.  The variable NoOutputOfPriorPicsFlag is derived for the decoder under test as follows:
    Int noOutputOfPriorPicsFlag;
    if( m_curPic->isCra() )
    {
      //-  If the current picture is a CRA picture, NoOutputOfPriorPicsFlag is set equal to 1
      // (regardless of the value of no_output_of_prior_pics_flag).
      noOutputOfPriorPicsFlag = true;
    }
    else if ( 0  )
    {
      // TBD
      //- Otherwise, if the value of pic_width_in_luma_samples, pic_height_in_luma_samples, chroma_format_idc,
      //  separate_colour_plane_flag, bit_depth_luma_minus8, bit_depth_chroma_minus8 or sps_max_dec_pic_buffering_minus1[ HighestTid ]
      //  derived from the active SPS is different from the value of pic_width_in_luma_samples, pic_height_in_luma_samples,
      //  chroma_format_idc, separate_colour_plane_flag, bit_depth_luma_minus8, bit_depth_chroma_minus8 or
      //  sps_max_dec_pic_buffering_minus1[ HighestTid ], respectively, derived from the SPS active for the preceding picture,
      //  NoOutputOfPriorPicsFlag may (but should not) be set to 1 by the decoder under test,
      //  regardless of the value of no_output_of_prior_pics_flag.
      //     NOTE - Although setting NoOutputOfPriorPicsFlag equal to no_output_of_prior_pics_flag is preferred under these conditions,
      //            the decoder under test is allowed to set NoOutputOfPriorPicsFlag to 1 in this case.
    }
    else
    {
      noOutputOfPriorPicsFlag = m_curPic->getSlice(0)->getNoOutputPriorPicsFlag();
    }
    //  2.  The value of NoOutputOfPriorPicsFlag derived for the decoder under test is applied for the HRD as follows:
    if (noOutputOfPriorPicsFlag)
    {
      //-  If NoOutputOfPriorPicsFlag is equal to 1, all picture storage buffers in the DPB are emptied
      //   without output of the pictures they contain and the DPB fullness is set equal to 0.
      m_dpb.emptySubDpb( nuhLayerId );
    }
    else if (!noOutputOfPriorPicsFlag)
    {
      //  -  Otherwise (NoOutputOfPriorPicsFlag is equal to 0), all picture storage buffers containing a picture that
      //     is marked as "not needed for output" and "unused for reference" are emptied (without output) and all non-empty
      //     picture storage buffers in the DPB are emptied by repeatedly invoking the "bumping" process specified in clause C.5.2.4
      //     and the DPB fullness is set equal to 0.

      m_dpb.emptySubDpbNotNeedForOutputAndUnusedForRef( nuhLayerId );
      while( m_dpb.getSubDpb( nuhLayerId, false  )->size() != 0 )
      {
        xC524Bumping();
      }
    }
  }
  else if ( !( m_curPic->isIrap() && m_curPic->getNoRaslOutputFlag() == 0 ) )
  {
    //  -  Otherwise (the current picture is not an IRAP picture with NoRaslOutputFlag equal to 1),
    //     all picture storage buffers containing a picture which are marked as "not needed for output" and "unused for reference"
    //     are emptied (without output). For each picture storage buffer that is emptied, the DPB fullness is decremented by one.

    m_dpb.emptySubDpbNotNeedForOutputAndUnusedForRef( nuhLayerId );

    Bool repeat = true;

    while( repeat )
    {
      TComSubDpb* dpb = m_dpb.getSubDpb( nuhLayerId, false );
      TComList<TComPic*> picsMarkedForOutput = dpb->getPicsMarkedNeedForOutput();
      // When one or more of the following conditions are true, the "bumping" process specified in clause C.5.2.4
      // is invoked repeatedly while further decrementing the DPB fullness by one for each additional picture storage buffer that
      // is emptied, until none of the following conditions are true:

      // -  The number of pictures in the DPB that are marked as "needed for output" is greater
      //    than sps_max_num_reorder_pics[ HighestTid ].
      Bool cond1 = ( picsMarkedForOutput.size() > sps->getSpsMaxNumReorderPics( m_highestTid ) );

      //  -  sps_max_latency_increase_plus1[ HighestTid ] is not equal to 0 and there is at least one picture in the DPB
      //     that is marked as "needed for output" for which the associated variable PicLatencyCount is greater than or equal
      //     to SpsMaxLatencyPictures[ HighestTid ].
      Bool anyPicWithGtOrEqLatencyCnt = false;
      for (TComList<TComPic*>::iterator itP = picsMarkedForOutput.begin(); itP != picsMarkedForOutput.end() && !anyPicWithGtOrEqLatencyCnt; itP++ )
      {
        anyPicWithGtOrEqLatencyCnt = anyPicWithGtOrEqLatencyCnt || ( (*itP)->getPicLatencyCount() >= sps->getSpsMaxLatencyPictures( m_highestTid ));
      }
      Bool cond2 = ( sps->getSpsMaxLatencyIncreasePlus1( m_highestTid ) != 0 ) && anyPicWithGtOrEqLatencyCnt;

      //  -  The number of pictures in the DPB is greater than or equal to sps_max_dec_pic_buffering_minus1[ HighestTid ] + 1.
      Bool cond3 = ( dpb->size() >= ( sps->getSpsMaxDecPicBufferingMinus1( m_highestTid ) + 1 ));

      if ( cond1 || cond2 || cond3 )
      {
        xC524Bumping();
      }
      else
      {
        repeat = false;
      }
    }
  }
}

Void TAppDecTop::xC523PicDecMarkAddBumpAndStor()
{
  ///////////////////////////////////////////////////////////////////////////////////////
  // C.5.2.3 Picture decoding, marking, additional bumping and storage  
  ///////////////////////////////////////////////////////////////////////////////////////
  
  Int nuhLayerId = m_curPic->getLayerId();
  const TComSPS* sps = m_curPic->getSlice(0)->getSPS();
  // The processes specified in this clause happen instantaneously when the last decoding unit of access unit n containing the
  // current picture is removed from the CPB.
  if (m_curPic->getPicOutputFlag() )
  {
    // When the current picture has PicOutputFlag equal to 1, for each picture in the DPB that is marked as "needed for output"
    // and follows the current picture in output order, the associated variable PicLatencyCount is set equal to PicLatencyCount + 1.
    TComSubDpb* dpb = m_dpb.getSubDpb( nuhLayerId, false);

    for (TComSubDpb::iterator itP = dpb->begin(); itP != dpb->end(); itP++)
    {
      TComPic* pic = (*itP);
      if ( pic->getOutputMark() && pic->getPOC() > m_curPic->getPOC() )
      {
        pic->setPicLatencyCount( pic->getPicLatencyCount() + 1 );
      }
    }
  }

  // The current picture is considered as decoded after the last decoding unit of the picture is decoded.
  // The current decoded picture is stored in an empty picture storage buffer in the DPB and the following applies:
  m_dpb.addNewPic( m_curPic );


  if (m_curPic->getPicOutputFlag())
  {
    // - If the current decoded picture has PicOutputFlag equal to 1, it is marked as "needed for output" and its associated
    //   variable PicLatencyCount is set equal to 0.
    m_curPic->setOutputMark( true );
    m_curPic->setPicLatencyCount( 0 );
  }
  else if (!m_curPic->getPicOutputFlag() )
  {
    // - Otherwise (the current decoded picture has PicOutputFlag equal to 0), it is marked as "not needed for output".
    m_curPic->setOutputMark( false );
  }

  // The current decoded picture is marked as "used for short-term reference".
  m_curPic->markAsUsedForShortTermReference();

  Bool repeat = true;

  while( repeat )
  {
    TComSubDpb* dpb = m_dpb.getSubDpb( nuhLayerId, false );
    TComList<TComPic*> picsMarkedForOutput = dpb->getPicsMarkedNeedForOutput();

    // When one or more of the following conditions are true, the "bumping" process specified in clause C.5.2.4
    // is invoked repeatedly until none of the following conditions are true:

    // - The number of pictures in the DPB that are marked as "needed for output" is greater than sps_max_num_reorder_pics[ HighestTid ].
    Bool cond1 = ( picsMarkedForOutput.size() > sps->getSpsMaxNumReorderPics( m_highestTid ) );

    // - sps_max_latency_increase_plus1[ HighestTid ] is not equal to 0 and there is at least one picture in the DPB that is marked
    //   as "needed for output" for which the associated variable PicLatencyCount that is greater than or equal to
    //   SpsMaxLatencyPictures[ HighestTid ].
    Bool anyPicWithGtOrEqLatencyCnt = false;
    for (TComList<TComPic*>::iterator itP = picsMarkedForOutput.begin(); itP != picsMarkedForOutput.end() && !anyPicWithGtOrEqLatencyCnt; itP++ )
    {
      anyPicWithGtOrEqLatencyCnt = anyPicWithGtOrEqLatencyCnt || ( (*itP)->getPicLatencyCount() >= sps->getSpsMaxLatencyPictures( m_highestTid ));
    }
    Bool cond2 = ( sps->getSpsMaxLatencyIncreasePlus1( m_highestTid ) != 0 ) && anyPicWithGtOrEqLatencyCnt;

    if ( cond1 || cond2 )
    {
      xC524Bumping();
    }
    else
    {
      repeat = false;
    }
  }
}

Void TAppDecTop::xC524Bumping( )
{
  ////////////////////////////////////////////////////////////////////////////////
  // C.5.2.4 "Bumping" process
  ////////////////////////////////////////////////////////////////////////////////

  // The "bumping" process consists of the following ordered steps:

  // 1.  The picture that is first for output is selected as the one having the smallest value of PicOrderCntVal of all pictures
  //     in the DPB marked as "needed for output".
  TComPic* pic = *(m_dpb.getSubDpb( 0, false)->getPicsMarkedNeedForOutput().begin()); // pics are sorted, so take first

  // 2.  The picture is cropped, using the conformance cropping window specified in the active SPS for the picture,
  //     the cropped picture is output, and the picture is marked as "not needed for output".
  xCropAndOutput( pic );
  pic->setOutputMark( false );

  //3.  When the picture storage buffer that included the picture that was cropped and output contains a picture marked
  //    as "unused for reference", the picture storage buffer is emptied.
  if (pic->getMarkedUnUsedForReference() )
  {
    m_dpb.removePic( pic );
  }

  // NOTE - For any two pictures picA and picB that belong to the same CVS and are output by the "bumping process", when picA
  //        is output earlier than picB, the value of PicOrderCntVal of picA is less than the value of PicOrderCntVal of picB.
}


Void TAppDecTop::xF813ComDecProcForACodedPic( DecProcPart curPart, Bool picPosInAuIndication )
{
  ////////////////////////////////////////////////////////////////////////////////
  // F.8.1.3  Common decoding process for a coded picture
  ////////////////////////////////////////////////////////////////////////////////

  // PicPosInAuIndication is interpreted based on curPart.
  // - If curPart is equal to START_PIC               , it indicates whether the current pic is the first in the current AU.
  // - Otherwise (if curPart is equal to FINALIZE_PIC), it indicates whether the current pic is the last  in the current AU.

  if (curPart == START_PIC )
  {
    Bool curPicIsFirstInAu = picPosInAuIndication;

    Int nuhLayerId = m_curPic->getLayerId();

    if ( !m_vps->getVpsBaseLayerInternalFlag() && m_vps->getVpsBaseLayerAvailableFlag() &&
      ( m_targetDecLayerSetIdx >= 0 && m_targetDecLayerSetIdx <= m_vps->getVpsNumLayerSetsMinus1() ) &&
      m_curPic->getSlice(0)->getTemporalId() <= m_vps->getSubLayersVpsMaxMinus1( 0 ) && curPicIsFirstInAu )
    {
      // When vps_base_layer_internal_flag is equal to 0, vps_base_layer_available_flag is equal to 1,
      // TargetDecLayerSetIdx is in the range of 0 to vps_num_layer_sets_minus1, inclusive,
      // TemporalId is less than or equal to sub_layers_vps_max_minus1[ 0 ], and the current picture
      // is the first coded picture of an access unit, clause F.8.1.8 is invoked prior to decoding the current picture.

      assert( false ); //TBD
    }

    //  When the current picture is an IRAP picture, the variable HandleCraAsBlaFlag is derived as specified in the following:
    if ( m_curPic->isIrap() )
    {
      if ( m_handleCraAsBlaFlagSetByExtMeans )
      {
        // If some external means not specified in this Specification is available to set the variable HandleCraAsBlaFlag to
        // a value for the current picture, the variable HandleCraAsBlaFlag is set equal to the value provided by the external means.
      }
      else
      {
        // - Otherwise, the variable HandleCraAsBlaFlag is set equal to 0.
        m_handleCraAsBlaFlag = false;
      }
    }

    if ( m_curPic->isIrap() && nuhLayerId == m_smallestLayerId )
    {
      // When the current picture is an IRAP picture and has nuh_layer_id equal to SmallestLayerId, the following applies:
      // The variable NoClrasOutputFlag is specified as follows:

      if( m_firstSliceInBitstream )
      {
        //  - If the current picture is the first picture in the bitstream, NoClrasOutputFlag is set equal to 1.
        m_curPic->setNoClrasOutputFlag(true );
      }
      else if( m_eosInLayer[ 0 ] || m_eosInLayer[ nuhLayerId ] )
      {
        //  - Otherwise, if the current picture is included in the first access unit that follows an access unit
        //    including an end of sequence NAL unit with nuh_layer_id equal to SmallestLayerId or 0 in decoding order,
        //    NoClrasOutputFlag is set equal to 1.

        m_curPic->setNoClrasOutputFlag(true );
      }
      else if ( m_curPic->isBla() || (m_curPic->isCra() && m_handleCraAsBlaFlag ))
      {
        //  - Otherwise, if the current picture is a BLA picture or a CRA picture with HandleCraAsBlaFlag equal to 1,
        //    NoClrasOutputFlag is set equal to 1.
        m_curPic->setNoClrasOutputFlag(true );
      }
      else if ( m_curPic->isIdr() && m_curPic->getSlice(0)->getCrossLayerBlaFlag() )
      {
        //  - Otherwise, if the current picture is an IDR picture with cross_layer_bla_flag is equal to 1,
        //    NoClrasOutputFlag is set equal to 1.
        m_curPic->setNoClrasOutputFlag(true );
      }
      else if ( m_noClrasOutputFlagSetByExtMeans )
      {
        m_curPic->setNoClrasOutputFlag( m_noClrasOutputFlag );
        //  - Otherwise, if some external means, not specified in this Specification, is available to set NoClrasOutputFlag,
        //    NoClrasOutputFlag is set by the external means.
      }
      else
      {
        //  - Otherwise, NoClrasOutputFlag is set equal to 0.
        m_curPic->setNoClrasOutputFlag(false );
      }

      //-  When NoClrasOutputFlag is equal to 1, the variable LayerInitializedFlag[ i ] is set equal to 0 for all values
      //  of i from 0 to vps_max_layer_id, inclusive, and the variable FirstPicInLayerDecodedFlag[ i ] is set equal to 0
      //  for all values of i from 0 to vps_max_layer_id, inclusive.
      if ( m_curPic->getNoClrasOutputFlag( ) )
      {
        for (Int i = 0; i <= m_vps->getVpsMaxLayerId(); i++)
        {
          m_layerInitilizedFlag       [i] = false;
          m_firstPicInLayerDecodedFlag[i] = false;
        }
      }
    }

    // The variables LayerResetFlag and dolLayerId are derived as follows:
    Int dolLayerId = -1;
    if ( m_curPic->isIrap() && nuhLayerId > m_smallestLayerId )
    {
      // - If the current picture is an IRAP picture and has nuh_layer_id nuhLayerId greater than SmallestLayerId,
      //   the following applies:

      if( m_eosInLayer[ nuhLayerId ] )
      {
        // -  If the current picture is the first picture, in decoding order, that follows an end of sequence NAL unit with
        //    nuh_layer_id equal to nuhLayerId, LayerResetFlag is set equal to 1 and dolLayerId is set equal to the nuh_layer_id
        //    value of the current NAL unit.

        m_layerResetFlag = true;
        dolLayerId = nuhLayerId;
      }
      else  if( ( m_curPic->isCra() && m_handleCraAsBlaFlag ) ||
        (m_curPic->isIdr() && m_curPic->getSlice(0)->getCrossLayerBlaFlag() ) || m_curPic->isBla() )
      {
        // - Otherwise, if the current picture is a CRA picture with HandleCraAsBlaFlag equal to 1, an IDR picture with
        //   cross_layer_bla_flag is equal to 1 or a BLA picture, LayerResetFlag is set equal to 1 and dolLayerId is set
        //   equal to the nuh_layer_id value of the current NAL unit.
        m_layerResetFlag = true;
        dolLayerId = nuhLayerId;
      }
      else
      {
        // -   Otherwise, LayerResetFlag is set equal to 0.
        m_layerResetFlag = false;
      }

      // NOTE 1 - An end of sequence NAL unit, a CRA picture with HandleCraAsBlaFlag equal to 1, an IDR picture with
      // cross_layer_bla_flag equal to 1, or a BLA picture, each with nuh_layer_id nuhLayerId greater than SmallestLayerId,
      // may be present to indicate a discontinuity of the layer with nuh_layer_id equal to nuhLayerId and its predicted layers.

      if (m_layerResetFlag )
      {
        //When LayerResetFlag is equal to 1, the following applies:
        //  - The values of LayerInitializedFlag and FirstPicInLayerDecodedFlag are updated as follows:

        for( Int i = 0; i < m_vps->getNumPredictedLayers( dolLayerId ); i++ )
        {
          Int iLayerId = m_vps->getIdPredictedLayer(  dolLayerId , i );
          m_layerInitilizedFlag       [ iLayerId ] = false;
          m_firstPicInLayerDecodedFlag[ iLayerId ] = false;
        }

        //  - Each picture that is in the DPB and has nuh_layer_id equal to dolLayerId is marked as "unused for reference".
        m_dpb.markSubDpbAsUnusedForReference( dolLayerId );

        //  - When NumPredictedLayers[ dolLayerId ] is greater than 0, each picture that is in the DPB and has nuh_layer_id
        //    equal to any value of IdPredictedLayer[ dolLayerId ][ i ] for the values of i in the range of 0 to
        //    NumPredictedLayers[ dolLayerId ] - 1, inclusive, is marked as "unused for reference".

        for( Int i = 0; i <= m_vps->getNumPredictedLayers( dolLayerId )- 1; i++  )
        {
          m_dpb.markSubDpbAsUnusedForReference( m_vps->getIdPredictedLayer( dolLayerId, i ) );
        }
      }
    }
    else
    {
      // - Otherwise, LayerResetFlag is set equal to 0.
      m_layerResetFlag = false;
    }

    if ( m_curPic->isIrap() )
    {
      // When the current picture is an IRAP picture, the following applies:

      if ( m_curPic->isIdr() || m_curPic->isBla() || m_curPic->getDecodingOrder() == 0 || m_eosInLayer[nuhLayerId] )
      {
        // - If the current picture with a particular value of nuh_layer_id is an IDR picture, a BLA picture, the first picture
        //   with that particular value of nuh_layer_id in the bitstream in decoding order or the first picture with that
        //   particular value of nuh_layer_id that follows an end of sequence NAL unit with that particular value of nuh_layer_id
        //   in decoding order, the variable NoRaslOutputFlag is set equal to 1.

        m_curPic->setNoRaslOutputFlag(  true );
      }
      else if ( m_layerInitilizedFlag[ nuhLayerId ] == 0 && xAllRefLayersInitilized( nuhLayerId ) )
      {
        // Otherwise, if LayerInitializedFlag[ nuh_layer_id ] is equal to 0 and LayerInitializedFlag[ refLayerId ] is equal to 1
        // for all values of refLayerId equal to IdDirectRefLayer[ nuh_layer_id ][ j ], where j is in the range of 0 to
        // NumDirectRefLayers[ nuh_layer_id ] - 1, inclusive, the variable NoRaslOutputFlag is set equal to 1.

        m_curPic->setNoRaslOutputFlag(  true );
      }
      else
      {
        // - Otherwise, the variable NoRaslOutputFlag is set equal to HandleCraAsBlaFlag.
        m_curPic->setNoRaslOutputFlag(  m_handleCraAsBlaFlag );
      }
    }

    // The following applies for the decoding of the current picture:
    if ( m_curPic->isIrap() && m_curPic->getNoRaslOutputFlag() && (
      ( nuhLayerId == 0 ) ||
      ( m_layerInitilizedFlag[ nuhLayerId ] == 0   && m_vps->getNumDirectRefLayers( nuhLayerId ) == 0  ) ||
      ( m_layerInitilizedFlag[ nuhLayerId ] == 0 && xAllRefLayersInitilized( nuhLayerId ) )
      ) )
    {
      // When the current picture is an IRAP picture with NoRaslOutputFlag equal to 1 and one of the following conditions is true,
      // LayerInitializedFlag[ nuh_layer_id ] is set equal to 1:
      // - nuh_layer_id is equal to 0.
      // - LayerInitializedFlag[ nuh_layer_id ] is equal to 0 and NumDirectRefLayers[ nuh_layer_id ] is equal to 0.
      // - LayerInitializedFlag[ nuh_layer_id ] is equal to 0 and LayerInitializedFlag[ refLayerId ] is equal to 1 for all values of
      //   refLayerId equal to IdDirectRefLayer[ nuh_layer_id ][ j ], where j is in the range of 0 to NumDirectRefLayers[ nuh_layer_id ] - 1, inclusive.

      m_layerInitilizedFlag[ nuhLayerId ] = true;
    }

    // The following applies for the decoding of the current picture:

    if ( nuhLayerId == 0 )
    {
      //  If the current picture has nuh_layer_id equal to 0, the decoding process for the current picture takes as inputs the syntax elements
      //  and upper-case variables from clause F.7 and the decoding process for a coded picture with nuh_layer_id equal to 0 as specified in clause F.8.1.4
      //  is invoked.

      xF814decProcForCodPicWithLIdZero( curPart );
    }
    else
    {
      Bool decodedPicWithLayerIdZeroProvided = false;
      if ( !m_vps->getVpsBaseLayerInternalFlag() && m_vps->getVpsBaseLayerAvailableFlag() &&
        ( m_targetDecLayerSetIdx >= 0 && m_targetDecLayerSetIdx <= m_vps->getVpsNumLayerSetsMinus1() ) &&
        m_curPic->getSlice(0)->getTemporalId() <= m_vps->getSubLayersVpsMaxMinus1( 0 ) && curPicIsFirstInAu
        && decodedPicWithLayerIdZeroProvided )
      {
        assert( false ); //TBD
        //TBD: Hybrid scalability
        //  When vps_base_layer_internal_flag is equal to 0, vps_base_layer_available_flag is equal to 1,
        //  TargetDecLayerSetIdx is in the range of 0 to vps_num_layer_sets_minus1, inclusive,
        //  TemporalId is less than or equal to sub_layers_vps_max_minus1[ 0 ], the current picture
        //  is the first coded picture of an access unit, and a decoded picture with nuh_layer_id equal
        //  to 0 is provided by external means for the current access unit, clause F.8.1.9 is invoked
        //  after the decoding of the slice segment header of the first slice segment, in decoding order,
        //  of the current picture, but prior to decoding any slice segment of the first
        //  coded picture of the access unit.
      }

      m_decProcPocAndRps = ANNEX_F;
      // For the decoding of the slice segment header of the first slice segment, in decoding order, of the current picture,
      // the decoding process for starting the decoding of a coded picture with nuh_layer_id greater than 0 specified in
      // clause F.8.1.5 is invoked.  --> This is done in the loop when receiving slices.

      // The decoding process is already selected before.
    }
  }
  else if( curPart == FINALIZE_PIC )
  {
    Bool curPicIsLastInAu = picPosInAuIndication;

    if (m_curPic->getLayerId() == 0 )
    {
      xF814decProcForCodPicWithLIdZero( curPart );
    }
    else
    {
      // - After all slices of the current picture have been decoded, the decoding process for ending the decoding of a
      //   coded picture with nuh_layer_id greater than 0 specified in clause F.8.1.6 is invoked.
      xF816decProcEndDecOfCodPicLIdGrtZero( );      
    }

    TComSlice* slice = m_curPic->getSlice(0);
    const TComVPS* vps = slice->getVPS();

    if ( curPicIsLastInAu )
    {
      //When the current picture is the last coded picture in an access unit in BitstreamToDecode, the following steps apply after
      // the decoding of the current picture, prior to the decoding of the next picture:

      //-  PicOutputFlag is updated as follows:
      TComPic* picAtOutputLayer = NULL;
      Int outputLayerId = -1;

      // Try to find pic at output layer.
      if( vps->getAltOutputLayerFlag( m_targetOptLayerSetIdx ) )
      {
        // When alt_output_layer_flag is equal to 1, the target output layer set can only contain one output layer.
        assert( vps->getNumOutputLayersInOutputLayerSet( m_targetOptLayerSetIdx ) == 1 );
        outputLayerId = vps->getTargetOptLayerIdList(m_targetOptLayerSetIdx)[0];
        picAtOutputLayer = m_curAu.getPic( outputLayerId );
      }

      if ( vps->getAltOutputLayerFlag( m_targetOptLayerSetIdx ) &&
        ( picAtOutputLayer == NULL || (picAtOutputLayer != NULL && !picAtOutputLayer->getPicOutputFlag() ) ) )
      {
        // If alt_output_layer_flag[ TargetOlsIdx ] is equal to 1 and the current access unit either does
        // not contain a picture at the output layer or contains a picture at the output layer that has PicOutputFlag equal to 0:

        // - The list nonOutputLayerPictures is set to be the list of the pictures of the access unit with PicOutputFlag equal to 1 and
        //   with nuh_layer_id values among the nuh_layer_id values of the reference layers of the output layer.
        TComList<TComPic*> nonOutputLayerPictures;
        for( TComList<TComPic*>::iterator itP= m_curAu.begin();  itP != m_curAu.end() ; itP++ )
        {
          TComPic* pic = (*itP);
          Bool isRefernceLayerOfOutputLayer = false;
          for ( Int i = 0; i < vps->getNumRefLayers( outputLayerId ) && !isRefernceLayerOfOutputLayer; i++ )
          {
            if ( pic->getLayerId() == vps->getIdRefLayer(outputLayerId, i) )
            {
              isRefernceLayerOfOutputLayer = true;
            }
          }

          if ( pic->getPicOutputFlag() && isRefernceLayerOfOutputLayer )
          {
            nonOutputLayerPictures.pushBack( pic );
          }
        }

        if (nonOutputLayerPictures.size() != 0 )
        {
          // -  When the list nonOutputLayerPictures is not empty,
          //    The picture with the highest nuh_layer_id value among the list nonOutputLayerPictures is removed from the list nonOutputLayerPictures.
          //    As in AU the picture with the highest layer id is the last
          nonOutputLayerPictures.pop_back();
        }

        //  -  PicOutputFlag for each picture that is included in the list nonOutputLayerPictures is set equal to 0.
        for( TComList<TComPic*>::iterator itP= nonOutputLayerPictures.begin();  itP != nonOutputLayerPictures.end() ; itP++ )
        {
          (*itP)->setPicOutputFlag( false );
        }
      }
      else
      {
        //Otherwise, PicOutputFlag for pictures that are not included in an output layer is set equal to 0.
        for( TComList<TComPic*>::iterator itP= m_curAu.begin();  itP != m_curAu.end() ; itP++ )
        {
          TComPic* pic = (*itP);

          Bool includedInOutputLayer = false;
          for (Int i = 0 ; i < vps->getNumOutputLayersInOutputLayerSet( m_targetOptLayerSetIdx ) && !includedInOutputLayer; i++)
          {
            includedInOutputLayer = ( pic->getLayerId() ==  vps->getTargetOptLayerIdList(m_targetOptLayerSetIdx)[i]);
          }

          if ( !includedInOutputLayer )
          {
            pic->setPicOutputFlag( false );
          }
        }
      }

      // When vps_base_layer_internal_flag is equal to 0, vps_base_layer_available_flag is equal to 1 and
      // TargetDecLayerSetIdx is in the range of 0 to vps_num_layer_sets_minus1, inclusive

      if( !vps->getVpsBaseLayerInternalFlag() && vps->getVpsBaseLayerAvailableFlag() && ( m_targetDecLayerSetIdx >= 0 && m_targetDecLayerSetIdx <= vps->getVpsNumLayerSetsMinus1()   ) )
      {
        if( !m_baseLayerOutputFlag )
        {
          // Check if base layer is a reference layer of the output layer
          // and if the access unit does not contain a picture at any other reference layer of the output layer
          Bool baseLayerIsRefOfOutputLayer = false;
          Bool auDoesNotContainPicAtAnyOtherRefLayerOfOptLayer = true;
          if ( vps->getAltOutputLayerFlag( m_targetOptLayerSetIdx ))
          {
            assert( outputLayerId >= 0 );

            for (Int i = 0; i < vps->getNumRefLayers( outputLayerId ); i++ )
            {
              Int refLayerId = vps->getIdRefLayer(outputLayerId, i);
              if( refLayerId == 0 )
              {
                baseLayerIsRefOfOutputLayer = true;
              }
              else
              {
                if ( m_curAu.getPic( refLayerId ) != NULL )
                {
                  auDoesNotContainPicAtAnyOtherRefLayerOfOptLayer = false;
                }
              }
            }
          }

          // If alt_output_layer_flag[ TargetOlsIdx ] is equal to 1, the base layer is a reference layer of the output layer,
          // the access unit does not contain a picture at the output layer or contains a picture at the output layer that has
          // PicOutputFlag equal to 0, and the access unit does not contain a picture at any other reference layer of the output layer,
          // BaseLayerPicOutputFlag is set equal to 1

          if ( vps->getAltOutputLayerFlag( m_targetOptLayerSetIdx ) && baseLayerIsRefOfOutputLayer
            && ( ( picAtOutputLayer == NULL ) || ( picAtOutputLayer != NULL && !picAtOutputLayer->getPicOutputFlag()  ) )
            &&  auDoesNotContainPicAtAnyOtherRefLayerOfOptLayer )
          {
            m_baseLayerPicOutputFlag = true;
          }
          else
          {
            m_baseLayerPicOutputFlag = false;
          }
        }
        else
        {
          m_baseLayerPicOutputFlag = true;
        }

        // NOTE 3 - The BaseLayerPicOutputFlag for each access unit is to be sent by an external means
        // to the base layer decoder for controlling the output of base layer decoded pictures.
        // BaseLayerPicOutputFlag equal to 1 for an access unit specifies that the base layer picture of the access unit is to be output.
        // BaseLayerPicOutputFlag equal to 0 for an access unit specifies that the base layer picture of the access unit is not to be output.

        //  The sub-DPB for the layer with nuh_layer_id equal to 0 is set to be empty.
        m_dpb.emptySubDpb( 0 );
      }

      // The variable AuOutputFlag that is associated with the current access unit is derived as follows:

      // Derive if at least one picture in the current access unit has PicOutputFlag equal to 1
      Bool atLeastOnePicInAuWithPicOptFlagOne = false;
      for( TComList<TComPic*>::iterator itP= m_curAu.begin();  itP != m_curAu.end() ; itP++ )
      {
        if ( (*itP)->getPicOutputFlag() )
        {
          atLeastOnePicInAuWithPicOptFlagOne = true;
        }
      }

      //If at least one picture in the current access unit has PicOutputFlag equal to 1
      if ( atLeastOnePicInAuWithPicOptFlagOne )
      {
        m_auOutputFlag = true;
      }
      else
      {
        m_auOutputFlag = false;
      }

      // The variable PicLatencyCount that is associated with the current access unit is set equal to 0.
      m_curAu.setPicLatencyCount( 0 );

      if ( m_auOutputFlag )
      {
        // for each access unit in the DPB
        // that has at least one picture marked as "needed for output" and
        // follows the current access unit in output order, the associated
        // variable PicLatencyCount is set equal to PicLatencyCount + 1.

        TComList<TComAu*> aus = m_dpb.getAusHavingPicsMarkedForOutput(); // <-- current AU is actually not yet in DPB, but this does not matter here.

        for(TComList<TComAu*>::iterator itA= aus.begin(); ( itA!=aus.end()); itA++)
        {
          if( m_curAu.getPoc() < (*itA)->getPoc())
          {
            (*itA)->setPicLatencyCount( (*itA)->getPicLatencyCount() + 1 );
          }
        }
      }
    }
  }
}

Void TAppDecTop::xF814decProcForCodPicWithLIdZero( DecProcPart curPart )
{
  ////////////////////////////////////////////////////////////////////////////////
  // F.8.1.4 Decoding process for a coded picture with nuh_layer_id equal to 0
  ////////////////////////////////////////////////////////////////////////////////

  x813decProcForCodPicWithLIdZero( curPart );

  if (curPart == START_PIC )
  {
    m_decProcPocAndRps = ANNEX_F;
  }
  else if (curPart == FINALIZE_PIC )
  {
    if ( !m_firstPicInLayerDecodedFlag[0] )
    {
      m_firstPicInLayerDecodedFlag[0] = true;
    }
  }
}

Void TAppDecTop::xF13521InitDpb()
{
  ////////////////////////////////////////////////////////////////////////////////
  // F.13.5.2.1 General
  ////////////////////////////////////////////////////////////////////////////////

  const TComDpbSize* dpbSize = m_vps->getDpbSize();
  m_maxNumReorderPics       = dpbSize->getMaxVpsNumReorderPics      ( m_targetOptLayerSetIdx, m_highestTid );
  m_maxLatencyIncreasePlus1 = dpbSize->getMaxVpsLatencyIncreasePlus1( m_targetOptLayerSetIdx, m_highestTid );
  m_maxLatencyValue         = dpbSize->getVpsMaxLatencyPictures     ( m_targetOptLayerSetIdx, m_highestTid );

  for(Int i = 0; i < MAX_NUM_LAYER_IDS; i++)
  {
    m_maxDecPicBufferingMinus1[ i ] = MIN_INT;
  }
  for( Int i = 0; i <= m_vps->getMaxLayersMinus1(); i++ )
  {
    Int currLayerId = m_vps->getLayerIdInNuh( i );
    for(Int layerIdx = 0 ; layerIdx < m_vps->getNumLayersInIdList( m_targetDecLayerSetIdx); layerIdx++ )
    {
      if( m_vps->getLayerSetLayerIdList( m_targetDecLayerSetIdx, layerIdx ) == currLayerId )
      {
        m_maxDecPicBufferingMinus1[currLayerId] = dpbSize->getMaxVpsDecPicBufferingMinus1( m_targetDecLayerSetIdx, layerIdx, m_highestTid );
      }
    }
  }
}

Void TAppDecTop::xF13522OutputAndRemOfPicsFromDpb( Bool beforePocDerivation )
{
  ////////////////////////////////////////////////////////////////////////////////
  // F.13.5.2.2 Output and removal of pictures from the DPB
  ////////////////////////////////////////////////////////////////////////////////

  if( beforePocDerivation )
  {
    // F.13.5.2.2
    // Part before POC and RPS derivation.

    if( m_curPic->getDecodingOrder() != 0 )
    {
      // When the current picture is not picture 0 in the current layer,
      // the output and removal of pictures in the current layer, with nuh_layer_id equal to currLayerId,
      // from the DPB before the decoding of the current picture, i.e., picture n, but after parsing the
      // slice header of the first slice of the current picture and before the invocation of the decoding
      // process for picture order count, happens instantaneously when the first decoding unit of the current
      // picture is removed from the CPB and proceeds as follows:

      if( m_curPic->getIsPocResettingPic() )
      {
        // When the current picture is a POC resetting picture, all pictures in the DPB that do
        // not belong to the current access unit and that are marked as "needed for output" are
        // output, starting with pictures with the smallest value of PicOrderCntVal of all pictures
        // excluding those in the current access unit in the DPB, in ascending order of the PicOrderCntVal
        // values, and pictures with the same value of PicOrderCntVal are output in ascending order
        // of the nuh_layer_id values. When a picture is output, it is cropped using the conformance cropping
        // window specified in the active SPS for the picture, the cropped picture is output, and
        // the picture is marked as "not needed for output"

        TComList<TComAu*>* aus = m_dpb.getAus(); // Theses are sorted by POC.
        for (TComList<TComAu*>::iterator itA = aus->begin(); itA != aus->end(); itA++ )
        {
          //Pictures in AUs are sorted by nuh_layer_id;
          for (TComAu::iterator itP = (*itA)->begin(); itP != (*itA)->end(); itP++ )
          {
            TComPic* pic = (*itP);
            if ( !m_curAu.containsPic( pic ) )
            {
              xCropAndOutput( pic );
              pic->setOutputMark( false );
            }
          }
        }
      }
    }
  }
  else if ( !beforePocDerivation )
  {
    //  The variable listOfSubDpbsToEmpty is derived as follows:

    Int nuhLayerId = m_curPic->getLayerId();
    TComList<TComSubDpb*> listOfSubDpbsToEmpty;

    if ( m_newVpsActivatedbyCurAu && ( m_curPic->isIrap()&& nuhLayerId == m_smallestLayerId
      && m_curPic->getNoRaslOutputFlag() && m_curPic->getNoClrasOutputFlag() ) )
    {
      // If a new VPS is activated by the current access unit or the current picture is IRAP picture
      // with nuh_layer_id equal to SmallestLayerId,   NoRaslOutputFlag equal to 1, and NoClrasOutputFlag equal to 1,
      // listOfSubDpbsToEmpty is set equal to all the sub-DPBs.
      listOfSubDpbsToEmpty = (*m_dpb.getSubDpbs());
    }
    else if (m_curPic->isIrap() && m_vps->getNumDirectRefLayers( nuhLayerId ) == 0 &&
      nuhLayerId > m_smallestLayerId && m_curPic->getNoRaslOutputFlag() && m_layerResetFlag  )
    {
      // Otherwise, if the current picture is an IRAP picture with any nuh_layer_id value indepLayerId
      // such that NumDirectRefLayers[ indepLayerId ] is equal to 0 and indepLayerId is greater than
      // SmallestLayerId, and with NoRaslOutputFlag equal to 1, and LayerResetFlag is equal to 1,

      // listOfSubDpbsToEmpty is set equal to the sub-DPBs containing the current layer and the sub-DPBs
      // containing the predicted layers of the current layer.

      listOfSubDpbsToEmpty.pushBack( m_dpb.getSubDpb( nuhLayerId, false  ) );
      for( Int i = 0; i < m_vps->getNumPredictedLayers( nuhLayerId ); i++  )
      {
        listOfSubDpbsToEmpty.pushBack( m_dpb.getSubDpb( m_vps->getIdPredictedLayer( nuhLayerId, i), false  ) );
      }
    }
    else
    {
      // Otherwise, crossLayerBufferEmptyFlag is set equal to 0.

      // The SPEC seems to have an issue here. Use current subDpb as in form F.13.3.2
      listOfSubDpbsToEmpty.pushBack( m_dpb.getSubDpb( nuhLayerId, false  ) );
    }

    // If the current picture is an IRAP picture with NoRaslOutputFlag equal to 1 and any of
    // the following conditions is true:
    //   - nuh_layer_id equal to SmallestLayerId,
    //   - nuh_layer_id of the current layer is greater than SmallestLayerId, and NumDirectRefLayers[ nuh_layer_id ]
    //     is equal to 0,

    if ( m_curPic->isIrap() && m_curPic->getNoRaslOutputFlag() && (
      ( nuhLayerId == m_smallestLayerId ) ||
      ( ( nuhLayerId > m_smallestLayerId ) && m_vps->getNumDirectRefLayers( nuhLayerId ) == 0 ) )
      )
    {
      // 1. The variable NoOutputOfPriorPicsFlag is derived for the decoder under test as follows:
      Bool noOutputOfPriorPicsFlag;
      if( m_curPic->isCra() )
      {
        noOutputOfPriorPicsFlag = true;
        // - If the current picture is a CRA picture, NoOutputOfPriorPicsFlag is set equal to 1
        // (regardless of the value of no_output_of_prior_pics_flag).
      }
      else if ( false )
      {
        // TBD
        // - Otherwise, if the value of pic_width_in_luma_samples, pic_height_in_luma_samples,
        //   chroma_format_idc, bit_depth_luma_minus8, bit_depth_chroma_minus8, separate_colour_plane_flag,
        //   or sps_max_dec_pic_buffering_minus1[ HighestTid ] derived from the active SPS for the current
        //   layer is different from the value of pic_width_in_luma_samples, pic_height_in_luma_samples,
        //   chroma_format_idc, bit_depth_luma_minus8, bit_depth_chroma_minus8, separate_colour_plane_flag,
        //   or sps_max_dec_pic_buffering_minus1[ HighestTid ], respectively, derived from the SPS that
        //   was active for the current layer when decoding the preceding picture in the current layer,
        //   NoOutputOfPriorPicsFlag may (but should not) be set equal to 1 by the decoder under test,
        //   regardless of the value of no_output_of_prior_pics_flag.
        //    NOTE - Although setting NoOutputOfPriorPicsFlag equal to no_output_of_prior_pics_flag is preferred
        //    under these conditions, the decoder under test is allowed to set NoOutputOfPriorPicsFlag to 1 in this case.
        // - Otherwise, NoOutputOfPriorPicsFlag is set equal to no_output_of_prior_pics_flag.

        // assert( 1 );
      }
      else
      {
        // - Otherwise, NoOutputOfPriorPicsFlag is set equal to no_output_of_prior_pics_flag.
        noOutputOfPriorPicsFlag = m_curPic->getSlice(0)->getNoOutputPriorPicsFlag();
      }

      // 2. The value of NoOutputOfPriorPicsFlag derived for the decoder under test is applied for the HRD as follows:
      if ( !noOutputOfPriorPicsFlag )
      {
        // - If NoOutputOfPriorPicsFlag is equal to 0, all non-empty picture storage buffers in all the sub-DPBs included
        //   in listOfSubDpbsToEmpty are output by repeatedly invoking the "bumping" process specified in clause
        //   F.13.5.2.4 until all these pictures are marked as "not needed for output".

        Bool repeat = true;
        while (repeat )
        {
          Bool allPicsMarkedNotNeedForOutput = true;
          for (TComList<TComSubDpb*>::iterator itS = listOfSubDpbsToEmpty.begin(); itS != listOfSubDpbsToEmpty.end() && allPicsMarkedNotNeedForOutput; itS++ )
          {
            allPicsMarkedNotNeedForOutput = allPicsMarkedNotNeedForOutput && ( (*itS)->areAllPicsMarkedNotNeedForOutput() );
          }

          if ( !allPicsMarkedNotNeedForOutput )
          {
            xF13524Bumping( m_dpb.getAusHavingPicsMarkedForOutput() );
          }
          else
          {
            repeat = false;
          }
        }
      }
      else
      {
        // - Otherwise (NoOutputOfPriorPicsFlag is equal to 1), all picture storage buffers containing a picture
        //   that is marked as "not needed for output" and "unused for reference" are emptied (without output),
        //   all pictures that are contained in a sub-DPB included in listOfSubDpbsToEmpty are emptied, and the sub-DPB
        //   fullness of each sub-DPB is decremented by the number of picture storage buffers emptied in that sub-DPB.
        m_dpb.emptyNotNeedForOutputAndUnusedForRef();

        for( TComList<TComSubDpb*>::iterator iS = listOfSubDpbsToEmpty.begin(); iS != listOfSubDpbsToEmpty.end(); iS++)
        {
          m_dpb.emptySubDpbs( &listOfSubDpbsToEmpty );
        }
      }
    }
    else
    {
      // -  Otherwise, all picture storage buffers that contain a picture in the current layer and that are marked as
      //   "not needed for output" and "unused for reference" are emptied (without output). For each picture storage buffer that is emptied,
      //   the sub-DPB fullness is decremented by one.

      m_dpb.emptySubDpbNotNeedForOutputAndUnusedForRef( nuhLayerId );

      //    When one or more of the following conditions are true, the "bumping" process specified in clause F.13.5.2.4
      //    is invoked repeatedly until none of the following conditions are true:

      Bool repeat = true;
      while ( repeat )
      {
        TComList<TComAu*> aus = m_dpb.getAusHavingPicsMarkedForOutput();

        // The number of access units that contain at least one decoded picture in the DPB marked
        // as "needed for output" is greater than MaxNumReorderPics.
        Bool cond1 = ( aus.size() > m_maxNumReorderPics );

        // MaxLatencyIncreasePlus1 is not equal to 0 and there is at least one access unit
        // that contains at least one decoded picture in the DPB marked as "needed for output"
        // for which the associated variable PicLatencyCount is greater than or equal to MaxLatencyValue.
        Bool auWithGreaterLatencyCount = false;
        for(TComList<TComAu*>::iterator itA= aus.begin(); ( itA!=aus.end()) && !auWithGreaterLatencyCount ; itA++)
        {
          if ( (*itA)->getPicLatencyCount() > m_maxLatencyValue )
          {
            auWithGreaterLatencyCount = true;
          }
        }

        Bool cond2 = (m_maxLatencyIncreasePlus1 != 0 ) && auWithGreaterLatencyCount;

        // The number of pictures in the sub-DPB is greater than or equal to MaxDecPicBufferingMinus1 + 1.
        Bool cond3 = ( m_dpb.getSubDpb( nuhLayerId, false )->size() >= m_maxDecPicBufferingMinus1[ nuhLayerId ] + 1 );

        if ( cond1  || cond2 || cond3 )
        {
          xF13524Bumping( aus );
        }
        else
        {
          repeat = false;
        }
      }
    }
  }
}

Void TAppDecTop::xF13523PicDecMarkAddBumpAndStor( Bool curPicIsLastInAu )
{
  ////////////////////////////////////////////////////////////////////////////////
  // F.13.5.2.3 Picture decoding, marking, additional bumping and storage
  ////////////////////////////////////////////////////////////////////////////////

  const TComVPS* vps = m_curPic->getSlice(0)->getVPS();

  // The current picture is considered as decoded after the last decoding unit of
  // the picture is decoded. The current decoded picture is stored in an empty picture
  // storage buffer in the sub-DPB.

  m_dpb.addNewPic( m_curPic );

  if ( curPicIsLastInAu )
  {
    // When the current picture is the last picture in an access unit, the following applies
    // for each decoded picture with nuh_layer_id greater than or
    // equal to ( vps_base_layer_internal_flag ? 0 : 1 ) of the access unit:

    for( TComList<TComPic*>::iterator itP = m_curAu.begin(); itP != m_curAu.end(); itP++ )
    {
      TComPic* pic = (*itP);
      if( pic->getLayerId() >= ( vps->getVpsBaseLayerInternalFlag() ? 0 : 1 ) )
      {
        if ( pic->getPicOutputFlag() )
        {
          //If the decoded picture has PicOutputFlag equal to 1, it is marked as "needed for output".
          pic->setOutputMark( true );
        }
        else
        {
          // Otherwise it is marked as "not needed for output".
          pic->setOutputMark( false );
        }
        // NOTE - Prior to investigating the conditions above, PicOutputFlag
        // of each picture of the access unit is updated as specified in clause F.8.1.2.
      }
    }
  }

  // The current decoded picture is marked as "used for short-term reference".
  m_curPic->markAsUsedForShortTermReference();


  Bool repeat = true;
  while ( repeat )
  {
    TComList<TComAu*> aus = m_dpb.getAusHavingPicsMarkedForOutput();

    // When one or more of the following conditions are true,
    // the "bumping" process specified in clause F.13.5.2.4 is invoked
    // repeatedly until none of the following conditions are true:

    // - The number of access units that contain at least one decoded picture in the DPB marked
    //   as "needed for output" is greater than MaxNumReorderPics.
    Bool cond1 = ( aus.size() > m_maxNumReorderPics );

    // - MaxLatencyIncreasePlus1 is not equal to 0 and there is at least one access unit
    //   that contains at least one decoded picture in the DPB marked as "needed for output"
    //   for which the associated variable PicLatencyCount is greater than or equal to MaxLatencyValue.
    Bool auWithGreaterLatencyCount = false;
    for(TComList<TComAu*>::iterator itA= aus.begin(); ( itA!=aus.end()) && !auWithGreaterLatencyCount ; itA++)
    {
      if ( (*itA)->getPicLatencyCount() > m_maxLatencyValue )
      {
        auWithGreaterLatencyCount = true;
      }
    }

    Bool cond2 = (m_maxLatencyIncreasePlus1 != 0 ) && auWithGreaterLatencyCount;

    if ( cond1  || cond2 )
    {
      xF13524Bumping( aus );
    }
    else
    {
      repeat = false;
    }
  }
}

Void TAppDecTop::xF13524Bumping( TComList<TComAu*> aus )
{
  ////////////////////////////////////////////////////////////////////////////////
  // F.13.5.2.4 "Bumping" process
  ////////////////////////////////////////////////////////////////////////////////

  // The picture or pictures that are first for output are selected as the ones having the
  // smallest value of PicOrderCntVal of all pictures in the DPB marked as "needed for output".

  assert( !aus.empty() );

  // Create copy, since original AU from DBP is modified when removing pic.
  TComAu auWithSmallestPoc = *((*aus.begin())); // List is sorted, hence the AU with smallest POC is the first.

  for(TComAu::iterator itP= auWithSmallestPoc.begin(); ( itP!=auWithSmallestPoc.end() ); itP++)
  {
    TComPic* pic = (*itP);

    if (pic->getOutputMark() )
    {
      // Each of these pictures is, in ascending nuh_layer_id order, cropped,
      // using the conformance cropping window specified in the active SPS for the picture,
      // the cropped picture is output, and the picture is marked as "not needed for output".

      // pictures are sorted in the AU in ascending nuh_layer_id order.


      xCropAndOutput( pic );
      pic->setOutputMark( false );

      // Each picture storage buffer that contains a picture marked as "unused for reference"
      // and that was one of the pictures cropped and output is emptied and the fullness of
      // the associated sub-DPB is decremented by one.

      if (pic->getMarkedUnUsedForReference() )
      {
        m_dpb.removePic( pic );
      }
    }
  }
}

Void TAppDecTop::xF816decProcEndDecOfCodPicLIdGrtZero()
{
  ////////////////////////////////////////////////////////////////////////////////
  // F.8.1.6  Decoding process for ending the decoding of a coded picture with nuh_layer_id greater than 0
  ////////////////////////////////////////////////////////////////////////////////

  const TComSlice* slice = m_curPic->getSlice( 0 );
  const Int nuhLayerId   = m_curPic->getLayerId();

  assert(  nuhLayerId != 0 );

  //The marking of decoded pictures is modified as specified in the following:
  for (Int i = 0; i < m_curPic->getDecodedRps()->m_numActiveRefLayerPics0;i++ )
  {
    m_curPic->getDecodedRps()->m_refPicSetInterLayer0[i]->markAsUsedForShortTermReference();
  }

  for (Int i = 0; i < m_curPic->getDecodedRps()->m_numActiveRefLayerPics1;i++ )
  {
    m_curPic->getDecodedRps()->m_refPicSetInterLayer1[i]->markAsUsedForShortTermReference();
  }

  // PicOutputFlag is set as follows:
  if ( !m_layerInitilizedFlag[ nuhLayerId ] )
  {
    // - If LayerInitializedFlag[ nuh_layer_id ] is equal to 0, PicOutputFlag is set equal to 0.
    m_curPic->setPicOutputFlag( false );
  }
  else if (m_curPic->isRasl( ) && m_noRaslOutputFlagAssocIrap[ nuhLayerId] )
  {
    // - Otherwise, if the current picture is a RASL picture and NoRaslOutputFlag of the associated IRAP picture is equal to 1,
    //   PicOutputFlag is set equal to 0.

    m_curPic->setPicOutputFlag( false );
  }
  else
  {
    // - Otherwise, PicOutputFlag is set equal to pic_output_flag.
    m_curPic->setPicOutputFlag( slice->getPicOutputFlag() );
  }

  // The decoded picture is marked as "used for short-term reference".
  m_curPic->markAsUsedForShortTermReference();

  if ( !m_firstPicInLayerDecodedFlag[ nuhLayerId ] )
  {
    // When FirstPicInLayerDecodedFlag[ nuh_layer_id ] is equal to 0, FirstPicInLayerDecodedFlag[ nuh_layer_id ] is set equal to 1.
    m_firstPicInLayerDecodedFlag[ nuhLayerId ] = true;
  }
}

TDecTop* TAppDecTop::xGetDecoder( InputNALUnit& nalu )
{
  return m_tDecTop[ xGetDecoderIdx( nalu.m_nuhLayerId )];
}


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
    m_tDecTop[ decIdx ]->setDpb( &m_dpb );
    m_tDecTop[ decIdx ]->setTargetOlsIdx( m_targetOptLayerSetIdx );
    m_tDecTop[ decIdx ]->setFirstPicInLayerDecodedFlag( m_firstPicInLayerDecodedFlag );
    m_tDecTop[ decIdx ]->setPocDecrementedInDPBFlag   ( m_pocDecrementedInDpbFlag    );
    m_tDecTop[ decIdx ]->setLastPresentPocResetIdc    ( m_lastPresentPocResetIdc );


#if O0043_BEST_EFFORT_DECODING
    m_cTDecTop[ decIdx ]->setForceDecodeBitDepth(m_forceDecodeBitDepth);
#endif
    if (!m_outputDecodedSEIMessagesFilename.empty())
    {
      std::ostream &os=m_seiMessageFileStream.is_open() ? m_seiMessageFileStream : std::cout;
      m_tDecTop[ decIdx ]->setDecodedSEIMessageOutputStream(&os);
    }

    // append pic list of new decoder to PicLists

    // create recon file related stuff
    TChar* pchTempFilename = NULL;
    if ( !m_reconFileName.empty() )
    {
      TChar buffer[4];
      sprintf(buffer,"_%i", layerId );
      assert ( !m_reconFileName.empty() );
      xAppendToFileNameEnd( m_reconFileName.c_str() , buffer, pchTempFilename );
      assert( m_pchReconFiles.size() == m_numDecoders );
    }

    m_pchReconFiles.push_back( pchTempFilename );

    m_tVideoIOYuvReconFile[ decIdx ] = new TVideoIOYuv;
    m_reconOpen           [ decIdx ] = false;

    // set others
    m_layerIdToDecIdx     [ layerId ] = decIdx;

    m_numDecoders++;
  };
  return decIdx;

}

Int TAppDecTop::xPreDecodePoc( InputNALUnit& nalu )
{  
  // - According to F.7.4.2.4.4, the POC of the current picture is required to detect whether it is the first in a new AU
  // - F.8.1.3 needs to know if the current picture is the first of an AU
  //   actually before its POC has been decoded.

  // Thus, in the software implementation the processes can not be invoked in the same order as in the Spec.
  // For this, this function decodes the POC before invoking F.8.1.3. However, this is done without
  // altering member variables

  // Do some stuff from F.8.1.3 required for POC derivation, which is not depending on AU detection.
  TDecTop* dec          = xGetDecoder( nalu ); 
  TComSlice* slicePilot = dec->getSlicePilot(); 
  Int nuhLayerId        = nalu.m_nuhLayerId; 
  Int smallestLayerId   = dec->getSmallestLayerId();

  Int handleCraAsBlaFlag; 
  if ( nalu.isIrap() )
  {
    if ( !m_handleCraAsBlaFlagSetByExtMeans )
    {
      handleCraAsBlaFlag = false;
    }
  }

  Bool firstPicInLayerDecodedFlag = m_firstPicInLayerDecodedFlag[ nalu.m_nuhLayerId ];
  
  if ( nalu.isIrap() && nuhLayerId == smallestLayerId )
  {
    Int noClrasOutputFlag;
    if( m_firstSliceInBitstream )
    {
      noClrasOutputFlag = true; 
    }
    else if( m_eosInLayer[ 0 ] || m_eosInLayer[ nuhLayerId ] )
    {
      noClrasOutputFlag = true;
    }
    else if ( nalu.isBla() || (nalu.isCra() && handleCraAsBlaFlag ))
    {
      noClrasOutputFlag = true; 
    }
    else if ( nalu.isIdr() && slicePilot->getCrossLayerBlaFlag() )
    {
      noClrasOutputFlag = true; 
    }
    else if ( m_noClrasOutputFlagSetByExtMeans )
    {
      noClrasOutputFlag = m_noClrasOutputFlag; 
    }
    else
    {     
      noClrasOutputFlag  = false;
    }

    if( noClrasOutputFlag )
    {
      firstPicInLayerDecodedFlag = false; 
    }    
  }

  // Derive POC
  return dec->preDecodePoc(firstPicInLayerDecodedFlag, m_newPicIsFstPicOfAllLayOfPocResetPer, m_newPicIsPocResettingPic );
}

Bool TAppDecTop::xDetectNewAu( InputNALUnit& nalu )
{
  TDecTop*        dec        = xGetDecoder( nalu );
  TComSlice*      slicePilot = dec->getSlicePilot();
  const TComVPS*  vps        = slicePilot->getVPS();

  Bool firstVclNaluOfAu;

  if (m_curPic == NULL )
  {
    // No picture decoded yet, so we have a new AU.
    firstVclNaluOfAu = true; 
  }
  else
  {
    if ( !vps->getVpsExtensionFlag() )
    {
      // Decoding according to clause 8, hence one pic per AU.
      firstVclNaluOfAu = slicePilot->getFirstSliceSegementInPicFlag();
    }
    else
    {
      if ( dec->getTargetOlsIdx() == 0 )
      {
        // Only the base layer is decoded, hence one pic per AU.
        firstVclNaluOfAu = slicePilot->getFirstSliceSegementInPicFlag();
      }
      else
      {
        // F.7.4.2.4.4  Order of NAL units and coded pictures and association to access units    
        //  An access unit consists of one or more coded pictures, each with a distinct value of 
        //  nuh_layer_id, and zero or more non-VCL NAL units. 

        //  A VCL NAL unit is the first VCL NAL unit of an access unit, when all of the following conditions are true:
        //  -  first_slice_segment_in_pic_flag is equal to 1.
        //  -  At least one of the following conditions is true:
        //     - The previous picture in decoding order belongs to a different POC resetting period 
        //       than the picture containing the VCL NAL unit.
        Bool prevPicDiffPocResetPeriod = m_newPicIsFstPicOfAllLayOfPocResetPer; 

        //     - PicOrderCntVal derived for the VCL NAL unit differs from the PicOrderCntVal of the 
        //       previous picture in decoding order.
        Bool prevPicDiffPoc = ( xPreDecodePoc( nalu ) != m_curPic->getPOC() );

        if( slicePilot->getFirstSliceSegementInPicFlag() && ( prevPicDiffPocResetPeriod || prevPicDiffPoc ) )
        {
          firstVclNaluOfAu = true;
        }
        else
        {
          firstVclNaluOfAu = false;
        }
      }
    }
  }
  return firstVclNaluOfAu;
}

Void TAppDecTop::xDetectNewPocResettingPeriod( InputNALUnit& nalu )
{
  TDecTop* dec  = xGetDecoder( nalu );
  dec->inferPocResetPeriodId();


  Int pocResetIdc         = dec->getSlicePilot()->getPocResetIdc();
  Int newPocResetPeriodId = dec->getSlicePilot()->getPocResetPeriodId();

  Int curPocResetPeriodId = ( m_curPic != NULL)  ? m_curPic->getPocResetPeriodId() : MIN_INT; 

  // Check if new picture starts a new poc resetting period.
  if(  ( pocResetIdc == 1 || pocResetIdc == 2 ) &&  ( curPocResetPeriodId != newPocResetPeriodId ) )
  {
    for (Int i = 0; i < MAX_NUM_LAYER_IDS; i++ )
    {
      m_firstPicInPocResettingPeriodReceived[ i ] = false;
    }
    m_newPicIsFstPicOfAllLayOfPocResetPer = true;
  }
  else
  {
    m_newPicIsFstPicOfAllLayOfPocResetPer = false;
  }

  // Check if current picture is a poc resetting picture (thus the first picture of this layer within the POC resetting period.
  if ( !m_firstPicInPocResettingPeriodReceived[ nalu.m_nuhLayerId ] )
  {
    m_newPicIsPocResettingPic = true;
    m_firstPicInPocResettingPeriodReceived[ nalu.m_nuhLayerId ] = true;
  }
  else
  {
    m_newPicIsPocResettingPic = false;
  }

}

Bool TAppDecTop::xAllRefLayersInitilized( Int curLayerId )
{
  Bool allRefLayersInitilizedFlag = true;
  for (Int i = 0; i < m_vps->getNumDirectRefLayers( curLayerId  ); i++ )
  {
    Int refLayerId = m_vps->getIdDirectRefLayer( curLayerId, i );
    allRefLayersInitilizedFlag = allRefLayersInitilizedFlag && m_layerInitilizedFlag[ refLayerId ];
  }

  return allRefLayersInitilizedFlag;
}

Void TAppDecTop::xInitFileIO()
{
  m_bitstreamFile.open(m_bitstreamFileName.c_str(), ifstream::in | ifstream::binary);

  if ( !m_bitstreamFile)
  {
    fprintf(stderr, "\nUnable to open bitstream file `%s' for reading\n", m_bitstreamFileName.c_str());
    exit(EXIT_FAILURE);
  }

  if (!m_outputDecodedSEIMessagesFilename.empty() && m_outputDecodedSEIMessagesFilename!="-")
  {
    m_seiMessageFileStream.open(m_outputDecodedSEIMessagesFilename.c_str(), std::ios::out);
    if (!m_seiMessageFileStream.is_open() || !m_seiMessageFileStream.good())
    {
      fprintf(stderr, "\nUnable to open file `%s' for writing decoded SEI messages\n", m_outputDecodedSEIMessagesFilename.c_str());
      exit(EXIT_FAILURE);
    }
  }

}

Void TAppDecTop::xOpenReconFile( TComPic* curPic )
{
  Int decIdx = xGetDecoderIdx( curPic->getLayerId() );

  if ( !m_reconFileName.empty() && !m_reconOpen[decIdx] )
  {
    const BitDepths &bitDepths= curPic->getPicSym()->getSPS().getBitDepths(); // use bit depths of first reconstructed picture.
    for (UInt channelType = 0; channelType < MAX_NUM_CHANNEL_TYPE; channelType++)
    {
      if (m_outputBitDepth[channelType] == 0)
      {
        m_outputBitDepth[channelType] = bitDepths.recon[channelType];
      }
    }

    m_tVideoIOYuvReconFile[decIdx]->open( m_pchReconFiles[decIdx], true, m_outputBitDepth, m_outputBitDepth, bitDepths.recon ); // write mode
    m_reconOpen[decIdx] = true;
  }
}

Void TAppDecTop::xFlushOutput()
{
  Bool repeat = true;
  while ( repeat )
  {
    if( m_decProcCvsg == ANNEX_F )
    {
      TComList<TComAu*> aus = m_dpb.getAusHavingPicsMarkedForOutput();
      if ( !aus.empty() )
      {
        xF13524Bumping( aus );
      }      
      else
      {
        repeat = false;
      }
    }      
    else if( m_decProcCvsg == CLAUSE_8 )
    {
      TComList<TComPic*> picsMarkedForOutput = m_dpb.getSubDpb( 0, false )->getPicsMarkedNeedForOutput();
      if (!picsMarkedForOutput.empty())
      {
        xC524Bumping();
      }
      else
      {
        repeat = false; 
      }
    }
  }
}

Void TAppDecTop::xCropAndOutput( TComPic* curPic )
{

  if ( m_printPicOutput )
  {
    std::cout << "  Output picture: ";
    curPic->print( 2 );
    std::cout << std::endl;
  }

  assert( !curPic->getHasGeneratedRefPics() );
  assert( !curPic->getIsGenerated()         );

  Int decIdx = xGetDecoderIdx( curPic->getLayerId() );

  if (!m_reconOpen[ decIdx ])
  {
    xOpenReconFile( curPic );
  }

  if ( m_pchReconFiles[ decIdx ] )
  {
    const Window &conf    = curPic->getConformanceWindow();
    const Window  defDisp = m_respectDefDispWindow ? curPic->getDefDisplayWindow() : Window();

    assert( conf   .getScaledFlag() );
    assert( defDisp.getScaledFlag() );

    m_tVideoIOYuvReconFile[decIdx]->write( curPic->getPicYuvRec(),
      m_outputColourSpaceConvert,
      conf.getWindowLeftOffset()   + defDisp.getWindowLeftOffset(),
      conf.getWindowRightOffset()  + defDisp.getWindowRightOffset(),
      conf.getWindowTopOffset()    + defDisp.getWindowTopOffset(),
      conf.getWindowBottomOffset() + defDisp.getWindowBottomOffset(),
      NUM_CHROMA_FORMAT
      , m_bClipOutputVideoToRec709Range);
  }
}

UInt TAppDecTop::getNumberOfChecksumErrorsDetected() const
{
  UInt numOfChecksumErrors = 0;
  for (Int i = 0; i < m_numDecoders; i++ )
  {
    numOfChecksumErrors += getNumberOfChecksumErrorsDetected( i );
  }
  return numOfChecksumErrors;
}

#endif

Void TAppDecTop::xOutputColourRemapPic(TComPic* pcPic)
{
  const TComSPS &sps=pcPic->getPicSym()->getSPS();
  SEIMessages colourRemappingInfo = getSeisByType(pcPic->getSEIs(), SEI::COLOUR_REMAPPING_INFO );
  SEIColourRemappingInfo *seiColourRemappingInfo = ( colourRemappingInfo.size() > 0 ) ? (SEIColourRemappingInfo*) *(colourRemappingInfo.begin()) : NULL;

  if (colourRemappingInfo.size() > 1)
  {
    printf ("Warning: Got multiple Colour Remapping Information SEI messages. Using first.");
  }
  if (seiColourRemappingInfo)
  {
    applyColourRemapping(*pcPic->getPicYuvRec(), *seiColourRemappingInfo, sps);

    // save the last CRI SEI received
    if (m_pcSeiColourRemappingInfoPrevious == NULL)
    {
      m_pcSeiColourRemappingInfoPrevious = new SEIColourRemappingInfo();
    }
    m_pcSeiColourRemappingInfoPrevious->copyFrom(*seiColourRemappingInfo);
  }
  else  // using the last CRI SEI received
  {
    // TODO: prevent persistence of CRI SEI across C(L)VS.
    if (m_pcSeiColourRemappingInfoPrevious != NULL)
    {
      if (m_pcSeiColourRemappingInfoPrevious->m_colourRemapPersistenceFlag == false)
      {
        printf("Warning No SEI-CRI message is present for the current picture, persistence of the CRI is not managed\n");
      }
      applyColourRemapping(*pcPic->getPicYuvRec(), *m_pcSeiColourRemappingInfoPrevious, sps);
    }
  }
}

// compute lut from SEI
// use at lutPoints points aligned on a power of 2 value
// SEI Lut must be in ascending values of coded Values
static std::vector<Int>
initColourRemappingInfoLut(const Int                                          bitDepth_in,     // bit-depth of the input values of the LUT
                           const Int                                          nbDecimalValues, // Position of the fixed point
                           const std::vector<SEIColourRemappingInfo::CRIlut> &lut,
                           const Int                                          maxValue, // maximum output value
                           const Int                                          lutOffset)
{
  const Int lutPoints = (1 << bitDepth_in) + 1 ;
  std::vector<Int> retLut(lutPoints);

  // missing values: need to define default values before first definition (check codedValue[0] == 0)
  Int iTargetPrev = (lut.size() && lut[0].codedValue == 0) ? lut[0].targetValue: 0;
  Int startPivot = (lut.size())? ((lut[0].codedValue == 0)? 1: 0): 1;
  Int iCodedPrev  = 0;
  // set max value with the coded bit-depth
  // + ((1 << nbDecimalValues) - 1) is for the added bits
  const Int maxValueFixedPoint = (maxValue << nbDecimalValues) + ((1 << nbDecimalValues) - 1);

  Int iValue = 0;

  for ( Int iPivot=startPivot ; iPivot < (Int)lut.size(); iPivot++ )
  {
    Int iCodedNext  = lut[iPivot].codedValue;
    Int iTargetNext = lut[iPivot].targetValue;

    // ensure correct bit depth and avoid overflow in lut address
    Int iCodedNext_bitDepth = std::min(iCodedNext, (1 << bitDepth_in));

    const Int divValue =  (iCodedNext - iCodedPrev > 0)? (iCodedNext - iCodedPrev): 1;
    const Int lutValInit = (lutOffset + iTargetPrev) << nbDecimalValues;
    const Int roundValue = divValue / 2;
    for ( ; iValue<iCodedNext_bitDepth; iValue++ )
    {
      Int value = iValue;
      Int interpol = ((((value-iCodedPrev) * (iTargetNext - iTargetPrev)) << nbDecimalValues) + roundValue) / divValue;               
      retLut[iValue]  = std::min(lutValInit + interpol , maxValueFixedPoint);
    }
    iCodedPrev  = iCodedNext;
    iTargetPrev = iTargetNext;
  }
  // fill missing values if necessary
  if(iCodedPrev < (1 << bitDepth_in)+1)
  {
    Int iCodedNext  = (1 << bitDepth_in);
    Int iTargetNext = (1 << bitDepth_in) - 1;

    const Int divValue =  (iCodedNext - iCodedPrev > 0)? (iCodedNext - iCodedPrev): 1;
    const Int lutValInit = (lutOffset + iTargetPrev) << nbDecimalValues;
    const Int roundValue = divValue / 2;

    for ( ; iValue<=iCodedNext; iValue++ )
    {
      Int value = iValue;
      Int interpol = ((((value-iCodedPrev) * (iTargetNext - iTargetPrev)) << nbDecimalValues) + roundValue) / divValue; 
      retLut[iValue]  = std::min(lutValInit + interpol , maxValueFixedPoint);
    }
  }
  return retLut;
}

static Void
initColourRemappingInfoLuts(std::vector<Int>      (&preLut)[3],
                            std::vector<Int>      (&postLut)[3],
                            SEIColourRemappingInfo &pCriSEI,
                            const Int               maxBitDepth)
{
  Int internalBitDepth = pCriSEI.m_colourRemapBitDepth;
  for ( Int c=0 ; c<3 ; c++ )
  {
    std::sort(pCriSEI.m_preLut[c].begin(), pCriSEI.m_preLut[c].end()); // ensure preLut is ordered in ascending values of codedValues   
    preLut[c] = initColourRemappingInfoLut(pCriSEI.m_colourRemapInputBitDepth, maxBitDepth - pCriSEI.m_colourRemapInputBitDepth, pCriSEI.m_preLut[c], ((1 << internalBitDepth) - 1), 0); //Fill preLut

    std::sort(pCriSEI.m_postLut[c].begin(), pCriSEI.m_postLut[c].end()); // ensure postLut is ordered in ascending values of codedValues       
    postLut[c] = initColourRemappingInfoLut(pCriSEI.m_colourRemapBitDepth, maxBitDepth - pCriSEI.m_colourRemapBitDepth, pCriSEI.m_postLut[c], (1 << internalBitDepth) - 1, 0); //Fill postLut
  }
}

// apply lut.
// Input lut values are aligned on power of 2 boundaries
static Int
applyColourRemappingInfoLut1D(Int inVal, const std::vector<Int> &lut, const Int inValPrecisionBits)
{
  const Int roundValue = (inValPrecisionBits)? 1 << (inValPrecisionBits - 1): 0;
  inVal = std::min(std::max(0, inVal), (Int)(((lut.size()-1) << inValPrecisionBits)));
  Int index  = (Int) std::min((inVal >> inValPrecisionBits), (Int)(lut.size()-2));
  Int outVal = (( inVal - (index<<inValPrecisionBits) ) * (lut[index+1] - lut[index]) + roundValue) >> inValPrecisionBits;
  outVal +=  lut[index] ;

  return outVal;
}  

static Int
applyColourRemappingInfoMatrix(const Int (&colourRemapCoeffs)[3], const Int postOffsetShift, const Int p0, const Int p1, const Int p2, const Int offset)
{
  Int YUVMat = (colourRemapCoeffs[0]* p0 + colourRemapCoeffs[1]* p1 + colourRemapCoeffs[2]* p2  + offset) >> postOffsetShift;
  return YUVMat;
}

static Void
setColourRemappingInfoMatrixOffset(Int (&matrixOffset)[3], Int offset0, Int offset1, Int offset2)
{
  matrixOffset[0] = offset0;
  matrixOffset[1] = offset1;
  matrixOffset[2] = offset2;
}

static Void
setColourRemappingInfoMatrixOffsets(      Int  (&matrixInputOffset)[3],
                                          Int  (&matrixOutputOffset)[3],
                                    const Int  bitDepth,
                                    const Bool crInputFullRangeFlag,
                                    const Int  crInputMatrixCoefficients,
                                    const Bool crFullRangeFlag,
                                    const Int  crMatrixCoefficients)
{
  // set static matrix offsets
  Int crInputOffsetLuma = (crInputFullRangeFlag)? 0:-(16 << (bitDepth-8));
  Int crOffsetLuma = (crFullRangeFlag)? 0:(16 << (bitDepth-8));
  Int crInputOffsetChroma = 0;
  Int crOffsetChroma = 0;

  switch(crInputMatrixCoefficients)
  {
    case MATRIX_COEFFICIENTS_RGB:
      crInputOffsetChroma = 0;
      if(!crInputFullRangeFlag)
      {
        fprintf(stderr, "WARNING: crInputMatrixCoefficients set to MATRIX_COEFFICIENTS_RGB and crInputFullRangeFlag not set\n");
        crInputOffsetLuma = 0;
      }
      break;
    case MATRIX_COEFFICIENTS_UNSPECIFIED:
    case MATRIX_COEFFICIENTS_BT709:
    case MATRIX_COEFFICIENTS_BT2020_NON_CONSTANT_LUMINANCE:
      crInputOffsetChroma = -(1 << (bitDepth-1));
      break;
    default:
      fprintf(stderr, "WARNING: crInputMatrixCoefficients set to undefined value: %d\n", crInputMatrixCoefficients);
  }

  switch(crMatrixCoefficients)
  {
    case MATRIX_COEFFICIENTS_RGB:
      crOffsetChroma = 0;
      if(!crFullRangeFlag)
      {
        fprintf(stderr, "WARNING: crMatrixCoefficients set to MATRIX_COEFFICIENTS_RGB and crInputFullRangeFlag not set\n");
        crOffsetLuma = 0;
      }
      break;
    case MATRIX_COEFFICIENTS_UNSPECIFIED:
    case MATRIX_COEFFICIENTS_BT709:
    case MATRIX_COEFFICIENTS_BT2020_NON_CONSTANT_LUMINANCE:
      crOffsetChroma = (1 << (bitDepth-1));
      break;
    default:
      fprintf(stderr, "WARNING: crMatrixCoefficients set to undefined value: %d\n", crMatrixCoefficients);
  }

  setColourRemappingInfoMatrixOffset(matrixInputOffset, crInputOffsetLuma, crInputOffsetChroma, crInputOffsetChroma);
  setColourRemappingInfoMatrixOffset(matrixOutputOffset, crOffsetLuma, crOffsetChroma, crOffsetChroma);
}

Void TAppDecTop::applyColourRemapping(const TComPicYuv& pic, SEIColourRemappingInfo& criSEI, const TComSPS &activeSPS)
{  
  const Int maxBitDepth = 16;

  // create colour remapped picture
  if( !criSEI.m_colourRemapCancelFlag && pic.getChromaFormat()!=CHROMA_400) // 4:0:0 not supported.
  {
    const Int          iHeight         = pic.getHeight(COMPONENT_Y);
    const Int          iWidth          = pic.getWidth(COMPONENT_Y);
    const ChromaFormat chromaFormatIDC = pic.getChromaFormat();

    TComPicYuv picYuvColourRemapped;
    picYuvColourRemapped.createWithoutCUInfo( iWidth, iHeight, chromaFormatIDC );

    const Int  iStrideIn   = pic.getStride(COMPONENT_Y);
    const Int  iCStrideIn  = pic.getStride(COMPONENT_Cb);
    const Int  iStrideOut  = picYuvColourRemapped.getStride(COMPONENT_Y);
    const Int  iCStrideOut = picYuvColourRemapped.getStride(COMPONENT_Cb);
    const Bool b444        = ( pic.getChromaFormat() == CHROMA_444 );
    const Bool b422        = ( pic.getChromaFormat() == CHROMA_422 );
    const Bool b420        = ( pic.getChromaFormat() == CHROMA_420 );

    std::vector<Int> preLut[3];
    std::vector<Int> postLut[3];
    Int matrixInputOffset[3];
    Int matrixOutputOffset[3];
    const Pel *YUVIn[MAX_NUM_COMPONENT];
    Pel *YUVOut[MAX_NUM_COMPONENT];
    YUVIn[COMPONENT_Y]  = pic.getAddr(COMPONENT_Y);
    YUVIn[COMPONENT_Cb] = pic.getAddr(COMPONENT_Cb);
    YUVIn[COMPONENT_Cr] = pic.getAddr(COMPONENT_Cr);
    YUVOut[COMPONENT_Y]  = picYuvColourRemapped.getAddr(COMPONENT_Y);
    YUVOut[COMPONENT_Cb] = picYuvColourRemapped.getAddr(COMPONENT_Cb);
    YUVOut[COMPONENT_Cr] = picYuvColourRemapped.getAddr(COMPONENT_Cr);

    const Int bitDepth = criSEI.m_colourRemapBitDepth;
    BitDepths        bitDepthsCriFile;
    bitDepthsCriFile.recon[CHANNEL_TYPE_LUMA]   = bitDepth;
    bitDepthsCriFile.recon[CHANNEL_TYPE_CHROMA] = bitDepth; // Different bitdepth is not implemented

    const Int postOffsetShift = criSEI.m_log2MatrixDenom;
    const Int matrixRound = 1 << (postOffsetShift - 1);
    const Int postLutInputPrecision = (maxBitDepth - criSEI.m_colourRemapBitDepth);

    if ( ! criSEI.m_colourRemapVideoSignalInfoPresentFlag ) // setting default
    {
      setColourRemappingInfoMatrixOffsets(matrixInputOffset, matrixOutputOffset, maxBitDepth,
          activeSPS.getVuiParameters()->getVideoFullRangeFlag(), activeSPS.getVuiParameters()->getMatrixCoefficients(),
          activeSPS.getVuiParameters()->getVideoFullRangeFlag(), activeSPS.getVuiParameters()->getMatrixCoefficients());
    }
    else
    {
      setColourRemappingInfoMatrixOffsets(matrixInputOffset, matrixOutputOffset, maxBitDepth,
          activeSPS.getVuiParameters()->getVideoFullRangeFlag(), activeSPS.getVuiParameters()->getMatrixCoefficients(),
          criSEI.m_colourRemapFullRangeFlag, criSEI.m_colourRemapMatrixCoefficients);
    }

    // add matrix rounding to output matrix offsets
    matrixOutputOffset[0] = (matrixOutputOffset[0] << postOffsetShift) + matrixRound;
    matrixOutputOffset[1] = (matrixOutputOffset[1] << postOffsetShift) + matrixRound;
    matrixOutputOffset[2] = (matrixOutputOffset[2] << postOffsetShift) + matrixRound;

    // Merge   matrixInputOffset and matrixOutputOffset to matrixOutputOffset
    matrixOutputOffset[0] += applyColourRemappingInfoMatrix(criSEI.m_colourRemapCoeffs[0], 0, matrixInputOffset[0], matrixInputOffset[1], matrixInputOffset[2], 0);
    matrixOutputOffset[1] += applyColourRemappingInfoMatrix(criSEI.m_colourRemapCoeffs[1], 0, matrixInputOffset[0], matrixInputOffset[1], matrixInputOffset[2], 0);
    matrixOutputOffset[2] += applyColourRemappingInfoMatrix(criSEI.m_colourRemapCoeffs[2], 0, matrixInputOffset[0], matrixInputOffset[1], matrixInputOffset[2], 0);

    // rescaling output: include CRI/output frame difference
    const Int scaleShiftOut_neg = abs(bitDepth - maxBitDepth);
    const Int scaleOut_round = 1 << (scaleShiftOut_neg-1);

    initColourRemappingInfoLuts(preLut, postLut, criSEI, maxBitDepth);

    assert(pic.getChromaFormat() != CHROMA_400);
    const Int hs = pic.getComponentScaleX(ComponentID(COMPONENT_Cb));
    const Int maxOutputValue = (1 << bitDepth) - 1;

    for( Int y = 0; y < iHeight; y++ )
    {
      for( Int x = 0; x < iWidth; x++ )
      {
        const Int xc = (x>>hs);
        Bool computeChroma = b444 || ((b422 || !(y&1)) && !(x&1));

        Int YUVPre_0 = applyColourRemappingInfoLut1D(YUVIn[COMPONENT_Y][x], preLut[0], 0);
        Int YUVPre_1 = applyColourRemappingInfoLut1D(YUVIn[COMPONENT_Cb][xc], preLut[1], 0);
        Int YUVPre_2 = applyColourRemappingInfoLut1D(YUVIn[COMPONENT_Cr][xc], preLut[2], 0);

        Int YUVMat_0 = applyColourRemappingInfoMatrix(criSEI.m_colourRemapCoeffs[0], postOffsetShift, YUVPre_0, YUVPre_1, YUVPre_2, matrixOutputOffset[0]);
        Int YUVLutB_0 = applyColourRemappingInfoLut1D(YUVMat_0, postLut[0], postLutInputPrecision);
        YUVOut[COMPONENT_Y][x] = std::min(maxOutputValue, (YUVLutB_0 + scaleOut_round) >> scaleShiftOut_neg);

        if( computeChroma )
        {
          Int YUVMat_1 = applyColourRemappingInfoMatrix(criSEI.m_colourRemapCoeffs[1], postOffsetShift, YUVPre_0, YUVPre_1, YUVPre_2, matrixOutputOffset[1]);
          Int YUVLutB_1 = applyColourRemappingInfoLut1D(YUVMat_1, postLut[1], postLutInputPrecision);
          YUVOut[COMPONENT_Cb][xc] = std::min(maxOutputValue, (YUVLutB_1 + scaleOut_round) >> scaleShiftOut_neg);

          Int YUVMat_2 = applyColourRemappingInfoMatrix(criSEI.m_colourRemapCoeffs[2], postOffsetShift, YUVPre_0, YUVPre_1, YUVPre_2, matrixOutputOffset[2]);
          Int YUVLutB_2 = applyColourRemappingInfoLut1D(YUVMat_2, postLut[2], postLutInputPrecision);
          YUVOut[COMPONENT_Cr][xc] = std::min(maxOutputValue, (YUVLutB_2 + scaleOut_round) >> scaleShiftOut_neg);
        }
      }

      YUVIn[COMPONENT_Y]  += iStrideIn;
      YUVOut[COMPONENT_Y] += iStrideOut;
      if( !(b420 && !(y&1)) )
      {
         YUVIn[COMPONENT_Cb]  += iCStrideIn;
         YUVIn[COMPONENT_Cr]  += iCStrideIn;
         YUVOut[COMPONENT_Cb] += iCStrideOut;
         YUVOut[COMPONENT_Cr] += iCStrideOut;
      }
    }
    //Write remapped picture in display order
    picYuvColourRemapped.dump( m_colourRemapSEIFileName, bitDepthsCriFile, true );
    picYuvColourRemapped.destroy();
  }
}
//! \}
