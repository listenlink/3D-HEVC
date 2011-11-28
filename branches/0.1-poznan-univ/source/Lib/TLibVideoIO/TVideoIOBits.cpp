

/** \file     TVideoIOBits.cpp
    \brief    bitstream file I/O class
*/

#include <cstdlib>
#include <fcntl.h>
#include <assert.h>
#include <sys/stat.h>
#include <fstream>
#include <iostream>

#include "TVideoIOBits.h"

using namespace std;

// ====================================================================================================================
// Public member functions
// ====================================================================================================================

/**
 \param pchFile    file name string
 \param bWriteMode file open mode
 */
Void TVideoIOBits::openBits( char* pchFile, Bool bWriteMode )
{
  if ( bWriteMode )
  {
    m_cHandle.open( pchFile, ios::binary | ios::out );
    
    if( m_cHandle.fail() )
    {
      printf("\nfailed to write Bitstream file\n");
      exit(0);
    }
  }
  else
  {
    m_cHandle.open( pchFile, ios::binary | ios::in );
    
    if( m_cHandle.fail() )
    {
      printf("\nfailed to read Bitstream file\n");
      exit(0);
    }
  }
  
  return;
}

Void TVideoIOBits::closeBits()
{
  m_cHandle.close();
}

/**
 \param  rpcBitstream    bitstream class pointer
 \retval                 true if EOF is reached
 */
Bool TVideoIOBits::readBits( TComBitstream*& rpcBitstream )
{
  UInt  uiBytes = 0;
  
  rpcBitstream->rewindStreamPacket();
  
  // check end-of-file
  if ( m_cHandle.eof() ) return true;
  
  // read 32-bit packet size
  m_cHandle.read( reinterpret_cast<char*>(&uiBytes), sizeof (Int) );
  
  // kolya
  if ( m_cHandle.eof() ) return true; //additional insertion to avoid over-reading, for <fstream>
  
  // read packet data
  m_cHandle.read(  reinterpret_cast<char*>(rpcBitstream->getBuffer()), uiBytes );
  
  // initialize parsing process
  rpcBitstream->initParsing ( uiBytes );
  
  assert (uiBytes >= 4);
  
  return false;
}

/** \param  pcBitstream   bitstream class pointer
 */
Void TVideoIOBits::writeBits( TComBitstream* pcBitstream )
{
  UInt*  plBuff  = pcBitstream->getStartStream();
  UInt   uiBytes = pcBitstream->getNumberOfWrittenBits() >> 3;
  
  // write 32-bit packet size
  m_cHandle.write( reinterpret_cast<char*>(&uiBytes ), sizeof(UInt) );
  
  // write packet data
  m_cHandle.write( reinterpret_cast<char*>(plBuff   ), uiBytes      );
}

////////////////////////////////////////////////////////////////////

Void TVideoIOBitsStartCode::openBits( char* pchFile, Bool bWriteMode )
{
  if ( bWriteMode )
  {
    m_cHandle.open( pchFile, ios::binary | ios::out );
    
    if( m_cHandle.fail() )
    {
      printf("\nfailed to write Bitstream file\n");
      exit(0);
    }
  }
  else
  {
    m_cHandle.open( pchFile, ios::binary | ios::in );
    
    if( m_cHandle.fail() )
    {
      printf("\nfailed to read Bitstream file\n");
      exit(0);
    }
  }
  
  return;
}

Void TVideoIOBitsStartCode::closeBits()
{
  m_cHandle.close();
}

/**
 \param  rpcBitstream    bitstream class pointer
 \retval                 true if EOF is reached
 */
Bool TVideoIOBitsStartCode::readBits( TComBitstream*& rpcBitstream )
{
  rpcBitstream->rewindStreamPacket();
  
  // check end-of-file
  if ( m_cHandle.eof() ) return true;
  
  UInt uiPacketSize = 0;
  if( 0 != xFindNextStartCode( uiPacketSize, reinterpret_cast<UChar*>(rpcBitstream->getBuffer()) ) )
  {
    return true;
  }
  
  // initialize parsing process
  rpcBitstream->initParsingConvertPayloadToRBSP( uiPacketSize );
  return false;
}

int TVideoIOBitsStartCode::xFindNextStartCode(UInt& ruiPacketSize, UChar* pucBuffer)
{
  UInt uiDummy = 0;
  m_cHandle.read( reinterpret_cast<char*>(&uiDummy), 3 );
  if ( m_cHandle.eof() ) return -1;
  assert( 0 == uiDummy );
  
  m_cHandle.read( reinterpret_cast<char*>(&uiDummy), 1 );
  if ( m_cHandle.eof() ) return -1;
  assert( 1 == uiDummy );
  
  Int iNextStartCodeBytes = 0;
  Int iBytesRead = 0;
  UInt uiZeros = 0;
  while( true )
  {
    UChar ucByte = 0;
    m_cHandle.read( reinterpret_cast<char*>(&ucByte), 1 );
    if ( m_cHandle.eof() )
    {
      iNextStartCodeBytes = 0;
      break;
    }
    pucBuffer[iBytesRead++] = ucByte;
    if( 1 < ucByte )
    {
      uiZeros = 0;
    }
    else if( 0 == ucByte )
    {
      uiZeros++;
    }
    else if( uiZeros > 2)
    {
      iNextStartCodeBytes = 3 + 1;
      break;
    }
    else
    {
      uiZeros = 0;
    }
  }
  
  ruiPacketSize = iBytesRead - iNextStartCodeBytes;
  
  m_cHandle.seekg( -iNextStartCodeBytes, ios::cur );
  return 0;
}

/**
 \param  pcBitstream   bitstream class pointer
 */
Void TVideoIOBitsStartCode::writeBits( TComBitstream* pcBitstream )
{
  UInt*  plBuff  = pcBitstream->getStartStream();
  UInt   uiBytes = pcBitstream->getNumberOfWrittenBits() >> 3;
  Char   ucZero = 0;
  Char   ucOne = 1;
  
  // write 32-bit packet size
  m_cHandle.write( &ucZero , sizeof(Char) );
  m_cHandle.write( &ucZero , sizeof(Char) );
  m_cHandle.write( &ucZero , sizeof(Char) );
  m_cHandle.write( &ucOne  , sizeof(Char) );
  
  // write packet data
  m_cHandle.write( reinterpret_cast<char*>(plBuff   ), uiBytes      );
}


