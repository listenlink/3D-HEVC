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




// Include files
#include "CommonDef.h"
#include "TComYuv.h"
#include "TComWedgelet.h"

#include <stdlib.h>
#include <memory.h>

using namespace std;

#if NH_3D_DMM
TComWedgelet::TComWedgelet( UInt uiWidth, UInt uiHeight ) : m_uhXs     ( 0 ),
                                                            m_uhYs     ( 0 ),
                                                            m_uhXe     ( 0 ),
                                                            m_uhYe     ( 0 ),
                                                            m_uhOri    ( 0 ),
                                                            m_eWedgeRes( FULL_PEL ),
                                                            m_bIsCoarse( false )
{
  create( uiWidth, uiHeight );
}

TComWedgelet::TComWedgelet( const TComWedgelet &rcWedge ) : m_uhXs     ( rcWedge.m_uhXs      ),
                                                            m_uhYs     ( rcWedge.m_uhYs      ),
                                                            m_uhXe     ( rcWedge.m_uhXe      ),
                                                            m_uhYe     ( rcWedge.m_uhYe      ),
                                                            m_uhOri    ( rcWedge.m_uhOri     ),
                                                            m_eWedgeRes( rcWedge.m_eWedgeRes ),
                                                            m_bIsCoarse( rcWedge.m_bIsCoarse ),
                                                            m_uiWidth  ( rcWedge.m_uiWidth   ),
                                                            m_uiHeight ( rcWedge.m_uiHeight  ),
                                                            m_pbPattern( (Bool*)xMalloc( Bool, (m_uiWidth * m_uiHeight) ) ),
                                                            m_pbScaledPattern( g_wedgePattern )
{
  ::memcpy( m_pbPattern, rcWedge.m_pbPattern, sizeof(Bool) * (m_uiWidth * m_uiHeight));
}

TComWedgelet::~TComWedgelet(void)
{
  destroy();
}

Void TComWedgelet::create( UInt uiWidth, UInt uiHeight )
{
  assert( uiWidth > 0 && uiHeight > 0 );

  m_uiWidth   = uiWidth;
  m_uiHeight  = uiHeight;

  m_pbPattern = (Bool*)xMalloc( Bool, (m_uiWidth * m_uiHeight) );
  m_pbScaledPattern = g_wedgePattern;
}

Void TComWedgelet::destroy()
{
  if( m_pbPattern ) { xFree( m_pbPattern ); m_pbPattern = NULL; }
}

Void TComWedgelet::clear()
{
  ::memset( m_pbPattern, 0, (m_uiWidth * m_uiHeight) * sizeof(Bool) );
}

Void TComWedgelet::setWedgelet( UChar uhXs, UChar uhYs, UChar uhXe, UChar uhYe, UChar uhOri, WedgeResolution eWedgeRes, Bool bIsCoarse )
{
  m_uhXs      = uhXs;
  m_uhYs      = uhYs;
  m_uhXe      = uhXe;
  m_uhYe      = uhYe;
  m_uhOri     = uhOri;
  m_eWedgeRes = eWedgeRes;
  m_bIsCoarse = bIsCoarse;

  xGenerateWedgePattern();
}

Bool TComWedgelet::checkNotPlain()
{
  for( UInt k = 1; k < (m_uiWidth * m_uiHeight); k++ )
  {
    if( m_pbPattern[0] != m_pbPattern[k] )
    {
      return true;
    }
  }
  return false;
}

Bool TComWedgelet::checkIdentical( Bool* pbRefPattern )
{
  for( UInt k = 0; k < (m_uiWidth * m_uiHeight); k++ )
  {
    if( m_pbPattern[k] != pbRefPattern[k] )
    {
      return false;
    }
  }
  return true;
}

Bool TComWedgelet::checkInvIdentical( Bool* pbRefPattern )
{
  for( UInt k = 0; k < (m_uiWidth * m_uiHeight); k++ )
  {
    if( m_pbPattern[k] == pbRefPattern[k] )
    {
      return false;
    }
  }
  return true;
}

Void TComWedgelet::generateWedgePatternByRotate(const TComWedgelet &rcWedge, Int rotate)
{
  Int stride = m_uiWidth;
  Int sinc, offsetI, offsetJ;
  
  sinc = 1;
  offsetI = ( sinc) < 0 ? stride-1 : 0; // 0
  offsetJ = (-sinc) < 0 ? stride-1 : 0; // stride - 1

  for (Int y = 0; y < stride; y++)
  {
    for (Int x = 0; x < stride; x++)
    {
      Int i = offsetI + sinc * y; // y
      Int j = offsetJ - sinc * x; // stride - 1 - x
      m_pbPattern[(y * stride) + x] = !rcWedge.m_pbPattern[(j * stride) + i];
    }
  }
  Int blocksize = rcWedge.m_uiWidth * (rcWedge.m_eWedgeRes == HALF_PEL ? 2 : 1);
  Int offsetX = (-sinc) < 0 ? blocksize - 1 : 0;
  Int offsetY = ( sinc) < 0 ? blocksize - 1 : 0;
  m_uhXs = offsetX - sinc * rcWedge.m_uhYs;
  m_uhYs = offsetY + sinc * rcWedge.m_uhXs;
  m_uhXe = offsetX - sinc * rcWedge.m_uhYe;
  m_uhYe = offsetY + sinc * rcWedge.m_uhXe;
  m_uhOri = rotate;
  m_eWedgeRes = rcWedge.m_eWedgeRes;
  m_bIsCoarse = rcWedge.m_bIsCoarse;
  m_uiWidth  = rcWedge.m_uiWidth;
  m_uiHeight = rcWedge.m_uiHeight;
}

Void TComWedgelet::xGenerateWedgePattern()
{
  UInt uiTempBlockSize = 0;
  UChar uhXs = 0, uhYs = 0, uhXe = 0, uhYe = 0;
  switch( m_eWedgeRes )
  {
  case(   FULL_PEL ): { uiTempBlockSize =  m_uiWidth;     uhXs =  m_uhXs;     uhYs =  m_uhYs;     uhXe =  m_uhXe;     uhYe =  m_uhYe;     } break;
  case(   HALF_PEL ): { uiTempBlockSize = (m_uiWidth<<1); uhXs =  m_uhXs;     uhYs =  m_uhYs;     uhXe =  m_uhXe;     uhYe =  m_uhYe;     } break;
  }

  Bool* pbTempPattern = new Bool[ (uiTempBlockSize * uiTempBlockSize) ];
  ::memset( pbTempPattern, 0, (uiTempBlockSize * uiTempBlockSize) * sizeof(Bool) );
  Int iTempStride = uiTempBlockSize;

  xDrawEdgeLine( uhXs, uhYs, uhXe, uhYe, pbTempPattern, iTempStride );

  Int shift = (m_eWedgeRes == HALF_PEL) ? 1 : 0;
  Int endPos = uhYe>>shift;
  for (Int y = 0; y <= endPos; y++)
  {
    for (Int x = 0; x < m_uiWidth && pbTempPattern[(y * m_uiWidth) + x] == 0; x++)
    {
      pbTempPattern[(y * m_uiWidth) + x] = true;
    }
  }
  for( UInt k = 0; k < (m_uiWidth * m_uiHeight); k++ )
  {
    m_pbPattern[k] = pbTempPattern[k];
  };

  if( pbTempPattern )
  {
    delete [] pbTempPattern;
    pbTempPattern = NULL;
  }
}

Void TComWedgelet::xDrawEdgeLine( UChar uhXs, UChar uhYs, UChar uhXe, UChar uhYe, Bool* pbPattern, Int iPatternStride )
{
  Int x0 = (Int)uhXs;
  Int y0 = (Int)uhYs;
  Int x1 = (Int)uhXe;
  Int y1 = (Int)uhYe;

  // direction independent Bresenham line
  bool steep = (abs(y1 - y0) > abs(x1 - x0));
  if( steep )
  {
    std::swap( x0, y0 );
    std::swap( x1, y1 );
  }

  bool backward = ( x0 > x1 );
  if( backward )
  {
    std::swap( x0, x1 );
    std::swap( y0, y1 );
  }

  Int deltax = x1 - x0;
  Int deltay = abs(y1 - y0);
  Int error = 0;
  Int deltaerr = (deltay<<1);

  Int ystep;
  Int y = y0;
  if( y0 < y1 ) ystep =  1;
  else          ystep = -1;

  for( Int x = x0; x <= x1; x++ )
  {
    Int shift = (m_eWedgeRes == HALF_PEL) ? 1 : 0;
    Int stride = iPatternStride >> shift;
    if( steep ) { pbPattern[((x>>shift) * stride) + (y>>shift)] = true; }
    else        { pbPattern[((y>>shift) * stride) + (x>>shift)] = true; }

    error += deltaerr;
    if( error >= deltax )
    {
      y += ystep;
      error = error - (deltax<<1);
    }
  }
}

Bool* TComWedgelet::getPatternScaled( UInt dstSize )
{
  Bool *pbSrcPat = this->getPattern();
  UInt uiSrcSize = this->getStride();

  if( 16 >= dstSize )
  {
    assert( dstSize == uiSrcSize );
    return pbSrcPat;
  }
  else
  {
    Int scale = (g_aucConvertToBit[dstSize] - g_aucConvertToBit[uiSrcSize]);
    assert(scale>=0);
    for (Int y=0; y<dstSize; y++)
    {
      for (Int x=0; x<dstSize; x++)
      {
        Int srcX = x>>scale;
        Int srcY = y>>scale;
        m_pbScaledPattern[y*dstSize + x] = pbSrcPat[ srcY*uiSrcSize + srcX ];
      }
    }
    return m_pbScaledPattern;
  }
}

Void TComWedgelet::getPatternScaledCopy( UInt dstSize, Bool* dstBuf )
{
  Bool *pbSrcPat = this->getPattern();
  UInt uiSrcSize = this->getStride();

  if( 16 >= dstSize )
  {
    assert( dstSize == uiSrcSize );
    memcpy( dstBuf, pbSrcPat, (dstSize*dstSize) );
  }
  else
  {
    Int scale = (g_aucConvertToBit[dstSize] - g_aucConvertToBit[uiSrcSize]);
    assert(scale>=0);
    for (Int y=0; y<dstSize; y++)
    {
      for (Int x=0; x<dstSize; x++)
      {
        Int srcX = x>>scale;
        Int srcY = y>>scale;
        dstBuf[y*dstSize + x] = pbSrcPat[ srcY*uiSrcSize + srcX ];
      }
    }
  }
}


TComWedgeNode::TComWedgeNode()
{
  m_uiPatternIdx = DMM_NO_WEDGE_IDX;
  for( UInt uiPos = 0; uiPos < DMM_NUM_WEDGE_REFINES; uiPos++ )
  {
    m_uiRefineIdx[uiPos] = DMM_NO_WEDGE_IDX;
  }
}

UInt TComWedgeNode::getPatternIdx()
{
  return m_uiPatternIdx;
}
UInt TComWedgeNode::getRefineIdx( UInt uiPos )
{
  assert( uiPos < DMM_NUM_WEDGE_REFINES );
  return m_uiRefineIdx[uiPos];
}
Void TComWedgeNode::setPatternIdx( UInt uiIdx )
{
  m_uiPatternIdx = uiIdx;
}
Void TComWedgeNode::setRefineIdx( UInt uiIdx, UInt uiPos )
{
  assert( uiPos < DMM_NUM_WEDGE_REFINES );
  m_uiRefineIdx[uiPos] = uiIdx;  
}
#endif //NH_3D_DMM
