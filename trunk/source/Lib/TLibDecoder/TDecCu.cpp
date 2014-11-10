/* The copyright in this software is being made available under the BSD
 * License, included below. This software may be subject to other third party
 * and contributor rights, including patent rights, and no such rights are
 * granted under this license.  
 *
* Copyright (c) 2010-2014, ITU/ISO/IEC
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
 \param    uiMaxDepth    total number of allowable depth
 \param    uiMaxWidth    largest CU width
 \param    uiMaxHeight   largest CU height
 */
Void TDecCu::create( UInt uiMaxDepth, UInt uiMaxWidth, UInt uiMaxHeight )
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
    
    m_ppcYuvResi[ui] = new TComYuv;    m_ppcYuvResi[ui]->create( uiWidth, uiHeight );
    m_ppcYuvReco[ui] = new TComYuv;    m_ppcYuvReco[ui]->create( uiWidth, uiHeight );
    m_ppcCU     [ui] = new TComDataCU; m_ppcCU     [ui]->create( uiNumPartitions, uiWidth, uiHeight, true, uiMaxWidth >> (m_uiMaxDepth - 1) );
#if H_3D_DBBP
    m_ppcYuvRecoDBBP[ui] = new TComYuv;    m_ppcYuvRecoDBBP[ui]->create( uiWidth, uiHeight );
#endif
  }
  
  m_bDecodeDQP = false;

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

/** \param    pcCU        pointer of CU data
 \param    ruiIsLast   last data?
 */
Void TDecCu::decodeCU( TComDataCU* pcCU, UInt& ruiIsLast )
{
  if ( pcCU->getSlice()->getPPS()->getUseDQP() )
  {
    setdQPFlag(true);
  }

  // start from the top level CU
  xDecodeCU( pcCU, 0, 0, ruiIsLast);
}

/** \param    pcCU        pointer of CU data
 */
Void TDecCu::decompressCU( TComDataCU* pcCU )
{
#if !H_3D_IV_MERGE
  xDecompressCU( pcCU, 0,  0 );
#endif
}

// ====================================================================================================================
// Protected member functions
// ====================================================================================================================

/**decode end-of-slice flag
 * \param pcCU
 * \param uiAbsPartIdx 
 * \param uiDepth 
 * \returns Bool
 */
Bool TDecCu::xDecodeSliceEnd( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth)
{
  UInt uiIsLast;
  TComPic* pcPic = pcCU->getPic();
  TComSlice * pcSlice = pcPic->getSlice(pcPic->getCurrSliceIdx());
  UInt uiCurNumParts    = pcPic->getNumPartInCU() >> (uiDepth<<1);
  UInt uiWidth = pcSlice->getSPS()->getPicWidthInLumaSamples();
  UInt uiHeight = pcSlice->getSPS()->getPicHeightInLumaSamples();
  UInt uiGranularityWidth = g_uiMaxCUWidth;
  UInt uiPosX = pcCU->getCUPelX() + g_auiRasterToPelX[ g_auiZscanToRaster[uiAbsPartIdx] ];
  UInt uiPosY = pcCU->getCUPelY() + g_auiRasterToPelY[ g_auiZscanToRaster[uiAbsPartIdx] ];

  if(((uiPosX+pcCU->getWidth(uiAbsPartIdx))%uiGranularityWidth==0||(uiPosX+pcCU->getWidth(uiAbsPartIdx)==uiWidth))
    &&((uiPosY+pcCU->getHeight(uiAbsPartIdx))%uiGranularityWidth==0||(uiPosY+pcCU->getHeight(uiAbsPartIdx)==uiHeight)))
  {
    m_pcEntropyDecoder->decodeTerminatingBit( uiIsLast );
  }
  else
  {
    uiIsLast=0;
  }
  
  if(uiIsLast) 
  {
    if(pcSlice->isNextSliceSegment()&&!pcSlice->isNextSlice()) 
    {
      pcSlice->setSliceSegmentCurEndCUAddr(pcCU->getSCUAddr()+uiAbsPartIdx+uiCurNumParts);
    }
    else 
    {
      pcSlice->setSliceCurEndCUAddr(pcCU->getSCUAddr()+uiAbsPartIdx+uiCurNumParts);
      pcSlice->setSliceSegmentCurEndCUAddr(pcCU->getSCUAddr()+uiAbsPartIdx+uiCurNumParts);
    }
  }

  return uiIsLast>0;
}

/** decode CU block recursively
 * \param pcCU
 * \param uiAbsPartIdx 
 * \param uiDepth 
 * \returns Void
 */

Void TDecCu::xDecodeCU( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth, UInt& ruiIsLast)
{
  TComPic* pcPic = pcCU->getPic();
  UInt uiCurNumParts    = pcPic->getNumPartInCU() >> (uiDepth<<1);
  UInt uiQNumParts      = uiCurNumParts>>2;
  
  Bool bBoundary = false;
  UInt uiLPelX   = pcCU->getCUPelX() + g_auiRasterToPelX[ g_auiZscanToRaster[uiAbsPartIdx] ];
  UInt uiRPelX   = uiLPelX + (g_uiMaxCUWidth>>uiDepth)  - 1;
  UInt uiTPelY   = pcCU->getCUPelY() + g_auiRasterToPelY[ g_auiZscanToRaster[uiAbsPartIdx] ];
  UInt uiBPelY   = uiTPelY + (g_uiMaxCUHeight>>uiDepth) - 1;
#if H_MV_ENC_DEC_TRAC
  DTRACE_CU_S("=========== coding_quadtree ===========\n")
  DTRACE_CU("x0", uiLPelX)
  DTRACE_CU("x1", uiTPelY)
  DTRACE_CU("log2CbSize", g_uiMaxCUWidth>>uiDepth)
  DTRACE_CU("cqtDepth"  , uiDepth)
#endif

  TComSlice * pcSlice = pcCU->getPic()->getSlice(pcCU->getPic()->getCurrSliceIdx());
  Bool bStartInCU = pcCU->getSCUAddr()+uiAbsPartIdx+uiCurNumParts>pcSlice->getSliceSegmentCurStartCUAddr()&&pcCU->getSCUAddr()+uiAbsPartIdx<pcSlice->getSliceSegmentCurStartCUAddr();
  if((!bStartInCU) && ( uiRPelX < pcSlice->getSPS()->getPicWidthInLumaSamples() ) && ( uiBPelY < pcSlice->getSPS()->getPicHeightInLumaSamples() ) )
  {
    m_pcEntropyDecoder->decodeSplitFlag( pcCU, uiAbsPartIdx, uiDepth );
  }
  else
  {
    bBoundary = true;
  }
  
  if( ( ( uiDepth < pcCU->getDepth( uiAbsPartIdx ) ) && ( uiDepth < g_uiMaxCUDepth - g_uiAddCUDepth ) ) || bBoundary )
  {
    UInt uiIdx = uiAbsPartIdx;
    if( (g_uiMaxCUWidth>>uiDepth) == pcCU->getSlice()->getPPS()->getMinCuDQPSize() && pcCU->getSlice()->getPPS()->getUseDQP())
    {
      setdQPFlag(true);
      pcCU->setQPSubParts( pcCU->getRefQP(uiAbsPartIdx), uiAbsPartIdx, uiDepth ); // set QP to default QP
    }

    for ( UInt uiPartUnitIdx = 0; uiPartUnitIdx < 4; uiPartUnitIdx++ )
    {
      uiLPelX   = pcCU->getCUPelX() + g_auiRasterToPelX[ g_auiZscanToRaster[uiIdx] ];
      uiTPelY   = pcCU->getCUPelY() + g_auiRasterToPelY[ g_auiZscanToRaster[uiIdx] ];
      
      Bool bSubInSlice = pcCU->getSCUAddr()+uiIdx+uiQNumParts>pcSlice->getSliceSegmentCurStartCUAddr();
      if ( bSubInSlice )
      {
        if ( !ruiIsLast && ( uiLPelX < pcCU->getSlice()->getSPS()->getPicWidthInLumaSamples() ) && ( uiTPelY < pcCU->getSlice()->getSPS()->getPicHeightInLumaSamples() ) )
        {
          xDecodeCU( pcCU, uiIdx, uiDepth+1, ruiIsLast );
        }
        else
        {
          pcCU->setOutsideCUPart( uiIdx, uiDepth+1 );
        }
      }
      
      uiIdx += uiQNumParts;
    }
    if( (g_uiMaxCUWidth>>uiDepth) == pcCU->getSlice()->getPPS()->getMinCuDQPSize() && pcCU->getSlice()->getPPS()->getUseDQP())
    {
      if ( getdQPFlag() )
      {
        UInt uiQPSrcPartIdx;
        if ( pcPic->getCU( pcCU->getAddr() )->getSliceSegmentStartCU(uiAbsPartIdx) != pcSlice->getSliceSegmentCurStartCUAddr() )
        {
          uiQPSrcPartIdx = pcSlice->getSliceSegmentCurStartCUAddr() % pcPic->getNumPartInCU();
        }
        else
        {
          uiQPSrcPartIdx = uiAbsPartIdx;
        }
        pcCU->setQPSubParts( pcCU->getRefQP( uiQPSrcPartIdx ), uiAbsPartIdx, uiDepth ); // set QP to default QP
      }
    }
    return;
  }
  
#if H_MV_ENC_DEC_TRAC
  DTRACE_CU_S("=========== coding_unit ===========\n")
#endif
#if !LGE_DDD_REMOVAL_J0042_J0030
#if H_3D_DDD
      pcCU->setUseDDD( false, uiAbsPartIdx, uiDepth );
#endif
#endif

  if( (g_uiMaxCUWidth>>uiDepth) >= pcCU->getSlice()->getPPS()->getMinCuDQPSize() && pcCU->getSlice()->getPPS()->getUseDQP())
  {
    setdQPFlag(true);
    pcCU->setQPSubParts( pcCU->getRefQP(uiAbsPartIdx), uiAbsPartIdx, uiDepth ); // set QP to default QP
  }
#if H_3D_NBDV 
  DisInfo DvInfo; 
  DvInfo.bDV = false;
  DvInfo.m_acNBDV.setZero();
  DvInfo.m_aVIdxCan = 0;
#if H_3D_NBDV_REF  
  DvInfo.m_acDoNBDV.setZero();
#endif
 
 
  if(!pcCU->getSlice()->isIntra())
  {
#if H_3D_ARP && H_3D_IV_MERGE
#if HHI_TOOL_PARAMETERS_I2_J0107
    if( pcCU->getSlice()->getIvResPredFlag() || pcCU->getSlice()->getIvMvPredFlag() )
#else
    if( pcCU->getSlice()->getVPS()->getUseAdvRP( pcCU->getSlice()->getLayerId() ) || pcCU->getSlice()->getVPS()->getIvMvPredFlag( pcCU->getSlice()->getLayerId() ))
#endif
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
        DvInfo.bDV = m_ppcCU[uiDepth]->getDispforDepth(0, 0, &DvInfo);
      }
      else
      {
#endif
#if H_3D_NBDV_REF
#if HHI_TOOL_PARAMETERS_I2_J0107
      if( pcCU->getSlice()->getDepthBasedBlkPartFlag() )  //Notes from QC: please check the condition for DoNBDV. Remove this comment once it is done.
#else
      if(pcCU->getSlice()->getVPS()->getDepthRefinementFlag( pcCU->getSlice()->getLayerIdInVps() ))  //Notes from QC: please check the condition for DoNBDV. Remove this comment once it is done.
#endif
      {
        DvInfo.bDV = m_ppcCU[uiDepth]->getDisMvpCandNBDV(&DvInfo, true);
      }
      else
#endif
      {
        DvInfo.bDV = m_ppcCU[uiDepth]->getDisMvpCandNBDV(&DvInfo);
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
  if (pcCU->getSlice()->getPPS()->getTransquantBypassEnableFlag())
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
#if !LGE_DDD_REMOVAL_J0042_J0030
#if H_3D_DDD
    if( uiMergeIndex == m_ppcCU[uiDepth]->getUseDDDCandIdx() )
    {
        assert( pcCU->getSlice()->getViewIndex() != 0 );
        pcCU->setUseDDD( true, uiAbsPartIdx, 0, uiDepth );
        pcCU->setDDDepthSubParts( m_ppcCU[uiDepth]->getDDTmpDepth(),uiAbsPartIdx, 0, uiDepth );
    }
#endif
#endif

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

    xFinishDecodeCU( pcCU, uiAbsPartIdx, uiDepth, ruiIsLast );
#if H_3D_IV_MERGE
    xDecompressCU(pcCU, uiAbsPartIdx, uiDepth );
#endif
    return;
  }
#if H_3D_SINGLE_DEPTH
  m_pcEntropyDecoder->decodeSingleDepthMode( pcCU, uiAbsPartIdx, uiDepth );
  if(!pcCU->getSingleDepthFlag(uiAbsPartIdx))
  {
#endif
  m_pcEntropyDecoder->decodePredMode( pcCU, uiAbsPartIdx, uiDepth );
  m_pcEntropyDecoder->decodePartSize( pcCU, uiAbsPartIdx, uiDepth );

#if H_3D_DIM_SDC
  m_pcEntropyDecoder->decodeSDCFlag( pcCU, uiAbsPartIdx, uiDepth );
#endif
  if (pcCU->isIntra( uiAbsPartIdx ) && pcCU->getPartitionSize( uiAbsPartIdx ) == SIZE_2Nx2N )
  {
    m_pcEntropyDecoder->decodeIPCMInfo( pcCU, uiAbsPartIdx, uiDepth );

    if(pcCU->getIPCMFlag(uiAbsPartIdx))
    {
      xFinishDecodeCU( pcCU, uiAbsPartIdx, uiDepth, ruiIsLast );
#if H_3D_IV_MERGE
      xDecompressCU(pcCU, uiAbsPartIdx, uiDepth );
#endif
      return;
    }
  }

  UInt uiCurrWidth      = pcCU->getWidth ( uiAbsPartIdx );
  UInt uiCurrHeight     = pcCU->getHeight( uiAbsPartIdx );
  
  // prediction mode ( Intra : direction mode, Inter : Mv, reference idx )
  m_pcEntropyDecoder->decodePredInfo( pcCU, uiAbsPartIdx, uiDepth, m_ppcCU[uiDepth]);
  // Coefficient decoding
  Bool bCodeDQP = getdQPFlag();
  m_pcEntropyDecoder->decodeCoeff( pcCU, uiAbsPartIdx, uiDepth, uiCurrWidth, uiCurrHeight, bCodeDQP );
  setdQPFlag( bCodeDQP );
#if H_3D_SINGLE_DEPTH
  }
#endif
  xFinishDecodeCU( pcCU, uiAbsPartIdx, uiDepth, ruiIsLast );
#if H_3D_IV_MERGE
  xDecompressCU(pcCU, uiAbsPartIdx, uiDepth );
#endif
}

Void TDecCu::xFinishDecodeCU( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth, UInt& ruiIsLast)
{
  if(  pcCU->getSlice()->getPPS()->getUseDQP())
  {
    pcCU->setQPSubParts( getdQPFlag()?pcCU->getRefQP(uiAbsPartIdx):pcCU->getCodedQP(), uiAbsPartIdx, uiDepth ); // set QP
  }

  ruiIsLast = xDecodeSliceEnd( pcCU, uiAbsPartIdx, uiDepth);
}

Void TDecCu::xDecompressCU( TComDataCU* pcCU, UInt uiAbsPartIdx,  UInt uiDepth )
{
  TComPic* pcPic = pcCU->getPic();
#if !H_3D_IV_MERGE
  Bool bBoundary = false;
  UInt uiLPelX   = pcCU->getCUPelX() + g_auiRasterToPelX[ g_auiZscanToRaster[uiAbsPartIdx] ];
  UInt uiRPelX   = uiLPelX + (g_uiMaxCUWidth>>uiDepth)  - 1;
  UInt uiTPelY   = pcCU->getCUPelY() + g_auiRasterToPelY[ g_auiZscanToRaster[uiAbsPartIdx] ];
  UInt uiBPelY   = uiTPelY + (g_uiMaxCUHeight>>uiDepth) - 1;
  
  UInt uiCurNumParts    = pcPic->getNumPartInCU() >> (uiDepth<<1);
  TComSlice * pcSlice = pcCU->getPic()->getSlice(pcCU->getPic()->getCurrSliceIdx());
  Bool bStartInCU = pcCU->getSCUAddr()+uiAbsPartIdx+uiCurNumParts>pcSlice->getSliceSegmentCurStartCUAddr()&&pcCU->getSCUAddr()+uiAbsPartIdx<pcSlice->getSliceSegmentCurStartCUAddr();
  if(bStartInCU||( uiRPelX >= pcSlice->getSPS()->getPicWidthInLumaSamples() ) || ( uiBPelY >= pcSlice->getSPS()->getPicHeightInLumaSamples() ) )
  {
    bBoundary = true;
  }
  
  if( ( ( uiDepth < pcCU->getDepth( uiAbsPartIdx ) ) && ( uiDepth < g_uiMaxCUDepth - g_uiAddCUDepth ) ) || bBoundary )
  {
    UInt uiNextDepth = uiDepth + 1;
    UInt uiQNumParts = pcCU->getTotalNumPart() >> (uiNextDepth<<1);
    UInt uiIdx = uiAbsPartIdx;
    for ( UInt uiPartIdx = 0; uiPartIdx < 4; uiPartIdx++ )
    {
      uiLPelX = pcCU->getCUPelX() + g_auiRasterToPelX[ g_auiZscanToRaster[uiIdx] ];
      uiTPelY = pcCU->getCUPelY() + g_auiRasterToPelY[ g_auiZscanToRaster[uiIdx] ];
      
      Bool binSlice = (pcCU->getSCUAddr()+uiIdx+uiQNumParts>pcSlice->getSliceSegmentCurStartCUAddr())&&(pcCU->getSCUAddr()+uiIdx<pcSlice->getSliceSegmentCurEndCUAddr());
      if(binSlice&&( uiLPelX < pcSlice->getSPS()->getPicWidthInLumaSamples() ) && ( uiTPelY < pcSlice->getSPS()->getPicHeightInLumaSamples() ) )
      {
        xDecompressCU(pcCU, uiIdx, uiNextDepth );
      }
      
      uiIdx += uiQNumParts;
    }
    return;
  }
#endif  
  // Residual reconstruction
  m_ppcYuvResi[uiDepth]->clear();
  
  m_ppcCU[uiDepth]->copySubCU( pcCU, uiAbsPartIdx, uiDepth );
  
#if H_MV_ENC_DEC_TRAC
#if ENC_DEC_TRACE
  stopAtPos  ( m_ppcCU[uiDepth]->getSlice()->getPOC(), 
    m_ppcCU[uiDepth]->getSlice()->getLayerId(), 
    m_ppcCU[uiDepth]->getCUPelX(),
    m_ppcCU[uiDepth]->getCUPelY(),
    m_ppcCU[uiDepth]->getWidth(0), 
    m_ppcCU[uiDepth]->getHeight(0) );
#endif
#endif

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
#if H_3D_SINGLE_DEPTH
      if( m_ppcCU[uiDepth]->getSingleDepthFlag(0) )
        xReconIntraSingleDepth( m_ppcCU[uiDepth], 0, uiDepth );
#if H_3D_DIM_SDC
      else if( m_ppcCU[uiDepth]->getSDCFlag(0) )
        xReconIntraSDC( m_ppcCU[uiDepth], 0, uiDepth );
#endif
      else
#else
#if H_3D_DIM_SDC
      if( m_ppcCU[uiDepth]->getSDCFlag(0) )
        xReconIntraSDC( m_ppcCU[uiDepth], 0, uiDepth );
      else
#endif
#endif
      xReconIntraQT( m_ppcCU[uiDepth], uiDepth );
      break;
    default:
      assert(0);
      break;
  }
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
#if H_3D_SINGLE_DEPTH
Void TDecCu::xReconIntraSingleDepth( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth )
{
  UInt uiWidth        = pcCU->getWidth  ( 0 );
  UInt uiHeight       = pcCU->getHeight ( 0 );

  TComYuv* pcRecoYuv  = m_ppcYuvReco[uiDepth];

  UInt    uiStride    = pcRecoYuv->getStride  ();
  Pel*    piReco      = pcRecoYuv->getLumaAddr( uiAbsPartIdx );


  AOF( uiWidth == uiHeight );
  AOF( uiAbsPartIdx == 0 );

  //construction of depth candidates
  Pel testDepth;
#if SINGLE_DEPTH_SIMP_J0115
  Pel DepthNeighbours[2];
#else
  Pel DepthNeighbours[5];
#endif
  Int index =0;
#if SINGLE_DEPTH_SIMP_J0115
  for( Int i = 0; (i < 2) && (index<SINGLE_DEPTH_MODE_CAND_LIST_SIZE) ; i++ )
#else
  for( Int i = 0; (i < 5) && (index<SINGLE_DEPTH_MODE_CAND_LIST_SIZE) ; i++ )
#endif
  {
    if(!pcCU->getNeighDepth (0, uiAbsPartIdx, &testDepth, i))
    {
      continue;
    }
    DepthNeighbours[index]=testDepth;
    index++;
#if !SINGLE_DEPTH_SIMP_J0115
    for(Int j=0;j<index-1;j++)
    {
     if( (DepthNeighbours[index-1]==DepthNeighbours[j]) )
     {
       index--;
       break;
     }
    }
#endif
  }

  if(index==0)
  {
    DepthNeighbours[index]=1<<(g_bitDepthY-1);
    index++;
  }

  if(index==1)
  {
    DepthNeighbours[index]=ClipY(DepthNeighbours[0] + 1 );
    index++;
  }

  for( UInt uiY = 0; uiY < uiHeight; uiY++ )
  {
    for( UInt uiX = 0; uiX < uiWidth; uiX++ )
    {
      piReco[ uiX ] =DepthNeighbours[(Int)pcCU->getSingleDepthValue(uiAbsPartIdx)];
    }
    piReco     += uiStride;
  }

  // clear UV
  UInt  uiStrideC     = pcRecoYuv->getCStride();
  Pel   *pRecCb       = pcRecoYuv->getCbAddr();
  Pel   *pRecCr       = pcRecoYuv->getCrAddr();

  for (Int y=0; y<uiHeight/2; y++)
  {
    for (Int x=0; x<uiWidth/2; x++)
    {
      pRecCb[x] = 1<<(g_bitDepthC-1);
      pRecCr[x] = 1<<(g_bitDepthC-1);
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
  Bool bValidMask = m_pcPrediction->getSegmentMaskFromDepth(pDepthPels, uiDepthStride, pcCU->getWidth(0), pcCU->getHeight(0), pMask);
  AOF(bValidMask);
  
  DBBPTmpData* pDBBPTmpData = pcCU->getDBBPTmpData();
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
TDecCu::xIntraRecLumaBlk( TComDataCU* pcCU,
                         UInt        uiTrDepth,
                         UInt        uiAbsPartIdx,
                         TComYuv*    pcRecoYuv,
                         TComYuv*    pcPredYuv, 
                         TComYuv*    pcResiYuv )
{
  UInt    uiWidth           = pcCU     ->getWidth   ( 0 ) >> uiTrDepth;
  UInt    uiHeight          = pcCU     ->getHeight  ( 0 ) >> uiTrDepth;
  UInt    uiStride          = pcRecoYuv->getStride  ();
  Pel*    piReco            = pcRecoYuv->getLumaAddr( uiAbsPartIdx );
  Pel*    piPred            = pcPredYuv->getLumaAddr( uiAbsPartIdx );
  Pel*    piResi            = pcResiYuv->getLumaAddr( uiAbsPartIdx );
  
  UInt    uiNumCoeffInc     = ( pcCU->getSlice()->getSPS()->getMaxCUWidth() * pcCU->getSlice()->getSPS()->getMaxCUHeight() ) >> ( pcCU->getSlice()->getSPS()->getMaxCUDepth() << 1 );
  TCoeff* pcCoeff           = pcCU->getCoeffY() + ( uiNumCoeffInc * uiAbsPartIdx );
  
  UInt    uiLumaPredMode    = pcCU->getLumaIntraDir     ( uiAbsPartIdx );
  
  UInt    uiZOrder          = pcCU->getZorderIdxInCU() + uiAbsPartIdx;
  Pel*    piRecIPred        = pcCU->getPic()->getPicYuvRec()->getLumaAddr( pcCU->getAddr(), uiZOrder );
  UInt    uiRecIPredStride  = pcCU->getPic()->getPicYuvRec()->getStride  ();
  Bool    useTransformSkip  = pcCU->getTransformSkip(uiAbsPartIdx, TEXT_LUMA);
  //===== init availability pattern =====
  Bool  bAboveAvail = false;
  Bool  bLeftAvail  = false;
  pcCU->getPattern()->initPattern   ( pcCU, uiTrDepth, uiAbsPartIdx );
  pcCU->getPattern()->initAdiPattern( pcCU, uiAbsPartIdx, uiTrDepth, 
                                     m_pcPrediction->getPredicBuf       (),
                                     m_pcPrediction->getPredicBufWidth  (),
                                     m_pcPrediction->getPredicBufHeight (),
                                     bAboveAvail, bLeftAvail );
  
  //===== get prediction signal =====
#if H_3D_DIM
  if( isDimMode( uiLumaPredMode ) )
  {
    m_pcPrediction->predIntraLumaDepth( pcCU, uiAbsPartIdx, uiLumaPredMode, piPred, uiStride, uiWidth, uiHeight );
  }
  else
  {
#endif
  m_pcPrediction->predIntraLumaAng( pcCU->getPattern(), uiLumaPredMode, piPred, uiStride, uiWidth, uiHeight, bAboveAvail, bLeftAvail );
#if H_3D_DIM
  }
#endif
  
#if H_3D
  Bool useDltFlag = (isDimMode( uiLumaPredMode ) || uiLumaPredMode == HOR_IDX || uiLumaPredMode == VER_IDX || uiLumaPredMode == DC_IDX) && pcCU->getSlice()->getIsDepth() && pcCU->getSlice()->getPPS()->getDLT()->getUseDLTFlag(pcCU->getSlice()->getLayerIdInVps());

  if ( pcCU->getCbf( uiAbsPartIdx, TEXT_LUMA, uiTrDepth ) || useDltFlag )
#else
  if ( pcCU->getCbf( uiAbsPartIdx, TEXT_LUMA, uiTrDepth ) )
#endif
  {
  //===== inverse transform =====
  m_pcTrQuant->setQPforQuant  ( pcCU->getQP(0), TEXT_LUMA, pcCU->getSlice()->getSPS()->getQpBDOffsetY(), 0 );

  Int scalingListType = (pcCU->isIntra(uiAbsPartIdx) ? 0 : 3) + g_eTTable[(Int)TEXT_LUMA];
    assert(scalingListType < SCALING_LIST_NUM);
  m_pcTrQuant->invtransformNxN( pcCU->getCUTransquantBypass(uiAbsPartIdx), TEXT_LUMA, pcCU->getLumaIntraDir( uiAbsPartIdx ), piResi, uiStride, pcCoeff, uiWidth, uiHeight, scalingListType, useTransformSkip );

  
  //===== reconstruction =====
  Pel* pPred      = piPred;
  Pel* pResi      = piResi;
  Pel* pReco      = piReco;
  Pel* pRecIPred  = piRecIPred;
  for( UInt uiY = 0; uiY < uiHeight; uiY++ )
  {
    for( UInt uiX = 0; uiX < uiWidth; uiX++ )
    {
      pReco    [ uiX ] = ClipY( pPred[ uiX ] + pResi[ uiX ] );
      pRecIPred[ uiX ] = pReco[ uiX ];
    }
    pPred     += uiStride;
    pResi     += uiStride;
    pReco     += uiStride;
    pRecIPred += uiRecIPredStride;
  }
}
  else
  {
    //===== reconstruction =====
    Pel* pPred      = piPred;
    Pel* pReco      = piReco;
    Pel* pRecIPred  = piRecIPred;
    for ( Int y = 0; y < uiHeight; y++ )
    {
      for ( Int x = 0; x < uiWidth; x++ )
      {
        pReco    [ x ] = pPred[ x ];
        pRecIPred[ x ] = pReco[ x ];
      }
      pPred     += uiStride;
      pReco     += uiStride;
      pRecIPred += uiRecIPredStride;
    }
  }
}


Void
TDecCu::xIntraRecChromaBlk( TComDataCU* pcCU,
                           UInt        uiTrDepth,
                           UInt        uiAbsPartIdx,
                           TComYuv*    pcRecoYuv,
                           TComYuv*    pcPredYuv, 
                           TComYuv*    pcResiYuv,
                           UInt        uiChromaId )
{
  UInt uiFullDepth  = pcCU->getDepth( 0 ) + uiTrDepth;
  UInt uiLog2TrSize = g_aucConvertToBit[ pcCU->getSlice()->getSPS()->getMaxCUWidth() >> uiFullDepth ] + 2;

  if( uiLog2TrSize == 2 )
  {
    assert( uiTrDepth > 0 );
    uiTrDepth--;
    UInt uiQPDiv = pcCU->getPic()->getNumPartInCU() >> ( ( pcCU->getDepth( 0 ) + uiTrDepth ) << 1 );
    Bool bFirstQ = ( ( uiAbsPartIdx % uiQPDiv ) == 0 );
    if( !bFirstQ )
    {
      return;
    }
  }

  TextType  eText             = ( uiChromaId > 0 ? TEXT_CHROMA_V : TEXT_CHROMA_U );
  UInt      uiWidth           = pcCU     ->getWidth   ( 0 ) >> ( uiTrDepth + 1 );
  UInt      uiHeight          = pcCU     ->getHeight  ( 0 ) >> ( uiTrDepth + 1 );
  UInt      uiStride          = pcRecoYuv->getCStride ();
  Pel*      piReco            = ( uiChromaId > 0 ? pcRecoYuv->getCrAddr( uiAbsPartIdx ) : pcRecoYuv->getCbAddr( uiAbsPartIdx ) );
  Pel*      piPred            = ( uiChromaId > 0 ? pcPredYuv->getCrAddr( uiAbsPartIdx ) : pcPredYuv->getCbAddr( uiAbsPartIdx ) );
  Pel*      piResi            = ( uiChromaId > 0 ? pcResiYuv->getCrAddr( uiAbsPartIdx ) : pcResiYuv->getCbAddr( uiAbsPartIdx ) );

  UInt      uiNumCoeffInc     = ( ( pcCU->getSlice()->getSPS()->getMaxCUWidth() * pcCU->getSlice()->getSPS()->getMaxCUHeight() ) >> ( pcCU->getSlice()->getSPS()->getMaxCUDepth() << 1 ) ) >> 2;
  TCoeff*   pcCoeff           = ( uiChromaId > 0 ? pcCU->getCoeffCr() : pcCU->getCoeffCb() ) + ( uiNumCoeffInc * uiAbsPartIdx );

  UInt      uiChromaPredMode  = pcCU->getChromaIntraDir( 0 );

  UInt      uiZOrder          = pcCU->getZorderIdxInCU() + uiAbsPartIdx;
  Pel*      piRecIPred        = ( uiChromaId > 0 ? pcCU->getPic()->getPicYuvRec()->getCrAddr( pcCU->getAddr(), uiZOrder ) : pcCU->getPic()->getPicYuvRec()->getCbAddr( pcCU->getAddr(), uiZOrder ) );
  UInt      uiRecIPredStride  = pcCU->getPic()->getPicYuvRec()->getCStride();
  Bool      useTransformSkipChroma = pcCU->getTransformSkip(uiAbsPartIdx,eText);
  //===== init availability pattern =====
  Bool  bAboveAvail = false;
  Bool  bLeftAvail  = false;
  pcCU->getPattern()->initPattern         ( pcCU, uiTrDepth, uiAbsPartIdx );

  pcCU->getPattern()->initAdiPatternChroma( pcCU, uiAbsPartIdx, uiTrDepth,
    m_pcPrediction->getPredicBuf       (),
    m_pcPrediction->getPredicBufWidth  (),
    m_pcPrediction->getPredicBufHeight (),
    bAboveAvail, bLeftAvail );
  Int* pPatChroma   = ( uiChromaId > 0 ? pcCU->getPattern()->getAdiCrBuf( uiWidth, uiHeight, m_pcPrediction->getPredicBuf() ) : pcCU->getPattern()->getAdiCbBuf( uiWidth, uiHeight, m_pcPrediction->getPredicBuf() ) );

  //===== get prediction signal =====
  {
    if( uiChromaPredMode == DM_CHROMA_IDX )
    {
      uiChromaPredMode = pcCU->getLumaIntraDir( 0 );
#if H_3D_DIM
      mapDepthModeToIntraDir( uiChromaPredMode );
#endif
    }
    m_pcPrediction->predIntraChromaAng( pPatChroma, uiChromaPredMode, piPred, uiStride, uiWidth, uiHeight, bAboveAvail, bLeftAvail );  
  }

  if ( pcCU->getCbf( uiAbsPartIdx, eText, uiTrDepth ) )
  {
    //===== inverse transform =====
    Int curChromaQpOffset;
    if(eText == TEXT_CHROMA_U)
    {
      curChromaQpOffset = pcCU->getSlice()->getPPS()->getChromaCbQpOffset() + pcCU->getSlice()->getSliceQpDeltaCb();
    }
    else
    {
      curChromaQpOffset = pcCU->getSlice()->getPPS()->getChromaCrQpOffset() + pcCU->getSlice()->getSliceQpDeltaCr();
    }
    m_pcTrQuant->setQPforQuant  ( pcCU->getQP(0), eText, pcCU->getSlice()->getSPS()->getQpBDOffsetC(), curChromaQpOffset );

    Int scalingListType = (pcCU->isIntra(uiAbsPartIdx) ? 0 : 3) + g_eTTable[(Int)eText];
    assert(scalingListType < SCALING_LIST_NUM);
    m_pcTrQuant->invtransformNxN( pcCU->getCUTransquantBypass(uiAbsPartIdx), eText, REG_DCT, piResi, uiStride, pcCoeff, uiWidth, uiHeight, scalingListType, useTransformSkipChroma );

    //===== reconstruction =====
    Pel* pPred      = piPred;
    Pel* pResi      = piResi;
    Pel* pReco      = piReco;
    Pel* pRecIPred  = piRecIPred;
    for( UInt uiY = 0; uiY < uiHeight; uiY++ )
    {
      for( UInt uiX = 0; uiX < uiWidth; uiX++ )
      {
        pReco    [ uiX ] = ClipC( pPred[ uiX ] + pResi[ uiX ] );
        pRecIPred[ uiX ] = pReco[ uiX ];
      }
      pPred     += uiStride;
      pResi     += uiStride;
      pReco     += uiStride;
      pRecIPred += uiRecIPredStride;
    }
  }
  else
  {
    //===== reconstruction =====
    Pel* pPred      = piPred;
    Pel* pReco      = piReco;
    Pel* pRecIPred  = piRecIPred;
    for ( Int y = 0; y < uiHeight; y++ )
    {
      for ( Int x = 0; x < uiWidth; x++ )
      {
        pReco    [ x ] = pPred[ x ];
        pRecIPred[ x ] = pReco[ x ];
      }
      pPred     += uiStride;
      pReco     += uiStride;
      pRecIPred += uiRecIPredStride;
    }    
  }
}


Void
TDecCu::xReconIntraQT( TComDataCU* pcCU, UInt uiDepth )
{
  UInt  uiInitTrDepth = ( pcCU->getPartitionSize(0) == SIZE_2Nx2N ? 0 : 1 );
  UInt  uiNumPart     = pcCU->getNumPartitions();
  UInt  uiNumQParts   = pcCU->getTotalNumPart() >> 2;
  
  if (pcCU->getIPCMFlag(0))
  {
    xReconPCM( pcCU, uiDepth );
    return;
  }

  for( UInt uiPU = 0; uiPU < uiNumPart; uiPU++ )
  {
    xIntraLumaRecQT( pcCU, uiInitTrDepth, uiPU * uiNumQParts, m_ppcYuvReco[uiDepth], m_ppcYuvReco[uiDepth], m_ppcYuvResi[uiDepth] );
  }  

  for( UInt uiPU = 0; uiPU < uiNumPart; uiPU++ )
  {
    xIntraChromaRecQT( pcCU, uiInitTrDepth, uiPU * uiNumQParts, m_ppcYuvReco[uiDepth], m_ppcYuvReco[uiDepth], m_ppcYuvResi[uiDepth] );
  }

}

#if H_3D_DIM_SDC
Void TDecCu::xReconIntraSDC( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth )
{
  UInt uiWidth        = pcCU->getWidth  ( 0 );
  UInt uiHeight       = pcCU->getHeight ( 0 );
  TComWedgelet* dmm4SegmentationOrg = new TComWedgelet( uiWidth, uiHeight );
  UInt numParts = 1;
  UInt sdcDepth    = 0;
  TComYuv* pcRecoYuv  = m_ppcYuvReco[uiDepth];
  TComYuv* pcPredYuv  = m_ppcYuvReco[uiDepth];
  TComYuv* pcResiYuv  = m_ppcYuvResi[uiDepth];

  UInt    uiStride = 0;
  Pel*    piReco;
  Pel*    piPred;
  Pel*    piResi;

  UInt    uiZOrder;        
  Pel*    piRecIPred;      
  UInt    uiRecIPredStride;

  UInt    uiLumaPredMode = 0;  

  if ((uiWidth >> pcCU->getSlice()->getSPS()->getQuadtreeTULog2MaxSize()) > 1)
  {
    numParts = uiWidth * uiWidth >> (2 * pcCU->getSlice()->getSPS()->getQuadtreeTULog2MaxSize());
    sdcDepth = g_aucConvertToBit[uiWidth] + 2 - pcCU->getSlice()->getSPS()->getQuadtreeTULog2MaxSize();
    uiWidth = uiHeight = (1 << pcCU->getSlice()->getSPS()->getQuadtreeTULog2MaxSize());
  }

  for ( Int i = 0; i < numParts; i++ )
  {
    uiStride    = pcRecoYuv->getStride  ();
    piReco      = pcRecoYuv->getLumaAddr( uiAbsPartIdx );
    piPred      = pcPredYuv->getLumaAddr( uiAbsPartIdx );
    piResi      = pcResiYuv->getLumaAddr( uiAbsPartIdx );

    uiZOrder          = pcCU->getZorderIdxInCU() + uiAbsPartIdx;
    piRecIPred        = pcCU->getPic()->getPicYuvRec()->getLumaAddr( pcCU->getAddr(), uiZOrder );
    uiRecIPredStride  = pcCU->getPic()->getPicYuvRec()->getStride  ();

    uiLumaPredMode    = pcCU->getLumaIntraDir     ( uiAbsPartIdx );

    AOF( uiWidth == uiHeight );

    //===== init availability pattern =====
    Bool  bAboveAvail = false;
    Bool  bLeftAvail  = false;

    pcCU->getPattern()->initPattern   ( pcCU, sdcDepth, uiAbsPartIdx );
    pcCU->getPattern()->initAdiPattern( pcCU, uiAbsPartIdx, sdcDepth, m_pcPrediction->getPredicBuf(), m_pcPrediction->getPredicBufWidth(), m_pcPrediction->getPredicBufHeight(), bAboveAvail, bLeftAvail );

    TComWedgelet* dmm4Segmentation = new TComWedgelet( uiWidth, uiHeight );
    //===== get prediction signal =====
    if( isDimMode( uiLumaPredMode ) )
    {
      m_pcPrediction->predIntraLumaDepth( pcCU, uiAbsPartIdx, uiLumaPredMode, piPred, uiStride, uiWidth, uiHeight, false, dmm4Segmentation  );
      Bool* dmm4PatternSplit = dmm4Segmentation->getPattern();
      Bool* dmm4PatternOrg = dmm4SegmentationOrg->getPattern();
      for( UInt k = 0; k < (uiWidth*uiHeight); k++ ) 
      { 
        dmm4PatternOrg[k+(uiAbsPartIdx<<4)] = dmm4PatternSplit[k];
      }
    }
    else
    {
      m_pcPrediction->predIntraLumaAng( pcCU->getPattern(), uiLumaPredMode, piPred, uiStride, uiWidth, uiHeight, bAboveAvail, bLeftAvail );
    }

    if ( numParts > 1 )
    {
      for( UInt uiY = 0; uiY < uiHeight; uiY++ )
      {
        for( UInt uiX = 0; uiX < uiWidth; uiX++ )
        {
          piReco        [ uiX ] = ClipY( piPred[ uiX ] );
          piRecIPred    [ uiX ] = piReco[ uiX ];
        }
        piPred     += uiStride;
        piReco     += uiStride;
        piRecIPred += uiRecIPredStride;
      }
    }
    uiAbsPartIdx += ( (uiWidth * uiWidth) >> 4 );
    dmm4Segmentation->destroy(); delete dmm4Segmentation;
  }
  uiAbsPartIdx = 0;
  
  if ( numParts > 1 )
  {
    uiWidth = pcCU->getWidth( 0 );
    uiHeight = pcCU->getHeight( 0 );
  }
  piReco      = pcRecoYuv->getLumaAddr( uiAbsPartIdx );
  piPred      = pcPredYuv->getLumaAddr( uiAbsPartIdx );
  piResi      = pcResiYuv->getLumaAddr( uiAbsPartIdx );
  uiZOrder          = pcCU->getZorderIdxInCU() + uiAbsPartIdx;
  piRecIPred        = pcCU->getPic()->getPicYuvRec()->getLumaAddr( pcCU->getAddr(), uiZOrder );
  uiRecIPredStride  = pcCU->getPic()->getPicYuvRec()->getStride  ();
  // number of segments depends on prediction mode
  UInt uiNumSegments = 1;
  Bool* pbMask = NULL;
  UInt uiMaskStride = 0;
  
  if( getDimType( uiLumaPredMode ) == DMM1_IDX )
  {
    Int uiTabIdx = pcCU->getDmmWedgeTabIdx(DMM1_IDX, uiAbsPartIdx);

    WedgeList* pacWedgeList  = pcCU->isDMM1UpscaleMode(uiWidth) ? &g_dmmWedgeLists[(g_aucConvertToBit[pcCU->getDMM1BasePatternWidth(uiWidth)])] :  &g_dmmWedgeLists[(g_aucConvertToBit[uiWidth])];
    TComWedgelet* pcWedgelet = &(pacWedgeList->at( uiTabIdx ));

    uiNumSegments = 2;

    pbMask       = pcCU->isDMM1UpscaleMode( uiWidth ) ? pcWedgelet->getScaledPattern(uiWidth) : pcWedgelet->getPattern();
    uiMaskStride = pcCU->isDMM1UpscaleMode( uiWidth ) ? uiWidth : pcWedgelet->getStride();
  }
  if( getDimType( uiLumaPredMode ) == DMM4_IDX )
  {
    uiNumSegments = 2;
    pbMask  = dmm4SegmentationOrg->getPattern();
    uiMaskStride = dmm4SegmentationOrg->getStride();
  }
  // get DC prediction for each segment
  Pel apDCPredValues[2];
  if ( getDimType( uiLumaPredMode ) == DMM1_IDX || getDimType( uiLumaPredMode ) == DMM4_IDX )
  {
    apDCPredValues[0] = pcCU->getDmmPredictor( 0 );
    apDCPredValues[1] = pcCU->getDmmPredictor( 1 );
  }
  else
  {
    m_pcPrediction->analyzeSegmentsSDC(piPred, uiStride, uiWidth, apDCPredValues, uiNumSegments, pbMask, uiMaskStride, uiLumaPredMode);
  }
  
  // reconstruct residual based on mask + DC residuals
  Pel apDCResiValues[2];
  for( UInt uiSegment = 0; uiSegment < uiNumSegments; uiSegment++ )
  {
#if H_3D_DIM_DLT
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
      
      pReco    [ uiX ] = ClipY( pPred[ uiX ] + pResiDC );
      pRecIPred[ uiX ] = pReco[ uiX ];
    }
    pPred     += uiStride;
    pResi     += uiStride;
    pReco     += uiStride;
    pRecIPred += uiRecIPredStride;
    pMask     += uiMaskStride;
  }
  
  // clear UV
  UInt  uiStrideC     = pcPredYuv->getCStride();
  Pel   *pRecCb       = pcPredYuv->getCbAddr();
  Pel   *pRecCr       = pcPredYuv->getCrAddr();
  
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
  dmm4SegmentationOrg->destroy(); delete dmm4SegmentationOrg;
}
#endif

/** Function for deriving recontructed PU/CU Luma sample with QTree structure
 * \param pcCU pointer of current CU
 * \param uiTrDepth current tranform split depth
 * \param uiAbsPartIdx  part index
 * \param pcRecoYuv pointer to reconstructed sample arrays
 * \param pcPredYuv pointer to prediction sample arrays
 * \param pcResiYuv pointer to residue sample arrays
 * 
 \ This function dervies recontructed PU/CU Luma sample with recursive QTree structure
 */
Void
TDecCu::xIntraLumaRecQT( TComDataCU* pcCU,
                     UInt        uiTrDepth,
                     UInt        uiAbsPartIdx,
                     TComYuv*    pcRecoYuv,
                     TComYuv*    pcPredYuv, 
                     TComYuv*    pcResiYuv )
{
  UInt uiFullDepth  = pcCU->getDepth(0) + uiTrDepth;
  UInt uiTrMode     = pcCU->getTransformIdx( uiAbsPartIdx );
  if( uiTrMode == uiTrDepth )
  {
    xIntraRecLumaBlk  ( pcCU, uiTrDepth, uiAbsPartIdx, pcRecoYuv, pcPredYuv, pcResiYuv );
  }
  else
  {
    UInt uiNumQPart  = pcCU->getPic()->getNumPartInCU() >> ( ( uiFullDepth + 1 ) << 1 );
    for( UInt uiPart = 0; uiPart < 4; uiPart++ )
    {
      xIntraLumaRecQT( pcCU, uiTrDepth + 1, uiAbsPartIdx + uiPart * uiNumQPart, pcRecoYuv, pcPredYuv, pcResiYuv );
    }
  }
}

/** Function for deriving recontructed PU/CU chroma samples with QTree structure
 * \param pcCU pointer of current CU
 * \param uiTrDepth current tranform split depth
 * \param uiAbsPartIdx  part index
 * \param pcRecoYuv pointer to reconstructed sample arrays
 * \param pcPredYuv pointer to prediction sample arrays
 * \param pcResiYuv pointer to residue sample arrays
 * 
 \ This function dervies recontructed PU/CU chroma samples with QTree recursive structure
 */
Void
TDecCu::xIntraChromaRecQT( TComDataCU* pcCU,
                     UInt        uiTrDepth,
                     UInt        uiAbsPartIdx,
                     TComYuv*    pcRecoYuv,
                     TComYuv*    pcPredYuv, 
                     TComYuv*    pcResiYuv )
{
  UInt uiFullDepth  = pcCU->getDepth(0) + uiTrDepth;
  UInt uiTrMode     = pcCU->getTransformIdx( uiAbsPartIdx );
  if( uiTrMode == uiTrDepth )
  {
    xIntraRecChromaBlk( pcCU, uiTrDepth, uiAbsPartIdx, pcRecoYuv, pcPredYuv, pcResiYuv, 0 );
    xIntraRecChromaBlk( pcCU, uiTrDepth, uiAbsPartIdx, pcRecoYuv, pcPredYuv, pcResiYuv, 1 );
  }
  else
  {
    UInt uiNumQPart  = pcCU->getPic()->getNumPartInCU() >> ( ( uiFullDepth + 1 ) << 1 );
    for( UInt uiPart = 0; uiPart < 4; uiPart++ )
    {
      xIntraChromaRecQT( pcCU, uiTrDepth + 1, uiAbsPartIdx + uiPart * uiNumQPart, pcRecoYuv, pcPredYuv, pcResiYuv );
    }
  }
}

Void TDecCu::xCopyToPic( TComDataCU* pcCU, TComPic* pcPic, UInt uiZorderIdx, UInt uiDepth )
{
  UInt uiCUAddr = pcCU->getAddr();
  
  m_ppcYuvReco[uiDepth]->copyToPicYuv  ( pcPic->getPicYuvRec (), uiCUAddr, uiZorderIdx );
  
  return;
}

Void TDecCu::xDecodeInterTexture ( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth )
{
  UInt    uiWidth    = pcCU->getWidth ( uiAbsPartIdx );
  UInt    uiHeight   = pcCU->getHeight( uiAbsPartIdx );
  TCoeff* piCoeff;
  
  Pel*    pResi;
  UInt    trMode = pcCU->getTransformIdx( uiAbsPartIdx );
  
  // Y
  piCoeff = pcCU->getCoeffY();
  pResi = m_ppcYuvResi[uiDepth]->getLumaAddr();

  m_pcTrQuant->setQPforQuant( pcCU->getQP( uiAbsPartIdx ), TEXT_LUMA, pcCU->getSlice()->getSPS()->getQpBDOffsetY(), 0 );

  m_pcTrQuant->invRecurTransformNxN ( pcCU, 0, TEXT_LUMA, pResi, 0, m_ppcYuvResi[uiDepth]->getStride(), uiWidth, uiHeight, trMode, 0, piCoeff );
  
  // Cb and Cr
  Int curChromaQpOffset = pcCU->getSlice()->getPPS()->getChromaCbQpOffset() + pcCU->getSlice()->getSliceQpDeltaCb();
  m_pcTrQuant->setQPforQuant( pcCU->getQP( uiAbsPartIdx ), TEXT_CHROMA, pcCU->getSlice()->getSPS()->getQpBDOffsetC(), curChromaQpOffset );

  uiWidth  >>= 1;
  uiHeight >>= 1;
  piCoeff = pcCU->getCoeffCb(); pResi = m_ppcYuvResi[uiDepth]->getCbAddr();
  m_pcTrQuant->invRecurTransformNxN ( pcCU, 0, TEXT_CHROMA_U, pResi, 0, m_ppcYuvResi[uiDepth]->getCStride(), uiWidth, uiHeight, trMode, 0, piCoeff );

  curChromaQpOffset = pcCU->getSlice()->getPPS()->getChromaCrQpOffset() + pcCU->getSlice()->getSliceQpDeltaCr();
  m_pcTrQuant->setQPforQuant( pcCU->getQP( uiAbsPartIdx ), TEXT_CHROMA, pcCU->getSlice()->getSPS()->getQpBDOffsetC(), curChromaQpOffset );

  piCoeff = pcCU->getCoeffCr(); pResi = m_ppcYuvResi[uiDepth]->getCrAddr();
  m_pcTrQuant->invRecurTransformNxN ( pcCU, 0, TEXT_CHROMA_V, pResi, 0, m_ppcYuvResi[uiDepth]->getCStride(), uiWidth, uiHeight, trMode, 0, piCoeff );
}

/** Function for deriving reconstructed luma/chroma samples of a PCM mode CU.
 * \param pcCU pointer to current CU
 * \param uiPartIdx part index
 * \param piPCM pointer to PCM code arrays
 * \param piReco pointer to reconstructed sample arrays
 * \param uiStride stride of reconstructed sample arrays
 * \param uiWidth CU width
 * \param uiHeight CU height
 * \param ttText texture component type
 * \returns Void
 */
Void TDecCu::xDecodePCMTexture( TComDataCU* pcCU, UInt uiPartIdx, Pel *piPCM, Pel* piReco, UInt uiStride, UInt uiWidth, UInt uiHeight, TextType ttText)
{
  UInt uiX, uiY;
  Pel* piPicReco;
  UInt uiPicStride;
  UInt uiPcmLeftShiftBit; 

  if( ttText == TEXT_LUMA )
  {
    uiPicStride   = pcCU->getPic()->getPicYuvRec()->getStride();
    piPicReco = pcCU->getPic()->getPicYuvRec()->getLumaAddr(pcCU->getAddr(), pcCU->getZorderIdxInCU()+uiPartIdx);
    uiPcmLeftShiftBit = g_bitDepthY - pcCU->getSlice()->getSPS()->getPCMBitDepthLuma();
  }
  else
  {
    uiPicStride = pcCU->getPic()->getPicYuvRec()->getCStride();

    if( ttText == TEXT_CHROMA_U )
    {
      piPicReco = pcCU->getPic()->getPicYuvRec()->getCbAddr(pcCU->getAddr(), pcCU->getZorderIdxInCU()+uiPartIdx);
    }
    else
    {
      piPicReco = pcCU->getPic()->getPicYuvRec()->getCrAddr(pcCU->getAddr(), pcCU->getZorderIdxInCU()+uiPartIdx);
    }
    uiPcmLeftShiftBit = g_bitDepthC - pcCU->getSlice()->getSPS()->getPCMBitDepthChroma();
  }

  for( uiY = 0; uiY < uiHeight; uiY++ )
  {
    for( uiX = 0; uiX < uiWidth; uiX++ )
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
  // Luma
  UInt uiWidth  = (g_uiMaxCUWidth >> uiDepth);
  UInt uiHeight = (g_uiMaxCUHeight >> uiDepth);

  Pel* piPcmY = pcCU->getPCMSampleY();
  Pel* piRecoY = m_ppcYuvReco[uiDepth]->getLumaAddr(0, uiWidth);

  UInt uiStride = m_ppcYuvResi[uiDepth]->getStride();

  xDecodePCMTexture( pcCU, 0, piPcmY, piRecoY, uiStride, uiWidth, uiHeight, TEXT_LUMA);

  // Cb and Cr
  UInt uiCWidth  = (uiWidth>>1);
  UInt uiCHeight = (uiHeight>>1);

  Pel* piPcmCb = pcCU->getPCMSampleCb();
  Pel* piPcmCr = pcCU->getPCMSampleCr();
  Pel* pRecoCb = m_ppcYuvReco[uiDepth]->getCbAddr();
  Pel* pRecoCr = m_ppcYuvReco[uiDepth]->getCrAddr();

  UInt uiCStride = m_ppcYuvReco[uiDepth]->getCStride();

  xDecodePCMTexture( pcCU, 0, piPcmCb, pRecoCb, uiCStride, uiCWidth, uiCHeight, TEXT_CHROMA_U);
  xDecodePCMTexture( pcCU, 0, piPcmCr, pRecoCr, uiCStride, uiCWidth, uiCHeight, TEXT_CHROMA_V);
}

/** Function for filling the PCM buffer of a CU using its reconstructed sample array 
 * \param pcCU pointer to current CU
 * \param uiDepth CU Depth
 * \returns Void
 */
Void TDecCu::xFillPCMBuffer(TComDataCU* pCU, UInt depth)
{
  // Luma
  UInt width  = (g_uiMaxCUWidth >> depth);
  UInt height = (g_uiMaxCUHeight >> depth);

  Pel* pPcmY = pCU->getPCMSampleY();
  Pel* pRecoY = m_ppcYuvReco[depth]->getLumaAddr(0, width);

  UInt stride = m_ppcYuvReco[depth]->getStride();

  for(Int y = 0; y < height; y++ )
  {
    for(Int x = 0; x < width; x++ )
    {
      pPcmY[x] = pRecoY[x];
    }
    pPcmY += width;
    pRecoY += stride;
  }

  // Cb and Cr
  UInt widthC  = (width>>1);
  UInt heightC = (height>>1);

  Pel* pPcmCb = pCU->getPCMSampleCb();
  Pel* pPcmCr = pCU->getPCMSampleCr();
  Pel* pRecoCb = m_ppcYuvReco[depth]->getCbAddr();
  Pel* pRecoCr = m_ppcYuvReco[depth]->getCrAddr();

  UInt strideC = m_ppcYuvReco[depth]->getCStride();

  for(Int y = 0; y < heightC; y++ )
  {
    for(Int x = 0; x < widthC; x++ )
    {
      pPcmCb[x] = pRecoCb[x];
      pPcmCr[x] = pRecoCr[x];
    }
    pPcmCr += widthC;
    pPcmCb += widthC;
    pRecoCb += strideC;
    pRecoCr += strideC;
  }

}

//! \}
