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

#include <list>
#include <stdio.h>
#include <fcntl.h>
#include <assert.h>

#ifndef __TAppRendererTOP__
#define __TAppRendererTOP__
#include "../../Lib/TLibCommon/TypeDef.h"
#if H_3D
#include "../../Lib/TLibRenderer/TRenTop.h"
#include "../../Lib/TLibVideoIO/TVideoIOYuv.h"
#include "TAppRendererCfg.h"
#include "TAppRendererTop.h"
#include "../../Lib/TLibRenderer/TRenModel.h"

// ====================================================================================================================
// Class definition
// ====================================================================================================================

/// encoder application class
class TAppRendererTop : public TAppRendererCfg
{
private:
  // class interface
  std::vector<TVideoIOYuv*>     m_apcTVideoIOYuvVideoInput;
  std::vector<TVideoIOYuv*>    m_apcTVideoIOYuvDepthInput;
  std::vector<TVideoIOYuv*>     m_apcTVideoIOYuvSynthOutput;

  // RendererInterface
  TRenTop*                     m_pcRenTop;

protected:
  // initialization
  Void  xCreateLib        ();                               ///< create renderer class and video io
  Void  xInitLib          ();                               ///< initialize renderer class
  Void  xDestroyLib       ();                               ///< destroy renderer class and video io
#if H_3D_VSO
  Void  xRenderModelFromString();                           ///< render using model using setup string
  Void  xRenderModelFromNums();                             ///< render using model using synth view numbers
#endif

public:
  TAppRendererTop();
  virtual ~TAppRendererTop();

  Void  render      ();                               ///< main encoding function
#if H_3D_VSO
  Void  renderModel ();
#endif
  Void  go          ();
  Void  renderUsedPelsMap();

};// END CLASS DEFINITION TAppRendererTop


#endif // H_3D
#endif // __TAppRendererTOP__
