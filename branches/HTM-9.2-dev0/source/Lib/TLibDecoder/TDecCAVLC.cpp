/* The copyright in this software is being made available under the BSD
* License, included below. This software may be subject to other third party
* and contributor rights, including patent rights, and no such rights are
* granted under this license.  
*
* Copyright (c) 2010-2013, ITU/ISO/IEC
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

/** \file     TDecCAVLC.cpp
\brief    CAVLC decoder class
*/

#include "TDecCAVLC.h"
#include "SEIread.h"
#include "TDecSlice.h"

//! \ingroup TLibDecoder
//! \{

#if ENC_DEC_TRACE

Void  xTraceSPSHeader (TComSPS *pSPS)
{
#if H_MV_ENC_DEC_TRAC
  if ( g_disableHLSTrace )
  {
    return; 
  }
  // To avoid mismatches
  fprintf( g_hTrace, "=========== Sequence Parameter Set ===========\n" );
#else
  fprintf( g_hTrace, "=========== Sequence Parameter Set ID: %d ===========\n", pSPS->getSPSId() );
#endif
}

Void  xTracePPSHeader (TComPPS *pPPS)
{
#if H_MV_ENC_DEC_TRAC
  if ( g_disableHLSTrace )
  {
    return; 
  }
  fprintf( g_hTrace, "=========== Picture Parameter Set ===========\n" );
#else
  fprintf( g_hTrace, "=========== Picture Parameter Set ID: %d ===========\n", pPPS->getPPSId() );
#endif
}

Void  xTraceSliceHeader (TComSlice *pSlice)
{
#if H_MV_ENC_DEC_TRAC
  if ( g_disableHLSTrace )
  {
    return; 
  }
#endif
  fprintf( g_hTrace, "=========== Slice ===========\n");
}

#endif

// ====================================================================================================================
// Constructor / destructor / create / destroy
// ====================================================================================================================

TDecCavlc::TDecCavlc()
{
#if H_3D
  m_aaiTempScale            = new Int* [ MAX_NUM_LAYERS ];
  m_aaiTempOffset           = new Int* [ MAX_NUM_LAYERS ];
  for( UInt uiVId = 0; uiVId < MAX_NUM_LAYERS; uiVId++ )
  {
    m_aaiTempScale            [ uiVId ] = new Int [ MAX_NUM_LAYERS ];
    m_aaiTempOffset           [ uiVId ] = new Int [ MAX_NUM_LAYERS ];
  }
#endif
}

TDecCavlc::~TDecCavlc()
{
#if H_3D
  for( UInt uiVId = 0; uiVId < MAX_NUM_LAYERS; uiVId++ )
  {
    delete [] m_aaiTempScale            [ uiVId ];
    delete [] m_aaiTempOffset           [ uiVId ];
  }
  delete [] m_aaiTempScale;
  delete [] m_aaiTempOffset;
#endif
}

// ====================================================================================================================
// Public member functions
// ====================================================================================================================

void TDecCavlc::parseShortTermRefPicSet( TComSPS* sps, TComReferencePictureSet* rps, Int idx )
{
  UInt code;
  UInt interRPSPred;
  if (idx > 0)
  {
    READ_FLAG(interRPSPred, "inter_ref_pic_set_prediction_flag");  rps->setInterRPSPrediction(interRPSPred);
  }
  else
  {
    interRPSPred = false;
    rps->setInterRPSPrediction(false);
  }

  if (interRPSPred) 
  {
    UInt bit;
    if(idx == sps->getRPSList()->getNumberOfReferencePictureSets())
    {
      READ_UVLC(code, "delta_idx_minus1" ); // delta index of the Reference Picture Set used for prediction minus 1
    }
    else
    {
      code = 0;
    }
    assert(code <= idx-1); // delta_idx_minus1 shall not be larger than idx-1, otherwise we will predict from a negative row position that does not exist. When idx equals 0 there is no legal value and interRPSPred must be zero. See J0185-r2
    Int rIdx =  idx - 1 - code;
    assert (rIdx <= idx-1 && rIdx >= 0); // Made assert tighter; if rIdx = idx then prediction is done from itself. rIdx must belong to range 0, idx-1, inclusive, see J0185-r2
    TComReferencePictureSet*   rpsRef = sps->getRPSList()->getReferencePictureSet(rIdx);
    Int k = 0, k0 = 0, k1 = 0;
    READ_CODE(1, bit, "delta_rps_sign"); // delta_RPS_sign
    READ_UVLC(code, "abs_delta_rps_minus1");  // absolute delta RPS minus 1
    Int deltaRPS = (1 - 2 * bit) * (code + 1); // delta_RPS
    for(Int j=0 ; j <= rpsRef->getNumberOfPictures(); j++)
    {
      READ_CODE(1, bit, "used_by_curr_pic_flag" ); //first bit is "1" if Idc is 1 
      Int refIdc = bit;
      if (refIdc == 0) 
      {
        READ_CODE(1, bit, "use_delta_flag" ); //second bit is "1" if Idc is 2, "0" otherwise.
        refIdc = bit<<1; //second bit is "1" if refIdc is 2, "0" if refIdc = 0.
      }
      if (refIdc == 1 || refIdc == 2)
      {
        Int deltaPOC = deltaRPS + ((j < rpsRef->getNumberOfPictures())? rpsRef->getDeltaPOC(j) : 0);
        rps->setDeltaPOC(k, deltaPOC);
        rps->setUsed(k, (refIdc == 1));

        if (deltaPOC < 0)
        {
          k0++;
        }
        else 
        {
          k1++;
        }
        k++;
      }  
      rps->setRefIdc(j,refIdc);  
    }
    rps->setNumRefIdc(rpsRef->getNumberOfPictures()+1);  
    rps->setNumberOfPictures(k);
    rps->setNumberOfNegativePictures(k0);
    rps->setNumberOfPositivePictures(k1);
    rps->sortDeltaPOC();
  }
  else
  {
    READ_UVLC(code, "num_negative_pics");           rps->setNumberOfNegativePictures(code);
    READ_UVLC(code, "num_positive_pics");           rps->setNumberOfPositivePictures(code);
    Int prev = 0;
    Int poc;
    for(Int j=0 ; j < rps->getNumberOfNegativePictures(); j++)
    {
      READ_UVLC(code, "delta_poc_s0_minus1");
      poc = prev-code-1;
      prev = poc;
      rps->setDeltaPOC(j,poc);
      READ_FLAG(code, "used_by_curr_pic_s0_flag");  rps->setUsed(j,code);
    }
    prev = 0;
    for(Int j=rps->getNumberOfNegativePictures(); j < rps->getNumberOfNegativePictures()+rps->getNumberOfPositivePictures(); j++)
    {
      READ_UVLC(code, "delta_poc_s1_minus1");
      poc = prev+code+1;
      prev = poc;
      rps->setDeltaPOC(j,poc);
      READ_FLAG(code, "used_by_curr_pic_s1_flag");  rps->setUsed(j,code);
    }
    rps->setNumberOfPictures(rps->getNumberOfNegativePictures()+rps->getNumberOfPositivePictures());
  }
#if PRINT_RPS_INFO
  rps->printDeltaPOC();
#endif
}

#if DLT_DIFF_CODING_IN_PPS
Void TDecCavlc::parsePPS(TComPPS* pcPPS, TComVPS* pcVPS )
#else
Void TDecCavlc::parsePPS(TComPPS* pcPPS)
#endif
{
#if ENC_DEC_TRACE  
  xTracePPSHeader (pcPPS);
#endif
  UInt  uiCode;

  Int   iCode;

  READ_UVLC( uiCode, "pps_pic_parameter_set_id");
  assert(uiCode <= 63);
  pcPPS->setPPSId (uiCode);
  
  READ_UVLC( uiCode, "pps_seq_parameter_set_id");
  assert(uiCode <= 15);
  pcPPS->setSPSId (uiCode);
  
  READ_FLAG( uiCode, "dependent_slice_segments_enabled_flag"    );    pcPPS->setDependentSliceSegmentsEnabledFlag   ( uiCode == 1 );
  READ_FLAG( uiCode, "output_flag_present_flag" );                    pcPPS->setOutputFlagPresentFlag( uiCode==1 );

  READ_CODE(3, uiCode, "num_extra_slice_header_bits");                pcPPS->setNumExtraSliceHeaderBits(uiCode);
  READ_FLAG ( uiCode, "sign_data_hiding_flag" ); pcPPS->setSignHideFlag( uiCode );

  READ_FLAG( uiCode,   "cabac_init_present_flag" );            pcPPS->setCabacInitPresentFlag( uiCode ? true : false );

  READ_UVLC(uiCode, "num_ref_idx_l0_default_active_minus1");
  assert(uiCode <= 14);
  pcPPS->setNumRefIdxL0DefaultActive(uiCode+1);
  
  READ_UVLC(uiCode, "num_ref_idx_l1_default_active_minus1");
  assert(uiCode <= 14);
  pcPPS->setNumRefIdxL1DefaultActive(uiCode+1);
  
  READ_SVLC(iCode, "init_qp_minus26" );                            pcPPS->setPicInitQPMinus26(iCode);
  READ_FLAG( uiCode, "constrained_intra_pred_flag" );              pcPPS->setConstrainedIntraPred( uiCode ? true : false );
  READ_FLAG( uiCode, "transform_skip_enabled_flag" );               
  pcPPS->setUseTransformSkip ( uiCode ? true : false ); 

  READ_FLAG( uiCode, "cu_qp_delta_enabled_flag" );            pcPPS->setUseDQP( uiCode ? true : false );
  if( pcPPS->getUseDQP() )
  {
    READ_UVLC( uiCode, "diff_cu_qp_delta_depth" );
    pcPPS->setMaxCuDQPDepth( uiCode );
  }
  else
  {
    pcPPS->setMaxCuDQPDepth( 0 );
  }
  READ_SVLC( iCode, "pps_cb_qp_offset");
  pcPPS->setChromaCbQpOffset(iCode);
  assert( pcPPS->getChromaCbQpOffset() >= -12 );
  assert( pcPPS->getChromaCbQpOffset() <=  12 );

  READ_SVLC( iCode, "pps_cr_qp_offset");
  pcPPS->setChromaCrQpOffset(iCode);
  assert( pcPPS->getChromaCrQpOffset() >= -12 );
  assert( pcPPS->getChromaCrQpOffset() <=  12 );

  READ_FLAG( uiCode, "pps_slice_chroma_qp_offsets_present_flag" );
  pcPPS->setSliceChromaQpFlag( uiCode ? true : false );

  READ_FLAG( uiCode, "weighted_pred_flag" );          // Use of Weighting Prediction (P_SLICE)
  pcPPS->setUseWP( uiCode==1 );
  READ_FLAG( uiCode, "weighted_bipred_flag" );         // Use of Bi-Directional Weighting Prediction (B_SLICE)
  pcPPS->setWPBiPred( uiCode==1 );

  READ_FLAG( uiCode, "transquant_bypass_enable_flag");
  pcPPS->setTransquantBypassEnableFlag(uiCode ? true : false);
  READ_FLAG( uiCode, "tiles_enabled_flag"               );    pcPPS->setTilesEnabledFlag            ( uiCode == 1 );
  READ_FLAG( uiCode, "entropy_coding_sync_enabled_flag" );    pcPPS->setEntropyCodingSyncEnabledFlag( uiCode == 1 );
  
  if( pcPPS->getTilesEnabledFlag() )
  {
    READ_UVLC ( uiCode, "num_tile_columns_minus1" );                pcPPS->setNumColumnsMinus1( uiCode );  
    READ_UVLC ( uiCode, "num_tile_rows_minus1" );                   pcPPS->setNumRowsMinus1( uiCode );  
    READ_FLAG ( uiCode, "uniform_spacing_flag" );                   pcPPS->setUniformSpacingFlag( uiCode );

    if( !pcPPS->getUniformSpacingFlag())
    {
      UInt* columnWidth = (UInt*)malloc(pcPPS->getNumColumnsMinus1()*sizeof(UInt));
      for(UInt i=0; i<pcPPS->getNumColumnsMinus1(); i++)
      { 
        READ_UVLC( uiCode, "column_width_minus1" );  
        columnWidth[i] = uiCode+1;
      }
      pcPPS->setColumnWidth(columnWidth);
      free(columnWidth);

      UInt* rowHeight = (UInt*)malloc(pcPPS->getNumRowsMinus1()*sizeof(UInt));
      for(UInt i=0; i<pcPPS->getNumRowsMinus1(); i++)
      {
        READ_UVLC( uiCode, "row_height_minus1" );
        rowHeight[i] = uiCode + 1;
      }
      pcPPS->setRowHeight(rowHeight);
      free(rowHeight);  
    }

    if(pcPPS->getNumColumnsMinus1() !=0 || pcPPS->getNumRowsMinus1() !=0)
    {
      READ_FLAG ( uiCode, "loop_filter_across_tiles_enabled_flag" );   pcPPS->setLoopFilterAcrossTilesEnabledFlag( uiCode ? true : false );
    }
  }
  READ_FLAG( uiCode, "loop_filter_across_slices_enabled_flag" );       pcPPS->setLoopFilterAcrossSlicesEnabledFlag( uiCode ? true : false );
  READ_FLAG( uiCode, "deblocking_filter_control_present_flag" );       pcPPS->setDeblockingFilterControlPresentFlag( uiCode ? true : false );
  if(pcPPS->getDeblockingFilterControlPresentFlag())
  {
    READ_FLAG( uiCode, "deblocking_filter_override_enabled_flag" );    pcPPS->setDeblockingFilterOverrideEnabledFlag( uiCode ? true : false );
    READ_FLAG( uiCode, "pps_disable_deblocking_filter_flag" );         pcPPS->setPicDisableDeblockingFilterFlag(uiCode ? true : false );
    if(!pcPPS->getPicDisableDeblockingFilterFlag())
    {
      READ_SVLC ( iCode, "pps_beta_offset_div2" );                     pcPPS->setDeblockingFilterBetaOffsetDiv2( iCode );
      READ_SVLC ( iCode, "pps_tc_offset_div2" );                       pcPPS->setDeblockingFilterTcOffsetDiv2( iCode );
    }
  }
#if H_MV
  if ( pcPPS->getLayerId() > 0 )
  {
    READ_FLAG( uiCode, "pps_infer_scaling_list_flag" ); pcPPS->setPpsInferScalingListFlag( uiCode == 1 );    
  }

  if( pcPPS->getPpsInferScalingListFlag( ) ) 
  {
    READ_CODE( 6, uiCode, "pps_scaling_list_ref_layer_id" ); pcPPS->setPpsScalingListRefLayerId( uiCode );
  }
  else
  {  
#endif
  READ_FLAG( uiCode, "pps_scaling_list_data_present_flag" );           pcPPS->setScalingListPresentFlag( uiCode ? true : false );
  if(pcPPS->getScalingListPresentFlag ())
  {
    parseScalingList( pcPPS->getScalingList() );
  }
#if H_MV
  }
#endif

  READ_FLAG( uiCode, "lists_modification_present_flag");
  pcPPS->setListsModificationPresentFlag(uiCode);

  READ_UVLC( uiCode, "log2_parallel_merge_level_minus2");
  pcPPS->setLog2ParallelMergeLevelMinus2 (uiCode);

  READ_FLAG( uiCode, "slice_segment_header_extension_present_flag");
  pcPPS->setSliceHeaderExtensionPresentFlag(uiCode);

  READ_FLAG( uiCode, "pps_extension_flag");
  if (uiCode)
  {
#if DLT_DIFF_CODING_IN_PPS
    parsePPSExtension( pcPPS, pcVPS );
    READ_FLAG( uiCode, "pps_extension2_flag");
    if ( uiCode )
    {
#endif
      while ( xMoreRbspData() )
      {
        READ_FLAG( uiCode, "pps_extension_data_flag");
      }
#if DLT_DIFF_CODING_IN_PPS
    }
#endif
  }
}

#if DLT_DIFF_CODING_IN_PPS
Void TDecCavlc::parsePPSExtension( TComPPS* pcPPS, TComVPS* pcVPS )
{
  UInt uiCode = 0; 
  TComDLT* pcDLT = new TComDLT;

  READ_FLAG(uiCode, "dlt_present_flag");
  pcDLT->setDltPresentFlag( (uiCode == 1) ? true : false );

  if ( pcDLT->getDltPresentFlag() )
  {
    READ_CODE(6, uiCode, "pps_depth_layers_minus1");
    pcDLT->setNumDepthViews( uiCode );

    READ_CODE(4, uiCode, "pps_bit_depth_for_depth_views_minus8");
    pcDLT->setDepthViewBitDepth( (uiCode+8) );

    for( Int i = 0; i <= pcVPS->getMaxLayersMinus1(); i++ )
    {
      if ( i != 0 )
      {
        if( pcVPS->getDepthId( i ) == 1 ) 
        {
          READ_FLAG(uiCode, "dlt_flag[i]");
          pcDLT->setUseDLTFlag(i, (uiCode == 1) ? true : false);

          if ( pcDLT->getUseDLTFlag( i ) )
          {
            Bool bDltBitMapRepFlag    = false;
            UInt uiMaxDiff            = 0xffffffff;
            UInt uiMinDiff            = 0;
            UInt uiCodeLength         = 0;

            READ_FLAG(uiCode, "inter_view_dlt_pred_enable_flag[ i ]"); 
            pcDLT->setInterViewDltPredEnableFlag( i, (uiCode == 1) ? true : false );

            if ( pcDLT->getInterViewDltPredEnableFlag( i ) == false )
            {
              READ_FLAG(uiCode, "dlt_bit_map_rep_flag[ layerId ]");
              bDltBitMapRepFlag = (uiCode == 1) ? true : false; 
            }
            else
            {
              bDltBitMapRepFlag = false;
            }
            
            UInt uiNumDepthValues = 0;
            Int  aiIdx2DepthValue[256];

            // Bit map
            if ( bDltBitMapRepFlag )
            {
              for (UInt d=0; d<256; d++)
              {
                READ_FLAG(uiCode, "dlt_bit_map_flag[ layerId ][ j ]");
                if (uiCode == 1)
                {
                  aiIdx2DepthValue[uiNumDepthValues] = d;
                  uiNumDepthValues++;
                }
              }
            }
            // Diff Coding
            else
            {
              READ_CODE(8, uiNumDepthValues, "num_depth_values_in_dlt[i]");   // num_entry

#if !H_3D_DELTA_DLT
              if ( pcDLT->getInterViewDltPredEnableFlag( i ) == false )       // Single-view DLT Diff Coding
#endif
              {
                // The condition if( pcVPS->getNumDepthValues(i) > 0 ) is always true since for Single-view Diff Coding, there is at least one depth value in depth component. 

                if (uiNumDepthValues > 1)
                {
                  READ_CODE(8, uiCode, "max_diff[ layerId ]"); 
                  uiMaxDiff = uiCode;
                }
                else
                {
                  uiMaxDiff = 0;           // when there is only one value in DLT
                }

                if (uiNumDepthValues > 2)
                {
                  uiCodeLength = (UInt) ceil(Log2(uiMaxDiff + 1));
                  READ_CODE(uiCodeLength, uiCode, "min_diff_minus1[ layerId ]");
                  uiMinDiff = uiCode + 1;
                }
                else
                {
                  uiMinDiff = uiMaxDiff;   // when there are only one or two values in DLT
                }

                READ_CODE(8, uiCode, "dlt_depth_value0[layerId]");   // entry0
                aiIdx2DepthValue[0] = uiCode;

                if (uiMaxDiff == uiMinDiff)
                {
                  for (UInt d=1; d<uiNumDepthValues; d++)
                  {
                    aiIdx2DepthValue[d] = aiIdx2DepthValue[d-1] + uiMinDiff + 0;
                  }
                }
                else
                {
                  uiCodeLength = (UInt) ceil(Log2(uiMaxDiff - uiMinDiff + 1));
                  for (UInt d=1; d<uiNumDepthValues; d++)
                  {
                    READ_CODE(uiCodeLength, uiCode, "dlt_depth_value_diff_minus_min[ layerId ][ j ]");
                    aiIdx2DepthValue[d] = aiIdx2DepthValue[d-1] + uiMinDiff + uiCode;
                  }
                }

              }
            }
            
#if H_3D_DELTA_DLT
            if( pcDLT->getInterViewDltPredEnableFlag( i ) )
            {
              // interpret decoded values as delta DLT
              AOF( pcVPS->getDepthId( 1 ) == 1 );
              AOF( i > 1 );
              // assumes ref layer id to be 1
              Int* piRefDLT = pcDLT->idx2DepthValue( 1 );
              UInt uiRefNum = pcDLT->getNumDepthValues( 1 );
              pcDLT->setDeltaDLT(i, piRefDLT, uiRefNum, aiIdx2DepthValue, uiNumDepthValues);
            }
            else
            {
              // store final DLT
              pcDLT->setDepthLUTs(i, aiIdx2DepthValue, uiNumDepthValues);
            }
#else
            // store final DLT
            pcDLT->setDepthLUTs(i, aiIdx2DepthValue, uiNumDepthValues);
#endif
          }
        }
      }
    }
  }

  pcPPS->setDLT( pcDLT );
}
#endif

Void  TDecCavlc::parseVUI(TComVUI* pcVUI, TComSPS *pcSPS)
{
#if ENC_DEC_TRACE
  fprintf( g_hTrace, "----------- vui_parameters -----------\n");
#endif
  UInt  uiCode;

  READ_FLAG(     uiCode, "aspect_ratio_info_present_flag");           pcVUI->setAspectRatioInfoPresentFlag(uiCode);
  if (pcVUI->getAspectRatioInfoPresentFlag())
  {
    READ_CODE(8, uiCode, "aspect_ratio_idc");                         pcVUI->setAspectRatioIdc(uiCode);
    if (pcVUI->getAspectRatioIdc() == 255)
    {
      READ_CODE(16, uiCode, "sar_width");                             pcVUI->setSarWidth(uiCode);
      READ_CODE(16, uiCode, "sar_height");                            pcVUI->setSarHeight(uiCode);
    }
  }

  READ_FLAG(     uiCode, "overscan_info_present_flag");               pcVUI->setOverscanInfoPresentFlag(uiCode);
  if (pcVUI->getOverscanInfoPresentFlag())
  {
    READ_FLAG(   uiCode, "overscan_appropriate_flag");                pcVUI->setOverscanAppropriateFlag(uiCode);
  }

  READ_FLAG(     uiCode, "video_signal_type_present_flag");           pcVUI->setVideoSignalTypePresentFlag(uiCode);
#if H_MV_6_PS_O0118_33
  assert( pcSPS->getLayerId() == 0 || !pcVUI->getVideoSignalTypePresentFlag() ); 
#endif

  if (pcVUI->getVideoSignalTypePresentFlag())
  {
    READ_CODE(3, uiCode, "video_format");                             pcVUI->setVideoFormat(uiCode);
    READ_FLAG(   uiCode, "video_full_range_flag");                    pcVUI->setVideoFullRangeFlag(uiCode);
    READ_FLAG(   uiCode, "colour_description_present_flag");          pcVUI->setColourDescriptionPresentFlag(uiCode);
    if (pcVUI->getColourDescriptionPresentFlag())
    {
      READ_CODE(8, uiCode, "colour_primaries");                       pcVUI->setColourPrimaries(uiCode);
      READ_CODE(8, uiCode, "transfer_characteristics");               pcVUI->setTransferCharacteristics(uiCode);
      READ_CODE(8, uiCode, "matrix_coefficients");                    pcVUI->setMatrixCoefficients(uiCode);
    }
  }

  READ_FLAG(     uiCode, "chroma_loc_info_present_flag");             pcVUI->setChromaLocInfoPresentFlag(uiCode);
  if (pcVUI->getChromaLocInfoPresentFlag())
  {
    READ_UVLC(   uiCode, "chroma_sample_loc_type_top_field" );        pcVUI->setChromaSampleLocTypeTopField(uiCode);
    READ_UVLC(   uiCode, "chroma_sample_loc_type_bottom_field" );     pcVUI->setChromaSampleLocTypeBottomField(uiCode);
  }

  READ_FLAG(     uiCode, "neutral_chroma_indication_flag");           pcVUI->setNeutralChromaIndicationFlag(uiCode);

  READ_FLAG(     uiCode, "field_seq_flag");                           pcVUI->setFieldSeqFlag(uiCode);

  READ_FLAG(uiCode, "frame_field_info_present_flag");                 pcVUI->setFrameFieldInfoPresentFlag(uiCode);

  READ_FLAG(     uiCode, "default_display_window_flag");
  if (uiCode != 0)
  {
    Window &defDisp = pcVUI->getDefaultDisplayWindow();
#if H_MV
    defDisp.setScaledFlag( false ); 
    READ_UVLC(   uiCode, "def_disp_win_left_offset" );                defDisp.setWindowLeftOffset  ( uiCode );
    READ_UVLC(   uiCode, "def_disp_win_right_offset" );               defDisp.setWindowRightOffset ( uiCode );
    READ_UVLC(   uiCode, "def_disp_win_top_offset" );                 defDisp.setWindowTopOffset   ( uiCode );
    READ_UVLC(   uiCode, "def_disp_win_bottom_offset" );              defDisp.setWindowBottomOffset( uiCode );
#else
    READ_UVLC(   uiCode, "def_disp_win_left_offset" );                defDisp.setWindowLeftOffset  ( uiCode * TComSPS::getWinUnitX( pcSPS->getChromaFormatIdc()) );
    READ_UVLC(   uiCode, "def_disp_win_right_offset" );               defDisp.setWindowRightOffset ( uiCode * TComSPS::getWinUnitX( pcSPS->getChromaFormatIdc()) );
    READ_UVLC(   uiCode, "def_disp_win_top_offset" );                 defDisp.setWindowTopOffset   ( uiCode * TComSPS::getWinUnitY( pcSPS->getChromaFormatIdc()) );
    READ_UVLC(   uiCode, "def_disp_win_bottom_offset" );              defDisp.setWindowBottomOffset( uiCode * TComSPS::getWinUnitY( pcSPS->getChromaFormatIdc()) );
#endif
  }
  TimingInfo *timingInfo = pcVUI->getTimingInfo();
  READ_FLAG(       uiCode, "vui_timing_info_present_flag");         timingInfo->setTimingInfoPresentFlag      (uiCode ? true : false);
  if(timingInfo->getTimingInfoPresentFlag())
  {
    READ_CODE( 32, uiCode, "vui_num_units_in_tick");                timingInfo->setNumUnitsInTick             (uiCode);
    READ_CODE( 32, uiCode, "vui_time_scale");                       timingInfo->setTimeScale                  (uiCode);
    READ_FLAG(     uiCode, "vui_poc_proportional_to_timing_flag");  timingInfo->setPocProportionalToTimingFlag(uiCode ? true : false);
    if(timingInfo->getPocProportionalToTimingFlag())
    {
      READ_UVLC(   uiCode, "vui_num_ticks_poc_diff_one_minus1");    timingInfo->setNumTicksPocDiffOneMinus1   (uiCode);
    }
  READ_FLAG(     uiCode, "hrd_parameters_present_flag");              pcVUI->setHrdParametersPresentFlag(uiCode);
  if( pcVUI->getHrdParametersPresentFlag() )
  {
    parseHrdParameters( pcVUI->getHrdParameters(), 1, pcSPS->getMaxTLayers() - 1 );
  }
  }
  READ_FLAG(     uiCode, "bitstream_restriction_flag");               pcVUI->setBitstreamRestrictionFlag(uiCode);
  if (pcVUI->getBitstreamRestrictionFlag())
  {
    READ_FLAG(   uiCode, "tiles_fixed_structure_flag");               pcVUI->setTilesFixedStructureFlag(uiCode);
    READ_FLAG(   uiCode, "motion_vectors_over_pic_boundaries_flag");  pcVUI->setMotionVectorsOverPicBoundariesFlag(uiCode);
    READ_FLAG(   uiCode, "restricted_ref_pic_lists_flag");            pcVUI->setRestrictedRefPicListsFlag(uiCode);
    READ_UVLC( uiCode, "min_spatial_segmentation_idc");            pcVUI->setMinSpatialSegmentationIdc(uiCode);
    assert(uiCode < 4096);
    READ_UVLC(   uiCode, "max_bytes_per_pic_denom" );                 pcVUI->setMaxBytesPerPicDenom(uiCode);
    READ_UVLC(   uiCode, "max_bits_per_mincu_denom" );                pcVUI->setMaxBitsPerMinCuDenom(uiCode);
    READ_UVLC(   uiCode, "log2_max_mv_length_horizontal" );           pcVUI->setLog2MaxMvLengthHorizontal(uiCode);
    READ_UVLC(   uiCode, "log2_max_mv_length_vertical" );             pcVUI->setLog2MaxMvLengthVertical(uiCode);
  }
}

Void TDecCavlc::parseHrdParameters(TComHRD *hrd, Bool commonInfPresentFlag, UInt maxNumSubLayersMinus1)
{
  UInt  uiCode;
  if( commonInfPresentFlag )
  {
    READ_FLAG( uiCode, "nal_hrd_parameters_present_flag" );           hrd->setNalHrdParametersPresentFlag( uiCode == 1 ? true : false );
    READ_FLAG( uiCode, "vcl_hrd_parameters_present_flag" );           hrd->setVclHrdParametersPresentFlag( uiCode == 1 ? true : false );
    if( hrd->getNalHrdParametersPresentFlag() || hrd->getVclHrdParametersPresentFlag() )
    {
      READ_FLAG( uiCode, "sub_pic_cpb_params_present_flag" );         hrd->setSubPicCpbParamsPresentFlag( uiCode == 1 ? true : false );
      if( hrd->getSubPicCpbParamsPresentFlag() )
      {
        READ_CODE( 8, uiCode, "tick_divisor_minus2" );                hrd->setTickDivisorMinus2( uiCode );
        READ_CODE( 5, uiCode, "du_cpb_removal_delay_length_minus1" ); hrd->setDuCpbRemovalDelayLengthMinus1( uiCode );
        READ_FLAG( uiCode, "sub_pic_cpb_params_in_pic_timing_sei_flag" ); hrd->setSubPicCpbParamsInPicTimingSEIFlag( uiCode == 1 ? true : false );
        READ_CODE( 5, uiCode, "dpb_output_delay_du_length_minus1"  ); hrd->setDpbOutputDelayDuLengthMinus1( uiCode );
      }
      READ_CODE( 4, uiCode, "bit_rate_scale" );                       hrd->setBitRateScale( uiCode );
      READ_CODE( 4, uiCode, "cpb_size_scale" );                       hrd->setCpbSizeScale( uiCode );
      if( hrd->getSubPicCpbParamsPresentFlag() )
      {
        READ_CODE( 4, uiCode, "cpb_size_du_scale" );                  hrd->setDuCpbSizeScale( uiCode );
      }
      READ_CODE( 5, uiCode, "initial_cpb_removal_delay_length_minus1" ); hrd->setInitialCpbRemovalDelayLengthMinus1( uiCode );
      READ_CODE( 5, uiCode, "au_cpb_removal_delay_length_minus1" );      hrd->setCpbRemovalDelayLengthMinus1( uiCode );
      READ_CODE( 5, uiCode, "dpb_output_delay_length_minus1" );       hrd->setDpbOutputDelayLengthMinus1( uiCode );
    }
  }
  Int i, j, nalOrVcl;
  for( i = 0; i <= maxNumSubLayersMinus1; i ++ )
  {
    READ_FLAG( uiCode, "fixed_pic_rate_general_flag" );                     hrd->setFixedPicRateFlag( i, uiCode == 1 ? true : false  );
    if( !hrd->getFixedPicRateFlag( i ) )
    {
      READ_FLAG( uiCode, "fixed_pic_rate_within_cvs_flag" );                hrd->setFixedPicRateWithinCvsFlag( i, uiCode == 1 ? true : false  );
    }
    else
    {
      hrd->setFixedPicRateWithinCvsFlag( i, true );
    }
    hrd->setLowDelayHrdFlag( i, 0 ); // Infered to be 0 when not present
    hrd->setCpbCntMinus1   ( i, 0 ); // Infered to be 0 when not present
    if( hrd->getFixedPicRateWithinCvsFlag( i ) )
    {
      READ_UVLC( uiCode, "elemental_duration_in_tc_minus1" );             hrd->setPicDurationInTcMinus1( i, uiCode );
    }
    else
    {      
      READ_FLAG( uiCode, "low_delay_hrd_flag" );                      hrd->setLowDelayHrdFlag( i, uiCode == 1 ? true : false  );
    }
    if (!hrd->getLowDelayHrdFlag( i ))
    {
      READ_UVLC( uiCode, "cpb_cnt_minus1" );                          hrd->setCpbCntMinus1( i, uiCode );      
    }
    for( nalOrVcl = 0; nalOrVcl < 2; nalOrVcl ++ )
    {
      if( ( ( nalOrVcl == 0 ) && ( hrd->getNalHrdParametersPresentFlag() ) ) ||
          ( ( nalOrVcl == 1 ) && ( hrd->getVclHrdParametersPresentFlag() ) ) )
      {
        for( j = 0; j <= ( hrd->getCpbCntMinus1( i ) ); j ++ )
        {
          READ_UVLC( uiCode, "bit_rate_value_minus1" );             hrd->setBitRateValueMinus1( i, j, nalOrVcl, uiCode );
          READ_UVLC( uiCode, "cpb_size_value_minus1" );             hrd->setCpbSizeValueMinus1( i, j, nalOrVcl, uiCode );
          if( hrd->getSubPicCpbParamsPresentFlag() )
          {
            READ_UVLC( uiCode, "cpb_size_du_value_minus1" );       hrd->setDuCpbSizeValueMinus1( i, j, nalOrVcl, uiCode );
            READ_UVLC( uiCode, "bit_rate_du_value_minus1" );       hrd->setDuBitRateValueMinus1( i, j, nalOrVcl, uiCode );
          }
          READ_FLAG( uiCode, "cbr_flag" );                          hrd->setCbrFlag( i, j, nalOrVcl, uiCode == 1 ? true : false  );
        }
      }
    }
  }
}

#if H_3D
Void TDecCavlc::parseSPS(TComSPS* pcSPS, Int viewIndex, Bool depthFlag )
#else
Void TDecCavlc::parseSPS(TComSPS* pcSPS)
#endif
{
#if ENC_DEC_TRACE  
  xTraceSPSHeader (pcSPS);
#endif

  UInt  uiCode;
  READ_CODE( 4,  uiCode, "sps_video_parameter_set_id");          pcSPS->setVPSId        ( uiCode );
#if H_MV
  if ( pcSPS->getLayerId() == 0 )
  {
#endif
  READ_CODE( 3,  uiCode, "sps_max_sub_layers_minus1" );          pcSPS->setMaxTLayers   ( uiCode+1 );
  assert(uiCode <= 6);
  
  READ_FLAG( uiCode, "sps_temporal_id_nesting_flag" );               pcSPS->setTemporalIdNestingFlag ( uiCode > 0 ? true : false );
  if ( pcSPS->getMaxTLayers() == 1 )
  {
    // sps_temporal_id_nesting_flag must be 1 when sps_max_sub_layers_minus1 is 0
    assert( uiCode == 1 );
  }
  
  parsePTL(pcSPS->getPTL(), 1, pcSPS->getMaxTLayers() - 1);
#if H_MV
  }
#endif
  READ_UVLC(     uiCode, "sps_seq_parameter_set_id" );           pcSPS->setSPSId( uiCode );
  assert(uiCode <= 15);
#if H_MV
  if ( pcSPS->getLayerId() > 0 )
  {
    READ_FLAG( uiCode, "update_rep_format_flag" );               pcSPS->setUpdateRepFormatFlag( uiCode == 1 );
#if H_MV_6_PS_REP_FORM_18_19_20
    if ( pcSPS->getUpdateRepFormatFlag() )
    { 
      READ_CODE( 8, uiCode, "sps_rep_format_idx" );                pcSPS->setSpsRepFormatIdx( uiCode );
    }
  }
  else
  {
#else
  }
  
  if ( pcSPS->getUpdateRepFormatFlag() )
  { 
#endif
#endif
  READ_UVLC(     uiCode, "chroma_format_idc" );                  pcSPS->setChromaFormatIdc( uiCode );
  assert(uiCode <= 3);
  // in the first version we only support chroma_format_idc equal to 1 (4:2:0), so separate_colour_plane_flag cannot appear in the bitstream
  assert (uiCode == 1);
  if( uiCode == 3 )
  {
    READ_FLAG(     uiCode, "separate_colour_plane_flag");        assert(uiCode == 0);
  }

  READ_UVLC (    uiCode, "pic_width_in_luma_samples" );          pcSPS->setPicWidthInLumaSamples ( uiCode    );
  READ_UVLC (    uiCode, "pic_height_in_luma_samples" );         pcSPS->setPicHeightInLumaSamples( uiCode    );
#if H_MV
  }
#endif
  READ_FLAG(     uiCode, "conformance_window_flag");
  if (uiCode != 0)
  {
    Window &conf = pcSPS->getConformanceWindow();
#if H_MV
    // Needs to be scaled later, when ChromaFormatIdc is known. 
    conf.setScaledFlag( false ); 
    READ_UVLC(   uiCode, "conf_win_left_offset" );               conf.setWindowLeftOffset  ( uiCode  );
    READ_UVLC(   uiCode, "conf_win_right_offset" );              conf.setWindowRightOffset ( uiCode  );
    READ_UVLC(   uiCode, "conf_win_top_offset" );                conf.setWindowTopOffset   ( uiCode  );
    READ_UVLC(   uiCode, "conf_win_bottom_offset" );             conf.setWindowBottomOffset( uiCode  );    
#else
    READ_UVLC(   uiCode, "conf_win_left_offset" );               conf.setWindowLeftOffset  ( uiCode * TComSPS::getWinUnitX( pcSPS->getChromaFormatIdc() ) );
    READ_UVLC(   uiCode, "conf_win_right_offset" );              conf.setWindowRightOffset ( uiCode * TComSPS::getWinUnitX( pcSPS->getChromaFormatIdc() ) );
    READ_UVLC(   uiCode, "conf_win_top_offset" );                conf.setWindowTopOffset   ( uiCode * TComSPS::getWinUnitY( pcSPS->getChromaFormatIdc() ) );
    READ_UVLC(   uiCode, "conf_win_bottom_offset" );             conf.setWindowBottomOffset( uiCode * TComSPS::getWinUnitY( pcSPS->getChromaFormatIdc() ) );
#endif
  }

#if H_MV
#if H_MV_6_PS_REP_FORM_18_19_20
  if ( pcSPS->getLayerId() == 0 )
#else
  if ( pcSPS->getUpdateRepFormatFlag() )
#endif
  { 
#endif
  READ_UVLC(     uiCode, "bit_depth_luma_minus8" );
  assert(uiCode <= 6);
  pcSPS->setBitDepthY( uiCode + 8 );
  pcSPS->setQpBDOffsetY( (Int) (6*uiCode) );

  READ_UVLC( uiCode,    "bit_depth_chroma_minus8" );
  assert(uiCode <= 6);
  pcSPS->setBitDepthC( uiCode + 8 );
  pcSPS->setQpBDOffsetC( (Int) (6*uiCode) );
#if H_MV
  }
#endif

  READ_UVLC( uiCode,    "log2_max_pic_order_cnt_lsb_minus4" );   pcSPS->setBitsForPOC( 4 + uiCode );
  assert(uiCode <= 12);

  UInt subLayerOrderingInfoPresentFlag;
  READ_FLAG(subLayerOrderingInfoPresentFlag, "sps_sub_layer_ordering_info_present_flag");
  
  for(UInt i=0; i <= pcSPS->getMaxTLayers()-1; i++)
  {
#if H_MV
    READ_UVLC ( uiCode, "sps_max_dec_pic_buffering_minus1[i]");
#else
    READ_UVLC ( uiCode, "sps_max_dec_pic_buffering_minus1");
#endif
    pcSPS->setMaxDecPicBuffering( uiCode + 1, i);
#if H_MV
    READ_UVLC ( uiCode, "sps_num_reorder_pics[i]" );
#else
    READ_UVLC ( uiCode, "sps_num_reorder_pics" );
#endif
    pcSPS->setNumReorderPics(uiCode, i);
#if H_MV
    READ_UVLC ( uiCode, "sps_max_latency_increase_plus1[i]");
#else
    READ_UVLC ( uiCode, "sps_max_latency_increase_plus1");
#endif
    pcSPS->setMaxLatencyIncrease( uiCode, i );

    if (!subLayerOrderingInfoPresentFlag)
    {
      for (i++; i <= pcSPS->getMaxTLayers()-1; i++)
      {
        pcSPS->setMaxDecPicBuffering(pcSPS->getMaxDecPicBuffering(0), i);
        pcSPS->setNumReorderPics(pcSPS->getNumReorderPics(0), i);
        pcSPS->setMaxLatencyIncrease(pcSPS->getMaxLatencyIncrease(0), i);
      }
      break;
    }
  }

  READ_UVLC( uiCode, "log2_min_coding_block_size_minus3" );
  Int log2MinCUSize = uiCode + 3;
  pcSPS->setLog2MinCodingBlockSize(log2MinCUSize);
  READ_UVLC( uiCode, "log2_diff_max_min_coding_block_size" );
  pcSPS->setLog2DiffMaxMinCodingBlockSize(uiCode);
  Int maxCUDepthDelta = uiCode;
  pcSPS->setMaxCUWidth  ( 1<<(log2MinCUSize + maxCUDepthDelta) ); 
  pcSPS->setMaxCUHeight ( 1<<(log2MinCUSize + maxCUDepthDelta) );
  READ_UVLC( uiCode, "log2_min_transform_block_size_minus2" );   pcSPS->setQuadtreeTULog2MinSize( uiCode + 2 );

  READ_UVLC( uiCode, "log2_diff_max_min_transform_block_size" ); pcSPS->setQuadtreeTULog2MaxSize( uiCode + pcSPS->getQuadtreeTULog2MinSize() );
  pcSPS->setMaxTrSize( 1<<(uiCode + pcSPS->getQuadtreeTULog2MinSize()) );

  READ_UVLC( uiCode, "max_transform_hierarchy_depth_inter" );    pcSPS->setQuadtreeTUMaxDepthInter( uiCode+1 );
  READ_UVLC( uiCode, "max_transform_hierarchy_depth_intra" );    pcSPS->setQuadtreeTUMaxDepthIntra( uiCode+1 );

  Int addCuDepth = max (0, log2MinCUSize - (Int)pcSPS->getQuadtreeTULog2MinSize() );
  pcSPS->setMaxCUDepth( maxCUDepthDelta + addCuDepth ); 

  READ_FLAG( uiCode, "scaling_list_enabled_flag" );                 pcSPS->setScalingListFlag ( uiCode );
  if(pcSPS->getScalingListFlag())
  {
#if H_MV
    if ( pcSPS->getLayerId() > 0 )
    {    
      READ_FLAG( uiCode, "sps_infer_scaling_list_flag" ); pcSPS->setSpsInferScalingListFlag( uiCode == 1 );
    }

    if ( pcSPS->getSpsInferScalingListFlag() )
    {
      READ_CODE( 6, uiCode, "sps_scaling_list_ref_layer_id" ); pcSPS->setSpsScalingListRefLayerId( uiCode );
    }
    else
    {    
#endif
    READ_FLAG( uiCode, "sps_scaling_list_data_present_flag" );                 pcSPS->setScalingListPresentFlag ( uiCode );
    if(pcSPS->getScalingListPresentFlag ())
    {
      parseScalingList( pcSPS->getScalingList() );
    }
#if H_MV
    }
#endif
  }
  READ_FLAG( uiCode, "amp_enabled_flag" );                          pcSPS->setUseAMP( uiCode );
  READ_FLAG( uiCode, "sample_adaptive_offset_enabled_flag" );       pcSPS->setUseSAO ( uiCode ? true : false );

  READ_FLAG( uiCode, "pcm_enabled_flag" ); pcSPS->setUsePCM( uiCode ? true : false );
  if( pcSPS->getUsePCM() )
  {
    READ_CODE( 4, uiCode, "pcm_sample_bit_depth_luma_minus1" );          pcSPS->setPCMBitDepthLuma   ( 1 + uiCode );
    READ_CODE( 4, uiCode, "pcm_sample_bit_depth_chroma_minus1" );        pcSPS->setPCMBitDepthChroma ( 1 + uiCode );
    READ_UVLC( uiCode, "log2_min_pcm_luma_coding_block_size_minus3" );   pcSPS->setPCMLog2MinSize (uiCode+3);
    READ_UVLC( uiCode, "log2_diff_max_min_pcm_luma_coding_block_size" ); pcSPS->setPCMLog2MaxSize ( uiCode+pcSPS->getPCMLog2MinSize() );
    READ_FLAG( uiCode, "pcm_loop_filter_disable_flag" );                 pcSPS->setPCMFilterDisableFlag ( uiCode ? true : false );
  }

  READ_UVLC( uiCode, "num_short_term_ref_pic_sets" );
  assert(uiCode <= 64);
  pcSPS->createRPSList(uiCode);

  TComRPSList* rpsList = pcSPS->getRPSList();
  TComReferencePictureSet* rps;

  for(UInt i=0; i< rpsList->getNumberOfReferencePictureSets(); i++)
  {
    rps = rpsList->getReferencePictureSet(i);
    parseShortTermRefPicSet(pcSPS,rps,i);
  }
  READ_FLAG( uiCode, "long_term_ref_pics_present_flag" );          pcSPS->setLongTermRefsPresent(uiCode);
  if (pcSPS->getLongTermRefsPresent()) 
  {
    READ_UVLC( uiCode, "num_long_term_ref_pic_sps" );
    pcSPS->setNumLongTermRefPicSPS(uiCode);
    for (UInt k = 0; k < pcSPS->getNumLongTermRefPicSPS(); k++)
    {
      READ_CODE( pcSPS->getBitsForPOC(), uiCode, "lt_ref_pic_poc_lsb_sps" );
      pcSPS->setLtRefPicPocLsbSps(k, uiCode);
      READ_FLAG( uiCode,  "used_by_curr_pic_lt_sps_flag[i]");
      pcSPS->setUsedByCurrPicLtSPSFlag(k, uiCode?1:0);
    }
  }
  READ_FLAG( uiCode, "sps_temporal_mvp_enable_flag" );            pcSPS->setTMVPFlagsPresent(uiCode);

  READ_FLAG( uiCode, "sps_strong_intra_smoothing_enable_flag" );  pcSPS->setUseStrongIntraSmoothing(uiCode);

  READ_FLAG( uiCode, "vui_parameters_present_flag" );             pcSPS->setVuiParametersPresentFlag(uiCode);

  if (pcSPS->getVuiParametersPresentFlag())
  {
    parseVUI(pcSPS->getVuiParameters(), pcSPS);
  }

  READ_FLAG( uiCode, "sps_extension_flag");
#if H_MV_6_PSEM_O0142_3
  pcSPS->setSpsExtensionFlag( uiCode ); 
#endif
  if (pcSPS->getSpsExtensionFlag( ) )
  {
#if !H_MV_6_PSEM_O0142_3
#if H_MV
    parseSPSExtension( pcSPS ); 
    READ_FLAG( uiCode, "sps_extension2_flag");
    if ( uiCode )
    {
#if H_3D
      parseSPSExtension2( pcSPS, viewIndex, depthFlag ); 
      READ_FLAG( uiCode, "sps_extension3_flag");
      if ( uiCode )
      {
#endif
#endif
#else    
    for (Int i = 0; i < PS_EX_T_MAX_NUM; i++)
    {
      READ_FLAG( uiCode, "sps_extension_type_flag" ); pcSPS->setSpsExtensionTypeFlag( i, uiCode );
#if H_3D
      assert( !pcSPS->getSpsExtensionTypeFlag( i ) || i == PS_EX_T_MV || i == PS_EX_T_3D || i == PS_EX_T_ESC ); 
#else
      assert( !pcSPS->getSpsExtensionTypeFlag( i ) || i == PS_EX_T_MV || i == PS_EX_T_ESC ); 
#endif
    }  

    if( pcSPS->getSpsExtensionTypeFlag( PS_EX_T_MV ))
    {
      parseSPSExtension( pcSPS ); 
    }

#if H_3D
    if( pcSPS->getSpsExtensionTypeFlag( PS_EX_T_3D ))
    {
      parseSPSExtension2( pcSPS, viewIndex, depthFlag  ); 
    }
#endif

    if ( pcSPS->getSpsExtensionTypeFlag( PS_EX_T_ESC ))
    {   
#endif
        while ( xMoreRbspData() )
        {
          READ_FLAG( uiCode, "sps_extension_data_flag");
        }
#if H_MV_6_PSEM_O0142_3
    }
#else
#if H_MV      
#if H_3D
      }
#endif
    }
#endif
#endif
  }
}

#if H_MV
Void TDecCavlc::parseSPSExtension( TComSPS* pcSPS )
{
  UInt uiCode; 
  READ_FLAG( uiCode, "inter_view_mv_vert_constraint_flag" );    pcSPS->setInterViewMvVertConstraintFlag(uiCode == 1 ? true : false);
#if !H_MV_6_SHVC_O0098_36
  READ_UVLC( uiCode, "sps_shvc_reserved_zero_idc" ); 
#else
  
  READ_UVLC( uiCode, "num_scaled_ref_layer_offsets" ); pcSPS->setNumScaledRefLayerOffsets( uiCode );

  for( Int i = 0; i < pcSPS->getNumScaledRefLayerOffsets( ); i++)
  {    
    READ_CODE( 6, uiCode, "scaled_ref_layer_id" ); pcSPS->setScaledRefLayerId( i, uiCode ); 

    Int j = pcSPS->getScaledRefLayerId( i ); 
    Int iCode; 
    READ_SVLC( iCode, "scaled_ref_layer_left_offset" ); pcSPS->setScaledRefLayerLeftOffset( j, iCode );
    READ_SVLC( iCode, "scaled_ref_layer_top_offset" ); pcSPS->setScaledRefLayerTopOffset( j, iCode );
    READ_SVLC( iCode, "scaled_ref_layer_right_offset" ); pcSPS->setScaledRefLayerRightOffset( j, iCode );
    READ_SVLC( iCode, "scaled_ref_layer_bottom_offset" ); pcSPS->setScaledRefLayerBottomOffset( j, iCode );
  }
#endif  
}

#if H_3D
Void TDecCavlc::parseSPSExtension2( TComSPS* pcSPS, Int viewIndex, Bool depthFlag )
{ 
  UInt uiCode; 
#if H_3D_QTLPC
  //GT: This has to go to VPS
  if( depthFlag )
  {
    READ_FLAG( uiCode, "use_qtl_flag" );
    pcSPS->setUseQTL( uiCode );
    READ_FLAG( uiCode, "use_pc_flag" );
    pcSPS->setUsePC( uiCode );
  }
#endif

#if !CAM_HLS_F0136_F0045_F0082
  UInt uiCamParPrecision = 0; 
  Bool bCamParSlice      = false; 
  if ( !depthFlag )
  {      
    READ_UVLC( uiCamParPrecision, "cp_precision" );
    READ_FLAG( uiCode, "cp_in_slice_header_flag" );    bCamParSlice = ( uiCode == 1 );
    if( !bCamParSlice )
    {       
      for( UInt uiBaseIndex = 0; uiBaseIndex < viewIndex; uiBaseIndex++ )
      {
        Int iCode; 
        READ_SVLC( iCode, "cp_scale" );                m_aaiTempScale  [ uiBaseIndex ][ viewIndex ]   = iCode;
        READ_SVLC( iCode, "cp_off" );                  m_aaiTempOffset [ uiBaseIndex ][ viewIndex ]   = iCode;
        READ_SVLC( iCode, "cp_inv_scale_plus_scale" ); m_aaiTempScale  [ viewIndex   ][ uiBaseIndex ] = iCode - m_aaiTempScale [ uiBaseIndex ][ viewIndex ];
        READ_SVLC( iCode, "cp_inv_off_plus_off" );     m_aaiTempOffset [ viewIndex   ][ uiBaseIndex ] = iCode - m_aaiTempOffset[ uiBaseIndex ][ viewIndex ];
      }
    }
  }
  pcSPS->initCamParaSPS( viewIndex, uiCamParPrecision, bCamParSlice, m_aaiTempScale, m_aaiTempOffset ); 
#endif
}
#endif
#endif

Void TDecCavlc::parseVPS(TComVPS* pcVPS)
{
  UInt  uiCode;
  
  READ_CODE( 4,  uiCode,  "vps_video_parameter_set_id" );         pcVPS->setVPSId( uiCode );
  READ_CODE( 2,  uiCode,  "vps_reserved_three_2bits" );           assert(uiCode == 3);
#if H_MV
#if H_MV_6_LAYER_ID_32
  READ_CODE( 6,  uiCode,  "vps_max_layers_minus1" );              pcVPS->setMaxLayersMinus1( std::min( uiCode, (UInt) ( MAX_NUM_LAYER_IDS-1) )  );
#else
  READ_CODE( 6,  uiCode,  "vps_max_layers_minus1" );              pcVPS->setMaxLayersMinus1( uiCode  );
#endif
#else
  READ_CODE( 6,  uiCode,  "vps_reserved_zero_6bits" );            assert(uiCode == 0);
#endif
  READ_CODE( 3,  uiCode,  "vps_max_sub_layers_minus1" );          pcVPS->setMaxTLayers( uiCode + 1 );
  READ_FLAG(     uiCode,  "vps_temporal_id_nesting_flag" );       pcVPS->setTemporalNestingFlag( uiCode ? true:false );
  assert (pcVPS->getMaxTLayers()>1||pcVPS->getTemporalNestingFlag());
#if H_MV
  READ_CODE( 16, uiCode,  "vps_extension_offset" );               
#else
  READ_CODE( 16, uiCode,  "vps_reserved_ffff_16bits" );           assert(uiCode == 0xffff);
#endif
  parsePTL ( pcVPS->getPTL(), true, pcVPS->getMaxTLayers()-1);
  UInt subLayerOrderingInfoPresentFlag;
  READ_FLAG(subLayerOrderingInfoPresentFlag, "vps_sub_layer_ordering_info_present_flag");
  for(UInt i = 0; i <= pcVPS->getMaxTLayers()-1; i++)
  {
    READ_UVLC( uiCode,  "vps_max_dec_pic_buffering_minus1[i]" );     pcVPS->setMaxDecPicBuffering( uiCode + 1, i );
    READ_UVLC( uiCode,  "vps_num_reorder_pics[i]" );          pcVPS->setNumReorderPics( uiCode, i );
    READ_UVLC( uiCode,  "vps_max_latency_increase_plus1[i]" );      pcVPS->setMaxLatencyIncrease( uiCode, i );

    if (!subLayerOrderingInfoPresentFlag)
    {
      for (i++; i <= pcVPS->getMaxTLayers()-1; i++)
      {
        pcVPS->setMaxDecPicBuffering(pcVPS->getMaxDecPicBuffering(0), i);
        pcVPS->setNumReorderPics(pcVPS->getNumReorderPics(0), i);
        pcVPS->setMaxLatencyIncrease(pcVPS->getMaxLatencyIncrease(0), i);
      }
      break;
    }
  }

  assert( pcVPS->getNumHrdParameters() < MAX_VPS_OP_SETS_PLUS1 );
#if H_MV
  assert( pcVPS->getVpsMaxLayerId() < MAX_VPS_NUH_LAYER_ID_PLUS1 );
  READ_CODE( 6, uiCode, "vps_max_layer_id" );   pcVPS->setVpsMaxLayerId( uiCode );

  READ_UVLC(    uiCode, "vps_max_num_layer_sets_minus1" );               pcVPS->setVpsNumLayerSetsMinus1( uiCode );
  for( UInt opsIdx = 1; opsIdx <= pcVPS->getVpsNumLayerSetsMinus1(); opsIdx ++ )
  {
    for( UInt i = 0; i <= pcVPS->getVpsMaxLayerId(); i ++ )
#else
  assert( pcVPS->getMaxNuhReservedZeroLayerId() < MAX_VPS_NUH_RESERVED_ZERO_LAYER_ID_PLUS1 );
  READ_CODE( 6, uiCode, "vps_max_nuh_reserved_zero_layer_id" );   pcVPS->setMaxNuhReservedZeroLayerId( uiCode );
  READ_UVLC(    uiCode, "vps_max_op_sets_minus1" );               pcVPS->setMaxOpSets( uiCode + 1 );
  for( UInt opsIdx = 1; opsIdx <= ( pcVPS->getMaxOpSets() - 1 ); opsIdx ++ )
  {
    // Operation point set
    for( UInt i = 0; i <= pcVPS->getMaxNuhReservedZeroLayerId(); i ++ )
#endif
    {
      READ_FLAG( uiCode, "layer_id_included_flag[opsIdx][i]" );     pcVPS->setLayerIdIncludedFlag( uiCode == 1 ? true : false, opsIdx, i );
    }
  }
#if H_MV_6_HRD_O0217_13
  pcVPS->deriveLayerSetLayerIdList(); 
#endif
  TimingInfo *timingInfo = pcVPS->getTimingInfo();
  READ_FLAG(       uiCode, "vps_timing_info_present_flag");         timingInfo->setTimingInfoPresentFlag      (uiCode ? true : false);
  if(timingInfo->getTimingInfoPresentFlag())
  {
    READ_CODE( 32, uiCode, "vps_num_units_in_tick");                timingInfo->setNumUnitsInTick             (uiCode);
    READ_CODE( 32, uiCode, "vps_time_scale");                       timingInfo->setTimeScale                  (uiCode);
    READ_FLAG(     uiCode, "vps_poc_proportional_to_timing_flag");  timingInfo->setPocProportionalToTimingFlag(uiCode ? true : false);
    if(timingInfo->getPocProportionalToTimingFlag())
    {
      READ_UVLC(   uiCode, "vps_num_ticks_poc_diff_one_minus1");    timingInfo->setNumTicksPocDiffOneMinus1   (uiCode);
    }
    READ_UVLC( uiCode, "vps_num_hrd_parameters" );                  pcVPS->setNumHrdParameters( uiCode );

    if( pcVPS->getNumHrdParameters() > 0 )
    {
      pcVPS->createHrdParamBuffer();
    }
    for( UInt i = 0; i < pcVPS->getNumHrdParameters(); i ++ )
    {
      READ_UVLC( uiCode, "hrd_op_set_idx" );                       pcVPS->setHrdOpSetIdx( uiCode, i );
      if( i > 0 )
      {
        READ_FLAG( uiCode, "cprms_present_flag[i]" );              pcVPS->setCprmsPresentFlag( uiCode == 1 ? true : false, i );
      }
      parseHrdParameters(pcVPS->getHrdParameters(i), pcVPS->getCprmsPresentFlag( i ), pcVPS->getMaxTLayers() - 1);
    }
  }
  READ_FLAG( uiCode,  "vps_extension_flag" );
  if (uiCode)
  {
#if H_MV
    m_pcBitstream->readOutTrailingBits();
    parseVPSExtension( pcVPS );   
    READ_FLAG( uiCode,  "vps_extension2_flag" );
    if (uiCode)
    {
#if H_3D
      m_pcBitstream->readOutTrailingBits();
#if CAM_HLS_F0136_F0045_F0082
      pcVPS->createCamPars(pcVPS->getNumViews());
#endif
      parseVPSExtension2( pcVPS );   
      READ_FLAG( uiCode,  "vps_extension3_flag" );
      if (uiCode)
      {      
#endif
#endif  
        while ( xMoreRbspData() )
        {
          READ_FLAG( uiCode, "vps_extension_data_flag");
        }
#if H_MV
#if H_3D
      }
#endif
    }
#endif
  }
  return; 
}

#if H_MV
Void TDecCavlc::parseVPSExtension( TComVPS* pcVPS )
{
  UInt uiCode; 
  READ_FLAG( uiCode, "avc_base_layer_flag" );                     pcVPS->setAvcBaseLayerFlag( uiCode == 1 ? true : false );
#if H_MV_6_PS_O0109_24
  READ_FLAG( uiCode, "vps_vui_present_flag" );                    pcVPS->setVpsVuiPresentFlag( uiCode == 1 );  if ( pcVPS->getVpsVuiPresentFlag() )
  {  
#endif
  READ_CODE( 16, uiCode, "vps_vui_offset" );                      pcVPS->setVpsVuiOffset( uiCode );
#if H_MV_6_PS_O0109_24
  }
#endif

  READ_FLAG( uiCode, "splitting_flag" );                          pcVPS->setSplittingFlag( uiCode == 1 ? true : false );

  for( Int sIdx = 0; sIdx < MAX_NUM_SCALABILITY_TYPES; sIdx++ )
  {
    READ_FLAG( uiCode,  "scalability_mask_flag[i]" );             pcVPS->setScalabilityMaskFlag( sIdx, uiCode == 1 ? true : false );      
  }

  for( Int sIdx = 0; sIdx < pcVPS->getNumScalabilityTypes( ) - ( pcVPS->getSplittingFlag() ? 1 : 0 ); sIdx++ )
  {
    READ_CODE( 3, uiCode, "dimension_id_len_minus1[j]" );       pcVPS->setDimensionIdLen( sIdx, uiCode + 1 );
  }

  if ( pcVPS->getSplittingFlag() )
  {
    pcVPS->setDimensionIdLen( pcVPS->getNumScalabilityTypes( ) - 1, pcVPS->inferLastDimsionIdLenMinus1() );       
  }

  READ_FLAG( uiCode, "vps_nuh_layer_id_present_flag" );           pcVPS->setVpsNuhLayerIdPresentFlag( uiCode == 1 ? true : false );

  for( Int i = 1; i <= pcVPS->getMaxLayersMinus1(); i++ )
  {
    if ( pcVPS->getVpsNuhLayerIdPresentFlag() )
    {
      READ_CODE( 6, uiCode, "layer_id_in_nuh[i]" );                pcVPS->setLayerIdInNuh( i, uiCode );
    }
    else
    {
      pcVPS->setLayerIdInNuh( i, i );; 
    }

    pcVPS->setLayerIdInVps( pcVPS->getLayerIdInNuh( i ), i ); 

    for( Int j = 0; j < pcVPS->getNumScalabilityTypes() ; j++ ) 
    {
      if ( !pcVPS->getSplittingFlag() )
      {
        READ_CODE( pcVPS->getDimensionIdLen( j ), uiCode, "dimension_id[i][j]" );  pcVPS->setDimensionId( i, j, uiCode );
      }
      else
      {
        pcVPS->setDimensionId( i, j, pcVPS->inferDimensionId( i, j)  );
      }
    }
  }

#if H_MV_6_PS_O0109_22
  READ_CODE( 4, uiCode, "view_id_len" ); pcVPS->setViewIdLen( uiCode );

  if ( pcVPS->getViewIdLen( ) > 0 )
  {    
    for( Int i = 0; i < pcVPS->getNumViews(); i++ )
    {
      READ_CODE( pcVPS->getViewIdLen( ), uiCode, "view_id_val[i]" ); pcVPS->setViewIdVal( i, uiCode );
    }
  }
  else
  {
    for( Int i = 0; i < pcVPS->getNumViews(); i++ )
    {
      pcVPS->setViewIdVal( i, 0 );  
    }
  }
#else
  // GT spec says: trac #39
  // if ( pcVPS->getNumViews() > 1 )  
  //   However, this is a bug in the text since, view_id_len_minus1 is needed to parse view_id_val. 
  {
    READ_CODE( 4, uiCode, "view_id_len_minus1" ); pcVPS->setViewIdLenMinus1( uiCode );
  }

  for( Int i = 0; i < pcVPS->getNumViews(); i++ )
  {
    READ_CODE( pcVPS->getViewIdLenMinus1( ) + 1, uiCode, "view_id_val[i]" ); pcVPS->setViewIdVal( i, uiCode );
  }
#endif


  for( Int i = 1; i <= pcVPS->getMaxLayersMinus1(); i++ )
  {
    for( Int j = 0; j < i; j++ )
    {
      READ_FLAG( uiCode, "direct_dependency_flag[i][j]" );             pcVPS->setDirectDependencyFlag( i, j, uiCode );
    }
  }
#if H_MV_6_MISC_O0062_31
  pcVPS->setRefLayers(); 
#endif
#if H_MV_6_ILDSD_O0120_26
  READ_FLAG( uiCode, "vps_sub_layers_max_minus1_present_flag" ); pcVPS->setVpsSubLayersMaxMinus1PresentFlag( uiCode == 1 );
  if ( pcVPS->getVpsSubLayersMaxMinus1PresentFlag() )
  {
    for (Int i = 0; i < pcVPS->getMaxLayersMinus1(); i++ )
    {
      READ_CODE( 3, uiCode, "sub_layers_vps_max_minus1" ); pcVPS->setSubLayersVpsMaxMinus1( i, uiCode );    
      pcVPS->checkSubLayersVpsMaxMinus1( i ); 

    }
  }  
  else
  {
    for (Int i = 0; i < pcVPS->getMaxLayersMinus1(); i++ )
    {
      pcVPS->setSubLayersVpsMaxMinus1( i, pcVPS->getMaxTLayers( ) - 1);    
    }
  }
#endif
  READ_FLAG( uiCode, "max_tid_ref_present_flag" ); pcVPS->setMaxTidRefPresentFlag( uiCode == 1 );

  if ( pcVPS->getMaxTidRefPresentFlag() )
  {    
    for( Int i = 0; i < pcVPS->getMaxLayersMinus1(); i++ )
    {
#if H_MV_6_ILDDS_O0225_30
      for( Int j = i + 1; j <= pcVPS->getMaxLayersMinus1(); j++ )
      {
        if ( pcVPS->getDirectDependencyFlag(j,i) )
        {
          READ_CODE( 3, uiCode, "max_tid_il_ref_pics_plus1" ); pcVPS->setMaxTidIlRefPicsPlus1( i, j, uiCode );
        }
      }
#else
      READ_CODE( 3, uiCode,       "max_tid_il_ref_pics_plus1[i]" );      pcVPS->setMaxTidIlRefPicPlus1( i , uiCode ); 
#endif
    }
  }

  READ_FLAG( uiCode, "all_ref_layers_active_flag" );             pcVPS->setAllRefLayersActiveFlag( uiCode == 1 );
  READ_CODE( 10, uiCode, "vps_number_layer_sets_minus1"      );  pcVPS->setVpsNumberLayerSetsMinus1    ( uiCode ); 
  READ_CODE( 6,  uiCode, "vps_num_profile_tier_level_minus1" );  pcVPS->setVpsNumProfileTierLevelMinus1( uiCode );

  for( Int i = 1; i <= pcVPS->getVpsNumProfileTierLevelMinus1(); i++ )
  {
    READ_FLAG(  uiCode, "vps_profile_present_flag[i]" );    pcVPS->setVpsProfilePresentFlag( i, uiCode == 1 );
    if( !pcVPS->getVpsProfilePresentFlag( i ) )
    {
      READ_CODE( 6, uiCode, "profile_ref_minus1[i]" ); pcVPS->setProfileRefMinus1( i, uiCode );
#if H_MV_6_PS_O0109_23
      pcVPS->checkProfileRefMinus1( i );      
#endif
    }
    parsePTL ( pcVPS->getPTL( i ), pcVPS->getVpsProfilePresentFlag( i ), pcVPS->getMaxTLayers()-1);
    if( !pcVPS->getVpsProfilePresentFlag( i ) )
    {
      TComPTL temp = *pcVPS->getPTL( i );
      *pcVPS->getPTL( i ) = *pcVPS->getPTL( pcVPS->getProfileRefMinus1( i ) + 1 );
      pcVPS->getPTL( i )->copyLevelFrom( &temp );
    }
  }

  Int numOutputLayerSets = pcVPS->getVpsNumberLayerSetsMinus1( ) + 1; 

  READ_FLAG( uiCode, "more_output_layer_sets_than_default_flag" ); pcVPS->setMoreOutputLayerSetsThanDefaultFlag( uiCode == 1 );

  if ( pcVPS->getMoreOutputLayerSetsThanDefaultFlag( ) )
  {
    READ_CODE( 10, uiCode, "num_add_output_layer_sets_minus1"      ); pcVPS->setNumAddOutputLayerSetsMinus1( uiCode );
    numOutputLayerSets += ( pcVPS->getNumAddOutputLayerSetsMinus1( ) + 1); 
  }

  if( numOutputLayerSets > 1)
  {
#if H_MV_6_PS_0109_25
    READ_CODE( 2, uiCode, "default_one_target_output_layer_idc" ); pcVPS->setDefaultOneTargetOutputLayerIdc( uiCode );
    pcVPS->checkDefaultOneTargetOutputLayerIdc(); 
#else
    READ_FLAG( uiCode, "default_one_target_output_layer_flag" ); pcVPS->setDefaultOneTargetOutputLayerFlag(  uiCode == 1); 
#endif
  }  

#if H_MV_6_HRD_O0217_13
  pcVPS->setOutputLayerFlag(0, 0, pcVPS->inferOutputLayerFlag( 0, 0 )); 
  pcVPS->setOutputLayerSetIdxMinus1(0, -1); 
#endif
  for( Int i = 1; i < numOutputLayerSets; i++ )
  {
    if( i > pcVPS->getVpsNumberLayerSetsMinus1( ) )
    {        
      READ_UVLC( uiCode,      "output_layer_set_idx_minus1[i]" ); pcVPS->setOutputLayerSetIdxMinus1( i, uiCode ); 
      for( Int j = 0; j < pcVPS->getNumLayersInIdList( j ) - 1; j++ )
      {
        READ_FLAG( uiCode, "output_layer_flag" ); pcVPS->setOutputLayerFlag( i, j, uiCode == 1 ); 
      }        
    }
#if H_MV_6_HRD_O0217_13
    else
    { // These inference rules would also be helpful in spec text
      pcVPS->setOutputLayerSetIdxMinus1(i, i - 1 ); 
      for( Int j = 0; j < pcVPS->getNumLayersInIdList( j ) - 1; j++ )
      {              
        pcVPS->setOutputLayerFlag(i,j, pcVPS->inferOutputLayerFlag( i, j )); 
      }
    }
#endif

    if ( pcVPS->getProfileLevelTierIdxLen()  > 0 )
    {      
      READ_CODE( pcVPS->getProfileLevelTierIdxLen(), uiCode,"profile_level_tier_idx[ i ]" );   pcVPS->setProfileLevelTierIdx( i , uiCode ); 
    }
  }
#if H_MV_6_GEN_0153_28
  if( pcVPS->getMaxLayersMinus1() > 0 )
  {
    READ_FLAG( uiCode, "alt_output_layer_flag" ); pcVPS->setAltOutputLayerFlag( uiCode == 1 );
  }
#endif
#if H_MV_6_HRD_O0217_13
  pcVPS->deriveTargetLayerIdLists(); 
#endif
  READ_FLAG( uiCode, "rep_format_idx_present_flag" ); pcVPS->setRepFormatIdxPresentFlag( uiCode == 1 );
  if ( pcVPS->getRepFormatIdxPresentFlag() )
  {
    READ_CODE( 4, uiCode, "vps_num_rep_formats_minus1" ); pcVPS->setVpsNumRepFormatsMinus1( uiCode );
  }

  for (Int i = 0; i <= pcVPS->getVpsNumRepFormatsMinus1(); i++ )
  { 
#if H_MV_6_PS_REP_FORM_18_19_20
    assert( pcVPS->getRepFormat(i) == NULL ); 
    TComRepFormat* curRepFormat = new TComRepFormat(); 
    TComRepFormat* prevRepFormat = i > 0 ? pcVPS->getRepFormat( i - 1) : NULL; 
    parseRepFormat( i, curRepFormat ,  prevRepFormat); 
    pcVPS->setRepFormat(i, curRepFormat ); 
#else
    assert( pcVPS->getRepFormat(i) == NULL ); 
    TComRepFormat* repFormat = new TComRepFormat(); 
    parseRepFormat( repFormat );
    pcVPS->setRepFormat(i, repFormat ); 
#endif
  }

  if( pcVPS->getRepFormatIdxPresentFlag() ) 
  {
    for( Int i = 1; i <=  pcVPS->getMaxLayersMinus1(); i++ )
    {
      if( pcVPS->getVpsNumRepFormatsMinus1() > 0 )
      {
#if H_MV_6_PS_REP_FORM_18_19_20
        READ_CODE( 8, uiCode, "vps_rep_format_idx" ); pcVPS->setVpsRepFormatIdx( i, uiCode );
#else
        READ_CODE( 4, uiCode, "vps_rep_format_idx" ); pcVPS->setVpsRepFormatIdx( i, uiCode );
#endif
      }
    }
  }

  READ_FLAG( uiCode, "max_one_active_ref_layer_flag" ); pcVPS->setMaxOneActiveRefLayerFlag ( uiCode == 1 ); 
#if H_MV_6_MISC_O0062_31
  for( Int i = 1; i  <=  pcVPS->getMaxLayersMinus1(); i++ )
  {
    if( pcVPS->getNumDirectRefLayers( pcVPS->getLayerIdInNuh( i ) )  ==  0 )
    {      
      READ_FLAG( uiCode, "poc_lsb_not_present_flag" ); pcVPS->setPocLsbNotPresentFlag( i, uiCode == 1 );
    }
  }
#endif

#if H_MV_6_HRD_O0217_13
  parseDpbSize( pcVPS ); 
#endif

#if !H_MV_6_PS_O0223_29
  READ_FLAG( uiCode, "cross_layer_irap_aligned_flag" ); pcVPS->setCrossLayerIrapAlignedFlag( uiCode == 1 );
#endif
  READ_UVLC( uiCode, "direct_dep_type_len_minus2")    ; pcVPS->setDirectDepTypeLenMinus2   ( uiCode ); 

#if H_MV_6_PS_O0096_21
  READ_FLAG( uiCode, "default_direct_dependency_flag" ); pcVPS->setDefaultDirectDependencyFlag( uiCode == 1 );
  if ( pcVPS->getDefaultDirectDependencyFlag( ) )
  {  
    READ_CODE( pcVPS->getDirectDepTypeLenMinus2( ) + 2, uiCode, "default_direct_dependency_type" ); pcVPS->setDefaultDirectDependencyType( uiCode );
  }

  for( Int i = 1; i <= pcVPS->getMaxLayersMinus1(); i++ )
  {
    for( Int j = 0; j < i; j++ )
    {
      if (pcVPS->getDirectDependencyFlag( i, j) )
      {        
        if ( pcVPS->getDefaultDirectDependencyFlag( ) )
        {  
          pcVPS->setDirectDependencyType( i, j , pcVPS->getDefaultDirectDependencyType( ) );
        }
        else
        {
          READ_CODE( pcVPS->getDirectDepTypeLenMinus2( ) + 2,  uiCode, "direct_dependency_type[i][j]" ); pcVPS->setDirectDependencyType( i, j , uiCode);
        }
      }
    }
  }  
#else
  for( Int i = 1; i <= pcVPS->getMaxLayersMinus1(); i++ )
  {
    for( Int j = 0; j < i; j++ )
    {
      if (pcVPS->getDirectDependencyFlag( i, j) )
      {        
        READ_CODE( pcVPS->getDirectDepTypeLenMinus2( ) + 2,  uiCode, "direct_dependency_type[i][j]" ); pcVPS->setDirectDependencyType( i, j , uiCode);
      }
    }
  }
#endif

  READ_FLAG( uiCode, "vps_shvc_reserved_zero_flag" ); 
#if !H_MV_6_PS_O0109_24
  READ_FLAG( uiCode, "vps_vui_present_flag" )       ; pcVPS->setVpsVuiPresentFlag( uiCode == 1 );
#endif

  if( pcVPS->getVpsVuiPresentFlag() )
  {
    m_pcBitstream->readOutTrailingBits(); // vps_vui_alignment_bit_equal_to_one
    parseVPSVUI( pcVPS ); 
  }     

  pcVPS->checkVPSExtensionSyntax(); 
#if !H_MV_6_MISC_O0062_31
  pcVPS->setRefLayers(); 
#endif
}

#if H_MV_6_PS_REP_FORM_18_19_20
Void TDecCavlc::parseRepFormat( Int i, TComRepFormat* pcRepFormat, TComRepFormat* pcPrevRepFormat )
#else
Void TDecCavlc::parseRepFormat( TComRepFormat* pcRepFormat )
#endif
{
  assert( pcRepFormat ); 

  UInt uiCode; 

#if H_MV_6_PS_REP_FORM_18_19_20
  READ_CODE( 16, uiCode, "pic_width_vps_in_luma_samples" );  pcRepFormat->setPicWidthVpsInLumaSamples ( uiCode );
  READ_CODE( 16, uiCode, "pic_height_vps_in_luma_samples" ); pcRepFormat->setPicHeightVpsInLumaSamples( uiCode );
  READ_FLAG( uiCode, "chroma_and_bit_depth_vps_present_flag" ); pcRepFormat->setChromaAndBitDepthVpsPresentFlag( uiCode == 1 );

  pcRepFormat->checkChromaAndBitDepthVpsPresentFlag( i ); 

  if ( pcRepFormat->getChromaAndBitDepthVpsPresentFlag() )
  {  
#endif
  READ_CODE( 2,  uiCode, "chroma_format_vps_idc" );          pcRepFormat->setChromaFormatVpsIdc       ( uiCode );
  if ( pcRepFormat->getChromaFormatVpsIdc() == 3 )
  {
    READ_FLAG( uiCode, "separate_colour_plane_vps_flag" ); pcRepFormat->setSeparateColourPlaneVpsFlag( uiCode == 1 ); 
  }
#if !H_MV_6_PS_REP_FORM_18_19_20
  READ_CODE( 16, uiCode, "pic_width_vps_in_luma_samples" );  pcRepFormat->setPicWidthVpsInLumaSamples ( uiCode );
  READ_CODE( 16, uiCode, "pic_height_vps_in_luma_samples" ); pcRepFormat->setPicHeightVpsInLumaSamples( uiCode );
#endif
  READ_CODE( 4,  uiCode, "bit_depth_vps_luma_minus8" );      pcRepFormat->setBitDepthVpsLumaMinus8    ( uiCode );
  READ_CODE( 4,  uiCode, "bit_depth_vps_chroma_minus8" );    pcRepFormat->setBitDepthVpsChromaMinus8  ( uiCode );
#if H_MV_6_PS_REP_FORM_18_19_20
  }
  else
  {
    pcRepFormat->inferChromaAndBitDepth(pcPrevRepFormat, false ); 
  }
#endif
}


Void TDecCavlc::parseVPSVUI( TComVPS* pcVPS )
{
  assert( pcVPS ); 

  TComVPSVUI* pcVPSVUI = pcVPS->getVPSVUI( ); 

  assert( pcVPSVUI ); 

  UInt uiCode; 
#if H_MV_6_PS_O0223_29
  READ_FLAG( uiCode, "cross_layer_pic_type_aligned_flag" ); pcVPSVUI->setCrossLayerPicTypeAlignedFlag( uiCode == 1 );
  if ( !pcVPSVUI->getCrossLayerPicTypeAlignedFlag() )
  {  
    READ_FLAG( uiCode, "cross_layer_irap_aligned_flag" ); pcVPSVUI->setCrossLayerIrapAlignedFlag( uiCode == 1 );
  }
#endif
  READ_FLAG( uiCode, "bit_rate_present_vps_flag" ); pcVPSVUI->setBitRatePresentVpsFlag( uiCode == 1 );
  READ_FLAG( uiCode, "pic_rate_present_vps_flag" ); pcVPSVUI->setPicRatePresentVpsFlag( uiCode == 1 );
  if( pcVPSVUI->getBitRatePresentVpsFlag( )  ||  pcVPSVUI->getPicRatePresentVpsFlag( ) )
  {
    for( Int i = 0; i  <=  pcVPS->getVpsNumberLayerSetsMinus1(); i++ )
    {
      for( Int j = 0; j  <=  pcVPS->getMaxTLayers(); j++ ) 
      {
        if( pcVPSVUI->getBitRatePresentVpsFlag( ) )
        {
          READ_FLAG( uiCode, "bit_rate_present_flag" ); pcVPSVUI->setBitRatePresentFlag( i, j, uiCode == 1 );            
        }
        if( pcVPSVUI->getPicRatePresentVpsFlag( )  )
        {
          READ_FLAG( uiCode, "pic_rate_present_flag" ); pcVPSVUI->setPicRatePresentFlag( i, j, uiCode == 1 );
        }
        if( pcVPSVUI->getBitRatePresentFlag( i, j ) )
        {
          READ_CODE( 16, uiCode, "avg_bit_rate" ); pcVPSVUI->setAvgBitRate( i, j, uiCode );
          READ_CODE( 16, uiCode, "max_bit_rate" ); pcVPSVUI->setMaxBitRate( i, j, uiCode );
        }
        if( pcVPSVUI->getPicRatePresentFlag( i, j ) )
        {
          READ_CODE( 2,  uiCode, "constant_pic_rate_idc" ); pcVPSVUI->setConstantPicRateIdc( i, j, uiCode );
          READ_CODE( 16, uiCode, "avg_pic_rate" );          pcVPSVUI->setAvgPicRate( i, j, uiCode );
        }
      }
    }
  }

#if H_MV_6_O0226_37
  READ_FLAG( uiCode, "tiles_not_in_use_flag" ); pcVPSVUI->setTilesNotInUseFlag( uiCode == 1 );
  if( !pcVPSVUI->getTilesNotInUseFlag() ) 
  {      
    for( Int i = 0; i  <=  pcVPS->getMaxLayersMinus1(); i++ )
    {
      READ_FLAG( uiCode, "tiles_in_use_flag[i]" ); pcVPSVUI->setTilesInUseFlag( i, uiCode == 1 );
      if( pcVPSVUI->getTilesInUseFlag( i ) )  
      {
        READ_FLAG( uiCode, "loop_filter_not_across_tiles_flag[i]" ); pcVPSVUI->setLoopFilterNotAcrossTilesFlag( i, uiCode == 1 );
      }
    }  

    for( Int i = 1; i  <=  pcVPS->getMaxLayersMinus1(); i++ )  
    {
      for( Int j = 0; j < pcVPS->getNumDirectRefLayers( pcVPS->getLayerIdInNuh( i ) ) ; j++ )
      {  
        Int layerIdx = pcVPS->getLayerIdInVps(pcVPS->getRefLayerId(pcVPS->getLayerIdInNuh( i ) , j  ));  
        if( pcVPSVUI->getTilesInUseFlag( i )  &&  pcVPSVUI->getTilesInUseFlag( layerIdx ) )  
        {
          READ_FLAG( uiCode, "tile_boundaries_aligned_flag[i][j]" ); pcVPSVUI->setTileBoundariesAlignedFlag( i, j, uiCode == 1 );
        }
      }  
    }
  }  
  
  READ_FLAG( uiCode, "wpp_not_in_use_flag" ); pcVPSVUI->setWppNotInUseFlag( uiCode == 1 );
  
  if( !pcVPSVUI->getWppNotInUseFlag( ))
  {
    for( Int i = 0; i  <=  pcVPS->getMaxLayersMinus1(); i++ )  
    {
      READ_FLAG( uiCode, "wpp_in_use_flag[i]" ); pcVPSVUI->setWppInUseFlag( i, uiCode == 1 );
    }
  }
#else
  for( Int i = 1; i  <=  pcVPS->getMaxLayersMinus1(); i++ )
  {
    for( Int  j = 0; j < pcVPS->getNumDirectRefLayers( pcVPS->getLayerIdInNuh( i ) ); j++ ) 
    {
      READ_FLAG( uiCode, "tile_boundaries_aligned_flag" ); pcVPSVUI->setTileBoundariesAlignedFlag( i, j, uiCode == 1 );
    }
  }
#endif

  READ_FLAG( uiCode, "ilp_restricted_ref_layers_flag" ); pcVPSVUI->setIlpRestrictedRefLayersFlag( uiCode == 1 );

  if( pcVPSVUI->getIlpRestrictedRefLayersFlag( ) )
  {
    for( Int i = 1; i  <=  pcVPS->getMaxLayersMinus1(); i++ )
    {
      for( Int j = 0; j < pcVPS->getNumDirectRefLayers( pcVPS->getLayerIdInNuh( i ) ); j++ )
      {
        READ_UVLC( uiCode, "min_spatial_segment_offset_plus1" ); pcVPSVUI->setMinSpatialSegmentOffsetPlus1( i, j, uiCode );
        if( pcVPSVUI->getMinSpatialSegmentOffsetPlus1( i, j ) > 0 )
        {
          READ_FLAG( uiCode, "ctu_based_offset_enabled_flag" ); pcVPSVUI->setCtuBasedOffsetEnabledFlag( i, j, uiCode == 1 );
          if( pcVPSVUI->getCtuBasedOffsetEnabledFlag( i, j ) )
          {
            READ_UVLC( uiCode, "min_horizontal_ctu_offset_plus1" ); pcVPSVUI->setMinHorizontalCtuOffsetPlus1( i, j, uiCode );
          }
        }
      }
    }
  }

#if H_MV_6_PS_O0118_33
  READ_FLAG( uiCode, "video_signal_info_idx_present_flag" ); pcVPSVUI->setVideoSignalInfoIdxPresentFlag( uiCode == 1 );
  if( pcVPSVUI->getVideoSignalInfoIdxPresentFlag() )
  {
    READ_CODE( 4, uiCode, "vps_num_video_signal_info_minus1" ); pcVPSVUI->setVpsNumVideoSignalInfoMinus1( uiCode );
  }
  else
  {
    pcVPSVUI->setVpsNumVideoSignalInfoMinus1( pcVPS->getMaxLayersMinus1() ); 
  }

  for( Int i = 0; i <= pcVPSVUI->getVpsNumVideoSignalInfoMinus1(); i++ )
  {
    assert( pcVPSVUI->getVideoSignalInfo( i ) == NULL ); 
    TComVideoSignalInfo* curVideoSignalInfo = new TComVideoSignalInfo();     
    parseVideoSignalInfo( curVideoSignalInfo ); 
    pcVPSVUI->setVideoSignalInfo(i, curVideoSignalInfo ); 
  }
  
  if( pcVPSVUI->getVideoSignalInfoIdxPresentFlag() && pcVPSVUI->getVpsNumVideoSignalInfoMinus1() > 0 )
  {
    for( Int i = 1; i <=  pcVPS->getMaxLayersMinus1(); i++ )
    {
      READ_CODE( 4, uiCode, "vps_video_signal_info_idx" ); pcVPSVUI->setVpsVideoSignalInfoIdx( i, uiCode );
      assert( pcVPSVUI->getVpsVideoSignalInfoIdx( i ) >= 0 && pcVPSVUI->getVpsVideoSignalInfoIdx( i ) <= pcVPSVUI->getVpsNumVideoSignalInfoMinus1() );
    }
  }
  else
  {
    for( Int i = 1; i <=  pcVPS->getMaxLayersMinus1(); i++ )
    {
      pcVPSVUI->setVpsVideoSignalInfoIdx( i, pcVPSVUI->getVideoSignalInfoIdxPresentFlag() ? 0 : i ); 
    }
  }
#endif
#if H_MV_6_HRD_O0164_15
  READ_FLAG( uiCode, "vps_vui_bsp_hrd_present_flag" ); pcVPSVUI->setVpsVuiBspHrdPresentFlag( uiCode == 1 );
  if ( pcVPSVUI->getVpsVuiBspHrdPresentFlag( ) )
  {
    parseVpsVuiBspHrdParameters( pcVPS ); 
}
#endif
}

#if H_MV_6_HRD_O0164_15
Void TDecCavlc::parseVpsVuiBspHrdParameters( TComVPS* pcVPS )
{
  assert( pcVPS ); 

  TComVPSVUI* pcVPSVUI = pcVPS->getVPSVUI( ); 

  assert( pcVPSVUI ); 

  TComVpsVuiBspHrdParameters*  vpsVuiBspHrdP = pcVPSVUI->getVpsVuiBspHrdParameters(); 
  
  assert ( vpsVuiBspHrdP );

  UInt uiCode; 
  READ_UVLC( uiCode, "vps_num_bsp_hrd_parameters_minus1" ); vpsVuiBspHrdP->setVpsNumBspHrdParametersMinus1( uiCode );
  for( Int i = 0; i <= vpsVuiBspHrdP->getVpsNumBspHrdParametersMinus1( ); i++ )
  {  
    if( i > 0 )
    {
      READ_FLAG( uiCode, "bsp_cprms_present_flag" ); vpsVuiBspHrdP->setBspCprmsPresentFlag( i, uiCode == 1 );
    }
    TComHRD* hrdParameters = vpsVuiBspHrdP->getHrdParametermeters( i ); 
    parseHrdParameters( hrdParameters, vpsVuiBspHrdP->getBspCprmsPresentFlag( i ), pcVPS->getMaxSubLayersMinus1() );     
  }  
  for( Int h = 1; h <= pcVPS->getVpsNumLayerSetsMinus1(); h++ )
  {  
    READ_UVLC( uiCode, "num_bitstream_partitions" ); vpsVuiBspHrdP->setNumBitstreamPartitions( h, uiCode );
    for( Int i = 0; i < vpsVuiBspHrdP->getNumBitstreamPartitions( h ); i++ )  
    {
      for( Int j = 0; j <= pcVPS->getMaxLayersMinus1(); j++ )  
      {
        if( pcVPS->getLayerIdIncludedFlag( h ,j ) )
        {
          READ_FLAG( uiCode, "layer_in_bsp_flag" ); vpsVuiBspHrdP->setLayerInBspFlag( h, i, j, uiCode == 1 );
        }
        else
        {
          vpsVuiBspHrdP->setLayerInBspFlag( h, i, j, false ); // This inference seems to be missing in spec 
        }
      }
    }
    vpsVuiBspHrdP->checkLayerInBspFlag( pcVPS, h ); 
    
    if( vpsVuiBspHrdP->getNumBitstreamPartitions( h ) )
    {  
      READ_UVLC( uiCode, "num_bsp_sched_combinations" ); vpsVuiBspHrdP->setNumBspSchedCombinations( h, uiCode );
      for( Int i = 0; i < vpsVuiBspHrdP->getNumBspSchedCombinations( h ); i++ )
      {
        for( Int j = 0; j < vpsVuiBspHrdP->getNumBitstreamPartitions( h ); j++ )
        {  
          READ_UVLC( uiCode, "bsp_comb_hrd_idx" ); vpsVuiBspHrdP->setBspCombHrdIdx( h, i, j, uiCode );
          READ_UVLC( uiCode, "bsp_comb_sched_idx" ); vpsVuiBspHrdP->setBspCombSchedIdx( h, i, j, uiCode );
        }  
      }
    }  
  }  
}  
#endif

#if H_MV_6_PS_O0118_33
Void TDecCavlc::parseVideoSignalInfo( TComVideoSignalInfo* pcVideoSignalInfo ) 
{
  UInt uiCode; 
  READ_CODE( 3, uiCode, "video_vps_format" );             pcVideoSignalInfo->setVideoVpsFormat( uiCode );
  READ_FLAG( uiCode, "video_full_range_vps_flag" );       pcVideoSignalInfo->setVideoFullRangeVpsFlag( uiCode == 1 );
  READ_CODE( 8, uiCode, "colour_primaries_vps" );         pcVideoSignalInfo->setColourPrimariesVps( uiCode );
  READ_CODE( 8, uiCode, "transfer_characteristics_vps" ); pcVideoSignalInfo->setTransferCharacteristicsVps( uiCode );
  READ_CODE( 8, uiCode, "matrix_coeffs_vps" );            pcVideoSignalInfo->setMatrixCoeffsVps( uiCode );
}
#endif

#if H_MV_6_HRD_O0217_13
Void TDecCavlc::parseDpbSize( TComVPS* vps )
{
  UInt uiCode; 
  TComDpbSize* dpbSize = vps->getDpbSize(); 
  assert ( dpbSize != 0 ); 

  for( Int i = 1; i < vps->getNumOutputLayerSets(); i++ )
  {  
    READ_FLAG( uiCode, "sub_layer_flag_info_present_flag" ); dpbSize->setSubLayerFlagInfoPresentFlag( i, uiCode == 1 );

    for( Int j = 0; j  <=  vps->getMaxTLayers() - 1 ; j++ )
    {  
      if( j > 0  &&  dpbSize->getSubLayerDpbInfoPresentFlag( i, j )  )  
      {
        READ_FLAG( uiCode, "sub_layer_dpb_info_present_flag" ); dpbSize->setSubLayerDpbInfoPresentFlag( i, j, uiCode == 1 );
      }
      if( dpbSize->getSubLayerDpbInfoPresentFlag( i, j ) )
      {  
        for( Int k = 0; k < vps->getNumSubDpbs( vps->getOutputLayerSetIdxMinus1( i ) + 1 ); k++ )   
        {
          READ_UVLC( uiCode, "max_vps_dec_pic_buffering_minus1" ); dpbSize->setMaxVpsDecPicBufferingMinus1( i, k, j, uiCode );
        }
        READ_UVLC( uiCode, "max_vps_num_reorder_pics" ); dpbSize->setMaxVpsNumReorderPics( i, j, uiCode );
        READ_UVLC( uiCode, "max_vps_latency_increase_plus1" ); dpbSize->setMaxVpsLatencyIncreasePlus1( i, j, uiCode );
      }
      else
      {
        if ( j > 0 )
        {
          for( Int k = 0; k < vps->getNumSubDpbs( vps->getOutputLayerSetIdxMinus1( i ) + 1 ); k++ )   
          {
            dpbSize->setMaxVpsDecPicBufferingMinus1( i, k, j, dpbSize->getMaxVpsDecPicBufferingMinus1( i,k, j - 1 ) );
          }
          dpbSize->setMaxVpsNumReorderPics      ( i, j, dpbSize->getMaxVpsNumReorderPics      ( i, j - 1 ) );
          dpbSize->setMaxVpsLatencyIncreasePlus1( i, j, dpbSize->getMaxVpsLatencyIncreasePlus1( i, j - 1 ) );
        }
      }
    }  
  }  
}
#endif
#endif

#if H_3D
Void TDecCavlc::parseVPSExtension2( TComVPS* pcVPS )
{
  UInt uiCode; 
  for( Int i = 0; i <= pcVPS->getMaxLayersMinus1(); i++ )
  {
#if H_3D_ARP
    pcVPS->setUseAdvRP  ( i, 0 );
    pcVPS->setARPStepNum( i, 1 );
#endif  
#if H_3D_SPIVMP
    pcVPS->setSubPULog2Size(i, 0);
#endif
    if ( i != 0 )
    {
      if( !( pcVPS->getDepthId( i ) == 1 ) )
      {
#if H_3D_IV_MERGE
        READ_FLAG( uiCode, "iv_mv_pred_flag[i]");          pcVPS->setIvMvPredFlag         ( i, uiCode == 1 ? true : false );
#if H_3D_SPIVMP
        READ_UVLC (uiCode, "log2_sub_PU_size_minus2");     pcVPS->setSubPULog2Size(i, uiCode+2); 
#endif
#endif
#if H_3D_ARP
        READ_FLAG( uiCode, "iv_res_pred_flag[i]"  );       pcVPS->setUseAdvRP  ( i, uiCode ); pcVPS->setARPStepNum( i, uiCode ? H_3D_ARP_WFNR : 1 );

#endif
#if H_3D_NBDV_REF
        READ_FLAG( uiCode, "depth_refinement_flag[i]");    pcVPS->setDepthRefinementFlag  ( i, uiCode == 1 ? true : false );
#endif
#if H_3D_VSP
        READ_FLAG( uiCode, "view_synthesis_pred_flag[i]"); pcVPS->setViewSynthesisPredFlag( i, uiCode == 1 ? true : false );
#endif
      }
      else
      {
#if H_3D_IV_MERGE
        if(i!=1)
        {
          READ_FLAG( uiCode, "iv_mv_pred_flag[i]");          pcVPS->setIvMvPredFlag         ( i, uiCode == 1 ? true : false );
        }
#endif
#if H_3D_SPIVMP
        if (i!=1)
        {
          READ_UVLC (uiCode, "log2_sub_PU_size_minus2[i]");     pcVPS->setSubPULog2Size(i, uiCode+2); 
        }
#endif
#if H_3D_IV_MERGE
        READ_FLAG( uiCode, "mpi_flag[i]" );             pcVPS->setMPIFlag( i, uiCode == 1 ? true : false );
#endif
        READ_FLAG( uiCode, "vps_depth_modes_flag[i]" );             pcVPS->setVpsDepthModesFlag( i, uiCode == 1 ? true : false );
        //          READ_FLAG( uiCode, "lim_qt_pred_flag[i]");                  pcVPS->setLimQtPreFlag     ( i, uiCode == 1 ? true : false ); 
#if H_3D_DIM_DLT
#if !DLT_DIFF_CODING_IN_PPS
        if( pcVPS->getVpsDepthModesFlag( i ) )
        {
          READ_FLAG( uiCode, "dlt_flag[i]" );                       pcVPS->setUseDLTFlag( i, uiCode == 1 ? true : false );
        }
        if( pcVPS->getUseDLTFlag( i ) )
        {
          // decode mapping
          UInt uiNumDepthValues;
          // parse number of values in DLT
          READ_UVLC(uiNumDepthValues, "num_depth_values_in_dlt[i]");

          // parse actual DLT values
          Int* aiIdx2DepthValue = (Int*) calloc(uiNumDepthValues, sizeof(Int));
          for(Int d=0; d<uiNumDepthValues; d++)
          {
            READ_UVLC(uiCode, "dlt_depth_value[i][d]");
            aiIdx2DepthValue[d] = (Int)uiCode;
          }

          pcVPS->setDepthLUTs(i, aiIdx2DepthValue, uiNumDepthValues);

          // clean memory
          free(aiIdx2DepthValue);
        }
#endif
#endif
#if H_3D_INTER_SDC
            READ_FLAG( uiCode, "depth_inter_SDC_flag" );              pcVPS->setInterSDCFlag( i, uiCode ? true : false );
#endif
      }
    }
  }
#if CAM_HLS_F0136_F0045_F0082
  UInt uiCamParPrecision = 0; 
  Bool bCamParSlice      = false; 
  Bool bCamParPresentFlag = false;

  READ_UVLC( uiCamParPrecision, "cp_precision" );
  for (UInt viewIndex=0; viewIndex<pcVPS->getNumViews(); viewIndex++)
  {
    READ_FLAG( uiCode, "cp_present_flag[i]" );                  bCamParPresentFlag = ( uiCode == 1);
    if ( bCamParPresentFlag )
    {
      READ_FLAG( uiCode, "cp_in_slice_segment_header_flag[i]" );          bCamParSlice = ( uiCode == 1);
      if ( !bCamParSlice )
      {
        for( UInt uiBaseIndex = 0; uiBaseIndex < viewIndex; uiBaseIndex++ )
        {
          Int iCode; 
          READ_SVLC( iCode, "vps_cp_scale" );                m_aaiTempScale  [ uiBaseIndex ][ viewIndex ]   = iCode;
          READ_SVLC( iCode, "vps_cp_off" );                  m_aaiTempOffset [ uiBaseIndex ][ viewIndex ]   = iCode;
          READ_SVLC( iCode, "vps_cp_inv_scale_plus_scale" ); m_aaiTempScale  [ viewIndex   ][ uiBaseIndex ] = iCode - m_aaiTempScale [ uiBaseIndex ][ viewIndex ];
          READ_SVLC( iCode, "vps_cp_inv_off_plus_off" );     m_aaiTempOffset [ viewIndex   ][ uiBaseIndex ] = iCode - m_aaiTempOffset[ uiBaseIndex ][ viewIndex ];
        }
      }
      pcVPS->initCamParaVPS( viewIndex, bCamParPresentFlag, uiCamParPrecision, bCamParSlice, m_aaiTempScale, m_aaiTempOffset ); 
    }
  }
#endif
  READ_FLAG( uiCode, "iv_mv_scaling_flag");                       pcVPS->setIvMvScalingFlag( uiCode == 1 ? true : false ); 
}
#endif

Void TDecCavlc::parseSliceHeader (TComSlice*& rpcSlice, ParameterSetManagerDecoder *parameterSetManager)
{
  UInt  uiCode;
  Int   iCode;

#if ENC_DEC_TRACE
  xTraceSliceHeader(rpcSlice);
#endif
  TComPPS* pps = NULL;
  TComSPS* sps = NULL;
#if H_MV
  TComVPS* vps = NULL;
#endif

  UInt firstSliceSegmentInPic;
  READ_FLAG( firstSliceSegmentInPic, "first_slice_segment_in_pic_flag" );
  if( rpcSlice->getRapPicFlag())
  { 
    READ_FLAG( uiCode, "no_output_of_prior_pics_flag" );  //ignored
  }
  READ_UVLC (    uiCode, "slice_pic_parameter_set_id" );  rpcSlice->setPPSId(uiCode);
  pps = parameterSetManager->getPrefetchedPPS(uiCode);
  //!KS: need to add error handling code here, if PPS is not available
  assert(pps!=0);
  sps = parameterSetManager->getPrefetchedSPS(pps->getSPSId());
  //!KS: need to add error handling code here, if SPS is not available
  assert(sps!=0);
#if H_MV
  vps = parameterSetManager->getPrefetchedVPS(sps->getVPSId());
  assert( vps != NULL );
  
  sps->inferRepFormat  ( vps , rpcSlice->getLayerId() ); 
  sps->inferScalingList( parameterSetManager->getActiveSPS( sps->getSpsScalingListRefLayerId() ) );   
#if H_MV_6_PS_O0118_33
  if ( sps->getVuiParametersPresentFlag() )
  {
    sps->getVuiParameters()->inferVideoSignalInfo( vps, rpcSlice->getLayerId() ); 
  }
#endif
  rpcSlice->setVPS(vps);      
  rpcSlice->setViewId   ( vps->getViewId   ( rpcSlice->getLayerId() )      );
  rpcSlice->setViewIndex( vps->getViewIndex( rpcSlice->getLayerId() )      );  
#if H_3D  
  rpcSlice->setIsDepth  ( vps->getDepthId  ( rpcSlice->getLayerId() ) == 1 );
#endif
#endif
  rpcSlice->setSPS(sps);
  rpcSlice->setPPS(pps);
  if( pps->getDependentSliceSegmentsEnabledFlag() && ( !firstSliceSegmentInPic ))
  {
    READ_FLAG( uiCode, "dependent_slice_segment_flag" );       rpcSlice->setDependentSliceSegmentFlag(uiCode ? true : false);
  }
  else
  {
    rpcSlice->setDependentSliceSegmentFlag(false);
  }
  Int numCTUs = ((sps->getPicWidthInLumaSamples()+sps->getMaxCUWidth()-1)/sps->getMaxCUWidth())*((sps->getPicHeightInLumaSamples()+sps->getMaxCUHeight()-1)/sps->getMaxCUHeight());
  Int maxParts = (1<<(sps->getMaxCUDepth()<<1));
  UInt sliceSegmentAddress = 0;
  Int bitsSliceSegmentAddress = 0;
  while(numCTUs>(1<<bitsSliceSegmentAddress))
  {
    bitsSliceSegmentAddress++;
  }

  if(!firstSliceSegmentInPic)
  {
    READ_CODE( bitsSliceSegmentAddress, sliceSegmentAddress, "slice_segment_address" );
  }
  //set uiCode to equal slice start address (or dependent slice start address)
  Int startCuAddress = maxParts*sliceSegmentAddress;
  rpcSlice->setSliceSegmentCurStartCUAddr( startCuAddress );
  rpcSlice->setSliceSegmentCurEndCUAddr(numCTUs*maxParts);

  if (rpcSlice->getDependentSliceSegmentFlag())
  {
    rpcSlice->setNextSlice          ( false );
    rpcSlice->setNextSliceSegment ( true  );
  }
  else
  {
    rpcSlice->setNextSlice          ( true  );
    rpcSlice->setNextSliceSegment ( false );

    rpcSlice->setSliceCurStartCUAddr(startCuAddress);
    rpcSlice->setSliceCurEndCUAddr(numCTUs*maxParts);
  }
  
  if(!rpcSlice->getDependentSliceSegmentFlag())
  {
#if H_MV    
    Int esb = 0; //Don't use i, otherwise will shadow something below
#if !H_MV_6_RALS_O0149_11
    if ( rpcSlice->getPPS()->getNumExtraSliceHeaderBits() > esb )
    {
      esb++; 
      READ_FLAG( uiCode, "poc_reset_flag" ); rpcSlice->setPocResetFlag( uiCode == 1 );
    }
#endif

    if ( rpcSlice->getPPS()->getNumExtraSliceHeaderBits() > esb )
    {
      esb++; 
      READ_FLAG( uiCode, "discardable_flag" ); rpcSlice->setDiscardableFlag( uiCode == 1 );
    }

#if H_MV_6_RALS_O0149_11
    if ( rpcSlice->getPPS()->getNumExtraSliceHeaderBits() > esb )
    {
      esb++; 
      READ_FLAG( uiCode, "cross_layer_bla_flag" ); rpcSlice->setCrossLayerBlaFlag( uiCode == 1 );
    }
    rpcSlice->checkCrossLayerBlaFlag( ); 

    if ( rpcSlice->getPPS()->getNumExtraSliceHeaderBits() > esb )
    {
      esb++; 
      READ_FLAG( uiCode, "poc_reset_flag" ); rpcSlice->setPocResetFlag( uiCode == 1 );
    }
#endif

    for (; esb < rpcSlice->getPPS()->getNumExtraSliceHeaderBits(); esb++)    
#else
    for (Int i = 0; i < rpcSlice->getPPS()->getNumExtraSliceHeaderBits(); i++)
#endif     
    {
      READ_FLAG(uiCode, "slice_reserved_undetermined_flag[]"); // ignored
    }

    READ_UVLC (    uiCode, "slice_type" );            rpcSlice->setSliceType((SliceType)uiCode);
    if( pps->getOutputFlagPresentFlag() )
    {
      READ_FLAG( uiCode, "pic_output_flag" );    rpcSlice->setPicOutputFlag( uiCode ? true : false );
    }
    else
    {
      rpcSlice->setPicOutputFlag( true );
    }
    // in the first version chroma_format_idc is equal to one, thus colour_plane_id will not be present
    assert (sps->getChromaFormatIdc() == 1 );
    // if( separate_colour_plane_flag  ==  1 )
    //   colour_plane_id                                      u(2)


#if H_MV_6_POC_31_35_38
    UInt slicePicOrderCntLsb = 0;
    Int iPOClsb = slicePicOrderCntLsb;  // Needed later
    if ( (rpcSlice->getLayerId() > 0 && !vps->getPocLsbNotPresentFlag( rpcSlice->getLayerIdInVps())) || !rpcSlice->getIdrPicFlag() )
    {
      READ_CODE(sps->getBitsForPOC(), slicePicOrderCntLsb, "slice_pic_order_cnt_lsb");        
    }    

    Bool picOrderCntMSBZeroFlag = false;     

    // as in HM code. However are all cases for IRAP picture with NoRaslOutputFlag equal to 1 covered??
    picOrderCntMSBZeroFlag = picOrderCntMSBZeroFlag || ( rpcSlice->getNalUnitType() == NAL_UNIT_CODED_SLICE_BLA_W_LP   ); 
    picOrderCntMSBZeroFlag = picOrderCntMSBZeroFlag || ( rpcSlice->getNalUnitType() == NAL_UNIT_CODED_SLICE_BLA_W_RADL ); 
    picOrderCntMSBZeroFlag = picOrderCntMSBZeroFlag || ( rpcSlice->getNalUnitType() == NAL_UNIT_CODED_SLICE_BLA_N_LP   ); 
    picOrderCntMSBZeroFlag = picOrderCntMSBZeroFlag ||   rpcSlice->getIdrPicFlag(); 

    // TBD picOrderCntMSBZeroFlag = picOrderCntMSBZeroFlag || ( rpcSlice->getLayerId() > 0 &&   !rpcSlice->getFirstPicInLayerDecodedFlag() ); 

    Int picOrderCntMSB = 0; 

    if ( !picOrderCntMSBZeroFlag )
    {
      Int prevPicOrderCnt    = rpcSlice->getPrevTid0POC();
      Int maxPicOrderCntLsb  = 1 << sps->getBitsForPOC();
      Int prevPicOrderCntLsb = prevPicOrderCnt & (maxPicOrderCntLsb - 1);
      Int prevPicOrderCntMsb = prevPicOrderCnt - prevPicOrderCntLsb;
            
      if( ( slicePicOrderCntLsb  <  prevPicOrderCntLsb ) && ( ( prevPicOrderCntLsb - slicePicOrderCntLsb )  >=  ( maxPicOrderCntLsb / 2 ) ) )
      {
        picOrderCntMSB = prevPicOrderCntMsb + maxPicOrderCntLsb;
      }
      else if( (slicePicOrderCntLsb  >  prevPicOrderCntLsb )  && ( (slicePicOrderCntLsb - prevPicOrderCntLsb )  >  ( maxPicOrderCntLsb / 2 ) ) ) 
      {
        picOrderCntMSB = prevPicOrderCntMsb - maxPicOrderCntLsb;
      }
      else
      {
        picOrderCntMSB = prevPicOrderCntMsb;
      }   
    }
      
    rpcSlice->setPOC( picOrderCntMSB + slicePicOrderCntLsb );
    if ( rpcSlice->getPocResetFlag() )  
    {
      rpcSlice->setPocBeforeReset   ( rpcSlice->getPOC() ); 
      rpcSlice->setPOC              ( 0 );
    }      
#endif

    if( rpcSlice->getIdrPicFlag() )
    {
#if !H_MV_6_POC_31_35_38
      rpcSlice->setPOC(0);
#endif
      TComReferencePictureSet* rps = rpcSlice->getLocalRPS();
      rps->setNumberOfNegativePictures(0);
      rps->setNumberOfPositivePictures(0);
      rps->setNumberOfLongtermPictures(0);
      rps->setNumberOfPictures(0);
      rpcSlice->setRPS(rps);
#if H_MV
      rpcSlice->setEnableTMVPFlag(false);
#endif
    }
    else
    {
#if !H_MV_6_POC_31_35_38
      READ_CODE(sps->getBitsForPOC(), uiCode, "pic_order_cnt_lsb");  
      Int iPOClsb = uiCode;
      Int iPrevPOC = rpcSlice->getPrevTid0POC();
      Int iMaxPOClsb = 1<< sps->getBitsForPOC();
      Int iPrevPOClsb = iPrevPOC & (iMaxPOClsb - 1);
      Int iPrevPOCmsb = iPrevPOC-iPrevPOClsb;
      Int iPOCmsb;
      if( ( iPOClsb  <  iPrevPOClsb ) && ( ( iPrevPOClsb - iPOClsb )  >=  ( iMaxPOClsb / 2 ) ) )
      {
        iPOCmsb = iPrevPOCmsb + iMaxPOClsb;
      }
      else if( (iPOClsb  >  iPrevPOClsb )  && ( (iPOClsb - iPrevPOClsb )  >  ( iMaxPOClsb / 2 ) ) ) 
      {
        iPOCmsb = iPrevPOCmsb - iMaxPOClsb;
      }
      else
      {
        iPOCmsb = iPrevPOCmsb;
      }
      if ( rpcSlice->getNalUnitType() == NAL_UNIT_CODED_SLICE_BLA_W_LP
        || rpcSlice->getNalUnitType() == NAL_UNIT_CODED_SLICE_BLA_W_RADL
        || rpcSlice->getNalUnitType() == NAL_UNIT_CODED_SLICE_BLA_N_LP )
      {
        // For BLA picture types, POCmsb is set to 0.
        iPOCmsb = 0;
      }
      rpcSlice->setPOC              (iPOCmsb+iPOClsb);
#if H_MV
      if ( rpcSlice->getPocResetFlag() )  
      {
        rpcSlice->setPocBeforeReset   ( rpcSlice->getPOC() ); 
        rpcSlice->setPOC              ( 0 );

      }      
#endif
#endif
      TComReferencePictureSet* rps;
      rps = rpcSlice->getLocalRPS();
      rpcSlice->setRPS(rps);
      READ_FLAG( uiCode, "short_term_ref_pic_set_sps_flag" );
      if(uiCode == 0) // use short-term reference picture set explicitly signalled in slice header
      {
        parseShortTermRefPicSet(sps,rps, sps->getRPSList()->getNumberOfReferencePictureSets());
      }
      else // use reference to short-term reference picture set in PPS
      {
        Int numBits = 0;
        while ((1 << numBits) < rpcSlice->getSPS()->getRPSList()->getNumberOfReferencePictureSets())
        {
          numBits++;
        }
        if (numBits > 0)
        {
          READ_CODE( numBits, uiCode, "short_term_ref_pic_set_idx");
        }
        else
        {
          uiCode = 0;
        }
        *rps = *(sps->getRPSList()->getReferencePictureSet(uiCode));
      }
      if(sps->getLongTermRefsPresent())
      {
        Int offset = rps->getNumberOfNegativePictures()+rps->getNumberOfPositivePictures();
        UInt numOfLtrp = 0;
        UInt numLtrpInSPS = 0;
        if (rpcSlice->getSPS()->getNumLongTermRefPicSPS() > 0)
        {
          READ_UVLC( uiCode, "num_long_term_sps");
          numLtrpInSPS = uiCode;
          numOfLtrp += numLtrpInSPS;
          rps->setNumberOfLongtermPictures(numOfLtrp);
        }
        Int bitsForLtrpInSPS = 0;
        while (rpcSlice->getSPS()->getNumLongTermRefPicSPS() > (1 << bitsForLtrpInSPS))
        {
          bitsForLtrpInSPS++;
        }
        READ_UVLC( uiCode, "num_long_term_pics");             rps->setNumberOfLongtermPictures(uiCode);
        numOfLtrp += uiCode;
        rps->setNumberOfLongtermPictures(numOfLtrp);
        Int maxPicOrderCntLSB = 1 << rpcSlice->getSPS()->getBitsForPOC();
        Int prevDeltaMSB = 0, deltaPocMSBCycleLT = 0;;
        for(Int j=offset+rps->getNumberOfLongtermPictures()-1, k = 0; k < numOfLtrp; j--, k++)
        {
          Int pocLsbLt;
          if (k < numLtrpInSPS)
          {
            uiCode = 0;
            if (bitsForLtrpInSPS > 0)
            {
              READ_CODE(bitsForLtrpInSPS, uiCode, "lt_idx_sps[i]");
            }
            Int usedByCurrFromSPS=rpcSlice->getSPS()->getUsedByCurrPicLtSPSFlag(uiCode);

            pocLsbLt = rpcSlice->getSPS()->getLtRefPicPocLsbSps(uiCode);
            rps->setUsed(j,usedByCurrFromSPS);
          }
          else
          {
            READ_CODE(rpcSlice->getSPS()->getBitsForPOC(), uiCode, "poc_lsb_lt"); pocLsbLt= uiCode;
            READ_FLAG( uiCode, "used_by_curr_pic_lt_flag");     rps->setUsed(j,uiCode);
          }
          READ_FLAG(uiCode,"delta_poc_msb_present_flag");
          Bool mSBPresentFlag = uiCode ? true : false;
          if(mSBPresentFlag)                  
          {
            READ_UVLC( uiCode, "delta_poc_msb_cycle_lt[i]" );
            Bool deltaFlag = false;
            //            First LTRP                               || First LTRP from SH
            if( (j == offset+rps->getNumberOfLongtermPictures()-1) || (j == offset+(numOfLtrp-numLtrpInSPS)-1) )
            {
              deltaFlag = true;
            }
            if(deltaFlag)
            {
              deltaPocMSBCycleLT = uiCode;
            }
            else
            {
              deltaPocMSBCycleLT = uiCode + prevDeltaMSB;              
            }

            Int pocLTCurr = rpcSlice->getPOC() - deltaPocMSBCycleLT * maxPicOrderCntLSB 
                                        - iPOClsb + pocLsbLt;
            rps->setPOC     (j, pocLTCurr); 
            rps->setDeltaPOC(j, - rpcSlice->getPOC() + pocLTCurr);
            rps->setCheckLTMSBPresent(j,true);  
          }
          else
          {
            rps->setPOC     (j, pocLsbLt);
            rps->setDeltaPOC(j, - rpcSlice->getPOC() + pocLsbLt);
            rps->setCheckLTMSBPresent(j,false);  
            
            // reset deltaPocMSBCycleLT for first LTRP from slice header if MSB not present
            if( j == offset+(numOfLtrp-numLtrpInSPS)-1 )
            {
              deltaPocMSBCycleLT = 0;
            }
          }
          prevDeltaMSB = deltaPocMSBCycleLT;
        }
        offset += rps->getNumberOfLongtermPictures();
        rps->setNumberOfPictures(offset);        
      }  
      if ( rpcSlice->getNalUnitType() == NAL_UNIT_CODED_SLICE_BLA_W_LP
        || rpcSlice->getNalUnitType() == NAL_UNIT_CODED_SLICE_BLA_W_RADL
        || rpcSlice->getNalUnitType() == NAL_UNIT_CODED_SLICE_BLA_N_LP )
      {
        // In the case of BLA picture types, rps data is read from slice header but ignored
        rps = rpcSlice->getLocalRPS();
        rps->setNumberOfNegativePictures(0);
        rps->setNumberOfPositivePictures(0);
        rps->setNumberOfLongtermPictures(0);
        rps->setNumberOfPictures(0);
        rpcSlice->setRPS(rps);
      }
      if (rpcSlice->getSPS()->getTMVPFlagsPresent())
      {
        READ_FLAG( uiCode, "slice_temporal_mvp_enable_flag" );
        rpcSlice->setEnableTMVPFlag( uiCode == 1 ? true : false ); 
      }
      else
      {
        rpcSlice->setEnableTMVPFlag(false);
      }
    }
#if H_MV
#if H_MV_6_ILDDS_ILREFPICS_27_34
    Bool interLayerPredLayerIdcPresentFlag = false; 
#endif
    Int layerId       = rpcSlice->getLayerId(); 
    if( rpcSlice->getLayerId() > 0 && !vps->getAllRefLayersActiveFlag() && vps->getNumDirectRefLayers( layerId ) > 0 )
    {   
      READ_FLAG( uiCode, "inter_layer_pred_enabled_flag" ); rpcSlice->setInterLayerPredEnabledFlag( uiCode == 1 );
      if( rpcSlice->getInterLayerPredEnabledFlag() && vps->getNumDirectRefLayers( layerId ) > 1 )
      {            
        if( !vps->getMaxOneActiveRefLayerFlag())  
        {
          READ_CODE( rpcSlice->getNumInterLayerRefPicsMinus1Len( ), uiCode, "num_inter_layer_ref_pics_minus1" ); rpcSlice->setNumInterLayerRefPicsMinus1( uiCode );
        }
        if ( rpcSlice->getNumActiveRefLayerPics() != vps->getNumDirectRefLayers( layerId ) )
        {
#if H_MV_6_ILDDS_ILREFPICS_27_34
          interLayerPredLayerIdcPresentFlag = true; 
#endif
          for( Int idx = 0; idx < rpcSlice->getNumActiveRefLayerPics(); idx++ )   
          {
            READ_CODE( rpcSlice->getInterLayerPredLayerIdcLen( ), uiCode, "inter_layer_pred_layer_idc" ); rpcSlice->setInterLayerPredLayerIdc( idx, uiCode );
          }
        }
      }  
    }
#if H_MV_6_ILDDS_ILREFPICS_27_34
    if ( !interLayerPredLayerIdcPresentFlag )
    {
      for( Int i = 0; i < rpcSlice->getNumActiveRefLayerPics(); i++ )   
      {
        rpcSlice->setInterLayerPredLayerIdc( i, rpcSlice->getRefLayerPicIdc( i ) );
      }
    }
#endif
#endif
    if(sps->getUseSAO())
    {
      READ_FLAG(uiCode, "slice_sao_luma_flag");  rpcSlice->setSaoEnabledFlag((Bool)uiCode);
      READ_FLAG(uiCode, "slice_sao_chroma_flag");  rpcSlice->setSaoEnabledFlagChroma((Bool)uiCode);
    }

    if (rpcSlice->getIdrPicFlag())
    {
      rpcSlice->setEnableTMVPFlag(false);
    }
    if (!rpcSlice->isIntra())
    {

      READ_FLAG( uiCode, "num_ref_idx_active_override_flag");
      if (uiCode)
      {
        READ_UVLC (uiCode, "num_ref_idx_l0_active_minus1" );  rpcSlice->setNumRefIdx( REF_PIC_LIST_0, uiCode + 1 );
        if (rpcSlice->isInterB())
        {
          READ_UVLC (uiCode, "num_ref_idx_l1_active_minus1" );  rpcSlice->setNumRefIdx( REF_PIC_LIST_1, uiCode + 1 );
        }
        else
        {
          rpcSlice->setNumRefIdx(REF_PIC_LIST_1, 0);
        }
      }
      else
      {
        rpcSlice->setNumRefIdx(REF_PIC_LIST_0, rpcSlice->getPPS()->getNumRefIdxL0DefaultActive());
        if (rpcSlice->isInterB())
        {
          rpcSlice->setNumRefIdx(REF_PIC_LIST_1, rpcSlice->getPPS()->getNumRefIdxL1DefaultActive());
        }
        else
        {
          rpcSlice->setNumRefIdx(REF_PIC_LIST_1,0);
        }
      }
    }
    // }
    TComRefPicListModification* refPicListModification = rpcSlice->getRefPicListModification();
    if(!rpcSlice->isIntra())
    {
      if( !rpcSlice->getPPS()->getListsModificationPresentFlag() || rpcSlice->getNumRpsCurrTempList() <= 1 )
      {
        refPicListModification->setRefPicListModificationFlagL0( 0 );
      }
      else
      {
        READ_FLAG( uiCode, "ref_pic_list_modification_flag_l0" ); refPicListModification->setRefPicListModificationFlagL0( uiCode ? 1 : 0 );
      }

      if(refPicListModification->getRefPicListModificationFlagL0())
      { 
        uiCode = 0;
        Int i = 0;
        Int numRpsCurrTempList0 = rpcSlice->getNumRpsCurrTempList();
        if ( numRpsCurrTempList0 > 1 )
        {
          Int length = 1;
          numRpsCurrTempList0 --;
          while ( numRpsCurrTempList0 >>= 1) 
          {
            length ++;
          }
          for (i = 0; i < rpcSlice->getNumRefIdx(REF_PIC_LIST_0); i ++)
          {
            READ_CODE( length, uiCode, "list_entry_l0" );
            refPicListModification->setRefPicSetIdxL0(i, uiCode );
          }
        }
        else
        {
          for (i = 0; i < rpcSlice->getNumRefIdx(REF_PIC_LIST_0); i ++)
          {
            refPicListModification->setRefPicSetIdxL0(i, 0 );
          }
        }
      }
    }
    else
    {
      refPicListModification->setRefPicListModificationFlagL0(0);
    }
    if(rpcSlice->isInterB())
    {
      if( !rpcSlice->getPPS()->getListsModificationPresentFlag() || rpcSlice->getNumRpsCurrTempList() <= 1 )
      {
        refPicListModification->setRefPicListModificationFlagL1( 0 );
      }
      else
      {
        READ_FLAG( uiCode, "ref_pic_list_modification_flag_l1" ); refPicListModification->setRefPicListModificationFlagL1( uiCode ? 1 : 0 );
      }
      if(refPicListModification->getRefPicListModificationFlagL1())
      {
        uiCode = 0;
        Int i = 0;
        Int numRpsCurrTempList1 = rpcSlice->getNumRpsCurrTempList();
        if ( numRpsCurrTempList1 > 1 )
        {
          Int length = 1;
          numRpsCurrTempList1 --;
          while ( numRpsCurrTempList1 >>= 1) 
          {
            length ++;
          }
          for (i = 0; i < rpcSlice->getNumRefIdx(REF_PIC_LIST_1); i ++)
          {
            READ_CODE( length, uiCode, "list_entry_l1" );
            refPicListModification->setRefPicSetIdxL1(i, uiCode );
          }
        }
        else
        {
          for (i = 0; i < rpcSlice->getNumRefIdx(REF_PIC_LIST_1); i ++)
          {
            refPicListModification->setRefPicSetIdxL1(i, 0 );
          }
        }
      }
    }  
    else
    {
      refPicListModification->setRefPicListModificationFlagL1(0);
    }
    if (rpcSlice->isInterB())
    {
      READ_FLAG( uiCode, "mvd_l1_zero_flag" );       rpcSlice->setMvdL1ZeroFlag( (uiCode ? true : false) );
    }

    rpcSlice->setCabacInitFlag( false ); // default
    if(pps->getCabacInitPresentFlag() && !rpcSlice->isIntra())
    {
      READ_FLAG(uiCode, "cabac_init_flag");
      rpcSlice->setCabacInitFlag( uiCode ? true : false );
    }

    if ( rpcSlice->getEnableTMVPFlag() )
    {
      if ( rpcSlice->getSliceType() == B_SLICE )
      {
        READ_FLAG( uiCode, "collocated_from_l0_flag" );
        rpcSlice->setColFromL0Flag(uiCode);
      }
      else
      {
        rpcSlice->setColFromL0Flag( 1 );
      }

      if ( rpcSlice->getSliceType() != I_SLICE &&
          ((rpcSlice->getColFromL0Flag() == 1 && rpcSlice->getNumRefIdx(REF_PIC_LIST_0) > 1)||
           (rpcSlice->getColFromL0Flag() == 0 && rpcSlice->getNumRefIdx(REF_PIC_LIST_1) > 1)))
      {
        READ_UVLC( uiCode, "collocated_ref_idx" );
        rpcSlice->setColRefIdx(uiCode);
      }
      else
      {
        rpcSlice->setColRefIdx(0);
      }
    }
    if ( (pps->getUseWP() && rpcSlice->getSliceType()==P_SLICE) || (pps->getWPBiPred() && rpcSlice->getSliceType()==B_SLICE) )
    {
      xParsePredWeightTable(rpcSlice);
      rpcSlice->initWpScaling();
    }
#if H_3D_IC
    else if( rpcSlice->getViewIndex() && ( rpcSlice->getSliceType() == P_SLICE || rpcSlice->getSliceType() == B_SLICE ) && !rpcSlice->getIsDepth())
    {
      UInt uiCodeTmp = 0;

      READ_FLAG ( uiCodeTmp, "slice_ic_enable_flag" );
      rpcSlice->setApplyIC( uiCodeTmp );

      if ( uiCodeTmp )
      {
        READ_FLAG ( uiCodeTmp, "ic_skip_mergeidx0" );
        rpcSlice->setIcSkipParseFlag( uiCodeTmp );
      }
    }
#endif
    if (!rpcSlice->isIntra())
    {
      READ_UVLC( uiCode, "five_minus_max_num_merge_cand");
#if H_3D_IV_MERGE
      if(rpcSlice->getIsDepth())
      {
        Bool bMPIFlag = rpcSlice->getVPS()->getMPIFlag( rpcSlice->getLayerIdInVps() ) ;
        Bool ivMvPredFlag = rpcSlice->getVPS()->getIvMvPredFlag( rpcSlice->getLayerIdInVps() ) ;
        rpcSlice->setMaxNumMergeCand(( ( bMPIFlag || ivMvPredFlag ) ? MRG_MAX_NUM_CANDS_MEM : MRG_MAX_NUM_CANDS) - uiCode);
      }
      else
      {
        Bool ivMvPredFlag = rpcSlice->getVPS()->getIvMvPredFlag( rpcSlice->getLayerIdInVps() ) ;
        rpcSlice->setMaxNumMergeCand(( ivMvPredFlag ? MRG_MAX_NUM_CANDS_MEM : MRG_MAX_NUM_CANDS) - uiCode);
      }

#else
      rpcSlice->setMaxNumMergeCand(MRG_MAX_NUM_CANDS - uiCode);
#endif
    }

    READ_SVLC( iCode, "slice_qp_delta" );
    rpcSlice->setSliceQp (26 + pps->getPicInitQPMinus26() + iCode);

    assert( rpcSlice->getSliceQp() >= -sps->getQpBDOffsetY() );
    assert( rpcSlice->getSliceQp() <=  51 );

    if (rpcSlice->getPPS()->getSliceChromaQpFlag())
    {
      READ_SVLC( iCode, "slice_qp_delta_cb" );
      rpcSlice->setSliceQpDeltaCb( iCode );
      assert( rpcSlice->getSliceQpDeltaCb() >= -12 );
      assert( rpcSlice->getSliceQpDeltaCb() <=  12 );
      assert( (rpcSlice->getPPS()->getChromaCbQpOffset() + rpcSlice->getSliceQpDeltaCb()) >= -12 );
      assert( (rpcSlice->getPPS()->getChromaCbQpOffset() + rpcSlice->getSliceQpDeltaCb()) <=  12 );

      READ_SVLC( iCode, "slice_qp_delta_cr" );
      rpcSlice->setSliceQpDeltaCr( iCode );
      assert( rpcSlice->getSliceQpDeltaCr() >= -12 );
      assert( rpcSlice->getSliceQpDeltaCr() <=  12 );
      assert( (rpcSlice->getPPS()->getChromaCrQpOffset() + rpcSlice->getSliceQpDeltaCr()) >= -12 );
      assert( (rpcSlice->getPPS()->getChromaCrQpOffset() + rpcSlice->getSliceQpDeltaCr()) <=  12 );
    }

    if (rpcSlice->getPPS()->getDeblockingFilterControlPresentFlag())
    {
      if(rpcSlice->getPPS()->getDeblockingFilterOverrideEnabledFlag())
      {
        READ_FLAG ( uiCode, "deblocking_filter_override_flag" );        rpcSlice->setDeblockingFilterOverrideFlag(uiCode ? true : false);
      }
      else
      {  
        rpcSlice->setDeblockingFilterOverrideFlag(0);
      }
      if(rpcSlice->getDeblockingFilterOverrideFlag())
      {
        READ_FLAG ( uiCode, "slice_disable_deblocking_filter_flag" );   rpcSlice->setDeblockingFilterDisable(uiCode ? 1 : 0);
        if(!rpcSlice->getDeblockingFilterDisable())
        {
          READ_SVLC( iCode, "slice_beta_offset_div2" );                       rpcSlice->setDeblockingFilterBetaOffsetDiv2(iCode);
          assert(rpcSlice->getDeblockingFilterBetaOffsetDiv2() >= -6 &&
                 rpcSlice->getDeblockingFilterBetaOffsetDiv2() <=  6);
          READ_SVLC( iCode, "slice_tc_offset_div2" );                         rpcSlice->setDeblockingFilterTcOffsetDiv2(iCode);
          assert(rpcSlice->getDeblockingFilterTcOffsetDiv2() >= -6 &&
                 rpcSlice->getDeblockingFilterTcOffsetDiv2() <=  6);
        }
      }
      else
      {
        rpcSlice->setDeblockingFilterDisable   ( rpcSlice->getPPS()->getPicDisableDeblockingFilterFlag() );
        rpcSlice->setDeblockingFilterBetaOffsetDiv2( rpcSlice->getPPS()->getDeblockingFilterBetaOffsetDiv2() );
        rpcSlice->setDeblockingFilterTcOffsetDiv2  ( rpcSlice->getPPS()->getDeblockingFilterTcOffsetDiv2() );
      }
    }
    else
    {  
      rpcSlice->setDeblockingFilterDisable       ( false );
      rpcSlice->setDeblockingFilterBetaOffsetDiv2( 0 );
      rpcSlice->setDeblockingFilterTcOffsetDiv2  ( 0 );
    }

    Bool isSAOEnabled = (!rpcSlice->getSPS()->getUseSAO())?(false):(rpcSlice->getSaoEnabledFlag()||rpcSlice->getSaoEnabledFlagChroma());
    Bool isDBFEnabled = (!rpcSlice->getDeblockingFilterDisable());

    if(rpcSlice->getPPS()->getLoopFilterAcrossSlicesEnabledFlag() && ( isSAOEnabled || isDBFEnabled ))
    {
      READ_FLAG( uiCode, "slice_loop_filter_across_slices_enabled_flag");
    }
    else
    {
      uiCode = rpcSlice->getPPS()->getLoopFilterAcrossSlicesEnabledFlag()?1:0;
    }
    rpcSlice->setLFCrossSliceBoundaryFlag( (uiCode==1)?true:false);

  }
  
    UInt *entryPointOffset          = NULL;
    UInt numEntryPointOffsets, offsetLenMinus1;
  if( pps->getTilesEnabledFlag() || pps->getEntropyCodingSyncEnabledFlag() )
  {
    READ_UVLC(numEntryPointOffsets, "num_entry_point_offsets"); rpcSlice->setNumEntryPointOffsets ( numEntryPointOffsets );
    if (numEntryPointOffsets>0)
    {
      READ_UVLC(offsetLenMinus1, "offset_len_minus1");
    }
    entryPointOffset = new UInt[numEntryPointOffsets];
    for (UInt idx=0; idx<numEntryPointOffsets; idx++)
    {
      READ_CODE(offsetLenMinus1+1, uiCode, "entry_point_offset_minus1");
      entryPointOffset[ idx ] = uiCode + 1;
    }
  }
  else
  {
    rpcSlice->setNumEntryPointOffsets ( 0 );
  }

#if CAM_HLS_F0044
#if CAM_HLS_F0136_F0045_F0082
  if( rpcSlice->getVPS()->hasCamParInSliceHeader( rpcSlice->getViewIndex() )  && !rpcSlice->getIsDepth() )
#else
  if( rpcSlice->getSPS()->hasCamParInSliceHeader() )
#endif
  {
    UInt uiViewIndex = rpcSlice->getViewIndex();
    for( UInt uiBaseIndex = 0; uiBaseIndex < uiViewIndex; uiBaseIndex++ )
    {
      READ_SVLC( iCode, "cp_scale" );                m_aaiTempScale [ uiBaseIndex ][ uiViewIndex ] = iCode;
      READ_SVLC( iCode, "cp_off" );                  m_aaiTempOffset[ uiBaseIndex ][ uiViewIndex ] = iCode;
      READ_SVLC( iCode, "cp_inv_scale_plus_scale" ); m_aaiTempScale [ uiViewIndex ][ uiBaseIndex ] = iCode - m_aaiTempScale [ uiBaseIndex ][ uiViewIndex ];
      READ_SVLC( iCode, "cp_inv_off_plus_off" );     m_aaiTempOffset[ uiViewIndex ][ uiBaseIndex ] = iCode - m_aaiTempOffset[ uiBaseIndex ][ uiViewIndex ];
    }
    rpcSlice->setCamparaSlice( m_aaiTempScale, m_aaiTempOffset );
  }

#endif

  if(pps->getSliceHeaderExtensionPresentFlag())
  {
    READ_UVLC(uiCode,"slice_header_extension_length");
#if H_3D && !CAM_HLS_F0044
#if CAM_HLS_F0136_F0045_F0082
    if( rpcSlice->getVPS()->hasCamParInSliceHeader( rpcSlice->getViewIndex() )  && !rpcSlice->getIsDepth() )
#else
    if( rpcSlice->getSPS()->hasCamParInSliceHeader() )
#endif
    {
      UInt uiViewIndex = rpcSlice->getViewIndex();
      for( UInt uiBaseIndex = 0; uiBaseIndex < uiViewIndex; uiBaseIndex++ )
      {
        READ_SVLC( iCode, "cp_scale" );                m_aaiTempScale [ uiBaseIndex ][ uiViewIndex ] = iCode;
        READ_SVLC( iCode, "cp_off" );                  m_aaiTempOffset[ uiBaseIndex ][ uiViewIndex ] = iCode;
        READ_SVLC( iCode, "cp_inv_scale_plus_scale" ); m_aaiTempScale [ uiViewIndex ][ uiBaseIndex ] = iCode - m_aaiTempScale [ uiBaseIndex ][ uiViewIndex ];
        READ_SVLC( iCode, "cp_inv_off_plus_off" );     m_aaiTempOffset[ uiViewIndex ][ uiBaseIndex ] = iCode - m_aaiTempOffset[ uiBaseIndex ][ uiViewIndex ];
      }
      rpcSlice->setCamparaSlice( m_aaiTempScale, m_aaiTempOffset );
    }

    READ_FLAG(uiCode,"slice_segment_header_extension2_flag"); 
    if ( uiCode )
    {    
      READ_UVLC(uiCode,"slice_header_extension2_length");
      for(Int i=0; i<uiCode; i++)
      {
        UInt ignore;
        READ_CODE(8,ignore,"slice_header_extension2_data_byte");
      }
    }
  }
#else
    for(Int i=0; i<uiCode; i++)
    {
      UInt ignore;
      READ_CODE(8,ignore,"slice_header_extension_data_byte");
    }
  }
#endif
  m_pcBitstream->readByteAlignment();

  if( pps->getTilesEnabledFlag() || pps->getEntropyCodingSyncEnabledFlag() )
  {
    Int endOfSliceHeaderLocation = m_pcBitstream->getByteLocation();
    
    // Adjust endOfSliceHeaderLocation to account for emulation prevention bytes in the slice segment header
    for ( UInt curByteIdx  = 0; curByteIdx<m_pcBitstream->numEmulationPreventionBytesRead(); curByteIdx++ )
    {
      if ( m_pcBitstream->getEmulationPreventionByteLocation( curByteIdx ) < endOfSliceHeaderLocation )
      {
        endOfSliceHeaderLocation++;
      }
    }

    Int  curEntryPointOffset     = 0;
    Int  prevEntryPointOffset    = 0;
    for (UInt idx=0; idx<numEntryPointOffsets; idx++)
    {
      curEntryPointOffset += entryPointOffset[ idx ];

      Int emulationPreventionByteCount = 0;
      for ( UInt curByteIdx  = 0; curByteIdx<m_pcBitstream->numEmulationPreventionBytesRead(); curByteIdx++ )
      {
        if ( m_pcBitstream->getEmulationPreventionByteLocation( curByteIdx ) >= ( prevEntryPointOffset + endOfSliceHeaderLocation ) && 
             m_pcBitstream->getEmulationPreventionByteLocation( curByteIdx ) <  ( curEntryPointOffset  + endOfSliceHeaderLocation ) )
        {
          emulationPreventionByteCount++;
        }
      }

      entryPointOffset[ idx ] -= emulationPreventionByteCount;
      prevEntryPointOffset = curEntryPointOffset;
    }

    if ( pps->getTilesEnabledFlag() )
    {
      rpcSlice->setTileLocationCount( numEntryPointOffsets );

      UInt prevPos = 0;
      for (Int idx=0; idx<rpcSlice->getTileLocationCount(); idx++)
      {
        rpcSlice->setTileLocation( idx, prevPos + entryPointOffset [ idx ] );
        prevPos += entryPointOffset[ idx ];
      }
    }
    else if ( pps->getEntropyCodingSyncEnabledFlag() )
    {
    Int numSubstreams = rpcSlice->getNumEntryPointOffsets()+1;
      rpcSlice->allocSubstreamSizes(numSubstreams);
      UInt *pSubstreamSizes       = rpcSlice->getSubstreamSizes();
      for (Int idx=0; idx<numSubstreams-1; idx++)
      {
        if ( idx < numEntryPointOffsets )
        {
          pSubstreamSizes[ idx ] = ( entryPointOffset[ idx ] << 3 ) ;
        }
        else
        {
          pSubstreamSizes[ idx ] = 0;
        }
      }
    }

    if (entryPointOffset)
    {
      delete [] entryPointOffset;
    }
  }

  return;
}
  
Void TDecCavlc::parsePTL( TComPTL *rpcPTL, Bool profilePresentFlag, Int maxNumSubLayersMinus1 )
{
  UInt uiCode;
  if(profilePresentFlag)
  {
    parseProfileTier(rpcPTL->getGeneralPTL());
  }
  READ_CODE( 8, uiCode, "general_level_idc" );    rpcPTL->getGeneralPTL()->setLevelIdc(uiCode);

  for (Int i = 0; i < maxNumSubLayersMinus1; i++)
  {
#if !H_MV
    if(profilePresentFlag)
    {
#endif
      READ_FLAG( uiCode, "sub_layer_profile_present_flag[i]" ); rpcPTL->setSubLayerProfilePresentFlag(i, uiCode);
#if H_MV
    rpcPTL->setSubLayerProfilePresentFlag( i, profilePresentFlag && rpcPTL->getSubLayerProfilePresentFlag(i) );
#else
    }
#endif
    READ_FLAG( uiCode, "sub_layer_level_present_flag[i]"   ); rpcPTL->setSubLayerLevelPresentFlag  (i, uiCode);
  }
  
  if (maxNumSubLayersMinus1 > 0)
  {
    for (Int i = maxNumSubLayersMinus1; i < 8; i++)
    {
      READ_CODE(2, uiCode, "reserved_zero_2bits");
      assert(uiCode == 0);
    }
  }
  
  for(Int i = 0; i < maxNumSubLayersMinus1; i++)
  {
    if( profilePresentFlag && rpcPTL->getSubLayerProfilePresentFlag(i) )
    {
      parseProfileTier(rpcPTL->getSubLayerPTL(i));
    }
    if(rpcPTL->getSubLayerLevelPresentFlag(i))
    {
      READ_CODE( 8, uiCode, "sub_layer_level_idc[i]" );   rpcPTL->getSubLayerPTL(i)->setLevelIdc(uiCode);
    }
  }
}

Void TDecCavlc::parseProfileTier(ProfileTierLevel *ptl)
{
  UInt uiCode;
  READ_CODE(2 , uiCode, "XXX_profile_space[]");   ptl->setProfileSpace(uiCode);
  READ_FLAG(    uiCode, "XXX_tier_flag[]"    );   ptl->setTierFlag    (uiCode ? 1 : 0);
  READ_CODE(5 , uiCode, "XXX_profile_idc[]"  );   ptl->setProfileIdc  (uiCode);
  for(Int j = 0; j < 32; j++)
  {
    READ_FLAG(  uiCode, "XXX_profile_compatibility_flag[][j]");   ptl->setProfileCompatibilityFlag(j, uiCode ? 1 : 0);
  }
  READ_FLAG(uiCode, "general_progressive_source_flag");
  ptl->setProgressiveSourceFlag(uiCode ? true : false);

  READ_FLAG(uiCode, "general_interlaced_source_flag");
  ptl->setInterlacedSourceFlag(uiCode ? true : false);
  
  READ_FLAG(uiCode, "general_non_packed_constraint_flag");
  ptl->setNonPackedConstraintFlag(uiCode ? true : false);
  
  READ_FLAG(uiCode, "general_frame_only_constraint_flag");
  ptl->setFrameOnlyConstraintFlag(uiCode ? true : false);
  
  READ_CODE(16, uiCode, "XXX_reserved_zero_44bits[0..15]");
  READ_CODE(16, uiCode, "XXX_reserved_zero_44bits[16..31]");
  READ_CODE(12, uiCode, "XXX_reserved_zero_44bits[32..43]");
}

Void TDecCavlc::parseTerminatingBit( UInt& ruiBit )
{
  ruiBit = false;
  Int iBitsLeft = m_pcBitstream->getNumBitsLeft();
  if(iBitsLeft <= 8)
  {
    UInt uiPeekValue = m_pcBitstream->peekBits(iBitsLeft);
    if (uiPeekValue == (1<<(iBitsLeft-1)))
    {
      ruiBit = true;
    }
  }
}

Void TDecCavlc::parseSkipFlag( TComDataCU* /*pcCU*/, UInt /*uiAbsPartIdx*/, UInt /*uiDepth*/ )
{
  assert(0);
}

Void TDecCavlc::parseCUTransquantBypassFlag( TComDataCU* /*pcCU*/, UInt /*uiAbsPartIdx*/, UInt /*uiDepth*/ )
{
  assert(0);
}

Void TDecCavlc::parseMVPIdx( Int& /*riMVPIdx*/ )
{
  assert(0);
}

Void TDecCavlc::parseSplitFlag     ( TComDataCU* /*pcCU*/, UInt /*uiAbsPartIdx*/, UInt /*uiDepth*/ )
{
  assert(0);
}

Void TDecCavlc::parsePartSize( TComDataCU* /*pcCU*/, UInt /*uiAbsPartIdx*/, UInt /*uiDepth*/ )
{
  assert(0);
}

Void TDecCavlc::parsePredMode( TComDataCU* /*pcCU*/, UInt /*uiAbsPartIdx*/, UInt /*uiDepth*/ )
{
  assert(0);
}

/** Parse I_PCM information. 
* \param pcCU pointer to CU
* \param uiAbsPartIdx CU index
* \param uiDepth CU depth
* \returns Void
*
* If I_PCM flag indicates that the CU is I_PCM, parse its PCM alignment bits and codes.  
*/
Void TDecCavlc::parseIPCMInfo( TComDataCU* /*pcCU*/, UInt /*uiAbsPartIdx*/, UInt /*uiDepth*/ )
{
  assert(0);
}

Void TDecCavlc::parseIntraDirLumaAng  ( TComDataCU* /*pcCU*/, UInt /*uiAbsPartIdx*/, UInt /*uiDepth*/ )
{ 
  assert(0);
}

Void TDecCavlc::parseIntraDirChroma( TComDataCU* /*pcCU*/, UInt /*uiAbsPartIdx*/, UInt /*uiDepth*/ )
{
  assert(0);
}

Void TDecCavlc::parseInterDir( TComDataCU* /*pcCU*/, UInt& /*ruiInterDir*/, UInt /*uiAbsPartIdx*/ )
{
  assert(0);
}

Void TDecCavlc::parseRefFrmIdx( TComDataCU* /*pcCU*/, Int& /*riRefFrmIdx*/, RefPicList /*eRefList*/ )
{
  assert(0);
}

Void TDecCavlc::parseMvd( TComDataCU* /*pcCU*/, UInt /*uiAbsPartIdx*/, UInt /*uiPartIdx*/, UInt /*uiDepth*/, RefPicList /*eRefList*/ )
{
  assert(0);
}

Void TDecCavlc::parseDeltaQP( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth )
{
  Int qp;
  Int  iDQp;

  xReadSvlc( iDQp );

  Int qpBdOffsetY = pcCU->getSlice()->getSPS()->getQpBDOffsetY();
  qp = (((Int) pcCU->getRefQP( uiAbsPartIdx ) + iDQp + 52 + 2*qpBdOffsetY )%(52+ qpBdOffsetY)) -  qpBdOffsetY;

  UInt uiAbsQpCUPartIdx = (uiAbsPartIdx>>((g_uiMaxCUDepth - pcCU->getSlice()->getPPS()->getMaxCuDQPDepth())<<1))<<((g_uiMaxCUDepth - pcCU->getSlice()->getPPS()->getMaxCuDQPDepth())<<1) ;
  UInt uiQpCUDepth =   min(uiDepth,pcCU->getSlice()->getPPS()->getMaxCuDQPDepth()) ;

  pcCU->setQPSubParts( qp, uiAbsQpCUPartIdx, uiQpCUDepth );
}

Void TDecCavlc::parseCoeffNxN( TComDataCU* /*pcCU*/, TCoeff* /*pcCoef*/, UInt /*uiAbsPartIdx*/, UInt /*uiWidth*/, UInt /*uiHeight*/, UInt /*uiDepth*/, TextType /*eTType*/ )
{
  assert(0);
}

Void TDecCavlc::parseTransformSubdivFlag( UInt& /*ruiSubdivFlag*/, UInt /*uiLog2TransformBlockSize*/ )
{
  assert(0);
}

Void TDecCavlc::parseQtCbf( TComDataCU* /*pcCU*/, UInt /*uiAbsPartIdx*/, TextType /*eType*/, UInt /*uiTrDepth*/, UInt /*uiDepth*/ )
{
  assert(0);
}

Void TDecCavlc::parseQtRootCbf( UInt /*uiAbsPartIdx*/, UInt& /*uiQtRootCbf*/ )
{
  assert(0);
}

Void TDecCavlc::parseTransformSkipFlags (TComDataCU* /*pcCU*/, UInt /*uiAbsPartIdx*/, UInt /*width*/, UInt /*height*/, UInt /*uiDepth*/, TextType /*eTType*/)
{
  assert(0);
}

Void TDecCavlc::parseMergeFlag ( TComDataCU* /*pcCU*/, UInt /*uiAbsPartIdx*/, UInt /*uiDepth*/, UInt /*uiPUIdx*/ )
{
  assert(0);
}

Void TDecCavlc::parseMergeIndex ( TComDataCU* /*pcCU*/, UInt& /*ruiMergeIndex*/ )
{
  assert(0);
}

#if H_3D_ARP
Void TDecCavlc::parseARPW( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth )
{
  assert(0);
}
#endif
#if H_3D_IC
Void TDecCavlc::parseICFlag( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth )
{
  assert(0);
}
#endif
#if H_3D_INTER_SDC
Void TDecCavlc::parseInterSDCFlag    ( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth )
{
  assert(0);
}

Void TDecCavlc::parseInterSDCResidualData ( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth, UInt uiPart )
{
  assert(0);
}
#endif
// ====================================================================================================================
// Protected member functions
// ====================================================================================================================

/** parse explicit wp tables
* \param TComSlice* pcSlice
* \returns Void
*/
Void TDecCavlc::xParsePredWeightTable( TComSlice* pcSlice )
{
  wpScalingParam  *wp;
  Bool            bChroma     = true; // color always present in HEVC ?
  SliceType       eSliceType  = pcSlice->getSliceType();
  Int             iNbRef       = (eSliceType == B_SLICE ) ? (2) : (1);
  UInt            uiLog2WeightDenomLuma, uiLog2WeightDenomChroma;
  UInt            uiTotalSignalledWeightFlags = 0;
  
  Int iDeltaDenom;
  // decode delta_luma_log2_weight_denom :
  READ_UVLC( uiLog2WeightDenomLuma, "luma_log2_weight_denom" );     // ue(v): luma_log2_weight_denom
  assert( uiLog2WeightDenomLuma <= 7 );
  if( bChroma ) 
  {
    READ_SVLC( iDeltaDenom, "delta_chroma_log2_weight_denom" );     // se(v): delta_chroma_log2_weight_denom
    assert((iDeltaDenom + (Int)uiLog2WeightDenomLuma)>=0);
    assert((iDeltaDenom + (Int)uiLog2WeightDenomLuma)<=7);
    uiLog2WeightDenomChroma = (UInt)(iDeltaDenom + uiLog2WeightDenomLuma);
  }

  for ( Int iNumRef=0 ; iNumRef<iNbRef ; iNumRef++ ) 
  {
    RefPicList  eRefPicList = ( iNumRef ? REF_PIC_LIST_1 : REF_PIC_LIST_0 );
    for ( Int iRefIdx=0 ; iRefIdx<pcSlice->getNumRefIdx(eRefPicList) ; iRefIdx++ ) 
    {
      pcSlice->getWpScaling(eRefPicList, iRefIdx, wp);

      wp[0].uiLog2WeightDenom = uiLog2WeightDenomLuma;
      wp[1].uiLog2WeightDenom = uiLog2WeightDenomChroma;
      wp[2].uiLog2WeightDenom = uiLog2WeightDenomChroma;

      UInt  uiCode;
      READ_FLAG( uiCode, "luma_weight_lX_flag" );           // u(1): luma_weight_l0_flag
      wp[0].bPresentFlag = ( uiCode == 1 );
      uiTotalSignalledWeightFlags += wp[0].bPresentFlag;
    }
    if ( bChroma ) 
    {
      UInt  uiCode;
      for ( Int iRefIdx=0 ; iRefIdx<pcSlice->getNumRefIdx(eRefPicList) ; iRefIdx++ ) 
      {
        pcSlice->getWpScaling(eRefPicList, iRefIdx, wp);
        READ_FLAG( uiCode, "chroma_weight_lX_flag" );      // u(1): chroma_weight_l0_flag
        wp[1].bPresentFlag = ( uiCode == 1 );
        wp[2].bPresentFlag = ( uiCode == 1 );
        uiTotalSignalledWeightFlags += 2*wp[1].bPresentFlag;
      }
    }
    for ( Int iRefIdx=0 ; iRefIdx<pcSlice->getNumRefIdx(eRefPicList) ; iRefIdx++ ) 
    {
      pcSlice->getWpScaling(eRefPicList, iRefIdx, wp);
      if ( wp[0].bPresentFlag ) 
      {
        Int iDeltaWeight;
        READ_SVLC( iDeltaWeight, "delta_luma_weight_lX" );  // se(v): delta_luma_weight_l0[i]
        assert( iDeltaWeight >= -128 );
        assert( iDeltaWeight <=  127 );
        wp[0].iWeight = (iDeltaWeight + (1<<wp[0].uiLog2WeightDenom));
        READ_SVLC( wp[0].iOffset, "luma_offset_lX" );       // se(v): luma_offset_l0[i]
        assert( wp[0].iOffset >= -128 );
        assert( wp[0].iOffset <=  127 );
      }
      else 
      {
        wp[0].iWeight = (1 << wp[0].uiLog2WeightDenom);
        wp[0].iOffset = 0;
      }
      if ( bChroma ) 
      {
        if ( wp[1].bPresentFlag ) 
        {
          for ( Int j=1 ; j<3 ; j++ ) 
          {
            Int iDeltaWeight;
            READ_SVLC( iDeltaWeight, "delta_chroma_weight_lX" );  // se(v): chroma_weight_l0[i][j]
            assert( iDeltaWeight >= -128 );
            assert( iDeltaWeight <=  127 );
            wp[j].iWeight = (iDeltaWeight + (1<<wp[1].uiLog2WeightDenom));

            Int iDeltaChroma;
            READ_SVLC( iDeltaChroma, "delta_chroma_offset_lX" );  // se(v): delta_chroma_offset_l0[i][j]
            assert( iDeltaChroma >= -512 );
            assert( iDeltaChroma <=  511 );
            Int pred = ( 128 - ( ( 128*wp[j].iWeight)>>(wp[j].uiLog2WeightDenom) ) );
            wp[j].iOffset = Clip3(-128, 127, (iDeltaChroma + pred) );
          }
        }
        else 
        {
          for ( Int j=1 ; j<3 ; j++ ) 
          {
            wp[j].iWeight = (1 << wp[j].uiLog2WeightDenom);
            wp[j].iOffset = 0;
          }
        }
      }
    }

    for ( Int iRefIdx=pcSlice->getNumRefIdx(eRefPicList) ; iRefIdx<MAX_NUM_REF ; iRefIdx++ ) 
    {
      pcSlice->getWpScaling(eRefPicList, iRefIdx, wp);

      wp[0].bPresentFlag = false;
      wp[1].bPresentFlag = false;
      wp[2].bPresentFlag = false;
    }
  }
  assert(uiTotalSignalledWeightFlags<=24);
}

/** decode quantization matrix
* \param scalingList quantization matrix information
*/
Void TDecCavlc::parseScalingList(TComScalingList* scalingList)
{
  UInt  code, sizeId, listId;
  Bool scalingListPredModeFlag;
  //for each size
  for(sizeId = 0; sizeId < SCALING_LIST_SIZE_NUM; sizeId++)
  {
    for(listId = 0; listId <  g_scalingListNum[sizeId]; listId++)
    {
      READ_FLAG( code, "scaling_list_pred_mode_flag");
      scalingListPredModeFlag = (code) ? true : false;
      if(!scalingListPredModeFlag) //Copy Mode
      {
        READ_UVLC( code, "scaling_list_pred_matrix_id_delta");
        scalingList->setRefMatrixId (sizeId,listId,(UInt)((Int)(listId)-(code)));
        if( sizeId > SCALING_LIST_8x8 )
        {
          scalingList->setScalingListDC(sizeId,listId,((listId == scalingList->getRefMatrixId (sizeId,listId))? 16 :scalingList->getScalingListDC(sizeId, scalingList->getRefMatrixId (sizeId,listId))));
        }
        scalingList->processRefMatrix( sizeId, listId, scalingList->getRefMatrixId (sizeId,listId));

      }
      else //DPCM Mode
      {
        xDecodeScalingList(scalingList, sizeId, listId);
      }
    }
  }

  return;
}
/** decode DPCM
* \param scalingList  quantization matrix information
* \param sizeId size index
* \param listId list index
*/
Void TDecCavlc::xDecodeScalingList(TComScalingList *scalingList, UInt sizeId, UInt listId)
{
  Int i,coefNum = min(MAX_MATRIX_COEF_NUM,(Int)g_scalingListSize[sizeId]);
  Int data;
  Int scalingListDcCoefMinus8 = 0;
  Int nextCoef = SCALING_LIST_START_VALUE;
  UInt* scan  = (sizeId == 0) ? g_auiSigLastScan [ SCAN_DIAG ] [ 1 ] :  g_sigLastScanCG32x32;
  Int *dst = scalingList->getScalingListAddress(sizeId, listId);

  if( sizeId > SCALING_LIST_8x8 )
  {
    READ_SVLC( scalingListDcCoefMinus8, "scaling_list_dc_coef_minus8");
    scalingList->setScalingListDC(sizeId,listId,scalingListDcCoefMinus8 + 8);
    nextCoef = scalingList->getScalingListDC(sizeId,listId);
  }

  for(i = 0; i < coefNum; i++)
  {
    READ_SVLC( data, "scaling_list_delta_coef");
    nextCoef = (nextCoef + data + 256 ) % 256;
    dst[scan[i]] = nextCoef;
  }
}

Bool TDecCavlc::xMoreRbspData()
{ 
  Int bitsLeft = m_pcBitstream->getNumBitsLeft();

  // if there are more than 8 bits, it cannot be rbsp_trailing_bits
  if (bitsLeft > 8)
  {
    return true;
  }

  UChar lastByte = m_pcBitstream->peekBits(bitsLeft);
  Int cnt = bitsLeft;

  // remove trailing bits equal to zero
  while ((cnt>0) && ((lastByte & 1) == 0))
  {
    lastByte >>= 1;
    cnt--;
  }
  // remove bit equal to one
  cnt--;

  // we should not have a negative number of bits
  assert (cnt>=0);

  // we have more data, if cnt is not zero
  return (cnt>0);
}

//! \}

