/* The copyright in this software is being made available under the BSD
 * License, included below. This software may be subject to other third party
 * and contributor rights, including patent rights, and no such rights are
 * granted under this license.
 *
 * Copyright (c) 2010-2015, ITU/ISO/IEC
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
 *  * Neither the name of the ITU/ISO/IEC nor the names of its contributors may
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

/** \file     decmain.cpp
    \brief    Decoder application main
*/

#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include "TAppDecTop.h"

//! \ingroup TAppDecoder
//! \{

// ====================================================================================================================
// Main function
// ====================================================================================================================

int main(int argc, char* argv[])
{
  Int returnCode = EXIT_SUCCESS;
#if !NH_MV
  TAppDecTop  cTAppDecTop;
#endif
  // print information
  fprintf( stdout, "\n" );
#if NH_MV
  fprintf( stdout, "3D-HTM Software: Decoder Version [%s] based on HM Version [%s]", NV_VERSION, HM_VERSION );  
#else
  fprintf( stdout, "HM software: Decoder Version [%s] (including RExt)", NV_VERSION );
#endif
  fprintf( stdout, NVM_ONOS );
  fprintf( stdout, NVM_COMPILEDBY );
  fprintf( stdout, NVM_BITS );
  fprintf( stdout, "\n" );

#if NH_MV
  Int numDecodings = 1; 
  Int curDecoding  = 0; 
  Double dResult = 0; 
  do {
    TAppDecTop  cTAppDecTop;
#endif
    // create application decoder class
    cTAppDecTop.create();

    // parse configuration
    if(!cTAppDecTop.parseCfg( argc, argv ))
    {
      cTAppDecTop.destroy();
      returnCode = EXIT_FAILURE;
      return returnCode;
    }
#if NH_MV
    numDecodings = cTAppDecTop.getNumDecodings(); 
#endif

    // starting time    
#if !NH_MV
  Double dResult;
#endif
    clock_t lBefore = clock();

    // call decoding function
#if NH_MV
    cTAppDecTop.decode( curDecoding );
#else
    cTAppDecTop.decode();
#endif

    if (cTAppDecTop.getNumberOfChecksumErrorsDetected() != 0)
    {
      printf("\n\n***ERROR*** A decoding mismatch occured: signalled md5sum does not match\n");
      returnCode = EXIT_FAILURE;
    }

    // ending time
#if NH_MV
    dResult += (double)(clock()-lBefore) / CLOCKS_PER_SEC;
#else
  dResult = (Double)(clock()-lBefore) / CLOCKS_PER_SEC;
  printf("\n Total Time: %12.3f sec.\n", dResult);
#endif

    // destroy application decoder class
    cTAppDecTop.destroy();
#if NH_MV
    curDecoding++; 
  } 
  while ( curDecoding < numDecodings );

  printf("\n Total Time: %12.3f sec.\n", dResult);
#endif
  return returnCode;
}

//! \}
