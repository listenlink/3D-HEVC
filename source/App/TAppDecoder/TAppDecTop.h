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
#include "TLibCommon/TComDepthMapGenerator.h"
#include "TLibDecoder/TDecTop.h"
#include "TAppDecCfg.h"
#if VSP_N
#include "../../Lib/TLibRenderer/TRenTop.h"
#endif

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
  std::vector<TDecTop*>           m_tDecTop;                      ///< decoder classes

  std::vector<TVideoIOYuv*>       m_tVideoIOYuvReconFile;         ///< reconstruction YUV class

  // for output control
  Bool                            m_abDecFlag[ MAX_GOP ];         ///< decoded flag in one GOP
  std::vector<Int>                m_pocLastDisplay;               ///< last POC in display order
  Bool                            m_useDepth;

  FILE*                           m_pScaleOffsetFile;
  CamParsCollector                m_cCamParsCollector;
#if DEPTH_MAP_GENERATION
#if VIDYO_VPS_INTEGRATION
  TComVPSAccess                   m_cVPSAccess;
#endif
  TComSPSAccess                   m_cSPSAccess;
  TComAUPicAccess                 m_cAUPicAccess;
#endif

#if VSP_N
  TRenTop                         m_cVSPRendererTop;
#endif

public:
  TAppDecTop();
  virtual ~TAppDecTop() {}
  
  Void  create            (); ///< create internal members
  Void  destroy           (); ///< destroy internal members
  Void  decode            (); ///< main decoding function
#if VIDYO_VPS_INTEGRATION
  Void  increaseNumberOfViews  (UInt layerId, UInt viewId, UInt isDepth);
#else
  Void  increaseNumberOfViews  (Int newNumberOfViewDepth);
#endif
  TDecTop* getTDecTop     ( Int viewId, Bool isDepth );

  std::vector<TComPic*> getInterViewRefPics( Int viewId, Int poc, Bool isDepth, TComSPS* sps );
  TComPic*              getPicFromView     ( Int viewId, Int poc, bool isDepth ) { return xGetPicFromView( viewId, poc, isDepth ); }

#if DEPTH_MAP_GENERATION
#if VIDYO_VPS_INTEGRATION
  TComVPSAccess*    getVPSAccess  () { return &m_cVPSAccess;   }
#endif
  TComSPSAccess*    getSPSAccess  () { return &m_cSPSAccess;   }
  TComAUPicAccess*  getAUPicAccess() { return &m_cAUPicAccess; }
  TDecTop*          getDecTop0    () { return m_tDecTop[0]; }
#endif

#if VSP_N
  Bool              getUseDepth   () { return m_useDepth; }
  TRenTop*          getVSPRendererTop(){ return &m_cVSPRendererTop; }
  Void              storeVSPInBuffer(TComPic* pcPicVSP, TComPic* pcPicAvail, Int iCodedViewIdx, Int iCoddedViewOrderIdx, Int iCurPoc, Bool bDepth);
#endif

protected:
//  Void  xCreateDecLib     (); ///< create internal classes
  Void  xDestroyDecLib    (); ///< destroy internal classes
//  Void  xInitDecLib       (); ///< initialize decoder class
  
#if H0567_DPB_PARAMETERS_PER_TEMPORAL_LAYER
  Void  xWriteOutput      ( TComList<TComPic*>* pcListPic, Int viewDepthId, UInt tId); ///< write YUV to file
#else
  Void  xWriteOutput      ( TComList<TComPic*>* pcListPic, Int viewDepthId ); ///< write YUV to file
#endif
  Void  xFlushOutput      ( TComList<TComPic*>* pcListPic, Int viewDepthId ); ///< flush all remaining decoded pictures to file

  TComPic* xGetPicFromView( Int viewId, Int poc, Bool isDepth );
};

//! \}
#endif

