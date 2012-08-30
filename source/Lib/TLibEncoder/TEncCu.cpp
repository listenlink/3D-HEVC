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

/** \file     TEncCu.cpp
    \brief    Coding Unit (CU) encoder class
*/

#include <stdio.h>
#include "TEncTop.h"
#include "TEncCu.h"
#include "TEncAnalyze.h"

#include <cmath>
#include <algorithm>
using namespace std;

//! \ingroup TLibEncoder
//! \{

// ====================================================================================================================
// Constructor / destructor / create / destroy
// ====================================================================================================================

/**
 \param    uiTotalDepth  total number of allowable depth
 \param    uiMaxWidth    largest CU width
 \param    uiMaxHeight   largest CU height
 */
Void TEncCu::create(UChar uhTotalDepth, UInt uiMaxWidth, UInt uiMaxHeight)
{
  Int i;
  
  m_uhTotalDepth   = uhTotalDepth + 1;
  m_ppcBestCU      = new TComDataCU*[m_uhTotalDepth-1];
  m_ppcTempCU      = new TComDataCU*[m_uhTotalDepth-1];
  
  m_ppcPredYuvBest = new TComYuv*[m_uhTotalDepth-1];
  m_ppcResiYuvBest = new TComYuv*[m_uhTotalDepth-1];
  m_ppcRecoYuvBest = new TComYuv*[m_uhTotalDepth-1];
  m_ppcPredYuvTemp = new TComYuv*[m_uhTotalDepth-1];
  m_ppcResiYuvTemp = new TComYuv*[m_uhTotalDepth-1];
  m_ppcRecoYuvTemp = new TComYuv*[m_uhTotalDepth-1];
  m_ppcOrigYuv     = new TComYuv*[m_uhTotalDepth-1];
#if HHI_INTER_VIEW_RESIDUAL_PRED
  m_ppcResPredTmp  = new TComYuv*[m_uhTotalDepth-1];
#endif
  
#if HHI_MPI
  m_puhDepthSaved  = new UChar[1ll<<( ( m_uhTotalDepth - 1 )<<1 )];
  m_puhWidthSaved  = new UChar[1ll<<( ( m_uhTotalDepth - 1 )<<1 )];
  m_puhHeightSaved = new UChar[1ll<<( ( m_uhTotalDepth - 1 )<<1 )];
#endif

  UInt uiNumPartitions;
  for( i=0 ; i<m_uhTotalDepth-1 ; i++)
  {
    uiNumPartitions = 1<<( ( m_uhTotalDepth - i - 1 )<<1 );
    UInt uiWidth  = uiMaxWidth  >> i;
    UInt uiHeight = uiMaxHeight >> i;
    
    m_ppcBestCU[i] = new TComDataCU; m_ppcBestCU[i]->create( uiNumPartitions, uiWidth, uiHeight, false, uiMaxWidth >> (m_uhTotalDepth - 1) );
    m_ppcTempCU[i] = new TComDataCU; m_ppcTempCU[i]->create( uiNumPartitions, uiWidth, uiHeight, false, uiMaxWidth >> (m_uhTotalDepth - 1) );
    
    m_ppcPredYuvBest[i] = new TComYuv; m_ppcPredYuvBest[i]->create(uiWidth, uiHeight);
    m_ppcResiYuvBest[i] = new TComYuv; m_ppcResiYuvBest[i]->create(uiWidth, uiHeight);
    m_ppcRecoYuvBest[i] = new TComYuv; m_ppcRecoYuvBest[i]->create(uiWidth, uiHeight);
    
    m_ppcPredYuvTemp[i] = new TComYuv; m_ppcPredYuvTemp[i]->create(uiWidth, uiHeight);
    m_ppcResiYuvTemp[i] = new TComYuv; m_ppcResiYuvTemp[i]->create(uiWidth, uiHeight);
    m_ppcRecoYuvTemp[i] = new TComYuv; m_ppcRecoYuvTemp[i]->create(uiWidth, uiHeight);
    
    m_ppcOrigYuv    [i] = new TComYuv; m_ppcOrigYuv    [i]->create(uiWidth, uiHeight);
#if HHI_INTER_VIEW_RESIDUAL_PRED
    m_ppcResPredTmp [i] = new TComYuv; m_ppcResPredTmp [i]->create(uiWidth, uiHeight);
#endif
  }
  
  m_bEncodeDQP = false;
#if BURST_IPCM
  m_checkBurstIPCMFlag = false;
#endif

  // initialize partition order.
  UInt* piTmp = &g_auiZscanToRaster[0];
  initZscanToRaster( m_uhTotalDepth, 1, 0, piTmp);
  initRasterToZscan( uiMaxWidth, uiMaxHeight, m_uhTotalDepth );
  
  // initialize conversion matrix from partition index to pel
  initRasterToPelXY( uiMaxWidth, uiMaxHeight, m_uhTotalDepth );
  initMotionReferIdx ( uiMaxWidth, uiMaxHeight, m_uhTotalDepth );
}

Void TEncCu::destroy()
{
  Int i;
  
#if HHI_MPI
  delete[] m_puhDepthSaved;  m_puhDepthSaved  = NULL;
  delete[] m_puhWidthSaved;  m_puhWidthSaved  = NULL;
  delete[] m_puhHeightSaved; m_puhHeightSaved = NULL;
#endif
  for( i=0 ; i<m_uhTotalDepth-1 ; i++)
  {
    if(m_ppcBestCU[i])
    {
      m_ppcBestCU[i]->destroy();      delete m_ppcBestCU[i];      m_ppcBestCU[i] = NULL;
    }
    if(m_ppcTempCU[i])
    {
      m_ppcTempCU[i]->destroy();      delete m_ppcTempCU[i];      m_ppcTempCU[i] = NULL;
    }
    if(m_ppcPredYuvBest[i])
    {
      m_ppcPredYuvBest[i]->destroy(); delete m_ppcPredYuvBest[i]; m_ppcPredYuvBest[i] = NULL;
    }
    if(m_ppcResiYuvBest[i])
    {
      m_ppcResiYuvBest[i]->destroy(); delete m_ppcResiYuvBest[i]; m_ppcResiYuvBest[i] = NULL;
    }
    if(m_ppcRecoYuvBest[i])
    {
      m_ppcRecoYuvBest[i]->destroy(); delete m_ppcRecoYuvBest[i]; m_ppcRecoYuvBest[i] = NULL;
    }
    if(m_ppcPredYuvTemp[i])
    {
      m_ppcPredYuvTemp[i]->destroy(); delete m_ppcPredYuvTemp[i]; m_ppcPredYuvTemp[i] = NULL;
    }
    if(m_ppcResiYuvTemp[i])
    {
      m_ppcResiYuvTemp[i]->destroy(); delete m_ppcResiYuvTemp[i]; m_ppcResiYuvTemp[i] = NULL;
    }
    if(m_ppcRecoYuvTemp[i])
    {
      m_ppcRecoYuvTemp[i]->destroy(); delete m_ppcRecoYuvTemp[i]; m_ppcRecoYuvTemp[i] = NULL;
    }
    if(m_ppcOrigYuv[i])
    {
      m_ppcOrigYuv[i]->destroy();     delete m_ppcOrigYuv[i];     m_ppcOrigYuv[i] = NULL;
    }
#if HHI_INTER_VIEW_RESIDUAL_PRED
    if(m_ppcResPredTmp[i])
    {
      m_ppcResPredTmp [i]->destroy(); delete m_ppcResPredTmp[i];  m_ppcResPredTmp[i] = NULL;
    }
#endif
  }
  if(m_ppcBestCU)
  {
    delete [] m_ppcBestCU;
    m_ppcBestCU = NULL;
  }
  if(m_ppcTempCU)
  {
    delete [] m_ppcTempCU;
    m_ppcTempCU = NULL;
  }
  
  if(m_ppcPredYuvBest)
  {
    delete [] m_ppcPredYuvBest;
    m_ppcPredYuvBest = NULL;
  }
  if(m_ppcResiYuvBest)
  {
    delete [] m_ppcResiYuvBest;
    m_ppcResiYuvBest = NULL;
  }
  if(m_ppcRecoYuvBest)
  {
    delete [] m_ppcRecoYuvBest;
    m_ppcRecoYuvBest = NULL;
  }
  if(m_ppcPredYuvTemp)
  {
    delete [] m_ppcPredYuvTemp;
    m_ppcPredYuvTemp = NULL;
  }
  if(m_ppcResiYuvTemp)
  {
    delete [] m_ppcResiYuvTemp;
    m_ppcResiYuvTemp = NULL;
  }
  if(m_ppcRecoYuvTemp)
  {
    delete [] m_ppcRecoYuvTemp;
    m_ppcRecoYuvTemp = NULL;
  }
  if(m_ppcOrigYuv)
  {
    delete [] m_ppcOrigYuv;
    m_ppcOrigYuv = NULL;
  }
#if HHI_INTER_VIEW_RESIDUAL_PRED
  if(m_ppcResPredTmp)
  {
    delete [] m_ppcResPredTmp;
    m_ppcResPredTmp = NULL;
  }
#endif
}

/** \param    pcEncTop      pointer of encoder class
 */
Void TEncCu::init( TEncTop* pcEncTop )
{
  m_pcEncCfg           = pcEncTop;
  m_pcPredSearch       = pcEncTop->getPredSearch();
  m_pcTrQuant          = pcEncTop->getTrQuant();
  m_pcBitCounter       = pcEncTop->getBitCounter();
  m_pcRdCost           = pcEncTop->getRdCost();
  
  m_pcEntropyCoder     = pcEncTop->getEntropyCoder();
  m_pcCavlcCoder       = pcEncTop->getCavlcCoder();
  m_pcSbacCoder       = pcEncTop->getSbacCoder();
  m_pcBinCABAC         = pcEncTop->getBinCABAC();
  
  m_pppcRDSbacCoder   = pcEncTop->getRDSbacCoder();
  m_pcRDGoOnSbacCoder = pcEncTop->getRDGoOnSbacCoder();
  
  m_bUseSBACRD        = pcEncTop->getUseSBACRD();
}

// ====================================================================================================================
// Public member functions
// ====================================================================================================================

/** \param  rpcCU pointer of CU data class
 */
Void TEncCu::compressCU( TComDataCU*& rpcCU )
{
  // initialize CU data
  m_ppcBestCU[0]->initCU( rpcCU->getPic(), rpcCU->getAddr() );
  m_ppcTempCU[0]->initCU( rpcCU->getPic(), rpcCU->getAddr() );

  // analysis of CU
  xCompressCU( m_ppcBestCU[0], m_ppcTempCU[0], 0 );

#if ADAPTIVE_QP_SELECTION
  if( m_pcEncCfg->getUseAdaptQpSelect() )
  {
    if(rpcCU->getSlice()->getSliceType()!=I_SLICE) //IIII
    {
      xLcuCollectARLStats( rpcCU);
    }
  }
#endif
}
/** \param  pcCU  pointer of CU data class, bForceTerminate when set to true terminates slice (default is false).
 */
Void TEncCu::encodeCU ( TComDataCU* pcCU, Bool bForceTerminate )
{
  if ( pcCU->getSlice()->getPPS()->getUseDQP() )
  {
    setdQPFlag(true);
  }

#if BURST_IPCM
  TComPic* pcPic = pcCU->getPic();
  Bool checkBurstIPCMFlag = (pcPic->getSlice(0)->getSPS()->getUsePCM())? true : false;

  setCheckBurstIPCMFlag( checkBurstIPCMFlag );

  pcCU->setNumSucIPCM(0);
  pcCU->setLastCUSucIPCMFlag(false);
#endif

  // Encode CU data
  xEncodeCU( pcCU, 0, 0 );
  
#if OL_FLUSH
  bool bTerminateSlice = bForceTerminate;
  UInt uiCUAddr = pcCU->getAddr();
    /* If at the end of an LCU line but not at the end of a substream, perform CABAC flush */
#if WPP_SIMPLIFICATION
    if (!bTerminateSlice && pcCU->getSlice()->getPPS()->getNumSubstreams() > 1)
#else
    if (!bTerminateSlice && pcCU->getSlice()->getPPS()->getCabacIstateReset())
#endif
    {
      Int iNumSubstreams = pcCU->getSlice()->getPPS()->getNumSubstreams();
      UInt uiWidthInLCUs = pcCU->getPic()->getPicSym()->getFrameWidthInCU();
      UInt uiCol     = uiCUAddr % uiWidthInLCUs;
      UInt uiLin     = uiCUAddr / uiWidthInLCUs;
#if !REMOVE_TILE_DEPENDENCE
      Int iBreakDep = pcCU->getPic()->getPicSym()->getTileBoundaryIndependenceIdr();
#endif
      UInt uiTileStartLCU = pcCU->getPic()->getPicSym()->getTComTile(pcCU->getPic()->getPicSym()->getTileIdxMap(uiCUAddr))->getFirstCUAddr();
      UInt uiTileLCUX = uiTileStartLCU % uiWidthInLCUs;
      UInt uiTileLCUY = uiTileStartLCU / uiWidthInLCUs;
      UInt uiTileWidth = pcCU->getPic()->getPicSym()->getTComTile(pcCU->getPic()->getPicSym()->getTileIdxMap(uiCUAddr))->getTileWidth();
      UInt uiTileHeight = pcCU->getPic()->getPicSym()->getTComTile(pcCU->getPic()->getPicSym()->getTileIdxMap(uiCUAddr))->getTileHeight();
      Int iNumSubstreamsPerTile = iNumSubstreams;
#if !REMOVE_TILE_DEPENDENCE
#if WPP_SIMPLIFICATION
      if (iBreakDep && pcCU->getSlice()->getPPS()->getNumSubstreams() > 1)
#else
      if (iBreakDep && pcCU->getSlice()->getPPS()->getEntropyCodingSynchro())
#endif
        iNumSubstreamsPerTile /= pcCU->getPic()->getPicSym()->getNumTiles();
      if ((iBreakDep && (uiCol == uiTileLCUX+uiTileWidth-1) && (uiLin+iNumSubstreamsPerTile < uiTileLCUY+uiTileHeight))
          || (!iBreakDep && (uiCol == uiWidthInLCUs-1) && (uiLin+iNumSubstreams < pcCU->getPic()->getFrameHeightInCU())))
      {
        m_pcEntropyCoder->encodeFlush();
      }
#else
#if WPP_SIMPLIFICATION
      if (pcCU->getSlice()->getPPS()->getNumSubstreams() > 1)
#else
      if (pcCU->getSlice()->getPPS()->getEntropyCodingSynchro())
#endif
      {
        iNumSubstreamsPerTile /= pcCU->getPic()->getPicSym()->getNumTiles();
      }
      if ((uiCol == uiTileLCUX+uiTileWidth-1) && (uiLin+iNumSubstreamsPerTile < uiTileLCUY+uiTileHeight))
      {
        m_pcEntropyCoder->encodeFlush();
      }
#endif
    }
#endif // OL_FLUSH
}

// ====================================================================================================================
// Protected member functions
// ====================================================================================================================
/** Derive small set of test modes for AMP encoder speed-up
 *\param   rpcBestCU
 *\param   eParentPartSize
 *\param   bTestAMP_Hor
 *\param   bTestAMP_Ver
 *\param   bTestMergeAMP_Hor
 *\param   bTestMergeAMP_Ver
 *\returns Void 
*/
#if AMP_ENC_SPEEDUP
#if AMP_MRG
Void TEncCu::deriveTestModeAMP (TComDataCU *&rpcBestCU, PartSize eParentPartSize, Bool &bTestAMP_Hor, Bool &bTestAMP_Ver, Bool &bTestMergeAMP_Hor, Bool &bTestMergeAMP_Ver)
#else
Void TEncCu::deriveTestModeAMP (TComDataCU *&rpcBestCU, PartSize eParentPartSize, Bool &bTestAMP_Hor, Bool &bTestAMP_Ver)
#endif
{
  if ( rpcBestCU->getPartitionSize(0) == SIZE_2NxN )
  {
    bTestAMP_Hor = true;
  }
  else if ( rpcBestCU->getPartitionSize(0) == SIZE_Nx2N )
  {
    bTestAMP_Ver = true;
  }
  else if ( rpcBestCU->getPartitionSize(0) == SIZE_2Nx2N && rpcBestCU->getMergeFlag(0) == false && rpcBestCU->isSkipped(0) == false )
  {
    bTestAMP_Hor = true;          
    bTestAMP_Ver = true;          
  }

#if AMP_MRG
  //! Utilizing the partition size of parent PU    
  if ( eParentPartSize >= SIZE_2NxnU && eParentPartSize <= SIZE_nRx2N )
  { 
    bTestMergeAMP_Hor = true;
    bTestMergeAMP_Ver = true;
  }

  if ( eParentPartSize == SIZE_NONE ) //! if parent is intra
  {
    if ( rpcBestCU->getPartitionSize(0) == SIZE_2NxN )
    {
      bTestMergeAMP_Hor = true;
    }
    else if ( rpcBestCU->getPartitionSize(0) == SIZE_Nx2N )
    {
      bTestMergeAMP_Ver = true;
    }
  }

  if ( rpcBestCU->getPartitionSize(0) == SIZE_2Nx2N && rpcBestCU->isSkipped(0) == false )
  {
    bTestMergeAMP_Hor = true;          
    bTestMergeAMP_Ver = true;          
  }

  if ( rpcBestCU->getWidth(0) == 64 )
  { 
    bTestAMP_Hor = false;
    bTestAMP_Ver = false;
  }    
#else
  //! Utilizing the partition size of parent PU        
  if ( eParentPartSize >= SIZE_2NxnU && eParentPartSize <= SIZE_nRx2N )
  { 
    bTestAMP_Hor = true;
    bTestAMP_Ver = true;
  }

  if ( eParentPartSize == SIZE_2Nx2N )
  { 
    bTestAMP_Hor = false;
    bTestAMP_Ver = false;
  }      
#endif
}
#endif

// ====================================================================================================================
// Protected member functions
// ====================================================================================================================
/** Compress a CU block recursively with enabling sub-LCU-level delta QP
 *\param   rpcBestCU
 *\param   rpcTempCU
 *\param   uiDepth
 *\returns Void
 *
 *- for loop of QP value to compress the current CU with all possible QP
*/
#if AMP_ENC_SPEEDUP
Void TEncCu::xCompressCU( TComDataCU*& rpcBestCU, TComDataCU*& rpcTempCU, UInt uiDepth, PartSize eParentPartSize )
#else
Void TEncCu::xCompressCU( TComDataCU*& rpcBestCU, TComDataCU*& rpcTempCU, UInt uiDepth )
#endif
{
  TComPic* pcPic = rpcBestCU->getPic();

#if OL_DEPTHLIMIT
  TComSPS *sps = pcPic->getSlice(0)->getSPS();
  TComPic *pcTexture;
  TComDataCU *pcTextureCU;
  static UInt* texPartInfo;
  static UInt uiTexPartIndex;
  static Bool depthMapDetect =  false;
  UInt uiPrevTexPartIndex = 0;
#if OL_DO_NOT_LIMIT_INTRA_SLICES_PART
  static Bool bIntraSliceDetect = false;
#endif
  Bool bTry2NxN = false;
  Bool bTryNx2N = false;
  if(uiDepth == 0)
  {
	pcTexture = rpcBestCU->getSlice()->getTexturePic();
	if(pcTexture != NULL) //depth map being encoded
	{
#if OL_DO_NOT_LIMIT_INTRA_SLICES_PART
		bIntraSliceDetect = (rpcBestCU->getSlice()->getSliceType()==I_SLICE);
#endif
		pcTextureCU = pcTexture->getCU( rpcBestCU->getAddr() );
		texPartInfo = pcTextureCU -> readPartInfo();
		uiTexPartIndex = 0;
		depthMapDetect = true;
	}
	else
	{
		depthMapDetect = false;
	}
  }
#endif
  // get Original YUV data from picture
  m_ppcOrigYuv[uiDepth]->copyFromPicYuv( pcPic->getPicYuvOrg(), rpcBestCU->getAddr(), rpcBestCU->getZorderIdxInCU() );

  // variables for fast encoder decision
  Bool    bEarlySkip  = false;
  Bool    bTrySplit    = true;
  Double  fRD_Skip    = MAX_DOUBLE;

  // variable for Early CU determination
  Bool    bSubBranch = true;

  // variable for Cbf fast mode PU decision
  Bool    doNotBlockPu = true;

  Bool    bTrySplitDQP  = true;

  static  Double  afCost[ MAX_CU_DEPTH ];
  static  Int      aiNum [ MAX_CU_DEPTH ];

  if ( rpcBestCU->getAddr() == 0 )
  {
    ::memset( afCost, 0, sizeof( afCost ) );
    ::memset( aiNum,  0, sizeof( aiNum  ) );
  }

  Bool bBoundary = false;
  UInt uiLPelX   = rpcBestCU->getCUPelX();
  UInt uiRPelX   = uiLPelX + rpcBestCU->getWidth(0)  - 1;
  UInt uiTPelY   = rpcBestCU->getCUPelY();
  UInt uiBPelY   = uiTPelY + rpcBestCU->getHeight(0) - 1;

#if HHI_INTERVIEW_SKIP
  Bool bFullyRenderedSec = true ;
  if( m_pcEncCfg->getInterViewSkip() )
  {
    Pel* pUsedSamples ;
    UInt uiStride ;
    pUsedSamples =  pcPic->getUsedPelsMap()->getLumaAddr( rpcBestCU->getAddr(), rpcBestCU->getZorderIdxInCU() );
    uiStride = pcPic->getUsedPelsMap()->getStride();

    for ( Int y=0; y<m_ppcOrigYuv[uiDepth]->getHeight(); y++)
    {
      for ( Int x=0; x<m_ppcOrigYuv[uiDepth]->getWidth(); x++)
      {
        if( pUsedSamples[x] !=0 )
        {
          bFullyRenderedSec = false ;
          break ;
        }
      }
      if ( !bFullyRenderedSec )
      {
        break;
      }
      pUsedSamples += uiStride ;
    }
  }
  else
  {
    bFullyRenderedSec = false ;
  }

#if HHI_INTERVIEW_SKIP_LAMBDA_SCALE
  if( bFullyRenderedSec )
  {
    m_pcRdCost->setLambdaScale( m_pcEncCfg->getInterViewSkipLambdaScale() );
  }
  else
  {
    m_pcRdCost->setLambdaScale( 1 );
  }
  rpcTempCU->getTotalCost()  = m_pcRdCost->calcRdCost( rpcTempCU->getTotalBits(), rpcTempCU->getTotalDistortion() );
#endif

#endif
  Int iBaseQP = xComputeQP( rpcBestCU, uiDepth );
  Int iMinQP;
  Int iMaxQP;
#if LOSSLESS_CODING
  Bool isAddLowestQP = false;
#if H0736_AVC_STYLE_QP_RANGE
  Int lowestQP = -rpcTempCU->getSlice()->getSPS()->getQpBDOffsetY();
#else
  Int lowestQP = 0;
#endif
#endif

  if( (g_uiMaxCUWidth>>uiDepth) >= rpcTempCU->getSlice()->getPPS()->getMinCuDQPSize() )
  {
    Int idQP = m_pcEncCfg->getMaxDeltaQP();
#if H0736_AVC_STYLE_QP_RANGE
    iMinQP = Clip3( -rpcTempCU->getSlice()->getSPS()->getQpBDOffsetY(), MAX_QP, iBaseQP-idQP );
    iMaxQP = Clip3( -rpcTempCU->getSlice()->getSPS()->getQpBDOffsetY(), MAX_QP, iBaseQP+idQP );
#if LOSSLESS_CODING
    if ( (rpcTempCU->getSlice()->getSPS()->getUseLossless()) && (lowestQP < iMinQP) && rpcTempCU->getSlice()->getPPS()->getUseDQP() )
    {
      isAddLowestQP = true; 
      iMinQP = iMinQP - 1;
       
    }
#endif
#else
    iMinQP = Clip3( MIN_QP, MAX_QP, iBaseQP-idQP );
    iMaxQP = Clip3( MIN_QP, MAX_QP, iBaseQP+idQP );
#if LOSSLESS_CODING
    if ( (rpcTempCU->getSlice()->getSPS()->getUseLossless()) && (lowestQP < iMinQP) && rpcTempCU->getSlice()->getPPS()->getUseDQP() )
    {
      isAddLowestQP = true;
      iMinQP = iMinQP - 1;
    }
#endif
#endif
  }
  else
  {
    iMinQP = rpcTempCU->getQP(0);
    iMaxQP = rpcTempCU->getQP(0);
  }

  // If slice start or slice end is within this cu...
  TComSlice * pcSlice = rpcTempCU->getPic()->getSlice(rpcTempCU->getPic()->getCurrSliceIdx());
  Bool bSliceStart = pcSlice->getEntropySliceCurStartCUAddr()>rpcTempCU->getSCUAddr()&&pcSlice->getEntropySliceCurStartCUAddr()<rpcTempCU->getSCUAddr()+rpcTempCU->getTotalNumPart();
  Bool bSliceEnd = (pcSlice->getEntropySliceCurEndCUAddr()>rpcTempCU->getSCUAddr()&&pcSlice->getEntropySliceCurEndCUAddr()<rpcTempCU->getSCUAddr()+rpcTempCU->getTotalNumPart());
  Bool bInsidePicture = ( uiRPelX < rpcBestCU->getSlice()->getSPS()->getPicWidthInLumaSamples() ) && ( uiBPelY < rpcBestCU->getSlice()->getSPS()->getPicHeightInLumaSamples() );
  // We need to split, so don't try these modes.
  if(!bSliceEnd && !bSliceStart && bInsidePicture )
  {
    for (Int iQP=iMinQP; iQP<=iMaxQP; iQP++)
    {
#if LOSSLESS_CODING
      if (isAddLowestQP && (iQP == iMinQP))
      {
        iQP = lowestQP;
      }
#endif
      // variables for fast encoder decision
      bEarlySkip  = false;
      bTrySplit    = true;
      fRD_Skip    = MAX_DOUBLE;

      rpcTempCU->initEstData( uiDepth, iQP );

#if OL_DEPTHLIMIT
  //logic for setting bTrySplit using the partition information that is stored of the texture colocated CU
#if OL_DO_NOT_LIMIT_INTRA_SLICES_PART
  if(depthMapDetect && !bIntraSliceDetect && sps->getUseDPL())
#else
  if(depthMapDetect && sps->getUseDPL()) //depth map being encoded
#endif
  {
	assert(uiDepth == (UInt)texPartInfo[uiTexPartIndex+1]);
	if((UInt)texPartInfo[uiTexPartIndex+0] == 1) //NxN modes
	{
		bTrySplit = true;
		bTryNx2N = true;
		bTry2NxN = true;
		uiPrevTexPartIndex = uiTexPartIndex; 
		uiTexPartIndex += 2;
	}
	else if((UInt)texPartInfo[uiTexPartIndex+0] == 0) //2Nx2N modes
	{
		UInt uiTexdepth;
		UInt temp_uiTexPartIndex;
		bTrySplit = false;
		//scan ahead till next depth
		uiTexdepth = (UInt)texPartInfo[uiTexPartIndex+1];
		uiPrevTexPartIndex = uiTexPartIndex;
		uiTexPartIndex+=2;
		temp_uiTexPartIndex = uiTexPartIndex; //store in case to rewind
		//temp_uiTexPartIndex+=2;
					while(uiTexdepth != (UInt)texPartInfo[uiTexPartIndex+1] && uiTexdepth != 0)
					{
						if((UInt)texPartInfo[uiTexPartIndex+1] < uiTexdepth)
						{
							break;
						}
						uiTexPartIndex+=2;
						
						if((UInt)texPartInfo[uiTexPartIndex+1] == OL_END_CU)
						{
							uiTexPartIndex = temp_uiTexPartIndex;
							uiTexdepth++;
							if(uiTexdepth >= g_uiMaxCUDepth)
							{
								//uiTexPartIndex-=2;
								break;
							}
						}
					}
	}
	else if((UInt)texPartInfo[uiTexPartIndex+0] == OL_END_CU)
	{
		bTrySplit = false;
		bTryNx2N = false;
		bTry2NxN = false;
	}
	else if((UInt)texPartInfo[uiTexPartIndex+0] == 2) //2NxN case
	{
		bTrySplit = false;
		bTryNx2N = false;
		bTry2NxN = true;
		uiPrevTexPartIndex = uiTexPartIndex; 
		uiTexPartIndex += 2;
	}
	else if((UInt)texPartInfo[uiTexPartIndex+0] == 3) //Nx2N case
	{
		bTrySplit = false;
		bTryNx2N = true;
		bTry2NxN = false;
		uiPrevTexPartIndex = uiTexPartIndex; 
		uiTexPartIndex += 2;
	}
  }
#endif


      // do inter modes, SKIP and 2Nx2N
      if( rpcBestCU->getSlice()->getSliceType() != I_SLICE )
      {
#if HHI_INTER_VIEW_RESIDUAL_PRED
        // check availability of residual prediction
        Bool  bResPredAvailable   = false;
        Bool  bResPredAllowed     =                    (!rpcBestCU->getSlice()->getSPS()->isDepth                () );
        bResPredAllowed           = bResPredAllowed && ( rpcBestCU->getSlice()->getSPS()->getViewId              () );
        bResPredAllowed           = bResPredAllowed && ( rpcBestCU->getSlice()->getSPS()->getMultiviewResPredMode() );
        if( bResPredAllowed )
        {
          bResPredAvailable       = rpcBestCU->getResidualSamples( 0, 
#if QC_SIMPLIFIEDIVRP_M24938
            true ,
#endif
            m_ppcResPredTmp[uiDepth] );
        }

        for( UInt uiResPrdId = 0; uiResPrdId < ( bResPredAvailable ? 2 : 1 ); uiResPrdId++ )
        {
          Bool bResPredFlag  = ( uiResPrdId > 0 );
#endif
#if HHI_INTER_VIEW_RESIDUAL_PRED
        rpcTempCU->setResPredIndicator( bResPredAvailable, bResPredFlag );
#endif
        // SKIP
#if HHI_INTERVIEW_SKIP
        xCheckRDCostMerge2Nx2N( rpcBestCU, rpcTempCU, bFullyRenderedSec );
#else
        xCheckRDCostMerge2Nx2N( rpcBestCU, rpcTempCU );
#endif
        rpcTempCU->initEstData( uiDepth, iQP );

        // fast encoder decision for early skip
        if ( m_pcEncCfg->getUseFastEnc() )
        {
          Int iIdx = g_aucConvertToBit[ rpcBestCU->getWidth(0) ];
          if ( aiNum [ iIdx ] > 5 && fRD_Skip < EARLY_SKIP_THRES*afCost[ iIdx ]/aiNum[ iIdx ] )
          {
            bEarlySkip = true;
            bTrySplit  = false;
          }
        }

        // 2Nx2N, NxN
        if ( !bEarlySkip )
        {
#if HHI_INTER_VIEW_RESIDUAL_PRED
          rpcTempCU->setResPredIndicator( bResPredAvailable, bResPredFlag );
#endif
#if HHI_INTERVIEW_SKIP
            xCheckRDCostInter( rpcBestCU, rpcTempCU, SIZE_2Nx2N, bFullyRenderedSec );

#else
            xCheckRDCostInter( rpcBestCU, rpcTempCU, SIZE_2Nx2N );
#endif
          rpcTempCU->initEstData( uiDepth, iQP );
          if(m_pcEncCfg->getUseCbfFastMode())
          {
            doNotBlockPu = rpcBestCU->getQtRootCbf( 0 ) != 0;
          }
        }
#if HHI_INTER_VIEW_RESIDUAL_PRED
        } // uiResPrdId
#endif
      } // != I_SLICE

#if OL_DEPTHLIMIT
#if OL_DO_NOT_LIMIT_INTRA_SLICES_PART
	  if(depthMapDetect && !bIntraSliceDetect  && sps->getUseDPL())
#else
	  if(depthMapDetect  && sps->getUseDPL()) //depth map being encoded
#endif
	  {
		  bTrySplitDQP = bTrySplit;
	  }
	  else
	  {
		if( (g_uiMaxCUWidth>>uiDepth) >= rpcTempCU->getSlice()->getPPS()->getMinCuDQPSize() )
		{
			if(iQP == iBaseQP)
			{
				bTrySplitDQP = bTrySplit;
			}
		}
		else
		{
			bTrySplitDQP = bTrySplit;
		}
	  }
#else

      if( (g_uiMaxCUWidth>>uiDepth) >= rpcTempCU->getSlice()->getPPS()->getMinCuDQPSize() )
      {
        if(iQP == iBaseQP)
        {
          bTrySplitDQP = bTrySplit;
        }
      }
      else
      {
        bTrySplitDQP = bTrySplit;
      }
#endif
#if LOSSLESS_CODING
      if (isAddLowestQP && (iQP == lowestQP))
      {
        iQP = iMinQP;
      }
#endif
    }  // end for iMinQP to iMaxQP


    for (Int iQP=iMinQP; iQP<=iMaxQP; iQP++)
    {
#if LOSSLESS_CODING
      if (isAddLowestQP && (iQP == iMinQP))
      {
        iQP = lowestQP;
      }
#endif
      rpcTempCU->initEstData( uiDepth, iQP );

      // do inter modes, NxN, 2NxN, and Nx2N
      if( rpcBestCU->getSlice()->getSliceType() != I_SLICE )
      {
#if HHI_INTER_VIEW_RESIDUAL_PRED
        // check availability of residual prediction
        Bool  bResPredAvailable   = false;
        Bool  bResPredAllowed     =                    (!rpcBestCU->getSlice()->getSPS()->isDepth                () );
        bResPredAllowed           = bResPredAllowed && ( rpcBestCU->getSlice()->getSPS()->getViewId              () );
        bResPredAllowed           = bResPredAllowed && ( rpcBestCU->getSlice()->getSPS()->getMultiviewResPredMode() );
        if( bResPredAllowed )
        {
          bResPredAvailable       = rpcBestCU->getResidualSamples( 0, 
#if QC_SIMPLIFIEDIVRP_M24938
            true,
#endif
            m_ppcResPredTmp[uiDepth] );
        }

        for( UInt uiResPrdId = 0; uiResPrdId < ( bResPredAvailable ? 2 : 1 ); uiResPrdId++ )
        {
          Bool bResPredFlag  = ( uiResPrdId > 0 );
#endif
        // 2Nx2N, NxN
        if ( !bEarlySkip )
        {

        if(!( rpcBestCU->getSlice()->getSPS()->getDisInter4x4()  && (rpcBestCU->getWidth(0)==8) && (rpcBestCU->getHeight(0)==8) ))
        {
          if( uiDepth == g_uiMaxCUDepth - g_uiAddCUDepth && doNotBlockPu)
          {
#if OL_DEPTHLIMIT //add code here to select 2NxN or Nx2N or none
#if OL_DO_NOT_LIMIT_INTRA_SLICES_PART
  if(depthMapDetect && !bIntraSliceDetect  && sps->getUseDPL())
#else
  if(depthMapDetect  && sps->getUseDPL()) //depth map being encoded
#endif
			{
				assert(uiDepth == (UInt)texPartInfo[uiPrevTexPartIndex+1]);
				if (bTrySplit)
				{	
#endif
#if HHI_INTER_VIEW_RESIDUAL_PRED
            rpcTempCU->setResPredIndicator( bResPredAvailable, bResPredFlag );
#endif
#if HHI_INTERVIEW_SKIP
            xCheckRDCostInter( rpcBestCU, rpcTempCU, SIZE_NxN, bFullyRenderedSec   );
#else
            xCheckRDCostInter( rpcBestCU, rpcTempCU, SIZE_NxN   );
#endif
            rpcTempCU->initEstData( uiDepth, iQP );
#if OL_DEPTHLIMIT
				}//bTrySplit
			}//depthMapDetect
			else//do things normally
			{
#if HHI_INTER_VIEW_RESIDUAL_PRED
            rpcTempCU->setResPredIndicator( bResPredAvailable, bResPredFlag );
#endif
#if HHI_INTERVIEW_SKIP
            xCheckRDCostInter( rpcBestCU, rpcTempCU, SIZE_NxN, bFullyRenderedSec   );
#else
            xCheckRDCostInter( rpcBestCU, rpcTempCU, SIZE_NxN   );
#endif
            rpcTempCU->initEstData( uiDepth, iQP );
          }
#endif
          } 
        }
        }

        { // 2NxN, Nx2N
#if OL_DEPTHLIMIT //add code here to select 2NxN or Nx2N or none
#if OL_DO_NOT_LIMIT_INTRA_SLICES_PART
  if(depthMapDetect && !bIntraSliceDetect  && sps->getUseDPL())
#else
  if(depthMapDetect  && sps->getUseDPL()) //depth map being encoded
#endif
			{
				assert(uiDepth == (UInt)texPartInfo[uiPrevTexPartIndex+1]);
				if (bTryNx2N)
				{	
#endif
          if(doNotBlockPu)
          {
#if HHI_INTER_VIEW_RESIDUAL_PRED
            rpcTempCU->setResPredIndicator( bResPredAvailable, bResPredFlag );
#endif
#if HHI_INTERVIEW_SKIP
            xCheckRDCostInter( rpcBestCU, rpcTempCU, SIZE_Nx2N, bFullyRenderedSec   );
#else
            xCheckRDCostInter( rpcBestCU, rpcTempCU, SIZE_Nx2N  );
#endif
            rpcTempCU->initEstData( uiDepth, iQP );
            if(m_pcEncCfg->getUseCbfFastMode() && rpcBestCU->getPartitionSize(0) == SIZE_Nx2N )
            {
              doNotBlockPu = rpcBestCU->getQtRootCbf( 0 ) != 0;
            }
          }
#if OL_DEPTHLIMIT
				}//bTryNx2N
			}//depthMapDetect
			else//do things normally
			{
          if(doNotBlockPu)
          {
#if HHI_INTER_VIEW_RESIDUAL_PRED
            rpcTempCU->setResPredIndicator( bResPredAvailable, bResPredFlag );
#endif
#if HHI_INTERVIEW_SKIP
            xCheckRDCostInter( rpcBestCU, rpcTempCU, SIZE_Nx2N, bFullyRenderedSec   );
#else
            xCheckRDCostInter( rpcBestCU, rpcTempCU, SIZE_Nx2N  );
#endif
            rpcTempCU->initEstData( uiDepth, iQP );
            if(m_pcEncCfg->getUseCbfFastMode() && rpcBestCU->getPartitionSize(0) == SIZE_Nx2N )
            {
              doNotBlockPu = rpcBestCU->getQtRootCbf( 0 ) != 0;
            }
          }
			}
#endif

#if OL_DEPTHLIMIT //add code here to select 2NxN or Nx2N or none
#if OL_DO_NOT_LIMIT_INTRA_SLICES_PART
  if(depthMapDetect && !bIntraSliceDetect  && sps->getUseDPL())
#else
  if(depthMapDetect  && sps->getUseDPL()) //depth map being encoded
#endif
			{
				assert(uiDepth == (UInt)texPartInfo[uiPrevTexPartIndex+1]);
				if (bTry2NxN)
				{	
#endif
          if(doNotBlockPu)
          {
#if HHI_INTER_VIEW_RESIDUAL_PRED
            rpcTempCU->setResPredIndicator( bResPredAvailable, bResPredFlag );
#endif
#if HHI_INTERVIEW_SKIP
            xCheckRDCostInter( rpcBestCU, rpcTempCU, SIZE_2NxN, bFullyRenderedSec   );
#else
            xCheckRDCostInter      ( rpcBestCU, rpcTempCU, SIZE_2NxN  );
#endif
            rpcTempCU->initEstData( uiDepth, iQP );
            if(m_pcEncCfg->getUseCbfFastMode() && rpcBestCU->getPartitionSize(0) == SIZE_2NxN)
            {
              doNotBlockPu = rpcBestCU->getQtRootCbf( 0 ) != 0;
            }
          }
#if OL_DEPTHLIMIT
				}//bTryNx2N
			}//depthMapDetect
			else//do things normally
			{
				 if(doNotBlockPu)
          {
#if HHI_INTER_VIEW_RESIDUAL_PRED
            rpcTempCU->setResPredIndicator( bResPredAvailable, bResPredFlag );
#endif
#if HHI_INTERVIEW_SKIP
            xCheckRDCostInter( rpcBestCU, rpcTempCU, SIZE_2NxN, bFullyRenderedSec   );
#else
            xCheckRDCostInter      ( rpcBestCU, rpcTempCU, SIZE_2NxN  );
#endif
            rpcTempCU->initEstData( uiDepth, iQP );
            if(m_pcEncCfg->getUseCbfFastMode() && rpcBestCU->getPartitionSize(0) == SIZE_2NxN)
            {
              doNotBlockPu = rpcBestCU->getQtRootCbf( 0 ) != 0;
            }
          }
  }
#endif
        }

#if 1
        //! Try AMP (SIZE_2NxnU, SIZE_2NxnD, SIZE_nLx2N, SIZE_nRx2N)
        if( pcPic->getSlice(0)->getSPS()->getAMPAcc(uiDepth) )
        {
#if AMP_ENC_SPEEDUP        
          Bool bTestAMP_Hor = false, bTestAMP_Ver = false;

#if AMP_MRG
          Bool bTestMergeAMP_Hor = false, bTestMergeAMP_Ver = false;

          deriveTestModeAMP (rpcBestCU, eParentPartSize, bTestAMP_Hor, bTestAMP_Ver, bTestMergeAMP_Hor, bTestMergeAMP_Ver);
#else
          deriveTestModeAMP (rpcBestCU, eParentPartSize, bTestAMP_Hor, bTestAMP_Ver);
#endif

          //! Do horizontal AMP
          if ( bTestAMP_Hor )
          {
#if OL_DEPTHLIMIT //add code here to select 2NxN or Nx2N or none
#if OL_DO_NOT_LIMIT_INTRA_SLICES_PART
  if(depthMapDetect && !bIntraSliceDetect  && sps->getUseDPL())
#else
  if(depthMapDetect  && sps->getUseDPL()) //depth map being encoded
#endif
			{
				assert(uiDepth == (UInt)texPartInfo[uiPrevTexPartIndex+1]);
				if (bTry2NxN)
				{	
#endif
            if(doNotBlockPu)
            {
#if HHI_INTER_VIEW_RESIDUAL_PRED
              rpcTempCU->setResPredIndicator( bResPredAvailable, bResPredFlag );
#endif
#if HHI_INTERVIEW_SKIP
              xCheckRDCostInter( rpcBestCU, rpcTempCU, SIZE_2NxnU, bFullyRenderedSec );
#else
              xCheckRDCostInter( rpcBestCU, rpcTempCU, SIZE_2NxnU );
#endif
              rpcTempCU->initEstData( uiDepth, iQP );
              if(m_pcEncCfg->getUseCbfFastMode() && rpcBestCU->getPartitionSize(0) == SIZE_2NxnU )
              {
                doNotBlockPu = rpcBestCU->getQtRootCbf( 0 ) != 0;
              }
            }
            if(doNotBlockPu)
            {
#if HHI_INTER_VIEW_RESIDUAL_PRED
              rpcTempCU->setResPredIndicator( bResPredAvailable, bResPredFlag );
#endif
#if HHI_INTERVIEW_SKIP
              xCheckRDCostInter( rpcBestCU, rpcTempCU, SIZE_2NxnD, bFullyRenderedSec );
#else
              xCheckRDCostInter( rpcBestCU, rpcTempCU, SIZE_2NxnD );
#endif
              rpcTempCU->initEstData( uiDepth, iQP );
              if(m_pcEncCfg->getUseCbfFastMode() && rpcBestCU->getPartitionSize(0) == SIZE_2NxnD )
              {
                doNotBlockPu = rpcBestCU->getQtRootCbf( 0 ) != 0;
              }
            }
#if OL_DEPTHLIMIT
				}//bTry2NxN
			}//depthMapDetect
			else//do things normally
			{
            if(doNotBlockPu)
            {
#if HHI_INTER_VIEW_RESIDUAL_PRED
              rpcTempCU->setResPredIndicator( bResPredAvailable, bResPredFlag );
#endif
#if HHI_INTERVIEW_SKIP
              xCheckRDCostInter( rpcBestCU, rpcTempCU, SIZE_2NxnU, bFullyRenderedSec );
#else
              xCheckRDCostInter( rpcBestCU, rpcTempCU, SIZE_2NxnU );
#endif
              rpcTempCU->initEstData( uiDepth, iQP );
              if(m_pcEncCfg->getUseCbfFastMode() && rpcBestCU->getPartitionSize(0) == SIZE_2NxnU )
              {
                doNotBlockPu = rpcBestCU->getQtRootCbf( 0 ) != 0;
              }
            }
            if(doNotBlockPu)
            {
#if HHI_INTER_VIEW_RESIDUAL_PRED
              rpcTempCU->setResPredIndicator( bResPredAvailable, bResPredFlag );
#endif
#if HHI_INTERVIEW_SKIP
              xCheckRDCostInter( rpcBestCU, rpcTempCU, SIZE_2NxnD, bFullyRenderedSec );
#else
              xCheckRDCostInter( rpcBestCU, rpcTempCU, SIZE_2NxnD );
#endif
              rpcTempCU->initEstData( uiDepth, iQP );
              if(m_pcEncCfg->getUseCbfFastMode() && rpcBestCU->getPartitionSize(0) == SIZE_2NxnD )
              {
                doNotBlockPu = rpcBestCU->getQtRootCbf( 0 ) != 0;
              }
            }
          }
#endif
          }
#if AMP_MRG
          else if ( bTestMergeAMP_Hor ) 
          {
#if OL_DEPTHLIMIT //add code here to select 2NxN or Nx2N or none
#if OL_DO_NOT_LIMIT_INTRA_SLICES_PART
  if(depthMapDetect && !bIntraSliceDetect  && sps->getUseDPL())
#else
  if(depthMapDetect  && sps->getUseDPL()) //depth map being encoded
#endif
			{
				assert(uiDepth == (UInt)texPartInfo[uiPrevTexPartIndex+1]);
				if (bTry2NxN)
				{	
#endif
            if(doNotBlockPu)
            {
#if HHI_INTER_VIEW_RESIDUAL_PRED
              rpcTempCU->setResPredIndicator( bResPredAvailable, bResPredFlag );
#endif
#if HHI_INTERVIEW_SKIP
              xCheckRDCostInter( rpcBestCU, rpcTempCU, SIZE_2NxnU, bFullyRenderedSec, true );
#else
              xCheckRDCostInter( rpcBestCU, rpcTempCU, SIZE_2NxnU, true );
#endif
              rpcTempCU->initEstData( uiDepth, iQP );
              if(m_pcEncCfg->getUseCbfFastMode() && rpcBestCU->getPartitionSize(0) == SIZE_2NxnU )
              {
                doNotBlockPu = rpcBestCU->getQtRootCbf( 0 ) != 0;
              }
            }
            if(doNotBlockPu)
            {
#if HHI_INTER_VIEW_RESIDUAL_PRED
              rpcTempCU->setResPredIndicator( bResPredAvailable, bResPredFlag );
#endif
#if HHI_INTERVIEW_SKIP
              xCheckRDCostInter( rpcBestCU, rpcTempCU, SIZE_2NxnD, bFullyRenderedSec, true );
#else
              xCheckRDCostInter( rpcBestCU, rpcTempCU, SIZE_2NxnD, true );
#endif
              rpcTempCU->initEstData( uiDepth, iQP );
              if(m_pcEncCfg->getUseCbfFastMode() && rpcBestCU->getPartitionSize(0) == SIZE_2NxnD )
              {
                doNotBlockPu = rpcBestCU->getQtRootCbf( 0 ) != 0;
              }
            }
#if OL_DEPTHLIMIT
				}//bTry2NxN
			}//depthMapDetect
			else//do things normally
			{
            if(doNotBlockPu)
            {
#if HHI_INTER_VIEW_RESIDUAL_PRED
              rpcTempCU->setResPredIndicator( bResPredAvailable, bResPredFlag );
#endif
#if HHI_INTERVIEW_SKIP
              xCheckRDCostInter( rpcBestCU, rpcTempCU, SIZE_2NxnU, bFullyRenderedSec, true );
#else
              xCheckRDCostInter( rpcBestCU, rpcTempCU, SIZE_2NxnU, true );
#endif
              rpcTempCU->initEstData( uiDepth, iQP );
              if(m_pcEncCfg->getUseCbfFastMode() && rpcBestCU->getPartitionSize(0) == SIZE_2NxnU )
              {
                doNotBlockPu = rpcBestCU->getQtRootCbf( 0 ) != 0;
              }
            }
            if(doNotBlockPu)
            {
#if HHI_INTER_VIEW_RESIDUAL_PRED
              rpcTempCU->setResPredIndicator( bResPredAvailable, bResPredFlag );
#endif
#if HHI_INTERVIEW_SKIP
              xCheckRDCostInter( rpcBestCU, rpcTempCU, SIZE_2NxnD, bFullyRenderedSec, true );
#else
              xCheckRDCostInter( rpcBestCU, rpcTempCU, SIZE_2NxnD, true );
#endif
              rpcTempCU->initEstData( uiDepth, iQP );
              if(m_pcEncCfg->getUseCbfFastMode() && rpcBestCU->getPartitionSize(0) == SIZE_2NxnD )
              {
                doNotBlockPu = rpcBestCU->getQtRootCbf( 0 ) != 0;
              }
            }

			}
#endif
          }
#endif

          //! Do horizontal AMP
          if ( bTestAMP_Ver )
          {
#if OL_DEPTHLIMIT //add code here to select 2NxN or Nx2N or none
#if OL_DO_NOT_LIMIT_INTRA_SLICES_PART
  if(depthMapDetect && !bIntraSliceDetect  && sps->getUseDPL())
#else
  if(depthMapDetect  && sps->getUseDPL()) //depth map being encoded
#endif
			{
				assert(uiDepth == (UInt)texPartInfo[uiPrevTexPartIndex+1]);
				if (bTryNx2N)
				{	
#endif
            if(doNotBlockPu)
            {
#if HHI_INTER_VIEW_RESIDUAL_PRED
              rpcTempCU->setResPredIndicator( bResPredAvailable, bResPredFlag );
#endif
#if HHI_INTERVIEW_SKIP
              xCheckRDCostInter( rpcBestCU, rpcTempCU, SIZE_nLx2N, bFullyRenderedSec );
#else
              xCheckRDCostInter( rpcBestCU, rpcTempCU, SIZE_nLx2N );
#endif
              rpcTempCU->initEstData( uiDepth, iQP );
              if(m_pcEncCfg->getUseCbfFastMode() && rpcBestCU->getPartitionSize(0) == SIZE_nLx2N )
              {
                doNotBlockPu = rpcBestCU->getQtRootCbf( 0 ) != 0;
              }
            }
            if(doNotBlockPu)
            {
#if HHI_INTER_VIEW_RESIDUAL_PRED
              rpcTempCU->setResPredIndicator( bResPredAvailable, bResPredFlag );
#endif
#if HHI_INTERVIEW_SKIP
              xCheckRDCostInter( rpcBestCU, rpcTempCU, SIZE_nRx2N, bFullyRenderedSec );
#else
              xCheckRDCostInter( rpcBestCU, rpcTempCU, SIZE_nRx2N );
#endif
              rpcTempCU->initEstData( uiDepth, iQP );
            }
#if OL_DEPTHLIMIT
				}//bTryNx2N
			}//depthMapDetect
			else//do things normally
			{
            if(doNotBlockPu)
            {
#if HHI_INTER_VIEW_RESIDUAL_PRED
              rpcTempCU->setResPredIndicator( bResPredAvailable, bResPredFlag );
#endif
#if HHI_INTERVIEW_SKIP
              xCheckRDCostInter( rpcBestCU, rpcTempCU, SIZE_nLx2N, bFullyRenderedSec );
#else
              xCheckRDCostInter( rpcBestCU, rpcTempCU, SIZE_nLx2N );
#endif
              rpcTempCU->initEstData( uiDepth, iQP );
              if(m_pcEncCfg->getUseCbfFastMode() && rpcBestCU->getPartitionSize(0) == SIZE_nLx2N )
              {
                doNotBlockPu = rpcBestCU->getQtRootCbf( 0 ) != 0;
              }
            }
            if(doNotBlockPu)
            {
#if HHI_INTER_VIEW_RESIDUAL_PRED
              rpcTempCU->setResPredIndicator( bResPredAvailable, bResPredFlag );
#endif
#if HHI_INTERVIEW_SKIP
              xCheckRDCostInter( rpcBestCU, rpcTempCU, SIZE_nRx2N, bFullyRenderedSec );
#else
              xCheckRDCostInter( rpcBestCU, rpcTempCU, SIZE_nRx2N );
#endif
              rpcTempCU->initEstData( uiDepth, iQP );
            }
          }
#endif
          }
#if AMP_MRG
          else if ( bTestMergeAMP_Ver )
          {
#if OL_DEPTHLIMIT //add code here to select 2NxN or Nx2N or none
#if OL_DO_NOT_LIMIT_INTRA_SLICES_PART
  if(depthMapDetect && !bIntraSliceDetect  && sps->getUseDPL())
#else
  if(depthMapDetect  && sps->getUseDPL()) //depth map being encoded
#endif
			{
				assert(uiDepth == (UInt)texPartInfo[uiPrevTexPartIndex+1]);
				if (bTryNx2N)
				{	
#endif
            if(doNotBlockPu)
            {
#if HHI_INTER_VIEW_RESIDUAL_PRED
              rpcTempCU->setResPredIndicator( bResPredAvailable, bResPredFlag );
#endif
#if HHI_INTERVIEW_SKIP
              xCheckRDCostInter( rpcBestCU, rpcTempCU, SIZE_nLx2N, bFullyRenderedSec, true );
#else
              xCheckRDCostInter( rpcBestCU, rpcTempCU, SIZE_nLx2N, true );
#endif
              rpcTempCU->initEstData( uiDepth, iQP );
              if(m_pcEncCfg->getUseCbfFastMode() && rpcBestCU->getPartitionSize(0) == SIZE_nLx2N )
              {
                doNotBlockPu = rpcBestCU->getQtRootCbf( 0 ) != 0;
              }
            }
            if(doNotBlockPu)
            {
#if HHI_INTER_VIEW_RESIDUAL_PRED
              rpcTempCU->setResPredIndicator( bResPredAvailable, bResPredFlag );
#endif
#if HHI_INTERVIEW_SKIP
              xCheckRDCostInter( rpcBestCU, rpcTempCU, SIZE_nRx2N, bFullyRenderedSec, true );
#else
              xCheckRDCostInter( rpcBestCU, rpcTempCU, SIZE_nRx2N, true );
#endif
              rpcTempCU->initEstData( uiDepth, iQP );
            }
#if OL_DEPTHLIMIT
				}//bTryNx2N
			}//depthMapDetect
			else//do things normally
			{
            if(doNotBlockPu)
            {
#if HHI_INTER_VIEW_RESIDUAL_PRED
              rpcTempCU->setResPredIndicator( bResPredAvailable, bResPredFlag );
#endif
#if HHI_INTERVIEW_SKIP
              xCheckRDCostInter( rpcBestCU, rpcTempCU, SIZE_nLx2N, bFullyRenderedSec, true );
#else
              xCheckRDCostInter( rpcBestCU, rpcTempCU, SIZE_nLx2N, true );
#endif
              rpcTempCU->initEstData( uiDepth, iQP );
              if(m_pcEncCfg->getUseCbfFastMode() && rpcBestCU->getPartitionSize(0) == SIZE_nLx2N )
              {
                doNotBlockPu = rpcBestCU->getQtRootCbf( 0 ) != 0;
              }
            }
            if(doNotBlockPu)
            {
#if HHI_INTER_VIEW_RESIDUAL_PRED
              rpcTempCU->setResPredIndicator( bResPredAvailable, bResPredFlag );
#endif
#if HHI_INTERVIEW_SKIP
              xCheckRDCostInter( rpcBestCU, rpcTempCU, SIZE_nRx2N, bFullyRenderedSec, true );
#else
              xCheckRDCostInter( rpcBestCU, rpcTempCU, SIZE_nRx2N, true );
#endif
              rpcTempCU->initEstData( uiDepth, iQP );
            }
          }
#endif
          }
#endif

#else
#if HHI_INTER_VIEW_RESIDUAL_PRED
              rpcTempCU->setResPredIndicator( bResPredAvailable, bResPredFlag );
#endif
#if HHI_INTERVIEW_SKIP
              xCheckRDCostInter( rpcBestCU, rpcTempCU, SIZE_2NxnU, bFullyRenderedSec );
#else
          xCheckRDCostInter( rpcBestCU, rpcTempCU, SIZE_2NxnU );
#endif
          rpcTempCU->initEstData( uiDepth, iQP );
#if HHI_INTER_VIEW_RESIDUAL_PRED
              rpcTempCU->setResPredIndicator( bResPredAvailable, bResPredFlag );
#endif
#if HHI_INTERVIEW_SKIP
              xCheckRDCostInter( rpcBestCU, rpcTempCU, SIZE_2NxnD, bFullyRenderedSec );
#else
          xCheckRDCostInter( rpcBestCU, rpcTempCU, SIZE_2NxnD );
#endif
          rpcTempCU->initEstData( uiDepth, iQP );
#if HHI_INTER_VIEW_RESIDUAL_PRED
              rpcTempCU->setResPredIndicator( bResPredAvailable, bResPredFlag );
#endif
#if HHI_INTERVIEW_SKIP
              xCheckRDCostInter( rpcBestCU, rpcTempCU, SIZE_nLx2N, bFullyRenderedSec );
#else
          xCheckRDCostInter( rpcBestCU, rpcTempCU, SIZE_nLx2N );
#endif
          rpcTempCU->initEstData( uiDepth, iQP );
#if HHI_INTER_VIEW_RESIDUAL_PRED
              rpcTempCU->setResPredIndicator( bResPredAvailable, bResPredFlag );
#endif
#if HHI_INTERVIEW_SKIP
              xCheckRDCostInter( rpcBestCU, rpcTempCU, SIZE_nRx2N, bFullyRenderedSec );
#else
          xCheckRDCostInter( rpcBestCU, rpcTempCU, SIZE_nRx2N );
#endif
          rpcTempCU->initEstData( uiDepth, iQP );

#endif
        } //! Try AMP (SIZE_2NxnU, SIZE_2NxnD, SIZE_nLx2N, SIZE_nRx2N)
#endif
#if HHI_INTER_VIEW_RESIDUAL_PRED
        } // uiResPrdId
#endif
      } // != I_SLICE

      // initialize PCM flag
      rpcTempCU->setIPCMFlag( 0, false);
      rpcTempCU->setIPCMFlagSubParts ( false, 0, uiDepth); //SUB_LCU_DQP

      // do normal intra modes
      if ( !bEarlySkip )
      {
        // speedup for inter frames
#if HHI_INTERVIEW_SKIP
      if( ( rpcBestCU->getSlice()->getSliceType() == I_SLICE ||
               rpcBestCU->getCbf( 0, TEXT_LUMA     ) != 0   ||
               rpcBestCU->getCbf( 0, TEXT_CHROMA_U ) != 0   ||
               rpcBestCU->getCbf( 0, TEXT_CHROMA_V ) != 0 ) && !bFullyRenderedSec ) // avoid very complex intra if it is unlikely
#else
        if( rpcBestCU->getSlice()->getSliceType() == I_SLICE || 
          rpcBestCU->getCbf( 0, TEXT_LUMA     ) != 0   ||
          rpcBestCU->getCbf( 0, TEXT_CHROMA_U ) != 0   ||
          rpcBestCU->getCbf( 0, TEXT_CHROMA_V ) != 0     ) // avoid very complex intra if it is unlikely
#endif
        {
          xCheckRDCostIntra( rpcBestCU, rpcTempCU, SIZE_2Nx2N );
          rpcTempCU->initEstData( uiDepth, iQP );
          if( uiDepth == g_uiMaxCUDepth - g_uiAddCUDepth )
          {
#if OL_DEPTHLIMIT //add code here to select or deselect NxN mode for Intra
#if OL_DO_NOT_LIMIT_INTRA_SLICES_PART
  if(depthMapDetect && !bIntraSliceDetect  && sps->getUseDPL())
#else
  if(depthMapDetect  && sps->getUseDPL()) //depth map being encoded
#endif
			{
				assert(uiDepth == (UInt)texPartInfo[uiPrevTexPartIndex+1]);
				if (bTrySplit)
				{
			
#endif
            if( rpcTempCU->getWidth(0) > ( 1 << rpcTempCU->getSlice()->getSPS()->getQuadtreeTULog2MinSize() ) )
            {
              xCheckRDCostIntra( rpcBestCU, rpcTempCU, SIZE_NxN   );
              rpcTempCU->initEstData( uiDepth, iQP );
            }
#if OL_DEPTHLIMIT
				}//bTrySplit
			 }//depthMapDetect
			else
			{
				if( rpcTempCU->getWidth(0) > ( 1 << rpcTempCU->getSlice()->getSPS()->getQuadtreeTULog2MinSize() ) )
				{
					xCheckRDCostIntra( rpcBestCU, rpcTempCU, SIZE_NxN   );
					rpcTempCU->initEstData( uiDepth, iQP );
				}
			}
#endif
          }
        }
      }

      // test PCM
      if(pcPic->getSlice(0)->getSPS()->getUsePCM()
        && rpcTempCU->getWidth(0) <= (1<<pcPic->getSlice(0)->getSPS()->getPCMLog2MaxSize())
        && rpcTempCU->getWidth(0) >= (1<<pcPic->getSlice(0)->getSPS()->getPCMLog2MinSize()) )
      {
        UInt uiRawBits = (g_uiBitDepth * rpcBestCU->getWidth(0) * rpcBestCU->getHeight(0) * 3 / 2);
        UInt uiBestBits = rpcBestCU->getTotalBits();
#if HHI_VSO
        Double dRDCostTemp = m_pcRdCost->getUseVSO() ? m_pcRdCost->calcRdCostVSO(uiRawBits, 0) : m_pcRdCost->calcRdCost(uiRawBits, 0);
        if((uiBestBits > uiRawBits) || (rpcBestCU->getTotalCost() > dRDCostTemp ))
#else
        if((uiBestBits > uiRawBits) || (rpcBestCU->getTotalCost() > m_pcRdCost->calcRdCost(uiRawBits, 0)))
#endif
        {
          xCheckIntraPCM (rpcBestCU, rpcTempCU);
          rpcTempCU->initEstData( uiDepth, iQP );
        }
      }
#if HHI_MPI
      if( rpcBestCU->getSlice()->getSPS()->getUseMVI() && rpcBestCU->getSlice()->getSliceType() != I_SLICE )
      {
        xCheckRDCostMvInheritance( rpcBestCU, rpcTempCU, uiDepth, false, false );
        rpcTempCU->initEstData( uiDepth, iQP );
        xCheckRDCostMvInheritance( rpcBestCU, rpcTempCU, uiDepth, true, false );
        rpcTempCU->initEstData( uiDepth, iQP );
      }
#endif
#if LOSSLESS_CODING
      if (isAddLowestQP && (iQP == lowestQP))
      {
        iQP = iMinQP;
      }
#endif
    }

    m_pcEntropyCoder->resetBits();
    m_pcEntropyCoder->encodeSplitFlag( rpcBestCU, 0, uiDepth, true );
    rpcBestCU->getTotalBits() += m_pcEntropyCoder->getNumberOfWrittenBits(); // split bits
    if(m_pcEncCfg->getUseSBACRD())
    {
      rpcBestCU->getTotalBins() += ((TEncBinCABAC *)((TEncSbac*)m_pcEntropyCoder->m_pcEntropyCoderIf)->getEncBinIf())->getBinsCoded();
    }

#if HHI_VSO    
    if ( m_pcRdCost->getUseVSO() )
    {
      rpcBestCU->getTotalCost()  = m_pcRdCost->calcRdCostVSO( rpcBestCU->getTotalBits(), rpcBestCU->getTotalDistortion() );
    }
    else
#endif
    {
    rpcBestCU->getTotalCost()  = m_pcRdCost->calcRdCost( rpcBestCU->getTotalBits(), rpcBestCU->getTotalDistortion() );
    }

    // accumulate statistics for early skip
    if ( m_pcEncCfg->getUseFastEnc() )
    {
      if ( rpcBestCU->isSkipped(0) )
      {
        Int iIdx = g_aucConvertToBit[ rpcBestCU->getWidth(0) ];
        afCost[ iIdx ] += rpcBestCU->getTotalCost();
        aiNum [ iIdx ] ++;
      }
    }

    // Early CU determination
    if( m_pcEncCfg->getUseEarlyCU() && ((*rpcBestCU->getPredictionMode()) == 0) )
    {
      bSubBranch = false;
    }
    else
    {
      bSubBranch = true;
    }
#if HHI_INTERVIEW_SKIP
  rpcBestCU->setRenderableSubParts(bFullyRenderedSec,0,rpcBestCU->getDepth( 0 )) ;
#endif
  }
  else if(!(bSliceEnd && bInsidePicture))
  {
    bBoundary = true;
  }

#if LOSSLESS_CODING 
  // copy orginal YUV samples to PCM buffer
  if( rpcBestCU->isLosslessCoded(0) && (rpcBestCU->getIPCMFlag(0) == false))
  {
    xFillPCMBuffer(rpcBestCU, m_ppcOrigYuv[uiDepth]);
  }
#endif
  if( (g_uiMaxCUWidth>>uiDepth) == rpcTempCU->getSlice()->getPPS()->getMinCuDQPSize() )
  {
    Int idQP = m_pcEncCfg->getMaxDeltaQP();
#if H0736_AVC_STYLE_QP_RANGE
    iMinQP = Clip3( -rpcTempCU->getSlice()->getSPS()->getQpBDOffsetY(), MAX_QP, iBaseQP-idQP );
    iMaxQP = Clip3( -rpcTempCU->getSlice()->getSPS()->getQpBDOffsetY(), MAX_QP, iBaseQP+idQP );
#if LOSSLESS_CODING
    if ( (rpcTempCU->getSlice()->getSPS()->getUseLossless()) && (lowestQP < iMinQP) && rpcTempCU->getSlice()->getPPS()->getUseDQP() )
    {
      isAddLowestQP = true;
      iMinQP = iMinQP - 1;      
    }
#endif
#else
    iMinQP = Clip3( MIN_QP, MAX_QP, iBaseQP-idQP );
    iMaxQP = Clip3( MIN_QP, MAX_QP, iBaseQP+idQP );
#if LOSSLESS_CODING
    if ( (rpcTempCU->getSlice()->getSPS()->getUseLossless()) && (lowestQP < iMinQP) && rpcTempCU->getSlice()->getPPS()->getUseDQP() )
    {
      isAddLowestQP = true;
      iMinQP = iMinQP - 1;
    }
#endif
#endif
  }
  else if( (g_uiMaxCUWidth>>uiDepth) > rpcTempCU->getSlice()->getPPS()->getMinCuDQPSize() )
  {
    iMinQP = iBaseQP;
    iMaxQP = iBaseQP;
  }
  else
  {
    Int iStartQP;
    if( pcPic->getCU( rpcTempCU->getAddr() )->getEntropySliceStartCU(rpcTempCU->getZorderIdxInCU()) == pcSlice->getEntropySliceCurStartCUAddr())
    {
      iStartQP = rpcTempCU->getQP(0);
    }
    else
    {
      UInt uiCurSliceStartPartIdx = pcSlice->getEntropySliceCurStartCUAddr() % pcPic->getNumPartInCU() - rpcTempCU->getZorderIdxInCU();
      iStartQP = rpcTempCU->getQP(uiCurSliceStartPartIdx);
    }
    iMinQP = iStartQP;
    iMaxQP = iStartQP;
  }

  for (Int iQP=iMinQP; iQP<=iMaxQP; iQP++)
  {
#if LOSSLESS_CODING
      if (isAddLowestQP && (iQP == iMinQP))
      {
        iQP = lowestQP;
      }
#endif
    rpcTempCU->initEstData( uiDepth, iQP );

    // further split
    if( bSubBranch && bTrySplitDQP && uiDepth < g_uiMaxCUDepth - g_uiAddCUDepth )
    {
#if HHI_VSO
      // reset Model
      if( m_pcRdCost->getUseRenModel() )
      {
        UInt  uiWidth     = m_ppcOrigYuv[uiDepth]->getWidth ( );
        UInt  uiHeight    = m_ppcOrigYuv[uiDepth]->getHeight( );
        Pel*  piSrc       = m_ppcOrigYuv[uiDepth]->getLumaAddr( 0 );
        UInt  uiSrcStride = m_ppcOrigYuv[uiDepth]->getStride();
        m_pcRdCost->setRenModelData( m_ppcBestCU[uiDepth], 0, piSrc, uiSrcStride, uiWidth, uiHeight );
      }
#endif
      UChar       uhNextDepth         = uiDepth+1;
      TComDataCU* pcSubBestPartCU     = m_ppcBestCU[uhNextDepth];
      TComDataCU* pcSubTempPartCU     = m_ppcTempCU[uhNextDepth];

      for ( UInt uiPartUnitIdx = 0; uiPartUnitIdx < 4; uiPartUnitIdx++ )
      {
        pcSubBestPartCU->initSubCU( rpcTempCU, uiPartUnitIdx, uhNextDepth, iQP );           // clear sub partition datas or init.
        pcSubTempPartCU->initSubCU( rpcTempCU, uiPartUnitIdx, uhNextDepth, iQP );           // clear sub partition datas or init.

        Bool bInSlice = pcSubBestPartCU->getSCUAddr()+pcSubBestPartCU->getTotalNumPart()>pcSlice->getEntropySliceCurStartCUAddr()&&pcSubBestPartCU->getSCUAddr()<pcSlice->getEntropySliceCurEndCUAddr();
        if(bInSlice && ( pcSubBestPartCU->getCUPelX() < pcSlice->getSPS()->getPicWidthInLumaSamples() ) && ( pcSubBestPartCU->getCUPelY() < pcSlice->getSPS()->getPicHeightInLumaSamples() ) )
        {
          if( m_bUseSBACRD )
          {
            if ( 0 == uiPartUnitIdx) //initialize RD with previous depth buffer
            {
              m_pppcRDSbacCoder[uhNextDepth][CI_CURR_BEST]->load(m_pppcRDSbacCoder[uiDepth][CI_CURR_BEST]);
            }
            else
            {
              m_pppcRDSbacCoder[uhNextDepth][CI_CURR_BEST]->load(m_pppcRDSbacCoder[uhNextDepth][CI_NEXT_BEST]);
            }
          }

#if AMP_ENC_SPEEDUP
          if ( rpcBestCU->isIntra(0) )
          {
            xCompressCU( pcSubBestPartCU, pcSubTempPartCU, uhNextDepth, SIZE_NONE );
          }
          else
          {
            xCompressCU( pcSubBestPartCU, pcSubTempPartCU, uhNextDepth, rpcBestCU->getPartitionSize(0) );
          }
#else
          xCompressCU( pcSubBestPartCU, pcSubTempPartCU, uhNextDepth );
#endif

          rpcTempCU->copyPartFrom( pcSubBestPartCU, uiPartUnitIdx, uhNextDepth );         // Keep best part data to current temporary data.
          xCopyYuv2Tmp( pcSubBestPartCU->getTotalNumPart()*uiPartUnitIdx, uhNextDepth );

#if HHI_VSO
#if HHI_VSO_SET_OPTIM 
#else 
          if( m_pcRdCost->getUseRenModel() ) // necessary ??
          {
            UInt  uiWidth     = m_ppcRecoYuvBest[uhNextDepth]->getWidth   (  );
            UInt  uiHeight    = m_ppcRecoYuvBest[uhNextDepth]->getHeight  (   );
            Pel*  piSrc       = m_ppcRecoYuvBest[uhNextDepth]->getLumaAddr( 0 );
            UInt  uiSrcStride = m_ppcRecoYuvBest[uhNextDepth]->getStride  (   );
            m_pcRdCost->setRenModelData( pcSubBestPartCU, 0, piSrc, uiSrcStride, uiWidth, uiHeight );
          }
#endif
#endif
        }
        else if (bInSlice)
        {
          pcSubBestPartCU->copyToPic( uhNextDepth );
          rpcTempCU->copyPartFrom( pcSubBestPartCU, uiPartUnitIdx, uhNextDepth );
        }
      }

      if( !bBoundary )
      {
        m_pcEntropyCoder->resetBits();
        m_pcEntropyCoder->encodeSplitFlag( rpcTempCU, 0, uiDepth, true );

        rpcTempCU->getTotalBits() += m_pcEntropyCoder->getNumberOfWrittenBits(); // split bits
        if(m_pcEncCfg->getUseSBACRD())
        {
          rpcTempCU->getTotalBins() += ((TEncBinCABAC *)((TEncSbac*)m_pcEntropyCoder->m_pcEntropyCoderIf)->getEncBinIf())->getBinsCoded();
        }
      }
#if HHI_INTERVIEW_SKIP_LAMBDA_SCALE
      if( bFullyRenderedSec )
      {
        m_pcRdCost->setLambdaScale( m_pcEncCfg->getInterViewSkipLambdaScale() );
      }
      else
      {
        m_pcRdCost->setLambdaScale( 1 );
      }
#endif

#if HHI_VSO
      if ( m_pcRdCost->getUseVSO() )
      {
        rpcTempCU->getTotalCost()  = m_pcRdCost->calcRdCostVSO( rpcTempCU->getTotalBits(), rpcTempCU->getTotalDistortion() );
      }
      else
#endif
      {           
      rpcTempCU->getTotalCost()  = m_pcRdCost->calcRdCost( rpcTempCU->getTotalBits(), rpcTempCU->getTotalDistortion() );
      }

      if( (g_uiMaxCUWidth>>uiDepth) == rpcTempCU->getSlice()->getPPS()->getMinCuDQPSize() && rpcTempCU->getSlice()->getPPS()->getUseDQP())
      {
        Bool bHasRedisual = false;
        for( UInt uiBlkIdx = 0; uiBlkIdx < rpcTempCU->getTotalNumPart(); uiBlkIdx ++)
        {
          if( ( pcPic->getCU( rpcTempCU->getAddr() )->getEntropySliceStartCU(uiBlkIdx+rpcTempCU->getZorderIdxInCU()) == rpcTempCU->getSlice()->getEntropySliceCurStartCUAddr() ) && 
              ( rpcTempCU->getCbf( uiBlkIdx, TEXT_LUMA ) || rpcTempCU->getCbf( uiBlkIdx, TEXT_CHROMA_U ) || rpcTempCU->getCbf( uiBlkIdx, TEXT_CHROMA_V ) ) )
          {
            bHasRedisual = true;
            break;
          }
        }

        UInt uiTargetPartIdx;
        if ( pcPic->getCU( rpcTempCU->getAddr() )->getEntropySliceStartCU(rpcTempCU->getZorderIdxInCU()) != pcSlice->getEntropySliceCurStartCUAddr() )
        {
          uiTargetPartIdx = pcSlice->getEntropySliceCurStartCUAddr() % pcPic->getNumPartInCU() - rpcTempCU->getZorderIdxInCU();
        }
        else
        {
          uiTargetPartIdx = 0;
        }
        if ( bHasRedisual )
        {
#if !RDO_WITHOUT_DQP_BITS
          m_pcEntropyCoder->resetBits();
          m_pcEntropyCoder->encodeQP( rpcTempCU, uiTargetPartIdx, false );
          rpcTempCU->getTotalBits() += m_pcEntropyCoder->getNumberOfWrittenBits(); // dQP bits
          if(m_pcEncCfg->getUseSBACRD())
          {
            rpcTempCU->getTotalBins() += ((TEncBinCABAC *)((TEncSbac*)m_pcEntropyCoder->m_pcEntropyCoderIf)->getEncBinIf())->getBinsCoded();
          }
#if HHI_VSO
          if ( m_pcRdCost->getUseLambdaScaleVSO())
          {
            rpcTempCU->getTotalCost()  = m_pcRdCost->calcRdCostVSO( rpcTempCU->getTotalBits(), rpcTempCU->getTotalDistortion() );
          }
          else
#endif
          {
          rpcTempCU->getTotalCost()  = m_pcRdCost->calcRdCost( rpcTempCU->getTotalBits(), rpcTempCU->getTotalDistortion() );
          }
#endif
        }
        else
        {
#if LOSSLESS_CODING
          if (((rpcTempCU->getQP(uiTargetPartIdx) != rpcTempCU->getRefQP(uiTargetPartIdx)) ) && (rpcTempCU->getSlice()->getSPS()->getUseLossless()))
          {
            rpcTempCU->getTotalCost() = MAX_DOUBLE;
          }
#endif
          rpcTempCU->setQPSubParts( rpcTempCU->getRefQP( uiTargetPartIdx ), 0, uiDepth ); // set QP to default QP
        }
      }

      if( m_bUseSBACRD )
      {
        m_pppcRDSbacCoder[uhNextDepth][CI_NEXT_BEST]->store(m_pppcRDSbacCoder[uiDepth][CI_TEMP_BEST]);
      }
      Bool bEntropyLimit=false;
      Bool bSliceLimit=false;
      bSliceLimit=rpcBestCU->getSlice()->getSliceMode()==AD_HOC_SLICES_FIXED_NUMBER_OF_BYTES_IN_SLICE&&(rpcBestCU->getTotalBits()>rpcBestCU->getSlice()->getSliceArgument()<<3);
      if(rpcBestCU->getSlice()->getEntropySliceMode()==SHARP_MULTIPLE_CONSTRAINT_BASED_ENTROPY_SLICE&&m_pcEncCfg->getUseSBACRD())
      {
        if(rpcBestCU->getTotalBins()>rpcBestCU->getSlice()->getEntropySliceArgument())
        {
          bEntropyLimit=true;
        }
      }
      else if(rpcBestCU->getSlice()->getEntropySliceMode()==SHARP_MULTIPLE_CONSTRAINT_BASED_ENTROPY_SLICE)
      {
        if(rpcBestCU->getTotalBits()>rpcBestCU->getSlice()->getEntropySliceArgument())
        {
          bEntropyLimit=true;
        }
      }
      if(rpcBestCU->getDepth(0)>=rpcBestCU->getSlice()->getPPS()->getSliceGranularity())
      {
        bSliceLimit=false;
        bEntropyLimit=false;
      }
      if(bSliceLimit||bEntropyLimit)
      {
        rpcBestCU->getTotalCost()=rpcTempCU->getTotalCost()+1;
      }
      xCheckBestMode( rpcBestCU, rpcTempCU, uiDepth);                                  // RD compare current larger prediction
    }                                                                                  // with sub partitioned prediction.
#if LOSSLESS_CODING
      if (isAddLowestQP && (iQP == lowestQP))
      {
        iQP = iMinQP;
      }
#endif
  } // SPLIT- QP Loop

#if HHI_VSO
  if( m_pcRdCost->getUseRenModel() )
  {
      UInt  uiWidth     = m_ppcRecoYuvBest[uiDepth]->getWidth   ( );
      UInt  uiHeight    = m_ppcRecoYuvBest[uiDepth]->getHeight  ( );
      Pel*  piSrc       = m_ppcRecoYuvBest[uiDepth]->getLumaAddr( 0 );
      UInt  uiSrcStride = m_ppcRecoYuvBest[uiDepth]->getStride  ( );
      m_pcRdCost->setRenModelData( rpcBestCU, 0, piSrc, uiSrcStride, uiWidth, uiHeight );
  }
#endif

  rpcBestCU->copyToPic(uiDepth);                                                     // Copy Best data to Picture for next partition prediction.

  xCopyYuv2Pic( rpcBestCU->getPic(), rpcBestCU->getAddr(), rpcBestCU->getZorderIdxInCU(), uiDepth, uiDepth, rpcBestCU, uiLPelX, uiTPelY );   // Copy Yuv data to picture Yuv
  if( bBoundary ||(bSliceEnd && bInsidePicture))
  {
    return;
  }

  // Assert if Best prediction mode is NONE
  // Selected mode's RD-cost must be not MAX_DOUBLE.
  assert( rpcBestCU->getPartitionSize ( 0 ) != SIZE_NONE  );
  assert( rpcBestCU->getPredictionMode( 0 ) != MODE_NONE  );
  assert( rpcBestCU->getTotalCost     (   ) != MAX_DOUBLE );
}

/** finish encoding a cu and handle end-of-slice conditions
 * \param pcCU
 * \param uiAbsPartIdx
 * \param uiDepth 
 * \returns Void
 */
Void TEncCu::finishCU( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth )
{
  TComPic* pcPic = pcCU->getPic();
  TComSlice * pcSlice = pcCU->getPic()->getSlice(pcCU->getPic()->getCurrSliceIdx());

  //Calculate end address
  UInt uiCUAddr = pcCU->getSCUAddr()+uiAbsPartIdx;

  UInt uiInternalAddress = pcPic->getPicSym()->getPicSCUAddr(pcSlice->getEntropySliceCurEndCUAddr()-1) % pcPic->getNumPartInCU();
  UInt uiExternalAddress = pcPic->getPicSym()->getPicSCUAddr(pcSlice->getEntropySliceCurEndCUAddr()-1) / pcPic->getNumPartInCU();
  UInt uiPosX = ( uiExternalAddress % pcPic->getFrameWidthInCU() ) * g_uiMaxCUWidth+ g_auiRasterToPelX[ g_auiZscanToRaster[uiInternalAddress] ];
  UInt uiPosY = ( uiExternalAddress / pcPic->getFrameWidthInCU() ) * g_uiMaxCUHeight+ g_auiRasterToPelY[ g_auiZscanToRaster[uiInternalAddress] ];
  UInt uiWidth = pcSlice->getSPS()->getPicWidthInLumaSamples();
  UInt uiHeight = pcSlice->getSPS()->getPicHeightInLumaSamples();
  while(uiPosX>=uiWidth||uiPosY>=uiHeight)
  {
    uiInternalAddress--;
    uiPosX = ( uiExternalAddress % pcPic->getFrameWidthInCU() ) * g_uiMaxCUWidth+ g_auiRasterToPelX[ g_auiZscanToRaster[uiInternalAddress] ];
    uiPosY = ( uiExternalAddress / pcPic->getFrameWidthInCU() ) * g_uiMaxCUHeight+ g_auiRasterToPelY[ g_auiZscanToRaster[uiInternalAddress] ];
  }
  uiInternalAddress++;
  if(uiInternalAddress==pcCU->getPic()->getNumPartInCU())
  {
    uiInternalAddress = 0;
    uiExternalAddress = pcPic->getPicSym()->getCUOrderMap(pcPic->getPicSym()->getInverseCUOrderMap(uiExternalAddress)+1);
  }
  UInt uiRealEndAddress = pcPic->getPicSym()->getPicSCUEncOrder(uiExternalAddress*pcPic->getNumPartInCU()+uiInternalAddress);

  // Encode slice finish
  Bool bTerminateSlice = false;
  if (uiCUAddr+(pcCU->getPic()->getNumPartInCU()>>(uiDepth<<1)) == uiRealEndAddress)
  {
    bTerminateSlice = true;
  }
  UInt uiGranularityWidth = g_uiMaxCUWidth>>(pcSlice->getPPS()->getSliceGranularity());
  uiPosX = pcCU->getCUPelX() + g_auiRasterToPelX[ g_auiZscanToRaster[uiAbsPartIdx] ];
  uiPosY = pcCU->getCUPelY() + g_auiRasterToPelY[ g_auiZscanToRaster[uiAbsPartIdx] ];
  Bool granularityBoundary=((uiPosX+pcCU->getWidth(uiAbsPartIdx))%uiGranularityWidth==0||(uiPosX+pcCU->getWidth(uiAbsPartIdx)==uiWidth))
    &&((uiPosY+pcCU->getHeight(uiAbsPartIdx))%uiGranularityWidth==0||(uiPosY+pcCU->getHeight(uiAbsPartIdx)==uiHeight));
  
#if BURST_IPCM
  if(granularityBoundary && (!(pcCU->getIPCMFlag(uiAbsPartIdx) && ( pcCU->getNumSucIPCM() > 1 ))))
#else
  if(granularityBoundary)
#endif
  {
    // The 1-terminating bit is added to all streams, so don't add it here when it's 1.
    if (!bTerminateSlice)
      m_pcEntropyCoder->encodeTerminatingBit( bTerminateSlice ? 1 : 0 );
  }
  
  Int numberOfWrittenBits = 0;
  if (m_pcBitCounter)
  {
    numberOfWrittenBits = m_pcEntropyCoder->getNumberOfWrittenBits();
  }
  
  // Calculate slice end IF this CU puts us over slice bit size.
  unsigned iGranularitySize = pcCU->getPic()->getNumPartInCU()>>(pcSlice->getPPS()->getSliceGranularity()<<1);
  int iGranularityEnd = ((pcCU->getSCUAddr()+uiAbsPartIdx)/iGranularitySize)*iGranularitySize;
  if(iGranularityEnd<=pcSlice->getEntropySliceCurStartCUAddr()) 
  {
    iGranularityEnd+=max(iGranularitySize,(pcCU->getPic()->getNumPartInCU()>>(uiDepth<<1)));
  }
  // Set slice end parameter
  if(pcSlice->getSliceMode()==AD_HOC_SLICES_FIXED_NUMBER_OF_BYTES_IN_SLICE&&!pcSlice->getFinalized()&&pcSlice->getSliceBits()+numberOfWrittenBits>pcSlice->getSliceArgument()<<3) 
  {
    pcSlice->setEntropySliceCurEndCUAddr(iGranularityEnd);
    pcSlice->setSliceCurEndCUAddr(iGranularityEnd);
    return;
  }
  // Set entropy slice end parameter
  if(m_pcEncCfg->getUseSBACRD()) 
  {
    TEncBinCABAC *pppcRDSbacCoder = (TEncBinCABAC *) m_pppcRDSbacCoder[0][CI_CURR_BEST]->getEncBinIf();
    UInt uiBinsCoded = pppcRDSbacCoder->getBinsCoded();
    if(pcSlice->getEntropySliceMode()==SHARP_MULTIPLE_CONSTRAINT_BASED_ENTROPY_SLICE&&!pcSlice->getFinalized()&&pcSlice->getEntropySliceCounter()+uiBinsCoded>pcSlice->getEntropySliceArgument())
    {
      pcSlice->setEntropySliceCurEndCUAddr(iGranularityEnd);
      return;
    }
  }
  else
  {
    if(pcSlice->getEntropySliceMode()==SHARP_MULTIPLE_CONSTRAINT_BASED_ENTROPY_SLICE&&!pcSlice->getFinalized()&&pcSlice->getEntropySliceCounter()+numberOfWrittenBits>pcSlice->getEntropySliceArgument()) 
    {
      pcSlice->setEntropySliceCurEndCUAddr(iGranularityEnd);
      return;
    }
  }
  if(granularityBoundary)
  {
    pcSlice->setSliceBits( (UInt)(pcSlice->getSliceBits() + numberOfWrittenBits) );
    if(m_pcEncCfg->getUseSBACRD()) 
    {
      TEncBinCABAC *pppcRDSbacCoder = (TEncBinCABAC *) m_pppcRDSbacCoder[0][CI_CURR_BEST]->getEncBinIf();
      pcSlice->setEntropySliceCounter(pcSlice->getEntropySliceCounter()+pppcRDSbacCoder->getBinsCoded());
      pppcRDSbacCoder->setBinsCoded( 0 );
    }
    else 
    {
      pcSlice->setEntropySliceCounter(pcSlice->getEntropySliceCounter()+numberOfWrittenBits);
    }
    if (m_pcBitCounter)
    {
      m_pcEntropyCoder->resetBits();      
    }
  }
}

/** Compute QP for each CU
 * \param pcCU Target CU
 * \param uiDepth CU depth
 * \returns quantization parameter
 */
Int TEncCu::xComputeQP( TComDataCU* pcCU, UInt uiDepth )
{
  Int iBaseQp = pcCU->getSlice()->getSliceQp();
  Int iQpOffset = 0;
  if ( m_pcEncCfg->getUseAdaptiveQP() )
  {
    TEncPic* pcEPic = dynamic_cast<TEncPic*>( pcCU->getPic() );
    UInt uiAQDepth = min( uiDepth, pcEPic->getMaxAQDepth()-1 );
    TEncPicQPAdaptationLayer* pcAQLayer = pcEPic->getAQLayer( uiAQDepth );
    UInt uiAQUPosX = pcCU->getCUPelX() / pcAQLayer->getAQPartWidth();
    UInt uiAQUPosY = pcCU->getCUPelY() / pcAQLayer->getAQPartHeight();
    UInt uiAQUStride = pcAQLayer->getAQPartStride();
    TEncQPAdaptationUnit* acAQU = pcAQLayer->getQPAdaptationUnit();

    Double dMaxQScale = pow(2.0, m_pcEncCfg->getQPAdaptationRange()/6.0);
    Double dAvgAct = pcAQLayer->getAvgActivity();
    Double dCUAct = acAQU[uiAQUPosY * uiAQUStride + uiAQUPosX].getActivity();
    Double dNormAct = (dMaxQScale*dCUAct + dAvgAct) / (dCUAct + dMaxQScale*dAvgAct);
    Double dQpOffset = log(dNormAct) / log(2.0) * 6.0;
    iQpOffset = Int(floor( dQpOffset + 0.49999 ));
  }
#if H0736_AVC_STYLE_QP_RANGE
  return Clip3(-pcCU->getSlice()->getSPS()->getQpBDOffsetY(), MAX_QP, iBaseQp+iQpOffset );
#else
  return Clip3( MIN_QP, MAX_QP, iBaseQp+iQpOffset );
#endif
}

/** encode a CU block recursively
 * \param pcCU
 * \param uiAbsPartIdx
 * \param uiDepth 
 * \returns Void
 */
Void TEncCu::xEncodeCU( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth )
{
  TComPic* pcPic = pcCU->getPic();
  
  Bool bBoundary = false;
  UInt uiLPelX   = pcCU->getCUPelX() + g_auiRasterToPelX[ g_auiZscanToRaster[uiAbsPartIdx] ];
  UInt uiRPelX   = uiLPelX + (g_uiMaxCUWidth>>uiDepth)  - 1;
  UInt uiTPelY   = pcCU->getCUPelY() + g_auiRasterToPelY[ g_auiZscanToRaster[uiAbsPartIdx] ];
  UInt uiBPelY   = uiTPelY + (g_uiMaxCUHeight>>uiDepth) - 1;
  
#if BURST_IPCM
  if( getCheckBurstIPCMFlag() )
  {
    pcCU->setLastCUSucIPCMFlag( checkLastCUSucIPCM( pcCU, uiAbsPartIdx ));
    pcCU->setNumSucIPCM( countNumSucIPCM ( pcCU, uiAbsPartIdx ) );
  }
#endif

  TComSlice * pcSlice = pcCU->getPic()->getSlice(pcCU->getPic()->getCurrSliceIdx());
  // If slice start is within this cu...
  Bool bSliceStart = pcSlice->getEntropySliceCurStartCUAddr() > pcPic->getPicSym()->getInverseCUOrderMap(pcCU->getAddr())*pcCU->getPic()->getNumPartInCU()+uiAbsPartIdx && 
    pcSlice->getEntropySliceCurStartCUAddr() < pcPic->getPicSym()->getInverseCUOrderMap(pcCU->getAddr())*pcCU->getPic()->getNumPartInCU()+uiAbsPartIdx+( pcPic->getNumPartInCU() >> (uiDepth<<1) );
  // We need to split, so don't try these modes.
  if(!bSliceStart&&( uiRPelX < pcSlice->getSPS()->getPicWidthInLumaSamples() ) && ( uiBPelY < pcSlice->getSPS()->getPicHeightInLumaSamples() ) )
  {
#if HHI_MPI
    if( pcCU->getTextureModeDepth( uiAbsPartIdx ) == -1 || uiDepth < pcCU->getTextureModeDepth( uiAbsPartIdx ) )
    {
#endif

    m_pcEntropyCoder->encodeSplitFlag( pcCU, uiAbsPartIdx, uiDepth );

#if HHI_MPI
    }
#endif
  }
  else
  {
    bBoundary = true;
  }
  
#if HHI_MPI
  if( uiDepth == pcCU->getTextureModeDepth( uiAbsPartIdx ) )
  {
    xSaveDepthWidthHeight( pcCU );
    pcCU->setSizeSubParts( g_uiMaxCUWidth>>uiDepth, g_uiMaxCUHeight>>uiDepth, uiAbsPartIdx, uiDepth );
    pcCU->setDepthSubParts( uiDepth, uiAbsPartIdx );

    if( ( uiRPelX < pcSlice->getSPS()->getPicWidthInLumaSamples() ) && ( uiBPelY < pcSlice->getSPS()->getPicHeightInLumaSamples() ) )
	{
      m_pcEntropyCoder->encodeSplitFlag( pcCU, uiAbsPartIdx, uiDepth );
	}
    if( !pcCU->getSlice()->isIntra() )
    {
      m_pcEntropyCoder->encodeSkipFlag( pcCU, uiAbsPartIdx );
    }

    if( pcCU->isSkipped( uiAbsPartIdx ) )
    {
      m_pcEntropyCoder->encodeMergeIndex( pcCU, uiAbsPartIdx, 0 );
      finishCU(pcCU,uiAbsPartIdx,uiDepth);
      xRestoreDepthWidthHeight( pcCU );
      return;
    }

    m_pcEntropyCoder->encodePredMode( pcCU, uiAbsPartIdx );

    m_pcEntropyCoder->encodePartSize( pcCU, uiAbsPartIdx, uiDepth );

    // prediction Info ( Intra : direction mode, Inter : Mv, reference idx )
    m_pcEntropyCoder->encodePredInfo( pcCU, uiAbsPartIdx );
    xRestoreDepthWidthHeight( pcCU );
  }
#endif

  if( ( ( uiDepth < pcCU->getDepth( uiAbsPartIdx ) ) && ( uiDepth < (g_uiMaxCUDepth-g_uiAddCUDepth) ) ) || bBoundary )
  {
    UInt uiQNumParts = ( pcPic->getNumPartInCU() >> (uiDepth<<1) )>>2;
    if( (g_uiMaxCUWidth>>uiDepth) == pcCU->getSlice()->getPPS()->getMinCuDQPSize() && pcCU->getSlice()->getPPS()->getUseDQP())
    {
      setdQPFlag(true);
    }
#if BURST_IPCM
    pcCU->setNumSucIPCM(0);
    pcCU->setLastCUSucIPCMFlag(false);
#endif
    for ( UInt uiPartUnitIdx = 0; uiPartUnitIdx < 4; uiPartUnitIdx++, uiAbsPartIdx+=uiQNumParts )
    {
      uiLPelX   = pcCU->getCUPelX() + g_auiRasterToPelX[ g_auiZscanToRaster[uiAbsPartIdx] ];
      uiTPelY   = pcCU->getCUPelY() + g_auiRasterToPelY[ g_auiZscanToRaster[uiAbsPartIdx] ];
      Bool bInSlice = pcCU->getSCUAddr()+uiAbsPartIdx+uiQNumParts>pcSlice->getEntropySliceCurStartCUAddr()&&pcCU->getSCUAddr()+uiAbsPartIdx<pcSlice->getEntropySliceCurEndCUAddr();
      if(bInSlice&&( uiLPelX < pcSlice->getSPS()->getPicWidthInLumaSamples() ) && ( uiTPelY < pcSlice->getSPS()->getPicHeightInLumaSamples() ) )
      {
        xEncodeCU( pcCU, uiAbsPartIdx, uiDepth+1 );
      }
    }
    return;
  }
  
  if( (g_uiMaxCUWidth>>uiDepth) >= pcCU->getSlice()->getPPS()->getMinCuDQPSize() && pcCU->getSlice()->getPPS()->getUseDQP())
  {
    setdQPFlag(true);
  }
#if HHI_MPI
  if( pcCU->getTextureModeDepth( uiAbsPartIdx ) == -1 )
{
#endif
  if( !pcCU->getSlice()->isIntra() )
  {
    m_pcEntropyCoder->encodeSkipFlag( pcCU, uiAbsPartIdx );
  }
#if HHI_MPI
}
#endif
  
  if( pcCU->isSkipped( uiAbsPartIdx ) )
  {
#if OL_DEPTHLIMIT
	if(pcCU->getPartDumpFlag())
	{
		pcCU->updatePartInfo(0,uiDepth);
		pcCU->incrementPartInfo();
	}
#endif
    m_pcEntropyCoder->encodeMergeIndex( pcCU, uiAbsPartIdx, 0 );
#if HHI_INTER_VIEW_RESIDUAL_PRED
    m_pcEntropyCoder->encodeResPredFlag( pcCU, uiAbsPartIdx, 0 );
#endif
    finishCU(pcCU,uiAbsPartIdx,uiDepth);
    return;
  }
#if HHI_MPI
  if( pcCU->getTextureModeDepth( uiAbsPartIdx ) == -1 )
  {
#endif
  m_pcEntropyCoder->encodePredMode( pcCU, uiAbsPartIdx );
  
  m_pcEntropyCoder->encodePartSize( pcCU, uiAbsPartIdx, uiDepth );
  
  if (pcCU->isIntra( uiAbsPartIdx ) && pcCU->getPartitionSize( uiAbsPartIdx ) == SIZE_2Nx2N )
  {
    m_pcEntropyCoder->encodeIPCMInfo( pcCU, uiAbsPartIdx );

    if(pcCU->getIPCMFlag(uiAbsPartIdx))
    {
      // Encode slice finish
      finishCU(pcCU,uiAbsPartIdx,uiDepth);
      return;
    }
  }

  // prediction Info ( Intra : direction mode, Inter : Mv, reference idx )
  m_pcEntropyCoder->encodePredInfo( pcCU, uiAbsPartIdx );
#if HHI_INTER_VIEW_RESIDUAL_PRED
    if( !pcCU->isIntra( uiAbsPartIdx ) )
    {
      m_pcEntropyCoder->encodeResPredFlag( pcCU, uiAbsPartIdx, 0 );
    }
#endif
#if HHI_MPI
  }
#endif
  
  // Encode Coefficients
  Bool bCodeDQP = getdQPFlag();
  m_pcEntropyCoder->encodeCoeff( pcCU, uiAbsPartIdx, uiDepth, pcCU->getWidth (uiAbsPartIdx), pcCU->getHeight(uiAbsPartIdx), bCodeDQP );
  setdQPFlag( bCodeDQP );

  // --- write terminating bit ---
  finishCU(pcCU,uiAbsPartIdx,uiDepth);
}

/** check RD costs for a CU block encoded with merge
 * \param rpcBestCU
 * \param rpcTempCU
 * \returns Void
 */
#if HHI_INTERVIEW_SKIP
Void TEncCu::xCheckRDCostMerge2Nx2N( TComDataCU*& rpcBestCU, TComDataCU*& rpcTempCU, Bool bSkipRes )
#else
Void TEncCu::xCheckRDCostMerge2Nx2N( TComDataCU*& rpcBestCU, TComDataCU*& rpcTempCU )
#endif
{
  assert( rpcTempCU->getSlice()->getSliceType() != I_SLICE );
#if HHI_INTER_VIEW_MOTION_PRED
  TComMvField  cMvFieldNeighbours[MRG_MAX_NUM_CANDS_MEM << 1]; // double length for mv of both lists
  UChar uhInterDirNeighbours[MRG_MAX_NUM_CANDS_MEM];
#else
  TComMvField  cMvFieldNeighbours[MRG_MAX_NUM_CANDS << 1]; // double length for mv of both lists
  UChar uhInterDirNeighbours[MRG_MAX_NUM_CANDS];
#endif
  Int numValidMergeCand = 0;

#if HHI_INTER_VIEW_RESIDUAL_PRED
  Bool  bResPrdAvail  = rpcTempCU->getResPredAvail( 0 );
  Bool  bResPrdFlag   = rpcTempCU->getResPredFlag ( 0 );
#endif

#if HHI_INTER_VIEW_MOTION_PRED
  for( UInt ui = 0; ui < MRG_MAX_NUM_CANDS_MEM; ++ui )
#else
  for( UInt ui = 0; ui < MRG_MAX_NUM_CANDS; ++ui )
#endif
  {
    uhInterDirNeighbours[ui] = 0;
  }
  UChar uhDepth = rpcTempCU->getDepth( 0 );

#if HHI_VSO
  if( m_pcRdCost->getUseRenModel() )
  {
    UInt  uiWidth     = m_ppcOrigYuv[uhDepth]->getWidth ( );
    UInt  uiHeight    = m_ppcOrigYuv[uhDepth]->getHeight( );
    Pel*  piSrc       = m_ppcOrigYuv[uhDepth]->getLumaAddr( );
    UInt  uiSrcStride = m_ppcOrigYuv[uhDepth]->getStride();
    m_pcRdCost->setRenModelData( rpcTempCU, 0, piSrc, uiSrcStride, uiWidth, uiHeight );
  }
#endif

  rpcTempCU->setPartSizeSubParts( SIZE_2Nx2N, 0, uhDepth ); // interprets depth relative to LCU level
  rpcTempCU->getInterMergeCandidates( 0, 0, uhDepth, cMvFieldNeighbours,uhInterDirNeighbours, numValidMergeCand );

#if FAST_DECISION_FOR_MRG_RD_COST
  Bool bestIsSkip = false;
#endif
  
  for( UInt uiMergeCand = 0; uiMergeCand < numValidMergeCand; ++uiMergeCand )
  {
    {
      TComYuv* pcPredYuvTemp = NULL;
#if LOSSLESS_CODING
      UInt iteration;
      if ( rpcTempCU->isLosslessCoded(0))
      {
        iteration = 1;
      }
      else 
      {
        iteration = 2;
      }

#if HHI_INTERVIEW_SKIP
    for( UInt uiNoResidual = (bSkipRes ? 1:0); uiNoResidual < iteration; ++uiNoResidual )
#else
      for( UInt uiNoResidual = 0; uiNoResidual < iteration; ++uiNoResidual )
#endif
#else
#if HHI_INTERVIEW_SKIP
    for( UInt uiNoResidual = (bSkipRes ? 1:0); uiNoResidual < 2; ++uiNoResidual )
#else
      for( UInt uiNoResidual = 0; uiNoResidual < 2; ++uiNoResidual )
#endif
#endif
      {
#if FAST_DECISION_FOR_MRG_RD_COST
        if( !(bestIsSkip && uiNoResidual == 0) )
        {
#endif
          // set MC parameters
          rpcTempCU->setPredModeSubParts( MODE_SKIP, 0, uhDepth ); // interprets depth relative to LCU level
          rpcTempCU->setPartSizeSubParts( SIZE_2Nx2N, 0, uhDepth ); // interprets depth relative to LCU level
          rpcTempCU->setMergeFlagSubParts( true, 0, 0, uhDepth ); // interprets depth relative to LCU level
          rpcTempCU->setMergeIndexSubParts( uiMergeCand, 0, 0, uhDepth ); // interprets depth relative to LCU level
          rpcTempCU->setInterDirSubParts( uhInterDirNeighbours[uiMergeCand], 0, 0, uhDepth ); // interprets depth relative to LCU level
          rpcTempCU->getCUMvField( REF_PIC_LIST_0 )->setAllMvField( cMvFieldNeighbours[0 + 2*uiMergeCand], SIZE_2Nx2N, 0, 0 ); // interprets depth relative to rpcTempCU level
          rpcTempCU->getCUMvField( REF_PIC_LIST_1 )->setAllMvField( cMvFieldNeighbours[1 + 2*uiMergeCand], SIZE_2Nx2N, 0, 0 ); // interprets depth relative to rpcTempCU level

#if HHI_INTER_VIEW_RESIDUAL_PRED
          rpcTempCU->setResPredAvailSubParts( bResPrdAvail, 0, 0, uhDepth );
          rpcTempCU->setResPredFlagSubParts ( bResPrdFlag,  0, 0, uhDepth );
#endif

          // do MC
#if HHI_INTERVIEW_SKIP
      if ( (uiNoResidual == 0) || bSkipRes ){
#else
      if ( uiNoResidual == 0 ){
#endif
            m_pcPredSearch->motionCompensation ( rpcTempCU, m_ppcPredYuvTemp[uhDepth] );
            // save pred adress
            pcPredYuvTemp = m_ppcPredYuvTemp[uhDepth];

          }
          else
          {
#if FAST_DECISION_FOR_MRG_RD_COST
            if( bestIsSkip)
            {
              m_pcPredSearch->motionCompensation ( rpcTempCU, m_ppcPredYuvTemp[uhDepth] );
              // save pred adress
              pcPredYuvTemp = m_ppcPredYuvTemp[uhDepth];
            }
            else
            {
#endif
              if ( pcPredYuvTemp != m_ppcPredYuvTemp[uhDepth])
              {
                //adress changes take best (old temp)
                pcPredYuvTemp = m_ppcPredYuvBest[uhDepth];
              }
#if FAST_DECISION_FOR_MRG_RD_COST
            }
#endif
          }
#if HHI_VSO
          if( m_pcRdCost->getUseRenModel() )
          { //Reset
            UInt  uiWidth     = m_ppcOrigYuv[uhDepth]->getWidth    ();
            UInt  uiHeight    = m_ppcOrigYuv[uhDepth]->getHeight   ();
            Pel*  piSrc       = m_ppcOrigYuv[uhDepth]->getLumaAddr ();
            UInt  uiSrcStride = m_ppcOrigYuv[uhDepth]->getStride   ();
            m_pcRdCost->setRenModelData( rpcTempCU, 0, piSrc, uiSrcStride, uiWidth, uiHeight );
          }
#endif
          // estimate residual and encode everything
          m_pcPredSearch->encodeResAndCalcRdInterCU( rpcTempCU,
            m_ppcOrigYuv    [uhDepth],
            pcPredYuvTemp,
            m_ppcResiYuvTemp[uhDepth],
            m_ppcResiYuvBest[uhDepth],
            m_ppcRecoYuvTemp[uhDepth],
#if HHI_INTER_VIEW_RESIDUAL_PRED
                                                     m_ppcResPredTmp [uhDepth],
#endif
            (uiNoResidual? true:false) );     
          Bool bQtRootCbf = rpcTempCU->getQtRootCbf(0) == 1;

#if H0736_AVC_STYLE_QP_RANGE
          Int orgQP = rpcTempCU->getQP( 0 );
          xCheckDQP( rpcTempCU );
          xCheckBestMode(rpcBestCU, rpcTempCU, uhDepth);
          rpcTempCU->initEstData( uhDepth, orgQP );
#else
          UInt uiOrgQP = rpcTempCU->getQP( 0 );
          xCheckDQP( rpcTempCU );
          xCheckBestMode(rpcBestCU, rpcTempCU, uhDepth);
          rpcTempCU->initEstData( uhDepth, uiOrgQP );
#endif

#if FAST_DECISION_FOR_MRG_RD_COST
          if( m_pcEncCfg->getUseFastDecisionForMerge() && !bestIsSkip )
          {
            bestIsSkip = rpcBestCU->getQtRootCbf(0) == 0;
          }
#endif

          if (!bQtRootCbf)
            break;
#if FAST_DECISION_FOR_MRG_RD_COST
        }
#endif
      }
    }
  }
}

#if AMP_MRG
#if HHI_INTERVIEW_SKIP
Void TEncCu::xCheckRDCostInter( TComDataCU*& rpcBestCU, TComDataCU*& rpcTempCU, PartSize ePartSize, Bool bSkipRes, Bool bUseMRG)
#else
Void TEncCu::xCheckRDCostInter( TComDataCU*& rpcBestCU, TComDataCU*& rpcTempCU, PartSize ePartSize, Bool bUseMRG)
#endif
#else
#if HHI_INTERVIEW_SKIP
Void TEncCu::xCheckRDCostInter( TComDataCU*& rpcBestCU, TComDataCU*& rpcTempCU, PartSize ePartSize, Bool bSkipRes)
#else
Void TEncCu::xCheckRDCostInter( TComDataCU*& rpcBestCU, TComDataCU*& rpcTempCU, PartSize ePartSize )
#endif
#endif
{
  UChar uhDepth = rpcTempCU->getDepth( 0 );
  
#if HHI_VSO
  if( m_pcRdCost->getUseRenModel() )
  {
    UInt  uiWidth     = m_ppcOrigYuv[uhDepth]->getWidth ( );
    UInt  uiHeight    = m_ppcOrigYuv[uhDepth]->getHeight( );
    Pel*  piSrc       = m_ppcOrigYuv[uhDepth]->getLumaAddr( );
    UInt  uiSrcStride = m_ppcOrigYuv[uhDepth]->getStride();
    m_pcRdCost->setRenModelData( rpcTempCU, 0, piSrc, uiSrcStride, uiWidth, uiHeight );
  }
#endif  

  rpcTempCU->setDepthSubParts( uhDepth, 0 );
  
#if HHI_INTER_VIEW_RESIDUAL_PRED
  Bool  bResPrdAvail  = rpcTempCU->getResPredAvail( 0 );
  Bool  bResPrdFlag   = rpcTempCU->getResPredFlag ( 0 );
#endif
  
  rpcTempCU->setPartSizeSubParts  ( ePartSize,  0, uhDepth );
#if HHI_INTER_VIEW_RESIDUAL_PRED
  rpcTempCU->setResPredAvailSubParts( bResPrdAvail, 0, 0, uhDepth );
  rpcTempCU->setResPredFlagSubParts ( bResPrdFlag,  0, 0, uhDepth );
#endif
  rpcTempCU->setPredModeSubParts  ( MODE_INTER, 0, uhDepth );
  
#if HHI_INTER_VIEW_RESIDUAL_PRED
#if !LG_RESTRICTEDRESPRED_M24766
  if( rpcTempCU->getResPredFlag( 0 ) )
  { // subtract residual prediction from original in motion search
    m_ppcOrigYuv[uhDepth]->add( m_ppcResPredTmp [uhDepth], rpcTempCU->getWidth( 0 ), rpcTempCU->getHeight( 0 ), true );
  }
#endif
#endif

#if AMP_MRG
  rpcTempCU->setMergeAMP (true);
  #if HHI_INTERVIEW_SKIP
#if LG_RESTRICTEDRESPRED_M24766
  m_pcPredSearch->predInterSearch ( rpcTempCU, m_ppcOrigYuv[uhDepth], m_ppcResPredTmp[uhDepth], m_ppcPredYuvTemp[uhDepth], m_ppcResiYuvTemp[uhDepth], m_ppcRecoYuvTemp[uhDepth], bSkipRes, bUseMRG  );
#else
  m_pcPredSearch->predInterSearch ( rpcTempCU, m_ppcOrigYuv[uhDepth], m_ppcPredYuvTemp[uhDepth], m_ppcResiYuvTemp[uhDepth], m_ppcRecoYuvTemp[uhDepth], bSkipRes, bUseMRG  );
#endif
#else
  m_pcPredSearch->predInterSearch ( rpcTempCU, m_ppcOrigYuv[uhDepth], m_ppcPredYuvTemp[uhDepth], m_ppcResiYuvTemp[uhDepth], m_ppcRecoYuvTemp[uhDepth], false, bUseMRG );
#endif
#else
  #if HHI_INTERVIEW_SKIP
  m_pcPredSearch->predInterSearch ( rpcTempCU, m_ppcOrigYuv[uhDepth], m_ppcPredYuvTemp[uhDepth], m_ppcResiYuvTemp[uhDepth], m_ppcRecoYuvTemp[uhDepth], bSkipRes );
#else  
  m_pcPredSearch->predInterSearch ( rpcTempCU, m_ppcOrigYuv[uhDepth], m_ppcPredYuvTemp[uhDepth], m_ppcResiYuvTemp[uhDepth], m_ppcRecoYuvTemp[uhDepth] );
#endif
#endif

#if HHI_INTER_VIEW_RESIDUAL_PRED
#if !LG_RESTRICTEDRESPRED_M24766
  if( rpcTempCU->getResPredFlag( 0 ) )
  { // add residual prediction to original again
    m_ppcOrigYuv[uhDepth]->add( m_ppcResPredTmp [uhDepth], rpcTempCU->getWidth( 0 ), rpcTempCU->getHeight( 0 ) );
  }
#endif
#endif

#if AMP_MRG
  if ( !rpcTempCU->getMergeAMP() )
  {
    return;
  }
#endif

#if HHI_INTERVIEW_SKIP
  m_pcPredSearch->encodeResAndCalcRdInterCU( rpcTempCU,
                                             m_ppcOrigYuv[uhDepth],
                                             m_ppcPredYuvTemp[uhDepth],
                                             m_ppcResiYuvTemp[uhDepth],
                                             m_ppcResiYuvBest[uhDepth],
                                             m_ppcRecoYuvTemp[uhDepth],
#if HHI_INTER_VIEW_RESIDUAL_PRED
                                             m_ppcResPredTmp [uhDepth],
#endif
                                             bSkipRes );
#else
  m_pcPredSearch->encodeResAndCalcRdInterCU( rpcTempCU,
                                             m_ppcOrigYuv[uhDepth],
                                             m_ppcPredYuvTemp[uhDepth],
                                             m_ppcResiYuvTemp[uhDepth],
                                             m_ppcResiYuvBest[uhDepth],
                                             m_ppcRecoYuvTemp[uhDepth],
#if HHI_INTER_VIEW_RESIDUAL_PRED
                                             m_ppcResPredTmp [uhDepth],
#endif
                                             false );
#endif
#if HHI_VSO
  if( m_pcRdCost->getUseLambdaScaleVSO() )
  {
    rpcTempCU->getTotalCost()  = m_pcRdCost->calcRdCostVSO( rpcTempCU->getTotalBits(), rpcTempCU->getTotalDistortion() );
  }
  else
#endif
  {
  rpcTempCU->getTotalCost()  = m_pcRdCost->calcRdCost( rpcTempCU->getTotalBits(), rpcTempCU->getTotalDistortion() );
  }

  xCheckDQP( rpcTempCU );
  xCheckBestMode(rpcBestCU, rpcTempCU, uhDepth);
}

Void TEncCu::xCheckRDCostIntra( TComDataCU*& rpcBestCU, TComDataCU*& rpcTempCU, PartSize eSize )
{
  UInt uiDepth = rpcTempCU->getDepth( 0 );
  
#if HHI_VSO
  if( m_pcRdCost->getUseRenModel() )
  {
    UInt  uiWidth     = m_ppcOrigYuv[uiDepth]->getWidth   ();
    UInt  uiHeight    = m_ppcOrigYuv[uiDepth]->getHeight  ();
    Pel*  piSrc       = m_ppcOrigYuv[uiDepth]->getLumaAddr();
    UInt  uiSrcStride = m_ppcOrigYuv[uiDepth]->getStride  ();
    m_pcRdCost->setRenModelData( rpcTempCU, 0, piSrc, uiSrcStride, uiWidth, uiHeight );
  }
#endif

  rpcTempCU->setPartSizeSubParts( eSize, 0, uiDepth );
  rpcTempCU->setPredModeSubParts( MODE_INTRA, 0, uiDepth );
  
  Bool bSeparateLumaChroma = true; // choose estimation mode
  Dist uiPreCalcDistC      = 0;
  if( !bSeparateLumaChroma )
  {
    m_pcPredSearch->preestChromaPredMode( rpcTempCU, m_ppcOrigYuv[uiDepth], m_ppcPredYuvTemp[uiDepth] );
  }
  m_pcPredSearch  ->estIntraPredQT      ( rpcTempCU, m_ppcOrigYuv[uiDepth], m_ppcPredYuvTemp[uiDepth], m_ppcResiYuvTemp[uiDepth], m_ppcRecoYuvTemp[uiDepth], uiPreCalcDistC, bSeparateLumaChroma );

  m_ppcRecoYuvTemp[uiDepth]->copyToPicLuma(rpcTempCU->getPic()->getPicYuvRec(), rpcTempCU->getAddr(), rpcTempCU->getZorderIdxInCU() );
  
  m_pcPredSearch  ->estIntraPredChromaQT( rpcTempCU, m_ppcOrigYuv[uiDepth], m_ppcPredYuvTemp[uiDepth], m_ppcResiYuvTemp[uiDepth], m_ppcRecoYuvTemp[uiDepth], uiPreCalcDistC );
  
  m_pcEntropyCoder->resetBits();
  m_pcEntropyCoder->encodeSkipFlag ( rpcTempCU, 0,          true );
  m_pcEntropyCoder->encodePredMode( rpcTempCU, 0,          true );
  m_pcEntropyCoder->encodePartSize( rpcTempCU, 0, uiDepth, true );
  m_pcEntropyCoder->encodePredInfo( rpcTempCU, 0,          true );
  m_pcEntropyCoder->encodeIPCMInfo(rpcTempCU, 0, true );

  // Encode Coefficients
  Bool bCodeDQP = getdQPFlag();
  m_pcEntropyCoder->encodeCoeff( rpcTempCU, 0, uiDepth, rpcTempCU->getWidth (0), rpcTempCU->getHeight(0), bCodeDQP );
  setdQPFlag( bCodeDQP );
  
  if( m_bUseSBACRD ) m_pcRDGoOnSbacCoder->store(m_pppcRDSbacCoder[uiDepth][CI_TEMP_BEST]);
  
  rpcTempCU->getTotalBits() = m_pcEntropyCoder->getNumberOfWrittenBits();
  if(m_pcEncCfg->getUseSBACRD())
  {
    rpcTempCU->getTotalBins() = ((TEncBinCABAC *)((TEncSbac*)m_pcEntropyCoder->m_pcEntropyCoderIf)->getEncBinIf())->getBinsCoded();
  }
#if HHI_VSO
  if( m_pcRdCost->getUseLambdaScaleVSO())
  {
    rpcTempCU->getTotalCost() = m_pcRdCost->calcRdCostVSO( rpcTempCU->getTotalBits(), rpcTempCU->getTotalDistortion() );
  }
  else
#endif
  {
  rpcTempCU->getTotalCost() = m_pcRdCost->calcRdCost( rpcTempCU->getTotalBits(), rpcTempCU->getTotalDistortion() );
  }
  
  xCheckDQP( rpcTempCU );
  xCheckBestMode(rpcBestCU, rpcTempCU, uiDepth);
}

/** Check R-D costs for a CU with PCM mode. 
 * \param rpcBestCU pointer to best mode CU data structure
 * \param rpcTempCU pointer to testing mode CU data structure
 * \returns Void
 * 
 * \note Current PCM implementation encodes sample values in a lossless way. The distortion of PCM mode CUs are zero. PCM mode is selected if the best mode yields bits greater than that of PCM mode.
 */
Void TEncCu::xCheckIntraPCM( TComDataCU*& rpcBestCU, TComDataCU*& rpcTempCU )
{
  UInt uiDepth = rpcTempCU->getDepth( 0 );

  rpcTempCU->setIPCMFlag(0, true);
  rpcTempCU->setIPCMFlagSubParts (true, 0, rpcTempCU->getDepth(0));
  rpcTempCU->setPartSizeSubParts( SIZE_2Nx2N, 0, uiDepth );
  rpcTempCU->setPredModeSubParts( MODE_INTRA, 0, uiDepth );

  m_pcPredSearch->IPCMSearch( rpcTempCU, m_ppcOrigYuv[uiDepth], m_ppcPredYuvTemp[uiDepth], m_ppcResiYuvTemp[uiDepth], m_ppcRecoYuvTemp[uiDepth]);

  if( m_bUseSBACRD ) m_pcRDGoOnSbacCoder->load(m_pppcRDSbacCoder[uiDepth][CI_CURR_BEST]);

  m_pcEntropyCoder->resetBits();
  m_pcEntropyCoder->encodeSkipFlag ( rpcTempCU, 0,          true );
  m_pcEntropyCoder->encodePredMode ( rpcTempCU, 0,          true );
  m_pcEntropyCoder->encodePartSize ( rpcTempCU, 0, uiDepth, true );
  m_pcEntropyCoder->encodeIPCMInfo ( rpcTempCU, 0, true );

  if( m_bUseSBACRD ) m_pcRDGoOnSbacCoder->store(m_pppcRDSbacCoder[uiDepth][CI_TEMP_BEST]);

  rpcTempCU->getTotalBits() = m_pcEntropyCoder->getNumberOfWrittenBits();
  if(m_pcEncCfg->getUseSBACRD())
  {
    rpcTempCU->getTotalBins() = ((TEncBinCABAC *)((TEncSbac*)m_pcEntropyCoder->m_pcEntropyCoderIf)->getEncBinIf())->getBinsCoded();
  }
#if HHI_VSO
  if ( m_pcRdCost->getUseVSO() )
  {
    rpcTempCU->getTotalCost() = m_pcRdCost->calcRdCostVSO( rpcTempCU->getTotalBits(), rpcTempCU->getTotalDistortion() );
  }
  else
#endif
  {  
  rpcTempCU->getTotalCost() = m_pcRdCost->calcRdCost( rpcTempCU->getTotalBits(), rpcTempCU->getTotalDistortion() );
  }

  xCheckDQP( rpcTempCU );
  xCheckBestMode( rpcBestCU, rpcTempCU, uiDepth );
}

// check whether current try is the best
Void TEncCu::xCheckBestMode( TComDataCU*& rpcBestCU, TComDataCU*& rpcTempCU )
{
  if( rpcTempCU->getTotalCost() < rpcBestCU->getTotalCost() )
  {
    TComYuv* pcYuv;
    UChar uhDepth = rpcBestCU->getDepth(0);

    // Change Information data
    TComDataCU* pcCU = rpcBestCU;
    rpcBestCU = rpcTempCU;
    rpcTempCU = pcCU;
    
    // Change Prediction data
    pcYuv = m_ppcPredYuvBest[uhDepth];
    m_ppcPredYuvBest[uhDepth] = m_ppcPredYuvTemp[uhDepth];
    m_ppcPredYuvTemp[uhDepth] = pcYuv;
    
    // Change Reconstruction data
    pcYuv = m_ppcRecoYuvBest[uhDepth];
    m_ppcRecoYuvBest[uhDepth] = m_ppcRecoYuvTemp[uhDepth];
    m_ppcRecoYuvTemp[uhDepth] = pcYuv;
    
    pcYuv = NULL;
    pcCU  = NULL;
    
    if( m_bUseSBACRD )  // store temp best CI for next CU coding
      m_pppcRDSbacCoder[uhDepth][CI_TEMP_BEST]->store(m_pppcRDSbacCoder[uhDepth][CI_NEXT_BEST]);
  }
}

/** check whether current try is the best with identifying the depth of current try
 * \param rpcBestCU
 * \param rpcTempCU
 * \returns Void
 */
Void TEncCu::xCheckBestMode( TComDataCU*& rpcBestCU, TComDataCU*& rpcTempCU, UInt uiDepth )
{
  if( rpcTempCU->getTotalCost() < rpcBestCU->getTotalCost() )
  {
    TComYuv* pcYuv;
    // Change Information data
    TComDataCU* pcCU = rpcBestCU;
    rpcBestCU = rpcTempCU;
    rpcTempCU = pcCU;

    // Change Prediction data
    pcYuv = m_ppcPredYuvBest[uiDepth];
    m_ppcPredYuvBest[uiDepth] = m_ppcPredYuvTemp[uiDepth];
    m_ppcPredYuvTemp[uiDepth] = pcYuv;

    // Change Reconstruction data
    pcYuv = m_ppcRecoYuvBest[uiDepth];
    m_ppcRecoYuvBest[uiDepth] = m_ppcRecoYuvTemp[uiDepth];
    m_ppcRecoYuvTemp[uiDepth] = pcYuv;

    pcYuv = NULL;
    pcCU  = NULL;

    if( m_bUseSBACRD )  // store temp best CI for next CU coding
      m_pppcRDSbacCoder[uiDepth][CI_TEMP_BEST]->store(m_pppcRDSbacCoder[uiDepth][CI_NEXT_BEST]);
  }
}

Void TEncCu::xCheckDQP( TComDataCU* pcCU )
{
  UInt uiDepth = pcCU->getDepth( 0 );

  if( pcCU->getSlice()->getPPS()->getUseDQP() && (g_uiMaxCUWidth>>uiDepth) >= pcCU->getSlice()->getPPS()->getMinCuDQPSize() )
  {
    if ( pcCU->getCbf( 0, TEXT_LUMA, 0 ) || pcCU->getCbf( 0, TEXT_CHROMA_U, 0 ) || pcCU->getCbf( 0, TEXT_CHROMA_V, 0 ) )
    {
#if !RDO_WITHOUT_DQP_BITS
      m_pcEntropyCoder->resetBits();
      m_pcEntropyCoder->encodeQP( pcCU, 0, false );
      pcCU->getTotalBits() += m_pcEntropyCoder->getNumberOfWrittenBits(); // dQP bits
      if(m_pcEncCfg->getUseSBACRD())
      {
        pcCU->getTotalBins() += ((TEncBinCABAC *)((TEncSbac*)m_pcEntropyCoder->m_pcEntropyCoderIf)->getEncBinIf())->getBinsCoded();
      }

      // GT: Change here??
#if HHI_VSO
      if ( m_pcRdCost->getUseVSO() )
      {
        pcCU->getTotalCost() = m_pcRdCost->calcRdCostVSO( pcCU->getTotalBits(), pcCU->getTotalDistortion() );
      }
      else
#endif
      {
      pcCU->getTotalCost() = m_pcRdCost->calcRdCost( pcCU->getTotalBits(), pcCU->getTotalDistortion() );
      }   
#endif
    }
    else
    {
#if LOSSLESS_CODING
      if ((  ( pcCU->getRefQP( 0 ) != pcCU->getQP( 0 )) ) && (pcCU->getSlice()->getSPS()->getUseLossless()))
      {
        pcCU->getTotalCost() = MAX_DOUBLE;
      }
#endif
      pcCU->setQPSubParts( pcCU->getRefQP( 0 ), 0, uiDepth ); // set QP to default QP
    }
  }
}

#if BURST_IPCM
/** Check whether the last CU shares the same root as the current CU and is IPCM or not.  
 * \param pcCU
 * \param uiCurAbsPartIdx
 * \returns Bool
 */
Bool TEncCu::checkLastCUSucIPCM( TComDataCU* pcCU, UInt uiCurAbsPartIdx )
{
  Bool lastCUSucIPCMFlag = false;

  UInt curDepth = pcCU->getDepth(uiCurAbsPartIdx);
  UInt shift = ((pcCU->getPic()->getSlice(pcCU->getPic()->getCurrSliceIdx())->getSPS()->getMaxCUDepth() - curDepth)<<1);
  UInt startPartUnitIdx = ((uiCurAbsPartIdx&(0x03<<shift))>>shift);

  TComSlice * pcSlice = pcCU->getPic()->getSlice(pcCU->getPic()->getCurrSliceIdx());
  if( pcSlice->getEntropySliceCurStartCUAddr() == ( pcCU->getSCUAddr() + uiCurAbsPartIdx ) )
  {
    return false;
  }

  if(curDepth > 0 && startPartUnitIdx > 0)
  {
    Int lastValidPartIdx = pcCU->getLastValidPartIdx((Int) uiCurAbsPartIdx );

    if( lastValidPartIdx >= 0 )
    {
      if(( pcCU->getSliceStartCU( uiCurAbsPartIdx ) == pcCU->getSliceStartCU( (UInt) lastValidPartIdx ))
        && 
        ( pcCU->getDepth( uiCurAbsPartIdx ) == pcCU->getDepth( (UInt) lastValidPartIdx )) 
        && 
        pcCU->getIPCMFlag( (UInt) lastValidPartIdx ) )
      {
        lastCUSucIPCMFlag = true;
      }
    }
  }

  return  lastCUSucIPCMFlag;
}

/** Count the number of successive IPCM CUs sharing the same root.
 * \param pcCU
 * \param uiCurAbsPartIdx
 * \returns Int
 */
Int TEncCu::countNumSucIPCM ( TComDataCU* pcCU, UInt uiCurAbsPartIdx )
{
  Int numSucIPCM = 0;
  UInt CurDepth = pcCU->getDepth(uiCurAbsPartIdx);

  if( pcCU->getIPCMFlag(uiCurAbsPartIdx) )
  {
    if(CurDepth == 0)
    {
       numSucIPCM = 1;
    }
    else 
    {
      TComPic* pcPic = pcCU->getPic();
      TComSlice * pcSlice = pcCU->getPic()->getSlice(pcCU->getPic()->getCurrSliceIdx());
      UInt qNumParts = ( pcPic->getNumPartInCU() >> ((CurDepth-1)<<1) )>>2;

      Bool continueFlag = true;
      UInt absPartIdx = uiCurAbsPartIdx;
      UInt shift = ((pcSlice->getSPS()->getMaxCUDepth() - CurDepth)<<1);
      UInt startPartUnitIdx = ((uiCurAbsPartIdx&(0x03<<shift))>>shift);

      for ( UInt partUnitIdx = startPartUnitIdx; partUnitIdx < 4 && continueFlag; partUnitIdx++, absPartIdx+=qNumParts )
      {
        UInt lPelX = pcCU->getCUPelX() + g_auiRasterToPelX[ g_auiZscanToRaster[absPartIdx] ];
        UInt tPelY = pcCU->getCUPelY() + g_auiRasterToPelY[ g_auiZscanToRaster[absPartIdx] ];
        Bool inSliceFlag = ( pcCU->getSCUAddr()+absPartIdx+qNumParts>pcSlice->getEntropySliceCurStartCUAddr() ) && ( pcCU->getSCUAddr()+absPartIdx < pcSlice->getEntropySliceCurEndCUAddr());

        if( inSliceFlag && ( lPelX < pcSlice->getSPS()->getPicWidthInLumaSamples() ) && ( tPelY < pcSlice->getSPS()->getPicHeightInLumaSamples() ) )
        {
          UInt uiDepth = pcCU->getDepth(absPartIdx);

          if( ( CurDepth == uiDepth) && pcCU->getIPCMFlag( absPartIdx ) )
          {
            numSucIPCM++;
          }
          else
          {
            continueFlag = false;
          }
        }
      }
    }
  }

  return numSucIPCM;
}
#endif

Void TEncCu::xCopyAMVPInfo (AMVPInfo* pSrc, AMVPInfo* pDst)
{
  pDst->iN = pSrc->iN;
  for (Int i = 0; i < pSrc->iN; i++)
  {
    pDst->m_acMvCand[i] = pSrc->m_acMvCand[i];
  }
}
Void TEncCu::xCopyYuv2Pic(TComPic* rpcPic, UInt uiCUAddr, UInt uiAbsPartIdx, UInt uiDepth, UInt uiSrcDepth, TComDataCU* pcCU, UInt uiLPelX, UInt uiTPelY )
{
  UInt uiRPelX   = uiLPelX + (g_uiMaxCUWidth>>uiDepth)  - 1;
  UInt uiBPelY   = uiTPelY + (g_uiMaxCUHeight>>uiDepth) - 1;
  TComSlice * pcSlice = pcCU->getPic()->getSlice(pcCU->getPic()->getCurrSliceIdx());
  Bool bSliceStart = pcSlice->getEntropySliceCurStartCUAddr() > rpcPic->getPicSym()->getInverseCUOrderMap(pcCU->getAddr())*pcCU->getPic()->getNumPartInCU()+uiAbsPartIdx && 
    pcSlice->getEntropySliceCurStartCUAddr() < rpcPic->getPicSym()->getInverseCUOrderMap(pcCU->getAddr())*pcCU->getPic()->getNumPartInCU()+uiAbsPartIdx+( pcCU->getPic()->getNumPartInCU() >> (uiDepth<<1) );
  Bool bSliceEnd   = pcSlice->getEntropySliceCurEndCUAddr() > rpcPic->getPicSym()->getInverseCUOrderMap(pcCU->getAddr())*pcCU->getPic()->getNumPartInCU()+uiAbsPartIdx && 
    pcSlice->getEntropySliceCurEndCUAddr() < rpcPic->getPicSym()->getInverseCUOrderMap(pcCU->getAddr())*pcCU->getPic()->getNumPartInCU()+uiAbsPartIdx+( pcCU->getPic()->getNumPartInCU() >> (uiDepth<<1) );
  if(!bSliceEnd && !bSliceStart && ( uiRPelX < pcSlice->getSPS()->getPicWidthInLumaSamples() ) && ( uiBPelY < pcSlice->getSPS()->getPicHeightInLumaSamples() ) )
  {
    UInt uiAbsPartIdxInRaster = g_auiZscanToRaster[uiAbsPartIdx];
    UInt uiSrcBlkWidth = rpcPic->getNumPartInWidth() >> (uiSrcDepth);
    UInt uiBlkWidth    = rpcPic->getNumPartInWidth() >> (uiDepth);
    UInt uiPartIdxX = ( ( uiAbsPartIdxInRaster % rpcPic->getNumPartInWidth() ) % uiSrcBlkWidth) / uiBlkWidth;
    UInt uiPartIdxY = ( ( uiAbsPartIdxInRaster / rpcPic->getNumPartInWidth() ) % uiSrcBlkWidth) / uiBlkWidth;
    UInt uiPartIdx = uiPartIdxY * ( uiSrcBlkWidth / uiBlkWidth ) + uiPartIdxX;
    m_ppcRecoYuvBest[uiSrcDepth]->copyToPicYuv( rpcPic->getPicYuvRec (), uiCUAddr, uiAbsPartIdx, uiDepth - uiSrcDepth, uiPartIdx);
  }
  else
  {
    UInt uiQNumParts = ( pcCU->getPic()->getNumPartInCU() >> (uiDepth<<1) )>>2;

    for ( UInt uiPartUnitIdx = 0; uiPartUnitIdx < 4; uiPartUnitIdx++, uiAbsPartIdx+=uiQNumParts )
    {
      UInt uiSubCULPelX   = uiLPelX + ( g_uiMaxCUWidth >>(uiDepth+1) )*( uiPartUnitIdx &  1 );
      UInt uiSubCUTPelY   = uiTPelY + ( g_uiMaxCUHeight>>(uiDepth+1) )*( uiPartUnitIdx >> 1 );

      Bool bInSlice = rpcPic->getPicSym()->getInverseCUOrderMap(pcCU->getAddr())*pcCU->getPic()->getNumPartInCU()+uiAbsPartIdx+uiQNumParts > pcSlice->getEntropySliceCurStartCUAddr() && 
        rpcPic->getPicSym()->getInverseCUOrderMap(pcCU->getAddr())*pcCU->getPic()->getNumPartInCU()+uiAbsPartIdx < pcSlice->getEntropySliceCurEndCUAddr();
      if(bInSlice&&( uiSubCULPelX < pcSlice->getSPS()->getPicWidthInLumaSamples() ) && ( uiSubCUTPelY < pcSlice->getSPS()->getPicHeightInLumaSamples() ) )
      {
        xCopyYuv2Pic( rpcPic, uiCUAddr, uiAbsPartIdx, uiDepth+1, uiSrcDepth, pcCU, uiSubCULPelX, uiSubCUTPelY );   // Copy Yuv data to picture Yuv
      }
    }
  }
}

Void TEncCu::xCopyYuv2Tmp( UInt uiPartUnitIdx, UInt uiNextDepth )
{
  UInt uiCurrDepth = uiNextDepth - 1;
  m_ppcRecoYuvBest[uiNextDepth]->copyToPartYuv( m_ppcRecoYuvTemp[uiCurrDepth], uiPartUnitIdx );
}

#if LOSSLESS_CODING 
/** Function for filling the PCM buffer of a CU using its original sample array 
 * \param pcCU pointer to current CU
 * \param pcOrgYuv pointer to original sample array
 * \returns Void
 */
Void TEncCu::xFillPCMBuffer     ( TComDataCU*& pCU, TComYuv* pOrgYuv )
{

  UInt   width        = pCU->getWidth(0);
  UInt   height       = pCU->getHeight(0);

  Pel*   pSrcY = pOrgYuv->getLumaAddr(0, width); 
  Pel*   pDstY = pCU->getPCMSampleY();
  UInt   srcStride = pOrgYuv->getStride();

  for(Int y = 0; y < height; y++ )
  {
    for(Int x = 0; x < width; x++ )
    {
      pDstY[x] = pSrcY[x];
    }
    pDstY += width;
    pSrcY += srcStride;
  }

  Pel* pSrcCb       = pOrgYuv->getCbAddr();
  Pel* pSrcCr       = pOrgYuv->getCrAddr();;

  Pel* pDstCb       = pCU->getPCMSampleCb();
  Pel* pDstCr       = pCU->getPCMSampleCr();;

  UInt srcStrideC = pOrgYuv->getCStride();
  UInt heightC   = height >> 1;
  UInt widthC    = width  >> 1;

  for(Int y = 0; y < heightC; y++ )
  {
    for(Int x = 0; x < widthC; x++ )
    {
      pDstCb[x] = pSrcCb[x];
      pDstCr[x] = pSrcCr[x];
    }
    pDstCb += widthC;
    pDstCr += widthC;
    pSrcCb += srcStrideC;
    pSrcCr += srcStrideC;
  }
}
#endif

#if ADAPTIVE_QP_SELECTION
/** Collect ARL statistics from one block
  */
Int TEncCu::xTuCollectARLStats(TCoeff* rpcCoeff, Int* rpcArlCoeff, Int NumCoeffInCU, Double* cSum, UInt* numSamples )
{
  for( Int n = 0; n < NumCoeffInCU; n++ )
  {
    Int u = abs( rpcCoeff[ n ] );
    Int absc = rpcArlCoeff[ n ];

    if( u != 0 )
    {
      if( u < LEVEL_RANGE )
      {
        cSum[ u ] += ( Double )absc;
        numSamples[ u ]++;
      }
      else 
      {
        cSum[ LEVEL_RANGE ] += ( Double )absc - ( Double )( u << ARL_C_PRECISION );
        numSamples[ LEVEL_RANGE ]++;
      }
    }
  }

  return 0;
}

/** Collect ARL statistics from one LCU
 * \param pcCU
 */
Void TEncCu::xLcuCollectARLStats(TComDataCU* rpcCU )
{
  Double cSum[ LEVEL_RANGE + 1 ];     //: the sum of DCT coefficients corresponding to datatype and quantization output
  UInt numSamples[ LEVEL_RANGE + 1 ]; //: the number of coefficients corresponding to datatype and quantization output

  TCoeff* pCoeffY = rpcCU->getCoeffY();
  Int* pArlCoeffY = rpcCU->getArlCoeffY();

  UInt uiMinCUWidth = g_uiMaxCUWidth >> g_uiMaxCUDepth;
  UInt uiMinNumCoeffInCU = 1 << uiMinCUWidth;

  memset( cSum, 0, sizeof( Double )*(LEVEL_RANGE+1) );
  memset( numSamples, 0, sizeof( UInt )*(LEVEL_RANGE+1) );

  // Collect stats to cSum[][] and numSamples[][]
  for(Int i = 0; i < rpcCU->getTotalNumPart(); i ++ )
  {
    UInt uiTrIdx = rpcCU->getTransformIdx(i);

    if(rpcCU->getPredictionMode(i) == MODE_INTER)
    if( rpcCU->getCbf( i, TEXT_LUMA, uiTrIdx ) )
    {
      xTuCollectARLStats(pCoeffY, pArlCoeffY, uiMinNumCoeffInCU, cSum, numSamples);
    }//Note that only InterY is processed. QP rounding is based on InterY data only.
   
    pCoeffY  += uiMinNumCoeffInCU;
    pArlCoeffY  += uiMinNumCoeffInCU;
  }

  for(Int u=1; u<LEVEL_RANGE;u++)
  {
    m_pcTrQuant->getSliceSumC()[u] += cSum[ u ] ;
    m_pcTrQuant->getSliceNSamples()[u] += numSamples[ u ] ;
  }
  m_pcTrQuant->getSliceSumC()[LEVEL_RANGE] += cSum[ LEVEL_RANGE ] ;
  m_pcTrQuant->getSliceNSamples()[LEVEL_RANGE] += numSamples[ LEVEL_RANGE ] ;
}
#endif

#if HHI_MPI
Void TEncCu::xCheckRDCostMvInheritance( TComDataCU*& rpcBestCU, TComDataCU*& rpcTempCU, UChar uhTextureModeDepth, Bool bSkipResidual, Bool bRecursiveCall )
{
  assert( rpcTempCU->getSlice()->getIsDepth() );
  TComDataCU *pcTextureCU = rpcTempCU->getSlice()->getTexturePic()->getCU( rpcTempCU->getAddr() );

  const UChar uhDepth  = rpcTempCU->getDepth( 0 );
  const Int   iQP      = rpcTempCU->getQP( 0 );
  assert( bRecursiveCall == ( uhDepth != uhTextureModeDepth ) );

  if( uhDepth == uhTextureModeDepth )
  {
    for( UInt ui = 0; ui < rpcTempCU->getTotalNumPart(); ui++ )
    {
      if( pcTextureCU->isIntra( rpcTempCU->getZorderIdxInCU() + ui ) )
      {
        return;
      }
    }
  }

#if HHI_VSO
  if( m_pcRdCost->getUseRenModel() && !bRecursiveCall)
  {
    UInt  uiWidth     = m_ppcOrigYuv[uhDepth]->getWidth   ();
    UInt  uiHeight    = m_ppcOrigYuv[uhDepth]->getHeight  ();
    Pel*  piSrc       = m_ppcOrigYuv[uhDepth]->getLumaAddr();
    UInt  uiSrcStride = m_ppcOrigYuv[uhDepth]->getStride  ();
    m_pcRdCost->setRenModelData( rpcTempCU, 0, piSrc, uiSrcStride, uiWidth, uiHeight );
  }
#endif

  Bool bSplit = uhDepth < pcTextureCU->getDepth( rpcTempCU->getZorderIdxInCU() );
  if( bSplit )
  {
    const UChar       uhNextDepth   = uhDepth+1;
    TComDataCU* pcSubBestPartCU     = m_ppcBestCU[uhNextDepth];
    TComDataCU* pcSubTempPartCU     = m_ppcTempCU[uhNextDepth];

    for ( UInt uiPartUnitIdx = 0; uiPartUnitIdx < 4; uiPartUnitIdx++ )
    {
      pcSubBestPartCU->initSubCU( rpcTempCU, uiPartUnitIdx, uhNextDepth, iQP );           // clear sub partition datas or init.
      pcSubTempPartCU->initSubCU( rpcTempCU, uiPartUnitIdx, uhNextDepth, iQP );           // clear sub partition datas or init.

      TComSlice * pcSlice = rpcTempCU->getPic()->getSlice(rpcTempCU->getPic()->getCurrSliceIdx());
      Bool bInSlice = pcSubBestPartCU->getSCUAddr()+pcSubBestPartCU->getTotalNumPart()>pcSlice->getEntropySliceCurStartCUAddr()&&pcSubBestPartCU->getSCUAddr()<pcSlice->getEntropySliceCurEndCUAddr();
      if(bInSlice && ( pcSubBestPartCU->getCUPelX() < pcSlice->getSPS()->getPicWidthInLumaSamples() ) && ( pcSubBestPartCU->getCUPelY() < pcSlice->getSPS()->getPicHeightInLumaSamples() ) )
      {
        if( m_bUseSBACRD )
        {
          if ( 0 == uiPartUnitIdx) //initialize RD with previous depth buffer
          {
            m_pppcRDSbacCoder[uhNextDepth][CI_CURR_BEST]->load(m_pppcRDSbacCoder[uhDepth][CI_CURR_BEST]);
          }
          else
          {
            m_pppcRDSbacCoder[uhNextDepth][CI_CURR_BEST]->load(m_pppcRDSbacCoder[uhNextDepth][CI_NEXT_BEST]);
          }
        }

        xCheckRDCostMvInheritance( pcSubBestPartCU, pcSubTempPartCU, uhTextureModeDepth, bSkipResidual, true );

        rpcTempCU->copyPartFrom( pcSubBestPartCU, uiPartUnitIdx, uhNextDepth );         // Keep best part data to current temporary data.
        xCopyYuv2Tmp( pcSubBestPartCU->getTotalNumPart()*uiPartUnitIdx, uhNextDepth );
      }
      else if (bInSlice)
      {
        pcSubBestPartCU->copyToPic( uhNextDepth );
        rpcTempCU->copyPartFrom( pcSubBestPartCU, uiPartUnitIdx, uhNextDepth );
      }
    }

    if( uhDepth == uhTextureModeDepth )
    {
      xAddMVISignallingBits( rpcTempCU );
    }

    // DQP stuff
    {
      if( (g_uiMaxCUWidth>>uhDepth) == rpcTempCU->getSlice()->getPPS()->getMinCuDQPSize() && rpcTempCU->getSlice()->getPPS()->getUseDQP())
      {
        TComPic *pcPic = rpcTempCU->getPic();
        TComSlice *pcSlice = rpcTempCU->getPic()->getSlice(rpcTempCU->getPic()->getCurrSliceIdx());
        Bool bHasRedisual = false;
        for( UInt uiBlkIdx = 0; uiBlkIdx < rpcTempCU->getTotalNumPart(); uiBlkIdx ++)
        {
          if( ( pcPic->getCU( rpcTempCU->getAddr() )->getEntropySliceStartCU(uiBlkIdx+rpcTempCU->getZorderIdxInCU()) == rpcTempCU->getSlice()->getEntropySliceCurStartCUAddr() ) &&
              ( rpcTempCU->getCbf( uiBlkIdx, TEXT_LUMA ) || rpcTempCU->getCbf( uiBlkIdx, TEXT_CHROMA_U ) || rpcTempCU->getCbf( uiBlkIdx, TEXT_CHROMA_V ) ) )
          {
            bHasRedisual = true;
            break;
          }
        }

        UInt uiTargetPartIdx;
        if ( pcPic->getCU( rpcTempCU->getAddr() )->getEntropySliceStartCU(rpcTempCU->getZorderIdxInCU()) != pcSlice->getEntropySliceCurStartCUAddr() )
        {
          uiTargetPartIdx = pcSlice->getEntropySliceCurStartCUAddr() % pcPic->getNumPartInCU() - rpcTempCU->getZorderIdxInCU();
        }
        else
        {
          uiTargetPartIdx = 0;
        }
        if ( ! bHasRedisual )
        {
  #if LOSSLESS_CODING
          if (((rpcTempCU->getQP(uiTargetPartIdx) != rpcTempCU->getRefQP(uiTargetPartIdx)) ) && (rpcTempCU->getSlice()->getSPS()->getUseLossless()))
          {
            rpcTempCU->getTotalCost() = MAX_DOUBLE;
          }
  #endif
          rpcTempCU->setQPSubParts( rpcTempCU->getRefQP( uiTargetPartIdx ), 0, uhDepth ); // set QP to default QP
        }
      }
    }

    if( m_bUseSBACRD )
    {
      m_pppcRDSbacCoder[uhNextDepth][CI_NEXT_BEST]->store(m_pppcRDSbacCoder[uhDepth][CI_TEMP_BEST]);
    }
  }
  else
  {
    rpcTempCU->setTextureModeDepthSubParts( uhTextureModeDepth, 0, uhDepth );
    rpcTempCU->copyTextureMotionDataFrom( pcTextureCU, uhDepth, rpcTempCU->getZorderIdxInCU() );
    rpcTempCU->setPartSizeSubParts( SIZE_NxN, 0, uhDepth );
    for( UInt ui = 0; ui < rpcTempCU->getTotalNumPart(); ui++ )
    {
      assert( rpcTempCU->getInterDir( ui ) != 0 );
      assert( rpcTempCU->getPredictionMode( ui ) != MODE_NONE );
    }
    rpcTempCU->setPredModeSubParts( bSkipResidual ? MODE_SKIP : MODE_INTER, 0, uhDepth );
    m_pcPredSearch->motionCompensation( rpcTempCU, m_ppcPredYuvTemp[uhDepth] );

    // get Original YUV data from picture
    m_ppcOrigYuv[uhDepth]->copyFromPicYuv( rpcBestCU->getPic()->getPicYuvOrg(), rpcBestCU->getAddr(), rpcBestCU->getZorderIdxInCU() );
    m_pcPredSearch->encodeResAndCalcRdInterCU( rpcTempCU,
                                               m_ppcOrigYuv[uhDepth],
                                               m_ppcPredYuvTemp[uhDepth],
                                               m_ppcResiYuvTemp[uhDepth],
                                               m_ppcResiYuvBest[uhDepth],
                                               m_ppcRecoYuvTemp[uhDepth],
#if HHI_INTER_VIEW_RESIDUAL_PRED
                                               m_ppcResPredTmp [uhDepth],
#endif
                                               bSkipResidual );

    if( uhDepth == uhTextureModeDepth )
    {
      xAddMVISignallingBits( rpcTempCU );
    }
    xCheckDQP( rpcTempCU );
  }

#if HHI_VSO
  if( m_pcRdCost->getUseLambdaScaleVSO() )
  {
    rpcTempCU->getTotalCost()  = m_pcRdCost->calcRdCostVSO( rpcTempCU->getTotalBits(), rpcTempCU->getTotalDistortion() );
  }
  else
#endif
  {
    rpcTempCU->getTotalCost()  = m_pcRdCost->calcRdCost( rpcTempCU->getTotalBits(), rpcTempCU->getTotalDistortion() );
  }

  if( rpcTempCU->getPredictionMode( 0 ) == MODE_SKIP && uhDepth == uhTextureModeDepth )
  {
    if( rpcTempCU->getSlice()->getPPS()->getUseDQP() && (g_uiMaxCUWidth>>uhDepth) >= rpcTempCU->getSlice()->getPPS()->getMinCuDQPSize() )
      rpcTempCU->setQPSubParts( rpcTempCU->getRefQP( 0 ), 0, uhDepth ); // set QP to default QP
  }
  xCheckBestMode( rpcBestCU, rpcTempCU, uhDepth );
  rpcBestCU->copyToPic(uhDepth);                                                     // Copy Best data to Picture for next partition prediction.

#if HHI_VSO
  if( !bSplit && bRecursiveCall && m_pcRdCost->getUseRenModel() )
  {
    UInt  uiWidth     = m_ppcRecoYuvBest[uhDepth]->getWidth   (   );
    UInt  uiHeight    = m_ppcRecoYuvBest[uhDepth]->getHeight  (   );
    UInt  uiSrcStride = m_ppcRecoYuvBest[uhDepth]->getStride  (   );
    Pel*  piSrc       = m_ppcRecoYuvBest[uhDepth]->getLumaAddr( 0 );
    m_pcRdCost->setRenModelData( rpcBestCU, 0, piSrc, uiSrcStride, uiWidth, uiHeight );
  }
#endif
}

Void TEncCu::xAddMVISignallingBits( TComDataCU* pcCU )
{
  const UChar uhDepth = pcCU->getTextureModeDepth( 0 );
  m_pcEntropyCoder->resetBits();
  xSaveDepthWidthHeight( pcCU );
  pcCU->setSizeSubParts( g_uiMaxCUWidth>>uhDepth, g_uiMaxCUHeight>>uhDepth, 0, uhDepth );
  pcCU->setDepthSubParts( uhDepth, 0 );
  pcCU->setPartSizeSubParts( SIZE_2Nx2N, 0, uhDepth );
  pcCU->setMergeFlagSubParts( true, 0, 0, uhDepth );
  pcCU->setMergeIndexSubParts( HHI_MPI_MERGE_POS, 0, 0, uhDepth );

  // check for skip mode
  {
    Bool bAllZero = true;
    for( UInt ui = 0; ui < pcCU->getTotalNumPart(); ui++ )
    {
      if( pcCU->getCbf( ui, TEXT_LUMA ) || pcCU->getCbf( ui, TEXT_CHROMA_U ) || pcCU->getCbf( ui, TEXT_CHROMA_V ) )
      {
        bAllZero = false;
        break;
      }
    }
    if( bAllZero )
      pcCU->setPredModeSubParts( MODE_SKIP, 0, uhDepth );
  }


  m_pcEntropyCoder->encodeSplitFlag( pcCU, 0, uhDepth, true );
  m_pcEntropyCoder->encodeSkipFlag( pcCU, 0, true );

  if( pcCU->isSkipped( 0 ) )
  {
    m_pcEntropyCoder->encodeMergeIndex( pcCU, 0, 0, true );
  }
  else
  {
    m_pcEntropyCoder->encodePredMode( pcCU, 0, true );
    m_pcEntropyCoder->encodePartSize( pcCU, 0, uhDepth, true );
    // prediction Info ( Intra : direction mode, Inter : Mv, reference idx )
    m_pcEntropyCoder->encodePredInfo( pcCU, 0, true );
  }
  xRestoreDepthWidthHeight( pcCU );

  pcCU->getTotalBits() += m_pcEntropyCoder->getNumberOfWrittenBits();
}

Void TEncCu::xSaveDepthWidthHeight( TComDataCU* pcCU )
{
  const Int iSizeInUchar  = sizeof( UChar ) * pcCU->getTotalNumPart();
  memcpy( m_puhDepthSaved, pcCU->getDepth(), iSizeInUchar );
  memcpy( m_puhWidthSaved, pcCU->getWidth(), iSizeInUchar );
  memcpy( m_puhHeightSaved, pcCU->getHeight(), iSizeInUchar );
}

Void TEncCu::xRestoreDepthWidthHeight( TComDataCU* pcCU )
{
  const Int iSizeInUchar  = sizeof( UChar ) * pcCU->getTotalNumPart();
  memcpy( pcCU->getDepth(), m_puhDepthSaved, iSizeInUchar );
  memcpy( pcCU->getWidth(), m_puhWidthSaved, iSizeInUchar );
  memcpy( pcCU->getHeight(), m_puhHeightSaved, iSizeInUchar );
}
#endif

//! \}
