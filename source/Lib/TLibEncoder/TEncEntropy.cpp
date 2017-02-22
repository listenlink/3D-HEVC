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
#include "TLibCommon/CommonDef.h"
#include "TLibCommon/TComSampleAdaptiveOffset.h"
#include "TLibCommon/TComTU.h"

#if ENVIRONMENT_VARIABLE_DEBUG_AND_TEST
#include "../TLibCommon/Debug.h"
static const Bool bDebugPredEnabled = DebugOptionList::DebugPred.getInt()!=0;
#endif

//! \ingroup TLibEncoder
//! \{

Void TEncEntropy::setEntropyCoder ( TEncEntropyIf* e )
{
  m_pcEntropyCoderIf = e;
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

Void TEncEntropy::encodePPS( const TComPPS* pcPPS )
{
  m_pcEntropyCoderIf->codePPS( pcPPS );
  return;
}

Void TEncEntropy::encodeSPS( const TComSPS* pcSPS )
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

Void TEncEntropy::encodeVPS( const TComVPS* pcVPS )
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
#if NH_3D_DIS
Void TEncEntropy::encodeDIS( TComDataCU* pcCU, UInt uiAbsPartIdx, Bool bRD )
{
  if( !pcCU->getSlice()->getDepthIntraSkipFlag() )
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
//! encode merge flag
Void TEncEntropy::encodeMergeFlag( TComDataCU* pcCU, UInt uiAbsPartIdx )
{
  // at least one merge candidate exists
  m_pcEntropyCoderIf->codeMergeFlag( pcCU, uiAbsPartIdx );
}

//! encode merge index
Void TEncEntropy::encodeMergeIndex( TComDataCU* pcCU, UInt uiAbsPartIdx, Bool bRD )
{
  if( bRD )
  {
    uiAbsPartIdx = 0;
    assert( pcCU->getPartitionSize(uiAbsPartIdx) == SIZE_2Nx2N );
  }
  m_pcEntropyCoderIf->codeMergeIndex( pcCU, uiAbsPartIdx );
}

#if NH_3D_IC
Void TEncEntropy::encodeICFlag( TComDataCU* pcCU, UInt uiAbsPartIdx, Bool bRD )
{
#if NH_3D_ARP
  if ( pcCU->isIntra( uiAbsPartIdx ) || ( pcCU->getSlice()->getViewIndex() == 0 ) || pcCU->getSlice()->getIsDepth() || pcCU->getARPW( uiAbsPartIdx ) > 0 )
#else
  if ( pcCU->isIntra( uiAbsPartIdx ) || ( pcCU->getSlice()->getViewIndex() == 0 ) || pcCU->getSlice()->getIsDepth() )
#endif
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

#if NH_3D_ARP
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


//! encode prediction mode
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

//! encode split flag
Void TEncEntropy::encodeSplitFlag( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth, Bool bRD )
{
  if( bRD )
  {
    uiAbsPartIdx = 0;
  }

  m_pcEntropyCoderIf->codeSplitFlag( pcCU, uiAbsPartIdx, uiDepth );
}

//! encode partition size
Void TEncEntropy::encodePartSize( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth, Bool bRD )
{
  if( bRD )
  {
    uiAbsPartIdx = 0;
  }

  m_pcEntropyCoderIf->codePartSize( pcCU, uiAbsPartIdx, uiDepth );
}


/** Encode I_PCM information.
 * \param pcCU          pointer to CU
 * \param uiAbsPartIdx  CU index
 * \param bRD           flag indicating estimation or encoding
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

Void TEncEntropy::xEncodeTransform( Bool& bCodeDQP, Bool& codeChromaQpAdj, TComTU &rTu )
{

//pcCU, absPartIdxCU, uiAbsPartIdx, uiDepth+1, uiTrIdx+1, quadrant,
  TComDataCU *pcCU=rTu.getCU();
  const UInt uiAbsPartIdx=rTu.GetAbsPartIdxTU();

  const UInt numValidComponent = pcCU->getPic()->getNumberValidComponents();
  const Bool bChroma = isChromaEnabled(pcCU->getPic()->getChromaFormat());
  const UInt uiTrIdx = rTu.GetTransformDepthRel();
  const UInt uiDepth = rTu.GetTransformDepthTotal();
#if NH_MV_ENC_DEC_TRAC
#if ENC_DEC_TRACE
  UInt uiLPelX   = pcCU->getCUPelX() + g_auiRasterToPelX[ g_auiZscanToRaster[uiAbsPartIdx] ];
  UInt uiTPelY   = pcCU->getCUPelY() + g_auiRasterToPelY[ g_auiZscanToRaster[uiAbsPartIdx] ];

  DTRACE_TU_S("=========== transform_tree ===========\n")
    DTRACE_TU("x0", uiLPelX)
    DTRACE_TU("x1", uiTPelY)
    DTRACE_TU("log2TrafoSize", pcCU->getSlice()->getSPS()->getMaxCUWidth()  >> uiDepth )
    DTRACE_TU("trafoDepth"  , uiDepth)
#endif
#endif

#if ENVIRONMENT_VARIABLE_DEBUG_AND_TEST
  const Bool bDebugRQT=pcCU->getSlice()->getFinalized() && DebugOptionList::DebugRQT.getInt()!=0;
  if (bDebugRQT)
  {
    printf("x..codeTransform: offsetLuma=%d offsetChroma=%d absPartIdx=%d, uiDepth=%d\n width=%d, height=%d, uiTrIdx=%d, uiInnerQuadIdx=%d\n",
           rTu.getCoefficientOffset(COMPONENT_Y), rTu.getCoefficientOffset(COMPONENT_Cb), uiAbsPartIdx, uiDepth, rTu.getRect(COMPONENT_Y).width, rTu.getRect(COMPONENT_Y).height, rTu.GetTransformDepthRel(), rTu.GetSectionNumber());
  }
#endif
  const UInt uiSubdiv = pcCU->getTransformIdx( uiAbsPartIdx ) > uiTrIdx;// + pcCU->getDepth( uiAbsPartIdx ) > uiDepth;
  const UInt uiLog2TrafoSize = rTu.GetLog2LumaTrSize();


  UInt cbf[MAX_NUM_COMPONENT] = {0,0,0};
  Bool bHaveACodedBlock       = false;
  Bool bHaveACodedChromaBlock = false;

  for(UInt ch=0; ch<numValidComponent; ch++)
  {
    const ComponentID compID = ComponentID(ch);

    cbf[compID] = pcCU->getCbf( uiAbsPartIdx, compID , uiTrIdx );
    
    if (cbf[ch] != 0)
    {
      bHaveACodedBlock = true;
      if (isChroma(compID))
      {
        bHaveACodedChromaBlock = true;
      }
    }
  }

  if( pcCU->isIntra(uiAbsPartIdx) && pcCU->getPartitionSize(uiAbsPartIdx) == SIZE_NxN && uiDepth == pcCU->getDepth(uiAbsPartIdx) )
  {
    assert( uiSubdiv );
  }
  else if( pcCU->isInter(uiAbsPartIdx) && (pcCU->getPartitionSize(uiAbsPartIdx) != SIZE_2Nx2N) && uiDepth == pcCU->getDepth(uiAbsPartIdx) &&  (pcCU->getSlice()->getSPS()->getQuadtreeTUMaxDepthInter() == 1) )
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

  for(UInt ch=COMPONENT_Cb; ch<numValidComponent; ch++)
  {
    const ComponentID compID=ComponentID(ch);
    if( bFirstCbfOfCU || rTu.ProcessingAllQuadrants(compID) )
    {
      if( bFirstCbfOfCU || pcCU->getCbf( uiAbsPartIdx, compID, uiTrDepthCurr - 1 ) )
      {
        m_pcEntropyCoderIf->codeQtCbf( rTu, compID, (uiSubdiv == 0) );
      }
    }
    else
    {
      assert( pcCU->getCbf( uiAbsPartIdx, compID, uiTrDepthCurr ) == pcCU->getCbf( uiAbsPartIdx, compID, uiTrDepthCurr - 1 ) );
    }
  }

  if( uiSubdiv )
  {
    TComTURecurse tuRecurseChild(rTu, true);
    do
    {
      xEncodeTransform( bCodeDQP, codeChromaQpAdj, tuRecurseChild );
    } while (tuRecurseChild.nextSection(rTu));
  }
  else
  {
#if !NH_MV_ENC_DEC_TRAC
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
    if( !pcCU->isIntra(uiAbsPartIdx) && uiDepth == pcCU->getDepth( uiAbsPartIdx ) && (!bChroma || (!pcCU->getCbf( uiAbsPartIdx, COMPONENT_Cb, 0 ) && !pcCU->getCbf( uiAbsPartIdx, COMPONENT_Cr, 0 ) ) ) )
    {
      assert( pcCU->getCbf( uiAbsPartIdx, COMPONENT_Y, 0 ) );
      //      printf( "saved one bin! " );
    }
    else
    {
      m_pcEntropyCoderIf->codeQtCbf( rTu, COMPONENT_Y, true ); //luma CBF is always at the lowest level
    }

    if ( bHaveACodedBlock )
    {
      // dQP: only for CTU once
      if ( pcCU->getSlice()->getPPS()->getUseDQP() )
      {
        if ( bCodeDQP )
        {
          encodeQP( pcCU, rTu.GetAbsPartIdxCU() );
          bCodeDQP = false;
        }
      }

      if ( pcCU->getSlice()->getUseChromaQpAdj() )
      {
        if ( bHaveACodedChromaBlock && codeChromaQpAdj && !pcCU->getCUTransquantBypass(rTu.GetAbsPartIdxCU()) )
        {
          encodeChromaQpAdjustment( pcCU, rTu.GetAbsPartIdxCU() );
          codeChromaQpAdj = false;
        }
      }

      const UInt numValidComp=pcCU->getPic()->getNumberValidComponents();

      for(UInt ch=COMPONENT_Y; ch<numValidComp; ch++)
      {
        const ComponentID compID=ComponentID(ch);

        if (rTu.ProcessComponentSection(compID))
        {
#if ENVIRONMENT_VARIABLE_DEBUG_AND_TEST
          if (bDebugRQT)
          {
            printf("Call NxN for chan %d width=%d height=%d cbf=%d\n", compID, rTu.getRect(compID).width, rTu.getRect(compID).height, 1);
          }
#endif

          if (rTu.getRect(compID).width != rTu.getRect(compID).height)
          {
            //code two sub-TUs
            TComTURecurse subTUIterator(rTu, false, TComTU::VERTICAL_SPLIT, true, compID);

            do
            {
              const UChar subTUCBF = pcCU->getCbf(subTUIterator.GetAbsPartIdxTU(compID), compID, (uiTrIdx + 1));

              if (subTUCBF != 0)
              {
#if ENVIRONMENT_VARIABLE_DEBUG_AND_TEST
                if (bDebugRQT)
                {
                  printf("Call NxN for chan %d width=%d height=%d cbf=%d\n", compID, subTUIterator.getRect(compID).width, subTUIterator.getRect(compID).height, 1);
                }
#endif
                m_pcEntropyCoderIf->codeCoeffNxN( subTUIterator, (pcCU->getCoeff(compID) + subTUIterator.getCoefficientOffset(compID)), compID );
              }
            }
            while (subTUIterator.nextSection(rTu));
          }
          else
          {
            if (isChroma(compID) && (cbf[COMPONENT_Y] != 0))
            {
              m_pcEntropyCoderIf->codeCrossComponentPrediction( rTu, compID );
            }

            if (cbf[compID] != 0)
            {
              m_pcEntropyCoderIf->codeCoeffNxN( rTu, (pcCU->getCoeff(compID) + rTu.getCoefficientOffset(compID)), compID );
            }
          }
        }
      }
    }
  }
}


//! encode intra direction for luma
Void TEncEntropy::encodeIntraDirModeLuma  ( TComDataCU* pcCU, UInt absPartIdx, Bool isMultiplePU )
{
  m_pcEntropyCoderIf->codeIntraDirLumaAng( pcCU, absPartIdx , isMultiplePU);
}


//! encode intra direction for chroma
Void TEncEntropy::encodeIntraDirModeChroma( TComDataCU* pcCU, UInt uiAbsPartIdx )
{
  m_pcEntropyCoderIf->codeIntraDirChroma( pcCU, uiAbsPartIdx );

#if ENVIRONMENT_VARIABLE_DEBUG_AND_TEST
  if (bDebugPredEnabled && pcCU->getSlice()->getFinalized())
  {
    UInt cdir=pcCU->getIntraDir(CHANNEL_TYPE_CHROMA, uiAbsPartIdx);
    if (cdir==36)
    {
      cdir=pcCU->getIntraDir(CHANNEL_TYPE_LUMA, uiAbsPartIdx);
    }
    printf("coding chroma Intra dir: %d, uiAbsPartIdx: %d, luma dir: %d\n", cdir, uiAbsPartIdx, pcCU->getIntraDir(CHANNEL_TYPE_LUMA, uiAbsPartIdx));
  }
#endif
}


Void TEncEntropy::encodePredInfo( TComDataCU* pcCU, UInt uiAbsPartIdx )
{
  if( pcCU->isIntra( uiAbsPartIdx ) )                                 // If it is Intra mode, encode intra prediction mode.
  {
    encodeIntraDirModeLuma  ( pcCU, uiAbsPartIdx,true );
    
    if (pcCU->getPic()->getChromaFormat()!=CHROMA_400)
    {
      encodeIntraDirModeChroma( pcCU, uiAbsPartIdx );

      if (enable4ChromaPUsInIntraNxNCU(pcCU->getPic()->getChromaFormat()) && pcCU->getPartitionSize( uiAbsPartIdx )==SIZE_NxN)
      {
        UInt uiPartOffset = ( pcCU->getPic()->getNumPartitionsInCtu() >> ( pcCU->getDepth(uiAbsPartIdx) << 1 ) ) >> 2;
        encodeIntraDirModeChroma( pcCU, uiAbsPartIdx + uiPartOffset   );
        encodeIntraDirModeChroma( pcCU, uiAbsPartIdx + uiPartOffset*2 );
        encodeIntraDirModeChroma( pcCU, uiAbsPartIdx + uiPartOffset*3 );
      }
    }
  }
  else                                                                // if it is Inter mode, encode motion vector and reference index
  {
    encodePUWise( pcCU, uiAbsPartIdx );
  }
}

Void TEncEntropy::encodeCrossComponentPrediction( TComTU &rTu, ComponentID compID )
{
  m_pcEntropyCoderIf->codeCrossComponentPrediction( rTu, compID );
}

//! encode motion information for every PU block
Void TEncEntropy::encodePUWise( TComDataCU* pcCU, UInt uiAbsPartIdx )
{
#if ENVIRONMENT_VARIABLE_DEBUG_AND_TEST
  const Bool bDebugPred = bDebugPredEnabled && pcCU->getSlice()->getFinalized();
#endif

  PartSize ePartSize = pcCU->getPartitionSize( uiAbsPartIdx );
  UInt uiNumPU = ( ePartSize == SIZE_2Nx2N ? 1 : ( ePartSize == SIZE_NxN ? 4 : 2 ) );
  UInt uiDepth = pcCU->getDepth( uiAbsPartIdx );
  UInt uiPUOffset = ( g_auiPUOffset[UInt( ePartSize )] << ( ( pcCU->getSlice()->getSPS()->getMaxTotalCUDepth() - uiDepth ) << 1 ) ) >> 4;

  for ( UInt uiPartIdx = 0, uiSubPartIdx = uiAbsPartIdx; uiPartIdx < uiNumPU; uiPartIdx++, uiSubPartIdx += uiPUOffset )
  {
#if NH_MV_ENC_DEC_TRAC
    DTRACE_PU_S("=========== prediction_unit ===========\n")
       //Todo: 
      //DTRACE_PU("x0", uiLPelX)
      //DTRACE_PU("x1", uiTPelY)
#endif
    encodeMergeFlag( pcCU, uiSubPartIdx );
    if ( pcCU->getMergeFlag( uiSubPartIdx ) )
    {
      encodeMergeIndex( pcCU, uiSubPartIdx );
#if ENVIRONMENT_VARIABLE_DEBUG_AND_TEST
      if (bDebugPred)
      {
        std::cout << "Coded merge flag, CU absPartIdx: " << uiAbsPartIdx << " PU(" << uiPartIdx << ") absPartIdx: " << uiSubPartIdx;
        std::cout << " merge index: " << (UInt)pcCU->getMergeIndex(uiSubPartIdx) << std::endl;
      }
#endif
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
#if ENVIRONMENT_VARIABLE_DEBUG_AND_TEST
          if (bDebugPred)
          {
            std::cout << "refListIdx: " << uiRefListIdx << std::endl;
            std::cout << "MVD horizontal: " << pcCU->getCUMvField(RefPicList(uiRefListIdx))->getMvd( uiAbsPartIdx ).getHor() << std::endl;
            std::cout << "MVD vertical:   " << pcCU->getCUMvField(RefPicList(uiRefListIdx))->getMvd( uiAbsPartIdx ).getVer() << std::endl;
            std::cout << "MVPIdxPU: " << pcCU->getMVPIdx(RefPicList( uiRefListIdx ), uiSubPartIdx) << std::endl;
            std::cout << "InterDir: " << (UInt)pcCU->getInterDir(uiSubPartIdx) << std::endl;
          }
#endif
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

//! encode reference frame index for a PU block
Void TEncEntropy::encodeRefFrmIdxPU( TComDataCU* pcCU, UInt uiAbsPartIdx, RefPicList eRefList )
{
  assert( pcCU->isInter( uiAbsPartIdx ) );

  if ( ( pcCU->getSlice()->getNumRefIdx( eRefList ) == 1 ) )
  {
    return;
  }

  if ( pcCU->getInterDir( uiAbsPartIdx ) & ( 1 << eRefList ) )
  {
    m_pcEntropyCoderIf->codeRefFrmIdx( pcCU, uiAbsPartIdx, eRefList );
  }

  return;
}

//! encode motion vector difference for a PU block
Void TEncEntropy::encodeMvdPU( TComDataCU* pcCU, UInt uiAbsPartIdx, RefPicList eRefList )
{
  assert( pcCU->isInter( uiAbsPartIdx ) );

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

Void TEncEntropy::encodeQtCbf( TComTU &rTu, const ComponentID compID, const Bool lowestLevel )
{
  m_pcEntropyCoderIf->codeQtCbf( rTu, compID, lowestLevel );
}

Void TEncEntropy::encodeTransformSubdivFlag( UInt uiSymbol, UInt uiCtx )
{
  m_pcEntropyCoderIf->codeTransformSubdivFlag( uiSymbol, uiCtx );
}

Void TEncEntropy::encodeQtRootCbf( TComDataCU* pcCU, UInt uiAbsPartIdx )
{
  m_pcEntropyCoderIf->codeQtRootCbf( pcCU, uiAbsPartIdx );
}

Void TEncEntropy::encodeQtCbfZero( TComTU &rTu, const ChannelType chType )
{
  m_pcEntropyCoderIf->codeQtCbfZero( rTu, chType );
}

Void TEncEntropy::encodeQtRootCbfZero( )
{
  m_pcEntropyCoderIf->codeQtRootCbfZero( );
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

//! encode chroma qp adjustment
Void TEncEntropy::encodeChromaQpAdjustment( TComDataCU* cu, UInt absPartIdx, Bool inRd )
{
  if( inRd )
  {
    absPartIdx = 0;
  }

  m_pcEntropyCoderIf->codeChromaQpAdjustment( cu, absPartIdx );
}

// texture

//! encode coefficients
Void TEncEntropy::encodeCoeff( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth, Bool& bCodeDQP, Bool& codeChromaQpAdj )
{

#if ENVIRONMENT_VARIABLE_DEBUG_AND_TEST
  const Bool bDebugRQT=pcCU->getSlice()->getFinalized() && DebugOptionList::DebugRQT.getInt()!=0;
#endif

#if NH_3D_SDC_INTRA
  if( pcCU->getSDCFlag( uiAbsPartIdx ) && pcCU->isIntra( uiAbsPartIdx ) )
  {
    assert( pcCU->getPartitionSize(uiAbsPartIdx) == SIZE_2Nx2N );
    assert( pcCU->getTransformIdx(uiAbsPartIdx) == 0 );
    assert( pcCU->getCbf(uiAbsPartIdx, COMPONENT_Y) == 1 );
  }
#endif
#if NH_3D_SDC_INTER
  if( pcCU->getSDCFlag( uiAbsPartIdx ) && !pcCU->isIntra( uiAbsPartIdx ) )
  {
    assert( !pcCU->isSkipped( uiAbsPartIdx ) );
    assert( !pcCU->isIntra( uiAbsPartIdx) );
    assert( pcCU->getSlice()->getIsDepth() );
  }
#endif
#if NH_3D
  if( pcCU->getSlice()->getIsDepth() )
  {
#if NH_3D_SDC_INTRA || NH_3D_SDC_INTER
    if( pcCU->getSDCFlag( uiAbsPartIdx ) )
    {
      m_pcEntropyCoderIf->codeDeltaDC( pcCU, uiAbsPartIdx );
      return;
    }
#endif
#if NH_3D_DMM
    if( pcCU->isIntra(uiAbsPartIdx) )
  {
    Int iPartNum = ( pcCU->isIntra( uiAbsPartIdx ) && pcCU->getPartitionSize( uiAbsPartIdx ) == SIZE_NxN ) ? 4 : 1;
      UInt uiPartOffset = ( pcCU->getPic()->getNumPartitionsInCtu() >> ( pcCU->getDepth( uiAbsPartIdx ) << 1 ) ) >> 2;
      for( Int iPart = 0; iPart < iPartNum; iPart++ )
      {
        if( isDmmMode( pcCU->getIntraDir( CHANNEL_TYPE_LUMA, uiAbsPartIdx + uiPartOffset*iPart ) ) ) 
        {
          m_pcEntropyCoderIf->codeDeltaDC( pcCU, uiAbsPartIdx + uiPartOffset*iPart );
        }
      }
    }
#endif
  }
#endif

  if( pcCU->isIntra(uiAbsPartIdx) )
  {
#if !NH_MV
    if (false)
    {
      DTRACE_CABAC_VL( g_nSymbolCounter++ )
      DTRACE_CABAC_T( "\tdecodeTransformIdx()\tCUDepth=" )
      DTRACE_CABAC_V( uiDepth )
      DTRACE_CABAC_T( "\n" )
    }
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

  TComTURecurse tuRecurse(pcCU, uiAbsPartIdx, uiDepth);
#if ENVIRONMENT_VARIABLE_DEBUG_AND_TEST
  if (bDebugRQT)
  {
    printf("..codeCoeff: uiAbsPartIdx=%d, PU format=%d, 2Nx2N=%d, NxN=%d\n", uiAbsPartIdx, pcCU->getPartitionSize(uiAbsPartIdx), SIZE_2Nx2N, SIZE_NxN);
  }
#endif

  xEncodeTransform( bCodeDQP, codeChromaQpAdj, tuRecurse );
}

Void TEncEntropy::encodeCoeffNxN( TComTU &rTu, TCoeff* pcCoef, const ComponentID compID)
{
#if ENC_DEC_TRACE && NH_MV_ENC_DEC_TRAC
  Bool oldTraceFracBits = g_traceEncFracBits; 
  g_traceEncFracBits = false; 
#endif

  TComDataCU *pcCU = rTu.getCU();

  if (pcCU->getCbf(rTu.GetAbsPartIdxTU(), compID, rTu.GetTransformDepthRel()) != 0)
  {
    if (rTu.getRect(compID).width != rTu.getRect(compID).height)
    {
      //code two sub-TUs
      TComTURecurse subTUIterator(rTu, false, TComTU::VERTICAL_SPLIT, true, compID);

      const UInt subTUSize = subTUIterator.getRect(compID).width * subTUIterator.getRect(compID).height;

      do
      {
        const UChar subTUCBF = pcCU->getCbf(subTUIterator.GetAbsPartIdxTU(compID), compID, (subTUIterator.GetTransformDepthRel() + 1));

        if (subTUCBF != 0)
        {
          m_pcEntropyCoderIf->codeCoeffNxN( subTUIterator, (pcCoef + (subTUIterator.GetSectionNumber() * subTUSize)), compID);
        }
      }
      while (subTUIterator.nextSection(rTu));
    }
    else
    {
      m_pcEntropyCoderIf->codeCoeffNxN(rTu, pcCoef, compID);
    }
  }
#if ENC_DEC_TRACE && NH_MV_ENC_DEC_TRAC
  g_traceEncFracBits = oldTraceFracBits; 
#endif
}

Void TEncEntropy::estimateBit (estBitsSbacStruct* pcEstBitsSbac, Int width, Int height, const ChannelType chType)
{
  const UInt heightAtEntropyCoding = (width != height) ? (height >> 1) : height;

  m_pcEntropyCoderIf->estBit ( pcEstBitsSbac, width, heightAtEntropyCoding, chType );
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

#if NH_3D_DMM || NH_3D_SDC_INTRA || NH_3D_SDC_INTER
Void TEncEntropy::encodeDeltaDC  ( TComDataCU* pcCU, UInt absPartIdx )
{
  m_pcEntropyCoderIf->codeDeltaDC( pcCU, absPartIdx );
}
#endif
#if NH_3D_SDC_INTRA || NH_3D_SDC_INTER
Void TEncEntropy::encodeSDCFlag( TComDataCU* pcCU, UInt uiAbsPartIdx, Bool bRD )
{
  if( bRD )
  {
    uiAbsPartIdx = 0;
  }
  
  if( ( !pcCU->isIntra( uiAbsPartIdx ) && !pcCU->getSlice()->getInterSdcFlag() ) || 
    ( pcCU->isIntra( uiAbsPartIdx ) && !pcCU->getSlice()->getIntraSdcWedgeFlag() ) )
  {
    return;
  }

  if( !pcCU->getSlice()->getIsDepth() || pcCU->getPartitionSize( uiAbsPartIdx ) != SIZE_2Nx2N || pcCU->isSkipped( uiAbsPartIdx ) )
  {
    return;
  }

  assert( pcCU->getPartitionSize( uiAbsPartIdx ) == SIZE_2Nx2N );

  m_pcEntropyCoderIf->codeSDCFlag( pcCU, uiAbsPartIdx );
}

#endif
#if NH_3D_DBBP
Void TEncEntropy::encodeDBBPFlag( TComDataCU* pcCU, UInt uiAbsPartIdx, Bool bRD )
{
  if( bRD )
  {
    uiAbsPartIdx = 0;
  }
  
  if( pcCU->getSlice()->getDepthBasedBlkPartFlag() && 
    ( pcCU->getPartitionSize(uiAbsPartIdx) == SIZE_2NxN || 
      pcCU->getPartitionSize(uiAbsPartIdx) == SIZE_Nx2N) && 
      pcCU->getWidth(uiAbsPartIdx) > 8 && 
      pcCU->getSlice()->getDefaultRefViewIdxAvailableFlag() )
  {
    m_pcEntropyCoderIf->codeDBBPFlag( pcCU, uiAbsPartIdx );
  }
}
#endif

//! \}
