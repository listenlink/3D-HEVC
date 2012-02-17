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


/** \file     TEncCU.cpp
    \brief    CU encoder class
*/

#include <stdio.h>
#include "TEncTop.h"
#include "TEncCu.h"
#include "TEncAnalyze.h"

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
  m_ppcResPredTmp  = new TComYuv*[m_uhTotalDepth-1];

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

    m_ppcBestCU[i] = new TComDataCU; m_ppcBestCU[i]->create( uiNumPartitions, uiWidth, uiHeight, false );
    m_ppcTempCU[i] = new TComDataCU; m_ppcTempCU[i]->create( uiNumPartitions, uiWidth, uiHeight, false );

    m_ppcPredYuvBest[i] = new TComYuv; m_ppcPredYuvBest[i]->create(uiWidth, uiHeight);
    m_ppcResiYuvBest[i] = new TComYuv; m_ppcResiYuvBest[i]->create(uiWidth, uiHeight);
    m_ppcRecoYuvBest[i] = new TComYuv; m_ppcRecoYuvBest[i]->create(uiWidth, uiHeight);

    m_ppcPredYuvTemp[i] = new TComYuv; m_ppcPredYuvTemp[i]->create(uiWidth, uiHeight);
    m_ppcResiYuvTemp[i] = new TComYuv; m_ppcResiYuvTemp[i]->create(uiWidth, uiHeight);
    m_ppcRecoYuvTemp[i] = new TComYuv; m_ppcRecoYuvTemp[i]->create(uiWidth, uiHeight);

    m_ppcOrigYuv    [i] = new TComYuv; m_ppcOrigYuv    [i]->create(uiWidth, uiHeight);

    m_ppcResPredTmp [i] = new TComYuv; m_ppcResPredTmp [i]->create(uiWidth, uiHeight);
  }

  // initialize partition order.
  UInt* piTmp = &g_auiZscanToRaster[0];
  initZscanToRaster( m_uhTotalDepth, 1, 0, piTmp);
  initRasterToZscan( uiMaxWidth, uiMaxHeight, m_uhTotalDepth );

  // initialize conversion matrix from partition index to pel
  initRasterToPelXY( uiMaxWidth, uiMaxHeight, m_uhTotalDepth );
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
    if(m_ppcResPredTmp[i])
    {
      m_ppcResPredTmp [i]->destroy(); delete m_ppcResPredTmp[i];  m_ppcResPredTmp[i] = NULL;
    }
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
  if(m_ppcResPredTmp)
  {
    delete [] m_ppcResPredTmp;
    m_ppcResPredTmp = NULL;
  }
}

/** \param    pcEncTop      pointer of encoder class
 */
Void TEncCu::init( TEncTop* pcEncTop )
{
  m_pcEncCfg           = pcEncTop;
  m_pcEncTop           = pcEncTop;
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
  // single-QP coding mode
  if ( rpcCU->getSlice()->getSPS()->getUseDQP() == false )
  {
    // initialize CU data
    m_ppcBestCU[0]->initCU( rpcCU->getPic(), rpcCU->getAddr() );
    m_ppcTempCU[0]->initCU( rpcCU->getPic(), rpcCU->getAddr() );

    // analysis of CU
    xCompressCU( m_ppcBestCU[0], m_ppcTempCU[0], 0 );
  }
  // multiple-QP coding mode
  else
  {
    Int iQP  = rpcCU->getSlice()->getSliceQp();
    Int idQP = m_pcEncCfg->getMaxDeltaQP();
    Int i;
    Int iBestQP = iQP;
    Double fBestCost = MAX_DOUBLE;

    rpcCU->getSlice()->setSliceQp( iQP );
    m_ppcBestCU[0]->initCU( rpcCU->getPic(), rpcCU->getAddr() );
    m_ppcTempCU[0]->initCU( rpcCU->getPic(), rpcCU->getAddr() );
    m_ppcBestCU[0]->setQPSubParts( iQP, 0, 0 );
    m_ppcTempCU[0]->setQPSubParts( iQP, 0, 0 );

    // first try
    xCompressCU( m_ppcBestCU[0], m_ppcTempCU[0], 0 );

    // for non-zero residual case
    if ( !( m_ppcBestCU[0]->isSkipped( 0 ) && m_ppcBestCU[0]->getDepth( 0 ) == 0 ) )
    {
      // add dQP bits
      m_pcEntropyCoder->resetBits();
      m_pcEntropyCoder->encodeQP( m_ppcBestCU[0], 0, false );
      m_ppcBestCU[0]->getTotalBits() += m_pcEntropyCoder->getNumberOfWrittenBits(); // dQP bits


#if HHI_VSO
      if ( m_pcRdCost->getUseLambdaScaleVSO() )
      {
        m_ppcBestCU[0]->getTotalCost() = m_pcRdCost->calcRdCostVSO( m_ppcBestCU[0]->getTotalBits(), m_ppcBestCU[0]->getTotalDistortion() );
      }
      else
#endif
      {
        m_ppcBestCU[0]->getTotalCost()  = m_pcRdCost->calcRdCost( m_ppcBestCU[0]->getTotalBits(), m_ppcBestCU[0]->getTotalDistortion() );
      }

      fBestCost = m_ppcBestCU[0]->getTotalCost();

      // try every case
      for ( i=iQP-idQP; i<=iQP+idQP; i++ )
      {
        if ( i == iQP ) continue;

        rpcCU->getSlice()->setSliceQp( i );
        m_ppcBestCU[0]->initCU( rpcCU->getPic(), rpcCU->getAddr() );
        m_ppcTempCU[0]->initCU( rpcCU->getPic(), rpcCU->getAddr() );
        m_ppcBestCU[0]->setQPSubParts( i, 0, 0 );
        m_ppcTempCU[0]->setQPSubParts( i, 0, 0 );

        xCompressCU( m_ppcBestCU[0], m_ppcTempCU[0], 0 );

        // add dQP bits
        rpcCU->getSlice()->setSliceQp( iQP );
        m_pcEntropyCoder->resetBits();
        m_pcEntropyCoder->encodeQP( m_ppcBestCU[0], 0, false );
        m_ppcBestCU[0]->getTotalBits() += m_pcEntropyCoder->getNumberOfWrittenBits(); // dQP bits

#if HHI_VSO
        if (m_pcRdCost->getUseLambdaScaleVSO())
        {
          m_ppcBestCU[0]->getTotalCost()  = m_pcRdCost->calcRdCostVSO( m_ppcBestCU[0]->getTotalBits(), m_ppcBestCU[0]->getTotalDistortion() );
        }
        else
#endif
        {
          m_ppcBestCU[0]->getTotalCost()  = m_pcRdCost->calcRdCost( m_ppcBestCU[0]->getTotalBits(), m_ppcBestCU[0]->getTotalDistortion() );
        }

        if ( fBestCost > m_ppcBestCU[0]->getTotalCost() )
        {
          fBestCost = m_ppcBestCU[0]->getTotalCost();
          iBestQP   = i;
        }
      }

      // perform best case
      rpcCU->getSlice()->setSliceQp( iBestQP );
      m_ppcBestCU[0]->initCU( rpcCU->getPic(), rpcCU->getAddr() );
      m_ppcTempCU[0]->initCU( rpcCU->getPic(), rpcCU->getAddr() );
      m_ppcBestCU[0]->setQPSubParts( iBestQP, 0, 0 );
      m_ppcTempCU[0]->setQPSubParts( iBestQP, 0, 0 );

      xCompressCU( m_ppcBestCU[0], m_ppcTempCU[0], 0 );

      // add dQP bits
      rpcCU->getSlice()->setSliceQp( iQP );
      m_pcEntropyCoder->resetBits();
      m_pcEntropyCoder->encodeQP( m_ppcBestCU[0], 0, false );
      m_ppcBestCU[0]->getTotalBits() += m_pcEntropyCoder->getNumberOfWrittenBits(); // dQP bits

#if HHI_VSO
      if (m_pcRdCost->getUseLambdaScaleVSO())
      {
        m_ppcBestCU[0]->getTotalCost()  = m_pcRdCost->calcRdCostVSO( m_ppcBestCU[0]->getTotalBits(), m_ppcBestCU[0]->getTotalDistortion() );
      }
      else
#endif
      {
        m_ppcBestCU[0]->getTotalCost()  = m_pcRdCost->calcRdCost( m_ppcBestCU[0]->getTotalBits(), m_ppcBestCU[0]->getTotalDistortion() );
      }
    }
  }
}

/** \param  pcCU  pointer of CU data class, bForceTerminate when set to true terminates slice (default is false).
 */
Void TEncCu::encodeCU ( TComDataCU* pcCU, Bool bForceTerminate )
{
#if SNY_DQP
  if ( pcCU->getSlice()->getSPS()->getUseDQP() )
  {
    pcCU->setdQPFlag(true);
  }
#endif//SNY_DQP
  // encode CU data
  xEncodeCU( pcCU, 0, 0 );

#if SNY_DQP
  // dQP: only for LCU
  if ( pcCU->getSlice()->getSPS()->getUseDQP() )
  {
    if ( pcCU->isSkipped( 0 ) && pcCU->getDepth( 0 ) == 0 )
    {
    }
    else if ( pcCU->getdQPFlag())// non-skip
    {

      m_pcEntropyCoder->encodeQP( pcCU, 0 );
      pcCU->setdQPFlag(false);
    }
  }
#else
  // dQP: only for LCU
  if ( pcCU->getSlice()->getSPS()->getUseDQP() )
  {
    if ( pcCU->isSkipped( 0 ) && pcCU->getDepth( 0 ) == 0 )
    {
    }
    else
    {
      m_pcEntropyCoder->encodeQP( pcCU, 0 );
    }
  }
#endif//SNY_DQP

  //--- write terminating bit ---
  Bool bTerminateSlice = bForceTerminate;
  UInt uiCUAddr = pcCU->getAddr();

  if (uiCUAddr == (pcCU->getPic()->getNumCUsInFrame()-1) )
    bTerminateSlice = true;

  if (uiCUAddr == (pcCU->getSlice()->getSliceCurEndCUAddr()-1))
    bTerminateSlice = true;

  m_pcEntropyCoder->encodeTerminatingBit( bTerminateSlice ? 1 : 0 );

  // Encode slice finish
  if ( bTerminateSlice )
  {
    m_pcEntropyCoder->encodeSliceFinish();
  }
}

// ====================================================================================================================
// Protected member functions
// ====================================================================================================================

Void TEncCu::xCompressCU( TComDataCU*& rpcBestCU, TComDataCU*& rpcTempCU, UInt uiDepth )
{
  TComPic* pcPic = rpcBestCU->getPic();

  // get Original YUV data from picture
  m_ppcOrigYuv[uiDepth]->copyFromPicYuv( pcPic->getPicYuvOrg(), rpcBestCU->getAddr(), rpcBestCU->getZorderIdxInCU() );

  // variables for fast encoder decision
  TComDataCU* pcTempCU;
  Bool    bEarlySkip  = false;
  Bool    bTrySplit    = true;
  Double  fRD_Skip    = MAX_DOUBLE;

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

#if ( HHI_INTERVIEW_SKIP)
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
  if( ( uiRPelX < rpcBestCU->getSlice()->getSPS()->getWidth() ) && ( uiBPelY < rpcBestCU->getSlice()->getSPS()->getHeight() ) )
  {
    // do inter modes
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
        bResPredAvailable       = rpcBestCU->getResidualSamples( 0, m_ppcResPredTmp[uiDepth] );
      }

      for( UInt uiResPrdId = 0; uiResPrdId < ( bResPredAvailable ? 2 : 1 ); uiResPrdId++ )
      {
        Bool bResPredFlag  = ( uiResPrdId > 0 );
#endif

      // SKIP
      pcTempCU = rpcTempCU;

      if( pcPic->getSlice(0)->getSPS()->getUseMRG() )
      {
#if !HHI_MRG_SKIP
#if HHI_INTER_VIEW_RESIDUAL_PRED
        rpcTempCU->setResPredIndicator( bResPredAvailable, bResPredFlag );
#endif
        xCheckRDCostAMVPSkip ( rpcBestCU, rpcTempCU );        rpcTempCU->initEstData();
#endif
#if HHI_INTER_VIEW_RESIDUAL_PRED
        rpcTempCU->setResPredIndicator( bResPredAvailable, bResPredFlag );
#endif
#if HHI_INTERVIEW_SKIP
        xCheckRDCostMerge2Nx2N( rpcBestCU, rpcTempCU, bFullyRenderedSec );            rpcTempCU->initEstData();
#else
        xCheckRDCostMerge2Nx2N( rpcBestCU, rpcTempCU );            rpcTempCU->initEstData();
#endif
      }
      else
      {
#if HHI_INTER_VIEW_RESIDUAL_PRED
        rpcTempCU->setResPredIndicator( bResPredAvailable, bResPredFlag );
#endif
        xCheckRDCostAMVPSkip ( rpcBestCU, rpcTempCU );        rpcTempCU->initEstData();
      }

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
#if HHI_DISABLE_INTER_NxN_SPLIT
#if HHI_INTER_VIEW_RESIDUAL_PRED
        rpcTempCU->setResPredIndicator( bResPredAvailable, bResPredFlag );
#endif
#if HHI_INTERVIEW_SKIP
        xCheckRDCostInter( rpcBestCU, rpcTempCU, SIZE_2Nx2N, bFullyRenderedSec );  rpcTempCU->initEstData();
#else
        xCheckRDCostInter( rpcBestCU, rpcTempCU, SIZE_2Nx2N );  rpcTempCU->initEstData();
#endif
        if( uiDepth == g_uiMaxCUDepth - g_uiAddCUDepth )
        {
#if HHI_INTER_VIEW_RESIDUAL_PRED
          rpcTempCU->setResPredIndicator( bResPredAvailable, bResPredFlag );
#endif
#if HHI_INTERVIEW_SKIP
          xCheckRDCostInter( rpcBestCU, rpcTempCU, SIZE_2Nx2N, bFullyRenderedSec );  rpcTempCU->initEstData();
#else
          xCheckRDCostInter( rpcBestCU, rpcTempCU, SIZE_NxN   );  rpcTempCU->initEstData();
#endif
        }
#else
#if HHI_INTER_VIEW_RESIDUAL_PRED
        rpcTempCU->setResPredIndicator( bResPredAvailable, bResPredFlag );
#endif
#if HHI_INTERVIEW_SKIP
        xCheckRDCostInter( rpcBestCU, rpcTempCU, SIZE_2Nx2N, bFullyRenderedSec );  rpcTempCU->initEstData();
#else
        xCheckRDCostInter( rpcBestCU, rpcTempCU, SIZE_2Nx2N );  rpcTempCU->initEstData();
#endif
#if HHI_INTER_VIEW_RESIDUAL_PRED
        rpcTempCU->setResPredIndicator( bResPredAvailable, bResPredFlag );
#endif
#if HHI_INTERVIEW_SKIP
        xCheckRDCostInter( rpcBestCU, rpcTempCU, SIZE_NxN, bFullyRenderedSec   );  rpcTempCU->initEstData();
#else
        xCheckRDCostInter( rpcBestCU, rpcTempCU, SIZE_NxN   );  rpcTempCU->initEstData();
#endif
#endif
      }

#if HHI_RMP_SWITCH
      if( pcPic->getSlice(0)->getSPS()->getUseRMP() )
#endif
      { // 2NxN, Nx2N
#if HHI_INTER_VIEW_RESIDUAL_PRED
        rpcTempCU->setResPredIndicator( bResPredAvailable, bResPredFlag );
#endif
#if HHI_INTERVIEW_SKIP
        xCheckRDCostInter( rpcBestCU, rpcTempCU, SIZE_Nx2N, bFullyRenderedSec  );  rpcTempCU->initEstData();
#else
        xCheckRDCostInter( rpcBestCU, rpcTempCU, SIZE_Nx2N  );  rpcTempCU->initEstData();
#endif
#if HHI_INTER_VIEW_RESIDUAL_PRED
        rpcTempCU->setResPredIndicator( bResPredAvailable, bResPredFlag );
#endif
#if HHI_INTERVIEW_SKIP
        xCheckRDCostInter      ( rpcBestCU, rpcTempCU, SIZE_2NxN, bFullyRenderedSec  );  rpcTempCU->initEstData();
#else
        xCheckRDCostInter      ( rpcBestCU, rpcTempCU, SIZE_2NxN  );  rpcTempCU->initEstData();
#endif
      }

#if HHI_INTER_VIEW_RESIDUAL_PRED
    } // uiResPrdId
#endif
    }

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
        xCheckRDCostIntra( rpcBestCU, rpcTempCU, SIZE_2Nx2N ); rpcTempCU->initEstData();
#if MTK_DISABLE_INTRA_NxN_SPLIT
        if( uiDepth == g_uiMaxCUDepth - g_uiAddCUDepth )
#endif
        {
          if( rpcTempCU->getWidth(0) > ( 1 << rpcTempCU->getSlice()->getSPS()->getQuadtreeTULog2MinSize() ) )
          {
            xCheckRDCostIntra( rpcBestCU, rpcTempCU, SIZE_NxN   ); rpcTempCU->initEstData();
          }
        }
      }
    }

    m_pcEntropyCoder->resetBits();
    m_pcEntropyCoder->encodeSplitFlag( rpcBestCU, 0, uiDepth, true );
    rpcBestCU->getTotalBits() += m_pcEntropyCoder->getNumberOfWrittenBits(); // split bits

#if HHI_VSO
    if (m_pcRdCost->getUseLambdaScaleVSO())
    {
      rpcBestCU->getTotalCost()  = m_pcRdCost->calcRdCostVSO( rpcBestCU->getTotalBits(), rpcBestCU->getTotalDistortion() );
    }
    else
#endif
    {
#if HHI_INTERVIEW_SKIP
  if(  m_pcEncCfg->getInterViewSkip())
      {
        TComYuv*  pRec    = m_ppcRecoYuvBest[ uiDepth ];
        TComYuv*  pOrg    = m_ppcOrigYuv    [ uiDepth ];
        Pel*      pUsedY  = pcPic->getUsedPelsMap()->getLumaAddr( rpcBestCU->getAddr(), rpcBestCU->getZorderIdxInCU() );
        Pel*      pUsedU  = pcPic->getUsedPelsMap()->getCbAddr  ( rpcBestCU->getAddr(), rpcBestCU->getZorderIdxInCU() );
        Pel*      pUsedV  = pcPic->getUsedPelsMap()->getCrAddr  ( rpcBestCU->getAddr(), rpcBestCU->getZorderIdxInCU() );
        Int       iUStrdY = pcPic->getUsedPelsMap()->getStride  ();
        Int       iUStrdC = pcPic->getUsedPelsMap()->getCStride ();
        UInt      uiWdt   = rpcBestCU->getWidth ( 0 );
        UInt      uiHgt   = rpcBestCU->getHeight( 0 );
        UInt      uiDist  = ( m_pcRdCost->getDistPart( pRec->getLumaAddr(), pRec->getStride(),  pOrg->getLumaAddr(), pOrg->getStride(),  pUsedY, iUStrdY, uiWdt,      uiHgt      )
                            + m_pcRdCost->getDistPart( pRec->getCbAddr(),   pRec->getCStride(), pOrg->getCbAddr(),   pOrg->getCStride(), pUsedU, iUStrdC, uiWdt >> 1, uiHgt >> 1 )
                            + m_pcRdCost->getDistPart( pRec->getCrAddr(),   pRec->getCStride(), pOrg->getCrAddr(),   pOrg->getCStride(), pUsedV, iUStrdC, uiWdt >> 1, uiHgt >> 1 ) );
//        printf("\nD(as is) = %d,   D(new) = %d,  diff = %d", rpcBestCU->getTotalDistortion(), uiDist, Int(rpcBestCU->getTotalDistortion()-uiDist) );
        rpcBestCU->getTotalDistortion() = uiDist;
      }
#endif
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
#if HHI_MPI
    if( rpcBestCU->getSlice()->getSPS()->getUseMVI() && rpcBestCU->getSlice()->getSliceType() != I_SLICE )
    {
      xCheckRDCostMvInheritance( rpcBestCU, rpcTempCU, uiDepth, false, false ); rpcTempCU->initEstData();
      rpcTempCU->setSizeSubParts( g_uiMaxCUWidth>>uiDepth, g_uiMaxCUHeight>>uiDepth, 0, uiDepth );
      rpcTempCU->setDepthSubParts( uiDepth, 0 );
      xCheckRDCostMvInheritance( rpcBestCU, rpcTempCU, uiDepth, true, false );  rpcTempCU->initEstData();
    }
#endif
  }
  else
  {
    bBoundary = true;
  }

  // further split
  if( bTrySplit && uiDepth < g_uiMaxCUDepth - g_uiAddCUDepth )
  {
#if HHI_VSO
    // reset Model
    if( m_pcRdCost->getUseRenModel() )
    {
      UInt  uiWidth     = m_ppcBestCU[uiDepth]->getWidth ( 0 );
      UInt  uiHeight    = m_ppcBestCU[uiDepth]->getHeight( 0 );
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
      pcSubBestPartCU->initSubCU( rpcBestCU, uiPartUnitIdx, uhNextDepth );           // clear sub partition datas or init.
      pcSubTempPartCU->initSubCU( rpcBestCU, uiPartUnitIdx, uhNextDepth );           // clear sub partition datas or init.

      if( ( pcSubBestPartCU->getCUPelX() < pcSubBestPartCU->getSlice()->getSPS()->getWidth() ) && ( pcSubBestPartCU->getCUPelY() < pcSubBestPartCU->getSlice()->getSPS()->getHeight() ) )
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

        xCompressCU( pcSubBestPartCU, pcSubTempPartCU, uhNextDepth );

#if HHI_VSO
        if( m_pcRdCost->getUseRenModel() )
        {
          UInt  uiWidth     = pcSubBestPartCU->getWidth ( 0 );
          UInt  uiHeight    = pcSubBestPartCU->getHeight( 0 );
          Pel*  piSrc       = m_ppcRecoYuvBest[pcSubBestPartCU->getDepth(0)]->getLumaAddr( 0 );
          UInt  uiSrcStride = m_ppcRecoYuvBest[pcSubBestPartCU->getDepth(0)]->getStride();
          m_pcRdCost->setRenModelData( pcSubBestPartCU, 0, piSrc, uiSrcStride, uiWidth, uiHeight );
        }
#endif
        rpcTempCU->copyPartFrom( pcSubBestPartCU, uiPartUnitIdx, uhNextDepth );         // Keep best part data to current temporary data.
        xCopyYuv2Tmp( pcSubBestPartCU->getTotalNumPart()*uiPartUnitIdx, uhNextDepth );
      }
    }

    if( !bBoundary )
    {
      m_pcEntropyCoder->resetBits();
      m_pcEntropyCoder->encodeSplitFlag( rpcTempCU, 0, uiDepth, true );

      rpcTempCU->getTotalBits() += m_pcEntropyCoder->getNumberOfWrittenBits(); // split bits
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
    if ( m_pcRdCost->getUseLambdaScaleVSO())
    {
      rpcTempCU->getTotalCost()  = m_pcRdCost->calcRdCostVSO( rpcTempCU->getTotalBits(), rpcTempCU->getTotalDistortion() );
    }
    else
#endif
    {
      rpcTempCU->getTotalCost()  = m_pcRdCost->calcRdCost( rpcTempCU->getTotalBits(), rpcTempCU->getTotalDistortion() );
    }

    if( m_bUseSBACRD )
    {
      m_pppcRDSbacCoder[uhNextDepth][CI_NEXT_BEST]->store(m_pppcRDSbacCoder[uiDepth][CI_TEMP_BEST]);
    }
    xCheckBestMode( rpcBestCU, rpcTempCU, uiDepth );                                          // RD compare current larger prediction

#if HHI_VSO
    if( m_pcRdCost->getUseRenModel() )
    {
      UInt  uiWidth     = rpcBestCU->getWidth ( 0 );
      UInt  uiHeight    = rpcBestCU->getHeight( 0 );
      Pel*  piSrc       = m_ppcRecoYuvBest[uiDepth]->getLumaAddr( 0 );
      UInt  uiSrcStride = m_ppcRecoYuvBest[uiDepth]->getStride();
      m_pcRdCost->setRenModelData( rpcBestCU, 0, piSrc, uiSrcStride, uiWidth, uiHeight );
    }
#endif
  }                                                                                  // with sub partitioned prediction.

  rpcBestCU->copyToPic(uiDepth);                                                     // Copy Best data to Picture for next partition prediction.

  if( bBoundary )
    return;

  xCopyYuv2Pic( rpcBestCU->getPic(), rpcBestCU->getAddr(), rpcBestCU->getZorderIdxInCU(), uiDepth );   // Copy Yuv data to picture Yuv

  // Assert if Best prediction mode is NONE
  // Selected mode's RD-cost must be not MAX_DOUBLE.
  assert( rpcBestCU->getPartitionSize ( 0 ) != SIZE_NONE  );
  assert( rpcBestCU->getPredictionMode( 0 ) != MODE_NONE  );
  assert( rpcBestCU->getTotalCost     (   ) != MAX_DOUBLE );
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

  if( ( uiRPelX < pcCU->getSlice()->getSPS()->getWidth() ) && ( uiBPelY < pcCU->getSlice()->getSPS()->getHeight() ) )
  {
#if HHI_MPI
    if( pcCU->getTextureModeDepth( uiAbsPartIdx ) == -1 || uiDepth < pcCU->getTextureModeDepth( uiAbsPartIdx ) )
#endif
      m_pcEntropyCoder->encodeSplitFlag( pcCU, uiAbsPartIdx, uiDepth );
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

    if( ( uiRPelX < pcCU->getSlice()->getSPS()->getWidth() ) && ( uiBPelY < pcCU->getSlice()->getSPS()->getHeight() ) )
      m_pcEntropyCoder->encodeSplitFlag( pcCU, uiAbsPartIdx, uiDepth );
    if( !pcCU->getSlice()->isIntra() )
    {
      m_pcEntropyCoder->encodeSkipFlag( pcCU, uiAbsPartIdx );
    }

    if( pcCU->isSkipped( uiAbsPartIdx ) )
    {
#if HHI_MRG_SKIP
      m_pcEntropyCoder->encodeMergeIndex( pcCU, uiAbsPartIdx, 0 );
#else
      if ( pcCU->getSlice()->getNumRefIdx( REF_PIC_LIST_0 ) > 0 ) //if ( ref. frame list0 has at least 1 entry )
      {
        m_pcEntropyCoder->encodeMVPIdx( pcCU, uiAbsPartIdx, REF_PIC_LIST_0);
      }
      if ( pcCU->getSlice()->getNumRefIdx( REF_PIC_LIST_1 ) > 0 ) //if ( ref. frame list1 has at least 1 entry )
      {
        m_pcEntropyCoder->encodeMVPIdx( pcCU, uiAbsPartIdx, REF_PIC_LIST_1);
      }
#endif
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
    for ( UInt uiPartUnitIdx = 0; uiPartUnitIdx < 4; uiPartUnitIdx++, uiAbsPartIdx+=uiQNumParts )
    {
      uiLPelX   = pcCU->getCUPelX() + g_auiRasterToPelX[ g_auiZscanToRaster[uiAbsPartIdx] ];
      uiTPelY   = pcCU->getCUPelY() + g_auiRasterToPelY[ g_auiZscanToRaster[uiAbsPartIdx] ];

      if( ( uiLPelX < pcCU->getSlice()->getSPS()->getWidth() ) && ( uiTPelY < pcCU->getSlice()->getSPS()->getHeight() ) )
        xEncodeCU( pcCU, uiAbsPartIdx, uiDepth+1 );
    }
    return;
  }

#if TSB_ALF_HEADER
#else
  m_pcEntropyCoder->encodeAlfCtrlFlag( pcCU, uiAbsPartIdx );
#endif

#if HHI_MPI
  if( !pcCU->getSlice()->isIntra() && pcCU->getTextureModeDepth( uiAbsPartIdx ) == -1 )
#else
  if( !pcCU->getSlice()->isIntra() )
#endif
  {
    m_pcEntropyCoder->encodeSkipFlag( pcCU, uiAbsPartIdx );
  }

  if( pcCU->isSkipped( uiAbsPartIdx ) )
  {
#if HHI_MRG_SKIP
    m_pcEntropyCoder->encodeMergeIndex( pcCU, uiAbsPartIdx, 0 );
#else
    if ( pcCU->getSlice()->getNumRefIdx( REF_PIC_LIST_0 ) > 0 ) //if ( ref. frame list0 has at least 1 entry )
    {
      m_pcEntropyCoder->encodeMVPIdx( pcCU, uiAbsPartIdx, REF_PIC_LIST_0);
    }
    if ( pcCU->getSlice()->getNumRefIdx( REF_PIC_LIST_1 ) > 0 ) //if ( ref. frame list1 has at least 1 entry )
    {
      m_pcEntropyCoder->encodeMVPIdx( pcCU, uiAbsPartIdx, REF_PIC_LIST_1);
    }
#endif
#if HHI_INTER_VIEW_RESIDUAL_PRED
    m_pcEntropyCoder->encodeResPredFlag( pcCU, uiAbsPartIdx, 0 );
#endif
    return;
  }
#if HHI_MPI
  if( pcCU->getTextureModeDepth( uiAbsPartIdx ) == -1 )
  {
#endif
    m_pcEntropyCoder->encodePredMode( pcCU, uiAbsPartIdx );

    m_pcEntropyCoder->encodePartSize( pcCU, uiAbsPartIdx, uiDepth );

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
  m_pcEntropyCoder->encodeCoeff( pcCU, uiAbsPartIdx, uiDepth, pcCU->getWidth (uiAbsPartIdx), pcCU->getHeight(uiAbsPartIdx) );
}

Void TEncCu::xCheckRDCostSkip( TComDataCU*& rpcBestCU, TComDataCU*& rpcTempCU, Bool bSkipRes )
{
  UChar uhDepth = rpcTempCU->getDepth( 0 );

#if HHI_VSO
  if( m_pcRdCost->getUseRenModel() )
  {
    UInt  uiWidth     = rpcTempCU->getWidth ( 0 );
    UInt  uiHeight    = rpcTempCU->getHeight( 0 );
    Pel*  piSrc       = m_ppcOrigYuv[uhDepth]->getLumaAddr( );
    UInt  uiSrcStride = m_ppcOrigYuv[uhDepth]->getStride();
    m_pcRdCost->setRenModelData( rpcTempCU, 0, piSrc, uiSrcStride, uiWidth, uiHeight );
  }
#endif

  rpcTempCU->setPredModeSubParts( MODE_SKIP,   0, uhDepth );
  rpcTempCU->setPartSizeSubParts( SIZE_2Nx2N,  0, uhDepth );

  m_pcPredSearch->predInterSkipSearch       ( rpcTempCU,
                                             m_ppcOrigYuv    [uhDepth],
                                             m_ppcPredYuvTemp[uhDepth],
                                             m_ppcResiYuvTemp[uhDepth],
                                             m_ppcRecoYuvTemp[uhDepth] );

  m_pcPredSearch->encodeResAndCalcRdInterCU ( rpcTempCU,
                                             m_ppcOrigYuv    [uhDepth],
                                             m_ppcPredYuvTemp[uhDepth],
                                             m_ppcResiYuvTemp[uhDepth],
                                             m_ppcResiYuvBest[uhDepth],
                                             m_ppcRecoYuvTemp[uhDepth],
                                             m_ppcResPredTmp [uhDepth],
                                             bSkipRes );

  xCheckBestMode( rpcBestCU, rpcTempCU, uhDepth );
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
  TComMvField  cMvFieldNeighbours[MRG_MAX_NUM_CANDS << 1]; // double length for mv of both lists
  UChar uhInterDirNeighbours[MRG_MAX_NUM_CANDS];
  UInt uiNeighbourCandIdx[MRG_MAX_NUM_CANDS]; //MVs with same idx => same cand

#if HHI_INTER_VIEW_RESIDUAL_PRED
  Bool  bResPrdAvail  = rpcTempCU->getResPredAvail( 0 );
  Bool  bResPrdFlag   = rpcTempCU->getResPredFlag ( 0 );
#endif

  for( UInt ui = 0; ui < MRG_MAX_NUM_CANDS; ++ui )
  {
    uhInterDirNeighbours[ui] = 0;
    uiNeighbourCandIdx[ui] = 0;
  }
  UChar uhDepth = rpcTempCU->getDepth( 0 );

#if HHI_VSO
  if( m_pcRdCost->getUseRenModel() )
  {
    UInt  uiWidth     = rpcTempCU->getWidth ( 0 );
    UInt  uiHeight    = rpcTempCU->getHeight( 0 );
    Pel*  piSrc       = m_ppcOrigYuv[uhDepth]->getLumaAddr( );
    UInt  uiSrcStride = m_ppcOrigYuv[uhDepth]->getStride();
    m_pcRdCost->setRenModelData( rpcTempCU, 0, piSrc, uiSrcStride, uiWidth, uiHeight );
  }
#endif

  rpcTempCU->setPartSizeSubParts( SIZE_2Nx2N, 0, uhDepth ); // interprets depth relative to LCU level
  rpcTempCU->getInterMergeCandidates( 0, 0, uhDepth, cMvFieldNeighbours,uhInterDirNeighbours, uiNeighbourCandIdx );

  Bool bValidCands = false;
  for( UInt uiMergeCand = 0; uiMergeCand < MRG_MAX_NUM_CANDS; ++uiMergeCand )
  {
    if( uiNeighbourCandIdx[uiMergeCand] == ( uiMergeCand + 1 ) )
    {
#if HHI_MRG_SKIP
      TComYuv* pcPredYuvTemp = NULL;
#if HHI_INTERVIEW_SKIP
      for( UInt uiNoResidual = (bSkipRes ? 1:0); uiNoResidual < 2; ++uiNoResidual )
#else
      for( UInt uiNoResidual = 0; uiNoResidual < 2; ++uiNoResidual )
#endif
      {
#endif
      bValidCands = true;
      // set MC parameters
#if HHI_MRG_SKIP
      rpcTempCU->setPredModeSubParts( MODE_SKIP, 0, uhDepth ); // interprets depth relative to LCU level
#else
      rpcTempCU->setPredModeSubParts( MODE_INTER, 0, uhDepth ); // interprets depth relative to LCU level
#endif
      rpcTempCU->setPartSizeSubParts( SIZE_2Nx2N, 0, uhDepth ); // interprets depth relative to LCU level
      rpcTempCU->setMergeFlagSubParts( true, 0, 0, uhDepth ); // interprets depth relative to LCU level
      rpcTempCU->setMergeIndexSubParts( uiMergeCand, 0, 0, uhDepth ); // interprets depth relative to LCU level
      rpcTempCU->setInterDirSubParts( uhInterDirNeighbours[uiMergeCand], 0, 0, uhDepth ); // interprets depth relative to LCU level
      for( UInt uiInner = 0; uiInner < MRG_MAX_NUM_CANDS; uiInner++ )
      {
        rpcTempCU->setNeighbourCandIdxSubParts( uiInner, uiNeighbourCandIdx[uiInner], 0, 0,uhDepth );
      }
      rpcTempCU->getCUMvField( REF_PIC_LIST_0 )->setAllMvField( cMvFieldNeighbours[0 + 2*uiMergeCand].getMv(), cMvFieldNeighbours[0 + 2*uiMergeCand].getRefIdx(), SIZE_2Nx2N, 0, 0, 0 ); // interprets depth relative to rpcTempCU level
      rpcTempCU->getCUMvField( REF_PIC_LIST_1 )->setAllMvField( cMvFieldNeighbours[1 + 2*uiMergeCand].getMv(), cMvFieldNeighbours[1 + 2*uiMergeCand].getRefIdx(), SIZE_2Nx2N, 0, 0, 0 ); // interprets depth relative to rpcTempCU level

#if HHI_INTER_VIEW_RESIDUAL_PRED
      rpcTempCU->setResPredAvailSubParts( bResPrdAvail, 0, 0, uhDepth );
      rpcTempCU->setResPredFlagSubParts ( bResPrdFlag,  0, 0, uhDepth );
#endif

#if HHI_MRG_SKIP
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
      else {
        if ( pcPredYuvTemp != m_ppcPredYuvTemp[uhDepth]) {
          //adress changes take best (old temp)
          pcPredYuvTemp = m_ppcPredYuvBest[uhDepth];
        }
      }
#if HHI_VSO
      if( m_pcRdCost->getUseRenModel() )
      { //Reset
        UInt  uiWidth     = rpcTempCU->getWidth ( 0 );
        UInt  uiHeight    = rpcTempCU->getHeight( 0 );
        Pel*  piSrc       = m_ppcOrigYuv[uhDepth]->getLumaAddr( );
        UInt  uiSrcStride = m_ppcOrigYuv[uhDepth]->getStride();
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
                                                m_ppcResPredTmp [uhDepth],
                                                (uiNoResidual? true:false) );
      Bool bQtRootCbf = rpcTempCU->getQtRootCbf(0) == 1;
#else
      // do MC
      m_pcPredSearch->motionCompensation ( rpcTempCU, m_ppcPredYuvTemp[uhDepth] );

      // estimate residual and encode everything
      m_pcPredSearch->encodeResAndCalcRdInterCU( rpcTempCU,
                                                 m_ppcOrigYuv    [uhDepth],
                                                 m_ppcPredYuvTemp[uhDepth],
                                                 m_ppcResiYuvTemp[uhDepth],
                                                 m_ppcResiYuvBest[uhDepth],
                                                 m_ppcRecoYuvTemp[uhDepth],
                                                 m_ppcResPredTmp [uhDepth],
                                                 false );
#endif
      xCheckBestMode(rpcBestCU, rpcTempCU, uhDepth );

      rpcTempCU->initEstData();
#if HHI_MRG_SKIP
      if (!bQtRootCbf)
        break;
      }
#endif
    }
  }
}

#if HHI_INTERVIEW_SKIP
Void TEncCu::xCheckRDCostInter( TComDataCU*& rpcBestCU, TComDataCU*& rpcTempCU, PartSize ePartSize, Bool bSkipRes)
#else
Void TEncCu::xCheckRDCostInter( TComDataCU*& rpcBestCU, TComDataCU*& rpcTempCU, PartSize ePartSize )
#endif
{
  UChar uhDepth = rpcTempCU->getDepth( 0 );

#if HHI_VSO
  if( m_pcRdCost->getUseRenModel() )
  {
    UInt  uiWidth     = rpcTempCU->getWidth ( 0 );
    UInt  uiHeight    = rpcTempCU->getHeight( 0 );
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
  rpcTempCU->setPartSizeSubParts    ( SIZE_2Nx2N,   0,    uhDepth );
#if HHI_INTER_VIEW_RESIDUAL_PRED
  rpcTempCU->setResPredAvailSubParts( bResPrdAvail, 0, 0, uhDepth );
  rpcTempCU->setResPredFlagSubParts ( bResPrdFlag,  0, 0, uhDepth );
#endif
  rpcTempCU->setPartSizeSubParts    ( ePartSize,    0,    uhDepth );
  rpcTempCU->setPredModeSubParts    ( MODE_INTER,   0,    uhDepth );

#if HHI_INTER_VIEW_RESIDUAL_PRED
  if( rpcTempCU->getResPredFlag( 0 ) )
  { // subtract residual prediction from original in motion search
    m_ppcOrigYuv[uhDepth]->add( m_ppcResPredTmp [uhDepth], rpcTempCU->getWidth( 0 ), rpcTempCU->getHeight( 0 ), true );
  }
#endif
#if HHI_INTERVIEW_SKIP
  m_pcPredSearch->predInterSearch ( rpcTempCU, m_ppcOrigYuv[uhDepth], m_ppcPredYuvTemp[uhDepth], m_ppcResiYuvTemp[uhDepth], m_ppcRecoYuvTemp[uhDepth], bSkipRes );
#else
  m_pcPredSearch->predInterSearch ( rpcTempCU, m_ppcOrigYuv[uhDepth], m_ppcPredYuvTemp[uhDepth], m_ppcResiYuvTemp[uhDepth], m_ppcRecoYuvTemp[uhDepth] );
#endif
#if HHI_INTER_VIEW_RESIDUAL_PRED
  if( rpcTempCU->getResPredFlag( 0 ) )
  { // add residual prediction to original again
    m_ppcOrigYuv[uhDepth]->add( m_ppcResPredTmp [uhDepth], rpcTempCU->getWidth( 0 ), rpcTempCU->getHeight( 0 ) );
  }
#endif

#if PART_MRG
  if (rpcTempCU->getWidth(0) > 8 && !rpcTempCU->getMergeFlag(0) && (ePartSize != SIZE_2Nx2N && ePartSize != SIZE_NxN))
  {
    return;
  }
#endif
#if HHI_INTERVIEW_SKIP
  m_pcPredSearch->encodeResAndCalcRdInterCU( rpcTempCU, m_ppcOrigYuv[uhDepth], m_ppcPredYuvTemp[uhDepth], m_ppcResiYuvTemp[uhDepth], m_ppcResiYuvBest[uhDepth], m_ppcRecoYuvTemp[uhDepth], m_ppcResPredTmp [uhDepth],bSkipRes );
#else
  m_pcPredSearch->encodeResAndCalcRdInterCU( rpcTempCU, m_ppcOrigYuv[uhDepth], m_ppcPredYuvTemp[uhDepth], m_ppcResiYuvTemp[uhDepth], m_ppcResiYuvBest[uhDepth], m_ppcRecoYuvTemp[uhDepth], m_ppcResPredTmp [uhDepth], false );
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

  xCheckBestMode( rpcBestCU, rpcTempCU, uhDepth );
}

Void TEncCu::xCheckRDCostIntra( TComDataCU*& rpcBestCU, TComDataCU*& rpcTempCU, PartSize eSize )
{
  UInt uiDepth = rpcTempCU->getDepth( 0 );

#if HHI_VSO
  if( m_pcRdCost->getUseRenModel() )
  {
    UInt  uiWidth     = rpcTempCU->getWidth ( 0 );
    UInt  uiHeight    = rpcTempCU->getHeight( 0 );
    Pel*  piSrc       = m_ppcOrigYuv[uiDepth]->getLumaAddr( );
    UInt  uiSrcStride = m_ppcOrigYuv[uiDepth]->getStride();
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

#if LM_CHROMA
  m_ppcRecoYuvTemp[uiDepth]->copyToPicLuma(rpcTempCU->getPic()->getPicYuvRec(), rpcTempCU->getAddr(), rpcTempCU->getZorderIdxInCU() );
#endif

  m_pcPredSearch  ->estIntraPredChromaQT( rpcTempCU, m_ppcOrigYuv[uiDepth], m_ppcPredYuvTemp[uiDepth], m_ppcResiYuvTemp[uiDepth], m_ppcRecoYuvTemp[uiDepth], uiPreCalcDistC );

  m_pcEntropyCoder->resetBits();
  m_pcEntropyCoder->encodeSkipFlag ( rpcTempCU, 0,          true );
  m_pcEntropyCoder->encodePredMode( rpcTempCU, 0,          true );
  m_pcEntropyCoder->encodePartSize( rpcTempCU, 0, uiDepth, true );
  m_pcEntropyCoder->encodePredInfo( rpcTempCU, 0,          true );

  // Encode Coefficients
  m_pcEntropyCoder->encodeCoeff( rpcTempCU, 0, uiDepth, rpcTempCU->getWidth (0), rpcTempCU->getHeight(0) );

  if( m_bUseSBACRD ) m_pcRDGoOnSbacCoder->store(m_pppcRDSbacCoder[uiDepth][CI_TEMP_BEST]);

  rpcTempCU->getTotalBits() = m_pcEntropyCoder->getNumberOfWrittenBits();
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

  xCheckBestMode( rpcBestCU, rpcTempCU, uiDepth );
}

// check whether current try is the best
Void TEncCu::xCheckBestMode( TComDataCU*& rpcBestCU, TComDataCU*& rpcTempCU, UChar uhDepth )
{
  if( rpcTempCU->getTotalCost() < rpcBestCU->getTotalCost() )
  {
    TComYuv* pcYuv;

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

Void TEncCu::xCheckRDCostAMVPSkip           ( TComDataCU*& rpcBestCU, TComDataCU*& rpcTempCU )
{
  UChar uhDepth = rpcTempCU->getDepth(0);

#if HHI_INTER_VIEW_RESIDUAL_PRED
  Bool  bResPrdAvail  = rpcTempCU->getResPredAvail( 0 );
  Bool  bResPrdFlag   = rpcTempCU->getResPredFlag ( 0 );
#endif

  AMVPInfo cAMVPInfo0;
  cAMVPInfo0.iN = 0;

  AMVPInfo cAMVPInfo1;
  cAMVPInfo1.iN = 0;

  if (rpcTempCU->getAMVPMode(0) == AM_EXPL)
  {
    rpcTempCU->setPredModeSubParts( MODE_SKIP, 0, uhDepth );
    rpcTempCU->setPartSizeSubParts( SIZE_2Nx2N,  0, uhDepth );

    if ( rpcTempCU->getSlice()->isInterP() && rpcTempCU->getSlice()->getNumRefIdx( REF_PIC_LIST_0 ) > 0 )
    {
      rpcTempCU->fillMvpCand(0, 0, REF_PIC_LIST_0, 0, &cAMVPInfo0);
    }
    else if ( rpcTempCU->getSlice()->isInterB() &&
             rpcTempCU->getSlice()->getNumRefIdx( REF_PIC_LIST_0 ) > 0 &&
             rpcTempCU->getSlice()->getNumRefIdx( REF_PIC_LIST_1 ) > 0  )
    {
      rpcTempCU->fillMvpCand(0, 0, REF_PIC_LIST_0, 0, &cAMVPInfo0);
      rpcTempCU->fillMvpCand(0, 0, REF_PIC_LIST_1, 0, &cAMVPInfo1);
    }
    else
    {
      assert( 0 );
    }
  }

  Int iMVP0, iMVP1;

  for (iMVP0 = (cAMVPInfo0.iN > 0? 0:-1); iMVP0 < cAMVPInfo0.iN; iMVP0++)
  {
    for (iMVP1 = (cAMVPInfo1.iN > 0? 0:-1); iMVP1 < cAMVPInfo1.iN; iMVP1++)
    {
      rpcTempCU->setPredModeSubParts( MODE_SKIP, 0, uhDepth );
      rpcTempCU->setPartSizeSubParts( SIZE_2Nx2N,  0, uhDepth );
#if HHI_INTER_VIEW_RESIDUAL_PRED
      rpcTempCU->setResPredAvailSubParts( bResPrdAvail, 0, 0, uhDepth );
      rpcTempCU->setResPredFlagSubParts ( bResPrdFlag,  0, 0, uhDepth );
#endif

      if (rpcTempCU->getSlice()->isInterB())
        rpcTempCU->setInterDirSubParts( 3, 0, 0, uhDepth );

      rpcTempCU->setMVPIdxSubParts( iMVP0, REF_PIC_LIST_0, 0, 0, uhDepth );
      rpcTempCU->setMVPIdxSubParts( iMVP1, REF_PIC_LIST_1, 0, 0, uhDepth );

      rpcTempCU->setMVPNumSubParts( cAMVPInfo0.iN, REF_PIC_LIST_0, 0, 0, uhDepth );
      rpcTempCU->setMVPNumSubParts( cAMVPInfo1.iN, REF_PIC_LIST_1, 0, 0, uhDepth );

      xCopyAMVPInfo(&cAMVPInfo0, rpcTempCU->getCUMvField(REF_PIC_LIST_0)->getAMVPInfo());
      xCopyAMVPInfo(&cAMVPInfo1, rpcTempCU->getCUMvField(REF_PIC_LIST_1)->getAMVPInfo());
      xCheckRDCostSkip ( rpcBestCU, rpcTempCU, true );      rpcTempCU->initEstData();
    }
  }
}

Void TEncCu::xCopyAMVPInfo (AMVPInfo* pSrc, AMVPInfo* pDst)
{
  pDst->iN = pSrc->iN;
  for (Int i = 0; i < pSrc->iN; i++)
  {
    pDst->m_acMvCand[i] = pSrc->m_acMvCand[i];
  }
}

Void TEncCu::xCopyYuv2Pic(TComPic* rpcPic, UInt uiCUAddr, UInt uiAbsZorderIdx, UInt uiDepth)
{
  m_ppcRecoYuvBest[uiDepth]->copyToPicYuv( rpcPic->getPicYuvRec (), uiCUAddr, uiAbsZorderIdx );
}

Void TEncCu::xCopyYuv2Tmp( UInt uiPartUnitIdx, UInt uiNextDepth )
{
  UInt uiCurrDepth = uiNextDepth - 1;
  m_ppcRecoYuvBest[uiNextDepth]->copyToPartYuv( m_ppcRecoYuvTemp[uiCurrDepth], uiPartUnitIdx );
}

#if HHI_MPI
Void TEncCu::xCheckRDCostMvInheritance( TComDataCU*& rpcBestCU, TComDataCU*& rpcTempCU, UChar uhTextureModeDepth, Bool bSkipResidual, Bool bRecursiveCall )
{
  assert( rpcTempCU->getSlice()->getSPS()->isDepth() );
  TComDataCU *pcTextureCU = rpcTempCU->getSlice()->getTexturePic()->getCU( rpcTempCU->getAddr() );

  const UChar uhDepth  = rpcTempCU->getDepth( 0 );
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
    UInt  uiWidth     = m_ppcTempCU [uhDepth]->getWidth ( 0 );
    UInt  uiHeight    = m_ppcTempCU [uhDepth]->getHeight( 0 );
    Pel*  piSrc       = m_ppcOrigYuv[uhDepth]->getLumaAddr( 0 );
    UInt  uiSrcStride = m_ppcOrigYuv[uhDepth]->getStride();
    m_pcRdCost->setRenModelData( m_ppcTempCU[uhDepth], 0, piSrc, uiSrcStride, uiWidth, uiHeight );
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
      pcSubBestPartCU->initSubCU( rpcBestCU, uiPartUnitIdx, uhNextDepth );           // clear sub partition datas or init.
      pcSubTempPartCU->initSubCU( rpcBestCU, uiPartUnitIdx, uhNextDepth );           // clear sub partition datas or init.

      if( ( pcSubBestPartCU->getCUPelX() < pcSubBestPartCU->getSlice()->getSPS()->getWidth() ) && ( pcSubBestPartCU->getCUPelY() < pcSubBestPartCU->getSlice()->getSPS()->getHeight() ) )
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
    }

    if( uhDepth == uhTextureModeDepth )
    {
      xAddMVISignallingBits( rpcTempCU );
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
    m_pcPredSearch->encodeResAndCalcRdInterCU( rpcTempCU, m_ppcOrigYuv[uhDepth], m_ppcPredYuvTemp[uhDepth], m_ppcResiYuvTemp[uhDepth], m_ppcResiYuvBest[uhDepth], m_ppcRecoYuvTemp[uhDepth], m_ppcResPredTmp [uhDepth], bSkipResidual );

    if( uhDepth == uhTextureModeDepth )
    {
      xAddMVISignallingBits( rpcTempCU );
    }
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

  xCheckBestMode( rpcBestCU, rpcTempCU, uhDepth );

#if HHI_VSO
  if( !bSplit && bRecursiveCall && m_pcRdCost->getUseRenModel() )
  {
    UInt  uiWidth     = rpcBestCU->getWidth ( 0 );
    UInt  uiHeight    = rpcBestCU->getHeight( 0 );
    Pel*  piSrc       = m_ppcRecoYuvBest[uhDepth]->getLumaAddr( 0 );
    UInt  uiSrcStride = m_ppcRecoYuvBest[uhDepth]->getStride();
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
  pcCU->setMergeIndexSubParts( 0, 0, 0, uhDepth );

  {
    TComMvField  cMvFieldNeighbours[MRG_MAX_NUM_CANDS << 1]; // double length for mv of both lists
    UChar uhInterDirNeighbours[MRG_MAX_NUM_CANDS];
    UInt uiNeighbourCandIdx[MRG_MAX_NUM_CANDS]; //MVs with same idx => same cand

    for( UInt ui = 0; ui < MRG_MAX_NUM_CANDS; ++ui )
    {
      uhInterDirNeighbours[ui] = 0;
      uiNeighbourCandIdx[ui] = 0;
    }
    pcCU->getInterMergeCandidates( 0, 0, uhDepth, cMvFieldNeighbours,uhInterDirNeighbours, uiNeighbourCandIdx );
    for( UInt uiMergeCand = 0; uiMergeCand < MRG_MAX_NUM_CANDS; uiMergeCand++ )
    {
      pcCU->setNeighbourCandIdxSubParts( uiMergeCand, uiNeighbourCandIdx[uiMergeCand], 0, 0,uhDepth );
    }
  }

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
#if HHI_MRG_SKIP
    m_pcEntropyCoder->encodeMergeIndex( pcCU, 0, 0, true );
#else
    if ( pcCU->getSlice()->getNumRefIdx( REF_PIC_LIST_0 ) > 0 ) //if ( ref. frame list0 has at least 1 entry )
    {
      m_pcEntropyCoder->encodeMVPIdx( pcCU, 0, REF_PIC_LIST_0, true );
    }
    if ( pcCU->getSlice()->getNumRefIdx( REF_PIC_LIST_1 ) > 0 ) //if ( ref. frame list1 has at least 1 entry )
    {
      m_pcEntropyCoder->encodeMVPIdx( pcCU, 0, REF_PIC_LIST_1, true );
    }
#endif
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
