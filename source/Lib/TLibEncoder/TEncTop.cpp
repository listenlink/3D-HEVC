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



/** \file     TEncTop.cpp
    \brief    encoder class
*/

#include "../TLibCommon/CommonDef.h"
#include "TEncTop.h"

#include "TEncGOP.h"
#include "../../App/TAppEncoder/TAppEncTop.h"

// ====================================================================================================================
// Constructor / destructor / create / destroy
// ====================================================================================================================

TEncTop::TEncTop()
{
  m_iPOCLast          = -1;
  m_iNumPicRcvd       =  0;
  m_uiNumAllPicCoded  =  0;
  m_pppcRDSbacCoder   =  NULL;
  m_pppcBinCoderCABAC =  NULL;
  m_cRDGoOnSbacCoder.init( &m_cRDGoOnBinCoderCABAC );
#if ENC_DEC_TRACE
  g_hTrace = fopen( "TraceEnc.txt", "wb" );
  g_bJustDoIt = g_bEncDecTraceDisable;
  g_nSymbolCounter = 0;
#endif
  m_bSeqFirst = true;
  m_iFrameNumInCodingOrder = 0;
}

TEncTop::~TEncTop()
{
#if ENC_DEC_TRACE
  fclose( g_hTrace );
#endif
}

Void TEncTop::create ()
{
  // initialize global variables
  if(m_uiViewId<1 && !m_bIsDepth)
    initROM();

  // create processing unit classes
  m_cPicEncoder.        create( getSourceWidth(), getSourceHeight(), g_uiMaxCUWidth, g_uiMaxCUHeight );
  m_cSliceEncoder.      create( getSourceWidth(), getSourceHeight(), g_uiMaxCUWidth, g_uiMaxCUHeight, g_uiMaxCUDepth );
  m_cCuEncoder.         create( g_uiMaxCUDepth, g_uiMaxCUWidth, g_uiMaxCUHeight );
#if MTK_SAO
  if (m_bUseSAO)
  {
    m_cEncSAO.create( getSourceWidth(), getSourceHeight(), g_uiMaxCUWidth, g_uiMaxCUHeight, g_uiMaxCUDepth );
    m_cEncSAO.createEncBuffer();
  }
#endif
  m_cAdaptiveLoopFilter.create( getSourceWidth(), getSourceHeight(), g_uiMaxCUWidth, g_uiMaxCUHeight, g_uiMaxCUDepth );
  m_cLoopFilter.        create( g_uiMaxCUDepth );
#if DEPTH_MAP_GENERATION
  m_cDepthMapGenerator. create( false, getSourceWidth(), getSourceHeight(), g_uiMaxCUDepth, g_uiMaxCUWidth, g_uiMaxCUHeight, g_uiBitDepth + g_uiBitIncrement );
#endif
#if HHI_INTER_VIEW_RESIDUAL_PRED
  m_cResidualGenerator. create( false, getSourceWidth(), getSourceHeight(), g_uiMaxCUDepth, g_uiMaxCUWidth, g_uiMaxCUHeight, g_uiBitDepth + g_uiBitIncrement );
#endif

#if MQT_BA_RA && MQT_ALF_NPASS
  if(m_bUseALF)
  {
    m_cAdaptiveLoopFilter.createAlfGlobalBuffers(m_iALFEncodePassReduction);
  }
#endif

  // if SBAC-based RD optimization is used
  if( m_bUseSBACRD )
  {
    UInt uiNumCEnc = 1 + Max( g_uiMaxCUDepth, AO_MAX_DEPTH ); // HS: prevents crash in SAO for small maximum CU sizes
    m_pppcRDSbacCoder = new TEncSbac** [uiNumCEnc];
    m_pppcBinCoderCABAC = new TEncBinCABAC** [uiNumCEnc];

    for ( Int iDepth = 0; iDepth < uiNumCEnc; iDepth++ )
    {
      m_pppcRDSbacCoder[iDepth] = new TEncSbac* [CI_NUM];
      m_pppcBinCoderCABAC[iDepth] = new TEncBinCABAC* [CI_NUM];

      for (Int iCIIdx = 0; iCIIdx < CI_NUM; iCIIdx ++ )
      {
        m_pppcRDSbacCoder[iDepth][iCIIdx] = new TEncSbac;
        m_pppcBinCoderCABAC [iDepth][iCIIdx] = new TEncBinCABAC;
        m_pppcRDSbacCoder   [iDepth][iCIIdx]->init( m_pppcBinCoderCABAC [iDepth][iCIIdx] );
      }
    }
  }

#if HHI_DMM_WEDGE_INTRA || HHI_DMM_PRED_TEX
  if( g_aacWedgeLists.empty() && m_bUseDMM && m_bIsDepth )
  {
    initWedgeLists();
  }
#endif
}

Void TEncTop::destroy ()
{
#if MQT_BA_RA && MQT_ALF_NPASS
  if(m_bUseALF)
  {
    m_cAdaptiveLoopFilter.destroyAlfGlobalBuffers();
  }
#endif

  // destroy processing unit classes
  m_cPicEncoder.        destroy();
  m_cSliceEncoder.      destroy();
  m_cCuEncoder.         destroy();
#if MTK_SAO
  if (m_cSPS.getUseSAO())
  {
    m_cEncSAO.destroy();
    m_cEncSAO.destoryEncBuffer();
  }
#endif
  m_cAdaptiveLoopFilter.destroy();
  m_cLoopFilter.        destroy();
#if DEPTH_MAP_GENERATION
  m_cDepthMapGenerator. destroy();
#endif
#if HHI_INTER_VIEW_RESIDUAL_PRED
  m_cResidualGenerator. destroy();
#endif

  // SBAC RD
  if( m_bUseSBACRD )
  {
    Int iDepth;
    for ( iDepth = 0; iDepth < g_uiMaxCUDepth+1; iDepth++ )
    {
      for (Int iCIIdx = 0; iCIIdx < CI_NUM; iCIIdx ++ )
      {
        delete m_pppcRDSbacCoder[iDepth][iCIIdx];
        delete m_pppcBinCoderCABAC[iDepth][iCIIdx];
      }
    }

    for ( iDepth = 0; iDepth < g_uiMaxCUDepth+1; iDepth++ )
    {
      delete [] m_pppcRDSbacCoder[iDepth];
      delete [] m_pppcBinCoderCABAC[iDepth];
    }

    delete [] m_pppcRDSbacCoder;
    delete [] m_pppcBinCoderCABAC;
  }

  // destroy ROM
  if(m_uiViewId<1 && !m_bIsDepth)
    destroyROM();

  return;
}

Void TEncTop::init( TAppEncTop* pcTAppEncTop )
{
  UInt *aTable4=NULL, *aTable8=NULL;
#if QC_MOD_LCEC
  UInt* aTableLastPosVlcIndex=NULL;
#endif
  // initialize SPS
  xInitSPS();

#if CONSTRAINED_INTRA_PRED
  // initialize PPS
  xInitPPS();
#endif

  // initialize processing unit classes
  m_cPicEncoder.  init( this );
  m_cSliceEncoder.init( this );
  m_cCuEncoder.   init( this );

  m_pcTAppEncTop = pcTAppEncTop;
#if DEPTH_MAP_GENERATION
  m_cDepthMapGenerator.init( (TComPrediction*)this->getPredSearch(), m_pcTAppEncTop->getSPSAccess(), m_pcTAppEncTop->getAUPicAccess() );
#endif
#if HHI_INTER_VIEW_RESIDUAL_PRED
  m_cResidualGenerator.init( &m_cTrQuant, &m_cDepthMapGenerator );
#endif

  // initialize transform & quantization class
  m_pcCavlcCoder = getCavlcCoder();
#if !CAVLC_COEF_LRG_BLK
  aTable8 = m_pcCavlcCoder->GetLP8Table();
#endif
  aTable4 = m_pcCavlcCoder->GetLP4Table();
#if QC_MOD_LCEC
  aTableLastPosVlcIndex=m_pcCavlcCoder->GetLastPosVlcIndexTable();

  m_cTrQuant.init( g_uiMaxCUWidth, g_uiMaxCUHeight, 1 << m_uiQuadtreeTULog2MaxSize, m_iSymbolMode, aTable4, aTable8,
    aTableLastPosVlcIndex, m_bUseRDOQ, true );
#else
  m_cTrQuant.init( g_uiMaxCUWidth, g_uiMaxCUHeight, 1 << m_uiQuadtreeTULog2MaxSize, m_iSymbolMode, aTable4, aTable8, m_bUseRDOQ, true );
#endif

  // initialize encoder search class
  m_cSearch.init( this, &m_cTrQuant, m_iSearchRange, m_bipredSearchRange, m_iFastSearch, 0, &m_cEntropyCoder, &m_cRdCost, getRDSbacCoder(), getRDGoOnSbacCoder() );
  m_cSeqIter  = TEncSeqStructure::Iterator( m_cSequenceStructure, 0, 0 );
  m_bPicWaitingForCoding = false ;

#if MQT_ALF_NPASS
  if(m_bUseALF)
  {
    m_cAdaptiveLoopFilter.setALFEncodePassReduction( m_iALFEncodePassReduction );
  }
#endif
}

// ====================================================================================================================
// Public member functions
// ====================================================================================================================

Void TEncTop::deletePicBuffer()
{
  TComList<TComPic*>::iterator iterPic = m_cListPic.begin();
  Int iSize = Int( m_cListPic.size() );

  for ( Int i = 0; i < iSize; i++ )
  {
    TComPic* pcPic = *(iterPic++);

    pcPic->destroy();
    delete pcPic;
    pcPic = NULL;
  }
}

/**
 - Application has picture buffer list with size of GOP + 1
 - Picture buffer list acts like as ring buffer
 - End of the list has the latest picture
 .
 \param   bEos                true if end-of-sequence is reached
 \param   pcPicYuvOrg         original YUV picture
 \retval  rcListPicYuvRecOut  list of reconstruction YUV pictures
 \retval  rcListBitstreamOut  list of output bitstreams
 \retval  iNumEncoded         number of encoded pictures
 */
Void TEncTop::encode( bool bEos, std::map<PicOrderCnt, TComPicYuv*>& rcMapPicYuvRecOut, TComBitstream* pcBitstreamOut, Bool& bNewPicNeeded )
{

  //  TComBitstream*  pcBitstreamOut ;
  TComPicYuv*     pcPicYuvRecOut;
  TComPic*        pcOrgRefList[2][MAX_REF_PIC_NUM];  //GT: not used?
  TComPic*        pcPic ;

  bool bSomethingCoded = false ;

  if (m_bPicWaitingForCoding )
  {
    std::map<Int, TComPic*>::iterator cIter = m_acInputPicMap.find( (Int)m_cSeqIter.getPoc() );
    const bool bPictureAvailable = cIter != m_acInputPicMap.end();
    if (bPictureAvailable) //GT: it is possible that poc to code is not in input-picmap, but m_bPicWaitingForCoding is true, since current poc is beyond sequence-end
    {
      assert( m_acOutputPicMap.find( m_cSeqIter.getPoc() ) != m_acOutputPicMap.end() );
      pcPicYuvRecOut = m_acOutputPicMap[m_cSeqIter.getPoc()];
      m_acOutputPicMap.erase( m_cSeqIter.getPoc() );
      pcPic          = cIter->second ;

#if DEPTH_MAP_GENERATION
      // add extra pic buffers
      Bool  bNeedPrdDepthMapBuf = ( m_uiPredDepthMapGeneration > 0 );
      if( bNeedPrdDepthMapBuf && !pcPic->getPredDepthMap() )
      {
        pcPic->addPrdDepthMapBuffer();
      }
#endif

      // needed? dont think so
      TComPicYuv      cPicOrg;
      cPicOrg.create( pcPic->getPicYuvOrg()->getWidth(), pcPic->getPicYuvOrg()->getHeight(), g_uiMaxCUWidth, g_uiMaxCUHeight, g_uiMaxCUDepth );
      pcPic->getPicYuvOrg()->copyToPic( &cPicOrg );
      xSetPicProperties( pcPic) ;
      m_cPicEncoder.compressPic( pcBitstreamOut, cPicOrg, pcPic, pcPicYuvRecOut, pcOrgRefList,  m_bSeqFirst,  m_cListPic);
      bSomethingCoded = true;
      m_acInputPicMap.erase( cIter );
      cPicOrg.destroy() ;
      assert( rcMapPicYuvRecOut.find( pcPic->getPOC() ) == rcMapPicYuvRecOut.end() );
      rcMapPicYuvRecOut[pcPic->getPOC()] = pcPicYuvRecOut;
    }
    else if(m_uiViewId==-1)
      printf("\nPOC %4d skipped due to sequence end", Int(m_cSeqIter.getPoc()) );
    else
    {
      if( m_bIsDepth )
      {
        printf("\nDepth View \t%4d\t POC %4d skipped due to sequence end", m_uiViewId, Int(m_cSeqIter.getPoc() ));
      }
      else
      {
        printf("\nView \t\t%4d\t POC %4d skipped due to sequence end", m_uiViewId, Int(m_cSeqIter.getPoc() ));
      }
    }
    ++m_cSeqIter; //GT: increment, even bPictureAvailable might be false, (POC beyond sequence end);

    Bool hitSeqEndButHasToCode = bEos && !m_acInputPicMap.empty();
    if( (m_acInputPicMap.find( (Int)m_cSeqIter.getPoc() ) != m_acInputPicMap.end()) || hitSeqEndButHasToCode ) //GT: new poc also in InputList
    {
      m_bPicWaitingForCoding = true;
      bNewPicNeeded = false ;
    }
    else
    {
      m_bPicWaitingForCoding = false;
      bNewPicNeeded = true ;
    }
  }


  if(bSomethingCoded && !m_bPicWaitingForCoding ) //GT: no gaps any more
  {
    m_uiNumAllPicCoded += m_iNumPicRcvd;
    m_iNumPicRcvd       = 0;
  }


  if (bEos&&m_uiViewId==-1)
  {
    printOutSummary (m_uiNumAllPicCoded);
  }
}


Void TEncTop::receivePic( bool bEos, TComPicYuv* pcPicYuvOrg, TComPicYuv* pcPicYuvRec, TComPicYuv* pcOrgPdmDepth )
{
  if( !m_bPicWaitingForCoding ) //GT: insert pcPicYuvOrg in m_acInputPicMap and m_cListPic
  {
    TComPic* pcPicCurr = NULL;
    xGetNewPicBuffer( pcPicCurr ); //GT: assigns next POC to input pic and stores it in m_cListPic
    pcPicYuvOrg->copyToPic( pcPicCurr->getPicYuvOrg() );
#if HHI_INTER_VIEW_MOTION_PRED
    if( m_uiMultiviewMvRegMode )
    {
      AOF( pcOrgPdmDepth );
      AOF( pcPicCurr->getOrgDepthMap() );
      pcOrgPdmDepth->copyToPic( pcPicCurr->getOrgDepthMap() );
    }
    else
    {
      AOT( pcOrgPdmDepth );
      AOT( pcPicCurr->getOrgDepthMap() );
    }
#endif
    m_acInputPicMap.insert( std::make_pair(pcPicCurr->getPOC(), pcPicCurr)); //GT: input pic to m_acInputPicMap
    assert( m_acOutputPicMap.find( pcPicCurr->getPOC() ) == m_acOutputPicMap.end() );
    m_acOutputPicMap[pcPicCurr->getPOC()] = pcPicYuvRec;
  }

  bool hitSeqEndButHasToCode = bEos && !m_acInputPicMap.empty();  //GT: End of sequence has been reached, but still pictures to code left
  m_bPicWaitingForCoding = m_bPicWaitingForCoding || hitSeqEndButHasToCode ;

  if( m_acInputPicMap.find( (Int)m_cSeqIter.getPoc() ) != m_acInputPicMap.end() ) //GT: If poc to code is in input-picmap; not necessary
  {
    m_bPicWaitingForCoding = true;
  }
}


Void
TEncTop::deleteExtraPicBuffers( Int iPoc )
{
  TComPic*                      pcPic = 0;
  TComList<TComPic*>::iterator  cIter = m_cListPic.begin();
  TComList<TComPic*>::iterator  cEnd  = m_cListPic.end  ();
  for( ; cIter != cEnd; cIter++ )
  {
    if( (*cIter)->getPOC() == iPoc )
    {
      pcPic = *cIter;
      break;
    }
  }
  AOF( pcPic );
  if ( pcPic )
  {
    pcPic->removeOriginalBuffer   ();
#if POZNAN_AVAIL_MAP
    pcPic->removeAvailabilityBuffer();
#endif
#if POZNAN_SYNTH_VIEW
    pcPic->removeSynthesisBuffer();
#endif
#if HHI_INTER_VIEW_MOTION_PRED
    pcPic->removeOrgDepthMapBuffer();
#endif
#if HHI_INTER_VIEW_RESIDUAL_PRED
    pcPic->removeResidualBuffer   ();
#endif
#if HHI_INTERVIEW_SKIP
    pcPic->removeUsedPelsMapBuffer();
#endif
  }
}


#if AMVP_BUFFERCOMPRESS
Void
TEncTop::compressMotion( Int iPoc )
{
  TComPic*                      pcPic = 0;
  TComList<TComPic*>::iterator  cIter = m_cListPic.begin();
  TComList<TComPic*>::iterator  cEnd  = m_cListPic.end  ();
  for( ; cIter != cEnd; cIter++ )
  {
    if( (*cIter)->getPOC() == iPoc )
    {
      pcPic = *cIter;
      break;
    }
  }
  AOF( pcPic );
  if ( pcPic )
  {
    pcPic->compressMotion();
  }
}
#endif



// ====================================================================================================================
// Protected member functions
// ====================================================================================================================

/**
 - Application has picture buffer list with size of GOP + 1
 - Picture buffer list acts like as ring buffer
 - End of the list has the latest picture
 .
 \retval rpcPic obtained picture buffer
 */
Void TEncTop::xGetNewPicBuffer ( TComPic*& rpcPic )
{
  TComSlice::sortPicList(m_cListPic);

  // bug-fix - erase frame memory (previous GOP) which is not used for reference any more
  if (m_cListPic.size() >=  m_uiCodedPictureStoreSize )  // 2)   //  K. Lee bug fix - for multiple reference > 2
  {
    rpcPic = m_cListPic.popFront();

    // is it necessary without long-term reference?
  }
  else
  {
    rpcPic = new TComPic;
    rpcPic->create( m_iSourceWidth, m_iSourceHeight, g_uiMaxCUWidth, g_uiMaxCUHeight, g_uiMaxCUDepth );
  }

  m_cListPic.pushBack( rpcPic );
  rpcPic->setReconMark (false);

  m_iPOCLast++;
  m_iNumPicRcvd++;

  rpcPic->addOriginalBuffer();
#if HHI_INTER_VIEW_MOTION_PRED
  if( m_uiMultiviewMvRegMode )
  {
    rpcPic->addOrgDepthMapBuffer();
  }
#endif

#if HHI_INTERVIEW_SKIP
  if( getInterViewSkip() )
  {
    rpcPic->addUsedPelsMapBuffer();
  }
#endif

  rpcPic->setCurrSliceIdx( 0 ); // MW
  rpcPic->getSlice(0)->setPOC( m_iPOCLast );

  // mark it should be extended
  rpcPic->getPicYuvRec()->setBorderExtension(false);
}


Void TEncTop::xInitSPS()
{
  m_cSPS.setWidth         ( m_iSourceWidth      );
  m_cSPS.setHeight        ( m_iSourceHeight     );
  m_cSPS.setPad           ( m_aiPad             );
  m_cSPS.setMaxCUWidth    ( g_uiMaxCUWidth      );
  m_cSPS.setMaxCUHeight   ( g_uiMaxCUHeight     );
  m_cSPS.setMaxCUDepth    ( g_uiMaxCUDepth      );
  m_cSPS.setMinTrDepth    ( 0                   );
  m_cSPS.setMaxTrDepth    ( 1                   );

  m_cSPS.setUseALF        ( m_bUseALF           );

  m_cSPS.setQuadtreeTULog2MaxSize( m_uiQuadtreeTULog2MaxSize );
  m_cSPS.setQuadtreeTULog2MinSize( m_uiQuadtreeTULog2MinSize );
  m_cSPS.setQuadtreeTUMaxDepthInter( m_uiQuadtreeTUMaxDepthInter    );
  m_cSPS.setQuadtreeTUMaxDepthIntra( m_uiQuadtreeTUMaxDepthIntra    );

  m_cSPS.setUseDQP        ( m_iMaxDeltaQP != 0  );
#if !HHI_NO_LowDelayCoding
  m_cSPS.setUseLDC        ( m_bUseLDC           );
#endif
  m_cSPS.setUsePAD        ( m_bUsePAD           );

  m_cSPS.setUseMRG        ( m_bUseMRG           ); // SOPH:

#if LM_CHROMA
  m_cSPS.setUseLMChroma   ( m_bUseLMChroma           );
#endif

  m_cSPS.setMaxTrSize   ( 1 << m_uiQuadtreeTULog2MaxSize );

  if( m_bIsDepth )
  {
    m_cSPS.initMultiviewSPSDepth    ( m_uiViewId, m_iViewOrderIdx );
#if DEPTH_MAP_GENERATION
    m_cSPS.setPredDepthMapGeneration( m_uiViewId, true );
#endif
#if HHI_INTER_VIEW_RESIDUAL_PRED
    m_cSPS.setMultiviewResPredMode  ( 0 );
#endif
  }
  else
  {
    m_cSPS.initMultiviewSPS           ( m_uiViewId, m_iViewOrderIdx, m_uiCamParPrecision, m_bCamParInSliceHeader, m_aaiCodedScale, m_aaiCodedOffset );
    if( m_uiViewId )
    {
#if DEPTH_MAP_GENERATION
#if HHI_INTER_VIEW_MOTION_PRED
      m_cSPS.setPredDepthMapGeneration( m_uiViewId, false, m_uiPredDepthMapGeneration, m_uiMultiviewMvPredMode, m_uiPdmPrecision, m_aaiPdmScaleNomDelta, m_aaiPdmOffset );
#else
      m_cSPS.setPredDepthMapGeneration( m_uiViewId, false, m_uiPredDepthMapGeneration, 0, m_uiPdmPrecision, m_aaiPdmScaleNomDelta, m_aaiPdmOffset );
#endif
#endif
#if HHI_INTER_VIEW_RESIDUAL_PRED
      m_cSPS.setMultiviewResPredMode  ( m_uiMultiviewResPredMode );
#endif
    }
    else
    {
#if DEPTH_MAP_GENERATION
      m_cSPS.setPredDepthMapGeneration( m_uiViewId, false );
#endif
#if HHI_INTER_VIEW_RESIDUAL_PRED
      m_cSPS.setMultiviewResPredMode  ( 0 );
#endif
    }
  }
  m_cSPS.setSPSId( ( m_uiViewId << 1 ) + ( m_bIsDepth ? 1 : 0 ) );

#if DCM_COMB_LIST
  m_cSPS.setUseLComb    ( m_bUseLComb           );
  m_cSPS.setLCMod       ( m_bLCMod   );
#endif

  Int i;
#if HHI_AMVP_OFF
  for ( i = 0; i < g_uiMaxCUDepth; i++ )
  {
    m_cSPS.setAMVPMode( i, AM_NONE );
  }
#else
  for ( i = 0; i < g_uiMaxCUDepth; i++ )
  {
    m_cSPS.setAMVPMode( i, AM_EXPL );
  }
#endif


#if HHI_RMP_SWITCH
  m_cSPS.setUseRMP( m_bUseRMP );
#endif

  m_cSPS.setBitDepth    ( g_uiBitDepth        );
  m_cSPS.setBitIncrement( g_uiBitIncrement    );

#if MTK_NONCROSS_INLOOP_FILTER
  m_cSPS.setLFCrossSliceBoundaryFlag( m_bLFCrossSliceBoundaryFlag );
#endif
#if MTK_SAO
  m_cSPS.setUseSAO             ( m_bUseSAO         );
#endif
#if HHI_DMM_WEDGE_INTRA || HHI_DMM_PRED_TEX
  m_cSPS.setUseDMM( m_bUseDMM );
#endif
#if HHI_MPI
  m_cSPS.setUseMVI( m_bUseMVI );
#endif

  m_cSPS.setCodedPictureBufferSize( m_uiCodedPictureStoreSize );
}

#if CONSTRAINED_INTRA_PRED
Void TEncTop::xInitPPS()
{
  m_cPPS.setConstrainedIntraPred( m_bUseConstrainedIntraPred );
  m_cPPS.setPPSId( ( m_uiViewId << 1 ) + ( m_bIsDepth ? 1 : 0 ) );
  m_cPPS.setSPSId( ( m_uiViewId << 1 ) + ( m_bIsDepth ? 1 : 0 ) );

#ifdef WEIGHT_PRED
  m_cPPS.setUseWP( m_bUseWeightPred );
  m_cPPS.setWPBiPredIdc( m_uiBiPredIdc );
#endif
}
#endif


Void TEncTop::setTEncTopList(std::vector<TEncTop*>* pacTEncTopList )
{
  assert(m_uiViewId!=-1); // not to be set for single view coding

  m_pacTEncTopList=pacTEncTopList;

}


Void TEncTop::printOutSummary(UInt uiNumAllPicCoded)
{
  assert (uiNumAllPicCoded == m_cAnalyzeAll.getNumPic());

  //--CFG_KDY
  m_cAnalyzeAll.setFrmRate( getFrameRate() );
  m_cAnalyzeI.setFrmRate( getFrameRate() );
  m_cAnalyzeP.setFrmRate( getFrameRate() );
  m_cAnalyzeB.setFrmRate( getFrameRate() );

  //-- all
  if(m_uiViewId==-1)
    printf( "\n\nSUMMARY --------------------------------------------------------\n" );
  else {
    if ( m_bIsDepth )
    {
      printf( "\n\nSUMMARY ---------------------------------------------- DEPTH %2d\n", m_uiViewId );
    }
    else
    {
      printf( "\n\nSUMMARY ---------------------------------------------- VIDEO %2d\n", m_uiViewId );
    }
  };
  m_cAnalyzeAll.printOut('a');

  printf( "\n\nI Slices--------------------------------------------------------\n" );
  m_cAnalyzeI.printOut('i');

  printf( "\n\nP Slices--------------------------------------------------------\n" );
  m_cAnalyzeP.printOut('p');

  printf( "\n\nB Slices--------------------------------------------------------\n" );
  m_cAnalyzeB.printOut('b');

#if _SUMMARY_OUT_
  m_cAnalyzeAll.printSummaryOut();
#endif
#if _SUMMARY_PIC_
  m_cAnalyzeI.printSummary('I');
  m_cAnalyzeP.printSummary('P');
  m_cAnalyzeB.printSummary('B');
#endif
}

Void TEncTop::xSetRefPics( TComPic* pcPic, RefPicList eRefPicList )
{
  assert(m_cSeqIter.getFrameDescriptor().getAllowedRelativeRefPocs( eRefPicList, pcPic->getViewIdx() ).size() == m_cSeqIter.getFrameDescriptor().getAllowedReferenceViewIdx( eRefPicList, pcPic->getViewIdx()).size());
#if 1
  // check if refPic is available
  Int iNumberOfRefs = 0;
  std::vector<Int> aiRefPocs ;
  std::vector<Int> aiRefViews ;

  for( Int i=0; i<(Int)m_cSeqIter.getFrameDescriptor().getAllowedRelativeRefPocs( eRefPicList, pcPic->getViewIdx() ).size(); i++ )
  {
    Int iRefPoc  = m_cSeqIter.getFrameDescriptor().getAllowedRelativeRefPocs( eRefPicList, pcPic->getViewIdx() )[i]+pcPic->getPOC() ;
    Int iRefViewIdx = m_cSeqIter.getFrameDescriptor().getAllowedReferenceViewIdx( eRefPicList, pcPic->getViewIdx() )[i];
    Bool bFoundRefPic = false;

    if( iRefPoc == pcPic->getPOC() ) // interview
    {
      assert( iRefViewIdx < (Int)m_uiViewId );
      aiRefPocs.push_back(iRefPoc);
      aiRefViews.push_back(iRefViewIdx);
      iNumberOfRefs++ ;
      bFoundRefPic = true ;
      continue;
    }
    else if ( iRefViewIdx < 0 ) // temporal
    {
      for( TComList<TComPic*>::iterator it = m_cListPic.begin(); it!=m_cListPic.end(); it++)
      {
        if( (*it)->getViewIdx() == pcPic->getViewIdx() && (*it)->getPOC() == iRefPoc && (*it)->getReconMark() )
        {
          aiRefPocs.push_back(iRefPoc);
          aiRefViews.push_back(m_uiViewId);
          bFoundRefPic = true ;
          iNumberOfRefs++ ;
          break;
        }
      }
      if( (iRefPoc < pcPic->getPOC()) && !bFoundRefPic )
      {
        printf("\nInconsistence in GOP-String!");
        assert(0);
      }
    }
  }
  for ( Int i=0; i< iNumberOfRefs; i++)
  {
    pcPic->setRefViewIdx( aiRefViews[i], eRefPicList, i );
    pcPic->setRefPOC(aiRefPocs[i], eRefPicList, i );
  }
  pcPic->setNumRefs( iNumberOfRefs, eRefPicList );
#else
  for( Int i=0; i<(Int)m_cSeqIter.getFrameDescriptor().getAllowedRelativeRefPocs( eRefPicList, pcPic->getViewIdx() ).size(); i++ )
  {
    const int iRefViewIdx = m_cSeqIter.getFrameDescriptor().getAllowedReferenceViewIdx( eRefPicList, pcPic->getViewIdx() )[i];
    if( iRefViewIdx < 0 )
    {
      // temporal reference from current view
      pcPic->setRefViewIdx( m_iViewIdx, eRefPicList, i );
    }
    else
    {
      pcPic->setRefViewIdx( iRefViewIdx, eRefPicList, i );
    }
    pcPic->setRefPOC(m_cSeqIter.getFrameDescriptor().getAllowedRelativeRefPocs( eRefPicList, pcPic->getViewIdx() )[i]+pcPic->getPOC(), eRefPicList, i );
  }
  pcPic->setNumRefs( m_cSeqIter.getFrameDescriptor().getAllowedRelativeRefPocs( eRefPicList, pcPic->getViewIdx() ).size(), eRefPicList );
#endif
}

Void TEncTop::xCheckSliceType(TComPic* pcPic)
{
  if( pcPic->getNumRefs(REF_PIC_LIST_0) == 0 && pcPic->getNumRefs(REF_PIC_LIST_1) == 0 && pcPic->getSliceType() == I_SLICE )
  {
    return ;
  }
  if( pcPic->getNumRefs(REF_PIC_LIST_0) != 0 && pcPic->getNumRefs(REF_PIC_LIST_1) == 0 && pcPic->getSliceType() == P_SLICE )
  {
    return ;
  }
  if( pcPic->getNumRefs(REF_PIC_LIST_0) != 0 && pcPic->getNumRefs(REF_PIC_LIST_1) != 0 && pcPic->getSliceType() == B_SLICE )
  {
    return ;
  }
  if( pcPic->getNumRefs(REF_PIC_LIST_0) == 0 && pcPic->getNumRefs(REF_PIC_LIST_1) == 0 )
  {
    pcPic->setSliceType(I_SLICE) ;
    return;
  }
  if( pcPic->getNumRefs(REF_PIC_LIST_0) != 0 && pcPic->getNumRefs(REF_PIC_LIST_1) == 0 )
  {
    pcPic->setSliceType(P_SLICE) ;
    return;
  }

  assert(0);
}


Void TEncTop::xSetPicProperties(TComPic* pcPic)
{
  pcPic->setSliceType( m_cSeqIter.getFrameDescriptor().getSliceType(m_uiViewId) );
  pcPic->setReferenced( m_cSeqIter.getFrameDescriptor().getStoreForRef(m_uiViewId) );
  pcPic->setColDir( m_cSeqIter.getFrameDescriptor().getColDir() ) ;
  const Bool bQpChangePointPassed = m_iFrameNumInCodingOrder++ >= getQpChangeFrame();
  const Int  iQpChangeOffset      = bQpChangePointPassed ? ( m_bIsDepth ? getQpChangeOffsetDepth() : getQpChangeOffsetVideo() ) : 0;
  pcPic->setQP(max(MIN_QP,min(MAX_QP, m_iQP+ m_aiTLayerQPOffset[m_cSeqIter.getFrameDescriptor().getTEncSeqStructureLayer(m_uiViewId)] + iQpChangeOffset )) );
  pcPic->setViewIdx( m_uiViewId );
#if 0
  pcPic->setNumRefs(0, REF_PIC_LIST_0);
  pcPic->setNumRefs(0, REF_PIC_LIST_1);


  if( m_cSeqIter.getFrameDescriptor().getSliceType(m_iViewIdx)== P_SLICE || m_cSeqIter.getFrameDescriptor().getSliceType(m_iViewIdx)== B_SLICE)
  {
    xSetRefPics( pcPic, REF_PIC_LIST_0 );
  }
  if( m_cSeqIter.getFrameDescriptor().getSliceType(m_iViewIdx)== B_SLICE)
  {
    xSetRefPics( pcPic, REF_PIC_LIST_1 );
  }
#else
  xSetRefPics( pcPic, REF_PIC_LIST_0 );
  xSetRefPics( pcPic, REF_PIC_LIST_1 );
  xCheckSliceType( pcPic );
#endif

  pcPic->setScaleOffset( m_aaiCodedScale, m_aaiCodedOffset );
}
