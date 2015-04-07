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

/** \file     TEncEntropy.cpp
    \brief    entropy encoder class
*/

#include "TEncEntropy.h"
#include "TLibCommon/TypeDef.h"
#include "TLibCommon/TComSampleAdaptiveOffset.h"

//! \ingroup TLibEncoder
//! \{

Void TEncEntropy::setEntropyCoder ( TEncEntropyIf* e, TComSlice* pcSlice )
{
  m_pcEntropyCoderIf = e;
  m_pcEntropyCoderIf->setSlice ( pcSlice );
}

Void TEncEntropy::encodeSliceHeader ( TComSlice* pcSlice )
{
  m_pcEntropyCoderIf->codeSliceHeader( pcSlice );
  return;
}

Void  TEncEntropy::encodeTilesWPPEntryPoint( TComSlice* pSlice )
{
  m_pcEntropyCoderIf->codeTilesWPPEntryPoint( pSlice );
}

Void TEncEntropy::encodeTerminatingBit      ( UInt uiIsLast )
{
  m_pcEntropyCoderIf->codeTerminatingBit( uiIsLast );
  
  return;
}

Void TEncEntropy::encodeSliceFinish()
{
  m_pcEntropyCoderIf->codeSliceFinish();
}

Void TEncEntropy::encodePPS( TComPPS* pcPPS )
{
  m_pcEntropyCoderIf->codePPS( pcPPS );
  return;
}

Void TEncEntropy::encodeSPS( TComSPS* pcSPS )
{
  m_pcEntropyCoderIf->codeSPS( pcSPS );
  return;
}

Void TEncEntropy::encodeCUTransquantBypassFlag( TComDataCU* pcCU, UInt uiAbsPartIdx, Bool bRD )
{
  if( bRD )
  {
    uiAbsPartIdx = 0;
  }
  m_pcEntropyCoderIf->codeCUTransquantBypassFlag( pcCU, uiAbsPartIdx );
}

Void TEncEntropy::encodeVPS( TComVPS* pcVPS )
{
  m_pcEntropyCoderIf->codeVPS( pcVPS );
  return;
}

Void TEncEntropy::encodeSkipFlag( TComDataCU* pcCU, UInt uiAbsPartIdx, Bool bRD )
{
  if ( pcCU->getSlice()->isIntra() )
  {
    return;
  }
  if( bRD )
  {
    uiAbsPartIdx = 0;
  }
  m_pcEntropyCoderIf->codeSkipFlag( pcCU, uiAbsPartIdx );
}
#if H_3D
Void TEncEntropy::encodeDIS( TComDataCU* pcCU, UInt uiAbsPartIdx, Bool bRD )
{
  if ( !pcCU->getSlice()->getIsDepth() )
  {
    return;
  }
  if( bRD )
  {
    uiAbsPartIdx = 0;
  }
  m_pcEntropyCoderIf->codeDIS( pcCU, uiAbsPartIdx );
}
#endif
/** encode merge flag
 * \param pcCU
 * \param uiAbsPartIdx
 * \returns Void
 */
Void TEncEntropy::encodeMergeFlag( TComDataCU* pcCU, UInt uiAbsPartIdx )
{ 
  // at least one merge candidate exists
  m_pcEntropyCoderIf->codeMergeFlag( pcCU, uiAbsPartIdx );
}

/** encode merge index
 * \param pcCU
 * \param uiAbsPartIdx
 * \param uiPUIdx
 * \param bRD
 * \returns Void
 */
Void TEncEntropy::encodeMergeIndex( TComDataCU* pcCU, UInt uiAbsPartIdx, Bool bRD )
{
  if( bRD )
  {
    uiAbsPartIdx = 0;
    assert( pcCU->getPartitionSize(uiAbsPartIdx) == SIZE_2Nx2N );
  }
  m_pcEntropyCoderIf->codeMergeIndex( pcCU, uiAbsPartIdx );
}

#if H_3D_IC
Void TEncEntropy::encodeICFlag( TComDataCU* pcCU, UInt uiAbsPartIdx, Bool bRD )
{
  if ( pcCU->isIntra( uiAbsPartIdx ) || ( pcCU->getSlice()->getViewIndex() == 0 ) || pcCU->getSlice()->getIsDepth() || pcCU->getARPW( uiAbsPartIdx ) > 0 )
  {
    return;
  }

  if( !pcCU->getSlice()->getApplyIC() )
  {
    return;
  }

  if( bRD )
  {
    uiAbsPartIdx = 0;
  }
  else
  {
    Int ICEnableCandidate = pcCU->getSlice()->getICEnableCandidate(pcCU->getSlice()->getDepth());
    Int ICEnableNum = pcCU->getSlice()->getICEnableNum(pcCU->getSlice()->getDepth());
    ICEnableCandidate++;
    if(pcCU->getICFlag(uiAbsPartIdx))
    {
      ICEnableNum++;
    }
    pcCU->getSlice()->setICEnableCandidate(pcCU->getSlice()->getDepth(), ICEnableCandidate);
    pcCU->getSlice()->setICEnableNum(pcCU->getSlice()->getDepth(), ICEnableNum);
  }
  if( pcCU->isICFlagRequired( uiAbsPartIdx ) )
  {
    m_pcEntropyCoderIf->codeICFlag( pcCU, uiAbsPartIdx );
  }
}
#endif

#if H_3D_ARP
Void TEncEntropy::encodeARPW( TComDataCU* pcCU, UInt uiAbsPartIdx )
{
  if( !pcCU->getSlice()->getARPStepNum() || pcCU->isIntra( uiAbsPartIdx ) ) 
  {
    return;
  }

  if ( pcCU->getPartitionSize(uiAbsPartIdx)!=SIZE_2Nx2N )
  {
    assert(pcCU->getARPW (uiAbsPartIdx) == 0);
  }
  else
  {
    m_pcEntropyCoderIf->codeARPW( pcCU, uiAbsPartIdx );
  }
}
#endif

/** encode prediction mode
 * \param pcCU
 * \param uiAbsPartIdx
 * \param bRD
 * \returns Void
 */
Void TEncEntropy::encodePredMode( TComDataCU* pcCU, UInt uiAbsPartIdx, Bool bRD )
{
  if( bRD )
  {
    uiAbsPartIdx = 0;
  }
  if ( pcCU->getSlice()->isIntra() )
  {
    return;
  }

  m_pcEntropyCoderIf->codePredMode( pcCU, uiAbsPartIdx );
}

// Split mode
Void TEncEntropy::encodeSplitFlag( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth, Bool bRD )
{
  if( bRD )
  {
    uiAbsPartIdx = 0;
  }
  m_pcEntropyCoderIf->codeSplitFlag( pcCU, uiAbsPartIdx, uiDepth );
}

/** encode partition size
 * \param pcCU
 * \param uiAbsPartIdx
 * \param uiDepth
 * \param bRD
 * \returns Void
 */
Void TEncEntropy::encodePartSize( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth, Bool bRD )
{
  if( bRD )
  {
    uiAbsPartIdx = 0;
  }
  
  m_pcEntropyCoderIf->codePartSize( pcCU, uiAbsPartIdx, uiDepth );
  
}

/** Encode I_PCM information. 
 * \param pcCU pointer to CU 
 * \param uiAbsPartIdx CU index
 * \param bRD flag indicating estimation or encoding
 * \returns Void
 */
Void TEncEntropy::encodeIPCMInfo( TComDataCU* pcCU, UInt uiAbsPartIdx, Bool bRD )
{
  if(!pcCU->getSlice()->getSPS()->getUsePCM()
    || pcCU->getWidth(uiAbsPartIdx) > (1<<pcCU->getSlice()->getSPS()->getPCMLog2MaxSize())
    || pcCU->getWidth(uiAbsPartIdx) < (1<<pcCU->getSlice()->getSPS()->getPCMLog2MinSize()))
  {
    return;
  }
  
  if( bRD )
  {
    uiAbsPartIdx = 0;
  }
  
  m_pcEntropyCoderIf->codeIPCMInfo ( pcCU, uiAbsPartIdx );
}

#if H_3D_DISABLE_CHROMA
Void TEncEntropy::xEncodeTransform( TComDataCU* pcCU,UInt offsetLuma, UInt offsetChroma, UInt uiAbsPartIdx, UInt uiDepth, UInt width, UInt height, UInt uiTrIdx, Bool& bCodeDQP, Bool rd )
#else
Void TEncEntropy::xEncodeTransform( TComDataCU* pcCU,UInt offsetLuma, UInt offsetChroma, UInt uiAbsPartIdx, UInt uiDepth, UInt width, UInt height, UInt uiTrIdx, Bool& bCodeDQP )
#endif
{

#if H_MV_ENC_DEC_TRAC
#if ENC_DEC_TRACE
  UInt uiLPelX   = pcCU->getCUPelX() + g_auiRasterToPelX[ g_auiZscanToRaster[uiAbsPartIdx] ];
  UInt uiTPelY   = pcCU->getCUPelY() + g_auiRasterToPelY[ g_auiZscanToRaster[uiAbsPartIdx] ];

  DTRACE_TU_S("=========== transform_tree ===========\n")
  DTRACE_TU("x0", uiLPelX)
  DTRACE_TU("x1", uiTPelY)
  DTRACE_TU("log2TrafoSize", g_uiMaxCUWidth>>uiDepth)
  DTRACE_TU("trafoDepth"  , uiDepth)
#endif
#endif

  const UInt uiSubdiv = pcCU->getTransformIdx( uiAbsPartIdx ) + pcCU->getDepth( uiAbsPartIdx ) > uiDepth;
  const UInt uiLog2TrafoSize = g_aucConvertToBit[pcCU->getSlice()->getSPS()->getMaxCUWidth()]+2 - uiDepth;
  UInt cbfY = pcCU->getCbf( uiAbsPartIdx, TEXT_LUMA    , uiTrIdx );
  UInt cbfU = pcCU->getCbf( uiAbsPartIdx, TEXT_CHROMA_U, uiTrIdx );
  UInt cbfV = pcCU->getCbf( uiAbsPartIdx, TEXT_CHROMA_V, uiTrIdx );
#if H_3D_DISABLE_CHROMA
  if( !rd && pcCU->getSlice()->getSPS()->getChromaFormatIdc() == 0 )
  {
    cbfU = 0;
    cbfV = 0;
  }
#endif
  if(uiTrIdx==0)
  {
    m_bakAbsPartIdxCU = uiAbsPartIdx;
  }
  if( uiLog2TrafoSize == 2 )
  {
    UInt partNum = pcCU->getPic()->getNumPartInCU() >> ( ( uiDepth - 1 ) << 1 );
    if( ( uiAbsPartIdx % partNum ) == 0 )
    {
      m_uiBakAbsPartIdx   = uiAbsPartIdx;
      m_uiBakChromaOffset = offsetChroma;
    }
    else if( ( uiAbsPartIdx % partNum ) == (partNum - 1) )
    {
      cbfU = pcCU->getCbf( m_uiBakAbsPartIdx, TEXT_CHROMA_U, uiTrIdx );
      cbfV = pcCU->getCbf( m_uiBakAbsPartIdx, TEXT_CHROMA_V, uiTrIdx );
#if H_3D_DISABLE_CHROMA
      if( !rd && pcCU->getSlice()->getSPS()->getChromaFormatIdc() == 0 )
      {
        cbfU = 0;
        cbfV = 0;
      }
#endif
    }
  }
  
  if( pcCU->getPredictionMode(uiAbsPartIdx) == MODE_INTRA && pcCU->getPartitionSize(uiAbsPartIdx) == SIZE_NxN && uiDepth == pcCU->getDepth(uiAbsPartIdx) )
  {
    assert( uiSubdiv );
  }
  else if( pcCU->getPredictionMode(uiAbsPartIdx) == MODE_INTER && (pcCU->getPartitionSize(uiAbsPartIdx) != SIZE_2Nx2N) && uiDepth == pcCU->getDepth(uiAbsPartIdx) &&  (pcCU->getSlice()->getSPS()->getQuadtreeTUMaxDepthInter() == 1) )
  {
    if ( uiLog2TrafoSize > pcCU->getQuadtreeTULog2MinSizeInCU(uiAbsPartIdx) )
    {
      assert( uiSubdiv );
    }
    else
    {
      assert(!uiSubdiv );
    }
  }
  else if( uiLog2TrafoSize > pcCU->getSlice()->getSPS()->getQuadtreeTULog2MaxSize() )
  {
    assert( uiSubdiv );
  }
  else if( uiLog2TrafoSize == pcCU->getSlice()->getSPS()->getQuadtreeTULog2MinSize() )
  {
    assert( !uiSubdiv );
  }
  else if( uiLog2TrafoSize == pcCU->getQuadtreeTULog2MinSizeInCU(uiAbsPartIdx) )
  {
    assert( !uiSubdiv );
  }
  else
  {
    assert( uiLog2TrafoSize > pcCU->getQuadtreeTULog2MinSizeInCU(uiAbsPartIdx) );
    m_pcEntropyCoderIf->codeTransformSubdivFlag( uiSubdiv, 5 - uiLog2TrafoSize );
  }

  const UInt uiTrDepthCurr = uiDepth - pcCU->getDepth( uiAbsPartIdx );
  const Bool bFirstCbfOfCU = uiTrDepthCurr == 0;
  if( bFirstCbfOfCU || uiLog2TrafoSize > 2 )
  {
#if H_3D_DISABLE_CHROMA
    if (pcCU->getSlice()->getSPS()->getChromaFormatIdc() != 0 || rd)
    {
      if( bFirstCbfOfCU || pcCU->getCbf( uiAbsPartIdx, TEXT_CHROMA_U, uiTrDepthCurr - 1 ) )
      {
        m_pcEntropyCoderIf->codeQtCbf( pcCU, uiAbsPartIdx, TEXT_CHROMA_U, uiTrDepthCurr );
      }
      if( bFirstCbfOfCU || pcCU->getCbf( uiAbsPartIdx, TEXT_CHROMA_V, uiTrDepthCurr - 1 ) )
      {
        m_pcEntropyCoderIf->codeQtCbf( pcCU, uiAbsPartIdx, TEXT_CHROMA_V, uiTrDepthCurr );
      }
    }
#else
    if( bFirstCbfOfCU || pcCU->getCbf( uiAbsPartIdx, TEXT_CHROMA_U, uiTrDepthCurr - 1 ) )
    {
      m_pcEntropyCoderIf->codeQtCbf( pcCU, uiAbsPartIdx, TEXT_CHROMA_U, uiTrDepthCurr );
    }
    if( bFirstCbfOfCU || pcCU->getCbf( uiAbsPartIdx, TEXT_CHROMA_V, uiTrDepthCurr - 1 ) )
    {
      m_pcEntropyCoderIf->codeQtCbf( pcCU, uiAbsPartIdx, TEXT_CHROMA_V, uiTrDepthCurr );
    }
#endif
  }
  else if( uiLog2TrafoSize == 2 )
  {
#if H_3D_DISABLE_CHROMA
    if ( rd && pcCU->getSlice()->getSPS()->getChromaFormatIdc() != 0 )
    {
    assert( pcCU->getCbf( uiAbsPartIdx, TEXT_CHROMA_U, uiTrDepthCurr ) == pcCU->getCbf( uiAbsPartIdx, TEXT_CHROMA_U, uiTrDepthCurr - 1 ) );
    assert( pcCU->getCbf( uiAbsPartIdx, TEXT_CHROMA_V, uiTrDepthCurr ) == pcCU->getCbf( uiAbsPartIdx, TEXT_CHROMA_V, uiTrDepthCurr - 1 ) );
  }
#else
    assert( pcCU->getCbf( uiAbsPartIdx, TEXT_CHROMA_U, uiTrDepthCurr ) == pcCU->getCbf( uiAbsPartIdx, TEXT_CHROMA_U, uiTrDepthCurr - 1 ) );
    assert( pcCU->getCbf( uiAbsPartIdx, TEXT_CHROMA_V, uiTrDepthCurr ) == pcCU->getCbf( uiAbsPartIdx, TEXT_CHROMA_V, uiTrDepthCurr - 1 ) );
#endif
  }
  
  if( uiSubdiv )
  {
    UInt size;
    width  >>= 1;
    height >>= 1;
    size = width*height;
    uiTrIdx++;
    ++uiDepth;
    const UInt partNum = pcCU->getPic()->getNumPartInCU() >> (uiDepth << 1);
    
#if H_3D_DISABLE_CHROMA
    xEncodeTransform( pcCU, offsetLuma, offsetChroma, uiAbsPartIdx, uiDepth, width, height, uiTrIdx, bCodeDQP, rd );

    uiAbsPartIdx += partNum;  offsetLuma += size;  offsetChroma += (size>>2);
    xEncodeTransform( pcCU, offsetLuma, offsetChroma, uiAbsPartIdx, uiDepth, width, height, uiTrIdx, bCodeDQP ,rd );

    uiAbsPartIdx += partNum;  offsetLuma += size;  offsetChroma += (size>>2);
    xEncodeTransform( pcCU, offsetLuma, offsetChroma, uiAbsPartIdx, uiDepth, width, height, uiTrIdx, bCodeDQP, rd );

    uiAbsPartIdx += partNum;  offsetLuma += size;  offsetChroma += (size>>2);
    xEncodeTransform( pcCU, offsetLuma, offsetChroma, uiAbsPartIdx, uiDepth, width, height, uiTrIdx, bCodeDQP, rd );
#else
    xEncodeTransform( pcCU, offsetLuma, offsetChroma, uiAbsPartIdx, uiDepth, width, height, uiTrIdx, bCodeDQP );

    uiAbsPartIdx += partNum;  offsetLuma += size;  offsetChroma += (size>>2);
    xEncodeTransform( pcCU, offsetLuma, offsetChroma, uiAbsPartIdx, uiDepth, width, height, uiTrIdx, bCodeDQP );

    uiAbsPartIdx += partNum;  offsetLuma += size;  offsetChroma += (size>>2);
    xEncodeTransform( pcCU, offsetLuma, offsetChroma, uiAbsPartIdx, uiDepth, width, height, uiTrIdx, bCodeDQP );

    uiAbsPartIdx += partNum;  offsetLuma += size;  offsetChroma += (size>>2);
    xEncodeTransform( pcCU, offsetLuma, offsetChroma, uiAbsPartIdx, uiDepth, width, height, uiTrIdx, bCodeDQP );
#endif
  }
  else
  {
#if !H_MV_ENC_DEC_TRAC
    {
      DTRACE_CABAC_VL( g_nSymbolCounter++ );
      DTRACE_CABAC_T( "\tTrIdx: abspart=" );
      DTRACE_CABAC_V( uiAbsPartIdx );
      DTRACE_CABAC_T( "\tdepth=" );
      DTRACE_CABAC_V( uiDepth );
      DTRACE_CABAC_T( "\ttrdepth=" );
      DTRACE_CABAC_V( pcCU->getTransformIdx( uiAbsPartIdx ) );
      DTRACE_CABAC_T( "\n" );
    }
#endif
    
#if H_3D_DISABLE_CHROMA
    Bool notcbfUV = !rd && pcCU->getSlice()->getSPS()->getChromaFormatIdc() == 0 ? 1 : ( !pcCU->getCbf( uiAbsPartIdx, TEXT_CHROMA_U, 0 ) && !pcCU->getCbf( uiAbsPartIdx, TEXT_CHROMA_V, 0 ) ) ;
    if( pcCU->getPredictionMode(uiAbsPartIdx) != MODE_INTRA && 
      uiDepth == pcCU->getDepth( uiAbsPartIdx ) && notcbfUV ) 
#else
    if( pcCU->getPredictionMode(uiAbsPartIdx) != MODE_INTRA && uiDepth == pcCU->getDepth( uiAbsPartIdx ) && !pcCU->getCbf( uiAbsPartIdx, TEXT_CHROMA_U, 0 ) && !pcCU->getCbf( uiAbsPartIdx, TEXT_CHROMA_V, 0 ) )
#endif
    {
      assert( pcCU->getCbf( uiAbsPartIdx, TEXT_LUMA, 0 ) );
      //      printf( "saved one bin! " );
    }
    else
    {
      m_pcEntropyCoderIf->codeQtCbf( pcCU, uiAbsPartIdx, TEXT_LUMA, pcCU->getTransformIdx( uiAbsPartIdx ) );
    }


    if ( cbfY || cbfU || cbfV )
    {
      // dQP: only for LCU once
      if ( pcCU->getSlice()->getPPS()->getUseDQP() )
      {
        if ( bCodeDQP )
        {
          encodeQP( pcCU, m_bakAbsPartIdxCU );
          bCodeDQP = false;
        }
      }
    }
    if( cbfY )
    {
      Int trWidth = width;
      Int trHeight = height;
      m_pcEntropyCoderIf->codeCoeffNxN( pcCU, (pcCU->getCoeffY()+offsetLuma), uiAbsPartIdx, trWidth, trHeight, uiDepth, TEXT_LUMA );
    }
    if( uiLog2TrafoSize > 2 )
    {
      Int trWidth = width >> 1;
      Int trHeight = height >> 1;
      if( cbfU )
      {
        m_pcEntropyCoderIf->codeCoeffNxN( pcCU, (pcCU->getCoeffCb()+offsetChroma), uiAbsPartIdx, trWidth, trHeight, uiDepth, TEXT_CHROMA_U );
      }
      if( cbfV )
      {
        m_pcEntropyCoderIf->codeCoeffNxN( pcCU, (pcCU->getCoeffCr()+offsetChroma), uiAbsPartIdx, trWidth, trHeight, uiDepth, TEXT_CHROMA_V );
      }
    }
    else
    {
      UInt partNum = pcCU->getPic()->getNumPartInCU() >> ( ( uiDepth - 1 ) << 1 );
      if( ( uiAbsPartIdx % partNum ) == (partNum - 1) )
      {
        Int trWidth = width;
        Int trHeight = height;
        if( cbfU )
        {
          m_pcEntropyCoderIf->codeCoeffNxN( pcCU, (pcCU->getCoeffCb()+m_uiBakChromaOffset), m_uiBakAbsPartIdx, trWidth, trHeight, uiDepth, TEXT_CHROMA_U );
        }
        if( cbfV )
        {
          m_pcEntropyCoderIf->codeCoeffNxN( pcCU, (pcCU->getCoeffCr()+m_uiBakChromaOffset), m_uiBakAbsPartIdx, trWidth, trHeight, uiDepth, TEXT_CHROMA_V );
        }
      }
    }
  }
}

// Intra direction for Luma
Void TEncEntropy::encodeIntraDirModeLuma  ( TComDataCU* pcCU, UInt absPartIdx, Bool isMultiplePU )
{
  m_pcEntropyCoderIf->codeIntraDirLumaAng( pcCU, absPartIdx , isMultiplePU);
}

// Intra direction for Chroma
Void TEncEntropy::encodeIntraDirModeChroma( TComDataCU* pcCU, UInt uiAbsPartIdx, Bool bRD )
{
  if( bRD )
  {
    uiAbsPartIdx = 0;
  }
  
  m_pcEntropyCoderIf->codeIntraDirChroma( pcCU, uiAbsPartIdx );
}

Void TEncEntropy::encodePredInfo( TComDataCU* pcCU, UInt uiAbsPartIdx, Bool bRD )
{
  if( bRD )
  {
    uiAbsPartIdx = 0;
  }
  if( pcCU->isIntra( uiAbsPartIdx ) )                                 // If it is Intra mode, encode intra prediction mode.
  {
    encodeIntraDirModeLuma  ( pcCU, uiAbsPartIdx,true );
#if H_3D_DIM_SDC
#if H_3D_DISABLE_CHROMA
    if(!pcCU->getSDCFlag(uiAbsPartIdx) && ( pcCU->getSlice()->getSPS()->getChromaFormatIdc() != 0  || bRD ) ) 
#else
    if(!pcCU->getSDCFlag(uiAbsPartIdx))
#endif
#endif
    encodeIntraDirModeChroma( pcCU, uiAbsPartIdx, bRD );
  }
  else                                                                // if it is Inter mode, encode motion vector and reference index
  {
    encodePUWise( pcCU, uiAbsPartIdx, bRD );
  }
}

/** encode motion information for every PU block
 * \param pcCU
 * \param uiAbsPartIdx
 * \param bRD
 * \returns Void
 */
Void TEncEntropy::encodePUWise( TComDataCU* pcCU, UInt uiAbsPartIdx, Bool bRD )
{
  if ( bRD )
  {
    uiAbsPartIdx = 0;
  }
  
  PartSize ePartSize = pcCU->getPartitionSize( uiAbsPartIdx );
  UInt uiNumPU = ( ePartSize == SIZE_2Nx2N ? 1 : ( ePartSize == SIZE_NxN ? 4 : 2 ) );
  UInt uiDepth = pcCU->getDepth( uiAbsPartIdx );
  UInt uiPUOffset = ( g_auiPUOffset[UInt( ePartSize )] << ( ( pcCU->getSlice()->getSPS()->getMaxCUDepth() - uiDepth ) << 1 ) ) >> 4;

  for ( UInt uiPartIdx = 0, uiSubPartIdx = uiAbsPartIdx; uiPartIdx < uiNumPU; uiPartIdx++, uiSubPartIdx += uiPUOffset )
  {
#if H_MV_ENC_DEC_TRAC
    DTRACE_PU_S("=========== prediction_unit ===========\n")
       //Todo: 
      //DTRACE_PU("x0", uiLPelX)
      //DTRACE_PU("x1", uiTPelY)
#endif
    encodeMergeFlag( pcCU, uiSubPartIdx );
    if ( pcCU->getMergeFlag( uiSubPartIdx ) )
    {
      encodeMergeIndex( pcCU, uiSubPartIdx );
    }
    else
    {
      encodeInterDirPU( pcCU, uiSubPartIdx );
      for ( UInt uiRefListIdx = 0; uiRefListIdx < 2; uiRefListIdx++ )
      {
        if ( pcCU->getSlice()->getNumRefIdx( RefPicList( uiRefListIdx ) ) > 0 )
        {
          encodeRefFrmIdxPU ( pcCU, uiSubPartIdx, RefPicList( uiRefListIdx ) );
          encodeMvdPU       ( pcCU, uiSubPartIdx, RefPicList( uiRefListIdx ) );
          encodeMVPIdxPU    ( pcCU, uiSubPartIdx, RefPicList( uiRefListIdx ) );
        }
      }
    }
  }

  return;
}

Void TEncEntropy::encodeInterDirPU( TComDataCU* pcCU, UInt uiAbsPartIdx )
{
  if ( !pcCU->getSlice()->isInterB() )
  {
    return;
  }

  m_pcEntropyCoderIf->codeInterDir( pcCU, uiAbsPartIdx );
  return;
}

/** encode reference frame index for a PU block
 * \param pcCU
 * \param uiAbsPartIdx
 * \param eRefList
 * \returns Void
 */
Void TEncEntropy::encodeRefFrmIdxPU( TComDataCU* pcCU, UInt uiAbsPartIdx, RefPicList eRefList )
{
  assert( !pcCU->isIntra( uiAbsPartIdx ) );
  {
    if ( ( pcCU->getSlice()->getNumRefIdx( eRefList ) == 1 ) )
    {
      return;
    }

    if ( pcCU->getInterDir( uiAbsPartIdx ) & ( 1 << eRefList ) )
    {
      m_pcEntropyCoderIf->codeRefFrmIdx( pcCU, uiAbsPartIdx, eRefList );
    }
  }

  return;
}

/** encode motion vector difference for a PU block
 * \param pcCU
 * \param uiAbsPartIdx
 * \param eRefList
 * \returns Void
 */
Void TEncEntropy::encodeMvdPU( TComDataCU* pcCU, UInt uiAbsPartIdx, RefPicList eRefList )
{
  assert( !pcCU->isIntra( uiAbsPartIdx ) );

  if ( pcCU->getInterDir( uiAbsPartIdx ) & ( 1 << eRefList ) )
  {
    m_pcEntropyCoderIf->codeMvd( pcCU, uiAbsPartIdx, eRefList );
  }
  return;
}

Void TEncEntropy::encodeMVPIdxPU( TComDataCU* pcCU, UInt uiAbsPartIdx, RefPicList eRefList )
{
  if ( (pcCU->getInterDir( uiAbsPartIdx ) & ( 1 << eRefList )) )
  {
    m_pcEntropyCoderIf->codeMVPIdx( pcCU, uiAbsPartIdx, eRefList );
  }

  return;
}

Void TEncEntropy::encodeQtCbf( TComDataCU* pcCU, UInt uiAbsPartIdx, TextType eType, UInt uiTrDepth )
{
  m_pcEntropyCoderIf->codeQtCbf( pcCU, uiAbsPartIdx, eType, uiTrDepth );
}

Void TEncEntropy::encodeTransformSubdivFlag( UInt uiSymbol, UInt uiCtx )
{
  m_pcEntropyCoderIf->codeTransformSubdivFlag( uiSymbol, uiCtx );
}

Void TEncEntropy::encodeQtRootCbf( TComDataCU* pcCU, UInt uiAbsPartIdx )
{
  m_pcEntropyCoderIf->codeQtRootCbf( pcCU, uiAbsPartIdx );
}

Void TEncEntropy::encodeQtCbfZero( TComDataCU* pcCU, TextType eType, UInt uiTrDepth )
{
  m_pcEntropyCoderIf->codeQtCbfZero( pcCU, eType, uiTrDepth );
}
Void TEncEntropy::encodeQtRootCbfZero( TComDataCU* pcCU )
{
  m_pcEntropyCoderIf->codeQtRootCbfZero( pcCU );
}

// dQP
Void TEncEntropy::encodeQP( TComDataCU* pcCU, UInt uiAbsPartIdx, Bool bRD )
{
  if( bRD )
  {
    uiAbsPartIdx = 0;
  }
  
  if ( pcCU->getSlice()->getPPS()->getUseDQP() )
  {
    m_pcEntropyCoderIf->codeDeltaQP( pcCU, uiAbsPartIdx );
  }
}


// texture
/** encode coefficients
 * \param pcCU
 * \param uiAbsPartIdx
 * \param uiDepth
 * \param uiWidth
 * \param uiHeight
 */
#if H_3D_DISABLE_CHROMA
Void TEncEntropy::encodeCoeff( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth, UInt uiWidth, UInt uiHeight, Bool& bCodeDQP, Bool rd )
#else
Void TEncEntropy::encodeCoeff( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth, UInt uiWidth, UInt uiHeight, Bool& bCodeDQP )
#endif
{
  UInt uiMinCoeffSize = pcCU->getPic()->getMinCUWidth()*pcCU->getPic()->getMinCUHeight();
  UInt uiLumaOffset   = uiMinCoeffSize*uiAbsPartIdx;
  UInt uiChromaOffset = uiLumaOffset>>2;
#if H_3D_DIM_SDC
  if( pcCU->getSDCFlag( uiAbsPartIdx ) && pcCU->isIntra( uiAbsPartIdx ) )
  {
    assert( pcCU->getPartitionSize(uiAbsPartIdx) == SIZE_2Nx2N );
    assert( pcCU->getTransformIdx(uiAbsPartIdx) == 0 );
    assert( pcCU->getCbf(uiAbsPartIdx, TEXT_LUMA) == 1 );
#if H_3D_DISABLE_CHROMA
    if (!pcCU->getSlice()->getIsDepth() )
    {
      assert( pcCU->getCbf(uiAbsPartIdx, TEXT_CHROMA_U) == 1 );
      assert( pcCU->getCbf(uiAbsPartIdx, TEXT_CHROMA_V) == 1 );
    }
#else
    assert( pcCU->getCbf(uiAbsPartIdx, TEXT_CHROMA_U) == 1 );
    assert( pcCU->getCbf(uiAbsPartIdx, TEXT_CHROMA_V) == 1 );
#endif
  }


  if( pcCU->getSDCFlag( uiAbsPartIdx ) && !pcCU->isIntra( uiAbsPartIdx ) )
  {
    assert( !pcCU->isSkipped( uiAbsPartIdx ) );
    assert( !pcCU->isIntra( uiAbsPartIdx) );
    assert( pcCU->getSlice()->getIsDepth() );
  }

  if( pcCU->getSlice()->getIsDepth() && ( pcCU->getSDCFlag( uiAbsPartIdx ) || pcCU->isIntra( uiAbsPartIdx ) ) )
  {
    Int iPartNum = ( pcCU->isIntra( uiAbsPartIdx ) && pcCU->getPartitionSize( uiAbsPartIdx ) == SIZE_NxN ) ? 4 : 1;
    UInt uiPartOffset = ( pcCU->getPic()->getNumPartInCU() >> ( pcCU->getDepth( uiAbsPartIdx ) << 1 ) ) >> 2;
    
    if( !pcCU->getSDCFlag( uiAbsPartIdx ) )
    {
      for( Int iPart = 0; iPart < iPartNum; iPart++ )
      {
        if( getDimType( pcCU->getLumaIntraDir( uiAbsPartIdx + uiPartOffset*iPart ) ) < DIM_NUM_TYPE ) 
        {
          m_pcEntropyCoderIf->codeDeltaDC( pcCU, uiAbsPartIdx + uiPartOffset*iPart );
        }
      }
    }
    else
    {
      m_pcEntropyCoderIf->codeDeltaDC( pcCU, uiAbsPartIdx );
      return;
    }
  }
#endif

  if( pcCU->isIntra(uiAbsPartIdx) )
  {
#if !H_MV
    DTRACE_CABAC_VL( g_nSymbolCounter++ )
    DTRACE_CABAC_T( "\tdecodeTransformIdx()\tCUDepth=" )
    DTRACE_CABAC_V( uiDepth )
    DTRACE_CABAC_T( "\n" )
#endif
  }
  else
  {
    if( !(pcCU->getMergeFlag( uiAbsPartIdx ) && pcCU->getPartitionSize(uiAbsPartIdx) == SIZE_2Nx2N ) )
    {
      m_pcEntropyCoderIf->codeQtRootCbf( pcCU, uiAbsPartIdx );
    }
    if ( !pcCU->getQtRootCbf( uiAbsPartIdx ) )
    {
      return;
    }
  }
  
#if H_3D_DISABLE_CHROMA
  xEncodeTransform( pcCU, uiLumaOffset, uiChromaOffset, uiAbsPartIdx, uiDepth, uiWidth, uiHeight, 0, bCodeDQP, rd);
#else
  xEncodeTransform( pcCU, uiLumaOffset, uiChromaOffset, uiAbsPartIdx, uiDepth, uiWidth, uiHeight, 0, bCodeDQP);
#endif
}

Void TEncEntropy::encodeCoeffNxN( TComDataCU* pcCU, TCoeff* pcCoeff, UInt uiAbsPartIdx, UInt uiTrWidth, UInt uiTrHeight, UInt uiDepth, TextType eType )
{
  // This is for Transform unit processing. This may be used at mode selection stage for Inter.
  m_pcEntropyCoderIf->codeCoeffNxN( pcCU, pcCoeff, uiAbsPartIdx, uiTrWidth, uiTrHeight, uiDepth, eType );
}

Void TEncEntropy::estimateBit (estBitsSbacStruct* pcEstBitsSbac, Int width, Int height, TextType eTType)
{  
  eTType = eTType == TEXT_LUMA ? TEXT_LUMA : TEXT_CHROMA;
  
  m_pcEntropyCoderIf->estBit ( pcEstBitsSbac, width, height, eTType );
}

Int TEncEntropy::countNonZeroCoeffs( TCoeff* pcCoef, UInt uiSize )
{
  Int count = 0;
  
  for ( Int i = 0; i < uiSize; i++ )
  {
    count += pcCoef[i] != 0;
  }
  
  return count;
}

/** encode quantization matrix
 * \param scalingList quantization matrix information
 */
Void TEncEntropy::encodeScalingList( TComScalingList* scalingList )
{
  m_pcEntropyCoderIf->codeScalingList( scalingList );
}

#if H_3D_INTER_SDC
Void TEncEntropy::encodeDeltaDC  ( TComDataCU* pcCU, UInt absPartIdx )
{
  m_pcEntropyCoderIf->codeDeltaDC( pcCU, absPartIdx );
}

Void TEncEntropy::encodeSDCFlag( TComDataCU* pcCU, UInt uiAbsPartIdx, Bool bRD )
{
  if( ( !pcCU->isIntra( uiAbsPartIdx ) && !pcCU->getSlice()->getInterSdcFlag() ) || 
    ( pcCU->isIntra( uiAbsPartIdx ) && !pcCU->getSlice()->getIntraSdcWedgeFlag() ) )
  {
    return;
  }

  if( !pcCU->getSlice()->getIsDepth() || pcCU->getPartitionSize( uiAbsPartIdx ) != SIZE_2Nx2N || pcCU->isSkipped( uiAbsPartIdx ) )
  {
    return;
  }

  assert( pcCU->getPartitionSize( uiAbsPartIdx ) == SIZE_2Nx2N || ( !pcCU->isIntra( uiAbsPartIdx ) && !pcCU->isSkipped( uiAbsPartIdx ) ) );

  if( bRD )
  {
    uiAbsPartIdx = 0;
  }

  m_pcEntropyCoderIf->codeSDCFlag( pcCU, uiAbsPartIdx );
}

#endif
#if H_3D_DBBP
Void TEncEntropy::encodeDBBPFlag( TComDataCU* pcCU, UInt uiAbsPartIdx, Bool bRD )
{
  if( pcCU->getSlice()->getDepthBasedBlkPartFlag() && 
    ( pcCU->getPartitionSize(uiAbsPartIdx) == SIZE_2NxN || 
      pcCU->getPartitionSize(uiAbsPartIdx) == SIZE_Nx2N) && 
      pcCU->getWidth(uiAbsPartIdx) > 8 && 
      pcCU->getSlice()->getDefaultRefViewIdxAvailableFlag() )
  {
    if( bRD )
    {
      uiAbsPartIdx = 0;
    }
    m_pcEntropyCoderIf->codeDBBPFlag( pcCU, uiAbsPartIdx );
  }
}
#endif
//! \}
