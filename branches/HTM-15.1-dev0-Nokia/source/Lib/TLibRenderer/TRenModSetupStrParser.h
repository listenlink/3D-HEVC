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

#ifndef __TRENMODSETUPSTRPARSER__
#define __TRENMODSETUPSTRPARSER__

#include "../TLibCommon/CommonDef.h"
#include "../TLibCommon/TComPicYuv.h"
#include "../TLibCommon/TypeDef.h"
#include "../TAppCommon/TAppComCamPara.h"
#if NH_3D



#include <list>
#include <vector>
#include <math.h>
#include <errno.h>
#include <vector>
#include <iostream>
#include <algorithm>
#include <functional>
#include <string>
#include <cstdio>
#include <cstring>


using namespace std;

class TRenModSetupStrParser
{
public:

  Int  getNumOfModels          ();
  Int  getNumOfBaseViews       ();

  Int  getNumOfModelsForView   ( Int iViewIdx, Int iContent );
  Int  getNumOfBaseViewsForView( Int iViewIdx, Int iContent );

  Void getSingleModelData      ( Int  iSrcViewIdx,
                                 Int  iSrcCnt,
                                 Int  iCurModel,
                                 Int& riModelNum,
                                 Int& riInterpolationType,
                                 Int& riLeftBaseViewIdx,
                                 Int& riRightBaseViewIdx,
                                 Int& riOrgRefBaseViewIdx,
                                 Int& riSynthViewRelNum
                               );

  Void getBaseViewData         ( Int   iSourceViewIdx,
                                 Int   iSourceContent,
                                 Int   iCurView,
                                 Int&  riBaseViewSIdx,
                                 Int&  riVideoDistMode,
                                 Int&  riDepthDistMode
                                );

  IntAry1d*  getSynthViews() { return &m_aiAllSynthViewNums;  }
  IntAry1d*  getBaseViews()  { return &m_aiAllBaseViewIdx;    }

  TRenModSetupStrParser();

  Void setString( Int iNumOfBaseViews, Char* pchSetStr );

private:
  IntAry2d                         m_aaaiBaseViewsIdx  [2];
  IntAry2d                         m_aaaiVideoDistMode [2];
  IntAry2d                         m_aaaiDepthDistMode [2];
  IntAry2d                         m_aaaiModelNums     [2];
  IntAry2d                         m_aaaiSynthViewNums [2];
  BoolAry2d                        m_aaabOrgRef        [2];
  BoolAry2d                        m_aaabExtrapolate   [2];
  IntAry2d                         m_aaaiBlendMode     [2];

  IntAry1d                         m_aiAllBaseViewIdx;
  IntAry1d                         m_aiAllSynthViewNums;

  Bool                             m_bCurrentViewSet;
  Int                              m_iCurrentView;
  Int                              m_iCurrentContent;
  Int                              m_iNumberOfModels;

  Char*                            m_pchSetStr;
  size_t                           m_iPosInStr;

private:
  Void xParseString();
  Void xParseSourceView();
  Void xReadViews         ( Char cType );
  Void xReadViewInfo      ( Char cType );
  Void xAddBaseView       ( Int iViewIdx, Char cVideoType, Char cDepthType );
  Void xAddSynthView      ( Int iViewNum, Char cType, Char cRefType );
  Void xError             ( Bool bIsError );
  Void xGetViewNumberRange( std::vector<Int>& raiViewNumbers );
  Void xGetNextCharGoOn   ( Char& rcNextChar );
  Void xGetNextChar       ( Char& rcNextChar );
};

#endif // NH_3D
#endif //__TRENMODEL__


