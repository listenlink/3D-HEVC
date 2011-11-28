

/** \file     TComBitStream.h
    \brief    class for handling bitstream (header)
*/

#ifndef __COMBITSTREAM__
#define __COMBITSTREAM__

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <stdio.h>
#include <assert.h>
#include "CommonDef.h"

// ====================================================================================================================
// Class definition
// ====================================================================================================================

/// pure virtual class for basic bit handling
class TComBitIf
{
public:
  virtual Void        writeAlignOne         () {};
  virtual Void        write                 ( UInt uiBits, UInt uiNumberOfBits )  = 0;
  virtual Void        resetBits             ()                                    = 0;
  virtual UInt getNumberOfWrittenBits() const = 0;
  virtual ~TComBitIf() {}
};

/// class for handling bitstream
class TComBitstream : public TComBitIf
{
protected:
  UInt*       m_apulStreamPacketBegin;
  UInt*       m_pulStreamPacket;
  UInt        m_uiBufSize;
  
  UInt        m_uiBitSize;
  Int         m_iValidBits;
  
  UInt        m_ulCurrentBits;
  UInt        m_uiBitsWritten;
  
  UInt        m_uiDWordsLeft;
  UInt        m_uiBitsLeft;
  UInt        m_uiNextBits;
  
  UInt        *m_auiSliceByteLocation, m_uiSliceCount;  // used to skip over slice start codes in initParsingConvertPayloadToRBSP()
  UInt        m_uiSliceProcessed;

  UInt xSwap ( UInt ui )
  {
    // heiko.schwarz@hhi.fhg.de: support for BSD systems as proposed by Steffen Kamp [kamp@ient.rwth-aachen.de]
#ifdef MSYS_BIG_ENDIAN
    return ui;
#else
    UInt ul2;
    
    ul2  = ui>>24;
    ul2 |= (ui>>8) & 0x0000ff00;
    ul2 |= (ui<<8) & 0x00ff0000;
    ul2 |= ui<<24;
    
    return ul2;
#endif
  }
  
  // read one word
  __inline Void xReadNextWord ();
  
public:
  TComBitstream()             {}
  virtual ~TComBitstream()    {}
  
  // create / destroy
  Void        create          ( UInt uiSizeInBytes );
  Void        destroy         ();
  
  // interface for encoding
  Void        write           ( UInt uiBits, UInt uiNumberOfBits );
  Void        writeAlignOne   ();
  Void        writeAlignZero  ();
  Void        convertRBSPToPayload( UInt uiStartPos = 0);
  // interface for decoding
  Void        initParsingConvertPayloadToRBSP( const UInt uiBytesRead );
  Void        initParsing     ( UInt uiNumBytes );
#if LCEC_INTRA_MODE || QC_LCEC_INTER_MODE
  Void        pseudoRead      ( UInt uiNumberOfBits, UInt& ruiBits );
#endif
  Void        read            ( UInt uiNumberOfBits, UInt& ruiBits );
  Void        readAlignOne    ();
  UInt        getSliceProcessed                ()       { return m_uiSliceProcessed;                }
  Void        setSliceProcessed                (UInt u) { m_uiSliceProcessed                = u;    }
  
  // interface for slice start-code positioning at encoder
  UInt        getSliceCount                    ()                            { return m_uiSliceCount;                     }
  UInt        getSliceByteLocation             ( UInt uiIdx )                { return m_auiSliceByteLocation[ uiIdx ];    }
  Void        setSliceCount                    ( UInt uiCount )              { m_uiSliceCount = uiCount;                  }
  Void        setSliceByteLocation             ( UInt uiIdx, UInt uiCount )  { m_auiSliceByteLocation[ uiIdx ] = uiCount; }

  // memory allocation / deallocation interface for "slice location" bookkeeping
  Void        allocateMemoryForSliceLocations       ( UInt uiMaxNumOfSlices );
  Void        freeMemoryAllocatedForSliceLocations  ();

  // Peek at bits in word-storage. Used in determining if we have completed reading of current bitstream and therefore slice in LCEC.
  UInt        peekBits (UInt uiBits) { return( m_ulCurrentBits >> (32 - uiBits));  }

  // reset internal status
  Void        resetBits       ()
  {
    m_uiBitSize = 0;
    m_iValidBits = 32;
    m_ulCurrentBits = 0;
    m_uiBitsWritten = 0;
  }
  
  // utility functions
  unsigned read(unsigned numberOfBits) { UInt tmp; read(numberOfBits, tmp); return tmp; }
  UInt* getStartStream() const { return m_apulStreamPacketBegin; }
  UInt*       getBuffer()               { return  m_pulStreamPacket;                    }
  Int         getBitsUntilByteAligned() { return m_iValidBits & (0x7);                  }
  Void        setModeSbac()             { m_uiBitsLeft = 8*((m_uiBitsLeft+7)/8);        } // stop bit + trailing stuffing bits
  Bool        isWordAligned()           { return  (0 == (m_iValidBits & (0x1f)));       }
  UInt getNumberOfWrittenBits() const { return  m_uiBitsWritten; }
  Void        flushBuffer();
  Void        rewindStreamPacket()      { m_pulStreamPacket = m_apulStreamPacketBegin;  }
  UInt        getBitsLeft()             { return  m_uiBitsLeft;                         }

  void insertAt(const TComBitstream& src, unsigned pos);
};

#endif

