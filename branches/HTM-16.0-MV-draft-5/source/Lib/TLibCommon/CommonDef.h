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

/** \file     CommonDef.h
    \brief    Defines version information, constants and small in-line functions
*/

#ifndef __COMMONDEF__
#define __COMMONDEF__

#include <algorithm>
#include <iostream>
#include <assert.h>
#include <limits>

#if _MSC_VER > 1000
// disable "signed and unsigned mismatch"
#pragma warning( disable : 4018 )
// disable Bool coercion "performance warning"
#pragma warning( disable : 4800 )
// NH_MV
// disabled decorated name length warning issued for IntAry5d
#pragma warning(disable : 4503)
// 
#endif // _MSC_VER > 1000
#include "TypeDef.h"
#ifdef _MSC_VER
#if _MSC_VER <= 1500
inline Int64 abs (Int64 x) { return _abs64(x); };
#endif
#endif
#if NH_MV
#include <assert.h>
#endif

//! \ingroup TLibCommon
//! \{

// ====================================================================================================================
// Version information
// ====================================================================================================================
#if NH_MV
#define NV_VERSION        "15.2"                ///< Current software version
#define HM_VERSION        "16.6"                ///< 
#else
#define NV_VERSION        "16.7"                 ///< Current software version
#endif
// ====================================================================================================================
// Platform information
// ====================================================================================================================

#ifdef __GNUC__
#define NVM_COMPILEDBY  "[GCC %d.%d.%d]", __GNUC__, __GNUC_MINOR__, __GNUC_PATCHLEVEL__
#ifdef __IA64__
#define NVM_ONARCH    "[on 64-bit] "
#else
#define NVM_ONARCH    "[on 32-bit] "
#endif
#endif

#ifdef __INTEL_COMPILER
#define NVM_COMPILEDBY  "[ICC %d]", __INTEL_COMPILER
#elif  _MSC_VER
#define NVM_COMPILEDBY  "[VS %d]", _MSC_VER
#endif

#ifndef NVM_COMPILEDBY
#define NVM_COMPILEDBY "[Unk-CXX]"
#endif

#ifdef _WIN32
#define NVM_ONOS        "[Windows]"
#elif  __linux
#define NVM_ONOS        "[Linux]"
#elif  __CYGWIN__
#define NVM_ONOS        "[Cygwin]"
#elif __APPLE__
#define NVM_ONOS        "[Mac OS X]"
#else
#define NVM_ONOS "[Unk-OS]"
#endif

#define NVM_BITS          "[%d bit] ", (sizeof(Void*) == 8 ? 64 : 32) ///< used for checking 64-bit O/S

#ifndef NULL
#define NULL              0
#endif

// ====================================================================================================================
// Common constants
// ====================================================================================================================

static const UInt   MAX_UINT =                            0xFFFFFFFFU; ///< max. value of unsigned 32-bit integer
static const Int    MAX_INT =                              2147483647; ///< max. value of signed 32-bit integer
#if NH_MV
static const Int    MIN_INT =                         (- MAX_INT - 1); ///< max. value of signed 32-bit integer
#endif
static const Double MAX_DOUBLE =                             1.7e+308; ///< max. value of Double-type value

// ====================================================================================================================
// Coding tool configuration
// ====================================================================================================================
// Most of these should not be changed - they resolve the meaning of otherwise magic numbers.

static const Int MAX_GOP =                                         64; ///< max. value of hierarchical GOP size
static const Int MAX_NUM_REF_PICS =                                16; ///< max. number of pictures used for reference
static const Int MAX_NUM_REF =                                     16; ///< max. number of entries in picture reference list
static const Int MAX_QP =                                          51;
static const Int NOT_VALID =                                       -1;

static const Int AMVP_MAX_NUM_CANDS =                               2; ///< AMVP: advanced motion vector prediction - max number of final candidates
static const Int AMVP_DECIMATION_FACTOR =                           4;
static const Int MRG_MAX_NUM_CANDS =                                5; ///< MERGE


static const Int MAX_TLAYER =                                       7; ///< Explicit temporal layer QP offset - max number of temporal layer

static const Int ADAPT_SR_SCALE =                                   1; ///< division factor for adaptive search range

static const Int MAX_NUM_PICS_IN_SOP =                           1024;

static const Int MAX_NESTING_NUM_OPS =                           1024;
static const Int MAX_NESTING_NUM_LAYER =                           64;

#if NH_MV
static const Int MAX_VPS_NUM_HRD_PARAMETERS =                    1024;
#else
static const Int MAX_VPS_NUM_HRD_PARAMETERS =                       1;
#endif
static const Int MAX_VPS_OP_SETS_PLUS1 =                         1024;
#if NH_MV
static const Int MAX_VPS_NUH_LAYER_ID_PLUS1 =                      63;                
#else
static const Int MAX_VPS_NUH_RESERVED_ZERO_LAYER_ID_PLUS1 =         1;
#endif

static const Int MAXIMUM_INTRA_FILTERED_WIDTH =                    16;
static const Int MAXIMUM_INTRA_FILTERED_HEIGHT =                   16;

static const Int MAX_CPB_CNT =                                     32; ///< Upper bound of (cpb_cnt_minus1 + 1)
#if NH_MV
static const Int MAX_NUM_LAYER_IDS =                               63;
static const Int MAX_NUM_SEIS      =                               1000;
#else
static const Int MAX_NUM_LAYER_IDS =                               64;
#endif

static const Int COEF_REMAIN_BIN_REDUCTION =                        3; ///< indicates the level at which the VLC transitions from Golomb-Rice to TU+EG(k)

static const Int CU_DQP_TU_CMAX =                                   5; ///< max number bins for truncated unary
static const Int CU_DQP_EG_k =                                      0; ///< expgolomb order

static const Int SBH_THRESHOLD =                                    4; ///< value of the fixed SBH controlling threshold

static const Int C1FLAG_NUMBER =                                    8; // maximum number of largerThan1 flag coded in one chunk:  16 in HM5
static const Int C2FLAG_NUMBER =                                    1; // maximum number of largerThan2 flag coded in one chunk:  16 in HM5

static const Int MAX_NUM_VPS =                                     16;
static const Int MAX_NUM_SPS =                                     16;
static const Int MAX_NUM_PPS =                                     64;


static const Int MLS_GRP_NUM =                                     64; ///< Max number of coefficient groups, max(16, 64)
static const Int MLS_CG_LOG2_WIDTH =                                2;
static const Int MLS_CG_LOG2_HEIGHT =                               2;
static const Int MLS_CG_SIZE =                                      4; ///< Coefficient group size of 4x4; = MLS_CG_LOG2_WIDTH + MLS_CG_LOG2_HEIGHT

#if ADAPTIVE_QP_SELECTION
static const Int ARL_C_PRECISION =                                  7; ///< G382: 7-bit arithmetic precision
static const Int LEVEL_RANGE =                                     30; ///< G382: max coefficient level in statistics collection
#endif

static const Int RVM_VCEGAM10_M =                                   4;

static const Int FAST_UDI_MAX_RDMODE_NUM =                         35; ///< maximum number of RD comparison in fast-UDI estimation loop

static const Int NUM_INTRA_MODE =                                  36;
static const Int PLANAR_IDX =                                       0;
static const Int VER_IDX =                                         26; ///< index for intra VERTICAL   mode
static const Int HOR_IDX =                                         10; ///< index for intra HORIZONTAL mode
static const Int DC_IDX =                                           1; ///< index for intra DC mode
static const Int NUM_CHROMA_MODE =                                  5; ///< total number of chroma modes
static const Int DM_CHROMA_IDX =                                   36; ///< chroma mode index for derived from luma intra mode

static const Int MDCS_ANGLE_LIMIT =                                 4; ///< 0 = Horizontal/vertical only, 1 = Horizontal/vertical +/- 1, 2 = Horizontal/vertical +/- 2 etc...
static const Int MDCS_MAXIMUM_WIDTH =                               8; ///< (measured in pixels) TUs with width greater than this can only use diagonal scan
static const Int MDCS_MAXIMUM_HEIGHT =                              8; ///< (measured in pixels) TUs with height greater than this can only use diagonal scan


static const Int LOG2_MAX_NUM_COLUMNS_MINUS1 =                      7;
static const Int LOG2_MAX_NUM_ROWS_MINUS1 =                         7;

static const Int CABAC_INIT_PRESENT_FLAG =                          1;

static const Int LUMA_INTERPOLATION_FILTER_SUB_SAMPLE_POSITIONS =   4;
static const Int CHROMA_INTERPOLATION_FILTER_SUB_SAMPLE_POSITIONS = 8;

static const Int MAX_NUM_LONG_TERM_REF_PICS =                      33;
static const Int NUM_LONG_TERM_REF_PIC_SPS =                        0;


static const Int MAX_QP_OFFSET_LIST_SIZE =                          6; ///< Maximum size of QP offset list is 6 entries

// Cost mode support
static const Int LOSSLESS_AND_MIXED_LOSSLESS_RD_COST_TEST_QP =      0; ///< QP to use for lossless coding.
static const Int LOSSLESS_AND_MIXED_LOSSLESS_RD_COST_TEST_QP_PRIME =4; ///< QP' to use for mixed_lossy_lossless coding.

static const Int RExt__GOLOMB_RICE_ADAPTATION_STATISTICS_SETS =     4;
static const Int RExt__GOLOMB_RICE_INCREMENT_DIVISOR =              4;

static const Int RExt__PREDICTION_WEIGHTING_ANALYSIS_DC_PRECISION = 0; ///< Additional fixed bit precision used during encoder-side weighting prediction analysis. Currently only used when high_precision_prediction_weighting_flag is set, for backwards compatibility reasons.

static const Int MAX_TIMECODE_SEI_SETS =                            3; ///< Maximum number of time sets

static const Int MAX_CU_DEPTH =                                     6; ///< log2(CTUSize)
static const Int MAX_CU_SIZE =                                     64; ///< = 1<<(MAX_CU_DEPTH)
static const Int MIN_PU_SIZE =                                      4;
static const Int MIN_TU_SIZE =                                      4;
static const Int MAX_TU_SIZE =                                     32;
static const Int MAX_NUM_PART_IDXS_IN_CTU_WIDTH = MAX_CU_SIZE/MIN_PU_SIZE; ///< maximum number of partition indices across the width of a CTU (or height of a CTU)
static const Int SCALING_LIST_REM_NUM =                             6;

static const Int QUANT_SHIFT =                                     14; ///< Q(4) = 2^14
static const Int IQUANT_SHIFT =                                     6;
static const Int SCALE_BITS =                                      15; ///< For fractional bit estimates in RDOQ

static const Int SCALING_LIST_NUM = MAX_NUM_COMPONENT * NUMBER_OF_PREDICTION_MODES; ///< list number for quantization matrix

static const Int SCALING_LIST_START_VALUE =                        8 ; ///< start value for dpcm mode
static const Int MAX_MATRIX_COEF_NUM =                            64 ; ///< max coefficient number for quantization matrix
static const Int MAX_MATRIX_SIZE_NUM =                             8 ; ///< max size number for quantization matrix
static const Int SCALING_LIST_BITS =                               8 ; ///< bit depth of scaling list entries
static const Int LOG2_SCALING_LIST_NEUTRAL_VALUE =                 4 ; ///< log2 of the value that, when used in a scaling list, has no effect on quantisation
static const Int SCALING_LIST_DC =                                16 ; ///< default DC value

static const Int CONTEXT_STATE_BITS =                              6 ;
static const Int LAST_SIGNIFICANT_GROUPS =                        10 ;

#if NH_MV
static const Int  MAX_VPS_NUM_ADD_LAYER_SETS =                  1024 ;
static const Int  MAX_NUM_SCALABILITY_TYPES =                     16 ;
static const Int  ENC_CFG_CONSOUT_SPACE =                         34 ;           
static const Int  MAX_NUM_LAYERS =                                63 ;
static const Int  MAX_VPS_PROFILE_TIER_LEVEL =                    64 ;
static const Int  MAX_VPS_ADD_OUTPUT_LAYER_SETS =               1024 ;
static const Int  MAX_VPS_OUTPUTLAYER_SETS =  MAX_VPS_ADD_OUTPUT_LAYER_SETS + MAX_VPS_OP_SETS_PLUS1 + MAX_VPS_OP_SETS_PLUS1 ;
static const Int  MAX_NUM_VIDEO_SIGNAL_INFO =                     16 ;
static const Int  MAX_NUM_SCALED_REF_LAYERS =     MAX_NUM_LAYERS - 1 ; 
static const Int  MAX_NUM_PICS_RPS          =                     16 ; 
static const Int  MAX_NUM_REF_LAYERS        =                     63 ;  

static IntAry1d getRangeVec( Int rngStart, Int rngEnd ) { IntAry1d rng; for (Int i = rngStart; i<=rngEnd; i++) rng.push_back(i);  return rng; };
static const IntAry1d IDR_NAL_UNIT_TYPES   = getRangeVec( NAL_UNIT_CODED_SLICE_IDR_W_RADL, NAL_UNIT_CODED_SLICE_IDR_N_LP ); 
static const IntAry1d IRAP_NAL_UNIT_TYPES  = getRangeVec( NAL_UNIT_CODED_SLICE_BLA_W_LP  , NAL_UNIT_CODED_SLICE_CRA      ); 
#endif

// ====================================================================================================================
// Macro functions
// ====================================================================================================================

template <typename T> inline T Clip3 (const T minVal, const T maxVal, const T a) { return std::min<T> (std::max<T> (minVal, a) , maxVal); }  ///< general min/max clip
template <typename T> inline T ClipBD(const T x, const Int bitDepth)             { return Clip3(T(0), T((1 << bitDepth)-1), x);           }

template <typename T> inline Void Check3( T minVal, T maxVal, T a)
{
  if ((a > maxVal) || (a < minVal))
  {
    std::cerr << "ERROR: Range check " << minVal << " >= " << a << " <= " << maxVal << " failed" << std::endl;
    assert(false);
    exit(1);
  }
}  ///< general min/max clip

#define DATA_ALIGN                  1                                                                 ///< use 32-bit aligned malloc/free
#if     DATA_ALIGN && _WIN32 && ( _MSC_VER > 1300 )
#define xMalloc( type, len )        _aligned_malloc( sizeof(type)*(len), 32 )
#define xFree( ptr )                _aligned_free  ( ptr )
#else
#define xMalloc( type, len )        malloc   ( sizeof(type)*(len) )
#define xFree( ptr )                free     ( ptr )
#endif

#define FATAL_ERROR_0(MESSAGE, EXITCODE)                      \
{                                                             \
  printf(MESSAGE);                                            \
  exit(EXITCODE);                                             \
}

template <typename ValueType> inline ValueType leftShift       (const ValueType value, const Int shift) { return (shift >= 0) ? ( value                                  << shift) : ( value                                   >> -shift); }
template <typename ValueType> inline ValueType rightShift      (const ValueType value, const Int shift) { return (shift >= 0) ? ( value                                  >> shift) : ( value                                   << -shift); }
template <typename ValueType> inline ValueType leftShift_round (const ValueType value, const Int shift) { return (shift >= 0) ? ( value                                  << shift) : ((value + (ValueType(1) << (-shift - 1))) >> -shift); }
template <typename ValueType> inline ValueType rightShift_round(const ValueType value, const Int shift) { return (shift >= 0) ? ((value + (ValueType(1) << (shift - 1))) >> shift) : ( value                                   << -shift); }
#if O0043_BEST_EFFORT_DECODING
// when shift = 0, returns value
// when shift = 1, (value + 0 + value[1]) >> 1
// when shift = 2, (value + 1 + value[2]) >> 2
// when shift = 3, (value + 3 + value[3]) >> 3
template <typename ValueType> inline ValueType rightShiftEvenRounding(const ValueType value, const UInt shift) { return (shift == 0) ? value : ((value + (1<<(shift-1))-1 + ((value>>shift)&1)) >> shift) ; }
#endif
#if NH_MV

#define AOF( exp )                  \
{                                   \
  if( !( exp ) )                    \
{                                 \
  assert( 0 );                    \
}                                 \
}

#define AOT( exp )            \
{                             \
  if( ( exp ) )               \
{                           \
  assert( 0 );              \
}                           \
}

template <typename T>
__inline T gSign(const T& t)
{
  if( t == 0 )
    return T(0);
  else
    return (t < 0) ? T(-1) : T(1);
}

template <typename T>
__inline T gCeilLog2( T val )
{
  assert( val > 0 ); 
  Int ceilLog2 = 0;
  while( val > ( 1 << ceilLog2 ) ) ceilLog2++;
  return ceilLog2;
}

#define RemoveBitIncrement( exp ) ( exp >> ( REN_BIT_DEPTH - 8 ) )

#endif


#if NH_MV
static const std::string NALU_TYPE_STR[] = {  
    "CODED_SLICE_TRAIL_N   ",  // 0
    "CODED_SLICE_TRAIL_R   ",  // 1
    "CODED_SLICE_TSA_N     ",  // 2
    "CODED_SLICE_TSA_R     ",  // 3
    "CODED_SLICE_STSA_N    ",  // 4
    "CODED_SLICE_STSA_R    ",  // 5
    "CODED_SLICE_RADL_N    ",  // 6
    "CODED_SLICE_RADL_R    ",  // 7
    "CODED_SLICE_RASL_N    ",  // 8
    "CODED_SLICE_RASL_R    ",  // 9
    "RESERVED_VCL_N10      ",
    "RESERVED_VCL_R11      ",
    "RESERVED_VCL_N12      ",
    "RESERVED_VCL_R13      ",
    "RESERVED_VCL_N14      ",
    "RESERVED_VCL_R15      ",
    "CODED_SLICE_BLA_W_LP  ",  // 16
    "CODED_SLICE_BLA_W_RADL",  // 17
    "CODED_SLICE_BLA_N_LP  ",  // 18
    "CODED_SLICE_IDR_W_RADL",  // 19
    "CODED_SLICE_IDR_N_LP  ",  // 20
    "CODED_SLICE_CRA       ",  // 21
    "RESERVED_IRAP_VCL22   ",
    "RESERVED_IRAP_VCL23   ",
    "RESERVED_VCL24        ",
    "RESERVED_VCL25        ",
    "RESERVED_VCL26        ",
    "RESERVED_VCL27        ",
    "RESERVED_VCL28        ",
    "RESERVED_VCL29        ",
    "RESERVED_VCL30        ",
    "RESERVED_VCL31        ",
    "VPS                   ",   // 32
    "SPS                   ",   // 33
    "PPS                   ",   // 34
    "ACCESS_UNIT_DELIMITER ",   // 35
    "EOS                   ",   // 36
    "EOB                   ",   // 37
    "FILLER_DATA           ",   // 38
    "PREFIX_SEI            ",   // 39
    "SUFFIX_SEI            ",   // 40
    "RESERVED_NVCL41       ",
    "RESERVED_NVCL42       ",
    "RESERVED_NVCL43       ",
    "RESERVED_NVCL44       ",
    "RESERVED_NVCL45       ",
    "RESERVED_NVCL46       ",
    "RESERVED_NVCL47       ",
    "UNSPECIFIED_48        ",
    "UNSPECIFIED_49        ",
    "UNSPECIFIED_50        ",
    "UNSPECIFIED_51        ",
    "UNSPECIFIED_52        ",
    "UNSPECIFIED_53        ",
    "UNSPECIFIED_54        ",
    "UNSPECIFIED_55        ",
    "UNSPECIFIED_56        ",
    "UNSPECIFIED_57        ",
    "UNSPECIFIED_58        ",
    "UNSPECIFIED_59        ",
    "UNSPECIFIED_60        ",
    "UNSPECIFIED_61        ",
    "UNSPECIFIED_62        ",
    "UNSPECIFIED_63        ",
    "INVALID               "               
  };
#endif

//! \}

#endif // end of #ifndef  __COMMONDEF__

