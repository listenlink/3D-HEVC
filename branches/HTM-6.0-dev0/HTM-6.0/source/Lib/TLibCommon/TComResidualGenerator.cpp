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



/** \file     TComResidualGenerator.cpp
    \brief    residual picture generator class
*/



#include "CommonDef.h"
#include "TComResidualGenerator.h"


#if H3D_IVRP


TComResidualGenerator::TComResidualGenerator()
{
  m_bCreated            = false;
  m_bInit               = false;
  m_bDecoder            = false;
  m_pcTrQuant           = 0;
  m_pcDepthMapGenerator = 0;
  m_pcSPSAccess         = 0;
  m_pcAUPicAccess       = 0;
  m_uiMaxDepth          = 0;
  m_uiOrgDepthBitDepth  = 0;
  m_ppcYuvTmp           = 0;
  m_ppcYuv              = 0;
  m_ppcCU               = 0;
}

TComResidualGenerator::~TComResidualGenerator()
{
  destroy ();
  uninit  ();
}

Void
TComResidualGenerator::create( Bool bDecoder, UInt uiPicWidth, UInt uiPicHeight, UInt uiMaxCUDepth, UInt uiMaxCUWidth, UInt uiMaxCUHeight, UInt uiOrgBitDepth )
{
  destroy();
  m_bDecoder            = bDecoder;
  m_uiMaxDepth          = uiMaxCUDepth;
  m_uiOrgDepthBitDepth  = uiOrgBitDepth;
  m_ppcYuvTmp           = new TComYuv*    [ NUM_TMP_YUV_BUFFERS ];
  m_ppcYuv              = new TComYuv*    [ m_uiMaxDepth ];
  m_ppcCU               = new TComDataCU* [ m_uiMaxDepth ];
  for( UInt uiIdx = 0; uiIdx < NUM_TMP_YUV_BUFFERS; uiIdx++ )
  {
    m_ppcYuvTmp[uiIdx]  = new TComYuv;    m_ppcYuvTmp[uiIdx]->create( uiMaxCUWidth, uiMaxCUHeight );
  }
  for( UInt uiDepth = 0; uiDepth < m_uiMaxDepth; uiDepth++ )
  {
    UInt  uiNumPart = 1 << ( ( m_uiMaxDepth - uiDepth ) << 1 );
    UInt  uiWidth   = uiMaxCUWidth  >> uiDepth;
    UInt  uiHeight  = uiMaxCUHeight >> uiDepth;

    m_ppcYuv[ uiDepth ] = new TComYuv;    m_ppcYuv[ uiDepth ]->create(            uiWidth, uiHeight                                           );
    m_ppcCU [ uiDepth ] = new TComDataCU; m_ppcCU [ uiDepth ]->create( uiNumPart, uiWidth, uiHeight, true, uiMaxCUWidth >> (uiMaxCUDepth - 1) );
  }
  m_cTmpPic.create( uiPicWidth, uiPicHeight, uiMaxCUWidth, uiMaxCUHeight, uiMaxCUDepth );
  m_bCreated            = true;
}

Void
TComResidualGenerator::destroy()
{
  if( m_bCreated )
  {
    m_bCreated            = false;
    for( UInt uiIdx = 0; uiIdx < NUM_TMP_YUV_BUFFERS; uiIdx++ )
    {
      m_ppcYuvTmp[uiIdx]->destroy();  delete m_ppcYuvTmp[uiIdx]; m_ppcYuvTmp[uiIdx] = 0;
    }
    for( UInt uiDepth = 0; uiDepth < m_uiMaxDepth; uiDepth++ )
    {
      m_ppcYuv[ uiDepth ]->destroy(); delete m_ppcYuv[ uiDepth ]; m_ppcYuv[ uiDepth ] = 0;
      m_ppcCU [ uiDepth ]->destroy(); delete m_ppcCU [ uiDepth ]; m_ppcCU [ uiDepth ] = 0;
    }
    delete [] m_ppcYuvTmp;  m_ppcYuvTmp = 0;
    delete [] m_ppcYuv;     m_ppcYuv = 0;
    delete [] m_ppcCU;      m_ppcCU  = 0;
    m_cTmpPic.destroy();
    m_uiMaxDepth          = 0;
    m_uiOrgDepthBitDepth  = 0;
    m_bDecoder            = false;
  }
}

Void
TComResidualGenerator::init( TComTrQuant* pcTrQuant, TComDepthMapGenerator* pcDepthMapGenerator )
{
  AOF( pcTrQuant           );
  AOF( pcDepthMapGenerator );
  AOF( pcDepthMapGenerator->getSPSAccess  () );
  AOF( pcDepthMapGenerator->getAUPicAccess() );
  uninit();
  m_pcTrQuant           = pcTrQuant;
  m_pcDepthMapGenerator = pcDepthMapGenerator;
  m_pcSPSAccess         = pcDepthMapGenerator->getSPSAccess  ();
  m_pcAUPicAccess       = pcDepthMapGenerator->getAUPicAccess();
  m_bInit               = true;
}

Void
TComResidualGenerator::uninit()
{
  if( m_bInit )
  {
    m_bInit               = false;
    m_pcTrQuant           = 0;
    m_pcDepthMapGenerator = 0;
    m_pcSPSAccess         = 0;
    m_pcAUPicAccess       = 0;
  }
}



Void  
TComResidualGenerator::initViewComponent( TComPic* pcPic )
{
  AOF  ( m_bCreated && m_bInit );
  AOF  ( pcPic );

  // set pointer in SPS
  pcPic->getSPS()->setResidualGenerator( this );
  ROFVS( pcPic->getSPS()->getMultiviewResPredMode() );

#if OUTPUT_RESIDUAL_PICTURES
  // dump reconstructed residual picture for first AU
  if( pcPic->getPOC() == 0 )
  {
    AOF( pcPic->getSPS()->getViewId() );
    UInt       uiBaseViewId = pcPic->getSPS()->getViewId() - 1;
    TComPic*   pcBasePic    = m_pcAUPicAccess->getPic( uiBaseViewId );
    AOF( pcBasePic );
    Char       acFilenameBase[1024];
    ::sprintf( acFilenameBase,  "RecResidual_%s", ( m_bDecoder ? "Dec" : "Enc" ) );
    xDumpResidual( pcBasePic, acFilenameBase );
  }
#endif
}



Void
TComResidualGenerator::setRecResidualPic( TComPic* pcPic )
{
  AOF  ( m_bCreated && m_bInit );
  AOF  ( pcPic );

#if MTK_MDIVRP_C0138
  if (pcPic->getSPS()->getViewId() != 0)
  {
    return;
  }
#endif

  if( pcPic->getPOC() == 0 )
  {
    if( pcPic->getSPS()->getViewId() == 0 || m_pcSPSAccess->getResPrd() != 0 )
    {
      // set residual picture
      AOT( pcPic->getResidual() );
      if( !pcPic->getResidual() )
      {
        pcPic->addResidualBuffer();
      }
      xSetRecResidualPic( pcPic );
    }
  }
  else
  {
    if( m_pcSPSAccess->getResPrd() != 0 && pcPic->getSPS()->getViewId() < m_pcAUPicAccess->getMaxVId() )
    {
      // set residual picture
      AOT( pcPic->getResidual() );
      if( !pcPic->getResidual() )
      {
        pcPic->addResidualBuffer();
      }
      xSetRecResidualPic( pcPic );

#if OUTPUT_RESIDUAL_PICTURES
      // dump reconstructed residual picture
      Char acFilenameBase[1024];
      ::sprintf( acFilenameBase,  "RecResidual_%s", ( m_bDecoder ? "Dec" : "Enc" ) );
      xDumpResidual( pcPic, acFilenameBase );
#endif
    }
  }
}

#if H3D_NBDV
#if MTK_RELEASE_DV_CONSTRAINT_C0129
Bool
TComResidualGenerator::getResidualSamples( TComDataCU* pcCU, UInt uiPUIdx, TComYuv* pcYuv, TComMv iDisp, Bool bRecon  ) 
#else
Bool
TComResidualGenerator::getResidualSamples( TComDataCU* pcCU, UInt uiPUIdx, TComYuv* pcYuv, Int iDisp, Bool bRecon  ) 
#endif
#else
Bool
TComResidualGenerator::getResidualSamples( TComDataCU* pcCU, UInt uiPUIdx, TComYuv* pcYuv, Bool bRecon ) 
#endif //H3D_NBDV
{
  AOF(  pcCU );
  UInt  uiPartAddr;
  Int   iBlkWidth, iBlkHeight, iXPos, iYPos;
  AOT(  uiPUIdx );
  uiPartAddr  = 0;
  iBlkWidth   = pcCU->getWidth  ( 0 );
  iBlkHeight  = pcCU->getHeight ( 0 );
  pcCU->getPic()->getPicYuvRec()->getTopLeftSamplePos( pcCU->getAddr(), pcCU->getZorderIdxInCU() + uiPartAddr, iXPos, iYPos );
#if H3D_NBDV
#if MTK_RELEASE_DV_CONSTRAINT_C0129
  return getResidualSamples( pcCU->getPic(), (UInt)iXPos, (UInt)iYPos, (UInt)iBlkWidth, (UInt)iBlkHeight, pcYuv, iDisp, bRecon);   
#else
  return getResidualSamples( pcCU->getPic(), (UInt)iXPos, (UInt)iYPos, (UInt)iBlkWidth, (UInt)iBlkHeight, pcYuv, iDisp, bRecon); 
#endif
#else
  return getResidualSamples( pcCU->getPic(), (UInt)iXPos, (UInt)iYPos, (UInt)iBlkWidth, (UInt)iBlkHeight, pcYuv, bRecon); 
#endif // H3D_NBDV
}
  
#if H3D_NBDV
#if MTK_RELEASE_DV_CONSTRAINT_C0129
Bool
TComResidualGenerator::getResidualSamples( TComPic* pcPic, UInt uiXPos, UInt uiYPos, UInt uiBlkWidth, UInt uiBlkHeight, TComYuv* pcYuv, TComMv iDisp, Bool bRecon)  
#else
Bool
TComResidualGenerator::getResidualSamples( TComPic* pcPic, UInt uiXPos, UInt uiYPos, UInt uiBlkWidth, UInt uiBlkHeight, TComYuv* pcYuv, Int iDisp, Bool bRecon) 
#endif
#else
Bool
TComResidualGenerator::getResidualSamples( TComPic* pcPic, UInt uiXPos, UInt uiYPos, UInt uiBlkWidth, UInt uiBlkHeight, TComYuv* pcYuv, Bool bRecon) 
#endif
{
#if MTK_C0138_FIXED
  UInt  uiBaseViewId  = 0;
#else
  UInt  uiBaseViewId  = m_pcDepthMapGenerator->getBaseViewId( 0 );
#endif
  if( !pcYuv )
  {
    pcYuv = m_ppcYuvTmp[1];
  }
  UInt uiXPosInRefView = uiXPos , uiYPosInRefView = uiYPos;
#if H3D_NBDV
#if MTK_RELEASE_DV_CONSTRAINT_C0129
  xSetPredResidualBlock( pcPic, uiBaseViewId, uiXPos, uiYPos, uiBlkWidth, uiBlkHeight, pcYuv, iDisp, &uiXPosInRefView , &uiYPosInRefView , bRecon  );
#else
  xSetPredResidualBlock( pcPic, uiBaseViewId, uiXPos, uiYPos, uiBlkWidth, uiBlkHeight, pcYuv, iDisp, &uiXPosInRefView , &uiYPosInRefView , bRecon  );
#endif
#else
  xSetPredResidualBlock( pcPic, uiBaseViewId, uiXPos, uiYPos, uiBlkWidth, uiBlkHeight, pcYuv, &uiXPosInRefView , &uiYPosInRefView , bRecon    );
#endif
#if MTK_MDIVRP_C0138
  return true;
#else
  return xIsNonZeroByCBF( uiBaseViewId , uiXPosInRefView , uiYPosInRefView , uiBlkWidth , uiBlkHeight );
#endif
}

Bool TComResidualGenerator::xIsNonZeroByCBF( UInt uiBaseViewId , UInt uiXPos , UInt uiYPos, UInt uiBlkWidth , UInt uiBlkHeight )
{
  TComPic* pcBasePic   = m_pcAUPicAccess->getPic( uiBaseViewId );
  const Int nMaxPicX = pcBasePic->getSPS()->getPicWidthInLumaSamples() - 1;
  const Int nMaxPicY = pcBasePic->getSPS()->getPicHeightInLumaSamples() - 1;
  for( UInt y = 0 ; y < uiBlkHeight ; y +=4 )
  {
    for( UInt x = 0 ; x <= uiBlkWidth ; x += 4 )
    {      // to cover both the mapped CU and the 1-pixel-right-shifted mapped CU
      Int iCuAddr = 0, iAbsZorderIdx = 0;
      pcBasePic->getPicYuvRec()->getCUAddrAndPartIdx( Min( uiXPos + x , nMaxPicX ) , Min( uiYPos + y , nMaxPicY ) , iCuAddr , iAbsZorderIdx );
      TComDataCU *pCUInRefView = pcBasePic->getCU( iCuAddr );
      if( pCUInRefView->isIntra( iAbsZorderIdx ) )
        // no inter-view residual pred from a intra CU
        continue;
      UInt uiTempTrDepth = pCUInRefView->getTransformIdx( iAbsZorderIdx );
      if( pCUInRefView->getCbf( iAbsZorderIdx , TEXT_LUMA , uiTempTrDepth )
        || pCUInRefView->getCbf( iAbsZorderIdx , TEXT_CHROMA_U , uiTempTrDepth )
        || pCUInRefView->getCbf( iAbsZorderIdx , TEXT_CHROMA_V , uiTempTrDepth ) )
        return( true );
}
  }

  return( false );
}



Void
TComResidualGenerator::xSetRecResidualPic( TComPic* pcPic )
{
  AOF( pcPic );
  AOF( pcPic->getResidual() );
  for( UInt uiCUAddr = 0; uiCUAddr < pcPic->getPicSym()->getNumberOfCUsInFrame(); uiCUAddr++ )
  {
    TComDataCU* pcCU = pcPic->getCU( uiCUAddr );
    xSetRecResidualCU( pcCU, 0, 0 );
  }
  pcPic->getResidual()->setBorderExtension( false );
  pcPic->getResidual()->extendPicBorder   ();
}


Void
TComResidualGenerator::xSetRecResidualCU( TComDataCU* pcCU, UInt uiDepth, UInt uiAbsPartIdx )
{
  UInt  uiLPelX   = pcCU->getCUPelX() + g_auiRasterToPelX[ g_auiZscanToRaster[ uiAbsPartIdx ] ];
  UInt  uiTPelY   = pcCU->getCUPelY() + g_auiRasterToPelY[ g_auiZscanToRaster[ uiAbsPartIdx ] ];
  UInt  uiRPelX   = uiLPelX           + ( g_uiMaxCUWidth  >> uiDepth ) - 1;
  UInt  uiBPelY   = uiTPelY           + ( g_uiMaxCUHeight >> uiDepth ) - 1;
  Bool  bBoundary = ( uiRPelX >= pcCU->getSlice()->getSPS()->getPicWidthInLumaSamples() || uiBPelY >= pcCU->getSlice()->getSPS()->getPicHeightInLumaSamples() );
  Bool  bSplit    = ( ( uiDepth < pcCU->getDepth( uiAbsPartIdx ) && uiDepth < ( g_uiMaxCUDepth - g_uiAddCUDepth ) ) || bBoundary );
  if(   bSplit )
  {
    UInt uiQNumParts = ( pcCU->getPic()->getNumPartInCU() >> ( uiDepth << 1 ) ) >> 2;
    for ( UInt uiPartUnitIdx = 0; uiPartUnitIdx < 4; uiPartUnitIdx++, uiAbsPartIdx += uiQNumParts )
    {
      uiLPelX       = pcCU->getCUPelX() + g_auiRasterToPelX[ g_auiZscanToRaster[ uiAbsPartIdx ] ];
      uiTPelY       = pcCU->getCUPelY() + g_auiRasterToPelY[ g_auiZscanToRaster[ uiAbsPartIdx ] ];
      Bool  bInside = ( uiLPelX < pcCU->getSlice()->getSPS()->getPicWidthInLumaSamples() && uiTPelY < pcCU->getSlice()->getSPS()->getPicHeightInLumaSamples() );
      if(   bInside )
      {
        xSetRecResidualCU( pcCU, uiDepth + 1, uiAbsPartIdx );
      }
    }
    return;
  }

  //--- set sub-CU and sub-residual ---
  TComDataCU* pcSubCU   = m_ppcCU [ uiDepth ];
  TComYuv*    pcSubRes  = m_ppcYuv[ uiDepth ];
  TComPicYuv* pcPicRes  = pcCU->getPic()->getResidual();
  UInt        uiCUAddr  = pcCU->getAddr();
  pcSubCU->copySubCU( pcCU, uiAbsPartIdx, uiDepth );

  //--- set residual ---
  switch( pcSubCU->getPredictionMode( 0 ) )
  {
  case MODE_INTRA:
    xSetRecResidualIntraCU( pcSubCU, pcSubRes );
    break;
  case MODE_SKIP:
  case MODE_INTER:
    xSetRecResidualInterCU( pcSubCU, pcSubRes );
    break;
  default:
    AOT( true );
    break;
  }

  //--- copy sub-residual ---
  pcSubRes->copyToPicYuv( pcPicRes, uiCUAddr, uiAbsPartIdx );
}


Void  
TComResidualGenerator::xSetRecResidualIntraCU( TComDataCU* pcCU, TComYuv* pcCUResidual )
{ 
  //===== set residual to zero for entire CU =====
  xClearResidual( pcCUResidual, 0, pcCU->getWidth( 0 ), pcCU->getHeight( 0 ) );
}


Void
TComResidualGenerator::xSetRecResidualInterCU( TComDataCU* pcCU, TComYuv* pcCUResidual )
{
  //===== reconstruct residual from coded transform coefficient levels =====
  xClearResidual( pcCUResidual, 0, pcCU->getWidth( 0 ), pcCU->getHeight( 0 ) );
  // luma
  UInt    uiWidth   = pcCU->getWidth  ( 0 );
  UInt    uiHeight  = pcCU->getHeight ( 0 );
  TCoeff* piCoeff   = pcCU->getCoeffY ();
  Pel*    pRes      = pcCUResidual->getLumaAddr();
  UInt    uiLumaTrMode, uiChromaTrMode;
#if LG_RESTRICTEDRESPRED_M24766  && !MTK_MDIVRP_C0138
  Int     iPUPredResiShift[4];
#endif
  pcCU->convertTransIdx             ( 0, pcCU->getTransformIdx( 0 ), uiLumaTrMode, uiChromaTrMode );
    m_pcTrQuant->setQPforQuant      ( pcCU->getQP( 0 ), !pcCU->getSlice()->getDepth(), pcCU->getSlice()->getSliceType(), TEXT_LUMA, pcCU->getSlice()->getSPS()->getQpBDOffsetY(), 0 );
  m_pcTrQuant->invRecurTransformNxN ( pcCU, 0, TEXT_LUMA, pRes, 0, pcCUResidual->getStride(), uiWidth, uiHeight, uiLumaTrMode, 0, piCoeff );
  // chroma Cb
  uiWidth   >>= 1;
  uiHeight  >>= 1;
  piCoeff     = pcCU->getCoeffCb();
  pRes        = pcCUResidual->getCbAddr();
    m_pcTrQuant->setQPforQuant      ( pcCU->getQP( 0 ), !pcCU->getSlice()->getDepth(), pcCU->getSlice()->getSliceType(), TEXT_CHROMA, pcCU->getSlice()->getSPS()->getQpBDOffsetC(), pcCU->getSlice()->getPPS()->getChromaQpOffset() );
  m_pcTrQuant->invRecurTransformNxN ( pcCU, 0, TEXT_CHROMA_U, pRes, 0, pcCUResidual->getCStride(), uiWidth, uiHeight, uiChromaTrMode, 0, piCoeff );
  // chroma Cr
  piCoeff     = pcCU->getCoeffCr();
  pRes        = pcCUResidual->getCrAddr();
  m_pcTrQuant->invRecurTransformNxN ( pcCU, 0, TEXT_CHROMA_V, pRes, 0, pcCUResidual->getCStride(), uiWidth, uiHeight, uiChromaTrMode, 0, piCoeff );

#if !MTK_MDIVRP_C0138
  if( pcCU->getResPredFlag( 0 ) )
  {
    AOF( pcCU->getResPredAvail( 0 ) );
    Bool bOK = pcCU->getResidualSamples( 0, true, m_ppcYuvTmp[0] );
    AOF( bOK );
#if LG_RESTRICTEDRESPRED_M24766
    pcCU->getPUResiPredShift(iPUPredResiShift, 0);
    pcCUResidual->add(iPUPredResiShift, pcCU->getPartitionSize(0), m_ppcYuvTmp[0], pcCU->getWidth( 0 ), pcCU->getHeight( 0 ) );
#else
    pcCUResidual->add( m_ppcYuvTmp[0], pcCU->getWidth( 0 ), pcCU->getHeight( 0 ) );
#endif
  }
#endif

  //===== clear inter-view predicted parts =====
  for( UInt uiPartIdx = 0; uiPartIdx < pcCU->getNumPartInter(); uiPartIdx++ )
  {
    xClearIntViewResidual( pcCU, pcCUResidual, uiPartIdx );
  }
}


Void
TComResidualGenerator::xClearIntViewResidual( TComDataCU* pcCU, TComYuv* pcCUResidual, UInt uiPartIdx )
{
  UInt uiCurrViewId = pcCU->getSlice()->getSPS()->getViewId();
  if(  uiCurrViewId )
  {
    Int             iWidth;
    Int             iHeight;
    UInt            uiAbsPartIdx;
    pcCU->getPartIndexAndSize( uiPartIdx, uiAbsPartIdx, iWidth, iHeight );
    TComCUMvField*  aiCurrMvField[2]  = { pcCU->getCUMvField( REF_PIC_LIST_0 ),        pcCU->getCUMvField( REF_PIC_LIST_1 )        };
    Int             aiCurrRefIdx [2]  = { aiCurrMvField[0]->getRefIdx( uiAbsPartIdx ), aiCurrMvField[1]->getRefIdx( uiAbsPartIdx ) };
    Bool            abCurrIntView[2]  = { aiCurrRefIdx[0] >= 0 && pcCU->getSlice()->getRefPic( REF_PIC_LIST_0, aiCurrRefIdx[0] )->getSPS()->getViewId() != uiCurrViewId,
                                          aiCurrRefIdx[1] >= 0 && pcCU->getSlice()->getRefPic( REF_PIC_LIST_1, aiCurrRefIdx[1] )->getSPS()->getViewId() != uiCurrViewId };
    Bool            bUsesInterViewPrd = ( abCurrIntView[0] || abCurrIntView[1] );
    if( bUsesInterViewPrd )
    { //===== set resiudal to zero =====
      xClearResidual( pcCUResidual, uiAbsPartIdx, (UInt)iWidth, (UInt)iHeight );
    }
  }
}


Void  
TComResidualGenerator::xClearResidual( TComYuv* pcCUResidual, UInt uiAbsPartIdx, UInt uiWidth, UInt uiHeight ) 
{
  // luma
  Pel* pSamplesY = pcCUResidual->getLumaAddr( uiAbsPartIdx );
  Int  iStrideY  = pcCUResidual->getStride  ();
  for( UInt  uiY = 0; uiY < uiHeight; uiY++, pSamplesY += iStrideY )
  {
    ::memset( pSamplesY, 0x00, uiWidth * sizeof( Pel ) );
  }
  // chroma
  uiWidth      >>= 1;
  uiHeight     >>= 1;
  Pel* pSamplesU = pcCUResidual->getCbAddr ( uiAbsPartIdx );
  Pel* pSamplesV = pcCUResidual->getCrAddr ( uiAbsPartIdx );
  Int  iStrideC  = pcCUResidual->getCStride();
  for( UInt  uiY = 0; uiY < uiHeight; uiY++, pSamplesU += iStrideC, pSamplesV += iStrideC )
  {
    ::memset( pSamplesU, 0x00, uiWidth * sizeof( Pel ) );
    ::memset( pSamplesV, 0x00, uiWidth * sizeof( Pel ) );
  }
}


#if H3D_NBDV
#if MTK_RELEASE_DV_CONSTRAINT_C0129
Void  
TComResidualGenerator::xSetPredResidualBlock( TComPic* pcPic, UInt uiBaseViewId, UInt uiXPos, UInt uiYPos, UInt uiBlkWidth, UInt uiBlkHeight, TComYuv* pcYuv, TComMv iDisp
                                             ,UInt * puiXPosInRefView , UInt * puiYPosInRefView , Bool bRecon )
#else // MTK_RELEASE_DV_CONSTRAINT_C0129
Void  
TComResidualGenerator::xSetPredResidualBlock( TComPic* pcPic, UInt uiBaseViewId, UInt uiXPos, UInt uiYPos, UInt uiBlkWidth, UInt uiBlkHeight, TComYuv* pcYuv, Int iDisp 
                                             ,UInt * puiXPosInRefView , UInt * puiYPosInRefView , Bool bRecon)
#endif // MTK_RELEASE_DV_CONSTRAINT_C0129
#else // H3D_NBDV
Void  
TComResidualGenerator::xSetPredResidualBlock( TComPic* pcPic, UInt uiBaseViewId, UInt uiXPos, UInt uiYPos, UInt uiBlkWidth, UInt uiBlkHeight, TComYuv* pcYuv 
                                             , UInt * puiXPosInRefView , UInt * puiYPosInRefView , Bool bRecon  )
#endif // H3D_NBDV
{
  //===== set and check some basic variables =====
  AOF(          pcYuv     );
  TComPic*      pcBasePic   = m_pcAUPicAccess->getPic( uiBaseViewId );
  AOF(          pcPic     );
  AOF(          pcBasePic );
  TComPicYuv*   pcBaseRes   = pcBasePic->getResidual    ();
  TComPicYuv*   pcPdmMap    = pcPic    ->getPredDepthMap();
  AOF(          pcBaseRes );
  AOF(          pcPdmMap  );
  UInt          uiPicWidth  = pcBaseRes->getWidth ();
  UInt          uiPicHeight = pcBaseRes->getHeight();
  AOT( uiXPos + uiBlkWidth  > uiPicWidth  );
  AOT( uiYPos + uiBlkHeight > uiPicHeight );

  //===== get disparity =====
#if H3D_NBDV
#if MTK_RELEASE_DV_CONSTRAINT_C0129
  Int iDisparity_y = iDisp.getVer();
  Int iDisparity   = iDisp.getHor();
#else
  Int iDisparity = iDisp;
#endif
#else //H3D_NBDV
  Int           iMidPosX    = Int( uiXPos + ( ( uiBlkWidth  - 1 ) >> 1 ) ) >> m_pcDepthMapGenerator->getSubSampExpX();
  Int           iMidPosY    = Int( uiYPos + ( ( uiBlkHeight - 1 ) >> 1 ) ) >> m_pcDepthMapGenerator->getSubSampExpY();
  Int           iDisparity  = m_pcDepthMapGenerator->getDisparity( pcPic, iMidPosX, iMidPosY, uiBaseViewId );
#endif //H3D_NBDV
  //===== compensate luma =====
  Int           iYWidth     = Int( uiBlkWidth  );
  Int           iYHeight    = Int( uiBlkHeight );
  Int           iYWeight1   = ( iDisparity & 3 );
  Int           iYWeight0   = 4 - iYWeight1;
  Int           iYRefPosX0  = Int( uiXPos )     + ( iDisparity >> 2 );
  Int           iYRefPosX1  = iYRefPosX0        + 1;
#if MTK_RELEASE_DV_CONSTRAINT_C0129
  Int           iYMaxPosY   = Int( uiPicHeight ) - 1;
  Int           iYWeight3   = ( iDisparity_y & 3 );
  Int           iYWeight2   = 4 - iYWeight3;
  Int           iYRefPosY0  = Max( 0, Min( iYMaxPosY, Int( uiYPos )     + ( iDisparity_y >> 2 )) );
  Int           iYRefPosY1  = Max( 0, Min( iYMaxPosY, iYRefPosY0 + 1 ));
#endif
  Int           iYMaxPosX   = Int( uiPicWidth ) - 1;
  Int           iSrcStrideY = pcBaseRes->getStride   ();
  Int           iDesStrideY = pcYuv    ->getStride   ();
#if MTK_RELEASE_DV_CONSTRAINT_C0129
  Pel*          pSrcSamplesY0= pcBaseRes->getLumaAddr ( 0 ) + iYRefPosY0 * iSrcStrideY;
  Pel*          pSrcSamplesY1= pcBaseRes->getLumaAddr ( 0 ) + iYRefPosY1 * iSrcStrideY;
#else
  Pel*          pSrcSamplesY= pcBaseRes->getLumaAddr ( 0 ) + uiYPos * iSrcStrideY;
#endif
  Pel*          pDesSamplesY= pcYuv    ->getLumaAddr ();


  if( puiXPosInRefView != NULL )
    *puiXPosInRefView = Max( 0, Min( iYMaxPosX, iYRefPosX0 ) );
  if( puiYPosInRefView != NULL )
    *puiYPosInRefView = uiYPos;
  if( bRecon == false )
    return;

#if MTK_RELEASE_DV_CONSTRAINT_C0129
  for(   Int iY = 0; iY < iYHeight; iY++, pSrcSamplesY0 += iSrcStrideY, pSrcSamplesY1 += iSrcStrideY, pDesSamplesY += iDesStrideY )
#else
  for(   Int iY = 0; iY < iYHeight; iY++, pSrcSamplesY += iSrcStrideY, pDesSamplesY += iDesStrideY )
#endif
  {
    for( Int iX = 0; iX < iYWidth; iX++ )
    {
      Int iXPic0        = Max( 0, Min( iYMaxPosX, iYRefPosX0 + iX ) );
      Int iXPic1        = Max( 0, Min( iYMaxPosX, iYRefPosX1 + iX ) );
#if MTK_RELEASE_DV_CONSTRAINT_C0129
      Pel Temp1,Temp2;
      Temp1 =( iYWeight0 * pSrcSamplesY0[iXPic0] + iYWeight1 * pSrcSamplesY0[iXPic1] + 2 ) >> 2;
      Temp2 =( iYWeight0 * pSrcSamplesY1[iXPic0] + iYWeight1 * pSrcSamplesY1[iXPic1] + 2 ) >> 2;
      pDesSamplesY[iX]  = ( iYWeight2 * Temp1 + iYWeight3 * Temp2 + 2 ) >> 2;
#else
      pDesSamplesY[iX]  = ( iYWeight0 * pSrcSamplesY[iXPic0] + iYWeight1 * pSrcSamplesY[iXPic1] + 2 ) >> 2;
#endif
    }
  }

  //===== compensate chroma =====
  Int           iCWidth     = Int( uiBlkWidth  >> 1 );
  Int           iCHeight    = Int( uiBlkHeight >> 1 );
  Int           iCWeight1   = ( iDisparity & 7 );
  Int           iCWeight0   = 8 - iCWeight1;
  Int           iCRefPosX0  = Int( uiXPos     >> 1 ) + ( iDisparity >> 3 );
  Int           iCRefPosX1  = iCRefPosX0             + 1;
#if MTK_RELEASE_DV_CONSTRAINT_C0129
  Int           iCMaxPosY   = Int( uiPicHeight >> 1 ) - 1;
  Int           iCWeight3   = ( iDisparity_y & 7 );
  Int           iCWeight2   = 8 - iCWeight3;
  Int           iCRefPosY0  = Max( 0, Min( iCMaxPosY, Int( uiYPos >> 1 )     + ( iDisparity_y >> 3 )) );
  Int           iCRefPosY1  = Max( 0, Min( iCMaxPosY, iCRefPosY0 + 1 ));
#endif
  Int           iCMaxPosX   = Int( uiPicWidth >> 1 ) - 1;
  Int           iSrcStrideC = pcBaseRes->getCStride();
  Int           iDesStrideC = pcYuv    ->getCStride();
#if MTK_RELEASE_DV_CONSTRAINT_C0129
  Pel*          pSrcSamplesU0= pcBaseRes->getCbAddr ( 0 ) + ( iCRefPosY0 >> 1 ) * iSrcStrideC;
  Pel*          pSrcSamplesU1= pcBaseRes->getCbAddr ( 0 ) + ( iCRefPosY1 >> 1 ) * iSrcStrideC;
  Pel*          pSrcSamplesV0= pcBaseRes->getCrAddr ( 0 ) + ( iCRefPosY0 >> 1 ) * iSrcStrideC;
  Pel*          pSrcSamplesV1= pcBaseRes->getCrAddr ( 0 ) + ( iCRefPosY1 >> 1 ) * iSrcStrideC;
#else
  Pel*          pSrcSamplesU= pcBaseRes->getCbAddr ( 0 ) + ( uiYPos >> 1 ) * iSrcStrideC;
  Pel*          pSrcSamplesV= pcBaseRes->getCrAddr ( 0 ) + ( uiYPos >> 1 ) * iSrcStrideC;
#endif
  Pel*          pDesSamplesU= pcYuv    ->getCbAddr ();
  Pel*          pDesSamplesV= pcYuv    ->getCrAddr ();
#if MTK_RELEASE_DV_CONSTRAINT_C0129
  for(   Int iY = 0; iY < iCHeight; iY++, pSrcSamplesU0 += iSrcStrideC, pSrcSamplesU1 += iSrcStrideC, pDesSamplesU += iDesStrideC,
                                          pSrcSamplesV0 += iSrcStrideC, pSrcSamplesV1 += iSrcStrideC, pDesSamplesV += iDesStrideC )
#else
  for(   Int iY = 0; iY < iCHeight; iY++, pSrcSamplesU += iSrcStrideC, pDesSamplesU += iDesStrideC,
                                          pSrcSamplesV += iSrcStrideC, pDesSamplesV += iDesStrideC )
#endif
  {
    for( Int iX = 0; iX < iCWidth; iX++ )
    {
      Int iXPic0        = Max( 0, Min( iCMaxPosX, iCRefPosX0 + iX ) );
      Int iXPic1        = Max( 0, Min( iCMaxPosX, iCRefPosX1 + iX ) );
#if MTK_RELEASE_DV_CONSTRAINT_C0129
      Pel Temp1,Temp2;
      Temp1 =( iCWeight0 * pSrcSamplesU0[iXPic0] + iCWeight1 * pSrcSamplesU0[iXPic1] + 4 ) >> 3;
      Temp2 =( iCWeight0 * pSrcSamplesU1[iXPic0] + iCWeight1 * pSrcSamplesU1[iXPic1] + 4 ) >> 3;
      pDesSamplesU[iX]  = ( iCWeight2 * Temp1 + iCWeight3 * Temp2 + 4 ) >> 3;
      Temp1 =( iCWeight0 * pSrcSamplesV0[iXPic0] + iCWeight1 * pSrcSamplesV0[iXPic1] + 4 ) >> 3;
      Temp2 =( iCWeight0 * pSrcSamplesV1[iXPic0] + iCWeight1 * pSrcSamplesV1[iXPic1] + 4 ) >> 3;
      pDesSamplesV[iX]  = ( iCWeight2 * Temp1 + iCWeight3 * Temp2 + 4 ) >> 3;
#else
      pDesSamplesU[iX]  = ( iCWeight0 * pSrcSamplesU[iXPic0] + iCWeight1 * pSrcSamplesU[iXPic1] + 4 ) >> 3;
      pDesSamplesV[iX]  = ( iCWeight0 * pSrcSamplesV[iXPic0] + iCWeight1 * pSrcSamplesV[iXPic1] + 4 ) >> 3;
#endif
    }
  }
}


Bool
TComResidualGenerator::xIsNonZero( TComYuv* pcYuv, UInt uiBlkWidth, UInt uiBlkHeight )
{
  AOF( pcYuv );
  //===== check luma =====
  Int   iYWidth   = Int( uiBlkWidth  );
  Int   iYHeight  = Int( uiBlkHeight );
  Int   iStrideY  = pcYuv->getStride   ();
  Pel*  pSamplesY = pcYuv->getLumaAddr ();
  for(   Int iY = 0; iY < iYHeight; iY++, pSamplesY += iStrideY )
  {
    for( Int iX = 0; iX < iYWidth; iX++ )
    {
      ROTRS( pSamplesY[iX], true );
    }
  }
  //===== compensate chroma =====
  Int   iCWidth   = Int( uiBlkWidth  >> 1 );
  Int   iCHeight  = Int( uiBlkHeight >> 1 );
  Int   iStrideC  = pcYuv->getCStride();
  Pel*  pSamplesU = pcYuv->getCbAddr ();
  Pel*  pSamplesV = pcYuv->getCrAddr ();
  for(   Int iY = 0; iY < iCHeight; iY++, pSamplesU += iStrideC, pSamplesV += iStrideC )
  {
    for( Int iX = 0; iX < iCWidth; iX++ )
    {
      ROTRS( pSamplesU[iX], true );
      ROTRS( pSamplesV[iX], true );
    }
  }
  return false;
}



Void
TComResidualGenerator::xDumpResidual( TComPic* pcPic, char* pFilenameBase )
{
  AOF( m_bCreated && m_bInit );
  AOF( pcPic );
  AOF( pFilenameBase );
  AOF( m_uiOrgDepthBitDepth == 8 + g_uiBitIncrement );

  // convert to output format (just clip high absolute values, since they are very unlikely)
  Int         iMin        = 0;
  Int         iMax        = ( 1 << m_uiOrgDepthBitDepth )  - 1;
  Int         iMid        = ( 1 << m_uiOrgDepthBitDepth ) >> 1;
  UInt        uiViewId    = pcPic   ->getSPS      ()->getViewId();
  TComPicYuv* pcPicYuv    = pcPic   ->getResidual ();
  // luma
  Int         iWidth      = pcPicYuv->getWidth    ();
  Int         iHeight     = pcPicYuv->getHeight   ();
  Int         iSrcStride  = pcPicYuv->getStride   ();
  Int         iDstStride  = m_cTmpPic.getStride   ();
  Pel*        pSrcSamples = pcPicYuv->getLumaAddr ( 0 );
  Pel*        pDstSamples = m_cTmpPic.getLumaAddr ( 0 );
  AOF( m_cTmpPic.getWidth () == iWidth  );
  AOF( m_cTmpPic.getHeight() == iHeight );
  for( Int iY = 0; iY < iHeight; iY++, pSrcSamples += iSrcStride, pDstSamples += iDstStride )
  {
    for( Int iX = 0; iX < iWidth; iX++ )
    {
      pDstSamples[ iX ] = Max( iMin, Min( iMax, iMid + pSrcSamples[ iX ] ) );
    }
  }
  // chroma
  iWidth    >>= 1;
  iHeight   >>= 1;
  iSrcStride  = pcPicYuv->getCStride();
  iDstStride  = m_cTmpPic.getCStride();
  Pel* pSrcCb = pcPicYuv->getCbAddr ( 0 );
  Pel* pSrcCr = pcPicYuv->getCrAddr ( 0 );
  Pel* pDstCb = m_cTmpPic.getCbAddr ( 0 );
  Pel* pDstCr = m_cTmpPic.getCrAddr ( 0 );
  for( Int iY = 0; iY < iHeight; iY++, pSrcCb += iSrcStride, pSrcCr += iSrcStride, pDstCb += iDstStride, pDstCr += iDstStride )
  {
    for( Int iX = 0; iX < iWidth; iX++ )
    {
      pDstCb[ iX ] = Max( iMin, Min( iMax, iMid + pSrcCb[ iX ] ) );
      pDstCr[ iX ] = Max( iMin, Min( iMax, iMid + pSrcCr[ iX ] ) );
    }
  }

  // output
  Char  acFilename[1024];
  ::sprintf     ( acFilename, "%s_V%d.yuv", pFilenameBase, uiViewId );
  m_cTmpPic.dump( acFilename, ( pcPic->getPOC() != 0 )  );
}


#endif // H3D_IVRP

