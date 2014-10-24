/* The copyright in this software is being made available under the BSD
 * License, included below. This software may be subject to other third party
 * and contributor rights, including patent rights, and no such rights are
 * granted under this license.  
 *
* Copyright (c) 2010-2014, ITU/ISO/IEC
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
#include "TDecTop.h"

#if H_MV
ParameterSetManagerDecoder TDecTop::m_parameterSetManagerDecoder;
#endif
//! \ingroup TLibDecoder
//! \{

#if H_3D
CamParsCollector::CamParsCollector()
: m_bInitialized( false )
{
  m_aaiCodedOffset         = new Int* [ MAX_NUM_LAYERS ];
  m_aaiCodedScale          = new Int* [ MAX_NUM_LAYERS ];
  for( UInt uiId = 0; uiId < MAX_NUM_LAYERS; uiId++ )
  {
    m_aaiCodedOffset      [ uiId ] = new Int [ MAX_NUM_LAYERS ];
    m_aaiCodedScale       [ uiId ] = new Int [ MAX_NUM_LAYERS ];
  }

  xCreateLUTs( (UInt)MAX_NUM_LAYERS, (UInt)MAX_NUM_LAYERS, m_adBaseViewShiftLUT, m_aiBaseViewShiftLUT );
  m_iLog2Precision   = LOG2_DISP_PREC_LUT;
  m_uiBitDepthForLUT = 8; // fixed
  m_receivedIdc = NULL; 
  m_vps         = NULL; 
}

CamParsCollector::~CamParsCollector()
{
  for( UInt uiId = 0; uiId < MAX_NUM_LAYERS; uiId++ )
  {
    delete [] m_aaiCodedOffset      [ uiId ];
    delete [] m_aaiCodedScale       [ uiId ];
  }
  delete [] m_aaiCodedOffset;
  delete [] m_aaiCodedScale;

  xDeleteArray( m_adBaseViewShiftLUT, MAX_NUM_LAYERS, MAX_NUM_LAYERS, 2 );
  xDeleteArray( m_aiBaseViewShiftLUT, MAX_NUM_LAYERS, MAX_NUM_LAYERS, 2 );
  xDeleteArray( m_receivedIdc, m_uiMaxViewIndex + 1 );
}


Void
CamParsCollector::init( FILE* pCodedScaleOffsetFile, TComVPS* vps)
{
  assert( !isInitialized() ); // Only one initialization currently supported
  m_bInitialized            = true;
  m_vps                     = vps; 
  m_bCamParsVaryOverTime    = false; 
  m_pCodedScaleOffsetFile   = pCodedScaleOffsetFile;
  m_lastPoc                 = -1;   
  m_firstReceivedPoc        = -2; 

  m_uiMaxViewIndex            = -1; 
  for (Int i = 0; i <= vps->getMaxLayersMinus1(); i++)
  {
    Int curViewIdx = m_vps->getViewIndex( m_vps->getLayerIdInNuh( i )); 
    m_bCamParsVaryOverTime = m_bCamParsVaryOverTime || vps->hasCamParInSliceHeader( curViewIdx );
    m_uiMaxViewIndex = std::max( m_uiMaxViewIndex, curViewIdx  ) ; 
  }

  assert( m_receivedIdc == NULL ); 
  m_receivedIdc = new Int*[ m_uiMaxViewIndex + 1]; 
  for (Int i = 0; i <= m_uiMaxViewIndex; i++)
  {
    m_receivedIdc[i] = new Int[ m_uiMaxViewIndex + 1 ]; 
  }

  xResetReceivedIdc( true ); 

  for (Int viewIndex = 0; viewIndex <= m_uiMaxViewIndex ; viewIndex++ )
  {
    if (m_vps->getCamParPresent( viewIndex ) )
    {    
      if( !m_vps->hasCamParInSliceHeader( viewIndex ) ) 
      {
        for (Int baseViewIndex = 0; baseViewIndex < viewIndex ; baseViewIndex++ )
        { 
          m_receivedIdc   [ baseViewIndex ][ viewIndex ] = -1; 
          m_aaiCodedScale [ baseViewIndex ][ viewIndex ] = m_vps->getCodedScale    (viewIndex) [ baseViewIndex ];
          m_aaiCodedOffset[ baseViewIndex ][ viewIndex ] = m_vps->getCodedOffset   (viewIndex) [ baseViewIndex ];

          m_receivedIdc   [ viewIndex ][ baseViewIndex ] = -1; 
          m_aaiCodedScale [ viewIndex ][ baseViewIndex ] = m_vps->getInvCodedScale (viewIndex) [ baseViewIndex ];
          m_aaiCodedOffset[ viewIndex ][ baseViewIndex ] = m_vps->getInvCodedOffset(viewIndex) [ baseViewIndex ];
          xInitLUTs( baseViewIndex, viewIndex, m_aaiCodedScale[ baseViewIndex ][ viewIndex ], m_aaiCodedOffset[ baseViewIndex ][ viewIndex ], m_adBaseViewShiftLUT, m_aiBaseViewShiftLUT );
          xInitLUTs( viewIndex, baseViewIndex, m_aaiCodedScale[ viewIndex ][ baseViewIndex ], m_aaiCodedOffset[ viewIndex ][ baseViewIndex ], m_adBaseViewShiftLUT, m_aiBaseViewShiftLUT );
        }
      }
    }
  }
}

Void
CamParsCollector::xResetReceivedIdc( Bool overWriteFlag )
{
  for (Int i = 0; i <= m_uiMaxViewIndex; i++)
  {  
    for (Int j = 0; j <= m_uiMaxViewIndex; j++)
    {
      if ( overWriteFlag ||  ( m_receivedIdc[i][j] != -1 ) )
      {
        m_receivedIdc[i][j] = 0; 
      }      
    }
  }
}


Void
CamParsCollector::xCreateLUTs( UInt uiNumberSourceViews, UInt uiNumberTargetViews, Double****& radLUT, Int****& raiLUT)
{

  uiNumberSourceViews = std::max( (UInt) 1, uiNumberSourceViews );
  uiNumberTargetViews = std::max( (UInt) 1, uiNumberTargetViews );

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
  Int     iLog2DivLuma   = m_uiBitDepthForLUT + m_vps->getCamParPrecision() + 1 - m_iLog2Precision;   AOF( iLog2DivLuma > 0 );
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
  }

  radLUT[ uiSourceView ][ uiTargetView ][ 0 ][ 256 ] = radLUT[ uiSourceView ][ uiTargetView ][ 0 ][ 255 ];
  radLUT[ uiSourceView ][ uiTargetView ][ 1 ][ 256 ] = radLUT[ uiSourceView ][ uiTargetView ][ 1 ][ 255 ];
  raiLUT[ uiSourceView ][ uiTargetView ][ 0 ][ 256 ] = raiLUT[ uiSourceView ][ uiTargetView ][ 0 ][ 255 ];
  raiLUT[ uiSourceView ][ uiTargetView ][ 1 ][ 256 ] = raiLUT[ uiSourceView ][ uiTargetView ][ 1 ][ 255 ];
}

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
    xOutput( m_lastPoc );
    return;
  }

#if !H_3D_FCO
  if ( pcSlice->getIsDepth())
  {
    return;
  }
#endif

  Int curPoc = pcSlice->getPOC();
  if( m_firstReceivedPoc == -2 )
  {
    m_firstReceivedPoc = curPoc; 
  }

  Bool newPocFlag = ( m_lastPoc != curPoc );  

  if ( newPocFlag )
  {    
    if( m_lastPoc != -1 )
    {
      xOutput( m_lastPoc );
    }

    xResetReceivedIdc( false ); 
    m_lastPoc = pcSlice->getPOC();
  }

  UInt uiViewIndex          = pcSlice->getViewIndex();  
  if( m_vps->getCamParPresent( uiViewIndex ) )
  {    
    if( m_vps->hasCamParInSliceHeader( uiViewIndex ) ) // check consistency of slice parameters here
    {   
      for( UInt uiBaseViewIndex = 0; uiBaseViewIndex < uiViewIndex; uiBaseViewIndex++ )
      {        
        if ( m_receivedIdc[ uiViewIndex ][ uiBaseViewIndex ] != 0 )
        {      
          AOF( m_aaiCodedScale [ uiViewIndex ][ uiBaseViewIndex ] == pcSlice->getInvCodedScale () [ uiBaseViewIndex ] );
          AOF( m_aaiCodedOffset[ uiViewIndex ][ uiBaseViewIndex ] == pcSlice->getInvCodedOffset() [ uiBaseViewIndex ] );
        }
        else
        {          
          m_receivedIdc   [ uiViewIndex ][ uiBaseViewIndex ]  = 1; 
          m_aaiCodedScale [ uiViewIndex ][ uiBaseViewIndex ]  = pcSlice->getInvCodedScale () [ uiBaseViewIndex ];
          m_aaiCodedOffset[ uiViewIndex ][ uiBaseViewIndex ]  = pcSlice->getInvCodedOffset() [ uiBaseViewIndex ];
          xInitLUTs( uiViewIndex, uiBaseViewIndex, m_aaiCodedScale[ uiViewIndex ][ uiBaseViewIndex ], m_aaiCodedOffset[ uiViewIndex ][ uiBaseViewIndex ], m_adBaseViewShiftLUT, m_aiBaseViewShiftLUT);
        }
        if ( m_receivedIdc[ uiBaseViewIndex ][ uiViewIndex ] != 0 )
        {      
          AOF( m_aaiCodedScale [ uiBaseViewIndex ][ uiViewIndex ] == pcSlice->getCodedScale    () [ uiBaseViewIndex ] );
          AOF( m_aaiCodedOffset[ uiBaseViewIndex ][ uiViewIndex ] == pcSlice->getCodedOffset   () [ uiBaseViewIndex ] );
        }
        else
        {        
          m_receivedIdc   [ uiBaseViewIndex ][ uiViewIndex ]  = 1; 
          m_aaiCodedScale [ uiBaseViewIndex ][ uiViewIndex ]  = pcSlice->getCodedScale    () [ uiBaseViewIndex ];
          m_aaiCodedOffset[ uiBaseViewIndex ][ uiViewIndex ]  = pcSlice->getCodedOffset   () [ uiBaseViewIndex ];
          xInitLUTs( uiBaseViewIndex, uiViewIndex, m_aaiCodedScale[ uiBaseViewIndex ][ uiViewIndex ], m_aaiCodedOffset[ uiBaseViewIndex ][ uiViewIndex ], m_adBaseViewShiftLUT, m_aiBaseViewShiftLUT);
        }
      }
    }
  }
}


#if H_3D_IV_MERGE
Void
CamParsCollector::copyCamParamForSlice( TComSlice* pcSlice )
{
  if( m_bCamParsVaryOverTime )
  {
    pcSlice->setCamparaSlice( m_aaiCodedScale, m_aaiCodedOffset );
  }
}
#endif


Void
CamParsCollector::xOutput( Int iPOC )
{
  if( m_pCodedScaleOffsetFile )
  {
    if( iPOC == m_firstReceivedPoc )
    {
      fprintf( m_pCodedScaleOffsetFile, "#  ViewIndex       ViewId\n" );
      fprintf( m_pCodedScaleOffsetFile, "#----------- ------------\n" );
      for( UInt uiViewIndex = 0; uiViewIndex <= m_uiMaxViewIndex; uiViewIndex++ )
      {
        fprintf( m_pCodedScaleOffsetFile, "%12d %12d\n", uiViewIndex, m_vps->getViewIdVal( uiViewIndex ) );
      }
      fprintf( m_pCodedScaleOffsetFile, "\n\n");
      fprintf( m_pCodedScaleOffsetFile, "# StartFrame     EndFrame   TargetView     BaseView   CodedScale  CodedOffset    Precision\n" );
      fprintf( m_pCodedScaleOffsetFile, "#----------- ------------ ------------ ------------ ------------ ------------ ------------\n" );
    }
    if( iPOC == m_firstReceivedPoc || m_bCamParsVaryOverTime  )
    {
      Int iS = iPOC;
      Int iE = ( m_bCamParsVaryOverTime ? iPOC : ~( 1 << 31 ) );
      for( UInt uiViewIndex = 0; uiViewIndex <= m_uiMaxViewIndex; uiViewIndex++ )
      {
        for( UInt uiBaseIndex = 0; uiBaseIndex <= m_uiMaxViewIndex; uiBaseIndex++ )
        {
          if( uiViewIndex != uiBaseIndex )
          {
            if ( m_receivedIdc[uiBaseIndex][uiViewIndex] != 0 )
            {            
              fprintf( m_pCodedScaleOffsetFile, "%12d %12d %12d %12d %12d %12d %12d\n",
                iS, iE, uiViewIndex, uiBaseIndex, m_aaiCodedScale[ uiBaseIndex ][ uiViewIndex ], m_aaiCodedOffset[ uiBaseIndex ][ uiViewIndex ], m_vps->getCamParPrecision() );
            }            
          }
        }
      }
    }
  }
}
#endif
TDecTop::TDecTop()
{
  m_pcPic = 0;
  m_iMaxRefPicNum = 0;
#if ENC_DEC_TRACE
#if H_MV
  if ( g_hTrace == NULL )
  {
#endif
  g_hTrace = fopen( "TraceDec.txt", "wb" );
  g_bJustDoIt = g_bEncDecTraceDisable;
  g_nSymbolCounter = 0;
#if H_MV
  }
#endif
#endif
  m_associatedIRAPType = NAL_UNIT_INVALID;
  m_pocCRA = 0;
  m_pocRandomAccess = MAX_INT;  
  m_prevPOC                = MAX_INT;
  m_bFirstSliceInPicture    = true;
  m_bFirstSliceInSequence   = true;
  m_prevSliceSkipped = false;
  m_skippedPOC = 0;
#if SETTING_NO_OUT_PIC_PRIOR
  m_bFirstSliceInBitstream  = true;
  m_lastPOCNoOutputPriorPics = -1;
  m_craNoRaslOutputFlag = false;
  m_isNoOutputPriorPics = false;
#endif
#if H_MV
  m_isLastNALWasEos = false;
  m_layerId = 0;
  m_viewId = 0;
#if H_3D
  m_viewIndex = 0; 
  m_isDepth = false;
  m_pcCamParsCollector = 0;
#endif
#if H_MV
  m_targetOptLayerSetIdx = -1; 
#endif
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
  m_uiSliceIdx = 0;
}

Void TDecTop::destroy()
{
  m_cGopDecoder.destroy();
  
  delete m_apcSlicePilot;
  m_apcSlicePilot = NULL;
  
  m_cSliceDecoder.destroy();
}

Void TDecTop::init()
{
  // initialize ROM
#if !H_MV
  initROM();
#endif
  m_cGopDecoder.init( &m_cEntropyDecoder, &m_cSbacDecoder, &m_cBinCABAC, &m_cCavlcDecoder, &m_cSliceDecoder, &m_cLoopFilter, &m_cSAO );
  m_cSliceDecoder.init( &m_cEntropyDecoder, &m_cCuDecoder );
  m_cEntropyDecoder.init(&m_cPrediction);
}

Void TDecTop::deletePicBuffer ( )
{
  TComList<TComPic*>::iterator  iterPic   = m_cListPic.begin();
  Int iSize = Int( m_cListPic.size() );
  
  for (Int i = 0; i < iSize; i++ )
  {
    TComPic* pcPic = *(iterPic++);
#if H_MV
    if( pcPic )
    {
#endif
    pcPic->destroy();
    
    delete pcPic;
    pcPic = NULL;
#if H_MV
    }
#endif
  }
  
  m_cSAO.destroy();
  
  m_cLoopFilter.        destroy();
  
#if !H_MV
  // destroy ROM
  destroyROM();
#endif
}

Void TDecTop::xGetNewPicBuffer ( TComSlice* pcSlice, TComPic*& rpcPic )
{
  Int  numReorderPics[MAX_TLAYER];
  Window &conformanceWindow = pcSlice->getSPS()->getConformanceWindow();
  Window defaultDisplayWindow = pcSlice->getSPS()->getVuiParametersPresentFlag() ? pcSlice->getSPS()->getVuiParameters()->getDefaultDisplayWindow() : Window();

#if H_MV
    assert( conformanceWindow   .getScaledFlag() ); 
    assert( defaultDisplayWindow.getScaledFlag() );
#endif
  for( Int temporalLayer=0; temporalLayer < MAX_TLAYER; temporalLayer++) 
  {
#if H_MV
    numReorderPics[temporalLayer] = ( getLayerId() == 0 ) ? pcSlice->getSPS()->getNumReorderPics(temporalLayer) : pcSlice->getVPS()->getNumReorderPics(temporalLayer);
#else
    numReorderPics[temporalLayer] = pcSlice->getSPS()->getNumReorderPics(temporalLayer);
#endif
  }
#if H_MV
  if ( getLayerId() == 0 )
  {  
    m_iMaxRefPicNum = pcSlice->getSPS()->getMaxDecPicBuffering(pcSlice->getTLayer());
  }
  else
  {
    m_iMaxRefPicNum = pcSlice->getVPS()->getMaxDecPicBuffering(pcSlice->getTLayer());
#if H_MV_HLS7_GEN
    TComVPS* vps         = pcSlice->getVPS();
    TComDpbSize* dpbSize = vps->getDpbSize(); 
    Int lsIdx            = vps->olsIdxToLsIdx( getTargetOutputLayerSetIdx()); // Is this correct, seems to be missing in spec?
    Int layerIdx         = vps->getIdxInLayerSet     ( lsIdx, getLayerId() ); 
    Int subDpbIdx        = dpbSize->getSubDpbAssigned( lsIdx, layerIdx ); 
    m_iMaxRefPicNum      = dpbSize->getMaxVpsDecPicBufferingMinus1(getTargetOutputLayerSetIdx(), subDpbIdx , vps->getSubLayersVpsMaxMinus1( vps->getLayerIdInVps( getLayerId() ) ) + 1 ) + 1 ;  
#endif    
  }
#else
  m_iMaxRefPicNum = pcSlice->getSPS()->getMaxDecPicBuffering(pcSlice->getTLayer());     // m_uiMaxDecPicBuffering has the space for the picture currently being decoded
#endif
  if (m_cListPic.size() < (UInt)m_iMaxRefPicNum)
  {
    rpcPic = new TComPic();
    
    rpcPic->create ( pcSlice->getSPS()->getPicWidthInLumaSamples(), pcSlice->getSPS()->getPicHeightInLumaSamples(), g_uiMaxCUWidth, g_uiMaxCUHeight, g_uiMaxCUDepth, 
                     conformanceWindow, defaultDisplayWindow, numReorderPics, true);
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
#if H_MV
      rpcPic->setPicOutputFlag(false); 
#endif
      bBufferIsAvailable = true;
      break;
    }

    if ( rpcPic->getSlice( 0 )->isReferenced() == false  && rpcPic->getOutputMark() == false)
    {
      rpcPic->setOutputMark(false);
#if H_MV
      rpcPic->setPicOutputFlag(false); 
#endif
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
  rpcPic->create ( pcSlice->getSPS()->getPicWidthInLumaSamples(), pcSlice->getSPS()->getPicHeightInLumaSamples(), g_uiMaxCUWidth, g_uiMaxCUHeight, g_uiMaxCUDepth,
                   conformanceWindow, defaultDisplayWindow, numReorderPics, true);
}

#if H_MV
Void TDecTop::endPicDecoding(Int& poc, TComList<TComPic*>*& rpcListPic, std::vector<Int>& targetDecLayerIdSet )
#else
Void TDecTop::executeLoopFilters(Int& poc, TComList<TComPic*>*& rpcListPic)
#endif
{
  if (!m_pcPic)
  {
    /* nothing to deblock */
    return;
  }
  
  TComPic*&   pcPic         = m_pcPic;

  // Execute Deblock + Cleanup

  m_cGopDecoder.filterPicture(pcPic);

  TComSlice::sortPicList( m_cListPic ); // sorting for application output
  poc                 = pcPic->getSlice(m_uiSliceIdx-1)->getPOC();
  rpcListPic          = &m_cListPic;  
  m_cCuDecoder.destroy();        
#if H_MV 
  TComSlice::markIvRefPicsAsShortTerm( m_refPicSetInterLayer0, m_refPicSetInterLayer1 );  
  TComSlice::markCurrPic( pcPic ); 
#endif
  m_bFirstSliceInPicture  = true;

  return;
}

#if SETTING_NO_OUT_PIC_PRIOR
Void TDecTop::checkNoOutputPriorPics (TComList<TComPic*>*& rpcListPic)
{
  if (!rpcListPic || !m_isNoOutputPriorPics) return;

  TComList<TComPic*>::iterator  iterPic   = rpcListPic->begin();

  while (iterPic != rpcListPic->end())
  {
    TComPic*& pcPicTmp = *(iterPic++);
    if (m_lastPOCNoOutputPriorPics != pcPicTmp->getPOC())
    {
      pcPicTmp->setOutputMark(false);
#if H_MV
      pcPicTmp->setPicOutputFlag(false); 
#endif
    }
  }
}
#endif

Void TDecTop::xCreateLostPicture(Int iLostPoc) 
{
  printf("\ninserting lost poc : %d\n",iLostPoc);
  TComSlice cFillSlice;
  cFillSlice.setSPS( m_parameterSetManagerDecoder.getFirstSPS() );
  cFillSlice.setPPS( m_parameterSetManagerDecoder.getFirstPPS() );
  cFillSlice.initSlice();
  TComPic *cFillPic;
  xGetNewPicBuffer(&cFillSlice,cFillPic);
  cFillPic->getSlice(0)->setSPS( m_parameterSetManagerDecoder.getFirstSPS() );
  cFillPic->getSlice(0)->setPPS( m_parameterSetManagerDecoder.getFirstPPS() );
  cFillPic->getSlice(0)->initSlice();
  
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

#if H_MV
  TComVPS* vps = m_parameterSetManagerDecoder.getVPS(sps->getVPSId());
  assert (vps != 0); 
  if (!m_parameterSetManagerDecoder.activatePPS(m_apcSlicePilot->getPPSId(),m_apcSlicePilot->isIRAP(), m_layerId ) )
#else
  if (false == m_parameterSetManagerDecoder.activatePPS(m_apcSlicePilot->getPPSId(),m_apcSlicePilot->isIRAP()))
#endif
  {
    printf ("Parameter set activation failed!");
    assert (0);
  }

#if H_MV
  sps->inferSpsMaxDecPicBufferingMinus1( vps, m_targetOptLayerSetIdx, getLayerId(), false ); 
  // When the value of vps_num_rep_formats_minus1 in the active VPS is equal to 0
  if ( vps->getVpsNumRepFormatsMinus1() == 0 )
  {
    //, it is a requirement of bitstream conformance that the value of update_rep_format_flag shall be equal to 0.
    assert( sps->getUpdateRepFormatFlag() == false ); 
  }
  sps->checkRpsMaxNumPics( vps, getLayerId() ); 

  if( sps->getLayerId() != 0 )
  {
    sps->inferSpsMaxSubLayersMinus1( true, vps ); 
  }

  // It is a requirement of bitstream conformance that, when the SPS is referred to by 
  // any current picture that belongs to an independent non-base layer, the value of 
  // MultiLayerExtSpsFlag derived from the SPS shall be equal to 0.

  if ( m_layerId > 0 && vps->getNumRefLayers( m_layerId ) == 0 )
  {  
    assert( sps->getMultiLayerExtSpsFlag() == 0 ); 
  }

  if( sps->getMultiLayerExtSpsFlag() )
  {
    sps->setTemporalIdNestingFlag( (sps->getMaxTLayers() > 1) ? vps->getTemporalNestingFlag() : true );
  }
#endif

  if( pps->getDependentSliceSegmentsEnabledFlag() )
  {
    Int NumCtx = pps->getEntropyCodingSyncEnabledFlag()?2:1;

    if (m_cSliceDecoder.getCtxMemSize() != NumCtx)
    {
      m_cSliceDecoder.initCtxMem(NumCtx);
      for ( UInt st = 0; st < NumCtx; st++ )
      {
        TDecSbac* ctx = NULL;
        ctx = new TDecSbac;
        ctx->init( &m_cBinCABAC );
        m_cSliceDecoder.setCtxMem( ctx, st );
      }
    }
  }

  m_apcSlicePilot->setPPS(pps);
  m_apcSlicePilot->setSPS(sps);
#if H_MV
  m_apcSlicePilot->setVPS(vps);  
  // The nuh_layer_id value of the NAL unit containing the PPS that is activated for a layer layerA with nuh_layer_id equal to nuhLayerIdA shall be equal to 0, or nuhLayerIdA, or the nuh_layer_id of a direct or indirect reference layer of layerA.
  assert( pps->getLayerId() == m_layerId || pps->getLayerId( ) == 0 || vps->getDependencyFlag( m_layerId, pps->getLayerId() ) );   
  // The nuh_layer_id value of the NAL unit containing the SPS that is activated for a layer layerA with nuh_layer_id equal to nuhLayerIdA shall be equal to 0, or nuhLayerIdA, or the nuh_layer_id of a direct or indirect reference layer of layerA.
  assert( sps->getLayerId() == m_layerId || sps->getLayerId( ) == 0 || vps->getDependencyFlag( m_layerId, sps->getLayerId() ) );
  sps->inferRepFormat  ( vps , m_layerId ); 
  sps->inferScalingList( m_parameterSetManagerDecoder.getActiveSPS( sps->getSpsScalingListRefLayerId() ) ); 

#endif
  pps->setSPS(sps);
  pps->setNumSubstreams(pps->getEntropyCodingSyncEnabledFlag() ? ((sps->getPicHeightInLumaSamples() + sps->getMaxCUHeight() - 1) / sps->getMaxCUHeight()) * (pps->getNumTileColumnsMinus1() + 1) : 1);
  pps->setMinCuDQPSize( sps->getMaxCUWidth() >> ( pps->getMaxCuDQPDepth()) );

  g_bitDepthY     = sps->getBitDepthY();
  g_bitDepthC     = sps->getBitDepthC();
  g_uiMaxCUWidth  = sps->getMaxCUWidth();
  g_uiMaxCUHeight = sps->getMaxCUHeight();
  g_uiMaxCUDepth  = sps->getMaxCUDepth();
  g_uiAddCUDepth  = max (0, sps->getLog2MinCodingBlockSize() - (Int)sps->getQuadtreeTULog2MinSize() );

  for (Int i = 0; i < sps->getLog2DiffMaxMinCodingBlockSize(); i++)
  {
    sps->setAMPAcc( i, sps->getUseAMP() );
  }

  for (Int i = sps->getLog2DiffMaxMinCodingBlockSize(); i < sps->getMaxCUDepth(); i++)
  {
    sps->setAMPAcc( i, 0 );
  }

  m_cSAO.destroy();
  m_cSAO.create( sps->getPicWidthInLumaSamples(), sps->getPicHeightInLumaSamples(), sps->getMaxCUWidth(), sps->getMaxCUHeight(), sps->getMaxCUDepth() );
  m_cLoopFilter.create( sps->getMaxCUDepth() );
}

#if H_MV
Bool TDecTop::xDecodeSlice(InputNALUnit &nalu, Int &iSkipFrame, Int iPOCLastDisplay, Bool newLayerFlag, Bool& sliceSkippedFlag  )
{
  assert( nalu.m_layerId == m_layerId ); 
#else
Bool TDecTop::xDecodeSlice(InputNALUnit &nalu, Int &iSkipFrame, Int iPOCLastDisplay )
{
#endif
  TComPic*&   pcPic         = m_pcPic;
  m_apcSlicePilot->initSlice();

  if (m_bFirstSliceInPicture)
  {
    m_uiSliceIdx     = 0;
  }
  else
  {
    m_apcSlicePilot->copySliceInfo( pcPic->getPicSym()->getSlice(m_uiSliceIdx-1) );
  }
  m_apcSlicePilot->setSliceIdx(m_uiSliceIdx);

  m_apcSlicePilot->setNalUnitType(nalu.m_nalUnitType);
  Bool nonReferenceFlag = (m_apcSlicePilot->getNalUnitType() == NAL_UNIT_CODED_SLICE_TRAIL_N ||
                           m_apcSlicePilot->getNalUnitType() == NAL_UNIT_CODED_SLICE_TSA_N   ||
                           m_apcSlicePilot->getNalUnitType() == NAL_UNIT_CODED_SLICE_STSA_N  ||
                           m_apcSlicePilot->getNalUnitType() == NAL_UNIT_CODED_SLICE_RADL_N  ||
                           m_apcSlicePilot->getNalUnitType() == NAL_UNIT_CODED_SLICE_RASL_N);
  m_apcSlicePilot->setTemporalLayerNonReferenceFlag(nonReferenceFlag);
  
  m_apcSlicePilot->setReferenced(true); // Putting this as true ensures that picture is referenced the first time it is in an RPS
  m_apcSlicePilot->setTLayerInfo(nalu.m_temporalId);

#if H_MV
  m_apcSlicePilot->setRefPicSetInterLayer( & m_refPicSetInterLayer0, &m_refPicSetInterLayer1 ); 
  m_apcSlicePilot->setLayerId( nalu.m_layerId );
  m_cEntropyDecoder.decodeSliceHeader (m_apcSlicePilot, &m_parameterSetManagerDecoder, m_targetOptLayerSetIdx );
#else
  m_cEntropyDecoder.decodeSliceHeader (m_apcSlicePilot, &m_parameterSetManagerDecoder);
#endif
  // set POC for dependent slices in skipped pictures
  if(m_apcSlicePilot->getDependentSliceSegmentFlag() && m_prevSliceSkipped) 
  {
    m_apcSlicePilot->setPOC(m_skippedPOC);
  }

  m_apcSlicePilot->setAssociatedIRAPPOC(m_pocCRA);
  m_apcSlicePilot->setAssociatedIRAPType(m_associatedIRAPType);

#if H_MV  
  TComVPS* vps     = m_apcSlicePilot->getVPS();
  Int layerId  = nalu.m_layerId;   
  setViewId   ( vps->getViewId   ( layerId )      );  
#if H_3D
  setViewIndex( vps->getViewIndex( layerId )      );  
  setIsDepth  ( vps->getDepthId  ( layerId ) == 1 );  
  m_ivPicLists->setVPS( vps ); 
#endif
#endif

#if SETTING_NO_OUT_PIC_PRIOR
  //For inference of NoOutputOfPriorPicsFlag
  if (m_apcSlicePilot->getRapPicFlag())
  {
    if ((m_apcSlicePilot->getNalUnitType() >= NAL_UNIT_CODED_SLICE_BLA_W_LP && m_apcSlicePilot->getNalUnitType() <= NAL_UNIT_CODED_SLICE_IDR_N_LP) || 
        (m_apcSlicePilot->getNalUnitType() == NAL_UNIT_CODED_SLICE_CRA && m_bFirstSliceInSequence) ||
        (m_apcSlicePilot->getNalUnitType() == NAL_UNIT_CODED_SLICE_CRA && m_apcSlicePilot->getHandleCraAsBlaFlag()))
    {
      m_apcSlicePilot->setNoRaslOutputFlag(true);
    }
    //the inference for NoOutputPriorPicsFlag
    if (!m_bFirstSliceInBitstream && m_apcSlicePilot->getRapPicFlag() && m_apcSlicePilot->getNoRaslOutputFlag())
    {
      if (m_apcSlicePilot->getNalUnitType() == NAL_UNIT_CODED_SLICE_CRA)
      {
        m_apcSlicePilot->setNoOutputPriorPicsFlag(true);
      }
    }
    else
    {
      m_apcSlicePilot->setNoOutputPriorPicsFlag(false);
    }

    if(m_apcSlicePilot->getNalUnitType() == NAL_UNIT_CODED_SLICE_CRA)
    {
      m_craNoRaslOutputFlag = m_apcSlicePilot->getNoRaslOutputFlag();
    }
  }
  if (m_apcSlicePilot->getRapPicFlag() && m_apcSlicePilot->getNoOutputPriorPicsFlag())
  {
    m_lastPOCNoOutputPriorPics = m_apcSlicePilot->getPOC();
    m_isNoOutputPriorPics = true;
  }
  else
  {
    m_isNoOutputPriorPics = false;
  }

  //For inference of PicOutputFlag
  if (m_apcSlicePilot->getNalUnitType() == NAL_UNIT_CODED_SLICE_RASL_N || m_apcSlicePilot->getNalUnitType() == NAL_UNIT_CODED_SLICE_RASL_R)
  {
    if ( m_craNoRaslOutputFlag )
    {
      m_apcSlicePilot->setPicOutputFlag(false);
    }
  }
#endif

#if FIX_POC_CRA_NORASL_OUTPUT
  if (m_apcSlicePilot->getNalUnitType() == NAL_UNIT_CODED_SLICE_CRA && m_craNoRaslOutputFlag) //Reset POC MSB when CRA has NoRaslOutputFlag equal to 1
  {
    Int iMaxPOClsb = 1 << m_apcSlicePilot->getSPS()->getBitsForPOC();
    m_apcSlicePilot->setPOC( m_apcSlicePilot->getPOC() & (iMaxPOClsb - 1) );
  }
#endif
#if H_MV
    xCeckNoClrasOutput();
#endif
    // Skip pictures due to random access
    if (isRandomAccessSkipPicture(iSkipFrame, iPOCLastDisplay))
    {
    m_prevSliceSkipped = true;
    m_skippedPOC = m_apcSlicePilot->getPOC();
#if H_MV
    sliceSkippedFlag = true; 
#endif
      return false;
    }
    // Skip TFD pictures associated with BLA/BLANT pictures
    if (isSkipPictureForBLA(iPOCLastDisplay))
    {
    m_prevSliceSkipped = true;
    m_skippedPOC = m_apcSlicePilot->getPOC();
#if H_MV
    sliceSkippedFlag = true; 
#endif
      return false;
    }

  // clear previous slice skipped flag
  m_prevSliceSkipped = false;

  //we should only get a different poc for a new picture (with CTU address==0)
  if (m_apcSlicePilot->isNextSlice() && m_apcSlicePilot->getPOC()!=m_prevPOC && !m_bFirstSliceInSequence && (m_apcSlicePilot->getSliceCurStartCUAddr()!=0))
  {
    printf ("Warning, the first slice of a picture might have been lost!\n");
  }
  // exit when a new picture is found
  if (m_apcSlicePilot->isNextSlice() && (m_apcSlicePilot->getSliceCurStartCUAddr() == 0 && !m_bFirstSliceInPicture) && !m_bFirstSliceInSequence )
  {
    if (m_prevPOC >= m_pocRandomAccess)
    {
      m_prevPOC = m_apcSlicePilot->getPOC();
      return true;
    }
    m_prevPOC = m_apcSlicePilot->getPOC();
  }
#if H_MV
  if ( newLayerFlag )
  {
    return false; 
  }
#if ENC_DEC_TRACE
#if H_MV_ENC_DEC_TRAC
  // parse remainder of SH
   g_disableHLSTrace = false; 
#endif
#endif
#endif
  // actual decoding starts here
#if H_MV
   // This part needs further testing !
   if ( m_apcSlicePilot->getPocResetFlag() )
   {   
     xResetPocInPicBuffer();
   }

   if ( m_apcSlicePilot->getTLayer() == 0 && m_apcSlicePilot->getEnableTMVPFlag() == 0 )
  {
    //update all pics in the DPB such that they cannot be used for TMPV ref
    TComList<TComPic*>::iterator  iterRefPic = m_cListPic.begin();  
    while( iterRefPic != m_cListPic.end() )
    {
      TComPic *refPic = *iterRefPic;
      if( ( refPic->getLayerId() == m_apcSlicePilot->getLayerId() ) && refPic->getReconMark() )
      {
        for(Int i = refPic->getNumAllocatedSlice()-1; i >= 0; i--)
        {

          TComSlice *refSlice = refPic->getSlice(i);
          refSlice->setAvailableForTMVPRefFlag( false );
        }
      }
      iterRefPic++;
    }
  }
  m_apcSlicePilot->setAvailableForTMVPRefFlag( true );
#endif

  xActivateParameterSets();

  if (m_apcSlicePilot->isNextSlice()) 
  {
    m_prevPOC = m_apcSlicePilot->getPOC();
  }
  m_bFirstSliceInSequence = false;
#if SETTING_NO_OUT_PIC_PRIOR  
  m_bFirstSliceInBitstream  = false;
#endif
  //detect lost reference picture and insert copy of earlier frame.
  Int lostPoc;
  while((lostPoc=m_apcSlicePilot->checkThatAllRefPicsAreAvailable(m_cListPic, m_apcSlicePilot->getRPS(), true, m_pocRandomAccess)) > 0)
  {
    xCreateLostPicture(lostPoc-1);
  }
  if (m_bFirstSliceInPicture)
  {
    // Buffer initialize for prediction.
    m_cPrediction.initTempBuff();
    m_apcSlicePilot->applyReferencePictureSet(m_cListPic, m_apcSlicePilot->getRPS());
#if H_MV
    m_apcSlicePilot->createInterLayerReferencePictureSet( m_ivPicLists, m_refPicSetInterLayer0, m_refPicSetInterLayer1 ); 
#endif
    //  Get a new picture buffer
    xGetNewPicBuffer (m_apcSlicePilot, pcPic);

    Bool isField = false;
    Bool isTff = false;
    
    if(!m_SEIs.empty())
    {
      // Check if any new Picture Timing SEI has arrived
      SEIMessages pictureTimingSEIs = extractSeisByType (m_SEIs, SEI::PICTURE_TIMING);
      if (pictureTimingSEIs.size()>0)
      {
        SEIPictureTiming* pictureTiming = (SEIPictureTiming*) *(pictureTimingSEIs.begin());
        isField = (pictureTiming->m_picStruct == 1) || (pictureTiming->m_picStruct == 2);
        isTff =  (pictureTiming->m_picStruct == 1);
      }
    }
    
    //Set Field/Frame coding mode
    m_pcPic->setField(isField);
    m_pcPic->setTopField(isTff);
    
    // transfer any SEI messages that have been received to the picture
    pcPic->setSEIs(m_SEIs);
    m_SEIs.clear();

    // Recursive structure
    m_cCuDecoder.create ( g_uiMaxCUDepth, g_uiMaxCUWidth, g_uiMaxCUHeight );
    m_cCuDecoder.init   ( &m_cEntropyDecoder, &m_cTrQuant, &m_cPrediction );
    m_cTrQuant.init     ( g_uiMaxCUWidth, g_uiMaxCUHeight, m_apcSlicePilot->getSPS()->getMaxTrSize());

    m_cSliceDecoder.create();
  }
  else
  {
    // Check if any new SEI has arrived
    if(!m_SEIs.empty())
    {
      // Currently only decoding Unit SEI message occurring between VCL NALUs copied
      SEIMessages &picSEI = pcPic->getSEIs();
      SEIMessages decodingUnitInfos = extractSeisByType (m_SEIs, SEI::DECODING_UNIT_INFO);
      picSEI.insert(picSEI.end(), decodingUnitInfos.begin(), decodingUnitInfos.end());
      deleteSEIs(m_SEIs);
    }
  }
  
  //  Set picture slice pointer
  TComSlice*  pcSlice = m_apcSlicePilot;
  Bool bNextSlice     = pcSlice->isNextSlice();

  UInt i;
  pcPic->getPicSym()->initTiles(pcSlice->getPPS());

  //generate the Coding Order Map and Inverse Coding Order Map
  UInt uiEncCUAddr;
  for(i=0, uiEncCUAddr=0; i<pcPic->getPicSym()->getNumberOfCUsInFrame(); i++, uiEncCUAddr = pcPic->getPicSym()->xCalculateNxtCUAddr(uiEncCUAddr))
  {
    pcPic->getPicSym()->setCUOrderMap(i, uiEncCUAddr);
    pcPic->getPicSym()->setInverseCUOrderMap(uiEncCUAddr, i);
  }
  pcPic->getPicSym()->setCUOrderMap(pcPic->getPicSym()->getNumberOfCUsInFrame(), pcPic->getPicSym()->getNumberOfCUsInFrame());
  pcPic->getPicSym()->setInverseCUOrderMap(pcPic->getPicSym()->getNumberOfCUsInFrame(), pcPic->getPicSym()->getNumberOfCUsInFrame());

  //convert the start and end CU addresses of the slice and dependent slice into encoding order
  pcSlice->setSliceSegmentCurStartCUAddr( pcPic->getPicSym()->getPicSCUEncOrder(pcSlice->getSliceSegmentCurStartCUAddr()) );
  pcSlice->setSliceSegmentCurEndCUAddr( pcPic->getPicSym()->getPicSCUEncOrder(pcSlice->getSliceSegmentCurEndCUAddr()) );
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

#if H_MV
  pcPic->setLayerId( nalu.m_layerId );
  pcPic->setViewId ( getViewId() );
#if H_3D
  pcPic->setViewIndex( getViewIndex() );
  pcPic->setIsDepth  ( getIsDepth  () );
#endif
#endif
  if (bNextSlice)
  {
    pcSlice->checkCRA(pcSlice->getRPS(), m_pocCRA, m_associatedIRAPType, m_cListPic );
    // Set reference list
#if H_MV    
    std::vector< TComPic* > tempRefPicLists[2];
    std::vector< Bool     > usedAsLongTerm [2];
    Int       numPocTotalCurr;

    pcSlice->getTempRefPicLists( m_cListPic, m_refPicSetInterLayer0, m_refPicSetInterLayer1, tempRefPicLists, usedAsLongTerm, numPocTotalCurr);
    pcSlice->setRefPicList     ( tempRefPicLists, usedAsLongTerm, numPocTotalCurr, true ); 
#if H_3D_ARP
    pcSlice->setARPStepNum(m_ivPicLists);
    if( pcSlice->getARPStepNum() > 1 )
    {
      // GT: This seems to be broken, not all nuh_layer_ids are necessarily present
      for(Int iLayerId = 0; iLayerId < nalu.m_layerId; iLayerId ++ )
      {
        Int  iViewIdx =   pcSlice->getVPS()->getViewIndex(iLayerId);
        Bool bIsDepth = ( pcSlice->getVPS()->getDepthId  ( iLayerId ) == 1 );
        if( iViewIdx<getViewIndex() && !bIsDepth )
        {
          pcSlice->setBaseViewRefPicList( m_ivPicLists->getPicList( iLayerId ), iViewIdx );
        }
      }
    }
#endif
#else
    pcSlice->setRefPicList( m_cListPic, true );
#endif

#if H_3D
    pcSlice->setIvPicLists( m_ivPicLists );         
#if H_3D_IV_MERGE
#if H_3D_FCO
    //assert( !getIsDepth() );
#else
    assert( !getIsDepth() || ( pcSlice->getTexturePic() != 0 ) );
#endif
#endif    
#endif
#if H_MV
    if( m_layerId > 0 && !pcSlice->isIntra() && pcSlice->getEnableTMVPFlag() )
    {
      TComPic* refPic = pcSlice->getRefPic(RefPicList(1 - pcSlice->getColFromL0Flag()), pcSlice->getColRefIdx());

      assert ( refPic );
      assert ( refPic->getPicSym()->getSlice(0)->getAvailableForTMVPRefFlag() == true );
    }
#endif

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
    if (!pcSlice->isIntra())
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
      if (pcSlice->isInterB())
      {
        for (iRefIdx = 0; iRefIdx < pcSlice->getNumRefIdx(REF_PIC_LIST_1) && bLowDelay; iRefIdx++)
        {
          if ( pcSlice->getRefPic(REF_PIC_LIST_1, iRefIdx)->getPOC() > iCurrPOC )
          {
            bLowDelay = false;
          }
        }        
      }

      pcSlice->setCheckLDC(bLowDelay);            
    }

    //---------------
    pcSlice->setRefPOCList();
#if  H_3D_TMVP
    if(pcSlice->getLayerId())
      pcSlice->generateAlterRefforTMVP();
#endif
  }

  pcPic->setCurrSliceIdx(m_uiSliceIdx);
  if(pcSlice->getSPS()->getScalingListFlag())
  {
    pcSlice->setScalingList ( pcSlice->getSPS()->getScalingList()  );
    if(pcSlice->getPPS()->getScalingListPresentFlag())
    {
      pcSlice->setScalingList ( pcSlice->getPPS()->getScalingList()  );
    }
    if(!pcSlice->getPPS()->getScalingListPresentFlag() && !pcSlice->getSPS()->getScalingListPresentFlag())
    {
      pcSlice->setDefaultScalingList();
    }
    m_cTrQuant.setScalingListDec(pcSlice->getScalingList());
    m_cTrQuant.setUseScalingList(true);
  }
  else
  {
    m_cTrQuant.setFlatScalingList();
    m_cTrQuant.setUseScalingList(false);
  }

#if H_3D_IV_MERGE
#if H_3D_FCO
  if( !pcSlice->getIsDepth() && m_pcCamParsCollector )
#else
  if( pcSlice->getIsDepth() && m_pcCamParsCollector )
#endif
  {
    m_pcCamParsCollector->copyCamParamForSlice( pcSlice );
  }
#endif

  //  Decode a picture
  m_cGopDecoder.decompressSlice(nalu.m_Bitstream, pcPic);
#if H_3D
  if( m_pcCamParsCollector )
  {
    m_pcCamParsCollector->setSlice( pcSlice );
  }
#endif
  m_bFirstSliceInPicture = false;
  m_uiSliceIdx++;

  return false;
}

Void TDecTop::xDecodeVPS()
{
  TComVPS* vps = new TComVPS();
  
  m_cEntropyDecoder.decodeVPS( vps );
  m_parameterSetManagerDecoder.storePrefetchedVPS(vps);  
}

Void TDecTop::xDecodeSPS()
{
  TComSPS* sps = new TComSPS();
#if H_MV
  sps->setLayerId( getLayerId() ); 
#endif
#if H_3D
  // Preliminary fix. assuming that all sps refer to the same VPS. 
  // Parsing dependency should be resolved!
  TComVPS* vps = m_parameterSetManagerDecoder.getPrefetchedVPS( 0 ); 
  assert( vps != 0 );
  m_cEntropyDecoder.decodeSPS( sps, vps->getViewIndex( m_layerId ), ( vps->getDepthId( m_layerId ) == 1 ) );
#else
  m_cEntropyDecoder.decodeSPS( sps );
#endif
  m_parameterSetManagerDecoder.storePrefetchedSPS(sps);
}

Void TDecTop::xDecodePPS()
{
  TComPPS* pps = new TComPPS();
#if H_MV
  pps->setLayerId( getLayerId() ); 
#endif
#if H_3D
  // Assuming that all PPS indirectly refer to the same VPS via different SPS
  // There is no parsing dependency in decoding DLT in PPS. 
  // The VPS information passed to decodePPS() is used to arrange the decoded DLT tables to their corresponding layers. 
  // This is equivalent to the process of 
  //   Step 1) decoding DLT tables based on the number of depth layers, and
  //   Step 2) mapping DLT tables to the depth layers
  // as described in the 3D-HEVC WD. 
  TComVPS* vps = m_parameterSetManagerDecoder.getPrefetchedVPS( 0 );
  m_cEntropyDecoder.decodePPS( pps, vps );
#else
  m_cEntropyDecoder.decodePPS( pps );
#endif
  m_parameterSetManagerDecoder.storePrefetchedPPS( pps );
}

Void TDecTop::xDecodeSEI( TComInputBitstream* bs, const NalUnitType nalUnitType )
{
  if(nalUnitType == NAL_UNIT_SUFFIX_SEI)
  {
#if H_MV
    m_seiReader.parseSEImessage( bs, m_pcPic->getSEIs(), nalUnitType, m_parameterSetManagerDecoder.getActiveSPS( m_layerId ) );
#else
    m_seiReader.parseSEImessage( bs, m_pcPic->getSEIs(), nalUnitType, m_parameterSetManagerDecoder.getActiveSPS() );
#endif
  }
  else
  {
#if H_MV
    m_seiReader.parseSEImessage( bs, m_SEIs, nalUnitType, m_parameterSetManagerDecoder.getActiveSPS( m_layerId ) );
#else
    m_seiReader.parseSEImessage( bs, m_SEIs, nalUnitType, m_parameterSetManagerDecoder.getActiveSPS() );
#endif
    SEIMessages activeParamSets = getSeisByType(m_SEIs, SEI::ACTIVE_PARAMETER_SETS);
    if (activeParamSets.size()>0)
    {
      SEIActiveParameterSets *seiAps = (SEIActiveParameterSets*)(*activeParamSets.begin());
      m_parameterSetManagerDecoder.applyPrefetchedPS();
      assert(seiAps->activeSeqParameterSetId.size()>0);
#if H_MV
      if (! m_parameterSetManagerDecoder.activateSPSWithSEI(seiAps->activeSeqParameterSetId[0], m_layerId ))
#else
      if (! m_parameterSetManagerDecoder.activateSPSWithSEI(seiAps->activeSeqParameterSetId[0] ))
#endif
      {
        printf ("Warning SPS activation with Active parameter set SEI failed");
      }
    }
  }
}

#if H_MV
Bool TDecTop::decode(InputNALUnit& nalu, Int& iSkipFrame, Int& iPOCLastDisplay, Bool newLayerFlag, Bool& sliceSkippedFlag )
#else
Bool TDecTop::decode(InputNALUnit& nalu, Int& iSkipFrame, Int& iPOCLastDisplay)
#endif
{
  // Initialize entropy decoder
  m_cEntropyDecoder.setEntropyDecoder (&m_cCavlcDecoder);
  m_cEntropyDecoder.setBitstream      (nalu.m_Bitstream);

  switch (nalu.m_nalUnitType)
  {
    case NAL_UNIT_VPS:
      xDecodeVPS();
#if H_MV
      m_isLastNALWasEos = false;
#endif
      return false;
      
    case NAL_UNIT_SPS:
      xDecodeSPS();
      return false;

    case NAL_UNIT_PPS:
      xDecodePPS();
      return false;
      
    case NAL_UNIT_PREFIX_SEI:
    case NAL_UNIT_SUFFIX_SEI:
#if H_MV
      if ( nalu.m_nalUnitType == NAL_UNIT_SUFFIX_SEI )
      {
        assert( m_isLastNALWasEos == false );
      }
#endif
      xDecodeSEI( nalu.m_Bitstream, nalu.m_nalUnitType );
      return false;

    case NAL_UNIT_CODED_SLICE_TRAIL_R:
    case NAL_UNIT_CODED_SLICE_TRAIL_N:
    case NAL_UNIT_CODED_SLICE_TSA_R:
    case NAL_UNIT_CODED_SLICE_TSA_N:
    case NAL_UNIT_CODED_SLICE_STSA_R:
    case NAL_UNIT_CODED_SLICE_STSA_N:
    case NAL_UNIT_CODED_SLICE_BLA_W_LP:
    case NAL_UNIT_CODED_SLICE_BLA_W_RADL:
    case NAL_UNIT_CODED_SLICE_BLA_N_LP:
    case NAL_UNIT_CODED_SLICE_IDR_W_RADL:
    case NAL_UNIT_CODED_SLICE_IDR_N_LP:
    case NAL_UNIT_CODED_SLICE_CRA:
    case NAL_UNIT_CODED_SLICE_RADL_N:
    case NAL_UNIT_CODED_SLICE_RADL_R:
    case NAL_UNIT_CODED_SLICE_RASL_N:
    case NAL_UNIT_CODED_SLICE_RASL_R:
#if H_MV
      if (nalu.m_nalUnitType == NAL_UNIT_CODED_SLICE_TRAIL_R || nalu.m_nalUnitType == NAL_UNIT_CODED_SLICE_TRAIL_N ||
          nalu.m_nalUnitType == NAL_UNIT_CODED_SLICE_TSA_R || nalu.m_nalUnitType == NAL_UNIT_CODED_SLICE_TSA_N ||
          nalu.m_nalUnitType == NAL_UNIT_CODED_SLICE_STSA_R || nalu.m_nalUnitType == NAL_UNIT_CODED_SLICE_STSA_N ||
          nalu.m_nalUnitType == NAL_UNIT_CODED_SLICE_RADL_R || nalu.m_nalUnitType == NAL_UNIT_CODED_SLICE_RADL_N ||
          nalu.m_nalUnitType == NAL_UNIT_CODED_SLICE_RASL_R || nalu.m_nalUnitType == NAL_UNIT_CODED_SLICE_RASL_N )
      {
        assert( m_isLastNALWasEos == false );
      }
      else
      {
        m_isLastNALWasEos = false;
      }

      return xDecodeSlice(nalu, iSkipFrame, iPOCLastDisplay, newLayerFlag, sliceSkippedFlag );
#else
      return xDecodeSlice(nalu, iSkipFrame, iPOCLastDisplay);
#endif
      break;
    case NAL_UNIT_EOS:
#if H_MV
      assert( m_isLastNALWasEos == false );
      //Check layer id of the nalu. if it is not 0, give a warning message and just return without doing anything.
      if (nalu.m_layerId > 0)
      {
        printf( "\nThis bitstream has EOS with non-zero layer id.\n" );
        return false;
      }
      m_isLastNALWasEos = true;
#endif
      m_associatedIRAPType = NAL_UNIT_INVALID;
      m_pocCRA = 0;
      m_pocRandomAccess = MAX_INT;
      m_prevPOC = MAX_INT;
      m_bFirstSliceInPicture = true;
      m_bFirstSliceInSequence = true;
      m_prevSliceSkipped = false;
      m_skippedPOC = 0;
      return false;
      
    case NAL_UNIT_ACCESS_UNIT_DELIMITER:
      // TODO: process AU delimiter
      return false;
      
    case NAL_UNIT_EOB:
      return false;
      
    case NAL_UNIT_FILLER_DATA:
#if H_MV
      assert( m_isLastNALWasEos == false );
#endif
      return false;
      
    case NAL_UNIT_RESERVED_VCL_N10:
    case NAL_UNIT_RESERVED_VCL_R11:
    case NAL_UNIT_RESERVED_VCL_N12:
    case NAL_UNIT_RESERVED_VCL_R13:
    case NAL_UNIT_RESERVED_VCL_N14:
    case NAL_UNIT_RESERVED_VCL_R15:
      
    case NAL_UNIT_RESERVED_IRAP_VCL22:
    case NAL_UNIT_RESERVED_IRAP_VCL23:
      
    case NAL_UNIT_RESERVED_VCL24:
    case NAL_UNIT_RESERVED_VCL25:
    case NAL_UNIT_RESERVED_VCL26:
    case NAL_UNIT_RESERVED_VCL27:
    case NAL_UNIT_RESERVED_VCL28:
    case NAL_UNIT_RESERVED_VCL29:
    case NAL_UNIT_RESERVED_VCL30:
    case NAL_UNIT_RESERVED_VCL31:
      
    case NAL_UNIT_RESERVED_NVCL41:
    case NAL_UNIT_RESERVED_NVCL42:
    case NAL_UNIT_RESERVED_NVCL43:
    case NAL_UNIT_RESERVED_NVCL44:
    case NAL_UNIT_RESERVED_NVCL45:
    case NAL_UNIT_RESERVED_NVCL46:
    case NAL_UNIT_RESERVED_NVCL47:
    case NAL_UNIT_UNSPECIFIED_48:
    case NAL_UNIT_UNSPECIFIED_49:
    case NAL_UNIT_UNSPECIFIED_50:
    case NAL_UNIT_UNSPECIFIED_51:
    case NAL_UNIT_UNSPECIFIED_52:
    case NAL_UNIT_UNSPECIFIED_53:
    case NAL_UNIT_UNSPECIFIED_54:
    case NAL_UNIT_UNSPECIFIED_55:
    case NAL_UNIT_UNSPECIFIED_56:
    case NAL_UNIT_UNSPECIFIED_57:
    case NAL_UNIT_UNSPECIFIED_58:
    case NAL_UNIT_UNSPECIFIED_59:
    case NAL_UNIT_UNSPECIFIED_60:
    case NAL_UNIT_UNSPECIFIED_61:
    case NAL_UNIT_UNSPECIFIED_62:
    case NAL_UNIT_UNSPECIFIED_63:
    default:
      assert (0);
  }

  return false;
}

/** Function for checking if picture should be skipped because of association with a previous BLA picture
 * \param iPOCLastDisplay POC of last picture displayed
 * \returns true if the picture should be skipped
 * This function skips all TFD pictures that follow a BLA picture
 * in decoding order and precede it in output order.
 */
Bool TDecTop::isSkipPictureForBLA(Int& iPOCLastDisplay)
{
  if ((m_associatedIRAPType == NAL_UNIT_CODED_SLICE_BLA_N_LP || m_associatedIRAPType == NAL_UNIT_CODED_SLICE_BLA_W_LP || m_associatedIRAPType == NAL_UNIT_CODED_SLICE_BLA_W_RADL) && 
       m_apcSlicePilot->getPOC() < m_pocCRA && (m_apcSlicePilot->getNalUnitType() == NAL_UNIT_CODED_SLICE_RASL_R || m_apcSlicePilot->getNalUnitType() == NAL_UNIT_CODED_SLICE_RASL_N))
  {
    iPOCLastDisplay++;
    return true;
  }
  return false;
}

/** Function for checking if picture should be skipped because of random access
 * \param iSkipFrame skip frame counter
 * \param iPOCLastDisplay POC of last picture displayed
 * \returns true if the picture shold be skipped in the random access.
 * This function checks the skipping of pictures in the case of -s option random access.
 * All pictures prior to the random access point indicated by the counter iSkipFrame are skipped.
 * It also checks the type of Nal unit type at the random access point.
 * If the random access point is CRA/CRANT/BLA/BLANT, TFD pictures with POC less than the POC of the random access point are skipped.
 * If the random access point is IDR all pictures after the random access point are decoded.
 * If the random access point is none of the above, a warning is issues, and decoding of pictures with POC 
 * equal to or greater than the random access point POC is attempted. For non IDR/CRA/BLA random 
 * access point there is no guarantee that the decoder will not crash.
 */
Bool TDecTop::isRandomAccessSkipPicture(Int& iSkipFrame,  Int& iPOCLastDisplay)
{
  if (iSkipFrame) 
  {
    iSkipFrame--;   // decrement the counter
    return true;
  }
#if H_MV
  else if ( !m_layerInitilizedFlag[ m_layerId ] ) // start of random access point, m_pocRandomAccess has not been set yet.
#else
  else if (m_pocRandomAccess == MAX_INT) // start of random access point, m_pocRandomAccess has not been set yet.
#endif
  {
    if (   m_apcSlicePilot->getNalUnitType() == NAL_UNIT_CODED_SLICE_CRA
        || m_apcSlicePilot->getNalUnitType() == NAL_UNIT_CODED_SLICE_BLA_W_LP
        || m_apcSlicePilot->getNalUnitType() == NAL_UNIT_CODED_SLICE_BLA_N_LP
        || m_apcSlicePilot->getNalUnitType() == NAL_UNIT_CODED_SLICE_BLA_W_RADL )
    {

#if H_MV
      if ( xAllRefLayersInitilized() )
      {
        m_layerInitilizedFlag[ m_layerId ] = true; 
        m_pocRandomAccess = m_apcSlicePilot->getPOC();
      }
      else
      {
        return true; 
      }
#else
      // set the POC random access since we need to skip the reordered pictures in the case of CRA/CRANT/BLA/BLANT.
      m_pocRandomAccess = m_apcSlicePilot->getPOC();
#endif
    }
    else if ( m_apcSlicePilot->getNalUnitType() == NAL_UNIT_CODED_SLICE_IDR_W_RADL || m_apcSlicePilot->getNalUnitType() == NAL_UNIT_CODED_SLICE_IDR_N_LP )
    {
#if H_MV
      if ( xAllRefLayersInitilized() )
      {
        m_layerInitilizedFlag[ m_layerId ] = true; 
        m_pocRandomAccess = -MAX_INT; // no need to skip the reordered pictures in IDR, they are decodable.
      }
      else 
      {
        return true; 
      }
#else
      m_pocRandomAccess = -MAX_INT; // no need to skip the reordered pictures in IDR, they are decodable.
#endif
    }
    else 
    {
#if H_MV
      static Bool warningMessage[MAX_NUM_LAYERS];
      static Bool warningInitFlag = false;
      
      if (!warningInitFlag)
      {
        for ( Int i = 0; i < MAX_NUM_LAYERS; i++)
        {
          warningMessage[i] = true; 
        }
        warningInitFlag = true; 
      }

      if ( warningMessage[getLayerId()] )
      {
        printf("\nLayer%3d   No valid random access point. VCL NAL units of this layer are discarded until next layer initialization picture. ", getLayerId() ); 
        warningMessage[m_layerId] = false; 
      }
#else
      static Bool warningMessage = false;
      if(!warningMessage)
      {
        printf("\nWarning: this is not a valid random access point and the data is discarded until the first CRA picture");
        warningMessage = true;
      }
#endif
      return true;
    }
  }
  // skip the reordered pictures, if necessary
  else if (m_apcSlicePilot->getPOC() < m_pocRandomAccess && (m_apcSlicePilot->getNalUnitType() == NAL_UNIT_CODED_SLICE_RASL_R || m_apcSlicePilot->getNalUnitType() == NAL_UNIT_CODED_SLICE_RASL_N))
  {
    iPOCLastDisplay++;
    return true;
  }
#if H_MV
  return !m_layerInitilizedFlag[ getLayerId() ]; 
#else
  // if we reach here, then the picture is not skipped.
  return false; 
#endif
}

#if H_MV
TComPic* TDecTop::getPic( Int poc )
{
  xGetPic( m_layerId, poc ); 
  TComList<TComPic*>* listPic = getListPic();
  TComPic* pcPic = NULL;
  for(TComList<TComPic*>::iterator it=listPic->begin(); it!=listPic->end(); it++)
  {
    if( (*it)->getPOC() == poc )
    {
      pcPic = *it ;
      break ;
    }
  }
  return pcPic;
}

TComPic* TDecTop::xGetPic( Int layerId, Int poc )
{ 
  return m_ivPicLists->getPic( layerId, poc ) ;
}

Void TDecTop::xResetPocInPicBuffer()
{
  TComList<TComPic*>::iterator  iterPic   = m_cListPic.begin();
  while (iterPic != m_cListPic.end())
  {
    TComPic* pic = *(iterPic++);
    if ( pic->getReconMark() )
    {
      for( Int i = 0; i < pic->getNumAllocatedSlice(); i++)
      {
        TComSlice* slice = pic->getSlice( i ); 
        slice->setPOC ( slice->getPOC() - m_apcSlicePilot->getPocBeforeReset() );            
      }         
    }     
  }
}

#if H_MV
Void TDecTop::xCeckNoClrasOutput()
{
  // This part needs further testing! 
  if ( getLayerId() == 0 )
  {    
    NalUnitType nut = m_apcSlicePilot->getNalUnitType(); 

    Bool isBLA =  ( nut == NAL_UNIT_CODED_SLICE_BLA_W_LP  )  || ( nut == NAL_UNIT_CODED_SLICE_BLA_N_LP ) || ( nut == NAL_UNIT_CODED_SLICE_BLA_W_RADL ); 
    Bool isIDR  = ( nut == NAL_UNIT_CODED_SLICE_IDR_W_RADL ) || ( nut == NAL_UNIT_CODED_SLICE_IDR_N_LP ); 
    Bool noClrasOutputFlag  = isBLA || ( isIDR  &&  m_apcSlicePilot->getCrossLayerBlaFlag() ); 

    if ( noClrasOutputFlag ) 
    {
      for (Int i = 0; i < MAX_NUM_LAYER_IDS; i++)
      {
        m_layerInitilizedFlag[i] = false; 
      } 
    }
  }
}

Bool TDecTop::xAllRefLayersInitilized()
{
  Bool allRefLayersInitilizedFlag = true; 
  TComVPS* vps = m_parameterSetManagerDecoder.getPrefetchedVPS( 0 ); 
  for (Int i = 0; i < vps->getNumDirectRefLayers( getLayerId()  ); i++ )
  {
    Int refLayerId = vps->getIdDirectRefLayer( m_layerId, i ); 
    allRefLayersInitilizedFlag = allRefLayersInitilizedFlag && m_layerInitilizedFlag[ refLayerId ]; 
  }

  return allRefLayersInitilizedFlag;
}
#endif
#endif
//! \}
