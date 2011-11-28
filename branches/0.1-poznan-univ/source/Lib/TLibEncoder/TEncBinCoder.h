

/** \file     TEncBinCoder.h
    \brief    binary entropy encoder interface
*/

#ifndef __TENC_BIN_CODER__
#define __TENC_BIN_CODER__

#include "../TLibCommon/ContextModel.h"
#include "../TLibCommon/TComBitStream.h"


class TEncBinCABAC;


class TEncBinIf
{
public:
  virtual Void  init              ( TComBitIf* pcTComBitIf )                  = 0;
  virtual Void  uninit            ()                                          = 0;

  virtual Void  start             ()                                          = 0;
  virtual Void  finish            ()                                          = 0;
  virtual Void  copyState         ( TEncBinIf* pcTEncBinIf )                  = 0;    

  virtual Void  resetBits         ()                                          = 0;
  virtual UInt  getNumWrittenBits ()                                          = 0;

  virtual Void  encodeBin         ( UInt  uiBin,  ContextModel& rcCtxModel )  = 0;
  virtual Void  encodeBinEP       ( UInt  uiBin                            )  = 0;
  virtual Void  encodeBinTrm      ( UInt  uiBin                            )  = 0;

  virtual TEncBinCABAC*   getTEncBinCABAC   ()  { return 0; }
  
  virtual ~TEncBinIf() {}
};


#endif

