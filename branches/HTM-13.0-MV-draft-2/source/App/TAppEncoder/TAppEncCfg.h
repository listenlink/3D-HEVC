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

/** \file     TAppEncCfg.h
    \brief    Handle encoder configuration parameters (header)
*/

#ifndef __TAPPENCCFG__
#define __TAPPENCCFG__

#include "TLibCommon/CommonDef.h"

#include "TLibEncoder/TEncCfg.h"
#include <sstream>
#include <vector>
#if H_3D
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
#if H_MV
  std::vector<char*>     m_pchInputFileList;                  ///< source file names
#else
  Char*     m_pchInputFile;                                   ///< source file name
#endif
  Char*     m_pchBitstreamFile;                               ///< output bitstream file
#if H_MV
  std::vector<char*>     m_pchReconFileList;                  ///< output reconstruction file names
  Int                    m_numberOfLayers;                    ///< number of Layers to Encode
  Int                    m_iNumberOfViews;                    ///< number of Layers that are views
#else
  Char*     m_pchReconFile;                                   ///< output reconstruction file
#endif
  #if H_MV
// VPS specification
  std::vector< std::vector<Int> > m_dimIds;                   ///< dimension ids ( pointers to m_viewId and m_depthFlag 
  std::vector<Int>       m_viewId;                            ///< view id
  std::vector<Int>       m_viewOrderIndex;                    ///< view order index  
  std::vector<Int>       m_auxId;                             ///< auxiliary id
#if H_3D
  std::vector<Int>       m_depthFlag;                         ///< depth flag
#endif
  std::vector<Int>       m_targetEncLayerIdList;              ///< layer Ids in Nuh to be encoded
  std::vector<Int>       m_layerIdInNuh;                      ///< layer Id in Nuh for each layer 
  Bool                   m_splittingFlag;                     ///< Splitting Flag
  Int                    m_scalabilityMask;                   ///< Mask indicating scalabilities, 1: texture; 3: texture + depth                                                                
  std::vector<Int>       m_dimensionIdLen;                    ///< Length of scalability dimension s 
  
// layer sets   
  Int                    m_vpsNumLayerSets;                   ///< Number of layer sets
  std::vector< std::vector<Int> > m_layerIdsInSets;           ///< LayerIds in vps of layer set 
  Int                    m_numAddLayerSets;                    ///< Number of additional layer sets
  std::vector< std::vector<Int> > m_highestLayerIdxPlus1;      ///< HighestLayerIdxPlus1 for each additional layer set and each independent layer (value with index 0 will be ignored)
  Int                    m_defaultOutputLayerIdc;             ///< Specifies output layers of layer sets, 0: output all layers, 1: output highest layers, 2: specified by LayerIdsInDefOuputLayerSet
  std::vector<Int>       m_outputLayerSetIdx;                 ///< Indices of layer sets used as additional output layer sets  
  std::vector< std::vector<Int> > m_layerIdsInAddOutputLayerSet; ///< LayerIds in vps of additional output layers
  std::vector< std::vector<Int> > m_layerIdsInDefOutputLayerSet; ///< Indices in vps of output layers in layer sets
  std::vector< std::vector< Int > > m_profileTierLevelIdx;      ///< Indices of of profile, per layer in layer set
  std::vector<Bool>      m_altOutputLayerFlag;                ///< Alt output layer flag

  // Dependencies
  std::vector< std::vector<Int> > m_directRefLayers;          ///< LayerIds of direct reference layers
  std::vector< std::vector<Int> > m_dependencyTypes;          ///< Dependency types of direct reference layers

  // VPS VUI
  Bool m_vpsVuiPresentFlag;
  Bool m_crossLayerPicTypeAlignedFlag;
  Bool m_crossLayerIrapAlignedFlag;
  Bool m_allLayersIdrAlignedFlag;
  Bool m_bitRatePresentVpsFlag;
  Bool m_picRatePresentVpsFlag;
  std::vector< std::vector<Bool > > m_bitRatePresentFlag;
  std::vector< std::vector<Bool > > m_picRatePresentFlag;
  std::vector< std::vector<Int  > > m_avgBitRate;
  std::vector< std::vector<Int  > > m_maxBitRate;
  std::vector< std::vector<Int  > > m_constantPicRateIdc;
  std::vector< std::vector<Int  > > m_avgPicRate;
  Bool                              m_tilesNotInUseFlag; 
  std::vector< Bool >               m_tilesInUseFlag;
  std::vector< Bool >               m_loopFilterNotAcrossTilesFlag; 
  Bool                              m_wppNotInUseFlag;
  std::vector< Bool >               m_wppInUseFlag;

  std::vector< std::vector<Bool > > m_tileBoundariesAlignedFlag;  
  Bool m_ilpRestrictedRefLayersFlag;
  std::vector< std::vector<Int  > > m_minSpatialSegmentOffsetPlus1;
  std::vector< std::vector<Bool > > m_ctuBasedOffsetEnabledFlag;
  std::vector< std::vector<Int  > > m_minHorizontalCtuOffsetPlus1;
  Bool m_singleLayerForNonIrapFlag;
  Bool m_higherLayerIrapSkipFlag;

#if !HHI_TOOL_PARAMETERS_I2_J0107
#if H_3D_IV_MERGE
  vector<Bool>           m_ivMvPredFlag;                      ///< Interview motion vector prediction 

#if H_3D_SPIVMP
  Int                    m_iSubPULog2Size;                    
  Int                    m_iSubPUMPILog2Size;                    
#endif
#endif
#if H_3D_QTLPC
  Bool                   m_bLimQtPredFlag;
#endif
#if H_3D_ARP                                                  /// < flag and number of weighting factors in ARP
  UInt                   m_uiUseAdvResPred;
  UInt                   m_uiARPStepNum;
#endif
#endif

#if H_3D_IC
  Bool   m_abUseIC;
  Bool   m_bUseLowLatencyICEnc;
#endif

#if !HHI_TOOL_PARAMETERS_I2_J0107
#if H_3D_NBDV_REF
  Bool m_depthRefinementFlag;  
#endif
#if H_3D_VSP
  Bool m_viewSynthesisPredFlag;
#endif
#if H_3D
  Bool m_ivMvScalingFlag; 
#endif
#endif
#endif
  Double    m_adLambdaModifier[ MAX_TLAYER ];                 ///< Lambda modifier array for each temporal layer
  // source specification
  Int       m_iFrameRate;                                     ///< source frame-rates (Hz)
  UInt      m_FrameSkip;                                      ///< number of skipped frames from the beginning
  Int       m_iSourceWidth;                                   ///< source width in pixel
  Int       m_iSourceHeight;                                  ///< source height in pixel (when interlaced = field height)
  
  Int       m_iSourceHeightOrg;                               ///< original source height in pixel (when interlaced = frame height)
  
  bool      m_isField;                                        ///< enable field coding
  bool      m_isTopFieldFirst;
  
  Int       m_conformanceWindowMode;
  Int       m_confWinLeft;
  Int       m_confWinRight;
  Int       m_confWinTop;
  Int       m_confWinBottom;
  Int       m_framesToBeEncoded;                              ///< number of encoded frames
  Int       m_aiPad[2];                                       ///< number of padded pixels for width and height
  
  // profile/level
#if H_MV
  std::vector< Profile::Name > m_profile;
  std::vector< Level::Tier   > m_levelTier;
  std::vector< Level::Name   > m_level;
  std::vector< Bool          > m_inblFlag; 
#else
  Profile::Name m_profile;
  Level::Tier   m_levelTier;
  Level::Name   m_level;
#endif

  Bool m_progressiveSourceFlag;
  Bool m_interlacedSourceFlag;
  Bool m_nonPackedConstraintFlag;
  Bool m_frameOnlyConstraintFlag;
  // coding structure
#if H_MV
  std::vector<Int> m_iIntraPeriod;                            ///< period of I-slice (random access period)
#else
  Int       m_iIntraPeriod;                                   ///< period of I-slice (random access period)
#endif
  Int       m_iDecodingRefreshType;                           ///< random access type
  Int       m_iGOPSize;                                       ///< GOP size of hierarchical structure
#if H_MV
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
  Bool      m_useTransformSkip;                               ///< flag for enabling intra transform skipping
  Bool      m_useTransformSkipFast;                           ///< flag for enabling fast intra transform skipping
  Bool      m_enableAMP;
  // coding quality
#if H_MV
  std::vector<Double>  m_fQP;                                 ///< QP value of key-picture (floating point) for each layer
  std::vector<Int>     m_iQP;                                 ///< QP value of key-picture (integer) for each layer
#else
  Double    m_fQP;                                            ///< QP value of key-picture (floating point)
  Int       m_iQP;                                            ///< QP value of key-picture (integer)
#endif
  Char*     m_pchdQPFile;                                     ///< QP offset for each slice (initialized from external file)
#if H_MV
  std::vector<Int*> m_aidQP;                                    ///< array of slice QP values for each layer
#else
  Int*      m_aidQP;                                          ///< array of slice QP values
#endif
  Int       m_iMaxDeltaQP;                                    ///< max. |delta QP|
  UInt      m_uiDeltaQpRD;                                    ///< dQP range for multi-pass slice QP optimization
  Int       m_iMaxCuDQPDepth;                                 ///< Max. depth for a minimum CuDQPSize (0:default)

  Int       m_cbQpOffset;                                     ///< Chroma Cb QP Offset (0:default) 
  Int       m_crQpOffset;                                     ///< Chroma Cr QP Offset (0:default)

#if ADAPTIVE_QP_SELECTION
  Bool      m_bUseAdaptQpSelect;
#endif

  Bool      m_bUseAdaptiveQP;                                 ///< Flag for enabling QP adaptation based on a psycho-visual model
  Int       m_iQPAdaptationRange;                             ///< dQP range by QP adaptation
  
#if H_MV
  Int       m_maxTempLayerMvc[MAX_NUM_LAYER_IDS];             ///< Max temporal layer for each layer
#else
  Int       m_maxTempLayer;                                  ///< Max temporal layer
#endif

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
  Int       m_inputBitDepthY;                               ///< bit-depth of input file (luma component)
  Int       m_inputBitDepthC;                               ///< bit-depth of input file (chroma component)
  Int       m_outputBitDepthY;                              ///< bit-depth of output file (luma component)
  Int       m_outputBitDepthC;                              ///< bit-depth of output file (chroma component)
  Int       m_internalBitDepthY;                            ///< bit-depth codec operates at in luma (input/output files will be converted)
  Int       m_internalBitDepthC;                            ///< bit-depth codec operates at in chroma (input/output files will be converted)

  // coding tools (PCM bit-depth)
  Bool      m_bPCMInputBitDepthFlag;                          ///< 0: PCM bit-depth is internal bit-depth. 1: PCM bit-depth is input bit-depth.

  // coding tool (SAO)
#if H_MV
  std::vector<Bool> m_bUseSAO; 
#else
  Bool      m_bUseSAO; 
#endif
  Int       m_maxNumOffsetsPerPic;                            ///< SAO maximun number of offset per picture
  Bool      m_saoLcuBoundary;                                 ///< SAO parameter estimation using non-deblocked pixels for LCU bottom and right boundary areas
  // coding tools (loop filter)
#if H_MV
  std::vector<Bool> m_bLoopFilterDisable;                     ///< flag for using deblocking filter for each layer
#else
  Bool      m_bLoopFilterDisable;                             ///< flag for using deblocking filter
#endif
  Bool      m_loopFilterOffsetInPPS;                         ///< offset for deblocking filter in 0 = slice header, 1 = PPS
  Int       m_loopFilterBetaOffsetDiv2;                     ///< beta offset for deblocking filter
  Int       m_loopFilterTcOffsetDiv2;                       ///< tc offset for deblocking filter
  Bool      m_DeblockingFilterControlPresent;                 ///< deblocking filter control present flag in PPS
  Bool      m_DeblockingFilterMetric;                         ///< blockiness metric in encoder
 
  // coding tools (PCM)
  Bool      m_usePCM;                                         ///< flag for using IPCM
  UInt      m_pcmLog2MaxSize;                                 ///< log2 of maximum PCM block size
  UInt      m_uiPCMLog2MinSize;                               ///< log2 of minimum PCM block size
  Bool      m_bPCMFilterDisableFlag;                          ///< PCM filter disable flag

  // coding tools (encoder-only parameters)
  Bool      m_bUseASR;                                        ///< flag for using adaptive motion search range
  Bool      m_bUseHADME;                                      ///< flag for using HAD in sub-pel ME
  Bool      m_useRDOQ;                                       ///< flag for using RD optimized quantization
  Bool      m_useRDOQTS;                                     ///< flag for using RD optimized quantization for transform skip
  Int      m_rdPenalty;                                      ///< RD-penalty for 32x32 TU for intra in non-intra slices (0: no RD-penalty, 1: RD-penalty, 2: maximum RD-penalty) 
  Int       m_iFastSearch;                                    ///< ME mode, 0 = full, 1 = diamond, 2 = PMVFAST
  Int       m_iSearchRange;                                   ///< ME search range
  Int       m_bipredSearchRange;                              ///< ME search range for bipred refinement
  Bool      m_bUseFastEnc;                                    ///< flag for using fast encoder setting
  Bool      m_bUseEarlyCU;                                    ///< flag for using Early CU setting
  Bool      m_useFastDecisionForMerge;                        ///< flag for using Fast Decision Merge RD-Cost 
  Bool      m_bUseCbfFastMode;                              ///< flag for using Cbf Fast PU Mode Decision
  Bool      m_useEarlySkipDetection;                         ///< flag for using Early SKIP Detection
  Int       m_sliceMode;                                     ///< 0: no slice limits, 1 : max number of CTBs per slice, 2: max number of bytes per slice, 
                                                             ///< 3: max number of tiles per slice
  Int       m_sliceArgument;                                 ///< argument according to selected slice mode
  Int       m_sliceSegmentMode;                              ///< 0: no slice segment limits, 1 : max number of CTBs per slice segment, 2: max number of bytes per slice segment, 
                                                             ///< 3: max number of tiles per slice segment
  Int       m_sliceSegmentArgument;                          ///< argument according to selected slice segment mode

  Bool      m_bLFCrossSliceBoundaryFlag;  ///< 1: filter across slice boundaries 0: do not filter across slice boundaries
  Bool      m_bLFCrossTileBoundaryFlag;   ///< 1: filter across tile boundaries  0: do not filter across tile boundaries
  Bool      m_tileUniformSpacingFlag;
  Int       m_numTileColumnsMinus1;
  Int       m_numTileRowsMinus1;
  std::vector<Int> m_tileColumnWidth;
  std::vector<Int> m_tileRowHeight;
  Int       m_iWaveFrontSynchro; //< 0: no WPP. >= 1: WPP is enabled, the "Top right" from which inheritance occurs is this LCU offset in the line above the current.
  Int       m_iWaveFrontSubstreams; //< If iWaveFrontSynchro, this is the number of substreams per frame (dependent tiles) or per tile (independent tiles).

  Bool      m_bUseConstrainedIntraPred;                       ///< flag for using constrained intra prediction
  
  Int       m_decodedPictureHashSEIEnabled;                    ///< Checksum(3)/CRC(2)/MD5(1)/disable(0) acting on decoded picture hash SEI message
  Int       m_recoveryPointSEIEnabled;
  Int       m_bufferingPeriodSEIEnabled;
  Int       m_pictureTimingSEIEnabled;
  Bool      m_toneMappingInfoSEIEnabled;
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
  Int       m_exposureCompensationValueSignFlag;
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
  Int       m_framePackingSEIEnabled;
  Int       m_framePackingSEIType;
  Int       m_framePackingSEIId;
  Int       m_framePackingSEIQuincunx;
  Int       m_framePackingSEIInterpretation;
  Int       m_displayOrientationSEIAngle;
  Int       m_temporalLevel0IndexSEIEnabled;
  Int       m_gradualDecodingRefreshInfoEnabled;
  Int       m_decodingUnitInfoSEIEnabled;
  Int       m_SOPDescriptionSEIEnabled;
  Int       m_scalableNestingSEIEnabled;
  // weighted prediction
  Bool      m_useWeightedPred;                    ///< Use of weighted prediction in P slices
  Bool      m_useWeightedBiPred;                  ///< Use of bi-directional weighted prediction in B slices
  
  UInt      m_log2ParallelMergeLevel;                         ///< Parallel merge estimation region
  UInt      m_maxNumMergeCand;                                ///< Max number of merge candidates

  Int       m_TMVPModeId;
  Int       m_signHideFlag;
  Bool      m_RCEnableRateControl;                ///< enable rate control or not
  Int       m_RCTargetBitrate;                    ///< target bitrate when rate control is enabled
  Int       m_RCKeepHierarchicalBit;              ///< 0: equal bit allocation; 1: fixed ratio bit allocation; 2: adaptive ratio bit allocation
  Bool      m_RCLCULevelRC;                       ///< true: LCU level rate control; false: picture level rate control
  Bool      m_RCUseLCUSeparateModel;              ///< use separate R-lambda model at LCU level
  Int       m_RCInitialQP;                        ///< inital QP for rate control
  Bool      m_RCForceIntraQP;                     ///< force all intra picture to use initial QP or not

#if KWU_RC_VIEWRC_E0227
  vector<Int>     m_viewTargetBits;
  Bool      m_viewWiseRateCtrl;                              ///< Flag for using view-wise rate control
#endif
#if KWU_RC_MADPRED_E0227
  UInt       m_depthMADPred;
#endif
  Int       m_useScalingListId;                               ///< using quantization matrix
  Char*     m_scalingListFile;                                ///< quantization matrix file name

  Bool      m_TransquantBypassEnableFlag;                     ///< transquant_bypass_enable_flag setting in PPS.
  Bool      m_CUTransquantBypassFlagForce;                    ///< if transquant_bypass_enable_flag, then, if true, all CU transquant bypass flags will be set to true.

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
#if H_MV
  Bool              m_subBistreamPropSEIEnabled;
  Int               m_sbPropNumAdditionalSubStreams;
  std::vector<Int>  m_sbPropSubBitstreamMode;
  std::vector<Int>  m_sbPropOutputLayerSetIdxToVps;
  std::vector<Int>  m_sbPropHighestSublayerId;
  std::vector<Int>  m_sbPropAvgBitRate;
  std::vector<Int>  m_sbPropMaxBitRate;
  Bool              m_outputVpsInfo;
#endif
#if H_3D
  // Camera parameters
  Char*     m_pchCameraParameterFile;                         ///< camera parameter file
  Char*     m_pchBaseViewCameraNumbers;
  TAppComCamPara m_cCameraData;
  Int       m_iCodedCamParPrecision;                          ///< precision for coding of camera parameters
#if H_3D_VSO
  Char*     m_pchVSOConfig;
  Bool      m_bUseVSO;                                    ///< flag for using View Synthesis Optimization
  Bool      m_bVSOLSTable;                                ///< Depth QP dependent Lagrange parameter optimization (m23714)
  Bool      m_bVSOEarlySkip;                              ///< Early skip of VSO computation (JCT3V-A0093 modification 4)

  //// Used for development by GT, might be removed later
  Double    m_dLambdaScaleVSO;                            ///< Scaling factor for Lambda in VSO mode
  Bool      m_bForceLambdaScaleVSO;                       ///< Use Lambda Scale for depth even if VSO is turned off
  Bool      m_bAllowNegDist;                              ///< Allow negative distortion in VSO
  UInt      m_uiVSOMode;                                  ///< Number of VSO Mode, 1 = , 2 = simple, org vs. ren, 3 = simple, ren vs. ren, 4 = full  

  // SAIT_VSO_EST_A0033
  Bool      m_bUseEstimatedVSD;                           ///< Flag for using model based VSD estimation instead of VSO for some encoder decisions (JCT3V-A0033 modification 3)  

  // LGE_WVSO_A0119
  Bool      m_bUseWVSO;                                    ///< flag for using View Synthesis Optimization  
  Int       m_iVSOWeight;
  Int       m_iVSDWeight;
  Int       m_iDWeight;

  // Ren Model String
  TRenModSetupStrParser       m_cRenModStrParser;
#endif
#if H_3D_DIM
#if !HHI_TOOL_PARAMETERS_I2_J0107
  Bool      m_useDMM;                                        ///< flag for using DMM
  Bool      m_useIVP;
  Bool      m_useSDC;                                        ///< flag for using SDC
#endif
  Bool      m_useDLT;                                        ///< flag for using DLT
#endif
#if !HHI_TOOL_PARAMETERS_I2_J0107
#if H_3D_SINGLE_DEPTH
  Bool     m_useSingleDepthMode;                          ///< flag for using single depth mode
#endif
#endif
#if H_3D_QTLPC
  Bool      m_bUseQTL;                                        ///< flag for using depth QuadTree Limitation
#endif
#if !HHI_TOOL_PARAMETERS_I2_J0107
#if H_3D_INTER_SDC
  Bool m_bDepthInterSDCFlag;                                ///< flag for inter SDC of depth map coding
#endif
#if H_3D_DBBP
  Bool      m_bUseDBBP;                                     ///< flag for depth-based block partitioning
#endif
#if H_3D_IV_MERGE
  Bool m_bMPIFlag;                                           ///< flag for MPI of depth map coding
#endif
#endif

#if HHI_TOOL_PARAMETERS_I2_J0107
  std::vector< Bool >    m_ivMvPredFlag;
  std::vector< Bool >    m_ivMvScalingFlag;
  Int                    m_log2SubPbSizeMinus3;
  Bool                   m_ivResPredFlag;
  Bool                   m_depthRefinementFlag;
  Bool                   m_viewSynthesisPredFlag;
  Bool                   m_depthBasedBlkPartFlag;
  Bool                   m_mpiFlag;
  Int                    m_log2MpiSubPbSizeMinus3;
  Bool                   m_intraContourFlag;
  Bool                   m_intraWedgeFlag;
  Bool                   m_intraSdcFlag;
  Bool                   m_qtPredFlag;
  Bool                   m_interSdcFlag;
  Bool                   m_intraSingleFlag;
#endif

#endif
  // internal member functions
  Void  xSetGlobal      ();                                   ///< set global variables
  Void  xCheckParameter ();                                   ///< check validity of configuration values
  Void  xPrintParameter ();                                   ///< print configuration values
  Void  xPrintUsage     ();                                   ///< print usage
#if H_MV

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
  Void xPrintParaVector( std::string description, std::vector<T> & rpcVector )
  {
    Int iSpace = max(1, ENC_CFG_CONSOUT_SPACE - (Int) description.length() ); 
    
    for ( Int i = 0; i < iSpace; i++ ) 
      description.append( " " ); 
      
    description.append( ":" ); 
    printf( "%s", description.c_str() ); 

    for(Int i=0;i<rpcVector.size();i++)                
      xPrintVectorElem( rpcVector[i] );

    printf("\n");
  }
  
  Void xPrintVectorElem( UInt   elem ) { printf(" %d"   , elem            );};
  Void xPrintVectorElem( Int    elem ) { printf(" %d"   , elem            );};
  
  Void xPrintVectorElem( Double elem ) { printf(" %5.2f", elem            );};  
  Void xPrintVectorElem( Bool   elem ) { printf(" %d"   , ( elem ? 1 : 0 ));};
#endif
#if H_MV
  Int   getGOPSize() { return m_iGOPSize; }
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

