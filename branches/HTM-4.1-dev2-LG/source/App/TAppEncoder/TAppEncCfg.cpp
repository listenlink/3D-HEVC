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

/** \file     TAppEncCfg.cpp
    \brief    Handle encoder configuration parameters
*/

#include <stdlib.h>
#include <cassert>
#include <cstring>
#include <string>
#include "TLibCommon/TComRom.h"
#include "TAppEncCfg.h"
#include "TAppCommon/program_options_lite.h"

#ifdef WIN32
#define strdup _strdup
#endif

using namespace std;
namespace po = df::program_options_lite;

//! \ingroup TAppEncoder
//! \{

/* configuration helper funcs */
void doOldStyleCmdlineOn(po::Options& opts, const std::string& arg);
void doOldStyleCmdlineOff(po::Options& opts, const std::string& arg);

// ====================================================================================================================
// Local constants
// ====================================================================================================================

/// max value of source padding size
/** \todo replace it by command line option
 */
#define MAX_PAD_SIZE                16

// ====================================================================================================================
// Constructor / destructor / initialization / destroy
// ====================================================================================================================

TAppEncCfg::TAppEncCfg()
{
  m_aidQP = NULL;
#if FIXES
  m_aidQPdepth = NULL;
#endif
}

TAppEncCfg::~TAppEncCfg()
{
  if ( m_aidQP )
  {
    delete[] m_aidQP; m_aidQP = NULL;
  }

#if FIXES
  if ( m_aidQPdepth )
  {
    delete[] m_aidQPdepth; m_aidQPdepth = NULL;
  }
#endif

  for(Int i = 0; i< m_pchInputFileList.size(); i++ )
  {
    if ( m_pchInputFileList[i] != NULL )
      free (m_pchInputFileList[i]);
  }
  for(Int i = 0; i< m_pchDepthInputFileList.size(); i++ )
  {
    if ( m_pchDepthInputFileList[i] != NULL )
      free (m_pchDepthInputFileList[i]);
  }
  for(Int i = 0; i< m_pchReconFileList.size(); i++ )
  {
    if ( m_pchReconFileList[i] != NULL )
      free (m_pchReconFileList[i]);
  }
  for(Int i = 0; i< m_pchDepthReconFileList.size(); i++ )
  {
    if ( m_pchDepthReconFileList[i] != NULL )
      free (m_pchDepthReconFileList[i]);
  }
  if (m_pchBitstreamFile != NULL)
    free (m_pchBitstreamFile) ;
#if HHI_VSO
  if (  m_pchVSOConfig != NULL)
    free (  m_pchVSOConfig );
#endif

#if FIX_MEM_LEAKS
 if ( m_pchCameraParameterFile != NULL )
   free ( m_pchCameraParameterFile ); 

 if ( m_pchBaseViewCameraNumbers != NULL )
   free ( m_pchBaseViewCameraNumbers ); 

 if ( m_pchdQPFile      != NULL ) 
   free ( m_pchdQPFile      );

 if ( m_pchColumnWidth  != NULL ) 
   free ( m_pchColumnWidth  );

 if ( m_pchRowHeight    != NULL ) 
   free ( m_pchRowHeight    );

 if ( m_scalingListFile != NULL ) 
   free ( m_scalingListFile );

#endif   

}

Void TAppEncCfg::create()
{
}

Void TAppEncCfg::destroy()
{
}

std::istringstream &operator>>( std::istringstream &in, GOPEntryMvc &entry )     //input
{
  in>>entry.m_sliceType;
  in>>entry.m_POC;
  in>>entry.m_QPOffset;
  in>>entry.m_QPFactor;
  in>>entry.m_temporalId;
  in>>entry.m_numRefPicsActive;
  in>>entry.m_refPic;
  in>>entry.m_numRefPics;
  for ( Int i = 0; i < entry.m_numRefPics; i++ )
  {
    in>>entry.m_referencePics[i];
  }
  in>>entry.m_interRPSPrediction;
  if (entry.m_interRPSPrediction)
  {
    in>>entry.m_deltaRIdxMinus1;
    in>>entry.m_deltaRPS;
    in>>entry.m_numRefIdc;
    for ( Int i = 0; i < entry.m_numRefIdc; i++ )
    {
      in>>entry.m_refIdc[i];
    }
  }
  in>>entry.m_numInterViewRefPics;
  for( Int i = 0; i < entry.m_numInterViewRefPics; i++ )
  {
    in>>entry.m_interViewRefs[i];
  }
  for( Int i = 0; i < entry.m_numInterViewRefPics; i++ )
  {
    in>>entry.m_interViewRefPosL0[i];
  }
  for( Int i = 0; i < entry.m_numInterViewRefPics; i++ )
  {
    in>>entry.m_interViewRefPosL1[i];
  }
  return in;
}

// ====================================================================================================================
// Public member functions
// ====================================================================================================================

/** \param  argc        number of arguments
    \param  argv        array of arguments
    \retval             true when success
 */
Bool TAppEncCfg::parseCfg( Int argc, Char* argv[] )
{
  bool do_help = false;
  
  string cfg_BitstreamFile;
  string cfg_dQPFile;
  string cfg_ColumnWidth;
  string cfg_RowHeight;
  string cfg_ScalingListFile;
  po::Options opts;
  opts.addOptions()
  ("help", do_help, false, "this help text")
  ("c", po::parseConfigFile, "configuration file name")
  
  /* File, I/O and source parameters */
  ("InputFile_%d,i_%d",       m_pchInputFileList,       (char *) 0 , MAX_VIEW_NUM , "original Yuv input file name %d")
  ("DepthInputFile_%d,di_%d", m_pchDepthInputFileList,  (char *) 0 , MAX_VIEW_NUM , "original Yuv depth input file name %d")
  ("ReconFile_%d,o_%d",       m_pchReconFileList,       (char *) 0 , MAX_VIEW_NUM , "reconstructed Yuv output file name %d")
  ("DepthReconFile_%d,do_%d", m_pchDepthReconFileList,  (char *) 0 , MAX_VIEW_NUM , "reconstructed Yuv depth output file name %d")
  ("BitstreamFile,b", cfg_BitstreamFile, string(""), "bitstream output file name")
  ("CodeDepthMaps",         m_bUsingDepthMaps, false, "Encode depth maps" )
  ("CodedCamParsPrecision", m_iCodedCamParPrecision, STD_CAM_PARAMETERS_PRECISION, "precision for coding of camera parameters (in units of 2^(-x) luma samples)" )
  ("LambdaModifier0,-LM0", m_adLambdaModifier[ 0 ], ( double )1.0, "Lambda modifier for temporal layer 0")
  ("LambdaModifier1,-LM1", m_adLambdaModifier[ 1 ], ( double )1.0, "Lambda modifier for temporal layer 1")
  ("LambdaModifier2,-LM2", m_adLambdaModifier[ 2 ], ( double )1.0, "Lambda modifier for temporal layer 2")
  ("LambdaModifier3,-LM3", m_adLambdaModifier[ 3 ], ( double )1.0, "Lambda modifier for temporal layer 3")
  ("SourceWidth,-wdt",      m_iSourceWidth,  0, "Source picture width")
  ("SourceHeight,-hgt",     m_iSourceHeight, 0, "Source picture height")
#if PIC_CROPPING
  ("CroppingMode",          m_croppingMode,  0, "Cropping mode (0: no cropping, 1:automatic padding, 2: padding, 3:cropping")
  ("CropLeft",              m_cropLeft,      0, "Left cropping/padding for cropping mode 3")
  ("CropRight",             m_cropRight,     0, "Right cropping/padding for cropping mode 3")
  ("CropTop",               m_cropTop,       0, "Top cropping/padding for cropping mode 3")
  ("CropBottom",            m_cropBottom,    0, "Bottom cropping/padding for cropping mode 3")
  ("HorizontalPadding,-pdx",m_aiPad[0],      0, "horizontal source padding for cropping mode 2")
  ("VerticalPadding,-pdy",  m_aiPad[1],      0, "vertical source padding for cropping mode 2")
#endif
  ("InputBitDepth",         m_uiInputBitDepth, 8u, "bit-depth of input file")
  ("BitDepth",              m_uiInputBitDepth, 8u, "deprecated alias of InputBitDepth")
  ("OutputBitDepth",        m_uiOutputBitDepth, 0u, "bit-depth of output file")
  ("InternalBitDepth",      m_uiInternalBitDepth, 0u, "Internal bit-depth (BitDepth+BitIncrement)")
#if !PIC_CROPPING
  ("HorizontalPadding,-pdx",m_aiPad[0],      0, "horizontal source padding size")
  ("VerticalPadding,-pdy",  m_aiPad[1],      0, "vertical source padding size")
  ("PAD",                   m_bUsePAD,   false, "automatic source padding of multiple of 16" )
#endif
  ("FrameRate,-fr",         m_iFrameRate,        0, "Frame rate")
  ("FrameSkip,-fs",         m_FrameSkip,         0u, "Number of frames to skip at start of input YUV")
  ("FramesToBeEncoded,f",   m_iFrameToBeEncoded, 0, "number of frames to be encoded (default=all)")
  ("FrameToBeEncoded",        m_iFrameToBeEncoded, 0, "deprecated alias of FramesToBeEncoded")
  
  ("NumberOfViews",         m_iNumberOfViews,    0, "Number of views")
  /* Unit definition parameters */
  ("MaxCUWidth",          m_uiMaxCUWidth,  64u)
  ("MaxCUHeight",         m_uiMaxCUHeight, 64u)
  /* todo: remove defaults from MaxCUSize */
  ("MaxCUSize,s",         m_uiMaxCUWidth,  64u, "max CU size")
  ("MaxCUSize,s",         m_uiMaxCUHeight, 64u, "max CU size")
  ("MaxPartitionDepth,h", m_uiMaxCUDepth,   4u, "CU depth")
  
  ("QuadtreeTULog2MaxSize", m_uiQuadtreeTULog2MaxSize, 6u)
  ("QuadtreeTULog2MinSize", m_uiQuadtreeTULog2MinSize, 2u)
  
  ("QuadtreeTUMaxDepthIntra", m_uiQuadtreeTUMaxDepthIntra, 1u)
  ("QuadtreeTUMaxDepthInter", m_uiQuadtreeTUMaxDepthInter, 2u)
  
  /* Coding structure paramters */
  ("IntraPeriod,-ip",m_iIntraPeriod, -1, "intra period in frames, (-1: only first frame)")
  ("DecodingRefreshType,-dr",m_iDecodingRefreshType, 0, "intra refresh, (0:none 1:CRA 2:IDR)")
  ("GOPSize,g",      m_iGOPSize,      1, "GOP size of temporal structure")
#if !H0567_DPB_PARAMETERS_PER_TEMPORAL_LAYER
  ("MaxNumberOfReorderPictures",   m_numReorderFrames,               -1, "Max. number of reorder pictures: -1: encoder determines value, >=0: set explicitly")
  ("MaxNumberOfReferencePictures", m_maxNumberOfReferencePictures, 6, "Max. number of reference pictures")
#endif
  ("ListCombination,-lc", m_bUseLComb, true, "combined reference list flag for uni-prediction in B-slices")
  ("LCModification", m_bLCMod, false, "enables signalling of combined reference list derivation")
  ("DisableInter4x4", m_bDisInter4x4, true, "Disable Inter 4x4")
  ("NSQT", m_enableNSQT, true, "Enable non-square transforms")
  ("AMP", m_enableAMP, true, "Enable asymmetric motion partitions")
  /* motion options */
  ("FastSearch", m_iFastSearch, 1, "0:Full search  1:Diamond  2:PMVFAST")
  ("SearchRange,-sr",m_iSearchRange, 96, "motion search range")
  ("BipredSearchRange", m_bipredSearchRange, 4, "motion search range for bipred refinement")
  ("HadamardME", m_bUseHADME, true, "hadamard ME for fractional-pel")
  ("ASR", m_bUseASR, false, "adaptive motion search range")
  
  /* Quantization parameters */
  ("QP,q",          m_adQP,     std::vector<double>(1,32), "Qp value, if value is float, QP is switched once during encoding, if two values are given the second is used for depth")
  ("DeltaQpRD,-dqr",m_uiDeltaQpRD,       0u, "max dQp offset for slice")
  ("MaxDeltaQP,d",  m_iMaxDeltaQP,        0, "max dQp offset for block")
  ("MaxCuDQPDepth,-dqd",  m_iMaxCuDQPDepth,        0, "max depth for a minimum CuDQP")

    ("ChromaQpOffset,   -cqo",   m_iChromaQpOffset,           0, "ChromaQpOffset")
    ("ChromaQpOffset2nd,-cqo2",  m_iChromaQpOffset2nd,        0, "ChromaQpOffset2nd")

#if ADAPTIVE_QP_SELECTION
    ("AdaptiveQpSelection,-aqps",   m_bUseAdaptQpSelect,           false, "AdaptiveQpSelection")
#endif

  ("AdaptiveQP,-aq", m_bUseAdaptiveQP, false, "QP adaptation based on a psycho-visual model")
  ("MaxQPAdaptationRange,-aqr", m_iQPAdaptationRange, 6, "QP adaptation range")
  ("dQPFile,m",     cfg_dQPFile, string(""), "dQP file name")
  ("RDOQ",          m_abUseRDOQ, std::vector<Bool>(1,true), "Enable RDOQ")
  ("TemporalLayerQPOffset_L0,-tq0", m_aiTLayerQPOffset[0], MAX_QP + 1, "QP offset of temporal layer 0")
  ("TemporalLayerQPOffset_L1,-tq1", m_aiTLayerQPOffset[1], MAX_QP + 1, "QP offset of temporal layer 1")
  ("TemporalLayerQPOffset_L2,-tq2", m_aiTLayerQPOffset[2], MAX_QP + 1, "QP offset of temporal layer 2")
  ("TemporalLayerQPOffset_L3,-tq3", m_aiTLayerQPOffset[3], MAX_QP + 1, "QP offset of temporal layer 3")
  
#if !H0566_TLA
  ("TLayeringBasedOnCodingStruct,-tl", m_bTLayering, false, "Temporal ID is set based on the hierarchical coding structure")
  
  ("TLayerSwitchingFlag_L0,-ts0", m_abTLayerSwitchingFlag[0], false, "Switching flag for temporal layer 0")
  ("TLayerSwitchingFlag_L1,-ts1", m_abTLayerSwitchingFlag[1], false, "Switching flag for temporal layer 1")
  ("TLayerSwitchingFlag_L2,-ts2", m_abTLayerSwitchingFlag[2], false, "Switching flag for temporal layer 2")
  ("TLayerSwitchingFlag_L3,-ts3", m_abTLayerSwitchingFlag[3], false, "Switching flag for temporal layer 3")
#endif

  /* Entropy coding parameters */
  ("SBACRD", m_bUseSBACRD, true, "SBAC based RD estimation")
  
  /* Deblocking filter parameters */
  ("LoopFilterDisable", m_abLoopFilterDisable, std::vector<Bool>(1,false), "Disables LoopFilter")

  ("LoopFilterOffsetInAPS", m_loopFilterOffsetInAPS, false)
  ("LoopFilterBetaOffset_div2", m_loopFilterBetaOffsetDiv2, 0 )
  ("LoopFilterTcOffset_div2", m_loopFilterTcOffsetDiv2, 0 )
#if DBL_CONTROL
#if FIX_DBL_CONTROL_DEFAULT
  ("DeblockingFilterControlPresent", m_DeblockingFilterControlPresent, true)
#else
  ("DeblockingFilterControlPresent", m_DeblockingFilterControlPresent, false)
#endif
#endif

  /* Camera Paremetes */
  ("CameraParameterFile,cpf", m_pchCameraParameterFile,    (Char *) 0, "Camera Parameter File Name")
  ("BaseViewCameraNumbers" ,  m_pchBaseViewCameraNumbers,  (Char *) 0, "Numbers of base views")

  /* View Synthesis Optimization */

#if HHI_VSO
  ("VSOConfig",                       m_pchVSOConfig            , (Char *) 0    , "VSO configuration")
  ("VSO",                             m_bUseVSO                 , false         , "Use VSO" )    
  ("VSOMode",                         m_uiVSOMode               , (UInt)   4    , "VSO Mode")
  ("LambdaScaleVSO",                  m_dLambdaScaleVSO         , (Double) 1    , "Lambda Scaling for VSO")

#if HHI_VSO_LS_TABLE
  ("VSOLSTable",                      m_bVSOLSTable             , true          , "Depth QP dependent video/depth rate allocation by Lagrange multiplier" )    
#endif

#if SAIT_VSO_EST_A0033
  ("UseEstimatedVSD",                 m_bUseEstimatedVSD        , true          , "Model based VSD estimation instead of rendering based for some encoder decisions" )      
#endif
#if LGE_VSO_EARLY_SKIP_A0093
  ("VSOEarlySkip",                    m_bVSOEarlySkip           , true          , "Early skip of VSO computation if synthesis error assumed to be zero" )      
#endif
  ("ForceLambdaScaleVSO",             m_bForceLambdaScaleVSO    , false         , "Force using Lambda Scale VSO also in non-VSO-Mode")
#if HHI_VSO_DIST_INT
  ("AllowNegDist",                    m_bAllowNegDist           , true          , "Allow negative Distortion in VSO")
#endif
#if LGE_WVSO_A0119
  ("WVSO",                            m_bUseWVSO                , true          , "Use depth fidelity term for VSO" )
  ("VSOWeight",                       m_iVSOWeight              , 10            , "Synthesized View Distortion Change weight" )
  ("VSDWeight",                       m_iVSDWeight              , 1             , "View Synthesis Distortion estimate weight" )
  ("DWeight",                         m_iDWeight                , 1             , "Depth Distortion weight" )
#endif

#if OL_DEPTHLIMIT_A0044
  ("DPL",                             m_bDepthPartitionLimiting , false         , "Use DepthPartitionLimiting" )
#endif

#endif

#if DEPTH_MAP_GENERATION
  ("PredDepthMapGen",  m_uiPredDepthMapGeneration, (UInt)0, "generation of prediction depth maps for motion data prediction" )
#endif
#if HHI_INTER_VIEW_MOTION_PRED
  ("MultiviewMvPred",  m_uiMultiviewMvPredMode,    (UInt)0, "usage of predicted depth maps" )
  ("MultiviewMvRegMode",        m_uiMultiviewMvRegMode,         (UInt)0, "regularization mode for multiview motion vectors" )
  ("MultiviewMvRegLambdaScale", m_dMultiviewMvRegLambdaScale, (Double)0, "lambda scale for multiview motion vector regularization" )
#endif
#if HHI_INTER_VIEW_RESIDUAL_PRED
  ("MultiviewResPred", m_uiMultiviewResPredMode,   (UInt)0, "usage of inter-view residual prediction" )
#endif

  /* Coding tools */
  ("LMChroma", m_bUseLMChroma, true, "intra chroma prediction based on recontructed luma")

  ("ALF", m_abUseALF, std::vector<Bool>(1,true), "Enables ALF")
  ("SAO", m_abUseSAO, std::vector<Bool>(1, true), "SAO")
#if SAO_UNIT_INTERLEAVING
  ("MaxNumOffsetsPerPic", m_maxNumOffsetsPerPic, 2048, "2048: default")   
  ("SAOInterleaving", m_saoInterleavingFlag, false, "0: SAO Picture Mode, 1: SAO Interleaving ")   
#endif

  ("ALFEncodePassReduction", m_iALFEncodePassReduction, 0, "0:Original 16-pass, 1: 1-pass, 2: 2-pass encoding")

  ("ALFMaxNumFilter,-ALFMNF", m_iALFMaxNumberFilters, 16, "16: No Constrained, 1-15: Constrained max number of filter")
#if LCU_SYNTAX_ALF
  ("ALFParamInSlice", m_bALFParamInSlice, false, "ALF parameters in 0: APS, 1: slice header")
  ("ALFPicBasedEncode", m_bALFPicBasedEncode, true, "ALF picture-based encoding 0: false, 1: true")
#endif

    ("SliceMode",            m_iSliceMode,           0, "0: Disable all Recon slice limits, 1: Enforce max # of LCUs, 2: Enforce max # of bytes")
    ("SliceArgument",        m_iSliceArgument,       0, "if SliceMode==1 SliceArgument represents max # of LCUs. if SliceMode==2 SliceArgument represents max # of bytes.")
    ("EntropySliceMode",     m_iEntropySliceMode,    0, "0: Disable all entropy slice limits, 1: Enforce max # of LCUs, 2: Enforce constraint based entropy slices")
    ("EntropySliceArgument", m_iEntropySliceArgument,0, "if EntropySliceMode==1 SliceArgument represents max # of LCUs. if EntropySliceMode==2 EntropySliceArgument represents max # of bins.")
    ("SliceGranularity",     m_iSliceGranularity,    0, "0: Slices always end at LCU borders. 1-3: slices may end at a depth of 1-3 below LCU level.")
    ("LFCrossSliceBoundaryFlag", m_bLFCrossSliceBoundaryFlag, true)

    ("ConstrainedIntraPred", m_bUseConstrainedIntraPred, false, "Constrained Intra Prediction")
    ("PCMEnabledFlag", m_usePCM         , false)
    ("PCMLog2MaxSize", m_pcmLog2MaxSize, 5u)
    ("PCMLog2MinSize", m_uiPCMLog2MinSize, 3u)

    ("PCMInputBitDepthFlag", m_bPCMInputBitDepthFlag, true)
    ("PCMFilterDisableFlag", m_bPCMFilterDisableFlag, false)
#if LOSSLESS_CODING
    ("LosslessCuEnabled", m_useLossless, false)
#endif
    ("weighted_pred_flag,-wpP",     m_bUseWeightPred, false, "weighted prediction flag (P-Slices)")
    ("weighted_bipred_idc,-wpBidc", m_uiBiPredIdc,    0u,    "weighted bipred idc (B-Slices)")
    ("TileInfoPresentFlag",         m_iColumnRowInfoPresent,         1,          "0: tiles parameters are NOT present in the PPS. 1: tiles parameters are present in the PPS")
    ("UniformSpacingIdc",           m_iUniformSpacingIdr,            0,          "Indicates if the column and row boundaries are distributed uniformly")
#if !REMOVE_TILE_DEPENDENCE
    ("TileBoundaryIndependenceIdc", m_iTileBoundaryIndependenceIdr,  1,          "Indicates if the column and row boundaries break the prediction")
#endif
    ("NumTileColumnsMinus1",        m_iNumColumnsMinus1,             0,          "Number of columns in a picture minus 1")
    ("ColumnWidthArray",            cfg_ColumnWidth,                 string(""), "Array containing ColumnWidth values in units of LCU")
    ("NumTileRowsMinus1",           m_iNumRowsMinus1,                0,          "Number of rows in a picture minus 1")
    ("RowHeightArray",              cfg_RowHeight,                   string(""), "Array containing RowHeight values in units of LCU")
    ("TileLocationInSliceHeaderFlag", m_iTileLocationInSliceHeaderFlag, 0,       "0: Disable transmission of tile location in slice header. 1: Transmit tile locations in slice header.")
    ("TileMarkerFlag",                m_iTileMarkerFlag,                0,       "0: Disable transmission of lightweight tile marker. 1: Transmit light weight tile marker.")
    ("MaxTileMarkerEntryPoints",    m_iMaxTileMarkerEntryPoints,    4,       "Maximum number of uniformly-spaced tile entry points (using light weigh tile markers). Default=4. If number of tiles < MaxTileMarkerEntryPoints then all tiles have entry points.")
    ("TileControlPresentFlag",       m_iTileBehaviorControlPresentFlag,         1,          "0: tiles behavior control parameters are NOT present in the PPS. 1: tiles behavior control parameters are present in the PPS")
    ("LFCrossTileBoundaryFlag",      m_bLFCrossTileBoundaryFlag,             true,          "1: cross-tile-boundary loop filtering. 0:non-cross-tile-boundary loop filtering")
    ("WaveFrontSynchro",            m_iWaveFrontSynchro,             0,          "0: no synchro; 1 synchro with TR; 2 TRR etc")
    ("WaveFrontFlush",              m_iWaveFrontFlush,               0,          "Flush and terminate CABAC coding for each LCU line")
    ("WaveFrontSubstreams",         m_iWaveFrontSubstreams,          1,          "# coded substreams wanted; per tile if TileBoundaryIndependenceIdc is 1, otherwise per frame")
    ("ScalingList",                 m_useScalingListId,              0,          "0: no scaling list, 1: default scaling lists, 2: scaling lists specified in ScalingListFile")
    ("ScalingListFile",             cfg_ScalingListFile,             string(""), "Scaling list file name")
#if MULTIBITS_DATA_HIDING
    ("SignHideFlag,-SBH",                m_signHideFlag, 1)
    ("SignHideThreshold,-TSIG",          m_signHidingThreshold,         4)
#endif
#if HHI_DMM_WEDGE_INTRA || HHI_DMM_PRED_TEX
  ("DMM",                         m_bUseDMM,                       false,      "depth model modes flag")
#endif

  /* Misc. */
  ("SEIpictureDigest", m_pictureDigestEnabled, true, "Control generation of picture_digest SEI messages\n"
                                              "\t1: use MD5\n"
                                              "\t0: disable")

  ("TMVP", m_enableTMVP, true, "Enable TMVP" )

  ("FEN", m_bUseFastEnc, false, "fast encoder setting")
  ("ECU", m_bUseEarlyCU, false, "Early CU setting") 
#if FAST_DECISION_FOR_MRG_RD_COST
  ("FDM", m_useFastDecisionForMerge, true, "Fast decision for Merge RD Cost") 
#endif
  ("CFM", m_bUseCbfFastMode, false, "Cbf fast mode setting")
#if HHI_INTERVIEW_SKIP
  ("InterViewSkip",  m_bInterViewSkip,    false, "usage of interview skip" )
#if HHI_INTERVIEW_SKIP_LAMBDA_SCALE
  ("InterViewSkipLambdaScale",  m_dInterViewSkipLambdaScale,    (Double)8, "lambda scale for interview skip" )
#endif
#endif
  /* Compatability with old style -1 FOO or -0 FOO options. */
  ("1", doOldStyleCmdlineOn, "turn option <name> on")
  ("0", doOldStyleCmdlineOff, "turn option <name> off")
#if HHI_MPI
  ("MVI", m_bUseMVI, false, "use motion vector inheritance for depth map coding")
#endif
  ;
  
  // parse coding structure
  for( Int k = 0; k < MAX_VIEW_NUM; k++ )
  {
    if( k == 0 )
    {
      for( Int i = 1; i < MAX_GOP + 1; i++ ) 
      {
        std::ostringstream cOSS;
        cOSS<<"Frame"<<i;
        opts.addOptions()( cOSS.str(), m_GOPListsMvc[k][i-1], GOPEntryMvc() );
      }
    }
    else
    {
      std::ostringstream cOSS1;
      cOSS1<<"FrameI"<<"_v"<<k;
      opts.addOptions()(cOSS1.str(), m_GOPListsMvc[k][MAX_GOP], GOPEntryMvc());

      for( Int i = 1; i < MAX_GOP + 1; i++ ) 
      {
        std::ostringstream cOSS2;
        cOSS2<<"Frame"<<i<<"_v"<<k;
        opts.addOptions()(cOSS2.str(), m_GOPListsMvc[k][i-1], GOPEntryMvc());
      }
    }
  }

  po::setDefaults(opts);
  const list<const char*>& argv_unhandled = po::scanArgv(opts, argc, (const char**) argv);

  for (list<const char*>::const_iterator it = argv_unhandled.begin(); it != argv_unhandled.end(); it++)
  {
    fprintf(stderr, "Unhandled argument ignored: `%s'\n", *it);
  }
  
  if (argc == 1 || do_help)
  {
    /* argc == 1: no options have been specified */
    po::doHelp(cout, opts);
    xPrintUsage();
    return false;
  }
  
  /*
   * Set any derived parameters
   */
  /* convert std::string to c string for compatability */
  m_pchBitstreamFile = cfg_BitstreamFile.empty() ? NULL : strdup(cfg_BitstreamFile.c_str());
  m_pchdQPFile = cfg_dQPFile.empty() ? NULL : strdup(cfg_dQPFile.c_str());
  
  m_pchColumnWidth = cfg_ColumnWidth.empty() ? NULL: strdup(cfg_ColumnWidth.c_str());
  m_pchRowHeight = cfg_RowHeight.empty() ? NULL : strdup(cfg_RowHeight.c_str());
  m_scalingListFile = cfg_ScalingListFile.empty() ? NULL : strdup(cfg_ScalingListFile.c_str());
  
  if ( m_bUsingDepthMaps )
  {
    for(Int i = 0; i < m_pchDepthReconFileList.size() ; i++)
    {
      if ((m_pchDepthInputFileList[i] != NULL) && (m_pchReconFileList[i] != NULL) && (i < m_iNumberOfViews) )
      {
        if (m_pchDepthReconFileList[i] == NULL )
        {
          xAppendToFileNameEnd( m_pchReconFileList[i], "_depth", m_pchDepthReconFileList[i] );
        }
      }
      else
      {
        m_pchDepthReconFileList[i] = NULL;
      }
    };
  }
  if ( m_adQP.size() < 2 )
  {
    m_adQP.push_back( m_adQP[0] );
  };
  for (UInt uiK = 0; uiK < m_adQP.size(); uiK++)
  {
    m_aiQP.push_back( (Int)( m_adQP[uiK] ) );
  }

#if PIC_CROPPING
  switch (m_croppingMode)
  {
  case 0:
    {
      // no cropping or padding
      m_cropLeft = m_cropRight = m_cropTop = m_cropBottom = 0;
      m_aiPad[1] = m_aiPad[0] = 0;
      break;
    }
  case 1:
    {
      // automatic padding to minimum CU size
      Int minCuSize = m_uiMaxCUHeight >> (m_uiMaxCUDepth - 1);
      if (m_iSourceWidth % minCuSize)
      {
        m_aiPad[0] = m_cropRight  = ((m_iSourceWidth / minCuSize) + 1) * minCuSize - m_iSourceWidth;
        m_iSourceWidth  += m_cropRight;
      }
      if (m_iSourceHeight % minCuSize)
      {
        m_aiPad[1] = m_cropBottom = ((m_iSourceHeight / minCuSize) + 1) * minCuSize - m_iSourceHeight;
        m_iSourceHeight += m_cropBottom;
      }
      break;
    }
  case 2:
    {
      //padding
      m_iSourceWidth  += m_aiPad[0];
      m_iSourceHeight += m_aiPad[1];
      m_cropRight  = m_aiPad[0];
      m_cropBottom = m_aiPad[1];
      break;
    }
  case 3:
    {
      // cropping
      if ((m_cropLeft == 0) && (m_cropRight == 0) && (m_cropTop == 0) && (m_cropBottom == 0))
      {
        fprintf(stderr, "Warning: Cropping enabled, but all cropping parameters set to zero\n");
      }
      if ((m_aiPad[1] != 0) || (m_aiPad[0]!=0))
      {
        fprintf(stderr, "Warning: Cropping enabled, padding parameters will be ignored\n");
      }
      m_aiPad[1] = m_aiPad[0] = 0;
      break;
    }
  }
#else

  // compute source padding size
  if ( m_bUsePAD )
  {
    if ( m_iSourceWidth%MAX_PAD_SIZE )
    {
      m_aiPad[0] = (m_iSourceWidth/MAX_PAD_SIZE+1)*MAX_PAD_SIZE - m_iSourceWidth;
    }
    
    if ( m_iSourceHeight%MAX_PAD_SIZE )
    {
      m_aiPad[1] = (m_iSourceHeight/MAX_PAD_SIZE+1)*MAX_PAD_SIZE - m_iSourceHeight;
    }
  }
  m_iSourceWidth  += m_aiPad[0];
  m_iSourceHeight += m_aiPad[1];
#endif
  
  // allocate slice-based dQP values
  m_aidQP = new Int[ m_iFrameToBeEncoded + m_iGOPSize + 1 ];
  m_aidQPdepth =  new Int[ m_iFrameToBeEncoded + m_iGOPSize + 1 ];
  ::memset( m_aidQP, 0, sizeof(Int)*( m_iFrameToBeEncoded + m_iGOPSize + 1 ) );
  ::memset( m_aidQPdepth, 0, sizeof(Int)*( m_iFrameToBeEncoded + m_iGOPSize + 1 ) );
  
  // handling of floating-point QP values
  // if QP is not integer, sequence is split into two sections having QP and QP+1
  m_aiQP[0] = (Int)( m_adQP[0] );
  if ( m_aiQP[0] < m_adQP[0] )
  {
    Int iSwitchPOC = (Int)( m_iFrameToBeEncoded - (m_adQP[0] - m_aiQP[0])*m_iFrameToBeEncoded + 0.5 );
    
    iSwitchPOC = (Int)( (Double)iSwitchPOC / m_iGOPSize + 0.5 )*m_iGOPSize;
    for ( Int i=iSwitchPOC; i<m_iFrameToBeEncoded + m_iGOPSize + 1; i++ )
    {
      m_aidQP[i] = 1;
    }
  }

  m_aiQP[1] = (Int)( m_adQP[1] );
  if ( m_aiQP[1] < m_adQP[1] )
  {
    Int iSwitchPOC = (Int)( m_iFrameToBeEncoded - (m_adQP[1] - m_aiQP[1])*m_iFrameToBeEncoded + 0.5 );

    iSwitchPOC = (Int)( (Double)iSwitchPOC / m_iGOPSize + 0.5 )*m_iGOPSize;
    for ( Int i=iSwitchPOC; i<m_iFrameToBeEncoded + m_iGOPSize + 1; i++ )
    {
      m_aidQPdepth[i] = 1;
    }
  }

  // reading external dQP description from file
  if ( m_pchdQPFile )
  {
    FILE* fpt=fopen( m_pchdQPFile, "r" );
    if ( fpt )
    {
      Int iValue;
      Int iPOC = 0;
      while ( iPOC < m_iFrameToBeEncoded )
      {
        if ( fscanf(fpt, "%d", &iValue ) == EOF ) break;
        m_aidQP[ iPOC ] = iValue;
        iPOC++;
      }
      fclose(fpt);
    }
  }

#if HHI_VSO
  m_bUseVSO = m_bUseVSO && m_bUsingDepthMaps && (m_uiVSOMode != 0);
#endif

#if LGE_WVSO_A0119
  m_bUseWVSO = m_bUseVSO && m_bUseWVSO && m_bUsingDepthMaps;
#endif
  xCleanUpVectors();

#if HHI_VSO
  if ( m_abUseALF .size() < 2)
    m_abUseALF .push_back( m_bUseVSO ? false : m_abUseALF[0]  );

  if ( m_abUseRDOQ.size() < 2)
    m_abUseRDOQ.push_back( m_bUseVSO ? true : m_abUseRDOQ[0] );

  if ( m_abLoopFilterDisable.size() < 2)
    m_abLoopFilterDisable.push_back( m_bUseVSO ? true : m_abLoopFilterDisable[0]  );

  if (m_abUseSAO.size() < 2)
    m_abUseSAO.push_back            ( m_bUseVSO ? false : m_abUseSAO[0] );
#else
  if ( m_abUseALF .size() < 2)
    m_abUseALF .push_back( m_abUseALF[0]  );

  if ( m_abUseRDOQ.size() < 2)
    m_abUseRDOQ.push_back( m_abUseRDOQ[0] );

  if ( m_abLoopFilterDisable.size() < 2)
    m_abLoopFilterDisable.push_back( m_abLoopFilterDisable[0]  );

  if (m_abUseSAO.size() < 2)
    m_abUseSAO.push_back            ( m_abUseSAO[0] );
#endif

#if HHI_VSO

#if HHI_VSO_LS_TABLE
  // Q&D
  Double adLambdaScaleTable[] = 
  {  0.031250, 0.031639, 0.032029, 0.032418, 0.032808, 0.033197, 0.033586, 0.033976, 0.034365, 0.034755, 
     0.035144, 0.035533, 0.035923, 0.036312, 0.036702, 0.037091, 0.037480, 0.037870, 0.038259, 0.038648, 
     0.039038, 0.039427, 0.039817, 0.040206, 0.040595, 0.040985, 0.041374, 0.041764, 0.042153, 0.042542, 
     0.042932, 0.043321, 0.043711, 0.044100, 0.044194, 0.053033, 0.061872, 0.070711, 0.079550, 0.088388, 
     0.117851, 0.147314, 0.176777, 0.235702, 0.294628, 0.353553, 0.471405, 0.589256, 0.707107, 0.707100, 
     0.753550, 0.800000  
  }; 
  if ( m_bVSOLSTable )
  {
    AOT( (m_aiQP[1] < 0) || (m_aiQP[1] > 51));
    m_dLambdaScaleVSO *= adLambdaScaleTable[m_aiQP[1]]; 
  }
#endif
#endif

 // set global variables
  xSetGlobal();

  // read and check camera parameters
#if HHI_VSO
if ( m_bUseVSO && m_uiVSOMode == 4)
{
  m_cRenModStrParser.setString( m_iNumberOfViews, m_pchVSOConfig );
  m_cCameraData     .init     ( (UInt)m_iNumberOfViews,
                                      m_uiInputBitDepth,
                                (UInt)m_iCodedCamParPrecision,
                                      m_FrameSkip,
                                (UInt)m_iFrameToBeEncoded,
                                      m_pchCameraParameterFile,
                                      m_pchBaseViewCameraNumbers,
                                      NULL,
                                      m_cRenModStrParser.getSynthViews(),
                                      LOG2_DISP_PREC_LUT );
}
else if ( m_bUseVSO && m_uiVSOMode != 4 )
{
  m_cCameraData     .init     ( (UInt)m_iNumberOfViews,
                                      m_uiInputBitDepth,
                                (UInt)m_iCodedCamParPrecision,
                                      m_FrameSkip,
                                (UInt)m_iFrameToBeEncoded,
                                      m_pchCameraParameterFile,
                                      m_pchBaseViewCameraNumbers,
                                      m_pchVSOConfig,
                                      NULL,
                                      LOG2_DISP_PREC_LUT );
}
else
{
  m_cCameraData     .init     ( (UInt)m_iNumberOfViews,
    m_uiInputBitDepth,
    (UInt)m_iCodedCamParPrecision,
    m_FrameSkip,
    (UInt)m_iFrameToBeEncoded,
    m_pchCameraParameterFile,
    m_pchBaseViewCameraNumbers,
    NULL,
    NULL,
    LOG2_DISP_PREC_LUT );
}
#else
  m_cCameraData     .init     ( (UInt)m_iNumberOfViews,
    m_uiInputBitDepth,
    (UInt)m_iCodedCamParPrecision,
    m_FrameSkip,
    (UInt)m_iFrameToBeEncoded,
    m_pchCameraParameterFile,
    m_pchBaseViewCameraNumbers,
    NULL,
    NULL,
    LOG2_DISP_PREC_LUT );
#endif


  // check validity of input parameters
  xCheckParameter();
  m_cCameraData.check( false, true );
  
  // print-out parameters
  xPrintParameter();
  
  return true;
}

// ====================================================================================================================
// Private member functions
// ====================================================================================================================

Bool confirmPara(Bool bflag, const char* message);

Void TAppEncCfg::xCheckParameter()
{
  bool check_failed = false; /* abort if there is a fatal configuration problem */
#define xConfirmPara(a,b) check_failed |= confirmPara(a,b)
  // check range of parameters
  xConfirmPara( m_uiInputBitDepth < 8,                                                      "InputBitDepth must be at least 8" );
  xConfirmPara( m_iFrameRate <= 0,                                                          "Frame rate must be more than 1" );
  xConfirmPara( m_iFrameToBeEncoded <= 0,                                                   "Total Number Of Frames encoded must be more than 0" );
  xConfirmPara( m_iGOPSize < 1 ,                                                            "GOP Size must be greater or equal to 1" );
  xConfirmPara( m_iGOPSize > 1 &&  m_iGOPSize % 2,                                          "GOP Size must be a multiple of 2, if GOP Size is greater than 1" );
  xConfirmPara( (m_iIntraPeriod > 0 && m_iIntraPeriod < m_iGOPSize) || m_iIntraPeriod == 0, "Intra period must be more than GOP size, or -1 , not 0" );
  xConfirmPara( m_iDecodingRefreshType < 0 || m_iDecodingRefreshType > 2,                   "Decoding Refresh Type must be equal to 0, 1 or 2" );
#if H0736_AVC_STYLE_QP_RANGE
  xConfirmPara( m_aiQP[0] < -6 * ((Int)m_uiInternalBitDepth - 8) || m_aiQP[0] > 51,         "QP exceeds supported range (-QpBDOffsety to 51)" );
  if ( m_aiQP.size() >= 2 )
  {
    xConfirmPara( m_aiQP[1] < -6 * ((Int)m_uiInternalBitDepth - 8) || m_aiQP[1] > 51,       "QP depth exceeds supported range (-QpBDOffsety to 51)" );
  }
#else
  xConfirmPara( m_aiQP[0] < 0 || m_aiQP[0] > 51,                                             "QP exceeds supported range (0 to 51)" );
  if ( m_aiQP.size() >= 2 )
  {
    xConfirmPara( m_aiQP[1] < 0 || m_aiQP[1] > 51,                                           "QP Depth exceeds supported range (0 to 51)" );
  }
#endif
  xConfirmPara( m_iALFEncodePassReduction < 0 || m_iALFEncodePassReduction > 2,             "ALFEncodePassReduction must be equal to 0, 1 or 2");
#if LCU_SYNTAX_ALF
  xConfirmPara( m_iALFMaxNumberFilters < 1,                                                 "ALFMaxNumFilter should be larger than 1");  
#else
  xConfirmPara( m_iALFMaxNumberFilters < 1 || m_iALFMaxNumberFilters > 16,                  "ALFMaxNumFilter exceeds supported range (1 to 16)");  
#endif
  xConfirmPara( m_loopFilterBetaOffsetDiv2 < -13 || m_loopFilterBetaOffsetDiv2 > 13,          "Loop Filter Beta Offset div. 2 exceeds supported range (-13 to 13)");
  xConfirmPara( m_loopFilterTcOffsetDiv2 < -13 || m_loopFilterTcOffsetDiv2 > 13,              "Loop Filter Tc Offset div. 2 exceeds supported range (-13 to 13)");
  xConfirmPara( m_iFastSearch < 0 || m_iFastSearch > 2,                                     "Fast Search Mode is not supported value (0:Full search  1:Diamond  2:PMVFAST)" );
  xConfirmPara( m_iSearchRange < 0 ,                                                        "Search Range must be more than 0" );
  xConfirmPara( m_bipredSearchRange < 0 ,                                                   "Search Range must be more than 0" );
  xConfirmPara( m_iMaxDeltaQP > 7,                                                          "Absolute Delta QP exceeds supported range (0 to 7)" );
  xConfirmPara( m_iMaxCuDQPDepth > m_uiMaxCUDepth - 1,                                          "Absolute depth for a minimum CuDQP exceeds maximum coding unit depth" );

  xConfirmPara( m_iChromaQpOffset    < -12,   "Min. Chroma Qp Offset is -12"     );
  xConfirmPara( m_iChromaQpOffset    >  12,   "Max. Chroma Qp Offset is  12"     );
  xConfirmPara( m_iChromaQpOffset2nd < -12,   "Min. Chroma Qp Offset 2nd is -12" );
  xConfirmPara( m_iChromaQpOffset2nd >  12,   "Max. Chroma Qp Offset 2nd is  12" );

  xConfirmPara( m_iQPAdaptationRange <= 0,                                                  "QP Adaptation Range must be more than 0" );
  if (m_iDecodingRefreshType == 2)
  {
    xConfirmPara( m_iIntraPeriod > 0 && m_iIntraPeriod <= m_iGOPSize ,                      "Intra period must be larger than GOP size for periodic IDR pictures");
  }
  xConfirmPara( (m_uiMaxCUWidth  >> m_uiMaxCUDepth) < 4,                                    "Minimum partition width size should be larger than or equal to 8");
  xConfirmPara( (m_uiMaxCUHeight >> m_uiMaxCUDepth) < 4,                                    "Minimum partition height size should be larger than or equal to 8");
  xConfirmPara( m_uiMaxCUWidth < 16,                                                        "Maximum partition width size should be larger than or equal to 16");
  xConfirmPara( m_uiMaxCUHeight < 16,                                                       "Maximum partition height size should be larger than or equal to 16");
#if PIC_CROPPING
  xConfirmPara( (m_iSourceWidth  % (m_uiMaxCUWidth  >> (m_uiMaxCUDepth-1)))!=0,             "Resulting coded frame width must be a multiple of the minimum CU size");
  xConfirmPara( (m_iSourceHeight % (m_uiMaxCUHeight >> (m_uiMaxCUDepth-1)))!=0,             "Resulting coded frame height must be a multiple of the minimum CU size");
#else
  xConfirmPara( (m_iSourceWidth  % (m_uiMaxCUWidth  >> (m_uiMaxCUDepth-1)))!=0,             "Frame width should be multiple of minimum CU size");
  xConfirmPara( (m_iSourceHeight % (m_uiMaxCUHeight >> (m_uiMaxCUDepth-1)))!=0,             "Frame height should be multiple of minimum CU size");
#endif
  
  xConfirmPara( m_uiQuadtreeTULog2MinSize < 2,                                        "QuadtreeTULog2MinSize must be 2 or greater.");
  xConfirmPara( m_uiQuadtreeTULog2MinSize > 5,                                        "QuadtreeTULog2MinSize must be 5 or smaller.");
  xConfirmPara( m_uiQuadtreeTULog2MaxSize < 2,                                        "QuadtreeTULog2MaxSize must be 2 or greater.");
  xConfirmPara( m_uiQuadtreeTULog2MaxSize > 5,                                        "QuadtreeTULog2MaxSize must be 5 or smaller.");
  xConfirmPara( m_uiQuadtreeTULog2MaxSize < m_uiQuadtreeTULog2MinSize,                "QuadtreeTULog2MaxSize must be greater than or equal to m_uiQuadtreeTULog2MinSize.");
  xConfirmPara( (1<<m_uiQuadtreeTULog2MinSize)>(m_uiMaxCUWidth >>(m_uiMaxCUDepth-1)), "QuadtreeTULog2MinSize must not be greater than minimum CU size" ); // HS
  xConfirmPara( (1<<m_uiQuadtreeTULog2MinSize)>(m_uiMaxCUHeight>>(m_uiMaxCUDepth-1)), "QuadtreeTULog2MinSize must not be greater than minimum CU size" ); // HS
  xConfirmPara( ( 1 << m_uiQuadtreeTULog2MinSize ) > ( m_uiMaxCUWidth  >> m_uiMaxCUDepth ), "Minimum CU width must be greater than minimum transform size." );
  xConfirmPara( ( 1 << m_uiQuadtreeTULog2MinSize ) > ( m_uiMaxCUHeight >> m_uiMaxCUDepth ), "Minimum CU height must be greater than minimum transform size." );
  xConfirmPara( m_uiQuadtreeTUMaxDepthInter < 1,                                                         "QuadtreeTUMaxDepthInter must be greater than or equal to 1" );
  xConfirmPara( m_uiQuadtreeTUMaxDepthInter > m_uiQuadtreeTULog2MaxSize - m_uiQuadtreeTULog2MinSize + 1, "QuadtreeTUMaxDepthInter must be less than or equal to the difference between QuadtreeTULog2MaxSize and QuadtreeTULog2MinSize plus 1" );
  xConfirmPara( m_uiQuadtreeTUMaxDepthIntra < 1,                                                         "QuadtreeTUMaxDepthIntra must be greater than or equal to 1" );
  xConfirmPara( m_uiQuadtreeTUMaxDepthIntra > m_uiQuadtreeTULog2MaxSize - m_uiQuadtreeTULog2MinSize + 1, "QuadtreeTUMaxDepthIntra must be less than or equal to the difference between QuadtreeTULog2MaxSize and QuadtreeTULog2MinSize plus 1" );

  xConfirmPara( m_iNumberOfViews > MAX_VIEW_NUM ,                                     "NumberOfViews must be less than or equal to MAX_VIEW_NUM");
  xConfirmPara    ( Int( m_pchInputFileList.size() ) < m_iNumberOfViews,              "Number of InputFiles must be greater than or equal to NumberOfViews" );
  xConfirmPara    ( Int( m_pchReconFileList.size() ) < m_iNumberOfViews,              "Number of ReconFiles must be greater than or equal to NumberOfViews" );
  xConfirmPara    ( m_iCodedCamParPrecision < 0 || m_iCodedCamParPrecision > 5,       "CodedCamParsPrecision must be in range of 0..5" );
#if HHI_INTERVIEW_SKIP
  xConfirmPara    ( m_bInterViewSkip && !m_bUsingDepthMaps,                       "RenderingSkipMode requires CodeDepthMaps = 1" );
#endif
#if DEPTH_MAP_GENERATION
  xConfirmPara    ( m_uiPredDepthMapGeneration > 2,                                   "PredDepthMapGen must be less than or equal to 2" );
  xConfirmPara    ( m_uiPredDepthMapGeneration >= 2 && !m_bUsingDepthMaps,            "PredDepthMapGen >= 2 requires CodeDepthMaps = 1" );
#endif
#if HHI_INTER_VIEW_MOTION_PRED
  xConfirmPara    ( m_uiMultiviewMvPredMode > 7,                                      "MultiviewMvPred must be less than or equal to 7" );
  xConfirmPara    ( m_uiMultiviewMvPredMode > 0 && m_uiPredDepthMapGeneration == 0 ,  "MultiviewMvPred > 0 requires PredDepthMapGen > 0" );
  xConfirmPara    ( m_uiMultiviewMvRegMode       > 1,                                 "MultiviewMvRegMode must be less than or equal to 1" );
  xConfirmPara    ( m_dMultiviewMvRegLambdaScale < 0.0,                               "MultiviewMvRegLambdaScale must not be negative" );
  if( m_uiMultiviewMvRegMode )
  {
    xConfirmPara  ( Int( m_pchDepthInputFileList.size() ) < m_iNumberOfViews,         "MultiviewMvRegMode > 0 requires the presence of input depth maps" );
  }
#endif
#if HHI_INTER_VIEW_RESIDUAL_PRED
  xConfirmPara    ( m_uiMultiviewResPredMode > 1,                                     "MultiviewResPred must be less than or equal to 1" );
  xConfirmPara    ( m_uiMultiviewResPredMode > 0 && m_uiPredDepthMapGeneration == 0 , "MultiviewResPred > 0 requires PredDepthMapGen > 0" );
#endif
  if( m_bUsingDepthMaps )
  {
    xConfirmPara  ( Int( m_pchDepthInputFileList.size() ) < m_iNumberOfViews,         "Number of DepthInputFiles must be greater than or equal to NumberOfViews" );
    xConfirmPara  ( Int( m_pchDepthReconFileList.size() ) < m_iNumberOfViews,         "Number of DepthReconFiles must be greater than or equal to NumberOfViews" );

#if HHI_VSO
    if( m_bUseVSO )
    {
      xConfirmPara( m_pchCameraParameterFile    == 0                             ,   "CameraParameterFile must be given");
      xConfirmPara(   m_pchVSOConfig            == 0                             ,   "VSO Setup string must be given");
      xConfirmPara( m_pchBaseViewCameraNumbers  == 0                             ,   "BaseViewCameraNumbers must be given" );
      xConfirmPara( m_iNumberOfViews != m_cCameraData.getBaseViewNumbers().size(),   "Number of Views in BaseViewCameraNumbers must be equal to NumberOfViews" );
      xConfirmPara( m_uiVSOMode > 4 ,                                                "VSO Mode must be less than 5");
    }
#endif
  }
#if ADAPTIVE_QP_SELECTION
#if H0736_AVC_STYLE_QP_RANGE
  xConfirmPara( m_bUseAdaptQpSelect == true && m_aiQP[0] < 0,                                              "AdaptiveQpSelection must be disabled when QP < 0.");
  xConfirmPara( m_bUseAdaptQpSelect == true && m_aiQP[1] < 0,                                              "AdaptiveQpSelection must be disabled when QP < 0.");
  xConfirmPara( m_bUseAdaptQpSelect == true && (m_iChromaQpOffset !=0 || m_iChromaQpOffset2nd != 0 ),  "AdaptiveQpSelection must be disabled when ChromaQpOffset is not equal to 0.");
#endif
#endif

  if( m_usePCM)
  {
    xConfirmPara(  m_uiPCMLog2MinSize < 3,                                      "PCMLog2MinSize must be 3 or greater.");
    xConfirmPara(  m_uiPCMLog2MinSize > 5,                                      "PCMLog2MinSize must be 5 or smaller.");
    xConfirmPara(  m_pcmLog2MaxSize > 5,                                        "PCMLog2MaxSize must be 5 or smaller.");
    xConfirmPara(  m_pcmLog2MaxSize < m_uiPCMLog2MinSize,                       "PCMLog2MaxSize must be equal to or greater than m_uiPCMLog2MinSize.");
  }

#if FIXED_NUMBER_OF_TILES_SLICE_MODE
  xConfirmPara( m_iSliceMode < 0 || m_iSliceMode > 3, "SliceMode exceeds supported range (0 to 3)" );
#endif
  if (m_iSliceMode!=0)
  {
    xConfirmPara( m_iSliceArgument < 1 ,         "SliceArgument should be larger than or equal to 1" );
  }
#if FIXED_NUMBER_OF_TILES_SLICE_MODE
  if (m_iSliceMode==3)
  {
    xConfirmPara( m_iSliceGranularity > 0 ,      "When SliceMode == 3 is chosen, the SliceGranularity must be 0" );
  }
#endif
  xConfirmPara( m_iEntropySliceMode < 0 || m_iEntropySliceMode > 2, "EntropySliceMode exceeds supported range (0 to 2)" );
  if (m_iEntropySliceMode!=0)
  {
    xConfirmPara( m_iEntropySliceArgument < 1 ,         "EntropySliceArgument should be larger than or equal to 1" );
  }
  xConfirmPara( m_iSliceGranularity >= m_uiMaxCUDepth, "SliceGranularity must be smaller than maximum cu depth");
  xConfirmPara( m_iSliceGranularity <0 || m_iSliceGranularity > 3, "SliceGranularity exceeds supported range (0 to 3)" );
  xConfirmPara( m_iSliceGranularity > m_iMaxCuDQPDepth, "SliceGranularity must be smaller smaller than or equal to maximum dqp depth" );

#if NO_COMBINED_PARALLEL
  bool tileFlag = (m_iNumColumnsMinus1 > 0 || m_iNumRowsMinus1 > 0 );
  xConfirmPara( tileFlag && m_iEntropySliceMode,            "Tile and Entropy Slice can not be applied together");
  xConfirmPara( tileFlag && m_iWaveFrontSynchro,            "Tile and Wavefront can not be applied together");
  xConfirmPara( m_iWaveFrontSynchro && m_iEntropySliceMode, "Wavefront and Entropy Slice can not be applied together");  
#endif

  // max CU width and height should be power of 2
  UInt ui = m_uiMaxCUWidth;
  while(ui)
  {
    ui >>= 1;
    if( (ui & 1) == 1)
      xConfirmPara( ui != 1 , "Width should be 2^n");
  }
  ui = m_uiMaxCUHeight;
  while(ui)
  {
    ui >>= 1;
    if( (ui & 1) == 1)
      xConfirmPara( ui != 1 , "Height should be 2^n");
  }
  
  xConfirmPara( m_iWaveFrontSynchro < 0, "WaveFrontSynchro cannot be negative" );
  xConfirmPara( m_iWaveFrontFlush < 0, "WaveFrontFlush cannot be negative" );
  xConfirmPara( m_iWaveFrontSubstreams <= 0, "WaveFrontSubstreams must be positive" );
  xConfirmPara( m_iWaveFrontSubstreams > 1 && !m_iWaveFrontSynchro, "Must have WaveFrontSynchro > 0 in order to have WaveFrontSubstreams > 1" );

#undef xConfirmPara
  if (check_failed)
  {
    exit(EXIT_FAILURE);
  }

  xCheckCodingStructureMvc();
}

Void TAppEncCfg::xCheckCodingStructureMvc()
{
  bool check_failed = false; /* abort if there is a fatal configuration problem */
#define xConfirmPara(a,b) check_failed |= confirmPara(a,b)

  // validate that POC of same frame is identical across multiple views
  Bool bErrorMvePoc = false;
  if( m_iNumberOfViews > 1 )
  {
    for( Int k = 1; k < m_iNumberOfViews; k++ )
    {
      for( Int i = 0; i < MAX_GOP; i++ )
      {
        if( m_GOPListsMvc[k][i].m_POC != m_GOPListsMvc[0][i].m_POC )
        {
          printf( "\nError: Frame%d_v%d POC %d is not identical to Frame%d POC\n", i, k, m_GOPListsMvc[k][i].m_POC, i );
          bErrorMvePoc = true;
        }
      }
    }
  }
  xConfirmPara( bErrorMvePoc,  "Invalid inter-view POC structure given" );

  // validate that baseview has no inter-view refs 
  Bool bErrorIvpBase = false;
  for( Int i = 0; i < MAX_GOP; i++ )
  {
    if( m_GOPListsMvc[0][i].m_numInterViewRefPics != 0 )
    {
      printf( "\nError: Frame%d inter_view refs not available in view 0\n", i );
      bErrorIvpBase = true;
    }
  }
  xConfirmPara( bErrorIvpBase, "Inter-view refs not possible in base view" );

  // validate inter-view refs
  Bool bErrorIvpEnhV = false;
  if( m_iNumberOfViews > 1 )
  {
    for( Int k = 1; k < m_iNumberOfViews; k++ )
    {
      for( Int i = 0; i < MAX_GOP+1; i++ )
      {
        for( Int j = 0; j < m_GOPListsMvc[k][i].m_numInterViewRefPics; j++ )
        {
          Int iAbsViewId = m_GOPListsMvc[k][i].m_interViewRefs[j] + k;
          if( iAbsViewId < 0 || iAbsViewId >= k )
          {
            printf( "\nError: inter-view ref pic %d is not available for Frame%d_v%d\n", m_GOPListsMvc[k][i].m_interViewRefs[j], i, k );
            bErrorIvpEnhV = true;
          }
          if( m_GOPListsMvc[k][i].m_interViewRefPosL0[j] < 0 || m_GOPListsMvc[k][i].m_interViewRefPosL0[j] > m_GOPListsMvc[k][i].m_numRefPicsActive )
          {
            printf( "\nError: inter-view ref pos %d on L0 is not available for Frame%d_v%d\n", m_GOPListsMvc[k][i].m_interViewRefPosL0[j], i, k );
            bErrorIvpEnhV = true;
          }
          if( m_GOPListsMvc[k][i].m_interViewRefPosL1[j] < 0 || m_GOPListsMvc[k][i].m_interViewRefPosL1[j] > m_GOPListsMvc[k][i].m_numRefPicsActive )
          {
            printf( "\nError: inter-view ref pos %d on L1 is not available for Frame%d_v%d\n", m_GOPListsMvc[k][i].m_interViewRefPosL1[j], i, k );
            bErrorIvpEnhV = true;
          }
        }
        if( i == MAX_GOP ) // inter-view refs at I pic position in base view
        {
          if( m_GOPListsMvc[k][MAX_GOP].m_sliceType != 'B' && m_GOPListsMvc[k][MAX_GOP].m_sliceType != 'P' && m_GOPListsMvc[k][MAX_GOP].m_sliceType != 'I' )
          {
            printf( "\nError: slice type of FrameI_v%d must be equal to B or P or I\n", k );
            bErrorIvpEnhV = true;
          }

          if( m_GOPListsMvc[k][MAX_GOP].m_POC != 0 )
          {
            printf( "\nError: POC %d not possible for FrameI_v%d, must be 0\n", m_GOPListsMvc[k][MAX_GOP].m_POC, k );
            bErrorIvpEnhV = true;
          }

          if( m_GOPListsMvc[k][MAX_GOP].m_temporalId != 0 )
          {
            printf( "\nWarning: Temporal id of FrameI_v%d must be 0 (cp. I-frame in base view)\n", k );
            m_GOPListsMvc[k][MAX_GOP].m_temporalId = 0;
          }

          if( !(m_GOPListsMvc[k][MAX_GOP].m_refPic) )
          {
            printf( "\nWarning: FrameI_v%d must be ref pic (cp. I-frame in base view)\n", k );
            m_GOPListsMvc[k][MAX_GOP].m_refPic = true;
          }

          if( m_GOPListsMvc[k][MAX_GOP].m_numRefPics != 0 )
          {
            printf( "\nWarning: temporal references not possible for FrameI_v%d\n", k );
            for( Int j = 0; j < m_GOPListsMvc[k][MAX_GOP].m_numRefPics; j++ )
            {
              m_GOPListsMvc[k][MAX_GOP].m_referencePics[j] = 0;
            }
            m_GOPListsMvc[k][MAX_GOP].m_numRefPics = 0;
          }

          if( m_GOPListsMvc[k][MAX_GOP].m_interRPSPrediction )
          {
            printf( "\nError: inter RPS prediction not possible for FrameI_v%d, must be 0\n", k );
            bErrorIvpEnhV = true;
          }

          if( m_GOPListsMvc[k][MAX_GOP].m_sliceType == 'I' && m_GOPListsMvc[k][MAX_GOP].m_numInterViewRefPics != 0 )
          {
            printf( "\nError: inter-view prediction not possible for FrameI_v%d with slice type I, #IV_ref_pics must be 0\n", k );
            bErrorIvpEnhV = true;
          }

          if( m_GOPListsMvc[k][MAX_GOP].m_numRefPicsActive > m_GOPListsMvc[k][MAX_GOP].m_numInterViewRefPics )
          {
            m_GOPListsMvc[k][MAX_GOP].m_numRefPicsActive = m_GOPListsMvc[k][MAX_GOP].m_numInterViewRefPics;
          }

          if( m_GOPListsMvc[k][MAX_GOP].m_sliceType == 'P' )
          {
            if( m_GOPListsMvc[k][MAX_GOP].m_numInterViewRefPics < 1 )
            {
              printf( "\nError: #IV_ref_pics must be at least one for FrameI_v%d with slice type P\n", k );
              bErrorIvpEnhV = true;
            }
            else
            {
              for( Int j = 0; j < m_GOPListsMvc[k][MAX_GOP].m_numInterViewRefPics; j++ )
              {
                if( m_GOPListsMvc[k][MAX_GOP].m_interViewRefPosL1[j] != 0 )
                {
                  printf( "\nError: inter-view ref pos %d on L1 not possible for FrameI_v%d with slice type P\n", m_GOPListsMvc[k][MAX_GOP].m_interViewRefPosL1[j], k );
                  bErrorIvpEnhV = true;
                }
              }
            }
          }

          if( m_GOPListsMvc[k][MAX_GOP].m_sliceType == 'B' && m_GOPListsMvc[k][MAX_GOP].m_numInterViewRefPics < 1 )
          {
            printf( "\nError: #IV_ref_pics must be at least one for FrameI_v%d with slice type B\n", k );
            bErrorIvpEnhV = true;
          }
        }
      }
    }
  }
  xConfirmPara( bErrorIvpEnhV, "Invalid inter-view coding structure for enhancement views given" );

  // validate temporal coding structure
  if( !bErrorMvePoc && !bErrorIvpBase && !bErrorIvpEnhV )
  {
    for( Int viewId = 0; viewId < m_iNumberOfViews; viewId++ )
    {
      Bool verifiedGOP = false;
      Bool errorGOP    = false;
      Int  checkGOP    = 1;
      Int  numRefs     = 1;
      Int refList[MAX_NUM_REF_PICS+1];
      refList[0] = 0;
      Bool isOK[MAX_GOP];
      for( Int i = 0; i < MAX_GOP; i++ ) { isOK[i] = false; }
      Int numOK = 0;
#if !H0567_DPB_PARAMETERS_PER_TEMPORAL_LAYER
      Int numReorderFramesRequired=0;
      m_maxNumberOfReferencePictures=0;
      Int lastDisp = -1;
#endif
      m_extraRPSs[viewId] = 0;
      //start looping through frames in coding order until we can verify that the GOP structure is correct.
      while( !verifiedGOP && !errorGOP )
      {
        Int curGOP = (checkGOP-1)%m_iGOPSize;
        Int curPOC = ((checkGOP-1)/m_iGOPSize)*m_iGOPSize + m_GOPListsMvc[viewId][curGOP].m_POC;    
        if( m_GOPListsMvc[viewId][curGOP].m_POC < 0 )
        {
          printf( "\nError: found fewer Reference Picture Sets than GOPSize for view %d\n", viewId );
          errorGOP = true;
        }
        else 
        {
          //check that all reference pictures are available, or have a POC < 0 meaning they might be available in the next GOP.
          Bool beforeI = false;
          for( Int i = 0; i < m_GOPListsMvc[viewId][curGOP].m_numRefPics; i++ ) 
          {
            Int absPOC = curPOC + m_GOPListsMvc[viewId][curGOP].m_referencePics[i];
            if( absPOC < 0 )
            {
              beforeI = true;
            }
            else 
            {
              Bool found = false;
              for( Int j = 0; j < numRefs; j++ )
              {
                if( refList[j] == absPOC ) 
                {
                  found = true;
                  for( Int k = 0; k < m_iGOPSize; k++ )
                  {
                    if( absPOC%m_iGOPSize == m_GOPListsMvc[viewId][k].m_POC%m_iGOPSize )
                    {
                      m_GOPListsMvc[viewId][curGOP].m_usedByCurrPic[i] = (m_GOPListsMvc[viewId][k].m_temporalId <= m_GOPListsMvc[viewId][curGOP].m_temporalId);
                    }
                  }
                }
              }
              if( !found )
              {
                printf("\nError: ref pic %d is not available for GOP frame %d of view %d\n", m_GOPListsMvc[viewId][curGOP].m_referencePics[i], curGOP+1, viewId );
                errorGOP = true;
              }
            }
          }
          if( !beforeI && !errorGOP )
          {
            //all ref frames were present
            if( !isOK[curGOP] ) 
            {
              numOK++;
              isOK[curGOP] = true;
              if( numOK == m_iGOPSize )
              {
                verifiedGOP = true;
              }
            }
          }
          else 
          {
            //create a new GOPEntry for this frame containing all the reference pictures that were available (POC > 0)
            m_GOPListsMvc[viewId][m_iGOPSize+m_extraRPSs[viewId]] = m_GOPListsMvc[viewId][curGOP];
            Int newRefs = 0;
            for( Int i = 0; i < m_GOPListsMvc[viewId][curGOP].m_numRefPics; i++ )
            {
              Int absPOC = curPOC + m_GOPListsMvc[viewId][curGOP].m_referencePics[i];
              if( absPOC >= 0 )
              {
                m_GOPListsMvc[viewId][m_iGOPSize+m_extraRPSs[viewId]].m_referencePics[newRefs] = m_GOPListsMvc[viewId][curGOP].m_referencePics[i];
                m_GOPListsMvc[viewId][m_iGOPSize+m_extraRPSs[viewId]].m_usedByCurrPic[newRefs] = m_GOPListsMvc[viewId][curGOP].m_usedByCurrPic[i];
                newRefs++;
              }
            }
            Int numPrefRefs = m_GOPListsMvc[viewId][curGOP].m_numRefPicsActive;

            for( Int offset = -1; offset > -checkGOP; offset-- )
            {
              //step backwards in coding order and include any extra available pictures we might find useful to replace the ones with POC < 0.
              Int offGOP =  (checkGOP - 1 + offset)%m_iGOPSize;
              Int offPOC = ((checkGOP - 1 + offset)/m_iGOPSize) * m_iGOPSize + m_GOPListsMvc[viewId][offGOP].m_POC;
              if( offPOC >= 0 && m_GOPListsMvc[viewId][offGOP].m_refPic && m_GOPListsMvc[viewId][offGOP].m_temporalId <= m_GOPListsMvc[viewId][curGOP].m_temporalId )
              {
                Bool newRef = false;
                for( Int i = 0; i < numRefs; i++ )
                {
                  if( refList[i] == offPOC )
                  {
                    newRef = true;
                  }
                }
                for( Int i = 0; i < newRefs; i++ ) 
                {
                  if( m_GOPListsMvc[viewId][m_iGOPSize+m_extraRPSs[viewId]].m_referencePics[i] == (offPOC - curPOC) )
                  {
                    newRef = false;
                  }
                }
                if( newRef ) 
                {
                  Int insertPoint = newRefs;
                  //this picture can be added, find appropriate place in list and insert it.
                  for( Int j = 0; j < newRefs; j++ )
                  {
                    if( m_GOPListsMvc[viewId][m_iGOPSize+m_extraRPSs[viewId]].m_referencePics[j] < (offPOC - curPOC) || 
                        m_GOPListsMvc[viewId][m_iGOPSize+m_extraRPSs[viewId]].m_referencePics[j] > 0 )
                    {
                      insertPoint = j;
                      break;
                    }
                  }
                  Int prev = offPOC - curPOC;
                  Int prevUsed = (m_GOPListsMvc[viewId][offGOP].m_temporalId <= m_GOPListsMvc[viewId][curGOP].m_temporalId);
                  for( Int j = insertPoint; j < newRefs+1; j++ )
                  {
                    Int newPrev = m_GOPListsMvc[viewId][m_iGOPSize+m_extraRPSs[viewId]].m_referencePics[j];
                    Int newUsed = m_GOPListsMvc[viewId][m_iGOPSize+m_extraRPSs[viewId]].m_usedByCurrPic[j];
                    m_GOPListsMvc[viewId][m_iGOPSize+m_extraRPSs[viewId]].m_referencePics[j] = prev;
                    m_GOPListsMvc[viewId][m_iGOPSize+m_extraRPSs[viewId]].m_usedByCurrPic[j] = prevUsed;
                    prevUsed = newUsed;
                    prev = newPrev;
                  }
                  newRefs++;
                }
              }
              if( newRefs >= numPrefRefs )
              {
                break;
              }
            }
            m_GOPListsMvc[viewId][m_iGOPSize+m_extraRPSs[viewId]].m_numRefPics = newRefs;
            m_GOPListsMvc[viewId][m_iGOPSize+m_extraRPSs[viewId]].m_POC = curPOC;
            if( m_extraRPSs[viewId] == 0 )
            {
              m_GOPListsMvc[viewId][m_iGOPSize+m_extraRPSs[viewId]].m_interRPSPrediction = 0;
              m_GOPListsMvc[viewId][m_iGOPSize+m_extraRPSs[viewId]].m_numRefIdc = 0;
            }
            else
            {
              Int rIdx =  m_iGOPSize + m_extraRPSs[viewId] - 1;
              Int refPOC = m_GOPListsMvc[viewId][rIdx].m_POC;
              Int refPics = m_GOPListsMvc[viewId][rIdx].m_numRefPics;
              Int newIdc = 0;
              for( Int i = 0; i <= refPics; i++ )
              {
                Int deltaPOC = ((i != refPics)? m_GOPListsMvc[viewId][rIdx].m_referencePics[i] : 0);  // check if the reference abs POC is >= 0
                Int absPOCref = refPOC + deltaPOC;
                Int refIdc = 0;
                for( Int j = 0; j < m_GOPListsMvc[viewId][m_iGOPSize+m_extraRPSs[viewId]].m_numRefPics; j++ )
                {
                  if( (absPOCref - curPOC) == m_GOPListsMvc[viewId][m_iGOPSize+m_extraRPSs[viewId]].m_referencePics[j] )
                  {
                    if( m_GOPListsMvc[viewId][m_iGOPSize+m_extraRPSs[viewId]].m_usedByCurrPic[j] )
                    {
                      refIdc = 1;
                    }
                    else
                    {
                      refIdc = 2;
                    }
                  }
                }
                m_GOPListsMvc[viewId][m_iGOPSize+m_extraRPSs[viewId]].m_refIdc[newIdc] = refIdc;
                newIdc++;
              }
              m_GOPListsMvc[viewId][m_iGOPSize+m_extraRPSs[viewId]].m_interRPSPrediction = 1;  
              m_GOPListsMvc[viewId][m_iGOPSize+m_extraRPSs[viewId]].m_numRefIdc = newIdc;
              m_GOPListsMvc[viewId][m_iGOPSize+m_extraRPSs[viewId]].m_deltaRPS = refPOC - m_GOPListsMvc[viewId][m_iGOPSize+m_extraRPSs[viewId]].m_POC; 
              m_GOPListsMvc[viewId][m_iGOPSize+m_extraRPSs[viewId]].m_deltaRIdxMinus1 = 0; 
            }
            curGOP = m_iGOPSize + m_extraRPSs[viewId];
            m_extraRPSs[viewId]++;
          }
          numRefs = 0;
          for( Int i = 0; i < m_GOPListsMvc[viewId][curGOP].m_numRefPics; i++ )
          {
            Int absPOC = curPOC + m_GOPListsMvc[viewId][curGOP].m_referencePics[i];
            if( absPOC >= 0 )
            {
              refList[numRefs] = absPOC;
              numRefs++;
            }
          }
#if !H0567_DPB_PARAMETERS_PER_TEMPORAL_LAYER
          if(m_maxNumberOfReferencePictures<numRefs)
          {
            m_maxNumberOfReferencePictures=numRefs;
          }
#endif
          refList[numRefs] = curPOC;
          numRefs++;
#if !H0567_DPB_PARAMETERS_PER_TEMPORAL_LAYER
          Int nonDisplayed=0;
          for(Int i=0; i<numRefs; i++) 
          {
            if(refList[i]==lastDisp+1) 
            {
              lastDisp=refList[i];
              i=0;
            }
          }
          for(Int i=0; i<numRefs; i++) 
          {
            if(refList[i]>lastDisp)
            {
              nonDisplayed++;
            }
          }
          if(nonDisplayed>numReorderFramesRequired)
          {
            numReorderFramesRequired=nonDisplayed;
          }
#endif
        }
        checkGOP++;
      }
#if !H0567_DPB_PARAMETERS_PER_TEMPORAL_LAYER
      if (m_numReorderFrames == -1)
      {
        m_numReorderFrames = numReorderFramesRequired;
      }
#endif
      xConfirmPara( errorGOP, "Invalid GOP structure given" );
#if H0566_TLA
      m_maxTempLayer[viewId] = 1;
#endif
      for( Int i = 0; i < m_iGOPSize; i++ ) 
      {
#if H0566_TLA
        if( m_GOPListsMvc[viewId][i].m_temporalId >= m_maxTempLayer[viewId] )
        {
          m_maxTempLayer[viewId] = m_GOPListsMvc[viewId][i].m_temporalId + 1;
        }
#endif
        xConfirmPara( m_GOPListsMvc[viewId][i].m_sliceType != 'B' && m_GOPListsMvc[viewId][i].m_sliceType != 'P', "Slice type must be equal to B or P" );
      }

#if H0567_DPB_PARAMETERS_PER_TEMPORAL_LAYER
      for( Int i = 0; i < MAX_TLAYER; i++ )
      {
        m_numReorderPics[viewId][i] = 0;
        m_maxDecPicBuffering[viewId][i] = 0;
      }
      for( Int i = 0; i < m_iGOPSize; i++ ) 
      {
        if( m_GOPListsMvc[viewId][i].m_numRefPics > m_maxDecPicBuffering[viewId][m_GOPListsMvc[viewId][i].m_temporalId] )
        {
          m_maxDecPicBuffering[viewId][m_GOPListsMvc[viewId][i].m_temporalId] = m_GOPListsMvc[viewId][i].m_numRefPics;
        }
        Int highestDecodingNumberWithLowerPOC = 0; 
        for( Int j = 0; j < m_iGOPSize; j++ )
        {
          if( m_GOPListsMvc[viewId][j].m_POC <= m_GOPListsMvc[viewId][i].m_POC )
          {
            highestDecodingNumberWithLowerPOC = j;
          }
        }
        Int numReorder = 0;
        for( Int j = 0; j < highestDecodingNumberWithLowerPOC; j++ )
        {
          if( m_GOPListsMvc[viewId][j].m_temporalId <= m_GOPListsMvc[viewId][i].m_temporalId && 
              m_GOPListsMvc[viewId][j].m_POC        >  m_GOPListsMvc[viewId][i].m_POC )
          {
            numReorder++;
          }
        }    
        if( numReorder > m_numReorderPics[viewId][m_GOPListsMvc[viewId][i].m_temporalId] )
        {
          m_numReorderPics[viewId][m_GOPListsMvc[viewId][i].m_temporalId] = numReorder;
        }
      }
      for( Int i = 0; i < MAX_TLAYER-1; i++ ) 
      {
        // a lower layer can not have higher value of m_numReorderPics than a higher layer
        if( m_numReorderPics[viewId][i+1] < m_numReorderPics[viewId][i] )
        {
          m_numReorderPics[viewId][i+1] = m_numReorderPics[viewId][i];
        }
        // the value of num_reorder_pics[ i ] shall be in the range of 0 to max_dec_pic_buffering[ i ], inclusive
        if( m_numReorderPics[viewId][i] > m_maxDecPicBuffering[viewId][i] )
        {
          m_maxDecPicBuffering[viewId][i] = m_numReorderPics[viewId][i];
        }
        // a lower layer can not have higher value of m_uiMaxDecPicBuffering than a higher layer
        if( m_maxDecPicBuffering[viewId][i+1] < m_maxDecPicBuffering[viewId][i] )
        {
          m_maxDecPicBuffering[viewId][i+1] = m_maxDecPicBuffering[viewId][i];
        }
      }
      // the value of num_reorder_pics[ i ] shall be in the range of 0 to max_dec_pic_buffering[ i ], inclusive
      if( m_numReorderPics[viewId][MAX_TLAYER-1] > m_maxDecPicBuffering[viewId][MAX_TLAYER-1] )
      {
        m_maxDecPicBuffering[viewId][MAX_TLAYER-1] = m_numReorderPics[viewId][MAX_TLAYER-1];
      }
#endif

#if H0567_DPB_PARAMETERS_PER_TEMPORAL_LAYER
      xConfirmPara( m_bUseLComb == false && m_numReorderPics[viewId][MAX_TLAYER-1] != 0, "ListCombination can only be 0 in low delay coding (more precisely when L0 and L1 are identical)" );  // Note however this is not the full necessary condition as ref_pic_list_combination_flag can only be 0 if L0 == L1.
#else
      xConfirmPara( m_bUseLComb==false && m_numReorderFrames!=0, "ListCombination can only be 0 in low delay coding (more precisely when L0 and L1 are identical)" );  // Note however this is not the full necessary condition as ref_pic_list_combination_flag can only be 0 if L0 == L1.
      xConfirmPara( m_numReorderFrames < numReorderFramesRequired, "For the used GOP the encoder requires more pictures for reordering than specified in MaxNumberOfReorderPictures" );
#endif
    }
  }
#undef xConfirmPara
  if( check_failed )
  {
    exit(EXIT_FAILURE);
  }
}

template <class T>
Void
TAppEncCfg::xCleanUpVector( std::vector<T>& rcVec, const T& rcInvalid )
{
  Int iFirstInv = (Int)rcVec.size();
  for( Int iIdx = 0; iIdx < (Int)rcVec.size(); iIdx++ )
  {
    if( rcVec[ iIdx ] == rcInvalid )
    {
      iFirstInv = iIdx;
      break;
    }
  }
  while( (Int)rcVec.size() > iFirstInv )
  {
    rcVec.pop_back();
  }
}

Void
TAppEncCfg::xCleanUpVectors()
{
  xCleanUpVector( m_pchInputFileList,       (char*)0 );
  xCleanUpVector( m_pchDepthInputFileList,  (char*)0 );
  xCleanUpVector( m_pchReconFileList,       (char*)0 );
  xCleanUpVector( m_pchDepthReconFileList,  (char*)0 );
}

/** \todo use of global variables should be removed later
 */
Void TAppEncCfg::xSetGlobal()
{
  // set max CU width & height
  g_uiMaxCUWidth  = m_uiMaxCUWidth;
  g_uiMaxCUHeight = m_uiMaxCUHeight;
  
  // compute actual CU depth with respect to config depth and max transform size
  g_uiAddCUDepth  = 0;
  while( (m_uiMaxCUWidth>>m_uiMaxCUDepth) > ( 1 << ( m_uiQuadtreeTULog2MinSize + g_uiAddCUDepth )  ) ) g_uiAddCUDepth++;
  
  m_uiMaxCUDepth += g_uiAddCUDepth;
  g_uiAddCUDepth++;
  g_uiMaxCUDepth = m_uiMaxCUDepth;
  
  // set internal bit-depth and constants
#if FULL_NBIT
  g_uiBitDepth = m_uiInternalBitDepth;
  g_uiBitIncrement = 0;
#else
  g_uiBitDepth = 8;
  g_uiBitIncrement = m_uiInternalBitDepth - g_uiBitDepth;
#endif

#if HHI_DMM_WEDGE_INTRA || HHI_DMM_PRED_TEX
  g_iDeltaDCsQuantOffset = g_uiBitIncrement - 2;
#endif

  g_uiBASE_MAX     = ((1<<(g_uiBitDepth))-1);
  
#if IBDI_NOCLIP_RANGE
  g_uiIBDI_MAX     = g_uiBASE_MAX << g_uiBitIncrement;
#else
  g_uiIBDI_MAX     = ((1<<(g_uiBitDepth+g_uiBitIncrement))-1);
#endif
  
  if (m_uiOutputBitDepth == 0)
  {
    m_uiOutputBitDepth = m_uiInternalBitDepth;
  }

  g_uiPCMBitDepthLuma = m_uiPCMBitDepthLuma = ((m_bPCMInputBitDepthFlag)? m_uiInputBitDepth : m_uiInternalBitDepth);
  g_uiPCMBitDepthChroma = ((m_bPCMInputBitDepthFlag)? m_uiInputBitDepth : m_uiInternalBitDepth);
}

Void TAppEncCfg::xPrintParameter()
{
  printf("\n");
  for( Int iCounter = 0; iCounter< m_iNumberOfViews; iCounter++)
  {
    printf("Texture Input File %i            : %s\n", iCounter, m_pchInputFileList[iCounter]);
  }
  if( m_bUsingDepthMaps )
  {
    for( Int iCounter = 0; iCounter < m_iNumberOfViews; iCounter++)
    {
      printf("Depth Input File %i              : %s\n", iCounter, m_pchDepthInputFileList[iCounter]);
    }
  }
  printf("Bitstream File                  : %s\n", m_pchBitstreamFile      );
  for( Int iCounter = 0; iCounter< m_iNumberOfViews; iCounter++)
  {
    printf("Texture Reconstruction File %i   : %s\n", iCounter, m_pchReconFileList[iCounter]);
  }
  if( m_bUsingDepthMaps )
  {
    for( Int iCounter = 0; iCounter< m_iNumberOfViews; iCounter++)
    {
      printf("Depth Reconstruction File %i     : %s\n", iCounter, m_pchDepthReconFileList[iCounter]);
    }
  }
#if PIC_CROPPING
  printf("Real     Format              : %dx%d %dHz\n", m_iSourceWidth - m_cropLeft - m_cropRight, m_iSourceHeight - m_cropTop - m_cropBottom, m_iFrameRate );
#else
  printf("Real     Format              : %dx%d %dHz\n", m_iSourceWidth - m_aiPad[0], m_iSourceHeight-m_aiPad[1], m_iFrameRate );
#endif
  printf("Internal Format              : %dx%d %dHz\n", m_iSourceWidth, m_iSourceHeight, m_iFrameRate );
  printf("Frame index                  : %u - %d (%d frames)\n", m_FrameSkip, m_FrameSkip+m_iFrameToBeEncoded-1, m_iFrameToBeEncoded );
  printf("CU size / depth              : %d / %d\n", m_uiMaxCUWidth, m_uiMaxCUDepth );
  printf("RQT trans. size (min / max)  : %d / %d\n", 1 << m_uiQuadtreeTULog2MinSize, 1 << m_uiQuadtreeTULog2MaxSize );
  printf("Max RQT depth inter          : %d\n", m_uiQuadtreeTUMaxDepthInter);
  printf("Max RQT depth intra          : %d\n", m_uiQuadtreeTUMaxDepthIntra);
  printf("Min PCM size                 : %d\n", 1 << m_uiPCMLog2MinSize);
  printf("Motion search range          : %d\n", m_iSearchRange );
  printf("Intra period                 : %d\n", m_iIntraPeriod );
  printf("Decoding refresh type        : %d\n", m_iDecodingRefreshType );
  printf("QP Texture                   : %5.2f\n", m_adQP[0] );
  if( m_bUsingDepthMaps )
  {
    printf("QP Depth                     : %5.2f\n", m_adQP[ m_adQP.size()  < 2 ? 0 : 1] );
  }
  printf("Max dQP signaling depth      : %d\n", m_iMaxCuDQPDepth);

  printf("Chroma Qp Offset             : %d\n", m_iChromaQpOffset   );
  printf("Chroma Qp Offset 2nd         : %d\n", m_iChromaQpOffset2nd);

  printf("QP adaptation                : %d (range=%d)\n", m_bUseAdaptiveQP, (m_bUseAdaptiveQP ? m_iQPAdaptationRange : 0) );
  printf("GOP size                     : %d\n", m_iGOPSize );
  printf("Internal bit depth           : %d\n", m_uiInternalBitDepth );
  printf("PCM sample bit depth         : %d\n", m_uiPCMBitDepthLuma );
  if((m_uiMaxCUWidth >> m_uiMaxCUDepth) == 4)
  {
    printf("DisableInter4x4              : %d\n", m_bDisInter4x4);  
  }

printf("Loop Filter Disabled         : %d %d\n", m_abLoopFilterDisable[0] ? 1 : 0,  m_abLoopFilterDisable[1] ? 1 : 0 );
  printf("Coded Camera Param. Precision: %d\n", m_iCodedCamParPrecision);

#if HHI_VSO
  printf("Force use of Lambda Scale    : %d\n", m_bForceLambdaScaleVSO );

  if ( m_bUseVSO )
  {
    printf("VSO Lambda Scale             : %5.2f\n", m_dLambdaScaleVSO );
    printf("VSO Mode                     : %d\n",    m_uiVSOMode       );
    printf("VSO Config                   : %s\n",    m_pchVSOConfig    );
#if HHI_VSO_DIST_INT
    printf("VSO Negative Distortion      : %d\n",    m_bAllowNegDist ? 1 : 0);
#endif
#if HHI_VSO_LS_TABLE
    printf("VSO LS Table                 : %d\n",    m_bVSOLSTable ? 1 : 0);    
#endif
#if SAIT_VSO_EST_A0033
    printf("VSO Estimated VSD            : %d\n",    m_bUseEstimatedVSD ? 1 : 0);        
#endif
#if LGE_VSO_EARLY_SKIP_A0093
    printf("VSO Early Skip               : %d\n",    m_bVSOEarlySkip ? 1 : 0);    
#endif
   
  }
#endif
#if HHI_INTERVIEW_SKIP
    printf("InterView Skip:              : %d\n",    m_bInterViewSkip ? 1:0 );
    printf("InterView Skip Lambda Scale  : %f\n",    m_dInterViewSkipLambdaScale );
#endif

  printf("\n");
  
  printf("TOOL CFG General: ");  
#if LCU_SYNTAX_ALF
  printf("ALFMNF:%d ", m_iALFMaxNumberFilters);
  printf("ALFInSlice:%d ", m_bALFParamInSlice);
  printf("ALFPicEnc:%d ", m_bALFPicBasedEncode);
#endif
  printf("IBD:%d ", !!g_uiBitIncrement);
  printf("HAD:%d ", m_bUseHADME           );
  printf("SRD:%d ", m_bUseSBACRD          );
  printf("SQP:%d ", m_uiDeltaQpRD         );
  printf("ASR:%d ", m_bUseASR             );
#if !PIC_CROPPING
  printf("PAD:%d ", m_bUsePAD             );
#endif
  printf("LComb:%d ", m_bUseLComb         );
  printf("LCMod:%d ", m_bLCMod         );
  printf("FEN:%d ", m_bUseFastEnc         );
  printf("ECU:%d ", m_bUseEarlyCU         );
#if FAST_DECISION_FOR_MRG_RD_COST
  printf("FDM:%d ", m_useFastDecisionForMerge );
#endif
  printf("CFM:%d ", m_bUseCbfFastMode         );
  printf("RQT:%d ", 1     );
  printf("LMC:%d ", m_bUseLMChroma        ); 
  printf("Slice: G=%d M=%d ", m_iSliceGranularity, m_iSliceMode);
  if (m_iSliceMode!=0)
  {
    printf("A=%d ", m_iSliceArgument);
  }
  printf("EntropySlice: M=%d ",m_iEntropySliceMode);
  if (m_iEntropySliceMode!=0)
  {
    printf("A=%d ", m_iEntropySliceArgument);
  }
  printf("CIP:%d ", m_bUseConstrainedIntraPred);
#if BURST_IPCM
  printf("PCM:%d ", (m_usePCM && (1<<m_uiPCMLog2MinSize) <= m_uiMaxCUWidth)? 1 : 0);
#else
  printf("PCM:%d ", ((1<<m_uiPCMLog2MinSize) <= m_uiMaxCUWidth)? 1 : 0);
#endif
#if SAO_UNIT_INTERLEAVING
  printf("SAOInterleaving:%d ", (m_saoInterleavingFlag)?(1):(0));
#endif
#if LOSSLESS_CODING
  printf("LosslessCuEnabled:%d ", (m_useLossless)? 1:0 );
#endif  
  printf("WPP:%d ", (Int)m_bUseWeightPred);
  printf("WPB:%d ", m_uiBiPredIdc);
#if !REMOVE_TILE_DEPENDENCE
  printf("TileBoundaryIndependence:%d ", m_iTileBoundaryIndependenceIdr ); 
#endif
  printf("TileLocationInSliceHdr:%d ", m_iTileLocationInSliceHeaderFlag);
  printf("TileMarker:%d", m_iTileMarkerFlag);
  if (m_iTileMarkerFlag)
  {
    printf("[%d] ", m_iMaxTileMarkerEntryPoints);
  }
  else
  {
    printf(" ");
  }
  printf(" WaveFrontSynchro:%d WaveFrontFlush:%d WaveFrontSubstreams:%d",
          m_iWaveFrontSynchro, m_iWaveFrontFlush, m_iWaveFrontSubstreams);
  printf(" ScalingList:%d ", m_useScalingListId );

  printf("TMVP:%d ", m_enableTMVP     );

#if ADAPTIVE_QP_SELECTION
  printf("AQpS:%d", m_bUseAdaptQpSelect   );
#endif

#if MULTIBITS_DATA_HIDING
  printf(" SignBitHidingFlag:%d SignBitHidingThreshold:%d", m_signHideFlag, m_signHidingThreshold);
#endif
  printf("\n");
  printf("TOOL CFG VIDEO  : ");
  printf("ALF:%d ", (m_abUseALF [0] ? 1 : 0) );
  printf("SAO:%d ", (m_abUseSAO [0] ? 1 : 0));
  printf("RDQ:%d ", (m_abUseRDOQ[0] ? 1 : 0) );
  printf("\n");

  printf("TOOL CFG DEPTH  : ");
  printf("ALF:%d ", (m_abUseALF [1] ? 1 : 0));
  printf("SAO:%d ", (m_abUseSAO [1] ? 1 : 0));
  printf("RDQ:%d ", (m_abUseRDOQ[1] ? 1 : 0));
#if HHI_VSO
  printf("VSO:%d ", m_bUseVSO             );
#endif
#if LGE_WVSO_A0119
  printf("WVSO:%d ", m_bUseWVSO );
#endif
#if OL_DEPTHLIMIT_A0044
  printf("DPL:%d ", m_bDepthPartitionLimiting);
#endif
#if HHI_DMM_WEDGE_INTRA || HHI_DMM_PRED_TEX
  printf("DMM:%d ", m_bUseDMM );
#endif
#if HHI_MPI
  printf("MVI:%d ", m_bUseMVI ? 1 : 0 );
#endif
#if LGE_WVSO_A0119
  if ( m_bUseWVSO )
    printf("\nVSO : VSD : SAD weight = %d : %d : %d ", m_iVSOWeight, m_iVSDWeight, m_iDWeight );
#endif
  printf("\n\n");
  
  fflush(stdout);
}

Void TAppEncCfg::xPrintUsage()
{
  printf( "          <name> = ALF - adaptive loop filter\n");
  printf( "                   IBD - bit-depth increasement\n");
  printf( "                   GPB - generalized B instead of P in low-delay mode\n");
  printf( "                   HAD - hadamard ME for fractional-pel\n");
  printf( "                   SRD - SBAC based RD estimation\n");
  printf( "                   RDQ - RDOQ\n");
  printf( "                   LDC - low-delay mode\n");
  printf( "                   NRF - non-reference frame marking in last layer\n");
  printf( "                   BQP - hier-P style QP assignment in low-delay mode\n");
  printf( "                   PAD - automatic source padding of multiple of 16\n");
  printf( "                   ASR - adaptive motion search range\n");
  printf( "                   FEN - fast encoder setting\n");  
  printf( "                   ECU - Early CU setting\n");
  printf( "                   CFM - Cbf fast mode setting\n");
  printf( "                   LMC - intra chroma prediction based on luma\n");
  printf( "\n" );
  printf( "  Example 1) TAppEncoder.exe -c test.cfg -q 32 -g 8 -f 9 -s 64 -h 4\n");
  printf("              -> QP 32, hierarchical-B GOP 8, 9 frames, 64x64-8x8 CU (~4x4 PU)\n\n");
  printf( "  Example 2) TAppEncoder.exe -c test.cfg -q 32 -g 4 -f 9 -s 64 -h 4 -1 LDC\n");
  printf("              -> QP 32, hierarchical-P GOP 4, 9 frames, 64x64-8x8 CU (~4x4 PU)\n\n");
}

Bool confirmPara(Bool bflag, const char* message)
{
  if (!bflag)
    return false;
  
  printf("Error: %s\n",message);
  return true;
}

/* helper function */
/* for handling "-1/-0 FOO" */
void translateOldStyleCmdline(const char* value, po::Options& opts, const std::string& arg)
{
  const char* argv[] = {arg.c_str(), value};
  /* replace some short names with their long name varients */
  if (arg == "LDC")
  {
    argv[0] = "LowDelayCoding";
  }
  else if (arg == "RDQ")
  {
    argv[0] = "RDOQ";
  }
  else if (arg == "HAD")
  {
    argv[0] = "HadamardME";
  }
  else if (arg == "SRD")
  {
    argv[0] = "SBACRD";
  }
  else if (arg == "IBD")
  {
    argv[0] = "BitIncrement";
  }
  /* issue a warning for change in FEN behaviour */
  if (arg == "FEN")
  {
    /* xxx todo */
  }
  po::storePair(opts, argv[0], argv[1]);
}

void doOldStyleCmdlineOn(po::Options& opts, const std::string& arg)
{
  if (arg == "IBD")
  {
    translateOldStyleCmdline("4", opts, arg);
    return;
  }
  translateOldStyleCmdline("1", opts, arg);
}

void doOldStyleCmdlineOff(po::Options& opts, const std::string& arg)
{
  translateOldStyleCmdline("0", opts, arg);
}

Void TAppEncCfg::xAppendToFileNameEnd( Char* pchInputFileName, const Char* pchStringToAppend, Char*& rpchOutputFileName)
{
  size_t iInLength     = strlen(pchInputFileName);
  size_t iAppendLength = strlen(pchStringToAppend);

  rpchOutputFileName = (Char*) malloc(iInLength+iAppendLength+1);
  Char* pCDot = strrchr(pchInputFileName,'.');
  pCDot = pCDot ? pCDot : pchInputFileName + iInLength;
  size_t iCharsToDot = pCDot - pchInputFileName ;
  size_t iCharsToEnd = iInLength - iCharsToDot;
  strncpy(rpchOutputFileName                            ,  pchInputFileName            , iCharsToDot  );
  strncpy(rpchOutputFileName+ iCharsToDot               ,  pchStringToAppend           , iAppendLength);
  strncpy(rpchOutputFileName+ iCharsToDot+iAppendLength ,  pchInputFileName+iCharsToDot, iCharsToEnd  );
  rpchOutputFileName[iInLength+iAppendLength] = '\0';
}

//! \}
