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
/** \file     TypeDef.h
    \brief    Define basic types, new types and enumerations
*/
#ifndef __TYPEDEF__
#define __TYPEDEF__
#ifndef __COMMONDEF__
#error Include CommonDef.h not TypeDef.h
#endif
#include <vector>
//! \ingroup TLibCommon
//! \{
/////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////// EXTENSION SELECTION ///////////////////////////////////  
/////////////////////////////////////////////////////////////////////////////////////////
/* HEVC_EXT might be defined by compiler/makefile options.
   Linux makefiles support the following settings:   
   make             -> HEVC_EXT not defined    
   make HEVC_EXT=0  -> NH_MV=0 H_3D=0   --> plain HM
   make HEVC_EXT=1  -> NH_MV=1 H_3D=0   --> MV only 
   make HEVC_EXT=2  -> NH_MV=1 H_3D=1   --> full 3D 
*/
#ifndef HEVC_EXT
#define HEVC_EXT                    2
#endif
#if ( HEVC_EXT < 0 )||( HEVC_EXT > 2 )
#error HEVC_EXT must be in the range of 0 to 2, inclusive. 
#endif
#define NH_MV          ( HEVC_EXT != 0)
#define NH_3D          ( HEVC_EXT == 2)

/////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////   MAJOR DEFINES   ///////////////////////////////////  
/////////////////////////////////////////////////////////////////////////////////////////
#if NH_MV
#define H_MV_ENC_DEC_TRAC                 1  //< CU/PU level tracking

#if NH_3D
#define NH_3D_VSO                         1
#define NH_3D_DMM                         1   // Depth modeling modes
#define NH_3D_SDC                         1   // Segment-wise DC coding for intra and inter
#define NH_3D_ENC_DEPTH                   1   // Encoder optimizations for depth, incl.
                                              // HHI_DEPTH_INTRA_SEARCH_RAU_C0160
                                              // LG_ZEROINTRADEPTHRESI_A0087
                                              // HHI_DMM4_ENC_I0066
                                              // H_3D_FAST_DEPTH_INTRA
#endif

#define TEMP_SDC_CLEANUP                  1   // PM: consider these cleanups for DMM and SDC

#if H_3D
#define H_3D_QTLPC                        1   // OL_QTLIMIT_PREDCODING_B0068 //JCT3V-B0068
                                              // HHI_QTLPC_RAU_OFF_C0160 JCT3V-C0160 change 2: quadtree limitation and predictive coding switched off in random access units 
                                              // MTK_TEX_DEP_PAR_G0055 Texture-partition-dependent depth partition. JCT3V-G0055
#define H_3D_VSO                          1   // VSO, View synthesis optimization, includes: 
                                              // HHI_VSO
                                              // HHI_VSO_LS_TABLE_M23714 enable table base Lagrange multiplier optimization
                                              // SAIT_VSO_EST_A0033, JCT3V-A0033 modification 3
                                              // LGE_WVSO_A0119
                                              // SCU_HS_VSD_BUGFIX_IMPROV_G0163
#define H_3D_NBDV                         1   // Neighboring block disparity derivation 
                                              // QC_JCT3V-A0097 
                                              // LGE_DVMCP_A0126
                                              // LGE_DVMCP_MEM_REDUCTION_B0135     
                                              // QC_SIMPLE_NBDV_B0047
                                              // FIX_LGE_DVMCP_B0133
                                              // QC_NBDV_LDB_FIX_C0055
                                              // MTK_SAIT_TEMPORAL_FIRST_ORDER_C0141_C0097
                                              // MTK_SIMPLIFY_DVTC_C0135           
                                              // QC_CU_NBDV_D0181
                                              // SEC_DEFAULT_DV_D0112
                                              // MTK_DVMCP_FIX_E0172       fix the mismatch between software and WD for DV derivation from DVMCP blocks, issue 2 in JCT3V-E0172
                                              // SEC_SIMPLIFIED_NBDV_E0142 Simplified NBDV, JCT3V-E0142 and JCT3V-E0190
                                              // MTK_NBDV_TN_FIX_E0172     fix the issue of DV derivation from the temporal neighboring blocks, issue 7 in JCT3V-E0172
                                              // MTK_TEXTURE_MRGCAND_BUGFIX_E0182  Bug fix for TEXTURE MERGING CANDIDATE     , JCT3V-E0182
                                              // LGE_SIMP_DISP_AVAIL_J0041    // Use 2 status for disparity availability - DISP_AVAILABLE and DISP_NONE
#define H_3D_ARP                          1   // Advanced residual prediction (ARP), JCT3V-D0177
                                              // QC_MTK_INTERVIEW_ARP_F0123_F0108 JCT3V-F0123; JCT3V-F0108
                                              // SHARP_ARP_REF_CHECK_F0105        ARP reference picture selection and DPB check
                                              // LGE_ARP_CTX_F0161                JCT3V-F0161
                                              // MTK_ARP_FLAG_CABAC_SIMP_G0061 Use 2 context for ARP flag referring to only left neighbor block in JCT3V-G0061
                                              // MTK_ARP_REF_SELECTION_G0053 ARP Reference picture selection in JCT3V-G0053 
                                              // MTK_ALIGN_SW_WD_BI_PRED_ARP_H0085  Align the SW and WD for the bi-prediction ARP PUs by disallowing non-normative fast bi-prediction for ARP PUs, JCT3V-H0085
                                              // QC_I0051_ARP_SIMP          
                                              // SHARP_ARP_CHROMA_I0104     
                                              // MTK_I0072_IVARP_SCALING_FIX
                                              // SEC_ARP_VIEW_REF_CHECK_J0037    Signaling iv_res_pred_weight_idx when the current slice has both view and temporal reference picture(s), JCT3V-J0037 item1
                                              // SEC_ARP_REM_ENC_RESTRICT_K0035    Removal of encoder restriction of ARP, JCT3V-K0035
#define H_3D_IC                           1   // Illumination Compensation, JCT3V-B0045, JCT3V-C0046, JCT3V-D0060
                                              // Unifying rounding offset, for IC part, JCT3V-D0135
                                              // Full Pel Interpolation for Depth, HHI_FULL_PEL_DEPTH_MAP_MV_ACC
                                              // SHARP_ILLUCOMP_REFINE_E0046
                                              // MTK_CLIPPING_ALIGN_IC_E0168       // To support simplify bi-prediction PU with identical motion checking, JCT3V-E0168
                                              // LGE_IC_CTX_F0160 //JCT3V-F0160
                                              // SEC_ONLY_TEXTURE_IC_F0151
                                              // MTK_IC_FLAG_CABAC_SIMP_G0061
                                              // SEC_IC_ARP_SIG_G0072, Disabling IC when ARP is enabled, option 1 in JCT3V-G0072, part 2 in JCT3V-G0121
                                              // MTK_LOW_LATENCY_IC_ENCODING_H0086  Low-latency IC encoding in JCT3V-H0086
                                              // MTK_LOW_LATENCY_IC_ENCODING_H0086_FIX  1  // Remove the global variables used in JCT3V-H0086
                                              // SEC_IC_NEIGHBOR_CLIP_I0080    // Clipping of neighboring sample position, JCT3V-I0080
                                              // LGE_CHROMA_IC_J0050_J0034
#if H_3D_NBDV
#define H_3D_NBDV_REF                     1   // Depth oriented neighboring block disparity derivation
                                              // MTK_D0156
                                              // MERL_D0166: Reference view selection in NBDV & Bi-VSP
                                              // MERL_C0152: Basic VSP
                                              // NBDV_DEFAULT_VIEWIDX_BUGFIX Bug fix for invalid default view index for NBDV
                                              // NTT_DoNBDV_VECTOR_CLIP_E0141 disparity vector clipping in DoNBDV, JCT3V-E0141 and JCT3V-E0209
                                              // SEC_VER_DONBDV_H0103          Vertical DV Restriction for DoNBDV
#endif
#define H_3D_VSP                          1   // View synthesis prediction
                                              // MERL_C0152: Basic VSP
                                              // MERL_D0166: Reference view selection in NBDV & Bi-VSP
                                              // MTK_D0105, LG_D0139: No VSP for depth
                                              // QC_D0191: Clean up
                                              // LG_D0092: Multiple VSP candidate allowed
                                              // MTK_VSP_FIX_ALIGN_WD_E0172
                                              // NTT_VSP_ADAPTIVE_SPLIT_E0207 adaptive sub-PU partitioning in VSP, JCT3V-E0207
                                              // NTT_VSP_DC_BUGFIX_E0208 bugfix for sub-PU based DC in VSP, JCT3V-E0208
                                              // NTT_VSP_COMMON_E0207_E0208 common part of JCT3V-E0207 and JCT3V-E0208
                                              // MTK_F0109_LG_F0120_VSP_BLOCK MTK_LG_SIMPLIFY_VSP_BLOCK_PARTITION_F0109_F0120  
                                              // SHARP_VSP_BLOCK_IN_AMP_F0102 VSP partitioning for AMP
                                              // MTK_VSP_SIMPLIFICATION_F0111 1. Inherited VSP also use NBDV of current CU, 2. VSP cannot be inherited from above LCU rowss
                                              // LGE_SHARP_VSP_INHERIT_F0104 
                                              // NTT_STORE_SPDV_VSP_G0148 Storing Sub-PU based DV for VSP
                                              // Restricted bi-prediction for VSP
                                              // MTK_MRG_LIST_SIZE_CLEANUP_J0059   1   // Include VSP for deriving merge candidate list size, JCT3V-J0059
                                              // SEC_A1_BASED_VSP_J0039            1   // Removal of redundant VSP in Merge list
#define H_3D_IV_MERGE                     1   // Inter-view motion merge candidate
                                              // HHI_INTER_VIEW_MOTION_PRED 
                                              // SAIT_IMPROV_MOTION_PRED_M24829, improved inter-view motion vector prediction
                                              // QC_MRG_CANS_B0048             , JCT3V-B0048, B0086, B0069
                                              // OL_DISMV_POS_B0069            , different pos for disparity MV candidate, B0069
                                              // MTK_INTERVIEW_MERGE_A0049     , second part
                                              // QC_AMVP_MRG_UNIFY_IVCAN_C0051     
                                              // TEXTURE MERGING CANDIDATE     , JCT3V-C0137
                                              // QC_INRIA_MTK_MRG_E0126 
                                              // ETRIKHU_MERGE_REUSE_F0093 QC_DEPTH_IV_MRG_F0125, JCT3V-F0125: Depth oriented Inter-view MV candidate
                                              // EC_MPI_ENABLING_MERGE_F0150, MPI flag in VPS and enabling in Merge mode
                                              // MTK_NBDV_IVREF_FIX_G0067      , Disable IvMC, VSP when IVREF is not available, JCT3V-G0067
                                              // SEC_DEPTH_DV_DERIVAITON_G0074, Simplification of DV derivation for depth, JCT3V-G0074
                                              // QC_DEPTH_MERGE_SIMP_G0127 Remove DV candidate and shifting candidate for depth coding
                                              // QC_IV_PRED_CONSTRAINT_H0137   Constraint on inter-view (motion) prediction tools
                                              // ETRIKHU_BUGFIX_H0083          bug-fix for DV candidate pruning
                                              // ETRIKHU_CLEANUP_H0083         cleaned-up source code for constructing merging candidate list
                                              // ETRIKHU_CLEANUP_H0083_MISSING missing guard macros added by GT
                                              // SHARP_SIMPLE_MERGE_H0062      Restrict 3D-HEVC merge cand in small PUs
                                              // MTK_DIS_SPBIP8X4_H0205        Disable bi-prediction for 8x4 and 4x8 sub PU and remove the SPIVMP 2Nx2N restriction
                                              // SEC_ADAPT_DISABLE_IVMP        Disabling IVMP merge candidates when IC is enabled, JCT3V-H0070
                                              // SEC_SIMP_SHIFTED_DV_I0086     Simplification of Shifted DV candidate, JCT3V-I0086
                                              // SEC_SHIFTED_IVMC_POS_K0036    Position Derivation for Shifted-IVMC, JCT3V-K0036
#define H_3D_TMVP                         1   // QC_TMVP_C0047 
                                              // Sony_M23639
                                              // H_3D_TMVP_SCALING_FIX_K0053       1   // QC/CY for K0053
#define H_3D_DIM                          1   // DIM, Depth intra modes, includes:
                                              // HHI_DMM_WEDGE_INTRA
                                              // HHI_DMM_PRED_TEX
                                              // FIX_WEDGE_NOFLOAT_D0036
                                              // LGE_EDGE_INTRA_A0070
                                              // LGE_DMM3_SIMP_C0044
                                              // QC_DC_PREDICTOR_D0183
                                              // HHI_DELTADC_DLT_D0035
                                              // PKU_QC_DEPTH_INTRA_UNI_D0195
                                              // RWTH_SDC_DLT_B0036
                                              // INTEL_SDC64_D0193
                                              // RWTH_SDC_CTX_SIMPL_D0032
                                              // LGE_CONCATENATE_D0141
                                              // FIX_SDC_ENC_RD_WVSO_D0163
                                              // MTK_SAMPLE_BASED_SDC_D0110
                                              // SEC_DMM2_E0146_HHIFIX Removal of DMM2 from DMMs
                                              // ZJU_DEPTH_INTRA_MODE_E0204 Simplified Binarization for depth_intra_mode
                                              // KWU_SDC_SIMPLE_DC_E0117 Simplified DC calculation for SDC
                                              // SCU_HS_DMM4_REMOVE_DIV_E0242 DMM4 Division Removal
                                              // LGE_SDC_REMOVE_DC_E0158 Removal of DC mode from SDC
                                              // LGE_PKU_DMM3_OVERLAP_E0159_HHIFIX 1   Removal of overlap between DMM3 and DMM1
                                              // LGE_PRED_RES_CODING_DLT_DOMAIN_F0159 JCT3V-F0159
                                              // HHI_DIM_PREDSAMP_FIX_F0171
                                              // SEC_DMM3_RBC_F0147 Removal of DMM3 and RBC from DMMs
                                              // QC_DIM_DELTADC_UNIFY_F0132 Unify delta DC coding in depth intra modes
                                              // Unify intra SDC and inter SDC
                                              // QC_GENERIC_SDC_G0122 Generalize SDC to all depth intra modes
                                              // SCU_HS_DEPTH_DC_PRED_G0143
                                              // HS_TSINGHUA_SDC_SPLIT_G0111
                                              // QC_PKU_SDC_SPLIT_G0123 Intra SDC Split
                                              // HS_DMM_SDC_PREDICTOR_UNIFY_H0108  Unification of DMM and SDC predictor derivation
                                              // LGE_SIMP_DIM_NOT_PRESENT_FLAG_CODING_H0119_H0135  Use only one context for CABAC of dim_not_present_flag
                                              // QC_SIMP_DELTADC_CODING_H0131   Simplify detaDC entropy coding 
                                              // MTK_DMM_SIMP_CODE_H0092        Remove CABAC context for DMM1 mode coding
                                              // MTK_DELTA_DC_FLAG_ONE_CONTEXT_H0084_H0100_H0113 Use only one context for CABAC of delta_dc_flag as in JCTVC-H0084, JCTVC-H0100 and JCTVC-H0113
                                              // MTK_SDC_FLAG_FIX_H0095                          Remove conditional check of PCM flag based on SDC flag, JCTVC-H0095
                                              // SEC_NO_RESI_DLT_H0105    
                                              // MTK_DLT_CODING_FIX_H0091 
                                              // HS_DMM_SIGNALLING_I0120
                                              // SHARP_DMM1_I0110 LUT size reduction for DMM1 proposed in JCT3V-I0110 
                                              // FAST_SDC_OFFSET_DECISION_I0084
                                              // SEPARATE_FLAG_I0085
                                              // H_3D_DELTA_DLT
                                              // RWTH_DLT_CLIP_I0057
                                              // MTK_DMM_SIM_J0035
                                              // MTK_J0033
                                              // SHARP_DLT_SIMP_J0029 DLT(DepthValue2Idx[]) table derivation cleanup
                                              // SHARP_DMM_CLEAN_K0042             1   // Generate DMM pattern with rotation 
#define H_3D_INTER_SDC                    1   // INTER SDC, Inter simplified depth coding
                                              // LGE_INTER_SDC_E0156 Enable inter SDC for depth coding
                                              // SEC_INTER_SDC_G0101 Improved inter SDC with multiple DC candidates
#define H_3D_SPIVMP                       1   // H_3D_SPIVMP JCT3V-F0110: Sub-PU level inter-view motion prediction
                                              // SEC_SPIVMP_MCP_SIZE_G0077, Apply SPIVMP only to 2Nx2N partition, JCT3V-G0077
                                              // QC_SPIVMP_MPI_G0119 Sub-PU level MPI merge candidate
                                              // Simplification on Sub-PU level temporal interview motion prediction
                                              // MPI_SUBPU_DEFAULT_MV_H0077_H0099_H0111_H0133
#define H_3D_DBBP                         1   // DBBP: Depth-based Block Partitioning and Merging
                                              // MTK_DBBP_AMP_REM_H0072   
                                              // RWTH_DBBP_NO_SPU_H0057   
                                              // SEC_DBBP_FILTERING_H0104 
                                              // MTK_DBBP_SIGNALING_H0094    
                                              // H_3D_FIX_DBBP_IVMP Fix . Enable IVMP is always disabled, when DBBP is enabled. The original intention is to disable Sub-PU IVMP when DBBP is enabled, not to disable IVMP itself. 
                                              // SEC_DBBP_EXPLICIT_SIG_I0077 Remove the partition derivation and signal dbbp_flag only when the partition mode is 2NxN/Nx2N, JCT3V-I0077
                                              // Disallow DBBP in 8x8 CU, JCT3V-I0078
                                              // SHARP_DBBP_SIMPLE_FLTER_I0109 Simple condition and one dimensional filter for DBBP
                                              // SEC_DBBP_DMM4_THRESHOLD_I0076 Simplification of threshold derivation for DBBP and DMM4, JCT3V-I0076
                                              // SEC_DBBP_VIEW_REF_CHECK_J0037 Signaling dbbp_flag when the current slice has view reference picture(s), JCT3V-J0037 item4
                                              // RWTH_DBBP_NO_SATD_K0028
                                              // HS_DBBP_CLEAN_K0048
#define H_3D_DDD                          1   // Disparity derived depth coding
                                              // LGE_DDD_REMOVAL_J0042_J0030 DDD removal
#define H_3D_DIS                          1   // Depth intra skip 
                                              // SEC_DEPTH_INTRA_SKIP_MODE_K0033  Depth intra skip mode
#define H_3D_FCO                          0   // Flexible coding order for 3D
#define H_3D_FAST_INTRA_SDC               1   // I0123
// OTHERS
                                              // MTK_SONY_PROGRESSIVE_MV_COMPRESSION_E0170 // Progressive MV Compression, JCT3V-E0170
#define H_3D_FAST_TEXTURE_ENCODING        1   // Fast merge mode decision and early CU determination for texture component of dependent view, JCT3V-E0173
                                              // MTK_FAST_TEXTURE_ENCODING_E0173
#if H_3D_DIM
#define H_3D_FAST_DEPTH_INTRA             1   // Fast DMM Selection
                                              // SCU_HS_FAST_DEPTH_INTRA_E0238_HHIFIX
#endif
//HLS
                                             // HHI_DEPENDENCY_SIGNALLING_I1_J0107
                                             // HHI_TOOL_PARAMETERS_I2_J0107
                                             // HHI_VPS_3D_EXTENSION_I3_J0107
                                             // HHI_INTER_COMP_PRED_K0052
                                             // HHI_RES_PRED_K0052       
                                             // HHI_CAM_PARA_K0052       
                                             // H_3D_DIRECT_DEP_TYPE     
// Rate Control
#define KWU_FIX_URQ                       1
#define KWU_RC_VIEWRC_E0227               0  ///< JCT3V-E0227, view-wise target bitrate allocation
#define KWU_RC_MADPRED_E0227              0  ///< JCT3V-E0227, inter-view MAD prediction
#endif // H_3D
/////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////   DERIVED DEFINES ///////////////////////////////////  
/////////////////////////////////////////////////////////////////////////////////////////
#if NH_3D
#define H_3D_OUTPUT_ACTIVE_TOOLS               0
#define H_3D_REN_MAX_DEV_OUT                   0
#endif
///// ***** VIEW SYNTHESIS OPTIMIZAION *********
#if NH_3D_VSO                                  
#define H_3D_VSO_DIST_INT                 1   // Allow negative synthesized view distortion change
#define H_3D_VSO_COLOR_PLANES             1   // Compute VSO distortion on color planes 
#define H_3D_VSO_EARLY_SKIP               1   // LGE_VSO_EARLY_SKIP_A0093, A0093 modification 4
#define H_3D_VSO_RM_ASSERTIONS            0   // Output VSO assertions
#define H_3D_VSO_SYNTH_DIST_OUT           0   // Output of synthesized view distortion instead of depth distortion in encoder output
#endif
////   ****** NEIGHBOURING BLOCK-BASED DISPARITY VECTOR  *********
#if H_3D_NBDV
#define DVFROM_LEFT                       0
#define DVFROM_ABOVE                      1
#define IDV_CANDS                         2
#endif
///// ***** ADVANCED INTERVIEW RESIDUAL PREDICTION *********
#if H_3D_ARP
#define H_3D_ARP_WFNR                     3
#endif
///// ***** DEPTH INTRA MODES *********
#if H_3D_DIM
                                              // HHI_DMM4_ENC_I0066
#define H_3D_DIM_DMM                      1   // Depth Modeling Modes
#define H_3D_DIM_SDC                      1   // Simplified Depth Coding method
#define H_3D_DIM_DLT                      1   // Depth Lookup Table
#define H_3D_DIM_ENC                      1   // Depth Intra encoder optimizations, includes:
                                              // HHI_DEPTH_INTRA_SEARCH_RAU_C0160
                                              // LG_ZEROINTRADEPTHRESI_A0087
#endif
/////////////////////////////////////////////////////////////////////////////////////
/// GT: Move values which are not flags to CommonDef.h and convert to static int !!
///////////////////////////////////////////////////////////////////////////////////
///// ***** VIEW SYNTHESIS PREDICTION *********
#if H_3D_VSP
#define H_3D_VSP_BLOCKSIZE                4   // Supported values: 1, 2, and 4
#if H_3D_VSP_BLOCKSIZE == 1
#define H_3D_VSP_CONSTRAINED              1   // Constrained VSP @ 1x1
#else
#define H_3D_VSP_CONSTRAINED              0
#endif
#endif
///// ***** ILLUMATION COMPENSATION *********
#if H_3D_IC
#define IC_REG_COST_SHIFT                 7
#define IC_CONST_SHIFT                    5
#define IC_SHIFT_DIFF                     12
#define IC_LOW_LATENCY_ENCODING_THRESHOLD 0.1 // Threshold for low-latency IC encoding in JCT3V-H0086
#endif
///// ***** DEPTH BASED BLOCK PARTITIONING *********
#if H_3D_DBBP
#define DBBP_INVALID_SHORT                (-4)
#define DBBP_PACK_MODE               SIZE_2NxN
#endif
///// ***** FCO *********
#if H_3D_FCO
#define H_3D_FCO_VSP_DONBDV_E0163               1   // Adaptive depth reference for flexible coding order
#else
#define H_3D_FCO_VSP_DONBDV_E0163               0   // Adaptive depth reference for flexible coding order
#endif
#if H_3D
#define PPS_FIX_DEPTH                           1
#endif
/////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////   MV_HEVC HLS  //////////////////////////////
/////////////////////////////////////////////////////////////////////////////////
// TBD: Check if integration is necessary. 
#define H_MV_HLS_PTL_LIMITS                  0
#define H_MV_HLS7_GEN                        0  // General changes (not tested)
// POC
// #define H_MV_HLS_7_POC_P0041_3            0 // (POC/P0041/POC reset) #3 It was remarked that we should require each non-IRAP picture that has discardable_flag equal to 1 to have NUT value indicating that it is a sub-layer non-reference picture. This was agreed. Decision: Adopt (with constraint for discardable_flag as described above) 
// #define H_MV_HLS_7_POC_P0041_FIXES        0 // (POC/P0041/Fixes) For each non-IRAP picture that has discardable_flag equal to 1 to have NUT value indicating that it is a sub-layer non-reference picture. 
// #define H_MV_HLS_7_POC_P0056_4            0 // (POC/P0056/layer tree poc) #4 Proposal 1: If the POC reset approach is adopted as the basis for multi-layer POC derivation, it is proposed to derive the POC anchor picture from the previous TID0 picture (that is not a RASL picture, a RADL picture or a sub-layer non-reference picture and not with discardable_flag equal to 1) of  the current layer or any of its reference layer. This is asserted to improve loss resilience and reduce bit rate overhead. Decision: Adopt Proposal 1 (with the suggested modifications Ewith text provided as P0297).
// SEI related
//#define H_MV_HLS_8_SEI_NODOC_53  0 // #53 (SEI    /NODOC/Added Multiview view position SEI message) Plain copy from AVC.
//#define H_MV_HLS_8_SEI_NODOC_52  0 // #52 (SEI    /NODOC/Added Multiview acquisition information SEI) Plain copy from AVC. 
//#define H_MV_HLS_8_SEI_NODOC_51  0 // #51 (SEI    /NODOC/Added Multiview scene information SEI message)
//#define H_MV_HLS_8_SEI_Q0189_35  0 // #35 (SEI    /Q0189/SEI message for indicating constraints on TMVP) Proposal 2.3,  SEI message for indicating constraints on TMVP
//#define H_MV_HLS_8_EDF_Q0116_29  0 // #29 (ED.FIX /Q0116/Recovery point SEI) , consider adding a note regarding how random accessibility is affected by the recovery point SEI message
//#define H_MV_HLS_8_GEN_Q0183_23  0 // #23 (GEN    /Q0183/SEI clean-ups) numerous small clean-ups on SEI messages.
//#define H_MV_HLS_8_MIS_Q0247_49  0 // #49 (MISC   /Q0247/frame-field information SEI message)
//#define H_MV_HLS_8_MIS_Q0189_34  0 // #34 (MISC   /Q0189/slice temporal mvp enabled flag) Proposal 2.2, clarification of semantics of slice temporal mvp enabled flag
//#define H_MV_HLS_8_EDF_Q0081_01  0 // #1  (ED.FIX /Q0081/alpha channel persist) On reuse of alpha planes in auxiliary pictures. It was asked why there would not be a presumption that the alpha channel content would simply persist, without needing the flag to indicate it. Decision (Ed.): Delegated to editors to clarify, as necessary, that the alpha channel content persists until cancelled or updated in output order.
//#define H_MV_HLS_8_SEI_Q0253_37  0 // #37 (SEI    /Q0253/layer not present), modified semantics of layers not present SEI message to correct bug introduced during editing 
//#define H_MV_HLS_8_SEI_Q0045_11  0 // #11 (SEI    /Q0045/Overlay) Proposal for an SEI message on selectable overlays. Decision: Adopt (modified for variable-length strings).
//#define H_MV_HLS_7_SEI_P0133_28  0 // (SEI/P0133/Recovery point SEI) #28 Decision: Adopt change to recover point semantics only (-v3)
//#define H_MV_HLS_7_SEI_P0123_25  0 // (SEI/P0123/Alpha channel info) #25 Add alpha channel information SEI message Decision: Adopt. Constrain the bit depth indicated to be equal to the coded bit depth of the aux picture. 
// DPB
//#define H_MV_HLS_8_HRD_Q0102_09  0 // #9  (HRD    /Q0102/NoOutputOfPriorPicsFlag) It was suggested that also the separate_colour_plane_flag should affect inference of NoOutputOfPriorPicsFlag. Decision (Ed.): Agreed (affects RExt text).
//#define H_MV_HLS_8_DBP_Q0154_38  0 // #38 (DBP    /Q0154/VPS DPB) Proposal in C.5.2.1: Add in the decoding process that when a new VPS is activated, all pictures in the DPB are marked as unused for reference
//#define H_MV_HLS_8_HRD_Q0154_10  0 // #10 (HRD    /Q0154/DPB Flushing and parameters) On picture flushing and DPB parameters Decision: Adopted (some details to be discussed further in BoG).
//#define H_MV_HLS_7_OTHER_P0187_1 0 // (OTHER/P0187/NoOutputOfPriorPicsFlag) #1 Inference of NoOutputOfPriorPicsFlag and proposes to take into account colour format and bit depth for the inference in addition to spatial resolution 
// OTHERS
//#define H_MV_HLS_8_HSB_Q0041_03  0 // #3  (HS     /Q0041/hybrid scalability) The proposed text was endorsed, with non-editorial open issues considered as follows ?// #define H_MV_HLS_7_OTHER_P0187_1          0 // (OTHER/P0187/NoOutputOfPriorPicsFlag) #1 Inference of NoOutputOfPriorPicsFlag and proposes to take into account colour format and bit depth for the inference in addition to spatial resolution 
//#define H_MV_HLS_8_MIS_Q0078_24  0 // #24 (MISC   /Q0078/scan and pic type) , Items 3 b,c and 4, clarifying which pictures in an output layer sets are applied the values of general_progressive_source_flag, general_interlaced_source_flag, general_non_packed_constraint_flag and general_frame_only_constraint_flag.
//#define H_MV_HLS_7_HRD_P0138_6   0 //     (HRD/P0138/HRD parameters for bitstreams excluding) #6 Decision: Adopt (as revised in updated contribution, with the specification of a flag in the BP SEI (HRD/P0192/sub-DPB) #12 Establish sub-DPBs based on the representation format indicated at the VPS level. It was suggested that the expressed shared capacity limit would need to be less than or equal to the sum of the individual capacity limits. Decision: Adopt as modified. Further study is encouraged on profile/level constraint selections. 
/////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////   HM RELATED DEFINES ////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////
#endif
// ====================================================================================================================
// Debugging
// ====================================================================================================================
#define DEBUG_STRING                                      0 ///< When enabled, prints out final decision debug info at encoder and decoder
#define DEBUG_ENCODER_SEARCH_BINS                         0 ///< When enabled, prints out each bin as it is coded during encoder search
#define DEBUG_CABAC_BINS                                  0 ///< When enabled, prints out each bin as it is coded during final encode and decode
#define DEBUG_INTRA_SEARCH_COSTS                          0 ///< When enabled, prints out the cost for each mode during encoder search
#define DEBUG_TRANSFORM_AND_QUANTISE                      0 ///< When enabled, prints out each TU as it passes through the transform-quantise-dequantise-inverseTransform process
#define ENVIRONMENT_VARIABLE_DEBUG_AND_TEST               0 ///< When enabled, allows control of debug modifications via environment variables
#define PRINT_MACRO_VALUES                                1 ///< When enabled, the encoder prints out a list of the non-environment-variable controlled macros and their values on startup
// TODO: rename this macro to DECODER_DEBUG_BIT_STATISTICS (may currently cause merge issues with other branches)
// This can be enabled by the makefile
#ifndef RExt__DECODER_DEBUG_BIT_STATISTICS
#define RExt__DECODER_DEBUG_BIT_STATISTICS                0 ///< 0 (default) = decoder reports as normal, 1 = decoder produces bit usage statistics (will impact decoder run time by up to ~10%)
#endif
// This can be enabled by the makefile
#if !NH_MV
#ifndef ENC_DEC_TRACE
#define ENC_DEC_TRACE                                     0
#endif
#endif
#define DEC_NUH_TRACE                                     0 ///< When trace enabled, enable tracing of NAL unit headers at the decoder (currently not possible at the encoder)
#define PRINT_RPS_INFO                                    0 ///< Enable/disable the printing of bits used to send the RPS.
// ====================================================================================================================
// Tool Switches - transitory (these macros are likely to be removed in future revisions)
// ====================================================================================================================
#define DECODER_CHECK_SUBSTREAM_AND_SLICE_TRAILING_BYTES  1 ///< TODO: integrate this macro into a broader conformance checking system.
#define T0196_SELECTIVE_RDOQ                              1 ///< selective RDOQ
// ====================================================================================================================
// Tool Switches
// ====================================================================================================================
#define ADAPTIVE_QP_SELECTION                             1 ///< G382: Adaptive reconstruction levels, non-normative part for adaptive QP selection
#define AMP_ENC_SPEEDUP                                   1 ///< encoder only speed-up by AMP mode skipping
#if AMP_ENC_SPEEDUP
#define AMP_MRG                                           1 ///< encoder only force merge for AMP partition (no motion search for AMP)
#endif
#define FAST_BIT_EST                                      1   ///< G763: Table-based bit estimation for CABAC
#define HHI_RQT_INTRA_SPEEDUP                             1           ///< tests one best mode with full rqt
#define HHI_RQT_INTRA_SPEEDUP_MOD                         0           ///< tests two best modes with full rqt
#if HHI_RQT_INTRA_SPEEDUP_MOD && !HHI_RQT_INTRA_SPEEDUP
#error
#endif
#define MATRIX_MULT                                       0 ///< Brute force matrix multiplication instead of partial butterfly
#define O0043_BEST_EFFORT_DECODING                        0 ///< 0 (default) = disable code related to best effort decoding, 1 = enable code relating to best effort decoding [ decode-side only ].
#define RDOQ_CHROMA_LAMBDA                                1 ///< F386: weighting of chroma for RDOQ
// This can be enabled by the makefile
#ifndef RExt__HIGH_BIT_DEPTH_SUPPORT
#define RExt__HIGH_BIT_DEPTH_SUPPORT                                           0 ///< 0 (default) use data type definitions for 8-10 bit video, 1 = use larger data types to allow for up to 16-bit video (originally developed as part of N0188)
#endif
// ====================================================================================================================
// Derived macros
// ====================================================================================================================
#if RExt__HIGH_BIT_DEPTH_SUPPORT
#define FULL_NBIT                                                              1 ///< When enabled, use distortion measure derived from all bits of source data, otherwise discard (bitDepth - 8) least-significant bits of distortion
#define RExt__HIGH_PRECISION_FORWARD_TRANSFORM                                 1 ///< 0 use original 6-bit transform matrices for both forward and inverse transform, 1 (default) = use original matrices for inverse transform and high precision matrices for forward transform
#else
#define FULL_NBIT                                                              0 ///< When enabled, use distortion measure derived from all bits of source data, otherwise discard (bitDepth - 8) least-significant bits of distortion
#define RExt__HIGH_PRECISION_FORWARD_TRANSFORM                                 0 ///< 0 (default) use original 6-bit transform matrices for both forward and inverse transform, 1 = use original matrices for inverse transform and high precision matrices for forward transform
#endif
#if FULL_NBIT
# define DISTORTION_PRECISION_ADJUSTMENT(x)  0
#else
# define DISTORTION_PRECISION_ADJUSTMENT(x) (x)
#endif
#if DEBUG_STRING
  #define DEBUG_STRING_PASS_INTO(name) , name
  #define DEBUG_STRING_PASS_INTO_OPTIONAL(name, exp) , (exp==0)?0:name
  #define DEBUG_STRING_FN_DECLARE(name) , std::string &name
  #define DEBUG_STRING_FN_DECLAREP(name) , std::string *name
  #define DEBUG_STRING_NEW(name) std::string name;
  #define DEBUG_STRING_OUTPUT(os, name) os << name;
  #define DEBUG_STRING_APPEND(str1, str2) str1+=str2;
  #define DEBUG_STRING_SWAP(str1, str2) str1.swap(str2);
  #define DEBUG_STRING_CHANNEL_CONDITION(compID) (true)
  #include <sstream>
  #include <iomanip>
#else
  #define DEBUG_STRING_PASS_INTO(name)
  #define DEBUG_STRING_PASS_INTO_OPTIONAL(name, exp)
  #define DEBUG_STRING_FN_DECLARE(name)
  #define DEBUG_STRING_FN_DECLAREP(name)
  #define DEBUG_STRING_NEW(name)
  #define DEBUG_STRING_OUTPUT(os, name)
  #define DEBUG_STRING_APPEND(str1, str2)
  #define DEBUG_STRING_SWAP(srt1, str2)
  #define DEBUG_STRING_CHANNEL_CONDITION(compID)
#endif
// ====================================================================================================================
// Error checks
// ====================================================================================================================
#if ((RExt__HIGH_PRECISION_FORWARD_TRANSFORM != 0) && (RExt__HIGH_BIT_DEPTH_SUPPORT == 0))
#error ERROR: cannot enable RExt__HIGH_PRECISION_FORWARD_TRANSFORM without RExt__HIGH_BIT_DEPTH_SUPPORT
#endif
// ====================================================================================================================
// Basic type redefinition
// ====================================================================================================================
typedef       void                Void;
typedef       bool                Bool;
#ifdef __arm__
typedef       signed char         Char;
#else
typedef       char                Char;
#endif
typedef       unsigned char       UChar;
typedef       short               Short;
typedef       unsigned short      UShort;
typedef       int                 Int;
typedef       unsigned int        UInt;
typedef       double              Double;
typedef       float               Float;
// ====================================================================================================================
// 64-bit integer type
// ====================================================================================================================
#ifdef _MSC_VER
typedef       __int64             Int64;
#if _MSC_VER <= 1200 // MS VC6
typedef       __int64             UInt64;   // MS VC6 does not support unsigned __int64 to double conversion
#else
typedef       unsigned __int64    UInt64;
#endif
#else
typedef       long long           Int64;
typedef       unsigned long long  UInt64;
#endif
// ====================================================================================================================
// Named numerical types
// ====================================================================================================================
#if RExt__HIGH_BIT_DEPTH_SUPPORT
typedef       Int             Pel;               ///< pixel type
typedef       Int64           TCoeff;            ///< transform coefficient
typedef       Int             TMatrixCoeff;      ///< transform matrix coefficient
typedef       Short           TFilterCoeff;      ///< filter coefficient
typedef       Int64           Intermediate_Int;  ///< used as intermediate value in calculations
typedef       UInt64          Intermediate_UInt; ///< used as intermediate value in calculations
#else
typedef       Short           Pel;               ///< pixel type
typedef       Int             TCoeff;            ///< transform coefficient
typedef       Short           TMatrixCoeff;      ///< transform matrix coefficient
typedef       Short           TFilterCoeff;      ///< filter coefficient
typedef       Int             Intermediate_Int;  ///< used as intermediate value in calculations
typedef       UInt            Intermediate_UInt; ///< used as intermediate value in calculations
#endif
#if FULL_NBIT
typedef       UInt64          Distortion;        ///< distortion measurement
#else
typedef       UInt            Distortion;        ///< distortion measurement
#endif
#if NH_MV                         
typedef std::vector< Int >        IntAry1d;
typedef std::vector< IntAry1d >   IntAry2d; 
typedef std::vector< IntAry2d >   IntAry3d; 
typedef std::vector< IntAry3d >   IntAry4d; 
typedef std::vector< IntAry4d >   IntAry5d; 
typedef std::vector< Bool >        BoolAry1d;
typedef std::vector< BoolAry1d >   BoolAry2d; 
typedef std::vector< BoolAry2d >   BoolAry3d; 
typedef std::vector< BoolAry3d >   BoolAry4d; 
typedef std::vector< BoolAry4d >   BoolAry5d; 
#endif
#if NH_3D_VSO
// ====================================================================================================================
// Define Distortion Types
// ====================================================================================================================
typedef       Int64           RMDist;     ///< renderer model distortion
#if H_3D_VSO_DIST_INT
typedef       Int64            Dist;       ///< RDO distortion
typedef       Int64            Dist64; 
#define       RDO_DIST_MIN     MIN_INT
#define       RDO_DIST_MAX     MAX_INT
#else
typedef       UInt             Dist;       ///< RDO distortion
typedef       UInt64           Dist; 
#define       RDO_DIST_MIN     0
#define       RDO_DIST_MAX     MAX_UINT
#endif
#endif
// ====================================================================================================================
// Enumeration
// ====================================================================================================================
enum RDPCMMode
{
  RDPCM_OFF             = 0,
  RDPCM_HOR             = 1,
  RDPCM_VER             = 2,
  NUMBER_OF_RDPCM_MODES = 3
};
enum RDPCMSignallingMode
{
  RDPCM_SIGNAL_IMPLICIT            = 0,
  RDPCM_SIGNAL_EXPLICIT            = 1,
  NUMBER_OF_RDPCM_SIGNALLING_MODES = 2
};
/// supported slice type
enum SliceType
{
  B_SLICE               = 0,
  P_SLICE               = 1,
  I_SLICE               = 2,
  NUMBER_OF_SLICE_TYPES = 3
};
/// chroma formats (according to semantics of chroma_format_idc)
enum ChromaFormat
{
  CHROMA_400        = 0,
  CHROMA_420        = 1,
  CHROMA_422        = 2,
  CHROMA_444        = 3,
  NUM_CHROMA_FORMAT = 4
};
enum ChannelType
{
  CHANNEL_TYPE_LUMA    = 0,
  CHANNEL_TYPE_CHROMA  = 1,
  MAX_NUM_CHANNEL_TYPE = 2
};
enum ComponentID
{
  COMPONENT_Y       = 0,
  COMPONENT_Cb      = 1,
  COMPONENT_Cr      = 2,
  MAX_NUM_COMPONENT = 3
};
enum InputColourSpaceConversion // defined in terms of conversion prior to input of encoder.
{
  IPCOLOURSPACE_UNCHANGED               = 0,
  IPCOLOURSPACE_YCbCrtoYCrCb            = 1, // Mainly used for debug!
  IPCOLOURSPACE_YCbCrtoYYY              = 2, // Mainly used for debug!
  IPCOLOURSPACE_RGBtoGBR                = 3,
  NUMBER_INPUT_COLOUR_SPACE_CONVERSIONS = 4
};
enum DeblockEdgeDir
{
  EDGE_VER     = 0,
  EDGE_HOR     = 1,
  NUM_EDGE_DIR = 2
};
/// supported partition shape
enum PartSize
{
  SIZE_2Nx2N           = 0,           ///< symmetric motion partition,  2Nx2N
  SIZE_2NxN            = 1,           ///< symmetric motion partition,  2Nx N
  SIZE_Nx2N            = 2,           ///< symmetric motion partition,   Nx2N
  SIZE_NxN             = 3,           ///< symmetric motion partition,   Nx N
  SIZE_2NxnU           = 4,           ///< asymmetric motion partition, 2Nx( N/2) + 2Nx(3N/2)
  SIZE_2NxnD           = 5,           ///< asymmetric motion partition, 2Nx(3N/2) + 2Nx( N/2)
  SIZE_nLx2N           = 6,           ///< asymmetric motion partition, ( N/2)x2N + (3N/2)x2N
  SIZE_nRx2N           = 7,           ///< asymmetric motion partition, (3N/2)x2N + ( N/2)x2N
  NUMBER_OF_PART_SIZES = 8
};
/// supported prediction type
enum PredMode
{
  MODE_INTER                 = 0,     ///< inter-prediction mode
  MODE_INTRA                 = 1,     ///< intra-prediction mode
  NUMBER_OF_PREDICTION_MODES = 2,
};
/// reference list index
enum RefPicList
{
  REF_PIC_LIST_0               = 0,   ///< reference list 0
  REF_PIC_LIST_1               = 1,   ///< reference list 1
  NUM_REF_PIC_LIST_01          = 2,
  REF_PIC_LIST_X               = 100  ///< special mark
};
/// distortion function index
enum DFunc
{
  DF_DEFAULT         = 0,
  DF_SSE             = 1,      ///< general size SSE
  DF_SSE4            = 2,      ///<   4xM SSE
  DF_SSE8            = 3,      ///<   8xM SSE
  DF_SSE16           = 4,      ///<  16xM SSE
  DF_SSE32           = 5,      ///<  32xM SSE
  DF_SSE64           = 6,      ///<  64xM SSE
  DF_SSE16N          = 7,      ///< 16NxM SSE
  DF_SAD             = 8,      ///< general size SAD
  DF_SAD4            = 9,      ///<   4xM SAD
  DF_SAD8            = 10,     ///<   8xM SAD
  DF_SAD16           = 11,     ///<  16xM SAD
  DF_SAD32           = 12,     ///<  32xM SAD
  DF_SAD64           = 13,     ///<  64xM SAD
  DF_SAD16N          = 14,     ///< 16NxM SAD
  DF_SADS            = 15,     ///< general size SAD with step
  DF_SADS4           = 16,     ///<   4xM SAD with step
  DF_SADS8           = 17,     ///<   8xM SAD with step
  DF_SADS16          = 18,     ///<  16xM SAD with step
  DF_SADS32          = 19,     ///<  32xM SAD with step
  DF_SADS64          = 20,     ///<  64xM SAD with step
  DF_SADS16N         = 21,     ///< 16NxM SAD with step
  DF_HADS            = 22,     ///< general size Hadamard with step
  DF_HADS4           = 23,     ///<   4xM HAD with step
  DF_HADS8           = 24,     ///<   8xM HAD with step
  DF_HADS16          = 25,     ///<  16xM HAD with step
  DF_HADS32          = 26,     ///<  32xM HAD with step
  DF_HADS64          = 27,     ///<  64xM HAD with step
  DF_HADS16N         = 28,     ///< 16NxM HAD with step
#if NH_3D_VSO
  DF_VSD      = 29,      ///< general size VSD
  DF_VSD4     = 30,      ///<   4xM VSD
  DF_VSD8     = 31,      ///<   8xM VSD
  DF_VSD16    = 32,      ///<  16xM VSD
  DF_VSD32    = 33,      ///<  32xM VSD
  DF_VSD64    = 34,      ///<  64xM VSD
  DF_VSD16N   = 35,      ///< 16NxM VSD
#endif
  DF_SAD12           = 43,
  DF_SAD24           = 44,
  DF_SAD48           = 45,
  DF_SADS12          = 46,
  DF_SADS24          = 47,
  DF_SADS48          = 48,
  DF_SSE_FRAME       = 50,     ///< Frame-based SSE
  DF_TOTAL_FUNCTIONS = 64
};
/// index for SBAC based RD optimization
enum CI_IDX
{
  CI_CURR_BEST = 0,     ///< best mode index
  CI_NEXT_BEST,         ///< next best index
  CI_TEMP_BEST,         ///< temporal index
  CI_CHROMA_INTRA,      ///< chroma intra index
  CI_QT_TRAFO_TEST,
  CI_QT_TRAFO_ROOT,
  CI_NUM,               ///< total number
};
/// motion vector predictor direction used in AMVP
enum MVP_DIR
{
  MD_LEFT = 0,          ///< MVP of left block
  MD_ABOVE,             ///< MVP of above block
  MD_ABOVE_RIGHT,       ///< MVP of above right block
  MD_BELOW_LEFT,        ///< MVP of below left block
  MD_ABOVE_LEFT         ///< MVP of above left block
};
#if H_3D
enum DefaultMergCandOrder
{
  MRG_T = 0,            ///< MPI
  MRG_D,                ///< DDD
  MRG_IVMC,             ///< Temporal inter-view
  MRG_A1,               ///< Left
  MRG_B1,               ///< Above
  MRG_VSP,              ///< VSP
  MRG_B0,               ///< Above right
  MRG_IVDC,             ///< Disparity inter-view
  MRG_A0,               ///< Left bottom
  MRG_B2,               ///< Above left
  MRG_IVSHIFT,          ///< Shifted IVMC of Shifted IVDC. (These are mutually exclusive)
  MRG_COL               ///< Temporal co-located
};
#endif
enum StoredResidualType
{
  RESIDUAL_RECONSTRUCTED          = 0,
  RESIDUAL_ENCODER_SIDE           = 1,
  NUMBER_OF_STORED_RESIDUAL_TYPES = 2
};
enum TransformDirection
{
  TRANSFORM_FORWARD              = 0,
  TRANSFORM_INVERSE              = 1,
  TRANSFORM_NUMBER_OF_DIRECTIONS = 2
};
/// supported ME search methods
enum MESearchMethod
{
  FULL_SEARCH                = 0,     ///< Full search
  DIAMOND                    = 1,     ///< Fast search
  SELECTIVE                  = 2      ///< Selective search
};
/// coefficient scanning type used in ACS
enum COEFF_SCAN_TYPE
{
  SCAN_DIAG = 0,        ///< up-right diagonal scan
  SCAN_HOR  = 1,        ///< horizontal first scan
  SCAN_VER  = 2,        ///< vertical first scan
  SCAN_NUMBER_OF_TYPES = 3
};
enum COEFF_SCAN_GROUP_TYPE
{
  SCAN_UNGROUPED   = 0,
  SCAN_GROUPED_4x4 = 1,
  SCAN_NUMBER_OF_GROUP_TYPES = 2
};
enum SignificanceMapContextType
{
  CONTEXT_TYPE_4x4    = 0,
  CONTEXT_TYPE_8x8    = 1,
  CONTEXT_TYPE_NxN    = 2,
  CONTEXT_TYPE_SINGLE = 3,
  CONTEXT_NUMBER_OF_TYPES = 4
};
enum ScalingListMode
{
  SCALING_LIST_OFF,
  SCALING_LIST_DEFAULT,
  SCALING_LIST_FILE_READ
};
enum ScalingListSize
{
  SCALING_LIST_4x4 = 0,
  SCALING_LIST_8x8,
  SCALING_LIST_16x16,
  SCALING_LIST_32x32,
  SCALING_LIST_SIZE_NUM
};
// Slice / Slice segment encoding modes
enum SliceConstraint
{
  NO_SLICES              = 0,          ///< don't use slices / slice segments
  FIXED_NUMBER_OF_CTU    = 1,          ///< Limit maximum number of largest coding tree units in a slice / slice segments
  FIXED_NUMBER_OF_BYTES  = 2,          ///< Limit maximum number of bytes in a slice / slice segment
  FIXED_NUMBER_OF_TILES  = 3,          ///< slices / slice segments span an integer number of tiles
};
enum SAOMode //mode
{
  SAO_MODE_OFF = 0,
  SAO_MODE_NEW,
  SAO_MODE_MERGE,
  NUM_SAO_MODES
};
enum SAOModeMergeTypes
{
  SAO_MERGE_LEFT =0,
  SAO_MERGE_ABOVE,
  NUM_SAO_MERGE_TYPES
};
enum SAOModeNewTypes
{
  SAO_TYPE_START_EO =0,
  SAO_TYPE_EO_0 = SAO_TYPE_START_EO,
  SAO_TYPE_EO_90,
  SAO_TYPE_EO_135,
  SAO_TYPE_EO_45,
  SAO_TYPE_START_BO,
  SAO_TYPE_BO = SAO_TYPE_START_BO,
  NUM_SAO_NEW_TYPES
};
#define NUM_SAO_EO_TYPES_LOG2 2
enum SAOEOClasses
{
  SAO_CLASS_EO_FULL_VALLEY = 0,
  SAO_CLASS_EO_HALF_VALLEY = 1,
  SAO_CLASS_EO_PLAIN       = 2,
  SAO_CLASS_EO_HALF_PEAK   = 3,
  SAO_CLASS_EO_FULL_PEAK   = 4,
  NUM_SAO_EO_CLASSES,
};
#define NUM_SAO_BO_CLASSES_LOG2  5
#define NUM_SAO_BO_CLASSES       (1<<NUM_SAO_BO_CLASSES_LOG2)
namespace Profile
{
  enum Name
  {
    NONE = 0,
    MAIN = 1,
    MAIN10 = 2,
    MAINSTILLPICTURE = 3,
    MAINREXT = 4,
    HIGHTHROUGHPUTREXT = 5
#if NH_MV
    ,MULTIVIEWMAIN = 6,
#if NH_3D
    MAIN3D = 8, 
#endif
#endif
  };
}
namespace Level
{
  enum Tier
  {
    MAIN = 0,
    HIGH = 1,
  };
  enum Name
  {
    // code = (level * 30)
    NONE     = 0,
    LEVEL1   = 30,
    LEVEL2   = 60,
    LEVEL2_1 = 63,
    LEVEL3   = 90,
    LEVEL3_1 = 93,
    LEVEL4   = 120,
    LEVEL4_1 = 123,
    LEVEL5   = 150,
    LEVEL5_1 = 153,
    LEVEL5_2 = 156,
    LEVEL6   = 180,
    LEVEL6_1 = 183,
    LEVEL6_2 = 186,
    LEVEL8_5 = 255,
  };
}
enum CostMode
{
  COST_STANDARD_LOSSY              = 0,
  COST_SEQUENCE_LEVEL_LOSSLESS     = 1,
  COST_LOSSLESS_CODING             = 2,
  COST_MIXED_LOSSLESS_LOSSY_CODING = 3
};
enum SPSExtensionFlagIndex
{
  SPS_EXT__REXT           = 0,
//SPS_EXT__MVHEVC         = 1, //for use in future versions
//SPS_EXT__SHVC           = 2, //for use in future versions
  NUM_SPS_EXTENSION_FLAGS = 8
};
enum PPSExtensionFlagIndex
{
  PPS_EXT__REXT           = 0,
//PPS_EXT__MVHEVC         = 1, //for use in future versions
//PPS_EXT__SHVC           = 2, //for use in future versions
  NUM_PPS_EXTENSION_FLAGS = 8
};
// TODO: Existing names used for the different NAL unit types can be altered to better reflect the names in the spec.
//       However, the names in the spec are not yet stable at this point. Once the names are stable, a cleanup
//       effort can be done without use of macros to alter the names used to indicate the different NAL unit types.
enum NalUnitType
{
  NAL_UNIT_CODED_SLICE_TRAIL_N = 0, // 0
  NAL_UNIT_CODED_SLICE_TRAIL_R,     // 1
  NAL_UNIT_CODED_SLICE_TSA_N,       // 2
  NAL_UNIT_CODED_SLICE_TSA_R,       // 3
  NAL_UNIT_CODED_SLICE_STSA_N,      // 4
  NAL_UNIT_CODED_SLICE_STSA_R,      // 5
  NAL_UNIT_CODED_SLICE_RADL_N,      // 6
  NAL_UNIT_CODED_SLICE_RADL_R,      // 7
  NAL_UNIT_CODED_SLICE_RASL_N,      // 8
  NAL_UNIT_CODED_SLICE_RASL_R,      // 9
  NAL_UNIT_RESERVED_VCL_N10,
  NAL_UNIT_RESERVED_VCL_R11,
  NAL_UNIT_RESERVED_VCL_N12,
  NAL_UNIT_RESERVED_VCL_R13,
  NAL_UNIT_RESERVED_VCL_N14,
  NAL_UNIT_RESERVED_VCL_R15,
  NAL_UNIT_CODED_SLICE_BLA_W_LP,    // 16
  NAL_UNIT_CODED_SLICE_BLA_W_RADL,  // 17
  NAL_UNIT_CODED_SLICE_BLA_N_LP,    // 18
  NAL_UNIT_CODED_SLICE_IDR_W_RADL,  // 19
  NAL_UNIT_CODED_SLICE_IDR_N_LP,    // 20
  NAL_UNIT_CODED_SLICE_CRA,         // 21
  NAL_UNIT_RESERVED_IRAP_VCL22,
  NAL_UNIT_RESERVED_IRAP_VCL23,
  NAL_UNIT_RESERVED_VCL24,
  NAL_UNIT_RESERVED_VCL25,
  NAL_UNIT_RESERVED_VCL26,
  NAL_UNIT_RESERVED_VCL27,
  NAL_UNIT_RESERVED_VCL28,
  NAL_UNIT_RESERVED_VCL29,
  NAL_UNIT_RESERVED_VCL30,
  NAL_UNIT_RESERVED_VCL31,
  NAL_UNIT_VPS,                     // 32
  NAL_UNIT_SPS,                     // 33
  NAL_UNIT_PPS,                     // 34
  NAL_UNIT_ACCESS_UNIT_DELIMITER,   // 35
  NAL_UNIT_EOS,                     // 36
  NAL_UNIT_EOB,                     // 37
  NAL_UNIT_FILLER_DATA,             // 38
  NAL_UNIT_PREFIX_SEI,              // 39
  NAL_UNIT_SUFFIX_SEI,              // 40
  NAL_UNIT_RESERVED_NVCL41,
  NAL_UNIT_RESERVED_NVCL42,
  NAL_UNIT_RESERVED_NVCL43,
  NAL_UNIT_RESERVED_NVCL44,
  NAL_UNIT_RESERVED_NVCL45,
  NAL_UNIT_RESERVED_NVCL46,
  NAL_UNIT_RESERVED_NVCL47,
  NAL_UNIT_UNSPECIFIED_48,
  NAL_UNIT_UNSPECIFIED_49,
  NAL_UNIT_UNSPECIFIED_50,
  NAL_UNIT_UNSPECIFIED_51,
  NAL_UNIT_UNSPECIFIED_52,
  NAL_UNIT_UNSPECIFIED_53,
  NAL_UNIT_UNSPECIFIED_54,
  NAL_UNIT_UNSPECIFIED_55,
  NAL_UNIT_UNSPECIFIED_56,
  NAL_UNIT_UNSPECIFIED_57,
  NAL_UNIT_UNSPECIFIED_58,
  NAL_UNIT_UNSPECIFIED_59,
  NAL_UNIT_UNSPECIFIED_60,
  NAL_UNIT_UNSPECIFIED_61,
  NAL_UNIT_UNSPECIFIED_62,
  NAL_UNIT_UNSPECIFIED_63,
  NAL_UNIT_INVALID,
};
#if NH_MV
/// scalability types
enum ScalabilityType
{
  DEPTH_ID = 0,    
  VIEW_ORDER_INDEX  = 1,
  DEPENDENCY_ID = 2,
  AUX_ID = 3,
};
#endif
#if NH_3D
// Renderer
enum BlenMod
{
  BLEND_NONE  = -1,
  BLEND_AVRG  = 0,
  BLEND_LEFT  = 1,
  BLEND_RIGHT = 2,
  BLEND_GEN   =  3
};
enum
{
  VIEWPOS_INVALID = -1,
  VIEWPOS_LEFT    = 0,
  VIEWPOS_RIGHT   = 1,
  VIEWPOS_MERGED  = 2
};
#endif
// ====================================================================================================================
// Type definition
// ====================================================================================================================
/// parameters for adaptive loop filter
class TComPicSym;
#define MAX_NUM_SAO_CLASSES  32  //(NUM_SAO_EO_GROUPS > NUM_SAO_BO_GROUPS)?NUM_SAO_EO_GROUPS:NUM_SAO_BO_GROUPS
struct SAOOffset
{
  SAOMode modeIdc; // NEW, MERGE, OFF
  Int typeIdc;     // union of SAOModeMergeTypes and SAOModeNewTypes, depending on modeIdc.
  Int typeAuxInfo; // BO: starting band index
  Int offset[MAX_NUM_SAO_CLASSES];
  SAOOffset();
  ~SAOOffset();
  Void reset();
  const SAOOffset& operator= (const SAOOffset& src);
};
struct SAOBlkParam
{
  SAOBlkParam();
  ~SAOBlkParam();
  Void reset();
  const SAOBlkParam& operator= (const SAOBlkParam& src);
  SAOOffset& operator[](Int compIdx){ return offsetParam[compIdx];}
private:
  SAOOffset offsetParam[MAX_NUM_COMPONENT];
};
struct BitDepths
{
#if O0043_BEST_EFFORT_DECODING
  Int recon[MAX_NUM_CHANNEL_TYPE]; ///< the bit depth used for reconstructing the video
  Int stream[MAX_NUM_CHANNEL_TYPE];///< the bit depth used indicated in the SPS
#else
  Int recon[MAX_NUM_CHANNEL_TYPE]; ///< the bit depth as indicated in the SPS
#endif
};
/// parameters for deblocking filter
typedef struct _LFCUParam
{
  Bool bInternalEdge;                     ///< indicates internal edge
  Bool bLeftEdge;                         ///< indicates left edge
  Bool bTopEdge;                          ///< indicates top edge
} LFCUParam;
//TU settings for entropy encoding
struct TUEntropyCodingParameters
{
  const UInt            *scan;
  const UInt            *scanCG;
        COEFF_SCAN_TYPE  scanType;
        UInt             widthInGroups;
        UInt             heightInGroups;
        UInt             firstSignificanceMapContext;
};
struct TComPictureHash
{
  std::vector<UChar> hash;
  Bool operator==(const TComPictureHash &other) const
  {
    if (other.hash.size() != hash.size())
    {
      return false;
    }
    for(UInt i=0; i<UInt(hash.size()); i++)
    {
      if (other.hash[i] != hash[i])
      {
        return false;
      }
    }
    return true;
  }
  Bool operator!=(const TComPictureHash &other) const
  {
    return !(*this == other);
  }
};
struct TComSEITimeSet
{
  TComSEITimeSet() : clockTimeStampFlag(false),
                     numUnitFieldBasedFlag(false),
                     countingType(0),
                     fullTimeStampFlag(false),
                     discontinuityFlag(false),
                     cntDroppedFlag(false),
                     numberOfFrames(0),
                     secondsValue(0),
                     minutesValue(0),
                     hoursValue(0),
                     secondsFlag(false),
                     minutesFlag(false),
                     hoursFlag(false),
                     timeOffsetLength(0),
                     timeOffsetValue(0)
  { }
  Bool clockTimeStampFlag;
  Bool numUnitFieldBasedFlag;
  Int  countingType;
  Bool fullTimeStampFlag;
  Bool discontinuityFlag;
  Bool cntDroppedFlag;
  Int  numberOfFrames;
  Int  secondsValue;
  Int  minutesValue;
  Int  hoursValue;
  Bool secondsFlag;
  Bool minutesFlag;
  Bool hoursFlag;
  Int  timeOffsetLength;
  Int  timeOffsetValue;
};
struct TComSEIMasteringDisplay
{
  Bool      colourVolumeSEIEnabled;
  UInt      maxLuminance;
  UInt      minLuminance;
  UShort    primaries[3][2];
  UShort    whitePoint[2];
};
//! \}
#if H_3D
#define !!! REMOVE THIS !!! Log2( n ) ( log((double)n) / log(2.0) ) // Ed.(GT): This is very very bad and should be fixed to used integer arithmetics ( see gCeilLog2 ) moreover it should not be defined in the tool macro section! 
#endif
#endif
