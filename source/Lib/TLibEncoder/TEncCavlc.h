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

/** \file     TEncCavlc.h
    \brief    CAVLC encoder class (header)
*/

#ifndef __TENCCAVLC__
#define __TENCCAVLC__

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "TLibCommon/CommonDef.h"
#include "TLibCommon/TComBitStream.h"
#include "TEncEntropy.h"
#include "TLibCommon/TComRom.h"

//! \ingroup TLibEncoder
//! \{

class TEncTop;

// ====================================================================================================================
// Class definition
// ====================================================================================================================

/// CAVLC encoder class
class TEncCavlc : public TEncEntropyIf
{
public:
  TEncCavlc();
  virtual ~TEncCavlc();
  
protected:
  TComBitIf*    m_pcBitIf;
  TComSlice*    m_pcSlice;
  UInt          m_uiCoeffCost;
  Bool          m_bAlfCtrl;
  UInt          m_uiMaxAlfCtrlDepth;
  Int           m_iSliceGranularity;  //!< slice granularity
  
  Void  xWriteCode            ( UInt uiCode, UInt uiLength );
  Void  xWriteUvlc            ( UInt uiCode );
  Void  xWriteSvlc            ( Int  iCode   );
  Void  xWriteFlag            ( UInt uiCode );
#if ENC_DEC_TRACE
  Void  xWriteCodeTr          ( UInt value, UInt  length, const Char *pSymbolName);
  Void  xWriteUvlcTr          ( UInt value,               const Char *pSymbolName);
  Void  xWriteSvlcTr          ( Int  value,               const Char *pSymbolName);
  Void  xWriteFlagTr          ( UInt value,               const Char *pSymbolName);
#endif
  
  Void  xWritePCMAlignZero    ();
  Void  xWriteEpExGolomb      ( UInt uiSymbol, UInt uiCount );
  Void  xWriteExGolombLevel    ( UInt uiSymbol );
  Void  xWriteUnaryMaxSymbol  ( UInt uiSymbol, UInt uiMaxSymbol );

#if RPS_IN_SPS
  Void codeShortTermRefPicSet              ( TComSPS* pcSPS, TComReferencePictureSet* pcRPS );
#else
  Void codeShortTermRefPicSet              ( TComPPS* pcPPS, TComReferencePictureSet* pcRPS );
#endif
  
  UInt  xConvertToUInt        ( Int iValue ) {  return ( iValue <= 0) ? -iValue<<1 : (iValue<<1)-1; }
  
public:
  
  Void  resetEntropy          ();
#if CABAC_INIT_FLAG
  Void  determineCabacInitIdx  () {};
#endif

  Void  setBitstream          ( TComBitIf* p )  { m_pcBitIf = p;  }
  Void  setSlice              ( TComSlice* p )  { m_pcSlice = p;  }
  Bool getAlfCtrl() {return m_bAlfCtrl;}
  UInt getMaxAlfCtrlDepth() {return m_uiMaxAlfCtrlDepth;}
  Void setAlfCtrl(Bool bAlfCtrl) {m_bAlfCtrl = bAlfCtrl;}
  Void setMaxAlfCtrlDepth(UInt uiMaxAlfCtrlDepth) {m_uiMaxAlfCtrlDepth = uiMaxAlfCtrlDepth;}
  Void  resetBits             ()                { m_pcBitIf->resetBits(); }
  Void  resetCoeffCost        ()                { m_uiCoeffCost = 0;  }
  UInt  getNumberOfWrittenBits()                { return  m_pcBitIf->getNumberOfWrittenBits();  }
  UInt  getCoeffCost          ()                { return  m_uiCoeffCost;  }
  
#if VIDYO_VPS_INTEGRATION|QC_MVHEVC_B0046
  Void  codeVPS                 ( TComVPS* pcVPS );
#endif

#if HHI_MPI
  Void  codeSPS                 ( TComSPS* pcSPS, Bool bIsDepth );
#else
  Void  codeSPS                 ( TComSPS* pcSPS );
#endif
  Void  codePPS                 ( TComPPS* pcPPS );
  void codeSEI(const SEI&);
  Void  codeSliceHeader         ( TComSlice* pcSlice );

  Void codeTileMarkerFlag(TComSlice* pcSlice);

#if TILES_WPP_ENTRY_POINT_SIGNALLING
  Void  codeTilesWPPEntryPoint( TComSlice* pSlice );
#else
  Void  codeSliceHeaderSubstreamTable( TComSlice* pcSlice );
#endif
  Void  codeTerminatingBit      ( UInt uilsLast );
  Void  codeSliceFinish         ();
  Void  codeFlush               () {}
  Void  encodeStart             () {}
  
#if HHI_INTER_VIEW_MOTION_PRED
  Void codeMVPIdx ( TComDataCU* pcCU, UInt uiAbsPartIdx, RefPicList eRefList, Int iNum );
#else
  Void codeMVPIdx ( TComDataCU* pcCU, UInt uiAbsPartIdx, RefPicList eRefList );
#endif
  Void codeAlfFlag       ( UInt uiCode );
  Void codeAlfUvlc       ( UInt uiCode );
  Void codeAlfSvlc       ( Int   iCode );
  Void codeAlfCtrlDepth();
#if LCU_SYNTAX_ALF
  Void codeAPSAlflag(UInt uiCode);
  Void codeAlfFixedLengthIdx( UInt idx, UInt numFilterSetsInBuffer);
#endif
  Void codeSaoFlag       ( UInt uiCode );
  Void codeSaoUvlc       ( UInt uiCode );
  Void codeSaoSvlc       ( Int   iCode );
#if SAO_UNIT_INTERLEAVING
  Void codeSaoRun        ( UInt uiCode, UInt maxValue  );
  Void codeSaoMergeLeft  ( UInt uiCode, UInt compIdx ){;}
  Void codeSaoMergeUp    ( UInt uiCode ){;}
  Void codeSaoTypeIdx    ( UInt uiCode ){ xWriteUvlc(uiCode   );}
  Void codeSaoUflc       ( UInt uiCode ){ assert(uiCode < 32); xWriteCode(uiCode, 5);}
#endif

  Void codeSkipFlag      ( TComDataCU* pcCU, UInt uiAbsPartIdx );
#if LGE_ILLUCOMP_B0045
  Void codeICFlag        ( TComDataCU* pcCU, UInt uiAbsPartIdx );
#endif
#if FORCE_REF_VSP==1
  Void codeVspFlag       ( TComDataCU* pcCU, UInt uiAbsPartIdx );
#endif
  Void codeMergeFlag     ( TComDataCU* pcCU, UInt uiAbsPartIdx );
  Void codeMergeIndex    ( TComDataCU* pcCU, UInt uiAbsPartIdx );
#if HHI_INTER_VIEW_RESIDUAL_PRED
  Void codeResPredFlag   ( TComDataCU* pcCU, UInt uiAbsPartIdx );
#endif
  Void codeAlfCtrlFlag   ( TComDataCU* pcCU, UInt uiAbsPartIdx );

  Void codeApsExtensionFlag ();

  /// set slice granularity
  Void setSliceGranularity(Int iSliceGranularity)  {m_iSliceGranularity = iSliceGranularity;}

  ///get slice granularity
  Int  getSliceGranularity()                       {return m_iSliceGranularity;             }

  Void codeAlfCtrlFlag   ( UInt uiSymbol );
  Void codeInterModeFlag( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth, UInt uiEncMode );
  Void codeSplitFlag     ( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth );
  
  Void codePartSize      ( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth );
  Void codePredMode      ( TComDataCU* pcCU, UInt uiAbsPartIdx );
  
#if BURST_IPCM
  Void codeIPCMInfo      ( TComDataCU* pcCU, UInt uiAbsPartIdx, Int numIPCM, Bool firstIPCMFlag);
#else
  Void codeIPCMInfo      ( TComDataCU* pcCU, UInt uiAbsPartIdx );
#endif

  Void codeTransformSubdivFlag( UInt uiSymbol, UInt uiCtx );
  Void codeQtCbf         ( TComDataCU* pcCU, UInt uiAbsPartIdx, TextType eType, UInt uiTrDepth );
  Void codeQtRootCbf     ( TComDataCU* pcCU, UInt uiAbsPartIdx );
  
  Void codeIntraDirLumaAng( TComDataCU* pcCU, UInt uiAbsPartIdx );
  
  Void codeIntraDirChroma( TComDataCU* pcCU, UInt uiAbsPartIdx );
  Void codeInterDir      ( TComDataCU* pcCU, UInt uiAbsPartIdx );
  Void codeRefFrmIdx     ( TComDataCU* pcCU, UInt uiAbsPartIdx, RefPicList eRefList );
  Void codeMvd           ( TComDataCU* pcCU, UInt uiAbsPartIdx, RefPicList eRefList );
  
  Void codeDeltaQP       ( TComDataCU* pcCU, UInt uiAbsPartIdx );
  
  Void codeCoeffNxN      ( TComDataCU* pcCU, TCoeff* pcCoef, UInt uiAbsPartIdx, UInt uiWidth, UInt uiHeight, UInt uiDepth, TextType eTType );
  
  Void estBit               (estBitsSbacStruct* pcEstBitsSbac, Int width, Int height, TextType eTType);
  
  Void xCodePredWeightTable          ( TComSlice* pcSlice );
  Void updateContextTables           ( SliceType eSliceType, Int iQp, Bool bExecuteFinish=true ) { return;   }
  Void updateContextTables           ( SliceType eSliceType, Int iQp  )                          { return;   }
  Void writeTileMarker               ( UInt uiTileIdx, UInt uiBitsUsed );

  Void  codeAPSInitInfo(TComAPS* pcAPS);  //!< code APS flags before encoding SAO and ALF parameters
  Void  codeFinish(Bool bEnd) { /*do nothing*/}
  Void codeScalingList  ( TComScalingList* scalingList );
  Void xCodeScalingList ( TComScalingList* scalingList, UInt sizeId, UInt listId);
  Void codeDFFlag       ( UInt uiCode, const Char *pSymbolName );
  Void codeDFSvlc       ( Int   iCode, const Char *pSymbolName );
  
#if RWTH_SDC_DLT_B0036
  Void codeSDCFlag          ( TComDataCU* pcCU, UInt uiAbsPartIdx );
  Void codeSDCResidualData  ( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiSegment );
  Void codeSDCPredMode          ( TComDataCU* pcCU, UInt uiAbsPartIdx );
#endif

};

//! \}

#endif // !defined(AFX_TENCCAVLC_H__EE8A0B30_945B_4169_B290_24D3AD52296F__INCLUDED_)

