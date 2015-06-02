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
/** \file     TComRom.h
    \brief    global variables & functions (header)
*/
#ifndef __TCOMROM__
#define __TCOMROM__
#include "CommonDef.h"
#include<stdio.h>
#include<iostream>
#if NH_3D_DMM
#include "TComWedgelet.h"
#endif
//! \ingroup TLibCommon
//! \{
// ====================================================================================================================
// Initialize / destroy functions
// ====================================================================================================================
Void         initROM();
Void         destroyROM();
// ====================================================================================================================
// Data structure related table & variable
// ====================================================================================================================
// flexible conversion from relative to absolute index
extern       UInt   g_auiZscanToRaster[ MAX_NUM_PART_IDXS_IN_CTU_WIDTH*MAX_NUM_PART_IDXS_IN_CTU_WIDTH ];
extern       UInt   g_auiRasterToZscan[ MAX_NUM_PART_IDXS_IN_CTU_WIDTH*MAX_NUM_PART_IDXS_IN_CTU_WIDTH ];
extern       UInt*  g_scanOrder[SCAN_NUMBER_OF_GROUP_TYPES][SCAN_NUMBER_OF_TYPES][ MAX_CU_DEPTH ][ MAX_CU_DEPTH ];
Void         initZscanToRaster ( Int iMaxDepth, Int iDepth, UInt uiStartVal, UInt*& rpuiCurrIdx );
Void         initRasterToZscan ( UInt uiMaxCUWidth, UInt uiMaxCUHeight, UInt uiMaxDepth         );
// conversion of partition index to picture pel position
extern       UInt   g_auiRasterToPelX[ MAX_NUM_PART_IDXS_IN_CTU_WIDTH*MAX_NUM_PART_IDXS_IN_CTU_WIDTH ];
extern       UInt   g_auiRasterToPelY[ MAX_NUM_PART_IDXS_IN_CTU_WIDTH*MAX_NUM_PART_IDXS_IN_CTU_WIDTH ];
Void         initRasterToPelXY ( UInt uiMaxCUWidth, UInt uiMaxCUHeight, UInt uiMaxDepth );
extern const UInt g_auiPUOffset[NUMBER_OF_PART_SIZES];
extern const Int g_quantScales[SCALING_LIST_REM_NUM];             // Q(QP%6)
extern const Int g_invQuantScales[SCALING_LIST_REM_NUM];          // IQ(QP%6)
#if RExt__HIGH_PRECISION_FORWARD_TRANSFORM
static const Int g_transformMatrixShift[TRANSFORM_NUMBER_OF_DIRECTIONS] = { 14, 6 };
#else
static const Int g_transformMatrixShift[TRANSFORM_NUMBER_OF_DIRECTIONS] = {  6, 6 };
#endif
extern const TMatrixCoeff g_aiT4 [TRANSFORM_NUMBER_OF_DIRECTIONS][4][4];
extern const TMatrixCoeff g_aiT8 [TRANSFORM_NUMBER_OF_DIRECTIONS][8][8];
extern const TMatrixCoeff g_aiT16[TRANSFORM_NUMBER_OF_DIRECTIONS][16][16];
extern const TMatrixCoeff g_aiT32[TRANSFORM_NUMBER_OF_DIRECTIONS][32][32];
// ====================================================================================================================
// Luma QP to Chroma QP mapping
// ====================================================================================================================
static const Int chromaQPMappingTableSize = 58;
extern const UChar  g_aucChromaScale[NUM_CHROMA_FORMAT][chromaQPMappingTableSize];
// ====================================================================================================================
// Scanning order & context mapping table
// ====================================================================================================================
extern const UInt   ctxIndMap4x4[4*4];
extern const UInt   g_uiGroupIdx[ MAX_TU_SIZE ];
extern const UInt   g_uiMinInGroup[ LAST_SIGNIFICANT_GROUPS ];
// ====================================================================================================================
// Intra prediction table
// ====================================================================================================================
extern const UChar  g_aucIntraModeNumFast_UseMPM[MAX_CU_DEPTH];
extern const UChar  g_aucIntraModeNumFast_NotUseMPM[MAX_CU_DEPTH];
extern const UChar  g_chroma422IntraAngleMappingTable[NUM_INTRA_MODE];
// ====================================================================================================================
// Depth coding modes
// ====================================================================================================================
#if NH_3D_DMM
extern const WedgeResolution                                 g_dmmWedgeResolution [6];
extern const UChar                                           g_dmm1TabIdxBits     [6];
extern Bool                                                  g_wedgePattern[32*32];
extern       std::vector< std::vector<TComWedgelet> >        g_dmmWedgeLists;
extern       std::vector< std::vector<TComWedgeNode> >       g_dmmWedgeNodeLists;
Void initWedgeLists( Bool initNodeList = false );
Void createWedgeList( UInt uiWidth, UInt uiHeight, std::vector<TComWedgelet> &racWedgeList, std::vector<TComWedgeRef> &racWedgeRefList, WedgeResolution eWedgeRes );
Void addWedgeletToList( TComWedgelet cWedgelet, std::vector<TComWedgelet> &racWedgeList, std::vector<TComWedgeRef> &racWedgeRefList );
WedgeList*     getWedgeListScaled    ( UInt blkSize );
WedgeNodeList* getWedgeNodeListScaled( UInt blkSize );
__inline Void mapDmmToIntraDir( UInt& intraMode ) { if( isDmmMode( intraMode ) ) intraMode = DC_IDX; }
__inline Void mapDmmToIntraDir(  Int& intraMode ) { if( isDmmMode( intraMode ) ) intraMode = DC_IDX; }
#endif
// ====================================================================================================================
// Mode-Dependent DST Matrices
// ====================================================================================================================
extern const TMatrixCoeff g_as_DST_MAT_4 [TRANSFORM_NUMBER_OF_DIRECTIONS][4][4];
#if H_MV_HLS_PTL_LIMITS
class TComGeneralTierAndLevelLimits
{
public:
  TComGeneralTierAndLevelLimits::TComGeneralTierAndLevelLimits
  ( Int maxLumaPs, 
    Int maxCPBMainTier, 
    Int maxCPBHighTier, 
    Int maxSliceSegmentsPerPicture, 
    Int maxTileRows, 
    Int maxTileCols )
  : m_maxLumaPs                 ( maxLumaPs                     ),
    m_maxCPBMainTier            ( maxCPBMainTier                ),
    m_maxCPBHighTier            ( maxCPBHighTier                ),
    m_maxSliceSegmentsPerPicture( maxSliceSegmentsPerPicture    ),
    m_maxTileRows               ( maxTileRows                   ),
    m_maxTileCols               ( maxTileCols                   );
  {};
  Int getMaxLumaPs                 ( ) { return m_maxLumaPs                 ; };
  Int getMaxCPBMainTier            ( ) { return m_maxCPBMainTier            ; };
  Int getMaxCPBHighTier            ( ) { return m_maxCPBHighTier            ; };
  Int getMaxSliceSegmentsPerPicture( ) { return m_maxSliceSegmentsPerPicture; };
  Int getMaxTileRows               ( ) { return m_maxTileRows               ; };
  Int getMaxTileCols               ( ) { return m_maxTileCols               ; };
private:
  const Int m_maxLumaPs;
  const Int m_maxCPBMainTier; 
  const Int m_maxCPBHighTier;
  const Int m_maxSliceSegmentsPerPicture; 
  const Int m_maxTileRows; 
  const Int m_maxTileCols; 
};
extern std::map< Level::Name, TComGeneralTierAndLevelLimits > g_generalTierAndLevelLimits;   
#endif
// ====================================================================================================================
// Misc.
// ====================================================================================================================
extern       Char   g_aucConvertToBit  [ MAX_CU_SIZE+1 ];   // from width to log2(width)-2
#if NH_MV
// Change later
#ifndef ENC_DEC_TRACE
#define ENC_DEC_TRACE 0
#endif
#endif
#if ENC_DEC_TRACE
extern FILE*  g_hTrace;
extern Bool   g_bJustDoIt;
extern const Bool g_bEncDecTraceEnable;
extern const Bool g_bEncDecTraceDisable;
extern Bool   g_HLSTraceEnable;
extern UInt64 g_nSymbolCounter;
#define COUNTER_START    1
#define COUNTER_END      0 //( UInt64(1) << 63 )
#define DTRACE_CABAC_F(x)     if ( ( g_nSymbolCounter >= COUNTER_START && g_nSymbolCounter <= COUNTER_END )|| g_bJustDoIt ) fprintf( g_hTrace, "%f", x );
#define DTRACE_CABAC_V(x)     if ( ( g_nSymbolCounter >= COUNTER_START && g_nSymbolCounter <= COUNTER_END )|| g_bJustDoIt ) fprintf( g_hTrace, "%d", x );
#define DTRACE_CABAC_VL(x)    if ( ( g_nSymbolCounter >= COUNTER_START && g_nSymbolCounter <= COUNTER_END )|| g_bJustDoIt ) fprintf( g_hTrace, "%lld", x );
#define DTRACE_CABAC_T(x)     if ( ( g_nSymbolCounter >= COUNTER_START && g_nSymbolCounter <= COUNTER_END )|| g_bJustDoIt ) fprintf( g_hTrace, "%s", x );
#define DTRACE_CABAC_X(x)     if ( ( g_nSymbolCounter >= COUNTER_START && g_nSymbolCounter <= COUNTER_END )|| g_bJustDoIt ) fprintf( g_hTrace, "%x", x );
#define DTRACE_CABAC_R( x,y ) if ( ( g_nSymbolCounter >= COUNTER_START && g_nSymbolCounter <= COUNTER_END )|| g_bJustDoIt ) fprintf( g_hTrace, x,    y );
#define DTRACE_CABAC_N        if ( ( g_nSymbolCounter >= COUNTER_START && g_nSymbolCounter <= COUNTER_END )|| g_bJustDoIt ) fprintf( g_hTrace, "\n"    );
#if H_MV_ENC_DEC_TRAC
 extern Bool   g_traceCU; 
 extern Bool   g_tracePU ; 
 extern Bool   g_traceTU; 
 extern Bool   g_disableHLSTrace;       // USE g_HLSTraceEnable to toggle HLS trace. Not this one!
 extern Bool   g_disableNumbering;      // Don't print numbers to trace file
 extern UInt64 g_stopAtCounter;         // Counter to set breakpoint. 
 extern Bool   g_traceCopyBack;         // Output samples on copy back  
 extern Bool   g_decTraceDispDer;       // Trace derived disparity vectors (decoder only) 
 extern Bool   g_decTraceMvFromMerge;   // Trace motion vectors obtained from merge (decoder only) 
 extern Bool   g_decTracePicOutput;     // Trace output of pictures
 extern Bool   g_stopAtPos;             // Stop at position
 extern Bool   g_outputPos;             // Output position
 extern Bool   g_traceCameraParameters; // Trace camera parameters
 extern Bool   g_encNumberOfWrittenBits;// Trace number of written bits
#define DTRACE_CU(x,y)             writeToTraceFile( x,y, g_traceCU );
#define DTRACE_PU(x,y)             writeToTraceFile( x,y, g_tracePU );
#define DTRACE_TU(x,y)             writeToTraceFile( x,y, g_traceTU );
#define DTRACE_CU_S(x)             writeToTraceFile( x,   g_traceCU );
#define DTRACE_PU_S(x)             writeToTraceFile( x,   g_tracePU );
#define DTRACE_TU_S(x)             writeToTraceFile( x,   g_traceTU );
 Void           tracePSHeader   ( const Char* psName, Int layerId ); 
 Void           writeToTraceFile( const Char* symbolName, Int val, Bool doIt );
 Void           writeToTraceFile( const Char* symbolName, Bool doIt );
 Void           stopAtPos       ( Int poc, Int layerId, Int cuPelX, Int cuPelY, Int cuWidth, Int cuHeight );           
#endif
#else
#define DTRACE_CABAC_F(x)
#define DTRACE_CABAC_V(x)
#define DTRACE_CABAC_VL(x)
#define DTRACE_CABAC_T(x)
#define DTRACE_CABAC_X(x)
#define DTRACE_CABAC_R( x,y )
#define DTRACE_CABAC_N
#if H_MV_ENC_DEC_TRAC
#define DTRACE_CU(x,y) ;             
#define DTRACE_PU(x,y) ;            
#define DTRACE_TU(x,y) ;            
#define DTRACE_CU_S(x) ;            
#define DTRACE_PU_S(x) ;            
#define DTRACE_TU_S(x) ;            
#endif
#endif
const Char* nalUnitTypeToString(NalUnitType type);
extern const Char *MatrixType[SCALING_LIST_SIZE_NUM][SCALING_LIST_NUM];
extern const Char *MatrixType_DC[SCALING_LIST_SIZE_NUM][SCALING_LIST_NUM];
extern const Int g_quantTSDefault4x4[4*4];
extern const Int g_quantIntraDefault8x8[8*8];
extern const Int g_quantInterDefault8x8[8*8];
extern const UInt g_scalingListSize [SCALING_LIST_SIZE_NUM];
extern const UInt g_scalingListSizeX[SCALING_LIST_SIZE_NUM];
//! \}
#endif  //__TCOMROM__
