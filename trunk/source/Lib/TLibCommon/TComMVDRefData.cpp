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
#include "TComMVDRefData.h"
#include "TComPicYuv.h"
#include <math.h>
#include <vector>
#include <iostream>
#include <fstream>

#if HHI_VSO
Void TComMVDRefData::setPicYuvBaseView( InterViewReference eView, Bool bDepth, TComPicYuv* pcOrgView, TComPicYuv* pcRecView )
{
  if ( bDepth )
  {
    m_apcDepthRec[eView] = pcRecView;
    m_apcDepthOrg[eView] = pcOrgView;
  }
  else
  {
    m_apcVideoRec[eView] = pcRecView;
    m_apcVideoOrg[eView] = pcOrgView;
  }
}

Void TComMVDRefData::setShiftLUTsBaseView( InterViewReference eView, Double** adInLut, Int** aiInLut )
{
  m_aiBaseViewShiftLUT[eView] = aiInLut;
  m_adBaseViewShiftLUT[eView] = adInLut;
}

TComMVDRefData::TComMVDRefData()
{
  m_adERViewShiftLUT = 0;
  m_aiERViewShiftLUT = 0;
  m_apcExternalReferenceViews.resize(0);

  for (UInt uiView = 0; uiView < 3; uiView++)
  {
    m_apcVideoOrg[uiView] = NULL;
    m_apcDepthOrg[uiView] = NULL;
    m_apcVideoRec[uiView] = NULL;
    m_apcDepthRec[uiView] = NULL;

    m_adBaseViewShiftLUT [uiView] = NULL;
    m_aiBaseViewShiftLUT [uiView] = NULL;

  };
}

Void TComMVDRefData::getRefPicYuvAndLUT( TComPicYuv**& rpacDistRefPicList, Int***& rppaiShiftLut )
{
  UInt uiNextEntry = 0;
  for ( UInt uiViewIdx = 0; uiViewIdx < m_aiExtViewRefInd.size(); uiViewIdx++ )
  {
    rppaiShiftLut[uiNextEntry]       = m_aiERViewShiftLUT         [ m_aiExtViewRefLUTInd[uiViewIdx] ];
    rpacDistRefPicList[uiNextEntry]  = m_apcExternalReferenceViews[ m_aiExtViewRefInd   [uiViewIdx] ];
    uiNextEntry++;
  }

  for ( UInt uiViewIdx = 0; uiViewIdx < m_aiBaseViewRefInd.size(); uiViewIdx++ )
  {
    rppaiShiftLut[uiNextEntry]      = m_aiBaseViewShiftLUT[ m_aiBaseViewRefInd[uiViewIdx] + 1 ];
    rpacDistRefPicList[uiNextEntry] = getPicYuvRecVideo( (InterViewReference) (m_aiBaseViewRefInd[uiViewIdx] + 1) ) ?  getPicYuvRecVideo( (InterViewReference) (m_aiBaseViewRefInd[uiViewIdx] + 1) ) : getPicYuvOrgVideo( (InterViewReference) (m_aiBaseViewRefInd[uiViewIdx] + 1) );
    uiNextEntry++;
  }
}

Void TComMVDRefData::getRefPicYuvAndLUTMode1( TComPicYuv**& rpacDistRefPicList, Int***& rppaiShiftLut )
{
  rppaiShiftLut[0] = m_aiBaseViewShiftLUT[ PREVVIEW ];
  rppaiShiftLut[1] = NULL;
  rppaiShiftLut[2] = m_aiBaseViewShiftLUT[ NEXTVIEW ];

  rpacDistRefPicList[0] = NULL;
  rpacDistRefPicList[1] = NULL;
  rpacDistRefPicList[2] = NULL;
}
#endif
