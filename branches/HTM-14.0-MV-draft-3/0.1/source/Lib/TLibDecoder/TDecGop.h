

/** \file     TDecGop.h
    \brief    GOP decoder class (header)
*/

#ifndef __TDECGOP__
#define __TDECGOP__

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "../TLibCommon/CommonDef.h"
#include "../TLibCommon/TComBitStream.h"
#include "../TLibCommon/TComList.h"
#include "../TLibCommon/TComPicYuv.h"
#include "../TLibCommon/TComPic.h"
#include "../TLibCommon/TComLoopFilter.h"
#include "../TLibCommon/TComAdaptiveLoopFilter.h"
#include "../TLibCommon/TComDepthMapGenerator.h"
#include "../TLibCommon/TComResidualGenerator.h"

#include "TDecEntropy.h"
#include "TDecSlice.h"
#include "TDecBinCoder.h"
#include "TDecBinCoderCABAC.h"

// ====================================================================================================================
// Class definition
// ====================================================================================================================

/// GOP decoder class
class TDecGop
{
private:
  Int                   m_iGopSize;
  TComList<TComPic*>    m_cListPic;         //  Dynamic buffer
  
  //  Access channel
  TDecEntropy*          m_pcEntropyDecoder;
  TDecSbac*             m_pcSbacDecoder;
  TDecBinCABAC*         m_pcBinCABAC;
  TDecCavlc*            m_pcCavlcDecoder;
  TDecSlice*            m_pcSliceDecoder;
  TComLoopFilter*       m_pcLoopFilter;

  TComDepthMapGenerator*  m_pcDepthMapGenerator;
  TComResidualGenerator*  m_pcResidualGenerator;
  
  // Adaptive Loop filter
  TComAdaptiveLoopFilter*       m_pcAdaptiveLoopFilter;
#if MTK_SAO
  TComSampleAdaptiveOffset*              m_pcSAO;
  SAOParam              m_cSaoParam;
#endif
  ALFParam              m_cAlfParam;
  Double                m_dDecTime;

  bool m_pictureDigestEnabled; ///< if true, handle picture_digest SEI messages

public:
  TDecGop();
  virtual ~TDecGop();
  
  Void  init    ( TDecEntropy*            pcEntropyDecoder, 
                 TDecSbac*               pcSbacDecoder, 
                 TDecBinCABAC*           pcBinCABAC,
                 TDecCavlc*              pcCavlcDecoder, 
                 TDecSlice*              pcSliceDecoder, 
                 TComLoopFilter*         pcLoopFilter, 
                 TComAdaptiveLoopFilter* pcAdaptiveLoopFilter,
#if MTK_SAO
                 TComSampleAdaptiveOffset*                pcSAO,
#endif
                 TComDepthMapGenerator*  pcDepthMapGenerator,
                 TComResidualGenerator*  pcResidualGenerator );
  Void  create  ();
  Void  destroy ();
  Void  decompressGop ( Bool bEos, TComBitstream* pcBitstream, TComPic*& rpcPic, Bool bExecuteDeblockAndAlf );
  Void  setGopSize( Int i) { m_iGopSize = i; }

  void setPictureDigestEnabled(bool enabled) { m_pictureDigestEnabled = enabled; }
};

#endif // !defined(AFX_TDECGOP_H__29440B7A_7CC0_48C7_8DD5_1A531D3CED45__INCLUDED_)

