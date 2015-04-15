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

/** \file     TAppEncCfg.cpp
    \brief    Handle encoder configuration parameters
*/

#include <stdlib.h>
#include <cassert>
#include <cstring>
#include <string>
#include "TLibCommon/TComRom.h"
#include "TAppEncCfg.h"

static istream& operator>>(istream &, Level::Name &);
static istream& operator>>(istream &, Level::Tier &);
static istream& operator>>(istream &, Profile::Name &);

#include "TAppCommon/program_options_lite.h"
#include "TLibEncoder/TEncRateCtrl.h"
#ifdef WIN32
#define strdup _strdup
#endif

using namespace std;
namespace po = df::program_options_lite;

//! \ingroup TAppEncoder
//! \{

// ====================================================================================================================
// Constructor / destructor / initialization / destroy
// ====================================================================================================================

TAppEncCfg::TAppEncCfg()
#if H_MV
: m_pchBitstreamFile()
#else
: m_pchInputFile()
, m_pchBitstreamFile()
, m_pchReconFile()
#endif
, m_pchdQPFile()
, m_pColumnWidth()
, m_pRowHeight()
, m_scalingListFile()
{
#if !H_MV
  m_aidQP = NULL;
#endif
  m_startOfCodedInterval = NULL;
  m_codedPivotValue = NULL;
  m_targetPivotValue = NULL;

#if KWU_RC_MADPRED_E0227
  m_depthMADPred = 0;
#endif
}

TAppEncCfg::~TAppEncCfg()
{
#if H_MV
  for( Int layer = 0; layer < m_aidQP.size(); layer++ )
  {
    if ( m_aidQP[layer] != NULL )
    {
      delete[] m_aidQP[layer];
      m_aidQP[layer] = NULL;
    }
  }
  for(Int i = 0; i< m_pchInputFileList.size(); i++ )
  {
    if ( m_pchInputFileList[i] != NULL )
      free (m_pchInputFileList[i]);
  }
#else
  if ( m_aidQP )
  {
    delete[] m_aidQP;
  }
#endif
  if ( m_startOfCodedInterval )
  {
    delete[] m_startOfCodedInterval;
    m_startOfCodedInterval = NULL;
  }
   if ( m_codedPivotValue )
  {
    delete[] m_codedPivotValue;
    m_codedPivotValue = NULL;
  }
  if ( m_targetPivotValue )
  {
    delete[] m_targetPivotValue;
    m_targetPivotValue = NULL;
  }
#if !H_MV
  free(m_pchInputFile);
#endif
  free(m_pchBitstreamFile);
#if H_MV
  for(Int i = 0; i< m_pchReconFileList.size(); i++ )
  {
    if ( m_pchReconFileList[i] != NULL )
      free (m_pchReconFileList[i]);
  }
#else
  free(m_pchReconFile);
#endif
  free(m_pchdQPFile);
  free(m_pColumnWidth);
  free(m_pRowHeight);
  free(m_scalingListFile);
#if H_MV
  for( Int i = 0; i < m_GOPListMvc.size(); i++ )
  {
    if( m_GOPListMvc[i] )
    {
      delete[] m_GOPListMvc[i];
      m_GOPListMvc[i] = NULL;
    }
  }
#endif
#if H_3D
#if H_3D_VSO
  if (  m_pchVSOConfig != NULL)
    free (  m_pchVSOConfig );
#endif
  if ( m_pchCameraParameterFile != NULL )
    free ( m_pchCameraParameterFile ); 

  if ( m_pchBaseViewCameraNumbers != NULL )
    free ( m_pchBaseViewCameraNumbers ); 
#endif
}

Void TAppEncCfg::create()
{
}

Void TAppEncCfg::destroy()
{
}

std::istringstream &operator>>(std::istringstream &in, GOPEntry &entry)     //input
{
  in>>entry.m_sliceType;
  in>>entry.m_POC;
  in>>entry.m_QPOffset;
  in>>entry.m_QPFactor;
  in>>entry.m_tcOffsetDiv2;
  in>>entry.m_betaOffsetDiv2;
  in>>entry.m_temporalId;
  in>>entry.m_numRefPicsActive;
  in>>entry.m_numRefPics;
  for ( Int i = 0; i < entry.m_numRefPics; i++ )
  {
    in>>entry.m_referencePics[i];
  }
  in>>entry.m_interRPSPrediction;
#if AUTO_INTER_RPS
  if (entry.m_interRPSPrediction==1)
  {
    in>>entry.m_deltaRPS;
    in>>entry.m_numRefIdc;
    for ( Int i = 0; i < entry.m_numRefIdc; i++ )
    {
      in>>entry.m_refIdc[i];
    }
  }
  else if (entry.m_interRPSPrediction==2)
  {
    in>>entry.m_deltaRPS;
  }
#else
  if (entry.m_interRPSPrediction)
  {
    in>>entry.m_deltaRPS;
    in>>entry.m_numRefIdc;
    for ( Int i = 0; i < entry.m_numRefIdc; i++ )
    {
      in>>entry.m_refIdc[i];
    }
  }
#endif
#if H_MV
  in>>entry.m_numActiveRefLayerPics;
  for( Int i = 0; i < entry.m_numActiveRefLayerPics; i++ )
  {
    in>>entry.m_interLayerPredLayerIdc[i];
  }
  for( Int i = 0; i < entry.m_numActiveRefLayerPics; i++ )
  {
    in>>entry.m_interViewRefPosL[0][i];
  }
  for( Int i = 0; i < entry.m_numActiveRefLayerPics; i++ )
  {
    in>>entry.m_interViewRefPosL[1][i];
  }
#endif
  return in;
}

static const struct MapStrToProfile {
  const Char* str;
  Profile::Name value;
} strToProfile[] = {
  {"none", Profile::NONE},
  {"main", Profile::MAIN},
  {"main10", Profile::MAIN10},
  {"main-still-picture", Profile::MAINSTILLPICTURE},
#if H_MV
  {"main-stereo",    Profile::MAINSTEREO},
  {"main-multiview", Profile::MAINMULTIVIEW},
#if H_3D
  {"main-3d"    , Profile::MAIN3D},
#endif
#endif
};

static const struct MapStrToTier {
  const Char* str;
  Level::Tier value;
} strToTier[] = {
  {"main", Level::MAIN},
  {"high", Level::HIGH},
};

static const struct MapStrToLevel {
  const Char* str;
  Level::Name value;
} strToLevel[] = {
  {"none",Level::NONE},
  {"1",   Level::LEVEL1},
  {"2",   Level::LEVEL2},
  {"2.1", Level::LEVEL2_1},
  {"3",   Level::LEVEL3},
  {"3.1", Level::LEVEL3_1},
  {"4",   Level::LEVEL4},
  {"4.1", Level::LEVEL4_1},
  {"5",   Level::LEVEL5},
  {"5.1", Level::LEVEL5_1},
  {"5.2", Level::LEVEL5_2},
  {"6",   Level::LEVEL6},
  {"6.1", Level::LEVEL6_1},
  {"6.2", Level::LEVEL6_2},
};

template<typename T, typename P>
static istream& readStrToEnum(P map[], unsigned long mapLen, istream &in, T &val)
{
  string str;
  in >> str;

  for (Int i = 0; i < mapLen; i++)
  {
    if (str == map[i].str)
    {
      val = map[i].value;
      goto found;
    }
  }
  /* not found */
  in.setstate(ios::failbit);
found:
  return in;
}

static istream& operator>>(istream &in, Profile::Name &profile)
{
  return readStrToEnum(strToProfile, sizeof(strToProfile)/sizeof(*strToProfile), in, profile);
}

static istream& operator>>(istream &in, Level::Tier &tier)
{
  return readStrToEnum(strToTier, sizeof(strToTier)/sizeof(*strToTier), in, tier);
}

static istream& operator>>(istream &in, Level::Name &level)
{
  return readStrToEnum(strToLevel, sizeof(strToLevel)/sizeof(*strToLevel), in, level);
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
  Bool do_help = false;
  
#if !H_MV
  string cfg_InputFile;
#endif
  string cfg_BitstreamFile;
#if !H_MV
  string cfg_ReconFile;
#endif
#if H_MV
  vector<Int>   cfg_dimensionLength; 
#if H_3D 
  cfg_dimensionLength.push_back( 2  );  // depth
  cfg_dimensionLength.push_back( 32 );  // texture 
#else
  cfg_dimensionLength.push_back( 64 ); 
#endif 
#endif
  string cfg_dQPFile;
  string cfg_ColumnWidth;
  string cfg_RowHeight;
  string cfg_ScalingListFile;
  string cfg_startOfCodedInterval;
  string cfg_codedPivotValue;
  string cfg_targetPivotValue;
  po::Options opts;
  opts.addOptions()
  ("help", do_help, false, "this help text")
  ("c", po::parseConfigFile, "configuration file name")
  
  // File, I/O and source parameters
#if H_MV
  ("InputFile_%d,i_%d",       m_pchInputFileList,       (char *) 0 , MAX_NUM_LAYER_IDS , "original Yuv input file name %d")
#else
  ("InputFile,i",           cfg_InputFile,     string(""), "Original YUV input file name")
#endif
  ("BitstreamFile,b",       cfg_BitstreamFile, string(""), "Bitstream output file name")
#if H_MV
  ("ReconFile_%d,o_%d",       m_pchReconFileList,       (char *) 0 , MAX_NUM_LAYER_IDS , "reconstructed Yuv output file name %d")
#else
  ("ReconFile,o",           cfg_ReconFile,     string(""), "Reconstructed YUV output file name")
#endif
#if H_MV
  ("NumberOfLayers",        m_numberOfLayers     , 1,                     "Number of layers")
#if !H_3D
  ("ScalabilityMask",       m_scalabilityMask    , 2                    , "Scalability Mask")    
#else
  ("ScalabilityMask",       m_scalabilityMask    , 3                    , "Scalability Mask, 1: Texture 3: Texture + Depth ")    
#endif  
  ("DimensionIdLen",        m_dimensionIdLen     , cfg_dimensionLength  , "Number of bits used to store dimensions Id")
  ("ViewOrderIndex",        m_viewOrderIndex     , std::vector<Int>(1,0), "View Order Index per layer")
  ("ViewId",                m_viewId             , std::vector<Int>(1,0), "View Id per View Order Index")
#if H_3D
  ("DepthFlag",             m_depthFlag          , std::vector<Int>(1,0), "Depth Flag")
#if H_3D_DIM
  ("DMM",                   m_useDMM,           true,  "Depth intra model modes")
  ("SDC",                   m_useSDC,           true,  "Simplified depth coding")
  ("DLT",                   m_useDLT,           true,  "Depth lookup table")
#endif
#endif
  ("LayerIdInNuh",          m_layerIdInNuh       , std::vector<Int>(1,0), "LayerId in Nuh")
  ("SplittingFlag",         m_splittingFlag      , false                , "Splitting Flag")    

  // Layer Sets + Output Layer Sets + Profile Tier Level
  ("VpsNumLayerSets",       m_vpsNumLayerSets    , 1                    , "Number of layer sets")    
  ("LayerIdsInSet_%d",      m_layerIdsInSets     , std::vector<Int>(1,0), MAX_VPS_OP_SETS_PLUS1 ,"LayerIds of Layer set")  
  ("DefaultTargetOutputLayerIdc"     , m_defaultTargetOutputLayerIdc     , 0, "Specifies output layers of layer sets, 0: output all layers, 1: output highest layer, 2: specified by LayerIdsInDefOutputLayerSet")
  ("OutputLayerSetIdx",     m_outputLayerSetIdx  , std::vector<Int>(0,0), "Indices of layer sets used as additional output layer sets")  

  ("LayerIdsInAddOutputLayerSet_%d", m_layerIdsInAddOutputLayerSet      , std::vector<Int>(0,0), MAX_VPS_ADD_OUTPUT_LAYER_SETS, "Indices in VPS of output layers in additional output layer set")  
  ("LayerIdsInDefOutputLayerSet_%d", m_layerIdsInDefOutputLayerSet      , std::vector<Int>(0,0), MAX_VPS_OP_SETS_PLUS1, "Indices in VPS of output layers in layer set")  
  ("ProfileLevelTierIdx",   m_profileLevelTierIdx, std::vector<Int>(1,0), "Indices to profile level tier")
  
  // Layer dependencies
  ("DirectRefLayers_%d",    m_directRefLayers    , std::vector<Int>(0,0), MAX_NUM_LAYERS, "LayerIds of direct reference layers")
  ("DependencyTypes_%d",    m_dependencyTypes    , std::vector<Int>(0,0), MAX_NUM_LAYERS, "Dependency types of direct reference layers, 0: Sample 1: Motion 2: Sample+Motion")
#endif
  ("SourceWidth,-wdt",      m_iSourceWidth,        0, "Source picture width")
  ("SourceHeight,-hgt",     m_iSourceHeight,       0, "Source picture height")
  ("InputBitDepth",         m_inputBitDepthY,    8, "Bit-depth of input file")
  ("OutputBitDepth",        m_outputBitDepthY,   0, "Bit-depth of output file (default:InternalBitDepth)")
  ("InternalBitDepth",      m_internalBitDepthY, 0, "Bit-depth the codec operates at. (default:InputBitDepth)"
                                                       "If different to InputBitDepth, source data will be converted")
  ("InputBitDepthC",        m_inputBitDepthC,    0, "As per InputBitDepth but for chroma component. (default:InputBitDepth)")
  ("OutputBitDepthC",       m_outputBitDepthC,   0, "As per OutputBitDepth but for chroma component. (default:InternalBitDepthC)")
  ("InternalBitDepthC",     m_internalBitDepthC, 0, "As per InternalBitDepth but for chroma component. (default:IntrenalBitDepth)")
  ("ConformanceMode",       m_conformanceMode,     0, "Window conformance mode (0: no window, 1:automatic padding, 2:padding, 3:conformance")
  ("HorizontalPadding,-pdx",m_aiPad[0],            0, "Horizontal source padding for conformance window mode 2")
  ("VerticalPadding,-pdy",  m_aiPad[1],            0, "Vertical source padding for conformance window mode 2")
  ("ConfLeft",              m_confLeft,            0, "Left offset for window conformance mode 3")
  ("ConfRight",             m_confRight,           0, "Right offset for window conformance mode 3")
  ("ConfTop",               m_confTop,             0, "Top offset for window conformance mode 3")
  ("ConfBottom",            m_confBottom,          0, "Bottom offset for window conformance mode 3")
  ("FrameRate,-fr",         m_iFrameRate,          0, "Frame rate")
  ("FrameSkip,-fs",         m_FrameSkip,          0u, "Number of frames to skip at start of input YUV")
  ("FramesToBeEncoded,f",   m_framesToBeEncoded,   0, "Number of frames to be encoded (default=all)")

  //Field coding parameters
  ("FieldCoding", m_isField, false, "Signals if it's a field based coding")
  ("TopFieldFirst, Tff", m_isTopFieldFirst, false, "In case of field based coding, signals whether if it's a top field first or not")
  
  // Profile and level
  ("Profile", m_profile,   Profile::NONE, "Profile to be used when encoding (Incomplete)")
  ("Level",   m_level,     Level::NONE,   "Level limit to be used, eg 5.1 (Incomplete)")
  ("Tier",    m_levelTier, Level::MAIN,   "Tier to use for interpretation of --Level")

  ("ProgressiveSource", m_progressiveSourceFlag, false, "Indicate that source is progressive")
  ("InterlacedSource",  m_interlacedSourceFlag,  false, "Indicate that source is interlaced")
  ("NonPackedSource",   m_nonPackedConstraintFlag, false, "Indicate that source does not contain frame packing")
  ("FrameOnly",         m_frameOnlyConstraintFlag, false, "Indicate that the bitstream contains only frames")
  
  // Unit definition parameters
  ("MaxCUWidth",              m_uiMaxCUWidth,             64u)
  ("MaxCUHeight",             m_uiMaxCUHeight,            64u)
  // todo: remove defaults from MaxCUSize
  ("MaxCUSize,s",             m_uiMaxCUWidth,             64u, "Maximum CU size")
  ("MaxCUSize,s",             m_uiMaxCUHeight,            64u, "Maximum CU size")
  ("MaxPartitionDepth,h",     m_uiMaxCUDepth,              4u, "CU depth")
  
  ("QuadtreeTULog2MaxSize",   m_uiQuadtreeTULog2MaxSize,   6u, "Maximum TU size in logarithm base 2")
  ("QuadtreeTULog2MinSize",   m_uiQuadtreeTULog2MinSize,   2u, "Minimum TU size in logarithm base 2")
  
  ("QuadtreeTUMaxDepthIntra", m_uiQuadtreeTUMaxDepthIntra, 1u, "Depth of TU tree for intra CUs")
  ("QuadtreeTUMaxDepthInter", m_uiQuadtreeTUMaxDepthInter, 2u, "Depth of TU tree for inter CUs")
#if H_MV  
  // Coding structure parameters
  ("IntraPeriod,-ip",         m_iIntraPeriod,std::vector<Int>(1,-1), "Intra period in frames, (-1: only first frame), per layer")
#else
  // Coding structure paramters
("IntraPeriod,-ip",         m_iIntraPeriod,              -1, "Intra period in frames, (-1: only first frame)")
#endif
  ("DecodingRefreshType,-dr", m_iDecodingRefreshType,       0, "Intra refresh type (0:none 1:CRA 2:IDR)")
  ("GOPSize,g",               m_iGOPSize,                   1, "GOP size of temporal structure")
  // motion options
  ("FastSearch",              m_iFastSearch,                1, "0:Full search  1:Diamond  2:PMVFAST")
  ("SearchRange,-sr",         m_iSearchRange,              96, "Motion search range")
  ("BipredSearchRange",       m_bipredSearchRange,          4, "Motion search range for bipred refinement")
  ("HadamardME",              m_bUseHADME,               true, "Hadamard ME for fractional-pel")
  ("ASR",                     m_bUseASR,                false, "Adaptive motion search range")

  // Mode decision parameters
  ("LambdaModifier0,-LM0", m_adLambdaModifier[ 0 ], ( Double )1.0, "Lambda modifier for temporal layer 0")
  ("LambdaModifier1,-LM1", m_adLambdaModifier[ 1 ], ( Double )1.0, "Lambda modifier for temporal layer 1")
  ("LambdaModifier2,-LM2", m_adLambdaModifier[ 2 ], ( Double )1.0, "Lambda modifier for temporal layer 2")
  ("LambdaModifier3,-LM3", m_adLambdaModifier[ 3 ], ( Double )1.0, "Lambda modifier for temporal layer 3")
  ("LambdaModifier4,-LM4", m_adLambdaModifier[ 4 ], ( Double )1.0, "Lambda modifier for temporal layer 4")
  ("LambdaModifier5,-LM5", m_adLambdaModifier[ 5 ], ( Double )1.0, "Lambda modifier for temporal layer 5")
  ("LambdaModifier6,-LM6", m_adLambdaModifier[ 6 ], ( Double )1.0, "Lambda modifier for temporal layer 6")
  ("LambdaModifier7,-LM7", m_adLambdaModifier[ 7 ], ( Double )1.0, "Lambda modifier for temporal layer 7")

  /* Quantization parameters */
#if H_MV
  ("QP,q",          m_fQP, std::vector<double>(1,30.0), "Qp values for each layer, if value is float, QP is switched once during encoding")
#else
  ("QP,q",          m_fQP,             30.0, "Qp value, if value is float, QP is switched once during encoding")
#endif
  ("DeltaQpRD,-dqr",m_uiDeltaQpRD,       0u, "max dQp offset for slice")
  ("MaxDeltaQP,d",  m_iMaxDeltaQP,        0, "max dQp offset for block")
  ("MaxCuDQPDepth,-dqd",  m_iMaxCuDQPDepth,        0, "max depth for a minimum CuDQP")

  ("CbQpOffset,-cbqpofs",  m_cbQpOffset,        0, "Chroma Cb QP Offset")
  ("CrQpOffset,-crqpofs",  m_crQpOffset,        0, "Chroma Cr QP Offset")

#if ADAPTIVE_QP_SELECTION
  ("AdaptiveQpSelection,-aqps",   m_bUseAdaptQpSelect,           false, "AdaptiveQpSelection")
#endif

  ("AdaptiveQP,-aq",                m_bUseAdaptiveQP,           false, "QP adaptation based on a psycho-visual model")
  ("MaxQPAdaptationRange,-aqr",     m_iQPAdaptationRange,           6, "QP adaptation range")
  ("dQPFile,m",                     cfg_dQPFile,           string(""), "dQP file name")
  ("RDOQ",                          m_useRDOQ,                  true )
  ("RDOQTS",                        m_useRDOQTS,                true )
  ("RDpenalty",                     m_rdPenalty,                0,  "RD-penalty for 32x32 TU for intra in non-intra slices. 0:disbaled  1:RD-penalty  2:maximum RD-penalty")
  
  // Deblocking filter parameters
#if H_MV
  ("LoopFilterDisable",              m_bLoopFilterDisable,             std::vector<Bool>(1,false), "Disable Loop Filter per Layer" )
#else
  ("LoopFilterDisable",              m_bLoopFilterDisable,             false )
#endif
  ("LoopFilterOffsetInPPS",          m_loopFilterOffsetInPPS,          false )
  ("LoopFilterBetaOffset_div2",      m_loopFilterBetaOffsetDiv2,           0 )
  ("LoopFilterTcOffset_div2",        m_loopFilterTcOffsetDiv2,             0 )
  ("DeblockingFilterControlPresent", m_DeblockingFilterControlPresent, false )
  ("DeblockingFilterMetric",         m_DeblockingFilterMetric,         false )

#if H_3D_ARP
  ("AdvMultiviewResPred",      m_uiUseAdvResPred,           (UInt)1, "Usage of Advanced Residual Prediction" )
#endif
#if H_3D_SPIVMP
  ("SubPULog2Size", m_iSubPULog2Size, (Int)3, "Sub-PU size index: 2^n")
  ("SubPUMPILog2Size", m_iSubPUMPILog2Size, (Int)3, "Sub-PU MPI size index: 2^n")
#endif
#if H_3D_IC
  ("IlluCompEnable",           m_abUseIC, true, "Enable illumination compensation")
#endif
#if H_3D_INTER_SDC
  ("InterSDC",                 m_bDepthInterSDCFlag,        true, "Enable depth inter SDC")
#endif
#if H_3D_DBBP
  ("DBBP",                     m_bUseDBBP,   true, "Enable depth-based block partitioning" )
#endif
#if H_3D_IV_MERGE
  ("MPI",                      m_bMPIFlag,        true, "Enable MPI")
#endif
  // Coding tools
  ("AMP",                      m_enableAMP,                 true,  "Enable asymmetric motion partitions")
  ("TransformSkip",            m_useTransformSkip,          false, "Intra transform skipping")
  ("TransformSkipFast",        m_useTransformSkipFast,      false, "Fast intra transform skipping")
#if H_MV
  ("SAO",                      m_bUseSAO, std::vector<Bool>(1,true), "Enable Sample Adaptive Offset per Layer")
#else
  ("SAO",                      m_bUseSAO,                   true,  "Enable Sample Adaptive Offset")
#endif
  ("MaxNumOffsetsPerPic",      m_maxNumOffsetsPerPic,       2048,  "Max number of SAO offset per picture (Default: 2048)")   
  ("SAOLcuBoundary",           m_saoLcuBoundary,            false, "0: right/bottom LCU boundary areas skipped from SAO parameter estimation, 1: non-deblocked pixels are used for those areas")
  ("SliceMode",                m_sliceMode,                0,     "0: Disable all Recon slice limits, 1: Enforce max # of LCUs, 2: Enforce max # of bytes, 3:specify tiles per dependent slice")
  ("SliceArgument",            m_sliceArgument,            0,     "Depending on SliceMode being:"
                                                                   "\t1: max number of CTUs per slice"
                                                                   "\t2: max number of bytes per slice"
                                                                   "\t3: max number of tiles per slice")
  ("SliceSegmentMode",         m_sliceSegmentMode,       0,     "0: Disable all slice segment limits, 1: Enforce max # of LCUs, 2: Enforce max # of bytes, 3:specify tiles per dependent slice")
  ("SliceSegmentArgument",     m_sliceSegmentArgument,   0,     "Depending on SliceSegmentMode being:"
                                                                   "\t1: max number of CTUs per slice segment"
                                                                   "\t2: max number of bytes per slice segment"
                                                                   "\t3: max number of tiles per slice segment")
  ("LFCrossSliceBoundaryFlag", m_bLFCrossSliceBoundaryFlag, true)

  ("ConstrainedIntraPred",     m_bUseConstrainedIntraPred,  false, "Constrained Intra Prediction")

  ("PCMEnabledFlag",           m_usePCM,                    false)
  ("PCMLog2MaxSize",           m_pcmLog2MaxSize,            5u)
  ("PCMLog2MinSize",           m_uiPCMLog2MinSize,          3u)
  ("PCMInputBitDepthFlag",     m_bPCMInputBitDepthFlag,     true)
  ("PCMFilterDisableFlag",     m_bPCMFilterDisableFlag,    false)

  ("WeightedPredP,-wpP",          m_useWeightedPred,               false,      "Use weighted prediction in P slices")
  ("WeightedPredB,-wpB",          m_useWeightedBiPred,             false,      "Use weighted (bidirectional) prediction in B slices")
  ("Log2ParallelMergeLevel",      m_log2ParallelMergeLevel,     2u,          "Parallel merge estimation region")
  ("UniformSpacingIdc",           m_iUniformSpacingIdr,            0,          "Indicates if the column and row boundaries are distributed uniformly")
  ("NumTileColumnsMinus1",        m_iNumColumnsMinus1,             0,          "Number of columns in a picture minus 1")
  ("ColumnWidthArray",            cfg_ColumnWidth,                 string(""), "Array containing ColumnWidth values in units of LCU")
  ("NumTileRowsMinus1",           m_iNumRowsMinus1,                0,          "Number of rows in a picture minus 1")
  ("RowHeightArray",              cfg_RowHeight,                   string(""), "Array containing RowHeight values in units of LCU")
  ("LFCrossTileBoundaryFlag",      m_bLFCrossTileBoundaryFlag,             true,          "1: cross-tile-boundary loop filtering. 0:non-cross-tile-boundary loop filtering")
  ("WaveFrontSynchro",            m_iWaveFrontSynchro,             0,          "0: no synchro; 1 synchro with TR; 2 TRR etc")
  ("ScalingList",                 m_useScalingListId,              0,          "0: no scaling list, 1: default scaling lists, 2: scaling lists specified in ScalingListFile")
  ("ScalingListFile",             cfg_ScalingListFile,             string(""), "Scaling list file name")
  ("SignHideFlag,-SBH",                m_signHideFlag, 1)
  ("MaxNumMergeCand",             m_maxNumMergeCand,             5u,         "Maximum number of merge candidates")

  /* Misc. */
  ("SEIDecodedPictureHash",       m_decodedPictureHashSEIEnabled, 0, "Control generation of decode picture hash SEI messages\n"
                                                                    "\t3: checksum\n"
                                                                    "\t2: CRC\n"
                                                                    "\t1: use MD5\n"
                                                                    "\t0: disable")
  ("SEIpictureDigest",            m_decodedPictureHashSEIEnabled, 0, "deprecated alias for SEIDecodedPictureHash")
  ("TMVPMode", m_TMVPModeId, 1, "TMVP mode 0: TMVP disable for all slices. 1: TMVP enable for all slices (default) 2: TMVP enable for certain slices only")
  ("FEN", m_bUseFastEnc, false, "fast encoder setting")
  ("ECU", m_bUseEarlyCU, false, "Early CU setting") 
  ("FDM", m_useFastDecisionForMerge, true, "Fast decision for Merge RD Cost") 
  ("CFM", m_bUseCbfFastMode, false, "Cbf fast mode setting")
  ("ESD", m_useEarlySkipDetection, false, "Early SKIP detection setting")
  ( "RateControl",         m_RCEnableRateControl,   false, "Rate control: enable rate control" )
  ( "TargetBitrate",       m_RCTargetBitrate,           0, "Rate control: target bitrate" )
  ( "KeepHierarchicalBit", m_RCKeepHierarchicalBit,     0, "Rate control: 0: equal bit allocation; 1: fixed ratio bit allocation; 2: adaptive ratio bit allocation" )
  ( "LCULevelRateControl", m_RCLCULevelRC,           true, "Rate control: true: LCU level RC; false: picture level RC" )
  ( "RCLCUSeparateModel",  m_RCUseLCUSeparateModel,  true, "Rate control: use LCU level separate R-lambda model" )
  ( "InitialQP",           m_RCInitialQP,               0, "Rate control: initial QP" )
  ( "RCForceIntraQP",      m_RCForceIntraQP,        false, "Rate control: force intra QP to be equal to initial QP" )

#if KWU_RC_VIEWRC_E0227
  ("ViewWiseTargetBits, -vtbr" ,  m_viewTargetBits,  std::vector<Int>(1, 32), "View-wise target bit-rate setting")
  ("TargetBitAssign, -ta", m_viewWiseRateCtrl, false, "View-wise rate control on/off")
#endif
#if KWU_RC_MADPRED_E0227
  ("DepthMADPred, -dm", m_depthMADPred, (UInt)0, "Depth based MAD prediction on/off")
#endif
#if H_MV

  // VPS VUI
  ("VpsVuiPresentFlag"           , m_vpsVuiPresentFlag           , false                                           , "VpsVuiPresentFlag           ")
  ("CrossLayerPicTypeAlignedFlag", m_crossLayerPicTypeAlignedFlag, false                                           , "CrossLayerPicTypeAlignedFlag")  // Could actually be derived by the encoder
  ("CrossLayerIrapAlignedFlag"   , m_crossLayerIrapAlignedFlag   , false                                           , "CrossLayerIrapAlignedFlag   ")  // Could actually be derived by the encoder
  ("AllLayersIdrAlignedFlag"     , m_allLayersIdrAlignedFlag     , false                                           , "CrossLayerIrapAlignedFlag   ")  // Could actually be derived by the encoder
  ("BitRatePresentVpsFlag"       , m_bitRatePresentVpsFlag       , false                                           , "BitRatePresentVpsFlag       ")
  ("PicRatePresentVpsFlag"       , m_picRatePresentVpsFlag       , false                                           , "PicRatePresentVpsFlag       ")
  ("BitRatePresentFlag"          , m_bitRatePresentFlag          , std::vector< Bool >(1,0)  ,MAX_VPS_OP_SETS_PLUS1, "BitRatePresentFlag per sub layer for the N-th layer set")
  ("PicRatePresentFlag"          , m_picRatePresentFlag          , std::vector< Bool >(1,0)  ,MAX_VPS_OP_SETS_PLUS1, "PicRatePresentFlag per sub layer for the N-th layer set")
  ("AvgBitRate"                  , m_avgBitRate                  , std::vector< Int  >(1,0)  ,MAX_VPS_OP_SETS_PLUS1, "AvgBitRate         per sub layer for the N-th layer set")
  ("MaxBitRate"                  , m_maxBitRate                  , std::vector< Int  >(1,0)  ,MAX_VPS_OP_SETS_PLUS1, "MaxBitRate         per sub layer for the N-th layer set")
  ("ConstantPicRateIdc"          , m_constantPicRateIdc          , std::vector< Int  >(1,0)  ,MAX_VPS_OP_SETS_PLUS1, "ConstantPicRateIdc per sub layer for the N-th layer set")
  ("AvgPicRate"                  , m_avgPicRate                  , std::vector< Int  >(1,0)  ,MAX_VPS_OP_SETS_PLUS1, "AvgPicRate         per sub layer for the N-th layer set")
  ("TilesNotInUseFlag"            , m_tilesNotInUseFlag            , true                                          , "TilesNotInUseFlag            ")
  ("TilesInUseFlag"               , m_tilesInUseFlag               , std::vector< Bool >(1,false)                   , "TilesInUseFlag               ")
  ("LoopFilterNotAcrossTilesFlag" , m_loopFilterNotAcrossTilesFlag , std::vector< Bool >(1,false)                  , "LoopFilterNotAcrossTilesFlag ")
  ("WppNotInUseFlag"              , m_wppNotInUseFlag              , true                                          , "WppNotInUseFlag              ")
  ("WppInUseFlag"                 , m_wppInUseFlag                 , std::vector< Bool >(1,0)                      , "WppInUseFlag                 ")
  ("TileBoundariesAlignedFlag"   , m_tileBoundariesAlignedFlag   , std::vector< Bool >(1,0)  ,MAX_NUM_LAYERS       , "TileBoundariesAlignedFlag    per direct reference for the N-th layer")
  ("IlpRestrictedRefLayersFlag"  , m_ilpRestrictedRefLayersFlag  , false                                           , "IlpRestrictedRefLayersFlag")
  ("MinSpatialSegmentOffsetPlus1", m_minSpatialSegmentOffsetPlus1, std::vector< Int  >(1,0)  ,MAX_NUM_LAYERS       , "MinSpatialSegmentOffsetPlus1 per direct reference for the N-th layer")
  ("CtuBasedOffsetEnabledFlag"   , m_ctuBasedOffsetEnabledFlag   , std::vector< Bool >(1,0)  ,MAX_NUM_LAYERS       , "CtuBasedOffsetEnabledFlag    per direct reference for the N-th layer")
  ("MinHorizontalCtuOffsetPlus1" , m_minHorizontalCtuOffsetPlus1 , std::vector< Int  >(1,0)  ,MAX_NUM_LAYERS       , "MinHorizontalCtuOffsetPlus1  per direct reference for the N-th layer")
#endif

  ("TransquantBypassEnableFlag", m_TransquantBypassEnableFlag, false, "transquant_bypass_enable_flag indicator in PPS")
  ("CUTransquantBypassFlagForce", m_CUTransquantBypassFlagForce, false, "Force transquant bypass mode, when transquant_bypass_enable_flag is enabled")
  ("RecalculateQPAccordingToLambda", m_recalculateQPAccordingToLambda, false, "Recalculate QP values according to lambda values. Do not suggest to be enabled in all intra case")
  ("StrongIntraSmoothing,-sis",      m_useStrongIntraSmoothing,           true, "Enable strong intra smoothing for 32x32 blocks")
  ("SEIActiveParameterSets",         m_activeParameterSetsSEIEnabled,          0, "Enable generation of active parameter sets SEI messages")
  ("VuiParametersPresent,-vui",      m_vuiParametersPresentFlag,           false, "Enable generation of vui_parameters()")
  ("AspectRatioInfoPresent",         m_aspectRatioInfoPresentFlag,         false, "Signals whether aspect_ratio_idc is present")
  ("AspectRatioIdc",                 m_aspectRatioIdc,                         0, "aspect_ratio_idc")
  ("SarWidth",                       m_sarWidth,                               0, "horizontal size of the sample aspect ratio")
  ("SarHeight",                      m_sarHeight,                              0, "vertical size of the sample aspect ratio")
  ("OverscanInfoPresent",            m_overscanInfoPresentFlag,            false, "Indicates whether conformant decoded pictures are suitable for display using overscan\n")
  ("OverscanAppropriate",            m_overscanAppropriateFlag,            false, "Indicates whether conformant decoded pictures are suitable for display using overscan\n")
  ("VideoSignalTypePresent",         m_videoSignalTypePresentFlag,         false, "Signals whether video_format, video_full_range_flag, and colour_description_present_flag are present")
  ("VideoFormat",                    m_videoFormat,                            5, "Indicates representation of pictures")
  ("VideoFullRange",                 m_videoFullRangeFlag,                 false, "Indicates the black level and range of luma and chroma signals")
  ("ColourDescriptionPresent",       m_colourDescriptionPresentFlag,       false, "Signals whether colour_primaries, transfer_characteristics and matrix_coefficients are present")
  ("ColourPrimaries",                m_colourPrimaries,                        2, "Indicates chromaticity coordinates of the source primaries")
  ("TransferCharateristics",         m_transferCharacteristics,                2, "Indicates the opto-electronic transfer characteristics of the source")
  ("MatrixCoefficients",             m_matrixCoefficients,                     2, "Describes the matrix coefficients used in deriving luma and chroma from RGB primaries")
  ("ChromaLocInfoPresent",           m_chromaLocInfoPresentFlag,           false, "Signals whether chroma_sample_loc_type_top_field and chroma_sample_loc_type_bottom_field are present")
  ("ChromaSampleLocTypeTopField",    m_chromaSampleLocTypeTopField,            0, "Specifies the location of chroma samples for top field")
  ("ChromaSampleLocTypeBottomField", m_chromaSampleLocTypeBottomField,         0, "Specifies the location of chroma samples for bottom field")
  ("NeutralChromaIndication",        m_neutralChromaIndicationFlag,        false, "Indicates that the value of all decoded chroma samples is equal to 1<<(BitDepthCr-1)")
  ("DefaultDisplayWindowFlag",       m_defaultDisplayWindowFlag,           false, "Indicates the presence of the Default Window parameters")
  ("DefDispWinLeftOffset",           m_defDispWinLeftOffset,                   0, "Specifies the left offset of the default display window from the conformance window")
  ("DefDispWinRightOffset",          m_defDispWinRightOffset,                  0, "Specifies the right offset of the default display window from the conformance window")
  ("DefDispWinTopOffset",            m_defDispWinTopOffset,                    0, "Specifies the top offset of the default display window from the conformance window")
  ("DefDispWinBottomOffset",         m_defDispWinBottomOffset,                 0, "Specifies the bottom offset of the default display window from the conformance window")
  ("FrameFieldInfoPresentFlag",      m_frameFieldInfoPresentFlag,               false, "Indicates that pic_struct and field coding related values are present in picture timing SEI messages")
  ("PocProportionalToTimingFlag",   m_pocProportionalToTimingFlag,         false, "Indicates that the POC value is proportional to the output time w.r.t. first picture in CVS")
  ("NumTicksPocDiffOneMinus1",      m_numTicksPocDiffOneMinus1,                0, "Number of ticks minus 1 that for a POC difference of one")
  ("BitstreamRestriction",           m_bitstreamRestrictionFlag,           false, "Signals whether bitstream restriction parameters are present")
  ("TilesFixedStructure",            m_tilesFixedStructureFlag,            false, "Indicates that each active picture parameter set has the same values of the syntax elements related to tiles")
  ("MotionVectorsOverPicBoundaries", m_motionVectorsOverPicBoundariesFlag, false, "Indicates that no samples outside the picture boundaries are used for inter prediction")
  ("MaxBytesPerPicDenom",            m_maxBytesPerPicDenom,                    2, "Indicates a number of bytes not exceeded by the sum of the sizes of the VCL NAL units associated with any coded picture")
  ("MaxBitsPerMinCuDenom",           m_maxBitsPerMinCuDenom,                   1, "Indicates an upper bound for the number of bits of coding_unit() data")
  ("Log2MaxMvLengthHorizontal",      m_log2MaxMvLengthHorizontal,             15, "Indicate the maximum absolute value of a decoded horizontal MV component in quarter-pel luma units")
  ("Log2MaxMvLengthVertical",        m_log2MaxMvLengthVertical,               15, "Indicate the maximum absolute value of a decoded vertical MV component in quarter-pel luma units")
  ("SEIRecoveryPoint",               m_recoveryPointSEIEnabled,                0, "Control generation of recovery point SEI messages")
  ("SEIBufferingPeriod",             m_bufferingPeriodSEIEnabled,              0, "Control generation of buffering period SEI messages")
  ("SEIPictureTiming",               m_pictureTimingSEIEnabled,                0, "Control generation of picture timing SEI messages")
  ("SEIToneMappingInfo",                       m_toneMappingInfoSEIEnabled,    false, "Control generation of Tone Mapping SEI messages")
  ("SEIToneMapId",                             m_toneMapId,                        0, "Specifies Id of Tone Mapping SEI message for a given session")
  ("SEIToneMapCancelFlag",                     m_toneMapCancelFlag,            false, "Indicates that Tone Mapping SEI message cancels the persistance or follows")
  ("SEIToneMapPersistenceFlag",                m_toneMapPersistenceFlag,        true, "Specifies the persistence of the Tone Mapping SEI message")
  ("SEIToneMapCodedDataBitDepth",              m_toneMapCodedDataBitDepth,         8, "Specifies Coded Data BitDepth of Tone Mapping SEI messages")
  ("SEIToneMapTargetBitDepth",                 m_toneMapTargetBitDepth,            8, "Specifies Output BitDepth of Tome mapping function")
  ("SEIToneMapModelId",                        m_toneMapModelId,                   0, "Specifies Model utilized for mapping coded data into target_bit_depth range\n"
                                                                                      "\t0:  linear mapping with clipping\n"
                                                                                      "\t1:  sigmoidal mapping\n"
                                                                                      "\t2:  user-defined table mapping\n"
                                                                                      "\t3:  piece-wise linear mapping\n"
                                                                                      "\t4:  luminance dynamic range information ")
  ("SEIToneMapMinValue",                              m_toneMapMinValue,                          0, "Specifies the minimum value in mode 0")
  ("SEIToneMapMaxValue",                              m_toneMapMaxValue,                       1023, "Specifies the maxmum value in mode 0")
  ("SEIToneMapSigmoidMidpoint",                       m_sigmoidMidpoint,                        512, "Specifies the centre point in mode 1")
  ("SEIToneMapSigmoidWidth",                          m_sigmoidWidth,                           960, "Specifies the distance between 5% and 95% values of the target_bit_depth in mode 1")
  ("SEIToneMapStartOfCodedInterval",                  cfg_startOfCodedInterval,          string(""), "Array of user-defined mapping table")
  ("SEIToneMapNumPivots",                             m_numPivots,                                0, "Specifies the number of pivot points in mode 3")
  ("SEIToneMapCodedPivotValue",                       cfg_codedPivotValue,               string(""), "Array of pivot point")
  ("SEIToneMapTargetPivotValue",                      cfg_targetPivotValue,              string(""), "Array of pivot point")
  ("SEIToneMapCameraIsoSpeedIdc",                     m_cameraIsoSpeedIdc,                        0, "Indicates the camera ISO speed for daylight illumination")
  ("SEIToneMapCameraIsoSpeedValue",                   m_cameraIsoSpeedValue,                    400, "Specifies the camera ISO speed for daylight illumination of Extended_ISO")
  ("SEIToneMapExposureCompensationValueSignFlag",     m_exposureCompensationValueSignFlag,        0, "Specifies the sign of ExposureCompensationValue")
  ("SEIToneMapExposureCompensationValueNumerator",    m_exposureCompensationValueNumerator,       0, "Specifies the numerator of ExposureCompensationValue")
  ("SEIToneMapExposureCompensationValueDenomIdc",     m_exposureCompensationValueDenomIdc,        2, "Specifies the denominator of ExposureCompensationValue")
  ("SEIToneMapRefScreenLuminanceWhite",               m_refScreenLuminanceWhite,                350, "Specifies reference screen brightness setting in units of candela per square metre")
  ("SEIToneMapExtendedRangeWhiteLevel",               m_extendedRangeWhiteLevel,                800, "Indicates the luminance dynamic range")
  ("SEIToneMapNominalBlackLevelLumaCodeValue",        m_nominalBlackLevelLumaCodeValue,          16, "Specifies luma sample value of the nominal black level assigned decoded pictures")
  ("SEIToneMapNominalWhiteLevelLumaCodeValue",        m_nominalWhiteLevelLumaCodeValue,         235, "Specifies luma sample value of the nominal white level assigned decoded pictures")
  ("SEIToneMapExtendedWhiteLevelLumaCodeValue",       m_extendedWhiteLevelLumaCodeValue,        300, "Specifies luma sample value of the extended dynamic range assigned decoded pictures")
  ("SEIFramePacking",                m_framePackingSEIEnabled,                 0, "Control generation of frame packing SEI messages")
  ("SEIFramePackingType",            m_framePackingSEIType,                    0, "Define frame packing arrangement\n"
                                                                                  "\t0: checkerboard - pixels alternatively represent either frames\n"
                                                                                  "\t1: column alternation - frames are interlaced by column\n"
                                                                                  "\t2: row alternation - frames are interlaced by row\n"
                                                                                  "\t3: side by side - frames are displayed horizontally\n"
                                                                                  "\t4: top bottom - frames are displayed vertically\n"
                                                                                  "\t5: frame alternation - one frame is alternated with the other")
  ("SEIFramePackingId",              m_framePackingSEIId,                      0, "Id of frame packing SEI message for a given session")
  ("SEIFramePackingQuincunx",        m_framePackingSEIQuincunx,                0, "Indicate the presence of a Quincunx type video frame")
  ("SEIFramePackingInterpretation",  m_framePackingSEIInterpretation,          0, "Indicate the interpretation of the frame pair\n"
                                                                                  "\t0: unspecified\n"
                                                                                  "\t1: stereo pair, frame0 represents left view\n"
                                                                                  "\t2: stereo pair, frame0 represents right view")
  ("SEIDisplayOrientation",          m_displayOrientationSEIAngle,             0, "Control generation of display orientation SEI messages\n"
                                                              "\tN: 0 < N < (2^16 - 1) enable display orientation SEI message with anticlockwise_rotation = N and display_orientation_repetition_period = 1\n"
                                                              "\t0: disable")
  ("SEITemporalLevel0Index",         m_temporalLevel0IndexSEIEnabled,          0, "Control generation of temporal level 0 index SEI messages")
  ("SEIGradualDecodingRefreshInfo",  m_gradualDecodingRefreshInfoEnabled,      0, "Control generation of gradual decoding refresh information SEI message")
  ("SEIDecodingUnitInfo",             m_decodingUnitInfoSEIEnabled,                       0, "Control generation of decoding unit information SEI message.")
  ("SEISOPDescription",              m_SOPDescriptionSEIEnabled,              0, "Control generation of SOP description SEI messages")
  ("SEIScalableNesting",             m_scalableNestingSEIEnabled,              0, "Control generation of scalable nesting SEI messages")
  ("SubBitstreamPropSEIEnabled",              m_subBistreamPropSEIEnabled,    false                     ,"Enable signaling of sub-bitstream property SEI message")
  ("SEISubBitstreamNumAdditionalSubStreams",  m_sbPropNumAdditionalSubStreams,0, "Number of substreams for which additional information is signalled")
  ("SEISubBitstreamSubBitstreamMode",         m_sbPropSubBitstreamMode,       std::vector< Int  >(1,0)  ,"Specifies mode of generation of the i-th sub-bitstream (0 or 1)")
  ("SEISubBitstreamOutputLayerSetIdxToVps",   m_sbPropOutputLayerSetIdxToVps, std::vector< Int  >(1,0)  ,"Specifies output layer set index of the i-th sub-bitstream ")
  ("SEISubBitstreamHighestSublayerId",        m_sbPropHighestSublayerId,      std::vector< Int  >(1,0)  ,"Specifies highest TemporalId of the i-th sub-bitstream")
  ("SEISubBitstreamAvgBitRate",               m_sbPropAvgBitRate,             std::vector< Int  >(1,0)  ,"Specifies average bit rate of the i-th sub-bitstream")
  ("SEISubBitstreamMaxBitRate",               m_sbPropMaxBitRate,             std::vector< Int  >(1,0)  ,"Specifies maximum bit rate of the i-th sub-bitstream")
#if H_3D
  ("CameraParameterFile,cpf", m_pchCameraParameterFile,    (Char *) 0, "Camera Parameter File Name")
  ("BaseViewCameraNumbers" ,  m_pchBaseViewCameraNumbers,  (Char *) 0, "Numbers of base views")
  ("CodedCamParsPrecision",   m_iCodedCamParPrecision,  STD_CAM_PARAMETERS_PRECISION, "precision for coding of camera parameters (in units of 2^(-x) luma samples)" )
/* View Synthesis Optimization */

#if H_3D_VSO
  ("VSOConfig",                       m_pchVSOConfig            , (Char *) 0    , "VSO configuration")
  ("VSO",                             m_bUseVSO                 , false         , "Use VSO" )    
  ("VSOMode",                         m_uiVSOMode               , (UInt)   4    , "VSO Mode")
  ("LambdaScaleVSO",                  m_dLambdaScaleVSO         , (Double) 1    , "Lambda Scaling for VSO")
  ("VSOLSTable",                      m_bVSOLSTable             , true          , "Depth QP dependent video/depth rate allocation by Lagrange multiplier" )      
  ("ForceLambdaScaleVSO",             m_bForceLambdaScaleVSO    , false         , "Force using Lambda Scale VSO also in non-VSO-Mode")
  ("AllowNegDist",                    m_bAllowNegDist           , true          , "Allow negative Distortion in VSO")
  
  ("UseEstimatedVSD",                 m_bUseEstimatedVSD        , true          , "Model based VSD estimation instead of rendering based for some encoder decisions" )      
  ("VSOEarlySkip",                    m_bVSOEarlySkip           , true          , "Early skip of VSO computation if synthesis error assumed to be zero" )      
  
  ("WVSO",                            m_bUseWVSO                , true          , "Use depth fidelity term for VSO" )
  ("VSOWeight",                       m_iVSOWeight              , 10            , "Synthesized View Distortion Change weight" )
  ("VSDWeight",                       m_iVSDWeight              , 1             , "View Synthesis Distortion estimate weight" )
  ("DWeight",                         m_iDWeight                , 1             , "Depth Distortion weight" )

#endif //HHI_VSO
#if H_3D_QTLPC
  ("QTL",                             m_bUseQTL                 , true          , "Use depth Quadtree Limitation" )
  ("PC",                              m_bUsePC                  , true          , "Use Predictive Coding with QTL" )
#endif
#if H_3D_IV_MERGE
  ("IvMvPred",                        m_ivMvPredFlag            , std::vector<Bool>(2, true)            , "inter view motion prediction " )
#endif
#if H_3D_NBDV_REF
  ("DepthRefinement",                 m_depthRefinementFlag,    true           , "depth refinement by DoNBDV" )  
#endif
#if H_3D_VSP
  ("ViewSynthesisPred",               m_viewSynthesisPredFlag,  true           , "view synthesis prediction " )  
#endif
#if H_3D
  ("IvMvScaling",                     m_ivMvScalingFlag      ,  true            , "inter view motion vector scaling" )    
#endif
#endif //H_3D
  ;
  #if H_MV
  // parse coding structure
  for( Int k = 0; k < MAX_NUM_LAYERS; k++ )
  {
    m_GOPListMvc.push_back( new GOPEntry[MAX_GOP + 1] );
    if( k == 0 )
    {
      m_GOPListMvc[0][0].m_sliceType = 'I'; 
      for( Int i = 1; i < MAX_GOP + 1; i++ ) 
      {
        std::ostringstream cOSS;
        cOSS<<"Frame"<<i;
        opts.addOptions()( cOSS.str(), m_GOPListMvc[k][i-1], GOPEntry() );
        if ( i != 1 )
        {
          opts.opt_list.back()->opt->opt_duplicate = true; 
        }        
      }
    }
    else
    {
      std::ostringstream cOSS1;
      cOSS1<<"FrameI"<<"_l"<<k;

      opts.addOptions()(cOSS1.str(), m_GOPListMvc[k][MAX_GOP], GOPEntry());
      if ( k > 1 )
      {
        opts.opt_list.back()->opt->opt_duplicate = true; 
      }        


      for( Int i = 1; i < MAX_GOP + 1; i++ ) 
      {
        std::ostringstream cOSS2;
        cOSS2<<"Frame"<<i<<"_l"<<k;
        opts.addOptions()(cOSS2.str(), m_GOPListMvc[k][i-1], GOPEntry());
        if ( i != 1 || k > 0 )
        {
          opts.opt_list.back()->opt->opt_duplicate = true; 
        }        
      }
    }
  }
#else
  for(Int i=1; i<MAX_GOP+1; i++) {
    std::ostringstream cOSS;
    cOSS<<"Frame"<<i;
    opts.addOptions()(cOSS.str(), m_GOPList[i-1], GOPEntry());
  }
#endif
  po::setDefaults(opts);
  const list<const Char*>& argv_unhandled = po::scanArgv(opts, argc, (const Char**) argv);

  if(m_isField)
  {
    //Frame height
    m_iSourceHeightOrg = m_iSourceHeight;
    //Field height
    m_iSourceHeight = m_iSourceHeight >> 1;
    //number of fields to encode
    m_framesToBeEncoded *= 2;
  }
  
  for (list<const Char*>::const_iterator it = argv_unhandled.begin(); it != argv_unhandled.end(); it++)
  {
    fprintf(stderr, "Unhandled argument ignored: `%s'\n", *it);
  }
  
  if (argc == 1 || do_help)
  {
    /* argc == 1: no options have been specified */
    po::doHelp(cout, opts);
    return false;
  }
  
  /*
   * Set any derived parameters
   */
  /* convert std::string to c string for compatability */
#if !H_MV
  m_pchInputFile = cfg_InputFile.empty() ? NULL : strdup(cfg_InputFile.c_str());
#endif
  m_pchBitstreamFile = cfg_BitstreamFile.empty() ? NULL : strdup(cfg_BitstreamFile.c_str());
#if !H_MV
  m_pchReconFile = cfg_ReconFile.empty() ? NULL : strdup(cfg_ReconFile.c_str());
#endif
  m_pchdQPFile = cfg_dQPFile.empty() ? NULL : strdup(cfg_dQPFile.c_str());
  
  Char* pColumnWidth = cfg_ColumnWidth.empty() ? NULL: strdup(cfg_ColumnWidth.c_str());
  Char* pRowHeight = cfg_RowHeight.empty() ? NULL : strdup(cfg_RowHeight.c_str());
  if( m_iUniformSpacingIdr == 0 && m_iNumColumnsMinus1 > 0 )
  {
    char *columnWidth;
    int  i=0;
    m_pColumnWidth = new UInt[m_iNumColumnsMinus1];
    columnWidth = strtok(pColumnWidth, " ,-");
    while(columnWidth!=NULL)
    {
      if( i>=m_iNumColumnsMinus1 )
      {
        printf( "The number of columns whose width are defined is larger than the allowed number of columns.\n" );
        exit( EXIT_FAILURE );
      }
      *( m_pColumnWidth + i ) = atoi( columnWidth );
      columnWidth = strtok(NULL, " ,-");
      i++;
    }
    if( i<m_iNumColumnsMinus1 )
    {
      printf( "The width of some columns is not defined.\n" );
      exit( EXIT_FAILURE );
    }
  }
  else
  {
    m_pColumnWidth = NULL;
  }

  if( m_iUniformSpacingIdr == 0 && m_iNumRowsMinus1 > 0 )
  {
    char *rowHeight;
    int  i=0;
    m_pRowHeight = new UInt[m_iNumRowsMinus1];
    rowHeight = strtok(pRowHeight, " ,-");
    while(rowHeight!=NULL)
    {
      if( i>=m_iNumRowsMinus1 )
      {
        printf( "The number of rows whose height are defined is larger than the allowed number of rows.\n" );
        exit( EXIT_FAILURE );
      }
      *( m_pRowHeight + i ) = atoi( rowHeight );
      rowHeight = strtok(NULL, " ,-");
      i++;
    }
    if( i<m_iNumRowsMinus1 )
    {
      printf( "The height of some rows is not defined.\n" );
      exit( EXIT_FAILURE );
   }
  }
  else
  {
    m_pRowHeight = NULL;
  }
#if H_MV
  free ( pColumnWidth );
  free ( pRowHeight   ); 
#endif
  m_scalingListFile = cfg_ScalingListFile.empty() ? NULL : strdup(cfg_ScalingListFile.c_str());
  
  /* rules for input, output and internal bitdepths as per help text */
  if (!m_internalBitDepthY) { m_internalBitDepthY = m_inputBitDepthY; }
  if (!m_internalBitDepthC) { m_internalBitDepthC = m_internalBitDepthY; }
  if (!m_inputBitDepthC) { m_inputBitDepthC = m_inputBitDepthY; }
  if (!m_outputBitDepthY) { m_outputBitDepthY = m_internalBitDepthY; }
  if (!m_outputBitDepthC) { m_outputBitDepthC = m_internalBitDepthC; }

  // TODO:ChromaFmt assumes 4:2:0 below
  switch (m_conformanceMode)
  {
  case 0:
    {
      // no conformance or padding
      m_confLeft = m_confRight = m_confTop = m_confBottom = 0;
      m_aiPad[1] = m_aiPad[0] = 0;
      break;
    }
  case 1:
    {
      // automatic padding to minimum CU size
      Int minCuSize = m_uiMaxCUHeight >> (m_uiMaxCUDepth - 1);
      if (m_iSourceWidth % minCuSize)
      {
        m_aiPad[0] = m_confRight  = ((m_iSourceWidth / minCuSize) + 1) * minCuSize - m_iSourceWidth;
        m_iSourceWidth  += m_confRight;
      }
      if (m_iSourceHeight % minCuSize)
      {
        m_aiPad[1] = m_confBottom = ((m_iSourceHeight / minCuSize) + 1) * minCuSize - m_iSourceHeight;
        m_iSourceHeight += m_confBottom;
        if ( m_isField )
        {
          m_iSourceHeightOrg += m_confBottom << 1;
          m_aiPad[1] = m_confBottom << 1;
        }
      }
      if (m_aiPad[0] % TComSPS::getWinUnitX(CHROMA_420) != 0)
      {
        fprintf(stderr, "Error: picture width is not an integer multiple of the specified chroma subsampling\n");
        exit(EXIT_FAILURE);
      }
      if (m_aiPad[1] % TComSPS::getWinUnitY(CHROMA_420) != 0)
      {
        fprintf(stderr, "Error: picture height is not an integer multiple of the specified chroma subsampling\n");
        exit(EXIT_FAILURE);
      }
      break;
    }
  case 2:
    {
      //padding
      m_iSourceWidth  += m_aiPad[0];
      m_iSourceHeight += m_aiPad[1];
      m_confRight  = m_aiPad[0];
      m_confBottom = m_aiPad[1];
      break;
    }
  case 3:
    {
      // conformance
      if ((m_confLeft == 0) && (m_confRight == 0) && (m_confTop == 0) && (m_confBottom == 0))
      {
        fprintf(stderr, "Warning: Conformance window enabled, but all conformance window parameters set to zero\n");
      }
      if ((m_aiPad[1] != 0) || (m_aiPad[0]!=0))
      {
        fprintf(stderr, "Warning: Conformance window enabled, padding parameters will be ignored\n");
      }
      m_aiPad[1] = m_aiPad[0] = 0;
      break;
    }
  }
  
  // allocate slice-based dQP values
#if H_MV
  xResizeVector( m_viewOrderIndex    ); 

  std::vector<Int> uniqueViewOrderIndices; 
  for( Int layer = 0; layer < m_numberOfLayers; layer++ )
  {    
    Bool isIn = false; 
    for ( Int i = 0 ; i < uniqueViewOrderIndices.size(); i++ )
    {
      isIn = isIn || ( m_viewOrderIndex[ layer ] == uniqueViewOrderIndices[ i ] ); 
    }
    if ( !isIn ) 
    {
      uniqueViewOrderIndices.push_back( m_viewOrderIndex[ layer ] ); 
    } 
  }
  m_iNumberOfViews = (Int) uniqueViewOrderIndices.size(); 

#if H_3D
  xResizeVector( m_depthFlag ); 
#endif
  xResizeVector( m_fQP ); 

  for( Int layer = 0; layer < m_numberOfLayers; layer++ )
  {
    m_aidQP.push_back( new Int[ m_framesToBeEncoded + m_iGOPSize + 1 ] );
    ::memset( m_aidQP[layer], 0, sizeof(Int)*( m_framesToBeEncoded + m_iGOPSize + 1 ) );

    // handling of floating-point QP values
    // if QP is not integer, sequence is split into two sections having QP and QP+1
    m_iQP.push_back((Int)( m_fQP[layer] ));
    if ( m_iQP[layer] < m_fQP[layer] )
    {
      Int iSwitchPOC = (Int)( m_framesToBeEncoded - (m_fQP[layer] - m_iQP[layer])*m_framesToBeEncoded + 0.5 );

      iSwitchPOC = (Int)( (Double)iSwitchPOC / m_iGOPSize + 0.5 )*m_iGOPSize;
      for ( Int i=iSwitchPOC; i<m_framesToBeEncoded + m_iGOPSize + 1; i++ )
      {
        m_aidQP[layer][i] = 1;
      }
    }
  }

  xResizeVector( m_bLoopFilterDisable ); 
  xResizeVector( m_bUseSAO ); 
  xResizeVector( m_iIntraPeriod ); 
  xResizeVector( m_tilesInUseFlag ); 
  xResizeVector( m_loopFilterNotAcrossTilesFlag ); 
  xResizeVector( m_wppInUseFlag ); 
#else
  m_aidQP = new Int[ m_framesToBeEncoded + m_iGOPSize + 1 ];
  ::memset( m_aidQP, 0, sizeof(Int)*( m_framesToBeEncoded + m_iGOPSize + 1 ) );
  
  // handling of floating-point QP values
  // if QP is not integer, sequence is split into two sections having QP and QP+1
  m_iQP = (Int)( m_fQP );
  if ( m_iQP < m_fQP )
  {
    Int iSwitchPOC = (Int)( m_framesToBeEncoded - (m_fQP - m_iQP)*m_framesToBeEncoded + 0.5 );
    
    iSwitchPOC = (Int)( (Double)iSwitchPOC / m_iGOPSize + 0.5 )*m_iGOPSize;
    for ( Int i=iSwitchPOC; i<m_framesToBeEncoded + m_iGOPSize + 1; i++ )
    {
      m_aidQP[i] = 1;
    }
  }
#endif
  
  // reading external dQP description from file
  if ( m_pchdQPFile )
  {
    FILE* fpt=fopen( m_pchdQPFile, "r" );
    if ( fpt )
    {
#if H_MV
      for( Int layer = 0; layer < m_numberOfLayers; layer++ )
      {
#endif
      Int iValue;
      Int iPOC = 0;
      while ( iPOC < m_framesToBeEncoded )
      {
        if ( fscanf(fpt, "%d", &iValue ) == EOF ) break;
#if H_MV
        m_aidQP[layer][ iPOC ] = iValue;
        iPOC++;
      }
#else
        m_aidQP[ iPOC ] = iValue;
        iPOC++;
#endif
      }
      fclose(fpt);
    }
  }
  m_iWaveFrontSubstreams = m_iWaveFrontSynchro ? (m_iSourceHeight + m_uiMaxCUHeight - 1) / m_uiMaxCUHeight : 1;

  if( m_toneMappingInfoSEIEnabled && !m_toneMapCancelFlag )
  {
    Char* pcStartOfCodedInterval = cfg_startOfCodedInterval.empty() ? NULL: strdup(cfg_startOfCodedInterval.c_str());
    Char* pcCodedPivotValue = cfg_codedPivotValue.empty() ? NULL: strdup(cfg_codedPivotValue.c_str());
    Char* pcTargetPivotValue = cfg_targetPivotValue.empty() ? NULL: strdup(cfg_targetPivotValue.c_str());
    if( m_toneMapModelId == 2 && pcStartOfCodedInterval )
    {
      char *startOfCodedInterval;
      UInt num = 1u<< m_toneMapTargetBitDepth;
      m_startOfCodedInterval = new Int[num];
      ::memset( m_startOfCodedInterval, 0, sizeof(Int)*num );
      startOfCodedInterval = strtok(pcStartOfCodedInterval, " .");
      int i = 0;
      while( startOfCodedInterval && ( i < num ) )
      {
        m_startOfCodedInterval[i] = atoi( startOfCodedInterval );
        startOfCodedInterval = strtok(NULL, " .");
        i++;
      }
    } 
    else
    {
      m_startOfCodedInterval = NULL;
    }
    if( ( m_toneMapModelId == 3 ) && ( m_numPivots > 0 ) )
    {
      if( pcCodedPivotValue && pcTargetPivotValue )
      {
        char *codedPivotValue;
        char *targetPivotValue;
        m_codedPivotValue = new Int[m_numPivots];
        m_targetPivotValue = new Int[m_numPivots];
        ::memset( m_codedPivotValue, 0, sizeof(Int)*( m_numPivots ) );
        ::memset( m_targetPivotValue, 0, sizeof(Int)*( m_numPivots ) );
        codedPivotValue = strtok(pcCodedPivotValue, " .");
        int i=0;
        while(codedPivotValue&&i<m_numPivots)
        {
          m_codedPivotValue[i] = atoi( codedPivotValue );
          codedPivotValue = strtok(NULL, " .");
          i++;
        }
        i=0;
        targetPivotValue = strtok(pcTargetPivotValue, " .");
        while(targetPivotValue&&i<m_numPivots)
        {
          m_targetPivotValue[i]= atoi( targetPivotValue );
          targetPivotValue = strtok(NULL, " .");
          i++;
        }
      }
    }
    else
    {
      m_codedPivotValue = NULL;
      m_targetPivotValue = NULL;
    }
  }
#if H_3D
  // set global varibles
  xSetGlobal();
#if H_3D_VSO
// Table base optimization 
  // Q&D
  Double adLambdaScaleTable[] = 
  {  0.031250, 0.031639, 0.032029, 0.032418, 0.032808, 0.033197, 0.033586, 0.033976, 0.034365, 0.034755, 
     0.035144, 0.035533, 0.035923, 0.036312, 0.036702, 0.037091, 0.037480, 0.037870, 0.038259, 0.038648, 
     0.039038, 0.039427, 0.039817, 0.040206, 0.040595, 0.040985, 0.041374, 0.041764, 0.042153, 0.042542, 
     0.042932, 0.043321, 0.043711, 0.044100, 0.044194, 0.053033, 0.061872, 0.070711, 0.079550, 0.088388, 
     0.117851, 0.147314, 0.176777, 0.235702, 0.294628, 0.353553, 0.471405, 0.589256, 0.707107, 0.707100, 
     0.753550, 0.800000  
  }; 
  if ( m_bUseVSO && m_bVSOLSTable )
  {
    Int firstDepthLayer = -1; 
    for (Int layer = 0; layer < m_numberOfLayers; layer++ )
    {
      if ( m_depthFlag[ layer ])
      {
        firstDepthLayer = layer;
        break; 
      }
    }
    AOT( firstDepthLayer == -1 );
    AOT( (m_iQP[firstDepthLayer] < 0) || (m_iQP[firstDepthLayer] > 51));
    m_dLambdaScaleVSO *= adLambdaScaleTable[m_iQP[firstDepthLayer]]; 
  }
#endif
#if H_3D_VSO
if ( m_bUseVSO && m_uiVSOMode == 4)
{
  m_cRenModStrParser.setString( m_iNumberOfViews, m_pchVSOConfig );
  m_cCameraData     .init     ( ((UInt) m_iNumberOfViews ), 
                                      g_bitDepthY,
                                (UInt)m_iCodedCamParPrecision,
                                      m_FrameSkip,
                                (UInt)m_framesToBeEncoded,
                                      m_pchCameraParameterFile,
                                      m_pchBaseViewCameraNumbers,
                                      NULL,
                                      m_cRenModStrParser.getSynthViews(),
                                      LOG2_DISP_PREC_LUT );
}
else if ( m_bUseVSO && m_uiVSOMode != 4 )
{
  m_cCameraData     .init     ( ((UInt) m_iNumberOfViews ), 
                                      g_bitDepthY,
                                (UInt)m_iCodedCamParPrecision,
                                      m_FrameSkip,
                                (UInt)m_framesToBeEncoded,
                                      m_pchCameraParameterFile,
                                      m_pchBaseViewCameraNumbers,
                                      m_pchVSOConfig,
                                      NULL,
                                      LOG2_DISP_PREC_LUT );
}
else
{
  m_cCameraData     .init     ( ((UInt) m_iNumberOfViews ), 
    g_bitDepthY,
    (UInt) m_iCodedCamParPrecision,
    m_FrameSkip,
    (UInt) m_framesToBeEncoded,
    m_pchCameraParameterFile,
    m_pchBaseViewCameraNumbers,
    NULL,
    NULL,
    LOG2_DISP_PREC_LUT );
}
#else
  m_cCameraData     .init     ( ((UInt) m_iNumberOfViews ), 
    g_bitDepthY,
    (UInt) m_iCodedCamParPrecision,
    m_FrameSkip,
    (UInt) m_framesToBeEncoded,
    m_pchCameraParameterFile,
    m_pchBaseViewCameraNumbers,
    NULL,
    NULL,
    LOG2_DISP_PREC_LUT );
#endif
  m_cCameraData.check( false, true );
#endif
  // check validity of input parameters
  xCheckParameter();

#if !H_3D
  // set global varibles
  xSetGlobal();
#endif
  
  // print-out parameters
  xPrintParameter();
  
  return true;
}
// ====================================================================================================================
// Private member functions
// ====================================================================================================================

Bool confirmPara(Bool bflag, const Char* message);

Void TAppEncCfg::xCheckParameter()
{
  if (!m_decodedPictureHashSEIEnabled)
  {
    fprintf(stderr, "******************************************************************\n");
    fprintf(stderr, "** WARNING: --SEIDecodedPictureHash is now disabled by default. **\n");
    fprintf(stderr, "**          Automatic verification of decoded pictures by a     **\n");
    fprintf(stderr, "**          decoder requires this option to be enabled.         **\n");
    fprintf(stderr, "******************************************************************\n");
  }
  if( m_profile==Profile::NONE )
  {
    fprintf(stderr, "***************************************************************************\n");
    fprintf(stderr, "** WARNING: For conforming bitstreams a valid Profile value must be set! **\n");
    fprintf(stderr, "***************************************************************************\n");
  }
  if( m_level==Level::NONE )
  {
    fprintf(stderr, "***************************************************************************\n");
    fprintf(stderr, "** WARNING: For conforming bitstreams a valid Level value must be set!   **\n");
    fprintf(stderr, "***************************************************************************\n");
  }

  Bool check_failed = false; /* abort if there is a fatal configuration problem */
#define xConfirmPara(a,b) check_failed |= confirmPara(a,b)
  // check range of parameters
  xConfirmPara( m_inputBitDepthY < 8,                                                     "InputBitDepth must be at least 8" );
  xConfirmPara( m_inputBitDepthC < 8,                                                     "InputBitDepthC must be at least 8" );
  xConfirmPara( m_iFrameRate <= 0,                                                          "Frame rate must be more than 1" );
  xConfirmPara( m_framesToBeEncoded <= 0,                                                   "Total Number Of Frames encoded must be more than 0" );
#if H_MV
  xConfirmPara( m_numberOfLayers > MAX_NUM_LAYER_IDS ,                                      "NumberOfLayers must be less than or equal to MAX_NUM_LAYER_IDS");


  xConfirmPara( m_layerIdInNuh[0] != 0      , "LayerIdInNuh must be 0 for the first layer. ");
  xConfirmPara( (m_layerIdInNuh.size()!=1) && (m_layerIdInNuh.size() < m_numberOfLayers) , "LayerIdInNuh must be given for all layers. ");
  
#if H_3D
  xConfirmPara( m_scalabilityMask != 2 && m_scalabilityMask != 3, "Scalability Mask must be equal to 2 or 3. ");
#else
  xConfirmPara( m_scalabilityMask != 2 , "Scalability Mask must be equal to 2. ");
#endif

#if H_3D
  if ( m_scalabilityMask & ( 1 << DEPTH_ID ) )
  {
    m_dimIds.push_back( m_depthFlag ); 
  }
#endif

  m_dimIds.push_back( m_viewOrderIndex );   
  xConfirmPara(  m_dimensionIdLen.size() < m_dimIds.size(), "DimensionIdLen must be given for all dimensions. "   );   Int dimBitOffset[MAX_NUM_SCALABILITY_TYPES+1]; 

  dimBitOffset[ 0 ] = 0; 
  for (Int j = 1; j <= ((Int) m_dimIds.size() - m_splittingFlag ? 1 : 0); j++ )
 {
    dimBitOffset[ j ] = dimBitOffset[ j - 1 ] + m_dimensionIdLen[ j - 1]; 
  }

  if ( m_splittingFlag )
  {
    dimBitOffset[ (Int) m_dimIds.size() ] = 6; 
  }
  
  for( Int j = 0; j < m_dimIds.size(); j++ )
  {    
    xConfirmPara( m_dimIds[j].size() < m_numberOfLayers,  "DimensionId must be given for all layers and all dimensions. ");   
    xConfirmPara( (m_dimIds[j][0] != 0)                 , "DimensionId of layer 0 must be 0. " );
    xConfirmPara( m_dimensionIdLen[j] < 1 || m_dimensionIdLen[j] > 8, "DimensionIdLen must be greater than 0 and less than 9 in all dimensions. " ); 
     

   for( Int i = 1; i < m_numberOfLayers; i++ )
   {     
      xConfirmPara(  ( m_dimIds[j][i] < 0 ) || ( m_dimIds[j][i] > ( ( 1 << m_dimensionIdLen[j] ) - 1 ) )   , "DimensionId shall be in the range of 0 to 2^DimensionIdLen - 1. " );
      if ( m_splittingFlag )
      {
        Int layerIdInNuh = (m_layerIdInNuh.size()!=1) ? m_layerIdInNuh[i] :  i; 
        xConfirmPara( ( ( layerIdInNuh & ( (1 << dimBitOffset[ j + 1 ] ) - 1) ) >> dimBitOffset[ j ] )  != m_dimIds[j][ i ]  , "When Splitting Flag is equal to 1 dimension ids shall match values derived from layer ids. "); 
      }
   }
 }

 for( Int i = 0; i < m_numberOfLayers; i++ )
 {
   for( Int j = 0; j < i; j++ )
   {     
     Int numDiff  = 0; 
     Int lastDiff = -1; 
     for( Int dim = 0; dim < m_dimIds.size(); dim++ )
     {
       if ( m_dimIds[dim][i] != m_dimIds[dim][j] )
       {
         numDiff ++; 
         lastDiff = dim; 
       }
     }

     Bool allEqual = ( numDiff == 0 ); 

     if ( allEqual ) 
     {
       printf( "\nError: Positions of Layers %d and %d are identical in scalability space\n", i, j);
     }

     xConfirmPara( allEqual , "Each layer shall have a different position in scalability space." );

#if !H_3D_FCO
     if ( numDiff  == 1 ) 
     {
       Bool inc = m_dimIds[ lastDiff ][ i ] > m_dimIds[ lastDiff ][ j ]; 
       Bool shallBeButIsNotIncreasing = ( !inc  ) ; 
       if ( shallBeButIsNotIncreasing )
       {       
         printf( "\nError: Positions of Layers %d and %d is not increasing in dimension %d \n", i, j, lastDiff);        
       }
       xConfirmPara( shallBeButIsNotIncreasing,  "DimensionIds shall be increasing within one dimension. " );
     }
#endif
   }
 }

 /// ViewId 
 xConfirmPara( m_viewId.size() != m_iNumberOfViews, "The number of ViewIds must be equal to the number of views." ); 

  /// Layer sets
  xConfirmPara( m_vpsNumLayerSets < 0 || m_vpsNumLayerSets > 1024, "VpsNumLayerSets must be greater than 0 and less than 1025. ") ; 
  for( Int lsIdx = 0; lsIdx < m_vpsNumLayerSets; lsIdx++ )
  {
    if (lsIdx == 0)
    {
      xConfirmPara( m_layerIdsInSets[lsIdx].size() != 1 || m_layerIdsInSets[lsIdx][0] != 0 , "0-th layer shall only include layer 0. ");
    }
    for ( Int i = 0; i < m_layerIdsInSets[lsIdx].size(); i++ )
    {
      xConfirmPara( m_layerIdsInSets[lsIdx][i] < 0 || m_layerIdsInSets[lsIdx].size() >= MAX_NUM_LAYER_IDS, "LayerIdsInSet must be greater than and less than MAX_NUM_LAYER_IDS" ); 
    }
  }

  // Output layer sets
  xConfirmPara( m_outputLayerSetIdx.size() > 1024, "The number of output layer set indices must be less than 1025.") ;
  for (Int lsIdx = 0; lsIdx < m_outputLayerSetIdx.size(); lsIdx++)
  {   
    Int refLayerSetIdx = m_outputLayerSetIdx[ lsIdx ]; 
    xConfirmPara(  refLayerSetIdx < 0 || refLayerSetIdx >= m_vpsNumLayerSets, "Output layer set idx must be greater or equal to 0 and less than the VpsNumLayerSets." );

    for (Int i = 0; i < m_layerIdsInAddOutputLayerSet[ lsIdx ].size(); i++)
    {
      Bool isAlsoInLayerSet = false; 
      for (Int j = 0; j < m_layerIdsInSets[ refLayerSetIdx ].size(); j++ )
      {
        if ( m_layerIdsInSets[ refLayerSetIdx ][ j ] == m_layerIdsInAddOutputLayerSet[ lsIdx ][ i ] )
        {
          isAlsoInLayerSet = true; 
          break; 
        }        
      }
      xConfirmPara( !isAlsoInLayerSet, "All output layers of a output layer set be included in corresponding layer set.");
    }
  }

  xConfirmPara( m_defaultTargetOutputLayerIdc < 0 || m_defaultTargetOutputLayerIdc > 2, "Default target output layer idc must greater than or equal to 0 and less than or equal to 2." );  

  if( m_defaultTargetOutputLayerIdc != 2 )
  {
    Bool anyDefaultOutputFlag = false;   
    for (Int lsIdx = 0; lsIdx < m_vpsNumLayerSets; lsIdx++)
    { 
      anyDefaultOutputFlag = anyDefaultOutputFlag || ( m_layerIdsInDefOutputLayerSet[lsIdx].size() != 0 );
    }    
    printf( "\nWarning: Ignoring LayerIdsInDefOutputLayerSet parameters, since defaultTargetOuputLayerIdc is not equal 2.\n" );    
  }
  else  
  {  
    for (Int lsIdx = 0; lsIdx < m_vpsNumLayerSets; lsIdx++)
    { 
      for (Int i = 0; i < m_layerIdsInDefOutputLayerSet[ lsIdx ].size(); i++)
      {
        Bool inLayerSetFlag = false; 
        for (Int j = 0; j < m_layerIdsInSets[ lsIdx].size(); j++ )
        {
          if ( m_layerIdsInSets[ lsIdx ][ j ] == m_layerIdsInDefOutputLayerSet[ lsIdx ][ i ] )
          {
            inLayerSetFlag = true; 
            break; 
          }        
        }
        xConfirmPara( !inLayerSetFlag, "All output layers of a output layer set must be included in corresponding layer set.");
      }
    }
  }

  xConfirmPara( m_profileLevelTierIdx.size() < m_vpsNumLayerSets + m_outputLayerSetIdx.size(), "The number of Profile Level Tier indices must be equal to the number of layer set plus the number of output layer set indices" );

  // Layer Dependencies  
  for (Int i = 0; i < m_numberOfLayers; i++ )
  {
    xConfirmPara( (i == 0)  && m_directRefLayers[0].size() != 0, "Layer 0 shall not have reference layers." ); 
    xConfirmPara( m_directRefLayers[i].size() != m_dependencyTypes[ i ].size(), "Each reference layer shall have a reference type." ); 
    for (Int j = 0; j < m_directRefLayers[i].size(); j++)
    {
      xConfirmPara( m_directRefLayers[i][j] < 0 || m_directRefLayers[i][j] >= i , "Reference layer id shall be greater than or equal to 0 and less than dependent layer id"); 
      xConfirmPara( m_dependencyTypes[i][j] < 0 || m_dependencyTypes[i][j] >  2 , "Dependency type shall be greater than or equal to 0 and less than 3"); 
    }        
  }  
#endif
  xConfirmPara( m_iGOPSize < 1 ,                                                            "GOP Size must be greater or equal to 1" );
  xConfirmPara( m_iGOPSize > 1 &&  m_iGOPSize % 2,                                          "GOP Size must be a multiple of 2, if GOP Size is greater than 1" );
#if H_MV
  for( Int layer = 0; layer < m_numberOfLayers; layer++ )
  {
    xConfirmPara( (m_iIntraPeriod[layer] > 0 && m_iIntraPeriod[layer] < m_iGOPSize) || m_iIntraPeriod[layer] == 0, "Intra period must be more than GOP size, or -1 , not 0" );
  }
#else
  xConfirmPara( (m_iIntraPeriod > 0 && m_iIntraPeriod < m_iGOPSize) || m_iIntraPeriod == 0, "Intra period must be more than GOP size, or -1 , not 0" );
#endif
  xConfirmPara( m_iDecodingRefreshType < 0 || m_iDecodingRefreshType > 2,                   "Decoding Refresh Type must be equal to 0, 1 or 2" );
#if H_MV
  for( Int layer = 0; layer < m_numberOfLayers; layer++ )
  {
    xConfirmPara( m_iQP[layer] <  -6 * (m_internalBitDepthY - 8) || m_iQP[layer] > 51,      "QP exceeds supported range (-QpBDOffsety to 51)" );
  }
#else
  xConfirmPara( m_iQP <  -6 * (m_internalBitDepthY - 8) || m_iQP > 51,                    "QP exceeds supported range (-QpBDOffsety to 51)" );
#endif
  xConfirmPara( m_loopFilterBetaOffsetDiv2 < -6 || m_loopFilterBetaOffsetDiv2 > 6,          "Loop Filter Beta Offset div. 2 exceeds supported range (-6 to 6)");
  xConfirmPara( m_loopFilterTcOffsetDiv2 < -6 || m_loopFilterTcOffsetDiv2 > 6,              "Loop Filter Tc Offset div. 2 exceeds supported range (-6 to 6)");
  xConfirmPara( m_iFastSearch < 0 || m_iFastSearch > 2,                                     "Fast Search Mode is not supported value (0:Full search  1:Diamond  2:PMVFAST)" );
  xConfirmPara( m_iSearchRange < 0 ,                                                        "Search Range must be more than 0" );
  xConfirmPara( m_bipredSearchRange < 0 ,                                                   "Search Range must be more than 0" );
  xConfirmPara( m_iMaxDeltaQP > 7,                                                          "Absolute Delta QP exceeds supported range (0 to 7)" );
  xConfirmPara( m_iMaxCuDQPDepth > m_uiMaxCUDepth - 1,                                          "Absolute depth for a minimum CuDQP exceeds maximum coding unit depth" );

  xConfirmPara( m_cbQpOffset < -12,   "Min. Chroma Cb QP Offset is -12" );
  xConfirmPara( m_cbQpOffset >  12,   "Max. Chroma Cb QP Offset is  12" );
  xConfirmPara( m_crQpOffset < -12,   "Min. Chroma Cr QP Offset is -12" );
  xConfirmPara( m_crQpOffset >  12,   "Max. Chroma Cr QP Offset is  12" );

  xConfirmPara( m_iQPAdaptationRange <= 0,                                                  "QP Adaptation Range must be more than 0" );
  if (m_iDecodingRefreshType == 2)
  {
#if H_MV
    for (Int i = 0; i < m_numberOfLayers; i++ )
    {
      xConfirmPara( m_iIntraPeriod[i] > 0 && m_iIntraPeriod[i] <= m_iGOPSize ,                      "Intra period must be larger than GOP size for periodic IDR pictures");
    }
#else
    xConfirmPara( m_iIntraPeriod > 0 && m_iIntraPeriod <= m_iGOPSize ,                      "Intra period must be larger than GOP size for periodic IDR pictures");
#endif
  }
  xConfirmPara( (m_uiMaxCUWidth  >> m_uiMaxCUDepth) < 4,                                    "Minimum partition width size should be larger than or equal to 8");
  xConfirmPara( (m_uiMaxCUHeight >> m_uiMaxCUDepth) < 4,                                    "Minimum partition height size should be larger than or equal to 8");
  xConfirmPara( m_uiMaxCUWidth < 16,                                                        "Maximum partition width size should be larger than or equal to 16");
  xConfirmPara( m_uiMaxCUHeight < 16,                                                       "Maximum partition height size should be larger than or equal to 16");
  xConfirmPara( (m_iSourceWidth  % (m_uiMaxCUWidth  >> (m_uiMaxCUDepth-1)))!=0,             "Resulting coded frame width must be a multiple of the minimum CU size");
  xConfirmPara( (m_iSourceHeight % (m_uiMaxCUHeight >> (m_uiMaxCUDepth-1)))!=0,             "Resulting coded frame height must be a multiple of the minimum CU size");
  
  xConfirmPara( m_uiQuadtreeTULog2MinSize < 2,                                        "QuadtreeTULog2MinSize must be 2 or greater.");
  xConfirmPara( m_uiQuadtreeTULog2MaxSize > 5,                                        "QuadtreeTULog2MaxSize must be 5 or smaller.");
  xConfirmPara( (1<<m_uiQuadtreeTULog2MaxSize) > m_uiMaxCUWidth,                                        "QuadtreeTULog2MaxSize must be log2(maxCUSize) or smaller.");
  
  xConfirmPara( m_uiQuadtreeTULog2MaxSize < m_uiQuadtreeTULog2MinSize,                "QuadtreeTULog2MaxSize must be greater than or equal to m_uiQuadtreeTULog2MinSize.");
  xConfirmPara( (1<<m_uiQuadtreeTULog2MinSize)>(m_uiMaxCUWidth >>(m_uiMaxCUDepth-1)), "QuadtreeTULog2MinSize must not be greater than minimum CU size" ); // HS
  xConfirmPara( (1<<m_uiQuadtreeTULog2MinSize)>(m_uiMaxCUHeight>>(m_uiMaxCUDepth-1)), "QuadtreeTULog2MinSize must not be greater than minimum CU size" ); // HS
  xConfirmPara( ( 1 << m_uiQuadtreeTULog2MinSize ) > ( m_uiMaxCUWidth  >> m_uiMaxCUDepth ), "Minimum CU width must be greater than minimum transform size." );
  xConfirmPara( ( 1 << m_uiQuadtreeTULog2MinSize ) > ( m_uiMaxCUHeight >> m_uiMaxCUDepth ), "Minimum CU height must be greater than minimum transform size." );
  xConfirmPara( m_uiQuadtreeTUMaxDepthInter < 1,                                                         "QuadtreeTUMaxDepthInter must be greater than or equal to 1" );
  xConfirmPara( m_uiMaxCUWidth < ( 1 << (m_uiQuadtreeTULog2MinSize + m_uiQuadtreeTUMaxDepthInter - 1) ), "QuadtreeTUMaxDepthInter must be less than or equal to the difference between log2(maxCUSize) and QuadtreeTULog2MinSize plus 1" );
  xConfirmPara( m_uiQuadtreeTUMaxDepthIntra < 1,                                                         "QuadtreeTUMaxDepthIntra must be greater than or equal to 1" );
  xConfirmPara( m_uiMaxCUWidth < ( 1 << (m_uiQuadtreeTULog2MinSize + m_uiQuadtreeTUMaxDepthIntra - 1) ), "QuadtreeTUMaxDepthInter must be less than or equal to the difference between log2(maxCUSize) and QuadtreeTULog2MinSize plus 1" );
  
  xConfirmPara(  m_maxNumMergeCand < 1,  "MaxNumMergeCand must be 1 or greater.");
  xConfirmPara(  m_maxNumMergeCand > 5,  "MaxNumMergeCand must be 5 or smaller.");

#if H_3D_ARP
  xConfirmPara( ( 0 != m_uiUseAdvResPred ) &&  ( 1 != m_uiUseAdvResPred ), "UseAdvResPred must be 0 or 1." );
#endif
#if H_3D_SPIVMP
  xConfirmPara( m_iSubPULog2Size < 3,                                        "SubPULog2Size must be 3 or greater.");
  xConfirmPara( m_iSubPULog2Size > 6,                                        "SubPULog2Size must be 6 or smaller.");
  xConfirmPara( (1<<m_iSubPULog2Size) > m_uiMaxCUWidth,                      "SubPULog2Size must be log2(maxCUSize) or smaller.");
 
  xConfirmPara( m_iSubPUMPILog2Size < 3,                                        "SubPUMPILog2Size must be 3 or greater.");
  xConfirmPara( m_iSubPUMPILog2Size > 6,                                        "SubPUMPILog2Size must be 6 or smaller.");
  xConfirmPara( ( 1 << m_iSubPUMPILog2Size ) > m_uiMaxCUWidth,                  "SubPUMPILog2Size must be log2(maxCUSize) or smaller.");
#endif
#if ADAPTIVE_QP_SELECTION
#if H_MV
  for( Int layer = 0; layer < m_numberOfLayers; layer++ )
  {
    xConfirmPara( m_bUseAdaptQpSelect == true && m_iQP[layer] < 0,                                     "AdaptiveQpSelection must be disabled when QP < 0.");
  }
#else
  xConfirmPara( m_bUseAdaptQpSelect == true && m_iQP < 0,                                              "AdaptiveQpSelection must be disabled when QP < 0.");
#endif
  xConfirmPara( m_bUseAdaptQpSelect == true && (m_cbQpOffset !=0 || m_crQpOffset != 0 ),               "AdaptiveQpSelection must be disabled when ChromaQpOffset is not equal to 0.");
#endif

  if( m_usePCM)
  {
    xConfirmPara(  m_uiPCMLog2MinSize < 3,                                      "PCMLog2MinSize must be 3 or greater.");
    xConfirmPara(  m_uiPCMLog2MinSize > 5,                                      "PCMLog2MinSize must be 5 or smaller.");
    xConfirmPara(  m_pcmLog2MaxSize > 5,                                        "PCMLog2MaxSize must be 5 or smaller.");
    xConfirmPara(  m_pcmLog2MaxSize < m_uiPCMLog2MinSize,                       "PCMLog2MaxSize must be equal to or greater than m_uiPCMLog2MinSize.");
  }

  xConfirmPara( m_sliceMode < 0 || m_sliceMode > 3, "SliceMode exceeds supported range (0 to 3)" );
  if (m_sliceMode!=0)
  {
    xConfirmPara( m_sliceArgument < 1 ,         "SliceArgument should be larger than or equal to 1" );
  }
  xConfirmPara( m_sliceSegmentMode < 0 || m_sliceSegmentMode > 3, "SliceSegmentMode exceeds supported range (0 to 3)" );
  if (m_sliceSegmentMode!=0)
  {
    xConfirmPara( m_sliceSegmentArgument < 1 ,         "SliceSegmentArgument should be larger than or equal to 1" );
  }
  
  Bool tileFlag = (m_iNumColumnsMinus1 > 0 || m_iNumRowsMinus1 > 0 );
  xConfirmPara( tileFlag && m_iWaveFrontSynchro,            "Tile and Wavefront can not be applied together");

  //TODO:ChromaFmt assumes 4:2:0 below
  xConfirmPara( m_iSourceWidth  % TComSPS::getWinUnitX(CHROMA_420) != 0, "Picture width must be an integer multiple of the specified chroma subsampling");
  xConfirmPara( m_iSourceHeight % TComSPS::getWinUnitY(CHROMA_420) != 0, "Picture height must be an integer multiple of the specified chroma subsampling");

  xConfirmPara( m_aiPad[0] % TComSPS::getWinUnitX(CHROMA_420) != 0, "Horizontal padding must be an integer multiple of the specified chroma subsampling");
  xConfirmPara( m_aiPad[1] % TComSPS::getWinUnitY(CHROMA_420) != 0, "Vertical padding must be an integer multiple of the specified chroma subsampling");

  xConfirmPara( m_confLeft   % TComSPS::getWinUnitX(CHROMA_420) != 0, "Left conformance window offset must be an integer multiple of the specified chroma subsampling");
  xConfirmPara( m_confRight  % TComSPS::getWinUnitX(CHROMA_420) != 0, "Right conformance window offset must be an integer multiple of the specified chroma subsampling");
  xConfirmPara( m_confTop    % TComSPS::getWinUnitY(CHROMA_420) != 0, "Top conformance window offset must be an integer multiple of the specified chroma subsampling");
  xConfirmPara( m_confBottom % TComSPS::getWinUnitY(CHROMA_420) != 0, "Bottom conformance window offset must be an integer multiple of the specified chroma subsampling");

#if H_3D
  xConfirmPara( m_pchCameraParameterFile    == 0                ,   "CameraParameterFile must be given");
  xConfirmPara( m_pchBaseViewCameraNumbers  == 0                ,   "BaseViewCameraNumbers must be given" );
  xConfirmPara( ((UInt) m_numberOfLayers >> 1 ) != m_cCameraData.getBaseViewNumbers().size(),   "Number of Views in BaseViewCameraNumbers must be equal to NumberOfViews" );
  xConfirmPara    ( m_iCodedCamParPrecision < 0 || m_iCodedCamParPrecision > 5,       "CodedCamParsPrecision must be in range of 0..5" );
#if H_3D_VSO
    if( m_bUseVSO )
    {
      xConfirmPara(   m_pchVSOConfig            == 0                             ,   "VSO Setup string must be given");
      xConfirmPara( m_uiVSOMode > 4 ,                                                "VSO Mode must be less than 5");
    }
#endif
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

#if H_MV
  // validate that POC of same frame is identical across multiple layers
  Bool bErrorMvePoc = false;
  if( m_numberOfLayers > 1 )
  {
    for( Int k = 1; k < m_numberOfLayers; k++ )
    {
      for( Int i = 0; i < MAX_GOP; i++ )
      {
        if( m_GOPListMvc[k][i].m_POC != m_GOPListMvc[0][i].m_POC )
        {
          printf( "\nError: Frame%d_l%d POC %d is not identical to Frame%d POC\n", i, k, m_GOPListMvc[k][i].m_POC, i );
          bErrorMvePoc = true;
        }
      }
    }
  }
  xConfirmPara( bErrorMvePoc,  "Invalid inter-layer POC structure given" );

  // validate that baseview has no inter-view refs 
  Bool bErrorIvpBase = false;
  for( Int i = 0; i < MAX_GOP; i++ )
  {
    if( m_GOPListMvc[0][i].m_numActiveRefLayerPics != 0 )
    {
      printf( "\nError: Frame%d inter_layer refs not available in layer 0\n", i );
      bErrorIvpBase = true;
    }
  }
  xConfirmPara( bErrorIvpBase, "Inter-layer refs not possible in base layer" );

  // validate inter-view refs
  Bool bErrorIvpEnhV = false;
  if( m_numberOfLayers > 1 )
  {
    for( Int layer = 1; layer < m_numberOfLayers; layer++ )
    {
      for( Int i = 0; i < MAX_GOP+1; i++ )
      {
        GOPEntry gopEntry = m_GOPListMvc[layer][i];  
        for( Int j = 0; j < gopEntry.m_numActiveRefLayerPics; j++ )
        {
          Int ilPredLayerIdc = gopEntry.m_interLayerPredLayerIdc[j];
          if( ilPredLayerIdc < 0 || ilPredLayerIdc >= m_directRefLayers[layer].size() )
          {
            printf( "\nError: inter-layer ref idc %d is not available for Frame%d_l%d\n", gopEntry.m_interLayerPredLayerIdc[j], i, layer );
            bErrorIvpEnhV = true;
          }
          if( gopEntry.m_interViewRefPosL[0][j] < -1 || gopEntry.m_interViewRefPosL[0][j] > gopEntry.m_numRefPicsActive )
          {
            printf( "\nError: inter-layer ref pos %d on L0 is not available for Frame%d_l%d\n", gopEntry.m_interViewRefPosL[0][j], i, layer );
            bErrorIvpEnhV = true;
          }
          if( gopEntry.m_interViewRefPosL[1][j] < -1  || gopEntry.m_interViewRefPosL[1][j] > gopEntry.m_numRefPicsActive )
          {
            printf( "\nError: inter-layer ref pos %d on L1 is not available for Frame%d_l%d\n", gopEntry.m_interViewRefPosL[1][j], i, layer );
            bErrorIvpEnhV = true;
          }
        }
        if( i == MAX_GOP ) // inter-view refs at I pic position in base view
        {
          if( gopEntry.m_sliceType != 'B' && gopEntry.m_sliceType != 'P' && gopEntry.m_sliceType != 'I' )
          {
            printf( "\nError: slice type of FrameI_l%d must be equal to B or P or I\n", layer );
            bErrorIvpEnhV = true;
          }

          if( gopEntry.m_POC != 0 )
          {
            printf( "\nError: POC %d not possible for FrameI_l%d, must be 0\n", gopEntry.m_POC, layer );
            bErrorIvpEnhV = true;
          }

          if( gopEntry.m_temporalId != 0 )
          {
            printf( "\nWarning: Temporal id of FrameI_l%d must be 0 (cp. I-frame in base layer)\n", layer );
            gopEntry.m_temporalId = 0;
          }

          if( gopEntry.m_numRefPics != 0 )
          {
            printf( "\nWarning: temporal references not possible for FrameI_l%d\n", layer );
            for( Int j = 0; j < m_GOPListMvc[layer][MAX_GOP].m_numRefPics; j++ )
            {
              gopEntry.m_referencePics[j] = 0;
            }
            gopEntry.m_numRefPics = 0;
          }

          if( gopEntry.m_interRPSPrediction )
          {
            printf( "\nError: inter RPS prediction not possible for FrameI_l%d, must be 0\n", layer );
            bErrorIvpEnhV = true;
          }

          if( gopEntry.m_sliceType == 'I' && gopEntry.m_numActiveRefLayerPics != 0 )
          {
            printf( "\nError: inter-layer prediction not possible for FrameI_l%d with slice type I, #IL_ref_pics must be 0\n", layer );
            bErrorIvpEnhV = true;
          }

          if( gopEntry.m_numRefPicsActive > gopEntry.m_numActiveRefLayerPics )
          {
            gopEntry.m_numRefPicsActive = gopEntry.m_numActiveRefLayerPics;
          }

          if( gopEntry.m_sliceType == 'P' )
          {
            if( gopEntry.m_numActiveRefLayerPics < 1 )
            {
              printf( "\nError: #IL_ref_pics must be at least one for FrameI_l%d with slice type P\n", layer );
              bErrorIvpEnhV = true;
            }
            else
            {
              for( Int j = 0; j < gopEntry.m_numActiveRefLayerPics; j++ )
              {
                if( gopEntry.m_interViewRefPosL[1][j] != -1 )
                {
                  printf( "\nError: inter-layer ref pos %d on L1 not possible for FrameI_l%d with slice type P\n", gopEntry.m_interViewRefPosL[1][j], layer );
                  bErrorIvpEnhV = true;
                }
              }
            }
          }

          if( gopEntry.m_sliceType == 'B' && gopEntry.m_numActiveRefLayerPics < 1 )
          {
            printf( "\nError: #IL_ref_pics must be at least one for FrameI_l%d with slice type B\n", layer );
            bErrorIvpEnhV = true;
          }
        }
      }
    }
  }
  xConfirmPara( bErrorIvpEnhV, "Invalid inter-layer coding structure for enhancement layers given" );

  // validate temporal coding structure
  if( !bErrorMvePoc && !bErrorIvpBase && !bErrorIvpEnhV )
  {
    for( Int layer = 0; layer < m_numberOfLayers; layer++ )
    {
      GOPEntry* m_GOPList            = m_GOPListMvc           [layer]; // It is not a member, but this name helps avoiding code duplication !!!
      Int&      m_extraRPSs          = m_extraRPSsMvc         [layer]; // It is not a member, but this name helps avoiding code duplication !!!
      Int&      m_maxTempLayer       = m_maxTempLayerMvc      [layer]; // It is not a member, but this name helps avoiding code duplication !!!
      Int*      m_maxDecPicBuffering = m_maxDecPicBufferingMvc[layer]; // It is not a member, but this name helps avoiding code duplication !!!
      Int*      m_numReorderPics     = m_numReorderPicsMvc    [layer]; // It is not a member, but this name helps avoiding code duplication !!!
#endif
  /* if this is an intra-only sequence, ie IntraPeriod=1, don't verify the GOP structure
   * This permits the ability to omit a GOP structure specification */
#if H_MV
  if (m_iIntraPeriod[layer] == 1 && m_GOPList[0].m_POC == -1) {
#else
  if (m_iIntraPeriod == 1 && m_GOPList[0].m_POC == -1) {
#endif
    m_GOPList[0] = GOPEntry();
    m_GOPList[0].m_QPFactor = 1;
    m_GOPList[0].m_betaOffsetDiv2 = 0;
    m_GOPList[0].m_tcOffsetDiv2 = 0;
    m_GOPList[0].m_POC = 1;
    m_GOPList[0].m_numRefPicsActive = 4;
  }
  
  Bool verifiedGOP=false;
  Bool errorGOP=false;
  Int checkGOP=1;
  Int numRefs = m_isField ? 2 : 1;
  Int refList[MAX_NUM_REF_PICS+1];
  refList[0]=0;
  if(m_isField)
  {
    refList[1] = 1;
  }
  Bool isOK[MAX_GOP];
  for(Int i=0; i<MAX_GOP; i++) 
  {
    isOK[i]=false;
  }
  Int numOK=0;
#if H_MV
  xConfirmPara( m_iIntraPeriod[layer] >=0&&(m_iIntraPeriod[layer]%m_iGOPSize!=0), "Intra period must be a multiple of GOPSize, or -1" ); 
#else
xConfirmPara( m_iIntraPeriod >=0&&(m_iIntraPeriod%m_iGOPSize!=0), "Intra period must be a multiple of GOPSize, or -1" );
#endif

  for(Int i=0; i<m_iGOPSize; i++)
  {
    if(m_GOPList[i].m_POC==m_iGOPSize)
    {
      xConfirmPara( m_GOPList[i].m_temporalId!=0 , "The last frame in each GOP must have temporal ID = 0 " );
    }
  }
  
#if H_MV
  if ( (m_iIntraPeriod[layer] != 1) && !m_loopFilterOffsetInPPS && m_DeblockingFilterControlPresent && (!m_bLoopFilterDisable[layer]) )
#else
  if ( (m_iIntraPeriod != 1) && !m_loopFilterOffsetInPPS && m_DeblockingFilterControlPresent && (!m_bLoopFilterDisable) )
#endif
  {
    for(Int i=0; i<m_iGOPSize; i++)
    {
      xConfirmPara( (m_GOPList[i].m_betaOffsetDiv2 + m_loopFilterBetaOffsetDiv2) < -6 || (m_GOPList[i].m_betaOffsetDiv2 + m_loopFilterBetaOffsetDiv2) > 6, "Loop Filter Beta Offset div. 2 for one of the GOP entries exceeds supported range (-6 to 6)" );
      xConfirmPara( (m_GOPList[i].m_tcOffsetDiv2 + m_loopFilterTcOffsetDiv2) < -6 || (m_GOPList[i].m_tcOffsetDiv2 + m_loopFilterTcOffsetDiv2) > 6, "Loop Filter Tc Offset div. 2 for one of the GOP entries exceeds supported range (-6 to 6)" );
    }
  }
  m_extraRPSs=0;
  //start looping through frames in coding order until we can verify that the GOP structure is correct.
  while(!verifiedGOP&&!errorGOP) 
  {
    Int curGOP = (checkGOP-1)%m_iGOPSize;
    Int curPOC = ((checkGOP-1)/m_iGOPSize)*m_iGOPSize + m_GOPList[curGOP].m_POC;    
    if(m_GOPList[curGOP].m_POC<0) 
    {
#if H_MV
      printf("\nError: found fewer Reference Picture Sets than GOPSize for layer %d\n", layer );
#else
      printf("\nError: found fewer Reference Picture Sets than GOPSize\n");
#endif
      errorGOP=true;
    }
    else 
    {
      //check that all reference pictures are available, or have a POC < 0 meaning they might be available in the next GOP.
      Bool beforeI = false;
      for(Int i = 0; i< m_GOPList[curGOP].m_numRefPics; i++) 
      {
        Int absPOC = curPOC+m_GOPList[curGOP].m_referencePics[i];
        if(absPOC < 0)
        {
          beforeI=true;
        }
        else 
        {
          Bool found=false;
          for(Int j=0; j<numRefs; j++) 
          {
            if(refList[j]==absPOC) 
            {
              found=true;
              for(Int k=0; k<m_iGOPSize; k++)
              {
                if(absPOC%m_iGOPSize == m_GOPList[k].m_POC%m_iGOPSize)
                {
                  if(m_GOPList[k].m_temporalId==m_GOPList[curGOP].m_temporalId)
                  {
                    m_GOPList[k].m_refPic = true;
                  }
                  m_GOPList[curGOP].m_usedByCurrPic[i]=m_GOPList[k].m_temporalId<=m_GOPList[curGOP].m_temporalId;
                }
              }
            }
          }
          if(!found)
          {
#if H_MV
            printf("\nError: ref pic %d is not available for GOP frame %d of layer %d\n", m_GOPList[curGOP].m_referencePics[i], curGOP+1, layer);
#else
            printf("\nError: ref pic %d is not available for GOP frame %d\n",m_GOPList[curGOP].m_referencePics[i],curGOP+1);
#endif
            errorGOP=true;
          }
        }
      }
      if(!beforeI&&!errorGOP)
      {
        //all ref frames were present
        if(!isOK[curGOP]) 
        {
          numOK++;
          isOK[curGOP]=true;
          if(numOK==m_iGOPSize)
          {
            verifiedGOP=true;
          }
        }
      }
      else 
      {
        //create a new GOPEntry for this frame containing all the reference pictures that were available (POC > 0)
        m_GOPList[m_iGOPSize+m_extraRPSs]=m_GOPList[curGOP];
        Int newRefs=0;
        for(Int i = 0; i< m_GOPList[curGOP].m_numRefPics; i++) 
        {
          Int absPOC = curPOC+m_GOPList[curGOP].m_referencePics[i];
          if(absPOC>=0)
          {
            m_GOPList[m_iGOPSize+m_extraRPSs].m_referencePics[newRefs]=m_GOPList[curGOP].m_referencePics[i];
            m_GOPList[m_iGOPSize+m_extraRPSs].m_usedByCurrPic[newRefs]=m_GOPList[curGOP].m_usedByCurrPic[i];
            newRefs++;
          }
        }
        Int numPrefRefs = m_GOPList[curGOP].m_numRefPicsActive;
        
        for(Int offset = -1; offset>-checkGOP; offset--)
        {
          //step backwards in coding order and include any extra available pictures we might find useful to replace the ones with POC < 0.
          Int offGOP = (checkGOP-1+offset)%m_iGOPSize;
          Int offPOC = ((checkGOP-1+offset)/m_iGOPSize)*m_iGOPSize + m_GOPList[offGOP].m_POC;
          if(offPOC>=0&&m_GOPList[offGOP].m_temporalId<=m_GOPList[curGOP].m_temporalId)
          {
            Bool newRef=false;
            for(Int i=0; i<numRefs; i++)
            {
              if(refList[i]==offPOC)
              {
                newRef=true;
              }
            }
            for(Int i=0; i<newRefs; i++) 
            {
              if(m_GOPList[m_iGOPSize+m_extraRPSs].m_referencePics[i]==offPOC-curPOC)
              {
                newRef=false;
              }
            }
            if(newRef) 
            {
              Int insertPoint=newRefs;
              //this picture can be added, find appropriate place in list and insert it.
              if(m_GOPList[offGOP].m_temporalId==m_GOPList[curGOP].m_temporalId)
              {
                m_GOPList[offGOP].m_refPic = true;
              }
              for(Int j=0; j<newRefs; j++)
              {
                if(m_GOPList[m_iGOPSize+m_extraRPSs].m_referencePics[j]<offPOC-curPOC||m_GOPList[m_iGOPSize+m_extraRPSs].m_referencePics[j]>0)
                {
                  insertPoint = j;
                  break;
                }
              }
              Int prev = offPOC-curPOC;
              Int prevUsed = m_GOPList[offGOP].m_temporalId<=m_GOPList[curGOP].m_temporalId;
              for(Int j=insertPoint; j<newRefs+1; j++)
              {
                Int newPrev = m_GOPList[m_iGOPSize+m_extraRPSs].m_referencePics[j];
                Int newUsed = m_GOPList[m_iGOPSize+m_extraRPSs].m_usedByCurrPic[j];
                m_GOPList[m_iGOPSize+m_extraRPSs].m_referencePics[j]=prev;
                m_GOPList[m_iGOPSize+m_extraRPSs].m_usedByCurrPic[j]=prevUsed;
                prevUsed=newUsed;
                prev=newPrev;
              }
              newRefs++;
            }
          }
          if(newRefs>=numPrefRefs)
          {
            break;
          }
        }
        m_GOPList[m_iGOPSize+m_extraRPSs].m_numRefPics=newRefs;
        m_GOPList[m_iGOPSize+m_extraRPSs].m_POC = curPOC;
        if (m_extraRPSs == 0)
        {
          m_GOPList[m_iGOPSize+m_extraRPSs].m_interRPSPrediction = 0;
          m_GOPList[m_iGOPSize+m_extraRPSs].m_numRefIdc = 0;
        }
        else
        {
          Int rIdx =  m_iGOPSize + m_extraRPSs - 1;
          Int refPOC = m_GOPList[rIdx].m_POC;
          Int refPics = m_GOPList[rIdx].m_numRefPics;
          Int newIdc=0;
          for(Int i = 0; i<= refPics; i++) 
          {
            Int deltaPOC = ((i != refPics)? m_GOPList[rIdx].m_referencePics[i] : 0);  // check if the reference abs POC is >= 0
            Int absPOCref = refPOC+deltaPOC;
            Int refIdc = 0;
            for (Int j = 0; j < m_GOPList[m_iGOPSize+m_extraRPSs].m_numRefPics; j++)
            {
              if ( (absPOCref - curPOC) == m_GOPList[m_iGOPSize+m_extraRPSs].m_referencePics[j])
              {
                if (m_GOPList[m_iGOPSize+m_extraRPSs].m_usedByCurrPic[j])
                {
                  refIdc = 1;
                }
                else
                {
                  refIdc = 2;
                }
              }
            }
            m_GOPList[m_iGOPSize+m_extraRPSs].m_refIdc[newIdc]=refIdc;
            newIdc++;
          }
          m_GOPList[m_iGOPSize+m_extraRPSs].m_interRPSPrediction = 1;  
          m_GOPList[m_iGOPSize+m_extraRPSs].m_numRefIdc = newIdc;
          m_GOPList[m_iGOPSize+m_extraRPSs].m_deltaRPS = refPOC - m_GOPList[m_iGOPSize+m_extraRPSs].m_POC; 
        }
        curGOP=m_iGOPSize+m_extraRPSs;
        m_extraRPSs++;
      }
      numRefs=0;
      for(Int i = 0; i< m_GOPList[curGOP].m_numRefPics; i++) 
      {
        Int absPOC = curPOC+m_GOPList[curGOP].m_referencePics[i];
        if(absPOC >= 0) 
        {
          refList[numRefs]=absPOC;
          numRefs++;
        }
      }
      refList[numRefs]=curPOC;
      numRefs++;
    }
    checkGOP++;
  }
  xConfirmPara(errorGOP,"Invalid GOP structure given");
  m_maxTempLayer = 1;
  for(Int i=0; i<m_iGOPSize; i++) 
  {
    if(m_GOPList[i].m_temporalId >= m_maxTempLayer)
    {
      m_maxTempLayer = m_GOPList[i].m_temporalId+1;
    }
    xConfirmPara(m_GOPList[i].m_sliceType!='B'&&m_GOPList[i].m_sliceType!='P'&&m_GOPList[i].m_sliceType!='I', "Slice type must be equal to B or P or I");
  }
  for(Int i=0; i<MAX_TLAYER; i++)
  {
    m_numReorderPics[i] = 0;
    m_maxDecPicBuffering[i] = 1;
  }
  for(Int i=0; i<m_iGOPSize; i++) 
  {
    if(m_GOPList[i].m_numRefPics+1 > m_maxDecPicBuffering[m_GOPList[i].m_temporalId])
    {
      m_maxDecPicBuffering[m_GOPList[i].m_temporalId] = m_GOPList[i].m_numRefPics + 1;
    }
    Int highestDecodingNumberWithLowerPOC = 0; 
    for(Int j=0; j<m_iGOPSize; j++)
    {
      if(m_GOPList[j].m_POC <= m_GOPList[i].m_POC)
      {
        highestDecodingNumberWithLowerPOC = j;
      }
    }
    Int numReorder = 0;
    for(Int j=0; j<highestDecodingNumberWithLowerPOC; j++)
    {
      if(m_GOPList[j].m_temporalId <= m_GOPList[i].m_temporalId && 
        m_GOPList[j].m_POC > m_GOPList[i].m_POC)
      {
        numReorder++;
      }
    }    
    if(numReorder > m_numReorderPics[m_GOPList[i].m_temporalId])
    {
      m_numReorderPics[m_GOPList[i].m_temporalId] = numReorder;
    }
  }
  for(Int i=0; i<MAX_TLAYER-1; i++) 
  {
    // a lower layer can not have higher value of m_numReorderPics than a higher layer
    if(m_numReorderPics[i+1] < m_numReorderPics[i])
    {
      m_numReorderPics[i+1] = m_numReorderPics[i];
    }
    // the value of num_reorder_pics[ i ] shall be in the range of 0 to max_dec_pic_buffering[ i ] - 1, inclusive
    if(m_numReorderPics[i] > m_maxDecPicBuffering[i] - 1)
    {
      m_maxDecPicBuffering[i] = m_numReorderPics[i] + 1;
    }
    // a lower layer can not have higher value of m_uiMaxDecPicBuffering than a higher layer
    if(m_maxDecPicBuffering[i+1] < m_maxDecPicBuffering[i])
    {
      m_maxDecPicBuffering[i+1] = m_maxDecPicBuffering[i];
    }
  }


  // the value of num_reorder_pics[ i ] shall be in the range of 0 to max_dec_pic_buffering[ i ] -  1, inclusive
  if(m_numReorderPics[MAX_TLAYER-1] > m_maxDecPicBuffering[MAX_TLAYER-1] - 1)
  {
    m_maxDecPicBuffering[MAX_TLAYER-1] = m_numReorderPics[MAX_TLAYER-1] + 1;
  }

  if(m_vuiParametersPresentFlag && m_bitstreamRestrictionFlag)
  { 
    Int PicSizeInSamplesY =  m_iSourceWidth * m_iSourceHeight;
    if(tileFlag)
    {
      Int maxTileWidth = 0;
      Int maxTileHeight = 0;
      Int widthInCU = (m_iSourceWidth % m_uiMaxCUWidth) ? m_iSourceWidth/m_uiMaxCUWidth + 1: m_iSourceWidth/m_uiMaxCUWidth;
      Int heightInCU = (m_iSourceHeight % m_uiMaxCUHeight) ? m_iSourceHeight/m_uiMaxCUHeight + 1: m_iSourceHeight/m_uiMaxCUHeight;
      if(m_iUniformSpacingIdr)
      {
        maxTileWidth = m_uiMaxCUWidth*((widthInCU+m_iNumColumnsMinus1)/(m_iNumColumnsMinus1+1));
        maxTileHeight = m_uiMaxCUHeight*((heightInCU+m_iNumRowsMinus1)/(m_iNumRowsMinus1+1));
        // if only the last tile-row is one treeblock higher than the others 
        // the maxTileHeight becomes smaller if the last row of treeblocks has lower height than the others
        if(!((heightInCU-1)%(m_iNumRowsMinus1+1)))
        {
          maxTileHeight = maxTileHeight - m_uiMaxCUHeight + (m_iSourceHeight % m_uiMaxCUHeight);
        }     
        // if only the last tile-column is one treeblock wider than the others 
        // the maxTileWidth becomes smaller if the last column of treeblocks has lower width than the others   
        if(!((widthInCU-1)%(m_iNumColumnsMinus1+1)))
        {
          maxTileWidth = maxTileWidth - m_uiMaxCUWidth + (m_iSourceWidth % m_uiMaxCUWidth);
        }
      }
      else // not uniform spacing
      {
        if(m_iNumColumnsMinus1<1)
        {
          maxTileWidth = m_iSourceWidth;
        }
        else
        {
          Int accColumnWidth = 0;
          for(Int col=0; col<(m_iNumColumnsMinus1); col++)
          {
            maxTileWidth = m_pColumnWidth[col]>maxTileWidth ? m_pColumnWidth[col]:maxTileWidth;
            accColumnWidth += m_pColumnWidth[col];
          }
          maxTileWidth = (widthInCU-accColumnWidth)>maxTileWidth ? m_uiMaxCUWidth*(widthInCU-accColumnWidth):m_uiMaxCUWidth*maxTileWidth;
        }
        if(m_iNumRowsMinus1<1)
        {
          maxTileHeight = m_iSourceHeight;
        }
        else
        {
          Int accRowHeight = 0;
          for(Int row=0; row<(m_iNumRowsMinus1); row++)
          {
            maxTileHeight = m_pRowHeight[row]>maxTileHeight ? m_pRowHeight[row]:maxTileHeight;
            accRowHeight += m_pRowHeight[row];
          }
          maxTileHeight = (heightInCU-accRowHeight)>maxTileHeight ? m_uiMaxCUHeight*(heightInCU-accRowHeight):m_uiMaxCUHeight*maxTileHeight;
        }
      }
      Int maxSizeInSamplesY = maxTileWidth*maxTileHeight;
      m_minSpatialSegmentationIdc = 4*PicSizeInSamplesY/maxSizeInSamplesY-4;
    }
    else if(m_iWaveFrontSynchro)
    {
      m_minSpatialSegmentationIdc = 4*PicSizeInSamplesY/((2*m_iSourceHeight+m_iSourceWidth)*m_uiMaxCUHeight)-4;
    }
    else if(m_sliceMode == 1)
    {
      m_minSpatialSegmentationIdc = 4*PicSizeInSamplesY/(m_sliceArgument*m_uiMaxCUWidth*m_uiMaxCUHeight)-4;
    }
    else
    {
      m_minSpatialSegmentationIdc = 0;
    }
  }
  xConfirmPara( m_iWaveFrontSynchro < 0, "WaveFrontSynchro cannot be negative" );
  xConfirmPara( m_iWaveFrontSubstreams <= 0, "WaveFrontSubstreams must be positive" );
  xConfirmPara( m_iWaveFrontSubstreams > 1 && !m_iWaveFrontSynchro, "Must have WaveFrontSynchro > 0 in order to have WaveFrontSubstreams > 1" );

  xConfirmPara( m_decodedPictureHashSEIEnabled<0 || m_decodedPictureHashSEIEnabled>3, "this hash type is not correct!\n");

  if (m_toneMappingInfoSEIEnabled)
  {
    xConfirmPara( m_toneMapCodedDataBitDepth < 8 || m_toneMapCodedDataBitDepth > 14 , "SEIToneMapCodedDataBitDepth must be in rage 8 to 14");
    xConfirmPara( m_toneMapTargetBitDepth < 1 || (m_toneMapTargetBitDepth > 16 && m_toneMapTargetBitDepth < 255) , "SEIToneMapTargetBitDepth must be in rage 1 to 16 or equal to 255");
    xConfirmPara( m_toneMapModelId < 0 || m_toneMapModelId > 4 , "SEIToneMapModelId must be in rage 0 to 4");
    xConfirmPara( m_cameraIsoSpeedValue == 0, "SEIToneMapCameraIsoSpeedValue shall not be equal to 0");
    xConfirmPara( m_extendedRangeWhiteLevel < 100, "SEIToneMapExtendedRangeWhiteLevel should be greater than or equal to 100");
    xConfirmPara( m_nominalBlackLevelLumaCodeValue >= m_nominalWhiteLevelLumaCodeValue, "SEIToneMapNominalWhiteLevelLumaCodeValue shall be greater than SEIToneMapNominalBlackLevelLumaCodeValue");
    xConfirmPara( m_extendedWhiteLevelLumaCodeValue < m_nominalWhiteLevelLumaCodeValue, "SEIToneMapExtendedWhiteLevelLumaCodeValue shall be greater than or equal to SEIToneMapNominalWhiteLevelLumaCodeValue");
  }

  if ( m_RCEnableRateControl )
  {
    if ( m_RCForceIntraQP )
    {
      if ( m_RCInitialQP == 0 )
      {
        printf( "\nInitial QP for rate control is not specified. Reset not to use force intra QP!" );
        m_RCForceIntraQP = false;
      }
    }
    xConfirmPara( m_uiDeltaQpRD > 0, "Rate control cannot be used together with slice level multiple-QP optimization!\n" );
  }
#if H_MV
  // VPS VUI
  for(Int i = 0; i < MAX_VPS_OP_SETS_PLUS1; i++ )
  { 
    for (Int j = 0; j < MAX_TLAYER; j++)
    {    
      if ( j < m_avgBitRate        [i].size() ) xConfirmPara( m_avgBitRate[i][j]         <  0 || m_avgBitRate[i][j]         > 65535, "avg_bit_rate            must be more than or equal to     0 and less than 65536" );
      if ( j < m_maxBitRate        [i].size() ) xConfirmPara( m_maxBitRate[i][j]         <  0 || m_maxBitRate[i][j]         > 65535, "max_bit_rate            must be more than or equal to     0 and less than 65536" );
      if ( j < m_constantPicRateIdc[i].size() ) xConfirmPara( m_constantPicRateIdc[i][j] <  0 || m_constantPicRateIdc[i][j] >     3, "constant_pic_rate_idc   must be more than or equal to     0 and less than     4" );
      if ( j < m_avgPicRate        [i].size() ) xConfirmPara( m_avgPicRate[i][j]         <  0 || m_avgPicRate[i][j]         > 65535, "avg_pic_rate            must be more than or equal to     0 and less than 65536" );
    }
  }
  // todo: replace value of 100 with requirement in spec
  for(Int i = 0; i < MAX_NUM_LAYERS; i++ )
  { 
    for (Int j = 0; j < MAX_NUM_LAYERS; j++)
    {    
      if ( j < m_minSpatialSegmentOffsetPlus1[i].size() ) xConfirmPara( m_minSpatialSegmentOffsetPlus1[i][j] < 0 || m_minSpatialSegmentOffsetPlus1[i][j] >   100, "min_spatial_segment_offset_plus1 must be more than or equal to     0 and less than   101" );
      if ( j < m_minHorizontalCtuOffsetPlus1[i] .size() ) xConfirmPara( m_minHorizontalCtuOffsetPlus1[i][j]  < 0 || m_minHorizontalCtuOffsetPlus1[i][j]  >   100, "min_horizontal_ctu_offset_plus1  must be more than or equal to     0 and less than   101" );
    }
  }
#endif

  xConfirmPara(!m_TransquantBypassEnableFlag && m_CUTransquantBypassFlagForce, "CUTransquantBypassFlagForce cannot be 1 when TransquantBypassEnableFlag is 0");

  xConfirmPara(m_log2ParallelMergeLevel < 2, "Log2ParallelMergeLevel should be larger than or equal to 2");
  if (m_framePackingSEIEnabled)
  {
    xConfirmPara(m_framePackingSEIType < 3 || m_framePackingSEIType > 5 , "SEIFramePackingType must be in rage 3 to 5");
  }

#if H_MV
  }
  }
  // Check input parameters for Sub-bitstream property SEI message
  if( m_subBistreamPropSEIEnabled )
  {
    xConfirmPara( 
      (this->m_sbPropNumAdditionalSubStreams != m_sbPropAvgBitRate.size() )
      || (this->m_sbPropNumAdditionalSubStreams != m_sbPropHighestSublayerId.size() )
      || (this->m_sbPropNumAdditionalSubStreams != m_sbPropMaxBitRate.size() )
      || (this->m_sbPropNumAdditionalSubStreams != m_sbPropOutputLayerSetIdxToVps.size() )
      || (this->m_sbPropNumAdditionalSubStreams != m_sbPropSubBitstreamMode.size()), "Some parameters of some sub-bitstream not defined");

    for( Int i = 0; i < m_sbPropNumAdditionalSubStreams; i++ )
    {
      xConfirmPara( m_sbPropSubBitstreamMode[i] < 0 || m_sbPropSubBitstreamMode[i] > 1, "Mode value should be 0 or 1" );
      xConfirmPara( m_sbPropHighestSublayerId[i] < 0 || m_sbPropHighestSublayerId[i] > MAX_TLAYER-1, "Maximum sub-layer ID out of range" );
      xConfirmPara( m_sbPropOutputLayerSetIdxToVps[i] < 0 || m_sbPropOutputLayerSetIdxToVps[i] >= MAX_VPS_OUTPUTLAYER_SETS, "OutputLayerSetIdxToVps should be within allowed range" );
    }
  }
#endif
#undef xConfirmPara
  if (check_failed)
  {
    exit(EXIT_FAILURE);
  }
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
  g_bitDepthY = m_internalBitDepthY;
  g_bitDepthC = m_internalBitDepthC;
  
  g_uiPCMBitDepthLuma = m_bPCMInputBitDepthFlag ? m_inputBitDepthY : m_internalBitDepthY;
  g_uiPCMBitDepthChroma = m_bPCMInputBitDepthFlag ? m_inputBitDepthC : m_internalBitDepthC;
}

Void TAppEncCfg::xPrintParameter()
{
  printf("\n");
#if H_MV
  for( Int layer = 0; layer < m_numberOfLayers; layer++)
  {
    printf("Input File %i                 : %s\n", layer, m_pchInputFileList[layer]);
  }
#else
  printf("Input          File          : %s\n", m_pchInputFile          );
#endif
  printf("Bitstream      File          : %s\n", m_pchBitstreamFile      );
#if H_MV
  for( Int layer = 0; layer < m_numberOfLayers; layer++)
  {
    printf("Reconstruction File %i        : %s\n", layer, m_pchReconFileList[layer]);
  }
#else
  printf("Reconstruction File          : %s\n", m_pchReconFile          );
#endif
#if H_MV
  xPrintParaVector( "ViewIdVal"     , m_viewId ); 
  xPrintParaVector( "ViewOrderIndex", m_viewOrderIndex ); 
#endif
#if H_3D
  xPrintParaVector( "DepthFlag", m_depthFlag ); 
  printf("Coded Camera Param. Precision: %d\n", m_iCodedCamParPrecision);
#endif
#if H_MV  
  xPrintParaVector( "QP"               , m_fQP                ); 
  xPrintParaVector( "LoopFilterDisable", m_bLoopFilterDisable ); 
  xPrintParaVector( "SAO"              , m_bUseSAO            ); 
#endif
  printf("Real     Format              : %dx%d %dHz\n", m_iSourceWidth - m_confLeft - m_confRight, m_iSourceHeight - m_confTop - m_confBottom, m_iFrameRate );
  printf("Internal Format              : %dx%d %dHz\n", m_iSourceWidth, m_iSourceHeight, m_iFrameRate );
  if (m_isField)
  {
    printf("Frame/Field          : Field based coding\n");
    printf("Field index          : %u - %d (%d fields)\n", m_FrameSkip, m_FrameSkip+m_framesToBeEncoded-1, m_framesToBeEncoded );
    if (m_isTopFieldFirst)
    {
      printf("Field Order            : Top field first\n");
    }
    else
    {
      printf("Field Order            : Bottom field first\n");
    }
  }
  else
  {
    printf("Frame/Field                  : Frame based coding\n");
  printf("Frame index                  : %u - %d (%d frames)\n", m_FrameSkip, m_FrameSkip+m_framesToBeEncoded-1, m_framesToBeEncoded );
  }
  printf("CU size / depth              : %d / %d\n", m_uiMaxCUWidth, m_uiMaxCUDepth );
  printf("RQT trans. size (min / max)  : %d / %d\n", 1 << m_uiQuadtreeTULog2MinSize, 1 << m_uiQuadtreeTULog2MaxSize );
  printf("Max RQT depth inter          : %d\n", m_uiQuadtreeTUMaxDepthInter);
  printf("Max RQT depth intra          : %d\n", m_uiQuadtreeTUMaxDepthIntra);
  printf("Min PCM size                 : %d\n", 1 << m_uiPCMLog2MinSize);
  printf("Motion search range          : %d\n", m_iSearchRange );
#if H_MV
  xPrintParaVector( "Intra period", m_iIntraPeriod );
#else
  printf("Intra period                 : %d\n", m_iIntraPeriod );
#endif
  printf("Decoding refresh type        : %d\n", m_iDecodingRefreshType );
#if !H_MV
  printf("QP                           : %5.2f\n", m_fQP );
#endif
  printf("Max dQP signaling depth      : %d\n", m_iMaxCuDQPDepth);

  printf("Cb QP Offset                 : %d\n", m_cbQpOffset   );
  printf("Cr QP Offset                 : %d\n", m_crQpOffset);

  printf("QP adaptation                : %d (range=%d)\n", m_bUseAdaptiveQP, (m_bUseAdaptiveQP ? m_iQPAdaptationRange : 0) );
  printf("GOP size                     : %d\n", m_iGOPSize );
  printf("Internal bit depth           : (Y:%d, C:%d)\n", m_internalBitDepthY, m_internalBitDepthC );
  printf("PCM sample bit depth         : (Y:%d, C:%d)\n", g_uiPCMBitDepthLuma, g_uiPCMBitDepthChroma );
  printf("RateControl                  : %d\n", m_RCEnableRateControl );
  if(m_RCEnableRateControl)
  {
    printf("TargetBitrate                : %d\n", m_RCTargetBitrate );
    printf("KeepHierarchicalBit          : %d\n", m_RCKeepHierarchicalBit );
    printf("LCULevelRC                   : %d\n", m_RCLCULevelRC );
    printf("UseLCUSeparateModel          : %d\n", m_RCUseLCUSeparateModel );
    printf("InitialQP                    : %d\n", m_RCInitialQP );
    printf("ForceIntraQP                 : %d\n", m_RCForceIntraQP );

#if KWU_RC_MADPRED_E0227
    printf("Depth based MAD prediction   : %d\n", m_depthMADPred);
#endif
#if KWU_RC_VIEWRC_E0227
    printf("View-wise Rate control       : %d\n", m_viewWiseRateCtrl);
    if(m_viewWiseRateCtrl)
    {

      printf("ViewWiseTargetBits           : ");
      for (Int i = 0 ; i < m_iNumberOfViews ; i++)
        printf("%d ", m_viewTargetBits[i]);
      printf("\n");
    }
    else
    {
      printf("TargetBitrate                : %d\n", m_RCTargetBitrate );
    }
#endif
  }
  printf("Max Num Merge Candidates     : %d\n", m_maxNumMergeCand);
#if H_3D
  printf("BaseViewCameraNumbers        : %s\n", m_pchBaseViewCameraNumbers ); 
  printf("Coded Camera Param. Precision: %d\n", m_iCodedCamParPrecision);
#if H_3D_VSO
  printf("Force use of Lambda Scale    : %d\n", m_bForceLambdaScaleVSO );

  if ( m_bUseVSO )
  {    
    printf("VSO Lambda Scale             : %5.2f\n", m_dLambdaScaleVSO );
    printf("VSO Mode                     : %d\n",    m_uiVSOMode       );
    printf("VSO Config                   : %s\n",    m_pchVSOConfig    );
    printf("VSO Negative Distortion      : %d\n",    m_bAllowNegDist ? 1 : 0);
    printf("VSO LS Table                 : %d\n",    m_bVSOLSTable ? 1 : 0);
    printf("VSO Estimated VSD            : %d\n",    m_bUseEstimatedVSD ? 1 : 0);
    printf("VSO Early Skip               : %d\n",    m_bVSOEarlySkip ? 1 : 0);   
    if ( m_bUseWVSO )
    printf("Dist. Weights (VSO/VSD/SAD)  : %d/%d/%d\n ", m_iVSOWeight, m_iVSDWeight, m_iDWeight );
  }
#endif //HHI_VSO
#endif //H_3D
  printf("\n");
#if H_MV
  printf("TOOL CFG General: ");
#else
  printf("TOOL CFG: ");
#endif
  printf("IBD:%d ", g_bitDepthY > m_inputBitDepthY || g_bitDepthC > m_inputBitDepthC);
  printf("HAD:%d ", m_bUseHADME           );
  printf("RDQ:%d ", m_useRDOQ            );
  printf("RDQTS:%d ", m_useRDOQTS        );
  printf("RDpenalty:%d ", m_rdPenalty  );
  printf("SQP:%d ", m_uiDeltaQpRD         );
  printf("ASR:%d ", m_bUseASR             );
  printf("FEN:%d ", m_bUseFastEnc         );
  printf("ECU:%d ", m_bUseEarlyCU         );
  printf("FDM:%d ", m_useFastDecisionForMerge );
  printf("CFM:%d ", m_bUseCbfFastMode         );
  printf("ESD:%d ", m_useEarlySkipDetection  );
  printf("RQT:%d ", 1     );
  printf("TransformSkip:%d ",     m_useTransformSkip              );
  printf("TransformSkipFast:%d ", m_useTransformSkipFast       );
  printf("Slice: M=%d ", m_sliceMode);
  if (m_sliceMode!=0)
  {
    printf("A=%d ", m_sliceArgument);
  }
  printf("SliceSegment: M=%d ",m_sliceSegmentMode);
  if (m_sliceSegmentMode!=0)
  {
    printf("A=%d ", m_sliceSegmentArgument);
  }
  printf("CIP:%d ", m_bUseConstrainedIntraPred);
#if !H_MV
  printf("SAO:%d ", (m_bUseSAO)?(1):(0));
#endif
  printf("PCM:%d ", (m_usePCM && (1<<m_uiPCMLog2MinSize) <= m_uiMaxCUWidth)? 1 : 0);
  if (m_TransquantBypassEnableFlag && m_CUTransquantBypassFlagForce)
  {
    printf("TransQuantBypassEnabled: =1 ");
  }
  else
  {
    printf("TransQuantBypassEnabled:%d ", (m_TransquantBypassEnableFlag)? 1:0 );
  }
  printf("WPP:%d ", (Int)m_useWeightedPred);
  printf("WPB:%d ", (Int)m_useWeightedBiPred);
  printf("PME:%d ", m_log2ParallelMergeLevel);
  printf(" WaveFrontSynchro:%d WaveFrontSubstreams:%d",
          m_iWaveFrontSynchro, m_iWaveFrontSubstreams);
  printf(" ScalingList:%d ", m_useScalingListId );
  printf("TMVPMode:%d ", m_TMVPModeId     );
#if ADAPTIVE_QP_SELECTION
  printf("AQpS:%d ", m_bUseAdaptQpSelect   );
#endif

  printf(" SignBitHidingFlag:%d ", m_signHideFlag);
  printf("RecalQP:%d ", m_recalculateQPAccordingToLambda ? 1 : 0 );
#if H_3D_VSO
  printf("VSO:%d ", m_bUseVSO   );
  printf("WVSO:%d ", m_bUseWVSO );  
#endif
#if H_3D_QTLPC
  printf("QTL:%d ", m_bUseQTL);
  printf("PC:%d " , m_bUsePC );
#endif
#if H_3D_IV_MERGE
  printf("IvMvPred:%d %d", m_ivMvPredFlag[0] ? 1 : 0, m_ivMvPredFlag[1] ? 1 : 0);
#if H_3D_SPIVMP
  printf(" SubPULog2Size:%d  " , m_iSubPULog2Size  );
  printf(" SubPUMPILog2Size:%d  " , m_iSubPUMPILog2Size  );
#endif
#endif
#if H_3D_ARP
  printf(" ARP:%d  ", m_uiUseAdvResPred  );
#endif
#if H_3D_IC
  printf( "IlluCompEnable:%d ", m_abUseIC);
#endif
#if H_3D_NBDV_REF
  printf("DepthRefinement:%d ", m_depthRefinementFlag );  
#endif
#if H_3D_VSP
  printf("ViewSynthesisPred:%d ", m_viewSynthesisPredFlag );
#endif
#if H_3D
  printf("IvMvScaling:%d ", m_ivMvScalingFlag ? 1 : 0  );
#endif
#if H_3D_DIM
  printf("DMM:%d ", m_useDMM );
  printf("SDC:%d ", m_useSDC );
  printf("DLT:%d ", m_useDLT );
#endif
#if H_3D_INTER_SDC
  printf( "interSDC:%d ", m_bDepthInterSDCFlag ? 1 : 0 );
#endif
#if H_3D_DBBP
  printf("DBBP:%d ", m_bUseDBBP ? 1 : 0);
#endif
#if H_3D_IV_MERGE
  printf( "MPI:%d ", m_bMPIFlag ? 1 : 0 );
#endif
  printf("\n\n");  

  fflush(stdout);
}

Bool confirmPara(Bool bflag, const Char* message)
{
  if (!bflag)
    return false;
  
  printf("Error: %s\n",message);
  return true;
}

//! \}
