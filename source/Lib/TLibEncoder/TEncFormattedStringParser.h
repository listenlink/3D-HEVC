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
