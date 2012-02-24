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



/** \file     TDecCu.cpp
    \brief    CU decoder class
*/

#include "TDecCu.h"

// ====================================================================================================================
// Constructor / destructor / create / destroy
// ====================================================================================================================

TDecCu::TDecCu()
{
  m_ppcYuvResi    = NULL;
  m_ppcYuvReco    = NULL;
  m_ppcYuvResPred = NULL;
#if POZNAN_AVAIL_MAP
  m_ppcYuvAvail   = NULL;
#endif
#if POZNAN_SYNTH_VIEW
  m_ppcYuvSynth   = NULL;
#endif
  m_ppcCU         = NULL;
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
  
  m_ppcYuvResi    = new TComYuv*    [m_uiMaxDepth-1];
  m_ppcYuvReco    = new TComYuv*    [m_uiMaxDepth-1];
  m_ppcYuvResPred = new TComYuv*    [m_uiMaxDepth-1];
#if POZNAN_AVAIL_MAP
  m_ppcYuvAvail   = new TComYuv*    [m_uiMaxDepth-1];
#endif
#if POZNAN_SYNTH_VIEW
  m_ppcYuvSynth   = new TComYuv*    [m_uiMaxDepth-1];
#endif
  m_ppcCU         = new TComDataCU* [m_uiMaxDepth-1];
  
  UInt uiNumPartitions;
  for ( UInt ui = 0; ui < m_uiMaxDepth-1; ui++ )
  {
    uiNumPartitions = 1<<( ( m_uiMaxDepth - ui - 1 )<<1 );
    UInt uiWidth  = uiMaxWidth  >> ui;
    UInt uiHeight = uiMaxHeight >> ui;
    
    m_ppcYuvResi   [ui] = new TComYuv;    m_ppcYuvResi   [ui]->create( uiWidth, uiHeight );
    m_ppcYuvReco   [ui] = new TComYuv;    m_ppcYuvReco   [ui]->create( uiWidth, uiHeight );
    m_ppcYuvResPred[ui] = new TComYuv;    m_ppcYuvResPred[ui]->create( uiWidth, uiHeight );
#if POZNAN_AVAIL_MAP
    m_ppcYuvAvail  [ui] = new TComYuv;    m_ppcYuvAvail  [ui]->create( uiWidth, uiHeight );
#endif
#if POZNAN_SYNTH_VIEW
    m_ppcYuvSynth  [ui] = new TComYuv;    m_ppcYuvSynth  [ui]->create( uiWidth, uiHeight );
#endif

    m_ppcCU        [ui] = new TComDataCU; m_ppcCU        [ui]->create( uiNumPartitions, uiWidth, uiHeight, true );
  }
  
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
    m_ppcYuvResi   [ui]->destroy(); delete m_ppcYuvResi   [ui]; m_ppcYuvResi   [ui] = NULL;
    m_ppcYuvReco   [ui]->destroy(); delete m_ppcYuvReco   [ui]; m_ppcYuvReco   [ui] = NULL;
    m_ppcYuvResPred[ui]->destroy(); delete m_ppcYuvResPred[ui]; m_ppcYuvResPred[ui] = NULL;
#if POZNAN_AVAIL_MAP
    m_ppcYuvAvail  [ui]->destroy(); delete m_ppcYuvAvail  [ui]; m_ppcYuvAvail  [ui] = NULL;
#endif
#if POZNAN_SYNTH_VIEW
    m_ppcYuvSynth  [ui]->destroy(); delete m_ppcYuvSynth  [ui]; m_ppcYuvSynth  [ui] = NULL;
#endif
    m_ppcCU        [ui]->destroy(); delete m_ppcCU        [ui]; m_ppcCU        [ui] = NULL;
  }
  
  delete [] m_ppcYuvResi;    m_ppcYuvResi    = NULL;
  delete [] m_ppcYuvReco;    m_ppcYuvReco    = NULL;
  delete [] m_ppcYuvResPred; m_ppcYuvResPred = NULL;
#if POZNAN_AVAIL_MAP
  delete [] m_ppcYuvAvail;   m_ppcYuvAvail   = NULL;
#endif
#if POZNAN_SYNTH_VIEW
  delete [] m_ppcYuvSynth;   m_ppcYuvSynth   = NULL;
#endif

  delete [] m_ppcCU;         m_ppcCU         = NULL;
}

// ====================================================================================================================
// Public member functions
// ====================================================================================================================

/** \param    pcCU        pointer of CU data
 \param    ruiIsLast   last data?
 */
Void TDecCu::decodeCU( TComDataCU* pcCU, UInt& ruiIsLast )
{
#if SNY_DQP   
  if ( pcCU->getSlice()->getSPS()->getUseDQP() )
  {
    pcCU->setdQPFlag(true); 
  }
#endif//SNY_DQP
  // start from the top level CU
  xDecodeCU( pcCU, 0, 0 );
  
#if SNY_DQP 
  // dQP: only for LCU
  if ( pcCU->getSlice()->getSPS()->getUseDQP() )
  {
    if ( pcCU->isSkipped( 0 ) && pcCU->getDepth( 0 ) == 0 )
    {
    }
    else if ( pcCU->getdQPFlag())// non-skip
    {
      m_pcEntropyDecoder->decodeQP( pcCU, 0, 0 );
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
      m_pcEntropyDecoder->decodeQP( pcCU, 0, 0 );
    }
  }
#endif//SNY_DQP
  
  //--- Read terminating bit ---
  m_pcEntropyDecoder->decodeTerminatingBit( ruiIsLast );
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

/** decode CU block recursively
 * \param pcCU
 * \param uiAbsPartIdx 
 * \param uiDepth 
 * \returns Void
 */
Void TDecCu::xDecodeCU( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth )
{
  TComPic* pcPic = pcCU->getPic();

#if POZNAN_ENCODE_ONLY_DISOCCLUDED_CU
  Bool bWholeCUCanBeSynthesized = false;
  Bool bOneSubCUCanNotBeSynthesied = false;
  Bool bSubCUCanBeSynthesized[4];
  Bool * pbSubCUCanBeSynthesized = bSubCUCanBeSynthesized;
  pcPic->checkSynthesisAvailability(pcCU, pcCU->getAddr(), uiAbsPartIdx, uiDepth, pbSubCUCanBeSynthesized); //KUBA SYNTH
  Int  iSubCUCanNotBeSynthesized = 0;
  Int  iSubCUCanBeSynthesizedCnt = 0;
  for(Int i = 0; i < 4; i++)
  {
    if (!bSubCUCanBeSynthesized[i])
    {
      iSubCUCanNotBeSynthesized = i;
    }
    else
    {
      iSubCUCanBeSynthesizedCnt ++;
    }
  }
  if(iSubCUCanBeSynthesizedCnt == 4)
  {
    bWholeCUCanBeSynthesized = true;
  }
  else if(iSubCUCanBeSynthesizedCnt == 3)
  {
    bOneSubCUCanNotBeSynthesied = true;
  }

  if(bWholeCUCanBeSynthesized)
  {
    pcCU->setPredModeSubParts( MODE_SYNTH, uiAbsPartIdx, uiDepth );
    pcCU->setDepthSubParts( uiDepth, uiAbsPartIdx );
    pcCU->setPartSizeSubParts( SIZE_2Nx2N, uiAbsPartIdx, uiDepth );
    pcCU->setSizeSubParts( g_uiMaxCUWidth>>uiDepth, g_uiMaxCUHeight>>uiDepth, uiAbsPartIdx, uiDepth );
    //pcCU->setSizeSubParts( pcCU->getSlice()->getSPS()->getMaxCUWidth()>>uiDepth, pcCU->getSlice()->getSPS()->getMaxCUHeight()>>uiDepth, uiAbsPartIdx, uiDepth );
    return;    
  }
#endif

  UInt uiCurNumParts    = pcPic->getNumPartInCU() >> (uiDepth<<1);
  UInt uiQNumParts      = uiCurNumParts>>2;
  
  Bool bBoundary = false;
  UInt uiLPelX   = pcCU->getCUPelX() + g_auiRasterToPelX[ g_auiZscanToRaster[uiAbsPartIdx] ];
  UInt uiRPelX   = uiLPelX + (g_uiMaxCUWidth>>uiDepth)  - 1;
  UInt uiTPelY   = pcCU->getCUPelY() + g_auiRasterToPelY[ g_auiZscanToRaster[uiAbsPartIdx] ];
  UInt uiBPelY   = uiTPelY + (g_uiMaxCUHeight>>uiDepth) - 1;
  
  if( ( uiRPelX < pcCU->getSlice()->getSPS()->getWidth() ) && ( uiBPelY < pcCU->getSlice()->getSPS()->getHeight() ) )
  {
#if POZNAN_ENCODE_ONLY_DISOCCLUDED_CU
    if(bOneSubCUCanNotBeSynthesied && (uiDepth < g_uiMaxCUDepth - g_uiAddCUDepth )) // Check if CU has 3 synthesied subCU - no split flag is send in that case and CU split is assumed
    {
      pcCU->setDepthSubParts( uiDepth + 1, uiAbsPartIdx );
    }
    else 
#endif
#if HHI_MPI
    if( pcCU->getTextureModeDepth( uiAbsPartIdx ) == -1 || uiDepth < pcCU->getTextureModeDepth( uiAbsPartIdx ) )
#endif
      m_pcEntropyDecoder->decodeSplitFlag( pcCU, uiAbsPartIdx, uiDepth );
  }
  else
  {
    bBoundary = true;
  }
  
  if( ( ( uiDepth < pcCU->getDepth( uiAbsPartIdx ) ) && ( uiDepth < g_uiMaxCUDepth - g_uiAddCUDepth ) ) || bBoundary )
  {
    UInt uiIdx = uiAbsPartIdx;
    for ( UInt uiPartUnitIdx = 0; uiPartUnitIdx < 4; uiPartUnitIdx++ )
    {
      uiLPelX   = pcCU->getCUPelX() + g_auiRasterToPelX[ g_auiZscanToRaster[uiIdx] ];
      uiTPelY   = pcCU->getCUPelY() + g_auiRasterToPelY[ g_auiZscanToRaster[uiIdx] ];
      
      if( ( uiLPelX < pcCU->getSlice()->getSPS()->getWidth() ) && ( uiTPelY < pcCU->getSlice()->getSPS()->getHeight() ) )
        xDecodeCU( pcCU, uiIdx, uiDepth+1 );
      
      uiIdx += uiQNumParts;
    }
    
    return;
  }
  
#if TSB_ALF_HEADER
#else
  m_pcEntropyDecoder->decodeAlfCtrlFlag( pcCU, uiAbsPartIdx, uiDepth );
#endif
  
  // decode CU mode and the partition size
#if HHI_MPI
  if( !pcCU->getSlice()->isIntra() && pcCU->getTextureModeDepth( uiAbsPartIdx ) == -1 )
#else
  if( !pcCU->getSlice()->isIntra() )
#endif
  {
    m_pcEntropyDecoder->decodeSkipFlag( pcCU, uiAbsPartIdx, uiDepth );
  }
  

  if( pcCU->isSkipped(uiAbsPartIdx) )
  {
#if HHI_MRG_SKIP
    m_ppcCU[uiDepth]->copyInterPredInfoFrom( pcCU, uiAbsPartIdx, REF_PIC_LIST_0 );
    m_ppcCU[uiDepth]->copyInterPredInfoFrom( pcCU, uiAbsPartIdx, REF_PIC_LIST_1 );
    TComMvField cMvFieldNeighbours[MRG_MAX_NUM_CANDS << 1]; // double length for mv of both lists
    UChar uhInterDirNeighbours[MRG_MAX_NUM_CANDS];
    UInt uiNeighbourCandIdx[MRG_MAX_NUM_CANDS]; //MVs with same idx => same cand
    for( UInt ui = 0; ui < MRG_MAX_NUM_CANDS; ++ui )
    {
      uhInterDirNeighbours[ui] = 0;
      uiNeighbourCandIdx[ui] = 0;
    }
    m_ppcCU[uiDepth]->getInterMergeCandidates( 0, 0, uiDepth, cMvFieldNeighbours, uhInterDirNeighbours, uiNeighbourCandIdx );
    for(UInt uiIter = 0; uiIter < MRG_MAX_NUM_CANDS; uiIter++ )
  {
      pcCU->setNeighbourCandIdxSubParts( uiIter, uiNeighbourCandIdx[uiIter], uiAbsPartIdx, 0, uiDepth );
    }
    m_pcEntropyDecoder->decodeMergeIndex( pcCU, 0, uiAbsPartIdx, SIZE_2Nx2N, uhInterDirNeighbours, cMvFieldNeighbours, uiDepth );
#else
    pcCU->setMVPIdxSubParts( -1, REF_PIC_LIST_0, uiAbsPartIdx, 0, uiDepth);
    pcCU->setMVPNumSubParts( -1, REF_PIC_LIST_0, uiAbsPartIdx, 0, uiDepth);
    
    pcCU->setMVPIdxSubParts( -1, REF_PIC_LIST_1, uiAbsPartIdx, 0, uiDepth);
    pcCU->setMVPNumSubParts( -1, REF_PIC_LIST_1, uiAbsPartIdx, 0, uiDepth);
    
    if ( pcCU->getSlice()->getNumRefIdx( REF_PIC_LIST_0 ) > 0 ) //if ( ref. frame list0 has at least 1 entry )
    {
      m_pcEntropyDecoder->decodeMVPIdx( pcCU, uiAbsPartIdx, uiDepth, REF_PIC_LIST_0, m_ppcCU[uiDepth]);
    }
    
    if ( pcCU->getSlice()->getNumRefIdx( REF_PIC_LIST_1 ) > 0 ) //if ( ref. frame list1 has at least 1 entry )
    {
      m_pcEntropyDecoder->decodeMVPIdx( pcCU, uiAbsPartIdx, uiDepth, REF_PIC_LIST_1, m_ppcCU[uiDepth]);
    }
#endif
#if HHI_MPI
    if( pcCU->getTextureModeDepth( uiAbsPartIdx ) == uiDepth )
    {
      TComDataCU *pcTextureCU = pcCU->getSlice()->getTexturePic()->getCU( pcCU->getAddr() );
      pcCU->copyTextureMotionDataFrom( pcTextureCU, uiDepth, pcCU->getZorderIdxInCU() + uiAbsPartIdx, uiAbsPartIdx );

      UInt uiCurrPartNumb = pcCU->getPic()->getNumPartInCU() >> (uiDepth << 1);
      for( UInt ui = 0; ui < uiCurrPartNumb; ui++ )
      {
        const UChar uhNewDepth = max( uiDepth, pcTextureCU->getDepth( uiAbsPartIdx + ui ) );
        pcCU->setPredictionMode( uiAbsPartIdx + ui, MODE_SKIP );
        pcCU->setPartitionSize( uiAbsPartIdx + ui, SIZE_2Nx2N );
        pcCU->setDepth( uiAbsPartIdx + ui, uhNewDepth );
        pcCU->setWidth( uiAbsPartIdx + ui, g_uiMaxCUWidth>>uhNewDepth );
        pcCU->setHeight( uiAbsPartIdx + ui, g_uiMaxCUHeight>>uhNewDepth );
      }
    }
#endif
 
#if HHI_INTER_VIEW_RESIDUAL_PRED
    m_pcEntropyDecoder->decodeResPredFlag( pcCU, uiAbsPartIdx, uiDepth, m_ppcCU[uiDepth], 0 );
#endif
    return;
  }

#if HHI_MPI
  if( pcCU->getTextureModeDepth( uiAbsPartIdx ) == -1 )
  {
#endif
    m_pcEntropyDecoder->decodePredMode( pcCU, uiAbsPartIdx, uiDepth );

    m_pcEntropyDecoder->decodePartSize( pcCU, uiAbsPartIdx, uiDepth );

    // prediction mode ( Intra : direction mode, Inter : Mv, reference idx )
    m_pcEntropyDecoder->decodePredInfo( pcCU, uiAbsPartIdx, uiDepth, m_ppcCU[uiDepth]);

#if HHI_MPI
    if( !pcCU->isIntra( uiAbsPartIdx ) )
    {
      m_ppcCU[uiDepth]  ->copyInterPredInfoFrom( pcCU, uiAbsPartIdx, REF_PIC_LIST_0 );
      m_ppcCU[uiDepth]  ->copyInterPredInfoFrom( pcCU, uiAbsPartIdx, REF_PIC_LIST_1 );
#if HHI_INTER_VIEW_RESIDUAL_PRED
      m_pcEntropyDecoder->decodeResPredFlag    ( pcCU, uiAbsPartIdx, uiDepth, m_ppcCU[uiDepth], 0 );
#endif
    }

    if( pcCU->getTextureModeDepth( uiAbsPartIdx ) == uiDepth )
    {
      assert( pcCU->getZorderIdxInCU() == 0 );
      TComDataCU *pcTextureCU = pcCU->getSlice()->getTexturePic()->getCU( pcCU->getAddr() );
      pcCU->copyTextureMotionDataFrom( pcTextureCU, uiDepth, pcCU->getZorderIdxInCU() + uiAbsPartIdx, uiAbsPartIdx );

      UInt uiCurrPartNumb = pcCU->getPic()->getNumPartInCU() >> (uiDepth << 1);
      for( UInt ui = 0; ui < uiCurrPartNumb; ui++ )
      {
        const UChar uhNewDepth = max( uiDepth, pcTextureCU->getDepth( uiAbsPartIdx + ui ) );
        pcCU->setPredictionMode( uiAbsPartIdx + ui, MODE_INTER );
        pcCU->setPartitionSize( uiAbsPartIdx + ui, SIZE_2Nx2N );
        pcCU->setDepth( uiAbsPartIdx + ui, uhNewDepth );
        pcCU->setWidth( uiAbsPartIdx + ui, g_uiMaxCUWidth>>uhNewDepth );
        pcCU->setHeight( uiAbsPartIdx + ui, g_uiMaxCUHeight>>uhNewDepth );
      }

      if( ( ( uiDepth < pcCU->getDepth( uiAbsPartIdx ) ) && ( uiDepth < g_uiMaxCUDepth - g_uiAddCUDepth ) ) || bBoundary )
      {
        UInt uiIdx = uiAbsPartIdx;
        for ( UInt uiPartUnitIdx = 0; uiPartUnitIdx < 4; uiPartUnitIdx++ )
        {
          uiLPelX   = pcCU->getCUPelX() + g_auiRasterToPelX[ g_auiZscanToRaster[uiIdx] ];
          uiTPelY   = pcCU->getCUPelY() + g_auiRasterToPelY[ g_auiZscanToRaster[uiIdx] ];

          if( ( uiLPelX < pcCU->getSlice()->getSPS()->getWidth() ) && ( uiTPelY < pcCU->getSlice()->getSPS()->getHeight() ) )
            xDecodeCU( pcCU, uiIdx, uiDepth+1 );

          uiIdx += uiQNumParts;
        }
        return;
      }
    }
  }
#endif

  UInt uiCurrWidth      = pcCU->getWidth ( uiAbsPartIdx );
  UInt uiCurrHeight     = pcCU->getHeight( uiAbsPartIdx );
  
  // Coefficient decoding
  m_pcEntropyDecoder->decodeCoeff( pcCU, uiAbsPartIdx, uiDepth, uiCurrWidth, uiCurrHeight );
  
}

Void TDecCu::xDecompressCU( TComDataCU* pcCU, TComDataCU* pcCUCur, UInt uiAbsPartIdx,  UInt uiDepth )
{
  TComPic* pcPic = pcCU->getPic();
#if POZNAN_SYNTH_VIEW
  if(pcPic->getPicYuvSynth())  m_ppcYuvSynth[uiDepth]->copyFromPicYuv( pcPic->getPicYuvSynth(), pcCU->getAddr(), uiAbsPartIdx );
#endif
#if POZNAN_AVAIL_MAP
  if(pcPic->getPicYuvAvail())  m_ppcYuvAvail[uiDepth]->copyFromPicYuv( pcPic->getPicYuvAvail(), pcCU->getAddr(), uiAbsPartIdx );
#endif
  
  Bool bBoundary = false;
  UInt uiLPelX   = pcCU->getCUPelX() + g_auiRasterToPelX[ g_auiZscanToRaster[uiAbsPartIdx] ];
  UInt uiRPelX   = uiLPelX + (g_uiMaxCUWidth>>uiDepth)  - 1;
  UInt uiTPelY   = pcCU->getCUPelY() + g_auiRasterToPelY[ g_auiZscanToRaster[uiAbsPartIdx] ];
  UInt uiBPelY   = uiTPelY + (g_uiMaxCUHeight>>uiDepth) - 1;
  
  if( ( uiRPelX >= pcCU->getSlice()->getSPS()->getWidth() ) || ( uiBPelY >= pcCU->getSlice()->getSPS()->getHeight() ) )
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
      
      if( ( uiLPelX < pcCU->getSlice()->getSPS()->getWidth() ) && ( uiTPelY < pcCU->getSlice()->getSPS()->getHeight() ) )
        xDecompressCU(pcCU, m_ppcCU[uiNextDepth], uiIdx, uiNextDepth );
      
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
      xReconIntraQT( m_ppcCU[uiDepth], uiAbsPartIdx, uiDepth );
      break;
#if POZNAN_ENCODE_ONLY_DISOCCLUDED_CU
    case MODE_SYNTH:
    //  break;
#if POZNAN_FILL_OCCLUDED_CU_WITH_SYNTHESIS
      m_ppcYuvReco[uiDepth]->copyFromPicYuv(pcPic->getPicYuvSynth(), pcCU->getAddr(), uiAbsPartIdx);
#else
      m_ppcYuvReco[uiDepth]->copyFromPicYuv(pcPic->getPicYuvAvail(), pcCU->getAddr(), uiAbsPartIdx); //Poprawi�
#endif
      //m_ppcYuvReco[uiDepth]->clear();
      break;
#endif
    default:
      assert(0);
      break;
  }
  
  xCopyToPic( m_ppcCU[uiDepth], pcPic, uiAbsPartIdx, uiDepth );
}

Void TDecCu::xReconInter( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth )
{
#if HHI_MPI
  if( pcCU->getTextureModeDepth( 0 ) != -1 )
    pcCU->setPartSizeSubParts( SIZE_NxN, 0, uiDepth );
#endif
  
  // inter prediction
  m_pcPrediction->motionCompensation( pcCU, m_ppcYuvReco[uiDepth] );
  
#if HHI_MPI
  if( pcCU->getTextureModeDepth( 0 ) != -1 )
    pcCU->setPartSizeSubParts( SIZE_2Nx2N, 0, uiDepth );
#endif
  
#if HHI_INTER_VIEW_RESIDUAL_PRED
  if( pcCU->getResPredFlag( 0 ) )
  {
    AOF( pcCU->getResPredAvail( 0 ) );
    Bool bOK = pcCU->getResidualSamples( 0, m_ppcYuvResPred[uiDepth] );
    AOF( bOK );
    m_ppcYuvReco[uiDepth]->add( m_ppcYuvResPred[uiDepth], pcCU->getWidth( 0 ), pcCU->getHeight( 0 ) );
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
    if( pcCU->getResPredFlag( 0 ) )
    {
      m_ppcYuvReco[uiDepth]->clip( pcCU->getWidth( 0 ), pcCU->getHeight( 0 ) );
    }
#endif
    m_ppcYuvReco[uiDepth]->copyPartToPartYuv( m_ppcYuvReco[uiDepth],0, pcCU->getWidth( 0 ),pcCU->getHeight( 0 ));
  }
}

Void TDecCu::xDecodeIntraTexture( TComDataCU* pcCU, UInt uiPartIdx, Pel* piReco, Pel* piPred, Pel* piResi, UInt uiStride, TCoeff* pCoeff, UInt uiWidth, UInt uiHeight, UInt uiCurrDepth )
{
  if( pcCU->getTransformIdx(0) == uiCurrDepth )
  {
    UInt uiX, uiY;
    TComPattern* pcPattern = pcCU->getPattern();
    UInt uiZorder          = pcCU->getZorderIdxInCU()+uiPartIdx;
    Pel* pPred             = piPred;
    Pel* pResi             = piResi;
    Pel* piPicReco         = pcCU->getPic()->getPicYuvRec()->getLumaAddr(pcCU->getAddr(), uiZorder);
    UInt uiPicStride       = pcCU->getPic()->getPicYuvRec()->getStride();
    
    pcPattern->initPattern( pcCU, uiCurrDepth, uiPartIdx );
    
    Bool bAboveAvail = false;
    Bool bLeftAvail  = false;
    
    pcPattern->initAdiPattern(pcCU, uiPartIdx, uiCurrDepth, m_pcPrediction->getPredicBuf(), m_pcPrediction->getPredicBufWidth(), m_pcPrediction->getPredicBufHeight(), bAboveAvail, bLeftAvail);
    
    m_pcPrediction->predIntraLumaAng( pcPattern, pcCU->getLumaIntraDir(uiPartIdx), pPred, uiStride, uiWidth, uiHeight, pcCU, bAboveAvail, bLeftAvail );
    
#if POZNAN_TEXTURE_TU_DELTA_QP_ACCORDING_TO_DEPTH
    m_pcTrQuant->setQPforQuant( pcCU->getQP(uiPartIdx) + pcCU->getQpOffsetForTextCU(uiPartIdx, true), !pcCU->getSlice()->getDepth(), pcCU->getSlice()->getSliceType(), TEXT_LUMA );
#else
    m_pcTrQuant->setQPforQuant( pcCU->getQP(uiPartIdx), !pcCU->getSlice()->getDepth(), pcCU->getSlice()->getSliceType(), TEXT_LUMA );
#endif
#if INTRA_DST_TYPE_7
  m_pcTrQuant->invtransformNxN(TEXT_LUMA, pcCU->getLumaIntraDir(uiPartIdx), pResi, uiStride, pCoeff, uiWidth, uiHeight );
#else
  m_pcTrQuant->invtransformNxN( pResi, uiStride, pCoeff, uiWidth, uiHeight );
#endif
    // Reconstruction
    {
      pResi = piResi;
      pPred = piPred;
      for( uiY = 0; uiY < uiHeight; uiY++ )
      {
        for( uiX = 0; uiX < uiWidth; uiX++ )
        {
          piReco   [uiX] = Clip(pPred[uiX] + pResi[uiX]);
          piPicReco[uiX] = piReco[uiX];
        }
        piReco    += uiStride;
        pPred     += uiStride;
        pResi     += uiStride;
        piPicReco += uiPicStride;
      }
    }
  }
  else
  {
    uiCurrDepth++;
    uiWidth  >>= 1;
    uiHeight >>= 1;
    UInt uiPartOffset  = pcCU->getTotalNumPart()>>(uiCurrDepth<<1);
    UInt uiCoeffOffset = uiWidth  * uiHeight;
    UInt uiPelOffset   = uiHeight * uiStride;
    Pel* pResi = piResi;
    Pel* pReco = piReco;
    Pel* pPred = piPred;
    xDecodeIntraTexture( pcCU, uiPartIdx, pReco, pPred, pResi, uiStride, pCoeff, uiWidth, uiHeight, uiCurrDepth );
    uiPartIdx += uiPartOffset;
    pCoeff    += uiCoeffOffset;
    pResi      = piResi + uiWidth;
    pReco      = piReco + uiWidth;
    pPred      = piPred + uiWidth;
    xDecodeIntraTexture( pcCU, uiPartIdx, pReco, pPred, pResi, uiStride, pCoeff, uiWidth, uiHeight, uiCurrDepth );
    uiPartIdx += uiPartOffset;
    pCoeff    += uiCoeffOffset;
    pResi      = piResi + uiPelOffset;
    pReco      = piReco + uiPelOffset;
    pPred      = piPred + uiPelOffset;
    xDecodeIntraTexture( pcCU, uiPartIdx, pReco, pPred, pResi, uiStride, pCoeff, uiWidth, uiHeight, uiCurrDepth );
    uiPartIdx += uiPartOffset;
    pCoeff    += uiCoeffOffset;
    pResi      = piResi + uiPelOffset + uiWidth;
    pReco      = piReco + uiPelOffset + uiWidth;
    pPred      = piPred + uiPelOffset + uiWidth;
    xDecodeIntraTexture( pcCU, uiPartIdx, pReco, pPred, pResi, uiStride, pCoeff, uiWidth, uiHeight, uiCurrDepth );
  }
}

// ADI chroma
Void TDecCu::xRecurIntraInvTransChroma(TComDataCU* pcCU, UInt uiAbsPartIdx, Pel* piResi, Pel* piPred, Pel* piReco, UInt uiStride, TCoeff* piCoeff, UInt uiWidth, UInt uiHeight, UInt uiTrMode, UInt uiCurrTrMode, TextType eText )
{
  if( uiTrMode == uiCurrTrMode )
  {
    UInt uiX, uiY;
    UInt uiZorder    = pcCU->getZorderIdxInCU()+uiAbsPartIdx;
    Pel* pResi       = piResi;
    Pel* pPred       = piPred;
    Pel* piPicReco;
    if( eText == TEXT_CHROMA_U )
      piPicReco= pcCU->getPic()->getPicYuvRec()->getCbAddr(pcCU->getAddr(), uiZorder);
    else
      piPicReco= pcCU->getPic()->getPicYuvRec()->getCrAddr(pcCU->getAddr(), uiZorder);
    
    UInt uiPicStride = pcCU->getPic()->getPicYuvRec()->getCStride();
    
    pcCU->getPattern()->initPattern( pcCU, uiCurrTrMode, uiAbsPartIdx );
    
    Bool bAboveAvail = false;
    Bool bLeftAvail  = false;
    
    pcCU->getPattern()->initAdiPatternChroma(pcCU,uiAbsPartIdx, uiCurrTrMode, m_pcPrediction->getPredicBuf(),m_pcPrediction->getPredicBufWidth(),m_pcPrediction->getPredicBufHeight(),bAboveAvail,bLeftAvail);
    
    UInt uiModeL        = pcCU->getLumaIntraDir(0);
    UInt uiMode         = pcCU->getChromaIntraDir(0);
    
    if (uiMode==4) uiMode = uiModeL;
    
    Int*   pPatChr;
    
    if (eText==TEXT_CHROMA_U)
    {
      pPatChr=  pcCU->getPattern()->getAdiCbBuf( uiWidth, uiHeight, m_pcPrediction->getPredicBuf() );
    }
    else // (eText==TEXT_CHROMA_V)
    {
      pPatChr=  pcCU->getPattern()->getAdiCrBuf( uiWidth, uiHeight, m_pcPrediction->getPredicBuf() );
    }
    
    m_pcPrediction-> predIntraChromaAng( pcCU->getPattern(), pPatChr, uiMode, pPred, uiStride, uiWidth, uiHeight, pcCU, bAboveAvail, bLeftAvail );
    
    // Inverse Transform
    if( pcCU->getCbf(0, eText, uiCurrTrMode) )
    {
#if INTRA_DST_TYPE_7
    m_pcTrQuant->invtransformNxN( eText, REG_DCT, pResi, uiStride, piCoeff, uiWidth, uiHeight);
#else
    m_pcTrQuant->invtransformNxN( pResi, uiStride, piCoeff, uiWidth, uiHeight );
#endif
    }
    pResi = piResi;
    
    for( uiY = 0; uiY < uiHeight; uiY++ )
    {
      for( uiX = 0; uiX < uiWidth; uiX++ )
      {
        piReco   [uiX] = Clip( pPred[uiX] + pResi[uiX] );
        piPicReco[uiX] = piReco[uiX];
      }
      
      pPred     += uiStride;
      pResi     += uiStride;
      piReco    += uiStride;
      piPicReco += uiPicStride;
    }
  }
  else
  {
    uiCurrTrMode++;
    uiWidth  >>= 1;
    uiHeight >>= 1;
    UInt uiCoeffOffset = uiWidth*uiHeight;
    UInt uiPelOffset   = uiHeight*uiStride;
    UInt uiPartOffst   = pcCU->getTotalNumPart()>>(uiCurrTrMode<<1);
    Pel* pResi = piResi;
    Pel* pPred = piPred;
    Pel* pReco = piReco;
    xRecurIntraInvTransChroma( pcCU, uiAbsPartIdx, pResi, pPred, pReco, uiStride, piCoeff, uiWidth, uiHeight, uiTrMode, uiCurrTrMode, eText );
    uiAbsPartIdx += uiPartOffst;
    piCoeff      += uiCoeffOffset;
    pResi         = piResi + uiWidth; pPred = piPred + uiWidth; pReco = piReco + uiWidth;
    xRecurIntraInvTransChroma( pcCU, uiAbsPartIdx, pResi, pPred, pReco, uiStride, piCoeff, uiWidth, uiHeight, uiTrMode, uiCurrTrMode, eText );
    uiAbsPartIdx += uiPartOffst;
    piCoeff      += uiCoeffOffset;
    pResi         = piResi + uiPelOffset; pPred = piPred + uiPelOffset; pReco = piReco + uiPelOffset;
    xRecurIntraInvTransChroma( pcCU, uiAbsPartIdx, pResi, pPred, pReco, uiStride, piCoeff, uiWidth, uiHeight, uiTrMode, uiCurrTrMode, eText );
    uiAbsPartIdx += uiPartOffst;
    piCoeff      += uiCoeffOffset;
    pResi         = piResi + uiPelOffset + uiWidth; pPred = piPred + uiPelOffset + uiWidth; pReco = piReco + uiPelOffset + uiWidth;
    xRecurIntraInvTransChroma( pcCU, uiAbsPartIdx, pResi, pPred, pReco, uiStride, piCoeff, uiWidth, uiHeight, uiTrMode, uiCurrTrMode, eText );
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
  
#if HHI_DMM_WEDGE_INTRA || HHI_DMM_PRED_TEX
  if( uiLumaPredMode > MAX_MODE_ID_INTRA_DIR )
  {
    m_pcPrediction->predIntraLumaDMM( pcCU, uiAbsPartIdx, uiLumaPredMode, piPred, uiStride, uiWidth, uiHeight, bAboveAvail, bLeftAvail, false );
  } 
  else
#endif
  {    
  //===== get prediction signal =====
  m_pcPrediction->predIntraLumaAng( pcCU->getPattern(), uiLumaPredMode, piPred, uiStride, uiWidth, uiHeight, pcCU, bAboveAvail, bLeftAvail );
  }
  //===== inverse transform =====
#if POZNAN_TEXTURE_TU_DELTA_QP_ACCORDING_TO_DEPTH
    m_pcTrQuant->setQPforQuant( pcCU->getQP(0) + pcCU->getQpOffsetForTextCU(uiAbsPartIdx, true), !pcCU->getSlice()->getDepth(), pcCU->getSlice()->getSliceType(), TEXT_LUMA );
#else
  m_pcTrQuant->setQPforQuant  ( pcCU->getQP(0), !pcCU->getSlice()->getDepth(), pcCU->getSlice()->getSliceType(), TEXT_LUMA );
#endif
#if INTRA_DST_TYPE_7
  m_pcTrQuant->invtransformNxN( TEXT_LUMA, pcCU->getLumaIntraDir( uiAbsPartIdx ), piResi, uiStride, pcCoeff, uiWidth, uiHeight );
#else
  m_pcTrQuant->invtransformNxN( piResi, uiStride, pcCoeff, uiWidth, uiHeight );
#endif  

  
  //===== reconstruction =====
  {
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
  if( uiLog2TrSize == pcCU->getSlice()->getSPS()->getQuadtreeTULog2MinSize() )
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
  
#if !LM_CHROMA
  if( uiChromaPredMode == 4 )
  {
    uiChromaPredMode          = pcCU->getLumaIntraDir( 0 );
  }
#endif
  
  UInt      uiZOrder          = pcCU->getZorderIdxInCU() + uiAbsPartIdx;
  Pel*      piRecIPred        = ( uiChromaId > 0 ? pcCU->getPic()->getPicYuvRec()->getCrAddr( pcCU->getAddr(), uiZOrder ) : pcCU->getPic()->getPicYuvRec()->getCbAddr( pcCU->getAddr(), uiZOrder ) );
  UInt      uiRecIPredStride  = pcCU->getPic()->getPicYuvRec()->getCStride();
  
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
#if LM_CHROMA
  if(pcCU->getSlice()->getSPS()->getUseLMChroma() && uiChromaPredMode == 3)
  {
      m_pcPrediction->predLMIntraChroma( pcCU->getPattern(), pPatChroma, piPred, uiStride, uiWidth, uiHeight, uiChromaId );
  }
  else
  {
    if( uiChromaPredMode == 4 )
    {
    uiChromaPredMode          = pcCU->getLumaIntraDir( 0 );
    }
  m_pcPrediction->predIntraChromaAng( pcCU->getPattern(), pPatChroma, uiChromaPredMode, piPred, uiStride, uiWidth, uiHeight, pcCU, bAboveAvail, bLeftAvail );  
  }
#else // LM_CHROMA
  m_pcPrediction->predIntraChromaAng( pcCU->getPattern(), pPatChroma, uiChromaPredMode, piPred, uiStride, uiWidth, uiHeight, pcCU, bAboveAvail, bLeftAvail );
#endif

  //===== inverse transform =====
#if POZNAN_TEXTURE_TU_DELTA_QP_ACCORDING_TO_DEPTH
  m_pcTrQuant->setQPforQuant  ( pcCU->getQP(0) + pcCU->getQpOffsetForTextCU(uiAbsPartIdx, true), !pcCU->getSlice()->getDepth(), pcCU->getSlice()->getSliceType(), eText );
#else
  m_pcTrQuant->setQPforQuant  ( pcCU->getQP(0), !pcCU->getSlice()->getDepth(), pcCU->getSlice()->getSliceType(), eText );
#endif
#if INTRA_DST_TYPE_7 
  m_pcTrQuant->invtransformNxN( eText, REG_DCT, piResi, uiStride, pcCoeff, uiWidth, uiHeight );
#else
  m_pcTrQuant->invtransformNxN( piResi, uiStride, pcCoeff, uiWidth, uiHeight );
#endif

  //===== reconstruction =====
  {
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
  
#if LM_CHROMA
  for( UInt uiPU = 0; uiPU < uiNumPart; uiPU++ )
  {
    xIntraLumaRecQT( pcCU, uiInitTrDepth, uiPU * uiNumQParts, m_ppcYuvReco[uiDepth], m_ppcYuvReco[uiDepth], m_ppcYuvResi[uiDepth] );
  }  

  for( UInt uiPU = 0; uiPU < uiNumPart; uiPU++ )
  {
    xIntraChromaRecQT( pcCU, uiInitTrDepth, uiPU * uiNumQParts, m_ppcYuvReco[uiDepth], m_ppcYuvReco[uiDepth], m_ppcYuvResi[uiDepth] );
  }
#else

  for( UInt uiPU = 0; uiPU < uiNumPart; uiPU++ )
  {
    xIntraRecQT( pcCU, uiInitTrDepth, uiPU * uiNumQParts, m_ppcYuvReco[uiDepth], m_ppcYuvReco[uiDepth], m_ppcYuvResi[uiDepth] );
  }
#endif

}

#if LM_CHROMA

/** Funtion for deriving recontructed PU/CU Luma sample with QTree structure
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

/** Funtion for deriving recontructed PU/CU chroma samples with QTree structure
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
#endif

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
#if POZNAN_TEXTURE_TU_DELTA_QP_ACCORDING_TO_DEPTH
  Int QPoffset = pcCU->getQpOffsetForTextCU(uiAbsPartIdx, false);
  m_pcTrQuant->setQPforQuant  ( pcCU->getQP(uiAbsPartIdx) + QPoffset, !pcCU->getSlice()->getDepth(), pcCU->getSlice()->getSliceType(), TEXT_LUMA );
#else
  m_pcTrQuant->setQPforQuant( pcCU->getQP( uiAbsPartIdx ), !pcCU->getSlice()->getDepth(), pcCU->getSlice()->getSliceType(), TEXT_LUMA );
#endif
  m_pcTrQuant->invRecurTransformNxN ( pcCU, 0, TEXT_LUMA, pResi, 0, m_ppcYuvResi[uiDepth]->getStride(), uiWidth, uiHeight, uiLumaTrMode, 0, piCoeff );
  
  // Cb and Cr
#if POZNAN_TEXTURE_TU_DELTA_QP_ACCORDING_TO_DEPTH
    m_pcTrQuant->setQPforQuant( pcCU->getQP( uiAbsPartIdx ) + QPoffset, !pcCU->getSlice()->getDepth(), pcCU->getSlice()->getSliceType(), TEXT_CHROMA );
#else
  m_pcTrQuant->setQPforQuant( pcCU->getQP( uiAbsPartIdx ), !pcCU->getSlice()->getDepth(), pcCU->getSlice()->getSliceType(), TEXT_CHROMA );
#endif
  
  uiWidth  >>= 1;
  uiHeight >>= 1;
  piCoeff = pcCU->getCoeffCb(); pResi = m_ppcYuvResi[uiDepth]->getCbAddr();
  m_pcTrQuant->invRecurTransformNxN ( pcCU, 0, TEXT_CHROMA_U, pResi, 0, m_ppcYuvResi[uiDepth]->getCStride(), uiWidth, uiHeight, uiChromaTrMode, 0, piCoeff );
  piCoeff = pcCU->getCoeffCr(); pResi = m_ppcYuvResi[uiDepth]->getCrAddr();
  m_pcTrQuant->invRecurTransformNxN ( pcCU, 0, TEXT_CHROMA_V, pResi, 0, m_ppcYuvResi[uiDepth]->getCStride(), uiWidth, uiHeight, uiChromaTrMode, 0, piCoeff );
}

