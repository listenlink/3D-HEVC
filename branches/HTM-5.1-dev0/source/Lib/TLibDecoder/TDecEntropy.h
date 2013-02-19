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
#include "../TLibCommon/TComAdaptiveLoopFilter.h"
#include "../TLibCommon/TComSampleAdaptiveOffset.h"

class TDecSbac;
class TDecCavlc;
class SEImessages;
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

#if OL_FLUSH
  virtual Void  decodeFlush()                                                                      = 0;
#endif

#if VIDYO_VPS_INTEGRATION|QC_MVHEVC_B0046
  virtual Void  parseVPS                  ( TComVPS* pcVPS )                       = 0;
#endif
#if HHI_MPI || OL_QTLIMIT_PREDCODING_B0068
  virtual Void  parseSPS                  ( TComSPS* pcSPS, Bool bIsDepth )                       = 0;
#else
  virtual Void  parseSPS                  ( TComSPS* pcSPS )                                      = 0;
#endif
#if TILES_OR_ENTROPY_SYNC_IDC  
  virtual Void  parsePPS                  ( TComPPS* pcPPS, ParameterSetManagerDecoder *parameterSet )                                      = 0;
#else
  virtual Void  parsePPS                  ( TComPPS* pcPPS )                                      = 0;
#endif
  virtual Void  parseAPS                  ( TComAPS* pAPS  )                                      = 0;
  virtual void parseSEI(SEImessages&) = 0;

#if LCU_SYNTAX_ALF
#if MTK_DEPTH_MERGE_TEXTURE_CANDIDATE_C0137
  virtual Void parseSliceHeader          ( TComSlice*& rpcSlice, ParameterSetManagerDecoder *parameterSetManager, AlfCUCtrlInfo &alfCUCtrl, AlfParamSet& alfParamSet, bool isDepth)       = 0;
#else
  virtual Void parseSliceHeader          ( TComSlice*& rpcSlice, ParameterSetManagerDecoder *parameterSetManager, AlfCUCtrlInfo &alfCUCtrl, AlfParamSet& alfParamSet)       = 0;
#endif
#else
  virtual Void parseSliceHeader          ( TComSlice*& rpcSlice, ParameterSetManagerDecoder *parameterSetManager, AlfCUCtrlInfo &alfCUCtrl )                                = 0;
#endif

  virtual Void  parseTerminatingBit       ( UInt& ruilsLast )                                     = 0;
  
#if H3D_IVMP
  virtual Void parseMVPIdx        ( Int& riMVPIdx, Int iNumAMVPCands ) = 0;
#else
  virtual Void parseMVPIdx        ( Int& riMVPIdx ) = 0;
#endif
  
public:
  virtual Void parseSkipFlag      ( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth ) = 0;
#if LGE_ILLUCOMP_B0045
  virtual Void parseICFlag        ( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth ) = 0;
#endif
  virtual Void parseSplitFlag     ( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth ) = 0;
  virtual Void parseMergeFlag     ( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth, UInt uiPUIdx ) = 0;
  virtual Void parseMergeIndex    ( TComDataCU* pcCU, UInt& ruiMergeIndex, UInt uiAbsPartIdx, UInt uiDepth ) = 0;
#if HHI_INTER_VIEW_RESIDUAL_PRED
  virtual Void parseResPredFlag   ( TComDataCU* pcCU, Bool& rbResPredFlag, UInt uiAbsPartIdx, UInt uiDepth ) = 0;
#endif
  virtual Void parsePartSize      ( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth ) = 0;
  virtual Void parsePredMode      ( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth ) = 0;
  
  virtual Void parseIntraDirLumaAng( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth ) = 0;
  
  virtual Void parseIntraDirChroma( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth ) = 0;
  
  virtual Void parseInterDir      ( TComDataCU* pcCU, UInt& ruiInterDir, UInt uiAbsPartIdx, UInt uiDepth ) = 0;
  virtual Void parseRefFrmIdx     ( TComDataCU* pcCU, Int& riRefFrmIdx, UInt uiAbsPartIdx, UInt uiDepth, RefPicList eRefList ) = 0;
  virtual Void parseMvd           ( TComDataCU* pcCU, UInt uiAbsPartAddr, UInt uiPartIdx, UInt uiDepth, RefPicList eRefList ) = 0;
  
  virtual Void parseTransformSubdivFlag( UInt& ruiSubdivFlag, UInt uiLog2TransformBlockSize ) = 0;
  virtual Void parseQtCbf         ( TComDataCU* pcCU, UInt uiAbsPartIdx, TextType eType, UInt uiTrDepth, UInt uiDepth ) = 0;
  virtual Void parseQtRootCbf     ( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth, UInt& uiQtRootCbf ) = 0;
  
  virtual Void parseDeltaQP       ( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth ) = 0;
  
  virtual Void parseIPCMInfo     ( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth) = 0;

  virtual Void parseCoeffNxN( TComDataCU* pcCU, TCoeff* pcCoef, UInt uiAbsPartIdx, UInt uiWidth, UInt uiHeight, UInt uiDepth, TextType eTType ) = 0;

  /// set slice granularity
  virtual Void setSliceGranularity(Int iSliceGranularity) = 0;

  /// get slice granularity
  virtual Int  getSliceGranularity()                      = 0;

  virtual Void readTileMarker   ( UInt& uiTileIdx, UInt uiBitsUsed ) = 0;
  virtual Void updateContextTables( SliceType eSliceType, Int iQp ) = 0;
  
#if RWTH_SDC_DLT_B0036
  virtual Void parseSDCFlag    ( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth ) = 0;
  virtual Void parseSDCPredMode     ( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth ) = 0;
  virtual Void parseSDCResidualData     ( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth, UInt uiPart ) = 0;
#endif
  
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
#if UNIFIED_TRANSFORM_TREE
  UInt    m_bakAbsPartIdxCU;
#endif
  
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

#if VIDYO_VPS_INTEGRATION|QC_MVHEVC_B0046
  Void    decodeVPS                   ( TComVPS* pcVPS ) { m_pcEntropyDecoderIf->parseVPS(pcVPS); }
#endif
  
#if HHI_MPI || OL_QTLIMIT_PREDCODING_B0068
  Void    decodeSPS                   ( TComSPS* pcSPS, Bool bIsDepth ) { m_pcEntropyDecoderIf->parseSPS(pcSPS, bIsDepth); }
#else
  Void    decodeSPS                   ( TComSPS* pcSPS     )    { m_pcEntropyDecoderIf->parseSPS(pcSPS);                    }
#endif
#if TILES_OR_ENTROPY_SYNC_IDC
  Void    decodePPS                   ( TComPPS* pcPPS, ParameterSetManagerDecoder *parameterSet    )    { m_pcEntropyDecoderIf->parsePPS(pcPPS, parameterSet);                    }
#else
  Void    decodePPS                   ( TComPPS* pcPPS     )    { m_pcEntropyDecoderIf->parsePPS(pcPPS);                    }
#endif
  Void    decodeAPS                   ( TComAPS* pAPS      )    { m_pcEntropyDecoderIf->parseAPS(pAPS);}
  void decodeSEI(SEImessages& seis) { m_pcEntropyDecoderIf->parseSEI(seis); }

#if LCU_SYNTAX_ALF
#if MTK_DEPTH_MERGE_TEXTURE_CANDIDATE_C0137
  Void    decodeSliceHeader           ( TComSlice*& rpcSlice, ParameterSetManagerDecoder *parameterSetManager, AlfCUCtrlInfo &alfCUCtrl, AlfParamSet& alfParamSet, bool isDepth)  { m_pcEntropyDecoderIf->parseSliceHeader(rpcSlice, parameterSetManager, alfCUCtrl, alfParamSet, isDepth);         }
#else
  Void    decodeSliceHeader           ( TComSlice*& rpcSlice, ParameterSetManagerDecoder *parameterSetManager, AlfCUCtrlInfo &alfCUCtrl, AlfParamSet& alfParamSet)  { m_pcEntropyDecoderIf->parseSliceHeader(rpcSlice, parameterSetManager, alfCUCtrl, alfParamSet);         }
#endif
#else
  Void    decodeSliceHeader           ( TComSlice*& rpcSlice, ParameterSetManagerDecoder *parameterSetManager, AlfCUCtrlInfo &alfCUCtrl )  { m_pcEntropyDecoderIf->parseSliceHeader(rpcSlice, parameterSetManager, alfCUCtrl);         }
#endif

  Void    decodeTerminatingBit        ( UInt& ruiIsLast )       { m_pcEntropyDecoderIf->parseTerminatingBit(ruiIsLast);     }
  
  TDecEntropyIf* getEntropyDecoder() { return m_pcEntropyDecoderIf; }
  
public:
  Void decodeSplitFlag         ( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth );
  Void decodeSkipFlag          ( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth );
#if LGE_ILLUCOMP_B0045
  Void decodeICFlag            ( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth );
#endif
  Void decodeMergeFlag         ( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth, UInt uiPUIdx );
  Void decodeMergeIndex        ( TComDataCU* pcSubCU, UInt uiPartIdx, UInt uiPartAddr, PartSize eCUMode, UChar* puhInterDirNeighbours, TComMvField* pcMvFieldNeighbours, UInt uiDepth );
#if HHI_INTER_VIEW_RESIDUAL_PRED && !MTK_MDIVRP_C0138
  Void decodeResPredFlag       ( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth, TComDataCU* pcSubCU, UInt uiPUIdx );
#endif
  Void decodePredMode          ( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth );
  Void decodePartSize          ( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth );
  
  Void decodeIPCMInfo          ( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth );

  Void decodePredInfo          ( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth, TComDataCU* pcSubCU );
  
  Void decodeIntraDirModeLuma  ( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth );
  Void decodeIntraDirModeChroma( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth );
  
#if !UNIFIED_TRANSFORM_TREE
  Void decodeTransformIdx      ( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth );
#endif
#if UNIFIED_TRANSFORM_TREE
  Void decodeQP                ( TComDataCU* pcCU, UInt uiAbsPartIdx );
#else
  Void decodeQP                ( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth );
#endif
  
  Void readTileMarker         ( UInt& uiTileIdx, UInt uiBitsUsed )  {  m_pcEntropyDecoderIf->readTileMarker( uiTileIdx, uiBitsUsed ); }
  Void updateContextTables    ( SliceType eSliceType, Int iQp ) { m_pcEntropyDecoderIf->updateContextTables( eSliceType, iQp ); }
  
  
private:
#if UNIFIED_TRANSFORM_TREE
  Void xDecodeTransform        ( TComDataCU* pcCU, UInt offsetLuma, UInt offsetChroma, UInt uiAbsPartIdx, UInt absTUPartIdx, UInt uiDepth, UInt width, UInt height, UInt uiTrIdx, UInt uiInnerQuadIdx, UInt& uiYCbfFront3, UInt& uiUCbfFront3, UInt& uiVCbfFront3, Bool& bCodeDQP );
#else
  Void xDecodeTransformSubdiv  ( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt absTUPartIdx, UInt uiDepth, UInt uiInnerQuadIdx, UInt& uiYCbfFront3, UInt& uiUCbfFront3, UInt& uiVCbfFront3 );
  
  Void xDecodeCoeff            ( TComDataCU* pcCU, UInt uiLumaOffset, UInt uiChromaOffset, UInt uiAbsPartIdx, UInt uiDepth, UInt uiWidth, UInt uiHeight, UInt uiTrIdx, UInt uiCurrTrIdx, Bool& bCodeDQP );
#endif //UNIFIED_TRANSFORM_TREE

public:
  Void decodeCoeff             ( TComDataCU* pcCU                 , UInt uiAbsPartIdx, UInt uiDepth, UInt uiWidth, UInt uiHeight, Bool& bCodeDQP );
  
  // ALF-related

  /// set slice granularity
  Void setSliceGranularity (Int iSliceGranularity) {m_pcEntropyDecoderIf->setSliceGranularity(iSliceGranularity);}

#if SAO_UNIT_INTERLEAVING
  Void decodeSaoParam         (SAOParam* saoParam);
  void decodeSaoLcu(Int rx, Int ry, Int compIdx, SAOParam* saoParam, Bool &repeatedRow );
  Void decodeSaoOneLcu(SaoLcuParam* saoLcuParam);
#endif

#if OL_FLUSH
  Void decodeFlush() { m_pcEntropyDecoderIf->decodeFlush(); }
#endif
  
#if RWTH_SDC_DLT_B0036
  Void decodeSDCPredMode( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth );
  Void decodeSDCFlag( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth );
  Void decodeSDCResidualData ( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth );
#endif

};// END CLASS DEFINITION TDecEntropy

//! \}

#endif // __TDECENTROPY__

