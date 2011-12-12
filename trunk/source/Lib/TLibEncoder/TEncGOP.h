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



/** \file     TEncPic.h
    \brief    GOP encoder class (header)
*/

#ifndef __TEncPic__
#define __TEncPic__

#include <stdlib.h>

#include "../TLibCommon/TComList.h"
#include "../TLibCommon/TComPic.h"
#include "../TLibCommon/TComBitStream.h"
#include "../TLibCommon/TComBitCounter.h"
#include "../TLibCommon/TComLoopFilter.h"
#include "../TLibCommon/TComDepthMapGenerator.h"
#include "../TLibCommon/TComResidualGenerator.h"
#include "TEncAdaptiveLoopFilter.h"
#include "TEncSlice.h"
#include "TEncEntropy.h"
#include "TEncCavlc.h"
#include "TEncSbac.h"

#include "TEncAnalyze.h"

#ifdef WEIGHT_PRED
#include "WeightPredAnalysis.h"
#endif

#if RVM_VCEGAM10
#include <vector>
#endif

class TEncTop;

// ====================================================================================================================
// Class definition
// ====================================================================================================================

/// GOP encoder class
class TEncPic
{
private:
  //  Data
  Int                     m_iHrchDepth;
  
  //  Access channel
  TEncTop*                m_pcEncTop;
  TEncCfg*                m_pcCfg;
  TEncSlice*              m_pcSliceEncoder;
  TComList<TComPic*>*     m_pcListPic;
  
  TEncEntropy*            m_pcEntropyCoder;
  TEncCavlc*              m_pcCavlcCoder;
  TEncSbac*               m_pcSbacCoder;
  TEncBinCABAC*           m_pcBinCABAC;
  TComLoopFilter*         m_pcLoopFilter;
#if DEPTH_MAP_GENERATION
  TComDepthMapGenerator*  m_pcDepthMapGenerator;
#endif
#if HHI_INTER_VIEW_RESIDUAL_PRED
  TComResidualGenerator*  m_pcResidualGenerator;
#endif
  
  // Adaptive Loop filter
  TEncAdaptiveLoopFilter* m_pcAdaptiveLoopFilter;
  //--Adaptive Loop filter
#if MTK_SAO
  TEncSampleAdaptiveOffset*  m_pcSAO;
#endif
  TComBitCounter*         m_pcBitCounter;
  
  TComRdCost*             m_pcRdCost;                           ///< RD cost computation
  
#if DCM_DECODING_REFRESH
  // clean decoding refresh
  Bool                    m_bRefreshPending;
  UInt                    m_uiPOCCDR;
#endif
  UInt*                   m_uiStoredStartCUAddrForEncodingSlice;
  UInt*                   m_uiStoredStartCUAddrForEncodingEntropySlice;

// #if MTK_NONCROSS_INLOOP_FILTER
//   UInt                    m_uiILSliceCount;
//   UInt*                   m_puiILSliceStartLCU;
//   UInt*                   m_puiILSliceEndLCU;
// #endif

#if RVM_VCEGAM10
  std::vector<Int> m_vRVM_RP;
#endif

public:
  TEncPic();
  virtual ~TEncPic();
  
  Void  create      ( Int iWidth, Int iHeight, UInt iMaxCUWidth, UInt iMaxCUHeight );
  Void  destroy     ();
  
  Void  init        ( TEncTop* pcTEncTop );
  Void compressPic( TComBitstream* pcBitstreamOut, TComPicYuv cPicOrg, TComPic* pcPic, TComPicYuv* pcPicYuvRecOut,
                 TComPic* pcOrgRefList[2][MAX_REF_PIC_NUM], Bool&  rbSeqFirst, TComList<TComPic*>& rcListPic  );
  TComList<TComPic*>*   getListPic()      { return m_pcListPic; }
  
  Void  preLoopFilterPicAll  ( TComPic* pcPic, UInt64& ruiDist, UInt64& ruiBits );

  
protected:
#if DCM_DECODING_REFRESH
  NalUnitType getNalUnitType( UInt uiPOCCurr );
#endif

  Void  xCalculateAddPSNR ( TComPic* pcPic, TComPicYuv* pcPicD, UInt uiBits, Double dEncTime );
  
  UInt64 xFindDistortionFrame (TComPicYuv* pcPic0, TComPicYuv* pcPic1);

#if RVM_VCEGAM10
  Double xCalculateRVM();
#endif
};// END CLASS DEFINITION TEncPic


#endif // __TEncPic__

