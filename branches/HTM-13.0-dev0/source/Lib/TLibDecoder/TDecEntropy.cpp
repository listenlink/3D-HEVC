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

/** \file     TDecEntropy.cpp
    \brief    entropy decoder class
*/

#include "TDecEntropy.h"

//! \ingroup TLibDecoder
//! \{

Void TDecEntropy::setEntropyDecoder         ( TDecEntropyIf* p )
{
  m_pcEntropyDecoderIf = p;
}

#include "TLibCommon/TComSampleAdaptiveOffset.h"

Void TDecEntropy::decodeSkipFlag( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth )
{
  m_pcEntropyDecoderIf->parseSkipFlag( pcCU, uiAbsPartIdx, uiDepth );
}
#if H_3D_SINGLE_DEPTH
Void TDecEntropy::decodeSingleDepthMode( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth )
{
  if( !pcCU->getSlice()->getIntraSingleFlag() )
  {
    return;
  }  
#if ALIGN_J0060_J0107
#else
  if ( !pcCU->getSlice()->getIsDepth() )
  {
    return;
  }
#if MTK_SINGLE_DEPTH_VPS_FLAG_J0060
  if(!pcCU->getSlice()->getVPS()->getSingleDepthModeFlag(pcCU->getSlice()->getLayerIdInVps()))
  {
     return;
  }
#else
  if(!pcCU->getSlice()->getApplySingleDepthMode())
  {
     return;
  }
#endif
#endif

  m_pcEntropyDecoderIf->parseSingleDepthMode( pcCU, uiAbsPartIdx, uiDepth );
}
#endif
Void TDecEntropy::decodeCUTransquantBypassFlag(TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth )
{
  m_pcEntropyDecoderIf->parseCUTransquantBypassFlag( pcCU, uiAbsPartIdx, uiDepth );
}

/** decode merge flag
 * \param pcSubCU
 * \param uiAbsPartIdx 
 * \param uiDepth
 * \param uiPUIdx 
 * \returns Void
 */
Void TDecEntropy::decodeMergeFlag( TComDataCU* pcSubCU, UInt uiAbsPartIdx, UInt uiDepth, UInt uiPUIdx )
{ 
  // at least one merge candidate exists
  m_pcEntropyDecoderIf->parseMergeFlag( pcSubCU, uiAbsPartIdx, uiDepth, uiPUIdx );
}

/** decode merge index
 * \param pcCU
 * \param uiPartIdx 
 * \param uiAbsPartIdx 
 * \param puhInterDirNeighbours pointer to list of inter direction from the casual neighbours
 * \param pcMvFieldNeighbours pointer to list of motion vector field from the casual neighbours
 * \param uiDepth
 * \returns Void
 */
Void TDecEntropy::decodeMergeIndex( TComDataCU* pcCU, UInt uiPartIdx, UInt uiAbsPartIdx, UInt uiDepth )
{
  UInt uiMergeIndex = 0;
  m_pcEntropyDecoderIf->parseMergeIndex( pcCU, uiMergeIndex );
  pcCU->setMergeIndexSubParts( uiMergeIndex, uiAbsPartIdx, uiPartIdx, uiDepth );
}

#if H_3D_ARP
Void TDecEntropy::decodeARPW( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth )
{
  if( !pcCU->getSlice()->getARPStepNum() || pcCU->isIntra( uiAbsPartIdx ) )
  {
    return;
  }

  if( pcCU->getPartitionSize(uiAbsPartIdx) != SIZE_2Nx2N )
  {
    pcCU->setARPWSubParts( 0 , uiAbsPartIdx, uiDepth );
  }
  else
  {
    m_pcEntropyDecoderIf->parseARPW( pcCU , uiAbsPartIdx , uiDepth );
  }
}
#endif

#if H_3D_IC
Void TDecEntropy::decodeICFlag( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth )
{
  pcCU->setICFlagSubParts( false , uiAbsPartIdx, 0, uiDepth );

  if ( pcCU->isIntra( uiAbsPartIdx ) || ( pcCU->getSlice()->getViewIndex() == 0 ) || pcCU->getSlice()->getIsDepth() || pcCU->getARPW( uiAbsPartIdx ) > 0 )
  {
    return;
  }

  if( !pcCU->getSlice()->getApplyIC() )
    return;

  if( pcCU->isICFlagRequired( uiAbsPartIdx ) )
    m_pcEntropyDecoderIf->parseICFlag( pcCU, uiAbsPartIdx, uiDepth );
}
#endif

Void TDecEntropy::decodeSplitFlag   ( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth )
{
  m_pcEntropyDecoderIf->parseSplitFlag( pcCU, uiAbsPartIdx, uiDepth );
}

Void TDecEntropy::decodePredMode( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth )
{
  m_pcEntropyDecoderIf->parsePredMode( pcCU, uiAbsPartIdx, uiDepth );
}

Void TDecEntropy::decodePartSize( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth )
{
  m_pcEntropyDecoderIf->parsePartSize( pcCU, uiAbsPartIdx, uiDepth );
  
#if H_3D_DBBP
#if SEC_DBBP_VIEW_REF_CHECK_J0037
  #if HHI_TOOL_PARAMETERS_I2_J0107
if( pcCU->getSlice()->getDepthBasedBlkPartFlag() && (pcCU->getPartitionSize(uiAbsPartIdx) == SIZE_2NxN || pcCU->getPartitionSize(uiAbsPartIdx) == SIZE_Nx2N) && pcCU->getWidth(uiAbsPartIdx) > 8 && pcCU->getSlice()->getDefaultRefViewIdxAvailableFlag() )
#else
if( pcCU->getSlice()->getVPS()->getUseDBBP(pcCU->getSlice()->getLayerIdInVps()) && (pcCU->getPartitionSize(uiAbsPartIdx) == SIZE_2NxN || pcCU->getPartitionSize(uiAbsPartIdx) == SIZE_Nx2N) && pcCU->getWidth(uiAbsPartIdx) > 8 && pcCU->getSlice()->getDefaultRefViewIdxAvailableFlag() )
#endif
#else
  if( pcCU->getSlice()->getVPS()->getUseDBBP(pcCU->getSlice()->getLayerIdInVps()) && (pcCU->getPartitionSize(uiAbsPartIdx) == SIZE_2NxN || pcCU->getPartitionSize(uiAbsPartIdx) == SIZE_Nx2N) && pcCU->getWidth(uiAbsPartIdx) > 8 )
#endif
  {
    decodeDBBPFlag(pcCU, uiAbsPartIdx, uiDepth);
  }
#endif
}

Void TDecEntropy::decodePredInfo    ( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth, TComDataCU* pcSubCU )
{
  if( pcCU->isIntra( uiAbsPartIdx ) )                                 // If it is Intra mode, encode intra prediction mode.
  {
    decodeIntraDirModeLuma  ( pcCU, uiAbsPartIdx, uiDepth );
#if H_3D_DIM_SDC
    if(!pcCU->getSDCFlag(uiAbsPartIdx))
#endif
    decodeIntraDirModeChroma( pcCU, uiAbsPartIdx, uiDepth );
  }
  else                                                                // if it is Inter mode, encode motion vector and reference index
  {
    decodePUWise( pcCU, uiAbsPartIdx, uiDepth, pcSubCU );
  }
}

/** Parse I_PCM information. 
 * \param pcCU  pointer to CUpointer to CU
 * \param uiAbsPartIdx CU index
 * \param uiDepth CU depth
 * \returns Void
 */
Void TDecEntropy::decodeIPCMInfo( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth )
{
  if(!pcCU->getSlice()->getSPS()->getUsePCM()
    || pcCU->getWidth(uiAbsPartIdx) > (1<<pcCU->getSlice()->getSPS()->getPCMLog2MaxSize())
    || pcCU->getWidth(uiAbsPartIdx) < (1<<pcCU->getSlice()->getSPS()->getPCMLog2MinSize()) )
  {
    return;
  }
  
  m_pcEntropyDecoderIf->parseIPCMInfo( pcCU, uiAbsPartIdx, uiDepth );
}

Void TDecEntropy::decodeIntraDirModeLuma  ( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth )
{
  m_pcEntropyDecoderIf->parseIntraDirLumaAng( pcCU, uiAbsPartIdx, uiDepth );
}

Void TDecEntropy::decodeIntraDirModeChroma( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth )
{
  m_pcEntropyDecoderIf->parseIntraDirChroma( pcCU, uiAbsPartIdx, uiDepth );
}

/** decode motion information for every PU block.
 * \param pcCU
 * \param uiAbsPartIdx 
 * \param uiDepth
 * \param pcSubCU
 * \returns Void
 */
Void TDecEntropy::decodePUWise( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth, TComDataCU* pcSubCU )
{
  PartSize ePartSize = pcCU->getPartitionSize( uiAbsPartIdx );
  UInt uiNumPU = ( ePartSize == SIZE_2Nx2N ? 1 : ( ePartSize == SIZE_NxN ? 4 : 2 ) );
  UInt uiPUOffset = ( g_auiPUOffset[UInt( ePartSize )] << ( ( pcCU->getSlice()->getSPS()->getMaxCUDepth() - uiDepth ) << 1 ) ) >> 4;

#if H_3D_IV_MERGE
  TComMvField cMvFieldNeighbours[MRG_MAX_NUM_CANDS_MEM << 1]; // double length for mv of both lists
  UChar uhInterDirNeighbours[MRG_MAX_NUM_CANDS_MEM];
#else
  TComMvField cMvFieldNeighbours[MRG_MAX_NUM_CANDS << 1]; // double length for mv of both lists
  UChar uhInterDirNeighbours[MRG_MAX_NUM_CANDS];
#endif
#if H_3D_SPIVMP
  Bool bSPIVMPFlag[MRG_MAX_NUM_CANDS_MEM];
  TComMvField*  pcMvFieldSP;
  UChar* puhInterDirSP;
  pcMvFieldSP = new TComMvField[pcCU->getPic()->getPicSym()->getNumPartition()*2]; 
  puhInterDirSP = new UChar[pcCU->getPic()->getPicSym()->getNumPartition()]; 
#endif
  for ( UInt ui = 0; ui < pcCU->getSlice()->getMaxNumMergeCand(); ui++ )
  {
    uhInterDirNeighbours[ui] = 0;
  }
  Int numValidMergeCand = 0;
  Bool isMerged = false;

  pcSubCU->copyInterPredInfoFrom( pcCU, uiAbsPartIdx, REF_PIC_LIST_0 );
  pcSubCU->copyInterPredInfoFrom( pcCU, uiAbsPartIdx, REF_PIC_LIST_1 );
#if H_3D_IV_MERGE
  pcSubCU->copyDVInfoFrom( pcCU, uiAbsPartIdx);
#endif
  for ( UInt uiPartIdx = 0, uiSubPartIdx = uiAbsPartIdx; uiPartIdx < uiNumPU; uiPartIdx++, uiSubPartIdx += uiPUOffset )
  {
#if H_MV_ENC_DEC_TRAC
    DTRACE_PU_S("=========== prediction_unit ===========\n")
    // ToDo: 
    //DTRACE_PU("x0", uiLPelX)
    //DTRACE_PU("x1", uiTPelY)
#endif
    decodeMergeFlag( pcCU, uiSubPartIdx, uiDepth, uiPartIdx );
    if ( pcCU->getMergeFlag( uiSubPartIdx ) )
    {
      decodeMergeIndex( pcCU, uiPartIdx, uiSubPartIdx, uiDepth );
      UInt uiMergeIndex = pcCU->getMergeIndex(uiSubPartIdx);
#if H_3D_ARP
      decodeARPW  ( pcCU, uiAbsPartIdx, uiDepth );
#endif
#if H_3D_IC
      decodeICFlag( pcCU, uiAbsPartIdx, uiDepth );
#endif
#if H_3D_DBBP
      if ( pcCU->getSlice()->getPPS()->getLog2ParallelMergeLevelMinus2() && ePartSize != SIZE_2Nx2N && pcSubCU->getWidth( 0 ) <= 8 && pcCU->getDBBPFlag(uiAbsPartIdx) == false )
#else
      if ( pcCU->getSlice()->getPPS()->getLog2ParallelMergeLevelMinus2() && ePartSize != SIZE_2Nx2N && pcSubCU->getWidth( 0 ) <= 8 ) 
#endif
      {
        pcSubCU->setPartSizeSubParts( SIZE_2Nx2N, 0, uiDepth );
        if ( !isMerged )
        {
#if H_3D_VSP
          Int vspFlag[MRG_MAX_NUM_CANDS_MEM];
          memset(vspFlag, 0, sizeof(Int)*MRG_MAX_NUM_CANDS_MEM);
#if H_3D_SPIVMP
          memset(bSPIVMPFlag, false, sizeof(Bool)*MRG_MAX_NUM_CANDS_MEM);
#endif
          pcSubCU->initAvailableFlags();
          pcSubCU->getInterMergeCandidates( 0, 0, cMvFieldNeighbours, uhInterDirNeighbours, numValidMergeCand);
          pcSubCU->xGetInterMergeCandidates( 0, 0, cMvFieldNeighbours, uhInterDirNeighbours
#if H_3D_SPIVMP
            , pcMvFieldSP, puhInterDirSP
#endif
            , numValidMergeCand );
          pcSubCU->buildMCL( cMvFieldNeighbours, uhInterDirNeighbours, vspFlag
#if H_3D_SPIVMP
            , bSPIVMPFlag
#endif
            , numValidMergeCand );
          pcCU->setVSPFlagSubParts( vspFlag[uiMergeIndex], uiSubPartIdx, uiPartIdx, uiDepth );

#else
#if H_3D
          pcSubCU->initAvailableFlags();
          pcSubCU->getInterMergeCandidates( 0, 0, cMvFieldNeighbours, uhInterDirNeighbours, numValidMergeCand);
          pcSubCU->xGetInterMergeCandidates( 0, 0, cMvFieldNeighbours, uhInterDirNeighbours, numValidMergeCand );

#else
          pcSubCU->getInterMergeCandidates( 0, 0, cMvFieldNeighbours, uhInterDirNeighbours, numValidMergeCand );
#endif
#endif
          isMerged = true;
        }
        pcSubCU->setPartSizeSubParts( ePartSize, 0, uiDepth );
      }
      else
      {
        uiMergeIndex = pcCU->getMergeIndex(uiSubPartIdx);
#if H_3D_VSP
        Int vspFlag[MRG_MAX_NUM_CANDS_MEM];
        memset(vspFlag, 0, sizeof(Int)*MRG_MAX_NUM_CANDS_MEM);
#if H_3D_SPIVMP
        memset(bSPIVMPFlag, false, sizeof(Bool)*MRG_MAX_NUM_CANDS_MEM);
#endif
        pcSubCU->initAvailableFlags();
        pcSubCU->getInterMergeCandidates( uiSubPartIdx-uiAbsPartIdx, uiPartIdx, cMvFieldNeighbours, uhInterDirNeighbours, numValidMergeCand, uiMergeIndex );
        pcSubCU->xGetInterMergeCandidates( uiSubPartIdx-uiAbsPartIdx, uiPartIdx, cMvFieldNeighbours, uhInterDirNeighbours
#if H_3D_SPIVMP
          , pcMvFieldSP, puhInterDirSP
#endif
          ,numValidMergeCand, uiMergeIndex );
        pcSubCU->buildMCL( cMvFieldNeighbours, uhInterDirNeighbours, vspFlag
#if H_3D_SPIVMP
          , bSPIVMPFlag
#endif
          ,numValidMergeCand );
        pcCU->setVSPFlagSubParts( vspFlag[uiMergeIndex], uiSubPartIdx, uiPartIdx, uiDepth );
#else
#if H_3D
        pcSubCU->initAvailableFlags();
        pcSubCU->getInterMergeCandidates( uiSubPartIdx-uiAbsPartIdx, uiPartIdx, cMvFieldNeighbours, uhInterDirNeighbours, numValidMergeCand, uiMergeIndex );
        pcSubCU->xGetInterMergeCandidates( uiSubPartIdx-uiAbsPartIdx, uiPartIdx, cMvFieldNeighbours, uhInterDirNeighbours, numValidMergeCand, uiMergeIndex );
#else
        pcSubCU->getInterMergeCandidates( uiSubPartIdx-uiAbsPartIdx, uiPartIdx, cMvFieldNeighbours, uhInterDirNeighbours, numValidMergeCand, uiMergeIndex );
#endif
#endif
      }
      pcCU->setInterDirSubParts( uhInterDirNeighbours[uiMergeIndex], uiSubPartIdx, uiPartIdx, uiDepth );
#if !LGE_DDD_REMOVAL_J0042_J0030
#if H_3D_DDD
      if( uiMergeIndex == pcSubCU->getUseDDDCandIdx() )
      {
          assert( pcCU->getSlice()->getViewIndex() != 0 );
          pcCU->setUseDDD( true, uiSubPartIdx, uiPartIdx, uiDepth );
          pcCU->setDDDepthSubParts( pcSubCU->getDDTmpDepth(),uiSubPartIdx, uiPartIdx, uiDepth );
      }
      else
      {
          pcCU->setUseDDD( false, uiSubPartIdx, uiPartIdx, uiDepth );
      }
#endif
#endif

      TComMv cTmpMv( 0, 0 );
      for ( UInt uiRefListIdx = 0; uiRefListIdx < 2; uiRefListIdx++ )
      {        
        if ( pcCU->getSlice()->getNumRefIdx( RefPicList( uiRefListIdx ) ) > 0 )
        {
          pcCU->setMVPIdxSubParts( 0, RefPicList( uiRefListIdx ), uiSubPartIdx, uiPartIdx, uiDepth);
          pcCU->setMVPNumSubParts( 0, RefPicList( uiRefListIdx ), uiSubPartIdx, uiPartIdx, uiDepth);
          pcCU->getCUMvField( RefPicList( uiRefListIdx ) )->setAllMvd( cTmpMv, ePartSize, uiSubPartIdx, uiDepth, uiPartIdx );
          pcCU->getCUMvField( RefPicList( uiRefListIdx ) )->setAllMvField( cMvFieldNeighbours[ 2*uiMergeIndex + uiRefListIdx ], ePartSize, uiSubPartIdx, uiDepth, uiPartIdx );
#if H_3D_VSP
#if H_3D_DBBP
          if( pcCU->getVSPFlag( uiSubPartIdx ) != 0 && !pcCU->getDBBPFlag( uiAbsPartIdx ) )
#else
          if( pcCU->getVSPFlag( uiSubPartIdx ) != 0 )
#endif
          {
            if ( uhInterDirNeighbours[ uiMergeIndex ] & (1<<uiRefListIdx) )
            {
              UInt dummy;
              Int vspSize;
              Int width, height;
              pcCU->getPartIndexAndSize( uiPartIdx, dummy, width, height, uiSubPartIdx, pcCU->getTotalNumPart()==256 );
              pcCU->setMvFieldPUForVSP( pcCU, uiSubPartIdx, width, height, RefPicList( uiRefListIdx ), cMvFieldNeighbours[ 2*uiMergeIndex + uiRefListIdx ].getRefIdx(), vspSize );
              pcCU->setVSPFlag( uiSubPartIdx, vspSize );
            }
          }
#endif
        }
      }
#if H_3D_SPIVMP
      pcCU->setSPIVMPFlagSubParts(bSPIVMPFlag[uiMergeIndex], uiSubPartIdx, uiPartIdx, uiDepth );  
      if (bSPIVMPFlag[uiMergeIndex] != 0)
      {
        Int iWidth, iHeight;
        UInt uiIdx;
        pcCU->getPartIndexAndSize( uiPartIdx, uiIdx, iWidth, iHeight, uiSubPartIdx, true );

        UInt uiSPAddr;

        Int iNumSPInOneLine, iNumSP, iSPWidth, iSPHeight;

        pcCU->getSPPara(iWidth, iHeight, iNumSP, iNumSPInOneLine, iSPWidth, iSPHeight);

        for (Int iPartitionIdx = 0; iPartitionIdx < iNumSP; iPartitionIdx++)
        {
          pcCU->getSPAbsPartIdx(uiSubPartIdx, iSPWidth, iSPHeight, iPartitionIdx, iNumSPInOneLine, uiSPAddr);
          pcCU->setInterDirSP(puhInterDirSP[iPartitionIdx], uiSPAddr, iSPWidth, iSPHeight);
          pcCU->getCUMvField( REF_PIC_LIST_0 )->setMvFieldSP(pcCU, uiSPAddr, pcMvFieldSP[2*iPartitionIdx], iSPWidth, iSPHeight);
          pcCU->getCUMvField( REF_PIC_LIST_1 )->setMvFieldSP(pcCU, uiSPAddr, pcMvFieldSP[2*iPartitionIdx + 1], iSPWidth, iSPHeight);
        }
      }
#endif
    }
    else
    {
      decodeInterDirPU( pcCU, uiSubPartIdx, uiDepth, uiPartIdx );
      for ( UInt uiRefListIdx = 0; uiRefListIdx < 2; uiRefListIdx++ )
      {        
        if ( pcCU->getSlice()->getNumRefIdx( RefPicList( uiRefListIdx ) ) > 0 )
        {
          decodeRefFrmIdxPU( pcCU,    uiSubPartIdx,              uiDepth, uiPartIdx, RefPicList( uiRefListIdx ) );
          decodeMvdPU      ( pcCU,    uiSubPartIdx,              uiDepth, uiPartIdx, RefPicList( uiRefListIdx ) );
          decodeMVPIdxPU   ( pcSubCU, uiSubPartIdx-uiAbsPartIdx, uiDepth, uiPartIdx, RefPicList( uiRefListIdx ) );
        }
      }
#if H_3D_ARP
      decodeARPW  ( pcCU, uiAbsPartIdx, uiDepth );
#endif
#if H_3D_IC
      decodeICFlag( pcCU, uiAbsPartIdx, uiDepth );
#endif
    }
    if ( (pcCU->getInterDir(uiSubPartIdx) == 3) && pcSubCU->isBipredRestriction(uiPartIdx) )
    {
      pcCU->getCUMvField( REF_PIC_LIST_1 )->setAllMv( TComMv(0,0), ePartSize, uiSubPartIdx, uiDepth, uiPartIdx);
      pcCU->getCUMvField( REF_PIC_LIST_1 )->setAllRefIdx( -1, ePartSize, uiSubPartIdx, uiDepth, uiPartIdx);
      pcCU->setInterDirSubParts( 1, uiSubPartIdx, uiPartIdx, uiDepth);
    }
  }
#if H_3D_SPIVMP
  delete[] pcMvFieldSP;
  delete[] puhInterDirSP;
#endif
  return;
}

/** decode inter direction for a PU block
 * \param pcCU
 * \param uiAbsPartIdx 
 * \param uiDepth
 * \param uiPartIdx 
 * \returns Void
 */
Void TDecEntropy::decodeInterDirPU( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth, UInt uiPartIdx )
{
  UInt uiInterDir;

  if ( pcCU->getSlice()->isInterP() )
  {
    uiInterDir = 1;
  }
  else
  {
    m_pcEntropyDecoderIf->parseInterDir( pcCU, uiInterDir, uiAbsPartIdx );
  }

  pcCU->setInterDirSubParts( uiInterDir, uiAbsPartIdx, uiPartIdx, uiDepth );
}

Void TDecEntropy::decodeRefFrmIdxPU( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth, UInt uiPartIdx, RefPicList eRefList )
{
  Int iRefFrmIdx = 0;
  Int iParseRefFrmIdx = pcCU->getInterDir( uiAbsPartIdx ) & ( 1 << eRefList );

  if ( pcCU->getSlice()->getNumRefIdx( eRefList ) > 1 && iParseRefFrmIdx )
  {
    m_pcEntropyDecoderIf->parseRefFrmIdx( pcCU, iRefFrmIdx, eRefList );
  }
  else if ( !iParseRefFrmIdx )
  {
    iRefFrmIdx = NOT_VALID;
  }
  else
  {
    iRefFrmIdx = 0;
  }

  PartSize ePartSize = pcCU->getPartitionSize( uiAbsPartIdx );
  pcCU->getCUMvField( eRefList )->setAllRefIdx( iRefFrmIdx, ePartSize, uiAbsPartIdx, uiDepth, uiPartIdx );
}

/** decode motion vector difference for a PU block
 * \param pcCU
 * \param uiAbsPartIdx 
 * \param uiDepth
 * \param uiPartIdx
 * \param eRefList 
 * \returns Void
 */
Void TDecEntropy::decodeMvdPU( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth, UInt uiPartIdx, RefPicList eRefList )
{
  if ( pcCU->getInterDir( uiAbsPartIdx ) & ( 1 << eRefList ) )
  {
    m_pcEntropyDecoderIf->parseMvd( pcCU, uiAbsPartIdx, uiPartIdx, uiDepth, eRefList );
  }
}

Void TDecEntropy::decodeMVPIdxPU( TComDataCU* pcSubCU, UInt uiPartAddr, UInt uiDepth, UInt uiPartIdx, RefPicList eRefList )
{
  Int iMVPIdx = -1;

  TComMv cZeroMv( 0, 0 );
  TComMv cMv     = cZeroMv;
  Int    iRefIdx = -1;

  TComCUMvField* pcSubCUMvField = pcSubCU->getCUMvField( eRefList );
  AMVPInfo* pAMVPInfo = pcSubCUMvField->getAMVPInfo();

  iRefIdx = pcSubCUMvField->getRefIdx(uiPartAddr);
  cMv = cZeroMv;

  if ( (pcSubCU->getInterDir(uiPartAddr) & ( 1 << eRefList )) )
  {
    m_pcEntropyDecoderIf->parseMVPIdx( iMVPIdx );
#if H_MV_ENC_DEC_TRAC
#if ENC_DEC_TRACE
    if ( eRefList == REF_PIC_LIST_0 )
    {
      DTRACE_PU("mvp_l0_flag", iMVPIdx)
    }
    else
    {
      DTRACE_PU("mvp_l1_flag", iMVPIdx)
    }
#endif
#endif
  }
  pcSubCU->fillMvpCand(uiPartIdx, uiPartAddr, eRefList, iRefIdx, pAMVPInfo);
  pcSubCU->setMVPNumSubParts(pAMVPInfo->iN, eRefList, uiPartAddr, uiPartIdx, uiDepth);
  pcSubCU->setMVPIdxSubParts( iMVPIdx, eRefList, uiPartAddr, uiPartIdx, uiDepth );
  if ( iRefIdx >= 0 )
  {
    m_pcPrediction->getMvPredAMVP( pcSubCU, uiPartIdx, uiPartAddr, eRefList, cMv);
    cMv += pcSubCUMvField->getMvd( uiPartAddr );
  }

  PartSize ePartSize = pcSubCU->getPartitionSize( uiPartAddr );
  pcSubCU->getCUMvField( eRefList )->setAllMv(cMv, ePartSize, uiPartAddr, 0, uiPartIdx);
}

Void TDecEntropy::xDecodeTransform( TComDataCU* pcCU, UInt offsetLuma, UInt offsetChroma, UInt uiAbsPartIdx, UInt uiDepth, UInt width, UInt height, UInt uiTrIdx, Bool& bCodeDQP, Int quadtreeTULog2MinSizeInCU)
{
  UInt uiSubdiv;
  const UInt uiLog2TrafoSize = g_aucConvertToBit[pcCU->getSlice()->getSPS()->getMaxCUWidth()]+2 - uiDepth;

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
  }
  if( pcCU->getPredictionMode(uiAbsPartIdx) == MODE_INTRA && pcCU->getPartitionSize(uiAbsPartIdx) == SIZE_NxN && uiDepth == pcCU->getDepth(uiAbsPartIdx) )
  {
    uiSubdiv = 1;
  }
  else if( (pcCU->getSlice()->getSPS()->getQuadtreeTUMaxDepthInter() == 1) && (pcCU->getPredictionMode(uiAbsPartIdx) == MODE_INTER) && ( pcCU->getPartitionSize(uiAbsPartIdx) != SIZE_2Nx2N ) && (uiDepth == pcCU->getDepth(uiAbsPartIdx)) )
  {
    uiSubdiv = (uiLog2TrafoSize > quadtreeTULog2MinSizeInCU);
  }
  else if( uiLog2TrafoSize > pcCU->getSlice()->getSPS()->getQuadtreeTULog2MaxSize() )
  {
    uiSubdiv = 1;
  }
  else if( uiLog2TrafoSize == pcCU->getSlice()->getSPS()->getQuadtreeTULog2MinSize() )
  {
    uiSubdiv = 0;
  }
  else if( uiLog2TrafoSize == quadtreeTULog2MinSizeInCU )
  {
    uiSubdiv = 0;
  }
  else
  {
    assert( uiLog2TrafoSize > quadtreeTULog2MinSizeInCU );
    m_pcEntropyDecoderIf->parseTransformSubdivFlag( uiSubdiv, 5 - uiLog2TrafoSize );
  }
  
  const UInt uiTrDepth = uiDepth - pcCU->getDepth( uiAbsPartIdx );
  {
    const Bool bFirstCbfOfCU = uiTrDepth == 0;
    if( bFirstCbfOfCU )
    {
      pcCU->setCbfSubParts( 0, TEXT_CHROMA_U, uiAbsPartIdx, uiDepth );
      pcCU->setCbfSubParts( 0, TEXT_CHROMA_V, uiAbsPartIdx, uiDepth );
    }
    if( bFirstCbfOfCU || uiLog2TrafoSize > 2 )
    {
      if( bFirstCbfOfCU || pcCU->getCbf( uiAbsPartIdx, TEXT_CHROMA_U, uiTrDepth - 1 ) )
      {
        m_pcEntropyDecoderIf->parseQtCbf( pcCU, uiAbsPartIdx, TEXT_CHROMA_U, uiTrDepth, uiDepth );
      }
      if( bFirstCbfOfCU || pcCU->getCbf( uiAbsPartIdx, TEXT_CHROMA_V, uiTrDepth - 1 ) )
      {
        m_pcEntropyDecoderIf->parseQtCbf( pcCU, uiAbsPartIdx, TEXT_CHROMA_V, uiTrDepth, uiDepth );
      }
    }
    else
    {
      pcCU->setCbfSubParts( pcCU->getCbf( uiAbsPartIdx, TEXT_CHROMA_U, uiTrDepth - 1 ) << uiTrDepth, TEXT_CHROMA_U, uiAbsPartIdx, uiDepth );
      pcCU->setCbfSubParts( pcCU->getCbf( uiAbsPartIdx, TEXT_CHROMA_V, uiTrDepth - 1 ) << uiTrDepth, TEXT_CHROMA_V, uiAbsPartIdx, uiDepth );
    }
  }
  
  if( uiSubdiv )
  {
    UInt size;
    width  >>= 1;
    height >>= 1;
    size = width*height;
    uiTrIdx++;
    ++uiDepth;
    const UInt uiQPartNum = pcCU->getPic()->getNumPartInCU() >> (uiDepth << 1);
    const UInt uiStartAbsPartIdx = uiAbsPartIdx;
    UInt uiYCbf = 0;
    UInt uiUCbf = 0;
    UInt uiVCbf = 0;
    
    for( Int i = 0; i < 4; i++ )
    {
      xDecodeTransform( pcCU, offsetLuma, offsetChroma, uiAbsPartIdx, uiDepth, width, height, uiTrIdx, bCodeDQP, quadtreeTULog2MinSizeInCU );
      uiYCbf |= pcCU->getCbf( uiAbsPartIdx, TEXT_LUMA, uiTrDepth+1 );
      uiUCbf |= pcCU->getCbf( uiAbsPartIdx, TEXT_CHROMA_U, uiTrDepth+1 );
      uiVCbf |= pcCU->getCbf( uiAbsPartIdx, TEXT_CHROMA_V, uiTrDepth+1 );
      uiAbsPartIdx += uiQPartNum;
      offsetLuma += size;  offsetChroma += (size>>2);
    }
    
    for( UInt ui = 0; ui < 4 * uiQPartNum; ++ui )
    {
      pcCU->getCbf( TEXT_LUMA     )[uiStartAbsPartIdx + ui] |= uiYCbf << uiTrDepth;
      pcCU->getCbf( TEXT_CHROMA_U )[uiStartAbsPartIdx + ui] |= uiUCbf << uiTrDepth;
      pcCU->getCbf( TEXT_CHROMA_V )[uiStartAbsPartIdx + ui] |= uiVCbf << uiTrDepth;
    }
  }
  else
  {
    assert( uiDepth >= pcCU->getDepth( uiAbsPartIdx ) );
    pcCU->setTrIdxSubParts( uiTrDepth, uiAbsPartIdx, uiDepth );
    
#if !H_MV_ENC_DEC_TRAC
    {
      DTRACE_CABAC_VL( g_nSymbolCounter++ );
      DTRACE_CABAC_T( "\tTrIdx: abspart=" );
      DTRACE_CABAC_V( uiAbsPartIdx );
      DTRACE_CABAC_T( "\tdepth=" );
      DTRACE_CABAC_V( uiDepth );
      DTRACE_CABAC_T( "\ttrdepth=" );
      DTRACE_CABAC_V( uiTrDepth );
      DTRACE_CABAC_T( "\n" );
    }
#endif
    
    pcCU->setCbfSubParts ( 0, TEXT_LUMA, uiAbsPartIdx, uiDepth );
    if( pcCU->getPredictionMode(uiAbsPartIdx) != MODE_INTRA && uiDepth == pcCU->getDepth( uiAbsPartIdx ) && !pcCU->getCbf( uiAbsPartIdx, TEXT_CHROMA_U, 0 ) && !pcCU->getCbf( uiAbsPartIdx, TEXT_CHROMA_V, 0 ) )
    {
      pcCU->setCbfSubParts( 1 << uiTrDepth, TEXT_LUMA, uiAbsPartIdx, uiDepth );
    }
    else
    {
      m_pcEntropyDecoderIf->parseQtCbf( pcCU, uiAbsPartIdx, TEXT_LUMA, uiTrDepth, uiDepth );
    }


    // transform_unit begin
    UInt cbfY = pcCU->getCbf( uiAbsPartIdx, TEXT_LUMA    , uiTrIdx );
    UInt cbfU = pcCU->getCbf( uiAbsPartIdx, TEXT_CHROMA_U, uiTrIdx );
    UInt cbfV = pcCU->getCbf( uiAbsPartIdx, TEXT_CHROMA_V, uiTrIdx );
    if( uiLog2TrafoSize == 2 )
    {
      UInt partNum = pcCU->getPic()->getNumPartInCU() >> ( ( uiDepth - 1 ) << 1 );
      if( ( uiAbsPartIdx % partNum ) == (partNum - 1) )
      {
        cbfU = pcCU->getCbf( m_uiBakAbsPartIdx, TEXT_CHROMA_U, uiTrIdx );
        cbfV = pcCU->getCbf( m_uiBakAbsPartIdx, TEXT_CHROMA_V, uiTrIdx );
      }
    }
    if ( cbfY || cbfU || cbfV )
    {
      // dQP: only for LCU
      if ( pcCU->getSlice()->getPPS()->getUseDQP() )
      {
        if ( bCodeDQP )
        {
          decodeQP( pcCU, m_bakAbsPartIdxCU);
          bCodeDQP = false;
        }
      }
    }
    if( cbfY )
    {
      Int trWidth = width;
      Int trHeight = height;
      m_pcEntropyDecoderIf->parseCoeffNxN( pcCU, (pcCU->getCoeffY()+offsetLuma), uiAbsPartIdx, trWidth, trHeight, uiDepth, TEXT_LUMA );
    }
    if( uiLog2TrafoSize > 2 )
    {
      Int trWidth = width >> 1;
      Int trHeight = height >> 1;
      if( cbfU )
      {
        m_pcEntropyDecoderIf->parseCoeffNxN( pcCU, (pcCU->getCoeffCb()+offsetChroma), uiAbsPartIdx, trWidth, trHeight, uiDepth, TEXT_CHROMA_U );
      }
      if( cbfV )
      {
        m_pcEntropyDecoderIf->parseCoeffNxN( pcCU, (pcCU->getCoeffCr()+offsetChroma), uiAbsPartIdx, trWidth, trHeight, uiDepth, TEXT_CHROMA_V );
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
          m_pcEntropyDecoderIf->parseCoeffNxN( pcCU, (pcCU->getCoeffCb()+m_uiBakChromaOffset), m_uiBakAbsPartIdx, trWidth, trHeight, uiDepth, TEXT_CHROMA_U );
        }
        if( cbfV )
        {
          m_pcEntropyDecoderIf->parseCoeffNxN( pcCU, (pcCU->getCoeffCr()+m_uiBakChromaOffset), m_uiBakAbsPartIdx, trWidth, trHeight, uiDepth, TEXT_CHROMA_V );
        }
      }
    }
    // transform_unit end
  }
}

Void TDecEntropy::decodeQP          ( TComDataCU* pcCU, UInt uiAbsPartIdx )
{
  if ( pcCU->getSlice()->getPPS()->getUseDQP() )
  {
    m_pcEntropyDecoderIf->parseDeltaQP( pcCU, uiAbsPartIdx, pcCU->getDepth( uiAbsPartIdx ) );
  }
}


/** decode coefficients
 * \param pcCU
 * \param uiAbsPartIdx 
 * \param uiDepth
 * \param uiWidth
 * \param uiHeight 
 * \returns Void
 */
Void TDecEntropy::decodeCoeff( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth, UInt uiWidth, UInt uiHeight, Bool& bCodeDQP )
{
  UInt uiMinCoeffSize = pcCU->getPic()->getMinCUWidth()*pcCU->getPic()->getMinCUHeight();
  UInt uiLumaOffset   = uiMinCoeffSize*uiAbsPartIdx;
  UInt uiChromaOffset = uiLumaOffset>>2;
#if H_3D_DIM_SDC
  if( pcCU->getSDCFlag( uiAbsPartIdx ) && pcCU->isIntra( uiAbsPartIdx) )
  {
    assert( pcCU->getPartitionSize(uiAbsPartIdx) == SIZE_2Nx2N );
    assert( pcCU->getTransformIdx(uiAbsPartIdx) == 0 );
    assert( pcCU->getCbf(uiAbsPartIdx, TEXT_LUMA) == 1 );
    assert( pcCU->getCbf(uiAbsPartIdx, TEXT_CHROMA_U) == 1 );
    assert( pcCU->getCbf(uiAbsPartIdx, TEXT_CHROMA_V) == 1 );
  }

#if H_3D_INTER_SDC
  if( pcCU->getSDCFlag( uiAbsPartIdx ) && !pcCU->isIntra( uiAbsPartIdx) )
  {
    assert( !pcCU->isSkipped( uiAbsPartIdx ) );
    assert( !pcCU->isIntra( uiAbsPartIdx) );
    assert( pcCU->getSlice()->getIsDepth() );
  }
#endif
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
          m_pcEntropyDecoderIf->parseDeltaDC( pcCU, uiAbsPartIdx + uiPartOffset*iPart, uiDepth + ( pcCU->getPartitionSize( uiAbsPartIdx ) == SIZE_NxN ) );
        }
      }
    }
    else
    {
      m_pcEntropyDecoderIf->parseDeltaDC( pcCU, uiAbsPartIdx, uiDepth );
      return;
    }
  }
#endif

  if( pcCU->isIntra(uiAbsPartIdx) )
  {
  }
  else
  {
    UInt uiQtRootCbf = 1;
    if( !( pcCU->getPartitionSize( uiAbsPartIdx) == SIZE_2Nx2N && pcCU->getMergeFlag( uiAbsPartIdx ) ) )
    {
      m_pcEntropyDecoderIf->parseQtRootCbf( uiAbsPartIdx, uiQtRootCbf );
    }
    if ( !uiQtRootCbf )
    {
      pcCU->setCbfSubParts( 0, 0, 0, uiAbsPartIdx, uiDepth );
      pcCU->setTrIdxSubParts( 0 , uiAbsPartIdx, uiDepth );
      return;
    }
    
  }

  Int getQuadtreeTULog2MinSizeInCU = pcCU->getQuadtreeTULog2MinSizeInCU(uiAbsPartIdx);

  xDecodeTransform( pcCU, uiLumaOffset, uiChromaOffset, uiAbsPartIdx, uiDepth, uiWidth, uiHeight, 0, bCodeDQP, getQuadtreeTULog2MinSizeInCU );
}

#if H_3D_INTER_SDC
Void TDecEntropy::decodeSDCFlag( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth )
{
  pcCU->setSDCFlagSubParts( false, uiAbsPartIdx, uiDepth );

#if HHI_TOOL_PARAMETERS_I2_J0107
  if( ( !pcCU->isIntra( uiAbsPartIdx ) && !pcCU->getSlice()->getInterSdcFlag() ) || 
    ( pcCU->isIntra( uiAbsPartIdx ) && !pcCU->getSlice()->getIntraSdcWedgeFlag() ) )
#else
  if( ( !pcCU->isIntra( uiAbsPartIdx ) && !pcCU->getSlice()->getVPS()->getInterSDCFlag( pcCU->getSlice()->getLayerIdInVps() ) ) || 
    ( pcCU->isIntra( uiAbsPartIdx ) && !pcCU->getSlice()->getVPS()->getVpsDepthModesFlag( pcCU->getSlice()->getLayerIdInVps() ) ) )
#endif
  {
    return;
  }

  if( !pcCU->getSlice()->getIsDepth() || pcCU->getPartitionSize( uiAbsPartIdx ) != SIZE_2Nx2N || pcCU->isSkipped( uiAbsPartIdx ) )
  {
    return;
  }

  assert( pcCU->getPartitionSize( uiAbsPartIdx ) == SIZE_2Nx2N || ( !pcCU->isIntra( uiAbsPartIdx ) && !pcCU->isSkipped( uiAbsPartIdx ) ) );
  m_pcEntropyDecoderIf->parseSDCFlag( pcCU, uiAbsPartIdx, uiDepth );
}
#endif
#if H_3D_DBBP
Void TDecEntropy::decodeDBBPFlag( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth )
{
  m_pcEntropyDecoderIf->parseDBBPFlag( pcCU, uiAbsPartIdx, uiDepth );
}
#endif

//! \}
