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

/** \file     TEncCavlc.cpp
    \brief    CAVLC encoder class
*/

#include "../TLibCommon/CommonDef.h"
#include "TEncCavlc.h"
#include "SEIwrite.h"

#if NH_MV
#include "TEncTop.h"
#endif

//! \ingroup TLibEncoder
//! \{

#if ENC_DEC_TRACE

#if !H_MV_ENC_DEC_TRAC
Void  xTraceVPSHeader ()
{
  fprintf( g_hTrace, "=========== Video Parameter Set     ===========\n" );
}

Void  xTraceSPSHeader ()
{
  fprintf( g_hTrace, "=========== Sequence Parameter Set  ===========\n" );
}

Void  xTracePPSHeader ()
{
  fprintf( g_hTrace, "=========== Picture Parameter Set  ===========\n");
}

Void  xTraceSliceHeader ( )
{
  fprintf( g_hTrace, "=========== Slice ===========\n");
}

Void  xTraceAccessUnitDelimiter ()
{
  fprintf( g_hTrace, "=========== Access Unit Delimiter ===========\n");
}

#endif
#endif

Void AUDWriter::codeAUD(TComBitIf& bs, const Int pictureType)
{
#if ENC_DEC_TRACE
  xTraceAccessUnitDelimiter();
#endif

  assert (pictureType < 3);
  setBitstream(&bs);
  WRITE_CODE(pictureType, 3, "pic_type");
  xWriteRbspTrailingBits();
}

// ====================================================================================================================
// Constructor / destructor / create / destroy
// ====================================================================================================================

TEncCavlc::TEncCavlc()
{
  m_pcBitIf           = NULL;
}

TEncCavlc::~TEncCavlc()
{
}


// ====================================================================================================================
// Public member functions
// ====================================================================================================================

Void TEncCavlc::resetEntropy(const TComSlice* /*pSlice*/)
{
}


Void TEncCavlc::codeShortTermRefPicSet( const TComReferencePictureSet* rps, Bool calledFromSliceHeader, Int idx)
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


Void TEncCavlc::codePPS( const TComPPS* pcPPS )
{
#if ENC_DEC_TRACE
#if H_MV_ENC_DEC_TRAC
  tracePSHeader( "PPS", pcPPS->getLayerId() ); 
#else
  xTracePPSHeader ();
#endif
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

  WRITE_SVLC( pcPPS->getQpOffset(COMPONENT_Cb), "pps_cb_qp_offset" );
  WRITE_SVLC( pcPPS->getQpOffset(COMPONENT_Cr), "pps_cr_qp_offset" );

  WRITE_FLAG( pcPPS->getSliceChromaQpFlag() ? 1 : 0,          "pps_slice_chroma_qp_offsets_present_flag" );

  WRITE_FLAG( pcPPS->getUseWP() ? 1 : 0,  "weighted_pred_flag" );   // Use of Weighting Prediction (P_SLICE)
  WRITE_FLAG( pcPPS->getWPBiPred() ? 1 : 0, "weighted_bipred_flag" );  // Use of Weighting Bi-Prediction (B_SLICE)
  WRITE_FLAG( pcPPS->getTransquantBypassEnableFlag() ? 1 : 0, "transquant_bypass_enable_flag" );
  WRITE_FLAG( pcPPS->getTilesEnabledFlag()             ? 1 : 0, "tiles_enabled_flag" );
  WRITE_FLAG( pcPPS->getEntropyCodingSyncEnabledFlag() ? 1 : 0, "entropy_coding_sync_enabled_flag" );
  if( pcPPS->getTilesEnabledFlag() )
  {
    WRITE_UVLC( pcPPS->getNumTileColumnsMinus1(),                                    "num_tile_columns_minus1" );
    WRITE_UVLC( pcPPS->getNumTileRowsMinus1(),                                       "num_tile_rows_minus1" );
    WRITE_FLAG( pcPPS->getTileUniformSpacingFlag(),                                  "uniform_spacing_flag" );
    if( !pcPPS->getTileUniformSpacingFlag() )
    {
      for(UInt i=0; i<pcPPS->getNumTileColumnsMinus1(); i++)
      {
        WRITE_UVLC( pcPPS->getTileColumnWidth(i)-1,                                  "column_width_minus1" );
      }
      for(UInt i=0; i<pcPPS->getNumTileRowsMinus1(); i++)
      {
        WRITE_UVLC( pcPPS->getTileRowHeight(i)-1,                                    "row_height_minus1" );
      }
    }
    if(pcPPS->getNumTileColumnsMinus1() !=0 || pcPPS->getNumTileRowsMinus1() !=0)
    {
      WRITE_FLAG( pcPPS->getLoopFilterAcrossTilesEnabledFlag()?1 : 0,          "loop_filter_across_tiles_enabled_flag");
    }
  }
  WRITE_FLAG( pcPPS->getLoopFilterAcrossSlicesEnabledFlag()?1 : 0,        "pps_loop_filter_across_slices_enabled_flag");
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
  WRITE_FLAG( pcPPS->getScalingListPresentFlag() ? 1 : 0,                          "pps_scaling_list_data_present_flag" );
  if( pcPPS->getScalingListPresentFlag() )
  {
    codeScalingList( pcPPS->getScalingList() );
  }
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
#if !NH_MV
  Bool pps_extension_present_flag=false;
  Bool pps_extension_flags[NUM_PPS_EXTENSION_FLAGS]={false};

  pps_extension_flags[PPS_EXT__REXT] = pcPPS->getPpsRangeExtension().settingsDifferFromDefaults(pcPPS->getUseTransformSkip());

  // Other PPS extension flags checked here.

  for(Int i=0; i<NUM_PPS_EXTENSION_FLAGS; i++)
  {
    pps_extension_present_flag|=pps_extension_flags[i];
  }

  WRITE_FLAG( (pps_extension_present_flag?1:0), "pps_extension_present_flag" );

  if (pps_extension_present_flag)
  {
#if ENC_DEC_TRACE || RExt__DECODER_DEBUG_BIT_STATISTICS
    static const TChar *syntaxStrings[]={ "pps_range_extension_flag",
                                         "pps_multilayer_extension_flag",
                                         "pps_extension_6bits[0]",
                                         "pps_extension_6bits[1]",
                                         "pps_extension_6bits[2]",
                                         "pps_extension_6bits[3]",
                                         "pps_extension_6bits[4]",
                                         "pps_extension_6bits[5]" };
#endif

    for(Int i=0; i<NUM_PPS_EXTENSION_FLAGS; i++)
    {
      WRITE_FLAG( pps_extension_flags[i]?1:0, syntaxStrings[i] );
    }

    for(Int i=0; i<NUM_PPS_EXTENSION_FLAGS; i++) // loop used so that the order is determined by the enum.
    {
      if (pps_extension_flags[i])
      {
        switch (PPSExtensionFlagIndex(i))
        {
          case PPS_EXT__REXT:
            {
              const TComPPSRExt &ppsRangeExtension = pcPPS->getPpsRangeExtension();
            if (pcPPS->getUseTransformSkip())
            {
                WRITE_UVLC( ppsRangeExtension.getLog2MaxTransformSkipBlockSize()-2,            "log2_max_transform_skip_block_size_minus2");
            }

              WRITE_FLAG((ppsRangeExtension.getCrossComponentPredictionEnabledFlag() ? 1 : 0), "cross_component_prediction_enabled_flag" );

              WRITE_FLAG(UInt(ppsRangeExtension.getChromaQpOffsetListEnabledFlag()),           "chroma_qp_offset_list_enabled_flag" );
              if (ppsRangeExtension.getChromaQpOffsetListEnabledFlag())
            {
                WRITE_UVLC(ppsRangeExtension.getDiffCuChromaQpOffsetDepth(),                   "diff_cu_chroma_qp_offset_depth");
                WRITE_UVLC(ppsRangeExtension.getChromaQpOffsetListLen() - 1,                   "chroma_qp_offset_list_len_minus1");
              /* skip zero index */
                for (Int cuChromaQpOffsetIdx = 0; cuChromaQpOffsetIdx < ppsRangeExtension.getChromaQpOffsetListLen(); cuChromaQpOffsetIdx++)
              {
                  WRITE_SVLC(ppsRangeExtension.getChromaQpOffsetListEntry(cuChromaQpOffsetIdx+1).u.comp.CbOffset,     "cb_qp_offset_list[i]");
                  WRITE_SVLC(ppsRangeExtension.getChromaQpOffsetListEntry(cuChromaQpOffsetIdx+1).u.comp.CrOffset,     "cr_qp_offset_list[i]");
              }
            }

              WRITE_UVLC( ppsRangeExtension.getLog2SaoOffsetScale(CHANNEL_TYPE_LUMA),           "log2_sao_offset_scale_luma"   );
              WRITE_UVLC( ppsRangeExtension.getLog2SaoOffsetScale(CHANNEL_TYPE_CHROMA),         "log2_sao_offset_scale_chroma" );
            }
            break;
          default:
            assert(pps_extension_flags[i]==false); // Should never get here with an active PPS extension flag.
            break;
        } // switch
      } // if flag present
    } // loop over PPS flags
  } // pps_extension_present_flag is non-zero
#else
    WRITE_FLAG( 1, "pps_extension_present_flag" );

    WRITE_FLAG( pcPPS->getPpsRangeExtensionsFlag( ) ? 1 : 0 , "pps_range_extensions_flag" );
    WRITE_FLAG( pcPPS->getPpsMultilayerExtensionFlag( ) ? 1 : 0 , "pps_multilayer_extension_flag" );
    WRITE_FLAG( pcPPS->getPps3dExtensionFlag( ) ? 1 : 0 , "pps_3d_extension_flag" );
    WRITE_CODE( pcPPS->getPpsExtension5bits( ), 5, "pps_extension_5bits" );
    if ( pcPPS->getPpsRangeExtensionsFlag() )
    { 
              const TComPPSRExt &ppsRangeExtension = pcPPS->getPpsRangeExtension();
              if (pcPPS->getUseTransformSkip())
              {
                WRITE_UVLC( ppsRangeExtension.getLog2MaxTransformSkipBlockSize()-2,            "log2_max_transform_skip_block_size_minus2");
              }

              WRITE_FLAG((ppsRangeExtension.getCrossComponentPredictionEnabledFlag() ? 1 : 0), "cross_component_prediction_enabled_flag" );

              WRITE_FLAG(UInt(ppsRangeExtension.getChromaQpOffsetListEnabledFlag()),           "chroma_qp_offset_list_enabled_flag" );
              if (ppsRangeExtension.getChromaQpOffsetListEnabledFlag())
              {
                WRITE_UVLC(ppsRangeExtension.getDiffCuChromaQpOffsetDepth(),                   "diff_cu_chroma_qp_offset_depth");
                WRITE_UVLC(ppsRangeExtension.getChromaQpOffsetListLen() - 1,                   "chroma_qp_offset_list_len_minus1");
                /* skip zero index */
                for (Int cuChromaQpOffsetIdx = 0; cuChromaQpOffsetIdx < ppsRangeExtension.getChromaQpOffsetListLen(); cuChromaQpOffsetIdx++)
                {
                  WRITE_SVLC(ppsRangeExtension.getChromaQpOffsetListEntry(cuChromaQpOffsetIdx+1).u.comp.CbOffset,     "cb_qp_offset_list[i]");
                  WRITE_SVLC(ppsRangeExtension.getChromaQpOffsetListEntry(cuChromaQpOffsetIdx+1).u.comp.CrOffset,     "cr_qp_offset_list[i]");
                }
              }

              WRITE_UVLC( ppsRangeExtension.getLog2SaoOffsetScale(CHANNEL_TYPE_LUMA),           "log2_sao_offset_scale_luma"   );
              WRITE_UVLC( ppsRangeExtension.getLog2SaoOffsetScale(CHANNEL_TYPE_CHROMA),         "log2_sao_offset_scale_chroma" );
    }

    if ( pcPPS->getPpsMultilayerExtensionFlag() )
    { 
      codePpsMultilayerExtension( pcPPS );
    }

#if NH_3D
    if( pcPPS->getPps3dExtensionFlag( )  ) // This probably needs to be aligned with Rext and SHVC
    {
      codePps3dExtension( pcPPS ); 
    }
#endif
#endif
  xWriteRbspTrailingBits();
}

#if NH_3D
Void  TEncCavlc::codePps3dExtension        ( const TComPPS* pcPPS )
{
#if NH_3D_DLT
  WRITE_FLAG( pcPPS->getDLT()->getDltPresentFlag() ? 1 : 0, "dlts_present_flag" );

  if ( pcPPS->getDLT()->getDltPresentFlag() )
  {
    WRITE_CODE(pcPPS->getDLT()->getNumDepthViews() - 1, 6, "pps_depth_layers_minus1");
    WRITE_CODE((pcPPS->getDLT()->getDepthViewBitDepth() - 8), 4, "pps_bit_depth_for_depth_layers_minus8");
    
    for( Int i = 0; i <= pcPPS->getDLT()->getNumDepthViews()-1; i++ )
    {
      Int layerId = pcPPS->getDLT()->getDepthIdxToLayerId(i);
      
      WRITE_FLAG( pcPPS->getDLT()->getUseDLTFlag( layerId ) ? 1 : 0, "dlt_flag[i]" );
      
      if ( pcPPS->getDLT()->getUseDLTFlag( layerId ) )
      {
        std::vector<Int> aiIdx2DepthValue_coded(256, 0);
        UInt uiNumDepthValues_coded = pcPPS->getDLT()->getNumDepthValues(layerId);
        
        // ----------------------------- Actual coding -----------------------------
        WRITE_FLAG( pcPPS->getDLT()->getInterViewDltPredEnableFlag( layerId ) ? 1 : 0, "dlt_pred_flag[i]");
        if ( pcPPS->getDLT()->getInterViewDltPredEnableFlag( layerId ) == false )
        {
          WRITE_FLAG( pcPPS->getDLT()->getUseBitmapRep( layerId ) ? 1 : 0, "dlt_val_flags_present_flag[i]" );
          
          for( UInt ui = 0; ui<uiNumDepthValues_coded; ui++ )
          {
            aiIdx2DepthValue_coded[ui] = pcPPS->getDLT()->idx2DepthValue(layerId, ui);
          }
        }
        else
        {
          AOF( layerId > 1 );
          // assumes ref layer id to be 1
          std::vector<Int> viRefDLT = pcPPS->getDLT()->idx2DepthValue( 1 );
          UInt uiRefNum = pcPPS->getDLT()->getNumDepthValues( 1 );
          pcPPS->getDLT()->getDeltaDLT(layerId, viRefDLT, uiRefNum, aiIdx2DepthValue_coded, uiNumDepthValues_coded);
        }
        
        // bit map coding
        if ( pcPPS->getDLT()->getUseBitmapRep( layerId ) )
        {
          UInt uiDltArrayIndex = 0;
          for (UInt d=0; d < 256; d++)
          {
            if ( d == aiIdx2DepthValue_coded[uiDltArrayIndex] )
            {
              WRITE_FLAG(1, "dlt_value_flag[i][j]");
              uiDltArrayIndex++;
            }
            else
            {
              WRITE_FLAG(0, "dlt_value_flag[i][j]");
            }
          }
        }
        // Diff Coding
        else
        {
          UInt uiMaxDiff               = 0;
          UInt uiMinDiff               = MAX_INT;
          UInt uiLengthMinDiff         = 0;
          UInt uiLengthDltDiffMinusMin = 0;
          
          std::vector<UInt> puiDltDiffValues(uiNumDepthValues_coded, 0);
          
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
          
          if ( uiNumDepthValues_coded > 2 )
          {
            uiLengthMinDiff    = (UInt) gCeilLog2(uiMaxDiff + 1);
          }
          if (uiMaxDiff > uiMinDiff)
          {
            uiLengthDltDiffMinusMin = (UInt) gCeilLog2(uiMaxDiff - uiMinDiff + 1);
          }
          
          WRITE_CODE(uiNumDepthValues_coded, 8, "num_val_delta_dlt");    // num_entry
          {
            // The condition if( uiNumDepthValues_coded > 0 ) is always true since for Single-view Diff Coding, there is at least one depth value in depth component.
            if ( uiNumDepthValues_coded > 1 )
            {
              WRITE_CODE(uiMaxDiff, 8, "max_diff");        // max_diff
            }
            
            if ( uiNumDepthValues_coded > 2 )
            {
              WRITE_CODE((uiMinDiff - 1), uiLengthMinDiff, "min_diff_minus1");     // min_diff_minus1
            }
            
            WRITE_CODE(aiIdx2DepthValue_coded[0], 8, "delta_dlt_val0");          // entry0
            
            if (uiMaxDiff > uiMinDiff)
            {
              for (UInt d=1; d < uiNumDepthValues_coded; d++)
              {
                WRITE_CODE( (puiDltDiffValues[d] - uiMinDiff), uiLengthDltDiffMinusMin, "delta_val_diff_minus_min[k]");    // entry_value_diff_minus_min[ k ]
              }
            }
          }
          
        }
      }
    }
  }
#endif
}
#endif

Void TEncCavlc::codeVUI( const TComVUI *pcVUI, const TComSPS* pcSPS )
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
#if NH_MV
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
      WRITE_CODE(pcVUI->getMatrixCoefficients(), 8,             "matrix_coeffs");
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
    WRITE_UVLC(defaultDisplayWindow.getWindowLeftOffset()  / TComSPS::getWinUnitX(pcSPS->getChromaFormatIdc()), "def_disp_win_left_offset");
    WRITE_UVLC(defaultDisplayWindow.getWindowRightOffset() / TComSPS::getWinUnitX(pcSPS->getChromaFormatIdc()), "def_disp_win_right_offset");
    WRITE_UVLC(defaultDisplayWindow.getWindowTopOffset()   / TComSPS::getWinUnitY(pcSPS->getChromaFormatIdc()), "def_disp_win_top_offset");
    WRITE_UVLC(defaultDisplayWindow.getWindowBottomOffset()/ TComSPS::getWinUnitY(pcSPS->getChromaFormatIdc()), "def_disp_win_bottom_offset");
  }
  const TimingInfo *timingInfo = pcVUI->getTimingInfo();
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
    WRITE_FLAG(pcVUI->getHrdParametersPresentFlag(),              "vui_hrd_parameters_present_flag");
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
    WRITE_UVLC(pcVUI->getMaxBitsPerMinCuDenom(),                "max_bits_per_min_cu_denom");
    WRITE_UVLC(pcVUI->getLog2MaxMvLengthHorizontal(),           "log2_max_mv_length_horizontal");
    WRITE_UVLC(pcVUI->getLog2MaxMvLengthVertical(),             "log2_max_mv_length_vertical");
  }
}

Void TEncCavlc::codeHrdParameters( const TComHRD *hrd, Bool commonInfPresentFlag, UInt maxNumSubLayersMinus1 )
{
  if( commonInfPresentFlag )
  {
    WRITE_FLAG( hrd->getNalHrdParametersPresentFlag() ? 1 : 0 ,  "nal_hrd_parameters_present_flag" );
    WRITE_FLAG( hrd->getVclHrdParametersPresentFlag() ? 1 : 0 ,  "vcl_hrd_parameters_present_flag" );
    if( hrd->getNalHrdParametersPresentFlag() || hrd->getVclHrdParametersPresentFlag() )
    {
      WRITE_FLAG( hrd->getSubPicCpbParamsPresentFlag() ? 1 : 0,  "sub_pic_hrd_params_present_flag" );
      if( hrd->getSubPicCpbParamsPresentFlag() )
      {
        WRITE_CODE( hrd->getTickDivisorMinus2(), 8,              "tick_divisor_minus2" );
        WRITE_CODE( hrd->getDuCpbRemovalDelayLengthMinus1(), 5,  "du_cpb_removal_delay_increment_length_minus1" );
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
    Bool fixedPixRateWithinCvsFlag = true;
    if( !hrd->getFixedPicRateFlag( i ) )
    {
      fixedPixRateWithinCvsFlag = hrd->getFixedPicRateWithinCvsFlag( i );
      WRITE_FLAG( hrd->getFixedPicRateWithinCvsFlag( i ) ? 1 : 0, "fixed_pic_rate_within_cvs_flag");
    }
    if( fixedPixRateWithinCvsFlag )
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

Void TEncCavlc::codeSPS( const TComSPS* pcSPS )
{

  const ChromaFormat format                = pcSPS->getChromaFormatIdc();
  const Bool         chromaEnabled         = isChromaEnabled(format);

#if ENC_DEC_TRACE
#if H_MV_ENC_DEC_TRAC
  tracePSHeader( "SPS", pcSPS->getLayerId() ); 
#else
  xTraceSPSHeader ();
#endif
#endif
  WRITE_CODE( pcSPS->getVPSId (),          4,       "sps_video_parameter_set_id" );
#if NH_MV
  if ( pcSPS->getLayerId() == 0 )
  {
#endif
  WRITE_CODE( pcSPS->getMaxTLayers() - 1,  3,       "sps_max_sub_layers_minus1" );
#if NH_MV
  }
  else
  {
    WRITE_CODE( pcSPS->getSpsExtOrMaxSubLayersMinus1( ), 3, "sps_ext_or_max_sub_layers_minus1" );
  }
  if ( !pcSPS->getMultiLayerExtSpsFlag() )
  {
#endif
  WRITE_FLAG( pcSPS->getTemporalIdNestingFlag() ? 1 : 0, "sps_temporal_id_nesting_flag" );
  codePTL(pcSPS->getPTL(), 1, pcSPS->getMaxTLayers() - 1);
#if NH_MV
}
#endif
  WRITE_UVLC( pcSPS->getSPSId (),                   "sps_seq_parameter_set_id" );
#if NH_MV
    if ( pcSPS->getMultiLayerExtSpsFlag() )
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
  WRITE_UVLC( Int(pcSPS->getChromaFormatIdc ()),    "chroma_format_idc" );
  if( format == CHROMA_444 )
  {
    WRITE_FLAG( 0,                                  "separate_colour_plane_flag");
  }

  WRITE_UVLC( pcSPS->getPicWidthInLumaSamples (),   "pic_width_in_luma_samples" );
  WRITE_UVLC( pcSPS->getPicHeightInLumaSamples(),   "pic_height_in_luma_samples" );
  Window conf = pcSPS->getConformanceWindow();

  WRITE_FLAG( conf.getWindowEnabledFlag(),          "conformance_window_flag" );
  if (conf.getWindowEnabledFlag())
  {
    WRITE_UVLC( conf.getWindowLeftOffset()   / TComSPS::getWinUnitX(pcSPS->getChromaFormatIdc() ), "conf_win_left_offset" );
    WRITE_UVLC( conf.getWindowRightOffset()  / TComSPS::getWinUnitX(pcSPS->getChromaFormatIdc() ), "conf_win_right_offset" );
    WRITE_UVLC( conf.getWindowTopOffset()    / TComSPS::getWinUnitY(pcSPS->getChromaFormatIdc() ), "conf_win_top_offset" );
    WRITE_UVLC( conf.getWindowBottomOffset() / TComSPS::getWinUnitY(pcSPS->getChromaFormatIdc() ), "conf_win_bottom_offset" );
  }
#if NH_MV
}
#endif
#if NH_MV
  if ( !pcSPS->getMultiLayerExtSpsFlag() )
  { 
#endif

  WRITE_UVLC( pcSPS->getBitDepth(CHANNEL_TYPE_LUMA) - 8,                      "bit_depth_luma_minus8" );

  WRITE_UVLC( chromaEnabled ? (pcSPS->getBitDepth(CHANNEL_TYPE_CHROMA) - 8):0,  "bit_depth_chroma_minus8" );
#if NH_MV
  }
#endif
  WRITE_UVLC( pcSPS->getBitsForPOC()-4,                 "log2_max_pic_order_cnt_lsb_minus4" );
#if NH_MV
  if ( !pcSPS->getMultiLayerExtSpsFlag()) 
  {  
#endif

  const Bool subLayerOrderingInfoPresentFlag = 1;
  WRITE_FLAG(subLayerOrderingInfoPresentFlag,       "sps_sub_layer_ordering_info_present_flag");
  for(UInt i=0; i <= pcSPS->getMaxTLayers()-1; i++)
  {
    WRITE_UVLC( pcSPS->getMaxDecPicBuffering(i) - 1,       "sps_max_dec_pic_buffering_minus1[i]" );
    WRITE_UVLC( pcSPS->getNumReorderPics(i),               "sps_max_num_reorder_pics[i]" );
#if NH_MV
    WRITE_UVLC( pcSPS->getSpsMaxLatencyIncreasePlus1(i),   "sps_max_latency_increase_plus1[i]" );
#else
    WRITE_UVLC( pcSPS->getMaxLatencyIncreasePlus1(i),      "sps_max_latency_increase_plus1[i]" );
#endif
    if (!subLayerOrderingInfoPresentFlag)
    {
      break;
    }
  }
#if NH_MV
  }
#endif

  assert( pcSPS->getMaxCUWidth() == pcSPS->getMaxCUHeight() );
  WRITE_UVLC( pcSPS->getLog2MinCodingBlockSize() - 3,                                "log2_min_luma_coding_block_size_minus3" );
  WRITE_UVLC( pcSPS->getLog2DiffMaxMinCodingBlockSize(),                             "log2_diff_max_min_luma_coding_block_size" );
  WRITE_UVLC( pcSPS->getQuadtreeTULog2MinSize() - 2,                                 "log2_min_luma_transform_block_size_minus2" );
  WRITE_UVLC( pcSPS->getQuadtreeTULog2MaxSize() - pcSPS->getQuadtreeTULog2MinSize(), "log2_diff_max_min_luma_transform_block_size" );
  WRITE_UVLC( pcSPS->getQuadtreeTUMaxDepthInter() - 1,                               "max_transform_hierarchy_depth_inter" );
  WRITE_UVLC( pcSPS->getQuadtreeTUMaxDepthIntra() - 1,                               "max_transform_hierarchy_depth_intra" );
  WRITE_FLAG( pcSPS->getScalingListFlag() ? 1 : 0,                                   "scaling_list_enabled_flag" );
  if(pcSPS->getScalingListFlag())
  {
#if NH_MV
    if ( pcSPS->getMultiLayerExtSpsFlag() )
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
      codeScalingList( pcSPS->getScalingList() );
    }
#if NH_MV
    }
#endif
  }
  WRITE_FLAG( pcSPS->getUseAMP() ? 1 : 0,                                            "amp_enabled_flag" );
  WRITE_FLAG( pcSPS->getUseSAO() ? 1 : 0,                                            "sample_adaptive_offset_enabled_flag");

  WRITE_FLAG( pcSPS->getUsePCM() ? 1 : 0,                                            "pcm_enabled_flag");
  if( pcSPS->getUsePCM() )
  {
    WRITE_CODE( pcSPS->getPCMBitDepth(CHANNEL_TYPE_LUMA) - 1, 4,                            "pcm_sample_bit_depth_luma_minus1" );
    WRITE_CODE( chromaEnabled ? (pcSPS->getPCMBitDepth(CHANNEL_TYPE_CHROMA) - 1) : 0, 4,    "pcm_sample_bit_depth_chroma_minus1" );
    WRITE_UVLC( pcSPS->getPCMLog2MinSize() - 3,                                      "log2_min_pcm_luma_coding_block_size_minus3" );
    WRITE_UVLC( pcSPS->getPCMLog2MaxSize() - pcSPS->getPCMLog2MinSize(),             "log2_diff_max_min_pcm_luma_coding_block_size" );
    WRITE_FLAG( pcSPS->getPCMFilterDisableFlag()?1 : 0,                              "pcm_loop_filter_disable_flag");
  }

  assert( pcSPS->getMaxTLayers() > 0 );

  const TComRPSList* rpsList = pcSPS->getRPSList();

  WRITE_UVLC(rpsList->getNumberOfReferencePictureSets(), "num_short_term_ref_pic_sets" );
  for(Int i=0; i < rpsList->getNumberOfReferencePictureSets(); i++)
  {
    const TComReferencePictureSet*rps = rpsList->getReferencePictureSet(i);
    codeShortTermRefPicSet( rps,false, i);
  }
#if NH_MV
  WRITE_FLAG( pcSPS->getLongTermRefPicsPresentFlag() ? 1 : 0,         "long_term_ref_pics_present_flag" );
  if (pcSPS->getLongTermRefPicsPresentFlag())
#else
  WRITE_FLAG( pcSPS->getLongTermRefsPresent() ? 1 : 0,         "long_term_ref_pics_present_flag" );
  if (pcSPS->getLongTermRefsPresent())
#endif
  {
    WRITE_UVLC(pcSPS->getNumLongTermRefPicSPS(), "num_long_term_ref_pics_sps" );
    for (UInt k = 0; k < pcSPS->getNumLongTermRefPicSPS(); k++)
    {
      WRITE_CODE( pcSPS->getLtRefPicPocLsbSps(k), pcSPS->getBitsForPOC(), "lt_ref_pic_poc_lsb_sps");
      WRITE_FLAG( pcSPS->getUsedByCurrPicLtSPSFlag(k), "used_by_curr_pic_lt_sps_flag[i]");
    }
  }
  WRITE_FLAG( pcSPS->getTMVPFlagsPresent()  ? 1 : 0,           "sps_temporal_mvp_enable_flag" );

  WRITE_FLAG( pcSPS->getUseStrongIntraSmoothing(),             "sps_strong_intra_smoothing_enable_flag" );

  WRITE_FLAG( pcSPS->getVuiParametersPresentFlag(),             "vui_parameters_present_flag" );
  if (pcSPS->getVuiParametersPresentFlag())
  {
      codeVUI(pcSPS->getVuiParameters(), pcSPS);
  }

#if !NH_MV
  Bool sps_extension_present_flag=false;
  Bool sps_extension_flags[NUM_SPS_EXTENSION_FLAGS]={false};

  sps_extension_flags[SPS_EXT__REXT] = pcSPS->getSpsRangeExtension().settingsDifferFromDefaults();

  // Other SPS extension flags checked here.

  for(Int i=0; i<NUM_SPS_EXTENSION_FLAGS; i++)
  {
    sps_extension_present_flag|=sps_extension_flags[i];
  }

  WRITE_FLAG( (sps_extension_present_flag?1:0), "sps_extension_present_flag" );

  if (sps_extension_present_flag)
  {
#if ENC_DEC_TRACE || RExt__DECODER_DEBUG_BIT_STATISTICS
    static const TChar *syntaxStrings[]={ "sps_range_extension_flag",
      "sps_multilayer_extension_flag",
      "sps_extension_6bits[0]",
      "sps_extension_6bits[1]",
      "sps_extension_6bits[2]",
      "sps_extension_6bits[3]",
      "sps_extension_6bits[4]",
      "sps_extension_6bits[5]" };
#endif

    for(Int i=0; i<NUM_SPS_EXTENSION_FLAGS; i++)
    {
      WRITE_FLAG( sps_extension_flags[i]?1:0, syntaxStrings[i] );
    }

    for(Int i=0; i<NUM_SPS_EXTENSION_FLAGS; i++) // loop used so that the order is determined by the enum.
    {
      if (sps_extension_flags[i])
      {
        switch (SPSExtensionFlagIndex(i))
        {
        case SPS_EXT__REXT:
          {
            const TComSPSRExt &spsRangeExtension=pcSPS->getSpsRangeExtension();

            WRITE_FLAG( (spsRangeExtension.getTransformSkipRotationEnabledFlag() ? 1 : 0),      "transform_skip_rotation_enabled_flag");
            WRITE_FLAG( (spsRangeExtension.getTransformSkipContextEnabledFlag() ? 1 : 0),       "transform_skip_context_enabled_flag");
            WRITE_FLAG( (spsRangeExtension.getRdpcmEnabledFlag(RDPCM_SIGNAL_IMPLICIT) ? 1 : 0), "implicit_rdpcm_enabled_flag" );
            WRITE_FLAG( (spsRangeExtension.getRdpcmEnabledFlag(RDPCM_SIGNAL_EXPLICIT) ? 1 : 0), "explicit_rdpcm_enabled_flag" );
            WRITE_FLAG( (spsRangeExtension.getExtendedPrecisionProcessingFlag() ? 1 : 0),       "extended_precision_processing_flag" );
            WRITE_FLAG( (spsRangeExtension.getIntraSmoothingDisabledFlag() ? 1 : 0),            "intra_smoothing_disabled_flag" );
            WRITE_FLAG( (spsRangeExtension.getHighPrecisionOffsetsEnabledFlag() ? 1 : 0),       "high_precision_offsets_enabled_flag" );
            WRITE_FLAG( (spsRangeExtension.getPersistentRiceAdaptationEnabledFlag() ? 1 : 0),   "persistent_rice_adaptation_enabled_flag" );
            WRITE_FLAG( (spsRangeExtension.getCabacBypassAlignmentEnabledFlag() ? 1 : 0),       "cabac_bypass_alignment_enabled_flag" );
          break;
          }
        default:
          assert(sps_extension_flags[i]==false); // Should never get here with an active SPS extension flag.
          break;
        }
      }
    }
  }
#else
  WRITE_FLAG( pcSPS->getSpsExtensionPresentFlag(), "sps_extension_present_flag" );

  if ( pcSPS->getSpsExtensionPresentFlag() )
  {
    WRITE_FLAG( pcSPS->getSpsRangeExtensionsFlag( )     ? 1 : 0 , "sps_range_extensions_flag" );
    WRITE_FLAG( pcSPS->getSpsMultilayerExtensionFlag( ) ? 1 : 0 , "sps_multilayer_extension_flag" );
    WRITE_FLAG( pcSPS->getSps3dExtensionFlag( )         ? 1 : 0 , "sps_3d_extension_flag" );
    WRITE_CODE( pcSPS->getSpsExtension5bits( )              , 5 , "sps_extension_5bits" );
  }

  if ( pcSPS->getSpsRangeExtensionsFlag() )
  {
    const TComSPSRExt &spsRangeExtension=pcSPS->getSpsRangeExtension();

    WRITE_FLAG( (spsRangeExtension.getTransformSkipRotationEnabledFlag() ? 1 : 0),      "transform_skip_rotation_enabled_flag");
    WRITE_FLAG( (spsRangeExtension.getTransformSkipContextEnabledFlag() ? 1 : 0),       "transform_skip_context_enabled_flag");
    WRITE_FLAG( (spsRangeExtension.getRdpcmEnabledFlag(RDPCM_SIGNAL_IMPLICIT) ? 1 : 0), "implicit_rdpcm_enabled_flag" );
    WRITE_FLAG( (spsRangeExtension.getRdpcmEnabledFlag(RDPCM_SIGNAL_EXPLICIT) ? 1 : 0), "explicit_rdpcm_enabled_flag" );
    WRITE_FLAG( (spsRangeExtension.getExtendedPrecisionProcessingFlag() ? 1 : 0),       "extended_precision_processing_flag" );
    WRITE_FLAG( (spsRangeExtension.getIntraSmoothingDisabledFlag() ? 1 : 0),            "intra_smoothing_disabled_flag" );
    WRITE_FLAG( (spsRangeExtension.getHighPrecisionOffsetsEnabledFlag() ? 1 : 0),       "high_precision_offsets_enabled_flag" );
    WRITE_FLAG( (spsRangeExtension.getPersistentRiceAdaptationEnabledFlag() ? 1 : 0),   "persistent_rice_adaptation_enabled_flag" );
    WRITE_FLAG( (spsRangeExtension.getCabacBypassAlignmentEnabledFlag() ? 1 : 0),       "cabac_bypass_alignment_enabled_flag" );
  }

  if ( pcSPS->getSpsMultilayerExtensionFlag() )
  {
    codeSPSExtension( pcSPS ); 
  }

#if NH_3D
  if ( pcSPS->getSps3dExtensionFlag() )
  {
    codeSPS3dExtension( pcSPS ); 
  }

#endif
#endif
  xWriteRbspTrailingBits();
}

#if NH_MV
Void TEncCavlc::codeSPSExtension( const TComSPS* pcSPS )
{
  WRITE_FLAG( pcSPS->getInterViewMvVertConstraintFlag() ? 1 : 0, "inter_view_mv_vert_constraint_flag" );
}


Void TEncCavlc::codePpsMultilayerExtension(const TComPPS* pcPPS)
{
  WRITE_FLAG( pcPPS->getPocResetInfoPresentFlag( ) ? 1 : 0 , "poc_reset_info_present_flag" );
  WRITE_FLAG( pcPPS->getPpsInferScalingListFlag( ) ? 1 : 0 , "pps_infer_scaling_list_flag" );
  if (pcPPS->getPpsInferScalingListFlag())
  {
    WRITE_CODE( pcPPS->getPpsScalingListRefLayerId( ), 6, "pps_scaling_list_ref_layer_id" );
  }
  WRITE_UVLC( 0, "num_ref_loc_offsets" );
  WRITE_FLAG( 0 , "colour_mapping_enabled_flag" );

}

#endif

#if NH_3D
Void TEncCavlc::codeSPS3dExtension( const TComSPS* pcSPS )
{
  const TComSps3dExtension* sps3dExt = pcSPS->getSps3dExtension();
  for( Int d = 0; d  <=  1; d++ )
  {
    WRITE_FLAG( sps3dExt->getIvMvPredFlag( d ) ? 1 : 0 , "iv_mv_pred_flag" );
    WRITE_FLAG( sps3dExt->getIvMvScalingFlag( d ) ? 1 : 0 , "iv_mv_scaling_flag" );
    if( d  ==  0 )
    {
      WRITE_UVLC( sps3dExt->getLog2SubPbSizeMinus3( d ), "log2_sub_pb_size_minus3" );
      WRITE_FLAG( sps3dExt->getIvResPredFlag( d ) ? 1 : 0 , "iv_res_pred_flag" );
      WRITE_FLAG( sps3dExt->getDepthRefinementFlag( d ) ? 1 : 0 , "depth_refinement_flag" );
      WRITE_FLAG( sps3dExt->getViewSynthesisPredFlag( d ) ? 1 : 0 , "view_synthesis_pred_flag" );
      WRITE_FLAG( sps3dExt->getDepthBasedBlkPartFlag( d ) ? 1 : 0 , "depth_based_blk_part_flag" );
    }
    else 
    {
      WRITE_FLAG( sps3dExt->getMpiFlag( d ) ? 1 : 0 , "mpi_flag" );
      WRITE_UVLC( sps3dExt->getLog2MpiSubPbSizeMinus3( d ), "log2_mpi_sub_pb_size_minus3" );
      WRITE_FLAG( sps3dExt->getIntraContourFlag( d ) ? 1 : 0 , "intra_contour_flag" );
      WRITE_FLAG( sps3dExt->getIntraSdcWedgeFlag( d ) ? 1 : 0 , "intra_sdc_wedge_flag" );
      WRITE_FLAG( sps3dExt->getQtPredFlag( d ) ? 1 : 0 , "qt_pred_flag" );
      WRITE_FLAG( sps3dExt->getInterSdcFlag( d ) ? 1 : 0 , "inter_sdc_flag" );
      WRITE_FLAG( sps3dExt->getDepthIntraSkipFlag( d ) ? 1 : 0 , "intra_skip_flag" );
    }
  }
}
#endif


Void TEncCavlc::codeVPS( const TComVPS* pcVPS )
{
#if ENC_DEC_TRACE
#if H_MV_ENC_DEC_TRAC
  tracePSHeader( "VPS", getEncTop()->getLayerId() ); 
#else
  xTraceVPSHeader();
#endif
#endif
  WRITE_CODE( pcVPS->getVPSId(),                    4,        "vps_video_parameter_set_id" );
#if NH_MV
  WRITE_FLAG( pcVPS->getVpsBaseLayerInternalFlag( ) ? 1 : 0 , "vps_base_layer_internal_flag" );
  WRITE_FLAG( pcVPS->getVpsBaseLayerAvailableFlag( ) ? 1 : 0 , "vps_base_layer_available_flag" );
#else
  WRITE_FLAG(                                       1,        "vps_base_layer_internal_flag" );
  WRITE_FLAG(                                       1,        "vps_base_layer_available_flag" );
#endif
#if NH_MV
  WRITE_CODE( pcVPS->getMaxLayersMinus1(),       6,        "vps_max_layers_minus1" );
#else
  WRITE_CODE( 0,                                    6,        "vps_max_layers_minus1" );
#endif
  WRITE_CODE( pcVPS->getMaxTLayers() - 1,           3,        "vps_max_sub_layers_minus1" );
  WRITE_FLAG( pcVPS->getTemporalNestingFlag(),                "vps_temporal_id_nesting_flag" );
  assert (pcVPS->getMaxTLayers()>1||pcVPS->getTemporalNestingFlag());
  WRITE_CODE( 0xffff,                              16,        "vps_reserved_0xffff_16bits" );
  codePTL( pcVPS->getPTL(), true, pcVPS->getMaxTLayers() - 1 );
  const Bool subLayerOrderingInfoPresentFlag = 1;
  WRITE_FLAG(subLayerOrderingInfoPresentFlag,              "vps_sub_layer_ordering_info_present_flag");
  for(UInt i=0; i <= pcVPS->getMaxTLayers()-1; i++)
  {
    WRITE_UVLC( pcVPS->getMaxDecPicBuffering(i) - 1,       "vps_max_dec_pic_buffering_minus1[i]" );
    WRITE_UVLC( pcVPS->getNumReorderPics(i),               "vps_max_num_reorder_pics[i]" );
    WRITE_UVLC( pcVPS->getMaxLatencyIncrease(i),           "vps_max_latency_increase_plus1[i]" );
    if (!subLayerOrderingInfoPresentFlag)
    {
      break;
    }
  }

  assert( pcVPS->getNumHrdParameters() <= MAX_VPS_NUM_HRD_PARAMETERS );
#if NH_MV
  assert( pcVPS->getVpsMaxLayerId() < MAX_VPS_NUH_LAYER_ID_PLUS1 );
  WRITE_CODE( pcVPS->getVpsMaxLayerId(), 6,                 "vps_max_layer_id" );  
  WRITE_UVLC( pcVPS->getVpsNumLayerSetsMinus1(),  "vps_num_layer_sets_minus1" );
  for( UInt opsIdx = 1; opsIdx <= pcVPS->getVpsNumLayerSetsMinus1(); opsIdx ++ )
  {
    // Operation point set
    for( UInt i = 0; i <= pcVPS->getVpsMaxLayerId(); i ++ )
    {
#else
  assert( pcVPS->getMaxNuhReservedZeroLayerId() < MAX_VPS_NUH_RESERVED_ZERO_LAYER_ID_PLUS1 );
  WRITE_CODE( pcVPS->getMaxNuhReservedZeroLayerId(), 6,     "vps_max_layer_id" );
  WRITE_UVLC( pcVPS->getMaxOpSets() - 1,                    "vps_num_layer_sets_minus1" );
  for( UInt opsIdx = 1; opsIdx <= ( pcVPS->getMaxOpSets() - 1 ); opsIdx ++ )
  {
    // Operation point set
    for( UInt i = 0; i <= pcVPS->getMaxNuhReservedZeroLayerId(); i ++ )
    {
      // Only applicable for version 1
      // pcVPS->setLayerIdIncludedFlag( true, opsIdx, i );
#endif
      WRITE_FLAG( pcVPS->getLayerIdIncludedFlag( opsIdx, i ) ? 1 : 0, "layer_id_included_flag[opsIdx][i]" );
    }
  }
  const TimingInfo *timingInfo = pcVPS->getTimingInfo();
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
    WRITE_UVLC( pcVPS->getNumHrdParameters(),                 "vps_num_hrd_parameters" );

    if( pcVPS->getNumHrdParameters() > 0 )
    {
      for( UInt i = 0; i < pcVPS->getNumHrdParameters(); i ++ )
      {
        // Only applicable for version 1
        WRITE_UVLC( pcVPS->getHrdOpSetIdx( i ),                "hrd_layer_set_idx" );
        if( i > 0 )
        {
          WRITE_FLAG( pcVPS->getCprmsPresentFlag( i ) ? 1 : 0, "cprms_present_flag[i]" );
        }
        codeHrdParameters(pcVPS->getHrdParameters(i), pcVPS->getCprmsPresentFlag( i ), pcVPS->getMaxTLayers() - 1);
      }
    }
  }
#if NH_MV
  WRITE_FLAG( pcVPS->getVpsExtensionFlag(),                     "vps_extension_flag" );
  m_pcBitIf->writeAlignOne();
  codeVPSExtension( pcVPS );                           
#if NH_3D
  WRITE_FLAG( 1,                     "vps_extension2_flag" );  
  WRITE_FLAG( 1,                     "vps_3d_extension_flag" );
  m_pcBitIf->writeAlignOne();      
  codeVPS3dExtension( pcVPS ); 
  WRITE_FLAG( 0,                     "vps_extension3_flag" );
#else
  WRITE_FLAG( 0,                     "vps_extension2_flag" );
#endif
#else
  WRITE_FLAG( 0,                     "vps_extension_flag" );
#endif

  //future extensions here..
  xWriteRbspTrailingBits();
}

#if NH_MV
Void TEncCavlc::codeVPSExtension( const TComVPS *pcVPS ) 
{ 
  if( pcVPS->getMaxLayersMinus1() > 0  &&  pcVPS->getVpsBaseLayerInternalFlag() )
  {
    codePTL( pcVPS->getPTL( 1 ),0, pcVPS->getMaxSubLayersMinus1()  );
  }

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
    assert( pcVPS->getDimensionIdLen( pcVPS->getNumScalabilityTypes( ) - 1 ) == pcVPS->inferLastDimsionIdLenMinus1() + 1 );
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

  if ( pcVPS->getNumIndependentLayers() > 1 ) 
  {
    WRITE_UVLC( pcVPS->getNumAddLayerSets( ), "num_add_layer_sets" );
  }
  for (Int i = 0; i < pcVPS->getNumAddLayerSets(); i++)
  {
    for (Int j = 1; j < pcVPS->getNumIndependentLayers(); j++)
    {
      WRITE_CODE( pcVPS->getHighestLayerIdxPlus1( i, j ), pcVPS->getHighestLayerIdxPlus1Len( j )  , "highest_layer_idx_plus1" );
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

  Int offsetVal =  ( pcVPS->getMaxLayersMinus1() > 0  &&  pcVPS->getVpsBaseLayerInternalFlag() ) ? 2 : 1;   
  for( Int i = pcVPS->getVpsBaseLayerInternalFlag() ? 2 : 1; i <= pcVPS->getVpsNumProfileTierLevelMinus1(); i++ )
  {
    WRITE_FLAG( pcVPS->getVpsProfilePresentFlag( i ) ? 1 : 0, "vps_profile_present_flag[i]" );
    codePTL( pcVPS->getPTL( offsetVal ), pcVPS->getVpsProfilePresentFlag( i ), pcVPS->getMaxTLayers() - 1 );
    offsetVal++;
  }


  if (pcVPS->getNumLayerSets() > 1)
  {
    WRITE_UVLC( pcVPS->getNumAddOlss( ), "num_add_olss" );
    WRITE_CODE( pcVPS->getDefaultOutputLayerIdc( ), 2, "default_output_layer_idc" );
  }

  assert( pcVPS->getOutputLayerFlag(0, 0) == pcVPS->inferOutputLayerFlag( 0, 0 )); 
  assert( pcVPS->getLayerSetIdxForOlsMinus1( 0 ) == -1 ); 


  if (pcVPS->getVpsBaseLayerInternalFlag() )
  {  
    assert( pcVPS->getProfileTierLevelIdx(0,0) == pcVPS->inferProfileTierLevelIdx(0,0) );
  }


  for( Int i = 1; i < pcVPS->getNumOutputLayerSets( ); i++ )
  {
    if( pcVPS->getNumLayerSets() > 2 && i >= pcVPS->getNumLayerSets( ) )    
    {      
      WRITE_CODE( pcVPS->getLayerSetIdxForOlsMinus1( i ), pcVPS->getLayerSetIdxForOlsMinus1Len( i ) ,      "layer_set_idx_for_ols_minus1[i]" );
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
        
    for ( Int j = 0; j < pcVPS->getNumLayersInIdList( pcVPS->olsIdxToLsIdx(i)); j++ )
    {    
      if (pcVPS->getNecessaryLayerFlag( i, j ) && pcVPS->getVpsNumProfileTierLevelMinus1() > 0 )
      {
        WRITE_CODE( pcVPS->getProfileTierLevelIdx( i, j ), pcVPS->getProfileTierLevelIdxLen() ,"profile_tier_level_idx[ i ][ j ]" );   
      }
      if (pcVPS->getNecessaryLayerFlag( i, j ) && pcVPS->getVpsNumProfileTierLevelMinus1() == 0 )
      {
        assert( pcVPS->getProfileTierLevelIdx( i , j ) == pcVPS->inferProfileTierLevelIdx( i, j ) );
      }

    }
    if( pcVPS->getNumOutputLayersInOutputLayerSet( i ) == 1 && pcVPS->getNumDirectRefLayers( pcVPS->getOlsHighestOutputLayerId( i ) ) > 0 )
    {
      WRITE_FLAG( pcVPS->getAltOutputLayerFlag( i ) ? 1 : 0 , "alt_output_layer_flag[ i ]" );
    }
  }

  WRITE_UVLC( pcVPS->getVpsNumRepFormatsMinus1( ), "vps_num_rep_formats_minus1" );

  for (Int i = 0; i <= pcVPS->getVpsNumRepFormatsMinus1(); i++ )
  {    
    const TComRepFormat* curRepFormat = pcVPS->getRepFormat(i);     
    const TComRepFormat* prevRepFormat = i > 0 ? pcVPS->getRepFormat( i - 1) : NULL; 
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
  WRITE_FLAG( pcVPS->getVpsPocLsbAlignedFlag( ) ? 1 : 0 , "vps_poc_lsb_aligned_flag" );
  for( Int i = 1; i  <=  pcVPS->getMaxLayersMinus1(); i++ )
  {
    if( pcVPS->getNumDirectRefLayers( pcVPS->getLayerIdInNuh( i ) )  ==  0 )
    {      
      WRITE_FLAG( pcVPS->getPocLsbNotPresentFlag( i ) ? 1 : 0 , "poc_lsb_not_present_flag" );
    }
  }

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
  else
  {
    const TComVPSVUI* pcVPSVUI = pcVPS->getVPSVUI( ); 
    assert( pcVPSVUI ); 
    assert( !pcVPSVUI->getCrossLayerIrapAlignedFlag() );     
  }

}

Void TEncCavlc::codeVideoSignalInfo( const TComVideoSignalInfo* pcVideoSignalInfo )
{
  assert( pcVideoSignalInfo ); 
  WRITE_CODE( pcVideoSignalInfo->getVideoVpsFormat( ), 3, "video_vps_format" );
  WRITE_FLAG( pcVideoSignalInfo->getVideoFullRangeVpsFlag( ) ? 1 : 0 , "video_full_range_vps_flag" );
  WRITE_CODE( pcVideoSignalInfo->getColourPrimariesVps( ), 8, "colour_primaries_vps" );
  WRITE_CODE( pcVideoSignalInfo->getTransferCharacteristicsVps( ), 8, "transfer_characteristics_vps" );
  WRITE_CODE( pcVideoSignalInfo->getMatrixCoeffsVps( ), 8, "matrix_coeffs_vps" );
}

Void TEncCavlc::codeDpbSize( const TComVPS* vps )
{ 
  const TComDpbSize * dpbSize = vps->getDpbSize(); 

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
          if ( vps->getNecessaryLayerFlag( i, k ) && ( vps->getVpsBaseLayerInternalFlag() || ( vps->getLayerSetLayerIdList(vps->olsIdxToLsIdx(i),k) != 0 ) ))
          {
            WRITE_UVLC( dpbSize->getMaxVpsDecPicBufferingMinus1( i, k, j ), "max_vps_dec_pic_buffering_minus1" );
          }
          else
          {
            if ( vps->getNecessaryLayerFlag( i, k ) && ( j == 0 ) && ( k == 0 ) ) 
            {
              assert( dpbSize->getMaxVpsDecPicBufferingMinus1(i ,k, j ) ==  0 );
            }
          }
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
            if ( vps->getNecessaryLayerFlag(i, k ) )
            {            
              assert( dpbSize->getMaxVpsDecPicBufferingMinus1( i, k, j ) == dpbSize->getMaxVpsDecPicBufferingMinus1( i,k, j - 1 ) );
            }
          }
          assert( dpbSize->getMaxVpsNumReorderPics      ( i, j ) ==  dpbSize->getMaxVpsNumReorderPics      ( i, j - 1 ) );
          assert( dpbSize->getMaxVpsLatencyIncreasePlus1( i, j ) ==  dpbSize->getMaxVpsLatencyIncreasePlus1( i, j - 1 ) );
        }
      }
    }
  }
}

Void TEncCavlc::codeRepFormat( Int i, const TComRepFormat* pcRepFormat, const TComRepFormat* pcPrevRepFormat )
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
    pcRepFormat->checkInferChromaAndBitDepth(pcPrevRepFormat ); 
  }
  WRITE_FLAG( pcRepFormat->getConformanceWindowVpsFlag( ) ? 1 : 0 , "conformance_window_vps_flag" );
  if ( pcRepFormat->getConformanceWindowVpsFlag() )
  {    
    WRITE_UVLC( pcRepFormat->getConfWinVpsLeftOffset( ), "conf_win_vps_left_offset" );
    WRITE_UVLC( pcRepFormat->getConfWinVpsRightOffset( ), "conf_win_vps_right_offset" );
    WRITE_UVLC( pcRepFormat->getConfWinVpsTopOffset( ), "conf_win_vps_top_offset" );
    WRITE_UVLC( pcRepFormat->getConfWinVpsBottomOffset( ), "conf_win_vps_bottom_offset" );
  }
}

Void TEncCavlc::codeVPSVUI( const TComVPS* pcVPS )
{
  assert( pcVPS ); 

  const TComVPSVUI* pcVPSVUI = pcVPS->getVPSVUI( ); 

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
    for( Int i = pcVPS->getVpsBaseLayerInternalFlag() ? 0 : 1; i < pcVPS->getNumLayerSets(); i++ )
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
    assert( pcVPSVUI->getVpsNumVideoSignalInfoMinus1() == pcVPS->getMaxLayersMinus1() - ( pcVPS->getVpsBaseLayerInternalFlag() ? 0 : 1)  ); 
  }

  for( Int i = 0; i <= pcVPSVUI->getVpsNumVideoSignalInfoMinus1(); i++ )
  {
    assert( pcVPSVUI->getVideoSignalInfo( i ) != NULL ); 
    const TComVideoSignalInfo* curVideoSignalInfo = pcVPSVUI->getVideoSignalInfo( i ); 
    codeVideoSignalInfo( curVideoSignalInfo );     

  }

  if( pcVPSVUI->getVideoSignalInfoIdxPresentFlag() && pcVPSVUI->getVpsNumVideoSignalInfoMinus1() > 0 )
  {
    for( Int i = pcVPS->getVpsBaseLayerInternalFlag() ? 0 : 1; i <=  pcVPS->getMaxLayersMinus1(); i++ )
    {
      WRITE_CODE( pcVPSVUI->getVpsVideoSignalInfoIdx( i ), 4, "vps_video_signal_info_idx" );
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
        Int layerIdx = pcVPS->getLayerIdInVps(pcVPS->getIdRefLayer(pcVPS->getLayerIdInNuh( i ) , j  ));  
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
  WRITE_FLAG( pcVPSVUI->getSingleLayerForNonIrapFlag( ) ? 1 : 0 , "single_layer_for_non_irap_flag" );
  WRITE_FLAG( pcVPSVUI->getHigherLayerIrapSkipFlag( ) ? 1 : 0 , "higher_layer_irap_skip_flag" );
  WRITE_FLAG( pcVPSVUI->getIlpRestrictedRefLayersFlag( ) ? 1 : 0 , "ilp_restricted_ref_layers_flag" );

  if( pcVPSVUI->getIlpRestrictedRefLayersFlag( ) )
  {
    for( Int i = 1; i  <=  pcVPS->getMaxLayersMinus1(); i++ )
    {
      for( Int j = 0; j < pcVPS->getNumDirectRefLayers( pcVPS->getLayerIdInNuh( i ) ); j++ )
      {
        if( pcVPS->getVpsBaseLayerInternalFlag() || pcVPS->getIdRefLayer( pcVPS->getLayerIdInNuh( i ), j ) > 0 )
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

Void TEncCavlc::codeVpsVuiBspHrdParameters( const TComVPS* pcVPS )
{
  assert( pcVPS ); 

  const TComVPSVUI* pcVPSVUI = pcVPS->getVPSVUI( ); 

  assert( pcVPSVUI ); 

  const TComVpsVuiBspHrdParameters*  vpsVuiBspHrdP = pcVPSVUI->getVpsVuiBspHrdParameters(); 

  assert ( vpsVuiBspHrdP );
  
  WRITE_UVLC( vpsVuiBspHrdP->getVpsNumAddHrdParams( ), "vps_num_add_hrd_params" );

  for( Int i = pcVPS->getNumHrdParameters(); i < pcVPS->getNumHrdParameters() + vpsVuiBspHrdP->getVpsNumAddHrdParams(); i++ )
  {  
    if( i > 0 )  
    {
      WRITE_FLAG( vpsVuiBspHrdP->getCprmsAddPresentFlag( i ) ? 1 : 0 , "cprms_add_present_flag" );
    }
    WRITE_UVLC( vpsVuiBspHrdP->getNumSubLayerHrdMinus1( i ), "num_sub_layer_hrd_minus1" );
    const TComHRD* hrdParameters = vpsVuiBspHrdP->getHrdParametermeters( i ); 
    codeHrdParameters( hrdParameters, vpsVuiBspHrdP->getCprmsAddPresentFlag( i ), vpsVuiBspHrdP->getNumSubLayerHrdMinus1( i ) );     
  }

  for( Int h = 1; h < pcVPS->getNumOutputLayerSets(); h++ )
  {  
    WRITE_UVLC( vpsVuiBspHrdP->getNumSignalledPartitioningSchemes( h ), "num_signalled_partitioning_schemes" );

    for( Int j = 1; j < vpsVuiBspHrdP->getNumSignalledPartitioningSchemes( h ) + 1; j++ )
    {  
      WRITE_UVLC( vpsVuiBspHrdP->getNumPartitionsInSchemeMinus1( h, j ), "num_partitions_in_scheme_minus1" );
      for( Int k = 0; k  <=  vpsVuiBspHrdP->getNumPartitionsInSchemeMinus1( h, j ); k++ )  
      {
        for( Int r = 0; r < pcVPS->getNumLayersInIdList(pcVPS->olsIdxToLsIdx( h ) )   ; r++ )  
        {
          WRITE_FLAG( vpsVuiBspHrdP->getLayerIncludedInPartitionFlag( h, j, k, r ) ? 1 : 0 , "layer_included_in_partition_flag" );
        }
      }
    }  
    for( Int i = 0; i < vpsVuiBspHrdP->getNumSignalledPartitioningSchemes( h ) + 1; i++ )  
    {
      for( Int t = 0; t  <=  pcVPS->getMaxSubLayersInLayerSetMinus1( pcVPS->olsIdxToLsIdx( h ) ); t++ )
      {  
        WRITE_UVLC( vpsVuiBspHrdP->getNumBspSchedulesMinus1( h, i, t ), "num_bsp_schedules_minus1" );
        for( Int j = 0; j  <=  vpsVuiBspHrdP->getNumBspSchedulesMinus1( h, i, t ); j++ )  
        {
          for( Int k = 0; k  <=  vpsVuiBspHrdP->getNumPartitionsInSchemeMinus1( h, j ); k++ )
          {  
            WRITE_CODE( vpsVuiBspHrdP->getBspHrdIdx( h, i, t, j, k ), vpsVuiBspHrdP->getBspHrdIdxLen( pcVPS ), "bsp_hrd_idx" );
            WRITE_UVLC( vpsVuiBspHrdP->getBspSchedIdx( h, i, t, j, k ), "bsp_sched_idx" );
          }  
        }
      }  
    }
  }  
}  
#endif

#if NH_3D
Void TEncCavlc::codeVPS3dExtension( const TComVPS* pcVPS )
{ 
  WRITE_UVLC( pcVPS->getCpPrecision( ), "cp_precision" );
  for (Int n = 1; n < pcVPS->getNumViews(); n++)
  {
    Int i      = pcVPS->getViewOIdxList( n );
    Int iInVps = pcVPS->getVoiInVps( i ); 
    WRITE_CODE( pcVPS->getNumCp( iInVps ), 6, "num_cp" );

    if( pcVPS->getNumCp( iInVps ) > 0 )
    {
      WRITE_FLAG( pcVPS->getCpInSliceSegmentHeaderFlag( iInVps ) ? 1 : 0 , "cp_in_slice_segment_header_flag" );
      for( Int m = 0; m < pcVPS->getNumCp( iInVps ); m++ )
      {
        WRITE_UVLC( pcVPS->getCpRefVoi( iInVps, m ), "cp_ref_voi" );
        if( !pcVPS->getCpInSliceSegmentHeaderFlag( iInVps ) )
        {
          Int j      = pcVPS->getCpRefVoi( iInVps, m );
          Int jInVps = pcVPS->getVoiInVps( j ); 
          WRITE_SVLC( pcVPS->getVpsCpScale   ( iInVps, jInVps ), "vps_cp_scale" );
          WRITE_SVLC( pcVPS->getVpsCpOff     ( iInVps, jInVps ), "vps_cp_off" );
          WRITE_SVLC( pcVPS->getVpsCpInvScale( iInVps, jInVps ) + pcVPS->getVpsCpScale( iInVps, jInVps ), "vps_cp_inv_scale_plus_scale" );
          WRITE_SVLC( pcVPS->getVpsCpInvOff  ( iInVps, jInVps ) + pcVPS->getVpsCpOff  ( iInVps, jInVps ), "vps_cp_inv_off_plus_off" );
        }
      }
    }
  }  
}
#endif


Void TEncCavlc::codeSliceHeader         ( TComSlice* pcSlice )
{
#if NH_MV
  const TComVPS* vps = pcSlice->getVPS(); 
#endif

#if ENC_DEC_TRACE
#if NH_MV
  tracePSHeader( "Slice", pcSlice->getLayerId() ); 
#else
  xTraceSliceHeader ();
#endif
#endif

  const ChromaFormat format                = pcSlice->getSPS()->getChromaFormatIdc();
  const UInt         numberValidComponents = getNumberValidComponents(format);
  const Bool         chromaEnabled         = isChromaEnabled(format);

  //calculate number of bits required for slice address
  Int maxSliceSegmentAddress = pcSlice->getPic()->getNumberOfCtusInFrame();
  Int bitsSliceSegmentAddress = 0;
  while(maxSliceSegmentAddress>(1<<bitsSliceSegmentAddress))
  {
    bitsSliceSegmentAddress++;
  }
  const Int ctuTsAddress = pcSlice->getSliceSegmentCurStartCtuTsAddr();

  //write slice address
  const Int sliceSegmentRsAddress = pcSlice->getPic()->getPicSym()->getCtuTsToRsAddrMap(ctuTsAddress);
#if NH_MV
  // This should actually be done somewhere else and not in writing process.
  pcSlice->setFirstSliceSegementInPicFlag( sliceSegmentRsAddress==0 ); 
  WRITE_FLAG( pcSlice->getFirstSliceSegementInPicFlag() , "first_slice_segment_in_pic_flag" );
#else
  WRITE_FLAG( sliceSegmentRsAddress==0, "first_slice_segment_in_pic_flag" );
#endif
  if ( pcSlice->getRapPicFlag() )
  {
    WRITE_FLAG( pcSlice->getNoOutputPriorPicsFlag() ? 1 : 0, "no_output_of_prior_pics_flag" );
  }
#if PPS_FIX_DEPTH
  if( pcSlice->getIsDepth() )
  {
    WRITE_UVLC( 1, "slice_pic_parameter_set_id" );
  }
  else
#endif
  WRITE_UVLC( pcSlice->getPPS()->getPPSId(), "slice_pic_parameter_set_id" );
  if ( pcSlice->getPPS()->getDependentSliceSegmentsEnabledFlag() && (sliceSegmentRsAddress!=0) )
  {
    WRITE_FLAG( pcSlice->getDependentSliceSegmentFlag() ? 1 : 0, "dependent_slice_segment_flag" );
  }
  if(sliceSegmentRsAddress>0)
  {
    WRITE_CODE( sliceSegmentRsAddress, bitsSliceSegmentAddress, "slice_segment_address" );
  }
  if ( !pcSlice->getDependentSliceSegmentFlag() )
  {
#if NH_MV    
    Int esb = 0;  //Don't use i, otherwise will shadow something below

    if ( pcSlice->getPPS()->getNumExtraSliceHeaderBits() > esb )
    {
      esb++; 
      WRITE_FLAG( pcSlice->getDiscardableFlag( ) ? 1 : 0 , "discardable_flag" );
      if (pcSlice->getDiscardableFlag( ))
      {
        assert(pcSlice->getNalUnitType() != NAL_UNIT_CODED_SLICE_TRAIL_R &&
          pcSlice->getNalUnitType() != NAL_UNIT_CODED_SLICE_TSA_R &&
          pcSlice->getNalUnitType() != NAL_UNIT_CODED_SLICE_STSA_R &&
          pcSlice->getNalUnitType() != NAL_UNIT_CODED_SLICE_RADL_R &&
          pcSlice->getNalUnitType() != NAL_UNIT_CODED_SLICE_RASL_R);
      }
    }

    if ( pcSlice->getPPS()->getNumExtraSliceHeaderBits() > esb )
    {
      esb++; 
      WRITE_FLAG( pcSlice->getCrossLayerBlaFlag( ) ? 1 : 0 , "cross_layer_bla_flag" );
    }
    pcSlice->checkCrossLayerBlaFlag( ); 

    for (; esb < pcSlice->getPPS()->getNumExtraSliceHeaderBits(); esb++)    
#else
    for (Int i = 0; i < pcSlice->getPPS()->getNumExtraSliceHeaderBits(); i++)
#endif
    {
      WRITE_FLAG(0, "slice_reserved_flag[]");
    }



    WRITE_UVLC( pcSlice->getSliceType(),       "slice_type" );

    if( pcSlice->getPPS()->getOutputFlagPresentFlag() )
    {
      WRITE_FLAG( pcSlice->getPicOutputFlag() ? 1 : 0, "pic_output_flag" );
    }

#if NH_MV
    if ( (pcSlice->getLayerId() > 0 && !vps->getPocLsbNotPresentFlag( pcSlice->getLayerIdInVps())) || !pcSlice->getIdrPicFlag() )
    {
      Int picOrderCntLSB = (pcSlice->getPOC()-pcSlice->getLastIDR()+(1<<pcSlice->getSPS()->getBitsForPOC())) & ((1<<pcSlice->getSPS()->getBitsForPOC())-1);
      WRITE_CODE( picOrderCntLSB, pcSlice->getSPS()->getBitsForPOC(), "slice_pic_order_cnt_lsb");
      pcSlice->setSlicePicOrderCntLsb( picOrderCntLSB ); 
    }

#endif

    if( !pcSlice->getIdrPicFlag() )
    {
#if !NH_MV
      Int picOrderCntLSB = (pcSlice->getPOC()-pcSlice->getLastIDR()+(1<<pcSlice->getSPS()->getBitsForPOC())) & ((1<<pcSlice->getSPS()->getBitsForPOC())-1);
      WRITE_CODE( picOrderCntLSB, pcSlice->getSPS()->getBitsForPOC(), "slice_pic_order_cnt_lsb");
#endif
      const TComReferencePictureSet* rps = pcSlice->getRPS();

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
        codeShortTermRefPicSet( rps, true, pcSlice->getSPS()->getRPSList()->getNumberOfReferencePictureSets());
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
#if NH_MV
      if(pcSlice->getSPS()->getLongTermRefPicsPresentFlag())
#else
      if(pcSlice->getSPS()->getLongTermRefsPresent())
#endif
      {
        Int numLtrpInSH = rps->getNumberOfLongtermPictures();
        Int ltrpInSPS[MAX_NUM_REF_PICS];
        Int numLtrpInSPS = 0;
        UInt ltrpIndex;
        Int counter = 0;
        // WARNING: The following code only works only if a matching long-term RPS is 
        //          found in the SPS for ALL long-term pictures
        //          The problem is that the SPS coded long-term pictures are moved to the
        //          beginning of the list which causes a mismatch when no reference picture
        //          list reordering is used
        //          NB: Long-term coding is currently not supported in general by the HM encoder
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
        // check that either all long-term pictures are coded in SPS or in slice header (no mixing)
        assert (numLtrpInSH==0 || numLtrpInSPS==0); 

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
        counter = 0;
        // Warning: If some pictures are moved to ltrpInSPS, i is referring to a wrong index 
        //          (mapping would be required)
        for(Int i=rps->getNumberOfPictures()-1 ; i > offset-1; i--, counter++)
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
#if NH_MV
        WRITE_FLAG( pcSlice->getEnableTMVPFlag() ? 1 : 0, "slice_temporal_mvp_enabled_flag" );
#else
        WRITE_FLAG( pcSlice->getEnableTMVPFlag() ? 1 : 0, "slice_temporal_mvp_enabled_flag" );
#endif
      }
    }
#if NH_MV
    Bool interLayerPredLayerIdcPresentFlag = false; 
    Int layerId = pcSlice->getLayerId(); 
#if NH_3D
    if( pcSlice->getLayerId() > 0 && !vps->getAllRefLayersActiveFlag() && vps->getNumRefListLayers( layerId ) > 0 )
#else
    if( pcSlice->getLayerId() > 0 && !vps->getAllRefLayersActiveFlag() && vps->getNumDirectRefLayers( layerId ) > 0 )
#endif
    {   
      WRITE_FLAG( pcSlice->getInterLayerPredEnabledFlag( ) ? 1 : 0 , "inter_layer_pred_enabled_flag" );
#if NH_3D
      if( pcSlice->getInterLayerPredEnabledFlag() && vps->getNumRefListLayers( layerId ) > 1 )
#else
      if( pcSlice->getInterLayerPredEnabledFlag() && vps->getNumDirectRefLayers( layerId ) > 1 )
#endif
      {            
        if( !vps->getMaxOneActiveRefLayerFlag())  
        {
          WRITE_CODE( pcSlice->getNumInterLayerRefPicsMinus1( ), pcSlice->getNumInterLayerRefPicsMinus1Len( ), "num_inter_layer_ref_pics_minus1" );
        }
#if NH_3D
        if ( pcSlice->getNumActiveRefLayerPics() != vps->getNumRefListLayers( layerId ) )
#else
        if ( pcSlice->getNumActiveRefLayerPics() != vps->getNumDirectRefLayers( layerId ) )
#endif
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

#if NH_3D      
  if( getEncTop()->decProcAnnexI() )
  {
      if ( pcSlice->getInCmpPredAvailFlag() )
      {
        WRITE_FLAG( pcSlice->getInCompPredFlag(), "in_comp_pred_flag" );
      }
  }
#endif

    if(pcSlice->getSPS()->getUseSAO())
    {
       WRITE_FLAG( pcSlice->getSaoEnabledFlag(CHANNEL_TYPE_LUMA), "slice_sao_luma_flag" );
       if (chromaEnabled)
       {
         WRITE_FLAG( pcSlice->getSaoEnabledFlag(CHANNEL_TYPE_CHROMA), "slice_sao_chroma_flag" );
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
        SliceType  encCABACTableIdx = pcSlice->getEncCABACTableIdx();
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
#if NH_3D_IC
    else if( pcSlice->getViewIndex() && ( pcSlice->getSliceType() == P_SLICE || pcSlice->getSliceType() == B_SLICE )
      && !pcSlice->getIsDepth() && vps->getNumRefListLayers( layerId ) > 0 
      && getEncTop()->decProcAnnexI()       
      )
    {
      WRITE_FLAG( pcSlice->getApplyIC() ? 1 : 0, "slice_ic_enable_flag" );
      if( pcSlice->getApplyIC() )
      {
        WRITE_FLAG( pcSlice->getIcSkipParseFlag() ? 1 : 0, "slice_ic_disabled_merge_zero_idx_flag" );
      }
    }
#endif
#if NH_3D_IV_MERGE
    assert(pcSlice->getMaxNumMergeCand()<=MRG_MAX_NUM_CANDS_MEM);
#else
    assert(pcSlice->getMaxNumMergeCand()<=MRG_MAX_NUM_CANDS);
#endif
    if (!pcSlice->isIntra())
    {
#if NH_3D_IV_MERGE
      WRITE_UVLC( ( ( pcSlice->getMpiFlag( ) || pcSlice->getIvMvPredFlag( ) || pcSlice->getViewSynthesisPredFlag( ) ) ? MRG_MAX_NUM_CANDS_MEM : MRG_MAX_NUM_CANDS ) - pcSlice->getMaxNumMergeCand(), "five_minus_max_num_merge_cand");
#else
      WRITE_UVLC(MRG_MAX_NUM_CANDS - pcSlice->getMaxNumMergeCand(), "five_minus_max_num_merge_cand");
#endif
    }
    Int iCode = pcSlice->getSliceQp() - ( pcSlice->getPPS()->getPicInitQPMinus26() + 26 );
    WRITE_SVLC( iCode, "slice_qp_delta" );
    if (pcSlice->getPPS()->getSliceChromaQpFlag())
    {
      if (numberValidComponents > COMPONENT_Cb)
      {
        WRITE_SVLC( pcSlice->getSliceChromaQpDelta(COMPONENT_Cb), "slice_cb_qp_offset" );
      }
      if (numberValidComponents > COMPONENT_Cr)
      {
        WRITE_SVLC( pcSlice->getSliceChromaQpDelta(COMPONENT_Cr), "slice_cr_qp_offset" );
      }
      assert(numberValidComponents <= COMPONENT_Cr+1);
    }

    if (pcSlice->getPPS()->getPpsRangeExtension().getChromaQpOffsetListEnabledFlag())
    {
      WRITE_FLAG(pcSlice->getUseChromaQpAdj(), "cu_chroma_qp_offset_enabled_flag");
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

    Bool isSAOEnabled = pcSlice->getSPS()->getUseSAO() && (pcSlice->getSaoEnabledFlag(CHANNEL_TYPE_LUMA) || (chromaEnabled && pcSlice->getSaoEnabledFlag(CHANNEL_TYPE_CHROMA)));
    Bool isDBFEnabled = (!pcSlice->getDeblockingFilterDisable());

      if(pcSlice->getPPS()->getLoopFilterAcrossSlicesEnabledFlag() && ( isSAOEnabled || isDBFEnabled ))
    {
      WRITE_FLAG(pcSlice->getLFCrossSliceBoundaryFlag()?1:0, "slice_loop_filter_across_slices_enabled_flag");
    }
#if NH_3D
    if (getEncTop()->decProcAnnexI() )
    {
      Int voiInVps = vps->getVoiInVps( pcSlice->getViewIndex() ); 
      if( vps->getCpInSliceSegmentHeaderFlag( voiInVps ) )
      {
        for( Int m = 0; m < vps->getNumCp( voiInVps ); m++ )
        {
          Int jInVps = vps->getVoiInVps( vps->getCpRefVoi( voiInVps, m ));
          WRITE_SVLC( pcSlice->getCpScale   ( jInVps )   , "cp_scale" );
          WRITE_SVLC( pcSlice->getCpOff     ( jInVps )   , "cp_off" );
          WRITE_SVLC( pcSlice->getCpInvScale( jInVps ) + pcSlice->getCpScale( jInVps ) , "cp_inv_scale_plus_scale" );
          WRITE_SVLC( pcSlice->getCpInvOff  ( jInVps ) + pcSlice->getCpOff  ( jInVps ) , "cp_inv_off_plus_off" );
        }
      }
    }
#endif

  }
#if !NH_MV
  if(pcSlice->getPPS()->getSliceHeaderExtensionPresentFlag())
  {
    WRITE_UVLC(0,"slice_segment_header_extension_length");
  }
#else
  if(pcSlice->getPPS()->getSliceHeaderExtensionPresentFlag())
  {
    // Derive the value of PocMsbValRequiredFlag

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


    if( !pcSlice->getPocMsbValRequiredFlag() &&  pcSlice->getVPS()->getVpsPocLsbAlignedFlag() )
    {
      shExtnLengthInBit++;    // For poc_msb_val_present_flag
    }
    else
    {
      if( pcSlice->getPocMsbValRequiredFlag() )
      {
        pcSlice->setPocMsbCycleValPresentFlag( true );
      }
      else
      {
        pcSlice->setPocMsbCycleValPresentFlag( false );
      }
    }

    if( pcSlice->getPocMsbCycleValPresentFlag() )
    {
//      Int iMaxPOClsb = 1<< pcSlice->getSPS()->getBitsForPOC(); currently unused

      UInt lengthVal = 1;
      UInt tempVal = pcSlice->getPocMsbCycleVal() + 1;
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

    if( !pcSlice->getPocMsbValRequiredFlag() &&  pcSlice->getVPS()->getVpsPocLsbAlignedFlag()  )
    {
      WRITE_FLAG( pcSlice->getPocMsbCycleValPresentFlag( ) ? 1 : 0 , "poc_msb_cycle_val_present_flag" );
    }
    else
    {
      assert( pcSlice->getPocMsbCycleValPresentFlag() ==  pcSlice->inferPocMsbCycleValPresentFlag( ) ); 
    }
    
    if( pcSlice->getPocMsbCycleValPresentFlag() )
    {
      WRITE_UVLC( pcSlice->getPocMsbCycleVal( ), "poc_msb_cycle_val" );
    }
    
    while( ( m_pcBitIf->getNumberOfWrittenBits() - posFollSliceSegHeaderExtLen ) < pcSlice->getSliceSegmentHeaderExtensionLength() * 8 )
    {
      WRITE_FLAG( 0, "slice_segment_header_extension_data_bit" );
    }

    assert( ( m_pcBitIf->getNumberOfWrittenBits() - posFollSliceSegHeaderExtLen ) == pcSlice->getSliceSegmentHeaderExtensionLength() * 8  ); 
  }
#endif 

}

Void TEncCavlc::codePTL( const TComPTL* pcPTL, Bool profilePresentFlag, Int maxNumSubLayersMinus1)
{
  if(profilePresentFlag)
  {
    codeProfileTier(pcPTL->getGeneralPTL(), false);    // general_...
  }
  WRITE_CODE( Int(pcPTL->getGeneralPTL()->getLevelIdc()), 8, "general_level_idc" );

  for (Int i = 0; i < maxNumSubLayersMinus1; i++)
  {
    WRITE_FLAG( pcPTL->getSubLayerProfilePresentFlag(i), "sub_layer_profile_present_flag[i]" );
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
    if( pcPTL->getSubLayerProfilePresentFlag(i) )
    {
      codeProfileTier(pcPTL->getSubLayerPTL(i), true);  // sub_layer_...
    }
    if( pcPTL->getSubLayerLevelPresentFlag(i) )
    {
      WRITE_CODE( Int(pcPTL->getSubLayerPTL(i)->getLevelIdc()), 8, "sub_layer_level_idc[i]" );
    }
  }
}

#if ENC_DEC_TRACE || RExt__DECODER_DEBUG_BIT_STATISTICS
Void TEncCavlc::codeProfileTier( const ProfileTierLevel* ptl, const Bool bIsSubLayer )
#define PTL_TRACE_TEXT(txt) bIsSubLayer?("sub_layer_" txt) : ("general_" txt)
#else
Void TEncCavlc::codeProfileTier( const ProfileTierLevel* ptl, const Bool /*bIsSubLayer*/ )
#define PTL_TRACE_TEXT(txt) txt
#endif
{
  WRITE_CODE( ptl->getProfileSpace(), 2 ,      PTL_TRACE_TEXT("profile_space"                   ));
  WRITE_FLAG( ptl->getTierFlag()==Level::HIGH, PTL_TRACE_TEXT("tier_flag"                       ));
  WRITE_CODE( Int(ptl->getProfileIdc()), 5 ,   PTL_TRACE_TEXT("profile_idc"                     ));
  for(Int j = 0; j < 32; j++)
  {
    WRITE_FLAG( ptl->getProfileCompatibilityFlag(j), PTL_TRACE_TEXT("profile_compatibility_flag[][j]" ));
  }

  WRITE_FLAG(ptl->getProgressiveSourceFlag(),   PTL_TRACE_TEXT("progressive_source_flag"         ));
  WRITE_FLAG(ptl->getInterlacedSourceFlag(),    PTL_TRACE_TEXT("interlaced_source_flag"          ));
  WRITE_FLAG(ptl->getNonPackedConstraintFlag(), PTL_TRACE_TEXT("non_packed_constraint_flag"      ));
  WRITE_FLAG(ptl->getFrameOnlyConstraintFlag(), PTL_TRACE_TEXT("frame_only_constraint_flag"      ));

#if NH_MV
  if( ptl->getV2ConstraintsPresentFlag() ) 
  {
    WRITE_FLAG( ptl->getMax12bitConstraintFlag( ) ? 1 : 0 , "max_12bit_constraint_flag" );
    WRITE_FLAG( ptl->getMax10bitConstraintFlag( ) ? 1 : 0 , "max_10bit_constraint_flag" );
    WRITE_FLAG( ptl->getMax8bitConstraintFlag( ) ? 1 : 0 , "max_8bit_constraint_flag" );
    WRITE_FLAG( ptl->getMax422chromaConstraintFlag( ) ? 1 : 0 , "max_422chroma_constraint_flag" );
    WRITE_FLAG( ptl->getMax420chromaConstraintFlag( ) ? 1 : 0 , "max_420chroma_constraint_flag" );
    WRITE_FLAG( ptl->getMaxMonochromeConstraintFlag( ) ? 1 : 0 , "max_monochrome_constraint_flag" );
    WRITE_FLAG( ptl->getIntraConstraintFlag( ) ? 1 : 0 , "intra_constraint_flag" );
    WRITE_FLAG( ptl->getOnePictureOnlyConstraintFlag( ) ? 1 : 0 , "one_picture_only_constraint_flag" );
    WRITE_FLAG( ptl->getLowerBitRateConstraintFlag( ) ? 1 : 0 , "lower_bit_rate_constraint_flag" );
    WRITE_CODE( 0, 16, "XXX_reserved_zero_34bits[0..15]");
    WRITE_CODE( 0, 16, "XXX_reserved_zero_34bits[16..31]");
    WRITE_CODE( 0, 2 , "XXX_reserved_zero_34bits[32..33]");
  }
  else
  {
    WRITE_CODE( 0, 16, "XXX_reserved_zero_43bits[0..15]");
    WRITE_CODE( 0, 16, "XXX_reserved_zero_43bits[16..31]");
    WRITE_CODE( 0, 11, "XXX_reserved_zero_43bits[32..42]");
  }
    if( ptl->getInbldPresentFlag() )
  {
    WRITE_FLAG( ptl->getInbldFlag( ) ? 1 : 0 , "inbld_flag" );
  }
  else
  {
    WRITE_FLAG(0, "reserved_zero_bit");
  }
#else
  if (ptl->getProfileIdc() == Profile::MAINREXT || ptl->getProfileIdc() == Profile::HIGHTHROUGHPUTREXT )
  {
    const UInt         bitDepthConstraint=ptl->getBitDepthConstraint();
    WRITE_FLAG(bitDepthConstraint<=12,          PTL_TRACE_TEXT("max_12bit_constraint_flag"       ));
    WRITE_FLAG(bitDepthConstraint<=10,          PTL_TRACE_TEXT("max_10bit_constraint_flag"       ));
    WRITE_FLAG(bitDepthConstraint<= 8,          PTL_TRACE_TEXT("max_8bit_constraint_flag"        ));
    const ChromaFormat chromaFmtConstraint=ptl->getChromaFormatConstraint();
    WRITE_FLAG(chromaFmtConstraint==CHROMA_422||chromaFmtConstraint==CHROMA_420||chromaFmtConstraint==CHROMA_400, PTL_TRACE_TEXT("max_422chroma_constraint_flag" ));
    WRITE_FLAG(chromaFmtConstraint==CHROMA_420||chromaFmtConstraint==CHROMA_400,                                  PTL_TRACE_TEXT("max_420chroma_constraint_flag" ));
    WRITE_FLAG(chromaFmtConstraint==CHROMA_400,                                                                   PTL_TRACE_TEXT("max_monochrome_constraint_flag"));
    WRITE_FLAG(ptl->getIntraConstraintFlag(),        PTL_TRACE_TEXT("intra_constraint_flag"           ));
    WRITE_FLAG(ptl->getOnePictureOnlyConstraintFlag(), PTL_TRACE_TEXT("one_picture_only_constraint_flag"));
    WRITE_FLAG(ptl->getLowerBitRateConstraintFlag(), PTL_TRACE_TEXT("lower_bit_rate_constraint_flag"  ));
    WRITE_CODE(0 , 16, PTL_TRACE_TEXT("reserved_zero_34bits[0..15]"     ));
    WRITE_CODE(0 , 16, PTL_TRACE_TEXT("reserved_zero_34bits[16..31]"    ));
    WRITE_CODE(0 ,  2, PTL_TRACE_TEXT("reserved_zero_34bits[32..33]"    ));
  }
  else
  {
    WRITE_CODE(0x0000 , 16, PTL_TRACE_TEXT("reserved_zero_43bits[0..15]"     ));
    WRITE_CODE(0x0000 , 16, PTL_TRACE_TEXT("reserved_zero_43bits[16..31]"    ));
    WRITE_CODE(0x000  , 11, PTL_TRACE_TEXT("reserved_zero_43bits[32..42]"    ));
  }
  WRITE_FLAG(false,   PTL_TRACE_TEXT("inbld_flag" ));
#undef PTL_TRACE_TEXT
#endif
}

/**
 * Write tiles and wavefront substreams sizes for the slice header (entry points).
 *
 * \param pSlice TComSlice structure that contains the substream size information.
 */
Void  TEncCavlc::codeTilesWPPEntryPoint( TComSlice* pSlice )
{
  if (!pSlice->getPPS()->getTilesEnabledFlag() && !pSlice->getPPS()->getEntropyCodingSyncEnabledFlag())
  {
    return;
  }
  UInt maxOffset = 0;
  for(Int idx=0; idx<pSlice->getNumberOfSubstreamSizes(); idx++)
  {
    UInt offset=pSlice->getSubstreamSize(idx);
    if ( offset > maxOffset )
    {
      maxOffset = offset;
    }
  }

  // Determine number of bits "offsetLenMinus1+1" required for entry point information
  UInt offsetLenMinus1 = 0;
  while (maxOffset >= (1u << (offsetLenMinus1 + 1)))
  {
    offsetLenMinus1++;
    assert(offsetLenMinus1 + 1 < 32);
  }

  WRITE_UVLC(pSlice->getNumberOfSubstreamSizes(), "num_entry_point_offsets");
  if (pSlice->getNumberOfSubstreamSizes()>0)
  {
    WRITE_UVLC(offsetLenMinus1, "offset_len_minus1");

    for (UInt idx=0; idx<pSlice->getNumberOfSubstreamSizes(); idx++)
    {
      WRITE_CODE(pSlice->getSubstreamSize(idx)-1, offsetLenMinus1+1, "entry_point_offset_minus1");
    }
  }
}

Void TEncCavlc::codeTerminatingBit      ( UInt /*uilsLast*/ )
{
}

Void TEncCavlc::codeSliceFinish ()
{
}

Void TEncCavlc::codeMVPIdx ( TComDataCU* /*pcCU*/, UInt /*uiAbsPartIdx*/, RefPicList /*eRefList*/ )
{
  assert(0);
}

Void TEncCavlc::codePartSize( TComDataCU* /*pcCU*/, UInt /*uiAbsPartIdx*/, UInt /*uiDepth*/ )
{
  assert(0);
}

Void TEncCavlc::codePredMode( TComDataCU* /*pcCU*/, UInt /*uiAbsPartIdx*/ )
{
  assert(0);
}

Void TEncCavlc::codeMergeFlag    ( TComDataCU* /*pcCU*/, UInt /*uiAbsPartIdx*/ )
{
  assert(0);
}

Void TEncCavlc::codeMergeIndex    ( TComDataCU* /*pcCU*/, UInt /*uiAbsPartIdx*/ )
{
  assert(0);
}

#if NH_3D_ARP
Void TEncCavlc::codeARPW( TComDataCU* pcCU, UInt uiAbsPartIdx )
{
  assert(0);
}
#endif

#if NH_3D_IC
Void TEncCavlc::codeICFlag( TComDataCU* pcCU, UInt uiAbsPartIdx )
{
  assert(0);
}
#endif

Void TEncCavlc::codeInterModeFlag( TComDataCU* /*pcCU*/, UInt /*uiAbsPartIdx*/, UInt /*uiDepth*/, UInt /*uiEncMode*/ )
{
  assert(0);
}

Void TEncCavlc::codeCUTransquantBypassFlag( TComDataCU* /*pcCU*/, UInt /*uiAbsPartIdx*/ )
{
  assert(0);
}

Void TEncCavlc::codeSkipFlag( TComDataCU* /*pcCU*/, UInt /*uiAbsPartIdx*/ )
{
  assert(0);
}
#if NH_3D_DIS
Void TEncCavlc::codeDIS( TComDataCU* pcCU, UInt uiAbsPartIdx )
{
  assert(0);
}
#endif

Void TEncCavlc::codeSplitFlag   ( TComDataCU* /*pcCU*/, UInt /*uiAbsPartIdx*/, UInt /*uiDepth*/ )
{
  assert(0);
}

Void TEncCavlc::codeTransformSubdivFlag( UInt /*uiSymbol*/, UInt /*uiCtx*/ )
{
  assert(0);
}

Void TEncCavlc::codeQtCbf( TComTU& /*rTu*/, const ComponentID /*compID*/, const Bool /*lowestLevel*/ )
{
  assert(0);
}

Void TEncCavlc::codeQtRootCbf( TComDataCU* /*pcCU*/, UInt /*uiAbsPartIdx*/ )
{
  assert(0);
}

Void TEncCavlc::codeQtCbfZero( TComTU& /*rTu*/, const ChannelType /*chType*/ )
{
  assert(0);
}
Void TEncCavlc::codeQtRootCbfZero( )
{
  assert(0);
}

Void TEncCavlc::codeTransformSkipFlags (TComTU& /*rTu*/, ComponentID /*component*/ )
{
  assert(0);
}

/** Code I_PCM information.
 * \param pcCU pointer to CU
 * \param uiAbsPartIdx CU index
 * \returns Void
 */
Void TEncCavlc::codeIPCMInfo( TComDataCU* /*pcCU*/, UInt /*uiAbsPartIdx*/ )
{
  assert(0);
}

Void TEncCavlc::codeIntraDirLumaAng( TComDataCU* /*pcCU*/, UInt /*uiAbsPartIdx*/, Bool /*isMultiple*/)
{
  assert(0);
}

Void TEncCavlc::codeIntraDirChroma( TComDataCU* /*pcCU*/, UInt /*uiAbsPartIdx*/ )
{
  assert(0);
}

Void TEncCavlc::codeInterDir( TComDataCU* /*pcCU*/, UInt /*uiAbsPartIdx*/ )
{
  assert(0);
}

Void TEncCavlc::codeRefFrmIdx( TComDataCU* /*pcCU*/, UInt /*uiAbsPartIdx*/, RefPicList /*eRefList*/ )
{
  assert(0);
}

Void TEncCavlc::codeMvd( TComDataCU* /*pcCU*/, UInt /*uiAbsPartIdx*/, RefPicList /*eRefList*/ )
{
  assert(0);
}

Void TEncCavlc::codeCrossComponentPrediction( TComTU& /*rTu*/, ComponentID /*compID*/ )
{
  assert(0);
}

Void TEncCavlc::codeDeltaQP( TComDataCU* pcCU, UInt uiAbsPartIdx )
{
  Int iDQp  = pcCU->getQP( uiAbsPartIdx ) - pcCU->getRefQP( uiAbsPartIdx );

  Int qpBdOffsetY =  pcCU->getSlice()->getSPS()->getQpBDOffset(CHANNEL_TYPE_LUMA);
  iDQp = (iDQp + 78 + qpBdOffsetY + (qpBdOffsetY/2)) % (52 + qpBdOffsetY) - 26 - (qpBdOffsetY/2);

  xWriteSvlc( iDQp );

  return;
}

Void TEncCavlc::codeChromaQpAdjustment( TComDataCU* /*pcCU*/, UInt /*uiAbsPartIdx*/ )
{
  assert(0);
}

Void TEncCavlc::codeCoeffNxN    ( TComTU& /*rTu*/, TCoeff* /*pcCoef*/, const ComponentID /*compID*/ )
{
  assert(0);
}

Void TEncCavlc::estBit( estBitsSbacStruct* /*pcEstBitsCabac*/, Int /*width*/, Int /*height*/, ChannelType /*chType*/ )
{
  // printf("error : no VLC mode support in this version\n");
  return;
}

// ====================================================================================================================
// Protected member functions
// ====================================================================================================================

//! Code weighted prediction tables
Void TEncCavlc::xCodePredWeightTable( TComSlice* pcSlice )
{
  WPScalingParam  *wp;
  const ChromaFormat    format                = pcSlice->getPic()->getChromaFormat();
  const UInt            numberValidComponents = getNumberValidComponents(format);
  const Bool            bChroma               = isChromaEnabled(format);
  const Int             iNbRef                = (pcSlice->getSliceType() == B_SLICE ) ? (2) : (1);
        Bool            bDenomCoded           = false;
        UInt            uiTotalSignalledWeightFlags = 0;

  if ( (pcSlice->getSliceType()==P_SLICE && pcSlice->getPPS()->getUseWP()) || (pcSlice->getSliceType()==B_SLICE && pcSlice->getPPS()->getWPBiPred()) )
  {
    for ( Int iNumRef=0 ; iNumRef<iNbRef ; iNumRef++ ) // loop over l0 and l1 syntax elements
    {
      RefPicList  eRefPicList = ( iNumRef ? REF_PIC_LIST_1 : REF_PIC_LIST_0 );

      // NOTE: wp[].uiLog2WeightDenom and wp[].bPresentFlag are actually per-channel-type settings.

      for ( Int iRefIdx=0 ; iRefIdx<pcSlice->getNumRefIdx(eRefPicList) ; iRefIdx++ )
      {
        pcSlice->getWpScaling(eRefPicList, iRefIdx, wp);
        if ( !bDenomCoded )
        {
          Int iDeltaDenom;
          WRITE_UVLC( wp[COMPONENT_Y].uiLog2WeightDenom, "luma_log2_weight_denom" );

          if( bChroma )
          {
            assert(wp[COMPONENT_Cb].uiLog2WeightDenom == wp[COMPONENT_Cr].uiLog2WeightDenom); // check the channel-type settings are consistent across components.
            iDeltaDenom = (wp[COMPONENT_Cb].uiLog2WeightDenom - wp[COMPONENT_Y].uiLog2WeightDenom);
            WRITE_SVLC( iDeltaDenom, "delta_chroma_log2_weight_denom" );
          }
          bDenomCoded = true;
        }
        WRITE_FLAG( wp[COMPONENT_Y].bPresentFlag, iNumRef==0?"luma_weight_l0_flag[i]":"luma_weight_l1_flag[i]" );
        uiTotalSignalledWeightFlags += wp[COMPONENT_Y].bPresentFlag;
      }
      if (bChroma)
      {
        for ( Int iRefIdx=0 ; iRefIdx<pcSlice->getNumRefIdx(eRefPicList) ; iRefIdx++ )
        {
          pcSlice->getWpScaling(eRefPicList, iRefIdx, wp);
          assert(wp[COMPONENT_Cb].bPresentFlag == wp[COMPONENT_Cr].bPresentFlag); // check the channel-type settings are consistent across components.
          WRITE_FLAG( wp[COMPONENT_Cb].bPresentFlag, iNumRef==0?"chroma_weight_l0_flag[i]":"chroma_weight_l1_flag[i]" );
          uiTotalSignalledWeightFlags += 2*wp[COMPONENT_Cb].bPresentFlag;
        }
      }

      for ( Int iRefIdx=0 ; iRefIdx<pcSlice->getNumRefIdx(eRefPicList) ; iRefIdx++ )
      {
        pcSlice->getWpScaling(eRefPicList, iRefIdx, wp);
        if ( wp[COMPONENT_Y].bPresentFlag )
        {
          Int iDeltaWeight = (wp[COMPONENT_Y].iWeight - (1<<wp[COMPONENT_Y].uiLog2WeightDenom));
          WRITE_SVLC( iDeltaWeight, iNumRef==0?"delta_luma_weight_l0[i]":"delta_luma_weight_l1[i]" );
          WRITE_SVLC( wp[COMPONENT_Y].iOffset, iNumRef==0?"luma_offset_l0[i]":"luma_offset_l1[i]" );
        }

        if ( bChroma )
        {
          if ( wp[COMPONENT_Cb].bPresentFlag )
          {
            for ( Int j = COMPONENT_Cb ; j < numberValidComponents ; j++ )
            {
              assert(wp[COMPONENT_Cb].uiLog2WeightDenom == wp[COMPONENT_Cr].uiLog2WeightDenom);
              Int iDeltaWeight = (wp[j].iWeight - (1<<wp[COMPONENT_Cb].uiLog2WeightDenom));
              WRITE_SVLC( iDeltaWeight, iNumRef==0?"delta_chroma_weight_l0[i]":"delta_chroma_weight_l1[i]" );

              Int range=pcSlice->getSPS()->getSpsRangeExtension().getHighPrecisionOffsetsEnabledFlag() ? (1<<pcSlice->getSPS()->getBitDepth(CHANNEL_TYPE_CHROMA))/2 : 128;
              Int pred = ( range - ( ( range*wp[j].iWeight)>>(wp[j].uiLog2WeightDenom) ) );
              Int iDeltaChroma = (wp[j].iOffset - pred);
              WRITE_SVLC( iDeltaChroma, iNumRef==0?"delta_chroma_offset_l0[i]":"delta_chroma_offset_l1[i]" );
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
Void TEncCavlc::codeScalingList( const TComScalingList &scalingList )
{
  //for each size
  for(UInt sizeId = 0; sizeId < SCALING_LIST_SIZE_NUM; sizeId++)
  {
    const Int predListStep = (sizeId == SCALING_LIST_32x32? (SCALING_LIST_NUM/NUMBER_OF_PREDICTION_MODES) : 1); // if 32x32, skip over chroma entries.

    for(UInt listId = 0; listId < SCALING_LIST_NUM; listId+=predListStep)
    {
      Bool scalingListPredModeFlag = scalingList.getScalingListPredModeFlag(sizeId, listId);
      WRITE_FLAG( scalingListPredModeFlag, "scaling_list_pred_mode_flag" );
      if(!scalingListPredModeFlag)// Copy Mode
      {
        if (sizeId == SCALING_LIST_32x32)
        {
          // adjust the code, to cope with the missing chroma entries
          WRITE_UVLC( ((Int)listId - (Int)scalingList.getRefMatrixId (sizeId,listId)) / (SCALING_LIST_NUM/NUMBER_OF_PREDICTION_MODES), "scaling_list_pred_matrix_id_delta");
        }
        else
        {
          WRITE_UVLC( (Int)listId - (Int)scalingList.getRefMatrixId (sizeId,listId), "scaling_list_pred_matrix_id_delta");
        }
      }
      else// DPCM Mode
      {
        xCodeScalingList(&scalingList, sizeId, listId);
      }
    }
  }
  return;
}
/** code DPCM
 * \param scalingList quantization matrix information
 * \param sizeId      size index
 * \param listId      list index
 */
Void TEncCavlc::xCodeScalingList(const TComScalingList* scalingList, UInt sizeId, UInt listId)
{
  Int coefNum = min(MAX_MATRIX_COEF_NUM,(Int)g_scalingListSize[sizeId]);
  UInt* scan  = g_scanOrder[SCAN_UNGROUPED][SCAN_DIAG][sizeId==0 ? 2 : 3][sizeId==0 ? 2 : 3];
  Int nextCoef = SCALING_LIST_START_VALUE;
  Int data;
  const Int *src = scalingList->getScalingListAddress(sizeId, listId);
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

Void TEncCavlc::codeExplicitRdpcmMode( TComTU& /*rTu*/, const ComponentID /*compID*/ )
 {
   assert(0);
 }
#if NH_3D_SDC_INTRA || NH_3D_SDC_INTER
Void TEncCavlc::codeSDCFlag ( TComDataCU* pcCU, UInt uiAbsPartIdx )
{
  assert(0);
}
#endif
    
#if NH_3D_DBBP
Void TEncCavlc::codeDBBPFlag    ( TComDataCU* pcCU, UInt uiAbsPartIdx )
{
  assert(0);
}
#endif


//! \}
