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
#include "TLibCommon/TComChromaFormat.h"
#if RExt__DECODER_DEBUG_BIT_STATISTICS
#include "TLibCommon/TComCodingStatistics.h"
#endif
#if NH_MV
#include "TDecTop.h"
#endif

//! \ingroup TLibDecoder
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
#endif
#endif
// ====================================================================================================================
// Constructor / destructor / create / destroy
// ====================================================================================================================

TDecCavlc::TDecCavlc()
{
}

TDecCavlc::~TDecCavlc()
{

}

// ====================================================================================================================
// Public member functions
// ====================================================================================================================

Void TDecCavlc::parseShortTermRefPicSet( TComSPS* sps, TComReferencePictureSet* rps, Int idx )
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

Void TDecCavlc::parsePPS(TComPPS* pcPPS)
{
#if ENC_DEC_TRACE
#if H_MV_ENC_DEC_TRAC
  tracePSHeader( "PPS", pcPPS->getLayerId() ); 
#else
  xTracePPSHeader ();
#endif
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
  pcPPS->setQpOffset(COMPONENT_Cb, iCode);
  assert( pcPPS->getQpOffset(COMPONENT_Cb) >= -12 );
  assert( pcPPS->getQpOffset(COMPONENT_Cb) <=  12 );

  READ_SVLC( iCode, "pps_cr_qp_offset");
  pcPPS->setQpOffset(COMPONENT_Cr, iCode);
  assert( pcPPS->getQpOffset(COMPONENT_Cr) >= -12 );
  assert( pcPPS->getQpOffset(COMPONENT_Cr) <=  12 );

  assert(MAX_NUM_COMPONENT<=3);

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

    const UInt tileColumnsMinus1 = pcPPS->getNumTileColumnsMinus1();
    const UInt tileRowsMinus1    = pcPPS->getNumTileRowsMinus1();
 
    if ( !pcPPS->getTileUniformSpacingFlag())
    {
      if (tileColumnsMinus1 > 0)
      {
        std::vector<Int> columnWidth(tileColumnsMinus1);
        for(UInt i = 0; i < tileColumnsMinus1; i++)
        { 
          READ_UVLC( uiCode, "column_width_minus1" );  
          columnWidth[i] = uiCode+1;
        }
        pcPPS->setTileColumnWidth(columnWidth);
      }

      if (tileRowsMinus1 > 0)
      {
        std::vector<Int> rowHeight (tileRowsMinus1);
        for(UInt i = 0; i < tileRowsMinus1; i++)
        {
          READ_UVLC( uiCode, "row_height_minus1" );
          rowHeight[i] = uiCode + 1;
        }
        pcPPS->setTileRowHeight(rowHeight);
      }
    }

    if ((tileColumnsMinus1 + tileRowsMinus1) != 0)
    {
      READ_FLAG ( uiCode, "loop_filter_across_tiles_enabled_flag" );   pcPPS->setLoopFilterAcrossTilesEnabledFlag( uiCode ? true : false );
    }
  }
  READ_FLAG( uiCode, "pps_loop_filter_across_slices_enabled_flag" );   pcPPS->setLoopFilterAcrossSlicesEnabledFlag( uiCode ? true : false );
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
    parseScalingList( &(pcPPS->getScalingList()) );
  }

  READ_FLAG( uiCode, "lists_modification_present_flag");
  pcPPS->setListsModificationPresentFlag(uiCode);

  READ_UVLC( uiCode, "log2_parallel_merge_level_minus2");
  pcPPS->setLog2ParallelMergeLevelMinus2 (uiCode);

  READ_FLAG( uiCode, "slice_segment_header_extension_present_flag");
  pcPPS->setSliceHeaderExtensionPresentFlag(uiCode);

  READ_FLAG( uiCode, "pps_extension_present_flag");
  if (uiCode)
  {
#if NH_MV
    READ_FLAG( uiCode, "pps_range_extensions_flag" ); pcPPS->setPpsRangeExtensionsFlag( uiCode == 1 );
    READ_FLAG( uiCode, "pps_multilayer_extension_flag" ); pcPPS->setPpsMultilayerExtensionFlag( uiCode == 1 );
    READ_FLAG( uiCode, "pps_3d_extension_flag" ); pcPPS->setPps3dExtensionFlag( uiCode == 1 );
    READ_CODE( 5, uiCode, "pps_extension_5bits" ); pcPPS->setPpsExtension5bits( uiCode );
    if ( pcPPS->getPpsRangeExtensionsFlag() )
    { 
      TComPPSRExt &ppsRangeExtension = pcPPS->getPpsRangeExtension();      

      if (pcPPS->getUseTransformSkip())
      {
        READ_UVLC( uiCode, "log2_max_transform_skip_block_size_minus2");
        ppsRangeExtension.setLog2MaxTransformSkipBlockSize(uiCode+2);
      }

      READ_FLAG( uiCode, "cross_component_prediction_enabled_flag");
      ppsRangeExtension.setCrossComponentPredictionEnabledFlag(uiCode != 0);

      READ_FLAG( uiCode, "chroma_qp_offset_list_enabled_flag");
      if (uiCode == 0)
      {
        ppsRangeExtension.clearChromaQpOffsetList();
        ppsRangeExtension.setDiffCuChromaQpOffsetDepth(0);
      }
      else
      {
        READ_UVLC(uiCode, "diff_cu_chroma_qp_offset_depth"); ppsRangeExtension.setDiffCuChromaQpOffsetDepth(uiCode);
        UInt tableSizeMinus1 = 0;
        READ_UVLC(tableSizeMinus1, "chroma_qp_offset_list_len_minus1");
        assert(tableSizeMinus1 < MAX_QP_OFFSET_LIST_SIZE);

        for (Int cuChromaQpOffsetIdx = 0; cuChromaQpOffsetIdx <= (tableSizeMinus1); cuChromaQpOffsetIdx++)
        {
          Int cbOffset;
          Int crOffset;
          READ_SVLC(cbOffset, "cb_qp_offset_list[i]");
          assert(cbOffset >= -12 && cbOffset <= 12);
          READ_SVLC(crOffset, "cr_qp_offset_list[i]");
          assert(crOffset >= -12 && crOffset <= 12);
          // table uses +1 for index (see comment inside the function)
          ppsRangeExtension.setChromaQpOffsetListEntry(cuChromaQpOffsetIdx+1, cbOffset, crOffset);
        }
        assert(ppsRangeExtension.getChromaQpOffsetListLen() == tableSizeMinus1 + 1);
      }

      READ_UVLC( uiCode, "log2_sao_offset_scale_luma");
      ppsRangeExtension.setLog2SaoOffsetScale(CHANNEL_TYPE_LUMA, uiCode);
      READ_UVLC( uiCode, "log2_sao_offset_scale_chroma");
      ppsRangeExtension.setLog2SaoOffsetScale(CHANNEL_TYPE_CHROMA, uiCode);    }

    if ( pcPPS->getPpsMultilayerExtensionFlag() )
    { 
      parsePpsMultilayerExtension( pcPPS ); 
    }

    if ( pcPPS->getPps3dExtensionFlag() )
    { 
#if NH_3D
      parsePps3dExtension( pcPPS );
#endif
    }
#if NH_3D
    if ( pcPPS->getPpsExtension5bits() )
#else
    if ( pcPPS->getPpsExtension5bits() || pcPPS->getPps3dExtensionFlag() )
#endif
    {
      while ( xMoreRbspData() )
      {
        READ_FLAG( uiCode, "pps_extension_data_flag");
      }
    }

#else
#if ENC_DEC_TRACE || RExt__DECODER_DEBUG_BIT_STATISTICS
    static const char *syntaxStrings[]={ "pps_range_extension_flag",
                                         "pps_multilayer_extension_flag",
                                         "pps_extension_6bits[0]",
                                         "pps_extension_6bits[1]",
                                         "pps_extension_6bits[2]",
                                         "pps_extension_6bits[3]",
                                         "pps_extension_6bits[4]",
                                         "pps_extension_6bits[5]" };
#endif

    Bool pps_extension_flags[NUM_PPS_EXTENSION_FLAGS];
    for(Int i=0; i<NUM_PPS_EXTENSION_FLAGS; i++)
    {
      READ_FLAG( uiCode, syntaxStrings[i] );
      pps_extension_flags[i] = uiCode!=0;
    }

    Bool bSkipTrailingExtensionBits=false;
    for(Int i=0; i<NUM_PPS_EXTENSION_FLAGS; i++) // loop used so that the order is determined by the enum.
    {
      if (pps_extension_flags[i])
      {
        switch (PPSExtensionFlagIndex(i))
        {
          case PPS_EXT__REXT:
            {
              TComPPSRExt &ppsRangeExtension = pcPPS->getPpsRangeExtension();
            assert(!bSkipTrailingExtensionBits);

            if (pcPPS->getUseTransformSkip())
            {
              READ_UVLC( uiCode, "log2_max_transform_skip_block_size_minus2");
                ppsRangeExtension.setLog2MaxTransformSkipBlockSize(uiCode+2);
            }

            READ_FLAG( uiCode, "cross_component_prediction_enabled_flag");
              ppsRangeExtension.setCrossComponentPredictionEnabledFlag(uiCode != 0);

            READ_FLAG( uiCode, "chroma_qp_offset_list_enabled_flag");
            if (uiCode == 0)
            {
                ppsRangeExtension.clearChromaQpOffsetList();
                ppsRangeExtension.setDiffCuChromaQpOffsetDepth(0);
            }
            else
            {
                READ_UVLC(uiCode, "diff_cu_chroma_qp_offset_depth"); ppsRangeExtension.setDiffCuChromaQpOffsetDepth(uiCode);
              UInt tableSizeMinus1 = 0;
              READ_UVLC(tableSizeMinus1, "chroma_qp_offset_list_len_minus1");
              assert(tableSizeMinus1 < MAX_QP_OFFSET_LIST_SIZE);

              for (Int cuChromaQpOffsetIdx = 0; cuChromaQpOffsetIdx <= (tableSizeMinus1); cuChromaQpOffsetIdx++)
              {
                Int cbOffset;
                Int crOffset;
                READ_SVLC(cbOffset, "cb_qp_offset_list[i]");
                assert(cbOffset >= -12 && cbOffset <= 12); 
                READ_SVLC(crOffset, "cr_qp_offset_list[i]");
                assert(crOffset >= -12 && crOffset <= 12);
                // table uses +1 for index (see comment inside the function)
                  ppsRangeExtension.setChromaQpOffsetListEntry(cuChromaQpOffsetIdx+1, cbOffset, crOffset);
              }
                assert(ppsRangeExtension.getChromaQpOffsetListLen() == tableSizeMinus1 + 1);
            }

            READ_UVLC( uiCode, "log2_sao_offset_scale_luma");
              ppsRangeExtension.setLog2SaoOffsetScale(CHANNEL_TYPE_LUMA, uiCode);
            READ_UVLC( uiCode, "log2_sao_offset_scale_chroma");
              ppsRangeExtension.setLog2SaoOffsetScale(CHANNEL_TYPE_CHROMA, uiCode);
            }
            break;
          default:
            bSkipTrailingExtensionBits=true;
            break;
        }
      }
    }
    if (bSkipTrailingExtensionBits)
    {
      while ( xMoreRbspData() )
      {
        READ_FLAG( uiCode, "pps_extension_data_flag");
      }
    }
#endif
  }
  xReadRbspTrailingBits();
}

#if NH_3D
Void TDecCavlc::parsePps3dExtension( TComPPS* pcPPS )
{
#if NH_3D_DLT
  UInt uiCode = 0; 
  //
  TComDLT* pcDLT = pcPPS->getDLT();

  READ_FLAG(uiCode, "dlts_present_flag");
  pcDLT->setDltPresentFlag( (uiCode == 1) ? true : false );

  if ( pcDLT->getDltPresentFlag() )
  {
    READ_CODE(6, uiCode, "pps_depth_layers_minus1");
#if NH_3D_VER141_DEC_COMP_FLAG
    pcDLT->setNumDepthViews( uiCode );
#else
    pcDLT->setNumDepthViews( uiCode+1 );
#endif
    
    READ_CODE(4, uiCode, "pps_bit_depth_for_depth_layers_minus8");
    pcDLT->setDepthViewBitDepth( (uiCode+8) );
    
#if NH_3D_DLT_FIX
    for( Int i = 0; i <= pcDLT->getNumDepthViews()-1; i++ )
#else
    for( Int i = 0; i <= pcDLT->getNumDepthViews(); i++ )
#endif
    {
      Int layerId = pcDLT->getDepthIdxToLayerId(i);
      
      READ_FLAG(uiCode, "dlt_flag[i]");
      pcDLT->setUseDLTFlag(layerId, (uiCode == 1) ? true : false);
      
      if ( pcDLT->getUseDLTFlag( layerId ) )
      {
        Bool bDltBitMapRepFlag    = false;
        UInt uiMaxDiff            = MAX_INT;
        UInt uiMinDiff            = 0;
        UInt uiCodeLength         = 0;
        
        READ_FLAG(uiCode, "dlt_pred_flag[i]");
        
        if( uiCode )
        {
          assert( pcDLT->getUseDLTFlag( 1 ));
        }
        pcDLT->setInterViewDltPredEnableFlag( layerId, (uiCode == 1) ? true : false );
        
        if ( pcDLT->getInterViewDltPredEnableFlag( layerId ) == false )
        {
          READ_FLAG(uiCode, "dlt_val_flags_present_flag[i]");
          bDltBitMapRepFlag = (uiCode == 1) ? true : false;
        }
        else
        {
          bDltBitMapRepFlag = false;
        }
        
        UInt uiNumDepthValues = 0;
        std::vector<Int> aiIdx2DepthValue(256, 0);
        
        // Bit map
        if ( bDltBitMapRepFlag )
        {
          for (UInt d=0; d<256; d++)
          {
            READ_FLAG(uiCode, "dlt_value_flag[i][j]");
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
          READ_CODE(8, uiNumDepthValues, "num_val_delta_dlt");   // num_entry
          
          {
            // The condition if( pcVPS->getNumDepthValues(i) > 0 ) is always true since for Single-view Diff Coding, there is at least one depth value in depth component.
            
            if (uiNumDepthValues > 1)
            {
              READ_CODE(8, uiCode, "max_diff");
              uiMaxDiff = uiCode;
            }
            else
            {
              uiMaxDiff = 0;           // when there is only one value in DLT
            }
            
            if (uiNumDepthValues > 2)
            {
              uiCodeLength = (UInt) gCeilLog2(uiMaxDiff + 1);
              READ_CODE(uiCodeLength, uiCode, "min_diff_minus1");
              uiMinDiff = uiCode + 1;
            }
            else
            {
              uiMinDiff = uiMaxDiff;   // when there are only one or two values in DLT
            }
            
            READ_CODE(8, uiCode, "delta_dlt_val0");   // entry0
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
              uiCodeLength = (UInt) gCeilLog2(uiMaxDiff - uiMinDiff + 1);
              for (UInt d=1; d<uiNumDepthValues; d++)
              {
                READ_CODE(uiCodeLength, uiCode, "delta_val_diff_minus_min[k]");
                aiIdx2DepthValue[d] = aiIdx2DepthValue[d-1] + uiMinDiff + uiCode;
              }
            }
            
          }
        }
        
        if( pcDLT->getInterViewDltPredEnableFlag( layerId ) )
        {
          // interpret decoded values as delta DLT
          AOF( layerId > 1 );
          // assumes ref layer id to be 1
          std::vector<Int> viRefDLT = pcDLT->idx2DepthValue( 1 );
          UInt uiRefNum = pcDLT->getNumDepthValues( 1 );
          pcDLT->setDeltaDLT(layerId, viRefDLT, uiRefNum, aiIdx2DepthValue, uiNumDepthValues);
        }
        else
        {
          // store final DLT
          pcDLT->setDepthLUTs(layerId, aiIdx2DepthValue, uiNumDepthValues);
        }
      }
    }
  }
#endif
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
      READ_CODE(16, uiCode, "sar_height");                              pcVUI->setSarHeight(uiCode);
    }
  }

  READ_FLAG(     uiCode, "overscan_info_present_flag");               pcVUI->setOverscanInfoPresentFlag(uiCode);
  if (pcVUI->getOverscanInfoPresentFlag())
  {
    READ_FLAG(   uiCode, "overscan_appropriate_flag");                pcVUI->setOverscanAppropriateFlag(uiCode);
  }

  READ_FLAG(     uiCode, "video_signal_type_present_flag");           pcVUI->setVideoSignalTypePresentFlag(uiCode);
#if NH_MV
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
      READ_CODE(8, uiCode, "matrix_coeffs");                          pcVUI->setMatrixCoefficients(uiCode);
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
#if NH_MV
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

    READ_FLAG(     uiCode, "vui_hrd_parameters_present_flag");        pcVUI->setHrdParametersPresentFlag(uiCode);
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
    READ_UVLC(   uiCode, "min_spatial_segmentation_idc");             pcVUI->setMinSpatialSegmentationIdc(uiCode);
    assert(uiCode < 4096);
    READ_UVLC(   uiCode, "max_bytes_per_pic_denom" );                 pcVUI->setMaxBytesPerPicDenom(uiCode);
    READ_UVLC(   uiCode, "max_bits_per_min_cu_denom" );               pcVUI->setMaxBitsPerMinCuDenom(uiCode);
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
      READ_FLAG( uiCode, "sub_pic_hrd_params_present_flag" );         hrd->setSubPicCpbParamsPresentFlag( uiCode == 1 ? true : false );
      if( hrd->getSubPicCpbParamsPresentFlag() )
      {
        READ_CODE( 8, uiCode, "tick_divisor_minus2" );                hrd->setTickDivisorMinus2( uiCode );
        READ_CODE( 5, uiCode, "du_cpb_removal_delay_increment_length_minus1" ); hrd->setDuCpbRemovalDelayLengthMinus1( uiCode );
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
#if H_MV_ENC_DEC_TRAC
  tracePSHeader( "SPS", pcSPS->getLayerId() ); 
#else
  xTraceSPSHeader ();
#endif
#endif

  UInt  uiCode;
  READ_CODE( 4,  uiCode, "sps_video_parameter_set_id");          pcSPS->setVPSId        ( uiCode );

#if NH_MV
  if ( pcSPS->getLayerId() == 0 )
  {
#endif
  READ_CODE( 3,  uiCode, "sps_max_sub_layers_minus1" );          pcSPS->setMaxTLayers   ( uiCode+1 );
  assert(uiCode <= 6);
#if NH_MV
  }
  else
  {
    READ_CODE( 3, uiCode, "sps_ext_or_max_sub_layers_minus1" ); pcSPS->setSpsExtOrMaxSubLayersMinus1( uiCode );    
    pcSPS->inferSpsMaxSubLayersMinus1( false, NULL );
  }
  if ( !pcSPS->getMultiLayerExtSpsFlag() )
  {
#endif

  READ_FLAG( uiCode, "sps_temporal_id_nesting_flag" );           pcSPS->setTemporalIdNestingFlag ( uiCode > 0 ? true : false );
  if ( pcSPS->getMaxTLayers() == 1 )
  {
    // sps_temporal_id_nesting_flag must be 1 when sps_max_sub_layers_minus1 is 0
    assert( uiCode == 1 );
  }

  parsePTL(pcSPS->getPTL(), 1, pcSPS->getMaxTLayers() - 1);
#if NH_MV
    pcSPS->getPTL()->inferGeneralValues ( true, 0, NULL ); 
    pcSPS->getPTL()->inferSubLayerValues( pcSPS->getMaxTLayers() - 1, 0, NULL );
  }
#endif
  READ_UVLC(     uiCode, "sps_seq_parameter_set_id" );           pcSPS->setSPSId( uiCode );
  assert(uiCode <= 15);
#if NH_MV
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

  READ_UVLC(     uiCode, "chroma_format_idc" );                  pcSPS->setChromaFormatIdc( ChromaFormat(uiCode) );
  assert(uiCode <= 3);
  if( pcSPS->getChromaFormatIdc() == CHROMA_444 )
  {
    READ_FLAG(     uiCode, "separate_colour_plane_flag");        assert(uiCode == 0);
  }

  READ_UVLC (    uiCode, "pic_width_in_luma_samples" );          pcSPS->setPicWidthInLumaSamples ( uiCode    );
  READ_UVLC (    uiCode, "pic_height_in_luma_samples" );         pcSPS->setPicHeightInLumaSamples( uiCode    );
  READ_FLAG(     uiCode, "conformance_window_flag");
  if (uiCode != 0)
  {
    Window &conf = pcSPS->getConformanceWindow();
#if NH_MV
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

#if NH_MV
  if ( !pcSPS->getMultiLayerExtSpsFlag() )
  { 
#endif

  READ_UVLC(     uiCode, "bit_depth_luma_minus8" );
#if O0043_BEST_EFFORT_DECODING
  pcSPS->setStreamBitDepth(CHANNEL_TYPE_LUMA, 8 + uiCode);
  const UInt forceDecodeBitDepth = pcSPS->getForceDecodeBitDepth();
  if (forceDecodeBitDepth != 0)
  {
    uiCode = forceDecodeBitDepth - 8;
  }
#endif
  assert(uiCode <= 8);
  pcSPS->setBitDepth(CHANNEL_TYPE_LUMA, 8 + uiCode);

#if O0043_BEST_EFFORT_DECODING
  pcSPS->setQpBDOffset(CHANNEL_TYPE_LUMA, (Int) (6*(pcSPS->getStreamBitDepth(CHANNEL_TYPE_LUMA)-8)) );
#else
  pcSPS->setQpBDOffset(CHANNEL_TYPE_LUMA, (Int) (6*uiCode) );
#endif

  READ_UVLC( uiCode,    "bit_depth_chroma_minus8" );
#if O0043_BEST_EFFORT_DECODING
  pcSPS->setStreamBitDepth(CHANNEL_TYPE_CHROMA, 8 + uiCode);
  if (forceDecodeBitDepth != 0)
  {
    uiCode = forceDecodeBitDepth - 8;
  }
#endif
  assert(uiCode <= 8);
  pcSPS->setBitDepth(CHANNEL_TYPE_CHROMA, 8 + uiCode);
#if O0043_BEST_EFFORT_DECODING
  pcSPS->setQpBDOffset(CHANNEL_TYPE_CHROMA,  (Int) (6*(pcSPS->getStreamBitDepth(CHANNEL_TYPE_CHROMA)-8)) );
#else
  pcSPS->setQpBDOffset(CHANNEL_TYPE_CHROMA,  (Int) (6*uiCode) );
#endif
#if NH_MV
  }
#endif

  READ_UVLC( uiCode,    "log2_max_pic_order_cnt_lsb_minus4" );   pcSPS->setBitsForPOC( 4 + uiCode );
  assert(uiCode <= 12);

#if NH_MV
  if ( !pcSPS->getMultiLayerExtSpsFlag()) 
  {  
#endif
  UInt subLayerOrderingInfoPresentFlag;
  READ_FLAG(subLayerOrderingInfoPresentFlag, "sps_sub_layer_ordering_info_present_flag");

  for(UInt i=0; i <= pcSPS->getMaxTLayers()-1; i++)
  {
    READ_UVLC ( uiCode, "sps_max_dec_pic_buffering_minus1[i]");
    pcSPS->setMaxDecPicBuffering( uiCode + 1, i);
    READ_UVLC ( uiCode, "sps_max_num_reorder_pics[i]" );
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
#if NH_MV
  }
#endif

  READ_UVLC( uiCode, "log2_min_luma_coding_block_size_minus3" );
  Int log2MinCUSize = uiCode + 3;
  pcSPS->setLog2MinCodingBlockSize(log2MinCUSize);
  READ_UVLC( uiCode, "log2_diff_max_min_luma_coding_block_size" );
  pcSPS->setLog2DiffMaxMinCodingBlockSize(uiCode);
  
  if (pcSPS->getPTL()->getGeneralPTL()->getLevelIdc() >= Level::LEVEL5)
  {
    assert(log2MinCUSize + pcSPS->getLog2DiffMaxMinCodingBlockSize() >= 5);
  }
  
  Int maxCUDepthDelta = uiCode;
  pcSPS->setMaxCUWidth  ( 1<<(log2MinCUSize + maxCUDepthDelta) );
  pcSPS->setMaxCUHeight ( 1<<(log2MinCUSize + maxCUDepthDelta) );
  READ_UVLC( uiCode, "log2_min_luma_transform_block_size_minus2" );   pcSPS->setQuadtreeTULog2MinSize( uiCode + 2 );

  READ_UVLC( uiCode, "log2_diff_max_min_luma_transform_block_size" ); pcSPS->setQuadtreeTULog2MaxSize( uiCode + pcSPS->getQuadtreeTULog2MinSize() );
  pcSPS->setMaxTrSize( 1<<(uiCode + pcSPS->getQuadtreeTULog2MinSize()) );

  READ_UVLC( uiCode, "max_transform_hierarchy_depth_inter" );    pcSPS->setQuadtreeTUMaxDepthInter( uiCode+1 );
  READ_UVLC( uiCode, "max_transform_hierarchy_depth_intra" );    pcSPS->setQuadtreeTUMaxDepthIntra( uiCode+1 );

  Int addCuDepth = max (0, log2MinCUSize - (Int)pcSPS->getQuadtreeTULog2MinSize() );
  pcSPS->setMaxTotalCUDepth( maxCUDepthDelta + addCuDepth  + getMaxCUDepthOffset(pcSPS->getChromaFormatIdc(), pcSPS->getQuadtreeTULog2MinSize()) );

  READ_FLAG( uiCode, "scaling_list_enabled_flag" );                 pcSPS->setScalingListFlag ( uiCode );
  if(pcSPS->getScalingListFlag())
  {
#if NH_MV
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
      parseScalingList( &(pcSPS->getScalingList()) );
    }
#if NH_MV
    }
#endif
  }
  READ_FLAG( uiCode, "amp_enabled_flag" );                          pcSPS->setUseAMP( uiCode );
  READ_FLAG( uiCode, "sample_adaptive_offset_enabled_flag" );       pcSPS->setUseSAO ( uiCode ? true : false );

  READ_FLAG( uiCode, "pcm_enabled_flag" ); pcSPS->setUsePCM( uiCode ? true : false );
  if( pcSPS->getUsePCM() )
  {
    READ_CODE( 4, uiCode, "pcm_sample_bit_depth_luma_minus1" );          pcSPS->setPCMBitDepth    ( CHANNEL_TYPE_LUMA, 1 + uiCode );
    READ_CODE( 4, uiCode, "pcm_sample_bit_depth_chroma_minus1" );        pcSPS->setPCMBitDepth    ( CHANNEL_TYPE_CHROMA, 1 + uiCode );
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
    READ_UVLC( uiCode, "num_long_term_ref_pics_sps" );
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

  READ_FLAG( uiCode, "sps_extension_present_flag");
#if NH_MV
  pcSPS->setSpsExtensionPresentFlag( uiCode ); 
  if (pcSPS->getSpsExtensionPresentFlag( ) )
  {  
    READ_FLAG( uiCode, "sps_range_extensions_flag" );     pcSPS->setSpsRangeExtensionsFlag( uiCode == 1 );
    READ_FLAG( uiCode, "sps_multilayer_extension_flag" ); pcSPS->setSpsMultilayerExtensionFlag( uiCode == 1 );
    READ_FLAG( uiCode   , "sps_3d_extension_flag" );      pcSPS->setSps3dExtensionFlag( uiCode == 1 );
    READ_CODE( 5, uiCode, "sps_extension_5bits" )  ;      pcSPS->setSpsExtension5bits( uiCode );

    if ( pcSPS->getSpsRangeExtensionsFlag() )
    {
              TComSPSRExt &spsRangeExtension = pcSPS->getSpsRangeExtension();
              READ_FLAG( uiCode, "transform_skip_rotation_enabled_flag");     spsRangeExtension.setTransformSkipRotationEnabledFlag(uiCode != 0);
              READ_FLAG( uiCode, "transform_skip_context_enabled_flag");      spsRangeExtension.setTransformSkipContextEnabledFlag (uiCode != 0);
              READ_FLAG( uiCode, "implicit_rdpcm_enabled_flag");              spsRangeExtension.setRdpcmEnabledFlag(RDPCM_SIGNAL_IMPLICIT, (uiCode != 0));
              READ_FLAG( uiCode, "explicit_rdpcm_enabled_flag");              spsRangeExtension.setRdpcmEnabledFlag(RDPCM_SIGNAL_EXPLICIT, (uiCode != 0));
              READ_FLAG( uiCode, "extended_precision_processing_flag");       spsRangeExtension.setExtendedPrecisionProcessingFlag (uiCode != 0);
              READ_FLAG( uiCode, "intra_smoothing_disabled_flag");            spsRangeExtension.setIntraSmoothingDisabledFlag      (uiCode != 0);
              READ_FLAG( uiCode, "high_precision_offsets_enabled_flag");      spsRangeExtension.setHighPrecisionOffsetsEnabledFlag (uiCode != 0);
              READ_FLAG( uiCode, "persistent_rice_adaptation_enabled_flag");  spsRangeExtension.setPersistentRiceAdaptationEnabledFlag (uiCode != 0);
              READ_FLAG( uiCode, "cabac_bypass_alignment_enabled_flag");      spsRangeExtension.setCabacBypassAlignmentEnabledFlag  (uiCode != 0);
    }

    if ( pcSPS->getSpsMultilayerExtensionFlag() )
    {
      parseSpsMultilayerExtension( pcSPS ); 
    }

    if ( pcSPS->getSps3dExtensionFlag() )
    {
#if NH_3D
      parseSps3dExtension( pcSPS ); 
#endif
    }

#if NH_3D
    if ( pcSPS->getSpsExtension5bits() )
#else
    if ( pcSPS->getSpsExtension5bits() || pcSPS->getSps3dExtensionFlag() )
#endif
    { 
      while ( xMoreRbspData() )
      {
        READ_FLAG( uiCode, "sps_extension_data_flag");
      }
    }
  }
#else
  if (uiCode)
  {

#if ENC_DEC_TRACE || RExt__DECODER_DEBUG_BIT_STATISTICS
    static const char *syntaxStrings[]={ "sps_range_extension_flag",
                                         "sps_multilayer_extension_flag",
                                         "sps_extension_6bits[0]",
                                         "sps_extension_6bits[1]",
                                         "sps_extension_6bits[2]",
                                         "sps_extension_6bits[3]",
                                         "sps_extension_6bits[4]",
                                         "sps_extension_6bits[5]" };
#endif
    Bool sps_extension_flags[NUM_SPS_EXTENSION_FLAGS];

    for(Int i=0; i<NUM_SPS_EXTENSION_FLAGS; i++)
    {
      READ_FLAG( uiCode, syntaxStrings[i] );
      sps_extension_flags[i] = uiCode!=0;
    }

    Bool bSkipTrailingExtensionBits=false;
    for(Int i=0; i<NUM_SPS_EXTENSION_FLAGS; i++) // loop used so that the order is determined by the enum.
    {
      if (sps_extension_flags[i])
      {
        switch (SPSExtensionFlagIndex(i))
        {
          case SPS_EXT__REXT:
            assert(!bSkipTrailingExtensionBits);
            {
              TComSPSRExt &spsRangeExtension = pcSPS->getSpsRangeExtension();
              READ_FLAG( uiCode, "transform_skip_rotation_enabled_flag");     spsRangeExtension.setTransformSkipRotationEnabledFlag(uiCode != 0);
              READ_FLAG( uiCode, "transform_skip_context_enabled_flag");      spsRangeExtension.setTransformSkipContextEnabledFlag (uiCode != 0);
              READ_FLAG( uiCode, "implicit_rdpcm_enabled_flag");              spsRangeExtension.setRdpcmEnabledFlag(RDPCM_SIGNAL_IMPLICIT, (uiCode != 0));
              READ_FLAG( uiCode, "explicit_rdpcm_enabled_flag");              spsRangeExtension.setRdpcmEnabledFlag(RDPCM_SIGNAL_EXPLICIT, (uiCode != 0));
              READ_FLAG( uiCode, "extended_precision_processing_flag");       spsRangeExtension.setExtendedPrecisionProcessingFlag (uiCode != 0);
              READ_FLAG( uiCode, "intra_smoothing_disabled_flag");            spsRangeExtension.setIntraSmoothingDisabledFlag      (uiCode != 0);
              READ_FLAG( uiCode, "high_precision_offsets_enabled_flag");      spsRangeExtension.setHighPrecisionOffsetsEnabledFlag (uiCode != 0);
              READ_FLAG( uiCode, "persistent_rice_adaptation_enabled_flag");  spsRangeExtension.setPersistentRiceAdaptationEnabledFlag (uiCode != 0);
              READ_FLAG( uiCode, "cabac_bypass_alignment_enabled_flag");      spsRangeExtension.setCabacBypassAlignmentEnabledFlag  (uiCode != 0);
            }
            break;
          default:
            bSkipTrailingExtensionBits=true;
            break;
        }
      }
    }
    if (bSkipTrailingExtensionBits)
    {
      while ( xMoreRbspData() )
      {
        READ_FLAG( uiCode, "sps_extension_data_flag");
      }
    }
  }
#endif
  xReadRbspTrailingBits();
}

#if NH_MV
Void TDecCavlc::parseSpsMultilayerExtension( TComSPS* pcSPS )
{
  UInt uiCode; 
  READ_FLAG( uiCode, "inter_view_mv_vert_constraint_flag" );    pcSPS->setInterViewMvVertConstraintFlag(uiCode == 1 ? true : false);  
}

#if NH_3D
Void TDecCavlc::parseSps3dExtension( TComSPS* pcSPS )
{ 
  TComSps3dExtension sps3dExt; 
  UInt uiCode; 
  for( Int d = 0; d  <=  1; d++ )
  {
    READ_FLAG( uiCode, "iv_mv_pred_flag" ); sps3dExt.setIvMvPredFlag( d, uiCode == 1 );
    READ_FLAG( uiCode, "iv_mv_scaling_flag" ); sps3dExt.setIvMvScalingFlag( d, uiCode == 1 );
    if( d  ==  0 )
    {
      READ_UVLC( uiCode, "log2_sub_pb_size_minus3" ); sps3dExt.setLog2SubPbSizeMinus3( d, uiCode );
      READ_FLAG( uiCode, "iv_res_pred_flag" ); sps3dExt.setIvResPredFlag( d, uiCode == 1 );
      READ_FLAG( uiCode, "depth_refinement_flag" ); sps3dExt.setDepthRefinementFlag( d, uiCode == 1 );
      READ_FLAG( uiCode, "view_synthesis_pred_flag" ); sps3dExt.setViewSynthesisPredFlag( d, uiCode == 1 );
      READ_FLAG( uiCode, "depth_based_blk_part_flag" ); sps3dExt.setDepthBasedBlkPartFlag( d, uiCode == 1 );
    }
    else 
    {
      READ_FLAG( uiCode, "mpi_flag" ); sps3dExt.setMpiFlag( d, uiCode == 1 );
      READ_UVLC( uiCode, "log2_mpi_sub_pb_size_minus3" ); sps3dExt.setLog2MpiSubPbSizeMinus3( d, uiCode );
      READ_FLAG( uiCode, "intra_contour_flag" ); sps3dExt.setIntraContourFlag( d, uiCode == 1 );
      READ_FLAG( uiCode, "intra_sdc_wedge_flag" ); sps3dExt.setIntraSdcWedgeFlag( d, uiCode == 1 );
      READ_FLAG( uiCode, "qt_pred_flag" ); sps3dExt.setQtPredFlag( d, uiCode == 1 );
      READ_FLAG( uiCode, "inter_sdc_flag" ); sps3dExt.setInterSdcFlag( d, uiCode == 1 );
      READ_FLAG( uiCode, "intra_skip_flag" ); sps3dExt.setDepthIntraSkipFlag( d, uiCode == 1 );
    }
  }
  pcSPS->setSps3dExtension( sps3dExt ); 
}
#endif

Void TDecCavlc::parsePpsMultilayerExtension(TComPPS* pcPPS)
{
  UInt uiCode = 0; 
  READ_FLAG( uiCode, "poc_reset_info_present_flag" ); pcPPS->setPocResetInfoPresentFlag( uiCode == 1 );
  READ_FLAG( uiCode, "pps_infer_scaling_list_flag" ); pcPPS->setPpsInferScalingListFlag( uiCode == 1 );
  if (pcPPS->getPpsInferScalingListFlag())
  {
  READ_CODE( 6, uiCode, "pps_scaling_list_ref_layer_id" ); pcPPS->setPpsScalingListRefLayerId( uiCode );
  }

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
#if ENC_DEC_TRACE
#if H_MV_ENC_DEC_TRAC
  tracePSHeader( "VPS", getDecTop()->getLayerId() ); 
#else
  xTraceVPSHeader ();
#endif
#endif

  UInt  uiCode;

  READ_CODE( 4,  uiCode,  "vps_video_parameter_set_id" );         pcVPS->setVPSId( uiCode );
#if NH_MV
  READ_FLAG( uiCode, "vps_base_layer_internal_flag" );            pcVPS->setVpsBaseLayerInternalFlag( uiCode == 1 );
  READ_FLAG( uiCode, "vps_base_layer_available_flag" );           pcVPS->setVpsBaseLayerAvailableFlag( uiCode == 1 );
#else
  READ_FLAG( uiCode,      "vps_base_layer_internal_flag" );       assert(uiCode == 1);
  READ_FLAG( uiCode,      "vps_base_layer_available_flag" );      assert(uiCode == 1);
#endif
#if NH_MV
  READ_CODE( 6,  uiCode,  "vps_max_layers_minus1" );              pcVPS->setMaxLayersMinus1( std::min( uiCode, (UInt) ( MAX_NUM_LAYER_IDS-1) )  );
#else
  READ_CODE( 6,  uiCode,  "vps_max_layers_minus1" );
#endif
  READ_CODE( 3,  uiCode,  "vps_max_sub_layers_minus1" );          pcVPS->setMaxTLayers( uiCode + 1 );    assert(uiCode+1 <= MAX_TLAYER);
  READ_FLAG(     uiCode,  "vps_temporal_id_nesting_flag" );       pcVPS->setTemporalNestingFlag( uiCode ? true:false );
  assert (pcVPS->getMaxTLayers()>1||pcVPS->getTemporalNestingFlag());
  READ_CODE( 16, uiCode,  "vps_reserved_0xffff_16bits" );         assert(uiCode == 0xffff);
  parsePTL ( pcVPS->getPTL(), true, pcVPS->getMaxTLayers()-1);
#if NH_MV
  pcVPS->getPTL()->inferGeneralValues ( true, 0, NULL );
  pcVPS->getPTL()->inferSubLayerValues( pcVPS->getMaxTLayers() - 1, 0, NULL );
#endif
  UInt subLayerOrderingInfoPresentFlag;
  READ_FLAG(subLayerOrderingInfoPresentFlag, "vps_sub_layer_ordering_info_present_flag");
  for(UInt i = 0; i <= pcVPS->getMaxTLayers()-1; i++)
  {
    READ_UVLC( uiCode,  "vps_max_dec_pic_buffering_minus1[i]" );    pcVPS->setMaxDecPicBuffering( uiCode + 1, i );
    READ_UVLC( uiCode,  "vps_max_num_reorder_pics[i]" );            pcVPS->setNumReorderPics( uiCode, i );
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
#if NH_MV
  assert( pcVPS->getVpsMaxLayerId() < MAX_VPS_NUH_LAYER_ID_PLUS1 );
  READ_CODE( 6, uiCode, "vps_max_layer_id" );   pcVPS->setVpsMaxLayerId( uiCode );

  READ_UVLC(    uiCode, "vps_num_layer_sets_minus1" );  pcVPS->setVpsNumLayerSetsMinus1( uiCode );
  for( UInt opsIdx = 1; opsIdx <= pcVPS->getVpsNumLayerSetsMinus1(); opsIdx ++ )
  {
    for( UInt i = 0; i <= pcVPS->getVpsMaxLayerId(); i ++ )
#else
  assert( pcVPS->getMaxNuhReservedZeroLayerId() < MAX_VPS_NUH_RESERVED_ZERO_LAYER_ID_PLUS1 );
  READ_CODE( 6, uiCode, "vps_max_layer_id" );                        pcVPS->setMaxNuhReservedZeroLayerId( uiCode );
  READ_UVLC(    uiCode, "vps_num_layer_sets_minus1" );               pcVPS->setMaxOpSets( uiCode + 1 );
  for( UInt opsIdx = 1; opsIdx <= ( pcVPS->getMaxOpSets() - 1 ); opsIdx ++ )
  {
    // Operation point set
    for( UInt i = 0; i <= pcVPS->getMaxNuhReservedZeroLayerId(); i ++ )
#endif
    {
      READ_FLAG( uiCode, "layer_id_included_flag[opsIdx][i]" );   pcVPS->setLayerIdIncludedFlag( uiCode == 1 ? true : false, opsIdx, i );
    }
  }
#if NH_MV
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
      READ_UVLC( uiCode, "hrd_layer_set_idx[i]" );                  pcVPS->setHrdOpSetIdx( uiCode, i );
      if( i > 0 )
      {
        READ_FLAG( uiCode, "cprms_present_flag[i]" );               pcVPS->setCprmsPresentFlag( uiCode == 1 ? true : false, i );
      }
      else
      {
        pcVPS->setCprmsPresentFlag( true, i );
      }

      parseHrdParameters(pcVPS->getHrdParameters(i), pcVPS->getCprmsPresentFlag( i ), pcVPS->getMaxTLayers() - 1);
    }
  }
#if NH_MV
  READ_FLAG( uiCode,  "vps_extension_flag" );                      pcVPS->setVpsExtensionFlag( uiCode == 1 ? true : false );
  if ( pcVPS->getVpsExtensionFlag() )
#else
  READ_FLAG( uiCode,  "vps_extension_flag" );
  if (uiCode)
#endif
  {
#if NH_MV
    m_pcBitstream->readOutTrailingBits();
    parseVPSExtension( pcVPS );   
    READ_FLAG( uiCode,  "vps_extension2_flag" );
    if (uiCode)
    {
#if NH_3D
      READ_FLAG( uiCode,  "vps_3d_extension_flag" );
      if ( uiCode )
      {
        m_pcBitstream->readOutTrailingBits();
        pcVPS->createCamPars(pcVPS->getNumViews());
        parseVps3dExtension( pcVPS );   
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
#if NH_MV
#if NH_3D
      }
#endif
    }
#endif
  }

  xReadRbspTrailingBits();
}

#if NH_MV
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

#if NH_3D
  pcVPS->initViewCompLayer( ); 
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

  std::vector<TComRepFormat> repFormats;
  repFormats.resize( pcVPS->getVpsNumRepFormatsMinus1() + 1 ); 
  for (Int i = 0; i <= pcVPS->getVpsNumRepFormatsMinus1(); i++ )
  {     
    TComRepFormat* curRepFormat = &repFormats[i]; 
    TComRepFormat* prevRepFormat = i > 0 ? &repFormats[ i - 1] : NULL; 
    parseRepFormat( i, curRepFormat ,  prevRepFormat);     
  }
  pcVPS->setRepFormat( repFormats ); 

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
  
  TComVPSVUI vpsVui;
  if( pcVPS->getVpsVuiPresentFlag() )
  {
    m_pcBitstream->readOutTrailingBits(); // vps_vui_alignment_bit_equal_to_one
    parseVPSVUI( pcVPS, &vpsVui ); 
  }     
  else
  {    
    // inference of syntax elements that differ from default inference (as done in constructor), when VPS VUI is not present    
    vpsVui.setCrossLayerIrapAlignedFlag( false ); 
  }
  pcVPS->setVPSVUI( vpsVui ); 
  pcVPS->checkVPSExtensionSyntax(); 
}

Void TDecCavlc::parseRepFormat( Int i, TComRepFormat* pcRepFormat, const TComRepFormat* pcPrevRepFormat )
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
    pcRepFormat->inferChromaAndBitDepth(pcPrevRepFormat ); 
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


Void TDecCavlc::parseVPSVUI( const TComVPS* pcVPS, TComVPSVUI* vpsVui )
{
  assert( pcVPS ); 
  assert( vpsVui ); 
  
  vpsVui->init(pcVPS->getNumAddLayerSets(),pcVPS->getMaxSubLayersMinus1() + 1, pcVPS->getMaxLayersMinus1() + 1 );  

  UInt uiCode; 
  READ_FLAG( uiCode, "cross_layer_pic_type_aligned_flag" ); vpsVui->setCrossLayerPicTypeAlignedFlag( uiCode == 1 );
  if ( !vpsVui->getCrossLayerPicTypeAlignedFlag() )
  {  
    READ_FLAG( uiCode, "cross_layer_irap_aligned_flag" ); vpsVui->setCrossLayerIrapAlignedFlag( uiCode == 1 );
  }
  if( vpsVui->getCrossLayerIrapAlignedFlag( ) )
  {
    READ_FLAG( uiCode, "all_layers_idr_aligned_flag" ); vpsVui->setAllLayersIdrAlignedFlag( uiCode == 1 );
  }
  READ_FLAG( uiCode, "bit_rate_present_vps_flag" ); vpsVui->setBitRatePresentVpsFlag( uiCode == 1 );
  READ_FLAG( uiCode, "pic_rate_present_vps_flag" ); vpsVui->setPicRatePresentVpsFlag( uiCode == 1 );
  if( vpsVui->getBitRatePresentVpsFlag( )  ||  vpsVui->getPicRatePresentVpsFlag( ) )
  {
    for( Int i = pcVPS->getVpsBaseLayerInternalFlag() ? 0 : 1; i  <  pcVPS->getNumLayerSets(); i++ )
    {
      for( Int j = 0; j  <=  pcVPS->getMaxSubLayersInLayerSetMinus1( i ); j++ ) 
      {
        if( vpsVui->getBitRatePresentVpsFlag( ) )
        {
          READ_FLAG( uiCode, "bit_rate_present_flag" ); vpsVui->setBitRatePresentFlag( i, j, uiCode == 1 );            
        }
        if( vpsVui->getPicRatePresentVpsFlag( )  )
        {
          READ_FLAG( uiCode, "pic_rate_present_flag" ); vpsVui->setPicRatePresentFlag( i, j, uiCode == 1 );
        }
        if( vpsVui->getBitRatePresentFlag( i, j ) )
        {
          READ_CODE( 16, uiCode, "avg_bit_rate" ); vpsVui->setAvgBitRate( i, j, uiCode );
          READ_CODE( 16, uiCode, "max_bit_rate" ); vpsVui->setMaxBitRate( i, j, uiCode );
        }
        if( vpsVui->getPicRatePresentFlag( i, j ) )
        {
          READ_CODE( 2,  uiCode, "constant_pic_rate_idc" ); vpsVui->setConstantPicRateIdc( i, j, uiCode );
          READ_CODE( 16, uiCode, "avg_pic_rate" );          vpsVui->setAvgPicRate( i, j, uiCode );
        }
      }
    }
  }

  READ_FLAG( uiCode, "video_signal_info_idx_present_flag" ); vpsVui->setVideoSignalInfoIdxPresentFlag( uiCode == 1 );
  if( vpsVui->getVideoSignalInfoIdxPresentFlag() )
  {
    READ_CODE( 4, uiCode, "vps_num_video_signal_info_minus1" ); vpsVui->setVpsNumVideoSignalInfoMinus1( uiCode );
  }
  else
  {
    vpsVui->setVpsNumVideoSignalInfoMinus1( (pcVPS->getMaxLayersMinus1() - pcVPS->getVpsBaseLayerInternalFlag()) ? 0 : 1 );
  }

  std::vector<TComVideoSignalInfo> videoSignalInfos; 
  videoSignalInfos.resize(vpsVui->getVpsNumVideoSignalInfoMinus1() + 1 );

  for( Int i = 0; i <= vpsVui->getVpsNumVideoSignalInfoMinus1(); i++ )
  { 
    parseVideoSignalInfo( &videoSignalInfos[i] );     
  }
  vpsVui->setVideoSignalInfo( videoSignalInfos ); 

  if( vpsVui->getVideoSignalInfoIdxPresentFlag() && vpsVui->getVpsNumVideoSignalInfoMinus1() > 0 )
  {
    for( Int i = pcVPS->getVpsBaseLayerInternalFlag() ? 0 : 1; i <=  pcVPS->getMaxLayersMinus1(); i++ )
    {
      READ_CODE( 4, uiCode, "vps_video_signal_info_idx" ); vpsVui->setVpsVideoSignalInfoIdx( i, uiCode );
    }
  }
  else if ( !vpsVui->getVideoSignalInfoIdxPresentFlag() )
  {
    for( Int i = pcVPS->getVpsBaseLayerInternalFlag() ? 0 : 1; i <=  pcVPS->getMaxLayersMinus1(); i++ )
    {
      vpsVui->setVpsVideoSignalInfoIdx( i, i );
    }
  }
  else
  {
    for( Int i = pcVPS->getVpsBaseLayerInternalFlag() ? 0 : 1; i <=  pcVPS->getMaxLayersMinus1(); i++ )
    {
      vpsVui->setVpsVideoSignalInfoIdx( i, 0 );
    }
  }

  READ_FLAG( uiCode, "tiles_not_in_use_flag" ); vpsVui->setTilesNotInUseFlag( uiCode == 1 );
  if( !vpsVui->getTilesNotInUseFlag() ) 
  {      
    for( Int i = pcVPS->getVpsBaseLayerInternalFlag() ? 0 : 1; i  <=  pcVPS->getMaxLayersMinus1(); i++ )
    {
      READ_FLAG( uiCode, "tiles_in_use_flag[i]" ); vpsVui->setTilesInUseFlag( i, uiCode == 1 );
      if( vpsVui->getTilesInUseFlag( i ) )  
      {
        READ_FLAG( uiCode, "loop_filter_not_across_tiles_flag[i]" ); vpsVui->setLoopFilterNotAcrossTilesFlag( i, uiCode == 1 );
      }
    }  
    for( Int i = pcVPS->getVpsBaseLayerInternalFlag() ? 1 : 2; i  <=  pcVPS->getMaxLayersMinus1(); i++ )  
    {
      for( Int j = 0; j < pcVPS->getNumDirectRefLayers( pcVPS->getLayerIdInNuh( i ) ) ; j++ )
      {
        Int layerIdx = pcVPS->getLayerIdInVps(pcVPS->getIdDirectRefLayer(pcVPS->getLayerIdInNuh( i ) , j  ));  
        if( vpsVui->getTilesInUseFlag( i )  &&  vpsVui->getTilesInUseFlag( layerIdx ) )  
        {
          READ_FLAG( uiCode, "tile_boundaries_aligned_flag[i][j]" ); vpsVui->setTileBoundariesAlignedFlag( i, j, uiCode == 1 );
        }
      }  
    }
  }  
  
  READ_FLAG( uiCode, "wpp_not_in_use_flag" ); vpsVui->setWppNotInUseFlag( uiCode == 1 );
  
  if( !vpsVui->getWppNotInUseFlag( ))
  {
    for( Int i = 0; i  <=  pcVPS->getMaxLayersMinus1(); i++ )  
    {
      READ_FLAG( uiCode, "wpp_in_use_flag[i]" ); vpsVui->setWppInUseFlag( i, uiCode == 1 );
    }
  }
  READ_FLAG( uiCode, "single_layer_for_non_irap_flag" ); vpsVui->setSingleLayerForNonIrapFlag( uiCode == 1 );
  READ_FLAG( uiCode, "higher_layer_irap_skip_flag" ); vpsVui->setHigherLayerIrapSkipFlag( uiCode == 1 );
  READ_FLAG( uiCode, "ilp_restricted_ref_layers_flag" ); vpsVui->setIlpRestrictedRefLayersFlag( uiCode == 1 );

  if( vpsVui->getIlpRestrictedRefLayersFlag( ) )
  {
    for( Int i = 1; i  <=  pcVPS->getMaxLayersMinus1(); i++ )
    {
      for( Int j = 0; j < pcVPS->getNumDirectRefLayers( pcVPS->getLayerIdInNuh( i ) ); j++ )
      {
        if( pcVPS->getVpsBaseLayerInternalFlag() || pcVPS->getIdDirectRefLayer( pcVPS->getLayerIdInNuh( i ), j ) > 0 )
        {        
          READ_UVLC( uiCode, "min_spatial_segment_offset_plus1" ); vpsVui->setMinSpatialSegmentOffsetPlus1( i, j, uiCode );
          if( vpsVui->getMinSpatialSegmentOffsetPlus1( i, j ) > 0 )
          {
            READ_FLAG( uiCode, "ctu_based_offset_enabled_flag" ); vpsVui->setCtuBasedOffsetEnabledFlag( i, j, uiCode == 1 );
            if( vpsVui->getCtuBasedOffsetEnabledFlag( i, j ) )
            {
              READ_UVLC( uiCode, "min_horizontal_ctu_offset_plus1" ); vpsVui->setMinHorizontalCtuOffsetPlus1( i, j, uiCode );
            }
          }
        }
      }
    }
  }

  READ_FLAG( uiCode, "vps_vui_bsp_hrd_present_flag" ); vpsVui->setVpsVuiBspHrdPresentFlag( uiCode == 1 );
  if ( vpsVui->getVpsVuiBspHrdPresentFlag( ) )
  {
    assert(pcVPS->getTimingInfo()->getTimingInfoPresentFlag() == 1);    
    parseVpsVuiBspHrdParameters( pcVPS, vpsVui );     
  }
  for( Int i = 1; i  <=  pcVPS->getMaxLayersMinus1(); i++ )
  {
    if( pcVPS->getNumDirectRefLayers( pcVPS->getLayerIdInNuh( i )) == 0 ) 
    {
      READ_FLAG( uiCode, "base_layer_parameter_set_compatibility_flag" ); vpsVui->setBaseLayerParameterSetCompatibilityFlag( i, uiCode == 1 );
    }
  }  
}

Void TDecCavlc::parseVpsVuiBspHrdParameters( const TComVPS* pcVPS, TComVPSVUI* vpsVui )
{
  assert( pcVPS  ); 
  assert( vpsVui ); 

  TComVpsVuiBspHrdParameters  vpsVuiBspHrdP;    
  
  UInt uiCode; 
  READ_UVLC( uiCode, "vps_num_add_hrd_params" ); vpsVuiBspHrdP.setVpsNumAddHrdParams( uiCode );
  vpsVuiBspHrdP.createAfterVpsNumAddHrdParams( pcVPS ); 
  for( Int i = pcVPS->getNumHrdParameters(); i < pcVPS->getNumHrdParameters() + vpsVuiBspHrdP.getVpsNumAddHrdParams(); i++ )
  {  
    if( i > 0 )  
    {
      READ_FLAG( uiCode, "cprms_add_present_flag" ); vpsVuiBspHrdP.setCprmsAddPresentFlag( i, uiCode == 1 );
    }
    else
    {
       vpsVuiBspHrdP.setCprmsAddPresentFlag( i, true );
    }

    READ_UVLC( uiCode, "num_sub_layer_hrd_minus1" ); vpsVuiBspHrdP.setNumSubLayerHrdMinus1( i, uiCode );
    
    TComHRD hrdParameters;
    parseHrdParameters( &hrdParameters, vpsVuiBspHrdP.getCprmsAddPresentFlag( i ), vpsVuiBspHrdP.getNumSubLayerHrdMinus1( i ) );     
    vpsVuiBspHrdP.setHrdParametermeters( i, hrdParameters ); 
  }

  vpsVuiBspHrdP.setNumPartitionsInSchemeMinus1( 0, 0, 0);
  vpsVuiBspHrdP.createAfterNumPartitionsInSchemeMinus1( pcVPS, 0, 0 );

  for( Int h = 0; h < pcVPS->getNumOutputLayerSets(); h++ )
  { 
    if ( h == 0)
    {
      vpsVuiBspHrdP.setNumSignalledPartitioningSchemes( h, 0 );
    }
    else
    {
      READ_UVLC( uiCode, "num_signalled_partitioning_schemes" ); vpsVuiBspHrdP.setNumSignalledPartitioningSchemes( h, uiCode );
    }    
    vpsVuiBspHrdP.createAfterNumSignalledPartitioningSchemes( pcVPS, h );

    for( Int j = 0; j < vpsVuiBspHrdP.getNumSignalledPartitioningSchemes( h ) + 1; j++ )
    { 
      if ( j == 0 && h == 0 )
      {
        vpsVuiBspHrdP.setNumPartitionsInSchemeMinus1( h, j, uiCode );
      }
      else if( j == 0 )
      {
        vpsVuiBspHrdP.setNumPartitionsInSchemeMinus1( h, j, pcVPS->getNumLayersInIdList( h ) - 1 );
      }
      else
      {
        READ_UVLC( uiCode, "num_partitions_in_scheme_minus1" ); vpsVuiBspHrdP.setNumPartitionsInSchemeMinus1( h, j, uiCode );
      }
      vpsVuiBspHrdP.createAfterNumPartitionsInSchemeMinus1(pcVPS, h, j );

      for( Int k = 0; k  <=  vpsVuiBspHrdP.getNumPartitionsInSchemeMinus1( h, j ); k++ )  
      {
        for( Int r = 0; r < pcVPS->getNumLayersInIdList(pcVPS->olsIdxToLsIdx( h ) )   ; r++ )  
        {
          if( h == 0 && j == 0 && k == 0 && r == 0 )
          {
             vpsVuiBspHrdP.setLayerIncludedInPartitionFlag( h, j, k, r, true );
          }
          else if ( h > 0 && j == 0 )
          {
             vpsVuiBspHrdP.setLayerIncludedInPartitionFlag( h, j, k, r, (k == r) );
          }
          else
          {
            READ_FLAG( uiCode, "layer_included_in_partition_flag" ); vpsVuiBspHrdP.setLayerIncludedInPartitionFlag( h, j, k, r, uiCode == 1 );
          }          
        }
      }
    }  
    if ( h > 0 )
    {
      for( Int i = 0; i < vpsVuiBspHrdP.getNumSignalledPartitioningSchemes( h ) + 1; i++ )  
      {
        for( Int t = 0; t  <=  pcVPS->getMaxSubLayersInLayerSetMinus1( pcVPS->olsIdxToLsIdx( i ) ); t++ )
        {  
          READ_UVLC( uiCode, "num_bsp_schedules_minus1" ); vpsVuiBspHrdP.setNumBspSchedulesMinus1( h, i, t, uiCode );
          vpsVuiBspHrdP.createAfterNumBspSchedulesMinus1(pcVPS, h, i, t );
          for( Int j = 0; j  <=  vpsVuiBspHrdP.getNumBspSchedulesMinus1( h, i, t ); j++ )  
          {
            for( Int k = 0; k  <=  vpsVuiBspHrdP.getNumPartitionsInSchemeMinus1( h, j ); k++ )
            {  
              READ_CODE( vpsVuiBspHrdP.getBspHrdIdxLen( pcVPS ), uiCode, "bsp_hrd_idx" ); vpsVuiBspHrdP.setBspHrdIdx( h, i, t, j, k, uiCode );
              READ_UVLC( uiCode, "bsp_sched_idx" ); vpsVuiBspHrdP.setBspSchedIdx( h, i, t, j, k, uiCode );
            }  
          }
        }  
      }
    }
  }  
  vpsVui->setVpsVuiBspHrdParameters( vpsVuiBspHrdP ); 
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
  TComDpbSize dpbSize; 
  
  dpbSize.init( vps->getNumOutputLayerSets(), vps->getVpsMaxLayerId() + 1, vps->getMaxSubLayersMinus1() + 1 ) ;

  for( Int i = 1; i < vps->getNumOutputLayerSets(); i++ )
  {  
    Int currLsIdx = vps->olsIdxToLsIdx( i ); 
    READ_FLAG( uiCode, "sub_layer_flag_info_present_flag" ); dpbSize.setSubLayerFlagInfoPresentFlag( i, uiCode == 1 );
    for( Int j = 0; j  <=  vps->getMaxSubLayersInLayerSetMinus1( currLsIdx ); j++ )
    {  
      if( j > 0  &&  dpbSize.getSubLayerDpbInfoPresentFlag( i, j )  )  
      {
        READ_FLAG( uiCode, "sub_layer_dpb_info_present_flag" ); dpbSize.setSubLayerDpbInfoPresentFlag( i, j, uiCode == 1 );
      }
      if( dpbSize.getSubLayerDpbInfoPresentFlag( i, j ) )
      {  
        for( Int k = 0; k < vps->getNumLayersInIdList( currLsIdx ); k++ )   
        {
          if ( vps->getNecessaryLayerFlag( i, k ) && ( vps->getVpsBaseLayerInternalFlag() || ( vps->getLayerSetLayerIdList(vps->olsIdxToLsIdx(i),k) != 0 ) ))
          {
            READ_UVLC( uiCode, "max_vps_dec_pic_buffering_minus1" ); dpbSize.setMaxVpsDecPicBufferingMinus1( i, k, j, uiCode );
          }
          else
          {
            if ( vps->getNecessaryLayerFlag( i, k ) && ( j == 0 ) && ( k == 0 ))
            {
              dpbSize.setMaxVpsDecPicBufferingMinus1(i ,k, j, 0 );
            }
          }
        }
        READ_UVLC( uiCode, "max_vps_num_reorder_pics" ); dpbSize.setMaxVpsNumReorderPics( i, j, uiCode );
        READ_UVLC( uiCode, "max_vps_latency_increase_plus1" ); dpbSize.setMaxVpsLatencyIncreasePlus1( i, j, uiCode );
      }
      else
      {
        if ( j > 0 )
        {
          for( Int k = 0; k < vps->getNumLayersInIdList( vps->olsIdxToLsIdx( i ) ); k++ )   
          {
            if ( vps->getNecessaryLayerFlag(i, k ) )
            {            
              dpbSize.setMaxVpsDecPicBufferingMinus1( i, k, j, dpbSize.getMaxVpsDecPicBufferingMinus1( i,k, j - 1 ) );
            }
          }
          dpbSize.setMaxVpsNumReorderPics      ( i, j, dpbSize.getMaxVpsNumReorderPics      ( i, j - 1 ) );
          dpbSize.setMaxVpsLatencyIncreasePlus1( i, j, dpbSize.getMaxVpsLatencyIncreasePlus1( i, j - 1 ) );
        }
      }
    }  
  }  
  vps->setDpbSize( dpbSize ); 
}

#if NH_3D
Void TDecCavlc::parseVps3dExtension( TComVPS* pcVPS )
{
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
}
#endif
#endif

Void TDecCavlc::parseSliceHeader (TComSlice* pcSlice, ParameterSetManager *parameterSetManager, const Int prevTid0POC)
{
  UInt  uiCode;
  Int   iCode;

#if ENC_DEC_TRACE
#if NH_MV
  tracePSHeader( "Slice", pcSlice->getLayerId() ); 
#else
  xTraceSliceHeader();
#endif
#endif
  TComPPS* pps = NULL;
  TComSPS* sps = NULL;
#if NH_MV
  TComVPS* vps = NULL;
#endif

  UInt firstSliceSegmentInPic;
  READ_FLAG( firstSliceSegmentInPic, "first_slice_segment_in_pic_flag" );
  if( pcSlice->getRapPicFlag())
  {
    READ_FLAG( uiCode, "no_output_of_prior_pics_flag" );  //ignored -- updated already
    pcSlice->setNoOutputPriorPicsFlag(uiCode ? true : false);
  }
  READ_UVLC (    uiCode, "slice_pic_parameter_set_id" );  pcSlice->setPPSId(uiCode);
  pps = parameterSetManager->getPPS(uiCode);
  //!KS: need to add error handling code here, if PPS is not available
  assert(pps!=0);
  sps = parameterSetManager->getSPS(pps->getSPSId());
  //!KS: need to add error handling code here, if SPS is not available
  assert(sps!=0);
#if NH_MV  
  vps = parameterSetManager->getVPS(sps->getVPSId());
  assert( vps != NULL );  
  m_decTop->initFromActiveVps( vps );
  Int targetOlsIdx = m_decTop->getTargetOlsIdx(); 

  // Do inference 
  sps->inferRepFormat  ( vps , pcSlice->getLayerId(), false ); 
  sps->inferScalingList( parameterSetManager->getActiveSPS( sps->getSpsScalingListRefLayerId() ) );   
  sps->inferSpsMaxDecPicBufferingMinus1( vps, targetOlsIdx, pcSlice->getLayerId(), false ); 

  if( sps->getMultiLayerExtSpsFlag() )
  {
    sps->setTemporalIdNestingFlag( (sps->getMaxTLayers() > 1) ? vps->getTemporalNestingFlag() : true );
  }

  if ( sps->getVuiParametersPresentFlag() )
  {
    sps->getVuiParameters()->inferVideoSignalInfo( vps, pcSlice->getLayerId() ); 
  }


  pcSlice->setVPS(vps);      
  pcSlice->setSPS(sps);
  pcSlice->setPPS(pps);

  pcSlice->setViewId   ( vps->getViewId   ( pcSlice->getLayerId() )      );
  pcSlice->setViewIndex( vps->getViewIndex( pcSlice->getLayerId() )      );  
#if NH_3D
  pcSlice->setIsDepth  ( vps->getDepthId  ( pcSlice->getLayerId() ) == 1 );
#endif
#endif

  const ChromaFormat chFmt = sps->getChromaFormatIdc();
  const UInt numValidComp=getNumberValidComponents(chFmt);
  const Bool bChroma=(chFmt!=CHROMA_400);

  if( pps->getDependentSliceSegmentsEnabledFlag() && ( !firstSliceSegmentInPic ))
  {
    READ_FLAG( uiCode, "dependent_slice_segment_flag" );       pcSlice->setDependentSliceSegmentFlag(uiCode ? true : false);
  }
  else
  {
    pcSlice->setDependentSliceSegmentFlag(false);
  }
  Int numCTUs = ((sps->getPicWidthInLumaSamples()+sps->getMaxCUWidth()-1)/sps->getMaxCUWidth())*((sps->getPicHeightInLumaSamples()+sps->getMaxCUHeight()-1)/sps->getMaxCUHeight());
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
  pcSlice->setSliceSegmentCurStartCtuTsAddr( sliceSegmentAddress );// this is actually a Raster-Scan (RS) address, but we do not have the RS->TS conversion table defined yet.
  pcSlice->setSliceSegmentCurEndCtuTsAddr(numCTUs);                // Set end as the last CTU of the picture.

#if NH_MV
    UInt slicePicOrderCntLsb = 0;
#endif


  if (!pcSlice->getDependentSliceSegmentFlag())
  {
    pcSlice->setSliceCurStartCtuTsAddr(sliceSegmentAddress); // this is actually a Raster-Scan (RS) address, but we do not have the RS->TS conversion table defined yet.
    pcSlice->setSliceCurEndCtuTsAddr(numCTUs);
  }

  if(!pcSlice->getDependentSliceSegmentFlag())
  {
#if NH_MV    
    Int esb = 0; //Don't use i, otherwise will shadow something below

    if ( pps->getNumExtraSliceHeaderBits() > esb )
    {
      esb++; 
      READ_FLAG( uiCode, "discardable_flag" ); pcSlice->setDiscardableFlag( uiCode == 1 );
      if ( uiCode == 1 )
      {
        assert(pcSlice->getNalUnitType() != NAL_UNIT_CODED_SLICE_TRAIL_R &&
          pcSlice->getNalUnitType() != NAL_UNIT_CODED_SLICE_TSA_R &&
          pcSlice->getNalUnitType() != NAL_UNIT_CODED_SLICE_STSA_R &&
          pcSlice->getNalUnitType() != NAL_UNIT_CODED_SLICE_RADL_R &&
          pcSlice->getNalUnitType() != NAL_UNIT_CODED_SLICE_RASL_R);
      }
    }

    if ( pps->getNumExtraSliceHeaderBits() > esb )
    {
      esb++; 
      READ_FLAG( uiCode, "cross_layer_bla_flag" ); pcSlice->setCrossLayerBlaFlag( uiCode == 1 );
    }
    pcSlice->checkCrossLayerBlaFlag( ); 


    for (; esb < pps->getNumExtraSliceHeaderBits(); esb++)    
#else
    for (Int i = 0; i < pps->getNumExtraSliceHeaderBits(); i++)
#endif
    {
      READ_FLAG(uiCode, "slice_reserved_flag[]"); // ignored
    }

    READ_UVLC (    uiCode, "slice_type" );            pcSlice->setSliceType((SliceType)uiCode);
    if( pps->getOutputFlagPresentFlag() )
    {
      READ_FLAG( uiCode, "pic_output_flag" );    pcSlice->setPicOutputFlag( uiCode ? true : false );
    }
    else
    {
      pcSlice->setPicOutputFlag( true );
    }

    // if (separate_colour_plane_flag == 1)
    //   read colour_plane_id
    //   (separate_colour_plane_flag == 1) is not supported in this version of the standard.

#if NH_MV
    Int iPOClsb = slicePicOrderCntLsb;  // Needed later
    if ( (pcSlice->getLayerId() > 0 && !vps->getPocLsbNotPresentFlag( pcSlice->getLayerIdInVps())) || !pcSlice->getIdrPicFlag() )
    {
      READ_CODE(sps->getBitsForPOC(), slicePicOrderCntLsb, "slice_pic_order_cnt_lsb");        
    }    
    pcSlice->setSlicePicOrderCntLsb( slicePicOrderCntLsb ); 

    Bool picOrderCntMSBZeroFlag = false;     

    // as in HM code. However are all cases for IRAP picture with NoRaslOutputFlag equal to 1 covered??
    picOrderCntMSBZeroFlag = picOrderCntMSBZeroFlag || ( pcSlice->getNalUnitType() == NAL_UNIT_CODED_SLICE_BLA_W_LP   ); 
    picOrderCntMSBZeroFlag = picOrderCntMSBZeroFlag || ( pcSlice->getNalUnitType() == NAL_UNIT_CODED_SLICE_BLA_W_RADL ); 
    picOrderCntMSBZeroFlag = picOrderCntMSBZeroFlag || ( pcSlice->getNalUnitType() == NAL_UNIT_CODED_SLICE_BLA_N_LP   ); 
    picOrderCntMSBZeroFlag = picOrderCntMSBZeroFlag ||   pcSlice->getIdrPicFlag(); 

    // TBD picOrderCntMSBZeroFlag = picOrderCntMSBZeroFlag || ( rpcSlice->getLayerId() > 0 &&   !rpcSlice->getFirstPicInLayerDecodedFlag() ); 

    Int picOrderCntMSB = 0; 

    if ( !picOrderCntMSBZeroFlag )
    {
      Int prevPicOrderCnt    = prevTid0POC;
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
      
    pcSlice->setPOC( picOrderCntMSB + slicePicOrderCntLsb );
    if ( pcSlice->getPocResetFlag() )  
    {
      pcSlice->setPocBeforeReset   ( pcSlice->getPOC() ); 
      pcSlice->setPOC              ( 0 );
    }      
#endif

    if( pcSlice->getIdrPicFlag() )
    {
#if !NH_MV
      pcSlice->setPOC(0);
#endif
      TComReferencePictureSet* rps = pcSlice->getLocalRPS();
      (*rps)=TComReferencePictureSet();
      pcSlice->setRPS(rps);
#if NH_MV
      pcSlice->setEnableTMVPFlag(false);
#endif
    }
    else
    {
#if !NH_MV
      READ_CODE(sps->getBitsForPOC(), uiCode, "slice_pic_order_cnt_lsb");
      Int iPOClsb = uiCode;
      Int iPrevPOC = prevTid0POC;
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
      if ( pcSlice->getNalUnitType() == NAL_UNIT_CODED_SLICE_BLA_W_LP
        || pcSlice->getNalUnitType() == NAL_UNIT_CODED_SLICE_BLA_W_RADL
        || pcSlice->getNalUnitType() == NAL_UNIT_CODED_SLICE_BLA_N_LP )
      {
        // For BLA picture types, POCmsb is set to 0.
        iPOCmsb = 0;
      }
      pcSlice->setPOC              (iPOCmsb+iPOClsb);
#endif
      TComReferencePictureSet* rps;
      rps = pcSlice->getLocalRPS();
      (*rps)=TComReferencePictureSet();

      pcSlice->setRPS(rps);
      READ_FLAG( uiCode, "short_term_ref_pic_set_sps_flag" );
      if(uiCode == 0) // use short-term reference picture set explicitly signalled in slice header
      {
        parseShortTermRefPicSet(sps,rps, sps->getRPSList()->getNumberOfReferencePictureSets());
#if NH_MV
        if ( !rps->getInterRPSPrediction( ) )
        { // check sum of num_positive_pics and num_negative_pics
          rps->checkMaxNumPics( 
            vps->getVpsExtensionFlag(), 
            MAX_INT,  // To be replaced by MaxDbpSize
            pcSlice->getLayerId(), 
            sps->getMaxDecPicBuffering( sps->getSpsMaxSubLayersMinus1() ) - 1 );
        }
#endif

      }
      else // use reference to short-term reference picture set in PPS
      {
        Int numBits = 0;
        while ((1 << numBits) < sps->getRPSList()->getNumberOfReferencePictureSets())
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
        if (sps->getNumLongTermRefPicSPS() > 0)
        {
          READ_UVLC( uiCode, "num_long_term_sps");
          numLtrpInSPS = uiCode;
          numOfLtrp += numLtrpInSPS;
          rps->setNumberOfLongtermPictures(numOfLtrp);
        }
        Int bitsForLtrpInSPS = 0;
        while (sps->getNumLongTermRefPicSPS() > (1 << bitsForLtrpInSPS))
        {
          bitsForLtrpInSPS++;
        }
        READ_UVLC( uiCode, "num_long_term_pics");             rps->setNumberOfLongtermPictures(uiCode);
        numOfLtrp += uiCode;
        rps->setNumberOfLongtermPictures(numOfLtrp);
        Int maxPicOrderCntLSB = 1 << sps->getBitsForPOC();
        Int prevDeltaMSB = 0, deltaPocMSBCycleLT = 0;
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
            Bool usedByCurrFromSPS=sps->getUsedByCurrPicLtSPSFlag(uiCode);

            pocLsbLt = sps->getLtRefPicPocLsbSps(uiCode);
            rps->setUsed(j,usedByCurrFromSPS);
          }
          else
          {
            READ_CODE(sps->getBitsForPOC(), uiCode, "poc_lsb_lt"); pocLsbLt= uiCode;
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

            Int pocLTCurr = pcSlice->getPOC() - deltaPocMSBCycleLT * maxPicOrderCntLSB
                                        - iPOClsb + pocLsbLt;
            rps->setPOC     (j, pocLTCurr);
            rps->setDeltaPOC(j, - pcSlice->getPOC() + pocLTCurr);
            rps->setCheckLTMSBPresent(j,true);
          }
          else
          {
            rps->setPOC     (j, pocLsbLt);
            rps->setDeltaPOC(j, - pcSlice->getPOC() + pocLsbLt);
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
#if NH_MV
      if ( !rps->getInterRPSPrediction( ) )
      { // check sum of NumPositivePics, NumNegativePics, num_long_term_sps and num_long_term_pics 
        rps->checkMaxNumPics( 
          vps->getVpsExtensionFlag(), 
            MAX_INT,  // To be replaced by MaxDbpsize
          pcSlice->getLayerId(), 
          sps->getMaxDecPicBuffering( sps->getSpsMaxSubLayersMinus1() ) - 1 );
      }
#endif

      if ( pcSlice->getNalUnitType() == NAL_UNIT_CODED_SLICE_BLA_W_LP
        || pcSlice->getNalUnitType() == NAL_UNIT_CODED_SLICE_BLA_W_RADL
        || pcSlice->getNalUnitType() == NAL_UNIT_CODED_SLICE_BLA_N_LP )
      {
        // In the case of BLA picture types, rps data is read from slice header but ignored
        rps = pcSlice->getLocalRPS();
        (*rps)=TComReferencePictureSet();
        pcSlice->setRPS(rps);
      }
      if (sps->getTMVPFlagsPresent())
      {
#if NH_MV
        READ_FLAG( uiCode, "slice_temporal_mvp_enabled_flag" );
#else
        READ_FLAG( uiCode, "slice_temporal_mvp_enabled_flag" );
#endif
        pcSlice->setEnableTMVPFlag( uiCode == 1 ? true : false );
      }
      else
      {
        pcSlice->setEnableTMVPFlag(false);
      }
    }
#if NH_MV
    Bool interLayerPredLayerIdcPresentFlag = false; 
    Int layerId       = pcSlice->getLayerId(); 
#if NH_3D
    if( pcSlice->getLayerId() > 0 && !vps->getAllRefLayersActiveFlag() && vps->getNumRefListLayers( layerId ) > 0 )
#else
    if( pcSlice->getLayerId() > 0 && !vps->getAllRefLayersActiveFlag() && vps->getNumDirectRefLayers( layerId ) > 0 )
#endif
    {   
      READ_FLAG( uiCode, "inter_layer_pred_enabled_flag" ); pcSlice->setInterLayerPredEnabledFlag( uiCode == 1 );
#if NH_3D
      if( pcSlice->getInterLayerPredEnabledFlag() && vps->getNumRefListLayers( layerId ) > 1 )
#else
      if( pcSlice->getInterLayerPredEnabledFlag() && vps->getNumDirectRefLayers( layerId ) > 1 )
#endif
      {            
        if( !vps->getMaxOneActiveRefLayerFlag())  
        {
          READ_CODE( pcSlice->getNumInterLayerRefPicsMinus1Len( ), uiCode, "num_inter_layer_ref_pics_minus1" ); pcSlice->setNumInterLayerRefPicsMinus1( uiCode );
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
            READ_CODE( pcSlice->getInterLayerPredLayerIdcLen( ), uiCode, "inter_layer_pred_layer_idc" ); pcSlice->setInterLayerPredLayerIdc( idx, uiCode );
          }
        }
      }  
    }
    if ( !interLayerPredLayerIdcPresentFlag )
    {
      for( Int i = 0; i < pcSlice->getNumActiveRefLayerPics(); i++ )   
      {
        pcSlice->setInterLayerPredLayerIdc( i, pcSlice->getRefLayerPicIdc( i ) );
      }
    }
#if NH_3D
    if ( getDecTop()->decProcAnnexI() )
    {    
      pcSlice->deriveInCmpPredAndCpAvailFlag( );
      if ( pcSlice->getInCmpPredAvailFlag() )
      {
        READ_FLAG(uiCode, "in_comp_pred_flag");  pcSlice->setInCompPredFlag((Bool)uiCode);      
      }
      pcSlice->init3dToolParameters(); 
    }
#endif
#endif
    if(sps->getUseSAO())
    {
      READ_FLAG(uiCode, "slice_sao_luma_flag");  pcSlice->setSaoEnabledFlag(CHANNEL_TYPE_LUMA, (Bool)uiCode);

      if (bChroma)
      {
        READ_FLAG(uiCode, "slice_sao_chroma_flag");  pcSlice->setSaoEnabledFlag(CHANNEL_TYPE_CHROMA, (Bool)uiCode);
      }
    }

    if (pcSlice->getIdrPicFlag())
    {
      pcSlice->setEnableTMVPFlag(false);
    }
    if (!pcSlice->isIntra())
    {

      READ_FLAG( uiCode, "num_ref_idx_active_override_flag");
      if (uiCode)
      {
        READ_UVLC (uiCode, "num_ref_idx_l0_active_minus1" );  pcSlice->setNumRefIdx( REF_PIC_LIST_0, uiCode + 1 );
        if (pcSlice->isInterB())
        {
          READ_UVLC (uiCode, "num_ref_idx_l1_active_minus1" );  pcSlice->setNumRefIdx( REF_PIC_LIST_1, uiCode + 1 );
        }
        else
        {
          pcSlice->setNumRefIdx(REF_PIC_LIST_1, 0);
        }
      }
      else
      {
        pcSlice->setNumRefIdx(REF_PIC_LIST_0, pps->getNumRefIdxL0DefaultActive());
        if (pcSlice->isInterB())
        {
          pcSlice->setNumRefIdx(REF_PIC_LIST_1, pps->getNumRefIdxL1DefaultActive());
        }
        else
        {
          pcSlice->setNumRefIdx(REF_PIC_LIST_1,0);
        }
      }
    }
    // }
    TComRefPicListModification* refPicListModification = pcSlice->getRefPicListModification();
    if(!pcSlice->isIntra())
    {
      if( !pps->getListsModificationPresentFlag() || pcSlice->getNumRpsCurrTempList() <= 1 )
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
        Int numRpsCurrTempList0 = pcSlice->getNumRpsCurrTempList();
        if ( numRpsCurrTempList0 > 1 )
        {
          Int length = 1;
          numRpsCurrTempList0 --;
          while ( numRpsCurrTempList0 >>= 1)
          {
            length ++;
          }
          for (i = 0; i < pcSlice->getNumRefIdx(REF_PIC_LIST_0); i ++)
          {
            READ_CODE( length, uiCode, "list_entry_l0" );
            refPicListModification->setRefPicSetIdxL0(i, uiCode );
          }
        }
        else
        {
          for (i = 0; i < pcSlice->getNumRefIdx(REF_PIC_LIST_0); i ++)
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
    if(pcSlice->isInterB())
    {
      if( !pps->getListsModificationPresentFlag() || pcSlice->getNumRpsCurrTempList() <= 1 )
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
        Int numRpsCurrTempList1 = pcSlice->getNumRpsCurrTempList();
        if ( numRpsCurrTempList1 > 1 )
        {
          Int length = 1;
          numRpsCurrTempList1 --;
          while ( numRpsCurrTempList1 >>= 1)
          {
            length ++;
          }
          for (i = 0; i < pcSlice->getNumRefIdx(REF_PIC_LIST_1); i ++)
          {
            READ_CODE( length, uiCode, "list_entry_l1" );
            refPicListModification->setRefPicSetIdxL1(i, uiCode );
          }
        }
        else
        {
          for (i = 0; i < pcSlice->getNumRefIdx(REF_PIC_LIST_1); i ++)
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
    if (pcSlice->isInterB())
    {
      READ_FLAG( uiCode, "mvd_l1_zero_flag" );       pcSlice->setMvdL1ZeroFlag( (uiCode ? true : false) );
    }

    pcSlice->setCabacInitFlag( false ); // default
    if(pps->getCabacInitPresentFlag() && !pcSlice->isIntra())
    {
      READ_FLAG(uiCode, "cabac_init_flag");
      pcSlice->setCabacInitFlag( uiCode ? true : false );
    }

    if ( pcSlice->getEnableTMVPFlag() )
    {
      if ( pcSlice->getSliceType() == B_SLICE )
      {
        READ_FLAG( uiCode, "collocated_from_l0_flag" );
        pcSlice->setColFromL0Flag(uiCode);
      }
      else
      {
        pcSlice->setColFromL0Flag( 1 );
      }

      if ( pcSlice->getSliceType() != I_SLICE &&
          ((pcSlice->getColFromL0Flag() == 1 && pcSlice->getNumRefIdx(REF_PIC_LIST_0) > 1)||
           (pcSlice->getColFromL0Flag() == 0 && pcSlice->getNumRefIdx(REF_PIC_LIST_1) > 1)))
      {
        READ_UVLC( uiCode, "collocated_ref_idx" );
        pcSlice->setColRefIdx(uiCode);
      }
      else
      {
        pcSlice->setColRefIdx(0);
      }
    }
    if ( (pps->getUseWP() && pcSlice->getSliceType()==P_SLICE) || (pps->getWPBiPred() && pcSlice->getSliceType()==B_SLICE) )
    {
      xParsePredWeightTable(pcSlice, sps);
      pcSlice->initWpScaling(sps);
    }

#if NH_3D_IC
    else if(    pcSlice->getViewIndex() && ( pcSlice->getSliceType() == P_SLICE || pcSlice->getSliceType() == B_SLICE ) 
             && !pcSlice->getIsDepth() && vps->getNumRefListLayers( layerId ) > 0 
             && getDecTop()->decProcAnnexI()
           )
    {
      UInt uiCodeTmp = 0;

      READ_FLAG ( uiCodeTmp, "slice_ic_enable_flag" );
      pcSlice->setApplyIC( uiCodeTmp );

      if ( uiCodeTmp )
      {
        READ_FLAG ( uiCodeTmp, "slice_ic_disabled_merge_zero_idx_flag" );
        pcSlice->setIcSkipParseFlag( uiCodeTmp );
      }
    }
#endif

    if (!pcSlice->isIntra())
    {
      READ_UVLC( uiCode, "five_minus_max_num_merge_cand");
#if NH_3D_IV_MERGE
      pcSlice->setMaxNumMergeCand(( ( pcSlice->getMpiFlag() || pcSlice->getIvMvPredFlag() || pcSlice->getViewSynthesisPredFlag() ) ? MRG_MAX_NUM_CANDS_MEM : MRG_MAX_NUM_CANDS) - uiCode);
#else
      pcSlice->setMaxNumMergeCand(MRG_MAX_NUM_CANDS - uiCode);
#endif
    }

    READ_SVLC( iCode, "slice_qp_delta" );
    pcSlice->setSliceQp (26 + pps->getPicInitQPMinus26() + iCode);

    assert( pcSlice->getSliceQp() >= -sps->getQpBDOffset(CHANNEL_TYPE_LUMA) );
    assert( pcSlice->getSliceQp() <=  51 );

    if (pps->getSliceChromaQpFlag())
    {
      if (numValidComp>COMPONENT_Cb)
      {
        READ_SVLC( iCode, "slice_cb_qp_offset" );
        pcSlice->setSliceChromaQpDelta(COMPONENT_Cb, iCode );
        assert( pcSlice->getSliceChromaQpDelta(COMPONENT_Cb) >= -12 );
        assert( pcSlice->getSliceChromaQpDelta(COMPONENT_Cb) <=  12 );
        assert( (pps->getQpOffset(COMPONENT_Cb) + pcSlice->getSliceChromaQpDelta(COMPONENT_Cb)) >= -12 );
        assert( (pps->getQpOffset(COMPONENT_Cb) + pcSlice->getSliceChromaQpDelta(COMPONENT_Cb)) <=  12 );
      }

      if (numValidComp>COMPONENT_Cr)
      {
        READ_SVLC( iCode, "slice_cr_qp_offset" );
        pcSlice->setSliceChromaQpDelta(COMPONENT_Cr, iCode );
        assert( pcSlice->getSliceChromaQpDelta(COMPONENT_Cr) >= -12 );
        assert( pcSlice->getSliceChromaQpDelta(COMPONENT_Cr) <=  12 );
        assert( (pps->getQpOffset(COMPONENT_Cr) + pcSlice->getSliceChromaQpDelta(COMPONENT_Cr)) >= -12 );
        assert( (pps->getQpOffset(COMPONENT_Cr) + pcSlice->getSliceChromaQpDelta(COMPONENT_Cr)) <=  12 );
      }
    }

    if (pps->getPpsRangeExtension().getChromaQpOffsetListEnabledFlag())
    {
      READ_FLAG(uiCode, "cu_chroma_qp_offset_enabled_flag"); pcSlice->setUseChromaQpAdj(uiCode != 0);
    }
    else
    {
      pcSlice->setUseChromaQpAdj(false);
    }

    if (pps->getDeblockingFilterControlPresentFlag())
    {
      if(pps->getDeblockingFilterOverrideEnabledFlag())
      {
        READ_FLAG ( uiCode, "deblocking_filter_override_flag" );        pcSlice->setDeblockingFilterOverrideFlag(uiCode ? true : false);
      }
      else
      {
        pcSlice->setDeblockingFilterOverrideFlag(0);
      }
      if(pcSlice->getDeblockingFilterOverrideFlag())
      {
        READ_FLAG ( uiCode, "slice_disable_deblocking_filter_flag" );   pcSlice->setDeblockingFilterDisable(uiCode ? 1 : 0);
        if(!pcSlice->getDeblockingFilterDisable())
        {
          READ_SVLC( iCode, "slice_beta_offset_div2" );                       pcSlice->setDeblockingFilterBetaOffsetDiv2(iCode);
          assert(pcSlice->getDeblockingFilterBetaOffsetDiv2() >= -6 &&
                 pcSlice->getDeblockingFilterBetaOffsetDiv2() <=  6);
          READ_SVLC( iCode, "slice_tc_offset_div2" );                         pcSlice->setDeblockingFilterTcOffsetDiv2(iCode);
          assert(pcSlice->getDeblockingFilterTcOffsetDiv2() >= -6 &&
                 pcSlice->getDeblockingFilterTcOffsetDiv2() <=  6);
        }
      }
      else
      {
        pcSlice->setDeblockingFilterDisable   ( pps->getPicDisableDeblockingFilterFlag() );
        pcSlice->setDeblockingFilterBetaOffsetDiv2( pps->getDeblockingFilterBetaOffsetDiv2() );
        pcSlice->setDeblockingFilterTcOffsetDiv2  ( pps->getDeblockingFilterTcOffsetDiv2() );
      }
    }
    else
    {
      pcSlice->setDeblockingFilterDisable       ( false );
      pcSlice->setDeblockingFilterBetaOffsetDiv2( 0 );
      pcSlice->setDeblockingFilterTcOffsetDiv2  ( 0 );
    }

    Bool isSAOEnabled = sps->getUseSAO() && (pcSlice->getSaoEnabledFlag(CHANNEL_TYPE_LUMA) || (bChroma && pcSlice->getSaoEnabledFlag(CHANNEL_TYPE_CHROMA)));
    Bool isDBFEnabled = (!pcSlice->getDeblockingFilterDisable());

    if(pps->getLoopFilterAcrossSlicesEnabledFlag() && ( isSAOEnabled || isDBFEnabled ))
    {
      READ_FLAG( uiCode, "slice_loop_filter_across_slices_enabled_flag");
    }
    else
    {
      uiCode = pps->getLoopFilterAcrossSlicesEnabledFlag()?1:0;
    }
    pcSlice->setLFCrossSliceBoundaryFlag( (uiCode==1)?true:false);

#if NH_3D
    if ( getDecTop()->decProcAnnexI() )
    {
      Int voiInVps = vps->getVoiInVps( pcSlice->getViewIndex() ); 
      if( vps->getCpInSliceSegmentHeaderFlag( voiInVps ) && !pcSlice->getIsDepth() )
      {
        for( Int m = 0; m < vps->getNumCp( voiInVps ); m++ )
        {
          Int jInVps = vps->getVoiInVps( vps->getCpRefVoi( voiInVps, m ));
          READ_SVLC( iCode, "cp_scale" );                pcSlice->setCpScale   ( jInVps, iCode );
          READ_SVLC( iCode, "cp_off" );                  pcSlice->setCpOff     ( jInVps, iCode );
          READ_SVLC( iCode, "cp_inv_scale_plus_scale" ); pcSlice->setCpInvScale( jInVps, iCode - pcSlice->getCpScale   ( jInVps ));
          READ_SVLC( iCode, "cp_inv_off_plus_off" );     pcSlice->setCpInvOff  ( jInVps, iCode - pcSlice->getCpOff     ( jInVps ));
        }
      }
    }
#endif

  }

  std::vector<UInt> entryPointOffset;
  if( pps->getTilesEnabledFlag() || pps->getEntropyCodingSyncEnabledFlag() )
  {
    UInt numEntryPointOffsets;
    UInt offsetLenMinus1;
    READ_UVLC(numEntryPointOffsets, "num_entry_point_offsets");
    if (numEntryPointOffsets>0)
    {
      READ_UVLC(offsetLenMinus1, "offset_len_minus1");
      entryPointOffset.resize(numEntryPointOffsets);
      for (UInt idx=0; idx<numEntryPointOffsets; idx++)
      {
        READ_CODE(offsetLenMinus1+1, uiCode, "entry_point_offset_minus1");
        entryPointOffset[ idx ] = uiCode + 1;
      }
    }
  }

  if(pps->getSliceHeaderExtensionPresentFlag())
  {
#if NH_MV
    READ_UVLC( uiCode, "slice_segment_header_extension_length" ); pcSlice->setSliceSegmentHeaderExtensionLength( uiCode );
    UInt posFollSliceSegHeaderExtLen = m_pcBitstream->getNumBitsRead();

    if( pps->getPocResetInfoPresentFlag() )
    {
      READ_CODE( 2, uiCode, "poc_reset_idc" ); pcSlice->setPocResetIdc( uiCode );
    }
    else
    {
      pcSlice->setPocResetIdc( 0 );
    }
    pcSlice->checkPocResetIdc(); 

    if ( pcSlice->getVPS()->getPocLsbNotPresentFlag(pcSlice->getLayerId()) && slicePicOrderCntLsb > 0 )
    {
      assert( pcSlice->getPocResetIdc() != 2 );
    }

    if( pcSlice->getPocResetIdc() !=  0 )
    {
      READ_CODE( 6, uiCode, "poc_reset_period_id" ); pcSlice->setPocResetPeriodId( uiCode );
    }
    else
    {
      // TODO Copy poc_reset_period from earlier picture
      pcSlice->setPocResetPeriodId( 0 );
    }
    
    if( pcSlice->getPocResetIdc() ==  3 ) 
    {
      READ_FLAG( uiCode, "full_poc_reset_flag" ); pcSlice->setFullPocResetFlag( uiCode == 1 );
      READ_CODE( pcSlice->getPocLsbValLen() , uiCode, "poc_lsb_val" ); pcSlice->setPocLsbVal( uiCode );
    }          
    pcSlice->checkPocLsbVal(); 

    // Derive the value of PocMs8bValRequiredFlag

    if( !pcSlice->getPocMsbValRequiredFlag() && pcSlice->getVPS()->getVpsPocLsbAlignedFlag() )
    {
      READ_FLAG( uiCode, "poc_msb_val_present_flag" ); pcSlice->setPocMsbValPresentFlag( uiCode == 1 );
    }
    else
    {
      pcSlice->setPocMsbValPresentFlag( pcSlice->inferPocMsbValPresentFlag( ) ); 
    }

    
    if( pcSlice->getPocMsbValPresentFlag() )
    {
      READ_UVLC( uiCode, "poc_msb_val" ); pcSlice->setPocMsbVal( uiCode );
    }

    while( ( m_pcBitstream->getNumBitsRead() - posFollSliceSegHeaderExtLen ) < pcSlice->getSliceSegmentHeaderExtensionLength() * 8 )
    {
     READ_FLAG( uiCode, "slice_segment_header_extension_data_bit" );
    }
    assert( ( m_pcBitstream->getNumBitsRead() - posFollSliceSegHeaderExtLen ) == pcSlice->getSliceSegmentHeaderExtensionLength() * 8  ); 
  }
#else

    READ_UVLC(uiCode,"slice_segment_header_extension_length");
    for(Int i=0; i<uiCode; i++)
    {
      UInt ignore;
      READ_CODE(8,ignore,"slice_segment_header_extension_data_byte");
    }
  }
#endif
#if RExt__DECODER_DEBUG_BIT_STATISTICS
  TComCodingStatistics::IncrementStatisticEP(STATS__BYTE_ALIGNMENT_BITS,m_pcBitstream->readByteAlignment(),0);
#else
  m_pcBitstream->readByteAlignment();
#endif

  pcSlice->clearSubstreamSizes();

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
    for (UInt idx=0; idx<entryPointOffset.size(); idx++)
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
      pcSlice->addSubstreamSize(entryPointOffset [ idx ] );
    }
  }

  return;
}

Void TDecCavlc::parsePTL( TComPTL *rpcPTL, Bool profilePresentFlag, Int maxNumSubLayersMinus1 )
{
  UInt uiCode;
  if(profilePresentFlag)
  {
    parseProfileTier(rpcPTL->getGeneralPTL(), false);
  }
  READ_CODE( 8, uiCode, "general_level_idc" );    rpcPTL->getGeneralPTL()->setLevelIdc(Level::Name(uiCode));

  for (Int i = 0; i < maxNumSubLayersMinus1; i++)
  {
    READ_FLAG( uiCode, "sub_layer_profile_present_flag[i]" ); rpcPTL->setSubLayerProfilePresentFlag(i, uiCode);
    READ_FLAG( uiCode, "sub_layer_level_present_flag[i]"   ); rpcPTL->setSubLayerLevelPresentFlag  (i, uiCode);
#if NH_MV
    // When profilePresentFlag is equal to 0, sub_layer_profile_present_flag[ i ] shall be equal to 0.
    assert( profilePresentFlag || !rpcPTL->getSubLayerProfilePresentFlag(i) );
#endif
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
    if( rpcPTL->getSubLayerProfilePresentFlag(i) )         
    {
      parseProfileTier(rpcPTL->getSubLayerPTL(i), true);
    }
    if(rpcPTL->getSubLayerLevelPresentFlag(i))
    {
      READ_CODE( 8, uiCode, "sub_layer_level_idc[i]" );   rpcPTL->getSubLayerPTL(i)->setLevelIdc(Level::Name(uiCode));
    }
  }
}

#if ENC_DEC_TRACE || RExt__DECODER_DEBUG_BIT_STATISTICS
Void TDecCavlc::parseProfileTier(ProfileTierLevel *ptl, const Bool bIsSubLayer)
#define PTL_TRACE_TEXT(txt) bIsSubLayer?("sub_layer_" txt) : ("general_" txt)
#else
Void TDecCavlc::parseProfileTier(ProfileTierLevel *ptl, const Bool /*bIsSubLayer*/)
#define PTL_TRACE_TEXT(txt) txt
#endif
{
  UInt uiCode;
  READ_CODE(2 , uiCode,   PTL_TRACE_TEXT("profile_space"                   )); ptl->setProfileSpace(uiCode);
  READ_FLAG(    uiCode,   PTL_TRACE_TEXT("tier_flag"                       )); ptl->setTierFlag    (uiCode ? Level::HIGH : Level::MAIN);
  READ_CODE(5 , uiCode,   PTL_TRACE_TEXT("profile_idc"                     )); ptl->setProfileIdc  (Profile::Name(uiCode));
  for(Int j = 0; j < 32; j++)
  {
    READ_FLAG(  uiCode,   PTL_TRACE_TEXT("profile_compatibility_flag[][j]" )); ptl->setProfileCompatibilityFlag(j, uiCode ? 1 : 0);
  }
  READ_FLAG(uiCode,       PTL_TRACE_TEXT("progressive_source_flag"         )); ptl->setProgressiveSourceFlag(uiCode ? true : false);

  READ_FLAG(uiCode,       PTL_TRACE_TEXT("interlaced_source_flag"          )); ptl->setInterlacedSourceFlag(uiCode ? true : false);

  READ_FLAG(uiCode,       PTL_TRACE_TEXT("non_packed_constraint_flag"      )); ptl->setNonPackedConstraintFlag(uiCode ? true : false);

  READ_FLAG(uiCode,       PTL_TRACE_TEXT("frame_only_constraint_flag"      )); ptl->setFrameOnlyConstraintFlag(uiCode ? true : false);
#if NH_MV
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

  if (ptl->getProfileIdc() == Profile::MAINREXT           || ptl->getProfileCompatibilityFlag(Profile::MAINREXT) ||
      ptl->getProfileIdc() == Profile::HIGHTHROUGHPUTREXT || ptl->getProfileCompatibilityFlag(Profile::HIGHTHROUGHPUTREXT))
  {
    UInt maxBitDepth=16;
    READ_FLAG(    uiCode, PTL_TRACE_TEXT("max_12bit_constraint_flag"       )); if (uiCode) maxBitDepth=12;
    READ_FLAG(    uiCode, PTL_TRACE_TEXT("max_10bit_constraint_flag"       )); if (uiCode) maxBitDepth=10;
    READ_FLAG(    uiCode, PTL_TRACE_TEXT("max_8bit_constraint_flag"        )); if (uiCode) maxBitDepth=8;
    ptl->setBitDepthConstraint(maxBitDepth);
    ChromaFormat chromaFmtConstraint=CHROMA_444;
    READ_FLAG(    uiCode, PTL_TRACE_TEXT("max_422chroma_constraint_flag"   )); if (uiCode) chromaFmtConstraint=CHROMA_422;
    READ_FLAG(    uiCode, PTL_TRACE_TEXT("max_420chroma_constraint_flag"   )); if (uiCode) chromaFmtConstraint=CHROMA_420;
    READ_FLAG(    uiCode, PTL_TRACE_TEXT("max_monochrome_constraint_flag"  )); if (uiCode) chromaFmtConstraint=CHROMA_400;
    ptl->setChromaFormatConstraint(chromaFmtConstraint);
    READ_FLAG(    uiCode, PTL_TRACE_TEXT("intra_constraint_flag"           )); ptl->setIntraConstraintFlag(uiCode != 0);
    READ_FLAG(    uiCode, PTL_TRACE_TEXT("one_picture_only_constraint_flag")); ptl->setOnePictureOnlyConstraintFlag(uiCode != 0);
    READ_FLAG(    uiCode, PTL_TRACE_TEXT("lower_bit_rate_constraint_flag"  )); ptl->setLowerBitRateConstraintFlag(uiCode != 0);
    READ_CODE(16, uiCode, PTL_TRACE_TEXT("reserved_zero_34bits[0..15]"     ));
    READ_CODE(16, uiCode, PTL_TRACE_TEXT("reserved_zero_34bits[16..31]"    ));
    READ_CODE(2,  uiCode, PTL_TRACE_TEXT("reserved_zero_34bits[32..33]"    ));
  }
  else
  {
    ptl->setBitDepthConstraint((ptl->getProfileIdc() == Profile::MAIN10)?10:8);
    ptl->setChromaFormatConstraint(CHROMA_420);
    ptl->setIntraConstraintFlag(false);
    ptl->setLowerBitRateConstraintFlag(true);
    READ_CODE(16, uiCode, PTL_TRACE_TEXT("reserved_zero_43bits[0..15]"     ));
    READ_CODE(16, uiCode, PTL_TRACE_TEXT("reserved_zero_43bits[16..31]"    ));
    READ_CODE(11, uiCode, PTL_TRACE_TEXT("reserved_zero_43bits[32..42]"    ));
  }

  if ((ptl->getProfileIdc() >= Profile::MAIN && ptl->getProfileIdc() <= Profile::HIGHTHROUGHPUTREXT) ||
       ptl->getProfileCompatibilityFlag(Profile::MAIN) ||
       ptl->getProfileCompatibilityFlag(Profile::MAIN10) ||
       ptl->getProfileCompatibilityFlag(Profile::MAINSTILLPICTURE) ||
       ptl->getProfileCompatibilityFlag(Profile::MAINREXT) ||
       ptl->getProfileCompatibilityFlag(Profile::HIGHTHROUGHPUTREXT) )
  {
    READ_FLAG(    uiCode, PTL_TRACE_TEXT("inbld_flag"                      )); assert(uiCode == 0);
  }
  else
  {
    READ_FLAG(    uiCode, PTL_TRACE_TEXT("reserved_zero_bit"               ));
  }
#undef PTL_TRACE_TEXT
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

Void TDecCavlc::parseRemainingBytes( Bool noTrailingBytesExpected )
{
  if (noTrailingBytesExpected)
  {
    const UInt numberOfRemainingSubstreamBytes=m_pcBitstream->getNumBitsLeft();
    assert (numberOfRemainingSubstreamBytes == 0);
  }
  else
  {
    while (m_pcBitstream->getNumBitsLeft())
    {
      UInt trailingNullByte=m_pcBitstream->readByte();
      if (trailingNullByte!=0)
      {
        printf("Trailing byte should be 0, but has value %02x\n", trailingNullByte);
        assert(trailingNullByte==0);
      }
    }
  }
}

#if NH_3D_DIS
Void TDecCavlc::parseDIS( TComDataCU* /*pcCU*/, UInt /*uiAbsPartIdx*/, UInt /*uiDepth*/ )
{
  assert(0);
}
#endif

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

Void TDecCavlc::parseCrossComponentPrediction( class TComTU& /*rTu*/, ComponentID /*compID*/ )
{
  assert(0);
}

Void TDecCavlc::parseDeltaQP( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth )
{
  Int  iDQp;

#if RExt__DECODER_DEBUG_BIT_STATISTICS
  READ_SVLC(iDQp, "delta_qp");
#else
  xReadSvlc( iDQp );
#endif

  Int qpBdOffsetY = pcCU->getSlice()->getSPS()->getQpBDOffset(CHANNEL_TYPE_LUMA);
  const Int qp = (((Int) pcCU->getRefQP( uiAbsPartIdx ) + iDQp + 52 + 2*qpBdOffsetY )%(52+ qpBdOffsetY)) -  qpBdOffsetY;

  const UInt maxCUDepth        = pcCU->getSlice()->getSPS()->getMaxTotalCUDepth();
  const UInt maxCuDQPDepth     = pcCU->getSlice()->getPPS()->getMaxCuDQPDepth();
  const UInt doubleDepthDifference = ((maxCUDepth - maxCuDQPDepth)<<1);
  const UInt uiAbsQpCUPartIdx = (uiAbsPartIdx>>doubleDepthDifference)<<doubleDepthDifference ;
  const UInt uiQpCUDepth =   min(uiDepth,pcCU->getSlice()->getPPS()->getMaxCuDQPDepth()) ;

  pcCU->setQPSubParts( qp, uiAbsQpCUPartIdx, uiQpCUDepth );
}

Void TDecCavlc::parseChromaQpAdjustment( TComDataCU* /*pcCU*/, UInt /*uiAbsPartIdx*/, UInt /*uiDepth*/ )
{
  assert(0);
}

Void TDecCavlc::parseCoeffNxN( TComTU &/*rTu*/, ComponentID /*compID*/ )
{
  assert(0);
}

Void TDecCavlc::parseTransformSubdivFlag( UInt& /*ruiSubdivFlag*/, UInt /*uiLog2TransformBlockSize*/ )
{
  assert(0);
}

Void TDecCavlc::parseQtCbf( TComTU &/*rTu*/, const ComponentID /*compID*/, const Bool /*lowestLevel*/ )
{
  assert(0);
}

Void TDecCavlc::parseQtRootCbf( UInt /*uiAbsPartIdx*/, UInt& /*uiQtRootCbf*/ )
{
  assert(0);
}

Void TDecCavlc::parseTransformSkipFlags (TComTU &/*rTu*/, ComponentID /*component*/)
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

#if NH_3D_ARP
Void TDecCavlc::parseARPW( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth )
{
  assert(0);
}
#endif
#if NH_3D_IC
Void TDecCavlc::parseICFlag( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth )
{
  assert(0);
}
#endif
#if NH_3D_SDC_INTRA || NH_3D_SDC_INTER
Void TDecCavlc::parseSDCFlag    ( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth )
{
  assert(0);
}

#endif
#if NH_3D_DBBP
  Void TDecCavlc::parseDBBPFlag( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth )
  {
    assert(0);
  }
#endif
// ====================================================================================================================
// Protected member functions
// ====================================================================================================================

//! parse explicit wp tables
Void TDecCavlc::xParsePredWeightTable( TComSlice* pcSlice, const TComSPS *sps )
{
        WPScalingParam *wp;
  const ChromaFormat    chFmt        = sps->getChromaFormatIdc();
  const Int             numValidComp = Int(getNumberValidComponents(chFmt));
  const Bool            bChroma      = (chFmt!=CHROMA_400);
  const SliceType       eSliceType   = pcSlice->getSliceType();
  const Int             iNbRef       = (eSliceType == B_SLICE ) ? (2) : (1);
        UInt            uiLog2WeightDenomLuma=0, uiLog2WeightDenomChroma=0;
        UInt            uiTotalSignalledWeightFlags = 0;

  Int iDeltaDenom;
  // decode delta_luma_log2_weight_denom :
  READ_UVLC( uiLog2WeightDenomLuma, "luma_log2_weight_denom" );
  assert( uiLog2WeightDenomLuma <= 7 );
  if( bChroma )
  {
    READ_SVLC( iDeltaDenom, "delta_chroma_log2_weight_denom" );
    assert((iDeltaDenom + (Int)uiLog2WeightDenomLuma)>=0);
    assert((iDeltaDenom + (Int)uiLog2WeightDenomLuma)<=7);
    uiLog2WeightDenomChroma = (UInt)(iDeltaDenom + uiLog2WeightDenomLuma);
  }



  for ( Int iNumRef=0 ; iNumRef<iNbRef ; iNumRef++ ) // loop over l0 and l1 syntax elements
  {
    RefPicList  eRefPicList = ( iNumRef ? REF_PIC_LIST_1 : REF_PIC_LIST_0 );
    for ( Int iRefIdx=0 ; iRefIdx<pcSlice->getNumRefIdx(eRefPicList) ; iRefIdx++ )
    {
      pcSlice->getWpScaling(eRefPicList, iRefIdx, wp);

      wp[COMPONENT_Y].uiLog2WeightDenom = uiLog2WeightDenomLuma;
      for(Int j=1; j<numValidComp; j++)
      {
        wp[j].uiLog2WeightDenom = uiLog2WeightDenomChroma;
      }

      UInt  uiCode;
      READ_FLAG( uiCode, iNumRef==0?"luma_weight_l0_flag[i]":"luma_weight_l1_flag[i]" );
      wp[COMPONENT_Y].bPresentFlag = ( uiCode == 1 );
      uiTotalSignalledWeightFlags += wp[COMPONENT_Y].bPresentFlag;
    }
    if ( bChroma )
    {
      UInt  uiCode;
      for ( Int iRefIdx=0 ; iRefIdx<pcSlice->getNumRefIdx(eRefPicList) ; iRefIdx++ )
      {
        pcSlice->getWpScaling(eRefPicList, iRefIdx, wp);
        READ_FLAG( uiCode, iNumRef==0?"chroma_weight_l0_flag[i]":"chroma_weight_l1_flag[i]" );
        for(Int j=1; j<numValidComp; j++)
        {
          wp[j].bPresentFlag = ( uiCode == 1 );
        }
        uiTotalSignalledWeightFlags += 2*wp[COMPONENT_Cb].bPresentFlag;
      }
    }
    for ( Int iRefIdx=0 ; iRefIdx<pcSlice->getNumRefIdx(eRefPicList) ; iRefIdx++ )
    {
      pcSlice->getWpScaling(eRefPicList, iRefIdx, wp);
      if ( wp[COMPONENT_Y].bPresentFlag )
      {
        Int iDeltaWeight;
        READ_SVLC( iDeltaWeight, iNumRef==0?"delta_luma_weight_l0[i]":"delta_luma_weight_l1[i]" );
        assert( iDeltaWeight >= -128 );
        assert( iDeltaWeight <=  127 );
        wp[COMPONENT_Y].iWeight = (iDeltaWeight + (1<<wp[COMPONENT_Y].uiLog2WeightDenom));
        READ_SVLC( wp[COMPONENT_Y].iOffset, iNumRef==0?"luma_offset_l0[i]":"luma_offset_l1[i]" );
        Int range=sps->getSpsRangeExtension().getHighPrecisionOffsetsEnabledFlag() ? (1<<sps->getBitDepth(CHANNEL_TYPE_LUMA))/2 : 128;
        assert( wp[0].iOffset >= -range );
        assert( wp[0].iOffset <   range );
      }
      else
      {
        wp[COMPONENT_Y].iWeight = (1 << wp[COMPONENT_Y].uiLog2WeightDenom);
        wp[COMPONENT_Y].iOffset = 0;
      }
      if ( bChroma )
      {
        if ( wp[COMPONENT_Cb].bPresentFlag )
        {
          Int range=sps->getSpsRangeExtension().getHighPrecisionOffsetsEnabledFlag() ? (1<<sps->getBitDepth(CHANNEL_TYPE_CHROMA))/2 : 128;
          for ( Int j=1 ; j<numValidComp ; j++ )
          {
            Int iDeltaWeight;
            READ_SVLC( iDeltaWeight, iNumRef==0?"delta_chroma_weight_l0[i]":"delta_chroma_weight_l1[i]" );
            assert( iDeltaWeight >= -128 );
            assert( iDeltaWeight <=  127 );
            wp[j].iWeight = (iDeltaWeight + (1<<wp[j].uiLog2WeightDenom));

            Int iDeltaChroma;
            READ_SVLC( iDeltaChroma, iNumRef==0?"delta_chroma_offset_l0[i]":"delta_chroma_offset_l1[i]" );
            assert( iDeltaChroma >= -4*range);
            assert( iDeltaChroma <   4*range);
            Int pred = ( range - ( ( range*wp[j].iWeight)>>(wp[j].uiLog2WeightDenom) ) );
            wp[j].iOffset = Clip3(-range, range-1, (iDeltaChroma + pred) );
          }
        }
        else
        {
          for ( Int j=1 ; j<numValidComp ; j++ )
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
    for(listId = 0; listId <  SCALING_LIST_NUM; listId++)
    {
      if ((sizeId==SCALING_LIST_32x32) && (listId%(SCALING_LIST_NUM/NUMBER_OF_PREDICTION_MODES) != 0))
      {
        Int *src = scalingList->getScalingListAddress(sizeId, listId);
        const Int size = min(MAX_MATRIX_COEF_NUM,(Int)g_scalingListSize[sizeId]);
        const Int *srcNextSmallerSize = scalingList->getScalingListAddress(sizeId-1, listId);
        for(Int i=0; i<size; i++)
        {
          src[i] = srcNextSmallerSize[i];
        }
        scalingList->setScalingListDC(sizeId,listId,(sizeId > SCALING_LIST_8x8) ? scalingList->getScalingListDC(sizeId-1, listId) : src[0]);
      }
      else
      {
        READ_FLAG( code, "scaling_list_pred_mode_flag");
        scalingListPredModeFlag = (code) ? true : false;
        scalingList->setScalingListPredModeFlag(sizeId, listId, scalingListPredModeFlag);
        if(!scalingListPredModeFlag) //Copy Mode
        {
          READ_UVLC( code, "scaling_list_pred_matrix_id_delta");

          if (sizeId==SCALING_LIST_32x32)
          {
            code*=(SCALING_LIST_NUM/NUMBER_OF_PREDICTION_MODES); // Adjust the decoded code for this size, to cope with the missing 32x32 chroma entries.
          }

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
  UInt* scan  = g_scanOrder[SCAN_UNGROUPED][SCAN_DIAG][sizeId==0 ? 2 : 3][sizeId==0 ? 2 : 3];
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

Void TDecCavlc::parseExplicitRdpcmMode( TComTU& /*rTu*/, ComponentID /*compID*/ )
{
  assert(0);
}
//! \}

