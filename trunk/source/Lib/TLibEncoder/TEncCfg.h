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


/** \file     TEncCfg.h
    \brief    encoder configuration class (header)
*/

#ifndef __TENCCFG__
#define __TENCCFG__

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "../TLibCommon/CommonDef.h"
#include "TEncSeqStructure.h"
#include <assert.h>

#include <vector>

// ====================================================================================================================
// Class definition
// ====================================================================================================================

/// encoder configuration class
class TEncCfg
{
protected:
  //==== File I/O ========
  Int       m_iFrameRate;
  Int       m_FrameSkip;
  Int       m_iSourceWidth;
  Int       m_iSourceHeight;
  Int       m_iFrameToBeEncoded;

  //====== Coding Structure ========
#if DCM_DECODING_REFRESH
  UInt      m_uiDecodingRefreshType;            ///< the type of decoding refresh employed for the random access.
#endif

  UInt      m_uiCodedPictureStoreSize ;
  Int       m_iGOPSize;
  Int       m_iRateGOPSize;

  std::string     m_cInputFormatString ;

  Int       m_iQP;                              //  if (AdaptiveQP == OFF)

  Int       m_aiTLayerQPOffset[MAX_TLAYER];
  Int       m_aiPad[2];

  //======= Transform =============
  UInt      m_uiQuadtreeTULog2MaxSize;
  UInt      m_uiQuadtreeTULog2MinSize;
  UInt      m_uiQuadtreeTUMaxDepthInter;
  UInt      m_uiQuadtreeTUMaxDepthIntra;

  //====== B Slice ========

  //====== Entropy Coding ========
  Int       m_iSymbolMode;                      //  (CAVLC, CABAC)

  //====== Loop/Deblock Filter ========
  Bool      m_bLoopFilterDisable;
  Int       m_iLoopFilterAlphaC0Offset;
  Int       m_iLoopFilterBetaOffset;

#if MTK_SAO
  Bool      m_bUseSAO;
#endif

  //====== Motion search ========
  Int       m_iFastSearch;                      //  0:Full search  1:Diamond  2:PMVFAST
  Int       m_iSearchRange;                     //  0:Full frame
  Int       m_bipredSearchRange;
  Int       m_iMaxDeltaQP;                      //  Max. absolute delta QP (1:default)

#if HHI_VSO
  //====== View Synthesis Optimization ======
  Bool      m_bForceLambdaScale;
#if HHI_VSO_DIST_INT
  Bool      m_bAllowNegDist;
#endif
  Double    m_dLambdaScaleVSO;
  UInt      m_uiVSOMode;
#endif

  //====== Tool list ========
  Bool      m_bUseSBACRD;
  Bool      m_bUseALF;
#if MQT_ALF_NPASS
  Int       m_iALFEncodePassReduction;
#endif
  Bool      m_bUseASR;
  Bool      m_bUseHADME;
#if DCM_COMB_LIST
  Bool      m_bUseLComb;
  Bool      m_bLCMod;
#endif
  Bool      m_bUseRDOQ;
#if !HHI_NO_LowDelayCoding
  Bool      m_bUseLDC;
#endif
  Bool      m_bUsePAD;
  Bool      m_bUseFastEnc;

#if HHI_VSO
  Bool      m_bUseVSO;
#endif  
  Bool      m_bUseMRG; // SOPH:
#if LM_CHROMA
  Bool      m_bUseLMChroma;
#endif

  Int*      m_aidQP;
  UInt      m_uiDeltaQpRD;

#if HHI_RMP_SWITCH
  Bool      m_bUseRMP;
#endif
#ifdef ROUNDING_CONTROL_BIPRED
  Bool m_useRoundingControlBipred;
#endif
#if CONSTRAINED_INTRA_PRED
  Bool      m_bUseConstrainedIntraPred;
#endif
#if HHI_DMM_WEDGE_INTRA || HHI_DMM_PRED_TEX
  Bool m_bUseDMM;
#endif
#if HHI_MPI
  Bool m_bUseMVI;
#endif

  //====== Slice ========
  Int       m_iSliceMode;
  Int       m_iSliceArgument;
  //====== Entropy Slice ========
  Int       m_iEntropySliceMode;
  Int       m_iEntropySliceArgument;
#if MTK_NONCROSS_INLOOP_FILTER
  Bool      m_bLFCrossSliceBoundaryFlag;
#endif

  bool m_pictureDigestEnabled; ///< enable(1)/disable(0) md5 computation and SEI signalling

#ifdef WEIGHT_PRED
  //====== Weighted Prediction ========
  Bool      m_bUseWeightPred;       // Use of Weighting Prediction (P_SLICE)
  UInt      m_uiBiPredIdc;          // Use of Bi-Directional Weighting Prediction (B_SLICE)
#endif

  TEncSeqStructure m_cSequenceStructure;
  //std::vector<int>  m_aiLayerQPOffset;

  UInt        m_uiViewId;
  Int         m_iViewOrderIdx;
  Bool        m_bIsDepth;
  UInt        m_uiCamParPrecision;
  Bool        m_bCamParInSliceHeader;
  Int**       m_aaiCodedScale;
  Int**       m_aaiCodedOffset;

#if DEPTH_MAP_GENERATION
  UInt        m_uiPredDepthMapGeneration;
  UInt        m_uiPdmPrecision;
  Int**       m_aaiPdmScaleNomDelta;
  Int**       m_aaiPdmOffset;
#endif
#if HHI_INTER_VIEW_MOTION_PRED
  UInt        m_uiMultiviewMvPredMode;
  UInt        m_uiMultiviewMvRegMode;
  Double      m_dMultiviewMvRegLambdaScale;
#endif
#if HHI_INTER_VIEW_RESIDUAL_PRED
  UInt        m_uiMultiviewResPredMode;
#endif

  PicOrderCnt m_iQpChangeFrame;
  Int         m_iQpChangeOffsetVideo;
  Int         m_iQpChangeOffsetDepth;
#if HHI_INTERVIEW_SKIP
  UInt        m_uiInterViewSkip;
#if HHI_INTERVIEW_SKIP_LAMBDA_SCALE
  Double      m_dInterViewSkipLambdaScale;
#endif
#endif

public:
  TEncCfg()          {}
  virtual ~TEncCfg() {}

  Void      setFrameRate                    ( Int   i )      { m_iFrameRate = i; }
  Void      setFrameSkip                    ( unsigned int i ) { m_FrameSkip = i; }
  Void      setSourceWidth                  ( Int   i )      { m_iSourceWidth = i; }
  Void      setSourceHeight                 ( Int   i )      { m_iSourceHeight = i; }
  Void      setFrameToBeEncoded             ( Int   i )      { m_iFrameToBeEncoded = i; }

  //====== Coding Structure ========
#if DCM_DECODING_REFRESH
  Void      setDecodingRefreshType          ( Int   i )      { m_uiDecodingRefreshType = (UInt)i; }
#endif
  Void      setCPSSize                      ( Int   i )      { m_uiCodedPictureStoreSize = (UInt) i ;}
  Void      setGOPSize                      ( Int   i )      { m_iGOPSize = i; }
  Void      setRateGOPSize                  ( Int   i )      { m_iRateGOPSize = i; }

  Void      setViewId                       ( UInt  u )      { m_uiViewId               = u; }
  Void      setViewOrderIdx                 ( Int   i )      { m_iViewOrderIdx          = i; }
  Void      setIsDepth                      ( Bool  b )      { m_bIsDepth               = b; }
  Void      setCamParPrecision              ( UInt  u )      { m_uiCamParPrecision      = u; }
  Void      setCamParInSliceHeader          ( Bool  b )      { m_bCamParInSliceHeader   = b; }
  Void      setCodedScale                   ( Int** p )      { m_aaiCodedScale          = p; }
  Void      setCodedOffset                  ( Int** p )      { m_aaiCodedOffset         = p; }

#if DEPTH_MAP_GENERATION
  Void      setPredDepthMapGeneration       ( UInt  u )      { m_uiPredDepthMapGeneration   = u; }
  Void      setPdmPrecision                 ( UInt  u )      { m_uiPdmPrecision             = u; }
  Void      setPdmScaleNomDelta             ( Int** p )      { m_aaiPdmScaleNomDelta        = p; }
  Void      setPdmOffset                    ( Int** p )      { m_aaiPdmOffset               = p; }
#endif
#if HHI_INTER_VIEW_MOTION_PRED
  Void      setMultiviewMvPredMode          ( UInt  u )      { m_uiMultiviewMvPredMode      = u; }
  Void      setMultiviewMvRegMode           ( UInt  u )      { m_uiMultiviewMvRegMode       = u; }
  Void      setMultiviewMvRegLambdaScale    ( Double d)      { m_dMultiviewMvRegLambdaScale = d; }
#endif
#if HHI_INTER_VIEW_RESIDUAL_PRED
  Void      setMultiviewResPredMode         ( UInt  u )      { m_uiMultiviewResPredMode     = u; }
#endif

#if HHI_INTERVIEW_SKIP
  Void      setInterViewSkip            ( UInt  u )       { m_uiInterViewSkip         = u; }
  Bool      getInterViewSkip            ( )       { return (m_uiInterViewSkip?true:false) ;}
#if HHI_INTERVIEW_SKIP_LAMBDA_SCALE
  Void      setInterViewSkipLambdaScale ( UInt  u )       { m_dInterViewSkipLambdaScale = u; }
  Double      getInterViewSkipLambdaScale ()                { return m_dInterViewSkipLambdaScale; }
#endif
#endif

  Bool      isDepthCoder                    ()               { return m_bIsDepth; }

  Void      setSeqStructure( std::string s )                 { m_cSequenceStructure.init( s ); }

  Void      setQP                           ( Int   i )      { m_iQP = i; }

  Void      setTemporalLayerQPOffset        ( Int*  piTemporalLayerQPOffset )      { for ( Int i = 0; i < MAX_TLAYER; i++ ) m_aiTLayerQPOffset[i] = piTemporalLayerQPOffset[i]; }
  Void      setPad                          ( Int*  iPad                   )      { for ( Int i = 0; i < 2; i++ ) m_aiPad[i] = iPad[i]; }

  //======== Transform =============
  Void      setQuadtreeTULog2MaxSize        ( UInt  u )      { m_uiQuadtreeTULog2MaxSize = u; }
  Void      setQuadtreeTULog2MinSize        ( UInt  u )      { m_uiQuadtreeTULog2MinSize = u; }
  Void      setQuadtreeTUMaxDepthInter      ( UInt  u )      { m_uiQuadtreeTUMaxDepthInter = u; }
  Void      setQuadtreeTUMaxDepthIntra      ( UInt  u )      { m_uiQuadtreeTUMaxDepthIntra = u; }

  //====== b; Slice ========

  //====== Entropy Coding ========
  Void      setSymbolMode                   ( Int   i )      { m_iSymbolMode = i; }

  //====== Loop/Deblock Filter ========
  Void      setLoopFilterDisable            ( Bool  b )      { m_bLoopFilterDisable       = b; }
  Void      setLoopFilterAlphaC0Offset      ( Int   i )      { m_iLoopFilterAlphaC0Offset = i; }
  Void      setLoopFilterBetaOffset         ( Int   i )      { m_iLoopFilterBetaOffset    = i; }

  //====== Motion search ========
  Void      setFastSearch                   ( Int   i )      { m_iFastSearch = i; }
  Void      setSearchRange                  ( Int   i )      { m_iSearchRange = i; }
  Void      setBipredSearchRange            ( Int   i )      { m_bipredSearchRange = i; }
  Void      setMaxDeltaQP                   ( Int   i )      { m_iMaxDeltaQP = i; }

#if HHI_VSO
 //==== VSO  ==========
  Void      setVSOMode                      ( UInt  ui )    { m_uiVSOMode   = ui; }
  Void      setForceLambdaScaleVSO          ( Bool  b )     { m_bForceLambdaScale = b; };
  Void      setLambdaScaleVSO               ( Double d )    { m_dLambdaScaleVSO   = d; };
#if HHI_VSO_DIST_INT
  Void      setAllowNegDist                 ( Bool b  )     { m_bAllowNegDist     = b; };
#endif
#endif

  //====== Sequence ========
  Int       getFrameRate                    ()      { return  m_iFrameRate; }
  unsigned int getFrameSkip                 ()      { return  m_FrameSkip; }
  Int       getSourceWidth                  ()      { return  m_iSourceWidth; }
  Int       getSourceHeight                 ()      { return  m_iSourceHeight; }
  Int       getFrameToBeEncoded             ()      { return  m_iFrameToBeEncoded; }

  //==== Coding Structure ========
#if DCM_DECODING_REFRESH
  UInt      getDecodingRefreshType          ()      { return  m_uiDecodingRefreshType; }
#endif
  UInt getCodedPictureBufferSize            ()      { return m_uiCodedPictureStoreSize ;}

  Int       getGOPSize                      ()      { return  m_iGOPSize; }
  Int       getRateGOPSize                  ()      { return  m_iRateGOPSize; }

  Int       getQP                           ()      { return  m_iQP; }

  Int       getTemporalLayerQPOffset        ( Int i )      { assert (i < MAX_TLAYER ); return  m_aiTLayerQPOffset[i]; }
  Int       getPad                          ( Int i )      { assert (i < 2 );                      return  m_aiPad[i]; }

  //======== Transform =============
  UInt      getQuadtreeTULog2MaxSize        ()      const { return m_uiQuadtreeTULog2MaxSize; }
  UInt      getQuadtreeTULog2MinSize        ()      const { return m_uiQuadtreeTULog2MinSize; }
  UInt      getQuadtreeTUMaxDepthInter      ()      const { return m_uiQuadtreeTUMaxDepthInter; }
  UInt      getQuadtreeTUMaxDepthIntra      ()      const { return m_uiQuadtreeTUMaxDepthIntra; }

  //==== b; Slice ========

  //==== Entropy Coding ========
  Int       getSymbolMode                   ()      { return  m_iSymbolMode; }

  //==== Loop/Deblock Filter ========
  Bool      getLoopFilterDisable            ()      { return  m_bLoopFilterDisable;       }
  Int       getLoopFilterAlphaC0Offget      ()      { return  m_iLoopFilterAlphaC0Offset; }
  Int       getLoopFilterBetaOffget         ()      { return  m_iLoopFilterBetaOffset;    }

  //==== Motion search ========
  Int       getFastSearch                   ()      { return  m_iFastSearch; }
  Int       getSearchRange                  ()      { return  m_iSearchRange; }
  Int       getMaxDeltaQP                   ()      { return  m_iMaxDeltaQP; }

#if HHI_INTER_VIEW_MOTION_PRED
  UInt      getMultiviewMvRegMode           ()      { return  m_uiMultiviewMvRegMode; }
  Double    getMultiviewMvRegLambdaScale    ()      { return  m_dMultiviewMvRegLambdaScale; }
#endif

#if SONY_COLPIC_AVAILABILITY
  Int       getViewOrderIdx                 ()      { return  m_iViewOrderIdx; }
#endif

#if HHI_VSO
  //==== VSO  ==========
  UInt      getVSOMode                      ()      { return m_uiVSOMode; }
  Bool      getForceLambdaScaleVSO          ()      { return m_bForceLambdaScale; }
  Double    getLambdaScaleVSO               ()      { return m_dLambdaScaleVSO;   }
#if HHI_VSO_DIST_INT
  Bool      getAllowNegDist                 ()      { return m_bAllowNegDist;     }
#endif
#endif

  //==== Tool list ========
  Void      setUseSBACRD                    ( Bool  b )     { m_bUseSBACRD  = b; }
  Void      setUseASR                       ( Bool  b )     { m_bUseASR     = b; }
  Void      setUseHADME                     ( Bool  b )     { m_bUseHADME   = b; }
  Void      setUseALF                       ( Bool  b )     { m_bUseALF   = b; }
#if DCM_COMB_LIST
  Void      setUseLComb                     ( Bool  b )     { m_bUseLComb   = b; }
  Void      setLCMod                        ( Bool  b )     { m_bLCMod   = b;    }
#endif
  Void      setUseRDOQ                      ( Bool  b )     { m_bUseRDOQ    = b; }
#if !HHI_NO_LowDelayCoding
  Void      setUseLDC                       ( Bool  b )     { m_bUseLDC     = b; }
#endif
  Void      setUsePAD                       ( Bool  b )     { m_bUsePAD     = b; }
  Void      setUseFastEnc                   ( Bool  b )     { m_bUseFastEnc = b; }
#if HHI_VSO
  Void      setUseVSO                       ( Bool  b )     { m_bUseVSO     = b; }
#endif
  Void      setUseMRG                       ( Bool  b )     { m_bUseMRG     = b; } // SOPH:
#if CONSTRAINED_INTRA_PRED
  Void      setUseConstrainedIntraPred      ( Bool  b )     { m_bUseConstrainedIntraPred = b; }
#endif
  Void      setdQPs                         ( Int*  p )     { m_aidQP       = p; }
  Void      setDeltaQpRD                    ( UInt  u )     {m_uiDeltaQpRD  = u; }

  Bool      getUseSBACRD                    ()      { return m_bUseSBACRD;  }
  Bool      getUseASR                       ()      { return m_bUseASR;     }
  Bool      getUseHADME                     ()      { return m_bUseHADME;   }
  Bool      getUseALF                       ()      { return m_bUseALF;     }
#if MQT_ALF_NPASS
  Void      setALFEncodePassReduction       (Int i)  { m_iALFEncodePassReduction = i; }
  Int       getALFEncodePassReduction       ()       { return m_iALFEncodePassReduction; }
#endif
#if DCM_COMB_LIST
  Bool      getUseLComb                     ()      { return m_bUseLComb;   }
  Bool      getLCMod                        ()      { return m_bLCMod; }
#endif
  Bool      getUseRDOQ                      ()      { return m_bUseRDOQ;    }
#if !HHI_NO_LowDelayCoding
  Bool      getUseLDC                       ()      { return m_bUseLDC;     }
#endif
  Bool      getUsePAD                       ()      { return m_bUsePAD;     }
  Bool      getUseFastEnc                   ()      { return m_bUseFastEnc; }

#if HHI_VSO
  Bool      getUseVSO                       ()      { return m_bUseVSO;     }
#endif
  Bool      getUseMRG                       ()      { return m_bUseMRG;     } // SOPH:
#if CONSTRAINED_INTRA_PRED
  Bool      getUseConstrainedIntraPred      ()      { return m_bUseConstrainedIntraPred; }
#endif
#if HHI_DMM_WEDGE_INTRA || HHI_DMM_PRED_TEX
  Void setUseDMM( Bool b) { m_bUseDMM = b;    }
  Bool getUseDMM()        { return m_bUseDMM; }
#endif

#if LM_CHROMA
  Bool getUseLMChroma                       ()      { return m_bUseLMChroma;        }
  Void setUseLMChroma                       ( Bool b ) { m_bUseLMChroma  = b;       }
#endif

  Int*      getdQPs                         ()      { return m_aidQP;       }
  UInt      getDeltaQpRD                    ()      { return m_uiDeltaQpRD; }
#if HHI_RMP_SWITCH
  Void      setUseRMP                      ( Bool b ) { m_bUseRMP = b; }
  Bool      getUseRMP                      ()      {return m_bUseRMP; }
#endif
#ifdef ROUNDING_CONTROL_BIPRED
  Void setUseRoundingControlBipred(Bool b) { m_useRoundingControlBipred = b; }
  Bool getUseRoundingControlBipred() { return m_useRoundingControlBipred; }
#endif
  //====== Slice ========
  Void  setSliceMode                   ( Int  i )       { m_iSliceMode = i;              }
  Void  setSliceArgument               ( Int  i )       { m_iSliceArgument = i;          }
  Int   getSliceMode                   ()              { return m_iSliceMode;           }
  Int   getSliceArgument               ()              { return m_iSliceArgument;       }
  //====== Entropy Slice ========
  Void  setEntropySliceMode            ( Int  i )      { m_iEntropySliceMode = i;       }
  Void  setEntropySliceArgument        ( Int  i )      { m_iEntropySliceArgument = i;   }
  Int   getEntropySliceMode            ()              { return m_iEntropySliceMode;    }
  Int   getEntropySliceArgument        ()              { return m_iEntropySliceArgument;}
#if MTK_NONCROSS_INLOOP_FILTER
  Void      setLFCrossSliceBoundaryFlag     ( Bool   bValue  )    { m_bLFCrossSliceBoundaryFlag = bValue; }
  Bool      getLFCrossSliceBoundaryFlag     ()                    { return m_bLFCrossSliceBoundaryFlag;   }
#endif
#if MTK_SAO
  Void      setUseSAO                  (Bool bVal)     {m_bUseSAO = bVal;}
  Bool      getUseSAO                  ()              {return m_bUseSAO;}
#endif
#if HHI_MPI
  Void      setUseMVI                  (Bool bVal)     {m_bUseMVI = bVal;}
#endif

  void setPictureDigestEnabled(bool b) { m_pictureDigestEnabled = b; }
  bool getPictureDigestEnabled() { return m_pictureDigestEnabled; }

#ifdef WEIGHT_PRED
  Void      setUseWP                        ( Bool  b )   { m_bUseWeightPred    = b;    }
  Void      setWPBiPredIdc                  ( UInt u )    { m_uiBiPredIdc       = u;    }
  Bool      getUseWP                        ()            { return m_bUseWeightPred;    }
  UInt      getWPBiPredIdc                  ()            { return m_uiBiPredIdc;       }
#endif

  Void setQpChangeFrame( PicOrderCnt iPoc ) { m_iQpChangeFrame = iPoc; }
  Int  getQpChangeFrame() { return (Int)m_iQpChangeFrame; }
  Void setQpChangeOffsetVideo( Int iOffset ) { m_iQpChangeOffsetVideo = iOffset; }
  Int  getQpChangeOffsetVideo() { return m_iQpChangeOffsetVideo; }
  Void setQpChangeOffsetDepth( Int iOffset ) { m_iQpChangeOffsetDepth = iOffset; }
  Int  getQpChangeOffsetDepth() { return m_iQpChangeOffsetDepth; }
};

#endif // !defined(AFX_TENCCFG_H__6B99B797_F4DA_4E46_8E78_7656339A6C41__INCLUDED_)

