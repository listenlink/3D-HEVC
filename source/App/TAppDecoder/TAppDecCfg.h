

/** \file     TAppDecCfg.h
    \brief    Decoder configuration class (header)
*/

#ifndef __TAPPDECCFG__
#define __TAPPDECCFG__

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "../../Lib/TLibCommon/CommonDef.h"

// ====================================================================================================================
// Class definition
// ====================================================================================================================

/// Decoder configuration class
class TAppDecCfg
{
protected:
  char*         m_pchBitstreamFile;                   ///< input bitstream file name
  char*         m_pchReconFile;                       ///< output reconstruction file name
  char*         m_pchScaleOffsetFile;                 ///< output coded scale and offset parameters
#if DCM_SKIP_DECODING_FRAMES
  Int           m_iSkipFrame;                         ///< counter for frames prior to the random access point to skip
#endif
  UInt          m_outputBitDepth;                     ///< bit depth used for writing output

  bool m_pictureDigestEnabled; ///< enable(1)/disable(0) acting on SEI picture_digest message
  Void xAppendToFileNameEnd( Char* pchInputFileName, const Char* pchStringToAppend, Char*& rpchOutputFileName); ///< create filenames

public:
  TAppDecCfg()          {}
  virtual ~TAppDecCfg() {}
  
  Bool  parseCfg        ( Int argc, Char* argv[] );   ///< initialize option class from configuration
};

#endif


