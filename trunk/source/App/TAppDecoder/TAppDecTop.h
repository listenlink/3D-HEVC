

/** \file     TAppDecTop.h
    \brief    Decoder application class (header)
*/

#ifndef __TAPPDECTOP__
#define __TAPPDECTOP__

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "../../Lib/TLibVideoIO/TVideoIOYuv.h"
#include "../../Lib/TLibVideoIO/TVideoIOBits.h"
#include "../../Lib/TLibCommon/TComList.h"
#include "../../Lib/TLibCommon/TComPicYuv.h"
#include "../../Lib/TLibCommon/TComBitStream.h"
#include "../../Lib/TLibCommon/TComDepthMapGenerator.h"
#include "../../Lib/TLibDecoder/TDecTop.h"
#include "TAppDecCfg.h"

// ====================================================================================================================
// Class definition
// ====================================================================================================================

/// decoder application class
class TAppDecTop : public TAppDecCfg
{
private:
  // class interface
  std::vector<TDecTop*>           m_acTDecTopList;
  std::vector<TDecTop*>           m_acTDecDepthTopList;
  TComBitstream*                  m_apcBitstream;                 ///< bitstream class
  TVideoIOBitsStartCode           m_cTVideoIOBitstreamFile;       ///< file I/O class
  std::vector<TVideoIOYuv*>       m_acTVideoIOYuvReconFileList;
  std::vector<TVideoIOYuv*>       m_acTVideoIOYuvDepthReconFileList;
  
  Bool m_bUsingDepth;

  // for output control
  //SB can be deleted?
  Bool                            m_abDecFlag[ MAX_GOP ];         ///< decoded flag in one GOP
//  Int                             m_iPOCLastDisplay;              ///< last POC in display order

  std::vector<Bool>               m_abDecFlagList;         ///< decoded flag in one GOP
  std::vector<Int>                m_aiPOCLastDisplayList;
  std::vector<Int>                m_aiDepthPOCLastDisplayList;

  FILE*                           m_pScaleOffsetFile;
  CamParsCollector                m_cCamParsCollector;
  TComSPSAccess                   m_cSPSAccess;
  TComAUPicAccess                 m_cAUPicAccess;
  
public:
  TAppDecTop();
  virtual ~TAppDecTop() {}
  
  Void  create            (); ///< create internal members
  Void  destroy           (); ///< destroy internal members
  Void  decode            (); ///< main decoding function
  Void  increaseNumberOfViews	(Int iNewNumberOfViews);
  Void  startUsingDepth() ;

// GT FIX
  std::vector<TComPic*> getSpatialRefPics( Int iViewIdx, Int iPoc, Bool bIsDepth );
  TComPic* getPicFromView( Int iViewIdx, Int iPoc, bool bDepth );
// GT FIX END

  TComSPSAccess*    getSPSAccess  () { return &m_cSPSAccess;   }
  TComAUPicAccess*  getAUPicAccess() { return &m_cAUPicAccess; }
  
protected:
  Void  xCreateDecLib     (); ///< create internal classes
  Void  xDestroyDecLib    (); ///< destroy internal classes
  Void  xInitDecLib       (); ///< initialize decoder class
  
  Void  xWriteOutput      ( TComList<TComPic*>* pcListPic ); ///< write YUV to file
};

#endif

