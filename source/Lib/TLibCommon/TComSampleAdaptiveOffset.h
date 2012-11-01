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

/** \file     TComSampleAdaptiveOffset.h
    \brief    sample adaptive offset class (header)
*/

#ifndef __TCOMSAMPLEADAPTIVEOFFSET__
#define __TCOMSAMPLEADAPTIVEOFFSET__

#include "CommonDef.h"
#include "TComPic.h"

//! \ingroup TLibCommon
//! \{

// ====================================================================================================================
// Constants
// ====================================================================================================================

#define SAO_MAX_DEPTH                 4
#define SAO_BO_BITS                   5
#define LUMA_GROUP_NUM                (1<<SAO_BO_BITS)
#if SAO_UNIT_INTERLEAVING
#define MAX_NUM_SAO_OFFSETS           4
#define MAX_NUM_SAO_CLASS             33
#else
#define MAX_NUM_SAO_CLASS             32
#endif
// ====================================================================================================================
// Class definition
// ====================================================================================================================

/// Sample Adaptive Offset class
class TComSampleAdaptiveOffset
{
protected:
  TComPic*          m_pcPic;

  static UInt m_uiMaxDepth;
  static const Int m_aiNumPartsInRow[5];
  static const Int m_aiNumPartsLevel[5];
  static const Int m_aiNumCulPartsLevel[5];
  static const UInt m_auiEoTable[9];
  static const UInt m_auiEoTable2D[9];
  static const UInt m_iWeightSao[MAX_NUM_SAO_TYPE];
  Int *m_iOffsetBo;
  Int m_iOffsetEo[LUMA_GROUP_NUM];

  Int  m_iPicWidth;
  Int  m_iPicHeight;
  UInt m_uiMaxSplitLevel;
  UInt m_uiMaxCUWidth;
  UInt m_uiMaxCUHeight;
  Int  m_iNumCuInWidth;
  Int  m_iNumCuInHeight;
  Int  m_iNumTotalParts;
  static Int m_iNumClass[MAX_NUM_SAO_TYPE];
  SliceType  m_eSliceType;
  Int        m_iPicNalReferenceIdc;

  UInt m_uiSaoBitIncrease;
  UInt m_uiQP;

  Pel   *m_pClipTable;
  Pel   *m_pClipTableBase;
#if SAO_UNIT_INTERLEAVING
  Pel   *m_lumaTableBo;
#else
  Pel   *m_ppLumaTableBo0;
  Pel   *m_ppLumaTableBo1;
#endif
  Int   *m_iUpBuff1;
  Int   *m_iUpBuff2;
  Int   *m_iUpBufft;
  Int   *ipSwap;
  Bool  m_bUseNIF;       //!< true for performing non-cross slice boundary ALF
  UInt  m_uiNumSlicesInPic;      //!< number of slices in picture
  Int   m_iSGDepth;              //!< slice granularity depth
  TComPicYuv* m_pcYuvTmp;    //!< temporary picture buffer pointer when non-across slice/tile boundary SAO is enabled

  Pel* m_pTmpU1;
  Pel* m_pTmpU2;
  Pel* m_pTmpL1;
  Pel* m_pTmpL2;
  Int* m_iLcuPartIdx;
#if SAO_UNIT_INTERLEAVING
  Int     m_maxNumOffsetsPerPic;
  Bool    m_saoInterleavingFlag;
#else
  Void initTmpSaoQuadTree(SAOQTPart *psQTPart, Int iYCbCr);
  Void disableSaoOnePart(SAOQTPart *psQTPart, UInt uiPartIdx, Int iYCbCr);
  Void xSaoQt2Lcu(SAOQTPart *psQTPart,UInt uiPartIdx);
  Void convertSaoQt2Lcu(SAOQTPart *psQTPart,UInt uiPartIdx);
  Void xSaoAllPart(SAOQTPart *psQTPart, Int iYCbCr);
#endif
public:
  TComSampleAdaptiveOffset         ();
  virtual ~TComSampleAdaptiveOffset();

  Void create( UInt uiSourceWidth, UInt uiSourceHeight, UInt uiMaxCUWidth, UInt uiMaxCUHeight, UInt uiMaxCUDepth );
  Void destroy ();

  Int  convertLevelRowCol2Idx(int level, int row, int col);
  void convertIdx2LevelRowCol(int idx, int *level, int *row, int *col);

  Void initSAOParam   (SAOParam *pcSaoParam, Int iPartLevel, Int iPartRow, Int iPartCol, Int iParentPartIdx, Int StartCUX, Int EndCUX, Int StartCUY, Int EndCUY, Int iYCbCr);
  Void allocSaoParam  (SAOParam* pcSaoParam);
  Void resetSAOParam  (SAOParam *pcSaoParam);
  Void freeSaoParam   (SAOParam *pcSaoParam);

  Void SAOProcess(TComPic* pcPic, SAOParam* pcSaoParam);
  Void processSaoCu(Int iAddr, Int iSaoType, Int iYCbCr);
  Void processSaoOnePart(SAOQTPart *psQTPart, UInt uiPartIdx, Int iYCbCr);
  Void processSaoQuadTree(SAOQTPart *psQTPart, UInt uiPartIdx, Int iYCbCr);
  Pel* getPicYuvAddr(TComPicYuv* pcPicYuv, Int iYCbCr,Int iAddr = 0);

  Void processSaoCuOrg(Int iAddr, Int iPartIdx, Int iYCbCr);  //!< LCU-basd SAO process without slice granularity 
  Void createPicSaoInfo(TComPic* pcPic, Int numSlicesInPic = 1);
  Void destroyPicSaoInfo();
  Void processSaoBlock(Pel* pDec, Pel* pRest, Int stride, Int iSaoType, UInt xPos, UInt yPos, UInt width, UInt height, Bool* pbBorderAvail);

#if SAO_UNIT_INTERLEAVING
  Void resetLcuPart(SaoLcuParam* saoLcuParam);
  Void convertQT2SaoUnit(SAOParam* saoParam, UInt partIdx, Int yCbCr);
  Void convertOnePart2SaoUnit(SAOParam *saoParam, UInt partIdx, Int yCbCr);
  Void processSaoUnitAll(SaoLcuParam* saoLcuParam, Bool oneUnitFlag, Int yCbCr);
  Void setSaoInterleavingFlag (Bool bVal)  {m_saoInterleavingFlag = bVal;}
  Bool getSaoInterleavingFlag ()           {return m_saoInterleavingFlag;}
#endif
};

//! \}
#endif

