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



/** \file     TAppEncCfg.h
    \brief    Handle encoder configuration parameters (header)
*/

#ifndef __TAPPENCCFG__
#define __TAPPENCCFG__

#include "../../Lib/TLibCommon/CommonDef.h"
#include "../../Lib/TLibCommon/TComMVDRefData.h"
#include "../../App/TAppCommon/TAppComCamPara.h"
#include "../../Lib/TLibRenderer/TRenTop.h"
#include "../../Lib/TLibRenderer/TRenModel.h"
#include "../../Lib/TLibRenderer/TRenModSetupStrParser.h"

#include <string>
#include <vector>

// ====================================================================================================================
// Class definition
// ====================================================================================================================

/// encoder configuration class
class TAppEncCfg
{
protected:
  // file I/O
  char*     m_pchBitstreamFile;                               ///< output bitstream file

  std::vector<char*>     m_pchInputFileList;                  ///< source file names
  std::vector<char*>     m_pchDepthInputFileList;             ///< source depth file names
  std::vector<char*>     m_pchReconFileList;                  ///< output reconstruction file names
  std::vector<char*>     m_pchDepthReconFileList;             ///< output depth reconstruction file names

  std::vector<char*>     m_pchERRefFileList;                  ///< virtual external reference view files names

  // source specification
  Int       m_iFrameRate;                                     ///< source frame-rates (Hz)
  unsigned int m_FrameSkip;                                   ///< number of skipped frames from the beginning
  Int       m_iSourceWidth;                                   ///< source width in pixel
  Int       m_iSourceHeight;                                  ///< source height in pixel
  Int       m_iFrameToBeEncoded;                              ///< number of encoded frames
  Bool      m_bUsePAD;                                        ///< flag for using source padding
  Int       m_aiPad[2];                                       ///< number of padded pixels for width and height

  Int       m_iNumberOfViews;                                ///< number Views to Encode
  Bool      m_bUsingDepthMaps ;


  // coding structure
#if DCM_DECODING_REFRESH
  Int       m_iDecodingRefreshType;                           ///< random access type
#endif
  UInt      m_uiCodedPictureStoreSize ;
  Int       m_iGOPSize;                                       ///< GOP size of hierarchical structure
  Int       m_iRateGOPSize;                                   ///< GOP size for QP variance
#if !HHI_NO_LowDelayCoding
  Bool      m_bUseLDC;                                        ///< flag for using low-delay coding mode
#endif
#if DCM_COMB_LIST
  Bool      m_bUseLComb;                                      ///< flag for using combined reference list for uni-prediction in B-slices (JCTVC-D421)
  Bool      m_bLCMod;                                         ///< flag for specifying whether the combined reference list for uni-prediction in B-slices is uploaded explicitly
#endif
  std::string     m_cInputFormatString ;                            // GOP string
  // coding quality

  std::vector<Double>  m_adQP;                                ///< QP value of key-picture (floating point) [0] video, [1] depth
  std::vector<Int>     m_aiQP;                                ///< QP value of key-picture (integer)

  Int       m_aiTLayerQPOffset[MAX_TLAYER];                   ///< QP offset corresponding to temporal layer depth
  char*     m_pchdQPFile;                                     ///< QP offset for each slice (initialized from external file)
  Int*      m_aidQP;                                          ///< array of slice QP values
  Int       m_iMaxDeltaQP;                                    ///< max. |delta QP|
  UInt      m_uiDeltaQpRD;                                    ///< dQP range for multi-pass slice QP optimization

  // coding unit (CU) definition
  UInt      m_uiMaxCUWidth;                                   ///< max. CU width in pixel
  UInt      m_uiMaxCUHeight;                                  ///< max. CU height in pixel
  UInt      m_uiMaxCUDepth;                                   ///< max. CU depth

  // transfom unit (TU) definition
  UInt      m_uiQuadtreeTULog2MaxSize;
  UInt      m_uiQuadtreeTULog2MinSize;

  UInt      m_uiQuadtreeTUMaxDepthInter;
  UInt      m_uiQuadtreeTUMaxDepthIntra;

  // coding tools (bit-depth)
  UInt      m_uiInputBitDepth;                                ///< bit-depth of input file
  UInt      m_uiOutputBitDepth;                               ///< bit-depth of output file
#ifdef ENABLE_IBDI
  UInt      m_uiBitIncrement;                                 ///< bit-depth increment
#endif
  UInt      m_uiInternalBitDepth;                             ///< Internal bit-depth (BitDepth+BitIncrement)

#if MTK_SAO
  vector<Bool> m_abUseSAO;
#endif

  // coding tools (loop filter)
  vector<Bool> m_abUseALF;                                    ///< flag for using adaptive loop filter [0] - video, [1] - depth
#ifdef MQT_ALF_NPASS
  Int       m_iALFEncodePassReduction;                        ///< ALF encoding pass, 0 = original 16-pass, 1 = 1-pass, 2 = 2-pass
#endif

  vector<Bool> m_abLoopFilterDisable;                         ///< flag for using deblocking filter filter [0] - video, [1] - depth
  Int       m_iLoopFilterAlphaC0Offset;                       ///< alpha offset for deblocking filter
  Int       m_iLoopFilterBetaOffset;                          ///< beta offset for deblocking filter

  // coding tools (entropy coder)
  Int       m_iSymbolMode;                                    ///< entropy coder mode, 0 = VLC, 1 = CABAC

  // coding tools (inter - merge motion partitions)
  Bool      m_bUseMRG;                                        ///< SOPH: flag for using motion partition Merge Mode

#if LM_CHROMA
  Bool      m_bUseLMChroma;                                  ///< JL: Chroma intra prediction based on luma signal
#endif

#if HHI_RMP_SWITCH
  Bool      m_bUseRMP;
#endif

  // coding tools (encoder-only parameters)
  Bool      m_bUseSBACRD;                                     ///< flag for using RD optimization based on SBAC
  Bool      m_bUseASR;                                        ///< flag for using adaptive motion search range
  Bool      m_bUseHADME;                                      ///< flag for using HAD in sub-pel ME
  vector<Bool> m_abUseRDOQ;                                   ///< flag for using RD optimized quantization [0]-video, [1]-depth
  Int       m_iFastSearch;                                    ///< ME mode, 0 = full, 1 = diamond, 2 = PMVFAST
  Int       m_iSearchRange;                                   ///< ME search range
  Int       m_bipredSearchRange;                              ///< ME search range for bipred refinement
  Bool      m_bUseFastEnc;                                    ///< flag for using fast encoder setting

#if DEPTH_MAP_GENERATION
  UInt      m_uiPredDepthMapGeneration;                       ///< using of (virtual) depth maps for texture coding
#endif
#if HHI_INTER_VIEW_MOTION_PRED
  UInt      m_uiMultiviewMvPredMode;                          ///< usage of predictors for multi-view mv prediction
  UInt      m_uiMultiviewMvRegMode;                           ///< regularization for multiview motion vectors
  Double    m_dMultiviewMvRegLambdaScale;                     ///< lambda scale for multiview motion vectors regularization
#endif
#if HHI_INTER_VIEW_RESIDUAL_PRED
  UInt      m_uiMultiviewResPredMode;                         ///< using multiview residual prediction
#endif

#if HHI_INTERVIEW_SKIP
  UInt      m_uiInterViewSkip;                            ///< usage of interview skip mode ( do not transmit residual)
#if HHI_INTERVIEW_SKIP_LAMBDA_SCALE
  Double    m_dInterViewSkipLambdaScale;                 ///< lambda scale for interview skip
#endif
#endif

  // camera parameter
  Char*     m_pchCameraParameterFile;                         ///< camera parameter file
  Char*     m_pchBaseViewCameraNumbers;
  TAppComCamPara m_cCameraData;

  Int       m_iCodedCamParPrecision;                          ///< precision for coding of camera parameters

#if HHI_INTERVIEW_SKIP
  TRenTop  m_cUsedPelsRenderer;                               ///< renderer for used pels map
#endif

#if HHI_VSO
  Char*     m_pchVSOConfig;
  Bool      m_bUseVSO;                                    ///< flag for using View Synthesis Optimization

  //// Used for development by GT, might be removed later
  Double    m_dLambdaScaleVSO;                            ///< Scaling factor for Lambda in VSO mode
  Bool      m_bForceLambdaScaleVSO;                       ///< Use Lambda Scale for depth even if VSO is turned off
#if HHI_VSO_DIST_INT
  Bool      m_bAllowNegDist;                              ///< Allow negative distortion in VSO
#endif
  UInt      m_uiVSOMode;                                  ///< Number of VSO Mode, 1 = , 2 = simple, org vs. ren, 3 = simple, ren vs. ren, 4 = full
  Int       m_iNumberOfExternalRefs;                      ///< number Virtual External Reference Views
  std::vector< std::vector<Int> > m_aaiBaseViewRefInd;    ///< View numbers of Base View References
  std::vector< std::vector<Int> > m_aaiERViewRefInd;      ///< View numbers of External ViewReferences
  std::vector< std::vector<Int> > m_aaiERViewRefLutInd;   ///< Indices of LUTs used for External View References
#endif

  Int       m_iSliceMode;           ///< 0: Disable all Recon slice limits, 1 : Maximum number of largest coding units per slice, 2: Maximum number of bytes in a slice
  Int       m_iSliceArgument;       ///< If m_iSliceMode==1, m_iSliceArgument=max. # of largest coding units. If m_iSliceMode==2, m_iSliceArgument=max. # of bytes.
  Int       m_iEntropySliceMode;    ///< 0: Disable all entropy slice limits, 1 : Maximum number of largest coding units per slice, 2: Constraint based entropy slice
  Int       m_iEntropySliceArgument;///< If m_iEntropySliceMode==1, m_iEntropySliceArgument=max. # of largest coding units. If m_iEntropySliceMode==2, m_iEntropySliceArgument=max. # of bins.

#if MTK_NONCROSS_INLOOP_FILTER
  Bool m_bLFCrossSliceBoundaryFlag;  ///< 0: Cross-slice-boundary in-loop filtering 1: non-cross-slice-boundary in-loop filtering
#endif
#ifdef ROUNDING_CONTROL_BIPRED
  Bool m_useRoundingControlBipred;
#endif
#if CONSTRAINED_INTRA_PRED
  Bool      m_bUseConstrainedIntraPred;                       ///< flag for using constrained intra prediction
#endif
#if HHI_DMM_WEDGE_INTRA || HHI_DMM_PRED_TEX
  Bool      m_bUseDMM;
#endif
#if HHI_MPI
  Bool      m_bUseMVI;  ///< flag for using Motion Vector Inheritance for depth map coding
#endif

  PicOrderCnt m_iQpChangeFrame;
  Int         m_iQpChangeOffsetVideo;
  Int         m_iQpChangeOffsetDepth;

  bool m_pictureDigestEnabled; ///< enable(1)/disable(0) md5 computation and SEI signalling

  //====== Weighted Prediction ========
#ifdef WEIGHT_PRED
  Bool                    m_bUseWeightPred;                   ///< Use of explicit Weighting Prediction for P_SLICE
  UInt                    m_uiBiPredIdc;                      ///< Use of Bi-Directional Weighting Prediction (B_SLICE): explicit(1) or implicit(2)
#endif

  // internal member functions
  Void  xSetGlobal      ();                                   ///< set global variables
  Void  xCheckParameter ();                                   ///< check validity of configuration values
  Void  xPrintParameter ();                                   ///< print configuration values
  Void  xPrintUsage     ();                                   ///< print usage

  Void  xCleanUpVectors ();                                   ///< clean up vector sizes
  Void  xInitCameraPars ();                                   ///< init camera parameters


  // set MVD Parameters and LUTs
  Void xSetShiftParameters();
  Void xGetShiftParameter( UInt uiSourceView, UInt uiTargetView, bool bExternal, double& rdScale, double& rdOffset ); ///< Get one Shift Parameters

  // util
  Void  xAppendToFileNameEnd( Char* pchInputFileName, const Char* pchStringToAppend, Char* & rpchOutputFileName);
  Bool  xConfirmParameter(Bool bflag, const char* message);


  template <class T> Void xCleanUpVector( std::vector<T>& rcVec, const T& rcInvalid );

#if HHI_VSO
  // Ren Model String
  TRenModSetupStrParser       m_cRenModStrParser;
#endif
public:

  TAppEncCfg();
  virtual ~TAppEncCfg();

public:
  Void  create    ();                                         ///< create option handling class
  Void  destroy   ();                                         ///< destroy option handling class
  Bool  parseCfg  ( Int argc, Char* argv[] );                 ///< parse configuration file to fill member variables

};// END CLASS DEFINITION TAppEncCfg

#endif // __TAPPENCCFG__

