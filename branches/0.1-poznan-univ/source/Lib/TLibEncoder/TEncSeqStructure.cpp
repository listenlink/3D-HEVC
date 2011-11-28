

#include "TEncSeqStructure.h"
#include "TEncFormattedStringParser.h"
#include <algorithm>

ErrVal FrameDescriptor::initFrameDescriptor( const std::string&  rcString, UInt &ruiIncrement, std::map<UInt, UInt>& rcColDirTracker)
{
  std::string::size_type uiPos = 0;

  while( 1 )
  {
    SliceType eSliceType;
    Bool bStoreForRef;
    Bool bIsIDR;
    UInt uiLayer;

    RNOK( TEncFormattedStringParser::extractFrameType( rcString, eSliceType, bStoreForRef, bIsIDR, uiPos ) );
    RNOK( TEncFormattedStringParser::extractFrameLayer( rcString, uiLayer, uiPos ) );

    m_aeSliceType.push_back( eSliceType );
    m_abStoreForRef.push_back( bStoreForRef );
    m_abIsIDR.push_back( bIsIDR );
    m_auiLayer.push_back( uiLayer );
    m_aaiAllowedRelativeRefPocsL0.push_back( std::vector<int>() );
    m_aaiAllowedRelativeRefPocsL1.push_back( std::vector<int>() );
    m_aaiAllowedReferenceViewIdxL0.push_back( std::vector<int>() );
    m_aaiAllowedReferenceViewIdxL1.push_back( std::vector<int>() );

    if( eSliceType == P_SLICE )
    {
      RNOK( TEncFormattedStringParser::extractAllowedRelativeRefPocs( rcString, m_aaiAllowedRelativeRefPocsL0.back(), m_aaiAllowedReferenceViewIdxL0.back(), uiPos ) );
    }
    else if( eSliceType == B_SLICE )
    {
      RNOK( TEncFormattedStringParser::extractAllowedRelativeRefPocs( rcString, m_aaiAllowedRelativeRefPocsL0.back(), m_aaiAllowedRelativeRefPocsL1.back(),
                                                                      m_aaiAllowedReferenceViewIdxL0.back(), m_aaiAllowedReferenceViewIdxL1.back(), uiPos ) );
    }

    // check that only available views are referenced
    {
      const Int iCurrViewIdx = Int( m_aeSliceType.size() - 1 );
      for( UInt ui = 0; ui < m_aaiAllowedReferenceViewIdxL0.back().size(); ui++ )
      {
        const Int iRefViewIdx = m_aaiAllowedReferenceViewIdxL0.back()[ui];
        ROF( iRefViewIdx == -1 || ( 0 <= iRefViewIdx && iRefViewIdx < iCurrViewIdx ) );
      }
      for( UInt ui = 0; ui < m_aaiAllowedReferenceViewIdxL1.back().size(); ui++ )
      {
        const Int iRefViewIdx = m_aaiAllowedReferenceViewIdxL1.back()[ui];
        ROF( iRefViewIdx == -1 || ( 0 <= iRefViewIdx && iRefViewIdx < iCurrViewIdx ) );
      }
    }

    ROT( eSliceType == I_SLICE && ! m_aaiAllowedRelativeRefPocsL0.back().empty() && ! m_aaiAllowedRelativeRefPocsL1.back().empty() );
    ROT( eSliceType == P_SLICE && ! m_aaiAllowedRelativeRefPocsL1.back().empty() );
    if( rcString[uiPos] == '_' )
    {
      uiPos++;
      break;
    }
  }

  RNOK( TEncFormattedStringParser::extractFrameIncrement( rcString, ruiIncrement, uiPos ) );

  if(rcColDirTracker.find( m_auiLayer[0] )==rcColDirTracker.end())
    rcColDirTracker.insert( std::make_pair(m_auiLayer[0],1) );

  m_uiColDir = rcColDirTracker[ m_auiLayer[0] ];
  rcColDirTracker[ m_auiLayer[0]  ] = 1 - rcColDirTracker[ m_auiLayer[0] ];

  ROF( std::string::npos == uiPos || rcString.length() == uiPos );

  return Err::m_nOK;
}


UInt64 TEncSeqStructure::FrameSequencePart::findIncrement( UInt64 uiIncrement, bool &rbSuccess ) const
{
  TOF( isInfinitelyLong() || uiIncrement < getSize() );
  rbSuccess = false;
  const UInt64 uiOffset = uiIncrement % xGetSize();
  for( UInt ui = 0; ui < UInt( m_auiIncrements.size() ); ui++ )
  {
    if( uiOffset == m_auiIncrements[ui] )
    {
      rbSuccess = true;
      return uiIncrement - uiOffset + ui;
    }
  }
  return 0;
}


TEncSeqStructure::FrameSequencePart::FrameSequencePart( const std::string& rcString )
{
  std::string cNoRepString;

  //----- get number of repetitions and number of frames -----
  UInt uiNumRep = 0;
  TNOK( TEncFormattedStringParser::extractRepetitions( rcString, cNoRepString, uiNumRep ) );
  xSetNumRep( uiNumRep );
  UInt uiNumberOfFrames = 0;
  TNOK( TEncFormattedStringParser::getNumberOfFrames( cNoRepString, uiNumberOfFrames ) );
  TOF( uiNumberOfFrames );
  //----- create array -----
  m_acFrameDescriptors.resize( uiNumberOfFrames );
  TOF( m_acFrameDescriptors.size() == uiNumberOfFrames );
  m_auiIncrements.resize( uiNumberOfFrames );

  //----- initialize array -----
  UInt uiIndex;
  size_t uiPos;
  for( uiPos = 0, uiIndex = 0; uiIndex < uiNumberOfFrames; uiIndex++ )
  {
    std::string cFDString;
    TNOK( TEncFormattedStringParser::extractNextFrameDescription( cNoRepString, cFDString, uiPos ) );
    UInt uiIncrement = 0;
    TNOK( m_acFrameDescriptors[uiIndex].initFrameDescriptor( cFDString, uiIncrement, m_cColDirTracker ) );
    m_auiIncrements[uiIndex] = uiIncrement;
  }
  TNOK( xCheck() );
}


ErrVal TEncSeqStructure::FrameSequencePart::xCheck()
{
  ROTS( m_acFrameDescriptors.empty() );

  std::vector<bool> abCovered( m_acFrameDescriptors.size(), false );
  for( UInt uiIndex = 0; uiIndex < m_acFrameDescriptors.size(); uiIndex++ )
  {
    const UInt uiInc = m_auiIncrements[uiIndex];
    ROT( uiInc >= m_acFrameDescriptors.size() ); // error in config file parameter GOPFormatString
    ROT( abCovered[ uiInc ] );
    abCovered[uiInc] = true;
  }

  return Err::m_nOK;
}


TEncSeqStructure::GeneralSequencePart::GeneralSequencePart( const std::string& rcString )
{
  std::string cNoRepString;

  UInt uiNumRep = 0;
  TNOK( TEncFormattedStringParser::extractRepetitions( rcString, cNoRepString, uiNumRep ) );
  xSetNumRep( uiNumRep );
  UInt uiNumberOfParts = 0;
  TNOK( TEncFormattedStringParser::getNumberOfParts( cNoRepString, uiNumberOfParts ) );
  TOF( uiNumberOfParts );

  //----- create array -----
  m_apcSequenceParts.resize( uiNumberOfParts );

  //----- initialize array -----
  UInt uiIndex;
  size_t uiPos;
  for( uiPos = 0, uiIndex = 0; uiIndex < uiNumberOfParts; uiIndex++ )
  {
    std::string cPartString;

    TNOK( TEncFormattedStringParser::extractPart( cNoRepString, cPartString, uiPos ) );

    if( TEncFormattedStringParser::isFrameSequencePart( cPartString ) )
    {
      m_apcSequenceParts[uiIndex] = new FrameSequencePart( cPartString );
    }
    else
    {
      m_apcSequenceParts[uiIndex] = new GeneralSequencePart( cPartString );
    }
  }
}


TEncSeqStructure::TEncSeqStructure()
: m_pcSequencePart( NULL )
{
}

TEncSeqStructure::~TEncSeqStructure()
{
  delete m_pcSequencePart; m_pcSequencePart = NULL ;
}

ErrVal TEncSeqStructure::init( const std::string& rcString )
{
  std::string cStringWithoutWhitespace;
  for( UInt ui = 0; ui < rcString.length(); ui++ )
  {
    if( rcString[ui] != ' ' && rcString[ui] != '\t' )
    {
      cStringWithoutWhitespace += rcString[ui];
    }
  }

  if( cStringWithoutWhitespace.find( "*n{" ) == std::string::npos )
  {
    cStringWithoutWhitespace = "*n{" + cStringWithoutWhitespace + "}";
  }
  m_pcSequencePart = new GeneralSequencePart( cStringWithoutWhitespace );

  return Err::m_nOK;
}


UInt TEncSeqStructure::getMaxAbsPocDiff( const UInt uiNumberOfFrames )
{
  TEncSeqStructure::Iterator cSeqIter = TEncSeqStructure::Iterator( *this, PicOrderCnt( 0 ), 0 );

  UInt uiMaxAbsPocDiff = 0;
  Int64 iLastPoc = cSeqIter.getPoc();
  int iNumFramesLeft = uiNumberOfFrames;

  for( int iIndex = 0; 0 != iNumFramesLeft; ++cSeqIter, iIndex++ )
  {
    Int64 iPoc = cSeqIter.getPoc();
    if( iPoc >= uiNumberOfFrames )
    {
      // skiped due to frame behind end of sequence
      continue;
    }
    iNumFramesLeft--;
    //----- check POC differences -----
    UInt uiAbsFrameDiffRef = (UInt) gAbs( iPoc - iLastPoc );
    if( uiMaxAbsPocDiff < uiAbsFrameDiffRef  )
    {
      uiMaxAbsPocDiff = uiAbsFrameDiffRef;
    }
    iLastPoc = iPoc;
  }

  return uiMaxAbsPocDiff;
}

TEncSeqStructure::Iterator::Iterator( const TEncSeqStructure &r, PicOrderCnt cLayerChangeStartPoc, int iLayerOffset )
: m_cBasePoc( 0 )
, m_cLayerChangeStartPoc( cLayerChangeStartPoc )
, m_iLayerOffset( iLayerOffset )
{
  m_acSeqPartPath.resize( 1 );
  xGetCurr().m_pcSeqPart = r.m_pcSequencePart;
  xGetCurr().m_uiCurrPos = 0;
  xGoToLeaf( false );
}

void TEncSeqStructure::Iterator::xGoToLeaf( bool bGoToRightmostLeaf )
{
  while( !xGetCurr().m_pcSeqPart->isLeafNode() )
  {
    SequencePartWithPos cPartWithPos;
    cPartWithPos.m_pcSeqPart = xGetCurr().m_pcSeqPart->getChildNode( xGetCurr().m_uiCurrPos );
    cPartWithPos.m_uiCurrPos = 0;
    if( bGoToRightmostLeaf )
    {
      TOT( cPartWithPos.m_pcSeqPart->isInfinitelyLong() );
      cPartWithPos.m_uiCurrPos = cPartWithPos.m_pcSeqPart->getSize() - 1;
    }
    m_acSeqPartPath.push_back( cPartWithPos );
  }
}


TEncSeqStructure::Iterator& TEncSeqStructure::Iterator::operator++()
{
  ++xGetCurr().m_uiCurrPos;
  if( !xGetCurr().m_pcSeqPart->isInfinitelyLong() && xGetCurr().m_uiCurrPos == xGetCurr().m_pcSeqPart->getSize() )
  {
    xGoToNextFrameSequencePart();
    xGetCurr().m_uiCurrPos = 0;
  }
  return *this;
}


TEncSeqStructure::Iterator& TEncSeqStructure::Iterator::traverseByPocDiff( Int64 iPocDiff )
{
  const PicOrderCnt cTargetPoc = getPoc() + iPocDiff;
  const Int64 iIncrement = Int64( xGetCurr().m_pcSeqPart->getIncrement( xGetCurr().m_uiCurrPos ) );
  if( iIncrement + iPocDiff < 0 )
  {
    xGoToPreviousFrameSequencePart();
    const Int64 iNewIncrement = Int64( xGetCurr().m_pcSeqPart->getIncrement( xGetCurr().m_uiCurrPos ) );
    const Int64 iNewSize = Int64( xGetCurr().m_pcSeqPart->getSize() );
    traverseByPocDiff( iPocDiff + iIncrement + iNewSize - iNewIncrement );
    TOF( cTargetPoc == getPoc() );
    return *this;
  }

  if( !xGetCurr().m_pcSeqPart->isInfinitelyLong() )
  {
    const Int64 iSize = Int64( xGetCurr().m_pcSeqPart->getSize() );
    if( iIncrement + iPocDiff >= iSize )
    {
      xGoToNextFrameSequencePart();
      traverseByPocDiff( iPocDiff - ( iSize - iIncrement ) );
      TOF( cTargetPoc == getPoc() );
      return *this;
    }
  }

  UInt64 uiPocDiffToFind = UInt64( iIncrement + iPocDiff );
  bool bFound = false;
  xGetCurr().m_uiCurrPos = xGetCurr().m_pcSeqPart->findIncrement( uiPocDiffToFind, bFound );
  TOF( bFound );
  TOF( cTargetPoc == getPoc() );
  return *this;
}


TEncSeqStructure::Iterator TEncSeqStructure::Iterator::getIterByPocDiff( Int64 iPocDiff ) const
{
  Iterator cCopy = *this;
  cCopy.traverseByPocDiff( iPocDiff );
  return cCopy;
}


void TEncSeqStructure::Iterator::xGoToPreviousFrameSequencePart()
{
  TOF( xGetCurr().m_pcSeqPart->isLeafNode() );
  TOF( m_acSeqPartPath.size() > 1 );
  m_acSeqPartPath.pop_back();
  while( xGetCurr().m_uiCurrPos == 0 )
  {
    TOF( m_acSeqPartPath.size() > 1 );
    m_acSeqPartPath.pop_back();
  }
  xGetCurr().m_uiCurrPos--;
  xGoToLeaf( true );
  m_cBasePoc -= xGetCurr().m_pcSeqPart->getSize();
}


void TEncSeqStructure::Iterator::xGoToNextFrameSequencePart()
{
  TOF( xGetCurr().m_pcSeqPart->isLeafNode() );
  TOT( xGetCurr().m_pcSeqPart->isInfinitelyLong() );
  m_cBasePoc += xGetCurr().m_pcSeqPart->getSize();
  m_acSeqPartPath.pop_back();
  while( !xGetCurr().m_pcSeqPart->isInfinitelyLong() && xGetCurr().m_uiCurrPos == xGetCurr().m_pcSeqPart->getSize() - 1 )
  {
    TOF( m_acSeqPartPath.size() > 1 );
    m_acSeqPartPath.pop_back();
  }
  xGetCurr().m_uiCurrPos++;
  xGoToLeaf( false );
}



