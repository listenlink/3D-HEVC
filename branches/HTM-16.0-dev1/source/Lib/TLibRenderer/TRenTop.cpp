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


#include "TRenImage.h"
#include "TRenTop.h"

#include "TRenFilter.h"
#include <iostream>
#include <math.h>
#include "../TLibCommon/CommonDef.h"
#if NH_3D_VSO


Void TRenTop::xGetDataPointers( PelImage*& rpcInputImage, PelImage*& rpcOutputImage, PelImage*& rpcInputDepth, PelImage*& rpcOutputDepth, PelImage*& rpcFilled, Bool bRenderDepth )
{
  UInt uiWidth = m_auiInputResolution[0] << m_iLog2SamplingFactor;

  UInt uiSubPelWidth  = uiWidth;

  if ( m_iInterpolationMode == eRenInt8Tap )
  {
    uiSubPelWidth <<= m_iRelShiftLUTPrec;
  }

  UInt uiHeight = m_auiInputResolution[1];

  if ( m_bUVUp )
  {
    rpcInputDepth  = new PelImage(uiSubPelWidth, uiHeight,1,0);
    rpcInputImage  = new PelImage(uiSubPelWidth, uiHeight,3,0);
    rpcOutputImage = new PelImage(uiWidth,        uiHeight,3,0);
    rpcOutputDepth = bRenderDepth  ? new PelImage(uiWidth, uiHeight,1,0) : 0;
    rpcFilled      = new PelImage(uiWidth, uiHeight,1,0);
  }
  else
  {
    rpcInputDepth  = new PelImage(uiSubPelWidth, uiHeight,1,1);
    rpcInputImage  = new PelImage(uiSubPelWidth, uiHeight,1,2);
    rpcOutputImage = new PelImage(uiWidth,        uiHeight,1,2);
    rpcOutputDepth = bRenderDepth ? new PelImage(uiWidth, uiHeight,1,1) : 0;
    rpcFilled      = new PelImage(uiWidth,        uiHeight,1,1);
  }

}

Void TRenTop::xGetDataPointerOutputImage( PelImage*& rpcOutputImage, PelImage*& rpcOutputDepth )
{

  UInt uiWidth  = m_auiInputResolution[0] << m_iLog2SamplingFactor;
  UInt uiHeight = m_auiInputResolution[1];

  if ( m_bUVUp )
  {
    rpcOutputImage = new PelImage(uiWidth, uiHeight,3,0);
    rpcOutputDepth = (m_iBlendMode == eRenBlendDepthFirst ) ? new PelImage(uiWidth, uiHeight,1,0) : NULL;
  }
  else
  {
    rpcOutputImage = new PelImage(uiWidth, uiHeight,1,2);
    rpcOutputDepth = (m_iBlendMode == eRenBlendDepthFirst ) ? new PelImage(uiWidth, uiHeight,1,1) : NULL;
  }
}

Void TRenTop::xConvertInputVideo( PelImage* pcOrgInputImage, PelImage* pcConvInputImage)
{
  TRenImagePlane<Pel>*  pcOrgPlane ;
  TRenImagePlane<Pel>*  pcConvPlane;

  Int iLog2SamplingFactor = m_iLog2SamplingFactor;

  if ( m_iInterpolationMode == eRenInt8Tap)
  {
    iLog2SamplingFactor += m_iRelShiftLUTPrec;
  }

  AOT( iLog2SamplingFactor > 2);

  for (UInt uiPlane = 0; uiPlane < 3; uiPlane++)
  {
    pcOrgPlane  = pcOrgInputImage ->getPlane(uiPlane);
    pcConvPlane = pcConvInputImage->getPlane(uiPlane);

    if (uiPlane == 0)
    {
      TRenFilter<REN_BIT_DEPTH>::sampleHorUp    ( iLog2SamplingFactor, pcOrgPlane->getPlaneData(), pcOrgPlane->getStride(), pcOrgPlane->getWidth(), pcOrgPlane->getHeight(), pcConvPlane->getPlaneData(), pcConvPlane->getStride());
    }
    else
    {
      if ( m_bUVUp )
      {
        TRenFilter<REN_BIT_DEPTH>::sampleCUpHorUp( iLog2SamplingFactor, pcOrgPlane->getPlaneData(), pcOrgPlane->getStride(), pcOrgPlane->getWidth(), pcOrgPlane->getHeight(), pcConvPlane->getPlaneData(), pcConvPlane->getStride());
      }
      else
      {
        TRenFilter<REN_BIT_DEPTH>::sampleCHorUp   ( iLog2SamplingFactor, pcOrgPlane->getPlaneData(), pcOrgPlane->getStride(), pcOrgPlane->getWidth(), pcOrgPlane->getHeight(), pcConvPlane->getPlaneData(), pcConvPlane->getStride());
      }
    }
  }
}

Void TRenTop::xConvertInputDepth( PelImage* pcOrgInputImage, PelImage* pcConvInputImage)
{
  PelImagePlane*  pcOrgPlane ;
  PelImagePlane*  pcConvPlane;

  // Full Plane
  pcOrgPlane  = pcOrgInputImage ->getPlane(0);
  pcConvPlane = pcConvInputImage->getPlane(0);

  Int iLog2SamplingFactor = m_iLog2SamplingFactor;

  if ( m_iInterpolationMode == eRenInt8Tap)
  {
    iLog2SamplingFactor += m_iRelShiftLUTPrec;
  }
  AOT( iLog2SamplingFactor > 2);

  TRenFilter<REN_BIT_DEPTH>::sampleHorUp(iLog2SamplingFactor, pcOrgPlane->getPlaneData(), pcOrgPlane->getStride(), pcOrgPlane->getWidth(), pcOrgPlane->getHeight(), pcConvPlane->getPlaneData(), pcConvPlane->getStride());

  if ( !m_bUVUp ) //GT: depth down
  {
    // Quarter Plane
    PelImagePlane* pcTempPlane = new PelImagePlane(pcOrgInputImage->getPlane(0)->getWidth(), ( pcOrgInputImage->getPlane(0)->getHeight() >> 1), REN_LUMA_MARGIN );

    TRenFilter<REN_BIT_DEPTH>::sampleVerDown2Tap13(pcOrgInputImage->getPlane(0), pcTempPlane, PICYUV_PAD);
    pcConvPlane = pcConvInputImage->getPlane(1);

    if ( iLog2SamplingFactor == 0 )
    {
      TRenFilter<REN_BIT_DEPTH>::sampleHorDown2Tap13(pcTempPlane, pcConvPlane, 0 );
    }
    else
    {
      TRenFilter<REN_BIT_DEPTH>::sampleHorUp    ( iLog2SamplingFactor - 1, pcTempPlane->getPlaneData(), pcTempPlane->getStride(), pcTempPlane->getWidth(), pcTempPlane->getHeight(), pcConvPlane->getPlaneData(), pcConvPlane->getStride());
    }
    delete pcTempPlane;
  }
}

Void TRenTop::xConvertInputData( PelImage* pcOrgInputImage, PelImage* pcOrgInputDepth, PelImage* pcConvInputImage, PelImage* pcConvInputDepth, Bool bMirror )
{
  //ToDo: remove unnecessary copying
  if ( bMirror )
  {
    m_pcTempImage->assign( pcOrgInputImage );
    TRenFilter<REN_BIT_DEPTH>::mirrorHor( m_pcTempImage );
    m_pcTempImage->extendMargin();
    xConvertInputVideo(    m_pcTempImage, pcConvInputImage );

    m_pcTempImage->getPlane(0)->assign( pcOrgInputDepth->getPlane(0) );
    TRenFilter<REN_BIT_DEPTH>::mirrorHor( m_pcTempImage->getPlane(0) );
    m_pcTempImage->getPlane(0)->extendMargin();
    xConvertInputDepth( m_pcTempImage, pcConvInputDepth );
  }
  else
  {
    m_pcTempImage->assign( pcOrgInputImage );
    m_pcTempImage->extendMargin();
    xConvertInputVideo( m_pcTempImage, pcConvInputImage );

    m_pcTempImage->getPlane(0)->assign( pcOrgInputDepth->getPlane(0) );
    m_pcTempImage->getPlane(0)->extendMargin();
    xConvertInputDepth( m_pcTempImage, pcConvInputDepth );
  }
}

Void TRenTop::xConvertOutputData( PelImage* pcOrgOutputImage, PelImage* pcConvOutputImage, Bool bMirror )
{
  Int iLog2SamplingFactor = m_iLog2SamplingFactor;

  for ( UInt uiPlane = 0; uiPlane < 3; uiPlane++)
  {
    PelImagePlane* pcOrgPlane  = pcOrgOutputImage ->getPlane(uiPlane);
    PelImagePlane* pcConvPlane = pcConvOutputImage->getPlane(uiPlane);

    pcOrgPlane->extendMargin();

    if ( uiPlane == 0 )
    {
      TRenFilter<REN_BIT_DEPTH>::sampleHorDown( iLog2SamplingFactor, pcOrgPlane->getPlaneData(), pcOrgPlane->getStride(), pcOrgPlane->getWidth(), pcOrgPlane->getHeight(), pcConvPlane->getPlaneData(), pcConvPlane->getStride());
    }
    else
    {
      if ( m_bUVUp )
      {
        TRenFilter<REN_BIT_DEPTH>::sampleCDownHorDown( iLog2SamplingFactor, pcOrgPlane->getPlaneData(), pcOrgPlane->getStride(), pcOrgPlane->getWidth(), pcOrgPlane->getHeight(), pcConvPlane->getPlaneData(), pcConvPlane->getStride());
      }
      else
      {
        TRenFilter<REN_BIT_DEPTH>::sampleCHorDown    ( iLog2SamplingFactor, pcOrgPlane->getPlaneData(), pcOrgPlane->getStride(), pcOrgPlane->getWidth(), pcOrgPlane->getHeight(), pcConvPlane->getPlaneData(), pcConvPlane->getStride());
      }
    }
  }

  if ( bMirror )
  {
    TRenFilter<REN_BIT_DEPTH>::mirrorHor( pcConvOutputImage );
  }

}

Void TRenTop::setShiftLUTs( Double** ppdShiftLUTLeft, Int** ppiShiftLUTLeft, Int** ppiBaseShiftLUTLeft, Double** ppdShiftLUTRight, Int** ppiShiftLUTRight, Int** ppiBaseShiftLUTRight,  Int iRelDistToLeft )
{
  m_ppdShiftLUTLeft  = ppdShiftLUTLeft;
  m_ppdShiftLUTRight = ppdShiftLUTRight;

  m_ppiShiftLUTLeft  = ppiShiftLUTLeft;
  m_ppiShiftLUTRight = ppiShiftLUTRight;

  if (  m_ppdShiftLUTRight != NULL && m_ppiShiftLUTRight != NULL )
  {
    for( UInt uiPlane = 0; uiPlane < 2; uiPlane++)
    {
      for (UInt uiDepthValue = 0; uiDepthValue <= 256; uiDepthValue++)
      {
        m_ppdShiftLUTRightMirror[uiPlane][uiDepthValue] = - m_ppdShiftLUTRight[uiPlane][uiDepthValue];
        m_ppiShiftLUTRightMirror[uiPlane][uiDepthValue] = - m_ppiShiftLUTRight[uiPlane][uiDepthValue];
      }
    }
  }

  if ( !m_bExtrapolate )
  {
    TRenFilter<REN_BIT_DEPTH>::setupZLUT( m_bBlendUseDistWeight, m_iBlendZThresPerc, iRelDistToLeft, ppiBaseShiftLUTLeft, ppiBaseShiftLUTRight, m_iBlendZThres, m_iBlendDistWeight, m_piInvZLUTLeft, m_piInvZLUTRight);
  }
}

Void TRenTop::extrapolateView( TComPicYuv* pcPicYuvVideo, TComPicYuv* pcPicYuvDepth, TComPicYuv* pcPicYuvSynthOut, Bool bRenderFromLeft )
{
  AOF( m_bExtrapolate );
  AOF( bRenderFromLeft ? m_ppiShiftLUTLeft || m_ppdShiftLUTLeft : m_ppiShiftLUTRight || m_ppdShiftLUTRight );
  AOF( m_auiInputResolution[0] == pcPicYuvVideo->getWidth ( COMPONENT_Y ));
  AOF( m_auiInputResolution[1] == pcPicYuvVideo->getHeight( COMPONENT_Y ));

  PelImage cInputImage ( pcPicYuvVideo    );
  PelImage cInputDepth ( pcPicYuvDepth    , true);
  PelImage cOutputImage( pcPicYuvSynthOut );

  m_pcOutputImage->init();
  m_pcFilled     ->assign(REN_IS_HOLE);

  xPreProcessDepth ( &cInputDepth,  &cInputDepth);
  xConvertInputData( &cInputImage, &cInputDepth, m_pcInputImage, m_pcInputDepth, !bRenderFromLeft );
  xShiftPixels(m_pcInputImage, m_pcInputDepth, m_pcOutputImage, m_pcFilled, bRenderFromLeft);
  xRemBoundaryNoise ( m_pcOutputImage, m_pcFilled, m_pcOutputImage, bRenderFromLeft); // Erode
  xFillHoles        ( m_pcOutputImage, m_pcFilled, m_pcOutputImage, bRenderFromLeft);
  xConvertOutputData( m_pcOutputImage, &cOutputImage, !bRenderFromLeft );
  xPostProcessImage (&cOutputImage, &cOutputImage);
  xCutMargin        ( &cOutputImage );
};

Void TRenTop::getUsedSamplesMap( TComPicYuv* pcPicYuvDepth, TComPicYuv* pcUsedSampleMap, Bool bRenderFromLeft )
{
  AOF( bRenderFromLeft ? m_ppiShiftLUTLeft && m_ppdShiftLUTLeft : m_ppiShiftLUTRight && m_ppdShiftLUTRight );
  AOF(m_auiInputResolution[0] == pcPicYuvDepth->getWidth (COMPONENT_Y));
  AOF(m_auiInputResolution[1] == pcPicYuvDepth->getHeight(COMPONENT_Y));

  PelImage cInputDepth ( pcPicYuvDepth    , true);
  PelImage cOutputImage( pcUsedSampleMap );
  PelImage cInputImage ( m_auiInputResolution[0], m_auiInputResolution[1], 1, 2 );
  cInputImage.assign(0);

  m_pcFilled     ->assign(REN_IS_HOLE);
  xConvertInputData(  &cInputImage,  &cInputDepth,   m_pcInputImage,  m_pcInputDepth, !bRenderFromLeft );
  xShiftPixels     (m_pcInputImage, m_pcInputDepth, m_pcOutputImage, m_pcFilled, bRenderFromLeft);

  xCreateAlphaMap  ( m_pcFilled, m_pcFilled, bRenderFromLeft );

  if ( !bRenderFromLeft )
  {
    TRenFilter<REN_BIT_DEPTH>::mirrorHor( m_pcFilled );
  }

  TRenFilter<REN_BIT_DEPTH>::filledToUsedPelMap( m_pcFilled, &cOutputImage, m_iUsedPelMapMarExt );
};


Void TRenTop::interpolateView( TComPicYuv* pcPicYuvVideoLeft, TComPicYuv* pcPicYuvDepthLeft, TComPicYuv* pcPicYuvVideoRight, TComPicYuv* pcPicYuvDepthRight, TComPicYuv* pcPicYuvSynthOut, Int iBlendMode, Int iSimEnhBaseView )
{
  assert( !m_bExtrapolate );
  assert( m_auiInputResolution[0] == pcPicYuvVideoLeft ->getWidth ( COMPONENT_Y ) );
  assert( m_auiInputResolution[1] == pcPicYuvVideoRight->getHeight( COMPONENT_Y ) );

  AOT( iBlendMode == 3);
  m_iBlendMode = iBlendMode;
  m_iSimEnhBaseView = iSimEnhBaseView;

  PelImage cLeftInputImage   ( pcPicYuvVideoLeft  );
  PelImage cLeftInputDepth   ( pcPicYuvDepthLeft,  true );
  PelImage cRightInputImage  ( pcPicYuvVideoRight );
  PelImage cRightInputDepth  ( pcPicYuvDepthRight, true );
  PelImage cOutputImage      ( pcPicYuvSynthOut   );

  m_pcLeftOutputImage ->init();
  m_pcRightOutputImage->init();
  m_pcOutputImage     ->init();

  if ( m_iBlendMode == eRenBlendDepthFirst )
  {
    m_pcOutputDepth->init();
  }

  m_pcLeftFilled ->assign(REN_IS_HOLE);
  m_pcRightFilled->assign(REN_IS_HOLE);

  xPreProcessDepth(&cLeftInputDepth , &cLeftInputDepth );
  xPreProcessDepth(&cRightInputDepth, &cRightInputDepth);

  xConvertInputData( &cLeftInputImage,  &cLeftInputDepth,  m_pcLeftInputImage,  m_pcLeftInputDepth  ,false );
  xConvertInputData( &cRightInputImage, &cRightInputDepth, m_pcRightInputImage, m_pcRightInputDepth ,true  );

  // Render from Left View to Right view
  if ( m_iBlendMode != eRenBlendDepthFirst )
  {
    xShiftPixels(m_pcLeftInputImage,  m_pcLeftInputDepth, m_pcLeftOutputImage, m_pcLeftFilled, true );
    xFillHoles  (m_pcLeftOutputImage, m_pcLeftFilled,     m_pcLeftOutputImage, true );
  }

  xShiftPixels(m_pcLeftInputDepth,  m_pcLeftInputDepth, m_pcLeftOutputDepth, m_pcLeftFilled, true );
  xFillHoles     ( m_pcLeftOutputDepth, m_pcLeftFilled,     m_pcLeftOutputDepth, true);
  xCreateAlphaMap( m_pcLeftFilled,      m_pcLeftFilled,     true );

  // Render from Right View to Left view
  if ( m_iBlendMode != eRenBlendDepthFirst )
  {
    xShiftPixels(m_pcRightInputImage , m_pcRightInputDepth, m_pcRightOutputImage, m_pcRightFilled, false );
    xFillHoles  (m_pcRightOutputImage, m_pcRightFilled,     m_pcRightOutputImage, false);
  }

  xShiftPixels(m_pcRightInputDepth,  m_pcRightInputDepth, m_pcRightOutputDepth, m_pcRightFilled, false);
  xFillHoles     ( m_pcRightOutputDepth, m_pcRightFilled,     m_pcRightOutputDepth, false);
  xCreateAlphaMap( m_pcRightFilled,      m_pcRightFilled, false );

  TRenFilter<REN_BIT_DEPTH>::mirrorHor( m_pcRightOutputImage );
  TRenFilter<REN_BIT_DEPTH>::mirrorHor( m_pcRightOutputDepth );
  TRenFilter<REN_BIT_DEPTH>::mirrorHor( m_pcRightFilled      );

  xEnhSimilarity( m_pcLeftOutputImage, m_pcRightOutputImage, m_pcLeftFilled, m_pcRightFilled );

  if ( m_iBlendMode == eRenBlendDepthFirst )
  {
    xBlend               ( m_pcLeftOutputDepth,  m_pcRightOutputDepth, m_pcLeftFilled,       m_pcRightFilled, m_pcLeftOutputDepth, m_pcRightOutputDepth, m_pcOutputDepth);

    xBackShiftPixels     ( m_pcLeftInputImage,   m_pcOutputDepth,      m_pcLeftOutputImage,  m_pcLeftFilled  , false);
    xFillHoles           ( m_pcLeftOutputImage,  m_pcLeftFilled,       m_pcLeftOutputImage, false);
    xCreateAlphaMap      ( m_pcLeftFilled,       m_pcLeftFilled,       true );

    TRenFilter<REN_BIT_DEPTH>::mirrorHor( m_pcRightInputImage );
    xBackShiftPixels     ( m_pcRightInputImage,  m_pcOutputDepth,      m_pcRightOutputImage, m_pcRightFilled , true );
    xFillHoles           ( m_pcRightOutputImage, m_pcRightFilled,      m_pcRightOutputImage, true);

    TRenFilter<REN_BIT_DEPTH>::mirrorHor( m_pcRightFilled );
    xCreateAlphaMap      ( m_pcRightFilled,      m_pcRightFilled,      true );
    TRenFilter<REN_BIT_DEPTH>::mirrorHor( m_pcRightFilled );
  }

  xBlend(m_pcLeftOutputImage, m_pcRightOutputImage, m_pcLeftFilled, m_pcRightFilled, m_pcLeftOutputDepth, m_pcRightOutputDepth, m_pcOutputImage);
  xConvertOutputData( m_pcOutputImage, &cOutputImage , false );

  xPostProcessImage  ( &cOutputImage, &cOutputImage);
  xCutMargin( &cOutputImage );
};


Void TRenTop::xPreProcessDepth( PelImage* pcInImage, PelImage* pcOutImage )
{
  if ( m_iPreProcMode == eRenPreProNone )
    return;

  PelImage* pcTemp;

  if (pcInImage == pcOutImage)
  {
    pcTemp = pcOutImage->create();
  }
  else
  {
    pcTemp = pcOutImage;
  }

  pcTemp->assign(pcInImage);

  switch ( m_iPreProcMode )
  {
    case eRenPreProBinom:
      TRenFilter<REN_BIT_DEPTH>::binominal(pcOutImage, pcTemp, m_iPreFilterSize);
      break;
    case eRenPreProNone:
      break;
    default:
      assert(0);
      break;
  }

  if (pcInImage == pcOutImage)
  {
    pcOutImage->setData(pcTemp, true);
    delete pcTemp;
  };

}

Void TRenTop::xShiftPlanePixelsLinInt( PelImagePlane** apcInputPlanes, PelImagePlane* pcDepthPlane, PelImagePlane** apcOutputPlanes, PelImagePlane* pcFilledPlane, UInt uiNumberOfPlanes )
{
  Int iWidth        = apcInputPlanes[0]->getWidth();
  Int iHeight       = apcInputPlanes[0]->getHeight();

  Int iInputStride  = apcInputPlanes [0]->getStride();
  Int iOutputStride = apcOutputPlanes[0]->getStride();

  Int iFilledStride = pcFilledPlane->getStride();
  Int iDepthStride  = pcDepthPlane ->getStride();

  pcFilledPlane->assign(REN_IS_HOLE);

  Pel** apcInputData  = new Pel*[ uiNumberOfPlanes ];
  Pel** apcOutputData = new Pel*[ uiNumberOfPlanes ];

  for( UInt uiCurPlane = 0; uiCurPlane < uiNumberOfPlanes; uiCurPlane++)
  {
    apcInputData   [uiCurPlane] = apcInputPlanes [uiCurPlane]->getPlaneData();
    apcOutputData  [uiCurPlane] = apcOutputPlanes[uiCurPlane]->getPlaneData();
    assert( iWidth        == apcInputPlanes [uiCurPlane]->getWidth()  && iWidth        == apcOutputPlanes[uiCurPlane]->getWidth() );
    assert( iHeight       == apcInputPlanes [uiCurPlane]->getHeight() && iHeight       == apcOutputPlanes[uiCurPlane]->getHeight());
    assert( iInputStride  == apcInputPlanes [uiCurPlane]->getStride() && iOutputStride == apcOutputPlanes[uiCurPlane]->getStride());
  }

  Pel* pcDepthData  = pcDepthPlane ->getPlaneData();
  Pel* pcFilledData = pcFilledPlane->getPlaneData();

  for(Int iPosY = 0; iPosY < iHeight; iPosY++)
  {
    Int iPrevShiftedPos = -1;
    Int iShiftedPos = -1;

    for(Int iPosX = 0; iPosX < iWidth; iPosX ++ )
    {
      Bool bExtrapolate = false;

      // compute disparity and shift
      iShiftedPos  = ( iPosX << m_iRelShiftLUTPrec ) - m_aiShiftLUTCur[RemoveBitIncrement( pcDepthData[iPosX])];

      if (iPosX == 0)
      {
        // in first iteration only get dLeftPos
        iPrevShiftedPos = iShiftedPos;
        continue;
      };

      Int iDeltaPos = iShiftedPos - iPrevShiftedPos;

      if ( iDeltaPos <= 0 || (iDeltaPos > (2 << m_iRelShiftLUTPrec)))
      {
        // skip Interpolation if pixel is shifted forwards (gap) or if  pixel is shifted backwards (foreground object)
        bExtrapolate = true;
      };

      Int iInterPolPos;
      if (!bExtrapolate)
      {  // Interpolate between j1 and j2
        for (iInterPolPos = ( iPrevShiftedPos + (1 << m_iRelShiftLUTPrec) - 1 ) >> m_iRelShiftLUTPrec  ; iInterPolPos <= (iShiftedPos >> m_iRelShiftLUTPrec); iInterPolPos++)
        {
          if ( (iInterPolPos >= iWidth) || (iInterPolPos < (Int) 0))
          {
            // skip Interpolation if Interpolation position is outside frame
            continue;
          };

          // Interpolate
          Int iDeltaCurPos  = (iInterPolPos << m_iRelShiftLUTPrec) - iPrevShiftedPos;
          for( UInt uiCurPlane = 0; uiCurPlane < uiNumberOfPlanes; uiCurPlane++)
          {
            Pel cVal  = (( apcInputData[uiCurPlane][iPosX - 1] * iDeltaPos +  ( apcInputData[uiCurPlane][iPosX] - apcInputData[uiCurPlane][iPosX - 1] ) * iDeltaCurPos ) / iDeltaPos );
            apcOutputData[uiCurPlane][iInterPolPos]  = cVal;
          }

          pcFilledData[iInterPolPos]  = REN_IS_FILLED;
        }
      }
      else
      { // Extrapolate right from dLeftPos and left from dRightPos
        Int iShiftedPosCeiled = (( iPrevShiftedPos + (1 << m_iRelShiftLUTPrec) - 1) >> m_iRelShiftLUTPrec ) << m_iRelShiftLUTPrec;
        if ( (iPrevShiftedPos + (m_iRelShiftLUTPrec >> 1) ) > iShiftedPosCeiled )
        {
          iInterPolPos = iShiftedPosCeiled >> m_iRelShiftLUTPrec;

          if ( (iInterPolPos >= iWidth) || (iInterPolPos < (Int) 0))
          {
            // skip Interpolation if Interpolation position is outside frame
            iPrevShiftedPos = iShiftedPos;
            continue;
          };

          for( UInt uiCurPlane = 0; uiCurPlane < uiNumberOfPlanes; uiCurPlane++)
          {
            apcOutputData[uiCurPlane][iInterPolPos]  = apcInputData[uiCurPlane][iPosX - 1];
          }

          pcFilledData[iInterPolPos]  = REN_IS_FILLED;
        }

        Int iPrevShiftedPosFloor = (iShiftedPos >> m_iRelShiftLUTPrec) << m_iRelShiftLUTPrec;
        if (iShiftedPos - (m_iRelShiftLUTPrec > 1) < iPrevShiftedPosFloor )
        {
          iInterPolPos = iPrevShiftedPosFloor >> m_iRelShiftLUTPrec;

          if ( (iInterPolPos >= iWidth) || (iInterPolPos < (Int) 0))
          {
            // skip Interpolation if Interpolation position is outside frame
            iPrevShiftedPos = iShiftedPos;
            continue;
          };

          for( UInt uiCurPlane = 0; uiCurPlane < uiNumberOfPlanes; uiCurPlane++)
          {
            apcOutputData[uiCurPlane][iInterPolPos]  = apcInputData[uiCurPlane][iPosX ];
          }

          pcFilledData[iInterPolPos]  = REN_IS_FILLED;
        }
      }
      iPrevShiftedPos = iShiftedPos;
    }

    for( UInt uiCurPlane = 0; uiCurPlane < uiNumberOfPlanes; uiCurPlane++)
    {
      apcOutputData[uiCurPlane] += iOutputStride;
      apcInputData [uiCurPlane] += iInputStride;
    }
    pcFilledData += iFilledStride;
    pcDepthData  += iDepthStride;
  }
  delete[] apcInputData;
  delete[] apcOutputData;
};


Void TRenTop::xShiftPlanePixelsLinReal( PelImagePlane** apcInputPlanes, PelImagePlane* pcDepthPlane, PelImagePlane** apcOutputPlanes, PelImagePlane* pcFilledPlane, UInt uiNumberOfPlanes )
{
  Int iWidth        = apcInputPlanes[0]->getWidth();
  Int iHeight       = apcInputPlanes[0]->getHeight();

  Int iInputStride  = apcInputPlanes [0]->getStride();
  Int iOutputStride = apcOutputPlanes[0]->getStride();

  Int iFilledStride = pcFilledPlane->getStride();
  Int iDepthStride  = pcDepthPlane ->getStride();

  pcFilledPlane->assign( REN_IS_HOLE );

  Pel** apcInputData  = new Pel*[ uiNumberOfPlanes ];
  Pel** apcOutputData = new Pel*[ uiNumberOfPlanes ];

  for( UInt uiCurPlane = 0; uiCurPlane < uiNumberOfPlanes; uiCurPlane++)
  {
    apcInputData   [uiCurPlane] = apcInputPlanes [uiCurPlane]->getPlaneData();
    apcOutputData  [uiCurPlane] = apcOutputPlanes[uiCurPlane]->getPlaneData();
    assert( iWidth        == apcInputPlanes [uiCurPlane]->getWidth()  && iWidth        == apcOutputPlanes[uiCurPlane]->getWidth() );
    assert( iHeight       == apcInputPlanes [uiCurPlane]->getHeight() && iHeight       == apcOutputPlanes[uiCurPlane]->getHeight());
    assert( iInputStride  == apcInputPlanes [uiCurPlane]->getStride() && iOutputStride == apcOutputPlanes[uiCurPlane]->getStride());
  }

  Pel* pcDepthData  = pcDepthPlane ->getPlaneData();
  Pel* pcFilledData = pcFilledPlane->getPlaneData();

  ///// FEM Stuff /////
  const UInt  cuiMaxPlaneNum = 6;  AOT( uiNumberOfPlanes > cuiMaxPlaneNum );
  IntImagePlane* apcDiffPlane[ cuiMaxPlaneNum ];
  Int*          ppiDiffPlanes[ cuiMaxPlaneNum ];
  Int             iDiffStride = 0;

  if ( m_iInterpolationMode == eRenIntFEM )
  {
    AOT( uiNumberOfPlanes > cuiMaxPlaneNum );
    for ( UInt uiCurPlane = 0; uiCurPlane < uiNumberOfPlanes; uiCurPlane++ )
    {
      apcDiffPlane[uiCurPlane] = new IntImagePlane( iWidth, iHeight, apcInputPlanes[uiCurPlane]->getPad());
      TRenFilter<REN_BIT_DEPTH>::diffHorSym(apcInputPlanes[uiCurPlane] , apcDiffPlane[uiCurPlane]);
      ppiDiffPlanes[uiCurPlane] = apcDiffPlane[uiCurPlane]->getPlaneData();
    }
    iDiffStride = apcDiffPlane[0]->getStride();
  }
  ///// FEM Stuff End /////

  for(Int iPosY = 0; iPosY < iHeight; iPosY++)
  {
    Double dShiftedPos = 0;
    Double dPrevShiftedPos = 0;

    for(Int iPosX = 0; iPosX < iWidth; iPosX ++ )
    {
        Bool bExtrapolate = false;

        // compute disparity and shift
        assert( RemoveBitIncrement(pcDepthData[iPosX]) >= 0 && RemoveBitIncrement(pcDepthData[iPosX]) <= 256 );
        dPrevShiftedPos  = (Double) iPosX - m_adShiftLUTCur[ RemoveBitIncrement(pcDepthData[iPosX])];

        if (iPosX == 0)
        {
          // in first iteration only get dLeftPos
          dShiftedPos = dPrevShiftedPos;
          continue;
        };

        Double dDeltaPos = dPrevShiftedPos - dShiftedPos;

        if ((dDeltaPos <= 0) || ( dDeltaPos > 2 ))
        {
          // skip Interpolation if pixel is shifted backwards (foreground object)  or if pixel is shifted forwards (gap)
          bExtrapolate = true;
        };

        Int iInterPolPos;
        if (!bExtrapolate)
        {  // Interpolate between j1 and j2
          for (iInterPolPos = (Int) ceil(dShiftedPos); iInterPolPos <= floor(dPrevShiftedPos); iInterPolPos++)
          {
            if ( (iInterPolPos >= (Int) iWidth) || (iInterPolPos < (Int) 0 ))
            {
              // skip Interpolation if Interpolation position is outside frame
              continue;
            };

            // Interpolate
            Pel cVal;
            if ( m_iInterpolationMode == eRenIntFEM ) //FEM Interpolation
            {
              for( UInt uiCurPlane = 0; uiCurPlane < uiNumberOfPlanes; uiCurPlane++)
              {
                cVal  = TRenFilter<REN_BIT_DEPTH>::interpCHSpline(iInterPolPos, dShiftedPos, dPrevShiftedPos, apcInputData[uiCurPlane][iPosX - 1], ppiDiffPlanes[uiCurPlane][iPosX - 1], apcInputData[uiCurPlane][iPosX], ppiDiffPlanes[uiCurPlane][iPosX] );
                apcOutputData[uiCurPlane][iInterPolPos]  = cVal;
              }
            }
            else
            {
              Double dDeltaJ  = (Double) iInterPolPos - dShiftedPos;

              for( UInt uiCurPlane = 0; uiCurPlane < uiNumberOfPlanes; uiCurPlane++)
              {
                cVal  = (UChar) ( (Double) apcInputData[uiCurPlane][iPosX - 1] +  ( (Double) apcInputData[uiCurPlane][iPosX] - (Double) apcInputData[uiCurPlane][iPosX - 1] ) / dDeltaPos * dDeltaJ + 0.5);
                apcOutputData[uiCurPlane][iInterPolPos]  = cVal;
              }
            };

            pcFilledData[iInterPolPos]  = REN_IS_FILLED;
          }
        }
        else
        { // Extrapolate right from dLeftPos and left from dRightPos
          if (dShiftedPos + 0.5 > ceil(dShiftedPos))
          {
            iInterPolPos = (Int) ceil(dShiftedPos);

            if ( (iInterPolPos >= (Int) iWidth) || (iInterPolPos < (Int) 0))
            {
              // skip Interpolation if Interpolation position is outside frame
              dShiftedPos = dPrevShiftedPos;
              continue;
            };

            for( UInt uiCurPlane = 0; uiCurPlane < uiNumberOfPlanes; uiCurPlane++)
            {
              apcOutputData[uiCurPlane][iInterPolPos]  = apcInputData[uiCurPlane][iPosX - 1];
            }

            pcFilledData[iInterPolPos]  = REN_IS_FILLED;
          }

          if (dPrevShiftedPos - 0.5 < floor(dPrevShiftedPos))
          {
            iInterPolPos = (Int) floor(dPrevShiftedPos);

            if ( (iInterPolPos >= (Int) iWidth) || (iInterPolPos < (Int) 0))
            {
              // skip Interpolation if Interpolation position is outside frame
              dShiftedPos = dPrevShiftedPos;
              continue;
            };

            for( UInt uiCurPlane = 0; uiCurPlane < uiNumberOfPlanes; uiCurPlane++)
            {
              apcOutputData[uiCurPlane][iInterPolPos]  = apcInputData[uiCurPlane][iPosX ];
            }

            pcFilledData[iInterPolPos]  = REN_IS_FILLED;
          }
        }
        dShiftedPos = dPrevShiftedPos;
      }

    for( UInt uiCurPlane = 0; uiCurPlane < uiNumberOfPlanes; uiCurPlane++)
    {
      apcOutputData[uiCurPlane] += iOutputStride;
      apcInputData [uiCurPlane] += iInputStride;

      if (m_iInterpolationMode ==  eRenIntFEM)
      {
        ppiDiffPlanes[ uiCurPlane ] += iDiffStride;
      }
    }

    pcFilledData += iFilledStride;
    pcDepthData  += iDepthStride;
  }

  if (m_iInterpolationMode ==  eRenIntFEM)
  {
    for( UInt uiCurPlane = 0; uiCurPlane < uiNumberOfPlanes; uiCurPlane++)
    {
      delete apcDiffPlane[uiCurPlane];
    }
  }

  delete[] apcInputData;
  delete[] apcOutputData;
}


Void TRenTop::xShiftPlanePixels( PelImagePlane** apcInPlane, PelImagePlane* pcDepthPlane, PelImagePlane** apcOutPlane, PelImagePlane* pcPlaneFilled, UInt uiNumberOfPlanes )
{
  switch ( m_iInterpolationMode)
  {
  case eRenIntFullPel:
    xShiftPlanePixelsFullPel( apcInPlane, pcDepthPlane, apcOutPlane, pcPlaneFilled, uiNumberOfPlanes);
    break;
  case eRenIntFEM:
  case eRenIntLinReal:
    xShiftPlanePixelsLinReal( apcInPlane, pcDepthPlane, apcOutPlane, pcPlaneFilled, uiNumberOfPlanes);
    break;
  case eRenIntLinInt:
    xShiftPlanePixelsLinInt ( apcInPlane, pcDepthPlane, apcOutPlane, pcPlaneFilled, uiNumberOfPlanes);
    break;
  case eRenInt8Tap:
    xShiftPlanePixels8Tap   ( apcInPlane, pcDepthPlane, apcOutPlane, pcPlaneFilled, uiNumberOfPlanes );
    break;
  default:
    AOF( false );
  }
}


Void TRenTop::xShiftPlanePixelsFullPel( PelImagePlane** apcInputPlanes, PelImagePlane* pcDepthPlane, PelImagePlane** apcOutputPlanes, PelImagePlane* pcFilledPlane, UInt uiNumberOfPlanes )
{
  Int iWidth        = apcInputPlanes[0]->getWidth();
  Int iHeight       = apcInputPlanes[0]->getHeight();

  Int iInputStride  = apcInputPlanes [0]->getStride();
  Int iOutputStride = apcOutputPlanes[0]->getStride();

  Int iFilledStride = pcFilledPlane->getStride();
  Int iDepthStride  = pcDepthPlane ->getStride();

  pcFilledPlane->assign(REN_IS_HOLE);

  Pel** apcInputData  = new Pel*[ uiNumberOfPlanes ];
  Pel** apcOutputData = new Pel*[ uiNumberOfPlanes ];

  for( UInt uiCurPlane = 0; uiCurPlane < uiNumberOfPlanes; uiCurPlane++)
  {
    apcInputData   [uiCurPlane] = apcInputPlanes [uiCurPlane]->getPlaneData();
    apcOutputData  [uiCurPlane] = apcOutputPlanes[uiCurPlane]->getPlaneData();
    assert( iWidth        == apcInputPlanes [uiCurPlane]->getWidth()  && iWidth        == apcOutputPlanes[uiCurPlane]->getWidth() );
    assert( iHeight       == apcInputPlanes [uiCurPlane]->getHeight() && iHeight       == apcOutputPlanes[uiCurPlane]->getHeight());
    assert( iInputStride  == apcInputPlanes [uiCurPlane]->getStride() && iOutputStride == apcOutputPlanes[uiCurPlane]->getStride());
  }

  Pel* pcDepthData  = pcDepthPlane ->getPlaneData();
  Pel* pcFilledData = pcFilledPlane->getPlaneData();

  for(Int iPosY = 0; iPosY < iHeight; iPosY++)
  {
    Int iPrevShiftedPos = -1;

    for(Int iPosX = 0; iPosX < iWidth; iPosX++)
    {
      assert( RemoveBitIncrement(pcDepthData[iPosX]) >= 0 && RemoveBitIncrement(pcDepthData[iPosX]) <= 256 );
      Int iShiftedPos = iPosX - m_aiShiftLUTCur[ RemoveBitIncrement(pcDepthData[iPosX])] ;
      if (iShiftedPos < iWidth && iShiftedPos >= 0)
      {
        Int iDiff = iShiftedPos - iPrevShiftedPos;
        if (( iDiff <= 2) && (iDiff > 0) )
        {
          for (Int iCurPos = iPrevShiftedPos+1; iCurPos <= iShiftedPos; iCurPos++)
          {
            for( UInt uiCurPlane = 0; uiCurPlane < uiNumberOfPlanes; uiCurPlane++)
            {
              apcOutputData[uiCurPlane][iCurPos] = apcInputData[uiCurPlane][iPosX];    // Only small gaps, therefor not necessary NN
            }
            pcFilledData[iCurPos] = REN_IS_FILLED;
          }
        }
        else
        {
          for( UInt uiCurPlane = 0; uiCurPlane < uiNumberOfPlanes; uiCurPlane++)
          {
            apcOutputData[uiCurPlane][iShiftedPos] = apcInputData[uiCurPlane][iPosX];
          }
          pcFilledData[iShiftedPos] = REN_IS_FILLED;
        }
        iPrevShiftedPos = iShiftedPos;
      }
    }
    for( UInt uiCurPlane = 0; uiCurPlane < uiNumberOfPlanes; uiCurPlane++)
    {
      apcOutputData[uiCurPlane] += iOutputStride;
      apcInputData [uiCurPlane] += iInputStride;
    }
    pcFilledData += iFilledStride;
    pcDepthData  += iDepthStride;
  }

  delete[] apcInputData;
  delete[] apcOutputData;
}

Void TRenTop::xBackShiftPlanePixels( PelImagePlane** apcInputPlanes, PelImagePlane* pcDepthPlane, PelImagePlane** apcOutputPlanes, PelImagePlane* pcFilledPlane, UInt uiNumberOfPlanes )
{
  Int iOutputWidth  = apcOutputPlanes[0]->getWidth();
  Int iInputWidth   = apcInputPlanes [0]->getWidth();
  Int iHeight       = apcInputPlanes [0]->getHeight();

  Int iInputStride  = apcInputPlanes [0]->getStride();
  Int iOutputStride = apcOutputPlanes[0]->getStride();

  Int iFilledStride = pcFilledPlane->getStride();
  Int iDepthStride  = pcDepthPlane ->getStride();

  Pel** apcInputData  = new Pel*[ uiNumberOfPlanes ];
  Pel** apcOutputData = new Pel*[ uiNumberOfPlanes ];

  Int iStep         = (1 << m_iRelShiftLUTPrec);

  for( UInt uiCurPlane = 0; uiCurPlane < uiNumberOfPlanes; uiCurPlane++)
  {
    apcInputData   [uiCurPlane] = apcInputPlanes [uiCurPlane]->getPlaneData();
    apcOutputData  [uiCurPlane] = apcOutputPlanes[uiCurPlane]->getPlaneData();
    AOF( iInputWidth   == apcInputPlanes [uiCurPlane]->getWidth()  && iOutputWidth  == apcOutputPlanes[uiCurPlane]->getWidth() );
    AOF( iHeight       == apcInputPlanes [uiCurPlane]->getHeight() && iHeight       == apcOutputPlanes[uiCurPlane]->getHeight());
    AOF( iInputStride  == apcInputPlanes [uiCurPlane]->getStride() && iOutputStride == apcOutputPlanes[uiCurPlane]->getStride());
    AOF( iInputWidth   == iOutputWidth * iStep );
  }

  Pel* pcDepthData  = pcDepthPlane ->getPlaneData();
  Pel* pcFilledData = pcFilledPlane->getPlaneData();


  for(Int iPosY = 0; iPosY < iHeight; iPosY++)
  {
    for(Int iPosX = 0; iPosX < iOutputWidth; iPosX ++)
    {
      Int iBackShiftedPos = (iPosX << m_iRelShiftLUTPrec) - m_aiShiftLUTCur[ RemoveBitIncrement( pcDepthData[iPosX] )];
      if( ( pcFilledData[iPosX] == REN_IS_FILLED )  && (iBackShiftedPos >= 0 ) && ( iBackShiftedPos < iInputWidth ) )
      {
        for( UInt uiCurPlane = 0; uiCurPlane < uiNumberOfPlanes; uiCurPlane++)
        {
          apcOutputData[uiCurPlane][iPosX] = apcInputData[uiCurPlane][iBackShiftedPos];
        }
      }
      else
      {
        for( UInt uiCurPlane = 0; uiCurPlane < uiNumberOfPlanes; uiCurPlane++)
        {
          apcOutputData[uiCurPlane][iPosX] = 0;
        }
        pcFilledData[iPosX] = REN_IS_HOLE;
      }
    }

    for( UInt uiCurPlane = 0; uiCurPlane < uiNumberOfPlanes; uiCurPlane++)
    {
      apcOutputData[uiCurPlane] += iOutputStride;
      apcInputData [uiCurPlane] += iInputStride;
    }
    pcFilledData += iFilledStride;
    pcDepthData  += iDepthStride;
  }

  delete[] apcInputData;
  delete[] apcOutputData;
}

Void TRenTop::xShiftPlanePixels8Tap( PelImagePlane** apcInputPlanes, PelImagePlane* pcDepthPlane, PelImagePlane** apcOutputPlanes, PelImagePlane* pcFilledPlane, UInt uiNumberOfPlanes  )
{
  Bool bRenderDepth = (apcInputPlanes[0] == pcDepthPlane);

  Int iOutputWidth  = apcOutputPlanes[0]->getWidth();
  Int iInputWidth   = apcInputPlanes [0]->getWidth();
  Int iHeight       = apcInputPlanes [0]->getHeight();

  Int iInputStride  = apcInputPlanes [0]->getStride();
  Int iOutputStride = apcOutputPlanes[0]->getStride();

  Int iFilledStride = pcFilledPlane->getStride();
  Int iDepthStride  = pcDepthPlane ->getStride();

  Int iStep         = (1 << m_iRelShiftLUTPrec);

  pcFilledPlane->assign(REN_IS_HOLE);

  Pel** apcInputData  = new Pel*[ uiNumberOfPlanes ];
  Pel** apcOutputData = new Pel*[ uiNumberOfPlanes ];

  for( UInt uiCurPlane = 0; uiCurPlane < uiNumberOfPlanes; uiCurPlane++)
  {
    apcInputData   [uiCurPlane] = apcInputPlanes [uiCurPlane]->getPlaneData();
    apcOutputData  [uiCurPlane] = apcOutputPlanes[uiCurPlane]->getPlaneData();
    AOF( iInputWidth   == apcInputPlanes [uiCurPlane]->getWidth()  && iOutputWidth  == apcOutputPlanes[uiCurPlane]->getWidth() );
    AOF( iHeight       == apcInputPlanes [uiCurPlane]->getHeight() && iHeight       == apcOutputPlanes[uiCurPlane]->getHeight());
    AOF( iInputStride  == apcInputPlanes [uiCurPlane]->getStride() && iOutputStride == apcOutputPlanes[uiCurPlane]->getStride());
    AOF( iInputWidth   == iOutputWidth * iStep );
  }

  Pel* pcDepthData  = pcDepthPlane ->getPlaneData();
  Pel* pcFilledData = pcFilledPlane->getPlaneData();

  for(Int iPosY = 0; iPosY < iHeight; iPosY++)
  {
    Int iPrevShiftedPos = -1;
    Int iShiftedPos     = -1;

    for(Int iPosX = 0; iPosX < iInputWidth; iPosX += iStep )
    {
      // compute disparity and shift
      iShiftedPos  =  iPosX - m_aiShiftLUTCur[RemoveBitIncrement(pcDepthData[iPosX])];

      if ( iPosX == 0 )
      {
        // in first iteration only get dLeftPos
        iPrevShiftedPos = iShiftedPos;
        continue;
      };

      Int iDeltaPos = iShiftedPos - iPrevShiftedPos;

      Bool bDisocclusion = ( iDeltaPos > (2 << m_iRelShiftLUTPrec) );
      Bool bOcclusion    = ( iDeltaPos <= 0 );

      Int iInterPolPos;
      if ( !bDisocclusion && !bOcclusion )
      {  // Interpolate between previous shifted pos and shifted pos
        for (iInterPolPos = xCeil( iPrevShiftedPos ); iInterPolPos <= xCeil (iShiftedPos ) -1 ; iInterPolPos++)
        {
          if ( (iInterPolPos < (Int) 0) || (iInterPolPos >= iOutputWidth))
          {
            // skip Interpolation if Interpolation position is outside frame
            continue;
          };

          // Interpolate
          Int iDeltaCurPos  = (iInterPolPos << m_iRelShiftLUTPrec) - iPrevShiftedPos;

          AOF( (iDeltaCurPos <= iDeltaPos) && ( iDeltaCurPos >= 0));
          AOF( iDeltaPos    <= (2 <<  m_iRelShiftLUTPrec)  );
          AOF( m_aaiSubPelShift[iDeltaPos][iDeltaCurPos] != 0xdeaddead);

          Int iSourcePos;

          if ( bRenderDepth )
          {
            iSourcePos = iPosX - iStep; // Render depth with Full Pel accuracy to avoid ringing at sharp depth edges;
          }
          else
          {
            iSourcePos = iPosX +  m_aaiSubPelShift[iDeltaPos][iDeltaCurPos];   // GT:  = iPosX - iStep + ( iStep * iDeltaCurPos + ( iDeltaPos >> 1) ) / iDeltaPos;
          }

          for( UInt uiCurPlane = 0; uiCurPlane < uiNumberOfPlanes; uiCurPlane++)
          {
            apcOutputData[uiCurPlane][iInterPolPos] = apcInputData[uiCurPlane][iSourcePos];
          }

          pcFilledData[ iInterPolPos]  = REN_IS_FILLED;
        }
      }
      else
        {
        // Fill Disocclusion Edge

        if ( bDisocclusion )
        {
          Int iPrevShiftedPosCeiled =  xCeil(iPrevShiftedPos) << m_iRelShiftLUTPrec;
          iInterPolPos = iPrevShiftedPosCeiled >> m_iRelShiftLUTPrec;

          if ((iPrevShiftedPos + (iStep >> 1) ) > iPrevShiftedPosCeiled )
          {
            if ( !((iInterPolPos < (Int) 0) || (iInterPolPos >= iOutputWidth)))
            {

              for( UInt uiCurPlane = 0; uiCurPlane < uiNumberOfPlanes; uiCurPlane++)
              {
                apcOutputData[uiCurPlane][iInterPolPos]  = apcInputData[uiCurPlane][iPosX - iStep];
              }
              pcFilledData[iInterPolPos]  = REN_IS_FILLED;

            }           
            iInterPolPos++;
          }          

          // Fill Disocclusion
          if ( m_bInstantHoleFilling )
          {
            for ( ; iInterPolPos <= xCeil (iShiftedPos ) -1 ; iInterPolPos++)
            {
              if ( ( iInterPolPos >= 0 ) && ( iInterPolPos < iOutputWidth ) )
              {
                if( pcFilledData[iInterPolPos] == REN_IS_HOLE )
                {               
                  Int iNextPos = std::min(iInputWidth-1,iPosX + iStep); 
                  Int iPosXBG  = ( std::abs( pcDepthData[iNextPos] - pcDepthData[iPosX] ) > 5  ) ? iPosX : iNextPos; 
                  for( UInt uiCurPlane = 0; uiCurPlane < uiNumberOfPlanes; uiCurPlane++)
                  {
                    apcOutputData[uiCurPlane][iInterPolPos]  = apcInputData[uiCurPlane][iPosXBG];
                  }
                }
                else
                {
                  pcFilledData[iInterPolPos] = REN_IS_HOLE + 1; 
                }
              }
            }
          }
        }

        //// Last sample next to occlusion
        Int iShiftedPosFloor = ( iShiftedPos >> m_iRelShiftLUTPrec ) << m_iRelShiftLUTPrec;
        if ( bOcclusion && (iShiftedPos - (iStep >> 1) < iShiftedPosFloor) )
        {
          iInterPolPos = iShiftedPosFloor >> m_iRelShiftLUTPrec;
          if ( !((iInterPolPos < (Int) 0) || (iInterPolPos >= iOutputWidth)))
          {        
            for( UInt uiCurPlane = 0; uiCurPlane < uiNumberOfPlanes; uiCurPlane++)
            {
              apcOutputData[uiCurPlane][iInterPolPos]  = apcInputData[uiCurPlane][iPosX ];
            }

            pcFilledData[iInterPolPos]  = REN_IS_FILLED;
          }
        }
      }
      iPrevShiftedPos = iShiftedPos;
    }

    for( UInt uiCurPlane = 0; uiCurPlane < uiNumberOfPlanes; uiCurPlane++)
    {
      apcOutputData[uiCurPlane] += iOutputStride;
      apcInputData [uiCurPlane] += iInputStride;
    }
    pcFilledData += iFilledStride;
    pcDepthData  += iDepthStride;
  }
  delete[] apcInputData;
  delete[] apcOutputData;
};

Void TRenTop::xShiftPixels(PelImage* pcInImage, PelImage* pcDepth, PelImage* pcOutImage, PelImage* pcFilledImage, Bool bShiftFromLeft )
{
  PelImage*  pcTemp = 0;

  if (pcInImage == pcOutImage)
  {
    pcTemp = pcOutImage->create();
  }
  else
  {
    pcTemp = pcOutImage;
  }

  Double ** ppdShiftLUT = bShiftFromLeft ? m_ppdShiftLUTLeft : m_ppdShiftLUTRightMirror;
  Int    ** ppiShiftLUT = bShiftFromLeft ? m_ppiShiftLUTLeft : m_ppiShiftLUTRightMirror;

  UInt uiNumFullPlanes = pcInImage->getNumberOfFullPlanes();
  UInt uiNumQuatPlanes = pcInImage->getNumberOfQuaterPlanes();

  assert( uiNumFullPlanes == pcOutImage->getNumberOfFullPlanes  () );
  assert( uiNumQuatPlanes == pcOutImage->getNumberOfQuaterPlanes() );

  m_aiShiftLUTCur = ppiShiftLUT[ 0 ];
  m_adShiftLUTCur = ppdShiftLUT[ 0 ];

  xShiftPlanePixels( pcInImage->getPlanes(), pcDepth->getPlane(0),  pcOutImage->getPlanes(), pcFilledImage->getPlane(0),  uiNumFullPlanes  );

  if (uiNumQuatPlanes > 0)
  {
    assert( pcDepth->getNumberOfPlanes() > 1 && pcFilledImage->getNumberOfPlanes() > 1);
    m_aiShiftLUTCur = ppiShiftLUT[ 1 ];
    m_adShiftLUTCur = ppdShiftLUT[ 1 ];
    xShiftPlanePixels( pcInImage->getPlanes()+uiNumFullPlanes,pcDepth->getPlane(1),  pcOutImage->getPlanes() + uiNumFullPlanes, pcFilledImage->getPlane(1),  uiNumQuatPlanes );
  }

  if (pcInImage == pcOutImage)
  {
    pcOutImage->assign(pcTemp);
    delete pcTemp;
  };
};

Void TRenTop::xBackShiftPixels(PelImage* pcInImage, PelImage* pcDepth, PelImage* pcOutImage, PelImage* pcFilledImage, Bool bShiftFromLeft )
{
  PelImage*  pcTemp = 0;

  if (pcInImage == pcOutImage)
  {
    pcTemp = pcOutImage->create();
  }
  else
  {
    pcTemp = pcOutImage;
  }

  Double ** ppdShiftLUT = bShiftFromLeft ? m_ppdShiftLUTLeft : m_ppdShiftLUTRight;
  Int    ** ppiShiftLUT = bShiftFromLeft ? m_ppiShiftLUTLeft : m_ppiShiftLUTRight;

  UInt uiNumFullPlanes = pcInImage->getNumberOfFullPlanes();
  UInt uiNumQuatPlanes = pcInImage->getNumberOfQuaterPlanes();

  assert( uiNumFullPlanes == pcOutImage->getNumberOfFullPlanes  () );
  assert( uiNumQuatPlanes == pcOutImage->getNumberOfQuaterPlanes() );

  m_aiShiftLUTCur = ppiShiftLUT[ 0 ];
  m_adShiftLUTCur = ppdShiftLUT[ 0 ];

  xBackShiftPlanePixels( pcInImage->getPlanes(), pcDepth->getPlane(0),  pcOutImage->getPlanes(), pcFilledImage->getPlane(0),  uiNumFullPlanes  );

  if (uiNumQuatPlanes > 0)
  {
    assert( pcDepth->getNumberOfPlanes() > 1 && pcFilledImage->getNumberOfPlanes() > 1);
    m_aiShiftLUTCur = ppiShiftLUT[ 1 ];
    m_adShiftLUTCur = ppdShiftLUT[ 1 ];
    xBackShiftPlanePixels( pcInImage->getPlanes()+uiNumFullPlanes,pcDepth->getPlane(1),  pcOutImage->getPlanes() + uiNumFullPlanes, pcFilledImage->getPlane(1),  uiNumQuatPlanes );
  }

  if (pcInImage == pcOutImage)
  {
    pcOutImage->assign(pcTemp);
    delete pcTemp;
  };
};

Void TRenTop::xFillHoles(PelImage* pcInImage, PelImage* pcFilled, PelImage* pcOutImage, Bool bRenderFromLeft )
{
  if (pcInImage != pcOutImage)
  {
    pcOutImage->assign(pcInImage);
  }

  switch (m_iHoleFillingMode)
  {
    case eRenHFNone:
      break;
    case eRenHFLWBackExt:
      xFillLWBackExt( pcInImage, pcFilled, pcOutImage, bRenderFromLeft);
      break;
    default:
      break;
  }
};

Void TRenTop::xFillLWBackExt( PelImage* pcInImage, PelImage* pcFilledImage, PelImage* pcOutImage, Bool bRenderFromLeft )
{
  UInt uiNumFullPlanes = pcInImage->getNumberOfFullPlanes();
  UInt uiNumQuatPlanes = pcInImage->getNumberOfQuaterPlanes();

  assert( uiNumFullPlanes == pcOutImage->getNumberOfFullPlanes  () );
  assert( uiNumQuatPlanes == pcOutImage->getNumberOfQuaterPlanes() );

  xFillPlaneHoles( pcInImage->getPlanes(), pcFilledImage->getPlane(0), pcOutImage->getPlanes(),  uiNumFullPlanes, bRenderFromLeft  );

  if (uiNumQuatPlanes > 0)
  {
    assert(  pcFilledImage->getNumberOfPlanes() > 1);
    xFillPlaneHoles( pcInImage->getPlanes()+uiNumFullPlanes, pcFilledImage->getPlane(1), pcOutImage->getPlanes() + uiNumFullPlanes,  uiNumQuatPlanes, bRenderFromLeft );
  }
};

Void TRenTop::xCreateAlphaMap(PelImage* pcFilledImage, PelImage* pcAlphaMapImage, Bool bRenderFromLeft )
{
  UInt uiNumFullPlanes = pcFilledImage  ->getNumberOfFullPlanes();
  UInt uiNumQuatPlanes = pcFilledImage->getNumberOfQuaterPlanes();

  AOF( uiNumFullPlanes == pcAlphaMapImage->getNumberOfFullPlanes  () );
  AOF( uiNumQuatPlanes == pcAlphaMapImage->getNumberOfQuaterPlanes() );

  xCreateAlphaMapPlane( pcFilledImage->getPlanes(),  pcAlphaMapImage->getPlanes(),  uiNumFullPlanes, bRenderFromLeft  );

  if (uiNumQuatPlanes > 0)
  {
    AOF(  pcFilledImage->getNumberOfPlanes() > 1);
    xCreateAlphaMapPlane( pcFilledImage->getPlanes()+ uiNumFullPlanes, pcAlphaMapImage->getPlanes()+uiNumFullPlanes,  uiNumQuatPlanes, bRenderFromLeft );
  }
};

Void TRenTop::xCreateAlphaMapPlane(PelImagePlane** apcFilledPlanes,  PelImagePlane** apcAlphaPlanes,  UInt uiNumberOfPlanes, Bool bRenderFromLeft)
{
  Int iWidth            = apcFilledPlanes [0]->getWidth();
  Int iHeight           = apcFilledPlanes [0]->getHeight();

  for( UInt uiCurPlane = 0; uiCurPlane < uiNumberOfPlanes; uiCurPlane++)
  {
    AOF( iWidth         == apcFilledPlanes [uiCurPlane]->getWidth()  && iWidth        == apcAlphaPlanes[uiCurPlane]->getWidth() );
    AOF( iHeight        == apcFilledPlanes [uiCurPlane]->getHeight() && iHeight       == apcAlphaPlanes[uiCurPlane]->getHeight());
  }

  Int iBlendWidth  = m_iBlendHoleMargin;
  Int iMaxBlendLevel;

  if (!m_bBlendUseDistWeight )
  {
    iMaxBlendLevel = ( 1 <<  REN_VDWEIGHT_PREC ) ;

    if ( m_iBlendMode == 0)
    {
      iMaxBlendLevel >>= 1;
    }
  }
  else
  {
    if ( m_iBlendMode == 0)
    {
      iMaxBlendLevel = bRenderFromLeft ? (1 << REN_VDWEIGHT_PREC) - m_iBlendDistWeight :  m_iBlendDistWeight;
    }
    else
    {
      iMaxBlendLevel  = ( 1 <<  REN_VDWEIGHT_PREC );
    }
  }

  Int iWeightStep = (iBlendWidth > 0) ? ( iMaxBlendLevel + (iBlendWidth >> 1) ) / iBlendWidth : 0;

  for( UInt uiCurPlane = 0; uiCurPlane < uiNumberOfPlanes; uiCurPlane++)
  {
    Int iFilledStride   = apcFilledPlanes [uiCurPlane]->getStride();
    Int iAlphaStride    = apcAlphaPlanes  [uiCurPlane]->getStride();

    Pel* pcFilledData = apcFilledPlanes   [uiCurPlane]->getPlaneData();
    Pel* pcAlphaData  = apcAlphaPlanes    [uiCurPlane]->getPlaneData();

    for(Int iYPos = 0; iYPos < iHeight; iYPos++)
    {
      for(Int iXPos = 0 ; iXPos < iWidth; iXPos++ )
      {
        if (pcFilledData[iXPos] == REN_IS_HOLE)
        {
          while( (pcFilledData[iXPos] == REN_IS_HOLE) && (iXPos < iWidth) )
          {
            pcAlphaData[iXPos] = REN_IS_HOLE;
            iXPos++;
          }

          if ( iXPos >= iWidth )
            continue;

          Int iWeight = 0;
          Int iLastFillPos = iXPos + iBlendWidth;

          while( (pcFilledData[iXPos] != REN_IS_HOLE) && (iXPos < iWidth) && (iXPos < iLastFillPos) )
          {
            AOF(  iWeight <= (1 << REN_VDWEIGHT_PREC) );
            pcAlphaData[iXPos]  = (iWeight == 0) ? 1 : iWeight;
            iWeight += iWeightStep;
            iXPos++;
          }
        }
        else
        {
          pcAlphaData[iXPos] = pcFilledData[iXPos];
        }
      }
      pcAlphaData    += iAlphaStride;
      pcFilledData   += iFilledStride;
    }
  }
}

Void TRenTop::xRemBoundaryNoise(PelImage* pcInImage, PelImage* pcFilledImage, PelImage* pcOutImage, Bool bRenderFromLeft )
{
  if (pcInImage != pcOutImage)
  {
    pcOutImage->assign(pcInImage);
  }

  UInt uiNumFullPlanes = pcInImage->getNumberOfFullPlanes();
  UInt uiNumQuatPlanes = pcInImage->getNumberOfQuaterPlanes();

  AOF( uiNumFullPlanes == pcOutImage->getNumberOfFullPlanes  () );
  AOF( uiNumQuatPlanes == pcOutImage->getNumberOfQuaterPlanes() );

  xRemBoundaryNoisePlane( pcInImage->getPlanes(), pcFilledImage->getPlane(0), pcOutImage->getPlanes(),  uiNumFullPlanes, bRenderFromLeft  );

  if (uiNumQuatPlanes > 0)
  {
    AOF(  pcFilledImage->getNumberOfPlanes() > 1);
    xRemBoundaryNoisePlane( pcInImage->getPlanes()+uiNumFullPlanes, pcFilledImage->getPlane(1), pcOutImage->getPlanes() + uiNumFullPlanes,  uiNumQuatPlanes, bRenderFromLeft );
  }
};

Void TRenTop::xRemBoundaryNoisePlane(PelImagePlane** apcInputPlanes,  PelImagePlane* pcFilledPlane, PelImagePlane** apcOutputPlanes, UInt uiNumberOfPlanes, Bool bRenderFromLeft)
{
  Int iWidth        = apcOutputPlanes[0]->getWidth();
  Int iHeight       = apcInputPlanes [0]->getHeight();

  Int iInputStride  = apcInputPlanes [0]->getStride();
  Int iOutputStride = apcOutputPlanes[0]->getStride();

  Int iFilledStride = pcFilledPlane->getStride();

  Pel** apcInputData  = new Pel*[ uiNumberOfPlanes ];
  Pel** apcOutputData = new Pel*[ uiNumberOfPlanes ];
  Pel*   pcFilledData = pcFilledPlane->getPlaneData();

  for( UInt uiCurPlane = 0; uiCurPlane < uiNumberOfPlanes; uiCurPlane++)
  {
    apcInputData   [uiCurPlane] = apcInputPlanes [uiCurPlane]->getPlaneData();
    apcOutputData  [uiCurPlane] = apcOutputPlanes[uiCurPlane]->getPlaneData();
    AOF( iWidth        == apcInputPlanes [uiCurPlane]->getWidth()  && iWidth        == apcOutputPlanes[uiCurPlane]->getWidth() );
    AOF( iHeight       == apcInputPlanes [uiCurPlane]->getHeight() && iHeight       == apcOutputPlanes[uiCurPlane]->getHeight());
    AOF( iInputStride  == apcInputPlanes [uiCurPlane]->getStride() && iOutputStride == apcOutputPlanes[uiCurPlane]->getStride());
  }

  Int iRemovalWidth  = m_iBlendHoleMargin;
  AOT(iRemovalWidth > 6);  // GT: insufficent padding

  for(Int iYPos = 0; iYPos < iHeight; iYPos++)
  {
    for(Int iXPos = iWidth-1; iXPos >= 0; iXPos-- )
    {
      if (pcFilledData[iXPos] == REN_IS_HOLE)
      {
        Int iSourcePos = iXPos + 1;

        // Get New Value
        while( (pcFilledData[iSourcePos] != REN_IS_HOLE) && ( iSourcePos < iWidth) && ( iSourcePos < iXPos + iRemovalWidth  ) ) iSourcePos++;

        if (iSourcePos == iWidth || pcFilledData[iSourcePos] != REN_IS_HOLE )
          iSourcePos--;

        Int iXPosRem = iSourcePos - 1;

        // Remove
        while( iXPosRem > iXPos)
        {
          for( UInt uiCurPlane = 0; uiCurPlane < uiNumberOfPlanes; uiCurPlane++)
          {
            apcOutputData[uiCurPlane][iXPosRem] = apcInputData[uiCurPlane][iSourcePos];
          }

          iXPosRem--;
        }

        // Skip Hole
        while( (pcFilledData[iXPos] == REN_IS_HOLE) && ( iXPos > 0) ) iXPos--;
      }
    }

    for( UInt uiCurPlane = 0; uiCurPlane < uiNumberOfPlanes; uiCurPlane++)
    {
      apcOutputData[uiCurPlane] += iOutputStride;
      apcInputData [uiCurPlane] += iInputStride;
    }
    pcFilledData += iFilledStride;
  }
  delete[] apcInputData;
  delete[] apcOutputData;
}

Void TRenTop::xFillPlaneHoles(PelImagePlane** apcInputPlanes,  PelImagePlane* pcFilledPlane, PelImagePlane** apcOutputPlanes, UInt uiNumberOfPlanes, Bool bRenderFromLeft)
{
  Int iWidth        = apcOutputPlanes[0]->getWidth();
  Int iHeight       = apcInputPlanes [0]->getHeight();

  Int iInputStride  = apcInputPlanes [0]->getStride();
  Int iOutputStride = apcOutputPlanes[0]->getStride();

  Int iFilledStride = pcFilledPlane->getStride();

  Pel** apcInputData  = new Pel*[ uiNumberOfPlanes ];
  Pel** apcOutputData = new Pel*[ uiNumberOfPlanes ];
  Pel*   pcFilledData = pcFilledPlane->getPlaneData();

  for( UInt uiCurPlane = 0; uiCurPlane < uiNumberOfPlanes; uiCurPlane++)
  {
    apcInputData   [uiCurPlane] = apcInputPlanes [uiCurPlane]->getPlaneData();
    apcOutputData  [uiCurPlane] = apcOutputPlanes[uiCurPlane]->getPlaneData();
    AOF( iWidth        == apcInputPlanes [uiCurPlane]->getWidth()  && iWidth        == apcOutputPlanes[uiCurPlane]->getWidth() );
    AOF( iHeight       == apcInputPlanes [uiCurPlane]->getHeight() && iHeight       == apcOutputPlanes[uiCurPlane]->getHeight());
    AOF( iInputStride  == apcInputPlanes [uiCurPlane]->getStride() && iOutputStride == apcOutputPlanes[uiCurPlane]->getStride());
  }

  for(Int iYPos = 0; iYPos < iHeight; iYPos++)
  {
    if ( !m_bInstantHoleFilling )
    {
    for(Int iXPos = 0 ; iXPos < iWidth; iXPos++ )
    {
      if (pcFilledData[iXPos] == REN_IS_HOLE)
      {
          Int iSourcePos;
          Int iLastFillPos;

        Int iXPosSearch = iXPos;
        while( (pcFilledData[iXPosSearch] == REN_IS_HOLE) && (iXPosSearch < iWidth) ) iXPosSearch++;

          if ( iXPosSearch >= iWidth )
        {
            continue;
          }
          else
          {
            iSourcePos   = iXPosSearch;
            iLastFillPos = iXPosSearch-1;
          }

        while( iXPos <= iLastFillPos)
        {
          for( UInt uiCurPlane = 0; uiCurPlane < uiNumberOfPlanes; uiCurPlane++)
          {
            apcOutputData[uiCurPlane][iXPos] = apcInputData[uiCurPlane][iSourcePos];
          }
          iXPos++;
        }
        }
      }
    }

    // Fill Right Gap
    Int iXPosSearch = iWidth -1;
    while( (pcFilledData[iXPosSearch] == REN_IS_HOLE) && (iXPosSearch >= 0) ) iXPosSearch--;
    if ( iXPosSearch < 0) iXPosSearch++;

    Int iSourcePos = iXPosSearch;

    for( Int iXPos = iSourcePos + 1; iXPos <  iWidth; iXPos++)
    {
      for( UInt uiCurPlane = 0; uiCurPlane < uiNumberOfPlanes; uiCurPlane++)
      {
        apcOutputData[uiCurPlane][iXPos] = apcInputData[uiCurPlane][iSourcePos];
      }
    }

    // Fill Left Gap
    iXPosSearch = 0;
    while( (pcFilledData[iXPosSearch] == REN_IS_HOLE) && (iXPosSearch < iWidth) ) iXPosSearch++;
    if ( iXPosSearch >= iWidth) iXPosSearch--;

    iSourcePos = iXPosSearch;

    for( Int iXPos = iSourcePos - 1; iXPos >= 0; iXPos--)
    {
      for( UInt uiCurPlane = 0; uiCurPlane < uiNumberOfPlanes; uiCurPlane++)
      {
        apcOutputData[uiCurPlane][iXPos] = apcInputData[uiCurPlane][iSourcePos];
      }
    }

    // Go to next line
    for( UInt uiCurPlane = 0; uiCurPlane < uiNumberOfPlanes; uiCurPlane++)
    {
      apcOutputData[uiCurPlane] += iOutputStride;
      apcInputData [uiCurPlane] += iInputStride;
    }
    pcFilledData += iFilledStride;
  }
  delete[] apcInputData;
  delete[] apcOutputData;
}

Void TRenTop::xPostProcessImage(PelImage* pcInImage, PelImage* pcOutImage)
{
  if ( m_iPostProcMode == eRenPostProNone )
    return;

  PelImage* pcTemp;

  if (pcInImage == pcOutImage)
  {
    pcTemp = pcOutImage->create();
  }
  else
  {
    pcTemp = pcOutImage;
  }

  pcTemp->assign(pcInImage);

  switch ( m_iPostProcMode )
  {
  case eRenPostProMed:
    TRenFilter<REN_BIT_DEPTH>::lineMedian3(pcTemp);
    break;
  case eRenPostProNone:
    break;
  default:
    assert(0);
  }

  if (pcInImage == pcOutImage)
  {
    pcOutImage->assign(pcTemp);
    delete pcTemp;
  };
}


Void TRenTop::xCutPlaneMargin( PelImagePlane* pcImagePlane, Pel cFill, UInt uiScale )
{
  UInt uiWidth  = pcImagePlane->getWidth();
  UInt uiHeight = pcImagePlane->getHeight();

  UInt uiStride    = pcImagePlane->getStride();
  Pel* pcPlaneData = pcImagePlane->getPlaneData();

  UInt uiCutLeft  =           m_auiCut[0] / uiScale;
  UInt uiCutRight = uiWidth - m_auiCut[1] / uiScale;

  for(UInt uiYPos = 0; uiYPos < uiHeight; uiYPos++)
  {
    for(UInt uiXPos = 0; uiXPos < (UInt) uiWidth ; uiXPos++)
    {
      if ( ( uiXPos < uiCutLeft  )  || (  uiXPos >=  uiCutRight )  )
      {
        pcPlaneData[uiXPos ] = cFill;
      }
    }
    pcPlaneData += uiStride;
  }
};

Void TRenTop::xCutMargin( PelImage* pcInputImage )
{
  if  ( ( m_auiCut[0] == 0 ) && ( m_auiCut[1] == 0 ) )
  {
    return;
  };

  UInt uiCurPlane = 0;
  for (; uiCurPlane < pcInputImage->getNumberOfFullPlanes(); uiCurPlane++ )
  {
    xCutPlaneMargin( pcInputImage->getPlane(uiCurPlane), (Pel) 0  , 1 );
  }

  for (; uiCurPlane < pcInputImage->getNumberOfPlanes(); uiCurPlane++ )
  {
    xCutPlaneMargin( pcInputImage->getPlane(uiCurPlane), (Pel) 128  , 2 );
  }

};


Void TRenTop::xEnhSimilarity( PelImage* pcLeftImage, PelImage* pcRightImage, PelImage* pcFilledLeft, PelImage* pcFilledRight )
{
  if (m_iSimEnhBaseView == 0)
    return;

  UInt uiNumFullPlanes = pcLeftImage->getNumberOfFullPlanes();
  UInt uiNumQuatPlanes = pcLeftImage->getNumberOfQuaterPlanes();

  if (uiNumQuatPlanes > 0)
  {
    assert( pcFilledLeft ->getNumberOfPlanes() > 1);
    assert( pcFilledRight->getNumberOfPlanes() > 1);
  };

  xEnhSimilarityPlane ( pcLeftImage->getPlanes()                , pcRightImage->getPlanes()                , pcFilledLeft->getPlane(0), pcFilledRight->getPlane(0), uiNumFullPlanes);
  if (uiNumQuatPlanes > 0)
  {
    xEnhSimilarityPlane ( pcLeftImage->getPlanes()+uiNumFullPlanes, pcRightImage->getPlanes()+uiNumFullPlanes, pcFilledLeft->getPlane(1), pcFilledRight->getPlane(1), uiNumQuatPlanes);
  }
}

Void TRenTop::xEnhSimilarityPlane       ( PelImagePlane** apcLeftPlane, PelImagePlane** apcRightPlane, PelImagePlane* pcFilledLeftPlane, PelImagePlane* pcFilledRightPlane, UInt uiNumberOfPlanes )
{  
  AOT( m_iSimEnhBaseView != 1 && m_iSimEnhBaseView != 2 );
  Int iWidth  = (*apcRightPlane)->getWidth ();
  Int iHeight = (*apcRightPlane)->getHeight();

  Int* aiHistLeft  = new Int[ ((Int64)1 ) << REN_BIT_DEPTH ];
  Int* aiHistRight = new Int[ ((Int64)1 ) << REN_BIT_DEPTH ];
  Pel* aiConvLUT   = new Pel[ ((Int64)1 ) << REN_BIT_DEPTH ];

  for (UInt uiCurPlane = 0; uiCurPlane < uiNumberOfPlanes; uiCurPlane++ )
  {
    for (Int iCurVal = 0 ; iCurVal < ( 1 << REN_BIT_DEPTH ); iCurVal++)
    {
      aiHistLeft [iCurVal] = 0;
      aiHistRight[iCurVal] = 0;
    }

    Pel* pcFilledRightData = pcFilledRightPlane    ->getPlaneData();
    Pel* pcRightImageData  = (*apcRightPlane )     ->getPlaneData();

    Pel* pcFilledLeftData  = pcFilledLeftPlane     ->getPlaneData();
    Pel* pcLeftImageData   = (*apcLeftPlane)       ->getPlaneData();

 

    for (UInt uiYPos = 0; uiYPos < iHeight; uiYPos++ )
    {
      for (UInt uiXPos = 0; uiXPos < iWidth; uiXPos++ )
      {
          if      ( pcFilledLeftData[uiXPos] == REN_IS_FILLED &&  pcFilledRightData[uiXPos] == REN_IS_FILLED )
          {
            aiHistLeft [pcLeftImageData   [uiXPos] ]++;
            aiHistRight[pcRightImageData  [uiXPos] ]++;
          }
      }


      pcFilledRightData +=    pcFilledRightPlane  ->getStride();
      pcRightImageData  += (*apcRightPlane)       ->getStride();

      pcFilledLeftData  +=    pcFilledLeftPlane   ->getStride();
      pcLeftImageData   +=  (*apcLeftPlane)       ->getStride();
    }

    Int iCumSumChange  = 0;
    Int iCumSumBase    = 0;
    Int iCurBaseVal    = 0;
    Int iCurChangeVal  = 0;

    Int* aiHistChange  = (m_iSimEnhBaseView == 2 ) ? aiHistLeft  : aiHistRight;
    Int* aiHistBase    = (m_iSimEnhBaseView == 2 ) ? aiHistRight : aiHistLeft ;

    iCumSumChange += aiHistChange[iCurChangeVal];
    iCumSumBase   += aiHistBase  [iCurBaseVal]  ;

    Int iCheckSumLeft  = 0;
    Int iCheckSumRight = 0;

    for (Int iCurVal = 0 ; iCurVal < ( 1 << REN_BIT_DEPTH ); iCurVal++)
    {
      iCheckSumLeft  += aiHistLeft [iCurVal];
      iCheckSumRight += aiHistRight[iCurVal];
    }


    while( iCurChangeVal < ( 1 << REN_BIT_DEPTH ) )
    {
      if ( iCumSumBase == iCumSumChange )
      {
        aiConvLUT[iCurChangeVal] = std::min( iCurBaseVal,  ( 1 << REN_BIT_DEPTH ) - 1 );
        iCurBaseVal  ++;
        iCurChangeVal++;
        iCumSumChange += aiHistChange[iCurChangeVal];
        if (iCurBaseVal <  ( 1 << REN_BIT_DEPTH ) )
        {
          iCumSumBase   += aiHistBase  [iCurBaseVal]  ;
        }
      }
      else if ( iCumSumBase < iCumSumChange )
      {
        iCurBaseVal++;
        if (iCurBaseVal < ( 1 << REN_BIT_DEPTH ) )
        {
          iCumSumBase   += aiHistBase  [iCurBaseVal]  ;
        }
      }
      else if ( iCumSumBase > iCumSumChange)
      {
        aiConvLUT[iCurChangeVal] = std::min(iCurBaseVal, ( 1 << REN_BIT_DEPTH )-1);
        iCurChangeVal++;
        iCumSumChange += aiHistChange  [iCurChangeVal]  ;
      }
    }

    Pel* pcChangeImageData   = ( ( m_iSimEnhBaseView == 2 ) ? (*apcLeftPlane) : (*apcRightPlane) )->getPlaneData();
    Int  iChangeImageStride  = ( ( m_iSimEnhBaseView == 2 ) ? (*apcLeftPlane) : (*apcRightPlane) )->getStride   ();

    for (UInt uiYPos = 0; uiYPos < iHeight; uiYPos++ )
    {
      for (UInt uiXPos = 0; uiXPos < iWidth; uiXPos++ )
      {
          pcChangeImageData  [uiXPos] = aiConvLUT[ pcChangeImageData[uiXPos]];
      }
      pcChangeImageData   +=  iChangeImageStride;
    }

    apcRightPlane ++;
    apcLeftPlane  ++;

  }

delete[] aiHistLeft ;
delete[] aiHistRight;
delete[] aiConvLUT  ;
}


Void TRenTop::xBlend( PelImage* pcLeftImage, PelImage* pcRightImage, PelImage* pcFilledLeft, PelImage* pcFilledRight, PelImage* pcLeftDepth, PelImage* pcRightDepth, PelImage* pcOutputImage )
{
  UInt uiNumFullPlanes = pcLeftImage->getNumberOfFullPlanes();
  UInt uiNumQuatPlanes = pcLeftImage->getNumberOfQuaterPlanes();

  assert( uiNumFullPlanes == pcRightImage->getNumberOfFullPlanes  () && uiNumFullPlanes == pcOutputImage->getNumberOfFullPlanes    ());
  assert( uiNumQuatPlanes == pcRightImage->getNumberOfQuaterPlanes() && uiNumQuatPlanes == pcOutputImage->getNumberOfQuaterPlanes  ());

  if (uiNumQuatPlanes > 0)
  {
    assert( pcLeftDepth ->getNumberOfPlanes() > 1 || pcFilledLeft ->getNumberOfPlanes() > 1);
    assert( pcRightDepth->getNumberOfPlanes() > 1 || pcFilledRight->getNumberOfPlanes() > 1);
  };

  switch (m_iBlendMode)
  {
  case eRenBlendAverg:
  case eRenBlendDepthFirst:
    xBlendPlanesAvg( pcLeftImage->getPlanes()                , pcRightImage->getPlanes()                , pcFilledLeft->getPlane(0), pcFilledRight->getPlane(0), pcLeftDepth->getPlane(0), pcRightDepth->getPlane(0), pcOutputImage->getPlanes(), uiNumFullPlanes);
    if (uiNumQuatPlanes > 0)
    {
      xBlendPlanesAvg( pcLeftImage->getPlanes()+uiNumFullPlanes, pcRightImage->getPlanes()+uiNumFullPlanes, pcFilledLeft->getPlane(1), pcFilledRight->getPlane(1), pcLeftDepth->getPlane(1), pcRightDepth->getPlane(1), pcOutputImage->getPlanes()+uiNumFullPlanes, uiNumQuatPlanes);
    }
    break;
  case eRenBlendLeft:
  case eRenBlendRight:
    xBlendPlanesOneView( pcLeftImage->getPlanes()                , pcRightImage->getPlanes()                , pcFilledLeft->getPlane(0), pcFilledRight->getPlane(0), pcLeftDepth->getPlane(0), pcRightDepth->getPlane(0), pcOutputImage->getPlanes(), uiNumFullPlanes);
    if (uiNumQuatPlanes > 0)
    {
      xBlendPlanesOneView( pcLeftImage->getPlanes()+uiNumFullPlanes, pcRightImage->getPlanes()+uiNumFullPlanes, pcFilledLeft->getPlane(1), pcFilledRight->getPlane(1), pcLeftDepth->getPlane(1), pcRightDepth->getPlane(1), pcOutputImage->getPlanes()+uiNumFullPlanes, uiNumQuatPlanes);
    }
    break;
  }
}

Void TRenTop::xBlendPlanesOneView( PelImagePlane** apcLeftPlane, PelImagePlane** apcRightPlane, PelImagePlane* pcFilledLeftPlane, PelImagePlane* pcFilledRightPlane, PelImagePlane* pcLeftDepthPlane, PelImagePlane* pcRightDepthPlane, PelImagePlane** apcOutputImagePlane, UInt uiNumberOfPlanes )
{
  for (UInt uiCurPlane = 0; uiCurPlane < uiNumberOfPlanes; uiCurPlane++ )
  {
    Pel* pcFilledRightData = pcFilledRightPlane    ->getPlaneData();
    Pel* pcRightImageData  = (*apcRightPlane )     ->getPlaneData();
    Pel* pcRightDepthData  = pcRightDepthPlane     ->getPlaneData();

    Pel* pcFilledLeftData  = pcFilledLeftPlane     ->getPlaneData();
    Pel* pcLeftImageData   = (*apcLeftPlane)       ->getPlaneData();
    Pel* pcLeftDepthData   = pcLeftDepthPlane      ->getPlaneData();
    Pel* pcOutputData      = (*apcOutputImagePlane)->getPlaneData();

    for (UInt uiYPos = 0; uiYPos < (*apcOutputImagePlane)->getHeight(); uiYPos++ )
    {
      for (UInt uiXPos = 0; uiXPos < (*apcOutputImagePlane)->getWidth(); uiXPos++ )
      {
        if      (m_iBlendMode == eRenBlendLeft  )
        {
          if      ( pcFilledLeftData[uiXPos] == REN_IS_FILLED ||  pcFilledRightData[uiXPos] == REN_IS_HOLE )
          {
            pcOutputData[uiXPos] = pcLeftImageData[uiXPos];
          }
          else if ( pcFilledLeftData[uiXPos] == REN_IS_HOLE )
          {
            pcOutputData[uiXPos] = pcRightImageData[uiXPos];
          }
          else
          {
            pcOutputData[uiXPos] = pcRightImageData[uiXPos] +  (Pel) (  ( (Int) ( pcLeftImageData[uiXPos] - pcRightImageData[uiXPos] ) * pcFilledLeftData[uiXPos] + (1 << (REN_VDWEIGHT_PREC - 1)) ) >> REN_VDWEIGHT_PREC );
          }
        }
        else if ( m_iBlendMode == eRenBlendRight )
        {
          if      ( pcFilledRightData[uiXPos] == REN_IS_FILLED || pcFilledLeftData[uiXPos] == REN_IS_HOLE )
          {
            pcOutputData[uiXPos] = pcRightImageData[uiXPos];
          }
          else if ( pcFilledRightData[uiXPos] == REN_IS_HOLE )
          {
            pcOutputData[uiXPos] = pcLeftImageData[uiXPos];
          }
          else
          {
            pcOutputData[uiXPos] = pcLeftImageData[uiXPos] +  (Pel) (  ( (Int) ( pcRightImageData[uiXPos] - pcLeftImageData[uiXPos] ) * pcFilledRightData[uiXPos] + (1 << (REN_VDWEIGHT_PREC - 1)) ) >> REN_VDWEIGHT_PREC );
          }
        }
        else
        {
          AOT(true);
        }
      }

      pcFilledRightData +=    pcFilledRightPlane  ->getStride();
      pcRightImageData  += (*apcRightPlane)       ->getStride();
      pcRightDepthData  +=    pcRightDepthPlane   ->getStride();

      pcFilledLeftData  +=    pcFilledLeftPlane   ->getStride();
      pcLeftImageData   +=  (*apcLeftPlane)       ->getStride();
      pcLeftDepthData   +=    pcLeftDepthPlane    ->getStride();
      pcOutputData      +=  (*apcOutputImagePlane)->getStride();
    }

    apcRightPlane ++;
    apcLeftPlane  ++;
    apcOutputImagePlane++;
  }
}

Void TRenTop::xBlendPlanesAvg( PelImagePlane** apcLeftPlane, PelImagePlane** apcRightPlane, PelImagePlane* pcFilledLeftPlane, PelImagePlane* pcFilledRightPlane, PelImagePlane* pcLeftDepthPlane, PelImagePlane* pcRightDepthPlane, PelImagePlane** apcOutputImagePlane, UInt uiNumberOfPlanes )
{
  for (UInt uiCurPlane = 0; uiCurPlane < uiNumberOfPlanes; uiCurPlane++ )
  {
    Pel* pcFilledRightData = pcFilledRightPlane   ->getPlaneData();
    Pel* pcRightVideoData  = (*apcRightPlane )    ->getPlaneData();
    Pel* pcRightDepthData  = pcRightDepthPlane    ->getPlaneData();

    Pel* pcFilledLeftData  = pcFilledLeftPlane    ->getPlaneData();
    Pel* pcLeftVideoData   = (*apcLeftPlane)      ->getPlaneData();
    Pel* pcLeftDepthData   = pcLeftDepthPlane     ->getPlaneData();

    Pel* pcOutputData      = (*apcOutputImagePlane)->getPlaneData();

    for (UInt uiYPos = 0; uiYPos < (*apcOutputImagePlane)->getHeight(); uiYPos++ )
    {
      for (UInt uiXPos = 0; uiXPos < (*apcOutputImagePlane)->getWidth(); uiXPos++ )
      {
        if      (  (pcFilledRightData[uiXPos] != REN_IS_HOLE ) && ( pcFilledLeftData[uiXPos] != REN_IS_HOLE) )
        {
          Int iDepthDifference  = m_piInvZLUTLeft[RemoveBitIncrement(pcLeftDepthData[uiXPos])] - m_piInvZLUTRight[RemoveBitIncrement(pcRightDepthData[uiXPos])];

          if ( abs ( iDepthDifference ) <= m_iBlendZThres )
          {
            if      ((pcFilledRightData[uiXPos] == REN_IS_FILLED) && ( pcFilledLeftData[uiXPos] != REN_IS_FILLED))
            {
              pcOutputData[uiXPos] = pcRightVideoData[uiXPos] +  (Pel) (  ( (Int) ( pcLeftVideoData[uiXPos] - pcRightVideoData[uiXPos] ) * (pcFilledLeftData[uiXPos]) + (1 << (REN_VDWEIGHT_PREC - 1)) ) >> REN_VDWEIGHT_PREC );
            }
            else if ((pcFilledRightData[uiXPos] != REN_IS_FILLED) && ( pcFilledLeftData[uiXPos] == REN_IS_FILLED))
            {
              pcOutputData[uiXPos] = pcLeftVideoData[uiXPos]  +  (Pel) (  ( (Int) ( pcRightVideoData[uiXPos] - pcLeftVideoData[uiXPos] ) * (pcFilledRightData[uiXPos]) + (1 << (REN_VDWEIGHT_PREC - 1)) ) >> REN_VDWEIGHT_PREC );
            }
            else
            {
              pcOutputData[uiXPos] = pcLeftVideoData[uiXPos]  +  (Pel) (  ( (Int) ( pcRightVideoData[uiXPos] - pcLeftVideoData[uiXPos] ) * m_iBlendDistWeight               + (1 << (REN_VDWEIGHT_PREC - 1)) ) >> REN_VDWEIGHT_PREC );
            }

          }
          else if ( iDepthDifference < 0 )
          {
            pcOutputData[uiXPos] = pcRightVideoData[uiXPos];
          }
          else
          {
            pcOutputData[uiXPos] = pcLeftVideoData[uiXPos];
          }
        }
        else if ( (pcFilledRightData[uiXPos] == REN_IS_HOLE) && (pcFilledLeftData[uiXPos] == REN_IS_HOLE))
        {
          pcOutputData[uiXPos] = m_piInvZLUTLeft[RemoveBitIncrement( pcLeftDepthData[uiXPos])]  < m_piInvZLUTRight[RemoveBitIncrement(pcRightDepthData[uiXPos])] ? pcLeftVideoData[uiXPos] : pcRightVideoData[uiXPos];
        }
        else
        {
          pcOutputData[uiXPos] =  (pcFilledLeftData[uiXPos] == REN_IS_HOLE) ? pcRightVideoData[uiXPos] : pcLeftVideoData[uiXPos];
        }
      }

      pcFilledRightData +=    pcFilledRightPlane  ->getStride();
      pcRightVideoData  += (*apcRightPlane)      ->getStride();
      pcRightDepthData  +=    pcRightDepthPlane   ->getStride();

      pcFilledLeftData  +=    pcFilledLeftPlane   ->getStride();
      pcLeftVideoData   +=  (*apcLeftPlane)       ->getStride();
      pcLeftDepthData   +=    pcLeftDepthPlane    ->getStride();
      pcOutputData      +=  (*apcOutputImagePlane)->getStride();
    };

    apcRightPlane ++;
    apcLeftPlane  ++;
    apcOutputImagePlane++;
  }
}

// Temporal Filter from Zhejiang University: (a little different from m16041: Temporal Improvement Method in View Synthesis)
Void TRenTop::temporalFilterVSRS( TComPicYuv* pcPicYuvVideoCur, TComPicYuv* pcPicYuvDepthCur, TComPicYuv* pcPicYuvVideoLast, TComPicYuv* pcPicYuvDepthLast, Bool bFirstFrame )
{
  Int iSADThres  = 100 ;  //threshold of sad in 4*4 block motion detection

  Int iWidth  = m_auiInputResolution[0];
  Int iHeight = m_auiInputResolution[1];

  //internal variables
  Int* piFlagMoving =  m_aiBlkMoving + 2;

  Int iVideoCurStride     = pcPicYuvVideoCur ->getStride( COMPONENT_Y );
  Int iVideoLastStride    = pcPicYuvVideoLast->getStride( COMPONENT_Y );
  Int iDepthCurStride     = pcPicYuvDepthCur ->getStride( COMPONENT_Y );
  Int iDepthLastStride    = pcPicYuvDepthLast->getStride( COMPONENT_Y );

  Pel* pcVideoCurData     = pcPicYuvVideoCur ->getAddr( COMPONENT_Y );
  Pel* pcVideoLastData    = pcPicYuvVideoLast->getAddr( COMPONENT_Y );
  Pel* pcDepthCurData     = pcPicYuvDepthCur ->getAddr( COMPONENT_Y );
  Pel* pcDepthLastData    = pcPicYuvDepthLast->getAddr( COMPONENT_Y );

  Pel* pcVideoCurDataFrm  = pcVideoCurData ;
  Pel* pcVideoLastDataFrm = pcVideoLastData;
  Pel* pcDepthCurDataFrm  = pcDepthCurData ;
  Pel* pcDepthLastDataFrm = pcDepthLastData;


  if( !bFirstFrame ) // first frame need not the weighting, but need to prepare the data
  {
    for ( Int iPosY = 0; iPosY < (iHeight >> 2); iPosY++)
    {
      //motion detection by SAD
      for ( Int iPosX = 0; iPosX < (iWidth >> 2);  iPosX++)
      {
        Int iSAD = 0;

        Pel* pcVideoCurDataBlk  = pcVideoCurDataFrm  + (iPosX << 2);
        Pel* pcVideoLastDataBlk = pcVideoLastDataFrm + (iPosX << 2);

        //GT: Check difference of block compared to last frame
        for( Int iCurPosY = 0; iCurPosY < 4; iCurPosY++)
        {
          for( Int iCurPosX = 0; iCurPosX < 4; iCurPosX++)
          {
            iSAD += abs( pcVideoLastDataBlk[iCurPosX] - pcVideoCurDataBlk[iCurPosX] );   //SAD
          }
          pcVideoLastDataBlk += iVideoLastStride;
          pcVideoCurDataBlk  += iVideoCurStride;
        }

        piFlagMoving[iPosX] = ( iSAD < iSADThres ) ? 0 : 1;
      }

      //temporal weighting according to motion detection result -- do a line
      for ( Int iPosX = 0; iPosX < (iWidth >> 2);  iPosX++)
      {
        //5 block
       Int iSumMoving = piFlagMoving[iPosX-2] + piFlagMoving[iPosX-1] + piFlagMoving[iPosX]   + piFlagMoving[iPosX+1] + piFlagMoving[iPosX+2];

        if( iSumMoving == 0 ) // if not moving
        {
          Pel* pcDepthCurDataBlk  = pcDepthCurDataFrm  + (iPosX << 2);
          Pel* pcDepthLastDataBlk = pcDepthLastDataFrm + (iPosX << 2);

          for( Int iCurPosY = 0; iCurPosY < 4; iCurPosY++)
          {
            for( Int iCurPosX = 0; iCurPosX < 4; iCurPosX++)
            { //Weight: 0.75
              Int iFilt = (( (pcDepthLastDataBlk[iCurPosX] << 1 ) + pcDepthLastDataBlk[iCurPosX] + pcDepthCurDataBlk[iCurPosX] + 2 ) >> 2 );
              assert( (iFilt >= 0) && (iFilt <  ( 1 << REN_BIT_DEPTH ) ) );
              pcDepthCurDataBlk[iCurPosX] = pcDepthLastDataBlk[iCurPosX];
              pcDepthCurDataBlk[iCurPosX] = iFilt;
            }

            pcDepthCurDataBlk  += iDepthCurStride;
            pcDepthLastDataBlk += iDepthLastStride;
          }
        }
      }

      pcDepthCurDataFrm  += ( iDepthCurStride  << 2);
      pcDepthLastDataFrm += ( iDepthLastStride << 2);
      pcVideoCurDataFrm  += ( iVideoCurStride  << 2);
      pcVideoLastDataFrm += ( iVideoLastStride << 2);
    }
  }
  pcPicYuvVideoCur->copyToPic( pcPicYuvVideoLast );
  pcPicYuvDepthCur->copyToPic( pcPicYuvDepthLast );
}

TRenTop::TRenTop()
{
  m_auiInputResolution[0] = 0;
  m_auiInputResolution[1] = 0;

  // Sub Pel Rendering
  m_iLog2SamplingFactor = 0;

  // ColorPlaneHandling
  m_bUVUp = true;


  //PreProcessing
  m_iPreProcMode         = eRenPreProNone;
  m_iPreFilterSize = 2;

  // Interpolation
  m_iInterpolationMode   = eRenIntFullPel;

  // Sim Enhancement
  m_iSimEnhBaseView      = 0;

  // Blending
  m_iBlendMode           = eRenBlendAverg;
  m_iBlendZThresPerc     = -1;
  m_bBlendUseDistWeight  = false;
  m_iBlendHoleMargin     = -1;

  m_iBlendZThres         = -1;
  m_iBlendDistWeight     = -1;

  // Hole Filling
  m_iHoleFillingMode     = eRenHFLWBackExt;
  m_bInstantHoleFilling  = false;

  // PostProcessing
  m_iPostProcMode        = eRenPostProNone;

  // Cut
  m_auiCut[0] = 0;
  m_auiCut[1] = 0;

  // Data
  m_uiSampledWidth = -1;

  // LUTs
  m_ppdShiftLUTLeft  = 0;
  m_ppdShiftLUTRight = 0;

  m_ppdShiftLUTRightMirror    = new Double*[2];
  m_ppdShiftLUTRightMirror[0] = new Double [257];
  m_ppdShiftLUTRightMirror[1] = new Double [257];

  m_adShiftLUTCur    = 0;

  m_ppiShiftLUTLeft  = 0;
  m_ppiShiftLUTRight = 0;
  m_ppiShiftLUTRightMirror    = new Int*[2];
  m_ppiShiftLUTRightMirror[0] = new Int[257];
  m_ppiShiftLUTRightMirror[1] = new Int[257];

  m_aiShiftLUTCur    = 0;
  m_piInvZLUTLeft  = new Int[257];
  m_piInvZLUTRight = new Int[257];

  // Buffers
  m_pcLeftInputImage   = 0;
  m_pcLeftInputDepth   = 0;
  m_pcLeftOutputImage  = 0;
  m_pcLeftOutputDepth  = 0;
  m_pcLeftFilled       = 0;

  m_pcRightInputImage  = 0;
  m_pcRightInputDepth  = 0;
  m_pcRightOutputImage = 0;
  m_pcRightOutputDepth = 0;
  m_pcRightFilled      = 0;

  m_pcOutputImage      = 0;
  m_pcOutputDepth      = 0;

  //Extrapolation
  m_pcInputImage       = 0;
  m_pcInputDepth       = 0;
  m_pcFilled           = 0;

  // SubPel
  m_aaiSubPelShift     = 0;

  // Temp
  m_pcTempImage        = 0;

  //Temporal Filter
  m_aiBlkMoving        = 0;
}


Void TRenTop::init(UInt uiImageWidth,
                   UInt uiImageHeight,
                   Bool  bExtrapolate,
                   UInt uiLog2SamplingFactor,
                   Int   iShiftLUTPrec,
                   Bool  bUVUp,
                   Int   iPreProcMode,
                   Int   iPreFilterKernelSize,
                   Int   iBlendMode,
                   Int   iBlendZThresPerc,
                   Bool  bBlendUseDistWeight,
                   Int   iBlendHoleMargin,
                   Int   iInterpolationMode,
                   Int   iHoleFillingMode,
                   Int   iPostProcMode,
                   Int   iUsedPelMapMarExt
                   )

{
  // Shift LUT Prec
  m_iRelShiftLUTPrec = iShiftLUTPrec - (Int) uiLog2SamplingFactor;

  // Sub Pel Rendering
  m_iLog2SamplingFactor = uiLog2SamplingFactor;

  // Extrapolation ?
  m_bExtrapolate = bExtrapolate;

  // ColorPlaneHandling
  m_bUVUp = bUVUp;

  //PreProcessing
  m_iPreProcMode = iPreProcMode;
  m_iPreFilterSize = iPreFilterKernelSize;

  // Interpolation
  m_iInterpolationMode = iInterpolationMode;

  //Blending
  m_iBlendMode          = iBlendMode;
  m_iBlendZThresPerc    = iBlendZThresPerc;
  m_bBlendUseDistWeight = bBlendUseDistWeight;
  m_iBlendHoleMargin    = iBlendHoleMargin;

  // Hole Filling
  m_iHoleFillingMode = iHoleFillingMode;

  m_bInstantHoleFilling   = (m_iInterpolationMode == eRenInt8Tap ) && (m_iHoleFillingMode != 0 );

  // PostProcessing
  m_iPostProcMode    = iPostProcMode;

  // Used pel map
  m_iUsedPelMapMarExt     = iUsedPelMapMarExt;

  // Cut
  m_auiCut[0] = 0;
  m_auiCut[1] = 0;

  m_auiInputResolution[0] = uiImageWidth;
  m_auiInputResolution[1] = uiImageHeight;

  if ( m_bExtrapolate )
  {
    PelImage*    pcDump        = 0;
    xGetDataPointers( m_pcInputImage, m_pcOutputImage, m_pcInputDepth, pcDump, m_pcFilled, false );
  }
  else
  {
    xGetDataPointers(m_pcLeftInputImage,  m_pcLeftOutputImage,  m_pcLeftInputDepth,  m_pcLeftOutputDepth,  m_pcLeftFilled,  true);
    xGetDataPointers(m_pcRightInputImage, m_pcRightOutputImage, m_pcRightInputDepth, m_pcRightOutputDepth, m_pcRightFilled, true);
    xGetDataPointerOutputImage(m_pcOutputImage, m_pcOutputDepth );
  }

  m_pcTempImage = new PelImage( m_auiInputResolution[0],m_auiInputResolution[1],1,2);

  // SubPelShiftLUT
  if (iInterpolationMode == eRenInt8Tap)
  {
    // SubPel Shift LUT
    Int iNumEntries = (1 << ( m_iRelShiftLUTPrec + 1) ) + 1 ;
    m_aaiSubPelShift = new Int*[ iNumEntries ];
    for (UInt uiEntry = 0; uiEntry < iNumEntries; uiEntry++)
    {
      m_aaiSubPelShift[uiEntry] = new Int[ iNumEntries ];
    }

    TRenFilter<REN_BIT_DEPTH>::setSubPelShiftLUT(m_iRelShiftLUTPrec, m_aaiSubPelShift, -1);
  }

  // Zheijang temporal filter
  m_aiBlkMoving    = new Int[ ( m_auiInputResolution[0] >> 2 ) + 4 ];
  m_aiBlkMoving[0] = 0;
  m_aiBlkMoving[1] = 0;
  m_aiBlkMoving[ ( m_auiInputResolution[0] >> 2 ) + 2 ] = 0;
  m_aiBlkMoving[ ( m_auiInputResolution[0] >> 2 ) + 3 ] = 0;
}


TRenTop::~TRenTop()
{
  if ( m_ppdShiftLUTRightMirror != NULL )
  {
    delete[] m_ppdShiftLUTRightMirror[0];
    delete[] m_ppdShiftLUTRightMirror[1];
    delete[] m_ppdShiftLUTRightMirror;
  };

  if ( m_ppiShiftLUTRightMirror != NULL )
  {
    delete[] m_ppiShiftLUTRightMirror[0];
    delete[] m_ppiShiftLUTRightMirror[1];
    delete[] m_ppiShiftLUTRightMirror;
  };

  if (m_piInvZLUTLeft      != NULL ) delete[] m_piInvZLUTLeft   ;
  if (m_piInvZLUTLeft      != NULL ) delete[] m_piInvZLUTRight  ;

  if (m_pcLeftInputImage   != NULL ) delete m_pcLeftInputImage  ;
  if (m_pcLeftInputDepth   != NULL ) delete m_pcLeftInputDepth  ;
  if (m_pcLeftOutputImage  != NULL ) delete m_pcLeftOutputImage ;
  if (m_pcLeftOutputDepth  != NULL ) delete m_pcLeftOutputDepth ;
  if (m_pcLeftFilled       != NULL ) delete m_pcLeftFilled      ;

  if (m_pcRightInputImage  != NULL ) delete m_pcRightInputImage ;
  if (m_pcRightInputDepth  != NULL ) delete m_pcRightInputDepth ;
  if (m_pcRightOutputImage != NULL ) delete m_pcRightOutputImage;
  if (m_pcRightOutputDepth != NULL ) delete m_pcRightOutputDepth;
  if (m_pcRightFilled      != NULL ) delete m_pcRightFilled     ;

  if (m_pcOutputImage      != NULL ) delete m_pcOutputImage     ;
  if (m_pcOutputDepth      != NULL ) delete m_pcOutputDepth     ;

  if (m_pcInputImage       != NULL ) delete m_pcInputImage      ;
  if (m_pcInputDepth       != NULL ) delete m_pcInputDepth      ;
  if (m_pcFilled           != NULL ) delete m_pcFilled          ;

  if (m_pcTempImage        != NULL ) delete m_pcTempImage       ;

  // SubPel LUT
  if ( m_aaiSubPelShift != NULL)
  {
    Int iNumEntries = (1 << ( m_iRelShiftLUTPrec + 1) ) + 1;
    for (UInt uiEntry = 0; uiEntry < iNumEntries; uiEntry++)
    {
      delete[] m_aaiSubPelShift[uiEntry];
    }
    delete[] m_aaiSubPelShift;
  }

  // Zheijang temporal filter
  if(m_aiBlkMoving         != NULL ) delete[] m_aiBlkMoving;
}
#endif // NH_3D

