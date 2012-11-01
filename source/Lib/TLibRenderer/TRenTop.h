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

#ifndef __TRENTOP__
#define __TRENTOP__

#include "TRenImage.h"
#include "../TLibCommon/CommonDef.h"
#include "../TLibCommon/TComPicYuv.h"
#include <list>
#include <vector>

using namespace std;

class TRenTop
{
// ENUM Modes


  // Interpolation Modes

#if NTT_SUBPEL
  enum { eRenIntFullPel = 0, eRenIntLinInt = 1, eRenIntLinReal = 2, eRenIntFEM = 3, eRenInt8Tap = 4, eRenInt8Tap2 = 5 };
#else
  enum { eRenIntFullPel = 0, eRenIntLinInt = 1, eRenIntLinReal = 2, eRenIntFEM = 3, eRenInt8Tap = 4 };
#endif

  // HoleFilling
  enum { eRenHFNone = 0, eRenHFLWBackExt = 1};

  // Pre-Processing
  enum { eRenPreProNone = 0, eRenPreProBinom = 1};

  // Post-Processing
  enum { eRenPostProNone = 0, eRenPostProMed = 1};

  // Merging
  enum { eRenBlendAverg = 0, eRenBlendLeft = 1, eRenBlendRight = 2, eRenBlendDepthFirst = 5 };

public:
  TRenTop();
  ~TRenTop();

  // Init
  Void init              ( UInt uiImageWitdh,
                           UInt uiImageHeight,
                           Bool bExtrapolate,
                           UInt uiLog2SamplingFactor,
                           Int  iLUTPrec,
                           Bool bUVUp,
                           Int  iPreProcMode,
                           Int  iPreFilterKernelSize,
                           Int  iBlendMode,
                           Int  iBlendZThresPerc,
                           Bool bBlendUseDistWeight,
                           Int  iBlendHoleMargin,
                           Int  iInterpolationMode,
                           Int  iHoleFillingMode,
                           Int  iPostProcMode,
                           Int  iUsedPelMapMarExt );

  Void setShiftLUTs      ( Double** ppdShiftLUTLeft,
                           Int**    ppiShiftLUTLeft,
                           Int**    ppiBaseShiftLUTLeft,
                           Double** ppdShiftLUTRight,
                           Int**    ppiShiftLUTRight,
                           Int**    ppiBaseShiftLUTRight,
                           Int      iRelDistLeft );
#if NTT_SUBPEL
  Void setFposLUTs      ( Int**    ppiFposLUTLeft, Int**    ppiFposLUTRight );
  Void setInterpolationMode ( Int iMode ) { m_iInterpolationMode = iMode; }
#endif

  // View Synthesis
  Void extrapolateView   ( TComPicYuv* pcPicYuvVideo,
                           TComPicYuv* pcPicYuvDepth,
                           TComPicYuv* pcPicYuvSynthOut,
                           Bool bRenderFromLeft );

#if VSP_N
  Void extrapolateAvailabilityView   ( TComPicYuv* pcPicYuvVideo,
                                       TComPicYuv* pcPicYuvDepth,
                                       TComPicYuv* pcPicYuvSynthOut,
                                       TComPicYuv* pcPicYuvAvailOut,
                                       Bool bRenderFromLeft );
#endif

  Void interpolateView   ( TComPicYuv* pcPicYuvVideoLeft,
                           TComPicYuv* pcPicYuvDepthLeft,
                           TComPicYuv* pcPicYuvVideoRight,
                           TComPicYuv* pcPicYuvDepthRight,
                           TComPicYuv* pcPicYuvSynthOut,
                           Int         iBlendMode,
                           Int         iSimEnhBaseView );
  // Tools
  Void getUsedSamplesMap ( TComPicYuv* pcPicYuvDepth,
                           TComPicYuv* pcUsedSampleMap,
                           Bool bRenderFromLeft );

  // Zhejiang Temporal Improvement
  Void temporalFilterVSRS( TComPicYuv* pcPicYuvVideoCur,
                           TComPicYuv* pcPicYuvDepthCur,
                           TComPicYuv* pcPicYuvVideoLast,
                           TComPicYuv* pcPicYuvDepthLast,
                           Bool bFirstFrame );

private:
  // Depth PreProcessing
  Void xPreProcessDepth(PelImage* pcInImage, PelImage* pcOutImage);

  // Pixel Shifting
  Void xShiftPixels              ( PelImage*        pcInImage,     PelImage*      pcDepth     , PelImage*        pcOutImage    , PelImage*      pcFilledImage, Bool bShiftFromLeft  );
  Void xShiftPlanePixels         ( PelImagePlane** apcInputPlanes, PelImagePlane* pcDepthPlane, PelImagePlane** apcOutputPlanes, PelImagePlane* pcPlaneFilled, UInt uiNumberOfPlanes);
  Void xShiftPlanePixelsLinReal  ( PelImagePlane** apcInputPlanes, PelImagePlane* pcDepthPlane, PelImagePlane** apcOutputPlanes, PelImagePlane* pcPlaneFilled, UInt uiNumberOfPlanes);
  Void xShiftPlanePixelsFullPel  ( PelImagePlane** apcInputPlanes, PelImagePlane* pcDepthPlane, PelImagePlane** apcOutputPlanes, PelImagePlane* pcPlaneFilled, UInt uiNumberOfPlanes);
  Void xShiftPlanePixelsLinInt   ( PelImagePlane** apcInputPlanes, PelImagePlane* pcDepthPlane, PelImagePlane** apcOutputPlanes, PelImagePlane* pcFilledPlane, UInt uiNumberOfPlanes);
  Void xShiftPlanePixels8Tap     ( PelImagePlane** apcInputPlanes, PelImagePlane* pcDepthPlane, PelImagePlane** apcOutputPlanes, PelImagePlane* pcFilledPlane, UInt uiNumberOfPlanes);

  Void xBackShiftPixels          ( PelImage*            pcInImage, PelImage*      pcDepth     , PelImage*            pcOutImage, PelImage*      pcFilledImage, Bool bShiftFromLeft   );
  Void xBackShiftPlanePixels     ( PelImagePlane** apcInputPlanes, PelImagePlane* pcDepthPlane, PelImagePlane** apcOutputPlanes, PelImagePlane* pcFilledPlane, UInt uiNumberOfPlanes );

  Int  xCeil                     ( Int iVal ) { return (( iVal + ( (1 << m_iRelShiftLUTPrec) - 1 ) ) >> m_iRelShiftLUTPrec);  }

  // Hole Filling
  Void xFillHoles                ( PelImage*       pcInImage,      PelImage*      pcFilled,      PelImage*       pcOutImage                            , Bool bRenderFromLeft );
  Void xFillLWBackExt            ( PelImage*       pcInImage,      PelImage*      pcFilled,      PelImage*       pcOutImage                            , Bool bRenderFromLeft );
  Void xFillPlaneHoles           ( PelImagePlane** apcInputPlanes, PelImagePlane* pcPlaneFilled, PelImagePlane** apcOutputPlanes, UInt uiNumberOfPlanes, Bool bRenderFromLeft );

  // Alpha Map Creation
  Void xCreateAlphaMap           (PelImage* pcFilledImage,         PelImage*       pcAlphaMapImage, Bool bRenderFromLeft );
  Void xCreateAlphaMapPlane      (PelImagePlane** apcFilledPlanes, PelImagePlane** apcAlphaPlanes,  UInt uiNumberOfPlanes, Bool bRenderFromLeft);

  // BoundaryNoiseErosion
  Void xRemBoundaryNoise            ( PelImage*       pcInImage,      PelImage*      pcFilled,      PelImage*       pcOutImage                            , Bool bRenderFromLeft );
  Void xRemBoundaryNoisePlane       ( PelImagePlane** apcInputPlanes, PelImagePlane* pcPlaneFilled, PelImagePlane** apcOutputPlanes, UInt uiNumberOfPlanes, Bool bRenderFromLeft );

  // Similarity Enhancement
  Void xEnhSimilarity            ( PelImage*      pcLeftImage,   PelImage*        pcRightImage, PelImage*      pcFilledLeft,      PelImage*      pcFilledRight      );
  Void xEnhSimilarityPlane       ( PelImagePlane** apcLeftPlane, PelImagePlane** apcRightPlane, PelImagePlane* pcFilledLeftPlane, PelImagePlane* pcFilledRightPlane, UInt uNumPlanes );

  // View Blending
  Void xBlend                    ( PelImage*       pcLeftImage,  PelImage*        pcRightImage, PelImage*      pcFilledLeft,      PelImage*      pcFilledRight,      PelImage* pcLeftDepth,           PelImage* pcRightDepth,           PelImage* pcOutputImage);
  Void xBlendPlanesAvg           ( PelImagePlane** apcLeftPlane, PelImagePlane** apcRightPlane, PelImagePlane* pcFilledLeftPlane, PelImagePlane* pcFilledRightPlane, PelImagePlane* pcLeftDepthPlane, PelImagePlane* pcRightDepthPlane, PelImagePlane** apcOutputImagePlane, UInt uNumPlanes );
  Void xBlendPlanesOneView       ( PelImagePlane** apcLeftPlane, PelImagePlane** apcRightPlane, PelImagePlane* pcFilledLeftPlane, PelImagePlane* pcFilledRightPlane, PelImagePlane* pcLeftDepthPlane, PelImagePlane* pcRightDepthPlane, PelImagePlane** apcOutputImagePlane, UInt uNumPlanes );

  // PostProcessing
  Void xCutMargin                ( PelImage* pcInputImage );
  Void xCutPlaneMargin           ( PelImagePlane* pcImagePlane, Pel cFill, UInt uiScale);
  Void xPostProcessImage         ( PelImage* pcInImage,       PelImage* pCOutImage);

  // Input Output Data Conversion
  Void xConvertInputData         ( PelImage* pcOrgInputImage, PelImage* pcOrgInputDepth, PelImage* pcConvInputImage, PelImage* pcConvInputDepth, Bool bMirror);
  Void xConvertOutputData        ( PelImage* pOrgOutputImage, PelImage* pConvOutputImage, Bool bMirror);
#if VSP_N
  Void xConvertOutputDataPlane0  ( PelImage* pOrgOutputImage, PelImage* pConvOutputImage, Bool bMirror);
#endif
#if NTT_SUBPEL
  Void xConvertInputDataSubpel   ( PelImage* pcOrgInputImage, PelImage* pcOrgInputDepth, PelImage* pcConvInputImage, PelImage* pcConvInputDepth, Bool bMirror);
  Void xConvertInputVideoSubpel  ( PelImage* pcOrgInputImage, PelImage* pcConvInputImage, PelImage* pcInputDepth, Bool bMirror );
  Void xConvertInputDepthSubpel  ( PelImage* pcOrgInputImage, PelImage* pcConvInputImage);
#endif

  Void xGetDataPointers          ( PelImage*& rpcInputImage,  PelImage*& rpcOutputImage, PelImage*& rpcInputDepth, PelImage*& rpcOutputDepth, PelImage*& rpcFilled, Bool bRenderDepth );
  Void xGetDataPointerOutputImage( PelImage*& rpcOutputImage, PelImage*& rpcOutputDepth );

  Void xConvertInputVideo        ( PelImage* pcOrgInputImage, PelImage* pcConvInputImage);
  Void xConvertInputDepth        ( PelImage* pcOrgInputImage, PelImage* pcConvInputImage);


  // Data
  UInt m_uiSampledWidth;    // Width after UPsampling

  // Resolution of input view
  UInt  m_auiInputResolution[2];

  // Extrapolation
  Bool m_bExtrapolate;

  // Input Conversion
  Int  m_iLog2SamplingFactor;
  Bool m_bUVUp;

  // PreProcessing
  Int  m_iPreProcMode;         //0: none, 1: binominal
  Int  m_iPreFilterSize;       // Half size

  // Similarity Enhancement
  Int  m_iSimEnhBaseView;      // 0: none, 1: left, 2: right

  // Blending
  Int  m_iBlendMode;           // 0: average;
  Int  m_iBlendZThresPerc;     // in percent of total depth
  Bool m_bBlendUseDistWeight;  // use weighting depending on viewing distance
  Int  m_iBlendHoleMargin;     // blending margin next to holes

  Int  m_iBlendZThres;         // absoluteInt  m_iBlendWeight;
  Int  m_iBlendDistWeight;     // Weight for view distance depending blending

  // Interpolation
  Int  m_iInterpolationMode;   //0: none; 1: Linear (Double), 2: FEM (Double)

  // Hole Filling
  Int  m_iHoleFillingMode;     //0: none; 1: LW Background extension
  Int  m_bInstantHoleFilling;  // perform hole filling while pixel shifting ( only supported for interpolation mode 4 )

  // Post Processing
  Int  m_iPostProcMode;        //0: none; 1: Median

  // Precision in LUT
  Int  m_iRelShiftLUTPrec;

  // Cut
  UInt m_auiCut[2];

  // Look up tables Shift
  Double** m_ppdShiftLUTLeft;
  Double** m_ppdShiftLUTRight;
  Double** m_ppdShiftLUTRightMirror; // For rendering the mirrored view
  Double*  m_adShiftLUTCur;

  Int**    m_ppiShiftLUTLeft;
  Int**    m_ppiShiftLUTRight;
  Int**    m_ppiShiftLUTRightMirror; // For rendering the mirrored view
  Int*     m_aiShiftLUTCur;
#if NTT_SUBPEL
  Int**    m_ppiFposLUTLeft;
  Int**    m_ppiFposLUTRight;
  Int**    m_ppiFposLUTRightMirror; // For rendering the mirrored view
#endif

  // Look up tables Z
  Int*     m_piInvZLUTLeft;          // Look up table entry is proportional to Z
  Int*     m_piInvZLUTRight;

  // Look up tables sub pel shift
  Int**    m_aaiSubPelShift;

   // Zhejiang Temporal Improvement
  Int*    m_aiBlkMoving;

  // Used pel map generation
  Int      m_iUsedPelMapMarExt;

  // Buffers

  // Interpolation
  PelImage* m_pcLeftInputImage  ;
  PelImage* m_pcLeftInputDepth  ;
  PelImage* m_pcLeftOutputImage ;
  PelImage* m_pcLeftOutputDepth ;
  PelImage* m_pcLeftFilled      ;
  PelImage* m_pcRightInputImage ;
  PelImage* m_pcRightInputDepth ;
  PelImage* m_pcRightOutputImage;
  PelImage* m_pcRightOutputDepth;
  PelImage* m_pcRightFilled     ;
  PelImage* m_pcOutputImage     ;
  PelImage* m_pcOutputDepth     ;

  // Extrapolation
  PelImage* m_pcInputImage      ;
  PelImage* m_pcInputDepth      ;
  PelImage* m_pcFilled          ;

  //Temp
  PelImage* m_pcTempImage       ;
};

#endif //__TRENTOP__
