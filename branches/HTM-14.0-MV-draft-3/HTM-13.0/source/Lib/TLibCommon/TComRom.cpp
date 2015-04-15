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

/** \file     TComRom.cpp
    \brief    global variables & functions
*/

#include "TComRom.h"
#include <memory.h>
#include <stdlib.h>
#include <stdio.h>
// ====================================================================================================================
// Initialize / destroy functions
// ====================================================================================================================

//! \ingroup TLibCommon
//! \{

// initialize ROM variables
Void initROM()
{
  Int i, c;
  
  // g_aucConvertToBit[ x ]: log2(x/4), if x=4 -> 0, x=8 -> 1, x=16 -> 2, ...
  ::memset( g_aucConvertToBit,   -1, sizeof( g_aucConvertToBit ) );
  c=0;
  for ( i=4; i<=MAX_CU_SIZE; i*=2 )
  {
    g_aucConvertToBit[ i ] = c;
    c++;
  }
  
  c=2;
  for ( i=0; i<MAX_CU_DEPTH; i++ )
  {
    g_auiSigLastScan[0][i] = new UInt[ c*c ];
    g_auiSigLastScan[1][i] = new UInt[ c*c ];
    g_auiSigLastScan[2][i] = new UInt[ c*c ];
    initSigLastScan( g_auiSigLastScan[0][i], g_auiSigLastScan[1][i], g_auiSigLastScan[2][i], c, c);

    c <<= 1;
  }  

#if H_MV
#if H_MV_HLS_PTL_LIMITS 
 g_generalTierAndLevelLimits[ Level::LEVEL1   ] = TComGeneralTierAndLevelLimits(    36864,     350,  INT_MIN,   16,   1,   1 );
 g_generalTierAndLevelLimits[ Level::LEVEL2   ] = TComGeneralTierAndLevelLimits(   122880,    1500,  INT_MIN,   16,   1,   1 );
 g_generalTierAndLevelLimits[ Level::LEVEL2_1 ] = TComGeneralTierAndLevelLimits(   245760,    3000,  INT_MIN,   20,   1,   1 );
 g_generalTierAndLevelLimits[ Level::LEVEL3   ] = TComGeneralTierAndLevelLimits(   552960,    6000,  INT_MIN,   30,   2,   2 );
 g_generalTierAndLevelLimits[ Level::LEVEL3_1 ] = TComGeneralTierAndLevelLimits(   983040,   10000,  INT_MIN,   40,   3,   3 );
 g_generalTierAndLevelLimits[ Level::LEVEL4   ] = TComGeneralTierAndLevelLimits(  2228224,   12000,    30000,   75,   5,   5 );
 g_generalTierAndLevelLimits[ Level::LEVEL4_1 ] = TComGeneralTierAndLevelLimits(  2228224,   20000,    50000,   75,   5,   5 );
 g_generalTierAndLevelLimits[ Level::LEVEL5   ] = TComGeneralTierAndLevelLimits(  8912896,   25000,   100000,  200,  11,  10 );
 g_generalTierAndLevelLimits[ Level::LEVEL5_1 ] = TComGeneralTierAndLevelLimits(  8912896,   40000,   160000,  200,  11,  10 );
 g_generalTierAndLevelLimits[ Level::LEVEL5_2 ] = TComGeneralTierAndLevelLimits(  8912896,   60000,   240000,  200,  11,  10 );
 g_generalTierAndLevelLimits[ Level::LEVEL6   ] = TComGeneralTierAndLevelLimits( 35651584,   60000,   240000,  600,  22,  20 );
 g_generalTierAndLevelLimits[ Level::LEVEL6_1 ] = TComGeneralTierAndLevelLimits( 35651584,  120000,   480000,  600,  22,  20 );
 g_generalTierAndLevelLimits[ Level::LEVEL6_2 ] = TComGeneralTierAndLevelLimits( 35651584,  240000,   800000,  600,  22,  20 );
#endif
#endif

}

Void destroyROM()
{
  for (Int i=0; i<MAX_CU_DEPTH; i++ )
  {
    delete[] g_auiSigLastScan[0][i];
    delete[] g_auiSigLastScan[1][i];
    delete[] g_auiSigLastScan[2][i];
  }

#if H_3D_DIM_DMM
  if( !g_dmmWedgeLists.empty() ) 
  {
    for( UInt ui = 0; ui < g_dmmWedgeLists.size(); ui++ ) { g_dmmWedgeLists[ui].clear(); }
    g_dmmWedgeLists.clear();
  }
  if( !g_dmmWedgeRefLists.empty() )
  {
    for( UInt ui = 0; ui < g_dmmWedgeRefLists.size(); ui++ ) { g_dmmWedgeRefLists[ui].clear(); }
    g_dmmWedgeRefLists.clear();
  }

  if( !g_dmmWedgeNodeLists.empty() )
  {
    for( UInt ui = 0; ui < g_dmmWedgeNodeLists.size(); ui++ ) { g_dmmWedgeNodeLists[ui].clear(); }
    g_dmmWedgeNodeLists.clear();
  }
#endif
}

// ====================================================================================================================
// Data structure related table & variable
// ====================================================================================================================

UInt g_uiMaxCUWidth  = MAX_CU_SIZE;
UInt g_uiMaxCUHeight = MAX_CU_SIZE;
UInt g_uiMaxCUDepth  = MAX_CU_DEPTH;
UInt g_uiAddCUDepth  = 0;
UInt g_auiZscanToRaster [ MAX_NUM_SPU_W*MAX_NUM_SPU_W ] = { 0, };
UInt g_auiRasterToZscan [ MAX_NUM_SPU_W*MAX_NUM_SPU_W ] = { 0, };
UInt g_auiRasterToPelX  [ MAX_NUM_SPU_W*MAX_NUM_SPU_W ] = { 0, };
UInt g_auiRasterToPelY  [ MAX_NUM_SPU_W*MAX_NUM_SPU_W ] = { 0, };

UInt g_auiPUOffset[8] = { 0, 8, 4, 4, 2, 10, 1, 5};

Void initZscanToRaster ( Int iMaxDepth, Int iDepth, UInt uiStartVal, UInt*& rpuiCurrIdx )
{
  Int iStride = 1 << ( iMaxDepth - 1 );
  
  if ( iDepth == iMaxDepth )
  {
    rpuiCurrIdx[0] = uiStartVal;
    rpuiCurrIdx++;
  }
  else
  {
    Int iStep = iStride >> iDepth;
    initZscanToRaster( iMaxDepth, iDepth+1, uiStartVal,                     rpuiCurrIdx );
    initZscanToRaster( iMaxDepth, iDepth+1, uiStartVal+iStep,               rpuiCurrIdx );
    initZscanToRaster( iMaxDepth, iDepth+1, uiStartVal+iStep*iStride,       rpuiCurrIdx );
    initZscanToRaster( iMaxDepth, iDepth+1, uiStartVal+iStep*iStride+iStep, rpuiCurrIdx );
  }
}

Void initRasterToZscan ( UInt uiMaxCUWidth, UInt uiMaxCUHeight, UInt uiMaxDepth )
{
  UInt  uiMinCUWidth  = uiMaxCUWidth  >> ( uiMaxDepth - 1 );
  UInt  uiMinCUHeight = uiMaxCUHeight >> ( uiMaxDepth - 1 );
  
  UInt  uiNumPartInWidth  = (UInt)uiMaxCUWidth  / uiMinCUWidth;
  UInt  uiNumPartInHeight = (UInt)uiMaxCUHeight / uiMinCUHeight;
  
  for ( UInt i = 0; i < uiNumPartInWidth*uiNumPartInHeight; i++ )
  {
    g_auiRasterToZscan[ g_auiZscanToRaster[i] ] = i;
  }
}

Void initRasterToPelXY ( UInt uiMaxCUWidth, UInt uiMaxCUHeight, UInt uiMaxDepth )
{
  UInt    i;
  
  UInt* uiTempX = &g_auiRasterToPelX[0];
  UInt* uiTempY = &g_auiRasterToPelY[0];
  
  UInt  uiMinCUWidth  = uiMaxCUWidth  >> ( uiMaxDepth - 1 );
  UInt  uiMinCUHeight = uiMaxCUHeight >> ( uiMaxDepth - 1 );
  
  UInt  uiNumPartInWidth  = uiMaxCUWidth  / uiMinCUWidth;
  UInt  uiNumPartInHeight = uiMaxCUHeight / uiMinCUHeight;
  
  uiTempX[0] = 0; uiTempX++;
  for ( i = 1; i < uiNumPartInWidth; i++ )
  {
    uiTempX[0] = uiTempX[-1] + uiMinCUWidth; uiTempX++;
  }
  for ( i = 1; i < uiNumPartInHeight; i++ )
  {
    memcpy(uiTempX, uiTempX-uiNumPartInWidth, sizeof(UInt)*uiNumPartInWidth);
    uiTempX += uiNumPartInWidth;
  }
  
  for ( i = 1; i < uiNumPartInWidth*uiNumPartInHeight; i++ )
  {
    uiTempY[i] = ( i / uiNumPartInWidth ) * uiMinCUWidth;
  }
};


Int g_quantScales[6] =
{
  26214,23302,20560,18396,16384,14564
};    

Int g_invQuantScales[6] =
{
  40,45,51,57,64,72
};

const Short g_aiT4[4][4] =
{
  { 64, 64, 64, 64},
  { 83, 36,-36,-83},
  { 64,-64,-64, 64},
  { 36,-83, 83,-36}
};

const Short g_aiT8[8][8] =
{
  { 64, 64, 64, 64, 64, 64, 64, 64},
  { 89, 75, 50, 18,-18,-50,-75,-89},
  { 83, 36,-36,-83,-83,-36, 36, 83},
  { 75,-18,-89,-50, 50, 89, 18,-75},
  { 64,-64,-64, 64, 64,-64,-64, 64},
  { 50,-89, 18, 75,-75,-18, 89,-50},
  { 36,-83, 83,-36,-36, 83,-83, 36},
  { 18,-50, 75,-89, 89,-75, 50,-18}
};

const Short g_aiT16[16][16] =
{
  { 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64},
  { 90, 87, 80, 70, 57, 43, 25,  9, -9,-25,-43,-57,-70,-80,-87,-90},
  { 89, 75, 50, 18,-18,-50,-75,-89,-89,-75,-50,-18, 18, 50, 75, 89},
  { 87, 57,  9,-43,-80,-90,-70,-25, 25, 70, 90, 80, 43, -9,-57,-87},
  { 83, 36,-36,-83,-83,-36, 36, 83, 83, 36,-36,-83,-83,-36, 36, 83},
  { 80,  9,-70,-87,-25, 57, 90, 43,-43,-90,-57, 25, 87, 70, -9,-80},
  { 75,-18,-89,-50, 50, 89, 18,-75,-75, 18, 89, 50,-50,-89,-18, 75},
  { 70,-43,-87,  9, 90, 25,-80,-57, 57, 80,-25,-90, -9, 87, 43,-70},
  { 64,-64,-64, 64, 64,-64,-64, 64, 64,-64,-64, 64, 64,-64,-64, 64},
  { 57,-80,-25, 90, -9,-87, 43, 70,-70,-43, 87,  9,-90, 25, 80,-57},
  { 50,-89, 18, 75,-75,-18, 89,-50,-50, 89,-18,-75, 75, 18,-89, 50},
  { 43,-90, 57, 25,-87, 70,  9,-80, 80, -9,-70, 87,-25,-57, 90,-43},
  { 36,-83, 83,-36,-36, 83,-83, 36, 36,-83, 83,-36,-36, 83,-83, 36},
  { 25,-70, 90,-80, 43,  9,-57, 87,-87, 57, -9,-43, 80,-90, 70,-25},
  { 18,-50, 75,-89, 89,-75, 50,-18,-18, 50,-75, 89,-89, 75,-50, 18},
  {  9,-25, 43,-57, 70,-80, 87,-90, 90,-87, 80,-70, 57,-43, 25, -9}
};

const Short g_aiT32[32][32] =
{
  { 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64},
  { 90, 90, 88, 85, 82, 78, 73, 67, 61, 54, 46, 38, 31, 22, 13,  4, -4,-13,-22,-31,-38,-46,-54,-61,-67,-73,-78,-82,-85,-88,-90,-90},
  { 90, 87, 80, 70, 57, 43, 25,  9, -9,-25,-43,-57,-70,-80,-87,-90,-90,-87,-80,-70,-57,-43,-25, -9,  9, 25, 43, 57, 70, 80, 87, 90},
  { 90, 82, 67, 46, 22, -4,-31,-54,-73,-85,-90,-88,-78,-61,-38,-13, 13, 38, 61, 78, 88, 90, 85, 73, 54, 31,  4,-22,-46,-67,-82,-90},
  { 89, 75, 50, 18,-18,-50,-75,-89,-89,-75,-50,-18, 18, 50, 75, 89, 89, 75, 50, 18,-18,-50,-75,-89,-89,-75,-50,-18, 18, 50, 75, 89},
  { 88, 67, 31,-13,-54,-82,-90,-78,-46, -4, 38, 73, 90, 85, 61, 22,-22,-61,-85,-90,-73,-38,  4, 46, 78, 90, 82, 54, 13,-31,-67,-88},
  { 87, 57,  9,-43,-80,-90,-70,-25, 25, 70, 90, 80, 43, -9,-57,-87,-87,-57, -9, 43, 80, 90, 70, 25,-25,-70,-90,-80,-43,  9, 57, 87},
  { 85, 46,-13,-67,-90,-73,-22, 38, 82, 88, 54, -4,-61,-90,-78,-31, 31, 78, 90, 61,  4,-54,-88,-82,-38, 22, 73, 90, 67, 13,-46,-85},
  { 83, 36,-36,-83,-83,-36, 36, 83, 83, 36,-36,-83,-83,-36, 36, 83, 83, 36,-36,-83,-83,-36, 36, 83, 83, 36,-36,-83,-83,-36, 36, 83},
  { 82, 22,-54,-90,-61, 13, 78, 85, 31,-46,-90,-67,  4, 73, 88, 38,-38,-88,-73, -4, 67, 90, 46,-31,-85,-78,-13, 61, 90, 54,-22,-82},
  { 80,  9,-70,-87,-25, 57, 90, 43,-43,-90,-57, 25, 87, 70, -9,-80,-80, -9, 70, 87, 25,-57,-90,-43, 43, 90, 57,-25,-87,-70,  9, 80},
  { 78, -4,-82,-73, 13, 85, 67,-22,-88,-61, 31, 90, 54,-38,-90,-46, 46, 90, 38,-54,-90,-31, 61, 88, 22,-67,-85,-13, 73, 82,  4,-78},
  { 75,-18,-89,-50, 50, 89, 18,-75,-75, 18, 89, 50,-50,-89,-18, 75, 75,-18,-89,-50, 50, 89, 18,-75,-75, 18, 89, 50,-50,-89,-18, 75},
  { 73,-31,-90,-22, 78, 67,-38,-90,-13, 82, 61,-46,-88, -4, 85, 54,-54,-85,  4, 88, 46,-61,-82, 13, 90, 38,-67,-78, 22, 90, 31,-73},
  { 70,-43,-87,  9, 90, 25,-80,-57, 57, 80,-25,-90, -9, 87, 43,-70,-70, 43, 87, -9,-90,-25, 80, 57,-57,-80, 25, 90,  9,-87,-43, 70},
  { 67,-54,-78, 38, 85,-22,-90,  4, 90, 13,-88,-31, 82, 46,-73,-61, 61, 73,-46,-82, 31, 88,-13,-90, -4, 90, 22,-85,-38, 78, 54,-67},
  { 64,-64,-64, 64, 64,-64,-64, 64, 64,-64,-64, 64, 64,-64,-64, 64, 64,-64,-64, 64, 64,-64,-64, 64, 64,-64,-64, 64, 64,-64,-64, 64},
  { 61,-73,-46, 82, 31,-88,-13, 90, -4,-90, 22, 85,-38,-78, 54, 67,-67,-54, 78, 38,-85,-22, 90,  4,-90, 13, 88,-31,-82, 46, 73,-61},
  { 57,-80,-25, 90, -9,-87, 43, 70,-70,-43, 87,  9,-90, 25, 80,-57,-57, 80, 25,-90,  9, 87,-43,-70, 70, 43,-87, -9, 90,-25,-80, 57},
  { 54,-85, -4, 88,-46,-61, 82, 13,-90, 38, 67,-78,-22, 90,-31,-73, 73, 31,-90, 22, 78,-67,-38, 90,-13,-82, 61, 46,-88,  4, 85,-54},
  { 50,-89, 18, 75,-75,-18, 89,-50,-50, 89,-18,-75, 75, 18,-89, 50, 50,-89, 18, 75,-75,-18, 89,-50,-50, 89,-18,-75, 75, 18,-89, 50},
  { 46,-90, 38, 54,-90, 31, 61,-88, 22, 67,-85, 13, 73,-82,  4, 78,-78, -4, 82,-73,-13, 85,-67,-22, 88,-61,-31, 90,-54,-38, 90,-46},
  { 43,-90, 57, 25,-87, 70,  9,-80, 80, -9,-70, 87,-25,-57, 90,-43,-43, 90,-57,-25, 87,-70, -9, 80,-80,  9, 70,-87, 25, 57,-90, 43},
  { 38,-88, 73, -4,-67, 90,-46,-31, 85,-78, 13, 61,-90, 54, 22,-82, 82,-22,-54, 90,-61,-13, 78,-85, 31, 46,-90, 67,  4,-73, 88,-38},
  { 36,-83, 83,-36,-36, 83,-83, 36, 36,-83, 83,-36,-36, 83,-83, 36, 36,-83, 83,-36,-36, 83,-83, 36, 36,-83, 83,-36,-36, 83,-83, 36},
  { 31,-78, 90,-61,  4, 54,-88, 82,-38,-22, 73,-90, 67,-13,-46, 85,-85, 46, 13,-67, 90,-73, 22, 38,-82, 88,-54, -4, 61,-90, 78,-31},
  { 25,-70, 90,-80, 43,  9,-57, 87,-87, 57, -9,-43, 80,-90, 70,-25,-25, 70,-90, 80,-43, -9, 57,-87, 87,-57,  9, 43,-80, 90,-70, 25},
  { 22,-61, 85,-90, 73,-38, -4, 46,-78, 90,-82, 54,-13,-31, 67,-88, 88,-67, 31, 13,-54, 82,-90, 78,-46,  4, 38,-73, 90,-85, 61,-22},
  { 18,-50, 75,-89, 89,-75, 50,-18,-18, 50,-75, 89,-89, 75,-50, 18, 18,-50, 75,-89, 89,-75, 50,-18,-18, 50,-75, 89,-89, 75,-50, 18},
  { 13,-38, 61,-78, 88,-90, 85,-73, 54,-31,  4, 22,-46, 67,-82, 90,-90, 82,-67, 46,-22, -4, 31,-54, 73,-85, 90,-88, 78,-61, 38,-13},
  {  9,-25, 43,-57, 70,-80, 87,-90, 90,-87, 80,-70, 57,-43, 25, -9, -9, 25,-43, 57,-70, 80,-87, 90,-90, 87,-80, 70,-57, 43,-25,  9},
  {  4,-13, 22,-31, 38,-46, 54,-61, 67,-73, 78,-82, 85,-88, 90,-90, 90,-90, 88,-85, 82,-78, 73,-67, 61,-54, 46,-38, 31,-22, 13, -4}
};

const UChar g_aucChromaScale[58]=
{
   0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15,16,
  17,18,19,20,21,22,23,24,25,26,27,28,29,29,30,31,32,
  33,33,34,34,35,35,36,36,37,37,38,39,40,41,42,43,44,
  45,46,47,48,49,50,51
};


// Mode-Dependent DCT/DST 
const Short g_as_DST_MAT_4 [4][4]=
{
  {29,   55,    74,   84},
  {74,   74,    0 ,  -74},
  {84,  -29,   -74,   55},
  {55,  -84,    74,  -29},
};


// ====================================================================================================================
// ADI
// ====================================================================================================================

#if FAST_UDI_USE_MPM
const UChar g_aucIntraModeNumFast[MAX_CU_DEPTH] =
{
  3,  //   2x2
  8,  //   4x4
  8,  //   8x8
  3,  //  16x16   
  3,  //  32x32   
  3   //  64x64   
};
#else // FAST_UDI_USE_MPM
const UChar g_aucIntraModeNumFast[MAX_CU_DEPTH] =
{
  3,  //   2x2
  9,  //   4x4
  9,  //   8x8
  4,  //  16x16   33
  4,  //  32x32   33
  5   //  64x64   33
};
#endif // FAST_UDI_USE_MPM

// chroma

const UChar g_aucConvertTxtTypeToIdx[4] = { 0, 1, 1, 2 };


// ====================================================================================================================
// Bit-depth
// ====================================================================================================================

Int  g_bitDepthY = 8;
Int  g_bitDepthC = 8;

UInt g_uiPCMBitDepthLuma     = 8;    // PCM bit-depth
UInt g_uiPCMBitDepthChroma   = 8;    // PCM bit-depth

#if H_3D_DIM_DMM
// ====================================================================================================================
// Depth coding modes
// ====================================================================================================================
const WedgeResolution g_dmmWedgeResolution[6] = 
{
  HALF_PEL,    //   4x4
  HALF_PEL,    //   8x8
  FULL_PEL,    //  16x16
  FULL_PEL,    //  32x32
  FULL_PEL,    //  64x64
  FULL_PEL     // 128x128
};

const UChar g_dmm1TabIdxBits[6] =
{ //2x2   4x4   8x8 16x16 32x32 64x64
#if MTK_DMM_SIM_J0035 
     0,    7,   10,   9,    9,   13 };
#else
     0,    7,   10,   11,   11,   13 };
#endif

const UChar g_dmm3IntraTabIdxBits[6] =
{ //2x2   4x4   8x8 16x16 32x32 64x64
     0,    4,    7,    8,    8,    0 };

Bool g_wedgePattern[32*32];

extern std::vector< std::vector<TComWedgelet> >   g_dmmWedgeLists;
extern std::vector< std::vector<TComWedgeRef> >   g_dmmWedgeRefLists;
extern std::vector< std::vector<TComWedgeNode> >  g_dmmWedgeNodeLists;
#endif

// ====================================================================================================================
// Misc.
// ====================================================================================================================

Char  g_aucConvertToBit  [ MAX_CU_SIZE+1 ];
#if ENC_DEC_TRACE
FILE*  g_hTrace = NULL;
const Bool g_bEncDecTraceEnable  = true;
const Bool g_bEncDecTraceDisable = false;
Bool   g_HLSTraceEnable = true;
Bool   g_bJustDoIt = false;
UInt64 g_nSymbolCounter = 0;
#if H_MV_ENC_DEC_TRAC
Bool g_traceCU = false; 
Bool g_tracePU = false; 
Bool g_traceTU = false; 
Bool g_disableHLSTrace = false; 
UInt64 g_stopAtCounter       = 0; 
Bool g_traceCopyBack         = false; 
Bool g_decTraceDispDer       = false; 
Bool g_decTraceMvFromMerge   = false; 
Bool g_decTracePicOutput     = false; 
Bool g_stopAtPos             = false; 
Bool g_outputPos             = false; 
#endif
#endif
// ====================================================================================================================
// Scanning order & context model mapping
// ====================================================================================================================

// scanning order table
UInt* g_auiSigLastScan[ 3 ][ MAX_CU_DEPTH ];

const UInt g_sigLastScan8x8[ 3 ][ 4 ] =
{
  {0, 2, 1, 3},
  {0, 1, 2, 3},
  {0, 2, 1, 3}
};
UInt g_sigLastScanCG32x32[ 64 ];

const UInt g_uiMinInGroup[ 10 ] = {0,1,2,3,4,6,8,12,16,24};
const UInt g_uiGroupIdx[ 32 ]   = {0,1,2,3,4,4,5,5,6,6,6,6,7,7,7,7,8,8,8,8,8,8,8,8,9,9,9,9,9,9,9,9};

Void initSigLastScan(UInt* pBuffD, UInt* pBuffH, UInt* pBuffV, Int iWidth, Int iHeight)
{
  const UInt  uiNumScanPos  = UInt( iWidth * iWidth );
  UInt        uiNextScanPos = 0;

  if( iWidth < 16 )
  {
  UInt* pBuffTemp = pBuffD;
  if( iWidth == 8 )
  {
    pBuffTemp = g_sigLastScanCG32x32;
  }
  for( UInt uiScanLine = 0; uiNextScanPos < uiNumScanPos; uiScanLine++ )
  {
    Int    iPrimDim  = Int( uiScanLine );
    Int    iScndDim  = 0;
    while( iPrimDim >= iWidth )
    {
      iScndDim++;
      iPrimDim--;
    }
    while( iPrimDim >= 0 && iScndDim < iWidth )
    {
      pBuffTemp[ uiNextScanPos ] = iPrimDim * iWidth + iScndDim ;
      uiNextScanPos++;
      iScndDim++;
      iPrimDim--;
    }
  }
  }
  if( iWidth > 4 )
  {
    UInt uiNumBlkSide = iWidth >> 2;
    UInt uiNumBlks    = uiNumBlkSide * uiNumBlkSide;
    UInt log2Blk      = g_aucConvertToBit[ uiNumBlkSide ] + 1;

    for( UInt uiBlk = 0; uiBlk < uiNumBlks; uiBlk++ )
    {
      uiNextScanPos   = 0;
      UInt initBlkPos = g_auiSigLastScan[ SCAN_DIAG ][ log2Blk ][ uiBlk ];
      if( iWidth == 32 )
      {
        initBlkPos = g_sigLastScanCG32x32[ uiBlk ];
      }
      UInt offsetY    = initBlkPos / uiNumBlkSide;
      UInt offsetX    = initBlkPos - offsetY * uiNumBlkSide;
      UInt offsetD    = 4 * ( offsetX + offsetY * iWidth );
      UInt offsetScan = 16 * uiBlk;
      for( UInt uiScanLine = 0; uiNextScanPos < 16; uiScanLine++ )
      {
        Int    iPrimDim  = Int( uiScanLine );
        Int    iScndDim  = 0;
        while( iPrimDim >= 4 )
        {
          iScndDim++;
          iPrimDim--;
        }
        while( iPrimDim >= 0 && iScndDim < 4 )
        {
          pBuffD[ uiNextScanPos + offsetScan ] = iPrimDim * iWidth + iScndDim + offsetD;
          uiNextScanPos++;
          iScndDim++;
          iPrimDim--;
        }
      }
    }
  }
  
  UInt uiCnt = 0;
  if( iWidth > 2 )
  {
    UInt numBlkSide = iWidth >> 2;
    for(Int blkY=0; blkY < numBlkSide; blkY++)
    {
      for(Int blkX=0; blkX < numBlkSide; blkX++)
      {
        UInt offset    = blkY * 4 * iWidth + blkX * 4;
        for(Int y=0; y < 4; y++)
        {
          for(Int x=0; x < 4; x++)
          {
            pBuffH[uiCnt] = y*iWidth + x + offset;
            uiCnt ++;
          }
        }
      }
    }

    uiCnt = 0;
    for(Int blkX=0; blkX < numBlkSide; blkX++)
    {
      for(Int blkY=0; blkY < numBlkSide; blkY++)
      {
        UInt offset    = blkY * 4 * iWidth + blkX * 4;
        for(Int x=0; x < 4; x++)
        {
          for(Int y=0; y < 4; y++)
          {
            pBuffV[uiCnt] = y*iWidth + x + offset;
            uiCnt ++;
          }
        }
      }
    }
  }
  else
  {
  for(Int iY=0; iY < iHeight; iY++)
  {
    for(Int iX=0; iX < iWidth; iX++)
    {
      pBuffH[uiCnt] = iY*iWidth + iX;
      uiCnt ++;
    }
  }

  uiCnt = 0;
  for(Int iX=0; iX < iWidth; iX++)
  {
    for(Int iY=0; iY < iHeight; iY++)
    {
      pBuffV[uiCnt] = iY*iWidth + iX;
      uiCnt ++;
    }
  }    
  }
}

Int g_quantTSDefault4x4[16] =
{
  16,16,16,16,
  16,16,16,16,
  16,16,16,16,
  16,16,16,16
};

Int g_quantIntraDefault8x8[64] =
{
  16,16,16,16,17,18,21,24,
  16,16,16,16,17,19,22,25,
  16,16,17,18,20,22,25,29,
  16,16,18,21,24,27,31,36,
  17,17,20,24,30,35,41,47,
  18,19,22,27,35,44,54,65,
  21,22,25,31,41,54,70,88,
  24,25,29,36,47,65,88,115
};

Int g_quantInterDefault8x8[64] =
{
  16,16,16,16,17,18,20,24,
  16,16,16,17,18,20,24,25,
  16,16,17,18,20,24,25,28,
  16,17,18,20,24,25,28,33,
  17,18,20,24,25,28,33,41,
  18,20,24,25,28,33,41,54,
  20,24,25,28,33,41,54,71,
  24,25,28,33,41,54,71,91
};
UInt g_scalingListSize   [4] = {16,64,256,1024}; 
UInt g_scalingListSizeX  [4] = { 4, 8, 16,  32};
UInt g_scalingListNum[SCALING_LIST_SIZE_NUM]={6,6,6,2};
Int  g_eTTable[4] = {0,3,1,2};

#if H_MV_ENC_DEC_TRAC
#if ENC_DEC_TRACE
Void stopAtPos( Int poc, Int layerId, Int cuPelX, Int cuPelY, Int cuWidth, Int cuHeight )
{

  if ( g_outputPos ) 
  {
    std::cout << "POC\t"        << poc 
              << "\tLayerId\t"  << layerId 
              << "\tCuPelX\t"   << cuPelX  
              << "\tCuPelY\t"   << cuPelY 
              << "\tCuWidth\t"  << cuWidth
              << "\tCuHeight\t" << cuHeight
              << std::endl; 
  }

  Bool stopFlag = false; 
  if ( g_stopAtPos && poc == 0 && layerId == 1 )
  {
    Bool stopAtCU = true; 
    if ( stopAtCU )        // Stop at CU with specific size
    {    
      stopFlag = ( cuPelX  == 888 ) && ( cuPelY  == 248 ) && ( cuWidth == 8 ) && ( cuHeight == 8); 
    }
    else
    {                     // Stop at specific position 
      Int xPos = 888; 
      Int yPos = 248; 

      Int cuPelXEnd = cuPelX + cuWidth  - 1; 
      Int cuPelYEnd = cuPelY + cuHeight - 1; 

      stopFlag = (cuPelX <= xPos ) && (cuPelXEnd >= xPos ) && (cuPelY <= yPos ) && (cuPelYEnd >= yPos ); 
    }
  }
  
  if ( stopFlag )
  { // Set breakpoint here. 
    std::cout << "Stop position. Break point here." << std::endl; 
  }  
}

Void writeToTraceFile( const Char* symbolName, Int val, Bool doIt )
{
  if ( ( ( g_nSymbolCounter >= COUNTER_START && g_nSymbolCounter <= COUNTER_END )|| g_bJustDoIt ) && doIt  ) 
  {
    if ( g_stopAtCounter == g_nSymbolCounter )
    {
      std::cout << "Break point here." << std::endl; 
    }
    fprintf( g_hTrace, "%8lld  ", g_nSymbolCounter++ );
    fprintf( g_hTrace, "%-50s       : %d\n", symbolName, val );     
    fflush ( g_hTrace );
    g_nSymbolCounter++; 
  }
}

Void writeToTraceFile( const Char* symbolName, Bool doIt )
{
  if ( ( ( g_nSymbolCounter >= COUNTER_START && g_nSymbolCounter <= COUNTER_END )|| g_bJustDoIt ) && doIt  ) 
  {
    fprintf( g_hTrace, "%s", symbolName );    
    fflush ( g_hTrace );
    g_nSymbolCounter++; 
  }
}

#endif
#endif
#if H_3D_DIM_DMM
std::vector< std::vector<TComWedgelet>  > g_dmmWedgeLists;
std::vector< std::vector<TComWedgeRef>  > g_dmmWedgeRefLists;
std::vector< std::vector<TComWedgeNode> > g_dmmWedgeNodeLists;

Void initWedgeLists( Bool initNodeList )
{
  if( !g_dmmWedgeLists.empty() ) return;

  for( UInt ui = g_aucConvertToBit[DIM_MIN_SIZE]; ui < (g_aucConvertToBit[DIM_MAX_SIZE]); ui++ )
  {
    UInt uiWedgeBlockSize = ((UInt)DIM_MIN_SIZE)<<ui;
    std::vector<TComWedgelet> acWedgeList;
    std::vector<TComWedgeRef> acWedgeRefList;
    createWedgeList( uiWedgeBlockSize, uiWedgeBlockSize, acWedgeList, acWedgeRefList, g_dmmWedgeResolution[ui] );
    g_dmmWedgeLists.push_back( acWedgeList );
    g_dmmWedgeRefLists.push_back( acWedgeRefList );

    if( initNodeList )
    {
      // create WedgeNodeList
      std::vector<TComWedgeNode> acWedgeNodeList;
      for( UInt uiPos = 0; uiPos < acWedgeList.size(); uiPos++ )
      {
        if( acWedgeList[uiPos].getIsCoarse() )
        {
          TComWedgeNode cWedgeNode;
          cWedgeNode.setPatternIdx( uiPos );

          UInt uiRefPos = 0;
          for( Int iOffS = -1; iOffS <= 1; iOffS++ )
          {
            for( Int iOffE = -1; iOffE <= 1; iOffE++ )
            {
              if( iOffS == 0 && iOffE == 0 ) { continue; }

              Int iSx = (Int)acWedgeList[uiPos].getStartX();
              Int iSy = (Int)acWedgeList[uiPos].getStartY();
              Int iEx = (Int)acWedgeList[uiPos].getEndX();
              Int iEy = (Int)acWedgeList[uiPos].getEndY();

              switch( acWedgeList[uiPos].getOri() )
              {
              case( 0 ): { iSx += iOffS; iEy += iOffE; } break;
              case( 1 ): { iSy += iOffS; iEx -= iOffE; } break;
              case( 2 ): { iSx -= iOffS; iEy -= iOffE; } break;
              case( 3 ): { iSy -= iOffS; iEx += iOffE; } break;
              case( 4 ): { iSx += iOffS; iEx += iOffE; } break;
              case( 5 ): { iSy += iOffS; iEy += iOffE; } break;
              default: assert( 0 );
              }

              for( UInt k = 0; k < acWedgeRefList.size(); k++ )
              {
                if( iSx == (Int)acWedgeRefList[k].getStartX() && 
                  iSy == (Int)acWedgeRefList[k].getStartY() && 
                  iEx == (Int)acWedgeRefList[k].getEndX()   && 
                  iEy == (Int)acWedgeRefList[k].getEndY()      )
                {
                  if( acWedgeRefList[k].getRefIdx() != cWedgeNode.getPatternIdx() )
                  {
                    Bool bNew = true;
                    for( UInt m = 0; m < uiRefPos; m++ ) { if( acWedgeRefList[k].getRefIdx() == cWedgeNode.getRefineIdx( m ) ) { bNew = false; break; } }

                    if( bNew ) 
                    {
                      cWedgeNode.setRefineIdx( acWedgeRefList[k].getRefIdx(), uiRefPos );
                      uiRefPos++;
                      break;
                    }
                  }
                }
              }
            }
          }
          acWedgeNodeList.push_back( cWedgeNode );
        }
      }
      g_dmmWedgeNodeLists.push_back( acWedgeNodeList );
    }
  }
}

Void createWedgeList( UInt uiWidth, UInt uiHeight, std::vector<TComWedgelet> &racWedgeList, std::vector<TComWedgeRef> &racWedgeRefList, WedgeResolution eWedgeRes )
{
  assert( uiWidth == uiHeight );

  UChar    uhStartX = 0,    uhStartY = 0,    uhEndX = 0,    uhEndY = 0;
  Int   iStepStartX = 0, iStepStartY = 0, iStepEndX = 0, iStepEndY = 0;

  UInt uiBlockSize = 0;
  switch( eWedgeRes )
  {
  case(   FULL_PEL ): { uiBlockSize =  uiWidth;     break; }
  case(   HALF_PEL ): { uiBlockSize = (uiWidth<<1); break; }
  }

  TComWedgelet cTempWedgelet( uiWidth, uiHeight );
  for( UInt uiOri = 0; uiOri < 6; uiOri++ )
  {
    // init the edge line parameters for each of the 6 wedgelet types
    switch( uiOri )
    {
    case( 0 ): {  uhStartX = 0;               uhStartY = 0;               uhEndX = 0;               uhEndY = 0;               iStepStartX = +1; iStepStartY =  0; iStepEndX =  0; iStepEndY = +1; break; }
    case( 1 ): {  uhStartX = (uiBlockSize-1); uhStartY = 0;               uhEndX = (uiBlockSize-1); uhEndY = 0;               iStepStartX =  0; iStepStartY = +1; iStepEndX = -1; iStepEndY =  0; break; }
    case( 2 ): {  uhStartX = (uiBlockSize-1); uhStartY = (uiBlockSize-1); uhEndX = (uiBlockSize-1); uhEndY = (uiBlockSize-1); iStepStartX = -1; iStepStartY =  0; iStepEndX =  0; iStepEndY = -1; break; }
    case( 3 ): {  uhStartX = 0;               uhStartY = (uiBlockSize-1); uhEndX = 0;               uhEndY = (uiBlockSize-1); iStepStartX =  0; iStepStartY = -1; iStepEndX = +1; iStepEndY =  0; break; }
    case( 4 ): {  uhStartX = 0;               uhStartY = 0;               uhEndX = 0;               uhEndY = (uiBlockSize-1); iStepStartX = +1; iStepStartY =  0; iStepEndX = +1; iStepEndY =  0; break; }
    case( 5 ): {  uhStartX = (uiBlockSize-1); uhStartY = 0;               uhEndX = 0;               uhEndY = 0;               iStepStartX =  0; iStepStartY = +1; iStepEndX =  0; iStepEndY = +1; break; }
    }

#if MTK_DMM_SIM_J0035
    for( Int iK = 0; iK < uiBlockSize; iK += (uiWidth>=16 ?2:1))
    {
      for( Int iL = 0; iL < uiBlockSize; iL += ((uiWidth>=16 && uiOri<4)?2:1) )
      {
        cTempWedgelet.setWedgelet( uhStartX + (iK*iStepStartX) , uhStartY + (iK*iStepStartY), uhEndX + (iL*iStepEndX), uhEndY + (iL*iStepEndY), (UChar)uiOri, eWedgeRes, ((iL%2)==0 && (iK%2)==0) );
        addWedgeletToList( cTempWedgelet, racWedgeList, racWedgeRefList );
      }
    }
#else
    for( Int iK = 0; iK < uiBlockSize; iK++ )
    {
      for( Int iL = 0; iL < uiBlockSize; iL++ )
      {
        cTempWedgelet.setWedgelet( uhStartX + (iK*iStepStartX) , uhStartY + (iK*iStepStartY), uhEndX + (iL*iStepEndX), uhEndY + (iL*iStepEndY), (UChar)uiOri, eWedgeRes, ((iL%2)==0 && (iK%2)==0) );
        addWedgeletToList( cTempWedgelet, racWedgeList, racWedgeRefList );
      }
    }
#endif
  }


}

Void addWedgeletToList( TComWedgelet cWedgelet, std::vector<TComWedgelet> &racWedgeList, std::vector<TComWedgeRef> &racWedgeRefList )
{
  Bool bValid = cWedgelet.checkNotPlain();
  if( bValid )
  {
    for( UInt uiPos = 0; uiPos < racWedgeList.size(); uiPos++ )
    {
      if( cWedgelet.checkIdentical( racWedgeList[uiPos].getPattern() ) )
      {
        TComWedgeRef cWedgeRef;
        cWedgeRef.setWedgeRef( cWedgelet.getStartX(), cWedgelet.getStartY(), cWedgelet.getEndX(), cWedgelet.getEndY(), uiPos );
        racWedgeRefList.push_back( cWedgeRef );
        bValid = false;
        return;
      }
    }
  }
  if( bValid )
  {
    for( UInt uiPos = 0; uiPos < racWedgeList.size(); uiPos++ )
    {
      if( cWedgelet.checkInvIdentical( racWedgeList[uiPos].getPattern() ) )
      {
        TComWedgeRef cWedgeRef;
        cWedgeRef.setWedgeRef( cWedgelet.getStartX(), cWedgelet.getStartY(), cWedgelet.getEndX(), cWedgelet.getEndY(), uiPos );
        racWedgeRefList.push_back( cWedgeRef );
        bValid = false;
        return;
      }
    }
  }
  if( bValid )
  {
    cWedgelet.findClosestAngle();
    racWedgeList.push_back( cWedgelet );
    TComWedgeRef cWedgeRef;
    cWedgeRef.setWedgeRef( cWedgelet.getStartX(), cWedgelet.getStartY(), cWedgelet.getEndX(), cWedgelet.getEndY(), (UInt)(racWedgeList.size()-1) );
    racWedgeRefList.push_back( cWedgeRef );
  }
}
#endif //H_3D_DIM_DMM

//! \}
