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


#include "TRenImage.h"
#include "TRenFilter.h"
#include "TRenSingleModel.h"

////////////// TRENSINGLE MODEL ///////////////
TRenSingleModel::TRenSingleModel()
:  m_iDistShift ( g_uiBitIncrement << 1 )
{
  m_iWidth  = -1;
  m_iHeight = -1;
  m_iStride = -1;
  m_iMode   = -1;
  m_iPad    = 12;
  m_iGapTolerance = -1;
  m_bUseOrgRef = false;

  m_pcPicYuvRef          = NULL;
  m_aapiRefVideoPel[0]   = NULL;
  m_aapiRefVideoPel[1]   = NULL;
  m_aapiRefVideoPel[2]   = NULL;


  m_aiRefVideoStrides[0] = -1;
  m_aiRefVideoStrides[1] = -1;
  m_aiRefVideoStrides[2] = -1;

  for (UInt uiViewNum = 0 ; uiViewNum < 2; uiViewNum++)
  {
    // LUT
    m_appiShiftLut[uiViewNum] = NULL;
    m_ppiCurLUT               = NULL;
    m_piInvZLUTLeft           = NULL;
    m_piInvZLUTRight          = NULL;

    // Cur Data
    m_apiBaseDepthPel      [uiViewNum] = NULL;
    m_aiBaseDepthStrides   [uiViewNum] = -1;

    // State Data
    m_apbOccluded          [uiViewNum] = NULL;
    m_apiFilled            [uiViewNum] = NULL;

    // Cur Data
    m_aapiBaseVideoPel     [uiViewNum] = NULL;
    m_aaiBaseVideoStrides  [uiViewNum] = NULL;
  };

  m_piError                            = NULL;

  for (UInt uiViewNum = 0 ; uiViewNum < 3; uiViewNum++)
  {
    m_apiSynthDepthPel[uiViewNum] = NULL;
    for (UInt uiPlaneNum = 0; uiPlaneNum < 3; uiPlaneNum++)
    {
      // Rendered Data
      m_aapiSynthVideoPel[uiViewNum][uiPlaneNum] = NULL;
    }
  }
}

TRenSingleModel::~TRenSingleModel()
{
  if ( m_apbOccluded[0] ) delete[] m_apbOccluded[0];
  if ( m_apbOccluded[1] ) delete[] m_apbOccluded[1];

  if ( m_apiFilled  [0] ) delete[] m_apiFilled  [0];
  if ( m_apiFilled  [1] ) delete[] m_apiFilled  [1];

  if ( m_piError        ) delete[] m_piError      ;


  for (UInt uiViewNum = 0 ; uiViewNum < 3; uiViewNum++)
  {
    for (UInt uiPlaneNum = 0; uiPlaneNum < 3; uiPlaneNum++)
    {
      if ( m_aapiSynthVideoPel[uiViewNum] && m_aapiSynthVideoPel[uiViewNum][uiPlaneNum] ) delete[] m_aapiSynthVideoPel[uiViewNum][uiPlaneNum];
    }
    if ( m_apiSynthDepthPel[uiViewNum] ) delete[] m_apiSynthDepthPel[uiViewNum];
  }

  delete[] (m_aapiRefVideoPel[0] - m_iPad * m_aiRefVideoStrides[0] - m_iPad );
  delete[] (m_aapiRefVideoPel[1] - m_iPad * m_aiRefVideoStrides[1] - m_iPad );
  delete[] (m_aapiRefVideoPel[2] - m_iPad * m_aiRefVideoStrides[2] - m_iPad );

  if ( m_piInvZLUTLeft  ) delete[] m_piInvZLUTLeft;
  if ( m_piInvZLUTRight ) delete[] m_piInvZLUTRight;
}

Void
TRenSingleModel::create( Int iMode, Int iWidth, Int iHeight, Int iShiftPrec, Int*** aaaiSubPelShiftTable, Int iHoleMargin, Bool bUseOrgRef, Int iBlendMode )
{
  m_iBlendMode = iBlendMode;
  m_iMode = iMode;
  m_iWidth  = iWidth;
  m_iHeight = iHeight;
  m_iStride = iWidth;

  m_iSampledWidth  = m_iWidth  << iShiftPrec;
  m_iSampledStride = m_iStride << iShiftPrec;

  m_iShiftPrec     = iShiftPrec;
  m_aaiSubPelShiftL = aaaiSubPelShiftTable[0];
  m_aaiSubPelShiftR = aaaiSubPelShiftTable[1];

  if (m_iMode == 2)
  {
    m_piInvZLUTLeft  = new Int[257];
    m_piInvZLUTRight = new Int[257];
  }

  m_iGapTolerance  = ( 2 << iShiftPrec );
  m_iHoleMargin    =  iHoleMargin;

  m_bUseOrgRef = bUseOrgRef;

  m_aiRefVideoStrides[0] = m_iStride + (m_iPad << 1);
  m_aiRefVideoStrides[1] = m_iStride + (m_iPad << 1);
  m_aiRefVideoStrides[2] = m_iStride + (m_iPad << 1);

  m_aapiRefVideoPel  [0] = new Pel[ m_aiRefVideoStrides[0] * (m_iHeight + (m_iPad << 1))];
  m_aapiRefVideoPel  [1] = new Pel[ m_aiRefVideoStrides[1] * (m_iHeight + (m_iPad << 1))];
  m_aapiRefVideoPel  [2] = new Pel[ m_aiRefVideoStrides[2] * (m_iHeight + (m_iPad << 1))];

  m_aapiRefVideoPel  [0] += m_aiRefVideoStrides[0] * m_iPad + m_iPad;
  m_aapiRefVideoPel  [1] += m_aiRefVideoStrides[1] * m_iPad + m_iPad;
  m_aapiRefVideoPel  [2] += m_aiRefVideoStrides[2] * m_iPad + m_iPad;

  m_piError               = new Int [m_iStride*m_iHeight];

  // Create Buffers
  if ( (m_iMode == 0) || (m_iMode == 2 ) )
  {
    m_apbOccluded        [0]  = new Bool[m_iStride*m_iHeight];
    m_apiFilled          [0]  = new Pel [m_iStride*m_iHeight];

    for (UInt uiPlaneNum = 0; uiPlaneNum < 3; uiPlaneNum++)
    {
      m_aapiSynthVideoPel[0][uiPlaneNum] = new Pel[m_iStride*m_iHeight];
    }
  }

  if ( (m_iMode == 1) || (m_iMode == 2 ) )
  {
    m_apbOccluded        [1]  = new Bool[m_iStride*m_iHeight];
    m_apiFilled          [1]  = new Pel [m_iStride*m_iHeight];

    for (UInt uiPlaneNum = 0; uiPlaneNum < 3; uiPlaneNum++)
    {
      m_aapiSynthVideoPel[1][uiPlaneNum] = new Pel[m_iStride*m_iHeight];
    }
  }

  if ( m_iMode == 2 )
  {
    m_apiSynthDepthPel[0] = new Pel[m_iStride*m_iHeight];
    m_apiSynthDepthPel[1] = new Pel[m_iStride*m_iHeight];
    m_apiSynthDepthPel[2] = new Pel[m_iStride*m_iHeight];

    for (UInt uiPlaneNum = 0; uiPlaneNum < 3; uiPlaneNum++)
    {
      m_aapiSynthVideoPel[2][uiPlaneNum] = new Pel[m_iStride*m_iHeight];
    }
  }
}

Void
TRenSingleModel::setLRView( Int iViewPos, Pel** apiCurVideoPel, Int* aiCurVideoStride, Pel* piCurDepthPel, Int iCurDepthStride )
{
  AOF(( iViewPos == 0) || (iViewPos == 1) );
  m_aapiBaseVideoPel      [iViewPos] = apiCurVideoPel;
  m_aaiBaseVideoStrides   [iViewPos] = aiCurVideoStride;
  m_apiBaseDepthPel       [iViewPos] = piCurDepthPel;
  m_aiBaseDepthStrides    [iViewPos] = iCurDepthStride;
}

Void
TRenSingleModel::setup( TComPicYuv* pcOrgVideo, Int** ppiShiftLutLeft, Int** ppiBaseShiftLutLeft, Int** ppiShiftLutRight,  Int** ppiBaseShiftLutRight,  Int iDistToLeft, Bool bKeepReference )
{
  AOT( !m_bUseOrgRef && pcOrgVideo );
  AOT( (ppiShiftLutLeft  == NULL) && (m_iMode == 0 || m_iMode == 2) );
  AOT( (ppiShiftLutRight == NULL) && (m_iMode == 1 || m_iMode == 2) );
  AOT( pcOrgVideo != NULL && bKeepReference );

  m_appiShiftLut[0] = ppiShiftLutLeft;
  m_appiShiftLut[1] = ppiShiftLutRight;

  // Copy Reference
  m_pcPicYuvRef = pcOrgVideo;

  if ( pcOrgVideo )
  {
    TRenFilter::copy(             pcOrgVideo->getLumaAddr(), pcOrgVideo->getStride() , m_iWidth,      m_iHeight,      m_aapiRefVideoPel[0], m_aiRefVideoStrides[0]);
    TRenFilter::sampleCUpHorUp(0, pcOrgVideo->getCbAddr()  , pcOrgVideo->getCStride(), m_iWidth >> 1, m_iHeight >> 1, m_aapiRefVideoPel[1], m_aiRefVideoStrides[1]);
    TRenFilter::sampleCUpHorUp(0, pcOrgVideo->getCrAddr()  , pcOrgVideo->getCStride(), m_iWidth >> 1, m_iHeight >> 1, m_aapiRefVideoPel[2], m_aiRefVideoStrides[2]);
  }

  // Initial Rendering
  xSetInts( m_piError                       , m_iStride, m_iWidth, m_iHeight, 0 );

  switch ( m_iMode )
  {
  case 0:
    xInitView( VIEWPOS_LEFT );
    xRenderL( 0, 0, m_iWidth, m_iHeight, m_aiBaseDepthStrides[0], m_apiBaseDepthPel[0], true );
    break;
  case 1:
    xInitView( VIEWPOS_RIGHT );
    xRenderR( 0, 0, m_iWidth, m_iHeight, m_aiBaseDepthStrides[1], m_apiBaseDepthPel[1], true );
    break;
  case 2:
    TRenFilter::setupZLUT( true, 30, iDistToLeft, ppiBaseShiftLutLeft, ppiBaseShiftLutRight, m_iBlendZThres, m_iBlendDistWeight, m_piInvZLUTLeft, m_piInvZLUTRight );
    xInitView( VIEWPOS_LEFT   );
    xInitView( VIEWPOS_RIGHT  );
    xInitView( VIEWPOS_MERGED );
    xRenderL( 0, 0, m_iWidth, m_iHeight, m_aiBaseDepthStrides[0], m_apiBaseDepthPel[0], true );
    xRenderR( 0, 0, m_iWidth, m_iHeight, m_aiBaseDepthStrides[1], m_apiBaseDepthPel[1], true );
    break;
  default:
    AOT(true);
  }

  // Get Rendered View as Reference
  if ( !pcOrgVideo && !bKeepReference )
  {
    xSetInts        ( m_piError                       , m_iStride, m_iWidth, m_iHeight, 0 );
      TRenFilter::copy( m_aapiSynthVideoPel[m_iMode ][0], m_iStride, m_iWidth, m_iHeight , m_aapiRefVideoPel[0], m_aiRefVideoStrides[0]);
      TRenFilter::copy( m_aapiSynthVideoPel[m_iMode ][1], m_iStride, m_iWidth, m_iHeight , m_aapiRefVideoPel[1], m_aiRefVideoStrides[1]);
      TRenFilter::copy( m_aapiSynthVideoPel[m_iMode ][2], m_iStride, m_iWidth, m_iHeight , m_aapiRefVideoPel[2], m_aiRefVideoStrides[2]);
  }
}

RMDist
TRenSingleModel::getDistDepth( Int iViewPos, Int iStartPosX, Int iStartPosY, Int iWidth, Int iHeight, Int iStride, Pel* piNewData )
{
  RMDist iSSE = 0;
  switch (iViewPos )
  {
  case 0:
    iSSE = xRenderL( iStartPosX,   iStartPosY,   iWidth,   iHeight,   iStride, piNewData, false );
    break;
  case 1:
    iSSE = xRenderR( iStartPosX,   iStartPosY,   iWidth,   iHeight,   iStride, piNewData, false );
    break;
  default:
    assert(0);
  }
  return iSSE;
}

Void
TRenSingleModel::setDepth( Int iViewPos, Int iStartPosX, Int iStartPosY, Int iWidth, Int iHeight, Int iStride, Pel* piNewData )
{
  switch (iViewPos )
  {
    case 0:
      xRenderL( iStartPosX,   iStartPosY,   iWidth,   iHeight,   iStride, piNewData, true );
      break;
    case 1:
      xRenderR( iStartPosX,   iStartPosY,   iWidth,   iHeight,   iStride, piNewData, true );
      break;
    default:
      assert(0);
  }
}


Void
TRenSingleModel::getSynthView( Int iViewPos, Pel**& rppiRenVideoPel, Pel*& rpiRenDepthPel, Int& riStride )
{
  rppiRenVideoPel = m_aapiSynthVideoPel[iViewPos];
  rpiRenDepthPel  = m_apiSynthDepthPel [iViewPos];
  riStride = m_iStride;
}


Void
TRenSingleModel::getRefView( TComPicYuv*& rpcPicYuvRefView, Pel**& rppiRefVideoPel, Int*& raiStrides )
{
  rpcPicYuvRefView = m_pcPicYuvRef;
  rppiRefVideoPel  = m_aapiRefVideoPel;
  raiStrides       = m_aiRefVideoStrides;
}


RMDist
TRenSingleModel::getDistVideo( Int iViewPos, Int iPlane, Int iStartPosX, Int iStartPosY, Int iWidth, Int iHeight, Int iStride, Pel* piNewData )
{
  AOF(false);
  return 0;
}

Void
TRenSingleModel::setVideo( Int iViewPos, Int iPlane, Int iStartPosX, Int iStartPosY, Int iWidth, Int iHeight, Int iStride, Pel* piNewData )
{
  AOF(false);
}



__inline Void
TRenSingleModel::xSetViewRow( Int iPosY )
{
  m_aapiBaseVideoPelRow     [m_iCurViewPos][0] = m_aapiBaseVideoPel   [m_iCurViewPos]  [0] + m_aaiBaseVideoStrides [ m_iCurViewPos ][0] * iPosY;
  m_aapiBaseVideoPelRow     [m_iCurViewPos][1] = m_aapiBaseVideoPel   [m_iCurViewPos]  [1] + m_aaiBaseVideoStrides [ m_iCurViewPos ][1] * iPosY;
  m_aapiBaseVideoPelRow     [m_iCurViewPos][2] = m_aapiBaseVideoPel   [m_iCurViewPos]  [2] + m_aaiBaseVideoStrides [ m_iCurViewPos ][2] * iPosY;

  m_apiBaseDepthPelRow      [m_iCurViewPos]    = m_apiBaseDepthPel    [m_iCurViewPos]      + m_aiBaseDepthStrides  [ m_iCurViewPos]     * iPosY;
  m_apbOccludedRow          [m_iCurViewPos]    = m_apbOccluded        [m_iCurViewPos]      + m_iStride                                  * iPosY;
  m_apiFilledRow            [m_iCurViewPos]    = m_apiFilled          [m_iCurViewPos]      + m_iStride                                  * iPosY;
  m_apiErrorRow                                = m_piError                                 + m_iStride                                  * iPosY;

  m_aapiSynthVideoPelRow    [m_iCurViewPos][0] = m_aapiSynthVideoPel  [m_iCurViewPos]  [0] + m_iStride                                  * iPosY;
  m_aapiSynthVideoPelRow    [m_iCurViewPos][1] = m_aapiSynthVideoPel  [m_iCurViewPos]  [1] + m_iStride                                  * iPosY;
  m_aapiSynthVideoPelRow    [m_iCurViewPos][2] = m_aapiSynthVideoPel  [m_iCurViewPos]  [2] + m_iStride                                  * iPosY;

  m_aapiRefVideoPelRow                     [0] = m_aapiRefVideoPel                     [0] + m_aiRefVideoStrides                    [0] * iPosY;
  m_aapiRefVideoPelRow                     [1] = m_aapiRefVideoPel                     [1] + m_aiRefVideoStrides                    [1] * iPosY;
  m_aapiRefVideoPelRow                     [2] = m_aapiRefVideoPel                     [2] + m_aiRefVideoStrides                    [2] * iPosY;

  if (m_iMode == 2)
  {
    m_apiSynthDepthPelRow [m_iCurViewPos ]     = m_apiSynthDepthPel   [m_iCurViewPos]      + m_iStride                                  * iPosY;
    m_aapiSynthVideoPelRow[m_iOtherViewPos][0] = m_aapiSynthVideoPel  [m_iOtherViewPos][0] + m_iStride                                  * iPosY;
    m_aapiSynthVideoPelRow[m_iOtherViewPos][1] = m_aapiSynthVideoPel  [m_iOtherViewPos][1] + m_iStride                                  * iPosY;
    m_aapiSynthVideoPelRow[m_iOtherViewPos][2] = m_aapiSynthVideoPel  [m_iOtherViewPos][2] + m_iStride                                  * iPosY;

    m_apiFilledRow        [m_iOtherViewPos]    = m_apiFilled          [m_iOtherViewPos]    + m_iStride                                  * iPosY;
    m_apiSynthDepthPelRow [m_iOtherViewPos]    = m_apiSynthDepthPel   [m_iOtherViewPos]    + m_iStride                                  * iPosY;

    m_aapiSynthVideoPelRow[2              ][0] = m_aapiSynthVideoPel[2]                [0] + m_iStride                                  * iPosY;
    m_aapiSynthVideoPelRow[2              ][1] = m_aapiSynthVideoPel[2]                [1] + m_iStride                                  * iPosY;
    m_aapiSynthVideoPelRow[2              ][2] = m_aapiSynthVideoPel[2]                [2] + m_iStride                                  * iPosY;
  }
}

__inline Void
TRenSingleModel::xIncViewRow( )
{
  m_aapiBaseVideoPelRow     [m_iCurViewPos][0] += m_aaiBaseVideoStrides [ m_iCurViewPos ][0];
  m_aapiBaseVideoPelRow     [m_iCurViewPos][1] += m_aaiBaseVideoStrides [ m_iCurViewPos ][1];
  m_aapiBaseVideoPelRow     [m_iCurViewPos][2] += m_aaiBaseVideoStrides [ m_iCurViewPos ][2];

  m_apiBaseDepthPelRow      [m_iCurViewPos]    += m_aiBaseDepthStrides  [ m_iCurViewPos]    ;
  m_apbOccludedRow          [m_iCurViewPos]    += m_iStride                                 ;
  m_apiFilledRow            [m_iCurViewPos]    += m_iStride                                 ;
  m_apiErrorRow                                += m_iStride                                 ;

  m_aapiSynthVideoPelRow    [m_iCurViewPos][0] += m_iStride                                 ;
  m_aapiSynthVideoPelRow    [m_iCurViewPos][1] += m_iStride                                 ;
  m_aapiSynthVideoPelRow    [m_iCurViewPos][2] += m_iStride                                 ;

  m_aapiRefVideoPelRow                     [0] += m_aiRefVideoStrides                    [0];
  m_aapiRefVideoPelRow                     [1] += m_aiRefVideoStrides                    [1];
  m_aapiRefVideoPelRow                     [2] += m_aiRefVideoStrides                    [2];

  if (m_iMode == 2)
  {
    m_apiSynthDepthPelRow [m_iCurViewPos ]     += m_iStride                                 ;    // This is correct!

    m_aapiSynthVideoPelRow[m_iOtherViewPos][0] += m_iStride                                 ;
    m_aapiSynthVideoPelRow[m_iOtherViewPos][1] += m_iStride                                 ;
    m_aapiSynthVideoPelRow[m_iOtherViewPos][2] += m_iStride                                 ;

    m_apiFilledRow        [m_iOtherViewPos]    += m_iStride                                 ;
    m_apiSynthDepthPelRow [m_iOtherViewPos]    += m_iStride                                 ;

    m_aapiSynthVideoPelRow[2              ][0] += m_iStride                                 ;
    m_aapiSynthVideoPelRow[2              ][1] += m_iStride                                 ;
    m_aapiSynthVideoPelRow[2              ][2] += m_iStride                                 ;
  }
}

__inline RMDist
TRenSingleModel::xRenderL( Int iStartPosX, Int iStartPosY, Int iWidth, Int iHeight, Int iStride, Pel* piNewData, Bool bAdd )
{
  m_bSet             = bAdd;
  m_iCurViewPos      = 0;
  m_iOtherViewPos    = 1;

  m_piNewDepthData   = piNewData;
  m_iNewDataWidth    = iWidth;
  m_iStartChangePosX = iStartPosX;

  if ((iWidth == 0) || (iHeight == 0))
    return 0;

  //TODO: Specialize to left and right; setData and getDist

  // Get Data
  m_ppiCurLUT      = m_appiShiftLut   [m_iCurViewPos];

  xSetViewRow      ( iStartPosY);

  // Init Start
  RMDist iError = 0;
  Int   iStartChangePos;

  iStartChangePos = m_iStartChangePosX;

  for (Int iPosY = iStartPosY; iPosY < iStartPosY + iHeight; iPosY++ )
  {
    m_bInOcclusion = false;

    Int iLastSPos;
    Int iEndChangePos         = m_iStartChangePosX + iWidth - 1;
    Int iPosXinNewData        = iWidth - 1;
    Int iMinChangedSPos       = m_iSampledWidth;

    if ( iEndChangePos == ( m_iWidth -1 )) // Special processing for rightmost depth sample
    {
      m_iCurDepth           = m_piNewDepthData[iPosXinNewData];
      Int iCurSPos          = xShiftNewData(iEndChangePos, iPosXinNewData);
      m_iLastOccludedSPos   = iCurSPos + 1;
      m_iLastOccludedSPosFP = xRangeLeftL( m_iLastOccludedSPos );
      xExtrapolateMarginL  ( iCurSPos, iEndChangePos, iError );
      iMinChangedSPos       = Min( iMinChangedSPos, (iEndChangePos << m_iShiftPrec) - m_ppiCurLUT[0][ RemoveBitIncrement( Max(m_apiBaseDepthPelRow[m_iCurViewPos][iEndChangePos], m_piNewDepthData[iPosXinNewData] )) ]);
      iLastSPos             = iCurSPos;
      m_iLastDepth          = m_iCurDepth;
      iPosXinNewData--;
      iEndChangePos--;
    }
    else
    {
      iLastSPos    = xShift(iEndChangePos+1);
      m_iLastDepth = m_apiBaseDepthPelRow[m_iCurViewPos][iEndChangePos+1];
      xInitRenderPartL( iEndChangePos, iLastSPos );
    }

    //// RENDER NEW DATA
    Int iCurPosX;
    for ( iCurPosX = iEndChangePos; iCurPosX >= iStartChangePos; iCurPosX-- )
    {
      // Get minimal changed sample position
      iMinChangedSPos = Min( iMinChangedSPos, (iCurPosX << m_iShiftPrec) - m_ppiCurLUT[0][ RemoveBitIncrement( Max(m_apiBaseDepthPelRow[m_iCurViewPos][iCurPosX], m_piNewDepthData[iPosXinNewData] )) ]);
      Int iCurSPos    = xShiftNewData(iCurPosX,iPosXinNewData);
      m_iCurDepth     = m_piNewDepthData[iPosXinNewData];
      xRenderRangeL(iCurSPos, iLastSPos, iCurPosX, iError );
      iLastSPos       = iCurSPos;
      m_iLastDepth    = m_iCurDepth;
      iPosXinNewData--;
    }

    //// RE-RENDER DATA LEFT TO NEW DATA
    while ( iCurPosX >= 0 )
    {
      Int iCurSPos = xShift(iCurPosX);
      m_iCurDepth  = m_apiBaseDepthPelRow[m_iCurViewPos][iCurPosX];
      xRenderRangeL( iCurSPos, iLastSPos, iCurPosX, iError );

      if ( iCurSPos < iMinChangedSPos )
      {
          break;
        }

      iCurPosX--;
      iLastSPos    = iCurSPos;
      m_iLastDepth = m_iCurDepth;
    }

    xIncViewRow();
    m_piNewDepthData += iStride;
  }
  return iError;
}


__inline RMDist
TRenSingleModel::xRenderR( Int iStartPosX, Int iStartPosY, Int iWidth, Int iHeight, Int iStride, Pel* piNewData, Bool bAdd )
{
  m_bSet             = bAdd;
  m_iCurViewPos      = 1;
  m_iOtherViewPos    = 0;

  m_piNewDepthData   = piNewData;
  m_iNewDataWidth    = iWidth;
  m_iStartChangePosX = iStartPosX;

  if ((iWidth == 0) || (iHeight == 0))
    return 0;

  // Get Data
  m_ppiCurLUT      = m_appiShiftLut   [m_iCurViewPos];

  xSetViewRow      ( iStartPosY);

  // Init Start
  RMDist iError = 0;
  Int   iEndChangePos;

  iEndChangePos = m_iStartChangePosX + iWidth - 1;


  for (Int iPosY = iStartPosY; iPosY < iStartPosY + iHeight; iPosY++ )
  {
    m_bInOcclusion = false;

    Int iLastSPos;
    Int iStartChangePos       = m_iStartChangePosX;
    Int iPosXinNewData        = 0;
    Int iMaxChangedSPos = -1;

    if ( iStartChangePos == 0 ) // Special processing for leftmost depth sample
    {
      m_iCurDepth           = m_piNewDepthData[iPosXinNewData];
      Int iCurSPos          = xShiftNewData(iStartChangePos, iPosXinNewData);
      m_iLastOccludedSPos   = iCurSPos - 1;
      m_iLastOccludedSPosFP = xRangeRightR( m_iLastOccludedSPos );
      xExtrapolateMarginR     ( iCurSPos, iStartChangePos, iError );
      iMaxChangedSPos       = Max( iMaxChangedSPos, (iStartChangePos << m_iShiftPrec) - m_ppiCurLUT[0][ RemoveBitIncrement( Max(m_apiBaseDepthPelRow[m_iCurViewPos][iStartChangePos], m_piNewDepthData[iPosXinNewData] )) ]);
      iLastSPos             = iCurSPos;
      m_iLastDepth          = m_iCurDepth;
      iPosXinNewData++;
      iStartChangePos++;
    }
    else
    {
      iLastSPos   = xShift(iStartChangePos-1);
      m_iLastDepth = m_apiBaseDepthPelRow[m_iCurViewPos][iStartChangePos-1];
      xInitRenderPartR( iStartChangePos, iLastSPos );
    }

    //// RENDER NEW DATA
    Int iCurPosX;
    for ( iCurPosX = iStartChangePos; iCurPosX <= iEndChangePos; iCurPosX++ )
    {
      // Get minimal changed sample position
      iMaxChangedSPos = Max( iMaxChangedSPos, (iCurPosX << m_iShiftPrec) - m_ppiCurLUT[0][ RemoveBitIncrement( Max(m_apiBaseDepthPelRow[m_iCurViewPos][iCurPosX], m_piNewDepthData[iPosXinNewData] )) ]);
      Int iCurSPos    = xShiftNewData(iCurPosX,iPosXinNewData);
      m_iCurDepth     = m_piNewDepthData[iPosXinNewData];
      xRenderRangeR(iCurSPos, iLastSPos, iCurPosX, iError );
      iLastSPos      = iCurSPos;
      m_iLastDepth    = m_iCurDepth;
      iPosXinNewData++;
    }

    //// RE-RENDER DATA LEFT TO NEW DATA
    while ( iCurPosX < m_iWidth )
    {
      Int iCurSPos = xShift(iCurPosX);
      m_iCurDepth  = m_apiBaseDepthPelRow[m_iCurViewPos][iCurPosX];
      xRenderRangeR( iCurSPos, iLastSPos, iCurPosX, iError );

      if ( iCurSPos > iMaxChangedSPos )
      {
          break;
        }
      iCurPosX++;
      iLastSPos    = iCurSPos;
      m_iLastDepth = m_iCurDepth;
    }
    xIncViewRow();
    m_piNewDepthData += iStride;
  }
  return iError;
}


__inline Void
TRenSingleModel::xInitRenderPartL(  Int iEndChangePos, Int iLastSPos )
{
  // GET MINIMAL OCCLUDED SAMPLE POSITION
  Int iCurPosX           = iEndChangePos;

  if ( ( iCurPosX + 1 < m_iWidth ) && (m_apbOccludedRow[m_iCurViewPos][ iCurPosX + 1] ) )
  {
    iCurPosX++;
    while ( (iCurPosX + 1 < m_iWidth) &&  (m_apbOccludedRow[m_iCurViewPos][ iCurPosX + 1] )  )
      iCurPosX++;

    if ( iCurPosX + 1 < m_iWidth )
    {
      iCurPosX++;
      m_iLastOccludedSPos = xShift(iCurPosX);
    }
    else
    {
      m_iLastOccludedSPos = xShift(iCurPosX) + 1;
    }

    m_iLastOccludedSPosFP = xRoundL( m_iLastOccludedSPos );
  }
  else
  {
    m_iLastOccludedSPos   = iLastSPos+1;
    m_iLastOccludedSPosFP = xRangeLeftL( m_iLastOccludedSPos );
  }

  m_bInOcclusion = iLastSPos >= m_iLastOccludedSPos;
};

__inline Void
TRenSingleModel::xInitRenderPartR(  Int iStartChangePos, Int iLastSPos )
{
  // GET MINIMAL OCCLUDED SAMPLE POSITION
  Int iCurPosX           = iStartChangePos;

  if ( ( iCurPosX - 1 > -1 ) && (m_apbOccludedRow[m_iCurViewPos][ iCurPosX - 1] ) )
  {
    iCurPosX--;
    while ( (iCurPosX - 1 > -1 ) &&  (m_apbOccludedRow[m_iCurViewPos][ iCurPosX - 1] )  )
      iCurPosX--;

    if ( iCurPosX - 1 > -1 )
    {
      iCurPosX--;
      m_iLastOccludedSPos = xShift(iCurPosX);
    }
    else
    {
      m_iLastOccludedSPos = xShift(iCurPosX) - 1;
    }
    m_iLastOccludedSPosFP = xRoundR( m_iLastOccludedSPos );
  }
  else
  {
    m_iLastOccludedSPos   = iLastSPos-1;
    m_iLastOccludedSPosFP = xRangeRightR( m_iLastOccludedSPos );
  }

  m_bInOcclusion = iLastSPos <= m_iLastOccludedSPos;
};


__inline Void
TRenSingleModel::xRenderShiftedRangeL(Int iCurSPos, Int iLastSPos, Int iCurPos, RMDist& riError )
{
  assert( iCurSPos <= iLastSPos );
  //assert( iRightSPos < m_iWidth );

  Int iDeltaSPos = iLastSPos - iCurSPos;
  if ( iDeltaSPos > m_iGapTolerance )
  {
    xFillHoleL( iCurSPos, iLastSPos, iCurPos, riError );
  }
  else
  {
    if (iLastSPos < 0 )
      return;

    AOT( iDeltaSPos    > m_iGapTolerance );

    m_iThisDepth = m_iCurDepth;
    for (Int iFillSPos = Max(0, xRangeLeftL(iCurSPos) ); iFillSPos <= min(xRangeRightL( iLastSPos ) ,m_iLastOccludedSPosFP-1); iFillSPos++ )
    {
      Int iDeltaCurSPos  = (iFillSPos << m_iShiftPrec) - iCurSPos;

      AOT( iDeltaCurSPos > iDeltaSPos );
      AOT( iDeltaCurSPos < 0 );
      AOT( m_aaiSubPelShiftL[iDeltaSPos][iDeltaCurSPos] == 0xdeaddead);

      Int iSourcePos = (iCurPos  << m_iShiftPrec) +  m_aaiSubPelShiftL[iDeltaSPos][iDeltaCurSPos];   // GT:  = iPosX - iStep + ( iStep * iDeltaCurPos + ( iDeltaPos >> 1) ) / iDeltaPos;
      xSetShiftedPel( iSourcePos, iFillSPos, REN_IS_FILLED, riError );
    }
  };
}

__inline Void
TRenSingleModel::xRenderShiftedRangeR(Int iCurSPos, Int iLastSPos, Int iCurPos, RMDist& riError )
{
  assert( iCurSPos >= iLastSPos );
  //assert( iRightSPos < m_iWidth );

  Int iDeltaSPos = iCurSPos - iLastSPos;
  if ( iDeltaSPos > m_iGapTolerance )
  {
    xFillHoleR( iCurSPos, iLastSPos, iCurPos, riError );
  }
  else
  {
    if (iLastSPos > m_iSampledWidth - 1 )
      return;

    m_iThisDepth = m_iCurDepth;
    AOT( iDeltaSPos    > m_iGapTolerance );
    for (Int iFillSPos = max(m_iLastOccludedSPosFP+1, xRangeLeftR(iLastSPos) ); iFillSPos <= min(xRangeRightR( iCurSPos ) ,m_iWidth -1); iFillSPos++ )
    {
      Int iDeltaCurSPos  = (iFillSPos << m_iShiftPrec) - iLastSPos;

      AOT( iDeltaCurSPos > iDeltaSPos );
      AOT( iDeltaCurSPos < 0 );
      AOT( m_aaiSubPelShiftR[iDeltaSPos][iDeltaCurSPos] == 0xdeaddead);

      Int iSourcePos = (iCurPos  << m_iShiftPrec) +  m_aaiSubPelShiftR[iDeltaSPos][iDeltaCurSPos];   // GT:  = iPosX - iStep + ( iStep * iDeltaCurPos + ( iDeltaPos >> 1) ) / iDeltaPos;

      xSetShiftedPel( iSourcePos, iFillSPos, REN_IS_FILLED, riError );
    }
  };
}



__inline Void
TRenSingleModel::xRenderRangeL(Int iCurSPos, Int iLastSPos, Int iCurPos, RMDist& riError )
{
  if (  !m_bInOcclusion )
  {
    if ( iCurSPos >= iLastSPos )
    {
      m_iLastOccludedSPos = iLastSPos;

      Int iRightSPosFP = xRoundL( iLastSPos );
      if ( ( iRightSPosFP == xRangeRightL(iLastSPos)) && (iRightSPosFP >= 0) )
      {
        m_iThisDepth = m_iLastDepth;
        xSetShiftedPel( (iCurPos+1) << m_iShiftPrec, iRightSPosFP, REN_IS_FILLED, riError );
      }
      m_iLastOccludedSPosFP = iRightSPosFP;

      m_bInOcclusion = true;

      if ( m_bSet )
      {
        m_apbOccludedRow[m_iCurViewPos][ iCurPos ] = true;
      }
    }
    else
    {
      if ( m_bSet )
      {
        m_apbOccludedRow[m_iCurViewPos][ iCurPos ] = false;
      }

      xRenderShiftedRangeL(iCurSPos, iLastSPos, iCurPos, riError );
    }
  }
  else
  {
    if ( iCurSPos < m_iLastOccludedSPos )
    {
      m_bInOcclusion = false;
      if ( m_bSet )
      {
        m_apbOccludedRow[m_iCurViewPos][ iCurPos ] = false;
      }

      xRenderShiftedRangeL(iCurSPos, iLastSPos, iCurPos, riError );
    }
    else
    {
      if ( m_bSet )
      {
        m_apbOccludedRow[m_iCurViewPos][ iCurPos ] = true;
      }
    }
  }
}

__inline Void
TRenSingleModel::xRenderRangeR(Int iCurSPos, Int iLastSPos, Int iCurPos, RMDist& riError )
{
  // Find out if current sample is occluded
  if (  !m_bInOcclusion )
  {
    if ( iCurSPos <= iLastSPos )
    {
      m_iLastOccludedSPos = iLastSPos;

      Int iLeftSPosFP = xRoundR( iLastSPos );
      if ( ( iLeftSPosFP == xRangeLeftR(iLastSPos)) && (iLeftSPosFP <= m_iWidth - 1) )
      {
        m_iThisDepth = m_iLastDepth;
        xSetShiftedPel( (iCurPos-1) << m_iShiftPrec, iLeftSPosFP, REN_IS_FILLED, riError );
      }
      m_iLastOccludedSPosFP = iLeftSPosFP;

      m_bInOcclusion = true;

      if ( m_bSet )
      {
        m_apbOccludedRow[m_iCurViewPos][ iCurPos ] = true;
      }
    }
    else
    {
      if ( m_bSet )
      {
        m_apbOccludedRow[m_iCurViewPos][ iCurPos ] = false;
      }

      xRenderShiftedRangeR(iCurSPos, iLastSPos, iCurPos, riError );
    }
  }
  else
  {
    if ( iCurSPos > m_iLastOccludedSPos )
    {
      m_bInOcclusion = false;
      if ( m_bSet )
      {
        m_apbOccludedRow[m_iCurViewPos][ iCurPos ] = false;
      }

      xRenderShiftedRangeR(iCurSPos, iLastSPos, iCurPos, riError );
    }
    else
    {
      if ( m_bSet )
      {
        m_apbOccludedRow[m_iCurViewPos][ iCurPos ] = true;
      }
    }
  }
}

__inline Void
TRenSingleModel::xFillHoleL( Int iCurSPos, Int iLastSPos, Int iCurPos, RMDist& riError )
{
  if (iLastSPos < 0)
    return;

  Int iStartFillSPos = iCurSPos;
  Int iStartFillPos  = iCurPos;
  Int iLastPos      = iCurPos + 1;

  Int iStartFillSPosFP = xRangeLeftL(iStartFillSPos);

  if (iStartFillSPosFP == xRoundL(iStartFillSPos))
  {
    if ((iStartFillSPosFP >= 0) && (iStartFillSPosFP < m_iLastOccludedSPosFP) )
    {
      m_iThisDepth = m_iCurDepth;
      xSetShiftedPel     ( iStartFillPos << m_iShiftPrec, iStartFillSPosFP, REN_IS_FILLED, riError );
    }
  }
  else
  {
    iStartFillSPosFP--;
  }

  m_iThisDepth = m_iLastDepth;
  for (Int iFillSPos = Max(iStartFillSPosFP+1,0); iFillSPos <= min(xRangeRightL( iLastSPos ), m_iLastOccludedSPosFP-1 ); iFillSPos++ )
  {
    xSetShiftedPel( iLastPos << m_iShiftPrec, iFillSPos, REN_IS_HOLE, riError );
  }
}

__inline Void
TRenSingleModel::xFillHoleR( Int iCurSPos, Int iLastSPos, Int iCurPos, RMDist& riError )
{
  if (iLastSPos < 0)
    return;

  Int iStartFillSPos = iCurSPos;
  Int iEndFillPos    = iCurPos;
  Int iLastPos       = iCurPos - 1;

  Int iStartFillSPosFP = xRangeRightR(iStartFillSPos);

  if (iStartFillSPosFP == xRoundR(iStartFillSPos))
  {
    if ((iStartFillSPosFP < m_iWidth) && (iStartFillSPosFP > m_iLastOccludedSPosFP) )
    {
      m_iThisDepth = m_iCurDepth;
      xSetShiftedPel( iEndFillPos << m_iShiftPrec, iStartFillSPosFP, REN_IS_FILLED, riError );
    }
  }
  else
  {
    iStartFillSPosFP++;
  }

  m_iThisDepth = m_iLastDepth;
  for (Int iFillSPos = max(xRangeLeftR( iLastSPos ), m_iLastOccludedSPosFP+1); iFillSPos <= min(iStartFillSPosFP,m_iWidth)-1 ; iFillSPos++ )
  {
    xSetShiftedPel( iLastPos << m_iShiftPrec, iFillSPos, REN_IS_HOLE, riError );
  }
}

__inline Void
TRenSingleModel::xExtrapolateMarginL(Int iCurSPos, Int iCurPos, RMDist& riError )
{
//  if (iLeftSPos < 0 )
//    return;

  Int iSPosFullPel = Max(0,xRangeLeftL(iCurSPos));

  m_iThisDepth = m_iCurDepth;
  if (iSPosFullPel < m_iWidth)
  {
    xSetShiftedPel( iCurPos << m_iShiftPrec, iSPosFullPel, REN_IS_FILLED, riError );
  }

  for (Int iFillSPos = iSPosFullPel +1; iFillSPos < m_iWidth; iFillSPos++ )
  {
    xSetShiftedPel( iCurPos << m_iShiftPrec, iFillSPos, REN_IS_HOLE, riError );
  }
}

__inline Void
TRenSingleModel::xExtrapolateMarginR(Int iCurSPos, Int iCurPos, RMDist& riError )
{
  //  if (iLeftSPos < 0 )
  //    return;

  Int iSPosFullPel = Min(m_iWidth-1,xRangeRightR(iCurSPos));

  m_iThisDepth = m_iCurDepth;
  if (iSPosFullPel > -1)
  {
    xSetShiftedPel( iCurPos << m_iShiftPrec, iSPosFullPel, REN_IS_FILLED, riError );
  }

  for (Int iFillSPos = iSPosFullPel -1; iFillSPos > -1; iFillSPos-- )
  {
    xSetShiftedPel( iCurPos << m_iShiftPrec, iFillSPos, REN_IS_HOLE, riError );
  }
}


__inline Int
TRenSingleModel::xShiftNewData( Int iPosX, Int iPosInNewData )
{
  AOT( iPosInNewData <               0 );
  AOF( iPosInNewData < m_iNewDataWidth );

  return (iPosX << m_iShiftPrec) - m_ppiCurLUT[0][ RemoveBitIncrement( m_piNewDepthData[iPosInNewData] )];
}

__inline Int
TRenSingleModel::xShift( Int iPosX )
{
 AOT( iPosX <        0);
 AOF( iPosX < m_iWidth);
 return (iPosX  << m_iShiftPrec) - m_ppiCurLUT[0][ RemoveBitIncrement( m_apiBaseDepthPelRow[m_iCurViewPos][iPosX] )];
}


__inline Int
TRenSingleModel::xShift( Int iPos, Int iPosInNewData )
{
  if ( (iPosInNewData >= 0) && (iPosInNewData < m_iNewDataWidth) )
  {
    return xShiftNewData(iPos ,iPosInNewData );
  }
  else
  {
    return xShift(iPos);
  }
}

__inline Int
TRenSingleModel::xRangeLeftL( Int iPos )
{
  return  ( iPos +  (1 << m_iShiftPrec) - 1) >> m_iShiftPrec;
}


__inline Int
TRenSingleModel::xRangeLeftR( Int iPos )
{

  return  xRangeRightR( iPos ) + 1;
}


__inline Int
TRenSingleModel::xRangeRightL( Int iPos )
{
  return xRangeLeftL(iPos) - 1;
}

__inline Int
TRenSingleModel::xRangeRightR( Int iPos )
{
  return iPos >> m_iShiftPrec;
}


__inline Int
TRenSingleModel::xRoundL( Int iPos )
{
  return  (iPos + (( 1 << m_iShiftPrec ) >> 1 )) >> m_iShiftPrec;
}

__inline Int
TRenSingleModel::xRoundR( Int iPos )
{
  return  (m_iShiftPrec == 0) ? iPos : xRoundL(iPos - 1);
}


Void
TRenSingleModel::xSetPels( Pel* piPelSource , Int iSourceStride, Int iWidth, Int iHeight, Pel iVal )
{
  for (Int iYPos = 0; iYPos < iHeight; iYPos++)
  {
    for (Int iXPos = 0; iXPos < iWidth; iXPos++)
    {
      piPelSource[iXPos] = iVal;
    }
    piPelSource += iSourceStride;
  }
}

Void
TRenSingleModel::xSetInts( Int* piPelSource , Int iSourceStride, Int iWidth, Int iHeight, Int iVal )
{
  for (Int iYPos = 0; iYPos < iHeight; iYPos++)
  {
    for (Int iXPos = 0; iXPos < iWidth; iXPos++)
    {
      piPelSource[iXPos] = iVal;
    }
    piPelSource += iSourceStride;
  }
}


Void
TRenSingleModel::xSetBools( Bool* pbPelSource , Int iSourceStride, Int iWidth, Int iHeight, Bool bVal )
{
  for (Int iYPos = 0; iYPos < iHeight; iYPos++)
  {
    for (Int iXPos = 0; iXPos < iWidth; iXPos++)
    {
      pbPelSource[iXPos] = bVal;
    }
    pbPelSource += iSourceStride;
  }
}

Void
TRenSingleModel::xInitView( Int iViewPos )
{
  AOT( iViewPos == VIEWPOS_MERGED && ( m_iMode == 0 || m_iMode == 1 ) );

  xSetPels( m_aapiSynthVideoPel[iViewPos][0], m_iStride, m_iWidth, m_iHeight, 0 );
  xSetPels( m_aapiSynthVideoPel[iViewPos][1], m_iStride, m_iWidth, m_iHeight, 128 << g_uiBitIncrement );
  xSetPels( m_aapiSynthVideoPel[iViewPos][2], m_iStride, m_iWidth, m_iHeight, 128 << g_uiBitIncrement );

  if ( iViewPos != VIEWPOS_MERGED)
  {
    xSetBools( m_apbOccluded     [iViewPos],  m_iStride, m_iWidth, m_iHeight, false );
    xSetPels ( m_apiFilled       [iViewPos],  m_iStride, m_iWidth, m_iHeight, REN_IS_HOLE);
    if ( m_iMode == 2 )
    {
      xSetPels( m_apiSynthDepthPel [iViewPos],  m_iStride, m_iWidth, m_iHeight, 0);
    }
  }
}

__inline Void
TRenSingleModel::xSetShiftedPel(Int iSourcePos, Int iTargetSPos, Pel iFilled, RMDist& riError )
{
  AOT( iSourcePos < 0         );
  AOT( iSourcePos >= m_iSampledWidth );

  AOT( iTargetSPos < 0         );
  AOT( iTargetSPos >= m_iWidth );
//  AOT(  m_apiFilledRow[m_iViewPos][iTargetSPos] != REN_IS_HOLE);

  if ( m_iMode == 2)
  {
    xSetShiftedPelBlend(iSourcePos, iTargetSPos, iFilled, riError );
    return;
  }

  if ( m_bSet )
  {
    m_aapiSynthVideoPelRow[m_iCurViewPos][0][iTargetSPos] = m_aapiBaseVideoPelRow[m_iCurViewPos][0][iSourcePos];
#if HHI_VSO_COLOR_PLANES
    m_aapiSynthVideoPelRow[m_iCurViewPos][1][iTargetSPos] = m_aapiBaseVideoPelRow[m_iCurViewPos][1][iSourcePos];
    m_aapiSynthVideoPelRow[m_iCurViewPos][2][iTargetSPos] = m_aapiBaseVideoPelRow[m_iCurViewPos][2][iSourcePos];
#endif
    m_apiFilledRow        [m_iCurViewPos]   [iTargetSPos] = iFilled;
    Int iDiffY = m_aapiRefVideoPelRow    [0][iTargetSPos] - m_aapiSynthVideoPelRow[m_iCurViewPos][0][iTargetSPos];
#if HHI_VSO_COLOR_PLANES
    Int iDiffU = m_aapiRefVideoPelRow    [1][iTargetSPos] - m_aapiSynthVideoPelRow[m_iCurViewPos][1][iTargetSPos];
    Int iDiffV = m_aapiRefVideoPelRow    [2][iTargetSPos] - m_aapiSynthVideoPelRow[m_iCurViewPos][2][iTargetSPos];
    m_apiErrorRow                           [iTargetSPos] = xGetDist( iDiffY, iDiffU, iDiffV);
#else
    m_apiErrorRow                           [iTargetSPos] = xGetDist(iDiffY);
#endif
  }
  else
  {
    Int iSDOld   = m_apiErrorRow            [iTargetSPos];
    Int iDiffY   = m_aapiRefVideoPelRow  [0][iTargetSPos] - m_aapiBaseVideoPelRow [m_iCurViewPos][0][iSourcePos];
#if HHI_VSO_COLOR_PLANES
    Int iDiffU   = m_aapiRefVideoPelRow  [1][iTargetSPos] - m_aapiBaseVideoPelRow [m_iCurViewPos][1][iSourcePos];
    Int iDiffV   = m_aapiRefVideoPelRow  [2][iTargetSPos] - m_aapiBaseVideoPelRow [m_iCurViewPos][2][iSourcePos];
    riError     += ( xGetDist(iDiffY,iDiffU,iDiffV) - iSDOld  );
#else
    riError     +=  ( xGetDist( iDiffY ) - iSDOld );
#endif
  }
}

__inline Void
TRenSingleModel::xSetShiftedPelBlend( Int iSourcePos, Int iTargetSPos, Pel iFilled, RMDist& riError )
{
  AOT( iSourcePos < 0         );
  AOT( iSourcePos >= m_iSampledWidth );

  AOT( iTargetSPos < 0         );
  AOT( iTargetSPos >= m_iWidth );
  //  AOT(  m_apiFilledRow[m_iViewPos][iTargetSPos] != REN_IS_HOLE);

  Pel piBlendedValueY;
#if HHI_VSO_COLOR_PLANES
  Pel piBlendedValueU;
  Pel piBlendedValueV;
#endif


  if (m_iCurViewPos == 0)
  {
    xGetBlendedValue (
      m_aapiBaseVideoPelRow                                    [0][0][iSourcePos ]  ,
      m_aapiSynthVideoPelRow                                   [1][0][iTargetSPos]  ,
#if HHI_VSO_COLOR_PLANES
      m_aapiBaseVideoPelRow                                    [0][1][iSourcePos ]  ,
      m_aapiSynthVideoPelRow                                   [1][1][iTargetSPos]  ,
      m_aapiBaseVideoPelRow                                    [0][2][iSourcePos ]  ,
      m_aapiSynthVideoPelRow                                   [1][2][iTargetSPos]  ,
#endif
      m_piInvZLUTLeft [RemoveBitIncrement(m_iThisDepth)                            ],
      m_piInvZLUTRight[RemoveBitIncrement(m_apiSynthDepthPelRow[1]   [iTargetSPos])],
      iFilled,
      m_apiFilledRow                                           [1]   [iTargetSPos]  ,
      piBlendedValueY
#if HHI_VSO_COLOR_PLANES
    , piBlendedValueU,
      piBlendedValueV
#endif
    );
  }
  else
  {
    xGetBlendedValue (
      m_aapiSynthVideoPelRow                                   [0][0][iTargetSPos],
      m_aapiBaseVideoPelRow                                    [1][0][iSourcePos ],
#if HHI_VSO_COLOR_PLANES
      m_aapiSynthVideoPelRow                                   [0][1][iTargetSPos],
      m_aapiBaseVideoPelRow                                    [1][1][iSourcePos ],
      m_aapiSynthVideoPelRow                                   [0][2][iTargetSPos],
      m_aapiBaseVideoPelRow                                    [1][2][iSourcePos ],
#endif
      m_piInvZLUTLeft [RemoveBitIncrement(m_apiSynthDepthPelRow[0]   [iTargetSPos])],
      m_piInvZLUTRight[RemoveBitIncrement(m_iThisDepth)                            ],
      m_apiFilledRow                                           [0]   [iTargetSPos],
      iFilled                                                                     ,
      piBlendedValueY
#if HHI_VSO_COLOR_PLANES
    , piBlendedValueU,
      piBlendedValueV
#endif
    );
  }

  if ( m_bSet )
  {
    m_apiSynthDepthPelRow [m_iCurViewPos]   [iTargetSPos] = m_iThisDepth;
    m_aapiSynthVideoPelRow[m_iCurViewPos][0][iTargetSPos] = m_aapiBaseVideoPelRow[m_iCurViewPos][0][iSourcePos];
    m_aapiSynthVideoPelRow[2            ][0][iTargetSPos] = piBlendedValueY;
#if HHI_VSO_COLOR_PLANES
    m_aapiSynthVideoPelRow[m_iCurViewPos][1][iTargetSPos] = m_aapiBaseVideoPelRow[m_iCurViewPos][1][iSourcePos];
    m_aapiSynthVideoPelRow[2            ][1][iTargetSPos] = piBlendedValueU;
    m_aapiSynthVideoPelRow[m_iCurViewPos][2][iTargetSPos] = m_aapiBaseVideoPelRow[m_iCurViewPos][2][iSourcePos];
    m_aapiSynthVideoPelRow[2            ][2][iTargetSPos] = piBlendedValueV;
#endif
    m_apiFilledRow        [m_iCurViewPos]   [iTargetSPos] = iFilled;

    Int iDiffY = m_aapiRefVideoPelRow    [0][iTargetSPos] - piBlendedValueY;
#if HHI_VSO_COLOR_PLANES
    Int iDiffU = m_aapiRefVideoPelRow    [1][iTargetSPos] - piBlendedValueU;
    Int iDiffV = m_aapiRefVideoPelRow    [2][iTargetSPos] - piBlendedValueV;
    m_apiErrorRow                           [iTargetSPos] = xGetDist(iDiffY, iDiffU, iDiffV );
#else
    m_apiErrorRow                           [iTargetSPos] = xGetDist(iDiffY);
#endif
  }
  else
  {
    Int iSDOld   = m_apiErrorRow            [iTargetSPos];
    Int iDiffY = m_aapiRefVideoPelRow    [0][iTargetSPos] - piBlendedValueY;
#if HHI_VSO_COLOR_PLANES
    Int iDiffU = m_aapiRefVideoPelRow    [1][iTargetSPos] - piBlendedValueU;
    Int iDiffV = m_aapiRefVideoPelRow    [2][iTargetSPos] - piBlendedValueV;
    riError   += ( xGetDist( iDiffY, iDiffU, iDiffV ) - iSDOld );
#else
    riError   += ( xGetDist( iDiffY )- iSDOld  );
#endif
  }
}


__inline Int
TRenSingleModel::xGetDist( Int iDiffY, Int iDiffU, Int iDiffV )
{
  return (          ((iDiffY * iDiffY) >> m_iDistShift)
             +  ((( ((iDiffU * iDiffU) >> m_iDistShift)
                   +((iDiffV * iDiffV) >> m_iDistShift)
                  )
                 ) >> 2
                )
         );
}

__inline Int
TRenSingleModel::xGetDist( Int iDiffY )
{
  return ((iDiffY * iDiffY) >> m_iDistShift);
}

#if HHI_VSO_COLOR_PLANES
__inline Void
TRenSingleModel::xGetBlendedValue( Pel iYL, Pel iYR, Pel iUL, Pel iUR, Pel iVL, Pel iVR, Pel iDepthL, Pel iDepthR, Int iFilledL, Int iFilledR, Pel& riY, Pel& riU, Pel&riV )
#else
Void
TRenSingleModel::xGetBlendedValue( Pel iYL, Pel iYR, Pel iDepthL, Pel iDepthR, Int iFilledL, Int iFilledR, Pel& riY )
#endif
{
  if (m_iBlendMode != 0 )
  {
    if (m_iBlendMode == 1 )
    {
#if HHI_VSO_COLOR_PLANES
      xGetBlendedValueBM1(  iYL,  iYR,  iUL,  iUR,  iVL,  iVR,  iDepthL,  iDepthR,  iFilledL,  iFilledR,  riY,  riU, riV );
#else
      xGetBlendedValueBM1(  iYL,  iYR,  iDepthL,  iDepthR,  iFilledL,  iFilledR,  riY );
#endif
    }
    else
    {
#if HHI_VSO_COLOR_PLANES
      xGetBlendedValueBM2(  iYL,  iYR,  iUL,  iUR,  iVL,  iVR,  iDepthL,  iDepthR,  iFilledL,  iFilledR,  riY,  riU, riV );
#else
      xGetBlendedValueBM2(  iYL,  iYR, iDepthL,  iDepthR,  iFilledL,  iFilledR,  riY );
#endif
    }
    return;
  }

  if (  (iFilledL != REN_IS_HOLE ) && ( iFilledR != REN_IS_HOLE) )
  {
    Int iDepthDifference = iDepthR - iDepthL;

    if ( abs ( iDepthDifference ) <= m_iBlendZThres )
    {
      if      ((iFilledL == REN_IS_FILLED) && ( iFilledR != REN_IS_FILLED))
      {
        riY = xBlend( iYL, iYR, iFilledR >> 1 );
#if HHI_VSO_COLOR_PLANES
        riU = xBlend( iUL, iUR, iFilledR >> 1 );
        riV = xBlend( iVL, iVR, iFilledR >> 1 );
#endif

      }
      else if ((iFilledL != REN_IS_FILLED) && ( iFilledR == REN_IS_FILLED))
      {
        riY = xBlend( iYR, iYL, (iFilledL >> 1) );
#if HHI_VSO_COLOR_PLANES
        riU = xBlend( iUR, iUL, (iFilledL >> 1) );
        riV = xBlend( iVR, iVL, (iFilledL >> 1) );
#endif
      }
      else
      {
        riY = xBlend( iYR, iYL, m_iBlendDistWeight );
#if HHI_VSO_COLOR_PLANES
        riU = xBlend( iUR, iUL, m_iBlendDistWeight );
        riV = xBlend( iVR, iVL, m_iBlendDistWeight );
#endif
      }
    }
    else if ( iDepthDifference < 0 )
    {
      riY = iYL;
#if HHI_VSO_COLOR_PLANES
      riU = iUL;
      riV = iVL;
#endif
    }
    else
    {
      riY = iYR;
#if HHI_VSO_COLOR_PLANES
      riU = iUR;
      riV = iVR;
#endif
    }
  }
  else if ( (iFilledL == REN_IS_HOLE) && (iFilledR == REN_IS_HOLE))
  {
    if ( iDepthR < iDepthL )
    {
        riY =  iYR;
#if HHI_VSO_COLOR_PLANES
        riU =  iUR;
        riV =  iVR;
#endif
    }
    else
    {
        riY =  iYL;
#if HHI_VSO_COLOR_PLANES
        riU =  iUL;
        riV =  iVL;
#endif
    }
  }
  else
  {
    if (iFilledR == REN_IS_HOLE)
    {
        riY = iYL;
#if HHI_VSO_COLOR_PLANES
        riU = iUL;
        riV = iVL;
#endif
    }
    else
    {
      riY = iYR;
#if HHI_VSO_COLOR_PLANES
      riU = iUR;
      riV = iVR;
#endif
    }
  }

}

__inline Void
#if HHI_VSO_COLOR_PLANES
TRenSingleModel::xGetBlendedValueBM1( Pel iYL, Pel iYR, Pel iUL, Pel iUR, Pel iVL, Pel iVR, Pel iDepthL, Pel iDepthR, Int iFilledL, Int iFilledR, Pel& riY, Pel& riU, Pel&riV )
#else
TRenSingleModel::xGetBlendedValueBM1( Pel iYL, Pel iYR, Pel iDepthL, Pel iDepthR, Int iFilledL, Int iFilledR, Pel& riY )
#endif
{
  if      ( iFilledL == REN_IS_FILLED ||  iFilledR == REN_IS_HOLE )
  {
    riY = iYL;
#if HHI_VSO_COLOR_PLANES
    riU = iUL;
    riV = iVL;
#endif
  }
  else if ( iFilledL == REN_IS_HOLE  )
  {
    riY = iYR;
#if HHI_VSO_COLOR_PLANES
    riU = iUR;
    riV = iVR;
#endif
  }
  else
  {
    riY = xBlend( iYR, iYL, iFilledL );
#if HHI_VSO_COLOR_PLANES
    riU = xBlend( iUR, iUL, iFilledL );
    riV = xBlend( iVR, iUL, iFilledL );
#endif
  }
}

__inline Void
#if HHI_VSO_COLOR_PLANES
TRenSingleModel::xGetBlendedValueBM2( Pel iYL, Pel iYR, Pel iUL, Pel iUR, Pel iVL, Pel iVR, Pel iDepthL, Pel iDepthR, Int iFilledL, Int iFilledR, Pel& riY, Pel& riU, Pel&riV )
#else
TRenSingleModel::xGetBlendedValueBM2( Pel iYL, Pel iYR, Pel iDepthL, Pel iDepthR, Int iFilledL, Int iFilledR, Pel& riY )
#endif
{
  if      ( iFilledR == REN_IS_FILLED ||  iFilledL == REN_IS_HOLE )
  {
    riY = iYR;
#if HHI_VSO_COLOR_PLANES
    riU = iUR;
    riV = iVR;
#endif
  }
  else if ( iFilledR == REN_IS_HOLE  )
  {
    riY = iYL;
#if HHI_VSO_COLOR_PLANES
    riU = iUL;
    riV = iVL;
#endif
  }
  else
  {
    riY = xBlend( iYL, iYR, iFilledR );
#if HHI_VSO_COLOR_PLANES
    riU = xBlend( iUL, iUR, iFilledR );
    riV = xBlend( iVL, iUR, iFilledR );
#endif
  }
}

__inline Pel
TRenSingleModel::xBlend( Pel pVal1, Pel pVal2, Int iWeightVal2 )
{
  return pVal1  +  (Pel) (  ( (Int) ( pVal2 - pVal1) * iWeightVal2 + (1 << (REN_VDWEIGHT_PREC - 1)) ) >> REN_VDWEIGHT_PREC );
}
