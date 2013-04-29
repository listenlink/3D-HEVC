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



/** \file     TComDepthMapGenerator.cpp
    \brief    depth map generator class
*/



#include "CommonDef.h"
#include "TComDepthMapGenerator.h"


#if DEPTH_MAP_GENERATION


TComDepthMapGenerator::TComDepthMapGenerator()
{
  m_bCreated            = false;
  m_bInit               = false;
  m_bDecoder            = false;
  m_pcPrediction        = 0;
  m_pcSPSAccess         = 0;
  m_pcAUPicAccess       = 0;
  m_uiMaxDepth          = 0;
  m_uiOrgDepthBitDepth  = 0;
  m_ppcYuv              = 0;
  m_ppcCU               = 0;
}

TComDepthMapGenerator::~TComDepthMapGenerator()
{
  destroy ();
  uninit  ();
}

Void
TComDepthMapGenerator::create( Bool bDecoder, UInt uiPicWidth, UInt uiPicHeight, UInt uiMaxCUDepth, UInt uiMaxCUWidth, UInt uiMaxCUHeight, UInt uiOrgBitDepth )
{
  destroy();
  m_bDecoder            = bDecoder;
  m_uiMaxDepth          = uiMaxCUDepth;
  m_uiOrgDepthBitDepth  = uiOrgBitDepth;
  m_ppcYuv              = new TComYuv*    [ m_uiMaxDepth ];
  m_ppcCU               = new TComDataCU* [ m_uiMaxDepth ];
  for( UInt uiDepth = 0; uiDepth < m_uiMaxDepth; uiDepth++ )
  {
    UInt  uiNumPart = 1 << ( ( m_uiMaxDepth - uiDepth ) << 1 );
    UInt  uiWidth   = uiMaxCUWidth  >> uiDepth;
    UInt  uiHeight  = uiMaxCUHeight >> uiDepth;

    m_ppcYuv[ uiDepth ] = new TComYuv;    m_ppcYuv[ uiDepth ]->create(            uiWidth, uiHeight       );
    m_ppcCU [ uiDepth ] = new TComDataCU; m_ppcCU [ uiDepth ]->create( uiNumPart, uiWidth, uiHeight, true );
  }
  m_cTmpPic.create( uiPicWidth, uiPicHeight, uiMaxCUWidth, uiMaxCUHeight, uiMaxCUDepth );
  xSetChroma( &m_cTmpPic, ( 1 << uiOrgBitDepth ) >> 1 );
  m_bCreated    = true;
}

Void
TComDepthMapGenerator::destroy()
{
  if( m_bCreated )
  {
    m_bCreated    = false;
    for( UInt uiDepth = 0; uiDepth < m_uiMaxDepth; uiDepth++ )
    {
      m_ppcYuv[ uiDepth ]->destroy(); delete m_ppcYuv[ uiDepth ]; m_ppcYuv[ uiDepth ] = 0;
      m_ppcCU [ uiDepth ]->destroy(); delete m_ppcCU [ uiDepth ]; m_ppcCU [ uiDepth ] = 0;
    }
    delete [] m_ppcYuv; m_ppcYuv = 0;
    delete [] m_ppcCU;  m_ppcCU  = 0;
    m_cTmpPic.destroy();
    m_uiMaxDepth          = 0;
    m_uiOrgDepthBitDepth  = 0;
    m_bDecoder            = false;
  }
}

Void
TComDepthMapGenerator::init( TComPrediction* pcPrediction, TComSPSAccess* pcSPSAccess, TComAUPicAccess* pcAUPicAccess )
{
  AOF( pcPrediction  );
  AOF( pcSPSAccess   );
  AOF( pcAUPicAccess );
  uninit();
  m_pcPrediction  = pcPrediction;
  m_pcSPSAccess   = pcSPSAccess;
  m_pcAUPicAccess = pcAUPicAccess;
  m_bInit         = true;
}

Void
TComDepthMapGenerator::uninit()
{
  if( m_bInit )
  {
    m_bInit         = false;
    m_pcPrediction  = 0;
    m_pcSPSAccess   = 0;
    m_pcAUPicAccess = 0;
  }
}

Void  
TComDepthMapGenerator::initViewComponent( TComPic* pcPic )
{
  AOF  ( m_bCreated && m_bInit );
  AOF  ( pcPic );
  AOT  ( pcPic->getSPS()->getViewId() && !pcPic->getSPS()->isDepth() && pcPic->getPOC() && pcPic->getSPS()->getPredDepthMapGeneration() != m_pcSPSAccess->getPdm() );
  m_bPDMAvailable = false;
  m_uiCurrViewId  = pcPic->getSPS()->getViewId();

  // update SPS list and AU pic list and set depth map generator in SPS
  m_pcSPSAccess  ->addSPS( pcPic->getSPS() );
  m_pcAUPicAccess->addPic( pcPic );
  pcPic->getSPS()->setDepthMapGenerator( this );

  // check whether we have depth data or don't use pred depth prediction
  ROFVS( pcPic->getSPS()->getViewId() );
  ROTVS( pcPic->getSPS()->isDepth  () );
  ROFVS( m_pcSPSAccess->getPdm     () );

  // set basic SPS parameters
  const Int iDisparityDir = 1; // 1 or -1, depending on the usage of disparity vectors
  TComSPS*  pcSPS         = pcPic->getSPS                 ();
  Int       iVOI          = pcSPS->getViewOrderIdx        ();
  UInt      uiPdmPrec     = pcSPS->getPdmPrecision        ();
  UInt      uiCamPrec     = pcSPS->getCamParPrecision     ();
  Bool      bInSlice      = pcSPS->hasCamParInSliceHeader ();
  Int       iScaleVOI01   = ( 1 << ( uiPdmPrec + PDM_INTER_CALC_SHIFT + PDM_VIRT_DEPTH_PRECISION - 2 ) );

  // check availability of base views and set base id list
  std::vector<Int>  aiAbsDeltaVOI;
  for( UInt uiBaseId = 0; uiBaseId < m_uiCurrViewId; uiBaseId++ )
  {
    TComSPS*  pcBaseSPS     = m_pcSPSAccess  ->getSPS( uiBaseId );
    TComPic*  pcBasePic     = m_pcAUPicAccess->getPic( uiBaseId );
    AOF( pcBaseSPS != 0 && pcBasePic != 0 );
    Int       iDeltaVOI     = iVOI - pcBaseSPS->getViewOrderIdx();
    Int       iAbsDeltaVOI  = ( iDeltaVOI < 0 ? -iDeltaVOI : iDeltaVOI ); 
    AOT( iAbsDeltaVOI == 0 );
    aiAbsDeltaVOI.push_back( iAbsDeltaVOI );
  }
  m_auiBaseIdList.clear();
  while( (UInt)m_auiBaseIdList.size() < m_uiCurrViewId )
  {
    Int       iMinAbsDelta  = MAX_INT;
    UInt      uiNextBaseId  = MAX_NUMBER_VIEWS;
    for( UInt uiBaseId = 0; uiBaseId < m_uiCurrViewId; uiBaseId++ )
    {
      if( aiAbsDeltaVOI[ uiBaseId ] > 0 && aiAbsDeltaVOI[ uiBaseId ] <= iMinAbsDelta )
      {
        iMinAbsDelta  = aiAbsDeltaVOI[ uiBaseId ];
        uiNextBaseId  = uiBaseId;
      }
    }
    m_auiBaseIdList.push_back( uiNextBaseId );
    aiAbsDeltaVOI[ uiNextBaseId ] = 0;
  }

  // check availability of prediction depth map
  if( m_uiCurrViewId )
  {
    Bool      bCheckVId1  = ( m_uiCurrViewId > 1 && m_auiBaseIdList[0] == 0 );
    UInt      uiBaseVId   = ( bCheckVId1 ? 1 : m_auiBaseIdList[0] );
    TComPic*  pcBasePic   = m_pcAUPicAccess->getPic( uiBaseVId );
    SliceType eSliceType  = pcBasePic->getCurrSlice()->getSliceType();
    Bool      bNoRAPdm    = ( pcPic->getSPS()->getPredDepthMapGeneration() == 1 );
    m_bPDMAvailable       = ( eSliceType != I_SLICE || !bNoRAPdm );
  }

  // update disparity depth conversion parameters
  for( UInt uiBaseId = 0; uiBaseId < m_uiCurrViewId; uiBaseId++ )
  {
    TComSPS*  pcBaseSPS   = m_pcSPSAccess->getSPS( uiBaseId );
    Int       iBaseVOI    = pcBaseSPS->getViewOrderIdx();

    // disparity -> virtual depth
    Int       iVNominator = ( 1 << PDM_LOG4_SCALE_DENOMINATOR ) + pcSPS->getPdmScaleNomDelta()[ uiBaseId ];
    Int       iVDiv       = iVOI - iBaseVOI;
    Int       iVAdd       = ( iVDiv > 0 ? iVDiv / 2 : -iVDiv / 2 );
    Int       iVScalePred = ( iScaleVOI01 + iVAdd ) / iVDiv;
    Int       iVShift     = PDM_INTER_CALC_SHIFT;
    Int       iVScale     = Int( ( (Int64)iVNominator * (Int64)iVScalePred + (Int64)( ( 1 << PDM_LOG4_SCALE_DENOMINATOR ) >> 1 ) ) >> PDM_LOG4_SCALE_DENOMINATOR );
    Int       iVOffset    = pcSPS->getPdmOffset()[ uiBaseId ] << PDM_OFFSET_SHIFT;
    m_aaiConvParsDisparity2VirtDepth[ uiBaseId ][ 0 ] = iDisparityDir * iVScale;
    m_aaiConvParsDisparity2VirtDepth[ uiBaseId ][ 1 ] = iDisparityDir * iVOffset + ( ( 1 << iVShift ) >> 1 );
    m_aaiConvParsDisparity2VirtDepth[ uiBaseId ][ 2 ] = iVShift;

    // virtual depth -> disparity
    Int       iVInvAdd    = ( iVScale > 0 ? iVScale / 2 : -iVScale / 2 );
    Int       iVInvScale  = Int( ( ( Int64(  1        ) << ( iVShift << 1 ) ) + iVInvAdd ) / Int64( iVScale ) );
    Int       iVInvOffset = Int( ( ( Int64( -iVOffset ) <<   iVShift        ) + iVInvAdd ) / Int64( iVScale ) );
    m_aaiConvParsVirtDepth2Disparity[ uiBaseId ][ 0 ] = iDisparityDir * iVInvScale;
    m_aaiConvParsVirtDepth2Disparity[ uiBaseId ][ 1 ] = iDisparityDir * iVInvOffset + ( ( 1 << iVShift ) >> 1 );
    m_aaiConvParsVirtDepth2Disparity[ uiBaseId ][ 2 ] = iVShift;

    // coded depth -> virtual depth
    Int       iCScale     = ( bInSlice ? pcPic->getCurrSlice()->getCodedScale () : pcSPS->getCodedScale () )[ uiBaseId ];
    Int       iCOffset    = ( bInSlice ? pcPic->getCurrSlice()->getCodedOffset() : pcSPS->getCodedOffset() )[ uiBaseId ] << m_uiOrgDepthBitDepth;
    Int       iCShift     = m_uiOrgDepthBitDepth + uiCamPrec + 1 - 2;
    Int       iCVShift    = PDM_INTER_CALC_SHIFT;
    Int       iTmpShift   = iVShift + iCShift - iCVShift; AOF( iTmpShift >= 0 )
    Int       iCVScale    = Int( ( Int64( iVScale ) * Int64( iCScale  ) + Int64( ( 1 << iTmpShift ) >> 1 ) ) >> iTmpShift );
    Int       iCVOffset   = Int( ( Int64( iVScale ) * Int64( iCOffset ) + Int64( ( 1 << iTmpShift ) >> 1 ) ) >> iTmpShift );
    iTmpShift             = iVShift - iCVShift;           AOF( iTmpShift >= 0 )
    iCVOffset            +=      ( iVOffset                             +      ( ( 1 << iTmpShift ) >> 1 ) ) >> iTmpShift;
    m_aaiConvParsOrigDepth2VirtDepth[ uiBaseId ][ 0 ] = iCVScale;
    m_aaiConvParsOrigDepth2VirtDepth[ uiBaseId ][ 1 ] = iCVOffset + ( ( 1 << iCVShift ) >> 1 );
    m_aaiConvParsOrigDepth2VirtDepth[ uiBaseId ][ 2 ] = iCVShift;

    // virtual depth -> coded depth
    Int       iCVAdd      = ( iCVScale > 0 ? iCVScale / 2 : -iCVScale / 2 );
    Int       iCVInvScale = Int( ( ( Int64(  1         ) << ( iCVShift << 1 ) ) + iCVAdd ) / Int64( iCVScale ) );
    Int       iCVInvOffset= Int( ( ( Int64( -iCVOffset ) <<   iCVShift        ) + iCVAdd ) / Int64( iCVScale ) );
    m_aaiConvParsVirtDepth2OrigDepth[ uiBaseId ][ 0 ] = iCVInvScale;
    m_aaiConvParsVirtDepth2OrigDepth[ uiBaseId ][ 1 ] = iCVInvOffset + ( ( 1 << iCVShift ) >> 1 );
    m_aaiConvParsVirtDepth2OrigDepth[ uiBaseId ][ 2 ] = iCVShift;
  }

  if( m_uiCurrViewId > 0 )
  {
    UInt      uiBaseId    = 0;
    UInt      uiBaseVOI   = 0; // per definition
    Int       iVNominator = ( 1 << PDM_LOG4_SCALE_DENOMINATOR ) + pcSPS->getPdmScaleNomDelta()[ uiBaseId ];
    Int       iVDiv       = iVOI - uiBaseVOI;
    Int       iVAdd       = ( iVDiv > 0 ? iVDiv / 2 : -iVDiv / 2 );
    Int       iVScalePred = ( iScaleVOI01 + iVAdd ) / iVDiv;
    Int       iVShift     = PDM_INTER_CALC_SHIFT;
    Int       iVScale     = Int( ( (Int64)iVNominator * (Int64)iVScalePred + (Int64)( ( 1 << PDM_LOG4_SCALE_DENOMINATOR ) >> 1 ) ) >> PDM_LOG4_SCALE_DENOMINATOR );
    Int       iVOffset    = pcSPS->getPdmOffset()[ uiBaseId ] << PDM_OFFSET_SHIFT;

    // coded depth -> virtual depth (current view)
    Int       iCScale     = ( bInSlice ? pcPic->getCurrSlice()->getInvCodedScale () : pcSPS->getInvCodedScale () )[ uiBaseId ];
    Int       iCOffset    = ( bInSlice ? pcPic->getCurrSlice()->getInvCodedOffset() : pcSPS->getInvCodedOffset() )[ uiBaseId ] << m_uiOrgDepthBitDepth;
    Int       iCShift     = m_uiOrgDepthBitDepth + uiCamPrec + 1 - 2;
    Int       iCVShift    = PDM_INTER_CALC_SHIFT;
    Int       iTmpShift   = iVShift + iCShift - iCVShift; AOF( iTmpShift >= 0 )
    Int       iCVScale    = Int( ( Int64( -iVScale ) * Int64( iCScale  ) + Int64( ( 1 << iTmpShift ) >> 1 ) ) >> iTmpShift );
    Int       iCVOffset   = Int( ( Int64( -iVScale ) * Int64( iCOffset ) + Int64( ( 1 << iTmpShift ) >> 1 ) ) >> iTmpShift );
    iTmpShift             = iVShift - iCVShift;           AOF( iTmpShift >= 0 )
    iCVOffset            +=      ( iVOffset                             +      ( ( 1 << iTmpShift ) >> 1 ) ) >> iTmpShift;
    m_aaiConvParsOrigDepth2VirtDepth[ m_uiCurrViewId ][ 0 ] = iCVScale;
    m_aaiConvParsOrigDepth2VirtDepth[ m_uiCurrViewId ][ 1 ] = iCVOffset + ( ( 1 << iCVShift ) >> 1 );
    m_aaiConvParsOrigDepth2VirtDepth[ m_uiCurrViewId ][ 2 ] = iCVShift;

    // virtual depth -> coded depth
    Int       iCVAdd      = ( iCVScale > 0 ? iCVScale / 2 : -iCVScale / 2 );
    Int       iCVInvScale = Int( ( ( Int64(  1         ) << ( iCVShift << 1 ) ) + iCVAdd ) / Int64( iCVScale ) );
    Int       iCVInvOffset= Int( ( ( Int64( -iCVOffset ) <<   iCVShift        ) + iCVAdd ) / Int64( iCVScale ) );
    m_aaiConvParsVirtDepth2OrigDepth[ m_uiCurrViewId ][ 0 ] = iCVInvScale;
    m_aaiConvParsVirtDepth2OrigDepth[ m_uiCurrViewId ][ 1 ] = iCVInvOffset + ( ( 1 << iCVShift ) >> 1 );
    m_aaiConvParsVirtDepth2OrigDepth[ m_uiCurrViewId ][ 2 ] = iCVShift;
  }
  else if( pcPic->getPOC() == 0 )
  { // set dummy values
    m_aaiConvParsOrigDepth2VirtDepth[ m_uiCurrViewId ][ 0 ] = 0;
    m_aaiConvParsOrigDepth2VirtDepth[ m_uiCurrViewId ][ 1 ] = 0;
    m_aaiConvParsOrigDepth2VirtDepth[ m_uiCurrViewId ][ 2 ] = 0;
    m_aaiConvParsVirtDepth2OrigDepth[ m_uiCurrViewId ][ 0 ] = 0;
    m_aaiConvParsVirtDepth2OrigDepth[ m_uiCurrViewId ][ 1 ] = 0;
    m_aaiConvParsVirtDepth2OrigDepth[ m_uiCurrViewId ][ 2 ] = 0;
  }


#if 0 // print out for debugging
  if( m_uiCurrViewId )
  {
    printf( "\n\ninit slice of view %d (VOI=%2d):\n===============================\n", m_uiCurrViewId, iVOI );
    {
      printf( "\n  disparity -> virtual depth:\n" );
      for( UInt uiBaseId = 0; uiBaseId < m_uiCurrViewId; uiBaseId++ )
      {
        Int*    pP = m_aaiConvParsDisparity2VirtDepth[ uiBaseId ];
        Double  dF = 1.0 / Double( 1 << pP[ 2 ] );
        Double  dA = dF  * Double( pP[ 0 ] );
        Double  dB = dF  * Double( pP[ 1 ] - ( ( 1 << pP[ 2 ] ) >> 1 ) );
        printf( "    BId=%d:    a = %10.4lf    b = %10.4lf\n", uiBaseId, dA, dB );
      }
      printf( "\n  virtual depth -> disparity:\n" );
      for( UInt uiBaseId = 0; uiBaseId < m_uiCurrViewId; uiBaseId++ )
      {
        Int*    pP = m_aaiConvParsVirtDepth2Disparity[ uiBaseId ];
        Double  dF = 1.0 / Double( 1 << pP[ 2 ] );
        Double  dA = dF  * Double( pP[ 0 ] );
        Double  dB = dF  * Double( pP[ 1 ] - ( ( 1 << pP[ 2 ] ) >> 1 ) );
        printf( "    BId=%d:    a = %10.4lf    b = %10.4lf\n", uiBaseId, dA, dB );
      }
      printf( "\n  original depth -> virtual depth:\n" );
      for( UInt uiBaseId = 0; uiBaseId <= m_uiCurrViewId; uiBaseId++ )
      {
        Int*    pP = m_aaiConvParsOrigDepth2VirtDepth[ uiBaseId ];
        Double  dF = 1.0 / Double( 1 << pP[ 2 ] );
        Double  dA = dF  * Double( pP[ 0 ] );
        Double  dB = dF  * Double( pP[ 1 ] - ( ( 1 << pP[ 2 ] ) >> 1 ) );
        printf( "    VId=%d:    a = %10.4lf    b = %10.4lf\n", uiBaseId, dA, dB );
      }
      printf( "\n  virtual depth -> original depth:\n" );
      for( UInt uiBaseId = 0; uiBaseId <= m_uiCurrViewId; uiBaseId++ )
      {
        Int*    pP = m_aaiConvParsVirtDepth2OrigDepth[ uiBaseId ];
        Double  dF = 1.0 / Double( 1 << pP[ 2 ] );
        Double  dA = dF  * Double( pP[ 0 ] );
        Double  dB = dF  * Double( pP[ 1 ] - ( ( 1 << pP[ 2 ] ) >> 1 ) );
        printf( "    VId=%d:    a = %10.4lf    b = %10.4lf\n", uiBaseId, dA, dB );
      }
      printf( "\n" );
    }
  }
#endif
}


Bool
TComDepthMapGenerator::predictDepthMap( TComPic* pcPic )
{
  AOF  ( m_bCreated && m_bInit );
  AOF  ( pcPic );
  ROTRS( pcPic->getSPS()->isDepth(),  true );
  ROFRS( m_pcSPSAccess->getPdm(),     true );
  AOF  ( pcPic->getPredDepthMap() );
  AOF  ( pcPic->getSPS()->getViewId() == m_uiCurrViewId );

#if PDM_OUTPUT_PRED_DEPTH_MAP
  Char acFilenameBase[1024];
  ::sprintf( acFilenameBase, "PDM_%s_Prd", ( m_bDecoder ? "Dec" : "Enc" ) );
#endif

  Bool bUndefined = true;
  if( m_uiCurrViewId )
  {
    AOF( m_auiBaseIdList.size() );
    UInt        uiBaseId    = m_auiBaseIdList[ 0 ];
    TComPic*    pcBasePic   = m_pcAUPicAccess->getPic( uiBaseId );
    AOF( pcBasePic );

    if( m_uiCurrViewId == 1 )
    {
      if( pcBasePic->getPOC() == 0 )
      {
        xClearDepthMap( pcBasePic );
      }
#if PDM_OUTPUT_PRED_DEPTH_MAP
      dumpDepthMap( pcBasePic, acFilenameBase );
#endif
    }

    Bool  bLoadDepth  = ( m_pcSPSAccess->getPdm() == 2 );
    if( m_pcSPSAccess->getPdm() > 2 )
    {
      bLoadDepth = ( pcBasePic->getCurrSlice()->getSliceType() == I_SLICE );
    }

    if( bLoadDepth)
    { // load coded depth of base view
      TComPic*  pcBaseDepth = m_pcAUPicAccess->getPic( uiBaseId, true );
      AOF( pcBaseDepth );
      AOF( pcBaseDepth->getPicYuvRec() );
      AOF( pcBaseDepth->getPicYuvRec()->getWidth () == pcBasePic->getPredDepthMap()->getWidth () );
      AOF( pcBaseDepth->getPicYuvRec()->getHeight() == pcBasePic->getPredDepthMap()->getHeight() );
      Int       iWidth      = pcBasePic  ->getPredDepthMap()->getWidth    ();
      Int       iHeight     = pcBasePic  ->getPredDepthMap()->getHeight   ();
      Int       iDesStride  = pcBasePic  ->getPredDepthMap()->getStride   ();
      Int       iSrcStride  = pcBaseDepth->getPicYuvRec   ()->getStride   ();
      Pel*      pDesSamples = pcBasePic  ->getPredDepthMap()->getLumaAddr ( 0 );
      Pel*      pSrcSamples = pcBaseDepth->getPicYuvRec   ()->getLumaAddr ( 0 );
      for( Int iY = 0; iY < iHeight; iY++, pSrcSamples += iSrcStride, pDesSamples += iDesStride )
      {
        for( Int iX = 0; iX < iWidth; iX++ )
        {
          pDesSamples[ iX ] = xGetVirtDepthFromOrigDepth( uiBaseId, pSrcSamples[ iX ] );
        }
      }
    }

    // convert depth of base view to current view
    bUndefined = xConvertDepthMapRef2Curr( pcPic, pcBasePic );

#if PDM_OUTPUT_PRED_DEPTH_MAP
    dumpDepthMap( pcPic, acFilenameBase );
#endif
  }
  else
  {
    xClearDepthMap( pcPic );
  }
  return bUndefined;
}


Void
TComDepthMapGenerator::updateDepthMap( TComPic* pcPic )
{
  AOF  ( m_bCreated && m_bInit );
  AOF  ( pcPic );
  ROTVS( pcPic->getSPS()->isDepth() );
  ROFVS( m_pcSPSAccess->getPdm() == 1 || m_pcSPSAccess->getPdm() == 3 );
  AOF  ( pcPic->getPredDepthMap() );
  AOF  ( pcPic->getSPS()->getViewId() == m_uiCurrViewId );

#if PDM_OUTPUT_PRED_DEPTH_MAP
  Char acFilenameBase[1024];
  ::sprintf( acFilenameBase, "PDM_%s_Upd", ( m_bDecoder ? "Dec" : "Enc" ) );
#endif

  // predict depth map using current coding symbols
  xPredictDepthMap( pcPic );
#if PDM_OUTPUT_PRED_DEPTH_MAP
  if( m_uiCurrViewId )
  {
    dumpDepthMap( pcPic, acFilenameBase );
  }
#endif

  // generate base depth map
  if( m_uiCurrViewId == 1 )
  {
    TComPic* pcBasePic = m_pcAUPicAccess->getPic( 0 );
    AOF( pcBasePic );
    xConvertDepthMapCurr2Ref( pcBasePic, pcPic );
#if PDM_OUTPUT_PRED_DEPTH_MAP
    dumpDepthMap( pcBasePic, acFilenameBase );
#endif
  }
}


Void
TComDepthMapGenerator::dumpDepthMap( TComPic* pcPic, char* pFilenameBase )
{
  AOF( m_bCreated && m_bInit );
  AOF( pcPic );
  AOF( pFilenameBase );
  AOF( m_uiOrgDepthBitDepth == 8 + g_uiBitIncrement );
  AOF( pcPic->getSPS()->getViewId() <= m_uiCurrViewId );

  // convert to output format 
  Int         iMax        = ( 1 << m_uiOrgDepthBitDepth ) - 1;
  UInt        uiViewId    = pcPic->getSPS()->getViewId();
  TComPicYuv* pcPicYuv    = pcPic->getPredDepthMap();
  Int         iWidth      = pcPicYuv->getWidth    ();
  Int         iHeight     = pcPicYuv->getHeight   ();
  Int         iSrcStride  = pcPicYuv->getStride   ();
  Int         iDstStride  = m_cTmpPic.getStride   ();
  Pel*        pSrcSamples = pcPicYuv->getLumaAddr ( 0 );
  Pel*        pDstSamples = m_cTmpPic.getLumaAddr ( 0 );
  Int         iMidOrgDpth = ( 1 << m_uiOrgDepthBitDepth ) >> 1;
  AOF( m_cTmpPic.getWidth () == iWidth  );
  AOF( m_cTmpPic.getHeight() == iHeight );
  for( Int iY = 0; iY < iHeight; iY++, pSrcSamples += iSrcStride, pDstSamples += iDstStride )
  {
    for( Int iX = 0; iX < iWidth; iX++ )
    {
      Int iOrgDepth     = ( pSrcSamples[ iX ] != PDM_UNDEFINED_DEPTH ? xGetOrigDepthFromVirtDepth( uiViewId, pSrcSamples[ iX ] ) : iMidOrgDpth );
      pDstSamples[ iX ] = Max( 0, Min( iMax, iOrgDepth ) );
    }
  }

  // output
  Char  acFilename[1024];
  ::sprintf     ( acFilename, "%s_V%d.yuv", pFilenameBase, uiViewId );
  m_cTmpPic.dump( acFilename, ( pcPic->getPOC() != 0 )  );
}


#if HHI_INTER_VIEW_MOTION_PRED
Void  
TComDepthMapGenerator::covertOrgDepthMap( TComPic* pcPic )
{
  AOF  ( m_bCreated && m_bInit   );
  AOF  ( pcPic );
  ROFVS( pcPic->getOrgDepthMap() );
  AOF  ( pcPic->getViewIdx() );

  UInt  uiBaseId = pcPic->getViewIdx();
  Int   iWidth   = pcPic->getOrgDepthMap()->getWidth    ();
  Int   iHeight  = pcPic->getOrgDepthMap()->getHeight   ();
  Int   iStride  = pcPic->getOrgDepthMap()->getStride   ();
  Pel*  pSamples = pcPic->getOrgDepthMap()->getLumaAddr ( 0 );
  for( Int iY = 0; iY < iHeight; iY++, pSamples += iStride )
  {
    for( Int iX = 0; iX < iWidth; iX++ )
    {
      pSamples[ iX ] = xGetVirtDepthFromOrigDepth( uiBaseId, pSamples[ iX ] );
    }
  }
}
#endif

Int
TComDepthMapGenerator::getDisparity( TComPic* pcPic, Int iPosX, Int iPosY, UInt uiRefViewId )
{
  AOF( pcPic );
  AOF( pcPic->getPredDepthMap() );
  AOF( iPosX >= 0 && iPosX < pcPic->getPredDepthMap()->getWidth () );
  AOF( iPosY >= 0 && iPosY < pcPic->getPredDepthMap()->getHeight() );
  Pel*   piPdmMap    = pcPic->getPredDepthMap()->getLumaAddr( 0 );
  Int    iStride     = pcPic->getPredDepthMap()->getStride  ();
  Int    iPrdDepth   = piPdmMap[ iPosX + iPosY * iStride ];
  Int    iDisparity  = xGetDisparityFromVirtDepth( uiRefViewId, iPrdDepth );
  return iDisparity;
}



#if HHI_INTER_VIEW_MOTION_PRED
Int
TComDepthMapGenerator::getPdmMergeCandidate( TComDataCU* pcCU, UInt uiPartIdx, Int* paiPdmRefIdx, TComMv* pacPdmMv )
{
  Int  iMaxNumInterPics  = 1;
  Int  iMaxNumAllPics    = 2;

  // inter-only
  Bool abPdmAvailable[2] = {false,false};
  for( Int iRefListId = 0; iRefListId < 2; iRefListId++ )
  {
    RefPicList  eRefPicList       = RefPicList( iRefListId );
    Int         iNumRefPics       = pcCU->getSlice()->getNumRefIdx( eRefPicList );
    TComMv      cMv;
    for( Int iPdmRefIdx = 0, iInterPics = 0; iPdmRefIdx < iNumRefPics && iInterPics < iMaxNumInterPics; iPdmRefIdx++ )
    {
      if( pcCU->getSlice()->getRefPOC( eRefPicList, iPdmRefIdx ) != pcCU->getSlice()->getPOC() )
      {
        if( getPdmMvPred( pcCU, uiPartIdx, eRefPicList, iPdmRefIdx, cMv, true ) )
        {
          pcCU->clipMv( cMv );
          abPdmAvailable[ iRefListId ] = true;
          paiPdmRefIdx  [ iRefListId ] = iPdmRefIdx;
          pacPdmMv      [ iRefListId ] = cMv;
          break;
        }
        iInterPics++;
      }
    }
  }
  Int    iPdmInterDir = ( abPdmAvailable[0] ? 1 : 0 ) + ( abPdmAvailable[1] ? 2 : 0 );
  if( 0==iPdmInterDir )
  { // check all, including inter view references
    for( Int iRefListId = 0; iRefListId < 2; iRefListId++ )
    {
      RefPicList  eRefPicList = RefPicList( iRefListId );
      Int         iNumRefPics = Min( iMaxNumAllPics, pcCU->getSlice()->getNumRefIdx( eRefPicList ) );
      TComMv      cMv;
      for( Int iPdmRefIdx = 0; iPdmRefIdx < iNumRefPics; iPdmRefIdx++ )
      {
        if( getPdmMvPred( pcCU, uiPartIdx, eRefPicList, iPdmRefIdx, cMv, true ) )
        {
          pcCU->clipMv( cMv );
          abPdmAvailable[ iRefListId ] = true;
          paiPdmRefIdx  [ iRefListId ] = iPdmRefIdx;
          pacPdmMv      [ iRefListId ] = cMv;
          break;
        }
      }
    }
    iPdmInterDir = ( abPdmAvailable[0] ? 1 : 0 ) + ( abPdmAvailable[1] ? 2 : 0 );
  }
  return iPdmInterDir;
}


Bool  
TComDepthMapGenerator::getPdmMvPred( TComDataCU* pcCU, UInt uiPartIdx, RefPicList eRefPicList, Int iRefIdx, TComMv& rcMv, Bool bMerge )
{
  AOF  ( m_bCreated && m_bInit );
  AOF  ( iRefIdx >= 0 );
  AOF  ( pcCU );
  ROFRS( m_bPDMAvailable, false );

  TComSlice*    pcSlice     = pcCU->getSlice ();
  TComPic*      pcPic       = pcCU->getPic   ();
  TComSPS*      pcSPS       = pcSlice->getSPS();
  AOF  ( pcPic->getPredDepthMap() );
  AOF  ( pcSPS->getViewId() == m_uiCurrViewId );
  
  TComPic*      pcRefPic    = pcSlice->getRefPic( eRefPicList, iRefIdx );
  UInt          uiRefViewId = pcRefPic->getSPS()->getViewId();
  Int           iRefPoc     = pcRefPic->getPOC();
  Bool          bInterview  = ( uiRefViewId < m_uiCurrViewId );
  AOT(  bInterview && iRefPoc != pcSlice->getPOC() );
  AOT( !bInterview && iRefPoc == pcSlice->getPOC() );

  Bool          bPdmIView   = ( ( pcSPS->getMultiviewMvPredMode() & PDM_USE_FOR_IVIEW ) == PDM_USE_FOR_IVIEW );
  Bool          bPdmInter   = ( ( pcSPS->getMultiviewMvPredMode() & PDM_USE_FOR_INTER ) == PDM_USE_FOR_INTER );
  Bool          bPdmMerge   = ( ( pcSPS->getMultiviewMvPredMode() & PDM_USE_FOR_MERGE ) == PDM_USE_FOR_MERGE );
  ROTRS( ( bInterview && !bMerge ) && !bPdmIView, false );
  ROTRS( (!bInterview && !bMerge ) && !bPdmInter, false );
  ROTRS(                  bMerge   && !bPdmMerge, false );

  //===== get predicted depth for middle position of current PU =====  
  Int  iPrdDepth, iCurrPosX, iCurrPosY;
  Bool bAvailable  = xGetPredDepth( pcCU, uiPartIdx, iPrdDepth, &iCurrPosX, &iCurrPosY );
  AOF( bAvailable );
  
  //===== inter-view motion vector prediction =====
  if( bInterview )
  {
    Int         iDisparity  = xGetDisparityFromVirtDepth( uiRefViewId, iPrdDepth );
    rcMv.set  ( iDisparity, 0 );
    return      true;
  }
  
  //===== inter motion vector prediction =====
  for( UInt uiBId = 0; uiBId < m_uiCurrViewId; uiBId++ )
  {
    //--- get base CU/PU and check prediction mode ---
    UInt        uiBaseId    = m_auiBaseIdList[ uiBId ];
    TComPic*    pcBasePic   = m_pcAUPicAccess->getPic( uiBaseId );
    TComPicYuv* pcBasePdm   = pcBasePic->getPredDepthMap();
    TComPicYuv* pcBaseRec   = pcBasePic->getPicYuvRec   ();
    Int         iDisparity  = xGetDisparityFromVirtDepth( uiBaseId, iPrdDepth );
    Int         iBasePosX   = Clip3( 0, pcBasePdm->getWidth () - 1, iCurrPosX + ( ( iDisparity + 2 ) >> 2 ) );
    Int         iBasePosY   = Clip3( 0, pcBasePdm->getHeight() - 1, iCurrPosY                               );
    Int         iBaseCUAddr;
    Int         iBaseAbsPartIdx;
    pcBaseRec->getCUAddrAndPartIdx( iBasePosX, iBasePosY, iBaseCUAddr, iBaseAbsPartIdx );
    TComDataCU* pcBaseCU    = pcBasePic->getCU( iBaseCUAddr );
    if( pcBaseCU->getPredictionMode( iBaseAbsPartIdx ) != MODE_INTER && pcBaseCU->getPredictionMode( iBaseAbsPartIdx ) != MODE_SKIP )
    {
      continue;
    }

    for( UInt uiBaseRefListId = 0; uiBaseRefListId < 2; uiBaseRefListId++ )
    {
      RefPicList  eBaseRefPicList = RefPicList( uiBaseRefListId );
      TComMvField cBaseMvField;
      pcBaseCU->getMvField( pcBaseCU, iBaseAbsPartIdx, eBaseRefPicList, cBaseMvField );
      Int         iBaseRefIdx     = cBaseMvField.getRefIdx();
      Int         iBaseRefPoc     = ( iBaseRefIdx >= 0 ? pcBaseCU->getSlice()->getRefPic( eBaseRefPicList, iBaseRefIdx )->getPOC() : -(1<<30) );
      if( iBaseRefIdx >= 0 && iBaseRefPoc == iRefPoc )
      {
        rcMv.set( cBaseMvField.getHor(), cBaseMvField.getVer() );
        return true;
      }
    }
  }
  return false;
}


Bool  // first version 
TComDepthMapGenerator::getIViewOrgDepthMvPred( TComDataCU* pcCU, UInt uiPartIdx, RefPicList eRefPicList, Int iRefIdx, TComMv& rcMv )
{
  AOF  ( m_bCreated && m_bInit );
  AOF  ( iRefIdx >= 0 );
  AOF  ( pcCU );

  TComSlice*    pcSlice     = pcCU->getSlice ();
  TComPic*      pcPic       = pcCU->getPic   ();
  TComSPS*      pcSPS       = pcSlice->getSPS();
  AOF  ( pcSPS->getViewId() == m_uiCurrViewId );
  ROFRS( pcPic->getOrgDepthMap(),      false );
  
  TComPic*      pcRefPic    = pcSlice->getRefPic( eRefPicList, iRefIdx );
  UInt          uiRefViewId = pcRefPic->getSPS()->getViewId();
  Int           iRefPoc     = pcRefPic->getPOC();
  ROFRS( uiRefViewId < m_uiCurrViewId, false );
  AOF  ( iRefPoc    == pcSlice->getPOC() );

  //===== get predicted depth for middle position of current PU (first version) =====  
  UInt          uiPartAddr;
  Int           iWidth;
  Int           iHeight;
  pcCU->getPartIndexAndSize( uiPartIdx, uiPartAddr, iWidth, iHeight );
  TComPicYuv*   pcOrgDepthMap  = pcPic->getOrgDepthMap();
  Pel*          piOrgDepthMap  = pcOrgDepthMap->getLumaAddr ( 0 );
  Int           iCurrStride    = pcOrgDepthMap->getStride   ();
  Int           iCurrPosX;
  Int           iCurrPosY;
  pcOrgDepthMap->getTopLeftSamplePos( pcCU->getAddr(), pcCU->getZorderIdxInCU() + uiPartAddr, iCurrPosX, iCurrPosY );
  iCurrPosX                    += ( iWidth  - 1 ) >> 1;
  iCurrPosY                    += ( iHeight - 1 ) >> 1;
  Int           iPrdDepth       = piOrgDepthMap[ iCurrPosX + iCurrPosY * iCurrStride ];
  
  //===== get disparity vector =====
  Int           iDisparity      = xGetDisparityFromVirtDepth( uiRefViewId, iPrdDepth );
  rcMv.set    ( iDisparity, 0 );
  return        true;
}
#endif






/*=======================================================*
 *=====                                             =====*
 *=====     p i c t u r e   o p e r a t i o n s     =====*
 *=====                                             =====*
 *=======================================================*/

Bool
TComDepthMapGenerator::xConvertDepthMapCurr2Ref( TComPic* pcRef, TComPic* pcCur )
{
  AOF( pcCur->getSPS()->getViewId() == m_uiCurrViewId );
  AOF( pcCur->getSPS()->getViewId()  > pcRef->getSPS()->getViewId() );
  AOF( pcCur->getPredDepthMap() );
  AOF( pcRef->getPredDepthMap() );
  AOF( pcRef->getPredDepthMap()->getWidth () == pcCur->getPredDepthMap()->getWidth () );
  AOF( pcRef->getPredDepthMap()->getHeight() == pcCur->getPredDepthMap()->getHeight() );

  xClearDepthMap( pcRef );
  TComPicYuv* pcCurDepthMap =  pcCur->getPredDepthMap    ();
  TComPicYuv* pcRefDepthMap =  pcRef->getPredDepthMap    ();
  Int         iWidth        =  pcCurDepthMap->getWidth   ();
  Int         iHeight       =  pcCurDepthMap->getHeight  ();
  Int         iCurStride    =  pcCurDepthMap->getStride  ();
  Int         iRefStride    =  pcRefDepthMap->getStride  ();
  Pel*        pCurSamples   =  pcCurDepthMap->getLumaAddr( 0 );
  Pel*        pRefSamples   =  pcRefDepthMap->getLumaAddr( 0 );
  Int         iRefViewIdx   =  pcRef->getViewIdx();
  for( Int iY = 0; iY < iHeight; iY++, pCurSamples += iCurStride, pRefSamples += iRefStride )
  {
    for( Int iXCur = 0; iXCur < iWidth; iXCur++ )
    {
      Int iDepth = pCurSamples[ iXCur ];
      Int iDisp  = xGetDisparityFromVirtDepth( iRefViewIdx, iDepth );
      Int iXRef  = iXCur + ( ( iDisp + 2 ) >> 2 );
      if( iXRef >= 0 && iXRef < iWidth && iDepth > pRefSamples[ iXRef ] )
      {
        pRefSamples[ iXRef ] = iDepth;
      }
    }
  }
  Bool    bUndefined = xFillDepthMapHoles( pcRef );
  pcRefDepthMap->setBorderExtension( false );
  pcRefDepthMap->extendPicBorder   ();
  return  bUndefined;
}


Bool
TComDepthMapGenerator::xConvertDepthMapRef2Curr( TComPic* pcCur, TComPic* pcRef )
{
  AOF( pcCur->getSPS()->getViewId() == m_uiCurrViewId );
  AOF( pcCur->getSPS()->getViewId()  > pcRef->getSPS()->getViewId() );
  AOF( pcCur->getPredDepthMap() );
  AOF( pcRef->getPredDepthMap() );
  AOF( pcRef->getPredDepthMap()->getWidth () == pcCur->getPredDepthMap()->getWidth () );
  AOF( pcRef->getPredDepthMap()->getHeight() == pcCur->getPredDepthMap()->getHeight() );

  xClearDepthMap( pcCur );
  TComPicYuv* pcRefDepthMap =  pcRef->getPredDepthMap    ();
  TComPicYuv* pcCurDepthMap =  pcCur->getPredDepthMap    ();
  Int         iWidth        =  pcRefDepthMap->getWidth   ();
  Int         iHeight       =  pcRefDepthMap->getHeight  ();
  Int         iRefStride    =  pcRefDepthMap->getStride  ();
  Int         iCurStride    =  pcCurDepthMap->getStride  ();
  Pel*        pRefSamples   =  pcRefDepthMap->getLumaAddr( 0 );
  Pel*        pCurSamples   =  pcCurDepthMap->getLumaAddr( 0 );
  Int         iRefViewIdx   =  pcRef->getViewIdx();
  for( Int iY = 0; iY < iHeight; iY++, pRefSamples += iRefStride, pCurSamples += iCurStride )
  {
    for( Int iXRef = 0; iXRef < iWidth; iXRef++ )
    {
      Int iDepth = pRefSamples[ iXRef ];
      Int iDisp  = xGetDisparityFromVirtDepth( iRefViewIdx, iDepth );
      Int iXCur  = iXRef - ( ( iDisp + 2 ) >> 2 );
      if( iXCur >= 0 && iXCur < iWidth && iDepth > pCurSamples[ iXCur ] )
      {
        pCurSamples[ iXCur ] = iDepth;
      }
    }
  }
  Bool    bUndefined = xFillDepthMapHoles( pcCur );
  pcCurDepthMap->setBorderExtension( false );
  pcCurDepthMap->extendPicBorder   ();
  return  bUndefined;
}


Bool
TComDepthMapGenerator::xPredictDepthMap( TComPic* pcPic )
{
  for( UInt uiCUAddr = 0; uiCUAddr < pcPic->getPicSym()->getNumberOfCUsInFrame(); uiCUAddr++ )
  {
    TComDataCU* pcCU = pcPic->getCU( uiCUAddr );
    xPredictCUDepthMap( pcCU, 0, 0 );
  }
  Bool    bUndefined = xFillDepthMapHoles( pcPic );
  pcPic->getPredDepthMap()->setBorderExtension( false );
  pcPic->getPredDepthMap()->extendPicBorder   ();
  return  bUndefined;
}


Bool
TComDepthMapGenerator::xFillDepthMapHoles( TComPic* pcPic )
{
  Bool        bUndefined  = false;      
  TComPicYuv* pcDepthMap  = pcPic->getPredDepthMap  ();
  Int         iWidth      = pcDepthMap->getWidth    ();
  Int         iHeight     = pcDepthMap->getHeight   ();
  Int         iStride     = pcDepthMap->getStride   ();
  Pel*        pDMSamples  = pcDepthMap->getLumaAddr ( 0 );
  // horizontal
  for( Int iY = 0; iY < iHeight && !bUndefined; iY++, pDMSamples += iStride )
  {
    for( Int iX = 0; iX < iWidth; iX++ )
    {
      if( pDMSamples[ iX ] == PDM_UNDEFINED_DEPTH )
      {
        Int  iE;
        for( iE = iX + 1; iE < iWidth; iE++ )
        {
          if( pDMSamples[ iE ] != PDM_UNDEFINED_DEPTH )
          {
            break;
          }
        }
        if( iX > 0 || iE < iWidth )
        {
          Int iDepth;
          if     ( iX > 0 && iE < iWidth )  iDepth  = ( pDMSamples[ iX-1 ] < pDMSamples[ iE ] ? pDMSamples[ iX-1 ] : pDMSamples[ iE ] ); 
          else if( iX > 0 )                 iDepth  =   pDMSamples[ iX-1 ]; 
          else /*( iE < iWidth )*/          iDepth  =   pDMSamples[ iE   ]; 
          for( Int iZ = iX; iZ < iE; iZ++ )
          {
            pDMSamples[ iZ ] = iDepth;
          }
        }
        else
        {
          bUndefined = true;
		  break;
        }
        iX = iE - 1;
      }
    }
  }
  
  if( bUndefined && m_uiCurrViewId )
  {
    pDMSamples          = pcDepthMap->getLumaAddr( 0 );
    Int  iMiddleOrgDpth = ( 1 << m_uiOrgDepthBitDepth ) >> 1;
    Int  iMiddleDepth   = xGetVirtDepthFromOrigDepth( m_uiCurrViewId, iMiddleOrgDpth );
    for( Int iY = 0; iY < iHeight; iY++, pDMSamples += iStride )
    {
      for( Int iX = 0; iX < iWidth; iX++ )
      {
        pDMSamples[ iX ] = iMiddleDepth;
       }
    }
  }
  return bUndefined;
}


Void
TComDepthMapGenerator::xClearDepthMap( TComPic* pcPic, Int iVal )
{
  TComPicYuv* pcDepthMap  = pcPic->getPredDepthMap  ();
  Int         iWidth      = pcDepthMap->getWidth    ();
  Int         iHeight     = pcDepthMap->getHeight   ();
  Int         iStride     = pcDepthMap->getStride   ();
  Pel*        pDMSamples  = pcDepthMap->getLumaAddr ( 0 );
  for( Int iY = 0; iY < iHeight; iY++, pDMSamples += iStride )
  {
    for( Int iX = 0; iX < iWidth; iX++ )
    {
      pDMSamples[ iX ] = iVal;
    }
  }
  pcDepthMap->setBorderExtension( false );
  pcDepthMap->extendPicBorder   ();
}

Void  
TComDepthMapGenerator::xSetChroma( TComPicYuv* pcPic, Int iVal )
{
  Int   iWidth      = pcPic->getWidth   () >> 1;
  Int   iHeight     = pcPic->getHeight  () >> 1;
  Int   iStride     = pcPic->getCStride ();
  Pel*  pCbSamples  = pcPic->getCbAddr  ( 0 );
  Pel*  pCrSamples  = pcPic->getCrAddr  ( 0 );
  for( Int iY = 0; iY < iHeight; iY++, pCbSamples += iStride, pCrSamples += iStride )
  {
    for( Int iX = 0; iX < iWidth; iX++ )
    {
      pCbSamples[ iX ] = pCrSamples[ iX ] = iVal;
    }
  }
}







/*===============================================*
 *=====                                     =====*
 *=====     C U   p r e d i c t i o n s     =====*
 *=====                                     =====*
 *===============================================*/

Void
TComDepthMapGenerator::xPredictCUDepthMap( TComDataCU* pcCU, UInt uiDepth, UInt uiAbsPartIdx )
{
  UInt  uiLPelX   = pcCU->getCUPelX() + g_auiRasterToPelX[ g_auiZscanToRaster[ uiAbsPartIdx ] ];
  UInt  uiTPelY   = pcCU->getCUPelY() + g_auiRasterToPelY[ g_auiZscanToRaster[ uiAbsPartIdx ] ];
  UInt  uiRPelX   = uiLPelX           + ( g_uiMaxCUWidth  >> uiDepth ) - 1;
  UInt  uiBPelY   = uiTPelY           + ( g_uiMaxCUHeight >> uiDepth ) - 1;
  Bool  bBoundary = ( uiRPelX >= pcCU->getSlice()->getSPS()->getWidth() || uiBPelY >= pcCU->getSlice()->getSPS()->getHeight() );
  Bool  bSplit    = ( ( uiDepth < pcCU->getDepth( uiAbsPartIdx ) && uiDepth < ( g_uiMaxCUDepth - g_uiAddCUDepth ) ) || bBoundary );
  if(   bSplit )
  {
    UInt uiQNumParts = ( pcCU->getPic()->getNumPartInCU() >> ( uiDepth << 1 ) ) >> 2;
    for ( UInt uiPartUnitIdx = 0; uiPartUnitIdx < 4; uiPartUnitIdx++, uiAbsPartIdx += uiQNumParts )
    {
      uiLPelX       = pcCU->getCUPelX() + g_auiRasterToPelX[ g_auiZscanToRaster[ uiAbsPartIdx ] ];
      uiTPelY       = pcCU->getCUPelY() + g_auiRasterToPelY[ g_auiZscanToRaster[ uiAbsPartIdx ] ];
      Bool  bInside = ( uiLPelX < pcCU->getSlice()->getSPS()->getWidth() && uiTPelY < pcCU->getSlice()->getSPS()->getHeight() );
      if(   bInside )
      {
        xPredictCUDepthMap( pcCU, uiDepth + 1, uiAbsPartIdx );
      }
    }
    return;
  }

  //--- set sub-CU and sub-depth-map ---
  TComDataCU* pcSubCU   = m_ppcCU [ uiDepth ];
  TComYuv*    pcSubDM   = m_ppcYuv[ uiDepth ];
  TComPicYuv* pcPicDM   = pcCU->getPic()->getPredDepthMap();
  UInt        uiCUAddr  = pcCU->getAddr();
  pcSubCU->copySubCU( pcCU, uiAbsPartIdx, uiDepth );

  //--- update depth map ---
  switch( pcSubCU->getPredictionMode( 0 ) )
  {
  case MODE_INTRA:
    xIntraPredictCUDepthMap( pcSubCU, pcSubDM );
    break;
  case MODE_SKIP:
  case MODE_INTER:
    xInterPredictCUDepthMap( pcSubCU, pcSubDM );
    break;
  default:
    AOT( true );
    break;
  }

  //--- copy sub-depth-map ---
  pcSubDM->copyToPicYuv( pcPicDM, uiCUAddr, uiAbsPartIdx );
}


Void
TComDepthMapGenerator::xIntraPredictCUDepthMap( TComDataCU* pcCU, TComYuv* pcCUDepthMap )
{
  UInt  uiInitTrDepth = ( pcCU->getPartitionSize( 0 ) == SIZE_2Nx2N ? 0 : 1 );
  UInt  uiNumPart     =   pcCU->getNumPartInter ();
  UInt  uiNumQParts   =   pcCU->getTotalNumPart () >> 2;
  for( UInt uiPU = 0; uiPU < uiNumPart; uiPU++ )
  {
    xIntraPredictBlkDepthMap( pcCU, pcCUDepthMap, uiPU * uiNumQParts, uiInitTrDepth );
  }
}


Void
TComDepthMapGenerator::xInterPredictCUDepthMap( TComDataCU* pcCU, TComYuv* pcCUDepthMap )
{
  for( UInt uiPartIdx = 0; uiPartIdx < pcCU->getNumPartInter(); uiPartIdx++ )
  {
    xInterPredictPUDepthMap( pcCU, pcCUDepthMap, uiPartIdx );
  }
}







/*=====================================================================*
 *=====                                                           =====*
 *=====     P U -   a n d   B l o c k   p r e d i c t i o n s     =====*
 *=====                                                           =====*
 *=====================================================================*/

Void
TComDepthMapGenerator::xIntraPredictBlkDepthMap( TComDataCU* pcCU, TComYuv* pcCUDepthMap, UInt uiAbsPartIdx, UInt uiTrDepth )
{
  UInt uiFullDepth  = pcCU->getDepth( 0 ) + uiTrDepth;
  UInt uiTrMode     = pcCU->getTransformIdx( uiAbsPartIdx );
  if( uiTrMode == uiTrDepth )
  {
    UInt  uiWidth         = pcCU->getWidth ( 0 ) >> uiTrDepth;
    UInt  uiHeight        = pcCU->getHeight( 0 ) >> uiTrDepth;
    UInt  uiStride        = pcCUDepthMap->getStride  ();
    Pel*  pDepthMap       = pcCUDepthMap->getLumaAddr( uiAbsPartIdx );
    UInt  uiLumaPredMode  = pcCU->getLumaIntraDir    ( uiAbsPartIdx );
    Bool  bAboveAvail     = false;
    Bool  bLeftAvail      = false;
    pcCU->getPattern()->initPattern   ( pcCU, uiTrDepth, uiAbsPartIdx, true );
    pcCU->getPattern()->initAdiPattern( pcCU, uiAbsPartIdx, uiTrDepth, 
                                        m_pcPrediction->getPredicBuf       (),
                                        m_pcPrediction->getPredicBufWidth  (),
                                        m_pcPrediction->getPredicBufHeight (),
                                        bAboveAvail, bLeftAvail, true );
    m_pcPrediction->predIntraDepthAng ( pcCU->getPattern(), uiLumaPredMode, pDepthMap, uiStride, uiWidth, uiHeight ); // could be replaced with directional intra prediction
                                                                                                                      // using "predIntraLumaAng", but note: 
                                                                                                                      //  - need to take care of neighbours with undefined depth
                                                                                                                      //  - special case for wedgelet mode (if available in normal views)
    // copy to picture array (for next intra prediction block)
    UInt  uiZOrderIdx     = pcCU->getZorderIdxInCU() + uiAbsPartIdx;
    Pel*  pPicDepthMap    = pcCU->getPic()->getPredDepthMap()->getLumaAddr( pcCU->getAddr(), uiZOrderIdx );
    Int   iPicStride      = pcCU->getPic()->getPredDepthMap()->getStride  ();
    for( UInt uiY = 0; uiY < uiHeight; uiY++, pDepthMap += uiStride, pPicDepthMap += iPicStride )
    {
      for( UInt uiX = 0; uiX < uiWidth; uiX++ )
      {
        pPicDepthMap[ uiX ] = pDepthMap[ uiX ];
      }
    }
  }
  else
  {
    UInt uiNumQPart  = pcCU->getPic()->getNumPartInCU() >> ( ( uiFullDepth + 1 ) << 1 );
    for( UInt uiPart = 0; uiPart < 4; uiPart++ )
    {
      xIntraPredictBlkDepthMap( pcCU, pcCUDepthMap, uiAbsPartIdx + uiPart * uiNumQPart, uiTrDepth + 1 );
    }
  }
}


Void  
TComDepthMapGenerator::xInterPredictPUDepthMap( TComDataCU* pcCU, TComYuv* pcCUDepthMap, UInt uiPartIdx )
{
  if ( pcCU->getSlice()->getSPS()->getViewId() )
  {
    AOF( m_uiCurrViewId == pcCU->getSlice()->getSPS()->getViewId() );
    // check for interview prediction
    Int             iWidth;
    Int             iHeight;
    UInt            uiAbsPartIdx;
    pcCU->getPartIndexAndSize( uiPartIdx, uiAbsPartIdx, iWidth, iHeight );
    TComCUMvField*  aiCurrMvField[2]  = { pcCU->getCUMvField( REF_PIC_LIST_0 ),        pcCU->getCUMvField( REF_PIC_LIST_1 )        };
    Int             aiCurrRefIdx [2]  = { aiCurrMvField[0]->getRefIdx( uiAbsPartIdx ), aiCurrMvField[1]->getRefIdx( uiAbsPartIdx ) };
    Bool            abCurrIntView[2]  = { aiCurrRefIdx[0] >= 0 && pcCU->getSlice()->getRefPic( REF_PIC_LIST_0, aiCurrRefIdx[0] )->getSPS()->getViewId() != m_uiCurrViewId,
                                          aiCurrRefIdx[1] >= 0 && pcCU->getSlice()->getRefPic( REF_PIC_LIST_1, aiCurrRefIdx[1] )->getSPS()->getViewId() != m_uiCurrViewId };
    Bool            bUsesInterViewPrd = ( abCurrIntView[0] || abCurrIntView[1] );
    if( bUsesInterViewPrd )
    {
      xIViewPUDepthMapUpdate  ( pcCU, pcCUDepthMap, uiPartIdx );
    }
    else
    { 
#if PDM_NO_INTER_UPDATE
      xInterPUDepthMapPrediction( pcCU, pcCUDepthMap, uiPartIdx );
#else
      xInterPUDepthMapUpdate  ( pcCU, pcCUDepthMap, uiPartIdx );
#endif
    }
  }
  else
  {
    xInterPUDepthMapPrediction( pcCU, pcCUDepthMap, uiPartIdx );
  }
}


Void  
TComDepthMapGenerator::xIViewPUDepthMapUpdate( TComDataCU* pcCU, TComYuv* pcCUDepthMap, UInt uiPartIdx )
{
  // get width, height, and part address
  Int   iWidth;
  Int   iHeight;
  UInt  uiAbsPartIdx;
  pcCU->getPartIndexAndSize( uiPartIdx, uiAbsPartIdx, iWidth, iHeight );

  // get depth values
  Int   iDepthValue   = PDM_UNDEFINED_DEPTH;
  Int   aiPrdDepth[2] = { PDM_UNDEFINED_DEPTH, PDM_UNDEFINED_DEPTH };
  for( Int iRefListId = 0; iRefListId < 2; iRefListId++ )
  {
    RefPicList      eRefPicList = RefPicList( iRefListId );
    TComCUMvField*  pcCUMvField = pcCU->getCUMvField( eRefPicList );
    Int             iRefIdx     = pcCUMvField->getRefIdx( uiAbsPartIdx );
    UInt            uiBaseId    = ( iRefIdx >= 0 ? pcCU->getSlice()->getRefPic( eRefPicList, iRefIdx )->getSPS()->getViewId() : MAX_NUMBER_VIEWS ); 
    Bool            bInterview  = ( iRefIdx >= 0 && uiBaseId < m_uiCurrViewId );
    if( bInterview )
    {
      Int           iMvX        = pcCUMvField->getMv( uiAbsPartIdx ).getHor();
      aiPrdDepth[ iRefListId ]  = xGetVirtDepthFromDisparity( uiBaseId, iMvX );
    }
  }
  if( aiPrdDepth[ 0 ] != PDM_UNDEFINED_DEPTH && aiPrdDepth[ 1 ] != PDM_UNDEFINED_DEPTH )
  {
    iDepthValue = ( aiPrdDepth[ 0 ] + aiPrdDepth[ 1 ] + 1 ) >> 2;
  }
  else
  {
    iDepthValue = ( aiPrdDepth[ 0 ] != PDM_UNDEFINED_DEPTH ? aiPrdDepth[ 0 ] : aiPrdDepth[ 1 ] );
    AOT( iDepthValue == PDM_UNDEFINED_DEPTH );
  }
  iDepthValue   = Max( 0, Min( PDM_MAX_ABS_VIRT_DEPTH, iDepthValue ) );

  // set depth map for PU
  Pel*  pDMSamples  = pcCUDepthMap->getLumaAddr( uiAbsPartIdx );
  Int   iStride     = pcCUDepthMap->getStride  ();
  for( Int iY = 0; iY < iHeight; iY++, pDMSamples += iStride )
  {
    for( Int iX = 0; iX < iWidth; iX++)
    {
      pDMSamples[ iX ] = (Pel)iDepthValue;
    }
  }
}


Void
TComDepthMapGenerator::xInterPUDepthMapUpdate( TComDataCU* pcCU, TComYuv* pcCUDepthMap, UInt uiPartIdx )
{
  const Int       iMaxAbsDeltaMvY   = 8 << 2;

  //===== determine block parameters for current access unit and current view =====
  Int             iWidth;
  Int             iHeight;
  UInt            uiAbsPartIdx;
  pcCU->getPartIndexAndSize ( uiPartIdx, uiAbsPartIdx, iWidth, iHeight );
  UInt            uiCurrViewId      = pcCU->getSlice()->getSPS()->getViewId();  
  Int             iNum4x4BlksY      = iHeight >> 2;
  Int             iNum4x4BlksX      = iWidth  >> 2;
  TComPicYuv*     pcCurrDepthMap    = pcCU->getPic()->getPredDepthMap();
  Pel*            piCurrDepthMap    = pcCurrDepthMap->getLumaAddr();
  Int             iCurrStride       = pcCurrDepthMap->getStride();
  TComCUMvField*  aiCurrMvField[2]  = { pcCU->getCUMvField( REF_PIC_LIST_0 ),        pcCU->getCUMvField( REF_PIC_LIST_1 )        };
  Int             aiCurrRefIdx [2]  = { aiCurrMvField[0]->getRefIdx( uiAbsPartIdx ), aiCurrMvField[1]->getRefIdx( uiAbsPartIdx ) };
  Int             iMinCurrListId    = ( aiCurrRefIdx [0] < 0 ? 1 : 0 );
  Int             iMaxCurrListId    = ( aiCurrRefIdx [1] < 0 ? 0 : 1 );
  Int             iCurrPUPosX;
  Int             iCurrPUPosY;
  pcCurrDepthMap->getTopLeftSamplePos( pcCU->getAddr(), pcCU->getZorderIdxInCU() + uiAbsPartIdx, iCurrPUPosX, iCurrPUPosY );
  AOT( uiCurrViewId    != m_uiCurrViewId );
  AOT( iMinCurrListId  >  iMaxCurrListId );
  AOT( aiCurrRefIdx[0] >= 0 && pcCU->getSlice()->getRefPic( REF_PIC_LIST_0, aiCurrRefIdx[0] )->getSPS()->getViewId() != uiCurrViewId );
  AOT( aiCurrRefIdx[1] >= 0 && pcCU->getSlice()->getRefPic( REF_PIC_LIST_1, aiCurrRefIdx[1] )->getSPS()->getViewId() != uiCurrViewId );

  //===== determine parameters for current access unit and base view =====
  AOF( m_auiBaseIdList.size() );
  UInt            uiBaseId          = m_auiBaseIdList[ 0 ];
  TComPic*        pcBasePic         = m_pcAUPicAccess->getPic( uiBaseId );
  AOF( pcBasePic );
  TComPicYuv*     pcBaseDepthMap    = pcBasePic->getPredDepthMap();
  TComPicYuv*     pcBaseRecPic      = pcBasePic->getPicYuvRec   ();
  Pel*            piBaseDepthMap    = pcBaseDepthMap->getLumaAddr();
  Int             iBaseStride       = pcBaseDepthMap->getStride();

  //===== initialize 4x4 block arrays =====
  for( Int i4x4BlkY = 0; i4x4BlkY < iNum4x4BlksY; i4x4BlkY++ )
  {
    for( Int i4x4BlkX = 0; i4x4BlkX < iNum4x4BlksX; i4x4BlkX++ )
    {
      m_aabDepthSet[ i4x4BlkY ][ i4x4BlkX ] = false;
      m_aai4x4Depth[ i4x4BlkY ][ i4x4BlkX ] = PDM_UNDEFINED_DEPTH;
    }
  }
  Int iNum4x4Set = 0;

  //===== determine depth based on 4x4 blocks =====
  for( Int i4x4BlkY = 0; i4x4BlkY < iNum4x4BlksY; i4x4BlkY++ )
  {
    for( Int i4x4BlkX = 0; i4x4BlkX < iNum4x4BlksX; i4x4BlkX++ )
    {
      // position parameters
      Int               iCurrBlkPosX        = iCurrPUPosX + ( i4x4BlkX << 2 );
      Int               iCurrBlkPosY        = iCurrPUPosY + ( i4x4BlkY << 2 );
      Int               iCurrSamplePosX     = iCurrBlkPosX + 1;
      Int               iCurrSamplePosY     = iCurrBlkPosY + 1;
      Int               iCurrPredDepth      = piCurrDepthMap[ iCurrSamplePosY * iCurrStride + iCurrSamplePosX ];
      Int               iCurrPredDisp       = xGetDisparityFromVirtDepth( uiBaseId, iCurrPredDepth );
      Int               iBaseSamplePosX     = iCurrSamplePosX + ( ( iCurrPredDisp + 2 ) >> 2 );
      Int               iBaseSamplePosY     = iCurrSamplePosY;
      iBaseSamplePosX                       = Clip3( 0, pcBaseDepthMap->getWidth () - 1, iBaseSamplePosX );
      iBaseSamplePosY                       = Clip3( 0, pcBaseDepthMap->getHeight() - 1, iBaseSamplePosY );

      // check for occlusion
      if( piBaseDepthMap[ iBaseSamplePosY * iBaseStride + iBaseSamplePosX ] != iCurrPredDepth )
      {
        continue;
      }

      // determine base motion data and check prediction mode
      Int               iBaseCUAddr;
      Int               iBaseAbsPartIdx;
      pcBaseRecPic->getCUAddrAndPartIdx( iBaseSamplePosX, iBaseSamplePosY, iBaseCUAddr, iBaseAbsPartIdx );
      TComDataCU*       pcBaseCU            = pcBasePic->getCU( iBaseCUAddr );
      if( pcBaseCU->getPredictionMode( iBaseAbsPartIdx ) != MODE_INTER && pcBaseCU->getPredictionMode( iBaseAbsPartIdx ) != MODE_SKIP )
      {
        continue;
      }

      // check whether base was inter-view predicted
      TComCUMvField*    aiBaseMvField[2]    = { pcBaseCU->getCUMvField( REF_PIC_LIST_0 ),       pcBaseCU->getCUMvField( REF_PIC_LIST_1 )       };
      Int               aiBaseRefIdx [2]    = { aiBaseMvField[0]->getRefIdx( iBaseAbsPartIdx ), aiBaseMvField[1]->getRefIdx( iBaseAbsPartIdx ) };
      Bool              abBaseIntView[2]    = { aiBaseRefIdx[0] >= 0 && pcBaseCU->getSlice()->getRefPic( REF_PIC_LIST_0, aiBaseRefIdx[0] )->getSPS()->getViewId() != uiBaseId,
                                                aiBaseRefIdx[1] >= 0 && pcBaseCU->getSlice()->getRefPic( REF_PIC_LIST_1, aiBaseRefIdx[1] )->getSPS()->getViewId() != uiBaseId };
      if( abBaseIntView[0] || abBaseIntView[1] )
      { // current depth is reliable
        m_aai4x4Depth[i4x4BlkY][i4x4BlkX]   = iCurrPredDepth;
        m_aabDepthSet[i4x4BlkY][i4x4BlkX]   = true;
        iNum4x4Set++;
        continue;
      }
      
      // determine depth candidates using an approximate 4-point relationship (if appropriate)
      std::vector<Int>  aiDepthCand;
      Int               iMinBaseListId      = ( aiBaseRefIdx [0] < 0 ? 1 : 0 );
      Int               iMaxBaseListId      = ( aiBaseRefIdx [1] < 0 ? 0 : 1 );
      AOT( iMinBaseListId > iMaxBaseListId );
      for( Int iCurrRefListId  = iMinCurrListId; iCurrRefListId <= iMaxCurrListId; iCurrRefListId++ )
      {
        RefPicList      eCurrRefPicList     = RefPicList( iCurrRefListId );
        Int             iCurrRefPoc         = pcCU->getSlice()->getRefPOC( eCurrRefPicList, aiCurrRefIdx[ iCurrRefListId ] );
        TComPic*        pcCurrRefPic        = pcCU->getSlice()->getRefPic( eCurrRefPicList, aiCurrRefIdx[ iCurrRefListId ] );
        TComPicYuv*     pcCurrRefDMap       = pcCurrRefPic->getPredDepthMap();
        Pel*            piCurrRefDMap       = pcCurrRefDMap->getLumaAddr();
        Int             iCurrRefStride      = pcCurrRefDMap->getStride();
        TComMv&         rcCurrMv            = aiCurrMvField[ iCurrRefListId ]->getMv( uiAbsPartIdx );
        Int             iCurrRefSamplePosX  = iCurrSamplePosX + ( ( rcCurrMv.getHor() + 2 ) >> 2 );
        Int             iCurrRefSamplePosY  = iCurrSamplePosY + ( ( rcCurrMv.getVer() + 2 ) >> 2 );
        Int             iCurrRefSamplePosXC = Clip3( 0, pcCurrRefDMap->getWidth () - 1, iCurrRefSamplePosX );
        Int             iCurrRefSamplePosYC = Clip3( 0, pcCurrRefDMap->getHeight() - 1, iCurrRefSamplePosY );
        Int             iCurrRefDepth       = piCurrRefDMap[ iCurrRefSamplePosYC * iCurrRefStride + iCurrRefSamplePosXC ];

        for( Int iBaseRefListId = iMinBaseListId; iBaseRefListId <= iMaxBaseListId; iBaseRefListId++ )
        {
          RefPicList    eBaseRefPicList     = RefPicList( iBaseRefListId );
          Int           iBaseRefPoc         = pcBaseCU->getSlice()->getRefPOC( eBaseRefPicList, aiBaseRefIdx[ iBaseRefListId ] );

          if( iCurrRefPoc == iBaseRefPoc )
          {
            // location and depth for path currView/currAU -> currView/refAU -> baseView/refAU
            Int         iCurrRefDisp        = xGetDisparityFromVirtDepth( uiBaseId, iCurrRefDepth );
            Int         iBaseRefSamplePosX  = iCurrRefSamplePosX + ( ( iCurrRefDisp + 2 ) >> 2 );
            Int         iBaseRefSamplePosY  = iCurrRefSamplePosY;
            TComPic*    pcBaseRefPic        = pcBaseCU->getSlice()->getRefPic( eBaseRefPicList, aiBaseRefIdx[ iBaseRefListId ] );
            TComPicYuv* pcBaseRefDMap       = pcBaseRefPic->getPredDepthMap();
            Pel*        piBaseRefDMap       = pcBaseRefDMap->getLumaAddr();
            Int         iBaseRefStride      = pcBaseRefDMap->getStride();
            iBaseRefSamplePosX              = Clip3( 0, pcBaseRefDMap->getWidth () - 1, iBaseRefSamplePosX );
            iBaseRefSamplePosY              = Clip3( 0, pcBaseRefDMap->getHeight() - 1, iBaseRefSamplePosY );
            Int         iBaseRefDepth       = piBaseRefDMap[ iBaseRefSamplePosY * iBaseRefStride + iBaseRefSamplePosX ];

            // location and depth for path currView/currAU ->baseView/currAU -> baseView/refAU
            TComMv&     rcBaseMv            = aiBaseMvField[ iBaseRefListId ]->getMv( iBaseAbsPartIdx );
            Int         iAbsDeltaMvY        = ( rcBaseMv.getAbsVer() > rcCurrMv.getVer() ? rcBaseMv.getAbsVer() - rcCurrMv.getVer() : rcCurrMv.getVer() - rcBaseMv.getAbsVer() );

            // check reliability (occlusion in reference access unit / vertical motion vector difference)
            if( iBaseRefDepth != iCurrRefDepth || iAbsDeltaMvY > iMaxAbsDeltaMvY )
            {
              continue;
            }

            // determine new depth
            Int         iCurrCandDisp       = iCurrRefDisp + rcCurrMv.getHor() - rcBaseMv.getHor();
            Int         iCurrCandDepth      = xGetVirtDepthFromDisparity( uiBaseId, iCurrCandDisp );
            aiDepthCand.push_back( iCurrCandDepth );
          } // iCurrRefPoc == iBaseRefPoc
        } // iBaseRefListId
      } // iCurrRefListId
      
      // set depth for 4x4 block
      if( aiDepthCand.size() )
      { // get depth with minimum change (probably most reliable)
        Int             iMinAbsDepthChange  = (1<<30);
        Int             iDepthForMinChange  = (1<<30);
        for( UInt uiCandId = 0; uiCandId < (UInt)aiDepthCand.size(); uiCandId++ )
        {
          Int           iAbsDeltaDepth      = ( aiDepthCand[uiCandId] > iCurrPredDepth ? aiDepthCand[uiCandId] > iCurrPredDepth : iCurrPredDepth - aiDepthCand[uiCandId] );
          if( iAbsDeltaDepth < iMinAbsDepthChange )
          {
            iMinAbsDepthChange              = iAbsDeltaDepth;
            iDepthForMinChange              = aiDepthCand[uiCandId];
          }
        }
        m_aai4x4Depth[i4x4BlkY][i4x4BlkX]   = Min( Max( 0, iDepthForMinChange ), PDM_MAX_ABS_VIRT_DEPTH );
        m_aabDepthSet[i4x4BlkY][i4x4BlkX]   = true;
        iNum4x4Set++;
      }
    } // i4x4BlkX
  } // i4x4BlkY

  //===== fall back (take original depth for 4x4 blocks) ====
  if( iNum4x4Set < Max( 1, ( iNum4x4BlksY * iNum4x4BlksX ) >> 2 ) )
  {
    iNum4x4Set = 0;
    for( Int i4x4BlkY = 0; i4x4BlkY < iNum4x4BlksY; i4x4BlkY++ )
    {
      for( Int i4x4BlkX = 0; i4x4BlkX < iNum4x4BlksX; i4x4BlkX++ )
      {
        Int             iCurrSamplePosX     = iCurrPUPosX + ( i4x4BlkX << 2 ) + 1;
        Int             iCurrSamplePosY     = iCurrPUPosY + ( i4x4BlkY << 2 ) + 1;
        m_aai4x4Depth[i4x4BlkY][i4x4BlkX]   = piCurrDepthMap[ iCurrSamplePosY * iCurrStride + iCurrSamplePosX ];
        m_aabDepthSet[i4x4BlkY][i4x4BlkX]   = true;
        iNum4x4Set++;
      }
    }
  }

#if PDM_ONE_DEPTH_PER_PU
  //===== set average in 4x4 depth array =====
  Int   iSum        = 0;
  for( Int i4x4BlkY = 0; i4x4BlkY < iNum4x4BlksY; i4x4BlkY++ )
  {
    for( Int i4x4BlkX = 0; i4x4BlkX < iNum4x4BlksX; i4x4BlkX++ )
    {
      if( m_aabDepthSet[ i4x4BlkY ][ i4x4BlkX ] )
      {
        iSum += m_aai4x4Depth[ i4x4BlkY ][ i4x4BlkX ];
      }
    }
  }
  Int   iDepth      = ( iSum + ( iNum4x4Set >> 1 ) ) / iNum4x4Set;
  iNum4x4Set        = iNum4x4BlksY * iNum4x4BlksX;
  for( Int i4x4BlkY = 0; i4x4BlkY < iNum4x4BlksY; i4x4BlkY++ )
  {
    for( Int i4x4BlkX = 0; i4x4BlkX < iNum4x4BlksX; i4x4BlkX++ )
    {
      m_aai4x4Depth[ i4x4BlkY ][ i4x4BlkX ] = iDepth;
      m_aabDepthSet[ i4x4BlkY ][ i4x4BlkX ] = true;
    }
  }
#endif
 
  //===== complete depth arrays =====
  while( iNum4x4BlksY * iNum4x4BlksX - iNum4x4Set )
  {
    for( Int i4x4BlkY = 0; i4x4BlkY < iNum4x4BlksY; i4x4BlkY++ )
    {
      for( Int i4x4BlkX = 0; i4x4BlkX < iNum4x4BlksX; i4x4BlkX++ )
      {
        if( !m_aabDepthSet[ i4x4BlkY ][ i4x4BlkX ] )
        { // could also use minimum of neighbours (occlusions)
          Int iNumNeighbours  = 0;
          Int iSumNeighbours  = 0;
          if( i4x4BlkY > 0              && m_aabDepthSet[ i4x4BlkY-1 ][ i4x4BlkX   ] ) { iSumNeighbours += m_aai4x4Depth[ i4x4BlkY-1 ][ i4x4BlkX   ]; iNumNeighbours++; }
          if( i4x4BlkY < iNum4x4BlksY-1 && m_aabDepthSet[ i4x4BlkY+1 ][ i4x4BlkX   ] ) { iSumNeighbours += m_aai4x4Depth[ i4x4BlkY+1 ][ i4x4BlkX   ]; iNumNeighbours++; }
          if( i4x4BlkX > 0              && m_aabDepthSet[ i4x4BlkY   ][ i4x4BlkX-1 ] ) { iSumNeighbours += m_aai4x4Depth[ i4x4BlkY   ][ i4x4BlkX-1 ]; iNumNeighbours++; }
          if( i4x4BlkX < iNum4x4BlksX-1 && m_aabDepthSet[ i4x4BlkY   ][ i4x4BlkX+1 ] ) { iSumNeighbours += m_aai4x4Depth[ i4x4BlkY   ][ i4x4BlkX+1 ]; iNumNeighbours++; }
          if( iNumNeighbours )
          {
            m_aai4x4Depth[ i4x4BlkY ][ i4x4BlkX ] = ( iSumNeighbours + ( iNumNeighbours >> 1 ) ) / iNumNeighbours;
            m_aabDepthSet[ i4x4BlkY ][ i4x4BlkX ] = true;
            iNum4x4Set++;
          }
        }
      }
    }
  }

  //===== set depth values =====
  Pel* piDepthMap = pcCUDepthMap->getLumaAddr( uiAbsPartIdx );
  Int  iCUStride  = pcCUDepthMap->getStride  ();
  for( Int iRow = 0; iRow < iHeight; iRow++, piDepthMap += iCUStride )
  {
    for( Int iCol = 0; iCol < iWidth; iCol++ )
    {
      piDepthMap[ iCol ] = m_aai4x4Depth[ iRow >> 2 ][ iCol >> 2 ];
    }
  }
}


Void
TComDepthMapGenerator::xInterPUDepthMapPrediction( TComDataCU* pcCU, TComYuv* pcCUDepthMap, UInt uiPartIdx )
{
  m_pcPrediction->motionCompensation( pcCU, pcCUDepthMap, REF_PIC_LIST_X, (Int)uiPartIdx, true );  
}


Bool
TComDepthMapGenerator::xGetPredDepth( TComDataCU* pcCU, UInt uiPartIdx, Int& riPrdDepth, Int* piPosX, Int* piPosY )
{
  AOF  ( m_bCreated && m_bInit );
  AOF  ( pcCU );
  ROFRS( m_bPDMAvailable, false );

  TComSlice*    pcSlice     = pcCU->getSlice ();
  TComPic*      pcPic       = pcCU->getPic   ();
  TComSPS*      pcSPS       = pcSlice->getSPS();
  AOF  ( pcPic->getPredDepthMap() );
  AOF  ( pcSPS->getViewId() == m_uiCurrViewId );
  
  //===== get predicted depth and disprity for middle position of current PU =====  
  UInt          uiPartAddr;
  Int           iWidth;
  Int           iHeight;
  pcCU->getPartIndexAndSize( uiPartIdx, uiPartAddr, iWidth, iHeight );
  TComPicYuv*   pcPredDepthMap  = pcPic->getPredDepthMap();
  Pel*          piPredDepthMap  = pcPredDepthMap->getLumaAddr ( 0 );
  Int           iCurrStride     = pcPredDepthMap->getStride   ();
  Int           iCurrPosX;
  Int           iCurrPosY;
  pcPredDepthMap->getTopLeftSamplePos( pcCU->getAddr(), pcCU->getZorderIdxInCU() + uiPartAddr, iCurrPosX, iCurrPosY );
  iCurrPosX  += ( iWidth  - 1 ) >> 1;
  iCurrPosY  += ( iHeight - 1 ) >> 1;
  riPrdDepth  = piPredDepthMap[ iCurrPosX + iCurrPosY * iCurrStride ];
  if( piPosX )
  {
    *piPosX   = iCurrPosX;
  }
  if( piPosY )
  {
    *piPosY   = iCurrPosY;
  }
  return        true;
}

#endif // DEPTH_MAP_GENERATION
