

#ifndef __TCOMMVDREFDATA__
#define __TCOMMVDREFDATA__

// Include files
#include "CommonDef.h"
#include "TComPicYuv.h"
#include <vector>

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


//GT VSO
  //PicYuvs
  std::vector<TComPicYuv*> m_apcExternalReferenceViews;

  // LUTs
  Double***   m_adERViewShiftLUT;
  Int***      m_aiERViewShiftLUT;

  // Indices
  std::vector<Int> m_aiBaseViewRefInd;
  std::vector<Int> m_aiExtViewRefInd;
  std::vector<Int> m_aiExtViewRefLUTInd;

//GT VSO end

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

//GT VSO
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
//GT VSO end

}; // END CLASS DEFINITION TComMVDRefData

#endif // __TCOMMVDREFDATA__



