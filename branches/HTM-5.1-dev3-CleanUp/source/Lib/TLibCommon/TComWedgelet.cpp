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




// Include files
#include "CommonDef.h"
#include "TComYuv.h"
#include "TComWedgelet.h"

#include <stdlib.h>
#include <memory.h>

using namespace std;


TComWedgelet::TComWedgelet( UInt uiWidth, UInt uiHeight ) : m_uhXs     ( 0 ),
                                                            m_uhYs     ( 0 ),
                                                            m_uhXe     ( 0 ),
                                                            m_uhYe     ( 0 ),
                                                            m_uhOri    ( 0 ),
                                                            m_eWedgeRes( FULL_PEL )
#if HHIQC_DMMFASTSEARCH_B0039
                                                            , m_bIsCoarse( false )
#endif
{
  create( uiWidth, uiHeight );
}

TComWedgelet::TComWedgelet( const TComWedgelet &rcWedge ) : m_uhXs     ( rcWedge.m_uhXs      ),
                                                            m_uhYs     ( rcWedge.m_uhYs      ),
                                                            m_uhXe     ( rcWedge.m_uhXe      ),
                                                            m_uhYe     ( rcWedge.m_uhYe      ),
                                                            m_uhOri    ( rcWedge.m_uhOri     ),
                                                            m_eWedgeRes( rcWedge.m_eWedgeRes ),
#if HHIQC_DMMFASTSEARCH_B0039
                                                            m_bIsCoarse( rcWedge.m_bIsCoarse ),
                                                            m_uiAng    ( rcWedge.m_uiAng     ),
#endif
                                                            m_uiWidth  ( rcWedge.m_uiWidth   ),
                                                            m_uiHeight ( rcWedge.m_uiHeight  ),
                                                            m_pbPattern( (Bool*)xMalloc( Bool, (m_uiWidth * m_uiHeight) ) )
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
}

Void TComWedgelet::destroy()
{
  if( m_pbPattern ) { xFree( m_pbPattern ); m_pbPattern = NULL; }
}

Void TComWedgelet::clear()
{
  ::memset( m_pbPattern, 0, (m_uiWidth * m_uiHeight) * sizeof(Bool) );
}

#if HHIQC_DMMFASTSEARCH_B0039
Void TComWedgelet::findClosetAngle()
{
  UInt uiAng=0,uiOptAng=0;
  UInt uiMinD=MAX_UINT;
  UInt uiTmpD=0;
  Int angTable[9]    = {0,    2,    5,   9,  13,  17,  21,  26,  32};
  
  UChar uhXs = m_uhXs;
  UChar uhYs = m_uhYs;
  UChar uhXe = m_uhXe;
  UChar uhYe = m_uhYe;

  for(uiAng=2; uiAng<=34; uiAng++)
  {
    Int iSign    = (uiAng<VER_IDX && uiAng>HOR_IDX ) ? -1 : 1;
    Int iVer     = uiAng>17 ? 32 : angTable[(uiAng>10) ? (uiAng-10) : (10-uiAng)];
    Int iHor     = uiAng<19 ? 32 : angTable[(uiAng>26) ? (uiAng-26) : (26-uiAng)];

    uiTmpD  = abs(iVer*iSign*(uhXs-uhXe) - iHor*(uhYe-uhYs));
    
    if( uiTmpD < uiMinD )
    {
      uiMinD = uiTmpD;
      uiOptAng = uiAng;
    }
  }
  m_uiAng = uiOptAng;
}

Void TComWedgelet::setWedgelet( UChar uhXs, UChar uhYs, UChar uhXe, UChar uhYe, UChar uhOri, WedgeResolution eWedgeRes, Bool bIsCoarse )
#else
Void TComWedgelet::setWedgelet( UChar uhXs, UChar uhYs, UChar uhXe, UChar uhYe, UChar uhOri, WedgeResolution eWedgeRes )
#endif
{
  m_uhXs      = uhXs;
  m_uhYs      = uhYs;
  m_uhXe      = uhXe;
  m_uhYe      = uhYe;
  m_uhOri     = uhOri;
  m_eWedgeRes = eWedgeRes;
#if HHIQC_DMMFASTSEARCH_B0039
  m_bIsCoarse = bIsCoarse;
#endif

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

#if HHI_DMM_WEDGE_INTRA
Bool TComWedgelet::checkPredDirAbovePossible( UInt uiPredDirBlockSize, UInt uiPredDirBlockOffset )
{
  WedgeResolution eContDWedgeRes = g_aeWedgeResolutionList[(UInt)g_aucConvertToBit[uiPredDirBlockSize]];
  UInt uiContDStartEndMax = 0;
  UInt uiContDStartEndOffset = 0;
  switch( eContDWedgeRes )
  {
  case( DOUBLE_PEL ): { uiContDStartEndMax = (uiPredDirBlockSize>>1); uiContDStartEndOffset = (uiPredDirBlockOffset>>1); break; }
  case(   FULL_PEL ): { uiContDStartEndMax =  uiPredDirBlockSize;     uiContDStartEndOffset =  uiPredDirBlockOffset;     break; }
  case(   HALF_PEL ): { uiContDStartEndMax = (uiPredDirBlockSize<<1); uiContDStartEndOffset = (uiPredDirBlockOffset<<1); break; }
  }

  if( m_uhOri == 2 || m_uhOri == 3 || m_uhOri == 4 )
  {
    UInt uiThisStartEndMax = 0;
    switch( m_eWedgeRes )
    {
    case( DOUBLE_PEL ): { uiThisStartEndMax = (m_uiWidth>>1); break; }
    case(   FULL_PEL ): { uiThisStartEndMax =  m_uiWidth;     break; }
    case(   HALF_PEL ): { uiThisStartEndMax = (m_uiWidth<<1); break; }
    }

    UChar uhStartX = m_uhXs;
    UChar uhStartY = m_uhYs;
    UChar uhEndX   = m_uhXe;
    UChar uhEndY   = m_uhYe;

    if( 2 == m_uhOri )
    {
      std::swap( uhStartX, uhEndX );
      std::swap( uhStartY, uhEndY );
    }

    UInt uiScaledEndX = (UInt)uhEndX;
    Int iDeltaRes = (Int)eContDWedgeRes - (Int)m_eWedgeRes;
    if( iDeltaRes > 0 ) { uiScaledEndX <<=  iDeltaRes; }
    if( iDeltaRes < 0 ) { uiScaledEndX >>= -iDeltaRes; }

    if( ((UInt)uhEndY == (uiThisStartEndMax-1)) && ((uiScaledEndX-uiContDStartEndOffset) > 0 && (uiScaledEndX-uiContDStartEndOffset) < (uiContDStartEndMax-1)) )
    {
      return true;
    }
  }

  return false;
}

Bool TComWedgelet::checkPredDirLeftPossible( UInt uiPredDirBlockSize, UInt uiPredDirBlockOffset )
{
  WedgeResolution eContDWedgeRes = g_aeWedgeResolutionList[(UInt)g_aucConvertToBit[uiPredDirBlockSize]];
  UInt uiContDStartEndMax = 0;
  UInt uiContDStartEndOffset = 0;
  switch( eContDWedgeRes )
  {
  case( DOUBLE_PEL ): { uiContDStartEndMax = (uiPredDirBlockSize>>1); uiContDStartEndOffset = (uiPredDirBlockOffset>>1); break; }
  case(   FULL_PEL ): { uiContDStartEndMax =  uiPredDirBlockSize;     uiContDStartEndOffset =  uiPredDirBlockOffset;     break; }
  case(   HALF_PEL ): { uiContDStartEndMax = (uiPredDirBlockSize<<1); uiContDStartEndOffset = (uiPredDirBlockOffset<<1); break; }
  }

  if( m_uhOri == 1 || m_uhOri == 2 || m_uhOri == 5 )
  {
    UInt uiThisStartEndMax = 0;
    switch( m_eWedgeRes )
    {
    case( DOUBLE_PEL ): { uiThisStartEndMax = (m_uiHeight>>1); break; }
    case(   FULL_PEL ): { uiThisStartEndMax =  m_uiHeight;     break; }
    case(   HALF_PEL ): { uiThisStartEndMax = (m_uiHeight<<1); break; }
    }

    UChar uhStartX = m_uhXs;
    UChar uhStartY = m_uhYs;
    UChar uhEndX   = m_uhXe;
    UChar uhEndY   = m_uhYe;

    if( 1 == m_uhOri || 5 == m_uhOri )
    {
      std::swap( uhStartX, uhEndX );
      std::swap( uhStartY, uhEndY );
    }

    UInt uiScaledEndY = (UInt)uhEndY;
    Int iDeltaRes = (Int)eContDWedgeRes - (Int)m_eWedgeRes;
    if( iDeltaRes > 0 ) { uiScaledEndY <<=  iDeltaRes; }
    if( iDeltaRes < 0 ) { uiScaledEndY >>= -iDeltaRes; }

    if( ((UInt)uhEndX == (uiThisStartEndMax-1)) && ((uiScaledEndY-uiContDStartEndOffset) > 0 && (uiScaledEndY-uiContDStartEndOffset) < (uiContDStartEndMax-1)) )
    {
      return true;
    }
  }

  return false;
}

Void TComWedgelet::getPredDirStartEndAbove( UChar& ruhXs, UChar& ruhYs, UChar& ruhXe, UChar& ruhYe, UInt uiPredDirBlockSize, UInt uiPredDirBlockOffset, Int iDeltaEnd )
{
  ruhXs = 0;
  ruhYs = 0;
  ruhXe = 0;
  ruhYe = 0;

  // get start/end of reference (=this) wedgelet
  UInt uiRefStartX = (UInt)getStartX();
  UInt uiRefStartY = (UInt)getStartY();
  UInt uiRefEndX   = (UInt)getEndX();
  UInt uiRefEndY   = (UInt)getEndY();

  WedgeResolution eContDWedgeRes = g_aeWedgeResolutionList[(UInt)g_aucConvertToBit[uiPredDirBlockSize]];
  UInt uiContDStartEndMax = 0;
  UInt uiContDStartEndOffset = 0;
  switch( eContDWedgeRes )
  {
  case( DOUBLE_PEL ): { uiContDStartEndMax = (uiPredDirBlockSize>>1); uiContDStartEndOffset = (uiPredDirBlockOffset>>1); break; }
  case(   FULL_PEL ): { uiContDStartEndMax =  uiPredDirBlockSize;     uiContDStartEndOffset =  uiPredDirBlockOffset;     break; }
  case(   HALF_PEL ): { uiContDStartEndMax = (uiPredDirBlockSize<<1); uiContDStartEndOffset = (uiPredDirBlockOffset<<1); break; }
  }
  Int iContDMaxPos = (Int)uiContDStartEndMax - 1;

  // swap if start/end if line orientation is not from top to bottom
  if( 2 == (UInt)getOri() )
  {
    std::swap( uiRefStartX, uiRefEndX );
    std::swap( uiRefStartY, uiRefEndY );
  }

  // calc slopes
  Int iA_DeltaX = (Int)uiRefEndX - (Int)uiRefStartX;
  Int iA_DeltaY = (Int)uiRefEndY - (Int)uiRefStartY;

  // get aligned end x value of ref wedge
  UInt uiScaledRefEndX = uiRefEndX;
  Int iDeltaRes = (Int)eContDWedgeRes - (Int)m_eWedgeRes;
  if( iDeltaRes > 0 ) { uiScaledRefEndX <<=  iDeltaRes; }
  if( iDeltaRes < 0 ) { uiScaledRefEndX >>= -iDeltaRes; }

  assert( uiScaledRefEndX >= uiContDStartEndOffset );
  Int iAlignedRefEndX = (Int)uiScaledRefEndX - (Int)uiContDStartEndOffset;

  // special for straight vertical wedge
  if( iA_DeltaX == 0 )
  {
    ruhXs = (UChar)iAlignedRefEndX;
    ruhYs = 0;

    Int iXe = iAlignedRefEndX + iDeltaEnd;
    if( iXe < 0 )
    {
      ruhXe = 0;
      ruhYe = (UChar)min( max( (iContDMaxPos + iXe), 0 ), iContDMaxPos );

      return;
    }
    else if( iXe > iContDMaxPos )
    {
      ruhXe = (UChar)iContDMaxPos;
      ruhYe = (UChar)min( max( (iContDMaxPos - (iXe - iContDMaxPos)), 0 ), iContDMaxPos );

      std::swap( ruhXs, ruhXe );
      std::swap( ruhYs, ruhYe );
      return;
    }
    else
    {
      ruhXe = (UChar)iXe;
      ruhYe = (UChar)iContDMaxPos;

      return;
    }
  }

  // special for straight horizontal short bottom line
  if( iA_DeltaY == 0 )
  {
    switch( (UInt)getOri() )
    {
    case( 2 ):
      {
        ruhXs = (UChar)(iAlignedRefEndX-1);
        ruhYs = 0;
        ruhXe = 0;
        ruhYe = (UChar)min( max( iDeltaEnd, 0 ), iContDMaxPos );

        return;
      }
    case( 3 ):
      {
        ruhXs = (UChar)(iAlignedRefEndX+1);
        ruhYs = 0;
        ruhXe = (UChar)iContDMaxPos;
        ruhYe = (UChar)min( max( -iDeltaEnd, 0 ), iContDMaxPos );

        std::swap( ruhXs, ruhXe );
        std::swap( ruhYs, ruhYe );
        return;
      }
    default:
      {
        assert( 0 );
        return;
      }
    }
  }

  // set start point depending on slope
  if( abs( iA_DeltaX ) >= abs( iA_DeltaY ) ) { if( iA_DeltaX < 0 ) { ruhXs = (UChar)(iAlignedRefEndX-1); ruhYs = 0; }
                                                if( iA_DeltaX > 0 ) { ruhXs = (UChar)(iAlignedRefEndX+1); ruhYs = 0; } }
  else                                                             { ruhXs = (UChar)(iAlignedRefEndX);   ruhYs = 0;   }

  // calc end point and determine orientation
  Int iVirtualEndX = (Int)ruhXs + roftoi( (Double)iContDMaxPos * ((Double)iA_DeltaX / (Double)iA_DeltaY) );

  if( iVirtualEndX < 0 )
  {
    Int iYe = roftoi( (Double)(0 - (Int)ruhXs) * ((Double)iA_DeltaY / (Double)iA_DeltaX) ) + iDeltaEnd;
    if( iYe < (Int)uiContDStartEndMax )
    {
      ruhXe = 0;
      ruhYe = (UChar)max( iYe, 0 );

      return;
    }
    else
    {
      ruhXe = (UChar)min( (iYe - iContDMaxPos), iContDMaxPos );
      ruhYe = (UChar)iContDMaxPos;

      return;
    }
  }
  else if( iVirtualEndX > iContDMaxPos )
  {
    Int iYe = roftoi( (Double)(iContDMaxPos - (Int)ruhXs) * ((Double)iA_DeltaY / (Double)iA_DeltaX) ) - iDeltaEnd;
    if( iYe < (Int)uiContDStartEndMax )
    {
      ruhXe = (UChar)iContDMaxPos;
      ruhYe = (UChar)max( iYe, 0 );

      std::swap( ruhXs, ruhXe );
      std::swap( ruhYs, ruhYe );
      return;
    }
    else
    {
      ruhXe = (UChar)max( (iContDMaxPos - (iYe - iContDMaxPos)), 0 );
      ruhYe = (UChar)iContDMaxPos;

      return;
    }
  }
  else
  {
    Int iXe = iVirtualEndX + iDeltaEnd;
    if( iXe < 0 )
    {
      ruhXe = 0;
      ruhYe = (UChar)max( (iContDMaxPos + iXe), 0 );

      return;
    }
    else if( iXe > iContDMaxPos )
    {
      ruhXe = (UChar)iContDMaxPos;
      ruhYe = (UChar)max( (iContDMaxPos - (iXe - iContDMaxPos)), 0 );

      std::swap( ruhXs, ruhXe );
      std::swap( ruhYs, ruhYe );
      return;
    }
    else
    {
      ruhXe = (UChar)iXe;
      ruhYe = (UChar)iContDMaxPos;

      return;
    }
  }
}

Void TComWedgelet::getPredDirStartEndLeft( UChar& ruhXs, UChar& ruhYs, UChar& ruhXe, UChar& ruhYe, UInt uiPredDirBlockSize, UInt uiPredDirBlockOffset, Int iDeltaEnd )
{
  ruhXs = 0;
  ruhYs = 0;
  ruhXe = 0;
  ruhYe = 0;

  // get start/end of reference (=this) wedgelet
  UInt uiRefStartX = (UInt)getStartX();
  UInt uiRefStartY = (UInt)getStartY();
  UInt uiRefEndX   = (UInt)getEndX();
  UInt uiRefEndY   = (UInt)getEndY();

  WedgeResolution eContDWedgeRes = g_aeWedgeResolutionList[(UInt)g_aucConvertToBit[uiPredDirBlockSize]];
  UInt uiContDStartEndMax = 0;
  UInt uiContDStartEndOffset = 0;
  switch( eContDWedgeRes )
  {
  case( DOUBLE_PEL ): { uiContDStartEndMax = (uiPredDirBlockSize>>1); uiContDStartEndOffset = (uiPredDirBlockOffset>>1); break; }
  case(   FULL_PEL ): { uiContDStartEndMax =  uiPredDirBlockSize;     uiContDStartEndOffset =  uiPredDirBlockOffset;     break; }
  case(   HALF_PEL ): { uiContDStartEndMax = (uiPredDirBlockSize<<1); uiContDStartEndOffset = (uiPredDirBlockOffset<<1); break; }
  }
  Int iContDMaxPos = (Int)uiContDStartEndMax - 1;

  // swap if start/end if line orientation is not from left to right
  if( 1 == (UInt)getOri() || 5 == (UInt)getOri() )
  {
    std::swap( uiRefStartX, uiRefEndX );
    std::swap( uiRefStartY, uiRefEndY );
  }

  Int iL_DeltaX = (Int)uiRefEndX - (Int)uiRefStartX;
  Int iL_DeltaY = (Int)uiRefEndY - (Int)uiRefStartY;

  UInt uiScaledRefEndY = uiRefEndY;
  Int iDeltaRes = (Int)eContDWedgeRes - (Int)m_eWedgeRes;
  if( iDeltaRes > 0 ) { uiScaledRefEndY <<=  iDeltaRes; }
  if( iDeltaRes < 0 ) { uiScaledRefEndY >>= -iDeltaRes; }

  assert( uiScaledRefEndY >= uiContDStartEndOffset );
  Int iAlignedRefEndY = (Int)uiScaledRefEndY - (Int)uiContDStartEndOffset;

  // special for straight horizontal wedge
  if( iL_DeltaY == 0 )
  {
    ruhXs = 0;
    ruhYs = (UChar)iAlignedRefEndY;

    Int iYe = iAlignedRefEndY - iDeltaEnd;
    if( iYe < 0 )
    {
      ruhXe = (UChar)min( max( (iContDMaxPos + iYe), 0 ), iContDMaxPos );
      ruhYe = 0;

      std::swap( ruhXs, ruhXe );
      std::swap( ruhYs, ruhYe );
      return;
    }
    else if( iYe > iContDMaxPos )
    {
      ruhXe = (UChar)min( max( (iContDMaxPos - (iYe - iContDMaxPos)), 0 ), iContDMaxPos );
      ruhYe = (UChar)iContDMaxPos;

      return;
    }
    else
    {
      ruhXe = (UChar)iContDMaxPos;
      ruhYe = (UChar)iYe;

      std::swap( ruhXs, ruhXe );
      std::swap( ruhYs, ruhYe );
      return;
    }
  }

  // special for straight vertical short right line
  if( iL_DeltaX == 0 )
  {
    switch( (UInt)getOri() )
    {
    case( 1 ):
      {
        ruhXs = 0;
        ruhYs = (UChar)(iAlignedRefEndY+1);
        ruhXe = (UChar)min( max( iDeltaEnd, 0 ), iContDMaxPos );
        ruhYe = (UChar)iContDMaxPos;

        return;
      }
    case( 2 ):
      {
        ruhXs = 0;
        ruhYs = (UChar)(iAlignedRefEndY-1);
        ruhXe = (UChar)min( max( -iDeltaEnd, 0 ), iContDMaxPos );
        ruhYe = 0;

        std::swap( ruhXs, ruhXe );
        std::swap( ruhYs, ruhYe );
        return;
      }
    default:
      {
        assert( 0 );
        return;
      }
    }
  }

  // set start point depending on slope
  if( abs( iL_DeltaY ) >= abs( iL_DeltaX ) ) { if( iL_DeltaY < 0 ) { ruhYs = (UChar)(iAlignedRefEndY-1); ruhXs = 0; }
                                               if( iL_DeltaY > 0 ) { ruhYs = (UChar)(iAlignedRefEndY+1); ruhXs = 0; } }
  else                                       {                       ruhYs = (UChar)(iAlignedRefEndY);   ruhXs = 0;   }

  // calc end point and determine orientation
  Int iVirtualEndY = (Int)ruhYs + roftoi( (Double)iContDMaxPos * ((Double)iL_DeltaY / (Double)iL_DeltaX) );

  if( iVirtualEndY < 0 )
  {
    Int iXe = roftoi( (Double)(0 - (Int)ruhYs ) * ((Double)iL_DeltaX / (Double)iL_DeltaY) ) - iDeltaEnd;
    if( iXe < (Int)uiContDStartEndMax )
    {
      ruhXe = (UChar)max( iXe, 0 );
      ruhYe = 0;

      std::swap( ruhXs, ruhXe );
      std::swap( ruhYs, ruhYe );
      return;
    }
    else
    {
      ruhXe = (UChar)iContDMaxPos;
      ruhYe = (UChar)min( (iXe - iContDMaxPos), iContDMaxPos );

      std::swap( ruhXs, ruhXe );
      std::swap( ruhYs, ruhYe );
      return;
    }
  }
  else if( iVirtualEndY > iContDMaxPos )
  {
    Int iXe = roftoi( (Double)(iContDMaxPos - (Int)ruhYs ) * ((Double)iL_DeltaX / (Double)iL_DeltaY) ) + iDeltaEnd;
    if( iXe < (Int)uiContDStartEndMax )
    {
      ruhXe = (UChar)max( iXe, 0 );
      ruhYe = (UChar)iContDMaxPos;

      return;
    }
    else
    {
      ruhXe = (UChar)iContDMaxPos;
      ruhYe = (UChar)max( (iContDMaxPos - (iXe - iContDMaxPos)), 0 );

      std::swap( ruhXs, ruhXe );
      std::swap( ruhYs, ruhYe );
      return;
    }
  }
  else
  {
    Int iYe = iVirtualEndY - iDeltaEnd;
    if( iYe < 0 )
    {
      ruhXe = (UChar)max( (iContDMaxPos + iYe), 0 );
      ruhYe = 0;

      std::swap( ruhXs, ruhXe );
      std::swap( ruhYs, ruhYe );
      return;
    }
    else if( iYe > iContDMaxPos )
    {
      ruhXe = (UChar)max( (iContDMaxPos - (iYe - iContDMaxPos)), 0 );
      ruhYe = (UChar)iContDMaxPos;

      return;
    }
    else
    {
      ruhXe = (UChar)iContDMaxPos;
      ruhYe = (UChar)iYe;

      std::swap( ruhXs, ruhXe );
      std::swap( ruhYs, ruhYe );
      return;
    }
  }
}
#endif

Void TComWedgelet::xGenerateWedgePattern()
{
  UInt uiTempBlockSize = 0;
  UChar uhXs = 0, uhYs = 0, uhXe = 0, uhYe = 0;
  switch( m_eWedgeRes )
  {
  case( DOUBLE_PEL ): { uiTempBlockSize =  m_uiWidth;     uhXs = (m_uhXs<<1); uhYs = (m_uhYs<<1); uhXe = (m_uhXe<<1); uhYe = (m_uhYe<<1); } break;
  case(   FULL_PEL ): { uiTempBlockSize =  m_uiWidth;     uhXs =  m_uhXs;     uhYs =  m_uhYs;     uhXe =  m_uhXe;     uhYe =  m_uhYe;     } break;
  case(   HALF_PEL ): { uiTempBlockSize = (m_uiWidth<<1); uhXs =  m_uhXs;     uhYs =  m_uhYs;     uhXe =  m_uhXe;     uhYe =  m_uhYe;     } break;
  }

  if( m_eWedgeRes == DOUBLE_PEL) // fix for line-end problem with DOUBLE_PEL resolution
  {
    if( m_uhOri == 1 ) { uhXs = uiTempBlockSize-1; }
    if( m_uhOri == 2 ) { uhXe = uiTempBlockSize-1; uhYs = uiTempBlockSize-1; }
    if( m_uhOri == 3 ) { uhYe = uiTempBlockSize-1; }
    if( m_uhOri == 4 ) { uhYe = uiTempBlockSize-1; }
    if( m_uhOri == 5 ) { uhXs = uiTempBlockSize-1; }
  }

  Bool* pbTempPattern = new Bool[ (uiTempBlockSize * uiTempBlockSize) ];
  ::memset( pbTempPattern, 0, (uiTempBlockSize * uiTempBlockSize) * sizeof(Bool) );
  Int iTempStride = uiTempBlockSize;

  xDrawEdgeLine( uhXs, uhYs, uhXe, uhYe, pbTempPattern, iTempStride );

  switch( m_uhOri )
  {
  case( 0 ): { for( UInt iX = 0;                 iX < uhXs;            iX++ ) { UInt iY = 0;                 while( pbTempPattern[(iY * iTempStride) + iX] == false ) { pbTempPattern[(iY * iTempStride) + iX] = true; iY++; } } } break;
  case( 1 ): { for( UInt iY = 0;                 iY < uhYs;            iY++ ) { UInt iX = uiTempBlockSize-1; while( pbTempPattern[(iY * iTempStride) + iX] == false ) { pbTempPattern[(iY * iTempStride) + iX] = true; iX--; } } } break;
  case( 2 ): { for( UInt iX = uiTempBlockSize-1; iX > uhXs;            iX-- ) { UInt iY = uiTempBlockSize-1; while( pbTempPattern[(iY * iTempStride) + iX] == false ) { pbTempPattern[(iY * iTempStride) + iX] = true; iY--; } } } break;
  case( 3 ): { for( UInt iY = uiTempBlockSize-1; iY > uhYs;            iY-- ) { UInt iX = 0;                 while( pbTempPattern[(iY * iTempStride) + iX] == false ) { pbTempPattern[(iY * iTempStride) + iX] = true; iX++; } } } break;
#if HHIQC_DMMFASTSEARCH_B0039
  case( 4 ): 
    { 
      if( (uhXs+uhXe) < uiTempBlockSize ) { for( UInt iY = 0; iY < uiTempBlockSize; iY++ ) { UInt iX = 0;                 while( pbTempPattern[(iY * iTempStride) + iX] == false ) { pbTempPattern[(iY * iTempStride) + iX] = true; iX++; } } }
      else                                { for( UInt iY = 0; iY < uiTempBlockSize; iY++ ) { UInt iX = uiTempBlockSize-1; while( pbTempPattern[(iY * iTempStride) + iX] == false ) { pbTempPattern[(iY * iTempStride) + iX] = true; iX--; } } }
    }
    break;
  case( 5 ): 
    { 
      if( (uhYs+uhYe) < uiTempBlockSize ) { for( UInt iX = 0; iX < uiTempBlockSize; iX++ ) { UInt iY = 0;                 while( pbTempPattern[(iY * iTempStride) + iX] == false ) { pbTempPattern[(iY * iTempStride) + iX] = true; iY++; } } }
      else                                { for( UInt iX = 0; iX < uiTempBlockSize; iX++ ) { UInt iY = uiTempBlockSize-1; while( pbTempPattern[(iY * iTempStride) + iX] == false ) { pbTempPattern[(iY * iTempStride) + iX] = true; iY--; } } }
    }
#else
  case( 4 ): { for( UInt iY = 0;               iY < uiTempBlockSize; iY++ ) { UInt iX = 0;                 while( pbTempPattern[(iY * iTempStride) + iX] == false ) { pbTempPattern[(iY * iTempStride) + iX] = true; iX++; } } } break;
  case( 5 ): { for( UInt iX = 0;               iX < uiTempBlockSize; iX++ ) { UInt iY = 0;                 while( pbTempPattern[(iY * iTempStride) + iX] == false ) { pbTempPattern[(iY * iTempStride) + iX] = true; iY++; } } } break;
#endif
  }

  clear();
  switch( m_eWedgeRes )
  {
  case( DOUBLE_PEL ): { for( UInt k = 0; k < (m_uiWidth * m_uiHeight); k++ ) { m_pbPattern[k] = pbTempPattern[k]; }; } break;
  case(   FULL_PEL ): { for( UInt k = 0; k < (m_uiWidth * m_uiHeight); k++ ) { m_pbPattern[k] = pbTempPattern[k]; }; } break;
  case(   HALF_PEL ): // sub-sampling by factor 2
    {
      Int iStride = getStride();

      UInt uiOffX, uiOffY;
      switch( m_uhOri )
      {
      case( 0 ): { uiOffX = 0; uiOffY = 0; } break;
      case( 1 ): { uiOffX = 1; uiOffY = 0; } break;
      case( 2 ): { uiOffX = 1; uiOffY = 1; } break;
      case( 3 ): { uiOffX = 0; uiOffY = 1; } break;
#if HHIQC_DMMFASTSEARCH_B0039
      case( 4 ): 
        { 
          if( (uhXs+uhXe) < uiTempBlockSize ) { uiOffX = 0; uiOffY = 0; }
          else                                { uiOffX = 1; uiOffY = 0; }
        } 
        break;
      case( 5 ): 
        { 
          if( (uhYs+uhYe) < uiTempBlockSize ) { uiOffX = 0; uiOffY = 0; }
          else                                { uiOffX = 0; uiOffY = 1; }
        } 
        break;
#else
      case( 4 ): { uiOffX = 0; uiOffY = 0; } break;
      case( 5 ): { uiOffX = 0; uiOffY = 0; } break;
#endif
      default:   { uiOffX = 0; uiOffY = 0; } break;
      }

      for(Int iY = 0; iY < m_uiHeight; iY++)
      {
        for(Int iX = 0; iX < m_uiWidth; iX++)
        {
          m_pbPattern[(iY * iStride) + iX] = pbTempPattern[(((iY<<1)+uiOffY) * iTempStride) + ((iX<<1)+uiOffX)];
        }
      }
    }
    break;
  }

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
  double error = 0.0;
  double deltaerr = (double)deltay / (double)deltax;

  Int ystep;
  Int y = y0;
  if( y0 < y1 ) ystep =  1;
  else          ystep = -1;

  for( Int x = x0; x <= x1; x++ )
  {
    if( steep ) { pbPattern[(x * iPatternStride) + y] = true; }
    else        { pbPattern[(y * iPatternStride) + x] = true; }

    error += deltaerr;
    if( error >= 0.5)
    {
      y += ystep;
      error = error - 1.0;
    }
  }
}

#if HHIQC_DMMFASTSEARCH_B0039
TComWedgeNode::TComWedgeNode()
{
  m_uiPatternIdx = NO_IDX;
  for( UInt uiPos = 0; uiPos < NUM_WEDGE_REFINES; uiPos++ )
  {
    m_uiRefineIdx[uiPos] = NO_IDX;
  }
}

UInt TComWedgeNode::getPatternIdx()
{
  return m_uiPatternIdx;
}
UInt TComWedgeNode::getRefineIdx( UInt uiPos )
{
  assert( uiPos < NUM_WEDGE_REFINES );
  return m_uiRefineIdx[uiPos];
}
Void TComWedgeNode::setPatternIdx( UInt uiIdx )
{
  m_uiPatternIdx = uiIdx;
}
Void TComWedgeNode::setRefineIdx( UInt uiIdx, UInt uiPos )
{
  assert( uiPos < NUM_WEDGE_REFINES );
  m_uiRefineIdx[uiPos] = uiIdx;  
}
#endif

#if HHI_DMM_PRED_TEX
TComWedgeDist::TComWedgeDist()
{
  init();
}

TComWedgeDist::~TComWedgeDist()
{
}

Void TComWedgeDist::init()
{
  m_afpDistortFunc[0] = TComWedgeDist::xGetSAD4;
  m_afpDistortFunc[1] = TComWedgeDist::xGetSAD8;
  m_afpDistortFunc[2] = TComWedgeDist::xGetSAD16;
  m_afpDistortFunc[3] = TComWedgeDist::xGetSAD32;

  m_afpDistortFunc[4] = TComWedgeDist::xGetSSE4;
  m_afpDistortFunc[5] = TComWedgeDist::xGetSSE8;
  m_afpDistortFunc[6] = TComWedgeDist::xGetSSE16;
  m_afpDistortFunc[7] = TComWedgeDist::xGetSSE32;
}

UInt TComWedgeDist::xGetSAD4( WedgeDistParam* pcDtParam )
{
  Pel* piOrg   = pcDtParam->pOrg;
  Pel* piCur   = pcDtParam->pCur;
  Int  iRows   = pcDtParam->iRows;
  Int  iSubShift  = pcDtParam->iSubShift;
  Int  iSubStep   = ( 1 << iSubShift );
  Int  iStrideCur = pcDtParam->iStrideCur*iSubStep;
  Int  iStrideOrg = pcDtParam->iStrideOrg*iSubStep;

  UInt uiSum = 0;

  for( ; iRows != 0; iRows-=iSubStep )
  {
    uiSum += abs( piOrg[0] - piCur[0] );
    uiSum += abs( piOrg[1] - piCur[1] );
    uiSum += abs( piOrg[2] - piCur[2] );
    uiSum += abs( piOrg[3] - piCur[3] );

    piOrg += iStrideOrg;
    piCur += iStrideCur;
  }

  uiSum <<= iSubShift;
  return ( uiSum >> g_uiBitIncrement );
}

UInt TComWedgeDist::xGetSAD8( WedgeDistParam* pcDtParam )
{
  Pel* piOrg      = pcDtParam->pOrg;
  Pel* piCur      = pcDtParam->pCur;
  Int  iRows      = pcDtParam->iRows;
  Int  iSubShift  = pcDtParam->iSubShift;
  Int  iSubStep   = ( 1 << iSubShift );
  Int  iStrideCur = pcDtParam->iStrideCur*iSubStep;
  Int  iStrideOrg = pcDtParam->iStrideOrg*iSubStep;

  UInt uiSum = 0;

  for( ; iRows != 0; iRows-=iSubStep )
  {
    uiSum += abs( piOrg[0] - piCur[0] );
    uiSum += abs( piOrg[1] - piCur[1] );
    uiSum += abs( piOrg[2] - piCur[2] );
    uiSum += abs( piOrg[3] - piCur[3] );
    uiSum += abs( piOrg[4] - piCur[4] );
    uiSum += abs( piOrg[5] - piCur[5] );
    uiSum += abs( piOrg[6] - piCur[6] );
    uiSum += abs( piOrg[7] - piCur[7] );

    piOrg += iStrideOrg;
    piCur += iStrideCur;
  }

  uiSum <<= iSubShift;
  return ( uiSum >> g_uiBitIncrement );
}

UInt TComWedgeDist::xGetSAD16( WedgeDistParam* pcDtParam )
{
  Pel* piOrg   = pcDtParam->pOrg;
  Pel* piCur   = pcDtParam->pCur;
  Int  iRows   = pcDtParam->iRows;
  Int  iSubShift  = pcDtParam->iSubShift;
  Int  iSubStep   = ( 1 << iSubShift );
  Int  iStrideCur = pcDtParam->iStrideCur*iSubStep;
  Int  iStrideOrg = pcDtParam->iStrideOrg*iSubStep;

  UInt uiSum = 0;

  for( ; iRows != 0; iRows-=iSubStep )
  {
    uiSum += abs( piOrg[0] - piCur[0] );
    uiSum += abs( piOrg[1] - piCur[1] );
    uiSum += abs( piOrg[2] - piCur[2] );
    uiSum += abs( piOrg[3] - piCur[3] );
    uiSum += abs( piOrg[4] - piCur[4] );
    uiSum += abs( piOrg[5] - piCur[5] );
    uiSum += abs( piOrg[6] - piCur[6] );
    uiSum += abs( piOrg[7] - piCur[7] );
    uiSum += abs( piOrg[8] - piCur[8] );
    uiSum += abs( piOrg[9] - piCur[9] );
    uiSum += abs( piOrg[10] - piCur[10] );
    uiSum += abs( piOrg[11] - piCur[11] );
    uiSum += abs( piOrg[12] - piCur[12] );
    uiSum += abs( piOrg[13] - piCur[13] );
    uiSum += abs( piOrg[14] - piCur[14] );
    uiSum += abs( piOrg[15] - piCur[15] );

    piOrg += iStrideOrg;
    piCur += iStrideCur;
  }

  uiSum <<= iSubShift;
  return ( uiSum >> g_uiBitIncrement );
}

UInt TComWedgeDist::xGetSAD32( WedgeDistParam* pcDtParam )
{
  Pel* piOrg   = pcDtParam->pOrg;
  Pel* piCur   = pcDtParam->pCur;
  Int  iRows   = pcDtParam->iRows;
  Int  iSubShift  = pcDtParam->iSubShift;
  Int  iSubStep   = ( 1 << iSubShift );
  Int  iStrideCur = pcDtParam->iStrideCur*iSubStep;
  Int  iStrideOrg = pcDtParam->iStrideOrg*iSubStep;

  UInt uiSum = 0;

  for( ; iRows != 0; iRows-=iSubStep )
  {
    uiSum += abs( piOrg[0] - piCur[0] );
    uiSum += abs( piOrg[1] - piCur[1] );
    uiSum += abs( piOrg[2] - piCur[2] );
    uiSum += abs( piOrg[3] - piCur[3] );
    uiSum += abs( piOrg[4] - piCur[4] );
    uiSum += abs( piOrg[5] - piCur[5] );
    uiSum += abs( piOrg[6] - piCur[6] );
    uiSum += abs( piOrg[7] - piCur[7] );
    uiSum += abs( piOrg[8] - piCur[8] );
    uiSum += abs( piOrg[9] - piCur[9] );
    uiSum += abs( piOrg[10] - piCur[10] );
    uiSum += abs( piOrg[11] - piCur[11] );
    uiSum += abs( piOrg[12] - piCur[12] );
    uiSum += abs( piOrg[13] - piCur[13] );
    uiSum += abs( piOrg[14] - piCur[14] );
    uiSum += abs( piOrg[15] - piCur[15] );
    uiSum += abs( piOrg[16] - piCur[16] );
    uiSum += abs( piOrg[17] - piCur[17] );
    uiSum += abs( piOrg[18] - piCur[18] );
    uiSum += abs( piOrg[19] - piCur[19] );
    uiSum += abs( piOrg[20] - piCur[20] );
    uiSum += abs( piOrg[21] - piCur[21] );
    uiSum += abs( piOrg[22] - piCur[22] );
    uiSum += abs( piOrg[23] - piCur[23] );
    uiSum += abs( piOrg[24] - piCur[24] );
    uiSum += abs( piOrg[25] - piCur[25] );
    uiSum += abs( piOrg[26] - piCur[26] );
    uiSum += abs( piOrg[27] - piCur[27] );
    uiSum += abs( piOrg[28] - piCur[28] );
    uiSum += abs( piOrg[29] - piCur[29] );
    uiSum += abs( piOrg[30] - piCur[30] );
    uiSum += abs( piOrg[31] - piCur[31] );

    piOrg += iStrideOrg;
    piCur += iStrideCur;
  }

  uiSum <<= iSubShift;
  return ( uiSum >> g_uiBitIncrement );
}

UInt TComWedgeDist::xGetSSE4( WedgeDistParam* pcDtParam )
{
  Pel* piOrg   = pcDtParam->pOrg;
  Pel* piCur   = pcDtParam->pCur;
  Int  iRows   = pcDtParam->iRows;
  Int  iStrideOrg = pcDtParam->iStrideOrg;
  Int  iStrideCur = pcDtParam->iStrideCur;

  UInt uiSum = 0;
  UInt uiShift = g_uiBitIncrement<<1;

  Int  iTemp;

  for( ; iRows != 0; iRows-- )
  {

    iTemp = piOrg[0] - piCur[0]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[1] - piCur[1]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[2] - piCur[2]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[3] - piCur[3]; uiSum += ( iTemp * iTemp ) >> uiShift;

    piOrg += iStrideOrg;
    piCur += iStrideCur;
  }

  return ( uiSum );
}

UInt TComWedgeDist::xGetSSE8( WedgeDistParam* pcDtParam )
{
  Pel* piOrg   = pcDtParam->pOrg;
  Pel* piCur   = pcDtParam->pCur;
  Int  iRows   = pcDtParam->iRows;
  Int  iStrideOrg = pcDtParam->iStrideOrg;
  Int  iStrideCur = pcDtParam->iStrideCur;

  UInt uiSum = 0;
  UInt uiShift = g_uiBitIncrement<<1;

  Int  iTemp;

  for( ; iRows != 0; iRows-- )
  {
    iTemp = piOrg[0] - piCur[0]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[1] - piCur[1]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[2] - piCur[2]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[3] - piCur[3]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[4] - piCur[4]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[5] - piCur[5]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[6] - piCur[6]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[7] - piCur[7]; uiSum += ( iTemp * iTemp ) >> uiShift;

    piOrg += iStrideOrg;
    piCur += iStrideCur;
  }

  return ( uiSum );
}

UInt TComWedgeDist::xGetSSE16( WedgeDistParam* pcDtParam )
{
  Pel* piOrg   = pcDtParam->pOrg;
  Pel* piCur   = pcDtParam->pCur;
  Int  iRows   = pcDtParam->iRows;
  Int  iStrideOrg = pcDtParam->iStrideOrg;
  Int  iStrideCur = pcDtParam->iStrideCur;

  UInt uiSum = 0;
  UInt uiShift = g_uiBitIncrement<<1;

  Int  iTemp;

  for( ; iRows != 0; iRows-- )
  {

    iTemp = piOrg[ 0] - piCur[ 0]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[ 1] - piCur[ 1]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[ 2] - piCur[ 2]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[ 3] - piCur[ 3]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[ 4] - piCur[ 4]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[ 5] - piCur[ 5]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[ 6] - piCur[ 6]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[ 7] - piCur[ 7]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[ 8] - piCur[ 8]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[ 9] - piCur[ 9]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[10] - piCur[10]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[11] - piCur[11]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[12] - piCur[12]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[13] - piCur[13]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[14] - piCur[14]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[15] - piCur[15]; uiSum += ( iTemp * iTemp ) >> uiShift;

    piOrg += iStrideOrg;
    piCur += iStrideCur;
  }

  return ( uiSum );
}

UInt TComWedgeDist::xGetSSE32( WedgeDistParam* pcDtParam )
{
  Pel* piOrg   = pcDtParam->pOrg;
  Pel* piCur   = pcDtParam->pCur;
  Int  iRows   = pcDtParam->iRows;
  Int  iStrideOrg = pcDtParam->iStrideOrg;
  Int  iStrideCur = pcDtParam->iStrideCur;

  UInt uiSum = 0;
  UInt uiShift = g_uiBitIncrement<<1;
  Int  iTemp;

  for( ; iRows != 0; iRows-- )
  {

    iTemp = piOrg[ 0] - piCur[ 0]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[ 1] - piCur[ 1]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[ 2] - piCur[ 2]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[ 3] - piCur[ 3]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[ 4] - piCur[ 4]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[ 5] - piCur[ 5]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[ 6] - piCur[ 6]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[ 7] - piCur[ 7]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[ 8] - piCur[ 8]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[ 9] - piCur[ 9]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[10] - piCur[10]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[11] - piCur[11]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[12] - piCur[12]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[13] - piCur[13]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[14] - piCur[14]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[15] - piCur[15]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[16] - piCur[16]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[17] - piCur[17]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[18] - piCur[18]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[19] - piCur[19]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[20] - piCur[20]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[21] - piCur[21]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[22] - piCur[22]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[23] - piCur[23]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[24] - piCur[24]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[25] - piCur[25]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[26] - piCur[26]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[27] - piCur[27]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[28] - piCur[28]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[29] - piCur[29]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[30] - piCur[30]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[31] - piCur[31]; uiSum += ( iTemp * iTemp ) >> uiShift;

    piOrg += iStrideOrg;
    piCur += iStrideCur;
  }

  return ( uiSum );
}

Void TComWedgeDist::setDistParam( UInt uiBlkWidth, UInt uiBlkHeight, WedgeDist eWDist, WedgeDistParam& rcDistParam )
{
  // set Block Width / Height
  rcDistParam.iCols    = uiBlkWidth;
  rcDistParam.iRows    = uiBlkHeight;
  rcDistParam.DistFunc = m_afpDistortFunc[eWDist + g_aucConvertToBit[ rcDistParam.iCols ] ];

  // initialize
  rcDistParam.iSubShift  = 0;
}

UInt TComWedgeDist::getDistPart( Pel* piCur, Int iCurStride,  Pel* piOrg, Int iOrgStride, UInt uiBlkWidth, UInt uiBlkHeight, WedgeDist eWDist )
{
  WedgeDistParam cDtParam;
  setDistParam( uiBlkWidth, uiBlkHeight, eWDist, cDtParam );
  cDtParam.pOrg       = piOrg;
  cDtParam.pCur       = piCur;
  cDtParam.iStrideOrg = iOrgStride;
  cDtParam.iStrideCur = iCurStride;
  cDtParam.iStep      = 1;

  return cDtParam.DistFunc( &cDtParam );
}
#endif
