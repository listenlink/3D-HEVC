

/** \file     TDecSlice.h
    \brief    slice decoder class (header)
*/

#ifndef __TDECSLICE__
#define __TDECSLICE__

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "../TLibCommon/CommonDef.h"
#include "../TLibCommon/TComBitStream.h"
#include "../TLibCommon/TComPic.h"
#include "TDecEntropy.h"
#include "TDecCu.h"

// ====================================================================================================================
// Class definition
// ====================================================================================================================

/// slice decoder class
class TDecSlice
{
private:
  // access channel
  TDecEntropy*    m_pcEntropyDecoder;
  TDecCu*         m_pcCuDecoder;
  UInt            m_uiCurrSliceIdx;

public:
  TDecSlice();
  virtual ~TDecSlice();
  
  Void  init              ( TDecEntropy* pcEntropyDecoder, TDecCu* pcMbDecoder );
  Void  create            ( TComSlice* pcSlice, Int iWidth, Int iHeight, UInt uiMaxWidth, UInt uiMaxHeight, UInt uiMaxDepth );
  Void  destroy           ();
  
  Void  decompressSlice   ( TComBitstream* pcBitstream, TComPic*& rpcPic );

};

#endif

