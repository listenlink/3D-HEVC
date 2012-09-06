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

/** \file     TEncTop.cpp
    \brief    encoder class
*/

#include "TLibCommon/CommonDef.h"
#include "TEncTop.h"
#include "TEncPic.h"
#include "../../App/TAppEncoder/TAppEncTop.h"
#if FAST_BIT_EST
#include "TLibCommon/ContextModel.h"
#endif

//! \ingroup TLibEncoder
//! \{

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

  m_iMaxRefPicNum     = 0;

#if FAST_BIT_EST
  ContextModel::buildNextStateTable();
#endif

  m_pcSbacCoders           = NULL;
  m_pcBinCoderCABACs       = NULL;
  m_ppppcRDSbacCoders      = NULL;
  m_ppppcBinCodersCABAC    = NULL;
  m_pcRDGoOnSbacCoders     = NULL;
  m_pcRDGoOnBinCodersCABAC = NULL;
  m_pcBitCounters          = NULL;
  m_pcRdCosts              = NULL;
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
#if FIX_INIT_ROM
  if( m_viewId == 0 && m_isDepth == false )
  {
#endif
    initROM();
#if FIX_INIT_ROM
  }
#endif


  // create processing unit classes
  m_cGOPEncoder.        create( getSourceWidth(), getSourceHeight(), g_uiMaxCUWidth, g_uiMaxCUHeight );
  m_cSliceEncoder.      create( getSourceWidth(), getSourceHeight(), g_uiMaxCUWidth, g_uiMaxCUHeight, g_uiMaxCUDepth );
  m_cCuEncoder.         create( g_uiMaxCUDepth, g_uiMaxCUWidth, g_uiMaxCUHeight );
  if (m_bUseSAO)
  {
#if SAO_UNIT_INTERLEAVING
    m_cEncSAO.setSaoInterleavingFlag(getSaoInterleavingFlag());
    m_cEncSAO.setMaxNumOffsetsPerPic(getMaxNumOffsetsPerPic());
#endif
    m_cEncSAO.create( getSourceWidth(), getSourceHeight(), g_uiMaxCUWidth, g_uiMaxCUHeight, g_uiMaxCUDepth );
    m_cEncSAO.createEncBuffer();
  }
#if ADAPTIVE_QP_SELECTION
  if (m_bUseAdaptQpSelect)
  {
    m_cTrQuant.initSliceQpDelta();
  }
#endif
  m_cAdaptiveLoopFilter.create( getSourceWidth(), getSourceHeight(), g_uiMaxCUWidth, g_uiMaxCUHeight, g_uiMaxCUDepth );
  m_cLoopFilter.        create( g_uiMaxCUDepth );
  
#if DEPTH_MAP_GENERATION
  m_cDepthMapGenerator. create( false, getSourceWidth(), getSourceHeight(), g_uiMaxCUDepth, g_uiMaxCUWidth, g_uiMaxCUHeight, g_uiBitDepth + g_uiBitIncrement, PDM_SUB_SAMP_EXP_X(m_uiPredDepthMapGeneration), PDM_SUB_SAMP_EXP_Y(m_uiPredDepthMapGeneration) );
#endif
#if HHI_INTER_VIEW_RESIDUAL_PRED
  m_cResidualGenerator. create( false, getSourceWidth(), getSourceHeight(), g_uiMaxCUDepth, g_uiMaxCUWidth, g_uiMaxCUHeight, g_uiBitDepth + g_uiBitIncrement );
#endif

  if(m_bUseALF)
  {
#if LCU_SYNTAX_ALF
    m_cAdaptiveLoopFilter.setAlfCoefInSlice(m_bALFParamInSlice);
    m_cAdaptiveLoopFilter.createAlfGlobalBuffers();
#else
    m_cAdaptiveLoopFilter.setGOPSize( getGOPSize() );
    m_cAdaptiveLoopFilter.createAlfGlobalBuffers(m_iALFEncodePassReduction);
#endif
  }

  if(m_bUseSAO || m_bUseALF)
  {
    m_vAPS.reserve(MAX_NUM_SUPPORTED_APS);
  }

  // if SBAC-based RD optimization is used
  if( m_bUseSBACRD )
  {
    m_pppcRDSbacCoder = new TEncSbac** [g_uiMaxCUDepth+1];
#if FAST_BIT_EST
    m_pppcBinCoderCABAC = new TEncBinCABACCounter** [g_uiMaxCUDepth+1];
#else
    m_pppcBinCoderCABAC = new TEncBinCABAC** [g_uiMaxCUDepth+1];
#endif
    
    for ( Int iDepth = 0; iDepth < g_uiMaxCUDepth+1; iDepth++ )
    {
      m_pppcRDSbacCoder[iDepth] = new TEncSbac* [CI_NUM];
#if FAST_BIT_EST
      m_pppcBinCoderCABAC[iDepth] = new TEncBinCABACCounter* [CI_NUM];
#else
      m_pppcBinCoderCABAC[iDepth] = new TEncBinCABAC* [CI_NUM];
#endif
      
      for (Int iCIIdx = 0; iCIIdx < CI_NUM; iCIIdx ++ )
      {
        m_pppcRDSbacCoder[iDepth][iCIIdx] = new TEncSbac;
#if FAST_BIT_EST
        m_pppcBinCoderCABAC [iDepth][iCIIdx] = new TEncBinCABACCounter;
#else
        m_pppcBinCoderCABAC [iDepth][iCIIdx] = new TEncBinCABAC;
#endif
        m_pppcRDSbacCoder   [iDepth][iCIIdx]->init( m_pppcBinCoderCABAC [iDepth][iCIIdx] );
      }
    }
  }
  m_pcTAppEncTop = NULL;
#if HHI_DMM_WEDGE_INTRA || HHI_DMM_PRED_TEX
  if( g_aacWedgeLists.empty() && m_bUseDMM )
  {
    initWedgeLists();
  }
#endif
}

/**
 - Allocate coders required for wavefront for the nominated number of substreams.
 .
 \param iNumSubstreams Determines how much information to allocate.
 */
Void TEncTop::createWPPCoders(Int iNumSubstreams)
{
  if (m_pcSbacCoders != NULL)
    return; // already generated.

  m_iNumSubstreams         = iNumSubstreams;
  m_pcSbacCoders           = new TEncSbac       [iNumSubstreams];
  m_pcBinCoderCABACs       = new TEncBinCABAC   [iNumSubstreams];
  m_pcRDGoOnSbacCoders     = new TEncSbac       [iNumSubstreams];
  m_pcRDGoOnBinCodersCABAC = new TEncBinCABAC   [iNumSubstreams];
  m_pcBitCounters          = new TComBitCounter [iNumSubstreams];
  m_pcRdCosts              = new TComRdCost     [iNumSubstreams];

  for ( UInt ui = 0 ; ui < iNumSubstreams; ui++ )
  {
    m_pcRDGoOnSbacCoders[ui].init( &m_pcRDGoOnBinCodersCABAC[ui] );
    m_pcSbacCoders[ui].init( &m_pcBinCoderCABACs[ui] );
  }
  if( m_bUseSBACRD )
  {
    m_ppppcRDSbacCoders      = new TEncSbac***    [iNumSubstreams];
    m_ppppcBinCodersCABAC    = new TEncBinCABAC***[iNumSubstreams];
    for ( UInt ui = 0 ; ui < iNumSubstreams ; ui++ )
    {
      m_ppppcRDSbacCoders[ui]  = new TEncSbac** [g_uiMaxCUDepth+1];
      m_ppppcBinCodersCABAC[ui]= new TEncBinCABAC** [g_uiMaxCUDepth+1];
      
      for ( Int iDepth = 0; iDepth < g_uiMaxCUDepth+1; iDepth++ )
      {
        m_ppppcRDSbacCoders[ui][iDepth]  = new TEncSbac*     [CI_NUM];
        m_ppppcBinCodersCABAC[ui][iDepth]= new TEncBinCABAC* [CI_NUM];

        for (Int iCIIdx = 0; iCIIdx < CI_NUM; iCIIdx ++ )
        {
          m_ppppcRDSbacCoders  [ui][iDepth][iCIIdx] = new TEncSbac;
          m_ppppcBinCodersCABAC[ui][iDepth][iCIIdx] = new TEncBinCABAC;
          m_ppppcRDSbacCoders  [ui][iDepth][iCIIdx]->init( m_ppppcBinCodersCABAC[ui][iDepth][iCIIdx] );
        }
      }
    }
  }
}

Void TEncTop::destroy ()
{
  if(m_bUseALF)
  {
    m_cAdaptiveLoopFilter.destroyAlfGlobalBuffers();
  }

  for(Int i=0; i< m_vAPS.size(); i++)
  {
    TComAPS& cAPS = m_vAPS[i];
    m_cGOPEncoder.freeAPS(&cAPS, &m_cSPS);
  }

  // destroy processing unit classes
  m_cGOPEncoder.        destroy();
  m_cSliceEncoder.      destroy();
  m_cCuEncoder.         destroy();
  if (m_cSPS.getUseSAO())
  {
    m_cEncSAO.destroy();
    m_cEncSAO.destroyEncBuffer();
  }
  m_cAdaptiveLoopFilter.destroy();
  m_cLoopFilter.        destroy();

#if DEPTH_MAP_GENERATION
  m_cDepthMapGenerator. destroy();
#endif
#if HHI_INTER_VIEW_RESIDUAL_PRED
  m_cResidualGenerator. destroy();
#endif

  m_RPSList.            destroy();
  
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

    for ( UInt ui = 0; ui < m_iNumSubstreams; ui++ )
    {
      for ( iDepth = 0; iDepth < g_uiMaxCUDepth+1; iDepth++ )
      {
        for (Int iCIIdx = 0; iCIIdx < CI_NUM; iCIIdx ++ )
        {
          delete m_ppppcRDSbacCoders  [ui][iDepth][iCIIdx];
          delete m_ppppcBinCodersCABAC[ui][iDepth][iCIIdx];
        }
      }

      for ( iDepth = 0; iDepth < g_uiMaxCUDepth+1; iDepth++ )
      {
        delete [] m_ppppcRDSbacCoders  [ui][iDepth];
        delete [] m_ppppcBinCodersCABAC[ui][iDepth];
      }
      delete[] m_ppppcRDSbacCoders  [ui];
      delete[] m_ppppcBinCodersCABAC[ui];
    }
    delete[] m_ppppcRDSbacCoders;
    delete[] m_ppppcBinCodersCABAC;
  }
  delete[] m_pcSbacCoders;
  delete[] m_pcBinCoderCABACs;
  delete[] m_pcRDGoOnSbacCoders;  
  delete[] m_pcRDGoOnBinCodersCABAC;
  delete[] m_pcBitCounters;
  delete[] m_pcRdCosts;
  
  // destroy ROM
  if(m_viewId == 0 && m_isDepth == false)
  {
  destroyROM();
  }
  
  return;
}

Void TEncTop::init( TAppEncTop* pcTAppEncTop )
{
  UInt *aTable4=NULL, *aTable8=NULL;
  UInt* aTableLastPosVlcIndex=NULL; 
  // initialize SPS
  xInitSPS();
  
  // initialize PPS
  m_cPPS.setSPS(&m_cSPS);
#if RPS_IN_SPS
  m_cSPS.setRPSList(&m_RPSList);
#else
  m_cPPS.setRPSList(&m_RPSList);
#endif
  xInitPPS();
  xInitRPS();

  xInitSPSforInterViewRefs();
  xInitPPSforTiles();

  // initialize processing unit classes
  m_cGOPEncoder.  init( this );
  m_cSliceEncoder.init( this );
  m_cCuEncoder.   init( this );
  
  m_pcTAppEncTop = pcTAppEncTop;

#if DEPTH_MAP_GENERATION
#if VIDYO_VPS_INTEGRATION
  m_cDepthMapGenerator.init( (TComPrediction*)this->getPredSearch(), m_pcTAppEncTop->getVPSAccess(), m_pcTAppEncTop->getSPSAccess(), m_pcTAppEncTop->getAUPicAccess() );
#else
  m_cDepthMapGenerator.init( (TComPrediction*)this->getPredSearch(), m_pcTAppEncTop->getSPSAccess(), m_pcTAppEncTop->getAUPicAccess() );
#endif
#endif
#if HHI_INTER_VIEW_RESIDUAL_PRED
  m_cResidualGenerator.init( &m_cTrQuant, &m_cDepthMapGenerator );
#endif

  // initialize transform & quantization class
  m_pcCavlcCoder = getCavlcCoder();
  
  m_cTrQuant.init( g_uiMaxCUWidth, g_uiMaxCUHeight, 1 << m_uiQuadtreeTULog2MaxSize,
                  0,
                  aTable4, aTable8, 
                  aTableLastPosVlcIndex, m_bUseRDOQ, true 
#if ADAPTIVE_QP_SELECTION                  
                  , m_bUseAdaptQpSelect
#endif
                  );
  
  // initialize encoder search class
  m_cSearch.init( this, &m_cTrQuant, m_iSearchRange, m_bipredSearchRange, m_iFastSearch, 0, &m_cEntropyCoder, &m_cRdCost, getRDSbacCoder(), getRDGoOnSbacCoder() );

  if(m_bUseALF)
  {
    m_cAdaptiveLoopFilter.setALFEncodePassReduction( m_iALFEncodePassReduction );
    m_cAdaptiveLoopFilter.setALFMaxNumberFilters( m_iALFMaxNumberFilters );
#if LCU_SYNTAX_ALF
    m_cAdaptiveLoopFilter.initPicQuadTreePartition(m_bALFPicBasedEncode );   
#endif
  }

  m_iMaxRefPicNum = 0;
}

// ====================================================================================================================
// Public member functions
// ====================================================================================================================

Void TEncTop::initNewPic( TComPicYuv* pcPicYuvOrg, TComPicYuv* pcOrgPdmDepth )
{
  TComPic* pcPicCurr = NULL;

  // get original YUV
  xGetNewPicBuffer( pcPicCurr );
  pcPicYuvOrg->copyToPic( pcPicCurr->getPicYuvOrg() );

#if SONY_COLPIC_AVAILABILITY
  pcPicCurr->setViewOrderIdx(m_iViewOrderIdx);
#endif
  pcPicCurr->setScaleOffset( m_aaiCodedScale, m_aaiCodedOffset );

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

#if DEPTH_MAP_GENERATION
  // add extra pic buffers
  Bool  bNeedPrdDepthMapBuf = ( m_uiPredDepthMapGeneration > 0 );
  if( bNeedPrdDepthMapBuf && !pcPicCurr->getPredDepthMap() )
  {
    pcPicCurr->addPrdDepthMapBuffer( PDM_SUB_SAMP_EXP_X(m_uiPredDepthMapGeneration), PDM_SUB_SAMP_EXP_Y(m_uiPredDepthMapGeneration) );
  }
#endif

  // compute image characteristics
  if ( getUseAdaptiveQP() )
  {
    m_cPreanalyzer.xPreanalyze( dynamic_cast<TEncPic*>( pcPicCurr ) );
  }
}

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
Void TEncTop::encode( bool bEos, TComPicYuv* pcPicYuvOrg, TComList<TComPicYuv*>& rcListPicYuvRecOut, std::list<AccessUnit>& accessUnitsOut, Int& iNumEncoded, Int gopId )
{  

  if( gopId == 0)
  {
    m_cGOPEncoder.initGOP(m_iPOCLast, m_iNumPicRcvd, m_cListPic, rcListPicYuvRecOut, accessUnitsOut);  
  }

  {
    m_cGOPEncoder.compressPicInGOP(m_iPOCLast, m_iNumPicRcvd, m_cListPic, rcListPicYuvRecOut, accessUnitsOut, gopId );
  }
  
  if( gopId + 1 == m_cGOPEncoder.getGOPSize() )
  {
    iNumEncoded         = m_iNumPicRcvd;
    m_iNumPicRcvd       = 0;
    m_uiNumAllPicCoded += iNumEncoded;
  }
}


#if HHI_INTERVIEW_SKIP || HHI_INTER_VIEW_MOTION_PRED || HHI_INTER_VIEW_RESIDUAL_PRED
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
#endif

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
  
#if H0567_DPB_PARAMETERS_PER_TEMPORAL_LAYER
  if (m_cListPic.size() >= (UInt)(m_iGOPSize + getMaxDecPicBuffering(MAX_TLAYER-1) + 2) )
#else
  if (m_cListPic.size() >= (UInt)(m_iGOPSize + getMaxNumberOfReferencePictures() + 2) )
#endif
  {
    TComList<TComPic*>::iterator iterPic  = m_cListPic.begin();
    Int iSize = Int( m_cListPic.size() );
    for ( Int i = 0; i < iSize; i++ )
    {
      rpcPic = *(++iterPic);
      if(rpcPic->getSlice(0)->isReferenced() == false)
         break;
    }
  }
  else
  {
        if ( getUseAdaptiveQP() )
        {
          TEncPic* pcEPic = new TEncPic;
          pcEPic->create( m_iSourceWidth, m_iSourceHeight, g_uiMaxCUWidth, g_uiMaxCUHeight, g_uiMaxCUDepth, m_cPPS.getMaxCuDQPDepth()+1 );
          rpcPic = pcEPic;
        }
        else
        {
          rpcPic = new TComPic;
          rpcPic->create( m_iSourceWidth, m_iSourceHeight, g_uiMaxCUWidth, g_uiMaxCUHeight, g_uiMaxCUDepth );
        }
    m_cListPic.pushBack( rpcPic );
  }
#if HHI_INTERVIEW_SKIP
  if( m_bInterViewSkip )
  {
    rpcPic->addUsedPelsMapBuffer();
  }
#endif
  rpcPic->setReconMark (false);
  
  m_iPOCLast++;
  m_iNumPicRcvd++;
  
#if HHI_INTER_VIEW_MOTION_PRED
  if( m_uiMultiviewMvRegMode )
  {
    rpcPic->addOrgDepthMapBuffer();
  }
#endif
  
  rpcPic->getSlice(0)->setPOC( m_iPOCLast );
  // mark it should be extended
  rpcPic->getPicYuvRec()->setBorderExtension(false);
#if FIXES
  rpcPic->getPicYuvOrg()->setBorderExtension(false); 
#endif
}

Void TEncTop::xInitSPS()
{
  m_cSPS.setPicWidthInLumaSamples         ( m_iSourceWidth      );
  m_cSPS.setPicHeightInLumaSamples        ( m_iSourceHeight     );
#if PIC_CROPPING
  m_cSPS.setPicCroppingFlag( m_croppingMode!= 0 );
  if (m_croppingMode != 0)
  {
    m_cSPS.setPicCropLeftOffset( m_cropLeft );
    m_cSPS.setPicCropRightOffset( m_cropRight );
    m_cSPS.setPicCropTopOffset( m_cropTop );
    m_cSPS.setPicCropBottomOffset( m_cropBottom );
  }
#else
  m_cSPS.setPad           ( m_aiPad             );
#endif
  m_cSPS.setMaxCUWidth    ( g_uiMaxCUWidth      );
  m_cSPS.setMaxCUHeight   ( g_uiMaxCUHeight     );
  m_cSPS.setMaxCUDepth    ( g_uiMaxCUDepth      );
  m_cSPS.setMinTrDepth    ( 0                   );
  m_cSPS.setMaxTrDepth    ( 1                   );
  
#if !H0567_DPB_PARAMETERS_PER_TEMPORAL_LAYER
  m_cSPS.setMaxNumberOfReferencePictures(m_maxNumberOfReferencePictures);
  m_cSPS.setNumReorderFrames(m_numReorderFrames);
#endif
  m_cSPS.setPCMLog2MinSize (m_uiPCMLog2MinSize);
  m_cSPS.setUsePCM        ( m_usePCM           );
  m_cSPS.setPCMLog2MaxSize( m_pcmLog2MaxSize  );

  m_cSPS.setUseALF        ( m_bUseALF           );
#if LCU_SYNTAX_ALF
  if(m_bUseALF)
  {
    m_cSPS.setUseALFCoefInSlice(m_bALFParamInSlice);
  }
#endif
  
  m_cSPS.setQuadtreeTULog2MaxSize( m_uiQuadtreeTULog2MaxSize );
  m_cSPS.setQuadtreeTULog2MinSize( m_uiQuadtreeTULog2MinSize );
  m_cSPS.setQuadtreeTUMaxDepthInter( m_uiQuadtreeTUMaxDepthInter    );
  m_cSPS.setQuadtreeTUMaxDepthIntra( m_uiQuadtreeTUMaxDepthIntra    );
  
#if LOSSLESS_CODING
  m_cSPS.setUseLossless   ( m_useLossless  );
#endif
#if !PIC_CROPPING
  m_cSPS.setUsePAD        ( m_bUsePAD           );
#endif
  m_cSPS.setUseLMChroma   ( m_bUseLMChroma           );  
  
  m_cSPS.setMaxTrSize   ( 1 << m_uiQuadtreeTULog2MaxSize );
  
  m_cSPS.setUseLComb    ( m_bUseLComb           );
  m_cSPS.setLCMod       ( m_bLCMod   );
  m_cSPS.setUseNSQT( m_useNSQT );
  
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
  
  for (i = 0; i < g_uiMaxCUDepth-1; i++ )
  {
    m_cSPS.setAMPAcc( i, m_useAMP );
    //m_cSPS.setAMPAcc( i, 1 );
  }

  m_cSPS.setUseAMP ( m_useAMP );

  for (i = g_uiMaxCUDepth-1; i < g_uiMaxCUDepth; i++ )
  {
    m_cSPS.setAMPAcc(i, 0);
  }

  m_cSPS.setBitDepth    ( g_uiBitDepth        );
  m_cSPS.setBitIncrement( g_uiBitIncrement    );
#if H0736_AVC_STYLE_QP_RANGE
  m_cSPS.setQpBDOffsetY ( (Int)(6*(g_uiBitDepth + g_uiBitIncrement - 8)) );
  m_cSPS.setQpBDOffsetC ( (Int)(6*(g_uiBitDepth + g_uiBitIncrement - 8)) );
#endif

  m_cSPS.setLFCrossSliceBoundaryFlag( m_bLFCrossSliceBoundaryFlag );
  m_cSPS.setUseSAO( m_bUseSAO );

#if !H0566_TLA
  if ( m_bTLayering )
  {
    Int iMaxTLayers = 1;
    for ( i = 1; ; i++)
    {
      iMaxTLayers = i;
      if ( (m_iGOPSize >> i) == 0 ) 
      {
        break;
      }
    }
  
    m_cSPS.setMaxTLayers( (UInt)iMaxTLayers );

    Bool bTemporalIdNestingFlag = true;
    for ( i = 0; i < m_cSPS.getMaxTLayers()-1; i++ )
    {
      if ( !m_abTLayerSwitchingFlag[i] )
      {
        bTemporalIdNestingFlag = false;
        break;
      }
    }

    m_cSPS.setTemporalIdNestingFlag( bTemporalIdNestingFlag );
  }
  else
  {
    m_cSPS.setMaxTLayers( 1 );
    m_cSPS.setTemporalIdNestingFlag( false );
  }
#else
  m_cSPS.setMaxTLayers( m_maxTempLayer );
  m_cSPS.setTemporalIdNestingFlag( false );
#endif
#if H0567_DPB_PARAMETERS_PER_TEMPORAL_LAYER
  for ( i = 0; i < m_cSPS.getMaxTLayers(); i++ )
  {
    m_cSPS.setMaxDecPicBuffering(m_maxDecPicBuffering[i], i);
    m_cSPS.setNumReorderPics(m_numReorderPics[i], i);
  }
#endif
  m_cSPS.setPCMBitDepthLuma (g_uiPCMBitDepthLuma);
  m_cSPS.setPCMBitDepthChroma (g_uiPCMBitDepthChroma);
  m_cSPS.setPCMFilterDisableFlag  ( m_bPCMFilterDisableFlag );

  m_cSPS.setLFCrossTileBoundaryFlag( m_bLFCrossTileBoundaryFlag );
  m_cSPS.setUniformSpacingIdr( m_iUniformSpacingIdr );
#if !REMOVE_TILE_DEPENDENCE
  m_cSPS.setTileBoundaryIndependenceIdr( m_iTileBoundaryIndependenceIdr );
#endif
  m_cSPS.setNumColumnsMinus1( m_iNumColumnsMinus1 );
  m_cSPS.setNumRowsMinus1( m_iNumRowsMinus1 );
  if( m_iUniformSpacingIdr == 0 )
  {
    m_cSPS.setColumnWidth( m_puiColumnWidth );
    m_cSPS.setRowHeight( m_puiRowHeight );
  }
  m_cSPS.setScalingListFlag ( (m_useScalingListId == 0) ? 0 : 1 );
  m_cSPS.setUseDF( m_loopFilterOffsetInAPS );

#if HHI_DMM_WEDGE_INTRA || HHI_DMM_PRED_TEX
  m_cSPS.setUseDMM( m_bUseDMM );
#endif
#if OL_DEPTHLIMIT_A0044
  m_cSPS.setUseDPL( m_bDepthPartitionLimiting );
#endif
#if HHI_MPI
  m_cSPS.setUseMVI( m_bUseMVI );
#endif

  if( m_isDepth )
  {
    m_cSPS.initMultiviewSPSDepth    ( m_viewId, m_iViewOrderIdx );
#if DEPTH_MAP_GENERATION
    m_cSPS.setPredDepthMapGeneration( m_viewId, true );
#endif
#if HHI_INTER_VIEW_RESIDUAL_PRED
    m_cSPS.setMultiviewResPredMode  ( 0 );
#endif
  }
  else
  {
    m_cSPS.initMultiviewSPS           ( m_viewId, m_iViewOrderIdx, m_uiCamParPrecision, m_bCamParInSliceHeader, m_aaiCodedScale, m_aaiCodedOffset );
    if( m_viewId )
    {
#if DEPTH_MAP_GENERATION
#if HHI_INTER_VIEW_MOTION_PRED
      m_cSPS.setPredDepthMapGeneration( m_viewId, false, m_uiPredDepthMapGeneration, m_uiMultiviewMvPredMode, m_uiPdmPrecision, m_aaiPdmScaleNomDelta, m_aaiPdmOffset );
#else
      m_cSPS.setPredDepthMapGeneration( m_viewId, false, m_uiPredDepthMapGeneration, 0, m_uiPdmPrecision, m_aaiPdmScaleNomDelta, m_aaiPdmOffset );
#endif
#endif
#if HHI_INTER_VIEW_RESIDUAL_PRED
      m_cSPS.setMultiviewResPredMode  ( m_uiMultiviewResPredMode );
#endif
    }
    else
    {
#if DEPTH_MAP_GENERATION
      m_cSPS.setPredDepthMapGeneration( m_viewId, false );
#endif
#if HHI_INTER_VIEW_RESIDUAL_PRED
      m_cSPS.setMultiviewResPredMode  ( 0 );
#endif
    }
  }
}

Void TEncTop::xInitPPS()
{
  m_cPPS.setConstrainedIntraPred( m_bUseConstrainedIntraPred );
  m_cPPS.setSliceGranularity(m_iSliceGranularity);
#if !H0566_TLA
  if ( m_cSPS.getTemporalIdNestingFlag() ) 
  {
    m_cPPS.setNumTLayerSwitchingFlags( 0 );
    for ( UInt i = 0; i < m_cSPS.getMaxTLayers() - 1; i++ )
    {
      m_cPPS.setTLayerSwitchingFlag( i, true );
    }
  }
  else
  {
    m_cPPS.setNumTLayerSwitchingFlags( m_cSPS.getMaxTLayers() - 1 );
    for ( UInt i = 0; i < m_cPPS.getNumTLayerSwitchingFlags(); i++ )
    {
      m_cPPS.setTLayerSwitchingFlag( i, m_abTLayerSwitchingFlag[i] );
    }
  }   
#endif
  Bool bUseDQP = (getMaxCuDQPDepth() > 0)? true : false;

#if LOSSLESS_CODING
#if H0736_AVC_STYLE_QP_RANGE
  Int lowestQP = - m_cSPS.getQpBDOffsetY();
#else
  Int lowestQP = 0;
#endif

  if(getUseLossless())
  {
    if ((getMaxCuDQPDepth() == 0) && (getMaxDeltaQP() == 0 ) && (getQP() == lowestQP) )
    {
      bUseDQP = false;
    }
    else
    {
      bUseDQP = true;
    }
  }
  else
  {
    if(bUseDQP == false)
    {
      if((getMaxDeltaQP() != 0 )|| getUseAdaptiveQP())
      {
        bUseDQP = true;
      }
    }
  }

#else
  if(bUseDQP == false)
  {
    if((getMaxDeltaQP() != 0 )|| getUseAdaptiveQP())
    {
      bUseDQP = true;
    }
  }
#endif

  if(bUseDQP)
  {
    m_cPPS.setUseDQP(true);
    m_cPPS.setMaxCuDQPDepth( m_iMaxCuDQPDepth );
    m_cPPS.setMinCuDQPSize( m_cPPS.getSPS()->getMaxCUWidth() >> ( m_cPPS.getMaxCuDQPDepth()) );
  }
  else
  {
    m_cPPS.setUseDQP(false);
    m_cPPS.setMaxCuDQPDepth( 0 );
    m_cPPS.setMinCuDQPSize( m_cPPS.getSPS()->getMaxCUWidth() >> ( m_cPPS.getMaxCuDQPDepth()) );
  }

  m_cPPS.setChromaQpOffset   ( m_iChromaQpOffset    );
  m_cPPS.setChromaQpOffset2nd( m_iChromaQpOffset2nd );

  m_cPPS.setEntropyCodingMode( 1 ); // In the PPS now, but also remains in slice header!
#if !WPP_SIMPLIFICATION
  m_cPPS.setEntropyCodingSynchro(m_iWaveFrontSynchro);
  m_cPPS.setCabacIstateReset(m_iWaveFrontFlush != 0);
#endif
  m_cPPS.setNumSubstreams(m_iWaveFrontSubstreams);
  m_cPPS.setUseWP( m_bUseWeightPred );
  m_cPPS.setWPBiPredIdc( m_uiBiPredIdc );
  m_cPPS.setEnableTMVPFlag( m_bEnableTMVP );
#if H0388
  m_cPPS.setOutputFlagPresentFlag( false );
#endif
#if MULTIBITS_DATA_HIDING
  m_cPPS.setSignHideFlag(getSignHideFlag());
  m_cPPS.setTSIG(getTSIG());
#endif
#if DBL_CONTROL
  m_cPPS.setDeblockingFilterControlPresent (m_DeblockingFilterControlPresent );
#endif
#if PARALLEL_MERGE
  m_cPPS.setLog2ParallelMergeLevelMinus2      (LOG2_PARALLEL_MERGE_LEVEL_MINUS2);
#endif
#if CABAC_INIT_FLAG
  m_cPPS.setCabacInitPresentFlag(CABAC_INIT_PRESENT_FLAG);
#endif
}

//Function for initializing m_RPSList, a list of TComReferencePictureSet, based on the GOPEntry objects read from the config file.
Void TEncTop::xInitRPS()
{
  TComReferencePictureSet*      rps;
  
  m_RPSList.create(getGOPSize()+m_extraRPSs);
  for( Int i = 0; i < getGOPSize()+m_extraRPSs; i++) 
  {
    GOPEntryMvc ge = getGOPEntry(i);
    rps = m_RPSList.getReferencePictureSet(i);
    rps->setNumberOfPictures(ge.m_numRefPics);
    rps->setNumRefIdc(ge.m_numRefIdc);
    Int numNeg = 0;
    Int numPos = 0;
    for( Int j = 0; j < ge.m_numRefPics; j++)
    {
      rps->setDeltaPOC(j,ge.m_referencePics[j]);
      rps->setUsed(j,ge.m_usedByCurrPic[j]);
      if(ge.m_referencePics[j]>0)
      {
        numPos++;
      }
      else
      {
        numNeg++;
      }
    }
    rps->setNumberOfNegativePictures(numNeg);
    rps->setNumberOfPositivePictures(numPos);
    rps->setInterRPSPrediction(ge.m_interRPSPrediction);
    if (ge.m_interRPSPrediction)
    {
      rps->setDeltaRIdxMinus1(ge.m_deltaRIdxMinus1);
      rps->setDeltaRPS(ge.m_deltaRPS);
      rps->setNumRefIdc(ge.m_numRefIdc);
      for (Int j = 0; j < ge.m_numRefIdc; j++ )
      {
        rps->setRefIdc(j, ge.m_refIdc[j]);
      }
#if WRITE_BACK
      // the folowing code overwrite the deltaPOC and Used by current values read from the config file with the ones
      // computed from the RefIdc.  This is not necessary if both are identical. Currently there is no check to see if they are identical.
      numNeg = 0;
      numPos = 0;
      TComReferencePictureSet*     RPSRef = m_RPSList.getReferencePictureSet(i-(ge.m_deltaRIdxMinus1+1));
      for (Int j = 0; j < ge.m_numRefIdc; j++ )
      {
        if (ge.m_refIdc[j])
        {
          Int deltaPOC = ge.m_deltaRPS + ((j < RPSRef->getNumberOfPictures())? RPSRef->getDeltaPOC(j) : 0);
          rps->setDeltaPOC((numNeg+numPos),deltaPOC);
          rps->setUsed((numNeg+numPos),ge.m_refIdc[j]==1?1:0);
          if (deltaPOC<0)
          {
            numNeg++;
          }
          else
          {
            numPos++;
          }
        }
      }
      rps->setNumberOfNegativePictures(numNeg);
      rps->setNumberOfPositivePictures(numPos);
      rps->sortDeltaPOC();
#endif
    }
  }
  
}

   // This is a function that 
   // determines what Reference Picture Set to use 
   // for a specific slice (with POC = POCCurr)
Void TEncTop::selectReferencePictureSet(TComSlice* slice, Int POCCurr, Int GOPid,TComList<TComPic*>& listPic )
{
  if( slice->getNalUnitType() == NAL_UNIT_CODED_SLICE_IDV && POCCurr == 0 )
  {
    TComReferencePictureSet* rps = slice->getLocalRPS();
    rps->setNumberOfNegativePictures(0);
    rps->setNumberOfPositivePictures(0);
    rps->setNumberOfLongtermPictures(0);
    rps->setNumberOfPictures(0);
    slice->setRPS(rps);
  }
  else
  {
    slice->setRPSidx(GOPid);

    for(Int extraNum=m_iGOPSize; extraNum<m_extraRPSs+m_iGOPSize; extraNum++)
    {    
      if(m_uiIntraPeriod > 0)
      {
        if(POCCurr%m_uiIntraPeriod==m_GOPList[extraNum].m_POC)
        {
          slice->setRPSidx(extraNum);
        }
      }
      else
      {
        if(POCCurr==m_GOPList[extraNum].m_POC)
        {
          slice->setRPSidx(extraNum);
        }
      }
    }

    slice->setRPS(getRPSList()->getReferencePictureSet(slice->getRPSidx()));
    slice->getRPS()->setNumberOfPictures(slice->getRPS()->getNumberOfNegativePictures()+slice->getRPS()->getNumberOfPositivePictures());
  }
}

Void TEncTop::xInitSPSforInterViewRefs()
{
  // base view
  if( getGOPEntry( MAX_GOP ).m_POC == -1 )
  { 
    m_cSPS.setNumberOfUsableInterViewRefs( 0 );
    m_cSPS.setListsModificationPresentFlag( false );
    return;
  }

  Int numberUsableInterViewRefs = 0;
  for( Int i = 0; i < getGOPSize()+1 && numberUsableInterViewRefs < MAX_VIEW_NUM; i++ ) 
  {
    GOPEntryMvc ge = ( i < getGOPSize() ) ? getGOPEntry( i ) : getGOPEntry( MAX_GOP );
    for( Int j = 0; j < ge.m_numInterViewRefPics; j++ )
    {
      // add ref view to list
      Bool onList = false;
      for( Int k = 0; k < numberUsableInterViewRefs; k++ )
      {
        if( ge.m_interViewRefs[j] == m_cSPS.getUsableInterViewRef( k ) )
        {
          onList = true;
          break;
        }
      }
      if( !onList )
      {
        m_cSPS.setUsableInterViewRef( (UInt)numberUsableInterViewRefs, ge.m_interViewRefs[j] );
        numberUsableInterViewRefs++;
      }
    }
  }
  m_cSPS.setNumberOfUsableInterViewRefs( numberUsableInterViewRefs );

  // sort inter view refs
  for( Int j = 1; j < m_cSPS.getNumberOfUsableInterViewRefs(); j++ )
  { 
    Int deltaViewId = m_cSPS.getUsableInterViewRef( j );
    for( Int k = j-1; k >= 0; k-- )
    {
      Int temp = m_cSPS.getUsableInterViewRef( k );
      if( deltaViewId > temp )
      {
        m_cSPS.setUsableInterViewRef( k+1, temp );
        m_cSPS.setUsableInterViewRef( k, deltaViewId );
      }
    }
  }

  m_cSPS.setListsModificationPresentFlag( true );
}

Void  TEncTop::xInitPPSforTiles()
{
    m_cPPS.setColumnRowInfoPresent( m_iColumnRowInfoPresent );
    m_cPPS.setUniformSpacingIdr( m_iUniformSpacingIdr );
#if !REMOVE_TILE_DEPENDENCE
    m_cPPS.setTileBoundaryIndependenceIdr( m_iTileBoundaryIndependenceIdr );
#endif
    m_cPPS.setNumColumnsMinus1( m_iNumColumnsMinus1 );
    m_cPPS.setNumRowsMinus1( m_iNumRowsMinus1 );
    if( m_iUniformSpacingIdr == 0 )
    {
      m_cPPS.setColumnWidth( m_puiColumnWidth );
      m_cPPS.setRowHeight( m_puiRowHeight );
    }
    m_cPPS.setTileBehaviorControlPresentFlag( m_iTileBehaviorControlPresentFlag );
    m_cPPS.setLFCrossTileBoundaryFlag( m_bLFCrossTileBoundaryFlag );

    // # substreams is "per tile" when tiles are independent.
#if FIX_REMOVE_TILE_DEPENDENCE
    if ( m_iWaveFrontSynchro )
#else
    if (m_iTileBoundaryIndependenceIdr && m_iWaveFrontSynchro)
#endif
    {
      m_cPPS.setNumSubstreams(m_iWaveFrontSubstreams * (m_iNumColumnsMinus1+1)*(m_iNumRowsMinus1+1));
    }
}

Void  TEncCfg::xCheckGSParameters()
{
  Int   iWidthInCU = ( m_iSourceWidth%g_uiMaxCUWidth ) ? m_iSourceWidth/g_uiMaxCUWidth + 1 : m_iSourceWidth/g_uiMaxCUWidth;
  Int   iHeightInCU = ( m_iSourceHeight%g_uiMaxCUHeight ) ? m_iSourceHeight/g_uiMaxCUHeight + 1 : m_iSourceHeight/g_uiMaxCUHeight;
  UInt  uiCummulativeColumnWidth = 0;
  UInt  uiCummulativeRowHeight = 0;

  //check the column relative parameters
  if( m_iNumColumnsMinus1 >= (1<<(LOG2_MAX_NUM_COLUMNS_MINUS1+1)) )
  {
    printf( "The number of columns is larger than the maximum allowed number of columns.\n" );
    exit( EXIT_FAILURE );
  }

  if( m_iNumColumnsMinus1 >= iWidthInCU )
  {
    printf( "The current picture can not have so many columns.\n" );
    exit( EXIT_FAILURE );
  }

  if( m_iNumColumnsMinus1 && m_iUniformSpacingIdr==0 )
  {
    for(Int i=0; i<m_iNumColumnsMinus1; i++)
      uiCummulativeColumnWidth += m_puiColumnWidth[i];

    if( uiCummulativeColumnWidth >= iWidthInCU )
    {
      printf( "The width of the column is too large.\n" );
      exit( EXIT_FAILURE );
    }
  }

  //check the row relative parameters
  if( m_iNumRowsMinus1 >= (1<<(LOG2_MAX_NUM_ROWS_MINUS1+1)) )
  {
    printf( "The number of rows is larger than the maximum allowed number of rows.\n" );
    exit( EXIT_FAILURE );
  }

  if( m_iNumRowsMinus1 >= iHeightInCU )
  {
    printf( "The current picture can not have so many rows.\n" );
    exit( EXIT_FAILURE );
  }

  if( m_iNumRowsMinus1 && m_iUniformSpacingIdr==0 )
  {
    for(Int i=0; i<m_iNumRowsMinus1; i++)
      uiCummulativeRowHeight += m_puiRowHeight[i];

    if( uiCummulativeRowHeight >= iHeightInCU )
    {
      printf( "The height of the row is too large.\n" );
      exit( EXIT_FAILURE );
    }
  }
}

Void TEncTop::setTEncTopList(std::vector<TEncTop*>* pacTEncTopList )
{
  assert(m_viewId!=-1); // not to be set for single view coding

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
  if(m_viewId==-1)
    printf( "\n\nSUMMARY --------------------------------------------------------\n" );
  else {
    if ( m_isDepth )
    {
      printf( "\n\nSUMMARY ---------------------------------------------- DEPTH %2d\n", m_viewId );
    }
    else
    {
      printf( "\n\nSUMMARY ---------------------------------------------- VIDEO %2d\n", m_viewId );
    }
  };
  m_cAnalyzeAll.printOut('a');

  printf( "\n\nI Slices--------------------------------------------------------\n" );
  m_cAnalyzeI.printOut('i');

  printf( "\n\nP Slices--------------------------------------------------------\n" );
  m_cAnalyzeP.printOut('p');

  printf( "\n\nB Slices--------------------------------------------------------\n" );
  m_cAnalyzeB.printOut('b');

//  m_cAnalyzeAll.printSummaryOut();
//  m_cAnalyzeI.printSummary('I');
//  m_cAnalyzeP.printSummary('P');
//  m_cAnalyzeB.printSummary('B');
}

//! \}
