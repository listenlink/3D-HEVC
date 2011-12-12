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


#include "TEncFormattedStringParser.h"
#include <sstream>
#include <algorithm>

const std::string TEncFormattedStringParser::sm_cSetOfTypes      ("AIPBNipbn");
const std::string TEncFormattedStringParser::sm_cSetOfDigits     ("0123456789");
const std::string TEncFormattedStringParser::sm_cSetOfPartStart  ("AIPBNipbn*");

ErrVal TEncFormattedStringParser::extractFrameType( const std::string &rcString,
                                                SliceType& reSliceType,
                                                bool &rbStoreForRef,
                                                bool &rbIsIDR,
                                                std::string::size_type &ruiStartPos )
{
  ROT( rcString.length() <= ruiStartPos );

  rbIsIDR = rcString[ruiStartPos] == 'A';
  rbStoreForRef = isupper( rcString[ruiStartPos] ) != 0;

  const char cType = rcString[ruiStartPos++];
  switch( toupper( cType ) )
  {
  case 'A':
  case 'I':
    reSliceType = I_SLICE ;
    break;
  case 'P':
    reSliceType = P_SLICE ;
    break;
  case 'B':
    reSliceType = B_SLICE ;
    break;
  default:
    ROT( 1 );
  }

  return Err::m_nOK;
}




ErrVal TEncFormattedStringParser::extractFrameIncrement( const std::string &rcString,
                                                     UInt &ruiIncrement,
                                                     std::string::size_type &ruiStartPos )
{
  RNOK( xExtractUInt( rcString, ruiIncrement, ruiStartPos ) );
  return Err::m_nOK;
}

ErrVal TEncFormattedStringParser::extractFrameLayer( const std::string &rcString,
                                                 UInt &ruiLayer,
                                                 std::string::size_type &ruiStartPos )
{
  ruiLayer = 0;
  ROTRS( std::string::npos == ruiStartPos, Err::m_nOK );
  ROTRS( rcString.length() <= ruiStartPos, Err::m_nOK );
  ROTRS( rcString[ruiStartPos] != 'L', Err::m_nOK );
  ruiStartPos++;
  RNOK( xExtractUInt( rcString, ruiLayer, ruiStartPos ) );
  return Err::m_nOK;
}

ErrVal TEncFormattedStringParser::xExtractRelRefPocAndRefViewIdx( const std::string &rcString,
                                                                  std::vector<int> &raiAllowedRelativeRefPocs,
                                                                  std::vector<int> &raiAllowedReferenceViewIdx,
                                                                  std::string::size_type &ruiStartPos )
{
  if( rcString[ruiStartPos] == 'v' || rcString[ruiStartPos] == 'V' )
  {
    ruiStartPos++;
    int iRefViewIdx = 0;
    RNOK( xExtractInt( rcString, iRefViewIdx, ruiStartPos ) );
    assert( iRefViewIdx >= 0 );
    ROF( raiAllowedReferenceViewIdx.end() == std::find( raiAllowedReferenceViewIdx.begin(), raiAllowedReferenceViewIdx.end(), iRefViewIdx ) );
    raiAllowedReferenceViewIdx.push_back( iRefViewIdx );
    raiAllowedRelativeRefPocs.push_back( 0 );
  }
  else
  {
    int iRelRefPoc = 0;
    RNOK( xExtractInt( rcString, iRelRefPoc, ruiStartPos ) );
    ROF( raiAllowedRelativeRefPocs.end() == std::find( raiAllowedRelativeRefPocs.begin(), raiAllowedRelativeRefPocs.end(), iRelRefPoc ) );
    raiAllowedRelativeRefPocs.push_back( iRelRefPoc );
    raiAllowedReferenceViewIdx.push_back( -1 );
  }
  return Err::m_nOK;
}

ErrVal TEncFormattedStringParser::extractAllowedRelativeRefPocs( const std::string &rcString,
                                                             std::vector<int> &raiAllowedRelativeRefPocs,
                                                             std::vector<int> &raiAllowedReferenceViewIdx,
                                                             std::string::size_type &ruiStartPos )
{
  ROF( raiAllowedRelativeRefPocs.empty() );
  ROTRS( std::string::npos == ruiStartPos, Err::m_nOK );
  ROTRS( rcString.length() <= ruiStartPos, Err::m_nOK );
  RNOK( xEatChar( rcString, '(', ruiStartPos ) );

  do
  {
    RNOK( xExtractRelRefPocAndRefViewIdx( rcString, raiAllowedRelativeRefPocs, raiAllowedReferenceViewIdx, ruiStartPos ) );
  }
  while( xEatChar( rcString, ',', ruiStartPos ) == Err::m_nOK );
  RNOK( xEatChar( rcString, ')', ruiStartPos ) );

  return Err::m_nOK;
}

ErrVal TEncFormattedStringParser::extractAllowedRelativeRefPocs( const std::string &rcString,
                                                             std::vector<int> &raiAllowedRelativeRefPocsL0,
                                                             std::vector<int> &raiAllowedRelativeRefPocsL1,
                                                             std::vector<int> &raiAllowedReferenceViewIdxL0,
                                                             std::vector<int> &raiAllowedReferenceViewIdxL1,
                                                             std::string::size_type &ruiStartPos )
{
  ROF( raiAllowedRelativeRefPocsL0.empty() );
  ROF( raiAllowedRelativeRefPocsL1.empty() );

  ROTRS( std::string::npos == ruiStartPos, Err::m_nOK );
  ROTRS( rcString.length() <= ruiStartPos, Err::m_nOK );

  RNOK( xEatChar( rcString, '(', ruiStartPos ) );
  do
  {
    RNOK( xExtractRelRefPocAndRefViewIdx( rcString, raiAllowedRelativeRefPocsL0, raiAllowedReferenceViewIdxL0, ruiStartPos ) );
  }
  while( xEatChar( rcString, ',', ruiStartPos ) == Err::m_nOK );
  RNOK( xEatChar( rcString, ';', ruiStartPos ) );
  do
  {
    RNOK( xExtractRelRefPocAndRefViewIdx( rcString, raiAllowedRelativeRefPocsL1, raiAllowedReferenceViewIdxL1, ruiStartPos ) );
  }
  while( xEatChar( rcString, ',', ruiStartPos ) == Err::m_nOK );
  RNOK( xEatChar( rcString, ')', ruiStartPos ) );
  return Err::m_nOK;
}






ErrVal TEncFormattedStringParser::xEatChar( const std::string &rcString,
                                        char cChar,
                                        std::string::size_type &ruiStartPos )
{
  ROTS( std::string::npos == ruiStartPos );
  ROTS( rcString.length() <= ruiStartPos );
  ROFS( rcString[ruiStartPos] == cChar );
  ruiStartPos++;
  return Err::m_nOK;
}

ErrVal TEncFormattedStringParser::xExtractUInt( const std::string &rcString,
                                            UInt &ruiVal,
                                            std::string::size_type &ruiStartPos )
{
  ROT( std::string::npos == ruiStartPos );
  ROT( rcString.length() <= ruiStartPos );
  const std::string::size_type uiPos = rcString.find_first_not_of( sm_cSetOfDigits, ruiStartPos + ( rcString[ruiStartPos] == '+' ? 1 : 0 ) );
  ROT( ruiStartPos == uiPos );
  std::stringstream( rcString.substr( ruiStartPos, std::string::npos == uiPos ? std::string::npos : uiPos - ruiStartPos ) ) >> ruiVal;
  ruiStartPos = uiPos;
  return Err::m_nOK;
}

ErrVal TEncFormattedStringParser::xExtractInt( const std::string &rcString,
                                           int &riVal,
                                           std::string::size_type &ruiStartPos )
{
  ROT( std::string::npos == ruiStartPos );
  ROT( rcString.length() <= ruiStartPos );
  const std::string::size_type uiPos = rcString.find_first_not_of( sm_cSetOfDigits, ruiStartPos + ( rcString[ruiStartPos] == '+' || rcString[ruiStartPos] == '-' ? 1 : 0 ) );
  ROT( ruiStartPos == uiPos );
  std::stringstream( rcString.substr( ruiStartPos, std::string::npos == uiPos ? std::string::npos : uiPos - ruiStartPos ) ) >> riVal;
  ruiStartPos = uiPos;
  return Err::m_nOK;
}


ErrVal
TEncFormattedStringParser::separatString   ( const std::string&  rcString,
                                         std::string& rcFDString,
                                         std::string& rcMmcoString,
                                         std::string& rcRplrStringL0,
                                         std::string& rcRplrStringL1)
{
  size_t uiMPos  = rcString.find_first_of( "M" );
  size_t uiR1Pos = rcString.find_first_of( "R" );
  size_t uiR2Pos = rcString.find_last_of ( "R" );

  size_t uiSize = rcString.size();

  if( std::string::npos == uiMPos )
  {
    rcMmcoString = "";
    if( std::string::npos == uiR1Pos )
    {
      rcFDString    = rcString;
      rcRplrStringL0 = "";
      rcRplrStringL1 = "";
    }
    else
    {
      rcFDString      = rcString.substr( 0,     uiR1Pos );
      if( uiR1Pos == uiR2Pos )
      {
        rcRplrStringL0 = rcString.substr( uiR1Pos, uiSize - uiR1Pos );
        rcRplrStringL1 = "";
      }
      else
      {
        rcRplrStringL0 = rcString.substr( uiR1Pos, uiR2Pos - uiR1Pos );
        rcRplrStringL1 = rcString.substr( uiR2Pos, uiSize  - uiR2Pos );
      }
    }
  }
  else
  {
    if( std::string::npos == uiR1Pos )
    {
      rcFDString      = rcString.substr( 0,     uiMPos );
      rcMmcoString    = rcString.substr( uiMPos, uiSize - uiMPos );
      rcRplrStringL0 = "";
      rcRplrStringL1 = "";
    }
    else
    {
      if( uiMPos < uiR1Pos )
      {
        rcFDString      = rcString.substr( 0,      uiMPos );
        rcMmcoString    = rcString.substr( uiMPos,  uiR1Pos - uiMPos );
        if( uiR1Pos == uiR2Pos )
        {
          rcRplrStringL0 = rcString.substr( uiR1Pos, uiSize - uiR1Pos );
          rcRplrStringL1 = "";
        }
        else
        {
          rcRplrStringL0 = rcString.substr( uiR1Pos, uiR2Pos - uiR1Pos );
          rcRplrStringL1 = rcString.substr( uiR2Pos, uiSize  - uiR2Pos );
        }
      }
      else
      {
        rcFDString      = rcString.substr( 0,      uiR1Pos );

        if( uiR1Pos == uiR2Pos )
        {
          rcRplrStringL0 = rcString.substr( uiR1Pos, uiMPos - uiR1Pos );
          rcRplrStringL1 = "";
          rcMmcoString    = rcString.substr( uiMPos,  uiSize - uiMPos );
        }
        else
        {
          if( uiMPos < uiR2Pos )
          {
            rcRplrStringL0 = rcString.substr( uiR1Pos, uiMPos  - uiR1Pos );
            rcMmcoString    = rcString.substr( uiMPos, uiR2Pos - uiMPos );
            rcRplrStringL1 = rcString.substr( uiR2Pos, uiSize  - uiR2Pos );

          }
          else
          {
            rcRplrStringL0 = rcString.substr( uiR1Pos, uiR2Pos - uiR1Pos );
            rcRplrStringL1 = rcString.substr( uiR2Pos, uiMPos  - uiR2Pos );
            rcMmcoString    = rcString.substr( uiMPos,  uiSize  - uiMPos );
          }
        }
      }
    }
  }

  if( rcRplrStringL0.size() == 1 )
  {
    rcRplrStringL0 = "";
  }

  if( rcRplrStringL1.size() == 1 )
  {
    rcRplrStringL1 = "";
  }

  return Err::m_nOK;
}

bool
TEncFormattedStringParser::isFrameSequencePart( const std::string& rcString )
{
  return ( rcString.find( '*', 1 ) == std::string::npos );
}


ErrVal
TEncFormattedStringParser::extractRepetitions( const std::string&  rcString,
                                           std::string&        rcNoRepString,
                                           UInt&               ruiNumberOfRepetitions )
{
  ROT( rcString.empty() );
  if( rcString[0] != '*' )
  {
    ruiNumberOfRepetitions = 1;
    rcNoRepString          = rcString;
  }
  else
  {
    size_t  uiLastPos   = rcString.length () - 1;
    size_t  uiOpenPos   = rcString.find   ( '{' );
    size_t  uiClosePos  = rcString.rfind  ( '}' );

    ROTS(  uiOpenPos   == std::string::npos  );
    ROFS(  uiClosePos  == uiLastPos          );

    rcNoRepString     = rcString.substr( uiOpenPos+1, uiClosePos-uiOpenPos-1 ); //GT: abc  if rcString = *n100{abc}
    std::string cNStr = rcString.substr( 1,           uiOpenPos - 1          ); //GT  n100 if rcString = *n100{abc}
    ROT( cNStr.empty() );

    if( uiOpenPos==2 && cNStr[0]=='n' )
    {
//      ruiNumberOfRepetitions = TypeLimits<UInt>::m_iMax; // reserved value representing "infinity"
      ruiNumberOfRepetitions = MAX_UINT; // reserved value representing "infinity"
    }
    else
    {
      ROFS( cNStr.find_first_not_of( sm_cSetOfDigits ) == std::string::npos );
      ruiNumberOfRepetitions  = atoi( cNStr.c_str() );
    }
  }

  return Err::m_nOK;
}

ErrVal
TEncFormattedStringParser::getNumberOfFrames( const std::string&  rcString,
                                          UInt&               ruiNumberOfFrames )
{
  size_t  uiPos           = rcString.find_first_of( sm_cSetOfTypes );
  size_t  uiUnderScorePos = 0 ;
  ruiNumberOfFrames = 0;


  while( uiPos != std::string::npos )
  {
    ruiNumberOfFrames++;
    uiUnderScorePos = rcString.find_first_of( std::string("_"), uiPos+1 ) ;
    ROT( uiUnderScorePos == std::string::npos ) ;
    uiPos = rcString.find_first_of( sm_cSetOfTypes, uiUnderScorePos+1 );
  }

  return Err::m_nOK;
}

ErrVal
TEncFormattedStringParser::extractNextFrameDescription( const std::string&  rcString,
                                                    std::string&        rcFDString,
                                                    size_t&        ruiStartPos )
{
  ROTS( ruiStartPos >= rcString.length()-1 );
  size_t uiUnderScorePos = rcString.find_first_of( std::string("_"), ruiStartPos+1 ) ;
  ROT( uiUnderScorePos == std::string::npos ) ;
  size_t uiEndPos = rcString.find_first_of( sm_cSetOfTypes, uiUnderScorePos+1 );

  rcFDString  = rcString.substr( ruiStartPos, uiEndPos - ruiStartPos );
  ruiStartPos = uiEndPos;

  return Err::m_nOK;
}

ErrVal
TEncFormattedStringParser::getNumberOfParts( const std::string& rcString,
                                         UInt&              ruiNumberOfParts )
{
  size_t  uiPos       = rcString.find_first_of( sm_cSetOfPartStart );
  ruiNumberOfParts  = 0;

  while( uiPos != std::string::npos )
  {
    ruiNumberOfParts++;

    if( rcString[uiPos] == '*' ) //GT: Exclude sub-parts
    {
      size_t  uiEndPos        = rcString.find( '{', uiPos+1 );
      UInt  uiOpenBrackets  = 1;
      ROTS( uiEndPos == std::string::npos );

      while( uiOpenBrackets )
      {
        uiEndPos  = rcString.find_first_of( "{}", uiEndPos+1 );
        ROTS( uiEndPos == std::string::npos );

        if( rcString[uiEndPos] == '{' )   uiOpenBrackets++;
        else                              uiOpenBrackets--;
      }

      uiPos = rcString.find_first_of( sm_cSetOfPartStart, uiEndPos + 1 );
    }
    else
    {
      uiPos = rcString.find( '*', uiPos+1 );
    }
  }

  return Err::m_nOK;
}

ErrVal
TEncFormattedStringParser::extractPart( const std::string&  rcString,
                                    std::string&        rcPartString,
                                    size_t&        ruiStartPos )
{
  ROTS( ruiStartPos >= rcString.length()-1 );

  size_t uiNextStartPos;

  if( rcString[ruiStartPos] == '*' )
  {
    size_t  uiEndPos        = rcString.find( '{', ruiStartPos+1 );
    UInt  uiOpenBrackets  = 1;
    ROTS( uiEndPos == std::string::npos );

    while( uiOpenBrackets )
    {
      uiEndPos  = rcString.find_first_of( "{}", uiEndPos+1 );
      ROTS( uiEndPos == std::string::npos );

      if( rcString[uiEndPos] == '{' )   uiOpenBrackets++;
      else                              uiOpenBrackets--;
    }

    uiNextStartPos = rcString.find_first_of( sm_cSetOfPartStart, uiEndPos + 1 );
  }
  else
  {
    uiNextStartPos = rcString.find( '*', ruiStartPos+1 );
  }

  rcPartString  = rcString.substr( ruiStartPos, uiNextStartPos - ruiStartPos );
  ruiStartPos   = uiNextStartPos;

  return Err::m_nOK;
}

ErrVal TEncFormattedStringParser::extractHighestLayerOfGOPString( const std::string &rcString, UInt &ruiLayer )
{
  std::string::size_type uiPos = 0;
  ruiLayer = 0;

  while( ( uiPos = rcString.find_first_of( 'L', uiPos ) ) != std::string::npos )
  {
    uiPos++;

    UInt ui = 0;
    RNOK( xExtractUInt( rcString, ui, uiPos ) );
    if( ui > ruiLayer )
      ruiLayer = ui;
  }
  return Err::m_nOK;
}


