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
  for ( i=4; i<MAX_CU_SIZE; i*=2 )
  {
    g_aucConvertToBit[ i ] = c;
    c++;
  }
  g_aucConvertToBit[ i ] = c;
  
  // g_auiFrameScanXY[ g_aucConvertToBit[ transformSize ] ]: zigzag scan array for transformSize
  c=2;
  for ( i=0; i<MAX_CU_DEPTH; i++ )
  {
    g_auiFrameScanXY[ i ] = new UInt[ c*c ];
    g_auiFrameScanX [ i ] = new UInt[ c*c ];
    g_auiFrameScanY [ i ] = new UInt[ c*c ];
    initFrameScanXY( g_auiFrameScanXY[i], g_auiFrameScanX[i], g_auiFrameScanY[i], c, c );
    g_auiSigLastScan[0][i] = new UInt[ c*c ];
    g_auiSigLastScan[1][i] = new UInt[ c*c ];
    g_auiSigLastScan[2][i] = new UInt[ c*c ];
    g_auiSigLastScan[3][i] = new UInt[ c*c ];
    initSigLastScan( g_auiSigLastScan[0][i], g_auiSigLastScan[1][i], g_auiSigLastScan[2][i], g_auiSigLastScan[3][i], c, c, i);

    c <<= 1;
  }  

  g_sigScanNSQT[0] = new UInt[ 64 ];  // 4x16
  g_sigScanNSQT[1] = new UInt[ 256 ]; // 8x32
  g_sigScanNSQT[2] = new UInt[ 64 ];  // 16x4
  g_sigScanNSQT[3] = new UInt[ 256 ]; // 32x8
  
  static int diagScanX[ 16 ] =
  {
    0, 0, 1, 0, 1, 2, 0, 1, 2, 3, 1, 2, 3, 2, 3, 3
  };
  static int diagScanY[ 16 ] =
  {
    0, 1, 0, 2, 1, 0, 3, 2, 1, 0, 3, 2, 1, 3, 2, 3
  };
  
  Int j;
  // 4x16 scan
  for (i = 0; i < 4; i++)
  {
    for (j = 0; j < 16; j++)
    {
      g_sigScanNSQT[ 0 ][ 16 * i + j ] = 16 * i + 4 * diagScanY[ j ] + diagScanX[ j ];
    }
  }
  
  // 8x32 scan
  for (i = 0; i < 16; i++)
  {
    Int x = g_sigCGScanNSQT[ 1 ][ i ] & 1;
    Int y = g_sigCGScanNSQT[ 1 ][ i ] >> 1;
    
    for (j = 0; j < 16; j++)
    {
      g_sigScanNSQT[ 1 ][ 16 * i + j ] = 32 * y + 4 * x + 8 * diagScanY[ j ] + diagScanX[ j ];
    }
  }
  
  // 16x4 scan
  for (i = 0; i < 4; i++)
  {
    for (j = 0; j < 16; j++)
    {
      g_sigScanNSQT[ 2 ][ 16 * i + j ] = 4 * i + 16 * diagScanY[ j ] + diagScanX[ j ];
    }
  }
  
  // 32x8 scan
  for (i = 0; i < 16; i++)
  {
    Int x = g_sigCGScanNSQT[ 3 ][ i ] & 7;
    Int y = g_sigCGScanNSQT[ 3 ][ i ] >> 3;
    
    for (j = 0; j < 16; j++)
    {
      g_sigScanNSQT[ 3 ][ 16 * i + j ] = 128 * y + 4 * x + 32 * diagScanY[ j ] + diagScanX[ j ];
    }
  }
}

Void destroyROM()
{
  Int i;
  
  for ( i=0; i<MAX_CU_DEPTH; i++ )
  {
    delete[] g_auiFrameScanXY[i];
    delete[] g_auiFrameScanX [i];
    delete[] g_auiFrameScanY [i];
    delete[] g_auiSigLastScan[0][i];
    delete[] g_auiSigLastScan[1][i];
    delete[] g_auiSigLastScan[2][i];
    delete[] g_auiSigLastScan[3][i];
  }
  for (i = 0; i < 4; i++)
  {
    delete[] g_sigScanNSQT[ i ];    
  }

#if HHI_DMM_WEDGE_INTRA || HHI_DMM_PRED_TEX
  if ( !g_aacWedgeLists.empty() )
  {
    for ( UInt ui = 0; ui < g_aacWedgeLists.size(); ui++ )
    {
      g_aacWedgeLists[ui].clear();
    }
    g_aacWedgeLists.clear();
  }

  if ( !g_aacWedgeRefLists.empty() )
  {
    for ( UInt ui = 0; ui < g_aacWedgeRefLists.size(); ui++ )
    {
      g_aacWedgeRefLists[ui].clear();
    }
    g_aacWedgeRefLists.clear();
  }
#if HHIQC_DMMFASTSEARCH_B0039
  if ( !g_aacWedgeNodeLists.empty() )
  {
    for ( UInt ui = 0; ui < g_aacWedgeNodeLists.size(); ui++ )
    {
      g_aacWedgeNodeLists[ui].clear();
    }
    g_aacWedgeNodeLists.clear();
  }
#endif
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
UInt g_motionRefer   [ MAX_NUM_SPU_W*MAX_NUM_SPU_W ] = { 0, }; 

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

/** generate motion data compression mapping table
* \param uiMaxCUWidth, width of LCU
* \param uiMaxCUHeight, hight of LCU
* \param uiMaxDepth, max depth of LCU
* \returns Void
*/
Void initMotionReferIdx ( UInt uiMaxCUWidth, UInt uiMaxCUHeight, UInt uiMaxDepth )
{
  Int  minCUWidth  = (Int)uiMaxCUWidth  >> ( (Int)uiMaxDepth - 1 );
  Int  minCUHeight = (Int)uiMaxCUHeight >> ( (Int)uiMaxDepth - 1 );

  Int  numPartInWidth  = (Int)uiMaxCUWidth  / (Int)minCUWidth;
  Int  numPartInHeight = (Int)uiMaxCUHeight / (Int)minCUHeight;

  for ( Int i = 0; i < numPartInWidth*numPartInHeight; i++ )
  {
    g_motionRefer[i] = i;
  }

  Int compressionNum = 2;

  for ( Int i = numPartInWidth*(numPartInHeight-1); i < numPartInWidth*numPartInHeight; i += compressionNum*2)
  {
    for ( Int j = 1; j < compressionNum; j++ )
    {
      g_motionRefer[g_auiRasterToZscan[i+j]] = g_auiRasterToZscan[i];
    }
  }

  for ( Int i = numPartInWidth*(numPartInHeight-1)+compressionNum*2-1; i < numPartInWidth*numPartInHeight; i += compressionNum*2)
  {
    for ( Int j = 1; j < compressionNum; j++ )
    {
      g_motionRefer[g_auiRasterToZscan[i-j]] = g_auiRasterToZscan[i];
    }
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

#if RWTH_SDC_DLT_B0036
#if SAIT_SDC_C0096
UInt g_auiSDCPredModes[RWTH_SDC_NUM_PRED_MODES] = { DC_IDX, DMM_WEDGE_FULL_IDX, PLANAR_IDX };
#else
UInt g_auiSDCPredModes[RWTH_SDC_NUM_PRED_MODES] = { DC_IDX, DMM_WEDGE_FULL_IDX, DMM_WEDGE_PREDDIR_IDX, PLANAR_IDX };
#endif
#endif

Int g_quantScales[6] =
{
  26214,23302,20560,18396,16384,14564
};    

Int g_invQuantScales[6] =
{
  40,45,51,57,64,72
};

const short g_aiT4[4][4] =
{
  { 64, 64, 64, 64},
  { 83, 36,-36,-83},
  { 64,-64,-64, 64},
  { 36,-83, 83,-36}
};

const short g_aiT8[8][8] =
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

const short g_aiT16[16][16] =
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

const short g_aiT32[32][32] =
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

const UChar g_aucChromaScale[52]=
{
  0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,
  12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,
  28,29,29,30,31,32,32,33,34,34,35,35,36,36,37,37,
  37,38,38,38,39,39,39,39
};


// Mode-Dependent DCT/DST 
const short g_as_DST_MAT_4 [4][4]=
{
  {29,   55,    74,   84},
  {74,   74,    0 ,  -74},
  {84,  -29,   -74,   55},
  {55,  -84,    74,  -29},
};

#if !LOGI_INTRA_NAME_3MPM
// Mapping each Unified Directional Intra prediction direction to DCT/DST transform 
// 0 implies use DCT, 1 implies DST

#if HHI_DMM_WEDGE_INTRA || HHI_DMM_PRED_TEX
const UChar g_aucDCTDSTMode_Vert[NUM_INTRA_MODE+NUM_DMM_MODE] =
#else
const UChar g_aucDCTDSTMode_Vert[NUM_INTRA_MODE] =
#endif
{ //0  1  2  3  4  5  6  7  8  9 10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 25 26 27 28 29 30 31 32 33 34 35
  1, 1, 0, 0, 1, 1, 1, 1, 1, 0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0
#if HHI_DMM_WEDGE_INTRA && HHI_DMM_PRED_TEX
  , 0, 0, 0, 0, 0, 0, 0, 0
#elif HHI_DMM_WEDGE_INTRA || HHI_DMM_PRED_TEX
  , 0, 0, 0, 0
#endif
};
#if HHI_DMM_WEDGE_INTRA || HHI_DMM_PRED_TEX
const UChar g_aucDCTDSTMode_Hor[NUM_INTRA_MODE+NUM_DMM_MODE] =
#else
const UChar g_aucDCTDSTMode_Hor[NUM_INTRA_MODE] =
#endif
{ //0  1  2  3  4  5  6  7  8  9 10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 25 26 27 28 29 30 31 32 33 34 35
  1, 0, 1, 0, 1, 1, 0, 0, 1, 1, 1, 1, 1, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 0
#if HHI_DMM_WEDGE_INTRA && HHI_DMM_PRED_TEX
  , 0, 0, 0, 0, 0, 0, 0, 0
#elif HHI_DMM_WEDGE_INTRA || HHI_DMM_PRED_TEX
  , 0, 0, 0, 0
#endif
};
#endif


// ====================================================================================================================
// ADI
// ====================================================================================================================

#if FAST_UDI_USE_MPM
const UChar g_aucIntraModeNumFast[7] =
{
  3,  //   2x2
  8,  //   4x4
  8,  //   8x8
  3,  //  16x16   
  3,  //  32x32   
  3,  //  64x64   
  3   // 128x128  
};
#else // FAST_UDI_USE_MPM
const UChar g_aucIntraModeNumFast[7] =
{
  3,  //   2x2
  9,  //   4x4
  9,  //   8x8
  4,  //  16x16   33
  4,  //  32x32   33
  5,  //  64x64   33
  4   // 128x128  33
};
#endif // FAST_UDI_USE_MPM

// chroma

const UChar g_aucConvertTxtTypeToIdx[4] = { 0, 1, 1, 2 };

#if !LOGI_INTRA_NAME_3MPM
// ====================================================================================================================
// Angular Intra prediction
// ====================================================================================================================

// g_aucAngIntraModeOrder
//   Indexing this array with the mode indicated in the bitstream
//   gives a logical index used in the prediction functions.
const UChar g_aucAngIntraModeOrder[NUM_INTRA_MODE] =
{     //  ModeOrder LogicalOrderInPredFunctions
  34, //  PLANAR_IDX PLANAR PLANAR
  9,  //  0 VER     DC
  25, //  1 HOR     VER-8 (diagonal from top-left to bottom-right = HOR-8)
  0,  //  2 DC      VER-7
  1,  //  4 VER-8   VER-6
  5,  //  5 VER-4   VER-5
  13, //  6 VER+4   VER-4
  17, //  7 VER+8   VER-3
  21, //  8 HOR-4   VER-2
  29, //  9 HOR+4   VER-1
  33, // 10 HOR+8   VER
  3,  // 11 VER-6   VER+1
  7,  // 12 VER-2   VER+2
  11, // 13 VER+2   VER+3
  15, // 14 VER+6   VER+4
  19, // 15 HOR-6   VER+5
  23, // 16 HOR-2   VER+6
  27, // 17 HOR+2   VER+7
  31, // 18 HOR+6   VER+8
  2,  // 19 VER-7   HOR-7
  4,  // 20 VER-5   HOR-6
  6,  // 21 VER-3   HOR-5
  8,  // 22 VER-1   HOR-4
  10, // 23 VER+1   HOR-3
  12, // 24 VER+3   HOR-2
  14, // 25 VER+5   HOR-1
  16, // 26 VER+7   HOR
  18, // 27 HOR-7   HOR+1
  20, // 28 HOR-5   HOR+2
  22, // 29 HOR-3   HOR+3
  24, // 30 HOR-1   HOR+4
  26, // 31 HOR+1   HOR+5
  28, // 32 HOR+3   HOR+6
  30, // 33 HOR+5   HOR+7
  32, // 34 HOR+7   HOR+8
  0, // LM_CHROMA_IDX 
};

const UChar g_aucIntraModeNumAng[7] =
{
  4,  //   2x2
  18,  //   4x4
  35,  //   8x8
  35,  //  16x16
  35,  //  32x32
  35,  //  64x64
  6   // 128x128
};

const UChar g_aucIntraModeBitsAng[7] =
{
  2,  //   2x2     3   1+1
  5,  //   4x4    17   4+1
  6,  //   8x8    34   5+esc
  6,  //  16x16   34   5+esc
  6,  //  32x32   34   5+esc
  6,  //  64x64   34   5+esc
  3   // 128x128   5   2+1
};
#endif

// ====================================================================================================================
// Bit-depth
// ====================================================================================================================

UInt g_uiBitDepth     = 8;    // base bit-depth
UInt g_uiBitIncrement = 0;    // increments
UInt g_uiIBDI_MAX     = 255;  // max. value after  IBDI
UInt g_uiBASE_MAX     = 255;  // max. value before IBDI

UInt g_uiPCMBitDepthLuma     = 8;    // PCM bit-depth
UInt g_uiPCMBitDepthChroma   = 8;    // PCM bit-depth

// ====================================================================================================================
// Depth model modes
// ====================================================================================================================
#if HHI_DMM_WEDGE_INTRA || HHI_DMM_PRED_TEX
Int g_iDeltaDCsQuantOffset = 0;

const WedgeResolution g_aeWedgeResolutionList[5] = 
{
  HALF_PEL,    //  4x4
  HALF_PEL,    //  8x8
  FULL_PEL,    // 16x16
  DOUBLE_PEL,  // 32x32
  DOUBLE_PEL   // 64x64
};

const UChar g_aucWedgeFullBitsListIdx[7] =
{
  0,   //   2x2
  7,   //   4x4    WedgeListSize[  HALF_PEL]   86
  10,  //   8x8    WedgeListSize[  HALF_PEL]  782
  11,  //  16x16   WedgeListSize[  FULL_PEL] 1394
  11,  //  32x32   WedgeListSize[DOUBLE_PEL] 1503
  13,  //  64x64   WedgeListSize[DOUBLE_PEL] 6079
  0    // 128x128  
};

#if LGE_DMM3_SIMP_C0044
const UChar g_aucWedgeTexPredBitsListIdx[7] =
{
  0,   //   2x2
  6,   //   4x4    
  9,  //   8x8    
  9,  //  16x16   
  9,  //  32x32   
  0,  //  64x64   
  0    // 128x128  
};
#endif

const UChar g_aucIntraSizeIdxToWedgeSize[7] =
{
  2,
  4,
  8,
  16,
  32,
  64,
  128
};
#endif

// ====================================================================================================================
// Misc.
// ====================================================================================================================

Char  g_aucConvertToBit  [ MAX_CU_SIZE+1 ];

#if ENC_DEC_TRACE
FILE*  g_hTrace = NULL;
const Bool g_bEncDecTraceEnable  = true;
const Bool g_bEncDecTraceDisable = false;
Bool   g_bJustDoIt = false;
UInt64 g_nSymbolCounter = 0;
#endif
// ====================================================================================================================
// Scanning order & context model mapping
// ====================================================================================================================

// scanning order table
UInt* g_auiFrameScanXY[ MAX_CU_DEPTH  ];
UInt* g_auiFrameScanX [ MAX_CU_DEPTH  ];
UInt* g_auiFrameScanY [ MAX_CU_DEPTH  ];
UInt* g_auiSigLastScan[4][ MAX_CU_DEPTH ];
UInt *g_sigScanNSQT[ 4 ]; // scan for non-square partitions
UInt g_sigCGScanNSQT[ 4 ][ 16 ] =
{
  { 0, 1, 2, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  { 0, 2, 1, 4, 3, 6, 5, 8, 7, 10, 9, 12, 11, 14, 13, 15 },
  { 0, 1, 2, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  { 0, 8, 1, 9, 2, 10, 3, 11, 4, 12, 5, 13, 6, 14, 7, 15 }
};

#if MULTILEVEL_SIGMAP_EXT
const UInt g_sigLastScan8x8[ 4 ][ 4 ] =
{
  {0, 1, 2, 3},
  {0, 1, 2, 3},
  {0, 1, 2, 3},
  {0, 2, 1, 3}
};
UInt g_sigLastScanCG32x32[ 64 ];
#endif

UInt* g_auiNonSquareSigLastScan[ 4 ];

const UInt g_uiMinInGroup[ 10 ] = {0,1,2,3,4,6,8,12,16,24};
const UInt g_uiGroupIdx[ 32 ]   = {0,1,2,3,4,4,5,5,6,6,6,6,7,7,7,7,8,8,8,8,8,8,8,8,9,9,9,9,9,9,9,9};
#if LAST_CTX_REDUCTION
const UInt g_uiLastCtx[ 28 ]    = 
{
  0,   1,  2,  2,                         // 4x4    4
  3,   4,  5,  5, 2,  2,                  // 8x8    6  
  6,   7,  8,  8, 9,  9, 2, 2,            // 16x16  8
  10, 11, 12, 12, 13, 13, 14, 14, 2, 2    // 32x32  10
};
#else
const UInt g_uiLastCtx[ 28 ]    = 
{
  0,   1,  2,  2,                         // 4x4    4
  3,   4,  5,  5, 6,  6,                  // 8x8    6  
  7,   8,  9,  9, 10, 10, 11, 11,         // 16x16  8
  12, 13, 14, 14, 15, 15, 16, 16, 17, 17  // 32x32  10
};
#endif

// Rice parameters for absolute transform levels
#if EIGHT_BITS_RICE_CODE
const UInt g_auiGoRiceRange[5] =
{
  7, 14, 26, 46, 78
};

const UInt g_auiGoRicePrefixLen[5] =
{
  8, 7, 6, 5, 4
};
#else
const UInt g_auiGoRiceRange[4] =
{
  7, 20, 42, 70
};

const UInt g_auiGoRicePrefixLen[4] =
{
  8, 10, 10, 8
};
#endif

#if EIGHT_BITS_RICE_CODE
const UInt g_aauiGoRiceUpdate[5][24] =
{
#if RESTRICT_GR1GR2FLAG_NUMBER
  {
    0, 0, 0, 1, 1, 1, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 4
  },
  {
    1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 4
  },
  {
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 4
  },
  {
    3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 4
  },
  {
    4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4
  }
#else
  {
    0, 0, 1, 1, 1, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 4
  },
  {
    1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 4
  },
  {
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 4
  },
  {
    3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 4
  },
  {
    4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4
  }
#endif
};
#else
const UInt g_aauiGoRiceUpdate[4][16] =
{
#if RESTRICT_GR1GR2FLAG_NUMBER
  {
    0, 0, 0, 1, 1, 1, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3,
  },
  { 
    1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3,
  },
  { 
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3,
  },
  { 
    3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 
  }
#else  
  {
    0, 0, 1, 1, 1, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3
  },
  {
    1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3
  },
  {
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3
  },
  {
    3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3
  }
#endif
};
#endif

// initialize g_auiFrameScanXY
Void initFrameScanXY( UInt* pBuff, UInt* pBuffX, UInt* pBuffY, Int iWidth, Int iHeight )
{
  Int x, y, c = 0;
  
  // starting point
  pBuffX[ c ] = 0;
  pBuffY[ c ] = 0;
  pBuff[ c++ ] = 0;
  
  // loop
  x=1; y=0;
  while (1)
  {
    // decrease loop
    while ( x>=0 )
    {
      if ( x >= 0 && x < iWidth && y >= 0 && y < iHeight )
      {
        pBuffX[ c ] = x;
        pBuffY[ c ] = y;
        pBuff[ c++ ] = x+y*iWidth;
      }
      x--; y++;
    }
    x=0;
    
    // increase loop
    while ( y>=0 )
    {
      if ( x >= 0 && x < iWidth && y >= 0 && y < iHeight )
      {
        pBuffX[ c ] = x;
        pBuffY[ c ] = y;
        pBuff[ c++ ] = x+y*iWidth;
      }
      x++; y--;
    }
    y=0;
    
    // termination condition
    if ( c >= iWidth*iHeight ) break;
  }  
}

Void initSigLastScan(UInt* pBuffZ, UInt* pBuffH, UInt* pBuffV, UInt* pBuffD, Int iWidth, Int iHeight, Int iDepth)
{
  const UInt  uiNumScanPos  = UInt( iWidth * iWidth );
  UInt        uiNextScanPos = 0;

  if( iWidth < 16 )
  {
#if MULTILEVEL_SIGMAP_EXT
  UInt* pBuffTemp = pBuffD;
  if( iWidth == 8 )
  {
    pBuffTemp = g_sigLastScanCG32x32;
  }
#endif
  for( UInt uiScanLine = 0; uiNextScanPos < uiNumScanPos; uiScanLine++ )
  {
    int    iPrimDim  = int( uiScanLine );
    int    iScndDim  = 0;
    while( iPrimDim >= iWidth )
    {
      iScndDim++;
      iPrimDim--;
    }
    while( iPrimDim >= 0 && iScndDim < iWidth )
    {
#if MULTILEVEL_SIGMAP_EXT
      pBuffTemp[ uiNextScanPos ] = iPrimDim * iWidth + iScndDim ;
#else
      pBuffD[ uiNextScanPos ] = iPrimDim * iWidth + iScndDim ;
#endif
      uiNextScanPos++;
      iScndDim++;
      iPrimDim--;
    }
  }
  }
#if MULTILEVEL_SIGMAP_EXT
  if( iWidth > 4 )
#else
  else
#endif
  {
    UInt uiNumBlkSide = iWidth >> 2;
    UInt uiNumBlks    = uiNumBlkSide * uiNumBlkSide;
    UInt log2Blk      = g_aucConvertToBit[ uiNumBlkSide ] + 1;

    for( UInt uiBlk = 0; uiBlk < uiNumBlks; uiBlk++ )
    {
      uiNextScanPos   = 0;
      UInt initBlkPos = g_auiSigLastScan[ SCAN_DIAG ][ log2Blk ][ uiBlk ];
#if MULTILEVEL_SIGMAP_EXT
      if( iWidth == 32 )
      {
        initBlkPos = g_sigLastScanCG32x32[ uiBlk ];
      }
#endif
      UInt offsetY    = initBlkPos / uiNumBlkSide;
      UInt offsetX    = initBlkPos - offsetY * uiNumBlkSide;
      UInt offsetD    = 4 * ( offsetX + offsetY * iWidth );
      UInt offsetScan = 16 * uiBlk;
      for( UInt uiScanLine = 0; uiNextScanPos < 16; uiScanLine++ )
      {
        int    iPrimDim  = int( uiScanLine );
        int    iScndDim  = 0;
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
  
  memcpy(pBuffZ, g_auiFrameScanXY[iDepth], sizeof(UInt)*iWidth*iHeight);

  UInt uiCnt = 0;
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

Void initNonSquareSigLastScan(UInt* pBuffZ, UInt uiWidth, UInt uiHeight)
{

  Int x, y, c = 0;

  // starting point
  pBuffZ[ c++ ] = 0;

  // loop
  if ( uiWidth > uiHeight )
  {
    x=0; y=1;
    while (1)
    {
      // increase loop
      while ( y>=0 )
      {
        if ( x >= 0 && x < uiWidth && y >= 0 && y < uiHeight )
        {
          pBuffZ[ c++ ] = x + y * uiWidth;
        }
        x++;
        y--;
      }
      y=0;

      // decrease loop
      while ( x>=0 )
      {
        if ( x >= 0 && x < uiWidth && y >= 0 && y < uiHeight )
        {
          pBuffZ[ c++ ] = x + y * uiWidth;
        }
        x--;
        y++;
      }
      x=0;

      // termination condition
      if ( c >= uiWidth * uiHeight ) 
        break;
    }
  }
  else
  {
    x=1; y=0;
    while (1)
    {
      // increase loop
      while ( x>=0 )
      {
        if ( x >= 0 && x < uiWidth && y >= 0 && y < uiHeight )
        {
          pBuffZ[ c++ ] = x + y * uiWidth;
        }
        x--;
        y++;
      }
      x=0;

      // decrease loop
      while ( y>=0 )
      {
        if ( x >= 0 && x < uiWidth && y >= 0 && y < uiHeight )
        {
          pBuffZ[ c++ ] = x + y * uiWidth;
        }
        x++;
        y--;
      }
      y=0;

      // termination condition
      if ( c >= uiWidth * uiHeight ) 
        break;
    }
  }
}

#if SCALING_LIST
Int g_quantIntraDefault4x4[16] =
{
  16,16,17,21,
  16,17,20,25,
  17,20,30,41,
  21,25,41,70
};
Int g_quantInterDefault4x4[16] =
{
  16,16,17,21,
  16,17,21,24,
  17,21,24,36,
  21,24,36,57
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
#else
Int g_quantIntraDefault4x4[16] =
{
   6,13,20,28,
  13,20,28,32,
  20,28,32,37,
  28,32,37,42
};
Int g_quantInterDefault4x4[16] =
{
  10,14,20,24,
  14,20,24,27,
  20,24,27,30,
  24,27,30,34
};

Int g_quantIntraDefault8x8[64] =
{
   6,10,13,16,18,23,25,27,
  10,11,16,18,23,25,27,29,
  13,16,18,23,25,27,29,31,
  16,18,23,25,27,29,31,33,
  18,23,25,27,29,31,33,36,
  23,25,27,29,31,33,36,38,
  25,27,29,31,33,36,38,40,
  27,29,31,33,36,38,40,42
};

Int g_quantInterDefault8x8[64] =
{
   9,13,15,17,19,21,22,24,
  13,13,17,19,21,22,24,25,
  15,17,19,21,22,24,25,27,
  17,19,21,22,24,25,27,28,
  19,21,22,24,25,27,28,30,
  21,22,24,25,27,28,30,32,
  22,24,25,27,28,30,32,33,
  24,25,27,28,30,32,33,35
};

Int g_quantIntraDefault16x16[256] =
{
  16,16,16,16,16,16,16,16,17,17,18,19,21,22,24,27,
  16,16,16,16,16,16,16,16,17,18,18,20,21,23,25,27,
  16,16,16,16,16,16,16,17,17,18,19,20,22,23,25,28,
  16,16,16,16,16,16,17,17,18,19,20,21,23,25,27,29,
  16,16,16,16,17,17,18,19,20,21,22,23,25,27,29,31,
  16,16,16,16,17,18,19,20,22,23,24,26,27,29,32,34,
  16,16,16,17,18,19,21,23,24,25,27,29,31,33,36,39,
  16,16,17,17,19,20,23,25,27,29,31,33,35,38,41,44,
  17,17,17,18,20,22,24,27,30,32,35,38,41,44,47,51,
  17,18,18,19,21,23,25,29,32,36,40,43,47,51,55,60,
  18,18,19,20,22,24,27,31,35,40,44,49,54,59,65,70,
  19,20,20,21,23,26,29,33,38,43,49,56,62,69,75,82,
  21,21,22,23,25,27,31,35,41,47,54,62,70,79,88,97,
  22,23,23,25,27,29,33,38,44,51,59,69,79,90,101,113,
  24,25,25,27,29,32,36,41,47,55,65,75,88,101,115,130,
  27,27,28,29,31,34,39,44,51,60,70,82,97,113,130,149
};

Int g_quantInterDefault16x16[256] =
{
  16,16,16,16,16,16,16,16,17,17,18,19,20,21,22,27,
  16,16,16,16,16,16,16,17,17,18,19,20,21,22,27,27,
  16,16,16,16,16,16,17,17,18,19,20,21,22,27,27,28,
  16,16,16,16,16,17,17,18,19,20,21,22,27,27,28,29,
  16,16,16,16,17,17,18,19,20,21,22,27,27,28,29,30,
  16,16,16,17,17,18,19,20,21,22,27,27,28,29,30,32,
  16,16,17,17,18,19,20,21,22,27,27,28,29,30,32,36,
  16,17,17,18,19,20,21,22,27,27,28,29,30,32,36,40,
  17,17,18,19,20,21,22,27,27,28,29,30,32,36,40,45,
  17,18,19,20,21,22,27,27,28,29,30,32,36,40,45,52,
  18,19,20,21,22,27,27,28,29,30,32,36,40,45,52,59,
  19,20,21,22,27,27,28,29,30,32,36,40,45,52,59,68,
  20,21,22,27,27,28,29,30,32,36,40,45,52,59,68,79,
  21,22,27,27,28,29,30,32,36,40,45,52,59,68,79,91,
  22,27,27,28,29,30,32,36,40,45,52,59,68,79,91,103,
  27,27,28,29,30,32,36,40,45,52,59,68,79,91,103,117
};

Int g_quantIntraDefault32x32[1024] =
{
  16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,17,17,17,18,18,19,19,20,21,22,22,23,24,25,27,28,
  16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,17,17,17,18,18,19,19,20,21,22,23,23,24,25,27,28,
  16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,17,17,17,18,18,18,19,20,20,21,22,23,24,25,26,27,28,
  16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,17,17,17,18,18,19,19,20,21,21,22,23,24,25,26,27,28,
  16,16,16,16,16,16,16,16,16,16,16,16,16,16,17,17,17,18,18,19,19,20,20,21,22,23,23,24,25,26,28,29,
  16,16,16,16,16,16,16,16,16,16,16,16,16,17,17,17,18,18,18,19,19,20,21,21,22,23,24,25,26,27,28,30,
  16,16,16,16,16,16,16,16,16,16,16,17,17,17,17,18,18,19,19,20,20,21,21,22,23,24,25,26,27,28,29,30,
  16,16,16,16,16,16,16,16,16,17,17,17,17,18,18,18,19,19,20,20,21,21,22,23,24,25,26,27,28,29,30,31,
  16,16,16,16,16,16,16,16,17,17,17,18,18,18,19,19,20,20,21,21,22,22,23,24,25,26,27,28,29,30,31,33,
  16,16,16,16,16,16,16,17,17,17,18,18,19,19,20,20,20,21,22,22,23,23,24,25,26,27,28,29,30,31,33,34,
  16,16,16,16,16,16,16,17,17,18,18,19,19,20,20,21,22,22,23,23,24,25,26,26,27,28,29,30,32,33,34,36,
  16,16,16,16,16,16,17,17,18,18,19,20,20,21,21,22,23,23,24,25,25,26,27,28,29,30,31,32,33,35,36,38,
  16,16,16,16,16,16,17,17,18,19,19,20,21,22,23,23,24,25,25,26,27,28,29,30,31,32,33,34,36,37,39,40,
  16,16,16,16,16,17,17,18,18,19,20,21,22,23,24,24,25,26,27,28,29,30,31,32,33,34,35,37,38,39,41,43,
  16,16,16,16,17,17,17,18,19,20,20,21,23,24,25,26,27,28,29,30,31,32,33,34,35,36,38,39,41,42,44,46,
  16,16,17,17,17,17,18,18,19,20,21,22,23,24,26,27,28,29,31,32,33,34,35,37,38,39,41,42,44,45,47,49,
  17,17,17,17,17,18,18,19,20,20,22,23,24,25,27,28,30,31,32,34,35,36,38,39,41,42,44,45,47,49,51,53,
  17,17,17,17,18,18,19,19,20,21,22,23,25,26,28,29,31,33,34,36,37,39,41,42,44,45,47,49,51,53,55,57,
  17,17,18,18,18,18,19,20,21,22,23,24,25,27,29,31,32,34,36,38,40,42,43,45,47,49,51,53,55,57,60,62,
  18,18,18,18,19,19,20,20,21,22,23,25,26,28,30,32,34,36,38,40,42,44,46,48,51,53,55,57,60,62,65,67,
  18,18,18,19,19,19,20,21,22,23,24,25,27,29,31,33,35,37,40,42,44,47,49,52,54,57,59,62,65,67,70,73,
  19,19,19,19,20,20,21,21,22,23,25,26,28,30,32,34,36,39,42,44,47,50,52,55,58,61,64,67,70,73,76,79,
  19,19,20,20,20,21,21,22,23,24,26,27,29,31,33,35,38,41,43,46,49,52,56,59,62,65,69,72,75,79,82,86,
  20,20,20,21,21,21,22,23,24,25,26,28,30,32,34,37,39,42,45,48,52,55,59,62,66,70,74,77,81,85,89,93,
  21,21,21,21,22,22,23,24,25,26,27,29,31,33,35,38,41,44,47,51,54,58,62,66,70,74,79,83,88,92,97,101,
  22,22,22,22,23,23,24,25,26,27,28,30,32,34,36,39,42,45,49,53,57,61,65,70,74,79,84,89,94,99,104,110,
  22,23,23,23,23,24,25,26,27,28,29,31,33,35,38,41,44,47,51,55,59,64,69,74,79,84,90,95,101,107,113,119,
  23,23,24,24,24,25,26,27,28,29,30,32,34,37,39,42,45,49,53,57,62,67,72,77,83,89,95,101,108,114,121,128,
  24,24,25,25,25,26,27,28,29,30,32,33,36,38,41,44,47,51,55,60,65,70,75,81,88,94,101,108,115,122,130,138,
  25,25,26,26,26,27,28,29,30,31,33,35,37,39,42,45,49,53,57,62,67,73,79,85,92,99,107,114,122,131,139,148,
  27,27,27,27,28,28,29,30,31,33,34,36,39,41,44,47,51,55,60,65,70,76,82,89,97,104,113,121,130,139,149,159,
  28,28,28,28,29,30,30,31,33,34,36,38,40,43,46,49,53,57,62,67,73,79,86,93,101,110,119,128,138,148,159,170
};

Int g_quantInterDefault32x32[1024] =
{
  16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,17,17,17,18,18,19,19,20,21,22,22,23,24,25,26,28,
  16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,17,17,17,18,18,19,19,20,21,22,22,23,24,25,26,28,28,
  16,16,16,16,16,16,16,16,16,16,16,16,16,16,17,17,17,18,18,19,19,20,21,22,22,23,24,25,26,28,28,28,
  16,16,16,16,16,16,16,16,16,16,16,16,16,17,17,17,18,18,19,19,20,21,22,22,23,24,25,26,28,28,28,28,
  16,16,16,16,16,16,16,16,16,16,16,16,17,17,17,18,18,19,19,20,21,22,22,23,24,25,26,28,28,28,28,29,
  16,16,16,16,16,16,16,16,16,16,16,17,17,17,18,18,19,19,20,21,22,22,23,24,25,26,28,28,28,28,29,30,
  16,16,16,16,16,16,16,16,16,16,17,17,17,18,18,19,19,20,21,22,22,23,24,25,26,28,28,28,28,29,30,30,
  16,16,16,16,16,16,16,16,16,17,17,17,18,18,19,19,20,21,22,22,23,24,25,26,28,28,28,28,29,30,30,31,
  16,16,16,16,16,16,16,16,17,17,17,18,18,19,19,20,21,22,22,23,24,25,26,28,28,28,28,29,30,30,31,32,
  16,16,16,16,16,16,16,17,17,17,18,18,19,19,20,21,22,22,23,24,25,26,28,28,28,28,29,30,30,31,32,33,
  16,16,16,16,16,16,17,17,17,18,18,19,19,20,21,22,22,23,24,25,26,28,28,28,28,29,30,30,31,32,33,34,
  16,16,16,16,16,17,17,17,18,18,19,19,20,21,22,22,23,24,25,26,28,28,28,28,29,30,30,31,32,33,34,35,
  16,16,16,16,17,17,17,18,18,19,19,20,21,22,22,23,24,25,26,28,28,28,28,29,30,30,31,32,33,34,35,36,
  16,16,16,17,17,17,18,18,19,19,20,21,22,22,23,24,25,26,28,28,28,28,29,30,30,31,32,33,34,35,36,38,
  16,16,17,17,17,18,18,19,19,20,21,22,22,23,24,25,26,28,28,28,28,29,30,30,31,32,33,34,35,36,38,40,
  16,17,17,17,18,18,19,19,20,21,22,22,23,24,25,26,28,28,28,28,29,30,30,31,32,33,34,35,36,38,40,42,
  17,17,17,18,18,19,19,20,21,22,22,23,24,25,26,28,28,28,28,29,30,30,31,32,33,34,35,36,38,40,42,45,
  17,17,18,18,19,19,20,21,22,22,23,24,25,26,28,28,28,28,29,30,30,31,32,33,34,35,36,38,40,42,45,48,
  17,18,18,19,19,20,21,22,22,23,24,25,26,28,28,28,28,29,30,30,31,32,33,34,35,36,38,40,42,45,48,52,
  18,18,19,19,20,21,22,22,23,24,25,26,28,28,28,28,29,30,30,31,32,33,34,35,36,38,40,42,45,48,52,56,
  18,19,19,20,21,22,22,23,24,25,26,28,28,28,28,29,30,30,31,32,33,34,35,36,38,40,42,45,48,52,56,60,
  19,19,20,21,22,22,23,24,25,26,28,28,28,28,29,30,30,31,32,33,34,35,36,38,40,42,45,48,52,56,60,64,
  19,20,21,22,22,23,24,25,26,28,28,28,28,29,30,30,31,32,33,34,35,36,38,40,42,45,48,52,56,60,64,69,
  20,21,22,22,23,24,25,26,28,28,28,28,29,30,30,31,32,33,34,35,36,38,40,42,45,48,52,56,60,64,69,74,
  21,22,22,23,24,25,26,28,28,28,28,29,30,30,31,32,33,34,35,36,38,40,42,45,48,52,56,60,64,69,74,80,
  22,22,23,24,25,26,28,28,28,28,29,30,30,31,32,33,34,35,36,38,40,42,45,48,52,56,60,64,69,74,80,87,
  22,23,24,25,26,28,28,28,28,29,30,30,31,32,33,34,35,36,38,40,42,45,48,52,56,60,64,69,74,80,87,94,
  23,24,25,26,28,28,28,28,29,30,30,31,32,33,34,35,36,38,40,42,45,48,52,56,60,64,69,74,80,87,94,101,
  24,25,26,28,28,28,28,29,30,30,31,32,33,34,35,36,38,40,42,45,48,52,56,60,64,69,74,80,87,94,101,108,
  25,26,28,28,28,28,29,30,30,31,32,33,34,35,36,38,40,42,45,48,52,56,60,64,69,74,80,87,94,101,108,115,
  26,28,28,28,28,29,30,30,31,32,33,34,35,36,38,40,42,45,48,52,56,60,64,69,74,80,87,94,101,108,115,123,
  28,28,28,28,29,30,30,31,32,33,34,35,36,38,40,42,45,48,52,56,60,64,69,74,80,87,94,101,108,115,123,131
};
#endif
UInt g_scalingListSize   [4] = {16,64,256,1024}; 
UInt g_scalingListSizeX  [4] = { 4, 8, 16,  32};
UInt g_scalingListNum[SCALING_LIST_SIZE_NUM]={6,6,6,2};
Int  g_eTTable[4] = {0,3,1,2};

#if HHI_DMM_WEDGE_INTRA || HHI_DMM_PRED_TEX
std::vector< std::vector<TComWedgelet> > g_aacWedgeLists;
std::vector< std::vector<TComWedgeRef> > g_aacWedgeRefLists;

#if HHIQC_DMMFASTSEARCH_B0039
std::vector< std::vector< std::vector<UInt> > > g_aauiWdgLstM3;
std::vector< std::vector< TComWedgeNode> >      g_aacWedgeNodeLists;
#endif

Void initWedgeLists()
{
  for( UInt ui = g_aucConvertToBit[DMM_WEDGEMODEL_MIN_SIZE]; ui < (g_aucConvertToBit[DMM_WEDGEMODEL_MAX_SIZE]+1); ui++ )
  {
    UInt uiWedgeBlockSize = ((UInt)DMM_WEDGEMODEL_MIN_SIZE)<<ui;
    std::vector<TComWedgelet> acWedgeList;
    std::vector<TComWedgeRef> acWedgeRefList;
    createWedgeList( uiWedgeBlockSize, uiWedgeBlockSize, acWedgeList, acWedgeRefList, g_aeWedgeResolutionList[ui] );
    g_aacWedgeLists.push_back( acWedgeList );
    g_aacWedgeRefLists.push_back( acWedgeRefList );

#if HHIQC_DMMFASTSEARCH_B0039
    // create WedgeNodeList
    std::vector<TComWedgeNode> acWedgeNodeList;
    for( UInt uiPos = 0; uiPos < acWedgeList.size(); uiPos++ )
    {
      if( acWedgeList[uiPos].getIsCoarse() )
      {
        TComWedgeNode cWedgeNode;
        cWedgeNode.setPatternIdx( uiPos );

        // set refinement idxs
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
                iEx == (Int)acWedgeRefList[k].getEndX() && 
                iEy == (Int)acWedgeRefList[k].getEndY()    )
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
    g_aacWedgeNodeLists.push_back( acWedgeNodeList );
#endif
  }
  return;
}

Void createWedgeList( UInt uiWidth, UInt uiHeight, std::vector<TComWedgelet> &racWedgeList, std::vector<TComWedgeRef> &racWedgeRefList, WedgeResolution eWedgeRes )
{
  assert( uiWidth == uiHeight );

  UChar    uhStartX = 0,    uhStartY = 0,    uhEndX = 0,    uhEndY = 0;
  Int   iStepStartX = 0, iStepStartY = 0, iStepEndX = 0, iStepEndY = 0;

  UInt uiBlockSize = 0;
  switch( eWedgeRes )
  {
  case( DOUBLE_PEL ): { uiBlockSize = (uiWidth>>1); break; }
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

    for( Int iK = 0; iK < uiBlockSize; iK++ )
    {
      for( Int iL = 0; iL < uiBlockSize; iL++ )
      {
#if HHIQC_DMMFASTSEARCH_B0039
        cTempWedgelet.setWedgelet( uhStartX + (iK*iStepStartX) , uhStartY + (iK*iStepStartY), uhEndX + (iL*iStepEndX), uhEndY + (iL*iStepEndY), (UChar)uiOri, eWedgeRes, ((iL%2)==0 && (iK%2)==0) );
#else
        cTempWedgelet.setWedgelet( uhStartX + (iK*iStepStartX) , uhStartY + (iK*iStepStartY), uhEndX + (iL*iStepEndX), uhEndY + (iL*iStepEndY), (UChar)uiOri, eWedgeRes );
#endif
        addWedgeletToList( cTempWedgelet, racWedgeList, racWedgeRefList );
      }
    }
  }
#if HHIQC_DMMFASTSEARCH_B0039
  UInt uiThrSz = DMM3_SIMPLIFY_TR;

  std::vector< std::vector<UInt> > auiWdgListSz;
  for( Int idxM=2; idxM<=34 ; idxM++)
  {
    std::vector<UInt> auiWdgList;
    for( Int idxW=0; idxW<racWedgeList.size(); idxW++)
    {
      UInt uiAbsDiff = abs(idxM-(Int)racWedgeList[idxW].getAng());
      if( uiAbsDiff <= uiThrSz )
      {
        auiWdgList.push_back(idxW);
      }
    }
    auiWdgListSz.push_back(auiWdgList);
  }
  g_aauiWdgLstM3.push_back(auiWdgListSz);
#endif
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
#if HHIQC_DMMFASTSEARCH_B0039
  cWedgelet.findClosetAngle();
#endif
    racWedgeList.push_back( cWedgelet );
    TComWedgeRef cWedgeRef;
    cWedgeRef.setWedgeRef( cWedgelet.getStartX(), cWedgelet.getStartY(), cWedgelet.getEndX(), cWedgelet.getEndY(), (UInt)(racWedgeList.size()-1) );
    racWedgeRefList.push_back( cWedgeRef );
  }
}
#endif

//! \}
