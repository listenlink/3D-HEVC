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



/** \file     TEncPic.cpp
    \brief    GOP encoder class
*/

#include "TEncTop.h"
#include "TEncGOP.h"
#include "TEncAnalyze.h"
#include "../libmd5/MD5.h"
#include "../TLibCommon/SEI.h"

#include <time.h>

#include "../../App/TAppEncoder/TAppEncTop.h"

// ====================================================================================================================
// Constructor / destructor / initialization / destroy
// ====================================================================================================================

TEncPic::TEncPic()
{
  m_pcCfg               = NULL;
  m_pcSliceEncoder      = NULL;
  m_pcListPic           = NULL;

  m_pcEntropyCoder      = NULL;
  m_pcCavlcCoder        = NULL;
  m_pcSbacCoder         = NULL;
  m_pcBinCABAC          = NULL;
#if DEPTH_MAP_GENERATION
  m_pcDepthMapGenerator = NULL;
#endif
#if HHI_INTER_VIEW_RESIDUAL_PRED
  m_pcResidualGenerator = NULL;
#endif

#if DCM_DECODING_REFRESH
  m_bRefreshPending     = 0;
  m_uiPOCCDR            = 0;
#endif

  return;
}

TEncPic::~TEncPic()
{
}

/** Create list to contain pointers to LCU start addresses of slice.
 * \param iWidth, iHeight are picture width, height. iMaxCUWidth, iMaxCUHeight are LCU width, height.
 */
Void  TEncPic::create( Int iWidth, Int iHeight, UInt iMaxCUWidth, UInt iMaxCUHeight )
{
  UInt uiWidthInCU       = ( iWidth %iMaxCUWidth  ) ? iWidth /iMaxCUWidth  + 1 : iWidth /iMaxCUWidth;
  UInt uiHeightInCU      = ( iHeight%iMaxCUHeight ) ? iHeight/iMaxCUHeight + 1 : iHeight/iMaxCUHeight;
  UInt uiNumCUsInFrame   = uiWidthInCU * uiHeightInCU;
  m_uiStoredStartCUAddrForEncodingSlice = new UInt [uiNumCUsInFrame+1];
  m_uiStoredStartCUAddrForEncodingEntropySlice = new UInt [uiNumCUsInFrame+1];
}

Void  TEncPic::destroy()
{
  delete [] m_uiStoredStartCUAddrForEncodingSlice; m_uiStoredStartCUAddrForEncodingSlice = NULL;
  delete [] m_uiStoredStartCUAddrForEncodingEntropySlice; m_uiStoredStartCUAddrForEncodingEntropySlice = NULL;
}

Void TEncPic::init ( TEncTop* pcTEncTop )
{
  m_pcEncTop     = pcTEncTop;
  m_pcCfg                = pcTEncTop;
  m_pcSliceEncoder       = pcTEncTop->getSliceEncoder();
  m_pcListPic            = pcTEncTop->getListPic();

  m_pcEntropyCoder       = pcTEncTop->getEntropyCoder();
  m_pcCavlcCoder         = pcTEncTop->getCavlcCoder();
  m_pcSbacCoder          = pcTEncTop->getSbacCoder();
  m_pcBinCABAC           = pcTEncTop->getBinCABAC();
  m_pcLoopFilter         = pcTEncTop->getLoopFilter();
  m_pcBitCounter         = pcTEncTop->getBitCounter();
#if DEPTH_MAP_GENERATION
  m_pcDepthMapGenerator  = pcTEncTop->getDepthMapGenerator();
#endif
#if HHI_INTER_VIEW_RESIDUAL_PRED
  m_pcResidualGenerator  = pcTEncTop->getResidualGenerator();
#endif

  // Adaptive Loop filter
  m_pcAdaptiveLoopFilter = pcTEncTop->getAdaptiveLoopFilter();
  //--Adaptive Loop filter
#if MTK_SAO
  m_pcSAO                = pcTEncTop->getSAO();
#endif
  m_pcRdCost             = pcTEncTop->getRdCost();

}

// ====================================================================================================================
// Public member functions
// ====================================================================================================================

Void TEncPic::compressPic( TComBitstream* pcBitstreamOut, TComPicYuv cPicOrg, TComPic* pcPic, TComPicYuv* pcPicYuvRecOut,
               TComPic* pcOrgRefList[2][MAX_REF_PIC_NUM], Bool&  rbSeqFirst, TComList<TComPic*>& rcListPic  )
{
  TComSlice*      pcSlice;

      //-- For time output for each slice
      long iBeforeTime = clock();

      //  Bitstream reset
      pcBitstreamOut->resetBits();
      pcBitstreamOut->rewindStreamPacket();

      //  Slice data initialization
      pcPic->clearSliceBuffer();
      assert(pcPic->getNumAllocatedSlice() == 1);
      m_pcSliceEncoder->setSliceIdx(0);
      pcPic->setCurrSliceIdx(0);
      m_pcSliceEncoder->initEncSlice ( pcPic, pcSlice );
      pcSlice->setSliceIdx(0);

      //  Set SPS
      pcSlice->setSPS( m_pcEncTop->getSPS() );
      pcSlice->setPPS( m_pcEncTop->getPPS() );
      pcSlice->setPPSId( pcSlice->getPPS()->getPPSId() );

  // set mutliview parameters
      pcSlice->initMultiviewSlice( pcPic->getCodedScale(), pcPic->getCodedOffset() );

#if DCM_DECODING_REFRESH
      // Set the nal unit type
      if( pcSlice->getPOC() == 0 )
        pcSlice->setNalUnitType( NAL_UNIT_CODED_SLICE_IDR );
      else
        pcSlice->setNalUnitType( NAL_UNIT_CODED_SLICE );

      //pcSlice->setNalUnitType(getNalUnitType(uiPOCCurr));
      // Do decoding refresh marking if any
      pcSlice->decodingRefreshMarking(m_uiPOCCDR, m_bRefreshPending, rcListPic);
#endif

// GT FIX
  std::vector<TComPic*> apcSpatRefPics = m_pcEncTop->getEncTop()->getSpatialRefPics( pcPic->getViewIdx(), pcSlice->getPOC(), m_pcEncTop->isDepthCoder() );
  TComPic * const pcTexturePic = m_pcEncTop->isDepthCoder() ? m_pcEncTop->getEncTop()->getPicFromView( pcPic->getViewIdx(), pcSlice->getPOC(), false ) : NULL;
  assert( ! m_pcEncTop->isDepthCoder() || pcTexturePic != NULL );
  pcSlice->setTexturePic( pcTexturePic );

  pcSlice->setRefPicListFromGOPSTring( rcListPic, apcSpatRefPics );

#if HHI_VSO
  m_pcEncTop->getEncTop()->setMVDPic(pcPic->getViewIdx(), pcSlice->getPOC(), pcPic->getMVDReferenceInfo() );


  Bool bUseVSO = m_pcEncTop->getUseVSO();
  m_pcRdCost->setUseVSO( bUseVSO );

  if ( bUseVSO )
  {
    Int iVSOMode = m_pcEncTop->getVSOMode();
    m_pcRdCost->setVSOMode( iVSOMode  );
#if HHI_VSO_DIST_INT
    m_pcRdCost->setAllowNegDist( m_pcEncTop->getAllowNegDist() );
#endif

    if ( iVSOMode == 4 )
    {
      m_pcEncTop->getEncTop()->setupRenModel( pcSlice->getPOC(), pcPic->getViewIdx(), m_pcEncTop->isDepthCoder() ? 1 : 0 );
    }
    else
  {
    m_pcRdCost->setRefDataFromMVDInfo( pcPic->getMVDReferenceInfo() );
  }
  }
#endif

#if HHI_INTERVIEW_SKIP
  if ( m_pcEncTop->getInterViewSkip() )
  {
    m_pcEncTop->getEncTop()->getUsedPelsMap( pcPic->getViewIdx(), pcPic->getPOC(), pcPic->getUsedPelsMap() );
  }
#endif

      pcSlice->setNoBackPredFlag( false );
#if DCM_COMB_LIST
      if ( pcSlice->getSliceType() == B_SLICE && !pcSlice->getRefPicListCombinationFlag())
#else
      if ( pcSlice->getSliceType() == B_SLICE )
#endif
      {
        if ( pcSlice->getNumRefIdx(RefPicList( 0 ) ) == pcSlice->getNumRefIdx(RefPicList( 1 ) ) )
        {
          pcSlice->setNoBackPredFlag( true );
          int i;
          for ( i=0; i < pcSlice->getNumRefIdx(RefPicList( 1 ) ); i++ )
          {
            if ( pcSlice->getRefPOC(RefPicList(1), i) != pcSlice->getRefPOC(RefPicList(0), i) )
            {
              pcSlice->setNoBackPredFlag( false );
              break;
            }
          }
        }
      }

#if DCM_COMB_LIST
      if(pcSlice->getNoBackPredFlag())
      {
        pcSlice->setNumRefIdx(REF_PIC_LIST_C, -1);
      }
      pcSlice->generateCombinedList();
#endif

      /////////////////////////////////////////////////////////////////////////////////////////////////// Compress a slice
      //  Slice compression
      if (m_pcCfg->getUseASR())
      {
        m_pcSliceEncoder->setSearchRange(pcSlice);
      }
#ifdef ROUNDING_CONTROL_BIPRED
      Bool b = true;
      if (m_pcCfg->getUseRoundingControlBipred())
      {
        if (m_pcCfg->getCodedPictureBufferSize()==1)
          b = ((pcSlice->getPOC()&1)==0);
        else
          b = (pcSlice->isReferenced() == 0);
      }

#if HIGH_ACCURACY_BI
      pcSlice->setRounding(false);
#else
      pcSlice->setRounding(b);
#endif
#endif

      UInt uiStartCUAddrSliceIdx = 0; // used to index "m_uiStoredStartCUAddrForEncodingSlice" containing locations of slice boundaries
      UInt uiStartCUAddrSlice    = 0; // used to keep track of current slice's starting CU addr.
      pcSlice->setSliceCurStartCUAddr( uiStartCUAddrSlice ); // Setting "start CU addr" for current slice
      memset(m_uiStoredStartCUAddrForEncodingSlice, 0, sizeof(UInt) * (pcPic->getPicSym()->getNumberOfCUsInFrame()+1));

      UInt uiStartCUAddrEntropySliceIdx = 0; // used to index "m_uiStoredStartCUAddrForEntropyEncodingSlice" containing locations of slice boundaries
      UInt uiStartCUAddrEntropySlice    = 0; // used to keep track of current Entropy slice's starting CU addr.
      pcSlice->setEntropySliceCurStartCUAddr( uiStartCUAddrEntropySlice ); // Setting "start CU addr" for current Entropy slice
      memset(m_uiStoredStartCUAddrForEncodingEntropySlice, 0, sizeof(UInt) * (pcPic->getPicSym()->getNumberOfCUsInFrame()+1));

      UInt uiNextCUAddr = 0;
      m_uiStoredStartCUAddrForEncodingSlice[uiStartCUAddrSliceIdx++]                = uiNextCUAddr;
      m_uiStoredStartCUAddrForEncodingEntropySlice[uiStartCUAddrEntropySliceIdx++]  = uiNextCUAddr;

#if DEPTH_MAP_GENERATION
      // init view component and predict virtual depth map
      m_pcDepthMapGenerator->initViewComponent( pcPic );
      m_pcDepthMapGenerator->predictDepthMap  ( pcPic );
#if HHI_INTER_VIEW_MOTION_PRED
      m_pcDepthMapGenerator->covertOrgDepthMap( pcPic );
#endif
#if HHI_INTER_VIEW_RESIDUAL_PRED
      m_pcResidualGenerator->initViewComponent( pcPic );
#endif
#endif

      while(uiNextCUAddr<pcPic->getPicSym()->getNumberOfCUsInFrame()) // determine slice boundaries
      {
        pcSlice->setNextSlice       ( false );
        pcSlice->setNextEntropySlice( false );
        assert(pcPic->getNumAllocatedSlice() == uiStartCUAddrSliceIdx);
        m_pcSliceEncoder->precompressSlice( pcPic );
        m_pcSliceEncoder->compressSlice   ( pcPic );

        Bool bNoBinBitConstraintViolated = (!pcSlice->isNextSlice() && !pcSlice->isNextEntropySlice());
        if (pcSlice->isNextSlice() || (bNoBinBitConstraintViolated && m_pcCfg->getSliceMode()==AD_HOC_SLICES_FIXED_NUMBER_OF_LCU_IN_SLICE))
        {
          uiStartCUAddrSlice                                              = pcSlice->getSliceCurEndCUAddr();
          // Reconstruction slice
          m_uiStoredStartCUAddrForEncodingSlice[uiStartCUAddrSliceIdx++]  = uiStartCUAddrSlice;
          // Entropy slice
          if (uiStartCUAddrEntropySliceIdx>0 && m_uiStoredStartCUAddrForEncodingEntropySlice[uiStartCUAddrEntropySliceIdx-1] != uiStartCUAddrSlice)
          {
            m_uiStoredStartCUAddrForEncodingEntropySlice[uiStartCUAddrEntropySliceIdx++]  = uiStartCUAddrSlice;
          }

          if (uiStartCUAddrSlice < pcPic->getPicSym()->getNumberOfCUsInFrame())
          {
            pcPic->allocateNewSlice();
            pcPic->setCurrSliceIdx                  ( uiStartCUAddrSliceIdx-1 );
            m_pcSliceEncoder->setSliceIdx           ( uiStartCUAddrSliceIdx-1 );
            pcSlice = pcPic->getSlice               ( uiStartCUAddrSliceIdx-1 );
            pcSlice->copySliceInfo                  ( pcPic->getSlice(0)      );
            pcSlice->setSliceIdx                    ( uiStartCUAddrSliceIdx-1 );
            pcSlice->setSliceCurStartCUAddr         ( uiStartCUAddrSlice      );
            pcSlice->setEntropySliceCurStartCUAddr  ( uiStartCUAddrSlice      );
            pcSlice->setSliceBits(0);
          }
        }
        else if (pcSlice->isNextEntropySlice() || (bNoBinBitConstraintViolated && m_pcCfg->getEntropySliceMode()==SHARP_FIXED_NUMBER_OF_LCU_IN_ENTROPY_SLICE))
        {
          uiStartCUAddrEntropySlice                                                     = pcSlice->getEntropySliceCurEndCUAddr();
          m_uiStoredStartCUAddrForEncodingEntropySlice[uiStartCUAddrEntropySliceIdx++]  = uiStartCUAddrEntropySlice;
          pcSlice->setEntropySliceCurStartCUAddr( uiStartCUAddrEntropySlice );
        }
        else
        {
          uiStartCUAddrSlice                                                            = pcSlice->getSliceCurEndCUAddr();
          uiStartCUAddrEntropySlice                                                     = pcSlice->getEntropySliceCurEndCUAddr();
        }

        uiNextCUAddr = (uiStartCUAddrSlice > uiStartCUAddrEntropySlice) ? uiStartCUAddrSlice : uiStartCUAddrEntropySlice;
      }
      m_uiStoredStartCUAddrForEncodingSlice[uiStartCUAddrSliceIdx++]                = pcSlice->getSliceCurEndCUAddr();
      m_uiStoredStartCUAddrForEncodingEntropySlice[uiStartCUAddrEntropySliceIdx++]  = pcSlice->getSliceCurEndCUAddr();

      pcSlice = pcPic->getSlice(0);
#if MTK_SAO  // PRE_DF
      SAOParam cSaoParam;
#endif

#if HHI_INTER_VIEW_RESIDUAL_PRED
      // set residual picture
      m_pcResidualGenerator->setRecResidualPic( pcPic );
#endif
#if DEPTH_MAP_GENERATION
      // update virtual depth map
      m_pcDepthMapGenerator->updateDepthMap( pcPic );
#endif

      //-- Loop filter
      m_pcLoopFilter->setCfg(pcSlice->getLoopFilterDisable(), m_pcCfg->getLoopFilterAlphaC0Offget(), m_pcCfg->getLoopFilterBetaOffget());
      m_pcLoopFilter->loopFilterPic( pcPic );

#if MTK_NONCROSS_INLOOP_FILTER
      pcSlice = pcPic->getSlice(0);

      if(pcSlice->getSPS()->getUseALF())
      {
        if(pcSlice->getSPS()->getLFCrossSliceBoundaryFlag())
        {
          m_pcAdaptiveLoopFilter->setUseNonCrossAlf(false);
        }
        else
        {
          UInt uiNumSlices = uiStartCUAddrSliceIdx-1;
          m_pcAdaptiveLoopFilter->setUseNonCrossAlf( (uiNumSlices > 1)  );
          if(m_pcAdaptiveLoopFilter->getUseNonCrossAlf())
          {
            m_pcAdaptiveLoopFilter->setNumSlicesInPic( uiNumSlices );
            m_pcAdaptiveLoopFilter->createSlice();

            //set the startLCU and endLCU addr. to ALF slices
            for(UInt i=0; i< uiNumSlices ; i++)
            {
              (*m_pcAdaptiveLoopFilter)[i].create(pcPic, i,
                                                  m_uiStoredStartCUAddrForEncodingSlice[i],
                                                  m_uiStoredStartCUAddrForEncodingSlice[i+1]-1
                                                  );

            }
          }
        }
      }
#endif
      /////////////////////////////////////////////////////////////////////////////////////////////////// File writing
      // Set entropy coder
      m_pcEntropyCoder->setEntropyCoder   ( m_pcCavlcCoder, pcSlice );

      /* write various header sets.
       * The header sets are written into a separate bitstream buffer to
       * allow SEI messages that are calculated after the picture has been
       * encoded to be sent before the picture.
       */
      TComBitstream bs_SPS_PPS_SEI;
      bs_SPS_PPS_SEI.create(512); /* TODO: this should dynamically resize */
      if ( rbSeqFirst )
      {
        m_pcEntropyCoder->setBitstream(&bs_SPS_PPS_SEI);

        m_pcEntropyCoder->encodeSPS( pcSlice->getSPS() );
        bs_SPS_PPS_SEI.write( 1, 1 );
        bs_SPS_PPS_SEI.writeAlignZero();
        // generate start code
        bs_SPS_PPS_SEI.write( 1, 32);

        m_pcEntropyCoder->encodePPS( pcSlice->getPPS() );
        bs_SPS_PPS_SEI.write( 1, 1 );
        bs_SPS_PPS_SEI.writeAlignZero();
        // generate start code
        bs_SPS_PPS_SEI.write( 1, 32);
        rbSeqFirst = false;
      }

      /* use the main bitstream buffer for storing the marshalled picture */
      m_pcEntropyCoder->setBitstream(pcBitstreamOut);

      uiStartCUAddrSliceIdx = 0;
      uiStartCUAddrSlice    = 0;
      pcBitstreamOut->allocateMemoryForSliceLocations( pcPic->getPicSym()->getNumberOfCUsInFrame() ); // Assuming number of slices <= number of LCU. Needs to be changed for sub-LCU slice coding.
      pcBitstreamOut->setSliceCount( 0 );                                      // intialize number of slices to zero, used while converting RBSP to NALU

      uiStartCUAddrEntropySliceIdx = 0;
      uiStartCUAddrEntropySlice    = 0;
      uiNextCUAddr                 = 0;
      pcSlice = pcPic->getSlice(uiStartCUAddrSliceIdx);
      while (uiNextCUAddr < pcPic->getPicSym()->getNumberOfCUsInFrame()) // Iterate over all slices
      {
        pcSlice->setNextSlice       ( false );
        pcSlice->setNextEntropySlice( false );
        if (uiNextCUAddr == m_uiStoredStartCUAddrForEncodingSlice[uiStartCUAddrSliceIdx])
        {
          pcSlice = pcPic->getSlice(uiStartCUAddrSliceIdx);
          pcPic->setCurrSliceIdx(uiStartCUAddrSliceIdx);
          m_pcSliceEncoder->setSliceIdx(uiStartCUAddrSliceIdx);
          assert(uiStartCUAddrSliceIdx == pcSlice->getSliceIdx());
          // Reconstruction slice
          pcSlice->setSliceCurStartCUAddr( uiNextCUAddr );  // to be used in encodeSlice() + context restriction
          pcSlice->setSliceCurEndCUAddr  ( m_uiStoredStartCUAddrForEncodingSlice[uiStartCUAddrSliceIdx+1 ] );
          // Entropy slice
          pcSlice->setEntropySliceCurStartCUAddr( uiNextCUAddr );  // to be used in encodeSlice() + context restriction
          pcSlice->setEntropySliceCurEndCUAddr  ( m_uiStoredStartCUAddrForEncodingEntropySlice[uiStartCUAddrEntropySliceIdx+1 ] );

          pcSlice->setNextSlice       ( true );

          uiStartCUAddrSliceIdx++;
          uiStartCUAddrEntropySliceIdx++;
        }
        else if (uiNextCUAddr == m_uiStoredStartCUAddrForEncodingEntropySlice[uiStartCUAddrEntropySliceIdx])
        {
          // Entropy slice
          pcSlice->setEntropySliceCurStartCUAddr( uiNextCUAddr );  // to be used in encodeSlice() + context restriction
          pcSlice->setEntropySliceCurEndCUAddr  ( m_uiStoredStartCUAddrForEncodingEntropySlice[uiStartCUAddrEntropySliceIdx+1 ] );

          pcSlice->setNextEntropySlice( true );

          uiStartCUAddrEntropySliceIdx++;
        }

        // Get ready for writing slice header (other than the first one in the picture)
        if (uiNextCUAddr!=0)
        {
          m_pcEntropyCoder->setEntropyCoder   ( m_pcCavlcCoder, pcSlice );
          m_pcEntropyCoder->setBitstream      ( pcBitstreamOut          );
          m_pcEntropyCoder->resetEntropy      ();
        }

      // write SliceHeader
      m_pcEntropyCoder->encodeSliceHeader ( pcSlice                 );

      // is it needed?
      if ( pcSlice->getSymbolMode() )
      {
        m_pcSbacCoder->init( (TEncBinIf*)m_pcBinCABAC );
        m_pcEntropyCoder->setEntropyCoder ( m_pcSbacCoder, pcSlice );
        m_pcEntropyCoder->resetEntropy    ();
      }

        if (uiNextCUAddr==0)  // Compute ALF params and write only for first slice header
        {
          // adaptive loop filter
#if MTK_SAO
          if ( pcSlice->getSPS()->getUseALF() || (pcSlice->getSPS()->getUseSAO()) )
#else
          if ( pcSlice->getSPS()->getUseALF())
#endif
          {
            ALFParam cAlfParam;
#if TSB_ALF_HEADER
            m_pcAdaptiveLoopFilter->setNumCUsInFrame(pcPic);
#endif
            m_pcAdaptiveLoopFilter->allocALFParam(&cAlfParam);

            // set entropy coder for RD
            if ( pcSlice->getSymbolMode() )
            {
              m_pcEntropyCoder->setEntropyCoder ( m_pcEncTop->getRDGoOnSbacCoder(), pcSlice );
            }
            else
            {
              m_pcEntropyCoder->setEntropyCoder ( m_pcCavlcCoder, pcSlice );
            }
            m_pcEntropyCoder->resetEntropy    ();
            m_pcEntropyCoder->setBitstream    ( m_pcBitCounter );

            m_pcAdaptiveLoopFilter->startALFEnc(pcPic, m_pcEntropyCoder );
#if MTK_SAO  // PostDF
            {
              if (pcSlice->getSPS()->getUseSAO())
              {
                m_pcSAO->startSaoEnc(pcPic, m_pcEntropyCoder, m_pcEncTop->getRDSbacCoder(), m_pcCfg->getUseSBACRD() ?  m_pcEncTop->getRDGoOnSbacCoder() : NULL);
                m_pcSAO->SAOProcess(pcPic->getSlice(0)->getLambda());
                m_pcSAO->copyQaoData(&cSaoParam);
                m_pcSAO->endSaoEnc();
              }
            }
#endif
            UInt uiMaxAlfCtrlDepth;

            UInt64 uiDist, uiBits;
#if MTK_SAO
            if ( pcSlice->getSPS()->getUseALF())
#endif
              m_pcAdaptiveLoopFilter->ALFProcess( &cAlfParam, pcPic->getSlice(0)->getLambda(), uiDist, uiBits, uiMaxAlfCtrlDepth );
#if MTK_SAO
            else
              cAlfParam.cu_control_flag = 0;
#endif
            m_pcAdaptiveLoopFilter->endALFEnc();

            // set entropy coder for writing
            m_pcSbacCoder->init( (TEncBinIf*)m_pcBinCABAC );
            if ( pcSlice->getSymbolMode() )
            {
              m_pcEntropyCoder->setEntropyCoder ( m_pcSbacCoder, pcSlice );
            }
            else
            {
              m_pcEntropyCoder->setEntropyCoder ( m_pcCavlcCoder, pcSlice );
            }
            m_pcEntropyCoder->resetEntropy    ();
            m_pcEntropyCoder->setBitstream    ( pcBitstreamOut );
            if (cAlfParam.cu_control_flag)
            {
              m_pcEntropyCoder->setAlfCtrl( true );
              m_pcEntropyCoder->setMaxAlfCtrlDepth(uiMaxAlfCtrlDepth);
              if (pcSlice->getSymbolMode() == 0)
              {
                m_pcCavlcCoder->setAlfCtrl(true);
                m_pcCavlcCoder->setMaxAlfCtrlDepth(uiMaxAlfCtrlDepth); //D0201
              }
            }
            else
            {
              m_pcEntropyCoder->setAlfCtrl(false);
            }
#if MTK_SAO
            if (pcSlice->getSPS()->getUseSAO())
            {
              m_pcEntropyCoder->encodeSaoParam(&cSaoParam);
            }
            if (pcSlice->getSPS()->getUseALF())
#endif
            m_pcEntropyCoder->encodeAlfParam(&cAlfParam);

#if TSB_ALF_HEADER
            if(cAlfParam.cu_control_flag)
            {
              m_pcEntropyCoder->encodeAlfCtrlParam(&cAlfParam);
            }
#endif
            m_pcAdaptiveLoopFilter->freeALFParam(&cAlfParam);
          }
        }

        // File writing
        m_pcSliceEncoder->encodeSlice( pcPic, pcBitstreamOut );

        //  End of bitstream & byte align
        pcBitstreamOut->write( 1, 1 );
        pcBitstreamOut->writeAlignZero();

        UInt uiBoundingAddrSlice, uiBoundingAddrEntropySlice;
        uiBoundingAddrSlice        = m_uiStoredStartCUAddrForEncodingSlice[uiStartCUAddrSliceIdx];
        uiBoundingAddrEntropySlice = m_uiStoredStartCUAddrForEncodingEntropySlice[uiStartCUAddrEntropySliceIdx];
        uiNextCUAddr               = min(uiBoundingAddrSlice, uiBoundingAddrEntropySlice);
        if (uiNextCUAddr < pcPic->getPicSym()->getNumberOfCUsInFrame())   // if more slices to be encoded insert start code
        {
          UInt uiSliceCount = pcBitstreamOut->getSliceCount();
          pcBitstreamOut->setSliceByteLocation( uiSliceCount, (pcBitstreamOut->getNumberOfWrittenBits()>>3) );
          pcBitstreamOut->setSliceCount( uiSliceCount+1 );
          pcBitstreamOut->write( 1, 32);
        }
      } // end iteration over slices


#if MTK_NONCROSS_INLOOP_FILTER
      if(pcSlice->getSPS()->getUseALF())
      {
        if(m_pcAdaptiveLoopFilter->getUseNonCrossAlf())
          m_pcAdaptiveLoopFilter->destroySlice();
      }
#endif


      pcBitstreamOut->flushBuffer();
      pcBitstreamOut->convertRBSPToPayload(0);

/*#if AMVP_BUFFERCOMPRESS
      pcPic->compressMotion(); // moved to end of access unit
#endif */
      pcBitstreamOut->freeMemoryAllocatedForSliceLocations();

      //-- For time output for each slice
      Double dEncTime = (double)(clock()-iBeforeTime) / CLOCKS_PER_SEC;

      xCalculateAddPSNR( pcPic, pcPic->getPicYuvRec(), pcBitstreamOut->getNumberOfWrittenBits(), dEncTime );

#if FIXED_ROUNDING_FRAME_MEMORY
      pcPic->getPicYuvRec()->xFixedRoundingPic();
#endif

      if (m_pcCfg->getPictureDigestEnabled()) {
        /* calculate MD5sum for entire reconstructed picture */
        SEIpictureDigest sei_recon_picture_digest;
        sei_recon_picture_digest.method = SEIpictureDigest::MD5;
        calcMD5(*pcPic->getPicYuvRec(), sei_recon_picture_digest.digest);
        printf("[MD5:%s] ", digestToString(sei_recon_picture_digest.digest));

        TComBitstream seiBs;
        seiBs.create(1024);
        /* write the SEI messages */
        m_pcEntropyCoder->setEntropyCoder(m_pcCavlcCoder, pcSlice);
        m_pcEntropyCoder->setBitstream(&seiBs);
        m_pcEntropyCoder->encodeSEI(sei_recon_picture_digest);
        /* and trailing bits */
        seiBs.write(1, 1);
        seiBs.writeAlignZero();
        seiBs.flushBuffer();
        seiBs.convertRBSPToPayload(0);

        /* append the SEI message after any SPS/PPS */
        /* the following loop is a work around current limitations in
         * TComBitstream that won't be fixed before HM-3.0 */
        UChar *seiData = reinterpret_cast<UChar *>(seiBs.getStartStream());
        for (Int i = 0; i < seiBs.getNumberOfWrittenBits()/8; i++)
        {
          bs_SPS_PPS_SEI.write(seiData[i], 8);
        }
        bs_SPS_PPS_SEI.write(1, 32);
        seiBs.destroy();
      }

      /* insert the bs_SPS_PPS_SEI before the pcBitstreamOut */
      bs_SPS_PPS_SEI.flushBuffer();
      pcBitstreamOut->insertAt(bs_SPS_PPS_SEI, 0);

      bs_SPS_PPS_SEI.destroy();
      pcPic->getPicYuvRec()->copyToPic(pcPicYuvRecOut);

      pcPic->setReconMark   ( true );

}

Void TEncPic::preLoopFilterPicAll( TComPic* pcPic, UInt64& ruiDist, UInt64& ruiBits )
{
  TComSlice* pcSlice = pcPic->getSlice(pcPic->getCurrSliceIdx());
  Bool bCalcDist = false;

  m_pcLoopFilter->setCfg(pcSlice->getLoopFilterDisable(), m_pcCfg->getLoopFilterAlphaC0Offget(), m_pcCfg->getLoopFilterBetaOffget());
  m_pcLoopFilter->loopFilterPic( pcPic );

  m_pcEntropyCoder->setEntropyCoder ( m_pcEncTop->getRDGoOnSbacCoder(), pcSlice );
  m_pcEntropyCoder->resetEntropy    ();
  m_pcEntropyCoder->setBitstream    ( m_pcBitCounter );

  // Adaptive Loop filter
  if( pcSlice->getSPS()->getUseALF() )
  {
    ALFParam cAlfParam;
#if TSB_ALF_HEADER
    m_pcAdaptiveLoopFilter->setNumCUsInFrame(pcPic);
#endif
    m_pcAdaptiveLoopFilter->allocALFParam(&cAlfParam);

    m_pcAdaptiveLoopFilter->startALFEnc(pcPic, m_pcEntropyCoder);

    UInt uiMaxAlfCtrlDepth;
    m_pcAdaptiveLoopFilter->ALFProcess(&cAlfParam, pcSlice->getLambda(), ruiDist, ruiBits, uiMaxAlfCtrlDepth );
    m_pcAdaptiveLoopFilter->endALFEnc();
    m_pcAdaptiveLoopFilter->freeALFParam(&cAlfParam);
  }

  m_pcEntropyCoder->resetEntropy    ();
  ruiBits += m_pcEntropyCoder->getNumberOfWrittenBits();

  if (!bCalcDist)
    ruiDist = xFindDistortionFrame(pcPic->getPicYuvOrg(), pcPic->getPicYuvRec());
}

// ====================================================================================================================
// Protected member functions
// ====================================================================================================================

UInt64 TEncPic::xFindDistortionFrame (TComPicYuv* pcPic0, TComPicYuv* pcPic1)
{
  Int     x, y;
  Pel*  pSrc0   = pcPic0 ->getLumaAddr();
  Pel*  pSrc1   = pcPic1 ->getLumaAddr();
#if IBDI_DISTORTION
  Int  iShift = g_uiBitIncrement;
  Int  iOffset = 1<<(g_uiBitIncrement-1);
#else
  UInt  uiShift = g_uiBitIncrement<<1;
#endif
  Int   iTemp;

  Int   iStride = pcPic0->getStride();
  Int   iWidth  = pcPic0->getWidth();
  Int   iHeight = pcPic0->getHeight();

  UInt64  uiTotalDiff = 0;

  for( y = 0; y < iHeight; y++ )
  {
    for( x = 0; x < iWidth; x++ )
    {
#if IBDI_DISTORTION
      iTemp = ((pSrc0[x]+iOffset)>>iShift) - ((pSrc1[x]+iOffset)>>iShift); uiTotalDiff += iTemp * iTemp;
#else
      iTemp = pSrc0[x] - pSrc1[x]; uiTotalDiff += (iTemp*iTemp) >> uiShift;
#endif
    }
    pSrc0 += iStride;
    pSrc1 += iStride;
  }

  iHeight >>= 1;
  iWidth  >>= 1;
  iStride >>= 1;

  pSrc0  = pcPic0->getCbAddr();
  pSrc1  = pcPic1->getCbAddr();

  for( y = 0; y < iHeight; y++ )
  {
    for( x = 0; x < iWidth; x++ )
    {
#if IBDI_DISTORTION
      iTemp = ((pSrc0[x]+iOffset)>>iShift) - ((pSrc1[x]+iOffset)>>iShift); uiTotalDiff += iTemp * iTemp;
#else
      iTemp = pSrc0[x] - pSrc1[x]; uiTotalDiff += (iTemp*iTemp) >> uiShift;
#endif
    }
    pSrc0 += iStride;
    pSrc1 += iStride;
  }

  pSrc0  = pcPic0->getCrAddr();
  pSrc1  = pcPic1->getCrAddr();

  for( y = 0; y < iHeight; y++ )
  {
    for( x = 0; x < iWidth; x++ )
    {
#if IBDI_DISTORTION
      iTemp = ((pSrc0[x]+iOffset)>>iShift) - ((pSrc1[x]+iOffset)>>iShift); uiTotalDiff += iTemp * iTemp;
#else
      iTemp = pSrc0[x] - pSrc1[x]; uiTotalDiff += (iTemp*iTemp) >> uiShift;
#endif
    }
    pSrc0 += iStride;
    pSrc1 += iStride;
  }

  return uiTotalDiff;
}

Void TEncPic::xCalculateAddPSNR( TComPic* pcPic, TComPicYuv* pcPicD, UInt uibits, Double dEncTime )
{
  Int     x, y;
  UInt64 uiSSDY  = 0;
  UInt64 uiSSDU  = 0;
  UInt64 uiSSDV  = 0;

  Double  dYPSNR  = 0.0;
  Double  dUPSNR  = 0.0;
  Double  dVPSNR  = 0.0;

  //===== calculate PSNR =====
  Pel*  pOrg    = pcPic ->getPicYuvOrg()->getLumaAddr();
  Pel*  pRec    = pcPicD->getLumaAddr();
  Int   iStride = pcPicD->getStride();

  Int   iWidth;
  Int   iHeight;

  iWidth  = pcPicD->getWidth () - m_pcEncTop->getPad(0);
  iHeight = pcPicD->getHeight() - m_pcEncTop->getPad(1);

  Int   iSize   = iWidth*iHeight;

  UInt   maxval = 255 * (1<<(g_uiBitDepth + g_uiBitIncrement -8));
  Double fRefValueY = (double) maxval * maxval * iSize;
  Double fRefValueC = fRefValueY / 4.0;

  for( y = 0; y < iHeight; y++ )
  {
    for( x = 0; x < iWidth; x++ )
    {
      Int iDiff = (Int)( pOrg[x] - pRec[x] );
      uiSSDY   += iDiff * iDiff;
    }
    pOrg += iStride;
    pRec += iStride;
  }

#if HHI_VSO
  if ( m_pcRdCost->getUseRenModel() )
  {
    TRenModel*  pcRenModel = m_pcEncTop->getEncTop()->getRenModel();
    Int64 iDistVSOY, iDistVSOU, iDistVSOV;
    pcRenModel->getTotalSSE( iDistVSOY, iDistVSOU, iDistVSOV );
    dYPSNR = ( iDistVSOY ? 10.0 * log10( fRefValueY / (Double) iDistVSOY ) : 99.99 );
    dUPSNR = ( iDistVSOU ? 10.0 * log10( fRefValueC / (Double) iDistVSOU ) : 99.99 );
    dVPSNR = ( iDistVSOV ? 10.0 * log10( fRefValueC / (Double) iDistVSOV ) : 99.99 );
  }
  else
#endif
  {
  iHeight >>= 1;
  iWidth  >>= 1;
  iStride >>= 1;
  pOrg  = pcPic ->getPicYuvOrg()->getCbAddr();
  pRec  = pcPicD->getCbAddr();

  for( y = 0; y < iHeight; y++ )
  {
    for( x = 0; x < iWidth; x++ )
    {
      Int iDiff = (Int)( pOrg[x] - pRec[x] );
      uiSSDU   += iDiff * iDiff;
    }
    pOrg += iStride;
    pRec += iStride;
  }

  pOrg  = pcPic ->getPicYuvOrg()->getCrAddr();
  pRec  = pcPicD->getCrAddr();

  for( y = 0; y < iHeight; y++ )
  {
    for( x = 0; x < iWidth; x++ )
    {
      Int iDiff = (Int)( pOrg[x] - pRec[x] );
      uiSSDV   += iDiff * iDiff;
    }
    pOrg += iStride;
    pRec += iStride;
  }
  dYPSNR            = ( uiSSDY ? 10.0 * log10( fRefValueY / (Double)uiSSDY ) : 99.99 );
  dUPSNR            = ( uiSSDU ? 10.0 * log10( fRefValueC / (Double)uiSSDU ) : 99.99 );
  dVPSNR            = ( uiSSDV ? 10.0 * log10( fRefValueC / (Double)uiSSDV ) : 99.99 );
  }
  // fix: total bits should consider slice size bits (32bit)
  uibits += 32;

#if RVM_VCEGAM10
  m_vRVM_RP.push_back( uibits );
#endif

  //===== add PSNR =====
  m_pcEncTop->m_cAnalyzeAll.addResult (dYPSNR, dUPSNR, dVPSNR, (Double)uibits);
  TComSlice*  pcSlice = pcPic->getSlice(0);
  if (pcSlice->isIntra())
  {
    m_pcEncTop->m_cAnalyzeI.addResult (dYPSNR, dUPSNR, dVPSNR, (Double)uibits);
  }
  if (pcSlice->isInterP())
  {
    m_pcEncTop->m_cAnalyzeP.addResult (dYPSNR, dUPSNR, dVPSNR, (Double)uibits);
  }
  if (pcSlice->isInterB())
  {
    m_pcEncTop->m_cAnalyzeB.addResult (dYPSNR, dUPSNR, dVPSNR, (Double)uibits);
  }
  if(pcPic->getViewIdx()!=-1)
  {
    if(! m_pcEncTop->isDepthCoder())
    {
      printf("\nView \t\t%2d\t POC %4d ( %c-SLICE, QP %d ) %10d bits ",
      pcSlice->getViewIdx(),
      pcSlice->getPOC(),
      pcSlice->isIntra() ? 'I' : pcSlice->isInterP() ? 'P' : 'B',
      pcSlice->getSliceQp(),
      uibits );
    }
    else
    {
      printf("\nDepth View \t%2d\t POC %4d ( %c-SLICE, QP %d ) %10d bits ",
            pcSlice->getViewIdx(),
            pcSlice->getPOC(),
            pcSlice->isIntra() ? 'I' : pcSlice->isInterP() ? 'P' : 'B',
            pcSlice->getSliceQp(),
            uibits );
    }
  }
  else
  {
    printf("\nPOC %4d ( %c-SLICE, QP %d ) %10d bits ",
           pcSlice->getPOC(),
           pcSlice->isIntra() ? 'I' : pcSlice->isInterP() ? 'P' : 'B',
           pcSlice->getSliceQp(),
           uibits );
  }

  printf( "[Y %6.4lf dB    U %6.4lf dB    V %6.4lf dB]  ", dYPSNR, dUPSNR, dVPSNR );
  printf ("[ET %5.0f ] ", dEncTime );

  for (Int iRefList = 0; iRefList < 2; iRefList++)
  {
    printf ("[L%d ", iRefList);
    for (Int iRefIndex = 0; iRefIndex < pcSlice->getNumRefIdx(RefPicList(iRefList)); iRefIndex++)
    {
      if( pcSlice->getViewIdx() != pcSlice->getRefViewIdx( RefPicList(iRefList), iRefIndex ) )
      {
        printf( "V%d", pcSlice->getRefViewIdx( RefPicList(iRefList), iRefIndex ) );
        if( pcSlice->getPOC() != pcSlice->getRefPOC( RefPicList(iRefList), iRefIndex ) )
          printf( "(%d)", pcSlice->getRefPOC( RefPicList(iRefList), iRefIndex ) );
        printf( " " );
      }
      else
        printf ("%d ", pcSlice->getRefPOC(RefPicList(iRefList), iRefIndex));
    }
    printf ("] ");
  }
#if DCM_COMB_LIST
  if(pcSlice->getNumRefIdx(REF_PIC_LIST_C)>0 && !pcSlice->getNoBackPredFlag())
  {
    printf ("[LC ");
    for (Int iRefIndex = 0; iRefIndex < pcSlice->getNumRefIdx(REF_PIC_LIST_C); iRefIndex++)
    {
      printf ("%d ", pcSlice->getRefPOC((RefPicList)pcSlice->getListIdFromIdxOfLC(iRefIndex), pcSlice->getRefIdxFromIdxOfLC(iRefIndex)));
    }
    printf ("] ");
  }
#endif

  fflush(stdout);
}

#if DCM_DECODING_REFRESH
/** Function for deciding the nal_unit_type.
 * \param uiPOCCurr POC of the current picture
 * \returns the nal_unit type of the picture
 * This function checks the configuration and returns the appropriate nal_unit_type for the picture.
 */
NalUnitType TEncPic::getNalUnitType(UInt uiPOCCurr)
{
  if (uiPOCCurr == 0)
  {
    return NAL_UNIT_CODED_SLICE_IDR;
  }
#if 0
  if (uiPOCCurr % m_pcCfg->getIntraPeriod() == 0)
  {
    if (m_pcCfg->getDecodingRefreshType() == 1)
    {
      return NAL_UNIT_CODED_SLICE_CDR;
    }
    else if (m_pcCfg->getDecodingRefreshType() == 2)
    {
      return NAL_UNIT_CODED_SLICE_IDR;
    }
  }
#endif
  return NAL_UNIT_CODED_SLICE;
}
#endif

#if RVM_VCEGAM10
Double TEncPic::xCalculateRVM()
{
  Double dRVM = 0;

  //if( m_pcCfg->getGOPSize() == 1 && m_pcCfg->getIntraPeriod() != 1 && m_pcCfg->getFrameToBeEncoded() > RVM_VCEGAM10_M * 2 )
  {
    // calculate RVM only for lowdelay configurations
    std::vector<Double> vRL , vB;
    size_t N = m_vRVM_RP.size();
    vRL.resize( N );
    vB.resize( N );

    Int i;
    Double dRavg = 0 , dBavg = 0;
    vB[RVM_VCEGAM10_M] = 0;
    for( i = RVM_VCEGAM10_M + 1 ; i < N - RVM_VCEGAM10_M + 1 ; i++ )
    {
      vRL[i] = 0;
      for( Int j = i - RVM_VCEGAM10_M ; j <= i + RVM_VCEGAM10_M - 1 ; j++ )
        vRL[i] += m_vRVM_RP[j];
      vRL[i] /= ( 2 * RVM_VCEGAM10_M );
      vB[i] = vB[i-1] + m_vRVM_RP[i] - vRL[i];
      dRavg += m_vRVM_RP[i];
      dBavg += vB[i];
    }

    dRavg /= ( N - 2 * RVM_VCEGAM10_M );
    dBavg /= ( N - 2 * RVM_VCEGAM10_M );

    double dSigamB = 0;
    for( i = RVM_VCEGAM10_M + 1 ; i < N - RVM_VCEGAM10_M + 1 ; i++ )
    {
      Double tmp = vB[i] - dBavg;
      dSigamB += tmp * tmp;
    }
    dSigamB = sqrt( dSigamB / ( N - 2 * RVM_VCEGAM10_M ) );

    double f = sqrt( 12.0 * ( RVM_VCEGAM10_M - 1 ) / ( RVM_VCEGAM10_M + 1 ) );

    dRVM = dSigamB / dRavg * f;
  }

  return( dRVM );
}
#endif

