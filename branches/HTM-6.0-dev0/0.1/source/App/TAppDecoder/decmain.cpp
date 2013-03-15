

/** \file     decmain.cpp
    \brief    Decoder application main
*/

#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include "TAppDecTop.h"

bool g_md5_mismatch = false; ///< top level flag that indicates if there has been a decoding mismatch

// ====================================================================================================================
// Main function
// ====================================================================================================================

int main(int argc, char* argv[])
{
  TAppDecTop  cTAppDecTop;

  // print information
  fprintf( stdout, "\n" );
  fprintf( stdout, "HM %s based Multiview Coder: Decoder Version [%s]", HM_VERSION, NV_VERSION );
  fprintf( stdout, NVM_ONOS );
  fprintf( stdout, NVM_COMPILEDBY );
  fprintf( stdout, NVM_BITS );
  fprintf( stdout, "\n" );

  // create application decoder class
  cTAppDecTop.create();

  // parse configuration
  if(!cTAppDecTop.parseCfg( argc, argv ))
  {
    cTAppDecTop.destroy();
    return 1;
  }

  // starting time
  double dResult;
  long lBefore = clock();

  // call decoding function
  cTAppDecTop.decode();

  if (g_md5_mismatch)
  {
    printf("\n\n***ERROR*** A decoding mismatch occured: signalled md5sum does not match\n");
  }

  // ending time
  dResult = (double)(clock()-lBefore) / CLOCKS_PER_SEC;
  printf("\n Total Time: %12.3f sec.\n", dResult);

  // destroy application decoder class
  cTAppDecTop.destroy();

  return g_md5_mismatch ? EXIT_FAILURE : EXIT_SUCCESS;
}


