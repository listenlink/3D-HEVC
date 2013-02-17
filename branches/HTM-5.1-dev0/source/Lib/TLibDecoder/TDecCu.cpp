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

/** \file     TDecCu.cpp
    \brief    CU decoder class
*/

#include "TDecCu.h"

#if RWTH_SDC_DLT_B0036
#define GetDepthValue2Idx(val)     (pcCU->getSlice()->getSPS()->depthValue2idx(val))
#define GetIdx2DepthValue(val)     (pcCU->getSlice()->getSPS()->idx2DepthValue(val))
#endif

//! \ingroup TLibDecoder
//! \{

// ====================================================================================================================
// Constructor / destructor / create / destroy
// ====================================================================================================================

TDecCu::TDecCu()
{
  m_ppcYuvResi = NULL;
  m_ppcYuvReco = NULL;
#if HHI_INTER_VIEW_RESIDUAL_PRED
  m_ppcYuvResPred = NULL;
#endif
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
#if HHI_INTER_VIEW_RESIDUAL_PRED
  m_ppcYuvResPred = new TComYuv*   [m_uiMaxDepth-1];
#endif
  m_ppcCU      = new TComDataCU*[m_uiMaxDepth-1];
  
  UInt uiNumPartitions;
  for ( UInt ui = 0; ui < m_uiMaxDepth-1; ui++ )
  {
    uiNumPartitions = 1<<( ( m_uiMaxDepth - ui - 1 )<<1 );
    UInt uiWidth  = uiMaxWidth  >> ui;
    UInt uiHeight = uiMaxHeight >> ui;
    
    m_ppcYuvResi[ui] = new TComYuv;    m_ppcYuvResi[ui]->create( uiWidth, uiHeight );
    m_ppcYuvReco[ui] = new TComYuv;    m_ppcYuvReco[ui]->create( uiWidth, uiHeight );
#if HHI_INTER_VIEW_RESIDUAL_PRED
    m_ppcYuvResPred[ui] = new TComYuv;    m_ppcYuvResPred[ui]->create( uiWidth, uiHeight );
#endif
    m_ppcCU     [ui] = new TComDataCU; m_ppcCU     [ui]->create( uiNumPartitions, uiWidth, uiHeight, true, uiMaxWidth >> (m_uiMaxDepth - 1) );
  }
  
  m_bDecodeDQP = false;

  // initialize partition order.
  UInt* piTmp = &g_auiZscanToRaster[0];
  initZscanToRaster(m_uiMaxDepth, 1, 0, piTmp);
  initRasterToZscan( uiMaxWidth, uiMaxHeight, m_uiMaxDepth );
  
  // initialize conversion matrix from partition index to pel
  initRasterToPelXY( uiMaxWidth, uiMaxHeight, m_uiMaxDepth );
  initMotionReferIdx ( uiMaxWidth, uiMaxHeight, m_uiMaxDepth );
}

Void TDecCu::destroy()
{
  for ( UInt ui = 0; ui < m_uiMaxDepth-1; ui++ )
  {
    m_ppcYuvResi[ui]->destroy(); delete m_ppcYuvResi[ui]; m_ppcYuvResi[ui] = NULL;
    m_ppcYuvReco[ui]->destroy(); delete m_ppcYuvReco[ui]; m_ppcYuvReco[ui] = NULL;
#if HHI_INTER_VIEW_RESIDUAL_PRED
    m_ppcYuvResPred[ui]->destroy(); delete m_ppcYuvResPred[ui]; m_ppcYuvResPred[ui] = NULL;
#endif
    m_ppcCU     [ui]->destroy(); delete m_ppcCU     [ui]; m_ppcCU     [ui] = NULL;
  }
  
  delete [] m_ppcYuvResi; m_ppcYuvResi = NULL;
  delete [] m_ppcYuvReco; m_ppcYuvReco = NULL;
#if HHI_INTER_VIEW_RESIDUAL_PRED
  delete [] m_ppcYuvResPred; m_ppcYuvResPred = NULL;
#endif
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

#if BURST_IPCM
  pcCU->setNumSucIPCM(0);
#endif

  // start from the top level CU
  xDecodeCU( pcCU, 0, 0, ruiIsLast);
}

/** \param    pcCU        pointer of CU data
 */
Void TDecCu::decompressCU( TComDataCU* pcCU )
{
  xDecompressCU( pcCU, pcCU, 0,  0 );
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
Bool TDecCu::xDecodeSliceEnd( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth) {
  UInt uiIsLast;
  TComPic* pcPic = pcCU->getPic();
  TComSlice * pcSlice = pcPic->getSlice(pcPic->getCurrSliceIdx());
  UInt uiCurNumParts    = pcPic->getNumPartInCU() >> (uiDepth<<1);
  UInt uiWidth = pcSlice->getSPS()->getPicWidthInLumaSamples();
  UInt uiHeight = pcSlice->getSPS()->getPicHeightInLumaSamples();
  UInt uiGranularityWidth = g_uiMaxCUWidth>>(pcSlice->getPPS()->getSliceGranularity());
  UInt uiPosX = pcCU->getCUPelX() + g_auiRasterToPelX[ g_auiZscanToRaster[uiAbsPartIdx] ];
  UInt uiPosY = pcCU->getCUPelY() + g_auiRasterToPelY[ g_auiZscanToRaster[uiAbsPartIdx] ];

#if HHI_MPI
  const UInt uiCUWidth  = pcCU->getTextureModeDepth( uiAbsPartIdx ) != -1 ? g_uiMaxCUWidth>>uiDepth  : pcCU->getWidth (uiAbsPartIdx);
  const UInt uiCUHeight = pcCU->getTextureModeDepth( uiAbsPartIdx ) != -1 ? g_uiMaxCUHeight>>uiDepth : pcCU->getHeight(uiAbsPartIdx);
  if(((uiPosX+uiCUWidth)%uiGranularityWidth==0||(uiPosX+uiCUWidth==uiWidth))
    &&((uiPosY+uiCUHeight)%uiGranularityWidth==0||(uiPosY+uiCUHeight==uiHeight)))
#else
  if(((uiPosX+pcCU->getWidth(uiAbsPartIdx))%uiGranularityWidth==0||(uiPosX+pcCU->getWidth(uiAbsPartIdx)==uiWidth))
    &&((uiPosY+pcCU->getHeight(uiAbsPartIdx))%uiGranularityWidth==0||(uiPosY+pcCU->getHeight(uiAbsPartIdx)==uiHeight)))
#endif
  {
    m_pcEntropyDecoder->decodeTerminatingBit( uiIsLast );
  }
  else
  {
    uiIsLast=0;
  }
  
  if(uiIsLast) 
  {
    if(pcSlice->isNextEntropySlice()&&!pcSlice->isNextSlice()) 
    {
      pcSlice->setEntropySliceCurEndCUAddr(pcCU->getSCUAddr()+uiAbsPartIdx+uiCurNumParts);
    }
    else 
    {
      pcSlice->setSliceCurEndCUAddr(pcCU->getSCUAddr()+uiAbsPartIdx+uiCurNumParts);
      pcSlice->setEntropySliceCurEndCUAddr(pcCU->getSCUAddr()+uiAbsPartIdx+uiCurNumParts);
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

  TComSlice * pcSlice = pcCU->getPic()->getSlice(pcCU->getPic()->getCurrSliceIdx());
  Bool bStartInCU = pcCU->getSCUAddr()+uiAbsPartIdx+uiCurNumParts>pcSlice->getEntropySliceCurStartCUAddr()&&pcCU->getSCUAddr()+uiAbsPartIdx<pcSlice->getEntropySliceCurStartCUAddr();
  if((!bStartInCU) && ( uiRPelX < pcSlice->getSPS()->getPicWidthInLumaSamples() ) && ( uiBPelY < pcSlice->getSPS()->getPicHeightInLumaSamples() ) )
  {
#if BURST_IPCM
    if(pcCU->getNumSucIPCM() == 0)
    {
#if HHI_MPI
      if( pcCU->getTextureModeDepth( uiAbsPartIdx ) == -1 || uiDepth < pcCU->getTextureModeDepth( uiAbsPartIdx ) )
#endif
      m_pcEntropyDecoder->decodeSplitFlag( pcCU, uiAbsPartIdx, uiDepth );
    }
    else
    {
      pcCU->setDepthSubParts( uiDepth, uiAbsPartIdx );
    }
#else
#if HHI_MPI
    if( pcCU->getTextureModeDepth( uiAbsPartIdx ) == -1 || uiDepth < pcCU->getTextureModeDepth( uiAbsPartIdx ) )
#endif
    m_pcEntropyDecoder->decodeSplitFlag( pcCU, uiAbsPartIdx, uiDepth );
#endif
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
      
      Bool bSubInSlice = pcCU->getSCUAddr()+uiIdx+uiQNumParts>pcSlice->getEntropySliceCurStartCUAddr();
      if ( bSubInSlice )
      {
        if ( ( uiLPelX < pcCU->getSlice()->getSPS()->getPicWidthInLumaSamples() ) && ( uiTPelY < pcCU->getSlice()->getSPS()->getPicHeightInLumaSamples() ) )
        {
          xDecodeCU( pcCU, uiIdx, uiDepth+1, ruiIsLast );
        }
        else
        {
          pcCU->setOutsideCUPart( uiIdx, uiDepth+1 );
        }
      }
      if(ruiIsLast)
      {
        break;
      }
      
      uiIdx += uiQNumParts;
    }
    if( (g_uiMaxCUWidth>>uiDepth) == pcCU->getSlice()->getPPS()->getMinCuDQPSize() && pcCU->getSlice()->getPPS()->getUseDQP())
    {
      if ( getdQPFlag() )
      {
        UInt uiQPSrcPartIdx;
        if ( pcPic->getCU( pcCU->getAddr() )->getEntropySliceStartCU(uiAbsPartIdx) != pcSlice->getEntropySliceCurStartCUAddr() )
        {
          uiQPSrcPartIdx = pcSlice->getEntropySliceCurStartCUAddr() % pcPic->getNumPartInCU();
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
  
  if( (g_uiMaxCUWidth>>uiDepth) >= pcCU->getSlice()->getPPS()->getMinCuDQPSize() && pcCU->getSlice()->getPPS()->getUseDQP())
  {
    setdQPFlag(true);
    pcCU->setQPSubParts( pcCU->getRefQP(uiAbsPartIdx), uiAbsPartIdx, uiDepth ); // set QP to default QP
  }

  // decode CU mode and the partition size
#if BURST_IPCM
  if( !pcCU->getSlice()->isIntra() && pcCU->getNumSucIPCM() == 0 )
#else
  if( !pcCU->getSlice()->isIntra() )
#endif
#if HHI_MPI
  if( pcCU->getTextureModeDepth( uiAbsPartIdx ) == -1 )
#endif
  {
    m_pcEntropyDecoder->decodeSkipFlag( pcCU, uiAbsPartIdx, uiDepth );
  }
 
  if( pcCU->isSkipped(uiAbsPartIdx) )
  {
    m_ppcCU[uiDepth]->copyInterPredInfoFrom( pcCU, uiAbsPartIdx, REF_PIC_LIST_0 );
    m_ppcCU[uiDepth]->copyInterPredInfoFrom( pcCU, uiAbsPartIdx, REF_PIC_LIST_1 );
#if HHI_INTER_VIEW_MOTION_PRED
    TComMvField cMvFieldNeighbours[MRG_MAX_NUM_CANDS_MEM << 1]; // double length for mv of both lists
    UChar uhInterDirNeighbours[MRG_MAX_NUM_CANDS_MEM];
    Int numValidMergeCand = 0;
    for( UInt ui = 0; ui < MRG_MAX_NUM_CANDS_MEM; ++ui )
#else
    TComMvField cMvFieldNeighbours[MRG_MAX_NUM_CANDS << 1]; // double length for mv of both lists
    UChar uhInterDirNeighbours[MRG_MAX_NUM_CANDS];
    Int numValidMergeCand = 0;
    for( UInt ui = 0; ui < MRG_MAX_NUM_CANDS; ++ui )
#endif
    {
      uhInterDirNeighbours[ui] = 0;
    }
    m_pcEntropyDecoder->decodeMergeIndex( pcCU, 0, uiAbsPartIdx, SIZE_2Nx2N, uhInterDirNeighbours, cMvFieldNeighbours, uiDepth );
#if HHI_MPI
    if( pcCU->getTextureModeDepth( uiAbsPartIdx ) == uiDepth )
    {
      TComDataCU *pcTextureCU = pcCU->getSlice()->getTexturePic()->getCU( pcCU->getAddr() );
      pcCU->copyTextureMotionDataFrom( pcTextureCU, uiDepth, pcCU->getZorderIdxInCU() + uiAbsPartIdx, uiAbsPartIdx );

      UInt uiCurrPartNumb = pcCU->getPic()->getNumPartInCU() >> (uiDepth << 1);

      for( UInt ui = 0; ui < uiCurrPartNumb; ui++ )
      {
        const UChar uhNewDepth = max<UInt>( uiDepth, pcTextureCU->getDepth( uiAbsPartIdx + ui ) );
#if MERL_VSP_C0152
        Int vspIdx = pcTextureCU->getVSPIndex( uiAbsPartIdx + ui);
        pcCU->setVSPIndex( uiAbsPartIdx + ui, vspIdx);
#endif
        pcCU->setPredictionMode( uiAbsPartIdx + ui, MODE_SKIP );
        pcCU->setPartitionSize( uiAbsPartIdx + ui, SIZE_2Nx2N );
        pcCU->setDepth( uiAbsPartIdx + ui, uhNewDepth );
        pcCU->setWidth( uiAbsPartIdx + ui, g_uiMaxCUWidth>>uhNewDepth );
        pcCU->setHeight( uiAbsPartIdx + ui, g_uiMaxCUHeight>>uhNewDepth );
      }
#if LGE_ILLUCOMP_DEPTH_C0046
      m_pcEntropyDecoder->decodeICFlag( pcCU, uiAbsPartIdx, uiDepth );
#endif
    }
    else
    {
#endif
#if SIMP_MRG_PRUN      
    UInt uiMergeIndex = pcCU->getMergeIndex(uiAbsPartIdx);
#if MERL_VSP_C0152
    Int iVSPIndexTrue[3] = {-1, -1, -1};
    m_ppcCU[uiDepth]->getInterMergeCandidates( 0, 0, uiDepth, cMvFieldNeighbours, uhInterDirNeighbours, numValidMergeCand, iVSPIndexTrue, uiMergeIndex );
    {
      Int iVSPIdx = 0;
      Int numVspIdx;
      numVspIdx = 3;
      for (Int i = 0; i < numVspIdx; i++)
      {
        if (iVSPIndexTrue[i] == uiMergeIndex)
          {
            iVSPIdx = i+1;
            break;
          }
      }
      pcCU->setVSPIndexSubParts( iVSPIdx, uiAbsPartIdx, 0, uiDepth );  //Initialize the VSP, may change later in get InterMergeCandidates()
    }
#else
    m_ppcCU[uiDepth]->getInterMergeCandidates( 0, 0, uiDepth, cMvFieldNeighbours, uhInterDirNeighbours, numValidMergeCand, uiMergeIndex );
#endif
#else
    m_ppcCU[uiDepth]->getInterMergeCandidates( 0, 0, uiDepth, cMvFieldNeighbours, uhInterDirNeighbours, numValidMergeCand );
    UInt uiMergeIndex = pcCU->getMergeIndex(uiAbsPartIdx);
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
#if LGE_ILLUCOMP_B0045
    m_pcEntropyDecoder->decodeICFlag( pcCU, uiAbsPartIdx, uiDepth );
#endif
#if HHI_MPI
    }
#endif
#if HHI_INTER_VIEW_RESIDUAL_PRED && !MTK_MDIVRP_C0138
    m_pcEntropyDecoder->decodeResPredFlag( pcCU, uiAbsPartIdx, uiDepth, m_ppcCU[uiDepth], 0 );
#endif
    xFinishDecodeCU( pcCU, uiAbsPartIdx, uiDepth, ruiIsLast );
    return;
  }

#if HHI_MPI
  if( pcCU->getTextureModeDepth( uiAbsPartIdx ) == -1 )
  {
#endif
#if BURST_IPCM
  if( pcCU->getNumSucIPCM() == 0 ) 
  {
    m_pcEntropyDecoder->decodePredMode( pcCU, uiAbsPartIdx, uiDepth );
    m_pcEntropyDecoder->decodePartSize( pcCU, uiAbsPartIdx, uiDepth );
  }
  else
  {
    pcCU->setPredModeSubParts( MODE_INTRA, uiAbsPartIdx, uiDepth );
    pcCU->setPartSizeSubParts( SIZE_2Nx2N, uiAbsPartIdx, uiDepth );
    pcCU->setSizeSubParts( g_uiMaxCUWidth>>uiDepth, g_uiMaxCUHeight>>uiDepth, uiAbsPartIdx, uiDepth ); 
    pcCU->setTrIdxSubParts( 0, uiAbsPartIdx, uiDepth );
  }
#else
  m_pcEntropyDecoder->decodePredMode( pcCU, uiAbsPartIdx, uiDepth );
  m_pcEntropyDecoder->decodePartSize( pcCU, uiAbsPartIdx, uiDepth );
#endif

  if (pcCU->isIntra( uiAbsPartIdx ) && pcCU->getPartitionSize( uiAbsPartIdx ) == SIZE_2Nx2N )
  {
    m_pcEntropyDecoder->decodeIPCMInfo( pcCU, uiAbsPartIdx, uiDepth );

    if(pcCU->getIPCMFlag(uiAbsPartIdx))
    {
      xFinishDecodeCU( pcCU, uiAbsPartIdx, uiDepth, ruiIsLast );
      return;
    }
  }

#if ! HHI_MPI
  UInt uiCurrWidth      = pcCU->getWidth ( uiAbsPartIdx );
  UInt uiCurrHeight     = pcCU->getHeight( uiAbsPartIdx );
#endif
  
  // prediction mode ( Intra : direction mode, Inter : Mv, reference idx )
  m_pcEntropyDecoder->decodePredInfo( pcCU, uiAbsPartIdx, uiDepth, m_ppcCU[uiDepth]);
  
#if LGE_ILLUCOMP_B0045
#if LGE_ILLUCOMP_DEPTH_C0046 && HHI_MPI
  if( pcCU->getTextureModeDepth( uiAbsPartIdx ) != uiDepth )
  {
#endif
  m_pcEntropyDecoder->decodeICFlag( pcCU, uiAbsPartIdx, uiDepth );
#endif

#if HHI_INTER_VIEW_RESIDUAL_PRED && !MTK_MDIVRP_C0138
  if( !pcCU->isIntra( uiAbsPartIdx ) )
  {
    m_pcEntropyDecoder->decodeResPredFlag    ( pcCU, uiAbsPartIdx, uiDepth, m_ppcCU[uiDepth], 0 );
  }
#endif
#if LGE_ILLUCOMP_DEPTH_C0046 && HHI_MPI
  }
#endif

#if HHI_MPI
    if( pcCU->getTextureModeDepth( uiAbsPartIdx ) == uiDepth )
    {
      assert( pcCU->getZorderIdxInCU() == 0 );
      TComDataCU *pcTextureCU = pcCU->getSlice()->getTexturePic()->getCU( pcCU->getAddr() );
      pcCU->copyTextureMotionDataFrom( pcTextureCU, uiDepth, pcCU->getZorderIdxInCU() + uiAbsPartIdx, uiAbsPartIdx );

      UInt uiCurrPartNumb = pcCU->getPic()->getNumPartInCU() >> (uiDepth << 1);

      for( UInt ui = 0; ui < uiCurrPartNumb; ui++ )
      {
        const UChar uhNewDepth = max<UInt>( uiDepth, pcTextureCU->getDepth( uiAbsPartIdx + ui ) );
#if MERL_VSP_C0152
        Int vspIdx = pcTextureCU->getVSPIndex( uiAbsPartIdx + ui);
        pcCU->setVSPIndex( uiAbsPartIdx + ui, vspIdx);
#endif
        pcCU->setPredictionMode( uiAbsPartIdx + ui, MODE_INTER );
        pcCU->setPartitionSize( uiAbsPartIdx + ui, SIZE_2Nx2N );
        pcCU->setDepth( uiAbsPartIdx + ui, uhNewDepth );
        pcCU->setWidth( uiAbsPartIdx + ui, g_uiMaxCUWidth>>uhNewDepth );
        pcCU->setHeight( uiAbsPartIdx + ui, g_uiMaxCUHeight>>uhNewDepth );
      }
#if LGE_ILLUCOMP_DEPTH_C0046
      m_pcEntropyDecoder->decodeICFlag( pcCU, uiAbsPartIdx, uiDepth );
#endif
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

          Bool bSubInSlice = pcCU->getSCUAddr()+uiIdx+uiQNumParts>pcSlice->getEntropySliceCurStartCUAddr();
          if ( bSubInSlice )
          {
            if ( ( uiLPelX < pcCU->getSlice()->getSPS()->getPicWidthInLumaSamples() ) && ( uiTPelY < pcCU->getSlice()->getSPS()->getPicHeightInLumaSamples() ) )
            {
              xDecodeCU( pcCU, uiIdx, uiDepth+1, ruiIsLast );
            }
            else
            {
              pcCU->setOutsideCUPart( uiIdx, uiDepth+1 );
            }
          }
          if(ruiIsLast)
          {
            break;
          }
          uiIdx += uiQNumParts;
        }
        if( (g_uiMaxCUWidth>>uiDepth) == pcCU->getSlice()->getPPS()->getMinCuDQPSize() && pcCU->getSlice()->getPPS()->getUseDQP())
        {
          if ( getdQPFlag() )
          {
            UInt uiQPSrcPartIdx;
            if ( pcPic->getCU( pcCU->getAddr() )->getEntropySliceStartCU(uiAbsPartIdx) != pcSlice->getEntropySliceCurStartCUAddr() )
            {
              uiQPSrcPartIdx = pcSlice->getEntropySliceCurStartCUAddr() % pcPic->getNumPartInCU();
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
    }
  }

  UInt uiCurrWidth      = pcCU->getWidth ( uiAbsPartIdx );
  UInt uiCurrHeight     = pcCU->getHeight( uiAbsPartIdx );
#endif

  // Coefficient decoding
  Bool bCodeDQP = getdQPFlag();
  m_pcEntropyDecoder->decodeCoeff( pcCU, uiAbsPartIdx, uiDepth, uiCurrWidth, uiCurrHeight, bCodeDQP );
  setdQPFlag( bCodeDQP );
  xFinishDecodeCU( pcCU, uiAbsPartIdx, uiDepth, ruiIsLast );
}

Void TDecCu::xFinishDecodeCU( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth, UInt& ruiIsLast)
{
  if( (g_uiMaxCUWidth>>uiDepth) >= pcCU->getSlice()->getPPS()->getMinCuDQPSize() && pcCU->getSlice()->getPPS()->getUseDQP())
  {
    if( getdQPFlag() )
    {
      pcCU->setQPSubParts( pcCU->getRefQP(uiAbsPartIdx), uiAbsPartIdx, uiDepth ); // set QP to default QP
    }
  }

#if BURST_IPCM
  if( pcCU->getNumSucIPCM() > 0 )
  {
    ruiIsLast = 0;
    return;
  }
#endif

  ruiIsLast = xDecodeSliceEnd( pcCU, uiAbsPartIdx, uiDepth);
}

Void TDecCu::xDecompressCU( TComDataCU* pcCU, TComDataCU* pcCUCur, UInt uiAbsPartIdx,  UInt uiDepth )
{
  TComPic* pcPic = pcCU->getPic();
  
  Bool bBoundary = false;
  UInt uiLPelX   = pcCU->getCUPelX() + g_auiRasterToPelX[ g_auiZscanToRaster[uiAbsPartIdx] ];
  UInt uiRPelX   = uiLPelX + (g_uiMaxCUWidth>>uiDepth)  - 1;
  UInt uiTPelY   = pcCU->getCUPelY() + g_auiRasterToPelY[ g_auiZscanToRaster[uiAbsPartIdx] ];
  UInt uiBPelY   = uiTPelY + (g_uiMaxCUHeight>>uiDepth) - 1;
  
  UInt uiCurNumParts    = pcPic->getNumPartInCU() >> (uiDepth<<1);
  TComSlice * pcSlice = pcCU->getPic()->getSlice(pcCU->getPic()->getCurrSliceIdx());
  Bool bStartInCU = pcCU->getSCUAddr()+uiAbsPartIdx+uiCurNumParts>pcSlice->getEntropySliceCurStartCUAddr()&&pcCU->getSCUAddr()+uiAbsPartIdx<pcSlice->getEntropySliceCurStartCUAddr();
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
      
      Bool binSlice = (pcCU->getSCUAddr()+uiIdx+uiQNumParts>pcSlice->getEntropySliceCurStartCUAddr())&&(pcCU->getSCUAddr()+uiIdx<pcSlice->getEntropySliceCurEndCUAddr());
      if(binSlice&&( uiLPelX < pcSlice->getSPS()->getPicWidthInLumaSamples() ) && ( uiTPelY < pcSlice->getSPS()->getPicHeightInLumaSamples() ) )
      {
        xDecompressCU(pcCU, m_ppcCU[uiNextDepth], uiIdx, uiNextDepth );
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
    case MODE_SKIP:
    case MODE_INTER:
      xReconInter( m_ppcCU[uiDepth], uiAbsPartIdx, uiDepth );
      break;
    case MODE_INTRA:
#if RWTH_SDC_DLT_B0036
      if( m_ppcCU[uiDepth]->getSDCFlag(0) )
        xReconIntraSDC( m_ppcCU[uiDepth], 0, uiDepth );
      else
#endif
      xReconIntraQT( m_ppcCU[uiDepth], uiAbsPartIdx, uiDepth );
      break;
    default:
      assert(0);
      break;
  }
#if LOSSLESS_CODING 
  if ( m_ppcCU[uiDepth]->isLosslessCoded(0) && (m_ppcCU[uiDepth]->getIPCMFlag(0) == false))
  {
    xFillPCMBuffer(m_ppcCU[uiDepth], uiAbsPartIdx, uiDepth);    
  }
#endif
  
  xCopyToPic( m_ppcCU[uiDepth], pcPic, uiAbsPartIdx, uiDepth );
}

Void TDecCu::xReconInter( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth )
{
#if HHI_MPI
#if FIX_MPI_B0065
  if( pcCU->getTextureModeDepth( 0 ) != -1 )
  {
    TComDataCU *pcTextureCU = pcCU->getSlice()->getTexturePic()->getCU( pcCU->getAddr() );
    if( uiDepth == pcTextureCU->getDepth(uiAbsPartIdx))
    {
      PartSize partSize = pcTextureCU->getPartitionSize(uiAbsPartIdx);
      pcCU->setPartSizeSubParts( partSize, 0, uiDepth );
    }
    else
    {
      pcCU->setPartSizeSubParts( SIZE_NxN, 0, uiDepth );
    }
  }
#else
  if( pcCU->getTextureModeDepth( 0 ) != -1 )
    pcCU->setPartSizeSubParts( SIZE_NxN, 0, uiDepth );
#endif
#endif
  
  // inter prediction
#if MERL_VSP_C0152
  m_pcPrediction->motionCompensation( pcCU, m_ppcYuvReco[uiDepth], uiAbsPartIdx );
#else
  m_pcPrediction->motionCompensation( pcCU, m_ppcYuvReco[uiDepth] );
#endif
#if MTK_MDIVRP_C0138
  if (pcCU->getMergeFlag(0) && pcCU->getMergeIndex(0)==0 && pcCU->getResPredAvail(0))
  {
    m_pcPrediction->residualPrediction(pcCU, m_ppcYuvReco[uiDepth], m_ppcYuvResPred[uiDepth]);
  }
#endif

#if HHI_MPI
  if( pcCU->getTextureModeDepth( 0 ) != -1 )
    pcCU->setPartSizeSubParts( SIZE_2Nx2N, 0, uiDepth );
#endif

#if HHI_INTER_VIEW_RESIDUAL_PRED && !MTK_MDIVRP_C0138
  if( pcCU->getResPredFlag( 0 ) )
  {
    AOF( pcCU->getResPredAvail( 0 ) );
    Bool bOK = pcCU->getResidualSamples( 0, 
#if QC_SIMPLIFIEDIVRP_M24938
      true,
#endif
      m_ppcYuvResPred[uiDepth] );
    AOF( bOK );
#if LG_RESTRICTEDRESPRED_M24766
    Int iPUResiPredShift[4];
    pcCU->getPUResiPredShift(iPUResiPredShift, 0);
    m_ppcYuvReco[uiDepth]->add(iPUResiPredShift, pcCU->getPartitionSize(0), m_ppcYuvResPred[uiDepth], pcCU->getWidth( 0 ), pcCU->getHeight( 0 ) );
#else
    m_ppcYuvReco[uiDepth]->add( m_ppcYuvResPred[uiDepth], pcCU->getWidth( 0 ), pcCU->getHeight( 0 ) );
#endif
  }
#endif

  // inter recon
  xDecodeInterTexture( pcCU, 0, uiDepth );
  
  // clip for only non-zero cbp case
  if  ( ( pcCU->getCbf( 0, TEXT_LUMA ) ) || ( pcCU->getCbf( 0, TEXT_CHROMA_U ) ) || ( pcCU->getCbf(0, TEXT_CHROMA_V ) ) )
  {
    m_ppcYuvReco[uiDepth]->addClip( m_ppcYuvReco[uiDepth], m_ppcYuvResi[uiDepth], 0, pcCU->getWidth( 0 ) );
  }
  else
  {
#if HHI_INTER_VIEW_RESIDUAL_PRED
#if MTK_MDIVRP_C0138
    if (pcCU->getMergeFlag(0) && pcCU->getMergeIndex(0)==0 && pcCU->getResPredAvail(0))
#else
    if( pcCU->getResPredFlag( 0 ) )
#endif
    {
      m_ppcYuvReco[uiDepth]->clip( pcCU->getWidth( 0 ), pcCU->getHeight( 0 ) );
    }
#endif
    m_ppcYuvReco[uiDepth]->copyPartToPartYuv( m_ppcYuvReco[uiDepth],0, pcCU->getWidth( 0 ),pcCU->getHeight( 0 ));
  }
}

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
  
  //===== init availability pattern =====
  Bool  bAboveAvail = false;
  Bool  bLeftAvail  = false;
  pcCU->getPattern()->initPattern   ( pcCU, uiTrDepth, uiAbsPartIdx );
  pcCU->getPattern()->initAdiPattern( pcCU, uiAbsPartIdx, uiTrDepth, 
                                     m_pcPrediction->getPredicBuf       (),
                                     m_pcPrediction->getPredicBufWidth  (),
                                     m_pcPrediction->getPredicBufHeight (),
                                     bAboveAvail, bLeftAvail );
#if LGE_EDGE_INTRA_A0070
  if( uiLumaPredMode >= EDGE_INTRA_IDX )
  {
    m_pcPrediction->predIntraLumaEdge( pcCU, pcCU->getPattern(), uiAbsPartIdx, uiWidth, uiHeight, piPred, uiStride
#if LGE_EDGE_INTRA_DELTA_DC
      , uiLumaPredMode == EDGE_INTRA_DELTA_IDX
#endif
      );
  } 
  else
#endif
  
  //===== get prediction signal =====
#if HHI_DMM_WEDGE_INTRA || HHI_DMM_PRED_TEX
  if( uiLumaPredMode >= NUM_INTRA_MODE )
  {
    m_pcPrediction->predIntraLumaDMM( pcCU, uiAbsPartIdx, uiLumaPredMode, piPred, uiStride, uiWidth, uiHeight, bAboveAvail, bLeftAvail, false );
  } 
  else
  {
#endif
  m_pcPrediction->predIntraLumaAng( pcCU->getPattern(), uiLumaPredMode, piPred, uiStride, uiWidth, uiHeight, pcCU, bAboveAvail, bLeftAvail );
#if HHI_DMM_WEDGE_INTRA || HHI_DMM_PRED_TEX
  }
#endif
  
  //===== inverse transform =====
#if H0736_AVC_STYLE_QP_RANGE
  m_pcTrQuant->setQPforQuant  ( pcCU->getQP(0), !pcCU->getSlice()->getDepth(), pcCU->getSlice()->getSliceType(), TEXT_LUMA, pcCU->getSlice()->getSPS()->getQpBDOffsetY(), 0 );
#else
  m_pcTrQuant->setQPforQuant  ( pcCU->getQP(0), !pcCU->getSlice()->getDepth(), pcCU->getSlice()->getSliceType(), TEXT_LUMA, 0 );
#endif

  Int scalingListType = (pcCU->isIntra(uiAbsPartIdx) ? 0 : 3) + g_eTTable[(Int)TEXT_LUMA];
  assert(scalingListType < 6);
#if LOSSLESS_CODING
  m_pcTrQuant->invtransformNxN( pcCU, TEXT_LUMA, pcCU->getLumaIntraDir( uiAbsPartIdx ), piResi, uiStride, pcCoeff, uiWidth, uiHeight, scalingListType );
#else  
  m_pcTrQuant->invtransformNxN(       TEXT_LUMA, pcCU->getLumaIntraDir( uiAbsPartIdx ), piResi, uiStride, pcCoeff, uiWidth, uiHeight, scalingListType );
#endif

  
  //===== reconstruction =====
  Pel* pPred      = piPred;
  Pel* pResi      = piResi;
  Pel* pReco      = piReco;
  Pel* pRecIPred  = piRecIPred;
  for( UInt uiY = 0; uiY < uiHeight; uiY++ )
  {
    for( UInt uiX = 0; uiX < uiWidth; uiX++ )
    {
      pReco    [ uiX ] = Clip( pPred[ uiX ] + pResi[ uiX ] );
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
  
  //===== init availability pattern =====
  Bool  bAboveAvail = false;
  Bool  bLeftAvail  = false;
  pcCU->getPattern()->initPattern         ( pcCU, uiTrDepth, uiAbsPartIdx );

  if( uiChromaPredMode == LM_CHROMA_IDX && uiChromaId == 0 )
  {
    pcCU->getPattern()->initAdiPattern( pcCU, uiAbsPartIdx, uiTrDepth, 
                                     m_pcPrediction->getPredicBuf       (),
                                     m_pcPrediction->getPredicBufWidth  (),
                                     m_pcPrediction->getPredicBufHeight (),
                                     bAboveAvail, bLeftAvail, 
                                     true );

    m_pcPrediction->getLumaRecPixels( pcCU->getPattern(), uiWidth, uiHeight );
  }
  
  pcCU->getPattern()->initAdiPatternChroma( pcCU, uiAbsPartIdx, uiTrDepth, 
                                           m_pcPrediction->getPredicBuf       (),
                                           m_pcPrediction->getPredicBufWidth  (),
                                           m_pcPrediction->getPredicBufHeight (),
                                           bAboveAvail, bLeftAvail );
  Int* pPatChroma   = ( uiChromaId > 0 ? pcCU->getPattern()->getAdiCrBuf( uiWidth, uiHeight, m_pcPrediction->getPredicBuf() ) : pcCU->getPattern()->getAdiCbBuf( uiWidth, uiHeight, m_pcPrediction->getPredicBuf() ) );
  
  //===== get prediction signal =====
  if( uiChromaPredMode == LM_CHROMA_IDX )
  {
    m_pcPrediction->predLMIntraChroma( pcCU->getPattern(), pPatChroma, piPred, uiStride, uiWidth, uiHeight, uiChromaId );
  }
  else
  {
    if( uiChromaPredMode == DM_CHROMA_IDX )
    {
      uiChromaPredMode = pcCU->getLumaIntraDir( 0 );
#if HHI_DMM_WEDGE_INTRA || HHI_DMM_PRED_TEX
      mapDMMtoIntraMode( uiChromaPredMode );
#endif
    }
    m_pcPrediction->predIntraChromaAng( pcCU->getPattern(), pPatChroma, uiChromaPredMode, piPred, uiStride, uiWidth, uiHeight, pcCU, bAboveAvail, bLeftAvail );  
  }

  //===== inverse transform =====
  if(eText == TEXT_CHROMA_U)
  {
#if H0736_AVC_STYLE_QP_RANGE
    m_pcTrQuant->setQPforQuant  ( pcCU->getQP(0), !pcCU->getSlice()->getDepth(), pcCU->getSlice()->getSliceType(), eText, pcCU->getSlice()->getSPS()->getQpBDOffsetC(), pcCU->getSlice()->getPPS()->getChromaQpOffset() );
#else
    m_pcTrQuant->setQPforQuant  ( pcCU->getQP(0), !pcCU->getSlice()->getDepth(), pcCU->getSlice()->getSliceType(), eText, pcCU->getSlice()->getPPS()->getChromaQpOffset() );
#endif
  }
  else
  {
#if H0736_AVC_STYLE_QP_RANGE
    m_pcTrQuant->setQPforQuant  ( pcCU->getQP(0), !pcCU->getSlice()->getDepth(), pcCU->getSlice()->getSliceType(), eText, pcCU->getSlice()->getSPS()->getQpBDOffsetC(), pcCU->getSlice()->getPPS()->getChromaQpOffset2nd() );
#else
    m_pcTrQuant->setQPforQuant  ( pcCU->getQP(0), !pcCU->getSlice()->getDepth(), pcCU->getSlice()->getSliceType(), eText, pcCU->getSlice()->getPPS()->getChromaQpOffset2nd() );
#endif
  }

  Int scalingListType = (pcCU->isIntra(uiAbsPartIdx) ? 0 : 3) + g_eTTable[(Int)eText];
  assert(scalingListType < 6);
#if LOSSLESS_CODING
  m_pcTrQuant->invtransformNxN( pcCU, eText, REG_DCT, piResi, uiStride, pcCoeff, uiWidth, uiHeight, scalingListType );
#else  
  m_pcTrQuant->invtransformNxN(       eText, REG_DCT, piResi, uiStride, pcCoeff, uiWidth, uiHeight, scalingListType );
#endif

  //===== reconstruction =====
  Pel* pPred      = piPred;
  Pel* pResi      = piResi;
  Pel* pReco      = piReco;
  Pel* pRecIPred  = piRecIPred;
  for( UInt uiY = 0; uiY < uiHeight; uiY++ )
  {
    for( UInt uiX = 0; uiX < uiWidth; uiX++ )
    {
      pReco    [ uiX ] = Clip( pPred[ uiX ] + pResi[ uiX ] );
      pRecIPred[ uiX ] = pReco[ uiX ];
    }
    pPred     += uiStride;
    pResi     += uiStride;
    pReco     += uiStride;
    pRecIPred += uiRecIPredStride;
  }
}

Void
TDecCu::xIntraRecQT( TComDataCU* pcCU,
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
    xIntraRecChromaBlk( pcCU, uiTrDepth, uiAbsPartIdx, pcRecoYuv, pcPredYuv, pcResiYuv, 0 );
    xIntraRecChromaBlk( pcCU, uiTrDepth, uiAbsPartIdx, pcRecoYuv, pcPredYuv, pcResiYuv, 1 );
  }
  else
  {
    UInt uiNumQPart  = pcCU->getPic()->getNumPartInCU() >> ( ( uiFullDepth + 1 ) << 1 );
    for( UInt uiPart = 0; uiPart < 4; uiPart++ )
    {
      xIntraRecQT( pcCU, uiTrDepth + 1, uiAbsPartIdx + uiPart * uiNumQPart, pcRecoYuv, pcPredYuv, pcResiYuv );
    }
  }
}

Void
TDecCu::xReconIntraQT( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth )
{
  UInt  uiInitTrDepth = ( pcCU->getPartitionSize(0) == SIZE_2Nx2N ? 0 : 1 );
  UInt  uiNumPart     = pcCU->getNumPartInter();
  UInt  uiNumQParts   = pcCU->getTotalNumPart() >> 2;
  
  if (pcCU->getIPCMFlag(0))
  {
    xReconPCM( pcCU, uiAbsPartIdx, uiDepth );
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

#if RWTH_SDC_DLT_B0036
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
#if HHI_DMM_WEDGE_INTRA || HHI_DMM_PRED_TEX
  if( uiLumaPredMode >= NUM_INTRA_MODE )
  {
    m_pcPrediction->predIntraLumaDMM( pcCU, uiAbsPartIdx, uiLumaPredMode, piPred, uiStride, uiWidth, uiHeight, bAboveAvail, bLeftAvail, false );
  }
  else
  {
#endif
    m_pcPrediction->predIntraLumaAng( pcCU->getPattern(), uiLumaPredMode, piPred, uiStride, uiWidth, uiHeight, pcCU, bAboveAvail, bLeftAvail );
#if HHI_DMM_WEDGE_INTRA || HHI_DMM_PRED_TEX
  }
#endif
  
  // number of segments depends on prediction mode
  UInt uiNumSegments = 1;  
  Bool* pbMask = NULL;
  UInt uiMaskStride = 0;
  
  if( uiLumaPredMode == DMM_WEDGE_FULL_IDX || uiLumaPredMode == DMM_WEDGE_PREDDIR_IDX )
  {
    Int uiTabIdx = (uiLumaPredMode == DMM_WEDGE_FULL_IDX)?pcCU->getWedgeFullTabIdx(uiAbsPartIdx):pcCU->getWedgePredDirTabIdx(uiAbsPartIdx);
    
    WedgeList* pacWedgeList = &g_aacWedgeLists[(g_aucConvertToBit[uiWidth])];
    TComWedgelet* pcWedgelet = &(pacWedgeList->at( uiTabIdx ));
    
    uiNumSegments = 2;
    pbMask = pcWedgelet->getPattern();
    uiMaskStride = pcWedgelet->getStride();
  }
  
  // get DC prediction for each segment
  Pel apDCPredValues[2];
  xAnalyzeSegmentsSDC(piPred, uiStride, uiWidth, apDCPredValues, uiNumSegments, pbMask, uiMaskStride);
  
  // reconstruct residual based on mask + DC residuals
  Pel apDCResiValues[2];
  Pel apDCRecoValues[2];
  for( UInt ui = 0; ui < uiNumSegments; ui++ )
  {
    Pel   pPredIdx    = GetDepthValue2Idx( apDCPredValues[ui] );
    Pel   pResiIdx    = pcCU->getSDCSegmentDCOffset(ui, uiAbsPartIdx);
    Pel   pRecoValue  = GetIdx2DepthValue( pPredIdx + pResiIdx );
    
    apDCRecoValues[ui]  = pRecoValue;
    apDCResiValues[ui]  = pRecoValue - apDCPredValues[ui];
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
      
      Pel pPredVal= apDCPredValues[ucSegment];
      Pel pResiDC = apDCResiValues[ucSegment];
      
      pReco    [ uiX ] = Clip( pPredVal + pResiDC );
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
      pRecCb[x] = (Pel)(128<<g_uiBitIncrement);
      pRecCr[x] = (Pel)(128<<g_uiBitIncrement);
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
  UInt    uiLumaTrMode, uiChromaTrMode;
  
  pcCU->convertTransIdx( uiAbsPartIdx, pcCU->getTransformIdx( uiAbsPartIdx ), uiLumaTrMode, uiChromaTrMode );
  
  // Y
  piCoeff = pcCU->getCoeffY();
  pResi = m_ppcYuvResi[uiDepth]->getLumaAddr();

#if H0736_AVC_STYLE_QP_RANGE
  m_pcTrQuant->setQPforQuant( pcCU->getQP( uiAbsPartIdx ), !pcCU->getSlice()->getDepth(), pcCU->getSlice()->getSliceType(), TEXT_LUMA, pcCU->getSlice()->getSPS()->getQpBDOffsetY(), 0 );
#else
  m_pcTrQuant->setQPforQuant( pcCU->getQP( uiAbsPartIdx ), !pcCU->getSlice()->getDepth(), pcCU->getSlice()->getSliceType(), TEXT_LUMA, 0 );
#endif

  m_pcTrQuant->invRecurTransformNxN ( pcCU, 0, TEXT_LUMA, pResi, 0, m_ppcYuvResi[uiDepth]->getStride(), uiWidth, uiHeight, uiLumaTrMode, 0, piCoeff );
  
  // Cb and Cr
#if H0736_AVC_STYLE_QP_RANGE
  m_pcTrQuant->setQPforQuant( pcCU->getQP( uiAbsPartIdx ), !pcCU->getSlice()->getDepth(), pcCU->getSlice()->getSliceType(), TEXT_CHROMA, pcCU->getSlice()->getSPS()->getQpBDOffsetC(), pcCU->getSlice()->getPPS()->getChromaQpOffset() );
#else
  m_pcTrQuant->setQPforQuant( pcCU->getQP( uiAbsPartIdx ), !pcCU->getSlice()->getDepth(), pcCU->getSlice()->getSliceType(), TEXT_CHROMA, pcCU->getSlice()->getPPS()->getChromaQpOffset() );
#endif

  uiWidth  >>= 1;
  uiHeight >>= 1;
  piCoeff = pcCU->getCoeffCb(); pResi = m_ppcYuvResi[uiDepth]->getCbAddr();
  m_pcTrQuant->invRecurTransformNxN ( pcCU, 0, TEXT_CHROMA_U, pResi, 0, m_ppcYuvResi[uiDepth]->getCStride(), uiWidth, uiHeight, uiChromaTrMode, 0, piCoeff );

#if H0736_AVC_STYLE_QP_RANGE
  m_pcTrQuant->setQPforQuant( pcCU->getQP( uiAbsPartIdx ), !pcCU->getSlice()->getDepth(), pcCU->getSlice()->getSliceType(), TEXT_CHROMA, pcCU->getSlice()->getSPS()->getQpBDOffsetC(), pcCU->getSlice()->getPPS()->getChromaQpOffset2nd() );
#else
  m_pcTrQuant->setQPforQuant( pcCU->getQP( uiAbsPartIdx ), !pcCU->getSlice()->getDepth(), pcCU->getSlice()->getSliceType(), TEXT_CHROMA, pcCU->getSlice()->getPPS()->getChromaQpOffset2nd() );
#endif

  piCoeff = pcCU->getCoeffCr(); pResi = m_ppcYuvResi[uiDepth]->getCrAddr();
  m_pcTrQuant->invRecurTransformNxN ( pcCU, 0, TEXT_CHROMA_V, pResi, 0, m_ppcYuvResi[uiDepth]->getCStride(), uiWidth, uiHeight, uiChromaTrMode, 0, piCoeff );
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
    uiPcmLeftShiftBit = g_uiBitDepth + g_uiBitIncrement - pcCU->getSlice()->getSPS()->getPCMBitDepthLuma();
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
    uiPcmLeftShiftBit = g_uiBitDepth + g_uiBitIncrement - pcCU->getSlice()->getSPS()->getPCMBitDepthChroma();
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
 * \param uiAbsPartIdx CU index
 * \param uiDepth CU Depth
 * \returns Void
 */
Void TDecCu::xReconPCM( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth )
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

#if LOSSLESS_CODING 
/** Function for filling the PCM buffer of a CU using its reconstructed sample array 
 * \param pcCU pointer to current CU
 * \param uiAbsPartIdx CU index
 * \param uiDepth CU Depth
 * \returns Void
 */
Void TDecCu::xFillPCMBuffer(TComDataCU* pCU, UInt absPartIdx, UInt depth)
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
#endif

#if RWTH_SDC_DLT_B0036
Void TDecCu::xAnalyzeSegmentsSDC( Pel* pOrig, UInt uiStride, UInt uiSize, Pel* rpSegMeans, UInt uiNumSegments, Bool* pMask, UInt uiMaskStride )
{
  Int iSumDepth[2];
  memset(iSumDepth, 0, sizeof(Int)*2);
  Int iSumPix[2];
  memset(iSumPix, 0, sizeof(Int)*2);
#if HS_REFERENCE_SUBSAMPLE_C0154
  Int subSamplePix;
  if ( uiSize == 64 || uiSize == 32 )
  {
    subSamplePix = 2;
  }
  else
  {
    subSamplePix = 1;
  }
  for (Int y=0; y<uiSize; y+=subSamplePix)
  {
    for (Int x=0; x<uiSize; x+=subSamplePix)
    {
      UChar ucSegment = pMask?(UChar)pMask[x]:0;
      assert( ucSegment < uiNumSegments );
  
      iSumDepth[ucSegment] += pOrig[x];
      iSumPix[ucSegment]   += 1;
    }
    pOrig  += uiStride*subSamplePix;
    pMask  += uiMaskStride*subSamplePix;
  }
#else
  for (Int y=0; y<uiSize; y++)
  {
    for (Int x=0; x<uiSize; x++)
    {
      UChar ucSegment = pMask?(UChar)pMask[x]:0;
      assert( ucSegment < uiNumSegments );
      
      iSumDepth[ucSegment] += pOrig[x];
      iSumPix[ucSegment]   += 1;
    }
    
    pOrig  += uiStride;
    pMask  += uiMaskStride;
  }
#endif
  // compute mean for each segment
  for( UChar ucSeg = 0; ucSeg < uiNumSegments; ucSeg++ )
  {
    if( iSumPix[ucSeg] > 0 )
      rpSegMeans[ucSeg] = iSumDepth[ucSeg] / iSumPix[ucSeg];
    else
      rpSegMeans[ucSeg] = 0;  // this happens for zero-segments
  }
}
#endif

//! \}
