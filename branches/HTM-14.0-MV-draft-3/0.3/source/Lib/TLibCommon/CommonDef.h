/* The copyright in this software is being made available under the BSD
 * License, included below. This software may be subject to other third party
 * and contributor rights, including patent rights, and no such rights are
 * granted under this license.
 *
 * Copyright (c) 2010-2011, ISO/IEC
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
 *  * Neither the name of the ISO/IEC nor the names of its contributors may
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
    \brief    Defines constants, macros and tool parameters
*/

#ifndef __COMMONDEF__
#define __COMMONDEF__

// this pragma can be used for turning off "signed and unsigned mismatch"
#if _MSC_VER > 1000
#pragma warning( disable : 4018 )
#endif // _MSC_VER > 1000
#include "TypeDef.h"
#include "TComRom.h"

#include <string>
#include <assert.h>
#include <stdlib.h>
#include <vector>

// ====================================================================================================================
// Version information
// ====================================================================================================================

#define HM_VERSION        "3.0rc2"                 ///< Current software version
#define NV_VERSION        "0.3"                    ///< Current software version

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

#define NVM_BITS          "[%d bit] ", (sizeof(void*) == 8 ? 64 : 32) ///< used for checking 64-bit O/S

#ifndef NULL
#define NULL              0
#endif


// ====================================================================================================================
// Common constants
// ====================================================================================================================

#define _SUMMARY_OUT_               0           ///< print-out PSNR results of all slices to summary.txt
#define _SUMMARY_PIC_               0           ///< print-out PSNR results for each slice type to summary.txt

#define MAX_REF_PIC_NUM             64
#define MAX_GOP                     64          ///< max. value of hierarchical GOP size

#define MAX_NUM_REF                 4           ///< max. value of multiple reference frames
#if DCM_COMB_LIST
#define MAX_NUM_REF_LC              8           ///< max. value of combined reference frames
#endif

#define MAX_UINT                    0xFFFFFFFFU ///< max. value of unsigned 32-bit integer
#define MAX_UINT64                  0xFFFFFFFFFFFFFFFFU
#define MAX_INT                     2147483647  ///< max. value of signed 32-bit integer
#define MIN_INT                     (-2147483647-1) // < min. value of signed 32-bit integer
#define MAX_DOUBLE                  1.7e+308    ///< max. value of double-type value

#define MIN_QP                      0
#define MAX_QP                      51

#define NOT_VALID                   -1

#define STD_CAM_PARAMETERS_PRECISION 5        ///< quarter luma sample accuarcy for derived disparities (as default)

#define MAX_INPUT_VIEW_NUM					10				///< max. number of input view for multiview coding

#if HHI_VSO
#define MAX_ERREF_VIEW_NUM					15				///< max. number of virtual external reference views
#endif
#define LOG2_DISP_PREC_LUT   				2		  		///< log2 of disparity precision used in integer disparity LUTs


#if ( HHI_INTER_VIEW_MOTION_PRED || HHI_INTER_VIEW_RESIDUAL_PRED )
#define DEPTH_MAP_GENERATION        1
#else
#define DEPTH_MAP_GENERATION        0
#endif

//>>>>> generation and usage of virtual prediction depth maps >>>>>
#define PDM_ONE_DEPTH_PER_PU              1         // use only a single depth for a prediction unit (in update)
#define PDM_NO_INTER_UPDATE               1         // no update for inter (but not inter-view) predicted blocks
#define PDM_MERGE_POS                     0         // position of pdm in merge list (0..5)
#define PDM_AMVP_POS                      2         // position of pdm in amvp list  (0..3)
#define PDM_OUTPUT_PRED_DEPTH_MAP         0         // output prediction depth map (for debugging)

#define PDM_INTERNAL_CALC_BIT_DEPTH       31        // bit depth for internal calculations (32 - 1 for signed values)
#define PDM_BITDEPTH_VIRT_DEPTH           15        // bit depth for virtual depth storage (16 - 1 for signed values)
#define PDM_LOG2_MAX_ABS_NORMAL_DISPARITY 8         // maximum absolute normal disparity = 256 (for setting accuracy)
#define PDM_VIRT_DEPTH_PRECISION          4         // must be greater than or equal to 2 (since MVs are given in quarter-pel units)

#define PDM_INTER_CALC_SHIFT              ( PDM_INTERNAL_CALC_BIT_DEPTH - PDM_BITDEPTH_VIRT_DEPTH )         // avoids overflow
#define PDM_LOG4_SCALE_DENOMINATOR        ( PDM_LOG2_MAX_ABS_NORMAL_DISPARITY + PDM_VIRT_DEPTH_PRECISION )  // accuracy of scaling factor
#define PDM_OFFSET_SHIFT                  ( PDM_LOG2_MAX_ABS_NORMAL_DISPARITY )                             // accuracy of offset

#define PDM_MAX_ABS_VIRT_DEPTH            (  ( 1 << PDM_BITDEPTH_VIRT_DEPTH ) - 1 )
#define PDM_UNDEFINED_DEPTH               ( -( 1 << PDM_BITDEPTH_VIRT_DEPTH )     )

#define PDM_USE_FOR_IVIEW                 1
#define PDM_USE_FOR_INTER                 2
#define PDM_USE_FOR_MERGE                 4

#define PDM_SUBSAMPLING_EXP               2         // subsampling factor is 2^PDM_SUBSAMPLING_EXP 
#define PDM_SUB_SAMP_EXP_X(Pdm)           ((Pdm)==1?PDM_SUBSAMPLING_EXP:0)
#define PDM_SUB_SAMP_EXP_Y(Pdm)           ((Pdm)==1?PDM_SUBSAMPLING_EXP:0)
//<<<<< generation and usage of virtual prediction depth maps <<<<<

#define OUTPUT_RESIDUAL_PICTURES          0         // output residual pictures (for debugging)

#define HHI_MPI_MERGE_POS                     0         // position of mvi in merge list (0..5)


// ====================================================================================================================
// Macro functions
// ====================================================================================================================

#define Max(x, y)                   ((x)>(y)?(x):(y))                                                 ///< max of (x, y)
#define Min(x, y)                   ((x)<(y)?(x):(y))                                                 ///< min of (x, y)
#define Median(a,b,c)               ((a)>(b)?(a)>(c)?(b)>(c)?(b):(c):(a):(b)>(c)?(a)>(c)?(a):(c):(b)) ///< 3-point median
#define Clip(x)                     ( Min(g_uiIBDI_MAX, Max( 0, (x)) ) )                              ///< clip with bit-depth range
#define Clip3( MinVal, MaxVal, a)   ( ((a)<(MinVal)) ? (MinVal) : (((a)>(MaxVal)) ? (MaxVal) :(a)) )  ///< general min/max clip
#define RemoveBitIncrement(x)       ( (x + ( (1 << g_uiBitIncrement) >> 1 )) >> g_uiBitIncrement )     ///< Remove Bit increment

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


// ====================================================================================================================
// Coding tool configuration
// ====================================================================================================================

// modified LCEC coefficient coding
#if QC_MOD_LCEC
#define MAX_TR1                           4
#endif

// AMVP: advanced motion vector prediction
#define AMVP_MAX_NUM_CANDS          6           ///< max number of final candidates
// MERGE
#define MRG_MAX_NUM_CANDS           6

// Reference memory management
#define DYN_REF_FREE                0           ///< dynamic free of reference memories

// defines for multiview coding
#define MAX_NUMBER_VIEWS            10

// Explicit temporal layer QP offset
#define MAX_TLAYER                  10           ///< max number of temporal layer
#define HB_LAMBDA_FOR_LDC           1           ///< use of B-style lambda for non-key pictures in low-delay mode

// Fast estimation of generalized B in low-delay mode
#define GPB_SIMPLE                  1           ///< Simple GPB mode
#if     GPB_SIMPLE
#define GPB_SIMPLE_UNI              1           ///< Simple mode for uni-direction
#endif

// Fast ME using smoother MV assumption
#define FASTME_SMOOTHER_MV          1           ///< reduce ME time using faster option

// Adaptive search range depending on POC difference
#define ADAPT_SR_SCALE              1           ///< division factor for adaptive search range

#define ENABLE_IBDI                 0

#define CLIP_TO_709_RANGE           0

// IBDI range restriction for skipping clip
#define IBDI_NOCLIP_RANGE           0           ///< restrict max. value after IBDI to skip clip

// Early-skip threshold (encoder)
#define EARLY_SKIP_THRES            1.50        ///< if RD < thres*avg[BestSkipRD]

const int g_iShift8x8    = 2;
const int g_iShift16x16  = 2;
const int g_iShift32x32  = 2;
const int g_iShift64x64  = 2;



//RENDERER
#define REN_LUMA_MARGIN   ( g_uiMaxCUWidth + 12 )
#define REN_VDWEIGHT_PREC  8
#define REN_IS_FILLED     ( 1 << REN_VDWEIGHT_PREC )
#define REN_USED_PEL       g_uiIBDI_MAX
#define REN_UNUSED_PEL     0
#define REN_IS_HOLE        0

/* End of Rounding control */

enum NalRefIdc
{
  NAL_REF_IDC_PRIORITY_LOWEST = 0,
  NAL_REF_IDC_PRIORITY_LOW,
  NAL_REF_IDC_PRIORITY_HIGH,
  NAL_REF_IDC_PRIORITY_HIGHEST
};

enum NalUnitType
{
  NAL_UNIT_UNSPECIFIED_0 = 0,
  NAL_UNIT_CODED_SLICE,
  NAL_UNIT_CODED_SLICE_DATAPART_A,
  NAL_UNIT_CODED_SLICE_DATAPART_B,
#if DCM_DECODING_REFRESH
  NAL_UNIT_CODED_SLICE_CDR,
#else
  NAL_UNIT_CODED_SLICE_DATAPART_C,
#endif
  NAL_UNIT_CODED_SLICE_IDR,
  NAL_UNIT_SEI,
  NAL_UNIT_SPS,
  NAL_UNIT_PPS,
  NAL_UNIT_ACCESS_UNIT_DELIMITER,
  NAL_UNIT_END_OF_SEQUENCE,
  NAL_UNIT_END_OF_STREAM,
  NAL_UNIT_FILLER_DATA,
  NAL_UNIT_RESERVED_13,
  NAL_UNIT_RESERVED_14,
  NAL_UNIT_RESERVED_15,
  NAL_UNIT_RESERVED_16,
  NAL_UNIT_RESERVED_17,
  NAL_UNIT_RESERVED_18,
  NAL_UNIT_RESERVED_19,
  NAL_UNIT_RESERVED_20,
  NAL_UNIT_RESERVED_21,
  NAL_UNIT_RESERVED_22,
  NAL_UNIT_RESERVED_23,
  NAL_UNIT_UNSPECIFIED_24,
  NAL_UNIT_UNSPECIFIED_25,
  NAL_UNIT_UNSPECIFIED_26,
  NAL_UNIT_UNSPECIFIED_27,
  NAL_UNIT_UNSPECIFIED_28,
  NAL_UNIT_UNSPECIFIED_29,
  NAL_UNIT_UNSPECIFIED_30,
  NAL_UNIT_UNSPECIFIED_31,
  NAL_UNIT_INVALID,
};

typedef _AlfParam    ALFParam;
#if MTK_SAO
typedef _SaoParam    SAOParam;
#endif


class VideoCodecException
{
public:
  VideoCodecException() : m_acFile( "unspecified" ), m_uiLine( 0 ) {}
  VideoCodecException( const char *pcFile, UInt uiLine ) : m_acFile( pcFile ), m_uiLine( uiLine ) {}

  std::string exceptionFile() const { return m_acFile; }
  UInt        exceptionLine() const { return m_uiLine; }

private:
  std::string m_acFile;
  UInt        m_uiLine;
};

#define RETURN_ERR_ON_EXCEPTION \
catch( std::exception &cEx ) \
{ \
  std::cerr << "Abnormal program termination: " << cEx.what() << std::endl; \
  return Err::m_nERR; \
} \
catch( VideoCodecException &cEx ) \
{ \
  std::cerr << std::endl << "VideoCodecExcteption caught at: " << std::endl; \
  std::cerr << "File: " << cEx.exceptionFile() << std::endl; \
  std::cerr << "Line: " << cEx.exceptionLine() << std::endl; \
  return Err::m_nERR; \
} \
catch( ... ) \
{ \
  std::cerr << "Caught unknown exception" << std::endl; \
  return Err::m_nERR; \
}


#define TOT(  exp ) { if(                exp  ) { throw VideoCodecException( __FILE__, __LINE__ ); } }
#define TOF(  exp ) { if(              !(exp) ) { throw VideoCodecException( __FILE__, __LINE__ ); } }
#define TNOK( exp ) { if( Err::m_nOK != (exp) ) { throw VideoCodecException( __FILE__, __LINE__ ); } }

#if defined( _DEBUG )

  #define TOT_DBG( exp )    TOT( exp )
  #define TOF_DBG( exp )    TOF( exp )

#else

  #define TOT_DBG( exp )    ((void)0)
  #define TOF_DBG( exp )    ((void)0)

#endif

typedef int            ErrVal;


class Err
{
public:
  enum
  {
    m_nOK               =  0,
    m_nERR              = -1,
    m_nEndOfFile        = -2,
    m_nEndOfBuffer      = -3,
    m_nInvalidParameter = -4,
    m_nDataNotAvailable = -5,
    m_nSizeNotAvailable = -5,
  };
};


#define ROF( exp )            \
{                             \
  if( !( exp ) )              \
  {                           \
    assert( 0 );              \
    return -1;                \
  }                           \
}

#define ROT( exp )            \
{                             \
  if( ( exp ) )               \
  {                           \
    assert( 0 );              \
    return -1;                \
  }                           \
}

#define ROFS( exp )           \
{                             \
  if( !( exp ) )              \
  {                           \
    return -1;                \
  }                           \
}

#define ROTS( exp )           \
{                             \
  if( ( exp ) )               \
  {                           \
    return -1;                \
  }                           \
}

#define ROFR( exp, retVal )   \
{                             \
  if( !( exp ) )              \
  {                           \
    assert( 0 );              \
    return retVal;            \
  }                           \
}

#define ROTR( exp, retVal )   \
{                             \
  if( ( exp ) )               \
  {                           \
    assert( 0 );              \
    return retVal;            \
  }                           \
}

#define ROFRS( exp, retVal )  \
{                             \
  if( !( exp ) )              \
  {                           \
    return retVal;            \
  }                           \
}

#define ROTRS( exp, retVal )  \
{                             \
  if( ( exp ) )               \
  {                           \
    return retVal;            \
  }                           \
}

#define ROFV( exp )           \
{                             \
  if( !( exp ) )              \
  {                           \
    assert( 0 );              \
    return;                   \
  }                           \
}

#define ROTV( exp )           \
{                             \
  if( ( exp ) )               \
  {                           \
    assert( 0 );              \
    return;                   \
  }                           \
}

#define ROFVS( exp )          \
{                             \
  if( !( exp ) )              \
  {                           \
    return;                   \
  }                           \
}

#define ROTVS( exp )          \
{                             \
  if( ( exp ) )               \
  {                           \
    return;                   \
  }                           \
}

#define RNOK( exp )                   \
{                                     \
  const ErrVal nMSysRetVal = ( exp ); \
  if( 0 != nMSysRetVal )              \
  {                                   \
    assert( 0 );                      \
    return nMSysRetVal;               \
  }                                   \
}

#define RNOKR( exp, retVal )        \
{                                   \
  if( 0 != ( exp ) )                \
  {                                 \
    assert( 0 );                    \
    return retVal;                  \
  }                                 \
}

#define RNOKS( exp )                  \
{                                     \
  const ErrVal nMSysRetVal = ( exp ); \
  if( 0 != nMSysRetVal )              \
  {                                   \
    return nMSysRetVal;               \
  }                                   \
}

#define RNOKRS( exp, retVal )       \
{                                   \
  if( 0 != ( exp ) )                \
  {                                 \
    return retVal;                  \
  }                                 \
}

#define RNOKV( exp )                \
{                                   \
  if( 0 != ( exp ) )                \
  {                                 \
    assert( 0 );                    \
    return;                         \
  }                                 \
}

#define RNOKVS( exp )               \
{                                   \
  if( 0 != ( exp ) )                \
  {                                 \
    return;                         \
  }                                 \
}

#define ANOK( exp )                 \
{                                   \
  if( 0 != ( exp ) )                \
  {                                 \
    assert( 0 );                    \
  }                                 \
}

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

#if defined( _DEBUG )

  #define AOT_DBG( exp )    AOT( exp )
  #define AOF_DBG( exp )    AOF( exp )

#else

  #define AOT_DBG( exp )    ((void)0)
  #define AOF_DBG( exp )    ((void)0)

#endif

#define DISABLE_DEFAULT_CPYCTOR_AND_ASOP(x) private: x( const x &r ); const x& operator=( const x &r )
typedef Int64 PicOrderCnt;

template<class T>
class AutoDeletePtrVector
  : public std::vector<T*>
{
public:
  AutoDeletePtrVector() {}

  virtual ~AutoDeletePtrVector()
  {
    while( !std::vector<T*>::empty() )
    {
      delete std::vector<T*>::back();
      std::vector<T*>::pop_back();
    }
  }

  DISABLE_DEFAULT_CPYCTOR_AND_ASOP( AutoDeletePtrVector );
};

  template<class T>
  __inline T gAbs(const T& x)
  {
    return x < 0 ? -x : x;
  }
//

template <typename T>
__inline T gSign(const T& t)
{
    if( t == 0 )
        return T(0);
    else
        return (t < 0) ? T(-1) : T(1);
}



#endif // end of #ifndef  __COMMONDEF__

