/* The copyright in this software is being made available under the BSD
 * License, included below. This software may be subject to other third party
 * and contributor rights, including patent rights, and no such rights are
 * granted under this license.  
 *
 * Copyright (c) 2010-2011, ISO/IEC
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *  * Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *  * Neither the name of the ISO/IEC nor the names of its contributors may
 *    be used to endorse or promote products derived from this software without
 *    specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

/** \file     TAppExtrTop.cpp
    \brief    Extractor application class
*/

#include <list>
#include <stdio.h>
#include <fcntl.h>
#include <assert.h>

#include "TAppExtrTop.h"

// ====================================================================================================================
// Local constants
// ====================================================================================================================

/// initial bitstream buffer size
/// should be large enough for parsing SPS
/// resized as a function of picture size after parsing SPS
#define BITS_BUF_SIZE 65536

// ====================================================================================================================
// Constructor / destructor / initialization / destroy
// ====================================================================================================================

TAppExtrTop::TAppExtrTop()
{
}

Void TAppExtrTop::create()
{
  m_apcInputBitstream  = new TComBitstream;
  m_apcInputBitstream->create( BITS_BUF_SIZE );

  if ( m_pchOutputBitstreamFile )
  {
     m_apcOutputBitstream  = new TComBitstream;
     m_apcOutputBitstream->create( BITS_BUF_SIZE );
  }
  else
  {
     m_apcOutputBitstream  = NULL;
  }
}

Void TAppExtrTop::destroy()
{
  if ( m_apcInputBitstream )
  {
    m_apcInputBitstream->destroy();
    delete m_apcInputBitstream;
    m_apcInputBitstream = NULL;
  }

  if ( m_apcOutputBitstream )
  {
    m_apcOutputBitstream->destroy();
    delete m_apcOutputBitstream;
    m_apcOutputBitstream = NULL;
  }
}

// ====================================================================================================================
// Public member functions
// ====================================================================================================================

/**
 - create internal class
 - initialize internal class
 - until the end of the bitstream, call extraction function in TExtrTop class
 - delete allocated buffers
 - destroy internal class
 .
 */
Void TAppExtrTop::extract()
{
  TComBitstream*      pcInputBitstream = m_apcInputBitstream;
  TComBitstream*      pcOutputBitstream = m_apcOutputBitstream;

  // create & initialize internal classes
  xCreateExtrLib();
  xInitExtrLib  ();

  // main extractor loop
  Bool resizedBitstreamBuffer = false;

  while ( !m_cTVideoIOInputBitstreamFile.readBits( pcInputBitstream ) )
  {
    // decide whether to extract packet or not
    if ( m_cTExtrTop.extract( pcInputBitstream, m_suiExtractLayerIds ) && pcOutputBitstream )
    {
       xCopyInputPacketIntoOutput();
       m_cTVideoIOOutputBitstreamFile.writeBits( pcOutputBitstream );
    }

    if ( !resizedBitstreamBuffer )
    {
      TComSPS *sps = m_cTExtrTop.getFirstSPS();
      if ( sps )
      {
        pcInputBitstream->destroy();
        pcInputBitstream->create(sps->getWidth() * sps->getHeight() * 2);
        if ( pcOutputBitstream )
        {
           pcOutputBitstream->destroy();
           pcOutputBitstream->create(sps->getWidth() * sps->getHeight() * 2);
        }
        resizedBitstreamBuffer = true;
      }
    }
  }

  // write SPS info file
  if ( m_pchSpsInfoFile )
  {
    m_cTExtrTop.dumpSpsInfo( m_cSpsInfoFileHandle );
  }
  m_cTExtrTop.dumpSpsInfo( std::cout );
  
  // destroy internal classes
  xDestroyExtrLib();
}

// ====================================================================================================================
// Protected member functions
// ====================================================================================================================

Void TAppExtrTop::xCreateExtrLib()
{
  // open bitstream files
  m_cTVideoIOInputBitstreamFile.openBits( m_pchInputBitstreamFile, false);  // read mode
  if ( m_pchOutputBitstreamFile )
  {
     m_cTVideoIOOutputBitstreamFile.openBits( m_pchOutputBitstreamFile, true);  // write mode
  }

  // open text file
  if ( m_pchSpsInfoFile )
  {
     m_cSpsInfoFileHandle.open( m_pchSpsInfoFile, ios::binary | ios::out );

    if( m_cSpsInfoFileHandle.fail() )
    {
      printf("\nfailed writing SPS info file\n");
      exit(0);
    }
  }

  // create extractor class
  m_cTExtrTop.create();
}

Void TAppExtrTop::xDestroyExtrLib()
{
  // close bitstream files
  m_cTVideoIOInputBitstreamFile.closeBits();
  if ( m_pchOutputBitstreamFile )
  {
     m_cTVideoIOOutputBitstreamFile.closeBits();
  }

  // open text file
  if ( m_pchSpsInfoFile )
  {
     m_cSpsInfoFileHandle.close();
  }

  // destroy extractor class
  m_cTExtrTop.destroy();
}

Void TAppExtrTop::xInitExtrLib()
{
  // initialize extractor class
  m_cTExtrTop.init();
}

Void TAppExtrTop::xCopyInputPacketIntoOutput()
{
  UInt  uiNumBytes = m_apcInputBitstream->reinitParsing();
  UInt  uiByteCount;
  UInt  uiByte;

  assert( m_apcOutputBitstream );

  m_apcOutputBitstream->rewindStreamPacket();
  m_apcOutputBitstream->resetBits();

  for ( uiByteCount = 0; uiByteCount < uiNumBytes; uiByteCount++ )
  {
    m_apcInputBitstream->read( 8, uiByte );
    m_apcOutputBitstream->write( uiByte, 8 );
  }

  m_apcOutputBitstream->flushBuffer();
}
