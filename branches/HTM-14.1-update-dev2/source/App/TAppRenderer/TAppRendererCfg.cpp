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




#include <stdlib.h>
#include <math.h>
#include <cassert>
#include <cstring>
#include <string>

#include "TAppRendererCfg.h"
#include "../../Lib/TAppCommon/program_options_lite.h"

#if NH_3D

using namespace std;
namespace po = df::program_options_lite;

// ====================================================================================================================
// Local constants
// ====================================================================================================================

#define MAX_INPUT_VIEW_NUM          10
#define MAX_OUTPUT_VIEW_NUM         64

// ====================================================================================================================
// Constructor / destructor / initialization / destroy
// ====================================================================================================================


TAppRendererCfg::TAppRendererCfg()
{
  
}

TAppRendererCfg::~TAppRendererCfg()
{
  for(Int i = 0; i< m_pchVideoInputFileList.size(); i++ )
  {
    if ( m_pchVideoInputFileList[i] != NULL )
      free (m_pchVideoInputFileList[i]);
  }

  for(Int i = 0; i< m_pchDepthInputFileList.size(); i++ )
  {
    if ( m_pchDepthInputFileList[i] != NULL )
      free (m_pchDepthInputFileList[i]);
  }

  for(Int i = 0; i< m_pchSynthOutputFileList.size(); i++ )
  {
    if ( m_pchSynthOutputFileList[i] != NULL )
      free (m_pchSynthOutputFileList[i]);
  }

  if ( m_pchVideoInputFileBaseName  ) free( m_pchVideoInputFileBaseName );
  if ( m_pchDepthInputFileBaseName  ) free( m_pchDepthInputFileBaseName );
  if ( m_pchSynthOutputFileBaseName ) free( m_pchSynthOutputFileBaseName);
  if ( m_pchCameraParameterFile     ) free( m_pchCameraParameterFile    );
  if ( m_pchBaseViewCameraNumbers   ) free( m_pchBaseViewCameraNumbers  );
  if ( m_pchSynthViewCameraNumbers  ) free( m_pchSynthViewCameraNumbers );
  if ( m_pchViewConfig              ) free( m_pchViewConfig         );
}

Void TAppRendererCfg::create()
{
}

Void TAppRendererCfg::destroy()
{
}

// ====================================================================================================================
// Public member functions
// ====================================================================================================================

/** \param  argc        number of arguments
\param  argv        array of arguments
\retval             true when success
*/
Bool TAppRendererCfg::parseCfg( Int argc, Char* argv[] )
{
  bool do_help = false;

  po::Options opts;
  opts.addOptions()
    ("help", do_help, false, "this help text")
    ("c", po::parseConfigFile, "configuration file name")

    /* File I/O */
    ("VideoInputFileBaseName,v",  m_pchVideoInputFileBaseName,  (Char*) 0, "Basename to generate video input file names")
    ("DepthInputFileBaseName,d",  m_pchDepthInputFileBaseName,  (Char*) 0, "Basename to generate depth input file names")
    ("SynthOutputFileBaseName,s", m_pchSynthOutputFileBaseName, (Char*) 0, "Basename to generate synthesized output file names")
    ("ContOutputFileNumbering", m_bContOutputFileNumbering  ,  false   , "Continuous Output File Numbering")
    ("Sweep"                  , m_bSweep                    ,  false   , "Store all views in first Output File")

    ("VideoInputFile_%d,v_%d",  m_pchVideoInputFileList ,    (Char *) 0, MAX_INPUT_VIEW_NUM , "Original Yuv video input file name %d")
    ("DepthInputFile_%d,d_%d",  m_pchDepthInputFileList ,    (Char *) 0, MAX_INPUT_VIEW_NUM , "Original Yuv depth input file name %d")
    ("SynthOutputFile_%d,s_%d", m_pchSynthOutputFileList,    (Char *) 0, MAX_OUTPUT_VIEW_NUM, "Synthesized Yuv output file name %d")

    ("InputBitDepth",           m_inputBitDepth[0],                     8, "Bit-depth of input file")
    ("OutputBitDepth",          m_outputBitDepth[0],                    0, "Bit-depth of output file (default:InternalBitDepth)")
    ("InternalBitDepth",        m_internalBitDepth[0],                  0, "Bit-depth the renderer operates at. (default:InputBitDepth)"                                                                          "If different to InputBitDepth, source data will be converted")

    ("InputBitDepthC",        m_inputBitDepth[1],    0, "As per InputBitDepth but for chroma component. (default:InputBitDepth)")
    ("OutputBitDepthC",       m_outputBitDepth[1],   0, "As per OutputBitDepth but for chroma component. (default:InternalBitDepthC)")
    ("InternalBitDepthC",     m_internalBitDepth[1], 0, "As per InternalBitDepth but for chroma component. (default:IntrenalBitDepth)")

    /* Source Specification */
    ("SourceWidth,-wdt",        m_iSourceWidth,                       0, "Source picture width")
    ("SourceHeight,-hgt",       m_iSourceHeight,                      0, "Source picture height")
    ("FrameSkip,-fs",           m_iFrameSkip,                         0, "Number of frames to skip at start of input YUV")
    ("FramesToBeRendered,f",    m_iFramesToBeRendered,                0, "Number of frames to be rendered (default=all)")

    /* Camera Specification */
    ("CameraParameterFile,-cpf", m_pchCameraParameterFile,          (Char *) 0, "Camera Parameter File Name")
    ("BaseViewCameraNumbers"  , m_pchBaseViewCameraNumbers,        (Char *) 0, "Numbers of base views")
    ("SynthViewCameraNumbers" , m_pchSynthViewCameraNumbers,       (Char *) 0, "Numbers of views to synthesis")
    ("ViewConfig"             , m_pchViewConfig,                   (Char *) 0, "View Configuration"               )

    /* Renderer Modes */
    ("Log2SamplingFactor",      m_iLog2SamplingFactor,                0, "Factor for horizontal up sampling before processing"     )
    ("UVup"              ,      m_bUVUp               ,            true, "Up sampling of chroma planes before processing"          )
    ("PreProcMode"       ,      m_iPreProcMode        ,               0, "Depth preprocessing: 0 = None, 1 = Binomial filtering"   )
    ("PreFilterSize"     ,      m_iPreFilterSize      ,               0, "For PreProcMode 1: Half Size of filter kernel"           )
    ("SimEnhance"        ,      m_bSimEnhance         ,           true, "Similarity enhancement of video" )
    ("BlendMode"         ,      m_iBlendMode          ,               0, "Blending of left and right image: 0: average, 1: only holes from right, 2: only holes from left, 3: first view in BaseViewOrder as main view" )
    ("BlendZThresPerc"   ,      m_iBlendZThresPerc    ,              30, "Z-difference threshold for blending in percent of total Z-range"   )
    ("BlendUseDistWeight",      m_bBlendUseDistWeight ,            true, "0: blend using average; 1: blend factor depends on view distance"  )
    ("BlendHoleMargin"   ,      m_iBlendHoleMargin    ,               6, "Margin around holes to fill with other view"                       )
    ("InterpolationMode" ,      m_iInterpolationMode  ,               4, "0: NN, 1:linear (int), 2:linear (double) , 3:cubic Hermite spline (double), 4: 8-tap (int)" )
    ("HoleFillingMode"   ,      m_iHoleFillingMode    ,               1, "0: None, 1: line wise background extension"              )
    ("PostProcMode"      ,      m_iPostProcMode       ,               0, "0: None, 1: horizontal 3-tap median"                     )
    ("RenderMode"        ,      m_iRenderMode         ,               0, "0: Use renderer, 1: use model renderer, 10: create used pels map")
    ("ShiftPrecision"    ,      m_iShiftPrecision     ,               2, "Shift Precision for Interpolation Mode 4"                )
    ("TemporalDepthFilter",     m_bTempDepthFilter    ,           false, "Temporal depth filtering"                                )
    ("RenderDirection"   ,      m_iRenderDirection    ,               0, "0: Interpolate, 1: Extrapolate from left, 2: Extrapolate from right")
    ("UsedPelMapMarExt"  ,      m_iUsedPelMapMarExt   ,               0, "Margin Extension in Pels for used pels map generation"   );

  po::setDefaults(opts);
  po::scanArgv(opts, argc, (const char**) argv);

  if (argc == 1 || do_help)
  {
    /* argc == 1: no options have been specified */
    po::doHelp(cout, opts);
    xPrintUsage();
    return false;
  }

  /*
  * Set any derived parameters before checking
  */

  /* rules for input, output and internal bitdepths as per help text */
  if (!m_internalBitDepth[0]) { m_internalBitDepth[0] = m_inputBitDepth[0]; }
  if (!m_internalBitDepth[1]) { m_internalBitDepth[1] = m_internalBitDepth[0]; }
  if (!m_inputBitDepth[1])    { m_inputBitDepth[1]    = m_inputBitDepth[0]; }
  if (!m_outputBitDepth[0])   { m_outputBitDepth[0]   = m_internalBitDepth[0]; }
  if (!m_outputBitDepth[1])   { m_outputBitDepth[1]   = m_internalBitDepth[1]; }

  UInt  uiInputBitDepth   = 8;
  UInt  uiCamParPrecision = 5;

  m_bUseSetupString = ( m_pchViewConfig != NULL ) && ( m_iRenderMode != 0);

  if ( m_iRenderMode == 10 )
  {
    m_cCameraData.init( MAX_INPUT_VIEW_NUM, uiInputBitDepth, uiCamParPrecision, (UInt)m_iFrameSkip, (UInt)m_iFramesToBeRendered,
      m_pchCameraParameterFile, m_pchBaseViewCameraNumbers, NULL, NULL, m_iLog2SamplingFactor+m_iShiftPrecision );
    m_iNumberOfInputViews  = (Int) m_cCameraData.getBaseViewNumbers() .size();
    m_iNumberOfOutputViews = m_iNumberOfInputViews - 1;
    m_iRenderDirection     = 1;
  }
  else
  {
  if ( m_bUseSetupString )
  {
    std::vector<Int>  iaTempViews;
    std::vector<Int>* piaTempViews;
    m_cCameraData     .convertNumberString( m_pchBaseViewCameraNumbers, iaTempViews, VIEW_NUM_PREC );
    m_cRenModStrParser.setString( (Int) iaTempViews.size(), m_pchViewConfig );
    piaTempViews               = m_cRenModStrParser.getSynthViews();
    m_iNumberOfOutputViews     = (Int) m_cRenModStrParser.getNumOfModels();
    m_iNumberOfInputViews      = (Int) m_cRenModStrParser.getNumOfBaseViews();
    m_bContOutputFileNumbering = true;

  m_cCameraData.init( MAX_INPUT_VIEW_NUM, uiInputBitDepth, uiCamParPrecision, (UInt)m_iFrameSkip, (UInt)m_iFramesToBeRendered,
      m_pchCameraParameterFile, m_pchBaseViewCameraNumbers, NULL, piaTempViews, m_iLog2SamplingFactor+m_iShiftPrecision );
  }
  else
  {
  m_cCameraData.init( MAX_INPUT_VIEW_NUM, uiInputBitDepth, uiCamParPrecision, (UInt)m_iFrameSkip, (UInt)m_iFramesToBeRendered,
      m_pchCameraParameterFile, m_pchBaseViewCameraNumbers, m_pchSynthViewCameraNumbers, NULL, m_iLog2SamplingFactor+m_iShiftPrecision );
  m_iNumberOfOutputViews = (Int) m_cCameraData.getSynthViewNumbers().size();
  m_iNumberOfInputViews  = (Int) m_cCameraData.getBaseViewNumbers() .size();
  }
  }

  if (m_pchSynthOutputFileBaseName != NULL)
    xConfirmParameter( strrchr(m_pchSynthOutputFileBaseName,'$')  == 0, "'$' must be a character in SynthOutputFileBaseName");

  if (m_pchDepthInputFileBaseName != NULL)
    xConfirmParameter( strrchr(m_pchDepthInputFileBaseName, '$')  == 0, "'$' must be a character in DepthInputFileBaseName" );

  if (m_pchVideoInputFileBaseName != NULL)
    xConfirmParameter( strrchr(m_pchVideoInputFileBaseName, '$')  == 0, "'$' must be a character in VideoInputFileBaseName" );

  xCreateFileNames();

  /*
  * check validity of input parameters
  */
  xCheckParameter();
  m_cCameraData.check( m_iRenderDirection == 0, m_iFramesToBeRendered != 0 );

  // print-out parameters
  xPrintParameter();

  return true;
}


// ====================================================================================================================
// Private member functions
// ====================================================================================================================

Void TAppRendererCfg::xCheckParameter()
{
  bool check_failed = false; /* abort if there is a fatal configuration problem */
#define xConfirmPara(a,b) check_failed |= xConfirmParameter(a,b)
  // check range of parameters

  /// File I/O

  // source specification
  xConfirmPara( m_iSourceWidth        <= 0,                   "Source width  must be greater than 0" );
  xConfirmPara( m_iSourceHeight       <= 0,                   "Source height must be greater than 0" );
  xConfirmPara( m_iFrameSkip          <  0,                   "Frame Skipping must be more than or equal to 0" );
  xConfirmPara( m_iFramesToBeRendered <  0,                   "Total Number Of Frames rendered must be more than 1" );

  // bit depth 
  xConfirmPara( m_internalBitDepth[0] != m_internalBitDepth[1],  "InternalBitDepth for luma and chroma must be equal. "); 
  xConfirmPara( m_inputBitDepth[0] < 8,                        "InputBitDepth must be at least 8" );
  xConfirmPara( m_inputBitDepth[1] < 8,                        "InputBitDepthC must be at least 8" );

  // camera specification
  xConfirmPara( m_iNumberOfInputViews  > MAX_INPUT_VIEW_NUM , "NumberOfInputViews must be less than of equal to MAX_INPUT_VIEW_NUM");
  xConfirmPara( m_iNumberOfOutputViews > MAX_OUTPUT_VIEW_NUM, "NumberOfOutputViews must be less than of equal to MAX_OUTPUT_VIEW_NUM");


  xConfirmPara( m_iRenderDirection < 0 || m_iRenderDirection > 2  , "RenderDirection must be greater than or equal to 0 and less than 3");
  xConfirmPara(m_iNumberOfOutputViews < 1,                    "Number of OutputViews must be greater or equal to 1");
  if ( m_iRenderDirection == 0 )
  {
    xConfirmPara( m_iNumberOfInputViews < 2,                  "Number of InputViews must be more than or equal to 2");
  }
  else
  {
    xConfirmPara( m_iNumberOfInputViews < 1,                  "Number of InputViews must be more than or equal to 1");
  }

  xConfirmPara( m_iLog2SamplingFactor < 0 || m_iLog2SamplingFactor >  4, "Log2SamplingFactor must be more than or equal to 0 and less than 5"  );
  xConfirmPara( m_iPreProcMode        < 0 || m_iPreProcMode        >  1, "PreProcMode        must be more than or equal to 0 and less than 2"  );


  xConfirmPara( m_iPreFilterSize      < 0 || m_iPreFilterSize      >  3, "PreFilterSize      must be more than or equal to 0 and less than 4" );
  xConfirmPara( m_iBlendMode          < 0 || m_iBlendMode          >  3, "BlendMode          must be more than or equal to 0 and less than 4"  );
  xConfirmPara( m_iBlendZThresPerc    < 0 || m_iBlendZThresPerc    > 100,"BlendZThresPerc    must be more than or equal to 0 and less than 101"  );
  xConfirmPara( m_iBlendHoleMargin    < 0 || m_iBlendHoleMargin    >  20,"BlendHoleMargin    must be more than or equal to 0 and less than 19"  );
  xConfirmPara( m_iInterpolationMode  < 0 || m_iInterpolationMode  >  4, "InterpolationMode  must be more than or equal to 0 and less than 5"  );
  xConfirmPara( m_iHoleFillingMode    < 0 || m_iHoleFillingMode    >  1, "HoleFillingMode    must be more than or equal to 0 and less than 2"  );
  xConfirmPara( m_iPostProcMode       < 0 || m_iPostProcMode       >  2, "PostProcMode       must be more than or equal to 0 and less than 3"  );

  Int iNumNonNULL;
  for (iNumNonNULL = 0; (iNumNonNULL < m_iNumberOfInputViews)  && m_pchDepthInputFileList[iNumNonNULL]; iNumNonNULL++);  xConfirmPara( iNumNonNULL < m_iNumberOfInputViews,  "Number of DepthInputFiles  must be greater than or equal to number of BaseViewNumbers" );
  for (iNumNonNULL = 0; (iNumNonNULL < m_iNumberOfInputViews)  && m_pchVideoInputFileList[iNumNonNULL]; iNumNonNULL++);  xConfirmPara( iNumNonNULL < m_iNumberOfInputViews,  "Number of DepthInputFiles  must be greater than or equal to number of BaseViewNumbers" );


  if ( !m_bSweep )
  {
  for (iNumNonNULL = 0; (iNumNonNULL < m_iNumberOfOutputViews) && m_pchSynthOutputFileList[iNumNonNULL]; iNumNonNULL++); xConfirmPara( iNumNonNULL < m_iNumberOfOutputViews, "Number of SynthOutputFiles must be greater than or equal to number of SynthViewNumbers" );
  }
  else
  {
      xConfirmPara( iNumNonNULL < 1, "Number of SynthOutputFiles must be equal to or more than 1" );
  }

#undef xConfirmPara
  if ( check_failed )
  {
    exit(EXIT_FAILURE);
  }

}



Void TAppRendererCfg::xPrintParameter()
{
  printf("\n");
  for( Int iCounter = 0; iCounter < m_iNumberOfInputViews; iCounter++)
  {
    printf("InputVideoFile_%i        : %s\n", iCounter, m_pchVideoInputFileList[iCounter]);
  }
  for( Int iCounter = 0; iCounter < m_iNumberOfInputViews; iCounter++)
  {
    printf("InputDepthFile_%i        : %s\n", iCounter, m_pchDepthInputFileList[iCounter]);
  }

  for( Int iCounter = 0; iCounter < m_iNumberOfOutputViews; iCounter++)
  {
    printf("SynthOutputFile_%i       : %s\n", iCounter, m_pchSynthOutputFileList[iCounter]);
  }

  printf("Format                  : %dx%d \n", m_iSourceWidth, m_iSourceHeight );
  printf("Frame index             : %d - %d (%d frames)\n", m_iFrameSkip, m_iFrameSkip+m_iFramesToBeRendered-1, m_iFramesToBeRendered);
  printf("CameraParameterFile     : %s\n", m_pchCameraParameterFile );
  printf("BaseViewNumbers         : %s  (%d views) \n", m_pchBaseViewCameraNumbers , m_iNumberOfInputViews  );
  printf("Sweep                   : %d\n", m_bSweep               );

  if ( m_bUseSetupString )
  {
    printf("ViewConfig              : %s\n", m_pchViewConfig );
  }
  else
  {
  printf("SynthViewNumbers        : %s  (%d views) \n", m_pchSynthViewCameraNumbers, m_iNumberOfOutputViews );
  }

  printf("Log2SamplingFactor      : %d\n", m_iLog2SamplingFactor );
  printf("UVUp                    : %d\n", m_bUVUp               );
  printf("PreProcMode             : %d\n", m_iPreProcMode        );
  printf("PreFilterSize           : %d\n", m_iPreFilterSize      );
  printf("SimEnhance              : %d\n", m_bSimEnhance         );
  printf("BlendMode               : %d\n", m_iBlendMode          );
  printf("BlendZThresPerc         : %d\n", m_iBlendZThresPerc    );
  printf("BlendUseDistWeight      : %d\n", m_bBlendUseDistWeight );
  printf("BlendHoleMargin         : %d\n", m_iBlendHoleMargin    );
  printf("InterpolationMode       : %d\n", m_iInterpolationMode  );
  printf("HoleFillingMode         : %d\n", m_iHoleFillingMode    );
  printf("PostProcMode            : %d\n", m_iPostProcMode       );
  printf("ShiftPrecision          : %d\n", m_iShiftPrecision     );
  printf("TemporalDepthFilter     : %d\n", m_bTempDepthFilter    );
  printf("RenderMode              : %d\n", m_iRenderMode         );
  printf("RendererDirection       : %d\n", m_iRenderDirection       );

  if (m_iRenderMode == 10 )
  {
    printf("UsedPelMapMarExt        : %d\n", m_iUsedPelMapMarExt );
  }

  printf("\n");

  //  printf("TOOL CFG: ");
  //  printf("ALF:%d ", m_bUseALF             );
  //  printf("\n");

  fflush(stdout);
}

Void TAppRendererCfg::xPrintUsage()
{
  printf( "\n" );
  printf( "  Example: TAppRenderer.exe -c test.cfg\n\n");
}

Bool TAppRendererCfg::xConfirmParameter(Bool bflag, const char* message)
{
  if (!bflag)
    return false;

  printf("Error: %s\n",message);
  return true;
}


Void TAppRendererCfg::xCreateFileNames()
{
  if ( m_iRenderMode == 10 )
    return;

  Int iPrecBefore;
  Int iPrecAfter;

  xGetMaxPrecision( m_cCameraData.getSynthViewNumbers(), iPrecBefore, iPrecAfter );


  if (iPrecBefore > LOG10_VIEW_NUM_PREC )
  {
    std::cerr << "Error: View Numbers with more than " << LOG10_VIEW_NUM_PREC << " digits are not supported" << std::endl;
    exit(EXIT_FAILURE);
  }

  AOT( !m_bContOutputFileNumbering && (m_cCameraData.getSynthViewNumbers().size() != m_iNumberOfOutputViews ));
  for(Int iIdx = 0; iIdx < m_iNumberOfOutputViews; iIdx++)
  {
    //GT; Create ReconFileNames
    if (m_pchSynthOutputFileList[iIdx] == NULL )
    {
      if ( m_bContOutputFileNumbering )
      {
        xAddNumberToFileName( m_pchSynthOutputFileBaseName, m_pchSynthOutputFileList[iIdx], (Int) ((iIdx+1) * VIEW_NUM_PREC) , 2, 0  );
      }
      else
      {
        xAddNumberToFileName( m_pchSynthOutputFileBaseName, m_pchSynthOutputFileList[iIdx], m_cCameraData.getSynthViewNumbers()[iIdx], iPrecBefore, iPrecAfter  );
      }
    }
  }

  xGetMaxPrecision( m_cCameraData.getBaseViewNumbers(), iPrecBefore, iPrecAfter );
  for(Int iIdx = 0; iIdx < m_cCameraData.getBaseViewNumbers().size() ; iIdx++)
  {
    //GT; Create ReconFileNames
    if (m_pchVideoInputFileList[iIdx] == NULL )
    {
      xAddNumberToFileName( m_pchVideoInputFileBaseName, m_pchVideoInputFileList[iIdx], m_cCameraData.getBaseViewNumbers()[iIdx], iPrecBefore, iPrecAfter  );
    }

    if (m_pchDepthInputFileList[iIdx] == NULL )
    {
      xAddNumberToFileName( m_pchDepthInputFileBaseName, m_pchDepthInputFileList[iIdx], m_cCameraData.getBaseViewNumbers()[iIdx], iPrecBefore, iPrecAfter  );
    }
  }
}

Void TAppRendererCfg::xAddNumberToFileName( Char* pchSourceFileName, Char*& rpchTargetFileName, Int iNumberToAdd, UInt uiPrecBefore, UInt uiPrecAfter )
{

  if ( pchSourceFileName == NULL )
  {
    std::cerr << "No BaseName for file name generation given." << std::endl;
    AOT(true);
    exit(EXIT_FAILURE);
  }

  Char pchNumberBuffer[2* LOG10_VIEW_NUM_PREC + 2];
  Char pchPrintBuffer[10];

  Double dNumberToAdd = ( (Double) iNumberToAdd ) / VIEW_NUM_PREC;

  UInt uiWidth = uiPrecBefore;

  if (uiPrecAfter != 0)
  {
    uiWidth += uiPrecAfter + 1;
  }

  sprintf( pchPrintBuffer, "%%0%d.%df", uiWidth, uiPrecAfter );
  sprintf( pchNumberBuffer, pchPrintBuffer, dNumberToAdd );

  if ( uiPrecAfter > 0 ) pchNumberBuffer[ uiPrecBefore ] = '_';

  size_t iInLength  = strlen(pchSourceFileName);
  size_t iAddLength = strlen(pchNumberBuffer);

  rpchTargetFileName = (Char*) malloc(iInLength+iAddLength+1);

  Char* pchPlaceHolder = strrchr(pchSourceFileName,'$');
  assert( pchPlaceHolder );

  size_t iCharsToPlaceHolder = pchPlaceHolder - pchSourceFileName;
  size_t iCharsToEnd         = iInLength      - iCharsToPlaceHolder;

  strncpy(rpchTargetFileName                               , pchSourceFileName                      , iCharsToPlaceHolder);
  strncpy(rpchTargetFileName+iCharsToPlaceHolder           , pchNumberBuffer                        , iAddLength         );
  strncpy(rpchTargetFileName+iCharsToPlaceHolder+iAddLength, pchSourceFileName+iCharsToPlaceHolder+1, iCharsToEnd-1      );
  rpchTargetFileName[iInLength+iAddLength-1] = '\0';
}

Void TAppRendererCfg::xGetMaxPrecision( IntAry1d aiIn, Int& iPrecBefore, Int& iPrecAfter )
{
  iPrecBefore = 0;
  iPrecAfter  = 0;

  for (UInt uiK = 0; uiK < aiIn.size(); uiK ++ )
  {
    if ( aiIn[uiK] == 0 ) continue;

    Int iCurPrec;
    iCurPrec = 0;
    for ( Int iCur = aiIn[uiK]; iCur != 0; iCur /= 10, iCurPrec++ );
    iPrecBefore = max(iPrecBefore, iCurPrec - LOG10_VIEW_NUM_PREC );

    iCurPrec = 0;
    for ( Int iCur = 1;  aiIn[uiK] % iCur == 0; iCur *= 10, iCurPrec++);
    iCurPrec = LOG10_VIEW_NUM_PREC - std::min((Int) LOG10_VIEW_NUM_PREC, iCurPrec-1 );
    iPrecAfter = max(iPrecAfter, iCurPrec );
  }
}

#endif
