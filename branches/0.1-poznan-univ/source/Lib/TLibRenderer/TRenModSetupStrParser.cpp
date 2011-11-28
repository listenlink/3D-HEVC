

#include "TRenImage.h"
#include "TRenFilter.h"
#include "TRenModSetupStrParser.h"

Int
TRenModSetupStrParser::getNumOfModels()
{
  return m_iNumberOfModels;
}

Int
TRenModSetupStrParser::getNumOfBaseViews()
{
  return (Int) m_aiAllBaseViewIdx  .size();
}

Int
TRenModSetupStrParser::getNumOfModelsForView( Int iViewIdx, Int iContent )
{
  return (Int) m_aaaiModelNums[iContent][iViewIdx].size();
}

Int
TRenModSetupStrParser::getNumOfBaseViewsForView( Int iViewIdx, Int iContent )
{
  return (Int) m_aaaiBaseViewsIdx[iContent][iViewIdx].size();
}

Void
TRenModSetupStrParser::getSingleModelData( Int iSrcViewIdx,
                                           Int iSrcCnt,
                                           Int iCurModel,
                                           Int& riModelNum,
                                           Int& riBlendMode,
                                           Int& riLeftBaseViewIdx,
                                           Int& riRightBaseViewIdx,
                                           Int& riOrgRefBaseViewIdx,
                                           Int& riSynthViewRelNum )
{
  Bool bExtrapolate    = m_aaabExtrapolate[iSrcCnt][iSrcViewIdx][iCurModel];
  Bool bOrgRef         = m_aaabOrgRef     [iSrcCnt][iSrcViewIdx][iCurModel];

  riOrgRefBaseViewIdx = bOrgRef ? m_aaaiSynthViewNums[iSrcCnt][iSrcViewIdx][iCurModel] / ( (Int) VIEW_NUM_PREC ) : -1;
  riSynthViewRelNum    = m_aaaiSynthViewNums[iSrcCnt][iSrcViewIdx][iCurModel];
  riModelNum           = m_aaaiModelNums    [iSrcCnt][iSrcViewIdx][iCurModel];
  riBlendMode          = m_aaaiBlendMode    [iSrcCnt][iSrcViewIdx][iCurModel];


  Int iSrcViewNum = iSrcViewIdx * ((Int) VIEW_NUM_PREC );
  if ( iSrcViewNum < riSynthViewRelNum )
  {
    riLeftBaseViewIdx  = iSrcViewIdx;
    riRightBaseViewIdx = -1;
  }
  else
  {
    riLeftBaseViewIdx = -1;
    riRightBaseViewIdx  = iSrcViewIdx;
  }

  if ( !bExtrapolate )
  {
    std::vector<Int> cCurBaseViews = m_aaaiBaseViewsIdx[iSrcCnt][iSrcViewIdx];

    Int iMinDist = MAX_INT;
    Int iNearestNum = -1;

    for (Int iCurBaseView = 0; iCurBaseView < cCurBaseViews.size(); iCurBaseView++ )
    {
      Int iCurBaseNum = m_aaaiBaseViewsIdx [iSrcCnt][iSrcViewIdx][iCurBaseView];

      if ( iCurBaseNum == iSrcViewNum )
        continue;

      Int iDist = iCurBaseNum - riSynthViewRelNum;

      if ( ( iDist <= 0  && riLeftBaseViewIdx == -1) || ( iDist >= 0  && riRightBaseViewIdx == -1 ) )
      {
        if ( abs(iDist) < iMinDist )
        {
          iMinDist = abs(iDist);
          iNearestNum = iCurBaseNum;
        }
      }
    }
    xError(iNearestNum == -1);

    if (riLeftBaseViewIdx == -1 )
    {
      riLeftBaseViewIdx = iNearestNum / (Int) (VIEW_NUM_PREC);
    }
    else
    {
      riRightBaseViewIdx = iNearestNum / (Int) (VIEW_NUM_PREC);
    }

    xError(riLeftBaseViewIdx  == -1 );
    xError(riRightBaseViewIdx == -1 );
    xError(riLeftBaseViewIdx  >= riRightBaseViewIdx );
  }
}

Void
TRenModSetupStrParser::getBaseViewData( Int iSourceViewIdx, Int iSourceContent, Int iCurView, Int& riBaseViewSIdx, Int& riVideoDistMode, Int& riDepthDistMode )
{
  riBaseViewSIdx = m_aaaiBaseViewsIdx  [iSourceContent][iSourceViewIdx][iCurView] / (Int) VIEW_NUM_PREC;
  riVideoDistMode            = m_aaaiVideoDistMode [iSourceContent][iSourceViewIdx][iCurView];
  riDepthDistMode            = m_aaaiDepthDistMode [iSourceContent][iSourceViewIdx][iCurView];
}

TRenModSetupStrParser::TRenModSetupStrParser()
{
  m_pchSetStr       = NULL;
  m_iPosInStr       = 0;
  m_iNumberOfModels = 0;
  m_bCurrentViewSet = false;
}

Void
TRenModSetupStrParser::setString( Int iNumOfBaseViews, Char* pchSetStr )
{
  for (Int iContent = 0; iContent < 2; iContent++)
  {
    m_aaaiBaseViewsIdx  [iContent].resize( iNumOfBaseViews );
    m_aaaiDepthDistMode [iContent].resize( iNumOfBaseViews );
    m_aaaiVideoDistMode [iContent].resize( iNumOfBaseViews );
    m_aaaiSynthViewNums [iContent].resize( iNumOfBaseViews );
    m_aaaiModelNums     [iContent].resize( iNumOfBaseViews );
    m_aaabOrgRef        [iContent].resize( iNumOfBaseViews );
    m_aaabExtrapolate   [iContent].resize( iNumOfBaseViews );
    m_aaaiBlendMode     [iContent].resize( iNumOfBaseViews );
  }

  AOT( m_pchSetStr );
  m_pchSetStr = pchSetStr;
  m_iPosInStr       = 0;
  m_bCurrentViewSet = false;

  xParseString();
}

Void
TRenModSetupStrParser::xParseString()
{
  Char cChar;
  xGetNextChar(cChar);
  while(  cChar != '\0' )
  {
    xParseSourceView();
    xGetNextChar(cChar);
  }
  // CHECK
  size_t iNumOfSrcViews = m_aaaiBaseViewsIdx[0].size();

  for (Int iSrcView = 0; iSrcView < iNumOfSrcViews; iSrcView++)
  {
    for (Int iContent = 0; iContent < 2; iContent++ )
    {
      size_t iNumOfBase  = m_aaaiBaseViewsIdx  [iContent][iSrcView].size();
      AOF( iNumOfBase   == m_aaaiDepthDistMode [iContent][iSrcView].size());
      AOF( iNumOfBase   == m_aaaiVideoDistMode [iContent][iSrcView].size());

      size_t iNumOfModels = m_aaaiSynthViewNums[iContent][iSrcView].size();
      AOF( iNumOfModels == m_aaaiModelNums     [iContent][iSrcView].size());
      AOF( iNumOfModels == m_aaabOrgRef        [iContent][iSrcView].size());
      AOF( iNumOfModels == m_aaabExtrapolate   [iContent][iSrcView].size());
    }
  }

  // SORT
  std::vector<Int>::iterator cIterNewEnd;

  std::sort( m_aiAllBaseViewIdx.begin(), m_aiAllBaseViewIdx.end() );
  cIterNewEnd = std::unique( m_aiAllBaseViewIdx.begin(), m_aiAllBaseViewIdx.end() );
  m_aiAllBaseViewIdx.erase( cIterNewEnd, m_aiAllBaseViewIdx.end() );

  std::sort( m_aiAllSynthViewNums.begin(), m_aiAllSynthViewNums.end() );
  cIterNewEnd = std::unique( m_aiAllSynthViewNums.begin(), m_aiAllSynthViewNums.end() );
  m_aiAllSynthViewNums.erase( cIterNewEnd, m_aiAllSynthViewNums.end() );
}

Void
TRenModSetupStrParser::xParseSourceView()
{
  m_bCurrentViewSet = false;

  Char cChar;
  xGetNextCharGoOn( cChar );
  xError( cChar != '[' );
  xReadViewInfo('B');

  Bool bContinueReading = true;
  while( bContinueReading )
  {
    xGetNextCharGoOn( cChar );
    switch ( cChar )
    {
    case 'B':
    case 'I':
    case 'E':
    case 'L':
    case 'R':
      xReadViews( cChar );
      break;
    case ']':
      bContinueReading = false;
      break;
    default:
      xError(true);
      break;
    }
  }
}

Void
TRenModSetupStrParser::xReadViews( Char cType )
{
  Char cChar;
  xGetNextCharGoOn( cChar );
  xError( cChar != '(' );

  Bool bContinue = true;
  while ( bContinue )
  {
    xGetNextChar( cChar );
    if (cChar == ')')
    {
      xGetNextCharGoOn( cChar );
      bContinue = false;
    }
    else
    {
      xReadViewInfo( cType );
    }
  }
}

Void
TRenModSetupStrParser::xReadViewInfo( Char cType )
{
  std::vector<Int> aiViewNums;
  aiViewNums.clear();

  switch ( cType )
  {
  case 'B':
    Char cVideoType;
    Char cDepthType;

    xGetNextCharGoOn   ( cVideoType );
    xGetNextCharGoOn   ( cDepthType );
    xGetViewNumberRange( aiViewNums );

    if ( !m_bCurrentViewSet )
    {
      xError( aiViewNums.size() != 1 );
      m_iCurrentView = aiViewNums[0] / (Int) VIEW_NUM_PREC;
      if      ( cVideoType == 'x' )
      {
        m_iCurrentContent = 0;
        m_bCurrentViewSet = true;
      }
      else if ( cDepthType == 'x' )
      {
        m_iCurrentContent = 1;
        m_bCurrentViewSet = true;
      }
      else
      {
        xError( true );
      }
    }

    for ( Int iIdx = 0; iIdx < aiViewNums.size(); iIdx++ )
    {
      xAddBaseView( aiViewNums[iIdx], cVideoType, cDepthType );
    }
    break;

  case 'E':
  case 'I':
  case 'L':
  case 'R':
    Char cRefType;
    xGetNextCharGoOn   ( cRefType   );
    xGetViewNumberRange( aiViewNums );
    for ( Int iIdx = 0; iIdx < aiViewNums.size(); iIdx++ )
    {
      xAddSynthView( aiViewNums[iIdx], cType, cRefType );
    }
  }
}

Void
TRenModSetupStrParser::xAddBaseView( Int iViewIdx, Char cVideoType, Char cDepthType )
{
  AOF( m_bCurrentViewSet );

  if ( cDepthType == 'x' ) cDepthType = 'o';
  if ( cVideoType == 'x' ) cVideoType = 'o';



  xError( cDepthType != 'o' && cDepthType != 'c' && cVideoType != 'r' );
  xError( cVideoType != 'o' && cVideoType != 'c' && cVideoType != 'r' );
  m_aiAllBaseViewIdx.push_back( iViewIdx );
  m_aaaiBaseViewsIdx  [m_iCurrentContent][m_iCurrentView].push_back( iViewIdx          );
  m_aaaiVideoDistMode [m_iCurrentContent][m_iCurrentView].push_back( ( cVideoType == 'c' ) ? 2 : ( (cVideoType == 'r') ? 1 :  0 ) );
  m_aaaiDepthDistMode [m_iCurrentContent][m_iCurrentView].push_back( ( cDepthType == 'c' ) ? 2 : ( (cDepthType == 'r') ? 1 :  0 ) );
}

Void
TRenModSetupStrParser::xAddSynthView( Int iViewNum, Char cType, Char cRefType )
{
  AOF( m_bCurrentViewSet );

  xError( cRefType != 's' && cRefType != 'o' );

  m_aiAllSynthViewNums.push_back( iViewNum );

  Int iBlendMode;
  switch ( cType )
  {
  case 'E':
    iBlendMode = -1;
    break;
  case 'I':
    iBlendMode = 0;
    break;
  case 'L':
    iBlendMode = 1;
    break;
  case 'R':
    iBlendMode = 2;
    break;
  default:
    xError(false);
    break;
  }

  m_aaaiBlendMode    [m_iCurrentContent][m_iCurrentView].push_back( iBlendMode        );
  m_aaaiSynthViewNums[m_iCurrentContent][m_iCurrentView].push_back( iViewNum          );
  m_aaabExtrapolate  [m_iCurrentContent][m_iCurrentView].push_back( cType    == 'E'   );
  m_aaabOrgRef       [m_iCurrentContent][m_iCurrentView].push_back( cRefType == 'o'   );
  m_aaaiModelNums    [m_iCurrentContent][m_iCurrentView].push_back( m_iNumberOfModels );

  m_iNumberOfModels++;
}

Void
TRenModSetupStrParser::xError( Bool bIsError )
{
  if ( bIsError )
  {
    std::cout << "RenModel setup string invalid. Last character read: " << m_iPosInStr << std::endl;
    AOF( false );
    exit(0);
  }
}

Void
TRenModSetupStrParser::xGetViewNumberRange( std::vector<Int>& raiViewNumbers )
{
  size_t iStartPos;
  size_t iEndPos;
  Char cChar;
  xGetNextCharGoOn(cChar );
  if (cChar == '{')
  {
    iStartPos = m_iPosInStr;
    while( m_pchSetStr[m_iPosInStr] != '}' )
    {
      xError( m_iPosInStr == '\0' );
      m_iPosInStr++;
    }
    iEndPos = m_iPosInStr - 1;
    m_iPosInStr++;
  }
  else
  {
    iStartPos = m_iPosInStr - 1;
    while( m_pchSetStr[m_iPosInStr] != ' ' && m_pchSetStr[m_iPosInStr] != ',' && m_pchSetStr[m_iPosInStr] != ')' )
    {
      xError( m_iPosInStr == '\0' );
      m_iPosInStr++;
    }
    iEndPos = m_iPosInStr - 1;
  }

  size_t iNumElem = iEndPos - iStartPos + 1;
  Char* pcTempBuffer = new Char[  iNumElem + 1];
  strncpy( pcTempBuffer, m_pchSetStr + iStartPos, iNumElem );
  pcTempBuffer[iNumElem] = '\0';

  TAppComCamPara::convertNumberString( pcTempBuffer, raiViewNumbers, VIEW_NUM_PREC );
  delete[] pcTempBuffer;
}

Void
TRenModSetupStrParser::xGetNextCharGoOn( Char& rcNextChar )
{
  while ( m_pchSetStr[m_iPosInStr] == ' ' || m_pchSetStr[m_iPosInStr] == ',' )
  {
    xError( m_pchSetStr[m_iPosInStr] == '\0' );
    m_iPosInStr++;
  }
  rcNextChar = m_pchSetStr[m_iPosInStr];
  m_iPosInStr++;
}

Void
TRenModSetupStrParser::xGetNextChar( Char& rcNextChar )
{
  size_t iPos = m_iPosInStr;
  while ( ( m_pchSetStr[iPos] == ' ' || m_pchSetStr[iPos] == ',' ) && m_pchSetStr[iPos] != '\0' ) iPos++;
  rcNextChar = m_pchSetStr[iPos];
}
