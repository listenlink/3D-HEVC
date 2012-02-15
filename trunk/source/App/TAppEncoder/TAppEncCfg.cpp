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



/** \file     TAppEncCfg.cpp
    \brief    Handle encoder configuration parameters
*/

#include <stdlib.h>
#include <math.h>
#include <cassert>
#include <cstring>
#include <string>
#include "TAppEncCfg.h"
#include "../../App/TAppCommon/program_options_lite.h"


using namespace std;
namespace po = df::program_options_lite;

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
#define MAX_INPUT_VIEW_NUM          10
#define MAX_ERREF_VIEW_NUM          15

// ====================================================================================================================
// Constructor / destructor / initialization / destroy
// ====================================================================================================================

TAppEncCfg::TAppEncCfg()
{
  m_aidQP = NULL;
#if HHI_VSO
  m_aaiERViewRefLutInd.resize( MAX_INPUT_VIEW_NUM );
#endif
}

TAppEncCfg::~TAppEncCfg()
{
  if ( m_aidQP )
  {
    delete[] m_aidQP; m_aidQP = NULL;
  }
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

  for(Int i = 0; i< m_pchERRefFileList.size(); i++ )
  {
    if ( m_pchERRefFileList[i] != NULL )
      free (m_pchERRefFileList[i]);
  }

  if (m_pchBitstreamFile != NULL)
    free (m_pchBitstreamFile) ;

  for(Int i = 0; i< m_pchDepthReconFileList.size(); i++ )
  {
    if ( m_pchDepthReconFileList[i] != NULL )
      free (m_pchDepthReconFileList[i]);
  }

  if (m_pchCameraParameterFile != NULL)
    free (m_pchCameraParameterFile);

  if (m_pchBaseViewCameraNumbers != NULL)
    free (m_pchBaseViewCameraNumbers);

#if HHI_VSO
  if (  m_pchVSOConfig != NULL)
    free (  m_pchVSOConfig );
#endif

}

Void TAppEncCfg::create()
{
}

Void TAppEncCfg::destroy()
{
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

  string cfg_InputFile;
  string cfg_BitstreamFile;
  string cfg_ReconFile;
  string cfg_dQPFile;
  po::Options opts;
  opts.addOptions()
  ("help", do_help, false, "this help text")
  ("c", po::parseConfigFile, "configuration file name")

  /* File, I/O and source parameters */
  ("InputFile_%d,i_%d",       m_pchInputFileList,       (char *) 0 , MAX_INPUT_VIEW_NUM , "original Yuv input file name %d")
  ("DepthInputFile_%d,di_%d", m_pchDepthInputFileList,  (char *) 0 , MAX_INPUT_VIEW_NUM , "original Yuv depth input file name %d")
  ("ReconFile_%d,o_%d",       m_pchReconFileList,       (char *) 0 , MAX_INPUT_VIEW_NUM , "reconstructed Yuv output file name %d")
  ("DepthReconFile_%d,do_%d", m_pchDepthReconFileList,  (char *) 0 , MAX_INPUT_VIEW_NUM , "reconstructed Yuv depth output file name %d")
  ("BitstreamFile,b", cfg_BitstreamFile, string(""), "bitstream output file name")

  ("CodeDepthMaps",         m_bUsingDepthMaps, false, "Encode depth maps" )
  ("CodedCamParsPrecision", m_iCodedCamParPrecision, STD_CAM_PARAMETERS_PRECISION, "precision for coding of camera parameters (in units of 2^(-x) luma samples)" )

#ifdef WEIGHT_PRED
  ("weighted_pred_flag,-wpP",     m_bUseWeightPred, false, "weighted prediction flag (P-Slices)")
  ("weighted_bipred_idc,-wpBidc", m_uiBiPredIdc,    0u,    "weighted bipred idc (B-Slices)")
#endif
  ("SourceWidth,-wdt",      m_iSourceWidth,  0, "Source picture width")
  ("SourceHeight,-hgt",     m_iSourceHeight, 0, "Source picture height")
  ("InputBitDepth",         m_uiInputBitDepth, 8u, "bit-depth of input file")
  ("BitDepth",              m_uiInputBitDepth, 8u, "deprecated alias of InputBitDepth")
  ("OutputBitDepth",        m_uiOutputBitDepth, 0u, "bit-depth of output file")
#if ENABLE_IBDI
  ("BitIncrement",          m_uiBitIncrement, 0xffffffffu, "bit-depth increasement")
#endif
  ("InternalBitDepth",      m_uiInternalBitDepth, 0u, "Internal bit-depth (BitDepth+BitIncrement)")
  ("HorizontalPadding,-pdx",m_aiPad[0],      0, "horizontal source padding size")
  ("VerticalPadding,-pdy",  m_aiPad[1],      0, "vertical source padding size")
  ("PAD",                   m_bUsePAD,   false, "automatic source padding of multiple of 16" )
  ("FrameRate,-fr",         m_iFrameRate,        0, "Frame rate")
  ("FrameSkip,-fs",         m_FrameSkip,         0u, "Number of frames to skip at start of input YUV")
  ("FramesToBeEncoded,f",   m_iFrameToBeEncoded, 0, "number of frames to be encoded (default=all)")
  ("FrameToBeEncoded",      m_iFrameToBeEncoded, 0, "depricated alias of FramesToBeEncoded")

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
  ("CodedPictureStoreSize,cpss",  m_uiCodedPictureStoreSize, 16u, "Size of coded picture Buffer")
#if DCM_DECODING_REFRESH
  ("DecodingRefreshType,-dr",m_iDecodingRefreshType, 0, "intra refresh, (0:none 1:CDR 2:IDR)")
#endif
  ("GOPSize,g",      m_iGOPSize,      1, "GOP size of temporal structure")
  ("RateGOPSize,-rg",m_iRateGOPSize, -1, "GOP size of hierarchical QP assignment (-1: implies inherit GOPSize value)")
#if !HHI_NO_LowDelayCoding
  ("LowDelayCoding",         m_bUseLDC,             false, "low-delay mode")
#endif
#if DCM_COMB_LIST
  ("ListCombination, -lc", m_bUseLComb, true, "combined reference list for uni-prediction in B-slices")
  ("LCModification", m_bLCMod, false, "enables signalling of combined reference list derivation")
#endif

  ("GOPFormatString,gfs", m_cInputFormatString,  string(""), "Group of pictures")

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
  ("dQPFile,m",     m_pchdQPFile , (char*) 0, "dQP file name")
  ("RDOQ",          m_abUseRDOQ, std::vector<Bool>(1,true), "Enable RDOQ")
  ("TemporalLayerQPOffset_L0,-tq0", m_aiTLayerQPOffset[0], 0, "QP offset of temporal layer 0")
  ("TemporalLayerQPOffset_L1,-tq1", m_aiTLayerQPOffset[1], 0, "QP offset of temporal layer 1")
  ("TemporalLayerQPOffset_L2,-tq2", m_aiTLayerQPOffset[2], 0, "QP offset of temporal layer 2")
  ("TemporalLayerQPOffset_L3,-tq3", m_aiTLayerQPOffset[3], 0, "QP offset of temporal layer 3")
  ("TemporalLayerQPOffset_L4,-tq4", m_aiTLayerQPOffset[4], 0, "QP offset of temporal layer 3")
  ("TemporalLayerQPOffset_L5,-tq5", m_aiTLayerQPOffset[5], 0, "QP offset of temporal layer 3")
  ("TemporalLayerQPOffset_L6,-tq6", m_aiTLayerQPOffset[6], 0, "QP offset of temporal layer 3")
  ("TemporalLayerQPOffset_L7,-tq7", m_aiTLayerQPOffset[7], 0, "QP offset of temporal layer 3")
  ("TemporalLayerQPOffset_L8,-tq8", m_aiTLayerQPOffset[8], 0, "QP offset of temporal layer 3")
  ("TemporalLayerQPOffset_L9,-tq9", m_aiTLayerQPOffset[9], 0, "QP offset of temporal layer 3")

  /* Entropy coding parameters */
  ("SymbolMode,-sym", m_iSymbolMode, 1, "symbol mode (0=VLC, 1=SBAC)")
  ("SBACRD", m_bUseSBACRD, true, "SBAC based RD estimation")

  /* Deblocking filter parameters */
  ("LoopFilterDisable", m_abLoopFilterDisable, std::vector<Bool>(1,false), "Disables LoopFilter")
  ("LoopFilterAlphaC0Offset", m_iLoopFilterAlphaC0Offset, 0)
  ("LoopFilterBetaOffset", m_iLoopFilterBetaOffset, 0 )

  /* Camera Paremetes */
  ("CameraParameterFile,cpf", m_pchCameraParameterFile,    (Char *) 0, "Camera Parameter File Name")
  ("BaseViewCameraNumbers" ,  m_pchBaseViewCameraNumbers,  (Char *) 0, "Numbers of base views")


    /* View Synthesis Optimization */

#if HHI_VSO
  ("VSOConfig",                       m_pchVSOConfig            , (Char *) 0    , "VSO configuration")
    ("VSO",                             m_bUseVSO                 , false         , "Use VSO" )
    // GT: For development, will be removed later
  ("VSOMode",                         m_uiVSOMode               , (UInt)   4    , "VSO Mode")
#if HHI_VSO_LS_TABLE
  ("LambdaScaleVSO",                  m_dLambdaScaleVSO         , (Double) 1  , "Lambda Scaling for VSO")
#else
  ("LambdaScaleVSO",                  m_dLambdaScaleVSO         , (Double) 0.5  , "Lambda Scaling for VSO")
#endif
    ("ForceLambdaScaleVSO",             m_bForceLambdaScaleVSO    , false         , "Force using Lambda Scale VSO also in non-VSO-Mode")
#if HHI_VSO_DIST_INT
  ("AllowNegDist",                    m_bAllowNegDist           , true         , "Allow negative Distortion in VSO")
#endif

    ("NumberOfExternalRefs",            m_iNumberOfExternalRefs   , 0             , "Number of virtual external reference views")
    ("ERRefFile_%d,er_%d",              m_pchERRefFileList        , (char *) 0    , MAX_ERREF_VIEW_NUM , "virtual external reference view file name %d")
    ("VSERViewReferences_%d,evr_%d"  ,  m_aaiERViewRefInd         , vector<Int>() , MAX_INPUT_VIEW_NUM, "Numbers of external virtual reference views to be used for this view")
    ("VSBaseViewReferences_%d,bvr_%d",  m_aaiBaseViewRefInd       , vector<Int>() , MAX_INPUT_VIEW_NUM, "Numbers of external virtual reference views to be used for this view")
#endif

  /* Coding tools */
  ("MRG", m_bUseMRG, true, "merging of motion partitions")

#if LM_CHROMA
  ("LMChroma", m_bUseLMChroma, true, "intra chroma prediction based on recontructed luma")
#endif

  ("ALF", m_abUseALF, std::vector<Bool>(1,true), "Enables ALF")
#if MTK_SAO
  ("SAO", m_abUseSAO, std::vector<Bool>(1, true), "SAO")
#endif

#if MQT_ALF_NPASS
  ("ALFEncodePassReduction", m_iALFEncodePassReduction, 0, "0:Original 16-pass, 1: 1-pass, 2: 2-pass encoding")
#endif
#if HHI_RMP_SWITCH
  ("RMP", m_bUseRMP ,true, "Rectangular motion partition" )
#endif
#ifdef ROUNDING_CONTROL_BIPRED
  ("RoundingControlBipred", m_useRoundingControlBipred, false, "Rounding control for bi-prediction")
#endif
    ("SliceMode",            m_iSliceMode,           0, "0: Disable all Recon slice limits, 1: Enforce max # of LCUs, 2: Enforce max # of bytes")
    ("SliceArgument",        m_iSliceArgument,       0, "if SliceMode==1 SliceArgument represents max # of LCUs. if SliceMode==2 SliceArgument represents max # of bytes.")
    ("EntropySliceMode",     m_iEntropySliceMode,    0, "0: Disable all entropy slice limits, 1: Enforce max # of LCUs, 2: Enforce constraint based entropy slices")
    ("EntropySliceArgument", m_iEntropySliceArgument,0, "if EntropySliceMode==1 SliceArgument represents max # of LCUs. if EntropySliceMode==2 EntropySliceArgument represents max # of bins.")
#if MTK_NONCROSS_INLOOP_FILTER
    ("LFCrossSliceBoundaryFlag", m_bLFCrossSliceBoundaryFlag, true)
#endif
#if CONSTRAINED_INTRA_PRED
  ("ConstrainedIntraPred", m_bUseConstrainedIntraPred, false, "Constrained Intra Prediction")
#endif
  /* Misc. */
  ("SEIpictureDigest", m_pictureDigestEnabled, false, "Control generation of picture_digest SEI messages\n"
                                              "\t1: use MD5\n"
                                              "\t0: disable")
  ("FEN", m_bUseFastEnc, false, "fast encoder setting")

#if HHI_DMM_WEDGE_INTRA || HHI_DMM_PRED_TEX
  ("DMM", m_bUseDMM, false, "add depth modes intra")
#endif
#if HHI_MPI
  ("MVI", m_bUseMVI, false, "use motion vector inheritance for depth map coding")
#endif

  /* Multiview tools */
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

  ("QpChangeFrame", m_iQpChangeFrame, PicOrderCnt(0), "start frame for QP change")
  ("QpChangeOffsetVideo", m_iQpChangeOffsetVideo, 0, "QP change offset for video")
  ("QpChangeOffsetDepth", m_iQpChangeOffsetDepth, 0, "QP change offset for depth")
#if HHI_INTERVIEW_SKIP
  ("InterViewSkip",  m_uiInterViewSkip,    (UInt)0, "usage of interview skip" )
#if HHI_INTERVIEW_SKIP_LAMBDA_SCALE
  ("InterViewSkipLambdaScale",  m_dInterViewSkipLambdaScale,    (Double)8, "lambda scale for interview skip" )
#endif
#endif

  /* Compatability with old style -1 FOO or -0 FOO options. */
  ("1", doOldStyleCmdlineOn, "turn option <name> on")
  ("0", doOldStyleCmdlineOff, "turn option <name> off")
  ;

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


// GT FIX
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
// GT FIX END

  if (m_iRateGOPSize == -1)
  {
    /* if rateGOPSize has not been specified, the default value is GOPSize */
    m_iRateGOPSize = m_iGOPSize;
  }

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

//GT QP Depth
  if ( m_adQP.size() < 2 )
  {
    m_adQP.push_back( m_adQP[0] );
  };
  for (UInt uiK = 0; uiK < m_adQP.size(); uiK++)
  {
    m_aiQP.push_back( (Int)( m_adQP[uiK] ) );
  }
//GT QP Depth end

#if HHI_VSO
  m_bUseVSO = m_bUseVSO && m_bUsingDepthMaps && (m_uiVSOMode != 0);
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
  if ( m_bUseVSO )
  {
    if ( m_iNumberOfExternalRefs != 0 )
    {
      xCleanUpVector( m_aaiERViewRefInd,    vector<Int>() );
      xCleanUpVector( m_aaiBaseViewRefInd,  vector<Int>() );
    }
    else
    {
      m_aaiERViewRefInd   .clear();
      m_aaiERViewRefInd   .resize( m_iNumberOfViews );
      m_aaiERViewRefLutInd.clear();
      m_aaiERViewRefLutInd.resize( m_iNumberOfViews );
      m_aaiBaseViewRefInd .clear();
      m_aaiBaseViewRefInd .resize( m_iNumberOfViews );
    }
  }

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
  AOT( (m_aiQP[1] < 0) || (m_aiQP[1] > 51));
  m_dLambdaScaleVSO *= adLambdaScaleTable[m_aiQP[1]]; 
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
#if ENABLE_IBDI
  xConfirmPara( m_uiInternalBitDepth > 0 && (int)m_uiBitIncrement != -1,                    "InternalBitDepth and BitIncrement may not be specified simultaneously");
#else
  xConfirmPara( m_uiInputBitDepth < 8,                                                      "InputBitDepth must be at least 8" );
#endif
  xConfirmPara( m_iFrameRate <= 0,                                                          "Frame rate must be more than 1" );
  xConfirmPara( m_iFrameToBeEncoded <= 0,                                                   "Total Number Of Frames encoded must be more than 1" );
  xConfirmPara( m_iGOPSize < 1 ,                                                            "GOP Size must be more than 1" );
#if DCM_DECODING_REFRESH
  xConfirmPara( m_iDecodingRefreshType < 0 || m_iDecodingRefreshType > 2,                   "Decoding Refresh Type must be equal to 0, 1 or 2" );
#endif
//GT QP Depth
  xConfirmPara( m_aiQP[0] < 0 || m_aiQP[0] > 51,                                             "QP exceeds supported range (0 to 51)" );
  if ( m_aiQP.size() >= 2 )
  {
    xConfirmPara( m_aiQP[1] < 0 || m_aiQP[1] > 51,                                           "QP Depth exceeds supported range (0 to 51)" );
  }
//GT QP Depth End
#if MQT_ALF_NPASS
  xConfirmPara( m_iALFEncodePassReduction < 0 || m_iALFEncodePassReduction > 2,             "ALFEncodePassReduction must be equal to 0, 1 or 2");
#endif
  xConfirmPara( m_iLoopFilterAlphaC0Offset < -26 || m_iLoopFilterAlphaC0Offset > 26,        "Loop Filter Alpha Offset exceeds supported range (-26 to 26)" );
  xConfirmPara( m_iLoopFilterBetaOffset < -26 || m_iLoopFilterBetaOffset > 26,              "Loop Filter Beta Offset exceeds supported range (-26 to 26)");
  xConfirmPara( m_iFastSearch < 0 || m_iFastSearch > 2,                                     "Fast Search Mode is not supported value (0:Full search  1:Diamond  2:PMVFAST)" );
  xConfirmPara( m_iSearchRange < 0 ,                                                        "Search Range must be more than 0" );
  xConfirmPara( m_bipredSearchRange < 0 ,                                                   "Search Range must be more than 0" );
  xConfirmPara( m_iMaxDeltaQP > 7,                                                          "Absolute Delta QP exceeds supported range (0 to 7)" );
  xConfirmPara( (m_uiMaxCUWidth  >> m_uiMaxCUDepth) < 4,                                    "Minimum partition width size should be larger than or equal to 8");
  xConfirmPara( (m_uiMaxCUHeight >> m_uiMaxCUDepth) < 4,                                    "Minimum partition height size should be larger than or equal to 8");
  xConfirmPara( m_uiMaxCUWidth < 16,                                                        "Maximum partition width size should be larger than or equal to 16");
  xConfirmPara( m_uiMaxCUHeight < 16,                                                       "Maximum partition height size should be larger than or equal to 16");
  xConfirmPara( (m_iSourceWidth  % (m_uiMaxCUWidth  >> (m_uiMaxCUDepth-1)))!=0,             "Frame width should be multiple of minimum CU size");
  xConfirmPara( (m_iSourceHeight % (m_uiMaxCUHeight >> (m_uiMaxCUDepth-1)))!=0,             "Frame height should be multiple of minimum CU size");

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

  xConfirmPara( m_iSliceMode < 0 || m_iSliceMode > 2, "SliceMode exceeds supported range (0 to 2)" );
  if (m_iSliceMode!=0)
  {
    xConfirmPara( m_iSliceArgument < 1 ,         "SliceArgument should be larger than or equal to 1" );
  }
  xConfirmPara( m_iEntropySliceMode < 0 || m_iEntropySliceMode > 2, "EntropySliceMode exceeds supported range (0 to 2)" );
  if (m_iEntropySliceMode!=0)
  {
    xConfirmPara( m_iEntropySliceArgument < 1 ,         "EntropySliceArgument should be larger than or equal to 1" );
  }

  xConfirmPara( m_iSymbolMode < 0 || m_iSymbolMode > 1,                                     "SymbolMode must be equal to 0 or 1" );

  // Check MultiView stuff
  xConfirmPara    ( m_iNumberOfViews > MAX_INPUT_VIEW_NUM ,                           "NumberOfViews must be less than or equal to MAX_INPUT_VIEW_NUM");
  xConfirmPara    ( Int( m_pchInputFileList.size() ) < m_iNumberOfViews,              "Number of InputFiles must be greater than or equal to NumberOfViews" );
  xConfirmPara    ( Int( m_pchReconFileList.size() ) < m_iNumberOfViews,              "Number of ReconFiles must be greater than or equal to NumberOfViews" );

  xConfirmPara    ( m_iCodedCamParPrecision < 0 || m_iCodedCamParPrecision > 5,       "CodedCamParsPrecision must be in range of 0..5" );
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

#if HHI_INTERVIEW_SKIP
  xConfirmPara    ( m_uiInterViewSkip > 1,                                        "RenderingSkipMode > 1 not supported" );
  xConfirmPara    ( m_uiInterViewSkip > 0 && !m_bUsingDepthMaps,                  "RenderingSkipMode > 0 requires CodeDepthMaps = 1" );
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
      xConfirmPara( m_uiVSOMode < 0 || m_uiVSOMode > 4 ,                             "VSO Mode must be greater than or equal to 0 and less than 5");
      xConfirmPara( m_iNumberOfExternalRefs               > MAX_ERREF_VIEW_NUM,      "NumberOfExternalRefs must be less than of equal to TAppMVEncCfg::MAX_ERREF_VIEW_NUM" );
      xConfirmPara( Int( m_pchERRefFileList .size() ) < m_iNumberOfExternalRefs,     "Number of ERRefFileFiles  must be greater than or equal to NumberOfExternalRefs" );
    }
#endif
  }

#if DCM_COMB_LIST
#if !HHI_NO_LowDelayCoding
  xConfirmPara( m_bUseLComb==false && m_bUseLDC==false,         "LComb can only be 0 if LowDelayCoding is 1" );
#else
  xConfirmPara( m_bUseLComb==false,                             "LComb can only be 0 if LowDelayCoding is 1" );
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

  // SBACRD is supported only for SBAC
  if ( m_iSymbolMode == 0 )
  {
    m_bUseSBACRD = false;
  }

#undef xConfirmPara
  if (check_failed)
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

#if HHI_VSO
  if ( m_bUseVSO)
  {
    xCleanUpVector( m_pchERRefFileList,       (char*)0 );
  }
#endif
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
#if ENABLE_IBDI
  if ((int)m_uiBitIncrement != -1)
  {
    g_uiBitDepth = m_uiInputBitDepth;
    g_uiBitIncrement = m_uiBitIncrement;
    m_uiInternalBitDepth = g_uiBitDepth + g_uiBitIncrement;
  }
  else
  {
    g_uiBitDepth = min(8u, m_uiInputBitDepth);
    if (m_uiInternalBitDepth == 0) {
      /* default increement = 2 */
      m_uiInternalBitDepth = 2 + g_uiBitDepth;
    }
    g_uiBitIncrement = m_uiInternalBitDepth - g_uiBitDepth;
  }
#else
#if FULL_NBIT
  g_uiBitDepth = m_uiInternalBitDepth;
  g_uiBitIncrement = 0;
#else
  g_uiBitDepth = 8;
  g_uiBitIncrement = m_uiInternalBitDepth - g_uiBitDepth;
#endif
#endif
#if HHI_DMM_WEDGE_INTRA || HHI_DMM_PRED_TEX
  g_dDeltaDCsQuantOffset = Double(g_uiBitIncrement) -  2.0;
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
}

Void TAppEncCfg::xPrintParameter()
{
  printf("\n");
  for( Int iCounter = 0; iCounter<m_iNumberOfViews; iCounter++)
  {
    printf("Input          File %i        : %s\n", iCounter, m_pchInputFileList[iCounter]);
  }
  for( Int iCounter = 0; iCounter < (Int)m_pchDepthInputFileList.size(); iCounter++)
  {
    printf("DepthInput     File %i        : %s\n", iCounter, m_pchDepthInputFileList[iCounter]);
  }
  printf("Bitstream      File          : %s\n", m_pchBitstreamFile      );
  for( Int iCounter = 0; iCounter<m_iNumberOfViews; iCounter++)
  {
    printf("Reconstruction File %i        : %s\n", iCounter, m_pchReconFileList[iCounter]);
  }

  for( Int iCounter = 0; iCounter<(Int)m_pchDepthReconFileList.size(); iCounter++)
  {
    printf("Reconstruction Depth File %i  : %s\n", iCounter, m_pchDepthReconFileList[iCounter]);
  }
  printf("Real     Format              : %dx%d %dHz\n", m_iSourceWidth - m_aiPad[0], m_iSourceHeight-m_aiPad[1], m_iFrameRate );
  printf("Internal Format              : %dx%d %dHz\n", m_iSourceWidth, m_iSourceHeight, m_iFrameRate );
  printf("Frame index                  : %u - %d (%d frames)\n", m_FrameSkip, m_FrameSkip+m_iFrameToBeEncoded-1, m_iFrameToBeEncoded );
  printf("CU size / depth              : %d / %d\n", m_uiMaxCUWidth, m_uiMaxCUDepth );
  printf("RQT trans. size (min / max)  : %d / %d\n", 1 << m_uiQuadtreeTULog2MinSize, 1 << m_uiQuadtreeTULog2MaxSize );
  printf("Max RQT depth inter          : %d\n", m_uiQuadtreeTUMaxDepthInter);
  printf("Max RQT depth intra          : %d\n", m_uiQuadtreeTUMaxDepthIntra);
  printf("Motion search range          : %d\n", m_iSearchRange );
#if DCM_DECODING_REFRESH
  printf("Decoding refresh type        : %d\n", m_iDecodingRefreshType );
#endif
  printf("QP                           : %5.2f\n", m_adQP[0] );
  printf("QP Depth                     : %5.2f\n", m_adQP[ m_adQP.size()  < 2 ? 0 : 1] );
  printf("Coded Picture Store Size     : %d\n", m_uiCodedPictureStoreSize);
  printf("GOP size                     : %d\n", m_iGOPSize );
  printf("Rate GOP size                : %d\n", m_iRateGOPSize );
  printf("Internal bit depth           : %d\n", m_uiInternalBitDepth );

  if ( m_iSymbolMode == 0 )
  {
    printf("Entropy coder                : VLC\n");
  }
  else if( m_iSymbolMode == 1 )
  {
    printf("Entropy coder                : CABAC\n");
  }
  else if( m_iSymbolMode == 2 )
  {
    printf("Entropy coder                : PIPE\n");
  }
  else
  {
    assert(0);
  }

  printf("GOP Format String            : %s\n", m_cInputFormatString.c_str() );
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
  }
#endif


  printf("\n");

  printf("TOOL CFG GENERAL: ");
  printf("IBD:%d ", !!g_uiBitIncrement);
  printf("HAD:%d ", m_bUseHADME           );
  printf("SRD:%d ", m_bUseSBACRD          );
  printf("SQP:%d ", m_uiDeltaQpRD         );
  printf("ASR:%d ", m_bUseASR             );
  printf("PAD:%d ", m_bUsePAD             );
  printf("LDC:%d ", m_bUseLDC             );
#if DCM_COMB_LIST
  printf("LComb:%d ", m_bUseLComb         );
  printf("LCMod:%d ", m_bLCMod         );
#endif
  printf("FEN:%d ", m_bUseFastEnc         );
  printf("RQT:%d ", 1     );
  printf("MRG:%d ", m_bUseMRG             ); // SOPH: Merge Mode
#if LM_CHROMA
  printf("LMC:%d ", m_bUseLMChroma        );
#endif
#if HHI_RMP_SWITCH
  printf("RMP:%d ", m_bUseRMP);
#endif
  printf("Slice:%d ",m_iSliceMode);
  if (m_iSliceMode!=0)
  {
    printf("(%d) ", m_iSliceArgument);
  }
  printf("EntropySlice:%d ",m_iEntropySliceMode);
  if (m_iEntropySliceMode!=0)
  {
    printf("(%d) ", m_iEntropySliceArgument);
  }
#if CONSTRAINED_INTRA_PRED
  printf("CIP:%d ", m_bUseConstrainedIntraPred);
#endif
#if MTK_SAO
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
#if HHI_DMM_WEDGE_INTRA || HHI_DMM_PRED_TEX
  printf("DMM:%d ", m_bUseDMM );
#endif
#if HHI_MPI
  printf("MVI:%d ", m_bUseMVI ? 1 : 0 );
#endif
  printf("\n");

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
  printf( "                   MRG - merging of motion partitions\n"); // SOPH: Merge Mode

#if LM_CHROMA
  printf( "                   LMC - intra chroma prediction based on luma\n");
#endif

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
