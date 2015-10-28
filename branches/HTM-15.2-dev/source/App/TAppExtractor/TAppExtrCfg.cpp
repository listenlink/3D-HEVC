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

/** \file     TAppExtrCfg.cpp
    \brief    Extractor configuration class
*/

#include <cstdio>
#include <cstring>
#include <string>
#include "TAppExtrCfg.h"
#include "../../Lib/TAppCommon/program_options_lite.h"
#include <stdio.h>
#include <stdlib.h>

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
Bool TAppExtrCfg::parseCfg( Int argc, TChar* argv[] )
{
  bool do_help = false;
  string cfg_InputBitstreamFile;
  string cfg_OutputBitstreamFile;
  string cfg_SpsInfoFile;
  string cfg_ExtractLayerIds;

  po::Options opts;
  opts.addOptions()
  ("help",                   do_help,                   false,      "this help text")
  ("InputBitstreamFile,i",   cfg_InputBitstreamFile,    string(""), "bitstream input file name")
  ("OutputBitstreamFile,o",  cfg_OutputBitstreamFile,   string(""), "bitstream output file name")
  ("SpsInfoFile,s",          cfg_SpsInfoFile,           string(""), "SPS info file name")
  ("ExtractLayerIds,e",      cfg_ExtractLayerIds,       string(""), "comma-separated list of layer IDs for extraction")
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
  m_pchInputBitstreamFile = cfg_InputBitstreamFile.empty() ? NULL : strdup(cfg_InputBitstreamFile.c_str());
  m_pchOutputBitstreamFile = cfg_OutputBitstreamFile.empty() ? NULL : strdup(cfg_OutputBitstreamFile.c_str());
  m_pchSpsInfoFile = cfg_SpsInfoFile.empty() ? NULL : strdup(cfg_SpsInfoFile.c_str());

  xSplitUIntString( cfg_ExtractLayerIds, m_suiExtractLayerIds );

  if (!m_pchInputBitstreamFile)
  {
    fprintf(stderr, "No input file specifed, aborting\n");
    return false;
  }

  if (!m_pchOutputBitstreamFile && !m_pchSpsInfoFile)
  {
    fprintf(stderr, "No output file specifed, aborting\n");
    return false;
  }

  return true;
}

Void TAppExtrCfg::xSplitUIntString( const std::string& rString, std::set<UInt>& rSet )
{
  char* cString = NULL;
  char* cElement = NULL;

  cString = new char[rString.size()+1];
  strcpy(cString, rString.c_str());

  cElement = strtok( cString, "," );
  while ( cElement != NULL )
  {
     rSet.insert( atoi( cElement ) );
     cElement = strtok( NULL, "," );
  }

  delete cString;
}

