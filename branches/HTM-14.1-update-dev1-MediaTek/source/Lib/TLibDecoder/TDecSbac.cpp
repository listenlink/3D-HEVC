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

/** \file     TDecSbac.cpp
    \brief    Context-adaptive entropy decoder class
*/

#include "TDecSbac.h"
#include "TLibCommon/TComTU.h"
#include "TLibCommon/TComTrQuant.h"

#if RExt__DECODER_DEBUG_BIT_STATISTICS
#include "TLibCommon/TComCodingStatistics.h"
//
#define RExt__DECODER_DEBUG_BIT_STATISTICS_PASS_OPT_ARG(a) , a
#else
#define RExt__DECODER_DEBUG_BIT_STATISTICS_PASS_OPT_ARG(a)
#endif

//! \ingroup TLibDecoder
//! \{

#if ENVIRONMENT_VARIABLE_DEBUG_AND_TEST
#include "../TLibCommon/Debug.h"
#endif


//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

TDecSbac::TDecSbac()
// new structure here
: m_pcBitstream                              ( 0 )
, m_pcTDecBinIf                              ( NULL )
, m_numContextModels                         ( 0 )
, m_cCUSplitFlagSCModel                      ( 1,             1,                      NUM_SPLIT_FLAG_CTX                   , m_contextModels + m_numContextModels, m_numContextModels)
, m_cCUSkipFlagSCModel                       ( 1,             1,                      NUM_SKIP_FLAG_CTX                    , m_contextModels + m_numContextModels, m_numContextModels)
#if H_3D
, m_cCUDISFlagSCModel         ( 1,             1,               NUM_DIS_FLAG_CTX             , m_contextModels + m_numContextModels, m_numContextModels)
, m_cCUDISTypeSCModel         ( 1,             1,               NUM_DIS_TYPE_CTX             , m_contextModels + m_numContextModels, m_numContextModels)
#endif
, m_cCUMergeFlagExtSCModel                   ( 1,             1,                      NUM_MERGE_FLAG_EXT_CTX               , m_contextModels + m_numContextModels, m_numContextModels)
, m_cCUMergeIdxExtSCModel                    ( 1,             1,                      NUM_MERGE_IDX_EXT_CTX                , m_contextModels + m_numContextModels, m_numContextModels)
#if H_3D_ARP
, m_cCUPUARPWSCModel          ( 1,             1,               NUM_ARPW_CTX                  , m_contextModels + m_numContextModels, m_numContextModels)
#endif
#if H_3D_IC
, m_cCUICFlagSCModel          ( 1,             1,               NUM_IC_FLAG_CTX               , m_contextModels + m_numContextModels, m_numContextModels)
#endif
, m_cCUPartSizeSCModel                       ( 1,             1,                      NUM_PART_SIZE_CTX                    , m_contextModels + m_numContextModels, m_numContextModels)
, m_cCUPredModeSCModel                       ( 1,             1,                      NUM_PRED_MODE_CTX                    , m_contextModels + m_numContextModels, m_numContextModels)
, m_cCUIntraPredSCModel                      ( 1,             1,                      NUM_INTRA_PREDICT_CTX                , m_contextModels + m_numContextModels, m_numContextModels)
, m_cCUChromaPredSCModel                     ( 1,             1,                      NUM_CHROMA_PRED_CTX                  , m_contextModels + m_numContextModels, m_numContextModels)
, m_cCUDeltaQpSCModel                        ( 1,             1,                      NUM_DELTA_QP_CTX                     , m_contextModels + m_numContextModels, m_numContextModels)
, m_cCUInterDirSCModel                       ( 1,             1,                      NUM_INTER_DIR_CTX                    , m_contextModels + m_numContextModels, m_numContextModels)
, m_cCURefPicSCModel                         ( 1,             1,                      NUM_REF_NO_CTX                       , m_contextModels + m_numContextModels, m_numContextModels)
, m_cCUMvdSCModel                            ( 1,             1,                      NUM_MV_RES_CTX                       , m_contextModels + m_numContextModels, m_numContextModels)
, m_cCUQtCbfSCModel                          ( 1,             NUM_QT_CBF_CTX_SETS,    NUM_QT_CBF_CTX_PER_SET               , m_contextModels + m_numContextModels, m_numContextModels)
, m_cCUTransSubdivFlagSCModel                ( 1,             1,                      NUM_TRANS_SUBDIV_FLAG_CTX            , m_contextModels + m_numContextModels, m_numContextModels)
, m_cCUQtRootCbfSCModel                      ( 1,             1,                      NUM_QT_ROOT_CBF_CTX                  , m_contextModels + m_numContextModels, m_numContextModels)
, m_cCUSigCoeffGroupSCModel                  ( 1,             2,                      NUM_SIG_CG_FLAG_CTX                  , m_contextModels + m_numContextModels, m_numContextModels)
, m_cCUSigSCModel                            ( 1,             1,                      NUM_SIG_FLAG_CTX                     , m_contextModels + m_numContextModels, m_numContextModels)
, m_cCuCtxLastX                              ( 1,             NUM_CTX_LAST_FLAG_SETS, NUM_CTX_LAST_FLAG_XY                 , m_contextModels + m_numContextModels, m_numContextModels)
, m_cCuCtxLastY                              ( 1,             NUM_CTX_LAST_FLAG_SETS, NUM_CTX_LAST_FLAG_XY                 , m_contextModels + m_numContextModels, m_numContextModels)
, m_cCUOneSCModel                            ( 1,             1,                      NUM_ONE_FLAG_CTX                     , m_contextModels + m_numContextModels, m_numContextModels)
, m_cCUAbsSCModel                            ( 1,             1,                      NUM_ABS_FLAG_CTX                     , m_contextModels + m_numContextModels, m_numContextModels)
, m_cMVPIdxSCModel                           ( 1,             1,                      NUM_MVP_IDX_CTX                      , m_contextModels + m_numContextModels, m_numContextModels)
, m_cSaoMergeSCModel                         ( 1,             1,                      NUM_SAO_MERGE_FLAG_CTX               , m_contextModels + m_numContextModels, m_numContextModels)
, m_cSaoTypeIdxSCModel                       ( 1,             1,                      NUM_SAO_TYPE_IDX_CTX                 , m_contextModels + m_numContextModels, m_numContextModels)
, m_cTransformSkipSCModel                    ( 1,             MAX_NUM_CHANNEL_TYPE,   NUM_TRANSFORMSKIP_FLAG_CTX           , m_contextModels + m_numContextModels, m_numContextModels)
, m_CUTransquantBypassFlagSCModel            ( 1,             1,                      NUM_CU_TRANSQUANT_BYPASS_FLAG_CTX    , m_contextModels + m_numContextModels, m_numContextModels)
, m_explicitRdpcmFlagSCModel                 ( 1,             MAX_NUM_CHANNEL_TYPE,   NUM_EXPLICIT_RDPCM_FLAG_CTX          , m_contextModels + m_numContextModels, m_numContextModels)
, m_explicitRdpcmDirSCModel                  ( 1,             MAX_NUM_CHANNEL_TYPE,   NUM_EXPLICIT_RDPCM_DIR_CTX           , m_contextModels + m_numContextModels, m_numContextModels)
, m_cCrossComponentPredictionSCModel         ( 1,             1,                      NUM_CROSS_COMPONENT_PREDICTION_CTX   , m_contextModels + m_numContextModels, m_numContextModels)
, m_ChromaQpAdjFlagSCModel                   ( 1,             1,                      NUM_CHROMA_QP_ADJ_FLAG_CTX           , m_contextModels + m_numContextModels, m_numContextModels)
, m_ChromaQpAdjIdcSCModel                    ( 1,             1,                      NUM_CHROMA_QP_ADJ_IDC_CTX            , m_contextModels + m_numContextModels, m_numContextModels)

#if NH_3D_DMM
, m_cNotDmmFlagSCModel                       ( 1,             1,                      NUM_NOTDMM_FLAG_CTX                  , m_contextModels + m_numContextModels, m_numContextModels)
, m_cDmmModeSCModel                          ( 1,             1,                      NUM_DMM_MODE_CTX                     , m_contextModels + m_numContextModels, m_numContextModels)
#endif
#if NH_3D_DMM || NH_3D_SDC_INTRA
, m_cDdcDataSCModel                          ( 1,             1,                      NUM_DDC_DATA_CTX                     , m_contextModels + m_numContextModels, m_numContextModels)
#endif
#if NH_3D_SDC_INTRA
, m_cSDCResidualFlagSCModel                  ( 1,             1,                     SDC_NUM_RESIDUAL_FLAG_CTX             , m_contextModels + m_numContextModels, m_numContextModels)
, m_cSDCResidualSCModel                      ( 1,             1,                     SDC_NUM_RESIDUAL_CTX                  , m_contextModels + m_numContextModels, m_numContextModels)
, m_cSDCFlagSCModel                          ( 1,             1,                     NUM_SDC_FLAG_CTX                      , m_contextModels + m_numContextModels, m_numContextModels)
, m_cDdcFlagSCModel                          ( 1,             1,                     NUM_DDC_FLAG_CTX                      , m_contextModels + m_numContextModels, m_numContextModels)
#endif
#if H_3D_DBBP
, m_cDBBPFlagSCModel                         ( 1,             1,                     DBBP_NUM_FLAG_CTX                     , m_contextModels + m_numContextModels, m_numContextModels)
#endif

{
  assert( m_numContextModels <= MAX_NUM_CTX_MOD );
}

TDecSbac::~TDecSbac()
{
}

// ====================================================================================================================
// Public member functions
// ====================================================================================================================

Void TDecSbac::resetEntropy(TComSlice* pSlice)
{
  SliceType sliceType  = pSlice->getSliceType();
  Int       qp         = pSlice->getSliceQp();

  if (pSlice->getPPS()->getCabacInitPresentFlag() && pSlice->getCabacInitFlag())
  {
    switch (sliceType)
    {
    case P_SLICE:           // change initialization table to B_SLICE initialization
      sliceType = B_SLICE;
      break;
    case B_SLICE:           // change initialization table to P_SLICE initialization
      sliceType = P_SLICE;
      break;
    default     :           // should not occur
      assert(0);
      break;
    }
  }

  m_cCUSplitFlagSCModel.initBuffer                ( sliceType, qp, (UChar*)INIT_SPLIT_FLAG );
  m_cCUSkipFlagSCModel.initBuffer                 ( sliceType, qp, (UChar*)INIT_SKIP_FLAG );
#if H_3D
  m_cCUDISFlagSCModel.initBuffer         ( sliceType, qp, (UChar*)INIT_DIS_FLAG );
  m_cCUDISTypeSCModel.initBuffer         ( sliceType, qp, (UChar*)INIT_DIS_TYPE );
#endif
  m_cCUMergeFlagExtSCModel.initBuffer             ( sliceType, qp, (UChar*)INIT_MERGE_FLAG_EXT );
  m_cCUMergeIdxExtSCModel.initBuffer              ( sliceType, qp, (UChar*)INIT_MERGE_IDX_EXT );
#if H_3D_ARP
  m_cCUPUARPWSCModel.initBuffer          ( sliceType, qp, (UChar*)INIT_ARPW );
#endif
#if H_3D_IC
  m_cCUICFlagSCModel.initBuffer          ( sliceType, qp, (UChar*)INIT_IC_FLAG );
#endif
  m_cCUPartSizeSCModel.initBuffer                 ( sliceType, qp, (UChar*)INIT_PART_SIZE );
  m_cCUPredModeSCModel.initBuffer                 ( sliceType, qp, (UChar*)INIT_PRED_MODE );
  m_cCUIntraPredSCModel.initBuffer                ( sliceType, qp, (UChar*)INIT_INTRA_PRED_MODE );
  m_cCUChromaPredSCModel.initBuffer               ( sliceType, qp, (UChar*)INIT_CHROMA_PRED_MODE );
  m_cCUInterDirSCModel.initBuffer                 ( sliceType, qp, (UChar*)INIT_INTER_DIR );
  m_cCUMvdSCModel.initBuffer                      ( sliceType, qp, (UChar*)INIT_MVD );
  m_cCURefPicSCModel.initBuffer                   ( sliceType, qp, (UChar*)INIT_REF_PIC );
  m_cCUDeltaQpSCModel.initBuffer                  ( sliceType, qp, (UChar*)INIT_DQP );
  m_cCUQtCbfSCModel.initBuffer                    ( sliceType, qp, (UChar*)INIT_QT_CBF );
  m_cCUQtRootCbfSCModel.initBuffer                ( sliceType, qp, (UChar*)INIT_QT_ROOT_CBF );
  m_cCUSigCoeffGroupSCModel.initBuffer            ( sliceType, qp, (UChar*)INIT_SIG_CG_FLAG );
  m_cCUSigSCModel.initBuffer                      ( sliceType, qp, (UChar*)INIT_SIG_FLAG );
  m_cCuCtxLastX.initBuffer                        ( sliceType, qp, (UChar*)INIT_LAST );
  m_cCuCtxLastY.initBuffer                        ( sliceType, qp, (UChar*)INIT_LAST );
  m_cCUOneSCModel.initBuffer                      ( sliceType, qp, (UChar*)INIT_ONE_FLAG );
  m_cCUAbsSCModel.initBuffer                      ( sliceType, qp, (UChar*)INIT_ABS_FLAG );
  m_cMVPIdxSCModel.initBuffer                     ( sliceType, qp, (UChar*)INIT_MVP_IDX );
  m_cSaoMergeSCModel.initBuffer                   ( sliceType, qp, (UChar*)INIT_SAO_MERGE_FLAG );
  m_cSaoTypeIdxSCModel.initBuffer                 ( sliceType, qp, (UChar*)INIT_SAO_TYPE_IDX );
  m_cCUTransSubdivFlagSCModel.initBuffer          ( sliceType, qp, (UChar*)INIT_TRANS_SUBDIV_FLAG );
  m_cTransformSkipSCModel.initBuffer              ( sliceType, qp, (UChar*)INIT_TRANSFORMSKIP_FLAG );
  m_CUTransquantBypassFlagSCModel.initBuffer      ( sliceType, qp, (UChar*)INIT_CU_TRANSQUANT_BYPASS_FLAG );
  m_explicitRdpcmFlagSCModel.initBuffer           ( sliceType, qp, (UChar*)INIT_EXPLICIT_RDPCM_FLAG);
  m_explicitRdpcmDirSCModel.initBuffer            ( sliceType, qp, (UChar*)INIT_EXPLICIT_RDPCM_DIR);
  m_cCrossComponentPredictionSCModel.initBuffer   ( sliceType, qp, (UChar*)INIT_CROSS_COMPONENT_PREDICTION );
  m_ChromaQpAdjFlagSCModel.initBuffer             ( sliceType, qp, (UChar*)INIT_CHROMA_QP_ADJ_FLAG );
  m_ChromaQpAdjIdcSCModel.initBuffer              ( sliceType, qp, (UChar*)INIT_CHROMA_QP_ADJ_IDC );
#if NH_3D_DMM
  m_cNotDmmFlagSCModel.initBuffer                 ( sliceType, qp, (UChar*)INIT_NOTDMM_FLAG );
  m_cDmmModeSCModel.initBuffer                    ( sliceType, qp, (UChar*)INIT_DMM_MODE );
#endif
#if NH_3D_DMM || NH_3D_SDC_INTRA
  m_cDdcDataSCModel.initBuffer                    ( sliceType, qp, (UChar*)INIT_DDC_DATA );
#endif
#if NH_3D_SDC_INTRA
  m_cSDCResidualFlagSCModel.initBuffer            ( sliceType, qp, (UChar*)INIT_SDC_RESIDUAL_FLAG );
  m_cSDCResidualSCModel.initBuffer                ( sliceType, qp, (UChar*)INIT_SDC_RESIDUAL );
  m_cSDCFlagSCModel.initBuffer                    ( sliceType, qp, (UChar*)INIT_SDC_FLAG );
  m_cDdcFlagSCModel.initBuffer                    ( sliceType, qp, (UChar*)INIT_DDC_FLAG );
#endif
#if H_3D_DBBP
  m_cDBBPFlagSCModel.initBuffer                   ( sliceType, qp, (UChar*)INIT_DBBP_FLAG );
#endif

  for (UInt statisticIndex = 0; statisticIndex < RExt__GOLOMB_RICE_ADAPTATION_STATISTICS_SETS ; statisticIndex++)
  {
    m_golombRiceAdaptationStatistics[statisticIndex] = 0;
  }

  m_pcTDecBinIf->start();
}

Void TDecSbac::parseTerminatingBit( UInt& ruiBit )
{
  m_pcTDecBinIf->decodeBinTrm( ruiBit );
  if ( ruiBit == 1 )
  {
    m_pcTDecBinIf->finish();

#if RExt__DECODER_DEBUG_BIT_STATISTICS
    TComCodingStatistics::IncrementStatisticEP(STATS__TRAILING_BITS, m_pcBitstream->readOutTrailingBits(),0);
#else
    m_pcBitstream->readOutTrailingBits();
#endif
  }
}

#if H_3D
  m_cCUDISFlagSCModel.initBuffer         ( eSliceType, iQp, (UChar*)INIT_DIS_FLAG );
  m_cCUDISTypeSCModel.initBuffer         ( eSliceType, iQp, (UChar*)INIT_DIS_TYPE );
#endif
#if H_3D_ARP
  m_cCUPUARPWSCModel.initBuffer          ( eSliceType, iQp, (UChar*)INIT_ARPW );
#endif
#if H_3D_IC
  m_cCUICFlagSCModel.initBuffer          ( eSliceType, iQp, (UChar*)INIT_IC_FLAG );
#endif
#if H_3D_DBBP
  m_cDBBPFlagSCModel.initBuffer              ( eSliceType, iQp, (UChar*)INIT_DBBP_FLAG );
#endif


Void TDecSbac::parseRemainingBytes( Bool noTrailingBytesExpected )
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

#if RExt__DECODER_DEBUG_BIT_STATISTICS
Void TDecSbac::xReadUnaryMaxSymbol( UInt& ruiSymbol, ContextModel* pcSCModel, Int iOffset, UInt uiMaxSymbol, const class TComCodingStatisticsClassType &whichStat )
#else
Void TDecSbac::xReadUnaryMaxSymbol( UInt& ruiSymbol, ContextModel* pcSCModel, Int iOffset, UInt uiMaxSymbol )
#endif
{
  if (uiMaxSymbol == 0)
  {
    ruiSymbol = 0;
    return;
  }

  m_pcTDecBinIf->decodeBin( ruiSymbol, pcSCModel[0] RExt__DECODER_DEBUG_BIT_STATISTICS_PASS_OPT_ARG(whichStat) );

  if( ruiSymbol == 0 || uiMaxSymbol == 1 )
  {
    return;
  }

  UInt uiSymbol = 0;
  UInt uiCont;

  do
  {
    m_pcTDecBinIf->decodeBin( uiCont, pcSCModel[ iOffset ] RExt__DECODER_DEBUG_BIT_STATISTICS_PASS_OPT_ARG(whichStat) );
    uiSymbol++;
  } while( uiCont && ( uiSymbol < uiMaxSymbol - 1 ) );

  if( uiCont && ( uiSymbol == uiMaxSymbol - 1 ) )
  {
    uiSymbol++;
  }

  ruiSymbol = uiSymbol;
}

#if RExt__DECODER_DEBUG_BIT_STATISTICS
Void TDecSbac::xReadEpExGolomb( UInt& ruiSymbol, UInt uiCount, const class TComCodingStatisticsClassType &whichStat )
#else
Void TDecSbac::xReadEpExGolomb( UInt& ruiSymbol, UInt uiCount )
#endif
{
  UInt uiSymbol = 0;
  UInt uiBit = 1;

  while( uiBit )
  {
    m_pcTDecBinIf->decodeBinEP( uiBit RExt__DECODER_DEBUG_BIT_STATISTICS_PASS_OPT_ARG(whichStat) );
    uiSymbol += uiBit << uiCount++;
  }

  if ( --uiCount )
  {
    UInt bins;
    m_pcTDecBinIf->decodeBinsEP( bins, uiCount RExt__DECODER_DEBUG_BIT_STATISTICS_PASS_OPT_ARG(whichStat) );
    uiSymbol += bins;
  }

  ruiSymbol = uiSymbol;
}

#if RExt__DECODER_DEBUG_BIT_STATISTICS
Void TDecSbac::xReadUnarySymbol( UInt& ruiSymbol, ContextModel* pcSCModel, Int iOffset, const class TComCodingStatisticsClassType &whichStat )
#else
Void TDecSbac::xReadUnarySymbol( UInt& ruiSymbol, ContextModel* pcSCModel, Int iOffset )
#endif
{
  m_pcTDecBinIf->decodeBin( ruiSymbol, pcSCModel[0] RExt__DECODER_DEBUG_BIT_STATISTICS_PASS_OPT_ARG(whichStat));

  if( !ruiSymbol )
  {
    return;
  }

  UInt uiSymbol = 0;
  UInt uiCont;

  do
  {
    m_pcTDecBinIf->decodeBin( uiCont, pcSCModel[ iOffset ] RExt__DECODER_DEBUG_BIT_STATISTICS_PASS_OPT_ARG(whichStat));
    uiSymbol++;
  } while( uiCont );

  ruiSymbol = uiSymbol;
}


/** Parsing of coeff_abs_level_remaing
 * \param rSymbol                 reference to coeff_abs_level_remaing
 * \param rParam                  reference to parameter
 * \param useLimitedPrefixLength
 * \param maxLog2TrDynamicRange
 */
#if RExt__DECODER_DEBUG_BIT_STATISTICS
Void TDecSbac::xReadCoefRemainExGolomb ( UInt &rSymbol, UInt &rParam, const Bool useLimitedPrefixLength, const Int maxLog2TrDynamicRange, const class TComCodingStatisticsClassType &whichStat )
#else
Void TDecSbac::xReadCoefRemainExGolomb ( UInt &rSymbol, UInt &rParam, const Bool useLimitedPrefixLength, const Int maxLog2TrDynamicRange )
#endif
{
  UInt prefix   = 0;
  UInt codeWord = 0;

  if (useLimitedPrefixLength)
  {
    const UInt longestPossiblePrefix = (32 - (COEF_REMAIN_BIN_REDUCTION + maxLog2TrDynamicRange)) + COEF_REMAIN_BIN_REDUCTION;

    do
    {
      prefix++;
      m_pcTDecBinIf->decodeBinEP( codeWord RExt__DECODER_DEBUG_BIT_STATISTICS_PASS_OPT_ARG(whichStat) );
    } while((codeWord != 0) && (prefix < longestPossiblePrefix));
  }
  else
  {
    do
    {
      prefix++;
      m_pcTDecBinIf->decodeBinEP( codeWord RExt__DECODER_DEBUG_BIT_STATISTICS_PASS_OPT_ARG(whichStat) );
    } while( codeWord);
  }

  codeWord  = 1 - codeWord;
  prefix -= codeWord;
  codeWord=0;

  if (prefix < COEF_REMAIN_BIN_REDUCTION )
  {
    m_pcTDecBinIf->decodeBinsEP(codeWord,rParam RExt__DECODER_DEBUG_BIT_STATISTICS_PASS_OPT_ARG(whichStat));
    rSymbol = (prefix<<rParam) + codeWord;
  }
  else if (useLimitedPrefixLength)
  {
    const UInt maximumPrefixLength = (32 - (COEF_REMAIN_BIN_REDUCTION + maxLog2TrDynamicRange));

    const UInt prefixLength = prefix - COEF_REMAIN_BIN_REDUCTION;
    const UInt suffixLength = (prefixLength == maximumPrefixLength) ? (maxLog2TrDynamicRange - rParam) : prefixLength;

    m_pcTDecBinIf->decodeBinsEP(codeWord, (suffixLength + rParam) RExt__DECODER_DEBUG_BIT_STATISTICS_PASS_OPT_ARG(whichStat));

    rSymbol = codeWord + ((((1 << prefixLength) - 1) + COEF_REMAIN_BIN_REDUCTION) << rParam);
  }
  else
  {
    m_pcTDecBinIf->decodeBinsEP(codeWord,prefix-COEF_REMAIN_BIN_REDUCTION+rParam RExt__DECODER_DEBUG_BIT_STATISTICS_PASS_OPT_ARG(whichStat));
    rSymbol = (((1<<(prefix-COEF_REMAIN_BIN_REDUCTION))+COEF_REMAIN_BIN_REDUCTION-1)<<rParam)+codeWord;
  }
}

/** Parse I_PCM information.
 * \param pcCU
 * \param uiAbsPartIdx
 * \param uiDepth
 * \returns Void
 *
 * If I_PCM flag indicates that the CU is I_PCM, parse its PCM alignment bits and codes.
 */
Void TDecSbac::parseIPCMInfo ( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth )
{
  UInt uiSymbol;

  m_pcTDecBinIf->decodeBinTrm(uiSymbol);

#if H_MV_ENC_DEC_TRAC
  DTRACE_CU("pcm_flag", uiSymbol)
#endif


  if (uiSymbol == 1)
  {
    Bool bIpcmFlag = true;
    const TComSPS &sps=*(pcCU->getSlice()->getSPS());

    pcCU->setPartSizeSubParts  ( SIZE_2Nx2N, uiAbsPartIdx, uiDepth );
    pcCU->setSizeSubParts      ( sps.getMaxCUWidth()>>uiDepth, sps.getMaxCUHeight()>>uiDepth, uiAbsPartIdx, uiDepth );
    pcCU->setTrIdxSubParts     ( 0, uiAbsPartIdx, uiDepth );
    pcCU->setIPCMFlagSubParts  ( bIpcmFlag, uiAbsPartIdx, uiDepth );

    const UInt minCoeffSizeY = pcCU->getPic()->getMinCUWidth() * pcCU->getPic()->getMinCUHeight();
    const UInt offsetY       = minCoeffSizeY * uiAbsPartIdx;
    for (UInt ch=0; ch < pcCU->getPic()->getNumberValidComponents(); ch++)
    {
      const ComponentID compID = ComponentID(ch);
      const UInt offset = offsetY >> (pcCU->getPic()->getComponentScaleX(compID) + pcCU->getPic()->getComponentScaleY(compID));
      Pel * pPCMSample  = pcCU->getPCMSample(compID) + offset;
      const UInt width  = pcCU->getWidth (uiAbsPartIdx) >> pcCU->getPic()->getComponentScaleX(compID);
      const UInt height = pcCU->getHeight(uiAbsPartIdx) >> pcCU->getPic()->getComponentScaleY(compID);
      const UInt sampleBits = pcCU->getSlice()->getSPS()->getPCMBitDepth(toChannelType(compID));
      for (UInt y=0; y<height; y++)
      {
        for (UInt x=0; x<width; x++)
        {
          UInt sample;
          m_pcTDecBinIf->xReadPCMCode(sampleBits, sample);
          pPCMSample[x] = sample;
        }
        pPCMSample += width;
      }
    }
    m_pcTDecBinIf->start();
  }
}

Void TDecSbac::parseCUTransquantBypassFlag( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth )
{
  UInt uiSymbol;
  m_pcTDecBinIf->decodeBin( uiSymbol, m_CUTransquantBypassFlagSCModel.get( 0, 0, 0 ) RExt__DECODER_DEBUG_BIT_STATISTICS_PASS_OPT_ARG(STATS__CABAC_BITS__TQ_BYPASS_FLAG) );
#if H_MV_ENC_DEC_TRAC
  DTRACE_CU("cu_transquant_bypass_flag", uiSymbol); 
#endif

  pcCU->setCUTransquantBypassSubParts(uiSymbol ? true : false, uiAbsPartIdx, uiDepth);
}

/** parse skip flag
 * \param pcCU
 * \param uiAbsPartIdx
 * \param uiDepth
 * \returns Void
 */
Void TDecSbac::parseSkipFlag( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth )
{
  if( pcCU->getSlice()->isIntra() )
  {
    return;
  }

  UInt uiSymbol = 0;
  UInt uiCtxSkip = pcCU->getCtxSkipFlag( uiAbsPartIdx );
  m_pcTDecBinIf->decodeBin( uiSymbol, m_cCUSkipFlagSCModel.get( 0, 0, uiCtxSkip ) RExt__DECODER_DEBUG_BIT_STATISTICS_PASS_OPT_ARG(STATS__CABAC_BITS__SKIP_FLAG) );
#if !H_MV_ENC_DEC_TRAC
  DTRACE_CABAC_VL( g_nSymbolCounter++ );
  DTRACE_CABAC_T( "\tSkipFlag" );
  DTRACE_CABAC_T( "\tuiCtxSkip: ");
  DTRACE_CABAC_V( uiCtxSkip );
  DTRACE_CABAC_T( "\tuiSymbol: ");
  DTRACE_CABAC_V( uiSymbol );
  DTRACE_CABAC_T( "\n");
#endif
  if( uiSymbol )
  {
    pcCU->setSkipFlagSubParts( true,        uiAbsPartIdx, uiDepth );
    pcCU->setPredModeSubParts( MODE_INTER,  uiAbsPartIdx, uiDepth );
    pcCU->setPartSizeSubParts( SIZE_2Nx2N, uiAbsPartIdx, uiDepth );
    pcCU->setSizeSubParts( pcCU->getSlice()->getSPS()->getMaxCUWidth()>>uiDepth, pcCU->getSlice()->getSPS()->getMaxCUHeight()>>uiDepth, uiAbsPartIdx, uiDepth );
    pcCU->setMergeFlagSubParts( true , uiAbsPartIdx, 0, uiDepth );
  }
#if H_MV_ENC_DEC_TRAC
  DTRACE_CU("cu_skip_flag", uiSymbol); 
#endif

}
#if H_3D
Void TDecSbac::parseDIS( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth )
{
  pcCU->setDISFlagSubParts( false,        uiAbsPartIdx, uiDepth );
  UInt uiSymbol = 0;
  m_pcTDecBinIf->decodeBin( uiSymbol, m_cCUDISFlagSCModel.get( 0, 0, 0 ) );
  if( uiSymbol )
  {
    pcCU->setDISFlagSubParts( true,        uiAbsPartIdx, uiDepth );
    pcCU->setSkipFlagSubParts( false,        uiAbsPartIdx, uiDepth );
    pcCU->setSDCFlagSubParts( false,        uiAbsPartIdx, uiDepth );
    pcCU->setPredModeSubParts( MODE_INTRA,  uiAbsPartIdx, uiDepth );
    pcCU->setPartSizeSubParts( SIZE_2Nx2N, uiAbsPartIdx, uiDepth );
    pcCU->setLumaIntraDirSubParts (DC_IDX, uiAbsPartIdx, uiDepth );
    pcCU->setSizeSubParts( g_uiMaxCUWidth>>uiDepth, g_uiMaxCUHeight>>uiDepth, uiAbsPartIdx, uiDepth );
    pcCU->setMergeFlagSubParts( false , uiAbsPartIdx, 0, uiDepth );
    pcCU->setTrIdxSubParts(0, uiAbsPartIdx, uiDepth);
    pcCU->setCbfSubParts(0, 1, 1, uiAbsPartIdx, uiDepth);

    UInt uiUnaryIdx = 0;
    UInt uiNumCand  = 4;

    if ( uiNumCand > 1 )
    {
      for( ; uiUnaryIdx < uiNumCand - 1; ++uiUnaryIdx )
      {
        UInt uiSymbol2 = 0;
        if ( uiUnaryIdx==0 )
        {
          m_pcTDecBinIf->decodeBin( uiSymbol2, m_cCUDISTypeSCModel.get( 0, 0, 0 ) );
        }
        else
        {
          m_pcTDecBinIf->decodeBinEP( uiSymbol2);
        }
        if( uiSymbol2 == 0 )
        {
          break;
        }
      }
    }
    pcCU->setDISTypeSubParts(uiUnaryIdx, uiAbsPartIdx, 0, uiDepth);
  }
}
#endif


/** parse merge flag
 * \param pcCU
 * \param uiAbsPartIdx
 * \param uiDepth
 * \param uiPUIdx
 * \returns Void
 */
Void TDecSbac::parseMergeFlag ( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth, UInt uiPUIdx )
{
  UInt uiSymbol;
  m_pcTDecBinIf->decodeBin( uiSymbol, *m_cCUMergeFlagExtSCModel.get( 0 ) RExt__DECODER_DEBUG_BIT_STATISTICS_PASS_OPT_ARG(STATS__CABAC_BITS__MERGE_FLAG) );
#if H_MV_ENC_DEC_TRAC
  DTRACE_PU("merge_flag", uiSymbol)
#endif
  pcCU->setMergeFlagSubParts( uiSymbol ? true : false, uiAbsPartIdx, uiPUIdx, uiDepth );
#if !H_MV_ENC_DEC_TRAC
  DTRACE_CABAC_VL( g_nSymbolCounter++ );
  DTRACE_CABAC_T( "\tMergeFlag: " );
  DTRACE_CABAC_V( uiSymbol );
  DTRACE_CABAC_T( "\tAddress: " );
  DTRACE_CABAC_V( pcCU->getCtuRsAddr() );
  DTRACE_CABAC_T( "\tuiAbsPartIdx: " );
  DTRACE_CABAC_V( uiAbsPartIdx );
  DTRACE_CABAC_T( "\n" );
#endif
}

Void TDecSbac::parseMergeIndex ( TComDataCU* pcCU, UInt& ruiMergeIndex )
{
  UInt uiUnaryIdx = 0;
  UInt uiNumCand = pcCU->getSlice()->getMaxNumMergeCand();
  if ( uiNumCand > 1 )
  {
    for( ; uiUnaryIdx < uiNumCand - 1; ++uiUnaryIdx )
    {
      UInt uiSymbol = 0;
      if ( uiUnaryIdx==0 )
      {
        m_pcTDecBinIf->decodeBin( uiSymbol, m_cCUMergeIdxExtSCModel.get( 0, 0, 0 ) RExt__DECODER_DEBUG_BIT_STATISTICS_PASS_OPT_ARG(STATS__CABAC_BITS__MERGE_INDEX) );
      }
      else
      {
        m_pcTDecBinIf->decodeBinEP( uiSymbol RExt__DECODER_DEBUG_BIT_STATISTICS_PASS_OPT_ARG(STATS__CABAC_BITS__MERGE_INDEX) );
      }
      if( uiSymbol == 0 )
      {
        break;
      }
    }
#if H_MV_ENC_DEC_TRAC
    DTRACE_PU("merge_idx", uiUnaryIdx)
#endif
  }
  ruiMergeIndex = uiUnaryIdx;

#if !H_MV_ENC_DEC_TRAC
  DTRACE_CABAC_VL( g_nSymbolCounter++ )
  DTRACE_CABAC_T( "\tparseMergeIndex()" )
  DTRACE_CABAC_T( "\tuiMRGIdx= " )
  DTRACE_CABAC_V( ruiMergeIndex )
  DTRACE_CABAC_T( "\n" )
#endif
}

Void TDecSbac::parseMVPIdx      ( Int& riMVPIdx )
{
  UInt uiSymbol;
  xReadUnaryMaxSymbol(uiSymbol, m_cMVPIdxSCModel.get(0), 1, AMVP_MAX_NUM_CANDS-1 RExt__DECODER_DEBUG_BIT_STATISTICS_PASS_OPT_ARG(STATS__CABAC_BITS__MVP_IDX) );
  riMVPIdx = uiSymbol;
}

Void TDecSbac::parseSplitFlag     ( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth )
{
  if( uiDepth == pcCU->getSlice()->getSPS()->getLog2DiffMaxMinCodingBlockSize() )
  {
    pcCU->setDepthSubParts( uiDepth, uiAbsPartIdx );
    return;
  }
#if RExt__DECODER_DEBUG_BIT_STATISTICS
  const TComCodingStatisticsClassType ctype(STATS__CABAC_BITS__SPLIT_FLAG, g_aucConvertToBit[pcCU->getSlice()->getSPS()->getMaxCUWidth()>>uiDepth]+2);
#endif

  UInt uiSymbol;
#if H_3D_QTLPC
  Bool bParseSplitFlag    = true;
  Bool    bLimQtPredFlag = pcCU->getPic()->getSlice(0)->getQtPredFlag();
  TComPic *pcTexture      = pcCU->getSlice()->getTexturePic();
  Bool bDepthMapDetect    = (pcTexture != NULL);
  Bool bIntraSliceDetect  = (pcCU->getSlice()->getSliceType() == I_SLICE);

  Bool rapPic = (pcCU->getSlice()->getNalUnitType() == NAL_UNIT_CODED_SLICE_IDR_W_RADL || pcCU->getSlice()->getNalUnitType() == NAL_UNIT_CODED_SLICE_IDR_N_LP || pcCU->getSlice()->getNalUnitType() == NAL_UNIT_CODED_SLICE_CRA);

#if H_3D_FCO
  if(bDepthMapDetect && !bIntraSliceDetect && !rapPic && bLimQtPredFlag && pcTexture->getReconMark())
#else
  if(bDepthMapDetect && !bIntraSliceDetect && !rapPic && bLimQtPredFlag)
#endif
  {
    TComDataCU *pcTextureCU = pcTexture->getCU(pcCU->getAddr());
    assert(pcTextureCU->getDepth(uiAbsPartIdx) >= uiDepth);
    bParseSplitFlag         = (pcTextureCU->getDepth(uiAbsPartIdx) > uiDepth);
  }

  if(bParseSplitFlag)
  {
#endif
  m_pcTDecBinIf->decodeBin( uiSymbol, m_cCUSplitFlagSCModel.get( 0, 0, pcCU->getCtxSplitFlag( uiAbsPartIdx, uiDepth ) ) RExt__DECODER_DEBUG_BIT_STATISTICS_PASS_OPT_ARG(ctype) );
#if H_MV_ENC_DEC_TRAC
    DTRACE_CU("split_cu_flag", uiSymbol); 
#else
  DTRACE_CABAC_VL( g_nSymbolCounter++ )
  DTRACE_CABAC_T( "\tSplitFlag\n" )
#endif
#if H_3D_QTLPC
  }
  else
  {
    uiSymbol = 0;
  }
#endif
  pcCU->setDepthSubParts( uiDepth + uiSymbol, uiAbsPartIdx );

  return;
}

/** parse partition size
 * \param pcCU
 * \param uiAbsPartIdx
 * \param uiDepth
 * \returns Void
 */
Void TDecSbac::parsePartSize( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth )
{
  UInt uiSymbol, uiMode = 0;
  PartSize eMode;
  const UChar cuWidth =UChar(pcCU->getSlice()->getSPS()->getMaxCUWidth()>>uiDepth);
  const UChar cuHeight=UChar(pcCU->getSlice()->getSPS()->getMaxCUHeight()>>uiDepth);
  const Int log2DiffMaxMinCodingBlockSize = pcCU->getSlice()->getSPS()->getLog2DiffMaxMinCodingBlockSize();

#if RExt__DECODER_DEBUG_BIT_STATISTICS
  const TComCodingStatisticsClassType ctype(STATS__CABAC_BITS__PART_SIZE, g_aucConvertToBit[cuWidth]+2);
#endif

  assert ( pcCU->getSlice()->getSPS()->getLog2DiffMaxMinCodingBlockSize() == log2DiffMaxMinCodingBlockSize);

#if H_3D_QTLPC
  Bool bParsePartSize    = true;

  Bool    bLimQtPredFlag = pcCU->getPic()->getSlice(0)->getQtPredFlag();
  TComPic *pcTexture     = pcCU->getSlice()->getTexturePic();
  Bool bDepthMapDetect   = (pcTexture != NULL);
  Bool bIntraSliceDetect = (pcCU->getSlice()->getSliceType() == I_SLICE);

  Bool rapPic     = (pcCU->getSlice()->getNalUnitType() == NAL_UNIT_CODED_SLICE_IDR_W_RADL || pcCU->getSlice()->getNalUnitType() == NAL_UNIT_CODED_SLICE_IDR_N_LP || pcCU->getSlice()->getNalUnitType() == NAL_UNIT_CODED_SLICE_CRA);

  Bool depthDependent = false;
  UInt uiTexturePart = uiMode;

#if H_3D_FCO
  if(bDepthMapDetect && !bIntraSliceDetect && !rapPic && bLimQtPredFlag && pcTexture->getReconMark())
#else
  if(bDepthMapDetect && !bIntraSliceDetect && !rapPic && bLimQtPredFlag )
#endif
  {
    TComDataCU *pcTextureCU = pcTexture->getCU(pcCU->getAddr());
    assert(pcTextureCU->getDepth(uiAbsPartIdx) >= uiDepth);
    if(pcTextureCU->getDepth(uiAbsPartIdx) == uiDepth )
    {
      depthDependent = true;
      uiTexturePart = pcTextureCU->getPartitionSize( uiAbsPartIdx );
    }
    if (pcTextureCU->getDepth(uiAbsPartIdx) == uiDepth && pcTextureCU->getPartitionSize( uiAbsPartIdx ) == SIZE_2Nx2N)
    {
      bParsePartSize = false;
      eMode          = SIZE_2Nx2N;
    }
  }
#endif

  if ( pcCU->isIntra( uiAbsPartIdx ) )
  {
#if H_3D_QTLPC
    if(bParsePartSize)
    {
#endif
    uiSymbol = 1;
    if( uiDepth == log2DiffMaxMinCodingBlockSize )
    {
      m_pcTDecBinIf->decodeBin( uiSymbol, m_cCUPartSizeSCModel.get( 0, 0, 0) RExt__DECODER_DEBUG_BIT_STATISTICS_PASS_OPT_ARG(ctype) );
#if H_MV_ENC_DEC_TRAC          
        DTRACE_CU("part_mode", uiSymbol)
#endif        
    }
    eMode = uiSymbol ? SIZE_2Nx2N : SIZE_NxN;
#if H_3D_QTLPC
    }
#endif
    UInt uiTrLevel = 0;
    UInt uiWidthInBit  = g_aucConvertToBit[pcCU->getWidth(uiAbsPartIdx)]+2;
    UInt uiTrSizeInBit = g_aucConvertToBit[pcCU->getSlice()->getSPS()->getMaxTrSize()]+2;
    uiTrLevel          = uiWidthInBit >= uiTrSizeInBit ? uiWidthInBit - uiTrSizeInBit : 0;
    if( eMode == SIZE_NxN )
    {
      pcCU->setTrIdxSubParts( 1+uiTrLevel, uiAbsPartIdx, uiDepth );
    }
    else
    {
      pcCU->setTrIdxSubParts( uiTrLevel, uiAbsPartIdx, uiDepth );
    }
  }
  else
  {
#if H_3D_QTLPC
    if(bParsePartSize)
    {
      if (depthDependent==false || uiTexturePart == SIZE_NxN|| uiTexturePart == SIZE_2Nx2N)
      {
#endif
    UInt uiMaxNumBits = 2;

    if( uiDepth == log2DiffMaxMinCodingBlockSize && !( cuWidth == 8 && cuHeight == 8 ) )
    {
      uiMaxNumBits ++;
    }

    for ( UInt ui = 0; ui < uiMaxNumBits; ui++ )
    {
      m_pcTDecBinIf->decodeBin( uiSymbol, m_cCUPartSizeSCModel.get( 0, 0, ui) RExt__DECODER_DEBUG_BIT_STATISTICS_PASS_OPT_ARG(ctype) );
      if ( uiSymbol )
      {
        break;
      }
      uiMode++;
    }
    eMode = (PartSize) uiMode;
    if ( pcCU->getSlice()->getSPS()->getUseAMP() && uiDepth < log2DiffMaxMinCodingBlockSize )
    {
      if (eMode == SIZE_2NxN)
      {
        m_pcTDecBinIf->decodeBin(uiSymbol, m_cCUPartSizeSCModel.get( 0, 0, 3 ) RExt__DECODER_DEBUG_BIT_STATISTICS_PASS_OPT_ARG(ctype));
        if (uiSymbol == 0)
        {
          m_pcTDecBinIf->decodeBinEP(uiSymbol RExt__DECODER_DEBUG_BIT_STATISTICS_PASS_OPT_ARG(ctype) );
          eMode = (uiSymbol == 0? SIZE_2NxnU : SIZE_2NxnD);
        }
      }
      else if (eMode == SIZE_Nx2N)
      {
        m_pcTDecBinIf->decodeBin(uiSymbol, m_cCUPartSizeSCModel.get( 0, 0, 3 ) RExt__DECODER_DEBUG_BIT_STATISTICS_PASS_OPT_ARG(ctype) );
        if (uiSymbol == 0)
        {
          m_pcTDecBinIf->decodeBinEP(uiSymbol RExt__DECODER_DEBUG_BIT_STATISTICS_PASS_OPT_ARG(ctype) );
          eMode = (uiSymbol == 0? SIZE_nLx2N : SIZE_nRx2N);
        }
      }
    }
#if H_3D_QTLPC
      }
      else if(uiTexturePart == SIZE_2NxN || uiTexturePart == SIZE_2NxnU || uiTexturePart == SIZE_2NxnD)
      {
        UInt uiMaxNumBits = 1;
        if ( pcCU->getSlice()->getSPS()->getAMPAcc( uiDepth ) )
        {
          uiMaxNumBits ++;
        }
        for ( UInt ui = 0; ui < uiMaxNumBits; ui++ )
        {
          m_pcTDecBinIf->decodeBin( uiSymbol, m_cCUPartSizeSCModel.get( 0, 0, ui) );
          if ( uiSymbol )
          {
            break;
          }
          uiMode++;
        }
        eMode = (PartSize) uiMode;
        if(uiMode && pcCU->getSlice()->getSPS()->getAMPAcc( uiDepth ) && uiSymbol==1 )
        {
          eMode = SIZE_2NxN;
        }
        else if (uiMode && pcCU->getSlice()->getSPS()->getAMPAcc( uiDepth )==0  && uiSymbol==0)
        {
          eMode = SIZE_2NxN;
        }
        else if (uiMode && pcCU->getSlice()->getSPS()->getAMPAcc( uiDepth ) && uiSymbol==0)
        {
          m_pcTDecBinIf->decodeBinEP(uiSymbol);
          eMode = (uiSymbol == 0? SIZE_2NxnU : SIZE_2NxnD);
        }
      }
      else if(uiTexturePart == SIZE_Nx2N|| uiTexturePart==SIZE_nLx2N || uiTexturePart==SIZE_nRx2N)
      {
        UInt uiMaxNumBits = 1;
        if ( pcCU->getSlice()->getSPS()->getAMPAcc( uiDepth ) )
        {
          uiMaxNumBits ++;
        }
        for ( UInt ui = 0; ui < uiMaxNumBits; ui++ )
        {
          m_pcTDecBinIf->decodeBin( uiSymbol, m_cCUPartSizeSCModel.get( 0, 0, ui) );
          if ( uiSymbol )
          {
            break;
          }
          uiMode++;
        }
        eMode = (PartSize) uiMode;
        if(uiMode && pcCU->getSlice()->getSPS()->getAMPAcc( uiDepth ) && uiSymbol==1 )
        {
          eMode = SIZE_Nx2N;
        }
        else if (uiMode && pcCU->getSlice()->getSPS()->getAMPAcc( uiDepth )==0  && uiSymbol==0)
        {
          eMode = SIZE_Nx2N;
        }
        else if (uiMode && pcCU->getSlice()->getSPS()->getAMPAcc( uiDepth ) && uiSymbol==0)
        {
          m_pcTDecBinIf->decodeBinEP(uiSymbol);
          eMode = (uiSymbol == 0? SIZE_nLx2N : SIZE_nRx2N);
        }
      }
      else
      {
        assert(0);
      }
#endif
#if H_MV_ENC_DEC_TRAC          
      DTRACE_CU("part_mode", eMode )
#endif
#if H_3D_QTLPC
    }
#endif
  }
  pcCU->setPartSizeSubParts( eMode, uiAbsPartIdx, uiDepth );
  pcCU->setSizeSubParts( cuWidth, cuHeight, uiAbsPartIdx, uiDepth );
}


/** parse prediction mode
 * \param pcCU
 * \param uiAbsPartIdx
 * \param uiDepth
 * \returns Void
 */
Void TDecSbac::parsePredMode( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth )
{
  if( pcCU->getSlice()->isIntra() )
  {
    pcCU->setPredModeSubParts( MODE_INTRA, uiAbsPartIdx, uiDepth );
    return;
  }

  UInt uiSymbol;
  Int  iPredMode = MODE_INTER;
  m_pcTDecBinIf->decodeBin( uiSymbol, m_cCUPredModeSCModel.get( 0, 0, 0 ) RExt__DECODER_DEBUG_BIT_STATISTICS_PASS_OPT_ARG(STATS__CABAC_BITS__PRED_MODE) );
  iPredMode += uiSymbol;
#if H_MV_ENC_DEC_TRAC          
  DTRACE_CU("pred_mode_flag", uiSymbol)
#endif        
  pcCU->setPredModeSubParts( (PredMode)iPredMode, uiAbsPartIdx, uiDepth );
}


Void TDecSbac::parseIntraDirLumaAng  ( TComDataCU* pcCU, UInt absPartIdx, UInt depth )
{
  PartSize mode = pcCU->getPartitionSize( absPartIdx );
  UInt partNum = mode==SIZE_NxN?4:1;
  UInt partOffset = ( pcCU->getPic()->getNumPartitionsInCtu() >> ( pcCU->getDepth(absPartIdx) << 1 ) ) >> 2;
  UInt mpmPred[4],symbol;
  Int j,intraPredMode;
  if (mode==SIZE_NxN)
  {
    depth++;
  }
#if RExt__DECODER_DEBUG_BIT_STATISTICS
  const TComCodingStatisticsClassType ctype(STATS__CABAC_BITS__INTRA_DIR_ANG, g_aucConvertToBit[pcCU->getSlice()->getSPS()->getMaxCUWidth()>>depth]+2, CHANNEL_TYPE_LUMA);
#endif
  for (j=0;j<partNum;j++)
  {
#if NH_3D_DMM
    if( pcCU->getSlice()->getIntraSdcWedgeFlag() || pcCU->getSlice()->getIntraContourFlag() )
    {
      xParseIntraDepthMode( pcCU, absPartIdx+partOffset*j, depth );
      xParseDmmData       ( pcCU, absPartIdx+partOffset*j, depth );
    }
    if( pcCU->getIntraDir( CHANNEL_TYPE_LUMA, absPartIdx+partOffset*j ) < NUM_INTRA_MODE )
    {
#endif
    m_pcTDecBinIf->decodeBin( symbol, m_cCUIntraPredSCModel.get( 0, 0, 0) RExt__DECODER_DEBUG_BIT_STATISTICS_PASS_OPT_ARG(ctype) );
    mpmPred[j] = symbol;
#if H_MV_ENC_DEC_TRAC          
      DTRACE_CU("prev_intra_luma_pred_flag", symbol)
#endif
#if NH_3D_DMM
    }
#endif
  }
  for (j=0;j<partNum;j++)
  {
#if NH_3D_DMM
    if( pcCU->getIntraDir( CHANNEL_TYPE_LUMA, absPartIdx+partOffset*j ) < NUM_INTRA_MODE )
    {
#endif
    Int preds[NUM_MOST_PROBABLE_MODES] = {-1, -1, -1};
    pcCU->getIntraDirPredictor(absPartIdx+partOffset*j, preds, COMPONENT_Y);
    if (mpmPred[j])
    {
      m_pcTDecBinIf->decodeBinEP( symbol RExt__DECODER_DEBUG_BIT_STATISTICS_PASS_OPT_ARG(ctype) );
      if (symbol)
      {
        m_pcTDecBinIf->decodeBinEP( symbol RExt__DECODER_DEBUG_BIT_STATISTICS_PASS_OPT_ARG(ctype) );
        symbol++;
      }
#if H_MV_ENC_DEC_TRAC          
        DTRACE_CU("mpm_idx", symbol)
#endif
      intraPredMode = preds[symbol];
    }
    else
    {
      m_pcTDecBinIf->decodeBinsEP( symbol, 5 RExt__DECODER_DEBUG_BIT_STATISTICS_PASS_OPT_ARG(ctype) );
      intraPredMode = symbol;
#if H_MV_ENC_DEC_TRAC          
        DTRACE_CU("rem_intra_luma_pred_mode", symbol)
#endif        

      //postponed sorting of MPMs (only in remaining branch)
      if (preds[0] > preds[1])
      {
        std::swap(preds[0], preds[1]);
      }
      if (preds[0] > preds[2])
      {
        std::swap(preds[0], preds[2]);
      }
      if (preds[1] > preds[2])
      {
        std::swap(preds[1], preds[2]);
      }
      for ( UInt i = 0; i < NUM_MOST_PROBABLE_MODES; i++ )
      {
        intraPredMode += ( intraPredMode >= preds[i] );
      }
    }
    pcCU->setIntraDirSubParts(CHANNEL_TYPE_LUMA, (UChar)intraPredMode, absPartIdx+partOffset*j, depth );
#if NH_3D_DMM
    }
#endif
  }
}


Void TDecSbac::parseIntraDirChroma( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth )
{
  UInt uiSymbol;
#if RExt__DECODER_DEBUG_BIT_STATISTICS
  const TComCodingStatisticsClassType ctype(STATS__CABAC_BITS__INTRA_DIR_ANG, g_aucConvertToBit[pcCU->getSlice()->getSPS()->getMaxCUWidth()>>uiDepth]+2, CHANNEL_TYPE_CHROMA);
#endif

  m_pcTDecBinIf->decodeBin( uiSymbol, m_cCUChromaPredSCModel.get( 0, 0, 0 ) RExt__DECODER_DEBUG_BIT_STATISTICS_PASS_OPT_ARG(ctype) );
  if( uiSymbol == 0 )
  {
#if H_MV_ENC_DEC_TRAC          
    DTRACE_CU("intra_chroma_pred_mode", uiSymbol )
#endif        
    uiSymbol = DM_CHROMA_IDX;
  }
  else
  {
    UInt uiIPredMode;
    m_pcTDecBinIf->decodeBinsEP( uiIPredMode, 2 RExt__DECODER_DEBUG_BIT_STATISTICS_PASS_OPT_ARG(ctype) );
#if H_MV_ENC_DEC_TRAC          
      DTRACE_CU("intra_chroma_pred_mode", uiIPredMode )
#endif        
    UInt uiAllowedChromaDir[ NUM_CHROMA_MODE ];
    pcCU->getAllowedChromaDir( uiAbsPartIdx, uiAllowedChromaDir );
    uiSymbol = uiAllowedChromaDir[ uiIPredMode ];
  }

  pcCU->setIntraDirSubParts( CHANNEL_TYPE_CHROMA, uiSymbol, uiAbsPartIdx, uiDepth );
}

Void TDecSbac::parseInterDir( TComDataCU* pcCU, UInt& ruiInterDir, UInt uiAbsPartIdx )
{
  UInt uiSymbol;
  const UInt uiCtx = pcCU->getCtxInterDir( uiAbsPartIdx );
  ContextModel *pCtx = m_cCUInterDirSCModel.get( 0 );

  uiSymbol = 0;
  if (pcCU->getPartitionSize(uiAbsPartIdx) == SIZE_2Nx2N || pcCU->getHeight(uiAbsPartIdx) != 8 )
  {
    m_pcTDecBinIf->decodeBin( uiSymbol, *( pCtx + uiCtx ) RExt__DECODER_DEBUG_BIT_STATISTICS_PASS_OPT_ARG(STATS__CABAC_BITS__INTER_DIR) );
  }

  if( uiSymbol )
  {
    uiSymbol = 2;
  }
  else
  {
    m_pcTDecBinIf->decodeBin( uiSymbol, *( pCtx + 4 ) RExt__DECODER_DEBUG_BIT_STATISTICS_PASS_OPT_ARG(STATS__CABAC_BITS__INTER_DIR) );
    assert(uiSymbol == 0 || uiSymbol == 1);
  }

  uiSymbol++;
  ruiInterDir = uiSymbol;
#if H_MV_ENC_DEC_TRAC
    DTRACE_PU("inter_pred_idc", ruiInterDir - 1 )    
#endif

  return;
}

Void TDecSbac::parseRefFrmIdx( TComDataCU* pcCU, Int& riRefFrmIdx, RefPicList eRefList )
{
  UInt uiSymbol;

  ContextModel *pCtx = m_cCURefPicSCModel.get( 0 );
  m_pcTDecBinIf->decodeBin( uiSymbol, *pCtx RExt__DECODER_DEBUG_BIT_STATISTICS_PASS_OPT_ARG(STATS__CABAC_BITS__REF_FRM_IDX) );

  if( uiSymbol )
  {
    UInt uiRefNum = pcCU->getSlice()->getNumRefIdx( eRefList ) - 2;
    pCtx++;
    UInt ui;
    for( ui = 0; ui < uiRefNum; ++ui )
    {
      if( ui == 0 )
      {
        m_pcTDecBinIf->decodeBin( uiSymbol, *pCtx RExt__DECODER_DEBUG_BIT_STATISTICS_PASS_OPT_ARG(STATS__CABAC_BITS__REF_FRM_IDX) );
      }
      else
      {
        m_pcTDecBinIf->decodeBinEP( uiSymbol RExt__DECODER_DEBUG_BIT_STATISTICS_PASS_OPT_ARG(STATS__CABAC_BITS__REF_FRM_IDX) );
      }
      if( uiSymbol == 0 )
      {
        break;
      }
    }
    uiSymbol = ui + 1;
  }
  riRefFrmIdx = uiSymbol;
#if H_MV_ENC_DEC_TRAC
#if ENC_DEC_TRACE
  if ( eRefList == REF_PIC_LIST_0 )
  {
    DTRACE_PU("ref_idx_l0", uiSymbol)
  }
  else
  {
    DTRACE_PU("ref_idx_l1", uiSymbol)
  }
#endif
#endif

  return;
}

Void TDecSbac::parseMvd( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiPartIdx, UInt uiDepth, RefPicList eRefList )
{
  UInt uiSymbol;
  UInt uiHorAbs;
  UInt uiVerAbs;
  UInt uiHorSign = 0;
  UInt uiVerSign = 0;
  ContextModel *pCtx = m_cCUMvdSCModel.get( 0 );

  if(pcCU->getSlice()->getMvdL1ZeroFlag() && eRefList == REF_PIC_LIST_1 && pcCU->getInterDir(uiAbsPartIdx)==3)
  {
    uiHorAbs=0;
    uiVerAbs=0;
  }
  else
  {
    m_pcTDecBinIf->decodeBin( uiHorAbs, *pCtx RExt__DECODER_DEBUG_BIT_STATISTICS_PASS_OPT_ARG(STATS__CABAC_BITS__MVD) );
    m_pcTDecBinIf->decodeBin( uiVerAbs, *pCtx RExt__DECODER_DEBUG_BIT_STATISTICS_PASS_OPT_ARG(STATS__CABAC_BITS__MVD) );

    const Bool bHorAbsGr0 = uiHorAbs != 0;
    const Bool bVerAbsGr0 = uiVerAbs != 0;
    pCtx++;

    if( bHorAbsGr0 )
    {
      m_pcTDecBinIf->decodeBin( uiSymbol, *pCtx RExt__DECODER_DEBUG_BIT_STATISTICS_PASS_OPT_ARG(STATS__CABAC_BITS__MVD) );
      uiHorAbs += uiSymbol;
    }

    if( bVerAbsGr0 )
    {
      m_pcTDecBinIf->decodeBin( uiSymbol, *pCtx RExt__DECODER_DEBUG_BIT_STATISTICS_PASS_OPT_ARG(STATS__CABAC_BITS__MVD) );
      uiVerAbs += uiSymbol;
    }

    if( bHorAbsGr0 )
    {
      if( 2 == uiHorAbs )
      {
        xReadEpExGolomb( uiSymbol, 1 RExt__DECODER_DEBUG_BIT_STATISTICS_PASS_OPT_ARG(STATS__CABAC_BITS__MVD_EP) );
        uiHorAbs += uiSymbol;
      }

      m_pcTDecBinIf->decodeBinEP( uiHorSign RExt__DECODER_DEBUG_BIT_STATISTICS_PASS_OPT_ARG(STATS__CABAC_BITS__MVD_EP) );
    }

    if( bVerAbsGr0 )
    {
      if( 2 == uiVerAbs )
      {
        xReadEpExGolomb( uiSymbol, 1 RExt__DECODER_DEBUG_BIT_STATISTICS_PASS_OPT_ARG(STATS__CABAC_BITS__MVD_EP) );
        uiVerAbs += uiSymbol;
      }

      m_pcTDecBinIf->decodeBinEP( uiVerSign RExt__DECODER_DEBUG_BIT_STATISTICS_PASS_OPT_ARG(STATS__CABAC_BITS__MVD_EP) );
    }

  }

  const TComMv cMv( uiHorSign ? -Int( uiHorAbs ): uiHorAbs, uiVerSign ? -Int( uiVerAbs ) : uiVerAbs );
  pcCU->getCUMvField( eRefList )->setAllMvd( cMv, pcCU->getPartitionSize( uiAbsPartIdx ), uiAbsPartIdx, uiDepth, uiPartIdx );
  return;
}

Void TDecSbac::parseCrossComponentPrediction( TComTU &rTu, ComponentID compID )
{
  TComDataCU *pcCU = rTu.getCU();

  if( isLuma(compID) || !pcCU->getSlice()->getPPS()->getPpsRangeExtension().getCrossComponentPredictionEnabledFlag() )
  {
    return;
  }

  const UInt uiAbsPartIdx = rTu.GetAbsPartIdxTU();

  if (!pcCU->isIntra(uiAbsPartIdx) || (pcCU->getIntraDir( CHANNEL_TYPE_CHROMA, uiAbsPartIdx ) == DM_CHROMA_IDX))
  {
    Char alpha  = 0;
    UInt symbol = 0;

    DTRACE_CABAC_VL( g_nSymbolCounter++ )
    DTRACE_CABAC_T("\tparseCrossComponentPrediction()")
    DTRACE_CABAC_T( "\tAddr=" )
    DTRACE_CABAC_V( compID )
    DTRACE_CABAC_T( "\tuiAbsPartIdx=" )
    DTRACE_CABAC_V( uiAbsPartIdx )
#if RExt__DECODER_DEBUG_BIT_STATISTICS
    TComCodingStatisticsClassType ctype(STATS__CABAC_BITS__CROSS_COMPONENT_PREDICTION, (g_aucConvertToBit[rTu.getRect(compID).width] + 2), compID);
#endif
    ContextModel *pCtx = m_cCrossComponentPredictionSCModel.get(0, 0) + ((compID == COMPONENT_Cr) ? (NUM_CROSS_COMPONENT_PREDICTION_CTX >> 1) : 0);
    m_pcTDecBinIf->decodeBin( symbol, pCtx[0] RExt__DECODER_DEBUG_BIT_STATISTICS_PASS_OPT_ARG(ctype) );

    if(symbol != 0)
    {
      // Cross-component prediction alpha is non-zero.
      UInt sign = 0;
      m_pcTDecBinIf->decodeBin( symbol, pCtx[1] RExt__DECODER_DEBUG_BIT_STATISTICS_PASS_OPT_ARG(ctype) );

      if (symbol != 0)
      {
        // alpha is 2 (symbol=1), 4(symbol=2) or 8(symbol=3).
        // Read up to two more bits
        xReadUnaryMaxSymbol( symbol, (pCtx + 2), 1, 2 RExt__DECODER_DEBUG_BIT_STATISTICS_PASS_OPT_ARG(ctype) );
        symbol += 1;
      }
      m_pcTDecBinIf->decodeBin( sign, pCtx[4] RExt__DECODER_DEBUG_BIT_STATISTICS_PASS_OPT_ARG(ctype) );

      alpha = (sign != 0) ? -(1 << symbol) : (1 << symbol);
    }
    DTRACE_CABAC_T( "\tAlpha=" )
    DTRACE_CABAC_V( alpha )
    DTRACE_CABAC_T( "\n" )

    pcCU->setCrossComponentPredictionAlphaPartRange( alpha, compID, uiAbsPartIdx, rTu.GetAbsPartIdxNumParts( compID ) );
  }
}

Void TDecSbac::parseTransformSubdivFlag( UInt& ruiSubdivFlag, UInt uiLog2TransformBlockSize )
{
  m_pcTDecBinIf->decodeBin( ruiSubdivFlag, m_cCUTransSubdivFlagSCModel.get( 0, 0, uiLog2TransformBlockSize )
      RExt__DECODER_DEBUG_BIT_STATISTICS_PASS_OPT_ARG(TComCodingStatisticsClassType(STATS__CABAC_BITS__TRANSFORM_SUBDIV_FLAG, 5-uiLog2TransformBlockSize))
                          );
#if !H_MV_ENC_DEC_TRAC
  DTRACE_CABAC_VL( g_nSymbolCounter++ )
  DTRACE_CABAC_T( "\tparseTransformSubdivFlag()" )
  DTRACE_CABAC_T( "\tsymbol=" )
  DTRACE_CABAC_V( ruiSubdivFlag )
  DTRACE_CABAC_T( "\tctx=" )
  DTRACE_CABAC_V( uiLog2TransformBlockSize )
  DTRACE_CABAC_T( "\n" )
#else
  DTRACE_TU("split_transform_flag", ruiSubdivFlag )
#endif

}

Void TDecSbac::parseQtRootCbf( UInt uiAbsPartIdx, UInt& uiQtRootCbf )
{
  UInt uiSymbol;
  const UInt uiCtx = 0;
  m_pcTDecBinIf->decodeBin( uiSymbol , m_cCUQtRootCbfSCModel.get( 0, 0, uiCtx ) RExt__DECODER_DEBUG_BIT_STATISTICS_PASS_OPT_ARG(STATS__CABAC_BITS__QT_ROOT_CBF) );
#if !H_MV_ENC_DEC_TRAC
  DTRACE_CABAC_VL( g_nSymbolCounter++ )
  DTRACE_CABAC_T( "\tparseQtRootCbf()" )
  DTRACE_CABAC_T( "\tsymbol=" )
  DTRACE_CABAC_V( uiSymbol )
  DTRACE_CABAC_T( "\tctx=" )
  DTRACE_CABAC_V( uiCtx )
  DTRACE_CABAC_T( "\tuiAbsPartIdx=" )
  DTRACE_CABAC_V( uiAbsPartIdx )
  DTRACE_CABAC_T( "\n" )
#else
  DTRACE_CU( "rqt_root_cbf", uiSymbol )
#endif

  uiQtRootCbf = uiSymbol;
}

Void TDecSbac::parseDeltaQP( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth )
{
  Int qp;
  UInt uiDQp;
  Int  iDQp;

  UInt uiSymbol;

  xReadUnaryMaxSymbol (uiDQp,  &m_cCUDeltaQpSCModel.get( 0, 0, 0 ), 1, CU_DQP_TU_CMAX RExt__DECODER_DEBUG_BIT_STATISTICS_PASS_OPT_ARG(STATS__CABAC_BITS__DELTA_QP_EP) );

  if( uiDQp >= CU_DQP_TU_CMAX)
  {
    xReadEpExGolomb( uiSymbol, CU_DQP_EG_k RExt__DECODER_DEBUG_BIT_STATISTICS_PASS_OPT_ARG(STATS__CABAC_BITS__DELTA_QP_EP));
    uiDQp+=uiSymbol;
  }

  if ( uiDQp > 0 )
  {
    UInt uiSign;
    Int qpBdOffsetY = pcCU->getSlice()->getSPS()->getQpBDOffset(CHANNEL_TYPE_LUMA);
    m_pcTDecBinIf->decodeBinEP(uiSign RExt__DECODER_DEBUG_BIT_STATISTICS_PASS_OPT_ARG(STATS__CABAC_BITS__DELTA_QP_EP));
    iDQp = uiDQp;
    if(uiSign)
    {
      iDQp = -iDQp;
    }
    qp = (((Int) pcCU->getRefQP( uiAbsPartIdx ) + iDQp + 52 + 2*qpBdOffsetY )%(52+qpBdOffsetY)) - qpBdOffsetY;
  }
  else
  {
    qp = pcCU->getRefQP(uiAbsPartIdx);
  }

  pcCU->setQPSubParts(qp, uiAbsPartIdx, uiDepth);
  pcCU->setCodedQP(qp);
}

/** parse chroma qp adjustment, converting to the internal table representation.
 * \returns Void
 */
Void TDecSbac::parseChromaQpAdjustment( TComDataCU* cu, UInt absPartIdx, UInt depth )
{
  UInt symbol;
#if RExt__DECODER_DEBUG_BIT_STATISTICS
  const TComCodingStatisticsClassType ctype(STATS__CABAC_BITS__CHROMA_QP_ADJUSTMENT, g_aucConvertToBit[cu->getSlice()->getSPS()->getMaxCUWidth()>>depth]+2, CHANNEL_TYPE_CHROMA);
#endif

  Int chromaQpOffsetListLen = cu->getSlice()->getPPS()->getPpsRangeExtension().getChromaQpOffsetListLen();

  // cu_chroma_qp_offset_flag
  m_pcTDecBinIf->decodeBin( symbol, m_ChromaQpAdjFlagSCModel.get( 0, 0, 0 ) RExt__DECODER_DEBUG_BIT_STATISTICS_PASS_OPT_ARG(ctype) );

  if (symbol && chromaQpOffsetListLen > 1)
  {
    // cu_chroma_qp_offset_idx
    xReadUnaryMaxSymbol( symbol,  &m_ChromaQpAdjIdcSCModel.get( 0, 0, 0 ), 0, chromaQpOffsetListLen - 1 RExt__DECODER_DEBUG_BIT_STATISTICS_PASS_OPT_ARG(ctype) );
    symbol++;
  }
  /* NB, symbol = 0 if outer flag is not set,
   *              1 if outer flag is set and there is no inner flag
   *              1+ otherwise */
  cu->setChromaQpAdjSubParts( symbol, absPartIdx, depth );
  cu->setCodedChromaQpAdj(symbol);
}

Void TDecSbac::parseQtCbf( TComTU &rTu, const ComponentID compID, const Bool lowestLevel )
{
  TComDataCU* pcCU = rTu.getCU();

  const UInt absPartIdx       = rTu.GetAbsPartIdxTU(compID);
  const UInt TUDepth          = rTu.GetTransformDepthRel();
  const UInt uiCtx            = pcCU->getCtxQtCbf( rTu, toChannelType(compID) );
  const UInt contextSet       = toChannelType(compID);

  const UInt width            = rTu.getRect(compID).width;
  const UInt height           = rTu.getRect(compID).height;
  const Bool canQuadSplit     = (width >= (MIN_TU_SIZE * 2)) && (height >= (MIN_TU_SIZE * 2));
  const UInt coveredPartIdxes = rTu.GetAbsPartIdxNumParts(compID);

  //             Since the CBF for chroma is coded at the highest level possible, if sub-TUs are
  //             to be coded for a 4x8 chroma TU, their CBFs must be coded at the highest 4x8 level
  //             (i.e. where luma TUs are 8x8 rather than 4x4)
  //    ___ ___
  //   |   |   | <- 4 x (8x8 luma + 4x8 4:2:2 chroma)
  //   |___|___|    each quadrant has its own chroma CBF
  //   |   |   | _ _ _ _
  //   |___|___|        |
  //   <--16--->        V
  //                   _ _
  //                  |_|_| <- 4 x 4x4 luma + 1 x 4x8 4:2:2 chroma
  //                  |_|_|    no chroma CBF is coded - instead the parent CBF is inherited
  //                  <-8->    if sub-TUs are present, their CBFs had to be coded at the parent level

  const UInt lowestTUDepth = TUDepth + ((!lowestLevel && !canQuadSplit) ? 1 : 0); //unsplittable TUs inherit their parent's CBF
        UInt lowestTUCBF   = 0;

  if ((width != height) && (lowestLevel || !canQuadSplit)) //if sub-TUs are present
  {
    const UInt subTUDepth        = lowestTUDepth + 1;
    const UInt partIdxesPerSubTU = rTu.GetAbsPartIdxNumParts(compID) >> 1;

    UInt combinedSubTUCBF = 0;

    for (UInt subTU = 0; subTU < 2; subTU++)
    {
      UInt uiCbf = MAX_UINT;
      m_pcTDecBinIf->decodeBin(uiCbf, m_cCUQtCbfSCModel.get(0, contextSet, uiCtx) RExt__DECODER_DEBUG_BIT_STATISTICS_PASS_OPT_ARG(TComCodingStatisticsClassType(STATS__CABAC_BITS__QT_CBF, g_aucConvertToBit[rTu.getRect(compID).width]+2, compID)));

      const UInt subTUAbsPartIdx = absPartIdx + (subTU * partIdxesPerSubTU);
      pcCU->setCbfPartRange((uiCbf << subTUDepth), compID, subTUAbsPartIdx, partIdxesPerSubTU);
      combinedSubTUCBF |= uiCbf;
#if !H_MV_ENC_DEC_TRAC  
      DTRACE_CABAC_VL( g_nSymbolCounter++ )
      DTRACE_CABAC_T( "\tparseQtCbf()" )
      DTRACE_CABAC_T( "\tsub-TU=" )
      DTRACE_CABAC_V( subTU )
      DTRACE_CABAC_T( "\tsymbol=" )
      DTRACE_CABAC_V( uiCbf )
      DTRACE_CABAC_T( "\tctx=" )
      DTRACE_CABAC_V( uiCtx )
      DTRACE_CABAC_T( "\tetype=" )
      DTRACE_CABAC_V( compID )
      DTRACE_CABAC_T( "\tuiAbsPartIdx=" )
      DTRACE_CABAC_V( subTUAbsPartIdx )
      DTRACE_CABAC_T( "\n" )
#else
  if ( compID == COMPONENT_Cb )
  {
    DTRACE_TU("cbf_cb", uiCbf )
  }
  else if ( compID == COMPONENT_Cr )
  {
    DTRACE_TU("cbf_cr", uiCbf )
  }
  else
  {
    DTRACE_TU("cbf_luma", uiCbf )
  }
#endif

    }

    //propagate the sub-TU CBF up to the lowest TU level
    if (combinedSubTUCBF != 0)
    {
      pcCU->bitwiseOrCbfPartRange((combinedSubTUCBF << lowestTUDepth), compID, absPartIdx, coveredPartIdxes);
      lowestTUCBF = combinedSubTUCBF;
    }
  }
  else
  {
    UInt uiCbf = MAX_UINT;
    m_pcTDecBinIf->decodeBin(uiCbf, m_cCUQtCbfSCModel.get(0, contextSet, uiCtx) RExt__DECODER_DEBUG_BIT_STATISTICS_PASS_OPT_ARG(TComCodingStatisticsClassType(STATS__CABAC_BITS__QT_CBF, g_aucConvertToBit[rTu.getRect(compID).width]+2, compID)));

    pcCU->setCbfSubParts((uiCbf << lowestTUDepth), compID, absPartIdx, rTu.GetTransformDepthTotalAdj(compID));
#if !H_MV_ENC_DEC_TRAC
    DTRACE_CABAC_VL( g_nSymbolCounter++ )
    DTRACE_CABAC_T( "\tparseQtCbf()" )
    DTRACE_CABAC_T( "\tsymbol=" )
    DTRACE_CABAC_V( uiCbf )
    DTRACE_CABAC_T( "\tctx=" )
    DTRACE_CABAC_V( uiCtx )
    DTRACE_CABAC_T( "\tetype=" )
    DTRACE_CABAC_V( compID )
    DTRACE_CABAC_T( "\tuiAbsPartIdx=" )
    DTRACE_CABAC_V( rTu.GetAbsPartIdxTU(compID) )
    DTRACE_CABAC_T( "\n" )
#else
    if ( compID == COMPONENT_Cb )
    {
      DTRACE_TU("cbf_cb", uiCbf )
    }
    else if ( compID == COMPONENT_Cr )
    {
      DTRACE_TU("cbf_cr", uiCbf )
    }
    else
    {
      DTRACE_TU("cbf_luma", uiCbf )
    }
#endif
    lowestTUCBF = uiCbf;
  }

  //propagate the lowest level CBF up to the current level
  if (lowestTUCBF != 0)
  {
    for (UInt depth = TUDepth; depth < lowestTUDepth; depth++)
    {
      pcCU->bitwiseOrCbfPartRange((lowestTUCBF << depth), compID, absPartIdx, coveredPartIdxes);
    }
  }
}


Void TDecSbac::parseTransformSkipFlags (TComTU &rTu, ComponentID component)
{
  TComDataCU* pcCU=rTu.getCU();
  UInt uiAbsPartIdx=rTu.GetAbsPartIdxTU(component);

  if (pcCU->getCUTransquantBypass(uiAbsPartIdx))
  {
    return;
  }

  if (!TUCompRectHasAssociatedTransformSkipFlag(rTu.getRect(component), pcCU->getSlice()->getPPS()->getPpsRangeExtension().getLog2MaxTransformSkipBlockSize()))
  {
    return;
  }

  UInt useTransformSkip;

  m_pcTDecBinIf->decodeBin( useTransformSkip , m_cTransformSkipSCModel.get( 0, toChannelType(component), 0 )
      RExt__DECODER_DEBUG_BIT_STATISTICS_PASS_OPT_ARG(TComCodingStatisticsClassType(STATS__CABAC_BITS__TRANSFORM_SKIP_FLAGS, component))
                          );

#if !H_MV_ENC_DEC_TRAC
  DTRACE_CABAC_VL( g_nSymbolCounter++ )
  DTRACE_CABAC_T("\tparseTransformSkip()");
  DTRACE_CABAC_T( "\tsymbol=" )
  DTRACE_CABAC_V( useTransformSkip )
  DTRACE_CABAC_T( "\tAddr=" )
  DTRACE_CABAC_V( pcCU->getCtuRsAddr() )
  DTRACE_CABAC_T( "\tetype=" )
  DTRACE_CABAC_V( component )
  DTRACE_CABAC_T( "\tuiAbsPartIdx=" )
  DTRACE_CABAC_V( rTu.GetAbsPartIdxTU() )
  DTRACE_CABAC_T( "\n" )
#endif

  pcCU->setTransformSkipPartRange( useTransformSkip, component, uiAbsPartIdx, rTu.GetAbsPartIdxNumParts(component));
}


/** Parse (X,Y) position of the last significant coefficient
 * \param uiPosLastX reference to X component of last coefficient
 * \param uiPosLastY reference to Y component of last coefficient
 * \param width  Block width
 * \param height Block height
 * \param component chroma compinent ID
 * \param uiScanIdx scan type (zig-zag, hor, ver)
 *
 * This method decodes the X and Y component within a block of the last significant coefficient.
 */
Void TDecSbac::parseLastSignificantXY( UInt& uiPosLastX, UInt& uiPosLastY, Int width, Int height, ComponentID component, UInt uiScanIdx )
{
  UInt uiLast;

  ContextModel *pCtxX = m_cCuCtxLastX.get( 0, toChannelType(component) );
  ContextModel *pCtxY = m_cCuCtxLastY.get( 0, toChannelType(component) );

#if RExt__DECODER_DEBUG_BIT_STATISTICS
  TComCodingStatisticsClassType ctype(STATS__CABAC_BITS__LAST_SIG_X_Y, g_aucConvertToBit[width]+2, component);
#endif


  if ( uiScanIdx == SCAN_VER )
  {
    swap( width, height );
  }

  Int blkSizeOffsetX, blkSizeOffsetY, shiftX, shiftY;
  getLastSignificantContextParameters(component, width, height, blkSizeOffsetX, blkSizeOffsetY, shiftX, shiftY);

  //------------------

  // posX

  for( uiPosLastX = 0; uiPosLastX < g_uiGroupIdx[ width - 1 ]; uiPosLastX++ )
  {
    m_pcTDecBinIf->decodeBin( uiLast, *( pCtxX + blkSizeOffsetX + (uiPosLastX >>shiftX) ) RExt__DECODER_DEBUG_BIT_STATISTICS_PASS_OPT_ARG(ctype) );

    if( !uiLast )
    {
      break;
    }
  }

  // posY

  for( uiPosLastY = 0; uiPosLastY < g_uiGroupIdx[ height - 1 ]; uiPosLastY++ )
  {
    m_pcTDecBinIf->decodeBin( uiLast, *( pCtxY + blkSizeOffsetY + (uiPosLastY >>shiftY)) RExt__DECODER_DEBUG_BIT_STATISTICS_PASS_OPT_ARG(ctype) );

    if( !uiLast )
    {
      break;
    }
  }

  // EP-coded part

  if ( uiPosLastX > 3 )
  {
    UInt uiTemp  = 0;
    UInt uiCount = ( uiPosLastX - 2 ) >> 1;
    for ( Int i = uiCount - 1; i >= 0; i-- )
    {
      m_pcTDecBinIf->decodeBinEP( uiLast RExt__DECODER_DEBUG_BIT_STATISTICS_PASS_OPT_ARG(ctype) );
      uiTemp += uiLast << i;
    }
    uiPosLastX = g_uiMinInGroup[ uiPosLastX ] + uiTemp;
  }
  if ( uiPosLastY > 3 )
  {
    UInt uiTemp  = 0;
    UInt uiCount = ( uiPosLastY - 2 ) >> 1;
    for ( Int i = uiCount - 1; i >= 0; i-- )
    {
      m_pcTDecBinIf->decodeBinEP( uiLast RExt__DECODER_DEBUG_BIT_STATISTICS_PASS_OPT_ARG(ctype) );
      uiTemp += uiLast << i;
    }
    uiPosLastY = g_uiMinInGroup[ uiPosLastY ] + uiTemp;
  }

  if( uiScanIdx == SCAN_VER )
  {
    swap( uiPosLastX, uiPosLastY );
  }
}

Void TDecSbac::parseCoeffNxN(  TComTU &rTu, ComponentID compID )
{
  TComDataCU* pcCU=rTu.getCU();
  const UInt uiAbsPartIdx=rTu.GetAbsPartIdxTU(compID);
  const TComRectangle &rRect=rTu.getRect(compID);
  const UInt uiWidth=rRect.width;
  const UInt uiHeight=rRect.height;
  TCoeff* pcCoef=(pcCU->getCoeff(compID)+rTu.getCoefficientOffset(compID));
  const TComSPS &sps=*(pcCU->getSlice()->getSPS());
#if !H_MV_ENC_DEC_TRAC
  DTRACE_CABAC_VL( g_nSymbolCounter++ )
  DTRACE_CABAC_T( "\tparseCoeffNxN()\teType=" )
  DTRACE_CABAC_V( compID )
  DTRACE_CABAC_T( "\twidth=" )
  DTRACE_CABAC_V( uiWidth )
  DTRACE_CABAC_T( "\theight=" )
  DTRACE_CABAC_V( uiHeight )
  DTRACE_CABAC_T( "\tdepth=" )
//  DTRACE_CABAC_V( rTu.GetTransformDepthTotalAdj(compID) )
  DTRACE_CABAC_V( rTu.GetTransformDepthTotal() )
  DTRACE_CABAC_T( "\tabspartidx=" )
//  DTRACE_CABAC_V( uiAbsPartIdx )
  DTRACE_CABAC_V( rTu.GetAbsPartIdxTU(compID) )
  DTRACE_CABAC_T( "\ttoCU-X=" )
  DTRACE_CABAC_V( pcCU->getCUPelX() )
  DTRACE_CABAC_T( "\ttoCU-Y=" )
  DTRACE_CABAC_V( pcCU->getCUPelY() )
  DTRACE_CABAC_T( "\tCU-addr=" )
  DTRACE_CABAC_V(  pcCU->getCtuRsAddr() )
  DTRACE_CABAC_T( "\tinCU-X=" )
//  DTRACE_CABAC_V( g_auiRasterToPelX[ g_auiZscanToRaster[uiAbsPartIdx] ] )
  DTRACE_CABAC_V( g_auiRasterToPelX[ g_auiZscanToRaster[rTu.GetAbsPartIdxTU(compID)] ] )
  DTRACE_CABAC_T( "\tinCU-Y=" )
// DTRACE_CABAC_V( g_auiRasterToPelY[ g_auiZscanToRaster[uiAbsPartIdx] ] )
  DTRACE_CABAC_V( g_auiRasterToPelY[ g_auiZscanToRaster[rTu.GetAbsPartIdxTU(compID)] ] )
  DTRACE_CABAC_T( "\tpredmode=" )
  DTRACE_CABAC_V(  pcCU->getPredictionMode( uiAbsPartIdx ) )
  DTRACE_CABAC_T( "\n" )
#endif

  //--------------------------------------------------------------------------------------------------

  if( uiWidth > sps.getMaxTrSize() )
  {
    std::cerr << "ERROR: parseCoeffNxN was passed a TU with dimensions larger than the maximum allowed size" << std::endl;
    assert(false);
    exit(1);
  }

  //--------------------------------------------------------------------------------------------------

  //set parameters

  const ChannelType  chType            = toChannelType(compID);
  const UInt         uiLog2BlockWidth  = g_aucConvertToBit[ uiWidth  ] + 2;
  const UInt         uiLog2BlockHeight = g_aucConvertToBit[ uiHeight ] + 2;
  const UInt         uiMaxNumCoeff     = uiWidth * uiHeight;
  const UInt         uiMaxNumCoeffM1   = uiMaxNumCoeff - 1;

  const ChannelType  channelType       = toChannelType(compID);
  const Bool         extendedPrecision = sps.getSpsRangeExtension().getExtendedPrecisionProcessingFlag();

  const Bool         alignCABACBeforeBypass = sps.getSpsRangeExtension().getCabacBypassAlignmentEnabledFlag();
  const Int          maxLog2TrDynamicRange  = sps.getMaxLog2TrDynamicRange(channelType);

#if RExt__DECODER_DEBUG_BIT_STATISTICS
  TComCodingStatisticsClassType ctype_group(STATS__CABAC_BITS__SIG_COEFF_GROUP_FLAG, uiLog2BlockWidth, compID);
  TComCodingStatisticsClassType ctype_map(STATS__CABAC_BITS__SIG_COEFF_MAP_FLAG, uiLog2BlockWidth, compID);
  TComCodingStatisticsClassType ctype_gt1(STATS__CABAC_BITS__GT1_FLAG, uiLog2BlockWidth, compID);
  TComCodingStatisticsClassType ctype_gt2(STATS__CABAC_BITS__GT2_FLAG, uiLog2BlockWidth, compID);
#endif

  Bool beValid;
  if (pcCU->getCUTransquantBypass(uiAbsPartIdx))
  {
    beValid = false;
    if((!pcCU->isIntra(uiAbsPartIdx)) && pcCU->isRDPCMEnabled(uiAbsPartIdx))
    {
      parseExplicitRdpcmMode(rTu, compID);
    }
  }
  else
  {
    beValid = pcCU->getSlice()->getPPS()->getSignHideFlag();
  }

  UInt absSum = 0;

  //--------------------------------------------------------------------------------------------------

  if(pcCU->getSlice()->getPPS()->getUseTransformSkip())
  {
    parseTransformSkipFlags(rTu, compID);
    //  This TU has coefficients and is transform skipped. Check whether is inter coded and if yes decode the explicit RDPCM mode
    if(pcCU->getTransformSkip(uiAbsPartIdx, compID) && (!pcCU->isIntra(uiAbsPartIdx)) && pcCU->isRDPCMEnabled(uiAbsPartIdx) )
    {
      parseExplicitRdpcmMode(rTu, compID);
      if(pcCU->getExplicitRdpcmMode(compID, uiAbsPartIdx) != RDPCM_OFF)
      {
        //  Sign data hiding is avoided for horizontal and vertical RDPCM modes
        beValid = false;
      }
    }
  }

  Int uiIntraMode = -1;
  const Bool       bIsLuma = isLuma(compID);
  Int isIntra = pcCU->isIntra(uiAbsPartIdx) ? 1 : 0;
  if ( isIntra && pcCU->isRDPCMEnabled(uiAbsPartIdx) )
  {
    const UInt partsPerMinCU = 1<<(2*(sps.getMaxTotalCUDepth() - sps.getLog2DiffMaxMinCodingBlockSize()));
    uiIntraMode = pcCU->getIntraDir( toChannelType(compID), uiAbsPartIdx );
    uiIntraMode = (uiIntraMode==DM_CHROMA_IDX && !bIsLuma) ? pcCU->getIntraDir(CHANNEL_TYPE_LUMA, getChromasCorrespondingPULumaIdx(uiAbsPartIdx, rTu.GetChromaFormat(), partsPerMinCU)) : uiIntraMode;
    uiIntraMode = ((rTu.GetChromaFormat() == CHROMA_422) && !bIsLuma) ? g_chroma422IntraAngleMappingTable[uiIntraMode] : uiIntraMode;

    Bool transformSkip = pcCU->getTransformSkip( uiAbsPartIdx,compID);
    Bool rdpcm_lossy = ( transformSkip /*&& isIntra*/ && ( (uiIntraMode == HOR_IDX) || (uiIntraMode == VER_IDX) ) );
    if ( rdpcm_lossy )
    {
      beValid = false;
    }
  }

  //--------------------------------------------------------------------------------------------------

  const Bool  bUseGolombRiceParameterAdaptation = sps.getSpsRangeExtension().getPersistentRiceAdaptationEnabledFlag();
        UInt &currentGolombRiceStatistic        = m_golombRiceAdaptationStatistics[rTu.getGolombRiceStatisticsIndex(compID)];

  //select scans
  TUEntropyCodingParameters codingParameters;
  getTUEntropyCodingParameters(codingParameters, rTu, compID);

  //===== decode last significant =====
  UInt uiPosLastX, uiPosLastY;
  parseLastSignificantXY( uiPosLastX, uiPosLastY, uiWidth, uiHeight, compID, codingParameters.scanType );
  UInt uiBlkPosLast      = uiPosLastX + (uiPosLastY<<uiLog2BlockWidth);
  pcCoef[ uiBlkPosLast ] = 1;

  //===== decode significance flags =====
  UInt uiScanPosLast;
  for( uiScanPosLast = 0; uiScanPosLast < uiMaxNumCoeffM1; uiScanPosLast++ )
  {
    UInt uiBlkPos = codingParameters.scan[ uiScanPosLast ];
    if( uiBlkPosLast == uiBlkPos )
    {
      break;
    }
  }

  ContextModel * const baseCoeffGroupCtx = m_cCUSigCoeffGroupSCModel.get( 0, isChroma(chType) );
  ContextModel * const baseCtx = m_cCUSigSCModel.get( 0, 0 ) + getSignificanceMapContextOffset(compID);

  const Int  iLastScanSet  = uiScanPosLast >> MLS_CG_SIZE;
  UInt c1                  = 1;
  UInt uiGoRiceParam       = 0;


  UInt uiSigCoeffGroupFlag[ MLS_GRP_NUM ];
  memset( uiSigCoeffGroupFlag, 0, sizeof(UInt) * MLS_GRP_NUM );

  Int  iScanPosSig             = (Int) uiScanPosLast;
  for( Int iSubSet = iLastScanSet; iSubSet >= 0; iSubSet-- )
  {
    Int  iSubPos   = iSubSet << MLS_CG_SIZE;
    uiGoRiceParam  = currentGolombRiceStatistic / RExt__GOLOMB_RICE_INCREMENT_DIVISOR;
    Bool updateGolombRiceStatistics = bUseGolombRiceParameterAdaptation; //leave the statistics at 0 when not using the adaptation system
    Int numNonZero = 0;

    Int lastNZPosInCG  = -1;
    Int firstNZPosInCG = 1 << MLS_CG_SIZE;

    Bool escapeDataPresentInGroup = false;

    Int pos[1 << MLS_CG_SIZE];

    if( iScanPosSig == (Int) uiScanPosLast )
    {
      lastNZPosInCG  = iScanPosSig;
      firstNZPosInCG = iScanPosSig;
      iScanPosSig--;
      pos[ numNonZero ] = uiBlkPosLast;
      numNonZero = 1;
    }

    // decode significant_coeffgroup_flag
    Int iCGBlkPos = codingParameters.scanCG[ iSubSet ];
    Int iCGPosY   = iCGBlkPos / codingParameters.widthInGroups;
    Int iCGPosX   = iCGBlkPos - (iCGPosY * codingParameters.widthInGroups);

    if( iSubSet == iLastScanSet || iSubSet == 0)
    {
      uiSigCoeffGroupFlag[ iCGBlkPos ] = 1;
    }
    else
    {
      UInt uiSigCoeffGroup;
      UInt uiCtxSig  = TComTrQuant::getSigCoeffGroupCtxInc( uiSigCoeffGroupFlag, iCGPosX, iCGPosY, codingParameters.widthInGroups, codingParameters.heightInGroups );
      m_pcTDecBinIf->decodeBin( uiSigCoeffGroup, baseCoeffGroupCtx[ uiCtxSig ] RExt__DECODER_DEBUG_BIT_STATISTICS_PASS_OPT_ARG(ctype_group) );
      uiSigCoeffGroupFlag[ iCGBlkPos ] = uiSigCoeffGroup;
    }

    // decode significant_coeff_flag
    const Int patternSigCtx = TComTrQuant::calcPatternSigCtx(uiSigCoeffGroupFlag, iCGPosX, iCGPosY, codingParameters.widthInGroups, codingParameters.heightInGroups);

    UInt uiBlkPos, uiSig, uiCtxSig;
    for( ; iScanPosSig >= iSubPos; iScanPosSig-- )
    {
      uiBlkPos  = codingParameters.scan[ iScanPosSig ];
      uiSig     = 0;

      if( uiSigCoeffGroupFlag[ iCGBlkPos ] )
      {
        if( iScanPosSig > iSubPos || iSubSet == 0  || numNonZero )
        {
          uiCtxSig  = TComTrQuant::getSigCtxInc( patternSigCtx, codingParameters, iScanPosSig, uiLog2BlockWidth, uiLog2BlockHeight, chType );
          m_pcTDecBinIf->decodeBin( uiSig, baseCtx[ uiCtxSig ] RExt__DECODER_DEBUG_BIT_STATISTICS_PASS_OPT_ARG(ctype_map) );
        }
        else
        {
          uiSig = 1;
        }
      }
      pcCoef[ uiBlkPos ] = uiSig;
      if( uiSig )
      {
        pos[ numNonZero ] = uiBlkPos;
        numNonZero ++;
        if( lastNZPosInCG == -1 )
        {
          lastNZPosInCG = iScanPosSig;
        }
        firstNZPosInCG = iScanPosSig;
      }
    }

    if( numNonZero > 0 )
    {
      Bool signHidden = ( lastNZPosInCG - firstNZPosInCG >= SBH_THRESHOLD );

      absSum = 0;

      const UInt uiCtxSet = getContextSetIndex(compID, iSubSet, (c1 == 0));
      c1 = 1;
      UInt uiBin;

      ContextModel *baseCtxMod = m_cCUOneSCModel.get( 0, 0 ) + (NUM_ONE_FLAG_CTX_PER_SET * uiCtxSet);

      Int absCoeff[1 << MLS_CG_SIZE];

      for ( Int i = 0; i < numNonZero; i++)
      {
        absCoeff[i] = 1;
      }
      Int numC1Flag = min(numNonZero, C1FLAG_NUMBER);
      Int firstC2FlagIdx = -1;

      for( Int idx = 0; idx < numC1Flag; idx++ )
      {
        m_pcTDecBinIf->decodeBin( uiBin, baseCtxMod[c1] RExt__DECODER_DEBUG_BIT_STATISTICS_PASS_OPT_ARG(ctype_gt1) );
        if( uiBin == 1 )
        {
          c1 = 0;
          if (firstC2FlagIdx == -1)
          {
            firstC2FlagIdx = idx;
          }
          else //if a greater-than-one has been encountered already this group
          {
            escapeDataPresentInGroup = true;
          }
        }
        else if( (c1 < 3) && (c1 > 0) )
        {
          c1++;
        }
        absCoeff[ idx ] = uiBin + 1;
      }

      if (c1 == 0)
      {
        baseCtxMod = m_cCUAbsSCModel.get( 0, 0 ) + (NUM_ABS_FLAG_CTX_PER_SET * uiCtxSet);
        if ( firstC2FlagIdx != -1)
        {
          m_pcTDecBinIf->decodeBin( uiBin, baseCtxMod[0] RExt__DECODER_DEBUG_BIT_STATISTICS_PASS_OPT_ARG(ctype_gt2) );
          absCoeff[ firstC2FlagIdx ] = uiBin + 2;
          if (uiBin != 0)
          {
            escapeDataPresentInGroup = true;
          }
        }
      }

      escapeDataPresentInGroup = escapeDataPresentInGroup || (numNonZero > C1FLAG_NUMBER);

      const Bool alignGroup = escapeDataPresentInGroup && alignCABACBeforeBypass;

#if RExt__DECODER_DEBUG_BIT_STATISTICS
      TComCodingStatisticsClassType ctype_signs((alignGroup ? STATS__CABAC_BITS__ALIGNED_SIGN_BIT    : STATS__CABAC_BITS__SIGN_BIT   ), uiLog2BlockWidth, compID);
      TComCodingStatisticsClassType ctype_escs ((alignGroup ? STATS__CABAC_BITS__ALIGNED_ESCAPE_BITS : STATS__CABAC_BITS__ESCAPE_BITS), uiLog2BlockWidth, compID);
#endif

      if (alignGroup)
      {
        m_pcTDecBinIf->align();
      }

      UInt coeffSigns;
      if ( signHidden && beValid )
      {
        m_pcTDecBinIf->decodeBinsEP( coeffSigns, numNonZero-1 RExt__DECODER_DEBUG_BIT_STATISTICS_PASS_OPT_ARG(ctype_signs) );
        coeffSigns <<= 32 - (numNonZero-1);
      }
      else
      {
        m_pcTDecBinIf->decodeBinsEP( coeffSigns, numNonZero RExt__DECODER_DEBUG_BIT_STATISTICS_PASS_OPT_ARG(ctype_signs) );
        coeffSigns <<= 32 - numNonZero;
      }

      Int iFirstCoeff2 = 1;
      if (escapeDataPresentInGroup)
      {
        for( Int idx = 0; idx < numNonZero; idx++ )
        {
          UInt baseLevel  = (idx < C1FLAG_NUMBER)? (2 + iFirstCoeff2) : 1;

          if( absCoeff[ idx ] == baseLevel)
          {
            UInt uiLevel;
            xReadCoefRemainExGolomb( uiLevel, uiGoRiceParam, extendedPrecision, maxLog2TrDynamicRange RExt__DECODER_DEBUG_BIT_STATISTICS_PASS_OPT_ARG(ctype_escs) );

            absCoeff[ idx ] = uiLevel + baseLevel;

            if (absCoeff[idx] > (3 << uiGoRiceParam))
            {
              uiGoRiceParam = bUseGolombRiceParameterAdaptation ? (uiGoRiceParam + 1) : (std::min<UInt>((uiGoRiceParam + 1), 4));
            }

            if (updateGolombRiceStatistics)
            {
              const UInt initialGolombRiceParameter = currentGolombRiceStatistic / RExt__GOLOMB_RICE_INCREMENT_DIVISOR;

              if (uiLevel >= (3 << initialGolombRiceParameter))
              {
                currentGolombRiceStatistic++;
              }
              else if (((uiLevel * 2) < (1 << initialGolombRiceParameter)) && (currentGolombRiceStatistic > 0))
              {
                currentGolombRiceStatistic--;
              }

              updateGolombRiceStatistics = false;
            }
          }

          if(absCoeff[ idx ] >= 2)
          {
            iFirstCoeff2 = 0;
          }
        }
      }

      for( Int idx = 0; idx < numNonZero; idx++ )
      {
        Int blkPos = pos[ idx ];
        // Signs applied later.
        pcCoef[ blkPos ] = absCoeff[ idx ];
        absSum += absCoeff[ idx ];

        if ( idx == numNonZero-1 && signHidden && beValid )
        {
          // Infer sign of 1st element.
          if (absSum&0x1)
          {
            pcCoef[ blkPos ] = -pcCoef[ blkPos ];
          }
        }
        else
        {
          Int sign = static_cast<Int>( coeffSigns ) >> 31;
          pcCoef[ blkPos ] = ( pcCoef[ blkPos ] ^ sign ) - sign;
          coeffSigns <<= 1;
        }
      }
    }
  }

#if ENVIRONMENT_VARIABLE_DEBUG_AND_TEST
  printSBACCoeffData(uiPosLastX, uiPosLastY, uiWidth, uiHeight, compID, uiAbsPartIdx, codingParameters.scanType, pcCoef);
#endif

  return;
}

Void TDecSbac::parseSaoMaxUvlc ( UInt& val, UInt maxSymbol )
{
  if (maxSymbol == 0)
  {
    val = 0;
    return;
  }

  UInt code;
  Int  i;
  m_pcTDecBinIf->decodeBinEP( code RExt__DECODER_DEBUG_BIT_STATISTICS_PASS_OPT_ARG(STATS__CABAC_BITS__SAO) );
  if ( code == 0 )
  {
    val = 0;
    return;
  }

  i=1;
  while (1)
  {
    m_pcTDecBinIf->decodeBinEP( code RExt__DECODER_DEBUG_BIT_STATISTICS_PASS_OPT_ARG(STATS__CABAC_BITS__SAO) );
    if ( code == 0 )
    {
      break;
    }
    i++;
    if (i == maxSymbol)
    {
      break;
    }
  }

  val = i;
}

Void TDecSbac::parseSaoUflc (UInt uiLength, UInt&  riVal)
{
  m_pcTDecBinIf->decodeBinsEP ( riVal, uiLength RExt__DECODER_DEBUG_BIT_STATISTICS_PASS_OPT_ARG(STATS__CABAC_BITS__SAO) );
}

Void TDecSbac::parseSaoMerge (UInt&  ruiVal)
{
  UInt uiCode;
  m_pcTDecBinIf->decodeBin( uiCode, m_cSaoMergeSCModel.get( 0, 0, 0 ) RExt__DECODER_DEBUG_BIT_STATISTICS_PASS_OPT_ARG(STATS__CABAC_BITS__SAO) );
  ruiVal = (Int)uiCode;
}

Void TDecSbac::parseSaoTypeIdx (UInt&  ruiVal)
{
  UInt uiCode;
  m_pcTDecBinIf->decodeBin( uiCode, m_cSaoTypeIdxSCModel.get( 0, 0, 0 ) RExt__DECODER_DEBUG_BIT_STATISTICS_PASS_OPT_ARG(STATS__CABAC_BITS__SAO) );
  if (uiCode == 0)
  {
    ruiVal = 0;
  }
  else
  {
    m_pcTDecBinIf->decodeBinEP( uiCode RExt__DECODER_DEBUG_BIT_STATISTICS_PASS_OPT_ARG(STATS__CABAC_BITS__SAO) );
    if (uiCode == 0)
    {
      ruiVal = 1;
    }
    else
    {
      ruiVal = 2;
    }
  }
}

Void TDecSbac::parseSaoSign(UInt& val)
{
  m_pcTDecBinIf->decodeBinEP ( val RExt__DECODER_DEBUG_BIT_STATISTICS_PASS_OPT_ARG(STATS__CABAC_BITS__SAO) );
}

Void TDecSbac::parseSAOBlkParam (SAOBlkParam& saoBlkParam
                                , Bool* sliceEnabled
                                , Bool leftMergeAvail
                                , Bool aboveMergeAvail
                                , const BitDepths &bitDepths
                                )
{
  UInt uiSymbol;

  Bool isLeftMerge = false;
  Bool isAboveMerge= false;

  if(leftMergeAvail)
  {
    parseSaoMerge(uiSymbol); //sao_merge_left_flag
    isLeftMerge = (uiSymbol?true:false);
  }

  if( aboveMergeAvail && !isLeftMerge)
  {
    parseSaoMerge(uiSymbol); //sao_merge_up_flag
    isAboveMerge = (uiSymbol?true:false);
  }

  if(isLeftMerge || isAboveMerge) //merge mode
  {
    for (UInt componentIndex = 0; componentIndex < MAX_NUM_COMPONENT; componentIndex++)
    {
      saoBlkParam[componentIndex].modeIdc = (sliceEnabled[componentIndex]) ? SAO_MODE_MERGE : SAO_MODE_OFF;
      saoBlkParam[componentIndex].typeIdc = (isLeftMerge)?SAO_MERGE_LEFT:SAO_MERGE_ABOVE;
    }
  }
  else //new or off mode
  {
    for(Int compId=COMPONENT_Y; compId < MAX_NUM_COMPONENT; compId++)
    {
      const ComponentID compIdx=ComponentID(compId);
      const ComponentID firstCompOfChType = getFirstComponentOfChannel(toChannelType(compIdx));
      SAOOffset& ctbParam = saoBlkParam[compIdx];
#if O0043_BEST_EFFORT_DECODING
      const Int bitDepthOrig = bitDepths.stream[toChannelType(compIdx)];
      const Int forceBitDepthAdjust = bitDepthOrig - bitDepths.recon[toChannelType(compIdx)];
#else
      const Int bitDepthOrig = bitDepths.recon[toChannelType(compIdx)];
#endif
      const Int maxOffsetQVal=TComSampleAdaptiveOffset::getMaxOffsetQVal(bitDepthOrig);
      if(!sliceEnabled[compIdx])
      {
        //off
        ctbParam.modeIdc = SAO_MODE_OFF;
        continue;
      }

      //type
      if(compIdx == firstCompOfChType)
      {
        parseSaoTypeIdx(uiSymbol); //sao_type_idx_luma or sao_type_idx_chroma

        assert(uiSymbol ==0 || uiSymbol ==1 || uiSymbol ==2);

        if(uiSymbol ==0) //OFF
        {
          ctbParam.modeIdc = SAO_MODE_OFF;
        }
        else if(uiSymbol == 1) //BO
        {
          ctbParam.modeIdc = SAO_MODE_NEW;
          ctbParam.typeIdc = SAO_TYPE_START_BO;
        }
        else //2, EO
        {
          ctbParam.modeIdc = SAO_MODE_NEW;
          ctbParam.typeIdc = SAO_TYPE_START_EO;
        }

      }
      else //Cr, follow Cb SAO type
      {
        ctbParam.modeIdc = saoBlkParam[COMPONENT_Cb].modeIdc;
        ctbParam.typeIdc = saoBlkParam[COMPONENT_Cb].typeIdc;
      }

      if(ctbParam.modeIdc == SAO_MODE_NEW)
      {
        Int offset[4];
        for(Int i=0; i< 4; i++)
        {
          parseSaoMaxUvlc(uiSymbol, maxOffsetQVal ); //sao_offset_abs
          offset[i] = (Int)uiSymbol;
        }

        if(ctbParam.typeIdc == SAO_TYPE_START_BO)
        {
          for(Int i=0; i< 4; i++)
          {
            if(offset[i] != 0)
            {
              parseSaoSign(uiSymbol); //sao_offset_sign
              if(uiSymbol)
              {
#if O0043_BEST_EFFORT_DECODING
                offset[i] >>= forceBitDepthAdjust;
#endif
                offset[i] = -offset[i];
              }
            }
          }
          parseSaoUflc(NUM_SAO_BO_CLASSES_LOG2, uiSymbol ); //sao_band_position
          ctbParam.typeAuxInfo = uiSymbol;

          for(Int i=0; i<4; i++)
          {
            ctbParam.offset[(ctbParam.typeAuxInfo+i)%MAX_NUM_SAO_CLASSES] = offset[i];
          }

        }
        else //EO
        {
          ctbParam.typeAuxInfo = 0;

          if(firstCompOfChType == compIdx)
          {
            parseSaoUflc(NUM_SAO_EO_TYPES_LOG2, uiSymbol ); //sao_eo_class_luma or sao_eo_class_chroma
            ctbParam.typeIdc += uiSymbol;
          }
          else
          {
            ctbParam.typeIdc = saoBlkParam[firstCompOfChType].typeIdc;
          }
          ctbParam.offset[SAO_CLASS_EO_FULL_VALLEY] = offset[0];
          ctbParam.offset[SAO_CLASS_EO_HALF_VALLEY] = offset[1];
          ctbParam.offset[SAO_CLASS_EO_PLAIN      ] = 0;
          ctbParam.offset[SAO_CLASS_EO_HALF_PEAK  ] = -offset[2];
          ctbParam.offset[SAO_CLASS_EO_FULL_PEAK  ] = -offset[3];
        }
      }
    }
  }
}

/**
 - Initialize our contexts from the nominated source.
 .
 \param pSrc Contexts to be copied.
 */
Void TDecSbac::xCopyContextsFrom( const TDecSbac* pSrc )
{
  memcpy(m_contextModels, pSrc->m_contextModels, m_numContextModels*sizeof(m_contextModels[0]));
  memcpy(m_golombRiceAdaptationStatistics, pSrc->m_golombRiceAdaptationStatistics, (sizeof(UInt) * RExt__GOLOMB_RICE_ADAPTATION_STATISTICS_SETS));
}

Void TDecSbac::xCopyFrom( const TDecSbac* pSrc )
{
  m_pcTDecBinIf->copyState( pSrc->m_pcTDecBinIf );
  xCopyContextsFrom( pSrc );
}

Void TDecSbac::load ( const TDecSbac* pSrc )
{
  xCopyFrom(pSrc);
}

Void TDecSbac::loadContexts ( const TDecSbac* pSrc )
{
  xCopyContextsFrom(pSrc);
}

/** Performs CABAC decoding of the explicit RDPCM mode
 * \param rTu current TU data structure
 * \param compID component identifier
 */
Void TDecSbac::parseExplicitRdpcmMode( TComTU &rTu, ComponentID compID )
{
  TComDataCU* cu = rTu.getCU();
  const UInt absPartIdx=rTu.GetAbsPartIdxTU(compID);
  const TComRectangle &rect = rTu.getRect(compID);
  const UInt tuHeight = g_aucConvertToBit[rect.height];
  const UInt tuWidth  = g_aucConvertToBit[rect.width];
  UInt code = 0;

  assert(tuHeight == tuWidth);

#if RExt__DECODER_DEBUG_BIT_STATISTICS
  const TComCodingStatisticsClassType ctype(STATS__EXPLICIT_RDPCM_BITS, g_aucConvertToBit[cu->getSlice()->getSPS()->getMaxCUWidth()>>rTu.GetTransformDepthTotal()]+2);
#endif

  m_pcTDecBinIf->decodeBin(code, m_explicitRdpcmFlagSCModel.get (0, toChannelType(compID), 0) RExt__DECODER_DEBUG_BIT_STATISTICS_PASS_OPT_ARG(ctype));

  if(code == 0)
  {
    cu->setExplicitRdpcmModePartRange( RDPCM_OFF, compID, absPartIdx, rTu.GetAbsPartIdxNumParts(compID));
  }
  else
  {
    m_pcTDecBinIf->decodeBin(code, m_explicitRdpcmDirSCModel.get (0, toChannelType(compID), 0) RExt__DECODER_DEBUG_BIT_STATISTICS_PASS_OPT_ARG(ctype));
    if(code == 0)
    {
      cu->setExplicitRdpcmModePartRange( RDPCM_HOR, compID, absPartIdx, rTu.GetAbsPartIdxNumParts(compID));
    }
    else
    {
      cu->setExplicitRdpcmModePartRange( RDPCM_VER, compID, absPartIdx, rTu.GetAbsPartIdxNumParts(compID));
    }
  }
}

#if H_3D_ARP
Void TDecSbac::parseARPW( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth )
{
  UInt uiMaxW = pcCU->getSlice()->getARPStepNum() - 1;
  UInt uiW = 0;
  UInt uiOffset = pcCU->getCTXARPWFlag(uiAbsPartIdx);
  UInt uiCode = 0;

  assert ( uiMaxW > 0 );

  m_pcTDecBinIf->decodeBin( uiCode , m_cCUPUARPWSCModel.get( 0, 0, 0 + uiOffset ) );

  uiW = uiCode;
  if( 1 == uiW )   
  {
    m_pcTDecBinIf->decodeBin( uiCode , m_cCUPUARPWSCModel.get( 0, 0, 2 ) );
    uiW += ( 1 == uiCode ? 1 : 0 );
  }
#if H_MV_ENC_DEC_TRAC
  DTRACE_CU("iv_res_pred_weight_idx", uiW )
#endif
  pcCU->setARPWSubParts( ( UChar )( uiW ) , uiAbsPartIdx, uiDepth );  
}
#endif

#if H_3D_IC
/** parse illumination compensation flag
 * \param pcCU
 * \param uiAbsPartIdx 
 * \param uiDepth
 * \returns Void
 */
Void TDecSbac::parseICFlag( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth )
{ 
  UInt uiSymbol = 0;
  m_pcTDecBinIf->decodeBin( uiSymbol, m_cCUICFlagSCModel.get( 0, 0, 0 ) );
#if !H_MV_ENC_DEC_TRAC 
  DTRACE_CABAC_VL( g_nSymbolCounter++ );
  DTRACE_CABAC_T( "\tICFlag" );
  DTRACE_CABAC_T( "\tuiCtxIC: ");
  DTRACE_CABAC_V( uiCtxIC );
  DTRACE_CABAC_T( "\tuiSymbol: ");
  DTRACE_CABAC_V( uiSymbol );
  DTRACE_CABAC_T( "\n");
#else
  DTRACE_CU("ic_flag", uiSymbol)
#endif
  
  pcCU->setICFlagSubParts( uiSymbol ? true : false , uiAbsPartIdx, 0, uiDepth );
}
#endif

#if NH_3D_DMM || NH_3D_SDC_INTRA || H_3D_INTER_SDC
Void TDecSbac::parseDeltaDC( TComDataCU* pcCU, UInt absPartIdx, UInt depth )
{
#if NH_3D_SDC_INTRA || H_3D_INTER_SDC
  if( !(pcCU->getSDCFlag( absPartIdx )) )
#endif
#if NH_3D_DMM
  if( !(pcCU->isIntra( absPartIdx ) && isDmmMode( pcCU->getIntraDir( CHANNEL_TYPE_LUMA, absPartIdx ) )) )
#endif
    assert( 0 );

  UInt symbol = 1;
#if NH_3D_SDC_INTRA
  if( pcCU->isIntra( absPartIdx ) && pcCU->getSDCFlag( absPartIdx ))
  {
    m_pcTDecBinIf->decodeBin( symbol, m_cDdcFlagSCModel.get( 0, 0, 0 ) RExt__DECODER_DEBUG_BIT_STATISTICS_PASS_OPT_ARG(STATS__CABAC_BITS__SDC_DELTADC_FLAG) );
    assert( pcCU->getPartitionSize( absPartIdx ) == SIZE_2Nx2N );
    assert( pcCU->getTransformIdx(absPartIdx) == 0 );
    assert( pcCU->getCbf(absPartIdx, COMPONENT_Y) == 1 );
    }
#endif
  UInt uiNumSegments = 1;
#if NH_3D_DMM
  uiNumSegments = isDmmMode( pcCU->getIntraDir( CHANNEL_TYPE_LUMA, absPartIdx ) ) ? 2 : 1;
#endif

  for( UInt segment = 0; segment < uiNumSegments; segment++ )
  {
    Pel valDeltaDC = 0;
    if( symbol )
    {
      xParseDeltaDC( valDeltaDC, uiNumSegments );
    }

    if( pcCU->isIntra( absPartIdx ) )
    {
#if NH_3D_SDC_INTRA
      if( pcCU->getSDCFlag( absPartIdx ) )
      {
        pcCU->setSDCSegmentDCOffset( valDeltaDC, segment, absPartIdx );
      }
      else
      {
#endif
#if NH_3D_DMM
        pcCU->setDmmDeltaDC( getDmmType( pcCU->getIntraDir( CHANNEL_TYPE_LUMA, absPartIdx ) ), segment, absPartIdx, valDeltaDC );
#endif
#if NH_3D_SDC_INTRA
      }
#endif
    }
#if H_3D_INTER_SDC
    else
    {
      pcCU->setSDCSegmentDCOffset( valDeltaDC, segment, absPartIdx );
    }
#endif
  }
}

Void TDecSbac::xParseDeltaDC( Pel& rValDeltaDC, UInt uiNumSeg )
{
  UInt absValDeltaDC = 0;
  xReadExGolombLevelDdc( absValDeltaDC );
  rValDeltaDC = (Pel)absValDeltaDC + ( uiNumSeg > 1 ? 0 : 1 );

  if( rValDeltaDC != 0 )
  {
    UInt uiSign;
    m_pcTDecBinIf->decodeBinEP( uiSign RExt__DECODER_DEBUG_BIT_STATISTICS_PASS_OPT_ARG(STATS__CABAC_BITS__DELTADC_SIGN_EP) );
    if ( uiSign )
    {
      rValDeltaDC = -rValDeltaDC;
    }
  }
#if H_MV_ENC_DEC_TRAC
  DTRACE_CU("delta_dc", rValDeltaDC);
#endif
}

Void TDecSbac::xReadExGolombLevelDdc( UInt& ruiSymbol )
{
  UInt uiSymbol;
  UInt uiCount = 0;
  do
  {
    m_pcTDecBinIf->decodeBin( uiSymbol, m_cDdcDataSCModel.get(0, 0, 0) RExt__DECODER_DEBUG_BIT_STATISTICS_PASS_OPT_ARG(STATS__CABAC_BITS__DELTADC_PREFIX) );
    uiCount++;
  }
  while( uiSymbol && ( uiCount != 3 ) );
  ruiSymbol = uiCount - 1;

  if( uiSymbol )
  {
    xReadEpExGolomb( uiSymbol, 0 RExt__DECODER_DEBUG_BIT_STATISTICS_PASS_OPT_ARG(STATS__CABAC_BITS__DELTADC_SUFFIX_EP) );
    ruiSymbol += uiSymbol + 1;
  }

  return;
}
#endif
#if NH_3D_DMM
Void TDecSbac::xParseIntraDepthMode( TComDataCU* pcCU, UInt absPartIdx, UInt depth )
{
  pcCU->setIntraDirSubParts( CHANNEL_TYPE_LUMA, 0, absPartIdx, depth );

  UInt uiSymbol;
  if( ( pcCU->getSlice()->getSPS()->getMaxCUWidth() >> pcCU->getDepth( absPartIdx ) ) < 64 ) //DMM and HEVC intra modes are both allowed
  {
    m_pcTDecBinIf->decodeBin( uiSymbol, m_cNotDmmFlagSCModel.get( 0, 0, 0 ) RExt__DECODER_DEBUG_BIT_STATISTICS_PASS_OPT_ARG(STATS__CABAC_BITS__NOTDMM_FLAG) );
  }
  else
  {
    uiSymbol = 1;
  }

  //decode DMM mode index
  if( !uiSymbol )
  {
    if( pcCU->getSlice()->getIntraSdcWedgeFlag() && pcCU->getSlice()->getIntraContourFlag() )
    {
      m_pcTDecBinIf->decodeBin( uiSymbol, m_cDmmModeSCModel.get( 0, 0, 0 ) RExt__DECODER_DEBUG_BIT_STATISTICS_PASS_OPT_ARG(STATS__CABAC_BITS__DMMMODE) );
      if( !uiSymbol )
      {
        pcCU->setIntraDirSubParts( CHANNEL_TYPE_LUMA, DMM_OFFSET, absPartIdx, depth );
      }
      else
      {
        pcCU->setIntraDirSubParts( CHANNEL_TYPE_LUMA, ( 1+ DMM_OFFSET ), absPartIdx, depth );
      }
    }
    else if( pcCU->getSlice()->getIntraSdcWedgeFlag() )
    {
      pcCU->setIntraDirSubParts( CHANNEL_TYPE_LUMA, DMM_OFFSET, absPartIdx, depth );
    }
    else if( pcCU->getSlice()->getIntraContourFlag() )
    {
      pcCU->setIntraDirSubParts( CHANNEL_TYPE_LUMA, ( 1+ DMM_OFFSET ), absPartIdx, depth );
    }
  }
}

Void TDecSbac::xParseDmmData( TComDataCU* pcCU, UInt absPartIdx, UInt depth )
{
  UInt dir = pcCU->getIntraDir( CHANNEL_TYPE_LUMA, absPartIdx );
  switch( getDmmType( dir ) )
  {
  case( DMM1_IDX ):
    {
      UInt uiTabIdx = 0;
      xParseDmm1WedgeIdx( uiTabIdx, g_dmm1TabIdxBits[pcCU->getIntraSizeIdx(absPartIdx)] );
      assert( uiTabIdx < getWedgeListScaled( pcCU->getWidth( absPartIdx ) )->size() );
      pcCU->setDmm1WedgeTabIdxSubParts( uiTabIdx, absPartIdx, depth );
    } break;
  case( DMM4_IDX ): break;
  default: break;
  }
}

Void TDecSbac::xParseDmm1WedgeIdx( UInt& ruiTabIdx, Int iNumBit )
{
  UInt uiSymbol, uiIdx = 0;
  for( Int i = 0; i < iNumBit; i++ )
  {
    m_pcTDecBinIf->decodeBinEP( uiSymbol RExt__DECODER_DEBUG_BIT_STATISTICS_PASS_OPT_ARG(STATS__CABAC_BITS__DMM1_BITS_EP) );
    uiIdx += uiSymbol << i;
  }
  ruiTabIdx = uiIdx;
}
#endif
#if NH_3D_SDC_INTRA || H_3D_INTER_SDC
Void TDecSbac::parseSDCFlag( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth )
{
  UInt uiSymbol = 0;

  m_pcTDecBinIf->decodeBin( uiSymbol, m_cSDCFlagSCModel.get( 0, 0, 0 ) RExt__DECODER_DEBUG_BIT_STATISTICS_PASS_OPT_ARG(STATS__CABAC_BITS__SDC_INTRA_FLAG) );
#if H_MV_ENC_DEC_TRAC
  DTRACE_CU("dc_only_flag", uiSymbol)
#endif
  if( uiSymbol )
  {
    pcCU->setSDCFlagSubParts( true, uiAbsPartIdx, uiDepth );
    pcCU->setTrIdxSubParts( 0, uiAbsPartIdx, uiDepth );
    pcCU->setCbfSubParts(1, COMPONENT_Y, uiAbsPartIdx, uiDepth);
  }
  else
  {
    pcCU->setSDCFlagSubParts( false, uiAbsPartIdx, uiDepth );
  }
}

#endif

#if H_3D_DBBP
Void TDecSbac::parseDBBPFlag( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth )
{
  AOF( pcCU->getSlice()->getDepthBasedBlkPartFlag() );
  AOF( !pcCU->getSlice()->getIsDepth() );
  
  UInt uiSymbol = 0;
  
  m_pcTDecBinIf->decodeBin( uiSymbol, m_cDBBPFlagSCModel.get( 0, 0, 0 ) );
  DTRACE_CU("dbbp_flag", uiSymbol)
  PartSize ePartSize = pcCU->getPartitionSize( uiAbsPartIdx );
  AOF( ePartSize == SIZE_2NxN || ePartSize == SIZE_Nx2N );
  UInt uiPUOffset = ( g_auiPUOffset[UInt( ePartSize )] << ( ( pcCU->getSlice()->getSPS()->getMaxCUDepth() - uiDepth ) << 1 ) ) >> 4;
  pcCU->setDBBPFlagSubParts(uiSymbol, uiAbsPartIdx, 0, uiDepth);
  pcCU->setDBBPFlagSubParts(uiSymbol, uiAbsPartIdx+uiPUOffset, 1, uiDepth);
}
#endif


//! \}
