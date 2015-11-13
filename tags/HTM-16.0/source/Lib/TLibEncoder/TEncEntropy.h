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

/** \file     TEncEntropy.h
    \brief    entropy encoder class (header)
*/

#ifndef __TENCENTROPY__
#define __TENCENTROPY__

#include "TLibCommon/TComSlice.h"
#include "TLibCommon/TComDataCU.h"
#include "TLibCommon/TComBitStream.h"
#include "TLibCommon/ContextModel.h"
#include "TLibCommon/TComPic.h"
#include "TLibCommon/TComTrQuant.h"
#include "TLibCommon/TComSampleAdaptiveOffset.h"
#include "TLibCommon/TComChromaFormat.h"

class TEncSbac;
class TEncCavlc;
class SEI;

// ====================================================================================================================
// Class definition
// ====================================================================================================================

/// entropy encoder pure class
class TEncEntropyIf
{
public:
  virtual Void  resetEntropy          (const TComSlice *pSlice)                = 0;
  virtual SliceType determineCabacInitIdx (const TComSlice *pSlice)                = 0;
  virtual Void  setBitstream          ( TComBitIf* p )  = 0;
  virtual Void  resetBits             ()                = 0;
  virtual UInt  getNumberOfWrittenBits()                = 0;

  virtual Void  codeVPS                 ( const TComVPS* pcVPS )                                      = 0;
  virtual Void  codeSPS                 ( const TComSPS* pcSPS )                                      = 0;
  virtual Void  codePPS                 ( const TComPPS* pcPPS )                                      = 0;
  virtual Void  codeSliceHeader         ( TComSlice* pcSlice )                                  = 0;

  virtual Void  codeTilesWPPEntryPoint  ( TComSlice* pSlice )     = 0;
  virtual Void  codeTerminatingBit      ( UInt uilsLast )                                       = 0;
  virtual Void  codeSliceFinish         ()                                                      = 0;
  virtual Void  codeMVPIdx ( TComDataCU* pcCU, UInt uiAbsPartIdx, RefPicList eRefList ) = 0;

public:
  virtual Void codeCUTransquantBypassFlag( TComDataCU* pcCU, UInt uiAbsPartIdx ) = 0;
  virtual Void codeSkipFlag      ( TComDataCU* pcCU, UInt uiAbsPartIdx ) = 0;
#if NH_3D_DIS
  virtual Void codeDIS          ( TComDataCU* pcCU, UInt uiAbsPartIdx ) = 0;
#endif
  virtual Void codeMergeFlag     ( TComDataCU* pcCU, UInt uiAbsPartIdx ) = 0;
  virtual Void codeMergeIndex    ( TComDataCU* pcCU, UInt uiAbsPartIdx ) = 0;
#if NH_3D_DMM || NH_3D_SDC_INTRA || NH_3D_SDC_INTER
  virtual Void codeDeltaDC       ( TComDataCU* pcCU, UInt uiAbsPartIdx ) = 0;
#endif
#if NH_3D_ARP  
  virtual Void codeARPW          ( TComDataCU* pcCU, UInt uiAbsPartIdx ) = 0;
#endif
#if NH_3D_IC
  virtual Void codeICFlag        ( TComDataCU* pcCU, UInt uiAbsPartIdx ) = 0;
#endif
#if NH_3D_SDC_INTRA || NH_3D_SDC_INTER
  virtual Void codeSDCFlag       ( TComDataCU* pcCU, UInt uiAbsPartIdx ) = 0;
#endif
#if NH_3D_DBBP
  virtual Void codeDBBPFlag      ( TComDataCU* pcCU, UInt uiAbsPartIdx ) = 0;
#endif
  virtual Void codeSplitFlag     ( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth ) = 0;

  virtual Void codePartSize      ( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth ) = 0;
  virtual Void codePredMode      ( TComDataCU* pcCU, UInt uiAbsPartIdx ) = 0;

  virtual Void codeIPCMInfo      ( TComDataCU* pcCU, UInt uiAbsPartIdx ) = 0;

  virtual Void codeTransformSubdivFlag( UInt uiSymbol, UInt uiCtx ) = 0;
  virtual Void codeQtCbf         ( TComTU &rTu, const ComponentID compID, const Bool lowestLevel ) = 0;
  virtual Void codeQtRootCbf     ( TComDataCU* pcCU, UInt uiAbsPartIdx ) = 0;
  virtual Void codeQtCbfZero     ( TComTU &rTu, const ChannelType chType ) = 0;
  virtual Void codeQtRootCbfZero ( ) = 0;
  virtual Void codeIntraDirLumaAng( TComDataCU* pcCU, UInt uiAbsPartIdx, Bool isMultiplePU ) = 0;

  virtual Void codeIntraDirChroma( TComDataCU* pcCU, UInt uiAbsPartIdx ) = 0;
  virtual Void codeInterDir      ( TComDataCU* pcCU, UInt uiAbsPartIdx ) = 0;
  virtual Void codeRefFrmIdx     ( TComDataCU* pcCU, UInt uiAbsPartIdx, RefPicList eRefList )      = 0;
  virtual Void codeMvd           ( TComDataCU* pcCU, UInt uiAbsPartIdx, RefPicList eRefList )      = 0;

  virtual Void codeCrossComponentPrediction( TComTU &rTu, ComponentID compID ) = 0;

  virtual Void codeDeltaQP       ( TComDataCU* pcCU, UInt uiAbsPartIdx ) = 0;
  virtual Void codeChromaQpAdjustment( TComDataCU* pcCU, UInt uiAbsPartIdx ) = 0;
  virtual Void codeCoeffNxN      ( TComTU &rTu, TCoeff* pcCoef, const ComponentID compID ) = 0;
  virtual Void codeTransformSkipFlags ( TComTU &rTu, ComponentID component ) = 0;
  virtual Void codeSAOBlkParam   (SAOBlkParam& saoBlkParam, const BitDepths &bitDepths, Bool* sliceEnabled, Bool leftMergeAvail, Bool aboveMergeAvail, Bool onlyEstMergeInfo = false)    =0;
  virtual Void estBit               (estBitsSbacStruct* pcEstBitsSbac, Int width, Int height, ChannelType chType) = 0;

  virtual Void codeExplicitRdpcmMode ( TComTU &rTu, const ComponentID compID ) = 0;

  virtual ~TEncEntropyIf() {}
};

/// entropy encoder class
class TEncEntropy
{
public:
  Void    setEntropyCoder           ( TEncEntropyIf* e );
  Void    setBitstream              ( TComBitIf* p )          { m_pcEntropyCoderIf->setBitstream(p);  }
  Void    resetBits                 ()                        { m_pcEntropyCoderIf->resetBits();      }
#if NH_MV
  UInt    getNumberOfWrittenBits    ()                        {
      D_PRINT_INDENT(g_encNumberOfWrittenBits,  "NumBits: " +  n2s( m_pcEntropyCoderIf->getNumberOfWrittenBits() ))
 return m_pcEntropyCoderIf->getNumberOfWrittenBits(); }
#else
  UInt    getNumberOfWrittenBits    ()                        { return m_pcEntropyCoderIf->getNumberOfWrittenBits(); }
#endif
  Void    resetEntropy              (const TComSlice *pSlice) { m_pcEntropyCoderIf->resetEntropy(pSlice);  }
  SliceType determineCabacInitIdx   (const TComSlice *pSlice) { return m_pcEntropyCoderIf->determineCabacInitIdx(pSlice); }

  Void    encodeSliceHeader         ( TComSlice* pcSlice );
  Void    encodeTilesWPPEntryPoint( TComSlice* pSlice );
  Void    encodeTerminatingBit      ( UInt uiIsLast );
  Void    encodeSliceFinish         ();
  TEncEntropyIf*      m_pcEntropyCoderIf;

public:
  Void encodeVPS               ( const TComVPS* pcVPS);
  // SPS
  Void encodeSPS               ( const TComSPS* pcSPS );
  Void encodePPS               ( const TComPPS* pcPPS );
  Void encodeSplitFlag         ( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth, Bool bRD = false );
  Void encodeCUTransquantBypassFlag( TComDataCU* pcCU, UInt uiAbsPartIdx, Bool bRD = false );
  Void encodeSkipFlag          ( TComDataCU* pcCU, UInt uiAbsPartIdx, Bool bRD = false );
#if NH_3D_DIS
  Void encodeDIS               ( TComDataCU* pcCU, UInt uiAbsPartIdx, Bool bRD=false );
#endif
  Void encodePUWise       ( TComDataCU* pcCU, UInt uiAbsPartIdx );
  Void encodeInterDirPU   ( TComDataCU* pcSubCU, UInt uiAbsPartIdx  );
  Void encodeRefFrmIdxPU  ( TComDataCU* pcSubCU, UInt uiAbsPartIdx, RefPicList eRefList );
  Void encodeMvdPU        ( TComDataCU* pcSubCU, UInt uiAbsPartIdx, RefPicList eRefList );
  Void encodeMVPIdxPU     ( TComDataCU* pcSubCU, UInt uiAbsPartIdx, RefPicList eRefList );
  Void encodeMergeFlag    ( TComDataCU* pcCU, UInt uiAbsPartIdx );
  Void encodeMergeIndex   ( TComDataCU* pcCU, UInt uiAbsPartIdx, Bool bRD = false );
#if NH_3D_ARP
  Void encodeARPW         ( TComDataCU* pcCU, UInt uiAbspartIdx );
#endif
#if NH_3D_IC
  Void encodeICFlag       ( TComDataCU* pcCU, UInt uiAbsPartIdx, Bool bRD = false );
#endif
#if NH_3D_DMM || NH_3D_SDC_INTRA || NH_3D_SDC_INTER
  Void encodeDeltaDC      ( TComDataCU* pcCU, UInt absPartIdx );
#endif
#if NH_3D_SDC_INTRA || NH_3D_SDC_INTER
  Void encodeSDCFlag      ( TComDataCU* pcCU, UInt uiAbsPartIdx, Bool bRD = false );
#endif
#if NH_3D_DBBP
  Void encodeDBBPFlag     ( TComDataCU* pcCU, UInt uiAbsPartIdx, Bool bRD = false );
#endif
  Void encodePredMode          ( TComDataCU* pcCU, UInt uiAbsPartIdx, Bool bRD = false );
  Void encodePartSize          ( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth, Bool bRD = false );
  Void encodeIPCMInfo          ( TComDataCU* pcCU, UInt uiAbsPartIdx, Bool bRD = false );
  Void encodePredInfo          ( TComDataCU* pcCU, UInt uiAbsPartIdx );
  Void encodeIntraDirModeLuma  ( TComDataCU* pcCU, UInt absPartIdx, Bool isMultiplePU = false );

  Void encodeIntraDirModeChroma( TComDataCU* pcCU, UInt uiAbsPartIdx );

  Void encodeTransformSubdivFlag( UInt uiSymbol, UInt uiCtx );
  Void encodeQtCbf             ( TComTU &rTu, const ComponentID compID, const Bool lowestLevel );

  Void encodeQtCbfZero         ( TComTU &rTu, const ChannelType chType );
  Void encodeQtRootCbfZero     ( );
  Void encodeQtRootCbf         ( TComDataCU* pcCU, UInt uiAbsPartIdx );
  Void encodeQP                ( TComDataCU* pcCU, UInt uiAbsPartIdx, Bool bRD = false );
  Void encodeChromaQpAdjustment ( TComDataCU* pcCU, UInt uiAbsPartIdx, Bool bRD = false );

  Void encodeCrossComponentPrediction( TComTU &rTu, ComponentID compID );

private:
  Void xEncodeTransform        ( Bool& bCodeDQP, Bool& codeChromaQpAdj, TComTU &rTu );

public:
  Void encodeCoeff             ( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth, Bool& bCodeDQP, Bool& codeChromaQpAdj );

  Void encodeCoeffNxN         ( TComTU &rTu, TCoeff* pcCoef, const ComponentID compID );

  Void estimateBit             ( estBitsSbacStruct* pcEstBitsSbac, Int width, Int height, ChannelType chType );

  Void encodeSAOBlkParam(SAOBlkParam& saoBlkParam, const BitDepths &bitDepths, Bool* sliceEnabled, Bool leftMergeAvail, Bool aboveMergeAvail){m_pcEntropyCoderIf->codeSAOBlkParam(saoBlkParam, bitDepths, sliceEnabled, leftMergeAvail, aboveMergeAvail, false);}

  static Int countNonZeroCoeffs( TCoeff* pcCoef, UInt uiSize );

};// END CLASS DEFINITION TEncEntropy

//! \}

#endif // __TENCENTROPY__

