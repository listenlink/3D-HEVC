

/** \file     TEncBinCoderCABAC.h
    \brief    binary entropy encoder of CABAC
*/

#ifndef __TENC_BIN_CODER_CABAC__
#define __TENC_BIN_CODER_CABAC__

#include "../TLibCommon/TComCABACTables.h"
#include "TEncBinCoder.h"


class TEncBinCABAC : public TEncBinIf
{
public:
  TEncBinCABAC ();
  virtual ~TEncBinCABAC();
  
  Void  init              ( TComBitIf* pcTComBitIf );
  Void  uninit            ();
  
  Void  start             ();
  Void  finish            ();
  Void  copyState         ( TEncBinIf* pcTEncBinIf );
  
  Void  resetBits         ();
  UInt  getNumWrittenBits ();
  
  Void  encodeBin         ( UInt  uiBin,  ContextModel& rcCtxModel );
  Void  encodeBinEP       ( UInt  uiBin                            );
  Void  encodeBinTrm      ( UInt  uiBin                            );
  
  TEncBinCABAC* getTEncBinCABAC()  { return this; }
  
  Void  setBinsCoded              ( UInt uiVal )  { m_uiBinsCoded = uiVal;          }
  UInt  getBinsCoded              ()              { return m_uiBinsCoded;           }
  Void  setBinCountingEnableFlag  ( Bool bFlag )  { m_bBinCountingEnabled = bFlag;  }
  Bool  getBinCountingEnableFlag  ()              { return m_bBinCountingEnabled;   }
protected:
  Void  xWriteBit               ( UInt uiBit );
  Void  xWriteBitAndBitsToFollow( UInt uiBit );
  
private:
  TComBitIf*          m_pcTComBitIf;
  UInt                m_uiLow;
  UInt                m_uiRange;
  UInt                m_uiBitsToFollow;
  UInt                m_uiByte;
  UInt                m_uiBitsLeft;
  UInt                m_uiBinsCoded;
  Bool                m_bBinCountingEnabled;
};


#endif

