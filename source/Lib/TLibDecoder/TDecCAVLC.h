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

/** \file     TDecCAVLC.h
    \brief    CAVLC decoder class (header)
*/

#ifndef __TDECCAVLC__
#define __TDECCAVLC__

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "TDecEntropy.h"
#include "SyntaxElementParser.h"

//! \ingroup TLibDecoder
//! \{

// ====================================================================================================================
// Class definition
// ====================================================================================================================
#if NH_MV
class TDecTop;
#endif
/// CAVLC decoder class
class TDecCavlc : public SyntaxElementParser, public TDecEntropyIf
{
public:
  TDecCavlc();
  virtual ~TDecCavlc();

protected:

#if NH_MV
  Void  parseShortTermRefPicSet            (TComSPS* pcSPS, TComStRefPicSet* pcRPS, Int stRpsIdx );
#else
  Void  parseShortTermRefPicSet            (TComSPS* pcSPS, TComReferencePictureSet* pcRPS, Int idx);
#endif

#if NH_MV
  TDecTop*  m_decTop;
#endif
public:

  /// rest entropy coder by intial QP and IDC in CABAC
  Void  resetEntropy        ( TComSlice* /*pcSlice*/  )     { assert(0); };
  Void  setBitstream        ( TComInputBitstream* p )   { m_pcBitstream = p; }
  Void  parseTransformSubdivFlag( UInt& ruiSubdivFlag, UInt uiLog2TransformBlockSize );
  Void  parseQtCbf          ( class TComTU &rTu, const ComponentID compID, const Bool lowestLevel );
  Void  parseQtRootCbf      ( UInt uiAbsPartIdx, UInt& uiQtRootCbf );
  Void  parseVPS            ( TComVPS* pcVPS );
#if NH_MV
  Void  parseVPSExtension   ( TComVPS* pcVPS ); 
  Void  parseRepFormat      ( Int i, TComRepFormat* curRepFormat, const TComRepFormat* prevRepFormat );
  Void  parseVPSVUI         ( const TComVPS* pcVPS, TComVPSVUI* vpsVui );
  Void  parseVideoSignalInfo( TComVideoSignalInfo* pcVideoSignalInfo ); 
  Void  parseDpbSize        ( TComVPS* pcVPS ); 
  Void  parseVpsVuiBspHrdParameters( const TComVPS* vps, TComVPSVUI* vpsVui ); 

  Void  parseSpsMultilayerExtension( TComSPS* pcSPS );  
  Void  parsePpsMultilayerExtension( TComPPS* pcPPS );
  Void  setDecTop           ( TDecTop* decTop ) { m_decTop = decTop; }; 
#endif
#if NH_3D
  Void  parseVps3dExtension  ( TComVPS* pcVPS ); 
  Void  parseSps3dExtension  ( TComSPS* pcSPS );  
  Void  parsePps3dExtension  ( TComPPS* pcPPS );
#endif

  Void  parseSPS            ( TComSPS* pcSPS );
  Void  parsePPS            ( TComPPS* pcPPS );

  Void  parseVUI            ( TComVUI* pcVUI, TComSPS* pcSPS );
  Void  parseSEI            ( SEIMessages& );
  Void  parsePTL            ( TComPTL *rpcPTL, Bool profilePresentFlag, Int maxNumSubLayersMinus1 );
  Void  parseProfileTier    (ProfileTierLevel *ptl, const Bool bIsSubLayer);
  Void  parseHrdParameters  (TComHRD *hrd, Bool cprms_present_flag, UInt tempLevelHigh);
#if NH_MV
  Void  parseFirstSliceSegmentInPicFlag( TComSlice* pcSlice );
  Void  parseSliceHeader    ( TComSlice* pcSlice, ParameterSetManager *parameterSetManager );
#else
  Void  parseSliceHeader    ( TComSlice* pcSlice, ParameterSetManager *parameterSetManager, const Int prevTid0POC);
#endif

  Void  parseTerminatingBit ( UInt& ruiBit );
  Void  parseRemainingBytes ( Bool noTrailingBytesExpected );

  Void  parseMVPIdx          ( Int& riMVPIdx );
        
  Void  parseSkipFlag        ( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth );
#if NH_3D_DIS
  Void  parseDIS             ( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth );
#endif
  Void parseCUTransquantBypassFlag( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth );
  Void parseMergeFlag       ( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth, UInt uiPUIdx );
  Void parseMergeIndex      ( TComDataCU* pcCU, UInt& ruiMergeIndex );
#if NH_3D_ARP 
  Void parseARPW            ( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth );
#endif
#if NH_3D_IC
  Void  parseICFlag         ( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth );
#endif
#if NH_3D_DMM || NH_3D_SDC_INTRA || NH_3D_SDC_INTER
  Void  parseDeltaDC        ( TComDataCU* /*pcCU*/, UInt /*absPartIdx*/, UInt /*depth*/ ) { assert(0); };
#endif
#if NH_3D_SDC_INTRA || NH_3D_SDC_INTER
  Void  parseSDCFlag        ( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth );
#endif
#if NH_3D_DBBP
  Void  parseDBBPFlag       ( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth );
#endif
  Void parseSplitFlag       ( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth );
  Void parsePartSize        ( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth );
  Void parsePredMode        ( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth );

  Void parseIntraDirLumaAng ( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth );
  Void parseIntraDirChroma  ( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth );

  Void parseInterDir        ( TComDataCU* pcCU, UInt& ruiInterDir, UInt uiAbsPartIdx );
  Void parseRefFrmIdx       ( TComDataCU* pcCU, Int& riRefFrmIdx, RefPicList eRefList );
  Void parseMvd             ( TComDataCU* pcCU, UInt uiAbsPartAddr,UInt uiPartIdx,    UInt uiDepth, RefPicList eRefList );

  Void parseCrossComponentPrediction( class TComTU &rTu, ComponentID compID );

  Void parseDeltaQP         ( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth );
  Void parseChromaQpAdjustment( TComDataCU* cu, UInt absPartIdx, UInt depth);

  Void parseCoeffNxN        ( class TComTU &rTu, ComponentID compID );

  Void parseTransformSkipFlags ( class TComTU &rTu, ComponentID component );

  Void parseIPCMInfo        ( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth);

  Void xParsePredWeightTable ( TComSlice* pcSlice, const TComSPS *sps );
  Void  parseScalingList     ( TComScalingList* scalingList );
  Void xDecodeScalingList    ( TComScalingList *scalingList, UInt sizeId, UInt listId);

  Void  parseExplicitRdpcmMode( TComTU &rTu, ComponentID compID );
#if NH_MV
  TDecTop*  getDecTop()      { return m_decTop; };
#endif

protected:
  Bool  xMoreRbspData();
};

//! \}

#endif // !defined(AFX_TDECCAVLC_H__9732DD64_59B0_4A41_B29E_1A5B18821EAD__INCLUDED_)
