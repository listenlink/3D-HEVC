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



/** \file     TRenInterpFilter.h
    \brief    interpolation filter class (header), (From HM 3.0 TComPredFilter))
*/

#ifndef __TRENINTERPFILTER__
#define __TRENINTERPFILTER__

// Include files
#include "TLibCommon/CommonDef.h"
#include "assert.h"
#if NH_3D_VSO

// ====================================================================================================================
// Constants
// ====================================================================================================================

// Local type definitions
#define HAL_IDX   1
#define QU0_IDX   0
#define QU1_IDX   2

// ====================================================================================================================
// Class definition
// ====================================================================================================================

/// interpolation filter class
template<UInt bitDepth>
class TRenInterpFilter
{
public:
  TRenInterpFilter();
  
  // DIF filter interface (for half & quarter)
  __inline Void xCTI_FilterHalfHor(Pel* piSrc, Int iSrcStride, Int iSrcStep, Int iWidth, Int iHeight, Int iDstStride, Int iDstStep, Pel*& rpiDst);
  __inline Void xCTI_FilterHalfHor(Int* piSrc, Int iSrcStride, Int iSrcStep, Int iWidth, Int iHeight, Int iDstStride, Int iDstStep, Pel*& rpiDst);
  
  __inline Void xCTI_FilterQuarter0Hor(Pel* piSrc, Int iSrcStride, Int iSrcStep, Int iWidth, Int iHeight, Int iDstStride, Int iDstStep, Pel*& rpiDst);
  __inline Void xCTI_FilterQuarter0Hor(Int* piSrc, Int iSrcStride, Int iSrcStep, Int iWidth, Int iHeight, Int iDstStride, Int iDstStep, Pel*& rpiDst);
  
  __inline Void xCTI_FilterQuarter1Hor(Pel* piSrc, Int iSrcStride, Int iSrcStep, Int iWidth, Int iHeight, Int iDstStride, Int iDstStep, Pel*& rpiDst);
  __inline Void xCTI_FilterQuarter1Hor(Int* piSrc, Int iSrcStride, Int iSrcStep, Int iWidth, Int iHeight, Int iDstStride, Int iDstStep, Pel*& rpiDst);
  
  __inline Void xCTI_FilterHalfVer (Pel* piSrc, Int iSrcStride, Int iSrcStep, Int iWidth, Int iHeight, Int iDstStride, Int iDstStep, Int*& rpiDst, Int iDstStridePel, Pel*& rpiDstPel );
  __inline Void xCTI_FilterHalfVer (Pel* piSrc, Int iSrcStride, Int iSrcStep, Int iWidth, Int iHeight, Int iDstStride, Int iDstStep, Int*& rpiDst );
  __inline Void xCTI_FilterHalfVer (Pel* piSrc, Int iSrcStride, Int iSrcStep, Int iWidth, Int iHeight, Int iDstStride, Int iDstStep, Pel*& rpiDst );
  
  __inline Void xCTI_FilterQuarter0Ver (Pel* piSrc, Int iSrcStride, Int iSrcStep, Int iWidth, Int iHeight, Int iDstStride, Int iDstStep, Int*& rpiDst );
  __inline Void xCTI_FilterQuarter0Ver (Pel* piSrc, Int iSrcStride, Int iSrcStep, Int iWidth, Int iHeight, Int iDstStride, Int iDstStep, Pel*& rpiDst );
  
  __inline Void xCTI_FilterQuarter1Ver (Pel* piSrc, Int iSrcStride, Int iSrcStep, Int iWidth, Int iHeight, Int iDstStride, Int iDstStep, Int*& rpiDst );
  __inline Void xCTI_FilterQuarter1Ver (Pel* piSrc, Int iSrcStride, Int iSrcStep, Int iWidth, Int iHeight, Int iDstStride, Int iDstStep, Pel*& rpiDst );
  
  __inline Void xCTI_Filter2DVerC (Pel* piSrc, Int iSrcStride,  Int iWidth, Int iHeight, Int iDstStride,  Int*& rpiDst, Int iMv);
  __inline Void xCTI_Filter2DHorC (Int* piSrc, Int iSrcStride,  Int iWidth, Int iHeight, Int iDstStride,  Pel*& rpiDst, Int iMV);
  __inline Void xCTI_Filter1DHorC (Pel* piSrc, Int iSrcStride,  Int iWidth, Int iHeight, Int iDstStride,  Pel*& rpiDst, Int iMV);
  __inline Void xCTI_Filter1DVerC (Pel* piSrc, Int iSrcStride,  Int iWidth, Int iHeight, Int iDstStride,  Pel*& rpiDst, Int iMV);

   __inline Int xCTI_Filter_VPS04_C_HAL( Pel* pSrc, Int iStride );
   __inline Int xCTI_Filter_VIS04_C_HAL( Int* pSrc, Int iStride );
   __inline Int xCTI_Filter_VP04_C_OCT0( Pel* pSrc, Int iStride );
   __inline Int xCTI_Filter_VI04_C_OCT0( Int* pSrc, Int iStride );
   __inline Int xCTI_Filter_VP04_C_QUA0( Pel* pSrc, Int iStride );
   __inline Int xCTI_Filter_VI04_C_QUA0( Int* pSrc, Int iStride );
   __inline Int xCTI_Filter_VP04_C_OCT1( Pel* pSrc, Int iStride );
   __inline Int xCTI_Filter_VI04_C_OCT1( Int* pSrc, Int iStride );
   __inline Int xCTI_Filter_VP04_C_OCT2( Pel* pSrc, Int iStride );
   __inline Int xCTI_Filter_VI04_C_OCT2( Int* pSrc, Int iStride );
   __inline Int xCTI_Filter_VP04_C_QUA1( Pel* pSrc, Int iStride );
   __inline Int xCTI_Filter_VI04_C_QUA1( Int* pSrc, Int iStride );
   __inline Int xCTI_Filter_VP04_C_OCT3( Pel* pSrc, Int iStride );
   __inline Int xCTI_Filter_VI04_C_OCT3( Int* pSrc, Int iStride );
private:
  __inline Int xClipY( Pel x ) { return std::min<Pel>(Pel((1 << bitDepth)-1), std::max<Pel>( Pel(0), x)); } 
  __inline Int xClipC( Pel x ) { return std::min<Pel>(Pel((1 << bitDepth)-1), std::max<Pel>( Pel(0), x)); }

};


// ------------------------------------------------------------------------------------------------
// DCTIF filters
// ------------------------------------------------------------------------------------------------
template<UInt bitDepth>
__inline Void TRenInterpFilter<bitDepth>::xCTI_FilterHalfHor(Pel* piSrc, Int iSrcStride, Int iSrcStep, Int iWidth, Int iHeight, Int iDstStride, Int iDstStep, Pel*& rpiDst)
{
  Pel*  piDst    = rpiDst;
  Int   iSum;
  Pel*  piSrcTmp;
  Int iSrcStep2 = iSrcStep*2;
  Int iSrcStep3 = iSrcStep*3;
  Int iSrcStep4 = iSrcStep*4;
  Int iSrcStep5 = iSrcStep*5;
  Int iSrcStep6 = iSrcStep*6;
  Int iSrcStep7 = iSrcStep*7;

  Int iTmp0, iTmp1, iTmp2, iTmp3, iTmpA;

  for ( Int y = iHeight; y != 0; y-- )
  {
    piSrcTmp = &piSrc[ -3*iSrcStep ];
    for ( Int x = 0; x < iWidth; x++ )
    {
      // { -1,4,-11,40,40,-11,4,-1   } 
      iTmp0 = piSrcTmp[        0]+piSrcTmp[iSrcStep7];
      iTmp1 = piSrcTmp[iSrcStep]+piSrcTmp[iSrcStep6];
      iTmp2 = piSrcTmp[iSrcStep2]+piSrcTmp[iSrcStep5];
      iTmp3 = piSrcTmp[iSrcStep3]+piSrcTmp[iSrcStep4];

      iTmpA = (iTmp3 << 2) - iTmp2;

      iSum  = (   iTmp1          << 2 )
            + (   iTmpA          << 3 )
            + (   iTmpA          << 1 )
            -    iTmp0 -  iTmp2;

      piDst   [x * iDstStep] = xClipY( (iSum +  32) >>  6 );
      piSrcTmp += iSrcStep;
    }
    piSrc += iSrcStride;
    piDst += iDstStride;
  }
  return;
}

template<UInt bitDepth>
__inline Void TRenInterpFilter<bitDepth>::xCTI_FilterHalfHor(Int* piSrc, Int iSrcStride, Int iSrcStep, Int iWidth, Int iHeight, Int iDstStride, Int iDstStep, Pel*& rpiDst)
{
  Pel*  piDst    = rpiDst;
  Int   iSum;
  Int*  piSrcTmp;
  Int iSrcStep2 = iSrcStep*2;
  Int iSrcStep3 = iSrcStep*3;
  Int iSrcStep4 = iSrcStep*4;
  Int iSrcStep5 = iSrcStep*5;
  Int iSrcStep6 = iSrcStep*6;
  Int iSrcStep7 = iSrcStep*7;

  Int iTmp0, iTmp1, iTmp2, iTmp3, iTmpA;

  for ( Int y = iHeight; y != 0; y-- )
  {
    piSrcTmp = &piSrc[ -3*iSrcStep ];
    for ( Int x = 0; x < iWidth; x++ )
    {
      // { -1,4,-11,40,40,-11,4,-1   } 
      iTmp0 = piSrcTmp[        0]+piSrcTmp[iSrcStep7];
      iTmp1 = piSrcTmp[iSrcStep ]+piSrcTmp[iSrcStep6];
      iTmp2 = piSrcTmp[iSrcStep2]+piSrcTmp[iSrcStep5];
      iTmp3 = piSrcTmp[iSrcStep3]+piSrcTmp[iSrcStep4];
      
      iTmpA = (iTmp3 << 2) - iTmp2;
      
      iSum  = (   iTmp1          << 2 )
            + (   iTmpA          << 3 )
            + (   iTmpA          << 1 )
            -    iTmp0 -  iTmp2;
      
      piDst   [x * iDstStep] = xClipY( (iSum +  2048) >>  12 );
      piSrcTmp += iSrcStep;
    }
    piSrc += iSrcStride;
    piDst += iDstStride;
  }
  return;
}

template<UInt bitDepth>
__inline Void TRenInterpFilter<bitDepth>::xCTI_FilterQuarter0Hor(Pel* piSrc, Int iSrcStride, Int iSrcStep, Int iWidth, Int iHeight, Int iDstStride, Int iDstStep, Pel*& rpiDst)
{  
  Pel*  piDst    = rpiDst;
  Int   iSum;
  Pel*  piSrcTmp;
  Int iSrcStep2 = iSrcStep*2;
  Int iSrcStep3 = iSrcStep*3;
  Int iSrcStep4 = iSrcStep*4;
  Int iSrcStep5 = iSrcStep*5;
  Int iSrcStep6 = iSrcStep*6;
  Int iSrcStep7 = iSrcStep*7;

  Int  iTmp1, iTmp2;

  for ( Int y = iHeight; y != 0; y-- )
  {
    piSrcTmp = &piSrc[ -3*iSrcStep ];
    for ( Int x = 0; x < iWidth; x++ )
    {
      // {-1,   4,  -10,  57,   19,  -7,   3,   -1  },
      
      iTmp1 = piSrcTmp[iSrcStep3] + piSrcTmp[iSrcStep5];
      iTmp2 = piSrcTmp[iSrcStep6] + piSrcTmp[iSrcStep4];
      
      iSum  =  iTmp1 + iTmp2 - piSrcTmp[0] - piSrcTmp[iSrcStep7]
             - ( ( piSrcTmp[iSrcStep2] - iTmp2 ) << 1 )
             + (  piSrcTmp[iSrcStep]             << 2 )
             - ( ( piSrcTmp[iSrcStep2] + iTmp1 ) << 3 )
             + (   piSrcTmp[iSrcStep4]           << 4 );
      
      piDst   [x * iDstStep] = xClipY(( (iSum +  32) >>  6 )+ piSrcTmp[iSrcStep3]);
      piSrcTmp += iSrcStep;
    }
    piSrc += iSrcStride;
    piDst += iDstStride;
  }
  return;
}

template<UInt bitDepth>
__inline Void TRenInterpFilter<bitDepth>::xCTI_FilterQuarter0Hor(Int* piSrc, Int iSrcStride, Int iSrcStep, Int iWidth, Int iHeight, Int iDstStride, Int iDstStep, Pel*& rpiDst)
{
  Pel*  piDst    = rpiDst;
  Int   iSum;
  Int*  piSrcTmp;
  Int iSrcStep2 = iSrcStep*2;
  Int iSrcStep3 = iSrcStep*3;
  Int iSrcStep4 = iSrcStep*4;
  Int iSrcStep5 = iSrcStep*5;
  Int iSrcStep6 = iSrcStep*6;
  Int iSrcStep7 = iSrcStep*7;

  Int  iTmp1, iTmp2;

  for ( Int y = iHeight; y != 0; y-- )
  {
    piSrcTmp = &piSrc[ -3*iSrcStep ];
    for ( Int x = 0; x < iWidth; x++ )
    {
      // {-1,   4,  -10,  57,   19,  -7,   3,   -1  },
    
      iTmp1 = piSrcTmp[iSrcStep3] + piSrcTmp[iSrcStep5];
      iTmp2 = piSrcTmp[iSrcStep6] + piSrcTmp[iSrcStep4];
      
      iSum  =  iTmp1 + iTmp2 - piSrcTmp[0] - piSrcTmp[iSrcStep7]
            - ( ( piSrcTmp[iSrcStep2] - iTmp2 ) << 1 )
            + (  piSrcTmp[iSrcStep]             << 2 )
            - ( ( piSrcTmp[iSrcStep2] + iTmp1 ) << 3 )
            + (   piSrcTmp[iSrcStep4]           << 4 )
            + (   piSrcTmp[iSrcStep3]           << 6 );
      
      piDst   [x * iDstStep] = xClipY( (iSum +  2048) >>  12 );
      piSrcTmp += iSrcStep;
    }
    piSrc += iSrcStride;
    piDst += iDstStride;
  }
  return;
}

template<UInt bitDepth>
__inline Void TRenInterpFilter<bitDepth>::xCTI_FilterQuarter1Hor(Pel* piSrc, Int iSrcStride, Int iSrcStep, Int iWidth, Int iHeight, Int iDstStride, Int iDstStep, Pel*& rpiDst)
{
  Pel*  piDst    = rpiDst;
  Int   iSum;
  Pel*  piSrcTmp;
  Int iSrcStep2 = iSrcStep*2;
  Int iSrcStep3 = iSrcStep*3;
  Int iSrcStep4 = iSrcStep*4;
  Int iSrcStep5 = iSrcStep*5;
  Int iSrcStep6 = iSrcStep*6;
  Int iSrcStep7 = iSrcStep*7;

  Int  iTmp1, iTmp2;
  for ( Int y = iHeight; y != 0; y-- )
  {
    piSrcTmp = &piSrc[ -3*iSrcStep ];
    for ( Int x = 0; x < iWidth; x++ )
    {
      // {-1,   3,  -7,  19,   57,  -10,   4,   -1  },
      
      iTmp1 = piSrcTmp[iSrcStep4] + piSrcTmp[iSrcStep2];
      iTmp2 = piSrcTmp[iSrcStep ] + piSrcTmp[iSrcStep3];
      
      iSum  =  iTmp1 + iTmp2 - piSrcTmp[0] - piSrcTmp[iSrcStep7]
            - ( ( piSrcTmp[iSrcStep5] - iTmp2 ) << 1 )
            + (   piSrcTmp[iSrcStep6]           << 2 )
            - ( ( piSrcTmp[iSrcStep5] + iTmp1 ) << 3 )
            + (   piSrcTmp[iSrcStep3]           << 4 );
      
      piDst   [x * iDstStep] = xClipY( ((iSum +  32) >>  6) + piSrcTmp[iSrcStep4] );
      piSrcTmp += iSrcStep;
    }
    piSrc += iSrcStride;
    piDst += iDstStride;
  }
  return;
}

template<UInt bitDepth>
__inline Void TRenInterpFilter<bitDepth>::xCTI_FilterQuarter1Hor(Int* piSrc, Int iSrcStride, Int iSrcStep, Int iWidth, Int iHeight, Int iDstStride, Int iDstStep, Pel*& rpiDst)
{
  Pel*  piDst    = rpiDst;
  Int   iSum;
  Int*  piSrcTmp;
  Int iSrcStep2 = iSrcStep*2;
  Int iSrcStep3 = iSrcStep*3;
  Int iSrcStep4 = iSrcStep*4;
  Int iSrcStep5 = iSrcStep*5;
  Int iSrcStep6 = iSrcStep*6;
  Int iSrcStep7 = iSrcStep*7;

  Int  iTmp1, iTmp2;
  for ( Int y = iHeight; y != 0; y-- )
  {
    piSrcTmp = &piSrc[ -3*iSrcStep ];
    for ( Int x = 0; x < iWidth; x++ )
    {
      // {-1,   3,  -7,  19,   57,  -10,   4,   -1  },
      
      iTmp1 = piSrcTmp[iSrcStep4] + piSrcTmp[iSrcStep2];
      iTmp2 = piSrcTmp[iSrcStep ] + piSrcTmp[iSrcStep3];
      
      iSum  =  iTmp1 + iTmp2 - piSrcTmp[0] - piSrcTmp[iSrcStep7]
            - ( ( piSrcTmp[iSrcStep5] - iTmp2 ) << 1 )
            + (   piSrcTmp[iSrcStep6]           << 2 )
            - ( ( piSrcTmp[iSrcStep5] + iTmp1 ) << 3 )
            + (   piSrcTmp[iSrcStep3]           << 4 )
            + (   piSrcTmp[iSrcStep4]           << 6 );
      
      piDst   [x * iDstStep] = xClipY( (iSum +  2048) >>  12 );
      piSrcTmp += iSrcStep;
    }
    piSrc += iSrcStride;
    piDst += iDstStride;
  }
  return;
}

template<UInt bitDepth>
__inline Void TRenInterpFilter<bitDepth>::xCTI_FilterHalfVer (Pel* piSrc, Int iSrcStride, Int iSrcStep, Int iWidth, Int iHeight, Int iDstStride, Int iDstStep, Int*& rpiDst, Int iDstStridePel, Pel*& rpiDstPel )
{
  Int*  piDst = rpiDst;
  Pel*  piDstPel = rpiDstPel;
  Int   iSum;
  Pel*  piSrcTmp;
  Int iSrcStride2 = iSrcStride*2;
  Int iSrcStride3 = iSrcStride*3;
  Int iSrcStride4 = iSrcStride*4;
  Int iSrcStride5 = iSrcStride*5;
  Int iSrcStride6 = iSrcStride*6;
  Int iSrcStride7 = iSrcStride*7;

  Int  iTmp0, iTmp1, iTmp2, iTmp3, iTmpA;
  for ( Int y = iHeight; y != 0; y-- )
  {
    piSrcTmp = &piSrc[ -3*iSrcStride ];
    for ( Int x = 0; x < iWidth; x++ )
    {
      // { -1,4,-11,40,40,-11,4,-1   } 
      iTmp0 = piSrcTmp[          0]+piSrcTmp[iSrcStride7];
      iTmp1 = piSrcTmp[iSrcStride ]+piSrcTmp[iSrcStride6];
      iTmp2 = piSrcTmp[iSrcStride2]+piSrcTmp[iSrcStride5];
      iTmp3 = piSrcTmp[iSrcStride3]+piSrcTmp[iSrcStride4];
      
      iTmpA = (iTmp3 << 2) - iTmp2;
      
      iSum  = (   iTmp1          << 2 )
            + (   iTmpA          << 3 )
            + (   iTmpA          << 1 )
            -    iTmp0 -  iTmp2;
      
      piDst[x * iDstStep]    = iSum;
      piDstPel[x * iDstStep] = xClipY( (iSum +  32) >>  6 );
      piSrcTmp += iSrcStep;
    }
    piSrc += iSrcStride;
    piDst += iDstStride;
    piDstPel += iDstStridePel;
  }
 return;
}

template<UInt bitDepth>
__inline Void TRenInterpFilter<bitDepth>::xCTI_FilterHalfVer (Pel* piSrc, Int iSrcStride, Int iSrcStep, Int iWidth, Int iHeight, Int iDstStride, Int iDstStep, Int*& rpiDst)
{
  Int*  piDst = rpiDst;
  Int   iSum;
  Pel*  piSrcTmp;
  Int iSrcStride2 = iSrcStride*2;
  Int iSrcStride3 = iSrcStride*3;
  Int iSrcStride4 = iSrcStride*4;
  Int iSrcStride5 = iSrcStride*5;
  Int iSrcStride6 = iSrcStride*6;
  Int iSrcStride7 = iSrcStride*7;

  Int  iTmp0, iTmp1, iTmp2, iTmp3, iTmpA;
  for ( Int y = iHeight; y != 0; y-- )
  {
    piSrcTmp = &piSrc[ -3*iSrcStride ];
    for ( Int x = 0; x < iWidth; x++ )
    {
      // { -1,4,-11,40,40,-11,4,-1   } 
      iTmp0 = piSrcTmp[          0]+piSrcTmp[iSrcStride7];
      iTmp1 = piSrcTmp[iSrcStride ]+piSrcTmp[iSrcStride6];
      iTmp2 = piSrcTmp[iSrcStride2]+piSrcTmp[iSrcStride5];
      iTmp3 = piSrcTmp[iSrcStride3]+piSrcTmp[iSrcStride4];
      
      iTmpA = (iTmp3 << 2) - iTmp2;
      
      iSum  = (   iTmp1          << 2 )
            + (   iTmpA          << 3 )
            + (   iTmpA          << 1 )
            -    iTmp0 -  iTmp2;        
      
      piDst[x * iDstStep] = iSum;
      piSrcTmp += iSrcStep;
    }
    piSrc += iSrcStride;
    piDst += iDstStride;
  }
  return;
}

template<UInt bitDepth>
__inline Void TRenInterpFilter<bitDepth>::xCTI_FilterHalfVer (Pel* piSrc, Int iSrcStride, Int iSrcStep, Int iWidth, Int iHeight, Int iDstStride, Int iDstStep, Pel*& rpiDst)
{
  Pel*  piDst = rpiDst;
  Int   iSum;
  Pel*  piSrcTmp;
  
  Int iSrcStride2 = iSrcStride*2;
  Int iSrcStride3 = iSrcStride*3;
  Int iSrcStride4 = iSrcStride*4;
  Int iSrcStride5 = iSrcStride*5;
  Int iSrcStride6 = iSrcStride*6;
  Int iSrcStride7 = iSrcStride*7;

  Int  iTmp0, iTmp1, iTmp2, iTmp3, iTmpA;
  for ( Int y = iHeight; y != 0; y-- )
  {
    piSrcTmp = &piSrc[ -3*iSrcStride ];
    for ( Int x = 0; x < iWidth; x++ )
    {
      // { -1,4,-11,40,40,-11,4,-1   } 
      iTmp0 = piSrcTmp[          0]+piSrcTmp[iSrcStride7];
      iTmp1 = piSrcTmp[iSrcStride ]+piSrcTmp[iSrcStride6];
      iTmp2 = piSrcTmp[iSrcStride2]+piSrcTmp[iSrcStride5];
      iTmp3 = piSrcTmp[iSrcStride3]+piSrcTmp[iSrcStride4];
      
      iTmpA = (iTmp3 << 2) - iTmp2;
      
      iSum  = (   iTmp1          << 2 )
            + (   iTmpA          << 3 )
            + (   iTmpA          << 1 )
            -    iTmp0 -  iTmp2;        
      
      piDst[x * iDstStep] = xClipY( (iSum +  32) >>  6 );
      piSrcTmp += iSrcStep;
    }
    piSrc += iSrcStride;
    piDst += iDstStride;
  }
  return;
}

template<UInt bitDepth>
__inline Void TRenInterpFilter<bitDepth>::xCTI_FilterQuarter0Ver (Pel* piSrc, Int iSrcStride, Int iSrcStep, Int iWidth, Int iHeight, Int iDstStride, Int iDstStep, Int*& rpiDst)
{
  Int*  piDst = rpiDst;
  Int   iSum;
  Pel*  piSrcTmp;
  Int iSrcStride2 = iSrcStride*2;
  Int iSrcStride3 = iSrcStride*3;
  Int iSrcStride4 = iSrcStride*4;
  Int iSrcStride5 = iSrcStride*5;
  Int iSrcStride6 = iSrcStride*6;
  Int iSrcStride7 = iSrcStride*7;

  Int  iTmp1, iTmp2;
  for ( Int y = iHeight; y != 0; y-- )
  {
    piSrcTmp = &piSrc[ -3*iSrcStride ];
    for ( Int x = 0; x < iWidth; x++ )
    {
      // {-1,   4,  -10,  57,   19,  -7,   3,   -1  },
      
      iTmp1 = piSrcTmp[iSrcStride3] + piSrcTmp[iSrcStride5];
      iTmp2 = piSrcTmp[iSrcStride6] + piSrcTmp[iSrcStride4];
      
      iSum  =  iTmp1 + iTmp2 - piSrcTmp[0] - piSrcTmp[iSrcStride7]
            - ( ( piSrcTmp[iSrcStride2] - iTmp2 ) << 1 )
            + (  piSrcTmp[iSrcStride]             << 2 )
            - ( ( piSrcTmp[iSrcStride2] + iTmp1 ) << 3 )
            + (   piSrcTmp[iSrcStride4]           << 4 )
            + (   piSrcTmp[iSrcStride3]           << 6 );
      
      piDst[x * iDstStep] = iSum;
      piSrcTmp += iSrcStep;
    }
    piSrc += iSrcStride;
    piDst += iDstStride;
  }
  return;
}

template<UInt bitDepth>
__inline Void TRenInterpFilter<bitDepth>::xCTI_FilterQuarter0Ver (Pel* piSrc, Int iSrcStride, Int iSrcStep, Int iWidth, Int iHeight, Int iDstStride, Int iDstStep, Pel*& rpiDst)
{
  Pel*  piDst = rpiDst;
  Int   iSum;
  Pel*  piSrcTmp;
  
  Int iSrcStride2 = iSrcStride*2;
  Int iSrcStride3 = iSrcStride*3;
  Int iSrcStride4 = iSrcStride*4;
  Int iSrcStride5 = iSrcStride*5;
  Int iSrcStride6 = iSrcStride*6;
  Int iSrcStride7 = iSrcStride*7;

  Int  iTmp1, iTmp2;
  for ( Int y = iHeight; y != 0; y-- )
  {
    piSrcTmp = &piSrc[ -3*iSrcStride ];
    for ( Int x = 0; x < iWidth; x++ )
    {
      // {-1,   4,  -10,  57,   19,  -7,   3,   -1  },
      
      iTmp1 = piSrcTmp[iSrcStride3] + piSrcTmp[iSrcStride5];
      iTmp2 = piSrcTmp[iSrcStride6] + piSrcTmp[iSrcStride4];
      
      iSum  =  iTmp1 + iTmp2 - piSrcTmp[0] - piSrcTmp[iSrcStride7]
            - ( ( piSrcTmp[iSrcStride2] - iTmp2 ) << 1 )
            + (  piSrcTmp[iSrcStride]             << 2 )
            - ( ( piSrcTmp[iSrcStride2] + iTmp1 ) << 3 )
            + (   piSrcTmp[iSrcStride4]           << 4 );
      
      piDst[x * iDstStep] = xClipY( ((iSum +  32) >>  6) + piSrcTmp[iSrcStride3] );
      piSrcTmp += iSrcStep;
    }
    piSrc += iSrcStride;
    piDst += iDstStride;
  }
  return;
}

template<UInt bitDepth>
__inline Void TRenInterpFilter<bitDepth>::xCTI_FilterQuarter1Ver (Pel* piSrc, Int iSrcStride, Int iSrcStep, Int iWidth, Int iHeight, Int iDstStride, Int iDstStep, Int*& rpiDst)
{
  Int*  piDst = rpiDst;
  Int   iSum;
  Pel*  piSrcTmp;
  Int iSrcStride2 = iSrcStride*2;
  Int iSrcStride3 = iSrcStride*3;
  Int iSrcStride4 = iSrcStride*4;
  Int iSrcStride5 = iSrcStride*5;
  Int iSrcStride6 = iSrcStride*6;
  Int iSrcStride7 = iSrcStride*7;

  Int  iTmp1, iTmp2;

  for ( Int y = iHeight; y != 0; y-- )
  {
    piSrcTmp = &piSrc[ -3*iSrcStride ];
    for ( Int x = 0; x < iWidth; x++ )
    {
      /// {-1,   3,  -7,  19,   57,  -10,   4,   -1  },
      iTmp1 = piSrcTmp[iSrcStride4] + piSrcTmp[iSrcStride2];
      iTmp2 = piSrcTmp[iSrcStride ] + piSrcTmp[iSrcStride3];
      
      iSum  =  iTmp1 + iTmp2 - piSrcTmp[0] - piSrcTmp[iSrcStride7]
            - ( ( piSrcTmp[iSrcStride5] - iTmp2 ) << 1 )
            + (   piSrcTmp[iSrcStride6]           << 2 )
            - ( ( piSrcTmp[iSrcStride5] + iTmp1 ) << 3 )
            + (   piSrcTmp[iSrcStride3]           << 4 )
            + (   piSrcTmp[iSrcStride4]           << 6 );
            
      piDst[x * iDstStep] = iSum;
      piSrcTmp += iSrcStep;
    }
    piSrc += iSrcStride;
    piDst += iDstStride;
  }
  return;
}

template<UInt bitDepth>
__inline Void TRenInterpFilter<bitDepth>::xCTI_FilterQuarter1Ver (Pel* piSrc, Int iSrcStride, Int iSrcStep, Int iWidth, Int iHeight, Int iDstStride, Int iDstStep, Pel*& rpiDst)
{
  Pel*  piDst = rpiDst;
  Int   iSum;
  Pel*  piSrcTmp;
  Int iSrcStride2 = iSrcStride*2;
  Int iSrcStride3 = iSrcStride*3;
  Int iSrcStride4 = iSrcStride*4;
  Int iSrcStride5 = iSrcStride*5;
  Int iSrcStride6 = iSrcStride*6;
  Int iSrcStride7 = iSrcStride*7;

  Int  iTmp1, iTmp2;

  for ( Int y = iHeight; y != 0; y-- )
  {
    piSrcTmp = &piSrc[ -3*iSrcStride ];
    for ( Int x = 0; x < iWidth; x++ )
    {
      /// {-1,   3,  -7,  19,   57,  -10,   4,   -1  },
      iTmp1 = piSrcTmp[iSrcStride4] + piSrcTmp[iSrcStride2];
      iTmp2 = piSrcTmp[iSrcStride ] + piSrcTmp[iSrcStride3];
      
      iSum  =  iTmp1 + iTmp2 - piSrcTmp[0] - piSrcTmp[iSrcStride7]
            - ( ( piSrcTmp[iSrcStride5] - iTmp2 ) << 1 )
            + (   piSrcTmp[iSrcStride6]           << 2 )
            - ( ( piSrcTmp[iSrcStride5] + iTmp1 ) << 3 )
            + (   piSrcTmp[iSrcStride3]           << 4 );
            
      piDst[x * iDstStep] = xClipY( ((iSum +  32) >>  6) +  piSrcTmp[iSrcStride4] );
      piSrcTmp += iSrcStep;
    }
    piSrc += iSrcStride;
    piDst += iDstStride;
  }
  return;
}

// ------------------------------------------------------------------------------------------------
// DCTIF filters for Chroma
// ------------------------------------------------------------------------------------------------
template<UInt bitDepth>
__inline Void TRenInterpFilter<bitDepth>::xCTI_Filter2DVerC (Pel* piSrc, Int iSrcStride,  Int iWidth, Int iHeight, Int iDstStride,  Int*& rpiDst, Int iMV)
{
  Int*  piDst = rpiDst;
  Int   iSum;
  Pel*  piSrcTmp;

  switch (iMV)
  {
  case 1:
  {  
      for ( Int y = iHeight; y != 0; y-- )
      {
        piSrcTmp = &piSrc[ -iSrcStride ];
        for ( Int x = 0; x < iWidth; x++ )
        {
          iSum      = xCTI_Filter_VP04_C_OCT0( piSrcTmp, iSrcStride );
          piDst[x ] = iSum;
          piSrcTmp++;
        }
        piSrc += iSrcStride;
        piDst += iDstStride;
      }
  }
  break;
  case 2:
  {  
      for ( Int y = iHeight; y != 0; y-- )
      {
        piSrcTmp = &piSrc[ -iSrcStride ];
        for ( Int x = 0; x < iWidth; x++ )
        {
          iSum      = xCTI_Filter_VP04_C_QUA0( piSrcTmp, iSrcStride );
          piDst[x ] = iSum;
          piSrcTmp++;
        }
        piSrc += iSrcStride;
        piDst += iDstStride;
      }
  }
  break;
  case 6:  
  {  
      for ( Int y = iHeight; y != 0; y-- )
      {
        piSrcTmp = &piSrc[ -iSrcStride ];
        for ( Int x = 0; x < iWidth; x++ )
        {
          iSum      = xCTI_Filter_VP04_C_QUA1( piSrcTmp, iSrcStride );
          piDst[x ] = iSum;
          piSrcTmp++;
        }
        piSrc += iSrcStride;
        piDst += iDstStride;
      }
  }
  break;
  case 3:  
  {  
      for ( Int y = iHeight; y != 0; y-- )
      {
        piSrcTmp = &piSrc[ -iSrcStride ];
        for ( Int x = 0; x < iWidth; x++ )
        {
          iSum      = xCTI_Filter_VP04_C_OCT1( piSrcTmp, iSrcStride );
          piDst[x ] = iSum;
          piSrcTmp++;
        }
        piSrc += iSrcStride;
        piDst += iDstStride;
      }
  }
  break;
  case 5:
  {  
      for ( Int y = iHeight; y != 0; y-- )
      {
        piSrcTmp = &piSrc[ -iSrcStride ];
        for ( Int x = 0; x < iWidth; x++ )
        {
          iSum      = xCTI_Filter_VP04_C_OCT2( piSrcTmp, iSrcStride );
          piDst[x ] = iSum;
          piSrcTmp++;
        }
        piSrc += iSrcStride;
        piDst += iDstStride;
      }
  }
  break;
  case 7:
  {  
      for ( Int y = iHeight; y != 0; y-- )
      {
        piSrcTmp = &piSrc[ -iSrcStride ];
        for ( Int x = 0; x < iWidth; x++ )
        {
          iSum      = xCTI_Filter_VP04_C_OCT3( piSrcTmp, iSrcStride );
          piDst[x ] = iSum;
          piSrcTmp++;
        }
        piSrc += iSrcStride;
        piDst += iDstStride;
      }
  }
  break;
  case 4: 
  {

      for ( Int y = iHeight; y != 0; y-- )
      {
        piSrcTmp = &piSrc[ -iSrcStride ];
        for ( Int x = 0; x < iWidth; x++ )
        {
          iSum      = xCTI_Filter_VPS04_C_HAL( piSrcTmp, iSrcStride );
          piDst[x ] = iSum;
          piSrcTmp++;
        }
        piSrc += iSrcStride;
        piDst += iDstStride;
      }
  }
  break;
  default:
    assert( 0 );
  }
  return;
}

template<UInt bitDepth>
__inline Void TRenInterpFilter<bitDepth>::xCTI_Filter2DHorC(Int* piSrc, Int iSrcStride,  Int iWidth, Int iHeight, Int iDstStride,  Pel*& rpiDst, Int iMV)
{
  Pel*  piDst    = rpiDst;
  Int   iSum;
  Int*  piSrcTmp;

  switch (iMV)
  {
  case 1:
  {  
      for ( Int y = iHeight; y != 0; y-- )
      {
        piSrcTmp = &piSrc[ -1 ];
        for ( Int x = 0; x < iWidth; x++ )
        {
          iSum         = xCTI_Filter_VI04_C_OCT0( piSrcTmp, 1 );
          piDst   [x ] = xClipC ((iSum +  2048) >>  12 );
          piSrcTmp++;
        }
        piSrc += iSrcStride;
        piDst += iDstStride;
      }
  }
  break;
  case 2:
  {  
      for ( Int y = iHeight; y != 0; y-- )
      {
        piSrcTmp = &piSrc[ -1 ];
        for ( Int x = 0; x < iWidth; x++ )
        {
          iSum         = xCTI_Filter_VI04_C_QUA0( piSrcTmp, 1 );
          piDst   [x ] = xClipC ((iSum +  2048) >>  12 );
          piSrcTmp++;
        }
        piSrc += iSrcStride;
        piDst += iDstStride;
      }
  }
  break;
  case 6:
  {  
      for ( Int y = iHeight; y != 0; y-- )
      {
        piSrcTmp = &piSrc[ -1 ];
        for ( Int x = 0; x < iWidth; x++ )
        {
          iSum         = xCTI_Filter_VI04_C_QUA1( piSrcTmp, 1 );
          piDst   [x ] = xClipC ((iSum +  2048) >>  12 );
          piSrcTmp++;
        }
        piSrc += iSrcStride;
        piDst += iDstStride;
      }
  }
  break;
  case 3:
  {  
      for ( Int y = iHeight; y != 0; y-- )
      {
        piSrcTmp = &piSrc[ -1 ];
        for ( Int x = 0; x < iWidth; x++ )
        {
          iSum         = xCTI_Filter_VI04_C_OCT1( piSrcTmp, 1 );
          piDst   [x ] = xClipC ((iSum +  2048) >>  12 );
          piSrcTmp++;
        }
        piSrc += iSrcStride;
        piDst += iDstStride;
      }
  }
  break;
  case 5:
  {  
      for ( Int y = iHeight; y != 0; y-- )
      {
        piSrcTmp = &piSrc[ -1 ];
        for ( Int x = 0; x < iWidth; x++ )
        {
          iSum         = xCTI_Filter_VI04_C_OCT2( piSrcTmp, 1 );
          piDst   [x ] = xClipC ((iSum +  2048) >>  12 );
          piSrcTmp++;
        }
        piSrc += iSrcStride;
        piDst += iDstStride;
      }
  }
  break;
  case 7:
  {  
      for ( Int y = iHeight; y != 0; y-- )
      {
        piSrcTmp = &piSrc[ -1 ];
        for ( Int x = 0; x < iWidth; x++ )
        {
          iSum         = xCTI_Filter_VI04_C_OCT3( piSrcTmp, 1 );
          piDst   [x ] = xClipC ((iSum +  2048) >>  12 );
          piSrcTmp++;
        }
        piSrc += iSrcStride;
        piDst += iDstStride;
      }
  }
  break;
  case 4:
  {
      for ( Int y = iHeight; y != 0; y-- )
      {
        piSrcTmp = &piSrc[ -1 ];
        for ( Int x = 0; x < iWidth; x++ )
        {
          iSum      = xCTI_Filter_VIS04_C_HAL( piSrcTmp, 1 );
          piDst   [x ] = xClipC ((iSum +  2048) >>  12 );
          piSrcTmp++;
        }
        piSrc += iSrcStride;
        piDst += iDstStride;
      }
  }
  break;
  default:
    assert( 0 );
  }

  return;
}

template<UInt bitDepth>
__inline Void TRenInterpFilter<bitDepth>::xCTI_Filter1DVerC (Pel* piSrc, Int iSrcStride, Int iWidth, Int iHeight, Int iDstStride,  Pel*& rpiDst, Int iMV)
{
  Pel*  piDst = rpiDst;
  Int   iSum;
  Pel*  piSrcTmp;

  switch (iMV)
  {
  case 1:
  {  
      for ( Int y = iHeight; y != 0; y-- )
      {
        piSrcTmp = &piSrc[ -iSrcStride ];
        for ( Int x = 0; x < iWidth; x++ )
        {
          iSum      = xCTI_Filter_VP04_C_OCT0( piSrcTmp,  iSrcStride );
          piDst[x ] = xClipC ((iSum +  32) >>  6 );
          piSrcTmp++;
        }
        piSrc += iSrcStride;
        piDst += iDstStride;
      }
  }
  break;
  case 2:
  {  
      for ( Int y = iHeight; y != 0; y-- )
      {
        piSrcTmp = &piSrc[ -iSrcStride ];
        for ( Int x = 0; x < iWidth; x++ )
        {
          iSum      = xCTI_Filter_VP04_C_QUA0( piSrcTmp,  iSrcStride );
          piDst[x ] = xClipC ((iSum +  32) >>  6 );
          piSrcTmp++;
        }
        piSrc += iSrcStride;
        piDst += iDstStride;
      }
  }
  break;
  case 6:
  {  
      for ( Int y = iHeight; y != 0; y-- )
      {
        piSrcTmp = &piSrc[ -iSrcStride ];
        for ( Int x = 0; x < iWidth; x++ )
        {
          iSum      = xCTI_Filter_VP04_C_QUA1( piSrcTmp,  iSrcStride );
          piDst[x ] = xClipC ((iSum +  32) >>  6 );
          piSrcTmp++;
        }
        piSrc += iSrcStride;
        piDst += iDstStride;
      }
  }
  break;
  case 3:
  {  
      for ( Int y = iHeight; y != 0; y-- )
      {
        piSrcTmp = &piSrc[ -iSrcStride ];
        for ( Int x = 0; x < iWidth; x++ )
        {
          iSum      = xCTI_Filter_VP04_C_OCT1( piSrcTmp,  iSrcStride );
          piDst[x ] = xClipC ((iSum +  32) >>  6 );
          piSrcTmp++;
        }
        piSrc += iSrcStride;
        piDst += iDstStride;
      }
  }
  break;
  case 5:
  {  
      for ( Int y = iHeight; y != 0; y-- )
      {
        piSrcTmp = &piSrc[ -iSrcStride ];
        for ( Int x = 0; x < iWidth; x++ )
        {
          iSum      = xCTI_Filter_VP04_C_OCT2( piSrcTmp,  iSrcStride );
          piDst[x ] = xClipC ((iSum +  32) >>  6 );
          piSrcTmp++;
        }
        piSrc += iSrcStride;
        piDst += iDstStride;
      }
  }
  break;
  case 7:
  {  
      for ( Int y = iHeight; y != 0; y-- )
      {
        piSrcTmp = &piSrc[ -iSrcStride ];
        for ( Int x = 0; x < iWidth; x++ )
        {
          iSum      = xCTI_Filter_VP04_C_OCT3( piSrcTmp,  iSrcStride );
          piDst[x ] = xClipC ((iSum +  32) >>  6 );
          piSrcTmp++;
        }
        piSrc += iSrcStride;
        piDst += iDstStride;
      }
  }
  break;
  case 4:
  {
      for ( Int y = iHeight; y != 0; y-- )
      {
        piSrcTmp = &piSrc[-iSrcStride ];
        for ( Int x = 0; x < iWidth; x++ )
        {
          iSum      = xCTI_Filter_VPS04_C_HAL( piSrcTmp, iSrcStride );
          piDst[x ] = xClipC ((iSum +  32) >>  6 );
          piSrcTmp++;
        }
        piSrc += iSrcStride;
        piDst += iDstStride;
      }
  }
  break;
  default:
    assert( 0 );
  }
  return;
}

template<UInt bitDepth>
__inline Void TRenInterpFilter<bitDepth>::xCTI_Filter1DHorC(Pel* piSrc, Int iSrcStride, Int iWidth, Int iHeight, Int iDstStride, Pel*& rpiDst, Int iMV)
{
  Pel*  piDst    = rpiDst;
  Int   iSum;
  Pel*  piSrcTmp;

  switch (iMV)
  {
  case 1:
  {  
      for ( Int y = iHeight; y != 0; y-- )
      {
        piSrcTmp = &piSrc[ -1 ];
        for ( Int x = 0; x < iWidth; x++ )
        {
          iSum         = xCTI_Filter_VP04_C_OCT0( piSrcTmp,  1 );
          piDst[x ] = xClipC ((iSum +  32) >>  6 );
          piSrcTmp++;
        }
        piSrc += iSrcStride;
        piDst += iDstStride;
      }
  }
  break;
  case 2:
  {  
      for ( Int y = iHeight; y != 0; y-- )
      {
        piSrcTmp = &piSrc[ -1 ];
        for ( Int x = 0; x < iWidth; x++ )
        {
          iSum         = xCTI_Filter_VP04_C_QUA0( piSrcTmp,  1 );
          piDst[x ] = xClipC ((iSum +  32) >>  6 );
          piSrcTmp++;
        }
        piSrc += iSrcStride;
        piDst += iDstStride;
      }
  }
  break;
  case 6:
  {  
      for ( Int y = iHeight; y != 0; y-- )
      {
        piSrcTmp = &piSrc[ -1 ];
        for ( Int x = 0; x < iWidth; x++ )
        {
          iSum         = xCTI_Filter_VP04_C_QUA1( piSrcTmp,  1 );
          piDst[x ] = xClipC ((iSum +  32) >>  6 );
          piSrcTmp++;
        }
        piSrc += iSrcStride;
        piDst += iDstStride;
      }
  }
  break;
  case 3:
  {  
      for ( Int y = iHeight; y != 0; y-- )
      {
        piSrcTmp = &piSrc[ -1 ];
        for ( Int x = 0; x < iWidth; x++ )
        {
          iSum         = xCTI_Filter_VP04_C_OCT1( piSrcTmp,  1 );
          piDst[x ] = xClipC ((iSum +  32) >>  6 );
          piSrcTmp++;
        }
        piSrc += iSrcStride;
        piDst += iDstStride;
      }
  }
  break;
  case 5:
  {  
      for ( Int y = iHeight; y != 0; y-- )
      {
        piSrcTmp = &piSrc[ -1 ];
        for ( Int x = 0; x < iWidth; x++ )
        {
          iSum         = xCTI_Filter_VP04_C_OCT2( piSrcTmp,  1 );
          piDst[x ] = xClipC ((iSum +  32) >>  6 );
          piSrcTmp++;
        }
        piSrc += iSrcStride;
        piDst += iDstStride;
      }
  }
  break;
  case 7:
  {  
      for ( Int y = iHeight; y != 0; y-- )
      {
        piSrcTmp = &piSrc[ -1 ];
        for ( Int x = 0; x < iWidth; x++ )
        {
          iSum         = xCTI_Filter_VP04_C_OCT3( piSrcTmp,  1 );
          piDst[x ] = xClipC ((iSum +  32) >>  6 );
          piSrcTmp++;
        }
        piSrc += iSrcStride;
        piDst += iDstStride;
      }
  }
  break;
  case 4:
  {
      for ( Int y = iHeight; y != 0; y-- )
      {
        piSrcTmp = &piSrc[ -1 ];
        for ( Int x = 0; x < iWidth; x++ )
        {
          iSum         = xCTI_Filter_VPS04_C_HAL( piSrcTmp,  1 );
          piDst[x ] = xClipC ((iSum +  32) >>  6 );
          piSrcTmp++;
        }
        piSrc += iSrcStride;
        piDst += iDstStride;
      }
  }
  break;
  default:
    assert( 0 );
 }
  return;
}

template<UInt bitDepth>
__inline Int TRenInterpFilter<bitDepth>::xCTI_Filter_VP04_C_OCT0( Pel* pSrc,  Int iStride )
{// {  -3,  60,   8,   -1,} // 1/8
  Int iSum, iIdx = 0;

  Int p0 = pSrc[0];     iIdx+= iStride;
  Int p1 = pSrc[iIdx];  iIdx+= iStride;
  Int p2 = pSrc[iIdx];  iIdx+= iStride;
  Int p3 = pSrc[iIdx];  
  iSum = (p1<<6) -((p1+p0)<<2) +p0 +(p2<<3) -p3;

  return iSum;
}
template<UInt bitDepth>
__inline Int TRenInterpFilter<bitDepth>::xCTI_Filter_VI04_C_OCT0( Int* pSrc, Int iStride )
{ // {  -3,  60,   8,   -1,} //1/8
  Int iSum, iIdx = 0;

  Int p0 = pSrc[0];     iIdx+= iStride;
  Int p1 = pSrc[iIdx];  iIdx+= iStride;
  Int p2 = pSrc[iIdx];  iIdx+= iStride;
  Int p3 = pSrc[iIdx];  
  iSum = (p1<<6) -((p1+p0)<<2) +p0 +(p2<<3) -p3;

  return iSum;
}
template<UInt bitDepth>
__inline Int TRenInterpFilter<bitDepth>::xCTI_Filter_VP04_C_QUA0( Pel* pSrc,  Int iStride )
{// {  -4,  54,  16,   -2,} // 1/4
  Int iSum, iIdx = 0;

  Int p0 = pSrc[0];     iIdx+= iStride;
  Int p1 = pSrc[iIdx];  iIdx+= iStride;
  Int p2 = pSrc[iIdx];  iIdx+= iStride;
  Int p3 = pSrc[iIdx];  
  iSum = (p1 << 6) + (p2 << 4) - (p1 << 3) - ( p0 << 2) - ((p1 + p3) << 1);

  return iSum;
}

template<UInt bitDepth>
__inline Int TRenInterpFilter<bitDepth>::xCTI_Filter_VI04_C_QUA0( Int* pSrc, Int iStride )
{ // {  -4,  54,  16,   -2,} //1/4
  Int iSum, iIdx = 0;

  Int p0 = pSrc[0];     iIdx+= iStride;
  Int p1 = pSrc[iIdx];  iIdx+= iStride;
  Int p2 = pSrc[iIdx];  iIdx+= iStride;
  Int p3 = pSrc[iIdx];  
  iSum = (p1 << 6) + (p2 << 4) - (p1 << 3) - ( p0 << 2) - ((p1 + p3) << 1);

  return iSum;
}
template<UInt bitDepth>
__inline Int TRenInterpFilter<bitDepth>::xCTI_Filter_VP04_C_QUA1( Pel* pSrc,  Int iStride )
{// {  -2,  16,  54,   -4,}// 3/4
  Int iSum, iIdx = 0;

  Int p0 = pSrc[0];     iIdx+= iStride;
  Int p1 = pSrc[iIdx];  iIdx+= iStride;
  Int p2 = pSrc[iIdx];  iIdx+= iStride;
  Int p3 = pSrc[iIdx];  
  iSum = (p2 << 6) + (p1 << 4) - (p2 << 3) - ( p3 << 2) - ((p2 + p0) << 1);

  return iSum;
}

template<UInt bitDepth>
__inline Int TRenInterpFilter<bitDepth>::xCTI_Filter_VI04_C_QUA1( Int* pSrc, Int iStride )
{// {  -2,  16,  54,   -4,}// 3/4
  Int iSum, iIdx = 0;

  Int p0 = pSrc[0];     iIdx+= iStride;
  Int p1 = pSrc[iIdx];  iIdx+= iStride;
  Int p2 = pSrc[iIdx];  iIdx+= iStride;
  Int p3 = pSrc[iIdx];  
  iSum = (p2 << 6) + (p1 << 4) - (p2 << 3) - ( p3 << 2) - ((p2 + p0) << 1);

  return iSum;
}

template<UInt bitDepth>
__inline Int TRenInterpFilter<bitDepth>::xCTI_Filter_VP04_C_OCT1( Pel* pSrc,  Int iStride )
{// {  -5,  46,  27,   -4,} // 3/8
  Int iSum, iIdx = 0;

  Int p0 = pSrc[0];     iIdx+= iStride;
  Int p1 = pSrc[iIdx];  iIdx+= iStride;
  Int p2 = pSrc[iIdx];  iIdx+= iStride;
  Int p3 = pSrc[iIdx];  
  Int t = p0 + p2;
  iSum = ((p1 + p2) << 5) + (p1 << 4) - ( (t + p3) << 2) - ( p1 << 1) - t;

  return iSum;
}

template<UInt bitDepth>
__inline Int TRenInterpFilter<bitDepth>::xCTI_Filter_VI04_C_OCT1( Int* pSrc, Int iStride )
{ // {  -5,  46,  27,   -4,} //3/8
  Int iSum, iIdx = 0;

  Int p0 = pSrc[0];     iIdx+= iStride;
  Int p1 = pSrc[iIdx];  iIdx+= iStride;
  Int p2 = pSrc[iIdx];  iIdx+= iStride;
  Int p3 = pSrc[iIdx];  
  Int t = p0 + p2;
  iSum = ((p1 + p2) << 5) + (p1 << 4) - ( (t + p3) << 2) - ( p1 << 1) - t;

  return iSum;
}

template<UInt bitDepth>
__inline Int TRenInterpFilter<bitDepth>::xCTI_Filter_VPS04_C_HAL( Pel* pSrc, Int iStride )
{
  // {  -4,  36,  36,   -4,}, // 1/2
  Int iSum;
  Int iTemp0 = pSrc[iStride*1]+pSrc[iStride*2];
  Int iTemp1 = pSrc[        0]+pSrc[iStride*3];

  iSum  = ((iTemp0<<3) + iTemp0 -iTemp1)<<2;

  return iSum;
}

template<UInt bitDepth>
__inline Int TRenInterpFilter<bitDepth>::xCTI_Filter_VIS04_C_HAL( Int* pSrc, Int iStride )
{
  // {  -4,  36,  36,   -4,}, //1/2
  Int iSum;
  Int iTemp0 = pSrc[iStride*1]+pSrc[iStride*2];
  Int iTemp1 = pSrc[        0]+pSrc[iStride*3];

  iSum  = ((iTemp0<<3) + iTemp0 -iTemp1)<<2;

  return iSum;
}

template<UInt bitDepth>
__inline Int TRenInterpFilter<bitDepth>::xCTI_Filter_VP04_C_OCT2( Pel* pSrc,  Int iStride )
{// {  -4,  27,  46,   -5,}, // 5/8
  Int iSum, iIdx = 0;

  Int p0 = pSrc[0];     iIdx+= iStride;
  Int p1 = pSrc[iIdx];  iIdx+= iStride;
  Int p2 = pSrc[iIdx];  iIdx+= iStride;
  Int p3 = pSrc[iIdx];  
  Int t = p1 + p3;
  iSum = ((p1 + p2) << 5) + (p2 << 4) - ( (t + p0) << 2) - ( p2 << 1) - t;

  return iSum;
}

template<UInt bitDepth>
__inline Int TRenInterpFilter<bitDepth>::xCTI_Filter_VI04_C_OCT2( Int* pSrc, Int iStride )
{ // {  -4,  27,  46,   -5,}, // 5/8
  Int iSum, iIdx = 0;

  Int p0 = pSrc[0];     iIdx+= iStride;
  Int p1 = pSrc[iIdx];  iIdx+= iStride;
  Int p2 = pSrc[iIdx];  iIdx+= iStride;
  Int p3 = pSrc[iIdx];  
  Int t = p1 + p3;
  iSum = ((p1 + p2) << 5) + (p2 << 4) - ( (t + p0) << 2) - ( p2 << 1) - t;

  return iSum;
}

template<UInt bitDepth>
__inline Int TRenInterpFilter<bitDepth>::xCTI_Filter_VP04_C_OCT3( Pel* pSrc,  Int iStride )
{// {  -1,   8,  60,   -3,} // 7/8
  Int iSum, iIdx = 0;

  Int p0 = pSrc[0];     iIdx+= iStride;
  Int p1 = pSrc[iIdx];  iIdx+= iStride;
  Int p2 = pSrc[iIdx];  iIdx+= iStride;
  Int p3 = pSrc[iIdx];  
  iSum = (p2<<6) -((p2+p3)<<2) +p3 +(p1<<3) -p0;

  return iSum;
}

template<UInt bitDepth>
__inline Int TRenInterpFilter<bitDepth>::xCTI_Filter_VI04_C_OCT3( Int* pSrc, Int iStride )
{ // {  -1,   8,  60,   -3,} // 7/8
  Int iSum, iIdx = 0;

  Int p0 = pSrc[0];     iIdx+= iStride;
  Int p1 = pSrc[iIdx];  iIdx+= iStride;
  Int p2 = pSrc[iIdx];  iIdx+= iStride;
  Int p3 = pSrc[iIdx];  
  iSum = (p2<<6) -((p2+p3)<<2) +p3 +(p1<<3) -p0;

  return iSum;
}

#endif // NH_3D
#endif // __TRENINTERP__
