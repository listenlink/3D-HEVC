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

/** \file     TDecEntropy.h
    \brief    entropy decoder class (header)
*/

#ifndef __TDECENTROPY__
#define __TDECENTROPY__

#include "../TLibCommon/CommonDef.h"
#include "../TLibCommon/TComBitStream.h"
#include "../TLibCommon/TComSlice.h"
#include "../TLibCommon/TComPic.h"
#include "../TLibCommon/TComPrediction.h"
#include "../TLibCommon/TComSampleAdaptiveOffset.h"

class TDecSbac;
class TDecCavlc;
class ParameterSetManagerDecoder;

//! \ingroup TLibDecoder
//! \{

// ====================================================================================================================
// Class definition
// ====================================================================================================================

/// entropy decoder pure class
class TDecEntropyIf
{
public:
  //  Virtual list for SBAC/CAVLC
  virtual Void  resetEntropy          ( TComSlice* pcSlice )     = 0;
  virtual Void  setBitstream          ( TComInputBitstream* p )  = 0;

  virtual Void  parseVPS                  ( TComVPS* pcVPS )                       = 0;
#if HHI_TOOL_PARAMETERS_I2_J0107
  virtual Void  parseSPS                  ( TComSPS* pcSPS )                                      = 0;
#else
#if H_3D
  virtual Void  parseSPS                  ( TComSPS* pcSPS, Int viewIndex, Bool depthFlag  )                    = 0;
#else
  virtual Void  parseSPS                  ( TComSPS* pcSPS )                                      = 0;
#endif
#endif
#if H_3D
  virtual Void  parsePPS                  ( TComPPS* pcPPS, TComVPS* pcVPS )                      = 0;
#else
  virtual Void  parsePPS                  ( TComPPS* pcPPS )                                      = 0;
#endif

#if H_MV
  virtual Void parseSliceHeader          ( TComSlice*& rpcSlice, ParameterSetManagerDecoder *parameterSetManager, Int targetOlsIdx )       = 0;
#else
  virtual Void parseSliceHeader          ( TComSlice*& rpcSlice, ParameterSetManagerDecoder *parameterSetManager)       = 0;
#endif

  virtual Void  parseTerminatingBit       ( UInt& ruilsLast )                                     = 0;
  
  virtual Void parseMVPIdx        ( Int& riMVPIdx ) = 0;
  
public:
  virtual Void parseSkipFlag      ( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth ) = 0;
#if H_3D_SINGLE_DEPTH
  virtual Void parseSingleDepthMode       ( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth ) = 0;
#endif
  virtual Void parseCUTransquantBypassFlag( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth ) = 0;
  virtual Void parseSplitFlag     ( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth ) = 0;
  virtual Void parseMergeFlag     ( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth, UInt uiPUIdx ) = 0;
  virtual Void parseMergeIndex    ( TComDataCU* pcCU, UInt& ruiMergeIndex ) = 0;
#if H_3D_ARP
  virtual Void parseARPW          ( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth ) = 0;
#endif
#if H_3D_IC
  virtual Void parseICFlag        ( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth ) = 0;
#endif
#if H_3D_INTER_SDC
  virtual Void parseDeltaDC       ( TComDataCU* pcCU, UInt absPartIdx, UInt depth ) = 0;
  virtual Void parseSDCFlag       ( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth ) = 0;
#endif
#if H_3D_DBBP
  virtual Void parseDBBPFlag      ( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth ) = 0;
#endif
  virtual Void parsePartSize      ( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth ) = 0;
  virtual Void parsePredMode      ( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth ) = 0;
  
  virtual Void parseIntraDirLumaAng( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth ) = 0;
  
  virtual Void parseIntraDirChroma( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth ) = 0;
  
  virtual Void parseInterDir      ( TComDataCU* pcCU, UInt& ruiInterDir, UInt uiAbsPartIdx ) = 0;
  virtual Void parseRefFrmIdx     ( TComDataCU* pcCU, Int& riRefFrmIdx, RefPicList eRefList ) = 0;
  virtual Void parseMvd           ( TComDataCU* pcCU, UInt uiAbsPartAddr, UInt uiPartIdx, UInt uiDepth, RefPicList eRefList ) = 0;
  
  virtual Void parseTransformSubdivFlag( UInt& ruiSubdivFlag, UInt uiLog2TransformBlockSize ) = 0;
  virtual Void parseQtCbf         ( TComDataCU* pcCU, UInt uiAbsPartIdx, TextType eType, UInt uiTrDepth, UInt uiDepth ) = 0;
  virtual Void parseQtRootCbf     ( UInt uiAbsPartIdx, UInt& uiQtRootCbf ) = 0;
  
  virtual Void parseDeltaQP       ( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth ) = 0;
  
  virtual Void parseIPCMInfo     ( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth) = 0;

  virtual Void parseCoeffNxN( TComDataCU* pcCU, TCoeff* pcCoef, UInt uiAbsPartIdx, UInt uiWidth, UInt uiHeight, UInt uiDepth, TextType eTType ) = 0;
  virtual Void parseTransformSkipFlags ( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt width, UInt height, UInt uiDepth, TextType eTType) = 0;
  virtual Void updateContextTables( SliceType eSliceType, Int iQp ) = 0;
  
  virtual ~TDecEntropyIf() {}
};

/// entropy decoder class
class TDecEntropy
{
private:
  TDecEntropyIf*  m_pcEntropyDecoderIf;
  TComPrediction* m_pcPrediction;
  UInt    m_uiBakAbsPartIdx;
  UInt    m_uiBakChromaOffset;
  UInt    m_bakAbsPartIdxCU;
  
public:
  Void init (TComPrediction* p) {m_pcPrediction = p;}
  Void decodePUWise       ( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth, TComDataCU* pcSubCU );
  Void decodeInterDirPU   ( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth, UInt uiPartIdx );
  Void decodeRefFrmIdxPU  ( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth, UInt uiPartIdx, RefPicList eRefList );
  Void decodeMvdPU        ( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth, UInt uiPartIdx, RefPicList eRefList );
  Void decodeMVPIdxPU     ( TComDataCU* pcSubCU, UInt uiPartAddr, UInt uiDepth, UInt uiPartIdx, RefPicList eRefList );
  
  Void    setEntropyDecoder           ( TDecEntropyIf* p );
  Void    setBitstream                ( TComInputBitstream* p ) { m_pcEntropyDecoderIf->setBitstream(p);                    }
  Void    resetEntropy                ( TComSlice* p)           { m_pcEntropyDecoderIf->resetEntropy(p);                    }
  Void    decodeVPS                   ( TComVPS* pcVPS ) { m_pcEntropyDecoderIf->parseVPS(pcVPS); }
#if HHI_TOOL_PARAMETERS_I2_J0107
  Void    decodeSPS                   ( TComSPS* pcSPS     )    { m_pcEntropyDecoderIf->parseSPS(pcSPS);                    }
#else
#if H_3D
  Void    decodeSPS                   ( TComSPS* pcSPS, Int viewIndex, Bool depthFlag )    { m_pcEntropyDecoderIf->parseSPS(pcSPS, viewIndex, depthFlag );                    }
#else
  Void    decodeSPS                   ( TComSPS* pcSPS     )    { m_pcEntropyDecoderIf->parseSPS(pcSPS);                    }
#endif
#endif
#if H_3D
  Void    decodePPS                   ( TComPPS* pcPPS, TComVPS* pcVPS )    { m_pcEntropyDecoderIf->parsePPS(pcPPS, pcVPS);                    }
#else
  Void    decodePPS                   ( TComPPS* pcPPS )    { m_pcEntropyDecoderIf->parsePPS(pcPPS);                    }
#endif
#if H_MV
  Void    decodeSliceHeader           ( TComSlice*& rpcSlice, ParameterSetManagerDecoder *parameterSetManager, Int targetOlsIdx)  { m_pcEntropyDecoderIf->parseSliceHeader(rpcSlice, parameterSetManager, targetOlsIdx );         }
#else
  Void    decodeSliceHeader           ( TComSlice*& rpcSlice, ParameterSetManagerDecoder *parameterSetManager)  { m_pcEntropyDecoderIf->parseSliceHeader(rpcSlice, parameterSetManager);         }
#endif
  Void    decodeTerminatingBit        ( UInt& ruiIsLast )       { m_pcEntropyDecoderIf->parseTerminatingBit(ruiIsLast);     }
  
  TDecEntropyIf* getEntropyDecoder() { return m_pcEntropyDecoderIf; }
  
public:
  Void decodeSplitFlag         ( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth );
  Void decodeSkipFlag          ( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth );
#if H_3D_SINGLE_DEPTH
  Void decodeSingleDepthMode ( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth ) ;
#endif
  Void decodeCUTransquantBypassFlag( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth );
  Void decodeMergeFlag         ( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth, UInt uiPUIdx );
  Void decodeMergeIndex        ( TComDataCU* pcSubCU, UInt uiPartIdx, UInt uiPartAddr, UInt uiDepth );
  Void decodePredMode          ( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth );
  Void decodePartSize          ( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth );
  
#if H_3D_ARP
  Void decodeARPW              ( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth );
#endif
#if H_3D_IC
  Void decodeICFlag            ( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth );
#endif
#if H_3D_INTER_SDC
  Void decodeSDCFlag           ( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth );
#endif
#if H_3D_DBBP
  Void decodeDBBPFlag          ( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth );
#endif
  Void decodeIPCMInfo          ( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth );

  Void decodePredInfo          ( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth, TComDataCU* pcSubCU );
  
  Void decodeIntraDirModeLuma  ( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth );
  Void decodeIntraDirModeChroma( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth );
  
  Void decodeQP                ( TComDataCU* pcCU, UInt uiAbsPartIdx );
  
  Void updateContextTables    ( SliceType eSliceType, Int iQp ) { m_pcEntropyDecoderIf->updateContextTables( eSliceType, iQp ); }
  
  
private:
  Void xDecodeTransform        ( TComDataCU* pcCU, UInt offsetLuma, UInt offsetChroma, UInt uiAbsPartIdx, UInt uiDepth, UInt width, UInt height, UInt uiTrIdx, Bool& bCodeDQP, Int getQuadtreeTULog2MinSizeInCU );

public:
  Void decodeCoeff             ( TComDataCU* pcCU                 , UInt uiAbsPartIdx, UInt uiDepth, UInt uiWidth, UInt uiHeight, Bool& bCodeDQP );
  
};// END CLASS DEFINITION TDecEntropy

//! \}

#endif // __TDECENTROPY__

