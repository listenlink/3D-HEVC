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

/** \file     TDecTop.cpp
    \brief    decoder class
*/
#include "NALread.h"
#include "TDecTop.h"
#if NH_MV
ParameterSetManager TDecTop::m_parameterSetManager;
#endif

//! \ingroup TLibDecoder
//! \{



TDecTop::TDecTop()
  : m_iMaxRefPicNum(0)
#if !NH_MV
  , m_associatedIRAPType(NAL_UNIT_INVALID)
  , m_pocCRA(0)
  , m_pocRandomAccess(MAX_INT)
  , m_cListPic()
  , m_parameterSetManager()
#endif
  , m_apcSlicePilot(NULL)
  , m_SEIs()
  , m_cPrediction()
  , m_cTrQuant()
  , m_cGopDecoder()
  , m_cSliceDecoder()
  , m_cCuDecoder()
  , m_cEntropyDecoder()
  , m_cCavlcDecoder()
  , m_cSbacDecoder()
  , m_cBinCABAC()
  , m_seiReader()
  , m_cLoopFilter()
  , m_cSAO()
  , m_pcPic(NULL)
#if !NH_MV
  , m_prevPOC(MAX_INT)
  , m_prevTid0POC(0)
  , m_bFirstSliceInPicture(true)
  , m_bFirstSliceInSequence(true)
  , m_prevSliceSkipped(false)
  , m_skippedPOC(0)
  , m_bFirstSliceInBitstream(true)
  , m_lastPOCNoOutputPriorPics(-1)
  , m_isNoOutputPriorPics(false)
  , m_craNoRaslOutputFlag(false)
#endif
#if O0043_BEST_EFFORT_DECODING
  , m_forceDecodeBitDepth(8)
#endif
  , m_pDecodedSEIOutputStream(NULL)
#if !NH_MV
  , m_warningMessageSkipPicture(false)
#endif
  , m_prefixSEINALUs()
{
#if !NH_MV
#if ENC_DEC_TRACE
  if (g_hTrace == NULL)
  {
    g_hTrace = fopen( "TraceDec.txt", "wb" );
  }
  g_bJustDoIt = g_bEncDecTraceDisable;
  g_nSymbolCounter = 0;
#endif
#endif

#if NH_MV
  m_layerId                       = 0;
  m_viewId                        = 0;


  m_decodingProcess               = CLAUSE_8;
  m_targetOlsIdx                  = -1;
  m_smallestLayerId               = -1;
  m_isInOwnTargetDecLayerIdList   = 0;
  m_prevPicOrderCnt               = 0;
  m_pocDecrementedInDpbFlag       = NULL;
  m_firstPicInLayerDecodedFlag    = NULL;
  m_lastPresentPocResetIdc        = NULL;

  m_prevIrapPoc                   = MIN_INT;
  m_prevIrapDecodingOrder         = MIN_INT;
  m_prevStsaDecOrder              = MIN_INT;
  m_prevStsaTemporalId            = MIN_INT;
#endif
}

TDecTop::~TDecTop()
{
#if ENC_DEC_TRACE
#if NH_MV
  if (g_hTrace != stdout && g_hTrace != NULL)
  {
    fclose( g_hTrace );
    g_hTrace = NULL;
  }
#else
  if (g_hTrace != stdout)
  {
    fclose( g_hTrace );
  }
#endif
#endif
  while (!m_prefixSEINALUs.empty())
  {
    delete m_prefixSEINALUs.front();
    m_prefixSEINALUs.pop_front();
  }
}

Void TDecTop::create()
{
  m_cGopDecoder.create();
  m_apcSlicePilot = new TComSlice;
  m_uiSliceIdx = 0;
}

Void TDecTop::destroy()
{

#if NH_MV
  m_cSAO.destroy();
  m_cLoopFilter.        destroy();
#endif

  m_cGopDecoder.destroy();

  delete m_apcSlicePilot;
  m_apcSlicePilot = NULL;

  m_cSliceDecoder.destroy();
}

Void TDecTop::init()
{
  // initialize ROM
#if !NH_MV
  initROM();
#endif
#if NH_MV
  m_cCavlcDecoder.setDecTop( this );
#endif
  m_cGopDecoder.init( &m_cEntropyDecoder, &m_cSbacDecoder, &m_cBinCABAC, &m_cCavlcDecoder, &m_cSliceDecoder, &m_cLoopFilter, &m_cSAO);
  m_cSliceDecoder.init( &m_cEntropyDecoder, &m_cCuDecoder );
  m_cEntropyDecoder.init(&m_cPrediction);
}

#if !NH_MV
Void TDecTop::deletePicBuffer ( )
{

  TComList<TComPic*>::iterator  iterPic   = m_cListPic.begin();
  Int iSize = Int( m_cListPic.size() );

  for (Int i = 0; i < iSize; i++ )
  {
    TComPic* pcPic = *(iterPic++);
    pcPic->destroy();

    delete pcPic;
    pcPic = NULL;
  }

  m_cSAO.destroy();

  m_cLoopFilter.        destroy();

  // destroy ROM
  destroyROM();
}

Void TDecTop::xGetNewPicBuffer ( const TComSPS &sps, const TComPPS &pps, TComPic*& rpcPic, const UInt temporalLayer )
{
  m_iMaxRefPicNum = sps.getMaxDecPicBuffering(temporalLayer);     // m_uiMaxDecPicBuffering has the space for the picture currently being decoded
  if (m_cListPic.size() < (UInt)m_iMaxRefPicNum)
  {
    rpcPic = new TComPic();

    rpcPic->create ( sps, pps, true);

    m_cListPic.pushBack( rpcPic );

    return;
  }

  Bool bBufferIsAvailable = false;
  TComList<TComPic*>::iterator  iterPic   = m_cListPic.begin();
  while (iterPic != m_cListPic.end())
  {
    rpcPic = *(iterPic++);
    if ( rpcPic->getReconMark() == false && rpcPic->getOutputMark() == false)
    {
      rpcPic->setOutputMark(false);
      bBufferIsAvailable = true;
      break;
    }

    if ( rpcPic->getSlice( 0 )->isReferenced() == false  && rpcPic->getOutputMark() == false)
    {
      rpcPic->setOutputMark(false);
      rpcPic->setReconMark( false );
      rpcPic->getPicYuvRec()->setBorderExtension( false );
      bBufferIsAvailable = true;
      break;
    }
  }

  if ( !bBufferIsAvailable )
  {
    //There is no room for this picture, either because of faulty encoder or dropped NAL. Extend the buffer.
    m_iMaxRefPicNum++;
    rpcPic = new TComPic();
    m_cListPic.pushBack( rpcPic );
  }
  rpcPic->destroy();
  rpcPic->create ( sps, pps, true);
}

Void TDecTop::executeLoopFilters(Int& poc, TComList<TComPic*>*& rpcListPic)
{
  if (!m_pcPic)
  {
    /* nothing to deblock */
    return;
  }

  TComPic*   pcPic         = m_pcPic;

  // Execute Deblock + Cleanup

  m_cGopDecoder.filterPicture(pcPic);

  TComSlice::sortPicList( m_cListPic ); // sorting for application output
  poc                 = pcPic->getSlice(m_uiSliceIdx-1)->getPOC();
  rpcListPic          = &m_cListPic;
  m_cCuDecoder.destroy();
  m_bFirstSliceInPicture  = true;
  return;
}

Void TDecTop::checkNoOutputPriorPics (TComList<TComPic*>* pcListPic)
{
  if (!pcListPic || !m_isNoOutputPriorPics)
  {
    return;
  }

  TComList<TComPic*>::iterator  iterPic   = pcListPic->begin();

  while (iterPic != pcListPic->end())
  {
    TComPic* pcPicTmp = *(iterPic++);
    if (m_lastPOCNoOutputPriorPics != pcPicTmp->getPOC())
    {
      pcPicTmp->setOutputMark(false);
    }
  }
}

Void TDecTop::xCreateLostPicture(Int iLostPoc)
{
  printf("\ninserting lost poc : %d\n",iLostPoc);
  TComPic *cFillPic;
  xGetNewPicBuffer(*(m_parameterSetManager.getFirstSPS()), *(m_parameterSetManager.getFirstPPS()), cFillPic, 0);
  cFillPic->getSlice(0)->initSlice();

  TComList<TComPic*>::iterator iterPic = m_cListPic.begin();
  Int closestPoc = 1000000;
  while ( iterPic != m_cListPic.end())
  {
    TComPic * rpcPic = *(iterPic++);
    if(abs(rpcPic->getPicSym()->getSlice(0)->getPOC() -iLostPoc)<closestPoc&&abs(rpcPic->getPicSym()->getSlice(0)->getPOC() -iLostPoc)!=0&&rpcPic->getPicSym()->getSlice(0)->getPOC()!=m_apcSlicePilot->getPOC())
    {
      closestPoc=abs(rpcPic->getPicSym()->getSlice(0)->getPOC() -iLostPoc);
    }
  }
  iterPic = m_cListPic.begin();
  while ( iterPic != m_cListPic.end())
  {
    TComPic *rpcPic = *(iterPic++);
    if(abs(rpcPic->getPicSym()->getSlice(0)->getPOC() -iLostPoc)==closestPoc&&rpcPic->getPicSym()->getSlice(0)->getPOC()!=m_apcSlicePilot->getPOC())
    {
      printf("copying picture %d to %d (%d)\n",rpcPic->getPicSym()->getSlice(0)->getPOC() ,iLostPoc,m_apcSlicePilot->getPOC());
      rpcPic->getPicYuvRec()->copyToPic(cFillPic->getPicYuvRec());
      break;
    }
  }
  cFillPic->setCurrSliceIdx(0);
  for(Int ctuRsAddr=0; ctuRsAddr<cFillPic->getNumberOfCtusInFrame(); ctuRsAddr++)
  {
    cFillPic->getCtu(ctuRsAddr)->initCtu(cFillPic, ctuRsAddr);
  }
  cFillPic->getSlice(0)->setReferenced(true);
  cFillPic->getSlice(0)->setPOC(iLostPoc);
  xUpdatePreviousTid0POC(cFillPic->getSlice(0));
  cFillPic->setReconMark(true);
  cFillPic->setOutputMark(true);
  if(m_pocRandomAccess == MAX_INT)
  {
    m_pocRandomAccess = iLostPoc;
  }
}
#endif


#if NH_MV
Void TDecTop::activatePSsAndInitPicOrSlice( TComPic* newPic )
{
  if ( m_apcSlicePilot->getFirstSliceSegementInPicFlag() )
  {
    assert( newPic != NULL );

#else
Void TDecTop::xActivateParameterSets()
{
  if (m_bFirstSliceInPicture)
  {
#endif
    const TComPPS *pps = m_parameterSetManager.getPPS(m_apcSlicePilot->getPPSId()); // this is a temporary PPS object. Do not store this value
    assert (pps != 0);

    const TComSPS *sps = m_parameterSetManager.getSPS(pps->getSPSId());             // this is a temporary SPS object. Do not store this value
    assert (sps != 0);

    m_parameterSetManager.clearSPSChangedFlag(sps->getSPSId());
    m_parameterSetManager.clearPPSChangedFlag(pps->getPPSId());
#if NH_MV
    const TComVPS* vps = m_parameterSetManager.getVPS(sps->getVPSId());
    assert (vps != 0);
    // TBD: check the condition on m_firstPicInLayerDecodedFlag
    if (!m_parameterSetManager.activatePPS(m_apcSlicePilot->getPPSId(),m_apcSlicePilot->isIRAP() || !m_firstPicInLayerDecodedFlag[m_layerId] , m_layerId ) )
#else
    if (false == m_parameterSetManager.activatePPS(m_apcSlicePilot->getPPSId(),m_apcSlicePilot->isIRAP()))
#endif
    {
      printf ("Parameter set activation failed!");
      assert (0);
    }

#if NH_MV
    if ( decProcAnnexG() )
    {
      // When the value of vps_num_rep_formats_minus1 in the active VPS is equal to 0
      if ( vps->getVpsNumRepFormatsMinus1() == 0 )
      {
        //, it is a requirement of bitstream conformance that the value of update_rep_format_flag shall be equal to 0.
        assert( sps->getUpdateRepFormatFlag() == false );
      }
      sps->checkRpsMaxNumPics( vps, getLayerId() );

      // It is a requirement of bitstream conformance that, when the SPS is referred to by
      // any current picture that belongs to an independent non-base layer, the value of
      // MultiLayerExtSpsFlag derived from the SPS shall be equal to 0.

      if ( m_layerId > 0 && vps->getNumRefLayers( m_layerId ) == 0 )
      {
        assert( sps->getMultiLayerExtSpsFlag() == 0 );
      }
    }
    m_seiReader.setLayerId ( newPic->getLayerId      ( ) );
    m_seiReader.setDecOrder( newPic->getDecodingOrder( ) );
#endif

    xParsePrefixSEImessages();

#if RExt__HIGH_BIT_DEPTH_SUPPORT==0
    if (sps->getSpsRangeExtension().getExtendedPrecisionProcessingFlag() || sps->getBitDepth(CHANNEL_TYPE_LUMA)>12 || sps->getBitDepth(CHANNEL_TYPE_CHROMA)>12 )
    {
      printf("High bit depth support must be enabled at compile-time in order to decode this bitstream\n");
      assert (0);
      exit(1);
    }
#endif

    // NOTE: globals were set up here originally. You can now use:
    // g_uiMaxCUDepth = sps->getMaxTotalCUDepth();
    // g_uiAddCUDepth = sps->getMaxTotalCUDepth() - sps->getLog2DiffMaxMinCodingBlockSize()

    //  Get a new picture buffer. This will also set up m_pcPic, and therefore give us a SPS and PPS pointer that we can use.
#if !NH_MV
    xGetNewPicBuffer (*(sps), *(pps), m_pcPic, m_apcSlicePilot->getTLayer());

    m_apcSlicePilot->applyReferencePictureSet(m_cListPic, m_apcSlicePilot->getRPS());
#else
    m_pcPic = newPic;
#endif

    // make the slice-pilot a real slice, and set up the slice-pilot for the next slice
    assert(m_pcPic->getNumAllocatedSlice() == (m_uiSliceIdx + 1));
    m_apcSlicePilot = m_pcPic->getPicSym()->swapSliceObject(m_apcSlicePilot, m_uiSliceIdx);

    // we now have a real slice:
    TComSlice *pSlice = m_pcPic->getSlice(m_uiSliceIdx);

    // Update the PPS and SPS pointers with the ones of the picture.
    pps=pSlice->getPPS();
    sps=pSlice->getSPS();

#if NH_MV
    pSlice->setPic( m_pcPic );
    vps=pSlice->getVPS();
    // The nuh_layer_id value of the NAL unit containing the PPS that is activated for a layer layerA with nuh_layer_id equal to nuhLayerIdA shall be equal to 0, or nuhLayerIdA, or the nuh_layer_id of a direct or indirect reference layer of layerA.
    assert( pps->getLayerId() == m_layerId || pps->getLayerId( ) == 0 || vps->getDependencyFlag( m_layerId, pps->getLayerId() ) );
    // The nuh_layer_id value of the NAL unit containing the SPS that is activated for a layer layerA with nuh_layer_id equal to nuhLayerIdA shall be equal to 0, or nuhLayerIdA, or the nuh_layer_id of a direct or indirect reference layer of layerA.
    assert( sps->getLayerId() == m_layerId || sps->getLayerId( ) == 0 || vps->getDependencyFlag( m_layerId, sps->getLayerId() ) );
#endif

    // Initialise the various objects for the new set of settings
    m_cSAO.create( sps->getPicWidthInLumaSamples(), sps->getPicHeightInLumaSamples(), sps->getChromaFormatIdc(), sps->getMaxCUWidth(), sps->getMaxCUHeight(), sps->getMaxTotalCUDepth(), pps->getPpsRangeExtension().getLog2SaoOffsetScale(CHANNEL_TYPE_LUMA), pps->getPpsRangeExtension().getLog2SaoOffsetScale(CHANNEL_TYPE_CHROMA) );
    m_cLoopFilter.create( sps->getMaxTotalCUDepth() );
    m_cPrediction.initTempBuff(sps->getChromaFormatIdc());


    Bool isField = false;
    Bool isTopField = false;

    if(!m_SEIs.empty())
    {
      // Check if any new Picture Timing SEI has arrived
      SEIMessages pictureTimingSEIs = getSeisByType(m_SEIs, SEI::PICTURE_TIMING);
      if (pictureTimingSEIs.size()>0)
      {
        SEIPictureTiming* pictureTiming = (SEIPictureTiming*) *(pictureTimingSEIs.begin());
        isField    = (pictureTiming->m_picStruct == 1) || (pictureTiming->m_picStruct == 2) || (pictureTiming->m_picStruct == 9) || (pictureTiming->m_picStruct == 10) || (pictureTiming->m_picStruct == 11) || (pictureTiming->m_picStruct == 12);
        isTopField = (pictureTiming->m_picStruct == 1) || (pictureTiming->m_picStruct == 9) || (pictureTiming->m_picStruct == 11);
      }
    }

    //Set Field/Frame coding mode
    m_pcPic->setField(isField);
    m_pcPic->setTopField(isTopField);

    // transfer any SEI messages that have been received to the picture
    m_pcPic->setSEIs(m_SEIs);
    m_SEIs.clear();

    // Recursive structure
    m_cCuDecoder.create ( sps->getMaxTotalCUDepth(), sps->getMaxCUWidth(), sps->getMaxCUHeight(), sps->getChromaFormatIdc() );
    m_cCuDecoder.init   ( &m_cEntropyDecoder, &m_cTrQuant, &m_cPrediction );
    m_cTrQuant.init     ( sps->getMaxTrSize() );

    m_cSliceDecoder.create();
  }
  else
  {
#if NH_MV
    assert( m_pcPic != NULL );
    assert( newPic  == NULL );
#endif
    // make the slice-pilot a real slice, and set up the slice-pilot for the next slice
    m_pcPic->allocateNewSlice();
    assert(m_pcPic->getNumAllocatedSlice() == (m_uiSliceIdx + 1));
    m_apcSlicePilot = m_pcPic->getPicSym()->swapSliceObject(m_apcSlicePilot, m_uiSliceIdx);

    TComSlice *pSlice = m_pcPic->getSlice(m_uiSliceIdx); // we now have a real slice.

    const TComSPS *sps = pSlice->getSPS();
    const TComPPS *pps = pSlice->getPPS();

    // check that the current active PPS has not changed...
    if (m_parameterSetManager.getSPSChangedFlag(sps->getSPSId()) )
    {
      printf("Error - a new SPS has been decoded while processing a picture\n");
      exit(1);
    }
    if (m_parameterSetManager.getPPSChangedFlag(pps->getPPSId()) )
    {
      printf("Error - a new PPS has been decoded while processing a picture\n");
      exit(1);
    }

    xParsePrefixSEImessages();

    // Check if any new SEI has arrived
    if(!m_SEIs.empty())
    {
      // Currently only decoding Unit SEI message occurring between VCL NALUs copied
      SEIMessages &picSEI = m_pcPic->getSEIs();
      SEIMessages decodingUnitInfos = extractSeisByType (m_SEIs, SEI::DECODING_UNIT_INFO);
      picSEI.insert(picSEI.end(), decodingUnitInfos.begin(), decodingUnitInfos.end());
      deleteSEIs(m_SEIs);
    }
  }
}

Void TDecTop::xParsePrefixSEIsForUnknownVCLNal()
{
  while (!m_prefixSEINALUs.empty())
  {
    // do nothing?
    printf("Discarding Prefix SEI associated with unknown VCL NAL unit.\n");
    delete m_prefixSEINALUs.front();
  }
  // TODO: discard following suffix SEIs as well?
}


Void TDecTop::xParsePrefixSEImessages()
{
  while (!m_prefixSEINALUs.empty())
  {
    InputNALUnit &nalu=*m_prefixSEINALUs.front();
#if NH_MV
    m_seiReader.parseSEImessage(&(nalu.getBitstream()), m_SEIs, nalu.m_nalUnitType, m_parameterSetManager.getActiveVPS(), m_parameterSetManager.getActiveSPS(getLayerId()), m_pDecodedSEIOutputStream);
#else
    m_seiReader.parseSEImessage( &(nalu.getBitstream()), m_SEIs, nalu.m_nalUnitType, m_parameterSetManager.getActiveSPS(), m_pDecodedSEIOutputStream );
#endif
    delete m_prefixSEINALUs.front();
    m_prefixSEINALUs.pop_front();
  }
}

#if !NH_MV
Bool TDecTop::xDecodeSlice(InputNALUnit &nalu, Int &iSkipFrame, Int iPOCLastDisplay )
{
  m_apcSlicePilot->initSlice(); // the slice pilot is an object to prepare for a new slice
  // it is not associated with picture, sps or pps structures.
  if (m_bFirstSliceInPicture)
  {
#else
Void TDecTop::decodeSliceHeader(InputNALUnit &nalu )
{
  // Initialize entropy decoder
  m_cEntropyDecoder.setEntropyDecoder (&m_cCavlcDecoder);
  m_cEntropyDecoder.setBitstream      (&(nalu.getBitstream()));

  assert( nalu.m_nuhLayerId == m_layerId );
  m_apcSlicePilot->initSlice(); // the slice pilot is an object to prepare for a new slice
  // it is not associated with picture, sps or pps structures.
  m_apcSlicePilot->setLayerId( nalu.m_nuhLayerId );
  m_cEntropyDecoder.decodeFirstSliceSegmentInPicFlag( m_apcSlicePilot );
  if ( m_apcSlicePilot->getFirstSliceSegementInPicFlag() )
  {
#endif
    m_uiSliceIdx = 0;
  }
  else
  {
    m_apcSlicePilot->copySliceInfo( m_pcPic->getPicSym()->getSlice(m_uiSliceIdx-1) );
  }
  m_apcSlicePilot->setSliceIdx(m_uiSliceIdx);

  m_apcSlicePilot->setNalUnitType(nalu.m_nalUnitType);
  Bool nonReferenceFlag = (m_apcSlicePilot->getNalUnitType() == NAL_UNIT_CODED_SLICE_TRAIL_N ||
    m_apcSlicePilot->getNalUnitType() == NAL_UNIT_CODED_SLICE_TSA_N   ||
    m_apcSlicePilot->getNalUnitType() == NAL_UNIT_CODED_SLICE_STSA_N  ||
    m_apcSlicePilot->getNalUnitType() == NAL_UNIT_CODED_SLICE_RADL_N  ||
    m_apcSlicePilot->getNalUnitType() == NAL_UNIT_CODED_SLICE_RASL_N);
  m_apcSlicePilot->setTemporalLayerNonReferenceFlag(nonReferenceFlag);
#if !NH_MV
  m_apcSlicePilot->setReferenced(true); // Putting this as true ensures that picture is referenced the first time it is in an RPS
#endif
  m_apcSlicePilot->setTLayerInfo(nalu.m_temporalId);
#if NH_MV
  m_cEntropyDecoder.decodeSliceHeader (m_apcSlicePilot, &m_parameterSetManager );
#else
#if ENC_DEC_TRACE
  const UInt64 originalSymbolCount = g_nSymbolCounter;
#endif
    m_cEntropyDecoder.decodeSliceHeader (m_apcSlicePilot, &m_parameterSetManager, m_prevTid0POC);
#endif

#if NH_MV
}
#else
  // set POC for dependent slices in skipped pictures
  if(m_apcSlicePilot->getDependentSliceSegmentFlag() && m_prevSliceSkipped)
  {
    m_apcSlicePilot->setPOC(m_skippedPOC);
  }

  xUpdatePreviousTid0POC(m_apcSlicePilot);
  m_apcSlicePilot->setAssociatedIRAPPOC(m_pocCRA);
  m_apcSlicePilot->setAssociatedIRAPType(m_associatedIRAPType);


  //For inference of NoOutputOfPriorPicsFlag
  if (m_apcSlicePilot->getRapPicFlag())
  {
      if ((m_apcSlicePilot->getNalUnitType() >= NAL_UNIT_CODED_SLICE_BLA_W_LP && m_apcSlicePilot->getNalUnitType() <= NAL_UNIT_CODED_SLICE_IDR_N_LP) ||
        (m_apcSlicePilot->getNalUnitType() == NAL_UNIT_CODED_SLICE_CRA && m_bFirstSliceInSequence) ||
        (m_apcSlicePilot->getNalUnitType() == NAL_UNIT_CODED_SLICE_CRA && m_apcSlicePilot->getHandleCraAsBlaFlag()))
      {
        m_apcSlicePilot->setNoRaslOutputFlag(true);
      }
    //the inference for NoOutputPriorPicsFlag
    if (!m_bFirstSliceInBitstream && m_apcSlicePilot->getRapPicFlag() && m_apcSlicePilot->getNoRaslOutputFlag())
    {
      if (m_apcSlicePilot->getNalUnitType() == NAL_UNIT_CODED_SLICE_CRA)
      {
        m_apcSlicePilot->setNoOutputPriorPicsFlag(true);
      }
    }
    else
    {
      m_apcSlicePilot->setNoOutputPriorPicsFlag(false);
    }

    if(m_apcSlicePilot->getNalUnitType() == NAL_UNIT_CODED_SLICE_CRA)
    {
      m_craNoRaslOutputFlag = m_apcSlicePilot->getNoRaslOutputFlag();
    }
  }
  if (m_apcSlicePilot->getRapPicFlag() && m_apcSlicePilot->getNoOutputPriorPicsFlag())
  {
    m_lastPOCNoOutputPriorPics = m_apcSlicePilot->getPOC();
    m_isNoOutputPriorPics = true;
  }
  else
  {
    m_isNoOutputPriorPics = false;
  }

  //For inference of PicOutputFlag
  if (m_apcSlicePilot->getNalUnitType() == NAL_UNIT_CODED_SLICE_RASL_N || m_apcSlicePilot->getNalUnitType() == NAL_UNIT_CODED_SLICE_RASL_R)
  {
    if ( m_craNoRaslOutputFlag )
    {
      m_apcSlicePilot->setPicOutputFlag(false);
    }
  }

  if (m_apcSlicePilot->getNalUnitType() == NAL_UNIT_CODED_SLICE_CRA && m_craNoRaslOutputFlag) //Reset POC MSB when CRA has NoRaslOutputFlag equal to 1
  {
    TComPPS *pps = m_parameterSetManager.getPPS(m_apcSlicePilot->getPPSId());
    assert (pps != 0);
    TComSPS *sps = m_parameterSetManager.getSPS(pps->getSPSId());
    assert (sps != 0);
    Int iMaxPOClsb = 1 << sps->getBitsForPOC();
    m_apcSlicePilot->setPOC( m_apcSlicePilot->getPOC() & (iMaxPOClsb - 1) );
    xUpdatePreviousTid0POC(m_apcSlicePilot);
  }

  // Skip pictures due to random access
  if (isRandomAccessSkipPicture(iSkipFrame, iPOCLastDisplay))
  {
    m_prevSliceSkipped = true;
    m_skippedPOC = m_apcSlicePilot->getPOC();
    return false;
  }
  // Skip TFD pictures associated with BLA/BLANT pictures
  if (isSkipPictureForBLA(iPOCLastDisplay))
  {
    m_prevSliceSkipped = true;
    m_skippedPOC = m_apcSlicePilot->getPOC();
    return false;
  }

  // clear previous slice skipped flag
  m_prevSliceSkipped = false;

  //we should only get a different poc for a new picture (with CTU address==0)
  if (!m_apcSlicePilot->getDependentSliceSegmentFlag() && m_apcSlicePilot->getPOC()!=m_prevPOC && !m_bFirstSliceInSequence && (m_apcSlicePilot->getSliceCurStartCtuTsAddr() != 0))
  {
    printf ("Warning, the first slice of a picture might have been lost!\n");
  }

  // exit when a new picture is found
  if (!m_apcSlicePilot->getDependentSliceSegmentFlag() && (m_apcSlicePilot->getSliceCurStartCtuTsAddr() == 0 && !m_bFirstSliceInPicture) )
  {
    if (m_prevPOC >= m_pocRandomAccess)
    {
      m_prevPOC = m_apcSlicePilot->getPOC();

#if ENC_DEC_TRACE
      //rewind the trace counter since we didn't actually decode the slice
      g_nSymbolCounter = originalSymbolCount;
#endif
      return true;
    }
    m_prevPOC = m_apcSlicePilot->getPOC();
  }

  //detect lost reference picture and insert copy of earlier frame.
  {
    Int lostPoc;
    while((lostPoc=m_apcSlicePilot->checkThatAllRefPicsAreAvailable(m_cListPic, m_apcSlicePilot->getRPS(), true, m_pocRandomAccess)) > 0)
    {
      xCreateLostPicture(lostPoc-1);
    }
  }

  if (!m_apcSlicePilot->getDependentSliceSegmentFlag())
  {
    m_prevPOC = m_apcSlicePilot->getPOC();
  }
  // actual decoding starts here
  xActivateParameterSets();

  m_bFirstSliceInSequence = false;
  m_bFirstSliceInBitstream  = false;


  TComSlice* pcSlice = m_pcPic->getPicSym()->getSlice(m_uiSliceIdx);

#endif

#if NH_MV
Void TDecTop::decodeSliceSegment(InputNALUnit &nalu )
{
  TComSlice* pcSlice = m_pcPic->getPicSym()->getSlice(m_uiSliceIdx);

  if ( m_pcPic->getHasGeneratedRefPics() )
  {
    if ( pcSlice->getFirstSliceSegementInPicFlag() )
    {
      std::cout << std:: setfill(' ')
        << "Layer "  << std::setw(2) << m_pcPic->getLayerId()
        << "   POC " << std::setw(4) << m_pcPic->getPOC()
        << " Reference pictures missing. Skipping picture." << std::endl;
    }
  }
  else
  {
    //Check Multiview Main profile constraint in G.11.1.1
    //  When ViewOrderIdx[ i ] derived according to any active VPS is equal to 1
    //  for the layer with nuh_layer_id equal to i in subBitstream,
    //  inter_view_mv_vert_constraint_flag shall be equal to 1
    //  in the sps_multilayer_extension( ) syntax structure in each active SPS for that layer.
    if( pcSlice->getSPS()->getPTL()->getGeneralPTL()->getProfileIdc()==Profile::MULTIVIEWMAIN
      &&
      pcSlice->getVPS()->getViewOrderIdx(pcSlice->getVPS()->getLayerIdInNuh(getLayerId()))==1
      )
    {
      assert( pcSlice->getSPS()->getInterViewMvVertConstraintFlag()==1 );
    }

    m_pcPic->setLayerId( nalu.m_nuhLayerId );
    m_pcPic->setViewId ( getViewId() );
#endif

    // When decoding the slice header, the stored start and end addresses were actually RS addresses, not TS addresses.
    // Now, having set up the maps, convert them to the correct form.
    pcSlice->setSliceSegmentCurStartCtuTsAddr( m_pcPic->getPicSym()->getCtuRsToTsAddrMap(pcSlice->getSliceSegmentCurStartCtuTsAddr()) );
    pcSlice->setSliceSegmentCurEndCtuTsAddr( m_pcPic->getPicSym()->getCtuRsToTsAddrMap(pcSlice->getSliceSegmentCurEndCtuTsAddr()) );
    if(!pcSlice->getDependentSliceSegmentFlag())
    {
      pcSlice->setSliceCurStartCtuTsAddr(m_pcPic->getPicSym()->getCtuRsToTsAddrMap(pcSlice->getSliceCurStartCtuTsAddr()));
      pcSlice->setSliceCurEndCtuTsAddr(m_pcPic->getPicSym()->getCtuRsToTsAddrMap(pcSlice->getSliceCurEndCtuTsAddr()));
    }

    m_pcPic->setTLayer(nalu.m_temporalId);


    if (!pcSlice->getDependentSliceSegmentFlag())
    {
#if !NH_MV
      pcSlice->checkCRA(pcSlice->getRPS(), m_pocCRA, m_associatedIRAPType, m_cListPic );
      // Set reference list
      pcSlice->setRefPicList( m_cListPic, true );
#else
      if (pcSlice->getSliceType() != I_SLICE )
      {
        if( m_decProcPocAndRps == ANNEX_F )
        {
          pcSlice->f834decProcForRefPicListConst();
        }
        else if ( m_decProcPocAndRps == CLAUSE_8 )
        {
          pcSlice->cl834DecProcForRefPicListConst();
        }
        else
        {
          assert( false );
        }
      }
#endif

      // For generalized B
      // note: maybe not existed case (always L0 is copied to L1 if L1 is empty)
      if (pcSlice->isInterB() && pcSlice->getNumRefIdx(REF_PIC_LIST_1) == 0)
      {
        Int iNumRefIdx = pcSlice->getNumRefIdx(REF_PIC_LIST_0);
        pcSlice->setNumRefIdx        ( REF_PIC_LIST_1, iNumRefIdx );

        for (Int iRefIdx = 0; iRefIdx < iNumRefIdx; iRefIdx++)
        {
          pcSlice->setRefPic(pcSlice->getRefPic(REF_PIC_LIST_0, iRefIdx), REF_PIC_LIST_1, iRefIdx);
        }
      }
      if (!pcSlice->isIntra())
      {
        Bool bLowDelay = true;
        Int  iCurrPOC  = pcSlice->getPOC();
        Int iRefIdx = 0;

        for (iRefIdx = 0; iRefIdx < pcSlice->getNumRefIdx(REF_PIC_LIST_0) && bLowDelay; iRefIdx++)
        {
          if ( pcSlice->getRefPic(REF_PIC_LIST_0, iRefIdx)->getPOC() > iCurrPOC )
          {
            bLowDelay = false;
          }
        }
        if (pcSlice->isInterB())
        {
          for (iRefIdx = 0; iRefIdx < pcSlice->getNumRefIdx(REF_PIC_LIST_1) && bLowDelay; iRefIdx++)
          {
            if ( pcSlice->getRefPic(REF_PIC_LIST_1, iRefIdx)->getPOC() > iCurrPOC )
            {
              bLowDelay = false;
            }
          }
        }

        pcSlice->setCheckLDC(bLowDelay);
      }

      //---------------
      pcSlice->setRefPOCList();
    }

    m_pcPic->setCurrSliceIdx(m_uiSliceIdx);
    if(pcSlice->getSPS()->getScalingListFlag())
    {
      TComScalingList scalingList;
      if(pcSlice->getPPS()->getScalingListPresentFlag())
      {
        scalingList = pcSlice->getPPS()->getScalingList();
      }
      else if (pcSlice->getSPS()->getScalingListPresentFlag())
      {
        scalingList = pcSlice->getSPS()->getScalingList();
      }
      else
      {
        scalingList.setDefaultScalingList();
      }
      m_cTrQuant.setScalingListDec(scalingList);
      m_cTrQuant.setUseScalingList(true);
    }
    else
    {
      const Int maxLog2TrDynamicRange[MAX_NUM_CHANNEL_TYPE] =
      {
        pcSlice->getSPS()->getMaxLog2TrDynamicRange(CHANNEL_TYPE_LUMA),
        pcSlice->getSPS()->getMaxLog2TrDynamicRange(CHANNEL_TYPE_CHROMA)
      };
      m_cTrQuant.setFlatScalingList(maxLog2TrDynamicRange, pcSlice->getSPS()->getBitDepths());
      m_cTrQuant.setUseScalingList(false);
    }


    //  Decode a picture
    m_cGopDecoder.decompressSlice(&(nalu.getBitstream()), m_pcPic);


#if !NH_MV
    m_bFirstSliceInPicture = false;
#else
  }
#endif
  m_uiSliceIdx++;

#if !NH_MV
  return false;
#endif
}


Void TDecTop::xDecodeVPS(const std::vector<UChar> &naluData)
{
  TComVPS* vps = new TComVPS();

  m_cEntropyDecoder.decodeVPS( vps );
  m_parameterSetManager.storeVPS(vps, naluData);
}

Void TDecTop::xDecodeSPS(const std::vector<UChar> &naluData)
{
  TComSPS* sps = new TComSPS();
#if NH_MV
  sps->setLayerId( getLayerId() );
#endif
#if O0043_BEST_EFFORT_DECODING
  sps->setForceDecodeBitDepth(m_forceDecodeBitDepth);
#endif
  m_cEntropyDecoder.decodeSPS( sps );
  m_parameterSetManager.storeSPS(sps, naluData);
}

Void TDecTop::xDecodePPS(const std::vector<UChar> &naluData)
{
  TComPPS* pps = new TComPPS();
#if NH_MV
  pps->setLayerId( getLayerId() );
#endif
  m_cEntropyDecoder.decodePPS( pps );

  m_parameterSetManager.storePPS( pps, naluData);
}

#if NH_MV
Bool TDecTop::decodeNonVclNalu(InputNALUnit& nalu )
#else
Bool TDecTop::decode(InputNALUnit& nalu, Int& iSkipFrame, Int& iPOCLastDisplay)
#endif
{
#if !NH_MV
  // ignore all NAL units of layers > 0
  if (nalu.m_nuhLayerId > 0)
  {
    fprintf (stderr, "Warning: found NAL unit with nuh_layer_id equal to %d. Ignoring.\n", nalu.m_nuhLayerId);
    return false;
  }
#endif
  // Initialize entropy decoder
  m_cEntropyDecoder.setEntropyDecoder (&m_cCavlcDecoder);
  m_cEntropyDecoder.setBitstream      (&(nalu.getBitstream()));

  switch (nalu.m_nalUnitType)
  {
    case NAL_UNIT_VPS:
      xDecodeVPS(nalu.getBitstream().getFifo());
      return false;

    case NAL_UNIT_SPS:
      xDecodeSPS(nalu.getBitstream().getFifo());
      return false;

    case NAL_UNIT_PPS:
      xDecodePPS(nalu.getBitstream().getFifo());
      return false;

    case NAL_UNIT_PREFIX_SEI:
      // Buffer up prefix SEI messages until SPS of associated VCL is known.
      m_prefixSEINALUs.push_back(new InputNALUnit(nalu));
      return false;

    case NAL_UNIT_SUFFIX_SEI:
      if (m_pcPic)
      {
#if NH_MV
      m_seiReader.parseSEImessage( &(nalu.getBitstream()), m_pcPic->getSEIs(), nalu.m_nalUnitType, m_parameterSetManager.getActiveVPS(), m_parameterSetManager.getActiveSPS(getLayerId()), m_pDecodedSEIOutputStream);
#else
        m_seiReader.parseSEImessage( &(nalu.getBitstream()), m_pcPic->getSEIs(), nalu.m_nalUnitType, m_parameterSetManager.getActiveSPS(), m_pDecodedSEIOutputStream );
#endif
      }
      else
      {
        printf ("Note: received suffix SEI but no picture currently active.\n");
      }
      return false;

    case NAL_UNIT_CODED_SLICE_TRAIL_R:
    case NAL_UNIT_CODED_SLICE_TRAIL_N:
    case NAL_UNIT_CODED_SLICE_TSA_R:
    case NAL_UNIT_CODED_SLICE_TSA_N:
    case NAL_UNIT_CODED_SLICE_STSA_R:
    case NAL_UNIT_CODED_SLICE_STSA_N:
    case NAL_UNIT_CODED_SLICE_BLA_W_LP:
    case NAL_UNIT_CODED_SLICE_BLA_W_RADL:
    case NAL_UNIT_CODED_SLICE_BLA_N_LP:
    case NAL_UNIT_CODED_SLICE_IDR_W_RADL:
    case NAL_UNIT_CODED_SLICE_IDR_N_LP:
    case NAL_UNIT_CODED_SLICE_CRA:
    case NAL_UNIT_CODED_SLICE_RADL_N:
    case NAL_UNIT_CODED_SLICE_RADL_R:
    case NAL_UNIT_CODED_SLICE_RASL_N:
    case NAL_UNIT_CODED_SLICE_RASL_R:
#if NH_MV
      assert( false );
      return 1;
#else
      return xDecodeSlice(nalu, iSkipFrame, iPOCLastDisplay);
#endif
      break;

    case NAL_UNIT_EOS:
#if !NH_MV
      m_associatedIRAPType = NAL_UNIT_INVALID;
      m_pocCRA = 0;
      m_pocRandomAccess = MAX_INT;
      m_prevPOC = MAX_INT;
      m_prevSliceSkipped = false;
      m_skippedPOC = 0;
#endif
      return false;

    case NAL_UNIT_ACCESS_UNIT_DELIMITER:
      {
        AUDReader audReader;
        UInt picType;
        audReader.parseAccessUnitDelimiter(&(nalu.getBitstream()),picType);
        printf ("Note: found NAL_UNIT_ACCESS_UNIT_DELIMITER\n");
      return false;
      }

    case NAL_UNIT_EOB:
      return false;

    case NAL_UNIT_FILLER_DATA:
      {
        FDReader fdReader;
        UInt size;
        fdReader.parseFillerData(&(nalu.getBitstream()),size);
        printf ("Note: found NAL_UNIT_FILLER_DATA with %u bytes payload.\n", size);
      return false;
      }

    case NAL_UNIT_RESERVED_VCL_N10:
    case NAL_UNIT_RESERVED_VCL_R11:
    case NAL_UNIT_RESERVED_VCL_N12:
    case NAL_UNIT_RESERVED_VCL_R13:
    case NAL_UNIT_RESERVED_VCL_N14:
    case NAL_UNIT_RESERVED_VCL_R15:

    case NAL_UNIT_RESERVED_IRAP_VCL22:
    case NAL_UNIT_RESERVED_IRAP_VCL23:

    case NAL_UNIT_RESERVED_VCL24:
    case NAL_UNIT_RESERVED_VCL25:
    case NAL_UNIT_RESERVED_VCL26:
    case NAL_UNIT_RESERVED_VCL27:
    case NAL_UNIT_RESERVED_VCL28:
    case NAL_UNIT_RESERVED_VCL29:
    case NAL_UNIT_RESERVED_VCL30:
    case NAL_UNIT_RESERVED_VCL31:
      printf ("Note: found reserved VCL NAL unit.\n");
      xParsePrefixSEIsForUnknownVCLNal();
      return false;

    case NAL_UNIT_RESERVED_NVCL41:
    case NAL_UNIT_RESERVED_NVCL42:
    case NAL_UNIT_RESERVED_NVCL43:
    case NAL_UNIT_RESERVED_NVCL44:
    case NAL_UNIT_RESERVED_NVCL45:
    case NAL_UNIT_RESERVED_NVCL46:
    case NAL_UNIT_RESERVED_NVCL47:
      printf ("Note: found reserved NAL unit.\n");
      return false;
    case NAL_UNIT_UNSPECIFIED_48:
    case NAL_UNIT_UNSPECIFIED_49:
    case NAL_UNIT_UNSPECIFIED_50:
    case NAL_UNIT_UNSPECIFIED_51:
    case NAL_UNIT_UNSPECIFIED_52:
    case NAL_UNIT_UNSPECIFIED_53:
    case NAL_UNIT_UNSPECIFIED_54:
    case NAL_UNIT_UNSPECIFIED_55:
    case NAL_UNIT_UNSPECIFIED_56:
    case NAL_UNIT_UNSPECIFIED_57:
    case NAL_UNIT_UNSPECIFIED_58:
    case NAL_UNIT_UNSPECIFIED_59:
    case NAL_UNIT_UNSPECIFIED_60:
    case NAL_UNIT_UNSPECIFIED_61:
    case NAL_UNIT_UNSPECIFIED_62:
    case NAL_UNIT_UNSPECIFIED_63:
      printf ("Note: found unspecified NAL unit.\n");
      return false;
    default:
      assert (0);
      break;
  }

  return false;
}

#if !NH_MV
/** Function for checking if picture should be skipped because of association with a previous BLA picture
 * \param iPOCLastDisplay POC of last picture displayed
 * \returns true if the picture should be skipped
 * This function skips all TFD pictures that follow a BLA picture
 * in decoding order and precede it in output order.
 */
Bool TDecTop::isSkipPictureForBLA(Int& iPOCLastDisplay)
{
  if ((m_associatedIRAPType == NAL_UNIT_CODED_SLICE_BLA_N_LP || m_associatedIRAPType == NAL_UNIT_CODED_SLICE_BLA_W_LP || m_associatedIRAPType == NAL_UNIT_CODED_SLICE_BLA_W_RADL) &&
       m_apcSlicePilot->getPOC() < m_pocCRA && (m_apcSlicePilot->getNalUnitType() == NAL_UNIT_CODED_SLICE_RASL_R || m_apcSlicePilot->getNalUnitType() == NAL_UNIT_CODED_SLICE_RASL_N))
  {
    iPOCLastDisplay++;
    return true;
  }
  return false;
}

/** Function for checking if picture should be skipped because of random access
 * \param iSkipFrame skip frame counter
 * \param iPOCLastDisplay POC of last picture displayed
 * \returns true if the picture shold be skipped in the random access.
 * This function checks the skipping of pictures in the case of -s option random access.
 * All pictures prior to the random access point indicated by the counter iSkipFrame are skipped.
 * It also checks the type of Nal unit type at the random access point.
 * If the random access point is CRA/CRANT/BLA/BLANT, TFD pictures with POC less than the POC of the random access point are skipped.
 * If the random access point is IDR all pictures after the random access point are decoded.
 * If the random access point is none of the above, a warning is issues, and decoding of pictures with POC
 * equal to or greater than the random access point POC is attempted. For non IDR/CRA/BLA random
 * access point there is no guarantee that the decoder will not crash.
 */
Bool TDecTop::isRandomAccessSkipPicture(Int& iSkipFrame,  Int& iPOCLastDisplay )
{
  if (iSkipFrame)
  {
    iSkipFrame--;   // decrement the counter
    return true;
  }
  else if (m_pocRandomAccess == MAX_INT) // start of random access point, m_pocRandomAccess has not been set yet.
  {
    if (   m_apcSlicePilot->getNalUnitType() == NAL_UNIT_CODED_SLICE_CRA
        || m_apcSlicePilot->getNalUnitType() == NAL_UNIT_CODED_SLICE_BLA_W_LP
        || m_apcSlicePilot->getNalUnitType() == NAL_UNIT_CODED_SLICE_BLA_N_LP
        || m_apcSlicePilot->getNalUnitType() == NAL_UNIT_CODED_SLICE_BLA_W_RADL )
    {

      // set the POC random access since we need to skip the reordered pictures in the case of CRA/CRANT/BLA/BLANT.
      m_pocRandomAccess = m_apcSlicePilot->getPOC();
    }
    else if ( m_apcSlicePilot->getNalUnitType() == NAL_UNIT_CODED_SLICE_IDR_W_RADL || m_apcSlicePilot->getNalUnitType() == NAL_UNIT_CODED_SLICE_IDR_N_LP )
    {
      m_pocRandomAccess = -MAX_INT; // no need to skip the reordered pictures in IDR, they are decodable.
    }
    else
    {
      if(!m_warningMessageSkipPicture)
      {
        printf("\nWarning: this is not a valid random access point and the data is discarded until the first CRA picture");
        m_warningMessageSkipPicture = true;
      }
      return true;
    }
  }
  // skip the reordered pictures, if necessary
  else if (m_apcSlicePilot->getPOC() < m_pocRandomAccess && (m_apcSlicePilot->getNalUnitType() == NAL_UNIT_CODED_SLICE_RASL_R || m_apcSlicePilot->getNalUnitType() == NAL_UNIT_CODED_SLICE_RASL_N))
  {
    iPOCLastDisplay++;
    return true;
  }
  // if we reach here, then the picture is not skipped.
  return false;
}

#else

Int TDecTop::preDecodePoc( Bool firstPicInLayerDecodedFlag, Bool isFstPicOfAllLayOfPocResetPer, Bool isPocResettingPicture )
{
  //Output of this process is PicOrderCntVal, the picture order count of the current picture.
  //  Picture order counts are used to identify pictures, for deriving motion parameters in merge mode and
  //  motion vector prediction and for decoder conformance checking (see clause F.13.5).

  //  Each coded picture is associated with a picture order count variable, denoted as PicOrderCntVal.

  TComSlice* slice = m_apcSlicePilot;
  const Int nuhLayerId   = slice->getLayerId();
  const TComVPS*   vps   = slice->getVPS();
  const TComSPS*   sps   = slice->getSPS();

  Int pocDecrementedInDpbFlag = m_pocDecrementedInDpbFlag[ nuhLayerId ];

  if ( isFstPicOfAllLayOfPocResetPer )
  {
    //  When the current picture is the first picture among all layers of a POC resetting period,
    //  the variable PocDecrementedInDPBFlag[ i ] is set equal to 0 for each value of i in the range of 0 to 62, inclusive.
    pocDecrementedInDpbFlag = false;
  }

  //  The variable pocResettingFlag is derived as follows:
  Bool pocResettingFlag;
  if ( isPocResettingPicture )
  {
    //-  If the current picture is a POC resetting picture, the following applies:
    if( vps->getVpsPocLsbAlignedFlag()  )
    {
      //  -  If vps_poc_lsb_aligned_flag is equal to 0, pocResettingFlag is set equal to 1.
      pocResettingFlag = true;
    }
    else if ( pocDecrementedInDpbFlag )
    {
      //  -  Otherwise, if PocDecrementedInDPBFlag[ nuh_layer_id ] is equal to 1, pocResettingFlag is set equal to 0.
      pocResettingFlag = false;
    }
    else
    {
      //  -  Otherwise, pocResettingFlag is set equal to 1.
      pocResettingFlag = true;
    }
  }
  else
  {
    //  -  Otherwise, pocResettingFlag is set equal to 0.
    pocResettingFlag = false;
  }

  Int picOrderCntMsb;
  Int picOrderCntVal;

  //  Depending on pocResettingFlag, the following applies:
  if ( pocResettingFlag )
  {
    //-  The PicOrderCntVal of the current picture is derived as follows:
    if( slice->getPocResetIdc()  ==  1 )
    {
      picOrderCntVal = slice->getSlicePicOrderCntLsb();
    }
    else if (slice->getPocResetIdc()  ==  2 )
    {
      picOrderCntVal = 0;
    }
    else
    {
      picOrderCntMsb = xGetCurrMsb( slice->getSlicePicOrderCntLsb(), slice->getFullPocResetFlag() ? 0 : slice->getPocLsbVal(), 0, sps->getMaxPicOrderCntLsb() );
      picOrderCntVal = picOrderCntMsb + slice->getSlicePicOrderCntLsb();
    }
  }
  else
  {
    //-  Otherwise (pocResettingFlag is equal to 0), the following applies:
    //-  The PicOrderCntVal of the current picture is derived as follows:

    if( slice->getPocMsbCycleValPresentFlag() )
    {
      picOrderCntMsb = slice->getPocMsbCycleVal() * sps->getMaxPicOrderCntLsb();
    }
    else if( !firstPicInLayerDecodedFlag  ||
      slice->getNalUnitType()  ==  NAL_UNIT_CODED_SLICE_IDR_N_LP || slice->getNalUnitType() ==  NAL_UNIT_CODED_SLICE_IDR_W_RADL )
    {
      picOrderCntMsb = 0; //     (F 62)
    }
    else
    {
      Int prevPicOrderCntLsb = m_prevPicOrderCnt & ( sps->getMaxPicOrderCntLsb() - 1 );
      Int prevPicOrderCntMsb = m_prevPicOrderCnt - prevPicOrderCntLsb;
      picOrderCntMsb = xGetCurrMsb( slice->getSlicePicOrderCntLsb(), prevPicOrderCntLsb, prevPicOrderCntMsb, sps->getMaxPicOrderCntLsb() );
    }
    picOrderCntVal = picOrderCntMsb + slice->getSlicePicOrderCntLsb();
  }
  return picOrderCntVal;
}

Void TDecTop::inferPocResetPeriodId()
{
  // Infer PocResetPeriodId
  // When not present, the value of poc_reset_period_id is inferred as follows:

  if ( !m_apcSlicePilot->getHasPocResetPeriodIdPresent() )
  {
    if ( m_lastPresentPocResetIdc[ m_apcSlicePilot->getLayerId() ] != MIN_INT )
    {
      // - If the previous picture picA that has poc_reset_period_id present in the slice segment header is present in the same layer
      //   of the bitstream as the current picture, the value of poc_reset_period_id is inferred to be equal to the value of the
      //   poc_reset_period_id of picA.

      m_apcSlicePilot->setPocResetPeriodId( m_lastPresentPocResetIdc[ m_apcSlicePilot->getLayerId() ] );
    }
    else
    {
      //- Otherwise, the value of poc_reset_period_id is inferred to be equal to 0.
      m_apcSlicePilot->setPocResetPeriodId( 0 );
    }
  }
  else
  {
    m_lastPresentPocResetIdc[ m_apcSlicePilot->getLayerId() ] = m_apcSlicePilot->getPocResetPeriodId();
  }
}


Void TDecTop::decodePocAndRps( )
{
  assert( m_uiSliceIdx == 0 );
  Int nuhLayerId = m_pcPic->getLayerId();
  if ( m_decProcPocAndRps == CLAUSE_8 )
  {
    // 8.1.3 Decoding process for a coded picture with nuh_layer_id equal to 0

    // Variables and functions relating to picture order count are derived as
    // specified in clause 8.3.1. This needs to be invoked only for the first slice
    // segment of a picture.
    x831DecProcForPicOrderCount( );

    // The decoding process for RPS in clause 8.3.2 is invoked, wherein reference
    // pictures may be marked as "unused for reference" or "used for long-term
    // reference". This needs to be invoked only for the first slice segment of a
    // picture.
    x832DecProcForRefPicSet    (  false );
  }
  else if( m_decProcPocAndRps == ANNEX_F )
  {
    // F.8.1.3 Common decoding process for a coded picture

    if (nuhLayerId == 0 )
    {
      // F.8.1.4 Decoding process for a coded picture with nuh_layer_id equal to
      // --> Clause 8.1.3 is invoked with replacments of 8.3.1, 8.3.2, and 8.3.3 by F.8.3.1, 8.3.2, and 8.3.3

      // Variables and functions relating to picture order count are derived as
      // specified in clause 8.3.1. This needs to be invoked only for the first slice
      // segment of a picture.
      xF831DecProcForPicOrderCount( );

      // The decoding process for RPS in clause 8.3.2 is invoked, wherein reference
      // pictures may be marked as "unused for reference" or "used for long-term
      // reference". This needs to be invoked only for the first slice segment of a
      // picture.
      xF832DecProcForRefPicSet( );
    }
    else
    {
      // F.8.1.5 Decoding process for starting the decoding of a coded picture with
      // nuh_layer_id greater than 0

      // Variables and functions relating to picture order count are derived in clause F.8.3.1.
      // This needs to be invoked only for the first slice segment of a picture. It is a requirement
      // of bitstream conformance that PicOrderCntVal of each picture in an access unit shall have the
      // same value during and at the end of decoding of the access unit
      xF831DecProcForPicOrderCount( );

      // The decoding process for RPS in clause F.8.3.2 is invoked, wherein only reference pictures with
      // nuh_layer_id equal to that of CurrPic may be marked as "unused for reference" or "used for
      // long-term reference" and any picture with a different value of nuh_layer_id is not marked.
      // This needs to be invoked only for the first slice segment of a picture.
      xF832DecProcForRefPicSet( );
    }
  }
  else
  {
    assert( false );
  }
}

Void TDecTop::genUnavailableRefPics( )
{
  assert( m_uiSliceIdx == 0 );
  Int nuhLayerId = m_pcPic->getLayerId();
  if ( m_decProcPocAndRps == CLAUSE_8 )
  {
    // 8.1.3 Decoding process for a coded picture with nuh_layer_id equal to 0

    if ( m_pcPic->isBla() || ( m_pcPic->isCra() && m_pcPic->getNoRaslOutputFlag() ) )
    {
      // When the current picture is a BLA picture or is a CRA picture
      // with NoRaslOutputFlag equal to 1, the decoding process for generating
      // unavailable reference pictures specified in clause 8.3.3 is invoked,
      // which needs to be invoked only for the first slice segment of a picture.
      x8331GenDecProcForGenUnavilRefPics();
    }
  }
  else if( m_decProcPocAndRps == ANNEX_F )
  {
    // F.8.1.3 Common decoding process for a coded picture

    if (nuhLayerId == 0 )
    {
      // F.8.1.4 Decoding process for a coded picture with nuh_layer_id equal to
      // --> Clause 8.1.3 is invoked with replacments of 8.3.1, 8.3.2, and 8.3.3 by F.8.3.1, 8.3.2, and 8.3.3

      if ( m_pcPic->isBla() || ( m_pcPic->isCra() && m_pcPic->getNoRaslOutputFlag() ) )
      {
        // When the current picture is a BLA picture or is a CRA picture
        // with NoRaslOutputFlag equal to 1, the decoding process for generating
        // unavailable reference pictures specified in clause 8.3.3 is invoked,
        // which needs to be invoked only for the first slice segment of a picture.
        xF833DecProcForGenUnavRefPics();
      }
#if NH_MV_FIX_INIT_NUM_ACTIVE_REF_LAYER_PICS
      TComDecodedRps* decRps = m_pcPic->getDecodedRps();
      decRps->m_numActiveRefLayerPics0 = 0;
      decRps->m_numActiveRefLayerPics1 = 0;
#endif
    }
    else
    {
      // F.8.1.5 Decoding process for starting the decoding of a coded picture with
      // nuh_layer_id greater than 0

      if ( !m_firstPicInLayerDecodedFlag[ nuhLayerId ] )
      {
        // When FirstPicInLayerDecodedFlag[ nuh_layer_id ] is equal to 0, the decoding process for generating
        // unavailable reference pictures for pictures first in decoding order within a layer specified in
        // clause F.8.1.7 is invoked, which needs to be invoked only for the first slice segment of a picture.
        xF817DecProcForGenUnavRefPicForPicsFrstInDecOrderInLay();
      }

      if ( m_firstPicInLayerDecodedFlag[ nuhLayerId ] && ( m_pcPic->isIrap() && m_pcPic->getNoRaslOutputFlag() ) )
      {
        // When FirstPicInLayerDecodedFlag[ nuh_layer_id ] is equal to 1 and the current picture is an IRAP
        // picture with NoRaslOutputFlag equal to 1, the decoding process for generating unavailable reference
        // pictures specified in clause F.8.3.3 is invoked, which needs to be invoked only for the first slice
        // segment of a picture.
        xF833DecProcForGenUnavRefPics();
      }

      if ( decProcAnnexG() )
      {
        // G.1.2 --> G.1.3
        xG813DecProcForInterLayerRefPicSet();
      }
      else
      {
#if NH_MV_FIX_INIT_NUM_ACTIVE_REF_LAYER_PICS
        TComDecodedRps* decRps = m_pcPic->getDecodedRps(); 
        decRps->m_numActiveRefLayerPics0 = 0;
        decRps->m_numActiveRefLayerPics1 = 0;      
#endif
      }
    }
  }
  else
  {
    assert( false );
  }

  xCheckUnavailableRefPics();
}
Void TDecTop::executeLoopFilters( )
{
  assert( m_pcPic != NULL );
  if ( !m_pcPic->getHasGeneratedRefPics() && !m_pcPic->getIsGenerated() )
  {
    m_cGopDecoder.filterPicture( m_pcPic );
  }
  m_cCuDecoder.destroy();
}

Void TDecTop::finalizePic()
{
  if( m_pcPic->isIrap() )
  {
    m_prevIrapPoc           = m_pcPic->getPOC();
    m_prevIrapDecodingOrder = m_pcPic->getDecodingOrder();
  }
  if( m_pcPic->isStsa() )
  {
    m_prevStsaDecOrder      = m_pcPic->getDecodingOrder();
    m_prevStsaTemporalId    = m_pcPic->getTemporalId()   ;
  }
}


Void TDecTop::initFromActiveVps( const TComVPS* vps )
{
  setViewId   ( vps->getViewId   ( getLayerId() )      );

  if ( !vps->getVpsExtensionFlag() )
  {
    m_decodingProcess = CLAUSE_8;
    m_isInOwnTargetDecLayerIdList = ( getLayerId() ==  0 );
  }
  else
  {
    if ( m_targetOlsIdx == -1 )
    {
      // Corresponds to specification by "External Means". (Should be set equal to 0, when no external means available. )
      m_targetOlsIdx = vps->getVpsNumLayerSetsMinus1();
    }

    Int targetDecLayerSetIdx = vps->olsIdxToLsIdx( m_targetOlsIdx );

    if ( targetDecLayerSetIdx <= vps->getVpsNumLayerSetsMinus1() && vps->getVpsBaseLayerInternalFlag() )
    {
      m_smallestLayerId = 0;
    }
    else if ( targetDecLayerSetIdx <= vps->getVpsNumLayerSetsMinus1() && !vps->getVpsBaseLayerInternalFlag() )
    {
      m_smallestLayerId = 0;
    }
    else if ( targetDecLayerSetIdx > vps->getVpsNumLayerSetsMinus1() && vps->getNumLayersInIdList( targetDecLayerSetIdx) == 1 )
    {

      // m_smallestLayerId = 0;
      // For now don't do change of layer id here.
      m_smallestLayerId = vps->getTargetDecLayerIdList( targetDecLayerSetIdx )[ 0 ];
    }
    else
    {
      m_smallestLayerId = vps->getTargetDecLayerIdList( targetDecLayerSetIdx )[ 0 ];
    }


    // Set profile
    Int lsIdx = vps->olsIdxToLsIdx( m_targetOlsIdx );
    Int lIdx = -1;
    for (Int j = 0; j < vps->getNumLayersInIdList( lsIdx ) ; j++ )
    {
      if ( vps->getLayerSetLayerIdList( lsIdx, j ) == getLayerId() )
      {
        lIdx = j;
        break;
      }
    }
    m_isInOwnTargetDecLayerIdList = (lIdx != -1);

    if ( m_isInOwnTargetDecLayerIdList )
    {
      Int profileIdc = vps->getPTL( vps->getProfileTierLevelIdx( m_targetOlsIdx, lIdx ) )->getGeneralPTL()->getProfileIdc();
      assert( profileIdc == 1 || profileIdc == 6 || profileIdc == 8 );

      if (  profileIdc == 6 )
      {
        m_decodingProcess = ANNEX_G;
      }
      else if (profileIdc == 7 )
      {
        m_decodingProcess = ANNEX_H;
      }
      else if (profileIdc == 8 )
      {
        m_decodingProcess = ANNEX_I;
      }
    }
  }
}


Bool TDecTop::getFirstSliceSegementInPicFlag()
{
  return m_apcSlicePilot->getFirstSliceSegementInPicFlag();
}

Void TDecTop::x831DecProcForPicOrderCount()
{
  /////////////////////////////////////////////////////
  // 8.3.1 Decoding process for picture order count //
  /////////////////////////////////////////////////////

  //  Output of this process is PicOrderCntVal, the picture order count of the current picture.
  //  Picture order counts are used to identify pictures, for deriving motion parameters in merge mode and
  //  motion vector prediction, and for decoder conformance checking (see clause C.5).
  //  Each coded picture is associated with a picture order count variable, denoted as PicOrderCntVal.

  const TComSlice* curSlice = m_pcPic->getSlice(0);

  Int prevPicOrderCntLsb = MIN_INT;
  Int prevPicOrderCntMsb = MIN_INT;
  if (!(m_pcPic->isIrap() && m_pcPic->getNoRaslOutputFlag() )  )
  {
    //  When the current picture is not an IRAP picture with NoRaslOutputFlag equal to 1,
    //  the variables prevPicOrderCntLsb and prevPicOrderCntMsb are derived as follows:

    //  -  Let prevTid0Pic be the previous picture in decoding order that has TemporalId equal to 0 and that is not a RASL picture,
    //     a RADL picture or an SLNR picture.

    //  -  The variable prevPicOrderCntLsb is set equal to slice_pic_order_cnt_lsb of prevTid0Pic.
    prevPicOrderCntLsb = m_prevTid0PicSlicePicOrderCntLsb;

    //  -  The variable prevPicOrderCntMsb is set equal to PicOrderCntMsb of prevTid0Pic.
    prevPicOrderCntMsb = m_prevTid0PicPicOrderCntMsb;
  }

  //  The variable PicOrderCntMsb of the current picture is derived as follows:

  Int slicePicOrderCntLsb = curSlice->getSlicePicOrderCntLsb();

  Int picOrderCntMsb;

  if (m_pcPic->isIrap() && m_pcPic->getNoRaslOutputFlag()  )
  {
    //-  If the current picture is an IRAP picture with NoRaslOutputFlag equal to 1, PicOrderCntMsb is set equal to 0.
    picOrderCntMsb = 0;
  }
  else
  {
    Int maxPicOrderCntLsb   = curSlice->getSPS()->getMaxPicOrderCntLsb();

  //  -  Otherwise, PicOrderCntMsb is derived as follows:

    if( ( slicePicOrderCntLsb < prevPicOrderCntLsb )  &&
      ( ( prevPicOrderCntLsb - slicePicOrderCntLsb )  >=  ( maxPicOrderCntLsb / 2 ) ) )
    {
      picOrderCntMsb = prevPicOrderCntMsb + maxPicOrderCntLsb;   // (8 1)
    }
    else if( (slicePicOrderCntLsb > prevPicOrderCntLsb )  &&
    ( ( slicePicOrderCntLsb - prevPicOrderCntLsb ) > ( maxPicOrderCntLsb / 2 ) ) )
    {
      picOrderCntMsb = prevPicOrderCntMsb - maxPicOrderCntLsb;
    }
    else
    {
      picOrderCntMsb = prevPicOrderCntMsb;
    }
  }

  //PicOrderCntVal is derived as follows:
  Int picOrderCntVal = picOrderCntMsb + slicePicOrderCntLsb; //   (8 2)

  //  NOTE 1 - All IDR pictures will have PicOrderCntVal equal to 0 since slice_pic_order_cnt_lsb is inferred to be 0 for IDR
  //  pictures and prevPicOrderCntLsb and prevPicOrderCntMsb are both set equal to 0.

  m_pcPic->getSlice(0)->setPOC( picOrderCntVal );

  // Update prevTid0Pic
  //   Let prevTid0Pic be the previous picture in decoding order that has TemporalId equal to 0 and that is not a RASL picture, a RADL picture or an SLNR picture.
  if( curSlice->getTemporalId() == 0  && !m_pcPic->isRasl() && !m_pcPic->isRadl() && !m_pcPic->isSlnr() )
  {
    m_prevTid0PicSlicePicOrderCntLsb = slicePicOrderCntLsb;
    m_prevTid0PicPicOrderCntMsb      = picOrderCntMsb;
  }
}

Void TDecTop::xF831DecProcForPicOrderCount()
{
  //Output of this process is PicOrderCntVal, the picture order count of the current picture.
  //  Picture order counts are used to identify pictures, for deriving motion parameters in merge mode and
  //  motion vector prediction and for decoder conformance checking (see clause F.13.5).

  //  Each coded picture is associated with a picture order count variable, denoted as PicOrderCntVal.

  const TComSlice* slice = m_pcPic->getSlice(0);
  const Int nuhLayerId   = m_pcPic->getLayerId();
  const TComVPS*   vps   = slice->getVPS();
  const TComSPS*   sps   = slice->getSPS();
  if ( m_pcPic->getIsFstPicOfAllLayOfPocResetPer() )
  {
    //  When the current picture is the first picture among all layers of a POC resetting period,
    //  the variable PocDecrementedInDPBFlag[ i ] is set equal to 0 for each value of i in the range of 0 to 62, inclusive.
    for (Int i = 0; i <= 62; i++)
    {
      m_pocDecrementedInDpbFlag[ i ] = 0;
    }
  }

  //  The variable pocResettingFlag is derived as follows:
  Bool pocResettingFlag;
  if (m_pcPic->getIsPocResettingPic() )
  {
    //-  If the current picture is a POC resetting picture, the following applies:
    if( vps->getVpsPocLsbAlignedFlag()  )
    {
      //  -  If vps_poc_lsb_aligned_flag is equal to 0, pocResettingFlag is set equal to 1.
      pocResettingFlag = true;
    }
    else if ( m_pocDecrementedInDpbFlag[ nuhLayerId ] )
    {
      //  -  Otherwise, if PocDecrementedInDPBFlag[ nuh_layer_id ] is equal to 1, pocResettingFlag is set equal to 0.
      pocResettingFlag = false;
    }
    else
    {
      //  -  Otherwise, pocResettingFlag is set equal to 1.
      pocResettingFlag = true;
    }
  }
  else
  {
    //  -  Otherwise, pocResettingFlag is set equal to 0.
    pocResettingFlag = false;
  }

  //  The list affectedLayerList is derived as follows:
  std::vector<Int> affectedLayerList;
  if (! vps->getVpsPocLsbAlignedFlag() )
  {
    //-  If vps_poc_lsb_aligned_flag is equal to 0, affectedLayerList consists of the nuh_layer_id of the current picture.
    affectedLayerList.push_back( nuhLayerId );
  }
  else
  {
    //  -  Otherwise, affectedLayerList consists of the nuh_layer_id of the current picture and the nuh_layer_id values
    //     equal to IdPredictedLayer[ currNuhLayerId ][ j ] for all values of j in the range of 0 to NumPredictedLayers[ currNuhLayerId ] - 1,
    //     inclusive, where currNuhLayerId is the nuh_layer_id value of the current picture.
    affectedLayerList.push_back( nuhLayerId );
    Int currNuhLayerId = nuhLayerId;
    for (Int j = 0; j <= vps->getNumPredictedLayers( currNuhLayerId )-1; j++ )
    {
      affectedLayerList.push_back( vps->getIdPredictedLayer(currNuhLayerId, j ) );
    }
  }

  Int picOrderCntMsb;
  Int picOrderCntVal;

  //  Depending on pocResettingFlag, the following applies:
  if ( pocResettingFlag )
  {
    //-  If pocResettingFlag is equal to 1, the following applies:
    if ( m_firstPicInLayerDecodedFlag[ nuhLayerId ] )
    {
      //-  The variables pocMsbDelta, pocLsbDelta and DeltaPocVal are derived as follows:
      Int pocMsbDelta;
      Int pocLsbDelta;
      Int deltaPocVal;

      {
        Int pocLsbVal;
        Int prevPicOrderCntLsb;
        Int prevPicOrderCntMsb;

        if( slice->getPocResetIdc() ==  3 )
        {
          pocLsbVal = slice->getPocLsbVal();
        }
        else
        {
          pocLsbVal = slice->getSlicePicOrderCntLsb();
        }

        if( slice->getPocMsbCycleValPresentFlag() )
        {
          pocMsbDelta = slice->getPocMsbCycleVal() * sps->getMaxPicOrderCntLsb();   // (F 60)
        }
        else
        {
          prevPicOrderCntLsb = m_prevPicOrderCnt & ( sps->getMaxPicOrderCntLsb() - 1 );
          prevPicOrderCntMsb = m_prevPicOrderCnt - prevPicOrderCntLsb;

          pocMsbDelta = xGetCurrMsb( pocLsbVal, prevPicOrderCntLsb, prevPicOrderCntMsb, sps->getMaxPicOrderCntLsb() );
        }

        if( slice->getPocResetIdc() == 2 ||  ( slice->getPocResetIdc() == 3  &&  slice->getFullPocResetFlag() ) )
        {
          pocLsbDelta = pocLsbVal;
        }
        else
        {
          pocLsbDelta = 0;
        }
        deltaPocVal = pocMsbDelta + pocLsbDelta;
      }

      //-  The PicOrderCntVal of each picture that has nuh_layer_id value nuhLayerId for which PocDecrementedInDPBFlag[ nuhLayerId ] is equal to 0
      //   and that is equal to any value in affectedLayerList is decremented by DeltaPocVal.
      for (Int i = 0; i < (Int) affectedLayerList.size(); i++ )
      {
        if ( !m_pocDecrementedInDpbFlag[ affectedLayerList[i] ] )
        {
          m_dpb->decrementPocsInSubDpb( affectedLayerList[i], deltaPocVal );
        }
      }

      //-  PocDecrementedInDPBFlag[ nuhLayerId ] is set equal to 1 for each value of nuhLayerId included in affectedLayerList.
      for (Int i = 0; i < (Int) affectedLayerList.size(); i++ )
      {
        m_pocDecrementedInDpbFlag[ affectedLayerList[i] ] = true;
      }
    }

    //-  The PicOrderCntVal of the current picture is derived as follows:
    if( slice->getPocResetIdc()  ==  1 )
    {
      picOrderCntVal = slice->getSlicePicOrderCntLsb();
    }
    else if (slice->getPocResetIdc()  ==  2 )
    {
      picOrderCntVal = 0;
    }
    else
    {
       picOrderCntMsb = xGetCurrMsb( slice->getSlicePicOrderCntLsb(), slice->getFullPocResetFlag() ? 0 : slice->getPocLsbVal(), 0, sps->getMaxPicOrderCntLsb() );
       picOrderCntVal = picOrderCntMsb + slice->getSlicePicOrderCntLsb();
    }
  }
  else
  {
    //-  Otherwise (pocResettingFlag is equal to 0), the following applies:
    //-  The PicOrderCntVal of the current picture is derived as follows:

    if( slice->getPocMsbCycleValPresentFlag() )
    {
      picOrderCntMsb = slice->getPocMsbCycleVal() * sps->getMaxPicOrderCntLsb();
    }
    else if( !m_firstPicInLayerDecodedFlag[ nuhLayerId ]  ||
    slice->getNalUnitType()  ==  NAL_UNIT_CODED_SLICE_IDR_N_LP || slice->getNalUnitType() ==  NAL_UNIT_CODED_SLICE_IDR_W_RADL )
    {
      picOrderCntMsb = 0; //     (F 62)
    }
    else
    {
        Int prevPicOrderCntLsb = m_prevPicOrderCnt & ( sps->getMaxPicOrderCntLsb() - 1 );
        Int prevPicOrderCntMsb = m_prevPicOrderCnt - prevPicOrderCntLsb;
        picOrderCntMsb = xGetCurrMsb( slice->getSlicePicOrderCntLsb(), prevPicOrderCntLsb, prevPicOrderCntMsb, sps->getMaxPicOrderCntLsb() );
    }
    picOrderCntVal = picOrderCntMsb + slice->getSlicePicOrderCntLsb();
  }

  m_pcPic->getSlice(0)->setPOC( picOrderCntVal );

  for (Int lId = 0; lId < (Int) affectedLayerList.size(); lId++ )
  {
    //  The value of PrevPicOrderCnt[ lId ] for each of the lId values included in affectedLayerList is derived as follows:

    if (!m_pcPic->isRasl() && !m_pcPic->isRadl() && !m_pcPic->isSlnr() && slice->getTemporalId() == 0 && !slice->getDiscardableFlag() )
    {
      //-  If the current picture is not a RASL picture, a RADL picture or a sub-layer non-reference picture, and the current picture
      //   has TemporalId equal to 0 and discardable_flag equal to 0, PrevPicOrderCnt[ lId ] is set equal to PicOrderCntVal.
      m_prevPicOrderCnt = picOrderCntVal;
    }
    else if ( slice->getPocResetIdc() == 3 &&  (
      ( !m_firstPicInLayerDecodedFlag[ nuhLayerId ]) ||
      ( m_firstPicInLayerDecodedFlag[ nuhLayerId ] && m_pcPic->getIsPocResettingPic() )
      ) )
    {
      //  -  Otherwise, when poc_reset_idc is equal to 3 and one of the following conditions is true, PrevPicOrderCnt[ lId ] is set equal to ( full_poc_reset_flag ? 0 : poc_lsb_val ):
      //     -  FirstPicInLayerDecodedFlag[ nuh_layer_id ] is equal to 0.
      //     -  FirstPicInLayerDecodedFlag[ nuh_layer_id ] is equal to 1 and the current picture is a POC resetting picture.
      m_prevPicOrderCnt = ( slice->getFullPocResetFlag() ? 0 : slice->getPocLsbVal() );
    }
  }
}

Int TDecTop::xGetCurrMsb( Int cl, Int pl, Int pm, Int ml )
{
  Int currMsb;
  if ((pl - cl) >= (ml/ 2))
  {
    currMsb = pm + ml;
  }
  else if ( (cl - pl) > (ml / 2))
  {
    currMsb = pm - ml;
  }
  else
  {
    currMsb = pm;
  }

  return currMsb;
}



Void TDecTop::x832DecProcForRefPicSet(  Bool annexFModifications )
{
  ///////////////////////////////////////////////////////////////////////////////////////
  // 8.3.2 8.3.2 Decoding process for reference picture set
  ///////////////////////////////////////////////////////////////////////////////////////

  TComSlice* slice = m_pcPic->getSlice( 0 );
  const TComSPS* sps = slice->getSPS();
  //  This process is invoked once per picture, after decoding of a slice header but prior to the decoding of any coding unit and prior
  //  to the decoding process for reference picture list construction for the slice as specified in clause 8.3.3.
  //  This process may result in one or more reference pictures in the DPB being marked as "unused for reference" or
  //  "used for long-term reference".

  // The variable currPicLayerId is set equal to nuh_layer_id of the current picture.
  Int currPicLayerId = m_pcPic->getLayerId();
  Int picOrderCntVal = m_pcPic->getPOC();

  if (m_pcPic->isIrap() && m_pcPic->getNoRaslOutputFlag()  )
  {
    // When the current picture is an IRAP picture with NoRaslOutputFlag equal to 1,
    // all reference pictures with nuh_layer_id equal to currPicLayerId currently in the
    // DPB (if any) are marked as "unused for reference".
    m_dpb->markSubDpbAsUnusedForReference( currPicLayerId );
  }
  // Short-term reference pictures are identified by their PicOrderCntVal values. Long-term reference pictures are identified either by
  // their PicOrderCntVal values or their slice_pic_order_cnt_lsb values.

  // Five lists of picture order count values are constructed to derive the RPS. These five lists are PocStCurrBefore,
  // PocStCurrAfter, PocStFoll, PocLtCurr and PocLtFoll, with NumPocStCurrBefore, NumPocStCurrAfter, NumPocStFoll,
  // NumPocLtCurr and NumPocLtFoll number of elements, respectively. The five lists and the five variables are derived as follows:

  TComDecodedRps* decRps = m_pcPic->getDecodedRps();

  std::vector<Int>& pocStCurrBefore = decRps->m_pocStCurrBefore;
  std::vector<Int>& pocStCurrAfter  = decRps->m_pocStCurrAfter;
  std::vector<Int>& pocStFoll       = decRps->m_pocStFoll;
  std::vector<Int>& pocLtCurr       = decRps->m_pocLtCurr;
  std::vector<Int>& pocLtFoll       = decRps->m_pocLtFoll;

  Int& numPocStCurrBefore = decRps->m_numPocStCurrBefore;
  Int& numPocStCurrAfter  = decRps->m_numPocStCurrAfter;
  Int& numPocStFoll       = decRps->m_numPocStFoll;
  Int& numPocLtCurr       = decRps->m_numPocLtCurr;
  Int& numPocLtFoll       = decRps->m_numPocLtFoll;

  std::vector<Int> currDeltaPocMsbPresentFlag, follDeltaPocMsbPresentFlag;

  if (m_pcPic->isIdr() )
  {
    // - If the current picture is an IDR picture, PocStCurrBefore, PocStCurrAfter, PocStFoll,
    //   PocLtCurr and PocLtFoll are all set to be empty, and NumPocStCurrBefore,
    //   NumPocStCurrAfter, NumPocStFoll, NumPocLtCurr and NumPocLtFoll are all set equal to 0.

    pocStCurrBefore.clear();
    pocStCurrAfter .clear();
    pocStFoll      .clear();
    pocLtCurr      .clear();
    pocLtFoll      .clear();
    numPocStCurrBefore = 0;
    numPocStCurrAfter  = 0;
    numPocStFoll       = 0;
    numPocLtCurr       = 0;
    numPocLtFoll       = 0;
  }
  else
  {
    const TComStRefPicSet* stRps  = slice->getStRps( slice->getCurrRpsIdx() );
    // -  Otherwise, the following applies:

    Int j = 0;
    Int k = 0;
    for( Int i = 0; i < stRps->getNumNegativePicsVar() ; i++ )
    {
      if( stRps->getUsedByCurrPicS0Var( i  ) )
      {
        pocStCurrBefore.push_back( picOrderCntVal + stRps->getDeltaPocS0Var( i ) ); j++;
      }
      else
      {
        pocStFoll      .push_back( picOrderCntVal + stRps->getDeltaPocS0Var( i ) ); k++;
      }
    }
    numPocStCurrBefore = j;

    j = 0;
    for (Int i = 0; i < stRps->getNumPositivePicsVar(); i++ )
    {
      if (stRps->getUsedByCurrPicS1Var( i ) )
      {
        pocStCurrAfter.push_back( picOrderCntVal + stRps->getDeltaPocS1Var( i ) ); j++;
      }
      else
      {
        pocStFoll     .push_back( picOrderCntVal + stRps->getDeltaPocS1Var( i ) ); k++;
      }
    }
    numPocStCurrAfter = j;
    numPocStFoll = k; //    (8 5)


    j = 0;
    k = 0;
    for( Int i = 0; i < slice->getNumLongTermSps( ) + slice->getNumLongTermPics(); i++ )
    {
      Int pocLt = slice->getPocLsbLtVar( i );
      if( slice->getDeltaPocMsbPresentFlag( i ) )
      {
        pocLt  +=  picOrderCntVal - slice->getDeltaPocMsbCycleLtVar( i ) * sps->getMaxPicOrderCntLsb() -
          ( picOrderCntVal & ( sps->getMaxPicOrderCntLsb() - 1 ) );
      }

      if( slice->getUsedByCurrPicLtVar(i))
      {
        pocLtCurr.push_back( pocLt );
        currDeltaPocMsbPresentFlag.push_back( slice->getDeltaPocMsbPresentFlag( i ) ); j++;
      }
      else
      {
        pocLtFoll.push_back( pocLt );
        follDeltaPocMsbPresentFlag.push_back( slice->getDeltaPocMsbPresentFlag( i ) ); k++;
      }
    }
    numPocLtCurr = j;
    numPocLtFoll = k;
  }

  assert(numPocStCurrAfter  == pocStCurrAfter   .size() );
  assert(numPocStCurrBefore == pocStCurrBefore  .size() );
  assert(numPocStFoll       == pocStFoll        .size() );
  assert(numPocLtCurr       == pocLtCurr        .size() );
  assert(numPocLtFoll       == pocLtFoll        .size() );

  // where PicOrderCntVal is the picture order count of the current picture as specified in clause 8.3.1.

  //   NOTE 2 - A value of CurrRpsIdx in the range of 0 to num_short_term_ref_pic_sets - 1, inclusive,
  //   indicates that a candidate short-term RPS from the active SPS for the current layer is being used,
  //   where CurrRpsIdx is the index of the candidate short-term RPS into the list of candidate short-term RPSs signalled
  //   in the active SPS for the current layer. CurrRpsIdx equal to num_short_term_ref_pic_sets indicates that
  //   the short-term RPS of the current picture is directly signalled in the slice header.

  for (Int i = 0; i <= numPocLtCurr - 1; i++  )
  {
      // For each i in the range of 0 to NumPocLtCurr - 1, inclusive, when CurrDeltaPocMsbPresentFlag[ i ] is equal to 1,
      // it is a requirement of bitstream conformance that the following conditions apply:
    if ( currDeltaPocMsbPresentFlag[i] )
    {
      // -  There shall be no j in the range of 0 to NumPocStCurrBefore - 1, inclusive,
      //    for which PocLtCurr[ i ] is equal to PocStCurrBefore[ j ].
      for (Int j = 0; j <= numPocStCurrBefore - 1; j++ )
      {
        assert(!( pocLtCurr[ i ] == pocStCurrBefore[ j ] ) );
      }

      // -  There shall be no j in the range of 0 to NumPocStCurrAfter - 1, inclusive,
      //    for which PocLtCurr[ i ] is equal to PocStCurrAfter[ j ].
      for (Int j = 0; j <= numPocStCurrAfter - 1; j++ )
      {
        assert(!( pocLtCurr[ i ] == pocStCurrAfter[ j ] ) );
      }

      // -  There shall be no j in the range of 0 to NumPocStFoll - 1, inclusive,
      //    for which PocLtCurr[ i ] is equal to PocStFoll[ j ].
      for (Int j = 0; j <= numPocStFoll - 1; j++ )
      {
        assert(!( pocLtCurr[ i ] == pocStFoll[ j ] ) );
      }

      // -  There shall be no j in the range of 0 to NumPocLtCurr - 1, inclusive,
      //    where j is not equal to i, for which PocLtCurr[ i ] is equal to PocLtCurr[ j ].
      for (Int j = 0; j <= numPocLtCurr - 1; j++ )
      {
        if ( i != j )
        {
          assert(!( pocLtCurr[ i ] == pocLtCurr[ j ] ) );
        }
      }
    }
  }

  for (Int i = 0; i <= numPocLtFoll - 1; i++  )
  {
    // For each i in the range of 0 to NumPocLtFoll - 1, inclusive, when FollDeltaPocMsbPresentFlag[ i ] is equal to 1,
    // it is a requirement of bitstream conformance that the following conditions apply:
    if ( follDeltaPocMsbPresentFlag[i] )
    {
      // -  There shall be no j in the range of 0 to NumPocStCurrBefore - 1, inclusive,
      //    for which PocLtFoll[ i ] is equal to PocStCurrBefore[ j ].
      for (Int j = 0; j <= numPocStCurrBefore - 1; j++ )
      {
        assert(!( pocLtFoll[ i ] == pocStCurrBefore[ j ] ) );
      }

      // -  There shall be no j in the range of 0 to NumPocStCurrAfter - 1, inclusive,
      //    for which PocLtFoll[ i ] is equal to PocStCurrAfter[ j ].
      for (Int j = 0; j <= numPocStCurrAfter - 1; j++ )
      {
        assert(!( pocLtFoll[ i ] == pocStCurrAfter[ j ] ) );
      }

      // -  There shall be no j in the range of 0 to NumPocStFoll - 1, inclusive,
      //    for which PocLtFoll[ i ] is equal to PocStFoll[ j ].
      for (Int j = 0; j <= numPocStFoll - 1; j++ )
      {
        assert(!( pocLtFoll[ i ] == pocStFoll[ j ] ) );
      }

      // -  There shall be no j in the range of 0 to NumPocLtFoll - 1, inclusive,
      //    where j is not equal to i, for which PocLtFoll[ i ] is equal to PocLtFoll[ j ].
      for (Int j = 0; j <= numPocLtFoll - 1; j++ )
      {
        if (j != i)
        {
          assert(!( pocLtFoll[ i ] == pocLtFoll[ j ] ) );
        }
      }

      // -  There shall be no j in the range of 0 to NumPocLtCurr - 1, inclusive,
      //    for which PocLtFoll[ i ] is equal to PocLtCurr[ j ].
      for (Int j = 0; j <= numPocLtCurr - 1; j++ )
      {
        assert(!( pocLtFoll[ i ] == pocLtCurr[ j ] ) );
      }
    }
  }

  Int maxPicOrderCntLsb = sps->getMaxPicOrderCntLsb();
  for (Int i = 0; i <= numPocLtCurr - 1; i++  )
  {
    // For each i in the range of 0 to NumPocLtCurr - 1, inclusive, when CurrDeltaPocMsbPresentFlag[ i ] is equal to 0,
    // it is a requirement of bitstream conformance that the following conditions apply:
    if ( currDeltaPocMsbPresentFlag[ i ] == 0  )
    {
      // -  There shall be no j in the range of 0 to NumPocStCurrBefore - 1, inclusive,
      //    for which PocLtCurr[ i ] is equal to ( PocStCurrBefore[ j ] & ( MaxPicOrderCntLsb - 1 ) ).
      for (Int j = 0; j <= numPocStCurrBefore - 1; j++ )
      {
        assert(!( pocLtCurr[ i ] == ( pocStCurrBefore[ j ] & ( maxPicOrderCntLsb - 1 ) ) ) );
      }

      // -  There shall be no j in the range of 0 to NumPocStCurrAfter - 1, inclusive,
      //    for which PocLtCurr[ i ] is equal to ( PocStCurrAfter[ j ] & ( MaxPicOrderCntLsb - 1 ) ).
      for (Int j = 0; j <= numPocStCurrAfter - 1; j++ )
      {
        assert(!( pocLtCurr[ i ] == ( pocStCurrAfter[ j ] & ( maxPicOrderCntLsb - 1 ) ) ) );
      }

      // -  There shall be no j in the range of 0 to NumPocStFoll - 1, inclusive,
      //    for which PocLtCurr[ i ] is equal to ( PocStFoll[ j ] & ( MaxPicOrderCntLsb - 1 ) ).
      for (Int j = 0; j <= numPocStFoll - 1; j++ )
      {
        assert(!( pocLtCurr[ i ] == ( pocStFoll[ j ] & ( maxPicOrderCntLsb - 1 ) ) ) );
      }

      // -  There shall be no j in the range of 0 to NumPocLtCurr - 1, inclusive,
      //    where j is not equal to i, for which PocLtCurr[ i ] is equal to ( PocLtCurr[ j ] & ( MaxPicOrderCntLsb - 1 ) ).
      for (Int j = 0; j <= numPocLtCurr - 1; j++ )
      {
        if (j != i)
        {
          assert(!( pocLtCurr[ i ] == ( pocLtCurr[ j ] & ( maxPicOrderCntLsb - 1 ) ) ) );
        }
      }
    }
  }

  for (Int i = 0; i <= numPocLtFoll - 1; i++  )
  {
    // For each i in the range of 0 to NumPocLtFoll - 1, inclusive, when FollDeltaPocMsbPresentFlag[ i ] is equal to 0,
    // it is a requirement of bitstream conformance that the following conditions apply:
    if ( follDeltaPocMsbPresentFlag[ i ] == 0  )
    {
      // -  There shall be no j in the range of 0 to NumPocStCurrBefore - 1, inclusive,
      //    for which PocLtFoll[ i ] is equal to ( PocStCurrBefore[ j ] & ( MaxPicOrderCntLsb - 1 ) ).
      for (Int j = 0; j <= numPocStCurrBefore - 1; j++ )
      {
        assert(!( pocLtFoll[ i ] == ( pocStCurrBefore[ j ] & ( maxPicOrderCntLsb - 1 ) ) ) );
      }

      // -  There shall be no j in the range of 0 to NumPocStCurrAfter - 1, inclusive,
      //    for which PocLtFoll[ i ] is equal to ( PocStCurrAfter[ j ] & ( MaxPicOrderCntLsb - 1 ) ).
      for (Int j = 0; j <= numPocStCurrAfter - 1; j++ )
      {
        assert(!( pocLtFoll[ i ] == ( pocStCurrAfter[ j ] & ( maxPicOrderCntLsb - 1 ) ) ) );
      }

      // -  There shall be no j in the range of 0 to NumPocStFoll - 1, inclusive,
      //    for which PocLtFoll[ i ] is equal to ( PocStFoll[ j ] & ( MaxPicOrderCntLsb - 1 ) ).
      for (Int j = 0; j <= numPocStFoll - 1; j++ )
      {
        assert(!( pocLtFoll[ i ] == ( pocStFoll[ j ] & ( maxPicOrderCntLsb - 1 ) ) ) );
      }

      // -  There shall be no j in the range of 0 to NumPocLtFoll - 1, inclusive,
      //    where j is not equal to i, for which PocLtFoll[ i ] is equal to ( PocLtFoll[ j ] & ( MaxPicOrderCntLsb - 1 ) ).
      for (Int j = 0; j <= numPocLtFoll - 1; j++ )
      {
        if (j != i)
        {
          assert(!( pocLtFoll[ i ] == ( pocLtFoll[ j ] & ( maxPicOrderCntLsb - 1 ) ) ) );
        }
      }

      // -  There shall be no j in the range of 0 to NumPocLtCurr - 1, inclusive,
      //    for which PocLtFoll[ i ] is equal to ( PocLtCurr[ j ] & ( MaxPicOrderCntLsb - 1 ) ).
      for (Int j = 0; j <= numPocLtCurr - 1; j++ )
      {
        assert(!( pocLtFoll[ i ] == ( pocLtCurr[ j ] & ( maxPicOrderCntLsb - 1 ) ) ) );
      }
    }
  }

  if ( !annexFModifications )
  {
    // The variable NumPicTotalCurr is derived as specified in clause 7.4.7.2.

    // It is a requirement of bitstream conformance that the following applies to the value of NumPicTotalCurr:
    if ( m_pcPic->isBla() || m_pcPic->isCra() )
    {
      // -  If the current picture is a BLA or CRA picture, the value of NumPicTotalCurr shall be equal to 0.
      assert( slice->getNumPicTotalCurr() == 0 );
    }
    else
    {
      // -  Otherwise,
      if ( slice->isInterP() || slice->isInterB() )
      {
        // when the current picture contains a P or B slice, the value of NumPicTotalCurr shall not be equal to 0.
        assert( slice->getNumPicTotalCurr() != 0 );
      }
    }
  }

  // The RPS of the current picture consists of five RPS lists; RefPicSetStCurrBefore, RefPicSetStCurrAfter, RefPicSetStFoll,
  // RefPicSetLtCurr and RefPicSetLtFoll. RefPicSetStCurrBefore, RefPicSetStCurrAfter and RefPicSetStFoll are collectively
  // referred to as the short-term RPS. RefPicSetLtCurr and RefPicSetLtFoll are collectively referred to as the long-term RPS.

  std::vector<TComPic*>& refPicSetStCurrBefore = decRps->m_refPicSetStCurrBefore;
  std::vector<TComPic*>& refPicSetStCurrAfter  = decRps->m_refPicSetStCurrAfter ;
  std::vector<TComPic*>& refPicSetStFoll       = decRps->m_refPicSetStFoll      ;
  std::vector<TComPic*>& refPicSetLtCurr       = decRps->m_refPicSetLtCurr      ;
  std::vector<TComPic*>& refPicSetLtFoll       = decRps->m_refPicSetLtFoll      ;

  std::vector<TComPic*>** refPicSetsCurr       = decRps->m_refPicSetsCurr       ;
  std::vector<TComPic*>** refPicSetsLt         = decRps->m_refPicSetsLt         ;
  std::vector<TComPic*>** refPicSetsAll        = decRps->m_refPicSetsAll        ;
  //   NOTE 3 - RefPicSetStCurrBefore, RefPicSetStCurrAfter and RefPicSetLtCurr contain all reference pictures that may be
  //   used for inter prediction of the current picture and one or more pictures that follow the current picture in decoding order.
  //   RefPicSetStFoll and RefPicSetLtFoll consist of all reference pictures that are not used for inter prediction of the current
  //   picture but may be used in inter prediction for one or more pictures that follow the current picture in decoding order.

  // The derivation process for the RPS and picture marking are performed according to the following ordered steps:
  // 1.  The following applies:

  TComSubDpb* dpb = m_dpb->getSubDpb( getLayerId(), false );
  assert( refPicSetLtCurr.empty() );
  for( Int i = 0; i < numPocLtCurr; i++ )
  {
    if( !currDeltaPocMsbPresentFlag[ i ] )
    {
      refPicSetLtCurr.push_back( dpb->getPicFromLsb( pocLtCurr[ i ], maxPicOrderCntLsb ) );
    }
    else
    {
      refPicSetLtCurr.push_back(dpb->getPic( pocLtCurr[ i ] ));
    }
  }

  assert( refPicSetLtFoll.empty() );
  for( Int i = 0; i < numPocLtFoll; i++ )
  {
   if( !follDeltaPocMsbPresentFlag[ i ] )
   {
     refPicSetLtFoll.push_back(dpb->getPicFromLsb(pocLtFoll[ i ], maxPicOrderCntLsb ));
   }
   else
   {
     refPicSetLtFoll.push_back(dpb->getPic( pocLtFoll[ i ] ));
   }
  }

  // 2.  All reference pictures that are included in RefPicSetLtCurr or RefPicSetLtFoll and have nuh_layer_id equal
  //     to currPicLayerId are marked as "used for long-term reference".
  for (Int i = 0; i < numPocLtCurr; i++)
  {
    if ( refPicSetLtCurr[i] != NULL )
    {
      refPicSetLtCurr[i]->markAsUsedForLongTermReference();
    }
  }

  for (Int i = 0; i < numPocLtFoll; i++)
  {
    if ( refPicSetLtFoll[i] != NULL )
    {
      refPicSetLtFoll[i]->markAsUsedForLongTermReference();
    }
  }

  // 3.  The following applies:
  assert( refPicSetStCurrBefore.empty() );
  for( Int i = 0; i < numPocStCurrBefore; i++ )
  {
    refPicSetStCurrBefore.push_back(dpb->getShortTermRefPic( pocStCurrBefore[ i ] ));
  }

  assert( refPicSetStCurrAfter.empty() );
  for( Int i = 0; i < numPocStCurrAfter; i++ )
  {
    refPicSetStCurrAfter.push_back(dpb->getShortTermRefPic( pocStCurrAfter[ i ] ));
  }

  assert( refPicSetStFoll.empty() );
  for( Int i = 0; i < numPocStFoll; i++ )
  {
    refPicSetStFoll.push_back(dpb->getShortTermRefPic( pocStFoll[ i ] ));
  }

  // 4.  All reference pictures in the DPB that are not included in RefPicSetLtCurr, RefPicSetLtFoll, RefPicSetStCurrBefore,
  //     RefPicSetStCurrAfter, or RefPicSetStFoll and have nuh_layer_id equal to currPicLayerId are marked as "unused for reference".
  TComSubDpb picsToMark = (*dpb);
  for (Int j = 0; j < 5; j++ )
  {
    picsToMark.removePics( *refPicSetsAll[j] );
  }
  picsToMark.markAllAsUnusedForReference();

  //     NOTE 4 - There may be one or more entries in the RPS lists that are equal to "no reference picture" because
  //     the corresponding pictures are not present in the DPB. Entries in RefPicSetStFoll or RefPicSetLtFoll that are equal
  //     to "no reference picture" should be ignored. An unintentional picture loss should be inferred for each entry in
  //     RefPicSetStCurrBefore, RefPicSetStCurrAfter, or RefPicSetLtCurr that is equal to "no reference picture".

  //     NOTE 5 - A picture cannot be included in more than one of the five RPS lists.


  // It is a requirement of bitstream conformance that the RPS is restricted as follows:


#if NH_MV_FIX_NO_REF_PICS_CHECK
  if ( !annexFModifications || m_firstPicInLayerDecodedFlag[ m_pcPic->getLayerId() ] )
  {
#endif
    for (Int j = 0; j < 3; j++ )
    {
      // -  There shall be no entry in RefPicSetStCurrBefore, RefPicSetStCurrAfter or RefPicSetLtCurr
      //    for which one or more of the following are true:

      std::vector<TComPic*>* currSet = refPicSetsCurr[j];
      for (Int i = 0; i < currSet->size(); i++)
      {
        TComPic* pic = (*currSet)[i];

        // -  The entry is equal to "no reference picture".
        assert( ! (pic == NULL ) );

        // -  The entry is an SLNR picture and has TemporalId equal to that of the current picture.
        assert( !( pic->isSlnr() && pic->getTemporalId() == m_pcPic->getTemporalId() ) );

        // -  The entry is a picture that has TemporalId greater than that of the current picture.
        assert( !(  pic->getTemporalId() > m_pcPic->getTemporalId() ) );
      }
    }
#if NH_MV_FIX_NO_REF_PICS_CHECK
  }
#endif

  //  -  There shall be no entry in RefPicSetLtCurr or RefPicSetLtFoll for which the
  //     difference between the picture order count value of the current picture and the picture order count
  //     value of the entry is greater than or equal to 2^24.
  for (Int j = 0; j < 2; j++ )
  {
    std::vector<TComPic*>* ltSet = refPicSetsLt[j];
    for (Int i = 0; i < ltSet->size(); i++)
    {
      TComPic* pic = (*ltSet)[i];
      if( pic != NULL )
      {
        assert(!( abs( m_pcPic->getPOC() - pic->getPOC() ) >= (1 << 24) ));
      }
    }
  }

  //   -  When the current picture is a temporal sub-layer access (TSA) picture, there shall be no picture
  //      included in the RPS with TemporalId greater than or equal to the TemporalId of the current picture.
  if (m_pcPic->isTsa() )
  {
    for (Int j = 0; j < 5; j++ )
    {
      std::vector<TComPic*>* aSet = refPicSetsAll[j];
      for (Int i = 0; i < aSet->size(); i++)
      {
        TComPic* pic = (*aSet)[i];
        if( pic != NULL )
        {
          assert( ! (pic->getTemporalId() >= m_pcPic->getTemporalId() ) );
        }
      }
    }
  }

  //   -  When the current picture is a step-wise temporal sub-layer access (STSA) picture,
  //      there shall be no picture included in RefPicSetStCurrBefore, RefPicSetStCurrAfter or RefPicSetLtCurr that has
  //      TemporalId equal to that of the current picture.
  if (m_pcPic->isStsa() )
  {
    for (Int j = 0; j < 3; j++ )
    {
      std::vector<TComPic*>* cSet = refPicSetsCurr[j];
      for (Int i = 0; i < cSet->size(); i++)
      {
        TComPic* pic = (*cSet)[i];
        if( pic != NULL )
        {
          assert( ! (pic->getTemporalId() == m_pcPic->getTemporalId() ) );
        }
      }
    }
  }

  //   -  When the current picture is a picture that follows, in decoding order, an STSA picture
  //      that has TemporalId equal to that of the current picture, there shall be no picture that has
  //      TemporalId equal to that of the current picture included in RefPicSetStCurrBefore, RefPicSetStCurrAfter
  //      or RefPicSetLtCurr that precedes the STSA picture in decoding order.
  if ( m_pcPic->getDecodingOrder() > m_prevStsaDecOrder && m_pcPic->getTemporalId() == m_prevStsaTemporalId  )
  {
    for (Int j = 0; j < 3; j++ )
    {
      std::vector<TComPic*>* cSet = refPicSetsCurr[j];
      for (Int i = 0; i < cSet->size(); i++)
      {
        TComPic* pic = (*cSet)[i];
        if( pic != NULL )
        {
          assert( ! (pic->getTemporalId() == m_pcPic->getTemporalId() && pic->getDecodingOrder() < m_prevStsaDecOrder  ) );
        }
      }
    }
  }

  //   -  When the current picture is a CRA picture, there shall be no picture included in the RPS that
  //      precedes, in output order or decoding order, any preceding IRAP picture in decoding order (when present).
  if ( m_pcPic->isCra() )
  {
    for (Int j = 0; j < 5; j++ )
    {
      std::vector<TComPic*>* aSet = refPicSetsAll[j];
      for (Int i = 0; i < aSet->size(); i++)
      {
        // TBD check whether it sufficient to test only the last IRAP
        TComPic* pic = (*aSet)[i];
        if( pic != NULL )
        {
          assert( ! (pic->getPOC()           < m_prevIrapPoc           ) );
          assert( ! (pic->getDecodingOrder() < m_prevIrapDecodingOrder ) );
        }
      }
    }
  }

  Bool isTrailingPicture = ( !m_pcPic->isIrap() ) && ( m_pcPic->getPOC() > m_prevIrapPoc );
  //   -  When the current picture is a trailing picture, there shall be no picture in RefPicSetStCurrBefore,
  //      RefPicSetStCurrAfter or RefPicSetLtCurr that was generated by the decoding process for generating unavailable
  //      reference pictures as specified in clause 8.3.3.
  if ( isTrailingPicture )
  {
    for (Int j = 0; j < 3; j++ )
    {
      std::vector<TComPic*>* cSet = refPicSetsCurr[j];
      for (Int i = 0; i < cSet->size(); i++)
      {
        TComPic* pic = (*cSet)[i];
        if( pic != NULL )
        {
          assert( ! (pic->getIsGeneratedCl833() ) );
        }
      }
    }
  }

  //   -  When the current picture is a trailing picture, there shall be no picture in the RPS that precedes the
  //      associated IRAP picture in output order or decoding order.
  if ( isTrailingPicture )
  {
    for (Int j = 0; j < 5; j++ )
    {
      std::vector<TComPic*>* aSet = refPicSetsAll[j];
      for (Int i = 0; i < aSet->size(); i++)
      {
        // TBD check whether it sufficient to test only the last IRAP
         TComPic* pic = (*aSet)[i];
        if( pic != NULL )
        {
          assert( ! (pic->getPOC()           < m_prevIrapPoc           ) );
          assert( ! (pic->getDecodingOrder() < m_prevIrapDecodingOrder ) );
        }
      }
    }
  }

  //   -  When the current picture is a RADL picture, there shall be no picture included in RefPicSetStCurrBefore,
  //      RefPicSetStCurrAfter or RefPicSetLtCurr that is any of the following:
  if ( m_pcPic->isRadl() )
  {
    for (Int j = 0; j < 3; j++ )
    {
      std::vector<TComPic*>* cSet = refPicSetsCurr[j];
      for (Int i = 0; i < cSet->size(); i++)
      {
        TComPic* pic = (*cSet)[i];
        if( pic != NULL )
        {
          // -  A RASL picture
          assert( ! (pic->isRasl() ) );
          // -  A picture that was generated by the decoding process for generating unavailable reference pictures
          //    as specified in clause 8.3.3
          assert( ! (pic->getIsGeneratedCl833() ) );
          // -  A picture that precedes the associated IRAP picture in decoding order
          assert( ! (pic->getDecodingOrder() < m_prevIrapDecodingOrder ) );
        }
      }
    }
  }


  if ( sps->getTemporalIdNestingFlag() )
  {
    // -  When sps_temporal_id_nesting_flag is equal to 1, the following applies:
    //    -  Let tIdA be the value of TemporalId of the current picture picA.
    TComPic* picA = m_pcPic;
    Int      tIdA = picA->getTemporalId();
    //   -  Any picture picB with TemporalId equal to tIdB that is less than or equal to tIdA shall not be included in
    //      RefPicSetStCurrBefore, RefPicSetStCurrAfter or RefPicSetLtCurr of picA when there exists a picture picC that
    //      has TemporalId less than tIdB, follows picB in decoding order, and precedes picA in decoding order.
    for (Int j = 0; j < 3; j++ )
    {
      std::vector<TComPic*>* cSet = refPicSetsCurr[j];
      for (Int i = 0; i < cSet->size(); i++)
      {
        TComPic* picB = (*cSet)[i];
        if( picB != NULL )
        {
          Int tIdB = picB->getTemporalId();

          if (tIdB <= tIdA)
          {
            for ( TComSubDpb::iterator itP = dpb->begin(); itP != dpb->end(); itP++ )
            {
              TComPic* picC = (*itP);
              assert(! ( picC->getTemporalId() < tIdB && picC->getDecodingOrder() > picB->getDecodingOrder() && picC->getDecodingOrder() < picA->getDecodingOrder()  )  );
            }
          }
        }
      }
    }
  }
}


Void TDecTop::xF832DecProcForRefPicSet()
{
  ///////////////////////////////////////////////////////////////////////////////////////
  // F.8.3.2 Decoding process for reference picture set
  ///////////////////////////////////////////////////////////////////////////////////////

  // The specifications in clause 8.3.2 apply with the following changes:
  // -  The references to clauses 7.4.7.2, 8.3.1, 8.3.3 and 8.3.4 are replaced with references to
  //    clauses F.7.4.7.2, F.8.3.1, F.8.3.3 and F.8.3.4, respectively.

  x832DecProcForRefPicSet( true );

  // -  The following specifications are added:
  if (m_pcPic->isIrap() && m_pcPic->getLayerId() == m_smallestLayerId )
  {
    // -  When the current picture is an IRAP picture with nuh_layer_id equal to SmallestLayerId,
    //    all reference pictures with any value of nuh_layer_id currently in the DPB (if any) are marked
    //    as "unused for reference" when at least one of the following conditions is true:

    if ( m_pcPic->getNoClrasOutputFlag() || m_pcPic->getActivatesNewVps() )
    {
      // -  The current picture has NoClrasOutputFlag is equal to 1.
      // -  The current picture activates a new VPS.
      m_dpb->markAllSubDpbAsUnusedForReference( );
    }
  }

  // -  It is a requirement of bitstream conformance that the RPS is restricted as follows:
  // -  When the current picture is a CRA picture, there shall be no picture in RefPicSetStCurrBefore, RefPicSetStCurrAfter
  //    or RefPicSetLtCurr.

  std::vector<TComPic*>** refPicSetsCurr       = m_pcPic->getDecodedRps()->m_refPicSetsCurr;

  if ( m_pcPic->isCra() )
  {
    for (Int j = 0; j < 3; j++ )
    {
      std::vector<TComPic*>* cSet = refPicSetsCurr[j];
      assert ( cSet->size() == 0 );
    }
  }

  // -  The constraints specified in clause 8.3.2 on the value of NumPicTotalCurr are replaced with the following:
  //    -  It is a requirement of bitstream conformance that the following applies to the value of NumPicTotalCurr:
  Int numPicTotalCurr = m_pcPic->getSlice(0)->getNumPicTotalCurr();
  Int currPicLayerId  = m_pcPic->getLayerId();
  const TComVPS* vps  = m_pcPic->getSlice(0)->getVPS();

  if ( ( m_pcPic->isBla() || m_pcPic->isCra() ) && (  (currPicLayerId == 0 ) || ( vps->getNumDirectRefLayers( currPicLayerId ) == 0 ) ) )
  {
    assert( numPicTotalCurr == 0 );
    // -  If the current picture is a BLA or CRA picture and either currPicLayerId is equal to 0 or
    //     NumDirectRefLayers[ currPicLayerId ] is equal to 0, the value of NumPicTotalCurr shall be equal to 0.
  }
  else
  {
    // TBD: check all slices
    if ( m_pcPic->getSlice(0)->getSliceType() == P_SLICE  ||  m_pcPic->getSlice(0)->getSliceType() == B_SLICE )
    {
      // -  Otherwise, when the current picture contains a P or B slice, the value of NumPicTotalCurr shall not be equal to 0.
      assert( numPicTotalCurr != 0 );
    }
  }
}


Void TDecTop::xG813DecProcForInterLayerRefPicSet()
{
  ////////////////////////////////////////////////////////////////////
  // G.8.1.3 Decoding process for inter-layer reference picture set //
  ////////////////////////////////////////////////////////////////////

  // Outputs of this process are updated lists of inter-layer reference pictures RefPicSetInterLayer0 and RefPicSetInterLayer1
  // and the variables NumActiveRefLayerPics0 and NumActiveRefLayerPics1.

  TComDecodedRps* decRps = m_pcPic->getDecodedRps();
  TComSlice* slice       = m_pcPic->getSlice( 0 );
  const TComVPS* vps     =  slice->getVPS();

  Int&                   numActiveRefLayerPics0 = decRps->m_numActiveRefLayerPics0;
  Int&                   numActiveRefLayerPics1 = decRps->m_numActiveRefLayerPics1;

  std::vector<TComPic*>& refPicSetInterLayer0   = decRps->m_refPicSetInterLayer0;
  std::vector<TComPic*>& refPicSetInterLayer1   = decRps->m_refPicSetInterLayer1;

  // The variable currLayerId is set equal to nuh_layer_id of the current picture.
  Int currLayerId = getLayerId();

  // The lists RefPicSetInterLayer0 and RefPicSetInterLayer1 are first emptied, NumActiveRefLayerPics0 and NumActiveRefLayerPics1
  // are set equal to 0 and the following applies:

  refPicSetInterLayer0.clear();
  refPicSetInterLayer1.clear();

  numActiveRefLayerPics0 = 0;
  numActiveRefLayerPics1 = 0;

  Int viewIdCurrLayerId  = vps->getViewId( currLayerId );
  Int viewId0            = vps->getViewId( 0   );

  for( Int i = 0; i < slice->getNumActiveRefLayerPics(); i++ )
  {
    Int viewIdRefPicLayerIdi = vps->getViewId( slice->getRefPicLayerId( i ) );

    Bool refPicSet0Flag =
      ( ( viewIdCurrLayerId <=  viewId0  &&  viewIdCurrLayerId <=  viewIdRefPicLayerIdi )  ||
      ( viewIdCurrLayerId >=  viewId0  &&  viewIdCurrLayerId >=  viewIdRefPicLayerIdi ) );

    TComPic* picX = m_dpb->getAu(slice->getPOC(), false )->getPic( slice->getRefPicLayerId( i ) );
    if ( picX != NULL )
    {
      // there is a picture picX in the DPB that is in the same access unit as the current picture and has
      // nuh_layer_id equal to RefPicLayerId[ i ]

      if ( refPicSet0Flag )
      {
        refPicSetInterLayer0.push_back( picX );
        refPicSetInterLayer0[ numActiveRefLayerPics0++ ]->markAsUsedForLongTermReference();
      }
      else
      {
        refPicSetInterLayer1.push_back( picX );
        refPicSetInterLayer1[ numActiveRefLayerPics1++ ]->markAsUsedForLongTermReference();
      }

      // There shall be no picture that has discardable_flag equal to 1 in RefPicSetInterLayer0 or RefPicSetInterLayer1.
      assert( ! picX->getSlice(0)->getDiscardableFlag() );

      // If the current picture is a RADL picture, there shall be no entry in RefPicSetInterLayer0 or RefPicSetInterLayer1
      // that is a RASL picture.
      if ( m_pcPic->isRadl() )
      {
        assert( ! picX->isRasl() );
      }
    }
    else
    {
      if( refPicSet0Flag )
      {
        refPicSetInterLayer0.push_back( NULL ); // "no reference picture" (G 1)
        numActiveRefLayerPics0++;
      }
      else
      {
        refPicSetInterLayer1.push_back( NULL ); // "no reference picture";
        numActiveRefLayerPics1++;
      }
      // There shall be no entry equal to "no reference picture" in RefPicSetInterLayer0 or RefPicSetInterLayer1.
      assert( false );
    }
  }
}


Void TDecTop::x8331GenDecProcForGenUnavilRefPics()
{
  ///////////////////////////////////////////////////////////////////////////////////////
  // 8.3.3.1  General decoding process for generating unavailable reference pictures ////
  ///////////////////////////////////////////////////////////////////////////////////////

  // This process is invoked once per coded picture when the current picture is a
  // BLA picture or is a CRA picture with NoRaslOutputFlag equal to 1.

  assert( m_pcPic->isBla() || (m_pcPic->isCra() && m_pcPic->getNoRaslOutputFlag() ) );
  TComDecodedRps* decRps = m_pcPic->getDecodedRps();

  std::vector<TComPic*>& refPicSetStFoll      = decRps->m_refPicSetStFoll;
  std::vector<TComPic*>& refPicSetLtFoll      = decRps->m_refPicSetLtFoll;

  const std::vector<Int>& pocStFoll             = decRps->m_pocStFoll;
  const std::vector<Int>& pocLtFoll             = decRps->m_pocLtFoll;

  const Int               numPocStFoll          = decRps->m_numPocStFoll;
  const Int               numPocLtFoll          = decRps->m_numPocLtFoll;

  // When this process is invoked, the following applies:
  for ( Int i = 0 ; i <= numPocStFoll - 1; i++ )
  {
    if ( refPicSetStFoll[ i ] == NULL )
    {
      //-  For each RefPicSetStFoll[ i ], with i in the range of 0 to NumPocStFoll - 1, inclusive, that is equal
      //   to "no reference picture", a picture is generated as specified in clause 8.3.3.2, and the following applies:
      TComPic* genPic = x8332GenOfOneUnavailPic( true );

      // -  The value of PicOrderCntVal for the generated picture is set equal to PocStFoll[ i ].
      genPic->getSlice(0)->setPOC( pocStFoll[ i ] );

      //-  The value of PicOutputFlag for the generated picture is set equal to 0.
      genPic->setPicOutputFlag( false );

      // -  The generated picture is marked as "used for short-term reference".
      genPic->markAsUsedForShortTermReference();

      // -  RefPicSetStFoll[ i ] is set to be the generated reference picture.
      refPicSetStFoll[ i ] = genPic;

      // -  The value of nuh_layer_id for the generated picture is set equal to nuh_layer_id of the current picture.
      genPic->setLayerId( m_pcPic-> getLayerId() );

      // Insert to DPB
      m_dpb->addNewPic( genPic );
    }
  }

  for ( Int i = 0 ; i <= numPocLtFoll - 1; i++ )
  {
    if ( refPicSetLtFoll[ i ] == NULL )
    {
      //-  For each RefPicSetLtFoll[ i ], with i in the range of 0 to NumPocLtFoll - 1, inclusive, that is equal to
      //   "no reference picture", a picture is generated as specified in clause 8.3.3.2, and the following applies:
      TComPic* genPic = x8332GenOfOneUnavailPic( true );

      //-  The value of PicOrderCntVal for the generated picture is set equal to PocLtFoll[ i ].
      genPic->getSlice(0)->setPOC( pocStFoll[ i ] );

      //  -  The value of slice_pic_order_cnt_lsb for the generated picture is inferred to be equal to ( PocLtFoll[ i ] & ( MaxPicOrderCntLsb - 1 ) ).
      genPic->getSlice(0)->setSlicePicOrderCntLsb( ( pocLtFoll[ i ] & ( m_pcPic->getSlice(0)->getSPS()->getMaxPicOrderCntLsb() - 1 ) ) );

      //  -  The value of PicOutputFlag for the generated picture is set equal to 0.
      genPic->setPicOutputFlag( false );

      //  -  The generated picture is marked as "used for long-term reference".
      genPic->markAsUsedForLongTermReference();

      //  -  RefPicSetLtFoll[ i ] is set to be the generated reference picture.
      refPicSetLtFoll[ i ] = genPic;

      //  -  The value of nuh_layer_id for the generated picture is set equal to nuh_layer_id of the current picture.
      genPic->setLayerId( m_pcPic-> getLayerId() );

      // Insert to DPB
      m_dpb->addNewPic( genPic );
    }
  }
}


TComPic* TDecTop::x8332GenOfOneUnavailPic( Bool calledFromCl8331 )
{
  ///////////////////////////////////////////////////////////////////////////////////////
  // 8.3.3.2 Generation of one unavailable picture
  ///////////////////////////////////////////////////////////////////////////////////////

  TComPic* genPic = new TComPic;
  genPic->create( *m_pcPic->getSlice(0)->getSPS(), *m_pcPic->getSlice(0)->getPPS(), true );
  genPic->setIsGenerated( true );
  genPic->setIsGeneratedCl833( calledFromCl8331 );
  return genPic;
}


Void TDecTop::xF817DecProcForGenUnavRefPicForPicsFrstInDecOrderInLay()
{
  ///////////////////////////////////////////////////////////////////////////////////////
  // F.8.1.7 Decoding process for generating unavailable reference pictures for pictures
  //         first in decoding order within a layer
  ///////////////////////////////////////////////////////////////////////////////////////

  //  This process is invoked for a picture with nuh_layer_id equal to layerId, when FirstPicInLayerDecodedFlag[layerId ] is equal to 0.
  assert( !m_firstPicInLayerDecodedFlag[ getLayerId() ] );


  TComDecodedRps* decRps = m_pcPic->getDecodedRps();

  std::vector<TComPic*>& refPicSetStCurrBefore = decRps->m_refPicSetStCurrBefore;
  std::vector<TComPic*>& refPicSetStCurrAfter  = decRps->m_refPicSetStCurrAfter;
  std::vector<TComPic*>& refPicSetStFoll       = decRps->m_refPicSetStFoll;
  std::vector<TComPic*>& refPicSetLtCurr       = decRps->m_refPicSetLtCurr;
  std::vector<TComPic*>& refPicSetLtFoll       = decRps->m_refPicSetLtFoll;


  const std::vector<Int>& pocStCurrBefore      = decRps->m_pocStCurrBefore;
  const std::vector<Int>& pocStCurrAfter       = decRps->m_pocStCurrAfter;
  const std::vector<Int>& pocStFoll            = decRps->m_pocStFoll;
  const std::vector<Int>& pocLtCurr            = decRps->m_pocLtCurr;
  const std::vector<Int>& pocLtFoll            = decRps->m_pocLtFoll;

  const Int numPocStCurrBefore                 = decRps->m_numPocStCurrBefore;
  const Int numPocStCurrAfter                  = decRps->m_numPocStCurrAfter;
  const Int numPocStFoll                       = decRps->m_numPocStFoll;
  const Int numPocLtCurr                       = decRps->m_numPocLtCurr;
  const Int numPocLtFoll                       = decRps->m_numPocLtFoll;

  Int nuhLayerId = m_pcPic-> getLayerId();
  for ( Int i = 0 ; i <= numPocStCurrBefore - 1; i++ )
  {
    if ( refPicSetStCurrBefore[ i ] == NULL )
    {
      //-  For each RefPicSetStCurrBefore[ i ], with i in the range of 0 to NumPocStCurrBefore - 1, inclusive, that is
      //  equal to "no reference picture", a picture is generated as specified in clause 8.3.3.2 and the following applies:
      TComPic* genPic = x8332GenOfOneUnavailPic( false );

      //-  The value of PicOrderCntVal for the generated picture is set equal to PocStCurrBefore[ i ].
      genPic->getSlice(0)->setPOC( pocStCurrBefore[ i ] );

      //  -  The value of PicOutputFlag for the generated picture is set equal to 0.
      genPic->setPicOutputFlag( false );

      //  -  The generated picture is marked as "used for short-term reference".
      genPic->markAsUsedForShortTermReference();

      //  -  RefPicSetStCurrBefore[ i ] is set to be the generated reference picture.
      refPicSetStCurrBefore[ i ] = genPic;

      //  -  The value of nuh_layer_id for the generated picture is set equal to nuh_layer_id.
      genPic->setLayerId( nuhLayerId );

      // Insert to DPB
      m_dpb->addNewPic( genPic );
    }
  }

  for ( Int i = 0 ; i <= numPocStCurrAfter - 1; i++ )
  {
    if ( refPicSetStCurrAfter[ i ] == NULL )
    {
      //  -  For each RefPicSetStCurrAfter[ i ], with i in the range of 0 to NumPocStCurrAfter - 1, inclusive, that is equal
      //     to "no reference picture", a picture is generated as specified in clause 8.3.3.2 and the following applies:
      TComPic* genPic = x8332GenOfOneUnavailPic( false );

      //  -  The value of PicOrderCntVal for the generated picture is set equal to PocStCurrAfter[ i ].
      genPic->getSlice(0)->setPOC( pocStCurrAfter[ i ] );

      //  -  The value of PicOutputFlag for the generated picture is set equal to 0.
      genPic->setPicOutputFlag( false );

      //  -  The generated picture is marked as "used for short-term reference".
      genPic->markAsUsedForShortTermReference();

      //  -  RefPicSetStCurrAfter[ i ] is set to be the generated reference picture.
      refPicSetStCurrAfter[ i ] = genPic;

      //  -  The value of nuh_layer_id for the generated picture is set equal to nuh_layer_id.
      genPic->setLayerId( nuhLayerId );

      // Insert to DPB
      m_dpb->addNewPic( genPic );

    }
  }

  for ( Int i = 0 ; i <= numPocStFoll - 1; i++ )
  {
    if ( refPicSetStFoll[ i ] == NULL )
    {
      //  -  For each RefPicSetStFoll[ i ], with i in the range of 0 to NumPocStFoll - 1, inclusive, that is equal to "no
      //     reference picture", a picture is generated as specified in clause 8.3.3.2 and the following applies:
      TComPic* genPic = x8332GenOfOneUnavailPic( false );

      //  -  The value of PicOrderCntVal for the generated picture is set equal to PocStFoll[ i ].
      genPic->getSlice(0)->setPOC( pocStFoll[ i ] );

      //  -  The value of PicOutputFlag for the generated picture is set equal to 0.
      genPic->setPicOutputFlag( false );

      //  -  The generated picture is marked as "used for short-term reference".
      genPic->markAsUsedForShortTermReference();

      //  -  RefPicSetStFoll[ i ] is set to be the generated reference picture.
      refPicSetStFoll[ i ] = genPic;

      //  -  The value of nuh_layer_id for the generated picture is set equal to nuh_layer_id.
      genPic->setLayerId( nuhLayerId );

      // Insert to DPB
      m_dpb->addNewPic( genPic );
    }
  }

  Int maxPicOrderCntLsb = m_pcPic->getSlice(0)->getSPS()->getMaxPicOrderCntLsb();
  for ( Int i = 0 ; i <= numPocLtCurr - 1; i++ )
  {
    if ( refPicSetLtCurr[ i ] == NULL )
    {
      //  -  For each RefPicSetLtCurr[ i ], with i in the range of 0 to NumPocLtCurr - 1, inclusive, that is equal to "no
      //     reference picture", a picture is generated as specified in clause 8.3.3.2 and the following applies:
      TComPic* genPic = x8332GenOfOneUnavailPic( false );

      //  -  The value of PicOrderCntVal for the generated picture is set equal to PocLtCurr[ i ].
      genPic->getSlice(0)->setPOC( pocLtCurr[ i ] );

      //  -  The value of slice_pic_order_cnt_lsb for the generated picture is inferred to be equal to ( PocLtCurr[ i ] & (
      //     MaxPicOrderCntLsb - 1 ) ).
      genPic->getSlice(0)->setSlicePicOrderCntLsb( ( pocLtCurr[ i ] & ( maxPicOrderCntLsb - 1 ) ) );

      //  -  The value of PicOutputFlag for the generated picture is set equal to 0.
      genPic->setPicOutputFlag( false );

      //  -  The generated picture is marked as "used for long-term reference".
      genPic->markAsUsedForLongTermReference();

      //  -  RefPicSetLtCurr[ i ] is set to be the generated reference picture.
      refPicSetLtCurr[ i ] = genPic;

      //  -  The value of nuh_layer_id for the generated picture is set equal to nuh_layer_id.
      genPic->setLayerId( nuhLayerId );

      // Insert to DPB
      m_dpb->addNewPic( genPic );
    }
  }

  for ( Int i = 0 ; i <= numPocLtFoll - 1; i++ )
  {
    if ( refPicSetLtFoll[ i ] == NULL )
    {
      //  -  For each RefPicSetLtFoll[ i ], with i in the range of 0 to NumPocLtFoll - 1, inclusive, that is equal to "no
      //     reference picture", a picture is generated as specified in clause 8.3.3.2 and the following applies:
      TComPic* genPic = x8332GenOfOneUnavailPic( false );

      //  -  The value of PicOrderCntVal for the generated picture is set equal to PocLtFoll[ i ].
      genPic->getSlice(0)->setPOC( pocLtFoll[ i ] );

      //  -  The value of slice_pic_order_cnt_lsb for the generated picture is inferred to be equal to ( PocLtCurr[ i ] & (
      //     MaxPicOrderCntLsb - 1 ) ).
      genPic->getSlice(0)->setSlicePicOrderCntLsb( ( pocLtCurr[ i ] & ( maxPicOrderCntLsb - 1 ) ) );

      //  -  The value of PicOutputFlag for the generated picture is set equal to 0.
      genPic->setPicOutputFlag( false );

      //  -  The generated picture is marked as "used for long-term reference".
      genPic->markAsUsedForLongTermReference();

      //  -  RefPicSetLtFoll[ i ] is set to be the generated reference picture.
      refPicSetLtFoll[ i ] = genPic;

      //  -  The value of nuh_layer_id for the generated picture is set equal to nuh_layer_id.
      genPic->setLayerId( nuhLayerId );

      // Insert to DPB
      m_dpb->addNewPic( genPic );
    }
  }
}

Void TDecTop::xF833DecProcForGenUnavRefPics()
{
  ///////////////////////////////////////////////////////////////////////////////////////
  // F.8.3.3 Decoding process for generating unavailable reference picture
  ///////////////////////////////////////////////////////////////////////////////////////

  x8331GenDecProcForGenUnavilRefPics();
}

Void TDecTop::xCheckUnavailableRefPics()
{
  std::vector<TComPic*>** refPicSetsCurr       = m_pcPic->getDecodedRps()->m_refPicSetsCurr;

  Bool hasGeneratedRefPic = false;
  for (Int j = 0; j < 3; j++ )
  {
    std::vector<TComPic*>* cSet = refPicSetsCurr[j];
    for (Int i = 0 ; i < cSet->size();  i++ )
    {
      assert( (*cSet)[i] != NULL );
      if ((*cSet)[i]->getIsGenerated() )
      {
        hasGeneratedRefPic = true;
      }
    }
  }
  m_pcPic->setHasGeneratedRefPics( hasGeneratedRefPic );
}

#endif

//! \}
