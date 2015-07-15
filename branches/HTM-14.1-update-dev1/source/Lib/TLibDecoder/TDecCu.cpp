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

/** \file     TDecCu.cpp
    \brief    CU decoder class
*/

#include "TDecCu.h"
#include "TLibCommon/TComTU.h"
#include "TLibCommon/TComPrediction.h"

//! \ingroup TLibDecoder
//! \{

// ====================================================================================================================
// Constructor / destructor / create / destroy
// ====================================================================================================================

TDecCu::TDecCu()
{
  m_ppcYuvResi = NULL;
  m_ppcYuvReco = NULL;
  m_ppcCU      = NULL;
#if H_3D_DBBP
  m_ppcYuvRecoDBBP = NULL;
#endif
}

TDecCu::~TDecCu()
{
}

Void TDecCu::init( TDecEntropy* pcEntropyDecoder, TComTrQuant* pcTrQuant, TComPrediction* pcPrediction)
{
  m_pcEntropyDecoder  = pcEntropyDecoder;
  m_pcTrQuant         = pcTrQuant;
  m_pcPrediction      = pcPrediction;
}

/**
 \param    uiMaxDepth      total number of allowable depth
 \param    uiMaxWidth      largest CU width
 \param    uiMaxHeight     largest CU height
 \param    chromaFormatIDC chroma format
 */
Void TDecCu::create( UInt uiMaxDepth, UInt uiMaxWidth, UInt uiMaxHeight, ChromaFormat chromaFormatIDC )
{
  m_uiMaxDepth = uiMaxDepth+1;

  m_ppcYuvResi = new TComYuv*[m_uiMaxDepth-1];
  m_ppcYuvReco = new TComYuv*[m_uiMaxDepth-1];
  m_ppcCU      = new TComDataCU*[m_uiMaxDepth-1];
#if H_3D_DBBP
  m_ppcYuvRecoDBBP = new TComYuv*[m_uiMaxDepth-1];
#endif

  UInt uiNumPartitions;
  for ( UInt ui = 0; ui < m_uiMaxDepth-1; ui++ )
  {
    uiNumPartitions = 1<<( ( m_uiMaxDepth - ui - 1 )<<1 );
    UInt uiWidth  = uiMaxWidth  >> ui;
    UInt uiHeight = uiMaxHeight >> ui;

    m_ppcYuvResi[ui] = new TComYuv;    m_ppcYuvResi[ui]->create( uiWidth, uiHeight, chromaFormatIDC );
    m_ppcYuvReco[ui] = new TComYuv;    m_ppcYuvReco[ui]->create( uiWidth, uiHeight, chromaFormatIDC );
    m_ppcCU     [ui] = new TComDataCU; m_ppcCU     [ui]->create( chromaFormatIDC, uiNumPartitions, uiWidth, uiHeight, true, uiMaxWidth >> (m_uiMaxDepth - 1) );
#if H_3D_DBBP
    m_ppcYuvRecoDBBP[ui] = new TComYuv;    m_ppcYuvRecoDBBP[ui]->create( uiWidth, uiHeight );
#endif
}

  m_bDecodeDQP = false;
  m_IsChromaQpAdjCoded = false;

  // initialize partition order.
  UInt* piTmp = &g_auiZscanToRaster[0];
  initZscanToRaster(m_uiMaxDepth, 1, 0, piTmp);
  initRasterToZscan( uiMaxWidth, uiMaxHeight, m_uiMaxDepth );

  // initialize conversion matrix from partition index to pel
  initRasterToPelXY( uiMaxWidth, uiMaxHeight, m_uiMaxDepth );
}

Void TDecCu::destroy()
{
  for ( UInt ui = 0; ui < m_uiMaxDepth-1; ui++ )
  {
    m_ppcYuvResi[ui]->destroy(); delete m_ppcYuvResi[ui]; m_ppcYuvResi[ui] = NULL;
    m_ppcYuvReco[ui]->destroy(); delete m_ppcYuvReco[ui]; m_ppcYuvReco[ui] = NULL;
    m_ppcCU     [ui]->destroy(); delete m_ppcCU     [ui]; m_ppcCU     [ui] = NULL;
#if H_3D_DBBP
    m_ppcYuvRecoDBBP[ui]->destroy(); delete m_ppcYuvRecoDBBP[ui]; m_ppcYuvRecoDBBP[ui] = NULL;
#endif
  }

  delete [] m_ppcYuvResi; m_ppcYuvResi = NULL;
  delete [] m_ppcYuvReco; m_ppcYuvReco = NULL;
  delete [] m_ppcCU     ; m_ppcCU      = NULL;
#if H_3D_DBBP
  delete [] m_ppcYuvRecoDBBP; m_ppcYuvRecoDBBP = NULL;
#endif
}

// ====================================================================================================================
// Public member functions
// ====================================================================================================================

/** 
 Parse a CTU.
 \param    pCtu                      [in/out] pointer to CTU data structure
 \param    isLastCtuOfSliceSegment   [out]    true, if last CTU of the slice segment
 */
Void TDecCu::decodeCtu( TComDataCU* pCtu, Bool& isLastCtuOfSliceSegment )
{
  if ( pCtu->getSlice()->getPPS()->getUseDQP() )
  {
    setdQPFlag(true);
  }

  if ( pCtu->getSlice()->getUseChromaQpAdj() )
  {
    setIsChromaQpAdjCoded(true);
  }

  // start from the top level CU
  xDecodeCU( pCtu, 0, 0, isLastCtuOfSliceSegment);
}

/** 
 Decoding process for a CTU.
 \param    pCtu                      [in/out] pointer to CTU data structure
 */
Void TDecCu::decompressCtu( TComDataCU* pCtu )
{
#if !H_3D_IV_MERGE
  xDecompressCU( pCtu, 0,  0 );
#endif
}

// ====================================================================================================================
// Protected member functions
// ====================================================================================================================

//! decode end-of-slice flag
Bool TDecCu::xDecodeSliceEnd( TComDataCU* pcCU, UInt uiAbsPartIdx )
{
  UInt uiIsLastCtuOfSliceSegment;

  if (pcCU->isLastSubCUOfCtu(uiAbsPartIdx))
  {
    m_pcEntropyDecoder->decodeTerminatingBit( uiIsLastCtuOfSliceSegment );
  }
  else
  {
    uiIsLastCtuOfSliceSegment=0;
  }

  return uiIsLastCtuOfSliceSegment>0;
}

//! decode CU block recursively
Void TDecCu::xDecodeCU( TComDataCU*const pcCU, const UInt uiAbsPartIdx, const UInt uiDepth, Bool &isLastCtuOfSliceSegment)
{
  TComPic* pcPic        = pcCU->getPic();
  const TComSPS &sps    = pcPic->getPicSym()->getSPS();
  const TComPPS &pps    = pcPic->getPicSym()->getPPS();
  const UInt maxCuWidth = sps.getMaxCUWidth();
  const UInt maxCuHeight= sps.getMaxCUHeight();
  UInt uiCurNumParts    = pcPic->getNumPartitionsInCtu() >> (uiDepth<<1);
  UInt uiQNumParts      = uiCurNumParts>>2;


  Bool bBoundary = false;
  UInt uiLPelX   = pcCU->getCUPelX() + g_auiRasterToPelX[ g_auiZscanToRaster[uiAbsPartIdx] ];
  UInt uiRPelX   = uiLPelX + (maxCuWidth>>uiDepth)  - 1;
  UInt uiTPelY   = pcCU->getCUPelY() + g_auiRasterToPelY[ g_auiZscanToRaster[uiAbsPartIdx] ];
  UInt uiBPelY   = uiTPelY + (maxCuHeight>>uiDepth) - 1;
#if H_MV_ENC_DEC_TRAC
  DTRACE_CU_S("=========== coding_quadtree ===========\n")
  DTRACE_CU("x0", uiLPelX)
  DTRACE_CU("x1", uiTPelY)
  DTRACE_CU("log2CbSize", maxCuWidth>>uiDepth)
  DTRACE_CU("cqtDepth"  , uiDepth)
#endif

  if( ( uiRPelX < sps.getPicWidthInLumaSamples() ) && ( uiBPelY < sps.getPicHeightInLumaSamples() ) )
  {
    m_pcEntropyDecoder->decodeSplitFlag( pcCU, uiAbsPartIdx, uiDepth );
  }
  else
  {
    bBoundary = true;
  }
  if( ( ( uiDepth < pcCU->getDepth( uiAbsPartIdx ) ) && ( uiDepth < sps.getLog2DiffMaxMinCodingBlockSize() ) ) || bBoundary )
  {
    UInt uiIdx = uiAbsPartIdx;
    if( uiDepth == pps.getMaxCuDQPDepth() && pps.getUseDQP())
    {
      setdQPFlag(true);
      pcCU->setQPSubParts( pcCU->getRefQP(uiAbsPartIdx), uiAbsPartIdx, uiDepth ); // set QP to default QP
    }

    if( uiDepth == pps.getPpsRangeExtension().getDiffCuChromaQpOffsetDepth() && pcCU->getSlice()->getUseChromaQpAdj() )
    {
      setIsChromaQpAdjCoded(true);
    }

    for ( UInt uiPartUnitIdx = 0; uiPartUnitIdx < 4; uiPartUnitIdx++ )
    {
      uiLPelX   = pcCU->getCUPelX() + g_auiRasterToPelX[ g_auiZscanToRaster[uiIdx] ];
      uiTPelY   = pcCU->getCUPelY() + g_auiRasterToPelY[ g_auiZscanToRaster[uiIdx] ];

      if ( !isLastCtuOfSliceSegment && ( uiLPelX < sps.getPicWidthInLumaSamples() ) && ( uiTPelY < sps.getPicHeightInLumaSamples() ) )
      {
        xDecodeCU( pcCU, uiIdx, uiDepth+1, isLastCtuOfSliceSegment );
      }
      else
      {
        pcCU->setOutsideCUPart( uiIdx, uiDepth+1 );
      }

      uiIdx += uiQNumParts;
    }
    if( uiDepth == pps.getMaxCuDQPDepth() && pps.getUseDQP())
    {
      if ( getdQPFlag() )
      {
        UInt uiQPSrcPartIdx = uiAbsPartIdx;
        pcCU->setQPSubParts( pcCU->getRefQP( uiQPSrcPartIdx ), uiAbsPartIdx, uiDepth ); // set QP to default QP
      }
    }
    return;
  }

#if H_MV_ENC_DEC_TRAC
  DTRACE_CU_S("=========== coding_unit ===========\n")
#if H_MV_ENC_DEC_TRAC
#if ENC_DEC_TRACE
    stopAtPos  ( pcCU->getSlice()->getPOC(), 
    pcCU->getSlice()->getLayerId(), 
    uiLPelX,
    uiTPelY,
    uiRPelX-uiLPelX+1, 
    uiBPelY-uiTPelY+1);
#endif
#endif

#endif

  if( uiDepth <= pps.getMaxCuDQPDepth() && pps.getUseDQP())
  {
    setdQPFlag(true);
    pcCU->setQPSubParts( pcCU->getRefQP(uiAbsPartIdx), uiAbsPartIdx, uiDepth ); // set QP to default QP
  }
#if H_3D_NBDV 
  DisInfo DvInfo; 
  DvInfo.m_acNBDV.setZero();
  DvInfo.m_aVIdxCan = 0;
#if H_3D_NBDV_REF  
  DvInfo.m_acDoNBDV.setZero();
#endif
 
if(!pcCU->getSlice()->isIntra())
  {
#if H_3D_ARP && H_3D_IV_MERGE
    if( pcCU->getSlice()->getIvResPredFlag() || pcCU->getSlice()->getIvMvPredFlag() )
#else 
#if H_3D_ARP
    if( pcCU->getSlice()->getVPS()->getUseAdvRP(pcCU->getSlice()->getLayerId()) )
#else
#if H_3D_IV_MERGE
    if( pcCU->getSlice()->getVPS()->getIvMvPredFlag(pcCU->getSlice()->getLayerId()) )
#else
    if (0)
#endif
#endif
#endif
    {
      m_ppcCU[uiDepth]->copyInterPredInfoFrom( pcCU, uiAbsPartIdx, REF_PIC_LIST_0, true );
      m_ppcCU[uiDepth]->copyDVInfoFrom( pcCU, uiAbsPartIdx);
      PartSize ePartTemp = m_ppcCU[uiDepth]->getPartitionSize(0);
      UChar cWidTemp     = m_ppcCU[uiDepth]->getWidth(0);
      UChar cHeightTemp  = m_ppcCU[uiDepth]->getHeight(0);
      m_ppcCU[uiDepth]->setWidth  ( 0, pcCU->getSlice()->getSPS()->getMaxCUWidth ()/(1<<uiDepth)  );
      m_ppcCU[uiDepth]->setHeight ( 0, pcCU->getSlice()->getSPS()->getMaxCUHeight()/(1<<uiDepth)  );
      m_ppcCU[uiDepth]->setPartSizeSubParts( SIZE_2Nx2N, 0, uiDepth );     
#if H_3D_IV_MERGE
      if( pcCU->getSlice()->getIsDepth())
      {
        m_ppcCU[uiDepth]->getDispforDepth(0, 0, &DvInfo);
      }
      else
      {
#endif
#if H_3D_NBDV_REF
      if( pcCU->getSlice()->getDepthBasedBlkPartFlag() )  //Notes from QC: please check the condition for DoNBDV. Remove this comment once it is done.
      {
        m_ppcCU[uiDepth]->getDisMvpCandNBDV(&DvInfo, true);
      }
      else
#endif
      {
        m_ppcCU[uiDepth]->getDisMvpCandNBDV(&DvInfo);
      }
#if H_3D_IV_MERGE
      }
#endif
#if ENC_DEC_TRACE && H_MV_ENC_DEC_TRAC   
      if ( g_decTraceDispDer )
      {
        DTRACE_CU( "RefViewIdx",  DvInfo.m_aVIdxCan );       
        DTRACE_CU( "MvDisp[x]", DvInfo.m_acNBDV.getHor() );
        DTRACE_CU( "MvDisp[y]", DvInfo.m_acNBDV.getVer() );
        DTRACE_CU( "MvRefinedDisp[x]", DvInfo.m_acDoNBDV.getHor() );
        DTRACE_CU( "MvRefinedDisp[y]", DvInfo.m_acDoNBDV.getVer() );
      }
#endif

      pcCU->setDvInfoSubParts(DvInfo, uiAbsPartIdx, uiDepth);
      m_ppcCU[uiDepth]->setPartSizeSubParts( ePartTemp, 0, uiDepth );
      m_ppcCU[uiDepth]->setWidth  ( 0, cWidTemp );
      m_ppcCU[uiDepth]->setHeight ( 0, cHeightTemp );
     }
  }
#endif

  if( uiDepth <= pps.getPpsRangeExtension().getDiffCuChromaQpOffsetDepth() && pcCU->getSlice()->getUseChromaQpAdj() )
  {
    setIsChromaQpAdjCoded(true);
  }

  if (pps.getTransquantBypassEnableFlag())
  {
    m_pcEntropyDecoder->decodeCUTransquantBypassFlag( pcCU, uiAbsPartIdx, uiDepth );
  }

  // decode CU mode and the partition size
  if( !pcCU->getSlice()->isIntra())
  {
    m_pcEntropyDecoder->decodeSkipFlag( pcCU, uiAbsPartIdx, uiDepth );
  }


  if( pcCU->isSkipped(uiAbsPartIdx) )
  {
#if H_MV_ENC_DEC_TRAC
    DTRACE_PU_S("=========== prediction_unit ===========\n")
    DTRACE_PU("x0", uiLPelX)
    DTRACE_PU("x1", uiTPelY)
#endif
    m_ppcCU[uiDepth]->copyInterPredInfoFrom( pcCU, uiAbsPartIdx, REF_PIC_LIST_0 );
    m_ppcCU[uiDepth]->copyInterPredInfoFrom( pcCU, uiAbsPartIdx, REF_PIC_LIST_1 );
#if H_3D_IV_MERGE
    m_ppcCU[uiDepth]->copyDVInfoFrom(pcCU, uiAbsPartIdx);
    TComMvField cMvFieldNeighbours[MRG_MAX_NUM_CANDS_MEM << 1]; // double length for mv of both lists
    UChar uhInterDirNeighbours[MRG_MAX_NUM_CANDS_MEM];
#else
    TComMvField cMvFieldNeighbours[MRG_MAX_NUM_CANDS << 1]; // double length for mv of both lists
    UChar uhInterDirNeighbours[MRG_MAX_NUM_CANDS];
#endif
    Int numValidMergeCand = 0;
    for( UInt ui = 0; ui < m_ppcCU[uiDepth]->getSlice()->getMaxNumMergeCand(); ++ui )
    {
      uhInterDirNeighbours[ui] = 0;
    }
    m_pcEntropyDecoder->decodeMergeIndex( pcCU, 0, uiAbsPartIdx, uiDepth );
    UInt uiMergeIndex = pcCU->getMergeIndex(uiAbsPartIdx);
#if H_3D_ARP
    m_pcEntropyDecoder->decodeARPW( pcCU , uiAbsPartIdx , uiDepth );
#endif
#if H_3D_IC
    m_pcEntropyDecoder->decodeICFlag( pcCU, uiAbsPartIdx, uiDepth );
#endif

#if H_3D_VSP
    Int vspFlag[MRG_MAX_NUM_CANDS_MEM];
    memset(vspFlag, 0, sizeof(Int)*MRG_MAX_NUM_CANDS_MEM);
#if H_3D_SPIVMP
    Bool bSPIVMPFlag[MRG_MAX_NUM_CANDS_MEM];
    memset(bSPIVMPFlag, false, sizeof(Bool)*MRG_MAX_NUM_CANDS_MEM);
    TComMvField*  pcMvFieldSP;
    UChar* puhInterDirSP;
    pcMvFieldSP = new TComMvField[pcCU->getPic()->getPicSym()->getNumPartition()*2]; 
    puhInterDirSP = new UChar[pcCU->getPic()->getPicSym()->getNumPartition()]; 
#endif
    m_ppcCU[uiDepth]->initAvailableFlags();
    m_ppcCU[uiDepth]->getInterMergeCandidates( 0, 0, cMvFieldNeighbours, uhInterDirNeighbours, numValidMergeCand, uiMergeIndex );
    m_ppcCU[uiDepth]->xGetInterMergeCandidates( 0, 0, cMvFieldNeighbours, uhInterDirNeighbours 
#if H_3D_SPIVMP
      , pcMvFieldSP, puhInterDirSP
#endif
      , numValidMergeCand, uiMergeIndex );

    m_ppcCU[uiDepth]->buildMCL( cMvFieldNeighbours, uhInterDirNeighbours, vspFlag
#if H_3D_SPIVMP
      , bSPIVMPFlag
#endif
      , numValidMergeCand );
    pcCU->setVSPFlagSubParts( vspFlag[uiMergeIndex], uiAbsPartIdx, 0, uiDepth );
#else
#if H_3D
    m_ppcCU[uiDepth]->initAvailableFlags();
    m_ppcCU[uiDepth]->getInterMergeCandidates( 0, 0, cMvFieldNeighbours, uhInterDirNeighbours, numValidMergeCand, uiMergeIndex );
    m_ppcCU[uiDepth]->xGetInterMergeCandidates( 0, 0, cMvFieldNeighbours, uhInterDirNeighbours, numValidMergeCand, uiMergeIndex );
#else
    m_ppcCU[uiDepth]->getInterMergeCandidates( 0, 0, cMvFieldNeighbours, uhInterDirNeighbours, numValidMergeCand, uiMergeIndex );
#endif
#endif
    pcCU->setInterDirSubParts( uhInterDirNeighbours[uiMergeIndex], uiAbsPartIdx, 0, uiDepth );

    TComMv cTmpMv( 0, 0 );
    for ( UInt uiRefListIdx = 0; uiRefListIdx < 2; uiRefListIdx++ )
    {
      if ( pcCU->getSlice()->getNumRefIdx( RefPicList( uiRefListIdx ) ) > 0 )
      {
        pcCU->setMVPIdxSubParts( 0, RefPicList( uiRefListIdx ), uiAbsPartIdx, 0, uiDepth);
        pcCU->setMVPNumSubParts( 0, RefPicList( uiRefListIdx ), uiAbsPartIdx, 0, uiDepth);
        pcCU->getCUMvField( RefPicList( uiRefListIdx ) )->setAllMvd( cTmpMv, SIZE_2Nx2N, uiAbsPartIdx, uiDepth );
        pcCU->getCUMvField( RefPicList( uiRefListIdx ) )->setAllMvField( cMvFieldNeighbours[ 2*uiMergeIndex + uiRefListIdx ], SIZE_2Nx2N, uiAbsPartIdx, uiDepth );
#if H_3D_VSP
        if( pcCU->getVSPFlag( uiAbsPartIdx ) != 0 )
        {
          if ( uhInterDirNeighbours[ uiMergeIndex ] & (1<<uiRefListIdx) )
          {
            UInt dummy;
            Int vspSize;
            Int width, height;
            m_ppcCU[uiDepth]->getPartIndexAndSize( uiAbsPartIdx, dummy, width, height );
            m_ppcCU[uiDepth]->setMvFieldPUForVSP( pcCU, uiAbsPartIdx, width, height, RefPicList( uiRefListIdx ), cMvFieldNeighbours[ 2*uiMergeIndex + uiRefListIdx ].getRefIdx(), vspSize );
            pcCU->setVSPFlag( uiAbsPartIdx, vspSize );
          }
        }
#endif
#if ENC_DEC_TRACE && H_MV_ENC_DEC_TRAC   
        if ( g_decTraceMvFromMerge )
        {        
          if ( uiRefListIdx == 0 )
          {
            DTRACE_PU( "mvL0[0]", cMvFieldNeighbours[ 2*uiMergeIndex + uiRefListIdx ].getHor());
            DTRACE_PU( "mvL0[1]", cMvFieldNeighbours[ 2*uiMergeIndex + uiRefListIdx ].getVer());
            DTRACE_PU( "refIdxL0   ", cMvFieldNeighbours[ 2*uiMergeIndex + uiRefListIdx ].getRefIdx());
          }
          else
          {
            DTRACE_PU( "mvL1[0]", cMvFieldNeighbours[ 2*uiMergeIndex + uiRefListIdx ].getHor());
            DTRACE_PU( "mvL1[1]", cMvFieldNeighbours[ 2*uiMergeIndex + uiRefListIdx ].getVer());
            DTRACE_PU( "refIdxL1", cMvFieldNeighbours[ 2*uiMergeIndex + uiRefListIdx ].getRefIdx());
          }
        }
#endif
      }
    }
#if H_3D_SPIVMP
    pcCU->setSPIVMPFlagSubParts(bSPIVMPFlag[uiMergeIndex], uiAbsPartIdx, 0, uiDepth ); 
    if (bSPIVMPFlag[uiMergeIndex])
    {
      UInt uiSPAddr;
      Int iWidth = pcCU->getWidth(uiAbsPartIdx);
      Int iHeight = pcCU->getHeight(uiAbsPartIdx);

      Int iNumSPInOneLine, iNumSP, iSPWidth, iSPHeight;

      pcCU->getSPPara(iWidth, iHeight, iNumSP, iNumSPInOneLine, iSPWidth, iSPHeight);

      for (Int iPartitionIdx = 0; iPartitionIdx < iNumSP; iPartitionIdx++)
      {
        pcCU->getSPAbsPartIdx(uiAbsPartIdx, iSPWidth, iSPHeight, iPartitionIdx, iNumSPInOneLine, uiSPAddr);
        pcCU->setInterDirSP(puhInterDirSP[iPartitionIdx], uiSPAddr, iSPWidth, iSPHeight);
        pcCU->getCUMvField( REF_PIC_LIST_0 )->setMvFieldSP(pcCU, uiSPAddr, pcMvFieldSP[2*iPartitionIdx], iSPWidth, iSPHeight);
        pcCU->getCUMvField( REF_PIC_LIST_1 )->setMvFieldSP(pcCU, uiSPAddr, pcMvFieldSP[2*iPartitionIdx + 1], iSPWidth, iSPHeight);
      }
    }
    delete[] pcMvFieldSP;
    delete[] puhInterDirSP;
#endif

    xFinishDecodeCU( pcCU, uiAbsPartIdx, uiDepth, isLastCtuOfSliceSegment );
#if H_3D_IV_MERGE
    xDecompressCU(pcCU, uiAbsPartIdx, uiDepth );
#endif

    return;
  }
#if NH_3D_DIS
  m_pcEntropyDecoder->decodeDIS( pcCU, uiAbsPartIdx, uiDepth );
  if(!pcCU->getDISFlag(uiAbsPartIdx))
  {
#endif

  m_pcEntropyDecoder->decodePredMode( pcCU, uiAbsPartIdx, uiDepth );
  m_pcEntropyDecoder->decodePartSize( pcCU, uiAbsPartIdx, uiDepth );

  if (pcCU->isIntra( uiAbsPartIdx ) && pcCU->getPartitionSize( uiAbsPartIdx ) == SIZE_2Nx2N )
  {
    m_pcEntropyDecoder->decodeIPCMInfo( pcCU, uiAbsPartIdx, uiDepth );

    if(pcCU->getIPCMFlag(uiAbsPartIdx))
    {
#if NH_3D_SDC_INTRA
      m_pcEntropyDecoder->decodeSDCFlag( pcCU, uiAbsPartIdx, uiDepth );
#endif
      xFinishDecodeCU( pcCU, uiAbsPartIdx, uiDepth, isLastCtuOfSliceSegment );
#if H_3D_IV_MERGE
      xDecompressCU(pcCU, uiAbsPartIdx, uiDepth );
#endif
      return;
    }
  }

  // prediction mode ( Intra : direction mode, Inter : Mv, reference idx )
  m_pcEntropyDecoder->decodePredInfo( pcCU, uiAbsPartIdx, uiDepth, m_ppcCU[uiDepth]);

  // Coefficient decoding
  Bool bCodeDQP = getdQPFlag();
  Bool isChromaQpAdjCoded = getIsChromaQpAdjCoded();
  m_pcEntropyDecoder->decodeCoeff( pcCU, uiAbsPartIdx, uiDepth, bCodeDQP, isChromaQpAdjCoded );
  setIsChromaQpAdjCoded( isChromaQpAdjCoded );
  setdQPFlag( bCodeDQP );
#if NH_3D_DIS
  }
#endif
  xFinishDecodeCU( pcCU, uiAbsPartIdx, uiDepth, isLastCtuOfSliceSegment );
#if H_3D_IV_MERGE
  xDecompressCU(pcCU, uiAbsPartIdx, uiDepth );
#endif
}

Void TDecCu::xFinishDecodeCU( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth, Bool &isLastCtuOfSliceSegment)
{
  if(  pcCU->getSlice()->getPPS()->getUseDQP())
  {
    pcCU->setQPSubParts( getdQPFlag()?pcCU->getRefQP(uiAbsPartIdx):pcCU->getCodedQP(), uiAbsPartIdx, uiDepth ); // set QP
  }

  if (pcCU->getSlice()->getUseChromaQpAdj() && !getIsChromaQpAdjCoded())
  {
    pcCU->setChromaQpAdjSubParts( pcCU->getCodedChromaQpAdj(), uiAbsPartIdx, uiDepth ); // set QP
  }

  isLastCtuOfSliceSegment = xDecodeSliceEnd( pcCU, uiAbsPartIdx );
}

Void TDecCu::xDecompressCU( TComDataCU* pCtu, UInt uiAbsPartIdx,  UInt uiDepth )
{
  TComPic* pcPic = pCtu->getPic();
#if !H_3D_IV_MERGE
  TComSlice * pcSlice = pCtu->getSlice();
  const TComSPS &sps=*(pcSlice->getSPS());

  Bool bBoundary = false;
  UInt uiLPelX   = pCtu->getCUPelX() + g_auiRasterToPelX[ g_auiZscanToRaster[uiAbsPartIdx] ];
  UInt uiRPelX   = uiLPelX + (sps.getMaxCUWidth()>>uiDepth)  - 1;
  UInt uiTPelY   = pCtu->getCUPelY() + g_auiRasterToPelY[ g_auiZscanToRaster[uiAbsPartIdx] ];
  UInt uiBPelY   = uiTPelY + (sps.getMaxCUHeight()>>uiDepth) - 1;

  if( ( uiRPelX >= sps.getPicWidthInLumaSamples() ) || ( uiBPelY >= sps.getPicHeightInLumaSamples() ) )
  {
    bBoundary = true;
  }

  if( ( ( uiDepth < pCtu->getDepth( uiAbsPartIdx ) ) && ( uiDepth < sps.getLog2DiffMaxMinCodingBlockSize() ) ) || bBoundary )
  {
    UInt uiNextDepth = uiDepth + 1;
    UInt uiQNumParts = pCtu->getTotalNumPart() >> (uiNextDepth<<1);
    UInt uiIdx = uiAbsPartIdx;
    for ( UInt uiPartIdx = 0; uiPartIdx < 4; uiPartIdx++ )
    {
      uiLPelX = pCtu->getCUPelX() + g_auiRasterToPelX[ g_auiZscanToRaster[uiIdx] ];
      uiTPelY = pCtu->getCUPelY() + g_auiRasterToPelY[ g_auiZscanToRaster[uiIdx] ];

      if( ( uiLPelX < sps.getPicWidthInLumaSamples() ) && ( uiTPelY < sps.getPicHeightInLumaSamples() ) )
      {
        xDecompressCU(pCtu, uiIdx, uiNextDepth );
      }

      uiIdx += uiQNumParts;
    }
    return;
  }
#endif
  // Residual reconstruction
  m_ppcYuvResi[uiDepth]->clear();

  m_ppcCU[uiDepth]->copySubCU( pCtu, uiAbsPartIdx );

  switch( m_ppcCU[uiDepth]->getPredictionMode(0) )
  {
    case MODE_INTER:
#if H_3D_DBBP
    if( m_ppcCU[uiDepth]->getDBBPFlag(0) )
    {
      xReconInterDBBP( m_ppcCU[uiDepth], uiAbsPartIdx, uiDepth );
    }
    else
    {
#endif
#if H_3D_INTER_SDC
      if( m_ppcCU[uiDepth]->getSDCFlag( 0 ) )
      {
        xReconInterSDC( m_ppcCU[uiDepth], uiAbsPartIdx, uiDepth );
      }
      else
      {
#endif
      xReconInter( m_ppcCU[uiDepth], uiDepth );
#if H_3D_INTER_SDC
      }
#endif
#if H_3D_DBBP
    }
#endif
      break;
    case MODE_INTRA:
#if NH_3D
#if NH_3D_DIS
    if( m_ppcCU[uiDepth]->getDISFlag(0) )
    {
      xReconDIS( m_ppcCU[uiDepth], 0, uiDepth );
    }
#else
    if(false )
    {
     // xReconDIS( m_ppcCU[uiDepth], 0, uiDepth );
    }
#endif
#if NH_3D_SDC_INTRA
    else if( m_ppcCU[uiDepth]->getSDCFlag(0) )
    {
      xReconIntraSDC( m_ppcCU[uiDepth], 0, uiDepth );
    }
#endif
    else
#endif
      xReconIntraQT( m_ppcCU[uiDepth], uiDepth );
      break;
    default:
      assert(0);
      break;
  }

#if DEBUG_STRING
  const PredMode predMode=m_ppcCU[uiDepth]->getPredictionMode(0);
  if (DebugOptionList::DebugString_Structure.getInt()&DebugStringGetPredModeMask(predMode))
  {
    PartSize eSize=m_ppcCU[uiDepth]->getPartitionSize(0);
    std::ostream &ss(std::cout);

    ss <<"###: " << (predMode==MODE_INTRA?"Intra   ":"Inter   ") << partSizeToString[eSize] << " CU at " << m_ppcCU[uiDepth]->getCUPelX() << ", " << m_ppcCU[uiDepth]->getCUPelY() << " width=" << UInt(m_ppcCU[uiDepth]->getWidth(0)) << std::endl;
  }
#endif

  if ( m_ppcCU[uiDepth]->isLosslessCoded(0) && (m_ppcCU[uiDepth]->getIPCMFlag(0) == false))
  {
    xFillPCMBuffer(m_ppcCU[uiDepth], uiDepth);
  }

  xCopyToPic( m_ppcCU[uiDepth], pcPic, uiAbsPartIdx, uiDepth );
}

Void TDecCu::xReconInter( TComDataCU* pcCU, UInt uiDepth )
{

  // inter prediction
  m_pcPrediction->motionCompensation( pcCU, m_ppcYuvReco[uiDepth] );

#if DEBUG_STRING
  const Int debugPredModeMask=DebugStringGetPredModeMask(MODE_INTER);
  if (DebugOptionList::DebugString_Pred.getInt()&debugPredModeMask)
  {
    printBlockToStream(std::cout, "###inter-pred: ", *(m_ppcYuvReco[uiDepth]));
  }
#endif

  // inter recon
  xDecodeInterTexture( pcCU, uiDepth );

#if DEBUG_STRING
  if (DebugOptionList::DebugString_Resi.getInt()&debugPredModeMask)
  {
    printBlockToStream(std::cout, "###inter-resi: ", *(m_ppcYuvResi[uiDepth]));
  }
#endif

  // clip for only non-zero cbp case
  if  ( pcCU->getQtRootCbf( 0) )
  {
    m_ppcYuvReco[uiDepth]->addClip( m_ppcYuvReco[uiDepth], m_ppcYuvResi[uiDepth], 0, pcCU->getWidth( 0 ), pcCU->getSlice()->getSPS()->getBitDepths() );
  }
  else
  {
    m_ppcYuvReco[uiDepth]->copyPartToPartYuv( m_ppcYuvReco[uiDepth],0, pcCU->getWidth( 0 ),pcCU->getHeight( 0 ));
  }
#if DEBUG_STRING
  if (DebugOptionList::DebugString_Reco.getInt()&debugPredModeMask)
  {
    printBlockToStream(std::cout, "###inter-reco: ", *(m_ppcYuvReco[uiDepth]));
  }
#endif

}

#if NH_3D_DIS
Void TDecCu::xReconDIS( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth )
{
  UInt uiWidth        = pcCU->getWidth  ( 0 );
  UInt uiHeight       = pcCU->getHeight ( 0 );

  TComYuv* pcRecoYuv  = m_ppcYuvReco[uiDepth];

  UInt    uiStride    = pcRecoYuv->getStride  (COMPONENT_Y);
  Pel*    piReco      = pcRecoYuv->getAddr( COMPONENT_Y, uiAbsPartIdx );


  AOF( uiWidth == uiHeight );
  AOF( uiAbsPartIdx == 0 );

  Bool  bAboveAvail = false;
  Bool  bLeftAvail  = false;
  
  TComTURecurse rTu(pcCU, 0);
  const ChromaFormat chFmt     = rTu.GetChromaFormat();

  if ( pcCU->getDISType(uiAbsPartIdx) == 0 )
  {
    const Bool bUseFilteredPredictions=TComPrediction::filteringIntraReferenceSamples(COMPONENT_Y, VER_IDX, uiWidth, uiHeight, chFmt, pcCU->getSlice()->getSPS()->getSpsRangeExtension().getIntraSmoothingDisabledFlag());
    m_pcPrediction->initIntraPatternChType( rTu, bAboveAvail, bLeftAvail, COMPONENT_Y, bUseFilteredPredictions  DEBUG_STRING_PASS_INTO(sTemp) );
    m_pcPrediction->predIntraAng( COMPONENT_Y,   VER_IDX, 0 /* Decoder does not have an original image */, 0, piReco, uiStride, rTu, bAboveAvail, bLeftAvail, bUseFilteredPredictions );
  }
  else if ( pcCU->getDISType(uiAbsPartIdx) == 1 )
  {
    const Bool bUseFilteredPredictions=TComPrediction::filteringIntraReferenceSamples(COMPONENT_Y, HOR_IDX, uiWidth, uiHeight, chFmt, pcCU->getSlice()->getSPS()->getSpsRangeExtension().getIntraSmoothingDisabledFlag());
    m_pcPrediction->initIntraPatternChType( rTu, bAboveAvail, bLeftAvail, COMPONENT_Y, bUseFilteredPredictions  DEBUG_STRING_PASS_INTO(sTemp) );
    m_pcPrediction->predIntraAng( COMPONENT_Y,   HOR_IDX, 0 /* Decoder does not have an original image */, 0, piReco, uiStride, rTu, bAboveAvail, bLeftAvail, bUseFilteredPredictions );
  }
  else if ( pcCU->getDISType(uiAbsPartIdx) == 2 )
  {
    Pel pSingleDepth = 1 << ( pcCU->getSlice()->getSPS()->getBitDepth(CHANNEL_TYPE_LUMA) - 1 );
    pcCU->getNeighDepth ( 0, 0, &pSingleDepth, 0 );
    for( UInt uiY = 0; uiY < uiHeight; uiY++ )
    {
      for( UInt uiX = 0; uiX < uiWidth; uiX++ )
      {
        piReco[ uiX ] = pSingleDepth;
      }
      piReco+= uiStride;
    }
  }
  else if ( pcCU->getDISType(uiAbsPartIdx) == 3 )
  {
    Pel pSingleDepth = 1 << ( pcCU->getSlice()->getSPS()->getBitDepth(CHANNEL_TYPE_LUMA) - 1 );
    pcCU->getNeighDepth ( 0, 0, &pSingleDepth, 1 );
    for( UInt uiY = 0; uiY < uiHeight; uiY++ )
    {
      for( UInt uiX = 0; uiX < uiWidth; uiX++ )
      {
        piReco[ uiX ] = pSingleDepth;
      }
      piReco+= uiStride;
    }
  }

  // clear UV
  UInt  uiStrideC     = pcRecoYuv->getStride(COMPONENT_Cb);
  Pel   *pRecCb       = pcRecoYuv->getAddr(COMPONENT_Cb);
  Pel   *pRecCr       = pcRecoYuv->getAddr(COMPONENT_Cr);

  for (Int y=0; y<uiHeight/2; y++)
  {
    for (Int x=0; x<uiWidth/2; x++)
    {
      pRecCb[x] = 1<<(pcCU->getSlice()->getSPS()->getBitDepth(CHANNEL_TYPE_CHROMA)-1);
      pRecCr[x] = 1<<(pcCU->getSlice()->getSPS()->getBitDepth(CHANNEL_TYPE_CHROMA)-1);
    }

    pRecCb += uiStrideC;
    pRecCr += uiStrideC;
  }
}
#endif
#if H_3D_INTER_SDC
Void TDecCu::xReconInterSDC( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth )
{
  // inter prediction
  m_pcPrediction->motionCompensation( pcCU, m_ppcYuvReco[uiDepth] );

  UInt  uiWidth      = pcCU->getWidth ( 0 );
  UInt  uiHeight     = pcCU->getHeight( 0 );

  Pel  *pResi;
  UInt uiPelX, uiPelY;
  UInt uiResiStride = m_ppcYuvResi[uiDepth]->getStride();

  pResi = m_ppcYuvResi[uiDepth]->getLumaAddr( 0 );
  for( uiPelY = 0; uiPelY < uiHeight; uiPelY++ )
  {
    for( uiPelX = 0; uiPelX < uiWidth; uiPelX++ )
    {
      pResi[ uiPelX ] = pcCU->getSDCSegmentDCOffset( 0, 0 );
    }
    pResi += uiResiStride;
  }

  m_ppcYuvReco[uiDepth]->addClip( m_ppcYuvReco[uiDepth], m_ppcYuvResi[uiDepth], 0, pcCU->getWidth( 0 ) );

  // clear UV
  UInt  uiStrideC     = m_ppcYuvReco[uiDepth]->getCStride();
  Pel   *pRecCb       = m_ppcYuvReco[uiDepth]->getCbAddr();
  Pel   *pRecCr       = m_ppcYuvReco[uiDepth]->getCrAddr();

  for (Int y = 0; y < uiHeight/2; y++)
  {
    for (Int x = 0; x < uiWidth/2; x++)
    {
      pRecCb[x] = (Pel)( 1 << ( g_bitDepthC - 1 ) );
      pRecCr[x] = (Pel)( 1 << ( g_bitDepthC - 1 ) );
    }

    pRecCb += uiStrideC;
    pRecCr += uiStrideC;
  }
}
#endif

#if H_3D_DBBP
Void TDecCu::xReconInterDBBP( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth )
{
  AOF(!pcCU->getSlice()->getIsDepth());
  AOF(!pcCU->getSlice()->isIntra());
  PartSize ePartSize = pcCU->getPartitionSize( 0 );
  
  // get collocated depth block
  UInt uiDepthStride = 0;
#if H_3D_FCO
  Pel* pDepthPels = pcCU->getVirtualDepthBlock(pcCU->getZorderIdxInCU(), pcCU->getWidth(0), pcCU->getHeight(0), uiDepthStride);
#else
  Pel* pDepthPels = pcCU->getVirtualDepthBlock(0, pcCU->getWidth(0), pcCU->getHeight(0), uiDepthStride);
#endif
  AOF( pDepthPels != NULL );
  AOF( uiDepthStride != 0 );
  
  // compute mask by segmenting depth block
  Bool pMask[MAX_CU_SIZE*MAX_CU_SIZE];
  Bool bValidMask = m_pcPrediction->getSegmentMaskFromDepth(pDepthPels, uiDepthStride, pcCU->getWidth(0), pcCU->getHeight(0), pMask, pcCU);
  AOF(bValidMask);
  
  DbbpTmpData* pDBBPTmpData = pcCU->getDBBPTmpData();
  TComYuv* apSegPredYuv[2] = { m_ppcYuvReco[uiDepth], m_ppcYuvRecoDBBP[uiDepth] };
  
  // first, extract the two sets of motion parameters
  UInt uiPUOffset = ( g_auiPUOffset[UInt( ePartSize )] << ( ( pcCU->getSlice()->getSPS()->getMaxCUDepth() - uiDepth ) << 1 ) ) >> 4;
  for( UInt uiSegment = 0; uiSegment < 2; uiSegment++ )
  {
    UInt uiPartAddr = uiSegment*uiPUOffset;
    
    pDBBPTmpData->auhInterDir[uiSegment] = pcCU->getInterDir(uiPartAddr);
    
    for ( UInt uiRefListIdx = 0; uiRefListIdx < 2; uiRefListIdx++ )
    {
      RefPicList eRefList = (RefPicList)uiRefListIdx;
      pcCU->getMvField(pcCU, uiPartAddr, eRefList, pDBBPTmpData->acMvField[uiSegment][eRefList]);
    }
    
    AOF( pcCU->getARPW(uiPartAddr) == 0 );
    AOF( pcCU->getICFlag(uiPartAddr) == false );
    AOF( pcCU->getSPIVMPFlag(uiPartAddr) == false );
    AOF( pcCU->getVSPFlag(uiPartAddr) == 0 );
  }
  
  // do motion compensation for each segment as 2Nx2N
  pcCU->setPartSizeSubParts( SIZE_2Nx2N, 0, uiDepth );
  pcCU->setPredModeSubParts( MODE_INTER, 0, uiDepth );
  for( UInt uiSegment = 0; uiSegment < 2; uiSegment++ )
  {
    pcCU->setInterDirSubParts( pDBBPTmpData->auhInterDir[uiSegment], 0, 0, uiDepth );
  
    for ( UInt uiRefListIdx = 0; uiRefListIdx < 2; uiRefListIdx++ )
    {
      RefPicList eRefList = (RefPicList)uiRefListIdx;

      pcCU->getCUMvField( eRefList )->setAllMvField( pDBBPTmpData->acMvField[uiSegment][eRefList], SIZE_2Nx2N, 0, 0 );
    }
    
    // inter prediction
    m_pcPrediction->motionCompensation( pcCU, apSegPredYuv[uiSegment] );
  }
  
  // restore motion information in both segments again
  pcCU->setPartSizeSubParts( ePartSize,  0, uiDepth );
  pcCU->setPredModeSubParts( MODE_INTER, 0, uiDepth );
  for( UInt uiSegment = 0; uiSegment < 2; uiSegment++ )
  {
    UInt uiPartAddr = uiSegment*uiPUOffset;
    
    pcCU->setDBBPFlagSubParts(true, uiPartAddr, uiSegment, uiDepth);
    pcCU->setInterDirSubParts(pDBBPTmpData->auhInterDir[uiSegment], uiPartAddr, uiSegment, uiDepth); // interprets depth relative to LCU level
    
    for ( UInt uiRefListIdx = 0; uiRefListIdx < 2; uiRefListIdx++ )
    {
      RefPicList eRefList = (RefPicList)uiRefListIdx;

      pcCU->getCUMvField( eRefList )->setAllMvField( pDBBPTmpData->acMvField[uiSegment][eRefList], ePartSize, uiPartAddr, 0, uiSegment ); // interprets depth relative to rpcTempCU level
    }
  }
  
  // reconstruct final prediction signal by combining both segments
  m_pcPrediction->combineSegmentsWithMask(apSegPredYuv, m_ppcYuvReco[uiDepth], pMask, pcCU->getWidth(0), pcCU->getHeight(0), 0, ePartSize);

  // inter recon
  xDecodeInterTexture( pcCU, 0, uiDepth );
  
  // clip for only non-zero cbp case
  if  ( ( pcCU->getCbf( 0, TEXT_LUMA ) ) || ( pcCU->getCbf( 0, TEXT_CHROMA_U ) ) || ( pcCU->getCbf(0, TEXT_CHROMA_V ) ) )
  {
    m_ppcYuvReco[uiDepth]->addClip( m_ppcYuvReco[uiDepth], m_ppcYuvResi[uiDepth], 0, pcCU->getWidth( 0 ) );
  }
  else
  {
    m_ppcYuvReco[uiDepth]->copyPartToPartYuv( m_ppcYuvReco[uiDepth],0, pcCU->getWidth( 0 ),pcCU->getHeight( 0 ));
  }
}
#endif


Void
TDecCu::xIntraRecBlk(       TComYuv*    pcRecoYuv,
                            TComYuv*    pcPredYuv,
                            TComYuv*    pcResiYuv,
                      const ComponentID compID,
                            TComTU     &rTu)
{
  if (!rTu.ProcessComponentSection(compID))
  {
    return;
  }
  const Bool       bIsLuma = isLuma(compID);


  TComDataCU *pcCU = rTu.getCU();
  const TComSPS &sps=*(pcCU->getSlice()->getSPS());
  const UInt uiAbsPartIdx=rTu.GetAbsPartIdxTU();

  const TComRectangle &tuRect  =rTu.getRect(compID);
  const UInt uiWidth           = tuRect.width;
  const UInt uiHeight          = tuRect.height;
  const UInt uiStride          = pcRecoYuv->getStride (compID);
        Pel* piPred            = pcPredYuv->getAddr( compID, uiAbsPartIdx );
  const ChromaFormat chFmt     = rTu.GetChromaFormat();

  if (uiWidth != uiHeight)
  {
    //------------------------------------------------

    //split at current level if dividing into square sub-TUs

    TComTURecurse subTURecurse(rTu, false, TComTU::VERTICAL_SPLIT, true, compID);

    //recurse further
    do
    {
      xIntraRecBlk(pcRecoYuv, pcPredYuv, pcResiYuv, compID, subTURecurse);
    } while (subTURecurse.nextSection(rTu));

    //------------------------------------------------

    return;
  }

  const UInt uiChPredMode  = pcCU->getIntraDir( toChannelType(compID), uiAbsPartIdx );
  const UInt partsPerMinCU = 1<<(2*(sps.getMaxTotalCUDepth() - sps.getLog2DiffMaxMinCodingBlockSize()));
  const UInt uiChCodedMode = (uiChPredMode==DM_CHROMA_IDX && !bIsLuma) ? pcCU->getIntraDir(CHANNEL_TYPE_LUMA, getChromasCorrespondingPULumaIdx(uiAbsPartIdx, chFmt, partsPerMinCU)) : uiChPredMode;
  const UInt uiChFinalMode = ((chFmt == CHROMA_422)       && !bIsLuma) ? g_chroma422IntraAngleMappingTable[uiChCodedMode] : uiChCodedMode;

  //===== init availability pattern =====
  Bool  bAboveAvail = false;
  Bool  bLeftAvail  = false;

  const Bool bUseFilteredPredictions=TComPrediction::filteringIntraReferenceSamples(compID, uiChFinalMode, uiWidth, uiHeight, chFmt, pcCU->getSlice()->getSPS()->getSpsRangeExtension().getIntraSmoothingDisabledFlag());

#if DEBUG_STRING
  std::ostream &ss(std::cout);
#endif

  DEBUG_STRING_NEW(sTemp)
  m_pcPrediction->initIntraPatternChType( rTu, bAboveAvail, bLeftAvail, compID, bUseFilteredPredictions  DEBUG_STRING_PASS_INTO(sTemp) );


  //===== get prediction signal =====
#if NH_3D_DMM
  if( bIsLuma && isDmmMode( uiChFinalMode ) )
  {
    m_pcPrediction->predIntraLumaDmm( pcCU, uiAbsPartIdx, getDmmType( uiChFinalMode ), piPred, uiStride, uiWidth, uiHeight );
  }
  else
  {
#endif
  m_pcPrediction->predIntraAng( compID,   uiChFinalMode, 0 /* Decoder does not have an original image */, 0, piPred, uiStride, rTu, bAboveAvail, bLeftAvail, bUseFilteredPredictions );
#if NH_3D_DMM
  }
#endif

#if DEBUG_STRING
  ss << sTemp;
#endif

  //===== inverse transform =====
  Pel*      piResi            = pcResiYuv->getAddr( compID, uiAbsPartIdx );
  TCoeff*   pcCoeff           = pcCU->getCoeff(compID) + rTu.getCoefficientOffset(compID);//( uiNumCoeffInc * uiAbsPartIdx );

  const QpParam cQP(*pcCU, compID);


  DEBUG_STRING_NEW(sDebug);
#if DEBUG_STRING
  const Int debugPredModeMask=DebugStringGetPredModeMask(MODE_INTRA);
  std::string *psDebug=(DebugOptionList::DebugString_InvTran.getInt()&debugPredModeMask) ? &sDebug : 0;
#endif
#if H_3D
  Bool useDltFlag = (isDmmMode( uiLumaPredMode ) || uiLumaPredMode == HOR_IDX || uiLumaPredMode == VER_IDX || uiLumaPredMode == DC_IDX) && pcCU->getSlice()->getIsDepth() && pcCU->getSlice()->getPPS()->getDLT()->getUseDLTFlag(pcCU->getSlice()->getLayerIdInVps());

  if ( pcCU->getCbf( uiAbsPartIdx, TEXT_LUMA, uiTrDepth ) || useDltFlag )
#else
  if (pcCU->getCbf(uiAbsPartIdx, compID, rTu.GetTransformDepthRel()) != 0)
#endif
  {
    m_pcTrQuant->invTransformNxN( rTu, compID, piResi, uiStride, pcCoeff, cQP DEBUG_STRING_PASS_INTO(psDebug) );
  }
  else
  {
    for (UInt y = 0; y < uiHeight; y++)
    {
      for (UInt x = 0; x < uiWidth; x++)
      {
        piResi[(y * uiStride) + x] = 0;
      }
    }
  }

#if DEBUG_STRING
  if (psDebug)
  {
    ss << (*psDebug);
  }
#endif

  //===== reconstruction =====
  const UInt uiRecIPredStride  = pcCU->getPic()->getPicYuvRec()->getStride(compID);

  const Bool useCrossComponentPrediction = isChroma(compID) && (pcCU->getCrossComponentPredictionAlpha(uiAbsPartIdx, compID) != 0);
  const Pel* pResiLuma  = pcResiYuv->getAddr( COMPONENT_Y, uiAbsPartIdx );
  const Int  strideLuma = pcResiYuv->getStride( COMPONENT_Y );

        Pel* pPred      = piPred;
        Pel* pResi      = piResi;
        Pel* pReco      = pcRecoYuv->getAddr( compID, uiAbsPartIdx );
        Pel* pRecIPred  = pcCU->getPic()->getPicYuvRec()->getAddr( compID, pcCU->getCtuRsAddr(), pcCU->getZorderIdxInCtu() + uiAbsPartIdx );


#if DEBUG_STRING
  const Bool bDebugPred=((DebugOptionList::DebugString_Pred.getInt()&debugPredModeMask) && DEBUG_STRING_CHANNEL_CONDITION(compID));
  const Bool bDebugResi=((DebugOptionList::DebugString_Resi.getInt()&debugPredModeMask) && DEBUG_STRING_CHANNEL_CONDITION(compID));
  const Bool bDebugReco=((DebugOptionList::DebugString_Reco.getInt()&debugPredModeMask) && DEBUG_STRING_CHANNEL_CONDITION(compID));
  if (bDebugPred || bDebugResi || bDebugReco)
  {
    ss << "###: " << "CompID: " << compID << " pred mode (ch/fin): " << uiChPredMode << "/" << uiChFinalMode << " absPartIdx: " << rTu.GetAbsPartIdxTU() << std::endl;
  }
#endif

  const Int clipbd = sps.getBitDepth(toChannelType(compID));
#if O0043_BEST_EFFORT_DECODING
  const Int bitDepthDelta = sps.getStreamBitDepth(toChannelType(compID)) - clipbd;
#endif

  if( useCrossComponentPrediction )
  {
    TComTrQuant::crossComponentPrediction( rTu, compID, pResiLuma, piResi, piResi, uiWidth, uiHeight, strideLuma, uiStride, uiStride, true );
  }

  for( UInt uiY = 0; uiY < uiHeight; uiY++ )
  {
#if DEBUG_STRING
    if (bDebugPred || bDebugResi || bDebugReco)
    {
      ss << "###: ";
    }

    if (bDebugPred)
    {
      ss << " - pred: ";
      for( UInt uiX = 0; uiX < uiWidth; uiX++ )
      {
        ss << pPred[ uiX ] << ", ";
      }
    }
    if (bDebugResi)
    {
      ss << " - resi: ";
    }
#endif

    for( UInt uiX = 0; uiX < uiWidth; uiX++ )
    {
#if DEBUG_STRING
      if (bDebugResi)
      {
        ss << pResi[ uiX ] << ", ";
      }
#endif
#if O0043_BEST_EFFORT_DECODING
      pReco    [ uiX ] = ClipBD( rightShiftEvenRounding<Pel>(pPred[ uiX ] + pResi[ uiX ], bitDepthDelta), clipbd );
#else
      pReco    [ uiX ] = ClipBD( pPred[ uiX ] + pResi[ uiX ], clipbd );
#endif
      pRecIPred[ uiX ] = pReco[ uiX ];
    }
#if DEBUG_STRING
    if (bDebugReco)
    {
      ss << " - reco: ";
      for( UInt uiX = 0; uiX < uiWidth; uiX++ )
      {
        ss << pReco[ uiX ] << ", ";
      }
    }

    if (bDebugPred || bDebugResi || bDebugReco)
    {
      ss << "\n";
    }
#endif
    pPred     += uiStride;
    pResi     += uiStride;
    pReco     += uiStride;
    pRecIPred += uiRecIPredStride;
  }
}

Void
TDecCu::xReconIntraQT( TComDataCU* pcCU, UInt uiDepth )
{
  if (pcCU->getIPCMFlag(0))
  {
    xReconPCM( pcCU, uiDepth );
    return;
  }
  const UInt numChType = pcCU->getPic()->getChromaFormat()!=CHROMA_400 ? 2 : 1;
  for (UInt chType=CHANNEL_TYPE_LUMA; chType<numChType; chType++)
  {
    const ChannelType chanType=ChannelType(chType);
    const Bool NxNPUHas4Parts = ::isChroma(chanType) ? enable4ChromaPUsInIntraNxNCU(pcCU->getPic()->getChromaFormat()) : true;
    const UInt uiInitTrDepth = ( pcCU->getPartitionSize(0) != SIZE_2Nx2N && NxNPUHas4Parts ? 1 : 0 );

    TComTURecurse tuRecurseCU(pcCU, 0);
    TComTURecurse tuRecurseWithPU(tuRecurseCU, false, (uiInitTrDepth==0)?TComTU::DONT_SPLIT : TComTU::QUAD_SPLIT);

    do
    {
      xIntraRecQT( m_ppcYuvReco[uiDepth], m_ppcYuvReco[uiDepth], m_ppcYuvResi[uiDepth], chanType, tuRecurseWithPU );
    } while (tuRecurseWithPU.nextSection(tuRecurseCU));
  }
}

#if NH_3D_SDC_INTRA
Void TDecCu::xReconIntraSDC( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth )
{
  TComYuv* pcRecoYuv  = m_ppcYuvReco[uiDepth];
  TComYuv* pcPredYuv  = m_ppcYuvReco[uiDepth];
  TComYuv* pcResiYuv  = m_ppcYuvResi[uiDepth];

  UInt uiWidth        = pcCU->getWidth ( 0 );
  UInt uiHeight       = pcCU->getHeight( 0 );
  UInt uiLumaPredMode = pcCU->getIntraDir( CHANNEL_TYPE_LUMA, uiAbsPartIdx );
  const Int bitDepthY = pcCU->getSlice()->getSPS()->getBitDepth(CHANNEL_TYPE_LUMA);
  const TComSPS     &sps    = *(pcCU->getSlice()->getSPS());
  const ChromaFormat chFmt  = pcCU->getPic()->getChromaFormat();

  UInt sdcDepth = 0;
  UInt uiStride;
  Pel* piReco;
  Pel* piPred;
  Pel* piResi;

  Pel* piRecIPred;
  UInt uiRecIPredStride;
  
  Pel apDCPredValues[2];
  UInt uiNumSegments;

  Bool* pbMask = NULL;
  UInt uiMaskStride = 0;

#if NH_3D_DMM
  if( isDmmMode( uiLumaPredMode ) )
  {
    assert( uiWidth == uiHeight  );
    assert( uiWidth >= DMM_MIN_SIZE && uiWidth <= DMM_MAX_SIZE );
    assert( !((uiWidth >> pcCU->getSlice()->getSPS()->getQuadtreeTULog2MaxSize()) > 1) );

    uiNumSegments     = 2;

    uiStride          = pcRecoYuv->getStride( COMPONENT_Y );
    piReco            = pcRecoYuv->getAddr  ( COMPONENT_Y, uiAbsPartIdx );
    piPred            = pcPredYuv->getAddr  ( COMPONENT_Y, uiAbsPartIdx );
    piResi            = pcResiYuv->getAddr  ( COMPONENT_Y, uiAbsPartIdx );

    piRecIPred        = pcCU->getPic()->getPicYuvRec()->getAddr  ( COMPONENT_Y, pcCU->getCtuRsAddr(), pcCU->getZorderIdxInCtu() + uiAbsPartIdx );
    uiRecIPredStride  = pcCU->getPic()->getPicYuvRec()->getStride( COMPONENT_Y );

    //===== init availability pattern =====
    TComTURecurse tuRecurseCU(pcCU, 0);
    TComTURecurse tuRecurseWithPU(tuRecurseCU, false, TComTU::DONT_SPLIT);

    Bool bAboveAvail = false;
    Bool bLeftAvail  = false;
    m_pcPrediction->initIntraPatternChType( tuRecurseWithPU, bAboveAvail, bLeftAvail, COMPONENT_Y, false DEBUG_STRING_PASS_INTO(sTemp) );

    // get partition
    pbMask       = new Bool[ uiWidth*uiHeight ];
    uiMaskStride = uiWidth;
    switch( getDmmType( uiLumaPredMode ) )
    {
    case( DMM1_IDX ): { (getWedgeListScaled( uiWidth )->at( pcCU->getDmm1WedgeTabIdx( uiAbsPartIdx ) )).getPatternScaledCopy( uiWidth, pbMask ); } break;
    case( DMM4_IDX ): { m_pcPrediction->predContourFromTex( pcCU, uiAbsPartIdx, uiWidth, uiHeight, pbMask );                                     } break;
    default: assert(0);
    }

    // get predicted partition values
    Pel predDC1 = 0, predDC2 = 0;
    m_pcPrediction->predBiSegDCs( pcCU, uiAbsPartIdx, uiWidth, uiHeight, pbMask, uiMaskStride, predDC1, predDC2 );

    // set prediction signal
    Pel* pDst = piPred;
    m_pcPrediction->assignBiSegDCs( pDst, uiStride, pbMask, uiMaskStride, predDC1, predDC2 );
    apDCPredValues[0] = predDC1;
    apDCPredValues[1] = predDC2;
  }
  else // regular HEVC intra modes
  {
#endif
    uiNumSegments = 1;

    if( ( uiWidth >> pcCU->getSlice()->getSPS()->getQuadtreeTULog2MaxSize() ) > 1 )
    {
      sdcDepth = g_aucConvertToBit[uiWidth] + 2 - pcCU->getSlice()->getSPS()->getQuadtreeTULog2MaxSize();
    }
    
    //===== loop over partitions =====
    TComTURecurse tuRecurseCU(pcCU, 0);
    TComTURecurse tuRecurseWithPU(tuRecurseCU, false, (sdcDepth==0)?TComTU::DONT_SPLIT:TComTU::QUAD_SPLIT);

    do
    {
      const TComRectangle &puRect = tuRecurseWithPU.getRect(COMPONENT_Y);
      const UInt uiAbsPartIdxTU = tuRecurseWithPU.GetAbsPartIdxTU();
      
      Pel* piPredTU       = pcPredYuv->getAddr  ( COMPONENT_Y, uiAbsPartIdxTU );
      UInt uiStrideTU     = pcPredYuv->getStride( COMPONENT_Y );
      
      Pel* piRecIPredTU   = pcCU->getPic()->getPicYuvRec()->getAddr( COMPONENT_Y, pcCU->getCtuRsAddr(), pcCU->getZorderIdxInCtu() + uiAbsPartIdxTU );
      UInt uiRecIPredStrideTU  = pcCU->getPic()->getPicYuvRec()->getStride(COMPONENT_Y);
      
      const Bool bUseFilter = TComPrediction::filteringIntraReferenceSamples(COMPONENT_Y, uiLumaPredMode, puRect.width, puRect.height, chFmt, sps.getSpsRangeExtension().getIntraSmoothingDisabledFlag());
      
      //===== init pattern for luma prediction =====
      Bool bAboveAvail = false;
      Bool bLeftAvail  = false;
      
      m_pcPrediction->initIntraPatternChType( tuRecurseWithPU, bAboveAvail, bLeftAvail, COMPONENT_Y, bUseFilter  DEBUG_STRING_PASS_INTO(sTemp) );
      
      m_pcPrediction->predIntraAng( COMPONENT_Y, uiLumaPredMode, NULL, uiStrideTU, piPredTU, uiStrideTU, tuRecurseWithPU, bAboveAvail, bLeftAvail, bUseFilter );
      
      // copy for prediction of next part
      for( UInt uiY = 0; uiY < puRect.height; uiY++ )
      {
        for( UInt uiX = 0; uiX < puRect.width; uiX++ )
        {
          piPredTU      [ uiX ] = ClipBD( piPredTU[ uiX ], bitDepthY );
          piRecIPredTU  [ uiX ] = piPredTU[ uiX ];
        }
        piPredTU     += uiStrideTU;
        piRecIPredTU += uiRecIPredStrideTU;
      }
      
      
    } while (tuRecurseWithPU.nextSection(tuRecurseCU));

    // reset to full block
    uiWidth  = pcCU->getWidth( 0 );
    uiHeight = pcCU->getHeight( 0 );

    uiStride          = pcRecoYuv->getStride( COMPONENT_Y );
    piReco            = pcRecoYuv->getAddr  ( COMPONENT_Y, uiAbsPartIdx );
    piPred            = pcPredYuv->getAddr  ( COMPONENT_Y, uiAbsPartIdx );
    piResi            = pcResiYuv->getAddr  ( COMPONENT_Y, uiAbsPartIdx );
    
    piRecIPred        = pcCU->getPic()->getPicYuvRec()->getAddr  ( COMPONENT_Y, pcCU->getCtuRsAddr(), pcCU->getZorderIdxInCtu() + uiAbsPartIdx );
    uiRecIPredStride  = pcCU->getPic()->getPicYuvRec()->getStride( COMPONENT_Y );

    m_pcPrediction->predConstantSDC( piPred, uiStride, uiWidth, apDCPredValues[0] ); apDCPredValues[1] = 0;
#if NH_3D_DMM
  }
#endif
  
  // reconstruct residual based on mask + DC residuals
  Pel apDCResiValues[2];
  for( UInt uiSegment = 0; uiSegment < uiNumSegments; uiSegment++ )
  {
#if NH_3D_DLT
    Pel   pPredIdx    = pcCU->getSlice()->getPPS()->getDLT()->depthValue2idx( pcCU->getSlice()->getLayerIdInVps(), apDCPredValues[uiSegment] );
    Pel   pResiIdx    = pcCU->getSDCSegmentDCOffset(uiSegment, uiAbsPartIdx);
    Pel   pRecoValue  = pcCU->getSlice()->getPPS()->getDLT()->idx2DepthValue( pcCU->getSlice()->getLayerIdInVps(), pPredIdx + pResiIdx );

    apDCResiValues[uiSegment]  = pRecoValue - apDCPredValues[uiSegment];
#else
    apDCResiValues[uiSegment]  = pcCU->getSDCSegmentDCOffset(uiSegment, uiAbsPartIdx);
#endif
  }
  
  //===== reconstruction =====
  Bool*pMask      = pbMask;
  Pel* pPred      = piPred;
  Pel* pResi      = piResi;
  Pel* pReco      = piReco;
  Pel* pRecIPred  = piRecIPred;
  
  for( UInt uiY = 0; uiY < uiHeight; uiY++ )
  {
    for( UInt uiX = 0; uiX < uiWidth; uiX++ )
    {
      UChar ucSegment = pMask?(UChar)pMask[uiX]:0;
      assert( ucSegment < uiNumSegments );
      
      Pel pResiDC = apDCResiValues[ucSegment];
      
      pReco    [ uiX ] = ClipBD( pPred[ uiX ] + pResiDC, bitDepthY );
      pRecIPred[ uiX ] = pReco[ uiX ];
    }
    pPred     += uiStride;
    pResi     += uiStride;
    pReco     += uiStride;
    pRecIPred += uiRecIPredStride;
    pMask     += uiMaskStride;
  }
  
  // clear chroma
  UInt  uiStrideC     = pcPredYuv->getStride( COMPONENT_Cb );
  Pel   *pRecCb       = pcPredYuv->getAddr  ( COMPONENT_Cb, uiAbsPartIdx );
  Pel   *pRecCr       = pcPredYuv->getAddr  ( COMPONENT_Cr, uiAbsPartIdx );
  
  for (Int y=0; y<uiHeight/2; y++)
  {
    for (Int x=0; x<uiWidth/2; x++)
    {
      pRecCb[x] = 128;
      pRecCr[x] = 128;
    }
    
    pRecCb += uiStrideC;
    pRecCr += uiStrideC;
  }
#if NH_3D_DMM
  if( pbMask ) { delete[] pbMask; }
#endif
}
#endif


/** Function for deriving reconstructed PU/CU chroma samples with QTree structure
 * \param pcRecoYuv pointer to reconstructed sample arrays
 * \param pcPredYuv pointer to prediction sample arrays
 * \param pcResiYuv pointer to residue sample arrays
 * \param chType    texture channel type (luma/chroma)
 * \param rTu       reference to transform data
 *
 \ This function derives reconstructed PU/CU chroma samples with QTree recursive structure
 */

Void
TDecCu::xIntraRecQT(TComYuv*    pcRecoYuv,
                    TComYuv*    pcPredYuv,
                    TComYuv*    pcResiYuv,
                    const ChannelType chType,
                    TComTU     &rTu)
{
  UInt uiTrDepth    = rTu.GetTransformDepthRel();
  TComDataCU *pcCU  = rTu.getCU();
  UInt uiAbsPartIdx = rTu.GetAbsPartIdxTU();
  UInt uiTrMode     = pcCU->getTransformIdx( uiAbsPartIdx );
  if( uiTrMode == uiTrDepth )
  {
    if (isLuma(chType))
    {
      xIntraRecBlk( pcRecoYuv, pcPredYuv, pcResiYuv, COMPONENT_Y,  rTu );
    }
    else
    {
      const UInt numValidComp=getNumberValidComponents(rTu.GetChromaFormat());
      for(UInt compID=COMPONENT_Cb; compID<numValidComp; compID++)
      {
        xIntraRecBlk( pcRecoYuv, pcPredYuv, pcResiYuv, ComponentID(compID), rTu );
      }
    }
  }
  else
  {
    TComTURecurse tuRecurseChild(rTu, false);
    do
    {
      xIntraRecQT( pcRecoYuv, pcPredYuv, pcResiYuv, chType, tuRecurseChild );
    } while (tuRecurseChild.nextSection(rTu));
  }
}

Void TDecCu::xCopyToPic( TComDataCU* pcCU, TComPic* pcPic, UInt uiZorderIdx, UInt uiDepth )
{
  UInt uiCtuRsAddr = pcCU->getCtuRsAddr();

  m_ppcYuvReco[uiDepth]->copyToPicYuv  ( pcPic->getPicYuvRec (), uiCtuRsAddr, uiZorderIdx );

  return;
}

Void TDecCu::xDecodeInterTexture ( TComDataCU* pcCU, UInt uiDepth )
{

  TComTURecurse tuRecur(pcCU, 0, uiDepth);

  for(UInt ch=0; ch<pcCU->getPic()->getNumberValidComponents(); ch++)
  {
    const ComponentID compID=ComponentID(ch);
    DEBUG_STRING_OUTPUT(std::cout, debug_reorder_data_inter_token[compID])

    m_pcTrQuant->invRecurTransformNxN ( compID, m_ppcYuvResi[uiDepth], tuRecur );
  }

  DEBUG_STRING_OUTPUT(std::cout, debug_reorder_data_inter_token[MAX_NUM_COMPONENT])
}

/** Function for deriving reconstructed luma/chroma samples of a PCM mode CU.
 * \param pcCU pointer to current CU
 * \param uiPartIdx part index
 * \param piPCM pointer to PCM code arrays
 * \param piReco pointer to reconstructed sample arrays
 * \param uiStride stride of reconstructed sample arrays
 * \param uiWidth CU width
 * \param uiHeight CU height
 * \param compID colour component ID
 * \returns Void
 */
Void TDecCu::xDecodePCMTexture( TComDataCU* pcCU, const UInt uiPartIdx, const Pel *piPCM, Pel* piReco, const UInt uiStride, const UInt uiWidth, const UInt uiHeight, const ComponentID compID)
{
        Pel* piPicReco         = pcCU->getPic()->getPicYuvRec()->getAddr(compID, pcCU->getCtuRsAddr(), pcCU->getZorderIdxInCtu()+uiPartIdx);
  const UInt uiPicStride       = pcCU->getPic()->getPicYuvRec()->getStride(compID);
  const TComSPS &sps           = *(pcCU->getSlice()->getSPS());
  const UInt uiPcmLeftShiftBit = sps.getBitDepth(toChannelType(compID)) - sps.getPCMBitDepth(toChannelType(compID));

  for(UInt uiY = 0; uiY < uiHeight; uiY++ )
  {
    for(UInt uiX = 0; uiX < uiWidth; uiX++ )
    {
      piReco[uiX] = (piPCM[uiX] << uiPcmLeftShiftBit);
      piPicReco[uiX] = piReco[uiX];
    }
    piPCM += uiWidth;
    piReco += uiStride;
    piPicReco += uiPicStride;
  }
}

/** Function for reconstructing a PCM mode CU.
 * \param pcCU pointer to current CU
 * \param uiDepth CU Depth
 * \returns Void
 */
Void TDecCu::xReconPCM( TComDataCU* pcCU, UInt uiDepth )
{
  const UInt maxCuWidth     = pcCU->getSlice()->getSPS()->getMaxCUWidth();
  const UInt maxCuHeight    = pcCU->getSlice()->getSPS()->getMaxCUHeight();
  for (UInt ch=0; ch < pcCU->getPic()->getNumberValidComponents(); ch++)
  {
    const ComponentID compID = ComponentID(ch);
    const UInt width  = (maxCuWidth >>(uiDepth+m_ppcYuvResi[uiDepth]->getComponentScaleX(compID)));
    const UInt height = (maxCuHeight>>(uiDepth+m_ppcYuvResi[uiDepth]->getComponentScaleY(compID)));
    const UInt stride = m_ppcYuvResi[uiDepth]->getStride(compID);
    Pel * pPCMChannel = pcCU->getPCMSample(compID);
    Pel * pRecChannel = m_ppcYuvReco[uiDepth]->getAddr(compID);
    xDecodePCMTexture( pcCU, 0, pPCMChannel, pRecChannel, stride, width, height, compID );
  }
}

/** Function for filling the PCM buffer of a CU using its reconstructed sample array
 * \param pCU   pointer to current CU
 * \param depth CU Depth
 */
Void TDecCu::xFillPCMBuffer(TComDataCU* pCU, UInt depth)
{
  const ChromaFormat format = pCU->getPic()->getChromaFormat();
  const UInt numValidComp   = getNumberValidComponents(format);
  const UInt maxCuWidth     = pCU->getSlice()->getSPS()->getMaxCUWidth();
  const UInt maxCuHeight    = pCU->getSlice()->getSPS()->getMaxCUHeight();

  for (UInt componentIndex = 0; componentIndex < numValidComp; componentIndex++)
  {
    const ComponentID component = ComponentID(componentIndex);

    const UInt width  = maxCuWidth  >> (depth + getComponentScaleX(component, format));
    const UInt height = maxCuHeight >> (depth + getComponentScaleY(component, format));

    Pel *source      = m_ppcYuvReco[depth]->getAddr(component, 0, width);
    Pel *destination = pCU->getPCMSample(component);

    const UInt sourceStride = m_ppcYuvReco[depth]->getStride(component);

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

//! \}
