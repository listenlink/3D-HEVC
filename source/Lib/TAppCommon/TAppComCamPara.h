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



#ifndef __TAPPCOMCAMPARA__
#define __TAPPCOMCAMPARA__

// Include files
#include "../../Lib/TLibCommon/CommonDef.h"
#include <vector>


#define LOG10_VIEW_NUM_PREC         5  // Precision of view numbering
#define VIEW_NUM_PREC               pow( 10.0, (Int)LOG10_VIEW_NUM_PREC )

// ====================================================================================================================
// Class definition
// ====================================================================================================================

class TAppComCamPara
{
private:
  // camera parameter data
  std::vector< std::vector<Double> >  m_aadCameraParameters;  ///< buffer for double values from file

  // miscellaneous variables
  Double              m_dViewNumPrec;                         ///< factor for camera number to create integer representation
  Int                 m_iLog2Precision;                       ///< shift precision in LUT
  UInt                m_uiInputBitDepth;                      ///< bit depth of input depth maps
  UInt                m_uiBitDepthForLUT;                     ///< bit depth used for table look-up
  UInt                m_uiFirstFrameId;                       ///< first frame id
  UInt                m_uiLastFrameId;                        ///< last frame id
  UInt                m_iCurrentFrameId;                      ///< currently set frame id

  Bool                m_bSetupFromCoded;                      ///< setup from coded parameter file
  Bool                m_bCamParsCodedPrecSet;                 ///< Coded Cam Para precision set for current frame;
  
  //SAIT_VSO_EST_A0033
  Double              m_dDispCoeff;

  // view lists
  std::vector<Int>    m_aiViewsInCfgFile;                     ///< views for which parameters are specified in cfg file (from left to right)
  std::vector<Int>    m_aiSynthViews;                         ///< View numbers of External ViewReferences
  std::vector<Int>    m_aiRelSynthViewsNum;                   ///< Relative view numbers of External ViewReferences
  std::vector<Int>    m_aiBaseViews;                          ///< View numbers of Base View References (in coding order)
  std::vector<Int>    m_aiSortedBaseViews;                    ///< View numbers of Base View References (from left to right)
  std::vector<Int>    m_aiBaseId2SortedId;                    ///< mapping from coding order to left-right order for base views
  std::vector<Int>    m_aiBaseSortedId2Id;                    ///< mapping from left-right order to coding order for base views
  Int                 m_iNumberOfBaseViews;                   ///< number of base views
  Int                 m_iNumberOfSynthViews;                  ///< number of synthesized views

  // SPS and slice header related variables
  std::vector<Int>    m_aiViewOrderIndex;                     ///< list of view order indices
  UInt                m_uiCamParsCodedPrecision;              ///< precision for coding of camera parameters (x: max error in disparity is 2^(-x) luma samples)
  Bool                m_bCamParsVaryOverTime;                 ///< flag specifying whether camera parameters vary for given frame numbers
  Int**               m_aaiCodedScale;                        ///< array of coded scaling parameters [RefView][TargetView]
  Int**               m_aaiCodedOffset;                       ///< array of coded offset  parameters [RefView][TargetView]
  Int**               m_aaiScaleAndOffsetSet;                 ///< array indicating whether scale and offset have been set

#if H_3D_PDM_CAM_PARAS
  // parameters for virtual depth map generation
  Int                 m_iPdmPrecision;                        ///< additional precision for disparity - virtual depth conversion
  Int**               m_aaiPdmScaleNomDelta;                  ///< [TargetView][RefView] delta for nominator of scale factor
  Int**               m_aaiPdmOffset;                         ///< [TargetView][RefView] offset parameter
#endif

  // scale and offset parameters
  Double***           m_adBaseViewShiftParameter;             ///< ShiftParameters between BaseViews e.g. [2][1][0] shift scale from view 2 to view 1; [2][1][1] shift offset from view 2 to view 1
  Int64 ***           m_aiBaseViewShiftParameter;             ///< ShiftParameters between BaseViews e.g. [2][1][0] shift scale from view 2 to view 1; [2][1][1] shift offset from view 2 to view 1     /* do we need 64 bit? */
  Double***           m_adSynthViewShiftParameter;            ///< ShiftParameters between BaseViews and ERViews e.g. [2][1][0] shift scale from base view 2 to er view 1;
  Int64 ***           m_aiSynthViewShiftParameter;            ///< ShiftParameters between BaseViews and ERViews e.g. [2][1][0] shift scale from base view 2 to er view 1;    /* do we need 64 bit? */

  // look-up tables
  Double****          m_adBaseViewShiftLUT;                    ///< Disparity LUT
  Int****             m_aiBaseViewShiftLUT;                    ///< Disparity LUT
  Double****          m_adSynthViewShiftLUT;                   ///< Disparity LUT
  Int****             m_aiSynthViewShiftLUT;                   ///< Disparity LUT

protected:
  // create and delete arrays
  Void                    xCreateLUTs   ( UInt uiNumberSourceViews, UInt uiNumberTargetViews, Double****& radLUT, Int****& raiLUT, Double***& radShiftParams, Int64***& raiShiftParams );
  Void                    xCreate2dArray( UInt uiNum1Ids, UInt uiNum2Ids, Int**& raaiArray );
  Void                    xInit2dArray  ( UInt uiNum1Ids, UInt uiNum2Ids, Int**& raaiArray, Int iValue );
  template<class T> Void  xDeleteArray  ( T*& rpt, UInt uiSize1, UInt uiSize2, UInt uiSize3 );
  template<class T> Void  xDeleteArray  ( T*& rpt, UInt uiSize1, UInt uiSize2 );
  template<class T> Void  xDeleteArray  ( T*& rpt, UInt uiSize );

  // functions for reading, initialization, sorting, getting data, etc.
  Void  xReadCameraParameterFile  ( Char* pchCfgFileName );
  Bool  xGetCameraDataRow         ( Int iView, UInt uiFrame, UInt& ruiFoundLine );

  Void  xGetSortedViewList        ( const std::vector<Int>& raiViews, std::vector<Int>& raiSortedViews, std::vector<Int>& raiId2SortedId, std::vector<Int>& raiSortedId2Id );
  Void  xGetViewOrderIndices      ( const std::vector<Int>& raiId2SortedId, std::vector<Int>& raiVOIdx );
  Bool  xGetCamParsChangeFlag     ();
  Int   xGetBaseViewId            ( Int iBaseView );
  Int   xGetViewId                 ( std::vector<Int> aiViewList, Int iBaseView );

  Bool  xGetLeftRightView         ( Int iView, std::vector<Int> aiSortedViews, Int& riLeftView, Int& riRightView, Int& riLeftSortedViewIdx, Int& riRightSortedViewIdx );
  Void  xGetPrevAndNextBaseView   ( Int iSourceViewNum, Int iTargetViewNum, Int& riPrevBaseViewNum, Int& riNextBaseViewNum );
  Void  xGetZNearZFar             ( Int iView, UInt uiFrame, Double& rdZNear, Double& rdZFar );
  Void  xGetGeometryData          ( Int dView, UInt uiFrame, Double& rdFocalLength, Double& rdPosition, Double& rdCameraShift, Bool& rbInterpolated );
  Void  xSetupBaseViewsFromCoded  ();
  Void  xSetupBaseViews           ( Char* pchBaseViewNumbers, UInt uiNumBaseViews );

  // functions for getting and setting scales and offsets
  Bool  xGetShiftParameterReal    ( UInt uiSourceView, UInt uiTargetView, UInt uiFrame, Bool bExternal, Bool bByIdx, Double& rdScale, Double& rdOffset );
  Void  xGetShiftParameterCoded   ( UInt uiSourceView, UInt uiTargetView, UInt uiFrame,                 Bool bByIdx, Int&    riScale, Int&    riOffset );
  Void  xGetShiftParameterInt     ( UInt uiSourceView, UInt uiTargetView, UInt uiFrame, Bool bExternal, Bool bByIdx, Int64&  riScale, Int64&  riOffset );
  Void  xGetCodedCameraData       ( UInt uiSourceView, UInt uiTargetView, Bool bByIdx,  UInt uiFrame, Int& riScale, Int& riOffset, Int& riPrecision );

  Void  xSetCodedScaleOffset      ( UInt uiFrame );
  Void  xSetShiftParametersAndLUT ( UInt uiNumViewDim1, UInt uiNumViewDim2, UInt uiFrame, Bool bExternalReference, Double****& radLUT, Int****& raiLUT, Double***& radShiftParams, Int64***& raiShiftParams );
  Void  xSetShiftParametersAndLUT ( UInt uiFrame );

  // getting conversion parameters for disparity to virtual depth conversion
  Void  xGetCameraShifts          ( UInt uiSourceView, UInt uiTargetView, UInt uiFrame, Double& rdCamPosShift, Double& rdPicPosShift );
  Void  xSetPdmConversionParams   ();

public:
  // constructor and destructor
  TAppComCamPara();
  ~TAppComCamPara();

  // initialization, check, and and update
  Void init   ( UInt    uiNumBaseViews,
                UInt    uiInputBitDepth,
                UInt    uiCodedCamParsPrecision,
                UInt    uiStartFrameId,
                UInt    uiNumFrames,
                Char*   pchCfgFileName,
                Char*   pchBaseViewNumbers,
                Char*   pchSynthViewNumbers,
                std::vector<Int>* paiSynthViewNumbers,
                Int     iLog2Precision );

  Void init   ( UInt    uiInputBitDepth,
                UInt    uiStartFrameId,
                UInt    uiNumFrames,
                Char*   pchCfgFileName,
                Char*   pchSynthViewNumbers,
                std::vector<Int>* paiSynthViewNumbers,
                Int     iLog2Precision
              );



  Void check  ( Bool    bCheckViewRange,
                Bool    bCheckFrameRange );
  Void update ( UInt    uiFrameId );

  // miscellaneous functions

  Int                 synthRelNum2Idx           ( Int iRelNum );
  Bool                getLeftRightBaseView      ( Int iSynthViewIdx, Int &riLeftViewIdx, Int &riRightViewIdx, Int &riRelDistToLeft, Bool& rbIsBaseView );
  Int                 getRelDistLeft            ( Int iSynthViewIdx, Int   iLeftViewIdx, Int iRightViewIdx );
  UInt                getCurFrameId             ()  { return m_iCurrentFrameId;   }
  static Void         convertNumberString       ( Char* pchViewNumberString, std::vector<Int>& raiViewNumbers, Double dViewNumPrec );

#if H_3D_VSO
  // SAIT_VSO_EST_A033
  Void                setDispCoeff              ( UInt uiStartFrameId, Int iViewIdx );
  Double              getDispCoeff              () { return m_dDispCoeff; }
#endif

  // function for getting parameters and parameter arrays
  std::vector<Int>&   getBaseViewNumbers        ()  { return m_aiBaseViews;       }
  std::vector<Int>&   getSortedBaseViewNumbers  ()  { return m_aiSortedBaseViews; }
  std::vector<Int>&   getSynthViewNumbers       ()  { return m_aiSynthViews;      }
  std::vector<Int>&   getRelSynthViewNumbers    ()  { return m_aiRelSynthViewsNum;}

  std::vector<Int>&   getBaseId2SortedId        ()   { return m_aiBaseId2SortedId; }
  std::vector<Int>&   getBaseSortedId2Id        ()   { return m_aiBaseSortedId2Id; }


  Double***           getBaseViewShiftParameterD()  { return         m_adBaseViewShiftParameter;  }
  Int***              getBaseViewShiftParameterI()  { return (Int***)m_aiBaseViewShiftParameter;  }

  Double****          getSynthViewShiftLUTD     ()  { return m_adSynthViewShiftLUT;  }
  Double****          getBaseViewShiftLUTD      ()  { return m_adBaseViewShiftLUT;   }
  Int****             getSynthViewShiftLUTI     ()  { return m_aiSynthViewShiftLUT;  }
  Int****             getBaseViewShiftLUTI      ()  { return m_aiBaseViewShiftLUT;   }

  Bool                getVaryingCameraParameters()  { return m_bCamParsVaryOverTime;    }
  UInt                getCamParsCodedPrecision  ()  { return m_uiCamParsCodedPrecision; }
  std::vector<Int>&   getViewOrderIndex         ()  { return m_aiViewOrderIndex;        }
  Int**               getCodedScale             ()  { return m_aaiCodedScale;           }
  Int**               getCodedOffset            ()  { return m_aaiCodedOffset;          }

#if H_3D_PDM_CAM_PARAS
  // parameters for virtual depth map generation
  Int                 getPdmPrecision           ()  { return m_iPdmPrecision;           }
  Int**               getPdmScaleNomDelta       ()  { return m_aaiPdmScaleNomDelta;     }
  Int**               getPdmOffset              ()  { return m_aaiPdmOffset;            }
#endif
};





template <class T>
Void TAppComCamPara::xDeleteArray( T*& rpt, UInt uiSize1, UInt uiSize2, UInt uiSize3 )
{
  if( rpt )
  {
    for( UInt uiK = 0; uiK < uiSize1; uiK++ )
    {
      for( UInt uiL = 0; uiL < uiSize2; uiL++ )
      {
        for( UInt uiM = 0; uiM < uiSize3; uiM++ )
        {
          delete[] rpt[ uiK ][ uiL ][ uiM ];
        }
        delete[] rpt[ uiK ][ uiL ];
      }
      delete[] rpt[ uiK ];
    }
    delete[] rpt;
  }
  rpt = NULL;
};


template <class T>
Void TAppComCamPara::xDeleteArray( T*& rpt, UInt uiSize1, UInt uiSize2 )
{
  if( rpt )
  {
    for( UInt uiK = 0; uiK < uiSize1; uiK++ )
    {
      for( UInt uiL = 0; uiL < uiSize2; uiL++ )
      {
        delete[] rpt[ uiK ][ uiL ];
      }
      delete[] rpt[ uiK ];
    }
    delete[] rpt;
  }
  rpt = NULL;
};


template <class T>
Void TAppComCamPara::xDeleteArray( T*& rpt, UInt uiSize )
{
  if( rpt )
  {
    for( UInt uiK = 0; uiK < uiSize; uiK++ )
    {
      delete[] rpt[ uiK ];
    }
    delete[] rpt;
  }
  rpt = NULL;
};



#endif // __TAPPCOMCAMPARA__



