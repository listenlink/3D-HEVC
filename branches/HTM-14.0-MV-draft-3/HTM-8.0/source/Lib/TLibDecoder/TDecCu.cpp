/* The copyright in this software is being made available under the BSD
 * License, included below. This software may be subject to other third party
 * and contributor rights, including patent rights, and no such rights are
 * granted under this license.  
 *
 * Copyright (c) 2010-2013, ITU/ISO/IEC
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
  
  UInt uiNumPartitions;
  for ( UInt ui = 0; ui < m_uiMaxDepth-1; ui++ )
  {
    uiNumPartitions = 1<<( ( m_uiMaxDepth - ui - 1 )<<1 );
    UInt uiWidth  = uiMaxWidth  >> ui;
    UInt uiHeight = uiMaxHeight >> ui;
    
    m_ppcYuvResi[ui] = new TComYuv;    m_ppcYuvResi[ui]->create( uiWidth, uiHeight );
    m_ppcYuvReco[ui] = new TComYuv;    m_ppcYuvReco[ui]->create( uiWidth, uiHeight );
    m_ppcCU     [ui] = new TComDataCU; m_ppcCU     [ui]->create( uiNumPartitions, uiWidth, uiHeight, true, uiMaxWidth >> (m_uiMaxDepth - 1) );
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
  }
  
  delete [] m_ppcYuvResi; m_ppcYuvResi = NULL;
  delete [] m_ppcYuvReco; m_ppcYuvReco = NULL;
  delete [] m_ppcCU     ; m_ppcCU      = NULL;
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
  xDecompressCU( pcCU, 0,  0 );
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
    if( pcCU->getSlice()->getVPS()->getUseAdvRP( pcCU->getSlice()->getLayerId() ) || pcCU->getSlice()->getVPS()->getIvMvPredFlag( pcCU->getSlice()->getLayerId() ))
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
#if H_3D_NBDV_REF
      if(pcCU->getSlice()->getVPS()->getDepthRefinementFlag( pcCU->getSlice()->getLayerIdInVps() ))  //Notes from QC: please check the condition for DoNBDV. Remove this comment once it is done.
        DvInfo.bDV = m_ppcCU[uiDepth]->getDisMvpCandNBDV(&DvInfo, true);
      else
#endif
        DvInfo.bDV = m_ppcCU[uiDepth]->getDisMvpCandNBDV(&DvInfo);

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

#if H_3D_VSP
    Int vspFlag[MRG_MAX_NUM_CANDS_MEM];
    memset(vspFlag, 0, sizeof(Int)*MRG_MAX_NUM_CANDS_MEM);
#if MTK_VSP_FIX_ALIGN_WD_E0172
    InheritedVSPDisInfo inheritedVSPDisInfo[MRG_MAX_NUM_CANDS_MEM];
    m_ppcCU[uiDepth]->getInterMergeCandidates( 0, 0, cMvFieldNeighbours, uhInterDirNeighbours, vspFlag, inheritedVSPDisInfo, numValidMergeCand, uiMergeIndex );
#else
#if MTK_VSP_FIX_E0172
    Int vspDir[MRG_MAX_NUM_CANDS_MEM];
    memset(vspDir, 0, sizeof(Int)*MRG_MAX_NUM_CANDS_MEM);
    m_ppcCU[uiDepth]->getInterMergeCandidates( 0, 0, cMvFieldNeighbours, uhInterDirNeighbours, vspFlag,vspDir, numValidMergeCand, uiMergeIndex );
    pcCU->setVSPDirSubParts( vspDir[uiMergeIndex], uiAbsPartIdx, 0, uiDepth );
#else
    m_ppcCU[uiDepth]->getInterMergeCandidates( 0, 0, cMvFieldNeighbours, uhInterDirNeighbours, vspFlag, numValidMergeCand, uiMergeIndex );
#endif
#endif// end of MTK_VSP_FIX_ALIGN_WD_E0172
    pcCU->setVSPFlagSubParts( vspFlag[uiMergeIndex], uiAbsPartIdx, 0, uiDepth );
#else
    m_ppcCU[uiDepth]->getInterMergeCandidates( 0, 0, cMvFieldNeighbours, uhInterDirNeighbours, numValidMergeCand, uiMergeIndex );
#endif
#if MTK_VSP_FIX_ALIGN_WD_E0172
    if(vspFlag[uiMergeIndex])
    {
      pcCU->setDvInfoSubParts(inheritedVSPDisInfo[uiMergeIndex].m_acDvInfo, uiAbsPartIdx, 0, uiDepth);
    }
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
      }
    }
#if H_3D_IC
    m_pcEntropyDecoder->decodeICFlag( pcCU, uiAbsPartIdx, uiDepth );
#endif
#if H_3D_ARP
    m_pcEntropyDecoder->decodeARPW( pcCU , uiAbsPartIdx , uiDepth );
#endif
    xFinishDecodeCU( pcCU, uiAbsPartIdx, uiDepth, ruiIsLast );
    return;
  }

  m_pcEntropyDecoder->decodePredMode( pcCU, uiAbsPartIdx, uiDepth );
  m_pcEntropyDecoder->decodePartSize( pcCU, uiAbsPartIdx, uiDepth );

  if (pcCU->isIntra( uiAbsPartIdx ) && pcCU->getPartitionSize( uiAbsPartIdx ) == SIZE_2Nx2N )
  {
    m_pcEntropyDecoder->decodeIPCMInfo( pcCU, uiAbsPartIdx, uiDepth );

    if(pcCU->getIPCMFlag(uiAbsPartIdx))
    {
      xFinishDecodeCU( pcCU, uiAbsPartIdx, uiDepth, ruiIsLast );
      return;
    }
  }

  UInt uiCurrWidth      = pcCU->getWidth ( uiAbsPartIdx );
  UInt uiCurrHeight     = pcCU->getHeight( uiAbsPartIdx );
  
  // prediction mode ( Intra : direction mode, Inter : Mv, reference idx )
  m_pcEntropyDecoder->decodePredInfo( pcCU, uiAbsPartIdx, uiDepth, m_ppcCU[uiDepth]);
#if H_3D_IC
  m_pcEntropyDecoder->decodeICFlag( pcCU, uiAbsPartIdx, uiDepth );
#endif
#if H_3D_ARP
  m_pcEntropyDecoder->decodeARPW    ( pcCU , uiAbsPartIdx , uiDepth );  
#endif  
#if LGE_INTER_SDC_E0156
  m_pcEntropyDecoder->decodeInterSDCFlag( pcCU, uiAbsPartIdx, uiDepth );
#endif
  // Coefficient decoding
  Bool bCodeDQP = getdQPFlag();
  m_pcEntropyDecoder->decodeCoeff( pcCU, uiAbsPartIdx, uiDepth, uiCurrWidth, uiCurrHeight, bCodeDQP );
  setdQPFlag( bCodeDQP );
  xFinishDecodeCU( pcCU, uiAbsPartIdx, uiDepth, ruiIsLast );
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
  
  // Residual reconstruction
  m_ppcYuvResi[uiDepth]->clear();
  
  m_ppcCU[uiDepth]->copySubCU( pcCU, uiAbsPartIdx, uiDepth );
  
  switch( m_ppcCU[uiDepth]->getPredictionMode(0) )
  {
    case MODE_INTER:
#if LGE_INTER_SDC_E0156
      if( m_ppcCU[uiDepth]->getInterSDCFlag( 0 ) )
      {
        xReconInterSDC( m_ppcCU[uiDepth], uiAbsPartIdx, uiDepth );
      }
      else
      {
#endif
      xReconInter( m_ppcCU[uiDepth], uiDepth );
#if LGE_INTER_SDC_E0156
      }
#endif
      break;
    case MODE_INTRA:
#if H_3D_DIM_SDC
      if( m_ppcCU[uiDepth]->getSDCFlag(0) )
        xReconIntraSDC( m_ppcCU[uiDepth], 0, uiDepth );
      else
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

#if LGE_INTER_SDC_E0156
Void TDecCu::xReconInterSDC( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth )
{
  // inter prediction
  m_pcPrediction->motionCompensation( pcCU, m_ppcYuvReco[uiDepth] );

  UInt  uiWidth      = pcCU->getWidth ( 0 );
  UInt  uiHeight     = pcCU->getHeight( 0 );
  UChar* pMask       = pcCU->getInterSDCMask();

  memset( pMask, 0, uiWidth*uiHeight );
  pcCU->xSetInterSDCCUMask( pcCU, pMask );

  Pel  *pResi;
  UInt uiPelX, uiPelY;
  UInt uiResiStride = m_ppcYuvResi[uiDepth]->getStride();

  pResi = m_ppcYuvResi[uiDepth]->getLumaAddr( 0 );
  for( uiPelY = 0; uiPelY < uiHeight; uiPelY++ )
  {
    for( uiPelX = 0; uiPelX < uiWidth; uiPelX++ )
    {
      UChar uiSeg = pMask[ uiPelX + uiPelY*uiWidth ];

      pResi[ uiPelX ] = pcCU->getInterSDCSegmentDCOffset( uiSeg, 0 );;
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
  
  //===== inverse transform =====
  m_pcTrQuant->setQPforQuant  ( pcCU->getQP(0), TEXT_LUMA, pcCU->getSlice()->getSPS()->getQpBDOffsetY(), 0 );

  Int scalingListType = (pcCU->isIntra(uiAbsPartIdx) ? 0 : 3) + g_eTTable[(Int)TEXT_LUMA];
  assert(scalingListType < 6);
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
  assert(scalingListType < 6);
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


Void
TDecCu::xReconIntraQT( TComDataCU* pcCU, UInt uiDepth )
{
  UInt  uiInitTrDepth = ( pcCU->getPartitionSize(0) == SIZE_2Nx2N ? 0 : 1 );
  UInt  uiNumPart     = pcCU->getNumPartInter();
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
  
  TComYuv* pcRecoYuv  = m_ppcYuvReco[uiDepth];
  TComYuv* pcPredYuv  = m_ppcYuvReco[uiDepth];
  TComYuv* pcResiYuv  = m_ppcYuvResi[uiDepth];
  
  UInt    uiStride    = pcRecoYuv->getStride  ();
  Pel*    piReco      = pcRecoYuv->getLumaAddr( uiAbsPartIdx );
  Pel*    piPred      = pcPredYuv->getLumaAddr( uiAbsPartIdx );
  Pel*    piResi      = pcResiYuv->getLumaAddr( uiAbsPartIdx );
  
  UInt    uiZOrder          = pcCU->getZorderIdxInCU() + uiAbsPartIdx;
  Pel*    piRecIPred        = pcCU->getPic()->getPicYuvRec()->getLumaAddr( pcCU->getAddr(), uiZOrder );
  UInt    uiRecIPredStride  = pcCU->getPic()->getPicYuvRec()->getStride  ();
  
  UInt    uiLumaPredMode    = pcCU->getLumaIntraDir     ( uiAbsPartIdx );
  
  AOF( uiWidth == uiHeight );
  AOF( uiAbsPartIdx == 0 );
  AOF( pcCU->getSDCAvailable(uiAbsPartIdx) );
  AOF( pcCU->getSDCFlag(uiAbsPartIdx) );
  
  //===== init availability pattern =====
  Bool  bAboveAvail = false;
  Bool  bLeftAvail  = false;
  pcCU->getPattern()->initPattern   ( pcCU, 0, uiAbsPartIdx );
  pcCU->getPattern()->initAdiPattern( pcCU, uiAbsPartIdx, 0, m_pcPrediction->getPredicBuf(), m_pcPrediction->getPredicBufWidth(), m_pcPrediction->getPredicBufHeight(), bAboveAvail, bLeftAvail );
  
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
  
  // number of segments depends on prediction mode
  UInt uiNumSegments = 1;
  Bool* pbMask = NULL;
  UInt uiMaskStride = 0;
  
  if( getDimType( uiLumaPredMode ) == DMM1_IDX )
  {
    Int uiTabIdx = pcCU->getDmmWedgeTabIdx(DMM1_IDX, uiAbsPartIdx);
    
    WedgeList* pacWedgeList = &g_dmmWedgeLists[(g_aucConvertToBit[uiWidth])];
    TComWedgelet* pcWedgelet = &(pacWedgeList->at( uiTabIdx ));
    
    uiNumSegments = 2;
    pbMask = pcWedgelet->getPattern();
    uiMaskStride = pcWedgelet->getStride();
  }
  
  // get DC prediction for each segment
  Pel apDCPredValues[2];
#if KWU_SDC_SIMPLE_DC_E0117
  m_pcPrediction->analyzeSegmentsSDC(piPred, uiStride, uiWidth, apDCPredValues, uiNumSegments, pbMask, uiMaskStride, uiLumaPredMode);
#else
  m_pcPrediction->analyzeSegmentsSDC(piPred, uiStride, uiWidth, apDCPredValues, uiNumSegments, pbMask, uiMaskStride);
#endif
  
  // reconstruct residual based on mask + DC residuals
  Pel apDCResiValues[2];
  for( UInt uiSegment = 0; uiSegment < uiNumSegments; uiSegment++ )
  {
#if H_3D_DIM_DLT
    Pel   pPredIdx    = pcCU->getSlice()->getVPS()->depthValue2idx( pcCU->getSlice()->getLayerIdInVps(), apDCPredValues[uiSegment] );
    Pel   pResiIdx    = pcCU->getSDCSegmentDCOffset(uiSegment, uiAbsPartIdx);
    Pel   pRecoValue  = pcCU->getSlice()->getVPS()->idx2DepthValue( pcCU->getSlice()->getLayerIdInVps(), pPredIdx + pResiIdx );
    
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
