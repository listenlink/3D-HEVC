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

/** \file     TDecTop.cpp
    \brief    decoder class
*/

#include "NALread.h"
#include "../../App/TAppDecoder/TAppDecTop.h"
#include "TDecTop.h"

//! \ingroup TLibDecoder
//! \{


CamParsCollector::CamParsCollector()
: m_bInitialized( false )
{
  m_aaiCodedOffset         = new Int* [ MAX_VIEW_NUM ];
  m_aaiCodedScale          = new Int* [ MAX_VIEW_NUM ];
  m_aiViewOrderIndex       = new Int  [ MAX_VIEW_NUM ];
#if QC_MVHEVC_B0046
  m_aiViewId               = new Int  [ MAX_VIEW_NUM ];
#endif
  m_aiViewReceived         = new Int  [ MAX_VIEW_NUM ];
  for( UInt uiId = 0; uiId < MAX_VIEW_NUM; uiId++ )
  {
    m_aaiCodedOffset      [ uiId ] = new Int [ MAX_VIEW_NUM ];
    m_aaiCodedScale       [ uiId ] = new Int [ MAX_VIEW_NUM ];
  }

#if MERL_VSP_C0152
  xCreateLUTs( (UInt)MAX_VIEW_NUM, (UInt)MAX_VIEW_NUM, m_adBaseViewShiftLUT, m_aiBaseViewShiftLUT );
  m_iLog2Precision   = LOG2_DISP_PREC_LUT;
  m_uiBitDepthForLUT = 8; // fixed
#endif
}

CamParsCollector::~CamParsCollector()
{
  for( UInt uiId = 0; uiId < MAX_VIEW_NUM; uiId++ )
  {
    delete [] m_aaiCodedOffset      [ uiId ];
    delete [] m_aaiCodedScale       [ uiId ];
  }
  delete [] m_aaiCodedOffset;
  delete [] m_aaiCodedScale;
  delete [] m_aiViewOrderIndex;
  delete [] m_aiViewReceived;

#if MERL_VSP_C0152
  xDeleteArray( m_adBaseViewShiftLUT, MAX_VIEW_NUM, MAX_VIEW_NUM, 2 );
  xDeleteArray( m_aiBaseViewShiftLUT, MAX_VIEW_NUM, MAX_VIEW_NUM, 2 );
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

#if MERL_VSP_C0152
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
#endif // end MERL_VSP_C0152

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

  AOF( pcSlice->getSPS()->getViewId() < MAX_VIEW_NUM );
#if !FCO_FIX  // Under flexible coding order, depth may need the camera parameters
  if ( pcSlice->getSPS()->isDepth  () )
  {
    return;
  }
#endif
  Bool  bFirstAU          = ( pcSlice->getPOC()               == 0 );
  Bool  bFirstSliceInAU   = ( pcSlice->getPOC()               != Int ( m_iLastPOC ) );
  Bool  bFirstSliceInView = ( pcSlice->getSPS()->getViewId()  != UInt( m_iLastViewId ) || bFirstSliceInAU );
  AOT(  bFirstSliceInAU  &&   pcSlice->getSPS()->getViewId()  != 0 );
#if FCO_FIX
#else
  AOT( !bFirstSliceInAU  &&   pcSlice->getSPS()->getViewId()   < UInt( m_iLastViewId ) );
#endif
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
    ::memset( m_aiViewReceived, 0x00, MAX_VIEW_NUM * sizeof( Int ) );
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
#if MERL_VSP_C0152
        xInitLUTs( uiBaseId, uiViewId, m_aaiCodedScale[ uiBaseId ][ uiViewId ], m_aaiCodedOffset[ uiBaseId ][ uiViewId ], m_adBaseViewShiftLUT, m_aiBaseViewShiftLUT);
        xInitLUTs( uiViewId, uiBaseId, m_aaiCodedScale[ uiViewId ][ uiBaseId ], m_aaiCodedOffset[ uiViewId ][ uiBaseId ], m_adBaseViewShiftLUT, m_aiBaseViewShiftLUT);
#endif
      }
      else
      {
        m_aaiCodedScale [ uiBaseId ][ uiViewId ]  = pcSlice->getSPS()->getCodedScale    () [ uiBaseId ];
        m_aaiCodedOffset[ uiBaseId ][ uiViewId ]  = pcSlice->getSPS()->getCodedOffset   () [ uiBaseId ];
        m_aaiCodedScale [ uiViewId ][ uiBaseId ]  = pcSlice->getSPS()->getInvCodedScale () [ uiBaseId ];
        m_aaiCodedOffset[ uiViewId ][ uiBaseId ]  = pcSlice->getSPS()->getInvCodedOffset() [ uiBaseId ];
#if MERL_VSP_C0152
        xInitLUTs( uiBaseId, uiViewId, m_aaiCodedScale[ uiBaseId ][ uiViewId ], m_aaiCodedOffset[ uiBaseId ][ uiViewId ], m_adBaseViewShiftLUT, m_aiBaseViewShiftLUT );
        xInitLUTs( uiViewId, uiBaseId, m_aaiCodedScale[ uiViewId ][ uiBaseId ], m_aaiCodedOffset[ uiViewId ][ uiBaseId ], m_adBaseViewShiftLUT, m_aiBaseViewShiftLUT );
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
#if MERL_VSP_C0152
        xInitLUTs( uiBaseId, uiViewId, m_aaiCodedScale[ uiBaseId ][ uiViewId ], m_aaiCodedOffset[ uiBaseId ][ uiViewId ], m_adBaseViewShiftLUT, m_aiBaseViewShiftLUT );
        xInitLUTs( uiViewId, uiBaseId, m_aaiCodedScale[ uiViewId ][ uiBaseId ], m_aaiCodedOffset[ uiViewId ][ uiBaseId ], m_adBaseViewShiftLUT, m_aiBaseViewShiftLUT );
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
, m_tAppDecTop( NULL )
, m_nalUnitTypeBaseView( NAL_UNIT_INVALID )
{
  m_pcPic = 0;
  m_iGopSize      = 0;
  m_bGopSizeSet   = false;
  m_iMaxRefPicNum = 0;
  m_uiValidPS = 0;
#if ENC_DEC_TRACE
  if(!g_hTrace) g_hTrace = fopen( "TraceDec.txt", "wb" );
  g_bJustDoIt = g_bEncDecTraceDisable;
  g_nSymbolCounter = 0;
#endif
  m_bRefreshPending = 0;
  m_pocCRA = 0;
  m_pocRandomAccess = MAX_INT;          
  m_prevPOC                = MAX_INT;
  m_bFirstSliceInPicture    = true;
  m_bFirstSliceInSequence   = true;
  m_pcCamParsCollector = 0;
#if QC_MVHEVC_B0046
  m_bFirstNal                  = false;
#endif
}

TDecTop::~TDecTop()
{
#if ENC_DEC_TRACE
  if(g_hTrace) fclose( g_hTrace );
  g_hTrace=NULL;
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
  m_tAppDecTop = NULL;

#if DEPTH_MAP_GENERATION
  m_cDepthMapGenerator.destroy();
#endif
#if H3D_IVRP
  m_cResidualGenerator.destroy();
#endif

}

Void TDecTop::init( TAppDecTop* pcTAppDecTop, Bool bFirstInstance )
{
  // initialize ROM
  if( bFirstInstance )
  {
  initROM();
  }
  m_cGopDecoder.init( &m_cEntropyDecoder, &m_cSbacDecoder, &m_cBinCABAC, &m_cCavlcDecoder, &m_cSliceDecoder, &m_cLoopFilter, &m_cAdaptiveLoopFilter, &m_cSAO
#if DEPTH_MAP_GENERATION
                    , &m_cDepthMapGenerator
#endif
#if H3D_IVRP
                    , &m_cResidualGenerator
#endif
    );
  m_cSliceDecoder.init( &m_cEntropyDecoder, &m_cCuDecoder );
  m_cEntropyDecoder.init(&m_cPrediction);
  m_tAppDecTop = pcTAppDecTop;
#if DEPTH_MAP_GENERATION
#if VIDYO_VPS_INTEGRATION
  m_cDepthMapGenerator.init( &m_cPrediction, m_tAppDecTop->getVPSAccess(), m_tAppDecTop->getSPSAccess(), m_tAppDecTop->getAUPicAccess() );
#else
  m_cDepthMapGenerator.init( &m_cPrediction, m_tAppDecTop->getSPSAccess(), m_tAppDecTop->getAUPicAccess() );
#endif
#endif
#if H3D_IVRP
  m_cResidualGenerator.init( &m_cTrQuant, &m_cDepthMapGenerator );
#endif
}

Void TDecTop::deletePicBuffer ( )
{
  TComList<TComPic*>::iterator  iterPic   = m_cListPic.begin();
  Int iSize = Int( m_cListPic.size() );
  
  for (Int i = 0; i < iSize; i++ )
  {
    if( *iterPic )
    {
      TComPic* pcPic = *(iterPic++);
      pcPic->destroy();
    
      delete pcPic;
      pcPic = NULL;
    }
  }
  
  // destroy ALF temporary buffers
  m_cAdaptiveLoopFilter.destroy();

  m_cSAO.destroy();
  
  m_cLoopFilter.        destroy();
  
  // destroy ROM
  if(m_viewId == 0 && m_isDepth == false)
  {
    destroyROM();
  }
}

#if H3D_IVRP
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
  //AOF( pcPic );
  if ( pcPic )
  {
    pcPic->removeResidualBuffer   ();
  }
}
#endif


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
//  AOF( pcPic );
  if ( pcPic )
  {
    pcPic->compressMotion();
  }
}

Void TDecTop::xUpdateGopSize (TComSlice* pcSlice)
{
  if ( !pcSlice->isIntra() && !m_bGopSizeSet)
  {
    m_iGopSize    = pcSlice->getPOC();
    m_bGopSizeSet = true;
    
    m_cGopDecoder.setGopSize(m_iGopSize);
  }
}

Void TDecTop::xGetNewPicBuffer ( TComSlice* pcSlice, TComPic*& rpcPic )
{
  xUpdateGopSize(pcSlice);
  
  m_iMaxRefPicNum = pcSlice->getSPS()->getMaxDecPicBuffering(pcSlice->getTLayer())+pcSlice->getSPS()->getNumReorderPics(pcSlice->getTLayer()) + 1; // +1 to have space for the picture currently being decoded

#if DEPTH_MAP_GENERATION
  UInt uiPdm                  = ( pcSlice->getSPS()->getViewId() ? pcSlice->getSPS()->getPredDepthMapGeneration() : m_tAppDecTop->getSPSAccess()->getPdm() );
  Bool bNeedPrdDepthMapBuffer = ( !pcSlice->getSPS()->isDepth() && uiPdm > 0 );
#endif

  if (m_cListPic.size() < (UInt)m_iMaxRefPicNum)
  {
    rpcPic = new TComPic();
    
    rpcPic->create ( pcSlice->getSPS()->getPicWidthInLumaSamples(), pcSlice->getSPS()->getPicHeightInLumaSamples(), g_uiMaxCUWidth, g_uiMaxCUHeight, g_uiMaxCUDepth, true);
    
#if DEPTH_MAP_GENERATION
    if( bNeedPrdDepthMapBuffer )
    {
      rpcPic->addPrdDepthMapBuffer( PDM_SUB_SAMP_EXP_X(uiPdm), PDM_SUB_SAMP_EXP_Y(uiPdm) );
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
    if ( rpcPic->getReconMark() == false && rpcPic->getOutputMark() == false)
    {
      rpcPic->setOutputMark(false);
      bBufferIsAvailable = true;
      break;
    }

    if ( rpcPic->getSlice( 0 )->isReferenced() == false  && rpcPic->getOutputMark() == false)
    {
      rpcPic->setOutputMark(false);
      rpcPic->setReconMark( false );
      rpcPic->getPicYuvRec()->setBorderExtension( false );
      bBufferIsAvailable = true;
      break;
    }
  }
  
  if ( !bBufferIsAvailable )
  {
    //There is no room for this picture, either because of faulty encoder or dropped NAL. Extend the buffer.
    m_iMaxRefPicNum++;
    rpcPic = new TComPic();
    m_cListPic.pushBack( rpcPic );
  }
  rpcPic->destroy();  
  rpcPic->create ( pcSlice->getSPS()->getPicWidthInLumaSamples(), pcSlice->getSPS()->getPicHeightInLumaSamples(), g_uiMaxCUWidth, g_uiMaxCUHeight, g_uiMaxCUDepth, true);
#if DEPTH_MAP_GENERATION
  if( bNeedPrdDepthMapBuffer && !rpcPic->getPredDepthMap() )
  {
    rpcPic->addPrdDepthMapBuffer( PDM_SUB_SAMP_EXP_X(uiPdm), PDM_SUB_SAMP_EXP_Y(uiPdm) );
  }
#endif
}

Void TDecTop::executeDeblockAndAlf(UInt& ruiPOC, TComList<TComPic*>*& rpcListPic, Int& iSkipFrame, Int& iPOCLastDisplay)
{
  if (!m_pcPic)
  {
    /* nothing to deblock */
    return;
  }
  
  TComPic*&   pcPic         = m_pcPic;

  // Execute Deblock and ALF only + Cleanup

  m_cGopDecoder.decompressGop(NULL, pcPic, true);

  TComSlice::sortPicList( m_cListPic ); // sorting for application output
  ruiPOC              = pcPic->getSlice(m_uiSliceIdx-1)->getPOC();
  rpcListPic          = &m_cListPic;  
  m_cCuDecoder.destroy();        
  m_bFirstSliceInPicture  = true;

  return;
}

Void TDecTop::xCreateLostPicture(Int iLostPoc) 
{
  printf("\ninserting lost poc : %d\n",iLostPoc);
  TComSlice cFillSlice;
  cFillSlice.setSPS( m_parameterSetManagerDecoder.getFirstSPS() );
  cFillSlice.setPPS( m_parameterSetManagerDecoder.getFirstPPS() );
  cFillSlice.initSlice();
  cFillSlice.initTiles();
  TComPic *cFillPic;
  xGetNewPicBuffer(&cFillSlice,cFillPic);
  cFillPic->getSlice(0)->setSPS( m_parameterSetManagerDecoder.getFirstSPS() );
  cFillPic->getSlice(0)->setPPS( m_parameterSetManagerDecoder.getFirstPPS() );
  cFillPic->getSlice(0)->initSlice();
  cFillPic->getSlice(0)->initTiles();

  
  
  TComList<TComPic*>::iterator iterPic = m_cListPic.begin();
  Int closestPoc = 1000000;
  while ( iterPic != m_cListPic.end())
  {
    TComPic * rpcPic = *(iterPic++);
    if(abs(rpcPic->getPicSym()->getSlice(0)->getPOC() -iLostPoc)<closestPoc&&abs(rpcPic->getPicSym()->getSlice(0)->getPOC() -iLostPoc)!=0&&rpcPic->getPicSym()->getSlice(0)->getPOC()!=m_apcSlicePilot->getPOC())
    {
      closestPoc=abs(rpcPic->getPicSym()->getSlice(0)->getPOC() -iLostPoc);
    }
  }
  iterPic = m_cListPic.begin();
  while ( iterPic != m_cListPic.end())
  {
    TComPic *rpcPic = *(iterPic++);
    if(abs(rpcPic->getPicSym()->getSlice(0)->getPOC() -iLostPoc)==closestPoc&&rpcPic->getPicSym()->getSlice(0)->getPOC()!=m_apcSlicePilot->getPOC())
    {
      printf("copying picture %d to %d (%d)\n",rpcPic->getPicSym()->getSlice(0)->getPOC() ,iLostPoc,m_apcSlicePilot->getPOC());
      rpcPic->getPicYuvRec()->copyToPic(cFillPic->getPicYuvRec());
      break;
    }
  }
  cFillPic->setCurrSliceIdx(0);
  for(Int i=0; i<cFillPic->getNumCUsInFrame(); i++) 
  {
    cFillPic->getCU(i)->initCU(cFillPic,i);
  }
  cFillPic->getSlice(0)->setReferenced(true);
  cFillPic->getSlice(0)->setPOC(iLostPoc);
  cFillPic->setReconMark(true);
  cFillPic->setOutputMark(true);
  if(m_pocRandomAccess == MAX_INT)
  {
    m_pocRandomAccess = iLostPoc;
  }
}


Void TDecTop::xActivateParameterSets()
{
  m_parameterSetManagerDecoder.applyPrefetchedPS();

  TComPPS *pps = m_parameterSetManagerDecoder.getPPS(m_apcSlicePilot->getPPSId());
  assert (pps != 0);

  TComSPS *sps = m_parameterSetManagerDecoder.getSPS(pps->getSPSId());
  assert (sps != 0);
#if VIDYO_VPS_INTEGRATION
  TComVPS *vps = m_parameterSetManagerDecoder.getVPS(sps->getVPSId());
  assert (vps != 0);
#if !QC_REM_IDV_B0046
  if( m_apcSlicePilot->getNalUnitType() == NAL_UNIT_CODED_SLICE_IDR || m_apcSlicePilot->getNalUnitType() == NAL_UNIT_CODED_SLICE_CRA )
#else
  if( (m_apcSlicePilot->getNalUnitType() == NAL_UNIT_CODED_SLICE_IDR || m_apcSlicePilot->getNalUnitType() == NAL_UNIT_CODED_SLICE_CRA) && !sps->getViewId() )
#endif
    // VPS can only be activated on IDR or CRA...
    getTAppDecTop()->getVPSAccess()->setActiveVPSId( sps->getVPSId() );
#endif
  m_apcSlicePilot->setPPS(pps);
  m_apcSlicePilot->setSPS(sps);
#if QC_MVHEVC_B0046
  TComVPS *vps = m_parameterSetManagerDecoder.getVPS(sps->getVPSId());
#endif
#if VIDYO_VPS_INTEGRATION|QC_MVHEVC_B0046
  m_apcSlicePilot->setVPS(vps);
#endif
  pps->setSPS(sps);

  if(sps->getUseSAO() || sps->getUseALF()|| sps->getScalingListFlag() || sps->getUseDF())
  {
    m_apcSlicePilot->setAPS( m_parameterSetManagerDecoder.getAPS(m_apcSlicePilot->getAPSId())  );
  }
  pps->setMinCuDQPSize( sps->getMaxCUWidth() >> ( pps->getMaxCuDQPDepth()) );

  for (Int i = 0; i < sps->getMaxCUDepth() - 1; i++)
  {
    sps->setAMPAcc( i, sps->getUseAMP() );
  }

  for (Int i = sps->getMaxCUDepth() - 1; i < sps->getMaxCUDepth(); i++)
  {
    sps->setAMPAcc( i, 0 );
  }

  m_cSAO.create( sps->getPicWidthInLumaSamples(), sps->getPicHeightInLumaSamples(), g_uiMaxCUWidth, g_uiMaxCUHeight, g_uiMaxCUDepth );
  m_cLoopFilter.        create( g_uiMaxCUDepth );
}

Bool TDecTop::xDecodeSlice(InputNALUnit &nalu, Int &iSkipFrame, Int iPOCLastDisplay )
{
  TComPic*&   pcPic         = m_pcPic;
  m_apcSlicePilot->initSlice();

  //!!!KS: DIRTY HACK
  m_apcSlicePilot->setPPSId(0);
  m_apcSlicePilot->setPPS(m_parameterSetManagerDecoder.getPrefetchedPPS(0));
  m_apcSlicePilot->setSPS(m_parameterSetManagerDecoder.getPrefetchedSPS(0));
#if VIDYO_VPS_INTEGRATION|QC_MVHEVC_B0046
#if QC_MVHEVC_B0046
  m_apcSlicePilot->setIsDepth(false);
#endif
  m_apcSlicePilot->setVPS(m_parameterSetManagerDecoder.getPrefetchedVPS(0));
#endif
  m_apcSlicePilot->initTiles();
#if QC_MVHEVC_B0046
  m_apcSlicePilot->setViewId( nalu.m_layerId );
  m_apcSlicePilot->setViewId( nalu.m_layerId );
  m_apcSlicePilot->setViewOrderIdx(m_apcSlicePilot->getVPS()->getViewId(nalu.m_layerId), nalu.m_layerId);
  Int iNumDirectRef = m_apcSlicePilot->getVPS()->getNumDirectRefLayer(nalu.m_layerId);
  m_apcSlicePilot->getSPS()->setNumberOfUsableInterViewRefs(iNumDirectRef);
  for(Int iNumIvRef = 0; iNumIvRef < iNumDirectRef; iNumIvRef ++)
  {
    Int iDeltaLayerId = m_apcSlicePilot->getVPS()->getDirectRefLayerId( nalu.m_layerId, iNumIvRef);
    m_apcSlicePilot->getSPS()->setUsableInterViewRef(iNumIvRef, (iDeltaLayerId-nalu.m_layerId));
  }
#endif
  if (m_bFirstSliceInPicture)
  {
    m_uiSliceIdx     = 0;
    m_uiLastSliceIdx = 0;
  }
  m_apcSlicePilot->setSliceIdx(m_uiSliceIdx);
  if (!m_bFirstSliceInPicture)
  {
    m_apcSlicePilot->copySliceInfo( pcPic->getPicSym()->getSlice(m_uiSliceIdx-1) );
  }

  m_apcSlicePilot->setNalUnitType(nalu.m_nalUnitType);
  if( m_bFirstSliceInPicture )
  {
#if QC_MVHEVC_B0046
    if( nalu.m_layerId == 0 ) { m_nalUnitTypeBaseView = nalu.m_nalUnitType; }
    else                     { m_nalUnitTypeBaseView = m_tAppDecTop->getTDecTop( 0, 0 )->getNalUnitTypeBaseView(); }
#else
#if VIDYO_VPS_INTEGRATION
    if( m_apcSlicePilot->getVPS()->getViewId(nalu.m_layerId) == 0 ) { m_nalUnitTypeBaseView = nalu.m_nalUnitType; }
    else { m_nalUnitTypeBaseView = m_tAppDecTop->getTDecTop( 0, m_apcSlicePilot->getVPS()->getDepthFlag(nalu.m_layerId) )->getNalUnitTypeBaseView(); }
#else
    if( nalu.m_viewId == 0 ) { m_nalUnitTypeBaseView = nalu.m_nalUnitType; }
    else                     { m_nalUnitTypeBaseView = m_tAppDecTop->getTDecTop( 0, nalu.m_isDepth )->getNalUnitTypeBaseView(); }
#endif
#endif
    m_apcSlicePilot->setNalUnitTypeBaseViewMvc( m_nalUnitTypeBaseView );
  }

  m_apcSlicePilot->setReferenced(nalu.m_nalRefFlag);
  m_apcSlicePilot->setTLayerInfo(nalu.m_temporalId);

  // ALF CU parameters should be part of the slice header -> needs to be fixed 
#if MTK_DEPTH_MERGE_TEXTURE_CANDIDATE_C0137
  m_cEntropyDecoder.decodeSliceHeader (m_apcSlicePilot, &m_parameterSetManagerDecoder, m_cGopDecoder.getAlfCuCtrlParam(), m_cGopDecoder.getAlfParamSet(),m_apcSlicePilot->getVPS()->getDepthFlag(nalu.m_layerId));
#else
  m_cEntropyDecoder.decodeSliceHeader (m_apcSlicePilot, &m_parameterSetManagerDecoder, m_cGopDecoder.getAlfCuCtrlParam(), m_cGopDecoder.getAlfParamSet());
#endif
  // byte align
  {
    Int numBitsForByteAlignment = nalu.m_Bitstream->getNumBitsUntilByteAligned();
    if ( numBitsForByteAlignment > 0 )
    {
      UInt bitsForByteAlignment;
      nalu.m_Bitstream->read( numBitsForByteAlignment, bitsForByteAlignment );
      assert( bitsForByteAlignment == ( ( 1 << numBitsForByteAlignment ) - 1 ) );
    }
  }

  // exit when a new picture is found
  if (m_apcSlicePilot->isNextSlice() && m_apcSlicePilot->getPOC()!=m_prevPOC && !m_bFirstSliceInSequence)
  {
    if (m_prevPOC >= m_pocRandomAccess)
    {
      m_prevPOC = m_apcSlicePilot->getPOC();
      return true;
    }
    m_prevPOC = m_apcSlicePilot->getPOC();
  }
  // actual decoding starts here
  xActivateParameterSets();
  m_apcSlicePilot->initTiles();

  if (m_apcSlicePilot->isNextSlice()) 
  {
    m_prevPOC = m_apcSlicePilot->getPOC();
  }
  m_bFirstSliceInSequence = false;
  if (m_apcSlicePilot->isNextSlice())
  {
    // Skip pictures due to random access
    if (isRandomAccessSkipPicture(iSkipFrame, iPOCLastDisplay))
    {
      return false;
    }
  }
  //detect lost reference picture and insert copy of earlier frame.
  while(m_apcSlicePilot->checkThatAllRefPicsAreAvailable(m_cListPic, m_apcSlicePilot->getRPS(), true, m_pocRandomAccess) > 0)
  {
    xCreateLostPicture(m_apcSlicePilot->checkThatAllRefPicsAreAvailable(m_cListPic, m_apcSlicePilot->getRPS(), false)-1);
  }
  if (m_bFirstSliceInPicture)
  {
    // Buffer initialize for prediction.
    m_cPrediction.initTempBuff();
    m_apcSlicePilot->applyReferencePictureSet(m_cListPic, m_apcSlicePilot->getRPS());
    //  Get a new picture buffer
    xGetNewPicBuffer (m_apcSlicePilot, pcPic);

#if INTER_VIEW_VECTOR_SCALING_C0115
    pcPic->setViewOrderIdx( m_apcSlicePilot->getVPS()->getViewOrderIdx(nalu.m_layerId) );    // will be changed to view_id
#endif
    /* transfer any SEI messages that have been received to the picture */
    pcPic->setSEIs(m_SEIs);
    m_SEIs = NULL;

    // Recursive structure
    m_cCuDecoder.create ( g_uiMaxCUDepth, g_uiMaxCUWidth, g_uiMaxCUHeight );
    m_cCuDecoder.init   ( &m_cEntropyDecoder, &m_cTrQuant, &m_cPrediction );
    m_cTrQuant.init     ( g_uiMaxCUWidth, g_uiMaxCUHeight, m_apcSlicePilot->getSPS()->getMaxTrSize());

    m_cSliceDecoder.create( m_apcSlicePilot, m_apcSlicePilot->getSPS()->getPicWidthInLumaSamples(), m_apcSlicePilot->getSPS()->getPicHeightInLumaSamples(), g_uiMaxCUWidth, g_uiMaxCUHeight, g_uiMaxCUDepth );
#if DEPTH_MAP_GENERATION
    UInt uiPdm = ( m_apcSlicePilot->getSPS()->getViewId() ? m_apcSlicePilot->getSPS()->getPredDepthMapGeneration() : m_tAppDecTop->getSPSAccess()->getPdm() );
    m_cDepthMapGenerator.create( true, m_apcSlicePilot->getSPS()->getPicWidthInLumaSamples(), m_apcSlicePilot->getSPS()->getPicHeightInLumaSamples(), g_uiMaxCUDepth, g_uiMaxCUWidth, g_uiMaxCUHeight, g_uiBitDepth + g_uiBitIncrement, PDM_SUB_SAMP_EXP_X(uiPdm), PDM_SUB_SAMP_EXP_Y(uiPdm) );
    TComDepthMapGenerator* pcDMG0 = m_tAppDecTop->getDecTop0()->getDepthMapGenerator();
    if( m_apcSlicePilot->getSPS()->getViewId() == 1 && ( pcDMG0->getSubSampExpX() != PDM_SUB_SAMP_EXP_X(uiPdm) || pcDMG0->getSubSampExpY() != PDM_SUB_SAMP_EXP_Y(uiPdm) ) )
    {
      pcDMG0->create( true, m_apcSlicePilot->getSPS()->getPicWidthInLumaSamples(), m_apcSlicePilot->getSPS()->getPicHeightInLumaSamples(), g_uiMaxCUDepth, g_uiMaxCUWidth, g_uiMaxCUHeight, g_uiBitDepth + g_uiBitIncrement, PDM_SUB_SAMP_EXP_X(uiPdm), PDM_SUB_SAMP_EXP_Y(uiPdm) );
    }
#endif
#if H3D_IVRP
    m_cResidualGenerator.create( true, m_apcSlicePilot->getSPS()->getPicWidthInLumaSamples(), m_apcSlicePilot->getSPS()->getPicHeightInLumaSamples(), g_uiMaxCUDepth, g_uiMaxCUWidth, g_uiMaxCUHeight, g_uiBitDepth + g_uiBitIncrement );
#endif
  }

  //  Set picture slice pointer
  TComSlice*  pcSlice = m_apcSlicePilot;
  Bool bNextSlice     = pcSlice->isNextSlice();

  UInt uiCummulativeTileWidth;
  UInt uiCummulativeTileHeight;
  UInt i, j, p;


  if( pcSlice->getPPS()->getColumnRowInfoPresent() == 1 )
  {
    //set NumColumnsMins1 and NumRowsMinus1
    pcPic->getPicSym()->setNumColumnsMinus1( pcSlice->getPPS()->getNumColumnsMinus1() );
    pcPic->getPicSym()->setNumRowsMinus1( pcSlice->getPPS()->getNumRowsMinus1() );

    //create the TComTileArray
    pcPic->getPicSym()->xCreateTComTileArray();

    if( pcSlice->getPPS()->getUniformSpacingIdr() == 1)
    {
      //set the width for each tile
      for(j=0; j < pcPic->getPicSym()->getNumRowsMinus1()+1; j++)
      {
        for(p=0; p < pcPic->getPicSym()->getNumColumnsMinus1()+1; p++)
        {
          pcPic->getPicSym()->getTComTile( j * (pcPic->getPicSym()->getNumColumnsMinus1()+1) + p )->
            setTileWidth( (p+1)*pcPic->getPicSym()->getFrameWidthInCU()/(pcPic->getPicSym()->getNumColumnsMinus1()+1) 
            - (p*pcPic->getPicSym()->getFrameWidthInCU())/(pcPic->getPicSym()->getNumColumnsMinus1()+1) );
        }
      }

      //set the height for each tile
      for(j=0; j < pcPic->getPicSym()->getNumColumnsMinus1()+1; j++)
      {
        for(p=0; p < pcPic->getPicSym()->getNumRowsMinus1()+1; p++)
        {
          pcPic->getPicSym()->getTComTile( p * (pcPic->getPicSym()->getNumColumnsMinus1()+1) + j )->
            setTileHeight( (p+1)*pcPic->getPicSym()->getFrameHeightInCU()/(pcPic->getPicSym()->getNumRowsMinus1()+1) 
            - (p*pcPic->getPicSym()->getFrameHeightInCU())/(pcPic->getPicSym()->getNumRowsMinus1()+1) );   
        }
      }
    }
    else
    {
      //set the width for each tile
      for(j=0; j < pcSlice->getPPS()->getNumRowsMinus1()+1; j++)
      {
        uiCummulativeTileWidth = 0;
        for(i=0; i < pcSlice->getPPS()->getNumColumnsMinus1(); i++)
        {
          pcPic->getPicSym()->getTComTile(j * (pcSlice->getPPS()->getNumColumnsMinus1()+1) + i)->setTileWidth( pcSlice->getPPS()->getColumnWidth(i) );
          uiCummulativeTileWidth += pcSlice->getPPS()->getColumnWidth(i);
        }
        pcPic->getPicSym()->getTComTile(j * (pcSlice->getPPS()->getNumColumnsMinus1()+1) + i)->setTileWidth( pcPic->getPicSym()->getFrameWidthInCU()-uiCummulativeTileWidth );
      }

      //set the height for each tile
      for(j=0; j < pcSlice->getPPS()->getNumColumnsMinus1()+1; j++)
      {
        uiCummulativeTileHeight = 0;
        for(i=0; i < pcSlice->getPPS()->getNumRowsMinus1(); i++)
        { 
          pcPic->getPicSym()->getTComTile(i * (pcSlice->getPPS()->getNumColumnsMinus1()+1) + j)->setTileHeight( pcSlice->getPPS()->getRowHeight(i) );
          uiCummulativeTileHeight += pcSlice->getPPS()->getRowHeight(i);
        }
        pcPic->getPicSym()->getTComTile(i * (pcSlice->getPPS()->getNumColumnsMinus1()+1) + j)->setTileHeight( pcPic->getPicSym()->getFrameHeightInCU()-uiCummulativeTileHeight );
      }
    }
  }
  else
  {
    //set NumColumnsMins1 and NumRowsMinus1
    pcPic->getPicSym()->setNumColumnsMinus1( pcSlice->getSPS()->getNumColumnsMinus1() );
    pcPic->getPicSym()->setNumRowsMinus1( pcSlice->getSPS()->getNumRowsMinus1() );

    //create the TComTileArray
    pcPic->getPicSym()->xCreateTComTileArray();

    //automatically set the column and row boundary if UniformSpacingIdr = 1
    if( pcSlice->getSPS()->getUniformSpacingIdr() == 1 )
    {
      //set the width for each tile
      for(j=0; j < pcPic->getPicSym()->getNumRowsMinus1()+1; j++)
      {
        for(p=0; p < pcPic->getPicSym()->getNumColumnsMinus1()+1; p++)
        {
          pcPic->getPicSym()->getTComTile( j * (pcPic->getPicSym()->getNumColumnsMinus1()+1) + p )->
            setTileWidth( (p+1)*pcPic->getPicSym()->getFrameWidthInCU()/(pcPic->getPicSym()->getNumColumnsMinus1()+1) 
            - (p*pcPic->getPicSym()->getFrameWidthInCU())/(pcPic->getPicSym()->getNumColumnsMinus1()+1) );
        }
      }

      //set the height for each tile
      for(j=0; j < pcPic->getPicSym()->getNumColumnsMinus1()+1; j++)
      {
        for(p=0; p < pcPic->getPicSym()->getNumRowsMinus1()+1; p++)
        {
          pcPic->getPicSym()->getTComTile( p * (pcPic->getPicSym()->getNumColumnsMinus1()+1) + j )->
            setTileHeight( (p+1)*pcPic->getPicSym()->getFrameHeightInCU()/(pcPic->getPicSym()->getNumRowsMinus1()+1) 
            - (p*pcPic->getPicSym()->getFrameHeightInCU())/(pcPic->getPicSym()->getNumRowsMinus1()+1) );   
        }
      }
    }
    else
    {
      //set the width for each tile
      for(j=0; j < pcSlice->getSPS()->getNumRowsMinus1()+1; j++)
      {
        uiCummulativeTileWidth = 0;
        for(i=0; i < pcSlice->getSPS()->getNumColumnsMinus1(); i++)
        {
          pcPic->getPicSym()->getTComTile(j * (pcSlice->getSPS()->getNumColumnsMinus1()+1) + i)->setTileWidth( pcSlice->getSPS()->getColumnWidth(i) );
          uiCummulativeTileWidth += pcSlice->getSPS()->getColumnWidth(i);
        }
        pcPic->getPicSym()->getTComTile(j * (pcSlice->getSPS()->getNumColumnsMinus1()+1) + i)->setTileWidth( pcPic->getPicSym()->getFrameWidthInCU()-uiCummulativeTileWidth );
      }

      //set the height for each tile
      for(j=0; j < pcSlice->getSPS()->getNumColumnsMinus1()+1; j++)
      {
        uiCummulativeTileHeight = 0;
        for(i=0; i < pcSlice->getSPS()->getNumRowsMinus1(); i++)
        { 
          pcPic->getPicSym()->getTComTile(i * (pcSlice->getSPS()->getNumColumnsMinus1()+1) + j)->setTileHeight( pcSlice->getSPS()->getRowHeight(i) );
          uiCummulativeTileHeight += pcSlice->getSPS()->getRowHeight(i);
        }
        pcPic->getPicSym()->getTComTile(i * (pcSlice->getSPS()->getNumColumnsMinus1()+1) + j)->setTileHeight( pcPic->getPicSym()->getFrameHeightInCU()-uiCummulativeTileHeight );
      }
    }
  }

  pcPic->getPicSym()->xInitTiles();

  //generate the Coding Order Map and Inverse Coding Order Map
  UInt uiEncCUAddr;
  for(i=0, uiEncCUAddr=0; i<pcPic->getPicSym()->getNumberOfCUsInFrame(); i++, uiEncCUAddr = pcPic->getPicSym()->xCalculateNxtCUAddr(uiEncCUAddr))
  {
    pcPic->getPicSym()->setCUOrderMap(i, uiEncCUAddr);
    pcPic->getPicSym()->setInverseCUOrderMap(uiEncCUAddr, i);
  }
  pcPic->getPicSym()->setCUOrderMap(pcPic->getPicSym()->getNumberOfCUsInFrame(), pcPic->getPicSym()->getNumberOfCUsInFrame());
  pcPic->getPicSym()->setInverseCUOrderMap(pcPic->getPicSym()->getNumberOfCUsInFrame(), pcPic->getPicSym()->getNumberOfCUsInFrame());

  //convert the start and end CU addresses of the slice and entropy slice into encoding order
  pcSlice->setEntropySliceCurStartCUAddr( pcPic->getPicSym()->getPicSCUEncOrder(pcSlice->getEntropySliceCurStartCUAddr()) );
  pcSlice->setEntropySliceCurEndCUAddr( pcPic->getPicSym()->getPicSCUEncOrder(pcSlice->getEntropySliceCurEndCUAddr()) );
  if(pcSlice->isNextSlice())
  {
    pcSlice->setSliceCurStartCUAddr(pcPic->getPicSym()->getPicSCUEncOrder(pcSlice->getSliceCurStartCUAddr()));
    pcSlice->setSliceCurEndCUAddr(pcPic->getPicSym()->getPicSCUEncOrder(pcSlice->getSliceCurEndCUAddr()));
  }

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

  pcPic->setTLayer(nalu.m_temporalId);

  if (bNextSlice)
  {
    pcSlice->checkCRA(pcSlice->getRPS(), m_pocCRA, m_cListPic); 

    if ( !pcSlice->getPPS()->getEnableTMVPFlag() && pcPic->getTLayer() == 0 )
    {
      pcSlice->decodingMarkingForNoTMVP( m_cListPic, pcSlice->getPOC() );
    }

    // Set reference list 
#if !QC_MVHEVC_B0046
#if VIDYO_VPS_INTEGRATION
    pcSlice->setViewId( pcSlice->getVPS()->getViewId(nalu.m_layerId) );
    pcSlice->setIsDepth( pcSlice->getVPS()->getDepthFlag(nalu.m_layerId) );
#else
    pcSlice->setViewId(m_viewId);
    pcSlice->setIsDepth(m_isDepth);
#endif
#endif

#if INTER_VIEW_VECTOR_SCALING_C0115
#if VIDYO_VPS_INTEGRATION
    pcSlice->setViewOrderIdx( pcSlice->getVPS()->getViewOrderIdx(nalu.m_layerId) );        // will be changed to view_id
#else
    pcSlice->setViewOrderIdx( pcPic->getViewOrderIdx() );
#endif
#endif

#if INTER_VIEW_VECTOR_SCALING_C0115
    pcSlice->setIVScalingFlag( pcSlice->getVPS()->getIVScalingFlag());
#endif

    assert( m_tAppDecTop != NULL );
    TComPic * const pcTexturePic = m_isDepth ? m_tAppDecTop->getPicFromView(  m_viewId, pcSlice->getPOC(), false ) : NULL;

#if FLEX_CODING_ORDER_M23723
    if (pcTexturePic != NULL)
    {
      assert( !m_isDepth || pcTexturePic != NULL );
      pcSlice->setTexturePic( pcTexturePic );
    }
#else
    assert( !m_isDepth || pcTexturePic != NULL );
    pcSlice->setTexturePic( pcTexturePic );
#endif


    std::vector<TComPic*> apcInterViewRefPics = m_tAppDecTop->getInterViewRefPics( m_viewId, pcSlice->getPOC(), m_isDepth, pcSlice->getSPS() );
    pcSlice->setRefPicListMvc( m_cListPic, apcInterViewRefPics );

    // For generalized B
    // note: maybe not existed case (always L0 is copied to L1 if L1 is empty)
    if (pcSlice->isInterB() && pcSlice->getNumRefIdx(REF_PIC_LIST_1) == 0)
    {
      Int iNumRefIdx = pcSlice->getNumRefIdx(REF_PIC_LIST_0);
      pcSlice->setNumRefIdx        ( REF_PIC_LIST_1, iNumRefIdx );

      for (Int iRefIdx = 0; iRefIdx < iNumRefIdx; iRefIdx++)
      {
        pcSlice->setRefPic(pcSlice->getRefPic(REF_PIC_LIST_0, iRefIdx), REF_PIC_LIST_1, iRefIdx);
      }
    }
    if (pcSlice->isInterB())
    {
      Bool bLowDelay = true;
      Int  iCurrPOC  = pcSlice->getPOC();
      Int iRefIdx = 0;

      for (iRefIdx = 0; iRefIdx < pcSlice->getNumRefIdx(REF_PIC_LIST_0) && bLowDelay; iRefIdx++)
      {
        if ( pcSlice->getRefPic(REF_PIC_LIST_0, iRefIdx)->getPOC() > iCurrPOC )
        {
          bLowDelay = false;
        }
      }
      for (iRefIdx = 0; iRefIdx < pcSlice->getNumRefIdx(REF_PIC_LIST_1) && bLowDelay; iRefIdx++)
      {
        if ( pcSlice->getRefPic(REF_PIC_LIST_1, iRefIdx)->getPOC() > iCurrPOC )
        {
          bLowDelay = false;
        }
      }

      pcSlice->setCheckLDC(bLowDelay);            
    }

    //---------------
    pcSlice->setRefPOCnViewListsMvc();

    if(!pcSlice->getRefPicListModificationFlagLC())
    {
      pcSlice->generateCombinedList();
    }

#if !FIX_LGE_WP_FOR_3D_C0223
    if( pcSlice->getRefPicListCombinationFlag() && pcSlice->getPPS()->getWPBiPredIdc()==1 && pcSlice->getSliceType()==B_SLICE )
    {
      pcSlice->setWpParamforLC();
    }
#endif
    pcSlice->setNoBackPredFlag( false );
    if ( pcSlice->getSliceType() == B_SLICE && !pcSlice->getRefPicListCombinationFlag())
    {
      if ( pcSlice->getNumRefIdx(RefPicList( 0 ) ) == pcSlice->getNumRefIdx(RefPicList( 1 ) ) )
      {
        pcSlice->setNoBackPredFlag( true );
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
  if(pcSlice->getSPS()->getScalingListFlag())
  {
    if(pcSlice->getAPS()->getScalingListEnabled())
    {
      pcSlice->setScalingList ( pcSlice->getAPS()->getScalingList()  );
      if(pcSlice->getScalingList()->getScalingListPresentFlag())
      {
        pcSlice->setDefaultScalingList();
      }
      m_cTrQuant.setScalingListDec(pcSlice->getScalingList());
    }
    m_cTrQuant.setUseScalingList(true);
  }
  else
  {
    m_cTrQuant.setFlatScalingList();
    m_cTrQuant.setUseScalingList(false);
  }

#if HHI_DMM_WEDGE_INTRA || HHI_DMM_PRED_TEX
  if( m_parameterSetManagerDecoder.getPrefetchedSPS(0)->getUseDMM() && g_aacWedgeLists.empty() )
  {
    initWedgeLists();
  }
#endif

#if FCO_DVP_REFINE_C0132_C0170
  if(m_bFirstSliceInPicture)
  {
    pcPic->setDepthCoded(false);

    if(m_viewId != 0)
    {
      if( m_isDepth == 0)
      {
        TComPic * recDepthMapBuffer;
        recDepthMapBuffer = m_tAppDecTop->getPicFromView( m_viewId, pcSlice->getPOC(), true );
        pcPic->setRecDepthMap(recDepthMapBuffer);

        if(recDepthMapBuffer != NULL)
        {
          pcPic->setDepthCoded(true);
        }
      }
    }
  }
#endif


#if MERL_VSP_C0152 // set BW LUT 
  if( m_pcCamParsCollector ) // Initialize the LUT elements 
  {
    m_pcCamParsCollector->setSlice( pcSlice );
  }
  if( pcSlice->getViewId() !=0 )
  {
    TComPic* pcBaseTxtPic = m_tAppDecTop->getPicFromView(  0, pcSlice->getPOC(), false );  // get base view reconstructed texture
    TComPic* pcBaseDepthPic = m_tAppDecTop->getPicFromView(  0, pcSlice->getPOC(), true ); // get base view reconstructed depth
     pcSlice->setRefPicBaseTxt(pcBaseTxtPic);
     pcSlice->setRefPicBaseDepth(pcBaseDepthPic);
  }
  getTAppDecTop()->setBWVSPLUT( pcSlice, pcSlice->getViewId(),  pcSlice->getPOC() ); // get the LUT for backward warping
#endif

  //  Decode a picture
  m_cGopDecoder.decompressGop(nalu.m_Bitstream, pcPic, false);

#if QC_IV_AS_LT_B0046
  std::vector<TComPic*> apcInterViewRefPics = m_tAppDecTop->getInterViewRefPics( m_viewId, pcSlice->getPOC(), m_isDepth, pcSlice->getSPS() );
  for( Int k = 0; k < apcInterViewRefPics.size(); k++ )
  {
    TComPic*  pcPicIv = apcInterViewRefPics[k];
    pcPicIv->setIsLongTerm( 0 );
  }
#endif
  if( m_pcCamParsCollector )
  {
    m_pcCamParsCollector->setSlice( pcSlice );
  }

  m_bFirstSliceInPicture = false;
  m_uiSliceIdx++;

  return false;
}

#if VIDYO_VPS_INTEGRATION|QC_MVHEVC_B0046
Void TDecTop::xDecodeVPS()
{
  TComVPS* vps = new TComVPS();
  
  m_cEntropyDecoder.decodeVPS( vps );
  m_parameterSetManagerDecoder.storePrefetchedVPS(vps);  
#if !QC_MVHEVC_B0046
  getTAppDecTop()->getVPSAccess()->addVPS( vps );
#endif
}
#endif

Void TDecTop::xDecodeSPS()
{
  TComSPS* sps = new TComSPS();
  TComRPSList* rps = new TComRPSList();
  sps->setRPSList(rps);
#if HHI_MPI || H3D_QTL
  m_cEntropyDecoder.decodeSPS( sps, m_isDepth );
#else
  m_cEntropyDecoder.decodeSPS( sps );
#endif
  m_parameterSetManagerDecoder.storePrefetchedSPS(sps);
  m_cAdaptiveLoopFilter.create( sps->getPicWidthInLumaSamples(), sps->getPicHeightInLumaSamples(), g_uiMaxCUWidth, g_uiMaxCUHeight, g_uiMaxCUDepth );
}

Void TDecTop::xDecodePPS()
{
  TComPPS* pps = new TComPPS();
  m_cEntropyDecoder.decodePPS( pps, &m_parameterSetManagerDecoder );
  m_parameterSetManagerDecoder.storePrefetchedPPS( pps );

  //!!!KS: Activate parameter sets for parsing APS (unless dependency is resolved)
  m_apcSlicePilot->setPPSId(pps->getPPSId());
  xActivateParameterSets();
  m_apcSlicePilot->initTiles();
}

Void TDecTop::xDecodeAPS()
{
  TComAPS  *aps = new TComAPS();
  allocAPS (aps);
  decodeAPS(aps);
  m_parameterSetManagerDecoder.storePrefetchedAPS(aps);
}

Void TDecTop::xDecodeSEI()
{
  m_SEIs = new SEImessages;
  m_cEntropyDecoder.decodeSEI(*m_SEIs);
}

Bool TDecTop::decode(InputNALUnit& nalu, Int& iSkipFrame, Int& iPOCLastDisplay)
{
  // Initialize entropy decoder
  m_cEntropyDecoder.setEntropyDecoder (&m_cCavlcDecoder);
  m_cEntropyDecoder.setBitstream      (nalu.m_Bitstream);

  switch (nalu.m_nalUnitType)
  {
#if VIDYO_VPS_INTEGRATION|QC_MVHEVC_B0046
    case NAL_UNIT_VPS:
      xDecodeVPS();
      return false;
#endif
    case NAL_UNIT_SPS:
      xDecodeSPS();
      return false;

    case NAL_UNIT_PPS:
      xDecodePPS();
      return false;
    case NAL_UNIT_APS:
      xDecodeAPS();
      return false;

    case NAL_UNIT_SEI:
      xDecodeSEI();
      return false;

    case NAL_UNIT_CODED_SLICE:
    case NAL_UNIT_CODED_SLICE_IDR:
#if !QC_REM_IDV_B0046
    case NAL_UNIT_CODED_SLICE_IDV:
#endif
    case NAL_UNIT_CODED_SLICE_CRA:
    case NAL_UNIT_CODED_SLICE_TLA:
      return xDecodeSlice(nalu, iSkipFrame, iPOCLastDisplay);
      break;
    default:
      assert (1);
  }

  return false;
}

#if QC_MVHEVC_B0046
Void TDecTop::xCopyVPS( TComVPS* pVPSV0 )
{
  m_parameterSetManagerDecoder.storePrefetchedVPS(pVPSV0);  
}

Void TDecTop::xCopySPS( TComSPS* pSPSV0 )
{
  TComSPS* sps = new TComSPS();
  sps = pSPSV0;
  m_parameterSetManagerDecoder.storePrefetchedSPS(sps);
  m_cAdaptiveLoopFilter.create( sps->getPicWidthInLumaSamples(), sps->getPicHeightInLumaSamples(), g_uiMaxCUWidth, g_uiMaxCUHeight, g_uiMaxCUDepth );
}

Void TDecTop::xCopyPPS(TComPPS* pPPSV0 )
{
  m_parameterSetManagerDecoder.storePrefetchedPPS( pPPSV0 );

  //!!!KS: Activate parameter sets for parsing APS (unless dependency is resolved)
  m_apcSlicePilot->setPPSId(pPPSV0->getPPSId());
  xActivateParameterSets();
  m_apcSlicePilot->initTiles();
}
#endif
/** Function for checking if picture should be skipped because of random access
 * \param iSkipFrame skip frame counter
 * \param iPOCLastDisplay POC of last picture displayed
 * \returns true if the picture shold be skipped in the random access.
 * This function checks the skipping of pictures in the case of -s option random access.
 * All pictures prior to the random access point indicated by the counter iSkipFrame are skipped.
 * It also checks the type of Nal unit type at the random access point.
 * If the random access point is CRA, pictures with POC equal to or greater than the CRA POC are decoded.
 * If the random access point is IDR all pictures after the random access point are decoded.
 * If the random access point is not IDR or CRA, a warning is issues, and decoding of pictures with POC 
 * equal to or greater than the random access point POC is attempted. For non IDR/CRA random 
 * access point there is no guarantee that the decoder will not crash.
 */
Bool TDecTop::isRandomAccessSkipPicture(Int& iSkipFrame,  Int& iPOCLastDisplay)
{
  if (iSkipFrame) 
  {
    iSkipFrame--;   // decrement the counter
    return true;
  }
  else if (m_pocRandomAccess == MAX_INT) // start of random access point, m_pocRandomAccess has not been set yet.
  {
    if( m_apcSlicePilot->getNalUnitTypeBaseViewMvc() == NAL_UNIT_CODED_SLICE_CRA )
    {
      m_pocRandomAccess = m_apcSlicePilot->getPOC(); // set the POC random access since we need to skip the reordered pictures in CRA.
    }
    else if( m_apcSlicePilot->getNalUnitTypeBaseViewMvc() == NAL_UNIT_CODED_SLICE_IDR )
    {
      m_pocRandomAccess = 0; // no need to skip the reordered pictures in IDR, they are decodable.
    }
    else 
    {
      static bool warningMessage = false;
      if(!warningMessage)
      {
        printf("\nWarning: this is not a valid random access point and the data is discarded until the first CRA picture");
        warningMessage = true;
      }
      return true;
    }
  }
  else if (m_apcSlicePilot->getPOC() < m_pocRandomAccess)  // skip the reordered pictures if necessary
  {
    iPOCLastDisplay++;
    return true;
  }
  // if we reach here, then the picture is not skipped.
  return false; 
}

Void TDecTop::allocAPS (TComAPS* pAPS)
{
  // we don't know the SPS before it has been activated. These fields could exist
  // depending on the corresponding flags in the APS, but SAO/ALF allocation functions will
  // have to be moved for that
  pAPS->createScalingList();
  pAPS->createSaoParam();
  m_cSAO.allocSaoParam(pAPS->getSaoParam());
  pAPS->createAlfParam();
}

//! \}
