

#include <list>
#include <stdio.h>
#include <fcntl.h>
#include <assert.h>



#ifndef __TAppRENDERERCFG__
#define __TAppRENDERERCFG__

#include "../TAppCommon/TAppComCamPara.h"
#include "../../Lib/TLibCommon/CommonDef.h"
#include "../../Lib/TLibCommon/TComMVDRefData.h"
#include "../../Lib/TLibRenderer/TRenModel.h"
#include "../../Lib/TLibRenderer/TRenModSetupStrParser.h"
#include <string>
#include <vector>

// ====================================================================================================================
// Class definition
// ====================================================================================================================

/// encoder configuration class
class TAppRendererCfg
{
protected:

  //// file I/O ////
  Char*              m_pchVideoInputFileBaseName;      ///< input video  file base name, placeholder for numbering $$
  Char*              m_pchDepthInputFileBaseName;      ///< input depth  file base name, placeholder for numbering $$
  Char*              m_pchSynthOutputFileBaseName;     ///< output synth file base name, placeholder for numbering $$
  Bool               m_bContOutputFileNumbering;       ///< use continous numbering instead of view numbering
  Bool               m_bSweep;                         ///< 1: Store view range in file

  // derived
  std::vector<Char*> m_pchVideoInputFileList;          ///< source file names
  std::vector<Char*> m_pchDepthInputFileList;          ///< source depth file names
  std::vector<Char*> m_pchSynthOutputFileList;         ///< output reconstruction file names

  //// source specification ////
  Int                m_iSourceWidth;                   ///< source width in pixel
  Int                m_iSourceHeight;                  ///< source height in pixel
  Int                m_iFrameSkip;                     ///< number of skipped frames from the beginning
  Int                m_iFramesToBeRendered;            ///< number of rendered frames

  ////camera specification ////
  Char*               m_pchCameraParameterFile;         ///< camera parameter file
  Char*               m_pchSynthViewCameraNumbers;            ///< numbers of views to synthesize
  Char*               m_pchViewConfig;                  ///< String to setup renderer
  Char*               m_pchBaseViewCameraNumbers;             ///< numbers of base views

  // derived
  TAppComCamPara      m_cCameraData;                    ///< class to store camera parameters
  TRenModSetupStrParser m_cRenModStrParser;             ///< class to manage View to be rendered
  Bool                m_bUseSetupString;                ///< true if setup string is used

  Int                 m_iNumberOfInputViews;						///< number of input Views
  Int                 m_iNumberOfOutputViews;						///< number views to synthesize

  //// renderer Modes ////
  Int                 m_iRenderDirection;                  ///< 0: interpolate, 1: extrapolate from left, 2: extrapolate from right

  Int                 m_iLog2SamplingFactor;            ///< factor for horizontal upsampling before processing
  Bool                m_bUVUp;                          ///< upsampling of chroma planes before processing
  Int                 m_iPreProcMode;                   ///< depth pre-processing: 0 = none, 1 = binominal filtering
  Int                 m_iPreFilterSize;                 ///< for PreProcMode = 1: size of filter kernel
  Bool                m_bSimEnhance;                    ///< Similarity enhancement before blending
  Int                 m_iBlendMode;                     ///< merging of left and right image: 0 = average, 1 = only holes from right, 2 = only holes from left
  Int                 m_iBlendZThresPerc;               ///< z-difference threshold for blending in percent of total z-range
  Bool                m_bBlendUseDistWeight;            ///< 0: blend using average; 1: blend factor depends on view distance
  Int                 m_iBlendHoleMargin;               ///< Margin around boundaries
  Bool                m_bTempDepthFilter;               ///< Zheijang temporal enhancement filter
  Int                 m_iInterpolationMode;             ///< 0: NN, 1: linear, 2: cspline
  Int                 m_iHoleFillingMode;               ///< 0: none, 1: line wise background extension
  Int                 m_iPostProcMode;                  ///< 0: none, 1: horizontal 3-tap median
  Int                 m_iRenderMode;                      ///< 0: use renderer
  Int                 m_iShiftPrecision;                ///< Precision used for Interpolation Mode 4
  Int                 m_iUsedPelMapMarExt;              ///< Used Pel map extra margin

  Void xCheckParameter ();                              ///< check validity of configuration values
  Void xPrintParameter ();                              ///< print configuration values
  Void xPrintUsage     ();                              ///< print usage
  Void xSetGlobal();

  Void xCreateFileNames();
  Void xGetMaxPrecision( std::vector< Int > adIn, Int& iPrecBefore, Int& iPrecAfter );
  Void xAddNumberToFileName( Char* pchSourceFileName, Char*& rpchTargetFileName, Int iNumberToAdd, UInt uiPrecBefore, UInt uiPrecAfter );
public:
  TAppRendererCfg();
  virtual ~TAppRendererCfg();

public:
  Void  create    ();                                         ///< create option handling class
  Void  destroy   ();                                         ///< destroy option handling class
  Bool  parseCfg  ( Int argc, Char* argv[] );                 ///< parse configuration file to fill member variables
  Bool  xConfirmParameter(Bool bflag, const Char* message);


};// END CLASS DEFINITION TAppRendererCfg



#endif // __TAppRENDERERCFG__

