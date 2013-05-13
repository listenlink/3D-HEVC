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

/** \file     TEncCfg.h
    \brief    encoder configuration class (header)
*/

#ifndef __TENCCFG__
#define __TENCCFG__

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "TLibCommon/CommonDef.h"
#include <assert.h>

struct GOPEntryMvc
{
  Int m_POC;
  Int m_QPOffset;
  Double m_QPFactor;
  Int m_temporalId;
  Bool m_refPic;
  Int m_numRefPicsActive;
  Char m_sliceType;
  Int m_numRefPics;
  Int m_referencePics[MAX_NUM_REF_PICS];
  Int m_usedByCurrPic[MAX_NUM_REF_PICS];
  Bool m_interRPSPrediction;
  Int m_deltaRIdxMinus1;
  Int m_deltaRPS;
  Int m_numRefIdc;
  Int m_refIdc[MAX_NUM_REF_PICS+1];
  Int m_numInterViewRefPics;
  Int m_interViewRefs[MAX_NUM_REF_PICS];
  Int m_interViewRefPosL0[MAX_NUM_REF_PICS];
  Int m_interViewRefPosL1[MAX_NUM_REF_PICS];
 GOPEntryMvc()
  : m_POC(-1)
  , m_QPOffset()
  , m_QPFactor()
  , m_temporalId()
  , m_refPic()
  , m_numRefPicsActive()
  , m_sliceType()
  , m_numRefPics()
  , m_interRPSPrediction()
  , m_deltaRIdxMinus1()
  , m_deltaRPS()
  , m_numRefIdc()
  , m_numInterViewRefPics()
  {
    ::memset( m_referencePics, 0, sizeof(m_referencePics) );
    ::memset( m_usedByCurrPic, 0, sizeof(m_usedByCurrPic) );
    ::memset( m_refIdc,        0, sizeof(m_refIdc) );
    ::memset( m_interViewRefs,   0, sizeof(m_interViewRefs) );
    ::memset( m_interViewRefPosL0, 0, sizeof(m_interViewRefPosL0) );
    ::memset( m_interViewRefPosL1, 0, sizeof(m_interViewRefPosL1) );
  }
};

std::istringstream &operator>>(std::istringstream &in, GOPEntryMvc &entry);

//! \ingroup TLibEncoder
//! \{

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
  Int       m_croppingMode;
  Int       m_cropLeft;
  Int       m_cropRight;
  Int       m_cropTop;
  Int       m_cropBottom;
  Int       m_iFrameToBeEncoded;
  Double    m_adLambdaModifier[ MAX_TLAYER ];

  //====== Coding Structure ========
  UInt      m_uiIntraPeriod;
  UInt      m_uiDecodingRefreshType;            ///< the type of decoding refresh employed for the random access.
  Int       m_iGOPSize;
  GOPEntryMvc m_GOPList[MAX_GOP+1];
  Int       m_extraRPSs;
  Int       m_maxDecPicBuffering[MAX_TLAYER];
  Int       m_numReorderPics[MAX_TLAYER];
  
  Int       m_iQP;                              //  if (AdaptiveQP == OFF)
  
  Int       m_aiTLayerQPOffset[MAX_TLAYER];
  Int       m_aiPad[2];
  

  Int       m_iMaxRefPicNum;                     ///< this is used to mimic the sliding mechanism used by the decoder
                                                 // TODO: We need to have a common sliding mechanism used by both the encoder and decoder

  Int       m_maxTempLayer;                      ///< Max temporal layer
  Bool      m_bDisInter4x4;
  Bool m_useAMP;
  //======= Transform =============
  UInt      m_uiQuadtreeTULog2MaxSize;
  UInt      m_uiQuadtreeTULog2MinSize;
  UInt      m_uiQuadtreeTUMaxDepthInter;
  UInt      m_uiQuadtreeTUMaxDepthIntra;
  
  Bool      m_useNSQT;
  
  //====== Loop/Deblock Filter ========
  Bool      m_bLoopFilterDisable;
  Bool      m_loopFilterOffsetInAPS;
  Int       m_loopFilterBetaOffsetDiv2;
  Int       m_loopFilterTcOffsetDiv2;
  Bool      m_DeblockingFilterControlPresent;
  Bool      m_bUseSAO;
#if LGE_ILLUCOMP_B0045
  Bool      m_bUseIC;
#endif
#if INTER_VIEW_VECTOR_SCALING_C0115
  Bool      m_bUseIVS;
#endif
  Int       m_maxNumOffsetsPerPic;
  Bool      m_saoInterleavingFlag;

  //====== Lossless ========
#if LOSSLESS_CODING
  Bool      m_useLossless;
#endif
  //====== Motion search ========
  Int       m_iFastSearch;                      //  0:Full search  1:Diamond  2:PMVFAST
  Int       m_iSearchRange;                     //  0:Full frame
  Int       m_bipredSearchRange;
#if DV_V_RESTRICTION_B0037
  Bool      m_bUseDisparitySearchRangeRestriction;
  Int       m_iVerticalDisparitySearchRange;
#endif
  //====== Quality control ========
  Int       m_iMaxDeltaQP;                      //  Max. absolute delta QP (1:default)
  Int       m_iMaxCuDQPDepth;                   //  Max. depth for a minimum CuDQP (0:default)

  Int       m_iChromaQpOffset  ;                //  ChromaQpOffset    (0:default)
  Int       m_iChromaQpOffset2nd;               //  ChromaQpOffset2nd (0:default)

#if ADAPTIVE_QP_SELECTION
  Bool      m_bUseAdaptQpSelect;
#endif

  Bool      m_bUseAdaptiveQP;
  Int       m_iQPAdaptationRange;
  
#if HHI_VSO
  //====== View Synthesis Optimization ======
  Bool      m_bUseVSO;
  Bool      m_bForceLambdaScale;
#if HHI_VSO_DIST_INT
  Bool      m_bAllowNegDist;
#endif
  Double    m_dLambdaScaleVSO;
  UInt      m_uiVSOMode;
#if LGE_WVSO_A0119
  Bool      m_bUseWVSO;
  Int       m_iVSOWeight;
  Int       m_iVSDWeight;
  Int       m_iDWeight;
#endif
#endif
#if SAIT_VSO_EST_A0033
  Bool      m_bUseEstimatedVSD; 
  Double    m_dDispCoeff;
#endif


  //====== Tool list ========
  Bool      m_bUseSBACRD;
  Bool      m_bUseALF;
  Int       m_iALFEncodePassReduction;

  Int       m_iALFMaxNumberFilters;
  Bool      m_bALFParamInSlice;
  Bool      m_bALFPicBasedEncode;

  Bool      m_bUseASR;
  Bool      m_bUseHADME;
  Bool      m_bUseLComb;
  Bool      m_bLCMod;
  Bool      m_bUseRDOQ;
  Bool      m_bUseFastEnc;
  Bool      m_bUseEarlyCU;

  Bool      m_useFastDecisionForMerge;
  Bool      m_bUseCbfFastMode;
  Bool      m_bUseLMChroma; 
#if HHI_MPI
  Bool      m_bUseMVI;
#endif
#if RWTH_SDC_DLT_B0036
  Bool      m_bUseDLT;
  Bool      m_bUseSDC;
#endif

  Int*      m_aidQP;
  UInt      m_uiDeltaQpRD;
  
  Bool      m_bUseConstrainedIntraPred;
  Bool      m_usePCM;
  UInt      m_pcmLog2MaxSize;
  UInt      m_uiPCMLog2MinSize;
  //====== Slice ========
  Int       m_iSliceMode;
  Int       m_iSliceArgument; 
  //====== Entropy Slice ========
  Int       m_iEntropySliceMode;
  Int       m_iEntropySliceArgument;
  Int       m_iSliceGranularity;
  Bool      m_bLFCrossSliceBoundaryFlag;

  Bool      m_bPCMInputBitDepthFlag;
  UInt      m_uiPCMBitDepthLuma;
  UInt      m_uiPCMBitDepthChroma;
  Bool      m_bPCMFilterDisableFlag;
  Int       m_iTileBehaviorControlPresentFlag;
  Bool      m_bLFCrossTileBoundaryFlag;
  Int       m_iColumnRowInfoPresent;
  Int       m_iUniformSpacingIdr;
  Int       m_iNumColumnsMinus1;
  UInt*     m_puiColumnWidth;
  Int       m_iNumRowsMinus1;
  UInt*     m_puiRowHeight;
  Int       m_iTileLocationInSliceHeaderFlag; //< enable(1)/disable(0) transmitssion of tile location in slice header

  Int       m_iTileMarkerFlag;              //< enable(1)/disable(0) transmitssion of light weight tile marker
  Int       m_iMaxTileMarkerEntryPoints;    //< maximum number of tile markers allowed in a slice (controls degree of parallelism)
  Double    m_dMaxTileMarkerOffset;         //< Calculated offset. Light weight tile markers will be transmitted for TileIdx= Offset, 2*Offset, 3*Offset ... 

  Int       m_iWaveFrontSynchro;
  Int       m_iWaveFrontFlush;
  Int       m_iWaveFrontSubstreams;

  Bool      m_pictureDigestEnabled; ///< enable(1)/disable(0) md5 computation and SEI signalling

  //====== Weighted Prediction ========
  Bool      m_bUseWeightPred;       //< Use of Weighting Prediction (P_SLICE)
  UInt      m_uiBiPredIdc;          //< Use of Bi-Directional Weighting Prediction (B_SLICE)
  Int       m_useScalingListId;            ///< Using quantization matrix i.e. 0=off, 1=default, 2=file.
  char*     m_scalingListFile;          ///< quantization matrix file name

  Bool      m_bEnableTMVP;
  Int       m_signHideFlag;
  Int       m_signHidingThreshold;

#if VIDYO_VPS_INTEGRATION|QC_MVHEVC_B0046
  UInt     m_layerId;
#endif
  
  Int      m_viewId;
  Bool     m_isDepth;

#if HHI_INTERVIEW_SKIP
  Bool        m_bInterViewSkip;
#if HHI_INTERVIEW_SKIP_LAMBDA_SCALE
  Double      m_dInterViewSkipLambdaScale;
#endif
#endif
#if HHI_DMM_WEDGE_INTRA || HHI_DMM_PRED_TEX
  Bool     m_bUseDMM;
#endif

#if HHI_DMM_PRED_TEX && FLEX_CODING_ORDER_M23723
  Bool m_bUseDMM34;
#endif

#if H3D_QTL
  Bool     m_bUseQTLPC;
#endif

  Int      m_iViewOrderIdx;

  UInt     m_uiCamParPrecision;
  Bool     m_bCamParInSliceHeader;
  Int**    m_aaiCodedScale;
  Int**    m_aaiCodedOffset;

#if DEPTH_MAP_GENERATION
  UInt     m_uiPredDepthMapGeneration;
  UInt     m_uiPdmPrecision;
  Int**    m_aaiPdmScaleNomDelta;
  Int**    m_aaiPdmOffset;
#endif

#if H3D_IVMP
  UInt        m_uiMultiviewMvPredMode;
  UInt        m_uiMultiviewMvRegMode;
  Double      m_dMultiviewMvRegLambdaScale;
#endif
#if H3D_IVRP
#if QC_ARP_D0177
  UInt      m_nUseAdvResPred;
  UInt      m_nARPStepNum;
#else
  UInt        m_uiMultiviewResPredMode;
#endif
#endif

public:
  TEncCfg()          {}
  virtual ~TEncCfg()
  {
    if( m_iUniformSpacingIdr == 0 )
    {
      if( m_iNumColumnsMinus1 )
      { 
        delete[] m_puiColumnWidth; 
        m_puiColumnWidth = NULL;
      }
      if( m_iNumRowsMinus1 )
      {
        delete[] m_puiRowHeight;
        m_puiRowHeight = NULL;
      }
    }
    m_iTileLocationInSliceHeaderFlag = 0;
    m_iTileMarkerFlag              = 0;
  }
  
  Void      setFrameRate                    ( Int   i )      { m_iFrameRate = i; }
  Void      setFrameSkip                    ( unsigned int i ) { m_FrameSkip = i; }
  Void      setSourceWidth                  ( Int   i )      { m_iSourceWidth = i; }
  Void      setSourceHeight                 ( Int   i )      { m_iSourceHeight = i; }
  Void      setCroppingMode                 ( Int   i )      { m_croppingMode = i; }
  Void      setCropLeft                     ( Int   i )      { m_cropLeft = i; }
  Void      setCropRight                    ( Int   i )      { m_cropRight = i; }
  Void      setCropTop                      ( Int   i )      { m_cropTop = i; }
  Void      setCropBottom                   ( Int   i )      { m_cropBottom = i; }
  Void      setFrameToBeEncoded             ( Int   i )      { m_iFrameToBeEncoded = i; }
  
  //====== Coding Structure ========
  Void      setIntraPeriod                  ( Int   i )      { m_uiIntraPeriod = (UInt)i; }
  Void      setDecodingRefreshType          ( Int   i )      { m_uiDecodingRefreshType = (UInt)i; }
  Void      setGOPSize                      ( Int   i )      { m_iGOPSize = i; }
  Void      setGopList                      ( GOPEntryMvc* GOPList ) { for( Int i = 0; i < MAX_GOP+1; i++ ) m_GOPList[i] = GOPList[i]; }
  Void      setExtraRPSs                    ( Int   i )      { m_extraRPSs = i; }
  GOPEntryMvc getGOPEntry                   ( Int   i )      { return m_GOPList[i]; }
  Void      setMaxDecPicBuffering           ( UInt u, UInt tlayer ) { m_maxDecPicBuffering[tlayer] = u;    }
  Void      setNumReorderPics               ( Int  i, UInt tlayer ) { m_numReorderPics[tlayer] = i;    }
  
#if HHI_INTERVIEW_SKIP
  Void      setInterViewSkip            ( UInt  u )       { m_bInterViewSkip         = u; }
  Bool      getInterViewSkip            ( )       { return m_bInterViewSkip  ;}
#if HHI_INTERVIEW_SKIP_LAMBDA_SCALE
  Void      setInterViewSkipLambdaScale ( UInt  u )       { m_dInterViewSkipLambdaScale = u; }
  Double      getInterViewSkipLambdaScale ()                { return m_dInterViewSkipLambdaScale; }
#endif
#endif
  Void      setQP                           ( Int   i )      { m_iQP = i; }
  
  Void      setTemporalLayerQPOffset        ( Int*  piTemporalLayerQPOffset )      { for ( Int i = 0; i < MAX_TLAYER; i++ ) m_aiTLayerQPOffset[i] = piTemporalLayerQPOffset[i]; }
  Void      setPad                          ( Int*  iPad                   )      { for ( Int i = 0; i < 2; i++ ) m_aiPad[i] = iPad[i]; }
  
  Int       getMaxRefPicNum                 ()                              { return m_iMaxRefPicNum;           }
  Void      setMaxRefPicNum                 ( Int iMaxRefPicNum )           { m_iMaxRefPicNum = iMaxRefPicNum;  }

  Bool      getMaxTempLayer                 ()                              { return m_maxTempLayer;              } 
  Void      setMaxTempLayer                 ( Int maxTempLayer )            { m_maxTempLayer = maxTempLayer;      }

  Bool      getDisInter4x4                  ()              { return m_bDisInter4x4;        }
  Void      setDisInter4x4                  ( Bool b )      { m_bDisInter4x4  = b;          }
  Bool      isDepthCoder                    ()               { return m_isDepth; }
  
#if DEPTH_MAP_GENERATION
  Void      setPredDepthMapGeneration       ( UInt  u )      { m_uiPredDepthMapGeneration   = u; }
  Void      setPdmPrecision                 ( UInt  u )      { m_uiPdmPrecision             = u; }
  Void      setPdmScaleNomDelta             ( Int** p )      { m_aaiPdmScaleNomDelta        = p; }
  Void      setPdmOffset                    ( Int** p )      { m_aaiPdmOffset               = p; }
#endif

#if H3D_IVMP
  Void      setMultiviewMvPredMode          ( UInt  u )      { m_uiMultiviewMvPredMode      = u; }
  Void      setMultiviewMvRegMode           ( UInt  u )      { m_uiMultiviewMvRegMode       = u; }
  Void      setMultiviewMvRegLambdaScale    ( Double d)      { m_dMultiviewMvRegLambdaScale = d; }
#endif
#if H3D_IVRP
#if QC_ARP_D0177
  UInt       getUseAdvRP                    ()                  { return m_nUseAdvResPred; }
  Void       setUseAdvRP                    ( UInt  u )         { m_nUseAdvResPred = u;    }
  UInt       getARPStepNum                  ()                  { return m_nARPStepNum;    }
  Void       setARPStepNum                  ( UInt  u )         { m_nARPStepNum = u;       }
#else
  Void      setMultiviewResPredMode         ( UInt  u )      { m_uiMultiviewResPredMode     = u; }
#endif
#endif
  
  //======== Transform =============
  Void      setQuadtreeTULog2MaxSize        ( UInt  u )      { m_uiQuadtreeTULog2MaxSize = u; }
  Void      setQuadtreeTULog2MinSize        ( UInt  u )      { m_uiQuadtreeTULog2MinSize = u; }
  Void      setQuadtreeTUMaxDepthInter      ( UInt  u )      { m_uiQuadtreeTUMaxDepthInter = u; }
  Void      setQuadtreeTUMaxDepthIntra      ( UInt  u )      { m_uiQuadtreeTUMaxDepthIntra = u; }
  
  Void setUseNSQT( Bool b ) { m_useNSQT = b; }
  Void setUseAMP( Bool b ) { m_useAMP = b; }
  
  //====== Loop/Deblock Filter ========
  Void      setLoopFilterDisable            ( Bool  b )      { m_bLoopFilterDisable       = b; }
  Void      setLoopFilterOffsetInAPS        ( Bool  b )      { m_loopFilterOffsetInAPS      = b; }
  Void      setLoopFilterBetaOffset         ( Int   i )      { m_loopFilterBetaOffsetDiv2  = i; }
  Void      setLoopFilterTcOffset           ( Int   i )      { m_loopFilterTcOffsetDiv2    = i; }
  Void      setDeblockingFilterControlPresent ( Bool b ) { m_DeblockingFilterControlPresent = b; }

  //====== Motion search ========
  Void      setFastSearch                   ( Int   i )      { m_iFastSearch = i; }
  Void      setSearchRange                  ( Int   i )      { m_iSearchRange = i; }
  Void      setBipredSearchRange            ( Int   i )      { m_bipredSearchRange = i; }
#if DV_V_RESTRICTION_B0037
  Void      setUseDisparitySearchRangeRestriction ( Bool   b )      { m_bUseDisparitySearchRangeRestriction = b; }
  Void      setVerticalDisparitySearchRange ( Int   i )      { m_iVerticalDisparitySearchRange = i; }
#endif
#if H3D_IVMP
  UInt      getMultiviewMvRegMode           ()      { return  m_uiMultiviewMvRegMode; }
  Double    getMultiviewMvRegLambdaScale    ()      { return  m_dMultiviewMvRegLambdaScale; }
#endif

#if HHI_VSO
 //==== VSO  ==========
  Void      setVSOMode                      ( UInt  ui )    { m_uiVSOMode   = ui; }
  Void      setForceLambdaScaleVSO          ( Bool  b )     { m_bForceLambdaScale = b; };
  Void      setLambdaScaleVSO               ( Double d )    { m_dLambdaScaleVSO   = d; };
#if HHI_VSO_DIST_INT
  Void      setAllowNegDist                 ( Bool b  )     { m_bAllowNegDist     = b; };
#endif
#if LGE_WVSO_A0119
  Void      setUseWVSO                      ( Bool  b )     { m_bUseWVSO   = b; }
  Void      setVSOWeight                    ( Int   i )     { m_iVSOWeight = i; }
  Void      setVSDWeight                    ( Int   i )     { m_iVSDWeight = i; }
  Void      setDWeight                      ( Int   i )     { m_iDWeight   = i; }
#endif
#endif

  //====== Quality control ========
  Void      setMaxDeltaQP                   ( Int   i )      { m_iMaxDeltaQP = i; }
  Void      setMaxCuDQPDepth                ( Int   i )      { m_iMaxCuDQPDepth = i; }

  Void      setChromaQpOffset               ( Int   i ) { m_iChromaQpOffset    = i; }
  Void      setChromaQpOffset2nd            ( Int   i ) { m_iChromaQpOffset2nd = i; }

#if ADAPTIVE_QP_SELECTION
  Void      setUseAdaptQpSelect             ( Bool   i ) { m_bUseAdaptQpSelect    = i; }
  Bool      getUseAdaptQpSelect             ()           { return   m_bUseAdaptQpSelect; }
#endif

  Void      setUseAdaptiveQP                ( Bool  b )      { m_bUseAdaptiveQP = b; }
  Void      setQPAdaptationRange            ( Int   i )      { m_iQPAdaptationRange = i; }
  
  //====== Lossless ========
#if LOSSLESS_CODING
  Void      setUseLossless                  (Bool    b  )        { m_useLossless = b;  }
#endif
  //====== Sequence ========
  Int       getFrameRate                    ()      { return  m_iFrameRate; }
  unsigned int getFrameSkip                 ()      { return  m_FrameSkip; }
  Int       getSourceWidth                  ()      { return  m_iSourceWidth; }
  Int       getSourceHeight                 ()      { return  m_iSourceHeight; }
  Int       getCroppingMode                 ()      { return  m_croppingMode; }
  Int       getCropLeft                     ()      { return  m_cropLeft; }
  Int       getCropRight                    ()      { return  m_cropRight; }
  Int       getCropTop                      ()      { return  m_cropTop; }
  Int       getCropBottom                   ()      { return  m_cropBottom; }
  Int       getFrameToBeEncoded             ()      { return  m_iFrameToBeEncoded; }
  void setLambdaModifier                    ( UInt uiIndex, Double dValue ) { m_adLambdaModifier[ uiIndex ] = dValue; }
  Double getLambdaModifier                  ( UInt uiIndex ) const { return m_adLambdaModifier[ uiIndex ]; }

  //==== Coding Structure ========
  UInt      getIntraPeriod                  ()      { return  m_uiIntraPeriod; }
  UInt      getDecodingRefreshType          ()      { return  m_uiDecodingRefreshType; }
  Int       getGOPSize                      ()      { return  m_iGOPSize; }
  Int       getMaxDecPicBuffering           (UInt tlayer) { return m_maxDecPicBuffering[tlayer]; }
  Int       getNumReorderPics               (UInt tlayer) { return m_numReorderPics[tlayer]; }
  Int       getQP                           ()      { return  m_iQP; }
  
  Int       getTemporalLayerQPOffset        ( Int i )      { assert (i < MAX_TLAYER ); return  m_aiTLayerQPOffset[i]; }
  Int       getPad                          ( Int i )      { assert (i < 2 );                      return  m_aiPad[i]; }
  
  //======== Transform =============
  UInt      getQuadtreeTULog2MaxSize        ()      const { return m_uiQuadtreeTULog2MaxSize; }
  UInt      getQuadtreeTULog2MinSize        ()      const { return m_uiQuadtreeTULog2MinSize; }
  UInt      getQuadtreeTUMaxDepthInter      ()      const { return m_uiQuadtreeTUMaxDepthInter; }
  UInt      getQuadtreeTUMaxDepthIntra      ()      const { return m_uiQuadtreeTUMaxDepthIntra; }
  
  //==== Loop/Deblock Filter ========
  Bool      getLoopFilterDisable            ()      { return  m_bLoopFilterDisable;       }
  Bool      getLoopFilterOffsetInAPS        ()      { return m_loopFilterOffsetInAPS; }
  Int       getLoopFilterBetaOffset         ()      { return m_loopFilterBetaOffsetDiv2; }
  Int       getLoopFilterTcOffset           ()      { return m_loopFilterTcOffsetDiv2; }
  Bool      getDeblockingFilterControlPresent()  { return  m_DeblockingFilterControlPresent; }

  //==== Motion search ========
  Int       getFastSearch                   ()      { return  m_iFastSearch; }
  Int       getSearchRange                  ()      { return  m_iSearchRange; }
#if DV_V_RESTRICTION_B0037
  Bool      getUseDisparitySearchRangeRestriction ()      { return  m_bUseDisparitySearchRangeRestriction; }
  Int       getVerticalDisparitySearchRange ()      { return  m_iVerticalDisparitySearchRange; }
#endif
#if HHI_VSO
  //==== VSO  ==========
  UInt      getVSOMode                      ()      { return m_uiVSOMode; }
  Bool      getForceLambdaScaleVSO          ()      { return m_bForceLambdaScale; }
  Double    getLambdaScaleVSO               ()      { return m_dLambdaScaleVSO;   }
#if HHI_VSO_DIST_INT
  Bool      getAllowNegDist                 ()      { return m_bAllowNegDist;     }
#endif
#if LGE_WVSO_A0119
  Bool      getUseWVSO                      ()      { return m_bUseWVSO;     }
  Int       getVSOWeight                    ()      { return m_iVSOWeight;    }
  Int       getVSDWeight                    ()      { return m_iVSDWeight;    }
  Int       getDWeight                      ()      { return m_iDWeight;    }
#endif
#endif

  //==== Quality control ========
  Int       getMaxDeltaQP                   ()      { return  m_iMaxDeltaQP; }
  Int       getMaxCuDQPDepth                ()      { return  m_iMaxCuDQPDepth; }
  Bool      getUseAdaptiveQP                ()      { return  m_bUseAdaptiveQP; }
  Int       getQPAdaptationRange            ()      { return  m_iQPAdaptationRange; }
  //====== Lossless ========
#if LOSSLESS_CODING
  Bool      getUseLossless                  ()      { return  m_useLossless;  }
#endif
  
  //==== Tool list ========
  Void      setUseSBACRD                    ( Bool  b )     { m_bUseSBACRD  = b; }
  Void      setUseASR                       ( Bool  b )     { m_bUseASR     = b; }
  Void      setUseHADME                     ( Bool  b )     { m_bUseHADME   = b; }
  Void      setUseALF                       ( Bool  b )     { m_bUseALF   = b; }
  Void      setUseLComb                     ( Bool  b )     { m_bUseLComb   = b; }
  Void      setLCMod                        ( Bool  b )     { m_bLCMod   = b;    }
  Void      setUseRDOQ                      ( Bool  b )     { m_bUseRDOQ    = b; }
#if HHI_VSO
  Void      setUseVSO                       ( Bool  b )     { m_bUseVSO     = b; }
#endif

#if SAIT_VSO_EST_A0033
  Void      setUseEstimatedVSD              ( Bool  b )     { m_bUseEstimatedVSD = b; }
  Void      setDispCoeff                    ( Double  d )   { m_dDispCoeff  = d; }
#endif

  Void      setUseFastEnc                   ( Bool  b )     { m_bUseFastEnc = b; }
  Void      setUseEarlyCU                   ( Bool  b )     { m_bUseEarlyCU = b; }
  Void      setUseFastDecisionForMerge      ( Bool  b )     { m_useFastDecisionForMerge = b; }
  Void      setUseCbfFastMode            ( Bool  b )     { m_bUseCbfFastMode = b; }
  Void      setUseConstrainedIntraPred      ( Bool  b )     { m_bUseConstrainedIntraPred = b; }
  Void      setPCMInputBitDepthFlag         ( Bool  b )     { m_bPCMInputBitDepthFlag = b; }
  Void      setPCMFilterDisableFlag         ( Bool  b )     {  m_bPCMFilterDisableFlag = b; }
  Void      setUsePCM                       ( Bool  b )     {  m_usePCM = b;               }
  Void      setPCMLog2MaxSize               ( UInt u )      { m_pcmLog2MaxSize = u;      }
  Void      setPCMLog2MinSize               ( UInt u )     { m_uiPCMLog2MinSize = u;      }
  Void      setdQPs                         ( Int*  p )     { m_aidQP       = p; }
  Void      setDeltaQpRD                    ( UInt  u )     {m_uiDeltaQpRD  = u; }
  Bool      getUseSBACRD                    ()      { return m_bUseSBACRD;  }
  Bool      getUseASR                       ()      { return m_bUseASR;     }
  Bool      getUseHADME                     ()      { return m_bUseHADME;   }
  Bool      getUseALF                       ()      { return m_bUseALF;     }
  Void      setALFEncodePassReduction       (Int i)  { m_iALFEncodePassReduction = i; }
  Int       getALFEncodePassReduction       ()       { return m_iALFEncodePassReduction; }

  Void      setALFMaxNumberFilters          (Int i)  { m_iALFMaxNumberFilters = i; } 
  Int       getALFMaxNumberFilters          ()       { return m_iALFMaxNumberFilters; } 
  Void      setALFParamInSlice              (Bool b) {m_bALFParamInSlice = b;}
  Bool      getALFParamInSlice              ()       {return m_bALFParamInSlice;}
  Void      setALFPicBasedEncode            (Bool b) {m_bALFPicBasedEncode = b;}
  Bool      getALFPicBasedEncode            ()       {return m_bALFPicBasedEncode;}

  Bool      getUseLComb                     ()      { return m_bUseLComb;   }
  Bool      getLCMod                        ()      { return m_bLCMod; }
  Bool      getUseRDOQ                      ()      { return m_bUseRDOQ;    }

#if HHI_VSO
  Bool      getUseVSO                       ()      { return m_bUseVSO;     }
#endif
#if SAIT_VSO_EST_A0033
  Bool      getUseEstimatedVSD              ()      { return m_bUseEstimatedVSD; }
  Double    getDispCoeff                    ()      { return m_dDispCoeff;    }
#endif

  Bool      getUseFastEnc                   ()      { return m_bUseFastEnc; }
  Bool      getUseEarlyCU                   ()      { return m_bUseEarlyCU; }
  Bool      getUseFastDecisionForMerge      ()      { return m_useFastDecisionForMerge; }
  Bool      getUseCbfFastMode           ()      { return m_bUseCbfFastMode; }
  Bool      getUseConstrainedIntraPred      ()      { return m_bUseConstrainedIntraPred; }
#if NS_HAD
  Bool      getUseNSQT                      ()      { return m_useNSQT; }
#endif
  Bool      getPCMInputBitDepthFlag         ()      { return m_bPCMInputBitDepthFlag;   } 
  Bool      getPCMFilterDisableFlag         ()      { return m_bPCMFilterDisableFlag;   } 
  Bool      getUsePCM                       ()      { return m_usePCM;                 }
  UInt      getPCMLog2MaxSize               ()      { return m_pcmLog2MaxSize;  }
  UInt      getPCMLog2MinSize               ()      { return  m_uiPCMLog2MinSize;  }

  Bool getUseLMChroma                       ()      { return m_bUseLMChroma;        }
  Void setUseLMChroma                       ( Bool b ) { m_bUseLMChroma  = b;       }

  Int*      getdQPs                         ()      { return m_aidQP;       }
  UInt      getDeltaQpRD                    ()      { return m_uiDeltaQpRD; }

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
  Void  setSliceGranularity            ( Int  i )      { m_iSliceGranularity = i;       }
  Int   getSliceGranularity            ()              { return m_iSliceGranularity;    }
  Void  setLFCrossSliceBoundaryFlag    ( Bool   bValue  )    { m_bLFCrossSliceBoundaryFlag = bValue; }
  Bool  getLFCrossSliceBoundaryFlag    ()                    { return m_bLFCrossSliceBoundaryFlag;   }

#if HHI_MPI
  Void  setUseMVI                      ( Bool bVal )   {m_bUseMVI = bVal;}
#endif
#if RWTH_SDC_DLT_B0036
  Void  setUseDLT                       ( Bool  b )     { m_bUseDLT   = b; }
  Void  setUseSDC                       ( Bool  b )     { m_bUseSDC   = b; }
#endif
  Void  setUseSAO                      ( Bool bVal )   {m_bUseSAO = bVal;}
  Bool  getUseSAO                      ()              {return m_bUseSAO;}
#if LGE_ILLUCOMP_B0045
  Void  setUseIC                       ( Bool bVal )   {m_bUseIC = bVal;}
  Bool  getUseIC                       ()              {return m_bUseIC;}
#endif
#if INTER_VIEW_VECTOR_SCALING_C0115
  Void  setUseIVS                       ( Bool bVal )   {m_bUseIVS = bVal;}
  Bool  getUseIVS                       ()              {return m_bUseIVS;}
#endif
  Void  setMaxNumOffsetsPerPic                   (Int iVal)            { m_maxNumOffsetsPerPic = iVal; }
  Int   getMaxNumOffsetsPerPic                   ()                    { return m_maxNumOffsetsPerPic; }
  Void  setSaoInterleavingFlag                   (bool bVal)           { m_saoInterleavingFlag = bVal; }
  Bool  getSaoInterleavingFlag                   ()                    { return m_saoInterleavingFlag; }
  Void  setTileBehaviorControlPresentFlag        ( Int i )             { m_iTileBehaviorControlPresentFlag = i;    }
  Int   getTileBehaviorControlPresentFlag        ()                    { return m_iTileBehaviorControlPresentFlag; }
  Void  setLFCrossTileBoundaryFlag               ( Bool   bValue  )    { m_bLFCrossTileBoundaryFlag = bValue; }
  Bool  getLFCrossTileBoundaryFlag               ()                    { return m_bLFCrossTileBoundaryFlag;   }
  Void  setColumnRowInfoPresent        ( Int i )           { m_iColumnRowInfoPresent = i; }
  Int   getColumnRowInfoPresent        ()                  { return m_iColumnRowInfoPresent; }
  Void  setUniformSpacingIdr           ( Int i )           { m_iUniformSpacingIdr = i; }
  Int   getUniformSpacingIdr           ()                  { return m_iUniformSpacingIdr; }
  Void  setNumColumnsMinus1            ( Int i )           { m_iNumColumnsMinus1 = i; }
  Int   getNumColumnsMinus1            ()                  { return m_iNumColumnsMinus1; }
  Void  setColumnWidth ( char* str )
  {
    char *columnWidth;
    int  i=0;
    Int  m_iWidthInCU = ( m_iSourceWidth%g_uiMaxCUWidth ) ? m_iSourceWidth/g_uiMaxCUWidth + 1 : m_iSourceWidth/g_uiMaxCUWidth;

    if( m_iUniformSpacingIdr == 0 && m_iNumColumnsMinus1 > 0 )
    {
      m_puiColumnWidth = new UInt[m_iNumColumnsMinus1];

      columnWidth = strtok(str, " ,-");
      while(columnWidth!=NULL)
      {
        if( i>=m_iNumColumnsMinus1 )
        {
          printf( "The number of columns whose width are defined is larger than the allowed number of columns.\n" );
          exit( EXIT_FAILURE );
        }
        *( m_puiColumnWidth + i ) = atoi( columnWidth );
        printf("col: m_iWidthInCU= %4d i=%4d width= %4d\n",m_iWidthInCU,i,m_puiColumnWidth[i]); //AFU
        columnWidth = strtok(NULL, " ,-");
        i++;
      }
      if( i<m_iNumColumnsMinus1 )
      {
        printf( "The width of some columns is not defined.\n" );
        exit( EXIT_FAILURE );
      }
    }
  }
  UInt  getColumnWidth                 ( UInt columnidx )  { return *( m_puiColumnWidth + columnidx ); }
  Void  setNumRowsMinus1               ( Int i )           { m_iNumRowsMinus1 = i; }
  Int   getNumRowsMinus1               ()                  { return m_iNumRowsMinus1; }
  Void  setRowHeight (char* str)
  {
    char *rowHeight;
    int  i=0;
    Int  m_iHeightInCU = ( m_iSourceHeight%g_uiMaxCUHeight ) ? m_iSourceHeight/g_uiMaxCUHeight + 1 : m_iSourceHeight/g_uiMaxCUHeight;

    if( m_iUniformSpacingIdr == 0 && m_iNumRowsMinus1 > 0 )
    {
      m_puiRowHeight = new UInt[m_iNumRowsMinus1];

      rowHeight = strtok(str, " ,-");
      while(rowHeight!=NULL)
      {
        if( i>=m_iNumRowsMinus1 )
        {
          printf( "The number of rows whose height are defined is larger than the allowed number of rows.\n" );
          exit( EXIT_FAILURE );
        }
        *( m_puiRowHeight + i ) = atoi( rowHeight );
        printf("row: m_iHeightInCU=%4d i=%4d height=%4d\n",m_iHeightInCU,i,m_puiRowHeight[i]); //AFU
        rowHeight = strtok(NULL, " ,-");
        i++;
      }
      if( i<m_iNumRowsMinus1 )
      {
        printf( "The height of some rows is not defined.\n" );
        exit( EXIT_FAILURE );
     }
    }
  }
  UInt  getRowHeight                   ( UInt rowIdx )     { return *( m_puiRowHeight + rowIdx ); }
  Void  xCheckGSParameters();
  Int  getTileLocationInSliceHeaderFlag ()                 { return m_iTileLocationInSliceHeaderFlag; }
  Void setTileLocationInSliceHeaderFlag ( Int iFlag )      { m_iTileLocationInSliceHeaderFlag = iFlag;}
  Int  getTileMarkerFlag              ()                 { return m_iTileMarkerFlag;              }
  Void setTileMarkerFlag              ( Int iFlag )      { m_iTileMarkerFlag = iFlag;             }
  Int  getMaxTileMarkerEntryPoints    ()                 { return m_iMaxTileMarkerEntryPoints;    }
  Void setMaxTileMarkerEntryPoints    ( Int iCount )     { m_iMaxTileMarkerEntryPoints = iCount;  }
  Double getMaxTileMarkerOffset       ()                 { return m_dMaxTileMarkerOffset;         }
  Void setMaxTileMarkerOffset         ( Double dCount )  { m_dMaxTileMarkerOffset = dCount;       }
  Void  setWaveFrontSynchro(Int iWaveFrontSynchro)       { m_iWaveFrontSynchro = iWaveFrontSynchro; }
  Int   getWaveFrontsynchro()                            { return m_iWaveFrontSynchro; }
  Void  setWaveFrontFlush(Int iWaveFrontFlush)           { m_iWaveFrontFlush = iWaveFrontFlush; }
  Int   getWaveFrontFlush()                              { return m_iWaveFrontFlush; }
  Void  setWaveFrontSubstreams(Int iWaveFrontSubstreams) { m_iWaveFrontSubstreams = iWaveFrontSubstreams; }
  Int   getWaveFrontSubstreams()                         { return m_iWaveFrontSubstreams; }
  void setPictureDigestEnabled(bool b) { m_pictureDigestEnabled = b; }
  bool getPictureDigestEnabled() { return m_pictureDigestEnabled; }

  Void      setUseWP               ( Bool  b )   { m_bUseWeightPred    = b;    }
  Void      setWPBiPredIdc         ( UInt u )    { m_uiBiPredIdc       = u;    }
  Bool      getUseWP               ()            { return m_bUseWeightPred;    }
  UInt      getWPBiPredIdc         ()            { return m_uiBiPredIdc;       }
  Void      setUseScalingListId    ( Int  u )    { m_useScalingListId       = u;   }
  Int       getUseScalingListId    ()            { return m_useScalingListId;      }
  Void      setScalingListFile     ( char*  pch ){ m_scalingListFile     = pch; }
  char*     getScalingListFile     ()            { return m_scalingListFile;    }

  Void      setEnableTMVP          ( Bool b )    { m_bEnableTMVP = b;    }
  Bool      getEnableTMVP          ()            { return m_bEnableTMVP; }
  Void      setSignHideFlag( Int signHideFlag )  { m_signHideFlag = signHideFlag; }
  Void      setTSIG( Int tsig )                  { m_signHidingThreshold = tsig; }
  Int       getSignHideFlag()                    { return m_signHideFlag; }
  Int       getTSIG()                            { return m_signHidingThreshold; }
#if VIDYO_VPS_INTEGRATION |QC_MVHEVC_B0046
  Void      setLayerId             ( UInt layerId )   { m_layerId = layerId; }
  UInt      getLayerId             ()               { return m_layerId; }
#endif

  Void      setViewId             ( Int viewId )   { m_viewId = viewId; }
  Int       getViewId             ()               { return m_viewId; }
  Void      setIsDepth            ( Bool isDepth ) { m_isDepth = isDepth; }
  Bool      getIsDepth            ()               { return m_isDepth; }

#if HHI_DMM_WEDGE_INTRA || HHI_DMM_PRED_TEX
  Void      setUseDMM( Bool b) { m_bUseDMM = b;    }
  Bool      getUseDMM()        { return m_bUseDMM; }
#endif

#if HHI_DMM_PRED_TEX && FLEX_CODING_ORDER_M23723
  Void setUseDMM34( Bool b) { m_bUseDMM34 = b;    }
  Bool getUseDMM34()        { return m_bUseDMM34; }
#endif

#if H3D_QTL
  Void      setUseQTLPC( Bool b ) { m_bUseQTLPC = b;    }
  Bool      getUseQTLPC()         { return m_bUseQTLPC; }
#endif
  
#if RWTH_SDC_DLT_B0036
  Bool      getUseDLT()      { return m_bUseDLT;     }
  Bool      getUseSDC()      { return m_bUseSDC;     }
#endif

  Void      setViewOrderIdx       ( Int   i )      { m_iViewOrderIdx          = i; }
  Void      setCamParPrecision              ( UInt  u )      { m_uiCamParPrecision      = u; }
  Void      setCamParInSliceHeader          ( Bool  b )      { m_bCamParInSliceHeader   = b; }
  Void      setCodedScale                   ( Int** p )      { m_aaiCodedScale          = p; }
  Void      setCodedOffset                  ( Int** p )      { m_aaiCodedOffset         = p; }

#if INTER_VIEW_VECTOR_SCALING_C0115
  Int       getViewOrderIdx                 ()      { return  m_iViewOrderIdx; }    // will be changed to view_id
#endif

};

//! \}

#endif // !defined(AFX_TENCCFG_H__6B99B797_F4DA_4E46_8E78_7656339A6C41__INCLUDED_)
