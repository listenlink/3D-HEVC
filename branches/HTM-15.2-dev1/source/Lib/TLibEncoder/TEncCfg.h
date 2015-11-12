/* The copyright in this software is being made available under the BSD
 * License, included below. This software may be subject to other third party
 * and contributor rights, including patent rights, and no such rights are
 * granted under this license.
 *
 * Copyright (c) 2010-2015, ITU/ISO/IEC
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
#include "TLibCommon/TComSlice.h"
#include <assert.h>

#if NH_MV
#include "TLibCommon/SEI.h"
#endif

struct GOPEntry
{
  Int m_POC;
  Int m_QPOffset;
  Double m_QPFactor;
  Int m_tcOffsetDiv2;
  Int m_betaOffsetDiv2;
  Int m_temporalId;
  Bool m_refPic;
  Int m_numRefPicsActive;
  SChar m_sliceType;
  Int m_numRefPics;
  Int m_referencePics[MAX_NUM_REF_PICS];
  Int m_usedByCurrPic[MAX_NUM_REF_PICS];
  Int m_interRPSPrediction;
  Int m_deltaRPS;
  Int m_numRefIdc;
  Int m_refIdc[MAX_NUM_REF_PICS+1];
  Bool m_isEncoded;
  #if NH_MV
  Int m_numActiveRefLayerPics;
  Int m_interLayerPredLayerIdc [MAX_NUM_REF_PICS];
  Int m_interViewRefPosL[2][MAX_NUM_REF_PICS];  
#endif

GOPEntry()
  : m_POC(-1)
  , m_QPOffset(0)
  , m_QPFactor(0)
  , m_tcOffsetDiv2(0)
  , m_betaOffsetDiv2(0)
  , m_temporalId(0)
  , m_refPic(false)
  , m_numRefPicsActive(0)
  , m_sliceType('P')
  , m_numRefPics(0)
  , m_interRPSPrediction(false)
  , m_deltaRPS(0)
  , m_numRefIdc(0)
  , m_isEncoded(false)
#if NH_MV
  , m_numActiveRefLayerPics(0)
#endif

  {
    ::memset( m_referencePics, 0, sizeof(m_referencePics) );
    ::memset( m_usedByCurrPic, 0, sizeof(m_usedByCurrPic) );
    ::memset( m_refIdc,        0, sizeof(m_refIdc) );
#if NH_MV
    ::memset( m_interLayerPredLayerIdc,   0, sizeof(m_interLayerPredLayerIdc) );
    ::memset( m_interViewRefPosL[0], -1, sizeof(m_interViewRefPosL[0]) );
    ::memset( m_interViewRefPosL[1], -1, sizeof(m_interViewRefPosL[1]) );
#endif
  }
};

std::istringstream &operator>>(std::istringstream &in, GOPEntry &entry);     //input
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
  Window    m_conformanceWindow;
  Int       m_framesToBeEncoded;
  Double    m_adLambdaModifier[ MAX_TLAYER ];
  std::vector<Double> m_adIntraLambdaModifier;
  Double    m_dIntraQpFactor;                                 ///< Intra Q Factor. If negative, use a default equation: 0.57*(1.0 - Clip3( 0.0, 0.5, 0.05*(Double)(isField ? (GopSize-1)/2 : GopSize-1) ))

  Bool      m_printMSEBasedSequencePSNR;
  Bool      m_printFrameMSE;
  Bool      m_printSequenceMSE;
  Bool      m_cabacZeroWordPaddingEnabled;

  /* profile & level */
  Profile::Name m_profile;
  Level::Tier   m_levelTier;
  Level::Name   m_level;
  Bool m_progressiveSourceFlag;
  Bool m_interlacedSourceFlag;
  Bool m_nonPackedConstraintFlag;
  Bool m_frameOnlyConstraintFlag;
  UInt              m_bitDepthConstraintValue;
  ChromaFormat      m_chromaFormatConstraintValue;
  Bool              m_intraConstraintFlag;
  Bool              m_onePictureOnlyConstraintFlag;
  Bool              m_lowerBitRateConstraintFlag;

  //====== Coding Structure ========
  UInt      m_uiIntraPeriod;
  UInt      m_uiDecodingRefreshType;            ///< the type of decoding refresh employed for the random access.
  Int       m_iGOPSize;
#if NH_MV
  GOPEntry  m_GOPList[MAX_GOP+1];
#else
  GOPEntry  m_GOPList[MAX_GOP];
#endif
  Int       m_extraRPSs;
  Int       m_maxDecPicBuffering[MAX_TLAYER];
  Int       m_numReorderPics[MAX_TLAYER];

  Int       m_iQP;                              //  if (AdaptiveQP == OFF)

  Int       m_aiPad[2];

  Bool      m_AccessUnitDelimiter;               ///< add Access Unit Delimiter NAL units

  Int       m_iMaxRefPicNum;                     ///< this is used to mimic the sliding mechanism used by the decoder
                                                 // TODO: We need to have a common sliding mechanism used by both the encoder and decoder

  Int       m_maxTempLayer;                      ///< Max temporal layer
  Bool      m_useAMP;
  UInt      m_maxCUWidth;
  UInt      m_maxCUHeight;
  UInt      m_maxTotalCUDepth;
  UInt      m_log2DiffMaxMinCodingBlockSize;

  //======= Transform =============
  UInt      m_uiQuadtreeTULog2MaxSize;
  UInt      m_uiQuadtreeTULog2MinSize;
  UInt      m_uiQuadtreeTUMaxDepthInter;
  UInt      m_uiQuadtreeTUMaxDepthIntra;

  //====== Loop/Deblock Filter ========
  Bool      m_bLoopFilterDisable;
  Bool      m_loopFilterOffsetInPPS;
  Int       m_loopFilterBetaOffsetDiv2;
  Int       m_loopFilterTcOffsetDiv2;
  Bool      m_DeblockingFilterMetric;
  Bool      m_bUseSAO;
  Bool      m_bTestSAODisableAtPictureLevel;
  Double    m_saoEncodingRate;       // When non-0 SAO early picture termination is enabled for luma and chroma
  Double    m_saoEncodingRateChroma; // The SAO early picture termination rate to use for chroma (when m_SaoEncodingRate is >0). If <=0, use results for luma.
  Int       m_maxNumOffsetsPerPic;
  Bool      m_saoCtuBoundary;

  //====== Motion search ========
  Bool      m_bDisableIntraPUsInInterSlices;
  MESearchMethod m_motionEstimationSearchMethod;
  Int       m_iSearchRange;                     //  0:Full frame
  Int       m_bipredSearchRange;
  Bool      m_bClipForBiPredMeEnabled;
  Bool      m_bFastMEAssumingSmootherMVEnabled;
  Int       m_minSearchWindow;
  Bool      m_bRestrictMESampling;

#if NH_MV
  Bool      m_bUseDisparitySearchRangeRestriction;
  Int       m_iVerticalDisparitySearchRange;
#endif
  //====== Quality control ========
  Int       m_iMaxDeltaQP;                      //  Max. absolute delta QP (1:default)
  Int       m_iMaxCuDQPDepth;                   //  Max. depth for a minimum CuDQP (0:default)
  Int       m_diffCuChromaQpOffsetDepth;        ///< If negative, then do not apply chroma qp offsets.

  Int       m_chromaCbQpOffset;                 //  Chroma Cb QP Offset (0:default)
  Int       m_chromaCrQpOffset;                 //  Chroma Cr Qp Offset (0:default)
  ChromaFormat m_chromaFormatIDC;

#if ADAPTIVE_QP_SELECTION
  Bool      m_bUseAdaptQpSelect;
#endif
  Bool      m_extendedPrecisionProcessingFlag;
  Bool      m_highPrecisionOffsetsEnabledFlag;
  Bool      m_bUseAdaptiveQP;
  Int       m_iQPAdaptationRange;

  //====== Tool list ========
  Int       m_bitDepth[MAX_NUM_CHANNEL_TYPE];
  Bool      m_bUseASR;
  Bool      m_bUseHADME;
  Bool      m_useRDOQ;
  Bool      m_useRDOQTS;
#if T0196_SELECTIVE_RDOQ
  Bool      m_useSelectiveRDOQ;
#endif
  UInt      m_rdPenalty;
  FastInterSearchMode m_fastInterSearchMode;
  Bool      m_bUseEarlyCU;
  Bool      m_useFastDecisionForMerge;
  Bool      m_bUseCbfFastMode;
  Bool      m_useEarlySkipDetection;
  Bool      m_crossComponentPredictionEnabledFlag;
  Bool      m_reconBasedCrossCPredictionEstimate;
  UInt      m_log2SaoOffsetScale[MAX_NUM_CHANNEL_TYPE];
  Bool      m_useTransformSkip;
  Bool      m_useTransformSkipFast;
  UInt      m_log2MaxTransformSkipBlockSize;
  Bool      m_transformSkipRotationEnabledFlag;
  Bool      m_transformSkipContextEnabledFlag;
  Bool      m_persistentRiceAdaptationEnabledFlag;
  Bool      m_cabacBypassAlignmentEnabledFlag;
  Bool      m_rdpcmEnabledFlag[NUMBER_OF_RDPCM_SIGNALLING_MODES];
  Int*      m_aidQP;
  UInt      m_uiDeltaQpRD;
  Bool      m_bFastDeltaQP;

  Bool      m_bUseConstrainedIntraPred;
  Bool      m_bFastUDIUseMPMEnabled;
  Bool      m_bFastMEForGenBLowDelayEnabled;
  Bool      m_bUseBLambdaForNonKeyLowDelayPictures;
  Bool      m_usePCM;
  Int       m_PCMBitDepth[MAX_NUM_CHANNEL_TYPE];
  UInt      m_pcmLog2MaxSize;
  UInt      m_uiPCMLog2MinSize;
  //====== Slice ========
  SliceConstraint m_sliceMode;
  Int       m_sliceArgument;
  //====== Dependent Slice ========
  SliceConstraint m_sliceSegmentMode;
  Int       m_sliceSegmentArgument;
  Bool      m_bLFCrossSliceBoundaryFlag;

  Bool      m_bPCMInputBitDepthFlag;
  Bool      m_bPCMFilterDisableFlag;
  Bool      m_intraSmoothingDisabledFlag;
  Bool      m_loopFilterAcrossTilesEnabledFlag;
  Bool      m_tileUniformSpacingFlag;
  Int       m_iNumColumnsMinus1;
  Int       m_iNumRowsMinus1;
  std::vector<Int> m_tileColumnWidth;
  std::vector<Int> m_tileRowHeight;

  Bool      m_entropyCodingSyncEnabledFlag;

  HashType  m_decodedPictureHashSEIType;
  Bool      m_bufferingPeriodSEIEnabled;
  Bool      m_pictureTimingSEIEnabled;
  Bool      m_recoveryPointSEIEnabled;
  Bool      m_toneMappingInfoSEIEnabled;
  Int       m_toneMapId;
  Bool      m_toneMapCancelFlag;
  Bool      m_toneMapPersistenceFlag;
  Int       m_codedDataBitDepth;
  Int       m_targetBitDepth;
  Int       m_modelId;
  Int       m_minValue;
  Int       m_maxValue;
  Int       m_sigmoidMidpoint;
  Int       m_sigmoidWidth;
  Int       m_numPivots;
  Int       m_cameraIsoSpeedIdc;
  Int       m_cameraIsoSpeedValue;
  Int       m_exposureIndexIdc;
  Int       m_exposureIndexValue;
  Bool      m_exposureCompensationValueSignFlag;
  Int       m_exposureCompensationValueNumerator;
  Int       m_exposureCompensationValueDenomIdc;
  Int       m_refScreenLuminanceWhite;
  Int       m_extendedRangeWhiteLevel;
  Int       m_nominalBlackLevelLumaCodeValue;
  Int       m_nominalWhiteLevelLumaCodeValue;
  Int       m_extendedWhiteLevelLumaCodeValue;
  Int*      m_startOfCodedInterval;
  Int*      m_codedPivotValue;
  Int*      m_targetPivotValue;
  Bool      m_framePackingSEIEnabled;
  Int       m_framePackingSEIType;
  Int       m_framePackingSEIId;
  Int       m_framePackingSEIQuincunx;
  Int       m_framePackingSEIInterpretation;
  Bool      m_segmentedRectFramePackingSEIEnabled;
  Bool      m_segmentedRectFramePackingSEICancel;
  Int       m_segmentedRectFramePackingSEIType;
  Bool      m_segmentedRectFramePackingSEIPersistence;
  Int       m_displayOrientationSEIAngle;
  Bool      m_temporalLevel0IndexSEIEnabled;
  Bool      m_gradualDecodingRefreshInfoEnabled;
  Int       m_noDisplaySEITLayer;
  Bool      m_decodingUnitInfoSEIEnabled;
  Bool      m_SOPDescriptionSEIEnabled;
  Bool      m_scalableNestingSEIEnabled;
  Bool      m_tmctsSEIEnabled;
  Bool      m_timeCodeSEIEnabled;
  Int       m_timeCodeSEINumTs;
  TComSEITimeSet   m_timeSetArray[MAX_TIMECODE_SEI_SETS];
  Bool      m_kneeSEIEnabled;
  Int       m_kneeSEIId;
  Bool      m_kneeSEICancelFlag;
  Bool      m_kneeSEIPersistenceFlag;
  Int       m_kneeSEIInputDrange;
  Int       m_kneeSEIInputDispLuminance;
  Int       m_kneeSEIOutputDrange;
  Int       m_kneeSEIOutputDispLuminance;
  Int       m_kneeSEINumKneePointsMinus1;
  Int*      m_kneeSEIInputKneePoint;
  Int*      m_kneeSEIOutputKneePoint;
  std::string m_colourRemapSEIFileRoot;          ///< SEI Colour Remapping File (initialized from external file)
  TComSEIMasteringDisplay m_masteringDisplay;
#if NH_MV
  SEIMessages* m_seiMessages; 
#endif
  //====== Weighted Prediction ========
  Bool      m_useWeightedPred;       //< Use of Weighting Prediction (P_SLICE)
  Bool      m_useWeightedBiPred;    //< Use of Bi-directional Weighting Prediction (B_SLICE)
  WeightedPredictionMethod m_weightedPredictionMethod;
  UInt      m_log2ParallelMergeLevelMinus2;       ///< Parallel merge estimation region
  UInt      m_maxNumMergeCand;                    ///< Maximum number of merge candidates
  ScalingListMode m_useScalingListId;            ///< Using quantization matrix i.e. 0=off, 1=default, 2=file.
  std::string m_scalingListFileName;              ///< quantization matrix file name
  Int       m_TMVPModeId;
  Bool      m_signHideFlag;
  Bool      m_RCEnableRateControl;
  Int       m_RCTargetBitrate;
  Int       m_RCKeepHierarchicalBit;
  Bool      m_RCLCULevelRC;
  Bool      m_RCUseLCUSeparateModel;
  Int       m_RCInitialQP;
  Bool      m_RCForceIntraQP;
#if U0132_TARGET_BITS_SATURATION
  Bool      m_RCCpbSaturationEnabled;                   
  UInt      m_RCCpbSize;
  Double    m_RCInitialCpbFullness;
#endif

  Bool      m_TransquantBypassEnableFlag;                     ///< transquant_bypass_enable_flag setting in PPS.
  Bool      m_CUTransquantBypassFlagForce;                    ///< if transquant_bypass_enable_flag, then, if true, all CU transquant bypass flags will be set to true.

  CostMode  m_costMode;                                       ///< The cost function to use, primarily when considering lossless coding.
#if NH_MV
  TComVPS*  m_cVPS;                                           ///< pointer to VPS, same for all layers
#else

  TComVPS   m_cVPS;
#endif

  Bool      m_recalculateQPAccordingToLambda;                 ///< recalculate QP value according to the lambda value
  Int       m_activeParameterSetsSEIEnabled;                  ///< enable active parameter set SEI message
  Bool      m_vuiParametersPresentFlag;                       ///< enable generation of VUI parameters
  Bool      m_aspectRatioInfoPresentFlag;                     ///< Signals whether aspect_ratio_idc is present
  Bool      m_chromaResamplingFilterHintEnabled;              ///< Signals whether chroma sampling filter hint data is present
  Int       m_chromaResamplingHorFilterIdc;                   ///< Specifies the Index of filter to use
  Int       m_chromaResamplingVerFilterIdc;                   ///< Specifies the Index of filter to use
  Int       m_aspectRatioIdc;                                 ///< aspect_ratio_idc
  Int       m_sarWidth;                                       ///< horizontal size of the sample aspect ratio
  Int       m_sarHeight;                                      ///< vertical size of the sample aspect ratio
  Bool      m_overscanInfoPresentFlag;                        ///< Signals whether overscan_appropriate_flag is present
  Bool      m_overscanAppropriateFlag;                        ///< Indicates whether conformant decoded pictures are suitable for display using overscan
  Bool      m_videoSignalTypePresentFlag;                     ///< Signals whether video_format, video_full_range_flag, and colour_description_present_flag are present
  Int       m_videoFormat;                                    ///< Indicates representation of pictures
  Bool      m_videoFullRangeFlag;                             ///< Indicates the black level and range of luma and chroma signals
  Bool      m_colourDescriptionPresentFlag;                   ///< Signals whether colour_primaries, transfer_characteristics and matrix_coefficients are present
  Int       m_colourPrimaries;                                ///< Indicates chromaticity coordinates of the source primaries
  Int       m_transferCharacteristics;                        ///< Indicates the opto-electronic transfer characteristics of the source
  Int       m_matrixCoefficients;                             ///< Describes the matrix coefficients used in deriving luma and chroma from RGB primaries
  Bool      m_chromaLocInfoPresentFlag;                       ///< Signals whether chroma_sample_loc_type_top_field and chroma_sample_loc_type_bottom_field are present
  Int       m_chromaSampleLocTypeTopField;                    ///< Specifies the location of chroma samples for top field
  Int       m_chromaSampleLocTypeBottomField;                 ///< Specifies the location of chroma samples for bottom field
  Bool      m_neutralChromaIndicationFlag;                    ///< Indicates that the value of all decoded chroma samples is equal to 1<<(BitDepthCr-1)
  Window    m_defaultDisplayWindow;                           ///< Represents the default display window parameters
  Bool      m_frameFieldInfoPresentFlag;                      ///< Indicates that pic_struct and other field coding related values are present in picture timing SEI messages
  Bool      m_pocProportionalToTimingFlag;                    ///< Indicates that the POC value is proportional to the output time w.r.t. first picture in CVS
  Int       m_numTicksPocDiffOneMinus1;                       ///< Number of ticks minus 1 that for a POC difference of one
  Bool      m_bitstreamRestrictionFlag;                       ///< Signals whether bitstream restriction parameters are present
  Bool      m_tilesFixedStructureFlag;                        ///< Indicates that each active picture parameter set has the same values of the syntax elements related to tiles
  Bool      m_motionVectorsOverPicBoundariesFlag;             ///< Indicates that no samples outside the picture boundaries are used for inter prediction
  Int       m_minSpatialSegmentationIdc;                      ///< Indicates the maximum size of the spatial segments in the pictures in the coded video sequence
  Int       m_maxBytesPerPicDenom;                            ///< Indicates a number of bytes not exceeded by the sum of the sizes of the VCL NAL units associated with any coded picture
  Int       m_maxBitsPerMinCuDenom;                           ///< Indicates an upper bound for the number of bits of coding_unit() data
  Int       m_log2MaxMvLengthHorizontal;                      ///< Indicate the maximum absolute value of a decoded horizontal MV component in quarter-pel luma units
  Int       m_log2MaxMvLengthVertical;                        ///< Indicate the maximum absolute value of a decoded vertical MV component in quarter-pel luma units

  Bool      m_useStrongIntraSmoothing;                        ///< enable the use of strong intra smoothing (bi_linear interpolation) for 32x32 blocks when reference samples are flat.
  Bool      m_bEfficientFieldIRAPEnabled;                     ///< enable to code fields in a specific, potentially more efficient, order.
  Bool      m_bHarmonizeGopFirstFieldCoupleEnabled;

  std::string m_summaryOutFilename;                           ///< filename to use for producing summary output file.
  std::string m_summaryPicFilenameBase;                       ///< Base filename to use for producing summary picture output files. The actual filenames used will have I.txt, P.txt and B.txt appended.
  UInt        m_summaryVerboseness;                           ///< Specifies the level of the verboseness of the text output.


#if NH_MV
  Int       m_layerId;
  Int       m_layerIdInVps;
  Int       m_viewId;
  Int       m_viewIndex; 
#endif 

public:
  TEncCfg()
  : m_tileColumnWidth()
  , m_tileRowHeight()
#if NH_MV
  , m_layerId(-1)
  , m_layerIdInVps(-1)
  , m_viewId(-1)
  , m_viewIndex(-1)
#endif

  {
    m_PCMBitDepth[CHANNEL_TYPE_LUMA]=8;
    m_PCMBitDepth[CHANNEL_TYPE_CHROMA]=8;
  }

  virtual ~TEncCfg()
  {}

  Void setProfile(Profile::Name profile) { m_profile = profile; }
  Void setLevel(Level::Tier tier, Level::Name level) { m_levelTier = tier; m_level = level; }

  Void      setFrameRate                    ( Int   i )      { m_iFrameRate = i; }
  Void      setFrameSkip                    ( UInt i ) { m_FrameSkip = i; }
  Void      setSourceWidth                  ( Int   i )      { m_iSourceWidth = i; }
  Void      setSourceHeight                 ( Int   i )      { m_iSourceHeight = i; }

  Window   &getConformanceWindow()                           { return m_conformanceWindow; }
  Void      setConformanceWindow (Int confLeft, Int confRight, Int confTop, Int confBottom ) { m_conformanceWindow.setWindow (confLeft, confRight, confTop, confBottom); }

  Void      setFramesToBeEncoded            ( Int   i )      { m_framesToBeEncoded = i; }

  Bool      getPrintMSEBasedSequencePSNR    ()         const { return m_printMSEBasedSequencePSNR;  }
  Void      setPrintMSEBasedSequencePSNR    (Bool value)     { m_printMSEBasedSequencePSNR = value; }

  Bool      getPrintFrameMSE                ()         const { return m_printFrameMSE;              }
  Void      setPrintFrameMSE                (Bool value)     { m_printFrameMSE = value;             }

  Bool      getPrintSequenceMSE             ()         const { return m_printSequenceMSE;           }
  Void      setPrintSequenceMSE             (Bool value)     { m_printSequenceMSE = value;          }

  Bool      getCabacZeroWordPaddingEnabled()           const { return m_cabacZeroWordPaddingEnabled;  }
  Void      setCabacZeroWordPaddingEnabled(Bool value)       { m_cabacZeroWordPaddingEnabled = value; }
#if NH_MV
  Void      setLayerId                       ( Int layerId )      { m_layerId = layerId; }
  Int       getLayerId                       ()                   { return m_layerId;    }
  Int       getLayerIdInVps                  ()                   { return m_layerIdInVps; }
  Void      setLayerIdInVps                  ( Int layerIdInVps)  { m_layerIdInVps = layerIdInVps; }
  Void      setViewId                        ( Int viewId  )      { m_viewId  = viewId;  }
  Int       getViewId                        ()                   { return m_viewId;    }
  Void      setViewIndex                     ( Int viewIndex  )   { m_viewIndex  = viewIndex;  }
  Int       getViewIndex                     ()                   { return m_viewIndex;    }
#endif
  //====== Coding Structure ========
  Void      setIntraPeriod                  ( Int   i )      { m_uiIntraPeriod = (UInt)i; }
  Void      setDecodingRefreshType          ( Int   i )      { m_uiDecodingRefreshType = (UInt)i; }
  Void      setGOPSize                      ( Int   i )      { m_iGOPSize = i; }
#if NH_MV
  Void      setGopList                      ( GOPEntry*  GOPList ) {  for ( Int i = 0; i < MAX_GOP+1; i++ ) m_GOPList[i] = GOPList[i]; }
#else
  Void      setGopList                      ( GOPEntry*  GOPList ) {  for ( Int i = 0; i < MAX_GOP; i++ ) m_GOPList[i] = GOPList[i]; }
#endif
  Void      setExtraRPSs                    ( Int   i )      { m_extraRPSs = i; }
  GOPEntry  getGOPEntry                     ( Int   i )      { return m_GOPList[i]; }
  Void      setEncodedFlag                  ( Int  i, Bool value )  { m_GOPList[i].m_isEncoded = value; }
  Void      setMaxDecPicBuffering           ( UInt u, UInt tlayer ) { m_maxDecPicBuffering[tlayer] = u;    }
  Void      setNumReorderPics               ( Int  i, UInt tlayer ) { m_numReorderPics[tlayer] = i;    }

  Void      setQP                           ( Int   i )      { m_iQP = i; }

  Void      setPad                          ( Int*  iPad                   )      { for ( Int i = 0; i < 2; i++ ) m_aiPad[i] = iPad[i]; }

  Int       getMaxRefPicNum                 ()                              { return m_iMaxRefPicNum;           }
  Void      setMaxRefPicNum                 ( Int iMaxRefPicNum )           { m_iMaxRefPicNum = iMaxRefPicNum;  }

  Int       getMaxTempLayer                 ()                              { return m_maxTempLayer;              } 
  Void      setMaxTempLayer                 ( Int maxTempLayer )            { m_maxTempLayer = maxTempLayer;      }
  Void      setMaxCUWidth                   ( UInt  u )      { m_maxCUWidth  = u; }
  Void      setMaxCUHeight                  ( UInt  u )      { m_maxCUHeight = u; }
  Void      setMaxTotalCUDepth              ( UInt  u )      { m_maxTotalCUDepth = u; }
  Void      setLog2DiffMaxMinCodingBlockSize( UInt  u )      { m_log2DiffMaxMinCodingBlockSize = u; }

  //======== Transform =============
  Void      setQuadtreeTULog2MaxSize        ( UInt  u )      { m_uiQuadtreeTULog2MaxSize = u; }
  Void      setQuadtreeTULog2MinSize        ( UInt  u )      { m_uiQuadtreeTULog2MinSize = u; }
  Void      setQuadtreeTUMaxDepthInter      ( UInt  u )      { m_uiQuadtreeTUMaxDepthInter = u; }
  Void      setQuadtreeTUMaxDepthIntra      ( UInt  u )      { m_uiQuadtreeTUMaxDepthIntra = u; }

  Void setUseAMP( Bool b ) { m_useAMP = b; }

  //====== Loop/Deblock Filter ========
  Void      setLoopFilterDisable            ( Bool  b )      { m_bLoopFilterDisable       = b; }
  Void      setLoopFilterOffsetInPPS        ( Bool  b )      { m_loopFilterOffsetInPPS      = b; }
  Void      setLoopFilterBetaOffset         ( Int   i )      { m_loopFilterBetaOffsetDiv2  = i; }
  Void      setLoopFilterTcOffset           ( Int   i )      { m_loopFilterTcOffsetDiv2    = i; }
  Void      setDeblockingFilterMetric       ( Bool  b )      { m_DeblockingFilterMetric = b; }

  //====== Motion search ========
  Void      setDisableIntraPUsInInterSlices ( Bool  b )      { m_bDisableIntraPUsInInterSlices = b; }
  Void      setMotionEstimationSearchMethod ( MESearchMethod e ) { m_motionEstimationSearchMethod = e; }
  Void      setSearchRange                  ( Int   i )      { m_iSearchRange = i; }
  Void      setBipredSearchRange            ( Int   i )      { m_bipredSearchRange = i; }
  Void      setClipForBiPredMeEnabled       ( Bool  b )      { m_bClipForBiPredMeEnabled = b; }
  Void      setFastMEAssumingSmootherMVEnabled ( Bool b )    { m_bFastMEAssumingSmootherMVEnabled = b; }
  Void      setMinSearchWindow              ( Int   i )      { m_minSearchWindow = i; }
  Void      setRestrictMESampling           ( Bool  b )      { m_bRestrictMESampling = b; }

#if NH_MV
  Void      setUseDisparitySearchRangeRestriction ( Bool   b )      { m_bUseDisparitySearchRangeRestriction = b; }
  Void      setVerticalDisparitySearchRange ( Int   i )      { m_iVerticalDisparitySearchRange = i; }
#endif

  //====== Quality control ========
  Void      setMaxDeltaQP                   ( Int   i )      { m_iMaxDeltaQP = i; }
  Void      setMaxCuDQPDepth                ( Int   i )      { m_iMaxCuDQPDepth = i; }

  Int       getDiffCuChromaQpOffsetDepth    ()         const { return m_diffCuChromaQpOffsetDepth;  }
  Void      setDiffCuChromaQpOffsetDepth    (Int value)      { m_diffCuChromaQpOffsetDepth = value; }

  Void      setChromaCbQpOffset             ( Int   i )      { m_chromaCbQpOffset = i; }
  Void      setChromaCrQpOffset             ( Int   i )      { m_chromaCrQpOffset = i; }

  Void      setChromaFormatIdc              ( ChromaFormat cf ) { m_chromaFormatIDC = cf; }
  ChromaFormat  getChromaFormatIdc          ( )              { return m_chromaFormatIDC; }

#if ADAPTIVE_QP_SELECTION
  Void      setUseAdaptQpSelect             ( Bool   i ) { m_bUseAdaptQpSelect    = i; }
  Bool      getUseAdaptQpSelect             ()           { return   m_bUseAdaptQpSelect; }
#endif

  Bool      getExtendedPrecisionProcessingFlag         ()         const { return m_extendedPrecisionProcessingFlag;  }
  Void      setExtendedPrecisionProcessingFlag         (Bool value)     { m_extendedPrecisionProcessingFlag = value; }

  Bool      getHighPrecisionOffsetsEnabledFlag() const { return m_highPrecisionOffsetsEnabledFlag; }
  Void      setHighPrecisionOffsetsEnabledFlag(Bool value) { m_highPrecisionOffsetsEnabledFlag = value; }

  Void      setUseAdaptiveQP                ( Bool  b )      { m_bUseAdaptiveQP = b; }
  Void      setQPAdaptationRange            ( Int   i )      { m_iQPAdaptationRange = i; }

  //====== Sequence ========
  Int       getFrameRate                    ()      { return  m_iFrameRate; }
  UInt      getFrameSkip                    ()      { return  m_FrameSkip; }
  Int       getSourceWidth                  ()      { return  m_iSourceWidth; }
  Int       getSourceHeight                 ()      { return  m_iSourceHeight; }
  Int       getFramesToBeEncoded            ()      { return  m_framesToBeEncoded; }
  
  //====== Lambda Modifiers ========
  Void setLambdaModifier                    ( UInt uiIndex, Double dValue ) { m_adLambdaModifier[ uiIndex ] = dValue; }
  Double getLambdaModifier                  ( UInt uiIndex ) const { return m_adLambdaModifier[ uiIndex ]; }
  Void      setIntraLambdaModifier          ( const std::vector<Double> &dValue )               { m_adIntraLambdaModifier = dValue;       }
  const std::vector<Double>& getIntraLambdaModifier()                        const { return m_adIntraLambdaModifier;         }
  Void      setIntraQpFactor                ( Double dValue )               { m_dIntraQpFactor = dValue;              }
  Double    getIntraQpFactor                ()                        const { return m_dIntraQpFactor;                }

  //==== Coding Structure ========
  UInt      getIntraPeriod                  ()      { return  m_uiIntraPeriod; }
  UInt      getDecodingRefreshType          ()      { return  m_uiDecodingRefreshType; }
  Int       getGOPSize                      ()      { return  m_iGOPSize; }
  Int       getMaxDecPicBuffering           (UInt tlayer) { return m_maxDecPicBuffering[tlayer]; }
  Int       getNumReorderPics               (UInt tlayer) { return m_numReorderPics[tlayer]; }
  Int       getQP                           ()      { return  m_iQP; }

  Int       getPad                          ( Int i )      { assert (i < 2 );                      return  m_aiPad[i]; }

  Bool      getAccessUnitDelimiter() const  { return m_AccessUnitDelimiter; }
  Void      setAccessUnitDelimiter(Bool val){ m_AccessUnitDelimiter = val; }

  //======== Transform =============
  UInt      getQuadtreeTULog2MaxSize        ()      const { return m_uiQuadtreeTULog2MaxSize; }
  UInt      getQuadtreeTULog2MinSize        ()      const { return m_uiQuadtreeTULog2MinSize; }
  UInt      getQuadtreeTUMaxDepthInter      ()      const { return m_uiQuadtreeTUMaxDepthInter; }
  UInt      getQuadtreeTUMaxDepthIntra      ()      const { return m_uiQuadtreeTUMaxDepthIntra; }

  //==== Loop/Deblock Filter ========
  Bool      getLoopFilterDisable            ()      { return  m_bLoopFilterDisable;       }
  Bool      getLoopFilterOffsetInPPS        ()      { return m_loopFilterOffsetInPPS; }
  Int       getLoopFilterBetaOffset         ()      { return m_loopFilterBetaOffsetDiv2; }
  Int       getLoopFilterTcOffset           ()      { return m_loopFilterTcOffsetDiv2; }
  Bool      getDeblockingFilterMetric       ()      { return m_DeblockingFilterMetric; }

  //==== Motion search ========
  Bool      getDisableIntraPUsInInterSlices () const { return m_bDisableIntraPUsInInterSlices; }
  MESearchMethod getMotionEstimationSearchMethod ( ) const { return m_motionEstimationSearchMethod; }
  Int       getSearchRange                  () const { return m_iSearchRange; }
  Bool      getClipForBiPredMeEnabled       () const { return m_bClipForBiPredMeEnabled; }
  Bool      getFastMEAssumingSmootherMVEnabled ( ) const { return m_bFastMEAssumingSmootherMVEnabled; }
  Int       getMinSearchWindow                 () const { return m_minSearchWindow; }
  Bool      getRestrictMESampling              () const { return m_bRestrictMESampling; }

#if NH_MV
  Bool      getUseDisparitySearchRangeRestriction ()      { return  m_bUseDisparitySearchRangeRestriction; }
  Int       getVerticalDisparitySearchRange ()            { return  m_iVerticalDisparitySearchRange; }
#endif

  //==== Quality control ========
  Int       getMaxDeltaQP                   ()      { return  m_iMaxDeltaQP; }
  Int       getMaxCuDQPDepth                ()      { return  m_iMaxCuDQPDepth; }
  Bool      getUseAdaptiveQP                ()      { return  m_bUseAdaptiveQP; }
  Int       getQPAdaptationRange            ()      { return  m_iQPAdaptationRange; }

  //==== Tool list ========
  Void      setBitDepth( const ChannelType chType, Int internalBitDepthForChannel ) { m_bitDepth[chType] = internalBitDepthForChannel; }
  Void      setUseASR                       ( Bool  b )     { m_bUseASR     = b; }
  Void      setUseHADME                     ( Bool  b )     { m_bUseHADME   = b; }
  Void      setUseRDOQ                      ( Bool  b )     { m_useRDOQ    = b; }
  Void      setUseRDOQTS                    ( Bool  b )     { m_useRDOQTS  = b; }
#if T0196_SELECTIVE_RDOQ
  Void      setUseSelectiveRDOQ             ( Bool b )      { m_useSelectiveRDOQ = b; }
#endif
  Void      setRDpenalty                    ( UInt  u )     { m_rdPenalty  = u; }
  Void      setFastInterSearchMode          ( FastInterSearchMode m ) { m_fastInterSearchMode = m; }
  Void      setUseEarlyCU                   ( Bool  b )     { m_bUseEarlyCU = b; }
  Void      setUseFastDecisionForMerge      ( Bool  b )     { m_useFastDecisionForMerge = b; }
  Void      setUseCbfFastMode            ( Bool  b )     { m_bUseCbfFastMode = b; }
  Void      setUseEarlySkipDetection        ( Bool  b )     { m_useEarlySkipDetection = b; }
  Void      setUseConstrainedIntraPred      ( Bool  b )     { m_bUseConstrainedIntraPred = b; }
  Void      setFastUDIUseMPMEnabled         ( Bool  b )     { m_bFastUDIUseMPMEnabled = b; }
  Void      setFastMEForGenBLowDelayEnabled ( Bool  b )     { m_bFastMEForGenBLowDelayEnabled = b; }
  Void      setUseBLambdaForNonKeyLowDelayPictures ( Bool b ) { m_bUseBLambdaForNonKeyLowDelayPictures = b; }

  Void      setPCMInputBitDepthFlag         ( Bool  b )     { m_bPCMInputBitDepthFlag = b; }
  Void      setPCMFilterDisableFlag         ( Bool  b )     {  m_bPCMFilterDisableFlag = b; }
  Void      setUsePCM                       ( Bool  b )     {  m_usePCM = b;               }
  Void      setPCMBitDepth( const ChannelType chType, Int pcmBitDepthForChannel ) { m_PCMBitDepth[chType] = pcmBitDepthForChannel; }
  Void      setPCMLog2MaxSize               ( UInt u )      { m_pcmLog2MaxSize = u;      }
  Void      setPCMLog2MinSize               ( UInt u )     { m_uiPCMLog2MinSize = u;      }
  Void      setdQPs                         ( Int*  p )     { m_aidQP       = p; }
  Void      setDeltaQpRD                    ( UInt  u )     {m_uiDeltaQpRD  = u; }
  Void      setFastDeltaQp                  ( Bool  b )     {m_bFastDeltaQP = b; }
  Bool      getUseASR                       ()      { return m_bUseASR;     }
  Bool      getUseHADME                     ()      { return m_bUseHADME;   }
  Bool      getUseRDOQ                      ()      { return m_useRDOQ;    }
  Bool      getUseRDOQTS                    ()      { return m_useRDOQTS;  }
#if T0196_SELECTIVE_RDOQ
  Bool      getUseSelectiveRDOQ             ()      { return m_useSelectiveRDOQ; }
#endif
  Int       getRDpenalty                    ()      { return m_rdPenalty;  }
  FastInterSearchMode getFastInterSearchMode() const{ return m_fastInterSearchMode;  }
  Bool      getUseEarlyCU                   ()      { return m_bUseEarlyCU; }
  Bool      getUseFastDecisionForMerge      ()      { return m_useFastDecisionForMerge; }
  Bool      getUseCbfFastMode               ()      { return m_bUseCbfFastMode; }
  Bool      getUseEarlySkipDetection        ()      { return m_useEarlySkipDetection; }
  Bool      getUseConstrainedIntraPred      ()      { return m_bUseConstrainedIntraPred; }
  Bool      getFastUDIUseMPMEnabled         ()      { return m_bFastUDIUseMPMEnabled; }
  Bool      getFastMEForGenBLowDelayEnabled ()      { return m_bFastMEForGenBLowDelayEnabled; }
  Bool      getUseBLambdaForNonKeyLowDelayPictures () { return m_bUseBLambdaForNonKeyLowDelayPictures; }
  Bool      getPCMInputBitDepthFlag         ()      { return m_bPCMInputBitDepthFlag;   }
  Bool      getPCMFilterDisableFlag         ()      { return m_bPCMFilterDisableFlag;   }
  Bool      getUsePCM                       ()      { return m_usePCM;                 }
  UInt      getPCMLog2MaxSize               ()      { return m_pcmLog2MaxSize;  }
  UInt      getPCMLog2MinSize               ()      { return  m_uiPCMLog2MinSize;  }

  Bool      getCrossComponentPredictionEnabledFlag     ()                const { return m_crossComponentPredictionEnabledFlag;   }
  Void      setCrossComponentPredictionEnabledFlag     (const Bool value)      { m_crossComponentPredictionEnabledFlag = value;  }
  Bool      getUseReconBasedCrossCPredictionEstimate ()                const { return m_reconBasedCrossCPredictionEstimate;  }
  Void      setUseReconBasedCrossCPredictionEstimate (const Bool value)      { m_reconBasedCrossCPredictionEstimate = value; }
  Void      setLog2SaoOffsetScale(ChannelType type, UInt uiBitShift)         { m_log2SaoOffsetScale[type] = uiBitShift; }

  Bool getUseTransformSkip                             ()      { return m_useTransformSkip;        }
  Void setUseTransformSkip                             ( Bool b ) { m_useTransformSkip  = b;       }
  Bool getTransformSkipRotationEnabledFlag             ()            const { return m_transformSkipRotationEnabledFlag;  }
  Void setTransformSkipRotationEnabledFlag             (const Bool value)  { m_transformSkipRotationEnabledFlag = value; }
  Bool getTransformSkipContextEnabledFlag              ()            const { return m_transformSkipContextEnabledFlag;  }
  Void setTransformSkipContextEnabledFlag              (const Bool value)  { m_transformSkipContextEnabledFlag = value; }
  Bool getPersistentRiceAdaptationEnabledFlag          ()                 const { return m_persistentRiceAdaptationEnabledFlag;  }
  Void setPersistentRiceAdaptationEnabledFlag          (const Bool value)       { m_persistentRiceAdaptationEnabledFlag = value; }
  Bool getCabacBypassAlignmentEnabledFlag              ()       const      { return m_cabacBypassAlignmentEnabledFlag;  }
  Void setCabacBypassAlignmentEnabledFlag              (const Bool value)  { m_cabacBypassAlignmentEnabledFlag = value; }
  Bool getRdpcmEnabledFlag                             (const RDPCMSignallingMode signallingMode)        const      { return m_rdpcmEnabledFlag[signallingMode];  }
  Void setRdpcmEnabledFlag                             (const RDPCMSignallingMode signallingMode, const Bool value) { m_rdpcmEnabledFlag[signallingMode] = value; }
  Bool getUseTransformSkipFast                         ()      { return m_useTransformSkipFast;    }
  Void setUseTransformSkipFast                         ( Bool b ) { m_useTransformSkipFast  = b;   }
  UInt getLog2MaxTransformSkipBlockSize                () const      { return m_log2MaxTransformSkipBlockSize;     }
  Void setLog2MaxTransformSkipBlockSize                ( UInt u )    { m_log2MaxTransformSkipBlockSize  = u;       }
  Bool getIntraSmoothingDisabledFlag               ()      const { return m_intraSmoothingDisabledFlag; }
  Void setIntraSmoothingDisabledFlag               (Bool bValue) { m_intraSmoothingDisabledFlag=bValue; }

  Int*      getdQPs                         ()       { return m_aidQP;       }
  UInt      getDeltaQpRD                    () const { return m_uiDeltaQpRD; }
  Bool      getFastDeltaQp                  () const { return m_bFastDeltaQP; }

  //====== Slice ========
  Void  setSliceMode                   ( SliceConstraint  i )        { m_sliceMode = i;              }
  Void  setSliceArgument               ( Int  i )                    { m_sliceArgument = i;          }
  SliceConstraint getSliceMode         () const                      { return m_sliceMode;           }
  Int   getSliceArgument               ()                            { return m_sliceArgument;       }
  //====== Dependent Slice ========
  Void  setSliceSegmentMode            ( SliceConstraint  i )        { m_sliceSegmentMode = i;       }
  Void  setSliceSegmentArgument        ( Int  i )                    { m_sliceSegmentArgument = i;   }
  SliceConstraint getSliceSegmentMode  () const                      { return m_sliceSegmentMode;    }
  Int   getSliceSegmentArgument        ()                            { return m_sliceSegmentArgument;}
  Void      setLFCrossSliceBoundaryFlag     ( Bool   bValue  )       { m_bLFCrossSliceBoundaryFlag = bValue; }
  Bool      getLFCrossSliceBoundaryFlag     ()                       { return m_bLFCrossSliceBoundaryFlag;   }

  Void      setUseSAO                  (Bool bVal)                   { m_bUseSAO = bVal; }
  Bool      getUseSAO                  ()                            { return m_bUseSAO; }
  Void  setTestSAODisableAtPictureLevel (Bool bVal)                  { m_bTestSAODisableAtPictureLevel = bVal; }
  Bool  getTestSAODisableAtPictureLevel ( ) const                    { return m_bTestSAODisableAtPictureLevel; }

  Void   setSaoEncodingRate(Double v)                                { m_saoEncodingRate = v; }
  Double getSaoEncodingRate() const                                  { return m_saoEncodingRate; }
  Void   setSaoEncodingRateChroma(Double v)                          { m_saoEncodingRateChroma = v; }
  Double getSaoEncodingRateChroma() const                            { return m_saoEncodingRateChroma; }
  Void  setMaxNumOffsetsPerPic                   (Int iVal)          { m_maxNumOffsetsPerPic = iVal; }
  Int   getMaxNumOffsetsPerPic                   ()                  { return m_maxNumOffsetsPerPic; }
  Void  setSaoCtuBoundary              (Bool val)                    { m_saoCtuBoundary = val; }
  Bool  getSaoCtuBoundary              ()                            { return m_saoCtuBoundary; }
  Void  setLFCrossTileBoundaryFlag               ( Bool   val  )     { m_loopFilterAcrossTilesEnabledFlag = val; }
  Bool  getLFCrossTileBoundaryFlag               ()                  { return m_loopFilterAcrossTilesEnabledFlag;   }
  Void  setTileUniformSpacingFlag      ( Bool b )                    { m_tileUniformSpacingFlag = b; }
  Bool  getTileUniformSpacingFlag      ()                            { return m_tileUniformSpacingFlag; }
  Void  setNumColumnsMinus1            ( Int i )                     { m_iNumColumnsMinus1 = i; }
  Int   getNumColumnsMinus1            ()                            { return m_iNumColumnsMinus1; }
  Void  setColumnWidth ( const std::vector<Int>& columnWidth )       { m_tileColumnWidth = columnWidth; }
  UInt  getColumnWidth                 ( UInt columnIdx )            { return m_tileColumnWidth[columnIdx]; }
  Void  setNumRowsMinus1               ( Int i )                     { m_iNumRowsMinus1 = i; }
  Int   getNumRowsMinus1               ()                            { return m_iNumRowsMinus1; }
  Void  setRowHeight ( const std::vector<Int>& rowHeight)            { m_tileRowHeight = rowHeight; }
  UInt  getRowHeight                   ( UInt rowIdx )               { return m_tileRowHeight[rowIdx]; }
  Void  xCheckGSParameters();
  Void  setEntropyCodingSyncEnabledFlag(Bool b)                      { m_entropyCodingSyncEnabledFlag = b; }
  Bool  getEntropyCodingSyncEnabledFlag() const                      { return m_entropyCodingSyncEnabledFlag; }
  Void  setDecodedPictureHashSEIType(HashType m)                     { m_decodedPictureHashSEIType = m; }
  HashType getDecodedPictureHashSEIType() const                      { return m_decodedPictureHashSEIType; }
  Void  setBufferingPeriodSEIEnabled(Bool b)                         { m_bufferingPeriodSEIEnabled = b; }
  Bool  getBufferingPeriodSEIEnabled() const                         { return m_bufferingPeriodSEIEnabled; }
  Void  setPictureTimingSEIEnabled(Bool b)                           { m_pictureTimingSEIEnabled = b; }
  Bool  getPictureTimingSEIEnabled() const                           { return m_pictureTimingSEIEnabled; }
  Void  setRecoveryPointSEIEnabled(Bool b)                           { m_recoveryPointSEIEnabled = b; }
  Bool  getRecoveryPointSEIEnabled() const                           { return m_recoveryPointSEIEnabled; }
  Void  setToneMappingInfoSEIEnabled(Bool b)                         { m_toneMappingInfoSEIEnabled = b;  }
  Bool  getToneMappingInfoSEIEnabled()                               { return m_toneMappingInfoSEIEnabled;  }
  Void  setTMISEIToneMapId(Int b)                                    { m_toneMapId = b;  }
  Int   getTMISEIToneMapId()                                         { return m_toneMapId;  }
  Void  setTMISEIToneMapCancelFlag(Bool b)                           { m_toneMapCancelFlag=b;  }
  Bool  getTMISEIToneMapCancelFlag()                                 { return m_toneMapCancelFlag;  }
  Void  setTMISEIToneMapPersistenceFlag(Bool b)                      { m_toneMapPersistenceFlag = b;  }
  Bool   getTMISEIToneMapPersistenceFlag()                           { return m_toneMapPersistenceFlag;  }
  Void  setTMISEICodedDataBitDepth(Int b)                            { m_codedDataBitDepth = b;  }
  Int   getTMISEICodedDataBitDepth()                                 { return m_codedDataBitDepth;  }
  Void  setTMISEITargetBitDepth(Int b)                               { m_targetBitDepth = b;  }
  Int   getTMISEITargetBitDepth()                                    { return m_targetBitDepth;  }
  Void  setTMISEIModelID(Int b)                                      { m_modelId = b;  }
  Int   getTMISEIModelID()                                           { return m_modelId;  }
  Void  setTMISEIMinValue(Int b)                                     { m_minValue = b;  }
  Int   getTMISEIMinValue()                                          { return m_minValue;  }
  Void  setTMISEIMaxValue(Int b)                                     { m_maxValue = b;  }
  Int   getTMISEIMaxValue()                                          { return m_maxValue;  }
  Void  setTMISEISigmoidMidpoint(Int b)                              { m_sigmoidMidpoint = b;  }
  Int   getTMISEISigmoidMidpoint()                                   { return m_sigmoidMidpoint;  }
  Void  setTMISEISigmoidWidth(Int b)                                 { m_sigmoidWidth = b;  }
  Int   getTMISEISigmoidWidth()                                      { return m_sigmoidWidth;  }
  Void  setTMISEIStartOfCodedInterva( Int*  p )                      { m_startOfCodedInterval = p;  }
  Int*  getTMISEIStartOfCodedInterva()                               { return m_startOfCodedInterval;  }
  Void  setTMISEINumPivots(Int b)                                    { m_numPivots = b;  }
  Int   getTMISEINumPivots()                                         { return m_numPivots;  }
  Void  setTMISEICodedPivotValue( Int*  p )                          { m_codedPivotValue = p;  }
  Int*  getTMISEICodedPivotValue()                                   { return m_codedPivotValue;  }
  Void  setTMISEITargetPivotValue( Int*  p )                         { m_targetPivotValue = p;  }
  Int*  getTMISEITargetPivotValue()                                  { return m_targetPivotValue;  }
  Void  setTMISEICameraIsoSpeedIdc(Int b)                            { m_cameraIsoSpeedIdc = b;  }
  Int   getTMISEICameraIsoSpeedIdc()                                 { return m_cameraIsoSpeedIdc;  }
  Void  setTMISEICameraIsoSpeedValue(Int b)                          { m_cameraIsoSpeedValue = b;  }
  Int   getTMISEICameraIsoSpeedValue()                               { return m_cameraIsoSpeedValue;  }
  Void  setTMISEIExposureIndexIdc(Int b)                             { m_exposureIndexIdc = b;  }
  Int   getTMISEIExposurIndexIdc()                                   { return m_exposureIndexIdc;  }
  Void  setTMISEIExposureIndexValue(Int b)                           { m_exposureIndexValue = b;  }
  Int   getTMISEIExposurIndexValue()                                 { return m_exposureIndexValue;  }
  Void  setTMISEIExposureCompensationValueSignFlag(Bool b)           { m_exposureCompensationValueSignFlag = b;  }
  Bool  getTMISEIExposureCompensationValueSignFlag()                 { return m_exposureCompensationValueSignFlag;  }
  Void  setTMISEIExposureCompensationValueNumerator(Int b)           { m_exposureCompensationValueNumerator = b;  }
  Int   getTMISEIExposureCompensationValueNumerator()                { return m_exposureCompensationValueNumerator;  }
  Void  setTMISEIExposureCompensationValueDenomIdc(Int b)            { m_exposureCompensationValueDenomIdc =b;  }
  Int   getTMISEIExposureCompensationValueDenomIdc()                 { return m_exposureCompensationValueDenomIdc;  }
  Void  setTMISEIRefScreenLuminanceWhite(Int b)                      { m_refScreenLuminanceWhite = b;  }
  Int   getTMISEIRefScreenLuminanceWhite()                           { return m_refScreenLuminanceWhite;  }
  Void  setTMISEIExtendedRangeWhiteLevel(Int b)                      { m_extendedRangeWhiteLevel = b;  }
  Int   getTMISEIExtendedRangeWhiteLevel()                           { return m_extendedRangeWhiteLevel;  }
  Void  setTMISEINominalBlackLevelLumaCodeValue(Int b)               { m_nominalBlackLevelLumaCodeValue = b;  }
  Int   getTMISEINominalBlackLevelLumaCodeValue()                    { return m_nominalBlackLevelLumaCodeValue;  }
  Void  setTMISEINominalWhiteLevelLumaCodeValue(Int b)               { m_nominalWhiteLevelLumaCodeValue = b;  }
  Int   getTMISEINominalWhiteLevelLumaCodeValue()                    { return m_nominalWhiteLevelLumaCodeValue;  }
  Void  setTMISEIExtendedWhiteLevelLumaCodeValue(Int b)              { m_extendedWhiteLevelLumaCodeValue =b;  }
  Int   getTMISEIExtendedWhiteLevelLumaCodeValue()                   { return m_extendedWhiteLevelLumaCodeValue;  }
  Void  setFramePackingArrangementSEIEnabled(Bool b)                 { m_framePackingSEIEnabled = b; }
  Bool  getFramePackingArrangementSEIEnabled() const                 { return m_framePackingSEIEnabled; }
  Void  setFramePackingArrangementSEIType(Int b)                     { m_framePackingSEIType = b; }
  Int   getFramePackingArrangementSEIType()                          { return m_framePackingSEIType; }
  Void  setFramePackingArrangementSEIId(Int b)                       { m_framePackingSEIId = b; }
  Int   getFramePackingArrangementSEIId()                            { return m_framePackingSEIId; }
  Void  setFramePackingArrangementSEIQuincunx(Int b)                 { m_framePackingSEIQuincunx = b; }
  Int   getFramePackingArrangementSEIQuincunx()                      { return m_framePackingSEIQuincunx; }
  Void  setFramePackingArrangementSEIInterpretation(Int b)           { m_framePackingSEIInterpretation = b; }
  Int   getFramePackingArrangementSEIInterpretation()                { return m_framePackingSEIInterpretation; }
  Void  setSegmentedRectFramePackingArrangementSEIEnabled(Bool b)    { m_segmentedRectFramePackingSEIEnabled = b; }
  Bool  getSegmentedRectFramePackingArrangementSEIEnabled() const    { return m_segmentedRectFramePackingSEIEnabled; }
  Void  setSegmentedRectFramePackingArrangementSEICancel(Int b)      { m_segmentedRectFramePackingSEICancel = b; }
  Int   getSegmentedRectFramePackingArrangementSEICancel()           { return m_segmentedRectFramePackingSEICancel; }
  Void  setSegmentedRectFramePackingArrangementSEIType(Int b)        { m_segmentedRectFramePackingSEIType = b; }
  Int   getSegmentedRectFramePackingArrangementSEIType()             { return m_segmentedRectFramePackingSEIType; }
  Void  setSegmentedRectFramePackingArrangementSEIPersistence(Int b) { m_segmentedRectFramePackingSEIPersistence = b; }
  Int   getSegmentedRectFramePackingArrangementSEIPersistence()      { return m_segmentedRectFramePackingSEIPersistence; }
  Void  setDisplayOrientationSEIAngle(Int b)                         { m_displayOrientationSEIAngle = b; }
  Int   getDisplayOrientationSEIAngle()                              { return m_displayOrientationSEIAngle; }
  Void  setTemporalLevel0IndexSEIEnabled(Bool b)                     { m_temporalLevel0IndexSEIEnabled = b; }
  Bool  getTemporalLevel0IndexSEIEnabled() const                     { return m_temporalLevel0IndexSEIEnabled; }
  Void  setGradualDecodingRefreshInfoEnabled(Bool b)                 { m_gradualDecodingRefreshInfoEnabled = b;    }
  Bool  getGradualDecodingRefreshInfoEnabled() const                 { return m_gradualDecodingRefreshInfoEnabled; }
  Void  setNoDisplaySEITLayer(Int b)                                 { m_noDisplaySEITLayer = b;    }
  Int   getNoDisplaySEITLayer()                                      { return m_noDisplaySEITLayer; }
  Void  setDecodingUnitInfoSEIEnabled(Bool b)                        { m_decodingUnitInfoSEIEnabled = b;    }
  Bool  getDecodingUnitInfoSEIEnabled() const                        { return m_decodingUnitInfoSEIEnabled; }
  Void  setSOPDescriptionSEIEnabled(Bool b)                          { m_SOPDescriptionSEIEnabled = b; }
  Bool  getSOPDescriptionSEIEnabled() const                          { return m_SOPDescriptionSEIEnabled; }
  Void  setScalableNestingSEIEnabled(Bool b)                         { m_scalableNestingSEIEnabled = b; }
  Bool  getScalableNestingSEIEnabled() const                         { return m_scalableNestingSEIEnabled; }
  Void  setTMCTSSEIEnabled(Bool b)                                   { m_tmctsSEIEnabled = b; }
  Bool  getTMCTSSEIEnabled()                                         { return m_tmctsSEIEnabled; }
  Void  setTimeCodeSEIEnabled(Bool b)                                { m_timeCodeSEIEnabled = b; }
  Bool  getTimeCodeSEIEnabled()                                      { return m_timeCodeSEIEnabled; }
  Void  setNumberOfTimeSets(Int value)                               { m_timeCodeSEINumTs = value; }
  Int   getNumberOfTimesets()                                        { return m_timeCodeSEINumTs; }
  Void  setTimeSet(TComSEITimeSet element, Int index)                { m_timeSetArray[index] = element; }
  TComSEITimeSet &getTimeSet(Int index)                              { return m_timeSetArray[index]; }
  const TComSEITimeSet &getTimeSet(Int index) const                  { return m_timeSetArray[index]; }
  Void  setKneeSEIEnabled(Int b)                                     { m_kneeSEIEnabled = b; }
  Bool  getKneeSEIEnabled()                                          { return m_kneeSEIEnabled; }
  Void  setKneeSEIId(Int b)                                          { m_kneeSEIId = b; }
  Int   getKneeSEIId()                                               { return m_kneeSEIId; }
  Void  setKneeSEICancelFlag(Bool b)                                 { m_kneeSEICancelFlag=b; }
  Bool  getKneeSEICancelFlag()                                       { return m_kneeSEICancelFlag; }
  Void  setKneeSEIPersistenceFlag(Bool b)                            { m_kneeSEIPersistenceFlag = b; }
  Bool  getKneeSEIPersistenceFlag()                                  { return m_kneeSEIPersistenceFlag; }
  Void  setKneeSEIInputDrange(Int b)                                 { m_kneeSEIInputDrange = b; }
  Int   getKneeSEIInputDrange()                                      { return m_kneeSEIInputDrange; }
  Void  setKneeSEIInputDispLuminance(Int b)                          { m_kneeSEIInputDispLuminance = b; }
  Int   getKneeSEIInputDispLuminance()                               { return m_kneeSEIInputDispLuminance; }
  Void  setKneeSEIOutputDrange(Int b)                                { m_kneeSEIOutputDrange = b; }
  Int   getKneeSEIOutputDrange()                                     { return m_kneeSEIOutputDrange; }
  Void  setKneeSEIOutputDispLuminance(Int b)                         { m_kneeSEIOutputDispLuminance = b; }
  Int   getKneeSEIOutputDispLuminance()                              { return m_kneeSEIOutputDispLuminance; }
  Void  setKneeSEINumKneePointsMinus1(Int b)                         { m_kneeSEINumKneePointsMinus1 = b; }
  Int   getKneeSEINumKneePointsMinus1()                              { return m_kneeSEINumKneePointsMinus1; }
  Void  setKneeSEIInputKneePoint(Int *p)                             { m_kneeSEIInputKneePoint = p; }
  Int*  getKneeSEIInputKneePoint()                                   { return m_kneeSEIInputKneePoint; }
  Void  setKneeSEIOutputKneePoint(Int *p)                            { m_kneeSEIOutputKneePoint = p; }
  Int*  getKneeSEIOutputKneePoint()                                  { return m_kneeSEIOutputKneePoint; }
  Void  setColourRemapInfoSEIFileRoot( const std::string &s )        { m_colourRemapSEIFileRoot = s; }
  const std::string &getColourRemapInfoSEIFileRoot() const           { return m_colourRemapSEIFileRoot; }

  Void  setMasteringDisplaySEI(const TComSEIMasteringDisplay &src)   { m_masteringDisplay = src; }
  const TComSEIMasteringDisplay &getMasteringDisplaySEI() const      { return m_masteringDisplay; }
#if NH_MV
  Void setSeiMessages(SEIMessages *p)                                { m_seiMessages = p;    }
  const SEIMessages*  getSeiMessages()                               { return m_seiMessages; }
#endif
  Void         setUseWP               ( Bool b )                     { m_useWeightedPred   = b;    }
  Void         setWPBiPred            ( Bool b )                     { m_useWeightedBiPred = b;    }
  Bool         getUseWP               ()                             { return m_useWeightedPred;   }
  Bool         getWPBiPred            ()                             { return m_useWeightedBiPred; }
  Void         setLog2ParallelMergeLevelMinus2   ( UInt u )          { m_log2ParallelMergeLevelMinus2       = u;    }
  UInt         getLog2ParallelMergeLevelMinus2   ()                  { return m_log2ParallelMergeLevelMinus2;       }
  Void         setMaxNumMergeCand                ( UInt u )          { m_maxNumMergeCand = u;      }
  UInt         getMaxNumMergeCand                ()                  { return m_maxNumMergeCand;   }
  Void         setUseScalingListId    ( ScalingListMode u )          { m_useScalingListId       = u;   }
  ScalingListMode getUseScalingListId    ()                          { return m_useScalingListId;      }
  Void         setScalingListFileName       ( const std::string &s ) { m_scalingListFileName = s;      }
  const std::string& getScalingListFileName () const                 { return m_scalingListFileName;   }
  Void         setTMVPModeId ( Int  u )                              { m_TMVPModeId = u;    }
  Int          getTMVPModeId ()                                      { return m_TMVPModeId; }
  WeightedPredictionMethod getWeightedPredictionMethod() const       { return m_weightedPredictionMethod; }
  Void         setWeightedPredictionMethod( WeightedPredictionMethod m ) { m_weightedPredictionMethod = m; }
  Void         setSignHideFlag( Bool signHideFlag )                  { m_signHideFlag = signHideFlag; }
  Bool         getSignHideFlag()                                     { return m_signHideFlag; }
  Bool         getUseRateCtrl         ()                             { return m_RCEnableRateControl;   }
  Void         setUseRateCtrl         ( Bool b )                     { m_RCEnableRateControl = b;      }
  Int          getTargetBitrate       ()                             { return m_RCTargetBitrate;       }
  Void         setTargetBitrate       ( Int bitrate )                { m_RCTargetBitrate  = bitrate;   }
  Int          getKeepHierBit         ()                             { return m_RCKeepHierarchicalBit; }
  Void         setKeepHierBit         ( Int i )                      { m_RCKeepHierarchicalBit = i;    }
  Bool         getLCULevelRC          ()                             { return m_RCLCULevelRC; }
  Void         setLCULevelRC          ( Bool b )                     { m_RCLCULevelRC = b; }
  Bool         getUseLCUSeparateModel ()                             { return m_RCUseLCUSeparateModel; }
  Void         setUseLCUSeparateModel ( Bool b )                     { m_RCUseLCUSeparateModel = b;    }
  Int          getInitialQP           ()                             { return m_RCInitialQP;           }
  Void         setInitialQP           ( Int QP )                     { m_RCInitialQP = QP;             }
  Bool         getForceIntraQP        ()                             { return m_RCForceIntraQP;        }
  Void         setForceIntraQP        ( Bool b )                     { m_RCForceIntraQP = b;           }

#if U0132_TARGET_BITS_SATURATION
  Bool         getCpbSaturationEnabled()                             { return m_RCCpbSaturationEnabled;}
  Void         setCpbSaturationEnabled( Bool b )                     { m_RCCpbSaturationEnabled = b;   }
  UInt         getCpbSize             ()                             { return m_RCCpbSize;}
  Void         setCpbSize             ( UInt ui )                    { m_RCCpbSize = ui;   }
  Double       getInitialCpbFullness  ()                             { return m_RCInitialCpbFullness;  }
  Void         setInitialCpbFullness  (Double f)                     { m_RCInitialCpbFullness = f;     }
#endif

  Bool         getTransquantBypassEnableFlag()                       { return m_TransquantBypassEnableFlag; }
  Void         setTransquantBypassEnableFlag(Bool flag)              { m_TransquantBypassEnableFlag = flag; }
  Bool         getCUTransquantBypassFlagForceValue()                 { return m_CUTransquantBypassFlagForce; }
  Void         setCUTransquantBypassFlagForceValue(Bool flag)        { m_CUTransquantBypassFlagForce = flag; }
  CostMode     getCostMode( )                                        { return m_costMode; }
  Void         setCostMode(CostMode m )                              { m_costMode = m; }
#if NH_MV
  Void         setVPS( TComVPS *p )                                  { m_cVPS = p;    }
  TComVPS*     getVPS()                                              { return m_cVPS; }
#else
  Void         setVPS(TComVPS *p)                                    { m_cVPS = *p; }
  TComVPS *    getVPS()                                              { return &m_cVPS; }
#endif

  Void         setUseRecalculateQPAccordingToLambda (Bool b)         { m_recalculateQPAccordingToLambda = b;    }
  Bool         getUseRecalculateQPAccordingToLambda ()               { return m_recalculateQPAccordingToLambda; }

  Void         setUseStrongIntraSmoothing ( Bool b )                 { m_useStrongIntraSmoothing = b;    }
  Bool         getUseStrongIntraSmoothing ()                         { return m_useStrongIntraSmoothing; }

  Void         setEfficientFieldIRAPEnabled( Bool b )                { m_bEfficientFieldIRAPEnabled = b; }
  Bool         getEfficientFieldIRAPEnabled( ) const                 { return m_bEfficientFieldIRAPEnabled; }

  Void         setHarmonizeGopFirstFieldCoupleEnabled( Bool b )      { m_bHarmonizeGopFirstFieldCoupleEnabled = b; }
  Bool         getHarmonizeGopFirstFieldCoupleEnabled( ) const       { return m_bHarmonizeGopFirstFieldCoupleEnabled; }

  Void         setActiveParameterSetsSEIEnabled ( Int b )            { m_activeParameterSetsSEIEnabled = b; }
  Int          getActiveParameterSetsSEIEnabled ()                   { return m_activeParameterSetsSEIEnabled; }
  Bool         getVuiParametersPresentFlag()                         { return m_vuiParametersPresentFlag; }
  Void         setVuiParametersPresentFlag(Bool i)                   { m_vuiParametersPresentFlag = i; }
  Bool         getAspectRatioInfoPresentFlag()                       { return m_aspectRatioInfoPresentFlag; }
  Void         setAspectRatioInfoPresentFlag(Bool i)                 { m_aspectRatioInfoPresentFlag = i; }
  Int          getAspectRatioIdc()                                   { return m_aspectRatioIdc; }
  Void         setAspectRatioIdc(Int i)                              { m_aspectRatioIdc = i; }
  Int          getSarWidth()                                         { return m_sarWidth; }
  Void         setSarWidth(Int i)                                    { m_sarWidth = i; }
  Int          getSarHeight()                                        { return m_sarHeight; }
  Void         setSarHeight(Int i)                                   { m_sarHeight = i; }
  Bool         getOverscanInfoPresentFlag()                          { return m_overscanInfoPresentFlag; }
  Void         setOverscanInfoPresentFlag(Bool i)                    { m_overscanInfoPresentFlag = i; }
  Bool         getOverscanAppropriateFlag()                          { return m_overscanAppropriateFlag; }
  Void         setOverscanAppropriateFlag(Bool i)                    { m_overscanAppropriateFlag = i; }
  Bool         getVideoSignalTypePresentFlag()                       { return m_videoSignalTypePresentFlag; }
  Void         setVideoSignalTypePresentFlag(Bool i)                 { m_videoSignalTypePresentFlag = i; }
  Int          getVideoFormat()                                      { return m_videoFormat; }
  Void         setVideoFormat(Int i)                                 { m_videoFormat = i; }
  Bool         getVideoFullRangeFlag()                               { return m_videoFullRangeFlag; }
  Void         setVideoFullRangeFlag(Bool i)                         { m_videoFullRangeFlag = i; }
  Bool         getColourDescriptionPresentFlag()                     { return m_colourDescriptionPresentFlag; }
  Void         setColourDescriptionPresentFlag(Bool i)               { m_colourDescriptionPresentFlag = i; }
  Int          getColourPrimaries()                                  { return m_colourPrimaries; }
  Void         setColourPrimaries(Int i)                             { m_colourPrimaries = i; }
  Int          getTransferCharacteristics()                          { return m_transferCharacteristics; }
  Void         setTransferCharacteristics(Int i)                     { m_transferCharacteristics = i; }
  Int          getMatrixCoefficients()                               { return m_matrixCoefficients; }
  Void         setMatrixCoefficients(Int i)                          { m_matrixCoefficients = i; }
  Bool         getChromaLocInfoPresentFlag()                         { return m_chromaLocInfoPresentFlag; }
  Void         setChromaLocInfoPresentFlag(Bool i)                   { m_chromaLocInfoPresentFlag = i; }
  Int          getChromaSampleLocTypeTopField()                      { return m_chromaSampleLocTypeTopField; }
  Void         setChromaSampleLocTypeTopField(Int i)                 { m_chromaSampleLocTypeTopField = i; }
  Int          getChromaSampleLocTypeBottomField()                   { return m_chromaSampleLocTypeBottomField; }
  Void         setChromaSampleLocTypeBottomField(Int i)              { m_chromaSampleLocTypeBottomField = i; }
  Bool         getNeutralChromaIndicationFlag()                      { return m_neutralChromaIndicationFlag; }
  Void         setNeutralChromaIndicationFlag(Bool i)                { m_neutralChromaIndicationFlag = i; }
  Window      &getDefaultDisplayWindow()                             { return m_defaultDisplayWindow; }
  Void         setDefaultDisplayWindow (Int offsetLeft, Int offsetRight, Int offsetTop, Int offsetBottom ) { m_defaultDisplayWindow.setWindow (offsetLeft, offsetRight, offsetTop, offsetBottom); }
  Bool         getFrameFieldInfoPresentFlag()                        { return m_frameFieldInfoPresentFlag; }
  Void         setFrameFieldInfoPresentFlag(Bool i)                  { m_frameFieldInfoPresentFlag = i; }
  Bool         getPocProportionalToTimingFlag()                      { return m_pocProportionalToTimingFlag; }
  Void         setPocProportionalToTimingFlag(Bool x)                { m_pocProportionalToTimingFlag = x;    }
  Int          getNumTicksPocDiffOneMinus1()                         { return m_numTicksPocDiffOneMinus1;    }
  Void         setNumTicksPocDiffOneMinus1(Int x)                    { m_numTicksPocDiffOneMinus1 = x;       }
  Bool         getBitstreamRestrictionFlag()                         { return m_bitstreamRestrictionFlag; }
  Void         setBitstreamRestrictionFlag(Bool i)                   { m_bitstreamRestrictionFlag = i; }
  Bool         getTilesFixedStructureFlag()                          { return m_tilesFixedStructureFlag; }
  Void         setTilesFixedStructureFlag(Bool i)                    { m_tilesFixedStructureFlag = i; }
  Bool         getMotionVectorsOverPicBoundariesFlag()               { return m_motionVectorsOverPicBoundariesFlag; }
  Void         setMotionVectorsOverPicBoundariesFlag(Bool i)         { m_motionVectorsOverPicBoundariesFlag = i; }
  Int          getMinSpatialSegmentationIdc()                        { return m_minSpatialSegmentationIdc; }
  Void         setMinSpatialSegmentationIdc(Int i)                   { m_minSpatialSegmentationIdc = i; }
  Int          getMaxBytesPerPicDenom()                              { return m_maxBytesPerPicDenom; }
  Void         setMaxBytesPerPicDenom(Int i)                         { m_maxBytesPerPicDenom = i; }
  Int          getMaxBitsPerMinCuDenom()                             { return m_maxBitsPerMinCuDenom; }
  Void         setMaxBitsPerMinCuDenom(Int i)                        { m_maxBitsPerMinCuDenom = i; }
  Int          getLog2MaxMvLengthHorizontal()                        { return m_log2MaxMvLengthHorizontal; }
  Void         setLog2MaxMvLengthHorizontal(Int i)                   { m_log2MaxMvLengthHorizontal = i; }
  Int          getLog2MaxMvLengthVertical()                          { return m_log2MaxMvLengthVertical; }
  Void         setLog2MaxMvLengthVertical(Int i)                     { m_log2MaxMvLengthVertical = i; }

  Bool         getProgressiveSourceFlag() const                      { return m_progressiveSourceFlag; }
  Void         setProgressiveSourceFlag(Bool b)                      { m_progressiveSourceFlag = b; }

  Bool         getInterlacedSourceFlag() const                       { return m_interlacedSourceFlag; }
  Void         setInterlacedSourceFlag(Bool b)                       { m_interlacedSourceFlag = b; }

  Bool         getNonPackedConstraintFlag() const                    { return m_nonPackedConstraintFlag; }
  Void         setNonPackedConstraintFlag(Bool b)                    { m_nonPackedConstraintFlag = b; }

  Bool         getFrameOnlyConstraintFlag() const                    { return m_frameOnlyConstraintFlag; }
  Void         setFrameOnlyConstraintFlag(Bool b)                    { m_frameOnlyConstraintFlag = b; }

  UInt         getBitDepthConstraintValue() const                    { return m_bitDepthConstraintValue; }
  Void         setBitDepthConstraintValue(UInt v)                    { m_bitDepthConstraintValue=v; }

  ChromaFormat getChromaFormatConstraintValue() const                { return m_chromaFormatConstraintValue; }
  Void         setChromaFormatConstraintValue(ChromaFormat v)        { m_chromaFormatConstraintValue=v; }

  Bool         getIntraConstraintFlag() const                        { return m_intraConstraintFlag; }
  Void         setIntraConstraintFlag(Bool b)                        { m_intraConstraintFlag=b; }

  Bool         getOnePictureOnlyConstraintFlag() const               { return m_onePictureOnlyConstraintFlag; }
  Void         setOnePictureOnlyConstraintFlag(Bool b)               { m_onePictureOnlyConstraintFlag=b; }

  Bool         getLowerBitRateConstraintFlag() const                 { return m_lowerBitRateConstraintFlag; }
  Void         setLowerBitRateConstraintFlag(Bool b)                 { m_lowerBitRateConstraintFlag=b; }
  Bool         getChromaResamplingFilterHintEnabled()                { return m_chromaResamplingFilterHintEnabled;}
  Void         setChromaResamplingFilterHintEnabled(Bool i)          { m_chromaResamplingFilterHintEnabled = i;}
  Int          getChromaResamplingHorFilterIdc()                     { return m_chromaResamplingHorFilterIdc;}
  Void         setChromaResamplingHorFilterIdc(Int i)                { m_chromaResamplingHorFilterIdc = i;}
  Int          getChromaResamplingVerFilterIdc()                     { return m_chromaResamplingVerFilterIdc;}
  Void         setChromaResamplingVerFilterIdc(Int i)                { m_chromaResamplingVerFilterIdc = i;}

  Void      setSummaryOutFilename(const std::string &s)              { m_summaryOutFilename = s; }
  const std::string& getSummaryOutFilename() const                   { return m_summaryOutFilename; }
  Void      setSummaryPicFilenameBase(const std::string &s)          { m_summaryPicFilenameBase = s; }
  const std::string& getSummaryPicFilenameBase() const               { return m_summaryPicFilenameBase; }

  Void      setSummaryVerboseness(UInt v)                            { m_summaryVerboseness = v; }
  UInt      getSummaryVerboseness( ) const                           { return m_summaryVerboseness; }


};

//! \}

#endif // !defined(AFX_TENCCFG_H__6B99B797_F4DA_4E46_8E78_7656339A6C41__INCLUDED_)
