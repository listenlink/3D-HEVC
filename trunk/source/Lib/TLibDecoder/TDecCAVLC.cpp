/* The copyright in this software is being made available under the BSD
 * License, included below. This software may be subject to other third party
 * and contributor rights, including patent rights, and no such rights are
 * granted under this license.  
 *
 * Copyright (c) 2010-2012, ITU/ISO/IEC
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

#define READ_CODE(length, code, name)     xReadCodeTr ( length, code, name )
#define READ_UVLC(        code, name)     xReadUvlcTr (         code, name )
#define READ_SVLC(        code, name)     xReadSvlcTr (         code, name )
#define READ_FLAG(        code, name)     xReadFlagTr (         code, name )

Void  xTraceSPSHeader (TComSPS *pSPS)
{
  fprintf( g_hTrace, "=========== Sequence Parameter Set ID: %d ===========\n", pSPS->getSPSId() );
}

Void  xTracePPSHeader (TComPPS *pPPS)
{
  fprintf( g_hTrace, "=========== Picture Parameter Set ID: %d ===========\n", pPPS->getPPSId() );
}

Void  xTraceAPSHeader (TComAPS *pAPS)
{
  fprintf( g_hTrace, "=========== Adaptation Parameter Set ===========\n");
}

Void  xTraceSliceHeader (TComSlice *pSlice)
{
  fprintf( g_hTrace, "=========== Slice ===========\n");
}


Void  TDecCavlc::xReadCodeTr           (UInt length, UInt& rValue, const Char *pSymbolName)
{
  xReadCode (length, rValue);
  fprintf( g_hTrace, "%8lld  ", g_nSymbolCounter++ );
  fprintf( g_hTrace, "%-40s u(%d) : %d\n", pSymbolName, length, rValue ); 
  fflush ( g_hTrace );
}

Void  TDecCavlc::xReadUvlcTr           (UInt& rValue, const Char *pSymbolName)
{
  xReadUvlc (rValue);
  fprintf( g_hTrace, "%8lld  ", g_nSymbolCounter++ );
  fprintf( g_hTrace, "%-40s u(v) : %d\n", pSymbolName, rValue ); 
  fflush ( g_hTrace );
}

Void  TDecCavlc::xReadSvlcTr           (Int& rValue, const Char *pSymbolName)
{
  xReadSvlc(rValue);
  fprintf( g_hTrace, "%8lld  ", g_nSymbolCounter++ );
  fprintf( g_hTrace, "%-40s s(v) : %d\n", pSymbolName, rValue ); 
  fflush ( g_hTrace );
}

Void  TDecCavlc::xReadFlagTr           (UInt& rValue, const Char *pSymbolName)
{
  xReadFlag(rValue);
  fprintf( g_hTrace, "%8lld  ", g_nSymbolCounter++ );
  fprintf( g_hTrace, "%-40s u(1) : %d\n", pSymbolName, rValue ); 
  fflush ( g_hTrace );
}

#else

#define READ_CODE(length, code, name)     xReadCode ( length, code )
#define READ_UVLC(        code, name)     xReadUvlc (         code )
#define READ_SVLC(        code, name)     xReadSvlc (         code )
#define READ_FLAG(        code, name)     xReadFlag (         code )

#endif



// ====================================================================================================================
// Constructor / destructor / create / destroy
// ====================================================================================================================

TDecCavlc::TDecCavlc()
{
  m_iSliceGranularity = 0;

  m_aaiTempScale            = new Int* [ MAX_VIEW_NUM ];
  m_aaiTempOffset           = new Int* [ MAX_VIEW_NUM ];
  m_aaiTempPdmScaleNomDelta = new Int* [ MAX_VIEW_NUM ];
  m_aaiTempPdmOffset        = new Int* [ MAX_VIEW_NUM ];
  for( UInt uiVId = 0; uiVId < MAX_VIEW_NUM; uiVId++ )
  {
    m_aaiTempScale            [ uiVId ] = new Int [ MAX_VIEW_NUM ];
    m_aaiTempOffset           [ uiVId ] = new Int [ MAX_VIEW_NUM ];
    m_aaiTempPdmScaleNomDelta [ uiVId ] = new Int [ MAX_VIEW_NUM ];
    m_aaiTempPdmOffset        [ uiVId ] = new Int [ MAX_VIEW_NUM ];
  }
}

TDecCavlc::~TDecCavlc()
{
  for( UInt uiVId = 0; uiVId < MAX_VIEW_NUM; uiVId++ )
  {
    delete [] m_aaiTempScale            [ uiVId ];
    delete [] m_aaiTempOffset           [ uiVId ];
    delete [] m_aaiTempPdmScaleNomDelta [ uiVId ];
    delete [] m_aaiTempPdmOffset        [ uiVId ];
  }
  delete [] m_aaiTempScale;
  delete [] m_aaiTempOffset;
  delete [] m_aaiTempPdmScaleNomDelta;
  delete [] m_aaiTempPdmOffset;
}

// ====================================================================================================================
// Public member functions
// ====================================================================================================================

/**
 * unmarshal a sequence of SEI messages from bitstream.
 */
void TDecCavlc::parseSEI(SEImessages& seis)
{
  assert(!m_pcBitstream->getNumBitsUntilByteAligned());
  do
  {
    parseSEImessage(*m_pcBitstream, seis);
    /* SEI messages are an integer number of bytes, something has failed
     * in the parsing if bitstream not byte-aligned */
    assert(!m_pcBitstream->getNumBitsUntilByteAligned());
  } while (0x80 != m_pcBitstream->peekBits(8));
  assert(m_pcBitstream->getNumBitsLeft() == 8); /* rsbp_trailing_bits */
}
#if RPS_IN_SPS
void TDecCavlc::parseShortTermRefPicSet( TComSPS* sps, TComReferencePictureSet* rps, Int idx )
#else
void TDecCavlc::parseShortTermRefPicSet( TComPPS* pcPPS, TComReferencePictureSet* rps, Int idx )
#endif
{
  UInt code;
  UInt interRPSPred;
  READ_FLAG(interRPSPred, "inter_ref_pic_set_prediction_flag");  rps->setInterRPSPrediction(interRPSPred);
  if (interRPSPred) 
  {
    UInt bit;
    READ_UVLC(code, "delta_idx_minus1" ); // delta index of the Reference Picture Set used for prediction minus 1
    Int rIdx =  idx - 1 - code;
    assert (rIdx <= idx && rIdx >= 0);
#if RPS_IN_SPS
    TComReferencePictureSet*   rpsRef = sps->getRPSList()->getReferencePictureSet(rIdx);
#else
    TComReferencePictureSet*   rpsRef = pcPPS->getRPSList()->getReferencePictureSet(rIdx);
#endif
    Int k = 0, k0 = 0, k1 = 0;
    READ_CODE(1, bit, "delta_rps_sign"); // delta_RPS_sign
    READ_UVLC(code, "abs_delta_rps_minus1");  // absolute delta RPS minus 1
    Int deltaRPS = (1 - (bit<<1)) * (code + 1); // delta_RPS
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

        if (deltaPOC < 0) {
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

Void TDecCavlc::parseAPS(TComAPS* aps)
{
#if ENC_DEC_TRACE  
  xTraceAPSHeader(aps);
#endif

  UInt uiCode;
  READ_UVLC(uiCode, "aps_id");                             aps->setAPSID(uiCode);
  READ_FLAG(uiCode, "aps_scaling_list_data_present_flag"); aps->setScalingListEnabled( (uiCode==1)?true:false );
  READ_FLAG(uiCode, "aps_deblocking_filter_flag");         aps->setLoopFilterOffsetInAPS( (uiCode==1)?true:false );
#if !SAO_UNIT_INTERLEAVING
  READ_FLAG(uiCode, "aps_sample_adaptive_offset_flag");    aps->setSaoEnabled( (uiCode==1)?true:false );
#endif
#if !LCU_SYNTAX_ALF
  READ_FLAG(uiCode, "aps_adaptive_loop_filter_flag");      aps->setAlfEnabled( (uiCode==1)?true:false );
#endif
  if(aps->getScalingListEnabled())
  {
    parseScalingList( aps->getScalingList() );
  }
#if DBL_CONTROL
  if(aps->getLoopFilterOffsetInAPS())
  {
    xParseDblParam( aps );    
  }
#endif
#if SAO_UNIT_INTERLEAVING
  READ_FLAG(uiCode, "aps_sao_interleaving_flag");      aps->setSaoInterleavingFlag( (uiCode==1)?true:false );
  if(!aps->getSaoInterleavingFlag())
  {
    READ_FLAG(uiCode, "aps_sample_adaptive_offset_flag");      aps->setSaoEnabled( (uiCode==1)?true:false );
#endif
  if(aps->getSaoEnabled())
  {
    aps->getSaoParam()->bSaoFlag[0] = true;
    xParseSaoParam( aps->getSaoParam() );
  }
#if SAO_UNIT_INTERLEAVING
  }
#endif
#if LCU_SYNTAX_ALF
  READ_FLAG(uiCode, "aps_adaptive_loop_filter_flag");      aps->setAlfEnabled( (uiCode==1)?true:false );
#endif
  if(aps->getAlfEnabled())
  {
#if !LCU_SYNTAX_ALF
    aps->getAlfParam()->alf_flag = 1;
#endif
    xParseAlfParam( aps->getAlfParam());
  }
  READ_FLAG( uiCode, "aps_extension_flag");
  if (uiCode)
  {
    while ( xMoreRbspData() )
    {
      READ_FLAG( uiCode, "aps_extension_data_flag");
    }
  }

}

#if DBL_CONTROL
Void  TDecCavlc::xParseDblParam       ( TComAPS* aps )
{
  UInt uiSymbol;
  Int iSymbol;

  parseDFFlag(uiSymbol, "loop_filter_disable");
  aps->setLoopFilterDisable(uiSymbol?true:false);

  if (!aps->getLoopFilterDisable())
  {
    parseDFSvlc(iSymbol, "beta_offset_div2");
    aps->setLoopFilterBetaOffset(iSymbol);
    parseDFSvlc(iSymbol, "tc_offset_div2");
    aps->setLoopFilterTcOffset(iSymbol);
  }
}
#endif
/** parse SAO parameters
 * \param pSaoParam
 */
Void TDecCavlc::xParseSaoParam(SAOParam* pSaoParam)
{
  UInt uiSymbol;

#if SAO_UNIT_INTERLEAVING
  int i,j, compIdx; 
  int numCuInWidth; 
  int numCuInHeight; 
  Bool repeatedRow[3];
  if (pSaoParam->bSaoFlag[0])                                                                    
  {     
    READ_FLAG (uiSymbol, "sao_cb_enable_flag");            pSaoParam->bSaoFlag[1]   = uiSymbol? true:false;  
    READ_FLAG (uiSymbol, "sao_cr_enable_flag");            pSaoParam->bSaoFlag[2]   = uiSymbol? true:false;  
    READ_UVLC (uiSymbol, "sao_num_lcu_in_width_minus1");   pSaoParam->numCuInWidth  = uiSymbol + 1;                          
    READ_UVLC (uiSymbol, "sao_num_lcu_in_height_minus1");  pSaoParam->numCuInHeight = uiSymbol + 1;                          
    numCuInWidth  = pSaoParam->numCuInWidth;
    numCuInHeight = pSaoParam->numCuInHeight;

    READ_FLAG (uiSymbol, "sao_one_luma_unit_flag");  pSaoParam->oneUnitFlag[0] = uiSymbol? true:false;  
    if (pSaoParam->oneUnitFlag[0] )
      xParseSaoOffset(&(pSaoParam->saoLcuParam[0][0]));

    if (pSaoParam->bSaoFlag[1])
    {
      READ_FLAG (uiSymbol, "sao_one_cb_unit_flag");  pSaoParam->oneUnitFlag[1] = uiSymbol? true:false;  
      if (pSaoParam->oneUnitFlag[1] )
        xParseSaoOffset(&(pSaoParam->saoLcuParam[1][0]));
    }
    if (pSaoParam->bSaoFlag[2])
    {
      READ_FLAG (uiSymbol, "sao_one_cr_unit_flag");  pSaoParam->oneUnitFlag[2] = uiSymbol? true:false;  
      if (pSaoParam->oneUnitFlag[2] )
        xParseSaoOffset(&(pSaoParam->saoLcuParam[2][0]));
    }
    for (j=0;j<numCuInHeight;j++)
    {
      for (compIdx=0;compIdx<3;compIdx++)
      {
        repeatedRow[compIdx] = 0;
      }
      for (i=0;i<numCuInWidth;i++)
      {
        for (compIdx=0; compIdx<3; compIdx++)
        {
          if (pSaoParam->bSaoFlag[compIdx]  && !pSaoParam->oneUnitFlag[compIdx]) 
          {
            if (j>0 && i==0) 
            {
              READ_FLAG (uiSymbol, "sao_repeat_row_flag");  repeatedRow[compIdx] = uiSymbol? true:false; 
            }
            xParseSaoUnit (i,j, compIdx, pSaoParam, repeatedRow[compIdx]);
          }
        }
      }
    }
  }
#else
  if (pSaoParam->bSaoFlag[0])
  {
    xParseSaoSplitParam (pSaoParam, 0, 0);
    xParseSaoOffsetParam(pSaoParam, 0, 0);
    READ_FLAG (uiSymbol, "sao_flag_cb");
    pSaoParam->bSaoFlag[1] = uiSymbol? true:false;
    if (pSaoParam->bSaoFlag[1])
    {
      xParseSaoSplitParam (pSaoParam, 0, 1);
      xParseSaoOffsetParam(pSaoParam, 0, 1);
    }

    READ_FLAG (uiSymbol, "sao_flag_cr");
    pSaoParam->bSaoFlag[2] = uiSymbol? true:false;
    if (pSaoParam->bSaoFlag[2])
    {
      xParseSaoSplitParam (pSaoParam, 0, 2);
      xParseSaoOffsetParam(pSaoParam, 0, 2);
    }
  }
#endif
}
#if SAO_UNIT_INTERLEAVING
/** copy SAO parameter
 * \param dst  
 * \param src 
 */
inline Void copySaoOneLcuParam(SaoLcuParam* dst,  SaoLcuParam* src)
{
  Int i;
  dst->partIdx = src->partIdx;
  dst->typeIdx = src->typeIdx;
  if (dst->typeIdx != -1)
  {
    if (dst->typeIdx == SAO_BO)
    {
      dst->bandPosition = src->bandPosition ;
    }
    else
    {
      dst->bandPosition = 0;
    }
    dst->length  = src->length;
    for (i=0;i<dst->length;i++)
    {
      dst->offset[i] = src->offset[i];
    }
  }
  else
  {
    dst->length  = 0;
    for (i=0;i<SAO_BO_LEN;i++)
    {
      dst->offset[i] = 0;
    }
  }
}
/** parse SAO offset
 * \param saoLcuParam SAO LCU parameters
 */
Void TDecCavlc::xParseSaoOffset(SaoLcuParam* saoLcuParam)
{
  UInt uiSymbol;
  Int iSymbol;
  static Int typeLength[MAX_NUM_SAO_TYPE] = {
    SAO_EO_LEN,
    SAO_EO_LEN,
    SAO_EO_LEN,
    SAO_EO_LEN,
    SAO_BO_LEN
  }; 

  READ_UVLC (uiSymbol, "sao_type_idx");   saoLcuParam->typeIdx = (Int)uiSymbol - 1;       
  if (uiSymbol)
  {
    saoLcuParam->length = typeLength[saoLcuParam->typeIdx];
    if( saoLcuParam->typeIdx == SAO_BO )
    {
      READ_CODE( 5, uiSymbol, "sao_band_position"); saoLcuParam->bandPosition = uiSymbol; 
      for(Int i=0; i< saoLcuParam->length; i++)
      {
        READ_SVLC (iSymbol, "sao_offset");    saoLcuParam->offset[i] = iSymbol;   
      }   
    }
    else if( saoLcuParam->typeIdx < 4 )
    {
      READ_UVLC (uiSymbol, "sao_offset");  saoLcuParam->offset[0] = uiSymbol;
      READ_UVLC (uiSymbol, "sao_offset");  saoLcuParam->offset[1] = uiSymbol;
      READ_UVLC (uiSymbol, "sao_offset");  saoLcuParam->offset[2] = -(Int)uiSymbol;
      READ_UVLC (uiSymbol, "sao_offset");  saoLcuParam->offset[3] = -(Int)uiSymbol;
    }
  }
  else
  {
    saoLcuParam->length = 0;
  }
}

/** parse SAO unit
 * \param rx x-axis location
 * \param ry y-axis location
 * \param compIdx color component index
 * \param saoParam SAO parameters
 * \param repeatedRow repeat row flag
 */
void TDecCavlc::xParseSaoUnit(Int rx, Int ry, Int compIdx, SAOParam* saoParam, Bool& repeatedRow )
{
  int addr, addrUp, addrLeft; 
  int numCuInWidth  = saoParam->numCuInWidth;
  SaoLcuParam* saoOneLcu;
  SaoLcuParam* saoOneLcuUp;
  SaoLcuParam* saoOneLcuLeft;
  UInt uiSymbol;
  Int  iSymbol;
  Int  runLeft;
  UInt maxValue;

  addr      =  rx + ry*numCuInWidth;
  addrLeft  =  (addr%numCuInWidth == 0) ? -1 : addr - 1;
  addrUp    =  (addr<numCuInWidth)      ? -1 : addr - numCuInWidth;

  saoOneLcu = &(saoParam->saoLcuParam[compIdx][addr]);      
  if (!repeatedRow)
  {
    runLeft = (addrLeft>=0 )  ? saoParam->saoLcuParam[compIdx][addrLeft].run : -1;
    if (rx == 0 || runLeft==0)
    {
      saoOneLcu->mergeLeftFlag = 0;
      if (ry == 0)
      {
        maxValue = numCuInWidth-rx-1;
        UInt length = 0;
        UInt val = 0;
        if (maxValue)
        {
          for(UInt i=0; i<32; i++)
          {
            if(maxValue&0x1)
            {
              length = i+1;
            }
            maxValue = (maxValue >> 1);
          }
          if(length)
          {
            READ_CODE(length, val, "sao_run_diff");
          }
        }
        uiSymbol = val;
        saoOneLcu->runDiff = uiSymbol; 
        xParseSaoOffset(saoOneLcu);
        saoOneLcu->run = saoOneLcu->runDiff;
      }
      else 
      {
        saoOneLcuUp = &(saoParam->saoLcuParam[compIdx][addrUp]);
        READ_SVLC (iSymbol , "sao_run_diff"     );  saoOneLcu->runDiff = iSymbol; 
        READ_FLAG (uiSymbol, "sao_merge_up_flag");  saoOneLcu->mergeUpFlag   = uiSymbol? true:false;
        if (!saoOneLcu->mergeUpFlag)
        {
          xParseSaoOffset(saoOneLcu);
        }
        else
        {
          saoOneLcuUp = &(saoParam->saoLcuParam[compIdx][addrUp]);
          copySaoOneLcuParam(saoOneLcu, saoOneLcuUp);
        }
        saoOneLcu->run = saoOneLcu->runDiff + saoOneLcuUp->run;
      }
    }
    else
    {
      saoOneLcuLeft = &(saoParam->saoLcuParam[compIdx][addrLeft]);
      copySaoOneLcuParam(saoOneLcu, saoOneLcuLeft);
      saoOneLcu->mergeLeftFlag = 1;
      saoOneLcu->run = saoOneLcuLeft->run-1;
    }
  }
  else
  {
    if (ry > 0)
    {
      saoOneLcuUp = &(saoParam->saoLcuParam[compIdx][addrUp]);
      copySaoOneLcuParam(saoOneLcu, saoOneLcuUp);
      saoOneLcu->mergeLeftFlag = 0;
      saoOneLcu->run = saoOneLcuUp->run;
    }
  }
}

#else
/** Decode quadtree split flag
 * \param  pSaoParam, iPartIdx
 */
Void TDecCavlc::xParseSaoSplitParam(SAOParam* pSaoParam, Int iPartIdx, Int iYCbCr)
{
  UInt uiSymbol;
  SAOQTPart*  pSaoPart = NULL;
  pSaoPart= &(pSaoParam->psSaoPart[iYCbCr][iPartIdx]);

  if(pSaoPart->PartLevel < pSaoParam->iMaxSplitLevel)
  {
    READ_FLAG (uiSymbol, "sao_split_flag");
    pSaoPart->bSplit = uiSymbol? true:false; 
    if(pSaoPart->bSplit)
    {
      for (Int i=0;i<NUM_DOWN_PART;i++)
      {
        xParseSaoSplitParam(pSaoParam, pSaoPart->DownPartsIdx[i], iYCbCr);
      }
    }
  }
  else
  {
    pSaoPart->bSplit = false; 
  }
}

/** Decode SAO for one partition
 * \param  pSaoParam, iPartIdx
 */
Void TDecCavlc::xParseSaoOffsetParam(SAOParam* pSaoParam, Int iPartIdx, Int iYCbCr)
{
  UInt uiSymbol;
  Int iSymbol;  
  SAOQTPart*  pSaoPart = NULL;
  pSaoPart = &(pSaoParam->psSaoPart[iYCbCr][iPartIdx]);

  static Int iTypeLength[MAX_NUM_SAO_TYPE] = {
    SAO_EO_LEN,
    SAO_EO_LEN,
    SAO_EO_LEN,
    SAO_EO_LEN,
    SAO_BO_LEN,
    SAO_BO_LEN
  };  
  if(!pSaoPart->bSplit)
  {
    READ_UVLC (uiSymbol, "sao_type_idx");
    if (uiSymbol)
    {
      pSaoPart->iBestType = uiSymbol-1;
      pSaoPart->bEnableFlag = true;
    }
    else
    {
      pSaoPart->iBestType = -1;
      pSaoPart->bEnableFlag = false;
    }

    if (pSaoPart->bEnableFlag)
    {
      pSaoPart->iLength = iTypeLength[pSaoPart->iBestType];
      for(Int i=0; i< pSaoPart->iLength; i++)
      {
        READ_SVLC (iSymbol, "sao_offset");
        pSaoPart->iOffset[i] = iSymbol;
      }
    }
    return;
  }

  //split
  if (pSaoPart->PartLevel < pSaoParam->iMaxSplitLevel)
  {
    for(Int i=0;i<NUM_DOWN_PART;i++)
    {
      xParseSaoOffsetParam(pSaoParam, pSaoPart->DownPartsIdx[i], iYCbCr);
    }
  }
}
#endif

#if LCU_SYNTAX_ALF 
Void TDecCavlc::xParseAlfParam(AlfParamSet* pAlfParamSet, Bool bSentInAPS, Int firstLCUAddr, Bool acrossSlice, Int numLCUInWidth, Int numLCUInHeight)
{
  Int  numLCU;
  UInt uiSymbol;
  Bool isEnabled[NUM_ALF_COMPONENT];
  Bool isUniParam[NUM_ALF_COMPONENT];
  
  isEnabled[ALF_Y] = true;
  READ_FLAG(uiSymbol, "alf_cb_enable_flag");  isEnabled[ALF_Cb] = ((uiSymbol ==1)?true:false);
  READ_FLAG(uiSymbol, "alf_cr_enable_flag");  isEnabled[ALF_Cr] = ((uiSymbol ==1)?true:false);
  READ_FLAG(uiSymbol, "alf_one_luma_unit_per_slice_flag");   isUniParam[ALF_Y] = ((uiSymbol ==1)?true:false);
  
  isUniParam[ALF_Cb] = true;
  if (isEnabled[ALF_Cb])
  {
    READ_FLAG(uiSymbol, "alf_one_cb_unit_per_slice_flag");   isUniParam[ALF_Cb] = ((uiSymbol ==1)?true:false);
  }
  
  isUniParam[ALF_Cr] = true;
  if (isEnabled[ALF_Cr])
  {
    READ_FLAG(uiSymbol, "alf_one_cr_unit_per_slice_flag");   isUniParam[ALF_Cr] = ((uiSymbol ==1)?true:false);
  }
  
  if(bSentInAPS)
  {
    READ_UVLC(uiSymbol, "alf_num_lcu_in_width_minus1");  numLCUInWidth = uiSymbol+1;
    READ_UVLC(uiSymbol, "alf_num_lcu_in_height_minus1");  numLCUInHeight = uiSymbol+1;
    numLCU = numLCUInWidth*numLCUInHeight;
  }
  else //sent in slice header
  {
    READ_UVLC(uiSymbol, "alf_num_lcu_in_slice_minus1");  numLCU = uiSymbol+1;
  }
  
  assert(pAlfParamSet != NULL);
  
  pAlfParamSet->create(numLCUInWidth, numLCUInHeight, numLCU);
  for(Int compIdx = 0; compIdx < NUM_ALF_COMPONENT; compIdx++)
  {
    pAlfParamSet->isEnabled[compIdx] = isEnabled[compIdx];
    pAlfParamSet->isUniParam[compIdx]= isUniParam[compIdx];
  }
  
  parseAlfParamSet(pAlfParamSet, firstLCUAddr, acrossSlice);
}


Void TDecCavlc::parseAlfParamSet(AlfParamSet* pAlfParamSet, Int firstLCUAddr, Bool alfAcrossSlice)
{
  Int numLCUInWidth = pAlfParamSet->numLCUInWidth;
  Int numLCU        = pAlfParamSet->numLCU;
  
  static Bool isRepeatedRow   [NUM_ALF_COMPONENT];
  static Int  numStoredFilters[NUM_ALF_COMPONENT];
  static Int* run             [NUM_ALF_COMPONENT];
  
  for(Int compIdx =0; compIdx < NUM_ALF_COMPONENT; compIdx++)
  {
    isRepeatedRow[compIdx]    = false;
    numStoredFilters[compIdx] = 0;
    
    run[compIdx] = new Int[numLCU+1];
    run[compIdx][0] = -1; 
  }
  
  UInt uiSymbol;
  Int  iSymbol, ry, rx, addrUp;
  
  for(Int i=0; i< numLCU; i++)
  {
    rx    = (i+ firstLCUAddr)% numLCUInWidth;
    ry    = (i+ firstLCUAddr)/ numLCUInWidth;
    
    for(Int compIdx =0; compIdx < NUM_ALF_COMPONENT; compIdx++)
    {
      AlfUnitParam& alfUnitParam = pAlfParamSet->alfUnitParam[compIdx][i];
      
      if(pAlfParamSet->isEnabled[compIdx])
      {
        if(!pAlfParamSet->isUniParam[compIdx])
        {
          addrUp = i-numLCUInWidth;
          if(rx ==0 && addrUp >=0)
          {
            READ_FLAG(uiSymbol, "alf_repeat_row _flag"); isRepeatedRow[compIdx] = ((uiSymbol ==1)?true:false);
          }
          
          if(isRepeatedRow[compIdx])
          {
            alfUnitParam.mergeType = ALF_MERGE_UP;
            assert(addrUp >=0);
            run[compIdx][i] = run[compIdx][addrUp];
          }
          else
          {
            if(rx == 0 || run[compIdx][i] < 0)
            {             
              if(addrUp < 0)
              {
                //alf_run_diff u(v)
                parseAlfFixedLengthRun(uiSymbol, rx, numLCUInWidth);
                run[compIdx][i] = uiSymbol;
              }
              else
              {
                //alf_run_diff s(v)
                READ_SVLC(iSymbol, "alf_run_diff");
                run[compIdx][i] = run[compIdx][addrUp] + iSymbol;
                assert(run[compIdx][i] >= 0);
              }
              
              if(ry > 0 && (addrUp >=0 || alfAcrossSlice))
              {
                //alf_merge_up_flag
                READ_FLAG(uiSymbol, "alf_merge_up_flag");  alfUnitParam.mergeType = ((uiSymbol ==1)?ALF_MERGE_UP:ALF_MERGE_DISABLED);
              }
              else
              {
                alfUnitParam.mergeType = ALF_MERGE_DISABLED;
              }
              
              if(alfUnitParam.mergeType != ALF_MERGE_UP)
              {
                //alf_lcu_enable_flag
                READ_FLAG(uiSymbol, "alf_lcu_enable_flag");  alfUnitParam.isEnabled = ((uiSymbol ==1)?true:false);
                
                if(alfUnitParam.isEnabled)
                {
                  if(numStoredFilters[compIdx] > 0)
                  {
                    //alf_new_filter_set_flag
                    READ_FLAG(uiSymbol, "alf_new_filter_set_flag");  alfUnitParam.isNewFilt = ((uiSymbol ==1)?true:false);
                    
                    if(!alfUnitParam.isNewFilt)
                    {
                      //alf_stored_filter_set_idx
                      parseAlfStoredFilterIdx(uiSymbol, numStoredFilters[compIdx]);
                      
                      alfUnitParam.storedFiltIdx = uiSymbol;
                      
                      assert( alfUnitParam.storedFiltIdx < numStoredFilters[compIdx]);
                    }
                  }
                  else
                  {
                    alfUnitParam.isNewFilt = true;
                  }
                  
                  if(alfUnitParam.isNewFilt)
                  {
                    alfUnitParam.alfFiltParam = new ALFParam(compIdx);
                    xParseAlfParam(alfUnitParam.alfFiltParam);
                    alfUnitParam.alfFiltParam->alf_flag = 1;
                    
                    numStoredFilters[compIdx]++;
                  }
                }
                
              }
            }
            else
            {
              alfUnitParam.mergeType = ALF_MERGE_LEFT;
            }
            
            run[compIdx][i+1] = run[compIdx][i] -1;
          }
          
        }
        else // uni-param
        {
          if(i == 0)
          {
            alfUnitParam.mergeType = ALF_MERGE_DISABLED;
            
            //alf_lcu_enable_flag
            READ_FLAG(uiSymbol, "alf_lcu_enable_flag");  alfUnitParam.isEnabled = ((uiSymbol ==1)?true:false);
            if(alfUnitParam.isEnabled)
            {
              alfUnitParam.isNewFilt = true;
              alfUnitParam.alfFiltParam = new ALFParam(compIdx);
              xParseAlfParam(alfUnitParam.alfFiltParam);
              alfUnitParam.alfFiltParam->alf_flag = 1;
            }
          }
          else
          {
            alfUnitParam.mergeType = ALF_MERGE_FIRST;
          }
          
        }
      }
      else
      {
        alfUnitParam.mergeType = ALF_MERGE_DISABLED;
        alfUnitParam.isEnabled = false;
      }
    }
  }
  
  for(Int compIdx =0; compIdx < NUM_ALF_COMPONENT; compIdx++)
  {
    delete[] run[compIdx];
  }
}


Void TDecCavlc::parseAlfFixedLengthRun( UInt& idx, UInt rx, UInt numLCUInWidth )
{
  assert(numLCUInWidth > rx);
  
  UInt length = 0;  
  UInt maxNumRun = numLCUInWidth - rx - 1; 
  
  for(UInt i=0; i<32; i++)
  {
    if(maxNumRun&0x1)
    {
      length = i+1;
    }
    maxNumRun = (maxNumRun >> 1);
  }
  
  idx = 0;
  if(length)
  {
    READ_CODE( length, idx, "alf_run_diff" );
  }
  else
  {
    idx = 0;
  }
}


Void TDecCavlc::parseAlfStoredFilterIdx( UInt& idx, UInt numFilterSetsInBuffer )
{
  assert(numFilterSetsInBuffer > 0);
  
  UInt length = 0;  
  UInt maxValue = numFilterSetsInBuffer - 1;
  
  for(UInt i=0; i<32; i++)
  {
    if(maxValue&0x1)
    {
      length = i+1;
    }
    maxValue = (maxValue >> 1);
  }
  
  idx = 0;
  if(length)
  {
    READ_CODE( length, idx, "alf_stored_filter_set_idx" );
  }
  else
  {
    idx = 0;
  }
}

#endif

Void TDecCavlc::xParseAlfParam(ALFParam* pAlfParam)
{
  UInt uiSymbol;
  Int iSymbol;
#if ALF_SINGLE_FILTER_SHAPE
  Int sqrFiltLengthTab[NUM_ALF_FILTER_SHAPE] = {ALF_FILTER_LEN}; 
#else
  Int sqrFiltLengthTab[2] = { 9, 9}; 
#endif

#if LCU_SYNTAX_ALF
  switch(pAlfParam->componentID)
  {
  case ALF_Cb:
  case ALF_Cr:
    {
      pAlfParam->filter_shape = ALF_CROSS9x7_SQUARE3x3;
      pAlfParam->num_coeff = sqrFiltLengthTab[pAlfParam->filter_shape];
      pAlfParam->filters_per_group = 1;
      for(Int pos=0; pos< pAlfParam->num_coeff; pos++)
      {
        READ_SVLC(iSymbol, "alf_filt_coeff");
        pAlfParam->coeffmulti[0][pos] = iSymbol;
      }
    }
    break;
  case ALF_Y:
    {
#endif
  pAlfParam->filters_per_group = 0;
  memset (pAlfParam->filterPattern, 0 , sizeof(Int)*NO_VAR_BINS);
#if !LCU_SYNTAX_ALF
  READ_FLAG (uiSymbol, "alf_region_adaptation_flag");
  pAlfParam->alf_pcr_region_flag = uiSymbol;  
#endif
#if ALF_SINGLE_FILTER_SHAPE  
  pAlfParam->filter_shape = 0;
#else
  READ_UVLC (uiSymbol, "alf_length_luma_minus_5_div2");
  pAlfParam->filter_shape = uiSymbol;
#endif
  pAlfParam->num_coeff = sqrFiltLengthTab[pAlfParam->filter_shape];

  // filters_per_fr
  READ_UVLC (uiSymbol, "alf_no_filters_minus1");
  pAlfParam->filters_per_group = uiSymbol + 1;

  if(uiSymbol == 1) // filters_per_group == 2
  {
    READ_UVLC (uiSymbol, "alf_start_second_filter");
    pAlfParam->startSecondFilter = uiSymbol;
    pAlfParam->filterPattern [uiSymbol] = 1;
  }
  else if (uiSymbol > 1) // filters_per_group > 2
  {
    pAlfParam->filters_per_group = 1;
#if LCU_SYNTAX_ALF
#if ALF_16_BA_GROUPS
    Int numMergeFlags = 16;
#else
    Int numMergeFlags = 15;
#endif
#else
#if ALF_16_BA_GROUPS
    Int numMergeFlags = 16;
#else
    Int numMergeFlags = pAlfParam->alf_pcr_region_flag ? 16 : 15;
#endif
#endif
    for (Int i=1; i<numMergeFlags; i++) 
    {
      READ_FLAG (uiSymbol,  "alf_filter_pattern");
      pAlfParam->filterPattern[i] = uiSymbol;
      pAlfParam->filters_per_group += uiSymbol;
    }
  }

  if (pAlfParam->filters_per_group > 1)
  {
    READ_FLAG (uiSymbol, "alf_pred_method");
    pAlfParam->predMethod = uiSymbol;
  }
  for(Int idx = 0; idx < pAlfParam->filters_per_group; ++idx)
  {
    READ_FLAG (uiSymbol,"alf_nb_pred_luma");
    pAlfParam->nbSPred[idx] = uiSymbol;
  }

#if ALF_SINGLE_FILTER_SHAPE
  Int minScanVal = MIN_SCAN_POS_CROSS;
#else
  Int minScanVal = (pAlfParam->filter_shape == ALF_STAR5x5) ? 0: MIN_SCAN_POS_CROSS;
#endif

  // Determine maxScanVal
  Int maxScanVal = 0;
  Int *pDepthInt = pDepthIntTabShapes[pAlfParam->filter_shape];
  for(Int idx = 0; idx < pAlfParam->num_coeff; idx++)
  {
    maxScanVal = max(maxScanVal, pDepthInt[idx]);
  }

  // Golomb parameters
#if LCU_SYNTAX_ALF
  if( pAlfParam->filters_per_group > 1 )
  {
#endif
  READ_UVLC (uiSymbol, "alf_min_kstart_minus1");
  pAlfParam->minKStart = 1 + uiSymbol;

  Int kMin = pAlfParam->minKStart;

  for(Int scanPos = minScanVal; scanPos < maxScanVal; scanPos++)
  {
    READ_FLAG (uiSymbol, "alf_golomb_index_bit");
    pAlfParam->kMinTab[scanPos] = kMin + uiSymbol;
    kMin = pAlfParam->kMinTab[scanPos];
  }
#if LCU_SYNTAX_ALF
  }
#endif

  Int scanPos;
  for(Int idx = 0; idx < pAlfParam->filters_per_group; ++idx)
  {
    for(Int i = 0; i < pAlfParam->num_coeff; i++)
    {
      scanPos = pDepthInt[i] - 1;
#if LCU_SYNTAX_ALF
      Int k = (pAlfParam->filters_per_group == 1) ? kTableTabShapes[ALF_CROSS9x7_SQUARE3x3][i] : pAlfParam->kMinTab[scanPos];
      pAlfParam->coeffmulti[idx][i] = xGolombDecode(k);
#else
      pAlfParam->coeffmulti[idx][i] = xGolombDecode(pAlfParam->kMinTab[scanPos]);
#endif
    }
  }
#if LCU_SYNTAX_ALF
    }
    break;
  default:
    {
      printf("Not a legal component ID for ALF\n");
      assert(0);
      exit(-1);
    }
  }
#else
  // filter parameters for chroma
  READ_UVLC (uiSymbol, "alf_chroma_idc");
  pAlfParam->chroma_idc = uiSymbol;

  if(pAlfParam->chroma_idc)
  {

#if ALF_SINGLE_FILTER_SHAPE 
    pAlfParam->filter_shape_chroma  = 0;
#else
    READ_UVLC (uiSymbol, "alf_length_chroma_minus_5_div2");

    pAlfParam->filter_shape_chroma = uiSymbol;
#endif
    pAlfParam->num_coeff_chroma = sqrFiltLengthTab[pAlfParam->filter_shape_chroma];
    // filter coefficients for chroma
    for(Int pos=0; pos<pAlfParam->num_coeff_chroma; pos++)
    {
      READ_SVLC (iSymbol, "alf_coeff_chroma");
      pAlfParam->coeff_chroma[pos] = iSymbol;
    }
  }
#endif
}

Int TDecCavlc::xGolombDecode(Int k)
{
  UInt uiSymbol;
  Int q = -1;
  Int nr = 0;
  Int a;

  uiSymbol = 1;
  while (uiSymbol)
  {
    xReadFlag(uiSymbol);
    q++;
  }
  for(a = 0; a < k; ++a)          // read out the sequential log2(M) bits
  {
    xReadFlag(uiSymbol);
    if(uiSymbol)
      nr += 1 << a;
  }
  nr += q << k;
  if(nr != 0)
  {
    xReadFlag(uiSymbol);
    nr = (uiSymbol)? nr: -nr;
  }
#if ENC_DEC_TRACE
  fprintf( g_hTrace, "%8lld  ", g_nSymbolCounter++ );
  fprintf( g_hTrace, "%-40s ge(v) : %d\n", "alf_coeff_luma", nr ); 
#endif
  return nr;
}

#if TILES_OR_ENTROPY_SYNC_IDC
Void TDecCavlc::parsePPS(TComPPS* pcPPS, ParameterSetManagerDecoder *parameterSet)
#else
Void TDecCavlc::parsePPS(TComPPS* pcPPS)
#endif
{
#if ENC_DEC_TRACE  
  xTracePPSHeader (pcPPS);
#endif
  UInt  uiCode;

  Int   iCode;

#if !RPS_IN_SPS
  TComRPSList* rpsList = pcPPS->getRPSList();
#endif
  READ_UVLC( uiCode, "pic_parameter_set_id");                      pcPPS->setPPSId (uiCode);
  READ_UVLC( uiCode, "seq_parameter_set_id");                      pcPPS->setSPSId (uiCode);

#if MULTIBITS_DATA_HIDING
  READ_FLAG ( uiCode, "sign_data_hiding_flag" ); pcPPS->setSignHideFlag( uiCode );
  if( pcPPS->getSignHideFlag() )
  {
    READ_CODE( 4, uiCode, "sign_hiding_threshold"); pcPPS->setTSIG(uiCode);
  }
#endif

#if CABAC_INIT_FLAG
  READ_FLAG( uiCode,   "cabac_init_present_flag" );            pcPPS->setCabacInitPresentFlag( uiCode ? true : false );
#endif
#if !RPS_IN_SPS
  // RPS is put before entropy_coding_mode_flag
  // since entropy_coding_mode_flag will probably be removed from the WD
  TComReferencePictureSet*      pcRPS;

  READ_UVLC( uiCode, "num_short_term_ref_pic_sets" );
  rpsList->create(uiCode);

  for(Int i=0; i< rpsList->getNumberOfReferencePictureSets(); i++)
  {
    pcRPS = rpsList->getReferencePictureSet(i);
    parseShortTermRefPicSet(pcPPS,pcRPS,i);
  }
  READ_FLAG( uiCode, "long_term_ref_pics_present_flag" );          pcPPS->setLongTermRefsPresent(uiCode);
#endif
  // entropy_coding_mode_flag
  // We code the entropy_coding_mode_flag, it's needed for tests.
  READ_FLAG( uiCode, "entropy_coding_mode_flag" );                 pcPPS->setEntropyCodingMode( uiCode ? true : false );
  if (pcPPS->getEntropyCodingMode())
  {
#if !WPP_SIMPLIFICATION
    READ_UVLC( uiCode, "entropy_coding_synchro" );                 pcPPS->setEntropyCodingSynchro( uiCode );
    READ_FLAG( uiCode, "cabac_istate_reset" );                     pcPPS->setCabacIstateReset( uiCode ? true : false );
#endif
#if !TILES_OR_ENTROPY_SYNC_IDC
#if !WPP_SIMPLIFICATION
    if ( pcPPS->getEntropyCodingSynchro() )
#endif
    {
      READ_UVLC( uiCode, "num_substreams_minus1" );                pcPPS->setNumSubstreams(uiCode+1);
    }
#endif
  }
#if !H0566_TLA
  READ_UVLC( uiCode, "num_temporal_layer_switching_point_flags" ); pcPPS->setNumTLayerSwitchingFlags( uiCode );
  for ( UInt i = 0; i < pcPPS->getNumTLayerSwitchingFlags(); i++ )
  {
    READ_FLAG( uiCode, "temporal_layer_switching_point_flag" );    pcPPS->setTLayerSwitchingFlag( i, uiCode > 0 ? true : false );
  }
#endif
  
  // num_ref_idx_l0_default_active_minus1
  // num_ref_idx_l1_default_active_minus1
  READ_SVLC(iCode, "pic_init_qp_minus26" );                        pcPPS->setPicInitQPMinus26(iCode);
  READ_FLAG( uiCode, "constrained_intra_pred_flag" );              pcPPS->setConstrainedIntraPred( uiCode ? true : false );
  READ_FLAG( uiCode, "enable_temporal_mvp_flag" );                 pcPPS->setEnableTMVPFlag( uiCode ? true : false );
  READ_CODE( 2, uiCode, "slice_granularity" );                     pcPPS->setSliceGranularity(uiCode);

  // alf_param() ?

  READ_UVLC( uiCode, "max_cu_qp_delta_depth");
  if(uiCode == 0)
  {
    pcPPS->setUseDQP (false);
    pcPPS->setMaxCuDQPDepth( 0 );
  }
  else
  {
    pcPPS->setUseDQP (true);
    pcPPS->setMaxCuDQPDepth(uiCode - 1);
  }

  READ_SVLC( iCode, "chroma_qp_offset");
  pcPPS->setChromaQpOffset(iCode);

  READ_SVLC( iCode, "chroma_qp_offset_2nd");
  pcPPS->setChromaQpOffset2nd(iCode);

  READ_FLAG( uiCode, "weighted_pred_flag" );          // Use of Weighting Prediction (P_SLICE)
  pcPPS->setUseWP( uiCode==1 );
  READ_CODE( 2, uiCode, "weighted_bipred_idc" );      // Use of Bi-Directional Weighting Prediction (B_SLICE)
  pcPPS->setWPBiPredIdc( uiCode );
//printf("TDecCavlc::parsePPS():\tm_bUseWeightPred=%d\tm_uiBiPredIdc=%d\n", pcPPS->getUseWP(), pcPPS->getWPBiPredIdc());

#if H0388
  READ_FLAG( uiCode, "output_flag_present_flag" );
  pcPPS->setOutputFlagPresentFlag( uiCode==1 );
#endif

#if TILES_OR_ENTROPY_SYNC_IDC
  if(parameterSet->getPrefetchedSPS(pcPPS->getSPSId())->getTilesOrEntropyCodingSyncIdc()==1)
  {
#endif
    READ_FLAG ( uiCode, "tile_info_present_flag" );
    pcPPS->setColumnRowInfoPresent(uiCode);
    READ_FLAG ( uiCode, "tile_control_present_flag" );
    pcPPS->setTileBehaviorControlPresentFlag(uiCode);
    if( pcPPS->getColumnRowInfoPresent() == 1 )
    {
      READ_UVLC ( uiCode, "num_tile_columns_minus1" );   
      pcPPS->setNumColumnsMinus1( uiCode );  
      READ_UVLC ( uiCode, "num_tile_rows_minus1" );  
      pcPPS->setNumRowsMinus1( uiCode );  
      READ_FLAG ( uiCode, "uniform_spacing_flag" );  
      pcPPS->setUniformSpacingIdr( uiCode );

      if( pcPPS->getUniformSpacingIdr() == 0 )
      {
        UInt* columnWidth = (UInt*)malloc(pcPPS->getNumColumnsMinus1()*sizeof(UInt));
        for(UInt i=0; i<pcPPS->getNumColumnsMinus1(); i++)
        { 
          READ_UVLC( uiCode, "column_width" );  
          columnWidth[i] = uiCode;  
        }
        pcPPS->setColumnWidth(columnWidth);
        free(columnWidth);

        UInt* rowHeight = (UInt*)malloc(pcPPS->getNumRowsMinus1()*sizeof(UInt));
        for(UInt i=0; i<pcPPS->getNumRowsMinus1(); i++)
        {
          READ_UVLC( uiCode, "row_height" );  
          rowHeight[i] = uiCode;  
        }
        pcPPS->setRowHeight(rowHeight);
        free(rowHeight);  
      }
    }


    if(pcPPS->getTileBehaviorControlPresentFlag() == 1)
    {
      Int iNumColTilesMinus1 = (pcPPS->getColumnRowInfoPresent() == 1)?(pcPPS->getNumColumnsMinus1()):(pcPPS->getSPS()->getNumColumnsMinus1());
      Int iNumRowTilesMinus1 = (pcPPS->getColumnRowInfoPresent() == 1)?(pcPPS->getNumColumnsMinus1()):(pcPPS->getSPS()->getNumRowsMinus1());
#if !REMOVE_TILE_DEPENDENCE
      pcPPS->setTileBoundaryIndependenceIdr( 1 ); //default
#endif
      pcPPS->setLFCrossTileBoundaryFlag(true); //default

      if(iNumColTilesMinus1 !=0 || iNumRowTilesMinus1 !=0)
      {
#if !REMOVE_TILE_DEPENDENCE
        READ_FLAG ( uiCode, "tile_boundary_independence_flag" );  
        pcPPS->setTileBoundaryIndependenceIdr( uiCode );

        if(pcPPS->getTileBoundaryIndependenceIdr() == 1)
        {
#endif
          READ_FLAG ( uiCode, "loop_filter_across_tile_flag" );  
          pcPPS->setLFCrossTileBoundaryFlag( (uiCode == 1)?true:false );
#if !REMOVE_TILE_DEPENDENCE
        }
#endif
      }
    }
#if TILES_OR_ENTROPY_SYNC_IDC
  }
  else if(parameterSet->getPrefetchedSPS(pcPPS->getSPSId())->getTilesOrEntropyCodingSyncIdc()==2)
  {
    READ_UVLC( uiCode, "num_substreams_minus1" );                pcPPS->setNumSubstreams(uiCode+1);
  }
#endif

#if DBL_CONTROL
  READ_FLAG( uiCode, "deblocking_filter_control_present_flag" ); 
  pcPPS->setDeblockingFilterControlPresent( uiCode ? true : false);
#endif
#if PARALLEL_MERGE
  READ_UVLC( uiCode, "log2_parallel_merge_level_minus2");
  assert(uiCode == LOG2_PARALLEL_MERGE_LEVEL_MINUS2);
  pcPPS->setLog2ParallelMergeLevelMinus2 (uiCode);
#endif

  READ_FLAG( uiCode, "pps_extension_flag");
  if (uiCode)
  {
    while ( xMoreRbspData() )
    {
      READ_FLAG( uiCode, "pps_extension_data_flag");
    }
  }
}

#if VIDYO_VPS_INTEGRATION
Void TDecCavlc::parseVPS(TComVPS* pcVPS)
{
  UInt  uiCode;
  Int   iCode;
  
  READ_CODE( 3, uiCode, "max_temporal_layers_minus1" );   pcVPS->setMaxTLayers( uiCode + 1 );
  READ_CODE( 5, uiCode, "max_layers_minus1" );            pcVPS->setMaxLayers( uiCode + 1 );
  READ_FLAG( uiCode,  "temporal_id_nesting_flag" );       pcVPS->setTemporalNestingFlag( uiCode ? true:false );
  READ_UVLC( uiCode,  "video_parameter_set_id" );         pcVPS->setVPSId( uiCode );
  for(UInt i = 0; i <= pcVPS->getMaxTLayers()-1; i++)
  {
    READ_UVLC( uiCode,  "max_dec_pic_buffering[i]" );     pcVPS->setMaxDecPicBuffering( uiCode, i );
    READ_UVLC( uiCode,  "num_reorder_pics[i]" );          pcVPS->setNumReorderPics( uiCode, i );
    READ_UVLC( uiCode,  "max_latency_increase[i]" );      pcVPS->setMaxLatencyIncrease( uiCode, i );
  }
  
  READ_CODE( 1, uiCode, "bit_equal_to_one" );             assert( uiCode );
  
  if( pcVPS->getMaxLayers() - 1 > 0 )
  {
    READ_UVLC( uiCode,  "extension_type" );               pcVPS->setExtensionType( uiCode );
    
    pcVPS->setViewOrderIdx( 0, 0 );
    pcVPS->setViewId( 0, 0 );
    pcVPS->setDepthFlag( 0, 0 );
    for(UInt i = 1; i <= pcVPS->getMaxLayers()-1; i++)
    {
      READ_FLAG( uiCode, "dependent_flag[i]" );           pcVPS->setDependentFlag( uiCode ? true:false, i);
      if( pcVPS->getDependentFlag(i) )
      {
        READ_UVLC( uiCode,  "delta_reference_layer_id_minus1[i]" ); pcVPS->setDependentLayer( i - uiCode + 1, i );
        if( pcVPS->getExtensionType() == VPS_EXTENSION_TYPE_MULTI_VIEW )
        {
          READ_UVLC( uiCode,  "view_id[i]" );             pcVPS->setViewId( uiCode, i );
          READ_FLAG( uiCode,  "depth_flag[i]" );          pcVPS->setDepthFlag( uiCode ? true:false, i ); 
          READ_SVLC( iCode,  "view_order_idx[i]" );       pcVPS->setViewOrderIdx( iCode, i );
        }
        
      }
    }
  }
  
  READ_FLAG( uiCode,  "vps_extension_flag" );          assert(!uiCode);
  //future extensions go here..
  
  return;
}

#endif

#if HHI_MPI
Void TDecCavlc::parseSPS(TComSPS* pcSPS, Bool bIsDepth)
#else
Void TDecCavlc::parseSPS(TComSPS* pcSPS)
#endif
{
#if ENC_DEC_TRACE  
  xTraceSPSHeader (pcSPS);
#endif
  
  UInt  uiCode;
  Int   iCode;

  READ_CODE( 8,  uiCode, "profile_idc" );                        pcSPS->setProfileIdc( uiCode );
  READ_CODE( 8,  uiCode, "reserved_zero_8bits" );
  READ_CODE( 8,  uiCode, "level_idc" );                          pcSPS->setLevelIdc( uiCode );
  READ_UVLC(     uiCode, "seq_parameter_set_id" );               pcSPS->setSPSId( uiCode );
#if VIDYO_VPS_INTEGRATION
  READ_UVLC(     uiCode, "video_parameter_set_id" );             pcSPS->setVPSId( uiCode );
#endif
  READ_UVLC(     uiCode, "chroma_format_idc" );                  pcSPS->setChromaFormatIdc( uiCode );
  READ_CODE( 3,  uiCode, "max_temporal_layers_minus1" );         pcSPS->setMaxTLayers( uiCode+1 );
  READ_UVLC (    uiCode, "pic_width_in_luma_samples" );          pcSPS->setPicWidthInLumaSamples ( uiCode    );
  READ_UVLC (    uiCode, "pic_height_in_luma_samples" );         pcSPS->setPicHeightInLumaSamples( uiCode    );
#if PIC_CROPPING
  READ_FLAG(     uiCode, "pic_cropping_flag");                   pcSPS->setPicCroppingFlag ( uiCode ? true : false );
  if (uiCode != 0)
  {
    READ_UVLC(   uiCode, "pic_crop_left_offset" );               pcSPS->setPicCropLeftOffset( uiCode );
    READ_UVLC(   uiCode, "pic_crop_right_offset" );              pcSPS->setPicCropRightOffset( uiCode );
    READ_UVLC(   uiCode, "pic_crop_top_offset" );                pcSPS->setPicCropTopOffset( uiCode );
    READ_UVLC(   uiCode, "pic_crop_bottom_offset" );             pcSPS->setPicCropBottomOffset( uiCode );
  }
#endif

#if FULL_NBIT
  READ_UVLC(     uiCode, "bit_depth_luma_minus8" );
  g_uiBitDepth = 8 + uiCode;
  g_uiBitIncrement = 0;
  pcSPS->setBitDepth(g_uiBitDepth);
  pcSPS->setBitIncrement(g_uiBitIncrement);
#else
  READ_UVLC(     uiCode, "bit_depth_luma_minus8" );
  g_uiBitDepth = 8;
  g_uiBitIncrement = uiCode;
  pcSPS->setBitDepth(g_uiBitDepth);
  pcSPS->setBitIncrement(g_uiBitIncrement);
#endif
  
#if HHI_DMM_WEDGE_INTRA || HHI_DMM_PRED_TEX
  g_iDeltaDCsQuantOffset = g_uiBitIncrement - 2;
#endif

#if H0736_AVC_STYLE_QP_RANGE
  pcSPS->setQpBDOffsetY( (Int) (6*uiCode) );
#endif

  g_uiBASE_MAX  = ((1<<(g_uiBitDepth))-1);
  
#if IBDI_NOCLIP_RANGE
  g_uiIBDI_MAX  = g_uiBASE_MAX << g_uiBitIncrement;
#else
  g_uiIBDI_MAX  = ((1<<(g_uiBitDepth+g_uiBitIncrement))-1);
#endif
  READ_UVLC( uiCode,    "bit_depth_chroma_minus8" );
#if H0736_AVC_STYLE_QP_RANGE
  pcSPS->setQpBDOffsetC( (Int) (6*uiCode) );
#endif

  READ_FLAG( uiCode, "pcm_enabled_flag" ); pcSPS->setUsePCM( uiCode ? true : false );

  if( pcSPS->getUsePCM() )
  {
    READ_CODE( 4, uiCode, "pcm_bit_depth_luma_minus1" );           pcSPS->setPCMBitDepthLuma   ( 1 + uiCode );
    READ_CODE( 4, uiCode, "pcm_bit_depth_chroma_minus1" );         pcSPS->setPCMBitDepthChroma ( 1 + uiCode );
  }

#if LOSSLESS_CODING
  READ_FLAG( uiCode, "qpprime_y_zero_transquant_bypass_flag" );    pcSPS->setUseLossless ( uiCode ? true : false );
#endif

  READ_UVLC( uiCode,    "log2_max_pic_order_cnt_lsb_minus4" );   pcSPS->setBitsForPOC( 4 + uiCode );
#if H0567_DPB_PARAMETERS_PER_TEMPORAL_LAYER
  for(UInt i=0; i <= pcSPS->getMaxTLayers()-1; i++)
  {
    READ_UVLC ( uiCode, "max_dec_pic_buffering");
    pcSPS->setMaxDecPicBuffering( uiCode, i);
    READ_UVLC ( uiCode, "num_reorder_pics" );
    pcSPS->setNumReorderPics(uiCode, i);
    READ_UVLC ( uiCode, "max_latency_increase");
    pcSPS->setMaxLatencyIncrease( uiCode, i );
  }
#else
  READ_UVLC( uiCode,    "max_num_ref_pics" );                    pcSPS->setMaxNumberOfReferencePictures(uiCode);
  READ_UVLC( uiCode,    "num_reorder_frames" );                  pcSPS->setNumReorderFrames(uiCode);
  READ_UVLC ( uiCode, "max_dec_frame_buffering");
  pcSPS->setMaxDecFrameBuffering( uiCode );
  READ_UVLC ( uiCode, "max_latency_increase");
  pcSPS->setMaxLatencyIncrease( uiCode );
#endif

#if H0412_REF_PIC_LIST_RESTRICTION
  READ_FLAG( uiCode, "restricted_ref_pic_lists_flag" );
  pcSPS->setRestrictedRefPicListsFlag( uiCode );
  if( pcSPS->getRestrictedRefPicListsFlag() )
  {
    READ_FLAG( uiCode, "lists_modification_present_flag" );
    pcSPS->setListsModificationPresentFlag(uiCode);
  }
  else 
  {
    pcSPS->setListsModificationPresentFlag(true);
  }
#endif
  READ_UVLC( uiCode, "log2_min_coding_block_size_minus3" );
  UInt log2MinCUSize = uiCode + 3;
  READ_UVLC( uiCode, "log2_diff_max_min_coding_block_size" );
  UInt uiMaxCUDepthCorrect = uiCode;
  pcSPS->setMaxCUWidth  ( 1<<(log2MinCUSize + uiMaxCUDepthCorrect) ); g_uiMaxCUWidth  = 1<<(log2MinCUSize + uiMaxCUDepthCorrect);
  pcSPS->setMaxCUHeight ( 1<<(log2MinCUSize + uiMaxCUDepthCorrect) ); g_uiMaxCUHeight = 1<<(log2MinCUSize + uiMaxCUDepthCorrect);
  READ_UVLC( uiCode, "log2_min_transform_block_size_minus2" );   pcSPS->setQuadtreeTULog2MinSize( uiCode + 2 );

  READ_UVLC( uiCode, "log2_diff_max_min_transform_block_size" ); pcSPS->setQuadtreeTULog2MaxSize( uiCode + pcSPS->getQuadtreeTULog2MinSize() );
  pcSPS->setMaxTrSize( 1<<(uiCode + pcSPS->getQuadtreeTULog2MinSize()) );

  if(log2MinCUSize == 3)
  {
    xReadFlag( uiCode ); pcSPS->setDisInter4x4( uiCode ? true : false );
  }

  if( pcSPS->getUsePCM() )
  {
    READ_UVLC( uiCode, "log2_min_pcm_coding_block_size_minus3" );  pcSPS->setPCMLog2MinSize (uiCode+3); 
    READ_UVLC( uiCode, "log2_diff_max_min_pcm_coding_block_size" ); pcSPS->setPCMLog2MaxSize ( uiCode+pcSPS->getPCMLog2MinSize() );
  }

  READ_UVLC( uiCode, "max_transform_hierarchy_depth_inter" );    pcSPS->setQuadtreeTUMaxDepthInter( uiCode+1 );
  READ_UVLC( uiCode, "max_transform_hierarchy_depth_intra" );    pcSPS->setQuadtreeTUMaxDepthIntra( uiCode+1 );
  g_uiAddCUDepth = 0;
  while( ( pcSPS->getMaxCUWidth() >> uiMaxCUDepthCorrect ) > ( 1 << ( pcSPS->getQuadtreeTULog2MinSize() + g_uiAddCUDepth )  ) )
  {
    g_uiAddCUDepth++;
  }
  pcSPS->setMaxCUDepth( uiMaxCUDepthCorrect+g_uiAddCUDepth  ); 
  g_uiMaxCUDepth  = uiMaxCUDepthCorrect+g_uiAddCUDepth;
  // BB: these parameters may be removed completly and replaced by the fixed values
  pcSPS->setMinTrDepth( 0 );
  pcSPS->setMaxTrDepth( 1 );
  READ_FLAG( uiCode, "scaling_list_enabled_flag" );                 pcSPS->setScalingListFlag ( uiCode );
  READ_FLAG( uiCode, "chroma_pred_from_luma_enabled_flag" );        pcSPS->setUseLMChroma ( uiCode ? true : false ); 
  READ_FLAG( uiCode, "deblocking_filter_in_aps_enabled_flag" );     pcSPS->setUseDF ( uiCode ? true : false );  
  READ_FLAG( uiCode, "loop_filter_across_slice_flag" );             pcSPS->setLFCrossSliceBoundaryFlag( uiCode ? true : false);
  READ_FLAG( uiCode, "asymmetric_motion_partitions_enabled_flag" ); pcSPS->setUseAMP( uiCode );
  READ_FLAG( uiCode, "non_square_quadtree_enabled_flag" );          pcSPS->setUseNSQT( uiCode );
  READ_FLAG( uiCode, "sample_adaptive_offset_enabled_flag" );       pcSPS->setUseSAO ( uiCode ? true : false );  
  READ_FLAG( uiCode, "adaptive_loop_filter_enabled_flag" );         pcSPS->setUseALF ( uiCode ? true : false );
#if LCU_SYNTAX_ALF
  if(pcSPS->getUseALF())
  {
    READ_FLAG( uiCode, "alf_coef_in_slice_flag" );      pcSPS->setUseALFCoefInSlice ( uiCode ? true : false );
  }
#endif
  if( pcSPS->getUsePCM() )
  {
    READ_FLAG( uiCode, "pcm_loop_filter_disable_flag" );           pcSPS->setPCMFilterDisableFlag ( uiCode ? true : false );
  }

  READ_FLAG( uiCode, "temporal_id_nesting_flag" );               pcSPS->setTemporalIdNestingFlag ( uiCode > 0 ? true : false );


#if RPS_IN_SPS
  TComRPSList* rpsList = pcSPS->getRPSList();
  TComReferencePictureSet* rps;

  READ_UVLC( uiCode, "num_short_term_ref_pic_sets" );
  rpsList->create(uiCode);

  for(UInt i=0; i< rpsList->getNumberOfReferencePictureSets(); i++)
  {
    rps = rpsList->getReferencePictureSet(i);
    parseShortTermRefPicSet(pcSPS,rps,i);
  }
  READ_FLAG( uiCode, "long_term_ref_pics_present_flag" );          pcSPS->setLongTermRefsPresent(uiCode);
#endif
#if !PIC_CROPPING
  //!!!KS: Syntax not in WD !!!

  xReadUvlc ( uiCode ); pcSPS->setPadX        ( uiCode    );
  xReadUvlc ( uiCode ); pcSPS->setPadY        ( uiCode    );
#endif
  
  // AMVP mode for each depth (AM_NONE or AM_EXPL)
  for (Int i = 0; i < pcSPS->getMaxCUDepth(); i++)
  {
    xReadFlag( uiCode );
    pcSPS->setAMVPMode( i, (AMVP_MODE)uiCode );
  }

#if TILES_WPP_ENTRY_POINT_SIGNALLING
  READ_CODE(2, uiCode, "tiles_or_entropy_coding_sync_idc");         pcSPS->setTilesOrEntropyCodingSyncIdc(uiCode);
#endif

#if TILES_OR_ENTROPY_SYNC_IDC
  if(pcSPS->getTilesOrEntropyCodingSyncIdc() == 1)
  {
#endif
    READ_UVLC ( uiCode, "num_tile_columns_minus1" );
    pcSPS->setNumColumnsMinus1( uiCode );  
    READ_UVLC ( uiCode, "num_tile_rows_minus1" ); 
    pcSPS->setNumRowsMinus1( uiCode ); 
    READ_FLAG ( uiCode, "uniform_spacing_flag" ); 
    pcSPS->setUniformSpacingIdr( uiCode );
    if( pcSPS->getUniformSpacingIdr() == 0 )
    {
      UInt* columnWidth = (UInt*)malloc(pcSPS->getNumColumnsMinus1()*sizeof(UInt));
      for(UInt i=0; i<pcSPS->getNumColumnsMinus1(); i++)
      { 
        READ_UVLC( uiCode, "column_width" );
        columnWidth[i] = uiCode;  
      }
      pcSPS->setColumnWidth(columnWidth);
      free(columnWidth);

      UInt* rowHeight = (UInt*)malloc(pcSPS->getNumRowsMinus1()*sizeof(UInt));
      for(UInt i=0; i<pcSPS->getNumRowsMinus1(); i++)
      {
        READ_UVLC( uiCode, "row_height" );
        rowHeight[i] = uiCode;  
      }
      pcSPS->setRowHeight(rowHeight);
      free(rowHeight);  
    }
#if !REMOVE_TILE_DEPENDENCE
    pcSPS->setTileBoundaryIndependenceIdr( 1 ); //default
#endif
    pcSPS->setLFCrossTileBoundaryFlag(true); //default

    if( pcSPS->getNumColumnsMinus1() !=0 || pcSPS->getNumRowsMinus1() != 0)
    {
#if !REMOVE_TILE_DEPENDENCE
      READ_FLAG ( uiCode, "tile_boundary_independence_flag" );  
      pcSPS->setTileBoundaryIndependenceIdr( uiCode );
      if(pcSPS->getTileBoundaryIndependenceIdr() == 1)
      {
#endif
        READ_FLAG ( uiCode, "loop_filter_across_tile_flag" );  
        pcSPS->setLFCrossTileBoundaryFlag( (uiCode==1)?true:false);
#if !REMOVE_TILE_DEPENDENCE
      }
#endif
    }
#if TILES_OR_ENTROPY_SYNC_IDC
  }
#endif
  READ_FLAG( uiCode, "sps_extension_flag");
  if(uiCode)
  {
    READ_FLAG( uiCode, "interview_refs_present_flag");
    if(uiCode)
    {
      READ_UVLC( uiCode, "num_usable_interview_refs_minus1" );
      pcSPS->setNumberOfUsableInterViewRefs( uiCode + 1 );

      Int prev = 0;
      for( Int j = 0 ; j < pcSPS->getNumberOfUsableInterViewRefs(); j++ )
      {
        READ_UVLC(uiCode, "delta_usable_interview_ref_minus1");
        pcSPS->setUsableInterViewRef( j, (prev - uiCode - 1) );
        prev = pcSPS->getUsableInterViewRef( j );
      }
    }

#if HHI_DMM_WEDGE_INTRA || HHI_DMM_PRED_TEX
    READ_FLAG( uiCode, "enable_dmm_flag" ); 
    pcSPS->setUseDMM( uiCode );
#endif

#if HHI_MPI
    if( bIsDepth )
    {
      READ_FLAG( uiCode, "use_mvi_flag" );
      pcSPS->setUseMVI( uiCode );
    }
#endif

    READ_FLAG( uiCode, "base_view_flag" ); 
    if( uiCode )
    { // baseview SPS -> set standard values
      pcSPS->initMultiviewSPS         ( 0 );
#if DEPTH_MAP_GENERATION
      pcSPS->setPredDepthMapGeneration( 0, false );
#endif
#if HHI_INTER_VIEW_RESIDUAL_PRED
    pcSPS->setMultiviewResPredMode  ( 0 );
#endif
    }
    else
    {
      READ_FLAG( uiCode, "depth_flag" ); 
      if( uiCode )
      {
        READ_UVLC( uiCode, "view_id" ); 
        READ_SVLC(  iCode, "view_order_idx" ); 
        pcSPS->initMultiviewSPSDepth    ( uiCode, iCode );
#if DEPTH_MAP_GENERATION
        pcSPS->setPredDepthMapGeneration( uiCode, true );
#endif
#if HHI_INTER_VIEW_RESIDUAL_PRED
      pcSPS->setMultiviewResPredMode  ( 0 );
#endif
      }
      else
      {
        UInt  uiViewId, uiCamParPrecision;
        Int   iVOI;
        Bool  bCamParSlice;
        READ_UVLC( uiViewId, "view_id" );  uiViewId++;
        READ_SVLC( iVOI, "view_order_idx" );
        READ_UVLC( uiCamParPrecision, "camera_parameter_precision" );
        READ_FLAG( uiCode, "camera_parameter_in_slice_header" );    bCamParSlice = ( uiCode == 1 );
        if( !bCamParSlice )
        {
          for( UInt uiBaseId = 0; uiBaseId < uiViewId; uiBaseId++ )
          {
            READ_SVLC( iCode, "coded_scale" );   m_aaiTempScale [ uiBaseId ][ uiViewId ] = iCode;
            READ_SVLC( iCode, "coded_offset" );   m_aaiTempOffset[ uiBaseId ][ uiViewId ] = iCode;
            READ_SVLC( iCode, "inverse_coded_scale_plus_coded_scale" );   m_aaiTempScale [ uiViewId ][ uiBaseId ] = iCode - m_aaiTempScale [ uiBaseId ][ uiViewId ];
            READ_SVLC( iCode, "inverse_coded_offset_plus_coded_offset" );   m_aaiTempOffset[ uiViewId ][ uiBaseId ] = iCode - m_aaiTempOffset[ uiBaseId ][ uiViewId ];
          }
        }
        pcSPS->initMultiviewSPS( uiViewId, iVOI, uiCamParPrecision, bCamParSlice, m_aaiTempScale, m_aaiTempOffset );

#if DEPTH_MAP_GENERATION
        UInt uiPredDepthMapGeneration = 0, uiPdmPrecision = 0;
#if HHI_INTER_VIEW_MOTION_PRED
        UInt uiMultiviewMvPredMode = 0;
#endif
#if HHI_INTER_VIEW_RESIDUAL_PRED
      UInt uiMultiviewResPredMode = 0;
#endif
        READ_UVLC( uiPredDepthMapGeneration, "Pdm_generation" );
        if( uiPredDepthMapGeneration )
        {
          READ_UVLC( uiPdmPrecision, "Pdm_precision" );
          for( UInt uiBaseId = 0; uiBaseId < uiViewId; uiBaseId++ )
          {
            READ_SVLC( iCode, "Pdm_scale_nom_delta" );   m_aaiTempPdmScaleNomDelta[ uiViewId ][ uiBaseId ] = iCode;
            READ_SVLC( iCode, "Pdm_offset" );   m_aaiTempPdmOffset       [ uiViewId ][ uiBaseId ] = iCode;
          }
#if HHI_INTER_VIEW_MOTION_PRED
          READ_UVLC( uiMultiviewMvPredMode, "multi_view_mv_pred_mode" );
#endif
#if HHI_INTER_VIEW_RESIDUAL_PRED
          READ_FLAG( uiMultiviewResPredMode, "multi_view_residual_pred_mode" );
#endif
        }
#if HHI_INTER_VIEW_MOTION_PRED
        pcSPS->setPredDepthMapGeneration( uiViewId, false, uiPredDepthMapGeneration, uiMultiviewMvPredMode, uiPdmPrecision, m_aaiTempPdmScaleNomDelta, m_aaiTempPdmOffset );
#else
        pcSPS->setPredDepthMapGeneration( uiViewId, false, uiPredDepthMapGeneration, 0, uiPdmPrecision, m_aaiTempPdmScaleNomDelta, m_aaiTempPdmOffset );
#endif
#endif
#if HHI_INTER_VIEW_RESIDUAL_PRED
      pcSPS->setMultiviewResPredMode  ( uiMultiviewResPredMode );
#endif
      }
    }
    READ_FLAG( uiCode, "sps_extension2_flag");
    if (uiCode)
    {
      while ( xMoreRbspData() )
      {
        READ_FLAG( uiCode, "sps_extension2_data_flag");
      }
    }
  }
}

Void TDecCavlc::readTileMarker   ( UInt& uiTileIdx, UInt uiBitsUsed )
{
  xReadCode ( uiBitsUsed, uiTileIdx );
}

#if LCU_SYNTAX_ALF
Void TDecCavlc::parseSliceHeader (TComSlice*& rpcSlice, ParameterSetManagerDecoder *parameterSetManager, AlfCUCtrlInfo &alfCUCtrl, AlfParamSet& alfParamSet)
#else
Void TDecCavlc::parseSliceHeader (TComSlice*& rpcSlice, ParameterSetManagerDecoder *parameterSetManager, AlfCUCtrlInfo &alfCUCtrl)
#endif
{
  UInt  uiCode;
  Int   iCode;
  
#if ENC_DEC_TRACE
  xTraceSliceHeader(rpcSlice);
#endif
  Int numCUs = ((rpcSlice->getSPS()->getPicWidthInLumaSamples()+rpcSlice->getSPS()->getMaxCUWidth()-1)/rpcSlice->getSPS()->getMaxCUWidth())*((rpcSlice->getSPS()->getPicHeightInLumaSamples()+rpcSlice->getSPS()->getMaxCUHeight()-1)/rpcSlice->getSPS()->getMaxCUHeight());
  Int maxParts = (1<<(rpcSlice->getSPS()->getMaxCUDepth()<<1));
  Int numParts = (1<<(rpcSlice->getPPS()->getSliceGranularity()<<1));
  UInt lCUAddress = 0;
  Int reqBitsOuter = 0;
  while(numCUs>(1<<reqBitsOuter))
  {
    reqBitsOuter++;
  }
  Int reqBitsInner = 0;
  while((numParts)>(1<<reqBitsInner)) 
  {
    reqBitsInner++;
  }

  READ_FLAG( uiCode, "first_slice_in_pic_flag" );
  UInt address;
  UInt innerAddress = 0;
  if(!uiCode)
  {
    READ_CODE( reqBitsOuter+reqBitsInner, address, "slice_address" );
    lCUAddress = address >> reqBitsInner;
    innerAddress = address - (lCUAddress<<reqBitsInner);
  }
  //set uiCode to equal slice start address (or entropy slice start address)
  uiCode=(maxParts*lCUAddress)+(innerAddress*(maxParts>>(rpcSlice->getPPS()->getSliceGranularity()<<1)));
  
  rpcSlice->setEntropySliceCurStartCUAddr( uiCode );
  rpcSlice->setEntropySliceCurEndCUAddr(numCUs*maxParts);

  //   slice_type
  READ_UVLC (    uiCode, "slice_type" );            rpcSlice->setSliceType((SliceType)uiCode);
  // lightweight_slice_flag
  READ_FLAG( uiCode, "entropy_slice_flag" );
  Bool bEntropySlice = uiCode ? true : false;

  if (bEntropySlice)
  {
    rpcSlice->setNextSlice        ( false );
    rpcSlice->setNextEntropySlice ( true  );
  }
  else
  {
    rpcSlice->setNextSlice        ( true  );
    rpcSlice->setNextEntropySlice ( false );
    
    uiCode=(maxParts*lCUAddress)+(innerAddress*(maxParts>>(rpcSlice->getPPS()->getSliceGranularity()<<1)));
    rpcSlice->setSliceCurStartCUAddr(uiCode);
    rpcSlice->setSliceCurEndCUAddr(numCUs*maxParts);
  }
  TComPPS* pps = NULL;
  TComSPS* sps = NULL;

  if (!bEntropySlice)
  {
    READ_UVLC (    uiCode, "pic_parameter_set_id" );  rpcSlice->setPPSId(uiCode);
    pps = parameterSetManager->getPrefetchedPPS(uiCode);
    sps = parameterSetManager->getPrefetchedSPS(pps->getSPSId());
    rpcSlice->setSPS(sps);
    rpcSlice->setPPS(pps);
#if H0388
    if( pps->getOutputFlagPresentFlag() )
    {
      READ_FLAG( uiCode, "pic_output_flag" );
      rpcSlice->setPicOutputFlag( uiCode ? true : false );
    }
    else
    {
      rpcSlice->setPicOutputFlag( true );
    }
#endif
    if(rpcSlice->getNalUnitType()==NAL_UNIT_CODED_SLICE_IDR) 
    { 
      READ_UVLC( uiCode, "idr_pic_id" );  //ignored
      READ_FLAG( uiCode, "no_output_of_prior_pics_flag" );  //ignored
      rpcSlice->setPOC(0);
      TComReferencePictureSet* rps = rpcSlice->getLocalRPS();
      rps->setNumberOfNegativePictures(0);
      rps->setNumberOfPositivePictures(0);
      rps->setNumberOfLongtermPictures(0);
      rps->setNumberOfPictures(0);
      rpcSlice->setRPS(rps);
    }
    else
    {
      READ_CODE(sps->getBitsForPOC(), uiCode, "pic_order_cnt_lsb");  
      Int iPOClsb = uiCode;
      Int iPrevPOC = rpcSlice->getPrevPOC();
      Int iMaxPOClsb = 1<< sps->getBitsForPOC();
      Int iPrevPOClsb = iPrevPOC%iMaxPOClsb;
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
      rpcSlice->setPOC( iPOCmsb+iPOClsb );

#if HHI_FIX
      if( rpcSlice->getNalUnitType() == NAL_UNIT_CODED_SLICE_IDV && rpcSlice->getPOC() == 0 )
#else
      if( rpcSlice->getNalUnitType() == NAL_UNIT_CODED_SLICE_IDV ) 
#endif
      {
        TComReferencePictureSet* rps = rpcSlice->getLocalRPS();
        rps->setNumberOfNegativePictures(0);
        rps->setNumberOfPositivePictures(0);
        rps->setNumberOfLongtermPictures(0);
        rps->setNumberOfPictures(0);
        rpcSlice->setRPS(rps);
      }
      else
      {
        TComReferencePictureSet* rps;
        READ_FLAG( uiCode, "short_term_ref_pic_set_sps_flag" );
        if(uiCode == 0) // use short-term reference picture set explicitly signalled in slice header
        {
          rps = rpcSlice->getLocalRPS();
#if RPS_IN_SPS
          parseShortTermRefPicSet(sps,rps, sps->getRPSList()->getNumberOfReferencePictureSets());
#else
          parseShortTermRefPicSet(pps,rps, pps->getRPSList()->getNumberOfReferencePictureSets());
#endif
          rpcSlice->setRPS(rps);
        }
        else // use reference to short-term reference picture set in PPS
        {
#if RPS_IN_SPS
          READ_UVLC( uiCode, "short_term_ref_pic_set_idx"); rpcSlice->setRPS(sps->getRPSList()->getReferencePictureSet(uiCode));
#else
          READ_UVLC( uiCode, "short_term_ref_pic_set_idx"); rpcSlice->setRPS(pps->getRPSList()->getReferencePictureSet(uiCode));
#endif
          rps = rpcSlice->getRPS();
        }
#if RPS_IN_SPS
        if(sps->getLongTermRefsPresent())
#else
        if(pps->getLongTermRefsPresent())
#endif
        {
          Int offset = rps->getNumberOfNegativePictures()+rps->getNumberOfPositivePictures();
          READ_UVLC( uiCode, "num_long_term_pics");             rps->setNumberOfLongtermPictures(uiCode);
          Int prev = 0;
#if LTRP_MULT
          Int prevMsb=0;
          Int prevDeltaPocLt=0;
#endif
          for(Int j=rps->getNumberOfLongtermPictures()+offset-1 ; j > offset-1; j--)
          {
            READ_UVLC(uiCode,"delta_poc_lsb_lt"); 
            prev += uiCode;

#if LTRP_MULT
            READ_FLAG(uiCode,"delta_poc_msb_present_flag");
            Int decDeltaPOCMsbPresent=uiCode;

            if(decDeltaPOCMsbPresent==1)
            {
              READ_UVLC(uiCode, "delta_poc_msb_cycle_lt_minus1");
              if(  (j==(rps->getNumberOfLongtermPictures()+offset-1)) || (prev!=prevDeltaPocLt) )
              {
                prevMsb=(1+uiCode); 
              }
              else
              {
                prevMsb+=(1+uiCode); 
              }
              Int decMaxPocLsb = 1<<rpcSlice->getSPS()->getBitsForPOC();
              rps->setPOC(j,rpcSlice->getPOC()-prev-(prevMsb)*decMaxPocLsb); 
              rps->setDeltaPOC(j,-(Int)(prev+(prevMsb)*decMaxPocLsb));
            }
            else
            {
#endif
              rps->setPOC(j,rpcSlice->getPOC()-prev);          
              rps->setDeltaPOC(j,-(Int)prev);
#if LTRP_MULT
            }
            prevDeltaPocLt=prev;
#endif
            READ_FLAG( uiCode, "used_by_curr_pic_lt_flag");     rps->setUsed(j,uiCode);
          }
          offset += rps->getNumberOfLongtermPictures();
          rps->setNumberOfPictures(offset);        
        }  
      }
    }

    if(sps->getUseSAO() || sps->getUseALF() || sps->getScalingListFlag() || sps->getUseDF())
    {
      //!!!KS: order is different in WD5! 
      if (sps->getUseALF())
      {
        READ_FLAG(uiCode, "slice_adaptive_loop_filter_flag");
        rpcSlice->setAlfEnabledFlag((Bool)uiCode);
      }
      if (sps->getUseSAO())
      {
#if SAO_UNIT_INTERLEAVING
        READ_FLAG(uiCode, "slice_sao_interleaving_flag");        rpcSlice->setSaoInterleavingFlag(uiCode);
#endif
        READ_FLAG(uiCode, "slice_sample_adaptive_offset_flag");  rpcSlice->setSaoEnabledFlag((Bool)uiCode);
#if SAO_UNIT_INTERLEAVING
        if (rpcSlice->getSaoEnabledFlag() && rpcSlice->getSaoInterleavingFlag())
        {
          READ_FLAG(uiCode, "sao_cb_enable_flag");  rpcSlice->setSaoEnabledFlagCb((Bool)uiCode);
          READ_FLAG(uiCode, "sao_cr_enable_flag");  rpcSlice->setSaoEnabledFlagCr((Bool)uiCode);
        }
        else
        {
          rpcSlice->setSaoEnabledFlagCb(0);
          rpcSlice->setSaoEnabledFlagCr(0);
        }
#endif
      }
      READ_UVLC (    uiCode, "aps_id" );  rpcSlice->setAPSId(uiCode);
    }
    if (!rpcSlice->isIntra())
    {
      READ_FLAG( uiCode, "num_ref_idx_active_override_flag");
      if (uiCode)
      {
        READ_CODE (3, uiCode, "num_ref_idx_l0_active_minus1" );  rpcSlice->setNumRefIdx( REF_PIC_LIST_0, uiCode + 1 );
        if (rpcSlice->isInterB())
        {
          READ_CODE (3, uiCode, "num_ref_idx_l1_active_minus1" );  rpcSlice->setNumRefIdx( REF_PIC_LIST_1, uiCode + 1 );
        }
        else
        {
          rpcSlice->setNumRefIdx(REF_PIC_LIST_1, 0);
        }
      }
      else
      {
        rpcSlice->setNumRefIdx(REF_PIC_LIST_0, 0);
        rpcSlice->setNumRefIdx(REF_PIC_LIST_1, 0);
      }
    }
    TComRefPicListModification* refPicListModification = rpcSlice->getRefPicListModification();
    if( !rpcSlice->isIntra() )
    {
#if H0412_REF_PIC_LIST_RESTRICTION
      if( !rpcSlice->getSPS()->getListsModificationPresentFlag() )
      {
        refPicListModification->setRefPicListModificationFlagL0( 0 );
      }
      else
      {
#endif
        READ_FLAG( uiCode, "ref_pic_list_modification_flag_l0" ); refPicListModification->setRefPicListModificationFlagL0( uiCode ? 1 : 0 );
#if H0412_REF_PIC_LIST_RESTRICTION
      }
#endif
      
      if(refPicListModification->getRefPicListModificationFlagL0())
      {
        uiCode = 0;
        Int i = 0;
#if H0137_0138_LIST_MODIFICATION
        Int NumPocTotalCurr = rpcSlice->getNumPocTotalCurrMvc();
        if ( NumPocTotalCurr > 1 )
        {
          Int length = 1;
          NumPocTotalCurr --;
          while ( NumPocTotalCurr >>= 1) 
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
#else
        Int list_modification_idc = 0;
        while(list_modification_idc != 3)  
        {
          READ_UVLC( uiCode, "list_modification_idc" ); refPicListModification->setListIdcL0(i, uiCode );
          list_modification_idc = uiCode;
          if(uiCode != 3)
          {
            READ_UVLC( uiCode, "ref_pic_set_idx" ); refPicListModification->setRefPicSetIdxL0(i, uiCode );
          }
          i++;
        }
        refPicListModification->setNumberOfRefPicListModificationsL0(i-1);
#endif
      }
#if !H0137_0138_LIST_MODIFICATION
      else
      {
        refPicListModification->setNumberOfRefPicListModificationsL0(0); 
      }
#endif
    }
    else
    {
      refPicListModification->setRefPicListModificationFlagL0(0);
#if !H0137_0138_LIST_MODIFICATION
      refPicListModification->setNumberOfRefPicListModificationsL0(0);
#endif
    }
    if(rpcSlice->isInterB())
    {
#if H0412_REF_PIC_LIST_RESTRICTION
      if( !rpcSlice->getSPS()->getListsModificationPresentFlag() )
      {
        refPicListModification->setRefPicListModificationFlagL1( 0 );
      }
      else
      {
#endif
        READ_FLAG( uiCode, "ref_pic_list_modification_flag_l1" ); refPicListModification->setRefPicListModificationFlagL1( uiCode ? 1 : 0 );
#if H0412_REF_PIC_LIST_RESTRICTION
      }
#endif
      if(refPicListModification->getRefPicListModificationFlagL1())
      {
        uiCode = 0;
        Int i = 0;
#if H0137_0138_LIST_MODIFICATION
        Int NumPocTotalCurr = rpcSlice->getNumPocTotalCurrMvc();
        if ( NumPocTotalCurr > 1 )
        {
          Int length = 1;
          NumPocTotalCurr --;
          while ( NumPocTotalCurr >>= 1) 
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
#else
        Int list_modification_idc = 0;
        while(list_modification_idc != 3)  
        {
          READ_UVLC( uiCode, "list_modification_idc" ); refPicListModification->setListIdcL1(i, uiCode );
          list_modification_idc = uiCode;
          if(uiCode != 3)
          {
            READ_UVLC( uiCode, "ref_pic_set_idx" ); refPicListModification->setRefPicSetIdxL1(i, uiCode );
          }
          i++;
        }
        refPicListModification->setNumberOfRefPicListModificationsL1(i-1);
#endif
      }
#if !H0137_0138_LIST_MODIFICATION
      else
      {
        refPicListModification->setNumberOfRefPicListModificationsL1(0);
      }
#endif
    }  
    else
    {
      refPicListModification->setRefPicListModificationFlagL1(0);
#if !H0137_0138_LIST_MODIFICATION
      refPicListModification->setNumberOfRefPicListModificationsL1(0);
#endif
    }
  }
  else
  {
    // initialize from previous slice
    pps = rpcSlice->getPPS();
    sps = rpcSlice->getSPS();
  }
  // ref_pic_list_combination( )
  //!!!KS: ref_pic_list_combination() should be conditioned on entropy_slice_flag
  if (rpcSlice->isInterB())
  {
    READ_FLAG( uiCode, "ref_pic_list_combination_flag" );       rpcSlice->setRefPicListCombinationFlag( uiCode ? 1 : 0 );
    if(uiCode)
    {
      READ_UVLC( uiCode, "num_ref_idx_lc_active_minus1" );      rpcSlice->setNumRefIdx( REF_PIC_LIST_C, uiCode + 1 );
      
#if H0412_REF_PIC_LIST_RESTRICTION
      if(rpcSlice->getSPS()->getListsModificationPresentFlag() )
      {
#endif
        READ_FLAG( uiCode, "ref_pic_list_modification_flag_lc" ); rpcSlice->setRefPicListModificationFlagLC( uiCode ? 1 : 0 );
        if(uiCode)
        {
          for (UInt i=0;i<rpcSlice->getNumRefIdx(REF_PIC_LIST_C);i++)
          {
            READ_FLAG( uiCode, "pic_from_list_0_flag" );
            rpcSlice->setListIdFromIdxOfLC(i, uiCode);
#if H0137_0138_LIST_MODIFICATION
          if (((rpcSlice->getListIdFromIdxOfLC(i) == REF_PIC_LIST_0) && (rpcSlice->getNumRefIdx( REF_PIC_LIST_0 ) == 1)) || ((rpcSlice->getListIdFromIdxOfLC(i) == REF_PIC_LIST_1) && (rpcSlice->getNumRefIdx( REF_PIC_LIST_1 ) == 1)) )
          {
            uiCode = 0;
          }
          else
          {
            READ_UVLC( uiCode, "ref_idx_list_curr" );
          }
#else
            READ_UVLC( uiCode, "ref_idx_list_curr" );
#endif
            rpcSlice->setRefIdxFromIdxOfLC(i, uiCode);
            rpcSlice->setRefIdxOfLC((RefPicList)rpcSlice->getListIdFromIdxOfLC(i), rpcSlice->getRefIdxFromIdxOfLC(i), i);
          }
        }
#if H0412_REF_PIC_LIST_RESTRICTION
      }
      else
      {
        rpcSlice->setRefPicListModificationFlagLC(false);
      }
#endif
    }
    else
    {
      rpcSlice->setRefPicListModificationFlagLC(false);
      rpcSlice->setNumRefIdx(REF_PIC_LIST_C, 0);
    }
  }
  else
  {
    rpcSlice->setRefPicListCombinationFlag(false);      
  }
  
#if H0111_MVD_L1_ZERO
  if (rpcSlice->isInterB())
  {
    READ_FLAG( uiCode, "mvd_l1_zero_flag" );       rpcSlice->setMvdL1ZeroFlag( (uiCode ? true : false) );
  }
#endif

#if CABAC_INIT_FLAG
  rpcSlice->setCabacInitFlag( false ); // default
  if(pps->getCabacInitPresentFlag() && !rpcSlice->isIntra())
  {
    READ_FLAG(uiCode, "cabac_init_flag");
    rpcSlice->setCabacInitFlag( uiCode ? true : false );
  }
#else
  if(pps->getEntropyCodingMode() && !rpcSlice->isIntra())
  {
    READ_UVLC(uiCode, "cabac_init_idc");
    rpcSlice->setCABACinitIDC(uiCode);
  }
  else if (pps->getEntropyCodingMode() && rpcSlice->isIntra())
  {
    rpcSlice->setCABACinitIDC(0);
  }
#endif

  if(!bEntropySlice)
  {
    READ_SVLC( iCode, "slice_qp_delta" ); 
    rpcSlice->setSliceQp (26 + pps->getPicInitQPMinus26() + iCode);

#if H0736_AVC_STYLE_QP_RANGE
    assert( rpcSlice->getSliceQp() >= -sps->getQpBDOffsetY() );
    assert( rpcSlice->getSliceQp() <=  51 );
#endif

#if DBL_CONTROL
    if (rpcSlice->getPPS()->getDeblockingFilterControlPresent())
    {
      if ( rpcSlice->getSPS()->getUseDF() )
      {
        READ_FLAG ( uiCode, "inherit_dbl_param_from_APS_flag" ); rpcSlice->setInheritDblParamFromAPS(uiCode ? 1 : 0);
      } else
      {
        rpcSlice->setInheritDblParamFromAPS(0);
      }
#else
    READ_FLAG ( uiCode, "inherit_dbl_param_from_APS_flag" ); rpcSlice->setInheritDblParamFromAPS(uiCode ? 1 : 0);
#endif
      if(!rpcSlice->getInheritDblParamFromAPS())
      {
        READ_FLAG ( uiCode, "disable_deblocking_filter_flag" );  rpcSlice->setLoopFilterDisable(uiCode ? 1 : 0);
        if(!rpcSlice->getLoopFilterDisable())
        {
          READ_SVLC( iCode, "beta_offset_div2" ); rpcSlice->setLoopFilterBetaOffset(iCode);
          READ_SVLC( iCode, "tc_offset_div2" ); rpcSlice->setLoopFilterTcOffset(iCode);
        }
      }
#if DBL_CONTROL
   }
#endif
    if ( rpcSlice->getSliceType() == B_SLICE )
    {
      READ_FLAG( uiCode, "collocated_from_l0_flag" );
      rpcSlice->setColDir(uiCode);
    }

#if COLLOCATED_REF_IDX
    if ( rpcSlice->getSliceType() != I_SLICE &&
      ((rpcSlice->getColDir()==0 && rpcSlice->getNumRefIdx(REF_PIC_LIST_0)>1)||
      (rpcSlice->getColDir() ==1 && rpcSlice->getNumRefIdx(REF_PIC_LIST_1)>1)))
    {
      READ_UVLC( uiCode, "collocated_ref_idx" );
      rpcSlice->setColRefIdx(uiCode);
    }
#endif
    
    if ( (pps->getUseWP() && rpcSlice->getSliceType()==P_SLICE) || (pps->getWPBiPredIdc() && rpcSlice->getSliceType()==B_SLICE) )
    {
      xParsePredWeightTable(rpcSlice);
      rpcSlice->initWpScaling();
    }
  }
 
  if (!bEntropySlice)
  {
    if( rpcSlice->getSPS()->hasCamParInSliceHeader() )
    {
      UInt uiViewId = rpcSlice->getSPS()->getViewId();
      for( UInt uiBaseId = 0; uiBaseId < uiViewId; uiBaseId++ )
      {
        READ_SVLC( iCode, "coded_scale" );   m_aaiTempScale [ uiBaseId ][ uiViewId ] = iCode;
        READ_SVLC( iCode, "coded_offset" );   m_aaiTempOffset[ uiBaseId ][ uiViewId ] = iCode;
        READ_SVLC( iCode, "inverse_coded_scale_plus_coded_scale" );   m_aaiTempScale [ uiViewId ][ uiBaseId ] = iCode - m_aaiTempScale [ uiBaseId ][ uiViewId ];
        READ_SVLC( iCode, "inverse_coded_offset_plus_coded_offset" );   m_aaiTempOffset[ uiViewId ][ uiBaseId ] = iCode - m_aaiTempOffset[ uiBaseId ][ uiViewId ];
      }
      rpcSlice->initMultiviewSlice( m_aaiTempScale, m_aaiTempOffset );
    }
  }

#if ( HHI_MPI || HHI_INTER_VIEW_MOTION_PRED )
  #if ( HHI_MPI && HHI_INTER_VIEW_MOTION_PRED )
  const int iExtraMergeCandidates = ( sps->getUseMVI() || sps->getMultiviewMvPredMode() ) ? 1 : 0;
  #elif HHI_MPI
  const int iExtraMergeCandidates = sps->getUseMVI() ? 1 : 0;
  #else
  const int iExtraMergeCandidates = sps->getMultiviewMvPredMode() ? 1 : 0;
  #endif
  READ_UVLC( uiCode, "5_minus_max_num_merge_cand");
  rpcSlice->setMaxNumMergeCand(MRG_MAX_NUM_CANDS + iExtraMergeCandidates - uiCode);
  assert(rpcSlice->getMaxNumMergeCand()==(MRG_MAX_NUM_CANDS_SIGNALED+iExtraMergeCandidates));
#else
  READ_UVLC( uiCode, "5_minus_max_num_merge_cand");
  rpcSlice->setMaxNumMergeCand(MRG_MAX_NUM_CANDS - uiCode);
  assert(rpcSlice->getMaxNumMergeCand()==MRG_MAX_NUM_CANDS_SIGNALED);
#endif

  if (!bEntropySlice)
  {
    if(sps->getUseALF() && rpcSlice->getAlfEnabledFlag())
    {
      UInt uiNumLCUsInWidth   = sps->getPicWidthInLumaSamples()  / g_uiMaxCUWidth;
      UInt uiNumLCUsInHeight  = sps->getPicHeightInLumaSamples() / g_uiMaxCUHeight;

      uiNumLCUsInWidth  += ( sps->getPicWidthInLumaSamples() % g_uiMaxCUWidth ) ? 1 : 0;
      uiNumLCUsInHeight += ( sps->getPicHeightInLumaSamples() % g_uiMaxCUHeight ) ? 1 : 0;

      Int uiNumCUsInFrame = uiNumLCUsInWidth* uiNumLCUsInHeight; 
#if LCU_SYNTAX_ALF
      if(sps->getUseALFCoefInSlice())
      {
        alfParamSet.releaseALFParam();
        alfParamSet.init();
        Bool isAcrossSlice = sps->getLFCrossSliceBoundaryFlag();   
        Int numSUinLCU    = 1<< (g_uiMaxCUDepth << 1); 
        Int firstLCUAddr   = rpcSlice->getSliceCurStartCUAddr() / numSUinLCU;  
        xParseAlfParam(&alfParamSet, false, firstLCUAddr, isAcrossSlice, uiNumLCUsInWidth, uiNumLCUsInHeight);
      }

      if(!sps->getUseALFCoefInSlice())
      {
#endif
      xParseAlfCuControlParam(alfCUCtrl, uiNumCUsInFrame);
#if LCU_SYNTAX_ALF
      }
#endif

    }
  }
  
  //!!!KS: The following syntax is not aligned with the working draft, TRACE support needs to be added
  rpcSlice->setTileMarkerFlag ( 0 ); // default
  if (!bEntropySlice)
  {
#if !REMOVE_TILE_DEPENDENCE
    if (sps->getTileBoundaryIndependenceIdr())
    {   
#endif
    xReadCode(1, uiCode); // read flag indicating if tile markers transmitted
    rpcSlice->setTileMarkerFlag( uiCode );
#if !REMOVE_TILE_DEPENDENCE
    }
#endif
  }

#if TILES_WPP_ENTRY_POINT_SIGNALLING
  Int tilesOrEntropyCodingSyncIdc = rpcSlice->getSPS()->getTilesOrEntropyCodingSyncIdc();
  UInt *entryPointOffset          = NULL;
  UInt numEntryPointOffsets, offsetLenMinus1;

  rpcSlice->setNumEntryPointOffsets ( 0 ); // default
  
  if (tilesOrEntropyCodingSyncIdc>0)
  {
    READ_UVLC(numEntryPointOffsets, "num_entry_point_offsets"); rpcSlice->setNumEntryPointOffsets ( numEntryPointOffsets );
    if (numEntryPointOffsets>0)
    {
      READ_UVLC(offsetLenMinus1, "offset_len_minus1");
    }
    entryPointOffset = new UInt[numEntryPointOffsets];
    for (UInt idx=0; idx<numEntryPointOffsets; idx++)
    {
      Int bitsRead = m_pcBitstream->getNumBitsRead();
      READ_CODE(offsetLenMinus1+1, uiCode, "entry_point_offset");
      entryPointOffset[ idx ] = uiCode;
      if ( idx == 0 && tilesOrEntropyCodingSyncIdc == 2 )
      {
        // Subtract distance from NALU header start to provide WPP 0-th substream the correct size.
        entryPointOffset[ idx ] -= ( bitsRead + numEntryPointOffsets*(offsetLenMinus1+1) ) >> 3;
      }
    }
  }

  if ( tilesOrEntropyCodingSyncIdc == 1 ) // tiles
  {
    rpcSlice->setTileLocationCount( numEntryPointOffsets );

    UInt prevPos = 0;
    for (Int idx=0; idx<rpcSlice->getTileLocationCount(); idx++)
    {
      rpcSlice->setTileLocation( idx, prevPos + entryPointOffset [ idx ] );
      prevPos += entryPointOffset[ idx ];
    }
  }
  else if ( tilesOrEntropyCodingSyncIdc == 2 ) // wavefront
  {
    Int numSubstreams = pps->getNumSubstreams();
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
#else
#if WPP_SIMPLIFICATION
  if (pps->getNumSubstreams() > 1)
#else
  if (pps->getEntropyCodingSynchro())
#endif
  {
    UInt uiNumSubstreams = pps->getNumSubstreams();
    rpcSlice->allocSubstreamSizes(uiNumSubstreams);
    UInt *puiSubstreamSizes = rpcSlice->getSubstreamSizes();

    for (UInt ui = 0; ui+1 < uiNumSubstreams; ui++)
    {
      xReadCode(2, uiCode);
      
      switch ( uiCode )
      {
      case 0:
        xReadCode(8,  uiCode);
        break;
      case 1:
        xReadCode(16, uiCode);
        break;
      case 2:
        xReadCode(24, uiCode);
        break;
      case 3:
        xReadCode(32, uiCode);
        break;
      default:
        printf("Error in parseSliceHeader\n");
        exit(-1);
        break;
      }
      puiSubstreamSizes[ui] = uiCode;
    }
  }
#endif

  if (!bEntropySlice)
  {
    // Reading location information
#if !REMOVE_TILE_DEPENDENCE
    if (sps->getTileBoundaryIndependenceIdr())
    {   
#endif
#if !TILES_WPP_ENTRY_POINT_SIGNALLING
      xReadCode(1, uiCode); // read flag indicating if location information signaled in slice header
      Bool bTileLocationInformationInSliceHeaderFlag = (uiCode)? true : false;

      if (bTileLocationInformationInSliceHeaderFlag)
      {
        // location count
        xReadCode(5, uiCode); // number of tiles for which location information signaled
        rpcSlice->setTileLocationCount ( uiCode + 1 );

        xReadCode(5, uiCode); // number of bits used by diff
        Int iBitsUsedByDiff = uiCode + 1;

        // read out tile start location
        Int iLastSize = 0;
        for (UInt uiIdx=0; uiIdx<rpcSlice->getTileLocationCount(); uiIdx++)
        {
          Int iAbsDiff, iCurSize, iCurDiff;
          if (uiIdx==0)
          {
            xReadCode(iBitsUsedByDiff-1, uiCode); iAbsDiff  = uiCode;
            rpcSlice->setTileLocation( uiIdx, iAbsDiff );
            iCurDiff  = iAbsDiff;
            iLastSize = iAbsDiff;
          }
          else
          {
            xReadCode(1, uiCode); // read sign
            Int iSign = (uiCode) ? -1 : +1;

            xReadCode(iBitsUsedByDiff-1, uiCode); iAbsDiff  = uiCode;
            iCurDiff  = (iSign) * iAbsDiff;
            iCurSize  = iLastSize + iCurDiff;
            iLastSize = iCurSize;
            rpcSlice->setTileLocation( uiIdx, rpcSlice->getTileLocation( uiIdx-1 ) + iCurSize ); // calculate byte location
          }
        }
      }
#endif

      // read out trailing bits
    m_pcBitstream->readOutTrailingBits();
#if !REMOVE_TILE_DEPENDENCE
    }
#endif
  }
  return;
}

Void TDecCavlc::xParseAlfCuControlParam(AlfCUCtrlInfo& cAlfParam, Int iNumCUsInPic)
{
  UInt uiSymbol;
  Int iSymbol;

  READ_FLAG (uiSymbol, "alf_cu_control_flag");
  cAlfParam.cu_control_flag = uiSymbol;
  if (cAlfParam.cu_control_flag)
  {
    READ_UVLC (uiSymbol, "alf_cu_control_max_depth"); 
    cAlfParam.alf_max_depth = uiSymbol;

    READ_SVLC (iSymbol, "alf_length_cu_control_info");
    cAlfParam.num_alf_cu_flag = (UInt)(iSymbol + iNumCUsInPic);

    cAlfParam.alf_cu_flag.resize(cAlfParam.num_alf_cu_flag);

    for(UInt i=0; i< cAlfParam.num_alf_cu_flag; i++)
    {
      READ_FLAG (cAlfParam.alf_cu_flag[i], "alf_cu_flag");
    }
  }
}

#if !CABAC_INIT_FLAG
Void TDecCavlc::resetEntropy          (TComSlice* pcSlice)
{
}
#endif

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

Void TDecCavlc::parseSkipFlag( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth )
{
  assert(0);
}

#if HHI_INTER_VIEW_MOTION_PRED
Void TDecCavlc::parseMVPIdx( Int& riMVPIdx, Int iAMVPCands )
#else
Void TDecCavlc::parseMVPIdx( Int& riMVPIdx )
#endif
{
  assert(0);
}

Void TDecCavlc::parseSplitFlag     ( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth )
{
  assert(0);
}

Void TDecCavlc::parsePartSize( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth )
{
  assert(0);
}

Void TDecCavlc::parsePredMode( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth )
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
Void TDecCavlc::parseIPCMInfo( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth )
{
#if BURST_IPCM
  assert(0);
#else
  UInt uiSymbol;

  xReadFlag( uiSymbol );

  if ( uiSymbol )
  {
    Bool bIpcmFlag   = true;

    xReadPCMAlignZero();

    pcCU->setPartSizeSubParts  ( SIZE_2Nx2N, uiAbsPartIdx, uiDepth );
    pcCU->setSizeSubParts      ( g_uiMaxCUWidth>>uiDepth, g_uiMaxCUHeight>>uiDepth, uiAbsPartIdx, uiDepth );
    pcCU->setIPCMFlagSubParts  ( bIpcmFlag, uiAbsPartIdx, uiDepth );

    UInt uiMinCoeffSize = pcCU->getPic()->getMinCUWidth()*pcCU->getPic()->getMinCUHeight();
    UInt uiLumaOffset   = uiMinCoeffSize*uiAbsPartIdx;
    UInt uiChromaOffset = uiLumaOffset>>2;

    Pel* piPCMSample;
    UInt uiWidth;
    UInt uiHeight;
    UInt uiSampleBits;
    UInt uiX, uiY;

    piPCMSample = pcCU->getPCMSampleY() + uiLumaOffset;
    uiWidth = pcCU->getWidth(uiAbsPartIdx);
    uiHeight = pcCU->getHeight(uiAbsPartIdx);
    uiSampleBits = pcCU->getSlice()->getSPS()->getPCMBitDepthLuma();

    for(uiY = 0; uiY < uiHeight; uiY++)
    {
      for(uiX = 0; uiX < uiWidth; uiX++)
      {
        UInt uiSample;
        xReadCode(uiSampleBits, uiSample);

        piPCMSample[uiX] = uiSample;
      }
      piPCMSample += uiWidth;
    }

    piPCMSample = pcCU->getPCMSampleCb() + uiChromaOffset;
    uiWidth = pcCU->getWidth(uiAbsPartIdx)/2;
    uiHeight = pcCU->getHeight(uiAbsPartIdx)/2;
    uiSampleBits = pcCU->getSlice()->getSPS()->getPCMBitDepthChroma();

    for(uiY = 0; uiY < uiHeight; uiY++)
    {
      for(uiX = 0; uiX < uiWidth; uiX++)
      {
        UInt uiSample;
        xReadCode(uiSampleBits, uiSample);
        piPCMSample[uiX] = uiSample;
      }
      piPCMSample += uiWidth;
    }

    piPCMSample = pcCU->getPCMSampleCr() + uiChromaOffset;
    uiWidth = pcCU->getWidth(uiAbsPartIdx)/2;
    uiHeight = pcCU->getHeight(uiAbsPartIdx)/2;
    uiSampleBits = pcCU->getSlice()->getSPS()->getPCMBitDepthChroma();

    for(uiY = 0; uiY < uiHeight; uiY++)
    {
      for(uiX = 0; uiX < uiWidth; uiX++)
      {
        UInt uiSample;
        xReadCode(uiSampleBits, uiSample);
        piPCMSample[uiX] = uiSample;
      }
      piPCMSample += uiWidth;
    }
  }
#endif
}

Void TDecCavlc::parseIntraDirLumaAng  ( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth )
{ 
  assert(0);
}

Void TDecCavlc::parseIntraDirChroma( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth )
{
  assert(0);
}

Void TDecCavlc::parseInterDir( TComDataCU* pcCU, UInt& ruiInterDir, UInt uiAbsPartIdx, UInt uiDepth )
{
  assert(0);
}

Void TDecCavlc::parseRefFrmIdx( TComDataCU* pcCU, Int& riRefFrmIdx, UInt uiAbsPartIdx, UInt uiDepth, RefPicList eRefList )
{
  assert(0);
}

Void TDecCavlc::parseMvd( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiPartIdx, UInt uiDepth, RefPicList eRefList )
{
  assert(0);
}

Void TDecCavlc::parseDeltaQP( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth )
{
#if H0736_AVC_STYLE_QP_RANGE
  Int qp;
#else
  UInt uiQp;
#endif
  Int  iDQp;
  
  xReadSvlc( iDQp );

#if H0736_AVC_STYLE_QP_RANGE
  Int qpBdOffsetY = pcCU->getSlice()->getSPS()->getQpBDOffsetY();
  qp = (((Int) pcCU->getRefQP( uiAbsPartIdx ) + iDQp + 52 + 2*qpBdOffsetY )%(52+ qpBdOffsetY)) -  qpBdOffsetY;
#else
  uiQp = pcCU->getRefQP( uiAbsPartIdx ) + iDQp;
#endif

  UInt uiAbsQpCUPartIdx = (uiAbsPartIdx>>(8-(pcCU->getSlice()->getPPS()->getMaxCuDQPDepth()<<1)))<<(8-(pcCU->getSlice()->getPPS()->getMaxCuDQPDepth()<<1)) ;
  UInt uiQpCUDepth =   min(uiDepth,pcCU->getSlice()->getPPS()->getMaxCuDQPDepth()) ;

#if H0736_AVC_STYLE_QP_RANGE
  pcCU->setQPSubParts( qp, uiAbsQpCUPartIdx, uiQpCUDepth );
#else
  pcCU->setQPSubParts( uiQp, uiAbsQpCUPartIdx, uiQpCUDepth );
#endif
}

Void TDecCavlc::parseCoeffNxN( TComDataCU* pcCU, TCoeff* pcCoef, UInt uiAbsPartIdx, UInt uiWidth, UInt uiHeight, UInt uiDepth, TextType eTType )
{
  assert(0);
}

Void TDecCavlc::parseTransformSubdivFlag( UInt& ruiSubdivFlag, UInt uiLog2TransformBlockSize )
{
  assert(0);
}

Void TDecCavlc::parseQtCbf( TComDataCU* pcCU, UInt uiAbsPartIdx, TextType eType, UInt uiTrDepth, UInt uiDepth )
{
  assert(0);
}

Void TDecCavlc::parseQtRootCbf( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth, UInt& uiQtRootCbf )
{
  assert(0);
}


Void TDecCavlc::parseMergeFlag ( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth, UInt uiPUIdx )
{
  assert(0);
}

Void TDecCavlc::parseMergeIndex ( TComDataCU* pcCU, UInt& ruiMergeIndex, UInt uiAbsPartIdx, UInt uiDepth )
{
  assert(0);
}

#if HHI_INTER_VIEW_RESIDUAL_PRED
Void
TDecCavlc::parseResPredFlag( TComDataCU* pcCU, Bool& rbResPredFlag, UInt uiAbsPartIdx, UInt uiDepth )
{
  assert(0);
}
#endif

// ====================================================================================================================
// Protected member functions
// ====================================================================================================================

Void TDecCavlc::xReadCode (UInt uiLength, UInt& ruiCode)
{
  assert ( uiLength > 0 );
  m_pcBitstream->read (uiLength, ruiCode);
}

Void TDecCavlc::xReadUvlc( UInt& ruiVal)
{
  UInt uiVal = 0;
  UInt uiCode = 0;
  UInt uiLength;
  m_pcBitstream->read( 1, uiCode );
  
  if( 0 == uiCode )
  {
    uiLength = 0;
    
    while( ! ( uiCode & 1 ))
    {
      m_pcBitstream->read( 1, uiCode );
      uiLength++;
    }
    
    m_pcBitstream->read( uiLength, uiVal );
    
    uiVal += (1 << uiLength)-1;
  }
  
  ruiVal = uiVal;
}

Void TDecCavlc::xReadSvlc( Int& riVal)
{
  UInt uiBits = 0;
  m_pcBitstream->read( 1, uiBits );
  if( 0 == uiBits )
  {
    UInt uiLength = 0;
    
    while( ! ( uiBits & 1 ))
    {
      m_pcBitstream->read( 1, uiBits );
      uiLength++;
    }
    
    m_pcBitstream->read( uiLength, uiBits );
    
    uiBits += (1 << uiLength);
    riVal = ( uiBits & 1) ? -(Int)(uiBits>>1) : (Int)(uiBits>>1);
  }
  else
  {
    riVal = 0;
  }
}

Void TDecCavlc::xReadFlag (UInt& ruiCode)
{
  m_pcBitstream->read( 1, ruiCode );
}

/** Parse PCM alignment zero bits.
 * \returns Void
 */
Void TDecCavlc::xReadPCMAlignZero( )
{
  UInt uiNumberOfBits = m_pcBitstream->getNumBitsUntilByteAligned();

  if(uiNumberOfBits)
  {
    UInt uiBits;
    UInt uiSymbol;

    for(uiBits = 0; uiBits < uiNumberOfBits; uiBits++)
    {
      xReadFlag( uiSymbol );

      if(uiSymbol)
      {
        printf("\nWarning! pcm_align_zero include a non-zero value.\n");
      }
    }
  }
}

Void TDecCavlc::xReadUnaryMaxSymbol( UInt& ruiSymbol, UInt uiMaxSymbol )
{
  if (uiMaxSymbol == 0)
  {
    ruiSymbol = 0;
    return;
  }
  
  xReadFlag( ruiSymbol );
  
  if (ruiSymbol == 0 || uiMaxSymbol == 1)
  {
    return;
  }
  
  UInt uiSymbol = 0;
  UInt uiCont;
  
  do
  {
    xReadFlag( uiCont );
    uiSymbol++;
  }
  while( uiCont && (uiSymbol < uiMaxSymbol-1) );
  
  if( uiCont && (uiSymbol == uiMaxSymbol-1) )
  {
    uiSymbol++;
  }
  
  ruiSymbol = uiSymbol;
}

Void TDecCavlc::xReadExGolombLevel( UInt& ruiSymbol )
{
  UInt uiSymbol ;
  UInt uiCount = 0;
  do
  {
    xReadFlag( uiSymbol );
    uiCount++;
  }
  while( uiSymbol && (uiCount != 13));
  
  ruiSymbol = uiCount-1;
  
  if( uiSymbol )
  {
    xReadEpExGolomb( uiSymbol, 0 );
    ruiSymbol += uiSymbol+1;
  }
  
  return;
}

Void TDecCavlc::xReadEpExGolomb( UInt& ruiSymbol, UInt uiCount )
{
  UInt uiSymbol = 0;
  UInt uiBit = 1;
  
  
  while( uiBit )
  {
    xReadFlag( uiBit );
    uiSymbol += uiBit << uiCount++;
  }
  
  uiCount--;
  while( uiCount-- )
  {
    xReadFlag( uiBit );
    uiSymbol += uiBit << uiCount;
  }
  
  ruiSymbol = uiSymbol;
  
  return;
}

UInt TDecCavlc::xGetBit()
{
  UInt ruiCode;
  m_pcBitstream->read( 1, ruiCode );
  return ruiCode;
}


/** parse explicit wp tables
 * \param TComSlice* pcSlice
 * \returns Void
 */
Void TDecCavlc::xParsePredWeightTable( TComSlice* pcSlice )
{
  wpScalingParam  *wp;
  Bool            bChroma     = true; // color always present in HEVC ?
  TComPPS*        pps         = pcSlice->getPPS();
  SliceType       eSliceType  = pcSlice->getSliceType();
  Int             iNbRef       = (eSliceType == B_SLICE ) ? (2) : (1);
  UInt            uiLog2WeightDenomLuma, uiLog2WeightDenomChroma;
  UInt            uiMode      = 0;

  if ( (eSliceType==P_SLICE && pps->getUseWP()) || (eSliceType==B_SLICE && pps->getWPBiPredIdc()==1 && pcSlice->getRefPicListCombinationFlag()==0) )
    uiMode = 1; // explicit
  else if ( eSliceType==B_SLICE && pps->getWPBiPredIdc()==2 )
    uiMode = 2; // implicit
  else if (eSliceType==B_SLICE && pps->getWPBiPredIdc()==1 && pcSlice->getRefPicListCombinationFlag())
    uiMode = 3; // combined explicit

  if ( uiMode == 1 )  // explicit
  {
    printf("\nTDecCavlc::xParsePredWeightTable(poc=%d) explicit...\n", pcSlice->getPOC());
    Int iDeltaDenom;
    // decode delta_luma_log2_weight_denom :
    READ_UVLC( uiLog2WeightDenomLuma, "luma_log2_weight_denom" );     // ue(v): luma_log2_weight_denom
    if( bChroma ) 
    {
      READ_SVLC( iDeltaDenom, "delta_chroma_log2_weight_denom" );     // se(v): delta_chroma_log2_weight_denom
      assert((iDeltaDenom + (Int)uiLog2WeightDenomLuma)>=0);
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
        if ( wp[0].bPresentFlag ) 
        {
          Int iDeltaWeight;
          READ_SVLC( iDeltaWeight, "delta_luma_weight_lX" );  // se(v): delta_luma_weight_l0[i]
          wp[0].iWeight = (iDeltaWeight + (1<<wp[0].uiLog2WeightDenom));
          READ_SVLC( wp[0].iOffset, "luma_offset_lX" );       // se(v): luma_offset_l0[i]
        }
        else 
        {
          wp[0].iWeight = (1 << wp[0].uiLog2WeightDenom);
          wp[0].iOffset = 0;
        }
        if ( bChroma ) 
        {
          READ_FLAG( uiCode, "chroma_weight_lX_flag" );      // u(1): chroma_weight_l0_flag
          wp[1].bPresentFlag = ( uiCode == 1 );
          wp[2].bPresentFlag = ( uiCode == 1 );
          if ( wp[1].bPresentFlag ) 
          {
            for ( Int j=1 ; j<3 ; j++ ) 
            {
              Int iDeltaWeight;
              READ_SVLC( iDeltaWeight, "delta_chroma_weight_lX" );  // se(v): chroma_weight_l0[i][j]
              wp[j].iWeight = (iDeltaWeight + (1<<wp[1].uiLog2WeightDenom));

              Int iDeltaChroma;
              READ_SVLC( iDeltaChroma, "delta_chroma_offset_lX" );  // se(v): delta_chroma_offset_l0[i][j]
              wp[j].iOffset = iDeltaChroma - ( ( (g_uiIBDI_MAX>>1)*wp[j].iWeight)>>(wp[j].uiLog2WeightDenom) ) + (g_uiIBDI_MAX>>1);
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
  }
  else if ( uiMode == 2 )  // implicit
  {
    printf("\nTDecCavlc::xParsePredWeightTable(poc=%d) implicit...\n", pcSlice->getPOC());
  }
  else if ( uiMode == 3 )  // combined explicit
  {
    printf("\nTDecCavlc::xParsePredWeightTable(poc=%d) combined explicit...\n", pcSlice->getPOC());
    Int iDeltaDenom;
    // decode delta_luma_log2_weight_denom :
    READ_UVLC ( uiLog2WeightDenomLuma, "luma_log2_weight_denom" );     // ue(v): luma_log2_weight_denom
    if( bChroma ) 
    {
      READ_SVLC( iDeltaDenom, "delta_chroma_log2_weight_denom" );      // ue(v): delta_chroma_log2_weight_denom
      assert((iDeltaDenom + (Int)uiLog2WeightDenomLuma)>=0);
      uiLog2WeightDenomChroma = (UInt)(iDeltaDenom + uiLog2WeightDenomLuma);
    }

    for ( Int iRefIdx=0 ; iRefIdx<pcSlice->getNumRefIdx(REF_PIC_LIST_C) ; iRefIdx++ ) 
    {
      pcSlice->getWpScalingLC(iRefIdx, wp);

      wp[0].uiLog2WeightDenom = uiLog2WeightDenomLuma;
      wp[1].uiLog2WeightDenom = uiLog2WeightDenomChroma;
      wp[2].uiLog2WeightDenom = uiLog2WeightDenomChroma;

      UInt  uiCode;
      READ_FLAG( uiCode, "luma_weight_lc_flag" );                  // u(1): luma_weight_lc_flag
      wp[0].bPresentFlag = ( uiCode == 1 );
      if ( wp[0].bPresentFlag ) 
      {
        Int iDeltaWeight;
        READ_SVLC( iDeltaWeight, "delta_luma_weight_lc" );          // se(v): delta_luma_weight_lc
        wp[0].iWeight = (iDeltaWeight + (1<<wp[0].uiLog2WeightDenom));
        READ_SVLC( wp[0].iOffset, "luma_offset_lc" );               // se(v): luma_offset_lc
      }
      else 
      {
        wp[0].iWeight = (1 << wp[0].uiLog2WeightDenom);
        wp[0].iOffset = 0;
      }
      if ( bChroma ) 
      {
        READ_FLAG( uiCode, "chroma_weight_lc_flag" );                // u(1): chroma_weight_lc_flag
        wp[1].bPresentFlag = ( uiCode == 1 );
        wp[2].bPresentFlag = ( uiCode == 1 );
        if ( wp[1].bPresentFlag ) 
        {
          for ( Int j=1 ; j<3 ; j++ ) 
          {
            Int iDeltaWeight;
            READ_SVLC( iDeltaWeight, "delta_chroma_weight_lc" );      // se(v): delta_chroma_weight_lc
            wp[j].iWeight = (iDeltaWeight + (1<<wp[1].uiLog2WeightDenom));

            Int iDeltaChroma;
            READ_SVLC( iDeltaChroma, "delta_chroma_offset_lc" );      // se(v): delta_chroma_offset_lc
            wp[j].iOffset = iDeltaChroma - ( ( (g_uiIBDI_MAX>>1)*wp[j].iWeight)>>(wp[j].uiLog2WeightDenom) ) + (g_uiIBDI_MAX>>1);
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

    for ( Int iRefIdx=pcSlice->getNumRefIdx(REF_PIC_LIST_C) ; iRefIdx<2*MAX_NUM_REF ; iRefIdx++ ) 
    {
      pcSlice->getWpScalingLC(iRefIdx, wp);

      wp[0].bPresentFlag = false;
      wp[1].bPresentFlag = false;
      wp[2].bPresentFlag = false;
    }
  }
  else
  {
    printf("\n wrong weight pred table syntax \n ");
    assert(0);
  }
}

/** decode quantization matrix
 * \param scalingList quantization matrix information
 */
Void TDecCavlc::parseScalingList(TComScalingList* scalingList)
{
  UInt  code, sizeId, listId;
  Bool scalingListPredModeFlag;
  READ_FLAG( code, "scaling_list_present_flag" );
  scalingList->setScalingListPresentFlag ( (code==1)?true:false );
  if(scalingList->getScalingListPresentFlag() == false)
  {
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
          scalingList->setRefMatrixId (sizeId,listId,(UInt)((Int)(listId)-(code+1)));
#if SCALING_LIST
          if( sizeId > SCALING_LIST_8x8 )
          {
            scalingList->setScalingListDC(sizeId,listId,scalingList->getScalingListDC(sizeId, scalingList->getRefMatrixId (sizeId,listId)));
          }
#endif
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
#if SCALING_LIST
  Int i,coefNum = min(MAX_MATRIX_COEF_NUM,(Int)g_scalingListSize[sizeId]);
  Int data;
  Int scalingListDcCoefMinus8 = 0;
  Int nextCoef = SCALING_LIST_START_VALUE;
  UInt* scan  = g_auiFrameScanXY [ (sizeId == 0)? 1 : 2 ];
  Bool stopNow = false;
  Int *dst = scalingList->getScalingListAddress(sizeId, listId);

  scalingList->setUseDefaultScalingMatrixFlag(sizeId,listId,false);
  if( sizeId > SCALING_LIST_8x8 )
  {
    READ_SVLC( scalingListDcCoefMinus8, "scaling_list_dc_coef_minus8");
    scalingList->setScalingListDC(sizeId,listId,scalingListDcCoefMinus8 + 8);
    if(scalingListDcCoefMinus8 == -8)
    {
      scalingList->processDefaultMarix(sizeId,listId);
    }
  }

  if( !scalingList->getUseDefaultScalingMatrixFlag(sizeId,listId))
  {
    for(i = 0; i < coefNum && !stopNow ; i++)
    {
      READ_SVLC( data, "scaling_list_delta_coef");
      nextCoef = (nextCoef + data + 256 ) % 256;
      if(sizeId < SCALING_LIST_16x16)
      {
        if( i == 0 && nextCoef == 0 )
        {
          scalingList->processDefaultMarix(sizeId,listId);
          stopNow = true;
        }
      }
      if(!stopNow)
      {
        dst[scan[i]] = nextCoef;
      }
    }
  }
#else
  Int i,coefNum = g_scalingListSize[sizeId];
  Int data;
  Int nextCoef = SCALING_LIST_START_VALUE;
  UInt* scan  = g_auiFrameScanXY [ sizeId + 1 ];
  Int *dst = scalingList->getScalingListAddress(sizeId, listId);

  for(i = 0; i < coefNum; i++)
  {
    READ_SVLC( data, "scaling_list_delta_coef");
    nextCoef = (nextCoef + data + 256 ) % 256;
    dst[scan[i]] = nextCoef;
  }
#endif
}

Void TDecCavlc::parseDFFlag(UInt& ruiVal, const Char *pSymbolName)
{
  READ_FLAG(ruiVal, pSymbolName);
}
Void TDecCavlc::parseDFSvlc(Int&  riVal, const Char *pSymbolName)
{
  READ_SVLC(riVal, pSymbolName);
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
