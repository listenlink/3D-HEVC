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

/** \file     TComYuv.cpp
    \brief    general YUV buffer class
    \todo     this should be merged with TComPicYuv
*/

#include <stdlib.h>
#include <memory.h>
#include <assert.h>
#include <math.h>

#include "CommonDef.h"
#include "TComYuv.h"
#include "TComInterpolationFilter.h"

//! \ingroup TLibCommon
//! \{

TComYuv::TComYuv()
{
  for(Int comp=0; comp<MAX_NUM_COMPONENT; comp++)
  {
    m_apiBuf[comp] = NULL;
  }
}

TComYuv::~TComYuv()
{
}

Void TComYuv::create( UInt iWidth, UInt iHeight, ChromaFormat chromaFormatIDC )
{
  // set width and height
  m_iWidth   = iWidth;
  m_iHeight  = iHeight;
  m_chromaFormatIDC = chromaFormatIDC;

  for(Int comp=0; comp<MAX_NUM_COMPONENT; comp++)
  {
    // memory allocation
    m_apiBuf[comp]  = (Pel*)xMalloc( Pel, getWidth(ComponentID(comp))*getHeight(ComponentID(comp)) );
  }
}

Void TComYuv::destroy()
{
  // memory free
  for(Int comp=0; comp<MAX_NUM_COMPONENT; comp++)
  {
    if (m_apiBuf[comp]!=NULL)
    {
      xFree( m_apiBuf[comp] );
      m_apiBuf[comp] = NULL;
    }
  }
}

Void TComYuv::clear()
{
  for(Int comp=0; comp<MAX_NUM_COMPONENT; comp++)
  {
    if (m_apiBuf[comp]!=NULL)
    {
      ::memset( m_apiBuf[comp], 0, ( getWidth(ComponentID(comp)) * getHeight(ComponentID(comp))  )*sizeof(Pel) );
    }
  }
}




Void TComYuv::copyToPicYuv   ( TComPicYuv* pcPicYuvDst, const UInt ctuRsAddr, const UInt uiAbsZorderIdx, const UInt uiPartDepth, const UInt uiPartIdx ) const
{
  for(Int comp=0; comp<getNumberValidComponents(); comp++)
  {
    copyToPicComponent  ( ComponentID(comp), pcPicYuvDst, ctuRsAddr, uiAbsZorderIdx, uiPartDepth, uiPartIdx );
  }
}

Void TComYuv::copyToPicComponent  ( const ComponentID compID, TComPicYuv* pcPicYuvDst, const UInt ctuRsAddr, const UInt uiAbsZorderIdx, const UInt uiPartDepth, const UInt uiPartIdx ) const
{
  const Int iWidth  = getWidth(compID) >>uiPartDepth;
  const Int iHeight = getHeight(compID)>>uiPartDepth;

  const Pel* pSrc     = getAddr(compID, uiPartIdx, iWidth);
        Pel* pDst     = pcPicYuvDst->getAddr ( compID, ctuRsAddr, uiAbsZorderIdx );

  const UInt  iSrcStride  = getStride(compID);
  const UInt  iDstStride  = pcPicYuvDst->getStride(compID);

  for ( Int y = iHeight; y != 0; y-- )
  {
    ::memcpy( pDst, pSrc, sizeof(Pel)*iWidth);
#if ENC_DEC_TRACE && H_MV_ENC_DEC_TRAC
    if ( g_traceCopyBack && g_nSymbolCounter >= g_stopAtCounter )
    { 
      for ( Int x = 0; x < iWidth; x++)
      {      
        std::cout << pSrc[ x ] << " " ; 
      }
      std::cout << std::endl;
    }
#endif
    pDst += iDstStride;
    pSrc += iSrcStride;
  }
}




Void TComYuv::copyFromPicYuv   ( const TComPicYuv* pcPicYuvSrc, const UInt ctuRsAddr, const UInt uiAbsZorderIdx )
{
  for(Int comp=0; comp<getNumberValidComponents(); comp++)
  {
    copyFromPicComponent  ( ComponentID(comp), pcPicYuvSrc, ctuRsAddr, uiAbsZorderIdx );
  }
}

Void TComYuv::copyFromPicComponent  ( const ComponentID compID, const TComPicYuv* pcPicYuvSrc, const UInt ctuRsAddr, const UInt uiAbsZorderIdx )
{
        Pel* pDst     = getAddr(compID);
  const Pel* pSrc     = pcPicYuvSrc->getAddr ( compID, ctuRsAddr, uiAbsZorderIdx );

  const UInt iDstStride  = getStride(compID);
  const UInt iSrcStride  = pcPicYuvSrc->getStride(compID);
  const Int  iWidth=getWidth(compID);
  const Int  iHeight=getHeight(compID);

  for (Int y = iHeight; y != 0; y-- )
  {
    ::memcpy( pDst, pSrc, sizeof(Pel)*iWidth);
    pDst += iDstStride;
    pSrc += iSrcStride;
  }
}




Void TComYuv::copyToPartYuv( TComYuv* pcYuvDst, const UInt uiDstPartIdx ) const
{
  for(Int comp=0; comp<getNumberValidComponents(); comp++)
  {
    copyToPartComponent  ( ComponentID(comp), pcYuvDst, uiDstPartIdx );
  }
}

Void TComYuv::copyToPartComponent( const ComponentID compID, TComYuv* pcYuvDst, const UInt uiDstPartIdx ) const
{
  const Pel* pSrc     = getAddr(compID);
        Pel* pDst     = pcYuvDst->getAddr( compID, uiDstPartIdx );

  const UInt iSrcStride  = getStride(compID);
  const UInt iDstStride  = pcYuvDst->getStride(compID);
  const Int  iWidth=getWidth(compID);
  const Int  iHeight=getHeight(compID);

  for (Int y = iHeight; y != 0; y-- )
  {
    ::memcpy( pDst, pSrc, sizeof(Pel)*iWidth);
    pDst += iDstStride;
    pSrc += iSrcStride;
  }
}




Void TComYuv::copyPartToYuv( TComYuv* pcYuvDst, const UInt uiSrcPartIdx ) const
{
  for(Int comp=0; comp<getNumberValidComponents(); comp++)
  {
    copyPartToComponent  ( ComponentID(comp), pcYuvDst, uiSrcPartIdx );
  }
}

Void TComYuv::copyPartToComponent( const ComponentID compID, TComYuv* pcYuvDst, const UInt uiSrcPartIdx ) const
{
  const Pel* pSrc     = getAddr(compID, uiSrcPartIdx);
        Pel* pDst     = pcYuvDst->getAddr(compID, 0 );

  const UInt  iSrcStride  = getStride(compID);
  const UInt  iDstStride  = pcYuvDst->getStride(compID);

  const UInt uiHeight = pcYuvDst->getHeight(compID);
  const UInt uiWidth = pcYuvDst->getWidth(compID);

  for ( UInt y = uiHeight; y != 0; y-- )
  {
    ::memcpy( pDst, pSrc, sizeof(Pel)*uiWidth);
    pDst += iDstStride;
    pSrc += iSrcStride;
  }
}




Void TComYuv::copyPartToPartYuv   ( TComYuv* pcYuvDst, const UInt uiPartIdx, const UInt iWidth, const UInt iHeight ) const
{
  for(Int comp=0; comp<getNumberValidComponents(); comp++)
  {
    copyPartToPartComponent   (ComponentID(comp), pcYuvDst, uiPartIdx, iWidth>>getComponentScaleX(ComponentID(comp)), iHeight>>getComponentScaleY(ComponentID(comp)) );
  }
}

Void TComYuv::copyPartToPartComponent  ( const ComponentID compID, TComYuv* pcYuvDst, const UInt uiPartIdx, const UInt iWidthComponent, const UInt iHeightComponent ) const
{
  const Pel* pSrc =           getAddr(compID, uiPartIdx);
        Pel* pDst = pcYuvDst->getAddr(compID, uiPartIdx);
  if( pSrc == pDst )
  {
    //th not a good idea
    //th best would be to fix the caller
    return ;
  }

  const UInt  iSrcStride = getStride(compID);
  const UInt  iDstStride = pcYuvDst->getStride(compID);
  for ( UInt y = iHeightComponent; y != 0; y-- )
  {
    ::memcpy( pDst, pSrc, iWidthComponent * sizeof(Pel) );
    pSrc += iSrcStride;
    pDst += iDstStride;
  }
}




Void TComYuv::copyPartToPartComponentMxN  ( const ComponentID compID, TComYuv* pcYuvDst, const TComRectangle &rect) const
{
  const Pel* pSrc =           getAddrPix( compID, rect.x0, rect.y0 );
        Pel* pDst = pcYuvDst->getAddrPix( compID, rect.x0, rect.y0 );
  if( pSrc == pDst )
  {
    //th not a good idea
    //th best would be to fix the caller
    return ;
  }

  const UInt  iSrcStride = getStride(compID);
  const UInt  iDstStride = pcYuvDst->getStride(compID);
  const UInt uiHeightComponent=rect.height;
  const UInt uiWidthComponent=rect.width;
  for ( UInt y = uiHeightComponent; y != 0; y-- )
  {
    ::memcpy( pDst, pSrc, uiWidthComponent * sizeof( Pel ) );
    pSrc += iSrcStride;
    pDst += iDstStride;
  }
}




Void TComYuv::addClip( const TComYuv* pcYuvSrc0, const TComYuv* pcYuvSrc1, const UInt uiTrUnitIdx, const UInt uiPartSize, const BitDepths &clipBitDepths )
{
  for(Int comp=0; comp<getNumberValidComponents(); comp++)
  {
    const ComponentID compID=ComponentID(comp);
    const Int uiPartWidth =uiPartSize>>getComponentScaleX(compID);
    const Int uiPartHeight=uiPartSize>>getComponentScaleY(compID);

    const Pel* pSrc0 = pcYuvSrc0->getAddr(compID, uiTrUnitIdx, uiPartWidth );
    const Pel* pSrc1 = pcYuvSrc1->getAddr(compID, uiTrUnitIdx, uiPartWidth );
          Pel* pDst  = getAddr(compID, uiTrUnitIdx, uiPartWidth );

    const UInt iSrc0Stride = pcYuvSrc0->getStride(compID);
    const UInt iSrc1Stride = pcYuvSrc1->getStride(compID);
    const UInt iDstStride  = getStride(compID);
    const Int clipbd = clipBitDepths.recon[toChannelType(compID)];
#if O0043_BEST_EFFORT_DECODING
    const Int bitDepthDelta = clipBitDepths.stream[toChannelType(compID)] - clipbd;
#endif

    for ( Int y = uiPartHeight-1; y >= 0; y-- )
    {
      for ( Int x = uiPartWidth-1; x >= 0; x-- )
      {
#if O0043_BEST_EFFORT_DECODING
        pDst[x] = Pel(ClipBD<Int>( Int(pSrc0[x]) + rightShiftEvenRounding<Pel>(pSrc1[x], bitDepthDelta), clipbd));
#else
        pDst[x] = Pel(ClipBD<Int>( Int(pSrc0[x]) + Int(pSrc1[x]), clipbd));
#endif
      }
      pSrc0 += iSrc0Stride;
      pSrc1 += iSrc1Stride;
      pDst  += iDstStride;
    }
  }
}




Void TComYuv::subtract( const TComYuv* pcYuvSrc0, const TComYuv* pcYuvSrc1, const UInt uiTrUnitIdx, const UInt uiPartSize )
{
  for(Int comp=0; comp<getNumberValidComponents(); comp++)
  {
    const ComponentID compID=ComponentID(comp);
    const Int uiPartWidth =uiPartSize>>getComponentScaleX(compID);
    const Int uiPartHeight=uiPartSize>>getComponentScaleY(compID);

    const Pel* pSrc0 = pcYuvSrc0->getAddr( compID, uiTrUnitIdx, uiPartWidth );
    const Pel* pSrc1 = pcYuvSrc1->getAddr( compID, uiTrUnitIdx, uiPartWidth );
          Pel* pDst  = getAddr( compID, uiTrUnitIdx, uiPartWidth );

    const Int  iSrc0Stride = pcYuvSrc0->getStride(compID);
    const Int  iSrc1Stride = pcYuvSrc1->getStride(compID);
    const Int  iDstStride  = getStride(compID);

    for (Int y = uiPartHeight-1; y >= 0; y-- )
    {
      for (Int x = uiPartWidth-1; x >= 0; x-- )
      {
        pDst[x] = pSrc0[x] - pSrc1[x];
      }
      pSrc0 += iSrc0Stride;
      pSrc1 += iSrc1Stride;
      pDst  += iDstStride;
    }
  }
}




Void TComYuv::addAvg( const TComYuv* pcYuvSrc0, const TComYuv* pcYuvSrc1, const UInt iPartUnitIdx, const UInt uiWidth, const UInt uiHeight, const BitDepths &clipBitDepths )
{
  for(Int comp=0; comp<getNumberValidComponents(); comp++)
  {
    const ComponentID compID=ComponentID(comp);
    const Pel* pSrc0  = pcYuvSrc0->getAddr( compID, iPartUnitIdx );
    const Pel* pSrc1  = pcYuvSrc1->getAddr( compID, iPartUnitIdx );
    Pel* pDst   = getAddr( compID, iPartUnitIdx );

    const UInt  iSrc0Stride = pcYuvSrc0->getStride(compID);
    const UInt  iSrc1Stride = pcYuvSrc1->getStride(compID);
    const UInt  iDstStride  = getStride(compID);
    const Int   clipbd      = clipBitDepths.recon[toChannelType(compID)];
    const Int   shiftNum    = std::max<Int>(2, (IF_INTERNAL_PREC - clipbd)) + 1;
    const Int   offset      = ( 1 << ( shiftNum - 1 ) ) + 2 * IF_INTERNAL_OFFS;

    const Int   iWidth      = uiWidth  >> getComponentScaleX(compID);
    const Int   iHeight     = uiHeight >> getComponentScaleY(compID);

    if (iWidth&1)
    {
      assert(0);
      exit(-1);
    }
    else if (iWidth&2)
    {
      for ( Int y = 0; y < iHeight; y++ )
      {
        for (Int x=0 ; x < iWidth; x+=2 )
        {
          pDst[ x + 0 ] = ClipBD( rightShift(( pSrc0[ x + 0 ] + pSrc1[ x + 0 ] + offset ), shiftNum), clipbd );
          pDst[ x + 1 ] = ClipBD( rightShift(( pSrc0[ x + 1 ] + pSrc1[ x + 1 ] + offset ), shiftNum), clipbd );
        }
        pSrc0 += iSrc0Stride;
        pSrc1 += iSrc1Stride;
        pDst  += iDstStride;
      }
    }
    else
    {
      for ( Int y = 0; y < iHeight; y++ )
      {
        for (Int x=0 ; x < iWidth; x+=4 )
        {
          pDst[ x + 0 ] = ClipBD( rightShift(( pSrc0[ x + 0 ] + pSrc1[ x + 0 ] + offset ), shiftNum), clipbd );
          pDst[ x + 1 ] = ClipBD( rightShift(( pSrc0[ x + 1 ] + pSrc1[ x + 1 ] + offset ), shiftNum), clipbd );
          pDst[ x + 2 ] = ClipBD( rightShift(( pSrc0[ x + 2 ] + pSrc1[ x + 2 ] + offset ), shiftNum), clipbd );
          pDst[ x + 3 ] = ClipBD( rightShift(( pSrc0[ x + 3 ] + pSrc1[ x + 3 ] + offset ), shiftNum), clipbd );
        }
        pSrc0 += iSrc0Stride;
        pSrc1 += iSrc1Stride;
        pDst  += iDstStride;
      }
    }
  }
}

Void TComYuv::removeHighFreq( const TComYuv* pcYuvSrc,
                              const UInt uiPartIdx,
                              const UInt uiWidth,
                              const UInt uiHeight,
                              const Int bitDepths[MAX_NUM_CHANNEL_TYPE],
                              const Bool bClipToBitDepths
                              )
{
  for(Int comp=0; comp<getNumberValidComponents(); comp++)
  {
    const ComponentID compID=ComponentID(comp);
    const Pel* pSrc  = pcYuvSrc->getAddr(compID, uiPartIdx);
    Pel* pDst  = getAddr(compID, uiPartIdx);

    const Int iSrcStride = pcYuvSrc->getStride(compID);
    const Int iDstStride = getStride(compID);
    const Int iWidth  = uiWidth >>getComponentScaleX(compID);
    const Int iHeight = uiHeight>>getComponentScaleY(compID);
    if (bClipToBitDepths)
    {
      const Int clipBd=bitDepths[toChannelType(compID)];
      for ( Int y = iHeight-1; y >= 0; y-- )
      {
        for ( Int x = iWidth-1; x >= 0; x-- )
        {
          pDst[x ] = ClipBD((2 * pDst[x]) - pSrc[x], clipBd);
        }
        pSrc += iSrcStride;
        pDst += iDstStride;
      }
    }
    else
    {
      for ( Int y = iHeight-1; y >= 0; y-- )
      {
        for ( Int x = iWidth-1; x >= 0; x-- )
        {
          pDst[x ] = (2 * pDst[x]) - pSrc[x];
        }
        pSrc += iSrcStride;
        pDst += iDstStride;
      }
    }
  }
}

#if NH_3D_VSO
Void TComYuv::addClipPartLuma( Int bitDepth, TComYuv* pcYuvSrc0, TComYuv* pcYuvSrc1, UInt uiTrUnitIdx, UInt uiPartSize )
{
  Int x, y;

  Pel* pSrc0 = pcYuvSrc0->getAddr( COMPONENT_Y, uiTrUnitIdx);
  Pel* pSrc1 = pcYuvSrc1->getAddr( COMPONENT_Y, uiTrUnitIdx);
  Pel* pDst  =            getAddr( COMPONENT_Y, uiTrUnitIdx);

  UInt iSrc0Stride = pcYuvSrc0->getStride( COMPONENT_Y );
  UInt iSrc1Stride = pcYuvSrc1->getStride( COMPONENT_Y );
  UInt iDstStride  =            getStride( COMPONENT_Y );
  for ( y = uiPartSize-1; y >= 0; y-- )
  {
    for ( x = uiPartSize-1; x >= 0; x-- )
    {
      pDst[x] = ClipBD( pSrc0[x] + pSrc1[x], bitDepth );      
    }
    pSrc0 += iSrc0Stride;
    pSrc1 += iSrc1Stride;
    pDst  += iDstStride;
  }
}

#if NH_3D_ARP
Void TComYuv::addARP( TComYuv* pcYuvSrc0, TComYuv* pcYuvSrc1, UInt uiAbsPartIdx, UInt uiWidth, UInt uiHeight, Bool bClip, const BitDepths &clipBitDepths )
{
  addARPLuma   ( pcYuvSrc0, pcYuvSrc1, uiAbsPartIdx, uiWidth   , uiHeight    , bClip , clipBitDepths);
  addARPChroma ( pcYuvSrc0, pcYuvSrc1, uiAbsPartIdx, uiWidth>>1, uiHeight>>1 , bClip , clipBitDepths);
}

Void TComYuv::addARPLuma( TComYuv* pcYuvSrc0, TComYuv* pcYuvSrc1, UInt uiAbsPartIdx, UInt uiWidth, UInt uiHeight, Bool bClip, const BitDepths &clipBitDepths )
{
  Int x, y;

  Pel* pSrc0 = pcYuvSrc0->getAddr( COMPONENT_Y, uiAbsPartIdx );
  Pel* pSrc1 = pcYuvSrc1->getAddr( COMPONENT_Y, uiAbsPartIdx );
  Pel* pDst  = getAddr( COMPONENT_Y, uiAbsPartIdx );

  UInt iSrc0Stride = pcYuvSrc0->getStride(COMPONENT_Y);
  UInt iSrc1Stride = pcYuvSrc1->getStride(COMPONENT_Y);
  UInt iDstStride  = getStride(COMPONENT_Y);
  const Int clipbd = clipBitDepths.recon[CHANNEL_TYPE_LUMA];
  Int iIFshift = IF_INTERNAL_PREC - clipbd;
  Int iOffSet  = ( 1 << ( iIFshift - 1 ) ) + IF_INTERNAL_OFFS;

  for ( y = uiHeight-1; y >= 0; y-- )
  {
    for ( x = uiWidth-1; x >= 0; x-- )
    {
      pDst[x] = pSrc0[x] + pSrc1[x];
      if( bClip )
      {
        pDst[x] = Pel(ClipBD<Int>(Int( ( pDst[x] + iOffSet ) >> iIFshift ), clipbd));
      }
    }
    pSrc0 += iSrc0Stride;
    pSrc1 += iSrc1Stride;
    pDst  += iDstStride;
  }
}

Void TComYuv::addARPChroma( TComYuv* pcYuvSrc0, TComYuv* pcYuvSrc1, UInt uiAbsPartIdx, UInt uiWidth, UInt uiHeight, Bool bClip, const BitDepths &clipBitDepths )
{
  Int x, y;

  Pel* pSrcU0 = pcYuvSrc0->getAddr( COMPONENT_Cb, uiAbsPartIdx );
  Pel* pSrcU1 = pcYuvSrc1->getAddr( COMPONENT_Cb, uiAbsPartIdx );
  Pel* pSrcV0 = pcYuvSrc0->getAddr( COMPONENT_Cr, uiAbsPartIdx );
  Pel* pSrcV1 = pcYuvSrc1->getAddr( COMPONENT_Cr, uiAbsPartIdx );
  Pel* pDstU = getAddr( COMPONENT_Cb, uiAbsPartIdx );
  Pel* pDstV = getAddr( COMPONENT_Cr, uiAbsPartIdx );

  UInt  iSrc0StrideCb = pcYuvSrc0->getStride(COMPONENT_Cb);
  UInt  iSrc1StrideCb = pcYuvSrc1->getStride(COMPONENT_Cb);
  UInt  iDstStrideCb  = getStride(COMPONENT_Cb);

  UInt  iSrc0StrideCr = pcYuvSrc0->getStride(COMPONENT_Cr);
  UInt  iSrc1StrideCr = pcYuvSrc1->getStride(COMPONENT_Cr);
  UInt  iDstStrideCr  = getStride(COMPONENT_Cr);

  const Int clipbd = clipBitDepths.recon[CHANNEL_TYPE_CHROMA];
  Int iIFshift = IF_INTERNAL_PREC - clipbd;
  Int iOffSet  = ( 1 << ( iIFshift - 1 ) ) + IF_INTERNAL_OFFS;

  for ( y = uiHeight-1; y >= 0; y-- )
  {
    for ( x = uiWidth-1; x >= 0; x-- )
    {
      pDstU[x] = pSrcU0[x] + pSrcU1[x];
      pDstV[x] = pSrcV0[x] + pSrcV1[x];
      if( bClip )
      {
        pDstU[x] = Pel(ClipBD<Int>( Int( ( pDstU[x] + iOffSet ) >> iIFshift ), clipbd));
        pDstV[x] = Pel(ClipBD<Int>( Int( ( pDstV[x] + iOffSet ) >> iIFshift ), clipbd));
      }
    }

    pSrcU0 += iSrc0StrideCb;
    pSrcU1 += iSrc1StrideCb;
    pSrcV0 += iSrc0StrideCr;
    pSrcV1 += iSrc1StrideCr;
    pDstU  += iDstStrideCb;
    pDstV  += iDstStrideCr;
  }
}

Void TComYuv::subtractARP( TComYuv* pcYuvSrc0, TComYuv* pcYuvSrc1, UInt uiAbsPartIdx, UInt uiWidth , UInt uiHeight )
{
  subtractARPLuma  ( pcYuvSrc0, pcYuvSrc1,  uiAbsPartIdx, uiWidth    , uiHeight    );

  if (uiWidth > 8 && pcYuvSrc1->getNumberValidComponents() > 1)
  {
    subtractARPChroma( pcYuvSrc0, pcYuvSrc1,  uiAbsPartIdx, uiWidth>>1 , uiHeight>>1 );
  }
}

Void TComYuv::subtractARPLuma( TComYuv* pcYuvSrc0, TComYuv* pcYuvSrc1, UInt uiAbsPartIdx, UInt uiWidth , UInt uiHeight )
{
  Int x, y;

  Pel* pSrc0 = pcYuvSrc0->getAddr(COMPONENT_Y, uiAbsPartIdx );
  Pel* pSrc1 = pcYuvSrc1->getAddr(COMPONENT_Y, uiAbsPartIdx );
  Pel* pDst  = getAddr           (COMPONENT_Y, uiAbsPartIdx );

  Int  iSrc0Stride = pcYuvSrc0->getStride(COMPONENT_Y);
  Int  iSrc1Stride = pcYuvSrc1->getStride(COMPONENT_Y);
  Int  iDstStride  = getStride(COMPONENT_Y);
  for ( y = uiHeight-1; y >= 0; y-- )
  {
    for ( x = uiWidth-1; x >= 0; x-- )
    {
      pDst[x] = pSrc0[x] - pSrc1[x];
    }
    pSrc0 += iSrc0Stride;
    pSrc1 += iSrc1Stride;
    pDst  += iDstStride;
  }
}

Void TComYuv::subtractARPChroma( TComYuv* pcYuvSrc0, TComYuv* pcYuvSrc1, UInt uiAbsPartIdx, UInt uiWidth , UInt uiHeight )
{
  Int x, y;

  Pel* pSrcU0 = pcYuvSrc0->getAddr(COMPONENT_Cb, uiAbsPartIdx );
  Pel* pSrcU1 = pcYuvSrc1->getAddr(COMPONENT_Cb, uiAbsPartIdx );
  Pel* pSrcV0 = pcYuvSrc0->getAddr(COMPONENT_Cr, uiAbsPartIdx );
  Pel* pSrcV1 = pcYuvSrc1->getAddr(COMPONENT_Cr, uiAbsPartIdx );
  Pel* pDstU  = getAddr(COMPONENT_Cb, uiAbsPartIdx );
  Pel* pDstV  = getAddr(COMPONENT_Cr, uiAbsPartIdx );

  Int  iSrc0Stride = pcYuvSrc0->getStride(COMPONENT_Cb);
  Int  iSrc1Stride = pcYuvSrc1->getStride(COMPONENT_Cb);
  Int  iDstStride  = getStride( COMPONENT_Cb );
  for ( y = uiHeight-1; y >= 0; y-- )
  {
    for ( x = uiWidth-1; x >= 0; x-- )
    {
      pDstU[x] = pSrcU0[x] - pSrcU1[x];
      pDstV[x] = pSrcV0[x] - pSrcV1[x];
    }
    pSrcU0 += iSrc0Stride;
    pSrcU1 += iSrc1Stride;
    pSrcV0 += iSrc0Stride;
    pSrcV1 += iSrc1Stride;
    pDstU  += iDstStride;
    pDstV  += iDstStride;
  }
}

Void TComYuv::multiplyARP( UInt uiAbsPartIdx , UInt uiWidth , UInt uiHeight , UChar dW )
{
  multiplyARPLuma( uiAbsPartIdx , uiWidth , uiHeight , dW );

  if ( uiWidth > 8 && getNumberValidComponents() > 1 )
  {
    multiplyARPChroma( uiAbsPartIdx , uiWidth >> 1 , uiHeight >> 1 , dW );
  }
}

Void TComYuv::xxMultiplyLine( Pel* pSrcDst , UInt uiWidth , UChar dW )
{
  assert( dW == 2 );
  for( UInt x = 0 ; x < uiWidth ; x++ )
    pSrcDst[x] =  pSrcDst[x] >> 1;
}

Void TComYuv::multiplyARPLuma( UInt uiAbsPartIdx , UInt uiWidth , UInt uiHeight , UChar dW )
{
  Pel* pDst  = getAddr(COMPONENT_Y, uiAbsPartIdx );
  Int  iDstStride  = getStride(COMPONENT_Y);
  for ( Int y = uiHeight-1; y >= 0; y-- )
  {
    xxMultiplyLine( pDst , uiWidth , dW );
    pDst  += iDstStride;
  }
}

Void TComYuv::multiplyARPChroma( UInt uiAbsPartIdx , UInt uiWidth , UInt uiHeight , UChar dW )
{
  Pel* pDstU  = getAddr( COMPONENT_Cb, uiAbsPartIdx );
  Pel* pDstV  = getAddr( COMPONENT_Cr, uiAbsPartIdx );

  Int  iDstStride  = getStride( COMPONENT_Cb );
  for ( Int y = uiHeight-1; y >= 0; y-- )
  {
    xxMultiplyLine( pDstU , uiWidth , dW );
    xxMultiplyLine( pDstV , uiWidth , dW );
    pDstU  += iDstStride;
    pDstV  += iDstStride;
  }
}
#endif
#endif

//! \}
