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



#include <list>
#include <stdio.h>
#include <fcntl.h>
#include <assert.h>
#include <math.h>

#include "TAppRendererTop.h"

// ====================================================================================================================
// Constructor / destructor / initialization / destroy
// ====================================================================================================================

TAppRendererTop::TAppRendererTop()
{

}

TAppRendererTop::~TAppRendererTop()
{

}


Void TAppRendererTop::xCreateLib()
{
  Int iInteralBitDepth = g_uiBitDepth + g_uiBitIncrement;
  Int iFileBitDepth    = 8;
  m_pcRenTop = new TRenTop();

  for(Int iViewIdx=0; iViewIdx<m_iNumberOfInputViews; iViewIdx++)
  {
    TVideoIOYuv* pcVideoInput = new TVideoIOYuv;
    TVideoIOYuv* pcDepthInput = new TVideoIOYuv;

    pcVideoInput->open( m_pchVideoInputFileList[iViewIdx], false, iFileBitDepth, iInteralBitDepth );  // read mode
    pcDepthInput->open( m_pchDepthInputFileList[iViewIdx], false, iFileBitDepth, iInteralBitDepth );  // read mode

    m_apcTVideoIOYuvVideoInput.push_back( pcVideoInput );
    m_apcTVideoIOYuvDepthInput.push_back( pcDepthInput );
  }

  for(Int iViewIdx=0; iViewIdx<m_iNumberOfOutputViews; iViewIdx++)
  {
    TVideoIOYuv* pcSynthOutput = new TVideoIOYuv;
    pcSynthOutput->open( m_pchSynthOutputFileList[iViewIdx], true, iFileBitDepth, iInteralBitDepth );  // write mode
    m_apcTVideoIOYuvSynthOutput.push_back( pcSynthOutput );
  }
}


Void TAppRendererTop::xDestroyLib()
{
  delete m_pcRenTop;

  for ( Int iViewIdx = 0; iViewIdx < m_iNumberOfInputViews; iViewIdx++ )
  {
    m_apcTVideoIOYuvVideoInput[iViewIdx]->close();
    m_apcTVideoIOYuvDepthInput[iViewIdx]->close();

    delete m_apcTVideoIOYuvDepthInput[iViewIdx];
    delete m_apcTVideoIOYuvVideoInput[iViewIdx];
  };

  for ( Int iViewIdx = 0; iViewIdx < m_iNumberOfOutputViews; iViewIdx++ )
  {
    m_apcTVideoIOYuvSynthOutput[iViewIdx]->close();
    delete m_apcTVideoIOYuvSynthOutput[iViewIdx];
  };
}

Void TAppRendererTop::xInitLib()
{
    m_pcRenTop->init(
    m_iSourceWidth,
    m_iSourceHeight,
    (m_iRenderDirection != 0),
    m_iLog2SamplingFactor,
    m_iLog2SamplingFactor+m_iShiftPrecision,
    m_bUVUp,
    m_iPreProcMode,
    m_iPreFilterSize,
    m_iBlendMode,
    m_iBlendZThresPerc,
    m_bBlendUseDistWeight,
    m_iBlendHoleMargin,
    m_iInterpolationMode,
    m_iHoleFillingMode,
    m_iPostProcMode,
    m_iUsedPelMapMarExt
    );
}

// ====================================================================================================================
// Public member functions
// ====================================================================================================================



Void TAppRendererTop::render()
{
  xCreateLib();
  xInitLib();

  // Create Buffers Input Views;
  std::vector<TComPicYuv*> apcPicYuvBaseVideo;
  std::vector<TComPicYuv*> apcPicYuvBaseDepth;

  // TemporalImprovement Filter
  std::vector<TComPicYuv*> apcPicYuvLastBaseVideo;
  std::vector<TComPicYuv*> apcPicYuvLastBaseDepth;

  Int aiPad[2] = { 0, 0 };

  for ( UInt uiBaseView = 0; uiBaseView < m_iNumberOfInputViews; uiBaseView++ )
  {
    TComPicYuv* pcNewVideoPic = new TComPicYuv;
    TComPicYuv* pcNewDepthPic = new TComPicYuv;

    pcNewVideoPic->create( m_iSourceWidth, m_iSourceHeight, 1, 1, 1 );
    apcPicYuvBaseVideo.push_back(pcNewVideoPic);

    pcNewDepthPic->create( m_iSourceWidth, m_iSourceHeight, 1, 1, 1 );
    apcPicYuvBaseDepth.push_back(pcNewDepthPic);

    //Temporal improvement Filter
    if ( m_bTempDepthFilter )
    {
      pcNewVideoPic = new TComPicYuv;
      pcNewDepthPic = new TComPicYuv;

      pcNewVideoPic->create( m_iSourceWidth, m_iSourceHeight, 1, 1, 1 );
      apcPicYuvLastBaseVideo.push_back(pcNewVideoPic);

      pcNewDepthPic->create( m_iSourceWidth, m_iSourceHeight, 1, 1, 1 );
      apcPicYuvLastBaseDepth.push_back(pcNewDepthPic);
    }
  }

  // Create Buffer for synthesized View
  TComPicYuv* pcPicYuvSynthOut = new TComPicYuv;
  pcPicYuvSynthOut->create( m_iSourceWidth, m_iSourceHeight, 1, 1, 1 );

  Bool bAnyEOS = false;

  Int iNumOfRenderedFrames = 0;
  Int iFrame = 0;

  while ( ( ( iNumOfRenderedFrames < m_iFramesToBeRendered ) || ( m_iFramesToBeRendered == 0 ) ) && !bAnyEOS )
  {

    // read in depth and video
    for(Int iBaseViewIdx=0; iBaseViewIdx < m_iNumberOfInputViews; iBaseViewIdx++ )
    {
      m_apcTVideoIOYuvVideoInput[iBaseViewIdx]->read( apcPicYuvBaseVideo[iBaseViewIdx], aiPad  ) ;

      apcPicYuvBaseVideo[iBaseViewIdx]->extendPicBorder();

      bAnyEOS |= m_apcTVideoIOYuvVideoInput[iBaseViewIdx]->isEof();

      m_apcTVideoIOYuvDepthInput[iBaseViewIdx]->read( apcPicYuvBaseDepth[iBaseViewIdx], aiPad  ) ;
      apcPicYuvBaseDepth[iBaseViewIdx]->extendPicBorder();
      bAnyEOS |= m_apcTVideoIOYuvDepthInput[iBaseViewIdx]->isEof();

      if ( m_bTempDepthFilter && (iFrame >= m_iFrameSkip) )
      {
        m_pcRenTop->temporalFilterVSRS( apcPicYuvBaseVideo[iBaseViewIdx], apcPicYuvBaseDepth[iBaseViewIdx], apcPicYuvLastBaseVideo[iBaseViewIdx], apcPicYuvLastBaseDepth[iBaseViewIdx], ( iFrame == m_iFrameSkip) );
      }
    }

    if ( iFrame < m_iFrameSkip ) // Skip Frames
    {
      std::cout << "Skipping Frame " << iFrame << std::endl;

      iFrame++;
      continue;
    }

    m_cCameraData.update( (UInt)iFrame - m_iFrameSkip );

    for(Int iSynthViewIdx=0; iSynthViewIdx < m_iNumberOfOutputViews; iSynthViewIdx++ )
    {
      Int  iLeftBaseViewIdx  = -1;
      Int  iRightBaseViewIdx = -1;

      Bool bIsBaseView = false;

      Int iRelDistToLeft;
      Bool bHasLRView = m_cCameraData.getLeftRightBaseView( iSynthViewIdx, iLeftBaseViewIdx, iRightBaseViewIdx, iRelDistToLeft, bIsBaseView );
      Bool bHasLView = ( iLeftBaseViewIdx != -1 );
      Bool bHasRView = ( iRightBaseViewIdx != -1 );
      Bool bRender   = true;

      Int  iBlendMode = m_iBlendMode;
      Int  iSimEnhBaseView = 0;

      switch( m_iRenderDirection )
      {
      /// INTERPOLATION
      case 0:
        AOF( bHasLRView || bIsBaseView );

        if ( !bHasLRView && bIsBaseView && m_iBlendMode == 0 )
        {
          bRender = false;
        }
        else
        {
          if ( bIsBaseView )
          {
            AOF( iLeftBaseViewIdx == iRightBaseViewIdx );
            Int iSortedBaseViewIdx = m_cCameraData.getBaseId2SortedId() [iLeftBaseViewIdx];

            if ( m_iBlendMode == 1 )
            {
              if ( iSortedBaseViewIdx - 1 >= 0 )
              {
                iLeftBaseViewIdx = m_cCameraData.getBaseSortedId2Id()[ iSortedBaseViewIdx - 1];
                bRender = true;
              }
              else
              {
                bRender = false;
              }
            }
            else if ( m_iBlendMode == 2 )
            {
              if ( iSortedBaseViewIdx + 1 < m_iNumberOfInputViews )
              {
                iRightBaseViewIdx = m_cCameraData.getBaseSortedId2Id()[ iSortedBaseViewIdx + 1];
                bRender = true;
              }
              else
              {
                bRender = false;
              }
            }
          }

          if ( m_iBlendMode == 3 )
          {
            if ( bIsBaseView && (iLeftBaseViewIdx == 0) )
            {
              bRender = false;
            }
            else
            {
              Int iDistLeft  = abs( m_cCameraData.getBaseId2SortedId()[0] - m_cCameraData.getBaseId2SortedId() [iLeftBaseViewIdx ]  );
              Int iDistRight = abs( m_cCameraData.getBaseId2SortedId()[0] - m_cCameraData.getBaseId2SortedId() [iRightBaseViewIdx]  );

              Int iFillViewIdx = iDistLeft > iDistRight ? iLeftBaseViewIdx : iRightBaseViewIdx;

              if( m_cCameraData.getBaseId2SortedId()[0] < m_cCameraData.getBaseId2SortedId() [iFillViewIdx] )
              {
                iBlendMode        = 1;
                iLeftBaseViewIdx  = 0;
                iRightBaseViewIdx = iFillViewIdx;
              }
              else
              {
                iBlendMode        = 2;
                iLeftBaseViewIdx  = iFillViewIdx;
                iRightBaseViewIdx = 0;
              }

            }
          }
          else
          {
            iBlendMode = m_iBlendMode;
          }
        }

        if ( m_bSimEnhance )
        {
          if ( m_iNumberOfInputViews == 3 && m_cCameraData.getRelSynthViewNumbers()[ iSynthViewIdx ] < VIEW_NUM_PREC  )
          {
            iSimEnhBaseView = 2; // Take middle view
          }
          else
          {
            iSimEnhBaseView = 1; // Take left view
          }
        }

          if ( bRender )
          {
          std::cout << "Rendering Frame "    << iFrame
                    << " of View "           << (Double) m_cCameraData.getSynthViewNumbers()[iSynthViewIdx    ] / VIEW_NUM_PREC
                    << "   Left BaseView: "  << (Double) m_cCameraData.getBaseViewNumbers() [iLeftBaseViewIdx ] / VIEW_NUM_PREC
                    << "   Right BaseView: " << (Double) m_cCameraData.getBaseViewNumbers() [iRightBaseViewIdx] / VIEW_NUM_PREC
                    << "   BlendMode: "      << iBlendMode
                    << std::endl;

          m_pcRenTop->setShiftLUTs(
            m_cCameraData.getSynthViewShiftLUTD()[iLeftBaseViewIdx ][iSynthViewIdx],
            m_cCameraData.getSynthViewShiftLUTI()[iLeftBaseViewIdx ][iSynthViewIdx],
            m_cCameraData.getBaseViewShiftLUTI ()[iLeftBaseViewIdx ][iRightBaseViewIdx],
            m_cCameraData.getSynthViewShiftLUTD()[iRightBaseViewIdx][iSynthViewIdx],
            m_cCameraData.getSynthViewShiftLUTI()[iRightBaseViewIdx][iSynthViewIdx],
            m_cCameraData.getBaseViewShiftLUTI ()[iRightBaseViewIdx][iLeftBaseViewIdx ],

            iRelDistToLeft
          );

          m_pcRenTop->interpolateView(
            apcPicYuvBaseVideo[iLeftBaseViewIdx ],
            apcPicYuvBaseDepth[iLeftBaseViewIdx ],
            apcPicYuvBaseVideo[iRightBaseViewIdx],
            apcPicYuvBaseDepth[iRightBaseViewIdx],
            pcPicYuvSynthOut,
            iBlendMode,
            iSimEnhBaseView
            );
        }
        else
        {
          AOT(iLeftBaseViewIdx != iRightBaseViewIdx );
          apcPicYuvBaseVideo[iLeftBaseViewIdx]->copyToPic( pcPicYuvSynthOut );
          std::cout << "Copied    Frame " << iFrame
                    << " of View "        << (Double) m_cCameraData.getSynthViewNumbers()[iSynthViewIdx] / VIEW_NUM_PREC
                    << "   (BaseView)  "    << std::endl;
        }

        break;
      /// EXTRAPOLATION FROM LEFT
      case 1:
        if ( !bHasLView ) // View to render is BaseView
        {
          bRender = false;
        }

          if (  bIsBaseView )
          {
          AOF( iLeftBaseViewIdx == iRightBaseViewIdx );
          Int iSortedBaseViewIdx = m_cCameraData.getBaseId2SortedId() [iLeftBaseViewIdx];
          if ( iSortedBaseViewIdx - 1 >= 0 )
          {
            iLeftBaseViewIdx = m_cCameraData.getBaseSortedId2Id()[ iSortedBaseViewIdx - 1];
          }
          else
          {
            std::cout << "Copied    Frame " << iFrame << " of BaseView " << (Double) m_cCameraData.getSynthViewNumbers()[iSynthViewIdx] / VIEW_NUM_PREC  << std::endl;
            apcPicYuvBaseVideo[iLeftBaseViewIdx]->copyToPic( pcPicYuvSynthOut ); // Copy Original
            bRender = false;
          }
        }


        if (bRender)
        {
          std::cout << "Rendering Frame " << iFrame << " of View " << (Double) m_cCameraData.getSynthViewNumbers()[iSynthViewIdx] / VIEW_NUM_PREC  << std::endl;
          m_pcRenTop->setShiftLUTs( m_cCameraData.getSynthViewShiftLUTD()[iLeftBaseViewIdx ][iSynthViewIdx],
            m_cCameraData.getSynthViewShiftLUTI()[iLeftBaseViewIdx ][iSynthViewIdx], NULL, NULL, NULL, NULL, -1 );
          m_pcRenTop->extrapolateView( apcPicYuvBaseVideo[iLeftBaseViewIdx ], apcPicYuvBaseDepth[iLeftBaseViewIdx ], pcPicYuvSynthOut, true );
        }
        break;
      /// EXTRAPOLATION FROM RIGHT
      case 2:            // extrapolation from right
        if ( !bHasRView ) // View to render is BaseView
        {
          bRender = false;
        }

          if (  bIsBaseView )
          {

          AOF( iLeftBaseViewIdx == iRightBaseViewIdx );
          Int iSortedBaseViewIdx = m_cCameraData.getBaseId2SortedId() [iLeftBaseViewIdx];
          if ( iSortedBaseViewIdx + 1 < m_iNumberOfInputViews )
          {
            iRightBaseViewIdx = m_cCameraData.getBaseSortedId2Id()[ iSortedBaseViewIdx + 1];
          }
          else
          {
            std::cout << "Copied    Frame " << iFrame << " of BaseView " << (Double) m_cCameraData.getSynthViewNumbers()[iSynthViewIdx] / VIEW_NUM_PREC  << std::endl;
            apcPicYuvBaseVideo[iLeftBaseViewIdx]->copyToPic( pcPicYuvSynthOut ); // Copy Original
            bRender = false;
          }
        }

        if ( bRender )
        {
          std::cout << "Rendering Frame " << iFrame << " of View " << (Double) m_cCameraData.getSynthViewNumbers()[iSynthViewIdx] / VIEW_NUM_PREC  << std::endl;
          m_pcRenTop->setShiftLUTs( NULL, NULL,NULL, m_cCameraData.getSynthViewShiftLUTD()[iRightBaseViewIdx ][iSynthViewIdx],
            m_cCameraData.getSynthViewShiftLUTI()[iRightBaseViewIdx ][iSynthViewIdx],NULL, iRelDistToLeft);
          m_pcRenTop->extrapolateView( apcPicYuvBaseVideo[iRightBaseViewIdx ], apcPicYuvBaseDepth[iRightBaseViewIdx ], pcPicYuvSynthOut, false);
        }
        break;
      }

      // Write Output

#if PIC_CROPPING
      m_apcTVideoIOYuvSynthOutput[m_bSweep ? 0 : iSynthViewIdx]->write( pcPicYuvSynthOut, 0, 0, 0, 0 );
#else
      m_apcTVideoIOYuvSynthOutput[m_bSweep ? 0 : iSynthViewIdx]->write( pcPicYuvSynthOut, aiPad );
#endif
    }
    iFrame++;
    iNumOfRenderedFrames++;
  }

  // Delete Buffers
  for ( UInt uiBaseView = 0; uiBaseView < m_iNumberOfInputViews; uiBaseView++ )
  {
    apcPicYuvBaseVideo[uiBaseView]->destroy();
    delete apcPicYuvBaseVideo[uiBaseView];

    apcPicYuvBaseDepth[uiBaseView]->destroy();
    delete apcPicYuvBaseDepth[uiBaseView];

    // Temporal Filter
    if ( m_bTempDepthFilter )
    {
      apcPicYuvLastBaseVideo[uiBaseView]->destroy();
      delete apcPicYuvLastBaseVideo[uiBaseView];

      apcPicYuvLastBaseDepth[uiBaseView]->destroy();
      delete apcPicYuvLastBaseDepth[uiBaseView];
    }
  }

  pcPicYuvSynthOut->destroy();
  delete pcPicYuvSynthOut;

  xDestroyLib();

}

Void TAppRendererTop::go()
{
  switch ( m_iRenderMode )
  {
  case 0:
    render();
    break;
  case 1:
    renderModel();
    break;
  case 10:
    renderUsedPelsMap( );
      break;

  default:
    AOT(true);
  }
}

Void TAppRendererTop::renderModel()
{
  if ( m_bUseSetupString )
  {
    xRenderModelFromString();
  }
  else
  {
    xRenderModelFromNums();
  }
}

Void TAppRendererTop::xRenderModelFromString()
{

    xCreateLib();
    xInitLib();

    // Create Buffers Input Views;
    std::vector<TComPicYuv*> apcPicYuvBaseVideo;
    std::vector<TComPicYuv*> apcPicYuvBaseDepth;


    for ( UInt uiBaseView = 0; uiBaseView < m_iNumberOfInputViews; uiBaseView++ )
    {
      TComPicYuv* pcNewVideoPic = new TComPicYuv;
      TComPicYuv* pcNewDepthPic = new TComPicYuv;

      pcNewVideoPic->create( m_iSourceWidth, m_iSourceHeight, 1, 1, 1 );
      apcPicYuvBaseVideo.push_back(pcNewVideoPic);

      pcNewDepthPic->create( m_iSourceWidth, m_iSourceHeight, 1, 1, 1 );
      apcPicYuvBaseDepth.push_back(pcNewDepthPic);
    }

    Int aiPad[2] = { 0, 0 };

    // Init Model
    TRenModel cCurModel;

    AOT( m_iLog2SamplingFactor != 0 );
    cCurModel.create( m_cRenModStrParser.getNumOfBaseViews(), m_cRenModStrParser.getNumOfModels(), m_iSourceWidth, m_iSourceHeight, m_iShiftPrecision, m_iBlendHoleMargin );

    for ( Int iViewIdx = 0; iViewIdx < m_iNumberOfInputViews; iViewIdx++ )
    {
      Int iNumOfModels   = m_cRenModStrParser.getNumOfModelsForView(iViewIdx, 1);

      for (Int iCurModel = 0; iCurModel < iNumOfModels; iCurModel++ )
      {
        Int iModelNum; Int iLeftViewNum; Int iRightViewNum; Int iDump; Int iOrgRefNum; Int iBlendMode;
        m_cRenModStrParser.getSingleModelData  ( iViewIdx, 1, iCurModel, iModelNum, iBlendMode, iLeftViewNum, iRightViewNum, iOrgRefNum, iDump ) ;
        cCurModel         .createSingleModel   ( iViewIdx, 1, iModelNum, iLeftViewNum, iRightViewNum, false, iBlendMode );

      }
    }

    // Create Buffer for synthesized View
    TComPicYuv* pcPicYuvSynthOut = new TComPicYuv;
    pcPicYuvSynthOut->create( m_iSourceWidth, m_iSourceHeight, 1, 1, 1 );

    Bool bAnyEOS = false;

    Int iNumOfRenderedFrames = 0;
    Int iFrame = 0;

    while ( ( ( iNumOfRenderedFrames < m_iFramesToBeRendered ) || ( m_iFramesToBeRendered == 0 ) ) && !bAnyEOS )
    {
      // read in depth and video
      for(Int iBaseViewIdx=0; iBaseViewIdx < m_iNumberOfInputViews; iBaseViewIdx++ )
      {
        m_apcTVideoIOYuvVideoInput[iBaseViewIdx]->read( apcPicYuvBaseVideo[iBaseViewIdx], aiPad  ) ;
        bAnyEOS |= m_apcTVideoIOYuvVideoInput[iBaseViewIdx]->isEof();

        m_apcTVideoIOYuvDepthInput[iBaseViewIdx]->read( apcPicYuvBaseDepth[iBaseViewIdx], aiPad  ) ;
        bAnyEOS |= m_apcTVideoIOYuvDepthInput[iBaseViewIdx]->isEof();
      }

      if ( iFrame < m_iFrameSkip )
      {
        continue;
      }


      for(Int iBaseViewIdx=0; iBaseViewIdx < m_iNumberOfInputViews; iBaseViewIdx++ )
      {
        TComPicYuv* pcPicYuvVideo = apcPicYuvBaseVideo[iBaseViewIdx];
        TComPicYuv* pcPicYuvDepth = apcPicYuvBaseDepth[iBaseViewIdx];
        Int iBaseViewSIdx = m_cCameraData.getBaseId2SortedId()[iBaseViewIdx ];
        cCurModel.setBaseView( iBaseViewSIdx, pcPicYuvVideo, pcPicYuvDepth, NULL, NULL );
      }

      for(Int iBaseViewIdx=0; iBaseViewIdx < m_iNumberOfInputViews; iBaseViewIdx++ )
      {
        m_cCameraData.update( (UInt)iFrame );

        // setup virtual views
        Int iBaseViewSIdx = m_cCameraData.getBaseId2SortedId()[iBaseViewIdx];

        cCurModel.setErrorMode( iBaseViewSIdx, 1, 0 );
        Int iNumOfSV  = m_cRenModStrParser.getNumOfModelsForView( iBaseViewSIdx, 1);
        for (Int iCurView = 0; iCurView < iNumOfSV; iCurView++ )
        {
          Int iOrgRefBaseViewSIdx;
          Int iLeftBaseViewSIdx;
          Int iRightBaseViewSIdx;
          Int iSynthViewRelNum;
          Int iModelNum;
          Int iBlendMode;

          m_cRenModStrParser.getSingleModelData(iBaseViewSIdx, 1, iCurView, iModelNum, iBlendMode, iLeftBaseViewSIdx, iRightBaseViewSIdx, iOrgRefBaseViewSIdx, iSynthViewRelNum );

          Int iLeftBaseViewIdx    = -1;
          Int iRightBaseViewIdx   = -1;

          TComPicYuv* pcPicYuvOrgRef  = NULL;
          Int**      ppiShiftLUTLeft  = NULL;
          Int**      ppiShiftLUTRight = NULL;
          Int**      ppiBaseShiftLUTLeft  = NULL;
          Int**      ppiBaseShiftLUTRight = NULL;


          Int        iDistToLeft      = -1;

          Int iSynthViewIdx = m_cCameraData.synthRelNum2Idx( iSynthViewRelNum );

          if ( iLeftBaseViewSIdx != -1 )
          {
            iLeftBaseViewIdx   = m_cCameraData.getBaseSortedId2Id()   [ iLeftBaseViewSIdx ];
            ppiShiftLUTLeft    = m_cCameraData.getSynthViewShiftLUTI()[ iLeftBaseViewIdx  ][ iSynthViewIdx  ];
          }

          if ( iRightBaseViewSIdx != -1 )
          {
            iRightBaseViewIdx  = m_cCameraData.getBaseSortedId2Id()   [iRightBaseViewSIdx ];
            ppiShiftLUTRight   = m_cCameraData.getSynthViewShiftLUTI()[ iRightBaseViewIdx ][ iSynthViewIdx ];
          }

          if ( iRightBaseViewSIdx != -1 && iLeftBaseViewSIdx != -1 )
          {

            ppiBaseShiftLUTLeft  = m_cCameraData.getBaseViewShiftLUTI() [ iLeftBaseViewIdx  ][ iRightBaseViewIdx ];
            ppiBaseShiftLUTRight = m_cCameraData.getBaseViewShiftLUTI() [ iRightBaseViewIdx ][ iLeftBaseViewIdx  ];
            iDistToLeft    = m_cCameraData.getRelDistLeft(  iSynthViewIdx , iLeftBaseViewIdx, iRightBaseViewIdx);
          }

          std::cout << "Rendering Frame " << iFrame << " of View " << (Double) m_cCameraData.getSynthViewNumbers()[iSynthViewIdx] / VIEW_NUM_PREC  << std::endl;

          cCurModel.setSingleModel( iModelNum, ppiShiftLUTLeft, ppiBaseShiftLUTLeft, ppiShiftLUTRight, ppiBaseShiftLUTRight, iDistToLeft, pcPicYuvOrgRef );

          Int iViewPos;
          if (iLeftBaseViewSIdx != -1 && iRightBaseViewSIdx != -1)
          {
            iViewPos = VIEWPOS_MERGED;
          }
          else if ( iLeftBaseViewSIdx != -1 )
          {
            iViewPos = VIEWPOS_LEFT;
          }
          else if ( iRightBaseViewSIdx != -1 )
          {
            iViewPos = VIEWPOS_RIGHT;
          }
          else
          {
            AOT(true);
          }

          cCurModel.getSynthVideo ( iModelNum, iViewPos, pcPicYuvSynthOut );

          // Write Output
#if PIC_CROPPING
          m_apcTVideoIOYuvSynthOutput[m_bSweep ? 0 : iModelNum]->write( pcPicYuvSynthOut, 0 ,0 ,0, 0 );
#else
          m_apcTVideoIOYuvSynthOutput[m_bSweep ? 0 : iModelNum]->write( pcPicYuvSynthOut, aiPad );
#endif
        }
      }
      iFrame++;
      iNumOfRenderedFrames++;
  }

    // Delete Buffers
    for ( UInt uiBaseView = 0; uiBaseView < m_iNumberOfInputViews; uiBaseView++ )
    {
      apcPicYuvBaseVideo[uiBaseView]->destroy();
      delete apcPicYuvBaseVideo[uiBaseView];

      apcPicYuvBaseDepth[uiBaseView]->destroy();
      delete apcPicYuvBaseDepth[uiBaseView];
}
    pcPicYuvSynthOut->destroy();
    delete pcPicYuvSynthOut;

    xDestroyLib();
}

Void TAppRendererTop::xRenderModelFromNums()
{
  xCreateLib();
  xInitLib();

  // Create Buffers Input Views;
  std::vector<TComPicYuv*> apcPicYuvBaseVideo;
  std::vector<TComPicYuv*> apcPicYuvBaseDepth;


  Int aiPad[2] = { 0, 0 };

  // Init Model
  TRenModel cCurModel;

  AOT( m_iLog2SamplingFactor != 0 );
  cCurModel.create( m_iNumberOfInputViews, m_iNumberOfOutputViews, m_iSourceWidth, m_iSourceHeight, m_iShiftPrecision, m_iBlendHoleMargin );

  for ( UInt uiBaseView = 0; uiBaseView < m_iNumberOfInputViews; uiBaseView++ )
  {
    TComPicYuv* pcNewVideoPic = new TComPicYuv;
    TComPicYuv* pcNewDepthPic = new TComPicYuv;

    pcNewVideoPic->create( m_iSourceWidth, m_iSourceHeight, 1, 1, 1 );
    apcPicYuvBaseVideo.push_back(pcNewVideoPic);

    pcNewDepthPic->create( m_iSourceWidth, m_iSourceHeight, 1, 1, 1 );
    apcPicYuvBaseDepth.push_back(pcNewDepthPic);
  }

  for(Int iSynthViewIdx=0; iSynthViewIdx < m_iNumberOfOutputViews; iSynthViewIdx++ )
  {
    Int  iLeftBaseViewIdx  = -1;
    Int  iRightBaseViewIdx = -1;
    Bool bIsBaseView = false;

    Int iRelDistToLeft;
    m_cCameraData.getLeftRightBaseView( iSynthViewIdx, iLeftBaseViewIdx, iRightBaseViewIdx, iRelDistToLeft,  bIsBaseView );

    if (m_iRenderDirection == 1 )
    {
      iRightBaseViewIdx = -1;
      AOT( iLeftBaseViewIdx == -1);
    }

    if (m_iRenderDirection == 2 )
    {
      iLeftBaseViewIdx = -1;
      AOT( iRightBaseViewIdx == -1);
    }

    Int iLeftBaseViewSIdx  = -1;
    Int iRightBaseViewSIdx = -1;

    if (iLeftBaseViewIdx != -1 )
    {
      iLeftBaseViewSIdx = m_cCameraData.getBaseId2SortedId()[iLeftBaseViewIdx];
    }

    if (iRightBaseViewIdx != -1 )
    {
      iRightBaseViewSIdx = m_cCameraData.getBaseId2SortedId()[iRightBaseViewIdx];
    }
    cCurModel.createSingleModel(-1, -1, iSynthViewIdx, iLeftBaseViewSIdx, iRightBaseViewSIdx, false, m_iBlendMode );
  }

  // Create Buffer for synthesized View
  TComPicYuv* pcPicYuvSynthOut = new TComPicYuv;
  pcPicYuvSynthOut->create( m_iSourceWidth, m_iSourceHeight, 1, 1, 1 );

  Bool bAnyEOS = false;

  Int iNumOfRenderedFrames = 0;
  Int iFrame = 0;

  while ( ( ( iNumOfRenderedFrames < m_iFramesToBeRendered ) || ( m_iFramesToBeRendered == 0 ) ) && !bAnyEOS )
  {
    // read in depth and video
    for(Int iBaseViewIdx=0; iBaseViewIdx < m_iNumberOfInputViews; iBaseViewIdx++ )
    {
      m_apcTVideoIOYuvVideoInput[iBaseViewIdx]->read( apcPicYuvBaseVideo[iBaseViewIdx], aiPad  ) ;
      bAnyEOS |= m_apcTVideoIOYuvVideoInput[iBaseViewIdx]->isEof();

      m_apcTVideoIOYuvDepthInput[iBaseViewIdx]->read( apcPicYuvBaseDepth[iBaseViewIdx], aiPad  ) ;
      bAnyEOS |= m_apcTVideoIOYuvDepthInput[iBaseViewIdx]->isEof();

      if ( iFrame >= m_iFrameSkip )
      {
        Int iBaseViewSIdx = m_cCameraData.getBaseId2SortedId()[iBaseViewIdx];
        cCurModel.setBaseView( iBaseViewSIdx, apcPicYuvBaseVideo[iBaseViewIdx], apcPicYuvBaseDepth[iBaseViewIdx], NULL, NULL );
      }
    }

    if ( iFrame < m_iFrameSkip ) // Skip Frames
    {
      iFrame++;
      continue;
    }

    m_cCameraData.update( (UInt)iFrame );

    for(Int iSynthViewIdx=0; iSynthViewIdx < m_iNumberOfOutputViews; iSynthViewIdx++ )
    {

      Int  iLeftBaseViewIdx  = -1;
      Int  iRightBaseViewIdx = -1;

      Bool bIsBaseView = false;

      Int iRelDistToLeft;
      Bool bHasLRView = m_cCameraData.getLeftRightBaseView( iSynthViewIdx, iLeftBaseViewIdx, iRightBaseViewIdx, iRelDistToLeft, bIsBaseView );
      Bool bHasLView = ( iLeftBaseViewIdx != -1 );
      Bool bHasRView = ( iRightBaseViewIdx != -1 );

      switch( m_iRenderDirection )
      {
        /// INTERPOLATION
      case 0:
        assert( bHasLRView || bIsBaseView );

        if ( !bHasLRView && bIsBaseView ) // View to render is BaseView
        {
          std::cout << "Copied    Frame " << iFrame << " of BaseView " << (Double) m_cCameraData.getSynthViewNumbers()[iSynthViewIdx] / VIEW_NUM_PREC  << std::endl;
          apcPicYuvBaseVideo[iLeftBaseViewIdx]->copyToPic( pcPicYuvSynthOut ); // Copy Original
        }
        else  // Render
        {
          std::cout << "Rendering Frame " << iFrame << " of View " << (Double) m_cCameraData.getSynthViewNumbers()[iSynthViewIdx] / VIEW_NUM_PREC  << std::endl;
          cCurModel.setSingleModel( iSynthViewIdx,
                                    m_cCameraData.getSynthViewShiftLUTI()[iLeftBaseViewIdx ][iSynthViewIdx]    ,
                                    m_cCameraData.getBaseViewShiftLUTI ()[iLeftBaseViewIdx ][iRightBaseViewIdx],
                                    m_cCameraData.getSynthViewShiftLUTI()[iRightBaseViewIdx][iSynthViewIdx]    ,
                                    m_cCameraData.getBaseViewShiftLUTI ()[iRightBaseViewIdx][iLeftBaseViewIdx] ,
                                    iRelDistToLeft,
                                    NULL );
          cCurModel.getSynthVideo ( iSynthViewIdx, VIEWPOS_MERGED, pcPicYuvSynthOut );
        }
        break;
        /// EXTRAPOLATION FROM LEFT
      case 1:

        if ( !bHasLView ) // View to render is BaseView
        {
          std::cout << "Copied    Frame " << iFrame << " of BaseView " << (Double) m_cCameraData.getSynthViewNumbers()[iSynthViewIdx] / VIEW_NUM_PREC  << std::endl;
          apcPicYuvBaseVideo[iLeftBaseViewIdx]->copyToPic( pcPicYuvSynthOut ); // Copy Original
        }
        else  // Render
        {
          std::cout << "Rendering Frame " << iFrame << " of View " << (Double) m_cCameraData.getSynthViewNumbers()[iSynthViewIdx] / VIEW_NUM_PREC  << std::endl;
          cCurModel.setSingleModel( iSynthViewIdx, m_cCameraData.getSynthViewShiftLUTI()[iLeftBaseViewIdx ][iSynthViewIdx], NULL, NULL, NULL, -1,  NULL);
          cCurModel.getSynthVideo ( iSynthViewIdx, VIEWPOS_LEFT, pcPicYuvSynthOut );
        }
        break;
        /// EXTRAPOLATION FROM RIGHT
      case 2:            // extrapolation from right
        if ( !bHasRView ) // View to render is BaseView
        {
          std::cout << "Copied    Frame " << iFrame << " of BaseView " << (Double) m_cCameraData.getSynthViewNumbers()[iSynthViewIdx] / VIEW_NUM_PREC  << std::endl;
          apcPicYuvBaseVideo[iRightBaseViewIdx]->copyToPic( pcPicYuvSynthOut ); // Copy Original
        }
        else  // Render
        {
          std::cout << "Rendering Frame " << iFrame << " of View " << (Double) m_cCameraData.getSynthViewNumbers()[iSynthViewIdx] / VIEW_NUM_PREC  << std::endl;
          cCurModel.setSingleModel( iSynthViewIdx, NULL , NULL, m_cCameraData.getSynthViewShiftLUTI()[iRightBaseViewIdx ][iSynthViewIdx], NULL, -1, NULL);
          cCurModel.getSynthVideo ( iSynthViewIdx, VIEWPOS_RIGHT, pcPicYuvSynthOut );
        }
        break;
      }

      // Write Output
#if PIC_CROPPING
      m_apcTVideoIOYuvSynthOutput[m_bSweep ? 0 : iSynthViewIdx]->write( pcPicYuvSynthOut, 0, 0, 0, 0 );
#else
      m_apcTVideoIOYuvSynthOutput[m_bSweep ? 0 : iSynthViewIdx]->write( pcPicYuvSynthOut, aiPad );
#endif
    }
    iFrame++;
    iNumOfRenderedFrames++;
  }

  // Delete Buffers
  for ( UInt uiBaseView = 0; uiBaseView < m_iNumberOfInputViews; uiBaseView++ )
  {
    apcPicYuvBaseVideo[uiBaseView]->destroy();
    delete apcPicYuvBaseVideo[uiBaseView];

    apcPicYuvBaseDepth[uiBaseView]->destroy();
    delete apcPicYuvBaseDepth[uiBaseView];
  }
  pcPicYuvSynthOut->destroy();
  delete pcPicYuvSynthOut;

  xDestroyLib();

}

Void TAppRendererTop::renderUsedPelsMap( )
{
  xCreateLib();
  xInitLib();

  // Create Buffers Input Views;
  std::vector<TComPicYuv*> apcPicYuvBaseVideo;
  std::vector<TComPicYuv*> apcPicYuvBaseDepth;

  // TemporalImprovement Filter
  std::vector<TComPicYuv*> apcPicYuvLastBaseVideo;
  std::vector<TComPicYuv*> apcPicYuvLastBaseDepth;

  Int aiPad[2] = { 0, 0 };

  for ( UInt uiBaseView = 0; uiBaseView < m_iNumberOfInputViews; uiBaseView++ )
  {
    TComPicYuv* pcNewVideoPic = new TComPicYuv;
    TComPicYuv* pcNewDepthPic = new TComPicYuv;

    pcNewVideoPic->create( m_iSourceWidth, m_iSourceHeight, 1, 1, 1 );
    apcPicYuvBaseVideo.push_back(pcNewVideoPic);

    pcNewDepthPic->create( m_iSourceWidth, m_iSourceHeight, 1, 1, 1 );
    apcPicYuvBaseDepth.push_back(pcNewDepthPic);

    //Temporal improvement Filter
    if ( m_bTempDepthFilter )
    {
      pcNewVideoPic = new TComPicYuv;
      pcNewDepthPic = new TComPicYuv;

      pcNewVideoPic->create( m_iSourceWidth, m_iSourceHeight, 1, 1, 1 );
      apcPicYuvLastBaseVideo.push_back(pcNewVideoPic);

      pcNewDepthPic->create( m_iSourceWidth, m_iSourceHeight, 1, 1, 1 );
      apcPicYuvLastBaseDepth.push_back(pcNewDepthPic);
    }
  }

  // Create Buffer for synthesized View
  TComPicYuv* pcPicYuvSynthOut = new TComPicYuv;
  pcPicYuvSynthOut->create( m_iSourceWidth, m_iSourceHeight, 1, 1, 1 );

  Bool bAnyEOS = false;

  Int iNumOfRenderedFrames = 0;
  Int iFrame = 0;

  while ( ( ( iNumOfRenderedFrames < m_iFramesToBeRendered ) || ( m_iFramesToBeRendered == 0 ) ) && !bAnyEOS )
  {
    // set shift LUT

    // read in depth and video
    for(Int iBaseViewIdx=0; iBaseViewIdx < m_iNumberOfInputViews; iBaseViewIdx++ )
    {
      m_apcTVideoIOYuvVideoInput[iBaseViewIdx]->read( apcPicYuvBaseVideo[iBaseViewIdx], aiPad  ) ;
      apcPicYuvBaseVideo[iBaseViewIdx]->extendPicBorder();
      bAnyEOS |= m_apcTVideoIOYuvVideoInput[iBaseViewIdx]->isEof();

      m_apcTVideoIOYuvDepthInput[iBaseViewIdx]->read( apcPicYuvBaseDepth[iBaseViewIdx], aiPad  ) ;
      apcPicYuvBaseDepth[iBaseViewIdx]->extendPicBorder();
      bAnyEOS |= m_apcTVideoIOYuvDepthInput[iBaseViewIdx]->isEof();

      if ( m_bTempDepthFilter && (iFrame >= m_iFrameSkip) )
      {
        m_pcRenTop->temporalFilterVSRS( apcPicYuvBaseVideo[iBaseViewIdx], apcPicYuvBaseDepth[iBaseViewIdx], apcPicYuvLastBaseVideo[iBaseViewIdx], apcPicYuvLastBaseDepth[iBaseViewIdx], ( iFrame == m_iFrameSkip) );
      }
    }

    if ( iFrame < m_iFrameSkip ) // Skip Frames
    {
      std::cout << "Skipping Frame " << iFrame << std::endl;

      iFrame++;
      continue;
    }

    m_cCameraData.update( (UInt)iFrame );

    for(Int iViewIdx=1; iViewIdx < m_iNumberOfInputViews; iViewIdx++ )
    {
      std::cout << "Rendering UsedPelsMap for Frame " << iFrame << " of View " << (Double) m_cCameraData.getBaseViewNumbers()[iViewIdx] << std::endl;

      Int iViewSIdx      = m_cCameraData.getBaseId2SortedId()[iViewIdx];
      Int iFirstViewSIdx = m_cCameraData.getBaseId2SortedId()[0];

      AOT( iViewSIdx == iFirstViewSIdx );

      Bool bFirstIsLeft = (iFirstViewSIdx < iViewSIdx);

      m_pcRenTop->setShiftLUTs(
        m_cCameraData.getBaseViewShiftLUTD()[0][iViewIdx],
        m_cCameraData.getBaseViewShiftLUTI()[0][iViewIdx],
        m_cCameraData.getBaseViewShiftLUTI()[0][iViewIdx],
        m_cCameraData.getBaseViewShiftLUTD()[0][iViewIdx],
        m_cCameraData.getBaseViewShiftLUTI()[0][iViewIdx],
        m_cCameraData.getBaseViewShiftLUTI()[0][iViewIdx],
        -1
        );

      m_pcRenTop->getUsedSamplesMap( apcPicYuvBaseDepth[0], pcPicYuvSynthOut, bFirstIsLeft );

      // Write Output
#if PIC_CROPPING
      m_apcTVideoIOYuvSynthOutput[iViewIdx-1]->write( pcPicYuvSynthOut, 0, 0, 0 );
#else
      m_apcTVideoIOYuvSynthOutput[iViewIdx-1]->write( pcPicYuvSynthOut, aiPad );
#endif

    }
    iFrame++;
    iNumOfRenderedFrames++;
  }

  // Delete Buffers
  for ( UInt uiBaseView = 0; uiBaseView < m_iNumberOfInputViews; uiBaseView++ )
  {
    apcPicYuvBaseVideo[uiBaseView]->destroy();
    delete apcPicYuvBaseVideo[uiBaseView];

    apcPicYuvBaseDepth[uiBaseView]->destroy();
    delete apcPicYuvBaseDepth[uiBaseView];

    // Temporal Filter
    if ( m_bTempDepthFilter )
    {
      apcPicYuvLastBaseVideo[uiBaseView]->destroy();
      delete apcPicYuvLastBaseVideo[uiBaseView];

      apcPicYuvLastBaseDepth[uiBaseView]->destroy();
      delete apcPicYuvLastBaseDepth[uiBaseView];
    }
  }
  pcPicYuvSynthOut->destroy();
  delete pcPicYuvSynthOut;

  xDestroyLib();

}
