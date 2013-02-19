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

/** \file     TAppEncCfg.h
    \brief    Handle encoder configuration parameters (header)
*/

#ifndef __TAPPENCCFG__
#define __TAPPENCCFG__

#include "TLibCommon/CommonDef.h"

#include "TLibEncoder/TEncCfg.h"
#include "TAppCommon/TAppComCamPara.h"
#include "TLibRenderer/TRenTop.h"
#include "TLibRenderer/TRenModel.h"
#include "TLibRenderer/TRenModSetupStrParser.h"

#include <sstream>
#include <vector>

//! \ingroup TAppEncoder
//! \{

// ====================================================================================================================
// Class definition
// ====================================================================================================================

/// encoder configuration class
class TAppEncCfg
{
protected:
  // file I/O
  std::vector<char*>     m_pchInputFileList;                  ///< source file names
  std::vector<char*>     m_pchDepthInputFileList;             ///< source depth file names
  std::vector<char*>     m_pchReconFileList;                  ///< output reconstruction file names
  std::vector<char*>     m_pchDepthReconFileList;             ///< output depth reconstruction file names
  char*     m_pchBitstreamFile;                               ///< output bitstream file
  Double    m_adLambdaModifier[ MAX_TLAYER ];                 ///< Lambda modifier array for each temporal layer
  // source specification
  Int       m_iFrameRate;                                     ///< source frame-rates (Hz)
  unsigned int m_FrameSkip;                                   ///< number of skipped frames from the beginning
  Int       m_iSourceWidth;                                   ///< source width in pixel
  Int       m_iSourceHeight;                                  ///< source height in pixel
  Int       m_croppingMode;
  Int       m_cropLeft;
  Int       m_cropRight;
  Int       m_cropTop;
  Int       m_cropBottom;
  Int       m_iFrameToBeEncoded;                              ///< number of encoded frames
  Int       m_aiPad[2];                                       ///< number of padded pixels for width and height
  
  Int       m_iNumberOfViews;                                 ///< number Views to Encode
  Bool      m_bUsingDepthMaps;
  
#if FLEX_CODING_ORDER_M23723
  Char*  m_pchMVCJointCodingOrder;      ///<  texture-depth coding order
  Bool    m_b3DVFlexOrder;    ///<  flexible coding order flag
#endif

  // coding structure
  Int       m_iIntraPeriod;                                   ///< period of I-slice (random access period)
  Int       m_iDecodingRefreshType;                           ///< random access type
  Int       m_iGOPSize;                                       ///< GOP size of hierarchical structure
  Int       m_extraRPSs[MAX_VIEW_NUM];
  GOPEntryMvc m_GOPListsMvc[MAX_VIEW_NUM][MAX_GOP+1];
  Int       m_numReorderPics[MAX_VIEW_NUM][MAX_TLAYER];       ///< total number of reorder pictures
  Int       m_maxDecPicBuffering[MAX_VIEW_NUM][MAX_TLAYER];   ///< total number of reference pictures needed for decoding
  Bool      m_bUseLComb;                                      ///< flag for using combined reference list for uni-prediction in B-slices (JCTVC-D421)
  Bool      m_bLCMod;                                         ///< flag for specifying whether the combined reference list for uni-prediction in B-slices is uploaded explicitly
  Bool      m_bDisInter4x4;
  Bool      m_enableNSQT;                                     ///< flag for enabling NSQT
  Bool      m_enableAMP;
  // coding quality
  std::vector<Double>  m_adQP;                                ///< QP value of key-picture (floating point) [0] video, [1] depth
  std::vector<Int>     m_aiQP;                                ///< QP value of key-picture (integer) [0] video, [1] depth
#if QC_MVHEVC_B0046
  std::vector<Int>     m_aiVId;                                ///< view id
#endif
  Int       m_aiTLayerQPOffset[MAX_TLAYER];                   ///< QP offset corresponding to temporal layer depth
  char*     m_pchdQPFile;                                     ///< QP offset for each slice (initialized from external file)
  Int*      m_aidQP;                                          ///< array of slice QP values
  Int*      m_aidQPdepth;                                     ///< array of depth slice QP values
  Int       m_iMaxDeltaQP;                                    ///< max. |delta QP|
  UInt      m_uiDeltaQpRD;                                    ///< dQP range for multi-pass slice QP optimization
  Int       m_iMaxCuDQPDepth;                                 ///< Max. depth for a minimum CuDQPSize (0:default)

  Int       m_iChromaQpOffset;                                 ///< ChromaQpOffset    (0:default) 
  Int       m_iChromaQpOffset2nd;                              ///< ChromaQpOffset2nd (0:default)

#if ADAPTIVE_QP_SELECTION
  Bool      m_bUseAdaptQpSelect;
#endif

  Bool      m_bUseAdaptiveQP;                                 ///< Flag for enabling QP adaptation based on a psycho-visual model
  Int       m_iQPAdaptationRange;                             ///< dQP range by QP adaptation
  
  Int       m_maxTempLayer[MAX_VIEW_NUM];                     ///< Max temporal layer

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
  UInt      m_uiInternalBitDepth;                             ///< Internal bit-depth (BitDepth+BitIncrement)

  // coding tools (PCM bit-depth)
  Bool      m_bPCMInputBitDepthFlag;                          ///< 0: PCM bit-depth is internal bit-depth. 1: PCM bit-depth is input bit-depth.
  UInt      m_uiPCMBitDepthLuma;                              ///< PCM bit-depth for luma

  // coding tool (lossless)
#if LOSSLESS_CODING
  Bool      m_useLossless;                                    ///< flag for using lossless coding
#endif
  vector<Bool> m_abUseSAO;
#if LGE_ILLUCOMP_B0045
#if LGE_ILLUCOMP_DEPTH_C0046
  vector<Bool> m_abUseIC;                                    ///< flag for using illumination compensation for inter-view prediction
#else
  Bool      m_bUseIC;                                     ///< flag for using illumination compensation for inter-view prediction
#endif
#endif
#if INTER_VIEW_VECTOR_SCALING_C0115
  Bool      m_bUseIVS;                                        ///< flag for using inter-view vector scaling
#endif
  Int       m_maxNumOffsetsPerPic;                            ///< SAO maximun number of offset per picture
  Bool      m_saoInterleavingFlag;                            ///< SAO interleaving flag
  // coding tools (loop filter)
  vector<Bool> m_abUseALF;                                    ///< flag for using adaptive loop filter [0] - video, [1] - depth
  Int       m_iALFEncodePassReduction;                        //!< ALF encoding pass, 0 = original 16-pass, 1 = 1-pass, 2 = 2-pass
  
  Int       m_iALFMaxNumberFilters;                           ///< ALF Max Number Filters in one picture
  Bool      m_bALFParamInSlice;
  Bool      m_bALFPicBasedEncode;

  vector<Bool> m_abLoopFilterDisable;                         ///< flag for using deblocking filter filter [0] - video, [1] - depth
  Bool      m_loopFilterOffsetInAPS;                         ///< offset for deblocking filter in 0 = slice header, 1 = APS
  Int       m_loopFilterBetaOffsetDiv2;                     ///< beta offset for deblocking filter
  Int       m_loopFilterTcOffsetDiv2;                       ///< tc offset for deblocking filter
  Bool      m_DeblockingFilterControlPresent;                 ///< deblocking filter control present flag in PPS
 
  Bool      m_bUseLMChroma;                                  ///< JL: Chroma intra prediction based on luma signal

  // coding tools (PCM)
  Bool      m_usePCM;                                         ///< flag for using IPCM
  UInt      m_pcmLog2MaxSize;                                 ///< log2 of maximum PCM block size
  UInt      m_uiPCMLog2MinSize;                               ///< log2 of minimum PCM block size
  Bool      m_bPCMFilterDisableFlag;                          ///< PCM filter disable flag

  // coding tools (encoder-only parameters)
  Bool      m_bUseSBACRD;                                     ///< flag for using RD optimization based on SBAC
  Bool      m_bUseASR;                                        ///< flag for using adaptive motion search range
  Bool      m_bUseHADME;                                      ///< flag for using HAD in sub-pel ME
vector<Bool> m_abUseRDOQ;                                   ///< flag for using RD optimized quantization [0]-video, [1]-depth
  Int       m_iFastSearch;                                    ///< ME mode, 0 = full, 1 = diamond, 2 = PMVFAST
  Int       m_iSearchRange;                                   ///< ME search range
#if DV_V_RESTRICTION_B0037
  Bool      m_bUseDisparitySearchRangeRestriction;            ///< restrict vertical search range for inter-view prediction
  Int       m_iVerticalDisparitySearchRange;                  ///< ME vertical search range for inter-view prediction 
#endif
  Int       m_bipredSearchRange;                              ///< ME search range for bipred refinement
  Bool      m_bUseFastEnc;                                    ///< flag for using fast encoder setting
#if HHI_INTERVIEW_SKIP
  Bool      m_bInterViewSkip;                            ///< usage of interview skip mode ( do not transmit residual)
#if HHI_INTERVIEW_SKIP_LAMBDA_SCALE
  Double    m_dInterViewSkipLambdaScale;                 ///< lambda scale for interview skip
#endif
#endif
  Bool      m_bUseEarlyCU;                                    ///< flag for using Early CU setting

#if DEPTH_MAP_GENERATION
  UInt      m_uiPredDepthMapGeneration;                       ///< using of (virtual) depth maps for texture coding
#endif
#if H3D_IVMP
  UInt      m_uiMultiviewMvPredMode;                          ///< usage of predictors for multi-view mv prediction
  UInt      m_uiMultiviewMvRegMode;                           ///< regularization for multiview motion vectors
  Double    m_dMultiviewMvRegLambdaScale;                     ///< lambda scale for multiview motion vectors regularization
#endif
#if HHI_INTER_VIEW_RESIDUAL_PRED
  UInt      m_uiMultiviewResPredMode;          ///< using multiview residual prediction
#endif

  Bool      m_useFastDecisionForMerge;         ///< flag for using Fast Decision Merge RD-Cost 
  Bool      m_bUseCbfFastMode;                 ///< flag for using Cbf Fast PU Mode Decision
  Int       m_iSliceMode;                      ///< 0: Disable all Recon slice limits, 1 : Maximum number of largest coding units per slice, 2: Maximum number of bytes in a slice
  Int       m_iSliceArgument;                  ///< If m_iSliceMode==1, m_iSliceArgument=max. # of largest coding units. If m_iSliceMode==2, m_iSliceArgument=max. # of bytes.
  Int       m_iEntropySliceMode;               ///< 0: Disable all entropy slice limits, 1 : Maximum number of largest coding units per slice, 2: Constraint based entropy slice
  Int       m_iEntropySliceArgument;           ///< If m_iEntropySliceMode==1, m_iEntropySliceArgument=max. # of largest coding units. If m_iEntropySliceMode==2, m_iEntropySliceArgument=max. # of bins.

  Int       m_iSliceGranularity;               ///< 0: Slices always end at LCU borders. 1-3: slices may end at a depth of 1-3 below LCU level.
  Bool      m_bLFCrossSliceBoundaryFlag;       ///< 0: Cross-slice-boundary in-loop filtering 1: non-cross-slice-boundary in-loop filtering
  Int       m_iTileBehaviorControlPresentFlag; //!< 1: tile behavior control parameters are in PPS 0: tile behavior control parameters are not in PPS
  Bool      m_bLFCrossTileBoundaryFlag;        //!< 1: Cross-tile-boundary in-loop filtering 0: non-cross-tile-boundary in-loop filtering
  Int       m_iColumnRowInfoPresent;
  Int       m_iUniformSpacingIdr;
  Int       m_iNumColumnsMinus1;
  char*     m_pchColumnWidth;
  Int       m_iNumRowsMinus1;
  char*     m_pchRowHeight;
  Int       m_iTileLocationInSliceHeaderFlag; //< enable(1)/disable(0) transmitssion of tile location in slice header
  Int       m_iTileMarkerFlag;              //< enable(1)/disable(0) transmitssion of light weight tile marker
  Int       m_iMaxTileMarkerEntryPoints;    //< maximum number of tile markers allowed in a slice (controls degree of parallelism)
  Double    m_dMaxTileMarkerOffset;         //< Calculated offset. Light weight tile markers will be transmitted for TileIdx= Offset, 2*Offset, 3*Offset ... 

  Int       m_iWaveFrontSynchro; //< 0: no WPP. >= 1: WPP is enabled, the "Top right" from which inheritance occurs is this LCU offset in the line above the current.
  Int       m_iWaveFrontFlush; //< enable(1)/disable(0) the CABAC flush at the end of each line of LCUs.
  Int       m_iWaveFrontSubstreams; //< If iWaveFrontSynchro, this is the number of substreams per frame (dependent tiles) or per tile (independent tiles).

  Bool      m_bUseConstrainedIntraPred;                       ///< flag for using constrained intra prediction
  
  bool m_pictureDigestEnabled; ///< enable(1)/disable(0) md5 computation and SEI signalling

  // weighted prediction
  Bool      m_bUseWeightPred;                                 ///< Use of explicit Weighting Prediction for P_SLICE
  UInt      m_uiBiPredIdc;                                    ///< Use of Bi-Directional Weighting Prediction (B_SLICE): explicit(1) or implicit(2)

#if TMVP_DEPTH_SWITCH
  vector<Bool> m_enableTMVP;                                  ///< Enable TMVP [0] video, [1] depth
#else
  Bool      m_enableTMVP;
#endif

  Int       m_signHideFlag;
  Int       m_signHidingThreshold;
#if HHI_MPI
  Bool      m_bUseMVI;  ///< flag for using Motion Vector Inheritance for depth map coding
#endif
#if RWTH_SDC_DLT_B0036
  Bool      m_bUseDLT;
  Bool      m_bUseSDC;
#endif

  Int       m_useScalingListId;                               ///< using quantization matrix
  char*     m_scalingListFile;                                ///< quantization matrix file name

  // camera parameter
  Char*     m_pchCameraParameterFile;                         ///< camera parameter file
  Char*     m_pchBaseViewCameraNumbers;
#if !QC_MVHEVC_B0046
  TAppComCamPara m_cCameraData;
#endif
  Int       m_iCodedCamParPrecision;                          ///< precision for coding of camera parameters

#if HHI_VSO
  Char*     m_pchVSOConfig;
  Bool      m_bUseVSO;                                    ///< flag for using View Synthesis Optimization
#if HHI_VSO_LS_TABLE_M23714
  Bool      m_bVSOLSTable;                                ///< Depth QP dependent Lagrange parameter optimization (m23714)
#endif
#if LGE_VSO_EARLY_SKIP_A0093
  Bool      m_bVSOEarlySkip;                              ///< Early skip of VSO computation (JCT3V-A0093 modification 4)
#endif
  //// Used for development by GT, might be removed later
  Double    m_dLambdaScaleVSO;                            ///< Scaling factor for Lambda in VSO mode
  Bool      m_bForceLambdaScaleVSO;                       ///< Use Lambda Scale for depth even if VSO is turned off
#if HHI_VSO_DIST_INT
  Bool      m_bAllowNegDist;                              ///< Allow negative distortion in VSO
#endif
  UInt      m_uiVSOMode;                                  ///< Number of VSO Mode, 1 = , 2 = simple, org vs. ren, 3 = simple, ren vs. ren, 4 = full  
#endif
#if SAIT_VSO_EST_A0033
  Bool      m_bUseEstimatedVSD;                           ///< Flag for using model based VSD estimation instead of VSO for some encoder decisions (JCT3V-A0033 modification 3)  
#endif
#if LGE_WVSO_A0119
  Bool      m_bUseWVSO;                                    ///< flag for using View Synthesis Optimization  
  Int       m_iVSOWeight;
  Int       m_iVSDWeight;
  Int       m_iDWeight;
#endif
  // coding tools (depth intra modes)
#if HHI_DMM_WEDGE_INTRA || HHI_DMM_PRED_TEX
  Bool      m_bUseDMM;                                        ///< flag for using DMM
#endif

#if OL_QTLIMIT_PREDCODING_B0068
  Bool      m_bUseQTLPC;                                      ///< flag for using depth QuadTree Limitation + Predictive Coding
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

  Void  xAppendToFileNameEnd( Char* pchInputFileName, const Char* pchStringToAppend, Char* & rpchOutputFileName);

  Void  xCheckCodingStructureMvc();                           ///< validate and configure inter-view coding structure

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

//! \}

#endif // __TAPPENCCFG__

