

/** \file     TDecBinCoder.h
    \brief    binary entropy decoder interface
*/

#ifndef __TDEC_BIN_CODER__
#define __TDEC_BIN_CODER__

#include "../TLibCommon/ContextModel.h"
#include "../TLibCommon/TComBitStream.h"


class TDecBinIf
{
public:
  virtual Void  init              ( TComBitstream* pcTComBitstream )          = 0;
  virtual Void  uninit            ()                                          = 0;

  virtual Void  start             ()                                          = 0;
  virtual Void  finish            ()                                          = 0;

  virtual Void  decodeBin         ( UInt& ruiBin, ContextModel& rcCtxModel )  = 0;
  virtual Void  decodeBinEP       ( UInt& ruiBin                           )  = 0;
  virtual Void  decodeBinTrm      ( UInt& ruiBin                           )  = 0;
  
  virtual ~TDecBinIf() {}
};

#endif

