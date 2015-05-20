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
#if NH_MV
ParameterSetManager TDecTop::m_parameterSetManager;
#endif

//! \ingroup TLibDecoder
//! \{

#if NH_3D
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
  xDeleteArray( m_receivedIdc, m_vps->getNumViews() );
}


Void
CamParsCollector::init( const TComVPS* vps)
{
  assert( !isInitialized() ); // Only one initialization currently supported
  m_bInitialized            = true;
  m_vps                     = vps; 
  m_bCamParsVaryOverTime    = false;   
  m_lastPoc                 = -1;   
  m_firstReceivedPoc        = -2; 

  for (Int i = 0; i <= vps->getMaxLayersMinus1(); i++)
  {
    Int curViewIdxInVps = m_vps->getVoiInVps( m_vps->getViewIndex( m_vps->getLayerIdInNuh( i ) ) ) ; 
    m_bCamParsVaryOverTime = m_bCamParsVaryOverTime || vps->getCpInSliceSegmentHeaderFlag( curViewIdxInVps );    
  }

  assert( m_receivedIdc == NULL ); 
  m_receivedIdc = new Int*[ m_vps->getNumViews() ]; 
  for (Int i = 0; i < m_vps->getNumViews(); i++)
  {
    m_receivedIdc[i] = new Int[ m_vps->getNumViews() ]; 
  }

  xResetReceivedIdc( true ); 

  for (Int voiInVps = 0; voiInVps < m_vps->getNumViews(); voiInVps++ )
  {
    if( !m_vps->getCpInSliceSegmentHeaderFlag( voiInVps ) ) 
    {
      for (Int baseVoiInVps = 0; baseVoiInVps < m_vps->getNumViews(); baseVoiInVps++ )
      { 
        if( m_vps->getCpPresentFlag( voiInVps, baseVoiInVps ) )
        {
          m_receivedIdc   [ baseVoiInVps ][ voiInVps ] = -1; 
          m_aaiCodedScale [ baseVoiInVps ][ voiInVps ] = m_vps->getCodedScale    (voiInVps) [ baseVoiInVps ];
          m_aaiCodedOffset[ baseVoiInVps ][ voiInVps ] = m_vps->getCodedOffset   (voiInVps) [ baseVoiInVps ];

          m_receivedIdc   [ voiInVps ][ baseVoiInVps ] = -1; 
          m_aaiCodedScale [ voiInVps ][ baseVoiInVps ] = m_vps->getInvCodedScale (voiInVps) [ baseVoiInVps ];
          m_aaiCodedOffset[ voiInVps ][ baseVoiInVps ] = m_vps->getInvCodedOffset(voiInVps) [ baseVoiInVps ];
          xInitLUTs( baseVoiInVps, voiInVps, m_aaiCodedScale[ baseVoiInVps ][ voiInVps ], m_aaiCodedOffset[ baseVoiInVps ][ voiInVps ], m_adBaseViewShiftLUT, m_aiBaseViewShiftLUT );
          xInitLUTs( voiInVps, baseVoiInVps, m_aaiCodedScale[ voiInVps ][ baseVoiInVps ], m_aaiCodedOffset[ voiInVps ][ baseVoiInVps ], m_adBaseViewShiftLUT, m_aiBaseViewShiftLUT );
        }
      }
    }
  }
}




Void
CamParsCollector::xResetReceivedIdc( Bool overWriteFlag )
{
  for (Int i = 0; i < m_vps->getNumViews(); i++)
  {  
    for (Int j = 0; j < m_vps->getNumViews(); j++)
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
  Int     iLog2DivLuma   = m_uiBitDepthForLUT + m_vps->getCpPrecision() + 1 - m_iLog2Precision;   AOF( iLog2DivLuma > 0 );
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

  UInt voiInVps          = m_vps->getVoiInVps(pcSlice->getViewIndex());  
  if( m_vps->getCpInSliceSegmentHeaderFlag( voiInVps ) ) // check consistency of slice parameters here
  {   
    for( Int baseVoiInVps = 0; baseVoiInVps < m_vps->getNumViews(); baseVoiInVps++ )
    {        
      if ( m_vps->getCpPresentFlag( voiInVps, baseVoiInVps ) )
      {
        if ( m_receivedIdc[ voiInVps ][ baseVoiInVps ] != 0 )
        {      
          AOF( m_aaiCodedScale [ voiInVps ][ baseVoiInVps ] == pcSlice->getInvCodedScale () [ baseVoiInVps ] );
          AOF( m_aaiCodedOffset[ voiInVps ][ baseVoiInVps ] == pcSlice->getInvCodedOffset() [ baseVoiInVps ] );
        }
        else
        {          
          m_receivedIdc   [ voiInVps ][ baseVoiInVps ]  = 1; 
          m_aaiCodedScale [ voiInVps ][ baseVoiInVps ]  = pcSlice->getInvCodedScale () [ baseVoiInVps ];
          m_aaiCodedOffset[ voiInVps ][ baseVoiInVps ]  = pcSlice->getInvCodedOffset() [ baseVoiInVps ];
          xInitLUTs( voiInVps, baseVoiInVps, m_aaiCodedScale[ voiInVps ][ baseVoiInVps ], m_aaiCodedOffset[ voiInVps ][ baseVoiInVps ], m_adBaseViewShiftLUT, m_aiBaseViewShiftLUT);
        }
        if ( m_receivedIdc[ baseVoiInVps ][ voiInVps ] != 0 )
        {      
          AOF( m_aaiCodedScale [ baseVoiInVps ][ voiInVps ] == pcSlice->getCodedScale    () [ baseVoiInVps ] );
          AOF( m_aaiCodedOffset[ baseVoiInVps ][ voiInVps ] == pcSlice->getCodedOffset   () [ baseVoiInVps ] );
        }
        else
        {        
          m_receivedIdc   [ baseVoiInVps ][ voiInVps ]  = 1; 
          m_aaiCodedScale [ baseVoiInVps ][ voiInVps ]  = pcSlice->getCodedScale    () [ baseVoiInVps ];
          m_aaiCodedOffset[ baseVoiInVps ][ voiInVps ]  = pcSlice->getCodedOffset   () [ baseVoiInVps ];
          xInitLUTs( baseVoiInVps, voiInVps, m_aaiCodedScale[ baseVoiInVps ][ voiInVps ], m_aaiCodedOffset[ baseVoiInVps ][ voiInVps ], m_adBaseViewShiftLUT, m_aiBaseViewShiftLUT);
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
      fprintf( m_pCodedScaleOffsetFile, "#ViewOrderIdx     ViewIdVal\n" );
      fprintf( m_pCodedScaleOffsetFile, "#------------ -------------\n" );
      
      for( UInt voiInVps = 0; voiInVps < m_vps->getNumViews(); voiInVps++ )
      {
        fprintf( m_pCodedScaleOffsetFile, "%13d %13d\n", m_vps->getViewOIdxList( voiInVps ), m_vps->getViewIdVal( m_vps->getViewOIdxList( voiInVps ) ) );
      }
      fprintf( m_pCodedScaleOffsetFile, "\n\n");
      fprintf( m_pCodedScaleOffsetFile, "# StartFrame     EndFrame    TargetVOI      BaseVOI   CodedScale  CodedOffset    Precision\n" );
      fprintf( m_pCodedScaleOffsetFile, "#----------- ------------ ------------ ------------ ------------ ------------ ------------\n" );
    }
    if( iPOC == m_firstReceivedPoc || m_bCamParsVaryOverTime  )
    {
      Int iS = iPOC;
      Int iE = ( m_bCamParsVaryOverTime ? iPOC : ~( 1 << 31 ) );
      for( UInt voiInVps = 0; voiInVps < m_vps->getNumViews(); voiInVps++ )
      {
        for( UInt baseVoiInVps = 0; baseVoiInVps < m_vps->getNumViews(); baseVoiInVps++ )
        {
          if( voiInVps != baseVoiInVps )
          {
            if ( m_receivedIdc[baseVoiInVps][voiInVps] != 0 )
            {            
              fprintf( m_pCodedScaleOffsetFile, "%12d %12d %12d %12d %12d %12d %12d\n",
                iS, iE, m_vps->getViewOIdxList( voiInVps ), m_vps->getViewOIdxList( baseVoiInVps ), 
                m_aaiCodedScale [ baseVoiInVps ][ voiInVps ], 
                m_aaiCodedOffset[ baseVoiInVps ][ voiInVps ], m_vps->getCpPrecision() );
            }            
          }
        }
      }
    }
  }
}
#endif


TDecTop::TDecTop()
  : m_iMaxRefPicNum(0)
  , m_associatedIRAPType(NAL_UNIT_INVALID)
  , m_pocCRA(0)
  , m_pocRandomAccess(MAX_INT)
  , m_cListPic()
#if !NH_MV
  , m_parameterSetManager()
#endif
  , m_apcSlicePilot(NULL)
  , m_SEIs()
  , m_cPrediction()
  , m_cTrQuant()
  , m_cGopDecoder()
  , m_cSliceDecoder()
  , m_cCuDecoder()
  , m_cEntropyDecoder()
  , m_cCavlcDecoder()
  , m_cSbacDecoder()
  , m_cBinCABAC()
  , m_seiReader()
  , m_cLoopFilter()
  , m_cSAO()
  , m_pcPic(NULL)
  , m_prevPOC(MAX_INT)
  , m_prevTid0POC(0)
  , m_bFirstSliceInPicture(true)
  , m_bFirstSliceInSequence(true)
  , m_prevSliceSkipped(false)
  , m_skippedPOC(0)
  , m_bFirstSliceInBitstream(true)
  , m_lastPOCNoOutputPriorPics(-1)
  , m_isNoOutputPriorPics(false)
  , m_craNoRaslOutputFlag(false)
#if O0043_BEST_EFFORT_DECODING
  , m_forceDecodeBitDepth(8)
#endif
  , m_pDecodedSEIOutputStream(NULL)
  , m_warningMessageSkipPicture(false)
  , m_prefixSEINALUs()
{
#if ENC_DEC_TRACE
#if NH_MV
  if ( g_hTrace == NULL )
  {
#endif
  if (g_hTrace == NULL)
  {
    g_hTrace = fopen( "TraceDec.txt", "wb" );
  }
  g_bJustDoIt = g_bEncDecTraceDisable;
  g_nSymbolCounter = 0;
#if NH_MV
  }
#endif
#endif
#if NH_MV
  m_isLastNALWasEos = false;
  m_layerId = 0;
  m_viewId = 0;
#if NH_3D
  m_viewIndex = 0; 
  m_isDepth = false;
  m_pcCamParsCollector = 0;
#endif
#if NH_MV
  m_targetOlsIdx = -1; 
#endif
#endif

}

TDecTop::~TDecTop()
{
#if ENC_DEC_TRACE
  if (g_hTrace != stdout)
  {
    fclose( g_hTrace );
  }
#endif
  while (!m_prefixSEINALUs.empty())
  {
    delete m_prefixSEINALUs.front();
    m_prefixSEINALUs.pop_front();
  }
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
#if !NH_MV
  initROM();
#endif
#if NH_MV
  m_cCavlcDecoder.setDecTop( this ); 
#endif
  m_cGopDecoder.init( &m_cEntropyDecoder, &m_cSbacDecoder, &m_cBinCABAC, &m_cCavlcDecoder, &m_cSliceDecoder, &m_cLoopFilter, &m_cSAO);
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
#if NH_MV
    if( pcPic )
    {
#endif
    pcPic->destroy();

    delete pcPic;
    pcPic = NULL;
#if NH_MV
    }
#endif
  }

  m_cSAO.destroy();

  m_cLoopFilter.        destroy();

#if !NH_MV
  // destroy ROM
  destroyROM();
#endif
}

Void TDecTop::xGetNewPicBuffer ( const TComSPS &sps, const TComPPS &pps, TComPic*& rpcPic, const UInt temporalLayer )
{
#if NH_MV
  if ( getLayerId() == 0 )
  {  
    m_iMaxRefPicNum = sps.getMaxDecPicBuffering(temporalLayer);     // m_uiMaxDecPicBuffering has the space for the picture currently being decoded
  }
  else
  {
    //GTCIC
    //m_iMaxRefPicNum = pcSlice->getVPS()->getMaxDecPicBuffering(pcSlice->getTLayer());    
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
  m_iMaxRefPicNum = sps.getMaxDecPicBuffering(temporalLayer);     // m_uiMaxDecPicBuffering has the space for the picture currently being decoded
#endif
  if (m_cListPic.size() < (UInt)m_iMaxRefPicNum)
  {
    rpcPic = new TComPic();

    rpcPic->create ( sps, pps, true);

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
#if NH_MV
      rpcPic->setPicOutputFlag(false); 
#endif
      bBufferIsAvailable = true;
      break;
    }

    if ( rpcPic->getSlice( 0 )->isReferenced() == false  && rpcPic->getOutputMark() == false)
    {
      rpcPic->setOutputMark(false);
#if NH_MV
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
  rpcPic->create ( sps, pps, true);
}
#if NH_MV
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

  TComPic*   pcPic         = m_pcPic;

  // Execute Deblock + Cleanup

  m_cGopDecoder.filterPicture(pcPic);

  TComSlice::sortPicList( m_cListPic ); // sorting for application output
  poc                 = pcPic->getSlice(m_uiSliceIdx-1)->getPOC();
  rpcListPic          = &m_cListPic;
  m_cCuDecoder.destroy();
#if NH_MV 
  TComSlice::markIvRefPicsAsShortTerm( m_refPicSetInterLayer0, m_refPicSetInterLayer1 );  
  TComSlice::markCurrPic( pcPic ); 
#endif
  m_bFirstSliceInPicture  = true;

  return;
}

Void TDecTop::checkNoOutputPriorPics (TComList<TComPic*>* pcListPic)
{
  if (!pcListPic || !m_isNoOutputPriorPics)
  {
    return;
  }

  TComList<TComPic*>::iterator  iterPic   = pcListPic->begin();

  while (iterPic != pcListPic->end())
  {
    TComPic* pcPicTmp = *(iterPic++);
    if (m_lastPOCNoOutputPriorPics != pcPicTmp->getPOC())
    {
      pcPicTmp->setOutputMark(false);
#if NH_MV
      pcPicTmp->setPicOutputFlag(false); 
#endif
    }
  }
}

Void TDecTop::xCreateLostPicture(Int iLostPoc)
{
  printf("\ninserting lost poc : %d\n",iLostPoc);
  TComPic *cFillPic;
  xGetNewPicBuffer(*(m_parameterSetManager.getFirstSPS()), *(m_parameterSetManager.getFirstPPS()), cFillPic, 0);
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
  for(Int ctuRsAddr=0; ctuRsAddr<cFillPic->getNumberOfCtusInFrame(); ctuRsAddr++)
  {
    cFillPic->getCtu(ctuRsAddr)->initCtu(cFillPic, ctuRsAddr);
  }
  cFillPic->getSlice(0)->setReferenced(true);
  cFillPic->getSlice(0)->setPOC(iLostPoc);
  xUpdatePreviousTid0POC(cFillPic->getSlice(0));
  cFillPic->setReconMark(true);
  cFillPic->setOutputMark(true);
  if(m_pocRandomAccess == MAX_INT)
  {
    m_pocRandomAccess = iLostPoc;
  }
}


Void TDecTop::xActivateParameterSets()
{
  if (m_bFirstSliceInPicture)
  {
    const TComPPS *pps = m_parameterSetManager.getPPS(m_apcSlicePilot->getPPSId()); // this is a temporary PPS object. Do not store this value
    assert (pps != 0);

    const TComSPS *sps = m_parameterSetManager.getSPS(pps->getSPSId());             // this is a temporary SPS object. Do not store this value
    assert (sps != 0);

    m_parameterSetManager.clearSPSChangedFlag(sps->getSPSId());
    m_parameterSetManager.clearPPSChangedFlag(pps->getPPSId());
#if NH_MV
    const TComVPS* vps = m_parameterSetManager.getVPS(sps->getVPSId());
    assert (vps != 0); 
    if (!m_parameterSetManager.activatePPS(m_apcSlicePilot->getPPSId(),m_apcSlicePilot->isIRAP(), m_layerId ) )
#else
    if (false == m_parameterSetManager.activatePPS(m_apcSlicePilot->getPPSId(),m_apcSlicePilot->isIRAP()))
#endif
    {
      printf ("Parameter set activation failed!");
      assert (0);
    }

#if NH_MV
  // When the value of vps_num_rep_formats_minus1 in the active VPS is equal to 0
  if ( vps->getVpsNumRepFormatsMinus1() == 0 )
  {
    //, it is a requirement of bitstream conformance that the value of update_rep_format_flag shall be equal to 0.
    assert( sps->getUpdateRepFormatFlag() == false ); 
  }
  sps->checkRpsMaxNumPics( vps, getLayerId() ); 

  // It is a requirement of bitstream conformance that, when the SPS is referred to by 
  // any current picture that belongs to an independent non-base layer, the value of 
  // MultiLayerExtSpsFlag derived from the SPS shall be equal to 0.

  if ( m_layerId > 0 && vps->getNumRefLayers( m_layerId ) == 0 )
  {  
    assert( sps->getMultiLayerExtSpsFlag() == 0 ); 
  }
#endif

    xParsePrefixSEImessages();

#if RExt__HIGH_BIT_DEPTH_SUPPORT==0
    if (sps->getSpsRangeExtension().getExtendedPrecisionProcessingFlag() || sps->getBitDepth(CHANNEL_TYPE_LUMA)>12 || sps->getBitDepth(CHANNEL_TYPE_CHROMA)>12 )
    {
      printf("High bit depth support must be enabled at compile-time in order to decode this bitstream\n");
      assert (0);
      exit(1);
    }
#endif

    // NOTE: globals were set up here originally. You can now use:
    // g_uiMaxCUDepth = sps->getMaxTotalCUDepth();
    // g_uiAddCUDepth = sps->getMaxTotalCUDepth() - sps->getLog2DiffMaxMinCodingBlockSize()

    //  Get a new picture buffer. This will also set up m_pcPic, and therefore give us a SPS and PPS pointer that we can use.
    xGetNewPicBuffer (*(sps), *(pps), m_pcPic, m_apcSlicePilot->getTLayer());
    m_apcSlicePilot->applyReferencePictureSet(m_cListPic, m_apcSlicePilot->getRPS());
#if NH_MV
    m_apcSlicePilot->createInterLayerReferencePictureSet( m_ivPicLists, m_refPicSetInterLayer0, m_refPicSetInterLayer1 ); 
#endif

    // make the slice-pilot a real slice, and set up the slice-pilot for the next slice
    assert(m_pcPic->getNumAllocatedSlice() == (m_uiSliceIdx + 1));
    m_apcSlicePilot = m_pcPic->getPicSym()->swapSliceObject(m_apcSlicePilot, m_uiSliceIdx);

    // we now have a real slice:
    TComSlice *pSlice = m_pcPic->getSlice(m_uiSliceIdx);

    // Update the PPS and SPS pointers with the ones of the picture.
    pps=pSlice->getPPS();
    sps=pSlice->getSPS();

#if NH_MV
    vps=pSlice->getVPS();  
    // The nuh_layer_id value of the NAL unit containing the PPS that is activated for a layer layerA with nuh_layer_id equal to nuhLayerIdA shall be equal to 0, or nuhLayerIdA, or the nuh_layer_id of a direct or indirect reference layer of layerA.
    assert( pps->getLayerId() == m_layerId || pps->getLayerId( ) == 0 || vps->getDependencyFlag( m_layerId, pps->getLayerId() ) );   
    // The nuh_layer_id value of the NAL unit containing the SPS that is activated for a layer layerA with nuh_layer_id equal to nuhLayerIdA shall be equal to 0, or nuhLayerIdA, or the nuh_layer_id of a direct or indirect reference layer of layerA.
    assert( sps->getLayerId() == m_layerId || sps->getLayerId( ) == 0 || vps->getDependencyFlag( m_layerId, sps->getLayerId() ) );
#endif

#if NH_3D
    if ( !m_pcCamParsCollector->isInitialized() )
    {
      m_pcCamParsCollector->init( vps );
    }
#endif
    // Initialise the various objects for the new set of settings
    m_cSAO.create( sps->getPicWidthInLumaSamples(), sps->getPicHeightInLumaSamples(), sps->getChromaFormatIdc(), sps->getMaxCUWidth(), sps->getMaxCUHeight(), sps->getMaxTotalCUDepth(), pps->getPpsRangeExtension().getLog2SaoOffsetScale(CHANNEL_TYPE_LUMA), pps->getPpsRangeExtension().getLog2SaoOffsetScale(CHANNEL_TYPE_CHROMA) );
    m_cLoopFilter.create( sps->getMaxTotalCUDepth() );
    m_cPrediction.initTempBuff(sps->getChromaFormatIdc());


    Bool isField = false;
    Bool isTopField = false;

    if(!m_SEIs.empty())
    {
      // Check if any new Picture Timing SEI has arrived
      SEIMessages pictureTimingSEIs = getSeisByType(m_SEIs, SEI::PICTURE_TIMING);
      if (pictureTimingSEIs.size()>0)
      {
        SEIPictureTiming* pictureTiming = (SEIPictureTiming*) *(pictureTimingSEIs.begin());
        isField    = (pictureTiming->m_picStruct == 1) || (pictureTiming->m_picStruct == 2) || (pictureTiming->m_picStruct == 9) || (pictureTiming->m_picStruct == 10) || (pictureTiming->m_picStruct == 11) || (pictureTiming->m_picStruct == 12);
        isTopField = (pictureTiming->m_picStruct == 1) || (pictureTiming->m_picStruct == 9) || (pictureTiming->m_picStruct == 11);
      }
    }

    //Set Field/Frame coding mode
    m_pcPic->setField(isField);
    m_pcPic->setTopField(isTopField);

    // transfer any SEI messages that have been received to the picture
    m_pcPic->setSEIs(m_SEIs);
    m_SEIs.clear();

    // Recursive structure
    m_cCuDecoder.create ( sps->getMaxTotalCUDepth(), sps->getMaxCUWidth(), sps->getMaxCUHeight(), sps->getChromaFormatIdc() );
    m_cCuDecoder.init   ( &m_cEntropyDecoder, &m_cTrQuant, &m_cPrediction );
    m_cTrQuant.init     ( sps->getMaxTrSize() );

    m_cSliceDecoder.create();
  }
  else
  {
    // make the slice-pilot a real slice, and set up the slice-pilot for the next slice
    m_pcPic->allocateNewSlice();
    assert(m_pcPic->getNumAllocatedSlice() == (m_uiSliceIdx + 1));
    m_apcSlicePilot = m_pcPic->getPicSym()->swapSliceObject(m_apcSlicePilot, m_uiSliceIdx);

    TComSlice *pSlice = m_pcPic->getSlice(m_uiSliceIdx); // we now have a real slice.

    const TComSPS *sps = pSlice->getSPS();
    const TComPPS *pps = pSlice->getPPS();

    // check that the current active PPS has not changed...
    if (m_parameterSetManager.getSPSChangedFlag(sps->getSPSId()) )
    {
      printf("Error - a new SPS has been decoded while processing a picture\n");
      exit(1);
    }
    if (m_parameterSetManager.getPPSChangedFlag(pps->getPPSId()) )
    {
      printf("Error - a new PPS has been decoded while processing a picture\n");
      exit(1);
    }

    xParsePrefixSEImessages();

    // Check if any new SEI has arrived
     if(!m_SEIs.empty())
     {
       // Currently only decoding Unit SEI message occurring between VCL NALUs copied
       SEIMessages &picSEI = m_pcPic->getSEIs();
       SEIMessages decodingUnitInfos = extractSeisByType (m_SEIs, SEI::DECODING_UNIT_INFO);
       picSEI.insert(picSEI.end(), decodingUnitInfos.begin(), decodingUnitInfos.end());
       deleteSEIs(m_SEIs);
     }
  }
}
Void TDecTop::xParsePrefixSEIsForUnknownVCLNal()
{
  while (!m_prefixSEINALUs.empty())
  {
    // do nothing?
    printf("Discarding Prefix SEI associated with unknown VCL NAL unit.\n");
    delete m_prefixSEINALUs.front();
  }
  // TODO: discard following suffix SEIs as well?
}


Void TDecTop::xParsePrefixSEImessages()
{
  while (!m_prefixSEINALUs.empty())
  {
    InputNALUnit &nalu=*m_prefixSEINALUs.front();
#if NH_MV
    m_seiReader.parseSEImessage( &(nalu.getBitstream()), m_SEIs, nalu.m_nalUnitType, m_parameterSetManager.getActiveSPS( getLayerId() ), m_pDecodedSEIOutputStream );
#else
    m_seiReader.parseSEImessage( &(nalu.getBitstream()), m_SEIs, nalu.m_nalUnitType, m_parameterSetManager.getActiveSPS(), m_pDecodedSEIOutputStream );
#endif
    delete m_prefixSEINALUs.front();
    m_prefixSEINALUs.pop_front();
  }
}


#if NH_MV
Bool TDecTop::xDecodeSlice(InputNALUnit &nalu, Int &iSkipFrame, Int iPOCLastDisplay, Bool newLayerFlag, Bool& sliceSkippedFlag  )
{
  assert( nalu.m_nuhLayerId == m_layerId ); 
#else
Bool TDecTop::xDecodeSlice(InputNALUnit &nalu, Int &iSkipFrame, Int iPOCLastDisplay )
{
#endif
  m_apcSlicePilot->initSlice(); // the slice pilot is an object to prepare for a new slice
                                // it is not associated with picture, sps or pps structures.

  if (m_bFirstSliceInPicture)
  {
    m_uiSliceIdx = 0;
  }
  else
  {
    m_apcSlicePilot->copySliceInfo( m_pcPic->getPicSym()->getSlice(m_uiSliceIdx-1) );
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

#if ENC_DEC_TRACE
  const UInt64 originalSymbolCount = g_nSymbolCounter;
#endif

#if NH_MV
  m_apcSlicePilot->setRefPicSetInterLayer( & m_refPicSetInterLayer0, &m_refPicSetInterLayer1 ); 
  m_apcSlicePilot->setLayerId( nalu.m_nuhLayerId );
#endif

  m_cEntropyDecoder.decodeSliceHeader (m_apcSlicePilot, &m_parameterSetManager, m_prevTid0POC);

  // set POC for dependent slices in skipped pictures
  if(m_apcSlicePilot->getDependentSliceSegmentFlag() && m_prevSliceSkipped)
  {
    m_apcSlicePilot->setPOC(m_skippedPOC);
  }

  xUpdatePreviousTid0POC(m_apcSlicePilot);

  m_apcSlicePilot->setAssociatedIRAPPOC(m_pocCRA);
  m_apcSlicePilot->setAssociatedIRAPType(m_associatedIRAPType);
#if NH_MV  
  const TComVPS* vps     = m_apcSlicePilot->getVPS();
  Int layerId            = nalu.m_nuhLayerId;   
  setViewId   ( vps->getViewId   ( layerId )      );  
#if NH_3D
  setViewIndex( vps->getViewIndex( layerId )      );  
  setIsDepth  ( vps->getDepthId  ( layerId ) == 1 );  
  m_ivPicLists->setVPS( vps ); 
#endif
#endif

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

  if (m_apcSlicePilot->getNalUnitType() == NAL_UNIT_CODED_SLICE_CRA && m_craNoRaslOutputFlag) //Reset POC MSB when CRA has NoRaslOutputFlag equal to 1
  {
    TComPPS *pps = m_parameterSetManager.getPPS(m_apcSlicePilot->getPPSId());
    assert (pps != 0);
    TComSPS *sps = m_parameterSetManager.getSPS(pps->getSPSId());
    assert (sps != 0);
    Int iMaxPOClsb = 1 << sps->getBitsForPOC();
    m_apcSlicePilot->setPOC( m_apcSlicePilot->getPOC() & (iMaxPOClsb - 1) );
    xUpdatePreviousTid0POC(m_apcSlicePilot);
  }
#if NH_MV
    xCeckNoClrasOutput();
#endif

  // Skip pictures due to random access

#if NH_MV
  if (isRandomAccessSkipPicture(iSkipFrame, iPOCLastDisplay, vps))
#else
  if (isRandomAccessSkipPicture(iSkipFrame, iPOCLastDisplay))
#endif
  {
    m_prevSliceSkipped = true;
    m_skippedPOC = m_apcSlicePilot->getPOC();
#if NH_MV
    sliceSkippedFlag = true; 
#endif
    return false;
  }
  // Skip TFD pictures associated with BLA/BLANT pictures
  if (isSkipPictureForBLA(iPOCLastDisplay))
  {
    m_prevSliceSkipped = true;
    m_skippedPOC = m_apcSlicePilot->getPOC();
#if NH_MV
    sliceSkippedFlag = true; 
#endif
    return false;
  }

  // clear previous slice skipped flag
  m_prevSliceSkipped = false;

  //we should only get a different poc for a new picture (with CTU address==0)
  if (!m_apcSlicePilot->getDependentSliceSegmentFlag() && m_apcSlicePilot->getPOC()!=m_prevPOC && !m_bFirstSliceInSequence && (m_apcSlicePilot->getSliceCurStartCtuTsAddr() != 0))
  {
    printf ("Warning, the first slice of a picture might have been lost!\n");
  }

  // exit when a new picture is found
  if (!m_apcSlicePilot->getDependentSliceSegmentFlag() && (m_apcSlicePilot->getSliceCurStartCtuTsAddr() == 0 && !m_bFirstSliceInPicture) )
  {
    if (m_prevPOC >= m_pocRandomAccess)
    {
      m_prevPOC = m_apcSlicePilot->getPOC();
#if ENC_DEC_TRACE
      //rewind the trace counter since we didn't actually decode the slice
      g_nSymbolCounter = originalSymbolCount;
#endif
      return true;
    }
    m_prevPOC = m_apcSlicePilot->getPOC();
  }
#if NH_MV
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

#if NH_MV
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

  //detect lost reference picture and insert copy of earlier frame.
  {
    Int lostPoc;
    while((lostPoc=m_apcSlicePilot->checkThatAllRefPicsAreAvailable(m_cListPic, m_apcSlicePilot->getRPS(), true, m_pocRandomAccess)) > 0)
    {
      xCreateLostPicture(lostPoc-1);
    }
  }

  if (!m_apcSlicePilot->getDependentSliceSegmentFlag())
  {
    m_prevPOC = m_apcSlicePilot->getPOC();
  }

  // actual decoding starts here
  xActivateParameterSets();

  m_bFirstSliceInSequence = false;
  m_bFirstSliceInBitstream  = false;


  TComSlice* pcSlice = m_pcPic->getPicSym()->getSlice(m_uiSliceIdx);

  // When decoding the slice header, the stored start and end addresses were actually RS addresses, not TS addresses.
  // Now, having set up the maps, convert them to the correct form.
  pcSlice->setSliceSegmentCurStartCtuTsAddr( m_pcPic->getPicSym()->getCtuRsToTsAddrMap(pcSlice->getSliceSegmentCurStartCtuTsAddr()) );
  pcSlice->setSliceSegmentCurEndCtuTsAddr( m_pcPic->getPicSym()->getCtuRsToTsAddrMap(pcSlice->getSliceSegmentCurEndCtuTsAddr()) );
  if(!pcSlice->getDependentSliceSegmentFlag())
  {
    pcSlice->setSliceCurStartCtuTsAddr(m_pcPic->getPicSym()->getCtuRsToTsAddrMap(pcSlice->getSliceCurStartCtuTsAddr()));
    pcSlice->setSliceCurEndCtuTsAddr(m_pcPic->getPicSym()->getCtuRsToTsAddrMap(pcSlice->getSliceCurEndCtuTsAddr()));
  }

  m_pcPic->setTLayer(nalu.m_temporalId);

#if NH_MV
  //Check Multiview Main profile constraint in G.11.1.1
  //  When ViewOrderIdx[ i ] derived according to any active VPS is equal to 1 
  //  for the layer with nuh_layer_id equal to i in subBitstream, 
  //  inter_view_mv_vert_constraint_flag shall be equal to 1 
  //  in the sps_multilayer_extension( ) syntax structure in each active SPS for that layer.
  if( pcSlice->getSPS()->getPTL()->getGeneralPTL()->getProfileIdc()==Profile::MULTIVIEWMAIN
      &&
      pcSlice->getVPS()->getViewOrderIdx(pcSlice->getVPS()->getLayerIdInNuh(getLayerId()))==1 
     )
  {
    assert( pcSlice->getSPS()->getInterViewMvVertConstraintFlag()==1 );
  }
#endif
#if NH_MV
  m_pcPic->setLayerId( nalu.m_nuhLayerId );
  m_pcPic->setViewId ( getViewId() );
#if NH_3D
  m_pcPic->setViewIndex( getViewIndex() );
  m_pcPic->setIsDepth  ( getIsDepth  () );
  pcSlice->setIvPicLists( m_ivPicLists );         
#endif
#endif

  if (!pcSlice->getDependentSliceSegmentFlag())
  {
    pcSlice->checkCRA(pcSlice->getRPS(), m_pocCRA, m_associatedIRAPType, m_cListPic );
    // Set reference list
#if NH_MV    
    std::vector< TComPic* > tempRefPicLists[2];
    std::vector< Bool     > usedAsLongTerm [2];
    Int       numPocTotalCurr;

    pcSlice->getTempRefPicLists( m_cListPic, m_refPicSetInterLayer0, m_refPicSetInterLayer1, tempRefPicLists, usedAsLongTerm, numPocTotalCurr);
    pcSlice->setRefPicList     ( tempRefPicLists, usedAsLongTerm, numPocTotalCurr, true ); 
#if H_3D
    pcSlice->setDefaultRefView();
#endif
#if H_3D_ARP
    pcSlice->setARPStepNum(m_ivPicLists);
    if( pcSlice->getARPStepNum() > 1 )
    {
      // GT: This seems to be broken, not all nuh_layer_ids are necessarily present
      for(Int iLayerId = 0; iLayerId < nalu.m_nuhLayerId; iLayerId ++ )
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

#if NH_3D
    pcSlice->checkInCompPredRefLayers(); 
#if H_3D_IV_MERGE
#if H_3D_FCO
    //assert( !getIsDepth() );
#else
    assert( !getIsDepth() || ( pcSlice->getTexturePic() != 0 ) );
#endif
#endif    
#endif
#if NH_MV
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
    {
      pcSlice->generateAlterRefforTMVP();
    }
#endif
  }

  m_pcPic->setCurrSliceIdx(m_uiSliceIdx);
  if(pcSlice->getSPS()->getScalingListFlag())
  {
    TComScalingList scalingList;
    if(pcSlice->getPPS()->getScalingListPresentFlag())
    {
      scalingList = pcSlice->getPPS()->getScalingList();
    }
    else if (pcSlice->getSPS()->getScalingListPresentFlag())
    {
      scalingList = pcSlice->getSPS()->getScalingList();
    }
    else
    {
      scalingList.setDefaultScalingList();
    }
    m_cTrQuant.setScalingListDec(scalingList);
    m_cTrQuant.setUseScalingList(true);
  }
  else
  {
    const Int maxLog2TrDynamicRange[MAX_NUM_CHANNEL_TYPE] =
    {
        pcSlice->getSPS()->getMaxLog2TrDynamicRange(CHANNEL_TYPE_LUMA),
        pcSlice->getSPS()->getMaxLog2TrDynamicRange(CHANNEL_TYPE_CHROMA)
    };
    m_cTrQuant.setFlatScalingList(maxLog2TrDynamicRange, pcSlice->getSPS()->getBitDepths());
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
  m_cGopDecoder.decompressSlice(&(nalu.getBitstream()), m_pcPic);

#if NH_3D
  if( m_pcCamParsCollector )
  {
    m_pcCamParsCollector->setSlice( pcSlice );
  }
#endif

  m_bFirstSliceInPicture = false;
  m_uiSliceIdx++;

  return false;
}

Void TDecTop::xDecodeVPS(const std::vector<UChar> &naluData)
{
  TComVPS* vps = new TComVPS();

  m_cEntropyDecoder.decodeVPS( vps );
  m_parameterSetManager.storeVPS(vps, naluData);
}

Void TDecTop::xDecodeSPS(const std::vector<UChar> &naluData)
{
  TComSPS* sps = new TComSPS();
#if NH_MV
  sps->setLayerId( getLayerId() ); 
#endif
#if O0043_BEST_EFFORT_DECODING
  sps->setForceDecodeBitDepth(m_forceDecodeBitDepth);
#endif
#if NH_3D
  // GT: Please don't add parsing dependency of SPS from VPS here again!!!
#endif
  m_cEntropyDecoder.decodeSPS( sps );
  m_parameterSetManager.storeSPS(sps, naluData);
}

Void TDecTop::xDecodePPS(const std::vector<UChar> &naluData)
{
  TComPPS* pps = new TComPPS();
#if NH_MV
  pps->setLayerId( getLayerId() ); 
#endif
#if H_3D
  // GT: Please don't add parsing dependency of SPS from VPS here again!!!
#endif
#if NH_3D_DLT
  // create mapping from depth layer indexes to layer ids
  Int j=0;
  for( Int i=0; i<=m_parameterSetManager.getFirstVPS()->getMaxLayersMinus1(); i++ )
  {
    Int layerId = m_parameterSetManager.getFirstVPS()->getLayerIdInNuh(i);
    if( m_parameterSetManager.getFirstVPS()->getDepthId(layerId) )
      pps->getDLT()->setDepthIdxToLayerId(j++, i);
  }
#endif
  m_cEntropyDecoder.decodePPS( pps );

  m_parameterSetManager.storePPS( pps, naluData);
}

#if NH_MV
Bool TDecTop::decode(InputNALUnit& nalu, Int& iSkipFrame, Int& iPOCLastDisplay, Bool newLayerFlag, Bool& sliceSkippedFlag )
#else
Bool TDecTop::decode(InputNALUnit& nalu, Int& iSkipFrame, Int& iPOCLastDisplay)
#endif
{
#if !NH_MV
  // ignore all NAL units of layers > 0
  if (nalu.m_nuhLayerId > 0)
  {
    fprintf (stderr, "Warning: found NAL unit with nuh_layer_id equal to %d. Ignoring.\n", nalu.m_nuhLayerId);
    return false;
  }
#endif
  // Initialize entropy decoder
  m_cEntropyDecoder.setEntropyDecoder (&m_cCavlcDecoder);
  m_cEntropyDecoder.setBitstream      (&(nalu.getBitstream()));

  switch (nalu.m_nalUnitType)
  {
    case NAL_UNIT_VPS:
      xDecodeVPS(nalu.getBitstream().getFifo());
#if NH_MV
      m_isLastNALWasEos = false;
#endif
      return false;

    case NAL_UNIT_SPS:
      xDecodeSPS(nalu.getBitstream().getFifo());
      return false;

    case NAL_UNIT_PPS:
      xDecodePPS(nalu.getBitstream().getFifo());
      return false;

    case NAL_UNIT_PREFIX_SEI:
      // Buffer up prefix SEI messages until SPS of associated VCL is known.
      m_prefixSEINALUs.push_back(new InputNALUnit(nalu));
      return false;

    case NAL_UNIT_SUFFIX_SEI:
#if NH_MV
      if ( nalu.m_nalUnitType == NAL_UNIT_SUFFIX_SEI )
      {
        assert( m_isLastNALWasEos == false );
      }
#endif
      if (m_pcPic)
      {
#if NH_MV
        m_seiReader.parseSEImessage( &(nalu.getBitstream()), m_pcPic->getSEIs(), nalu.m_nalUnitType, m_parameterSetManager.getActiveSPS( getLayerId() ), m_pDecodedSEIOutputStream );
#else
        m_seiReader.parseSEImessage( &(nalu.getBitstream()), m_pcPic->getSEIs(), nalu.m_nalUnitType, m_parameterSetManager.getActiveSPS(), m_pDecodedSEIOutputStream );
#endif
      }
      else
      {
        printf ("Note: received suffix SEI but no picture currently active.\n");
      }
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
#if NH_MV
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
#if NH_MV
      assert( m_isLastNALWasEos == false );
      //Check layer id of the nalu. if it is not 0, give a warning message and just return without doing anything.
      if (nalu.m_nuhLayerId > 0)
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
      m_prevSliceSkipped = false;
      m_skippedPOC = 0;
      return false;

    case NAL_UNIT_ACCESS_UNIT_DELIMITER:
      {
        AUDReader audReader;
        UInt picType;
        audReader.parseAccessUnitDelimiter(&(nalu.getBitstream()),picType);
        printf ("Note: found NAL_UNIT_ACCESS_UNIT_DELIMITER\n");
      return false;
      }

    case NAL_UNIT_EOB:
      return false;

    case NAL_UNIT_FILLER_DATA:
      {
        FDReader fdReader;
        UInt size;
        fdReader.parseFillerData(&(nalu.getBitstream()),size);
        printf ("Note: found NAL_UNIT_FILLER_DATA with %u bytes payload.\n", size);
#if NH_MV
      assert( m_isLastNALWasEos == false );
#endif
      return false;
      }

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
      printf ("Note: found reserved VCL NAL unit.\n");
      xParsePrefixSEIsForUnknownVCLNal();
      return false;

    case NAL_UNIT_RESERVED_NVCL41:
    case NAL_UNIT_RESERVED_NVCL42:
    case NAL_UNIT_RESERVED_NVCL43:
    case NAL_UNIT_RESERVED_NVCL44:
    case NAL_UNIT_RESERVED_NVCL45:
    case NAL_UNIT_RESERVED_NVCL46:
    case NAL_UNIT_RESERVED_NVCL47:
      printf ("Note: found reserved NAL unit.\n");
      return false;
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
      printf ("Note: found unspecified NAL unit.\n");
      return false;
    default:
      assert (0);
      break;
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
#if NH_MV
Bool TDecTop::isRandomAccessSkipPicture(Int& iSkipFrame,  Int& iPOCLastDisplay, const TComVPS* vps)
#else
Bool TDecTop::isRandomAccessSkipPicture(Int& iSkipFrame,  Int& iPOCLastDisplay )
#endif
{
  if (iSkipFrame)
  {
    iSkipFrame--;   // decrement the counter
    return true;
  }
#if NH_MV
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

#if NH_MV
      if ( xAllRefLayersInitilized( vps ) )
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
#if NH_MV
      if ( xAllRefLayersInitilized( vps) )
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
#if NH_MV
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
      if(!m_warningMessageSkipPicture)
      {
        printf("\nWarning: this is not a valid random access point and the data is discarded until the first CRA picture");
        m_warningMessageSkipPicture = true;
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
  // if we reach here, then the picture is not skipped.
#if NH_MV
  return !m_layerInitilizedFlag[ getLayerId() ]; 
#else
  return false;
#endif
}
#if NH_MV
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

Bool TDecTop::xAllRefLayersInitilized( const TComVPS* vps )
{
  Bool allRefLayersInitilizedFlag = true;   
  for (Int i = 0; i < vps->getNumDirectRefLayers( getLayerId()  ); i++ )
  {
    Int refLayerId = vps->getIdDirectRefLayer( m_layerId, i ); 
    allRefLayersInitilizedFlag = allRefLayersInitilizedFlag && m_layerInitilizedFlag[ refLayerId ]; 
  }

  return allRefLayersInitilizedFlag;
}


Void TDecTop::initFromActiveVps( const TComVPS* vps )
{
  if ( m_targetOlsIdx == -1 )
  {
    // Not normative! Corresponds to specification by "External Means". (Should be set equal to 0, when no external means available. ) 
    m_targetOlsIdx = vps->getVpsNumLayerSetsMinus1(); 
  }
#if NH_3D
  // Set profile
  Int lsIdx = vps->olsIdxToLsIdx( m_targetOlsIdx );
  Int lIdx = -1; 
  for (Int j = 0; j < vps->getNumLayersInIdList( lsIdx ); j++ )
  {
    if ( vps->getLayerSetLayerIdList( lsIdx, j ) == getLayerId() )
    {
      lIdx = j; 
      break; 
    }        
  }
  assert( lIdx != -1 ); 

  Int profileIdc = vps->getPTL( vps->getProfileTierLevelIdx( m_targetOlsIdx, lIdx ) )->getGeneralPTL()->getProfileIdc();
  assert( profileIdc == 1 || profileIdc == 6 || profileIdc == 8 ); 
  m_profileIdc = profileIdc;   
#endif
}
#endif

//! \}
