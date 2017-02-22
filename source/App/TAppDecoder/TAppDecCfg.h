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

/** \file     TAppDecCfg.h
    \brief    Decoder configuration class (header)
*/

#ifndef __TAPPDECCFG__
#define __TAPPDECCFG__

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "TLibCommon/CommonDef.h"
#include <vector>
#if NH_MV
#include <fstream>
#endif

//! \ingroup TAppDecoder
//! \{

// ====================================================================================================================
// Class definition
// ====================================================================================================================

/// Decoder configuration class
class TAppDecCfg
{
protected:
  std::string   m_bitstreamFileName;                    ///< input bitstream file name
  std::string   m_reconFileName;                        ///< output reconstruction file name
  Int           m_iSkipFrame;                           ///< counter for frames prior to the random access point to skip
  Int           m_outputBitDepth[MAX_NUM_CHANNEL_TYPE]; ///< bit depth used for writing output
  InputColourSpaceConversion m_outputColourSpaceConvert;

  Int           m_iMaxTemporalLayer;                  ///< maximum temporal layer to be decoded
  Int           m_decodedPictureHashSEIEnabled;       ///< Checksum(3)/CRC(2)/MD5(1)/disable(0) acting on decoded picture hash SEI message
  Bool          m_decodedNoDisplaySEIEnabled;         ///< Enable(true)/disable(false) writing only pictures that get displayed based on the no display SEI message
  std::string   m_colourRemapSEIFileName;             ///< output Colour Remapping file name
  std::vector<Int> m_targetDecLayerIdSet;             ///< set of LayerIds to be included in the sub-bitstream extraction process.

  Int           m_respectDefDispWindow;               ///< Only output content inside the default display window
#if O0043_BEST_EFFORT_DECODING
  UInt          m_forceDecodeBitDepth;                ///< if non-zero, force the bit depth at the decoder (best effort decoding)
#endif
  std::string   m_outputDecodedSEIMessagesFilename;   ///< filename to output decoded SEI messages to. If '-', then use stdout. If empty, do not output details.
  Bool          m_bClipOutputVideoToRec709Range;      ///< If true, clip the output video to the Rec 709 range on saving.
#if NH_MV
  std::vector<TChar*> m_pchReconFiles;                ///< array of output reconstruction file name create from output reconstruction file name

  std::vector<Int> m_targetOptLayerSetInd;            ///< target output layer set indices (multiple decoding when more than one element, e.g. for additional layer sets)
  Int           m_targetOptLayerSetIdx;               ///< current target output layer set index

  Int           m_targetDecLayerSetIdx;
  Int           m_baseLayerOutputFlag;
  Int           m_baseLayerPicOutputFlag;
  Int           m_auOutputFlag;
  Int           m_maxLayerId;                         ///< maximum nuh_layer_id decoded
  std::ifstream m_bitstreamFile;
  Int           m_highestTid;
  Bool          m_targetDecLayerIdSetFileEmpty;       ///< indication if target layers are given by file

  Bool          m_printVpsInfo;                       ///< Output VPS information
  Bool          m_printPicOutput;                     ///< Print information on picture output
  Bool          m_printReceivedNalus;                 ///< Print information on received NAL units
#if NH_3D
  TChar*        m_pchScaleOffsetFile;                   ///< output coded scale and offset parameters
  Bool          m_depth420OutputFlag;                   ///< output depth layers in 4:2:0
#endif

  Void xAppendToFileNameEnd( const TChar* pchInputFileName, const TChar* pchStringToAppend, TChar*& rpchOutputFileName); ///< create filenames
#endif

public:
  TAppDecCfg()
  : m_bitstreamFileName()
  , m_reconFileName()
  , m_iSkipFrame(0)
  // m_outputBitDepth array initialised below
  , m_outputColourSpaceConvert(IPCOLOURSPACE_UNCHANGED)
  , m_iMaxTemporalLayer(-1)
  , m_decodedPictureHashSEIEnabled(0)
  , m_decodedNoDisplaySEIEnabled(false)
  , m_colourRemapSEIFileName()
  , m_targetDecLayerIdSet()
  , m_respectDefDispWindow(0)
#if O0043_BEST_EFFORT_DECODING
  , m_forceDecodeBitDepth(0)
#endif
  , m_outputDecodedSEIMessagesFilename()
  , m_bClipOutputVideoToRec709Range(false)
#if NH_MV
  , m_highestTid(-1)
  , m_targetDecLayerIdSetFileEmpty(true)
#endif

  {
    for (UInt channelTypeIndex = 0; channelTypeIndex < MAX_NUM_CHANNEL_TYPE; channelTypeIndex++)
    {
      m_outputBitDepth[channelTypeIndex] = 0;
    }
#if NH_3D
    m_pchScaleOffsetFile = NULL;
#endif
  }

  virtual ~TAppDecCfg() {}

  Bool  parseCfg        ( Int argc, TChar* argv[] );   ///< initialize option class from configuration
#if NH_MV
  Int   getNumDecodings ( ) { return (Int) m_targetOptLayerSetInd.size();  }; 
#endif
};

//! \}

#endif


