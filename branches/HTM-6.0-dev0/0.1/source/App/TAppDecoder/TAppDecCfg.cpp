

/** \file     TAppDecCfg.cpp
    \brief    Decoder configuration class
*/

#include <cstdio>
#include <cstring>
#include <string>
#include "TAppDecCfg.h"
#include "../../App/TAppCommon/program_options_lite.h"

#ifdef WIN32
#define strdup _strdup
#endif

using namespace std;
namespace po = df::program_options_lite;

// ====================================================================================================================
// Public member functions
// ====================================================================================================================

/** \param argc number of arguments
    \param argv array of arguments
 */
Bool TAppDecCfg::parseCfg( Int argc, Char* argv[] )
{
  bool do_help = false;
  string cfg_BitstreamFile;
  string cfg_ReconFile;
  string cfg_ScaleOffsetFile;

  po::Options opts;
  opts.addOptions()
  ("help", do_help, false, "this help text")
  ("BitstreamFile,b", cfg_BitstreamFile, string(""), "bitstream input file name")
  ("ReconFile,o",     cfg_ReconFile,     string(""), "reconstructed YUV output file name\n"
                                                     "YUV writing is skipped if omitted")
  ("ScaleOffsetFile,p", cfg_ScaleOffsetFile, string(""), "file with coded scales and offsets")
#if DCM_SKIP_DECODING_FRAMES
  ("SkipFrames,s", m_iSkipFrame, 0, "number of frames to skip before random access")
#endif
  ("OutputBitDepth,d", m_outputBitDepth, 0u, "bit depth of YUV output file (use 0 for native depth)")
  ("SEIpictureDigest", m_pictureDigestEnabled, false, "Control handling of picture_digest SEI messages\n"
                                              "\t1: check\n"
                                              "\t0: ignore")
  ;

  po::setDefaults(opts);
  const list<const char*>& argv_unhandled = po::scanArgv(opts, argc, (const char**) argv);

  for (list<const char*>::const_iterator it = argv_unhandled.begin(); it != argv_unhandled.end(); it++)
  {
    fprintf(stderr, "Unhandled argument ignored: `%s'\n", *it);
  }

  if (argc == 1 || do_help)
  {
    po::doHelp(cout, opts);
    return false;
  }

  /* convert std::string to c string for compatability */
  m_pchBitstreamFile = cfg_BitstreamFile.empty() ? NULL : strdup(cfg_BitstreamFile.c_str());
  m_pchReconFile = cfg_ReconFile.empty() ? NULL : strdup(cfg_ReconFile.c_str());
  m_pchScaleOffsetFile = cfg_ScaleOffsetFile.empty() ? NULL : strdup(cfg_ScaleOffsetFile.c_str());

  if (!m_pchBitstreamFile)
  {
    fprintf(stderr, "No input file specifed, aborting\n");
    return false;
  }

  return true;
}

Void TAppDecCfg::xAppendToFileNameEnd( Char* pchInputFileName, const Char* pchStringToAppend, Char*& rpchOutputFileName)
{
  size_t iInLength     = strlen(pchInputFileName);
  size_t iAppendLength = strlen(pchStringToAppend); 

  rpchOutputFileName = (Char*) malloc(iInLength+iAppendLength+1);												
  Char* pCDot = strrchr(pchInputFileName,'.'); 				
  pCDot = pCDot ? pCDot : pchInputFileName + iInLength;				
  size_t iCharsToDot = pCDot - pchInputFileName ; 
  size_t iCharsToEnd = iInLength - iCharsToDot; 				
  strncpy(rpchOutputFileName                            ,  pchInputFileName            , iCharsToDot  );
  strncpy(rpchOutputFileName+ iCharsToDot               ,  pchStringToAppend           , iAppendLength); 
  strncpy(rpchOutputFileName+ iCharsToDot+iAppendLength ,  pchInputFileName+iCharsToDot, iCharsToEnd  );				
  rpchOutputFileName[iInLength+iAppendLength] = '\0'; 				 
}
