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

/** \file     TComDataCU.cpp
    \brief    CU data structure
    \todo     not all entities are documented
*/

#include "TComDataCU.h"
#include "TComPic.h"
#include "TComDepthMapGenerator.h"
#include "TComResidualGenerator.h"

//! \ingroup TLibCommon
//! \{

#if ADAPTIVE_QP_SELECTION
Int * TComDataCU::m_pcGlbArlCoeffY  = NULL;
Int * TComDataCU::m_pcGlbArlCoeffCb = NULL;
Int * TComDataCU::m_pcGlbArlCoeffCr = NULL;
#endif

#if MERL_VSP_C0152

#define CHECK_ADD_YET(pcCURef,uiIdx,vspIdx) && !( (pcCURef)->getVSPIndex(uiIdx) == vspIdx && bVspMvZeroDone[vspIdx-1] )

inline Void TComDataCU::xInheritVspMode( TComDataCU* pcCURef, UInt uiIdx, Bool* bVspMvZeroDone, Int iCount, Int* iVSPIndexTrue, TComMvField* pcMvFieldNeighbours, DisInfo* pDInfo )
{
  Int vspIdx = (Int) pcCURef->getVSPIndex(uiIdx);
  if( vspIdx != 0 )
  {
    Int idx = vspIdx - 1;
    bVspMvZeroDone[idx] = true;
    iVSPIndexTrue [idx] = iCount;

    // no need to reset Inter Dir

    // set MV using checked disparity
    if (vspIdx < 4)
    {
      pcMvFieldNeighbours[ iCount<<1].setMvField ( pDInfo->m_acMvCand[0],  NOT_VALID );
      if ( pcCURef->getSlice()->isInterB() )
      {
         pcMvFieldNeighbours[(iCount<<1)+1 ].setMvField ( pDInfo->m_acMvCand[0],  NOT_VALID );
      }
    }
  }
}

inline Bool TComDataCU::xAddVspMergeCand( UChar ucVspMergePos, Int vspIdx, Bool* bVspMvZeroDone, UInt uiDepth, Bool* abCandIsInter, Int& iCount,
                                          UChar* puhInterDirNeighbours, TComMvField* pcMvFieldNeighbours, Int* iVSPIndexTrue, Int mrgCandIdx, DisInfo* pDInfo )
{
#if MERL_VSP_C0152_BugFix_ForNoDepthCase
  TComPic* pRefPicBaseDepth = NULL;
  pRefPicBaseDepth = getSlice()->getRefPicBaseDepth();
  if(ucVspMergePos == VSP_MERGE_POS && pRefPicBaseDepth) //VSP can be used only when depth is used as input
#else
  if( ucVspMergePos == VSP_MERGE_POS )
#endif
  if( ucVspMergePos == VSP_MERGE_POS )
  {
    Int idx = vspIdx - 1;
    {
      if( getSlice()->getSPS()->getViewId() != 0 && bVspMvZeroDone[idx] == false )
      {
        {
          abCandIsInter [iCount] = true;
          bVspMvZeroDone[idx] = true;

          // get Inter Dir
          Int iInterDir = ((getSlice()->getNumRefIdx(REF_PIC_LIST_0) > 0 && getSlice()->getNumRefIdx(REF_PIC_LIST_1) > 0) ? 3 :
            (getSlice()->getNumRefIdx(REF_PIC_LIST_0) > 0 ? 1 : 2));
          puhInterDirNeighbours[iCount] = iInterDir; // The direction information does not matter
          // get Mv using checked disparity vector
          if (vspIdx < 4) // spatial
          {
            pcMvFieldNeighbours[iCount<<1].setMvField(pDInfo->m_acMvCand[0], NOT_VALID );
            if ( getSlice()->isInterB() )
            {
              pcMvFieldNeighbours[(iCount<<1)+1].setMvField( pDInfo->m_acMvCand[0], NOT_VALID );
            }
          }
          iVSPIndexTrue[idx] = iCount;
          if ( mrgCandIdx == iCount )
          {
            return false;
          }
          iCount ++;
        }
      }
    }
  }
  return true;
}
#endif

// ====================================================================================================================
// Constructor / destructor / create / destroy
// ====================================================================================================================

TComDataCU::TComDataCU()
{
  m_pcPic              = NULL;
  m_pcSlice            = NULL;
  m_puhDepth           = NULL;
#if HHI_MPI
  m_piTextureModeDepth = NULL;
#endif
  
  m_pePartSize         = NULL;
#if HHI_INTERVIEW_SKIP
  m_pbRenderable       = NULL;
#endif
  m_pePredMode         = NULL;
  m_puiAlfCtrlFlag     = NULL;
  m_puiTmpAlfCtrlFlag  = NULL;
  m_puhWidth           = NULL;
  m_puhHeight          = NULL;
  m_phQP               = NULL;
  m_pbMergeFlag        = NULL;
#if LGE_ILLUCOMP_B0045
  m_pbICFlag           = NULL;
#endif
  m_puhMergeIndex      = NULL;
#if MERL_VSP_C0152
  m_piVSPIndex         = NULL;
#endif
  m_puhLumaIntraDir    = NULL;
  m_puhChromaIntraDir  = NULL;
  m_puhInterDir        = NULL;
  m_puhTrIdx           = NULL;
#if NSQT_MOD2
  m_nsqtPartIdx        = NULL;
#endif
  m_puhCbf[0]          = NULL;
  m_puhCbf[1]          = NULL;
  m_puhCbf[2]          = NULL;
  m_pcTrCoeffY         = NULL;
  m_pcTrCoeffCb        = NULL;
  m_pcTrCoeffCr        = NULL;
#if ADAPTIVE_QP_SELECTION  
  m_ArlCoeffIsAliasedAllocation = false;
  m_pcArlCoeffY        = NULL;
  m_pcArlCoeffCb       = NULL;
  m_pcArlCoeffCr       = NULL;
#endif
  
  m_pbIPCMFlag         = NULL;
  m_pcIPCMSampleY      = NULL;
  m_pcIPCMSampleCb     = NULL;
  m_pcIPCMSampleCr     = NULL;

  m_pcPattern          = NULL;
  
  m_pcCUAboveLeft      = NULL;
  m_pcCUAboveRight     = NULL;
  m_pcCUAbove          = NULL;
  m_pcCULeft           = NULL;
  
  m_apcCUColocated[0]  = NULL;
  m_apcCUColocated[1]  = NULL;
  
  m_apiMVPIdx[0]       = NULL;
  m_apiMVPIdx[1]       = NULL;
  m_apiMVPNum[0]       = NULL;
  m_apiMVPNum[1]       = NULL;
  
  m_bDecSubCu          = false;
  m_uiSliceStartCU        = 0;
  m_uiEntropySliceStartCU = 0;

#if HHI_DMM_WEDGE_INTRA
  m_puiWedgeFullTabIdx       = NULL;
  m_piWedgeFullDeltaDC1      = NULL;
  m_piWedgeFullDeltaDC2      = NULL;

  m_puiWedgePredDirTabIdx    = NULL;
  m_piWedgePredDirDeltaDC1   = NULL;
  m_piWedgePredDirDeltaDC2   = NULL;
  m_piWedgePredDirDeltaEnd   = NULL;
#endif
#if HHI_DMM_PRED_TEX
  m_puiWedgePredTexTabIdx    = NULL;
#if LGE_DMM3_SIMP_C0044
  m_puiWedgePredTexIntraTabIdx = NULL;
#endif
  m_piWedgePredTexDeltaDC1   = NULL;
  m_piWedgePredTexDeltaDC2   = NULL;

  m_piContourPredTexDeltaDC1 = NULL;
  m_piContourPredTexDeltaDC2 = NULL;
#endif
#if H3D_IVRP
  m_pbResPredAvailable = NULL;
  m_pbResPredFlag      = NULL;
#endif
#if LGE_EDGE_INTRA_A0070
  m_pucEdgeCode         = NULL;
  m_pucEdgeNumber       = NULL;
  m_pucEdgeStartPos     = NULL;
  m_pbEdgeLeftFirst     = NULL;
  m_pbEdgePartition     = NULL;
#if LGE_EDGE_INTRA_DELTA_DC
  m_piEdgeDeltaDC0      = NULL;
  m_piEdgeDeltaDC1      = NULL;
#endif
#endif
#if RWTH_SDC_DLT_B0036
  m_pbSDCFlag           = NULL;
  m_apSegmentDCOffset[0] = NULL;
  m_apSegmentDCOffset[1] = NULL;
#endif
}

TComDataCU::~TComDataCU()
{
}

Void TComDataCU::create(UInt uiNumPartition, UInt uiWidth, UInt uiHeight, Bool bDecSubCu, Int unitSize
#if ADAPTIVE_QP_SELECTION
                        , Bool bGlobalRMARLBuffer
#endif                                              
                        )
{
  m_bDecSubCu = bDecSubCu;
  
  m_pcPic              = NULL;
  m_pcSlice            = NULL;
  m_uiNumPartition     = uiNumPartition;
  m_unitSize = unitSize;
  
  if ( !bDecSubCu )
  {
    m_phQP               = (Char*     )xMalloc(Char,     uiNumPartition);
    m_puhDepth           = (UChar*    )xMalloc(UChar,    uiNumPartition);
#if HHI_MPI
    m_piTextureModeDepth = (Int*      )xMalloc(Int,      uiNumPartition);
#endif
    m_puhWidth           = (UChar*    )xMalloc(UChar,    uiNumPartition);
    m_puhHeight          = (UChar*    )xMalloc(UChar,    uiNumPartition);
    m_pePartSize         = new Char[ uiNumPartition ];
#if HHI_INTERVIEW_SKIP
    m_pbRenderable        = (Bool*  )xMalloc(Bool,   uiNumPartition);
#endif
    memset( m_pePartSize, SIZE_NONE,uiNumPartition * sizeof( *m_pePartSize ) );
    m_pePredMode         = new Char[ uiNumPartition ];
    
#if RWTH_SDC_DLT_B0036
    m_pbSDCFlag          = (Bool*  )xMalloc(Bool,   uiNumPartition);
    m_apSegmentDCOffset[0] = (Pel*)xMalloc(Pel, uiNumPartition);
    m_apSegmentDCOffset[1] = (Pel*)xMalloc(Pel, uiNumPartition);
#endif
    
    m_puiAlfCtrlFlag     = new Bool[ uiNumPartition ];
    
    m_pbMergeFlag        = (Bool*  )xMalloc(Bool,   uiNumPartition);
#if LGE_ILLUCOMP_B0045
    m_pbICFlag           = (Bool*  )xMalloc(Bool,   uiNumPartition);
#endif
    m_puhMergeIndex      = (UChar* )xMalloc(UChar,  uiNumPartition);
#if MERL_VSP_C0152
    m_piVSPIndex         = (Char*  )xMalloc(Char,   uiNumPartition);
#endif
#if H3D_IVRP
    m_pbResPredAvailable = (Bool*  )xMalloc(Bool,   uiNumPartition);
    m_pbResPredFlag      = (Bool*  )xMalloc(Bool,   uiNumPartition);
#endif
    m_puhLumaIntraDir    = (UChar* )xMalloc(UChar,  uiNumPartition);
    m_puhChromaIntraDir  = (UChar* )xMalloc(UChar,  uiNumPartition);
    m_puhInterDir        = (UChar* )xMalloc(UChar,  uiNumPartition);
    
    m_puhTrIdx           = (UChar* )xMalloc(UChar,  uiNumPartition);
    m_nsqtPartIdx        = (UChar* )xMalloc(UChar,  uiNumPartition);
    
    m_puhCbf[0]          = (UChar* )xMalloc(UChar,  uiNumPartition);
    m_puhCbf[1]          = (UChar* )xMalloc(UChar,  uiNumPartition);
    m_puhCbf[2]          = (UChar* )xMalloc(UChar,  uiNumPartition);
    
    m_apiMVPIdx[0]       = new Char[ uiNumPartition ];
    m_apiMVPIdx[1]       = new Char[ uiNumPartition ];
    m_apiMVPNum[0]       = new Char[ uiNumPartition ];
    m_apiMVPNum[1]       = new Char[ uiNumPartition ];
    memset( m_apiMVPIdx[0], -1,uiNumPartition * sizeof( Char ) );
    memset( m_apiMVPIdx[1], -1,uiNumPartition * sizeof( Char ) );
    
    m_pcTrCoeffY         = (TCoeff*)xMalloc(TCoeff, uiWidth*uiHeight);
    m_pcTrCoeffCb        = (TCoeff*)xMalloc(TCoeff, uiWidth*uiHeight/4);
    m_pcTrCoeffCr        = (TCoeff*)xMalloc(TCoeff, uiWidth*uiHeight/4);
    memset( m_pcTrCoeffY, 0,uiWidth*uiHeight * sizeof( TCoeff ) );
    memset( m_pcTrCoeffCb, 0,uiWidth*uiHeight/4 * sizeof( TCoeff ) );
    memset( m_pcTrCoeffCr, 0,uiWidth*uiHeight/4 * sizeof( TCoeff ) );
#if ADAPTIVE_QP_SELECTION    
    if( bGlobalRMARLBuffer )
    {
      if( m_pcGlbArlCoeffY == NULL )
      {
        m_pcGlbArlCoeffY   = (Int*)xMalloc(Int, uiWidth*uiHeight);
        m_pcGlbArlCoeffCb  = (Int*)xMalloc(Int, uiWidth*uiHeight/4);
        m_pcGlbArlCoeffCr  = (Int*)xMalloc(Int, uiWidth*uiHeight/4);
      }
      m_pcArlCoeffY        = m_pcGlbArlCoeffY;
      m_pcArlCoeffCb       = m_pcGlbArlCoeffCb;
      m_pcArlCoeffCr       = m_pcGlbArlCoeffCr;
      m_ArlCoeffIsAliasedAllocation = true;
    }
    else
    {
      m_pcArlCoeffY        = (Int*)xMalloc(Int, uiWidth*uiHeight);
      m_pcArlCoeffCb       = (Int*)xMalloc(Int, uiWidth*uiHeight/4);
      m_pcArlCoeffCr       = (Int*)xMalloc(Int, uiWidth*uiHeight/4);
    }
#endif
    
    m_pbIPCMFlag         = (Bool*  )xMalloc(Bool, uiNumPartition);
    m_pcIPCMSampleY      = (Pel*   )xMalloc(Pel , uiWidth*uiHeight);
    m_pcIPCMSampleCb     = (Pel*   )xMalloc(Pel , uiWidth*uiHeight/4);
    m_pcIPCMSampleCr     = (Pel*   )xMalloc(Pel , uiWidth*uiHeight/4);

    m_acCUMvField[0].create( uiNumPartition );
    m_acCUMvField[1].create( uiNumPartition );
    
#if HHI_DMM_WEDGE_INTRA
    m_puiWedgeFullTabIdx       = (UInt*)xMalloc(UInt, uiNumPartition);
    m_piWedgeFullDeltaDC1      = (Int* )xMalloc(Int,  uiNumPartition);
    m_piWedgeFullDeltaDC2      = (Int* )xMalloc(Int,  uiNumPartition);

    m_puiWedgePredDirTabIdx    = (UInt*)xMalloc(UInt, uiNumPartition);
    m_piWedgePredDirDeltaDC1   = (Int* )xMalloc(Int,  uiNumPartition);
    m_piWedgePredDirDeltaDC2   = (Int* )xMalloc(Int,  uiNumPartition);
    m_piWedgePredDirDeltaEnd   = (Int* )xMalloc(Int,  uiNumPartition);
#endif
#if HHI_DMM_PRED_TEX
    m_puiWedgePredTexTabIdx    = (UInt*)xMalloc(UInt, uiNumPartition);
#if LGE_DMM3_SIMP_C0044
    m_puiWedgePredTexIntraTabIdx    = (UInt*)xMalloc(UInt, uiNumPartition);
#endif
    m_piWedgePredTexDeltaDC1   = (Int* )xMalloc(Int,  uiNumPartition);
    m_piWedgePredTexDeltaDC2   = (Int* )xMalloc(Int,  uiNumPartition);

    m_piContourPredTexDeltaDC1 = (Int* )xMalloc(Int,  uiNumPartition);
    m_piContourPredTexDeltaDC2 = (Int* )xMalloc(Int,  uiNumPartition);
#endif
#if LGE_EDGE_INTRA_A0070
    m_pucEdgeCode       = (UChar*)xMalloc(UChar, uiNumPartition * LGE_EDGE_INTRA_MAX_EDGE_NUM_PER_4x4);
    m_pucEdgeNumber     = (UChar*)xMalloc(UChar, uiNumPartition);
    m_pucEdgeStartPos   = (UChar*)xMalloc(UChar, uiNumPartition);
    m_pbEdgeLeftFirst   = (Bool*) xMalloc(Bool,  uiNumPartition);
    m_pbEdgePartition   = (Bool*) xMalloc(Bool,  uiNumPartition * 16);
#if LGE_EDGE_INTRA_DELTA_DC
    m_piEdgeDeltaDC0    = (Int* ) xMalloc(Int,   uiNumPartition);
    m_piEdgeDeltaDC1    = (Int* ) xMalloc(Int,   uiNumPartition);
#endif
#endif
  }
  else
  {
    m_acCUMvField[0].setNumPartition(uiNumPartition );
    m_acCUMvField[1].setNumPartition(uiNumPartition );
  }
  
  m_uiSliceStartCU        = (UInt*  )xMalloc(UInt, uiNumPartition);
  m_uiEntropySliceStartCU = (UInt*  )xMalloc(UInt, uiNumPartition);
  
  // create pattern memory
  m_pcPattern            = (TComPattern*)xMalloc(TComPattern, 1);
  
  // create motion vector fields
  
  m_pcCUAboveLeft      = NULL;
  m_pcCUAboveRight     = NULL;
  m_pcCUAbove          = NULL;
  m_pcCULeft           = NULL;
  
  m_apcCUColocated[0]  = NULL;
  m_apcCUColocated[1]  = NULL;
}

Void TComDataCU::destroy()
{
  m_pcPic              = NULL;
  m_pcSlice            = NULL;
  
  if ( m_pcPattern )
  { 
    xFree(m_pcPattern);
    m_pcPattern = NULL;
  }
  
  // encoder-side buffer free
  if ( !m_bDecSubCu )
  {
    if ( m_phQP               ) { xFree(m_phQP);                m_phQP              = NULL; }
    if ( m_puhDepth           ) { xFree(m_puhDepth);            m_puhDepth          = NULL; }
#if HHI_MPI
    if ( m_piTextureModeDepth ) { xFree(m_piTextureModeDepth);  m_piTextureModeDepth= NULL; }
#endif
    if ( m_puhWidth           ) { xFree(m_puhWidth);            m_puhWidth          = NULL; }
    if ( m_puhHeight          ) { xFree(m_puhHeight);           m_puhHeight         = NULL; }
    if ( m_pePartSize         ) { delete[] m_pePartSize;        m_pePartSize        = NULL; }
#if HHI_INTERVIEW_SKIP
    if ( m_pbRenderable        ) { xFree(m_pbRenderable);         m_pbRenderable       = NULL; }
#endif
    if ( m_pePredMode         ) { delete[] m_pePredMode;        m_pePredMode        = NULL; }
#if RWTH_SDC_DLT_B0036
    if ( m_pbSDCFlag          ) { xFree(m_pbSDCFlag);      m_pbSDCFlag    = NULL; }
    if ( m_apSegmentDCOffset[0]          ) { xFree(m_apSegmentDCOffset[0]);      m_apSegmentDCOffset[0]    = NULL; }
    if ( m_apSegmentDCOffset[1]          ) { xFree(m_apSegmentDCOffset[1]);      m_apSegmentDCOffset[1]    = NULL; }
#endif
    if ( m_puhCbf[0]          ) { xFree(m_puhCbf[0]);           m_puhCbf[0]         = NULL; }
    if ( m_puhCbf[1]          ) { xFree(m_puhCbf[1]);           m_puhCbf[1]         = NULL; }
    if ( m_puhCbf[2]          ) { xFree(m_puhCbf[2]);           m_puhCbf[2]         = NULL; }
    if ( m_puiAlfCtrlFlag     ) { delete[] m_puiAlfCtrlFlag;    m_puiAlfCtrlFlag    = NULL; }
    if ( m_puhInterDir        ) { xFree(m_puhInterDir);         m_puhInterDir       = NULL; }
    if ( m_pbMergeFlag        ) { xFree(m_pbMergeFlag);         m_pbMergeFlag       = NULL; }
#if LGE_ILLUCOMP_B0045
    if ( m_pbICFlag           ) { xFree(m_pbICFlag);            m_pbICFlag          = NULL; }
#endif
#if MERL_VSP_C0152
    if ( m_piVSPIndex         ) { xFree(m_piVSPIndex);          m_piVSPIndex        = NULL; }
#endif
    if ( m_puhMergeIndex      ) { xFree(m_puhMergeIndex);       m_puhMergeIndex     = NULL; }
#if H3D_IVRP
    if ( m_pbResPredAvailable ) { xFree(m_pbResPredAvailable);  m_pbResPredAvailable= NULL; }
    if ( m_pbResPredFlag      ) { xFree(m_pbResPredFlag);       m_pbResPredFlag     = NULL; }
#endif
    if ( m_puhLumaIntraDir    ) { xFree(m_puhLumaIntraDir);     m_puhLumaIntraDir   = NULL; }
    if ( m_puhChromaIntraDir  ) { xFree(m_puhChromaIntraDir);   m_puhChromaIntraDir = NULL; }
    if ( m_puhTrIdx           ) { xFree(m_puhTrIdx);            m_puhTrIdx          = NULL; }
    if ( m_nsqtPartIdx        ) { xFree(m_nsqtPartIdx);         m_nsqtPartIdx       = NULL; }
    if ( m_pcTrCoeffY         ) { xFree(m_pcTrCoeffY);          m_pcTrCoeffY        = NULL; }
    if ( m_pcTrCoeffCb        ) { xFree(m_pcTrCoeffCb);         m_pcTrCoeffCb       = NULL; }
    if ( m_pcTrCoeffCr        ) { xFree(m_pcTrCoeffCr);         m_pcTrCoeffCr       = NULL; }
#if ADAPTIVE_QP_SELECTION
    if (!m_ArlCoeffIsAliasedAllocation)
    {
      xFree(m_pcArlCoeffY); m_pcArlCoeffY = 0;
      xFree(m_pcArlCoeffCb); m_pcArlCoeffCb = 0;
      xFree(m_pcArlCoeffCr); m_pcArlCoeffCr = 0;
    }
    if ( m_pcGlbArlCoeffY     ) { xFree(m_pcGlbArlCoeffY);      m_pcGlbArlCoeffY    = NULL; }
    if ( m_pcGlbArlCoeffCb    ) { xFree(m_pcGlbArlCoeffCb);     m_pcGlbArlCoeffCb   = NULL; }
    if ( m_pcGlbArlCoeffCr    ) { xFree(m_pcGlbArlCoeffCr);     m_pcGlbArlCoeffCr   = NULL; }
#endif
    if ( m_pbIPCMFlag         ) { xFree(m_pbIPCMFlag   );       m_pbIPCMFlag        = NULL; }
    if ( m_pcIPCMSampleY      ) { xFree(m_pcIPCMSampleY);       m_pcIPCMSampleY     = NULL; }
    if ( m_pcIPCMSampleCb     ) { xFree(m_pcIPCMSampleCb);      m_pcIPCMSampleCb    = NULL; }
    if ( m_pcIPCMSampleCr     ) { xFree(m_pcIPCMSampleCr);      m_pcIPCMSampleCr    = NULL; }
    if ( m_apiMVPIdx[0]       ) { delete[] m_apiMVPIdx[0];      m_apiMVPIdx[0]      = NULL; }
    if ( m_apiMVPIdx[1]       ) { delete[] m_apiMVPIdx[1];      m_apiMVPIdx[1]      = NULL; }
    if ( m_apiMVPNum[0]       ) { delete[] m_apiMVPNum[0];      m_apiMVPNum[0]      = NULL; }
    if ( m_apiMVPNum[1]       ) { delete[] m_apiMVPNum[1];      m_apiMVPNum[1]      = NULL; }
    
#if HHI_DMM_WEDGE_INTRA
    if ( m_puiWedgeFullTabIdx       ) { xFree(m_puiWedgeFullTabIdx      ); m_puiWedgeFullTabIdx       = NULL; }
    if ( m_piWedgeFullDeltaDC1      ) { xFree(m_piWedgeFullDeltaDC1     ); m_piWedgeFullDeltaDC1      = NULL; }
    if ( m_piWedgeFullDeltaDC2      ) { xFree(m_piWedgeFullDeltaDC2     ); m_piWedgeFullDeltaDC2      = NULL; }

    if ( m_puiWedgePredDirTabIdx    ) { xFree(m_puiWedgePredDirTabIdx   ); m_puiWedgePredDirTabIdx    = NULL; }
    if ( m_piWedgePredDirDeltaEnd   ) { xFree(m_piWedgePredDirDeltaEnd  ); m_piWedgePredDirDeltaEnd   = NULL; }
    if ( m_piWedgePredDirDeltaDC1   ) { xFree(m_piWedgePredDirDeltaDC1  ); m_piWedgePredDirDeltaDC1   = NULL; }
    if ( m_piWedgePredDirDeltaDC2   ) { xFree(m_piWedgePredDirDeltaDC2  ); m_piWedgePredDirDeltaDC2   = NULL; }
#endif
#if HHI_DMM_PRED_TEX
    if ( m_puiWedgePredTexTabIdx    ) { xFree(m_puiWedgePredTexTabIdx   ); m_puiWedgePredTexTabIdx    = NULL; }
#if LGE_DMM3_SIMP_C0044
    if ( m_puiWedgePredTexIntraTabIdx ) { xFree(m_puiWedgePredTexIntraTabIdx); m_puiWedgePredTexIntraTabIdx    = NULL; }
#endif
    if ( m_piWedgePredTexDeltaDC1   ) { xFree(m_piWedgePredTexDeltaDC1  ); m_piWedgePredTexDeltaDC1   = NULL; }
    if ( m_piWedgePredTexDeltaDC2   ) { xFree(m_piWedgePredTexDeltaDC2  ); m_piWedgePredTexDeltaDC2   = NULL; }

    if ( m_piContourPredTexDeltaDC1 ) { xFree(m_piContourPredTexDeltaDC1); m_piContourPredTexDeltaDC1 = NULL; }
    if ( m_piContourPredTexDeltaDC2 ) { xFree(m_piContourPredTexDeltaDC2); m_piContourPredTexDeltaDC2 = NULL; }
#endif    
#if LGE_EDGE_INTRA_A0070
  if ( m_pbEdgeLeftFirst  ) { xFree(m_pbEdgeLeftFirst);   m_pbEdgeLeftFirst = NULL; }
  if ( m_pucEdgeStartPos  ) { xFree(m_pucEdgeStartPos);   m_pucEdgeStartPos = NULL; }
  if ( m_pucEdgeNumber    ) { xFree(m_pucEdgeNumber);     m_pucEdgeNumber   = NULL; }
  if ( m_pucEdgeCode      ) { xFree(m_pucEdgeCode);       m_pucEdgeCode     = NULL; }
  if ( m_pbEdgePartition    ) { xFree(m_pbEdgePartition); m_pbEdgePartition = NULL; }
#if LGE_EDGE_INTRA_DELTA_DC
  if ( m_piEdgeDeltaDC0     ) { xFree(m_piEdgeDeltaDC0);  m_piEdgeDeltaDC0  = NULL; }
  if ( m_piEdgeDeltaDC1     ) { xFree(m_piEdgeDeltaDC1);  m_piEdgeDeltaDC1  = NULL; }
#endif
#endif
    m_acCUMvField[0].destroy();
    m_acCUMvField[1].destroy();
    
  }
  
  m_pcCUAboveLeft       = NULL;
  m_pcCUAboveRight      = NULL;
  m_pcCUAbove           = NULL;
  m_pcCULeft            = NULL;
  
  m_apcCUColocated[0]   = NULL;
  m_apcCUColocated[1]   = NULL;

  if( m_uiSliceStartCU )
  {
    xFree(m_uiSliceStartCU);
    m_uiSliceStartCU=NULL;
  }
  if(m_uiEntropySliceStartCU )
  {
    xFree(m_uiEntropySliceStartCU);
    m_uiEntropySliceStartCU=NULL;
  }
}

const NDBFBlockInfo& NDBFBlockInfo::operator= (const NDBFBlockInfo& src)
{
  this->tileID = src.tileID;
  this->sliceID= src.sliceID;
  this->startSU= src.startSU;
  this->endSU  = src.endSU;
  this->widthSU= src.widthSU;
  this->heightSU=src.heightSU;
  this->posX   = src.posX;
  this->posY   = src.posY;
  this->width  = src.width;
  this->height = src.height;
  ::memcpy(this->isBorderAvailable, src.isBorderAvailable, sizeof(Bool)*((Int)NUM_SGU_BORDER));
  this->allBordersAvailable = src.allBordersAvailable;

  return *this;
}


// ====================================================================================================================
// Public member functions
// ====================================================================================================================

// --------------------------------------------------------------------------------------------------------------------
// Initialization
// --------------------------------------------------------------------------------------------------------------------

/**
 - initialize top-level CU
 - internal buffers are already created
 - set values before encoding a CU
 .
 \param  pcPic     picture (TComPic) class pointer
 \param  iCUAddr   CU address
 */
Void TComDataCU::initCU( TComPic* pcPic, UInt iCUAddr )
{
  m_pcPic              = pcPic;
  m_pcSlice            = pcPic->getSlice(pcPic->getCurrSliceIdx());
  m_uiCUAddr           = iCUAddr;
  m_uiCUPelX           = ( iCUAddr % pcPic->getFrameWidthInCU() ) * g_uiMaxCUWidth;
  m_uiCUPelY           = ( iCUAddr / pcPic->getFrameWidthInCU() ) * g_uiMaxCUHeight;
  m_uiAbsIdxInLCU      = 0;
  m_dTotalCost         = MAX_DOUBLE;
  m_uiTotalDistortion  = 0;
  m_uiTotalBits        = 0;
  m_uiTotalBins        = 0;
  m_uiNumPartition     = pcPic->getNumPartInCU();
  m_numSucIPCM       = 0;
  m_lastCUSucIPCMFlag   = false;

  for(int i=0; i<pcPic->getNumPartInCU(); i++) 
  {
    if(pcPic->getPicSym()->getInverseCUOrderMap(iCUAddr)*pcPic->getNumPartInCU()+i>=getSlice()->getSliceCurStartCUAddr())
    {
      m_uiSliceStartCU[i]=getSlice()->getSliceCurStartCUAddr();
    }
    else
    {
      m_uiSliceStartCU[i]=pcPic->getCU(getAddr())->m_uiSliceStartCU[i];
    }
  }
  for(int i=0; i<pcPic->getNumPartInCU(); i++) 
  {
    if(pcPic->getPicSym()->getInverseCUOrderMap(iCUAddr)*pcPic->getNumPartInCU()+i>=getSlice()->getEntropySliceCurStartCUAddr())
    {
      m_uiEntropySliceStartCU[i]=getSlice()->getEntropySliceCurStartCUAddr();
    }
    else
    {
      m_uiEntropySliceStartCU[i]=pcPic->getCU(getAddr())->m_uiEntropySliceStartCU[i];
    }
  }

  Int partStartIdx = getSlice()->getEntropySliceCurStartCUAddr() - pcPic->getPicSym()->getInverseCUOrderMap(iCUAddr) * pcPic->getNumPartInCU();

  Int numElements = min<Int>( partStartIdx, m_uiNumPartition );
  for ( Int ui = 0; ui < numElements; ui++ )
  {
    TComDataCU * pcFrom = pcPic->getCU(getAddr());
    m_pePartSize[ui] = pcFrom->getPartitionSize(ui);
    m_pePredMode[ui] = pcFrom->getPredictionMode(ui);
    m_puhDepth[ui] = pcFrom->getDepth(ui);
#if HHI_MPI
    m_piTextureModeDepth[ui] = pcFrom->getTextureModeDepth(ui);
#endif
#if RWTH_SDC_DLT_B0036
    m_pbSDCFlag[ui] = pcFrom->getSDCFlag(ui);
#endif
    m_puhWidth  [ui] = pcFrom->getWidth(ui);
    m_puhHeight [ui] = pcFrom->getHeight(ui);
    m_puhTrIdx  [ui] = pcFrom->getTransformIdx(ui);
    m_nsqtPartIdx[ui] = pcFrom->getNSQTPartIdx(ui);
    m_apiMVPIdx[0][ui] = pcFrom->m_apiMVPIdx[0][ui];;
    m_apiMVPIdx[1][ui] = pcFrom->m_apiMVPIdx[1][ui];
    m_apiMVPNum[0][ui] = pcFrom->m_apiMVPNum[0][ui];
    m_apiMVPNum[1][ui] = pcFrom->m_apiMVPNum[1][ui];
    m_phQP[ui]=pcFrom->m_phQP[ui];
    m_puiAlfCtrlFlag[ui]=pcFrom->m_puiAlfCtrlFlag[ui];
    m_pbMergeFlag[ui]=pcFrom->m_pbMergeFlag[ui];
#if LGE_ILLUCOMP_B0045
    m_pbICFlag[ui]=pcFrom->m_pbICFlag[ui];
#endif
    m_puhMergeIndex[ui]=pcFrom->m_puhMergeIndex[ui];
#if MERL_VSP_C0152
    m_piVSPIndex[ui] = pcFrom->m_piVSPIndex[ui];
#endif
    m_puhLumaIntraDir[ui]=pcFrom->m_puhLumaIntraDir[ui];
    m_puhChromaIntraDir[ui]=pcFrom->m_puhChromaIntraDir[ui];
    m_puhInterDir[ui]=pcFrom->m_puhInterDir[ui];
    m_puhCbf[0][ui]=pcFrom->m_puhCbf[0][ui];
    m_puhCbf[1][ui]=pcFrom->m_puhCbf[1][ui];
    m_puhCbf[2][ui]=pcFrom->m_puhCbf[2][ui];
    m_pbIPCMFlag[ui] = pcFrom->m_pbIPCMFlag[ui];
  }
  
  Int firstElement = max<Int>( partStartIdx, 0 );
  numElements = m_uiNumPartition - firstElement;
  
  if ( numElements > 0 )
  {
    memset( m_pePartSize        + firstElement, SIZE_NONE,                numElements * sizeof( *m_pePartSize ) );
    memset( m_pePredMode        + firstElement, MODE_NONE,                numElements * sizeof( *m_pePredMode ) );
    memset( m_puhDepth          + firstElement, 0,                        numElements * sizeof( *m_puhDepth ) );
#if HHI_MPI
    memset( m_piTextureModeDepth+ firstElement,-1,                        numElements * sizeof( *m_piTextureModeDepth ) );
#endif
#if RWTH_SDC_DLT_B0036
    memset( m_pbSDCFlag        + firstElement,     0,                     numElements * sizeof( *m_pbSDCFlag ) );
    memset( m_apSegmentDCOffset[0]        + firstElement,     0,                     numElements * sizeof( *m_apSegmentDCOffset[0] ) );
    memset( m_apSegmentDCOffset[1]        + firstElement,     0,                     numElements * sizeof( *m_apSegmentDCOffset[1] ) );
#endif
    memset( m_puhTrIdx          + firstElement, 0,                        numElements * sizeof( *m_puhTrIdx ) );
    memset( m_nsqtPartIdx       + firstElement, 0,                        numElements * sizeof( *m_nsqtPartIdx) );
    memset( m_puhWidth          + firstElement, g_uiMaxCUWidth,           numElements * sizeof( *m_puhWidth ) );
    memset( m_puhHeight         + firstElement, g_uiMaxCUHeight,          numElements * sizeof( *m_puhHeight ) );
    memset( m_apiMVPIdx[0]      + firstElement, -1,                       numElements * sizeof( *m_apiMVPIdx[0] ) );
    memset( m_apiMVPIdx[1]      + firstElement, -1,                       numElements * sizeof( *m_apiMVPIdx[1] ) );
    memset( m_apiMVPNum[0]      + firstElement, -1,                       numElements * sizeof( *m_apiMVPNum[0] ) );
    memset( m_apiMVPNum[1]      + firstElement, -1,                       numElements * sizeof( *m_apiMVPNum[1] ) );
    memset( m_phQP              + firstElement, getSlice()->getSliceQp(), numElements * sizeof( *m_phQP ) );
    memset( m_puiAlfCtrlFlag    + firstElement, false,                    numElements * sizeof( *m_puiAlfCtrlFlag ) );
    memset( m_pbMergeFlag       + firstElement, false,                    numElements * sizeof( *m_pbMergeFlag ) );
#if LGE_ILLUCOMP_B0045
    memset( m_pbICFlag          + firstElement, false,                    numElements * sizeof( *m_pbICFlag ) );
#endif
    memset( m_puhMergeIndex     + firstElement, 0,                        numElements * sizeof( *m_puhMergeIndex ) );
#if MERL_VSP_C0152
    memset( m_piVSPIndex        + firstElement, 0,                        numElements * sizeof( *m_piVSPIndex ) );
#endif
    memset( m_puhLumaIntraDir   + firstElement, 2,                        numElements * sizeof( *m_puhLumaIntraDir ) );
    memset( m_puhChromaIntraDir + firstElement, 0,                        numElements * sizeof( *m_puhChromaIntraDir ) );
    memset( m_puhInterDir       + firstElement, 0,                        numElements * sizeof( *m_puhInterDir ) );
    memset( m_puhCbf[0]         + firstElement, 0,                        numElements * sizeof( *m_puhCbf[0] ) );
    memset( m_puhCbf[1]         + firstElement, 0,                        numElements * sizeof( *m_puhCbf[1] ) );
    memset( m_puhCbf[2]         + firstElement, 0,                        numElements * sizeof( *m_puhCbf[2] ) );
    memset( m_pbIPCMFlag        + firstElement, false,                    numElements * sizeof( *m_pbIPCMFlag ) );

#if HHI_DMM_WEDGE_INTRA
    memset( m_puiWedgeFullTabIdx       + firstElement, 0, sizeof( UInt ) * numElements );
    memset( m_piWedgeFullDeltaDC1      + firstElement, 0, sizeof( Int  ) * numElements );
    memset( m_piWedgeFullDeltaDC2      + firstElement, 0, sizeof( Int  ) * numElements );

    memset( m_puiWedgePredDirTabIdx    + firstElement, 0, sizeof( UInt ) * numElements );
    memset( m_piWedgePredDirDeltaDC1   + firstElement, 0, sizeof( Int  ) * numElements );
    memset( m_piWedgePredDirDeltaDC2   + firstElement, 0, sizeof( Int  ) * numElements );
    memset( m_piWedgePredDirDeltaEnd   + firstElement, 0, sizeof( Int  ) * numElements );
#endif
#if HHI_DMM_PRED_TEX
    memset( m_puiWedgePredTexTabIdx    + firstElement, 0, sizeof( UInt ) * numElements );
#if LGE_DMM3_SIMP_C0044
    memset( m_puiWedgePredTexIntraTabIdx + firstElement, 0, sizeof( UInt ) * numElements );
#endif
    memset( m_piWedgePredTexDeltaDC1   + firstElement, 0, sizeof( Int  ) * numElements );
    memset( m_piWedgePredTexDeltaDC2   + firstElement, 0, sizeof( Int  ) * numElements );

    memset( m_piContourPredTexDeltaDC1 + firstElement, 0, sizeof( Int  ) * numElements );
    memset( m_piContourPredTexDeltaDC2 + firstElement, 0, sizeof( Int  ) * numElements );
#endif 
#if HHI_INTERVIEW_SKIP
    memset (m_pbRenderable             + firstElement, false, sizeof( Bool ) * numElements) ;
#endif
#if H3D_IVRP
    memset( m_pbResPredAvailable       + firstElement, 0    , sizeof( Bool ) * numElements );
    memset( m_pbResPredFlag            + firstElement, 0    , sizeof( Bool ) * numElements );
#endif
  }
  
  UInt uiTmp = g_uiMaxCUWidth*g_uiMaxCUHeight;
  if ( 0 >= partStartIdx ) 
  {
    m_acCUMvField[0].clearMvField();
    m_acCUMvField[1].clearMvField();
    memset( m_pcTrCoeffY , 0, sizeof( TCoeff ) * uiTmp );
#if ADAPTIVE_QP_SELECTION
    memset( m_pcArlCoeffY , 0, sizeof( Int ) * uiTmp );  
#endif
    memset( m_pcIPCMSampleY , 0, sizeof( Pel ) * uiTmp );
    uiTmp  >>= 2;
    memset( m_pcTrCoeffCb, 0, sizeof( TCoeff ) * uiTmp );
    memset( m_pcTrCoeffCr, 0, sizeof( TCoeff ) * uiTmp );
#if ADAPTIVE_QP_SELECTION  
    memset( m_pcArlCoeffCb, 0, sizeof( Int ) * uiTmp );
    memset( m_pcArlCoeffCr, 0, sizeof( Int ) * uiTmp );
#endif
    memset( m_pcIPCMSampleCb , 0, sizeof( Pel ) * uiTmp );
    memset( m_pcIPCMSampleCr , 0, sizeof( Pel ) * uiTmp );
  }
  else 
  {
    TComDataCU * pcFrom = pcPic->getCU(getAddr());
    m_acCUMvField[0].copyFrom(&pcFrom->m_acCUMvField[0],m_uiNumPartition,0);
    m_acCUMvField[1].copyFrom(&pcFrom->m_acCUMvField[1],m_uiNumPartition,0);
    for(int i=0; i<uiTmp; i++) 
    {
      m_pcTrCoeffY[i]=pcFrom->m_pcTrCoeffY[i];
#if ADAPTIVE_QP_SELECTION
      m_pcArlCoeffY[i]=pcFrom->m_pcArlCoeffY[i];
#endif
      m_pcIPCMSampleY[i]=pcFrom->m_pcIPCMSampleY[i];
    }
    for(int i=0; i<(uiTmp>>2); i++) 
    {
      m_pcTrCoeffCb[i]=pcFrom->m_pcTrCoeffCb[i];
      m_pcTrCoeffCr[i]=pcFrom->m_pcTrCoeffCr[i];
#if ADAPTIVE_QP_SELECTION
      m_pcArlCoeffCb[i]=pcFrom->m_pcArlCoeffCb[i];
      m_pcArlCoeffCr[i]=pcFrom->m_pcArlCoeffCr[i];
#endif
      m_pcIPCMSampleCb[i]=pcFrom->m_pcIPCMSampleCb[i];
      m_pcIPCMSampleCr[i]=pcFrom->m_pcIPCMSampleCr[i];
    }
  }

  // Setting neighbor CU
  m_pcCULeft        = NULL;
  m_pcCUAbove       = NULL;
  m_pcCUAboveLeft   = NULL;
  m_pcCUAboveRight  = NULL;

  m_apcCUColocated[0] = NULL;
  m_apcCUColocated[1] = NULL;

  UInt uiWidthInCU = pcPic->getFrameWidthInCU();
  if ( m_uiCUAddr % uiWidthInCU )
  {
    m_pcCULeft = pcPic->getCU( m_uiCUAddr - 1 );
  }

  if ( m_uiCUAddr / uiWidthInCU )
  {
    m_pcCUAbove = pcPic->getCU( m_uiCUAddr - uiWidthInCU );
  }

  if ( m_pcCULeft && m_pcCUAbove )
  {
    m_pcCUAboveLeft = pcPic->getCU( m_uiCUAddr - uiWidthInCU - 1 );
  }

  if ( m_pcCUAbove && ( (m_uiCUAddr%uiWidthInCU) < (uiWidthInCU-1) )  )
  {
    m_pcCUAboveRight = pcPic->getCU( m_uiCUAddr - uiWidthInCU + 1 );
  }

  if ( getSlice()->getNumRefIdx( REF_PIC_LIST_0 ) > 0 )
  {
    m_apcCUColocated[0] = getSlice()->getRefPic( REF_PIC_LIST_0, 0)->getCU( m_uiCUAddr );
  }

  if ( getSlice()->getNumRefIdx( REF_PIC_LIST_1 ) > 0 )
  {
    m_apcCUColocated[1] = getSlice()->getRefPic( REF_PIC_LIST_1, 0)->getCU( m_uiCUAddr );
  }
}

/** initialize prediction data with enabling sub-LCU-level delta QP
*\param  uiDepth  depth of the current CU
*\param  qp     qp for the current CU
*- set CU width and CU height according to depth
*- set qp value according to input qp 
*- set last-coded qp value according to input last-coded qp 
*/
Void TComDataCU::initEstData( UInt uiDepth, Int qp )
{
  m_dTotalCost         = MAX_DOUBLE;
  m_uiTotalDistortion  = 0;
  m_uiTotalBits        = 0;
  m_uiTotalBins        = 0;

  UChar uhWidth  = g_uiMaxCUWidth  >> uiDepth;
  UChar uhHeight = g_uiMaxCUHeight >> uiDepth;

  for (UInt ui = 0; ui < m_uiNumPartition; ui++)
  {
    if(getPic()->getPicSym()->getInverseCUOrderMap(getAddr())*m_pcPic->getNumPartInCU()+m_uiAbsIdxInLCU+ui >= getSlice()->getEntropySliceCurStartCUAddr())
    {
#if HHI_INTERVIEW_SKIP
      m_pbRenderable[ui]=  0 ;
#endif
      m_apiMVPIdx[0][ui] = -1;
      m_apiMVPIdx[1][ui] = -1;
      m_apiMVPNum[0][ui] = -1;
      m_apiMVPNum[1][ui] = -1;
      m_puhDepth  [ui] = uiDepth;
      m_puhWidth  [ui] = uhWidth;
      m_puhHeight [ui] = uhHeight;
      m_puhTrIdx  [ui] = 0;
#if HHI_MPI
      m_piTextureModeDepth[ui] = -1;
#endif
      m_nsqtPartIdx[ui] = 0;
      m_pePartSize[ui] = SIZE_NONE;
      m_pePredMode[ui] = MODE_NONE;
      m_pbIPCMFlag[ui] = 0;
      m_phQP[ui] = qp;
      m_puiAlfCtrlFlag[ui]= false;
      m_pbMergeFlag[ui] = 0;
#if LGE_ILLUCOMP_B0045
      m_pbICFlag[ui]    = false;
#endif
      m_puhMergeIndex[ui] = 0;
#if MERL_VSP_C0152
      m_piVSPIndex[ui] = 0;
#endif
#if H3D_IVRP
      m_pbResPredAvailable[ui] = 0;
      m_pbResPredFlag[ui]      = 0;
#endif
      m_puhLumaIntraDir[ui] = 2;
      m_puhChromaIntraDir[ui] = 0;
      m_puhInterDir[ui] = 0;
      m_puhCbf[0][ui] = 0;
      m_puhCbf[1][ui] = 0;
      m_puhCbf[2][ui] = 0;
#if HHI_DMM_WEDGE_INTRA
    m_puiWedgeFullTabIdx      [ui] = 0;
    m_piWedgeFullDeltaDC1     [ui] = 0;
    m_piWedgeFullDeltaDC2     [ui] = 0;

    m_puiWedgePredDirTabIdx   [ui] = 0;
    m_piWedgePredDirDeltaDC1  [ui] = 0;
    m_piWedgePredDirDeltaDC2  [ui] = 0;
    m_piWedgePredDirDeltaEnd  [ui] = 0;
#endif
#if HHI_DMM_PRED_TEX
    m_puiWedgePredTexTabIdx   [ui] = 0;
#if LGE_DMM3_SIMP_C0044
    m_puiWedgePredTexIntraTabIdx [ui] = 0;
#endif
    m_piWedgePredTexDeltaDC1  [ui] = 0;
    m_piWedgePredTexDeltaDC2  [ui] = 0;

    m_piContourPredTexDeltaDC1[ui] = 0;
    m_piContourPredTexDeltaDC2[ui] = 0;
#endif 
#if RWTH_SDC_DLT_B0036
    m_pbSDCFlag[ui] = false;
    m_apSegmentDCOffset[0][ui] = 0;
    m_apSegmentDCOffset[1][ui] = 0;
#endif
    }
  }

  UInt uiTmp = uhWidth*uhHeight;

  if(getPic()->getPicSym()->getInverseCUOrderMap(getAddr())*m_pcPic->getNumPartInCU()+m_uiAbsIdxInLCU >= getSlice()->getEntropySliceCurStartCUAddr())
  {
    m_acCUMvField[0].clearMvField();
    m_acCUMvField[1].clearMvField();
    uiTmp = uhWidth*uhHeight;
    
    memset( m_pcTrCoeffY,    0, uiTmp * sizeof( *m_pcTrCoeffY    ) );
#if ADAPTIVE_QP_SELECTION
    memset( m_pcArlCoeffY ,  0, uiTmp * sizeof( *m_pcArlCoeffY   ) );
#endif
    memset( m_pcIPCMSampleY, 0, uiTmp * sizeof( *m_pcIPCMSampleY ) );

    uiTmp>>=2;
    memset( m_pcTrCoeffCb,    0, uiTmp * sizeof( *m_pcTrCoeffCb    ) );
    memset( m_pcTrCoeffCr,    0, uiTmp * sizeof( *m_pcTrCoeffCr    ) );
#if ADAPTIVE_QP_SELECTION  
    memset( m_pcArlCoeffCb,   0, uiTmp * sizeof( *m_pcArlCoeffCb   ) );
    memset( m_pcArlCoeffCr,   0, uiTmp * sizeof( *m_pcArlCoeffCr   ) );
#endif
    memset( m_pcIPCMSampleCb, 0, uiTmp * sizeof( *m_pcIPCMSampleCb ) );
    memset( m_pcIPCMSampleCr, 0, uiTmp * sizeof( *m_pcIPCMSampleCr ) );
  }
}


// initialize Sub partition
Void TComDataCU::initSubCU( TComDataCU* pcCU, UInt uiPartUnitIdx, UInt uiDepth, Int qp )
{
  assert( uiPartUnitIdx<4 );

  UInt uiPartOffset = ( pcCU->getTotalNumPart()>>2 )*uiPartUnitIdx;

  m_pcPic              = pcCU->getPic();
  m_pcSlice            = m_pcPic->getSlice(m_pcPic->getCurrSliceIdx());
  m_uiCUAddr           = pcCU->getAddr();
  m_uiAbsIdxInLCU      = pcCU->getZorderIdxInCU() + uiPartOffset;

  m_uiCUPelX           = pcCU->getCUPelX() + ( g_uiMaxCUWidth>>uiDepth  )*( uiPartUnitIdx &  1 );
  m_uiCUPelY           = pcCU->getCUPelY() + ( g_uiMaxCUHeight>>uiDepth  )*( uiPartUnitIdx >> 1 );

  m_dTotalCost         = MAX_DOUBLE;
  m_uiTotalDistortion  = 0;
  m_uiTotalBits        = 0;
  m_uiTotalBins        = 0;
  m_uiNumPartition     = pcCU->getTotalNumPart() >> 2;

  m_numSucIPCM       = 0;
  m_lastCUSucIPCMFlag   = false;

  Int iSizeInUchar = sizeof( UChar  ) * m_uiNumPartition;
  Int iSizeInBool  = sizeof( Bool   ) * m_uiNumPartition;
#if MERL_VSP_C0152
  Int iSizeInChar  = sizeof( Char   ) * m_uiNumPartition;
#endif

  Int sizeInChar = sizeof( Char  ) * m_uiNumPartition;
  memset( m_phQP,              qp,  sizeInChar );

  memset( m_puiAlfCtrlFlag,     0, iSizeInBool );
  memset( m_pbMergeFlag,        0, iSizeInBool  );
#if LGE_ILLUCOMP_B0045
  memset( m_pbICFlag,           0, iSizeInBool  );
#endif
#if HHI_INTERVIEW_SKIP
  memset( m_pbRenderable,        0, iSizeInBool  );
#endif
  memset( m_puhMergeIndex,      0, iSizeInUchar );
#if MERL_VSP_C0152
  memset( m_piVSPIndex,         0, iSizeInChar );
#endif
#if H3D_IVRP
  memset( m_pbResPredAvailable, 0, iSizeInBool  );
  memset( m_pbResPredFlag,      0, iSizeInBool  );
#endif
  memset( m_puhLumaIntraDir,    2, iSizeInUchar );
  memset( m_puhChromaIntraDir,  0, iSizeInUchar );
  memset( m_puhInterDir,        0, iSizeInUchar );
  memset( m_puhTrIdx,           0, iSizeInUchar );
  memset( m_puhCbf[0],          0, iSizeInUchar );
  memset( m_puhCbf[1],          0, iSizeInUchar );
  memset( m_puhCbf[2],          0, iSizeInUchar );
  memset( m_puhDepth,     uiDepth, iSizeInUchar );
#if HHI_MPI
  memset( m_piTextureModeDepth, -1, sizeof( Int ) * m_uiNumPartition );
#endif

#if HHI_DMM_WEDGE_INTRA
  memset( m_puiWedgeFullTabIdx,       0, sizeof( UInt ) * m_uiNumPartition );
  memset( m_piWedgeFullDeltaDC1,      0, sizeof( Int  ) * m_uiNumPartition );
  memset( m_piWedgeFullDeltaDC2,      0, sizeof( Int  ) * m_uiNumPartition );

  memset( m_puiWedgePredDirTabIdx,    0, sizeof( UInt ) * m_uiNumPartition );
  memset( m_piWedgePredDirDeltaDC1,   0, sizeof( Int  ) * m_uiNumPartition );
  memset( m_piWedgePredDirDeltaDC2,   0, sizeof( Int  ) * m_uiNumPartition );
  memset( m_piWedgePredDirDeltaEnd,   0, sizeof( Int  ) * m_uiNumPartition );
#endif
#if HHI_DMM_PRED_TEX
  memset( m_puiWedgePredTexTabIdx,    0, sizeof( UInt ) * m_uiNumPartition );
#if LGE_DMM3_SIMP_C0044
  memset( m_puiWedgePredTexIntraTabIdx, 0, sizeof( UInt ) * m_uiNumPartition );
#endif
  memset( m_piWedgePredTexDeltaDC1,   0, sizeof( Int  ) * m_uiNumPartition );
  memset( m_piWedgePredTexDeltaDC2,   0, sizeof( Int  ) * m_uiNumPartition );

  memset( m_piContourPredTexDeltaDC1, 0, sizeof( Int  ) * m_uiNumPartition );
  memset( m_piContourPredTexDeltaDC2, 0, sizeof( Int  ) * m_uiNumPartition );
#endif   
#if RWTH_SDC_DLT_B0036
  memset( m_pbSDCFlag,     0, sizeof(Bool) * m_uiNumPartition  );
  memset( m_apSegmentDCOffset[0],     0, sizeof(Pel) * m_uiNumPartition);
  memset( m_apSegmentDCOffset[1],     0, sizeof(Pel) * m_uiNumPartition);
#endif

  UChar uhWidth  = g_uiMaxCUWidth  >> uiDepth;
  UChar uhHeight = g_uiMaxCUHeight >> uiDepth;
  memset( m_puhWidth,          uhWidth,  iSizeInUchar );
  memset( m_puhHeight,         uhHeight, iSizeInUchar );
  memset( m_pbIPCMFlag,        0, iSizeInBool  );
  for (UInt ui = 0; ui < m_uiNumPartition; ui++)
  {
    m_pePartSize[ui] = SIZE_NONE;
    m_pePredMode[ui] = MODE_NONE;
#if HHI_INTERVIEW_SKIP
    m_pbRenderable[ui]=  0 ;
#endif
    m_apiMVPIdx[0][ui] = -1;
    m_apiMVPIdx[1][ui] = -1;
    m_apiMVPNum[0][ui] = -1;
    m_apiMVPNum[1][ui] = -1;
    if(m_pcPic->getPicSym()->getInverseCUOrderMap(getAddr())*m_pcPic->getNumPartInCU()+m_uiAbsIdxInLCU+ui<getSlice()->getEntropySliceCurStartCUAddr())
    {
      m_apiMVPIdx[0][ui] = pcCU->m_apiMVPIdx[0][uiPartOffset+ui];
      m_apiMVPIdx[1][ui] = pcCU->m_apiMVPIdx[1][uiPartOffset+ui];;
      m_apiMVPNum[0][ui] = pcCU->m_apiMVPNum[0][uiPartOffset+ui];;
      m_apiMVPNum[1][ui] = pcCU->m_apiMVPNum[1][uiPartOffset+ui];;
      m_puhDepth  [ui] = pcCU->getDepth(uiPartOffset+ui);
#if HHI_MPI
      m_piTextureModeDepth[ui] = pcCU->getTextureModeDepth(uiPartOffset+ui);
#endif
      m_puhWidth  [ui] = pcCU->getWidth(uiPartOffset+ui);
      m_puhHeight  [ui] = pcCU->getHeight(uiPartOffset+ui);
      m_puhTrIdx  [ui] = pcCU->getTransformIdx(uiPartOffset+ui);
      m_nsqtPartIdx[ui] = pcCU->getNSQTPartIdx(uiPartOffset+ui);
      m_pePartSize[ui] = pcCU->getPartitionSize(uiPartOffset+ui);
      m_pePredMode[ui] = pcCU->getPredictionMode(uiPartOffset+ui);
      m_pbIPCMFlag[ui]=pcCU->m_pbIPCMFlag[uiPartOffset+ui];
      m_phQP[ui] = pcCU->m_phQP[uiPartOffset+ui];
      m_puiAlfCtrlFlag[ui]=pcCU->m_puiAlfCtrlFlag[uiPartOffset+ui];
      m_pbMergeFlag[ui]=pcCU->m_pbMergeFlag[uiPartOffset+ui];
#if LGE_ILLUCOMP_B0045
      m_pbICFlag[ui]=pcCU->m_pbICFlag[uiPartOffset+ui];
#endif
      m_puhMergeIndex[ui]=pcCU->m_puhMergeIndex[uiPartOffset+ui];
#if MERL_VSP_C0152
      m_piVSPIndex[ui]=pcCU->m_piVSPIndex[uiPartOffset+ui];
#endif
      m_puhLumaIntraDir[ui]=pcCU->m_puhLumaIntraDir[uiPartOffset+ui];
      m_puhChromaIntraDir[ui]=pcCU->m_puhChromaIntraDir[uiPartOffset+ui];
      m_puhInterDir[ui]=pcCU->m_puhInterDir[uiPartOffset+ui];
      m_puhCbf[0][ui]=pcCU->m_puhCbf[0][uiPartOffset+ui];
      m_puhCbf[1][ui]=pcCU->m_puhCbf[1][uiPartOffset+ui];
      m_puhCbf[2][ui]=pcCU->m_puhCbf[2][uiPartOffset+ui];

#if HHI_DMM_WEDGE_INTRA
      m_puiWedgeFullTabIdx      [ui]=pcCU->getWedgeFullTabIdx       (uiPartOffset+ui);
      m_piWedgeFullDeltaDC1     [ui]=pcCU->getWedgeFullDeltaDC1     (uiPartOffset+ui);
      m_piWedgeFullDeltaDC2     [ui]=pcCU->getWedgeFullDeltaDC2     (uiPartOffset+ui);

      m_puiWedgePredDirTabIdx   [ui]=pcCU->getWedgePredDirTabIdx    (uiPartOffset+ui);
      m_piWedgePredDirDeltaDC1  [ui]=pcCU->getWedgePredDirDeltaDC1  (uiPartOffset+ui);
      m_piWedgePredDirDeltaDC2  [ui]=pcCU->getWedgePredDirDeltaDC2  (uiPartOffset+ui);
      m_piWedgePredDirDeltaEnd  [ui]=pcCU->getWedgePredDirDeltaEnd  (uiPartOffset+ui);
#endif
#if HHI_DMM_PRED_TEX
      m_puiWedgePredTexTabIdx   [ui]=pcCU->getWedgePredTexTabIdx    (uiPartOffset+ui);
#if LGE_DMM3_SIMP_C0044
      m_puiWedgePredTexIntraTabIdx [ui]=pcCU->getWedgePredTexIntraTabIdx (uiPartOffset+ui);
#endif
      m_piWedgePredTexDeltaDC1  [ui]=pcCU->getWedgePredTexDeltaDC1  (uiPartOffset+ui);
      m_piWedgePredTexDeltaDC2  [ui]=pcCU->getWedgePredTexDeltaDC2  (uiPartOffset+ui);

      m_piContourPredTexDeltaDC1[ui]=pcCU->getContourPredTexDeltaDC1(uiPartOffset+ui);
      m_piContourPredTexDeltaDC2[ui]=pcCU->getContourPredTexDeltaDC2(uiPartOffset+ui);
#endif    
#if H3D_IVRP
      m_pbResPredAvailable[ui] = pcCU->m_pbResPredAvailable[ uiPartOffset + ui ];
      m_pbResPredFlag     [ui] = pcCU->m_pbResPredFlag     [ uiPartOffset + ui ];
#endif
#if RWTH_SDC_DLT_B0036
      m_pbSDCFlag         [ui] = pcCU->m_pbSDCFlag         [ uiPartOffset + ui ];
      m_apSegmentDCOffset[0][ui] = pcCU->m_apSegmentDCOffset[0][ uiPartOffset + ui ];
      m_apSegmentDCOffset[1][ui] = pcCU->m_apSegmentDCOffset[1][ uiPartOffset + ui ];
#endif
    }
  }
  UInt uiTmp = uhWidth*uhHeight;
  memset( m_pcTrCoeffY , 0, sizeof(TCoeff)*uiTmp );
#if ADAPTIVE_QP_SELECTION  
  memset( m_pcArlCoeffY , 0, sizeof(Int)*uiTmp );
#endif
  memset( m_pcIPCMSampleY , 0, sizeof( Pel ) * uiTmp );
  uiTmp >>= 2;
  memset( m_pcTrCoeffCb, 0, sizeof(TCoeff)*uiTmp );
  memset( m_pcTrCoeffCr, 0, sizeof(TCoeff)*uiTmp );
#if ADAPTIVE_QP_SELECTION
  memset( m_pcArlCoeffCb, 0, sizeof(Int)*uiTmp );
  memset( m_pcArlCoeffCr, 0, sizeof(Int)*uiTmp );
#endif
  memset( m_pcIPCMSampleCb , 0, sizeof( Pel ) * uiTmp );
  memset( m_pcIPCMSampleCr , 0, sizeof( Pel ) * uiTmp );
  m_acCUMvField[0].clearMvField();
  m_acCUMvField[1].clearMvField();

  if(m_pcPic->getPicSym()->getInverseCUOrderMap(getAddr())*m_pcPic->getNumPartInCU()+m_uiAbsIdxInLCU<getSlice()->getEntropySliceCurStartCUAddr())
  {
    // Part of this CU contains data from an older slice. Now copy in that data.
    UInt uiMaxCuWidth=pcCU->getSlice()->getSPS()->getMaxCUWidth();
    UInt uiMaxCuHeight=pcCU->getSlice()->getSPS()->getMaxCUHeight();
    TComDataCU * bigCU = getPic()->getCU(getAddr());
    Int minui = uiPartOffset;
    minui = -minui;
    pcCU->m_acCUMvField[0].copyTo(&m_acCUMvField[0],minui,uiPartOffset,m_uiNumPartition);
    pcCU->m_acCUMvField[1].copyTo(&m_acCUMvField[1],minui,uiPartOffset,m_uiNumPartition);
    UInt uiCoffOffset = uiMaxCuWidth*uiMaxCuHeight*m_uiAbsIdxInLCU/pcCU->getPic()->getNumPartInCU();
    uiTmp = uhWidth*uhHeight;
    for(int i=0; i<uiTmp; i++) 
    {
      m_pcTrCoeffY[i]=bigCU->m_pcTrCoeffY[uiCoffOffset+i];
#if ADAPTIVE_QP_SELECTION
      m_pcArlCoeffY[i]=bigCU->m_pcArlCoeffY[uiCoffOffset+i];
#endif
      m_pcIPCMSampleY[i]=bigCU->m_pcIPCMSampleY[uiCoffOffset+i];
    }
    uiTmp>>=2;
    uiCoffOffset>>=2;
    for(int i=0; i<uiTmp; i++) 
    {
      m_pcTrCoeffCr[i]=bigCU->m_pcTrCoeffCr[uiCoffOffset+i];
      m_pcTrCoeffCb[i]=bigCU->m_pcTrCoeffCb[uiCoffOffset+i];
#if ADAPTIVE_QP_SELECTION
      m_pcArlCoeffCr[i]=bigCU->m_pcArlCoeffCr[uiCoffOffset+i];
      m_pcArlCoeffCb[i]=bigCU->m_pcArlCoeffCb[uiCoffOffset+i];
#endif
      m_pcIPCMSampleCb[i]=bigCU->m_pcIPCMSampleCb[uiCoffOffset+i];
      m_pcIPCMSampleCr[i]=bigCU->m_pcIPCMSampleCr[uiCoffOffset+i];
    }
  }

  m_pcCULeft        = pcCU->getCULeft();
  m_pcCUAbove       = pcCU->getCUAbove();
  m_pcCUAboveLeft   = pcCU->getCUAboveLeft();
  m_pcCUAboveRight  = pcCU->getCUAboveRight();

  m_apcCUColocated[0] = pcCU->getCUColocated(REF_PIC_LIST_0);
  m_apcCUColocated[1] = pcCU->getCUColocated(REF_PIC_LIST_1);
  memcpy(m_uiSliceStartCU,pcCU->m_uiSliceStartCU+uiPartOffset,sizeof(UInt)*m_uiNumPartition);
  memcpy(m_uiEntropySliceStartCU,pcCU->m_uiEntropySliceStartCU+uiPartOffset,sizeof(UInt)*m_uiNumPartition);
}

Void TComDataCU::setOutsideCUPart( UInt uiAbsPartIdx, UInt uiDepth )
{
  UInt uiNumPartition = m_uiNumPartition >> (uiDepth << 1);
  UInt uiSizeInUchar = sizeof( UChar  ) * uiNumPartition;

  UChar uhWidth  = g_uiMaxCUWidth  >> uiDepth;
  UChar uhHeight = g_uiMaxCUHeight >> uiDepth;
  memset( m_puhDepth    + uiAbsPartIdx,     uiDepth,  uiSizeInUchar );
  memset( m_puhWidth    + uiAbsPartIdx,     uhWidth,  uiSizeInUchar );
  memset( m_puhHeight   + uiAbsPartIdx,     uhHeight, uiSizeInUchar );
}

// --------------------------------------------------------------------------------------------------------------------
// Copy
// --------------------------------------------------------------------------------------------------------------------

Void TComDataCU::copySubCU( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth )
{
  UInt uiPart = uiAbsPartIdx;
  
  m_pcPic              = pcCU->getPic();
  m_pcSlice            = pcCU->getSlice();
  m_uiCUAddr           = pcCU->getAddr();
  m_uiAbsIdxInLCU      = uiAbsPartIdx;
  
  m_uiCUPelX           = pcCU->getCUPelX() + g_auiRasterToPelX[ g_auiZscanToRaster[uiAbsPartIdx] ];
  m_uiCUPelY           = pcCU->getCUPelY() + g_auiRasterToPelY[ g_auiZscanToRaster[uiAbsPartIdx] ];
  
  UInt uiWidth         = g_uiMaxCUWidth  >> uiDepth;
  UInt uiHeight        = g_uiMaxCUHeight >> uiDepth;
  
  m_phQP=pcCU->getQP()                    + uiPart;
  m_pePartSize = pcCU->getPartitionSize() + uiPart;
#if HHI_INTERVIEW_SKIP
  m_pbRenderable         = pcCU->getRenderable()        + uiPart;

#endif
  m_pePredMode=pcCU->getPredictionMode()  + uiPart;
  
  m_pbMergeFlag         = pcCU->getMergeFlag()        + uiPart;
#if LGE_ILLUCOMP_B0045
  m_pbICFlag            = pcCU->getICFlag()           + uiPart;
#endif
  m_puhMergeIndex       = pcCU->getMergeIndex()       + uiPart;
#if MERL_VSP_C0152
  m_piVSPIndex          = pcCU->getVSPIndex()         + uiPart;
#endif
#if H3D_IVRP
  m_pbResPredAvailable  = pcCU->getResPredAvail()     + uiPart;
  m_pbResPredFlag       = pcCU->getResPredFlag ()     + uiPart;
#endif
  m_puhLumaIntraDir     = pcCU->getLumaIntraDir()     + uiPart;
  m_puhChromaIntraDir   = pcCU->getChromaIntraDir()   + uiPart;
  m_puhInterDir         = pcCU->getInterDir()         + uiPart;
  m_puhTrIdx            = pcCU->getTransformIdx()     + uiPart;
  m_nsqtPartIdx         = pcCU->getNSQTPartIdx()      + uiPart;
  
  m_puhCbf[0]= pcCU->getCbf(TEXT_LUMA)            + uiPart;
  m_puhCbf[1]= pcCU->getCbf(TEXT_CHROMA_U)        + uiPart;
  m_puhCbf[2]= pcCU->getCbf(TEXT_CHROMA_V)        + uiPart;
  
  m_puhDepth=pcCU->getDepth()                     + uiPart;
  m_puhWidth=pcCU->getWidth()                     + uiPart;
  m_puhHeight=pcCU->getHeight()                   + uiPart;
#if HHI_MPI
  m_piTextureModeDepth=pcCU->getTextureModeDepth()+ uiPart;
#endif
  
  m_apiMVPIdx[0]=pcCU->getMVPIdx(REF_PIC_LIST_0)  + uiPart;
  m_apiMVPIdx[1]=pcCU->getMVPIdx(REF_PIC_LIST_1)  + uiPart;
  m_apiMVPNum[0]=pcCU->getMVPNum(REF_PIC_LIST_0)  + uiPart;
  m_apiMVPNum[1]=pcCU->getMVPNum(REF_PIC_LIST_1)  + uiPart;
  
  m_pbIPCMFlag         = pcCU->getIPCMFlag()        + uiPart;

  m_pcCUAboveLeft      = pcCU->getCUAboveLeft();
  m_pcCUAboveRight     = pcCU->getCUAboveRight();
  m_pcCUAbove          = pcCU->getCUAbove();
  m_pcCULeft           = pcCU->getCULeft();
  
  m_apcCUColocated[0] = pcCU->getCUColocated(REF_PIC_LIST_0);
  m_apcCUColocated[1] = pcCU->getCUColocated(REF_PIC_LIST_1);
  
  UInt uiTmp = uiWidth*uiHeight;
  UInt uiMaxCuWidth=pcCU->getSlice()->getSPS()->getMaxCUWidth();
  UInt uiMaxCuHeight=pcCU->getSlice()->getSPS()->getMaxCUHeight();
  
  UInt uiCoffOffset = uiMaxCuWidth*uiMaxCuHeight*uiAbsPartIdx/pcCU->getPic()->getNumPartInCU();
  
  m_pcTrCoeffY = pcCU->getCoeffY() + uiCoffOffset;
#if ADAPTIVE_QP_SELECTION
  m_pcArlCoeffY= pcCU->getArlCoeffY() + uiCoffOffset;  
#endif
  m_pcIPCMSampleY = pcCU->getPCMSampleY() + uiCoffOffset;

  uiTmp >>= 2;
  uiCoffOffset >>=2;
  m_pcTrCoeffCb=pcCU->getCoeffCb() + uiCoffOffset;
  m_pcTrCoeffCr=pcCU->getCoeffCr() + uiCoffOffset;
#if ADAPTIVE_QP_SELECTION  
  m_pcArlCoeffCb=pcCU->getArlCoeffCb() + uiCoffOffset;
  m_pcArlCoeffCr=pcCU->getArlCoeffCr() + uiCoffOffset;
#endif
  m_pcIPCMSampleCb = pcCU->getPCMSampleCb() + uiCoffOffset;
  m_pcIPCMSampleCr = pcCU->getPCMSampleCr() + uiCoffOffset;

  m_acCUMvField[0].linkToWithOffset( pcCU->getCUMvField(REF_PIC_LIST_0), uiPart );
  m_acCUMvField[1].linkToWithOffset( pcCU->getCUMvField(REF_PIC_LIST_1), uiPart );
  memcpy(m_uiSliceStartCU,pcCU->m_uiSliceStartCU+uiPart,sizeof(UInt)*m_uiNumPartition);
  memcpy(m_uiEntropySliceStartCU,pcCU->m_uiEntropySliceStartCU+uiPart,sizeof(UInt)*m_uiNumPartition);

#if HHI_DMM_WEDGE_INTRA
  m_puiWedgeFullTabIdx       = pcCU->getWedgeFullTabIdx()        + uiPart;   
  m_piWedgeFullDeltaDC1      = pcCU->getWedgeFullDeltaDC1()      + uiPart;   
  m_piWedgeFullDeltaDC2      = pcCU->getWedgeFullDeltaDC2()      + uiPart;   

  m_puiWedgePredDirTabIdx    = pcCU->getWedgePredDirTabIdx()     + uiPart;   
  m_piWedgePredDirDeltaDC1   = pcCU->getWedgePredDirDeltaDC1()   + uiPart;   
  m_piWedgePredDirDeltaDC2   = pcCU->getWedgePredDirDeltaDC2()   + uiPart;   
  m_piWedgePredDirDeltaEnd   = pcCU->getWedgePredDirDeltaEnd()   + uiPart;
#endif
#if HHI_DMM_PRED_TEX
  m_puiWedgePredTexTabIdx    = pcCU->getWedgePredTexTabIdx()     + uiPart;   
#if LGE_DMM3_SIMP_C0044
  m_puiWedgePredTexIntraTabIdx = pcCU->getWedgePredTexIntraTabIdx() + uiPart;   
#endif
  m_piWedgePredTexDeltaDC1   = pcCU->getWedgePredTexDeltaDC1()   + uiPart;   
  m_piWedgePredTexDeltaDC2   = pcCU->getWedgePredTexDeltaDC2()   + uiPart;   

  m_piContourPredTexDeltaDC1 = pcCU->getContourPredTexDeltaDC1() + uiPart;   
  m_piContourPredTexDeltaDC2 = pcCU->getContourPredTexDeltaDC2() + uiPart;   
#endif
#if LGE_EDGE_INTRA_A0070
  if( pcCU->getSlice()->getSPS()->isDepth() )
  {
    m_pucEdgeCode         = pcCU->getEdgeCode( uiPart );
    m_pucEdgeNumber       = pcCU->getEdgeNumber() + uiPart;
    m_pucEdgeStartPos     = pcCU->getEdgeStartPos() + uiPart;
    m_pbEdgeLeftFirst     = pcCU->getEdgeLeftFirst() + uiPart;
    m_pbEdgePartition     = pcCU->getEdgePartition( uiPart );
#if LGE_EDGE_INTRA_DELTA_DC
    m_piEdgeDeltaDC0      = pcCU->getEdgeDeltaDC0() + uiPart;
    m_piEdgeDeltaDC1      = pcCU->getEdgeDeltaDC1() + uiPart;
#endif
  }
#endif
#if RWTH_SDC_DLT_B0036
  m_pbSDCFlag               = pcCU->getSDCFlag()            + uiPart;
  m_apSegmentDCOffset[0]    = pcCU->getSDCSegmentDCOffset(0) + uiPart;
  m_apSegmentDCOffset[1]    = pcCU->getSDCSegmentDCOffset(1) + uiPart;
#endif
}

// Copy inter prediction info from the biggest CU
Void TComDataCU::copyInterPredInfoFrom    ( TComDataCU* pcCU, UInt uiAbsPartIdx, RefPicList eRefPicList )
{
  m_pcPic              = pcCU->getPic();
  m_pcSlice            = pcCU->getSlice();
  m_uiCUAddr           = pcCU->getAddr();
  m_uiAbsIdxInLCU      = uiAbsPartIdx;

  Int iRastPartIdx     = g_auiZscanToRaster[uiAbsPartIdx];
  m_uiCUPelX           = pcCU->getCUPelX() + m_pcPic->getMinCUWidth ()*( iRastPartIdx % m_pcPic->getNumPartInWidth() );
  m_uiCUPelY           = pcCU->getCUPelY() + m_pcPic->getMinCUHeight()*( iRastPartIdx / m_pcPic->getNumPartInWidth() );
  
  m_pcCUAboveLeft      = pcCU->getCUAboveLeft();
  m_pcCUAboveRight     = pcCU->getCUAboveRight();
  m_pcCUAbove          = pcCU->getCUAbove();
  m_pcCULeft           = pcCU->getCULeft();
  
  m_apcCUColocated[0]  = pcCU->getCUColocated(REF_PIC_LIST_0);
  m_apcCUColocated[1]  = pcCU->getCUColocated(REF_PIC_LIST_1);
  
  m_pePartSize         = pcCU->getPartitionSize ()        + uiAbsPartIdx;
#if HHI_INTERVIEW_SKIP
  m_pbRenderable         = pcCU->getRenderable()        + uiAbsPartIdx;

#endif
  m_pePredMode         = pcCU->getPredictionMode()        + uiAbsPartIdx;
  m_puhInterDir        = pcCU->getInterDir      ()        + uiAbsPartIdx;
  
  m_puhDepth           = pcCU->getDepth ()                + uiAbsPartIdx;
  m_puhWidth           = pcCU->getWidth ()                + uiAbsPartIdx;
  m_puhHeight          = pcCU->getHeight()                + uiAbsPartIdx;
  
  m_pbMergeFlag        = pcCU->getMergeFlag()             + uiAbsPartIdx;
#if LGE_ILLUCOMP_B0045
  m_pbICFlag           = pcCU->getICFlag()                + uiAbsPartIdx;
#endif
  m_puhMergeIndex      = pcCU->getMergeIndex()            + uiAbsPartIdx;
#if MERL_VSP_C0152
  m_piVSPIndex         = pcCU->getVSPIndex()              + uiAbsPartIdx;
#endif
#if H3D_IVRP
  m_pbResPredAvailable = pcCU->getResPredAvail()          + uiAbsPartIdx;
  m_pbResPredFlag      = pcCU->getResPredFlag ()          + uiAbsPartIdx;
#endif
  m_apiMVPIdx[eRefPicList] = pcCU->getMVPIdx(eRefPicList) + uiAbsPartIdx;
  m_apiMVPNum[eRefPicList] = pcCU->getMVPNum(eRefPicList) + uiAbsPartIdx;
  
  m_acCUMvField[ eRefPicList ].linkToWithOffset( pcCU->getCUMvField(eRefPicList), uiAbsPartIdx );
#if HHI_MPI
  m_piTextureModeDepth = pcCU->getTextureModeDepth() + uiAbsPartIdx;
#endif

  memcpy(m_uiSliceStartCU,pcCU->m_uiSliceStartCU+uiAbsPartIdx,sizeof(UInt)*m_uiNumPartition);
  memcpy(m_uiEntropySliceStartCU,pcCU->m_uiEntropySliceStartCU+uiAbsPartIdx,sizeof(UInt)*m_uiNumPartition);
}

// Copy small CU to bigger CU.
// One of quarter parts overwritten by predicted sub part.
Void TComDataCU::copyPartFrom( TComDataCU* pcCU, UInt uiPartUnitIdx, UInt uiDepth )
{
  assert( uiPartUnitIdx<4 );
  
  m_dTotalCost         += pcCU->getTotalCost();
  m_uiTotalDistortion  += pcCU->getTotalDistortion();
  m_uiTotalBits        += pcCU->getTotalBits();
  
  UInt uiOffset         = pcCU->getTotalNumPart()*uiPartUnitIdx;
  
  UInt uiNumPartition = pcCU->getTotalNumPart();
  Int iSizeInUchar  = sizeof( UChar ) * uiNumPartition;
  Int iSizeInBool   = sizeof( Bool  ) * uiNumPartition;
#if MERL_VSP_C0152
  Int iSizeInChar   = sizeof( Char )  * uiNumPartition;
#endif
  
  Int sizeInChar  = sizeof( Char ) * uiNumPartition;
  memcpy( m_phQP       + uiOffset, pcCU->getQP(),             sizeInChar                        );
  memcpy( m_pePartSize + uiOffset, pcCU->getPartitionSize(),  sizeof( *m_pePartSize ) * uiNumPartition );
  memcpy( m_pePredMode + uiOffset, pcCU->getPredictionMode(), sizeof( *m_pePredMode ) * uiNumPartition );
  #if HHI_INTERVIEW_SKIP
  memcpy( m_pbRenderable + uiOffset, pcCU->getRenderable(),         iSizeInBool  );
#endif
  memcpy( m_puiAlfCtrlFlag      + uiOffset, pcCU->getAlfCtrlFlag(),       iSizeInBool  );
  memcpy( m_pbMergeFlag         + uiOffset, pcCU->getMergeFlag(),         iSizeInBool  );
#if LGE_ILLUCOMP_B0045
  memcpy( m_pbICFlag            + uiOffset, pcCU->getICFlag(),            iSizeInBool );
#endif
  memcpy( m_puhMergeIndex       + uiOffset, pcCU->getMergeIndex(),        iSizeInUchar );
#if MERL_VSP_C0152
  memcpy( m_piVSPIndex          + uiOffset, pcCU->getVSPIndex(),          iSizeInChar );
#endif
#if H3D_IVRP
  memcpy( m_pbResPredAvailable  + uiOffset, pcCU->getResPredAvail(),      iSizeInBool  );
  memcpy( m_pbResPredFlag       + uiOffset, pcCU->getResPredFlag(),       iSizeInBool  );
#endif
  memcpy( m_puhLumaIntraDir     + uiOffset, pcCU->getLumaIntraDir(),      iSizeInUchar );
  memcpy( m_puhChromaIntraDir   + uiOffset, pcCU->getChromaIntraDir(),    iSizeInUchar );
  memcpy( m_puhInterDir         + uiOffset, pcCU->getInterDir(),          iSizeInUchar );
  memcpy( m_puhTrIdx            + uiOffset, pcCU->getTransformIdx(),      iSizeInUchar );
  memcpy( m_nsqtPartIdx         + uiOffset, pcCU->getNSQTPartIdx(),       iSizeInUchar );
  
  memcpy( m_puhCbf[0] + uiOffset, pcCU->getCbf(TEXT_LUMA)    , iSizeInUchar );
  memcpy( m_puhCbf[1] + uiOffset, pcCU->getCbf(TEXT_CHROMA_U), iSizeInUchar );
  memcpy( m_puhCbf[2] + uiOffset, pcCU->getCbf(TEXT_CHROMA_V), iSizeInUchar );
  
  memcpy( m_puhDepth  + uiOffset, pcCU->getDepth(),  iSizeInUchar );
  memcpy( m_puhWidth  + uiOffset, pcCU->getWidth(),  iSizeInUchar );
  memcpy( m_puhHeight + uiOffset, pcCU->getHeight(), iSizeInUchar );
  
  memcpy( m_apiMVPIdx[0] + uiOffset, pcCU->getMVPIdx(REF_PIC_LIST_0), iSizeInUchar );
  memcpy( m_apiMVPIdx[1] + uiOffset, pcCU->getMVPIdx(REF_PIC_LIST_1), iSizeInUchar );
  memcpy( m_apiMVPNum[0] + uiOffset, pcCU->getMVPNum(REF_PIC_LIST_0), iSizeInUchar );
  memcpy( m_apiMVPNum[1] + uiOffset, pcCU->getMVPNum(REF_PIC_LIST_1), iSizeInUchar );
  
  memcpy( m_pbIPCMFlag + uiOffset, pcCU->getIPCMFlag(), iSizeInBool );

  m_pcCUAboveLeft      = pcCU->getCUAboveLeft();
  m_pcCUAboveRight     = pcCU->getCUAboveRight();
  m_pcCUAbove          = pcCU->getCUAbove();
  m_pcCULeft           = pcCU->getCULeft();
  
  m_apcCUColocated[0] = pcCU->getCUColocated(REF_PIC_LIST_0);
  m_apcCUColocated[1] = pcCU->getCUColocated(REF_PIC_LIST_1);
  
  m_acCUMvField[0].copyFrom( pcCU->getCUMvField( REF_PIC_LIST_0 ), pcCU->getTotalNumPart(), uiOffset );
  m_acCUMvField[1].copyFrom( pcCU->getCUMvField( REF_PIC_LIST_1 ), pcCU->getTotalNumPart(), uiOffset );
  
  UInt uiTmp  = g_uiMaxCUWidth*g_uiMaxCUHeight >> (uiDepth<<1);
  UInt uiTmp2 = uiPartUnitIdx*uiTmp;
  memcpy( m_pcTrCoeffY  + uiTmp2, pcCU->getCoeffY(),  sizeof(TCoeff)*uiTmp );
#if ADAPTIVE_QP_SELECTION
  memcpy( m_pcArlCoeffY  + uiTmp2, pcCU->getArlCoeffY(),  sizeof(Int)*uiTmp );
#endif
  memcpy( m_pcIPCMSampleY + uiTmp2 , pcCU->getPCMSampleY(), sizeof(Pel) * uiTmp );

  uiTmp >>= 2; uiTmp2>>= 2;
  memcpy( m_pcTrCoeffCb + uiTmp2, pcCU->getCoeffCb(), sizeof(TCoeff)*uiTmp );
  memcpy( m_pcTrCoeffCr + uiTmp2, pcCU->getCoeffCr(), sizeof(TCoeff)*uiTmp );
#if ADAPTIVE_QP_SELECTION
  memcpy( m_pcArlCoeffCb + uiTmp2, pcCU->getArlCoeffCb(), sizeof(Int)*uiTmp );
  memcpy( m_pcArlCoeffCr + uiTmp2, pcCU->getArlCoeffCr(), sizeof(Int)*uiTmp );
#endif
  memcpy( m_pcIPCMSampleCb + uiTmp2 , pcCU->getPCMSampleCb(), sizeof(Pel) * uiTmp );
  memcpy( m_pcIPCMSampleCr + uiTmp2 , pcCU->getPCMSampleCr(), sizeof(Pel) * uiTmp );
  m_uiTotalBins += pcCU->getTotalBins();
  memcpy( m_uiSliceStartCU        + uiOffset, pcCU->m_uiSliceStartCU,        sizeof( UInt ) * uiNumPartition  );
  memcpy( m_uiEntropySliceStartCU + uiOffset, pcCU->m_uiEntropySliceStartCU, sizeof( UInt ) * uiNumPartition  );

#if HHI_DMM_WEDGE_INTRA
  memcpy( m_puiWedgeFullTabIdx       + uiOffset, pcCU->getWedgeFullTabIdx(),        sizeof( UInt ) * uiNumPartition );
  memcpy( m_piWedgeFullDeltaDC1      + uiOffset, pcCU->getWedgeFullDeltaDC1(),      sizeof( Int  ) * uiNumPartition );
  memcpy( m_piWedgeFullDeltaDC2      + uiOffset, pcCU->getWedgeFullDeltaDC2(),      sizeof( Int  ) * uiNumPartition );

  memcpy( m_puiWedgePredDirTabIdx    + uiOffset, pcCU->getWedgePredDirTabIdx(),     sizeof( UInt ) * uiNumPartition );
  memcpy( m_piWedgePredDirDeltaDC1   + uiOffset, pcCU->getWedgePredDirDeltaDC1(),   sizeof( Int  ) * uiNumPartition );
  memcpy( m_piWedgePredDirDeltaDC2   + uiOffset, pcCU->getWedgePredDirDeltaDC2(),   sizeof( Int  ) * uiNumPartition );
  memcpy( m_piWedgePredDirDeltaEnd   + uiOffset, pcCU->getWedgePredDirDeltaEnd(),   sizeof( Int  ) * uiNumPartition );
#endif
#if HHI_DMM_PRED_TEX
  memcpy( m_puiWedgePredTexTabIdx    + uiOffset, pcCU->getWedgePredTexTabIdx(),     sizeof( UInt ) * uiNumPartition );
#if LGE_DMM3_SIMP_C0044
  memcpy( m_puiWedgePredTexIntraTabIdx + uiOffset, pcCU->getWedgePredTexIntraTabIdx(), sizeof( UInt ) * uiNumPartition );
#endif
  memcpy( m_piWedgePredTexDeltaDC1   + uiOffset, pcCU->getWedgePredTexDeltaDC1(),   sizeof( Int  ) * uiNumPartition );
  memcpy( m_piWedgePredTexDeltaDC2   + uiOffset, pcCU->getWedgePredTexDeltaDC2(),   sizeof( Int  ) * uiNumPartition );

  memcpy( m_piContourPredTexDeltaDC1 + uiOffset, pcCU->getContourPredTexDeltaDC1(), sizeof( Int  ) * uiNumPartition );
  memcpy( m_piContourPredTexDeltaDC2 + uiOffset, pcCU->getContourPredTexDeltaDC2(), sizeof( Int  ) * uiNumPartition );
#endif

#if LGE_EDGE_INTRA_A0070
  if( getSlice()->getSPS()->isDepth() )
  {
    memcpy( getEdgeCode( uiOffset ), pcCU->getEdgeCode(0), iSizeInUchar * LGE_EDGE_INTRA_MAX_EDGE_NUM_PER_4x4 );
    memcpy( getEdgeNumber() + uiOffset, pcCU->getEdgeNumber(), iSizeInUchar );
    memcpy( getEdgeStartPos() + uiOffset, pcCU->getEdgeStartPos(), iSizeInUchar );
    memcpy( getEdgeLeftFirst() + uiOffset, pcCU->getEdgeLeftFirst(), iSizeInBool );
    memcpy( getEdgePartition( uiOffset ), pcCU->getEdgePartition(0), iSizeInBool * 16 );
#if LGE_EDGE_INTRA_DELTA_DC
    memcpy( getEdgeDeltaDC0() + uiOffset, pcCU->getEdgeDeltaDC0(), sizeof( Int  ) * uiNumPartition );
    memcpy( getEdgeDeltaDC1() + uiOffset, pcCU->getEdgeDeltaDC1(), sizeof( Int  ) * uiNumPartition );
#endif
  }
#endif

#if HHI_MPI
  memcpy( m_piTextureModeDepth + uiOffset, pcCU->getTextureModeDepth(), sizeof( Int ) * uiNumPartition );
#endif
#if RWTH_SDC_DLT_B0036
  memcpy( m_pbSDCFlag     + uiOffset, pcCU->getSDCFlag(),      iSizeInBool  );
  memcpy( m_apSegmentDCOffset[0]     + uiOffset, pcCU->getSDCSegmentDCOffset(0), sizeof( Pel ) * uiNumPartition);
  memcpy( m_apSegmentDCOffset[1]     + uiOffset, pcCU->getSDCSegmentDCOffset(1), sizeof( Pel ) * uiNumPartition);
#endif
}

// Copy current predicted part to a CU in picture.
// It is used to predict for next part
Void TComDataCU::copyToPic( UChar uhDepth )
{
  TComDataCU*& rpcCU = m_pcPic->getCU( m_uiCUAddr );
  
  rpcCU->getTotalCost()       = m_dTotalCost;
  rpcCU->getTotalDistortion() = m_uiTotalDistortion;
  rpcCU->getTotalBits()       = m_uiTotalBits;
  
  Int iSizeInUchar  = sizeof( UChar ) * m_uiNumPartition;
  Int iSizeInBool   = sizeof( Bool  ) * m_uiNumPartition;
#if MERL_VSP_C0152
  Int iSizeInChar   = sizeof( Char  ) * m_uiNumPartition;
#endif
  
  Int sizeInChar  = sizeof( Char ) * m_uiNumPartition;
  memcpy( rpcCU->getQP() + m_uiAbsIdxInLCU, m_phQP, sizeInChar  );

  memcpy( rpcCU->getPartitionSize()  + m_uiAbsIdxInLCU, m_pePartSize, sizeof( *m_pePartSize ) * m_uiNumPartition );
#if HHI_INTERVIEW_SKIP
  memcpy( rpcCU->getRenderable()         + m_uiAbsIdxInLCU, m_pbRenderable,         iSizeInBool  );
#endif
  memcpy( rpcCU->getPredictionMode() + m_uiAbsIdxInLCU, m_pePredMode, sizeof( *m_pePredMode ) * m_uiNumPartition );
  
  memcpy( rpcCU->getAlfCtrlFlag()    + m_uiAbsIdxInLCU, m_puiAlfCtrlFlag,    iSizeInBool  );
  
  memcpy( rpcCU->getMergeFlag()         + m_uiAbsIdxInLCU, m_pbMergeFlag,         iSizeInBool  );
#if LGE_ILLUCOMP_B0045
  memcpy( rpcCU->getICFlag()            + m_uiAbsIdxInLCU, m_pbICFlag,            iSizeInBool );
#endif
  memcpy( rpcCU->getMergeIndex()        + m_uiAbsIdxInLCU, m_puhMergeIndex,       iSizeInUchar );
#if MERL_VSP_C0152
  memcpy( rpcCU->getVSPIndex()          + m_uiAbsIdxInLCU, m_piVSPIndex,         iSizeInChar );
#endif
#if H3D_IVRP
  memcpy( rpcCU->getResPredAvail()      + m_uiAbsIdxInLCU, m_pbResPredAvailable,  iSizeInBool  );
  memcpy( rpcCU->getResPredFlag()       + m_uiAbsIdxInLCU, m_pbResPredFlag,       iSizeInBool  );
#endif
  memcpy( rpcCU->getLumaIntraDir()      + m_uiAbsIdxInLCU, m_puhLumaIntraDir,     iSizeInUchar );
  memcpy( rpcCU->getChromaIntraDir()    + m_uiAbsIdxInLCU, m_puhChromaIntraDir,   iSizeInUchar );
  memcpy( rpcCU->getInterDir()          + m_uiAbsIdxInLCU, m_puhInterDir,         iSizeInUchar );
  memcpy( rpcCU->getTransformIdx()      + m_uiAbsIdxInLCU, m_puhTrIdx,            iSizeInUchar );
  memcpy( rpcCU->getNSQTPartIdx()       + m_uiAbsIdxInLCU, m_nsqtPartIdx,         iSizeInUchar );
  
  memcpy( rpcCU->getCbf(TEXT_LUMA)     + m_uiAbsIdxInLCU, m_puhCbf[0], iSizeInUchar );
  memcpy( rpcCU->getCbf(TEXT_CHROMA_U) + m_uiAbsIdxInLCU, m_puhCbf[1], iSizeInUchar );
  memcpy( rpcCU->getCbf(TEXT_CHROMA_V) + m_uiAbsIdxInLCU, m_puhCbf[2], iSizeInUchar );
  
  memcpy( rpcCU->getDepth()  + m_uiAbsIdxInLCU, m_puhDepth,  iSizeInUchar );
  memcpy( rpcCU->getWidth()  + m_uiAbsIdxInLCU, m_puhWidth,  iSizeInUchar );
  memcpy( rpcCU->getHeight() + m_uiAbsIdxInLCU, m_puhHeight, iSizeInUchar );
  
  memcpy( rpcCU->getMVPIdx(REF_PIC_LIST_0) + m_uiAbsIdxInLCU, m_apiMVPIdx[0], iSizeInUchar );
  memcpy( rpcCU->getMVPIdx(REF_PIC_LIST_1) + m_uiAbsIdxInLCU, m_apiMVPIdx[1], iSizeInUchar );
  memcpy( rpcCU->getMVPNum(REF_PIC_LIST_0) + m_uiAbsIdxInLCU, m_apiMVPNum[0], iSizeInUchar );
  memcpy( rpcCU->getMVPNum(REF_PIC_LIST_1) + m_uiAbsIdxInLCU, m_apiMVPNum[1], iSizeInUchar );
  
  m_acCUMvField[0].copyTo( rpcCU->getCUMvField( REF_PIC_LIST_0 ), m_uiAbsIdxInLCU );
  m_acCUMvField[1].copyTo( rpcCU->getCUMvField( REF_PIC_LIST_1 ), m_uiAbsIdxInLCU );
  
  memcpy( rpcCU->getIPCMFlag() + m_uiAbsIdxInLCU, m_pbIPCMFlag,         iSizeInBool  );

  UInt uiTmp  = (g_uiMaxCUWidth*g_uiMaxCUHeight)>>(uhDepth<<1);
  UInt uiTmp2 = m_uiAbsIdxInLCU*m_pcPic->getMinCUWidth()*m_pcPic->getMinCUHeight();
  memcpy( rpcCU->getCoeffY()  + uiTmp2, m_pcTrCoeffY,  sizeof(TCoeff)*uiTmp  );
#if ADAPTIVE_QP_SELECTION  
  memcpy( rpcCU->getArlCoeffY()  + uiTmp2, m_pcArlCoeffY,  sizeof(Int)*uiTmp  );
#endif
  memcpy( rpcCU->getPCMSampleY() + uiTmp2 , m_pcIPCMSampleY, sizeof(Pel)*uiTmp );

  uiTmp >>= 2; uiTmp2 >>= 2;
  memcpy( rpcCU->getCoeffCb() + uiTmp2, m_pcTrCoeffCb, sizeof(TCoeff)*uiTmp  );
  memcpy( rpcCU->getCoeffCr() + uiTmp2, m_pcTrCoeffCr, sizeof(TCoeff)*uiTmp  );
#if ADAPTIVE_QP_SELECTION
  memcpy( rpcCU->getArlCoeffCb() + uiTmp2, m_pcArlCoeffCb, sizeof(Int)*uiTmp  );
  memcpy( rpcCU->getArlCoeffCr() + uiTmp2, m_pcArlCoeffCr, sizeof(Int)*uiTmp  );
#endif
  memcpy( rpcCU->getPCMSampleCb() + uiTmp2 , m_pcIPCMSampleCb, sizeof( Pel ) * uiTmp );
  memcpy( rpcCU->getPCMSampleCr() + uiTmp2 , m_pcIPCMSampleCr, sizeof( Pel ) * uiTmp );
  rpcCU->getTotalBins() = m_uiTotalBins;
  memcpy( rpcCU->m_uiSliceStartCU        + m_uiAbsIdxInLCU, m_uiSliceStartCU,        sizeof( UInt ) * m_uiNumPartition  );
  memcpy( rpcCU->m_uiEntropySliceStartCU + m_uiAbsIdxInLCU, m_uiEntropySliceStartCU, sizeof( UInt ) * m_uiNumPartition  );

#if HHI_DMM_WEDGE_INTRA
  memcpy( rpcCU->getWedgeFullTabIdx()        + m_uiAbsIdxInLCU, m_puiWedgeFullTabIdx,       sizeof( UInt ) * m_uiNumPartition );
  memcpy( rpcCU->getWedgeFullDeltaDC1()      + m_uiAbsIdxInLCU, m_piWedgeFullDeltaDC1,      sizeof( Int  ) * m_uiNumPartition );
  memcpy( rpcCU->getWedgeFullDeltaDC2()      + m_uiAbsIdxInLCU, m_piWedgeFullDeltaDC2,      sizeof( Int  ) * m_uiNumPartition );

  memcpy( rpcCU->getWedgePredDirTabIdx()     + m_uiAbsIdxInLCU, m_puiWedgePredDirTabIdx,    sizeof( UInt ) * m_uiNumPartition );
  memcpy( rpcCU->getWedgePredDirDeltaDC1()   + m_uiAbsIdxInLCU, m_piWedgePredDirDeltaDC1,   sizeof( Int  ) * m_uiNumPartition );
  memcpy( rpcCU->getWedgePredDirDeltaDC2()   + m_uiAbsIdxInLCU, m_piWedgePredDirDeltaDC2,   sizeof( Int  ) * m_uiNumPartition );
  memcpy( rpcCU->getWedgePredDirDeltaEnd()   + m_uiAbsIdxInLCU, m_piWedgePredDirDeltaEnd,   sizeof( Int  ) * m_uiNumPartition );
#endif
#if HHI_DMM_PRED_TEX
  memcpy( rpcCU->getWedgePredTexTabIdx()     + m_uiAbsIdxInLCU, m_puiWedgePredTexTabIdx,    sizeof( UInt ) * m_uiNumPartition );
#if LGE_DMM3_SIMP_C0044
  memcpy( rpcCU->getWedgePredTexIntraTabIdx() + m_uiAbsIdxInLCU, m_puiWedgePredTexIntraTabIdx,    sizeof( UInt ) * m_uiNumPartition );
#endif
  memcpy( rpcCU->getWedgePredTexDeltaDC1()   + m_uiAbsIdxInLCU, m_piWedgePredTexDeltaDC1,   sizeof( Int  ) * m_uiNumPartition );
  memcpy( rpcCU->getWedgePredTexDeltaDC2()   + m_uiAbsIdxInLCU, m_piWedgePredTexDeltaDC2,   sizeof( Int  ) * m_uiNumPartition );

  memcpy( rpcCU->getContourPredTexDeltaDC1() + m_uiAbsIdxInLCU, m_piContourPredTexDeltaDC1, sizeof( Int  ) * m_uiNumPartition );
  memcpy( rpcCU->getContourPredTexDeltaDC2() + m_uiAbsIdxInLCU, m_piContourPredTexDeltaDC2, sizeof( Int  ) * m_uiNumPartition );
#endif

#if LGE_EDGE_INTRA_A0070
  if( rpcCU->getSlice()->getSPS()->isDepth() )
  {
    memcpy( rpcCU->getEdgeCode( m_uiAbsIdxInLCU ),        m_pucEdgeCode,    iSizeInUchar * LGE_EDGE_INTRA_MAX_EDGE_NUM_PER_4x4 );
    memcpy( rpcCU->getEdgeNumber()    + m_uiAbsIdxInLCU,  m_pucEdgeNumber,  iSizeInUchar );
    memcpy( rpcCU->getEdgeStartPos()  + m_uiAbsIdxInLCU,  m_pucEdgeStartPos,iSizeInUchar );
    memcpy( rpcCU->getEdgeLeftFirst() + m_uiAbsIdxInLCU,  m_pbEdgeLeftFirst,iSizeInBool );
    memcpy( rpcCU->getEdgePartition( m_uiAbsIdxInLCU ),   m_pbEdgePartition,iSizeInBool * 16 );
#if LGE_EDGE_INTRA_DELTA_DC
    memcpy( rpcCU->getEdgeDeltaDC0() + m_uiAbsIdxInLCU,   m_piEdgeDeltaDC0, sizeof( Int  ) * m_uiNumPartition );
    memcpy( rpcCU->getEdgeDeltaDC1() + m_uiAbsIdxInLCU,   m_piEdgeDeltaDC1, sizeof( Int  ) * m_uiNumPartition );
#endif
  }
#endif

#if HHI_MPI
  memcpy( rpcCU->getTextureModeDepth() + m_uiAbsIdxInLCU, m_piTextureModeDepth, sizeof( Int ) * m_uiNumPartition );
#endif
#if RWTH_SDC_DLT_B0036
  memcpy( rpcCU->getSDCFlag() + m_uiAbsIdxInLCU, m_pbSDCFlag,      iSizeInBool  );
  memcpy( rpcCU->getSDCSegmentDCOffset(0) + m_uiAbsIdxInLCU, m_apSegmentDCOffset[0], sizeof( Pel ) * m_uiNumPartition);
  memcpy( rpcCU->getSDCSegmentDCOffset(1) + m_uiAbsIdxInLCU, m_apSegmentDCOffset[1], sizeof( Pel ) * m_uiNumPartition);
#endif
}

Void TComDataCU::copyToPic( UChar uhDepth, UInt uiPartIdx, UInt uiPartDepth )
{
  TComDataCU*&  rpcCU       = m_pcPic->getCU( m_uiCUAddr );
  UInt          uiQNumPart  = m_uiNumPartition>>(uiPartDepth<<1);
  
  UInt uiPartStart          = uiPartIdx*uiQNumPart;
  UInt uiPartOffset         = m_uiAbsIdxInLCU + uiPartStart;
  
  rpcCU->getTotalCost()       = m_dTotalCost;
  rpcCU->getTotalDistortion() = m_uiTotalDistortion;
  rpcCU->getTotalBits()       = m_uiTotalBits;
  
  Int iSizeInUchar  = sizeof( UChar  ) * uiQNumPart;
  Int iSizeInBool   = sizeof( Bool   ) * uiQNumPart;
#if MERL_VSP_C0152
  Int iSizeInChar   = sizeof( Char   ) * uiQNumPart;
#endif
  
  Int sizeInChar  = sizeof( Char ) * uiQNumPart;
  memcpy( rpcCU->getQP() + uiPartOffset, m_phQP, sizeInChar );
  memcpy( rpcCU->getPartitionSize()  + uiPartOffset, m_pePartSize, sizeof( *m_pePartSize ) * uiQNumPart );
#if HHI_INTERVIEW_SKIP
  memcpy( rpcCU->getRenderable()         + uiPartOffset, m_pbRenderable,         iSizeInBool  );
#endif
  memcpy( rpcCU->getPredictionMode() + uiPartOffset, m_pePredMode, sizeof( *m_pePredMode ) * uiQNumPart );
  
  memcpy( rpcCU->getAlfCtrlFlag()       + uiPartOffset, m_puiAlfCtrlFlag,      iSizeInBool  );
  memcpy( rpcCU->getMergeFlag()         + uiPartOffset, m_pbMergeFlag,         iSizeInBool  );
#if LGE_ILLUCOMP_B0045
  memcpy( rpcCU->getICFlag()            + uiPartOffset, m_pbICFlag,            iSizeInBool );
#endif
  memcpy( rpcCU->getMergeIndex()        + uiPartOffset, m_puhMergeIndex,       iSizeInUchar );
#if MERL_VSP_C0152
  memcpy( rpcCU->getVSPIndex()          + uiPartOffset, m_piVSPIndex,         iSizeInChar );
#endif
#if H3D_IVRP
  memcpy( rpcCU->getResPredAvail()      + uiPartOffset, m_pbResPredAvailable,  iSizeInBool  );
  memcpy( rpcCU->getResPredFlag()       + uiPartOffset, m_pbResPredFlag,       iSizeInBool  );
#endif
  memcpy( rpcCU->getLumaIntraDir()      + uiPartOffset, m_puhLumaIntraDir,     iSizeInUchar );
  memcpy( rpcCU->getChromaIntraDir()    + uiPartOffset, m_puhChromaIntraDir,   iSizeInUchar );
  memcpy( rpcCU->getInterDir()          + uiPartOffset, m_puhInterDir,         iSizeInUchar );
  memcpy( rpcCU->getTransformIdx()      + uiPartOffset, m_puhTrIdx,            iSizeInUchar );
  memcpy( rpcCU->getNSQTPartIdx()       + uiPartOffset, m_nsqtPartIdx,         iSizeInUchar );
  
  memcpy( rpcCU->getCbf(TEXT_LUMA)     + uiPartOffset, m_puhCbf[0], iSizeInUchar );
  memcpy( rpcCU->getCbf(TEXT_CHROMA_U) + uiPartOffset, m_puhCbf[1], iSizeInUchar );
  memcpy( rpcCU->getCbf(TEXT_CHROMA_V) + uiPartOffset, m_puhCbf[2], iSizeInUchar );
  
  memcpy( rpcCU->getDepth()  + uiPartOffset, m_puhDepth,  iSizeInUchar );
  memcpy( rpcCU->getWidth()  + uiPartOffset, m_puhWidth,  iSizeInUchar );
  memcpy( rpcCU->getHeight() + uiPartOffset, m_puhHeight, iSizeInUchar );
  
  memcpy( rpcCU->getMVPIdx(REF_PIC_LIST_0) + uiPartOffset, m_apiMVPIdx[0], iSizeInUchar );
  memcpy( rpcCU->getMVPIdx(REF_PIC_LIST_1) + uiPartOffset, m_apiMVPIdx[1], iSizeInUchar );
  memcpy( rpcCU->getMVPNum(REF_PIC_LIST_0) + uiPartOffset, m_apiMVPNum[0], iSizeInUchar );
  memcpy( rpcCU->getMVPNum(REF_PIC_LIST_1) + uiPartOffset, m_apiMVPNum[1], iSizeInUchar );
  m_acCUMvField[0].copyTo( rpcCU->getCUMvField( REF_PIC_LIST_0 ), m_uiAbsIdxInLCU, uiPartStart, uiQNumPart );
  m_acCUMvField[1].copyTo( rpcCU->getCUMvField( REF_PIC_LIST_1 ), m_uiAbsIdxInLCU, uiPartStart, uiQNumPart );
  
  memcpy( rpcCU->getIPCMFlag() + uiPartOffset, m_pbIPCMFlag,         iSizeInBool  );

  UInt uiTmp  = (g_uiMaxCUWidth*g_uiMaxCUHeight)>>((uhDepth+uiPartDepth)<<1);
  UInt uiTmp2 = uiPartOffset*m_pcPic->getMinCUWidth()*m_pcPic->getMinCUHeight();
  memcpy( rpcCU->getCoeffY()  + uiTmp2, m_pcTrCoeffY,  sizeof(TCoeff)*uiTmp  );
#if ADAPTIVE_QP_SELECTION
  memcpy( rpcCU->getArlCoeffY()  + uiTmp2, m_pcArlCoeffY,  sizeof(Int)*uiTmp  );
#endif
 
  memcpy( rpcCU->getPCMSampleY() + uiTmp2 , m_pcIPCMSampleY, sizeof( Pel ) * uiTmp );

  uiTmp >>= 2; uiTmp2 >>= 2;
  memcpy( rpcCU->getCoeffCb() + uiTmp2, m_pcTrCoeffCb, sizeof(TCoeff)*uiTmp  );
  memcpy( rpcCU->getCoeffCr() + uiTmp2, m_pcTrCoeffCr, sizeof(TCoeff)*uiTmp  );
#if ADAPTIVE_QP_SELECTION
  memcpy( rpcCU->getArlCoeffCb() + uiTmp2, m_pcArlCoeffCb, sizeof(Int)*uiTmp  );
  memcpy( rpcCU->getArlCoeffCr() + uiTmp2, m_pcArlCoeffCr, sizeof(Int)*uiTmp  );
#endif

  memcpy( rpcCU->getPCMSampleCb() + uiTmp2 , m_pcIPCMSampleCb, sizeof( Pel ) * uiTmp );
  memcpy( rpcCU->getPCMSampleCr() + uiTmp2 , m_pcIPCMSampleCr, sizeof( Pel ) * uiTmp );
  rpcCU->getTotalBins() = m_uiTotalBins;
  memcpy( rpcCU->m_uiSliceStartCU        + uiPartOffset, m_uiSliceStartCU,        sizeof( UInt ) * uiQNumPart  );
  memcpy( rpcCU->m_uiEntropySliceStartCU + uiPartOffset, m_uiEntropySliceStartCU, sizeof( UInt ) * uiQNumPart  );

#if HHI_DMM_WEDGE_INTRA
  memcpy( rpcCU->getWedgeFullTabIdx()        + uiPartOffset, m_puiWedgeFullTabIdx,       sizeof( UInt ) * uiQNumPart );
  memcpy( rpcCU->getWedgeFullDeltaDC1()      + uiPartOffset, m_piWedgeFullDeltaDC1,      sizeof( Int  ) * uiQNumPart );
  memcpy( rpcCU->getWedgeFullDeltaDC2()      + uiPartOffset, m_piWedgeFullDeltaDC2,      sizeof( Int  ) * uiQNumPart );

  memcpy( rpcCU->getWedgePredDirTabIdx()     + uiPartOffset, m_puiWedgePredDirTabIdx,    sizeof( UInt ) * uiQNumPart );
  memcpy( rpcCU->getWedgePredDirDeltaDC1()   + uiPartOffset, m_piWedgePredDirDeltaDC1,   sizeof( Int  ) * uiQNumPart );
  memcpy( rpcCU->getWedgePredDirDeltaDC2()   + uiPartOffset, m_piWedgePredDirDeltaDC2,   sizeof( Int  ) * uiQNumPart );
  memcpy( rpcCU->getWedgePredDirDeltaEnd()   + uiPartOffset, m_piWedgePredDirDeltaEnd,   sizeof( Int  ) * uiQNumPart );
#endif
#if HHI_DMM_PRED_TEX
  memcpy( rpcCU->getWedgePredTexTabIdx()     + uiPartOffset, m_puiWedgePredTexTabIdx,    sizeof( UInt ) * uiQNumPart );
#if LGE_DMM3_SIMP_C0044
  memcpy( rpcCU->getWedgePredTexIntraTabIdx() + uiPartOffset, m_puiWedgePredTexIntraTabIdx, sizeof( UInt ) * uiQNumPart );
#endif
  memcpy( rpcCU->getWedgePredTexDeltaDC1()   + uiPartOffset, m_piWedgePredTexDeltaDC1,   sizeof( Int  ) * uiQNumPart );
  memcpy( rpcCU->getWedgePredTexDeltaDC2()   + uiPartOffset, m_piWedgePredTexDeltaDC2,   sizeof( Int  ) * uiQNumPart );

  memcpy( rpcCU->getContourPredTexDeltaDC1() + uiPartOffset, m_piContourPredTexDeltaDC1, sizeof( Int  ) * uiQNumPart );
  memcpy( rpcCU->getContourPredTexDeltaDC2() + uiPartOffset, m_piContourPredTexDeltaDC2, sizeof( Int  ) * uiQNumPart );
#endif

#if LGE_EDGE_INTRA_A0070
  if( rpcCU->getSlice()->getSPS()->isDepth() )
  {
    memcpy( rpcCU->getEdgeCode( uiPartOffset ),       m_pucEdgeCode,    iSizeInUchar * LGE_EDGE_INTRA_MAX_EDGE_NUM_PER_4x4 );
    memcpy( rpcCU->getEdgeNumber()    + uiPartOffset, m_pucEdgeNumber,  iSizeInUchar );
    memcpy( rpcCU->getEdgeStartPos()  + uiPartOffset, m_pucEdgeStartPos,iSizeInUchar );
    memcpy( rpcCU->getEdgeLeftFirst() + uiPartOffset, m_pbEdgeLeftFirst,iSizeInBool );
    memcpy( rpcCU->getEdgePartition( uiPartOffset ),  m_pbEdgePartition,iSizeInBool * 16 );
#if LGE_EDGE_INTRA_DELTA_DC
    memcpy( rpcCU->getEdgeDeltaDC0() + uiPartOffset,  m_piEdgeDeltaDC0, sizeof( Int  ) * uiQNumPart  );
    memcpy( rpcCU->getEdgeDeltaDC1() + uiPartOffset,  m_piEdgeDeltaDC1, sizeof( Int  ) * uiQNumPart  );
#endif
  }
#endif

#if HHI_MPI
  memcpy( rpcCU->getTextureModeDepth() + uiPartOffset, m_piTextureModeDepth, sizeof( Int ) * uiQNumPart  );
#endif
#if RWTH_SDC_DLT_B0036
  memcpy( rpcCU->getSDCFlag() + uiPartOffset, m_pbSDCFlag,      iSizeInBool  );
  memcpy( rpcCU->getSDCSegmentDCOffset(0) + uiPartOffset, m_apSegmentDCOffset[0], sizeof( Pel ) * uiQNumPart);
  memcpy( rpcCU->getSDCSegmentDCOffset(1) + uiPartOffset, m_apSegmentDCOffset[1], sizeof( Pel ) * uiQNumPart);
#endif
}

// --------------------------------------------------------------------------------------------------------------------
// Other public functions
// --------------------------------------------------------------------------------------------------------------------

TComDataCU* TComDataCU::getPULeft( UInt& uiLPartUnitIdx, UInt uiCurrPartUnitIdx, Bool bEnforceSliceRestriction, Bool bEnforceEntropySliceRestriction
                                 , Bool bEnforceTileRestriction
                                 )
{
  UInt uiAbsPartIdx       = g_auiZscanToRaster[uiCurrPartUnitIdx];
  UInt uiAbsZorderCUIdx   = g_auiZscanToRaster[m_uiAbsIdxInLCU];
  UInt uiNumPartInCUWidth = m_pcPic->getNumPartInWidth();
  
  if ( !RasterAddress::isZeroCol( uiAbsPartIdx, uiNumPartInCUWidth ) )
  {
    uiLPartUnitIdx = g_auiRasterToZscan[ uiAbsPartIdx - 1 ];
    if ( RasterAddress::isEqualCol( uiAbsPartIdx, uiAbsZorderCUIdx, uiNumPartInCUWidth ) )
    {
      TComDataCU* pcTempReconCU = m_pcPic->getCU( getAddr() );
      if ((bEnforceSliceRestriction        && (pcTempReconCU==NULL || pcTempReconCU->getSlice() == NULL || pcTempReconCU->getSCUAddr()+uiLPartUnitIdx < m_pcPic->getCU( getAddr() )->getSliceStartCU       (uiCurrPartUnitIdx)))
        ||(bEnforceEntropySliceRestriction && (pcTempReconCU==NULL || pcTempReconCU->getSlice() == NULL || pcTempReconCU->getSCUAddr()+uiLPartUnitIdx < m_pcPic->getCU( getAddr() )->getEntropySliceStartCU(uiCurrPartUnitIdx))))
      {
        return NULL;
      }
      return m_pcPic->getCU( getAddr() );
    }
    else
    {
      uiLPartUnitIdx -= m_uiAbsIdxInLCU;
      if ((bEnforceSliceRestriction        && (this->getSCUAddr()+uiLPartUnitIdx < m_pcPic->getCU( getAddr() )->getSliceStartCU       (uiCurrPartUnitIdx) || m_pcSlice==NULL))
        ||(bEnforceEntropySliceRestriction && (this->getSCUAddr()+uiLPartUnitIdx < m_pcPic->getCU( getAddr() )->getEntropySliceStartCU(uiCurrPartUnitIdx) || m_pcSlice==NULL)))
      {
        return NULL;
      }
      return this;
    }
  }
  
  uiLPartUnitIdx = g_auiRasterToZscan[ uiAbsPartIdx + uiNumPartInCUWidth - 1 ];


  if ( (bEnforceSliceRestriction && (m_pcCULeft==NULL || m_pcCULeft->getSlice()==NULL || m_pcCULeft->getSCUAddr()+uiLPartUnitIdx < m_pcPic->getCU( getAddr() )->getSliceStartCU(uiCurrPartUnitIdx)))
      ||
       (bEnforceEntropySliceRestriction && (m_pcCULeft==NULL || m_pcCULeft->getSlice()==NULL || m_pcCULeft->getSCUAddr()+uiLPartUnitIdx < m_pcPic->getCU( getAddr() )->getEntropySliceStartCU(uiCurrPartUnitIdx)))
      ||
       (bEnforceTileRestriction && ( m_pcCULeft==NULL || m_pcCULeft->getSlice()==NULL || (m_pcPic->getPicSym()->getTileIdxMap( m_pcCULeft->getAddr() ) != m_pcPic->getPicSym()->getTileIdxMap(getAddr()))  )  )
      )
  {
    return NULL;
  }
  return m_pcCULeft;
}


TComDataCU* TComDataCU::getPUAbove( UInt& uiAPartUnitIdx, UInt uiCurrPartUnitIdx, Bool bEnforceSliceRestriction, Bool bEnforceEntropySliceRestriction, Bool MotionDataCompresssion
                                  , Bool planarAtLCUBoundary 
                                  , Bool bEnforceTileRestriction 
                                  )
{
  UInt uiAbsPartIdx       = g_auiZscanToRaster[uiCurrPartUnitIdx];
  UInt uiAbsZorderCUIdx   = g_auiZscanToRaster[m_uiAbsIdxInLCU];
  UInt uiNumPartInCUWidth = m_pcPic->getNumPartInWidth();
  
  if ( !RasterAddress::isZeroRow( uiAbsPartIdx, uiNumPartInCUWidth ) )
  {
    uiAPartUnitIdx = g_auiRasterToZscan[ uiAbsPartIdx - uiNumPartInCUWidth ];
    if ( RasterAddress::isEqualRow( uiAbsPartIdx, uiAbsZorderCUIdx, uiNumPartInCUWidth ) )
    {
      TComDataCU* pcTempReconCU = m_pcPic->getCU( getAddr() );
      if ((bEnforceSliceRestriction        && (pcTempReconCU==NULL || pcTempReconCU->getSlice() == NULL || pcTempReconCU->getSCUAddr()+uiAPartUnitIdx < m_pcPic->getCU( getAddr() )->getSliceStartCU       (uiCurrPartUnitIdx)))
        ||(bEnforceEntropySliceRestriction && (pcTempReconCU==NULL || pcTempReconCU->getSlice() == NULL || pcTempReconCU->getSCUAddr()+uiAPartUnitIdx < m_pcPic->getCU( getAddr() )->getEntropySliceStartCU(uiCurrPartUnitIdx))))
      {
        return NULL;
      }
      return m_pcPic->getCU( getAddr() );
    }
    else
    {
      uiAPartUnitIdx -= m_uiAbsIdxInLCU;
      if ((bEnforceSliceRestriction        && (this->getSCUAddr()+uiAPartUnitIdx < m_pcPic->getCU( getAddr() )->getSliceStartCU       (uiCurrPartUnitIdx) || m_pcSlice==NULL))
        ||(bEnforceEntropySliceRestriction && (this->getSCUAddr()+uiAPartUnitIdx < m_pcPic->getCU( getAddr() )->getEntropySliceStartCU(uiCurrPartUnitIdx) || m_pcSlice==NULL)))
      {
        return NULL;
      }
      return this;
    }
  }

  if(planarAtLCUBoundary)
  {
    return NULL;
  }
  
  uiAPartUnitIdx = g_auiRasterToZscan[ uiAbsPartIdx + m_pcPic->getNumPartInCU() - uiNumPartInCUWidth ];
  if(MotionDataCompresssion)
  {
    uiAPartUnitIdx = g_motionRefer[uiAPartUnitIdx];
  }

  if ( (bEnforceSliceRestriction && (m_pcCUAbove==NULL || m_pcCUAbove->getSlice()==NULL || m_pcCUAbove->getSCUAddr()+uiAPartUnitIdx < m_pcPic->getCU( getAddr() )->getSliceStartCU(uiCurrPartUnitIdx)))
      ||
       (bEnforceEntropySliceRestriction && (m_pcCUAbove==NULL || m_pcCUAbove->getSlice()==NULL || m_pcCUAbove->getSCUAddr()+uiAPartUnitIdx < m_pcPic->getCU( getAddr() )->getEntropySliceStartCU(uiCurrPartUnitIdx)))
      ||
       (bEnforceTileRestriction &&(m_pcCUAbove==NULL || m_pcCUAbove->getSlice()==NULL || (m_pcPic->getPicSym()->getTileIdxMap( m_pcCUAbove->getAddr() ) != m_pcPic->getPicSym()->getTileIdxMap(getAddr()))))
      )
  {
    return NULL;
  }
  return m_pcCUAbove;
}

TComDataCU* TComDataCU::getPUAboveLeft( UInt& uiALPartUnitIdx, UInt uiCurrPartUnitIdx, Bool bEnforceSliceRestriction, Bool bEnforceEntropySliceRestriction, Bool MotionDataCompresssion )
{
  UInt uiAbsPartIdx       = g_auiZscanToRaster[uiCurrPartUnitIdx];
  UInt uiAbsZorderCUIdx   = g_auiZscanToRaster[m_uiAbsIdxInLCU];
  UInt uiNumPartInCUWidth = m_pcPic->getNumPartInWidth();
  
  if ( !RasterAddress::isZeroCol( uiAbsPartIdx, uiNumPartInCUWidth ) )
  {
    if ( !RasterAddress::isZeroRow( uiAbsPartIdx, uiNumPartInCUWidth ) )
    {
      uiALPartUnitIdx = g_auiRasterToZscan[ uiAbsPartIdx - uiNumPartInCUWidth - 1 ];
      if ( RasterAddress::isEqualRowOrCol( uiAbsPartIdx, uiAbsZorderCUIdx, uiNumPartInCUWidth ) )
      {
        TComDataCU* pcTempReconCU = m_pcPic->getCU( getAddr() );
        if ((bEnforceSliceRestriction        && (pcTempReconCU==NULL || pcTempReconCU->getSlice() == NULL || pcTempReconCU->getSCUAddr()+uiALPartUnitIdx < m_pcPic->getCU( getAddr() )->getSliceStartCU       (uiCurrPartUnitIdx)))
          ||(bEnforceEntropySliceRestriction && (pcTempReconCU==NULL || pcTempReconCU->getSlice() == NULL || pcTempReconCU->getSCUAddr()+uiALPartUnitIdx < m_pcPic->getCU( getAddr() )->getEntropySliceStartCU(uiCurrPartUnitIdx)))) 
        {
          return NULL;
        }
        return m_pcPic->getCU( getAddr() );
      }
      else
      {
        uiALPartUnitIdx -= m_uiAbsIdxInLCU;
        if ((bEnforceSliceRestriction        && (this->getSCUAddr()+uiALPartUnitIdx < m_pcPic->getCU( getAddr() )->getSliceStartCU       (uiCurrPartUnitIdx) || m_pcSlice==NULL))
          ||(bEnforceEntropySliceRestriction && (this->getSCUAddr()+uiALPartUnitIdx < m_pcPic->getCU( getAddr() )->getEntropySliceStartCU(uiCurrPartUnitIdx) || m_pcSlice==NULL)))
        {
          return NULL;
        }
        return this;
      }
    }
    uiALPartUnitIdx = g_auiRasterToZscan[ uiAbsPartIdx + getPic()->getNumPartInCU() - uiNumPartInCUWidth - 1 ];
    if(MotionDataCompresssion)
    {
      uiALPartUnitIdx = g_motionRefer[uiALPartUnitIdx];
    }
  if ( (bEnforceSliceRestriction && (m_pcCUAbove==NULL || m_pcCUAbove->getSlice()==NULL || 
       m_pcCUAbove->getSCUAddr()+uiALPartUnitIdx < m_pcPic->getCU( getAddr() )->getSliceStartCU(uiCurrPartUnitIdx)||
       (m_pcPic->getPicSym()->getTileIdxMap( m_pcCUAbove->getAddr() ) != m_pcPic->getPicSym()->getTileIdxMap(getAddr()))
       ))||
       (bEnforceEntropySliceRestriction && (m_pcCUAbove==NULL || m_pcCUAbove->getSlice()==NULL || 
       m_pcCUAbove->getSCUAddr()+uiALPartUnitIdx < m_pcPic->getCU( getAddr() )->getEntropySliceStartCU(uiCurrPartUnitIdx)||
       (m_pcPic->getPicSym()->getTileIdxMap( m_pcCUAbove->getAddr() ) != m_pcPic->getPicSym()->getTileIdxMap(getAddr()))
       ))
     )
    {
      return NULL;
    }
    return m_pcCUAbove;
  }
  
  if ( !RasterAddress::isZeroRow( uiAbsPartIdx, uiNumPartInCUWidth ) )
  {
    uiALPartUnitIdx = g_auiRasterToZscan[ uiAbsPartIdx - 1 ];
  if ( (bEnforceSliceRestriction && (m_pcCULeft==NULL || m_pcCULeft->getSlice()==NULL || 
       m_pcCULeft->getSCUAddr()+uiALPartUnitIdx < m_pcPic->getCU( getAddr() )->getSliceStartCU(uiCurrPartUnitIdx)||
       (m_pcPic->getPicSym()->getTileIdxMap( m_pcCULeft->getAddr() ) != m_pcPic->getPicSym()->getTileIdxMap(getAddr()))
       ))||
       (bEnforceEntropySliceRestriction && (m_pcCULeft==NULL || m_pcCULeft->getSlice()==NULL || 
       m_pcCULeft->getSCUAddr()+uiALPartUnitIdx < m_pcPic->getCU( getAddr() )->getEntropySliceStartCU(uiCurrPartUnitIdx)||
       (m_pcPic->getPicSym()->getTileIdxMap( m_pcCULeft->getAddr() ) != m_pcPic->getPicSym()->getTileIdxMap(getAddr()))
       ))
     )
    {
      return NULL;
    }
    return m_pcCULeft;
  }
  
  uiALPartUnitIdx = g_auiRasterToZscan[ m_pcPic->getNumPartInCU() - 1 ];
  if(MotionDataCompresssion)
  {
    uiALPartUnitIdx = g_motionRefer[uiALPartUnitIdx];
  }
  if ( (bEnforceSliceRestriction && (m_pcCUAboveLeft==NULL || m_pcCUAboveLeft->getSlice()==NULL || 
       m_pcCUAboveLeft->getSCUAddr()+uiALPartUnitIdx < m_pcPic->getCU( getAddr() )->getSliceStartCU(uiCurrPartUnitIdx)||
       (m_pcPic->getPicSym()->getTileIdxMap( m_pcCUAboveLeft->getAddr() ) != m_pcPic->getPicSym()->getTileIdxMap(getAddr()))
       ))||
       (bEnforceEntropySliceRestriction && (m_pcCUAboveLeft==NULL || m_pcCUAboveLeft->getSlice()==NULL || 
       m_pcCUAboveLeft->getSCUAddr()+uiALPartUnitIdx < m_pcPic->getCU( getAddr() )->getEntropySliceStartCU(uiCurrPartUnitIdx)||
       (m_pcPic->getPicSym()->getTileIdxMap( m_pcCUAboveLeft->getAddr() ) != m_pcPic->getPicSym()->getTileIdxMap(getAddr()))
       ))
     )
  {
    return NULL;
  }
  return m_pcCUAboveLeft;
}

TComDataCU* TComDataCU::getPUAboveRight( UInt& uiARPartUnitIdx, UInt uiCurrPartUnitIdx, Bool bEnforceSliceRestriction, Bool bEnforceEntropySliceRestriction, Bool MotionDataCompresssion )
{
  UInt uiAbsPartIdxRT     = g_auiZscanToRaster[uiCurrPartUnitIdx];
  UInt uiAbsZorderCUIdx   = g_auiZscanToRaster[ m_uiAbsIdxInLCU ] + m_puhWidth[0] / m_pcPic->getMinCUWidth() - 1;
  UInt uiNumPartInCUWidth = m_pcPic->getNumPartInWidth();
  
  if( ( m_pcPic->getCU(m_uiCUAddr)->getCUPelX() + g_auiRasterToPelX[uiAbsPartIdxRT] + m_pcPic->getMinCUWidth() ) >= m_pcSlice->getSPS()->getPicWidthInLumaSamples() )
  {
    uiARPartUnitIdx = MAX_UINT;
    return NULL;
  }
  
  if ( RasterAddress::lessThanCol( uiAbsPartIdxRT, uiNumPartInCUWidth - 1, uiNumPartInCUWidth ) )
  {
    if ( !RasterAddress::isZeroRow( uiAbsPartIdxRT, uiNumPartInCUWidth ) )
    {
      if ( uiCurrPartUnitIdx > g_auiRasterToZscan[ uiAbsPartIdxRT - uiNumPartInCUWidth + 1 ] )
      {
        uiARPartUnitIdx = g_auiRasterToZscan[ uiAbsPartIdxRT - uiNumPartInCUWidth + 1 ];
        if ( RasterAddress::isEqualRowOrCol( uiAbsPartIdxRT, uiAbsZorderCUIdx, uiNumPartInCUWidth ) )
        {
          TComDataCU* pcTempReconCU = m_pcPic->getCU( getAddr() );
          if ((bEnforceSliceRestriction        && (pcTempReconCU==NULL || pcTempReconCU->getSlice() == NULL || pcTempReconCU->getSCUAddr()+uiARPartUnitIdx < m_pcPic->getCU( getAddr() )->getSliceStartCU       (uiCurrPartUnitIdx)))
            ||(bEnforceEntropySliceRestriction && (pcTempReconCU==NULL || pcTempReconCU->getSlice() == NULL || pcTempReconCU->getSCUAddr()+uiARPartUnitIdx < m_pcPic->getCU( getAddr() )->getEntropySliceStartCU(uiCurrPartUnitIdx))))
          {
            return NULL;
          }
          return m_pcPic->getCU( getAddr() );
        }
        else
        {
          uiARPartUnitIdx -= m_uiAbsIdxInLCU;
          if ((bEnforceSliceRestriction        && (this->getSCUAddr()+uiARPartUnitIdx < m_pcPic->getCU( getAddr() )->getSliceStartCU       (uiCurrPartUnitIdx) || m_pcSlice==NULL))
            ||(bEnforceEntropySliceRestriction && (this->getSCUAddr()+uiARPartUnitIdx < m_pcPic->getCU( getAddr() )->getEntropySliceStartCU(uiCurrPartUnitIdx) || m_pcSlice==NULL)))
          {
            return NULL;
          }
          return this;
        }
      }
      uiARPartUnitIdx = MAX_UINT;
      return NULL;
    }
    uiARPartUnitIdx = g_auiRasterToZscan[ uiAbsPartIdxRT + m_pcPic->getNumPartInCU() - uiNumPartInCUWidth + 1 ];
    if(MotionDataCompresssion)
    {
      uiARPartUnitIdx = g_motionRefer[uiARPartUnitIdx];
    }

  if ( (bEnforceSliceRestriction && (m_pcCUAbove==NULL || m_pcCUAbove->getSlice()==NULL || 
       m_pcCUAbove->getSCUAddr()+uiARPartUnitIdx < m_pcPic->getCU( getAddr() )->getSliceStartCU(uiCurrPartUnitIdx)||
       (m_pcPic->getPicSym()->getTileIdxMap( m_pcCUAbove->getAddr() ) != m_pcPic->getPicSym()->getTileIdxMap(getAddr()))
       ))||
       (bEnforceEntropySliceRestriction && (m_pcCUAbove==NULL || m_pcCUAbove->getSlice()==NULL || 
       m_pcCUAbove->getSCUAddr()+uiARPartUnitIdx < m_pcPic->getCU( getAddr() )->getEntropySliceStartCU(uiCurrPartUnitIdx)||
       (m_pcPic->getPicSym()->getTileIdxMap( m_pcCUAbove->getAddr() ) != m_pcPic->getPicSym()->getTileIdxMap(getAddr()))
       ))
     )
    {
      return NULL;
    }
    return m_pcCUAbove;
  }
  
  if ( !RasterAddress::isZeroRow( uiAbsPartIdxRT, uiNumPartInCUWidth ) )
  {
    uiARPartUnitIdx = MAX_UINT;
    return NULL;
  }
  
  uiARPartUnitIdx = g_auiRasterToZscan[ m_pcPic->getNumPartInCU() - uiNumPartInCUWidth ];
  if(MotionDataCompresssion)
  {
    uiARPartUnitIdx = g_motionRefer[uiARPartUnitIdx];
  }
  if ( (bEnforceSliceRestriction && (m_pcCUAboveRight==NULL || m_pcCUAboveRight->getSlice()==NULL ||
       m_pcPic->getPicSym()->getInverseCUOrderMap( m_pcCUAboveRight->getAddr()) > m_pcPic->getPicSym()->getInverseCUOrderMap( getAddr()) ||
       m_pcCUAboveRight->getSCUAddr()+uiARPartUnitIdx < m_pcPic->getCU( getAddr() )->getSliceStartCU(uiCurrPartUnitIdx)||
       (m_pcPic->getPicSym()->getTileIdxMap( m_pcCUAboveRight->getAddr() ) != m_pcPic->getPicSym()->getTileIdxMap(getAddr()))
       ))||
       (bEnforceEntropySliceRestriction && (m_pcCUAboveRight==NULL || m_pcCUAboveRight->getSlice()==NULL || 
       m_pcPic->getPicSym()->getInverseCUOrderMap( m_pcCUAboveRight->getAddr()) > m_pcPic->getPicSym()->getInverseCUOrderMap( getAddr()) ||
       m_pcCUAboveRight->getSCUAddr()+uiARPartUnitIdx < m_pcPic->getCU( getAddr() )->getEntropySliceStartCU(uiCurrPartUnitIdx)||
       (m_pcPic->getPicSym()->getTileIdxMap( m_pcCUAboveRight->getAddr() ) != m_pcPic->getPicSym()->getTileIdxMap(getAddr()))
       ))
     )
  {
    return NULL;
  }
  return m_pcCUAboveRight;
}

TComDataCU* TComDataCU::getPUBelowLeft( UInt& uiBLPartUnitIdx, UInt uiCurrPartUnitIdx, Bool bEnforceSliceRestriction, Bool bEnforceEntropySliceRestriction )
{
  UInt uiAbsPartIdxLB     = g_auiZscanToRaster[uiCurrPartUnitIdx];
  UInt uiAbsZorderCUIdxLB = g_auiZscanToRaster[ m_uiAbsIdxInLCU ] + (m_puhHeight[0] / m_pcPic->getMinCUHeight() - 1)*m_pcPic->getNumPartInWidth();
  UInt uiNumPartInCUWidth = m_pcPic->getNumPartInWidth();
  
  if( ( m_pcPic->getCU(m_uiCUAddr)->getCUPelY() + g_auiRasterToPelY[uiAbsPartIdxLB] + m_pcPic->getMinCUHeight() ) >= m_pcSlice->getSPS()->getPicHeightInLumaSamples() )
  {
    uiBLPartUnitIdx = MAX_UINT;
    return NULL;
  }
  
  if ( RasterAddress::lessThanRow( uiAbsPartIdxLB, m_pcPic->getNumPartInHeight() - 1, uiNumPartInCUWidth ) )
  {
    if ( !RasterAddress::isZeroCol( uiAbsPartIdxLB, uiNumPartInCUWidth ) )
    {
      if ( uiCurrPartUnitIdx > g_auiRasterToZscan[ uiAbsPartIdxLB + uiNumPartInCUWidth - 1 ] )
      {
        uiBLPartUnitIdx = g_auiRasterToZscan[ uiAbsPartIdxLB + uiNumPartInCUWidth - 1 ];
        if ( RasterAddress::isEqualRowOrCol( uiAbsPartIdxLB, uiAbsZorderCUIdxLB, uiNumPartInCUWidth ) )
        {
          TComDataCU* pcTempReconCU = m_pcPic->getCU( getAddr() );
          if ((bEnforceSliceRestriction        && (pcTempReconCU==NULL || pcTempReconCU->getSlice() == NULL || pcTempReconCU->getSCUAddr()+uiBLPartUnitIdx < m_pcPic->getCU( getAddr() )->getSliceStartCU       (uiCurrPartUnitIdx)))
            ||(bEnforceEntropySliceRestriction && (pcTempReconCU==NULL || pcTempReconCU->getSlice() == NULL || pcTempReconCU->getSCUAddr()+uiBLPartUnitIdx < m_pcPic->getCU( getAddr() )->getEntropySliceStartCU(uiCurrPartUnitIdx))))
          {
            return NULL;
          }
          return m_pcPic->getCU( getAddr() );
        }
        else
        {
          uiBLPartUnitIdx -= m_uiAbsIdxInLCU;
          if ((bEnforceSliceRestriction        && (this->getSCUAddr()+uiBLPartUnitIdx < m_pcPic->getCU( getAddr() )->getSliceStartCU       (uiCurrPartUnitIdx) || m_pcSlice==NULL))
            ||(bEnforceEntropySliceRestriction && (this->getSCUAddr()+uiBLPartUnitIdx < m_pcPic->getCU( getAddr() )->getEntropySliceStartCU(uiCurrPartUnitIdx) || m_pcSlice==NULL)))
          {
            return NULL;
          }
          return this;
        }
      }
      uiBLPartUnitIdx = MAX_UINT;
      return NULL;
    }
    uiBLPartUnitIdx = g_auiRasterToZscan[ uiAbsPartIdxLB + uiNumPartInCUWidth*2 - 1 ];
  if ( (bEnforceSliceRestriction && (m_pcCULeft==NULL || m_pcCULeft->getSlice()==NULL || 
       m_pcCULeft->getSCUAddr()+uiBLPartUnitIdx < m_pcPic->getCU( getAddr() )->getSliceStartCU(uiCurrPartUnitIdx)||
       (m_pcPic->getPicSym()->getTileIdxMap( m_pcCULeft->getAddr() ) != m_pcPic->getPicSym()->getTileIdxMap(getAddr()))
       ))||
       (bEnforceEntropySliceRestriction && (m_pcCULeft==NULL || m_pcCULeft->getSlice()==NULL || 
       m_pcCULeft->getSCUAddr()+uiBLPartUnitIdx < m_pcPic->getCU( getAddr() )->getEntropySliceStartCU(uiCurrPartUnitIdx)||
       (m_pcPic->getPicSym()->getTileIdxMap( m_pcCULeft->getAddr() ) != m_pcPic->getPicSym()->getTileIdxMap(getAddr()))
       ))
     )
    {
      return NULL;
    }
    return m_pcCULeft;
  }
  
  uiBLPartUnitIdx = MAX_UINT;
  return NULL;
}

TComDataCU* TComDataCU::getPUBelowLeftAdi(UInt& uiBLPartUnitIdx, UInt uiPuHeight,  UInt uiCurrPartUnitIdx, UInt uiPartUnitOffset, Bool bEnforceSliceRestriction, Bool bEnforceEntropySliceRestriction )
{
  UInt uiAbsPartIdxLB     = g_auiZscanToRaster[uiCurrPartUnitIdx];
  UInt uiAbsZorderCUIdxLB = g_auiZscanToRaster[ m_uiAbsIdxInLCU ] + ((m_puhHeight[0] / m_pcPic->getMinCUHeight()) - 1)*m_pcPic->getNumPartInWidth();
  UInt uiNumPartInCUWidth = m_pcPic->getNumPartInWidth();
  
  if( ( m_pcPic->getCU(m_uiCUAddr)->getCUPelY() + g_auiRasterToPelY[uiAbsPartIdxLB] + (m_pcPic->getPicSym()->getMinCUHeight() * uiPartUnitOffset)) >= m_pcSlice->getSPS()->getPicHeightInLumaSamples())
  {
    uiBLPartUnitIdx = MAX_UINT;
    return NULL;
  }
  
  if ( RasterAddress::lessThanRow( uiAbsPartIdxLB, m_pcPic->getNumPartInHeight() - uiPartUnitOffset, uiNumPartInCUWidth ) )
  {
    if ( !RasterAddress::isZeroCol( uiAbsPartIdxLB, uiNumPartInCUWidth ) )
    {
      if ( uiCurrPartUnitIdx > g_auiRasterToZscan[ uiAbsPartIdxLB + uiPartUnitOffset * uiNumPartInCUWidth - 1 ] )
      {
        uiBLPartUnitIdx = g_auiRasterToZscan[ uiAbsPartIdxLB + uiPartUnitOffset * uiNumPartInCUWidth - 1 ];
        if ( RasterAddress::isEqualRowOrCol( uiAbsPartIdxLB, uiAbsZorderCUIdxLB, uiNumPartInCUWidth ) )
        {
          TComDataCU* pcTempReconCU = m_pcPic->getCU( getAddr() );
          if ((bEnforceSliceRestriction        && (pcTempReconCU==NULL || pcTempReconCU->getSlice() == NULL || pcTempReconCU->getSCUAddr()+uiBLPartUnitIdx < m_pcPic->getCU( getAddr() )->getSliceStartCU       (uiCurrPartUnitIdx)))
            ||(bEnforceEntropySliceRestriction && (pcTempReconCU==NULL || pcTempReconCU->getSlice() == NULL || pcTempReconCU->getSCUAddr()+uiBLPartUnitIdx < m_pcPic->getCU( getAddr() )->getEntropySliceStartCU(uiCurrPartUnitIdx))))
          {
            return NULL;
          }
          return m_pcPic->getCU( getAddr() );
        }
        else
        {
          uiBLPartUnitIdx -= m_uiAbsIdxInLCU;
          if ((bEnforceSliceRestriction        && (this->getSCUAddr()+uiBLPartUnitIdx < m_pcPic->getCU( getAddr() )->getSliceStartCU       (uiCurrPartUnitIdx) || m_pcSlice==NULL))
            ||(bEnforceEntropySliceRestriction && (this->getSCUAddr()+uiBLPartUnitIdx < m_pcPic->getCU( getAddr() )->getEntropySliceStartCU(uiCurrPartUnitIdx) || m_pcSlice==NULL)))
          {
            return NULL;
          }
          return this;
        }
      }
      uiBLPartUnitIdx = MAX_UINT;
      return NULL;
    }
    uiBLPartUnitIdx = g_auiRasterToZscan[ uiAbsPartIdxLB + (1+uiPartUnitOffset) * uiNumPartInCUWidth - 1 ];
  if ( (bEnforceSliceRestriction && (m_pcCULeft==NULL || m_pcCULeft->getSlice()==NULL || 
       m_pcCULeft->getSCUAddr()+uiBLPartUnitIdx < m_pcPic->getCU( getAddr() )->getSliceStartCU(uiCurrPartUnitIdx)||
       (m_pcPic->getPicSym()->getTileIdxMap( m_pcCULeft->getAddr() ) != m_pcPic->getPicSym()->getTileIdxMap(getAddr()))
       ))||
       (bEnforceEntropySliceRestriction && (m_pcCULeft==NULL || m_pcCULeft->getSlice()==NULL || 
       m_pcCULeft->getSCUAddr()+uiBLPartUnitIdx < m_pcPic->getCU( getAddr() )->getEntropySliceStartCU(uiCurrPartUnitIdx)||
       (m_pcPic->getPicSym()->getTileIdxMap( m_pcCULeft->getAddr() ) != m_pcPic->getPicSym()->getTileIdxMap(getAddr()))
       ))
     )
    {
      return NULL;
    }
    return m_pcCULeft;
  }
  
  uiBLPartUnitIdx = MAX_UINT;
  return NULL;
}

TComDataCU* TComDataCU::getPUAboveRightAdi(UInt&  uiARPartUnitIdx, UInt uiPuWidth, UInt uiCurrPartUnitIdx, UInt uiPartUnitOffset, Bool bEnforceSliceRestriction, Bool bEnforceEntropySliceRestriction )
{
  UInt uiAbsPartIdxRT     = g_auiZscanToRaster[uiCurrPartUnitIdx];
  UInt uiAbsZorderCUIdx   = g_auiZscanToRaster[ m_uiAbsIdxInLCU ] + (m_puhWidth[0] / m_pcPic->getMinCUWidth()) - 1;
  UInt uiNumPartInCUWidth = m_pcPic->getNumPartInWidth();
  
  if( ( m_pcPic->getCU(m_uiCUAddr)->getCUPelX() + g_auiRasterToPelX[uiAbsPartIdxRT] + (m_pcPic->getPicSym()->getMinCUHeight() * uiPartUnitOffset)) >= m_pcSlice->getSPS()->getPicWidthInLumaSamples() )
  {
    uiARPartUnitIdx = MAX_UINT;
    return NULL;
  }
  
  if ( RasterAddress::lessThanCol( uiAbsPartIdxRT, uiNumPartInCUWidth - uiPartUnitOffset, uiNumPartInCUWidth ) )
  {
    if ( !RasterAddress::isZeroRow( uiAbsPartIdxRT, uiNumPartInCUWidth ) )
    {
      if ( uiCurrPartUnitIdx > g_auiRasterToZscan[ uiAbsPartIdxRT - uiNumPartInCUWidth + uiPartUnitOffset ] )
      {
        uiARPartUnitIdx = g_auiRasterToZscan[ uiAbsPartIdxRT - uiNumPartInCUWidth + uiPartUnitOffset ];
        if ( RasterAddress::isEqualRowOrCol( uiAbsPartIdxRT, uiAbsZorderCUIdx, uiNumPartInCUWidth ) )
        {
          TComDataCU* pcTempReconCU = m_pcPic->getCU( getAddr() );
          if ((bEnforceSliceRestriction && (pcTempReconCU==NULL || pcTempReconCU->getSlice() == NULL || pcTempReconCU->getSCUAddr()+uiARPartUnitIdx < m_pcPic->getCU( getAddr() )->getSliceStartCU(uiCurrPartUnitIdx)))
           ||( bEnforceEntropySliceRestriction && (pcTempReconCU==NULL || pcTempReconCU->getSlice() == NULL || pcTempReconCU->getSCUAddr()+uiARPartUnitIdx < m_pcPic->getCU( getAddr() )->getEntropySliceStartCU(uiCurrPartUnitIdx))))

          {
            return NULL;
          }
          return m_pcPic->getCU( getAddr() );
        }
        else
        {
          uiARPartUnitIdx -= m_uiAbsIdxInLCU;
          if ((bEnforceSliceRestriction && (this->getSCUAddr()+uiARPartUnitIdx < m_pcPic->getCU( getAddr() )->getSliceStartCU(uiCurrPartUnitIdx) || m_pcSlice==NULL))
            ||(bEnforceEntropySliceRestriction && (this->getSCUAddr()+uiARPartUnitIdx < m_pcPic->getCU( getAddr() )->getEntropySliceStartCU(uiCurrPartUnitIdx) || m_pcSlice==NULL)))
          {
            return NULL;
          }
          return this;
        }
      }
      uiARPartUnitIdx = MAX_UINT;
      return NULL;
    }
    uiARPartUnitIdx = g_auiRasterToZscan[ uiAbsPartIdxRT + m_pcPic->getNumPartInCU() - uiNumPartInCUWidth + uiPartUnitOffset ];
  if ( (bEnforceSliceRestriction && (m_pcCUAbove==NULL || m_pcCUAbove->getSlice()==NULL || 
       m_pcCUAbove->getSCUAddr()+uiARPartUnitIdx < m_pcPic->getCU( getAddr() )->getSliceStartCU(uiCurrPartUnitIdx)||
       (m_pcPic->getPicSym()->getTileIdxMap( m_pcCUAbove->getAddr() ) != m_pcPic->getPicSym()->getTileIdxMap(getAddr()))
       ))||
       (bEnforceEntropySliceRestriction && (m_pcCUAbove==NULL || m_pcCUAbove->getSlice()==NULL || 
       m_pcCUAbove->getSCUAddr()+uiARPartUnitIdx < m_pcPic->getCU( getAddr() )->getEntropySliceStartCU(uiCurrPartUnitIdx)||
       (m_pcPic->getPicSym()->getTileIdxMap( m_pcCUAbove->getAddr() ) != m_pcPic->getPicSym()->getTileIdxMap(getAddr()))
       ))
     )
    {
      return NULL;
    }
    return m_pcCUAbove;
  }
  
  if ( !RasterAddress::isZeroRow( uiAbsPartIdxRT, uiNumPartInCUWidth ) )
  {
    uiARPartUnitIdx = MAX_UINT;
    return NULL;
  }
  
  uiARPartUnitIdx = g_auiRasterToZscan[ m_pcPic->getNumPartInCU() - uiNumPartInCUWidth + uiPartUnitOffset-1 ];
  if ( (bEnforceSliceRestriction && (m_pcCUAboveRight==NULL || m_pcCUAboveRight->getSlice()==NULL ||
       m_pcPic->getPicSym()->getInverseCUOrderMap( m_pcCUAboveRight->getAddr()) > m_pcPic->getPicSym()->getInverseCUOrderMap( getAddr()) ||
       m_pcCUAboveRight->getSCUAddr()+uiARPartUnitIdx < m_pcPic->getCU( getAddr() )->getSliceStartCU(uiCurrPartUnitIdx)||
       (m_pcPic->getPicSym()->getTileIdxMap( m_pcCUAboveRight->getAddr() ) != m_pcPic->getPicSym()->getTileIdxMap(getAddr()))
       ))||
       (bEnforceEntropySliceRestriction && (m_pcCUAboveRight==NULL || m_pcCUAboveRight->getSlice()==NULL || 
       m_pcPic->getPicSym()->getInverseCUOrderMap( m_pcCUAboveRight->getAddr()) > m_pcPic->getPicSym()->getInverseCUOrderMap( getAddr()) ||
       m_pcCUAboveRight->getSCUAddr()+uiARPartUnitIdx < m_pcPic->getCU( getAddr() )->getEntropySliceStartCU(uiCurrPartUnitIdx)||
       (m_pcPic->getPicSym()->getTileIdxMap( m_pcCUAboveRight->getAddr() ) != m_pcPic->getPicSym()->getTileIdxMap(getAddr()))
       ))
     )
  {
    return NULL;
  }
  return m_pcCUAboveRight;
}

/** Get left QpMinCu
*\param   uiLPartUnitIdx
*\param   uiCurrAbsIdxInLCU
*\param   bEnforceSliceRestriction
*\param   bEnforceEntropySliceRestriction
*\returns TComDataCU*   point of TComDataCU of left QpMinCu
*/
TComDataCU* TComDataCU::getQpMinCuLeft( UInt& uiLPartUnitIdx, UInt uiCurrAbsIdxInLCU, Bool bEnforceSliceRestriction, Bool bEnforceEntropySliceRestriction)
{
  UInt uiNumPartInCUWidth = m_pcPic->getNumPartInWidth();

  UInt uiAbsRorderQpMinCUIdx   = g_auiZscanToRaster[(uiCurrAbsIdxInLCU>>(8-(getSlice()->getPPS()->getMaxCuDQPDepth()<<1)))<<(8-(getSlice()->getPPS()->getMaxCuDQPDepth()<<1))];
  UInt uiNumPartInQpMinCUWidth = getSlice()->getPPS()->getMinCuDQPSize()>>2;

  UInt uiAbsZorderQpMinCUIdx   = (uiCurrAbsIdxInLCU>>(8-(getSlice()->getPPS()->getMaxCuDQPDepth()<<1)))<<(8-(getSlice()->getPPS()->getMaxCuDQPDepth()<<1));
  if( (uiCurrAbsIdxInLCU != uiAbsZorderQpMinCUIdx) && 
    ((bEnforceSliceRestriction && (m_pcPic->getCU( getAddr() )->getSliceStartCU(uiCurrAbsIdxInLCU) != m_pcPic->getCU( getAddr() )->getSliceStartCU (uiAbsZorderQpMinCUIdx))) ||
    (bEnforceEntropySliceRestriction &&(m_pcPic->getCU( getAddr() )->getEntropySliceStartCU(uiCurrAbsIdxInLCU) != m_pcPic->getCU( getAddr() )->getEntropySliceStartCU (uiAbsZorderQpMinCUIdx)) )) )
  {
    return NULL;
  }

  if ( !RasterAddress::isZeroCol( uiAbsRorderQpMinCUIdx, uiNumPartInCUWidth ) )
  {
    uiLPartUnitIdx = g_auiRasterToZscan[ uiAbsRorderQpMinCUIdx - uiNumPartInQpMinCUWidth ];
    TComDataCU* pcTempReconCU = m_pcPic->getCU( getAddr() );
    if ((bEnforceSliceRestriction        && (pcTempReconCU==NULL || pcTempReconCU->getSlice() == NULL || pcTempReconCU->getSCUAddr()+uiLPartUnitIdx < m_pcPic->getCU( getAddr() )->getSliceStartCU       (uiAbsZorderQpMinCUIdx)))
      ||(bEnforceEntropySliceRestriction && (pcTempReconCU==NULL || pcTempReconCU->getSlice() == NULL || pcTempReconCU->getSCUAddr()+uiLPartUnitIdx < m_pcPic->getCU( getAddr() )->getEntropySliceStartCU(uiAbsZorderQpMinCUIdx))))
    {
      return NULL;
    }
    return m_pcPic->getCU( getAddr() );
  }

  uiLPartUnitIdx = g_auiRasterToZscan[ uiAbsRorderQpMinCUIdx + uiNumPartInCUWidth - uiNumPartInQpMinCUWidth ];

  if ( (bEnforceSliceRestriction && (m_pcCULeft==NULL || m_pcCULeft->getSlice()==NULL || 
       m_pcCULeft->getSCUAddr()+uiLPartUnitIdx < m_pcPic->getCU( getAddr() )->getSliceStartCU(uiAbsZorderQpMinCUIdx)||
       (m_pcPic->getPicSym()->getTileIdxMap( m_pcCULeft->getAddr() ) != m_pcPic->getPicSym()->getTileIdxMap(getAddr()))
       ))||
       (bEnforceEntropySliceRestriction && (m_pcCULeft==NULL || m_pcCULeft->getSlice()==NULL || 
       m_pcCULeft->getSCUAddr()+uiLPartUnitIdx < m_pcPic->getCU( getAddr() )->getEntropySliceStartCU(uiAbsZorderQpMinCUIdx)||
       (m_pcPic->getPicSym()->getTileIdxMap( m_pcCULeft->getAddr() ) != m_pcPic->getPicSym()->getTileIdxMap(getAddr()))
       ))
     )
  {
    return NULL;
  }

  if ( m_pcCULeft && m_pcCULeft->getAddr() != getAddr() )
  {
    return NULL;
  }

  return m_pcCULeft;
}

/** Get Above QpMinCu
*\param   aPartUnitIdx
*\param   currAbsIdxInLCU
*\param   enforceSliceRestriction
*\param   enforceEntropySliceRestriction
*\returns TComDataCU*   point of TComDataCU of above QpMinCu
*/
TComDataCU* TComDataCU::getQpMinCuAbove( UInt& aPartUnitIdx, UInt currAbsIdxInLCU, Bool enforceSliceRestriction, Bool enforceEntropySliceRestriction )
{
  UInt numPartInCUWidth = m_pcPic->getNumPartInWidth();

  UInt absRorderQpMinCUIdx   = g_auiZscanToRaster[(currAbsIdxInLCU>>(8-(getSlice()->getPPS()->getMaxCuDQPDepth()<<1)))<<(8-(getSlice()->getPPS()->getMaxCuDQPDepth()<<1))];
 
  UInt absZorderQpMinCUIdx   = (currAbsIdxInLCU>>(8-(getSlice()->getPPS()->getMaxCuDQPDepth()<<1)))<<(8-(getSlice()->getPPS()->getMaxCuDQPDepth()<<1));
  if( (currAbsIdxInLCU != absZorderQpMinCUIdx) && 
     ((enforceSliceRestriction        && (m_pcPic->getCU( getAddr() )->getSliceStartCU(currAbsIdxInLCU) != m_pcPic->getCU( getAddr() )->getSliceStartCU (absZorderQpMinCUIdx)))
    ||(enforceEntropySliceRestriction && (m_pcPic->getCU( getAddr() )->getEntropySliceStartCU(currAbsIdxInLCU) != m_pcPic->getCU( getAddr() )->getEntropySliceStartCU (absZorderQpMinCUIdx)) )) )
  {
    return NULL;
  }

  if ( !RasterAddress::isZeroRow( absRorderQpMinCUIdx, numPartInCUWidth ) )
  {
    aPartUnitIdx = g_auiRasterToZscan[ absRorderQpMinCUIdx - numPartInCUWidth ];
    TComDataCU* pcTempReconCU = m_pcPic->getCU( getAddr() );
    if ((enforceSliceRestriction        && (pcTempReconCU==NULL || pcTempReconCU->getSlice() == NULL || pcTempReconCU->getSCUAddr()+aPartUnitIdx < m_pcPic->getCU( getAddr() )->getSliceStartCU       (absZorderQpMinCUIdx)))
      ||(enforceEntropySliceRestriction && (pcTempReconCU==NULL || pcTempReconCU->getSlice() == NULL || pcTempReconCU->getSCUAddr()+aPartUnitIdx < m_pcPic->getCU( getAddr() )->getEntropySliceStartCU(absZorderQpMinCUIdx))))
    {
      return NULL;
    }
    return m_pcPic->getCU( getAddr() );
  }
  
  aPartUnitIdx = g_auiRasterToZscan[ absRorderQpMinCUIdx + m_pcPic->getNumPartInCU() - numPartInCUWidth ];

  if ( (enforceSliceRestriction && (m_pcCUAbove==NULL || m_pcCUAbove->getSlice()==NULL || 
       m_pcCUAbove->getSCUAddr()+aPartUnitIdx < m_pcPic->getCU( getAddr() )->getSliceStartCU(absZorderQpMinCUIdx)||
       (m_pcPic->getPicSym()->getTileIdxMap( m_pcCUAbove->getAddr() ) != m_pcPic->getPicSym()->getTileIdxMap(getAddr()))
       ))||
       (enforceEntropySliceRestriction && (m_pcCUAbove==NULL || m_pcCUAbove->getSlice()==NULL || 
       m_pcCUAbove->getSCUAddr()+aPartUnitIdx < m_pcPic->getCU( getAddr() )->getEntropySliceStartCU(absZorderQpMinCUIdx)||
       (m_pcPic->getPicSym()->getTileIdxMap( m_pcCUAbove->getAddr() ) != m_pcPic->getPicSym()->getTileIdxMap(getAddr()))
       ))
     )
  {
    return NULL;
  }

  if ( m_pcCUAbove && m_pcCUAbove->getAddr() != getAddr() )
  {
    return NULL;
  }

  return m_pcCUAbove;
}

/** Get reference QP from left QpMinCu or latest coded QP
*\param   uiCurrAbsIdxInLCU
*\returns Char   reference QP value
*/
Char TComDataCU::getRefQP( UInt uiCurrAbsIdxInLCU )
{
  UInt        lPartIdx, aPartIdx;
  lPartIdx = 0; 
  aPartIdx = 0;
  TComDataCU* cULeft  = getQpMinCuLeft ( lPartIdx, m_uiAbsIdxInLCU + uiCurrAbsIdxInLCU );
  TComDataCU* cUAbove = getQpMinCuAbove( aPartIdx, m_uiAbsIdxInLCU + uiCurrAbsIdxInLCU );
  return (((cULeft? cULeft->getQP( lPartIdx ): getLastCodedQP( uiCurrAbsIdxInLCU )) + (cUAbove? cUAbove->getQP( aPartIdx ): getLastCodedQP( uiCurrAbsIdxInLCU )) + 1) >> 1);
}

Int TComDataCU::getLastValidPartIdx( Int iAbsPartIdx )
{
  Int iLastValidPartIdx = iAbsPartIdx-1;
  while ( iLastValidPartIdx >= 0
       && getPredictionMode( iLastValidPartIdx ) == MODE_NONE )
  {
    UInt uiDepth = getDepth( iLastValidPartIdx );
    iLastValidPartIdx -= m_uiNumPartition>>(uiDepth<<1);
  }
  return iLastValidPartIdx;
}

Char TComDataCU::getLastCodedQP( UInt uiAbsPartIdx )
{
  UInt uiQUPartIdxMask = ~((1<<(8-(getSlice()->getPPS()->getMaxCuDQPDepth()<<1)))-1);
  Int iLastValidPartIdx = getLastValidPartIdx( uiAbsPartIdx&uiQUPartIdxMask );
  if ( uiAbsPartIdx < m_uiNumPartition
    && (getSCUAddr()+iLastValidPartIdx < getSliceStartCU(m_uiAbsIdxInLCU+uiAbsPartIdx) || getSCUAddr()+iLastValidPartIdx < getEntropySliceStartCU(m_uiAbsIdxInLCU+uiAbsPartIdx) ))
  {
    return getSlice()->getSliceQp();
  }
  else
  if ( iLastValidPartIdx >= 0 )
  {
    return getQP( iLastValidPartIdx );
  }
  else
  {
    if ( getZorderIdxInCU() > 0 )
    {
      return getPic()->getCU( getAddr() )->getLastCodedQP( getZorderIdxInCU() );
    }
    else if ( getAddr() > 0 )
    {
      return getPic()->getCU( getAddr()-1 )->getLastCodedQP( getPic()->getNumPartInCU() );
    }
    else
    {
      return getSlice()->getSliceQp();
    }
  }
}
#if LOSSLESS_CODING
/** Check whether the CU is coded in lossless coding mode
 * \param   uiAbsPartIdx
 * \returns true if the CU is coded in lossless coding mode; false if otherwise 
 */
Bool TComDataCU::isLosslessCoded(UInt absPartIdx)
{
  return ( getSlice()->getSPS()->getUseLossless() && ((getQP(absPartIdx) + getSlice()->getSPS()->getQpBDOffsetY()) == 0) );
}
#endif

/** Get allowed chroma intra modes
*\param   uiAbsPartIdx
*\param   uiModeList  pointer to chroma intra modes array
*\returns 
*- fill uiModeList with chroma intra modes
*/
Void TComDataCU::getAllowedChromaDir( UInt uiAbsPartIdx, UInt* uiModeList )
{
  uiModeList[0] = PLANAR_IDX;
  uiModeList[1] = VER_IDX;
  uiModeList[2] = HOR_IDX;
  uiModeList[3] = DC_IDX;
  uiModeList[4] = LM_CHROMA_IDX;
  uiModeList[5] = DM_CHROMA_IDX;

  UInt uiLumaMode = getLumaIntraDir( uiAbsPartIdx );
#if HHI_DMM_WEDGE_INTRA || HHI_DMM_PRED_TEX
  mapDMMtoIntraMode( uiLumaMode );
#endif

  for( Int i = 0; i < NUM_CHROMA_MODE - 2; i++ )
  {
    if( uiLumaMode == uiModeList[i] )
    {
      uiModeList[i] = 34; // VER+8 mode
      break;
    }
  }
}

/** Get most probable intra modes
*\param   uiAbsPartIdx
*\param   uiIntraDirPred  pointer to the array for MPM storage
*\param   piMode          it is set with MPM mode in case both MPM are equal. It is used to restrict RD search at encode side.
*\returns Number of MPM
*/
Int TComDataCU::getIntraDirLumaPredictor( UInt uiAbsPartIdx, Int* uiIntraDirPred, Int* piMode  )
{
  TComDataCU* pcTempCU;
  UInt        uiTempPartIdx;
  Int         iLeftIntraDir, iAboveIntraDir;
  Int         uiPredNum = 0;
  
  // Get intra direction of left PU
  pcTempCU = getPULeft( uiTempPartIdx, m_uiAbsIdxInLCU + uiAbsPartIdx );
  
  iLeftIntraDir  = pcTempCU ? ( pcTempCU->isIntra( uiTempPartIdx ) ? pcTempCU->getLumaIntraDir( uiTempPartIdx ) : DC_IDX ) : DC_IDX;
#if HHI_DMM_WEDGE_INTRA || HHI_DMM_PRED_TEX
  mapDMMtoIntraMode( iLeftIntraDir );
#endif
#if LGE_EDGE_INTRA_A0070
  mapEdgeIntratoDC( iLeftIntraDir );
#endif
  
  // Get intra direction of above PU
  pcTempCU = getPUAbove( uiTempPartIdx, m_uiAbsIdxInLCU + uiAbsPartIdx, true, true, false, true );
  
  iAboveIntraDir = pcTempCU ? ( pcTempCU->isIntra( uiTempPartIdx ) ? pcTempCU->getLumaIntraDir( uiTempPartIdx ) : DC_IDX ) : DC_IDX;
#if HHI_DMM_WEDGE_INTRA || HHI_DMM_PRED_TEX
  mapDMMtoIntraMode( iAboveIntraDir );
#endif
#if LGE_EDGE_INTRA_A0070
  mapEdgeIntratoDC( iAboveIntraDir );
#endif
  
  uiPredNum = 3;
  if(iLeftIntraDir == iAboveIntraDir)
  {
    if( piMode )
    {
      *piMode = 1;
    }
    
    if (iLeftIntraDir > 1) // angular modes
    {
      uiIntraDirPred[0] = iLeftIntraDir;
      uiIntraDirPred[1] = ((iLeftIntraDir + 29) % 32) + 2;
      uiIntraDirPred[2] = ((iLeftIntraDir - 1 ) % 32) + 2;
    }
    else //non-angular
    {
      uiIntraDirPred[0] = PLANAR_IDX;
      uiIntraDirPred[1] = DC_IDX;
      uiIntraDirPred[2] = VER_IDX; 
    }
  }
  else
  {
    if( piMode )
    {
      *piMode = 2;
    }
    uiIntraDirPred[0] = iLeftIntraDir;
    uiIntraDirPred[1] = iAboveIntraDir;
    
    if (iLeftIntraDir && iAboveIntraDir ) //both modes are non-planar
    {
      uiIntraDirPred[2] = PLANAR_IDX;
    }
    else
    {
      uiIntraDirPred[2] =  (iLeftIntraDir+iAboveIntraDir)<2? VER_IDX : DC_IDX;
    }
  }
  
  return uiPredNum;
}

#if LGE_EDGE_INTRA_A0070
UInt TComDataCU::getCtxEdgeIntra( UInt uiAbsPartIdx )
{
  UInt        uiCtx = 0;
  return uiCtx;
}
#endif

UInt TComDataCU::getCtxSplitFlag( UInt uiAbsPartIdx, UInt uiDepth )
{
  TComDataCU* pcTempCU;
  UInt        uiTempPartIdx;
  UInt        uiCtx;
  // Get left split flag
  pcTempCU = getPULeft( uiTempPartIdx, m_uiAbsIdxInLCU + uiAbsPartIdx );
  uiCtx  = ( pcTempCU ) ? ( ( pcTempCU->getDepth( uiTempPartIdx ) > uiDepth ) ? 1 : 0 ) : 0;
  
  // Get above split flag
  pcTempCU = getPUAbove( uiTempPartIdx, m_uiAbsIdxInLCU + uiAbsPartIdx );
  uiCtx += ( pcTempCU ) ? ( ( pcTempCU->getDepth( uiTempPartIdx ) > uiDepth ) ? 1 : 0 ) : 0;
  
  return uiCtx;
}

UInt TComDataCU::getCtxQtCbf( UInt uiAbsPartIdx, TextType eType, UInt uiTrDepth )
{
  if( eType )
  {
    return uiTrDepth;
  }
  else
  {
    const UInt uiDepth = getDepth( uiAbsPartIdx );
    const UInt uiLog2TrafoSize = g_aucConvertToBit[ getSlice()->getSPS()->getMaxCUWidth() ] + 2 - uiDepth - uiTrDepth;
    const UInt uiCtx = uiTrDepth == 0 || uiLog2TrafoSize == getSlice()->getSPS()->getQuadtreeTULog2MaxSize() ? 1 : 0;
    return uiCtx;
  }
}

UInt TComDataCU::getQuadtreeTULog2MinSizeInCU( UInt absPartIdx )
{
  UInt log2CbSize = g_aucConvertToBit[getWidth( absPartIdx )] + 2;
  PartSize  partSize  = getPartitionSize( absPartIdx );
  UInt quadtreeTUMaxDepth = getPredictionMode( absPartIdx ) == MODE_INTRA ? m_pcSlice->getSPS()->getQuadtreeTUMaxDepthIntra() : m_pcSlice->getSPS()->getQuadtreeTUMaxDepthInter(); 
  Int intraSplitFlag = ( getPredictionMode( absPartIdx ) == MODE_INTRA && partSize == SIZE_NxN ) ? 1 : 0;
  Int interSplitFlag = ((quadtreeTUMaxDepth == 1) && (getPredictionMode( absPartIdx ) == MODE_INTER) && (partSize != SIZE_2Nx2N) );
  
  UInt log2MinTUSizeInCU = 0;
  if (log2CbSize < (m_pcSlice->getSPS()->getQuadtreeTULog2MinSize() + quadtreeTUMaxDepth - 1 + interSplitFlag + intraSplitFlag) ) 
  {
    // when fully making use of signaled TUMaxDepth + inter/intraSplitFlag, resulting luma TB size is < QuadtreeTULog2MinSize
    log2MinTUSizeInCU = m_pcSlice->getSPS()->getQuadtreeTULog2MinSize();
  }
  else
  {
    // when fully making use of signaled TUMaxDepth + inter/intraSplitFlag, resulting luma TB size is still >= QuadtreeTULog2MinSize
    log2MinTUSizeInCU = log2CbSize - ( quadtreeTUMaxDepth - 1 + interSplitFlag + intraSplitFlag); // stop when trafoDepth == hierarchy_depth = splitFlag
    if ( log2MinTUSizeInCU > m_pcSlice->getSPS()->getQuadtreeTULog2MaxSize())
    {
      // when fully making use of signaled TUMaxDepth + inter/intraSplitFlag, resulting luma TB size is still > QuadtreeTULog2MaxSize
      log2MinTUSizeInCU = m_pcSlice->getSPS()->getQuadtreeTULog2MaxSize();
    }  
  }
  return log2MinTUSizeInCU;
}


#if H3D_IVRP
UInt
TComDataCU::getCtxResPredFlag( UInt uiAbsPartIdx )
{
#if 1 // simple context
  UInt        uiCtx = 0;
#else
  TComDataCU* pcTempCU;
  UInt        uiTempPartIdx;
  UInt        uiCtx = 0;
  
  // Get BCBP of left PU
  pcTempCU = getPULeft( uiTempPartIdx, m_uiAbsIdxInLCU + uiAbsPartIdx );
  uiCtx    = ( pcTempCU ) ? pcTempCU->getResPredFlag( uiTempPartIdx ) : 0;
  
  // Get BCBP of above PU
  pcTempCU = getPUAbove( uiTempPartIdx, m_uiAbsIdxInLCU + uiAbsPartIdx );
  uiCtx   += ( pcTempCU ) ? pcTempCU->getResPredFlag( uiTempPartIdx ) : 0;
#endif  
  return uiCtx;
}
#endif


UInt TComDataCU::getCtxSkipFlag( UInt uiAbsPartIdx )
{
  TComDataCU* pcTempCU;
  UInt        uiTempPartIdx;
  UInt        uiCtx = 0;
  
  // Get BCBP of left PU
  pcTempCU = getPULeft( uiTempPartIdx, m_uiAbsIdxInLCU + uiAbsPartIdx );
  uiCtx    = ( pcTempCU ) ? pcTempCU->isSkipped( uiTempPartIdx ) : 0;
  
  // Get BCBP of above PU
  pcTempCU = getPUAbove( uiTempPartIdx, m_uiAbsIdxInLCU + uiAbsPartIdx );
  uiCtx   += ( pcTempCU ) ? pcTempCU->isSkipped( uiTempPartIdx ) : 0;
  
  return uiCtx;
}

#if LGE_ILLUCOMP_B0045
UInt TComDataCU::getCtxICFlag( UInt uiAbsPartIdx )
{
  UInt        uiCtx = 0;

  return uiCtx;
}
#endif

UInt TComDataCU::getCtxInterDir( UInt uiAbsPartIdx )
{
  return getDepth( uiAbsPartIdx );
}

Void TComDataCU::setCbfSubParts( UInt uiCbfY, UInt uiCbfU, UInt uiCbfV, UInt uiAbsPartIdx, UInt uiDepth )
{
  UInt uiCurrPartNumb = m_pcPic->getNumPartInCU() >> (uiDepth << 1);
  memset( m_puhCbf[0] + uiAbsPartIdx, uiCbfY, sizeof( UChar ) * uiCurrPartNumb );
  memset( m_puhCbf[1] + uiAbsPartIdx, uiCbfU, sizeof( UChar ) * uiCurrPartNumb );
  memset( m_puhCbf[2] + uiAbsPartIdx, uiCbfV, sizeof( UChar ) * uiCurrPartNumb );
}

Void TComDataCU::setCbfSubParts( UInt uiCbf, TextType eTType, UInt uiAbsPartIdx, UInt uiDepth )
{
  UInt uiCurrPartNumb = m_pcPic->getNumPartInCU() >> (uiDepth << 1);
  memset( m_puhCbf[g_aucConvertTxtTypeToIdx[eTType]] + uiAbsPartIdx, uiCbf, sizeof( UChar ) * uiCurrPartNumb );
}

/** Sets a coded block flag for all sub-partitions of a partition
 * \param uiCbf The value of the coded block flag to be set
 * \param eTType
 * \param uiAbsPartIdx
 * \param uiPartIdx
 * \param uiDepth
 * \returns Void
 */
Void TComDataCU::setCbfSubParts ( UInt uiCbf, TextType eTType, UInt uiAbsPartIdx, UInt uiPartIdx, UInt uiDepth )
{
  setSubPart<UChar>( uiCbf, m_puhCbf[g_aucConvertTxtTypeToIdx[eTType]], uiAbsPartIdx, uiDepth, uiPartIdx );
}

Void TComDataCU::setDepthSubParts( UInt uiDepth, UInt uiAbsPartIdx )
{
  UInt uiCurrPartNumb = m_pcPic->getNumPartInCU() >> (uiDepth << 1);
  memset( m_puhDepth + uiAbsPartIdx, uiDepth, sizeof(UChar)*uiCurrPartNumb );
}

Bool TComDataCU::isFirstAbsZorderIdxInDepth (UInt uiAbsPartIdx, UInt uiDepth)
{
  UInt uiPartNumb = m_pcPic->getNumPartInCU() >> (uiDepth << 1);
  return (((m_uiAbsIdxInLCU + uiAbsPartIdx)% uiPartNumb) == 0);
}

Void TComDataCU::setAlfCtrlFlagSubParts( Bool uiFlag, UInt uiAbsPartIdx, UInt uiDepth )
{
  UInt uiCurrPartNumb = m_pcPic->getNumPartInCU() >> ( 2 * uiDepth );
  memset( m_puiAlfCtrlFlag + uiAbsPartIdx, uiFlag, sizeof( *m_puiAlfCtrlFlag ) * uiCurrPartNumb );
}

Void TComDataCU::createTmpAlfCtrlFlag()
{
  m_puiTmpAlfCtrlFlag = new Bool[ m_uiNumPartition ];
}

Void TComDataCU::destroyTmpAlfCtrlFlag()
{
  if(m_puiTmpAlfCtrlFlag)
  {
    delete[] m_puiTmpAlfCtrlFlag;
    m_puiTmpAlfCtrlFlag = NULL;
  }
}

Void TComDataCU::copyAlfCtrlFlagToTmp()
{
  memcpy( m_puiTmpAlfCtrlFlag, m_puiAlfCtrlFlag, sizeof( *m_puiAlfCtrlFlag )*m_uiNumPartition );
}

Void TComDataCU::copyAlfCtrlFlagFromTmp()
{
  memcpy( m_puiAlfCtrlFlag, m_puiTmpAlfCtrlFlag, sizeof( *m_puiAlfCtrlFlag )*m_uiNumPartition );
}

Void TComDataCU::setPartSizeSubParts( PartSize eMode, UInt uiAbsPartIdx, UInt uiDepth )
{
  assert( sizeof( *m_pePartSize) == 1 );
  memset( m_pePartSize + uiAbsPartIdx, eMode, m_pcPic->getNumPartInCU() >> ( 2 * uiDepth ) );
}
#if HHI_INTERVIEW_SKIP
Void TComDataCU::setRenderableSubParts ( Bool bRenderable, UInt uiAbsPartIdx, UInt uiDepth )
{
  UInt uiCurrPartNumb = m_pcPic->getNumPartInCU() >> (uiDepth << 1);

  for (UInt ui = 0; ui < uiCurrPartNumb; ui++ )
  {
    m_pbRenderable[uiAbsPartIdx + ui] = bRenderable;
  }
}
#endif

Void TComDataCU::setPredModeSubParts( PredMode eMode, UInt uiAbsPartIdx, UInt uiDepth )
{
  assert( sizeof( *m_pePartSize) == 1 );
  memset( m_pePredMode + uiAbsPartIdx, eMode, m_pcPic->getNumPartInCU() >> ( 2 * uiDepth ) );
}

Void TComDataCU::setQPSubParts( Int qp, UInt uiAbsPartIdx, UInt uiDepth )
{
  UInt uiCurrPartNumb = m_pcPic->getNumPartInCU() >> (uiDepth << 1);
  TComSlice * pcSlice = getPic()->getSlice(getPic()->getCurrSliceIdx());

  for(UInt uiSCUIdx = uiAbsPartIdx; uiSCUIdx < uiAbsPartIdx+uiCurrPartNumb; uiSCUIdx++)
  {
    if( m_pcPic->getCU( getAddr() )->getEntropySliceStartCU(uiSCUIdx+getZorderIdxInCU()) == pcSlice->getEntropySliceCurStartCUAddr() )
    {
      m_phQP[uiSCUIdx] = qp;
    }
  }
}

Void TComDataCU::setLumaIntraDirSubParts( UInt uiDir, UInt uiAbsPartIdx, UInt uiDepth )
{
  UInt uiCurrPartNumb = m_pcPic->getNumPartInCU() >> (uiDepth << 1);
  
  memset( m_puhLumaIntraDir + uiAbsPartIdx, uiDir, sizeof(UChar)*uiCurrPartNumb );
}

template<typename T>
Void TComDataCU::setSubPart( T uiParameter, T* puhBaseLCU, UInt uiCUAddr, UInt uiCUDepth, UInt uiPUIdx )
{
  assert( sizeof(T) == 1 ); // Using memset() works only for types of size 1
  
  UInt uiCurrPartNumQ = (m_pcPic->getNumPartInCU() >> (2 * uiCUDepth)) >> 2;
  switch ( m_pePartSize[ uiCUAddr ] )
  {
    case SIZE_2Nx2N:
      memset( puhBaseLCU + uiCUAddr, uiParameter, 4 * uiCurrPartNumQ );
      break;
    case SIZE_2NxN:
      memset( puhBaseLCU + uiCUAddr, uiParameter, 2 * uiCurrPartNumQ );
      break;
    case SIZE_Nx2N:
      memset( puhBaseLCU + uiCUAddr, uiParameter, uiCurrPartNumQ );
      memset( puhBaseLCU + uiCUAddr + 2 * uiCurrPartNumQ, uiParameter, uiCurrPartNumQ );
      break;
    case SIZE_NxN:
      memset( puhBaseLCU + uiCUAddr, uiParameter, uiCurrPartNumQ ); 
      break;
    case SIZE_2NxnU:
      if ( uiPUIdx == 0 )
      {
        memset( puhBaseLCU + uiCUAddr, uiParameter, (uiCurrPartNumQ >> 1) );                      
        memset( puhBaseLCU + uiCUAddr + uiCurrPartNumQ, uiParameter, (uiCurrPartNumQ >> 1) );                      
      }
      else if ( uiPUIdx == 1 )
      {
        memset( puhBaseLCU + uiCUAddr, uiParameter, (uiCurrPartNumQ >> 1) );                      
        memset( puhBaseLCU + uiCUAddr + uiCurrPartNumQ, uiParameter, ((uiCurrPartNumQ >> 1) + (uiCurrPartNumQ << 1)) );                      
      }
      else
      {
        assert(0);
      }
      break;
    case SIZE_2NxnD:
      if ( uiPUIdx == 0 )
      {
        memset( puhBaseLCU + uiCUAddr, uiParameter, ((uiCurrPartNumQ << 1) + (uiCurrPartNumQ >> 1)) );                      
        memset( puhBaseLCU + uiCUAddr + (uiCurrPartNumQ << 1) + uiCurrPartNumQ, uiParameter, (uiCurrPartNumQ >> 1) );                      
      }
      else if ( uiPUIdx == 1 )
      {
        memset( puhBaseLCU + uiCUAddr, uiParameter, (uiCurrPartNumQ >> 1) );                      
        memset( puhBaseLCU + uiCUAddr + uiCurrPartNumQ, uiParameter, (uiCurrPartNumQ >> 1) );                      
      }
      else
      {
        assert(0);
      }
      break;
    case SIZE_nLx2N:
      if ( uiPUIdx == 0 )
      {
        memset( puhBaseLCU + uiCUAddr, uiParameter, (uiCurrPartNumQ >> 2) );
        memset( puhBaseLCU + uiCUAddr + (uiCurrPartNumQ >> 1), uiParameter, (uiCurrPartNumQ >> 2) ); 
        memset( puhBaseLCU + uiCUAddr + (uiCurrPartNumQ << 1), uiParameter, (uiCurrPartNumQ >> 2) ); 
        memset( puhBaseLCU + uiCUAddr + (uiCurrPartNumQ << 1) + (uiCurrPartNumQ >> 1), uiParameter, (uiCurrPartNumQ >> 2) ); 
      }
      else if ( uiPUIdx == 1 )
      {
        memset( puhBaseLCU + uiCUAddr, uiParameter, (uiCurrPartNumQ >> 2) );
        memset( puhBaseLCU + uiCUAddr + (uiCurrPartNumQ >> 1), uiParameter, (uiCurrPartNumQ + (uiCurrPartNumQ >> 2)) ); 
        memset( puhBaseLCU + uiCUAddr + (uiCurrPartNumQ << 1), uiParameter, (uiCurrPartNumQ >> 2) ); 
        memset( puhBaseLCU + uiCUAddr + (uiCurrPartNumQ << 1) + (uiCurrPartNumQ >> 1), uiParameter, (uiCurrPartNumQ + (uiCurrPartNumQ >> 2)) ); 
      }
      else
      {
        assert(0);
      }
      break;
    case SIZE_nRx2N:
      if ( uiPUIdx == 0 )
      {      
        memset( puhBaseLCU + uiCUAddr, uiParameter, (uiCurrPartNumQ + (uiCurrPartNumQ >> 2)) );                           
        memset( puhBaseLCU + uiCUAddr + uiCurrPartNumQ + (uiCurrPartNumQ >> 1), uiParameter, (uiCurrPartNumQ >> 2) );                           
        memset( puhBaseLCU + uiCUAddr + (uiCurrPartNumQ << 1), uiParameter, (uiCurrPartNumQ + (uiCurrPartNumQ >> 2)) );                           
        memset( puhBaseLCU + uiCUAddr + (uiCurrPartNumQ << 1) + uiCurrPartNumQ + (uiCurrPartNumQ >> 1), uiParameter, (uiCurrPartNumQ >> 2) );                           
      }
      else if ( uiPUIdx == 1 )
      {
        memset( puhBaseLCU + uiCUAddr, uiParameter, (uiCurrPartNumQ >> 2) );                           
        memset( puhBaseLCU + uiCUAddr + (uiCurrPartNumQ >> 1), uiParameter, (uiCurrPartNumQ >> 2) );                           
        memset( puhBaseLCU + uiCUAddr + (uiCurrPartNumQ << 1), uiParameter, (uiCurrPartNumQ >> 2) );                           
        memset( puhBaseLCU + uiCUAddr + (uiCurrPartNumQ << 1) + (uiCurrPartNumQ >> 1), uiParameter, (uiCurrPartNumQ >> 2) );                          
      }
      else
      {
        assert(0);
      }
      break;
    default:
      assert( 0 );
  }
}

Void TComDataCU::setMergeFlagSubParts ( Bool bMergeFlag, UInt uiAbsPartIdx, UInt uiPartIdx, UInt uiDepth )
{
  setSubPart( bMergeFlag, m_pbMergeFlag, uiAbsPartIdx, uiDepth, uiPartIdx );
}

#if RWTH_SDC_DLT_B0036
Void TComDataCU::setSDCFlagSubParts ( Bool bSDCFlag, UInt uiAbsPartIdx, UInt uiPartIdx, UInt uiDepth )
{
  setSubPart( bSDCFlag, m_pbSDCFlag, uiAbsPartIdx, uiDepth, uiPartIdx );
}

UInt TComDataCU::getCtxSDCFlag( UInt uiAbsPartIdx )
{
  TComDataCU* pcTempCU;
  UInt        uiTempPartIdx;
  UInt        uiCtx = 0;
  
  // left PU
  pcTempCU = getPULeft( uiTempPartIdx, m_uiAbsIdxInLCU + uiAbsPartIdx, true, false );
  uiCtx    = ( pcTempCU ) ? pcTempCU->getSDCFlag( uiTempPartIdx ) : 0;
  
  // above PU
  pcTempCU = getPUAbove( uiTempPartIdx, m_uiAbsIdxInLCU + uiAbsPartIdx, true, false );
  uiCtx   += ( pcTempCU ) ? pcTempCU->getSDCFlag( uiTempPartIdx ) : 0;
  
  return uiCtx;
}

Bool TComDataCU::getSDCAvailable( UInt uiAbsPartIdx )
{
  if( !getSlice()->getSPS()->isDepth() || getPartitionSize(uiAbsPartIdx) != SIZE_2Nx2N )
    return false;
  
  UInt uiLumaPredMode = getLumaIntraDir( uiAbsPartIdx );
  
  if(!isIntra(uiAbsPartIdx))
    return false;
  
  for(UInt ui=0; ui<RWTH_SDC_NUM_PRED_MODES; ui++)
  {
    if( g_auiSDCPredModes[ui] == uiLumaPredMode )
      return true;
  }
  // else
  return false;
}
#endif

Void TComDataCU::setMergeIndexSubParts ( UInt uiMergeIndex, UInt uiAbsPartIdx, UInt uiPartIdx, UInt uiDepth )
{
  setSubPart<UChar>( uiMergeIndex, m_puhMergeIndex, uiAbsPartIdx, uiDepth, uiPartIdx );
}

#if MERL_VSP_C0152
Void TComDataCU::setVSPIndexSubParts ( Char iVSPIdx, UInt uiAbsPartIdx, UInt uiPartIdx, UInt uiDepth )
{
  setSubPart<Char>( iVSPIdx, m_piVSPIndex, uiAbsPartIdx, uiDepth, uiPartIdx );
}
#endif

#if H3D_IVRP
Void TComDataCU::setResPredAvailSubParts( Bool bResPredAvailable, UInt uiAbsPartIdx, UInt uiPartIdx, UInt uiDepth )
{
  setSubPart( bResPredAvailable, m_pbResPredAvailable, uiAbsPartIdx, uiDepth, uiPartIdx );
}

Void TComDataCU::setResPredFlagSubParts( Bool bResPredFlag, UInt uiAbsPartIdx, UInt uiPartIdx, UInt uiDepth )
{
  setSubPart( bResPredFlag, m_pbResPredFlag, uiAbsPartIdx, uiDepth, uiPartIdx );
}
#endif

Void TComDataCU::setChromIntraDirSubParts( UInt uiDir, UInt uiAbsPartIdx, UInt uiDepth )
{
  UInt uiCurrPartNumb = m_pcPic->getNumPartInCU() >> (uiDepth << 1);
  
  memset( m_puhChromaIntraDir + uiAbsPartIdx, uiDir, sizeof(UChar)*uiCurrPartNumb );
}

Void TComDataCU::setInterDirSubParts( UInt uiDir, UInt uiAbsPartIdx, UInt uiPartIdx, UInt uiDepth )
{
  setSubPart<UChar>( uiDir, m_puhInterDir, uiAbsPartIdx, uiDepth, uiPartIdx );
}

#if LGE_ILLUCOMP_B0045
Void TComDataCU::setICFlagSubParts( Bool bICFlag, UInt uiAbsPartIdx, UInt uiPartIdx, UInt uiDepth )
{
  memset( m_pbICFlag + uiAbsPartIdx, bICFlag, (m_pcPic->getNumPartInCU() >> ( 2 * uiDepth ))*sizeof(Bool) );
}

#if LGE_ILLUCOMP_DEPTH_C0046
//This modification is not needed after integrating JCT3V-C0137
Bool TComDataCU::isICFlagRequired(UInt uiAbsPartIdx, UInt uiDepth)
{
  UInt uiPartAddr;
  UInt iNumbPart;

  if(!getSlice()->getIsDepth())
  {
    Int iWidth, iHeight;

    iNumbPart = ( getPartitionSize(uiAbsPartIdx) == SIZE_2Nx2N ? 1 : (getPartitionSize(uiAbsPartIdx) == SIZE_NxN ? 4 : 2) );

    for(UInt i = 0; i < iNumbPart; i++)
    {
      getPartIndexAndSize(i, uiPartAddr, iWidth, iHeight, uiAbsPartIdx, true);
      uiPartAddr += uiAbsPartIdx;

      for(UInt uiRefIdx = 0; uiRefIdx < 2; uiRefIdx++)
      {
        RefPicList eRefList = uiRefIdx ? REF_PIC_LIST_1 : REF_PIC_LIST_0;
        Int iBestRefIdx = getCUMvField(eRefList)->getRefIdx(uiPartAddr);

        if((getInterDir(uiPartAddr) & (uiRefIdx+1)) && iBestRefIdx >= 0 && getSlice()->getViewId() != getSlice()->getRefViewId(eRefList, iBestRefIdx))
        {
          return true;
        }
      }
    }
  }
  else
  {
    iNumbPart = getPic()->getNumPartInCU() >> (uiDepth << 1);

    for(UInt i = 0; i < iNumbPart; i++)
    {
      uiPartAddr = uiAbsPartIdx + i;

      for(UInt uiRefIdx = 0; uiRefIdx < 2; uiRefIdx++)
      {
        RefPicList eRefList = uiRefIdx ? REF_PIC_LIST_1 : REF_PIC_LIST_0;
        Int iBestRefIdx = getCUMvField(eRefList)->getRefIdx(uiPartAddr);

        if((getInterDir(uiPartAddr) & (uiRefIdx+1)) && iBestRefIdx >= 0 && getSlice()->getViewId() != getSlice()->getRefViewId(eRefList, iBestRefIdx))
        {
          return true;
        }
      }
    }
  }

  return false;
}
#else
Bool TComDataCU::isICFlagRequired(UInt uiAbsPartIdx)
{
  UInt uiPartAddr;
  UInt iNumbPart;
  Int iWidth, iHeight;

  UInt uiPartMode = getPartitionSize(uiAbsPartIdx);

  iNumbPart = (uiPartMode == SIZE_2Nx2N ? 1 : (uiPartMode == SIZE_NxN ? 4 : 2) );

  for(UInt i = 0; i < iNumbPart; i++)
  {
    getPartIndexAndSize(i, uiPartAddr, iWidth, iHeight, uiAbsPartIdx, true);
    uiPartAddr += uiAbsPartIdx;

    for(UInt uiRefIdx = 0; uiRefIdx < 2; uiRefIdx++)
    {
      RefPicList eRefList = uiRefIdx ? REF_PIC_LIST_1 : REF_PIC_LIST_0;
      Int iBestRefIdx = getCUMvField(eRefList)->getRefIdx(uiPartAddr);

      if((getInterDir(uiPartAddr) & (uiRefIdx+1)) && iBestRefIdx >= 0 && getSlice()->getViewId() != getSlice()->getRefViewId(eRefList, iBestRefIdx))
      {
        return true;
    }
  }
  }
  return false;
}
#endif
#endif

Void TComDataCU::setMVPIdxSubParts( Int iMVPIdx, RefPicList eRefPicList, UInt uiAbsPartIdx, UInt uiPartIdx, UInt uiDepth )
{
  setSubPart<Char>( iMVPIdx, m_apiMVPIdx[eRefPicList], uiAbsPartIdx, uiDepth, uiPartIdx );
}

Void TComDataCU::setMVPNumSubParts( Int iMVPNum, RefPicList eRefPicList, UInt uiAbsPartIdx, UInt uiPartIdx, UInt uiDepth )
{
  setSubPart<Char>( iMVPNum, m_apiMVPNum[eRefPicList], uiAbsPartIdx, uiDepth, uiPartIdx );
}


Void TComDataCU::setTrIdxSubParts( UInt uiTrIdx, UInt uiAbsPartIdx, UInt uiDepth )
{
  UInt uiCurrPartNumb = m_pcPic->getNumPartInCU() >> (uiDepth << 1);
  
  memset( m_puhTrIdx + uiAbsPartIdx, uiTrIdx, sizeof(UChar)*uiCurrPartNumb );
}

Void TComDataCU::setNSQTIdxSubParts( UInt absPartIdx, UInt depth )
{
  UInt currPartNumb = m_pcPic->getNumPartInCU() >> (depth << 1);
  
  memset( m_nsqtPartIdx + absPartIdx, absPartIdx, sizeof(UChar)*currPartNumb );
}

Void  TComDataCU::setNSQTIdxSubParts( UInt log2TrafoSize, UInt absPartIdx, UInt absTUPartIdx, UInt trMode )
{
  UInt trWidth, trHeight;
  UInt nsTUWidthInBaseUnits, nsTUHeightInBaseUnits;
  UInt lcuWidthInBaseUnits = getPic()->getNumPartInWidth();
  UInt minTuSize = ( 1 << getSlice()->getSPS()->getQuadtreeTULog2MinSize() );

  trWidth = trHeight = ( 1 << log2TrafoSize );

  if ( useNonSquareTrans( trMode, absPartIdx ) && ( trWidth > minTuSize ) )
  {
    trWidth  = ( m_pePartSize[absPartIdx] == SIZE_Nx2N || m_pePartSize[absPartIdx] == SIZE_nLx2N || m_pePartSize[absPartIdx] == SIZE_nRx2N )? trWidth >> 1  : trWidth << 1;
    trHeight = ( m_pePartSize[absPartIdx] == SIZE_Nx2N || m_pePartSize[absPartIdx] == SIZE_nLx2N || m_pePartSize[absPartIdx] == SIZE_nRx2N )? trHeight << 1 : trHeight >> 1;
  }

  nsTUWidthInBaseUnits  = trWidth / getPic()->getMinCUWidth();
  nsTUHeightInBaseUnits = trHeight / getPic()->getMinCUHeight();
  
  if ( nsTUWidthInBaseUnits > nsTUHeightInBaseUnits )
  {
    UInt currPartNumb = nsTUHeightInBaseUnits*nsTUHeightInBaseUnits;
    memset( m_nsqtPartIdx + absTUPartIdx                                                                , absPartIdx, sizeof(UChar)*(currPartNumb) );
    memset( m_nsqtPartIdx + g_auiRasterToZscan[g_auiZscanToRaster[absTUPartIdx]+  nsTUHeightInBaseUnits], absPartIdx, sizeof(UChar)*(currPartNumb) );
    memset( m_nsqtPartIdx + g_auiRasterToZscan[g_auiZscanToRaster[absTUPartIdx]+2*nsTUHeightInBaseUnits], absPartIdx, sizeof(UChar)*(currPartNumb) );
    memset( m_nsqtPartIdx + g_auiRasterToZscan[g_auiZscanToRaster[absTUPartIdx]+3*nsTUHeightInBaseUnits], absPartIdx, sizeof(UChar)*(currPartNumb) );
  }
  else if ( nsTUWidthInBaseUnits < nsTUHeightInBaseUnits )
  {
    UInt currPartNumb = nsTUWidthInBaseUnits*nsTUWidthInBaseUnits;
    memset( m_nsqtPartIdx + absTUPartIdx                                                                                   , absPartIdx, sizeof(UChar)*(currPartNumb) );
    memset( m_nsqtPartIdx + g_auiRasterToZscan[g_auiZscanToRaster[absTUPartIdx]+  nsTUWidthInBaseUnits*lcuWidthInBaseUnits], absPartIdx, sizeof(UChar)*(currPartNumb) );
    memset( m_nsqtPartIdx + g_auiRasterToZscan[g_auiZscanToRaster[absTUPartIdx]+2*nsTUWidthInBaseUnits*lcuWidthInBaseUnits], absPartIdx, sizeof(UChar)*(currPartNumb) );
    memset( m_nsqtPartIdx + g_auiRasterToZscan[g_auiZscanToRaster[absTUPartIdx]+3*nsTUWidthInBaseUnits*lcuWidthInBaseUnits], absPartIdx, sizeof(UChar)*(currPartNumb) );
  }
  else
  {
    UInt currPartNumb = nsTUWidthInBaseUnits*nsTUHeightInBaseUnits;
    memset( m_nsqtPartIdx + absTUPartIdx, absPartIdx, sizeof(UChar)*(currPartNumb) );
  }
}

Void TComDataCU::setSizeSubParts( UInt uiWidth, UInt uiHeight, UInt uiAbsPartIdx, UInt uiDepth )
{
  UInt uiCurrPartNumb = m_pcPic->getNumPartInCU() >> (uiDepth << 1);
  
  memset( m_puhWidth  + uiAbsPartIdx, uiWidth,  sizeof(UChar)*uiCurrPartNumb );
  memset( m_puhHeight + uiAbsPartIdx, uiHeight, sizeof(UChar)*uiCurrPartNumb );
}

UChar TComDataCU::getNumPartInter()
{
  UChar iNumPart = 0;
  
  switch ( m_pePartSize[0] )
  {
    case SIZE_2Nx2N:    iNumPart = 1; break;
    case SIZE_2NxN:     iNumPart = 2; break;
    case SIZE_Nx2N:     iNumPart = 2; break;
    case SIZE_NxN:      iNumPart = 4; break;
    case SIZE_2NxnU:    iNumPart = 2; break;
    case SIZE_2NxnD:    iNumPart = 2; break;
    case SIZE_nLx2N:    iNumPart = 2; break;
    case SIZE_nRx2N:    iNumPart = 2; break;
    default:            assert (0);   break;
  }
  
  return  iNumPart;
}

#if H3D_IVRP
Void TComDataCU::getPartIndexAndSize( UInt uiPartIdx, UInt& ruiPartAddr, Int& riWidth, Int& riHeight, UInt uiAbsPartIdx, Bool bLCU)
{
  UInt uiNumPartition  = bLCU ? (getWidth(uiAbsPartIdx)*getHeight(uiAbsPartIdx) >> 4) : m_uiNumPartition;
  UInt uiTmpAbsPartIdx = bLCU ? uiAbsPartIdx : 0;

  switch ( m_pePartSize[uiTmpAbsPartIdx] )
  {
  case SIZE_2NxN:
    riWidth = getWidth(uiTmpAbsPartIdx);      riHeight = getHeight(uiTmpAbsPartIdx) >> 1; ruiPartAddr = ( uiPartIdx == 0 )? 0 : uiNumPartition >> 1;
    break;
  case SIZE_Nx2N:
    riWidth = getWidth(uiTmpAbsPartIdx) >> 1; riHeight = getHeight(uiTmpAbsPartIdx);      ruiPartAddr = ( uiPartIdx == 0 )? 0 : uiNumPartition >> 2;
    break;
  case SIZE_NxN:
    riWidth = getWidth(uiTmpAbsPartIdx) >> 1; riHeight = getHeight(uiTmpAbsPartIdx) >> 1; ruiPartAddr = ( uiNumPartition >> 2 ) * uiPartIdx;
    break;
  case SIZE_2NxnU:
    riWidth     = getWidth(uiTmpAbsPartIdx);
    riHeight    = ( uiPartIdx == 0 ) ?  getHeight(uiTmpAbsPartIdx) >> 2 : ( getHeight(uiTmpAbsPartIdx) >> 2 ) + ( getHeight(uiTmpAbsPartIdx) >> 1 );
    ruiPartAddr = ( uiPartIdx == 0 ) ? 0 : uiNumPartition >> 3;
    break;
  case SIZE_2NxnD:
    riWidth     = getWidth(uiTmpAbsPartIdx);
    riHeight    = ( uiPartIdx == 0 ) ?  ( getHeight(uiTmpAbsPartIdx) >> 2 ) + ( getHeight(uiTmpAbsPartIdx) >> 1 ) : getHeight(uiTmpAbsPartIdx) >> 2;
    ruiPartAddr = ( uiPartIdx == 0 ) ? 0 : (uiNumPartition >> 1) + (uiNumPartition >> 3);
    break;
  case SIZE_nLx2N:
    riWidth     = ( uiPartIdx == 0 ) ? getWidth(uiTmpAbsPartIdx) >> 2 : ( getWidth(uiTmpAbsPartIdx) >> 2 ) + ( getWidth(uiTmpAbsPartIdx) >> 1 );
    riHeight    = getHeight(uiTmpAbsPartIdx);
    ruiPartAddr = ( uiPartIdx == 0 ) ? 0 : uiNumPartition >> 4;
    break;
  case SIZE_nRx2N:
    riWidth     = ( uiPartIdx == 0 ) ? ( getWidth(uiTmpAbsPartIdx) >> 2 ) + ( getWidth(uiTmpAbsPartIdx) >> 1 ) : getWidth(uiTmpAbsPartIdx) >> 2;
    riHeight    = getHeight(uiTmpAbsPartIdx);
    ruiPartAddr = ( uiPartIdx == 0 ) ? 0 : (uiNumPartition >> 2) + (uiNumPartition >> 4);
    break;
  default:
    assert ( m_pePartSize[uiTmpAbsPartIdx ] == SIZE_2Nx2N ); 
    riWidth = getWidth(uiTmpAbsPartIdx);      riHeight = getHeight(uiTmpAbsPartIdx);      ruiPartAddr = 0;
    break;
  }
}
#else
Void TComDataCU::getPartIndexAndSize( UInt uiPartIdx, UInt& ruiPartAddr, Int& riWidth, Int& riHeight )
{
  switch ( m_pePartSize[0] )
  {
    case SIZE_2NxN:
      riWidth = getWidth(0);      riHeight = getHeight(0) >> 1; ruiPartAddr = ( uiPartIdx == 0 )? 0 : m_uiNumPartition >> 1;
      break;
    case SIZE_Nx2N:
      riWidth = getWidth(0) >> 1; riHeight = getHeight(0);      ruiPartAddr = ( uiPartIdx == 0 )? 0 : m_uiNumPartition >> 2;
      break;
    case SIZE_NxN:
      riWidth = getWidth(0) >> 1; riHeight = getHeight(0) >> 1; ruiPartAddr = ( m_uiNumPartition >> 2 ) * uiPartIdx;
      break;
    case SIZE_2NxnU:
      riWidth     = getWidth(0);
      riHeight    = ( uiPartIdx == 0 ) ?  getHeight(0) >> 2 : ( getHeight(0) >> 2 ) + ( getHeight(0) >> 1 );
      ruiPartAddr = ( uiPartIdx == 0 ) ? 0 : m_uiNumPartition >> 3;
      break;
    case SIZE_2NxnD:
      riWidth     = getWidth(0);
      riHeight    = ( uiPartIdx == 0 ) ?  ( getHeight(0) >> 2 ) + ( getHeight(0) >> 1 ) : getHeight(0) >> 2;
      ruiPartAddr = ( uiPartIdx == 0 ) ? 0 : (m_uiNumPartition >> 1) + (m_uiNumPartition >> 3);
      break;
    case SIZE_nLx2N:
      riWidth     = ( uiPartIdx == 0 ) ? getWidth(0) >> 2 : ( getWidth(0) >> 2 ) + ( getWidth(0) >> 1 );
      riHeight    = getHeight(0);
      ruiPartAddr = ( uiPartIdx == 0 ) ? 0 : m_uiNumPartition >> 4;
      break;
    case SIZE_nRx2N:
      riWidth     = ( uiPartIdx == 0 ) ? ( getWidth(0) >> 2 ) + ( getWidth(0) >> 1 ) : getWidth(0) >> 2;
      riHeight    = getHeight(0);
      ruiPartAddr = ( uiPartIdx == 0 ) ? 0 : (m_uiNumPartition >> 2) + (m_uiNumPartition >> 4);
      break;
    default:
      assert ( m_pePartSize[0] == SIZE_2Nx2N );
      riWidth = getWidth(0);      riHeight = getHeight(0);      ruiPartAddr = 0;
      break;
  }
}
#endif

Void TComDataCU::getMvField ( TComDataCU* pcCU, UInt uiAbsPartIdx, RefPicList eRefPicList, TComMvField& rcMvField )
{
  if ( pcCU == NULL )  // OUT OF BOUNDARY
  {
    TComMv  cZeroMv;
    rcMvField.setMvField( cZeroMv, NOT_VALID );
    return;
  }
  
  TComCUMvField*  pcCUMvField = pcCU->getCUMvField( eRefPicList );
  rcMvField.setMvField( pcCUMvField->getMv( uiAbsPartIdx ), pcCUMvField->getRefIdx( uiAbsPartIdx ) );
}

Void TComDataCU::deriveLeftRightTopIdxGeneral ( PartSize eCUMode, UInt uiAbsPartIdx, UInt uiPartIdx, UInt& ruiPartIdxLT, UInt& ruiPartIdxRT )
{
  ruiPartIdxLT = m_uiAbsIdxInLCU + uiAbsPartIdx;
  UInt uiPUWidth = 0;
  
  switch ( m_pePartSize[uiAbsPartIdx] )
  {
    case SIZE_2Nx2N: uiPUWidth = m_puhWidth[uiAbsPartIdx];  break;
    case SIZE_2NxN:  uiPUWidth = m_puhWidth[uiAbsPartIdx];   break;
    case SIZE_Nx2N:  uiPUWidth = m_puhWidth[uiAbsPartIdx]  >> 1;  break;
    case SIZE_NxN:   uiPUWidth = m_puhWidth[uiAbsPartIdx]  >> 1; break;
    case SIZE_2NxnU:   uiPUWidth = m_puhWidth[uiAbsPartIdx]; break;
    case SIZE_2NxnD:   uiPUWidth = m_puhWidth[uiAbsPartIdx]; break;
    case SIZE_nLx2N:   
      if ( uiPartIdx == 0 )
      {
        uiPUWidth = m_puhWidth[uiAbsPartIdx]  >> 2; 
      }
      else if ( uiPartIdx == 1 )
      {
        uiPUWidth = (m_puhWidth[uiAbsPartIdx]  >> 1) + (m_puhWidth[uiAbsPartIdx]  >> 2); 
      }
      else
      {
        assert(0);
      }
      break;
    case SIZE_nRx2N:   
      if ( uiPartIdx == 0 )
      {
        uiPUWidth = (m_puhWidth[uiAbsPartIdx]  >> 1) + (m_puhWidth[uiAbsPartIdx]  >> 2); 
      }
      else if ( uiPartIdx == 1 )
      {
        uiPUWidth = m_puhWidth[uiAbsPartIdx]  >> 2; 
      }
      else
      {
        assert(0);
      }
      break;
    default:
      assert (0);
      break;
  }
  
  ruiPartIdxRT = g_auiRasterToZscan [g_auiZscanToRaster[ ruiPartIdxLT ] + uiPUWidth / m_pcPic->getMinCUWidth() - 1 ];
}

Void TComDataCU::deriveLeftBottomIdxGeneral( PartSize eCUMode, UInt uiAbsPartIdx, UInt uiPartIdx, UInt& ruiPartIdxLB )
{
  UInt uiPUHeight = 0;
  switch ( m_pePartSize[uiAbsPartIdx] )
  {
    case SIZE_2Nx2N: uiPUHeight = m_puhHeight[uiAbsPartIdx];    break;
    case SIZE_2NxN:  uiPUHeight = m_puhHeight[uiAbsPartIdx] >> 1;    break;
    case SIZE_Nx2N:  uiPUHeight = m_puhHeight[uiAbsPartIdx];  break;
    case SIZE_NxN:   uiPUHeight = m_puhHeight[uiAbsPartIdx] >> 1;    break;
    case SIZE_2NxnU: 
      if ( uiPartIdx == 0 )
      {
        uiPUHeight = m_puhHeight[uiAbsPartIdx] >> 2;    
      }
      else if ( uiPartIdx == 1 )
      {
        uiPUHeight = (m_puhHeight[uiAbsPartIdx] >> 1) + (m_puhHeight[uiAbsPartIdx] >> 2);    
      }
      else
      {
        assert(0);
      }
      break;
    case SIZE_2NxnD: 
      if ( uiPartIdx == 0 )
      {
        uiPUHeight = (m_puhHeight[uiAbsPartIdx] >> 1) + (m_puhHeight[uiAbsPartIdx] >> 2);    
      }
      else if ( uiPartIdx == 1 )
      {
        uiPUHeight = m_puhHeight[uiAbsPartIdx] >> 2;    
      }
      else
      {
        assert(0);
      }
      break;
    case SIZE_nLx2N: uiPUHeight = m_puhHeight[uiAbsPartIdx];  break;
    case SIZE_nRx2N: uiPUHeight = m_puhHeight[uiAbsPartIdx];  break;
    default:
      assert (0);
      break;
  }
  
  ruiPartIdxLB      = g_auiRasterToZscan [g_auiZscanToRaster[ m_uiAbsIdxInLCU + uiAbsPartIdx ] + ((uiPUHeight / m_pcPic->getMinCUHeight()) - 1)*m_pcPic->getNumPartInWidth()];
}

Void TComDataCU::deriveLeftRightTopIdx ( PartSize eCUMode, UInt uiPartIdx, UInt& ruiPartIdxLT, UInt& ruiPartIdxRT )
{
  ruiPartIdxLT = m_uiAbsIdxInLCU;
  ruiPartIdxRT = g_auiRasterToZscan [g_auiZscanToRaster[ ruiPartIdxLT ] + m_puhWidth[0] / m_pcPic->getMinCUWidth() - 1 ];
  
  switch ( m_pePartSize[0] )
  {
    case SIZE_2Nx2N:                                                                                                                                break;
    case SIZE_2NxN:
      ruiPartIdxLT += ( uiPartIdx == 0 )? 0 : m_uiNumPartition >> 1; ruiPartIdxRT += ( uiPartIdx == 0 )? 0 : m_uiNumPartition >> 1;
      break;
    case SIZE_Nx2N:
      ruiPartIdxLT += ( uiPartIdx == 0 )? 0 : m_uiNumPartition >> 2; ruiPartIdxRT -= ( uiPartIdx == 1 )? 0 : m_uiNumPartition >> 2;
      break;
    case SIZE_NxN:
      ruiPartIdxLT += ( m_uiNumPartition >> 2 ) * uiPartIdx;         ruiPartIdxRT +=  ( m_uiNumPartition >> 2 ) * ( uiPartIdx - 1 );
      break;
    case SIZE_2NxnU:
      ruiPartIdxLT += ( uiPartIdx == 0 )? 0 : m_uiNumPartition >> 3;
      ruiPartIdxRT += ( uiPartIdx == 0 )? 0 : m_uiNumPartition >> 3;
      break;
    case SIZE_2NxnD:
      ruiPartIdxLT += ( uiPartIdx == 0 )? 0 : ( m_uiNumPartition >> 1 ) + ( m_uiNumPartition >> 3 );
      ruiPartIdxRT += ( uiPartIdx == 0 )? 0 : ( m_uiNumPartition >> 1 ) + ( m_uiNumPartition >> 3 );
      break;
    case SIZE_nLx2N:
      ruiPartIdxLT += ( uiPartIdx == 0 )? 0 : m_uiNumPartition >> 4;
      ruiPartIdxRT -= ( uiPartIdx == 1 )? 0 : ( m_uiNumPartition >> 2 ) + ( m_uiNumPartition >> 4 );
      break;
    case SIZE_nRx2N:
      ruiPartIdxLT += ( uiPartIdx == 0 )? 0 : ( m_uiNumPartition >> 2 ) + ( m_uiNumPartition >> 4 );
      ruiPartIdxRT -= ( uiPartIdx == 1 )? 0 : m_uiNumPartition >> 4;
      break;
    default:
      assert (0);
      break;
  }
  
}

Void TComDataCU::deriveLeftBottomIdx( PartSize      eCUMode,   UInt  uiPartIdx,      UInt&      ruiPartIdxLB )
{
  ruiPartIdxLB      = g_auiRasterToZscan [g_auiZscanToRaster[ m_uiAbsIdxInLCU ] + ( ((m_puhHeight[0] / m_pcPic->getMinCUHeight())>>1) - 1)*m_pcPic->getNumPartInWidth()];
  
  switch ( m_pePartSize[0] )
  {
    case SIZE_2Nx2N:
      ruiPartIdxLB += m_uiNumPartition >> 1;
      break;
    case SIZE_2NxN:
      ruiPartIdxLB += ( uiPartIdx == 0 )? 0 : m_uiNumPartition >> 1;
      break;
    case SIZE_Nx2N:
      ruiPartIdxLB += ( uiPartIdx == 0 )? m_uiNumPartition >> 1 : (m_uiNumPartition >> 2)*3;
      break;
    case SIZE_NxN:
      ruiPartIdxLB += ( m_uiNumPartition >> 2 ) * uiPartIdx;
      break;
    case SIZE_2NxnU:
      ruiPartIdxLB += ( uiPartIdx == 0 ) ? -((Int)m_uiNumPartition >> 3) : m_uiNumPartition >> 1;
      break;
    case SIZE_2NxnD:
      ruiPartIdxLB += ( uiPartIdx == 0 ) ? (m_uiNumPartition >> 2) + (m_uiNumPartition >> 3): m_uiNumPartition >> 1;
      break;
    case SIZE_nLx2N:
      ruiPartIdxLB += ( uiPartIdx == 0 ) ? m_uiNumPartition >> 1 : (m_uiNumPartition >> 1) + (m_uiNumPartition >> 4);
      break;
    case SIZE_nRx2N:
      ruiPartIdxLB += ( uiPartIdx == 0 ) ? m_uiNumPartition >> 1 : (m_uiNumPartition >> 1) + (m_uiNumPartition >> 2) + (m_uiNumPartition >> 4);
      break;
    default:
      assert (0);
      break;
  }
}

/** Derives the partition index of neighbouring bottom right block
 * \param [in]  eCUMode
 * \param [in]  uiPartIdx 
 * \param [out] ruiPartIdxRB 
 */
Void TComDataCU::deriveRightBottomIdx( PartSize      eCUMode,   UInt  uiPartIdx,      UInt&      ruiPartIdxRB )
{
  ruiPartIdxRB      = g_auiRasterToZscan [g_auiZscanToRaster[ m_uiAbsIdxInLCU ] + ( ((m_puhHeight[0] / m_pcPic->getMinCUHeight())>>1) - 1)*m_pcPic->getNumPartInWidth() +  m_puhWidth[0] / m_pcPic->getMinCUWidth() - 1];

  switch ( m_pePartSize[0] )
  {
    case SIZE_2Nx2N:  
      ruiPartIdxRB += m_uiNumPartition >> 1;    
      break;
    case SIZE_2NxN:  
      ruiPartIdxRB += ( uiPartIdx == 0 )? 0 : m_uiNumPartition >> 1;   
      break;
    case SIZE_Nx2N:  
      ruiPartIdxRB += ( uiPartIdx == 0 )? m_uiNumPartition >> 2 : (m_uiNumPartition >> 1);   
      break;
    case SIZE_NxN:   
      ruiPartIdxRB += ( m_uiNumPartition >> 2 ) * ( uiPartIdx - 1 );   
      break;
    case SIZE_2NxnU:
      ruiPartIdxRB += ( uiPartIdx == 0 ) ? -((Int)m_uiNumPartition >> 3) : m_uiNumPartition >> 1;
      break;
    case SIZE_2NxnD:
      ruiPartIdxRB += ( uiPartIdx == 0 ) ? (m_uiNumPartition >> 2) + (m_uiNumPartition >> 3): m_uiNumPartition >> 1;
      break;
    case SIZE_nLx2N:
      ruiPartIdxRB += ( uiPartIdx == 0 ) ? (m_uiNumPartition >> 3) + (m_uiNumPartition >> 4): m_uiNumPartition >> 1;
      break;
    case SIZE_nRx2N:
      ruiPartIdxRB += ( uiPartIdx == 0 ) ? (m_uiNumPartition >> 2) + (m_uiNumPartition >> 3) + (m_uiNumPartition >> 4) : m_uiNumPartition >> 1;
      break;
    default:
      assert (0);
      break;
  }
}

Void TComDataCU::deriveLeftRightTopIdxAdi ( UInt& ruiPartIdxLT, UInt& ruiPartIdxRT, UInt uiPartOffset, UInt uiPartDepth )
{
  UInt uiNumPartInWidth = (m_puhWidth[0]/m_pcPic->getMinCUWidth())>>uiPartDepth;
  ruiPartIdxLT = m_uiAbsIdxInLCU + uiPartOffset;
  ruiPartIdxRT = g_auiRasterToZscan[ g_auiZscanToRaster[ ruiPartIdxLT ] + uiNumPartInWidth - 1 ];
}

Void TComDataCU::deriveLeftBottomIdxAdi( UInt& ruiPartIdxLB, UInt uiPartOffset, UInt uiPartDepth )
{
  UInt uiAbsIdx;
  UInt uiMinCuWidth, uiWidthInMinCus;
  
  uiMinCuWidth    = getPic()->getMinCUWidth();
  uiWidthInMinCus = (getWidth(0)/uiMinCuWidth)>>uiPartDepth;
  uiAbsIdx        = getZorderIdxInCU()+uiPartOffset+(m_uiNumPartition>>(uiPartDepth<<1))-1;
  uiAbsIdx        = g_auiZscanToRaster[uiAbsIdx]-(uiWidthInMinCus-1);
  ruiPartIdxLB    = g_auiRasterToZscan[uiAbsIdx];
}

Bool TComDataCU::hasEqualMotion( UInt uiAbsPartIdx, TComDataCU* pcCandCU, UInt uiCandAbsPartIdx )
{

  if ( getInterDir( uiAbsPartIdx ) != pcCandCU->getInterDir( uiCandAbsPartIdx ) )
  {
    return false;
  }

  for ( UInt uiRefListIdx = 0; uiRefListIdx < 2; uiRefListIdx++ )
  {
    if ( getInterDir( uiAbsPartIdx ) & ( 1 << uiRefListIdx ) )
    {
      if ( getCUMvField( RefPicList( uiRefListIdx ) )->getMv( uiAbsPartIdx )     != pcCandCU->getCUMvField( RefPicList( uiRefListIdx ) )->getMv( uiCandAbsPartIdx ) || 
        getCUMvField( RefPicList( uiRefListIdx ) )->getRefIdx( uiAbsPartIdx ) != pcCandCU->getCUMvField( RefPicList( uiRefListIdx ) )->getRefIdx( uiCandAbsPartIdx ) )
      {
        return false;
      }
    }
  }

  return true;
}

/** Constructs a list of merging candidates
 * \param uiAbsPartIdx
 * \param uiPUIdx 
 * \param uiDepth
 * \param pcMvFieldNeighbours
 * \param puhInterDirNeighbours
 * \param numValidMergeCand
 */
#if MERL_VSP_C0152
Void TComDataCU::getInterMergeCandidates( UInt uiAbsPartIdx, UInt uiPUIdx, UInt uiDepth, TComMvField* pcMvFieldNeighbours, UChar* puhInterDirNeighbours, Int& numValidMergeCand, Int* iVSPIndexTrue, Int mrgCandIdx )
#else
Void TComDataCU::getInterMergeCandidates( UInt uiAbsPartIdx, UInt uiPUIdx, UInt uiDepth, TComMvField* pcMvFieldNeighbours, UChar* puhInterDirNeighbours, Int& numValidMergeCand, Int mrgCandIdx )
#endif
{
#if H3D_IVMP
#if MTK_DEPTH_MERGE_TEXTURE_CANDIDATE_C0137
#if FCO_FIX
  const Int extraMergeCand = ( ( ( getSlice()->getIsDepth() && m_pcSlice->getTexturePic() ) || getSlice()->getSPS()->getMultiviewMvPredMode() )? 1 : 0 );
#else
  const Int extraMergeCand = ( ( getSlice()->getIsDepth() || getSlice()->getSPS()->getMultiviewMvPredMode() )? 1 : 0 );
#endif
#else
  const Int extraMergeCand = ( getSlice()->getSPS()->getMultiviewMvPredMode() ? 1 : 0 );
#endif
#endif

  UInt uiAbsPartAddr = m_uiAbsIdxInLCU + uiAbsPartIdx;
  UInt uiIdx = 1;
#if H3D_IVMP
  Bool abCandIsInter[ MRG_MAX_NUM_CANDS_MEM ];
  for( UInt ui = 0; ui < MRG_MAX_NUM_CANDS_MEM; ++ui )
#else
  Bool abCandIsInter[ MRG_MAX_NUM_CANDS ];
  for( UInt ui = 0; ui < MRG_MAX_NUM_CANDS; ++ui )
#endif
  {
    abCandIsInter[ui] = false;
  }
  // compute the location of the current PU
  Int xP, yP, nPSW, nPSH;
  this->getPartPosition(uiPUIdx, xP, yP, nPSW, nPSH);

#if MERL_VSP_C0152
  Bool bVspMvZeroDone[3] = {false, false, false};
#endif
  
  Int iCount = 0;

  UInt uiPartIdxLT, uiPartIdxRT, uiPartIdxLB;
  PartSize cCurPS = getPartitionSize( uiAbsPartIdx );
  deriveLeftRightTopIdxGeneral( cCurPS, uiAbsPartIdx, uiPUIdx, uiPartIdxLT, uiPartIdxRT );
  deriveLeftBottomIdxGeneral( cCurPS, uiAbsPartIdx, uiPUIdx, uiPartIdxLB );

#if H3D_IVMP
  Bool bNoPdmMerge   = ( m_pcSlice->getSPS()->getViewId() == 0 || ( m_pcSlice->getSPS()->getMultiviewMvPredMode() & PDM_USE_FOR_MERGE ) != PDM_USE_FOR_MERGE );

  //===== add merge with predicted depth maps =====
  TComMv  acPdmMv       [4];
  Int     aiPdmRefIdx   [4] = {-1, -1, -1, -1};
  Bool    bLeftAvai         = false;
  Int     iPosLeftAbove[2]  = {-1, -1};

#if H3D_NBDV
  acPdmMv[0].m_bDvMcp = acPdmMv[1].m_bDvMcp = acPdmMv[2].m_bDvMcp = acPdmMv[3].m_bDvMcp = false; 
#endif //H3D_NBDV

#if H3D_IVRP
  Bool bDVAvail = true;
#endif

#if H3D_NBDV
  DisInfo cDisInfo;
  cDisInfo.iN = 0;
  if(!bNoPdmMerge)
  {
#if FCO_DVP_REFINE_C0132_C0170
    if( !getPic()->getDepthCoded() )
#endif
    getDisMvpCandNBDV(uiPUIdx, uiAbsPartIdx, &cDisInfo , true
#if MERL_VSP_C0152
            , true
#endif
);
  }
#if FCO_DVP_REFINE_C0132_C0170
  if(getPic()->getDepthCoded() )
  {
    TComPic*      pcCodedDepthMap = getPic()->getRecDepthMap();
    TComMv        cColMv;

    cColMv.setZero();
    estimateDVFromDM(uiPUIdx, pcCodedDepthMap, uiAbsPartIdx, &cColMv, false);

    cDisInfo.iN = 1;
    cDisInfo.m_acMvCand[0].setHor( cColMv.getHor() );
    cDisInfo.m_acMvCand[0].setVer( cColMv.getVer() );
    cDisInfo.m_aVIdxCan[0] = 0;

  }
#endif
  if(cDisInfo.iN==0)
  {
    cDisInfo.iN = 1;
    cDisInfo.m_acMvCand[0].setHor(0);
    cDisInfo.m_acMvCand[0].setVer(0);
    cDisInfo.m_aVIdxCan[0] = 0;
#if H3D_IVRP
    bDVAvail = false;
#endif
  }

#if MTK_DEPTH_MERGE_TEXTURE_CANDIDATE_C0137
#if FCO_FIX
  if( m_pcSlice->getIsDepth() && m_pcSlice->getTexturePic() )
#else
  if( m_pcSlice->getIsDepth())
#endif
  {
    UInt uiPartIdxCenter;
    xDeriveCenterIdx( cCurPS, uiPUIdx, uiPartIdxCenter );    
    TComDataCU *pcTextureCU = m_pcSlice->getTexturePic()->getCU( getAddr() );
    if ( pcTextureCU && !pcTextureCU->isIntra( uiPartIdxCenter ) )
    {
      abCandIsInter[iCount] = true;      
      puhInterDirNeighbours[iCount] = pcTextureCU->getInterDir( uiPartIdxCenter );
      if( ( puhInterDirNeighbours[iCount] & 1 ) == 1 )
      {
        pcTextureCU->getMvField( pcTextureCU, uiPartIdxCenter, REF_PIC_LIST_0, pcMvFieldNeighbours[iCount<<1] );
        TComMv cMvPred = pcMvFieldNeighbours[iCount<<1].getMv();
        const TComMv cAdd( 1 << ( 2 - 1 ), 1 << ( 2 - 1 ) );
        cMvPred+=cAdd;
        cMvPred>>=2;
        clipMv(cMvPred);
        pcMvFieldNeighbours[iCount<<1].setMvField(cMvPred,pcMvFieldNeighbours[iCount<<1].getRefIdx());
      }
      if ( getSlice()->isInterB() )
      {
        if( ( puhInterDirNeighbours[iCount] & 2 ) == 2 )
        {
          pcTextureCU->getMvField( pcTextureCU, uiPartIdxCenter, REF_PIC_LIST_1, pcMvFieldNeighbours[(iCount<<1)+1] );
          TComMv cMvPred = pcMvFieldNeighbours[(iCount<<1)+1].getMv();
          const TComMv cAdd( 1 << ( 2 - 1 ), 1 << ( 2 - 1 ) );
          cMvPred+=cAdd;
          cMvPred>>=2;
          clipMv(cMvPred);
          pcMvFieldNeighbours[(iCount<<1)+1].setMvField(cMvPred,pcMvFieldNeighbours[(iCount<<1)+1].getRefIdx());
        }
      }
#if MERL_VSP_C0152
      xInheritVspMode( pcTextureCU, uiPartIdxCenter, bVspMvZeroDone, iCount, iVSPIndexTrue, pcMvFieldNeighbours, &cDisInfo ) ;
#endif
      if ( mrgCandIdx == iCount )
      {
        return;
      }
      iCount ++;
    }
  }
#endif

  Int iPdmDir[2] = {0, 0};
  getUnifiedMvPredCan(uiPUIdx, REF_PIC_LIST_0, 0, aiPdmRefIdx, acPdmMv, &cDisInfo, iPdmDir, true);
  Int iPdmInterDir;
#else // H3D_NBDV
  Int iPdmDir[2] = {0, 0};
  Int     iPdmInterDir      = getPdmMergeCandidate( uiPUIdx, aiPdmRefIdx, acPdmMv );
  iPdmDir[0] = iPdmInterDir; 
  iPdmDir[1] = iPdmInterDir; 
#endif // H3D_NBDV
#if H3D_IVRP
  if (m_pcSlice->getSPS()->getMultiviewResPredMode()==1 && iPdmDir[0] && !bNoPdmMerge && cCurPS == SIZE_2Nx2N && bDVAvail)
  {
    setResPredAvailSubParts(true, 0, 0, uiDepth);
  }
#endif

  if( iPdmDir[0] && !bNoPdmMerge )
  {
    abCandIsInter        [ iCount ] = true;
    puhInterDirNeighbours[ iCount ] = iPdmDir[0];
    iPdmInterDir                    = iPdmDir[0];

    if( ( iPdmInterDir & 1 ) == 1 )
    {
      pcMvFieldNeighbours[ iCount<<1    ].setMvField( acPdmMv[ 0 ], aiPdmRefIdx[ 0 ] );
    }
#if FIX_CU_BASED_MRG_CAND_LIST_B0136
    else
    {
      pcMvFieldNeighbours[ iCount<<1    ].setMvField( TComMv(0,0), NOT_VALID );
    }
#endif
    if( ( iPdmInterDir & 2 ) == 2 )
    {
      pcMvFieldNeighbours[(iCount<<1)+1 ].setMvField( acPdmMv[ 1 ], aiPdmRefIdx[ 1 ] );
    }
#if FIX_CU_BASED_MRG_CAND_LIST_B0136
    else
    {
      pcMvFieldNeighbours[(iCount<<1)+1 ].setMvField( TComMv(0,0), NOT_VALID );
    }
#endif
    if ( mrgCandIdx == iCount )
    {
      return;
    }
    iCount ++;
  }  
  
#endif //  H3D_IVMP 

#if MERL_VSP_COMPENSATION_C0152
  //===== vsp 0 ===== 
  if( iCount < 4 + extraMergeCand )
    if ( !xAddVspMergeCand(0, 1, bVspMvZeroDone, uiDepth, abCandIsInter, iCount, puhInterDirNeighbours, pcMvFieldNeighbours, iVSPIndexTrue, mrgCandIdx, &cDisInfo) )
      return;
#endif

  //left
  UInt uiLeftPartIdx = 0;
  TComDataCU* pcCULeft = 0;
  pcCULeft = getPULeft( uiLeftPartIdx, uiPartIdxLB, true, false );
  if (pcCULeft) 
  {
    if (!pcCULeft->isDiffMER(xP -1, yP+nPSH-1, xP, yP))
    {
      pcCULeft = NULL;
    }
  }
  PartSize partSize = getPartitionSize( uiAbsPartIdx );
  if (!(uiPUIdx == 1 && (partSize == SIZE_Nx2N || partSize == SIZE_nLx2N || partSize == SIZE_nRx2N)))
  {
    if ( pcCULeft && !pcCULeft->isIntra( uiLeftPartIdx )
#if MERL_VSP_C0152
      CHECK_ADD_YET(pcCULeft, uiLeftPartIdx, 1)
#endif
      )
    {
      abCandIsInter[iCount] = true;
      // get Inter Dir
      puhInterDirNeighbours[iCount] = pcCULeft->getInterDir( uiLeftPartIdx );
      // get Mv from Left
      pcCULeft->getMvField( pcCULeft, uiLeftPartIdx, REF_PIC_LIST_0, pcMvFieldNeighbours[iCount<<1] );
      if ( getSlice()->isInterB() )
      {
        pcCULeft->getMvField( pcCULeft, uiLeftPartIdx, REF_PIC_LIST_1, pcMvFieldNeighbours[(iCount<<1)+1] );
      }
#if FIX_CU_BASED_MRG_CAND_LIST_B0136
      else
      {
        pcMvFieldNeighbours[(iCount<<1)+1 ].setMvField( TComMv(0,0), NOT_VALID );
      }
#endif

#if H3D_IVMP
      Bool bRemoveSpa = false; //pruning to inter-view candidates
      Int  iCnloop    = iCount - 1;
      for(; iCnloop >= 0; iCnloop --)
      {
        if(puhInterDirNeighbours[iCount] == puhInterDirNeighbours[iCnloop] && pcMvFieldNeighbours[iCnloop<<1]==pcMvFieldNeighbours[(iCount<<1)] && pcMvFieldNeighbours[(iCnloop<<1)+1]==pcMvFieldNeighbours[(iCount<<1)+1])
        {
          bRemoveSpa                      = true;
          abCandIsInter        [ iCount ] = false;

          //reset to the default value for IC, MC
          puhInterDirNeighbours[iCount]   = 0;
          TComMv  cZeroMv;
          pcMvFieldNeighbours[iCount<<1].setMvField( cZeroMv, NOT_VALID );
          pcMvFieldNeighbours[(iCount<<1)+1].setMvField( cZeroMv, NOT_VALID );
          break;
        }
      }
      if(!bRemoveSpa)
      {
        bLeftAvai = true;
        iPosLeftAbove[0] = iCount;
#endif  // H3D_IVMP 
#if H3D_NBDV
        pcMvFieldNeighbours[iCount<<1    ].getMv().m_bDvMcp = false;
        pcMvFieldNeighbours[(iCount<<1)+1].getMv().m_bDvMcp = false;
#endif
#if MERL_VSP_C0152
        xInheritVspMode( pcCULeft, uiLeftPartIdx, bVspMvZeroDone, iCount, iVSPIndexTrue, pcMvFieldNeighbours, &cDisInfo ) ;
#endif
        if ( mrgCandIdx == iCount )
        {
          return;
        }
        iCount ++;
#if H3D_IVMP
      }
#endif 
    }
  }

  // above
  UInt uiAbovePartIdx = 0;
  TComDataCU* pcCUAbove = 0;
  pcCUAbove = getPUAbove( uiAbovePartIdx, uiPartIdxRT, true, false, true );
    if (pcCUAbove) 
    {
      if (!pcCUAbove->isDiffMER(xP+nPSW-1, yP-1, xP, yP))
      {
        pcCUAbove = NULL;
      }
    }
  if ( pcCUAbove && !pcCUAbove->isIntra( uiAbovePartIdx ) 
#if MERL_VSP_C0152
    CHECK_ADD_YET(pcCUAbove, uiAbovePartIdx, 1)
#endif
    && !(uiPUIdx == 1 && (cCurPS == SIZE_2NxN || cCurPS == SIZE_2NxnU || cCurPS == SIZE_2NxnD))
    && ( !pcCULeft || pcCULeft->isIntra( uiLeftPartIdx ) || !pcCULeft->hasEqualMotion( uiLeftPartIdx, pcCUAbove, uiAbovePartIdx ) ) )
  {
    abCandIsInter[iCount] = true;
    // get Inter Dir
    puhInterDirNeighbours[iCount] = pcCUAbove->getInterDir( uiAbovePartIdx );
    // get Mv from Left
    pcCUAbove->getMvField( pcCUAbove, uiAbovePartIdx, REF_PIC_LIST_0, pcMvFieldNeighbours[iCount<<1] );
    if ( getSlice()->isInterB() )
    {
      pcCUAbove->getMvField( pcCUAbove, uiAbovePartIdx, REF_PIC_LIST_1, pcMvFieldNeighbours[(iCount<<1)+1] );
    }
#if FIX_CU_BASED_MRG_CAND_LIST_B0136
    else
    {
      pcMvFieldNeighbours[(iCount<<1)+1 ].setMvField( TComMv(0,0), NOT_VALID );
    }
#endif
#if H3D_IVMP
    Bool bRemoveSpa = false; //pruning to inter-view candidates
    Int  iCnloop    = bLeftAvai? (iCount-2): (iCount-1);
    for(; iCnloop >= 0; iCnloop --)
    {
      if(puhInterDirNeighbours[iCount] == puhInterDirNeighbours[iCnloop] && pcMvFieldNeighbours[iCnloop<<1]==pcMvFieldNeighbours[(iCount<<1)] && pcMvFieldNeighbours[(iCnloop<<1)+1]==pcMvFieldNeighbours[(iCount<<1)+1])
      {
        bRemoveSpa                      = true;
        abCandIsInter        [ iCount ] = false;

        //reset to the default value for IC, MC
        puhInterDirNeighbours[iCount]   = 0;
        TComMv  cZeroMv;
        pcMvFieldNeighbours[iCount<<1].setMvField( cZeroMv, NOT_VALID );
        pcMvFieldNeighbours[(iCount<<1)+1].setMvField( cZeroMv, NOT_VALID );
        break;
      }
    }
    if(!bRemoveSpa)
    {
      iPosLeftAbove[1] = iCount;
#endif //  H3D_IVMP 
#if H3D_NBDV
    pcMvFieldNeighbours[iCount<<1    ].getMv().m_bDvMcp = false;
    pcMvFieldNeighbours[(iCount<<1)+1].getMv().m_bDvMcp = false;
#endif
#if MERL_VSP_C0152
     xInheritVspMode( pcCUAbove, uiAbovePartIdx, bVspMvZeroDone, iCount, iVSPIndexTrue, pcMvFieldNeighbours, &cDisInfo ) ;
#endif
    if ( mrgCandIdx == iCount )
    {
      return;
    }
    iCount ++;
#if H3D_IVMP
    }
#endif
  }

  // above right
  UInt uiAboveRightPartIdx = 0;
  TComDataCU* pcCUAboveRight = 0;
  pcCUAboveRight = getPUAboveRight( uiAboveRightPartIdx, uiPartIdxRT, true, false, true );
  if (pcCUAboveRight) 
  {
    if (!pcCUAboveRight->isDiffMER(xP+nPSW, yP-1, xP, yP))
    {
      pcCUAboveRight = NULL;
    }
  }
  if ( pcCUAboveRight && !pcCUAboveRight->isIntra( uiAboveRightPartIdx ) 
#if MERL_VSP_C0152
    CHECK_ADD_YET(pcCUAboveRight, uiAboveRightPartIdx, 1)
#endif
    && ( !pcCUAbove || pcCUAbove->isIntra( uiAbovePartIdx ) || !pcCUAbove->hasEqualMotion( uiAbovePartIdx, pcCUAboveRight, uiAboveRightPartIdx ) ) )
  {
    abCandIsInter[iCount] = true;
    // get Inter Dir
    puhInterDirNeighbours[iCount] = pcCUAboveRight->getInterDir( uiAboveRightPartIdx );
    // get Mv from Left
    pcCUAboveRight->getMvField( pcCUAboveRight, uiAboveRightPartIdx, REF_PIC_LIST_0, pcMvFieldNeighbours[iCount<<1] );
    if ( getSlice()->isInterB() )
    {
      pcCUAboveRight->getMvField( pcCUAboveRight, uiAboveRightPartIdx, REF_PIC_LIST_1, pcMvFieldNeighbours[(iCount<<1)+1] );
    }
#if FIX_CU_BASED_MRG_CAND_LIST_B0136
    else
    {
      pcMvFieldNeighbours[(iCount<<1)+1 ].setMvField( TComMv(0,0), NOT_VALID );
    }
#endif
#if H3D_NBDV
    pcMvFieldNeighbours[iCount<<1    ].getMv().m_bDvMcp = false;
    pcMvFieldNeighbours[(iCount<<1)+1].getMv().m_bDvMcp = false;
#endif
#if MERL_VSP_C0152
    xInheritVspMode( pcCUAboveRight, uiAboveRightPartIdx, bVspMvZeroDone, iCount, iVSPIndexTrue, pcMvFieldNeighbours, &cDisInfo ) ;
#endif
    if ( mrgCandIdx == iCount )
    {
      return;
    }
    iCount ++;
  }

#if H3D_IVMP

  if(extraMergeCand)
  {
    if(!bNoPdmMerge && iPdmDir[1] )
    {
      assert(iCount < MRG_MAX_NUM_CANDS_MEM);
      Bool bRemoveSpa = false; //pruning to A1, B1
      abCandIsInter        [ iCount ] = true;
      puhInterDirNeighbours[ iCount ] = iPdmDir[1];
      if( ( iPdmDir[1] & 1 ) == 1 )
      {
        pcMvFieldNeighbours[ iCount<<1    ].setMvField( acPdmMv[ 2 ], aiPdmRefIdx[ 2 ] );
      }
      if( ( iPdmDir[1] & 2 ) == 2 )
      {
        pcMvFieldNeighbours[(iCount<<1)+1 ].setMvField( acPdmMv[ 3 ], aiPdmRefIdx[ 3 ] );
      }
      for(Int i = 0; i < 2; i ++)
      {
        Int iCnloop = iPosLeftAbove[i];
        if(iCnloop == -1) 
          continue;
        if(puhInterDirNeighbours[iCount] == puhInterDirNeighbours[iCnloop] && pcMvFieldNeighbours[iCnloop<<1]==pcMvFieldNeighbours[(iCount<<1)] && pcMvFieldNeighbours[(iCnloop<<1)+1]==pcMvFieldNeighbours[(iCount<<1)+1])
        {
          bRemoveSpa                      = true;
          abCandIsInter        [ iCount ] = false;
          //reset to the default value for IC, MC
          puhInterDirNeighbours[iCount]   = 0;
          TComMv  cZeroMv;
          pcMvFieldNeighbours[iCount<<1].setMvField( cZeroMv, NOT_VALID );
          pcMvFieldNeighbours[(iCount<<1)+1].setMvField( cZeroMv, NOT_VALID );
          break;
        }
      }
      if(!bRemoveSpa)
      {
#if H3D_NBDV
        pcMvFieldNeighbours[iCount<<1    ].getMv().m_bDvMcp = false;
        pcMvFieldNeighbours[(iCount<<1)+1].getMv().m_bDvMcp = false;
#endif
        if ( mrgCandIdx == iCount )
          return;
        iCount ++;
      }
    }    
  }
#endif // H3D_IVMP 


  //left bottom
  UInt uiLeftBottomPartIdx = 0;
  TComDataCU* pcCULeftBottom = 0;
  pcCULeftBottom = this->getPUBelowLeft( uiLeftBottomPartIdx, uiPartIdxLB, true, false );
  if (pcCULeftBottom)
  {
    if (!pcCULeftBottom->isDiffMER(xP-1, yP+nPSH, xP, yP))
    {
      pcCULeftBottom = NULL;
    }
  }
  if ( pcCULeftBottom && !pcCULeftBottom->isIntra( uiLeftBottomPartIdx ) 
#if MERL_VSP_C0152
    CHECK_ADD_YET(pcCULeftBottom, uiLeftBottomPartIdx, 1)
#endif
    && ( !pcCULeft || pcCULeft->isIntra( uiLeftPartIdx ) || !pcCULeft->hasEqualMotion( uiLeftPartIdx, pcCULeftBottom, uiLeftBottomPartIdx ) ) )
  {
    abCandIsInter[iCount] = true;
    // get Inter Dir
    puhInterDirNeighbours[iCount] = pcCULeftBottom->getInterDir( uiLeftBottomPartIdx );
    // get Mv from Left
    pcCULeftBottom->getMvField( pcCULeftBottom, uiLeftBottomPartIdx, REF_PIC_LIST_0, pcMvFieldNeighbours[iCount<<1] );
    if ( getSlice()->isInterB() )
    {
      pcCULeftBottom->getMvField( pcCULeftBottom, uiLeftBottomPartIdx, REF_PIC_LIST_1, pcMvFieldNeighbours[(iCount<<1)+1] );
    }
#if FIX_CU_BASED_MRG_CAND_LIST_B0136
    else
    {
      pcMvFieldNeighbours[(iCount<<1)+1 ].setMvField( TComMv(0,0), NOT_VALID );
    }
#endif
#if H3D_NBDV
    pcMvFieldNeighbours[iCount<<1    ].getMv().m_bDvMcp = false;
    pcMvFieldNeighbours[(iCount<<1)+1].getMv().m_bDvMcp = false;
#endif
#if MERL_VSP_C0152
     xInheritVspMode( pcCULeftBottom, uiLeftBottomPartIdx, bVspMvZeroDone, iCount, iVSPIndexTrue, pcMvFieldNeighbours, &cDisInfo ) ;
#endif
    if ( mrgCandIdx == iCount )
    {
      return;
    }
    iCount ++;
  }
 
  
  // above left
#if H3D_IVMP
  if( iCount < 4 + extraMergeCand )
#else
  if( iCount < 4 )
#endif
  {
    UInt uiAboveLeftPartIdx = 0;
    TComDataCU* pcCUAboveLeft = 0;
    pcCUAboveLeft = getPUAboveLeft( uiAboveLeftPartIdx, uiAbsPartAddr, true, false, true );
    if (pcCUAboveLeft) 
    {
      if (!pcCUAboveLeft->isDiffMER(xP-1, yP-1, xP, yP))
      {
        pcCUAboveLeft = NULL;
      }
    }
    if( pcCUAboveLeft && !pcCUAboveLeft->isIntra( uiAboveLeftPartIdx )
#if MERL_VSP_C0152
     CHECK_ADD_YET(pcCUAboveLeft, uiAboveLeftPartIdx, 1)
#endif
     && ( !pcCULeft || pcCULeft->isIntra( uiLeftPartIdx ) || !pcCULeft->hasEqualMotion( uiLeftPartIdx, pcCUAboveLeft, uiAboveLeftPartIdx ) )
     && ( !pcCUAbove || pcCUAbove->isIntra( uiAbovePartIdx ) || !pcCUAbove->hasEqualMotion( uiAbovePartIdx, pcCUAboveLeft, uiAboveLeftPartIdx ) )
     )
    {
      abCandIsInter[iCount] = true;
      // get Inter Dir
      puhInterDirNeighbours[iCount] = pcCUAboveLeft->getInterDir( uiAboveLeftPartIdx );
      // get Mv from Left
      pcCUAboveLeft->getMvField( pcCUAboveLeft, uiAboveLeftPartIdx, REF_PIC_LIST_0, pcMvFieldNeighbours[iCount<<1] );
      if ( getSlice()->isInterB() )
      {
        pcCUAboveLeft->getMvField( pcCUAboveLeft, uiAboveLeftPartIdx, REF_PIC_LIST_1, pcMvFieldNeighbours[(iCount<<1)+1] );
      }
#if FIX_CU_BASED_MRG_CAND_LIST_B0136
      else
      {
        pcMvFieldNeighbours[(iCount<<1)+1 ].setMvField( TComMv(0,0), NOT_VALID );
      }
#endif
#if H3D_NBDV
      pcMvFieldNeighbours[iCount<<1    ].getMv().m_bDvMcp = false;
      pcMvFieldNeighbours[(iCount<<1)+1].getMv().m_bDvMcp = false;
#endif
#if MERL_VSP_C0152
     xInheritVspMode( pcCUAboveLeft, uiAboveLeftPartIdx, bVspMvZeroDone, iCount, iVSPIndexTrue, pcMvFieldNeighbours, &cDisInfo ) ;
#endif
      if ( mrgCandIdx == iCount )
      {
        return;
      }
      iCount ++;
    }
  }

#if MERL_VSP_COMPENSATION_C0152
  //===== vsp 5 =====
  if( iCount < 4 + extraMergeCand )
    if ( !xAddVspMergeCand(5, 1, bVspMvZeroDone, uiDepth, abCandIsInter, iCount, puhInterDirNeighbours, pcMvFieldNeighbours, iVSPIndexTrue, mrgCandIdx, &cDisInfo) )
      return;
#endif

  if ( getSlice()->getPPS()->getEnableTMVPFlag() 
#if H3D_IVMP
       && iCount < (MRG_MAX_NUM_CANDS_SIGNALED + extraMergeCand)
#endif
  )
  {
    // col [2]
#if !QC_TMVP_IDX_MOD_B0046
    Int iRefIdxSkip[2] = {-1, -1};
    for (Int i=0; i<2; i++)
    {
      RefPicList  eRefPicList = ( i==1 ? REF_PIC_LIST_1 : REF_PIC_LIST_0 );
      Int iRefIdxTmp;
      if ( uiPUIdx != 0 )
      {
        iRefIdxTmp = 0;
      }
      else
      {    
        iRefIdxTmp = (pcCULeft != NULL) ? pcCULeft->getCUMvField(eRefPicList)->getRefIdx(uiLeftPartIdx) : -1;
      }
      iRefIdxSkip[i] = (iRefIdxTmp != -1) ? iRefIdxTmp : 0;
    }
#endif
    //>> MTK colocated-RightBottom
    UInt uiPartIdxRB;
    Int uiLCUIdx = getAddr();
    PartSize eCUMode = getPartitionSize( 0 );

#if MERL_VSP_C0152 // Potential bug, not related to BW_VSP
    if (eCUMode==SIZE_NxN)
    {
      printf("Size NxN ???");
      assert(0);
    }
#endif

    deriveRightBottomIdx( eCUMode, uiPUIdx, uiPartIdxRB );  

    UInt uiAbsPartIdxTmp = g_auiZscanToRaster[uiPartIdxRB];
    UInt uiNumPartInCUWidth = m_pcPic->getNumPartInWidth();

    TComMv cColMv;
#if QC_TMVP_IDX_MOD_B0046
    Int iRefIdx = 0;
#else
    Int iRefIdx;
#endif
    if      ( ( m_pcPic->getCU(m_uiCUAddr)->getCUPelX() + g_auiRasterToPelX[uiAbsPartIdxTmp] + m_pcPic->getMinCUWidth() ) >= m_pcSlice->getSPS()->getPicWidthInLumaSamples() )  // image boundary check
    {
      uiLCUIdx = -1;
    }
    else if ( ( m_pcPic->getCU(m_uiCUAddr)->getCUPelY() + g_auiRasterToPelY[uiAbsPartIdxTmp] + m_pcPic->getMinCUHeight() ) >= m_pcSlice->getSPS()->getPicHeightInLumaSamples() )
    {
      uiLCUIdx = -1;
    }
    else
    {
      if ( ( uiAbsPartIdxTmp % uiNumPartInCUWidth < uiNumPartInCUWidth - 1 ) &&           // is not at the last column of LCU 
        ( uiAbsPartIdxTmp / uiNumPartInCUWidth < m_pcPic->getNumPartInHeight() - 1 ) ) // is not at the last row    of LCU
      {
        uiAbsPartAddr = g_auiRasterToZscan[ uiAbsPartIdxTmp + uiNumPartInCUWidth + 1 ];
        uiLCUIdx = getAddr();
      }
      else if ( uiAbsPartIdxTmp % uiNumPartInCUWidth < uiNumPartInCUWidth - 1 )           // is not at the last column of LCU But is last row of LCU
      {
        uiAbsPartAddr = g_auiRasterToZscan[ (uiAbsPartIdxTmp + uiNumPartInCUWidth + 1) % m_pcPic->getNumPartInCU() ];
        uiLCUIdx = -1 ; 
      }
      else if ( uiAbsPartIdxTmp / uiNumPartInCUWidth < m_pcPic->getNumPartInHeight() - 1 ) // is not at the last row of LCU But is last column of LCU
      {
        uiAbsPartAddr = g_auiRasterToZscan[ uiAbsPartIdxTmp + 1 ];
        uiLCUIdx = getAddr() + 1;
      }
      else //is the right bottom corner of LCU                       
      {
        uiAbsPartAddr = 0;
        uiLCUIdx = -1 ; 
      }
    }
#if !QC_TMVP_IDX_MOD_B0046
    iRefIdx = iRefIdxSkip[0];
#else
    iRefIdx = 0;
#endif
    Bool bExistMV = false;
    UInt uiPartIdxCenter;
    UInt uiCurLCUIdx = getAddr();
    xDeriveCenterIdx( eCUMode, uiPUIdx, uiPartIdxCenter );
    bExistMV = uiLCUIdx >= 0 && xGetColMVP( REF_PIC_LIST_0, uiLCUIdx, uiAbsPartAddr, cColMv, iRefIdx 
#if QC_TMVP_MRG_REFIDX_C0047
    , 1
#endif
    );
    if( bExistMV == false )
    {
      bExistMV = xGetColMVP( REF_PIC_LIST_0, uiCurLCUIdx, uiPartIdxCenter,  cColMv, iRefIdx 
#if QC_TMVP_MRG_REFIDX_C0047
    , 1
#endif
    );
    }
    if( bExistMV )
    {
      UInt uiArrayAddr = iCount;
      abCandIsInter[uiArrayAddr] = true;
      pcMvFieldNeighbours[uiArrayAddr << 1].setMvField( cColMv, iRefIdx );

      if ( getSlice()->isInterB() )
      {       
#if !QC_TMVP_IDX_MOD_B0046
        iRefIdx = iRefIdxSkip[1];
#else
        iRefIdx = 0;
#endif
        bExistMV = uiLCUIdx >= 0 && xGetColMVP( REF_PIC_LIST_1, uiLCUIdx, uiAbsPartAddr, cColMv, iRefIdx
#if QC_TMVP_MRG_REFIDX_C0047
        , 1
#endif
        );
        if( bExistMV == false )
        {
          bExistMV = xGetColMVP( REF_PIC_LIST_1, uiCurLCUIdx, uiPartIdxCenter,  cColMv, iRefIdx 
#if QC_TMVP_MRG_REFIDX_C0047
        , 1
#endif  
        );
        }
        if( bExistMV )
        {
          pcMvFieldNeighbours[ ( uiArrayAddr << 1 ) + 1 ].setMvField( cColMv, iRefIdx );
          puhInterDirNeighbours[uiArrayAddr] = 3;
        }
        else
        {
          puhInterDirNeighbours[uiArrayAddr] = 1;
#if FIX_CU_BASED_MRG_CAND_LIST_B0136
          pcMvFieldNeighbours[ ( uiArrayAddr << 1 ) + 1 ].setMvField( TComMv(0,0), NOT_VALID );
#endif
        }
      }
      else
      {
        puhInterDirNeighbours[uiArrayAddr] = 1;
#if FIX_CU_BASED_MRG_CAND_LIST_B0136
        pcMvFieldNeighbours[ ( uiArrayAddr << 1 ) + 1 ].setMvField( TComMv(0,0), NOT_VALID );
#endif
      }
#if H3D_NBDV
      pcMvFieldNeighbours[uiArrayAddr<<1    ].getMv().m_bDvMcp = false;
      pcMvFieldNeighbours[(uiArrayAddr<<1)+1].getMv().m_bDvMcp = false;
#endif
      if ( mrgCandIdx == iCount )
      {
        return;
      }
      iCount++;
    }
    uiIdx++;

  }

  UInt uiArrayAddr = iCount;
  UInt uiCutoff = uiArrayAddr;
    
  if ( getSlice()->isInterB() )
  {
#if H3D_IVMP
    Int iCombinedCount = 0;
    Int iMaxCombCount  = ( extraMergeCand ?  6 :  5 );
    Int iMaxIdx        = ( extraMergeCand ? 20 : 12 );
    UInt uiPriorityList0[20] = {0 , 1, 0, 2, 1, 2, 0, 3, 1, 3, 2, 3,    0, 4, 1, 4, 2, 4, 3, 4 };
    UInt uiPriorityList1[20] = {1 , 0, 2, 0, 2, 1, 3, 0, 3, 1, 3, 2,    4, 0, 4, 1, 4, 2, 4, 3 };

    for (Int idx=0; idx<uiCutoff*(uiCutoff-1) && uiArrayAddr!=MRG_MAX_NUM_CANDS_MEM && iCombinedCount<iMaxCombCount && idx < iMaxIdx; idx++)
#else
    Int iCombinedCount = 0;
    UInt uiPriorityList0[12] = {0 , 1, 0, 2, 1, 2, 0, 3, 1, 3, 2, 3};
    UInt uiPriorityList1[12] = {1 , 0, 2, 0, 2, 1, 3, 0, 3, 1, 3, 2};

    for (Int idx=0; idx<uiCutoff*(uiCutoff-1) && uiArrayAddr!=MRG_MAX_NUM_CANDS && iCombinedCount<5 && idx < 12; idx++)
#endif
    {
      Int i = uiPriorityList0[idx]; Int j = uiPriorityList1[idx];
#if MERL_VSP_C0152
      Bool bValid = true;
      if (pcMvFieldNeighbours[i<<1].getRefIdx() < 0 || pcMvFieldNeighbours[(j<<1)+1].getRefIdx() < 0) // NOT_VALID
        bValid = false;
#endif
      if (abCandIsInter[i] && abCandIsInter[j]&& (puhInterDirNeighbours[i]&0x1)&&(puhInterDirNeighbours[j]&0x2)
#if MERL_VSP_C0152
       && bValid
#endif
         )
      {
        abCandIsInter[uiArrayAddr] = true;
        puhInterDirNeighbours[uiArrayAddr] = 3;

        // get Mv from cand[i] and cand[j]
        pcMvFieldNeighbours[uiArrayAddr << 1].setMvField(pcMvFieldNeighbours[i<<1].getMv(), pcMvFieldNeighbours[i<<1].getRefIdx());
        pcMvFieldNeighbours[( uiArrayAddr << 1 ) + 1].setMvField(pcMvFieldNeighbours[(j<<1)+1].getMv(), pcMvFieldNeighbours[(j<<1)+1].getRefIdx());

        Int iRefPOCL0    = m_pcSlice->getRefPOC   ( REF_PIC_LIST_0, pcMvFieldNeighbours[(uiArrayAddr<<1)].getRefIdx() );
        Int iRefViewIdL0 = m_pcSlice->getRefViewId( REF_PIC_LIST_0, pcMvFieldNeighbours[(uiArrayAddr<<1)].getRefIdx() );
        Int iRefPOCL1    = m_pcSlice->getRefPOC   ( REF_PIC_LIST_1, pcMvFieldNeighbours[(uiArrayAddr<<1)+1].getRefIdx() );
        Int iRefViewIdL1 = m_pcSlice->getRefViewId( REF_PIC_LIST_1, pcMvFieldNeighbours[(uiArrayAddr<<1)+1].getRefIdx() );
        if(iRefPOCL0 == iRefPOCL1 && iRefViewIdL0 == iRefViewIdL1 && pcMvFieldNeighbours[(uiArrayAddr<<1)].getMv() == pcMvFieldNeighbours[(uiArrayAddr<<1)+1].getMv())
        {
          abCandIsInter[uiArrayAddr] = false;
        }
        else
        {
          uiArrayAddr++;
          iCombinedCount++;
        }
      }
    }
  }

  Int iNumRefIdx = (getSlice()->isInterB()) ? min(m_pcSlice->getNumRefIdx(REF_PIC_LIST_0), m_pcSlice->getNumRefIdx(REF_PIC_LIST_1)) : m_pcSlice->getNumRefIdx(REF_PIC_LIST_0);
#if H3D_IVMP
  for (int r=0; r<iNumRefIdx && uiArrayAddr!=MRG_MAX_NUM_CANDS_MEM; r++)
#else
  for (int r=0; r<iNumRefIdx && uiArrayAddr!=MRG_MAX_NUM_CANDS; r++)
#endif
  {
    abCandIsInter[uiArrayAddr] = true;
    puhInterDirNeighbours[uiArrayAddr] = 1;
    pcMvFieldNeighbours[uiArrayAddr << 1].setMvField( TComMv(0, 0), r);

    if ( getSlice()->isInterB() )
    {
      puhInterDirNeighbours[uiArrayAddr] = 3;
      pcMvFieldNeighbours[(uiArrayAddr << 1) + 1].setMvField(TComMv(0, 0), r);
    }
#if FIX_CU_BASED_MRG_CAND_LIST_B0136
    else
    {
      pcMvFieldNeighbours[ ( uiArrayAddr << 1 ) + 1 ].setMvField( TComMv(0,0), NOT_VALID );
    }
#endif
    uiArrayAddr++;
  }
#if H3D_IVMP
  if (uiArrayAddr > MRG_MAX_NUM_CANDS_SIGNALED + extraMergeCand )
  {
    uiArrayAddr = MRG_MAX_NUM_CANDS_SIGNALED + extraMergeCand;
  }
#else
  if (uiArrayAddr > MRG_MAX_NUM_CANDS_SIGNALED)
  {
    uiArrayAddr = MRG_MAX_NUM_CANDS_SIGNALED;
  }
#endif
  numValidMergeCand = uiArrayAddr;
}

/** Check the duplicated candidate in the list
 * \param pcMvFieldNeighbours
 * \param puhInterDirNeighbours 
 * \param pbCandIsInter
 * \param ruiArrayAddr
 * \returns Void
 */
Void TComDataCU::xCheckDuplicateCand(TComMvField* pcMvFieldNeighbours, UChar* puhInterDirNeighbours, bool* pbCandIsInter, UInt& ruiArrayAddr)
{
  if (getSlice()->isInterB())
  {
    UInt uiMvFieldNeighIdxCurr = ruiArrayAddr << 1;
    Int iRefIdxL0 = pcMvFieldNeighbours[ uiMvFieldNeighIdxCurr ].getRefIdx();
    Int iRefIdxL1 = pcMvFieldNeighbours[ uiMvFieldNeighIdxCurr + 1 ].getRefIdx();
    TComMv MvL0 = pcMvFieldNeighbours[ uiMvFieldNeighIdxCurr ].getMv();
    TComMv MvL1 = pcMvFieldNeighbours[ uiMvFieldNeighIdxCurr + 1 ].getMv();

    for (int k=0; k<ruiArrayAddr; k++)
    {
      UInt uiMvFieldNeighIdxComp = k << 1;
      if (iRefIdxL0 == pcMvFieldNeighbours[ uiMvFieldNeighIdxComp ].getRefIdx() && 
          iRefIdxL1 == pcMvFieldNeighbours[ uiMvFieldNeighIdxComp + 1 ].getRefIdx() &&
          MvL0 == pcMvFieldNeighbours[ uiMvFieldNeighIdxComp ].getMv() && 
          MvL1 == pcMvFieldNeighbours[ uiMvFieldNeighIdxComp + 1 ].getMv() &&
          puhInterDirNeighbours[ ruiArrayAddr ] == puhInterDirNeighbours[ k ] )
      {
        pbCandIsInter[ruiArrayAddr] = false;
        break;
      }
    }
  }
  else
  {
    UInt uiMvFieldNeighIdxCurr = ruiArrayAddr << 1;
    Int iRefIdxL0 = pcMvFieldNeighbours[ uiMvFieldNeighIdxCurr ].getRefIdx();
    TComMv MvL0 = pcMvFieldNeighbours[ uiMvFieldNeighIdxCurr ].getMv();

    for (int k=0; k<ruiArrayAddr; k++)
    {
      UInt uiMvFieldNeighIdxComp = k << 1;
      if (iRefIdxL0 == pcMvFieldNeighbours[ uiMvFieldNeighIdxComp ].getRefIdx() && 
          MvL0 == pcMvFieldNeighbours[ uiMvFieldNeighIdxComp ].getMv() && 
          puhInterDirNeighbours[ ruiArrayAddr ] == puhInterDirNeighbours[ k ] )
      {
        pbCandIsInter[ruiArrayAddr] = false;
        break;
      }
    }
  }

  if (pbCandIsInter[ruiArrayAddr])
  {
    ++ruiArrayAddr;
  }
}

Void TComDataCU::xCheckCornerCand( TComDataCU* pcCorner, UInt uiCornerPUIdx, UInt uiIter, Bool& rbValidCand )
{
  if( uiIter == 0 )
  {
    if( pcCorner && !pcCorner->isIntra( uiCornerPUIdx ) 
#if MERL_VSP_C0152
     && !pcCorner->getVSPIndex( uiCornerPUIdx )
#endif
    )
    {
      rbValidCand = true;
      if( getSlice()->isInterB() )
      {
        if ( pcCorner->getInterDir( uiCornerPUIdx ) == 1 )
        {
          if( pcCorner->getCUMvField(REF_PIC_LIST_0)->getRefIdx( uiCornerPUIdx ) != 0 )
          {
            rbValidCand = false;
          }
        }
        else if ( pcCorner->getInterDir( uiCornerPUIdx ) == 2 )
        {
          if( pcCorner->getCUMvField(REF_PIC_LIST_1)->getRefIdx( uiCornerPUIdx ) != 0 )
          {
            rbValidCand = false;
          }
        }
        else
        {
          if( pcCorner->getCUMvField(REF_PIC_LIST_0)->getRefIdx( uiCornerPUIdx ) != 0 || pcCorner->getCUMvField(REF_PIC_LIST_1)->getRefIdx( uiCornerPUIdx ) != 0 )
          {
            rbValidCand = false;
          }
        }
      }
      else if( pcCorner->getCUMvField(REF_PIC_LIST_0)->getRefIdx( uiCornerPUIdx ) != 0 )
      {
        rbValidCand = false;
      }
    }
  }
  else
  {
    if( pcCorner && !pcCorner->isIntra( uiCornerPUIdx ) 
#if MERL_VSP_C0152
     && !pcCorner->getVSPIndex( uiCornerPUIdx )
#endif
    )
    {
      rbValidCand = true;
      if( getSlice()->isInterB() )
      {
        if ( pcCorner->getInterDir( uiCornerPUIdx ) == 1 )
        {
          if( pcCorner->getCUMvField(REF_PIC_LIST_0)->getRefIdx( uiCornerPUIdx ) < 0 )
          {
            rbValidCand = false;
          }
        }
        else if ( pcCorner->getInterDir( uiCornerPUIdx ) == 2 )
        {
          if( pcCorner->getCUMvField(REF_PIC_LIST_1)->getRefIdx( uiCornerPUIdx ) < 0 )
          {
            rbValidCand = false;
          }
        }
        else
        {
          if( pcCorner->getCUMvField(REF_PIC_LIST_0)->getRefIdx( uiCornerPUIdx ) < 0 || pcCorner->getCUMvField(REF_PIC_LIST_1)->getRefIdx( uiCornerPUIdx ) < 0 )
          {
            rbValidCand = false;
          }
        }
      }
      else if( pcCorner->getCUMvField(REF_PIC_LIST_0)->getRefIdx( uiCornerPUIdx ) < 0 )
      {
        rbValidCand = false;
      }
    }
  }
}
/** Check whether the current PU and a spatial neighboring PU are in a same ME region.
 * \param xN, xN   location of the upper-left corner pixel of a neighboring PU
 * \param xP, yP   location of the upper-left corner pixel of the current PU
 * \returns Bool
 */
Bool TComDataCU::isDiffMER(Int xN, Int yN, Int xP, Int yP)
{

  UInt plevel = this->getSlice()->getPPS()->getLog2ParallelMergeLevelMinus2() + 2;
  if ((xN>>plevel)!= (xP>>plevel))
  {
    return true;
  }
  if ((yN>>plevel)!= (yP>>plevel))
  {
    return true;
  }
  return false;
}
/** calculate the location of upper-left corner pixel and size of the current PU.
 * \param partIdx  PU index within a CU
 * \param xP, yP   location of the upper-left corner pixel of the current PU
 * \param PSW, nPSH    size of the curren PU
 * \returns Void
 */
Void TComDataCU::getPartPosition( UInt partIdx, Int& xP, Int& yP, Int& nPSW, Int& nPSH)
{
  UInt col = m_uiCUPelX;
  UInt row = m_uiCUPelY;

  switch ( m_pePartSize[0] )
  {
  case SIZE_2NxN:
    nPSW = getWidth(0);      
    nPSH = getHeight(0) >> 1; 
    xP   = col;
    yP   = (partIdx ==0)? row: row + nPSH;
    break;
  case SIZE_Nx2N:
    nPSW = getWidth(0) >> 1; 
    nPSH = getHeight(0);      
    xP   = (partIdx ==0)? col: col + nPSW;
    yP   = row;
    break;
  case SIZE_NxN:
    nPSW = getWidth(0) >> 1; 
    nPSH = getHeight(0) >> 1; 
    xP   = col + (partIdx&0x1)*nPSW;
    yP   = row + (partIdx>>1)*nPSH;
    break;
  case SIZE_2NxnU:
    nPSW = getWidth(0);
    nPSH = ( partIdx == 0 ) ?  getHeight(0) >> 2 : ( getHeight(0) >> 2 ) + ( getHeight(0) >> 1 );
    xP   = col;
    yP   = (partIdx ==0)? row: row + getHeight(0) - nPSH;

    break;
  case SIZE_2NxnD:
    nPSW = getWidth(0);
    nPSH = ( partIdx == 0 ) ?  ( getHeight(0) >> 2 ) + ( getHeight(0) >> 1 ) : getHeight(0) >> 2;
    xP   = col;
    yP   = (partIdx ==0)? row: row + getHeight(0) - nPSH;
    break;
  case SIZE_nLx2N:
    nPSW = ( partIdx == 0 ) ? getWidth(0) >> 2 : ( getWidth(0) >> 2 ) + ( getWidth(0) >> 1 );
    nPSH = getHeight(0);
    xP   = (partIdx ==0)? col: col + getWidth(0) - nPSW;
    yP   = row;
    break;
  case SIZE_nRx2N:
    nPSW = ( partIdx == 0 ) ? ( getWidth(0) >> 2 ) + ( getWidth(0) >> 1 ) : getWidth(0) >> 2;
    nPSH = getHeight(0);
    xP   = (partIdx ==0)? col: col + getWidth(0) - nPSW;
    yP   = row;
    break;
  default:
    assert ( m_pePartSize[0] == SIZE_2Nx2N );
    nPSW = getWidth(0);      
    nPSH = getHeight(0);      
    xP   = col ;
    yP   = row ;

    break;
  }
}

AMVP_MODE TComDataCU::getAMVPMode(UInt uiIdx)
{
  return m_pcSlice->getSPS()->getAMVPMode(m_puhDepth[uiIdx]);
}
#if H3D_NBDV
/** construct a list of disparity motion vectors from the neighbouring PUs **/
Void TComDataCU::getDisMvpCand ( UInt uiPartIdx, UInt uiPartAddr,DisInfo* pDInfo )
{
  PartSize eCUMode = getPartitionSize( uiPartAddr );
  TComDataCU* pcTmpCU = NULL;
  pDInfo->iN = 0;

  RefPicList eRefPicList = REF_PIC_LIST_0 ;
  //-- Get Spatial MV
  UInt uiPartIdxLT, uiPartIdxRT, uiPartIdxLB;
  UInt uiNumPartInCUWidth = m_pcPic->getNumPartInWidth();

  deriveLeftRightTopIdxGeneral( eCUMode, uiPartAddr, uiPartIdx, uiPartIdxLT, uiPartIdxRT );
  deriveLeftBottomIdxGeneral( eCUMode, uiPartAddr, uiPartIdx, uiPartIdxLB );

  UInt uiIdx = 0;
    pcTmpCU = getPULeft(uiIdx, uiPartIdxLB, true, false);
  if(pcTmpCU != NULL && !pcTmpCU->isIntra( uiIdx ) )
#if MERL_VSP_C0152
  if(! pcTmpCU->getVSPIndex(uiIdx))
#endif
  {
    for(Int iList = 0; iList < (getSlice()->isInterB() ? 2: 1); iList ++)
    {
      eRefPicList = RefPicList(iList);
      Int refId = pcTmpCU->getCUMvField(eRefPicList)->getRefIdx(uiIdx) ;
      if( refId >= 0)
      {
        Int refViewIdx  = pcTmpCU->getSlice()->getRefViewId( eRefPicList, refId);
        if (refViewIdx!= m_pcSlice->getViewId())
        {
           TComMv cMvPred = pcTmpCU->getCUMvField(eRefPicList)->getMv(uiIdx);
           clipMv(cMvPred);
           pDInfo->m_acMvCand[ pDInfo->iN] = cMvPred; 
           pDInfo->m_aVIdxCan[ pDInfo->iN++] = refViewIdx;
           return;
        }
      }
    }
  }
      pcTmpCU = getPUAbove(uiIdx, uiPartIdxRT, true, false, true);

  if(pcTmpCU != NULL && !pcTmpCU->isIntra( uiIdx ))
#if MERL_VSP_C0152
  if(! pcTmpCU->getVSPIndex(uiIdx))
#endif
  {
    for(Int iList = 0; iList < (getSlice()->isInterB() ? 2: 1); iList ++)
    {
      eRefPicList = RefPicList(iList);
      Int refId = pcTmpCU->getCUMvField(eRefPicList)->getRefIdx(uiIdx) ;
      if( refId >= 0)
      {
        Int refViewIdx  = pcTmpCU->getSlice()->getRefViewId( eRefPicList, refId);
        if (refViewIdx!= m_pcSlice->getViewId())
        {
           TComMv cMvPred = pcTmpCU->getCUMvField(eRefPicList)->getMv(uiIdx);
           clipMv(cMvPred);
           pDInfo->m_acMvCand[ pDInfo->iN] = cMvPred; 
           pDInfo->m_aVIdxCan[ pDInfo->iN++] = refViewIdx;
           return;
        }
      }
    }
  }

      pcTmpCU = getPUAboveRight(uiIdx, uiPartIdxRT, true, false, true);
  if(pcTmpCU != NULL && !pcTmpCU->isIntra( uiIdx ) )
#if MERL_VSP_C0152
  if(! pcTmpCU->getVSPIndex(uiIdx))
#endif
  {
    for(Int iList = 0; iList < (getSlice()->isInterB() ? 2: 1); iList ++)
    {
      eRefPicList = RefPicList(iList);
      Int refId = pcTmpCU->getCUMvField(eRefPicList)->getRefIdx(uiIdx) ;
      if( refId >= 0)
      {
        Int refViewIdx  = pcTmpCU->getSlice()->getRefViewId( eRefPicList, refId);
        if (refViewIdx!= m_pcSlice->getViewId())
        {
           TComMv cMvPred = pcTmpCU->getCUMvField(eRefPicList)->getMv(uiIdx);
           clipMv(cMvPred);
           pDInfo->m_acMvCand[ pDInfo->iN] = cMvPred; 
           pDInfo->m_aVIdxCan[ pDInfo->iN++] = refViewIdx;
           return;
        }
      }
    }
  }
      pcTmpCU = getPUBelowLeft(uiIdx, uiPartIdxLB, true, false);
  if(pcTmpCU != NULL && !pcTmpCU->isIntra( uiIdx ))
#if MERL_VSP_C0152
  if(! pcTmpCU->getVSPIndex(uiIdx))
#endif
  {
    for(Int iList = 0; iList < (getSlice()->isInterB() ? 2: 1); iList ++)
    {
      eRefPicList = RefPicList(iList);
      Int refId = pcTmpCU->getCUMvField(eRefPicList)->getRefIdx(uiIdx) ;
      if( refId >= 0)
      {
        Int refViewIdx  = pcTmpCU->getSlice()->getRefViewId( eRefPicList, refId);
        if (refViewIdx!= m_pcSlice->getViewId())
        {
           TComMv cMvPred = pcTmpCU->getCUMvField(eRefPicList)->getMv(uiIdx);
           clipMv(cMvPred);
           pDInfo->m_acMvCand[ pDInfo->iN] = cMvPred; 
           pDInfo->m_aVIdxCan[ pDInfo->iN++] = refViewIdx;
           return;
        }
      }
    }
  }

  // Above predictor search
      pcTmpCU = getPUAboveLeft(uiIdx, (m_uiAbsIdxInLCU + uiPartAddr), true, false, true);
    assert(uiPartIdxLT == (m_uiAbsIdxInLCU + uiPartAddr));
  if(pcTmpCU != NULL && !pcTmpCU->isIntra( uiIdx ))
#if MERL_VSP_C0152
  if(! pcTmpCU->getVSPIndex(uiIdx))
#endif
  {
    for(Int iList = 0; iList < (getSlice()->isInterB() ? 2: 1); iList ++)
    {
      eRefPicList = RefPicList(iList);
      Int refId = pcTmpCU->getCUMvField(eRefPicList)->getRefIdx(uiIdx) ;
      if( refId >= 0)
      {
        Int refViewIdx  = pcTmpCU->getSlice()->getRefViewId( eRefPicList, refId);
        if (refViewIdx!= m_pcSlice->getViewId())
        {
           TComMv cMvPred = pcTmpCU->getCUMvField(eRefPicList)->getMv(uiIdx);
           clipMv(cMvPred);
           pDInfo->m_acMvCand[ pDInfo->iN] = cMvPred; 
           pDInfo->m_aVIdxCan[ pDInfo->iN++] = refViewIdx;
           return;
        }
      }
    }
  }
  //Get temporal MV
  TComMv cColMv;
  Int iTargetViewIdx = 0;
  Int iTStartViewIdx = 0;
  UInt uiPartIdxRB, uiBRIdx;
  Int uiViewIdxCurr= getSlice()->getViewId();

  UInt uiAbsPartAddr;
  int uiLCUIdx = getAddr();
  Int uiLCUnew = uiLCUIdx;
  eCUMode = getPartitionSize( 0 );
  deriveRightBottomIdx( eCUMode, uiPartIdx, uiPartIdxRB );  
  uiBRIdx = uiPartIdxLT;
  UInt uiAbsPartIdxTmp = g_auiZscanToRaster[uiPartIdxRB];
  if ( (( m_pcPic->getCU(m_uiCUAddr)->getCUPelX() + g_auiRasterToPelX[uiAbsPartIdxTmp] + m_pcPic->getMinCUWidth() ) < m_pcSlice->getSPS()->getPicWidthInLumaSamples() ) &&(( m_pcPic->getCU(m_uiCUAddr)->getCUPelY() + g_auiRasterToPelY[uiAbsPartIdxTmp] + m_pcPic->getMinCUHeight() ) < m_pcSlice->getSPS()->getPicHeightInLumaSamples() ))  // image boundary check
  {
    if ( ( uiAbsPartIdxTmp % uiNumPartInCUWidth < uiNumPartInCUWidth - 1 ) &&           // is not at the last column of LCU 
      ( uiAbsPartIdxTmp / uiNumPartInCUWidth < m_pcPic->getNumPartInHeight() - 1 ) ) // is not at the last row    of LCU
    {
      uiBRIdx = g_auiRasterToZscan[ uiAbsPartIdxTmp + uiNumPartInCUWidth + 1 ];
    }
    else if ( uiAbsPartIdxTmp % uiNumPartInCUWidth < uiNumPartInCUWidth - 1 )           // is not at the last column of LCU But is last row of LCU
    {
      uiBRIdx = g_auiRasterToZscan[ (uiAbsPartIdxTmp + uiNumPartInCUWidth + 1) % m_pcPic->getNumPartInCU() ];
      uiLCUnew = uiLCUIdx + m_pcPic->getFrameWidthInCU();
    }
    else if ( uiAbsPartIdxTmp / uiNumPartInCUWidth < m_pcPic->getNumPartInHeight() - 1 ) // is not at the last row of LCU But is last column of LCU
    {
      uiBRIdx = g_auiRasterToZscan[ uiAbsPartIdxTmp + 1 ];
      uiLCUnew = uiLCUIdx + 1;
    }
    else //is the right bottom corner of LCU                       
    {
      uiBRIdx = 0;
      uiLCUnew = uiLCUIdx + m_pcPic->getFrameWidthInCU() + 1;
    }
  }

  Int MaxRef = getSlice()->getNumRefIdx(RefPicList(0));
  UInt uiNumPartInCUHeight = m_pcPic->getNumPartInHeight();
  UInt uiPUVert = (UInt)((Int)g_auiZscanToRaster[uiPartIdxLB]/(Int)(uiNumPartInCUWidth)-(Int)g_auiZscanToRaster[uiPartIdxLT]/(Int)(uiNumPartInCUWidth));
  UInt uiPUHorStart  = g_auiZscanToRaster[uiPartIdxLT];
  UInt uiPUHorEnd    = g_auiZscanToRaster[uiPartIdxRT];
  for( Int i= 0; i< getSlice()->getNumRefIdx(RefPicList(0)); i ++)
  {
    getSlice()->getRefPic( RefPicList(0), i)->setCandPicCheckedFlag(0);
  }
  for( Int i= 0; i< getSlice()->getNumRefIdx(RefPicList(1)); i ++)
  {
    getSlice()->getRefPic( RefPicList(1), i)->setCandPicCheckedFlag(0);
  }
  {
    //check the col-located picture
    eRefPicList = RefPicList(getSlice()->isInterB() ? getSlice()->getColDir() : 0);
#if COLLOCATED_REF_IDX
    Int lpRef = getSlice()->getColRefIdx();
#else
    Int lpRef = 0;
#endif
    if( m_pcSlice->getViewId() == getSlice()->getRefPic( eRefPicList, lpRef)->getViewId() ) 
    {
      if (uiViewIdxCurr > 1)  
      {
        if( (uiLCUnew >= 0 && xGetColDisMV( eRefPicList, lpRef, uiLCUnew, uiBRIdx, cColMv, iTargetViewIdx, iTStartViewIdx)) )
        {
          clipMv(cColMv);
          pDInfo->m_acMvCand[pDInfo->iN] = cColMv;
          pDInfo->m_aVIdxCan[pDInfo->iN++] = iTargetViewIdx;
          return ;
        }
      }
      for(UInt uiLoopVert = 0; uiLoopVert <= uiPUVert; uiLoopVert+=4)
        for( uiIdx = uiPUHorStart; uiIdx <= uiPUHorEnd; uiIdx +=4 )
        {
          uiAbsPartAddr = g_auiRasterToZscan[uiIdx+uiNumPartInCUWidth*uiLoopVert];
          if ( uiLCUIdx >= 0 && xGetColDisMV( eRefPicList, lpRef, uiLCUIdx, uiAbsPartAddr, cColMv, iTargetViewIdx, iTStartViewIdx ) ) 
          {
            clipMv(cColMv);
            pDInfo->m_acMvCand[pDInfo->iN] = cColMv;
            pDInfo->m_aVIdxCan[pDInfo->iN++] = iTargetViewIdx;
            return ;
          }
        }
        for(UInt uiIdx_y = 0; uiIdx_y <uiNumPartInCUHeight; uiIdx_y +=4 )
          for(UInt uiIdx_X = 0; uiIdx_X <uiNumPartInCUWidth; uiIdx_X +=4 )
          {
            uiIdx = uiIdx_X + uiIdx_y * uiNumPartInCUWidth;
            uiAbsPartAddr = g_auiRasterToZscan[uiIdx];
            if ( uiLCUIdx >= 0 && xGetColDisMV( eRefPicList,  lpRef, uiLCUIdx, uiAbsPartAddr, cColMv, iTargetViewIdx, iTStartViewIdx ) ) 
            {
              clipMv(cColMv);
              pDInfo->m_acMvCand[pDInfo->iN] = cColMv;
              pDInfo->m_aVIdxCan[pDInfo->iN++] = iTargetViewIdx;
              return ;
            }
          }
          if (uiViewIdxCurr == 1)  
          {
            if( (uiLCUnew >= 0 && xGetColDisMV( eRefPicList, lpRef, uiLCUnew, uiBRIdx, cColMv, iTargetViewIdx, iTStartViewIdx)) ) 
            {
              clipMv(cColMv);
              pDInfo->m_acMvCand[pDInfo->iN] = cColMv;
              pDInfo->m_aVIdxCan[pDInfo->iN++] = iTargetViewIdx;
              return ;
            }
          }
    }
    getSlice()->getRefPic( eRefPicList, lpRef)->setCandPicCheckedFlag(1);
  }
  {
    //check the remaining reference pictures in list0 and list1
    if(getSlice()->isInterB())
    {
      if(getSlice()->getNumRefIdx(RefPicList(0))< getSlice()->getNumRefIdx(RefPicList(1)))
        MaxRef = getSlice()->getNumRefIdx(RefPicList(1));
    }
    for(Int lpRef = 0; lpRef < MaxRef; lpRef++)
    {
      for(Int lpNr = 0; lpNr < (getSlice()->isInterB() ? 2: 1); lpNr ++)
      {
        eRefPicList = RefPicList(0);
        if(getSlice()->isInterB())
          eRefPicList = RefPicList(lpNr==0 ? (getSlice()->getColDir()): (1-getSlice()->getColDir()));
        if(getSlice()->getRefPic( eRefPicList, lpRef)->getCandPicCheckedFlag())
          continue;
        
        if(lpRef >= getSlice()->getNumRefIdx(eRefPicList)||(m_pcSlice->getViewId() != getSlice()->getRefPic( eRefPicList, lpRef)->getViewId()))
          continue;
        if (uiViewIdxCurr > 1 ) 
        {
          if( (uiLCUnew >= 0 && xGetColDisMV( eRefPicList, lpRef, uiLCUnew, uiBRIdx, cColMv, iTargetViewIdx, iTStartViewIdx)) )
          {
          clipMv(cColMv);
          pDInfo->m_acMvCand[pDInfo->iN] = cColMv;
          pDInfo->m_aVIdxCan[pDInfo->iN++] = iTargetViewIdx;
          return ;
          }
        }
        for(UInt uiLoopVert = 0; uiLoopVert <= uiPUVert; uiLoopVert += 4)
          for( uiIdx = uiPUHorStart; uiIdx <= uiPUHorEnd; uiIdx += 4 )
          {
            uiAbsPartAddr = g_auiRasterToZscan[uiIdx+uiNumPartInCUWidth*uiLoopVert];
            if ( uiLCUIdx >= 0 && xGetColDisMV( eRefPicList, lpRef, uiLCUIdx, uiAbsPartAddr, cColMv, iTargetViewIdx, iTStartViewIdx ) ) 
            {
              clipMv(cColMv);
              pDInfo->m_acMvCand[pDInfo->iN] = cColMv;
              pDInfo->m_aVIdxCan[pDInfo->iN++] = iTargetViewIdx;
              return ;
            }
          }
        for(UInt uiIdx_y = 0; uiIdx_y <uiNumPartInCUHeight; uiIdx_y += 4 )
          for(UInt uiIdx_X = 0; uiIdx_X <uiNumPartInCUWidth; uiIdx_X += 4 )
          {
            uiIdx = uiIdx_X + uiIdx_y * uiNumPartInCUWidth;
            uiAbsPartAddr = g_auiRasterToZscan[uiIdx];
            if ( uiLCUIdx >= 0 && xGetColDisMV( eRefPicList,  lpRef, uiLCUIdx, uiAbsPartAddr, cColMv, iTargetViewIdx, iTStartViewIdx ) ) 
            {
              clipMv(cColMv);
              pDInfo->m_acMvCand[pDInfo->iN] = cColMv;
              pDInfo->m_aVIdxCan[pDInfo->iN++] = iTargetViewIdx;
              return ;
            }
          }
        if (uiViewIdxCurr == 1 ) 
        {
          if( (uiLCUnew >= 0 && xGetColDisMV( eRefPicList, lpRef, uiLCUnew, uiBRIdx, cColMv, iTargetViewIdx, iTStartViewIdx)) )
          {
            clipMv(cColMv);
            pDInfo->m_acMvCand[pDInfo->iN] = cColMv;
            pDInfo->m_aVIdxCan[pDInfo->iN++] = iTargetViewIdx;
            return ;
          }
        }
        getSlice()->getRefPic( eRefPicList, lpRef)->setCandPicCheckedFlag(1);
      }//reference lists
    }//reference indices
  }//remaining pictures
}

#if MERL_VSP_C0152
Pel TComDataCU::getMcpFromDM(TComPicYuv* pcBaseViewDepthPicYuv, TComMv* mv, Int iBlkX, Int iBlkY, Int iBlkWidth, Int iBlkHeight, Int* aiShiftLUT, Int iShiftPrec, Bool bSimpleDvpRefine)
{
  Int depStride =  pcBaseViewDepthPicYuv->getStride();

  Int iPictureWidth  = pcBaseViewDepthPicYuv->getWidth();
  Int iPictureHeight = pcBaseViewDepthPicYuv->getHeight();

  Int depthPosX = Clip3(0,   iPictureWidth - iBlkWidth,  iBlkX + (mv->getHor()>>2));
  Int depthPosY = Clip3(0,   iPictureHeight- iBlkHeight,  iBlkY + (mv->getVer()>>2));

  Pel *pDepthPel   = pcBaseViewDepthPicYuv->getLumaAddr() + depthPosX + depthPosY * depStride;
  Pel  maxDepthVal = 0;

  if ( bSimpleDvpRefine )
  {
    Int depthStartPosX = Clip3(0,   iPictureWidth - iBlkWidth,  iBlkX + (mv->getHor()>>2));
    Int depthStartPosY = Clip3(0,   iPictureHeight- iBlkHeight,  iBlkY + (mv->getVer()>>2));
    Int depthEndPosX   = Clip3(0,   iPictureWidth - 1,  iBlkX + iBlkWidth - 1 + (mv->getHor()>>2));
    Int depthEndPosY   = Clip3(0,   iPictureHeight - 1,  iBlkY + iBlkHeight - 1 + (mv->getVer()>>2));
    Int iCenterX = (depthStartPosX + depthEndPosX) >> 1;
    Int iCenterY = (depthStartPosY + depthEndPosY) >> 1;

    Pel *depthTL  = pcBaseViewDepthPicYuv->getLumaAddr();
    Int aiDepth[5];
    aiDepth[0] = depthTL[ (depthStartPosY) * depStride + depthStartPosX ];      // Left Top
    aiDepth[1] = depthTL[ (depthEndPosY)   * depStride + depthStartPosX ];      // Left Bottom
    aiDepth[2] = depthTL[ (depthStartPosY) * depStride + depthEndPosX   ];      // Right Top
    aiDepth[3] = depthTL[ (depthEndPosY)   * depStride + depthEndPosX   ];      // Right Bottom
    aiDepth[4] = depthTL[ (iCenterY)       * depStride + iCenterX       ];      // Center
    for (Int i = 0; i < 5; i++)
    {
      if (maxDepthVal < aiDepth[i])
        maxDepthVal = aiDepth[i];
    }
  }
  else
  {
    for (Int j = 0; j < iBlkHeight; j++)
    {
      for (Int i = 0; i < iBlkWidth; i++)
      {
        if (maxDepthVal < pDepthPel[i])
          maxDepthVal = pDepthPel[i];
  }
      pDepthPel += depStride;
    }
  }

  Int iDisp = aiShiftLUT[ maxDepthVal ] << iShiftPrec;

  return iDisp;
}
Void TComDataCU::estimateDVFromDM(UInt uiPartIdx, TComPic* picDepth, UInt uiPartAddr, TComMv* cMvPred, Bool bSimpleDvpRefine)
{
  if (picDepth)
  {
    UInt uiAbsPartAddrCurrCU = m_uiAbsIdxInLCU + uiPartAddr;
    Int iWidth, iHeight;
    getPartIndexAndSize( uiPartIdx, uiPartAddr, iWidth, iHeight ); // The modified value of uiPartAddr won't be used any more
    
    TComPicYuv* pcBaseViewDepthPicYuv = picDepth->getPicYuvRec();
    Int iBlkX = ( getAddr() % picDepth->getFrameWidthInCU() ) * g_uiMaxCUWidth  + g_auiRasterToPelX[ g_auiZscanToRaster[ uiAbsPartAddrCurrCU ] ];
    Int iBlkY = ( getAddr() / picDepth->getFrameWidthInCU() ) * g_uiMaxCUHeight + g_auiRasterToPelY[ g_auiZscanToRaster[ uiAbsPartAddrCurrCU ] ];
    Int* aiShiftLUT;
    Int  iShiftPrec;

    getSlice()->getBWVSPLUTParam(aiShiftLUT, iShiftPrec);

    Pel iDisp = getMcpFromDM( pcBaseViewDepthPicYuv, cMvPred, iBlkX, iBlkY, iWidth, iHeight, aiShiftLUT, iShiftPrec, bSimpleDvpRefine );
    cMvPred->setHor( iDisp );
    clipMv(*cMvPred);
  }
}
#endif

#if H3D_NBDV

Bool TComDataCU::xCheckSpatialNBDV( TComDataCU* pcTmpCU, UInt uiIdx, UInt uiPartIdx, UInt uiPartAddr, DisInfo* pNbDvInfo, Bool bSearchForMvpDv, McpDisInfo* paMvpDvInfo, UInt uiMvpDvPos )
{
  Bool bDepthRefine = true; 

  if( pcTmpCU != NULL && !pcTmpCU->isIntra( uiIdx ) )
  {
    Bool bTmpIsSkipped = pcTmpCU->isSkipped( uiIdx );
    for(Int iList = 0; iList < (getSlice()->isInterB() ? 2: 1); iList ++)
    {
      RefPicList eRefPicList = RefPicList(iList);
      Int      refId = pcTmpCU->getCUMvField(eRefPicList)->getRefIdx(uiIdx) ;
      TComMv cMvPred = pcTmpCU->getCUMvField(eRefPicList)->getMv(uiIdx);

      if( refId >= 0)
      {
        Int refViewIdx  = pcTmpCU->getSlice()->getRefViewId( eRefPicList, refId );
        if (refViewIdx != m_pcSlice->getViewId()) // DCP
        {          
          clipMv(cMvPred);

          TComPic* picDepth = getSlice()->getRefPicBaseDepth();

          if (picDepth && bDepthRefine)
            estimateDVFromDM(uiPartIdx, picDepth, uiPartAddr, &cMvPred, true);

          pNbDvInfo->m_acMvCand[ pNbDvInfo->iN   ] = cMvPred;
          pNbDvInfo->m_aVIdxCan[ pNbDvInfo->iN++ ] = refViewIdx;

          return true;
        }
        else if ( bSearchForMvpDv && cMvPred.m_bDvMcp && bTmpIsSkipped )
        {
          paMvpDvInfo->m_acMvCand[iList][ uiMvpDvPos ] = cMvPred;
          paMvpDvInfo->m_aVIdxCan[iList][ uiMvpDvPos ] = refViewIdx;
          paMvpDvInfo->m_bAvailab[iList][ uiMvpDvPos ] = true;
          paMvpDvInfo->m_bFound                        = true; 
        }
      }
#if MERL_VSP_C0152
      else if (pcTmpCU->getVSPIndex(uiIdx) != 0) // is VSP
      { 
        TComPic* picDepth = pcTmpCU->getSlice()->getRefPicBaseDepth();
        if (picDepth && bDepthRefine)
          estimateDVFromDM(uiPartIdx, picDepth, uiPartAddr, &cMvPred);

        cMvPred.setVer(0);

        pNbDvInfo->m_acMvCand[ pNbDvInfo->iN]   = cMvPred;
        pNbDvInfo->m_aVIdxCan[ pNbDvInfo->iN++] = 0; // refViewIdx;
        return true;
      }
#endif
    }
  }
  return false; 
}


Void TComDataCU::xDeriveRightBottomNbIdx( PartSize eCUMode, UInt uiPartIdx, Int &riLCUIdxRBNb, Int &riPartIdxRBNb )
{
  UInt uiNumPartInCUWidth = m_pcPic->getNumPartInWidth();  
    Int uiLCUIdx = getAddr();
  
  UInt uiPartIdxRB;
    deriveRightBottomIdx( eCUMode, uiPartIdx, uiPartIdxRB );  
    UInt uiAbsPartIdxTmp = g_auiZscanToRaster[uiPartIdxRB];

    if (( m_pcPic->getCU(m_uiCUAddr)->getCUPelX() + g_auiRasterToPelX[uiAbsPartIdxTmp] + m_pcPic->getMinCUWidth() )>= m_pcSlice->getSPS()->getPicWidthInLumaSamples() )
    {
    riLCUIdxRBNb  = -1;
    riPartIdxRBNb = -1;
    }
    else if(( m_pcPic->getCU(m_uiCUAddr)->getCUPelY() + g_auiRasterToPelY[uiAbsPartIdxTmp] + m_pcPic->getMinCUHeight() )>= m_pcSlice->getSPS()->getPicHeightInLumaSamples() )
    {
    riLCUIdxRBNb  = -1;
    riPartIdxRBNb = -1;
    }
    else
    {
      if ( ( uiAbsPartIdxTmp % uiNumPartInCUWidth < uiNumPartInCUWidth - 1 ) &&           // is not at the last column of LCU 
      ( uiAbsPartIdxTmp / uiNumPartInCUWidth < m_pcPic->getNumPartInHeight() - 1 ) ) // is not at the last row    of LCU
      {
      riPartIdxRBNb = g_auiRasterToZscan[ uiAbsPartIdxTmp + uiNumPartInCUWidth + 1 ];
      }
      else if ( uiAbsPartIdxTmp % uiNumPartInCUWidth < uiNumPartInCUWidth - 1 )           // is not at the last column of LCU But is last row of LCU
      {
      riPartIdxRBNb = -1;
      riLCUIdxRBNb  = -1;
      }
      else if ( uiAbsPartIdxTmp / uiNumPartInCUWidth < m_pcPic->getNumPartInHeight() - 1 ) // is not at the last row of LCU But is last column of LCU
      {
      riPartIdxRBNb = g_auiRasterToZscan[ uiAbsPartIdxTmp + 1 ];
      riLCUIdxRBNb = uiLCUIdx + 1;
      }
      else //is the right bottom corner of LCU                       
      {
      riPartIdxRBNb = -1;
      riLCUIdxRBNb  = -1;
      }
    }
      }


Void TComDataCU::getDisMvpCandNBDV( UInt uiPartIdx, UInt uiPartAddr, DisInfo* pDInfo , Bool bParMerge
#if MERL_VSP_C0152
                                , Bool bDepthRefine
#endif
                                )
{
  //// ******* Init variables ******* /////
  // Init disparity struct for results
  pDInfo->iN = 0;  

  // Init struct for disparities from MCP neighboring blocks
  McpDisInfo cMvpDvInfo;
  cMvpDvInfo.m_bFound = false; 
  for (UInt iCurDvMcpCand = 0; iCurDvMcpCand < MCP_DIS_CANS; iCurDvMcpCand++)
  {
    for (UInt iList = 0; iList < 2; iList++)
    {
      cMvpDvInfo.m_acMvCand[iList][iCurDvMcpCand].setZero();
      cMvpDvInfo.m_aVIdxCan[iList][iCurDvMcpCand] = 0; 
      cMvpDvInfo.m_bAvailab[iList][iCurDvMcpCand] = false; 
    }
  }

  // Get Positions  
  PartSize eCUMode    = getPartitionSize( uiPartAddr );    
  UInt uiPartIdxLT, uiPartIdxRT, uiPartIdxLB;  

  deriveLeftRightTopIdxGeneral( eCUMode, uiPartAddr, uiPartIdx, uiPartIdxLT, uiPartIdxRT );
  deriveLeftBottomIdxGeneral  ( eCUMode, uiPartAddr, uiPartIdx, uiPartIdxLB );

  // Get parameters for parallel merge
  Int xP, yP, nPSW, nPSH;
  if( bParMerge)
    this->getPartPosition(uiPartIdx, xP, yP, nPSW, nPSH);

  //// ******* Get disparity from temporal neighboring blocks ******* /////
  if ( getSlice()->getPPS()->getEnableTMVPFlag() )
  {
    TComMv cColMv;
    Int iTargetViewIdx = 0;
    Int iTStartViewIdx = 0;    

    ///*** Derive center position ***
    UInt uiPartIdxCenter;
    Int  uiLCUIdx   = getAddr();

    xDeriveCenterIdx( eCUMode, uiPartIdx, uiPartIdxCenter );

    ///*** Derive bottom right neighbour position ***
    eCUMode          = getPartitionSize( 0 ); // Necessary?    
    Int iLCUIdxRBNb  = -1;    
    Int iPartIdxRBNb = -1;

    xDeriveRightBottomNbIdx( eCUMode, uiPartIdx, iLCUIdxRBNb, iPartIdxRBNb );

    ///*** Search temporal candidate pictures for disparity vector ***
    const Int iNumCandPics = 2;
    for(Int curCandPic = 0; curCandPic < iNumCandPics; curCandPic++)
    {
      // Get candidate picture
      RefPicList eCurRefPicList   = REF_PIC_LIST_0 ;
      Int        curCandPicRefIdx = 0;

      if( curCandPic == 0 ) // check the co-located picture
      { 
        eCurRefPicList   = RefPicList(getSlice()->isInterB() ? getSlice()->getColDir() : 0);
        curCandPicRefIdx = getSlice()->getColRefIdx();
      }
      else                  // check RAP
      {
        if( !(getPic()->getRapbCheck()) )
          break;

        eCurRefPicList   = getPic()->getRapRefList();
        curCandPicRefIdx = getPic()->getRapRefIdx();
      }

      // Check BR and Center       
      if( m_pcSlice->getViewId() == getSlice()->getRefPic( eCurRefPicList, curCandPicRefIdx)->getViewId() ) 
      {
        for(Int curPosition = 0; curPosition < 2; curPosition++) 
        {
          Bool bCheck = false; 
          if ( curPosition == 0 && iLCUIdxRBNb >= 0 )
            bCheck = xGetColDisMV( eCurRefPicList, curCandPicRefIdx, iLCUIdxRBNb, iPartIdxRBNb,  cColMv, iTargetViewIdx, iTStartViewIdx);

          if (curPosition == 1 )
            bCheck = xGetColDisMV( eCurRefPicList, curCandPicRefIdx, uiLCUIdx,   uiPartIdxCenter,  cColMv, iTargetViewIdx, iTStartViewIdx );

          if( bCheck )
          {
            clipMv(cColMv);
            TComPic* picDepth = getSlice()->getRefPicBaseDepth();
            if (picDepth && bDepthRefine)
              estimateDVFromDM(uiPartIdx, picDepth, uiPartAddr, &cColMv, true);

            pDInfo->m_acMvCand[ pDInfo->iN] = cColMv;
            pDInfo->m_aVIdxCan[ pDInfo->iN++] = iTargetViewIdx;
            return ;
          }
        } // Loop positions
      }
    } // Loop candidate views
  } // if TMVP Flag

  UInt uiIdx = 0;
  Bool        bCheckMcpDv = false;   
  TComDataCU* pcTmpCU     = NULL;


  //// ******* Get disparity from left block ******* /////
  pcTmpCU = getPULeft(uiIdx, uiPartIdxLB, true, false);
  if ( uiPartIdx == 1 && (eCUMode == SIZE_Nx2N || eCUMode == SIZE_nLx2N || eCUMode == SIZE_nRx2N) )
  {
    pcTmpCU = NULL;
  }

  if (pcTmpCU && bParMerge) 
  {
    if (!pcTmpCU->isDiffMER(xP -1, yP+nPSH-1, xP, yP))
    {
      pcTmpCU = NULL;
    }
  }

  bCheckMcpDv = true; 
  if ( xCheckSpatialNBDV( pcTmpCU, uiIdx, uiPartIdx, uiPartAddr, pDInfo, bCheckMcpDv, &cMvpDvInfo, DVFROM_LEFT ) )
    return;


  //// ******* Get disparity from above block ******* /////
  pcTmpCU = getPUAbove(uiIdx, uiPartIdxRT, true, false, true);
  if ( uiPartIdx == 1 && (eCUMode == SIZE_2NxN || eCUMode == SIZE_2NxnU || eCUMode == SIZE_2NxnD) )
  {
    pcTmpCU = NULL;
  }

  if (pcTmpCU && bParMerge) 
  {
    if (!pcTmpCU->isDiffMER(xP+nPSW-1, yP-1, xP, yP))
    {
      pcTmpCU = NULL;
    }
  }

  if(pcTmpCU != NULL )
  {
    bCheckMcpDv = ( ( getAddr() - pcTmpCU->getAddr() ) == 0);
    if ( xCheckSpatialNBDV( pcTmpCU, uiIdx, uiPartIdx, uiPartAddr, pDInfo, bCheckMcpDv, &cMvpDvInfo, DVFROM_ABOVE ) )
      return;
  }

  //// ******* Get disparity from above right block ******* /////
  pcTmpCU = getPUAboveRight(uiIdx, uiPartIdxRT, true, false, true);

  if (pcTmpCU && bParMerge) 
  {
    if (!pcTmpCU->isDiffMER(xP+nPSW, yP-1, xP, yP))
    {
      pcTmpCU = NULL;
    }
  }

  if(pcTmpCU != NULL )
  {
    bCheckMcpDv = ( ( getAddr() - pcTmpCU->getAddr() ) == 0);
    if ( xCheckSpatialNBDV( pcTmpCU, uiIdx, uiPartIdx, uiPartAddr, pDInfo, bCheckMcpDv, &cMvpDvInfo, DVFROM_ABOVERIGHT ) )
      return;
  }


  //// ******* Get disparity from below left block ******* /////
  pcTmpCU = getPUBelowLeft(uiIdx, uiPartIdxLB, true, false);

  if (pcTmpCU && bParMerge)
  {
    if (!pcTmpCU->isDiffMER(xP-1, yP+nPSH, xP, yP))
    {
      pcTmpCU = NULL;
    }
  }

  if( pcTmpCU != NULL )
  {
    bCheckMcpDv = true; 
    if ( xCheckSpatialNBDV( pcTmpCU, uiIdx, uiPartIdx, uiPartAddr, pDInfo, bCheckMcpDv, &cMvpDvInfo, DVFROM_LEFTBELOW ) )
      return;
  }


  //// ******* Get disparity from above left block ******* /////
  pcTmpCU = getPUAboveLeft(uiIdx, (m_uiAbsIdxInLCU + uiPartAddr), true, false, true);
  assert(uiPartIdxLT == (m_uiAbsIdxInLCU + uiPartAddr));

  if (pcTmpCU && bParMerge) 
  {
    if (!pcTmpCU->isDiffMER(xP-1, yP-1, xP, yP))
    {
      pcTmpCU = NULL;
    }
  }

  if( pcTmpCU != NULL )
  {
    bCheckMcpDv = (( getAddr() - pcTmpCU->getAddr() ) <= 1); 
    if ( xCheckSpatialNBDV( pcTmpCU, uiIdx, uiPartIdx, uiPartAddr, pDInfo, bCheckMcpDv, &cMvpDvInfo, DVFROM_ABOVELEFT ) )
      return;
  }

  //// ******* Search MCP blocks ******* /////
  if( cMvpDvInfo.m_bFound ) 
  {
    for( Int curPos = 1 ; curPos < MCP_DIS_CANS - 1 ; curPos++ ) 
    {
      for(Int iList = 0; iList < (getSlice()->isInterB() ? 2: 1); iList ++)
      {
        if( cMvpDvInfo.m_bAvailab[iList][curPos] )
        {
          TComMv cDispVec = cMvpDvInfo.m_acMvCand[iList][ curPos ];
          clipMv( cDispVec );

          TComPic* picDepth = getSlice()->getRefPicBaseDepth();
          if (picDepth && bDepthRefine)
            estimateDVFromDM(uiPartIdx, picDepth, uiPartAddr, &cDispVec, true);

          pDInfo->m_acMvCand[ pDInfo->iN]   = cDispVec;
          pDInfo->m_aVIdxCan[ pDInfo->iN++] = 0;
          return;
        }
      }
    }
  }
  return;
}
#endif

#endif

/** Constructs a list of candidates for AMVP
 * \param uiPartIdx
 * \param uiPartAddr 
 * \param eRefPicList
 * \param iRefIdx
 * \param pInfo
 */
#if H3D_IVMP
Void TComDataCU::fillMvpCand ( UInt uiPartIdx, UInt uiPartAddr, RefPicList eRefPicList, Int iRefIdx, AMVPInfo* pInfo, Int iMVPIdx)
{
  if (!m_pcSlice->getSPS()->getViewId() || !m_pcSlice->getSPS()->getMultiviewMvPredMode())
  {
    // HEVC
    fillMvpCandBase(uiPartIdx, uiPartAddr, eRefPicList, iRefIdx, pInfo);
  }
  else
  {
    if (iMVPIdx!=0)
    {
      // HEVC part
      fillMvpCandBase(uiPartIdx, uiPartAddr, eRefPicList, iRefIdx, pInfo);
      if (iRefIdx < 0)
      {
        return;
      }
      for (Int j = AMVP_MAX_NUM_CANDS - 1; j >= 0; j--)
      {
        pInfo->m_acMvCand[j+1] = pInfo->m_acMvCand[j];
      }
      pInfo->iN++;
    }
    if (iMVPIdx<=0)
    {
#if H3D_NBDV
      // Extension part
      DisInfo cDisInfo;
      cDisInfo.iN = 0;
#if FCO_DVP_REFINE_C0132_C0170
      if( !getPic()->getDepthCoded() )
#endif
      getDisMvpCandNBDV(uiPartIdx, uiPartAddr, &cDisInfo, false
#if MERL_VSP_C0152
            , true
#endif
              ); 
#if FCO_DVP_REFINE_C0132_C0170
      if(getPic()->getDepthCoded() )
      {
        TComPic*      pcCodedDepthMap = getPic()->getRecDepthMap();
        TComMv        cColMv;

        cColMv.setZero();
        estimateDVFromDM(uiPartIdx, pcCodedDepthMap, uiPartAddr, &cColMv, false);

        cDisInfo.iN = 1;
        cDisInfo.m_acMvCand[0].setHor( cColMv.getHor() );
        cDisInfo.m_acMvCand[0].setVer( cColMv.getVer() );
        cDisInfo.m_aVIdxCan[0] = 0;
      }
#endif
      if(cDisInfo.iN==0)
      {
        cDisInfo.iN = 1;
        cDisInfo.m_acMvCand[0].setHor(0);
        cDisInfo.m_acMvCand[0].setVer(0);
        cDisInfo.m_aVIdxCan[0] = 0;
      }
      Int paiPdmRefIdx[4] = {-1, -1, -1, -1};
      Int iPdmDir[4]      = {-1, -1, -1, -1};
      TComMv cPdmMvPred[4];
      cPdmMvPred[0].m_bDvMcp = cPdmMvPred[1].m_bDvMcp = false; 
      if(getUnifiedMvPredCan(uiPartIdx, eRefPicList, iRefIdx, paiPdmRefIdx, cPdmMvPred, &cDisInfo, iPdmDir, false))
#else // H3D_NBDV
      if( getPdmMvPred( uiPartIdx, eRefPicList, iRefIdx, cPdmMvPred ) )
#endif

      {
        clipMv( cPdmMvPred[0] );
        pInfo->m_acMvCand[0] = cPdmMvPred[0];
      }
      else
      {
        pInfo->m_acMvCand[0].set(0,0);
      }
    }
  }
}


Void TComDataCU::fillMvpCandBase( UInt uiPartIdx, UInt uiPartAddr, RefPicList eRefPicList, Int iRefIdx, AMVPInfo* pInfo )
#else //  H3D_IVMP  
Void TComDataCU::fillMvpCand ( UInt uiPartIdx, UInt uiPartAddr, RefPicList eRefPicList, Int iRefIdx, AMVPInfo* pInfo )
#endif //  H3D_IVMP 
{
  PartSize eCUMode = getPartitionSize( 0 );
  
  TComMv cMvPred;
  Bool bAddedSmvp = false;

  pInfo->iN = 0;  
  if (iRefIdx < 0)
  {
    return;
  }

  //-- Get Spatial MV
  UInt uiPartIdxLT, uiPartIdxRT, uiPartIdxLB;
  UInt uiNumPartInCUWidth = m_pcPic->getNumPartInWidth();
  Bool bAdded = false;
  
  deriveLeftRightTopIdx( eCUMode, uiPartIdx, uiPartIdxLT, uiPartIdxRT );
  deriveLeftBottomIdx( eCUMode, uiPartIdx, uiPartIdxLB );
  
  TComDataCU* tmpCU = NULL;
  UInt idx;
  tmpCU = getPUBelowLeft(idx, uiPartIdxLB, true, false);
  bAddedSmvp = (tmpCU != NULL) && (tmpCU->getPredictionMode(idx) != MODE_INTRA);

  if (!bAddedSmvp)
  {
    tmpCU = getPULeft(idx, uiPartIdxLB, true, false);
    bAddedSmvp = (tmpCU != NULL) && (tmpCU->getPredictionMode(idx) != MODE_INTRA);
  }

  // Left predictor search
  bAdded = xAddMVPCand( pInfo, eRefPicList, iRefIdx, uiPartIdxLB, MD_BELOW_LEFT);
  if (!bAdded) 
  {
    bAdded = xAddMVPCand( pInfo, eRefPicList, iRefIdx, uiPartIdxLB, MD_LEFT );
  }
  
  if(!bAdded)
  {
    bAdded = xAddMVPCandOrder( pInfo, eRefPicList, iRefIdx, uiPartIdxLB, MD_BELOW_LEFT);
    if (!bAdded) 
    {
      bAdded = xAddMVPCandOrder( pInfo, eRefPicList, iRefIdx, uiPartIdxLB, MD_LEFT );
    }
  }

  // Above predictor search
  bAdded = xAddMVPCand( pInfo, eRefPicList, iRefIdx, uiPartIdxRT, MD_ABOVE_RIGHT);

  if (!bAdded) 
  {
    bAdded = xAddMVPCand( pInfo, eRefPicList, iRefIdx, uiPartIdxRT, MD_ABOVE);
  }

  if(!bAdded)
  {
    bAdded = xAddMVPCand( pInfo, eRefPicList, iRefIdx, uiPartIdxLT, MD_ABOVE_LEFT);
  }
  bAdded = bAddedSmvp;
  if (pInfo->iN==2) bAdded = true;

  if(!bAdded)
  {
    bAdded = xAddMVPCandOrder( pInfo, eRefPicList, iRefIdx, uiPartIdxRT, MD_ABOVE_RIGHT);
    if (!bAdded) 
    {
      bAdded = xAddMVPCandOrder( pInfo, eRefPicList, iRefIdx, uiPartIdxRT, MD_ABOVE);
    }

    if(!bAdded)
    {
      bAdded = xAddMVPCandOrder( pInfo, eRefPicList, iRefIdx, uiPartIdxLT, MD_ABOVE_LEFT);
    }
  }
  
  if (getAMVPMode(uiPartAddr) == AM_NONE)  //Should be optimized later for special cases
  {
    assert(pInfo->iN > 0);
    pInfo->iN = 1;
    return;
  }


  if ( pInfo->iN == 2 )
  {
    if ( pInfo->m_acMvCand[ 0 ] == pInfo->m_acMvCand[ 1 ] )
    {
      pInfo->iN = 1;
#if QC_MRG_CANS_B0048
      pInfo->m_acMvCand[ 1 ].set(0, 0);
#endif
    }
  }

  if ( getSlice()->getPPS()->getEnableTMVPFlag() )
  {
    // Get Temporal Motion Predictor
    int iRefIdx_Col = iRefIdx;
    TComMv cColMv;
    UInt uiPartIdxRB;
    UInt uiAbsPartIdx;  
    UInt uiAbsPartAddr;
    int uiLCUIdx = getAddr();

    deriveRightBottomIdx( eCUMode, uiPartIdx, uiPartIdxRB );
    uiAbsPartAddr = m_uiAbsIdxInLCU + uiPartAddr;

    //----  co-located RightBottom Temporal Predictor (H) ---//
    uiAbsPartIdx = g_auiZscanToRaster[uiPartIdxRB];
    if ( ( m_pcPic->getCU(m_uiCUAddr)->getCUPelX() + g_auiRasterToPelX[uiAbsPartIdx] + m_pcPic->getMinCUWidth() ) >= m_pcSlice->getSPS()->getPicWidthInLumaSamples() )  // image boundary check
    {
      uiLCUIdx = -1;
    }
    else if ( ( m_pcPic->getCU(m_uiCUAddr)->getCUPelY() + g_auiRasterToPelY[uiAbsPartIdx] + m_pcPic->getMinCUHeight() ) >= m_pcSlice->getSPS()->getPicHeightInLumaSamples() )
    {
      uiLCUIdx = -1;
    }
    else
    {
      if ( ( uiAbsPartIdx % uiNumPartInCUWidth < uiNumPartInCUWidth - 1 ) &&           // is not at the last column of LCU 
        ( uiAbsPartIdx / uiNumPartInCUWidth < m_pcPic->getNumPartInHeight() - 1 ) ) // is not at the last row    of LCU
      {
        uiAbsPartAddr = g_auiRasterToZscan[ uiAbsPartIdx + uiNumPartInCUWidth + 1 ];
        uiLCUIdx = getAddr();
      }
      else if ( uiAbsPartIdx % uiNumPartInCUWidth < uiNumPartInCUWidth - 1 )           // is not at the last column of LCU But is last row of LCU
      {
        uiAbsPartAddr = g_auiRasterToZscan[ (uiAbsPartIdx + uiNumPartInCUWidth + 1) % m_pcPic->getNumPartInCU() ];
        uiLCUIdx      = -1 ; 
      }
      else if ( uiAbsPartIdx / uiNumPartInCUWidth < m_pcPic->getNumPartInHeight() - 1 ) // is not at the last row of LCU But is last column of LCU
      {
        uiAbsPartAddr = g_auiRasterToZscan[ uiAbsPartIdx + 1 ];
        uiLCUIdx = getAddr() + 1;
      }
      else //is the right bottom corner of LCU                       
      {
        uiAbsPartAddr = 0;
        uiLCUIdx      = -1 ; 
      }
    }
    if ( uiLCUIdx >= 0 && xGetColMVP( eRefPicList, uiLCUIdx, uiAbsPartAddr, cColMv, iRefIdx_Col ) )
    {
#if H3D_NBDV
      cColMv.m_bDvMcp = false;
#endif
      pInfo->m_acMvCand[pInfo->iN++] = cColMv;
    }
    else 
    {
      UInt uiPartIdxCenter;
      UInt uiCurLCUIdx = getAddr();
      xDeriveCenterIdx( eCUMode, uiPartIdx, uiPartIdxCenter );
      if (xGetColMVP( eRefPicList, uiCurLCUIdx, uiPartIdxCenter,  cColMv, iRefIdx_Col ))
      {
#if H3D_NBDV
        cColMv.m_bDvMcp = false;
#endif
        pInfo->m_acMvCand[pInfo->iN++] = cColMv;
      }
    }
    //----  co-located RightBottom Temporal Predictor  ---//
  }


  if (pInfo->iN > AMVP_MAX_NUM_CANDS)
  {
    pInfo->iN = AMVP_MAX_NUM_CANDS;
  }
  else if (pInfo->iN < AMVP_MAX_NUM_CANDS)
  {
      pInfo->m_acMvCand[pInfo->iN].set(0,0);
      pInfo->iN++;
  }
  return ;
}

Void TComDataCU::clipMv    (TComMv&  rcMv)
{
  Int  iMvShift = 2;
#if HHI_FULL_PEL_DEPTH_MAP_MV_ACC
  if( getSlice()->getIsDepth() )
    iMvShift = 0;
#endif
  Int iOffset = 8;
  Int iHorMax = ( m_pcSlice->getSPS()->getPicWidthInLumaSamples() + iOffset - m_uiCUPelX - 1 ) << iMvShift;
  Int iHorMin = (       -(Int)g_uiMaxCUWidth - iOffset - (Int)m_uiCUPelX + 1 ) << iMvShift;
  
  Int iVerMax = ( m_pcSlice->getSPS()->getPicHeightInLumaSamples() + iOffset - m_uiCUPelY - 1 ) << iMvShift;
  Int iVerMin = (       -(Int)g_uiMaxCUHeight - iOffset - (Int)m_uiCUPelY + 1 ) << iMvShift;
  
  rcMv.setHor( min (iHorMax, max (iHorMin, rcMv.getHor())) );
  rcMv.setVer( min (iVerMax, max (iVerMin, rcMv.getVer())) );
}


Void TComDataCU::convertTransIdx( UInt uiAbsPartIdx, UInt uiTrIdx, UInt& ruiLumaTrMode, UInt& ruiChromaTrMode )
{
  ruiLumaTrMode   = uiTrIdx;
  ruiChromaTrMode = uiTrIdx;
  return;
}

UInt TComDataCU::getIntraSizeIdx(UInt uiAbsPartIdx)
{
  UInt uiShift = ( (m_puhTrIdx[uiAbsPartIdx]==0) && (m_pePartSize[uiAbsPartIdx]==SIZE_NxN) ) ? m_puhTrIdx[uiAbsPartIdx]+1 : m_puhTrIdx[uiAbsPartIdx];
  uiShift = ( m_pePartSize[uiAbsPartIdx]==SIZE_NxN ? 1 : 0 );
  
  UChar uiWidth = m_puhWidth[uiAbsPartIdx]>>uiShift;
  UInt  uiCnt = 0;
  while( uiWidth )
  {
    uiCnt++;
    uiWidth>>=1;
  }
  uiCnt-=2;
  return uiCnt > 6 ? 6 : uiCnt;
}

Void TComDataCU::clearCbf( UInt uiIdx, TextType eType, UInt uiNumParts )
{
  ::memset( &m_puhCbf[g_aucConvertTxtTypeToIdx[eType]][uiIdx], 0, sizeof(UChar)*uiNumParts);
}

/** Set a I_PCM flag for all sub-partitions of a partition.
 * \param bIpcmFlag I_PCM flag
 * \param uiAbsPartIdx patition index
 * \param uiDepth CU depth
 * \returns Void
 */
Void TComDataCU::setIPCMFlagSubParts  (Bool bIpcmFlag, UInt uiAbsPartIdx, UInt uiDepth)
{
  UInt uiCurrPartNumb = m_pcPic->getNumPartInCU() >> (uiDepth << 1);

  memset(m_pbIPCMFlag + uiAbsPartIdx, bIpcmFlag, sizeof(Bool)*uiCurrPartNumb );
}

/** Test whether the current block is skipped
 * \param uiPartIdx Block index
 * \returns Flag indicating whether the block is skipped
 */
Bool TComDataCU::isSkipped( UInt uiPartIdx )
{
  if ( m_pcSlice->isIntra () )
  {
    return false;
  }
  return ( m_pePredMode[ uiPartIdx ] == MODE_SKIP && getMergeFlag( uiPartIdx ) && !getQtRootCbf( uiPartIdx ) );
}

// ====================================================================================================================
// Protected member functions
// ====================================================================================================================

Bool TComDataCU::xAddMVPCand( AMVPInfo* pInfo, RefPicList eRefPicList, Int iRefIdx, UInt uiPartUnitIdx, MVP_DIR eDir )
{
  TComDataCU* pcTmpCU = NULL;
  UInt uiIdx;
  switch( eDir )
  {
    case MD_LEFT:
    {
      pcTmpCU = getPULeft(uiIdx, uiPartUnitIdx, true, false);
      break;
    }
    case MD_ABOVE:
    {
      pcTmpCU = getPUAbove(uiIdx, uiPartUnitIdx, true, false, true);
      break;
    }
    case MD_ABOVE_RIGHT:
    {
      pcTmpCU = getPUAboveRight(uiIdx, uiPartUnitIdx, true, false, true);
      break;
    }
    case MD_BELOW_LEFT:
    {
      pcTmpCU = getPUBelowLeft(uiIdx, uiPartUnitIdx, true, false);
      break;
    }
    case MD_ABOVE_LEFT:
    {
      pcTmpCU = getPUAboveLeft(uiIdx, uiPartUnitIdx, true, false, true);
      break;
    }
    default:
    {
      break;
    }
  }
 
#if MERL_VSP_C0152
  if(pcTmpCU != NULL && pcTmpCU->getVSPIndex(uiIdx))
  {
    return false;
  }
#endif

  if ( pcTmpCU != NULL && m_pcSlice->isEqualRef(eRefPicList, pcTmpCU->getCUMvField(eRefPicList)->getRefIdx(uiIdx), iRefIdx) )
  {
    TComMv cMvPred = pcTmpCU->getCUMvField(eRefPicList)->getMv(uiIdx);
#if H3D_NBDV
    cMvPred.m_bDvMcp = false;
#endif 
    pInfo->m_acMvCand[ pInfo->iN++] = cMvPred;
    return true;
  }

  if ( pcTmpCU == NULL ) 
  {
    return false;
  }
  
  RefPicList eRefPicList2nd = REF_PIC_LIST_0;
  if(       eRefPicList == REF_PIC_LIST_0 )
  {
    eRefPicList2nd = REF_PIC_LIST_1;
  }
  else if ( eRefPicList == REF_PIC_LIST_1)
  {
    eRefPicList2nd = REF_PIC_LIST_0;
  }


  Int iCurrRefPOC = m_pcSlice->getRefPic( eRefPicList, iRefIdx)->getPOC();
  Int iNeibRefPOC;


  if( pcTmpCU->getCUMvField(eRefPicList2nd)->getRefIdx(uiIdx) >= 0 )
  {
#if QC_IV_AS_LT_B0046
    Bool bIsCurrRefLongTerm = m_pcSlice->getRefPic( eRefPicList, iRefIdx)->getIsLongTerm();
    Bool bIsNeibRefLongTerm = m_pcSlice->getRefPic( eRefPicList2nd, pcTmpCU->getCUMvField(eRefPicList2nd)->getRefIdx(uiIdx))->getIsLongTerm();
    iNeibRefPOC = pcTmpCU->getSlice()->getRefPOC( eRefPicList2nd, pcTmpCU->getCUMvField(eRefPicList2nd)->getRefIdx(uiIdx) );
    if ( (bIsCurrRefLongTerm == bIsNeibRefLongTerm) && (iNeibRefPOC == iCurrRefPOC) )
#else
    if( pcTmpCU->getSlice()->getRefViewId( eRefPicList2nd, pcTmpCU->getCUMvField(eRefPicList2nd)->getRefIdx(uiIdx) ) != m_pcSlice->getRefViewId( eRefPicList, iRefIdx ) )
    {
      return false;
    }
    iNeibRefPOC = pcTmpCU->getSlice()->getRefPOC( eRefPicList2nd, pcTmpCU->getCUMvField(eRefPicList2nd)->getRefIdx(uiIdx) );
    if( iNeibRefPOC == iCurrRefPOC ) // Same Reference Frame But Diff List//
#endif
    {
      TComMv cMvPred = pcTmpCU->getCUMvField(eRefPicList2nd)->getMv(uiIdx);
#if H3D_NBDV
      cMvPred.m_bDvMcp = false;
#endif
      pInfo->m_acMvCand[ pInfo->iN++] = cMvPred;
      return true;
    }
  }
  return false;
}


/** 
 * \param pInfo
 * \param eRefPicList 
 * \param iRefIdx
 * \param uiPartUnitIdx
 * \param eDir
 * \returns Bool
 */
Bool TComDataCU::xAddMVPCandOrder( AMVPInfo* pInfo, RefPicList eRefPicList, Int iRefIdx, UInt uiPartUnitIdx, MVP_DIR eDir )
{
  TComDataCU* pcTmpCU = NULL;
  UInt uiIdx;

  switch( eDir )
  {
  case MD_LEFT:
    {
      pcTmpCU = getPULeft(uiIdx, uiPartUnitIdx, true, false);
      break;
    }
  case MD_ABOVE:
    {
      pcTmpCU = getPUAbove(uiIdx, uiPartUnitIdx, true, false, true);
      break;
    }
  case MD_ABOVE_RIGHT:
    {
      pcTmpCU = getPUAboveRight(uiIdx, uiPartUnitIdx, true, false, true);
      break;
    }
  case MD_BELOW_LEFT:
    {
      pcTmpCU = getPUBelowLeft(uiIdx, uiPartUnitIdx, true, false);
      break;
    }
  case MD_ABOVE_LEFT:
    {
      pcTmpCU = getPUAboveLeft(uiIdx, uiPartUnitIdx, true, false, true);
      break;
    }
  default:
    {
      break;
    }
  }

  if ( pcTmpCU == NULL ) 
  {
    return false;
  }

#if MERL_VSP_C0152
  if(pcTmpCU->getVSPIndex(uiIdx))
  {
    return false;
  }
#endif
  RefPicList eRefPicList2nd = REF_PIC_LIST_0;
  if(       eRefPicList == REF_PIC_LIST_0 )
  {
    eRefPicList2nd = REF_PIC_LIST_1;
  }
  else if ( eRefPicList == REF_PIC_LIST_1)
  {
    eRefPicList2nd = REF_PIC_LIST_0;
  }

  Int iCurrPOC = m_pcSlice->getPOC();
  Int iCurrRefPOC = m_pcSlice->getRefPic( eRefPicList, iRefIdx)->getPOC();
  Int iNeibPOC = iCurrPOC;
  Int iNeibRefPOC;
#if INTER_VIEW_VECTOR_SCALING_C0116
  Int iCurrViewId = m_pcSlice->getViewOrderIdx(); // will be changed to view_id
  Int iCurrRefViewId = m_pcSlice->getRefPic(eRefPicList, iRefIdx)->getViewOrderIdx(); // will be changed to view_id
  Int iNeibViewId = iCurrViewId;
  Int iNeibRefViewId;
#endif
#if QC_IV_AS_LT_B0046
  Bool bIsCurrRefLongTerm = m_pcSlice->getRefPic( eRefPicList, iRefIdx)->getIsLongTerm();
  Bool bIsNeibRefLongTerm = false;
#endif
  if( pcTmpCU->getCUMvField(eRefPicList2nd)->getRefIdx(uiIdx) >= 0 )
  {
    iNeibRefPOC = pcTmpCU->getSlice()->getRefPOC( eRefPicList2nd, pcTmpCU->getCUMvField(eRefPicList2nd)->getRefIdx(uiIdx) );
#if INTER_VIEW_VECTOR_SCALING_C0116
    iNeibRefViewId = pcTmpCU->getSlice()->getRefPic(eRefPicList2nd, pcTmpCU->getCUMvField(eRefPicList2nd)->getRefIdx(uiIdx))->getViewOrderIdx(); // will be changed to view_id
#endif
#if QC_IV_AS_LT_B0046
    bIsNeibRefLongTerm = m_pcSlice->getRefPic( eRefPicList2nd, pcTmpCU->getCUMvField(eRefPicList2nd)->getRefIdx(uiIdx))->getIsLongTerm() ;
#if INTER_VIEW_VECTOR_SCALING_C0116
    if ( (bIsCurrRefLongTerm == bIsNeibRefLongTerm) && ((iNeibRefPOC == iCurrRefPOC) && (iNeibRefViewId == iCurrRefViewId)))
#else
    if ( (bIsCurrRefLongTerm == bIsNeibRefLongTerm) && (iNeibRefPOC == iCurrRefPOC) )
#endif
#else
    if( pcTmpCU->getSlice()->getRefViewId( eRefPicList2nd, pcTmpCU->getCUMvField(eRefPicList2nd)->getRefIdx(uiIdx) ) != m_pcSlice->getRefViewId( eRefPicList, iRefIdx ) )
      return false;
    if( iNeibRefPOC == iCurrRefPOC ) // Same Reference Frame But Diff List//
#endif
    {
      TComMv cMvPred = pcTmpCU->getCUMvField(eRefPicList2nd)->getMv(uiIdx);
#if H3D_NBDV
      cMvPred.m_bDvMcp = false;
#endif
      clipMv(cMvPred);
      pInfo->m_acMvCand[ pInfo->iN++] = cMvPred;
      return true;
    }
  }

  //---------------  V1 (END) ------------------//
  if( pcTmpCU->getCUMvField(eRefPicList)->getRefIdx(uiIdx) >= 0)
  {
    iNeibRefPOC = pcTmpCU->getSlice()->getRefPOC( eRefPicList, pcTmpCU->getCUMvField(eRefPicList)->getRefIdx(uiIdx) );
#if INTER_VIEW_VECTOR_SCALING_C0116
    iNeibRefViewId = pcTmpCU->getSlice()->getRefPic(eRefPicList, pcTmpCU->getCUMvField(eRefPicList)->getRefIdx(uiIdx))->getViewOrderIdx(); // will be changed to view_id
#endif
    TComMv cMvPred = pcTmpCU->getCUMvField(eRefPicList)->getMv(uiIdx);
    TComMv rcMv;
#if QC_IV_AS_LT_B0046
    bIsNeibRefLongTerm = m_pcSlice->getRefPic( eRefPicList, pcTmpCU->getCUMvField(eRefPicList)->getRefIdx(uiIdx))->getIsLongTerm(); 
    if ( bIsCurrRefLongTerm == bIsNeibRefLongTerm )
    {
#else
    if( pcTmpCU->getSlice()->getRefViewId( eRefPicList, pcTmpCU->getCUMvField(eRefPicList)->getRefIdx(uiIdx) ) != m_pcSlice->getRefViewId( eRefPicList, iRefIdx ) )
    {
      return false;
    }
#endif
#if INTER_VIEW_VECTOR_SCALING_C0116
    Int iScale = 4096;
    if((iCurrRefPOC != iNeibRefPOC)  )    // inter & inter
        iScale = xGetDistScaleFactor( iCurrPOC, iCurrRefPOC, iNeibPOC, iNeibRefPOC );
    else if(m_pcSlice->getIVScalingFlag())    // inter-view & inter-view
        iScale = xGetDistScaleFactor( iCurrViewId, iCurrRefViewId, iNeibViewId, iNeibRefViewId );
#else
    Int iScale = xGetDistScaleFactor( iCurrPOC, iCurrRefPOC, iNeibPOC, iNeibRefPOC );
#endif
    if ( iScale == 4096 )
    {
      rcMv = cMvPred;
    }
    else
    {
      rcMv = cMvPred.scaleMv( iScale );
    }
#if H3D_NBDV
    rcMv.m_bDvMcp = false;
#endif
    pInfo->m_acMvCand[ pInfo->iN++] = rcMv;
    return true;
#if QC_IV_AS_LT_B0046
    }
#endif
  }
  //---------------------- V2(END) --------------------//
  if( pcTmpCU->getCUMvField(eRefPicList2nd)->getRefIdx(uiIdx) >= 0)
  {
    iNeibRefPOC = pcTmpCU->getSlice()->getRefPOC( eRefPicList2nd, pcTmpCU->getCUMvField(eRefPicList2nd)->getRefIdx(uiIdx) );
#if INTER_VIEW_VECTOR_SCALING_C0116
    iNeibRefViewId = pcTmpCU->getSlice()->getRefPic(eRefPicList2nd, pcTmpCU->getCUMvField(eRefPicList2nd)->getRefIdx(uiIdx))->getViewOrderIdx(); // will be changed to view_id
#endif
    TComMv cMvPred = pcTmpCU->getCUMvField(eRefPicList2nd)->getMv(uiIdx);
    TComMv rcMv;
#if QC_IV_AS_LT_B0046
    bIsNeibRefLongTerm = m_pcSlice->getRefPic( eRefPicList2nd, pcTmpCU->getCUMvField(eRefPicList2nd)->getRefIdx(uiIdx))->getIsLongTerm(); ;
    if ( bIsCurrRefLongTerm == bIsNeibRefLongTerm )
    {
#else
    if( pcTmpCU->getSlice()->getRefViewId( eRefPicList2nd, pcTmpCU->getCUMvField(eRefPicList2nd)->getRefIdx(uiIdx) ) != m_pcSlice->getRefViewId( eRefPicList, iRefIdx ) )
    {
      return false;
    }
#endif
#if INTER_VIEW_VECTOR_SCALING_C0116
    Int iScale = 4096;
    if((iCurrRefPOC != iNeibRefPOC))    // inter & inter
        iScale = xGetDistScaleFactor( iCurrPOC, iCurrRefPOC, iNeibPOC, iNeibRefPOC );
    else if(m_pcSlice->getIVScalingFlag())    // inter-view & inter-view
        iScale = xGetDistScaleFactor( iCurrViewId, iCurrRefViewId, iNeibViewId, iNeibRefViewId );
#else
    Int iScale = xGetDistScaleFactor( iCurrPOC, iCurrRefPOC, iNeibPOC, iNeibRefPOC );
#endif
    if ( iScale == 4096 )
    {
      rcMv = cMvPred;
    }
    else
    {
      rcMv = cMvPred.scaleMv( iScale );
    }
#if H3D_NBDV
    rcMv.m_bDvMcp = false;
#endif
    pInfo->m_acMvCand[ pInfo->iN++] = rcMv;
    return true;
#if QC_IV_AS_LT_B0046
  }
#endif
  }
  //---------------------- V3(END) --------------------//
  return false;
}

#if H3D_NBDV
Bool TComDataCU::xGetColDisMV( RefPicList eRefPicList, Int refidx, Int uiCUAddr, Int uiPartUnitIdx, TComMv& rcMv , Int & iTargetViewIdx, Int & iStartViewIdx )
{
  UInt uiAbsPartAddr = uiPartUnitIdx;

  RefPicList  eColRefPicList = REF_PIC_LIST_0;
  Int iColViewIdx, iColRefViewIdx;
  TComPic *pColPic = getSlice()->getRefPic( eRefPicList, refidx);
  TComDataCU *pColCU = pColPic->getCU( uiCUAddr );
  iColViewIdx = pColCU->getSlice()->getViewId();
#if MERL_VSP_C0152
  if( pColCU->getCUMvField(eColRefPicList)->getRefIdx(uiAbsPartAddr) < 0)
  {
    return false;
  }
#endif

  if (pColCU->getPic()==0||pColCU->getPartitionSize(uiPartUnitIdx)==SIZE_NONE||pColCU->isIntra(uiAbsPartAddr))
  {
    return false;
  }
  for (Int ilist = 0; ilist < (pColCU->getSlice()->isInterB()? 2:1); ilist++) 
  {
    if(pColCU->getSlice()->isInterB())
    {
        eColRefPicList = RefPicList(ilist);
    }

    Int iColRefIdx = pColCU->getCUMvField(eColRefPicList)->getRefIdx(uiAbsPartAddr);

    if (iColRefIdx < 0)
    {
      continue;
    }

    iColRefViewIdx = pColCU->getSlice()->getRefPic(eColRefPicList, iColRefIdx)->getViewId();

    if ( iColViewIdx    == iColRefViewIdx ) // temporal vector
    {
      continue;
    }
    else 
    {
      rcMv = pColCU->getCUMvField(eColRefPicList)->getMv(uiAbsPartAddr);
      rcMv.m_bDvMcp = false;
      iTargetViewIdx  = iColRefViewIdx ;
      iStartViewIdx   = iColViewIdx   ;
      return true;    
    }
  }

  return false;
}
#endif 
/** 
 * \param eRefPicList
 * \param uiCUAddr 
 * \param uiPartUnitIdx
 * \param riRefIdx
 * \returns Bool
 */
Bool TComDataCU::xGetColMVP( RefPicList eRefPicList, Int uiCUAddr, Int uiPartUnitIdx, TComMv& rcMv, Int& riRefIdx 
#if QC_TMVP_MRG_REFIDX_C0047
  ,
  Bool bMRG
#endif
)
{
  UInt uiAbsPartAddr = uiPartUnitIdx;

  RefPicList  eColRefPicList;
  Int iColPOC, iColRefPOC, iCurrPOC, iCurrRefPOC, iScale;
  TComMv cColMv;
#if INTER_VIEW_VECTOR_SCALING_C0115
  Int iColViewId, iColRefViewId, iCurrViewId, iCurrRefViewId;
#endif

  // use coldir.
#if COLLOCATED_REF_IDX
  TComPic *pColPic = getSlice()->getRefPic( RefPicList(getSlice()->isInterB() ? getSlice()->getColDir() : 0), getSlice()->getColRefIdx());
#else
  TComPic *pColPic = getSlice()->getRefPic( RefPicList(getSlice()->isInterB() ? getSlice()->getColDir() : 0), 0);
#endif
  TComDataCU *pColCU = pColPic->getCU( uiCUAddr );
  if(pColCU->getPic()==0||pColCU->getPartitionSize(uiPartUnitIdx)==SIZE_NONE)
  {
    return false;
  }
  iCurrPOC = m_pcSlice->getPOC();    
  iCurrRefPOC = m_pcSlice->getRefPic(eRefPicList, riRefIdx)->getPOC();
  iColPOC = pColCU->getSlice()->getPOC();  
#if INTER_VIEW_VECTOR_SCALING_C0115
  iCurrViewId = m_pcSlice->getViewOrderIdx(); // will be changed to view_id    
  iCurrRefViewId = m_pcSlice->getRefPic(eRefPicList, riRefIdx)->getViewOrderIdx(); // will be changed to view_id
  iColViewId = pColCU->getSlice()->getViewOrderIdx(); // will be changed to view_id
#endif

  if (pColCU->isIntra(uiAbsPartAddr))
  {
    return false;
  }

#if !INTER_VIEW_VECTOR_SCALING_C0115&!QC_IV_AS_LT_B0046
  if( m_pcSlice->getRefPic(eRefPicList, riRefIdx)->getViewId() != m_pcSlice->getViewId() )
    return false;
#endif

  if ( !pColPic->getUsedForTMVP() )
  {
    return false;
  }

  eColRefPicList = getSlice()->getCheckLDC() ? eRefPicList : RefPicList(1-getSlice()->getColDir());

  Int iColRefIdx = pColCU->getCUMvField(RefPicList(eColRefPicList))->getRefIdx(uiAbsPartAddr);

  if (iColRefIdx < 0 )
  {
    eColRefPicList = RefPicList(1 - eColRefPicList);
    iColRefIdx = pColCU->getCUMvField(RefPicList(eColRefPicList))->getRefIdx(uiAbsPartAddr);

    if (iColRefIdx < 0 )
    {
      return false;
    }
  }

  // Scale the vector.
  iColRefPOC = pColCU->getSlice()->getRefPOC(eColRefPicList, iColRefIdx);
#if INTER_VIEW_VECTOR_SCALING_C0115
  iColRefViewId = pColCU->getSlice()->getRefPic( eColRefPicList, pColCU->getCUMvField(eColRefPicList)->getRefIdx(uiAbsPartAddr))->getViewOrderIdx(); // will be changed to view_id 
#endif
#if !QC_IV_AS_LT_B0046
  if( pColCU->getSlice()->getRefViewId( eColRefPicList, iColRefIdx ) != pColCU->getSlice()->getViewId() )
  {
    return false;
  }
#else
  Bool bIsCurrRefLongTerm = m_pcSlice->getRefPic(eRefPicList, riRefIdx)->getIsLongTerm();
  Bool bIsColRefLongTerm = pColCU->getSlice()->getWasLongTerm(eColRefPicList, iColRefIdx); 
  if(bIsCurrRefLongTerm != bIsColRefLongTerm) 
  {
#if QC_TMVP_MRG_REFIDX_C0047
    cColMv = pColCU->getCUMvField(eColRefPicList)->getMv(uiAbsPartAddr);
    iCurrRefPOC = m_pcSlice->getRefPic(eRefPicList, riRefIdx)->getPOC();
    if(bMRG )
    {
      Int iUpdRefIdx  = m_pcSlice->getNewRefIdx(eRefPicList);
      if(iUpdRefIdx > 0 )
      {
        riRefIdx = iUpdRefIdx;
        bIsCurrRefLongTerm = m_pcSlice->getRefPic(eRefPicList, riRefIdx)->getIsLongTerm();
        iCurrRefPOC = m_pcSlice->getRefPic(eRefPicList, riRefIdx)->getPOC();
#if INTER_VIEW_VECTOR_SCALING_C0115
        iCurrRefViewId = m_pcSlice->getRefPic(eRefPicList, riRefIdx)->getViewOrderIdx(); // will be changed to view_id
#endif
      }
      else
        return false;
    }else
    {
#endif
    assert( ((iColPOC == iColRefPOC)&&(iCurrPOC != iCurrRefPOC))||((iColPOC != iColRefPOC)&&(iCurrPOC == iCurrRefPOC)));
    return false;
#if QC_TMVP_MRG_REFIDX_C0047
    }
#endif
  }
#endif
  cColMv = pColCU->getCUMvField(eColRefPicList)->getMv(uiAbsPartAddr);

  iCurrRefPOC = m_pcSlice->getRefPic(eRefPicList, riRefIdx)->getPOC();
#if QC_IV_AS_LT_B0046
  {
    assert( ((iColPOC != iColRefPOC)&&(iCurrPOC != iCurrRefPOC))||((iColPOC == iColRefPOC)&&(iCurrPOC == iCurrRefPOC)));
    if(!bIsCurrRefLongTerm)  //short-term
    {
      iScale = xGetDistScaleFactor(iCurrPOC, iCurrRefPOC, iColPOC, iColRefPOC);
      if ( iScale == 4096 )
      {
        rcMv = cColMv;
      }
      else
      {
        rcMv = cColMv.scaleMv( iScale );
      }
    }else
#if INTER_VIEW_VECTOR_SCALING_C0115
    {
        if((iCurrPOC == iCurrRefPOC) && m_pcSlice->getIVScalingFlag())    // inter-view & inter-view
            iScale = xGetDistScaleFactor( iCurrViewId, iCurrRefViewId, iColViewId, iColRefViewId );
        else
            iScale = 4096;            // inter & inter
      if ( iScale == 4096 )
      {
        rcMv = cColMv;
      }
      else
      {
        rcMv = cColMv.scaleMv( iScale );
      }
  }
#else
    rcMv = cColMv; //inter-view
#endif
  }
#endif

  return true;
}

UInt TComDataCU::xGetMvdBits(TComMv cMvd)
{
  return ( xGetComponentBits(cMvd.getHor()) + xGetComponentBits(cMvd.getVer()) );
}

UInt TComDataCU::xGetComponentBits(Int iVal)
{
  UInt uiLength = 1;
  UInt uiTemp   = ( iVal <= 0) ? (-iVal<<1)+1: (iVal<<1);
  
  assert ( uiTemp );
  
  while ( 1 != uiTemp )
  {
    uiTemp >>= 1;
    uiLength += 2;
  }
  
  return uiLength;
}


Int TComDataCU::xGetDistScaleFactor(Int iCurrPOC, Int iCurrRefPOC, Int iColPOC, Int iColRefPOC)
{
  Int iDiffPocD = iColPOC - iColRefPOC;
  Int iDiffPocB = iCurrPOC - iCurrRefPOC;
  
  if( iDiffPocD == iDiffPocB )
  {
    return 4096;
  }
  else
  {
    Int iTDB      = Clip3( -128, 127, iDiffPocB );
    Int iTDD      = Clip3( -128, 127, iDiffPocD );
    Int iX        = (0x4000 + abs(iTDD/2)) / iTDD;
    Int iScale    = Clip3( -4096, 4095, (iTDB * iX + 32) >> 6 );
    return iScale;
  }
}

/** 
 * \param eCUMode
 * \param uiPartIdx 
 * \param ruiPartIdxCenter
 * \returns Void
 */
Void TComDataCU::xDeriveCenterIdx( PartSize eCUMode, UInt uiPartIdx, UInt& ruiPartIdxCenter )
{
  UInt uiPartAddr;
  Int  iPartWidth;
  Int  iPartHeight;
  getPartIndexAndSize( uiPartIdx, uiPartAddr, iPartWidth, iPartHeight);
  
  ruiPartIdxCenter = m_uiAbsIdxInLCU+uiPartAddr; // partition origin.
  ruiPartIdxCenter = g_auiRasterToZscan[ g_auiZscanToRaster[ ruiPartIdxCenter ]
                                        + ( iPartHeight/m_pcPic->getMinCUHeight()  )/2*m_pcPic->getNumPartInWidth()
                                        + ( iPartWidth/m_pcPic->getMinCUWidth()  )/2];
}

/** 
 * \param uiPartIdx
 * \param eRefPicList 
 * \param iRefIdx
 * \param pcMv
 * \returns Bool
 */
Bool TComDataCU::xGetCenterCol( UInt uiPartIdx, RefPicList eRefPicList, int iRefIdx, TComMv *pcMv )
{
  PartSize eCUMode = getPartitionSize( 0 );
  
  Int iCurrPOC = m_pcSlice->getPOC();
  
  // use coldir.
#if COLLOCATED_REF_IDX
  TComPic *pColPic = getSlice()->getRefPic( RefPicList(getSlice()->isInterB() ? getSlice()->getColDir() : 0), getSlice()->getColRefIdx());
#else
  TComPic *pColPic = getSlice()->getRefPic( RefPicList(getSlice()->isInterB() ? getSlice()->getColDir() : 0), 0);
#endif
  TComDataCU *pColCU = pColPic->getCU( m_uiCUAddr );
  
  Int iColPOC = pColCU->getSlice()->getPOC();
  UInt uiPartIdxCenter;
  xDeriveCenterIdx( eCUMode, uiPartIdx, uiPartIdxCenter );
  
  if (pColCU->isIntra(uiPartIdxCenter))
  {
    return false;
  }

  if( m_pcSlice->getRefPic( eRefPicList, iRefIdx )->getViewId() != m_pcSlice->getViewId() )
  {
    return false;
  }

  // Prefer a vector crossing us.  Prefer shortest.
  RefPicList eColRefPicList = REF_PIC_LIST_0;
  bool bFirstCrosses = false;
  Int  iFirstColDist = -1;
  for (Int l = 0; l < 2; l++)
  {
    bool bSaveIt = false;
    int iColRefIdx = pColCU->getCUMvField(RefPicList(l))->getRefIdx(uiPartIdxCenter);
    if (iColRefIdx < 0)
    {
      continue;
    }
    int iColRefPOC = pColCU->getSlice()->getRefPOC(RefPicList(l), iColRefIdx);
    if( pColCU->getSlice()->getRefViewId( RefPicList(l), iColRefIdx ) != pColCU->getSlice()->getViewId() )
    {
      continue;
    }
    int iColDist = abs(iColRefPOC - iColPOC);
    bool bCrosses = iColPOC < iCurrPOC ? iColRefPOC > iCurrPOC : iColRefPOC < iCurrPOC;
    if (iFirstColDist < 0)
    {
      bSaveIt = true;
    }
    else if (bCrosses && !bFirstCrosses)
    {
      bSaveIt = true;
    }
    else if (bCrosses == bFirstCrosses && l == eRefPicList)
    {
      bSaveIt = true;
    }
    
    if (bSaveIt)
    {
      bFirstCrosses = bCrosses;
      iFirstColDist = iColDist;
      eColRefPicList = RefPicList(l);
    }
  }

#if MERL_VSP_C0152 // Preventive
  if (pColCU->getCUMvField(eColRefPicList)->getRefIdx(uiPartIdxCenter) < 0) // NOT_VALID
    return false;
#endif
  
  // Scale the vector.
  Int iColRefPOC = pColCU->getSlice()->getRefPOC(eColRefPicList, pColCU->getCUMvField(eColRefPicList)->getRefIdx(uiPartIdxCenter));
  TComMv cColMv = pColCU->getCUMvField(eColRefPicList)->getMv(uiPartIdxCenter);
  
  Int iCurrRefPOC = m_pcSlice->getRefPic(eRefPicList, iRefIdx)->getPOC();
  Int iScale = xGetDistScaleFactor(iCurrPOC, iCurrRefPOC, iColPOC, iColRefPOC);
  if ( iScale == 4096 )
  {
    pcMv[0] = cColMv;
  }
  else
  {
    pcMv[0] = cColMv.scaleMv( iScale );
  }
  
  return true;
}

Void TComDataCU::compressMV()
{
  Int scaleFactor = 4 * AMVP_DECIMATION_FACTOR / m_unitSize;
  if (scaleFactor > 0)
  {
#if HHI_MPI
    m_acCUMvField[0].compress(m_pePredMode, m_puhInterDir, scaleFactor);
    m_acCUMvField[1].compress(m_pePredMode, m_puhInterDir, scaleFactor);
#else
    m_acCUMvField[0].compress(m_pePredMode, scaleFactor);
    m_acCUMvField[1].compress(m_pePredMode, scaleFactor);    
#endif
  }
}

UInt TComDataCU::getCoefScanIdx(UInt uiAbsPartIdx, UInt uiWidth, Bool bIsLuma, Bool bIsIntra)
{
  
  UInt uiCTXIdx;
  UInt uiScanIdx;
  UInt uiDirMode;

  if ( !bIsIntra ) 
  {
    uiScanIdx = SCAN_ZIGZAG;
    return uiScanIdx;
  }

  switch(uiWidth)
  {
    case  2: uiCTXIdx = 6; break;
    case  4: uiCTXIdx = 5; break;
    case  8: uiCTXIdx = 4; break;
    case 16: uiCTXIdx = 3; break;
    case 32: uiCTXIdx = 2; break;
    case 64: uiCTXIdx = 1; break;
    default: uiCTXIdx = 0; break;
  }

  if ( bIsLuma )
  {
    uiDirMode = getLumaIntraDir(uiAbsPartIdx);
#if HHI_DMM_WEDGE_INTRA || HHI_DMM_PRED_TEX
    mapDMMtoIntraMode( uiDirMode );
#endif

    uiScanIdx = SCAN_ZIGZAG;
    if (uiCTXIdx >3 && uiCTXIdx < 6) //if multiple scans supported for PU size
    {
      uiScanIdx = abs((Int) uiDirMode - VER_IDX) < 5 ? 1 : (abs((Int)uiDirMode - HOR_IDX) < 5 ? 2 : 0);
    }
  }
  else
  {
    uiDirMode = getChromaIntraDir(uiAbsPartIdx);
    if( uiDirMode == DM_CHROMA_IDX )
    {
      // get number of partitions in current CU
      UInt depth = getDepth(uiAbsPartIdx);
      UInt numParts = getPic()->getNumPartInCU() >> (2 * depth);
      
      // get luma mode from upper-left corner of current CU
      uiDirMode = getLumaIntraDir((uiAbsPartIdx/numParts)*numParts);
#if HHI_DMM_WEDGE_INTRA || HHI_DMM_PRED_TEX
    mapDMMtoIntraMode( uiDirMode );
#endif
    }
    uiScanIdx = SCAN_ZIGZAG;
    if (uiCTXIdx >4 && uiCTXIdx < 7) //if multiple scans supported for PU size
    {
      uiScanIdx = abs((Int) uiDirMode - VER_IDX) < 5 ? 1 : (abs((Int)uiDirMode - HOR_IDX) < 5 ? 2 : 0);
    }
  }

  return uiScanIdx;
}

Bool TComDataCU::useNonSquareTrans(UInt uiTrMode, Int absPartIdx)
{
  UInt minTuSize = ( 1 << ( getSlice()->getSPS()->getQuadtreeTULog2MinSize() + 1 ) );
  const UInt uiLog2TrSize = g_aucConvertToBit[ getSlice()->getSPS()->getMaxCUWidth() >> ( m_puhDepth[ absPartIdx ] + uiTrMode ) ] + 2;
  if ( uiTrMode && uiLog2TrSize < getSlice()->getSPS()->getQuadtreeTULog2MaxSize() && ( getWidth( absPartIdx ) > minTuSize ) &&
      ( m_pePartSize[absPartIdx] == SIZE_Nx2N || m_pePartSize[absPartIdx] == SIZE_2NxN || ( m_pePartSize[absPartIdx] >= SIZE_2NxnU && m_pePartSize[absPartIdx] <= SIZE_nRx2N ) ) )
  {
    return getSlice()->getSPS()->getUseNSQT();
  }
  else
  {
    return false;
  }
}

Void TComDataCU::getNSQTSize(Int trMode, Int absPartIdx, Int &trWidth, Int &trHeight)
{
  UInt minTuSize = ( 1 << getSlice()->getSPS()->getQuadtreeTULog2MinSize() );
  if ( useNonSquareTrans( trMode, absPartIdx ) && ( trWidth > minTuSize ) )
  {
    trWidth  = ( m_pePartSize[absPartIdx] == SIZE_Nx2N || m_pePartSize[absPartIdx] == SIZE_nLx2N || m_pePartSize[absPartIdx] == SIZE_nRx2N )? trWidth >> 1 : trWidth << 1;
    trHeight = ( m_pePartSize[absPartIdx] == SIZE_Nx2N || m_pePartSize[absPartIdx] == SIZE_nLx2N || m_pePartSize[absPartIdx] == SIZE_nRx2N )? trHeight << 1 : trHeight >> 1;
  }
}
Bool TComDataCU::useNonSquarePU(UInt absPartIdx)
{
  if ( ( m_pePartSize[absPartIdx] == SIZE_Nx2N ) || ( m_pePartSize[absPartIdx] == SIZE_2NxN ) || ( m_pePartSize[absPartIdx] >= SIZE_2NxnU && m_pePartSize[absPartIdx] <= SIZE_nRx2N ) )
  {
    return true;
  }
  else
  {
    return false;
  }
}

UInt TComDataCU::getInterTUSplitDirection( Int trWidth, Int trHeight, Int trLastWidth, Int trLastHeight )
{
  UInt interTUSplitDirection = 2;
  if ( ( trWidth == trLastWidth ) && ( trHeight < trLastHeight ) )
  {
    interTUSplitDirection = 0;
  }
  else if ( ( trWidth < trLastWidth ) && ( trHeight == trLastHeight ) )
  {
    interTUSplitDirection = 1;
  }    

  return interTUSplitDirection;
}

UInt TComDataCU::getNSAbsPartIdx ( UInt log2TrafoSize, UInt absPartIdx, UInt absTUPartIdx, UInt innerQuadIdx, UInt trMode )
{
  Int trWidth, trHeight, trLastWidth, trLastHeight;
  UInt lcuWidthInBaseUnits = getPic()->getNumPartInWidth();
  UInt nsTUWidthInBaseUnits, nsTUHeightInBaseUnits;
  UInt interTUSplitDirection;
  
  if( isIntra(absPartIdx) )
  {
    return absPartIdx;
  }

  trWidth = trHeight = ( 1 << log2TrafoSize );
  trLastWidth = trWidth << 1, trLastHeight = trHeight << 1;  

  getNSQTSize( trMode,     absPartIdx, trWidth,     trHeight );
  getNSQTSize( trMode - 1, absPartIdx, trLastWidth, trLastHeight );
  interTUSplitDirection = getInterTUSplitDirection ( trWidth, trHeight, trLastWidth, trLastHeight );

  nsTUWidthInBaseUnits  = trWidth / getPic()->getMinCUWidth();
  nsTUHeightInBaseUnits = trHeight / getPic()->getMinCUHeight();    

  if ( interTUSplitDirection != 2 )  
  {
    UInt uiNSTUBaseUnits = nsTUWidthInBaseUnits < nsTUHeightInBaseUnits ? nsTUWidthInBaseUnits : nsTUHeightInBaseUnits;
    absTUPartIdx = g_auiRasterToZscan[ g_auiZscanToRaster[absTUPartIdx] + innerQuadIdx * uiNSTUBaseUnits * lcuWidthInBaseUnits * ( 1 - interTUSplitDirection ) + innerQuadIdx * uiNSTUBaseUnits * interTUSplitDirection ];
  }
  else  
  {
    absTUPartIdx = g_auiRasterToZscan[ g_auiZscanToRaster[absTUPartIdx] + (innerQuadIdx & 0x01) * nsTUWidthInBaseUnits + ( ( innerQuadIdx >> 1 ) & 0x01 ) * nsTUHeightInBaseUnits * lcuWidthInBaseUnits ]; 
  }

  return absTUPartIdx;  
}

UInt TComDataCU::getNSAddrChroma( UInt uiLog2TrSizeC, UInt uiTrModeC, UInt uiQuadrant, UInt absTUPartIdx )
{  
  if( uiLog2TrSizeC != getSlice()->getSPS()->getQuadtreeTULog2MinSize() )
  {
    return absTUPartIdx;
  }

  UInt lcuWidthInBaseUnits = getPic()->getNumPartInWidth();
  Int trWidth  = ( 1 << ( uiLog2TrSizeC + 1 ) );
  Int trHeight = ( 1 << ( uiLog2TrSizeC + 1 ) );
  Int trLastWidth = trWidth << 1, trLastHeight = trHeight << 1;

  getNSQTSize ( uiTrModeC - 1, absTUPartIdx, trLastWidth, trLastHeight );
  getNSQTSize ( uiTrModeC,     absTUPartIdx, trWidth,     trHeight );

  UInt interTUSplitDirection = getInterTUSplitDirection ( trWidth, trHeight, trLastWidth, trLastHeight );
  UInt nsTUWidthInBaseUnits  = trWidth / getPic()->getMinCUWidth();
  UInt nsTUHeightInBaseUnits = trHeight / getPic()->getMinCUHeight();
  UInt firstTURasterIdx = 0, absTUPartIdxC = 0; 

  if(interTUSplitDirection != 2)
  {
    UInt uiNSTUBaseUnits = nsTUWidthInBaseUnits < nsTUHeightInBaseUnits ? 1 : lcuWidthInBaseUnits;
    firstTURasterIdx   = g_auiZscanToRaster[absTUPartIdx] - uiQuadrant * uiNSTUBaseUnits;
    absTUPartIdxC      = g_auiRasterToZscan[firstTURasterIdx] + uiQuadrant * nsTUWidthInBaseUnits * nsTUHeightInBaseUnits;
  }
  else
  {
    UInt uiNSTUBaseUnits = nsTUWidthInBaseUnits < nsTUHeightInBaseUnits ? 2 * lcuWidthInBaseUnits : 2;
    firstTURasterIdx   = g_auiZscanToRaster[absTUPartIdx] - ( ( uiQuadrant >> 1 ) & 0x01 ) * nsTUHeightInBaseUnits * lcuWidthInBaseUnits - ( uiQuadrant & 0x01 ) * nsTUWidthInBaseUnits;
    absTUPartIdxC      = g_auiRasterToZscan[firstTURasterIdx + uiQuadrant * uiNSTUBaseUnits];
  }

  return absTUPartIdxC;
}


UInt TComDataCU::getSCUAddr()
{ 
  return getPic()->getPicSym()->getInverseCUOrderMap(m_uiCUAddr)*(1<<(m_pcSlice->getSPS()->getMaxCUDepth()<<1))+m_uiAbsIdxInLCU; 
}

/** Set neighboring blocks availabilities for non-deblocked filtering 
 * \param numLCUInPicWidth number of LCUs in picture width
 * \param numLCUInPicHeight number of LCUs in picture height
 * \param numSUInLCUWidth number of SUs in LCU width
 * \param numSUInLCUHeight number of SUs in LCU height
 * \param picWidth picture width
 * \param picHeight picture height
 * \param bIndependentSliceBoundaryEnabled true for independent slice boundary enabled
 * \param bTopTileBoundary true means that top boundary coincides tile boundary
 * \param bDownTileBoundary true means that bottom boundary coincides tile boundary
 * \param bLeftTileBoundary true means that left boundary coincides tile boundary
 * \param bRightTileBoundary true means that right boundary coincides tile boundary
 * \param bIndependentTileBoundaryEnabled true for independent tile boundary enabled
 */
Void TComDataCU::setNDBFilterBlockBorderAvailability(UInt numLCUInPicWidth, UInt numLCUInPicHeight, UInt numSUInLCUWidth, UInt numSUInLCUHeight, UInt picWidth, UInt picHeight
                                                    ,Bool bIndependentSliceBoundaryEnabled
                                                    ,Bool bTopTileBoundary, Bool bDownTileBoundary, Bool bLeftTileBoundary, Bool bRightTileBoundary
                                                    ,Bool bIndependentTileBoundaryEnabled)
{
  UInt numSUInLCU = numSUInLCUWidth*numSUInLCUHeight;
  Int* pSliceIDMapLCU = m_piSliceSUMap;

  UInt uiLPelX, uiTPelY;
  UInt width, height;
  Bool bPicRBoundary, bPicBBoundary, bPicTBoundary, bPicLBoundary;
  Bool bLCURBoundary= false, bLCUBBoundary= false, bLCUTBoundary= false, bLCULBoundary= false;
  Bool* pbAvailBorder;
  Bool* pbAvail;
  UInt rTLSU, rBRSU, widthSU, heightSU;
  UInt zRefSU;
  Int* pRefID;
  Int* pRefMapLCU;
  UInt rTRefSU= 0, rBRefSU= 0, rLRefSU= 0, rRRefSU= 0;
  Int* pRRefMapLCU= NULL;
  Int* pLRefMapLCU= NULL;
  Int* pTRefMapLCU= NULL;
  Int* pBRefMapLCU= NULL;
  Int  sliceID;
  UInt numSGU = (UInt)m_vNDFBlock.size();

  for(Int i=0; i< numSGU; i++)
  {
    NDBFBlockInfo& rSGU = m_vNDFBlock[i];

    sliceID = rSGU.sliceID;
    uiLPelX = rSGU.posX;
    uiTPelY = rSGU.posY;
    width   = rSGU.width;
    height  = rSGU.height;

    rTLSU     = g_auiZscanToRaster[ rSGU.startSU ];
    rBRSU     = g_auiZscanToRaster[ rSGU.endSU   ];
    widthSU   = rSGU.widthSU;
    heightSU  = rSGU.heightSU;

    pbAvailBorder = rSGU.isBorderAvailable;

    bPicTBoundary= (uiTPelY == 0                       )?(true):(false);
    bPicLBoundary= (uiLPelX == 0                       )?(true):(false);
    bPicRBoundary= (!(uiLPelX+ width < picWidth )  )?(true):(false);
    bPicBBoundary= (!(uiTPelY + height < picHeight))?(true):(false);

    bLCULBoundary = (rTLSU % numSUInLCUWidth == 0)?(true):(false);
    bLCURBoundary = ( (rTLSU+ widthSU) % numSUInLCUWidth == 0)?(true):(false);
    bLCUTBoundary = ( (UInt)(rTLSU / numSUInLCUWidth)== 0)?(true):(false);
    bLCUBBoundary = ( (UInt)(rBRSU / numSUInLCUWidth) == (numSUInLCUHeight-1) )?(true):(false);

    //       SGU_L
    pbAvail = &(pbAvailBorder[SGU_L]);
    if(bPicLBoundary)
    {
      *pbAvail = false;
    }
    else if (!bIndependentSliceBoundaryEnabled)
    {
      *pbAvail = true;
    }
    else
    {
      //      bLCULBoundary = (rTLSU % uiNumSUInLCUWidth == 0)?(true):(false);
      if(bLCULBoundary)
      {
        rLRefSU     = rTLSU + numSUInLCUWidth -1;
        zRefSU      = g_auiRasterToZscan[rLRefSU];
        pRefMapLCU = pLRefMapLCU= (pSliceIDMapLCU - numSUInLCU);
      }
      else
      {
        zRefSU   = g_auiRasterToZscan[rTLSU - 1];
        pRefMapLCU  = pSliceIDMapLCU;
      }
      pRefID = pRefMapLCU + zRefSU;
      *pbAvail = (*pRefID == sliceID)?(true):(false);
    }

    //       SGU_R
    pbAvail = &(pbAvailBorder[SGU_R]);
    if(bPicRBoundary)
    {
      *pbAvail = false;
    }
    else if (!bIndependentSliceBoundaryEnabled)
    {
      *pbAvail = true;
    }
    else
    {
      //       bLCURBoundary = ( (rTLSU+ uiWidthSU) % uiNumSUInLCUWidth == 0)?(true):(false);
      if(bLCURBoundary)
      {
        rRRefSU      = rTLSU + widthSU - numSUInLCUWidth;
        zRefSU       = g_auiRasterToZscan[rRRefSU];
        pRefMapLCU  = pRRefMapLCU= (pSliceIDMapLCU + numSUInLCU);
      }
      else
      {
        zRefSU       = g_auiRasterToZscan[rTLSU + widthSU];
        pRefMapLCU  = pSliceIDMapLCU;
      }
      pRefID = pRefMapLCU + zRefSU;
      *pbAvail = (*pRefID == sliceID)?(true):(false);
    }

    //       SGU_T
    pbAvail = &(pbAvailBorder[SGU_T]);
    if(bPicTBoundary)
    {
      *pbAvail = false;
    }
    else if (!bIndependentSliceBoundaryEnabled)
    {
      *pbAvail = true;
    }
    else
    {
      //      bLCUTBoundary = ( (UInt)(rTLSU / uiNumSUInLCUWidth)== 0)?(true):(false);
      if(bLCUTBoundary)
      {
        rTRefSU      = numSUInLCU - (numSUInLCUWidth - rTLSU);
        zRefSU       = g_auiRasterToZscan[rTRefSU];
        pRefMapLCU  = pTRefMapLCU= (pSliceIDMapLCU - (numLCUInPicWidth*numSUInLCU));
      }
      else
      {
        zRefSU       = g_auiRasterToZscan[rTLSU - numSUInLCUWidth];
        pRefMapLCU  = pSliceIDMapLCU;
      }
      pRefID = pRefMapLCU + zRefSU;
      *pbAvail = (*pRefID == sliceID)?(true):(false);
    }

    //       SGU_B
    pbAvail = &(pbAvailBorder[SGU_B]);
    if(bPicBBoundary)
    {
      *pbAvail = false;
    }
    else if (!bIndependentSliceBoundaryEnabled)
    {
      *pbAvail = true;
    }
    else
    {
      //      bLCUBBoundary = ( (UInt)(rBRSU / uiNumSUInLCUWidth) == (uiNumSUInLCUHeight-1) )?(true):(false);
      if(bLCUBBoundary)
      {
        rBRefSU      = rTLSU % numSUInLCUWidth;
        zRefSU       = g_auiRasterToZscan[rBRefSU];
        pRefMapLCU  = pBRefMapLCU= (pSliceIDMapLCU + (numLCUInPicWidth*numSUInLCU));
      }
      else
      {
        zRefSU       = g_auiRasterToZscan[rTLSU + (heightSU*numSUInLCUWidth)];
        pRefMapLCU  = pSliceIDMapLCU;
      }
      pRefID = pRefMapLCU + zRefSU;
      *pbAvail = (*pRefID == sliceID)?(true):(false);
    }

    //       SGU_TL
    pbAvail = &(pbAvailBorder[SGU_TL]);
    if(bPicTBoundary || bPicLBoundary)
    {
      *pbAvail = false;
    }
    else if (!bIndependentSliceBoundaryEnabled)
    {
      *pbAvail = true;
    }
    else
    {
      if(bLCUTBoundary && bLCULBoundary)
      {
        zRefSU       = numSUInLCU -1;
        pRefMapLCU  = pSliceIDMapLCU - ( (numLCUInPicWidth+1)*numSUInLCU);
      }
      else if(bLCUTBoundary)
      {
        zRefSU       = g_auiRasterToZscan[ rTRefSU- 1];
        pRefMapLCU  = pTRefMapLCU;
      }
      else if(bLCULBoundary)
      {
        zRefSU       = g_auiRasterToZscan[ rLRefSU- numSUInLCUWidth ];
        pRefMapLCU  = pLRefMapLCU;
      }
      else //inside LCU
      {
        zRefSU       = g_auiRasterToZscan[ rTLSU - numSUInLCUWidth -1];
        pRefMapLCU  = pSliceIDMapLCU;
      }
      pRefID = pRefMapLCU + zRefSU;
      *pbAvail = (*pRefID == sliceID)?(true):(false);
    }

    //       SGU_TR
    pbAvail = &(pbAvailBorder[SGU_TR]);
    if(bPicTBoundary || bPicRBoundary)
    {
      *pbAvail = false;
    }
    else if (!bIndependentSliceBoundaryEnabled)
    {
      *pbAvail = true;
    }
    else
    {
      if(bLCUTBoundary && bLCURBoundary)
      {
        zRefSU      = g_auiRasterToZscan[numSUInLCU - numSUInLCUWidth];
        pRefMapLCU  = pSliceIDMapLCU - ( (numLCUInPicWidth-1)*numSUInLCU);        
      }
      else if(bLCUTBoundary)
      {
        zRefSU       = g_auiRasterToZscan[ rTRefSU+ widthSU];
        pRefMapLCU  = pTRefMapLCU;
      }
      else if(bLCURBoundary)
      {
        zRefSU       = g_auiRasterToZscan[ rRRefSU- numSUInLCUWidth ];
        pRefMapLCU  = pRRefMapLCU;
      }
      else //inside LCU
      {
        zRefSU       = g_auiRasterToZscan[ rTLSU - numSUInLCUWidth +widthSU];
        pRefMapLCU  = pSliceIDMapLCU;
      }
      pRefID = pRefMapLCU + zRefSU;
      *pbAvail = (*pRefID == sliceID)?(true):(false);
    }

    //       SGU_BL
    pbAvail = &(pbAvailBorder[SGU_BL]);
    if(bPicBBoundary || bPicLBoundary)
    {
      *pbAvail = false;
    }
    else if (!bIndependentSliceBoundaryEnabled)
    {
      *pbAvail = true;
    }
    else
    {
      if(bLCUBBoundary && bLCULBoundary)
      {
        zRefSU      = g_auiRasterToZscan[numSUInLCUWidth - 1];
        pRefMapLCU  = pSliceIDMapLCU + ( (numLCUInPicWidth-1)*numSUInLCU);        
      }
      else if(bLCUBBoundary)
      {
        zRefSU       = g_auiRasterToZscan[ rBRefSU - 1];
        pRefMapLCU  = pBRefMapLCU;
      }
      else if(bLCULBoundary)
      {
        zRefSU       = g_auiRasterToZscan[ rLRefSU+ heightSU*numSUInLCUWidth ];
        pRefMapLCU  = pLRefMapLCU;
      }
      else //inside LCU
      {
        zRefSU       = g_auiRasterToZscan[ rTLSU + heightSU*numSUInLCUWidth -1];
        pRefMapLCU  = pSliceIDMapLCU;
      }
      pRefID = pRefMapLCU + zRefSU;
      *pbAvail = (*pRefID == sliceID)?(true):(false);
    }

    //       SGU_BR
    pbAvail = &(pbAvailBorder[SGU_BR]);
    if(bPicBBoundary || bPicRBoundary)
    {
      *pbAvail = false;
    }
    else if (!bIndependentSliceBoundaryEnabled)
    {
      *pbAvail = true;
    }
    else
    {
      if(bLCUBBoundary && bLCURBoundary)
      {
        zRefSU = 0;
        pRefMapLCU = pSliceIDMapLCU+ ( (numLCUInPicWidth+1)*numSUInLCU);
      }
      else if(bLCUBBoundary)
      {
        zRefSU      = g_auiRasterToZscan[ rBRefSU + widthSU];
        pRefMapLCU = pBRefMapLCU;
      }
      else if(bLCURBoundary)
      {
        zRefSU      = g_auiRasterToZscan[ rRRefSU + (heightSU*numSUInLCUWidth)];
        pRefMapLCU = pRRefMapLCU;
      }
      else //inside LCU
      {
        zRefSU      = g_auiRasterToZscan[ rTLSU + (heightSU*numSUInLCUWidth)+ widthSU];
        pRefMapLCU = pSliceIDMapLCU;
      }
      pRefID = pRefMapLCU + zRefSU;
      *pbAvail = (*pRefID == sliceID)?(true):(false);
    }

    if(bIndependentTileBoundaryEnabled)
    {
      //left LCU boundary
      if(!bPicLBoundary && bLCULBoundary)
      {
        if(bLeftTileBoundary)
        {
          pbAvailBorder[SGU_L] = pbAvailBorder[SGU_TL] = pbAvailBorder[SGU_BL] = false;
        }
      }
      //right LCU boundary
      if(!bPicRBoundary && bLCURBoundary)
      {
        if(bRightTileBoundary)
        {
          pbAvailBorder[SGU_R] = pbAvailBorder[SGU_TR] = pbAvailBorder[SGU_BR] = false;
        }
      }
      //top LCU boundary
      if(!bPicTBoundary && bLCUTBoundary)
      {
        if(bTopTileBoundary)
        {
          pbAvailBorder[SGU_T] = pbAvailBorder[SGU_TL] = pbAvailBorder[SGU_TR] = false;
        }
      }
      //down LCU boundary
      if(!bPicBBoundary && bLCUBBoundary)
      {
        if(bDownTileBoundary)
        {
          pbAvailBorder[SGU_B] = pbAvailBorder[SGU_BL] = pbAvailBorder[SGU_BR] = false;
        }
      }
    }
    rSGU.allBordersAvailable = true;
    for(Int b=0; b< NUM_SGU_BORDER; b++)
    {
      if(pbAvailBorder[b] == false)
      {
        rSGU.allBordersAvailable = false;
        break;
      }
    }

  }
}

Void TComDataCU::getPosInPic( UInt uiAbsPartIndex, Int& riPosX, Int& riPosY )
{
  riPosX = g_auiRasterToPelX[g_auiZscanToRaster[uiAbsPartIndex]] + getCUPelX();
  riPosY = g_auiRasterToPelY[g_auiZscanToRaster[uiAbsPartIndex]] + getCUPelY();  
}

// -------------------------------------------------------------------------------------------------------------------
// public functions for depth model modes
// -------------------------------------------------------------------------------------------------------------------
#if HHI_DMM_WEDGE_INTRA
Void TComDataCU::setWedgeFullTabIdxSubParts( UInt uiTIdx, UInt uiAbsPartIdx, UInt uiDepth )
{
  UInt uiCurrPartNumb = m_pcPic->getNumPartInCU() >> (uiDepth << 1);

  for ( UInt ui = 0; ui < uiCurrPartNumb; ui++ )
  {
    m_puiWedgeFullTabIdx[uiAbsPartIdx+ui] = uiTIdx;
  }
}

Void TComDataCU::setWedgeFullDeltaDC1SubParts( Int iDC1, UInt uiAbsPartIdx, UInt uiDepth )
{
  UInt uiCurrPartNumb = m_pcPic->getNumPartInCU() >> (uiDepth << 1);

  for ( UInt ui = 0; ui < uiCurrPartNumb; ui++ )
  {
    m_piWedgeFullDeltaDC1[uiAbsPartIdx+ui] = iDC1;
  }
}

Void TComDataCU::setWedgeFullDeltaDC2SubParts( Int iDC2, UInt uiAbsPartIdx, UInt uiDepth )
{
  UInt uiCurrPartNumb = m_pcPic->getNumPartInCU() >> (uiDepth << 1);

  for ( UInt ui = 0; ui < uiCurrPartNumb; ui++ )
  {
    m_piWedgeFullDeltaDC2[uiAbsPartIdx+ui] = iDC2;
  }
}

Void TComDataCU::setWedgePredDirTabIdxSubParts( UInt uiTIdx, UInt uiAbsPartIdx, UInt uiDepth )
{
  UInt uiCurrPartNumb = m_pcPic->getNumPartInCU() >> (uiDepth << 1);

  for ( UInt ui = 0; ui < uiCurrPartNumb; ui++ )
  {
    m_puiWedgePredDirTabIdx[uiAbsPartIdx+ui] = uiTIdx;
  }
}

Void TComDataCU::setWedgePredDirDeltaDC1SubParts( Int iDC1, UInt uiAbsPartIdx, UInt uiDepth )
{
  UInt uiCurrPartNumb = m_pcPic->getNumPartInCU() >> (uiDepth << 1);

  for ( UInt ui = 0; ui < uiCurrPartNumb; ui++ )
  {
    m_piWedgePredDirDeltaDC1[uiAbsPartIdx+ui] = iDC1;
  }
}

Void TComDataCU::setWedgePredDirDeltaDC2SubParts( Int iDC2, UInt uiAbsPartIdx, UInt uiDepth )
{
  UInt uiCurrPartNumb = m_pcPic->getNumPartInCU() >> (uiDepth << 1);

  for ( UInt ui = 0; ui < uiCurrPartNumb; ui++ )
  {
    m_piWedgePredDirDeltaDC2[uiAbsPartIdx+ui] = iDC2;
  }
}

Void TComDataCU::setWedgePredDirDeltaEndSubParts( Int iDelta, UInt uiAbsPartIdx, UInt uiDepth )
{
  UInt uiCurrPartNumb = m_pcPic->getNumPartInCU() >> (uiDepth << 1);

  for ( UInt ui = 0; ui < uiCurrPartNumb; ui++ )
  {
    m_piWedgePredDirDeltaEnd[uiAbsPartIdx+ui] = iDelta;
  }
}
#endif
#if HHI_DMM_PRED_TEX
Void TComDataCU::setWedgePredTexTabIdxSubParts( UInt uiTIdx, UInt uiAbsPartIdx, UInt uiDepth )
{
  UInt uiCurrPartNumb = m_pcPic->getNumPartInCU() >> (uiDepth << 1);

  for ( UInt ui = 0; ui < uiCurrPartNumb; ui++ )
  {
    m_puiWedgePredTexTabIdx[uiAbsPartIdx+ui] = uiTIdx;
  }
}

#if LGE_DMM3_SIMP_C0044
Void TComDataCU::setWedgePredTexIntraTabIdxSubParts( UInt uiTIdx, UInt uiAbsPartIdx, UInt uiDepth )
{
  UInt uiCurrPartNumb = m_pcPic->getNumPartInCU() >> (uiDepth << 1);

  for ( UInt ui = 0; ui < uiCurrPartNumb; ui++ )
  {
    m_puiWedgePredTexIntraTabIdx[uiAbsPartIdx+ui] = uiTIdx;
  }
}
#endif

Void TComDataCU::setWedgePredTexDeltaDC1SubParts( Int iDC1, UInt uiAbsPartIdx, UInt uiDepth )
{
  UInt uiCurrPartNumb = m_pcPic->getNumPartInCU() >> (uiDepth << 1);

  for ( UInt ui = 0; ui < uiCurrPartNumb; ui++ )
  {
    m_piWedgePredTexDeltaDC1[uiAbsPartIdx+ui] = iDC1;
  }
}

Void TComDataCU::setWedgePredTexDeltaDC2SubParts( Int iDC2, UInt uiAbsPartIdx, UInt uiDepth )
{
  UInt uiCurrPartNumb = m_pcPic->getNumPartInCU() >> (uiDepth << 1);

  for ( UInt ui = 0; ui < uiCurrPartNumb; ui++ )
  {
    m_piWedgePredTexDeltaDC2[uiAbsPartIdx+ui] = iDC2;
  }
}

Void TComDataCU::setContourPredTexDeltaDC1SubParts( Int iDC1, UInt uiAbsPartIdx, UInt uiDepth )
{
  UInt uiCurrPartNumb = m_pcPic->getNumPartInCU() >> (uiDepth << 1);

  for ( UInt ui = 0; ui < uiCurrPartNumb; ui++ )
  {
    m_piContourPredTexDeltaDC1[uiAbsPartIdx+ui] = iDC1;
  }
}

Void TComDataCU::setContourPredTexDeltaDC2SubParts( Int iDC2, UInt uiAbsPartIdx, UInt uiDepth )
{
  UInt uiCurrPartNumb = m_pcPic->getNumPartInCU() >> (uiDepth << 1);

  for ( UInt ui = 0; ui < uiCurrPartNumb; ui++ )
  {
    m_piContourPredTexDeltaDC2[uiAbsPartIdx+ui] = iDC2;
  }
}
#endif

#if HHI_MPI
Void TComDataCU::setTextureModeDepthSubParts( Int iTextureModeDepth, UInt uiAbsPartIdx, UInt uiDepth )
{
  UInt uiCurrPartNumb = m_pcPic->getNumPartInCU() >> (uiDepth << 1);

  for (UInt ui = 0; ui < uiCurrPartNumb; ui++ )
  {
    m_piTextureModeDepth[uiAbsPartIdx + ui] = iTextureModeDepth;
  }
}

Void TComDataCU::copyTextureMotionDataFrom( TComDataCU* pcCU, UInt uiDepth, UInt uiAbsPartIdxSrc, UInt uiAbsPartIdxDst )
{
  assert( getSlice()->getIsDepth() && ! pcCU->getSlice()->getIsDepth() );
  UInt uiNumPartition = m_pcPic->getNumPartInCU() >> (uiDepth << 1);

  memcpy( m_pePredMode + uiAbsPartIdxDst,  pcCU->getPredictionMode() + uiAbsPartIdxSrc, sizeof( m_pePredMode[0] ) * uiNumPartition );
  memcpy( m_puhInterDir + uiAbsPartIdxDst, pcCU->getInterDir() + uiAbsPartIdxSrc,       sizeof( m_puhInterDir[0] ) * uiNumPartition );

#if MERL_VSP_C0152 && MTK_UNCONSTRAINED_MVI_B0083
  memcpy( m_piVSPIndex + uiAbsPartIdxDst,  pcCU->getVSPIndex() + uiAbsPartIdxSrc, sizeof( m_piVSPIndex[0] ) * uiNumPartition );
#endif

#if !MTK_UNCONSTRAINED_MVI_B0083
  memcpy( m_apiMVPIdx[0] + uiAbsPartIdxDst, pcCU->getMVPIdx(REF_PIC_LIST_0) + uiAbsPartIdxSrc, sizeof(*m_apiMVPIdx[0]) * uiNumPartition );
  memcpy( m_apiMVPIdx[1] + uiAbsPartIdxDst, pcCU->getMVPIdx(REF_PIC_LIST_1) + uiAbsPartIdxSrc, sizeof(*m_apiMVPIdx[0]) * uiNumPartition );
  memcpy( m_apiMVPNum[0] + uiAbsPartIdxDst, pcCU->getMVPNum(REF_PIC_LIST_0) + uiAbsPartIdxSrc, sizeof(*m_apiMVPNum[0]) * uiNumPartition );
  memcpy( m_apiMVPNum[1] + uiAbsPartIdxDst, pcCU->getMVPNum(REF_PIC_LIST_1) + uiAbsPartIdxSrc, sizeof(*m_apiMVPNum[0]) * uiNumPartition );
#endif

  pcCU->getCUMvField( REF_PIC_LIST_0 )->copyTo( &m_acCUMvField[0], -Int(uiAbsPartIdxSrc) + uiAbsPartIdxDst, uiAbsPartIdxSrc, uiNumPartition );
  pcCU->getCUMvField( REF_PIC_LIST_1 )->copyTo( &m_acCUMvField[1], -Int(uiAbsPartIdxSrc) + uiAbsPartIdxDst, uiAbsPartIdxSrc, uiNumPartition );

#if MTK_UNCONSTRAINED_MVI_B0083
  if( pcCU->getSlice()->getSliceType() == P_SLICE)
  {
    m_acCUMvField[0].setUndefinedMv( uiAbsPartIdxDst, uiNumPartition, m_pePredMode, m_puhInterDir,  0, 1);
    m_acCUMvField[1].setUndefinedMv( uiAbsPartIdxDst, uiNumPartition, m_pePredMode, m_puhInterDir, -1, 1);
  }
  else
  {
    m_acCUMvField[0].setUndefinedMv( uiAbsPartIdxDst, uiNumPartition, m_pePredMode, m_puhInterDir,  0, 3);
    m_acCUMvField[1].setUndefinedMv( uiAbsPartIdxDst, uiNumPartition, m_pePredMode, m_puhInterDir,  0, 3);
  }
#endif


#if HHI_FULL_PEL_DEPTH_MAP_MV_ACC
  m_acCUMvField[0].decreaseMvAccuracy( uiAbsPartIdxDst, uiNumPartition, 2 );
  m_acCUMvField[1].decreaseMvAccuracy( uiAbsPartIdxDst, uiNumPartition, 2 );
#endif
}
#endif

// -------------------------------------------------------------------------------------------------------------------
// public functions for Multi-view tools
// -------------------------------------------------------------------------------------------------------------------
#if H3D_IVMP
#if !H3D_NBDV
Int
TComDataCU::getPdmMergeCandidate( UInt uiPartIdx, Int* paiPdmRefIdx, TComMv* pacPdmMv )
{
  TComDepthMapGenerator* pcDepthMapGenerator = m_pcSlice->getSPS()->getDepthMapGenerator();
  ROFRS( pcDepthMapGenerator, 0 );
  return pcDepthMapGenerator->getPdmMergeCandidate( this, uiPartIdx, paiPdmRefIdx, pacPdmMv );
}


Bool
TComDataCU::getPdmMvPred( UInt uiPartIdx, RefPicList eRefPicList, Int iRefIdx, TComMv& rcMv, Bool bMerge )
{
  TComDepthMapGenerator* pcDepthMapGenerator = m_pcSlice->getSPS()->getDepthMapGenerator();
  ROFRS( pcDepthMapGenerator, false );
  return pcDepthMapGenerator->getPdmMvPred( this, uiPartIdx, eRefPicList, iRefIdx, rcMv, bMerge );
}
#else //H3D_NBDV
Bool
TComDataCU::getUnifiedMvPredCan(UInt uiPartIdx, RefPicList eRefPicList, Int iRefIdx, Int* paiPdmRefIdx, TComMv* pacPdmMv, DisInfo* pDInfo, Int* iPdm, Bool bMerge )
{
  TComDepthMapGenerator* pcDepthMapGenerator = m_pcSlice->getSPS()->getDepthMapGenerator();
  ROFRS( pcDepthMapGenerator, false );
  if (pDInfo->iN > 0 && pcDepthMapGenerator->getPdmCandidate(this, uiPartIdx, eRefPicList, iRefIdx, paiPdmRefIdx, pacPdmMv, pDInfo, iPdm, bMerge))
    return true;
  return false;
}
#endif //H3D_NBDV

Bool      
TComDataCU::getIViewOrgDepthMvPred( UInt uiPartIdx, RefPicList eRefPicList, Int iRefIdx, TComMv& rcMv )
{
  TComDepthMapGenerator* pcDepthMapGenerator = m_pcSlice->getSPS()->getDepthMapGenerator();
  ROFRS( pcDepthMapGenerator, false );
  return pcDepthMapGenerator->getIViewOrgDepthMvPred( this, uiPartIdx, eRefPicList, iRefIdx, rcMv );
}
#endif

#if H3D_IVRP
Bool
TComDataCU::getResidualSamples( UInt uiPartIdx, Bool bRecon, TComYuv* pcYuv )
{
  TComResidualGenerator*  pcResidualGenerator = m_pcSlice->getSPS()->getResidualGenerator();
  ROFRS( pcResidualGenerator, false );
#if H3D_NBDV
  DisInfo cDisInfo;
  cDisInfo.iN = 0;
  PartSize m_peSaved =  getPartitionSize( 0 );
  m_pePartSize[0] =  SIZE_2Nx2N;
#if FCO_DVP_REFINE_C0132_C0170
  if(getPic()->getDepthCoded() )
  {
    TComPic*      pcCodedDepthMap = getPic()->getRecDepthMap();
    TComMv        cColMv;

    cColMv.setZero();
    estimateDVFromDM(0, pcCodedDepthMap, 0, &cColMv, false);

    cDisInfo.iN = 1;
    cDisInfo.m_acMvCand[0].setHor( cColMv.getHor() );
    cDisInfo.m_acMvCand[0].setVer( cColMv.getVer() );
    cDisInfo.m_aVIdxCan[0] = 0;

  }
  else
#endif
  getDisMvpCandNBDV( 0, 0,  &cDisInfo, false );
  if( cDisInfo.iN == 0)
  {
    m_pePartSize[0] = m_peSaved;
    return false;
  }
  else
  {
    Bool bAvailable = pcResidualGenerator->getResidualSamples( this, uiPartIdx, pcYuv, cDisInfo.m_acMvCand[0], bRecon );       
    m_pePartSize[0] = m_peSaved;
    return bAvailable;
  }
#else
  return pcResidualGenerator->getResidualSamples( this, uiPartIdx, pcYuv, bRecon ); 
#endif
}
#endif

#if LGE_EDGE_INTRA_A0070
Void TComDataCU::reconPartition( UInt uiAbsPartIdx, UInt uiDepth, Bool bLeft, UChar ucStartPos, UChar ucNumEdge, UChar* pucEdgeCode, Bool* pbRegion )
{
  Int iWidth;
  Int iHeight;
  if( uiDepth == 0 )
  {
    iWidth = 64;
    iHeight = 64;
  }
  else if( uiDepth == 1 )
  {
    iWidth = 32;
    iHeight = 32;
  }
  else if( uiDepth == 2 )
  {
    iWidth = 16;
    iHeight = 16;
  }
  else if( uiDepth == 3 )
  {
    iWidth = 8;
    iHeight = 8;
  }
  else // uiDepth == 4
  {
    iWidth = 4;
    iHeight = 4;
  }

  Int iPtr = 0;
  Int iX, iY;
  Int iDir = -1;
  Int iDiffX = 0, iDiffY = 0;

  // 1. Edge Code -> Vert & Horz Edges
  Bool*  pbEdge = (Bool*) xMalloc( Bool, 4 * iWidth * iHeight );

  for( UInt ui = 0; ui < 4 * iWidth * iHeight; ui++ )
    pbEdge  [ ui ] = false;

  // Direction : left(0), right(1), top(2), bottom(3), left-top(4), right-top(5), left-bottom(6), right-bottom(7)
  // Code      : 0deg(0), 45deg(1), -45deg(2), 90deg(3), -90deg(4), 135deg(5), -135deg(6)
  const UChar tableDir[8][7] = { { 0, 6, 4, 3, 2, 7, 5 },
  { 1, 5, 7, 2, 3, 4, 6 },
  { 2, 4, 5, 0, 1, 6, 7 },
  { 3, 7, 6, 1, 0, 5, 4 },
  { 4, 0, 2, 6, 5, 3, 1 },
  { 5, 2, 1, 4, 7, 0, 3 },
  { 6, 3, 0, 7, 4, 1, 2 },
  { 7, 1, 3, 5, 6, 2, 0 }};

  UChar ucCode = pucEdgeCode[iPtr++];

  if( !bLeft )
  {
    iX = ucStartPos;
    iY = 0;

    switch(ucCode)
    {
    case 0: // bottom
      iDir = 3;
      if(iX > 0) pbEdge[ 2 * (iX - 1) + 1 + iY * 4 * iWidth ] = true;
      break;
    case 2: // left-bottom
      iDir = 6;
      if(iX > 0) pbEdge[ 2 * (iX - 1) + 1 + iY * 4 * iWidth ] = true;
      break;
    case 1: // right-bottom
      iDir = 7;
      if(iX > 0) pbEdge[ 2 * (iX - 1) + 1 + iY * 4 * iWidth ] = true;
      if(iY < iHeight - 1) pbEdge[ 2 * iX + (2 * (iY + 0) + 1) * 2 * iWidth ] = true;
      break;
    case 4: // left
      iDir = 0;
      assert(false);
      break;
    case 3: // right
      iDir = 1;
      if(iX > 0) pbEdge[ 2 * (iX - 1) + 1 + iY * 4 * iWidth ] = true;
      if(iY < iHeight - 1) pbEdge[ 2 * iX + (2 * (iY + 0) + 1) * 2 * iWidth ] = true;
      break;
    }
  }
  else
  {
    iX = 0;
    iY = ucStartPos;

    switch(ucCode)
    {
    case 0: // right
      iDir = 1;
      if(iY < iHeight - 1) pbEdge[ 2 * iX + (2 * (iY + 0) + 1) * 2 * iWidth ] = true;
      break;
    case 1: // right-top
      iDir = 5;
      if(iY < iHeight - 1) pbEdge[ 2 * iX + (2 * (iY + 0) + 1) * 2 * iWidth ] = true;
      if(iX < iWidth - 1) pbEdge[ 2 * (iX + 0) + 1 + iY * 4 * iWidth ] = true;
      break;
    case 2: // right-bottom
      iDir = 7;
      if(iY < iHeight - 1) pbEdge[ 2 * iX + (2 * (iY + 0) + 1) * 2 * iWidth ] = true;
      break;
    case 3: // top
      iDir = 2;
      if(iY < iHeight - 1) pbEdge[ 2 * iX + (2 * (iY + 0) + 1) * 2 * iWidth ] = true;
      if(iX < iWidth - 1) pbEdge[ 2 * (iX + 0) + 1 + iY * 4 * iWidth ] = true;
      break;
    case 4: // bottom
      iDir = 3;
      assert(false);
      break;
    }
  }

  switch( iDir )
  {
  case 0: // left
    iDiffX = -1;
    iDiffY = 0;
    break;
  case 1: // right
    iDiffX = +1;
    iDiffY = 0;
    break;
  case 2: // top
    iDiffX = 0;
    iDiffY = -1;
    break;
  case 3: // bottom
    iDiffX = 0;
    iDiffY = +1;
    break;
  case 4: // left-top
    iDiffX = -1;
    iDiffY = -1;
    break;
  case 5: // right-top
    iDiffX = +1;
    iDiffY = -1;
    break;
  case 6: // left-bottom
    iDiffX = -1;
    iDiffY = +1;
    break;
  case 7: // right-bottom
    iDiffX = +1;
    iDiffY = +1;
    break;
  }

  iX += iDiffX;
  iY += iDiffY;

  while( iPtr < ucNumEdge )
  {
    ucCode = pucEdgeCode[iPtr++];

    Int iNewDir = tableDir[iDir][ucCode];

    switch( iNewDir )
    {
    case 0: // left
      iDiffX = -1;
      iDiffY = 0;
      break;
    case 1: // right
      iDiffX = +1;
      iDiffY = 0;
      break;
    case 2: // top
      iDiffX = 0;
      iDiffY = -1;
      break;
    case 3: // bottom
      iDiffX = 0;
      iDiffY = +1;
      break;
    case 4: // left-top
      iDiffX = -1;
      iDiffY = -1;
      break;
    case 5: // right-top
      iDiffX = +1;
      iDiffY = -1;
      break;
    case 6: // left-bottom
      iDiffX = -1;
      iDiffY = +1;
      break;
    case 7: // right-bottom
      iDiffX = +1;
      iDiffY = +1;
      break;
    }

    switch( iDir )
    {
    case 0: // left
      switch( ucCode )
      {
      case 0:
      case 2:
        if(iY > 0) pbEdge[ 2 * iX + (2 * (iY - 1) + 1) * 2 * iWidth ] = true;
        break;
      case 1:
      case 3:
        if(iY > 0) pbEdge[ 2 * iX + (2 * (iY - 1) + 1) * 2 * iWidth ] = true;
        if(iX > 0) pbEdge[ 2 * (iX - 1) + 1 + iY * 4 * iWidth ] = true;
        break;
      case 4:
      case 6:
        // no
        break;
      case 5:
        if(iY > 0) pbEdge[ 2 * iX + (2 * (iY - 1) + 1) * 2 * iWidth ] = true;
        if(iY < iHeight - 1) pbEdge[ 2 * iX + (2 * (iY + 0) + 1) * 2 * iWidth ] = true;
        if(iX > 0) pbEdge[ 2 * (iX - 1) + 1 + iY * 4 * iWidth ] = true;
        break;
      }
      break;
    case 1: // right
      switch( ucCode )
      {
      case 0:
      case 2:
        if(iY < iHeight - 1) pbEdge[ 2 * iX + (2 * (iY + 0) + 1) * 2 * iWidth ] = true;
        break;
      case 1:
      case 3:
        if(iY < iHeight - 1) pbEdge[ 2 * iX + (2 * (iY + 0) + 1) * 2 * iWidth ] = true;
        if(iX < iWidth - 1) pbEdge[ 2 * (iX + 0) + 1 + iY * 4 * iWidth ] = true;
        break;
      case 4:
      case 6:
        // no
        break;
      case 5:
        if(iY > 0) pbEdge[ 2 * iX + (2 * (iY - 1) + 1) * 2 * iWidth ] = true;
        if(iY < iHeight - 1) pbEdge[ 2 * iX + (2 * (iY + 0) + 1) * 2 * iWidth ] = true;
        if(iX < iWidth - 1) pbEdge[ 2 * (iX + 0) + 1 + iY * 4 * iWidth ] = true;
        break;
      }
      break;
    case 2: // top
      switch( ucCode )
      {
      case 0:
      case 2:
        if(iX < iWidth - 1) pbEdge[ 2 * (iX + 0) + 1 + iY * 4 * iWidth ] = true;
        break;
      case 1:
      case 3:
        if(iY > 0) pbEdge[ 2 * iX + (2 * (iY - 1) + 1) * 2 * iWidth ] = true;
        if(iX < iWidth - 1) pbEdge[ 2 * (iX + 0) + 1 + iY * 4 * iWidth ] = true;
        break;
      case 4:
      case 6:
        // no
        break;
      case 5:
        if(iY > 0) pbEdge[ 2 * iX + (2 * (iY - 1) + 1) * 2 * iWidth ] = true;
        if(iX > 0) pbEdge[ 2 * (iX - 1) + 1 + iY * 4 * iWidth ] = true;
        if(iX < iWidth - 1) pbEdge[ 2 * (iX + 0) + 1 + iY * 4 * iWidth ] = true;
        break;
      }
      break;
    case 3: // bottom
      switch( ucCode )
      {
      case 0:
      case 2:
        if(iX > 0) pbEdge[ 2 * (iX - 1) + 1 + iY * 4 * iWidth ] = true;
        break;
      case 1:
      case 3:
        if(iX > 0) pbEdge[ 2 * (iX - 1) + 1 + iY * 4 * iWidth ] = true;
        if(iY < iHeight - 1) pbEdge[ 2 * iX + (2 * (iY + 0) + 1) * 2 * iWidth ] = true;
        break;
      case 4:
      case 6:
        // no
        break;
      case 5:
        if(iX > 0) pbEdge[ 2 * (iX - 1) + 1 + iY * 4 * iWidth ] = true;
        if(iX < iWidth - 1) pbEdge[ 2 * (iX + 0) + 1 + iY * 4 * iWidth ] = true;
        if(iY < iHeight - 1) pbEdge[ 2 * iX + (2 * (iY + 0) + 1) * 2 * iWidth ] = true;
        break;
      }
      break;
    case 4: // left-top
      switch( ucCode )
      {
      case 0:
      case 1:
        if(iY > 0) pbEdge[ 2 * iX + (2 * (iY - 1) + 1) * 2 * iWidth ] = true;
        if(iX < iWidth - 1) pbEdge[ 2 * (iX + 0) + 1 + iY * 4 * iWidth ] = true;
        break;
      case 2:
      case 4:
        if(iX < iWidth - 1) pbEdge[ 2 * (iX + 0) + 1 + iY * 4 * iWidth ] = true;
        break;
      case 3:
      case 5:
        if(iY > 0) pbEdge[ 2 * iX + (2 * (iY - 1) + 1) * 2 * iWidth ] = true;
        if(iX > 0) pbEdge[ 2 * (iX - 1) + 1 + iY * 4 * iWidth ] = true;
        if(iX < iWidth - 1) pbEdge[ 2 * (iX + 0) + 1 + iY * 4 * iWidth ] = true;
        break;
      case 6:
        // no
        break;
      }
      break;
    case 5: // right-top
      switch( ucCode )
      {
      case 0:
      case 1:
        if(iY < iHeight - 1) pbEdge[ 2 * iX + (2 * (iY + 0) + 1) * 2 * iWidth ] = true;
        if(iX < iWidth - 1) pbEdge[ 2 * (iX + 0) + 1 + iY * 4 * iWidth ] = true;
        break;
      case 2:
      case 4:
        if(iY < iHeight - 1) pbEdge[ 2 * iX + (2 * (iY + 0) + 1) * 2 * iWidth ] = true;
        break;
      case 3:
      case 5:
        if(iY > 0) pbEdge[ 2 * iX + (2 * (iY - 1) + 1) * 2 * iWidth ] = true;
        if(iY < iHeight - 1) pbEdge[ 2 * iX + (2 * (iY + 0) + 1) * 2 * iWidth ] = true;
        if(iX < iWidth - 1) pbEdge[ 2 * (iX + 0) + 1 + iY * 4 * iWidth ] = true;
        break;
      case 6:
        // no
        break;
      }
      break;
    case 6: // left-bottom
      switch( ucCode )
      {
      case 0:
      case 1:
        if(iY > 0) pbEdge[ 2 * iX + (2 * (iY - 1) + 1) * 2 * iWidth ] = true;
        if(iX > 0) pbEdge[ 2 * (iX - 1) + 1 + iY * 4 * iWidth ] = true;
        break;
      case 2:
      case 4:
        if(iY > 0) pbEdge[ 2 * iX + (2 * (iY - 1) + 1) * 2 * iWidth ] = true;
        break;
      case 3:
      case 5:
        if(iY > 0) pbEdge[ 2 * iX + (2 * (iY - 1) + 1) * 2 * iWidth ] = true;
        if(iX > 0) pbEdge[ 2 * (iX - 1) + 1 + iY * 4 * iWidth ] = true;
        if(iY < iHeight - 1) pbEdge[ 2 * iX + (2 * (iY + 0) + 1) * 2 * iWidth ] = true;
        break;
      case 6:
        // no
        break;
      }
      break;
    case 7: // right-bottom
      switch( ucCode )
      {
      case 0:
      case 1:
        if(iY < iHeight - 1) pbEdge[ 2 * iX + (2 * (iY + 0) + 1) * 2 * iWidth ] = true;
        if(iX > 0) pbEdge[ 2 * (iX - 1) + 1 + iY * 4 * iWidth ] = true;
        break;
      case 2:
      case 4:
        if(iX > 0) pbEdge[ 2 * (iX - 1) + 1 + iY * 4 * iWidth ] = true;
        break;
      case 3:
      case 5:
        if(iX > 0) pbEdge[ 2 * (iX - 1) + 1 + iY * 4 * iWidth ] = true;
        if(iX < iWidth - 1) pbEdge[ 2 * (iX + 0) + 1 + iY * 4 * iWidth ] = true;
        if(iY < iHeight - 1) pbEdge[ 2 * iX + (2 * (iY + 0) + 1) * 2 * iWidth ] = true;
        break;
      case 6:
        // no
        break;
      }
      break;
    }

    assert( iX >= 0 && iX <= iWidth );
    assert( iY >= 0 && iY <= iHeight );

    iX += iDiffX;
    iY += iDiffY;
    iDir = iNewDir;
  }

  // finalize edge chain
  if( iX == iWidth-1 )
  {
    if( iY == 0 )
    {
      if( iDir == 1 )
      {
        pbEdge[ 2 * iX + (2 * iY + 1) * 2 * iWidth ] = true;
      }
      else if( iDir == 5 )
      {
        pbEdge[ 2 * iX + (2 * iY + 1) * 2 * iWidth ] = true;
      }
      else
      {
        assert(false);
      }
    }
    else if( iY == iHeight-1 )
    {
      if( iDir == 3 )
      {
        pbEdge[ 2 * iX - 1 + 2 * iY * 2 * iWidth ] = true;
      }
      else if( iDir == 7 )
      {
        pbEdge[ 2 * iX - 1 + 2 * iY * 2 * iWidth ] = true;
      }
      else
      {
        assert(false);
      }
    }
    else
    {
      if( iDir == 1 )
      {
        pbEdge[ 2 * iX + (2 * iY + 1) * 2 * iWidth ] = true;
      }
      else if( iDir == 3 )
      {
        pbEdge[ 2 * iX - 1 + 2 * iY * 2 * iWidth ] = true;
        pbEdge[ 2 * iX + (2 * iY + 1) * 2 * iWidth ] = true;
      }
      else if( iDir == 5 )
      {
        pbEdge[ 2 * iX + (2 * iY + 1) * 2 * iWidth ] = true;
      }
      else if( iDir == 7 )
      {
        pbEdge[ 2 * iX - 1 + 2 * iY * 2 * iWidth ] = true;
        pbEdge[ 2 * iX + (2 * iY + 1) * 2 * iWidth ] = true;
      }
      else
      {
        assert(false);
      }
    }
  }
  else if( iX == 0 )
  {
    if( iY == 0 )
    {
      if( iDir == 2 )
      {
        pbEdge[ 2 * iX + 1 + 2 * iY * 2 * iWidth ] = true;
      }
      else if( iDir == 4 )
      {
        pbEdge[ 2 * iX + 1 + 2 * iY * 2 * iWidth ] = true;
      }
      else
      {
        assert(false);
      }
    }
    else if( iY == iHeight-1 )
    {
      if( iDir == 0 )
      {
        pbEdge[ 2 * iX + (2 * iY - 1) * 2 * iWidth ] = true;
      }
      else if( iDir == 6 )
      {
        pbEdge[ 2 * iX + (2 * iY - 1) * 2 * iWidth ] = true;
      }
      else
      {
        assert(false);
      }
    }
    else
    {
      if( iDir == 0 )
      {
        pbEdge[ 2 * iX + (2 * iY - 1) * 2 * iWidth ] = true;
      }
      else if( iDir == 2 )
      {
        pbEdge[ 2 * iX + 1 + 2 * iY * 2 * iWidth ] = true;
        pbEdge[ 2 * iX + (2 * iY - 1) * 2 * iWidth ] = true;
      }
      else if( iDir == 4 )
      {
        pbEdge[ 2 * iX + 1 + 2 * iY * 2 * iWidth ] = true;
        pbEdge[ 2 * iX + (2 * iY - 1) * 2 * iWidth ] = true;
      }
      else if( iDir == 6 )
      {
        pbEdge[ 2 * iX + (2 * iY - 1) * 2 * iWidth ] = true;
      }
      else
      {
        assert(false);
      }
    }
  }
  else if( iY == 0 )
  {
    if( iDir == 1 )
    {
      pbEdge[ 2 * iX + (2 * iY + 1) * 2 * iWidth ] = true;
      pbEdge[ 2 * iX + 1 + 2 * iY * 2 * iWidth ] = true;
    }
    else if( iDir == 2 )
    {
      pbEdge[ 2 * iX + 1 + 2 * iY * 2 * iWidth ] = true;
    }
    else if( iDir == 4 )
    {
      pbEdge[ 2 * iX + 1 + 2 * iY * 2 * iWidth ] = true;
    }
    else if( iDir == 5 )
    {
      pbEdge[ 2 * iX + (2 * iY + 1) * 2 * iWidth ] = true;
      pbEdge[ 2 * iX + 1 + 2 * iY * 2 * iWidth ] = true;
    }
    else
    {
      assert(false);
    }
  }
  else if( iY == iHeight-1 )
  {
    if( iDir == 0 )
    {
      pbEdge[ 2 * iX + (2 * iY - 1) * 2 * iWidth ] = true;
      pbEdge[ 2 * iX - 1 + 2 * iY * 2 * iWidth ] = true;
    }
    else if( iDir == 3 )
    {
      pbEdge[ 2 * iX - 1 + 2 * iY * 2 * iWidth ] = true;
    }
    else if( iDir == 6 )
    {
      pbEdge[ 2 * iX + (2 * iY - 1) * 2 * iWidth ] = true;
      pbEdge[ 2 * iX - 1 + 2 * iY * 2 * iWidth ] = true;
    }
    else if( iDir == 7 )
    {
      pbEdge[ 2 * iX - 1 + 2 * iY * 2 * iWidth ] = true;
    }
    else
    {
      assert(false);
    }
  }
  else
  {
    printf("xPredIntraEdge: wrong termination\n");
    assert(false);
  }

  // Reconstruct Region from Chain Code
  Bool* pbVisit  = (Bool*) xMalloc( Bool, iWidth * iHeight );
  Int*  piStack  = (Int* ) xMalloc( Int,  iWidth * iHeight );

  for( UInt ui = 0; ui < iWidth * iHeight; ui++ )
  {
    pbRegion[ ui ] = true; // fill it as region 1 (we'll discover region 0 next)
    pbVisit [ ui ] = false;
  }

  iPtr = 0;
  piStack[iPtr++] = (0 << 8) | (0);
  pbRegion[ 0 ] = false;

  while(iPtr > 0)
  {
    Int iTmp = piStack[--iPtr];
    Int iX1, iY1;
    iX1 = iTmp & 0xff;
    iY1 = (iTmp >> 8) & 0xff;

    pbVisit[ iX1 + iY1 * iWidth ] = true;

    assert( iX1 >= 0 && iX1 < iWidth );
    assert( iY1 >= 0 && iY1 < iHeight );

    if( iX1 > 0 && !pbEdge[ 2 * iX1 - 1 + 4 * iY1 * iWidth ] && !pbVisit[ iX1 - 1 + iY1 * iWidth ] )
    {
      piStack[iPtr++] = (iY1 << 8) | (iX1 - 1);
      pbRegion[ iX1 - 1 + iY1 * iWidth ] = false;
    }
    if( iX1 < iWidth - 1 && !pbEdge[ 2 * iX1 + 1 + 4 * iY1 * iWidth ] && !pbVisit[ iX1 + 1 + iY1 * iWidth ] )
    {
      piStack[iPtr++] = (iY1 << 8) | (iX1 + 1);
      pbRegion[ iX1 + 1 + iY1 * iWidth ] = false;
    }
    if( iY1 > 0 && !pbEdge[ 2 * iX1 + 2 * (2 * iY1 - 1) * iWidth ] && !pbVisit[ iX1 + (iY1 - 1) * iWidth ] )
    {
      piStack[iPtr++] = ((iY1 - 1) << 8) | iX1;
      pbRegion[ iX1 + (iY1 - 1) * iWidth ] = false;
    }
    if( iY1 < iHeight - 1 && !pbEdge[ 2 * iX1 + 2 * (2 * iY1 + 1) * iWidth ] && !pbVisit[ iX1 + (iY1 + 1) * iWidth ] )
    {
      piStack[iPtr++] = ((iY1 + 1) << 8) | iX1;
      pbRegion[ iX1 + (iY1 + 1) * iWidth ] = false;
    }
  }

  xFree( pbEdge );
  xFree( pbVisit );
  xFree( piStack );
}

#endif

//! \}
