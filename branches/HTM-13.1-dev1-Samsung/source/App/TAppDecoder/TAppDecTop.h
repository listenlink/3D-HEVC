/* The copyright in this software is being made available under the BSD
 * License, included below. This software may be subject to other third party
 * and contributor rights, including patent rights, and no such rights are
 * granted under this license.  
 *
* Copyright (c) 2010-2014, ITU/ISO/IEC
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

/** \file     TAppDecTop.h
    \brief    Decoder application class (header)
*/

#ifndef __TAPPDECTOP__
#define __TAPPDECTOP__

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "TLibVideoIO/TVideoIOYuv.h"
#include "TLibCommon/TComList.h"
#include "TLibCommon/TComPicYuv.h"
#include "TLibDecoder/TDecTop.h"
#include "TAppDecCfg.h"

//! \ingroup TAppDecoder
//! \{

// ====================================================================================================================
// Class definition
// ====================================================================================================================

/// decoder application class
class TAppDecTop : public TAppDecCfg
{
private:
  // class interface
#if H_MV
  TDecTop*                        m_tDecTop             [ MAX_NUM_LAYERS ];    ///< decoder classes
  TVideoIOYuv*                    m_tVideoIOYuvReconFile[ MAX_NUM_LAYERS ];    ///< reconstruction YUV class
  Int                             m_layerIdToDecIdx     [ MAX_NUM_LAYER_IDS ]; ///< maping from layer id to decoder index
  Int                             m_numDecoders;                               ///< number of decoder instances
  TComPicLists                    m_ivPicLists;                                ///< picture buffers of decoder instances
  Bool                            m_layerInitilizedFlags[ MAX_NUM_LAYER_IDS ]; ///< for layerwise startup 
  TComVPS*                        m_vps;                                ///< active VPS
#else
  TDecTop                         m_cTDecTop;                     ///< decoder class
  TVideoIOYuv                     m_cTVideoIOYuvReconFile;        ///< reconstruction YUV class
#endif
    // for output control
#if H_MV
  Int                             m_pocLastDisplay      [ MAX_NUM_LAYERS ]; ///< last POC in display order
  Bool                            m_reconOpen           [ MAX_NUM_LAYERS ]; ///< reconstruction file opened
  Bool                            m_markedForOutput; 
#else
  Int                             m_iPOCLastDisplay;              ///< last POC in display order
#endif

#if H_3D
  FILE*                           m_pScaleOffsetFile;
  CamParsCollector                m_cCamParsCollector;
#endif
public:
  TAppDecTop();
  virtual ~TAppDecTop() {}
  
  Void  create            (); ///< create internal members
  Void  destroy           (); ///< destroy internal members
  Void  decode            (); ///< main decoding function

protected:
  Void  xCreateDecLib     (); ///< create internal classes
  Void  xDestroyDecLib    (); ///< destroy internal classes
  Void  xInitDecLib       (); ///< initialize decoder class

#if H_MV
  Void  xWriteOutput      ( TComList<TComPic*>* pcListPic, Int layerId, Int tId ); ///< write YUV to file

  Void  xMarkForOutput   ( Bool allLayersDecoded, Int pocLastPic, Int layerIdLastPic );         
  Void  xMarkAltOutPic    ( Int targetOutputLayer, Int pocLastPic );

  Void  xFlushOutput      ( TComList<TComPic*>* pcListPic, Int layerId ); ///< flush all remaining decoded pictures to file
  Int   xGetDecoderIdx    ( Int layerId, Bool createFlag = false );
#else
  Void  xWriteOutput      ( TComList<TComPic*>* pcListPic , UInt tId); ///< write YUV to file
  Void  xFlushOutput      ( TComList<TComPic*>* pcListPic ); ///< flush all remaining decoded pictures to file
#endif
  Bool  isNaluWithinTargetDecLayerIdSet ( InputNALUnit* nalu ); ///< check whether given Nalu is within targetDecLayerIdSet
};

//! \}

#endif

