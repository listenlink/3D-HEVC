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

/** \file     TEncCu.cpp
    \brief    Coding Unit (CU) encoder class
*/

#include <stdio.h>
#include "TEncTop.h"
#include "TEncCu.h"
#include "TEncAnalyze.h"
#include "TLibCommon/Debug.h"

#include <cmath>
#include <algorithm>
using namespace std;


//! \ingroup TLibEncoder
//! \{

// ====================================================================================================================
// Constructor / destructor / create / destroy
// ====================================================================================================================

/**
 \param    uhTotalDepth  total number of allowable depth
 \param    uiMaxWidth    largest CU width
 \param    uiMaxHeight   largest CU height
 \param    chromaFormat  chroma format
 */
Void TEncCu::create(UChar uhTotalDepth, UInt uiMaxWidth, UInt uiMaxHeight, ChromaFormat chromaFormat)
{
  Int i;

  m_uhTotalDepth   = uhTotalDepth + 1;
  m_ppcBestCU      = new TComDataCU*[m_uhTotalDepth-1];
  m_ppcTempCU      = new TComDataCU*[m_uhTotalDepth-1];

#if NH_3D_ARP
  m_ppcWeightedTempCU = new TComDataCU*[m_uhTotalDepth-1];
#endif 

  m_ppcPredYuvBest = new TComYuv*[m_uhTotalDepth-1];
  m_ppcResiYuvBest = new TComYuv*[m_uhTotalDepth-1];
  m_ppcRecoYuvBest = new TComYuv*[m_uhTotalDepth-1];
  m_ppcPredYuvTemp = new TComYuv*[m_uhTotalDepth-1];
  m_ppcResiYuvTemp = new TComYuv*[m_uhTotalDepth-1];
  m_ppcRecoYuvTemp = new TComYuv*[m_uhTotalDepth-1];
  m_ppcOrigYuv     = new TComYuv*[m_uhTotalDepth-1];
#if NH_3D_DBBP
  m_ppcOrigYuvDBBP = new TComYuv*[m_uhTotalDepth-1];
#endif

  UInt uiNumPartitions;
  for( i=0 ; i<m_uhTotalDepth-1 ; i++)
  {
    uiNumPartitions = 1<<( ( m_uhTotalDepth - i - 1 )<<1 );
    UInt uiWidth  = uiMaxWidth  >> i;
    UInt uiHeight = uiMaxHeight >> i;

    m_ppcBestCU[i] = new TComDataCU; m_ppcBestCU[i]->create( chromaFormat, uiNumPartitions, uiWidth, uiHeight, false, uiMaxWidth >> (m_uhTotalDepth - 1) );
    m_ppcTempCU[i] = new TComDataCU; m_ppcTempCU[i]->create( chromaFormat, uiNumPartitions, uiWidth, uiHeight, false, uiMaxWidth >> (m_uhTotalDepth - 1) );
#if NH_3D_ARP
    m_ppcWeightedTempCU[i] = new TComDataCU; m_ppcWeightedTempCU[i]->create( chromaFormat, uiNumPartitions, uiWidth, uiHeight, false, uiMaxWidth >> (m_uhTotalDepth - 1) );
#endif  

    m_ppcPredYuvBest[i] = new TComYuv; m_ppcPredYuvBest[i]->create(uiWidth, uiHeight, chromaFormat);
    m_ppcResiYuvBest[i] = new TComYuv; m_ppcResiYuvBest[i]->create(uiWidth, uiHeight, chromaFormat);
    m_ppcRecoYuvBest[i] = new TComYuv; m_ppcRecoYuvBest[i]->create(uiWidth, uiHeight, chromaFormat);

    m_ppcPredYuvTemp[i] = new TComYuv; m_ppcPredYuvTemp[i]->create(uiWidth, uiHeight, chromaFormat);
    m_ppcResiYuvTemp[i] = new TComYuv; m_ppcResiYuvTemp[i]->create(uiWidth, uiHeight, chromaFormat);
    m_ppcRecoYuvTemp[i] = new TComYuv; m_ppcRecoYuvTemp[i]->create(uiWidth, uiHeight, chromaFormat);

    m_ppcOrigYuv    [i] = new TComYuv; m_ppcOrigYuv    [i]->create(uiWidth, uiHeight, chromaFormat);
#if NH_3D_DBBP
    m_ppcOrigYuvDBBP[i] = new TComYuv; m_ppcOrigYuvDBBP[i]->create(uiWidth, uiHeight, chromaFormat);
#endif

  }

  m_bEncodeDQP          = false;

#if KWU_RC_MADPRED_E0227
  m_LCUPredictionSAD = 0;
  m_addSADDepth      = 0;
  m_temporalSAD      = 0;
  m_spatialSAD       = 0;
#endif

  m_stillToCodeChromaQpOffsetFlag  = false;
  m_cuChromaQpOffsetIdxPlus1       = 0;

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
#if NH_3D_ARP
    if(m_ppcWeightedTempCU[i])
    {
      m_ppcWeightedTempCU[i]->destroy(); delete m_ppcWeightedTempCU[i]; m_ppcWeightedTempCU[i] = NULL;
    }
#endif
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
#if NH_3D_DBBP
    if(m_ppcOrigYuvDBBP[i])
    {
      m_ppcOrigYuvDBBP[i]->destroy(); delete m_ppcOrigYuvDBBP[i]; m_ppcOrigYuvDBBP[i] = NULL;
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

#if NH_3D_ARP
  if(m_ppcWeightedTempCU)
  {
    delete [] m_ppcWeightedTempCU; 
    m_ppcWeightedTempCU = NULL; 
  }
#endif
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
#if NH_3D_DBBP
  if(m_ppcOrigYuvDBBP)
  {
    delete [] m_ppcOrigYuvDBBP;
    m_ppcOrigYuvDBBP = NULL;
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
  m_pcRdCost           = pcEncTop->getRdCost();

  m_pcEntropyCoder     = pcEncTop->getEntropyCoder();
  m_pcBinCABAC         = pcEncTop->getBinCABAC();

  m_pppcRDSbacCoder    = pcEncTop->getRDSbacCoder();
  m_pcRDGoOnSbacCoder  = pcEncTop->getRDGoOnSbacCoder();

  m_pcRateCtrl         = pcEncTop->getRateCtrl();
}

// ====================================================================================================================
// Public member functions
// ====================================================================================================================

/** 
 \param  pCtu pointer of CU data class
 */
Void TEncCu::compressCtu( TComDataCU* pCtu )
{
  // initialize CU data
  m_ppcBestCU[0]->initCtu( pCtu->getPic(), pCtu->getCtuRsAddr() );
  m_ppcTempCU[0]->initCtu( pCtu->getPic(), pCtu->getCtuRsAddr() );

#if NH_3D_ARP
  m_ppcWeightedTempCU[0]->initCtu( pCtu->getPic(), pCtu->getCtuRsAddr() );
#endif

#if KWU_RC_MADPRED_E0227
  m_LCUPredictionSAD = 0;
  m_addSADDepth      = 0;
  m_temporalSAD      = 0;
  m_spatialSAD       = 0;
#endif

  // analysis of CU
  DEBUG_STRING_NEW(sDebug)

  xCompressCU( m_ppcBestCU[0], m_ppcTempCU[0], 0 DEBUG_STRING_PASS_INTO(sDebug) );
  DEBUG_STRING_OUTPUT(std::cout, sDebug)

#if ADAPTIVE_QP_SELECTION
  if( m_pcEncCfg->getUseAdaptQpSelect() )
  {
    if(pCtu->getSlice()->getSliceType()!=I_SLICE) //IIII
    {
      xCtuCollectARLStats( pCtu );
    }
  }
#endif
}
/** \param  pCtu  pointer of CU data class
 */
Void TEncCu::encodeCtu ( TComDataCU* pCtu )
{
  if ( pCtu->getSlice()->getPPS()->getUseDQP() )
  {
    setdQPFlag(true);
  }

  if ( pCtu->getSlice()->getUseChromaQpAdj() )
  {
    setCodeChromaQpAdjFlag(true);
  }

  // Encode CU data
  xEncodeCU( pCtu, 0, 0 );
}

// ====================================================================================================================
// Protected member functions
// ====================================================================================================================
//! Derive small set of test modes for AMP encoder speed-up
#if AMP_ENC_SPEEDUP
#if AMP_MRG
Void TEncCu::deriveTestModeAMP (TComDataCU *pcBestCU, PartSize eParentPartSize, Bool &bTestAMP_Hor, Bool &bTestAMP_Ver, Bool &bTestMergeAMP_Hor, Bool &bTestMergeAMP_Ver)
#else
Void TEncCu::deriveTestModeAMP (TComDataCU *pcBestCU, PartSize eParentPartSize, Bool &bTestAMP_Hor, Bool &bTestAMP_Ver)
#endif
{
  if ( pcBestCU->getPartitionSize(0) == SIZE_2NxN )
  {
    bTestAMP_Hor = true;
  }
  else if ( pcBestCU->getPartitionSize(0) == SIZE_Nx2N )
  {
    bTestAMP_Ver = true;
  }
  else if ( pcBestCU->getPartitionSize(0) == SIZE_2Nx2N && pcBestCU->getMergeFlag(0) == false && pcBestCU->isSkipped(0) == false )
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

  if ( eParentPartSize == NUMBER_OF_PART_SIZES ) //! if parent is intra
  {
    if ( pcBestCU->getPartitionSize(0) == SIZE_2NxN )
    {
      bTestMergeAMP_Hor = true;
    }
    else if ( pcBestCU->getPartitionSize(0) == SIZE_Nx2N )
    {
      bTestMergeAMP_Ver = true;
    }
  }

  if ( pcBestCU->getPartitionSize(0) == SIZE_2Nx2N && pcBestCU->isSkipped(0) == false )
  {
    bTestMergeAMP_Hor = true;
    bTestMergeAMP_Ver = true;
  }

  if ( pcBestCU->getWidth(0) == 64 )
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
/** Compress a CU block recursively with enabling sub-CTU-level delta QP
 *  - for loop of QP value to compress the current CU with all possible QP
*/
#if AMP_ENC_SPEEDUP
Void TEncCu::xCompressCU( TComDataCU*& rpcBestCU, TComDataCU*& rpcTempCU, UInt uiDepth DEBUG_STRING_FN_DECLARE(sDebug_), PartSize eParentPartSize )
#else
Void TEncCu::xCompressCU( TComDataCU*& rpcBestCU, TComDataCU*& rpcTempCU, UInt uiDepth )
#endif
{
  TComPic* pcPic = rpcBestCU->getPic();
  DEBUG_STRING_NEW(sDebug)
  const TComPPS &pps=*(rpcTempCU->getSlice()->getPPS());
  const TComSPS &sps=*(rpcTempCU->getSlice()->getSPS());


#if H_3D_QTLPC
  Bool  bLimQtPredFalg    = pcPic->getSlice(0)->getQtPredFlag(); 
  TComPic *pcTexture      = rpcBestCU->getSlice()->getTexturePic();

  Bool  depthMapDetect    = (pcTexture != NULL);
  Bool  bIntraSliceDetect = (rpcBestCU->getSlice()->getSliceType() == I_SLICE);

  Bool rapPic             = (rpcBestCU->getSlice()->getNalUnitType() == NAL_UNIT_CODED_SLICE_IDR_W_RADL || rpcBestCU->getSlice()->getNalUnitType() == NAL_UNIT_CODED_SLICE_IDR_N_LP || rpcBestCU->getSlice()->getNalUnitType() == NAL_UNIT_CODED_SLICE_CRA);

  Bool bTry2NxN           = true;
  Bool bTryNx2N           = true;
#endif

  // get Original YUV data from picture
  m_ppcOrigYuv[uiDepth]->copyFromPicYuv( pcPic->getPicYuvOrg(), rpcBestCU->getCtuRsAddr(), rpcBestCU->getZorderIdxInCtu() );

#if H_3D_QTLPC  
  Bool    bTrySplit     = true;
  Bool    bTrySplitDQP  = true;
#endif

    // variable for Early CU determination
  Bool    bSubBranch = true;

  // variable for Cbf fast mode PU decision
  Bool    doNotBlockPu = true;
  Bool    earlyDetectionSkipMode = false;

#if NH_3D_NBDV
  DisInfo DvInfo; 
  DvInfo.m_acNBDV.setZero();
  DvInfo.m_aVIdxCan = 0;
#if NH_3D_NBDV_REF
  DvInfo.m_acDoNBDV.setZero();
#endif
#endif
  Bool bBoundary = false;
  UInt uiLPelX   = rpcBestCU->getCUPelX();
  UInt uiRPelX   = uiLPelX + rpcBestCU->getWidth(0)  - 1;
  UInt uiTPelY   = rpcBestCU->getCUPelY();
  UInt uiBPelY   = uiTPelY + rpcBestCU->getHeight(0) - 1;

#if H_MV_ENC_DEC_TRAC
#if ENC_DEC_TRACE
    stopAtPos  ( rpcBestCU->getSlice()->getPOC(), 
                 rpcBestCU->getSlice()->getLayerId(), 
                 rpcBestCU->getCUPelX(),
                 rpcBestCU->getCUPelY(),
                 rpcBestCU->getWidth(0), 
                 rpcBestCU->getHeight(0) );
#endif
#endif

  Int iBaseQP = xComputeQP( rpcBestCU, uiDepth );
  Int iMinQP;
  Int iMaxQP;
  Bool isAddLowestQP = false;

  const UInt numberValidComponents = rpcBestCU->getPic()->getNumberValidComponents();

  if( uiDepth <= pps.getMaxCuDQPDepth() )
  {
    Int idQP = m_pcEncCfg->getMaxDeltaQP();
    iMinQP = Clip3( -sps.getQpBDOffset(CHANNEL_TYPE_LUMA), MAX_QP, iBaseQP-idQP );
    iMaxQP = Clip3( -sps.getQpBDOffset(CHANNEL_TYPE_LUMA), MAX_QP, iBaseQP+idQP );
  }
  else
  {
    iMinQP = rpcTempCU->getQP(0);
    iMaxQP = rpcTempCU->getQP(0);
  }

  if ( m_pcEncCfg->getUseRateCtrl() )
  {
    iMinQP = m_pcRateCtrl->getRCQP();
    iMaxQP = m_pcRateCtrl->getRCQP();
  }

  // transquant-bypass (TQB) processing loop variable initialisation ---

  const Int lowestQP = iMinQP; // For TQB, use this QP which is the lowest non TQB QP tested (rather than QP'=0) - that way delta QPs are smaller, and TQB can be tested at all CU levels.

  if ( (pps.getTransquantBypassEnableFlag()) )
  {
    isAddLowestQP = true; // mark that the first iteration is to cost TQB mode.
    iMinQP = iMinQP - 1;  // increase loop variable range by 1, to allow testing of TQB mode along with other QPs
    if ( m_pcEncCfg->getCUTransquantBypassFlagForceValue() )
    {
      iMaxQP = iMinQP;
    }
  }

#if H_3D_IC
  Bool bICEnabled = rpcTempCU->getSlice()->getViewIndex() && ( rpcTempCU->getSlice()->getSliceType() == P_SLICE || rpcTempCU->getSlice()->getSliceType() == B_SLICE ) && !rpcTempCU->getSlice()->getIsDepth();
  bICEnabled = bICEnabled && rpcTempCU->getSlice()->getApplyIC();
#endif

  TComSlice * pcSlice = rpcTempCU->getPic()->getSlice(rpcTempCU->getPic()->getCurrSliceIdx());
  // We need to split, so don't try these modes.
  if ( ( uiRPelX < sps.getPicWidthInLumaSamples() ) &&
       ( uiBPelY < sps.getPicHeightInLumaSamples() ) )
  {
#if  H_3D_FAST_TEXTURE_ENCODING
    Bool bIVFMerge = false;
    Int  iIVFMaxD = 0;
    Bool bFMD = false;
#endif
    for (Int iQP=iMinQP; iQP<=iMaxQP; iQP++)
    {
      const Bool bIsLosslessMode = isAddLowestQP && (iQP == iMinQP);

      if (bIsLosslessMode)
      {
        iQP = lowestQP;
      }

#if H_3D_QTLPC
      bTrySplit    = true;
#endif

      m_cuChromaQpOffsetIdxPlus1 = 0;
      if (pcSlice->getUseChromaQpAdj())
      {
        /* Pre-estimation of chroma QP based on input block activity may be performed
         * here, using for example m_ppcOrigYuv[uiDepth] */
        /* To exercise the current code, the index used for adjustment is based on
         * block position
         */
        Int lgMinCuSize = sps.getLog2MinCodingBlockSize() +
                          std::max<Int>(0, sps.getLog2DiffMaxMinCodingBlockSize()-Int(pps.getPpsRangeExtension().getDiffCuChromaQpOffsetDepth()));
        m_cuChromaQpOffsetIdxPlus1 = ((uiLPelX >> lgMinCuSize) + (uiTPelY >> lgMinCuSize)) % (pps.getPpsRangeExtension().getChromaQpOffsetListLen() + 1);
      }

      rpcTempCU->initEstData( uiDepth, iQP, bIsLosslessMode );
#if H_3D_QTLPC
      //logic for setting bTrySplit using the partition information that is stored of the texture colocated CU
#if H_3D_FCO
      if(depthMapDetect && !bIntraSliceDetect && !rapPic && ( m_pcEncCfg->getUseQTL() || bLimQtPredFalg ) && pcTexture->getReconMark())
#else
      if(depthMapDetect && !bIntraSliceDetect && !rapPic && ( m_pcEncCfg->getUseQTL() || bLimQtPredFalg ))
#endif
      {
        TComDataCU* pcTextureCU = pcTexture->getCU( rpcBestCU->getAddr() ); //Corresponding texture LCU
        UInt uiCUIdx            = rpcBestCU->getZorderIdxInCU();
        assert(pcTextureCU->getDepth(uiCUIdx) >= uiDepth); //Depth cannot be more partitioned than the texture.
        if (pcTextureCU->getDepth(uiCUIdx) > uiDepth || pcTextureCU->getPartitionSize(uiCUIdx) == SIZE_NxN) //Texture was split.
        {
          bTrySplit = true;
          bTryNx2N  = true;
          bTry2NxN  = true;
        }
        else
        {
          bTrySplit = false;
          bTryNx2N  = false;
          bTry2NxN  = false;
          if( pcTextureCU->getDepth(uiCUIdx) == uiDepth && pcTextureCU->getPartitionSize(uiCUIdx) != SIZE_2Nx2N)
          {
            if(pcTextureCU->getPartitionSize(uiCUIdx)==SIZE_2NxN || pcTextureCU->getPartitionSize(uiCUIdx)==SIZE_2NxnU|| pcTextureCU->getPartitionSize(uiCUIdx)==SIZE_2NxnD)
            {
              bTry2NxN  = true;
            }
            else
            {
              bTryNx2N  = true;
            }
          }
        }
      }
#endif

#if NH_3D_NBDV
      if( rpcTempCU->getSlice()->getSliceType() != I_SLICE )
      {
#if NH_3D_ARP && H_3D_IV_MERGE
        if( rpcTempCU->getSlice()->getIvResPredFlag() || rpcTempCU->getSlice()->getIvMvPredFlag() )
#else 
#if NH_3D_ARP
        if( rpcTempCU->getSlice()->getIvResPredFlag() )
#else
#if H_3D_IV_MERGE
        if( rpcTempCU->getSlice()->getVPS()->getIvMvPredFlag(rpcTempCU->getSlice()->getLayerId()) )
#else
#if NH_3D_DBBP
        if( rpcTempCU->getSlice()->getDepthBasedBlkPartFlag() )
#else
        if (0)
#endif
#endif
#endif
#endif
        {
          PartSize ePartTemp = rpcTempCU->getPartitionSize(0);
          rpcTempCU->setPartSizeSubParts(SIZE_2Nx2N, 0, uiDepth);
#if H_3D_IV_MERGE
          if (rpcTempCU->getSlice()->getIsDepth() )
          {
            rpcTempCU->getDispforDepth(0, 0, &DvInfo);
          }
          else
          {
#endif 
#if NH_3D_NBDV_REF
            if( rpcTempCU->getSlice()->getDepthRefinementFlag() )
            {
              rpcTempCU->getDisMvpCandNBDV(&DvInfo, true);
            }
            else
#endif 
            {
              rpcTempCU->getDisMvpCandNBDV(&DvInfo);
            }
#if H_3D_IV_MERGE
          }
#endif
          rpcTempCU->setDvInfoSubParts(DvInfo, 0, uiDepth);
          rpcBestCU->setDvInfoSubParts(DvInfo, 0, uiDepth);
          rpcTempCU->setPartSizeSubParts( ePartTemp, 0, uiDepth );
        }
      }
#if  H_3D_FAST_TEXTURE_ENCODING
      if(rpcTempCU->getSlice()->getViewIndex() && !rpcTempCU->getSlice()->getIsDepth() && rpcTempCU->getSlice()->getDefaultRefViewIdxAvailableFlag() )
      {
        PartSize ePartTemp = rpcTempCU->getPartitionSize(0);
        rpcTempCU->setPartSizeSubParts( SIZE_2Nx2N, 0, uiDepth ); 
        rpcTempCU->getIVNStatus( 0, &DvInfo,  bIVFMerge, iIVFMaxD);
        rpcTempCU->setPartSizeSubParts( ePartTemp, 0, uiDepth );
      }
#endif
#endif
      // do inter modes, SKIP and 2Nx2N
      if( rpcBestCU->getSlice()->getSliceType() != I_SLICE )
      {
#if H_3D_IC
        for( UInt uiICId = 0; uiICId < ( bICEnabled ? 2 : 1 ); uiICId++ )
        {
          Bool bICFlag = uiICId ? true : false;
#endif
        // 2Nx2N
        if(m_pcEncCfg->getUseEarlySkipDetection())
        {
#if H_3D_IC
            rpcTempCU->setICFlagSubParts(bICFlag, 0, 0, uiDepth);
#endif
#if  H_3D_FAST_TEXTURE_ENCODING
          xCheckRDCostInter( rpcBestCU, rpcTempCU, SIZE_2Nx2N, bFMD );  rpcTempCU->initEstData( uiDepth, iQP, bIsLosslessMode  );//by Competition for inter_2Nx2N
#else
          xCheckRDCostInter( rpcBestCU, rpcTempCU, SIZE_2Nx2N DEBUG_STRING_PASS_INTO(sDebug) );
          rpcTempCU->initEstData( uiDepth, iQP, bIsLosslessMode );//by Competition for inter_2Nx2N
#endif
#if H_3D_VSP || NH_3D_DBBP
            rpcTempCU->setDvInfoSubParts(DvInfo, 0, uiDepth);
#endif
        }
        // SKIP
#if H_3D_IC
          rpcTempCU->setICFlagSubParts(bICFlag, 0, 0, uiDepth);
#endif
        xCheckRDCostMerge2Nx2N( rpcBestCU, rpcTempCU DEBUG_STRING_PASS_INTO(sDebug), &earlyDetectionSkipMode );//by Merge for inter_2Nx2N
#if  H_3D_FAST_TEXTURE_ENCODING
          bFMD = bIVFMerge && rpcBestCU->isSkipped(0);
#endif

        rpcTempCU->initEstData( uiDepth, iQP, bIsLosslessMode );
#if H_3D_VSP || NH_3D_DBBP
          rpcTempCU->setDvInfoSubParts(DvInfo, 0, uiDepth);
#endif

        if(!m_pcEncCfg->getUseEarlySkipDetection())
        {
          // 2Nx2N, NxN
#if H_3D_IC
            rpcTempCU->setICFlagSubParts(bICFlag, 0, 0, uiDepth);
#endif
#if  H_3D_FAST_TEXTURE_ENCODING
            xCheckRDCostInter( rpcBestCU, rpcTempCU, SIZE_2Nx2N, bFMD );  rpcTempCU->initEstData( uiDepth, iQP, bIsLosslessMode );
#else

          xCheckRDCostInter( rpcBestCU, rpcTempCU, SIZE_2Nx2N DEBUG_STRING_PASS_INTO(sDebug) );
          rpcTempCU->initEstData( uiDepth, iQP, bIsLosslessMode );
#endif
#if H_3D_VSP || NH_3D_DBBP
            rpcTempCU->setDvInfoSubParts(DvInfo, 0, uiDepth);
#endif
#if NH_3D_DBBP
            if( rpcTempCU->getSlice()->getDepthBasedBlkPartFlag() && rpcTempCU->getSlice()->getDefaultRefViewIdxAvailableFlag() )
            {
              xCheckRDCostInterDBBP( rpcBestCU, rpcTempCU, false );
              rpcTempCU->initEstData( uiDepth, iQP, bIsLosslessMode  );
#if H_3D_VSP || NH_3D_DBBP
              rpcTempCU->setDvInfoSubParts(DvInfo, 0, uiDepth);
#endif
            }
#endif

          if(m_pcEncCfg->getUseCbfFastMode())
          {
            doNotBlockPu = rpcBestCU->getQtRootCbf( 0 ) != 0;
          }
        }
#if H_3D_IC
        }
#endif
      }
#if H_3D_QTLPC      
      if(depthMapDetect && !bIntraSliceDetect && !rapPic && ( m_pcEncCfg->getUseQTL() || bLimQtPredFalg ))
      {
        bTrySplitDQP = bTrySplit;
      }
#endif

      if (bIsLosslessMode) // Restore loop variable if lossless mode was searched.
      {
        iQP = iMinQP;
      }
    }

#if KWU_RC_MADPRED_E0227
    if ( uiDepth <= m_addSADDepth )
    {
      m_LCUPredictionSAD += m_temporalSAD;
      m_addSADDepth = uiDepth;
    }
#endif
#if H_3D_DIM_ENC
    if( rpcBestCU->getSlice()->getIsDepth() && rpcBestCU->getSlice()->isIRAP() )
    {
      earlyDetectionSkipMode = false;
    }
#endif
#if H_3D
    rpcTempCU->initEstData( uiDepth, iMinQP, isAddLowestQP  );
    if( rpcBestCU->getSlice()->getDepthIntraSkipFlag() )
    {
      xCheckRDCostDIS( rpcBestCU, rpcTempCU, SIZE_2Nx2N );
      rpcTempCU->initEstData( uiDepth, iMinQP, isAddLowestQP  );
    }
#endif
    if(!earlyDetectionSkipMode)
    {
      for (Int iQP=iMinQP; iQP<=iMaxQP; iQP++)
      {
        const Bool bIsLosslessMode = isAddLowestQP && (iQP == iMinQP); // If lossless, then iQP is irrelevant for subsequent modules.

        if (bIsLosslessMode)
        {
          iQP = lowestQP;
        }

        rpcTempCU->initEstData( uiDepth, iQP, bIsLosslessMode );

        // do inter modes, NxN, 2NxN, and Nx2N
        if( rpcBestCU->getSlice()->getSliceType() != I_SLICE )
        {
          // 2Nx2N, NxN

          if(!( (rpcBestCU->getWidth(0)==8) && (rpcBestCU->getHeight(0)==8) ))
          {
            if( uiDepth == sps.getLog2DiffMaxMinCodingBlockSize() && doNotBlockPu
#if H_3D_QTLPC
                && bTrySplit
#endif
)
            {
#if  H_3D_FAST_TEXTURE_ENCODING
                xCheckRDCostInter( rpcBestCU, rpcTempCU, SIZE_NxN, bFMD  );
#else

              xCheckRDCostInter( rpcBestCU, rpcTempCU, SIZE_NxN DEBUG_STRING_PASS_INTO(sDebug)   );
#endif
              rpcTempCU->initEstData( uiDepth, iQP, bIsLosslessMode );
#if H_3D_VSP || NH_3D_DBBP
                rpcTempCU->setDvInfoSubParts(DvInfo, 0, uiDepth);
#endif

            }
          }

          if(doNotBlockPu
#if H_3D_QTLPC
            && bTryNx2N
#endif
)
          {
#if  H_3D_FAST_TEXTURE_ENCODING
            xCheckRDCostInter( rpcBestCU, rpcTempCU, SIZE_Nx2N, bFMD  );
#else
            xCheckRDCostInter( rpcBestCU, rpcTempCU, SIZE_Nx2N DEBUG_STRING_PASS_INTO(sDebug)  );
#endif
            rpcTempCU->initEstData( uiDepth, iQP, bIsLosslessMode );
#if H_3D_VSP || NH_3D_DBBP
            rpcTempCU->setDvInfoSubParts(DvInfo, 0, uiDepth);
#endif

            if(m_pcEncCfg->getUseCbfFastMode() && rpcBestCU->getPartitionSize(0) == SIZE_Nx2N )
            {
              doNotBlockPu = rpcBestCU->getQtRootCbf( 0 ) != 0;
            }
          }
          if(doNotBlockPu
#if H_3D_QTLPC
            && bTry2NxN
#endif
)
          {
#if  H_3D_FAST_TEXTURE_ENCODING
            xCheckRDCostInter      ( rpcBestCU, rpcTempCU, SIZE_2NxN, bFMD  );
#else

            xCheckRDCostInter      ( rpcBestCU, rpcTempCU, SIZE_2NxN DEBUG_STRING_PASS_INTO(sDebug)  );
#endif

            rpcTempCU->initEstData( uiDepth, iQP, bIsLosslessMode );
#if H_3D_VSP || NH_3D_DBBP
            rpcTempCU->setDvInfoSubParts(DvInfo, 0, uiDepth);
#endif

            if(m_pcEncCfg->getUseCbfFastMode() && rpcBestCU->getPartitionSize(0) == SIZE_2NxN)
            {
              doNotBlockPu = rpcBestCU->getQtRootCbf( 0 ) != 0;
            }
          }

          //! Try AMP (SIZE_2NxnU, SIZE_2NxnD, SIZE_nLx2N, SIZE_nRx2N)
          if(sps.getUseAMP() && uiDepth < sps.getLog2DiffMaxMinCodingBlockSize() )
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
              if(doNotBlockPu
#if H_3D_QTLPC
                && bTry2NxN
#endif
)
              {
#if  H_3D_FAST_TEXTURE_ENCODING
                xCheckRDCostInter( rpcBestCU, rpcTempCU, SIZE_2NxnU, bFMD );
#else
                xCheckRDCostInter( rpcBestCU, rpcTempCU, SIZE_2NxnU DEBUG_STRING_PASS_INTO(sDebug) );
#endif
                rpcTempCU->initEstData( uiDepth, iQP, bIsLosslessMode );
#if H_3D_VSP || NH_3D_DBBP
                rpcTempCU->setDvInfoSubParts(DvInfo, 0, uiDepth);
#endif
                if(m_pcEncCfg->getUseCbfFastMode() && rpcBestCU->getPartitionSize(0) == SIZE_2NxnU )
                {
                  doNotBlockPu = rpcBestCU->getQtRootCbf( 0 ) != 0;
                }
              }
              if(doNotBlockPu
#if H_3D_QTLPC
                && bTry2NxN
#endif
)
              {
#if  H_3D_FAST_TEXTURE_ENCODING
                xCheckRDCostInter( rpcBestCU, rpcTempCU, SIZE_2NxnD, bFMD );
#else
                xCheckRDCostInter( rpcBestCU, rpcTempCU, SIZE_2NxnD DEBUG_STRING_PASS_INTO(sDebug) );
#endif

                rpcTempCU->initEstData( uiDepth, iQP, bIsLosslessMode );
#if H_3D_VSP || NH_3D_DBBP
                rpcTempCU->setDvInfoSubParts(DvInfo, 0, uiDepth);
#endif

                if(m_pcEncCfg->getUseCbfFastMode() && rpcBestCU->getPartitionSize(0) == SIZE_2NxnD )
                {
                  doNotBlockPu = rpcBestCU->getQtRootCbf( 0 ) != 0;
                }
              }
            }
#if AMP_MRG
            else if ( bTestMergeAMP_Hor )
            {
              if(doNotBlockPu
#if H_3D_QTLPC
                && bTry2NxN
#endif
)
              {
#if  H_3D_FAST_TEXTURE_ENCODING
                xCheckRDCostInter( rpcBestCU, rpcTempCU, SIZE_2NxnU, bFMD, true );
#else

                xCheckRDCostInter( rpcBestCU, rpcTempCU, SIZE_2NxnU DEBUG_STRING_PASS_INTO(sDebug), true );
#endif

                rpcTempCU->initEstData( uiDepth, iQP, bIsLosslessMode );
#if H_3D_VSP || NH_3D_DBBP
                rpcTempCU->setDvInfoSubParts(DvInfo, 0, uiDepth);
#endif
                if(m_pcEncCfg->getUseCbfFastMode() && rpcBestCU->getPartitionSize(0) == SIZE_2NxnU )
                {
                  doNotBlockPu = rpcBestCU->getQtRootCbf( 0 ) != 0;
                }
              }
              if(doNotBlockPu
#if H_3D_QTLPC
                && bTry2NxN
#endif
)
              {
#if  H_3D_FAST_TEXTURE_ENCODING
                xCheckRDCostInter( rpcBestCU, rpcTempCU, SIZE_2NxnD, bFMD, true );
#else
                xCheckRDCostInter( rpcBestCU, rpcTempCU, SIZE_2NxnD DEBUG_STRING_PASS_INTO(sDebug), true );
#endif
                rpcTempCU->initEstData( uiDepth, iQP, bIsLosslessMode );
#if H_3D_VSP || NH_3D_DBBP
                rpcTempCU->setDvInfoSubParts(DvInfo, 0, uiDepth);
#endif

                if(m_pcEncCfg->getUseCbfFastMode() && rpcBestCU->getPartitionSize(0) == SIZE_2NxnD )
                {
                  doNotBlockPu = rpcBestCU->getQtRootCbf( 0 ) != 0;
                }
              }
            }
#endif

            //! Do horizontal AMP
            if ( bTestAMP_Ver )
            {
              if(doNotBlockPu
#if H_3D_QTLPC
                && bTryNx2N
#endif
)
              {
#if  H_3D_FAST_TEXTURE_ENCODING
                xCheckRDCostInter( rpcBestCU, rpcTempCU, SIZE_nLx2N, bFMD );
#else
                xCheckRDCostInter( rpcBestCU, rpcTempCU, SIZE_nLx2N DEBUG_STRING_PASS_INTO(sDebug) );
#endif

                rpcTempCU->initEstData( uiDepth, iQP, bIsLosslessMode );
#if H_3D_VSP || NH_3D_DBBP
                rpcTempCU->setDvInfoSubParts(DvInfo, 0, uiDepth);
#endif
                if(m_pcEncCfg->getUseCbfFastMode() && rpcBestCU->getPartitionSize(0) == SIZE_nLx2N )
                {
                  doNotBlockPu = rpcBestCU->getQtRootCbf( 0 ) != 0;
                }
              }
              if(doNotBlockPu
#if H_3D_QTLPC
                && bTryNx2N
#endif
)
              {
#if  H_3D_FAST_TEXTURE_ENCODING
                xCheckRDCostInter( rpcBestCU, rpcTempCU, SIZE_nRx2N, bFMD );
#else
                xCheckRDCostInter( rpcBestCU, rpcTempCU, SIZE_nRx2N DEBUG_STRING_PASS_INTO(sDebug) );
#endif
                rpcTempCU->initEstData( uiDepth, iQP, bIsLosslessMode );
#if H_3D_VSP || NH_3D_DBBP
                rpcTempCU->setDvInfoSubParts(DvInfo, 0, uiDepth);
#endif
              }
            }
#if AMP_MRG
            else if ( bTestMergeAMP_Ver )
            {
              if(doNotBlockPu
#if H_3D_QTLPC
                && bTryNx2N
#endif
)
              {
#if  H_3D_FAST_TEXTURE_ENCODING
                xCheckRDCostInter( rpcBestCU, rpcTempCU, SIZE_nLx2N, bFMD, true );
#else
                xCheckRDCostInter( rpcBestCU, rpcTempCU, SIZE_nLx2N DEBUG_STRING_PASS_INTO(sDebug), true );
#endif
                rpcTempCU->initEstData( uiDepth, iQP, bIsLosslessMode );
#if H_3D_VSP || NH_3D_DBBP
                rpcTempCU->setDvInfoSubParts(DvInfo, 0, uiDepth);
#endif
                if(m_pcEncCfg->getUseCbfFastMode() && rpcBestCU->getPartitionSize(0) == SIZE_nLx2N )
                {
                  doNotBlockPu = rpcBestCU->getQtRootCbf( 0 ) != 0;
                }
              }
              if(doNotBlockPu
#if H_3D_QTLPC
                && bTryNx2N
#endif
)
              {
#if  H_3D_FAST_TEXTURE_ENCODING
                xCheckRDCostInter( rpcBestCU, rpcTempCU, SIZE_nRx2N, bFMD, true );
#else

                xCheckRDCostInter( rpcBestCU, rpcTempCU, SIZE_nRx2N DEBUG_STRING_PASS_INTO(sDebug), true );
#endif
                rpcTempCU->initEstData( uiDepth, iQP, bIsLosslessMode );
#if H_3D_VSP || NH_3D_DBBP
                rpcTempCU->setDvInfoSubParts(DvInfo, 0, uiDepth);
#endif

              }
            }
#endif

#else
#if H_3D_QTLPC
            if (bTry2NxN)
            {
#endif

            xCheckRDCostInter( rpcBestCU, rpcTempCU, SIZE_2NxnU );
            rpcTempCU->initEstData( uiDepth, iQP, bIsLosslessMode );
#if H_3D_VSP || NH_3D_DBBP
              rpcTempCU->setDvInfoSubParts(DvInfo, 0, uiDepth);
#endif
            xCheckRDCostInter( rpcBestCU, rpcTempCU, SIZE_2NxnD );
            rpcTempCU->initEstData( uiDepth, iQP, bIsLosslessMode );
#if H_3D_VSP || NH_3D_DBBP
              rpcTempCU->setDvInfoSubParts(DvInfo, 0, uiDepth);
#endif
#if H_3D_QTLPC
            }
            if (bTryNx2N)
            {
#endif
            xCheckRDCostInter( rpcBestCU, rpcTempCU, SIZE_nLx2N );
            rpcTempCU->initEstData( uiDepth, iQP, bIsLosslessMode );
#if H_3D_VSP || NH_3D_DBBP
              rpcTempCU->setDvInfoSubParts(DvInfo, 0, uiDepth);
#endif
            xCheckRDCostInter( rpcBestCU, rpcTempCU, SIZE_nRx2N );
            rpcTempCU->initEstData( uiDepth, iQP, bIsLosslessMode );
#if H_3D_VSP || NH_3D_DBBP
              rpcTempCU->setDvInfoSubParts(DvInfo, 0, uiDepth);
#endif
#if H_3D_QTLPC
            }
#endif


#endif
          }
        }
#if  H_3D_FAST_TEXTURE_ENCODING
        if(!bFMD)
        {
#endif

        // do normal intra modes
        // speedup for inter frames
        Double intraCost = 0.0;

        if((rpcBestCU->getSlice()->getSliceType() == I_SLICE)                                     ||
            ((!m_pcEncCfg->getDisableIntraPUsInInterSlices()) && (
           (rpcBestCU->getCbf( 0, COMPONENT_Y  ) != 0)                                            ||
          ((rpcBestCU->getCbf( 0, COMPONENT_Cb ) != 0) && (numberValidComponents > COMPONENT_Cb)) ||
          ((rpcBestCU->getCbf( 0, COMPONENT_Cr ) != 0) && (numberValidComponents > COMPONENT_Cr))) 
 // avoid very complex intra if it is unlikely
#if H_3D_DIM_ENC
            || rpcBestCU->getSlice()->getIsDepth()
#endif
            ))
        {
#if H_3D_DIM
            Bool bOnlyIVP = false;
            Bool bUseIVP = true;
            if( rpcBestCU->getSlice()->getIsDepth() && !(rpcBestCU->getSlice()->isIRAP()) && 
              rpcBestCU->getSlice()->getSliceType() != I_SLICE && 
              rpcBestCU->getCbf( 0, TEXT_LUMA     ) == 0 &&
              rpcBestCU->getCbf( 0, TEXT_CHROMA_U ) == 0 &&
              rpcBestCU->getCbf( 0, TEXT_CHROMA_V ) == 0 
              )
            { 
              bOnlyIVP = true;
              bUseIVP = rpcBestCU->getSlice()->getIntraContourFlag();
            }
            if( bUseIVP )
            {
              xCheckRDCostIntra( rpcBestCU, rpcTempCU, SIZE_2Nx2N, bOnlyIVP );
#else
          xCheckRDCostIntra( rpcBestCU, rpcTempCU, intraCost, SIZE_2Nx2N DEBUG_STRING_PASS_INTO(sDebug) );
#endif
#if KWU_RC_MADPRED_E0227
            if ( uiDepth <= m_addSADDepth )
            {
              m_LCUPredictionSAD += m_spatialSAD;
              m_addSADDepth = uiDepth;
            }
#endif

          rpcTempCU->initEstData( uiDepth, iQP, bIsLosslessMode );
          if( uiDepth == sps.getLog2DiffMaxMinCodingBlockSize() )
          {
#if H_3D_QTLPC //Try IntraNxN
              if(bTrySplit)
              {
#endif
            if( rpcTempCU->getWidth(0) > ( 1 << sps.getQuadtreeTULog2MinSize() ) )
            {
              Double tmpIntraCost;
#if H_3D_DIM
              xCheckRDCostIntra( rpcBestCU, rpcTempCU, SIZE_NxN, bOnlyIVP );
#else
              xCheckRDCostIntra( rpcBestCU, rpcTempCU, tmpIntraCost, SIZE_NxN DEBUG_STRING_PASS_INTO(sDebug)   );
#endif

              intraCost = std::min(intraCost, tmpIntraCost);
              rpcTempCU->initEstData( uiDepth, iQP, bIsLosslessMode );
            }
#if H_3D_QTLPC
              }
#endif
          }
#if H_3D
          }
#endif
        }

        // test PCM
        if(sps.getUsePCM()
          && rpcTempCU->getWidth(0) <= (1<<sps.getPCMLog2MaxSize())
          && rpcTempCU->getWidth(0) >= (1<<sps.getPCMLog2MinSize()) )
        {
          UInt uiRawBits = getTotalBits(rpcBestCU->getWidth(0), rpcBestCU->getHeight(0), rpcBestCU->getPic()->getChromaFormat(), sps.getBitDepths().recon);
          UInt uiBestBits = rpcBestCU->getTotalBits();
#if NH_3D_VSO // M7
          Double dRDCostTemp = m_pcRdCost->getUseLambdaScaleVSO() ? m_pcRdCost->calcRdCostVSO(uiRawBits, 0) : m_pcRdCost->calcRdCost(uiRawBits, 0);
          if((uiBestBits > uiRawBits) || (rpcBestCU->getTotalCost() > dRDCostTemp ))
#else
          if((uiBestBits > uiRawBits) || (rpcBestCU->getTotalCost() > m_pcRdCost->calcRdCost(uiRawBits, 0)))
#endif

          {
            xCheckIntraPCM (rpcBestCU, rpcTempCU);
            rpcTempCU->initEstData( uiDepth, iQP, bIsLosslessMode );
          }
        }
#if  H_3D_FAST_TEXTURE_ENCODING
        }
#endif
        if (bIsLosslessMode) // Restore loop variable if lossless mode was searched.
        {
          iQP = iMinQP;
        }
      }
    }

    m_pcRDGoOnSbacCoder->load(m_pppcRDSbacCoder[uiDepth][CI_NEXT_BEST]);
    m_pcEntropyCoder->resetBits();
    m_pcEntropyCoder->encodeSplitFlag( rpcBestCU, 0, uiDepth, true );
    rpcBestCU->getTotalBits() += m_pcEntropyCoder->getNumberOfWrittenBits(); // split bits
    rpcBestCU->getTotalBins() += ((TEncBinCABAC *)((TEncSbac*)m_pcEntropyCoder->m_pcEntropyCoderIf)->getEncBinIf())->getBinsCoded();
#if NH_3D_VSO // M8
    if ( m_pcRdCost->getUseVSO() )    
    {
      rpcBestCU->getTotalCost()  = m_pcRdCost->calcRdCostVSO( rpcBestCU->getTotalBits(), rpcBestCU->getTotalDistortion() );    
    }
    else
#endif
      rpcBestCU->getTotalCost()  = m_pcRdCost->calcRdCost( rpcBestCU->getTotalBits(), rpcBestCU->getTotalDistortion() );
    m_pcRDGoOnSbacCoder->store(m_pppcRDSbacCoder[uiDepth][CI_NEXT_BEST]);

    // Early CU determination
    if( m_pcEncCfg->getUseEarlyCU() && rpcBestCU->isSkipped(0) )
    {
      bSubBranch = false;
    }
    else
    {
      bSubBranch = true;
    }
#if  H_3D_FAST_TEXTURE_ENCODING
    if(rpcBestCU->getSlice()->getViewIndex() && !rpcBestCU->getSlice()->getIsDepth() && (uiDepth >=iIVFMaxD) && rpcBestCU->isSkipped(0))
    {
      bSubBranch = false;
    }
#endif
  }
  else
  {
    bBoundary = true;
  }

  // copy orginal YUV samples to PCM buffer
  if( rpcBestCU->isLosslessCoded(0) && (rpcBestCU->getIPCMFlag(0) == false))
  {
    xFillPCMBuffer(rpcBestCU, m_ppcOrigYuv[uiDepth]);
  }

  if( uiDepth == pps.getMaxCuDQPDepth() )
  {
    Int idQP = m_pcEncCfg->getMaxDeltaQP();
    iMinQP = Clip3( -sps.getQpBDOffset(CHANNEL_TYPE_LUMA), MAX_QP, iBaseQP-idQP );
    iMaxQP = Clip3( -sps.getQpBDOffset(CHANNEL_TYPE_LUMA), MAX_QP, iBaseQP+idQP );
  }
  else if( uiDepth < pps.getMaxCuDQPDepth() )
  {
    iMinQP = iBaseQP;
    iMaxQP = iBaseQP;
  }
  else
  {
    const Int iStartQP = rpcTempCU->getQP(0);
    iMinQP = iStartQP;
    iMaxQP = iStartQP;
  }

  if ( m_pcEncCfg->getUseRateCtrl() )
  {
    iMinQP = m_pcRateCtrl->getRCQP();
    iMaxQP = m_pcRateCtrl->getRCQP();
  }

  if ( m_pcEncCfg->getCUTransquantBypassFlagForceValue() )
  {
    iMaxQP = iMinQP; // If all TUs are forced into using transquant bypass, do not loop here.
  }

  for (Int iQP=iMinQP; iQP<=iMaxQP; iQP++)
  {
    const Bool bIsLosslessMode = false; // False at this level. Next level down may set it to true.

    rpcTempCU->initEstData( uiDepth, iQP, bIsLosslessMode );

    // further split
#if H_3D_QTLPC
    if( bSubBranch && bTrySplitDQP && uiDepth < g_uiMaxCUDepth - g_uiAddCUDepth )
#else
    if( bSubBranch && uiDepth < sps.getLog2DiffMaxMinCodingBlockSize() )
#endif
    {
#if NH_3D_VSO // M9
      // reset Model
      if( m_pcRdCost->getUseRenModel() )
      {
        UInt  uiWidth     = m_ppcOrigYuv[uiDepth]->getWidth ( COMPONENT_Y );
        UInt  uiHeight    = m_ppcOrigYuv[uiDepth]->getHeight( COMPONENT_Y );
        Pel*  piSrc       = m_ppcOrigYuv[uiDepth]->getAddr  ( COMPONENT_Y, 0 );
        UInt  uiSrcStride = m_ppcOrigYuv[uiDepth]->getStride( COMPONENT_Y  );
        m_pcRdCost->setRenModelData( m_ppcBestCU[uiDepth], 0, piSrc, uiSrcStride, uiWidth, uiHeight );
      }
#endif
      UChar       uhNextDepth         = uiDepth+1;
      TComDataCU* pcSubBestPartCU     = m_ppcBestCU[uhNextDepth];
      TComDataCU* pcSubTempPartCU     = m_ppcTempCU[uhNextDepth];
      DEBUG_STRING_NEW(sTempDebug)
#if NH_3D_ARP
      m_ppcWeightedTempCU[uhNextDepth]->setSlice( m_ppcWeightedTempCU[ uiDepth]->getSlice()); 
      m_ppcWeightedTempCU[uhNextDepth]->setPic  ( m_ppcWeightedTempCU[ uiDepth] ); 
#endif
      for ( UInt uiPartUnitIdx = 0; uiPartUnitIdx < 4; uiPartUnitIdx++ )
      {
        pcSubBestPartCU->initSubCU( rpcTempCU, uiPartUnitIdx, uhNextDepth, iQP );           // clear sub partition datas or init.
        pcSubTempPartCU->initSubCU( rpcTempCU, uiPartUnitIdx, uhNextDepth, iQP );           // clear sub partition datas or init.

        if( ( pcSubBestPartCU->getCUPelX() < sps.getPicWidthInLumaSamples() ) && ( pcSubBestPartCU->getCUPelY() < sps.getPicHeightInLumaSamples() ) )
        {
          if ( 0 == uiPartUnitIdx) //initialize RD with previous depth buffer
          {
            m_pppcRDSbacCoder[uhNextDepth][CI_CURR_BEST]->load(m_pppcRDSbacCoder[uiDepth][CI_CURR_BEST]);
          }
          else
          {
            m_pppcRDSbacCoder[uhNextDepth][CI_CURR_BEST]->load(m_pppcRDSbacCoder[uhNextDepth][CI_NEXT_BEST]);
          }

#if AMP_ENC_SPEEDUP
          DEBUG_STRING_NEW(sChild)
          if ( !rpcBestCU->isInter(0) )
          {
            xCompressCU( pcSubBestPartCU, pcSubTempPartCU, uhNextDepth DEBUG_STRING_PASS_INTO(sChild), NUMBER_OF_PART_SIZES );
          }
          else
          {

            xCompressCU( pcSubBestPartCU, pcSubTempPartCU, uhNextDepth DEBUG_STRING_PASS_INTO(sChild), rpcBestCU->getPartitionSize(0) );
          }
          DEBUG_STRING_APPEND(sTempDebug, sChild)
#else
          xCompressCU( pcSubBestPartCU, pcSubTempPartCU, uhNextDepth );
#endif

          rpcTempCU->copyPartFrom( pcSubBestPartCU, uiPartUnitIdx, uhNextDepth );         // Keep best part data to current temporary data.
          xCopyYuv2Tmp( pcSubBestPartCU->getTotalNumPart()*uiPartUnitIdx, uhNextDepth );
        }
        else
        {
          pcSubBestPartCU->copyToPic( uhNextDepth );
          rpcTempCU->copyPartFrom( pcSubBestPartCU, uiPartUnitIdx, uhNextDepth );
        }
      }

      m_pcRDGoOnSbacCoder->load(m_pppcRDSbacCoder[uhNextDepth][CI_NEXT_BEST]);
      if( !bBoundary )
      {
        m_pcEntropyCoder->resetBits();
        m_pcEntropyCoder->encodeSplitFlag( rpcTempCU, 0, uiDepth, true );

        rpcTempCU->getTotalBits() += m_pcEntropyCoder->getNumberOfWrittenBits(); // split bits
        rpcTempCU->getTotalBins() += ((TEncBinCABAC *)((TEncSbac*)m_pcEntropyCoder->m_pcEntropyCoderIf)->getEncBinIf())->getBinsCoded();
      }
#if NH_3D_VSO // M10
      if ( m_pcRdCost->getUseVSO() )
      {
        rpcTempCU->getTotalCost()  = m_pcRdCost->calcRdCostVSO( rpcTempCU->getTotalBits(), rpcTempCU->getTotalDistortion() );
      }
      else
#endif
        rpcTempCU->getTotalCost()  = m_pcRdCost->calcRdCost( rpcTempCU->getTotalBits(), rpcTempCU->getTotalDistortion() );

      if( uiDepth == pps.getMaxCuDQPDepth() && pps.getUseDQP())
      {
        Bool hasResidual = false;
        for( UInt uiBlkIdx = 0; uiBlkIdx < rpcTempCU->getTotalNumPart(); uiBlkIdx ++)
        {
          if( (     rpcTempCU->getCbf(uiBlkIdx, COMPONENT_Y)
                || (rpcTempCU->getCbf(uiBlkIdx, COMPONENT_Cb) && (numberValidComponents > COMPONENT_Cb))
                || (rpcTempCU->getCbf(uiBlkIdx, COMPONENT_Cr) && (numberValidComponents > COMPONENT_Cr)) ) )
          {
            hasResidual = true;
            break;
          }
        }

        UInt uiTargetPartIdx = 0;
        if ( hasResidual )
        {
          m_pcEntropyCoder->resetBits();
          m_pcEntropyCoder->encodeQP( rpcTempCU, uiTargetPartIdx, false );
          rpcTempCU->getTotalBits() += m_pcEntropyCoder->getNumberOfWrittenBits(); // dQP bits
          rpcTempCU->getTotalBins() += ((TEncBinCABAC *)((TEncSbac*)m_pcEntropyCoder->m_pcEntropyCoderIf)->getEncBinIf())->getBinsCoded();
#if NH_3D_VSO // M11
          if ( m_pcRdCost->getUseLambdaScaleVSO())          
          {
            rpcTempCU->getTotalCost()  = m_pcRdCost->calcRdCostVSO( rpcTempCU->getTotalBits(), rpcTempCU->getTotalDistortion() );          
          }
          else
#endif
            rpcTempCU->getTotalCost()  = m_pcRdCost->calcRdCost( rpcTempCU->getTotalBits(), rpcTempCU->getTotalDistortion() );

          Bool foundNonZeroCbf = false;
          rpcTempCU->setQPSubCUs( rpcTempCU->getRefQP( uiTargetPartIdx ), 0, uiDepth, foundNonZeroCbf );
          assert( foundNonZeroCbf );
        }
        else
        {
          rpcTempCU->setQPSubParts( rpcTempCU->getRefQP( uiTargetPartIdx ), 0, uiDepth ); // set QP to default QP
        }
      }

      m_pcRDGoOnSbacCoder->store(m_pppcRDSbacCoder[uiDepth][CI_TEMP_BEST]);

      // If the configuration being tested exceeds the maximum number of bytes for a slice / slice-segment, then
      // a proper RD evaluation cannot be performed. Therefore, termination of the
      // slice/slice-segment must be made prior to this CTU.
      // This can be achieved by forcing the decision to be that of the rpcTempCU.
      // The exception is each slice / slice-segment must have at least one CTU.
      const Bool isEndOfSlice        =    pcSlice->getSliceMode()==FIXED_NUMBER_OF_BYTES
                                       && ((pcSlice->getSliceBits()+rpcBestCU->getTotalBits())>pcSlice->getSliceArgument()<<3)
                                       && rpcBestCU->getCtuRsAddr() != pcPic->getPicSym()->getCtuTsToRsAddrMap(pcSlice->getSliceCurStartCtuTsAddr())
                                       && rpcBestCU->getCtuRsAddr() != pcPic->getPicSym()->getCtuTsToRsAddrMap(pcSlice->getSliceSegmentCurStartCtuTsAddr());
      const Bool isEndOfSliceSegment =    pcSlice->getSliceSegmentMode()==FIXED_NUMBER_OF_BYTES
                                       && ((pcSlice->getSliceSegmentBits()+rpcBestCU->getTotalBits()) > pcSlice->getSliceSegmentArgument()<<3)
                                       && rpcBestCU->getCtuRsAddr() != pcPic->getPicSym()->getCtuTsToRsAddrMap(pcSlice->getSliceSegmentCurStartCtuTsAddr());
                                           // Do not need to check slice condition for slice-segment since a slice-segment is a subset of a slice.
      if(isEndOfSlice||isEndOfSliceSegment)
      {
        rpcBestCU->getTotalCost()=MAX_DOUBLE;
      }

      xCheckBestMode( rpcBestCU, rpcTempCU, uiDepth DEBUG_STRING_PASS_INTO(sDebug) DEBUG_STRING_PASS_INTO(sTempDebug) DEBUG_STRING_PASS_INTO(false) ); // RD compare current larger prediction
                                                                                       // with sub partitioned prediction.
    }
  }
#if NH_3D_VSO // M12 
  if( m_pcRdCost->getUseRenModel() )
  {
    UInt  uiWidth     = m_ppcRecoYuvBest[uiDepth]->getWidth   ( COMPONENT_Y );
    UInt  uiHeight    = m_ppcRecoYuvBest[uiDepth]->getHeight  ( COMPONENT_Y );
    Pel*  piSrc       = m_ppcRecoYuvBest[uiDepth]->getAddr    ( COMPONENT_Y,  0 );
    UInt  uiSrcStride = m_ppcRecoYuvBest[uiDepth]->getStride  ( COMPONENT_Y );
    m_pcRdCost->setRenModelData( rpcBestCU, 0, piSrc, uiSrcStride, uiWidth, uiHeight );
  }
#endif

  DEBUG_STRING_APPEND(sDebug_, sDebug);

  rpcBestCU->copyToPic(uiDepth);                                                     // Copy Best data to Picture for next partition prediction.

  xCopyYuv2Pic( rpcBestCU->getPic(), rpcBestCU->getCtuRsAddr(), rpcBestCU->getZorderIdxInCtu(), uiDepth, uiDepth );   // Copy Yuv data to picture Yuv
  if (bBoundary)
  {
    return;
  }

  // Assert if Best prediction mode is NONE
  // Selected mode's RD-cost must be not MAX_DOUBLE.
  assert( rpcBestCU->getPartitionSize ( 0 ) != NUMBER_OF_PART_SIZES       );
  assert( rpcBestCU->getPredictionMode( 0 ) != NUMBER_OF_PREDICTION_MODES );
  assert( rpcBestCU->getTotalCost     (   ) != MAX_DOUBLE                 );
}

/** finish encoding a cu and handle end-of-slice conditions
 * \param pcCU
 * \param uiAbsPartIdx
 * \param uiDepth
 * \returns Void
 */
Void TEncCu::finishCU( TComDataCU* pcCU, UInt uiAbsPartIdx )
{
  TComPic* pcPic = pcCU->getPic();
  TComSlice * pcSlice = pcCU->getPic()->getSlice(pcCU->getPic()->getCurrSliceIdx());

  //Calculate end address
  const Int  currentCTUTsAddr = pcPic->getPicSym()->getCtuRsToTsAddrMap(pcCU->getCtuRsAddr());
  const Bool isLastSubCUOfCtu = pcCU->isLastSubCUOfCtu(uiAbsPartIdx);
  if ( isLastSubCUOfCtu )
  {
    // The 1-terminating bit is added to all streams, so don't add it here when it's 1.
    // i.e. when the slice segment CurEnd CTU address is the current CTU address+1.
    if (pcSlice->getSliceSegmentCurEndCtuTsAddr() != currentCTUTsAddr+1)
    {
      m_pcEntropyCoder->encodeTerminatingBit( 0 );
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

  return Clip3(-pcCU->getSlice()->getSPS()->getQpBDOffset(CHANNEL_TYPE_LUMA), MAX_QP, iBaseQp+iQpOffset );
}

/** encode a CU block recursively
 * \param pcCU
 * \param uiAbsPartIdx
 * \param uiDepth
 * \returns Void
 */
Void TEncCu::xEncodeCU( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth )
{
        TComPic   *const pcPic   = pcCU->getPic();
        TComSlice *const pcSlice = pcCU->getSlice();
  const TComSPS   &sps =*(pcSlice->getSPS());
  const TComPPS   &pps =*(pcSlice->getPPS());

  const UInt maxCUWidth  = sps.getMaxCUWidth();
  const UInt maxCUHeight = sps.getMaxCUHeight();

        Bool bBoundary = false;
        UInt uiLPelX   = pcCU->getCUPelX() + g_auiRasterToPelX[ g_auiZscanToRaster[uiAbsPartIdx] ];
  const UInt uiRPelX   = uiLPelX + (maxCUWidth>>uiDepth)  - 1;
        UInt uiTPelY   = pcCU->getCUPelY() + g_auiRasterToPelY[ g_auiZscanToRaster[uiAbsPartIdx] ];
  const UInt uiBPelY   = uiTPelY + (maxCUHeight>>uiDepth) - 1;

#if H_MV_ENC_DEC_TRAC
  DTRACE_CU_S("=========== coding_quadtree ===========\n")
  DTRACE_CU("x0", uiLPelX)
  DTRACE_CU("x1", uiTPelY)
  DTRACE_CU("log2CbSize", maxCUWidth>>uiDepth )
  DTRACE_CU("cqtDepth"  , uiDepth)
#endif

  if( ( uiRPelX < sps.getPicWidthInLumaSamples() ) && ( uiBPelY < sps.getPicHeightInLumaSamples() ) )
  {
    m_pcEntropyCoder->encodeSplitFlag( pcCU, uiAbsPartIdx, uiDepth );
  }
  else
  {
    bBoundary = true;
  }

  if( ( ( uiDepth < pcCU->getDepth( uiAbsPartIdx ) ) && ( uiDepth < sps.getLog2DiffMaxMinCodingBlockSize() ) ) || bBoundary )
  {
    UInt uiQNumParts = ( pcPic->getNumPartitionsInCtu() >> (uiDepth<<1) )>>2;
    if( uiDepth == pps.getMaxCuDQPDepth() && pps.getUseDQP())
    {
      setdQPFlag(true);
    }

    if( uiDepth == pps.getPpsRangeExtension().getDiffCuChromaQpOffsetDepth() && pcSlice->getUseChromaQpAdj())
    {
      setCodeChromaQpAdjFlag(true);
    }

    for ( UInt uiPartUnitIdx = 0; uiPartUnitIdx < 4; uiPartUnitIdx++, uiAbsPartIdx+=uiQNumParts )
    {
      uiLPelX   = pcCU->getCUPelX() + g_auiRasterToPelX[ g_auiZscanToRaster[uiAbsPartIdx] ];
      uiTPelY   = pcCU->getCUPelY() + g_auiRasterToPelY[ g_auiZscanToRaster[uiAbsPartIdx] ];
      if( ( uiLPelX < sps.getPicWidthInLumaSamples() ) && ( uiTPelY < sps.getPicHeightInLumaSamples() ) )
      {
        xEncodeCU( pcCU, uiAbsPartIdx, uiDepth+1 );
      }
    }
    return;
  }

#if H_MV_ENC_DEC_TRAC
  DTRACE_CU_S("=========== coding_unit ===========\n")
#endif


  if( uiDepth <= pps.getMaxCuDQPDepth() && pps.getUseDQP())
  {
    setdQPFlag(true);
  }

  if( uiDepth <= pps.getPpsRangeExtension().getDiffCuChromaQpOffsetDepth() && pcSlice->getUseChromaQpAdj())
  {
    setCodeChromaQpAdjFlag(true);
  }

  if (pps.getTransquantBypassEnableFlag())
  {
    m_pcEntropyCoder->encodeCUTransquantBypassFlag( pcCU, uiAbsPartIdx );
  }

  if( !pcSlice->isIntra() )
  {
    m_pcEntropyCoder->encodeSkipFlag( pcCU, uiAbsPartIdx );
  }

  if( pcCU->isSkipped( uiAbsPartIdx ) )
  {
#if H_MV_ENC_DEC_TRAC
    DTRACE_PU_S("=========== prediction_unit ===========\n")
    DTRACE_PU("x0", uiLPelX)
    DTRACE_PU("x1", uiTPelY)
#endif

    m_pcEntropyCoder->encodeMergeIndex( pcCU, uiAbsPartIdx );
#if NH_3D_ARP
    m_pcEntropyCoder->encodeARPW( pcCU , uiAbsPartIdx );
#endif
#if H_3D_IC
    m_pcEntropyCoder->encodeICFlag  ( pcCU, uiAbsPartIdx );
#endif

    finishCU(pcCU,uiAbsPartIdx);
    return;
  }

#if H_3D
  m_pcEntropyCoder->encodeDIS( pcCU, uiAbsPartIdx );
  if(!pcCU->getDISFlag(uiAbsPartIdx))
  {
#endif
  m_pcEntropyCoder->encodePredMode( pcCU, uiAbsPartIdx );
  m_pcEntropyCoder->encodePartSize( pcCU, uiAbsPartIdx, uiDepth );

  if (pcCU->isIntra( uiAbsPartIdx ) && pcCU->getPartitionSize( uiAbsPartIdx ) == SIZE_2Nx2N )
  {
    m_pcEntropyCoder->encodeIPCMInfo( pcCU, uiAbsPartIdx );

    if(pcCU->getIPCMFlag(uiAbsPartIdx))
    {
#if H_3D_DIM_SDC
      m_pcEntropyCoder->encodeSDCFlag( pcCU, uiAbsPartIdx, false );
#endif  

      // Encode slice finish
      finishCU(pcCU,uiAbsPartIdx);
      return;
    }
  }

  // prediction Info ( Intra : direction mode, Inter : Mv, reference idx )
  m_pcEntropyCoder->encodePredInfo( pcCU, uiAbsPartIdx );
#if NH_3D_DBBP
  m_pcEntropyCoder->encodeDBBPFlag( pcCU, uiAbsPartIdx );
#endif
#if H_3D_DIM_SDC
  m_pcEntropyCoder->encodeSDCFlag( pcCU, uiAbsPartIdx, false );
#endif  
#if NH_3D_ARP
  m_pcEntropyCoder->encodeARPW( pcCU , uiAbsPartIdx );
#endif
#if H_3D_IC
  m_pcEntropyCoder->encodeICFlag  ( pcCU, uiAbsPartIdx );
#endif

  // Encode Coefficients
  Bool bCodeDQP = getdQPFlag();
  Bool codeChromaQpAdj = getCodeChromaQpAdjFlag();
  m_pcEntropyCoder->encodeCoeff( pcCU, uiAbsPartIdx, uiDepth, bCodeDQP, codeChromaQpAdj );
  setCodeChromaQpAdjFlag( codeChromaQpAdj );
  setdQPFlag( bCodeDQP );
#if H_3D
  }
#endif


  // --- write terminating bit ---
  finishCU(pcCU,uiAbsPartIdx);
}

Int xCalcHADs8x8_ISlice(Pel *piOrg, Int iStrideOrg)
{
  Int k, i, j, jj;
  Int diff[64], m1[8][8], m2[8][8], m3[8][8], iSumHad = 0;

  for( k = 0; k < 64; k += 8 )
  {
    diff[k+0] = piOrg[0] ;
    diff[k+1] = piOrg[1] ;
    diff[k+2] = piOrg[2] ;
    diff[k+3] = piOrg[3] ;
    diff[k+4] = piOrg[4] ;
    diff[k+5] = piOrg[5] ;
    diff[k+6] = piOrg[6] ;
    diff[k+7] = piOrg[7] ;

    piOrg += iStrideOrg;
  }

  //horizontal
  for (j=0; j < 8; j++)
  {
    jj = j << 3;
    m2[j][0] = diff[jj  ] + diff[jj+4];
    m2[j][1] = diff[jj+1] + diff[jj+5];
    m2[j][2] = diff[jj+2] + diff[jj+6];
    m2[j][3] = diff[jj+3] + diff[jj+7];
    m2[j][4] = diff[jj  ] - diff[jj+4];
    m2[j][5] = diff[jj+1] - diff[jj+5];
    m2[j][6] = diff[jj+2] - diff[jj+6];
    m2[j][7] = diff[jj+3] - diff[jj+7];

    m1[j][0] = m2[j][0] + m2[j][2];
    m1[j][1] = m2[j][1] + m2[j][3];
    m1[j][2] = m2[j][0] - m2[j][2];
    m1[j][3] = m2[j][1] - m2[j][3];
    m1[j][4] = m2[j][4] + m2[j][6];
    m1[j][5] = m2[j][5] + m2[j][7];
    m1[j][6] = m2[j][4] - m2[j][6];
    m1[j][7] = m2[j][5] - m2[j][7];

    m2[j][0] = m1[j][0] + m1[j][1];
    m2[j][1] = m1[j][0] - m1[j][1];
    m2[j][2] = m1[j][2] + m1[j][3];
    m2[j][3] = m1[j][2] - m1[j][3];
    m2[j][4] = m1[j][4] + m1[j][5];
    m2[j][5] = m1[j][4] - m1[j][5];
    m2[j][6] = m1[j][6] + m1[j][7];
    m2[j][7] = m1[j][6] - m1[j][7];
  }

  //vertical
  for (i=0; i < 8; i++)
  {
    m3[0][i] = m2[0][i] + m2[4][i];
    m3[1][i] = m2[1][i] + m2[5][i];
    m3[2][i] = m2[2][i] + m2[6][i];
    m3[3][i] = m2[3][i] + m2[7][i];
    m3[4][i] = m2[0][i] - m2[4][i];
    m3[5][i] = m2[1][i] - m2[5][i];
    m3[6][i] = m2[2][i] - m2[6][i];
    m3[7][i] = m2[3][i] - m2[7][i];

    m1[0][i] = m3[0][i] + m3[2][i];
    m1[1][i] = m3[1][i] + m3[3][i];
    m1[2][i] = m3[0][i] - m3[2][i];
    m1[3][i] = m3[1][i] - m3[3][i];
    m1[4][i] = m3[4][i] + m3[6][i];
    m1[5][i] = m3[5][i] + m3[7][i];
    m1[6][i] = m3[4][i] - m3[6][i];
    m1[7][i] = m3[5][i] - m3[7][i];

    m2[0][i] = m1[0][i] + m1[1][i];
    m2[1][i] = m1[0][i] - m1[1][i];
    m2[2][i] = m1[2][i] + m1[3][i];
    m2[3][i] = m1[2][i] - m1[3][i];
    m2[4][i] = m1[4][i] + m1[5][i];
    m2[5][i] = m1[4][i] - m1[5][i];
    m2[6][i] = m1[6][i] + m1[7][i];
    m2[7][i] = m1[6][i] - m1[7][i];
  }

  for (i = 0; i < 8; i++)
  {
    for (j = 0; j < 8; j++)
    {
      iSumHad += abs(m2[i][j]);
    }
  }
  iSumHad -= abs(m2[0][0]);
  iSumHad =(iSumHad+2)>>2;
  return(iSumHad);
}

Int  TEncCu::updateCtuDataISlice(TComDataCU* pCtu, Int width, Int height)
{
  Int  xBl, yBl;
  const Int iBlkSize = 8;

  Pel* pOrgInit   = pCtu->getPic()->getPicYuvOrg()->getAddr(COMPONENT_Y, pCtu->getCtuRsAddr(), 0);
  Int  iStrideOrig = pCtu->getPic()->getPicYuvOrg()->getStride(COMPONENT_Y);
  Pel  *pOrg;

  Int iSumHad = 0;
  for ( yBl=0; (yBl+iBlkSize)<=height; yBl+= iBlkSize)
  {
    for ( xBl=0; (xBl+iBlkSize)<=width; xBl+= iBlkSize)
    {
      pOrg = pOrgInit + iStrideOrig*yBl + xBl;
      iSumHad += xCalcHADs8x8_ISlice(pOrg, iStrideOrig);
    }
  }
  return(iSumHad);
}

/** check RD costs for a CU block encoded with merge
 * \param rpcBestCU
 * \param rpcTempCU
 * \param earlyDetectionSkipMode
 */
Void TEncCu::xCheckRDCostMerge2Nx2N( TComDataCU*& rpcBestCU, TComDataCU*& rpcTempCU DEBUG_STRING_FN_DECLARE(sDebug), Bool *earlyDetectionSkipMode )
{
  assert( rpcTempCU->getSlice()->getSliceType() != I_SLICE );
#if H_3D_IV_MERGE
  TComMvField  cMvFieldNeighbours[MRG_MAX_NUM_CANDS_MEM << 1]; // double length for mv of both lists
  UChar uhInterDirNeighbours[MRG_MAX_NUM_CANDS_MEM];
#else
  TComMvField  cMvFieldNeighbours[2 * MRG_MAX_NUM_CANDS]; // double length for mv of both lists
  UChar uhInterDirNeighbours[MRG_MAX_NUM_CANDS];
#endif
  Int numValidMergeCand = 0;
  const Bool bTransquantBypassFlag = rpcTempCU->getCUTransquantBypass(0);

  for( UInt ui = 0; ui < rpcTempCU->getSlice()->getMaxNumMergeCand(); ++ui )
  {
    uhInterDirNeighbours[ui] = 0;
  }
  UChar uhDepth = rpcTempCU->getDepth( 0 );
#if H_3D_IC
  Bool bICFlag = rpcTempCU->getICFlag( 0 );
#endif
#if NH_3D_VSO // M1  //necessary here?
  if( m_pcRdCost->getUseRenModel() )
  {
    UInt  uiWidth     = m_ppcOrigYuv[uhDepth]->getWidth ( COMPONENT_Y );
    UInt  uiHeight    = m_ppcOrigYuv[uhDepth]->getHeight( COMPONENT_Y );
    Pel*  piSrc       = m_ppcOrigYuv[uhDepth]->getAddr  ( COMPONENT_Y );
    UInt  uiSrcStride = m_ppcOrigYuv[uhDepth]->getStride( COMPONENT_Y );
    m_pcRdCost->setRenModelData( rpcTempCU, 0, piSrc, uiSrcStride, uiWidth, uiHeight );
  }
#endif

#if NH_3D_ARP
  DisInfo cOrigDisInfo = rpcTempCU->getDvInfo(0);
#else
#endif

  rpcTempCU->setPartSizeSubParts( SIZE_2Nx2N, 0, uhDepth ); // interprets depth relative to CTU level
#if H_3D_VSP
#if !NH_3D_ARP
  Int vspFlag[MRG_MAX_NUM_CANDS_MEM];
  memset(vspFlag, 0, sizeof(Int)*MRG_MAX_NUM_CANDS_MEM);
  InheritedVSPDisInfo inheritedVSPDisInfo[MRG_MAX_NUM_CANDS_MEM];
  rpcTempCU->m_bAvailableFlagA1 = 0;
  rpcTempCU->m_bAvailableFlagB1 = 0;
  rpcTempCU->m_bAvailableFlagB0 = 0;
  rpcTempCU->m_bAvailableFlagA0 = 0;
  rpcTempCU->m_bAvailableFlagB2 = 0;
  rpcTempCU->getInterMergeCandidates( 0, 0, cMvFieldNeighbours, uhInterDirNeighbours, numValidMergeCand );
  rpcTempCU->xGetInterMergeCandidates( 0, 0, cMvFieldNeighbours,uhInterDirNeighbours, vspFlag,inheritedVSPDisInfo, numValidMergeCand );
#endif
#else
#if H_3D
  rpcTempCU->m_bAvailableFlagA1 = 0;
  rpcTempCU->m_bAvailableFlagB1 = 0;
  rpcTempCU->m_bAvailableFlagB0 = 0;
  rpcTempCU->m_bAvailableFlagA0 = 0;
  rpcTempCU->m_bAvailableFlagB2 = 0;
  rpcTempCU->getInterMergeCandidates( 0, 0, cMvFieldNeighbours, uhInterDirNeighbours, numValidMergeCand );
  rpcTempCU->xGetInterMergeCandidates( 0, 0, cMvFieldNeighbours,uhInterDirNeighbours, numValidMergeCand );
#else
  rpcTempCU->getInterMergeCandidates( 0, 0, cMvFieldNeighbours,uhInterDirNeighbours, numValidMergeCand );
#endif
#endif
#if H_3D_IV_MERGE
  Int mergeCandBuffer[MRG_MAX_NUM_CANDS_MEM];
#else
  Int mergeCandBuffer[MRG_MAX_NUM_CANDS];
#endif

  for( UInt ui = 0; ui < numValidMergeCand; ++ui )
  {
    mergeCandBuffer[ui] = 0;
  }

  Bool bestIsSkip = false;

  UInt iteration;
  if ( rpcTempCU->isLosslessCoded(0))
  {
    iteration = 1;
  }
  else
  {
    iteration = 2;
  }
  DEBUG_STRING_NEW(bestStr)

#if NH_3D_ARP
  Int nARPWMax = rpcTempCU->getSlice()->getARPStepNum() - 1;
#if H_3D_IC
  if( nARPWMax < 0 || bICFlag )
#else
  if( nARPWMax < 0 )
#endif
  {
    nARPWMax = 0;
  }
  for( Int nARPW = nARPWMax; nARPW >= 0 ; nARPW-- )
  {
#if NH_3D_IV_MERGE
    memset( mergeCandBuffer, 0, MRG_MAX_NUM_CANDS_MEM * sizeof(Int) );    
#else
    memset( mergeCandBuffer, 0, MRG_MAX_NUM_CANDS * sizeof(Int) );
#endif
    rpcTempCU->setPartSizeSubParts(   SIZE_2Nx2N , 0 , uhDepth ); // interprets depth relative to LCU level
    rpcTempCU->setARPWSubParts    ( (UChar)nARPW , 0 , uhDepth );
#if H_3D_IC
    rpcTempCU->setICFlagSubParts( bICFlag, 0, 0, uhDepth );
#endif
    rpcTempCU->getDvInfo(0) = cOrigDisInfo;
    rpcTempCU->setDvInfoSubParts(cOrigDisInfo, 0, uhDepth );
    Int vspFlag[MRG_MAX_NUM_CANDS_MEM];
    memset(vspFlag, 0, sizeof(Int)*MRG_MAX_NUM_CANDS_MEM);
#if H_3D_SPIVMP
    Bool bSPIVMPFlag[MRG_MAX_NUM_CANDS_MEM];
    memset(bSPIVMPFlag, false, sizeof(Bool)*MRG_MAX_NUM_CANDS_MEM);
    TComMvField*  pcMvFieldSP;
    UChar* puhInterDirSP;
    pcMvFieldSP = new TComMvField[rpcTempCU->getPic()->getPicSym()->getNumPartition()*2]; 
    puhInterDirSP = new UChar[rpcTempCU->getPic()->getPicSym()->getNumPartition()]; 
#endif
#if H_3D
    rpcTempCU->initAvailableFlags();
    rpcTempCU->getInterMergeCandidates( 0, 0, cMvFieldNeighbours, uhInterDirNeighbours, numValidMergeCand );
    rpcTempCU->xGetInterMergeCandidates( 0, 0, cMvFieldNeighbours,uhInterDirNeighbours
#if H_3D_SPIVMP
      , pcMvFieldSP, puhInterDirSP
#endif
      , numValidMergeCand 
      );

    rpcTempCU->buildMCL( cMvFieldNeighbours,uhInterDirNeighbours, vspFlag
#if H_3D_SPIVMP
      , bSPIVMPFlag
#endif
      , numValidMergeCand 
      );
#else
    rpcTempCU->getInterMergeCandidates( 0, 0, cMvFieldNeighbours,uhInterDirNeighbours, numValidMergeCand );
#endif


#endif

  for( UInt uiNoResidual = 0; uiNoResidual < iteration; ++uiNoResidual )
  {
    for( UInt uiMergeCand = 0; uiMergeCand < numValidMergeCand; ++uiMergeCand )
    {
#if H_3D_IC
      if( rpcTempCU->getSlice()->getApplyIC() && rpcTempCU->getSlice()->getIcSkipParseFlag() )
      {
        if( bICFlag && uiMergeCand == 0 ) 
        {
          continue;
        }
      }
#endif

      if(!(uiNoResidual==1 && mergeCandBuffer[uiMergeCand]==1))
      {
        if( !(bestIsSkip && uiNoResidual == 0) )
        {
          DEBUG_STRING_NEW(tmpStr)
          // set MC parameters
          rpcTempCU->setPredModeSubParts( MODE_INTER, 0, uhDepth ); // interprets depth relative to CTU level
#if H_3D_IC
          rpcTempCU->setICFlagSubParts( bICFlag, 0, 0, uhDepth );
#endif
          rpcTempCU->setCUTransquantBypassSubParts( bTransquantBypassFlag, 0, uhDepth );
          rpcTempCU->setChromaQpAdjSubParts( bTransquantBypassFlag ? 0 : m_cuChromaQpOffsetIdxPlus1, 0, uhDepth );
          rpcTempCU->setPartSizeSubParts( SIZE_2Nx2N, 0, uhDepth ); // interprets depth relative to CTU level
          rpcTempCU->setMergeFlagSubParts( true, 0, 0, uhDepth ); // interprets depth relative to CTU level
          rpcTempCU->setMergeIndexSubParts( uiMergeCand, 0, 0, uhDepth ); // interprets depth relative to CTU level
#if NH_3D_ARP
          rpcTempCU->setARPWSubParts( (UChar)nARPW , 0 , uhDepth );
#endif
#if H_3D_VSP
          rpcTempCU->setVSPFlagSubParts( vspFlag[uiMergeCand], 0, 0, uhDepth );
#endif
#if H_3D_SPIVMP
          rpcTempCU->setSPIVMPFlagSubParts(bSPIVMPFlag[uiMergeCand], 0, 0, uhDepth);
          if (bSPIVMPFlag[uiMergeCand])
          {
            UInt uiSPAddr;
            Int iWidth = rpcTempCU->getWidth(0);
            Int iHeight = rpcTempCU->getHeight(0);
            Int iNumSPInOneLine, iNumSP, iSPWidth, iSPHeight;
            rpcTempCU->getSPPara(iWidth, iHeight, iNumSP, iNumSPInOneLine, iSPWidth, iSPHeight);
            for (Int iPartitionIdx = 0; iPartitionIdx < iNumSP; iPartitionIdx++)
            {
              rpcTempCU->getSPAbsPartIdx(0, iSPWidth, iSPHeight, iPartitionIdx, iNumSPInOneLine, uiSPAddr);
              rpcTempCU->setInterDirSP(puhInterDirSP[iPartitionIdx], uiSPAddr, iSPWidth, iSPHeight);
              rpcTempCU->getCUMvField( REF_PIC_LIST_0 )->setMvFieldSP(rpcTempCU, uiSPAddr, pcMvFieldSP[2*iPartitionIdx], iSPWidth, iSPHeight);
              rpcTempCU->getCUMvField( REF_PIC_LIST_1 )->setMvFieldSP(rpcTempCU, uiSPAddr, pcMvFieldSP[2*iPartitionIdx + 1], iSPWidth, iSPHeight);
            }
          }
          else
#endif
#if H_3D_VSP
          {
            if ( vspFlag[uiMergeCand] )
            {
              UInt partAddr;
              Int vspSize;
              Int width, height;
              rpcTempCU->getPartIndexAndSize( 0, partAddr, width, height );
              if( uhInterDirNeighbours[ uiMergeCand ] & 0x01 )
              {
                rpcTempCU->setMvFieldPUForVSP( rpcTempCU, partAddr, width, height, REF_PIC_LIST_0, cMvFieldNeighbours[ 2*uiMergeCand + 0 ].getRefIdx(), vspSize );
                rpcTempCU->setVSPFlag( partAddr, vspSize );
              }
              else
              {
                rpcTempCU->getCUMvField( REF_PIC_LIST_0 )->setAllMvField( cMvFieldNeighbours[0 + 2*uiMergeCand], SIZE_2Nx2N, 0, 0 ); // interprets depth relative to rpcTempCU level
              }
              if( uhInterDirNeighbours[ uiMergeCand ] & 0x02 )
              {
                rpcTempCU->setMvFieldPUForVSP( rpcTempCU, partAddr, width, height, REF_PIC_LIST_1 , cMvFieldNeighbours[ 2*uiMergeCand + 1 ].getRefIdx(), vspSize );
                rpcTempCU->setVSPFlag( partAddr, vspSize );
              }
              else
              {
                rpcTempCU->getCUMvField( REF_PIC_LIST_1 )->setAllMvField( cMvFieldNeighbours[1 + 2*uiMergeCand], SIZE_2Nx2N, 0, 0 ); // interprets depth relative to rpcTempCU level
              }
              rpcTempCU->setInterDirSubParts( uhInterDirNeighbours[uiMergeCand], 0, 0, uhDepth ); // interprets depth relative to LCU level
            }
            else
            {
#endif
          rpcTempCU->setInterDirSubParts( uhInterDirNeighbours[uiMergeCand], 0, 0, uhDepth ); // interprets depth relative to CTU level
          rpcTempCU->getCUMvField( REF_PIC_LIST_0 )->setAllMvField( cMvFieldNeighbours[0 + 2*uiMergeCand], SIZE_2Nx2N, 0, 0 ); // interprets depth relative to rpcTempCU level
          rpcTempCU->getCUMvField( REF_PIC_LIST_1 )->setAllMvField( cMvFieldNeighbours[1 + 2*uiMergeCand], SIZE_2Nx2N, 0, 0 ); // interprets depth relative to rpcTempCU level
#if H_3D_VSP
            }
          }
#endif
          // do MC
          m_pcPredSearch->motionCompensation ( rpcTempCU, m_ppcPredYuvTemp[uhDepth] );
          // estimate residual and encode everything
#if NH_3D_VSO //M2
          if( m_pcRdCost->getUseRenModel() )
          { //Reset
            UInt  uiWidth     = m_ppcOrigYuv[uhDepth]->getWidth    ( COMPONENT_Y );
            UInt  uiHeight    = m_ppcOrigYuv[uhDepth]->getHeight   ( COMPONENT_Y );
            Pel*  piSrc       = m_ppcOrigYuv[uhDepth]->getAddr     ( COMPONENT_Y );
            UInt  uiSrcStride = m_ppcOrigYuv[uhDepth]->getStride   ( COMPONENT_Y );
            m_pcRdCost->setRenModelData( rpcTempCU, 0, piSrc, uiSrcStride, uiWidth, uiHeight );
          }
#endif
          m_pcPredSearch->encodeResAndCalcRdInterCU( rpcTempCU,
                                                     m_ppcOrigYuv    [uhDepth],
                                                     m_ppcPredYuvTemp[uhDepth],
                                                     m_ppcResiYuvTemp[uhDepth],
                                                     m_ppcResiYuvBest[uhDepth],
                                                     m_ppcRecoYuvTemp[uhDepth],
                                                     (uiNoResidual != 0) DEBUG_STRING_PASS_INTO(tmpStr) );

#if DEBUG_STRING
          DebugInterPredResiReco(tmpStr, *(m_ppcPredYuvTemp[uhDepth]), *(m_ppcResiYuvBest[uhDepth]), *(m_ppcRecoYuvTemp[uhDepth]), DebugStringGetPredModeMask(rpcTempCU->getPredictionMode(0)));
#endif

          if ((uiNoResidual == 0) && (rpcTempCU->getQtRootCbf(0) == 0))
          {
            // If no residual when allowing for one, then set mark to not try case where residual is forced to 0
            mergeCandBuffer[uiMergeCand] = 1;
          }
#if H_3D
          rpcTempCU->setDISFlagSubParts( false, 0, uhDepth );
#endif
#if H_3D_VSP // possible bug fix
          if( rpcTempCU->getSkipFlag(0) )
          {
            rpcTempCU->setTrIdxSubParts(0, 0, uhDepth);
          }
#endif
#if H_3D_INTER_SDC
          TComDataCU *rpcTempCUPre = rpcTempCU;
#endif
          Int orgQP = rpcTempCU->getQP( 0 );
          xCheckDQP( rpcTempCU );
          xCheckBestMode(rpcBestCU, rpcTempCU, uhDepth DEBUG_STRING_PASS_INTO(bestStr) DEBUG_STRING_PASS_INTO(tmpStr));
#if H_3D_INTER_SDC
          if( rpcTempCU->getSlice()->getInterSdcFlag() && !uiNoResidual )
          {
            Double dOffsetCost[3] = {MAX_DOUBLE,MAX_DOUBLE,MAX_DOUBLE};
            for( Int uiOffest = 1 ; uiOffest <= 5 ; uiOffest++ )
            {
              if( uiOffest > 3)
              {
                if ( dOffsetCost[0] < (0.9*dOffsetCost[1]) && dOffsetCost[0] < (0.9*dOffsetCost[2]) )
                {
                  continue;
                }
                if ( dOffsetCost[1] < dOffsetCost[0] && dOffsetCost[0] < dOffsetCost[2] &&  uiOffest == 5)
                {
                  continue;
                }
                if ( dOffsetCost[0] < dOffsetCost[1] && dOffsetCost[2] < dOffsetCost[0] &&  uiOffest == 4)
                {
                  continue;
                }
              }
              if( rpcTempCU != rpcTempCUPre )
              {
                rpcTempCU->initEstData( uhDepth, orgQP, bTransquantBypassFlag  );
                rpcTempCU->copyPartFrom( rpcBestCU, 0, uhDepth );
              }
              rpcTempCU->setSkipFlagSubParts( false, 0, uhDepth );
#if H_3D
              rpcTempCU->setDISFlagSubParts( false, 0, uhDepth );
#endif
              rpcTempCU->setTrIdxSubParts( 0, 0, uhDepth );
              rpcTempCU->setCbfSubParts( 1, 1, 1, 0, uhDepth );
#if NH_3D_VSO //M2
              if( m_pcRdCost->getUseRenModel() )
              { //Reset
                UInt  uiWidth     = m_ppcOrigYuv[uhDepth]->getWidth    ( COMPONENT_Y );
                UInt  uiHeight    = m_ppcOrigYuv[uhDepth]->getHeight   ( COMPONENT_Y );
                Pel*  piSrc       = m_ppcOrigYuv[uhDepth]->getAddr     ( COMPONENT_Y );
                UInt  uiSrcStride = m_ppcOrigYuv[uhDepth]->getStride   ( COMPONENT_Y );
                m_pcRdCost->setRenModelData( rpcTempCU, 0, piSrc, uiSrcStride, uiWidth, uiHeight );
              }
#endif
              Int iSdcOffset = 0;
              if(uiOffest % 2 == 0)
              {
                iSdcOffset = uiOffest >> 1;
              }
              else
              {
                iSdcOffset = -1 * (uiOffest >> 1);
              }
              m_pcPredSearch->encodeResAndCalcRdInterSDCCU( rpcTempCU, 
                m_ppcOrigYuv[uhDepth], 
                ( rpcTempCU != rpcTempCUPre ) ? m_ppcPredYuvBest[uhDepth] : m_ppcPredYuvTemp[uhDepth], 
                m_ppcResiYuvTemp[uhDepth], 
                m_ppcRecoYuvTemp[uhDepth],
                iSdcOffset,
                uhDepth );
              if (uiOffest <= 3 )
              {
                dOffsetCost [uiOffest -1] = rpcTempCU->getTotalCost();
              }

              xCheckDQP( rpcTempCU );
              xCheckBestMode( rpcBestCU, rpcTempCU, uhDepth );
            }
          }
#endif

          rpcTempCU->initEstData( uhDepth, orgQP, bTransquantBypassFlag );
          if( m_pcEncCfg->getUseFastDecisionForMerge() && !bestIsSkip )
          {
#if H_3D_INTER_SDC
            if( rpcTempCU->getSlice()->getInterSdcFlag() )
            {
              bestIsSkip = !rpcBestCU->getSDCFlag( 0 ) && ( rpcBestCU->getQtRootCbf(0) == 0 );
            }
            else
            {
#endif
            bestIsSkip = rpcBestCU->getQtRootCbf(0) == 0;
#if H_3D_INTER_SDC
            }
#endif
          }
        }
      }
    }

    if(uiNoResidual == 0 && m_pcEncCfg->getUseEarlySkipDetection())
    {
      if(rpcBestCU->getQtRootCbf( 0 ) == 0)
      {
        if( rpcBestCU->getMergeFlag( 0 ))
        {
          *earlyDetectionSkipMode = true;
        }
        else if(m_pcEncCfg->getFastSearch() != SELECTIVE)
        {
          Int absoulte_MV=0;
          for ( UInt uiRefListIdx = 0; uiRefListIdx < 2; uiRefListIdx++ )
          {
            if ( rpcBestCU->getSlice()->getNumRefIdx( RefPicList( uiRefListIdx ) ) > 0 )
            {
              TComCUMvField* pcCUMvField = rpcBestCU->getCUMvField(RefPicList( uiRefListIdx ));
              Int iHor = pcCUMvField->getMvd( 0 ).getAbsHor();
              Int iVer = pcCUMvField->getMvd( 0 ).getAbsVer();
              absoulte_MV+=iHor+iVer;
            }
          }

          if(absoulte_MV == 0)
          {
            *earlyDetectionSkipMode = true;
          }
        }
      }
    }
  }
  DEBUG_STRING_APPEND(sDebug, bestStr)
#if H_3D_SPIVMP
 delete[] pcMvFieldSP;
 delete[] puhInterDirSP;
#endif
#if NH_3D_ARP
 }
#endif

}


#if AMP_MRG
#if  H_3D_FAST_TEXTURE_ENCODING
Void TEncCu::xCheckRDCostInter( TComDataCU*& rpcBestCU, TComDataCU*& rpcTempCU, PartSize ePartSize, Bool bFMD, Bool bUseMRG)
#else
Void TEncCu::xCheckRDCostInter( TComDataCU*& rpcBestCU, TComDataCU*& rpcTempCU, PartSize ePartSize DEBUG_STRING_FN_DECLARE(sDebug), Bool bUseMRG)
#endif
#else
Void TEncCu::xCheckRDCostInter( TComDataCU*& rpcBestCU, TComDataCU*& rpcTempCU, PartSize ePartSize )
#endif
{
  DEBUG_STRING_NEW(sTest)

  // prior to this, rpcTempCU will have just been reset using rpcTempCU->initEstData( uiDepth, iQP, bIsLosslessMode );
#if H_3D || NH_3D_ARP
  const Bool bTransquantBypassFlag = rpcTempCU->getCUTransquantBypass(0);
#endif
#if  H_3D_FAST_TEXTURE_ENCODING
  if(!(bFMD && (ePartSize == SIZE_2Nx2N)))  //have  motion estimation or merge check
  {
#endif
  UChar uhDepth = rpcTempCU->getDepth( 0 );
#if NH_3D_ARP
    Bool bFirstTime = true;
    Int nARPWMax    = rpcTempCU->getSlice()->getARPStepNum() - 1;
#if H_3D_IC
    if( nARPWMax < 0 || ePartSize != SIZE_2Nx2N || rpcTempCU->getICFlag(0) )
#else
    if( nARPWMax < 0 || ePartSize != SIZE_2Nx2N )
#endif
    {
      nARPWMax = 0;
    }

    for( Int nARPW = 0; nARPW <= nARPWMax; nARPW++ )
    {
      if( !bFirstTime && rpcTempCU->getSlice()->getIvResPredFlag() )
      {
        rpcTempCU->initEstData( rpcTempCU->getDepth(0), rpcTempCU->getQP(0),bTransquantBypassFlag );      
      }
#endif
#if NH_3D_VSO // M3
      if( m_pcRdCost->getUseRenModel() )
      {
        UInt  uiWidth     = m_ppcOrigYuv[uhDepth]->getWidth ( COMPONENT_Y );
        UInt  uiHeight    = m_ppcOrigYuv[uhDepth]->getHeight( COMPONENT_Y );
        Pel*  piSrc       = m_ppcOrigYuv[uhDepth]->getAddr  ( COMPONENT_Y );
        UInt  uiSrcStride = m_ppcOrigYuv[uhDepth]->getStride( COMPONENT_Y );
        m_pcRdCost->setRenModelData( rpcTempCU, 0, piSrc, uiSrcStride, uiWidth, uiHeight );
      }
#endif
#if H_3D
      rpcTempCU->setDISFlagSubParts( false, 0, uhDepth );
#endif
  rpcTempCU->setPartSizeSubParts  ( ePartSize,  0, uhDepth );
  rpcTempCU->setPredModeSubParts  ( MODE_INTER, 0, uhDepth );
  rpcTempCU->setChromaQpAdjSubParts( rpcTempCU->getCUTransquantBypass(0) ? 0 : m_cuChromaQpOffsetIdxPlus1, 0, uhDepth );
#if NH_3D_ARP
  rpcTempCU->setARPWSubParts( (UChar)nARPW , 0 , uhDepth );
#endif
#if NH_3D_ARP
  if( bFirstTime == false && nARPWMax )
  {
    rpcTempCU->copyPartFrom( m_ppcWeightedTempCU[uhDepth] , 0 , uhDepth );
    rpcTempCU->setARPWSubParts( (UChar)nARPW , 0 , uhDepth );

    m_pcPredSearch->motionCompensation( rpcTempCU , m_ppcPredYuvTemp[uhDepth] );
  }
  else
  {
    bFirstTime = false;
#endif
#if AMP_MRG
  rpcTempCU->setMergeAMP (true);
#if  H_3D_FAST_TEXTURE_ENCODING
        m_pcPredSearch->predInterSearch ( rpcTempCU, m_ppcOrigYuv[uhDepth], m_ppcPredYuvTemp[uhDepth], m_ppcResiYuvTemp[uhDepth], m_ppcRecoYuvTemp[uhDepth], bFMD, false, bUseMRG );
#else
  m_pcPredSearch->predInterSearch ( rpcTempCU, m_ppcOrigYuv[uhDepth], m_ppcPredYuvTemp[uhDepth], m_ppcResiYuvTemp[uhDepth], m_ppcRecoYuvTemp[uhDepth] DEBUG_STRING_PASS_INTO(sTest), false, bUseMRG );
#endif

#else
  m_pcPredSearch->predInterSearch ( rpcTempCU, m_ppcOrigYuv[uhDepth], m_ppcPredYuvTemp[uhDepth], m_ppcResiYuvTemp[uhDepth], m_ppcRecoYuvTemp[uhDepth] );
#endif
#if NH_3D_ARP
  if( nARPWMax )
  {
    m_ppcWeightedTempCU[uhDepth]->copyPartFrom( rpcTempCU , 0 , uhDepth );
  }
  }
#endif

#if AMP_MRG
  if ( !rpcTempCU->getMergeAMP() )
  {
#if NH_3D_ARP
    if( nARPWMax )
    {
      continue;
    }
    else
#endif
    return;
  }
#endif
#if KWU_RC_MADPRED_E0227
      if ( m_pcEncCfg->getUseRateCtrl() && m_pcEncCfg->getLCULevelRC() && ePartSize == SIZE_2Nx2N && uhDepth <= m_addSADDepth )
      {
        UInt SAD = m_pcRdCost->getSADPart( g_bitDepthY, m_ppcPredYuvTemp[uhDepth]->getLumaAddr(), m_ppcPredYuvTemp[uhDepth]->getStride(),
          m_ppcOrigYuv[uhDepth]->getLumaAddr(), m_ppcOrigYuv[uhDepth]->getStride(),
          rpcTempCU->getWidth(0), rpcTempCU->getHeight(0) );
        m_temporalSAD = (Int)SAD;
      }
#endif

  m_pcPredSearch->encodeResAndCalcRdInterCU( rpcTempCU, m_ppcOrigYuv[uhDepth], m_ppcPredYuvTemp[uhDepth], m_ppcResiYuvTemp[uhDepth], m_ppcResiYuvBest[uhDepth], m_ppcRecoYuvTemp[uhDepth], false DEBUG_STRING_PASS_INTO(sTest) );
#if H_3D_VSP // possible bug fix
      if( rpcTempCU->getQtRootCbf(0)==0 )
      {
        rpcTempCU->setTrIdxSubParts(0, 0, uhDepth);
      }
#endif
#if NH_3D_VSO // M4
  if( m_pcRdCost->getUseLambdaScaleVSO() )
  {
    rpcTempCU->getTotalCost()  = m_pcRdCost->calcRdCostVSO( rpcTempCU->getTotalBits(), rpcTempCU->getTotalDistortion() );
  }
  else            
#endif
    rpcTempCU->getTotalCost()  = m_pcRdCost->calcRdCost( rpcTempCU->getTotalBits(), rpcTempCU->getTotalDistortion() );

#if DEBUG_STRING
  DebugInterPredResiReco(sTest, *(m_ppcPredYuvTemp[uhDepth]), *(m_ppcResiYuvBest[uhDepth]), *(m_ppcRecoYuvTemp[uhDepth]), DebugStringGetPredModeMask(rpcTempCU->getPredictionMode(0)));
#endif
#if H_3D_INTER_SDC
      TComDataCU *rpcTempCUPre = rpcTempCU;
#endif

  xCheckDQP( rpcTempCU );
  xCheckBestMode(rpcBestCU, rpcTempCU, uhDepth DEBUG_STRING_PASS_INTO(sDebug) DEBUG_STRING_PASS_INTO(sTest));
#if H_3D_INTER_SDC
      if( rpcTempCU->getSlice()->getInterSdcFlag() && ePartSize == SIZE_2Nx2N)
      {
        Double dOffsetCost[3] = {MAX_DOUBLE,MAX_DOUBLE,MAX_DOUBLE};
        for( Int uiOffest = 1 ; uiOffest <= 5 ; uiOffest++ )
        {
          if( uiOffest > 3)
          {
            if ( dOffsetCost[0] < (0.9*dOffsetCost[1]) && dOffsetCost[0] < (0.9*dOffsetCost[2]) )
            {
              continue;
            }
            if ( dOffsetCost[1] < dOffsetCost[0] && dOffsetCost[0] < dOffsetCost[2] &&  uiOffest == 5)
            {
              continue;
            }
            if ( dOffsetCost[0] < dOffsetCost[1] && dOffsetCost[2] < dOffsetCost[0] &&  uiOffest == 4)
            {
              continue;
            }
          }

          if( rpcTempCU != rpcTempCUPre )
          {
            Int orgQP = rpcBestCU->getQP( 0 );
            rpcTempCU->initEstData( uhDepth, orgQP ,bTransquantBypassFlag );      
            rpcTempCU->copyPartFrom( rpcBestCU, 0, uhDepth );
          }
          rpcTempCU->setSkipFlagSubParts( false, 0, uhDepth );
#if H_3D
          rpcTempCU->setDISFlagSubParts( false, 0, uhDepth );
#endif
          rpcTempCU->setTrIdxSubParts( 0, 0, uhDepth );
          rpcTempCU->setCbfSubParts( 1, 1, 1, 0, uhDepth );
#if NH_3D_VSO // M3
          if( m_pcRdCost->getUseRenModel() )
          {
            UInt  uiWidth     = m_ppcOrigYuv[uhDepth]->getWidth ( COMPONENT_Y );
            UInt  uiHeight    = m_ppcOrigYuv[uhDepth]->getHeight( COMPONENT_Y  );
            Pel*  piSrc       = m_ppcOrigYuv[uhDepth]->getAddr  ( COMPONENT_Y  );
            UInt  uiSrcStride = m_ppcOrigYuv[uhDepth]->getStride( COMPONENT_Y  );
            m_pcRdCost->setRenModelData( rpcTempCU, 0, piSrc, uiSrcStride, uiWidth, uiHeight );
          }
#endif

          Int iSdcOffset = 0;
          if(uiOffest % 2 == 0)
          {
            iSdcOffset = uiOffest >> 1;
          }
          else
          {
            iSdcOffset = -1 * (uiOffest >> 1);
          }
          m_pcPredSearch->encodeResAndCalcRdInterSDCCU( rpcTempCU, 
            m_ppcOrigYuv[uhDepth],
            ( rpcTempCU != rpcTempCUPre ) ? m_ppcPredYuvBest[uhDepth] : m_ppcPredYuvTemp[uhDepth],
            m_ppcResiYuvTemp[uhDepth],
            m_ppcRecoYuvTemp[uhDepth],
            iSdcOffset,
            uhDepth );
          if (uiOffest <= 3 )
          {
            dOffsetCost [uiOffest -1] = rpcTempCU->getTotalCost();
          }

          xCheckDQP( rpcTempCU );
          xCheckBestMode(rpcBestCU, rpcTempCU, uhDepth);
        }

      }
#endif
#if NH_3D_ARP
    }
#endif
#if  H_3D_FAST_TEXTURE_ENCODING
  }
#endif
}

#if NH_3D_DBBP
Void TEncCu::xInvalidateOriginalSegments( TComYuv* pOrigYuv, TComYuv* pOrigYuvTemp, Bool* pMask, UInt uiValidSegment )
{
  UInt  uiWidth     = pOrigYuv->getWidth (COMPONENT_Y);
  UInt  uiHeight    = pOrigYuv->getHeight(COMPONENT_Y);
  Pel*  piSrc       = pOrigYuv->getAddr(COMPONENT_Y);
  UInt  uiSrcStride = pOrigYuv->getStride(COMPONENT_Y);
  Pel*  piDst       = pOrigYuvTemp->getAddr(COMPONENT_Y);
  UInt  uiDstStride = pOrigYuvTemp->getStride(COMPONENT_Y);
  
  UInt  uiMaskStride= MAX_CU_SIZE;
  
  AOF( uiWidth == uiHeight );
  
  // backup pointer
  Bool* pMaskStart = pMask;
  
  for (Int y=0; y<uiHeight; y++)
  {
    for (Int x=0; x<uiWidth; x++)
    {
      UChar ucSegment = (UChar)pMask[x];
      AOF( ucSegment < 2 );
      
      piDst[x] = (ucSegment==uiValidSegment)?piSrc[x]:DBBP_INVALID_SHORT;
    }
    
    piSrc  += uiSrcStride;
    piDst  += uiDstStride;
    pMask  += uiMaskStride;
  }
  
  // now invalidate chroma
  Pel*  piSrcU       = pOrigYuv->getAddr(COMPONENT_Cb);
  Pel*  piSrcV       = pOrigYuv->getAddr(COMPONENT_Cr);
  UInt  uiSrcStrideC = pOrigYuv->getStride(COMPONENT_Cb);
  Pel*  piDstU       = pOrigYuvTemp->getAddr(COMPONENT_Cb);
  Pel*  piDstV       = pOrigYuvTemp->getAddr(COMPONENT_Cr);
  UInt  uiDstStrideC = pOrigYuvTemp->getStride(COMPONENT_Cb);
  pMask = pMaskStart;
  
  for (Int y=0; y<uiHeight/2; y++)
  {
    for (Int x=0; x<uiWidth/2; x++)
    {
      UChar ucSegment = (UChar)pMask[x*2];
      AOF( ucSegment < 2 );
      
      piDstU[x] = (ucSegment==uiValidSegment)?piSrcU[x]:DBBP_INVALID_SHORT;
      piDstV[x] = (ucSegment==uiValidSegment)?piSrcV[x]:DBBP_INVALID_SHORT;
    }
    
    piSrcU  += uiSrcStrideC;
    piSrcV  += uiSrcStrideC;
    piDstU  += uiDstStrideC;
    piDstV  += uiDstStrideC;
    pMask   += 2*uiMaskStride;
  }
}
#endif

#if H_3D_DIS
Void TEncCu::xCheckRDCostDIS( TComDataCU*& rpcBestCU, TComDataCU*& rpcTempCU, PartSize eSize )
{
  UInt uiDepth = rpcTempCU->getDepth( 0 );
  if( !rpcBestCU->getSlice()->getIsDepth() || (eSize != SIZE_2Nx2N))
  {
    return;
  }

#if NH_3D_VSO // M5
  if( m_pcRdCost->getUseRenModel() )
  {
    UInt  uiWidth     = m_ppcOrigYuv[uiDepth]->getWidth   ( COMPONENT_Y );
    UInt  uiHeight    = m_ppcOrigYuv[uiDepth]->getHeight  ( COMPONENT_Y );
    Pel*  piSrc       = m_ppcOrigYuv[uiDepth]->getAddr    ( COMPONENT_Y );
    UInt  uiSrcStride = m_ppcOrigYuv[uiDepth]->getStride  ( COMPONENT_Y );
    m_pcRdCost->setRenModelData( rpcTempCU, 0, piSrc, uiSrcStride, uiWidth, uiHeight );
  }
#endif

  rpcTempCU->setSkipFlagSubParts( false, 0, uiDepth );
  rpcTempCU->setPartSizeSubParts( SIZE_2Nx2N, 0, uiDepth );
  rpcTempCU->setPredModeSubParts( MODE_INTRA, 0, uiDepth );
  rpcTempCU->setCUTransquantBypassSubParts( rpcTempCU->getCUTransquantBypass(0), 0, uiDepth );

  rpcTempCU->setTrIdxSubParts(0, 0, uiDepth);
  rpcTempCU->setCbfSubParts(0, 1, 1, 0, uiDepth);
  rpcTempCU->setDISFlagSubParts(true, 0, uiDepth);
  rpcTempCU->setLumaIntraDirSubParts (DC_IDX, 0, uiDepth);
#if H_3D_DIM_SDC
  rpcTempCU->setSDCFlagSubParts( false, 0, uiDepth);
#endif

  UInt uiPreCalcDistC;
  m_pcPredSearch  ->estIntraPredDIS      ( rpcTempCU, m_ppcOrigYuv[uiDepth], m_ppcPredYuvTemp[uiDepth], m_ppcResiYuvTemp[uiDepth], m_ppcRecoYuvTemp[uiDepth], uiPreCalcDistC, false );

#if ENC_DEC_TRACE && H_MV_ENC_DEC_TRAC
  Int oldTraceCopyBack = g_traceCopyBack; 
  g_traceCopyBack = false;  
#endif
  m_ppcRecoYuvTemp[uiDepth]->copyToPicLuma(rpcTempCU->getPic()->getPicYuvRec(), rpcTempCU->getAddr(), rpcTempCU->getZorderIdxInCU() );
  #if ENC_DEC_TRACE && H_MV_ENC_DEC_TRAC  
    g_traceCopyBack = oldTraceCopyBack; 
  #endif


  m_pcEntropyCoder->resetBits();
  if ( rpcTempCU->getSlice()->getPPS()->getTransquantBypassEnableFlag())
  {
    m_pcEntropyCoder->encodeCUTransquantBypassFlag( rpcTempCU, 0,          true );
  }
  m_pcEntropyCoder->encodeSkipFlag ( rpcTempCU, 0,          true );
  m_pcEntropyCoder->encodeDIS( rpcTempCU, 0,          true );


  m_pcRDGoOnSbacCoder->store(m_pppcRDSbacCoder[uiDepth][CI_TEMP_BEST]);

  rpcTempCU->getTotalBits() = m_pcEntropyCoder->getNumberOfWrittenBits();
  rpcTempCU->getTotalBins() = ((TEncBinCABAC *)((TEncSbac*)m_pcEntropyCoder->m_pcEntropyCoderIf)->getEncBinIf())->getBinsCoded();


#if NH_3D_VSO // M6
  if( m_pcRdCost->getUseLambdaScaleVSO())
  {
    rpcTempCU->getTotalCost() = m_pcRdCost->calcRdCostVSO( rpcTempCU->getTotalBits(), rpcTempCU->getTotalDistortion() );  
  }
  else
#endif
    rpcTempCU->getTotalCost() = m_pcRdCost->calcRdCost( rpcTempCU->getTotalBits(), rpcTempCU->getTotalDistortion() );


  xCheckDQP( rpcTempCU );
  xCheckBestMode(rpcBestCU, rpcTempCU, uiDepth);
}
#endif

#if NH_3D_DBBP
Void TEncCu::xCheckRDCostInterDBBP( TComDataCU*& rpcBestCU, TComDataCU*& rpcTempCU, Bool bUseMRG )
{
  AOF( !rpcTempCU->getSlice()->getIsDepth() );
  
  UChar uhDepth = rpcTempCU->getDepth( 0 );
  
#if NH_3D_VSO
  if( m_pcRdCost->getUseRenModel() )
  {
    UInt  uiWidth     = m_ppcOrigYuv[uhDepth]->getWidth ( COMPONENT_Y );
    UInt  uiHeight    = m_ppcOrigYuv[uhDepth]->getHeight( COMPONENT_Y );
    Pel*  piSrc       = m_ppcOrigYuv[uhDepth]->getAddr  ( COMPONENT_Y );
    UInt  uiSrcStride = m_ppcOrigYuv[uhDepth]->getStride( COMPONENT_Y );
    m_pcRdCost->setRenModelData( rpcTempCU, 0, piSrc, uiSrcStride, uiWidth, uiHeight );
  }
#endif
  
  UInt uiWidth  = rpcTempCU->getWidth(0);
  UInt uiHeight = rpcTempCU->getHeight(0);
  AOF( uiWidth == uiHeight );
  
#if NH_3D_DBBP
  if(uiWidth <= 8)
  {
    return;
  }
#endif
  
  rpcTempCU->setPartSizeSubParts( SIZE_2Nx2N,  0, uhDepth );
  
  // fetch virtual depth block
  UInt uiDepthStride = 0;
#if H_3D_FCO
  Pel* pDepthPels = rpcTempCU->getVirtualDepthBlock(rpcTempCU->getZorderIdxInCU(), uiWidth, uiHeight, uiDepthStride);
#else
  Pel* pDepthPels = rpcTempCU->getVirtualDepthBlock(0, uiWidth, uiHeight, uiDepthStride);
#endif
  AOF( pDepthPels != NULL );
  AOF( uiDepthStride != 0 );
  
  PartSize eVirtualPartSize = m_pcPredSearch->getPartitionSizeFromDepth(pDepthPels, uiDepthStride, uiWidth, rpcTempCU);

  // derive partitioning from depth
  Bool pMask[MAX_CU_SIZE*MAX_CU_SIZE];
  Bool bValidMask = m_pcPredSearch->getSegmentMaskFromDepth(pDepthPels, uiDepthStride, uiWidth, uiHeight, pMask, rpcTempCU);
  
  if( !bValidMask )
  {
    return;
  }
  
  // find optimal motion/disparity vector for each segment
  DisInfo originalDvInfo = rpcTempCU->getDvInfo(0);
  DbbpTmpData* pDBBPTmpData = rpcTempCU->getDBBPTmpData();
  TComYuv* apPredYuv[2] = { m_ppcRecoYuvTemp[uhDepth], m_ppcPredYuvTemp[uhDepth] };
  
  // find optimal motion vector fields for both segments (as 2Nx2N)
  rpcTempCU->setDepthSubParts( uhDepth, 0 );
  rpcTempCU->setPartSizeSubParts( SIZE_2Nx2N,  0, uhDepth );
  rpcTempCU->setPredModeSubParts( MODE_INTER, 0, uhDepth );
  for( UInt uiSegment = 0; uiSegment < 2; uiSegment++ )
  {
    rpcTempCU->setDBBPFlagSubParts(true, 0, 0, uhDepth);
    rpcTempCU->setDvInfoSubParts(originalDvInfo, 0, uhDepth);
    
    // invalidate all other segments in original YUV
    xInvalidateOriginalSegments(m_ppcOrigYuv[uhDepth], m_ppcOrigYuvDBBP[uhDepth], pMask, uiSegment);
    
    // do motion estimation for this segment
    m_pcRdCost->setUseMask(true);
    rpcTempCU->getDBBPTmpData()->eVirtualPartSize = eVirtualPartSize;
    rpcTempCU->getDBBPTmpData()->uiVirtualPartIndex = uiSegment;

    m_pcPredSearch->predInterSearch( rpcTempCU, m_ppcOrigYuvDBBP[uhDepth], apPredYuv[uiSegment], m_ppcResiYuvTemp[uhDepth], m_ppcResiYuvTemp[uhDepth], false, bUseMRG );
    m_pcRdCost->setUseMask(false);
    
    // extract motion parameters of full block for this segment
    pDBBPTmpData->auhInterDir[uiSegment] = rpcTempCU->getInterDir(0);
    
    pDBBPTmpData->abMergeFlag[uiSegment] = rpcTempCU->getMergeFlag(0);
    pDBBPTmpData->auhMergeIndex[uiSegment] = rpcTempCU->getMergeIndex(0);

#if NH_3D_VSP
    AOF( rpcTempCU->getSPIVMPFlag(0) == false );
    AOF( rpcTempCU->getVSPFlag(0) == 0 );
#endif
    
    for ( UInt uiRefListIdx = 0; uiRefListIdx < 2; uiRefListIdx++ )
    {
      RefPicList eRefList = (RefPicList)uiRefListIdx;
      
      pDBBPTmpData->acMvd[uiSegment][eRefList] = rpcTempCU->getCUMvField(eRefList)->getMvd(0);
      pDBBPTmpData->aiMvpNum[uiSegment][eRefList] = rpcTempCU->getMVPNum(eRefList, 0);
      pDBBPTmpData->aiMvpIdx[uiSegment][eRefList] = rpcTempCU->getMVPIdx(eRefList, 0);
      
      rpcTempCU->getMvField(rpcTempCU, 0, eRefList, pDBBPTmpData->acMvField[uiSegment][eRefList]);
    }
  }
  
  // store final motion/disparity information in each PU using derived partitioning
  rpcTempCU->setDepthSubParts( uhDepth, 0 );
  rpcTempCU->setPartSizeSubParts  ( eVirtualPartSize,  0, uhDepth );
  rpcTempCU->setPredModeSubParts  ( MODE_INTER, 0, uhDepth );
  
  UInt uiPUOffset = ( g_auiPUOffset[UInt( eVirtualPartSize )] << ( ( rpcTempCU->getSlice()->getSPS()->getMaxTotalCUDepth() - uhDepth ) << 1 ) ) >> 4;
  for( UInt uiSegment = 0; uiSegment < 2; uiSegment++ )
  {
    UInt uiPartAddr = uiSegment*uiPUOffset;
    
    rpcTempCU->setDBBPFlagSubParts(true, uiPartAddr, uiSegment, uhDepth);
    
    // now set stored information from 2Nx2N motion search to each partition
    rpcTempCU->setInterDirSubParts(pDBBPTmpData->auhInterDir[uiSegment], uiPartAddr, uiSegment, uhDepth); // interprets depth relative to LCU level
    
    rpcTempCU->setMergeFlagSubParts(pDBBPTmpData->abMergeFlag[uiSegment], uiPartAddr, uiSegment, uhDepth);
    rpcTempCU->setMergeIndexSubParts(pDBBPTmpData->auhMergeIndex[uiSegment], uiPartAddr, uiSegment, uhDepth);
        
    for ( UInt uiRefListIdx = 0; uiRefListIdx < 2; uiRefListIdx++ )
    {
      RefPicList eRefList = (RefPicList)uiRefListIdx;
      
      rpcTempCU->getCUMvField( eRefList )->setAllMvd(pDBBPTmpData->acMvd[uiSegment][eRefList], eVirtualPartSize, uiPartAddr, 0, uiSegment);
      rpcTempCU->setMVPNum(eRefList, uiPartAddr, pDBBPTmpData->aiMvpNum[uiSegment][eRefList]);
      rpcTempCU->setMVPIdx(eRefList, uiPartAddr, pDBBPTmpData->aiMvpIdx[uiSegment][eRefList]);
      
      rpcTempCU->getCUMvField( eRefList )->setAllMvField( pDBBPTmpData->acMvField[uiSegment][eRefList], eVirtualPartSize, uiPartAddr, 0, uiSegment ); // interprets depth relative to rpcTempCU level
    }
  }
  
  // reconstruct final prediction signal by combining both segments
  Int bitDepthY = rpcTempCU->getSlice()->getSPS()->getBitDepth(CHANNEL_TYPE_LUMA);
  m_pcPredSearch->combineSegmentsWithMask(apPredYuv, m_ppcPredYuvTemp[uhDepth], pMask, uiWidth, uiHeight, 0, eVirtualPartSize, bitDepthY);
  m_pcPredSearch->encodeResAndCalcRdInterCU( rpcTempCU, m_ppcOrigYuv[uhDepth], m_ppcPredYuvTemp[uhDepth], m_ppcResiYuvTemp[uhDepth], m_ppcResiYuvBest[uhDepth], m_ppcRecoYuvTemp[uhDepth], false );
  
  xCheckDQP( rpcTempCU );
  xCheckBestMode(rpcBestCU, rpcTempCU, uhDepth);
}
#endif

#if H_3D_DIM
Void TEncCu::xCheckRDCostIntra( TComDataCU*& rpcBestCU, TComDataCU*& rpcTempCU, PartSize eSize, Bool bOnlyIVP )
#else
Void TEncCu::xCheckRDCostIntra( TComDataCU *&rpcBestCU,
                                TComDataCU *&rpcTempCU,
                                Double      &cost,
                                PartSize     eSize
                                DEBUG_STRING_FN_DECLARE(sDebug) )
#endif
{
  DEBUG_STRING_NEW(sTest)

  UInt uiDepth = rpcTempCU->getDepth( 0 );
#if NH_3D_VSO // M5
  if( m_pcRdCost->getUseRenModel() )
  {
    UInt  uiWidth     = m_ppcOrigYuv[uiDepth]->getWidth   ( COMPONENT_Y );
    UInt  uiHeight    = m_ppcOrigYuv[uiDepth]->getHeight  ( COMPONENT_Y );
    Pel*  piSrc       = m_ppcOrigYuv[uiDepth]->getAddr    ( COMPONENT_Y );
    UInt  uiSrcStride = m_ppcOrigYuv[uiDepth]->getStride  ( COMPONENT_Y );
    m_pcRdCost->setRenModelData( rpcTempCU, 0, piSrc, uiSrcStride, uiWidth, uiHeight );
  }
#endif

  rpcTempCU->setSkipFlagSubParts( false, 0, uiDepth );
#if H_3D
  rpcTempCU->setDISFlagSubParts( false, 0, uiDepth );
#endif


  rpcTempCU->setPartSizeSubParts( eSize, 0, uiDepth );
  rpcTempCU->setPredModeSubParts( MODE_INTRA, 0, uiDepth );
  rpcTempCU->setChromaQpAdjSubParts( rpcTempCU->getCUTransquantBypass(0) ? 0 : m_cuChromaQpOffsetIdxPlus1, 0, uiDepth );

  Pel resiLuma[NUMBER_OF_STORED_RESIDUAL_TYPES][MAX_CU_SIZE * MAX_CU_SIZE];

  m_pcPredSearch->estIntraPredLumaQT( rpcTempCU, m_ppcOrigYuv[uiDepth], m_ppcPredYuvTemp[uiDepth], m_ppcResiYuvTemp[uiDepth], m_ppcRecoYuvTemp[uiDepth], resiLuma DEBUG_STRING_PASS_INTO(sTest) );

  m_ppcRecoYuvTemp[uiDepth]->copyToPicComponent(COMPONENT_Y, rpcTempCU->getPic()->getPicYuvRec(), rpcTempCU->getCtuRsAddr(), rpcTempCU->getZorderIdxInCtu() );

  if (rpcBestCU->getPic()->getChromaFormat()!=CHROMA_400)
  {
#if H_3D_DIM
    m_pcPredSearch->estIntraPredQT      ( rpcTempCU, m_ppcOrigYuv[uiDepth], m_ppcPredYuvTemp[uiDepth], m_ppcResiYuvTemp[uiDepth], m_ppcRecoYuvTemp[uiDepth], uiPreCalcDistC, bSeparateLumaChroma, bOnlyIVP );
#else
    m_pcPredSearch->estIntraPredChromaQT( rpcTempCU, m_ppcOrigYuv[uiDepth], m_ppcPredYuvTemp[uiDepth], m_ppcResiYuvTemp[uiDepth], m_ppcRecoYuvTemp[uiDepth], resiLuma DEBUG_STRING_PASS_INTO(sTest) );
#endif
  }
#if H_3D_DIM_SDC
  if( !rpcTempCU->getSDCFlag( 0 ) )
#endif

  m_pcEntropyCoder->resetBits();

  if ( rpcTempCU->getSlice()->getPPS()->getTransquantBypassEnableFlag())
  {
    m_pcEntropyCoder->encodeCUTransquantBypassFlag( rpcTempCU, 0,          true );
  }
#if H_3D
  m_pcEntropyCoder->encodeDIS( rpcTempCU, 0,          true );
  if(!rpcTempCU->getDISFlag(0))
  {
#endif
  m_pcEntropyCoder->encodeSkipFlag ( rpcTempCU, 0,          true );
  m_pcEntropyCoder->encodePredMode( rpcTempCU, 0,          true );
  m_pcEntropyCoder->encodePartSize( rpcTempCU, 0, uiDepth, true );
  m_pcEntropyCoder->encodePredInfo( rpcTempCU, 0 );
  m_pcEntropyCoder->encodeIPCMInfo(rpcTempCU, 0, true );
#if H_3D_DIM_SDC
    m_pcEntropyCoder->encodeSDCFlag( rpcTempCU, 0, true );
#endif

  // Encode Coefficients
  Bool bCodeDQP = getdQPFlag();
  Bool codeChromaQpAdjFlag = getCodeChromaQpAdjFlag();
  m_pcEntropyCoder->encodeCoeff( rpcTempCU, 0, uiDepth, bCodeDQP, codeChromaQpAdjFlag );
  setCodeChromaQpAdjFlag( codeChromaQpAdjFlag );
  setdQPFlag( bCodeDQP );
#if H_3D
  }
#endif
  m_pcRDGoOnSbacCoder->store(m_pppcRDSbacCoder[uiDepth][CI_TEMP_BEST]);

  rpcTempCU->getTotalBits() = m_pcEntropyCoder->getNumberOfWrittenBits();
  rpcTempCU->getTotalBins() = ((TEncBinCABAC *)((TEncSbac*)m_pcEntropyCoder->m_pcEntropyCoderIf)->getEncBinIf())->getBinsCoded();
#if NH_3D_VSO // M6
  if( m_pcRdCost->getUseLambdaScaleVSO())  
  {
    rpcTempCU->getTotalCost() = m_pcRdCost->calcRdCostVSO( rpcTempCU->getTotalBits(), rpcTempCU->getTotalDistortion() );  
  }
  else
#endif
    rpcTempCU->getTotalCost() = m_pcRdCost->calcRdCost( rpcTempCU->getTotalBits(), rpcTempCU->getTotalDistortion() );

  xCheckDQP( rpcTempCU );

  cost = rpcTempCU->getTotalCost();

  xCheckBestMode(rpcBestCU, rpcTempCU, uiDepth DEBUG_STRING_PASS_INTO(sDebug) DEBUG_STRING_PASS_INTO(sTest));
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

#if NH_3D_VSO // VERY NEW
  if( m_pcRdCost->getUseRenModel() )
  {
    UInt  uiWidth     = m_ppcOrigYuv[uiDepth]->getWidth   ( COMPONENT_Y );
    UInt  uiHeight    = m_ppcOrigYuv[uiDepth]->getHeight  ( COMPONENT_Y );
    Pel*  piSrc       = m_ppcOrigYuv[uiDepth]->getAddr    ( COMPONENT_Y );
    UInt  uiSrcStride = m_ppcOrigYuv[uiDepth]->getStride  ( COMPONENT_Y );
    m_pcRdCost->setRenModelData( rpcTempCU, 0, piSrc, uiSrcStride, uiWidth, uiHeight );
  }
#endif

  rpcTempCU->setSkipFlagSubParts( false, 0, uiDepth );
#if H_3D
  rpcTempCU->setDISFlagSubParts( false, 0, uiDepth );
#endif
  rpcTempCU->setIPCMFlag(0, true);
  rpcTempCU->setIPCMFlagSubParts (true, 0, rpcTempCU->getDepth(0));
  rpcTempCU->setPartSizeSubParts( SIZE_2Nx2N, 0, uiDepth );
  rpcTempCU->setPredModeSubParts( MODE_INTRA, 0, uiDepth );
  rpcTempCU->setTrIdxSubParts ( 0, 0, uiDepth );
  rpcTempCU->setChromaQpAdjSubParts( rpcTempCU->getCUTransquantBypass(0) ? 0 : m_cuChromaQpOffsetIdxPlus1, 0, uiDepth );

  m_pcPredSearch->IPCMSearch( rpcTempCU, m_ppcOrigYuv[uiDepth], m_ppcPredYuvTemp[uiDepth], m_ppcResiYuvTemp[uiDepth], m_ppcRecoYuvTemp[uiDepth]);

  m_pcRDGoOnSbacCoder->load(m_pppcRDSbacCoder[uiDepth][CI_CURR_BEST]);

  m_pcEntropyCoder->resetBits();

  if ( rpcTempCU->getSlice()->getPPS()->getTransquantBypassEnableFlag())
  {
    m_pcEntropyCoder->encodeCUTransquantBypassFlag( rpcTempCU, 0,          true );
  }

  m_pcEntropyCoder->encodeSkipFlag ( rpcTempCU, 0,          true );
#if H_3D
  m_pcEntropyCoder->encodeDIS( rpcTempCU, 0,          true );
#endif
  m_pcEntropyCoder->encodePredMode ( rpcTempCU, 0,          true );
  m_pcEntropyCoder->encodePartSize ( rpcTempCU, 0, uiDepth, true );
  m_pcEntropyCoder->encodeIPCMInfo ( rpcTempCU, 0, true );
#if H_3D_DIM_SDC
  m_pcEntropyCoder->encodeSDCFlag( rpcTempCU, 0, true );
#endif
  m_pcRDGoOnSbacCoder->store(m_pppcRDSbacCoder[uiDepth][CI_TEMP_BEST]);

  rpcTempCU->getTotalBits() = m_pcEntropyCoder->getNumberOfWrittenBits();
  rpcTempCU->getTotalBins() = ((TEncBinCABAC *)((TEncSbac*)m_pcEntropyCoder->m_pcEntropyCoderIf)->getEncBinIf())->getBinsCoded();
#if NH_3D_VSO // M44
  if ( m_pcRdCost->getUseVSO() )
  {
    rpcTempCU->getTotalCost() = m_pcRdCost->calcRdCostVSO( rpcTempCU->getTotalBits(), rpcTempCU->getTotalDistortion() );
  }
  else
#endif
    rpcTempCU->getTotalCost() = m_pcRdCost->calcRdCost( rpcTempCU->getTotalBits(), rpcTempCU->getTotalDistortion() );

  xCheckDQP( rpcTempCU );
  DEBUG_STRING_NEW(a)
  DEBUG_STRING_NEW(b)
  xCheckBestMode(rpcBestCU, rpcTempCU, uiDepth DEBUG_STRING_PASS_INTO(a) DEBUG_STRING_PASS_INTO(b));
}

/** check whether current try is the best with identifying the depth of current try
 * \param rpcBestCU
 * \param rpcTempCU
 * \param uiDepth
 */
Void TEncCu::xCheckBestMode( TComDataCU*& rpcBestCU, TComDataCU*& rpcTempCU, UInt uiDepth DEBUG_STRING_FN_DECLARE(sParent) DEBUG_STRING_FN_DECLARE(sTest) DEBUG_STRING_PASS_INTO(Bool bAddSizeInfo) )
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

    // store temp best CI for next CU coding
    m_pppcRDSbacCoder[uiDepth][CI_TEMP_BEST]->store(m_pppcRDSbacCoder[uiDepth][CI_NEXT_BEST]);


#if DEBUG_STRING
    DEBUG_STRING_SWAP(sParent, sTest)
    const PredMode predMode=rpcBestCU->getPredictionMode(0);
    if ((DebugOptionList::DebugString_Structure.getInt()&DebugStringGetPredModeMask(predMode)) && bAddSizeInfo)
    {
      std::stringstream ss(stringstream::out);
      ss <<"###: " << (predMode==MODE_INTRA?"Intra   ":"Inter   ") << partSizeToString[rpcBestCU->getPartitionSize(0)] << " CU at " << rpcBestCU->getCUPelX() << ", " << rpcBestCU->getCUPelY() << " width=" << UInt(rpcBestCU->getWidth(0)) << std::endl;
      sParent+=ss.str();
    }
#endif
  }
}

Void TEncCu::xCheckDQP( TComDataCU* pcCU )
{
  UInt uiDepth = pcCU->getDepth( 0 );

  const TComPPS &pps = *(pcCU->getSlice()->getPPS());
  if ( pps.getUseDQP() && uiDepth <= pps.getMaxCuDQPDepth() )
  {
    if ( pcCU->getQtRootCbf( 0) )
    {
      m_pcEntropyCoder->resetBits();
      m_pcEntropyCoder->encodeQP( pcCU, 0, false );
      pcCU->getTotalBits() += m_pcEntropyCoder->getNumberOfWrittenBits(); // dQP bits
      pcCU->getTotalBins() += ((TEncBinCABAC *)((TEncSbac*)m_pcEntropyCoder->m_pcEntropyCoderIf)->getEncBinIf())->getBinsCoded();
#if NH_3D_VSO // M45
      if ( m_pcRdCost->getUseVSO() )      
      {
        pcCU->getTotalCost() = m_pcRdCost->calcRdCostVSO( pcCU->getTotalBits(), pcCU->getTotalDistortion() );      
      }
      else
#endif
        pcCU->getTotalCost() = m_pcRdCost->calcRdCost( pcCU->getTotalBits(), pcCU->getTotalDistortion() );
    }
    else
    {
      pcCU->setQPSubParts( pcCU->getRefQP( 0 ), 0, uiDepth ); // set QP to default QP
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
Void TEncCu::xCopyYuv2Pic(TComPic* rpcPic, UInt uiCUAddr, UInt uiAbsPartIdx, UInt uiDepth, UInt uiSrcDepth )
{
  UInt uiAbsPartIdxInRaster = g_auiZscanToRaster[uiAbsPartIdx];
  UInt uiSrcBlkWidth = rpcPic->getNumPartInCtuWidth() >> (uiSrcDepth);
  UInt uiBlkWidth    = rpcPic->getNumPartInCtuWidth() >> (uiDepth);
  UInt uiPartIdxX = ( ( uiAbsPartIdxInRaster % rpcPic->getNumPartInCtuWidth() ) % uiSrcBlkWidth) / uiBlkWidth;
  UInt uiPartIdxY = ( ( uiAbsPartIdxInRaster / rpcPic->getNumPartInCtuWidth() ) % uiSrcBlkWidth) / uiBlkWidth;
  UInt uiPartIdx = uiPartIdxY * ( uiSrcBlkWidth / uiBlkWidth ) + uiPartIdxX;
  m_ppcRecoYuvBest[uiSrcDepth]->copyToPicYuv( rpcPic->getPicYuvRec (), uiCUAddr, uiAbsPartIdx, uiDepth - uiSrcDepth, uiPartIdx);

  m_ppcPredYuvBest[uiSrcDepth]->copyToPicYuv( rpcPic->getPicYuvPred (), uiCUAddr, uiAbsPartIdx, uiDepth - uiSrcDepth, uiPartIdx);
}

Void TEncCu::xCopyYuv2Tmp( UInt uiPartUnitIdx, UInt uiNextDepth )
{
  UInt uiCurrDepth = uiNextDepth - 1;
  m_ppcRecoYuvBest[uiNextDepth]->copyToPartYuv( m_ppcRecoYuvTemp[uiCurrDepth], uiPartUnitIdx );
  m_ppcPredYuvBest[uiNextDepth]->copyToPartYuv( m_ppcPredYuvBest[uiCurrDepth], uiPartUnitIdx);
}

/** Function for filling the PCM buffer of a CU using its original sample array
 * \param pCU pointer to current CU
 * \param pOrgYuv pointer to original sample array
 */
Void TEncCu::xFillPCMBuffer     ( TComDataCU* pCU, TComYuv* pOrgYuv )
{
  const ChromaFormat format = pCU->getPic()->getChromaFormat();
  const UInt numberValidComponents = getNumberValidComponents(format);
  for (UInt componentIndex = 0; componentIndex < numberValidComponents; componentIndex++)
  {
    const ComponentID component = ComponentID(componentIndex);

    const UInt width  = pCU->getWidth(0)  >> getComponentScaleX(component, format);
    const UInt height = pCU->getHeight(0) >> getComponentScaleY(component, format);

    Pel *source      = pOrgYuv->getAddr(component, 0, width);
    Pel *destination = pCU->getPCMSample(component);

    const UInt sourceStride = pOrgYuv->getStride(component);

    for (Int line = 0; line < height; line++)
    {
      for (Int column = 0; column < width; column++)
      {
        destination[column] = source[column];
      }

      source      += sourceStride;
      destination += width;
    }
  }
}

#if ADAPTIVE_QP_SELECTION
/** Collect ARL statistics from one block
  */
Int TEncCu::xTuCollectARLStats(TCoeff* rpcCoeff, TCoeff* rpcArlCoeff, Int NumCoeffInCU, Double* cSum, UInt* numSamples )
{
  for( Int n = 0; n < NumCoeffInCU; n++ )
  {
    TCoeff u = abs( rpcCoeff[ n ] );
    TCoeff absc = rpcArlCoeff[ n ];

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

//! Collect ARL statistics from one CTU
Void TEncCu::xCtuCollectARLStats(TComDataCU* pCtu )
{
  Double cSum[ LEVEL_RANGE + 1 ];     //: the sum of DCT coefficients corresponding to data type and quantization output
  UInt numSamples[ LEVEL_RANGE + 1 ]; //: the number of coefficients corresponding to data type and quantization output

  TCoeff* pCoeffY = pCtu->getCoeff(COMPONENT_Y);
  TCoeff* pArlCoeffY = pCtu->getArlCoeff(COMPONENT_Y);
  const TComSPS &sps = *(pCtu->getSlice()->getSPS());

  const UInt uiMinCUWidth = sps.getMaxCUWidth() >> sps.getMaxTotalCUDepth(); // NOTE: ed - this is not the minimum CU width. It is the square-root of the number of coefficients per part.
  const UInt uiMinNumCoeffInCU = 1 << uiMinCUWidth;                          // NOTE: ed - what is this?

  memset( cSum, 0, sizeof( Double )*(LEVEL_RANGE+1) );
  memset( numSamples, 0, sizeof( UInt )*(LEVEL_RANGE+1) );

  // Collect stats to cSum[][] and numSamples[][]
  for(Int i = 0; i < pCtu->getTotalNumPart(); i ++ )
  {
    UInt uiTrIdx = pCtu->getTransformIdx(i);

    if(pCtu->isInter(i) && pCtu->getCbf( i, COMPONENT_Y, uiTrIdx ) )
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

//! \}
