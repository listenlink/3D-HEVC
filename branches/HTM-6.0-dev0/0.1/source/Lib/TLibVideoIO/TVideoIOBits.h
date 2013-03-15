

/** \file     TVideoIOBits.h
    \brief    bitstream file I/O class (header)
*/

#ifndef __TVIDEOIOBITS__
#define __TVIDEOIOBITS__

#include <stdio.h>
#include <fstream>
#include <iostream>
#include "../TLibCommon/CommonDef.h"
#include "../TLibCommon/TComBitStream.h"

// ====================================================================================================================
// Class definition
// ====================================================================================================================

/// bitstream file I/O class
class TVideoIOBits
{
private:
  std::fstream   m_cHandle;                                      ///< file handle
  
public:
  TVideoIOBits()            {}
  virtual ~TVideoIOBits()   {}
  
  Void openBits   ( char* pchFile,  Bool bWriteMode );      ///< open or create file
  Void closeBits  ();                                       ///< close file
  
  Bool readBits   ( TComBitstream*& rpcBitstream    );      ///< read  one packet from file
  Void writeBits  ( TComBitstream*  pcBitstream     );      ///< write one packet to   file
  
};

/// bitstream file I/O class
class TVideoIOBitsStartCode
{
private:
  std::fstream   m_cHandle;                                      ///< file handle
  
public:
  TVideoIOBitsStartCode()            
  {
  }
  virtual ~TVideoIOBitsStartCode()   {}
  
  Void openBits   ( char* pchFile,  Bool bWriteMode );      ///< open or create file
  Void closeBits  ();                                       ///< close file
  
  Bool readBits   ( TComBitstream*& rpcBitstream    );      ///< read  one packet from file
  Void writeBits  ( TComBitstream*  pcBitstream     );      ///< write one packet to   file
  
  std::streampos   getFileLocation      ()                          { return m_cHandle.tellg();                           }
  Void             setFileLocation      (std::streampos uiLocation) { m_cHandle.seekg(uiLocation, std::ios_base::beg);    }
  Void             clear                ()                          { m_cHandle.clear();                                  }
  Bool             good                 ()                          { return m_cHandle.good();                            }

private:
  int xFindNextStartCode(UInt& ruiPacketSize, UChar* pucBuffer); ///< get packet size and number of startcode bytes and seeks to the packet's start position
  
};

#endif // __TVIDEOIOBITS__

