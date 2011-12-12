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



#ifndef __TCOMMVDREFDATA__
#define __TCOMMVDREFDATA__


// Include files

#include "CommonDef.h"
#include "TComPicYuv.h"
#include <vector>
#if HHI_VSO
// ====================================================================================================================
// Class definition
// ====================================================================================================================


class TComMVDRefData
{
private:
  // PicYuvs
  TComPicYuv* m_apcVideoOrg[3];
  TComPicYuv* m_apcDepthOrg[3];
  TComPicYuv* m_apcVideoRec[3];
  TComPicYuv* m_apcDepthRec[3];

  // LUTs
  Double**    m_adBaseViewShiftLUT[3];
  Int**       m_aiBaseViewShiftLUT[3];

  //PicYuvs
  std::vector<TComPicYuv*> m_apcExternalReferenceViews;

  // LUTs
  Double***   m_adERViewShiftLUT;
  Int***      m_aiERViewShiftLUT;

  // Indices
  std::vector<Int> m_aiBaseViewRefInd;
  std::vector<Int> m_aiExtViewRefInd;
  std::vector<Int> m_aiExtViewRefLUTInd;

public:
  TComMVDRefData();

  //  PicYuvs
  TComPicYuv* getPicYuvRecVideo   ( InterViewReference eIVRef )  { return m_apcVideoRec[eIVRef]; };
  TComPicYuv* getPicYuvOrgVideo   ( InterViewReference eIVRef )  { return m_apcVideoOrg[eIVRef]; };
  TComPicYuv* getPicYuvRecDepth   ( InterViewReference eIVRef )  { return m_apcDepthRec[eIVRef]; };
  TComPicYuv* getPicYuvOrgDepth   ( InterViewReference eIVRef )  { return m_apcDepthOrg[eIVRef]; };
  Void        setPicYuvBaseView   ( InterViewReference eView, Bool bDepth, TComPicYuv* pcOrgView, TComPicYuv* pcRecView );


  TComPicYuv* getPicYuvVideo      ( ) { return getPicYuvOrgVideo( CURRVIEW ); /*return getRecVideo( CURRVIEW ) ?  getRecVideo( CURRVIEW ) : getOrgVideo( CURRVIEW ); */ };
  //TComPicYuv* getVideoData( ) { return getRecVideo( CURRVIEW ) ?  getRecVideo( CURRVIEW ) : getOrgVideo( CURRVIEW );  };

  // Luts
  Int**       getShiftLUTLBaseView       ( InterViewReference eIVRef ) { return m_aiBaseViewShiftLUT[eIVRef]; };
  Double**    getShiftLUTDBaseView       ( InterViewReference eIVRef ) { return m_adBaseViewShiftLUT[eIVRef]; };
  Void        setShiftLUTsBaseView       ( InterViewReference eView, Double** adInLut, Int** alInLut );

//  Void render( TComPicYuv* pOut ) /*Void render( TComPicYuv* pIn, TComPicYuv* pOut, TComPicYuv* pDepth, Long** aalLUT ) */;


  //  PicYuvs
  TComPicYuv* getPicYuvERView   ( UInt uiReferenceNum )              { return m_apcExternalReferenceViews[uiReferenceNum]; };
  Void        setPicYuvERViews  ( std::vector<TComPicYuv*> papcIn  ) { m_apcExternalReferenceViews = papcIn; };

  //  LUTS
  Int**       getShiftLUTIERView         ( UInt uiTargetViewNum )      { return (m_aiERViewShiftLUT)[uiTargetViewNum]; };
  Double**    getShiftLUTDERView         ( UInt uiTargetViewNum )      { return (m_adERViewShiftLUT)[uiTargetViewNum]; };

  Void        setShiftLUTsERView    ( Double*** adInLut, Int***   aiInLut) { m_aiERViewShiftLUT = aiInLut; m_adERViewShiftLUT = adInLut; };

  // Set Indices of actually used references
  Void setRefViewInd  ( std::vector<Int> aiBaseViewRef, std::vector<Int> aiEViewRef, std::vector<Int> aiEViewRefLut  ) { m_aiBaseViewRefInd = aiBaseViewRef, m_aiExtViewRefInd = aiEViewRef, m_aiExtViewRefLUTInd = aiEViewRefLut; }

  // Get Reference Data
  UInt getNumOfRefViews() { return (UInt) m_aiExtViewRefInd.size() + (UInt) m_aiBaseViewRefInd.size(); };
  Void getRefPicYuvAndLUT     ( TComPicYuv**& rpacDistRefPicList, Int***& rppalShiftLut);
  Void getRefPicYuvAndLUTMode1( TComPicYuv**& rpacDistRefPicList, Int***& rppaiShiftLut );

}; // END CLASS DEFINITION TComMVDRefData
#endif // HHI_VSO
#endif // __TCOMMVDREFDATA__



