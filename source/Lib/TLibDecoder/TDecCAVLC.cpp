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

/** \file     TDecCAVLC.cpp
\brief    CAVLC decoder class
*/

#include "TDecCAVLC.h"
#include "SEIread.h"
#include "TDecSlice.h"
#if H_3D_ANNEX_SELECTION_FIX
#include "TDecTop.h"
#endif
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
  fprintf( g_hTrace, "=========== Sequence Parameter Set LayerId: %d ===========\n", pSPS->getLayerId() );
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
  fprintf( g_hTrace, "=========== Picture Parameter Set LayerId: %d ===========\n", pPPS->getLayerId() );
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
#if !HHI_CAM_PARA_K0052
#if H_3D
  m_aaiTempScale            = new Int* [ MAX_NUM_LAYERS ];
  m_aaiTempOffset           = new Int* [ MAX_NUM_LAYERS ];
  for( UInt uiVId = 0; uiVId < MAX_NUM_LAYERS; uiVId++ )
  {
    m_aaiTempScale            [ uiVId ] = new Int [ MAX_NUM_LAYERS ];
    m_aaiTempOffset           [ uiVId ] = new Int [ MAX_NUM_LAYERS ];
  }
#endif
#endif
}

TDecCavlc::~TDecCavlc()
{
#if !HHI_CAM_PARA_K0052
#if H_3D
  for( UInt uiVId = 0; uiVId < MAX_NUM_LAYERS; uiVId++ )
  {
    delete [] m_aaiTempScale            [ uiVId ];
    delete [] m_aaiTempOffset           [ uiVId ];
  }
  delete [] m_aaiTempScale;
  delete [] m_aaiTempOffset;
#endif
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

#if H_3D
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
    READ_UVLC ( uiCode, "num_tile_columns_minus1" );                pcPPS->setNumTileColumnsMinus1( uiCode );  
    READ_UVLC ( uiCode, "num_tile_rows_minus1" );                   pcPPS->setNumTileRowsMinus1( uiCode );  
    READ_FLAG ( uiCode, "uniform_spacing_flag" );                   pcPPS->setTileUniformSpacingFlag( uiCode == 1 );

    if( !pcPPS->getTileUniformSpacingFlag())
    {
      std::vector<Int> columnWidth(pcPPS->getNumTileColumnsMinus1());
      for(UInt i=0; i<pcPPS->getNumTileColumnsMinus1(); i++)
      { 
        READ_UVLC( uiCode, "column_width_minus1" );  
        columnWidth[i] = uiCode+1;
      }
      pcPPS->setTileColumnWidth(columnWidth);

      std::vector<Int> rowHeight (pcPPS->getTileNumRowsMinus1());
      for(UInt i=0; i<pcPPS->getTileNumRowsMinus1(); i++)
      {
        READ_UVLC( uiCode, "row_height_minus1" );
        rowHeight[i] = uiCode + 1;
      }
      pcPPS->setTileRowHeight(rowHeight);
    }

    if(pcPPS->getNumTileColumnsMinus1() !=0 || pcPPS->getTileNumRowsMinus1() !=0)
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
  READ_FLAG( uiCode, "pps_scaling_list_data_present_flag" );           pcPPS->setScalingListPresentFlag( uiCode ? true : false );
  if(pcPPS->getScalingListPresentFlag ())
  {
    parseScalingList( pcPPS->getScalingList() );
  }
  READ_FLAG( uiCode, "lists_modification_present_flag");
  pcPPS->setListsModificationPresentFlag(uiCode);

  READ_UVLC( uiCode, "log2_parallel_merge_level_minus2");
  pcPPS->setLog2ParallelMergeLevelMinus2 (uiCode);

  READ_FLAG( uiCode, "slice_segment_header_extension_present_flag");
  pcPPS->setSliceHeaderExtensionPresentFlag(uiCode);
  
#if H_MV
  READ_FLAG( uiCode, "pps_extension_present_flag");
#else
  READ_FLAG( uiCode, "pps_extension_flag");
#endif
  if (uiCode)
  {

#if H_MV
    READ_FLAG( uiCode, "pps_range_extensions_flag" ); pcPPS->setPpsRangeExtensionsFlag( uiCode == 1 );
    READ_FLAG( uiCode, "pps_multilayer_extension_flag" ); pcPPS->setPpsMultilayerExtensionFlag( uiCode == 1 );
#if !H_3D
    READ_CODE( 6, uiCode, "pps_extension_6bits" ); pcPPS->setPpsExtension6bits( uiCode );
#else
    READ_FLAG( uiCode, "pps_3d_extension_flag" ); pcPPS->setPps3dExtensionFlag( uiCode == 1 );
    READ_CODE( 5, uiCode, "pps_extension_5bits" ); pcPPS->setPpsExtension5bits( uiCode );
#endif
    if ( pcPPS->getPpsRangeExtensionsFlag() )
    { 
      assert(0); 
    }

    if ( pcPPS->getPpsMultilayerExtensionFlag() )
    { 
      parsePPSMultilayerExtension( pcPPS ); 
    }
#if !H_3D
    if ( pcPPS->getPpsExtension6bits() )
    {
#else
    if ( pcPPS->getPps3dExtensionFlag() )
    { 
      parsePPSExtension( pcPPS, pcVPS );
    }
    if ( pcPPS->getPpsExtension5bits() )
    {
#endif

#endif

      while ( xMoreRbspData() )
      {
        READ_FLAG( uiCode, "pps_extension_data_flag");
      }
#if H_MV
    }
#endif
  }
}


#if H_3D
Void TDecCavlc::parsePPSExtension( TComPPS* pcPPS, TComVPS* pcVPS )
{
  //Ed.(GT): pcVPS should not be used here. Needs to be fixed. 
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

            if( uiCode )
            {
                assert( pcDLT->getUseDLTFlag( 1 ));
            }
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
#if H_MV
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

Void TDecCavlc::parseSPS(TComSPS* pcSPS)
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
#if H_MV
  }
  else
  {
    READ_CODE( 3, uiCode, "sps_ext_or_max_sub_layers_minus1" ); pcSPS->setSpsExtOrMaxSubLayersMinus1( uiCode );    
    pcSPS->inferSpsMaxSubLayersMinus1( false, NULL );
  }
  if ( !pcSPS->getMultiLayerExtSpsFlag() )
  {
#endif

    READ_FLAG( uiCode, "sps_temporal_id_nesting_flag" );               pcSPS->setTemporalIdNestingFlag ( uiCode > 0 ? true : false );
    if ( pcSPS->getMaxTLayers() == 1 )
    {
      // sps_temporal_id_nesting_flag must be 1 when sps_max_sub_layers_minus1 is 0
      assert( uiCode == 1 );
    }

    parsePTL(pcSPS->getPTL(), 1, pcSPS->getMaxTLayers() - 1);
#if H_MV
    pcSPS->getPTL()->inferGeneralValues ( true, 0, NULL ); 
    pcSPS->getPTL()->inferSubLayerValues( pcSPS->getMaxTLayers() - 1, 0, NULL );
  }
#endif
  READ_UVLC(     uiCode, "sps_seq_parameter_set_id" );           pcSPS->setSPSId( uiCode );
  assert(uiCode <= 15);
#if H_MV
  if ( pcSPS->getMultiLayerExtSpsFlag() )
  {
    READ_FLAG( uiCode, "update_rep_format_flag" );               pcSPS->setUpdateRepFormatFlag( uiCode == 1 );
    if ( pcSPS->getUpdateRepFormatFlag() )
    { 
      READ_CODE( 8, uiCode, "sps_rep_format_idx" );                pcSPS->setSpsRepFormatIdx( uiCode );
    }
  }
  else
  {
#endif
    READ_UVLC(     uiCode, "chroma_format_idc" );                  pcSPS->setChromaFormatIdc( uiCode );
    assert(uiCode <= 3);
    // in the first version we only support chroma_format_idc equal to 1 (4:2:0), so separate_colour_plane_flag cannot appear in the bitstream
#if !H_3D_DISABLE_CHROMA
    assert (uiCode == 1);
#endif
    if( uiCode == 3 )
    {
      READ_FLAG(     uiCode, "separate_colour_plane_flag");        assert(uiCode == 0);
    }

    READ_UVLC (    uiCode, "pic_width_in_luma_samples" );          pcSPS->setPicWidthInLumaSamples ( uiCode    );
    READ_UVLC (    uiCode, "pic_height_in_luma_samples" );         pcSPS->setPicHeightInLumaSamples( uiCode    );
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
  }
#else
    READ_UVLC(   uiCode, "conf_win_left_offset" );               conf.setWindowLeftOffset  ( uiCode * TComSPS::getWinUnitX( pcSPS->getChromaFormatIdc() ) );
    READ_UVLC(   uiCode, "conf_win_right_offset" );              conf.setWindowRightOffset ( uiCode * TComSPS::getWinUnitX( pcSPS->getChromaFormatIdc() ) );
    READ_UVLC(   uiCode, "conf_win_top_offset" );                conf.setWindowTopOffset   ( uiCode * TComSPS::getWinUnitY( pcSPS->getChromaFormatIdc() ) );
    READ_UVLC(   uiCode, "conf_win_bottom_offset" );             conf.setWindowBottomOffset( uiCode * TComSPS::getWinUnitY( pcSPS->getChromaFormatIdc() ) );
#endif
  }

#if H_MV
  if ( !pcSPS->getMultiLayerExtSpsFlag() )
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

#if H_MV
  if ( !pcSPS->getMultiLayerExtSpsFlag()) 
  {  
#endif
    UInt subLayerOrderingInfoPresentFlag;
    READ_FLAG(subLayerOrderingInfoPresentFlag, "sps_sub_layer_ordering_info_present_flag");

    for(UInt i=0; i <= pcSPS->getMaxTLayers()-1; i++)
    {
      READ_UVLC ( uiCode, "sps_max_dec_pic_buffering_minus1[i]");
      pcSPS->setMaxDecPicBuffering( uiCode + 1, i);
      READ_UVLC ( uiCode, "sps_num_reorder_pics[i]" );
      pcSPS->setNumReorderPics(uiCode, i);
      READ_UVLC ( uiCode, "sps_max_latency_increase_plus1[i]");
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
#if H_MV
  }
#endif

  READ_UVLC( uiCode, "log2_min_coding_block_size_minus3" );
  Int log2MinCUSize = uiCode + 3;
  pcSPS->setLog2MinCodingBlockSize(log2MinCUSize);
  READ_UVLC( uiCode, "log2_diff_max_min_coding_block_size" );
  pcSPS->setLog2DiffMaxMinCodingBlockSize(uiCode);

  if (pcSPS->getPTL()->getGeneralPTL()->getLevelIdc() >= Level::LEVEL5)
  {
    assert(log2MinCUSize + pcSPS->getLog2DiffMaxMinCodingBlockSize() >= 5);
  }

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
    if ( pcSPS->getMultiLayerExtSpsFlag() )
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


#if H_MV
  READ_FLAG( uiCode, "sps_extension_present_flag");
  pcSPS->setSpsExtensionPresentFlag( uiCode ); 
  if (pcSPS->getSpsExtensionPresentFlag( ) )
#else
  READ_FLAG( uiCode, "sps_extension_flag");
  if (uiCode)
#endif
  {
#if H_MV
    READ_FLAG( uiCode, "sps_range_extensions_flag" ); pcSPS->setSpsRangeExtensionsFlag( uiCode == 1 );
    READ_FLAG( uiCode, "sps_multilayer_extension_flag" ); pcSPS->setSpsMultilayerExtensionFlag( uiCode == 1 );
#if !H_3D
    READ_CODE( 6, uiCode, "sps_extension_6bits" ); pcSPS->setSpsExtension6bits( uiCode );
#else
    READ_FLAG( uiCode, "sps_3d_extension_flag" ); pcSPS->setSps3dExtensionFlag( uiCode == 1 );
    READ_CODE( 5, uiCode, "sps_extension_5bits" ); pcSPS->setSpsExtension5bits( uiCode );
#endif
  }

  if ( pcSPS->getSpsRangeExtensionsFlag() )
  {
    assert( 0 ); 
  }

  if ( pcSPS->getSpsMultilayerExtensionFlag() )
  {
    parseSPSExtension( pcSPS ); 
  }

#if H_3D
  if ( pcSPS->getSps3dExtensionFlag() )
  {
    parseSPS3dExtension( pcSPS ); 
  }

  if ( pcSPS->getSpsExtension5bits() )
  { 
#else
  if ( pcSPS->getSpsExtension6bits() )
  { 
#endif

#endif
    while ( xMoreRbspData() )
    {
      READ_FLAG( uiCode, "sps_extension_data_flag");
    }
  }
}

#if H_MV
Void TDecCavlc::parseSPSExtension( TComSPS* pcSPS )
{
  UInt uiCode; 
  READ_FLAG( uiCode, "inter_view_mv_vert_constraint_flag" );    pcSPS->setInterViewMvVertConstraintFlag(uiCode == 1 ? true : false);
  
}

#if H_3D
Void TDecCavlc::parseSPS3dExtension( TComSPS* pcSPS )
{ 
  TComSps3dExtension* sps3dExt = pcSPS->getSps3dExtension(); 
  UInt uiCode; 
  for( Int d = 0; d  <=  1; d++ )
  {
    READ_FLAG( uiCode, "iv_mv_pred_flag" ); sps3dExt->setIvMvPredFlag( d, uiCode == 1 );
    READ_FLAG( uiCode, "iv_mv_scaling_flag" ); sps3dExt->setIvMvScalingFlag( d, uiCode == 1 );
    if( d  ==  0 )
    {
      READ_UVLC( uiCode, "log2_sub_pb_size_minus3" ); sps3dExt->setLog2SubPbSizeMinus3( d, uiCode );
      READ_FLAG( uiCode, "iv_res_pred_flag" ); sps3dExt->setIvResPredFlag( d, uiCode == 1 );
      READ_FLAG( uiCode, "depth_refinement_flag" ); sps3dExt->setDepthRefinementFlag( d, uiCode == 1 );
      READ_FLAG( uiCode, "view_synthesis_pred_flag" ); sps3dExt->setViewSynthesisPredFlag( d, uiCode == 1 );
      READ_FLAG( uiCode, "depth_based_blk_part_flag" ); sps3dExt->setDepthBasedBlkPartFlag( d, uiCode == 1 );
    }
    else 
    {
      READ_FLAG( uiCode, "mpi_flag" ); sps3dExt->setMpiFlag( d, uiCode == 1 );
      READ_UVLC( uiCode, "log2_mpi_sub_pb_size_minus3" ); sps3dExt->setLog2MpiSubPbSizeMinus3( d, uiCode );
      READ_FLAG( uiCode, "intra_contour_flag" ); sps3dExt->setIntraContourFlag( d, uiCode == 1 );
      READ_FLAG( uiCode, "intra_sdc_wedge_flag" ); sps3dExt->setIntraSdcWedgeFlag( d, uiCode == 1 );
      READ_FLAG( uiCode, "qt_pred_flag" ); sps3dExt->setQtPredFlag( d, uiCode == 1 );
      READ_FLAG( uiCode, "inter_sdc_flag" ); sps3dExt->setInterSdcFlag( d, uiCode == 1 );
#if SEC_DEPTH_INTRA_SKIP_MODE_K0033
      READ_FLAG( uiCode, "intra_skip_flag" ); sps3dExt->setDepthIntraSkipFlag( d, uiCode == 1 );
#else
      READ_FLAG( uiCode, "intra_single_flag" ); sps3dExt->setIntraSingleFlag( d, uiCode == 1 );
#endif
    }
  }
}
#endif

Void TDecCavlc::parsePPSMultilayerExtension(TComPPS* pcPPS)
{
  UInt uiCode = 0; 
  READ_FLAG( uiCode, "poc_reset_info_present_flag" ); pcPPS->setPocResetInfoPresentFlag( uiCode == 1 );
  READ_FLAG( uiCode, "pps_infer_scaling_list_flag" ); pcPPS->setPpsInferScalingListFlag( uiCode == 1 );
#if FIX_TICKET_95
  if (pcPPS->getPpsInferScalingListFlag())
  {
  READ_CODE( 6, uiCode, "pps_scaling_list_ref_layer_id" ); pcPPS->setPpsScalingListRefLayerId( uiCode );
  }
#else
  READ_CODE( 6, uiCode, "pps_scaling_list_ref_layer_id" ); pcPPS->setPpsScalingListRefLayerId( uiCode );
#endif

  UInt numRefLocOffsets;; 
  READ_UVLC( numRefLocOffsets, "num_ref_loc_offsets" );

  // All of the following stuff is not needed, but allowed to be present.
  for (Int i = 0; i < numRefLocOffsets; i++ )
  { 
    Int   iCode = 0; 
    READ_CODE( 6, uiCode, "ref_loc_offset_layer_id" ); 
    READ_FLAG( uiCode, "scaled_ref_layer_offset_present_flag" );

    if (uiCode)
    {    
      READ_SVLC( iCode, "scaled_ref_layer_left_offset" );   
      READ_SVLC( iCode, "scaled_ref_layer_top_offset" );    
      READ_SVLC( iCode, "scaled_ref_layer_right_offset" );  
      READ_SVLC( iCode, "scaled_ref_layer_bottom_offset" ); 
    }

    READ_FLAG( uiCode, "ref_region_offset_present_flag" );  
    if (uiCode)
    {    
      READ_SVLC( iCode, "ref_region_left_offset" );     
      READ_SVLC( iCode, "ref_region_top_offset" );      
      READ_SVLC( iCode, "ref_region_right_offset" );    
      READ_SVLC( iCode, "ref_region_bottom_offset" );   
    }

    READ_FLAG( uiCode, "resample_phase_set_present_flag" );
    if (uiCode)
    {      
      READ_UVLC( uiCode, "phase_hor_luma" );             
      READ_UVLC( uiCode, "phase_ver_luma" );             
      READ_UVLC( uiCode, "phase_hor_chroma_plus8" );     
      READ_UVLC( uiCode, "phase_ver_chroma_plus8" );     
    }
  }
  READ_FLAG( uiCode, "colour_mapping_enabled_flag" );   
  // This is required to equal to 0 for Multiview Main profile. 
  assert( uiCode == 0 ); 
}

#endif

Void TDecCavlc::parseVPS(TComVPS* pcVPS)
{
  UInt  uiCode;
  
  READ_CODE( 4,  uiCode,  "vps_video_parameter_set_id" );         pcVPS->setVPSId( uiCode );
#if H_MV
  READ_FLAG( uiCode, "vps_base_layer_internal_flag" );            pcVPS->setVpsBaseLayerInternalFlag( uiCode == 1 );
  READ_FLAG( uiCode, "vps_base_layer_available_flag" );           pcVPS->setVpsBaseLayerAvailableFlag( uiCode == 1 );
#else
  READ_CODE( 2,  uiCode,  "vps_reserved_three_2bits" );           assert(uiCode == 3);
#endif
#if H_MV
  READ_CODE( 6,  uiCode,  "vps_max_layers_minus1" );              pcVPS->setMaxLayersMinus1( std::min( uiCode, (UInt) ( MAX_NUM_LAYER_IDS-1) )  );
#else
  READ_CODE( 6,  uiCode,  "vps_reserved_zero_6bits" );            assert(uiCode == 0);
#endif
  READ_CODE( 3,  uiCode,  "vps_max_sub_layers_minus1" );          pcVPS->setMaxTLayers( uiCode + 1 );    assert(uiCode+1 <= MAX_TLAYER);
  READ_FLAG(     uiCode,  "vps_temporal_id_nesting_flag" );       pcVPS->setTemporalNestingFlag( uiCode ? true:false );
  assert (pcVPS->getMaxTLayers()>1||pcVPS->getTemporalNestingFlag());

  READ_CODE( 16, uiCode,  "vps_reserved_ffff_16bits" );           assert(uiCode == 0xffff);
  parsePTL ( pcVPS->getPTL(), true, pcVPS->getMaxTLayers()-1);
#if H_MV
  pcVPS->getPTL()->inferGeneralValues ( true, 0, NULL );
  pcVPS->getPTL()->inferSubLayerValues( pcVPS->getMaxTLayers() - 1, 0, NULL );
#endif
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

  READ_UVLC(    uiCode, "vps_num_layer_sets_minus1" );  pcVPS->setVpsNumLayerSetsMinus1( uiCode );
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
#if H_MV
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
      else
      {
        pcVPS->setCprmsPresentFlag( true, i );
      }

      parseHrdParameters(pcVPS->getHrdParameters(i), pcVPS->getCprmsPresentFlag( i ), pcVPS->getMaxTLayers() - 1);
    }
  }
#if H_MV
  READ_FLAG( uiCode,  "vps_extension_flag" );                      pcVPS->setVpsExtensionFlag( uiCode == 1 ? true : false );
  if ( pcVPS->getVpsExtensionFlag() )
#else
  READ_FLAG( uiCode,  "vps_extension_flag" );
  if (uiCode)
#endif
  {
#if H_MV
    m_pcBitstream->readOutTrailingBits();
    parseVPSExtension( pcVPS );   
    READ_FLAG( uiCode,  "vps_extension2_flag" );
    if (uiCode)
    {
#if H_3D
      READ_FLAG( uiCode,  "vps_3d_extension_flag" );
      if ( uiCode )
      {
        m_pcBitstream->readOutTrailingBits();
        pcVPS->createCamPars(pcVPS->getNumViews());
        parseVPS3dExtension( pcVPS );   
      }
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

  if( pcVPS->getMaxLayersMinus1() > 0  &&  pcVPS->getVpsBaseLayerInternalFlag() )
  {
    parsePTL( pcVPS->getPTL( 1 ),0, pcVPS->getMaxSubLayersMinus1()  );  
    
    pcVPS->getPTL( 1 )->inferGeneralValues ( false, 1, pcVPS->getPTL( 0 ) );
    pcVPS->getPTL( 1 )->inferSubLayerValues( pcVPS->getMaxSubLayersMinus1(), 1, pcVPS->getPTL( 0 ) );    
  }

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

  pcVPS->initNumViews(); 

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

#if HHI_INTER_COMP_PRED_K0052
#if H_3D
  pcVPS->initViewCompLayer( ); 
#endif
#endif

  for( Int i = 1; i <= pcVPS->getMaxLayersMinus1(); i++ )
  {
    for( Int j = 0; j < i; j++ )
    {
      READ_FLAG( uiCode, "direct_dependency_flag[i][j]" );             pcVPS->setDirectDependencyFlag( i, j, uiCode );
    }
  }
  pcVPS->setRefLayers(); 

  if ( pcVPS->getNumIndependentLayers() > 1 ) 
  {
    READ_UVLC( uiCode, "num_add_layer_sets"      ); pcVPS->setNumAddLayerSets( uiCode );
  }
  for (Int i = 0; i < pcVPS->getNumAddLayerSets(); i++)
  {
    for (Int j = 1; j < pcVPS->getNumIndependentLayers(); j++)
    {
      READ_CODE( pcVPS->getHighestLayerIdxPlus1Len( j ) , uiCode, "highest_layer_idx_plus1" ); pcVPS->setHighestLayerIdxPlus1( i, j, uiCode );
    }
    pcVPS->deriveAddLayerSetLayerIdList( i );
  }

  READ_FLAG( uiCode, "vps_sub_layers_max_minus1_present_flag" ); pcVPS->setVpsSubLayersMaxMinus1PresentFlag( uiCode == 1 );
  if ( pcVPS->getVpsSubLayersMaxMinus1PresentFlag() )
  {
    for (Int i = 0; i <= pcVPS->getMaxLayersMinus1(); i++ )
    {
      READ_CODE( 3, uiCode, "sub_layers_vps_max_minus1" ); pcVPS->setSubLayersVpsMaxMinus1( i, uiCode );    
      pcVPS->checkSubLayersVpsMaxMinus1( i ); 
    }
  }  
  else
  {
    for (Int i = 0; i <= pcVPS->getMaxLayersMinus1(); i++ )
    {
      pcVPS->setSubLayersVpsMaxMinus1( i, pcVPS->getMaxTLayers( ) - 1);    
    }
  }
  READ_FLAG( uiCode, "max_tid_ref_present_flag" ); pcVPS->setMaxTidRefPresentFlag( uiCode == 1 );

  if ( pcVPS->getMaxTidRefPresentFlag() )
  {    
    for( Int i = 0; i < pcVPS->getMaxLayersMinus1(); i++ )
    {
      for( Int j = i + 1; j <= pcVPS->getMaxLayersMinus1(); j++ )
      {
        if ( pcVPS->getDirectDependencyFlag(j,i) )
        {
          READ_CODE( 3, uiCode, "max_tid_il_ref_pics_plus1" ); pcVPS->setMaxTidIlRefPicsPlus1( i, j, uiCode );
        }
      }
    }
  }

  READ_FLAG( uiCode, "all_ref_layers_active_flag" );             pcVPS->setAllRefLayersActiveFlag( uiCode == 1 );
  READ_UVLC( uiCode, "vps_num_profile_tier_level_minus1" );  pcVPS->setVpsNumProfileTierLevelMinus1( uiCode );  

  Int offsetVal =  ( pcVPS->getMaxLayersMinus1() > 0  &&  pcVPS->getVpsBaseLayerInternalFlag() ) ? 2 : 1;   
  for( Int i = pcVPS->getVpsBaseLayerInternalFlag() ? 2 : 1; i <= pcVPS->getVpsNumProfileTierLevelMinus1(); i++ )
  {
    READ_FLAG(  uiCode, "vps_profile_present_flag[i]" );    pcVPS->setVpsProfilePresentFlag( i, uiCode == 1 );
    parsePTL ( pcVPS->getPTL( offsetVal ), pcVPS->getVpsProfilePresentFlag( i ), pcVPS->getMaxTLayers()-1);
    pcVPS->getPTL( offsetVal )->inferGeneralValues ( pcVPS->getVpsProfilePresentFlag( i ), offsetVal, pcVPS->getPTL( offsetVal - 1 ) );    
    pcVPS->getPTL( offsetVal )->inferSubLayerValues( pcVPS->getMaxSubLayersMinus1()      , offsetVal, pcVPS->getPTL( offsetVal - 1 ) );    
    offsetVal++;
  }


  if (pcVPS->getNumLayerSets() > 1)
  {
    READ_UVLC( uiCode, "num_add_olss" ); pcVPS->setNumAddOlss( uiCode );
    READ_CODE( 2, uiCode, "default_output_layer_idc" ); pcVPS->setDefaultOutputLayerIdc( std::min( uiCode, (UInt) 2 ) );    
  }

  pcVPS->initTargetLayerIdLists( ); 


  pcVPS->setOutputLayerFlag(0, 0, pcVPS->inferOutputLayerFlag( 0, 0 )); 
  pcVPS->setLayerSetIdxForOlsMinus1(0, -1); 

  pcVPS->deriveNecessaryLayerFlags( 0 ); 
  pcVPS->deriveTargetLayerIdList( 0 ); 

  if (pcVPS->getVpsBaseLayerInternalFlag() )
  {  
    pcVPS->setProfileTierLevelIdx(0,0, pcVPS->inferProfileTierLevelIdx(0,0) );
  }
  for( Int i = 1; i < pcVPS->getNumOutputLayerSets( ); i++ )
  {
    if( pcVPS->getNumLayerSets() > 2 && i >= pcVPS->getNumLayerSets( ) )    
    {        
      READ_CODE( pcVPS->getLayerSetIdxForOlsMinus1Len( i ), uiCode, "layer_set_idx_for_ols_minus1[i]" ); pcVPS->setLayerSetIdxForOlsMinus1( i, uiCode ); 
    }

    if ( i > pcVPS->getVpsNumLayerSetsMinus1() || pcVPS->getDefaultOutputLayerIdc() == 2 )
    {       
      for( Int j = 0; j < pcVPS->getNumLayersInIdList( pcVPS->olsIdxToLsIdx( i ) ); j++ )
      {
        READ_FLAG( uiCode, "output_layer_flag" ); pcVPS->setOutputLayerFlag( i, j, uiCode == 1 ); 
      }
    }
    else
    { 
      for( Int j = 0; j < pcVPS->getNumLayersInIdList( pcVPS->olsIdxToLsIdx( i ) ); j++ )
      {              
        pcVPS->setOutputLayerFlag(i,j, pcVPS->inferOutputLayerFlag( i, j )); 
      }
    }
    pcVPS->deriveNecessaryLayerFlags( i ); 
    pcVPS->deriveTargetLayerIdList( i ); 

    for ( Int j = 0; j < pcVPS->getNumLayersInIdList( pcVPS->olsIdxToLsIdx(i)); j++ )
    {    
      if (pcVPS->getNecessaryLayerFlag( i, j ) && pcVPS->getVpsNumProfileTierLevelMinus1() > 0 )
      {
        READ_CODE( pcVPS->getProfileTierLevelIdxLen(), uiCode,"profile_tier_level_idx[ i ][ j ]" );   pcVPS->setProfileTierLevelIdx( i, j, uiCode ); 
      }
      if (pcVPS->getNecessaryLayerFlag( i, j ) && pcVPS->getVpsNumProfileTierLevelMinus1() == 0 )
      {
        pcVPS->setProfileTierLevelIdx( i , j, pcVPS->inferProfileTierLevelIdx( i, j) );
      }
    }

    if( pcVPS->getNumOutputLayersInOutputLayerSet( i ) == 1 && pcVPS->getNumDirectRefLayers( pcVPS->getOlsHighestOutputLayerId( i ) ) > 0 )
    {
      READ_FLAG( uiCode, "alt_output_layer_flag[ i ]" ); pcVPS->setAltOutputLayerFlag( i, uiCode == 1 );
    }
  }

  READ_UVLC( uiCode, "vps_num_rep_formats_minus1" ); pcVPS->setVpsNumRepFormatsMinus1( uiCode );

  for (Int i = 0; i <= pcVPS->getVpsNumRepFormatsMinus1(); i++ )
  { 
    assert( pcVPS->getRepFormat(i) == NULL ); 
    TComRepFormat* curRepFormat = new TComRepFormat(); 
    TComRepFormat* prevRepFormat = i > 0 ? pcVPS->getRepFormat( i - 1) : NULL; 
    parseRepFormat( i, curRepFormat ,  prevRepFormat); 
    pcVPS->setRepFormat(i, curRepFormat ); 
  }

  if ( pcVPS->getVpsNumRepFormatsMinus1() > 0 )
  {
    READ_FLAG( uiCode, "rep_format_idx_present_flag" ); pcVPS->setRepFormatIdxPresentFlag( uiCode == 1 );
  }
  if( pcVPS->getRepFormatIdxPresentFlag() ) 
  {
    for( Int i = pcVPS->getVpsBaseLayerInternalFlag() ? 1 : 0; i <=  pcVPS->getMaxLayersMinus1(); i++ )
    {
      READ_CODE( pcVPS->getVpsRepFormatIdxLen(), uiCode, "vps_rep_format_idx[i]" ); pcVPS->setVpsRepFormatIdx( i, uiCode );
    }
  }
  else
  {
    for( Int i = pcVPS->getVpsBaseLayerInternalFlag() ? 1 : 0; i <=  pcVPS->getMaxLayersMinus1(); i++ )
    {
      pcVPS->setVpsRepFormatIdx( i, pcVPS->inferVpsRepFormatIdx( i ) );
    }
  }

  READ_FLAG( uiCode, "max_one_active_ref_layer_flag" ); pcVPS->setMaxOneActiveRefLayerFlag ( uiCode == 1 ); 

  READ_FLAG( uiCode, "vps_poc_lsb_aligned_flag" ); pcVPS->setVpsPocLsbAlignedFlag( uiCode == 1 );
  for( Int i = 1; i  <=  pcVPS->getMaxLayersMinus1(); i++ )
  {
    if( pcVPS->getNumDirectRefLayers( pcVPS->getLayerIdInNuh( i ) )  ==  0 )
    {      
      READ_FLAG( uiCode, "poc_lsb_not_present_flag" ); pcVPS->setPocLsbNotPresentFlag( i, uiCode == 1 );
    }
  }

  parseDpbSize( pcVPS ); 

  READ_UVLC( uiCode, "direct_dep_type_len_minus2")    ; pcVPS->setDirectDepTypeLenMinus2   ( uiCode ); 

  READ_FLAG( uiCode, "default_direct_dependency_flag" ); pcVPS->setDefaultDirectDependencyFlag( uiCode == 1 );
  if ( pcVPS->getDefaultDirectDependencyFlag( ) )
  {  
    READ_CODE( pcVPS->getDirectDepTypeLenMinus2( ) + 2, uiCode, "default_direct_dependency_type" ); pcVPS->setDefaultDirectDependencyType( uiCode );
  }

  for( Int i = pcVPS->getVpsBaseLayerInternalFlag() ?  1 : 2; i <= pcVPS->getMaxLayersMinus1(); i++ )
  {
    for( Int j = pcVPS->getVpsBaseLayerInternalFlag() ?  0 : 1; j < i; j++ )
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
  READ_UVLC( uiCode, "vps_non_vui_extension_length" ); pcVPS->setVpsNonVuiExtensionLength( uiCode ); 
  for ( Int i = 1; i <= pcVPS->getVpsNonVuiExtensionLength(); i++ )
  {
    READ_CODE( 8, uiCode, "vps_non_vui_extension_data_byte" );
  }
  READ_FLAG( uiCode, "vps_vui_present_flag" );  pcVPS->setVpsVuiPresentFlag( uiCode == 1 );
  if( pcVPS->getVpsVuiPresentFlag() )
  {
    m_pcBitstream->readOutTrailingBits(); // vps_vui_alignment_bit_equal_to_one
    parseVPSVUI( pcVPS ); 
  }     
  else
  {
    TComVPSVUI* pcVPSVUI = pcVPS->getVPSVUI( ); 
    assert( pcVPSVUI ); 
    pcVPSVUI->inferVpsVui( false ); 
  }
  pcVPS->checkVPSExtensionSyntax(); 
}

Void TDecCavlc::parseRepFormat( Int i, TComRepFormat* pcRepFormat, TComRepFormat* pcPrevRepFormat )
{
  assert( pcRepFormat ); 

  UInt uiCode; 

  READ_CODE( 16, uiCode, "pic_width_vps_in_luma_samples" );  pcRepFormat->setPicWidthVpsInLumaSamples ( uiCode );
  READ_CODE( 16, uiCode, "pic_height_vps_in_luma_samples" ); pcRepFormat->setPicHeightVpsInLumaSamples( uiCode );
  READ_FLAG( uiCode, "chroma_and_bit_depth_vps_present_flag" ); pcRepFormat->setChromaAndBitDepthVpsPresentFlag( uiCode == 1 );

  pcRepFormat->checkChromaAndBitDepthVpsPresentFlag( i ); 

  if ( pcRepFormat->getChromaAndBitDepthVpsPresentFlag() )
  {  
    READ_CODE( 2,  uiCode, "chroma_format_vps_idc" );          pcRepFormat->setChromaFormatVpsIdc       ( uiCode );
    if ( pcRepFormat->getChromaFormatVpsIdc() == 3 )
    {
      READ_FLAG( uiCode, "separate_colour_plane_vps_flag" ); pcRepFormat->setSeparateColourPlaneVpsFlag( uiCode == 1 ); 
    }
    READ_CODE( 4,  uiCode, "bit_depth_vps_luma_minus8" );      pcRepFormat->setBitDepthVpsLumaMinus8    ( uiCode );
    READ_CODE( 4,  uiCode, "bit_depth_vps_chroma_minus8" );    pcRepFormat->setBitDepthVpsChromaMinus8  ( uiCode );
  }
  else
  {
    pcRepFormat->inferChromaAndBitDepth(pcPrevRepFormat, false ); 
  }
  READ_FLAG( uiCode, "conformance_window_vps_flag" ); pcRepFormat->setConformanceWindowVpsFlag( uiCode == 1 );
  if ( pcRepFormat->getConformanceWindowVpsFlag() )
  {
    READ_UVLC( uiCode, "conf_win_vps_left_offset" ); pcRepFormat->setConfWinVpsLeftOffset( uiCode );
    READ_UVLC( uiCode, "conf_win_vps_right_offset" ); pcRepFormat->setConfWinVpsRightOffset( uiCode );
    READ_UVLC( uiCode, "conf_win_vps_top_offset" ); pcRepFormat->setConfWinVpsTopOffset( uiCode );
    READ_UVLC( uiCode, "conf_win_vps_bottom_offset" ); pcRepFormat->setConfWinVpsBottomOffset( uiCode );
  }
}


Void TDecCavlc::parseVPSVUI( TComVPS* pcVPS )
{
  assert( pcVPS ); 

  TComVPSVUI* pcVPSVUI = pcVPS->getVPSVUI( ); 

  assert( pcVPSVUI ); 

  UInt uiCode; 
  READ_FLAG( uiCode, "cross_layer_pic_type_aligned_flag" ); pcVPSVUI->setCrossLayerPicTypeAlignedFlag( uiCode == 1 );
  if ( !pcVPSVUI->getCrossLayerPicTypeAlignedFlag() )
  {  
    READ_FLAG( uiCode, "cross_layer_irap_aligned_flag" ); pcVPSVUI->setCrossLayerIrapAlignedFlag( uiCode == 1 );
  }
  if( pcVPSVUI->getCrossLayerIrapAlignedFlag( ) )
  {
    READ_FLAG( uiCode, "all_layers_idr_aligned_flag" ); pcVPSVUI->setAllLayersIdrAlignedFlag( uiCode == 1 );
  }
  READ_FLAG( uiCode, "bit_rate_present_vps_flag" ); pcVPSVUI->setBitRatePresentVpsFlag( uiCode == 1 );
  READ_FLAG( uiCode, "pic_rate_present_vps_flag" ); pcVPSVUI->setPicRatePresentVpsFlag( uiCode == 1 );
  if( pcVPSVUI->getBitRatePresentVpsFlag( )  ||  pcVPSVUI->getPicRatePresentVpsFlag( ) )
  {
    for( Int i = pcVPS->getVpsBaseLayerInternalFlag() ? 0 : 1; i  <  pcVPS->getNumLayerSets(); i++ )
    {
      for( Int j = 0; j  <=  pcVPS->getMaxSubLayersInLayerSetMinus1( i ); j++ ) 
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

  READ_FLAG( uiCode, "video_signal_info_idx_present_flag" ); pcVPSVUI->setVideoSignalInfoIdxPresentFlag( uiCode == 1 );
  if( pcVPSVUI->getVideoSignalInfoIdxPresentFlag() )
  {
    READ_CODE( 4, uiCode, "vps_num_video_signal_info_minus1" ); pcVPSVUI->setVpsNumVideoSignalInfoMinus1( uiCode );
  }
  else
  {
    pcVPSVUI->setVpsNumVideoSignalInfoMinus1( pcVPS->getMaxLayersMinus1() - pcVPS->getVpsBaseLayerInternalFlag() ? 0 : 1 ); 
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
    for( Int i = pcVPS->getVpsBaseLayerInternalFlag() ? 0 : 1; i <=  pcVPS->getMaxLayersMinus1(); i++ )
    {
      READ_CODE( 4, uiCode, "vps_video_signal_info_idx" ); pcVPSVUI->setVpsVideoSignalInfoIdx( i, uiCode );
    }
  }
  else if ( !pcVPSVUI->getVideoSignalInfoIdxPresentFlag() )
  {
    for( Int i = pcVPS->getVpsBaseLayerInternalFlag() ? 0 : 1; i <=  pcVPS->getMaxLayersMinus1(); i++ )
    {
      pcVPSVUI->setVpsVideoSignalInfoIdx( i, i );
    }
  }
  else
  {
    for( Int i = pcVPS->getVpsBaseLayerInternalFlag() ? 0 : 1; i <=  pcVPS->getMaxLayersMinus1(); i++ )
    {
      pcVPSVUI->setVpsVideoSignalInfoIdx( i, 0 );
    }
  }

  READ_FLAG( uiCode, "tiles_not_in_use_flag" ); pcVPSVUI->setTilesNotInUseFlag( uiCode == 1 );
  if( !pcVPSVUI->getTilesNotInUseFlag() ) 
  {      
    for( Int i = pcVPS->getVpsBaseLayerInternalFlag() ? 0 : 1; i  <=  pcVPS->getMaxLayersMinus1(); i++ )
    {
      READ_FLAG( uiCode, "tiles_in_use_flag[i]" ); pcVPSVUI->setTilesInUseFlag( i, uiCode == 1 );
      if( pcVPSVUI->getTilesInUseFlag( i ) )  
      {
        READ_FLAG( uiCode, "loop_filter_not_across_tiles_flag[i]" ); pcVPSVUI->setLoopFilterNotAcrossTilesFlag( i, uiCode == 1 );
      }
    }  
    for( Int i = pcVPS->getVpsBaseLayerInternalFlag() ? 1 : 2; i  <=  pcVPS->getMaxLayersMinus1(); i++ )  
    {
      for( Int j = 0; j < pcVPS->getNumDirectRefLayers( pcVPS->getLayerIdInNuh( i ) ) ; j++ )
      {
        Int layerIdx = pcVPS->getLayerIdInVps(pcVPS->getIdDirectRefLayer(pcVPS->getLayerIdInNuh( i ) , j  ));  
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
  READ_FLAG( uiCode, "single_layer_for_non_irap_flag" ); pcVPSVUI->setSingleLayerForNonIrapFlag( uiCode == 1 );
  READ_FLAG( uiCode, "higher_layer_irap_skip_flag" ); pcVPSVUI->setHigherLayerIrapSkipFlag( uiCode == 1 );
  READ_FLAG( uiCode, "ilp_restricted_ref_layers_flag" ); pcVPSVUI->setIlpRestrictedRefLayersFlag( uiCode == 1 );

  if( pcVPSVUI->getIlpRestrictedRefLayersFlag( ) )
  {
    for( Int i = 1; i  <=  pcVPS->getMaxLayersMinus1(); i++ )
    {
      for( Int j = 0; j < pcVPS->getNumDirectRefLayers( pcVPS->getLayerIdInNuh( i ) ); j++ )
      {
        if( pcVPS->getVpsBaseLayerInternalFlag() || pcVPS->getIdDirectRefLayer( pcVPS->getLayerIdInNuh( i ), j ) > 0 )
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
  }

  READ_FLAG( uiCode, "vps_vui_bsp_hrd_present_flag" ); pcVPSVUI->setVpsVuiBspHrdPresentFlag( uiCode == 1 );
  if ( pcVPSVUI->getVpsVuiBspHrdPresentFlag( ) )
  {
    assert(pcVPS->getTimingInfo()->getTimingInfoPresentFlag() == 1);
    parseVpsVuiBspHrdParameters( pcVPS ); 
  }
  for( Int i = 1; i  <=  pcVPS->getMaxLayersMinus1(); i++ )
  {
    if( pcVPS->getNumDirectRefLayers( pcVPS->getLayerIdInNuh( i )) == 0 ) 
    {
      READ_FLAG( uiCode, "base_layer_parameter_set_compatibility_flag" ); pcVPSVUI->setBaseLayerParameterSetCompatibilityFlag( i, uiCode == 1 );
    }
  }
}

Void TDecCavlc::parseVpsVuiBspHrdParameters( TComVPS* pcVPS )
{
  assert( pcVPS ); 

  TComVPSVUI* pcVPSVUI = pcVPS->getVPSVUI( ); 

  assert( pcVPSVUI ); 

  TComVpsVuiBspHrdParameters*  vpsVuiBspHrdP = pcVPSVUI->getVpsVuiBspHrdParameters(); 
  assert( vpsVuiBspHrdP == NULL ); 
  vpsVuiBspHrdP = new TComVpsVuiBspHrdParameters; 
  pcVPSVUI->setVpsVuiBspHrdParameters( vpsVuiBspHrdP ); 
  UInt uiCode; 
  READ_UVLC( uiCode, "vps_num_add_hrd_params" ); vpsVuiBspHrdP->setVpsNumAddHrdParams( uiCode );
  vpsVuiBspHrdP->createAfterVpsNumAddHrdParams( pcVPS ); 
  for( Int i = pcVPS->getNumHrdParameters(); i < pcVPS->getNumHrdParameters() + vpsVuiBspHrdP->getVpsNumAddHrdParams(); i++ )
  {  
    if( i > 0 )  
    {
      READ_FLAG( uiCode, "cprms_add_present_flag" ); vpsVuiBspHrdP->setCprmsAddPresentFlag( i, uiCode == 1 );
    }
    else
    {
       vpsVuiBspHrdP->setCprmsAddPresentFlag( i, true );
    }

    READ_UVLC( uiCode, "num_sub_layer_hrd_minus1" ); vpsVuiBspHrdP->setNumSubLayerHrdMinus1( i, uiCode );
    TComHRD* hrdParameters = vpsVuiBspHrdP->getHrdParametermeters( i ); 
    parseHrdParameters( hrdParameters, vpsVuiBspHrdP->getCprmsAddPresentFlag( i ), vpsVuiBspHrdP->getNumSubLayerHrdMinus1( i ) );     
  }

  vpsVuiBspHrdP->setNumPartitionsInSchemeMinus1( 0, 0, 0);
  vpsVuiBspHrdP->createAfterNumPartitionsInSchemeMinus1( 0, 0 );

  for( Int h = 0; h < pcVPS->getNumOutputLayerSets(); h++ )
  { 
    if ( h == 0)
    {
      vpsVuiBspHrdP->setNumSignalledPartitioningSchemes( h, 0 );
    }
    else
    {
      READ_UVLC( uiCode, "num_signalled_partitioning_schemes" ); vpsVuiBspHrdP->setNumSignalledPartitioningSchemes( h, uiCode );
    }    
    vpsVuiBspHrdP->createAfterNumSignalledPartitioningSchemes( h );

    for( Int j = 0; j < vpsVuiBspHrdP->getNumSignalledPartitioningSchemes( h ) + 1; j++ )
    { 
      if ( j == 0 && h == 0 )
      {
        vpsVuiBspHrdP->setNumPartitionsInSchemeMinus1( h, j, uiCode );
      }
      else if( j == 0 )
      {
        vpsVuiBspHrdP->setNumPartitionsInSchemeMinus1( h, j, pcVPS->getNumLayersInIdList( h ) - 1 );
      }
      else
      {
        READ_UVLC( uiCode, "num_partitions_in_scheme_minus1" ); vpsVuiBspHrdP->setNumPartitionsInSchemeMinus1( h, j, uiCode );
      }
      vpsVuiBspHrdP->createAfterNumPartitionsInSchemeMinus1( h, j );

      for( Int k = 0; k  <=  vpsVuiBspHrdP->getNumPartitionsInSchemeMinus1( h, j ); k++ )  
      {
        for( Int r = 0; r < pcVPS->getNumLayersInIdList(pcVPS->olsIdxToLsIdx( h ) )   ; r++ )  
        {
          if( h == 0 && j == 0 && k == 0 && r == 0 )
          {
             vpsVuiBspHrdP->setLayerIncludedInPartitionFlag( h, j, k, r, true );
          }
          else if ( h > 0 && j == 0 )
          {
             vpsVuiBspHrdP->setLayerIncludedInPartitionFlag( h, j, k, r, (k == r) );
          }
          else
          {
            READ_FLAG( uiCode, "layer_included_in_partition_flag" ); vpsVuiBspHrdP->setLayerIncludedInPartitionFlag( h, j, k, r, uiCode == 1 );
          }          
        }
      }
    }  
    if ( h > 0 )
    {
      for( Int i = 0; i < vpsVuiBspHrdP->getNumSignalledPartitioningSchemes( h ) + 1; i++ )  
      {
        for( Int t = 0; t  <=  pcVPS->getMaxSubLayersInLayerSetMinus1( pcVPS->olsIdxToLsIdx( i ) ); t++ )
        {  
          READ_UVLC( uiCode, "num_bsp_schedules_minus1" ); vpsVuiBspHrdP->setNumBspSchedulesMinus1( h, i, t, uiCode );
          vpsVuiBspHrdP->createAfterNumBspSchedulesMinus1( h, i, t );
          for( Int j = 0; j  <=  vpsVuiBspHrdP->getNumBspSchedulesMinus1( h, i, t ); j++ )  
          {
            for( Int k = 0; k  <=  vpsVuiBspHrdP->getNumPartitionsInSchemeMinus1( h, j ); k++ )
            {  
              READ_CODE( vpsVuiBspHrdP->getBspHrdIdxLen( pcVPS ), uiCode, "bsp_hrd_idx" ); vpsVuiBspHrdP->setBspHrdIdx( h, i, t, j, k, uiCode );
              READ_UVLC( uiCode, "bsp_sched_idx" ); vpsVuiBspHrdP->setBspSchedIdx( h, i, t, j, k, uiCode );
            }  
          }
        }  
      }
    }
  }  
}

Void TDecCavlc::parseVideoSignalInfo( TComVideoSignalInfo* pcVideoSignalInfo ) 
{
  UInt uiCode; 
  READ_CODE( 3, uiCode, "video_vps_format" );             pcVideoSignalInfo->setVideoVpsFormat( uiCode );
  READ_FLAG( uiCode, "video_full_range_vps_flag" );       pcVideoSignalInfo->setVideoFullRangeVpsFlag( uiCode == 1 );
  READ_CODE( 8, uiCode, "colour_primaries_vps" );         pcVideoSignalInfo->setColourPrimariesVps( uiCode );
  READ_CODE( 8, uiCode, "transfer_characteristics_vps" ); pcVideoSignalInfo->setTransferCharacteristicsVps( uiCode );
  READ_CODE( 8, uiCode, "matrix_coeffs_vps" );            pcVideoSignalInfo->setMatrixCoeffsVps( uiCode );
}

Void TDecCavlc::parseDpbSize( TComVPS* vps )
{
  UInt uiCode; 
  TComDpbSize* dpbSize = vps->getDpbSize(); 
  assert ( dpbSize != 0 ); 

  for( Int i = 1; i < vps->getNumOutputLayerSets(); i++ )
  {  
    Int currLsIdx = vps->olsIdxToLsIdx( i ); 
    READ_FLAG( uiCode, "sub_layer_flag_info_present_flag" ); dpbSize->setSubLayerFlagInfoPresentFlag( i, uiCode == 1 );
    for( Int j = 0; j  <=  vps->getMaxSubLayersInLayerSetMinus1( currLsIdx ); j++ )
    {  
      if( j > 0  &&  dpbSize->getSubLayerDpbInfoPresentFlag( i, j )  )  
      {
        READ_FLAG( uiCode, "sub_layer_dpb_info_present_flag" ); dpbSize->setSubLayerDpbInfoPresentFlag( i, j, uiCode == 1 );
      }
      if( dpbSize->getSubLayerDpbInfoPresentFlag( i, j ) )
      {  
        for( Int k = 0; k < vps->getNumLayersInIdList( currLsIdx ); k++ )   
        {
          if ( vps->getNecessaryLayerFlag( i, k ) && ( vps->getVpsBaseLayerInternalFlag() || ( vps->getLayerSetLayerIdList(vps->olsIdxToLsIdx(i),k) != 0 ) ))
          {
            READ_UVLC( uiCode, "max_vps_dec_pic_buffering_minus1" ); dpbSize->setMaxVpsDecPicBufferingMinus1( i, k, j, uiCode );
          }
          else
          {
            if ( vps->getNecessaryLayerFlag( i, k ) && ( j == 0 ) && ( k == 0 ))
            {
              dpbSize->setMaxVpsDecPicBufferingMinus1(i ,k, j, 0 );
            }
          }
        }
        READ_UVLC( uiCode, "max_vps_num_reorder_pics" ); dpbSize->setMaxVpsNumReorderPics( i, j, uiCode );
        READ_UVLC( uiCode, "max_vps_latency_increase_plus1" ); dpbSize->setMaxVpsLatencyIncreasePlus1( i, j, uiCode );
      }
      else
      {
        if ( j > 0 )
        {
          for( Int k = 0; k < vps->getNumLayersInIdList( vps->olsIdxToLsIdx( i ) ); k++ )   
          {
            if ( vps->getNecessaryLayerFlag(i, k ) )
            {            
              dpbSize->setMaxVpsDecPicBufferingMinus1( i, k, j, dpbSize->getMaxVpsDecPicBufferingMinus1( i,k, j - 1 ) );
            }
          }
          dpbSize->setMaxVpsNumReorderPics      ( i, j, dpbSize->getMaxVpsNumReorderPics      ( i, j - 1 ) );
          dpbSize->setMaxVpsLatencyIncreasePlus1( i, j, dpbSize->getMaxVpsLatencyIncreasePlus1( i, j - 1 ) );
        }
      }
    }  
  }  
}

#if H_3D
Void TDecCavlc::parseVPS3dExtension( TComVPS* pcVPS )
{
#if HHI_CAM_PARA_K0052
  UInt uiCode;   
  READ_UVLC( uiCode, "cp_precision"); pcVPS->setCpPrecision( uiCode ) ;
  
  for (Int n = 1; n < pcVPS->getNumViews(); n++)
  {
    Int i      = pcVPS->getViewOIdxList( n );
    Int iInVps = pcVPS->getVoiInVps( i ); 
    READ_CODE( 6, uiCode, "num_cp" ); pcVPS->setNumCp( iInVps, uiCode );

    if( pcVPS->getNumCp( iInVps ) > 0 )
    {
      READ_FLAG( uiCode, "cp_in_slice_segment_header_flag" ); pcVPS->setCpInSliceSegmentHeaderFlag( iInVps, uiCode == 1 );
      for( Int m = 0; m < pcVPS->getNumCp( iInVps ); m++ )
      {
        READ_UVLC( uiCode, "cp_ref_voi" ); pcVPS->setCpRefVoi( iInVps, m, uiCode );
        if( !pcVPS->getCpInSliceSegmentHeaderFlag( iInVps ) )
        {
          Int j      = pcVPS->getCpRefVoi( iInVps, m );
          Int jInVps = pcVPS->getVoiInVps( j ); 
          Int iCode;
          READ_SVLC( iCode, "vps_cp_scale" );                pcVPS->setVpsCpScale   ( iInVps, jInVps, iCode );
          READ_SVLC( iCode, "vps_cp_off" );                  pcVPS->setVpsCpOff     ( iInVps, jInVps, iCode );
          READ_SVLC( iCode, "vps_cp_inv_scale_plus_scale" ); pcVPS->setVpsCpInvScale( iInVps, jInVps, iCode - pcVPS->getVpsCpScale( iInVps, jInVps ) );
          READ_SVLC( iCode, "vps_cp_inv_off_plus_off" );     pcVPS->setVpsCpInvOff  ( iInVps, jInVps, iCode - pcVPS->getVpsCpOff  ( iInVps, jInVps ) );
        }
      }
    }    
  }
  pcVPS->deriveCpPresentFlag(); 
#else
  UInt uiCode; 

  UInt uiCamParPrecision = 0; 
  Bool bCamParSlice      = false; 
  Bool bCamParPresentFlag = false;

  READ_UVLC( uiCamParPrecision, "cp_precision" );
#if HHI_VIEW_ID_LIST_I5_J0107
  for (Int n = 1; n < pcVPS->getNumViews(); n++)
  {
    Int viewIndex = pcVPS->getViewOIdxList( n ); 
#else
  for (UInt viewIndex=1; viewIndex<pcVPS->getNumViews(); viewIndex++)
  {
#endif
    pcVPS->setCamParPresent         ( viewIndex, false );
    pcVPS->setHasCamParInSliceHeader( viewIndex, false );
    READ_FLAG( uiCode, "cp_present_flag[i]" );                  bCamParPresentFlag = ( uiCode == 1);
    if ( bCamParPresentFlag )
    {
      READ_FLAG( uiCode, "cp_in_slice_segment_header_flag[i]" );          bCamParSlice = ( uiCode == 1);
      if ( !bCamParSlice )
      {
#if HHI_VIEW_ID_LIST_I5_J0107
        for( UInt m = 0; m < n; n++ )
        {
          Int uiBaseIndex = pcVPS->getViewOIdxList ( m ); 
          Int iCode; 
          READ_SVLC( iCode, "vps_cp_scale" );                m_aaiTempScale  [ uiBaseIndex ][ viewIndex ]   = iCode;
          READ_SVLC( iCode, "vps_cp_off" );                  m_aaiTempOffset [ uiBaseIndex ][ viewIndex ]   = iCode;
          READ_SVLC( iCode, "vps_cp_inv_scale_plus_scale" ); m_aaiTempScale  [ viewIndex   ][ uiBaseIndex ] = iCode - m_aaiTempScale [ uiBaseIndex ][ viewIndex ];
          READ_SVLC( iCode, "vps_cp_inv_off_plus_off" );     m_aaiTempOffset [ viewIndex   ][ uiBaseIndex ] = iCode - m_aaiTempOffset[ uiBaseIndex ][ viewIndex ];
        }
      }
      pcVPS->initCamParaVPS( viewIndex, bCamParPresentFlag, uiCamParPrecision, bCamParSlice, m_aaiTempScale, m_aaiTempOffset ); 
#else
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
#endif
    }
  }
#endif
}
#endif
#endif
#if H_MV
Void TDecCavlc::parseSliceHeader (TComSlice*& rpcSlice, ParameterSetManagerDecoder *parameterSetManager, Int targetOlsIdx)
#else
Void TDecCavlc::parseSliceHeader (TComSlice*& rpcSlice, ParameterSetManagerDecoder *parameterSetManager)
#endif
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
    READ_FLAG( uiCode, "no_output_of_prior_pics_flag" );  //ignored -- updated already
#if SETTING_NO_OUT_PIC_PRIOR
    rpcSlice->setNoOutputPriorPicsFlag(uiCode ? true : false);
#else
    rpcSlice->setNoOutputPicPrior( false );
#endif
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
  sps->inferSpsMaxDecPicBufferingMinus1( vps, targetOlsIdx, rpcSlice->getLayerId(), false ); 

  if ( sps->getVuiParametersPresentFlag() )
  {
    sps->getVuiParameters()->inferVideoSignalInfo( vps, rpcSlice->getLayerId() ); 
  }
  rpcSlice->setVPS(vps);      
  rpcSlice->setViewId   ( vps->getViewId   ( rpcSlice->getLayerId() )      );
  rpcSlice->setViewIndex( vps->getViewIndex( rpcSlice->getLayerId() )      );  
#if H_3D  
  rpcSlice->setIsDepth  ( vps->getDepthId  ( rpcSlice->getLayerId() ) == 1 );
#endif
#endif
  rpcSlice->setSPS(sps);
#if !HHI_INTER_COMP_PRED_K0052
#if H_3D
  rpcSlice->init3dToolParameters();
#endif
#endif
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
  
#if H_MV
    UInt slicePicOrderCntLsb = 0;
#endif

  if(!rpcSlice->getDependentSliceSegmentFlag())
  {
#if H_MV    
    Int esb = 0; //Don't use i, otherwise will shadow something below

    if ( rpcSlice->getPPS()->getNumExtraSliceHeaderBits() > esb )
    {
      esb++; 
      READ_FLAG( uiCode, "discardable_flag" ); rpcSlice->setDiscardableFlag( uiCode == 1 );
      if ( uiCode == 1 )
      {
        assert(rpcSlice->getNalUnitType() != NAL_UNIT_CODED_SLICE_TRAIL_R &&
          rpcSlice->getNalUnitType() != NAL_UNIT_CODED_SLICE_TSA_R &&
          rpcSlice->getNalUnitType() != NAL_UNIT_CODED_SLICE_STSA_R &&
          rpcSlice->getNalUnitType() != NAL_UNIT_CODED_SLICE_RADL_R &&
          rpcSlice->getNalUnitType() != NAL_UNIT_CODED_SLICE_RASL_R);
      }
    }

    if ( rpcSlice->getPPS()->getNumExtraSliceHeaderBits() > esb )
    {
      esb++; 
      READ_FLAG( uiCode, "cross_layer_bla_flag" ); rpcSlice->setCrossLayerBlaFlag( uiCode == 1 );
    }
    rpcSlice->checkCrossLayerBlaFlag( ); 


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

#if H_3D_DISABLE_CHROMA
    assert (sps->getChromaFormatIdc() == 1 || rpcSlice->getIsDepth() );
    assert (sps->getChromaFormatIdc() == 0 || !rpcSlice->getIsDepth() );
#else
    assert (sps->getChromaFormatIdc() == 1 );
#endif
    // if( separate_colour_plane_flag  ==  1 )
    //   colour_plane_id                                      u(2)


#if H_MV
    Int iPOClsb = slicePicOrderCntLsb;  // Needed later
    if ( (rpcSlice->getLayerId() > 0 && !vps->getPocLsbNotPresentFlag( rpcSlice->getLayerIdInVps())) || !rpcSlice->getIdrPicFlag() )
    {
      READ_CODE(sps->getBitsForPOC(), slicePicOrderCntLsb, "slice_pic_order_cnt_lsb");        
    }    
    rpcSlice->setSlicePicOrderCntLsb( slicePicOrderCntLsb ); 

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
#if !H_MV
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
#if !H_MV
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
#endif
      TComReferencePictureSet* rps;
      rps = rpcSlice->getLocalRPS();
      rpcSlice->setRPS(rps);
      READ_FLAG( uiCode, "short_term_ref_pic_set_sps_flag" );
      if(uiCode == 0) // use short-term reference picture set explicitly signalled in slice header
      {        
        parseShortTermRefPicSet(sps,rps, sps->getRPSList()->getNumberOfReferencePictureSets());
#if H_MV
        if ( !rps->getInterRPSPrediction( ) )
        { // check sum of num_positive_pics and num_negative_pics
          rps->checkMaxNumPics( 
            vps->getVpsExtensionFlag(), 
            MAX_INT,  // To be replaced by MaxDbpSize
            rpcSlice->getLayerId(), 
            sps->getMaxDecPicBuffering( sps->getSpsMaxSubLayersMinus1() ) - 1 );
        }
#endif
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
#if H_MV
      if ( !rps->getInterRPSPrediction( ) )
      { // check sum of NumPositivePics, NumNegativePics, num_long_term_sps and num_long_term_pics 
        rps->checkMaxNumPics( 
          vps->getVpsExtensionFlag(), 
            MAX_INT,  // To be replaced by MaxDbpsize
          rpcSlice->getLayerId(), 
          sps->getMaxDecPicBuffering( sps->getSpsMaxSubLayersMinus1() ) - 1 );
      }
#endif
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
#if H_MV
        READ_FLAG( uiCode, "slice_temporal_mvp_enabled_flag" );
#else
        READ_FLAG( uiCode, "slice_temporal_mvp_enable_flag" );
#endif
        rpcSlice->setEnableTMVPFlag( uiCode == 1 ? true : false ); 
      }
      else
      {
        rpcSlice->setEnableTMVPFlag(false);
      }
    }
#if H_MV
    Bool interLayerPredLayerIdcPresentFlag = false; 
    Int layerId       = rpcSlice->getLayerId(); 
#if H_3D
    if( rpcSlice->getLayerId() > 0 && !vps->getAllRefLayersActiveFlag() && vps->getNumRefListLayers( layerId ) > 0 )
#else
    if( rpcSlice->getLayerId() > 0 && !vps->getAllRefLayersActiveFlag() && vps->getNumDirectRefLayers( layerId ) > 0 )
#endif
    {   
      READ_FLAG( uiCode, "inter_layer_pred_enabled_flag" ); rpcSlice->setInterLayerPredEnabledFlag( uiCode == 1 );
#if H_3D
      if( rpcSlice->getInterLayerPredEnabledFlag() && vps->getNumRefListLayers( layerId ) > 1 )
#else
      if( rpcSlice->getInterLayerPredEnabledFlag() && vps->getNumDirectRefLayers( layerId ) > 1 )
#endif
      {            
        if( !vps->getMaxOneActiveRefLayerFlag())  
        {
          READ_CODE( rpcSlice->getNumInterLayerRefPicsMinus1Len( ), uiCode, "num_inter_layer_ref_pics_minus1" ); rpcSlice->setNumInterLayerRefPicsMinus1( uiCode );
        }
#if H_3D
        if ( rpcSlice->getNumActiveRefLayerPics() != vps->getNumRefListLayers( layerId ) )
#else
        if ( rpcSlice->getNumActiveRefLayerPics() != vps->getNumDirectRefLayers( layerId ) )
#endif
        {
          interLayerPredLayerIdcPresentFlag = true; 
          for( Int idx = 0; idx < rpcSlice->getNumActiveRefLayerPics(); idx++ )   
          {
            READ_CODE( rpcSlice->getInterLayerPredLayerIdcLen( ), uiCode, "inter_layer_pred_layer_idc" ); rpcSlice->setInterLayerPredLayerIdc( idx, uiCode );
          }
        }
      }  
    }
    if ( !interLayerPredLayerIdcPresentFlag )
    {
      for( Int i = 0; i < rpcSlice->getNumActiveRefLayerPics(); i++ )   
      {
        rpcSlice->setInterLayerPredLayerIdc( i, rpcSlice->getRefLayerPicIdc( i ) );
      }
    }
#if HHI_INTER_COMP_PRED_K0052
#if H_3D
    if ( getDecTop()->decProcAnnexI() )
    {    
      rpcSlice->deriveInCmpPredAndCpAvailFlag();
      if ( rpcSlice->getInCmpPredAvailFlag() )
      {
        READ_FLAG(uiCode, "in_comp_pred_flag");  rpcSlice->setInCompPredFlag((Bool)uiCode);      
      }
      rpcSlice->init3dToolParameters(); 
    }
#endif
#endif
#endif
    if(sps->getUseSAO())
    {
      READ_FLAG(uiCode, "slice_sao_luma_flag");  rpcSlice->setSaoEnabledFlag((Bool)uiCode);
#if H_3D_DISABLE_CHROMA
      if( !rpcSlice->getIsDepth() )
      {
      READ_FLAG(uiCode, "slice_sao_chroma_flag");  rpcSlice->setSaoEnabledFlagChroma((Bool)uiCode);
    }
      else
      {
        rpcSlice->setSaoEnabledFlagChroma( false );
      }
      
#else
      READ_FLAG(uiCode, "slice_sao_chroma_flag");  rpcSlice->setSaoEnabledFlagChroma((Bool)uiCode);
#endif
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
#if H_3D_ANNEX_SELECTION_FIX
    else if(    rpcSlice->getViewIndex() && ( rpcSlice->getSliceType() == P_SLICE || rpcSlice->getSliceType() == B_SLICE ) 
             && !rpcSlice->getIsDepth() && vps->getNumRefListLayers( layerId ) > 0 
             && getDecTop()->decProcAnnexI()
           )
#else
    else if( rpcSlice->getViewIndex() && ( rpcSlice->getSliceType() == P_SLICE || rpcSlice->getSliceType() == B_SLICE ) && !rpcSlice->getIsDepth() && vps->getNumRefListLayers( layerId ) > 0 )
#endif
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
      rpcSlice->setMaxNumMergeCand(( ( rpcSlice->getMpiFlag() || rpcSlice->getIvMvPredFlag() || rpcSlice->getViewSynthesisPredFlag() ) ? MRG_MAX_NUM_CANDS_MEM : MRG_MAX_NUM_CANDS) - uiCode);
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

#if HHI_CAM_PARA_K0052
#if H_3D
    if ( getDecTop()->decProcAnnexI() )
    {
      Int voiInVps = vps->getVoiInVps( rpcSlice->getViewIndex() ); 
      if( vps->getCpInSliceSegmentHeaderFlag( voiInVps ) && !rpcSlice->getIsDepth() )
      {
        for( Int m = 0; m < vps->getNumCp( voiInVps ); m++ )
        {
          Int jInVps = vps->getVoiInVps( vps->getCpRefVoi( voiInVps, m ));
          READ_SVLC( iCode, "cp_scale" );                rpcSlice->setCpScale   ( jInVps, iCode );
          READ_SVLC( iCode, "cp_off" );                  rpcSlice->setCpOff     ( jInVps, iCode );
          READ_SVLC( iCode, "cp_inv_scale_plus_scale" ); rpcSlice->setCpInvScale( jInVps, iCode - rpcSlice->getCpScale   ( jInVps ));
          READ_SVLC( iCode, "cp_inv_off_plus_off" );     rpcSlice->setCpInvOff  ( jInVps, iCode - rpcSlice->getCpOff     ( jInVps ));
        }
      }
    }
#endif
#endif
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

#if !HHI_CAM_PARA_K0052
#if H_3D
#if H_3D_FCO
  if( rpcSlice->getVPS()->hasCamParInSliceHeader( rpcSlice->getViewIndex() )  && rpcSlice->getIsDepth() )
#else
  if( rpcSlice->getVPS()->hasCamParInSliceHeader( rpcSlice->getViewIndex() )  && !rpcSlice->getIsDepth() )
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
#endif

  if(pps->getSliceHeaderExtensionPresentFlag())
  {
#if H_MV
    READ_UVLC( uiCode, "slice_segment_header_extension_length" ); rpcSlice->setSliceSegmentHeaderExtensionLength( uiCode );
    UInt posFollSliceSegHeaderExtLen = m_pcBitstream->getNumBitsRead();

    if( rpcSlice->getPPS()->getPocResetInfoPresentFlag() )
    {
      READ_CODE( 2, uiCode, "poc_reset_idc" ); rpcSlice->setPocResetIdc( uiCode );
    }
    else
    {
      rpcSlice->setPocResetIdc( 0 );
    }
    rpcSlice->checkPocResetIdc(); 

    if ( rpcSlice->getVPS()->getPocLsbNotPresentFlag(rpcSlice->getLayerId()) && slicePicOrderCntLsb > 0 )
    {
      assert( rpcSlice->getPocResetIdc() != 2 );
    }

    if( rpcSlice->getPocResetIdc() !=  0 )
    {
      READ_CODE( 6, uiCode, "poc_reset_period_id" ); rpcSlice->setPocResetPeriodId( uiCode );
    }
    else
    {
      // TODO Copy poc_reset_period from earlier picture
      rpcSlice->setPocResetPeriodId( 0 );
    }
    
    if( rpcSlice->getPocResetIdc() ==  3 ) 
    {
      READ_FLAG( uiCode, "full_poc_reset_flag" ); rpcSlice->setFullPocResetFlag( uiCode == 1 );
      READ_CODE( rpcSlice->getPocLsbValLen() , uiCode, "poc_lsb_val" ); rpcSlice->setPocLsbVal( uiCode );
    }          
    rpcSlice->checkPocLsbVal(); 

    // Derive the value of PocMs8bValRequiredFlag

    if( !rpcSlice->getPocMsbValRequiredFlag() && rpcSlice->getVPS()->getVpsPocLsbAlignedFlag() )
    {
      READ_FLAG( uiCode, "poc_msb_val_present_flag" ); rpcSlice->setPocMsbValPresentFlag( uiCode == 1 );
    }
    else
    {
      rpcSlice->setPocMsbValPresentFlag( rpcSlice->inferPocMsbValPresentFlag( ) ); 
    }

    
    if( rpcSlice->getPocMsbValPresentFlag() )
    {
      READ_UVLC( uiCode, "poc_msb_val" ); rpcSlice->setPocMsbVal( uiCode );
    }

    while( ( m_pcBitstream->getNumBitsRead() - posFollSliceSegHeaderExtLen ) < rpcSlice->getSliceSegmentHeaderExtensionLength() * 8 )
    {
     READ_FLAG( uiCode, "slice_segment_header_extension_data_bit" );
    }
    assert( ( m_pcBitstream->getNumBitsRead() - posFollSliceSegHeaderExtLen ) == rpcSlice->getSliceSegmentHeaderExtensionLength() * 8  ); 
  }
#else
    READ_UVLC( uiCode, "slice_header_extension_length" );
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
      // When profilePresentFlag is equal to 0, sub_layer_profile_present_flag[ i ] shall be equal to 0.
      assert( profilePresentFlag || !rpcPTL->getSubLayerProfilePresentFlag(i) );
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
#if H_MV
    if( rpcPTL->getSubLayerProfilePresentFlag(i) )         
#else
    if( profilePresentFlag && rpcPTL->getSubLayerProfilePresentFlag(i) )          
#endif
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
  
#if H_MV
  if( ptl->getV2ConstraintsPresentFlag() )
  {
    READ_FLAG( uiCode, "max_12bit_constraint_flag" );        ptl->setMax12bitConstraintFlag      ( uiCode == 1 );
    READ_FLAG( uiCode, "max_10bit_constraint_flag" );        ptl->setMax10bitConstraintFlag      ( uiCode == 1 );
    READ_FLAG( uiCode, "max_8bit_constraint_flag" );         ptl->setMax8bitConstraintFlag       ( uiCode == 1 );
    READ_FLAG( uiCode, "max_422chroma_constraint_flag" );    ptl->setMax422chromaConstraintFlag  ( uiCode == 1 );
    READ_FLAG( uiCode, "max_420chroma_constraint_flag" );    ptl->setMax420chromaConstraintFlag  ( uiCode == 1 );
    READ_FLAG( uiCode, "max_monochrome_constraint_flag" );   ptl->setMaxMonochromeConstraintFlag ( uiCode == 1 );
    READ_FLAG( uiCode, "intra_constraint_flag" );            ptl->setIntraConstraintFlag         ( uiCode == 1 );
    READ_FLAG( uiCode, "one_picture_only_constraint_flag" ); ptl->setOnePictureOnlyConstraintFlag( uiCode == 1 );
    READ_FLAG( uiCode, "lower_bit_rate_constraint_flag" );   ptl->setLowerBitRateConstraintFlag  ( uiCode == 1 );    
    READ_CODE(16, uiCode, "XXX_reserved_zero_34bits[0..15]");
    READ_CODE(16, uiCode, "XXX_reserved_zero_34bits[16..31]");
    READ_CODE(2 , uiCode, "XXX_reserved_zero_34bits[32..33]");
  }
  else
  {
    READ_CODE(16, uiCode, "XXX_reserved_zero_43bits[0..15]");
    READ_CODE(16, uiCode, "XXX_reserved_zero_43bits[16..31]");
    READ_CODE(11, uiCode, "XXX_reserved_zero_43bits[32..42]");
  }
  if( ptl->getInbldPresentFlag() )
  {
    READ_FLAG( uiCode, "inbld_flag" ); ptl->setInbldFlag( uiCode == 1 );
  }
  else
  {
    READ_FLAG(uiCode, "reserved_zero_bit");
  }
#else
  READ_CODE(16, uiCode, "XXX_reserved_zero_44bits[0..15]");
  READ_CODE(16, uiCode, "XXX_reserved_zero_44bits[16..31]");
  READ_CODE(12, uiCode, "XXX_reserved_zero_44bits[32..43]");
#endif
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

#if SEC_DEPTH_INTRA_SKIP_MODE_K0033
Void TDecCavlc::parseDIS( TComDataCU* /*pcCU*/, UInt /*uiAbsPartIdx*/, UInt /*uiDepth*/ )
{
  assert(0);
}
#else
#if H_3D_SINGLE_DEPTH
Void TDecCavlc::parseSingleDepthMode( TComDataCU* /*pcCU*/, UInt /*uiAbsPartIdx*/, UInt /*uiDepth*/ )
{
  assert(0);
}
#endif
#endif

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
Void TDecCavlc::parseDeltaDC( TComDataCU* /*pcCU*/, UInt /*uiAbsPartIdx*/, UInt /*uiDepth*/ )
{ 
  assert(0);
}

Void TDecCavlc::parseSDCFlag    ( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth )
{
  assert(0);
}

#endif
#if H_3D_DBBP
  Void TDecCavlc::parseDBBPFlag( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth )
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
#if H_3D_DISABLE_CHROMA
  Bool            bChroma     = !pcSlice->getIsDepth();
#else
  Bool            bChroma     = true; // color always present in HEVC ?
#endif
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
  else
  { 
    // For some reasons this is also needed to fix a compiler warning when H_3D_DISABLE_CHROMA is equal to 0.
    uiLog2WeightDenomChroma = 0; 
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

