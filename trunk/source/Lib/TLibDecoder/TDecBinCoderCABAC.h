

/** \file     TDecBinCoderCABAC.h
    \brief    binary entropy decoder of CABAC
*/

#ifndef __TDEC_BIN_CODER_CABAC__
#define __TDEC_BIN_CODER_CABAC__

#include "../TLibCommon/TComCABACTables.h"
#include "TDecBinCoder.h"


class TDecBinCABAC : public TDecBinIf
{
public:
  TDecBinCABAC ();
  virtual ~TDecBinCABAC();

  Void  init              ( TComBitstream* pcTComBitstream );
  Void  uninit            ();

  Void  start             ();
  Void  finish            ();

  Void  decodeBin         ( UInt& ruiBin, ContextModel& rcCtxModel );
  Void  decodeBinEP       ( UInt& ruiBin                           );
  Void  decodeBinTrm      ( UInt& ruiBin                           );

private:
  Void  xReadBit          ( UInt& ruiVal );

private:
  TComBitstream*      m_pcTComBitstream;
  UInt                m_uiRange;
  UInt                m_uiValue;
};


#endif

