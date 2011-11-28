


// Include files
#include "CommonDef.h"
#include "TComYuv.h"
#include "TComWedgelet.h"

#include <stdlib.h>
#include <memory.h>


TComWedgelet::TComWedgelet( UInt uiWidth, UInt uiHeight ) : m_uhXs     ( 0 ),
                                                            m_uhYs     ( 0 ),
                                                            m_uhXe     ( 0 ),
                                                            m_uhYe     ( 0 ),
                                                            m_uhOri    ( 0 ),
                                                            m_eWedgeRes( FULL_PEL )
{
  create( uiWidth, uiHeight );
}

TComWedgelet::TComWedgelet( const TComWedgelet &rcWedge ) : m_uhXs     ( rcWedge.m_uhXs      ),
                                                            m_uhYs     ( rcWedge.m_uhYs      ),
                                                            m_uhXe     ( rcWedge.m_uhXe      ),
                                                            m_uhYe     ( rcWedge.m_uhYe      ),
                                                            m_uhOri    ( rcWedge.m_uhOri     ),
                                                            m_eWedgeRes( rcWedge.m_eWedgeRes ),
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

Void TComWedgelet::setWedgelet( UChar uhXs, UChar uhYs, UChar uhXe, UChar uhYe, UChar uhOri, WedgeResolution eWedgeRes )
{
  m_uhXs      = uhXs;
  m_uhYs      = uhYs;
  m_uhXe      = uhXe;
  m_uhYe      = uhYe;
  m_uhOri     = uhOri;
  m_eWedgeRes = eWedgeRes;

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

Bool TComWedgelet::checkNotIdentical( Bool* pbRefPattern )
{
  for( UInt k = 0; k < (m_uiWidth * m_uiHeight); k++ )
  {
    if( m_pbPattern[k] != pbRefPattern[k] )
    {
      return true;
    }
  }
  return false;
}

Bool TComWedgelet::checkNotInvIdentical( Bool* pbRefPattern )
{
  for( UInt k = 0; k < (m_uiWidth * m_uiHeight); k++ )
  {
    if( m_pbPattern[k] == pbRefPattern[k] )
    {
      return true;
    }
  }
  return false;
}

#if HHI_DMM_INTRA
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
  UInt uiContDOri;

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
      uiContDOri = 0;
      ruhXe = 0;
      ruhYe = (UChar)Min( Max( ((uiContDStartEndMax-1) + iXe), 0 ), (uiContDStartEndMax-1) );

      return;
    }
    else if( iXe > (uiContDStartEndMax-1) )
    {
      uiContDOri = 1;
      ruhXe = (UChar)(uiContDStartEndMax-1);
      ruhYe = (UChar)Min( Max( ((uiContDStartEndMax-1) - (iXe - (uiContDStartEndMax-1))), 0 ), (uiContDStartEndMax-1) );

      std::swap( ruhXs, ruhXe );
      std::swap( ruhYs, ruhYe );
      return;
    }
    else
    {
      uiContDOri = 4;
      ruhXe = (UChar)iXe;
      ruhYe = (UChar)(uiContDStartEndMax-1);

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
        uiContDOri = 0;
        ruhXs = (UChar)(iAlignedRefEndX-1);
        ruhYs = 0;
        ruhXe = 0;
        ruhYe = (UChar)Min( Max( iDeltaEnd, 0 ), (uiContDStartEndMax-1) );

        return;
      }
    case( 3 ):
      {
        uiContDOri = 1;
        ruhXs = (UChar)(iAlignedRefEndX+1);
        ruhYs = 0;
        ruhXe = (UChar)(uiContDStartEndMax-1);
        ruhYe = (UChar)Min( Max( -iDeltaEnd, 0 ), (uiContDStartEndMax-1) );

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
  Int iVirtualEndX = (Int)ruhXs + roftoi( (Double)(uiContDStartEndMax-1) * ((Double)iA_DeltaX / (Double)iA_DeltaY) );

  if( iVirtualEndX < 0 )
  {
    Int iYe = roftoi( (Double)(0 - (Int)ruhXs) * ((Double)iA_DeltaY / (Double)iA_DeltaX) ) + iDeltaEnd;
    if( iYe < (Int)uiContDStartEndMax )
    {
      uiContDOri = 0;
      ruhXe = 0;
      ruhYe = (UChar)Max( iYe, 0 );

      return;
    }
    else
    {
      uiContDOri = 4;
      ruhXe = (UChar)Min( (iYe - (uiContDStartEndMax-1)), (uiContDStartEndMax-1) );
      ruhYe = (UChar)(uiContDStartEndMax-1);

      return;
    }
  }
  else if( iVirtualEndX > (uiContDStartEndMax-1) )
  {
    Int iYe = roftoi( (Double)((Int)(uiContDStartEndMax-1) - (Int)ruhXs) * ((Double)iA_DeltaY / (Double)iA_DeltaX) ) - iDeltaEnd;
    if( iYe < (Int)uiContDStartEndMax )
    {
      uiContDOri = 1;
      ruhXe = (UChar)(uiContDStartEndMax-1);
      ruhYe = (UChar)Max( iYe, 0 );

      std::swap( ruhXs, ruhXe );
      std::swap( ruhYs, ruhYe );
      return;
    }
    else
    {
      uiContDOri = 4;
      ruhXe = (UChar)Max( ((uiContDStartEndMax-1) - (iYe - (uiContDStartEndMax-1))), 0 );
      ruhYe = (UChar)(uiContDStartEndMax-1);

      return;
    }
  }
  else
  {
    Int iXe = iVirtualEndX + iDeltaEnd;
    if( iXe < 0 )
    {
      uiContDOri = 0;
      ruhXe = 0;
      ruhYe = (UChar)Max( ((uiContDStartEndMax-1) + iXe), 0 );

      return;
    }
    else if( iXe > (uiContDStartEndMax-1) )
    {
      uiContDOri = 1;
      ruhXe = (UChar)(uiContDStartEndMax-1);
      ruhYe = (UChar)Max( ((uiContDStartEndMax-1) - (iXe - (uiContDStartEndMax-1))), 0 );

      std::swap( ruhXs, ruhXe );
      std::swap( ruhYs, ruhYe );
      return;
    }
    else
    {
      uiContDOri = 4;
      ruhXe = (UChar)iXe;
      ruhYe = (UChar)(uiContDStartEndMax-1);

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
  UInt uiContDOri;

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
      uiContDOri = 0;
      ruhXe = (UChar)Min( Max( ((uiContDStartEndMax-1) + iYe), 0 ), (uiContDStartEndMax-1) );
      ruhYe = 0;

      std::swap( ruhXs, ruhXe );
      std::swap( ruhYs, ruhYe );
      return;
    }
    else if( iYe > (uiContDStartEndMax-1) )
    {
      uiContDOri = 3;
      ruhXe = (UChar)Min( Max( ((uiContDStartEndMax-1) - (iYe - (uiContDStartEndMax-1))), 0 ), (uiContDStartEndMax-1) );
      ruhYe = (UChar)(uiContDStartEndMax-1);

      return;
    }
    else
    {
      uiContDOri = 5;
      ruhXe = (UChar)(uiContDStartEndMax-1);
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
        uiContDOri = 3;
        ruhXs = 0;
        ruhYs = (UChar)(iAlignedRefEndY+1);
        ruhXe = (UChar)Min( Max( iDeltaEnd, 0 ), (uiContDStartEndMax-1) );
        ruhYe = (UChar)(uiContDStartEndMax-1);

        return;
      }
    case( 2 ):
      {
        uiContDOri = 0;
        ruhXs = 0;
        ruhYs = (UChar)(iAlignedRefEndY-1);
        ruhXe = (UChar)Min( Max( -iDeltaEnd, 0 ), (uiContDStartEndMax-1) );
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
  Int iVirtualEndY = (Int)ruhYs + roftoi( (Double)(uiContDStartEndMax-1) * ((Double)iL_DeltaY / (Double)iL_DeltaX) );

  if( iVirtualEndY < 0 )
  {
    Int iXe = roftoi( (Double)(0 - (Int)ruhYs ) * ((Double)iL_DeltaX / (Double)iL_DeltaY) ) - iDeltaEnd;
    if( iXe < (Int)uiContDStartEndMax )
    {
      uiContDOri = 0;
      ruhXe = (UChar)Max( iXe, 0 );
      ruhYe = 0;

      std::swap( ruhXs, ruhXe );
      std::swap( ruhYs, ruhYe );
      return;
    }
    else
    {
      uiContDOri = 5;
      ruhXe = (UChar)(uiContDStartEndMax-1);
      ruhYe = (UChar)Min( (iXe - (uiContDStartEndMax-1)), (uiContDStartEndMax-1) );

      std::swap( ruhXs, ruhXe );
      std::swap( ruhYs, ruhYe );
      return;
    }
  }
  else if( iVirtualEndY > (uiContDStartEndMax-1) )
  {
    Int iXe = roftoi( (Double)((Int)(uiContDStartEndMax-1) - (Int)ruhYs ) * ((Double)iL_DeltaX / (Double)iL_DeltaY) ) + iDeltaEnd;
    if( iXe < (Int)uiContDStartEndMax )
    {
      uiContDOri = 3;
      ruhXe = (UChar)Max( iXe, 0 );
      ruhYe = (UChar)(uiContDStartEndMax-1);

      return;
    }
    else
    {
      uiContDOri = 5;
      ruhXe = (UChar)(uiContDStartEndMax-1);
      ruhYe = (UChar)Max( ((uiContDStartEndMax-1) - (iXe - (uiContDStartEndMax-1))), 0 );

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
      uiContDOri = 0;
      ruhXe = (UChar)Max( ((uiContDStartEndMax-1) + iYe), 0 );
      ruhYe = 0;

      std::swap( ruhXs, ruhXe );
      std::swap( ruhYs, ruhYe );
      return;
    }
    else if( iYe > (uiContDStartEndMax-1) )
    {
      uiContDOri = 3;
      ruhXe = (UChar)Max( ((uiContDStartEndMax-1) - (iYe - (uiContDStartEndMax-1))), 0 );
      ruhYe = (UChar)(uiContDStartEndMax-1);

      return;
    }
    else
    {
      uiContDOri = 5;
      ruhXe = (UChar)(uiContDStartEndMax-1);
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
  case( 4 ): { for( UInt iY = 0;                 iY < uiTempBlockSize; iY++ ) { UInt iX = 0;                 while( pbTempPattern[(iY * iTempStride) + iX] == false ) { pbTempPattern[(iY * iTempStride) + iX] = true; iX++; } } } break;
  case( 5 ): { for( UInt iX = 0;                 iX < uiTempBlockSize; iX++ ) { UInt iY = 0;                 while( pbTempPattern[(iY * iTempStride) + iX] == false ) { pbTempPattern[(iY * iTempStride) + iX] = true; iY++; } } } break;
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
      case( 4 ): { uiOffX = 0; uiOffY = 0; } break;
      case( 5 ): { uiOffX = 0; uiOffY = 0; } break;
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
