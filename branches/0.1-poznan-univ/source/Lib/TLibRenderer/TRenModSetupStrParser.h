


#ifndef __TRENMODSETUPSTRPARSER__
#define __TRENMODSETUPSTRPARSER__

#include "../TLibCommon/CommonDef.h"
#include "../TLibCommon/TComPicYuv.h"
#include "../TLibCommon/TypeDef.h"
#include "../../App/TAppCommon/TAppComCamPara.h"


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

  std::vector<Int>* getSynthViews() { return &m_aiAllSynthViewNums;  }
  std::vector<Int>* getBaseViews()  { return &m_aiAllBaseViewIdx;    }

  TRenModSetupStrParser();

  Void setString( Int iNumOfBaseViews, Char* pchSetStr );

private:
  std::vector< std::vector<Int > > m_aaaiBaseViewsIdx  [2];
  std::vector< std::vector<Int > > m_aaaiVideoDistMode [2];
  std::vector< std::vector<Int > > m_aaaiDepthDistMode [2];
  std::vector< std::vector<Int > > m_aaaiModelNums     [2];
  std::vector< std::vector<Int > > m_aaaiSynthViewNums [2];
  std::vector< std::vector<Bool> > m_aaabOrgRef        [2];
  std::vector< std::vector<Bool> > m_aaabExtrapolate   [2];
  std::vector< std::vector<Int > > m_aaaiBlendMode     [2];

  std::vector<Int>                 m_aiAllBaseViewIdx;
  std::vector<Int>                 m_aiAllSynthViewNums;

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
#endif //__TRENMODEL__
