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

/** \file     TAppEncCfg.h
    \brief    Handle encoder configuration parameters (header)
*/

#ifndef __TAPPENCCFG__
#define __TAPPENCCFG__

#include "TLibCommon/CommonDef.h"

#include "TLibEncoder/TEncCfg.h"
#include <sstream>
#include <vector>
#if NH_3D
#include "TAppCommon/TAppComCamPara.h"
#include "TLibRenderer/TRenModel.h"
#include "TLibRenderer/TRenModSetupStrParser.h"
#endif
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
#if NH_MV
  std::vector<TChar*>     m_pchInputFileList;                  ///< source file names
#else
  std::string m_inputFileName;                                ///< source file name
#endif
  std::string m_bitstreamFileName;                            ///< output bitstream file
#if NH_MV
  std::vector<char*>     m_pchReconFileList;                  ///< output reconstruction file names
  Int                    m_numberOfLayers;                    ///< number of Layers to Encode
  Int                    m_iNumberOfViews;                    ///< number of Layers that are views
#else
  std::string m_reconFileName;                                ///< output reconstruction file
#endif
  #if NH_MV
// VPS specification
  IntAry2d m_dimIds;                   ///< dimension ids ( pointers to m_viewId and m_depthFlag 
  IntAry1d               m_viewId;                            ///< view id
  IntAry1d               m_viewOrderIndex;                    ///< view order index  
  IntAry1d               m_auxId;                             ///< auxiliary id
#if NH_3D
  IntAry1d               m_depthFlag;                         ///< depth flag
#endif
  IntAry1d               m_targetEncLayerIdList;              ///< layer Ids in Nuh to be encoded
  IntAry1d               m_layerIdInNuh;                      ///< layer Id in Nuh for each layer 
  Bool                   m_splittingFlag;                     ///< Splitting Flag
  Int                    m_scalabilityMask;                   ///< Mask indicating scalabilities, 1: texture; 3: texture + depth                                                                
  IntAry1d               m_dimensionIdLen;                    ///< Length of scalability dimension s 
  
// layer sets   
  Int                    m_vpsNumLayerSets;                   ///< Number of layer sets
  IntAry2d m_layerIdsInSets;           ///< LayerIds in vps of layer set 
  Int                    m_numAddLayerSets;                    ///< Number of additional layer sets
  IntAry2d m_highestLayerIdxPlus1;      ///< HighestLayerIdxPlus1 for each additional layer set and each independent layer (value with index 0 will be ignored)
  Int                    m_defaultOutputLayerIdc;             ///< Specifies output layers of layer sets, 0: output all layers, 1: output highest layers, 2: specified by LayerIdsInDefOuputLayerSet
  IntAry1d               m_outputLayerSetIdx;                 ///< Indices of layer sets used as additional output layer sets  
  IntAry2d m_layerIdsInAddOutputLayerSet; ///< LayerIds in vps of additional output layers
  IntAry2d m_layerIdsInDefOutputLayerSet; ///< Indices in vps of output layers in layer sets
  IntAry2d               m_profileTierLevelIdx;      ///< Indices of of profile, per layer in layer set
  BoolAry1d              m_altOutputLayerFlag;                ///< Alt output layer flag

  // Dependencies
  IntAry2d m_directRefLayers;          ///< LayerIds of direct reference layers
  IntAry2d m_dependencyTypes;          ///< Dependency types of direct reference layers

  // VPS VUI
  Bool m_vpsVuiPresentFlag;
  Bool m_crossLayerPicTypeAlignedFlag;
  Bool m_crossLayerIrapAlignedFlag;
  Bool m_allLayersIdrAlignedFlag;
  Bool m_bitRatePresentVpsFlag;
  Bool m_picRatePresentVpsFlag;
  BoolAry2d m_bitRatePresentFlag;
  BoolAry2d m_picRatePresentFlag;
  IntAry2d  m_avgBitRate;
  IntAry2d  m_maxBitRate;
  IntAry2d  m_constantPicRateIdc;
  IntAry2d  m_avgPicRate;
  Bool      m_tilesNotInUseFlag; 
  BoolAry1d m_tilesInUseFlag;
  BoolAry1d m_loopFilterNotAcrossTilesFlag; 
  Bool      m_wppNotInUseFlag;
  BoolAry1d m_wppInUseFlag;

  BoolAry2d m_tileBoundariesAlignedFlag;  
  Bool      m_ilpRestrictedRefLayersFlag;
  IntAry2d  m_minSpatialSegmentOffsetPlus1;
  BoolAry2d m_ctuBasedOffsetEnabledFlag;
  IntAry2d  m_minHorizontalCtuOffsetPlus1;
  Bool      m_singleLayerForNonIrapFlag;
  Bool      m_higherLayerIrapSkipFlag;
#if NH_3D
  Bool      m_abUseIC;
  Bool      m_bUseLowLatencyICEnc;
#endif
#endif
  Double    m_adLambdaModifier[ MAX_TLAYER ];                 ///< Lambda modifier array for each temporal layer
  std::vector<Double> m_adIntraLambdaModifier;                ///< Lambda modifier for Intra pictures, one for each temporal layer. If size>temporalLayer, then use [temporalLayer], else if size>0, use [size()-1], else use m_adLambdaModifier.
  Double    m_dIntraQpFactor;                                 ///< Intra Q Factor. If negative, use a default equation: 0.57*(1.0 - Clip3( 0.0, 0.5, 0.05*(Double)(isField ? (GopSize-1)/2 : GopSize-1) ))

  // source specification
  Int       m_iFrameRate;                                     ///< source frame-rates (Hz)
  UInt      m_FrameSkip;                                   ///< number of skipped frames from the beginning
  Int       m_iSourceWidth;                                   ///< source width in pixel
  Int       m_iSourceHeight;                                  ///< source height in pixel (when interlaced = field height)

  Int       m_iSourceHeightOrg;                               ///< original source height in pixel (when interlaced = frame height)

  Bool      m_isField;                                        ///< enable field coding
  Bool      m_isTopFieldFirst;
  Bool      m_bEfficientFieldIRAPEnabled;                     ///< enable an efficient field IRAP structure.
  Bool      m_bHarmonizeGopFirstFieldCoupleEnabled;

  Int       m_conformanceWindowMode;
  Int       m_confWinLeft;
  Int       m_confWinRight;
  Int       m_confWinTop;
  Int       m_confWinBottom;
  Int       m_framesToBeEncoded;                              ///< number of encoded frames
  Int       m_aiPad[2];                                       ///< number of padded pixels for width and height
  Bool      m_AccessUnitDelimiter;                            ///< add Access Unit Delimiter NAL units
  InputColourSpaceConversion m_inputColourSpaceConvert;       ///< colour space conversion to apply to input video
  Bool      m_snrInternalColourSpace;                       ///< if true, then no colour space conversion is applied for snr calculation, otherwise inverse of input is applied.
  Bool      m_outputInternalColourSpace;                    ///< if true, then no colour space conversion is applied for reconstructed video, otherwise inverse of input is applied.
  ChromaFormat m_InputChromaFormatIDC;

  Bool      m_printMSEBasedSequencePSNR;
  Bool      m_printFrameMSE;
  Bool      m_printSequenceMSE;
  Bool      m_cabacZeroWordPaddingEnabled;
  Bool      m_bClipInputVideoToRec709Range;
  Bool      m_bClipOutputVideoToRec709Range;

  // profile/level
#if NH_MV
  std::vector< Profile::Name > m_profiles;
  std::vector< Level::Tier   > m_levelTier;
  std::vector< Level::Name   > m_level;
  std::vector< Bool          > m_inblFlag; 
#else
  Profile::Name m_profile;
  Level::Tier   m_levelTier;
  Level::Name   m_level;
#endif
  UInt          m_bitDepthConstraint;
  ChromaFormat  m_chromaFormatConstraint;
  Bool          m_intraConstraintFlag;
  Bool          m_onePictureOnlyConstraintFlag;
  Bool          m_lowerBitRateConstraintFlag;
  Bool m_progressiveSourceFlag;
  Bool m_interlacedSourceFlag;
  Bool m_nonPackedConstraintFlag;
  Bool m_frameOnlyConstraintFlag;

  // coding structure
#if NH_MV
  IntAry1d  m_iIntraPeriod;                            ///< period of I-slice (random access period)
#else
  Int       m_iIntraPeriod;                                   ///< period of I-slice (random access period)
#endif
  Int       m_iDecodingRefreshType;                           ///< random access type
  Int       m_iGOPSize;                                       ///< GOP size of hierarchical structure
#if NH_MV
  Int       m_extraRPSsMvc[MAX_NUM_LAYERS];                       ///< extra RPSs added to handle CRA for each layer
  std::vector< GOPEntry* >  m_GOPListMvc;                            ///< the coding structure entries from the config file for each layer 
  Int       m_numReorderPicsMvc[MAX_NUM_LAYERS][MAX_TLAYER];      ///< total number of reorder pictures for each layer
  Int       m_maxDecPicBufferingMvc[MAX_NUM_LAYERS][MAX_TLAYER];  ///< total number of reference pictures needed for decoding for each layer
#else
  Int       m_extraRPSs;                                      ///< extra RPSs added to handle CRA
  GOPEntry  m_GOPList[MAX_GOP];                               ///< the coding structure entries from the config file
  Int       m_numReorderPics[MAX_TLAYER];                     ///< total number of reorder pictures
  Int       m_maxDecPicBuffering[MAX_TLAYER];                 ///< total number of pictures in the decoded picture buffer
  #endif
  Bool      m_crossComponentPredictionEnabledFlag;                          ///< flag enabling the use of cross-component prediction
  Bool      m_reconBasedCrossCPredictionEstimate;             ///< causes the alpha calculation in encoder search to be based on the decoded residual rather than the pre-transform encoder-side residual
#if NH_MV
  UInt      m_log2SaoOffsetScale[MAX_NUM_LAYERS][MAX_NUM_CHANNEL_TYPE];        ///< number of bits for the upward bit shift operation on the decoded SAO offsets
#else
  UInt      m_log2SaoOffsetScale[MAX_NUM_CHANNEL_TYPE];       ///< number of bits for the upward bit shift operation on the decoded SAO offsets
#endif
  Bool      m_useTransformSkip;                               ///< flag for enabling intra transform skipping
  Bool      m_useTransformSkipFast;                           ///< flag for enabling fast intra transform skipping
  UInt      m_log2MaxTransformSkipBlockSize;                  ///< transform-skip maximum size (minimum of 2)
  Bool      m_transformSkipRotationEnabledFlag;               ///< control flag for transform-skip/transquant-bypass residual rotation
  Bool      m_transformSkipContextEnabledFlag;                ///< control flag for transform-skip/transquant-bypass single significance map context
  Bool      m_rdpcmEnabledFlag[NUMBER_OF_RDPCM_SIGNALLING_MODES];///< control flags for residual DPCM
  Bool      m_enableAMP;
  Bool      m_persistentRiceAdaptationEnabledFlag;            ///< control flag for Golomb-Rice parameter adaptation over each slice
  Bool      m_cabacBypassAlignmentEnabledFlag;

  // coding quality
#if NH_MV
  std::vector<Double>  m_fQP;                                 ///< QP value of key-picture (floating point) for each layer
  std::vector<Int>     m_iQP;                                 ///< QP value of key-picture (integer) for each layer
#else
  Double    m_fQP;                                            ///< QP value of key-picture (floating point)
  Int       m_iQP;                                            ///< QP value of key-picture (integer)
#endif
  std::string m_dQPFileName;                                  ///< QP offset for each slice (initialized from external file)
#if NH_MV
  std::vector<Int*> m_aidQP;                                    ///< array of slice QP values for each layer
#else
  Int*      m_aidQP;                                          ///< array of slice QP values
#endif
  Int       m_iMaxDeltaQP;                                    ///< max. |delta QP|
  UInt      m_uiDeltaQpRD;                                    ///< dQP range for multi-pass slice QP optimization
  Int       m_iMaxCuDQPDepth;                                 ///< Max. depth for a minimum CuDQPSize (0:default)
  Int       m_diffCuChromaQpOffsetDepth;                      ///< If negative, then do not apply chroma qp offsets.
  Bool      m_bFastDeltaQP;                                   ///< Fast Delta QP (false:default)

  Int       m_cbQpOffset;                                     ///< Chroma Cb QP Offset (0:default)
  Int       m_crQpOffset;                                     ///< Chroma Cr QP Offset (0:default)

#if ADAPTIVE_QP_SELECTION
  Bool      m_bUseAdaptQpSelect;
#endif
  TComSEIMasteringDisplay m_masteringDisplay;
#if NH_MV_SEI
  std::vector<char*>     m_seiCfgFileNames;               ///< SEI message files.
  SEIMessages            m_seiMessages;                       ///< Buffer for SEI messages. 
#endif

  Bool      m_bUseAdaptiveQP;                                 ///< Flag for enabling QP adaptation based on a psycho-visual model
  Int       m_iQPAdaptationRange;                             ///< dQP range by QP adaptation

#if NH_MV
  Int       m_maxTempLayerMvc[MAX_NUM_LAYER_IDS];             ///< Max temporal layer for each layer
#else
  Int       m_maxTempLayer;                                  ///< Max temporal layer
#endif

  // coding unit (CU) definition
  // TODO: Remove MaxCUWidth/MaxCUHeight and replace with MaxCUSize.
  UInt      m_uiMaxCUWidth;                                   ///< max. CU width in pixel
  UInt      m_uiMaxCUHeight;                                  ///< max. CU height in pixel
  UInt      m_uiMaxCUDepth;                                   ///< max. CU depth (as specified by command line)
  UInt      m_uiMaxTotalCUDepth;                              ///< max. total CU depth - includes depth of transform-block structure
  UInt      m_uiLog2DiffMaxMinCodingBlockSize;                ///< difference between largest and smallest CU depth

  // transfom unit (TU) definition
  UInt      m_uiQuadtreeTULog2MaxSize;
  UInt      m_uiQuadtreeTULog2MinSize;

  UInt      m_uiQuadtreeTUMaxDepthInter;
  UInt      m_uiQuadtreeTUMaxDepthIntra;

  // coding tools (bit-depth)
  Int       m_inputBitDepth   [MAX_NUM_CHANNEL_TYPE];         ///< bit-depth of input file
  Int       m_outputBitDepth  [MAX_NUM_CHANNEL_TYPE];         ///< bit-depth of output file
  Int       m_MSBExtendedBitDepth[MAX_NUM_CHANNEL_TYPE];      ///< bit-depth of input samples after MSB extension
  Int       m_internalBitDepth[MAX_NUM_CHANNEL_TYPE];         ///< bit-depth codec operates at (input/output files will be converted)
  Bool      m_extendedPrecisionProcessingFlag;
  Bool      m_highPrecisionOffsetsEnabledFlag;

  //coding tools (chroma format)
  ChromaFormat m_chromaFormatIDC;

  // coding tools (PCM bit-depth)
  Bool      m_bPCMInputBitDepthFlag;                          ///< 0: PCM bit-depth is internal bit-depth. 1: PCM bit-depth is input bit-depth.

  // coding tool (SAO)
#if NH_MV
  std::vector<Bool> m_bUseSAO; 
#else
  Bool      m_bUseSAO;
#endif
  Bool      m_bTestSAODisableAtPictureLevel;
  Double    m_saoEncodingRate;                                ///< When >0 SAO early picture termination is enabled for luma and chroma
  Double    m_saoEncodingRateChroma;                          ///< The SAO early picture termination rate to use for chroma (when m_SaoEncodingRate is >0). If <=0, use results for luma.
  Int       m_maxNumOffsetsPerPic;                            ///< SAO maximun number of offset per picture
  Bool      m_saoCtuBoundary;                                 ///< SAO parameter estimation using non-deblocked pixels for CTU bottom and right boundary areas
  // coding tools (loop filter)
#if NH_MV
  std::vector<Bool> m_bLoopFilterDisable;                     ///< flag for using deblocking filter for each layer
#else
  Bool      m_bLoopFilterDisable;                             ///< flag for using deblocking filter
#endif
  Bool      m_loopFilterOffsetInPPS;                         ///< offset for deblocking filter in 0 = slice header, 1 = PPS
  Int       m_loopFilterBetaOffsetDiv2;                     ///< beta offset for deblocking filter
  Int       m_loopFilterTcOffsetDiv2;                       ///< tc offset for deblocking filter
  Bool      m_DeblockingFilterMetric;                         ///< blockiness metric in encoder

  // coding tools (PCM)
  Bool      m_usePCM;                                         ///< flag for using IPCM
  UInt      m_pcmLog2MaxSize;                                 ///< log2 of maximum PCM block size
  UInt      m_uiPCMLog2MinSize;                               ///< log2 of minimum PCM block size
  Bool      m_bPCMFilterDisableFlag;                          ///< PCM filter disable flag
  Bool      m_enableIntraReferenceSmoothing;                  ///< flag for enabling(default)/disabling intra reference smoothing/filtering

  // coding tools (encoder-only parameters)
  Bool      m_bUseASR;                                        ///< flag for using adaptive motion search range
  Bool      m_bUseHADME;                                      ///< flag for using HAD in sub-pel ME
  Bool      m_useRDOQ;                                       ///< flag for using RD optimized quantization
  Bool      m_useRDOQTS;                                     ///< flag for using RD optimized quantization for transform skip
#if T0196_SELECTIVE_RDOQ
  Bool      m_useSelectiveRDOQ;                               ///< flag for using selective RDOQ
#endif
  Int       m_rdPenalty;                                      ///< RD-penalty for 32x32 TU for intra in non-intra slices (0: no RD-penalty, 1: RD-penalty, 2: maximum RD-penalty)
  Bool      m_bDisableIntraPUsInInterSlices;                  ///< Flag for disabling intra predicted PUs in inter slices.
  MESearchMethod m_motionEstimationSearchMethod;
  Bool      m_bRestrictMESampling;                            ///< Restrict sampling for the Selective ME
  Int       m_iSearchRange;                                   ///< ME search range
  Int       m_bipredSearchRange;                              ///< ME search range for bipred refinement
  Int       m_minSearchWindow;                                ///< ME minimum search window size for the Adaptive Window ME
  Bool      m_bClipForBiPredMeEnabled;                        ///< Enables clipping for Bi-Pred ME.
  Bool      m_bFastMEAssumingSmootherMVEnabled;               ///< Enables fast ME assuming a smoother MV.
#if NH_MV
  Bool      m_bUseDisparitySearchRangeRestriction;            ///< restrict vertical search range for inter-view prediction
  Int       m_iVerticalDisparitySearchRange;                  ///< ME vertical search range for inter-view prediction
#endif
  FastInterSearchMode m_fastInterSearchMode;                  ///< Parameter that controls fast encoder settings
  Bool      m_bUseEarlyCU;                                    ///< flag for using Early CU setting
  Bool      m_useFastDecisionForMerge;                        ///< flag for using Fast Decision Merge RD-Cost
  Bool      m_bUseCbfFastMode;                              ///< flag for using Cbf Fast PU Mode Decision
  Bool      m_useEarlySkipDetection;                         ///< flag for using Early SKIP Detection
  SliceConstraint m_sliceMode;
  Int       m_sliceArgument;                                 ///< argument according to selected slice mode
  SliceConstraint m_sliceSegmentMode;
  Int       m_sliceSegmentArgument;                          ///< argument according to selected slice segment mode

  Bool      m_bLFCrossSliceBoundaryFlag;  ///< 1: filter across slice boundaries 0: do not filter across slice boundaries
  Bool      m_bLFCrossTileBoundaryFlag;   ///< 1: filter across tile boundaries  0: do not filter across tile boundaries
  Bool      m_tileUniformSpacingFlag;
  Int       m_numTileColumnsMinus1;
  Int       m_numTileRowsMinus1;
  std::vector<Int> m_tileColumnWidth;
  std::vector<Int> m_tileRowHeight;
  Bool      m_entropyCodingSyncEnabledFlag;

  Bool      m_bUseConstrainedIntraPred;                       ///< flag for using constrained intra prediction
  Bool      m_bFastUDIUseMPMEnabled;
  Bool      m_bFastMEForGenBLowDelayEnabled;
  Bool      m_bUseBLambdaForNonKeyLowDelayPictures;

  HashType  m_decodedPictureHashSEIType;                      ///< Checksum mode for decoded picture hash SEI message
  Bool      m_recoveryPointSEIEnabled;
  Bool      m_bufferingPeriodSEIEnabled;
  Bool      m_pictureTimingSEIEnabled;
  Bool      m_toneMappingInfoSEIEnabled;
  Bool      m_chromaResamplingFilterSEIenabled;
  Int       m_chromaResamplingHorFilterIdc;
  Int       m_chromaResamplingVerFilterIdc;
  Int       m_toneMapId;
  Bool      m_toneMapCancelFlag;
  Bool      m_toneMapPersistenceFlag;
  Int       m_toneMapCodedDataBitDepth;
  Int       m_toneMapTargetBitDepth;
  Int       m_toneMapModelId;
  Int       m_toneMapMinValue;
  Int       m_toneMapMaxValue;
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
  TComSEITimeSet m_timeSetArray[MAX_TIMECODE_SEI_SETS];
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
  // weighted prediction
  Bool      m_useWeightedPred;                    ///< Use of weighted prediction in P slices
  Bool      m_useWeightedBiPred;                  ///< Use of bi-directional weighted prediction in B slices
  WeightedPredictionMethod m_weightedPredictionMethod;

  UInt      m_log2ParallelMergeLevel;                         ///< Parallel merge estimation region
  UInt      m_maxNumMergeCand;                                ///< Max number of merge candidates

  Int       m_TMVPModeId;
  Bool      m_signHideFlag;
  Bool      m_RCEnableRateControl;                ///< enable rate control or not
  Int       m_RCTargetBitrate;                    ///< target bitrate when rate control is enabled
  Int       m_RCKeepHierarchicalBit;              ///< 0: equal bit allocation; 1: fixed ratio bit allocation; 2: adaptive ratio bit allocation
  Bool      m_RCLCULevelRC;                       ///< true: LCU level rate control; false: picture level rate control NOTE: code-tidy - rename to m_RCCtuLevelRC
  Bool      m_RCUseLCUSeparateModel;              ///< use separate R-lambda model at LCU level                        NOTE: code-tidy - rename to m_RCUseCtuSeparateModel
  Int       m_RCInitialQP;                        ///< inital QP for rate control
  Bool      m_RCForceIntraQP;                     ///< force all intra picture to use initial QP or not

#if U0132_TARGET_BITS_SATURATION
  Bool      m_RCCpbSaturationEnabled;             ///< enable target bits saturation to avoid CPB overflow and underflow
  UInt      m_RCCpbSize;                          ///< CPB size
  Double    m_RCInitialCpbFullness;               ///< initial CPB fullness 
#endif

#if KWU_RC_VIEWRC_E0227
  vector<Int>     m_viewTargetBits;
  Bool      m_viewWiseRateCtrl;                              ///< Flag for using view-wise rate control
#endif
#if KWU_RC_MADPRED_E0227
  UInt       m_depthMADPred;
#endif

  ScalingListMode m_useScalingListId;                         ///< using quantization matrix
  std::string m_scalingListFileName;                          ///< quantization matrix file name

  Bool      m_TransquantBypassEnableFlag;                     ///< transquant_bypass_enable_flag setting in PPS.
  Bool      m_CUTransquantBypassFlagForce;                    ///< if transquant_bypass_enable_flag, then, if true, all CU transquant bypass flags will be set to true.
  CostMode  m_costMode;                                       ///< Cost mode to use

  Bool      m_recalculateQPAccordingToLambda;                 ///< recalculate QP value according to the lambda value
  Bool      m_useStrongIntraSmoothing;                        ///< enable strong intra smoothing for 32x32 blocks where the reference samples are flat
  Int       m_activeParameterSetsSEIEnabled;

  Bool      m_vuiParametersPresentFlag;                       ///< enable generation of VUI parameters
  Bool      m_aspectRatioInfoPresentFlag;                     ///< Signals whether aspect_ratio_idc is present
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
  Bool      m_defaultDisplayWindowFlag;                       ///< Indicates the presence of the default window parameters
  Int       m_defDispWinLeftOffset;                           ///< Specifies the left offset from the conformance window of the default window
  Int       m_defDispWinRightOffset;                          ///< Specifies the right offset from the conformance window of the default window
  Int       m_defDispWinTopOffset;                            ///< Specifies the top offset from the conformance window of the default window
  Int       m_defDispWinBottomOffset;                         ///< Specifies the bottom offset from the conformance window of the default window
  Bool      m_frameFieldInfoPresentFlag;                      ///< Indicates that pic_struct values are present in picture timing SEI messages
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
  std::string m_colourRemapSEIFileRoot;

  std::string m_summaryOutFilename;                           ///< filename to use for producing summary output file.
  std::string m_summaryPicFilenameBase;                       ///< Base filename to use for producing summary picture output files. The actual filenames used will have I.txt, P.txt and B.txt appended.
  UInt        m_summaryVerboseness;                           ///< Specifies the level of the verboseness of the text output.
#if NH_MV
#if !NH_MV_SEI
  Bool              m_subBistreamPropSEIEnabled;
  Int               m_sbPropNumAdditionalSubStreams;
  IntAry1d          m_sbPropSubBitstreamMode;
  IntAry1d          m_sbPropOutputLayerSetIdxToVps;
  IntAry1d          m_sbPropHighestSublayerId;
  IntAry1d          m_sbPropAvgBitRate;
  IntAry1d          m_sbPropMaxBitRate;
#endif
  Bool              m_outputVpsInfo;
#endif
#if NH_3D
  // Output Format
  Bool      m_depth420OutputFlag;                             ///< Output depth layers in 4:2:0 format
  // Camera parameters
  TChar*    m_pchCameraParameterFile;                         ///< camera parameter file
  TChar*    m_pchBaseViewCameraNumbers;
  TAppComCamPara m_cCameraData;
  Int       m_iCodedCamParPrecision;                          ///< precision for coding of camera parameters
#if NH_3D_VSO
  TChar*    m_pchVSOConfig;
  Bool      m_bUseVSO;                                        ///< flag for using View Synthesis Optimization
  Bool      m_bVSOLSTable;                                    ///< Depth QP dependent Lagrange parameter optimization (m23714)
  Bool      m_bVSOEarlySkip;                                  ///< Early skip of VSO computation (JCT3V-A0093 modification 4)
                                                              
  //// Used for development by GT, might be removed later     
  Double    m_dLambdaScaleVSO;                                ///< Scaling factor for Lambda in VSO mode
  Bool      m_bForceLambdaScaleVSO;                           ///< Use Lambda Scale for depth even if VSO is turned off
  Bool      m_bAllowNegDist;                                  ///< Allow negative distortion in VSO
  UInt      m_uiVSOMode;                                      ///< Number of VSO Mode, 1 = , 2 = simple, org vs. ren, 3 = simple, ren vs. ren, 4 = full  
                                                              
  // SAIT_VSO_EST_A0033                                       
  Bool      m_bUseEstimatedVSD;                               ///< Flag for using model based VSD estimation instead of VSO for some encoder decisions (JCT3V-A0033 modification 3)  
                                                              
  // LGE_WVSO_A0119                                           
  Bool      m_bUseWVSO;                                       ///< flag for using View Synthesis Optimization  
  Int       m_iVSOWeight;
  Int       m_iVSDWeight;
  Int       m_iDWeight;

  // Ren Model String
  TRenModSetupStrParser       m_cRenModStrParser;
#endif

  Bool       m_useDLT;                                        ///< flag for using DLT
  Bool       m_bUseQTL;                                        ///< flag for using depth QuadTree Limitation
  BoolAry1d  m_ivMvPredFlag;
  BoolAry1d  m_ivMvScalingFlag;
  Int        m_log2SubPbSizeMinus3;
  Bool       m_ivResPredFlag;
  Bool       m_depthRefinementFlag;
  Bool       m_viewSynthesisPredFlag;
  Bool       m_depthBasedBlkPartFlag;
  Bool       m_mpiFlag;
  Int        m_log2MpiSubPbSizeMinus3;
  Bool       m_intraContourFlag;
  Bool       m_intraWedgeFlag;
  Bool       m_intraSdcFlag;
  Bool       m_qtPredFlag;
  Bool       m_interSdcFlag;
  Bool       m_depthIntraSkipFlag; 
#endif
  // internal member functions
  Void  xCheckParameter ();                                   ///< check validity of configuration values
  Void  xPrintParameter ();                                   ///< print configuration values
  Void  xPrintUsage     ();                                   ///< print usage
#if NH_MV

  template<typename T>
  Void xReadStrToEnum(string in, std::vector<T> &val)
  {
    val.clear(); 

    char* cElement = NULL;
    char* cString = new char[in.size()+1];
    strcpy(cString, in.c_str());

    cElement = strtok( cString, " " );
    while ( cElement != NULL )
    {
      T profile;       
      std::istringstream ss( cElement ); 
      ss >> profile; 
      val.push_back( profile ); 
      cElement = strtok( NULL, " " );
    }
    delete[] cString;
  }


  template <typename T>
  Void xResizeVector(  std::vector<T> & rpcVector )
  {
    for( Int layer = 0; rpcVector.size() < m_numberOfLayers; layer++ )
    {
      assert( rpcVector.size() > 0 );
      rpcVector.push_back( rpcVector[layer] );      
    }

    for( ; rpcVector.size() > m_numberOfLayers; )
    {      
      rpcVector.pop_back( );      
    }
  }

  template <typename T>
  Void xResizeVector(  std::vector<T> & rpcVector, UInt n )
  {
    for( Int layer = 0; rpcVector.size() < n; layer++ )
    {
      assert( rpcVector.size() > 0 );
      rpcVector.push_back( rpcVector[layer] );      
    }

    for( ; rpcVector.size() > n; )
    {      
      rpcVector.pop_back( );      
    }
  }


  template <typename T>
  Void xPrintParaVector( std::string description, std::vector<T> & rpcVector )
  {
    Int iSpace = max(1, ENC_CFG_CONSOUT_SPACE - (Int) description.length() ); 
    
    for ( Int i = 0; i < iSpace; i++ )
    {
      description.append( " " ); 
    }
      
    description.append( ":" ); 
    printf( "%s", description.c_str() ); 

    for(Int i=0;i<rpcVector.size();i++)
    {
      xPrintVectorElem( rpcVector[i] );
    }

    printf("\n");
  }
  
  Void xPrintVectorElem( UInt   elem ) { printf(" %d"   , elem            );};
  Void xPrintVectorElem( Int    elem ) { printf(" %d"   , elem            );};
  
  Void xPrintVectorElem( Double elem ) { printf(" %5.2f", elem            );};  
  Void xPrintVectorElem( Bool   elem ) { printf(" %d"   , ( elem ? 1 : 0 ));};
#if NH_MV_SEI
  Void xParseSeiCfg();
#endif
#endif
#if NH_MV
  Int   getGOPSize() { return m_iGOPSize; }
#endif
public:
  TAppEncCfg();
  virtual ~TAppEncCfg();

public:
  Void  create    ();                                         ///< create option handling class
  Void  destroy   ();                                         ///< destroy option handling class
  Bool  parseCfg  ( Int argc, TChar* argv[] );                ///< parse configuration file to fill member variables

};// END CLASS DEFINITION TAppEncCfg

//! \}

#endif // __TAPPENCCFG__

