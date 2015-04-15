///* The copyright in this software is being made available under the BSD
// * License, included below. This software may be subject to other third party
// * and contributor rights, including patent rights, and no such rights are
// * granted under this license.  
// *
// * Copyright (c) 2010-2015, ITU/ISO/IEC
// * All rights reserved.
// *
// * Redistribution and use in source and binary forms, with or without
// * modification, are permitted provided that the following conditions are met:
// *
// *  * Redistributions of source code must retain the above copyright notice,
// *    this list of conditions and the following disclaimer.
// *  * Redistributions in binary form must reproduce the above copyright notice,
// *    this list of conditions and the following disclaimer in the documentation
// *    and/or other materials provided with the distribution.
// *  * Neither the name of the ISO/IEC nor the names of its contributors may
// *    be used to endorse or promote products derived from this software without
// *    specific prior written permission.
// *
// * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS
// * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
// * THE POSSIBILITY OF SUCH DAMAGE.
// */
//
///** \file     TAppExtrTop.cpp
//    \brief    Extractor application class
#include "TAppExtrTop.h"
#include "../../Lib/TLibDecoder/AnnexBread.h"
#include <fstream>
#include <list>
#include <stdio.h>
#include <fcntl.h>
#include <assert.h>

#if H_MV
// ====================================================================================================================
// Constructor / destructor / initialization / destroy
// ====================================================================================================================

TAppExtrTop::TAppExtrTop()
{
  // To suppress compiler warnings on potential division by 0. 
  g_uiMaxCUWidth  = 1;
  g_uiMaxCUHeight = 1;
}

TAppExtrTop::~TAppExtrTop()
{
}

// ====================================================================================================================
// Public member functions
// ====================================================================================================================

//
//until the end of the bitstream, call extraction function in TExtrTop class
//

Void TAppExtrTop::extract()
{

  ifstream inputBitstreamFile( m_pchInputBitstreamFile, ifstream::in | ifstream::binary );
  if( inputBitstreamFile.fail() )
  {
    fprintf( stderr, "\nfailed to open bitstream file `%s' for reading\n", m_pchInputBitstreamFile );
    exit( EXIT_FAILURE );
  }

  fstream outputBitstreamFile;
  if( m_pchOutputBitstreamFile )
  {
    outputBitstreamFile.open( m_pchOutputBitstreamFile, fstream::binary | fstream::out );
    if( outputBitstreamFile.fail() )
    {
      fprintf( stderr, "\nfailed to open bitstream file `%s' for writing\n", m_pchOutputBitstreamFile );
      exit( EXIT_FAILURE );
    }
  }

  InputByteStream inputBytestream( inputBitstreamFile );

  Bool bEndOfFile = false;
  while( !bEndOfFile )
  {
    streampos location = inputBitstreamFile.tellg();
    AnnexBStats stats = AnnexBStats();
    vector<uint8_t> nalUnit;
    InputNALUnit nalu;
    bEndOfFile = byteStreamNALUnit( inputBytestream, nalUnit, stats );

    // handle NAL unit
    if( nalUnit.empty() )
    {
      /* this can happen if the following occur:
       *  - empty input file
       *  - two back-to-back start_code_prefixes
       *  - start_code_prefix immediately followed by EOF
       */
      fprintf( stderr, "Warning: Attempt to decode an empty NAL unit\n" );
    }
    else
    {
      read( nalu, nalUnit );

      // decide whether to extract packet or not
      if ( m_cTExtrTop.extract( nalu, m_suiExtractLayerIds ) && outputBitstreamFile.is_open() )
      {
        inputBitstreamFile.clear();

        streampos location2 = inputBitstreamFile.tellg();
        inputBitstreamFile.seekg( location );

        do
        {
          outputBitstreamFile.put( inputBitstreamFile.get() );
        } while( inputBitstreamFile.tellg() != location2 );
      }
    }
  }

  inputBitstreamFile.close();
  outputBitstreamFile.close();


  // write SPS info file
  if ( m_pchSpsInfoFile )
  {
    fstream cSpsInfoFileHandle( m_pchSpsInfoFile, fstream::binary | fstream::out );

    if( cSpsInfoFileHandle.fail() )
    {
      fprintf( stderr, "\nfailed writing SPS info file\n" );
      exit( EXIT_FAILURE );
    }

    m_cTExtrTop.dumpSpsInfo( cSpsInfoFileHandle );

    cSpsInfoFileHandle.close();
  }

  m_cTExtrTop.dumpVpsInfo( std::cout ); 
  m_cTExtrTop.dumpSpsInfo( std::cout );

}
#endif
