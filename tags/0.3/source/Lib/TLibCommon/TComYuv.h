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



/** \file     TComYuv.h
    \brief    general YUV buffer class (header)
    \todo     this should be merged with TComPicYuv \n
              check usage of removeHighFreq function
*/

#ifndef __TCOMYUV__
#define __TCOMYUV__
#include <assert.h>
#include "CommonDef.h"
#include "TComPicYuv.h"

// ====================================================================================================================
// Class definition
// ====================================================================================================================

/// general YUV buffer class
class TComYuv
{
private:
  
  // ------------------------------------------------------------------------------------------------------------------
  //  YUV buffer
  // ------------------------------------------------------------------------------------------------------------------
  
  Pel*    m_apiBufY;
  Pel*    m_apiBufU;
  Pel*    m_apiBufV;
  
  // ------------------------------------------------------------------------------------------------------------------
  //  Parameter for general YUV buffer usage
  // ------------------------------------------------------------------------------------------------------------------
  
  UInt     m_iWidth;
  UInt     m_iHeight;
  UInt     m_iCWidth;
  UInt     m_iCHeight;
  
public:
  
  TComYuv();
  virtual ~TComYuv();
  
  // ------------------------------------------------------------------------------------------------------------------
  //  Memory management
  // ------------------------------------------------------------------------------------------------------------------
  
  Void    create            ( UInt iWidth, UInt iHeight );  ///< Create  YUV buffer
  Void    destroy           ();                             ///< Destroy YUV buffer
  Void    clear             ();                             ///< clear   YUV buffer
  
  // ------------------------------------------------------------------------------------------------------------------
  //  Copy, load, store YUV buffer
  // ------------------------------------------------------------------------------------------------------------------
  
  //  Copy YUV buffer to picture buffer
  Void    copyToPicYuv         ( TComPicYuv* pcPicYuvDst, UInt iCuAddr, UInt uiAbsZorderIdx, UInt uiPartDepth = 0, UInt uiPartIdx = 0 );
  Void    copyToPicLuma        ( TComPicYuv* pcPicYuvDst, UInt iCuAddr, UInt uiAbsZorderIdx, UInt uiPartDepth = 0, UInt uiPartIdx = 0 );
  Void    copyToPicChroma      ( TComPicYuv* pcPicYuvDst, UInt iCuAddr, UInt uiAbsZorderIdx, UInt uiPartDepth = 0, UInt uiPartIdx = 0 );
  
  //  Copy YUV buffer from picture buffer
  Void    copyFromPicYuv       ( TComPicYuv* pcPicYuvSrc, UInt iCuAddr, UInt uiAbsZorderIdx );
  Void    copyFromPicLuma      ( TComPicYuv* pcPicYuvSrc, UInt iCuAddr, UInt uiAbsZorderIdx );
  Void    copyFromPicChroma    ( TComPicYuv* pcPicYuvSrc, UInt iCuAddr, UInt uiAbsZorderIdx );
  
  //  Copy Small YUV buffer to the part of other Big YUV buffer
  Void    copyToPartYuv         ( TComYuv*    pcYuvDst,    UInt uiDstPartIdx );
  Void    copyToPartLuma        ( TComYuv*    pcYuvDst,    UInt uiDstPartIdx );
  Void    copyToPartChroma      ( TComYuv*    pcYuvDst,    UInt uiDstPartIdx );
  
  //  Copy the part of Big YUV buffer to other Small YUV buffer
  Void    copyPartToYuv         ( TComYuv*    pcYuvDst,    UInt uiSrcPartIdx );
  Void    copyPartToLuma        ( TComYuv*    pcYuvDst,    UInt uiSrcPartIdx );
  Void    copyPartToChroma      ( TComYuv*    pcYuvDst,    UInt uiSrcPartIdx );
  
  //  Copy YUV partition buffer to other YUV partition buffer
  Void    copyPartToPartYuv     ( TComYuv*    pcYuvDst, UInt uiPartIdx, UInt uiWidth, UInt uiHeight );
  Void    copyPartToPartLuma    ( TComYuv*    pcYuvDst, UInt uiPartIdx, UInt uiWidth, UInt uiHeight );
  Void    copyPartToPartChroma  ( TComYuv*    pcYuvDst, UInt uiPartIdx, UInt uiWidth, UInt uiHeight );
#if DEPTH_MAP_GENERATION
  Void    copyPartToPartYuvPdm  ( TComYuv*    pcYuvDst, UInt uiPartIdx, UInt uiWidth, UInt uiHeight, UInt uiSubSampExpX, UInt uiSubSampExpY );
  Void    copyPartToPartLumaPdm ( TComYuv*    pcYuvDst, UInt uiPartIdx, UInt uiWidth, UInt uiHeight, UInt uiSubSampExpX, UInt uiSubSampExpY );
#endif
  
  // ------------------------------------------------------------------------------------------------------------------
  //  Algebraic operation for YUV buffer
  // ------------------------------------------------------------------------------------------------------------------
  
  //  Clip(pcYuvSrc0 + pcYuvSrc1) -> m_apiBuf
  Void    addClip           ( TComYuv* pcYuvSrc0, TComYuv* pcYuvSrc1, UInt uiTrUnitIdx, UInt uiPartSize );
  Void    addClipLuma       ( TComYuv* pcYuvSrc0, TComYuv* pcYuvSrc1, UInt uiTrUnitIdx, UInt uiPartSize );
  Void    addClipChroma     ( TComYuv* pcYuvSrc0, TComYuv* pcYuvSrc1, UInt uiTrUnitIdx, UInt uiPartSize );
  Void    addClipPartLuma   ( TComYuv* pcYuvSrc0, TComYuv* pcYuvSrc1, UInt uiTrUnitIdx, UInt uiPartSize ); //GT

  
  //  pcYuvSrc0 - pcYuvSrc1 -> m_apiBuf
  Void    subtract          ( TComYuv* pcYuvSrc0, TComYuv* pcYuvSrc1, UInt uiTrUnitIdx, UInt uiPartSize );
  Void    subtractLuma      ( TComYuv* pcYuvSrc0, TComYuv* pcYuvSrc1, UInt uiTrUnitIdx, UInt uiPartSize );
  Void    subtractChroma    ( TComYuv* pcYuvSrc0, TComYuv* pcYuvSrc1, UInt uiTrUnitIdx, UInt uiPartSize );
  
  //  (pcYuvSrc0 + pcYuvSrc1)/2 for YUV partition
#ifdef ROUNDING_CONTROL_BIPRED
  Void    addAvg            ( TComYuv* pcYuvSrc0, TComYuv* pcYuvSrc1, UInt iPartUnitIdx, UInt iWidth, UInt iHeight, Bool bRound );
#endif
  Void    addAvg            ( TComYuv* pcYuvSrc0, TComYuv* pcYuvSrc1, UInt iPartUnitIdx, UInt iWidth, UInt iHeight );
#if DEPTH_MAP_GENERATION
  Void    addAvgPdm         ( TComYuv* pcYuvSrc0, TComYuv* pcYuvSrc1, UInt iPartUnitIdx, UInt iWidth, UInt iHeight, UInt uiSubSampExpX, UInt uiSubSampExpY );
#endif

  //   Remove High frequency
  Void    removeHighFreq    ( TComYuv* pcYuvSrc, UInt uiWidht, UInt uiHeight );
  Void    removeHighFreq    ( TComYuv* pcYuvSrc, UInt uiPartIdx, UInt uiWidht, UInt uiHeight );

  Void    add               ( TComYuv* pcYuvAdd, Int iWidth, Int iHeight, Bool bSubtract = false );
  Void    addLuma           ( TComYuv* pcYuvAdd, Int iWidth, Int iHeight, Bool bSubtract );
  Void    addChroma         ( TComYuv* pcYuvAdd, Int iWidth, Int iHeight, Bool bSubtract );

  Void    clip              ( Int iWidth, Int iHeight );
  Void    clipLuma          ( Int iWidth, Int iHeight );
  Void    clipChroma        ( Int iWidth, Int iHeight );
  
  // ------------------------------------------------------------------------------------------------------------------
  //  Access function for YUV buffer
  // ------------------------------------------------------------------------------------------------------------------
  
  //  Access starting position of YUV buffer
  Pel*    getLumaAddr ()    { return m_apiBufY; }
  Pel*    getCbAddr   ()    { return m_apiBufU; }
  Pel*    getCrAddr   ()    { return m_apiBufV; }
  
  //  Access starting position of YUV partition unit buffer
  Pel*    getLumaAddr       ( UInt iPartUnitIdx );
  Pel*    getCbAddr         ( UInt iPartUnitIdx );
  Pel*    getCrAddr         ( UInt iPartUnitIdx );
  
  //  Access starting position of YUV transform unit buffer
  Pel*    getLumaAddr       ( UInt iTransUnitIdx, UInt iBlkSize );
  Pel*    getCbAddr         ( UInt iTransUnitIdx, UInt iBlkSize );
  Pel*    getCrAddr         ( UInt iTransUnitIdx, UInt iBlkSize );
  
  //  Get stride value of YUV buffer
  UInt    getStride   ()    { return  m_iWidth;   }
  UInt    getCStride  ()    { return  m_iCWidth;  }
  UInt    getHeight   ()    { return  m_iHeight;  }
  
  UInt    getWidth    ()    { return  m_iWidth;   }
  UInt    getCHeight  ()    { return  m_iCHeight; }
  UInt    getCWidth   ()    { return  m_iCWidth;  }
  
  Void    printout();
  
  // ------------------------------------------------------------------------------------------------------------------
  //  Miscellaneous
  // ------------------------------------------------------------------------------------------------------------------
  
  __inline Pel  xClip  (Pel x )      { return ( (x < 0) ? 0 : (x > (Pel)g_uiIBDI_MAX) ? (Pel)g_uiIBDI_MAX : x ); }
  
};// END CLASS DEFINITION TComYuv

#endif // __TCOMYUV__

