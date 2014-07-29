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

/** \file     TEncCavlc.cpp
    \brief    CAVLC encoder class
*/

#include "../TLibCommon/CommonDef.h"
#include "TEncCavlc.h"
#include "SEIwrite.h"
#include "../TLibCommon/TypeDef.h"

//! \ingroup TLibEncoder
//! \{

#if ENC_DEC_TRACE

Void  xTraceSPSHeader (TComSPS *pSPS)
{
#if H_MV_ENC_DEC_TRAC
  fprintf( g_hTrace, "=========== Sequence Parameter Set ===========\n" );
#else
  fprintf( g_hTrace, "=========== Sequence Parameter Set ID: %d ===========\n", pSPS->getSPSId() );
#endif
}

Void  xTracePPSHeader (TComPPS *pPPS)
{
#if H_MV_ENC_DEC_TRAC
  fprintf( g_hTrace, "=========== Picture Parameter Set ===========\n" );
#else
  fprintf( g_hTrace, "=========== Picture Parameter Set ID: %d ===========\n", pPPS->getPPSId() );
#endif
}

Void  xTraceSliceHeader (TComSlice *pSlice)
{
  fprintf( g_hTrace, "=========== Slice ===========\n");
}

#endif



// ====================================================================================================================
// Constructor / destructor / create / destroy
// ====================================================================================================================

TEncCavlc::TEncCavlc()
{
  m_pcBitIf           = NULL;
  m_uiCoeffCost       = 0;
}

TEncCavlc::~TEncCavlc()
{
}


// ====================================================================================================================
// Public member functions
// ====================================================================================================================

Void TEncCavlc::resetEntropy()
{
}


Void TEncCavlc::codeDFFlag(UInt uiCode, const Char *pSymbolName)
{
  WRITE_FLAG(uiCode, pSymbolName);
}
Void TEncCavlc::codeDFSvlc(Int iCode, const Char *pSymbolName)
{
  WRITE_SVLC(iCode, pSymbolName);
}

Void TEncCavlc::codeShortTermRefPicSet( TComSPS* pcSPS, TComReferencePictureSet* rps, Bool calledFromSliceHeader, Int idx)
{
#if PRINT_RPS_INFO
  Int lastBits = getNumberOfWrittenBits();
#endif
  if (idx > 0)
  {
  WRITE_FLAG( rps->getInterRPSPrediction(), "inter_ref_pic_set_prediction_flag" ); // inter_RPS_prediction_flag
  }
  if (rps->getInterRPSPrediction())
  {
    Int deltaRPS = rps->getDeltaRPS();
    if(calledFromSliceHeader)
    {
      WRITE_UVLC( rps->getDeltaRIdxMinus1(), "delta_idx_minus1" ); // delta index of the Reference Picture Set used for prediction minus 1
    }

    WRITE_CODE( (deltaRPS >=0 ? 0: 1), 1, "delta_rps_sign" ); //delta_rps_sign
    WRITE_UVLC( abs(deltaRPS) - 1, "abs_delta_rps_minus1"); // absolute delta RPS minus 1

    for(Int j=0; j < rps->getNumRefIdc(); j++)
    {
      Int refIdc = rps->getRefIdc(j);
      WRITE_CODE( (refIdc==1? 1: 0), 1, "used_by_curr_pic_flag" ); //first bit is "1" if Idc is 1 
      if (refIdc != 1) 
      {
        WRITE_CODE( refIdc>>1, 1, "use_delta_flag" ); //second bit is "1" if Idc is 2, "0" otherwise.
      }
    }
  }
  else
  {
    WRITE_UVLC( rps->getNumberOfNegativePictures(), "num_negative_pics" );
    WRITE_UVLC( rps->getNumberOfPositivePictures(), "num_positive_pics" );

    Int prev = 0;
    for(Int j=0 ; j < rps->getNumberOfNegativePictures(); j++)
    {
      WRITE_UVLC( prev-rps->getDeltaPOC(j)-1, "delta_poc_s0_minus1" );
      prev = rps->getDeltaPOC(j);
      WRITE_FLAG( rps->getUsed(j), "used_by_curr_pic_s0_flag"); 
    }
    prev = 0;
    for(Int j=rps->getNumberOfNegativePictures(); j < rps->getNumberOfNegativePictures()+rps->getNumberOfPositivePictures(); j++)
    {
      WRITE_UVLC( rps->getDeltaPOC(j)-prev-1, "delta_poc_s1_minus1" );
      prev = rps->getDeltaPOC(j);
      WRITE_FLAG( rps->getUsed(j), "used_by_curr_pic_s1_flag" ); 
    }
  }

#if PRINT_RPS_INFO
  printf("irps=%d (%2d bits) ", rps->getInterRPSPrediction(), getNumberOfWrittenBits() - lastBits);
  rps->printDeltaPOC();
#endif
}


Void TEncCavlc::codePPS( TComPPS* pcPPS )
{
#if ENC_DEC_TRACE  
  xTracePPSHeader (pcPPS);
#endif
  
  WRITE_UVLC( pcPPS->getPPSId(),                             "pps_pic_parameter_set_id" );
  WRITE_UVLC( pcPPS->getSPSId(),                             "pps_seq_parameter_set_id" );
  WRITE_FLAG( pcPPS->getDependentSliceSegmentsEnabledFlag()    ? 1 : 0, "dependent_slice_segments_enabled_flag" );
  WRITE_FLAG( pcPPS->getOutputFlagPresentFlag() ? 1 : 0,     "output_flag_present_flag" );
  WRITE_CODE( pcPPS->getNumExtraSliceHeaderBits(), 3,        "num_extra_slice_header_bits");
  WRITE_FLAG( pcPPS->getSignHideFlag(), "sign_data_hiding_flag" );
  WRITE_FLAG( pcPPS->getCabacInitPresentFlag() ? 1 : 0,   "cabac_init_present_flag" );
#if PPS_FIX_DEPTH
  if( pcPPS->getSPS()->getVPS()->getDepthId(pcPPS->getSPS()->getLayerId()) )
  {
    WRITE_UVLC( pcPPS->getNumRefIdxL0DefaultActive(),     "num_ref_idx_l0_default_active_minus1");
    WRITE_UVLC( pcPPS->getNumRefIdxL1DefaultActive(),     "num_ref_idx_l1_default_active_minus1");
  }
  else
  {
#endif
  WRITE_UVLC( pcPPS->getNumRefIdxL0DefaultActive()-1,     "num_ref_idx_l0_default_active_minus1");
  WRITE_UVLC( pcPPS->getNumRefIdxL1DefaultActive()-1,     "num_ref_idx_l1_default_active_minus1");
#if PPS_FIX_DEPTH
  }
#endif
  WRITE_SVLC( pcPPS->getPicInitQPMinus26(),                  "init_qp_minus26");
  WRITE_FLAG( pcPPS->getConstrainedIntraPred() ? 1 : 0,      "constrained_intra_pred_flag" );
  WRITE_FLAG( pcPPS->getUseTransformSkip() ? 1 : 0,  "transform_skip_enabled_flag" ); 
  WRITE_FLAG( pcPPS->getUseDQP() ? 1 : 0, "cu_qp_delta_enabled_flag" );
  if ( pcPPS->getUseDQP() )
  {
    WRITE_UVLC( pcPPS->getMaxCuDQPDepth(), "diff_cu_qp_delta_depth" );
  }
  WRITE_SVLC( pcPPS->getChromaCbQpOffset(),                   "pps_cb_qp_offset" );
  WRITE_SVLC( pcPPS->getChromaCrQpOffset(),                   "pps_cr_qp_offset" );
  WRITE_FLAG( pcPPS->getSliceChromaQpFlag() ? 1 : 0,          "pps_slice_chroma_qp_offsets_present_flag" );

  WRITE_FLAG( pcPPS->getUseWP() ? 1 : 0,  "weighted_pred_flag" );   // Use of Weighting Prediction (P_SLICE)
  WRITE_FLAG( pcPPS->getWPBiPred() ? 1 : 0, "weighted_bipred_flag" );  // Use of Weighting Bi-Prediction (B_SLICE)
  WRITE_FLAG( pcPPS->getTransquantBypassEnableFlag() ? 1 : 0, "transquant_bypass_enable_flag" );
  WRITE_FLAG( pcPPS->getTilesEnabledFlag()             ? 1 : 0, "tiles_enabled_flag" );
  WRITE_FLAG( pcPPS->getEntropyCodingSyncEnabledFlag() ? 1 : 0, "entropy_coding_sync_enabled_flag" );
  if( pcPPS->getTilesEnabledFlag() )
  {
    WRITE_UVLC( pcPPS->getNumColumnsMinus1(),                                    "num_tile_columns_minus1" );
    WRITE_UVLC( pcPPS->getNumRowsMinus1(),                                       "num_tile_rows_minus1" );
    WRITE_FLAG( pcPPS->getUniformSpacingFlag(),                                  "uniform_spacing_flag" );
    if( pcPPS->getUniformSpacingFlag() == 0 )
    {
      for(UInt i=0; i<pcPPS->getNumColumnsMinus1(); i++)
      {
        WRITE_UVLC( pcPPS->getColumnWidth(i)-1,                                  "column_width_minus1" );
      }
      for(UInt i=0; i<pcPPS->getNumRowsMinus1(); i++)
      {
        WRITE_UVLC( pcPPS->getRowHeight(i)-1,                                    "row_height_minus1" );
      }
    }
    if(pcPPS->getNumColumnsMinus1() !=0 || pcPPS->getNumRowsMinus1() !=0)
    {
      WRITE_FLAG( pcPPS->getLoopFilterAcrossTilesEnabledFlag()?1 : 0,          "loop_filter_across_tiles_enabled_flag");
    }
  }
  WRITE_FLAG( pcPPS->getLoopFilterAcrossSlicesEnabledFlag()?1 : 0,        "loop_filter_across_slices_enabled_flag");
  WRITE_FLAG( pcPPS->getDeblockingFilterControlPresentFlag()?1 : 0,       "deblocking_filter_control_present_flag");
  if(pcPPS->getDeblockingFilterControlPresentFlag())
  {
    WRITE_FLAG( pcPPS->getDeblockingFilterOverrideEnabledFlag() ? 1 : 0,  "deblocking_filter_override_enabled_flag" ); 
    WRITE_FLAG( pcPPS->getPicDisableDeblockingFilterFlag() ? 1 : 0,       "pps_disable_deblocking_filter_flag" );
    if(!pcPPS->getPicDisableDeblockingFilterFlag())
    {
      WRITE_SVLC( pcPPS->getDeblockingFilterBetaOffsetDiv2(),             "pps_beta_offset_div2" );
      WRITE_SVLC( pcPPS->getDeblockingFilterTcOffsetDiv2(),               "pps_tc_offset_div2" );
    }
  }
#if H_MV
  if ( pcPPS->getLayerId() > 0 )
  {
    WRITE_FLAG( pcPPS->getPpsInferScalingListFlag( ) ? 1 : 0 , "pps_infer_scaling_list_flag" );
  }

  if( pcPPS->getPpsInferScalingListFlag( ) ) 
  {
    WRITE_CODE( pcPPS->getPpsScalingListRefLayerId( ), 6, "pps_scaling_list_ref_layer_id" );
  }
  else
  {  
#endif  
  WRITE_FLAG( pcPPS->getScalingListPresentFlag() ? 1 : 0,                          "pps_scaling_list_data_present_flag" ); 
  if( pcPPS->getScalingListPresentFlag() )
  {
    codeScalingList( m_pcSlice->getScalingList() );
  }
#if H_MV
  }
#endif
#if PPS_FIX_DEPTH
  if( pcPPS->getSPS()->getVPS()->getDepthId(pcPPS->getSPS()->getLayerId()) )
  {
    WRITE_FLAG( 1, "lists_modification_present_flag" );
  }
  else
#endif
  WRITE_FLAG( pcPPS->getListsModificationPresentFlag(), "lists_modification_present_flag");
  WRITE_UVLC( pcPPS->getLog2ParallelMergeLevelMinus2(), "log2_parallel_merge_level_minus2");
  WRITE_FLAG( pcPPS->getSliceHeaderExtensionPresentFlag() ? 1 : 0, "slice_segment_header_extension_present_flag");

  #if H_3D
    if(( !pcPPS->getSPS()->getVPS()->getDepthId( pcPPS->getSPS()->getLayerId() ) )||
        pcPPS->getLayerId() != 1  )
    {
      pcPPS->setPps3dExtensionFlag( false ); 
    }
  #endif

#if !H_MV
  WRITE_FLAG( 0, "pps_extension_flag" );
#else
  WRITE_FLAG( 1, "pps_extension_present_flag" );

    WRITE_FLAG( pcPPS->getPpsRangeExtensionsFlag( ) ? 1 : 0 , "pps_range_extensions_flag" );
    WRITE_FLAG( pcPPS->getPpsMultilayerExtensionFlag( ) ? 1 : 0 , "pps_multilayer_extension_flag" );
#if !H_3D
    WRITE_CODE( pcPPS->getPpsExtension6bits( ), 6, "pps_extension_6bits" );
#else
    WRITE_FLAG( pcPPS->getPps3dExtensionFlag( ) ? 1 : 0 , "pps_3d_extension_flag" );
    WRITE_CODE( pcPPS->getPpsExtension5bits( ), 5, "pps_extension_5bits" );
#endif
    if ( pcPPS->getPpsRangeExtensionsFlag() )
    { 
      assert(0); 
    }

    if ( pcPPS->getPpsMultilayerExtensionFlag() )
    { 
      WRITE_FLAG( pcPPS->getPocResetInfoPresentFlag( ) ? 1 : 0 , "poc_reset_info_present_flag" );
      WRITE_FLAG( 0, "pps_extension_reserved_zero_flag" );
    }

#if H_3D
    if( pcPPS->getPps3dExtensionFlag( )  ) // This probably needs to be aligned with Rext and SHVC
    {
      codePPSExtension( pcPPS ); 
    }
#endif
#endif

}

#if H_3D
Void  TEncCavlc::codePPSExtension        ( TComPPS* pcPPS )
{
  // Assuming that all PPS indirectly refer to the same VPS via different SPS
  // There is no parsing dependency in decoding DLT in PPS. 
  // The VPS information passed to decodePPS() is used to arrange the decoded DLT tables to their corresponding layers. 
  // This is equivalent to the process of 
  //   Step 1) decoding DLT tables based on the number of depth layers, and
  //   Step 2) mapping DLT tables to the depth layers
  // as described in the 3D-HEVC WD.
  TComVPS* pcVPS = pcPPS->getSPS()->getVPS();

  TComDLT* pcDLT = pcPPS->getDLT();

  WRITE_FLAG( pcDLT->getDltPresentFlag() ? 1 : 0, "dlt_present_flag" );

  if ( pcDLT->getDltPresentFlag() )
  {
    WRITE_CODE(pcDLT->getNumDepthViews(), 6, "pps_depth_layers_minus1");
    WRITE_CODE((pcDLT->getDepthViewBitDepth() - 8), 4, "pps_bit_depth_for_depth_views_minus8");

    for( Int i = 0; i <= pcVPS->getMaxLayersMinus1(); i++ )
    {
      if ( i != 0 )
      {
        if ( pcVPS->getDepthId( i ) == 1 )
        {
          WRITE_FLAG( pcDLT->getUseDLTFlag( i ) ? 1 : 0, "dlt_flag[i]" );

          if ( pcDLT->getUseDLTFlag( i ) )
          {
            WRITE_FLAG( pcDLT->getInterViewDltPredEnableFlag( i ) ? 1 : 0, "inter_view_dlt_pred_enable_flag[ i ]");

            // ----------------------------- determine whether to use bit-map -----------------------------
            Bool bDltBitMapRepFlag       = false;
            UInt uiNumBitsNonBitMap      = 0;
            UInt uiNumBitsBitMap         = 0;

            UInt uiMaxDiff               = 0;
            UInt uiMinDiff               = 0xffffffff;
            UInt uiLengthMinDiff         = 0;
            UInt uiLengthDltDiffMinusMin = 0;

            UInt* puiDltDiffValues       = NULL;
            
            Int aiIdx2DepthValue_coded[256];
            UInt uiNumDepthValues_coded = 0;
            
            uiNumDepthValues_coded = pcDLT->getNumDepthValues(i);
            for( UInt ui = 0; ui<uiNumDepthValues_coded; ui++ )
            {
              aiIdx2DepthValue_coded[ui] = pcDLT->idx2DepthValue(i, ui);
            }
            
#if H_3D_DELTA_DLT
            if( pcDLT->getInterViewDltPredEnableFlag( i ) )
            {
              AOF( pcVPS->getDepthId( 1 ) == 1 );
              AOF( i > 1 );
              // assumes ref layer id to be 1
              Int* piRefDLT = pcDLT->idx2DepthValue( 1 );
              UInt uiRefNum = pcDLT->getNumDepthValues( 1 );
              pcDLT->getDeltaDLT(i, piRefDLT, uiRefNum, aiIdx2DepthValue_coded, &uiNumDepthValues_coded);
            }
#endif

            if ( NULL == (puiDltDiffValues = (UInt *)calloc(uiNumDepthValues_coded, sizeof(UInt))) )
            {
              exit(-1);
            }

            for (UInt d = 1; d < uiNumDepthValues_coded; d++)
            {
              puiDltDiffValues[d] = aiIdx2DepthValue_coded[d] - aiIdx2DepthValue_coded[d-1];

              if ( uiMaxDiff < puiDltDiffValues[d] )
              {
                uiMaxDiff = puiDltDiffValues[d];
              }

              if ( uiMinDiff > puiDltDiffValues[d] )
              {
                uiMinDiff = puiDltDiffValues[d];
              }
            }

            // counting bits
            // diff coding branch
            uiNumBitsNonBitMap += 8;                          // u(v) bits for num_depth_values_in_dlt[layerId] (i.e. num_entry[ layerId ])

            if ( uiNumDepthValues_coded > 1 )
            {
              uiNumBitsNonBitMap += 8;                        // u(v) bits for max_diff[ layerId ]
            }

            if ( uiNumDepthValues_coded > 2 )
            {
              uiLengthMinDiff    = (UInt) ceil(Log2(uiMaxDiff + 1));
              uiNumBitsNonBitMap += uiLengthMinDiff;          // u(v)  bits for min_diff[ layerId ]
            }

            uiNumBitsNonBitMap += 8;                          // u(v) bits for dlt_depth_value0[ layerId ]

            if (uiMaxDiff > uiMinDiff)
            {
              uiLengthDltDiffMinusMin = (UInt) ceil(Log2(uiMaxDiff - uiMinDiff + 1));
              uiNumBitsNonBitMap += uiLengthDltDiffMinusMin * (uiNumDepthValues_coded - 1);  // u(v) bits for dlt_depth_value_diff_minus_min[ layerId ][ j ]
            }

            // bit map branch
            uiNumBitsBitMap = 256;   // uiNumBitsBitMap = 1 << pcDLT->getDepthViewBitDepth(); 

            // determine bDltBitMapFlag
            bDltBitMapRepFlag = (uiNumBitsBitMap > uiNumBitsNonBitMap) ? false : true;

            // ----------------------------- Actual coding -----------------------------
            if ( pcDLT->getInterViewDltPredEnableFlag( i ) == false )
            {
              WRITE_FLAG( bDltBitMapRepFlag ? 1 : 0, "dlt_bit_map_rep_flag[ layerId ]" );
            }
            else
            {
              bDltBitMapRepFlag = false;
            }

            // bit map coding
            if ( bDltBitMapRepFlag )
            {
              UInt uiDltArrayIndex = 0; 
              for (UInt d=0; d < 256; d++)
              {
                if ( d == aiIdx2DepthValue_coded[uiDltArrayIndex] )
                {                  
                  WRITE_FLAG(1, "dlt_bit_map_flag[ layerId ][ j ]");
                  uiDltArrayIndex++;
                }
                else
                {
                  WRITE_FLAG(0, "dlt_bit_map_flag[ layerId ][ j ]");
                }
              }
            }
            // Diff Coding
            else
            {
              WRITE_CODE(uiNumDepthValues_coded, 8, "num_depth_values_in_dlt[i]");    // num_entry

#if !H_3D_DELTA_DLT
              if ( pcDLT->getInterViewDltPredEnableFlag( i ) == false )   // Single-view DLT Diff Coding
#endif
              {
                // The condition if( uiNumDepthValues_coded > 0 ) is always true since for Single-view Diff Coding, there is at least one depth value in depth component.
                if ( uiNumDepthValues_coded > 1 )
                {
                  WRITE_CODE(uiMaxDiff, 8, "max_diff[ layerId ]");        // max_diff
                }

                if ( uiNumDepthValues_coded > 2 )
                {
                  WRITE_CODE((uiMinDiff - 1), uiLengthMinDiff, "min_diff_minus1[ layerId ]");     // min_diff_minus1
                }

                WRITE_CODE(aiIdx2DepthValue_coded[0], 8, "dlt_depth_value0[layerId]");          // entry0

                if (uiMaxDiff > uiMinDiff)
                {
                  for (UInt d=1; d < uiNumDepthValues_coded; d++)
                  {
                    WRITE_CODE( (puiDltDiffValues[d] - uiMinDiff), uiLengthDltDiffMinusMin, "dlt_depth_value_diff_minus_min[ layerId ][ j ]");    // entry_value_diff_minus_min[ k ]
                  }
                }
              }
            }

            free(puiDltDiffValues);
          }
        }
      }
    }
  }
}
#endif

Void TEncCavlc::codeVUI( TComVUI *pcVUI, TComSPS* pcSPS )
{
#if ENC_DEC_TRACE
  fprintf( g_hTrace, "----------- vui_parameters -----------\n");
#endif
  WRITE_FLAG(pcVUI->getAspectRatioInfoPresentFlag(),            "aspect_ratio_info_present_flag");
  if (pcVUI->getAspectRatioInfoPresentFlag())
  {
    WRITE_CODE(pcVUI->getAspectRatioIdc(), 8,                   "aspect_ratio_idc" );
    if (pcVUI->getAspectRatioIdc() == 255)
    {
      WRITE_CODE(pcVUI->getSarWidth(), 16,                      "sar_width");
      WRITE_CODE(pcVUI->getSarHeight(), 16,                     "sar_height");
    }
  }
  WRITE_FLAG(pcVUI->getOverscanInfoPresentFlag(),               "overscan_info_present_flag");
  if (pcVUI->getOverscanInfoPresentFlag())
  {
    WRITE_FLAG(pcVUI->getOverscanAppropriateFlag(),             "overscan_appropriate_flag");
  }
  WRITE_FLAG(pcVUI->getVideoSignalTypePresentFlag(),            "video_signal_type_present_flag");
#if H_MV
  assert( pcSPS->getLayerId() == 0 || !pcVUI->getVideoSignalTypePresentFlag() ); 
#endif
  if (pcVUI->getVideoSignalTypePresentFlag())
  {
    WRITE_CODE(pcVUI->getVideoFormat(), 3,                      "video_format");
    WRITE_FLAG(pcVUI->getVideoFullRangeFlag(),                  "video_full_range_flag");
    WRITE_FLAG(pcVUI->getColourDescriptionPresentFlag(),        "colour_description_present_flag");
    if (pcVUI->getColourDescriptionPresentFlag())
    {
      WRITE_CODE(pcVUI->getColourPrimaries(), 8,                "colour_primaries");
      WRITE_CODE(pcVUI->getTransferCharacteristics(), 8,        "transfer_characteristics");
      WRITE_CODE(pcVUI->getMatrixCoefficients(), 8,             "matrix_coefficients");
    }
  }

  WRITE_FLAG(pcVUI->getChromaLocInfoPresentFlag(),              "chroma_loc_info_present_flag");
  if (pcVUI->getChromaLocInfoPresentFlag())
  {
    WRITE_UVLC(pcVUI->getChromaSampleLocTypeTopField(),         "chroma_sample_loc_type_top_field");
    WRITE_UVLC(pcVUI->getChromaSampleLocTypeBottomField(),      "chroma_sample_loc_type_bottom_field");
  }

  WRITE_FLAG(pcVUI->getNeutralChromaIndicationFlag(),           "neutral_chroma_indication_flag");
  WRITE_FLAG(pcVUI->getFieldSeqFlag(),                          "field_seq_flag");
  WRITE_FLAG(pcVUI->getFrameFieldInfoPresentFlag(),             "frame_field_info_present_flag");

  Window defaultDisplayWindow = pcVUI->getDefaultDisplayWindow();
  WRITE_FLAG(defaultDisplayWindow.getWindowEnabledFlag(),       "default_display_window_flag");
  if( defaultDisplayWindow.getWindowEnabledFlag() )
  {
    WRITE_UVLC(defaultDisplayWindow.getWindowLeftOffset(),      "def_disp_win_left_offset");
    WRITE_UVLC(defaultDisplayWindow.getWindowRightOffset(),     "def_disp_win_right_offset");
    WRITE_UVLC(defaultDisplayWindow.getWindowTopOffset(),       "def_disp_win_top_offset");
    WRITE_UVLC(defaultDisplayWindow.getWindowBottomOffset(),    "def_disp_win_bottom_offset");
  }
  TimingInfo *timingInfo = pcVUI->getTimingInfo();
  WRITE_FLAG(timingInfo->getTimingInfoPresentFlag(),          "vui_timing_info_present_flag");
  if(timingInfo->getTimingInfoPresentFlag())
  {
    WRITE_CODE(timingInfo->getNumUnitsInTick(), 32,           "vui_num_units_in_tick");
    WRITE_CODE(timingInfo->getTimeScale(),      32,           "vui_time_scale");
    WRITE_FLAG(timingInfo->getPocProportionalToTimingFlag(),  "vui_poc_proportional_to_timing_flag");
    if(timingInfo->getPocProportionalToTimingFlag())
    {
      WRITE_UVLC(timingInfo->getNumTicksPocDiffOneMinus1(),   "vui_num_ticks_poc_diff_one_minus1");
    }
  WRITE_FLAG(pcVUI->getHrdParametersPresentFlag(),              "hrd_parameters_present_flag");
  if( pcVUI->getHrdParametersPresentFlag() )
  {
    codeHrdParameters(pcVUI->getHrdParameters(), 1, pcSPS->getMaxTLayers() - 1 );
  }
  }

  WRITE_FLAG(pcVUI->getBitstreamRestrictionFlag(),              "bitstream_restriction_flag");
  if (pcVUI->getBitstreamRestrictionFlag())
  {
    WRITE_FLAG(pcVUI->getTilesFixedStructureFlag(),             "tiles_fixed_structure_flag");
    WRITE_FLAG(pcVUI->getMotionVectorsOverPicBoundariesFlag(),  "motion_vectors_over_pic_boundaries_flag");
    WRITE_FLAG(pcVUI->getRestrictedRefPicListsFlag(),           "restricted_ref_pic_lists_flag");
    WRITE_UVLC(pcVUI->getMinSpatialSegmentationIdc(),           "min_spatial_segmentation_idc");
    WRITE_UVLC(pcVUI->getMaxBytesPerPicDenom(),                 "max_bytes_per_pic_denom");
    WRITE_UVLC(pcVUI->getMaxBitsPerMinCuDenom(),                "max_bits_per_mincu_denom");
    WRITE_UVLC(pcVUI->getLog2MaxMvLengthHorizontal(),           "log2_max_mv_length_horizontal");
    WRITE_UVLC(pcVUI->getLog2MaxMvLengthVertical(),             "log2_max_mv_length_vertical");
  }
}

Void TEncCavlc::codeHrdParameters( TComHRD *hrd, Bool commonInfPresentFlag, UInt maxNumSubLayersMinus1 )
{
  if( commonInfPresentFlag )
  {
    WRITE_FLAG( hrd->getNalHrdParametersPresentFlag() ? 1 : 0 ,  "nal_hrd_parameters_present_flag" );
    WRITE_FLAG( hrd->getVclHrdParametersPresentFlag() ? 1 : 0 ,  "vcl_hrd_parameters_present_flag" );
    if( hrd->getNalHrdParametersPresentFlag() || hrd->getVclHrdParametersPresentFlag() )
    {
      WRITE_FLAG( hrd->getSubPicCpbParamsPresentFlag() ? 1 : 0,  "sub_pic_cpb_params_present_flag" );
      if( hrd->getSubPicCpbParamsPresentFlag() )
      {
        WRITE_CODE( hrd->getTickDivisorMinus2(), 8,              "tick_divisor_minus2" );
        WRITE_CODE( hrd->getDuCpbRemovalDelayLengthMinus1(), 5,  "du_cpb_removal_delay_length_minus1" );
        WRITE_FLAG( hrd->getSubPicCpbParamsInPicTimingSEIFlag() ? 1 : 0, "sub_pic_cpb_params_in_pic_timing_sei_flag" );
        WRITE_CODE( hrd->getDpbOutputDelayDuLengthMinus1(), 5,   "dpb_output_delay_du_length_minus1"  );
      }
      WRITE_CODE( hrd->getBitRateScale(), 4,                     "bit_rate_scale" );
      WRITE_CODE( hrd->getCpbSizeScale(), 4,                     "cpb_size_scale" );
      if( hrd->getSubPicCpbParamsPresentFlag() )
      {
        WRITE_CODE( hrd->getDuCpbSizeScale(), 4,                "du_cpb_size_scale" ); 
      }
      WRITE_CODE( hrd->getInitialCpbRemovalDelayLengthMinus1(), 5, "initial_cpb_removal_delay_length_minus1" );
      WRITE_CODE( hrd->getCpbRemovalDelayLengthMinus1(),        5, "au_cpb_removal_delay_length_minus1" );
      WRITE_CODE( hrd->getDpbOutputDelayLengthMinus1(),         5, "dpb_output_delay_length_minus1" );
    }
  }
  Int i, j, nalOrVcl;
  for( i = 0; i <= maxNumSubLayersMinus1; i ++ )
  {
    WRITE_FLAG( hrd->getFixedPicRateFlag( i ) ? 1 : 0,          "fixed_pic_rate_general_flag");
    if( !hrd->getFixedPicRateFlag( i ) )
    {
      WRITE_FLAG( hrd->getFixedPicRateWithinCvsFlag( i ) ? 1 : 0, "fixed_pic_rate_within_cvs_flag");
    }
    else
    {
      hrd->setFixedPicRateWithinCvsFlag( i, true );
    }
    if( hrd->getFixedPicRateWithinCvsFlag( i ) )
    {
      WRITE_UVLC( hrd->getPicDurationInTcMinus1( i ),           "elemental_duration_in_tc_minus1");
    }
    else
    {
      WRITE_FLAG( hrd->getLowDelayHrdFlag( i ) ? 1 : 0,           "low_delay_hrd_flag");
    }
    if (!hrd->getLowDelayHrdFlag( i ))
    {
      WRITE_UVLC( hrd->getCpbCntMinus1( i ),                      "cpb_cnt_minus1");
    }
    
    for( nalOrVcl = 0; nalOrVcl < 2; nalOrVcl ++ )
    {
      if( ( ( nalOrVcl == 0 ) && ( hrd->getNalHrdParametersPresentFlag() ) ) ||
          ( ( nalOrVcl == 1 ) && ( hrd->getVclHrdParametersPresentFlag() ) ) )
      {
        for( j = 0; j <= ( hrd->getCpbCntMinus1( i ) ); j ++ )
        {
          WRITE_UVLC( hrd->getBitRateValueMinus1( i, j, nalOrVcl ), "bit_rate_value_minus1");
          WRITE_UVLC( hrd->getCpbSizeValueMinus1( i, j, nalOrVcl ), "cpb_size_value_minus1");
          if( hrd->getSubPicCpbParamsPresentFlag() )
          {
            WRITE_UVLC( hrd->getDuCpbSizeValueMinus1( i, j, nalOrVcl ), "cpb_size_du_value_minus1");  
            WRITE_UVLC( hrd->getDuBitRateValueMinus1( i, j, nalOrVcl ), "bit_rate_du_value_minus1");
          }
          WRITE_FLAG( hrd->getCbrFlag( i, j, nalOrVcl ) ? 1 : 0, "cbr_flag");
        }
      }
    }
  }
}

#if H_3D
Void TEncCavlc::codeSPS( TComSPS* pcSPS, Int viewIndex, Bool depthFlag )
#else
Void TEncCavlc::codeSPS( TComSPS* pcSPS )
#endif
{
#if ENC_DEC_TRACE  
  xTraceSPSHeader (pcSPS);
#endif
  WRITE_CODE( pcSPS->getVPSId (),          4,       "sps_video_parameter_set_id" );
#if H_MV
  if ( pcSPS->getLayerId() == 0 )
  {
#endif
  WRITE_CODE( pcSPS->getMaxTLayers() - 1,  3,       "sps_max_sub_layers_minus1" );
  WRITE_FLAG( pcSPS->getTemporalIdNestingFlag() ? 1 : 0,                             "sps_temporal_id_nesting_flag" );
  codePTL(pcSPS->getPTL(), 1, pcSPS->getMaxTLayers() - 1);
#if H_MV
}
#endif
  WRITE_UVLC( pcSPS->getSPSId (),                   "sps_seq_parameter_set_id" );
#if H_MV
  if ( pcSPS->getLayerId() > 0 )
  {
    WRITE_FLAG( pcSPS->getUpdateRepFormatFlag( ) ? 1 : 0 , "update_rep_format_flag" );
    if ( pcSPS->getUpdateRepFormatFlag() )
    { 
      WRITE_CODE( pcSPS->getSpsRepFormatIdx( ), 8, "sps_rep_format_idx" );
    }
  }
  else
  {
#endif
  WRITE_UVLC( pcSPS->getChromaFormatIdc (),         "chroma_format_idc" );
  assert(pcSPS->getChromaFormatIdc () == 1);
  // in the first version chroma_format_idc can only be equal to 1 (4:2:0)
  if( pcSPS->getChromaFormatIdc () == 3 )
  {
    WRITE_FLAG( 0,                                  "separate_colour_plane_flag");
  }

  WRITE_UVLC( pcSPS->getPicWidthInLumaSamples (),   "pic_width_in_luma_samples" );
  WRITE_UVLC( pcSPS->getPicHeightInLumaSamples(),   "pic_height_in_luma_samples" );
#if H_MV
  }
#endif
  Window conf = pcSPS->getConformanceWindow();

  WRITE_FLAG( conf.getWindowEnabledFlag(),          "conformance_window_flag" );
  if (conf.getWindowEnabledFlag())
  {
    WRITE_UVLC( conf.getWindowLeftOffset()   / TComSPS::getWinUnitX(pcSPS->getChromaFormatIdc() ), "conf_win_left_offset" );
    WRITE_UVLC( conf.getWindowRightOffset()  / TComSPS::getWinUnitX(pcSPS->getChromaFormatIdc() ), "conf_win_right_offset" );
    WRITE_UVLC( conf.getWindowTopOffset()    / TComSPS::getWinUnitY(pcSPS->getChromaFormatIdc() ), "conf_win_top_offset" );
    WRITE_UVLC( conf.getWindowBottomOffset() / TComSPS::getWinUnitY(pcSPS->getChromaFormatIdc() ), "conf_win_bottom_offset" );
  }
#if H_MV
  if ( pcSPS->getLayerId() == 0 )
  { 
#endif
  WRITE_UVLC( pcSPS->getBitDepthY() - 8,             "bit_depth_luma_minus8" );
  WRITE_UVLC( pcSPS->getBitDepthC() - 8,             "bit_depth_chroma_minus8" );
#if H_MV
  }
#endif
  WRITE_UVLC( pcSPS->getBitsForPOC()-4,                 "log2_max_pic_order_cnt_lsb_minus4" );
#if H_MV
  if ( pcSPS->getLayerId() == 0 ) 
  {  
#endif
  const Bool subLayerOrderingInfoPresentFlag = 1;
  WRITE_FLAG(subLayerOrderingInfoPresentFlag,       "sps_sub_layer_ordering_info_present_flag");
  for(UInt i=0; i <= pcSPS->getMaxTLayers()-1; i++)
  {
    WRITE_UVLC( pcSPS->getMaxDecPicBuffering(i) - 1,       "sps_max_dec_pic_buffering_minus1[i]" );
    WRITE_UVLC( pcSPS->getNumReorderPics(i),               "sps_num_reorder_pics[i]" );
    WRITE_UVLC( pcSPS->getMaxLatencyIncrease(i),           "sps_max_latency_increase_plus1[i]" );
    if (!subLayerOrderingInfoPresentFlag)
    {
      break;
    }
  }
#if H_MV
  }
#endif

  assert( pcSPS->getMaxCUWidth() == pcSPS->getMaxCUHeight() );
  
  WRITE_UVLC( pcSPS->getLog2MinCodingBlockSize() - 3,                                "log2_min_coding_block_size_minus3" );
  WRITE_UVLC( pcSPS->getLog2DiffMaxMinCodingBlockSize(),                             "log2_diff_max_min_coding_block_size" );
  WRITE_UVLC( pcSPS->getQuadtreeTULog2MinSize() - 2,                                 "log2_min_transform_block_size_minus2" );
  WRITE_UVLC( pcSPS->getQuadtreeTULog2MaxSize() - pcSPS->getQuadtreeTULog2MinSize(), "log2_diff_max_min_transform_block_size" );
  WRITE_UVLC( pcSPS->getQuadtreeTUMaxDepthInter() - 1,                               "max_transform_hierarchy_depth_inter" );
  WRITE_UVLC( pcSPS->getQuadtreeTUMaxDepthIntra() - 1,                               "max_transform_hierarchy_depth_intra" );
  WRITE_FLAG( pcSPS->getScalingListFlag() ? 1 : 0,                                   "scaling_list_enabled_flag" ); 
  if(pcSPS->getScalingListFlag())
  {
#if H_MV
    if ( pcSPS->getLayerId() > 0 )
    {    
      WRITE_FLAG( pcSPS->getSpsInferScalingListFlag( ) ? 1 : 0 , "sps_infer_scaling_list_flag" );
    }

    if ( pcSPS->getSpsInferScalingListFlag() )
    {
      WRITE_CODE( pcSPS->getSpsScalingListRefLayerId( ), 6, "sps_scaling_list_ref_layer_id" );
    }
    else
    {    
#endif
    WRITE_FLAG( pcSPS->getScalingListPresentFlag() ? 1 : 0,                          "sps_scaling_list_data_present_flag" ); 
    if(pcSPS->getScalingListPresentFlag())
    {
      codeScalingList( m_pcSlice->getScalingList() );
    }
#if H_MV
    }
#endif
  }
  WRITE_FLAG( pcSPS->getUseAMP() ? 1 : 0,                                            "amp_enabled_flag" );
  WRITE_FLAG( pcSPS->getUseSAO() ? 1 : 0,                                            "sample_adaptive_offset_enabled_flag");

  WRITE_FLAG( pcSPS->getUsePCM() ? 1 : 0,                                            "pcm_enabled_flag");
  if( pcSPS->getUsePCM() )
  {
    WRITE_CODE( pcSPS->getPCMBitDepthLuma() - 1, 4,                                  "pcm_sample_bit_depth_luma_minus1" );
    WRITE_CODE( pcSPS->getPCMBitDepthChroma() - 1, 4,                                "pcm_sample_bit_depth_chroma_minus1" );
    WRITE_UVLC( pcSPS->getPCMLog2MinSize() - 3,                                      "log2_min_pcm_luma_coding_block_size_minus3" );
    WRITE_UVLC( pcSPS->getPCMLog2MaxSize() - pcSPS->getPCMLog2MinSize(),             "log2_diff_max_min_pcm_luma_coding_block_size" );
    WRITE_FLAG( pcSPS->getPCMFilterDisableFlag()?1 : 0,                              "pcm_loop_filter_disable_flag");
  }

  assert( pcSPS->getMaxTLayers() > 0 );         

  TComRPSList* rpsList = pcSPS->getRPSList();
  TComReferencePictureSet*      rps;

  WRITE_UVLC(rpsList->getNumberOfReferencePictureSets(), "num_short_term_ref_pic_sets" );
  for(Int i=0; i < rpsList->getNumberOfReferencePictureSets(); i++)
  {
    rps = rpsList->getReferencePictureSet(i);
    codeShortTermRefPicSet(pcSPS,rps,false, i);
  }
  WRITE_FLAG( pcSPS->getLongTermRefsPresent() ? 1 : 0,         "long_term_ref_pics_present_flag" );
  if (pcSPS->getLongTermRefsPresent()) 
  {
    WRITE_UVLC(pcSPS->getNumLongTermRefPicSPS(), "num_long_term_ref_pic_sps" );
    for (UInt k = 0; k < pcSPS->getNumLongTermRefPicSPS(); k++)
    {
      WRITE_CODE( pcSPS->getLtRefPicPocLsbSps(k), pcSPS->getBitsForPOC(), "lt_ref_pic_poc_lsb_sps");
      WRITE_FLAG( pcSPS->getUsedByCurrPicLtSPSFlag(k), "used_by_curr_pic_lt_sps_flag");
    }
  }
  WRITE_FLAG( pcSPS->getTMVPFlagsPresent()  ? 1 : 0,           "sps_temporal_mvp_enable_flag" );

  WRITE_FLAG( pcSPS->getUseStrongIntraSmoothing(),             "sps_strong_intra_smoothing_enable_flag" );

  WRITE_FLAG( pcSPS->getVuiParametersPresentFlag(),             "vui_parameters_present_flag" );
  if (pcSPS->getVuiParametersPresentFlag())
  {
      codeVUI(pcSPS->getVuiParameters(), pcSPS);
  }

#if !H_MV
  WRITE_FLAG( 0, "sps_extension_flag" );
#else
  WRITE_FLAG( pcSPS->getSpsExtensionPresentFlag(), "sps_extension_present_flag" );

  if ( pcSPS->getSpsExtensionPresentFlag() )
  {
    WRITE_FLAG( pcSPS->getSpsRangeExtensionsFlag( ) ? 1 : 0 , "sps_range_extensions_flag" );
    WRITE_FLAG( pcSPS->getSpsMultilayerExtensionFlag( ) ? 1 : 0 , "sps_multilayer_extension_flag" );
#if !H_3D
    WRITE_CODE( pcSPS->getSpsExtension6bits( ), 6, "sps_extension_6bits" );  
#else
    WRITE_FLAG( pcSPS->getSps3dExtensionFlag( ) ? 1 : 0 , "sps_3d_extension_flag" );
    WRITE_CODE( pcSPS->getSpsExtension5bits( ), 5, "sps_extension_5bits" );
#endif
  }

  if ( pcSPS->getSpsRangeExtensionsFlag() )
  {
    assert( 0 ); 
  }

  if ( pcSPS->getSpsMultilayerExtensionFlag() )
  {
    codeSPSExtension( pcSPS ); 
  }

#if H_3D
  if ( pcSPS->getSps3dExtensionFlag() )
  {
    codeSPSExtension2( pcSPS, viewIndex, depthFlag  ); 
  }

#endif
#endif

}

#if H_MV
Void TEncCavlc::codeSPSExtension( TComSPS* pcSPS )
{
  WRITE_FLAG( pcSPS->getInterViewMvVertConstraintFlag() ? 1 : 0, "inter_view_mv_vert_constraint_flag" );

  WRITE_UVLC( pcSPS->getNumScaledRefLayerOffsets( ), "num_scaled_ref_layer_offsets" );

  for( Int i = 0; i < pcSPS->getNumScaledRefLayerOffsets( ); i++)
  {    
    WRITE_CODE( pcSPS->getScaledRefLayerId( i ), 6, "scaled_ref_layer_id" );

    Int j = pcSPS->getScaledRefLayerId( i ); 
    
    WRITE_SVLC( pcSPS->getScaledRefLayerLeftOffset( j ), "scaled_ref_layer_left_offset" );
    WRITE_SVLC( pcSPS->getScaledRefLayerTopOffset( j ), "scaled_ref_layer_top_offset" );
    WRITE_SVLC( pcSPS->getScaledRefLayerRightOffset( j ), "scaled_ref_layer_right_offset" );
    WRITE_SVLC( pcSPS->getScaledRefLayerBottomOffset( j ), "scaled_ref_layer_bottom_offset" );
    WRITE_FLAG( 0, "sps_multilayer_ext_reserved_zero_flag[ j ]" );
  }
}
#endif

#if H_3D
Void TEncCavlc::codeSPSExtension2( TComSPS* pcSPS, Int viewIndex, Bool depthFlag )
{
#if H_3D_QTLPC
#if !MTK_I0099_VPS_EX2
//GT: This has to go to VPS
if( depthFlag )
{
  WRITE_FLAG( pcSPS->getUseQTL() ? 1 : 0, "use_qtl_flag");
  WRITE_FLAG( pcSPS->getUsePC()  ? 1 : 0, "use_pc_flag");
}
#endif
#endif
}
#endif

Void TEncCavlc::codeVPS( TComVPS* pcVPS )
{
  WRITE_CODE( pcVPS->getVPSId(),                    4,        "vps_video_parameter_set_id" );
#if H_MV
  WRITE_FLAG( pcVPS->getVpsBaseLayerInternalFlag( ) ? 1 : 0 , "vps_base_layer_internal_flag" );
  WRITE_FLAG( 1                                             , "vps_reserved_one_bit" );
#else
  WRITE_CODE( 3,                                    2,        "vps_reserved_three_2bits" );
#endif
#if H_MV
  WRITE_CODE( pcVPS->getMaxLayersMinus1(),       6,        "vps_max_layers_minus1" );
#else
  WRITE_CODE( 0,                                    6,        "vps_reserved_zero_6bits" );
#endif
  WRITE_CODE( pcVPS->getMaxTLayers() - 1,           3,        "vps_max_sub_layers_minus1" );
  WRITE_FLAG( pcVPS->getTemporalNestingFlag(),                "vps_temporal_id_nesting_flag" );
  assert (pcVPS->getMaxTLayers()>1||pcVPS->getTemporalNestingFlag());
  WRITE_CODE( 0xffff,                              16,        "vps_reserved_ffff_16bits" );
  codePTL( pcVPS->getPTL(), true, pcVPS->getMaxTLayers() - 1 );
  const Bool subLayerOrderingInfoPresentFlag = 1;
  WRITE_FLAG(subLayerOrderingInfoPresentFlag,              "vps_sub_layer_ordering_info_present_flag");
  for(UInt i=0; i <= pcVPS->getMaxTLayers()-1; i++)
  {
    WRITE_UVLC( pcVPS->getMaxDecPicBuffering(i) - 1,       "vps_max_dec_pic_buffering_minus1[i]" );
    WRITE_UVLC( pcVPS->getNumReorderPics(i),               "vps_num_reorder_pics[i]" );
    WRITE_UVLC( pcVPS->getMaxLatencyIncrease(i),           "vps_max_latency_increase_plus1[i]" );
    if (!subLayerOrderingInfoPresentFlag)
    {
      break;
    }
  }

  assert( pcVPS->getNumHrdParameters() <= MAX_VPS_NUM_HRD_PARAMETERS );
#if H_MV
  assert( pcVPS->getVpsMaxLayerId() < MAX_VPS_NUH_LAYER_ID_PLUS1 );
  WRITE_CODE( pcVPS->getVpsMaxLayerId(), 6,                 "vps_max_layer_id" );  
  
  WRITE_UVLC( pcVPS->getVpsNumLayerSetsMinus1(),  "vps_max_num_layer_sets_minus1" );
  for( UInt opsIdx = 1; opsIdx <= pcVPS->getVpsNumLayerSetsMinus1(); opsIdx ++ )
  {
    // Operation point set
    for( UInt i = 0; i <= pcVPS->getVpsMaxLayerId(); i ++ )
    {
#else
  assert( pcVPS->getMaxNuhReservedZeroLayerId() < MAX_VPS_NUH_RESERVED_ZERO_LAYER_ID_PLUS1 );
  WRITE_CODE( pcVPS->getMaxNuhReservedZeroLayerId(), 6,     "vps_max_nuh_reserved_zero_layer_id" );
  pcVPS->setMaxOpSets(1);
  WRITE_UVLC( pcVPS->getMaxOpSets() - 1,                    "vps_max_op_sets_minus1" );
  for( UInt opsIdx = 1; opsIdx <= ( pcVPS->getMaxOpSets() - 1 ); opsIdx ++ )
  {
    // Operation point set
    for( UInt i = 0; i <= pcVPS->getMaxNuhReservedZeroLayerId(); i ++ )
    {
      // Only applicable for version 1
      pcVPS->setLayerIdIncludedFlag( true, opsIdx, i );
#endif
      WRITE_FLAG( pcVPS->getLayerIdIncludedFlag( opsIdx, i ) ? 1 : 0, "layer_id_included_flag[opsIdx][i]" );
    }
  }
  TimingInfo *timingInfo = pcVPS->getTimingInfo();
  WRITE_FLAG(timingInfo->getTimingInfoPresentFlag(),          "vps_timing_info_present_flag");
  if(timingInfo->getTimingInfoPresentFlag())
  {
    WRITE_CODE(timingInfo->getNumUnitsInTick(), 32,           "vps_num_units_in_tick");
    WRITE_CODE(timingInfo->getTimeScale(),      32,           "vps_time_scale");
    WRITE_FLAG(timingInfo->getPocProportionalToTimingFlag(),  "vps_poc_proportional_to_timing_flag");
    if(timingInfo->getPocProportionalToTimingFlag())
    {
      WRITE_UVLC(timingInfo->getNumTicksPocDiffOneMinus1(),   "vps_num_ticks_poc_diff_one_minus1");
    }
    pcVPS->setNumHrdParameters( 0 );
    WRITE_UVLC( pcVPS->getNumHrdParameters(),                 "vps_num_hrd_parameters" );

    if( pcVPS->getNumHrdParameters() > 0 )
    {
      pcVPS->createHrdParamBuffer();
    }
    for( UInt i = 0; i < pcVPS->getNumHrdParameters(); i ++ )
    {
      // Only applicable for version 1
      pcVPS->setHrdOpSetIdx( 0, i );
      WRITE_UVLC( pcVPS->getHrdOpSetIdx( i ),                "hrd_op_set_idx" );
      if( i > 0 )
      {
        WRITE_FLAG( pcVPS->getCprmsPresentFlag( i ) ? 1 : 0, "cprms_present_flag[i]" );
      }
      codeHrdParameters(pcVPS->getHrdParameters(i), pcVPS->getCprmsPresentFlag( i ), pcVPS->getMaxTLayers() - 1);
    }
  }
#if H_MV
  WRITE_FLAG( pcVPS->getVpsExtensionFlag(),                     "vps_extension_flag" );
  m_pcBitIf->writeAlignOne();
  codeVPSExtension( pcVPS );                           
#if H_3D
  WRITE_FLAG( 1,                     "vps_extension2_flag" );
  m_pcBitIf->writeAlignOne();      
  codeVPSExtension2( pcVPS ); 
  WRITE_FLAG( 0,                     "vps_extension3_flag" );
#else
  WRITE_FLAG( 0,                     "vps_extension2_flag" );
#endif
#else
  WRITE_FLAG( 0,                     "vps_extension_flag" );
#endif
  //future extensions here..

  return;
}



#if H_MV
Void TEncCavlc::codeVPSExtension( TComVPS *pcVPS ) 
{  
  WRITE_FLAG( pcVPS->getSplittingFlag() ? 1 : 0,             "splitting_flag" );

  for( Int type = 0; type < MAX_NUM_SCALABILITY_TYPES; type++ )
  {
    WRITE_FLAG( pcVPS->getScalabilityMaskFlag( type ) ? 1 : 0,   "scalability_mask_flag[i]" );
  }

  for( Int sIdx = 0; sIdx < pcVPS->getNumScalabilityTypes( ) - ( pcVPS->getSplittingFlag() ? 1 : 0 ); sIdx++ )
  {
    WRITE_CODE( pcVPS->getDimensionIdLen( sIdx ) - 1 , 3,    "dimension_id_len_minus1[j]");    
  }

  if ( pcVPS->getSplittingFlag() )
  {
    pcVPS->setDimensionIdLen( pcVPS->getNumScalabilityTypes( ) - 1 ,pcVPS->inferLastDimsionIdLenMinus1() + 1 );       
  }    

  WRITE_FLAG( pcVPS->getVpsNuhLayerIdPresentFlag() ? 1 : 0,  "vps_nuh_layer_id_present_flag");

  for( Int i = 1; i <= pcVPS->getMaxLayersMinus1(); i++ )
  {
    if ( pcVPS->getVpsNuhLayerIdPresentFlag() )
    {      
      WRITE_CODE( pcVPS->getLayerIdInNuh( i ), 6,          "layer_id_in_nuh[i]");
    }
    else
    {
      assert( pcVPS->getLayerIdInNuh( i ) == i ); 
    }

    assert(  pcVPS->getLayerIdInVps( pcVPS->getLayerIdInNuh( i ) ) == i ); 

    for( Int j = 0; j < pcVPS->getNumScalabilityTypes() ; j++ )
    {
      if ( !pcVPS->getSplittingFlag() )
      {
        WRITE_CODE( pcVPS->getDimensionId( i, j ), pcVPS->getDimensionIdLen( j ), "dimension_id[i][j]");      
      }
      else
      {
        assert( pcVPS->getDimensionId( i, j ) ==  pcVPS->inferDimensionId( i, j )  );
      }
    }
  }

  WRITE_CODE( pcVPS->getViewIdLen( ), 4, "view_id_len" );

  if ( pcVPS->getViewIdLen( ) > 0 )
  {    
    for( Int i = 0; i < pcVPS->getNumViews(); i++ )
    {
      WRITE_CODE( pcVPS->getViewIdVal( i ), pcVPS->getViewIdLen( ), "view_id_val[i]" );
    }
  }
  else
  {
    for( Int i = 0; i < pcVPS->getNumViews(); i++ )
    {
      assert( pcVPS->getViewIdVal( i ) == 0 ); 
    }
  }


  for( Int i = 1; i <= pcVPS->getMaxLayersMinus1(); i++ )
  {
    for( Int j = 0; j < i; j++ )
    {
      WRITE_FLAG( pcVPS->getDirectDependencyFlag( i, j ),    "direct_dependency_flag[i][j]" );
    }
  }

  WRITE_FLAG( pcVPS->getVpsSubLayersMaxMinus1PresentFlag( ) ? 1 : 0 , "vps_sub_layers_max_minus1_present_flag" );
  if ( pcVPS->getVpsSubLayersMaxMinus1PresentFlag() )
  {
    for (Int i = 0; i <= pcVPS->getMaxLayersMinus1(); i++ )
    {
      WRITE_CODE( pcVPS->getSubLayersVpsMaxMinus1( i ), 3, "sub_layers_vps_max_minus1" );
      pcVPS->checkSubLayersVpsMaxMinus1( i ); 
    }
  }  
  else
  {
    for (Int i = 0; i <= pcVPS->getMaxLayersMinus1(); i++ )
    {
      assert( pcVPS->getSubLayersVpsMaxMinus1( i ) + 1 == pcVPS->getMaxTLayers( ) );    
    }
  }
  WRITE_FLAG( pcVPS->getMaxTidRefPresentFlag( ) ? 1 : 0 , "max_tid_ref_present_flag" );

  if ( pcVPS->getMaxTidRefPresentFlag() )
  {    
    for( Int i = 0; i < pcVPS->getMaxLayersMinus1(); i++ )
    {
      for( Int j = i + 1; j <= pcVPS->getMaxLayersMinus1(); j++ )
      {
        if ( pcVPS->getDirectDependencyFlag(j,i) )
        {
          WRITE_CODE( pcVPS->getMaxTidIlRefPicsPlus1( i, j ), 3, "max_tid_il_ref_pics_plus1" );
        }
      }
    }
  }

  WRITE_FLAG( pcVPS->getAllRefLayersActiveFlag( ) ? 1 : 0 , "all_ref_layers_active_flag" );
  WRITE_UVLC( pcVPS->getVpsNumProfileTierLevelMinus1( ), "vps_num_profile_tier_level_minus1" );

  for( Int i = 1; i <= pcVPS->getVpsNumProfileTierLevelMinus1(); i++ )
  {
    WRITE_FLAG( pcVPS->getVpsProfilePresentFlag( i ) ? 1 : 0, "vps_profile_present_flag[i]" );
    codePTL( pcVPS->getPTL( i ), pcVPS->getVpsProfilePresentFlag( i ), pcVPS->getMaxTLayers() - 1 );
  }

  if ( pcVPS->getNumIndependentLayers() > 1 ) 
  {
   WRITE_UVLC( pcVPS->getNumAddLayerSets( ), "num_add_layer_sets" );
  }
  for (Int i = 0; i < pcVPS->getNumAddLayerSets(); i++)
  {
    for (Int j = 0; j < pcVPS->getNumIndependentLayers(); j++)
    {
      WRITE_CODE( pcVPS->getHighestLayerIdxPlus1( i, j ), pcVPS->getHighestLayerIdxPlus1Len( j )  , "highest_layer_idx_plus1" );
    }
  }
  if (pcVPS->getNumLayerSets() > 1)
  {
    WRITE_UVLC( pcVPS->getNumAddOlss( ), "num_add_olss" );
    WRITE_CODE( pcVPS->getDefaultOutputLayerIdc( ), 2, "default_output_layer_idc" );
  }

  assert( pcVPS->getOutputLayerFlag(0, 0) == pcVPS->inferOutputLayerFlag( 0, 0 )); 
  assert( pcVPS->getLayerSetIdxForOlsMinus1( 0 ) == -1 ); 




  for( Int i = 1; i < pcVPS->getNumOutputLayerSets( ); i++ )
  {
    if( i >= pcVPS->getNumLayerSets( ) )    
    {      
      WRITE_UVLC( pcVPS->getLayerSetIdxForOlsMinus1( i ),      "layer_set_idx_for_ols_minus1[i]" );
    }

    if ( i > pcVPS->getVpsNumLayerSetsMinus1() || pcVPS->getDefaultOutputLayerIdc() == 2 )
    {       
      for( Int j = 0; j < pcVPS->getNumLayersInIdList( pcVPS->olsIdxToLsIdx( i ) ); j++ )
      {
        WRITE_FLAG( pcVPS->getOutputLayerFlag( i, j) ? 1 : 0, "output_layer_flag" );
      }              
    }
    else
    {
      for( Int j = 0; j < pcVPS->getNumLayersInIdList( pcVPS->olsIdxToLsIdx( i ) ) - 1; j++ )
      {              
        assert( pcVPS->getOutputLayerFlag( i , j ) == pcVPS->inferOutputLayerFlag( i, j )); 
      }
    }        


    if ( pcVPS->getProfileLevelTierIdxLen()  > 0 )
    {      
      WRITE_CODE( pcVPS->getProfileLevelTierIdx( i ), pcVPS->getProfileLevelTierIdxLen() ,"profile_level_tier_idx[ i ]" );   
    }
    if( pcVPS->getNumOutputLayersInOutputLayerSet( i ) == 1 && pcVPS->getNumDirectRefLayers( pcVPS->getOlsHighestOutputLayerId( i ) ) > 0 )
    {
      WRITE_FLAG( pcVPS->getAltOutputLayerFlag( i ) ? 1 : 0 , "alt_output_layer_flag[ i ]" );
    }
  }

  WRITE_UVLC( pcVPS->getVpsNumRepFormatsMinus1( ), "vps_num_rep_formats_minus1" );

  for (Int i = 0; i <= pcVPS->getVpsNumRepFormatsMinus1(); i++ )
  {    

    TComRepFormat* curRepFormat = pcVPS->getRepFormat(i);     
    TComRepFormat* prevRepFormat = i > 0 ? pcVPS->getRepFormat( i - 1) : NULL; 
    codeRepFormat( i, curRepFormat ,  prevRepFormat); 

  }

  if ( pcVPS->getVpsNumRepFormatsMinus1() > 0 )
  {
    WRITE_FLAG( pcVPS->getRepFormatIdxPresentFlag( ) ? 1 : 0 , "rep_format_idx_present_flag" );
  }

  if( pcVPS->getRepFormatIdxPresentFlag() ) 
  {
    for( Int i = pcVPS->getVpsBaseLayerInternalFlag() ? 1 : 0; i <=  pcVPS->getMaxLayersMinus1(); i++ )
    {
        WRITE_CODE( pcVPS->getVpsRepFormatIdx(i), pcVPS->getVpsRepFormatIdxLen(), "vps_rep_format_idx[i]" );
    }
  }
  else
  {
    for( Int i = pcVPS->getVpsBaseLayerInternalFlag() ? 1 : 0; i <=  pcVPS->getMaxLayersMinus1(); i++ )
    {
      assert( pcVPS->getVpsRepFormatIdx( i ) ==  pcVPS->inferVpsRepFormatIdx( i ) );
    }
  }

  WRITE_FLAG( pcVPS->getMaxOneActiveRefLayerFlag( ) ? 1 : 0, "max_one_active_ref_layer_flag" );
#if H_MV_HLS7_GEN
  WRITE_FLAG( pcVPS->getVpsPocLsbAlignedFlag( ) ? 1 : 0 , "vps_poc_lsb_aligned_flag" );
#endif
  for( Int i = 1; i  <=  pcVPS->getMaxLayersMinus1(); i++ )
  {
    if( pcVPS->getNumDirectRefLayers( pcVPS->getLayerIdInNuh( i ) )  ==  0 )
    {      
      WRITE_FLAG( pcVPS->getPocLsbNotPresentFlag( i ) ? 1 : 0 , "poc_lsb_not_present_flag" );
    }
  }

  WRITE_FLAG( 0, "vps_reserved_zero_flag" );
  codeDpbSize( pcVPS ); 

  WRITE_UVLC( pcVPS->getDirectDepTypeLenMinus2 ( ),         "direct_dep_type_len_minus2"); 

  WRITE_FLAG( pcVPS->getDefaultDirectDependencyFlag( ) ? 1 : 0 , "default_direct_dependency_flag" );
  if ( pcVPS->getDefaultDirectDependencyFlag( ) )
  {  
    WRITE_CODE( pcVPS->getDefaultDirectDependencyType( ), pcVPS->getDirectDepTypeLenMinus2( ) + 2 , "default_direct_dependency_type" );    
  }

  for( Int i = pcVPS->getVpsBaseLayerInternalFlag() ?  1 : 2; i <= pcVPS->getMaxLayersMinus1(); i++ )
  {
    for( Int j = pcVPS->getVpsBaseLayerInternalFlag() ?  0 : 1; j < i; j++ )
    {
      if (pcVPS->getDirectDependencyFlag( i, j) )
      {        
        if ( pcVPS->getDefaultDirectDependencyFlag( ) )
        {  
          assert( pcVPS->getDirectDependencyType( i, j ) == pcVPS->getDefaultDirectDependencyType( ) );
        }
        else
        {
          assert ( pcVPS->getDirectDependencyType( i, j ) != -1 ); 
          WRITE_CODE( pcVPS->getDirectDependencyType( i, j ),pcVPS->getDirectDepTypeLenMinus2( ) + 2,  "direct_dependency_type[i][j]" );
        }
      }
    }
  }  
  WRITE_UVLC( 0, "vps_non_vui_extension_length" );
  



  WRITE_FLAG( pcVPS->getVpsVuiPresentFlag() ? 1 : 0 , "vps_vui_present_flag" );
  if( pcVPS->getVpsVuiPresentFlag() )
  {
    m_pcBitIf->writeAlignOne();  // vps_vui_alignment_bit_equal_to_one
    codeVPSVUI( pcVPS ); 
  }     
  {
    TComVPSVUI* pcVPSVUI = pcVPS->getVPSVUI( ); 
    assert( pcVPSVUI ); 
    pcVPSVUI->inferVpsVui( true ); 
  }

}

Void TEncCavlc::codeVideoSignalInfo( TComVideoSignalInfo* pcVideoSignalInfo )
{
  assert( pcVideoSignalInfo ); 
  WRITE_CODE( pcVideoSignalInfo->getVideoVpsFormat( ), 3, "video_vps_format" );
  WRITE_FLAG( pcVideoSignalInfo->getVideoFullRangeVpsFlag( ) ? 1 : 0 , "video_full_range_vps_flag" );
  WRITE_CODE( pcVideoSignalInfo->getColourPrimariesVps( ), 8, "colour_primaries_vps" );
  WRITE_CODE( pcVideoSignalInfo->getTransferCharacteristicsVps( ), 8, "transfer_characteristics_vps" );
  WRITE_CODE( pcVideoSignalInfo->getMatrixCoeffsVps( ), 8, "matrix_coeffs_vps" );
}

Void TEncCavlc::codeDpbSize( TComVPS* vps )
{ 
  TComDpbSize* dpbSize = vps->getDpbSize(); 
  assert ( dpbSize != 0 ); 

  for( Int i = 1; i < vps->getNumOutputLayerSets(); i++ )
  {  
    Int currLsIdx = vps->olsIdxToLsIdx( i ); 
    WRITE_FLAG( dpbSize->getSubLayerFlagInfoPresentFlag( i ) ? 1 : 0 , "sub_layer_flag_info_present_flag" );
    for( Int j = 0; j  <=  vps->getMaxSubLayersInLayerSetMinus1( currLsIdx ); j++ )
    {  
      if( j > 0  &&  dpbSize->getSubLayerDpbInfoPresentFlag( i, j )  )  
      {
        WRITE_FLAG( dpbSize->getSubLayerDpbInfoPresentFlag( i, j ) ? 1 : 0 , "sub_layer_dpb_info_present_flag" );
      }
      if( dpbSize->getSubLayerDpbInfoPresentFlag( i, j ) )
      {  
        for( Int k = 0; k < vps->getNumLayersInIdList( currLsIdx ); k++ )   
        {
          WRITE_UVLC( dpbSize->getMaxVpsDecPicBufferingMinus1( i, k, j ), "max_vps_dec_pic_buffering_minus1" );
        }
        WRITE_UVLC( dpbSize->getMaxVpsNumReorderPics( i, j ), "max_vps_num_reorder_pics" );
        WRITE_UVLC( dpbSize->getMaxVpsLatencyIncreasePlus1( i, j ), "max_vps_latency_increase_plus1" );
      }
      else
      {
        if ( j > 0 )
        {
          for( Int k = 0; k < vps->getNumLayersInIdList( vps->olsIdxToLsIdx( i ) ); k++ )   
          {
            assert( dpbSize->getMaxVpsDecPicBufferingMinus1( i, k, j ) == dpbSize->getMaxVpsDecPicBufferingMinus1( i,k, j - 1 ) );
          }
          assert( dpbSize->getMaxVpsNumReorderPics      ( i, j ) ==  dpbSize->getMaxVpsNumReorderPics      ( i, j - 1 ) );
          assert( dpbSize->getMaxVpsLatencyIncreasePlus1( i, j ) ==  dpbSize->getMaxVpsLatencyIncreasePlus1( i, j - 1 ) );
        }
      }
    }
  }
}

Void TEncCavlc::codeRepFormat( Int i, TComRepFormat* pcRepFormat, TComRepFormat* pcPrevRepFormat )
{
  assert( pcRepFormat ); 

  WRITE_CODE( pcRepFormat->getPicWidthVpsInLumaSamples( ),  16, "pic_width_vps_in_luma_samples" );
  WRITE_CODE( pcRepFormat->getPicHeightVpsInLumaSamples( ), 16, "pic_height_vps_in_luma_samples" );
  WRITE_FLAG( pcRepFormat->getChromaAndBitDepthVpsPresentFlag( ) ? 1 : 0 , "chroma_and_bit_depth_vps_present_flag" );
  
  pcRepFormat->checkChromaAndBitDepthVpsPresentFlag( i ); 

  if ( pcRepFormat->getChromaAndBitDepthVpsPresentFlag() )
  {  
  WRITE_CODE( pcRepFormat->getChromaFormatVpsIdc( ), 2, "chroma_format_vps_idc" );

  if ( pcRepFormat->getChromaFormatVpsIdc() == 3 )
  {
    WRITE_FLAG( pcRepFormat->getSeparateColourPlaneVpsFlag( ) ? 1 : 0 , "separate_colour_plane_vps_flag" );
  }
  WRITE_CODE( pcRepFormat->getBitDepthVpsLumaMinus8( ),      4, "bit_depth_vps_luma_minus8" );
  WRITE_CODE( pcRepFormat->getBitDepthVpsChromaMinus8( ),    4, "bit_depth_vps_chroma_minus8" );
  }
  else
  {
    pcRepFormat->inferChromaAndBitDepth(pcPrevRepFormat, true ); 
  }
}

Void TEncCavlc::codeVPSVUI( TComVPS* pcVPS )
{
  assert( pcVPS ); 

  TComVPSVUI* pcVPSVUI = pcVPS->getVPSVUI( ); 

  assert( pcVPSVUI ); 


  WRITE_FLAG( pcVPSVUI->getCrossLayerPicTypeAlignedFlag( ) ? 1 : 0 , "cross_layer_pic_type_aligned_flag" );
  if ( !pcVPSVUI->getCrossLayerPicTypeAlignedFlag() )
  {  
    WRITE_FLAG( pcVPSVUI->getCrossLayerIrapAlignedFlag( ) ? 1 : 0 , "cross_layer_irap_aligned_flag" );
  }
  if( pcVPSVUI->getCrossLayerIrapAlignedFlag( ) )
  {
    WRITE_FLAG( pcVPSVUI->getAllLayersIdrAlignedFlag( ) ? 1 : 0 , "all_layers_idr_aligned_flag" );
  }
  WRITE_FLAG( pcVPSVUI->getBitRatePresentVpsFlag( ) ? 1 : 0 , "bit_rate_present_vps_flag" );
  WRITE_FLAG( pcVPSVUI->getPicRatePresentVpsFlag( ) ? 1 : 0 , "pic_rate_present_vps_flag" );
  if( pcVPSVUI->getBitRatePresentVpsFlag( )  ||  pcVPSVUI->getPicRatePresentVpsFlag( ) )
  {
    for( Int i = pcVPS->getVpsBaseLayerInternalFlag() ? 0 : 1; i  <=  pcVPS->getVpsNumLayerSetsMinus1(); i++ )
    {
      for( Int j = 0; j  <=  pcVPS->getMaxSubLayersInLayerSetMinus1( i ); j++ ) 
      {
        if( pcVPSVUI->getBitRatePresentVpsFlag( ) )
        {
          WRITE_FLAG( pcVPSVUI->getBitRatePresentFlag( i, j ) ? 1 : 0 , "bit_rate_present_flag" );
        }
        if( pcVPSVUI->getBitRatePresentVpsFlag( )  )
        {
          WRITE_FLAG( pcVPSVUI->getPicRatePresentFlag( i, j ) ? 1 : 0 , "pic_rate_present_flag" );
        }
        if( pcVPSVUI->getBitRatePresentFlag( i, j ) )
        {
          WRITE_CODE( pcVPSVUI->getAvgBitRate( i, j ), 16, "avg_bit_rate" );
          WRITE_CODE( pcVPSVUI->getMaxBitRate( i, j ), 16, "max_bit_rate" );
        }
        if( pcVPSVUI->getPicRatePresentFlag( i, j ) )
        {
          WRITE_CODE( pcVPSVUI->getConstantPicRateIdc( i, j ), 2, "constant_pic_rate_idc" );
          WRITE_CODE( pcVPSVUI->getAvgPicRate( i, j ), 16, "avg_pic_rate" );
        }
      }
    }
  }

  WRITE_FLAG( pcVPSVUI->getVideoSignalInfoIdxPresentFlag( ) ? 1 : 0 , "video_signal_info_idx_present_flag" );
  if( pcVPSVUI->getVideoSignalInfoIdxPresentFlag() )
  {
    WRITE_CODE( pcVPSVUI->getVpsNumVideoSignalInfoMinus1( ), 4, "vps_num_video_signal_info_minus1" );
  }
  else
  {
    pcVPSVUI->setVpsNumVideoSignalInfoMinus1( pcVPS->getMaxLayersMinus1() ); 
  }

  for( Int i = 0; i <= pcVPSVUI->getVpsNumVideoSignalInfoMinus1(); i++ )
  {
    assert( pcVPSVUI->getVideoSignalInfo( i ) != NULL ); 
    TComVideoSignalInfo* curVideoSignalInfo = pcVPSVUI->getVideoSignalInfo( i ); 
    codeVideoSignalInfo( curVideoSignalInfo );     

  }

  if( pcVPSVUI->getVideoSignalInfoIdxPresentFlag() && pcVPSVUI->getVpsNumVideoSignalInfoMinus1() > 0 )
  {
    for( Int i = 1; i <=  pcVPS->getMaxLayersMinus1(); i++ )
    {
      WRITE_CODE( pcVPSVUI->getVpsVideoSignalInfoIdx( i ), 4, "vps_video_signal_info_idx" );
      assert( pcVPSVUI->getVpsVideoSignalInfoIdx( i ) >= 0 && pcVPSVUI->getVpsVideoSignalInfoIdx( i ) <= pcVPSVUI->getVpsNumVideoSignalInfoMinus1() );
    }
  }
  else
  {
    for( Int i = 1; i <=  pcVPS->getMaxLayersMinus1(); i++ )
    {
      assert( pcVPSVUI->getVpsVideoSignalInfoIdx( i  ) == ( pcVPSVUI->getVideoSignalInfoIdxPresentFlag() ? 0 : i ) ); 
    }
  }
  WRITE_FLAG( pcVPSVUI->getTilesNotInUseFlag( ) ? 1 : 0 , "tiles_not_in_use_flag" );
  if( !pcVPSVUI->getTilesNotInUseFlag() ) 
  {      
    for( Int i = pcVPS->getVpsBaseLayerInternalFlag() ? 0 : 1; i  <=  pcVPS->getMaxLayersMinus1(); i++ )
    {
      WRITE_FLAG( pcVPSVUI->getTilesInUseFlag( i ) ? 1 : 0 , "tiles_in_use_flag[i]" );
      if( pcVPSVUI->getTilesInUseFlag( i ) )  
      {
        WRITE_FLAG( pcVPSVUI->getLoopFilterNotAcrossTilesFlag( i ) ? 1 : 0, "loop_filter_not_across_tiles_flag[i]" );
      }
    }  
    for( Int i = pcVPS->getVpsBaseLayerInternalFlag() ? 1 : 2; i  <=  pcVPS->getMaxLayersMinus1(); i++ )  
    {
      for( Int j = 0; j < pcVPS->getNumDirectRefLayers( pcVPS->getLayerIdInNuh( i ) ) ; j++ )
      {  
        Int layerIdx = pcVPS->getLayerIdInVps(pcVPS->getRefLayerId(pcVPS->getLayerIdInNuh( i ) , j  ));  
        if( pcVPSVUI->getTilesInUseFlag( i )  &&  pcVPSVUI->getTilesInUseFlag( layerIdx ) )  
        {
          WRITE_FLAG( pcVPSVUI->getTileBoundariesAlignedFlag( i, j ) ? 1 : 0 , "tile_boundaries_aligned_flag[i][j]" );
        }
      }  
    }
  }  

  WRITE_FLAG( pcVPSVUI->getWppNotInUseFlag( ) ? 1 : 0 , "wpp_not_in_use_flag" );

  if( !pcVPSVUI->getWppNotInUseFlag( ) )
  {
    for( Int i = 0; i  <=  pcVPS->getMaxLayersMinus1(); i++ )  
    {
      WRITE_FLAG( pcVPSVUI->getWppInUseFlag( i ) ? 1 : 0 , "wpp_in_use_flag[i]" );
    }
  }
  WRITE_CODE( 0, 3, "vps_vui_reserved_zero_3bits" );
  WRITE_FLAG( pcVPSVUI->getIlpRestrictedRefLayersFlag( ) ? 1 : 0 , "ilp_restricted_ref_layers_flag" );

  if( pcVPSVUI->getIlpRestrictedRefLayersFlag( ) )
  {
    for( Int i = 1; i  <=  pcVPS->getMaxLayersMinus1(); i++ )
    {
      for( Int j = 0; j < pcVPS->getNumDirectRefLayers( pcVPS->getLayerIdInNuh( i ) ); j++ )
      {
        if( pcVPS->getVpsBaseLayerInternalFlag() || pcVPS->getRefLayerId( pcVPS->getLayerIdInNuh( i ), j ) > 0 )
        {        
          WRITE_UVLC( pcVPSVUI->getMinSpatialSegmentOffsetPlus1( i, j ), "min_spatial_segment_offset_plus1" );
          if( pcVPSVUI->getMinSpatialSegmentOffsetPlus1( i, j ) > 0 )
          {
            WRITE_FLAG( pcVPSVUI->getCtuBasedOffsetEnabledFlag( i, j ) ? 1 : 0 , "ctu_based_offset_enabled_flag" );
            if( pcVPSVUI->getCtuBasedOffsetEnabledFlag( i, j ) )
            {
              WRITE_UVLC( pcVPSVUI->getMinHorizontalCtuOffsetPlus1( i, j ), "min_horizontal_ctu_offset_plus1" );
            }
          }
        }
      }
    }
  }


  WRITE_FLAG( pcVPSVUI->getVpsVuiBspHrdPresentFlag( ) ? 1 : 0 , "vps_vui_bsp_hrd_present_flag" );
  if ( pcVPSVUI->getVpsVuiBspHrdPresentFlag( ) )
  {
    codeVpsVuiBspHrdParameters( pcVPS ); 
  }
  for( Int i = 1; i  <=  pcVPS->getMaxLayersMinus1(); i++ )
  {
    if( pcVPS->getNumDirectRefLayers( pcVPS->getLayerIdInNuh( i )) == 0 ) 
    {
      WRITE_FLAG( pcVPSVUI->getBaseLayerParameterSetCompatibilityFlag( i ) ? 1 : 0 , "base_layer_parameter_set_compatibility_flag" );
    }
  }
}

Void TEncCavlc::codeVpsVuiBspHrdParameters( TComVPS* pcVPS )
{
  assert( pcVPS ); 

  TComVPSVUI* pcVPSVUI = pcVPS->getVPSVUI( ); 

  assert( pcVPSVUI ); 

  TComVpsVuiBspHrdParameters*  vpsVuiBspHrdP = pcVPSVUI->getVpsVuiBspHrdParameters(); 

  assert ( vpsVuiBspHrdP );

  
  WRITE_UVLC( vpsVuiBspHrdP->getVpsNumBspHrdParametersMinus1( ), "vps_num_bsp_hrd_parameters_minus1" );
  for( Int i = 0; i <= vpsVuiBspHrdP->getVpsNumBspHrdParametersMinus1( ); i++ )
  {  
    if( i > 0 )
    {
      WRITE_FLAG( vpsVuiBspHrdP->getBspCprmsPresentFlag( i ) ? 1 : 0 , "bsp_cprms_present_flag" );
    }
    TComHRD* hrdParameters = vpsVuiBspHrdP->getHrdParametermeters( i ); 
    codeHrdParameters( hrdParameters, vpsVuiBspHrdP->getBspCprmsPresentFlag( i ), pcVPS->getMaxSubLayersMinus1() );     
  }  
  for( Int h = 1; h <= pcVPS->getVpsNumLayerSetsMinus1(); h++ )
  {  
    WRITE_UVLC( vpsVuiBspHrdP->getNumBitstreamPartitions( h ), "num_bitstream_partitions" );
    for( Int i = 0; i < vpsVuiBspHrdP->getNumBitstreamPartitions( h ); i++ )  
    {
      for( Int j = 0; j < pcVPS->getNumLayersInIdList( h ); j++ )  
      {
        WRITE_FLAG( vpsVuiBspHrdP->getLayerInBspFlag( h, i, j ) ? 1 : 0 , "layer_in_bsp_flag" );
      }
    }
    vpsVuiBspHrdP->checkLayerInBspFlag( pcVPS, h ); 

    if( vpsVuiBspHrdP->getNumBitstreamPartitions( h ) )
    { 
      WRITE_UVLC( vpsVuiBspHrdP->getNumBspSchedCombinationsMinus1( h ), "num_bsp_sched_combinations_minus1" );
      for( Int i = 0; i <= vpsVuiBspHrdP->getNumBspSchedCombinationsMinus1( h ); i++ )
      {
        for( Int j = 0; j < vpsVuiBspHrdP->getNumBitstreamPartitions( h ); j++ )
        {  
          WRITE_CODE( vpsVuiBspHrdP->getBspCombHrdIdx( h, i, j ),  vpsVuiBspHrdP->getBspCombHrdIdxLen(), "bsp_comb_hrd_idx" ); 
          WRITE_UVLC( vpsVuiBspHrdP->getBspCombSchedIdx( h, i, j ), "bsp_comb_sched_idx" );

          vpsVuiBspHrdP->checkBspCombHrdAndShedIdx( pcVPS, h, i, j ); 
        }  
      }
    }  
  }  
}  

#endif

#if H_3D
Void TEncCavlc::codeVPSExtension2( TComVPS* pcVPS )
{ 
#if SEC_VPS_CLEANUP_I0090
  for( Int i = 1; i <= pcVPS->getMaxLayersMinus1(); i++ )
#else
  for( Int i = 0; i <= pcVPS->getMaxLayersMinus1(); i++ )
#endif
  {
#if !SEC_VPS_CLEANUP_I0090
    if (i!= 0)
#endif
    {
#if MTK_I0099_VPS_EX2
      WRITE_FLAG( pcVPS->getIvMvPredFlag         ( i ) ? 1 : 0 , "iv_mv_pred_flag[i]");
#endif
      if ( !( pcVPS->getDepthId( i ) == 1 ) )
      {
#if H_3D_IV_MERGE
#if !MTK_I0099_VPS_EX2
        WRITE_FLAG( pcVPS->getIvMvPredFlag         ( i ) ? 1 : 0 , "iv_mv_pred_flag[i]");
#endif
#if H_3D_SPIVMP
        WRITE_UVLC( pcVPS->getSubPULog2Size(i)-3, "log2_sub_PU_size_minus3[i]");
#endif
#endif
#if H_3D_ARP
        WRITE_FLAG( pcVPS->getUseAdvRP             ( i ) ? 1 : 0,  "iv_res_pred_flag[i]"  );
#endif
#if H_3D_NBDV_REF
        WRITE_FLAG( pcVPS->getDepthRefinementFlag  ( i ) ? 1 : 0 , "depth_refinement_flag[i]");
#endif
#if H_3D_VSP
        WRITE_FLAG( pcVPS->getViewSynthesisPredFlag( i ) ? 1 : 0 , "view_synthesis_pred_flag[i]");
#endif
#if H_3D_DBBP
        WRITE_FLAG( pcVPS->getUseDBBP( i ) ? 1 : 0, "use_dbbp_flag[i]" );
#endif
      }          
      else
      {
#if !MTK_I0099_VPS_EX2
        if(i!=1)
        {
          WRITE_FLAG( pcVPS->getIvMvPredFlag         ( i ) ? 1 : 0 , "iv_mv_pred_flag[i]");
        }
#if H_3D_SPIVMP
        if (i!=1)
        {
          WRITE_UVLC( pcVPS->getSubPULog2Size(i)-3, "log2_sub_PU_size_minus3[i]");
        }
#endif
#endif
#if H_3D_IV_MERGE
        WRITE_FLAG( pcVPS->getMPIFlag( i ) ? 1 : 0 ,          "mpi_flag[i]" );
#endif
#if MTK_I0099_VPS_EX2
        WRITE_UVLC( pcVPS->getSubPUMPILog2Size(i)-3, "log2_mpi_sub_PU_size_minus3[i]");
#endif
        WRITE_FLAG( pcVPS->getVpsDepthModesFlag( i ) ? 1 : 0 ,          "vps_depth_modes_flag[i]" );
#if MTK_I0099_VPS_EX2
        WRITE_FLAG( pcVPS->getLimQtPredFlag    ( i ) ? 1 : 0 ,          "lim_qt_pred_flag[i]"     ); 
#endif
#if H_3D_INTER_SDC
        WRITE_FLAG( pcVPS->getInterSDCFlag( i ) ? 1 : 0, "depth_inter_SDC_flag" );
#endif
      }
    }  
  }
  WRITE_UVLC( pcVPS->getCamParPrecision(), "cp_precision" );
#if SEC_VPS_CLEANUP_I0090
  for (UInt viewIndex=1; viewIndex<pcVPS->getNumViews(); viewIndex++)
#else
  for (UInt viewIndex=0; viewIndex<pcVPS->getNumViews(); viewIndex++)
#endif
  {
    WRITE_FLAG( pcVPS->getCamParPresent(viewIndex) ? 1 : 0, "cp_present_flag[i]" );
    if ( pcVPS->getCamParPresent(viewIndex) )
    {
      WRITE_FLAG( pcVPS->hasCamParInSliceHeader(viewIndex) ? 1 : 0, "cp_in_slice_segment_header_flag[i]" );
      if ( !pcVPS->hasCamParInSliceHeader(viewIndex) )
      {
        for( UInt uiIndex = 0; uiIndex < viewIndex; uiIndex++ )
        {
          WRITE_SVLC( pcVPS->getCodedScale    (viewIndex)[ uiIndex ],                                               "vps_cp_scale" );
          WRITE_SVLC( pcVPS->getCodedOffset   (viewIndex)[ uiIndex ],                                               "vps_cp_off" );
          WRITE_SVLC( pcVPS->getInvCodedScale (viewIndex)[ uiIndex ] + pcVPS->getCodedScale (viewIndex)[ uiIndex ], "vps_cp_inv_scale_plus_scale" );
          WRITE_SVLC( pcVPS->getInvCodedOffset(viewIndex)[ uiIndex ] + pcVPS->getCodedOffset(viewIndex)[ uiIndex ], "vps_cp_inv_off_plus_off" );
        }
      }
    }
  }
#if !MTK_I0099_VPS_EX2
  WRITE_UVLC( pcVPS->getSubPUMPILog2Size( ) - 3, "log2_sub_PU_MPI_size_minus3");
#endif
#if H_3D_TMVP
  WRITE_FLAG( pcVPS->getIvMvScalingFlag( ) ? 1 : 0 ,          "iv_mv_scaling_flag" );
#endif
}
#endif

Void TEncCavlc::codeSliceHeader         ( TComSlice* pcSlice )
{
#if H_MV
  TComVPS* vps = pcSlice->getVPS(); 
#endif
#if ENC_DEC_TRACE  
  xTraceSliceHeader (pcSlice);
#endif

  //calculate number of bits required for slice address
  Int maxSliceSegmentAddress = pcSlice->getPic()->getNumCUsInFrame();
  Int bitsSliceSegmentAddress = 0;
  while(maxSliceSegmentAddress>(1<<bitsSliceSegmentAddress)) 
  {
    bitsSliceSegmentAddress++;
  }
  Int ctuAddress;
  if (pcSlice->isNextSlice())
  {
    // Calculate slice address
    ctuAddress = (pcSlice->getSliceCurStartCUAddr()/pcSlice->getPic()->getNumPartInCU());
  }
  else
  {
    // Calculate slice address
    ctuAddress = (pcSlice->getSliceSegmentCurStartCUAddr()/pcSlice->getPic()->getNumPartInCU());
  }
  //write slice address
  Int sliceSegmentAddress = pcSlice->getPic()->getPicSym()->getCUOrderMap(ctuAddress);

  WRITE_FLAG( sliceSegmentAddress==0, "first_slice_segment_in_pic_flag" );
  if ( pcSlice->getRapPicFlag() )
  {
#if SETTING_NO_OUT_PIC_PRIOR
    WRITE_FLAG( pcSlice->getNoOutputPriorPicsFlag() ? 1 : 0, "no_output_of_prior_pics_flag" );
#else
    WRITE_FLAG( 0, "no_output_of_prior_pics_flag" );
#endif
  }
#if PPS_FIX_DEPTH
  if( pcSlice->getIsDepth() )
  {
    WRITE_UVLC( 1, "slice_pic_parameter_set_id" );
  }
  else
#endif
  WRITE_UVLC( pcSlice->getPPS()->getPPSId(), "slice_pic_parameter_set_id" );
  pcSlice->setDependentSliceSegmentFlag(!pcSlice->isNextSlice());
  if ( pcSlice->getPPS()->getDependentSliceSegmentsEnabledFlag() && (sliceSegmentAddress!=0) )
  {
    WRITE_FLAG( pcSlice->getDependentSliceSegmentFlag() ? 1 : 0, "dependent_slice_segment_flag" );
  }
  if(sliceSegmentAddress>0)
  {
    WRITE_CODE( sliceSegmentAddress, bitsSliceSegmentAddress, "slice_segment_address" );
  }
  if ( !pcSlice->getDependentSliceSegmentFlag() )
  {
#if H_MV    
    Int esb = 0;  //Don't use i, otherwise will shadow something below

    if ( pcSlice->getPPS()->getNumExtraSliceHeaderBits() > esb )
    {
      esb++; 
      WRITE_FLAG( pcSlice->getDiscardableFlag( ) ? 1 : 0 , "discardable_flag" );
    }

    if ( pcSlice->getPPS()->getNumExtraSliceHeaderBits() > esb )
    {
      esb++; 
      WRITE_FLAG( pcSlice->getCrossLayerBlaFlag( ) ? 1 : 0 , "cross_layer_bla_flag" );
    }
    pcSlice->checkCrossLayerBlaFlag( ); 
#if !H_MV_HLS7_GEN
    if ( pcSlice->getPPS()->getNumExtraSliceHeaderBits() > esb )
    {
      esb++; 
      WRITE_FLAG( pcSlice->getPocResetFlag( ) ? 1 : 0 , "poc_reset_flag" );
    }
#endif



    for (; esb < pcSlice->getPPS()->getNumExtraSliceHeaderBits(); esb++)    
#else
    for (Int i = 0; i < pcSlice->getPPS()->getNumExtraSliceHeaderBits(); i++)
#endif
    {
      assert(!!"slice_reserved_undetermined_flag[]");
      WRITE_FLAG(0, "slice_reserved_undetermined_flag[]");
    }

    WRITE_UVLC( pcSlice->getSliceType(),       "slice_type" );

    if( pcSlice->getPPS()->getOutputFlagPresentFlag() )
    {
      WRITE_FLAG( pcSlice->getPicOutputFlag() ? 1 : 0, "pic_output_flag" );
    }

    // in the first version chroma_format_idc is equal to one, thus colour_plane_id will not be present
    assert (pcSlice->getSPS()->getChromaFormatIdc() == 1 );
    // if( separate_colour_plane_flag  ==  1 )
    //   colour_plane_id                                      u(2)
#if H_MV
    if ( (pcSlice->getLayerId() > 0 && !vps->getPocLsbNotPresentFlag( pcSlice->getLayerIdInVps())) || !pcSlice->getIdrPicFlag() )
    {
      Int picOrderCntLSB = (pcSlice->getPOC()-pcSlice->getLastIDR()+(1<<pcSlice->getSPS()->getBitsForPOC())) & ((1<<pcSlice->getSPS()->getBitsForPOC())-1);
      WRITE_CODE( picOrderCntLSB, pcSlice->getSPS()->getBitsForPOC(), "slice_pic_order_cnt_lsb");
      pcSlice->setSlicePicOrderCntLsb( picOrderCntLSB ); 
    }

#endif
    if( !pcSlice->getIdrPicFlag() )
    {
#if !H_MV
      Int picOrderCntLSB = (pcSlice->getPOC()-pcSlice->getLastIDR()+(1<<pcSlice->getSPS()->getBitsForPOC())) & ((1<<pcSlice->getSPS()->getBitsForPOC())-1);
      WRITE_CODE( picOrderCntLSB, pcSlice->getSPS()->getBitsForPOC(), "pic_order_cnt_lsb");
#endif
      TComReferencePictureSet* rps = pcSlice->getRPS();
      
      // check for bitstream restriction stating that:
      // If the current picture is a BLA or CRA picture, the value of NumPocTotalCurr shall be equal to 0.
      // Ideally this process should not be repeated for each slice in a picture
      if (pcSlice->isIRAP())
      {
          for (Int picIdx = 0; picIdx < rps->getNumberOfPictures(); picIdx++)
          {
          assert (!rps->getUsed(picIdx));
          }
        }

      if(pcSlice->getRPSidx() < 0)
      {
        WRITE_FLAG( 0, "short_term_ref_pic_set_sps_flag");
        codeShortTermRefPicSet(pcSlice->getSPS(), rps, true, pcSlice->getSPS()->getRPSList()->getNumberOfReferencePictureSets());
      }
      else
      {
        WRITE_FLAG( 1, "short_term_ref_pic_set_sps_flag");
        Int numBits = 0;
        while ((1 << numBits) < pcSlice->getSPS()->getRPSList()->getNumberOfReferencePictureSets())
        {
          numBits++;
        }
        if (numBits > 0)
        {
          WRITE_CODE( pcSlice->getRPSidx(), numBits, "short_term_ref_pic_set_idx" );          
        }
      }
      if(pcSlice->getSPS()->getLongTermRefsPresent())
      {
        Int numLtrpInSH = rps->getNumberOfLongtermPictures();
        Int ltrpInSPS[MAX_NUM_REF_PICS];
        Int numLtrpInSPS = 0;
        UInt ltrpIndex;
        Int counter = 0;
        for(Int k = rps->getNumberOfPictures()-1; k > rps->getNumberOfPictures()-rps->getNumberOfLongtermPictures()-1; k--) 
        {
          if (findMatchingLTRP(pcSlice, &ltrpIndex, rps->getPOC(k), rps->getUsed(k))) 
          {
            ltrpInSPS[numLtrpInSPS] = ltrpIndex;
            numLtrpInSPS++;
          }
          else
          {
            counter++;
          }
        }
        numLtrpInSH -= numLtrpInSPS;

        Int bitsForLtrpInSPS = 0;
        while (pcSlice->getSPS()->getNumLongTermRefPicSPS() > (1 << bitsForLtrpInSPS))
        {
          bitsForLtrpInSPS++;
        }
        if (pcSlice->getSPS()->getNumLongTermRefPicSPS() > 0) 
        {
          WRITE_UVLC( numLtrpInSPS, "num_long_term_sps");
        }
        WRITE_UVLC( numLtrpInSH, "num_long_term_pics");
        // Note that the LSBs of the LT ref. pic. POCs must be sorted before.
        // Not sorted here because LT ref indices will be used in setRefPicList()
        Int prevDeltaMSB = 0, prevLSB = 0;
        Int offset = rps->getNumberOfNegativePictures() + rps->getNumberOfPositivePictures();
        for(Int i=rps->getNumberOfPictures()-1 ; i > offset-1; i--)
        {
          if (counter < numLtrpInSPS)
          {
            if (bitsForLtrpInSPS > 0)
            {
              WRITE_CODE( ltrpInSPS[counter], bitsForLtrpInSPS, "lt_idx_sps[i]");              
            }
          }
          else 
          {
            WRITE_CODE( rps->getPocLSBLT(i), pcSlice->getSPS()->getBitsForPOC(), "poc_lsb_lt");
            WRITE_FLAG( rps->getUsed(i), "used_by_curr_pic_lt_flag"); 
          }
          WRITE_FLAG( rps->getDeltaPocMSBPresentFlag(i), "delta_poc_msb_present_flag");

          if(rps->getDeltaPocMSBPresentFlag(i))
          {
            Bool deltaFlag = false;
            //  First LTRP from SPS                 ||  First LTRP from SH                              || curr LSB            != prev LSB
            if( (i == rps->getNumberOfPictures()-1) || (i == rps->getNumberOfPictures()-1-numLtrpInSPS) || (rps->getPocLSBLT(i) != prevLSB) )
            {
              deltaFlag = true;
            }
            if(deltaFlag)
            {
              WRITE_UVLC( rps->getDeltaPocMSBCycleLT(i), "delta_poc_msb_cycle_lt[i]" );
            }
            else
            {              
              Int differenceInDeltaMSB = rps->getDeltaPocMSBCycleLT(i) - prevDeltaMSB;
              assert(differenceInDeltaMSB >= 0);
              WRITE_UVLC( differenceInDeltaMSB, "delta_poc_msb_cycle_lt[i]" );
            }
            prevLSB = rps->getPocLSBLT(i);
            prevDeltaMSB = rps->getDeltaPocMSBCycleLT(i);
          }
        }
      }
      if (pcSlice->getSPS()->getTMVPFlagsPresent())
      {
        WRITE_FLAG( pcSlice->getEnableTMVPFlag() ? 1 : 0, "slice_temporal_mvp_enable_flag" );
      }
    }
#if H_MV
    Bool interLayerPredLayerIdcPresentFlag = false; 
    Int layerId = pcSlice->getLayerId(); 
    if( pcSlice->getLayerId() > 0 && !vps->getAllRefLayersActiveFlag() && vps->getNumDirectRefLayers( layerId ) > 0 )
    {   
      WRITE_FLAG( pcSlice->getInterLayerPredEnabledFlag( ) ? 1 : 0 , "inter_layer_pred_enabled_flag" );
      if( pcSlice->getInterLayerPredEnabledFlag() && vps->getNumDirectRefLayers( layerId ) > 1 )
      {            
        if( !vps->getMaxOneActiveRefLayerFlag())  
        {
          WRITE_CODE( pcSlice->getNumInterLayerRefPicsMinus1( ), pcSlice->getNumInterLayerRefPicsMinus1Len( ), "num_inter_layer_ref_pics_minus1" );
        }
        if ( pcSlice->getNumActiveRefLayerPics() != vps->getNumDirectRefLayers( layerId ) )
        {        
          interLayerPredLayerIdcPresentFlag = true; 
          for( Int idx = 0; idx < pcSlice->getNumActiveRefLayerPics(); idx++ )   
          {
            WRITE_CODE( pcSlice->getInterLayerPredLayerIdc( idx ), pcSlice->getInterLayerPredLayerIdcLen( ), "inter_layer_pred_layer_idc" );
          }
        }
      }  
    }
    if ( !interLayerPredLayerIdcPresentFlag )
    {
      for( Int i = 0; i < pcSlice->getNumActiveRefLayerPics(); i++ )   
      {
        assert( pcSlice->getInterLayerPredLayerIdc( i ) == pcSlice->getRefLayerPicIdc( i ) );
      }
    }
#endif
    if(pcSlice->getSPS()->getUseSAO())
    {
      if (pcSlice->getSPS()->getUseSAO())
      {
         WRITE_FLAG( pcSlice->getSaoEnabledFlag(), "slice_sao_luma_flag" );
         WRITE_FLAG( pcSlice->getSaoEnabledFlagChroma(), "slice_sao_chroma_flag" );
      }
    }

    //check if numrefidxes match the defaults. If not, override

    if (!pcSlice->isIntra())
    {
      Bool overrideFlag = (pcSlice->getNumRefIdx( REF_PIC_LIST_0 )!=pcSlice->getPPS()->getNumRefIdxL0DefaultActive()||(pcSlice->isInterB()&&pcSlice->getNumRefIdx( REF_PIC_LIST_1 )!=pcSlice->getPPS()->getNumRefIdxL1DefaultActive()));
#if PPS_FIX_DEPTH
      overrideFlag |= (pcSlice->getIsDepth() && !pcSlice->getViewIndex());
#endif
      WRITE_FLAG( overrideFlag ? 1 : 0,                               "num_ref_idx_active_override_flag");
      if (overrideFlag) 
      {
        WRITE_UVLC( pcSlice->getNumRefIdx( REF_PIC_LIST_0 ) - 1,      "num_ref_idx_l0_active_minus1" );
        if (pcSlice->isInterB())
        {
          WRITE_UVLC( pcSlice->getNumRefIdx( REF_PIC_LIST_1 ) - 1,    "num_ref_idx_l1_active_minus1" );
        }
        else
        {
          pcSlice->setNumRefIdx(REF_PIC_LIST_1, 0);
        }
      }
    }
    else
    {
      pcSlice->setNumRefIdx(REF_PIC_LIST_0, 0);
      pcSlice->setNumRefIdx(REF_PIC_LIST_1, 0);
    }
#if PPS_FIX_DEPTH
    if( (pcSlice->getPPS()->getListsModificationPresentFlag() || (pcSlice->getIsDepth() && !pcSlice->getViewIndex())) && pcSlice->getNumRpsCurrTempList() > 1)
#else
    if( pcSlice->getPPS()->getListsModificationPresentFlag() && pcSlice->getNumRpsCurrTempList() > 1)
#endif
    {
      TComRefPicListModification* refPicListModification = pcSlice->getRefPicListModification();
      if(!pcSlice->isIntra())
      {
        WRITE_FLAG(pcSlice->getRefPicListModification()->getRefPicListModificationFlagL0() ? 1 : 0,       "ref_pic_list_modification_flag_l0" );
        if (pcSlice->getRefPicListModification()->getRefPicListModificationFlagL0())
        {
          Int numRpsCurrTempList0 = pcSlice->getNumRpsCurrTempList();
          if (numRpsCurrTempList0 > 1)
          {
            Int length = 1;
            numRpsCurrTempList0 --;
            while ( numRpsCurrTempList0 >>= 1) 
            {
              length ++;
            }
            for(Int i = 0; i < pcSlice->getNumRefIdx( REF_PIC_LIST_0 ); i++)
            {
              WRITE_CODE( refPicListModification->getRefPicSetIdxL0(i), length, "list_entry_l0");
            }
          }
        }
      }
      if(pcSlice->isInterB())
      {    
        WRITE_FLAG(pcSlice->getRefPicListModification()->getRefPicListModificationFlagL1() ? 1 : 0,       "ref_pic_list_modification_flag_l1" );
        if (pcSlice->getRefPicListModification()->getRefPicListModificationFlagL1())
        {
          Int numRpsCurrTempList1 = pcSlice->getNumRpsCurrTempList();
          if ( numRpsCurrTempList1 > 1 )
          {
            Int length = 1;
            numRpsCurrTempList1 --;
            while ( numRpsCurrTempList1 >>= 1)
            {
              length ++;
            }
            for(Int i = 0; i < pcSlice->getNumRefIdx( REF_PIC_LIST_1 ); i++)
            {
              WRITE_CODE( refPicListModification->getRefPicSetIdxL1(i), length, "list_entry_l1");
            }
          }
        }
      }
    }
    
    if (pcSlice->isInterB())
    {
      WRITE_FLAG( pcSlice->getMvdL1ZeroFlag() ? 1 : 0,   "mvd_l1_zero_flag");
    }

    if(!pcSlice->isIntra())
    {
      if (!pcSlice->isIntra() && pcSlice->getPPS()->getCabacInitPresentFlag())
      {
        SliceType sliceType   = pcSlice->getSliceType();
        Int  encCABACTableIdx = pcSlice->getPPS()->getEncCABACTableIdx();
        Bool encCabacInitFlag = (sliceType!=encCABACTableIdx && encCABACTableIdx!=I_SLICE) ? true : false;
        pcSlice->setCabacInitFlag( encCabacInitFlag );
        WRITE_FLAG( encCabacInitFlag?1:0, "cabac_init_flag" );
      }
    }

    if ( pcSlice->getEnableTMVPFlag() )
    {
      if ( pcSlice->getSliceType() == B_SLICE )
      {
        WRITE_FLAG( pcSlice->getColFromL0Flag(), "collocated_from_l0_flag" );
      }

      if ( pcSlice->getSliceType() != I_SLICE &&
        ((pcSlice->getColFromL0Flag()==1 && pcSlice->getNumRefIdx(REF_PIC_LIST_0)>1)||
        (pcSlice->getColFromL0Flag()==0  && pcSlice->getNumRefIdx(REF_PIC_LIST_1)>1)))
      {
        WRITE_UVLC( pcSlice->getColRefIdx(), "collocated_ref_idx" );
      }
    }
    if ( (pcSlice->getPPS()->getUseWP() && pcSlice->getSliceType()==P_SLICE) || (pcSlice->getPPS()->getWPBiPred() && pcSlice->getSliceType()==B_SLICE) )
    {
      xCodePredWeightTable( pcSlice );
    }
#if H_3D_IC
    else if( pcSlice->getViewIndex() && ( pcSlice->getSliceType() == P_SLICE || pcSlice->getSliceType() == B_SLICE ) && !pcSlice->getIsDepth())
    {
      WRITE_FLAG( pcSlice->getApplyIC() ? 1 : 0, "slice_ic_enable_flag" );
      if( pcSlice->getApplyIC() )
      {
        WRITE_FLAG( pcSlice->getIcSkipParseFlag() ? 1 : 0, "ic_skip_mergeidx0" );
      }
    }
#endif

#if H_3D_IV_MERGE
    assert(pcSlice->getMaxNumMergeCand()<=MRG_MAX_NUM_CANDS_MEM);
#else
    assert(pcSlice->getMaxNumMergeCand()<=MRG_MAX_NUM_CANDS);
#endif
    if (!pcSlice->isIntra())
    {
#if H_3D_IV_MERGE
      if(pcSlice->getIsDepth())
      {
        Bool bMPIFlag = pcSlice->getVPS()->getMPIFlag( pcSlice->getLayerIdInVps() ) ;
        Bool ivMvPredFlag = pcSlice->getVPS()->getIvMvPredFlag( pcSlice->getLayerIdInVps() ) ;
        WRITE_UVLC( ( ( bMPIFlag || ivMvPredFlag ) ? MRG_MAX_NUM_CANDS_MEM : MRG_MAX_NUM_CANDS ) - pcSlice->getMaxNumMergeCand(), "five_minus_max_num_merge_cand");
      }
      else
      {
        Bool ivMvPredFlag = pcSlice->getVPS()->getIvMvPredFlag( pcSlice->getLayerIdInVps() ) ;
        WRITE_UVLC( ( ivMvPredFlag ? MRG_MAX_NUM_CANDS_MEM : MRG_MAX_NUM_CANDS ) - pcSlice->getMaxNumMergeCand(), "five_minus_max_num_merge_cand");
      }
#else
      WRITE_UVLC(MRG_MAX_NUM_CANDS - pcSlice->getMaxNumMergeCand(), "five_minus_max_num_merge_cand");
#endif
    }
    Int iCode = pcSlice->getSliceQp() - ( pcSlice->getPPS()->getPicInitQPMinus26() + 26 );
    WRITE_SVLC( iCode, "slice_qp_delta" ); 
    if (pcSlice->getPPS()->getSliceChromaQpFlag())
    {
      iCode = pcSlice->getSliceQpDeltaCb();
      WRITE_SVLC( iCode, "slice_qp_delta_cb" );
      iCode = pcSlice->getSliceQpDeltaCr();
      WRITE_SVLC( iCode, "slice_qp_delta_cr" );
    }
    if (pcSlice->getPPS()->getDeblockingFilterControlPresentFlag())
    {
      if (pcSlice->getPPS()->getDeblockingFilterOverrideEnabledFlag() )
      {
        WRITE_FLAG(pcSlice->getDeblockingFilterOverrideFlag(), "deblocking_filter_override_flag");
      }
      if (pcSlice->getDeblockingFilterOverrideFlag())
      {
        WRITE_FLAG(pcSlice->getDeblockingFilterDisable(), "slice_disable_deblocking_filter_flag");
        if(!pcSlice->getDeblockingFilterDisable())
        {
          WRITE_SVLC (pcSlice->getDeblockingFilterBetaOffsetDiv2(), "slice_beta_offset_div2");
          WRITE_SVLC (pcSlice->getDeblockingFilterTcOffsetDiv2(),   "slice_tc_offset_div2");
        }
      }
    }

    Bool isSAOEnabled = (!pcSlice->getSPS()->getUseSAO())?(false):(pcSlice->getSaoEnabledFlag()||pcSlice->getSaoEnabledFlagChroma());
    Bool isDBFEnabled = (!pcSlice->getDeblockingFilterDisable());

    if(pcSlice->getPPS()->getLoopFilterAcrossSlicesEnabledFlag() && ( isSAOEnabled || isDBFEnabled ))
    {
      WRITE_FLAG(pcSlice->getLFCrossSliceBoundaryFlag()?1:0, "slice_loop_filter_across_slices_enabled_flag");
    }
  }
#if H_3D
  if( pcSlice->getVPS()->hasCamParInSliceHeader( pcSlice->getViewIndex() ) && !pcSlice->getIsDepth() )
  {
    for( UInt uiId = 0; uiId < pcSlice->getViewIndex(); uiId++ )
    {
      WRITE_SVLC( pcSlice->getCodedScale    ()[ uiId ],                                     "cp_scale" );
      WRITE_SVLC( pcSlice->getCodedOffset   ()[ uiId ],                                     "cp_off" );
      WRITE_SVLC( pcSlice->getInvCodedScale ()[ uiId ] + pcSlice->getCodedScale ()[ uiId ], "cp_inv_scale_plus_scale" );
      WRITE_SVLC( pcSlice->getInvCodedOffset()[ uiId ] + pcSlice->getCodedOffset()[ uiId ], "cp_inv_off_plus_off" );
    }
  }
#endif


#if !H_MV
  if(pcSlice->getPPS()->getSliceHeaderExtensionPresentFlag())
  {
    WRITE_UVLC(0,"slice_header_extension_length");
  }
#else
  if(pcSlice->getPPS()->getSliceHeaderExtensionPresentFlag())
  {
    // Derive the value of PocMsbValRequiredFlag
    pcSlice->setPocMsbValRequiredFlag( pcSlice->getCraPicFlag() || pcSlice->getBlaPicFlag()
                                          /* || related to vps_poc_lsb_aligned_flag */
                                          );

    // Determine value of SH extension length.
    Int shExtnLengthInBit = 0;
    if (pcSlice->getPPS()->getPocResetInfoPresentFlag())
    {
      shExtnLengthInBit += 2;
    }
    if (pcSlice->getPocResetIdc() > 0)
    {
      shExtnLengthInBit += 6;
    }
    if (pcSlice->getPocResetIdc() == 3)
    {
      shExtnLengthInBit += (pcSlice->getSPS()->getBitsForPOC() + 1);
    }


    if( !pcSlice->getPocMsbValRequiredFlag() /* TODO &&  pcSlice->getVPS()->getVpsPocLsbAlignedFlag() */ )
    {
      shExtnLengthInBit++;    // For poc_msb_val_present_flag
    }
    else
    {
      if( pcSlice->getPocMsbValRequiredFlag() )
      {
        pcSlice->setPocMsbValPresentFlag( true );
      }
      else
      {
        pcSlice->setPocMsbValPresentFlag( false );
      }
    }

    if( pcSlice->getPocMsbValPresentFlag() )
    {
//      Int iMaxPOClsb = 1<< pcSlice->getSPS()->getBitsForPOC(); currently unused

      UInt lengthVal = 1;
      UInt tempVal = pcSlice->getPocMsbVal() + 1;
      assert ( tempVal );
      while( 1 != tempVal )
      {
        tempVal >>= 1;
        lengthVal += 2;
      }
      shExtnLengthInBit += lengthVal;
    }
    Int shExtnAdditionalBits = 0;
    if(shExtnLengthInBit % 8 != 0)
    {
      shExtnAdditionalBits = 8 - (shExtnLengthInBit % 8);
    }
    pcSlice->setSliceSegmentHeaderExtensionLength((shExtnLengthInBit + shExtnAdditionalBits) / 8);
     

    WRITE_UVLC( pcSlice->getSliceSegmentHeaderExtensionLength( ), "slice_segment_header_extension_length" );
    UInt posFollSliceSegHeaderExtLen = m_pcBitIf->getNumberOfWrittenBits();
    if( pcSlice->getPPS()->getPocResetInfoPresentFlag() )
    {
      WRITE_CODE( pcSlice->getPocResetIdc( ), 2, "poc_reset_idc" );
    }
    else
    {
      assert( pcSlice->getPocResetIdc( ) == 0 );
    }

    pcSlice->checkPocResetIdc(); 

    if( pcSlice->getPocResetIdc() !=  0 )
    {
      WRITE_CODE( pcSlice->getPocResetPeriodId( ), 6, "poc_reset_period_id" );
    }
    
    if( pcSlice->getPocResetIdc() ==  3 ) 
    {
      WRITE_FLAG( pcSlice->getFullPocResetFlag( ) ? 1 : 0 , "full_poc_reset_flag" );
      WRITE_CODE( pcSlice->getPocLsbVal( ), pcSlice->getPocLsbValLen() , "poc_lsb_val" );
    }              
    pcSlice->checkPocLsbVal(); 

    if( !pcSlice->getPocMsbValRequiredFlag() /* TODO &&  pcSlice->getVPS()->getVpsPocLsbAlignedFlag() */ )
    {
      WRITE_FLAG( pcSlice->getPocMsbValPresentFlag( ) ? 1 : 0 , "poc_msb_val_present_flag" );
    }
    else
    {
      assert( pcSlice->getPocMsbValPresentFlag() ==  pcSlice->inferPocMsbValPresentFlag( ) ); 
    }
    
    if( pcSlice->getPocMsbValPresentFlag() )
    {
      WRITE_UVLC( pcSlice->getPocMsbVal( ), "poc_msb_val" );
    }
    
    while( ( m_pcBitIf->getNumberOfWrittenBits() - posFollSliceSegHeaderExtLen ) < pcSlice->getSliceSegmentHeaderExtensionLength() * 8 )
    {
      WRITE_FLAG( 0, "slice_segment_header_extension_data_bit" );
    }

    assert( ( m_pcBitIf->getNumberOfWrittenBits() - posFollSliceSegHeaderExtLen ) == pcSlice->getSliceSegmentHeaderExtensionLength() * 8  ); 
  }
#endif 
}

Void TEncCavlc::codePTL( TComPTL* pcPTL, Bool profilePresentFlag, Int maxNumSubLayersMinus1)
{
  if(profilePresentFlag)
  {
    codeProfileTier(pcPTL->getGeneralPTL());    // general_...
  }
  WRITE_CODE( pcPTL->getGeneralPTL()->getLevelIdc(), 8, "general_level_idc" );

  for (Int i = 0; i < maxNumSubLayersMinus1; i++)
  {
#if !H_MV
    if(profilePresentFlag)
    {
#endif
      WRITE_FLAG( pcPTL->getSubLayerProfilePresentFlag(i), "sub_layer_profile_present_flag[i]" );
#if !H_MV
    }
#endif
    
    WRITE_FLAG( pcPTL->getSubLayerLevelPresentFlag(i),   "sub_layer_level_present_flag[i]" );
  }
  
  if (maxNumSubLayersMinus1 > 0)
  {
    for (Int i = maxNumSubLayersMinus1; i < 8; i++)
    {
      WRITE_CODE(0, 2, "reserved_zero_2bits");
    }
  }
  
  for(Int i = 0; i < maxNumSubLayersMinus1; i++)
  {
    if( profilePresentFlag && pcPTL->getSubLayerProfilePresentFlag(i) )
    {
      codeProfileTier(pcPTL->getSubLayerPTL(i));  // sub_layer_...
    }
    if( pcPTL->getSubLayerLevelPresentFlag(i) )
    {
      WRITE_CODE( pcPTL->getSubLayerPTL(i)->getLevelIdc(), 8, "sub_layer_level_idc[i]" );
    }
  }
}
Void TEncCavlc::codeProfileTier( ProfileTierLevel* ptl )
{
  WRITE_CODE( ptl->getProfileSpace(), 2 ,     "XXX_profile_space[]");
  WRITE_FLAG( ptl->getTierFlag    (),         "XXX_tier_flag[]"    );
  WRITE_CODE( ptl->getProfileIdc  (), 5 ,     "XXX_profile_idc[]"  );   
  for(Int j = 0; j < 32; j++)
  {
    WRITE_FLAG( ptl->getProfileCompatibilityFlag(j), "XXX_profile_compatibility_flag[][j]");   
  }

  WRITE_FLAG(ptl->getProgressiveSourceFlag(),   "general_progressive_source_flag");
  WRITE_FLAG(ptl->getInterlacedSourceFlag(),    "general_interlaced_source_flag");
  WRITE_FLAG(ptl->getNonPackedConstraintFlag(), "general_non_packed_constraint_flag");
  WRITE_FLAG(ptl->getFrameOnlyConstraintFlag(), "general_frame_only_constraint_flag");
  
  WRITE_CODE(0 , 16, "XXX_reserved_zero_44bits[0..15]");
  WRITE_CODE(0 , 16, "XXX_reserved_zero_44bits[16..31]");
  WRITE_CODE(0 , 12, "XXX_reserved_zero_44bits[32..43]");
    }

/**
 - write wavefront substreams sizes for the slice header.
 .
 \param pcSlice Where we find the substream size information.
 */
Void  TEncCavlc::codeTilesWPPEntryPoint( TComSlice* pSlice )
{
  if (!pSlice->getPPS()->getTilesEnabledFlag() && !pSlice->getPPS()->getEntropyCodingSyncEnabledFlag())
  {
    return;
  }
  UInt numEntryPointOffsets = 0, offsetLenMinus1 = 0, maxOffset = 0;
  Int  numZeroSubstreamsAtStartOfSlice  = 0;
  UInt *entryPointOffset = NULL;
  if ( pSlice->getPPS()->getTilesEnabledFlag() )
  {
    numEntryPointOffsets = pSlice->getTileLocationCount();
    entryPointOffset     = new UInt[numEntryPointOffsets];
    for (Int idx=0; idx<pSlice->getTileLocationCount(); idx++)
    {
      if ( idx == 0 )
      {
        entryPointOffset [ idx ] = pSlice->getTileLocation( 0 );
      }
      else
      {
        entryPointOffset [ idx ] = pSlice->getTileLocation( idx ) - pSlice->getTileLocation( idx-1 );
      }

      if ( entryPointOffset[ idx ] > maxOffset )
      {
        maxOffset = entryPointOffset[ idx ];
      }
    }
  }
  else if ( pSlice->getPPS()->getEntropyCodingSyncEnabledFlag() )
  {
    UInt* pSubstreamSizes               = pSlice->getSubstreamSizes();
    Int maxNumParts                       = pSlice->getPic()->getNumPartInCU();
    numZeroSubstreamsAtStartOfSlice       = pSlice->getSliceSegmentCurStartCUAddr()/maxNumParts/pSlice->getPic()->getFrameWidthInCU();
    Int  numZeroSubstreamsAtEndOfSlice    = pSlice->getPic()->getFrameHeightInCU()-1 - ((pSlice->getSliceSegmentCurEndCUAddr()-1)/maxNumParts/pSlice->getPic()->getFrameWidthInCU());
    numEntryPointOffsets                  = pSlice->getPPS()->getNumSubstreams() - numZeroSubstreamsAtStartOfSlice - numZeroSubstreamsAtEndOfSlice - 1;
    pSlice->setNumEntryPointOffsets(numEntryPointOffsets);
    entryPointOffset           = new UInt[numEntryPointOffsets];
    for (Int idx=0; idx<numEntryPointOffsets; idx++)
    {
      entryPointOffset[ idx ] = ( pSubstreamSizes[ idx+numZeroSubstreamsAtStartOfSlice ] >> 3 ) ;
      if ( entryPointOffset[ idx ] > maxOffset )
      {
        maxOffset = entryPointOffset[ idx ];
      }
    }
  }
  // Determine number of bits "offsetLenMinus1+1" required for entry point information
  offsetLenMinus1 = 0;
  while (maxOffset >= (1u << (offsetLenMinus1 + 1)))
  {
    offsetLenMinus1++;
    assert(offsetLenMinus1 + 1 < 32);    
  }

  WRITE_UVLC(numEntryPointOffsets, "num_entry_point_offsets");
  if (numEntryPointOffsets>0)
  {
    WRITE_UVLC(offsetLenMinus1, "offset_len_minus1");
  }

  for (UInt idx=0; idx<numEntryPointOffsets; idx++)
  {
    WRITE_CODE(entryPointOffset[ idx ]-1, offsetLenMinus1+1, "entry_point_offset_minus1");
  }

  delete [] entryPointOffset;
}

Void TEncCavlc::codeTerminatingBit      ( UInt uilsLast )
{
}

Void TEncCavlc::codeSliceFinish ()
{
}

Void TEncCavlc::codeMVPIdx ( TComDataCU* pcCU, UInt uiAbsPartIdx, RefPicList eRefList )
{
  assert(0);
}

Void TEncCavlc::codePartSize( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth )
{
  assert(0);
}

Void TEncCavlc::codePredMode( TComDataCU* pcCU, UInt uiAbsPartIdx )
{
  assert(0);
}

Void TEncCavlc::codeMergeFlag    ( TComDataCU* pcCU, UInt uiAbsPartIdx )
{
  assert(0);
}

Void TEncCavlc::codeMergeIndex    ( TComDataCU* pcCU, UInt uiAbsPartIdx )
{
  assert(0);
}

#if H_3D_ARP
Void TEncCavlc::codeARPW( TComDataCU* pcCU, UInt uiAbsPartIdx )
{
  assert(0);
}
#endif

#if H_3D_IC
Void TEncCavlc::codeICFlag( TComDataCU* pcCU, UInt uiAbsPartIdx )
{
  assert(0);
}
#endif

Void TEncCavlc::codeInterModeFlag( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth, UInt uiEncMode )
{
  assert(0);
}

Void TEncCavlc::codeCUTransquantBypassFlag( TComDataCU* pcCU, UInt uiAbsPartIdx )
{
  assert(0);
}

Void TEncCavlc::codeSkipFlag( TComDataCU* pcCU, UInt uiAbsPartIdx )
{
  assert(0);
}

Void TEncCavlc::codeSplitFlag   ( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth )
{
  assert(0);
}

Void TEncCavlc::codeTransformSubdivFlag( UInt uiSymbol, UInt uiCtx )
{
  assert(0);
}

Void TEncCavlc::codeQtCbf( TComDataCU* pcCU, UInt uiAbsPartIdx, TextType eType, UInt uiTrDepth )
{
  assert(0);
}

Void TEncCavlc::codeQtRootCbf( TComDataCU* pcCU, UInt uiAbsPartIdx )
{
  assert(0);
}

Void TEncCavlc::codeQtCbfZero( TComDataCU* pcCU, TextType eType, UInt uiTrDepth )
{
  assert(0);
}
Void TEncCavlc::codeQtRootCbfZero( TComDataCU* pcCU )
{
  assert(0);
}

Void TEncCavlc::codeTransformSkipFlags (TComDataCU* pcCU, UInt uiAbsPartIdx, UInt width, UInt height, TextType eTType )
{
  assert(0);
}

/** Code I_PCM information.
 * \param pcCU pointer to CU
 * \param uiAbsPartIdx CU index
 * \returns Void
 */
Void TEncCavlc::codeIPCMInfo( TComDataCU* pcCU, UInt uiAbsPartIdx )
{
  assert(0);
}

Void TEncCavlc::codeIntraDirLumaAng( TComDataCU* pcCU, UInt uiAbsPartIdx, Bool isMultiple)
{
  assert(0);
}

Void TEncCavlc::codeIntraDirChroma( TComDataCU* pcCU, UInt uiAbsPartIdx )
{
  assert(0);
}

Void TEncCavlc::codeInterDir( TComDataCU* pcCU, UInt uiAbsPartIdx )
{
  assert(0);
}

Void TEncCavlc::codeRefFrmIdx( TComDataCU* pcCU, UInt uiAbsPartIdx, RefPicList eRefList )
{
  assert(0);
}

Void TEncCavlc::codeMvd( TComDataCU* pcCU, UInt uiAbsPartIdx, RefPicList eRefList )
{
  assert(0);
}

Void TEncCavlc::codeDeltaQP( TComDataCU* pcCU, UInt uiAbsPartIdx )
{
  Int iDQp  = pcCU->getQP( uiAbsPartIdx ) - pcCU->getRefQP( uiAbsPartIdx );

  Int qpBdOffsetY =  pcCU->getSlice()->getSPS()->getQpBDOffsetY();
  iDQp = (iDQp + 78 + qpBdOffsetY + (qpBdOffsetY/2)) % (52 + qpBdOffsetY) - 26 - (qpBdOffsetY/2);

  xWriteSvlc( iDQp );
  
  return;
}

Void TEncCavlc::codeCoeffNxN    ( TComDataCU* pcCU, TCoeff* pcCoef, UInt uiAbsPartIdx, UInt uiWidth, UInt uiHeight, UInt uiDepth, TextType eTType )
{
  assert(0);
}

Void TEncCavlc::estBit( estBitsSbacStruct* pcEstBitsCabac, Int width, Int height, TextType eTType )
{
  // printf("error : no VLC mode support in this version\n");
  return;
}

// ====================================================================================================================
// Protected member functions
// ====================================================================================================================

/** code explicit wp tables
 * \param TComSlice* pcSlice
 * \returns Void
 */
Void TEncCavlc::xCodePredWeightTable( TComSlice* pcSlice )
{
  wpScalingParam  *wp;
  Bool            bChroma     = true; // color always present in HEVC ?
  Int             iNbRef       = (pcSlice->getSliceType() == B_SLICE ) ? (2) : (1);
  Bool            bDenomCoded  = false;
  UInt            uiMode = 0;
  UInt            uiTotalSignalledWeightFlags = 0;
  if ( (pcSlice->getSliceType()==P_SLICE && pcSlice->getPPS()->getUseWP()) || (pcSlice->getSliceType()==B_SLICE && pcSlice->getPPS()->getWPBiPred()) )
  {
    uiMode = 1; // explicit
  }
  if(uiMode == 1)
  {

    for ( Int iNumRef=0 ; iNumRef<iNbRef ; iNumRef++ ) 
    {
      RefPicList  eRefPicList = ( iNumRef ? REF_PIC_LIST_1 : REF_PIC_LIST_0 );

      for ( Int iRefIdx=0 ; iRefIdx<pcSlice->getNumRefIdx(eRefPicList) ; iRefIdx++ ) 
      {
        pcSlice->getWpScaling(eRefPicList, iRefIdx, wp);
        if ( !bDenomCoded ) 
        {
          Int iDeltaDenom;
          WRITE_UVLC( wp[0].uiLog2WeightDenom, "luma_log2_weight_denom" );     // ue(v): luma_log2_weight_denom

          if( bChroma )
          {
            iDeltaDenom = (wp[1].uiLog2WeightDenom - wp[0].uiLog2WeightDenom);
            WRITE_SVLC( iDeltaDenom, "delta_chroma_log2_weight_denom" );       // se(v): delta_chroma_log2_weight_denom
          }
          bDenomCoded = true;
        }
        WRITE_FLAG( wp[0].bPresentFlag, "luma_weight_lX_flag" );               // u(1): luma_weight_lX_flag
        uiTotalSignalledWeightFlags += wp[0].bPresentFlag;
      }
      if (bChroma) 
      {
        for ( Int iRefIdx=0 ; iRefIdx<pcSlice->getNumRefIdx(eRefPicList) ; iRefIdx++ ) 
        {
          pcSlice->getWpScaling(eRefPicList, iRefIdx, wp);
          WRITE_FLAG( wp[1].bPresentFlag, "chroma_weight_lX_flag" );           // u(1): chroma_weight_lX_flag
          uiTotalSignalledWeightFlags += 2*wp[1].bPresentFlag;
        }
      }

      for ( Int iRefIdx=0 ; iRefIdx<pcSlice->getNumRefIdx(eRefPicList) ; iRefIdx++ ) 
      {
        pcSlice->getWpScaling(eRefPicList, iRefIdx, wp);
        if ( wp[0].bPresentFlag ) 
        {
          Int iDeltaWeight = (wp[0].iWeight - (1<<wp[0].uiLog2WeightDenom));
          WRITE_SVLC( iDeltaWeight, "delta_luma_weight_lX" );                  // se(v): delta_luma_weight_lX
          WRITE_SVLC( wp[0].iOffset, "luma_offset_lX" );                       // se(v): luma_offset_lX
        }

        if ( bChroma ) 
        {
          if ( wp[1].bPresentFlag )
          {
            for ( Int j=1 ; j<3 ; j++ ) 
            {
              Int iDeltaWeight = (wp[j].iWeight - (1<<wp[1].uiLog2WeightDenom));
              WRITE_SVLC( iDeltaWeight, "delta_chroma_weight_lX" );            // se(v): delta_chroma_weight_lX

              Int pred = ( 128 - ( ( 128*wp[j].iWeight)>>(wp[j].uiLog2WeightDenom) ) );
              Int iDeltaChroma = (wp[j].iOffset - pred);
              WRITE_SVLC( iDeltaChroma, "delta_chroma_offset_lX" );            // se(v): delta_chroma_offset_lX
            }
          }
        }
      }
    }
    assert(uiTotalSignalledWeightFlags<=24);
  }
}

/** code quantization matrix
 *  \param scalingList quantization matrix information
 */
Void TEncCavlc::codeScalingList( TComScalingList* scalingList )
{
  UInt listId,sizeId;
  Bool scalingListPredModeFlag;

    //for each size
    for(sizeId = 0; sizeId < SCALING_LIST_SIZE_NUM; sizeId++)
    {
      for(listId = 0; listId < g_scalingListNum[sizeId]; listId++)
      {
        scalingListPredModeFlag = scalingList->checkPredMode( sizeId, listId );
        WRITE_FLAG( scalingListPredModeFlag, "scaling_list_pred_mode_flag" );
        if(!scalingListPredModeFlag)// Copy Mode
        {
          WRITE_UVLC( (Int)listId - (Int)scalingList->getRefMatrixId (sizeId,listId), "scaling_list_pred_matrix_id_delta");
        }
        else// DPCM Mode
        {
          xCodeScalingList(scalingList, sizeId, listId);
        }
      }
    }
  return;
}
/** code DPCM
 * \param scalingList quantization matrix information
 * \param sizeIdc size index
 * \param listIdc list index
 */
Void TEncCavlc::xCodeScalingList(TComScalingList* scalingList, UInt sizeId, UInt listId)
{
  Int coefNum = min(MAX_MATRIX_COEF_NUM,(Int)g_scalingListSize[sizeId]);
  UInt* scan  = (sizeId == 0) ? g_auiSigLastScan [ SCAN_DIAG ] [ 1 ] :  g_sigLastScanCG32x32;
  Int nextCoef = SCALING_LIST_START_VALUE;
  Int data;
  Int *src = scalingList->getScalingListAddress(sizeId, listId);
    if( sizeId > SCALING_LIST_8x8 )
    {
      WRITE_SVLC( scalingList->getScalingListDC(sizeId,listId) - 8, "scaling_list_dc_coef_minus8");
      nextCoef = scalingList->getScalingListDC(sizeId,listId);
    }
    for(Int i=0;i<coefNum;i++)
    {
      data = src[scan[i]] - nextCoef;
      nextCoef = src[scan[i]];
      if(data > 127)
      {
        data = data - 256;
      }
      if(data < -128)
      {
        data = data + 256;
      }

      WRITE_SVLC( data,  "scaling_list_delta_coef");
    }
}
Bool TEncCavlc::findMatchingLTRP ( TComSlice* pcSlice, UInt *ltrpsIndex, Int ltrpPOC, Bool usedFlag )
{
  // Bool state = true, state2 = false;
  Int lsb = ltrpPOC & ((1<<pcSlice->getSPS()->getBitsForPOC())-1);
  for (Int k = 0; k < pcSlice->getSPS()->getNumLongTermRefPicSPS(); k++)
  {
    if ( (lsb == pcSlice->getSPS()->getLtRefPicPocLsbSps(k)) && (usedFlag == pcSlice->getSPS()->getUsedByCurrPicLtSPSFlag(k)) )
    {
      *ltrpsIndex = k;
      return true;
    }
  }
  return false;
}
Bool TComScalingList::checkPredMode(UInt sizeId, UInt listId)
{
  for(Int predListIdx = (Int)listId ; predListIdx >= 0; predListIdx--)
  {
    if( !memcmp(getScalingListAddress(sizeId,listId),((listId == predListIdx) ?
      getScalingListDefaultAddress(sizeId, predListIdx): getScalingListAddress(sizeId, predListIdx)),sizeof(Int)*min(MAX_MATRIX_COEF_NUM,(Int)g_scalingListSize[sizeId])) // check value of matrix
     && ((sizeId < SCALING_LIST_16x16) || (getScalingListDC(sizeId,listId) == getScalingListDC(sizeId,predListIdx)))) // check DC value
    {
      setRefMatrixId(sizeId, listId, predListIdx);
      return false;
    }
  }
  return true;
}

#if H_3D_INTER_SDC
Void TEncCavlc::codeDeltaDC( TComDataCU* pcCU, UInt uiAbsPartIdx )
{
  assert(0);
}

Void TEncCavlc::codeSDCFlag ( TComDataCU* pcCU, UInt uiAbsPartIdx )
{
  assert(0);
}

#endif
    
#if H_3D_DBBP
Void TEncCavlc::codeDBBPFlag    ( TComDataCU* pcCU, UInt uiAbsPartIdx )
{
  assert(0);
}
#endif
//! \}
