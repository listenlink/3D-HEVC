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



/** \file     TEncCU.h
    \brief    CU encoder class (header)
*/

#ifndef __TENCCU__
#define __TENCCU__

// Include files
#include "../TLibCommon/CommonDef.h"
#include "../TLibCommon/TComYuv.h"
#include "../TLibCommon/TComPrediction.h"
#include "../TLibCommon/TComTrQuant.h"
#include "../TLibCommon/TComBitCounter.h"
#include "../TLibCommon/TComDataCU.h"

#include "TEncEntropy.h"
#include "TEncSearch.h"

class TEncTop;
class TEncSbac;
class TEncCavlc;
class TEncSlice;

// ====================================================================================================================
// Class definition
// ====================================================================================================================

/// CU encoder class
class TEncCu
{
private:
  
  TComDataCU**            m_ppcBestCU;      ///< Best CUs in each depth
  TComDataCU**            m_ppcTempCU;      ///< Temporary CUs in each depth
  UChar                   m_uhTotalDepth;
  
  TComYuv**               m_ppcPredYuvBest; ///< Best Prediction Yuv for each depth
  TComYuv**               m_ppcResiYuvBest; ///< Best Residual Yuv for each depth
  TComYuv**               m_ppcRecoYuvBest; ///< Best Reconstruction Yuv for each depth
  TComYuv**               m_ppcPredYuvTemp; ///< Temporary Prediction Yuv for each depth
  TComYuv**               m_ppcResiYuvTemp; ///< Temporary Residual Yuv for each depth
  TComYuv**               m_ppcRecoYuvTemp; ///< Temporary Reconstruction Yuv for each depth
  TComYuv**               m_ppcOrigYuv;     ///< Original Yuv for each depth
  TComYuv**               m_ppcResPredTmp;  ///< Temporary residual prediction for each depth
  
  //  Data : encoder control
  Int                     m_iQp;            ///< Last QP
  
  //  Access channel
  TEncCfg*                m_pcEncCfg;
  TEncTop*                m_pcEncTop;
  TComPrediction*         m_pcPrediction;
  TEncSearch*             m_pcPredSearch;
  TComTrQuant*            m_pcTrQuant;
  TComBitCounter*         m_pcBitCounter;
  TComRdCost*             m_pcRdCost;
  
  TEncEntropy*            m_pcEntropyCoder;
  TEncCavlc*              m_pcCavlcCoder;
  TEncSbac*               m_pcSbacCoder;
  TEncBinCABAC*           m_pcBinCABAC;
  
  // SBAC RD
  TEncSbac***             m_pppcRDSbacCoder;
  TEncSbac*               m_pcRDGoOnSbacCoder;
  Bool                    m_bUseSBACRD;
  
#if HHI_MPI
  UChar *m_puhDepthSaved;
  UChar *m_puhWidthSaved;
  UChar *m_puhHeightSaved;
#endif

public:
  /// copy parameters from encoder class
  Void  init                ( TEncTop* pcEncTop );
  
  /// create internal buffers
  Void  create              ( UChar uhTotalDepth, UInt iMaxWidth, UInt iMaxHeight );
  
  /// destroy internal buffers
  Void  destroy             ();
  
  /// CU analysis function
  Void  compressCU          ( TComDataCU*&  rpcCU );
  
  /// CU encoding function
  Void  encodeCU            ( TComDataCU*    pcCU, Bool bForceTerminate = false  );
  
  /// set QP value
  Void  setQpLast           ( Int iQp ) { m_iQp = iQp; }
  
protected:
  Void  xCompressCU         ( TComDataCU*& rpcBestCU, TComDataCU*& rpcTempCU, UInt uiDepth        );
  Void  xEncodeCU           ( TComDataCU*  pcCU, UInt uiAbsPartIdx,           UInt uiDepth        );
  
  Void  xCheckRDCostAMVPSkip( TComDataCU*& rpcBestCU, TComDataCU*& rpcTempCU                      );
  
#if HHI_INTERVIEW_SKIP
  Void xCheckRDCostMerge2Nx2N( TComDataCU*& rpcBestCU, TComDataCU*& rpcTempCU, Bool bFullyRendered ) ;
#else
  Void  xCheckRDCostMerge2Nx2N( TComDataCU*& rpcBestCU, TComDataCU*& rpcTempCU                  );
#endif
  
  Void  xCheckRDCostSkip    ( TComDataCU*& rpcBestCU, TComDataCU*& rpcTempCU, Bool bBSkipRes      );
#if HHI_INTERVIEW_SKIP
  Void xCheckRDCostInter( TComDataCU*& rpcBestCU, TComDataCU*& rpcTempCU, PartSize ePartSize, Bool bFullyRendered ) ;
#else
  Void  xCheckRDCostInter   ( TComDataCU*& rpcBestCU, TComDataCU*& rpcTempCU, PartSize ePartSize  );
#endif
  Void  xCheckRDCostIntra   ( TComDataCU*& rpcBestCU, TComDataCU*& rpcTempCU, PartSize ePartSize  );
  Void  xCheckBestMode      ( TComDataCU*& rpcBestCU, TComDataCU*& rpcTempCU, UChar uhDepth       );
  
  Void  xCopyAMVPInfo       ( AMVPInfo* pSrc, AMVPInfo* pDst );
  Void  xCopyYuv2Pic        ( TComPic* rpcPic, UInt uiCUAddr, UInt uiAbsZorderIdx, UInt uiDepth );
  Void  xCopyYuv2Tmp        ( UInt uhPartUnitIdx, UInt uiDepth );
  Void  xAddMVISignallingBits( TComDataCU* pcCU );
#if HHI_MPI
  Void  xCheckRDCostMvInheritance( TComDataCU*& rpcBestCU, TComDataCU*& rpcTempCU, UChar uhTextureModeDepth, Bool bSkipResidual, Bool bRecursiveCall );
  Void  xSaveDepthWidthHeight( TComDataCU* pcCU );
  Void  xRestoreDepthWidthHeight( TComDataCU* pcCU );
#endif
};


#endif // __TENCMB__

