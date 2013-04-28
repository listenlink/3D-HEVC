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

#if !defined(AFX_TEncFormattedStringParser_H__268768B8_4D1D_484A_904E_586985833BAC__INCLUDED_)
#define AFX_TEncFormattedStringParser_H__268768B8_4D1D_484A_904E_586985833BAC__INCLUDED_

#include "../TLibCommon/CommonDef.h"
#include <vector>

class TEncFormattedStringParser
{
public:
  static  ErrVal  separatString               ( const std::string&  rcString,
                                                      std::string& rcFDString,
                                                      std::string& rcMmcoString,
                                                      std::string& rcRplrStringL0,
                                                      std::string& rcRplrStringL1 );
  static bool   isFrameSequencePart         ( const std::string&  rcString                  );

  static ErrVal extractRepetitions          ( const std::string&  rcString,
                                              std::string&        rcNoRepString,
                                              UInt&               ruiNumberOfRepetitions    );

  static ErrVal getNumberOfFrames           ( const std::string&  rcString,
                                              UInt&               ruiNumberOfFrames         );

  static ErrVal extractNextFrameDescription ( const std::string&  rcString,
                                              std::string&        rcFDString,
                                              size_t&        ruiStartPos );

  static ErrVal getNumberOfParts            ( const std::string&  rcString,
                                              UInt&               ruiNumberOfParts          );

  static ErrVal extractPart                 ( const std::string&  rcString,
                                              std::string&        rcPartString,
                                              size_t&        ruiStartPos );

  static ErrVal extractFrameType( const std::string &rcString,
                                  SliceType& reSliceType,
                                  bool &rbStoreForRef,
                                  bool &rbIsIDR,
                                  std::string::size_type &ruiStartPos );
  static ErrVal extractFrameIncrement( const std::string &rcString,
                                       UInt &ruiIncrement,
                                       std::string::size_type &ruiStartPos );

  static ErrVal extractFrameLayer( const std::string &rcString,
                                   UInt &ruiLayer,
                                   std::string::size_type &ruiStartPos );

  static ErrVal extractAllowedRelativeRefPocs( const std::string &rcString,
                                               std::vector<int> &raiAllowedRelativeRefPocs,
                                               std::vector<int> &raiAllowedReferenceViewIdx,
                                               std::string::size_type &ruiStartPos );
  static ErrVal extractAllowedRelativeRefPocs( const std::string &rcString,
                                                 std::vector<int> &raiAllowedRelativeRefPocsL0,
                                                 std::vector<int> &raiAllowedRelativeRefPocsL1,
                                                 std::vector<int> &raiAllowedReferenceViewIdxL0,
                                                 std::vector<int> &raiAllowedReferenceViewIdxL1,
                                                 std::string::size_type &ruiStartPos );


  static ErrVal extractHighestLayerOfGOPString( const std::string &rcString, UInt &ruiLayer );

private:
  static ErrVal xExtractUInt( const std::string &rcString,
                              UInt &ruiVal,
                              std::string::size_type &ruiStartPos );

  static ErrVal xExtractInt( const std::string &rcString,
                             int &riVal,
                             std::string::size_type &ruiStartPos );

  static ErrVal xEatChar( const std::string &rcString,
                          char cChar,
                          std::string::size_type &ruiStartPos );

  static ErrVal xExtractRelRefPocAndRefViewIdx( const std::string &rcString,
                                                std::vector<int> &raiAllowedRelativeRefPocs,
                                                std::vector<int> &raiAllowedReferenceViewIdx,
                                                std::string::size_type &ruiStartPos );

  static const std::string sm_cSetOfTypes;
  static const std::string sm_cSetOfDigits;
  static const std::string sm_cSetOfPartStart;
};




#endif // !defined(AFX_TEncFormattedStringParser_H__268768B8_4D1D_484A_904E_586985833BAC__INCLUDED_)
