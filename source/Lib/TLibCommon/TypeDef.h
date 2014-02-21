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

/** \file     TypeDef.h
    \brief    Define basic types, new types and enumerations
*/

#ifndef _TYPEDEF__
#define _TYPEDEF__

//! \ingroup TLibCommon
//! \{
/////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////// EXTENSION SELECTION ///////////////////////////////////  
/////////////////////////////////////////////////////////////////////////////////////////

/* HEVC_EXT might be defined by compiler/makefile options.
   
   Linux makefiles support the following settings:   
   make             -> HEVC_EXT not defined    
   make HEVC_EXT=0  -> H_MV=0 H_3D=0   --> plain HM
   make HEVC_EXT=1  -> H_MV=1 H_3D=0   --> MV only 
   make HEVC_EXT=2  -> H_MV=1 H_3D=1   --> full 3D 
*/

#ifndef HEVC_EXT
#define HEVC_EXT                    1
#endif

#if ( HEVC_EXT < 0 )||( HEVC_EXT > 2 )
#error HEVC_EXT must be in the range of 0 to 2, inclusive. 
#endif

#define H_MV          ( HEVC_EXT != 0)
#define H_3D          ( HEVC_EXT == 2)


/////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////   MAJOR DEFINES   ///////////////////////////////////  
/////////////////////////////////////////////////////////////////////////////////////////

#if H_MV
#define H_MV_ENC_DEC_TRAC                 1  //< CU/PU level tracking
#endif

#if H_3D
#define H_3D_QTLPC                        1   // OL_QTLIMIT_PREDCODING_B0068 //JCT3V-B0068
                                              // HHI_QTLPC_RAU_OFF_C0160     // JCT3V-C0160 change 2: quadtree limitation and predictive coding switched off in random access units 

#define H_3D_VSO                          1   // VSO, View synthesis optimization, includes: 
                                              // HHI_VSO
                                              // HHI_VSO_LS_TABLE_M23714 enable table base Lagrange multiplier optimization
                                              // SAIT_VSO_EST_A0033, JCT3V-A0033 modification 3
                                              // LGE_WVSO_A0119
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
#define H_3D_ARP                          1   // Advanced residual prediction (ARP), JCT3V-D0177
                                              // QC_MTK_INTERVIEW_ARP_F0123_F0108 JCT3V-F0123; JCT3V-F0108
                                              // SHARP_ARP_REF_CHECK_F0105        ARP reference picture selection and DPB check
                                              // LGE_ARP_CTX_F0161                JCT3V-F0161
#define H_3D_IC                           1   // Illumination Compensation, JCT3V-B0045, JCT3V-C0046, JCT3V-D0060
                                              // Unifying rounding offset, for IC part, JCT3V-D0135
                                              // Full Pel Interpolation for Depth, HHI_FULL_PEL_DEPTH_MAP_MV_ACC
                                              // SHARP_ILLUCOMP_REFINE_E0046
                                              // MTK_CLIPPING_ALIGN_IC_E0168       // To support simplify bi-prediction PU with identical motion checking, JCT3V-E0168
                                              // LGE_IC_CTX_F0160 //JCT3V-F0160
                                              // SEC_ONLY_TEXTURE_IC_F0151

#if H_3D_NBDV
#define H_3D_NBDV_REF                     1   // Depth oriented neighboring block disparity derivation
                                              // MTK_D0156
                                              // MERL_D0166: Reference view selection in NBDV & Bi-VSP
                                              // MERL_C0152: Basic VSP
                                              // NBDV_DEFAULT_VIEWIDX_BUGFIX Bug fix for invalid default view index for NBDV
                                              // NTT_DoNBDV_VECTOR_CLIP_E0141 disparity vector clipping in DoNBDV, JCT3V-E0141 and JCT3V-E0209
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
#define H_3D_TMVP                         1   // QC_TMVP_C0047 
                                              // Sony_M23639

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

#define H_3D_INTER_SDC                    1   // INTER SDC, Inter simplified depth coding
                                              // LGE_INTER_SDC_E0156  Enable inter SDC for depth coding
#define H_3D_SPIVMP                       1   // H_3D_SPIVMP    // JCT3V-F0110: Sub-PU level inter-view motion prediction
#define H_3D_FCO                          0   // Flexible coding order for 3D



// OTHERS
                                              // MTK_SONY_PROGRESSIVE_MV_COMPRESSION_E0170 // Progressive MV Compression, JCT3V-E0170
#define H_3D_REN_MAX_DEV_OUT              0   // Output maximal possible shift deviation 
#define H_3D_FAST_TEXTURE_ENCODING        1   // Fast merge mode decision and early CU determination for texture component of dependent view, JCT3V-E0173
                                              // MTK_FAST_TEXTURE_ENCODING_E0173
#if H_3D_DIM
#define H_3D_FAST_DEPTH_INTRA             1   // Fast DMM and RBC Mode Selection
                                              // SCU_HS_FAST_DEPTH_INTRA_E0238_HHIFIX
#endif

// Rate Control
#define KWU_FIX_URQ                       1
#define KWU_RC_VIEWRC_E0227               0  ///< JCT3V-E0227, view-wise target bitrate allocation
#define KWU_RC_MADPRED_E0227              0  ///< JCT3V-E0227, inter-view MAD prediction

#endif // H_3D



/////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////   DERIVED DEFINES ///////////////////////////////////  
/////////////////////////////////////////////////////////////////////////////////////////

///// ***** VIEW SYNTHESIS OPTIMIZAION *********
#if H_3D_VSO                                  
#define H_3D_VSO_DIST_INT                 1   // Allow negative synthesized view distortion change
#define H_3D_VSO_COLOR_PLANES             1   // Compute VSO distortion on color planes 
#define H_3D_VSO_EARLY_SKIP               1   // LGE_VSO_EARLY_SKIP_A0093, A0093 modification 4
#define H_3D_VSO_RM_ASSERTIONS            0   // Output VSO assertions
#define H_3D_VSO_SYNTH_DIST_OUT           0   // Output of synthesized view distortion instead of depth distortion in encoder output
#define H_3D_VSO_FIX                      0   // This fix should be enabled after verification 
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
#define H_3D_DIM_DMM                      1   // Depth Modeling Modes
#define H_3D_DIM_SDC                      1   // Simplified Depth Coding method
#define H_3D_DIM_DLT                      1   // Depth Lookup Table

#if H_3D_DIM_DLT
#define H_3D_DELTA_DLT                    1
#endif
#define H_3D_DIM_ENC                      1   // Depth Intra encoder optimizations, includes:
                                              // HHI_DEPTH_INTRA_SEARCH_RAU_C0160
                                              // LG_ZEROINTRADEPTHRESI_A0087
#endif
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


/////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////   HTM-10.0 Integrations //////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////
#if H_3D
#if  H_3D_QTLPC
#define MTK_TEX_DEP_PAR_G0055             1   // Texture-partition-dependent depth partition. JCT3V-G0055
#endif

#define MTK_DDD_G0063                     1   // Disparity derived depth coding

#if H_3D_VSP
#define MTK_RBIP_VSP_G0069                1   // Restricted bi-prediction for VSP
#define NTT_STORE_SPDV_VSP_G0148          1   // Storing Sub-PU based DV for VSP
#endif

#define H_3D_DBBP                         1   // DBBP: Depth-based Block Partitioning and Merging

#if H_3D_DBBP
#define DBBP_INVALID_SHORT                (-4)
#define RWTH_DBBP_PACK_MODE               SIZE_2NxN
#endif

#if H_3D_DIM
#define QC_PKU_SDC_SPLIT_G0123            1   // Intra SDC Split
#if QC_PKU_SDC_SPLIT_G0123
#define HS_TSINGHUA_SDC_SPLIT_G0111       1
#endif
#define SCU_HS_DEPTH_DC_PRED_G0143        1
#define QC_GENERIC_SDC_G0122              1  // Generalize SDC to all depth intra modes
#if H_3D_DIM_SDC && H_3D_INTER_SDC
#define QC_SDC_UNIFY_G0130                1  // Unify intra SDC and inter SDC
#define QC_SDC_UNIFY_G0130_FIX            1  // Fix bug of G0130
#define QC_SDC_UNIFY_G0130_FIX2           1  // Fix bug of G0130
#endif
#define SEC_INTER_SDC_G0101               1  // Improved inter SDC with multiple DC candidates
#endif

#define KHU_SIMP_SPIVMP_G0147             1  // Simplification on Sub-PU level temporal interview motion prediction

#if H_3D_SPIVMP
#define QC_SPIVMP_MPI_G0119               1 // Sub-PU level MPI merge candidate
#endif

#define QC_DEPTH_MERGE_SIMP_G0127         1 // Remove DV candidate and shifting candidate for depth coding
#define SEC_IC_ARP_SIG_G0072              1   // Disabling IC when ARP is enabled, option 1 in JCT3V-G0072, part 2 in JCT3V-G0121
#define SCU_HS_VSD_BUGFIX_IMPROV_G0163    1
#define SEC_SPIVMP_MCP_SIZE_G0077         1  // Apply SPIVMP only to 2Nx2N partition, JCT3V-G0077
#define SEC_DEPTH_DV_DERIVAITON_G0074     1  // Simplification of DV derivation for depth, JCT3V-G0074
#define MTK_ARP_REF_SELECTION_G0053       1   // ARP Reference picture selection in JCT3V-G0053 
#define MTK_ARP_FLAG_CABAC_SIMP_G0061     1   // Use 2 context for ARP flag referring to only left neighbor block in JCT3V-G0061
#define MTK_IC_FLAG_CABAC_SIMP_G0061      1   // Use only 1 context for IC flag in JCT3V-G0061
#define MTK_NBDV_IVREF_FIX_G0067          1   // Disable IvMC, VSP when IVREF is not available, JCT3V-G0067
#endif

/////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////   HTM-10.1 Integrations //////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////


// TBD

// #define H_MV_HLS_7_ED_FIX_P0130_34        0 // (ED.FIX/P0130/il ref pic set no reference pic) #34 For proposal 5, delegated to editors
// #define H_MV_HLS_7_OTHER_P0187_1          0 // (OTHER/P0187/NoOutputOfPriorPicsFlag) #1 Inference of NoOutputOfPriorPicsFlag and proposes to take into account colour format and bit depth for the inference in addition to spatial resolution 
// #define H_MV_HLS_7_VPS_P0306_22           0 // (VPS/P0306/ue(v) coded syntax elements) #22 Several minor modifications to the VPS syntax, consistent with eliminating the previous intention to avoid ue(v) parsing in the VPS 
// #define H_MV_HLS_7_SEI_P0133_28           0 // (SEI/P0133/Recovery point SEI) #28 Decision: Adopt change to recover point semantics only (-v3)
#define H_MV_HLS_7_VPS_P0125_24           1 // (VPS/P0125/VPS extension offset ) #24 Decision: Keep it as a reserved FFFF value. 
#define H_MV_HLS_7_VPS_P0307_23           1 // (VPS/P0307/VPS VUI extension)  #23 Decision: Adopt modification in P0307.
// #define H_MV_HLS_7_POC_P0041_3            0 // (POC/P0041/POC reset) #3 It was remarked that we should require each non-IRAP picture that has discardable_flag equal to 1 to have NUT value indicating that it is a sub-layer non-reference picture. This was agreed. Decision: Adopt (with constraint for discardable_flag as described above) 
// #define H_MV_HLS_7_POC_P0041_FIXES        0 // (POC/P0041/Fixes) For each non-IRAP picture that has discardable_flag equal to 1 to have NUT value indicating that it is a sub-layer non-reference picture. 
// #define H_MV_HLS_7_SEI_P0204_26           0 // (SEI/P0204/sub-bitstream SEI) #26 Add sub-bitstream property SEI message. Decision: Adopt
// #define H_MV_HLS_7_MISC_P0130_20          0 // (MISC/P0130/discardable not in inter-layer RPS) #20 Add constraint restricting pictures marked as discardable from being present in the temporal or inter-layer RPS,


// #define H_MV_HLS_7_SEI_P0123_25           0 // (SEI/P0123/Alpha channel info) #25 Add alpha channel information SEI message Decision: Adopt. Constrain the bit depth indicated to be equal to the coded bit depth of the aux picture. 
// #define H_MV_HLS_7_VPS_P0300_27           0 // Output part only. (VPS/P0300/alt output layer flag) #27 Change alt output layer flag to be signalled within the loop of output layer sets, from JCTVC-P0300-v2. Decision: Adopt. 
// #define H_MV_HLS_7_POC_P0056_4            0 // (POC/P0056/layer tree poc) #4 Proposal 1: If the POC reset approach is adopted as the basis for multi-layer POC derivation, it is proposed to derive the POC anchor picture from the previous TID0 picture (that is not a RASL picture, a RADL picture or a sub-layer non-reference picture and not with discardable_flag equal to 1) of  the current layer or any of its reference layer. This is asserted to improve loss resilience and reduce bit rate overhead. Decision: Adopt Proposal 1 (with the suggested modifications – with text provided as P0297).
// #define H_MV_HLS_7_HRD_P0138_6            0 // (HRD/P0138/HRD parameters for bitstreams excluding) #6 Decision: Adopt (as revised in updated contribution, with the specification of a flag in the BP SEI (HRD/P0192/sub-DPB) #12 Establish sub-DPBs based on the representation format indicated at the VPS level. It was suggested that the expressed shared capacity limit would need to be less than or equal to the sum of the individual capacity limits. Decision: Adopt as modified. Further study is encouraged on profile/level constraint selections. 





#define H_MV_HLS7_GEN                          0  // General changes (not tested)
#define H_MV_HLS_7_OUTPUT_LAYERS_5_10_22_27    1  // Output layer sets, various
                                                  // (VPS/P0300/alt output layer flag) #27 Change alt output layer flag to be signalled within the loop of output layer sets, from JCTVC-P0300-v2. Decision: Adopt. 
                                                  // (VPS/P0156/Num of output_layer_flag) #10 Proposal 3: The output_layer_flag[ i ][ j ] is signalled for j equal to 0 to NumLayersInIdList[ lsIdx ] inclusive. It was remarked that we might be able to just assume that the top layer is always output; however, this was not entirely clear , so the safe thing to do may be to also send the flag for this layer.
                                                  // (VPS/P0295/Default output layer sets) #5 Discussion from (P0110). Decision: Three-state approach (text in P0295, decoder shall allow 3 to be present and shall treat 3 the same as the value 2). 


#define H_MV_HLS_7_HRD_P0156_7                 1  // (HRD/P0156/MaxSubLayersInLayerSetMinus1) #7 Proposal 1: signal, in the VPS extension, the DPB parameters for an output layer set for sub-DPBs only up to the maximum temporal sub-layers in the corresponding layer set
#define H_MV_HLS_7_VPS_P0048_14                1  // (VPS/P0048/profile_ref_minus1 rem) #14 Remove profile_ref_minus1 from the VPS extension, from JCTVC-P0048
#define H_MV_HLS_7_VPS_P0076_15                1  // (VPS/P0076/video signal info move) #15 Move video signal information syntax structure earlier in the VPS VUI.
#define H_MV_HLS_7_SPS_P0155_16_32             1  // (SPS/P0155/sps_sub_layer_ordering_info) #16, #32 Not signal the sps_max_num_reorder_pics[], sps_max_latency_increase_plus1[], and sps_max_dec_pic_buffering_minus1[] syntax elements in the SPS when nuh_layer_id > 0. 
#define H_MV_HLS_7_GEN_P0166_PPS_EXTENSION     1  // (GEN/P0166/pps_extension) #17 Add PPS extension type flags for conditional presence of syntax extensions per extension type, aligned with the SPS extension type flags, from JCTVC-P0166. Further align the SPS extension type flags syntax between RExt and MV-HEVC/SHVC
#define H_MV_HLS_7_FIX_SET_DPB_SIZE            1  // Fix derivation dpb size parameters
#define H_MV_HLS_7_RESERVED_FLAGS              1  // Added flags
                                                  // (SPS/P0312/SHVC reserved flag) The flag will be used for the syntax vert_phase_position_enable_flag in SHVC draft
                                                  // (VPS/O0215/SHVC reserved flag): this flag will be used for the syntax cross_layer_phase_alignment_flag in SHVC draft.
                                                  // (VPS VUI/O0199,P0312/SHVC reserved flags) the 3 reserved bits will be used for the syntaxes single_layer_for_non_irap_flag, higher_layer_irap_skip_flag and vert_phase_position_not_in_use_flag in SHVC draft.
#define H_MV_FIX_VPS_LAYER_ID_NOT_EQUAL_ZERO   1  // Discard VPS with nuh_layer_Id > 0
#define H_MV_HLS_7_MISC_P0130_EOS              1  // (MISC/P0130/EOS NAL layer id) #19 Require that end of bitstream NAL unit shall have nuh_layer_id equal to 0, from JCTVC-P0130. Decoders shall allow an end of bitstream NAL unit with nuh_layer_id > 0 to be present, and shall ignore the NAL unit.
#define H_MV_HLS_7_MISC_P0182_13               1  // (MISC/P0182/BL PS Compatibility flag) #13 Define the flag (in VPS VUI) with the proposed semantics, without specifying an associated extraction process. Editors to select the position in the VPS VUI.
#define H_MV_HLS_7_MISC_P0068_21               1  // (MISC/P0068/all irap idr flag) #21 Add flag in VUI to indicate that all IRAP pictures are IDRs and that all layer pictures in an AU are IDR aligned, from JCTVC-P0068 proposal 1.
#define H_MV_HLS_7_FIX_INFER_CROSS_LAYER_IRAP_ALIGNED_FLAG               1  // Fix inference of cross_layer_irap_aligned_flag
#define H_MV_HLS_7_MISC_P0079_18               1  // (MISC/P0079/NumActiveRefLayerPics) #18 Modification of derivation of variable NumActiveRefLayerPics.
#define FIX_CAM_PARS_COLLECTOR                 1
#define UPDATE_HM13                            1  // Only some parts in H_3D parts are marked! 
#if H_3D
#define H_3D_FIX_G0148_BRACE              1 
#endif
/////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////   HM RELATED DEFINES ////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////
#define BUGFIX_INTRAPERIOD 1
#define SAO_ENCODE_ALLOW_USE_PREDEBLOCK 1

#define FIX1172 1 ///< fix ticket #1172

#define MAX_NUM_PICS_IN_SOP           1024

#define MAX_NESTING_NUM_OPS         1024
#define MAX_NESTING_NUM_LAYER       64

#define MAX_VPS_NUM_HRD_PARAMETERS                1
#define MAX_VPS_OP_SETS_PLUS1                     1024
#if H_MV
#define MAX_VPS_NUH_LAYER_ID_PLUS1  63
#define MAX_NUM_SCALABILITY_TYPES   16
#define ENC_CFG_CONSOUT_SPACE       29           
#else
#define MAX_VPS_NUH_RESERVED_ZERO_LAYER_ID_PLUS1  1
#endif


#define MAX_CPB_CNT                     32  ///< Upper bound of (cpb_cnt_minus1 + 1)
#if H_MV
#define MAX_NUM_LAYER_IDS               63
#define MAX_NUM_LAYERS                  63
#define MAX_VPS_PROFILE_TIER_LEVEL      64
#define MAX_VPS_ADD_OUTPUT_LAYER_SETS   1024
#define MAX_VPS_OUTPUTLAYER_SETS        ( MAX_VPS_ADD_OUTPUT_LAYER_SETS + MAX_VPS_OP_SETS_PLUS1 )
#define  MAX_NUM_VIDEO_SIGNAL_INFO      16
#define MAX_NUM_SCALED_REF_LAYERS       MAX_NUM_LAYERS-1
#define MAX_NUM_BSP_HRD_PARAMETERS      100 ///< Maximum value is actually not specified
#define MAX_NUM_BITSTREAM_PARTITIONS    100 ///< Maximum value is actually not specified 
#define MAX_NUM_BSP_SCHED_COMBINATION   100 ///< Maximum value is actually not specified 
#else
#define MAX_NUM_LAYER_IDS                64
#endif

#define COEF_REMAIN_BIN_REDUCTION        3 ///< indicates the level at which the VLC 
                                           ///< transitions from Golomb-Rice to TU+EG(k)

#define CU_DQP_TU_CMAX 5                   ///< max number bins for truncated unary
#define CU_DQP_EG_k 0                      ///< expgolomb order

#define SBH_THRESHOLD                    4  ///< I0156: value of the fixed SBH controlling threshold
  
#define SEQUENCE_LEVEL_LOSSLESS           0  ///< H0530: used only for sequence or frame-level lossless coding

#define DISABLING_CLIP_FOR_BIPREDME         1  ///< Ticket #175
  
#define C1FLAG_NUMBER               8 // maximum number of largerThan1 flag coded in one chunk :  16 in HM5
#define C2FLAG_NUMBER               1 // maximum number of largerThan2 flag coded in one chunk:  16 in HM5 
#define SAO_ENCODING_CHOICE              1  ///< I0184: picture early termination
#if SAO_ENCODING_CHOICE
#define SAO_ENCODING_RATE                0.75
#define SAO_ENCODING_CHOICE_CHROMA       1 ///< J0044: picture early termination Luma and Chroma are handled separately
#if SAO_ENCODING_CHOICE_CHROMA
#define SAO_ENCODING_RATE_CHROMA         0.5
#endif
#endif

#define MAX_NUM_VPS                16
#define MAX_NUM_SPS                16
#define MAX_NUM_PPS                64

#define RDOQ_CHROMA_LAMBDA          1   ///< F386: weighting of chroma for RDOQ

#define MIN_SCAN_POS_CROSS          4

#define FAST_BIT_EST                1   ///< G763: Table-based bit estimation for CABAC

#define MLS_GRP_NUM                         64     ///< G644 : Max number of coefficient groups, max(16, 64)
#define MLS_CG_SIZE                         4      ///< G644 : Coefficient group size of 4x4

#define ADAPTIVE_QP_SELECTION               1      ///< G382: Adaptive reconstruction levels, non-normative part for adaptive QP selection
#if ADAPTIVE_QP_SELECTION
#define ARL_C_PRECISION                     7      ///< G382: 7-bit arithmetic precision
#define LEVEL_RANGE                         30     ///< G382: max coefficient level in statistics collection
#endif

#define HHI_RQT_INTRA_SPEEDUP             1           ///< tests one best mode with full rqt
#define HHI_RQT_INTRA_SPEEDUP_MOD         0           ///< tests two best modes with full rqt

#if HHI_RQT_INTRA_SPEEDUP_MOD && !HHI_RQT_INTRA_SPEEDUP
#error
#endif

#define VERBOSE_RATE 0 ///< Print additional rate information in encoder

#define AMVP_DECIMATION_FACTOR            4

#define SCAN_SET_SIZE                     16
#define LOG2_SCAN_SET_SIZE                4

#define FAST_UDI_MAX_RDMODE_NUM               35          ///< maximum number of RD comparison in fast-UDI estimation loop 

#define ZERO_MVD_EST                          0           ///< Zero Mvd Estimation in normal mode

#define NUM_INTRA_MODE 36
#if !REMOVE_LM_CHROMA
#define LM_CHROMA_IDX  35
#endif

#define WRITE_BACK                      1           ///< Enable/disable the encoder to replace the deltaPOC and Used by current from the config file with the values derived by the refIdc parameter.
#define AUTO_INTER_RPS                  1           ///< Enable/disable the automatic generation of refIdc from the deltaPOC and Used by current from the config file.
#define PRINT_RPS_INFO                  0           ///< Enable/disable the printing of bits used to send the RPS.
                                                    // using one nearest frame as reference frame, and the other frames are high quality (POC%4==0) frames (1+X)
                                                    // this should be done with encoder only decision
                                                    // but because of the absence of reference frame management, the related code was hard coded currently

#define RVM_VCEGAM10_M 4

#define PLANAR_IDX             0
#define VER_IDX                26                    // index for intra VERTICAL   mode
#define HOR_IDX                10                    // index for intra HORIZONTAL mode
#define DC_IDX                 1                     // index for intra DC mode
#define NUM_CHROMA_MODE        5                     // total number of chroma modes
#define DM_CHROMA_IDX          36                    // chroma mode index for derived from luma intra mode


#define FAST_UDI_USE_MPM 1

#define RDO_WITHOUT_DQP_BITS              0           ///< Disable counting dQP bits in RDO-based mode decision

#define FULL_NBIT 0 ///< When enabled, compute costs using full sample bitdepth.  When disabled, compute costs as if it is 8-bit source video.
#if FULL_NBIT
# define DISTORTION_PRECISION_ADJUSTMENT(x) 0
#else
# define DISTORTION_PRECISION_ADJUSTMENT(x) (x)
#endif

#define LOG2_MAX_NUM_COLUMNS_MINUS1        7
#define LOG2_MAX_NUM_ROWS_MINUS1           7
#define LOG2_MAX_COLUMN_WIDTH              13
#define LOG2_MAX_ROW_HEIGHT                13

#define MATRIX_MULT                             0   // Brute force matrix multiplication instead of partial butterfly

#define REG_DCT 65535

#define AMP_SAD                               1           ///< dedicated SAD functions for AMP
#define AMP_ENC_SPEEDUP                       1           ///< encoder only speed-up by AMP mode skipping
#if AMP_ENC_SPEEDUP
#define AMP_MRG                               1           ///< encoder only force merge for AMP partition (no motion search for AMP)
#endif

#define CABAC_INIT_PRESENT_FLAG     1

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
// Type definition
// ====================================================================================================================

typedef       UChar           Pxl;        ///< 8-bit pixel type
typedef       Short           Pel;        ///< 16-bit pixel type
typedef       Int             TCoeff;     ///< transform coefficient

#if H_3D_VSO
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
/// parameters for adaptive loop filter
class TComPicSym;

// Slice / Slice segment encoding modes
enum SliceConstraint
{
  NO_SLICES              = 0,          ///< don't use slices / slice segments
  FIXED_NUMBER_OF_LCU    = 1,          ///< Limit maximum number of largest coding tree blocks in a slice / slice segments
  FIXED_NUMBER_OF_BYTES  = 2,          ///< Limit maximum number of bytes in a slice / slice segment
  FIXED_NUMBER_OF_TILES  = 3,          ///< slices / slice segments span an integer number of tiles
};

enum SAOComponentIdx
{
  SAO_Y =0,
  SAO_Cb,
  SAO_Cr,
  NUM_SAO_COMPONENTS
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
enum SAOBOClasses
{
  //SAO_CLASS_BO_BAND0 = 0,
  //SAO_CLASS_BO_BAND1,
  //SAO_CLASS_BO_BAND2,
  //...
  //SAO_CLASS_BO_BAND31,

  NUM_SAO_BO_CLASSES = (1<<NUM_SAO_BO_CLASSES_LOG2),
};
#define MAX_NUM_SAO_CLASSES  32  //(NUM_SAO_EO_GROUPS > NUM_SAO_BO_GROUPS)?NUM_SAO_EO_GROUPS:NUM_SAO_BO_GROUPS

struct SAOOffset
{
  Int modeIdc; //NEW, MERGE, OFF
  Int typeIdc; //NEW: EO_0, EO_90, EO_135, EO_45, BO. MERGE: left, above
  Int typeAuxInfo; //BO: starting band index
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
  SAOOffset offsetParam[NUM_SAO_COMPONENTS];

};

/// parameters for deblocking filter
typedef struct _LFCUParam
{
  Bool bInternalEdge;                     ///< indicates internal edge
  Bool bLeftEdge;                         ///< indicates left edge
  Bool bTopEdge;                          ///< indicates top edge
} LFCUParam;

// ====================================================================================================================
// Enumeration
// ====================================================================================================================

/// supported slice type
enum SliceType
{
  B_SLICE,
  P_SLICE,
  I_SLICE
};

/// chroma formats (according to semantics of chroma_format_idc)
enum ChromaFormat
{
  CHROMA_400  = 0,
  CHROMA_420  = 1,
  CHROMA_422  = 2,
  CHROMA_444  = 3
};

/// supported partition shape
enum PartSize
{
  SIZE_2Nx2N,           ///< symmetric motion partition,  2Nx2N
  SIZE_2NxN,            ///< symmetric motion partition,  2Nx N
  SIZE_Nx2N,            ///< symmetric motion partition,   Nx2N
  SIZE_NxN,             ///< symmetric motion partition,   Nx N
  SIZE_2NxnU,           ///< asymmetric motion partition, 2Nx( N/2) + 2Nx(3N/2)
  SIZE_2NxnD,           ///< asymmetric motion partition, 2Nx(3N/2) + 2Nx( N/2)
  SIZE_nLx2N,           ///< asymmetric motion partition, ( N/2)x2N + (3N/2)x2N
  SIZE_nRx2N,           ///< asymmetric motion partition, (3N/2)x2N + ( N/2)x2N
  SIZE_NONE = 15
};

/// supported prediction type
enum PredMode
{
  MODE_INTER,           ///< inter-prediction mode
  MODE_INTRA,           ///< intra-prediction mode
  MODE_NONE = 15
};

/// texture component type
enum TextType
{
  TEXT_LUMA,            ///< luma
  TEXT_CHROMA,          ///< chroma (U+V)
  TEXT_CHROMA_U,        ///< chroma U
  TEXT_CHROMA_V,        ///< chroma V
  TEXT_ALL,             ///< Y+U+V
  TEXT_NONE = 15
};

/// reference list index
enum RefPicList
{
  REF_PIC_LIST_0 = 0,   ///< reference list 0
  REF_PIC_LIST_1 = 1,   ///< reference list 1
  REF_PIC_LIST_X = 100  ///< special mark
};

/// distortion function index
enum DFunc
{
  DF_DEFAULT  = 0,
  DF_SSE      = 1,      ///< general size SSE
  DF_SSE4     = 2,      ///<   4xM SSE
  DF_SSE8     = 3,      ///<   8xM SSE
  DF_SSE16    = 4,      ///<  16xM SSE
  DF_SSE32    = 5,      ///<  32xM SSE
  DF_SSE64    = 6,      ///<  64xM SSE
  DF_SSE16N   = 7,      ///< 16NxM SSE
  
  DF_SAD      = 8,      ///< general size SAD
  DF_SAD4     = 9,      ///<   4xM SAD
  DF_SAD8     = 10,     ///<   8xM SAD
  DF_SAD16    = 11,     ///<  16xM SAD
  DF_SAD32    = 12,     ///<  32xM SAD
  DF_SAD64    = 13,     ///<  64xM SAD
  DF_SAD16N   = 14,     ///< 16NxM SAD
  
  DF_SADS     = 15,     ///< general size SAD with step
  DF_SADS4    = 16,     ///<   4xM SAD with step
  DF_SADS8    = 17,     ///<   8xM SAD with step
  DF_SADS16   = 18,     ///<  16xM SAD with step
  DF_SADS32   = 19,     ///<  32xM SAD with step
  DF_SADS64   = 20,     ///<  64xM SAD with step
  DF_SADS16N  = 21,     ///< 16NxM SAD with step
  
  DF_HADS     = 22,     ///< general size Hadamard with step
  DF_HADS4    = 23,     ///<   4xM HAD with step
  DF_HADS8    = 24,     ///<   8xM HAD with step
  DF_HADS16   = 25,     ///<  16xM HAD with step
  DF_HADS32   = 26,     ///<  32xM HAD with step
  DF_HADS64   = 27,     ///<  64xM HAD with step
  DF_HADS16N  = 28,     ///< 16NxM HAD with step
#if H_3D_VSO
  DF_VSD      = 29,      ///< general size VSD
  DF_VSD4     = 30,      ///<   4xM VSD
  DF_VSD8     = 31,      ///<   8xM VSD
  DF_VSD16    = 32,      ///<  16xM VSD
  DF_VSD32    = 33,      ///<  32xM VSD
  DF_VSD64    = 34,      ///<  64xM VSD
  DF_VSD16N   = 35,      ///< 16NxM VSD
#endif

#if AMP_SAD
  DF_SAD12    = 43,
  DF_SAD24    = 44,
  DF_SAD48    = 45,

  DF_SADS12   = 46,
  DF_SADS24   = 47,
  DF_SADS48   = 48,

  DF_SSE_FRAME = 50     ///< Frame-based SSE
#else
  DF_SSE_FRAME = 33     ///< Frame-based SSE
#endif
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

/// coefficient scanning type used in ACS
enum COEFF_SCAN_TYPE
{
  SCAN_DIAG = 0,         ///< up-right diagonal scan
  SCAN_HOR,              ///< horizontal first scan
  SCAN_VER               ///< vertical first scan
};

namespace Profile
{
  enum Name
  {
    NONE = 0,
    MAIN = 1,
    MAIN10 = 2,
    MAINSTILLPICTURE = 3,
#if H_MV
    MAINSTEREO = 4,
    MAINMULTIVIEW = 5,
#if H_3D
    MAIN3D = 6, 
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
  };
}
//! \}

#if H_MV

#if H_MV_HLS_7_GEN_P0166_PPS_EXTENSION
enum PpsExtensionTypes
{
  PPS_EX_T_MV      = 0,
#if H_3D
  PPS_EX_T_3D      = 3,
#endif
  PPS_EX_T_ESC     = 7,
  PPS_EX_T_MAX_NUM = 8
};

//Below for sps, would be good if this could be aligned
#endif

  enum PsExtensionTypes
  {
    PS_EX_T_MV   = 1,
#if H_3D
    PS_EX_T_3D   = 3,
#endif
    PS_EX_T_ESC  = 7,
    PS_EX_T_MAX_NUM = 8
  };

/// scalability types
  enum ScalabilityType
  {
#if H_3D
    DEPTH_ID = 0,    
#endif    
    VIEW_ORDER_INDEX  = 1,
  };
#endif
#if H_3D
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

#define Log2( n ) ( log((double)n) / log(2.0) ) // Ed.(GT): This is very very bad and should be fixed to used integer arithmetics ( see gCeilLog2 ) moreover it should not be defined in the tool macro section! 
#endif
#endif
