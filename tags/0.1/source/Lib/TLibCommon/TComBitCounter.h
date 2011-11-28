

/** \file     TComBitcounter.h
    \brief    class for counting bits (header)
*/

#ifndef __COMBITCOUNTER__
#define __COMBITCOUNTER__

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "TComBitStream.h"

// ====================================================================================================================
// Class definition
// ====================================================================================================================

/// class for counting bits
class TComBitCounter : public TComBitIf
{
protected:
  UInt  m_uiBitCounter;
  
public:
  TComBitCounter()            {}
  virtual ~TComBitCounter()   {}
  
  Void        write                 ( UInt uiBits, UInt uiNumberOfBits )  { m_uiBitCounter += uiNumberOfBits; }
  Void        resetBits             ()                                    { m_uiBitCounter = 0;               }
  UInt getNumberOfWrittenBits() const { return m_uiBitCounter; }
};

#endif

