

#include <time.h>
#include "TAppRendererTop.h"

// ====================================================================================================================
// Main function
// ====================================================================================================================

int main(int argc, char* argv[])
	{
  TAppRendererTop  cTAppRendererTop;

  // print information
  fprintf( stdout, "\n" );
  fprintf( stdout, "Multiview plus Depth Renderer: Renderer Version [%s]", NV_VERSION );
  fprintf( stdout, NVM_ONOS );
  fprintf( stdout, NVM_COMPILEDBY );
  fprintf( stdout, NVM_BITS );
  fprintf( stdout, "\n" );

  // create application renderer class
  cTAppRendererTop.create();

  // parse configuration
  if(!cTAppRendererTop.parseCfg( argc, argv ))
  {
    cTAppRendererTop.destroy();
    return 1;
  }

  // starting time
  double dResult;
  long lBefore = clock();

  // call rendering function
  cTAppRendererTop.go();

  // ending time
  dResult = (double)(clock()-lBefore) / CLOCKS_PER_SEC;
  printf("\n Total Time: %12.3f sec.\n", dResult);

  // destroy application renderer class
  cTAppRendererTop.destroy();

  return 0;
}


