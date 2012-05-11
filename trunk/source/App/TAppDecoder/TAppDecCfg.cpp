/* The copyright in this software is being made available under the BSD
 * License, included below. This software may be subject to other third party
 * and contributor rights, including patent rights, and no such rights are
 * granted under this license.  
 *
 * Copyright (c) 2010-2012, ITU/ISO/IEC
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

/** \file     TAppDecCfg.cpp
    \brief    Decoder configuration class
*/

#include <cstdio>
#include <cstring>
#include <string>
#include "TAppDecCfg.h"
#include "TAppCommon/program_options_lite.h"

#ifdef WIN32
#define strdup _strdup
#endif

using namespace std;
namespace po = df::program_options_lite;

//! \ingroup TAppDecoder
//! \{

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
  ("SkipFrames,s", m_iSkipFrame, 0, "number of frames to skip before random access")
  ("OutputBitDepth,d", m_outputBitDepth, 0u, "bit depth of YUV output file (use 0 for native depth)")
  ("MaxTemporalLayer,t", m_iMaxTemporalLayer, -1, "Maximum Temporal Layer to be decoded. -1 to decode all layers")
  ("SEIpictureDigest", m_pictureDigestEnabled, true, "Control handling of picture_digest SEI messages\n"
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

//! \}
