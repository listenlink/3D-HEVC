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

/** \file     TDecTop.cpp
    \brief    decoder class
*/

#include "../../App/TAppDecoder/TAppDecTop.h"
#include "TDecTop.h"



CamParsCollector::CamParsCollector()
: m_bInitialized( false )
{
  m_aaiCodedOffset         = new Int* [ MAX_NUMBER_VIEWS ];
  m_aaiCodedScale          = new Int* [ MAX_NUMBER_VIEWS ];
  m_aiViewOrderIndex       = new Int  [ MAX_NUMBER_VIEWS ];
  m_aiViewReceived         = new Int  [ MAX_NUMBER_VIEWS ];
  for( UInt uiId = 0; uiId < MAX_NUMBER_VIEWS; uiId++ )
  {
    m_aaiCodedOffset      [ uiId ] = new Int [ MAX_NUMBER_VIEWS ];
    m_aaiCodedScale       [ uiId ] = new Int [ MAX_NUMBER_VIEWS ];
  }
#if POZNAN_CU_SKIP || POZNAN_CU_SYNTH || (POZNAN_MP && !POZNAN_MP_USE_DEPTH_MAP_GENERATION)
  xCreateLUTs   ( (UInt)MAX_NUMBER_VIEWS, (UInt)MAX_NUMBER_VIEWS,  m_adBaseViewShiftLUT,  m_aiBaseViewShiftLUT);

  m_iLog2Precision   = LOG2_DISP_PREC_LUT;
  m_uiBitDepthForLUT = 8; //fixed
#endif
}

CamParsCollector::~CamParsCollector()
{
  for( UInt uiId = 0; uiId < MAX_NUMBER_VIEWS; uiId++ )
  {
    delete [] m_aaiCodedOffset      [ uiId ];
    delete [] m_aaiCodedScale       [ uiId ];
  }
  delete [] m_aaiCodedOffset;
  delete [] m_aaiCodedScale;
  delete [] m_aiViewOrderIndex;
  delete [] m_aiViewReceived;

#if POZNAN_CU_SKIP || POZNAN_CU_SYNTH || (POZNAN_MP && !POZNAN_MP_USE_DEPTH_MAP_GENERATION)
  xDeleteArray( m_adBaseViewShiftLUT,        MAX_NUMBER_VIEWS, MAX_NUMBER_VIEWS,  2 );
  xDeleteArray( m_aiBaseViewShiftLUT,        MAX_NUMBER_VIEWS, MAX_NUMBER_VIEWS,  2 );
#endif
}

Void
CamParsCollector::init( FILE* pCodedScaleOffsetFile )
{
  m_bInitialized            = true;
  m_pCodedScaleOffsetFile   = pCodedScaleOffsetFile;
  m_uiCamParsCodedPrecision = 0;
  m_bCamParsVaryOverTime    = false;
  m_iLastViewId             = -1;
  m_iLastPOC                = -1;
  m_uiMaxViewId             = 0;
}

#if POZNAN_CU_SKIP || POZNAN_CU_SYNTH || (POZNAN_MP && !POZNAN_MP_USE_DEPTH_MAP_GENERATION)
/*
Void
CamParsCollector::initLUT(TComSPS &cComSPS)
{
    //===== create arrays =====
  //xCreateLUTs   ( (UInt)m_iNumberOfBaseViews, (UInt)m_iNumberOfBaseViews,  m_adBaseViewShiftLUT,  m_aiBaseViewShiftLUT,  m_adBaseViewShiftParameter,  m_aiBaseViewShiftParameter  );
  
  
  //xCreate2dArray( (UInt)m_iNumberOfBaseViews, (UInt)m_iNumberOfBaseViews,  m_aaiCodedScale           );
  //xCreate2dArray( (UInt)m_iNumberOfBaseViews, (UInt)m_iNumberOfBaseViews,  m_aaiCodedOffset          );
  //xCreate2dArray( (UInt)m_iNumberOfBaseViews, (UInt)m_iNumberOfBaseViews,  m_aaiScaleAndOffsetSet    );
  //xInit2dArray  ( (UInt)m_iNumberOfBaseViews, (UInt)m_iNumberOfBaseViews,  m_aaiScaleAndOffsetSet, 0 );

  //xCreate2dArray( (UInt)m_iNumberOfBaseViews, (UInt)m_iNumberOfBaseViews,  m_aaiPdmScaleNomDelta     );
  //xCreate2dArray( (UInt)m_iNumberOfBaseViews, (UInt)m_iNumberOfBaseViews,  m_aaiPdmOffset            );

  //===== init disparity to virtual depth conversion parameters =====
  //xSetPdmConversionParams();

  //===== init arrays for first frame =====
  //xSetShiftParametersAndLUT( m_uiFirstFrameId );
}//*/

Void
CamParsCollector::xCreateLUTs( UInt uiNumberSourceViews, UInt uiNumberTargetViews, Double****& radLUT, Int****& raiLUT)
{
  //AOF( m_uiBitDepthForLUT == 8 );
  //AOF(radLUT == NULL && raiLUT == NULL );

  uiNumberSourceViews = Max( 1, uiNumberSourceViews );
  uiNumberTargetViews = Max( 1, uiNumberTargetViews );

  radLUT         = new Double***[ uiNumberSourceViews ];
  raiLUT         = new Int   ***[ uiNumberSourceViews ];

  for( UInt uiSourceView = 0; uiSourceView < uiNumberSourceViews; uiSourceView++ )
  {
    radLUT        [ uiSourceView ] = new Double**[ uiNumberTargetViews ];
    raiLUT        [ uiSourceView ] = new Int   **[ uiNumberTargetViews ];

    for( UInt uiTargetView = 0; uiTargetView < uiNumberTargetViews; uiTargetView++ )
    {
      radLUT        [ uiSourceView ][ uiTargetView ]      = new Double*[ 2 ];
      radLUT        [ uiSourceView ][ uiTargetView ][ 0 ] = new Double [ 257 ];
      radLUT        [ uiSourceView ][ uiTargetView ][ 1 ] = new Double [ 257 ];

      raiLUT        [ uiSourceView ][ uiTargetView ]      = new Int*   [ 2 ];
      raiLUT        [ uiSourceView ][ uiTargetView ][ 0 ] = new Int    [ 257 ];
      raiLUT        [ uiSourceView ][ uiTargetView ][ 1 ] = new Int    [ 257 ];
    }
  }
}

Void 
CamParsCollector::xInitLUTs( UInt uiSourceView, UInt uiTargetView, Int iScale, Int iOffset, Double****& radLUT, Int****& raiLUT)
{
  Int     iLog2DivLuma   = m_uiBitDepthForLUT + m_uiCamParsCodedPrecision + 1 - m_iLog2Precision;   AOF( iLog2DivLuma > 0 );
  Int     iLog2DivChroma = iLog2DivLuma + 1;

  iOffset <<= m_uiBitDepthForLUT;

  Double dScale  = (Double) iScale  / (( Double ) ( 1 << iLog2DivLuma ));
  Double dOffset = (Double) iOffset / (( Double ) ( 1 << iLog2DivLuma ));

  // offsets including rounding offsets
  Int64 iOffsetLuma   = iOffset + ( ( 1 << iLog2DivLuma   ) >> 1 );
  Int64 iOffsetChroma = iOffset + ( ( 1 << iLog2DivChroma ) >> 1 );

  for( UInt uiDepthValue = 0; uiDepthValue < 256; uiDepthValue++ )
  {
    // real-valued look-up tables
    Double  dShiftLuma      = ( (Double)uiDepthValue * dScale + dOffset ) * Double( 1 << m_iLog2Precision );
    Double  dShiftChroma    = dShiftLuma / 2;
    radLUT[ uiSourceView ][ uiTargetView ][ 0 ][ uiDepthValue ] = dShiftLuma;
    radLUT[ uiSourceView ][ uiTargetView ][ 1 ][ uiDepthValue ] = dShiftChroma;

    // integer-valued look-up tables
    Int64   iTempScale      = (Int64)uiDepthValue * iScale;
    Int64   iTestScale      = ( iTempScale + iOffset       );   // for checking accuracy of camera parameters
    Int64   iShiftLuma      = ( iTempScale + iOffsetLuma   ) >> iLog2DivLuma;
    Int64   iShiftChroma    = ( iTempScale + iOffsetChroma ) >> iLog2DivChroma;
    raiLUT[ uiSourceView ][ uiTargetView ][ 0 ][ uiDepthValue ] = (Int)iShiftLuma;
    raiLUT[ uiSourceView ][ uiTargetView ][ 1 ][ uiDepthValue ] = (Int)iShiftChroma;

    // maximum deviation
    //dMaxDispDev     = Max( dMaxDispDev,    fabs( Double( (Int) iTestScale   ) - dShiftLuma * Double( 1 << iLog2DivLuma ) ) / Double( 1 << iLog2DivLuma ) );
    //dMaxRndDispDvL  = Max( dMaxRndDispDvL, fabs( Double( (Int) iShiftLuma   ) - dShiftLuma   ) );
    //dMaxRndDispDvC  = Max( dMaxRndDispDvC, fabs( Double( (Int) iShiftChroma ) - dShiftChroma ) );
  }

  radLUT[ uiSourceView ][ uiTargetView ][ 0 ][ 256 ] = radLUT[ uiSourceView ][ uiTargetView ][ 0 ][ 255 ];
  radLUT[ uiSourceView ][ uiTargetView ][ 1 ][ 256 ] = radLUT[ uiSourceView ][ uiTargetView ][ 1 ][ 255 ];
  raiLUT[ uiSourceView ][ uiTargetView ][ 0 ][ 256 ] = raiLUT[ uiSourceView ][ uiTargetView ][ 0 ][ 255 ];
  raiLUT[ uiSourceView ][ uiTargetView ][ 1 ][ 256 ] = raiLUT[ uiSourceView ][ uiTargetView ][ 1 ][ 255 ];
}

Bool
CamParsCollector::getNearestBaseView( Int iSynthViewIdx, Int &riNearestViewIdx, Int &riRelDistToLeft, Bool& rbRenderFromLeft)
{
  /*
  riNearestViewIdx = 0;

  Bool bDecencdingVN = ( m_aiSortedBaseViews.size() >= 2 && m_aiSortedBaseViews[ 0 ] > m_aiSortedBaseViews[ 1 ] );
  Int  iFactor       = ( bDecencdingVN ? -1 : 1 );

  if( ( m_aiBaseId2SortedId[iSynthViewIdx] - m_aiBaseId2SortedId[riNearestViewIdx] ) * iFactor  <= 0 )
  {
    rbRenderFromLeft = true;
  }
  else
  {
    rbRenderFromLeft = false;
  }

  riRelDistToLeft = 128; //Not used for now;
//*/
  return true;
}

#endif

Void
CamParsCollector::uninit()
{
  m_bInitialized = false;
}

Void
CamParsCollector::setSlice( TComSlice* pcSlice )
{
  if( pcSlice == 0 )
  {
    AOF( xIsComplete() );
    if( m_bCamParsVaryOverTime || m_iLastPOC == 0 )
    {
      xOutput( m_iLastPOC );
    }
    return;
  }

  AOF( pcSlice->getSPS()->getViewId() < MAX_NUMBER_VIEWS );
  if ( pcSlice->getSPS()->isDepth  () )
  {
    return;
  }
  Bool  bFirstAU          = ( pcSlice->getPOC()               == 0 );
  Bool  bFirstSliceInAU   = ( pcSlice->getPOC()               != Int ( m_iLastPOC ) );
  Bool  bFirstSliceInView = ( pcSlice->getSPS()->getViewId()  != UInt( m_iLastViewId ) || bFirstSliceInAU );
  AOT(  bFirstSliceInAU  &&   pcSlice->getSPS()->getViewId()  != 0 );
  AOT( !bFirstSliceInAU  &&   pcSlice->getSPS()->getViewId()   < UInt( m_iLastViewId ) );
  AOT( !bFirstSliceInAU  &&   pcSlice->getSPS()->getViewId()   > UInt( m_iLastViewId + 1 ) );
  AOT( !bFirstAU         &&   pcSlice->getSPS()->getViewId()   > m_uiMaxViewId );
  if ( !bFirstSliceInView )
  {
    if( m_bCamParsVaryOverTime ) // check consistency of slice parameters here
    {
      UInt uiViewId = pcSlice->getSPS()->getViewId();
      for( UInt uiBaseId = 0; uiBaseId < uiViewId; uiBaseId++ )
      {
        AOF( m_aaiCodedScale [ uiBaseId ][ uiViewId ] == pcSlice->getCodedScale    () [ uiBaseId ] );
        AOF( m_aaiCodedOffset[ uiBaseId ][ uiViewId ] == pcSlice->getCodedOffset   () [ uiBaseId ] );
        AOF( m_aaiCodedScale [ uiViewId ][ uiBaseId ] == pcSlice->getInvCodedScale () [ uiBaseId ] );
        AOF( m_aaiCodedOffset[ uiViewId ][ uiBaseId ] == pcSlice->getInvCodedOffset() [ uiBaseId ] );
      }
    }
    return;
  }

  if( bFirstSliceInAU )
  {
    if( !bFirstAU )
    {
      AOF( xIsComplete() );
      xOutput( m_iLastPOC );
    }
    ::memset( m_aiViewReceived, 0x00, MAX_NUMBER_VIEWS * sizeof( Int ) );
  }

  UInt uiViewId                 = pcSlice->getSPS()->getViewId();
  m_aiViewReceived[ uiViewId ]  = 1;
  if( bFirstAU )
  {
    m_uiMaxViewId                     = Max( m_uiMaxViewId, uiViewId );
    m_aiViewOrderIndex[ uiViewId ]    = pcSlice->getSPS()->getViewOrderIdx();
    if( uiViewId == 1 )
    {
      m_uiCamParsCodedPrecision       = pcSlice->getSPS()->getCamParPrecision     ();
      m_bCamParsVaryOverTime          = pcSlice->getSPS()->hasCamParInSliceHeader ();
    }
    else if( uiViewId > 1 )
    {
      AOF( m_uiCamParsCodedPrecision == pcSlice->getSPS()->getCamParPrecision     () );
      AOF( m_bCamParsVaryOverTime    == pcSlice->getSPS()->hasCamParInSliceHeader () );
    }
    for( UInt uiBaseId = 0; uiBaseId < uiViewId; uiBaseId++ )
    {
      if( m_bCamParsVaryOverTime )
      {
        m_aaiCodedScale [ uiBaseId ][ uiViewId ]  = pcSlice->getCodedScale    () [ uiBaseId ];
        m_aaiCodedOffset[ uiBaseId ][ uiViewId ]  = pcSlice->getCodedOffset   () [ uiBaseId ];
        m_aaiCodedScale [ uiViewId ][ uiBaseId ]  = pcSlice->getInvCodedScale () [ uiBaseId ];
        m_aaiCodedOffset[ uiViewId ][ uiBaseId ]  = pcSlice->getInvCodedOffset() [ uiBaseId ];
#if POZNAN_CU_SKIP || POZNAN_CU_SYNTH || (POZNAN_MP && !POZNAN_MP_USE_DEPTH_MAP_GENERATION)
        xInitLUTs(uiBaseId,uiViewId,m_aaiCodedScale [ uiBaseId ][ uiViewId ],m_aaiCodedOffset[ uiBaseId ][ uiViewId ],m_adBaseViewShiftLUT,m_aiBaseViewShiftLUT);
        xInitLUTs(uiViewId,uiBaseId,m_aaiCodedScale [ uiViewId ][ uiBaseId ],m_aaiCodedOffset[ uiViewId ][ uiBaseId ],m_adBaseViewShiftLUT,m_aiBaseViewShiftLUT);
#endif
      }
      else
      {
        m_aaiCodedScale [ uiBaseId ][ uiViewId ]  = pcSlice->getSPS()->getCodedScale    () [ uiBaseId ];
        m_aaiCodedOffset[ uiBaseId ][ uiViewId ]  = pcSlice->getSPS()->getCodedOffset   () [ uiBaseId ];
        m_aaiCodedScale [ uiViewId ][ uiBaseId ]  = pcSlice->getSPS()->getInvCodedScale () [ uiBaseId ];
        m_aaiCodedOffset[ uiViewId ][ uiBaseId ]  = pcSlice->getSPS()->getInvCodedOffset() [ uiBaseId ];
#if POZNAN_CU_SKIP || POZNAN_CU_SYNTH || (POZNAN_MP && !POZNAN_MP_USE_DEPTH_MAP_GENERATION)
        xInitLUTs(uiBaseId,uiViewId,m_aaiCodedScale [ uiBaseId ][ uiViewId ],m_aaiCodedOffset[ uiBaseId ][ uiViewId ],m_adBaseViewShiftLUT,m_aiBaseViewShiftLUT);
        xInitLUTs(uiViewId,uiBaseId,m_aaiCodedScale [ uiViewId ][ uiBaseId ],m_aaiCodedOffset[ uiViewId ][ uiBaseId ],m_adBaseViewShiftLUT,m_aiBaseViewShiftLUT);
#endif
      }
    }
  }
  else
  {
    AOF( m_aiViewOrderIndex[ uiViewId ] == pcSlice->getSPS()->getViewOrderIdx() );
    if( m_bCamParsVaryOverTime )
    {
      for( UInt uiBaseId = 0; uiBaseId < uiViewId; uiBaseId++ )
      {
        m_aaiCodedScale [ uiBaseId ][ uiViewId ]  = pcSlice->getCodedScale    () [ uiBaseId ];
        m_aaiCodedOffset[ uiBaseId ][ uiViewId ]  = pcSlice->getCodedOffset   () [ uiBaseId ];
        m_aaiCodedScale [ uiViewId ][ uiBaseId ]  = pcSlice->getInvCodedScale () [ uiBaseId ];
        m_aaiCodedOffset[ uiViewId ][ uiBaseId ]  = pcSlice->getInvCodedOffset() [ uiBaseId ];
#if POZNAN_CU_SKIP || POZNAN_CU_SYNTH || (POZNAN_MP && !POZNAN_MP_USE_DEPTH_MAP_GENERATION)
        xInitLUTs(uiBaseId,uiViewId,m_aaiCodedScale [ uiBaseId ][ uiViewId ],m_aaiCodedOffset[ uiBaseId ][ uiViewId ],m_adBaseViewShiftLUT,m_aiBaseViewShiftLUT);
        xInitLUTs(uiViewId,uiBaseId,m_aaiCodedScale [ uiViewId ][ uiBaseId ],m_aaiCodedOffset[ uiViewId ][ uiBaseId ],m_adBaseViewShiftLUT,m_aiBaseViewShiftLUT);
#endif
      }
    }
  }
  m_iLastViewId = (Int)pcSlice->getSPS()->getViewId();
  m_iLastPOC    = (Int)pcSlice->getPOC();
}

Bool
CamParsCollector::xIsComplete()
{
  for( UInt uiView = 0; uiView <= m_uiMaxViewId; uiView++ )
  {
    if( m_aiViewReceived[ uiView ] == 0 )
    {
      return false;
    }
  }
  return true;
}

Void
CamParsCollector::xOutput( Int iPOC )
{
  if( m_pCodedScaleOffsetFile )
  {
    if( iPOC == 0 )
    {
      fprintf( m_pCodedScaleOffsetFile, "#     ViewId ViewOrderIdx\n" );
      fprintf( m_pCodedScaleOffsetFile, "#----------- ------------\n" );
      for( UInt uiViewId = 0; uiViewId <= m_uiMaxViewId; uiViewId++ )
      {
        fprintf( m_pCodedScaleOffsetFile, "%12d %12d\n", uiViewId, m_aiViewOrderIndex[ uiViewId ] );
      }
      fprintf( m_pCodedScaleOffsetFile, "\n\n");
      fprintf( m_pCodedScaleOffsetFile, "# StartFrame     EndFrame   TargetView     BaseView   CodedScale  CodedOffset    Precision\n" );
      fprintf( m_pCodedScaleOffsetFile, "#----------- ------------ ------------ ------------ ------------ ------------ ------------\n" );
    }
    if( iPOC == 0 || m_bCamParsVaryOverTime )
    {
      Int iS = iPOC;
      Int iE = ( m_bCamParsVaryOverTime ? iPOC : ~( 1 << 31 ) );
      for( UInt uiViewId = 0; uiViewId <= m_uiMaxViewId; uiViewId++ )
      {
        for( UInt uiBaseId = 0; uiBaseId <= m_uiMaxViewId; uiBaseId++ )
        {
          if( uiViewId != uiBaseId )
          {
            fprintf( m_pCodedScaleOffsetFile, "%12d %12d %12d %12d %12d %12d %12d\n",
                     iS, iE, uiViewId, uiBaseId, m_aaiCodedScale[ uiBaseId ][ uiViewId ], m_aaiCodedOffset[ uiBaseId ][ uiViewId ], m_uiCamParsCodedPrecision );
          }
        }
      }
    }
  }
}




TDecTop::TDecTop()
: m_SEIs(0)
{
  m_iGopSize      = 0;
  m_bGopSizeSet   = false;
  m_iMaxRefPicNum = 0;
  m_uiValidPS = 0;
  m_bIsDepth = false ;
  m_iViewIdx = 0 ;
#if ENC_DEC_TRACE
  g_hTrace = fopen( "TraceDec.txt", "wb" );
  g_bJustDoIt = g_bEncDecTraceDisable;
  g_nSymbolCounter = 0;
#endif
#if DCM_DECODING_REFRESH
  m_bRefreshPending = 0;
  m_uiPOCCDR = 0;
#if DCM_SKIP_DECODING_FRAMES
  m_uiPOCRA = MAX_UINT;
#endif
#endif
  m_uiPrevPOC               = UInt(-1);
  m_bFirstSliceInPicture    = true;
  m_bFirstSliceInSequence   = true;
  m_pcCamParsCollector = 0;
#if POZNAN_MP
  m_pcMP = NULL;
#endif
}

TDecTop::~TDecTop()
{
#if ENC_DEC_TRACE
  fclose( g_hTrace );
#endif
}

Void TDecTop::create()
{
  m_cGopDecoder.create();
  m_apcSlicePilot = new TComSlice;
  m_uiSliceIdx = m_uiLastSliceIdx = 0;
}

Void TDecTop::destroy()
{
  m_cGopDecoder.destroy();

  delete m_apcSlicePilot;
  m_apcSlicePilot = NULL;

  m_cSliceDecoder.destroy();

#if DEPTH_MAP_GENERATION
  m_cDepthMapGenerator.destroy();
#endif
#if HHI_INTER_VIEW_RESIDUAL_PRED
  m_cResidualGenerator.destroy();
#endif

#if POZNAN_MP
  m_pcMP = NULL;
#endif
}

Void TDecTop::init( TAppDecTop* pcTAppDecTop, Bool bFirstInstance )
{
  // initialize ROM
  if( bFirstInstance )
    initROM();
#if MTK_SAO
  m_cGopDecoder.  init( &m_cEntropyDecoder, &m_cSbacDecoder, &m_cBinCABAC, &m_cCavlcDecoder, &m_cSliceDecoder, &m_cLoopFilter, &m_cAdaptiveLoopFilter, &m_cSAO
#if DEPTH_MAP_GENERATION
                      , &m_cDepthMapGenerator
#endif
#if HHI_INTER_VIEW_RESIDUAL_PRED
                      , &m_cResidualGenerator 
#endif
                      );
#else
  m_cGopDecoder.  init( &m_cEntropyDecoder, &m_cSbacDecoder, &m_cBinCABAC, &m_cCavlcDecoder, &m_cSliceDecoder, &m_cLoopFilter, &m_cAdaptiveLoopFilter
#if DEPTH_MAP_GENERATION
                      , &m_cDepthMapGenerator
#endif
#if HHI_INTER_VIEW_RESIDUAL_PRED
                      , &m_cResidualGenerator 
#endif
                      );
#endif
  m_cSliceDecoder.init( &m_cEntropyDecoder, &m_cCuDecoder );
  m_cEntropyDecoder.init(&m_cPrediction);

  m_pcTAppDecTop = pcTAppDecTop;
#if DEPTH_MAP_GENERATION
  m_cDepthMapGenerator.init( &m_cPrediction, m_pcTAppDecTop->getSPSAccess(), m_pcTAppDecTop->getAUPicAccess() );
#endif
#if HHI_INTER_VIEW_RESIDUAL_PRED
  m_cResidualGenerator.init( &m_cTrQuant, &m_cDepthMapGenerator );
#endif

#if POZNAN_MP
  m_pcMP = pcTAppDecTop->getMP();
#endif
}

Void TDecTop::setSPS(TComSPS cSPS)
{
  m_cSPS = cSPS ;
      if ( !m_cAdaptiveLoopFilter.isCreated())
      {
  m_cAdaptiveLoopFilter.create( m_cSPS.getWidth(), m_cSPS.getHeight(), g_uiMaxCUWidth, g_uiMaxCUHeight, g_uiMaxCUDepth );
#if MTK_SAO
  m_cSAO.create( m_cSPS.getWidth(), m_cSPS.getHeight(), g_uiMaxCUWidth, g_uiMaxCUHeight, g_uiMaxCUDepth );
#endif
  m_cLoopFilter.        create( g_uiMaxCUDepth );
      }
  m_uiValidPS |= 1;
}

Void TDecTop::deletePicBuffer ( )
{
  TComList<TComPic*>::iterator  iterPic   = m_cListPic.begin();
  Int iSize = Int( m_cListPic.size() );

  for (Int i = 0; i < iSize; i++ )
  {
    TComPic* pcPic = *(iterPic++);
    pcPic->destroy();

    delete pcPic;
    pcPic = NULL;
  }

  // destroy ALF temporary buffers
  m_cAdaptiveLoopFilter.destroy();

#if MTK_SAO
  m_cSAO.destroy();
#endif

  m_cLoopFilter.        destroy();

  // destroy ROM
  if( m_iViewIdx <= 0 && !m_bIsDepth)
    destroyROM();
}

Void TDecTop::xGetNewPicBuffer ( TComSlice* pcSlice, TComPic*& rpcPic )
{
  m_iMaxRefPicNum = getCodedPictureBufferSize( );

#if DEPTH_MAP_GENERATION
  Bool bNeedPrdDepthMapBuffer = ( !pcSlice->getSPS()->isDepth() && ( pcSlice->getSPS()->getViewId() == 0 || pcSlice->getSPS()->getPredDepthMapGeneration() > 0 ) );
#endif

  if (m_cListPic.size() < (UInt)m_iMaxRefPicNum)
  {
    rpcPic = new TComPic;
    rpcPic->create ( pcSlice->getSPS()->getWidth(), pcSlice->getSPS()->getHeight(), g_uiMaxCUWidth, g_uiMaxCUHeight, g_uiMaxCUDepth );
#if DEPTH_MAP_GENERATION
    if( bNeedPrdDepthMapBuffer )
    {
      rpcPic->addPrdDepthMapBuffer();
    }
#endif
    m_cListPic.pushBack( rpcPic );

    return;
  }

  Bool bBufferIsAvailable = false;
  TComList<TComPic*>::iterator  iterPic   = m_cListPic.begin();
  while (iterPic != m_cListPic.end())
  {
    rpcPic = *(iterPic++);
    if ( rpcPic->getReconMark() == false )
    {
      bBufferIsAvailable = true;
      break;
    }
  }

  if ( !bBufferIsAvailable )
  {
    pcSlice->sortPicList(m_cListPic);
    iterPic = m_cListPic.begin();
    rpcPic = *(iterPic);
    rpcPic->setReconMark(false);

    // mark it should be extended
  }
  rpcPic->getPicYuvRec()->setBorderExtension(false);

#if DEPTH_MAP_GENERATION
  if( bNeedPrdDepthMapBuffer && !rpcPic->getPredDepthMap() )
  {
    rpcPic->addPrdDepthMapBuffer();
  }
#endif
}


Void
TDecTop::deleteExtraPicBuffers( Int iPoc )
{
  TComPic*                      pcPic = 0;
  TComList<TComPic*>::iterator  cIter = m_cListPic.begin();
  TComList<TComPic*>::iterator  cEnd  = m_cListPic.end  ();
  for( ; cIter != cEnd; cIter++ )
  {
    if( (*cIter)->getPOC() == iPoc )
    {
      pcPic = *cIter;
      break;
    }
  }
  AOF( pcPic );
  if ( pcPic )
  {
    pcPic->removeOriginalBuffer   ();
#if HHI_INTER_VIEW_MOTION_PRED
    pcPic->removeOrgDepthMapBuffer();
#endif
#if HHI_INTER_VIEW_RESIDUAL_PRED
    pcPic->removeResidualBuffer   ();
#endif
#if HHI_INTERVIEW_SKIP
    pcPic->removeUsedPelsMapBuffer();
#endif
#if POZNAN_CU_SYNTH || POZNAN_CU_SKIP
    pcPic->removeAvailabilityBuffer();
#endif
#if POZNAN_CU_SYNTH
    pcPic->removeSynthesisBuffer();
#endif
#if POZNAN_TEXTURE_TU_DELTA_QP_ACCORDING_TO_DEPTH
    pcPic->removeSynthesisDepthBuffer();
#endif
  }
}

#if AMVP_BUFFERCOMPRESS
Void
TDecTop::compressMotion( Int iPoc )
{
  TComPic*                      pcPic = 0;
  TComList<TComPic*>::iterator  cIter = m_cListPic.begin();
  TComList<TComPic*>::iterator  cEnd  = m_cListPic.end  ();
  for( ; cIter != cEnd; cIter++ )
  {
    if( (*cIter)->getPOC() == iPoc )
    {
      pcPic = *cIter;
      break;
    }
  }
  AOF( pcPic );
  if ( pcPic )
  {
    pcPic->compressMotion();
  }
}
#endif


Void TDecTop::executeDeblockAndAlf(Bool bEos, TComBitstream* pcBitstream, UInt& ruiPOC, TComList<TComPic*>*& rpcListPic, Int& iSkipFrame,  Int& iPOCLastDisplay)
{
  TComPic*&   pcPic         = m_pcPic;

  // Execute Deblock and ALF only + Cleanup
  TComSlice* pcSlice  = pcPic->getPicSym()->getSlice( m_uiSliceIdx                  );
  m_cGopDecoder.decompressGop ( bEos, pcBitstream, pcPic, true);

  if( m_pcCamParsCollector && bEos )
  {
    m_pcCamParsCollector->setSlice( 0 );
  }

  pcSlice->sortPicList        ( m_cListPic );       //  sorting for application output
  ruiPOC              = pcPic->getSlice(m_uiSliceIdx-1)->getPOC();
  rpcListPic          = &m_cListPic;
  m_cCuDecoder.destroy();
  m_bFirstSliceInPicture  = true;

  return;
}

#if DCM_SKIP_DECODING_FRAMES
Bool TDecTop::decode (Bool bEos, TComBitstream* pcBitstream, UInt& ruiPOC, TComList<TComPic*>*& rpcListPic, NalUnitType& reNalUnitType, TComSPS& cComSPS, Int& iSkipFrame,  Int& iPOCLastDisplay)
#else
Void TDecTop::decode (Bool bEos, TComBitstream* pcBitstream, UInt& ruiPOC, TComList<TComPic*>*& rpcListPic, NalUnitType& reNalUnitType, TComSPS& cComSPS )
#endif
{
  if (m_bFirstSliceInPicture)
  {
    rpcListPic = NULL;
  }
  TComPic*&   pcPic         = m_pcPic;

  // Initialize entropy decoder
  m_cEntropyDecoder.setEntropyDecoder (&m_cCavlcDecoder);
  m_cEntropyDecoder.setBitstream      (pcBitstream);

  NalUnitType eNalUnitType;
  UInt        TemporalId;
  Bool        OutputFlag;

  m_cEntropyDecoder.decodeNalUnitHeader(eNalUnitType, TemporalId, OutputFlag);
  reNalUnitType = eNalUnitType;

  switch (eNalUnitType)
  {
    case NAL_UNIT_SPS:
    {
      TComSPS cTempSPS;
      m_cEntropyDecoder.decodeSPS( &cTempSPS );

      if( (m_iViewIdx == cTempSPS.getViewId()) && ( m_bIsDepth == cTempSPS.isDepth() ) )
      {
        m_cSPS = cTempSPS;
        cComSPS = m_cSPS;
      }
      else
      {
#if POZNAN_NONLINEAR_DEPTH
        // For texture complete depth power information. Depth power is sended, for example, for base view depth map and it should be available prior non-base texture decoding
        if(!cTempSPS.isDepth() && cTempSPS.getViewId())
        {
          Float fDepthPower = getDecTop()->getPicFromView( 0, pcPic->getPOC(), true )->getSPS()->getDepthPower();
          cTempSPS.setDepthPower(fDepthPower);
        }
#endif
        cComSPS = cTempSPS;
        return false;
      }
      

      // create ALF temporary buffer
      if ( !m_cAdaptiveLoopFilter.isCreated())
      {
      m_cAdaptiveLoopFilter.create( m_cSPS.getWidth(), m_cSPS.getHeight(), g_uiMaxCUWidth, g_uiMaxCUHeight, g_uiMaxCUDepth );
#if MTK_SAO
      m_cSAO.create( m_cSPS.getWidth(), m_cSPS.getHeight(), g_uiMaxCUWidth, g_uiMaxCUHeight, g_uiMaxCUDepth );
#endif
      m_cLoopFilter.        create( g_uiMaxCUDepth );
      }
      m_uiValidPS |= 1;

#if POZNAN_MP
#if POZNAN_MP_USE_DEPTH_MAP_GENERATION
      m_pcMP->init( m_cSPS.getHeight(), m_cSPS.getWidth() );
#else
      m_pcMP->init( m_cSPS.getHeight(), m_cSPS.getWidth(), m_pcCamParsCollector->getBaseViewShiftLUTI());
#endif 
#endif 

      return false;
    }

    case NAL_UNIT_PPS:
      m_cEntropyDecoder.decodePPS( &m_cPPS );
      assert( m_cPPS.getSPSId() == m_cSPS.getSPSId() );
      m_uiValidPS |= 2;
      return false;

    case NAL_UNIT_SEI:
      m_SEIs = new SEImessages;
      m_cEntropyDecoder.decodeSEI(*m_SEIs);
      return false;

    case NAL_UNIT_CODED_SLICE:
    case NAL_UNIT_CODED_SLICE_IDR:
    case NAL_UNIT_CODED_SLICE_CDR:
    {
      // make sure we already received both parameter sets
      assert( 3 == m_uiValidPS );
      if (m_bFirstSliceInPicture)
      {
        m_apcSlicePilot->initSlice();
        m_uiSliceIdx     = 0;
        m_uiLastSliceIdx = 0;
      }
      m_apcSlicePilot->setSliceIdx(m_uiSliceIdx);

      //  Read slice header
      m_apcSlicePilot->setSPS( &m_cSPS );
      m_apcSlicePilot->setPPS( &m_cPPS );
      m_apcSlicePilot->setSliceIdx(m_uiSliceIdx);
      m_apcSlicePilot->setViewIdx( m_cSPS.getViewId() );
      if (!m_bFirstSliceInPicture)
      {
        memcpy(m_apcSlicePilot, pcPic->getPicSym()->getSlice(m_uiSliceIdx-1), sizeof(TComSlice));
      }

#if DCM_DECODING_REFRESH
      m_apcSlicePilot->setNalUnitType        (eNalUnitType);
#endif
      m_cEntropyDecoder.decodeSliceHeader (m_apcSlicePilot);

//      if (m_apcSlicePilot->isNextSlice() && m_apcSlicePilot->getPOC()!=m_uiPrevPOC && !m_bFirstSliceInSequence)
//      if (m_apcSlicePilot->isNextSlice() && ( m_apcSlicePilot->getPOC()!=m_uiPrevPOC || m_apcSlicePilot->getPPSId() != m_cPPS.getPPSId() ) && !m_bFirstSliceInSequence)
      if (m_apcSlicePilot->isNextSlice() && ( m_apcSlicePilot->getPOC()!=m_uiPrevPOC || m_apcSlicePilot->getPPSId() != m_cPPS.getPPSId() ) && !m_bFirstSliceInPicture)
      {
        m_uiPrevPOC = m_apcSlicePilot->getPOC();
        return true;
      }
      if (m_apcSlicePilot->isNextSlice())
        m_uiPrevPOC = m_apcSlicePilot->getPOC();
      m_bFirstSliceInSequence = false;
      if (m_apcSlicePilot->isNextSlice())
      {
#if DCM_SKIP_DECODING_FRAMES
        // Skip pictures due to random access
        if (isRandomAccessSkipPicture(iSkipFrame, iPOCLastDisplay))
        {
          return false;
        }
#endif
      }

      if (m_bFirstSliceInPicture)
      {
        // Buffer initialize for prediction.
        m_cPrediction.initTempBuff();
        //  Get a new picture buffer
        xGetNewPicBuffer (m_apcSlicePilot, pcPic);

        pcPic->setViewIdx( m_cSPS.getViewId() );

        /* transfer any SEI messages that have been received to the picture */
        pcPic->setSEIs(m_SEIs);
        m_SEIs = NULL;

        // Recursive structure
        m_cCuDecoder.create ( g_uiMaxCUDepth, g_uiMaxCUWidth, g_uiMaxCUHeight );
        m_cCuDecoder.init   ( &m_cEntropyDecoder, &m_cTrQuant, &m_cPrediction );
        m_cTrQuant.init     ( g_uiMaxCUWidth, g_uiMaxCUHeight, m_apcSlicePilot->getSPS()->getMaxTrSize());

        m_cSliceDecoder.create( m_apcSlicePilot, m_apcSlicePilot->getSPS()->getWidth(), m_apcSlicePilot->getSPS()->getHeight(), g_uiMaxCUWidth, g_uiMaxCUHeight, g_uiMaxCUDepth );

#if DEPTH_MAP_GENERATION
        m_cDepthMapGenerator.create( true, m_apcSlicePilot->getSPS()->getWidth(), m_apcSlicePilot->getSPS()->getHeight(), g_uiMaxCUDepth, g_uiMaxCUWidth, g_uiMaxCUHeight, g_uiBitDepth + g_uiBitIncrement );
#endif
#if HHI_INTER_VIEW_RESIDUAL_PRED
        m_cResidualGenerator.create( true, m_apcSlicePilot->getSPS()->getWidth(), m_apcSlicePilot->getSPS()->getHeight(), g_uiMaxCUDepth, g_uiMaxCUWidth, g_uiMaxCUHeight, g_uiBitDepth + g_uiBitIncrement );
#endif
      }

      //  Set picture slice pointer
      TComSlice*  pcSlice = m_apcSlicePilot;
      Bool bNextSlice     = pcSlice->isNextSlice();
      if (m_bFirstSliceInPicture)
      {
        if(pcPic->getNumAllocatedSlice() != 1)
        {
          pcPic->clearSliceBuffer();
        }
      }
      else
      {
        pcPic->allocateNewSlice();
      }
      assert(pcPic->getNumAllocatedSlice() == (m_uiSliceIdx + 1));
      m_apcSlicePilot = pcPic->getPicSym()->getSlice(m_uiSliceIdx);
      pcPic->getPicSym()->setSlice(pcSlice, m_uiSliceIdx);

#if POZNAN_MP
	  pcSlice->setMP(m_pcMP);
#endif

      if (bNextSlice)
      {
#if DCM_DECODING_REFRESH
        // Do decoding refresh marking if any
        pcSlice->decodingRefreshMarking(m_uiPOCCDR, m_bRefreshPending, m_cListPic);
#endif

        // Set reference list
        std::vector<TComPic*> apcSpatRefPics = getDecTop()->getSpatialRefPics( pcPic->getViewIdx(), pcSlice->getPOC(), m_cSPS.isDepth() );
        TComPic * const pcTexturePic = m_cSPS.isDepth() ? getDecTop()->getPicFromView( pcPic->getViewIdx(), pcSlice->getPOC(), false ) : NULL;
        assert( ! m_cSPS.isDepth() || pcTexturePic != NULL );
        pcSlice->setTexturePic( pcTexturePic );
        pcSlice->setViewIdx( pcPic->getViewIdx() );
        pcSlice->setRefPicListExplicitlyDecoderSided(m_cListPic, apcSpatRefPics) ;// temporary solution

#if DCM_COMB_LIST
        if(!pcSlice->getRefPicListModificationFlagLC())
        {
          pcSlice->generateCombinedList();
        }
#endif

        pcSlice->setNoBackPredFlag( false );
#if DCM_COMB_LIST
        if ( pcSlice->getSliceType() == B_SLICE && !pcSlice->getRefPicListCombinationFlag())
#else
          if ( pcSlice->getSliceType() == B_SLICE )
#endif
          {
            if ( pcSlice->getNumRefIdx(RefPicList( 0 ) ) == pcSlice->getNumRefIdx(RefPicList( 1 ) ) )
            {
              pcSlice->setNoBackPredFlag( true );
              int i;
              for ( i=0; i < pcSlice->getNumRefIdx(RefPicList( 1 ) ); i++ )
              {
                if ( pcSlice->getRefPOC(RefPicList(1), i) != pcSlice->getRefPOC(RefPicList(0), i) )
                {
                  pcSlice->setNoBackPredFlag( false );
                  break;
                }
              }
            }
          }
      }

      pcPic->setCurrSliceIdx(m_uiSliceIdx);

#if HHI_DMM_WEDGE_INTRA || HHI_DMM_PRED_TEX
    if ( m_cSPS.getUseDMM() && g_aacWedgeLists.empty() && m_bIsDepth )
      {
        initWedgeLists();
      }
#endif

#if POZNAN_CU_SKIP || POZNAN_CU_SYNTH
      if( m_pcCamParsCollector )
      {
        m_pcCamParsCollector->setSlice( pcSlice );
      }
      //if(!getIsDepth())
        getDecTop()->storeSynthPicsInBuffer(pcSlice->getViewIdx(),pcSlice->getSPS()->getViewOrderIdx(),pcSlice->getPOC(),getIsDepth());
#endif

#if POZNAN_MP 
	std::vector<TComPic*> apcSpatDataRefPics = getDecTop()->getSpatialRefPics( pcPic->getViewIdx(), pcSlice->getPOC(), m_cSPS.isDepth() );
	pcSlice->getMP()->setRefPicsList(&apcSpatDataRefPics);  
#if !POZNAN_MP_USE_DEPTH_MAP_GENERATION
	std::vector<TComPic*> apcSpatDepthRefPics = getDecTop()->getSpatialRefPics( pcPic->getViewIdx(), pcSlice->getPOC(), true );
	pcSlice->getMP()->setDepthRefPicsList(&apcSpatDepthRefPics);
#endif
#endif

      //  Decode a picture
      m_cGopDecoder.decompressGop ( bEos, pcBitstream, pcPic, false );

#if POZNAN_MP
	  pcSlice->getMP()->disable();
#endif

      if( m_pcCamParsCollector )
      {
        m_pcCamParsCollector->setSlice( pcSlice );
      }

      m_bFirstSliceInPicture = false;
      m_uiSliceIdx++;
    }
      break;
    default:
      assert (1);
  }

  return false;
}

#if DCM_SKIP_DECODING_FRAMES
/** Function for checking if picture should be skipped because of random access
 * \param iSkipFrame skip frame counter
 * \param iPOCLastDisplay POC of last picture displayed
 * \returns true if the picture shold be skipped in the random access.
 * This function checks the skipping of pictures in the case of -s option random access.
 * All pictures prior to the random access point indicated by the counter iSkipFrame are skipped.
 * It also checks the type of Nal unit type at the random access point.
 * If the random access point is CDR, pictures with POC equal to or greater than the CDR POC are decoded.
 * If the random access point is IDR all pictures after the random access point are decoded.
 * If the random access point is not IDR or CDR, a warning is issues, and decoding of pictures with POC
 * equal to or greater than the random access point POC is attempted. For non IDR/CDR random
 * access point there is no guarantee that the decoder will not crash.
 */
Bool TDecTop::isRandomAccessSkipPicture(Int& iSkipFrame,  Int& iPOCLastDisplay)
{
  if (iSkipFrame)
  {
    iSkipFrame--;   // decrement the counter
    return true;
  }
  else if (m_uiPOCRA == MAX_UINT) // start of random access point, m_uiPOCRA has not been set yet.
  {
    if (m_apcSlicePilot->getNalUnitType() == NAL_UNIT_CODED_SLICE_CDR)
    {
      m_uiPOCRA = m_apcSlicePilot->getPOC(); // set the POC random access since we need to skip the reordered pictures in CDR.
    }
    else if (m_apcSlicePilot->getNalUnitType() == NAL_UNIT_CODED_SLICE_IDR)
    {
      m_uiPOCRA = 0; // no need to skip the reordered pictures in IDR, they are decodable.
    }
    else
    {
      printf("\nUnsafe random access point. Decoder may crash.");
      m_uiPOCRA = m_apcSlicePilot->getPOC(); // set the POC random access skip the reordered pictures and try to decode if possible.  This increases the chances of avoiding a decoder crash.
      //m_uiPOCRA = 0;
    }
  }
  else if (m_apcSlicePilot->getPOC() < m_uiPOCRA)  // skip the reordered pictures if necessary
  {
    iPOCLastDisplay++;
    return true;
  }
  // if we reach here, then the picture is not skipped.
  return false;
}
#endif

