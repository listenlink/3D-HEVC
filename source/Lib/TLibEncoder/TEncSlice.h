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

/** \file     TEncSlice.h
    \brief    slice encoder class (header)
*/

#ifndef __TENCSLICE__
#define __TENCSLICE__

// Include files
#include "TLibCommon/CommonDef.h"
#include "TLibCommon/TComList.h"
#include "TLibCommon/TComPic.h"
#include "TLibCommon/TComPicYuv.h"
#include "TEncCu.h"
#include "WeightPredAnalysis.h"
#include "TEncRateCtrl.h"

//! \ingroup TLibEncoder
//! \{

class TEncTop;
class TEncGOP;

// ====================================================================================================================
// Class definition
// ====================================================================================================================

/// slice encoder class
class TEncSlice
  : public WeightPredAnalysis
{
private:
  // encoder configuration
  TEncCfg*                m_pcCfg;                              ///< encoder configuration class
  
  // pictures
  TComList<TComPic*>*     m_pcListPic;                          ///< list of pictures
  TComPicYuv*             m_apcPicYuvPred;                      ///< prediction picture buffer
  TComPicYuv*             m_apcPicYuvResi;                      ///< residual picture buffer
  
  // processing units
  TEncGOP*                m_pcGOPEncoder;                       ///< GOP encoder
  TEncCu*                 m_pcCuEncoder;                        ///< CU encoder
  
  // encoder search
  TEncSearch*             m_pcPredSearch;                       ///< encoder search class
  
  // coding tools
  TEncEntropy*            m_pcEntropyCoder;                     ///< entropy encoder
  TEncCavlc*              m_pcCavlcCoder;                       ///< CAVLC encoder
  TEncSbac*               m_pcSbacCoder;                        ///< SBAC encoder
  TEncBinCABAC*           m_pcBinCABAC;                         ///< Bin encoder CABAC
  TComTrQuant*            m_pcTrQuant;                          ///< transform & quantization
  
  // RD optimization
  TComBitCounter*         m_pcBitCounter;                       ///< bit counter
  TComRdCost*             m_pcRdCost;                           ///< RD cost computation
  TEncSbac***             m_pppcRDSbacCoder;                    ///< storage for SBAC-based RD optimization
  TEncSbac*               m_pcRDGoOnSbacCoder;                  ///< go-on SBAC encoder
  UInt64                  m_uiPicTotalBits;                     ///< total bits for the picture
#if H_3D_VSO
  Dist64                  m_uiPicDist;                          ///< total distortion for the picture
#else
  UInt64                  m_uiPicDist;                          ///< total distortion for the picture
#endif
  Double                  m_dPicRdCost;                         ///< picture-level RD cost
  Double*                 m_pdRdPicLambda;                      ///< array of lambda candidates
  Double*                 m_pdRdPicQp;                          ///< array of picture QP candidates (double-type for lambda)
  Int*                    m_piRdPicQp;                          ///< array of picture QP candidates (Int-type)
  TEncBinCABAC*           m_pcBufferBinCoderCABACs;       ///< line of bin coder CABAC
  TEncSbac*               m_pcBufferSbacCoders;                 ///< line to store temporary contexts
  TEncBinCABAC*           m_pcBufferLowLatBinCoderCABACs;       ///< dependent tiles: line of bin coder CABAC
  TEncSbac*               m_pcBufferLowLatSbacCoders;           ///< dependent tiles: line to store temporary contexts
  TEncRateCtrl*           m_pcRateCtrl;                         ///< Rate control manager
  UInt                    m_uiSliceIdx;
  std::vector<TEncSbac*> CTXMem;
#if !LGE_DDD_REMOVAL_J0042_J0030
#if H_3D_DDD
  Int          m_iDDDScale;
  Int          m_iDDDOffset;
  UInt         m_uiDDDPrecision;
#endif
#endif
public:
  TEncSlice();
  virtual ~TEncSlice();
  
  Void    create              ( Int iWidth, Int iHeight, UInt iMaxCUWidth, UInt iMaxCUHeight, UChar uhTotalDepth );
  Void    destroy             ();
  Void    init                ( TEncTop* pcEncTop );
  
  /// preparation of slice encoding (reference marking, QP and lambda)
#if H_MV
  Void    initEncSlice        ( TComPic* pcPic, Int pocLast, Int pocCurr, Int iNumPicRcvd, 
                                Int iGOPid, TComSlice*& rpcSlice, TComVPS* pVPS, TComSPS* pSPS, TComPPS *pPPS, Int layerId, bool isField  ); 
#else
  Void    initEncSlice        ( TComPic*  pcPic, Int pocLast, Int pocCurr, Int iNumPicRcvd,
                                Int iGOPid,   TComSlice*& rpcSlice, TComSPS* pSPS, TComPPS *pPPS, bool isField  );
#endif
  Void    resetQP             ( TComPic* pic, Int sliceQP, Double lambda );
  // compress and encode slice
  Void    precompressSlice    ( TComPic*& rpcPic                                );      ///< precompress slice for multi-loop opt.
  Void    compressSlice       ( TComPic*& rpcPic                                );      ///< analysis stage of slice
  Void    calCostSliceI       ( TComPic*& rpcPic );
  Void    encodeSlice         ( TComPic*& rpcPic, TComOutputBitstream* pcSubstreams  );
  
  // misc. functions
  Void    setSearchRange      ( TComSlice* pcSlice  );                                  ///< set ME range adaptively
  UInt64  getTotalBits        ()  { return m_uiPicTotalBits; }
  
  TEncCu*        getCUEncoder() { return m_pcCuEncoder; }                        ///< CU encoder
  Void    xDetermineStartAndBoundingCUAddr  ( UInt& uiStartCUAddr, UInt& uiBoundingCUAddr, TComPic*& rpcPic, Bool bEncodeSlice );
  UInt    getSliceIdx()         { return m_uiSliceIdx;                    }
  Void    setSliceIdx(UInt i)   { m_uiSliceIdx = i;                       }
  Void      initCtxMem( UInt i );
  Void      setCtxMem( TEncSbac* sb, Int b )   { CTXMem[b] = sb; }
#if !LGE_DDD_REMOVAL_J0042_J0030
#if H_3D_DDD
  Void setDDDPar( Int iScale, Int iOffset, UInt uiPrecision ){ m_iDDDScale = iScale; m_iDDDOffset = iOffset; m_uiDDDPrecision = uiPrecision; }
#endif
#endif

private:
  Double  xGetQPValueAccordingToLambda ( Double lambda );
};

//! \}

#endif // __TENCSLICE__
