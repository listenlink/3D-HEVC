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
#include "TAppComCamPara.h"

#include <math.h>
#include <errno.h>
#include <vector>
#include <iostream>
#include <fstream>
#include <algorithm>
#include <functional>




Void
TAppComCamPara::xCreateLUTs( UInt uiNumberSourceViews, UInt uiNumberTargetViews, Double****& radLUT, Int****& raiLUT, Double***& radShiftParams, Int64***& raiShiftParams )
{
  AOF( m_uiBitDepthForLUT == 8 );
  AOF( radShiftParams == NULL && raiShiftParams == NULL && radLUT == NULL && raiLUT == NULL );

  uiNumberSourceViews = Max( 1, uiNumberSourceViews );
  uiNumberTargetViews = Max( 1, uiNumberTargetViews );

  radShiftParams = new Double** [ uiNumberSourceViews ];
  raiShiftParams = new Int64 ** [ uiNumberSourceViews ];
  radLUT         = new Double***[ uiNumberSourceViews ];
  raiLUT         = new Int   ***[ uiNumberSourceViews ];

  for( UInt uiSourceView = 0; uiSourceView < uiNumberSourceViews; uiSourceView++ )
  {
    radShiftParams[ uiSourceView ] = new Double* [ uiNumberTargetViews ];
    raiShiftParams[ uiSourceView ] = new Int64 * [ uiNumberTargetViews ];
    radLUT        [ uiSourceView ] = new Double**[ uiNumberTargetViews ];
    raiLUT        [ uiSourceView ] = new Int   **[ uiNumberTargetViews ];

    for( UInt uiTargetView = 0; uiTargetView < uiNumberTargetViews; uiTargetView++ )
    {
      radShiftParams[ uiSourceView ][ uiTargetView ]      = new Double [ 2 ];
      raiShiftParams[ uiSourceView ][ uiTargetView ]      = new Int64  [ 2 ];

      radLUT        [ uiSourceView ][ uiTargetView ]      = new Double*[ 2 ];
      radLUT        [ uiSourceView ][ uiTargetView ][ 0 ] = new Double [ 257 ];
      radLUT        [ uiSourceView ][ uiTargetView ][ 1 ] = new Double [ 257 ];

      raiLUT        [ uiSourceView ][ uiTargetView ]      = new Int*   [ 2 ];
      raiLUT        [ uiSourceView ][ uiTargetView ][ 0 ] = new Int    [ 257 ];
      raiLUT        [ uiSourceView ][ uiTargetView ][ 1 ] = new Int    [ 257 ];
    }
  }
}


Void
TAppComCamPara::xCreate2dArray( UInt uiNum1Ids, UInt uiNum2Ids, Int**& raaiArray )
{
  AOT( raaiArray || uiNum1Ids == 0 || uiNum2Ids == 0 );
  raaiArray = new Int* [ uiNum1Ids ];
  for( UInt uiId1 = 0; uiId1 < uiNum1Ids; uiId1++ )
  {
    raaiArray[ uiId1 ] = new Int [ uiNum2Ids ];
  }
}


Void
TAppComCamPara::xInit2dArray( UInt uiNum1Ids, UInt uiNum2Ids, Int**& raaiArray, Int iValue )
{
  for( UInt uiId1 = 0; uiId1 < uiNum1Ids; uiId1++ )
  {
    for( UInt uiId2 = 0; uiId2 < uiNum2Ids; uiId2++ )
    {
      raaiArray[ uiId1 ][ uiId2 ] = iValue;
    }
  }
}


Void
TAppComCamPara::convertNumberString( Char* pchViewNumberString, std::vector<Int>& raiViewNumbers, Double dViewNumPrec )
{
  Bool bStringIsRange = false;
  Int  iIdx           = 0;
  std::vector<Double> adViewNumbers;

  while( pchViewNumberString != 0 && pchViewNumberString[ iIdx ] != 0 )
  {
    if( pchViewNumberString[ iIdx ] == ':' )
    {
      bStringIsRange              = true;
      pchViewNumberString[ iIdx ] = ' ';
    }
    iIdx++;
  }

  Char* pcNextStart = pchViewNumberString;
  Char* pcEnd       = pcNextStart + iIdx;
  Char* pcOldStart  = 0;

  while( pcNextStart < pcEnd )
  {
    errno = 0;
    adViewNumbers.push_back( ( strtod( pcNextStart, &pcNextStart ) ) );

    if( errno == ERANGE || pcNextStart == pcOldStart )
    {
      std::cerr << "Error Parsing View Number String: `" << pchViewNumberString << "'" << std::endl;
      AOT(true);
      exit( EXIT_FAILURE );
    };

    while( pcNextStart < pcEnd && ( *pcNextStart == ' ' || *pcNextStart == '\t' || *pcNextStart == '\r' ) ) pcNextStart++;

    pcOldStart = pcNextStart;
  }

  if( bStringIsRange )
  {
    if( adViewNumbers.size() != 3 )
    {
      std::cerr << "Error Parsing SynthViewNumbers: `" << pchViewNumberString << "'" << std::endl;
      AOT(true);
      exit( EXIT_FAILURE );
    }

    Double dRangeBegin = adViewNumbers[0];
    Double dRangeStep  = adViewNumbers[1];
    Double dRangeEnd   = adViewNumbers[2];

    if( ( ( dRangeEnd - dRangeBegin > 0 ) != ( dRangeStep > 0 ) ) || dRangeStep == 0 )
    {
      std::cerr << "Error Parsing SynthViewNumbers: `" << pchViewNumberString << "'" << std::endl;
      AOT(true);
      exit( EXIT_FAILURE );
    }

    raiViewNumbers.clear();

    Double dFac = ( dRangeBegin > dRangeEnd ? -1 : 1 );

    for( Double dViewNumber = dRangeBegin; ( dViewNumber - dRangeEnd ) * dFac <= 0; dViewNumber += dRangeStep )
    {
      raiViewNumbers.push_back( (Int)( dViewNumber * dViewNumPrec ) );
    }
  }
  else
  {
    for( UInt uiViewNum = 0; uiViewNum < adViewNumbers.size(); uiViewNum++ )
    {
      raiViewNumbers.push_back( (Int)( adViewNumbers[ uiViewNum ] * dViewNumPrec ) );
    }
  }
}


Void
TAppComCamPara::xReadCameraParameterFile( Char* pchCfgFileName )
{
  std::ifstream cCfgStream( pchCfgFileName, std::ifstream::in );
  if( !cCfgStream )
  {
    std::cerr << "Failed to open config file: `" << pchCfgFileName << "'" << std::endl;
    exit( EXIT_FAILURE );
  }

  Int iLineNumber = 0;
  do
  {
    std::string cLine;
    getline( cCfgStream, cLine );
    iLineNumber++;

    size_t iStart = cLine.find_first_not_of( " \t\n\r" );

    if( iStart == std::string::npos )
    {
      continue;
    }

    if( cLine[iStart] == '#' )
    {
      continue;
    }

    Char* pcNextStart = (Char*) cLine.data();
    Char* pcEnd = pcNextStart + cLine.length();

    std::vector<Double> caNewLine;
    caNewLine.clear();

    Char* pcOldStart = 0;
    while( pcNextStart < pcEnd )
    {
      errno = 0;
      caNewLine.push_back( strtod( pcNextStart, &pcNextStart ) ) ;

      if( errno == ERANGE || ( pcNextStart == pcOldStart ) )
      {
        std::cerr << "Failed reading config file: `" << pchCfgFileName << "' Error parsing double values in Line: " << iLineNumber << ' ' << std::endl;
        assert( 0 );
        exit( EXIT_FAILURE );
      };
      pcOldStart = pcNextStart;

      while( ( pcNextStart < pcEnd ) && ( *pcNextStart == ' ' || *pcNextStart == '\t' || *pcNextStart == '\r' ) ) pcNextStart++;
    }

    if ( ( caNewLine.size() != 2 ) && ( caNewLine.size() != 7 ) && ( caNewLine.size() != 6 ) && ( caNewLine.size() != 8 ) )
    {
      std::cerr << "Failed reading config file: `" << pchCfgFileName << "'" << std::endl;
      std::cerr << "Invalid number of entries" << std::endl;
      AOF(false);
      exit( EXIT_FAILURE );
    }
    m_aadCameraParameters.push_back( caNewLine );
  }
  while( cCfgStream );
}

Void
TAppComCamPara::xGetCodedCameraData( UInt uiSourceView, UInt uiTargetView, Bool bByIdx,  UInt uiFrame, Int& riScale, Int& riOffset, Int& riPrecision )
{
  if( bByIdx )
  {
    uiSourceView = m_aiBaseViews[ uiSourceView ];
    uiTargetView = m_aiBaseViews[ uiTargetView ];
  }

  Int iFoundLine = -1;
  for( UInt uiCurViewLine = 0; uiCurViewLine < m_aadCameraParameters.size(); uiCurViewLine++ )
  {
    if ( m_aadCameraParameters[uiCurViewLine].size() == 2 )
      continue;

    if(      ( (Int)( m_aadCameraParameters[ uiCurViewLine ][ 3 ] * m_dViewNumPrec ) == uiSourceView )
          && ( (Int)( m_aadCameraParameters[ uiCurViewLine ][ 2 ] * m_dViewNumPrec ) == uiTargetView )
      )
    {
      if( ( (UInt)m_aadCameraParameters[ uiCurViewLine ][ 0 ] <= uiFrame ) && ( (UInt)m_aadCameraParameters[ uiCurViewLine ][ 1 ] >= uiFrame ) )
      {
        if( iFoundLine != -1 )
        {
          std::cerr << "Error CameraParameters for SourceView " << (Double) uiSourceView / m_dViewNumPrec << " and Target View " << (Double) uiTargetView / m_dViewNumPrec << " and Frame " << uiFrame << " given multiple times."  << std::endl;
          AOT(true);
          exit( EXIT_FAILURE );
        }
        else
        {
          iFoundLine = uiCurViewLine;
        }
      }
    }
  }

  if ( iFoundLine == -1 )
  {
    std::cerr << "Error CameraParameters for SourceView " << (Double) uiSourceView / m_dViewNumPrec << " and Target View " << (Double) uiTargetView / m_dViewNumPrec << " and Frame " << uiFrame << " not found."  << std::endl;
    AOT(true);
    exit( EXIT_FAILURE );
  }

  riScale     = (Int)( m_aadCameraParameters[ iFoundLine ][ 4 ] );
  riOffset    = (Int)( m_aadCameraParameters[ iFoundLine ][ 5 ] );
  riPrecision = (Int)( m_aadCameraParameters[ iFoundLine ][ 6 ] );
}

Bool
TAppComCamPara::xGetCameraDataRow( Int iView, UInt uiFrame, UInt& ruiFoundLine )
{
  ruiFoundLine = -1;
  for( UInt uiCurViewLine = 0; uiCurViewLine < m_aadCameraParameters.size(); uiCurViewLine++ )
  {
    if( (Int)( m_aadCameraParameters[ uiCurViewLine ][ 0 ] * m_dViewNumPrec ) == iView )
    {
      if( ( (UInt)m_aadCameraParameters[ uiCurViewLine ][ 1 ] <= uiFrame ) && ( (UInt)m_aadCameraParameters[ uiCurViewLine ][ 2 ] >= uiFrame ) )
      {
        if( ruiFoundLine != -1 )
        {
          std::cerr << "Error CameraParameters for View " << (Double) iView / m_dViewNumPrec << " and Frame " << uiFrame << " given multiple times."  << std::endl;
          exit( EXIT_FAILURE );
        }
        else
        {
          ruiFoundLine = uiCurViewLine;
        }
      }
    }
  }
  return ( ruiFoundLine == -1 );
}


Void
TAppComCamPara::xGetSortedViewList( const std::vector<Int>& raiViews, std::vector<Int>& raiSortedViews, std::vector<Int>& raiId2SortedId, std::vector<Int>& raiSortedId2Id )
{
  AOF( raiViews.size() > 0 );
  Int iNumViews   = (Int)raiViews.size();
  raiId2SortedId  = std::vector<Int>( raiViews.size(), -1 );
  raiSortedId2Id.clear();
  raiSortedViews.clear();
  for( Int iSortId = 0; iSortId < iNumViews; iSortId++ )
  {
    Int  iLeftMostBaseId = -1;
    for( Int iBaseId = 0; iLeftMostBaseId == -1 && iBaseId < iNumViews; iBaseId++ )
    {
      if( raiId2SortedId[ iBaseId ] == -1 )
      {
        UInt   uiFoundLine   = -1;
        xGetCameraDataRow( raiViews[ iBaseId ], 0, uiFoundLine );
        AOT(   uiFoundLine  == -1 ); // something wrong
        Double dXPos         = m_aadCameraParameters[ uiFoundLine ][ 4 ];
        Double dZNear        = m_aadCameraParameters[ uiFoundLine ][ 6 ];
        Double dZFar         = m_aadCameraParameters[ uiFoundLine ][ 7 ];
        Double dSign         = ( dZFar > 0 ? 1.0 : -1.0 );
        Bool   bLeftMost     = true;
        AOF( dZNear * dZFar  > 0.0 ); // otherwise, z parameters are not correct

        for( Int iTestBaseId = 0; bLeftMost && iTestBaseId < iNumViews; iTestBaseId++ )
        {
          if( iTestBaseId != iBaseId && raiId2SortedId[ iTestBaseId ] == -1 )
          {
            UInt   uiFoundLineTest  = -1;
            xGetCameraDataRow( raiViews[ iTestBaseId ], 0, uiFoundLineTest );
            AOT(   uiFoundLineTest == -1 ); // something wrong
            Double dXPosTest        = m_aadCameraParameters[ uiFoundLineTest ][ 4 ];
            Double dZNearTest       = m_aadCameraParameters[ uiFoundLineTest ][ 6 ];
            Double dZFarTest        = m_aadCameraParameters[ uiFoundLineTest ][ 7 ];
            AOF( dZNearTest * dZFarTest > 0.0 ); // otherwise, z parameters are not correct
            AOF( dZNearTest * dSign     > 0.0 ); // otherwise, z parameters are not consistent
            Double dDeltaXPos       = dSign * ( dXPosTest - dXPos );
            bLeftMost               = ( bLeftMost && dDeltaXPos > 0.0 );
          }
        }
        if( bLeftMost )
        {
          iLeftMostBaseId = iBaseId;
        }
      }
    }
    AOT( iLeftMostBaseId == -1 ); // something wrong
    raiId2SortedId[ iLeftMostBaseId ] = iSortId;
    raiSortedId2Id.push_back( iLeftMostBaseId );
    raiSortedViews.push_back( raiViews[ iLeftMostBaseId ] );
  }

  // sanity check
  if( iNumViews > 2 )
  {
    Int   iDeltaView  = gSign( raiSortedViews[ 1 ] - raiSortedViews[ 0 ] );
    Bool  bOutOfOrder = false;
    for(  Int  iSIdx  = 2; iSIdx < iNumViews; iSIdx++ )
    {
      bOutOfOrder = ( bOutOfOrder || iDeltaView * gSign( raiSortedViews[ iSIdx ] - raiSortedViews[ iSIdx - 1 ] ) < 0 );
    }
    if( bOutOfOrder )
    {
      std::cerr << "ERROR: View numbering must be strictly increasing or decreasing from left to right" << std::endl;
      exit(EXIT_FAILURE);
    }
  }
}


Void
TAppComCamPara::xGetViewOrderIndices( const std::vector<Int>& raiId2SortedId, std::vector<Int>& raiVOIdx )
{
  AOF( raiId2SortedId.size() );
  raiVOIdx  =      raiId2SortedId;
  Int iSize = (Int)raiId2SortedId.size();
  Int iOffs =      raiId2SortedId[ 0 ];
  for( Int iIdx = 0; iIdx < iSize; iIdx++ )
  {
    raiVOIdx[ iIdx ] -= iOffs;
  }
}


Bool
TAppComCamPara::xGetCamParsChangeFlag()
{
  Bool bChangeDetected = false;
  for( Int iBaseViewId = 0; !bChangeDetected && iBaseViewId < m_iNumberOfBaseViews; iBaseViewId++ )
  {
    if ( m_bSetupFromCoded )
    {
      for( Int iTargetViewId = 0; !bChangeDetected && iTargetViewId < m_iNumberOfBaseViews; iTargetViewId++ )
      {
        Int iTargetView = m_aiBaseViews[iTargetViewId];
        Int iSourceView = m_aiBaseViews[iBaseViewId  ];

        Int iS1 ,iSX;
        Int iO1 ,iOX;
        Int iP1 ,iPX;

        if ( iSourceView == iTargetView )
          continue;

        xGetCodedCameraData( iSourceView, iTargetView, false, 0, iS1, iO1, iP1 );
        for( UInt uiFrameId = m_uiFirstFrameId + 1; !bChangeDetected && uiFrameId <= m_uiLastFrameId; uiFrameId++ )
        {
          xGetCodedCameraData( iSourceView, iTargetView, false, uiFrameId, iSX, iOX, iPX );

          if( iS1 != iSX || iO1 != iOX || iP1 != iPX )
          {
            bChangeDetected = true;
          }
        }
      }
    }
    else
    {
    Int     iBaseView  = m_aiBaseViews[ iBaseViewId ];
    Double  dFL1, dFLX;
    Double  dCP1, dCPX;
    Double  dCS1, dCSX;
    Double  dZN1, dZNX;
    Double  dZF1, dZFX;
    Bool    bInterpolated;
    xGetGeometryData( iBaseView, m_uiFirstFrameId, dFL1, dCP1, dCS1, bInterpolated );  AOT( bInterpolated );
    xGetZNearZFar   ( iBaseView, m_uiFirstFrameId, dZN1, dZF1 );

    for( UInt uiFrameId = m_uiFirstFrameId + 1; !bChangeDetected && uiFrameId <= m_uiLastFrameId; uiFrameId++ )
    {
      xGetGeometryData( iBaseView, uiFrameId, dFLX, dCPX, dCSX, bInterpolated );  AOT( bInterpolated );
      xGetZNearZFar   ( iBaseView, uiFrameId, dZNX, dZFX );

      if( dFL1 != dFLX || dCP1 != dCPX || dCS1 != dCSX || dZN1 != dZNX || dZF1 != dZFX )
      {
        bChangeDetected = true;
      }
    }
  }
  }
  return bChangeDetected;
}

Int
TAppComCamPara::xGetViewId( std::vector<Int> aiViewList, Int iBaseView )
{
  Int  iViewId = -1;
  for( Int iId = 0; iId < (Int)aiViewList.size(); iId++ )
  {
    if( aiViewList[ iId ] == iBaseView )
    {
      iViewId = iId;
      break;
    }
  }
  AOT(   iViewId == -1 );
  return iViewId;
}

Int
TAppComCamPara::xGetBaseViewId( Int iBaseView )
{
  return xGetViewId( m_aiBaseViews, iBaseView );
}


Bool
TAppComCamPara::xGetLeftRightView( Int iView, std::vector<Int> aiSortedViews, Int& riLeftView, Int& riRightView, Int& riLeftSortedViewIdx, Int& riRightSortedViewIdx )
{
  Bool bFoundLRView  = false;
  Int  iLeftView     = -1;
  Int  iRightView    = -1;
  Int  iLeftViewIdx  = -1;
  Int  iRightViewIdx = -1;
  Bool bDecencdingVN = ( aiSortedViews.size() >= 2 && aiSortedViews[ 0 ] > aiSortedViews[ 1 ] );
  Int  iFactor       = ( bDecencdingVN ? -1 : 1 );

  for( Int iIdx = -1; iIdx < (Int)aiSortedViews.size(); iIdx++ )
  {
    if( iIdx == -1 )
    {
      if( ( aiSortedViews[ iIdx + 1 ] - iView ) * iFactor > 0  )
      {
        bFoundLRView  = false;
        iLeftView     = -1;
        iRightView    = aiSortedViews[ iIdx + 1 ];
        iLeftViewIdx  = -1;
        iRightViewIdx = iIdx + 1;
        break;
      }
    }
    else if ( iIdx == (Int)aiSortedViews.size() - 1 )
    {
      if( ( aiSortedViews[ iIdx ] - iView ) * iFactor < 0  )
      {
        bFoundLRView  = false;
        iLeftView     = aiSortedViews[ iIdx ];
        iRightView    = -1;
        iLeftViewIdx  = iIdx;
        iRightViewIdx = -1;
        break;
      }
    }
    else
    {
      if( ( ( aiSortedViews[ iIdx ] - iView ) * iFactor <= 0 ) && ( ( aiSortedViews[ iIdx + 1 ] - iView ) * iFactor >= 0 ) )
      {
        bFoundLRView  = true;
        iLeftView     = aiSortedViews[ iIdx ];
        iRightView    = aiSortedViews[ iIdx + 1 ];
        iLeftViewIdx  = iIdx;
        iRightViewIdx = iIdx + 1;
        break;
      }
    }
  }

  if ( ( iView == iLeftView ) || ( iView == iRightView ) )
  {
    iLeftViewIdx  = ( iView == iLeftView ) ? iLeftViewIdx : iRightViewIdx;
    iRightViewIdx = iLeftViewIdx;
    iLeftView     = iView;
    iRightView    = iView;
    bFoundLRView  = false;
  }

  riLeftView           = iLeftView;
  riRightView          = iRightView;
  riLeftSortedViewIdx  = iLeftViewIdx;
  riRightSortedViewIdx = iRightViewIdx;

  return bFoundLRView;
}


Void
TAppComCamPara::xGetPrevAndNextBaseView( Int iSourceViewNum, Int iTargetViewNum, Int& riPrevBaseViewNum, Int& riNextBaseViewNum )
{
  Int iLeftView;
  Int iRightView;
  Int iDummy;
  xGetLeftRightView( iTargetViewNum, m_aiSortedBaseViews, iLeftView, iRightView, iDummy, iDummy );

  if( iLeftView == iRightView )
  {
    riPrevBaseViewNum = iLeftView;
    riNextBaseViewNum = iLeftView;
  }
  else
  {
    Bool bDecencdingVN   = ( m_aiSortedBaseViews.size() >= 2 && m_aiSortedBaseViews[ 0 ] > m_aiSortedBaseViews[ 1 ] );
    Bool bNextViewIsLeft = ( bDecencdingVN ? ( iSourceViewNum < iTargetViewNum ) : ( iSourceViewNum > iTargetViewNum ) );
    if ( bNextViewIsLeft )
    {
      riPrevBaseViewNum = iRightView;
      riNextBaseViewNum = iLeftView;
    }
    else
    {
      riPrevBaseViewNum = iLeftView;
      riNextBaseViewNum = iRightView;
    }
  }
}


Void
TAppComCamPara::xGetZNearZFar( Int iView, UInt uiFrame, Double& rdZNear, Double& rdZFar )
{
  UInt uiFoundLine = -1;
  if( !xGetCameraDataRow( iView, uiFrame, uiFoundLine ) || !( m_aadCameraParameters[ uiFoundLine ].size() < 8 ) )
  {
    rdZNear = m_aadCameraParameters[ uiFoundLine ][ 6 ];
    rdZFar  = m_aadCameraParameters[ uiFoundLine ][ 7 ];
  }
  else
  {
    std::cerr << "No ZNear or no ZFar for View " << (Double)iView / m_dViewNumPrec << " and Frame " << uiFrame << " given in CameraParameterFile" << std::endl;
    exit( EXIT_FAILURE );
  }
}


Void
TAppComCamPara::xGetGeometryData( Int iView, UInt uiFrame, Double& rdFocalLength, Double& rdPosition, Double& rdCameraShift, Bool& rbInterpolated )
{
  UInt uiFoundLine = -1;
  if ( !xGetCameraDataRow( iView, uiFrame, uiFoundLine ) )
  {
    AOT( m_aadCameraParameters[ uiFoundLine ].size() < 6 );
    rbInterpolated = false;
    rdFocalLength =  m_aadCameraParameters[ uiFoundLine ][ 3 ];
    rdPosition    =  m_aadCameraParameters[ uiFoundLine ][ 4 ];
    rdCameraShift =  m_aadCameraParameters[ uiFoundLine ][ 5 ];
  }
  else
  {
    UInt uiLeftViewLine;
    UInt uiRightViewLine;
    Int  iLeftView;
    Int  iRightView;
    Int  iDummy;

    if( !xGetLeftRightView( iView, m_aiViewsInCfgFile, iLeftView, iRightView, iDummy, iDummy ) ||
         xGetCameraDataRow( iLeftView,  uiFrame, uiLeftViewLine  )                             ||
         xGetCameraDataRow( iRightView, uiFrame, uiRightViewLine )
      )
    {
      std::cerr << "No Left or no Right View next to View " << (Double)iView / m_dViewNumPrec << " for Frame " << uiFrame << " given in CameraParameterFile" << std::endl;
      AOT(true);
      exit( EXIT_FAILURE );
    }
    AOT( m_aadCameraParameters[ uiLeftViewLine  ].size() < 6 );
    AOT( m_aadCameraParameters[ uiRightViewLine ].size() < 6 );

    // Linear Interpolation
    Double dFactor = ( (Double)( iView - iLeftView ) ) / ( (Double)( iRightView - iLeftView ) );
    rdFocalLength  = m_aadCameraParameters[ uiLeftViewLine ][ 3 ] + dFactor * ( m_aadCameraParameters[ uiRightViewLine ][ 3 ] - m_aadCameraParameters[ uiLeftViewLine ][ 3 ] );
    rdPosition     = m_aadCameraParameters[ uiLeftViewLine ][ 4 ] + dFactor * ( m_aadCameraParameters[ uiRightViewLine ][ 4 ] - m_aadCameraParameters[ uiLeftViewLine ][ 4 ] );
    rdCameraShift  = m_aadCameraParameters[ uiLeftViewLine ][ 5 ] + dFactor * ( m_aadCameraParameters[ uiRightViewLine ][ 5 ] - m_aadCameraParameters[ uiLeftViewLine ][ 5 ] );
    rbInterpolated = true;
  }
}


Bool
TAppComCamPara::xGetShiftParameterReal( UInt uiSourceView, UInt uiTargetView, UInt uiFrame, Bool bExternal, Bool bByIdx, Double& rdScale, Double& rdOffset )
{
  AOT( m_bSetupFromCoded );

  Bool   bInterpolatedSource;
  Double dMinDepthSource;
  Double dMaxDepthSource;
  Double dFocalLengthSource;
  Double dPositionSource;
  Double dIntersectionSource;

  Bool   bInterpolatedTarget;
  Double dPositionTarget;
  Double dIntersectionTarget;
  Double dFocalLengthTarget;

  Int    iTargetViewNum;
  Int    iSourceViewNum;

  if( bByIdx )
  {
    iSourceViewNum = m_aiBaseViews[ uiSourceView ];
    iTargetViewNum = ( bExternal ? m_aiSynthViews[ uiTargetView ] : m_aiBaseViews[ uiTargetView ] );
  }
  else
  {
    iSourceViewNum = (Int) uiSourceView;
    iTargetViewNum = (Int) uiTargetView;
  }

  xGetGeometryData( iSourceViewNum, uiFrame, dFocalLengthSource, dPositionSource, dIntersectionSource, bInterpolatedSource );
  xGetZNearZFar   ( iSourceViewNum, uiFrame, dMinDepthSource,    dMaxDepthSource );
  xGetGeometryData( iTargetViewNum, uiFrame, dFocalLengthTarget, dPositionTarget, dIntersectionTarget, bInterpolatedTarget );

  Double dFactor = dFocalLengthSource * ( dPositionTarget - dPositionSource );
  rdScale        = dFactor * ( 1.0 / dMinDepthSource - 1.0 / dMaxDepthSource ) / (Double)( ( 1 << m_uiInputBitDepth ) - 1 );
  rdOffset       = dFactor / dMaxDepthSource - dIntersectionTarget + dIntersectionSource;

  return ( bInterpolatedSource || bInterpolatedTarget );
}


Void
TAppComCamPara::xGetShiftParameterCoded( UInt uiSourceView, UInt uiTargetView, UInt uiFrame, Bool bByIdx, Int& riScale, Int& riOffset )
{
  if ( m_bSetupFromCoded )
  {
    if ( uiSourceView == uiTargetView )
    {
      riScale  = 0;
      riOffset = 0;
      return;
    }
    Int iCamParsCodedPrecision;
    xGetCodedCameraData( uiSourceView, uiTargetView,  bByIdx, uiFrame, riScale, riOffset, iCamParsCodedPrecision );

    if ( m_bCamParsCodedPrecSet )
    {
      AOT( m_uiCamParsCodedPrecision != (UInt) iCamParsCodedPrecision );
    }
    else
    {
      m_uiCamParsCodedPrecision = (UInt) iCamParsCodedPrecision;
      m_bCamParsCodedPrecSet    = true;
    }
  }
  else
  {
  Double  dScale, dOffset;
  Bool    bInterpolated = xGetShiftParameterReal( uiSourceView, uiTargetView, uiFrame, false, bByIdx, dScale, dOffset );
  AOT(    bInterpolated ); // must be base view

  Double  dMultOffset   = (Double)( 1 << ( m_uiCamParsCodedPrecision + 1 ) );
  Double  dMultScale    = (Double)( 1 << ( m_uiCamParsCodedPrecision + 1 + m_uiInputBitDepth ) );
  riOffset              = (Int)floor( dMultOffset * dOffset + .5 );
  riScale               = (Int)floor( dMultScale  * dScale  + .5 );
}

}


Void
TAppComCamPara::xGetShiftParameterInt( UInt uiSourceView, UInt uiTargetView, UInt uiFrame, Bool bExternal, Bool bByIdx, Int64& riScale, Int64& riOffset )
{
  Int    iTargetViewNum;
  Int    iSourceViewNum;
  Int    iPrevBaseViewNum;
  Int    iNextBaseViewNum;
  Int    iSourceViewRelNum;
  Int    iTargetViewRelNum;

  if( bByIdx )
  {

    iSourceViewNum = m_aiBaseViews[ uiSourceView ];
    iSourceViewRelNum = m_aiBaseId2SortedId[ uiSourceView ] * ((Int) m_dViewNumPrec );

    if ( bExternal )
    {
      iTargetViewNum    = m_aiSynthViews      [ uiTargetView ];
      iTargetViewRelNum = m_aiRelSynthViewsNum[ uiTargetView ];
    }
    else
    {
      iTargetViewNum    = m_aiBaseViews       [ uiTargetView ];
      iTargetViewRelNum = m_aiBaseId2SortedId [ uiTargetView ] * ((Int) m_dViewNumPrec );
    }
  }
  else
  {
    iSourceViewNum = (Int) uiSourceView;
    iTargetViewNum = (Int) uiTargetView;
    iSourceViewRelNum = m_aiBaseId2SortedId[ xGetBaseViewId( uiSourceView) ] * ((Int) m_dViewNumPrec );

    if ( bExternal )
    {
      iTargetViewRelNum = m_aiRelSynthViewsNum[ xGetViewId( m_aiSynthViews, (Int) uiTargetView )];
    }
    else
    {
      iTargetViewRelNum = m_aiBaseId2SortedId[ xGetBaseViewId( uiTargetView) ] * ((Int) m_dViewNumPrec );
    }
  }
  xGetPrevAndNextBaseView( iSourceViewNum, iTargetViewNum, iPrevBaseViewNum, iNextBaseViewNum );
  AOT( iPrevBaseViewNum == -1 ); // should not happen
  AOT( iNextBaseViewNum == -1 ); // should not happen

  Int iSrcId    = xGetBaseViewId( iSourceViewNum   );
  Int iPrevId   = xGetBaseViewId( iPrevBaseViewNum );
  Int iNextId   = xGetBaseViewId( iNextBaseViewNum );
  AOF( m_aaiScaleAndOffsetSet[ iSrcId ][ iPrevId ] ); // coded scale and offset must be set
  AOF( m_aaiScaleAndOffsetSet[ iSrcId ][ iNextId ] ); // coded scale and offset must be set

  Int iNextBaseViewRelNum = m_aiBaseId2SortedId[ iNextId ] * ((Int) m_dViewNumPrec );
  Int iPrevBaseViewRelNum = m_aiBaseId2SortedId[ iPrevId ] * ((Int) m_dViewNumPrec );

  Int64 iPrevScale  = (Int64)m_aaiCodedScale [ iSrcId ][ iPrevId ];
  Int64 iNextScale  = (Int64)m_aaiCodedScale [ iSrcId ][ iNextId ];
  Int64 iPrevOffset = (Int64)m_aaiCodedOffset[ iSrcId ][ iPrevId ] << m_uiBitDepthForLUT;
  Int64 iNextOffset = (Int64)m_aaiCodedOffset[ iSrcId ][ iNextId ] << m_uiBitDepthForLUT;

  if( iPrevBaseViewNum == iNextBaseViewNum )
  {
    riScale   = iNextScale;
    riOffset  = iNextOffset;
  }
  else
  {
    riScale   = Int64( iTargetViewRelNum    - iPrevBaseViewRelNum ) * iNextScale;
    riScale  += Int64( iNextBaseViewRelNum  - iTargetViewRelNum   ) * iPrevScale;
    riOffset  = Int64( iTargetViewRelNum   - iPrevBaseViewRelNum ) * iNextOffset;
    riOffset += Int64( iNextBaseViewRelNum - iTargetViewRelNum   ) * iPrevOffset;
    Int64 iD  = Int64( iNextBaseViewRelNum - iPrevBaseViewRelNum );
    Int64 iSA = ( riScale  > 0 ? iD / 2 : -iD / 2 );
    Int64 iOA = ( riOffset > 0 ? iD / 2 : -iD / 2 );
    riScale   = ( riScale  + iSA  ) / iD;
    riOffset  = ( riOffset + iOA  ) / iD;
  }
}


Void
TAppComCamPara::xSetCodedScaleOffset( UInt uiFrame )
{
  for( UInt uiSourceId = 0; uiSourceId < m_iNumberOfBaseViews; uiSourceId++ )
  {
    for( UInt uiTargetId = 0; uiTargetId < m_iNumberOfBaseViews; uiTargetId++ )
    {
      Int iScale, iOffset;
      xGetShiftParameterCoded( uiSourceId, uiTargetId, uiFrame, true, iScale, iOffset );
      m_aaiCodedScale        [ uiSourceId ][ uiTargetId ] = iScale;
      m_aaiCodedOffset       [ uiSourceId ][ uiTargetId ] = iOffset;
      m_aaiScaleAndOffsetSet [ uiSourceId ][ uiTargetId ] = 1;
    }
  }
}


Void
TAppComCamPara::xSetShiftParametersAndLUT( UInt uiNumberSourceViews, UInt uiNumberTargetViews, UInt uiFrame, Bool bExternalReference , Double****& radLUT, Int****& raiLUT, Double***& radShiftParams, Int64***& raiShiftParams )
{
  if( uiNumberSourceViews <= 1 || uiNumberTargetViews == 0 )
  {
    return;
  }
  AOF( radShiftParams != NULL && raiShiftParams != NULL && radLUT != NULL && raiLUT != NULL );
  AOF( m_uiBitDepthForLUT == 8 );

  Int     iLog2DivLuma   = m_uiBitDepthForLUT + m_uiCamParsCodedPrecision + 1 - m_iLog2Precision;   AOF( iLog2DivLuma > 0 );
  Int     iLog2DivChroma = iLog2DivLuma + 1;
  Double  dMaxDispDev    = 0.0;
  Double  dMaxRndDispDvL = 0.0;
  Double  dMaxRndDispDvC = 0.0;
  for( UInt uiSourceView = 0; uiSourceView < uiNumberSourceViews; uiSourceView++ )
  {
    for( UInt uiTargetView = 0; uiTargetView < uiNumberTargetViews; uiTargetView++ )
    {

      // integer-valued scale and offset
      Int64 iScale, iOffset;
      xGetShiftParameterInt ( uiSourceView, uiTargetView, uiFrame, bExternalReference, true, iScale, iOffset );
      raiShiftParams[ uiSourceView][ uiTargetView ][ 0 ] = iScale;
      raiShiftParams[ uiSourceView][ uiTargetView ][ 1 ] = iOffset;

      // offsets including rounding offsets
      Int64 iOffsetLuma   = iOffset + ( ( 1 << iLog2DivLuma   ) >> 1 );
      Int64 iOffsetChroma = iOffset + ( ( 1 << iLog2DivChroma ) >> 1 );

      // real-valued scale and offset
      Double dScale, dOffset;

      if ( m_bSetupFromCoded )
      {
        dScale  = (Double) iScale  / (( Double ) ( 1 << iLog2DivLuma ));
        dOffset = (Double) iOffset / (( Double ) ( 1 << iLog2DivLuma ));
      }
      else
      {
        xGetShiftParameterReal( uiSourceView, uiTargetView, uiFrame, bExternalReference, true, dScale, dOffset );
      }

      radShiftParams[ uiSourceView][ uiTargetView ][ 0 ] = dScale;
      radShiftParams[ uiSourceView][ uiTargetView ][ 1 ] = dOffset;

      for( UInt uiDepthValue = 0; uiDepthValue < 256; uiDepthValue++ )
      {
        // real-valued look-up tables
        Double  dShiftLuma      = ( (Double)uiDepthValue * dScale + dOffset ) * Double( 1 << m_iLog2Precision );
        Double  dShiftChroma    = dShiftLuma / 2;
        radLUT[ uiSourceView ][ uiTargetView ][ 0 ][ uiDepthValue ] = dShiftLuma;
        radLUT[ uiSourceView ][ uiTargetView ][ 1 ][ uiDepthValue ] = dShiftChroma;

        // integer-valued look-up tables
        Int64   iTempScale      = (Int64)uiDepthValue * iScale;
        Int64   iTestScale      = ( iTempScale + iOffset       );   // for checking accuracy of camera parameters
        Int64   iShiftLuma      = ( iTempScale + iOffsetLuma   ) >> iLog2DivLuma;
        Int64   iShiftChroma    = ( iTempScale + iOffsetChroma ) >> iLog2DivChroma;
        raiLUT[ uiSourceView ][ uiTargetView ][ 0 ][ uiDepthValue ] = (Int)iShiftLuma;
        raiLUT[ uiSourceView ][ uiTargetView ][ 1 ][ uiDepthValue ] = (Int)iShiftChroma;

        // maximum deviation
        dMaxDispDev     = Max( dMaxDispDev,    fabs( Double( (Int) iTestScale   ) - dShiftLuma * Double( 1 << iLog2DivLuma ) ) / Double( 1 << iLog2DivLuma ) );
        dMaxRndDispDvL  = Max( dMaxRndDispDvL, fabs( Double( (Int) iShiftLuma   ) - dShiftLuma   ) );
        dMaxRndDispDvC  = Max( dMaxRndDispDvC, fabs( Double( (Int) iShiftChroma ) - dShiftChroma ) );
      }

      radLUT[ uiSourceView ][ uiTargetView ][ 0 ][ 256 ] = radLUT[ uiSourceView ][ uiTargetView ][ 0 ][ 255 ];
      radLUT[ uiSourceView ][ uiTargetView ][ 1 ][ 256 ] = radLUT[ uiSourceView ][ uiTargetView ][ 1 ][ 255 ];
      raiLUT[ uiSourceView ][ uiTargetView ][ 0 ][ 256 ] = raiLUT[ uiSourceView ][ uiTargetView ][ 0 ][ 255 ];
      raiLUT[ uiSourceView ][ uiTargetView ][ 1 ][ 256 ] = raiLUT[ uiSourceView ][ uiTargetView ][ 1 ][ 255 ];
    }
  }

  // check maximum deviation
  Double  dMaxAllowedDispDev    =       Double( 1 << m_iLog2Precision ) / Double( 1 << m_uiCamParsCodedPrecision );       //  counting only the impact of camera parameter rounding
  Double  dMaxAllowedRndDispDvL = 0.5 + Double( 1 << m_iLog2Precision ) / Double( 1 << m_uiCamParsCodedPrecision );       // final rounding and impact of camera parameter rounding
  Double  dMaxAllowedRndDispDvC = 0.5 + Double( 1 << m_iLog2Precision ) / Double( 1 << m_uiCamParsCodedPrecision ) / 2.0; // final rounding and impact of camera parameter rounding

  if( ( dMaxDispDev >= dMaxAllowedDispDev || dMaxRndDispDvL >= dMaxAllowedRndDispDvL || dMaxRndDispDvC >= dMaxAllowedRndDispDvC ) && !m_bSetupFromCoded )
  {
    std::cout << "Warning: Something wrong with the accuracy of coded camera parameters:" << std::endl;
    if( dMaxDispDev    >= dMaxAllowedDispDev    )
    {
      std::cout << "   max disparity difference is " << dMaxDispDev    << " (allowed: " << dMaxAllowedDispDev    << ")" << std::endl;
    }
    if( dMaxRndDispDvL >= dMaxAllowedRndDispDvL )
    {
      std::cout << "   max rnd luma   disp diff is " << dMaxRndDispDvL << " (allowed: " << dMaxAllowedRndDispDvL << ")" << std::endl;
    }
    if( dMaxRndDispDvC >= dMaxAllowedRndDispDvC )
    {
      std::cout << "   max rnd chroma disp diff is " << dMaxRndDispDvC << " (allowed: " << dMaxAllowedRndDispDvC << ")" << std::endl;
    }
  }
}


Void
TAppComCamPara::xSetShiftParametersAndLUT( UInt uiFrame )
{
  xInit2dArray             ( (UInt)m_iNumberOfBaseViews, (UInt)m_iNumberOfBaseViews,  m_aaiScaleAndOffsetSet, 0 );
  xSetCodedScaleOffset     (                                                          uiFrame );
  xSetShiftParametersAndLUT( (UInt)m_iNumberOfBaseViews, (UInt)m_iNumberOfBaseViews,  uiFrame, false, m_adBaseViewShiftLUT,  m_aiBaseViewShiftLUT,  m_adBaseViewShiftParameter,  m_aiBaseViewShiftParameter  );
  xSetShiftParametersAndLUT( (UInt)m_iNumberOfBaseViews, (UInt)m_iNumberOfSynthViews, uiFrame, true,  m_adSynthViewShiftLUT, m_aiSynthViewShiftLUT, m_adSynthViewShiftParameter, m_aiSynthViewShiftParameter );
};


Void
TAppComCamPara::xGetCameraShifts( UInt uiSourceView, UInt uiTargetView, UInt uiFrame, Double& rdCamPosShift, Double& rdPicPosShift )
{
  Double  dDummy, dCamPosSource, dCamPosTarget, dPicPosSource, dPicPosTarget;
  Bool    bInterpolatedSource, bInterpolatedTarget;
  Int     iTargetViewNum = m_aiBaseViews[ uiTargetView ];
  Int     iSourceViewNum = m_aiBaseViews[ uiSourceView ];

  xGetGeometryData( iSourceViewNum, uiFrame, dDummy, dCamPosSource, dPicPosSource, bInterpolatedSource );
  xGetGeometryData( iTargetViewNum, uiFrame, dDummy, dCamPosTarget, dPicPosTarget, bInterpolatedTarget );
  AOT( bInterpolatedSource || bInterpolatedTarget );

  rdCamPosShift =  ( dCamPosTarget - dCamPosSource );
  rdPicPosShift = -( dPicPosTarget - dPicPosSource ); // to be consistent
}


Void
TAppComCamPara::xSetPdmConversionParams()
{
  AOF( m_aiViewOrderIndex[ 0 ] == 0 );
  if ( m_bSetupFromCoded || m_iNumberOfBaseViews    <  2 )
  {
    return;
  }

  //--- determine (virtual) camera parameter shift between view order index 1 and base view (view order index 0) ---
  Double        dCamPosShift, dPicPosShift;
  Int           iMinVOI       = (1<<30);
  Int           iMinAbsVOI    = (1<<30);
  Int           iMinAbsVOIId  = 0;
  for( Int iBaseId = 1; iBaseId < m_iNumberOfBaseViews; iBaseId++ )
  {
    Int iAbsVOI = ( m_aiViewOrderIndex[ iBaseId ] < 0 ? -m_aiViewOrderIndex[ iBaseId ] : m_aiViewOrderIndex[ iBaseId ] );
    if( iAbsVOI < iMinAbsVOI )
    {
      iMinVOI      = m_aiViewOrderIndex[ iBaseId ];
      iMinAbsVOI   = iAbsVOI;
      iMinAbsVOIId = iBaseId;
    }
  }
  AOF( iMinAbsVOIId != 0 && iMinAbsVOI != 0 );
  xGetCameraShifts( 0, iMinAbsVOIId, m_uiFirstFrameId, dCamPosShift, dPicPosShift );
  Double  dCamPosShiftVOI01     = dCamPosShift / Double( iMinVOI );
  Double  dAbsCamPosShiftVOI01  = ( dCamPosShiftVOI01 < 0.0 ? -dCamPosShiftVOI01 : dCamPosShiftVOI01 );

  //--- determine maximum absolute camera position shift, precision, and base scale ---
  Double  dMaxAbsCamPosShift = 0.0;
  for( Int iTargetId = 1; iTargetId < m_iNumberOfBaseViews; iTargetId++ )
  {
    for( Int iBaseId = 0; iBaseId < iTargetId; iBaseId++ )
    {
      xGetCameraShifts( (UInt)iBaseId, (UInt)iTargetId, m_uiFirstFrameId, dCamPosShift, dPicPosShift );
      dCamPosShift        = ( dCamPosShift < 0.0                ? -dCamPosShift : dCamPosShift       );
      dMaxAbsCamPosShift  = ( dCamPosShift > dMaxAbsCamPosShift ?  dCamPosShift : dMaxAbsCamPosShift );
    }
  }
  Double  dEpsilon    = 1e-15;
  Double  dShiftRatio = dMaxAbsCamPosShift / dAbsCamPosShiftVOI01 - dEpsilon;
  Int     iPrecision  = 0;  for( ; (Double)( 1 << iPrecision ) < dShiftRatio; iPrecision++ );
  Int     iPrecShift  = iPrecision + PDM_INTER_CALC_SHIFT + PDM_VIRT_DEPTH_PRECISION - 2;
  AOF(    iPrecShift  < PDM_INTERNAL_CALC_BIT_DEPTH );
  Int     iScaleVOI01 = 1 << iPrecShift;
  m_iPdmPrecision     = iPrecision;

  //--- loop over target views ---
  for( Int iTargetId = 1; iTargetId < m_iNumberOfBaseViews; iTargetId++ )
  {
    // set scale and offset parameters for other views
    for( Int iBaseId = 0; iBaseId < iTargetId; iBaseId++ )
    {
      xGetCameraShifts( (UInt)iBaseId, (UInt)iTargetId, m_uiFirstFrameId, dCamPosShift, dPicPosShift );
      Double  dScale      = Double( iScaleVOI01 ) * dCamPosShiftVOI01 / dCamPosShift;
      Int     iDiv        = m_aiViewOrderIndex[ iTargetId ] - m_aiViewOrderIndex[ iBaseId ];
      Int     iAdd        = ( iDiv > 0 ? iDiv / 2 : -iDiv / 2 );
      Int     iScalePred  = ( iScaleVOI01 + iAdd ) / iDiv;
      Double  dFactor     = dScale / (Double)iScalePred * pow( 2.0, PDM_LOG4_SCALE_DENOMINATOR );
      Int     iNominator  = (Int)floor( dFactor + .5 );
      Int     iNomDelta   = iNominator - ( 1 << PDM_LOG4_SCALE_DENOMINATOR );
      Int     iScale      = Int( ( (Int64)iNominator * (Int64)iScalePred + (Int64)( ( 1 << PDM_LOG4_SCALE_DENOMINATOR ) >> 1 ) ) >> PDM_LOG4_SCALE_DENOMINATOR );
      Double  dOffset     = -dPicPosShift * Double( iScale ) * pow( 2.0, 2 - PDM_OFFSET_SHIFT );
      Int     iOffset     = (Int)floor( dOffset + .5 );

      m_aaiPdmScaleNomDelta [ iTargetId ][ iBaseId ]  = iNomDelta;
      m_aaiPdmOffset        [ iTargetId ][ iBaseId ]  = iOffset;
    }
  }
}



TAppComCamPara::TAppComCamPara()
{
  m_dViewNumPrec              = VIEW_NUM_PREC;  // fixed
  m_iLog2Precision            = -1;
  m_uiInputBitDepth           = 0;
  m_uiBitDepthForLUT          = 8;              // fixed
  m_uiFirstFrameId            = 0;
  m_uiLastFrameId             = 0;

  m_iNumberOfBaseViews        = -1;
  m_iNumberOfSynthViews       = -1;

  m_uiCamParsCodedPrecision   = 0;
  m_bCamParsVaryOverTime      = true;

  m_aaiCodedScale             = 0;
  m_aaiCodedOffset            = 0;
  m_aaiScaleAndOffsetSet      = 0;

  m_iPdmPrecision             = 0;
  m_aaiPdmScaleNomDelta       = 0;
  m_aaiPdmOffset              = 0;

  m_adBaseViewShiftParameter  = 0;
  m_aiBaseViewShiftParameter  = 0;
  m_adSynthViewShiftParameter = 0;
  m_aiSynthViewShiftParameter = 0;

  m_adBaseViewShiftLUT        = 0;
  m_aiBaseViewShiftLUT        = 0;
  m_adSynthViewShiftLUT       = 0;
  m_aiSynthViewShiftLUT       = 0;

  m_bSetupFromCoded           = false;
  m_bCamParsCodedPrecSet      = false;


}


TAppComCamPara::~TAppComCamPara()
{
  xDeleteArray( m_adBaseViewShiftParameter,  m_iNumberOfBaseViews, m_iNumberOfBaseViews     );
  xDeleteArray( m_aiBaseViewShiftParameter,  m_iNumberOfBaseViews, m_iNumberOfBaseViews     );
  xDeleteArray( m_adBaseViewShiftLUT,        m_iNumberOfBaseViews, m_iNumberOfBaseViews,  2 );
  xDeleteArray( m_aiBaseViewShiftLUT,        m_iNumberOfBaseViews, m_iNumberOfBaseViews,  2 );

  xDeleteArray( m_adSynthViewShiftParameter, m_iNumberOfBaseViews, Max(1,m_iNumberOfSynthViews));
  xDeleteArray( m_aiSynthViewShiftParameter, m_iNumberOfBaseViews, Max(1,m_iNumberOfSynthViews));
  xDeleteArray( m_adSynthViewShiftLUT,       m_iNumberOfBaseViews, Max(1,m_iNumberOfSynthViews), 2 );
  xDeleteArray( m_aiSynthViewShiftLUT,       m_iNumberOfBaseViews, Max(1,m_iNumberOfSynthViews), 2 );

  xDeleteArray( m_aaiCodedScale,             m_iNumberOfBaseViews );
  xDeleteArray( m_aaiCodedOffset,            m_iNumberOfBaseViews );
  xDeleteArray( m_aaiScaleAndOffsetSet,      m_iNumberOfBaseViews );

  xDeleteArray( m_aaiPdmScaleNomDelta,       m_iNumberOfBaseViews );
  xDeleteArray( m_aaiPdmOffset,              m_iNumberOfBaseViews );
}

Void
TAppComCamPara::xSetupBaseViewsFromCoded()
{
  //===== get and sort views given in camera parameter file and set list of base views and related arrays =====
  // get left-right order and coding order from cfg-file
  std::vector<Int> aiViewOrderIdx;   // Left Right Order
  std::vector<Int> aiViewId ;        // Coding     Order

  Int iMinViewOrderIdx = MAX_INT;
  for( UInt uiRow = 0; uiRow < m_aadCameraParameters.size(); uiRow++ )
  {
    if (m_aadCameraParameters[uiRow].size() != 2 )
      break;

    Int iViewOrderIdx  = (Int)( m_aadCameraParameters[ uiRow ][ 1 ] );
    iMinViewOrderIdx   = Min( iViewOrderIdx, iMinViewOrderIdx );

    aiViewOrderIdx     .push_back( iViewOrderIdx );
    aiViewId           .push_back( (Int) m_aadCameraParameters[ uiRow ][ 0 ]  );
  }

  // create base view numbers
  AOT( aiViewId.size() != aiViewOrderIdx.size() );
  m_iNumberOfBaseViews = (Int) aiViewId.size();
  for (Int iCurBaseView = 0; iCurBaseView < m_iNumberOfBaseViews; iCurBaseView++ )
  {
    aiViewOrderIdx[iCurBaseView] = ( aiViewOrderIdx[iCurBaseView] - iMinViewOrderIdx);
    m_aiBaseViews      .push_back(  aiViewOrderIdx[iCurBaseView] * ( (Int) m_dViewNumPrec) );
    m_aiBaseId2SortedId.push_back( iCurBaseView );
    m_aiBaseSortedId2Id.push_back( iCurBaseView );

  }

  m_iNumberOfBaseViews = (Int) m_aiBaseViews.size();

  std::vector<Int> aiSortedViewOrderIdx = aiViewOrderIdx;

  // sort base views according to View Order Idx
  m_aiSortedBaseViews = m_aiBaseViews;
  for (Int iCurBaseView = 1; iCurBaseView < m_iNumberOfBaseViews; iCurBaseView++ )
  {
    Int iCurViewOrder = aiSortedViewOrderIdx[iCurBaseView];
    for (Int iCurSearchPos = iCurBaseView; iCurSearchPos >= 0; iCurSearchPos-- )
    {
      if ( iCurViewOrder < aiSortedViewOrderIdx[iCurSearchPos] )
      {
        Int iTempViewId = m_aiSortedBaseViews[iCurSearchPos];
        m_aiSortedBaseViews[iCurSearchPos] = m_aiSortedBaseViews[iCurBaseView];
        m_aiSortedBaseViews[iCurBaseView ] = iTempViewId;

        Int iTempViewOrderIdx = aiSortedViewOrderIdx[iCurSearchPos];
        aiSortedViewOrderIdx[iCurSearchPos] = aiSortedViewOrderIdx[iCurBaseView];
        aiSortedViewOrderIdx[iCurBaseView ] = iTempViewOrderIdx;

        Int iTempPos = m_aiBaseSortedId2Id[iCurSearchPos];
        m_aiBaseSortedId2Id[iCurSearchPos] = m_aiBaseSortedId2Id[iCurBaseView];
        m_aiBaseSortedId2Id[iCurBaseView] = iTempPos;
        iCurBaseView--;
      }
    }
  }

  for (Int iCurBaseView = 0; iCurBaseView < m_iNumberOfBaseViews; iCurBaseView++ )
  {
    m_aiBaseId2SortedId[m_aiBaseSortedId2Id[iCurBaseView]] = iCurBaseView;
  }

  m_aiViewsInCfgFile = m_aiSortedBaseViews;

  // check
  if( m_aiViewsInCfgFile.size() < 2 )
  {
    std::cerr << "Failed reading camera parameter file" << std::endl;
    std::cerr << "At least two views must be given" << std::endl;
    AOT(true);
    exit( EXIT_FAILURE );
  }

  // translate coding order to view order
  for( UInt uiRow = 0; uiRow < m_aadCameraParameters.size(); uiRow++ )
{
    if (m_aadCameraParameters[uiRow].size() == 2 )
      continue;

    m_aadCameraParameters[ uiRow ][ 2 ] = (Double) aiViewOrderIdx[ xGetViewId( aiViewId, (Int) m_aadCameraParameters[ uiRow ][ 2 ] ) ];
    m_aadCameraParameters[ uiRow ][ 3 ] = (Double) aiViewOrderIdx[ xGetViewId( aiViewId, (Int) m_aadCameraParameters[ uiRow ][ 3 ] ) ];
  }
}

Void TAppComCamPara::xSetupBaseViews( Char* pchBaseViewNumbers, UInt uiNumBaseViews )
  {
    // init list
    std::vector<Int> aiViewsInCfg;
    for( UInt uiRow = 0; uiRow < m_aadCameraParameters.size(); uiRow++ )
    {
      aiViewsInCfg.push_back( (Int)( m_aadCameraParameters[ uiRow ][ 0 ] * m_dViewNumPrec ) );
    }
    // remove duplicated items
    std::sort( aiViewsInCfg.begin(), aiViewsInCfg.end() );
    std::vector<Int>::iterator cIterNewEnd = std::unique( aiViewsInCfg.begin(), aiViewsInCfg.end() );
    aiViewsInCfg.erase( cIterNewEnd, aiViewsInCfg.end() );
    // sort (from left to right)
    std::vector<Int> aiDummyI2SI, aiDummySI2I;
    xGetSortedViewList( aiViewsInCfg, m_aiViewsInCfgFile, aiDummyI2SI, aiDummySI2I );
    // check
    if( m_aiViewsInCfgFile.size() < 2 )
    {
    std::cerr << "Failed reading config file" << std::endl;
      std::cerr << "At least two views must be given" << std::endl;
      exit( EXIT_FAILURE );
    }



  //===== set list of base views and related arrays =====
  if( pchBaseViewNumbers == 0 )
  {
    std::cerr << "BaseViewCameraNumbers must be given" << std::endl;
    exit( EXIT_FAILURE );
  };

  convertNumberString( pchBaseViewNumbers, m_aiBaseViews, m_dViewNumPrec  );
  while( (UInt)m_aiBaseViews.size() > uiNumBaseViews )
  {
    m_aiBaseViews.pop_back();
  }
  xGetSortedViewList( m_aiBaseViews, m_aiSortedBaseViews, m_aiBaseId2SortedId, m_aiBaseSortedId2Id );
  m_iNumberOfBaseViews = (Int)m_aiBaseViews.size();
}


Void
TAppComCamPara::init( UInt   uiNumBaseViews,
                      UInt   uiInputBitDepth,
                      UInt   uiCodedCamParsPrecision,
                      UInt   uiStartFrameId,
                      UInt   uiNumFrames,
                      Char*  pchCfgFileName,
                      Char*  pchBaseViewNumbers,
                      Char*  pchSynthViewNumbers,
                      std::vector<Int>* paiSynthViewNumbers,
                      Int    iLog2Precision )
{
  //===== set miscellaneous variables =====
  m_uiInputBitDepth         = uiInputBitDepth;
  m_uiFirstFrameId          = uiStartFrameId;
  m_uiLastFrameId           = uiStartFrameId + uiNumFrames - 1;
  m_uiCamParsCodedPrecision = uiCodedCamParsPrecision;
  m_iLog2Precision          = iLog2Precision;

  xReadCameraParameterFile( pchCfgFileName );

  m_bSetupFromCoded         = ( m_aadCameraParameters[ 0 ].size() == 2 );

  if ( m_bSetupFromCoded )
  {
    std::cout << "Detected decoded camera parameter file. Overwriting base view settings from cfg file. " << std::endl;
    xSetupBaseViewsFromCoded();
  }
  else
  {
    xSetupBaseViews( pchBaseViewNumbers, uiNumBaseViews );
  }

  //===== set list of external (virtual) views =====
  m_aiSynthViews.clear();

  if( pchSynthViewNumbers != 0 || paiSynthViewNumbers != 0)
  {
    std::vector<Int> aiTmpSynthViews;

    AOT( ( pchSynthViewNumbers != NULL ) && ( paiSynthViewNumbers != NULL ) );

    if ( pchSynthViewNumbers != NULL )
    {
      convertNumberString( pchSynthViewNumbers, aiTmpSynthViews, m_dViewNumPrec );
    }
    else
    {
      aiTmpSynthViews = (*paiSynthViewNumbers);
    }

    for( UInt uiSId = 0; uiSId < (UInt)aiTmpSynthViews.size(); uiSId++ )
    {

      Int iViewNumPrec        = (Int) m_dViewNumPrec;
      Int iLeftBaseViewIdx    =   aiTmpSynthViews[ uiSId ]                        / iViewNumPrec;
      Int iRightBaseViewIdx   = ( aiTmpSynthViews[ uiSId ] + (iViewNumPrec - 1) ) / iViewNumPrec;

      if ( iLeftBaseViewIdx < 0 || iRightBaseViewIdx >= m_iNumberOfBaseViews )
      {
        std::cerr << "SynthViewCameraNumbers must be greater and equal to 0 and smaller than number of base views" << std::endl;
        AOT(true);
        exit( EXIT_FAILURE );
      }

      Int64  iLeftBaseViewRelNum = iLeftBaseViewIdx  * iViewNumPrec;
      Int64 iRightBaseViewRelNum = iRightBaseViewIdx * iViewNumPrec;

      Int64 iDiffBaseViewRelNum  = iRightBaseViewRelNum - iLeftBaseViewRelNum;

      Int64 iSynthViewRelNum     = aiTmpSynthViews[ uiSId ];
      Int64 iLeftBaseNum         = m_aiSortedBaseViews[ iLeftBaseViewIdx  ];
      Int64 iRightBaseNum        = m_aiSortedBaseViews[ iRightBaseViewIdx ];
      Int64 iDiffBaseNum         = iRightBaseNum - iLeftBaseNum;
      Int64 iSynthViewNum;

      if ( iDiffBaseViewRelNum != 0)
      {
        AOT( (Int) iDiffBaseViewRelNum != iViewNumPrec );
        Int iFact = iDiffBaseNum > 0 ? 1 : -1;
        iSynthViewNum = iLeftBaseNum + ( iDiffBaseNum * ( iSynthViewRelNum - iLeftBaseViewRelNum ) + (iViewNumPrec >> 1) * iFact ) / ( iViewNumPrec );
      }
      else
      {
        iSynthViewNum = iLeftBaseNum;
      }

      m_aiRelSynthViewsNum.push_back(  aiTmpSynthViews[ uiSId ] );
      m_aiSynthViews      .push_back(  (Int) iSynthViewNum  );
    }
  }
  m_iNumberOfSynthViews = (Int)m_aiSynthViews.size();


  //===== set derived parameters =====
  xGetViewOrderIndices( m_aiBaseId2SortedId, m_aiViewOrderIndex );
  m_bCamParsVaryOverTime = xGetCamParsChangeFlag();


  //===== create arrays =====
  xCreateLUTs   ( (UInt)m_iNumberOfBaseViews, (UInt)m_iNumberOfBaseViews,  m_adBaseViewShiftLUT,  m_aiBaseViewShiftLUT,  m_adBaseViewShiftParameter,  m_aiBaseViewShiftParameter  );
  xCreateLUTs   ( (UInt)m_iNumberOfBaseViews, (UInt)m_iNumberOfSynthViews, m_adSynthViewShiftLUT, m_aiSynthViewShiftLUT, m_adSynthViewShiftParameter, m_aiSynthViewShiftParameter );
  xCreate2dArray( (UInt)m_iNumberOfBaseViews, (UInt)m_iNumberOfBaseViews,  m_aaiCodedScale           );
  xCreate2dArray( (UInt)m_iNumberOfBaseViews, (UInt)m_iNumberOfBaseViews,  m_aaiCodedOffset          );
  xCreate2dArray( (UInt)m_iNumberOfBaseViews, (UInt)m_iNumberOfBaseViews,  m_aaiScaleAndOffsetSet    );
  xInit2dArray  ( (UInt)m_iNumberOfBaseViews, (UInt)m_iNumberOfBaseViews,  m_aaiScaleAndOffsetSet, 0 );

  xCreate2dArray( (UInt)m_iNumberOfBaseViews, (UInt)m_iNumberOfBaseViews,  m_aaiPdmScaleNomDelta     );
  xCreate2dArray( (UInt)m_iNumberOfBaseViews, (UInt)m_iNumberOfBaseViews,  m_aaiPdmOffset            );

  //===== init disparity to virtual depth conversion parameters =====
  xSetPdmConversionParams();

  //===== init arrays for first frame =====
  xSetShiftParametersAndLUT( m_uiFirstFrameId );
}


Void
TAppComCamPara::check( Bool bCheckViewRange, Bool bCheckFrameRange )
{
  if( bCheckFrameRange )
  {
    Double dDummy;

    for( UInt uiBaseView = 0; uiBaseView < m_aiBaseViews.size(); uiBaseView++ )
    {
      if ( m_bSetupFromCoded )
      {
        for( UInt uiTargetView = 0; uiTargetView < m_aiBaseViews.size(); uiTargetView++ )
        {
          if ( uiTargetView == uiBaseView )
            continue;

          for( UInt uiFrame = m_uiFirstFrameId; uiFrame <= m_uiLastFrameId; uiFrame++ )
          {
            Int iDummy;

            xGetCodedCameraData( uiBaseView, uiTargetView, true , uiFrame, iDummy, iDummy, iDummy );
          }
        }
      }
      else
      {
      for( UInt uiFrame = m_uiFirstFrameId; uiFrame <= m_uiLastFrameId; uiFrame++ )
      {
        Bool bInterpolatedCur;
        xGetGeometryData( m_aiBaseViews[ uiBaseView ], uiFrame, dDummy, dDummy, dDummy, bInterpolatedCur );
        xGetZNearZFar   ( m_aiBaseViews[ uiBaseView ], uiFrame, dDummy, dDummy );

        if( bInterpolatedCur )
        {
          std::cerr << "Error: CameraParameters for BaseView " << (Double)m_aiBaseViews[ uiBaseView ] / m_dViewNumPrec << " and Frame " << uiFrame << " not defined. "  << std::endl;
          exit( EXIT_FAILURE );
        }
      }

      }
    }

    for( UInt uiERView = 0; uiERView < m_aiSynthViews.size() && !m_bSetupFromCoded; uiERView++ )
    {
      Bool bInterpolated = false;
      for( UInt uiFrame = m_uiFirstFrameId; uiFrame <= m_uiLastFrameId; uiFrame++ )
      {
        Bool bInterpolatedCur;
        xGetGeometryData( m_aiSynthViews[ uiERView ], uiFrame, dDummy, dDummy, dDummy, bInterpolatedCur );
        bInterpolated |= bInterpolatedCur;
      }
      if( bInterpolated )
      {
        std::cout << "Interpolating Camera Parameters for View " << (Double)m_aiSynthViews[ uiERView ] / m_dViewNumPrec << std::endl;
      }
    }
  }

  if( bCheckViewRange )
  {
    Bool bAllExist = true;
    for( Int iSynthViewIdx = 0; iSynthViewIdx < m_iNumberOfSynthViews; iSynthViewIdx++ )
    {
      Bool bIsBaseView;
      Int  iDummy;
      Bool bExist = getLeftRightBaseView( iSynthViewIdx, iDummy, iDummy, iDummy, bIsBaseView );
      bAllExist  &= ( bExist || bIsBaseView );
    }
    if( !bAllExist )
    {
      std::cerr << "SynthViewNumbers must be within the range of BaseViewNumbers"  << std::endl;
      exit( EXIT_FAILURE );
    }
  }
}


Void
TAppComCamPara::update( UInt uiFrameId )
{

  m_iCurrentFrameId = uiFrameId;
  m_bCamParsCodedPrecSet = false;

  AOF( 0 <= uiFrameId && uiFrameId <= m_uiLastFrameId - m_uiFirstFrameId );
  if ( m_bCamParsVaryOverTime )
  {
    xSetShiftParametersAndLUT( m_uiFirstFrameId + uiFrameId );
  }
}


Bool
TAppComCamPara::getLeftRightBaseView( Int iSynthViewIdx, Int &riLeftViewIdx, Int &riRightViewIdx, Int &riRelDistToLeft, Bool& rbIsBaseView )
{
  Int    iLeftSortedViewIdx, iRightSortedViewIdx, iDummy;
  Bool   bExist  = xGetLeftRightView( m_aiSynthViews[ iSynthViewIdx ], m_aiSortedBaseViews, iDummy, iDummy, iLeftSortedViewIdx, iRightSortedViewIdx );
  rbIsBaseView   = ( iLeftSortedViewIdx == iRightSortedViewIdx && iLeftSortedViewIdx != -1 );

  Int iLeftViewIdx  = ( iLeftSortedViewIdx  != -1 ? m_aiBaseSortedId2Id[ iLeftSortedViewIdx  ] : -1 );
  Int iRightViewIdx = ( iRightSortedViewIdx != -1 ? m_aiBaseSortedId2Id[ iRightSortedViewIdx ] : -1 );

  if ( iLeftSortedViewIdx != -1 && iRightSortedViewIdx != -1 )
  {
    riRelDistToLeft = getRelDistLeft(  iSynthViewIdx, iLeftViewIdx, iRightViewIdx);
  }
  else
  {
    riRelDistToLeft = -1;
  }

  riLeftViewIdx  = iLeftViewIdx;
  riRightViewIdx = iRightViewIdx;

  return bExist;
}

Int TAppComCamPara::getRelDistLeft( Int iSynthViewIdx, Int iLeftViewIdx, Int iRightViewIdx )
{
  //GT: Get normalized distance
  Int iLeftViewDist  = abs ( m_aiBaseId2SortedId[ iLeftViewIdx  ] * ((Int) m_dViewNumPrec) - m_aiRelSynthViewsNum [ iSynthViewIdx ]);
  Int iRightViewDist = abs ( m_aiBaseId2SortedId[ iRightViewIdx ] * ((Int) m_dViewNumPrec) - m_aiRelSynthViewsNum [ iSynthViewIdx ]);
  Int64 iDistSum = iLeftViewDist + iRightViewDist;
  return (iDistSum == 0) ? (1 << (REN_VDWEIGHT_PREC -1) ) : (Int) (( (((Int64) iLeftViewDist ) << REN_VDWEIGHT_PREC ) + (iDistSum >> 1) )  / iDistSum );
}

Int
TAppComCamPara::synthRelNum2Idx( Int iRelNum )
{
  return xGetViewId(m_aiRelSynthViewsNum, iRelNum );
}
