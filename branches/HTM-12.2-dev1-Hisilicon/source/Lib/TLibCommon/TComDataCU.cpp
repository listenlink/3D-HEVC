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

/** \file     TComDataCU.cpp
    \brief    CU data structure
    \todo     not all entities are documented
*/

#include "TComDataCU.h"
#include "TComPic.h"

//! \ingroup TLibCommon
//! \{

#if ADAPTIVE_QP_SELECTION
Int * TComDataCU::m_pcGlbArlCoeffY  = NULL;
Int * TComDataCU::m_pcGlbArlCoeffCb = NULL;
Int * TComDataCU::m_pcGlbArlCoeffCr = NULL;
#endif

// ====================================================================================================================
// Constructor / destructor / create / destroy
// ====================================================================================================================

TComDataCU::TComDataCU()
{
  m_pcPic              = NULL;
  m_pcSlice            = NULL;
  m_puhDepth           = NULL;
  
  m_skipFlag           = NULL;
#if H_3D_SINGLE_DEPTH
  m_singleDepthFlag     = NULL;
  m_apSingleDepthValue  = NULL;
#endif  
  m_pePartSize         = NULL;
  m_pePredMode         = NULL;
  m_CUTransquantBypass = NULL;
  m_puhWidth           = NULL;
  m_puhHeight          = NULL;
  m_phQP               = NULL;
  m_pbMergeFlag        = NULL;
  m_puhMergeIndex      = NULL;
  m_puhLumaIntraDir    = NULL;
  m_puhChromaIntraDir  = NULL;
  m_puhInterDir        = NULL;
  m_puhTrIdx           = NULL;
  m_puhTransformSkip[0] = NULL;
  m_puhTransformSkip[1] = NULL;
  m_puhTransformSkip[2] = NULL;
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

#if H_3D_DIM
  for( Int i = 0; i < DIM_NUM_TYPE; i++ )
  {
    m_dimDeltaDC[i][0] = NULL; 
    m_dimDeltaDC[i][1] = NULL;
  }
#if H_3D_DIM_DMM
  for( Int i = 0; i < DMM_NUM_TYPE; i++ )
  {
    m_dmmWedgeTabIdx[i] = NULL;
  }
#endif
#if H_3D_DIM_SDC
  m_pbSDCFlag             = NULL;
  m_apSegmentDCOffset[0]  = NULL;
  m_apSegmentDCOffset[1]  = NULL;
#endif
#endif
  m_bDecSubCu          = false;
  m_sliceStartCU        = 0;
  m_sliceSegmentStartCU = 0;
#if H_3D_NBDV
  m_pDvInfo              = NULL;
#endif
#if H_3D_VSP
  m_piVSPFlag            = NULL;
#endif
#if H_3D_SPIVMP
  m_pbSPIVMPFlag         = NULL;
#endif
#if H_3D_ARP
  m_puhARPW              = NULL;
#endif
#if H_3D_IC
  m_pbICFlag             = NULL;
#endif
#if H_3D_INTER_SDC
#endif
#if H_3D_DBBP
  m_pbDBBPFlag         = NULL;
#endif

#if H_3D_DDD
  m_pucDisparityDerivedDepth = NULL;
  m_pbUseDDD = NULL;
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
    m_puhWidth           = (UChar*    )xMalloc(UChar,    uiNumPartition);
    m_puhHeight          = (UChar*    )xMalloc(UChar,    uiNumPartition);

    m_skipFlag           = new Bool[ uiNumPartition ];
#if H_3D_SINGLE_DEPTH
    m_singleDepthFlag     = new Bool[ uiNumPartition ];
    m_apSingleDepthValue  = (Pel*)xMalloc(Pel, uiNumPartition);
#endif
    m_pePartSize         = new Char[ uiNumPartition ];
    memset( m_pePartSize, SIZE_NONE,uiNumPartition * sizeof( *m_pePartSize ) );
    m_pePredMode         = new Char[ uiNumPartition ];
    m_CUTransquantBypass = new Bool[ uiNumPartition ];
    m_pbMergeFlag        = (Bool*  )xMalloc(Bool,   uiNumPartition);
    m_puhMergeIndex      = (UChar* )xMalloc(UChar,  uiNumPartition);
#if H_3D_VSP
    m_piVSPFlag          = (Char*  )xMalloc(Char,   uiNumPartition);
#endif
#if H_3D_SPIVMP
    m_pbSPIVMPFlag       = (Bool*  )xMalloc(Bool,   uiNumPartition);
#endif
    m_puhLumaIntraDir    = (UChar* )xMalloc(UChar,  uiNumPartition);
    m_puhChromaIntraDir  = (UChar* )xMalloc(UChar,  uiNumPartition);
    m_puhInterDir        = (UChar* )xMalloc(UChar,  uiNumPartition);
    
    m_puhTrIdx           = (UChar* )xMalloc(UChar,  uiNumPartition);
    m_puhTransformSkip[0] = (UChar* )xMalloc(UChar,  uiNumPartition);
    m_puhTransformSkip[1] = (UChar* )xMalloc(UChar,  uiNumPartition);
    m_puhTransformSkip[2] = (UChar* )xMalloc(UChar,  uiNumPartition);

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
#if H_3D_NBDV 
    m_pDvInfo            = (DisInfo* )xMalloc(DisInfo,  uiNumPartition);
#endif
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
    
#if H_3D_ARP
    m_puhARPW            = new UChar[ uiNumPartition];
#endif
#if H_3D_IC
    m_pbICFlag           = (Bool* )xMalloc(Bool,   uiNumPartition);
#endif
#if H_3D_DIM
    for( Int i = 0; i < DIM_NUM_TYPE; i++ )
    {
      m_dimDeltaDC[i][0] = (Pel* )xMalloc(Pel, uiNumPartition); 
      m_dimDeltaDC[i][1] = (Pel* )xMalloc(Pel, uiNumPartition);
    }
#if H_3D_DIM_DMM
    for( Int i = 0; i < DMM_NUM_TYPE; i++ )
    {
      m_dmmWedgeTabIdx[i]    = (UInt*)xMalloc(UInt, uiNumPartition);
    }
#endif
#if H_3D_DIM_SDC
    m_pbSDCFlag             = (Bool*)xMalloc(Bool, uiNumPartition);
    m_apSegmentDCOffset[0]  = (Pel*)xMalloc(Pel, uiNumPartition);
    m_apSegmentDCOffset[1]  = (Pel*)xMalloc(Pel, uiNumPartition);
#endif
#endif
#if H_3D_DBBP
    m_pbDBBPFlag         = (Bool*  )xMalloc(Bool,   uiNumPartition);
#endif
  }
  else
  {
    m_acCUMvField[0].setNumPartition(uiNumPartition );
    m_acCUMvField[1].setNumPartition(uiNumPartition );
  }
  m_sliceStartCU        = (UInt*  )xMalloc(UInt, uiNumPartition);
  m_sliceSegmentStartCU = (UInt*  )xMalloc(UInt, uiNumPartition);
  
  // create pattern memory
  m_pcPattern            = (TComPattern*)xMalloc(TComPattern, 1);
  
#if H_3D_DDD
  m_pucDisparityDerivedDepth         = (UChar*  )xMalloc( UChar,  uiNumPartition);
  m_pbUseDDD                         = (Bool*  ) xMalloc( Bool,   uiNumPartition);
#endif 

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
    if ( m_puhWidth           ) { xFree(m_puhWidth);            m_puhWidth          = NULL; }
    if ( m_puhHeight          ) { xFree(m_puhHeight);           m_puhHeight         = NULL; }

    if ( m_skipFlag           ) { delete[] m_skipFlag;          m_skipFlag          = NULL; }
#if H_3D_SINGLE_DEPTH
    if ( m_singleDepthFlag    ) { delete[] m_singleDepthFlag;   m_singleDepthFlag     = NULL; }
    if ( m_apSingleDepthValue ) { xFree(m_apSingleDepthValue);  m_apSingleDepthValue  = NULL; }
#endif
    if ( m_pePartSize         ) { delete[] m_pePartSize;        m_pePartSize        = NULL; }
    if ( m_pePredMode         ) { delete[] m_pePredMode;        m_pePredMode        = NULL; }
    if ( m_CUTransquantBypass ) { delete[] m_CUTransquantBypass;m_CUTransquantBypass = NULL; }
    if ( m_puhCbf[0]          ) { xFree(m_puhCbf[0]);           m_puhCbf[0]         = NULL; }
    if ( m_puhCbf[1]          ) { xFree(m_puhCbf[1]);           m_puhCbf[1]         = NULL; }
    if ( m_puhCbf[2]          ) { xFree(m_puhCbf[2]);           m_puhCbf[2]         = NULL; }
    if ( m_puhInterDir        ) { xFree(m_puhInterDir);         m_puhInterDir       = NULL; }
    if ( m_pbMergeFlag        ) { xFree(m_pbMergeFlag);         m_pbMergeFlag       = NULL; }
    if ( m_puhMergeIndex      ) { xFree(m_puhMergeIndex);       m_puhMergeIndex     = NULL; }
#if H_3D_VSP
    if ( m_piVSPFlag          ) { xFree(m_piVSPFlag);           m_piVSPFlag         = NULL; }
#endif
#if H_3D_SPIVMP
    if ( m_pbSPIVMPFlag       ) { xFree(m_pbSPIVMPFlag);           m_pbSPIVMPFlag         = NULL; }
#endif
    if ( m_puhLumaIntraDir    ) { xFree(m_puhLumaIntraDir);     m_puhLumaIntraDir   = NULL; }
    if ( m_puhChromaIntraDir  ) { xFree(m_puhChromaIntraDir);   m_puhChromaIntraDir = NULL; }
    if ( m_puhTrIdx           ) { xFree(m_puhTrIdx);            m_puhTrIdx          = NULL; }
    if ( m_puhTransformSkip[0]) { xFree(m_puhTransformSkip[0]); m_puhTransformSkip[0] = NULL; }
    if ( m_puhTransformSkip[1]) { xFree(m_puhTransformSkip[1]); m_puhTransformSkip[1] = NULL; }
    if ( m_puhTransformSkip[2]) { xFree(m_puhTransformSkip[2]); m_puhTransformSkip[2] = NULL; }
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
#if H_3D_NBDV 
    if ( m_pDvInfo            ) { xFree(m_pDvInfo);             m_pDvInfo           = NULL; }
#endif

#if H_3D_DDD
    if ( m_pucDisparityDerivedDepth ) { xFree(m_pucDisparityDerivedDepth);          m_pucDisparityDerivedDepth        = NULL; }
    if ( m_pbUseDDD                 ) { xFree(m_pbUseDDD);                          m_pbUseDDD                        = NULL; }
#endif

#if H_3D_ARP
    if ( m_puhARPW            ) { delete[] m_puhARPW;           m_puhARPW           = NULL; }
#endif
#if H_3D_IC
    if ( m_pbICFlag           ) { xFree(m_pbICFlag);            m_pbICFlag          = NULL; }
#endif
    m_acCUMvField[0].destroy();
    m_acCUMvField[1].destroy();

#if H_3D_DIM
    for( Int i = 0; i < DIM_NUM_TYPE; i++ )
    {
      if ( m_dimDeltaDC[i][0] ) { xFree( m_dimDeltaDC[i][0] ); m_dimDeltaDC[i][0] = NULL; }
      if ( m_dimDeltaDC[i][1] ) { xFree( m_dimDeltaDC[i][1] ); m_dimDeltaDC[i][1] = NULL; }
    }
#if H_3D_DIM_DMM
    for( Int i = 0; i < DMM_NUM_TYPE; i++ )
    {
      if ( m_dmmWedgeTabIdx[i] ) { xFree( m_dmmWedgeTabIdx[i] ); m_dmmWedgeTabIdx[i] = NULL; }
    }
#endif
#if H_3D_DIM_SDC
    if ( m_pbSDCFlag            ) { xFree(m_pbSDCFlag);             m_pbSDCFlag             = NULL; }
    if ( m_apSegmentDCOffset[0] ) { xFree(m_apSegmentDCOffset[0]);  m_apSegmentDCOffset[0]  = NULL; }
    if ( m_apSegmentDCOffset[1] ) { xFree(m_apSegmentDCOffset[1]);  m_apSegmentDCOffset[1]  = NULL; }
#endif    
#endif    
#if H_3D_DBBP
    if ( m_pbDBBPFlag         ) { xFree(m_pbDBBPFlag);          m_pbDBBPFlag        = NULL; }
#endif
  }
  m_pcCUAboveLeft       = NULL;
  m_pcCUAboveRight      = NULL;
  m_pcCUAbove           = NULL;
  m_pcCULeft            = NULL;
  
  m_apcCUColocated[0]   = NULL;
  m_apcCUColocated[1]   = NULL;

  if( m_sliceStartCU )
  {
    xFree(m_sliceStartCU);
    m_sliceStartCU=NULL;
  }
  if(m_sliceSegmentStartCU )
  {
    xFree(m_sliceSegmentStartCU);
    m_sliceSegmentStartCU=NULL;
  }
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
  
  for(Int i=0; i<pcPic->getNumPartInCU(); i++)
  {
    if(pcPic->getPicSym()->getInverseCUOrderMap(iCUAddr)*pcPic->getNumPartInCU()+i>=getSlice()->getSliceCurStartCUAddr())
    {
      m_sliceStartCU[i]=getSlice()->getSliceCurStartCUAddr();
    }
    else
    {
      m_sliceStartCU[i]=pcPic->getCU(getAddr())->m_sliceStartCU[i];
    }
  }
  for(Int i=0; i<pcPic->getNumPartInCU(); i++)
  {
    if(pcPic->getPicSym()->getInverseCUOrderMap(iCUAddr)*pcPic->getNumPartInCU()+i>=getSlice()->getSliceSegmentCurStartCUAddr())
    {
      m_sliceSegmentStartCU[i]=getSlice()->getSliceSegmentCurStartCUAddr();
    }
    else
    {
      m_sliceSegmentStartCU[i]=pcPic->getCU(getAddr())->m_sliceSegmentStartCU[i];
    }
  }

  Int partStartIdx = getSlice()->getSliceSegmentCurStartCUAddr() - pcPic->getPicSym()->getInverseCUOrderMap(iCUAddr) * pcPic->getNumPartInCU();

  Int numElements = min<Int>( partStartIdx, m_uiNumPartition );
  for ( Int ui = 0; ui < numElements; ui++ )
  {
    TComDataCU * pcFrom = pcPic->getCU(getAddr());
    m_skipFlag[ui]   = pcFrom->getSkipFlag(ui);
#if H_3D_SINGLE_DEPTH
    m_singleDepthFlag[ui]    = pcFrom->getSingleDepthFlag(ui);
    m_apSingleDepthValue[ui] = pcFrom->getSingleDepthValue(ui);
#endif
    m_pePartSize[ui] = pcFrom->getPartitionSize(ui);
    m_pePredMode[ui] = pcFrom->getPredictionMode(ui);
    m_CUTransquantBypass[ui] = pcFrom->getCUTransquantBypass(ui);
    m_puhDepth[ui] = pcFrom->getDepth(ui);
#if H_3D_ARP
    m_puhARPW   [ui] = pcFrom->getARPW( ui );
#endif
#if H_3D_IC
    m_pbICFlag[ui]   =  pcFrom->m_pbICFlag[ui];
#endif

#if H_3D_DDD
    m_pucDisparityDerivedDepth[ui] = pcFrom->m_pucDisparityDerivedDepth[ui];
    m_pbUseDDD[ui] = pcFrom->m_pbUseDDD[ui];
#endif

    m_puhWidth  [ui] = pcFrom->getWidth(ui);
    m_puhHeight [ui] = pcFrom->getHeight(ui);
    m_puhTrIdx  [ui] = pcFrom->getTransformIdx(ui);
    m_puhTransformSkip[0][ui] = pcFrom->getTransformSkip(ui,TEXT_LUMA);
    m_puhTransformSkip[1][ui] = pcFrom->getTransformSkip(ui,TEXT_CHROMA_U);
    m_puhTransformSkip[2][ui] = pcFrom->getTransformSkip(ui,TEXT_CHROMA_V);
    m_apiMVPIdx[0][ui] = pcFrom->m_apiMVPIdx[0][ui];;
    m_apiMVPIdx[1][ui] = pcFrom->m_apiMVPIdx[1][ui];
    m_apiMVPNum[0][ui] = pcFrom->m_apiMVPNum[0][ui];
    m_apiMVPNum[1][ui] = pcFrom->m_apiMVPNum[1][ui];
    m_phQP[ui]=pcFrom->m_phQP[ui];
    m_pbMergeFlag[ui]=pcFrom->m_pbMergeFlag[ui];
    m_puhMergeIndex[ui]=pcFrom->m_puhMergeIndex[ui];
#if H_3D_VSP
    m_piVSPFlag[ui] = pcFrom->m_piVSPFlag[ui];
#endif
#if H_3D_SPIVMP
    m_pbSPIVMPFlag[ui] = pcFrom->m_pbSPIVMPFlag[ui];
#endif
    m_puhLumaIntraDir[ui]=pcFrom->m_puhLumaIntraDir[ui];
    m_puhChromaIntraDir[ui]=pcFrom->m_puhChromaIntraDir[ui];
    m_puhInterDir[ui]=pcFrom->m_puhInterDir[ui];
    m_puhCbf[0][ui]=pcFrom->m_puhCbf[0][ui];
    m_puhCbf[1][ui]=pcFrom->m_puhCbf[1][ui];
    m_puhCbf[2][ui]=pcFrom->m_puhCbf[2][ui];
    m_pbIPCMFlag[ui] = pcFrom->m_pbIPCMFlag[ui];
#if H_3D_DIM_SDC
    m_pbSDCFlag[ui] = pcFrom->m_pbSDCFlag[ui];
#endif
#if H_3D_DBBP
    m_pbDBBPFlag[ui] = pcFrom->m_pbDBBPFlag[ui];
#endif
  }
  
  Int firstElement = max<Int>( partStartIdx, 0 );
  numElements = m_uiNumPartition - firstElement;
  
  if ( numElements > 0 )
  {
    memset( m_skipFlag          + firstElement, false,                    numElements * sizeof( *m_skipFlag ) );
#if H_3D_SINGLE_DEPTH
    memset( m_singleDepthFlag     + firstElement, false,                  numElements * sizeof( *m_singleDepthFlag ) );
    memset( m_apSingleDepthValue  + firstElement,     0,                  numElements * sizeof( *m_apSingleDepthValue ) );
#endif
    memset( m_pePartSize        + firstElement, SIZE_NONE,                numElements * sizeof( *m_pePartSize ) );
    memset( m_pePredMode        + firstElement, MODE_NONE,                numElements * sizeof( *m_pePredMode ) );
    memset( m_CUTransquantBypass+ firstElement, false,                    numElements * sizeof( *m_CUTransquantBypass) );
    memset( m_puhDepth          + firstElement, 0,                        numElements * sizeof( *m_puhDepth ) );
    memset( m_puhTrIdx          + firstElement, 0,                        numElements * sizeof( *m_puhTrIdx ) );
    memset( m_puhTransformSkip[0] + firstElement, 0,                      numElements * sizeof( *m_puhTransformSkip[0]) );
    memset( m_puhTransformSkip[1] + firstElement, 0,                      numElements * sizeof( *m_puhTransformSkip[1]) );
    memset( m_puhTransformSkip[2] + firstElement, 0,                      numElements * sizeof( *m_puhTransformSkip[2]) );
    memset( m_puhWidth          + firstElement, g_uiMaxCUWidth,           numElements * sizeof( *m_puhWidth ) );
    memset( m_puhHeight         + firstElement, g_uiMaxCUHeight,          numElements * sizeof( *m_puhHeight ) );
    memset( m_apiMVPIdx[0]      + firstElement, -1,                       numElements * sizeof( *m_apiMVPIdx[0] ) );
    memset( m_apiMVPIdx[1]      + firstElement, -1,                       numElements * sizeof( *m_apiMVPIdx[1] ) );
    memset( m_apiMVPNum[0]      + firstElement, -1,                       numElements * sizeof( *m_apiMVPNum[0] ) );
    memset( m_apiMVPNum[1]      + firstElement, -1,                       numElements * sizeof( *m_apiMVPNum[1] ) );
    memset( m_phQP              + firstElement, getSlice()->getSliceQp(), numElements * sizeof( *m_phQP ) );
    memset( m_pbMergeFlag       + firstElement, false,                    numElements * sizeof( *m_pbMergeFlag ) );
    memset( m_puhMergeIndex     + firstElement, 0,                        numElements * sizeof( *m_puhMergeIndex ) );
#if H_3D_VSP
    memset( m_piVSPFlag         + firstElement, 0,                        numElements * sizeof( *m_piVSPFlag ) );
#endif
#if H_3D_SPIVMP
    memset( m_pbSPIVMPFlag      + firstElement, 0,                        numElements * sizeof( *m_pbSPIVMPFlag ) );
#endif
    memset( m_puhLumaIntraDir   + firstElement, DC_IDX,                   numElements * sizeof( *m_puhLumaIntraDir ) );
    memset( m_puhChromaIntraDir + firstElement, 0,                        numElements * sizeof( *m_puhChromaIntraDir ) );
    memset( m_puhInterDir       + firstElement, 0,                        numElements * sizeof( *m_puhInterDir ) );
    memset( m_puhCbf[0]         + firstElement, 0,                        numElements * sizeof( *m_puhCbf[0] ) );
    memset( m_puhCbf[1]         + firstElement, 0,                        numElements * sizeof( *m_puhCbf[1] ) );
    memset( m_puhCbf[2]         + firstElement, 0,                        numElements * sizeof( *m_puhCbf[2] ) );
    memset( m_pbIPCMFlag        + firstElement, false,                    numElements * sizeof( *m_pbIPCMFlag ) );
#if H_3D_ARP
    memset( m_puhARPW           + firstElement, 0,                        numElements * sizeof( UChar )         );
#endif
#if H_3D_IC
    memset( m_pbICFlag          + firstElement, false,                    numElements * sizeof( *m_pbICFlag )   );
#endif

#if H_3D_DDD
    memset( m_pucDisparityDerivedDepth        + firstElement, 0,           numElements * sizeof( *m_pucDisparityDerivedDepth ) );
    memset( m_pbUseDDD                        + firstElement, 0,           numElements * sizeof( *m_pbUseDDD ) );
#endif

#if H_3D_DIM
    for( Int i = 0; i < DIM_NUM_TYPE; i++ )
    {
      memset( m_dimDeltaDC[i][0] + firstElement, 0,                       numElements * sizeof( *m_dimDeltaDC[i][0] ) );
      memset( m_dimDeltaDC[i][1] + firstElement, 0,                       numElements * sizeof( *m_dimDeltaDC[i][1] ) );
    }
#if H_3D_DIM_DMM
    for( Int i = 0; i < DMM_NUM_TYPE; i++ )
    {
      memset( m_dmmWedgeTabIdx[i] + firstElement, 0,                      numElements * sizeof( *m_dmmWedgeTabIdx[i] ) );
    }
#endif
#if H_3D_DIM_SDC
    memset( m_pbSDCFlag             + firstElement,     0,                numElements * sizeof( *m_pbSDCFlag            ) );
    memset( m_apSegmentDCOffset[0]  + firstElement,     0,                numElements * sizeof( *m_apSegmentDCOffset[0] ) );
    memset( m_apSegmentDCOffset[1]  + firstElement,     0,                numElements * sizeof( *m_apSegmentDCOffset[1] ) );
#endif
    m_apDmmPredictor[0] = 0;
    m_apDmmPredictor[1] = 0;
#endif
#if H_3D_DBBP
    memset( m_pbDBBPFlag        + firstElement, false,                    numElements * sizeof( *m_pbDBBPFlag ) );
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
    for(Int i=0; i<uiTmp; i++)
    {
      m_pcTrCoeffY[i]=pcFrom->m_pcTrCoeffY[i];
#if ADAPTIVE_QP_SELECTION
      m_pcArlCoeffY[i]=pcFrom->m_pcArlCoeffY[i];
#endif
      m_pcIPCMSampleY[i]=pcFrom->m_pcIPCMSampleY[i];
    }
    for(Int i=0; i<(uiTmp>>2); i++)
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
Void TComDataCU::initEstData( UInt uiDepth, Int qp, Bool bTransquantBypass )
{
  m_dTotalCost         = MAX_DOUBLE;
  m_uiTotalDistortion  = 0;
  m_uiTotalBits        = 0;
  m_uiTotalBins        = 0;

  UChar uhWidth  = g_uiMaxCUWidth  >> uiDepth;
  UChar uhHeight = g_uiMaxCUHeight >> uiDepth;

  for (UInt ui = 0; ui < m_uiNumPartition; ui++)
  {
    if(getPic()->getPicSym()->getInverseCUOrderMap(getAddr())*m_pcPic->getNumPartInCU()+m_uiAbsIdxInLCU+ui >= getSlice()->getSliceSegmentCurStartCUAddr())
    {
      m_apiMVPIdx[0][ui] = -1;
      m_apiMVPIdx[1][ui] = -1;
      m_apiMVPNum[0][ui] = -1;
      m_apiMVPNum[1][ui] = -1;
      m_puhDepth  [ui] = uiDepth;
      m_puhWidth  [ui] = uhWidth;
      m_puhHeight [ui] = uhHeight;
      m_puhTrIdx  [ui] = 0;
      m_puhTransformSkip[0][ui] = 0;
      m_puhTransformSkip[1][ui] = 0;
      m_puhTransformSkip[2][ui] = 0;
      m_skipFlag[ui]   = false;
#if H_3D_SINGLE_DEPTH
      m_singleDepthFlag[ui]     = false;
      m_apSingleDepthValue[ui]  = 0;
#endif
      m_pePartSize[ui] = SIZE_NONE;
      m_pePredMode[ui] = MODE_NONE;
      m_CUTransquantBypass[ui] = bTransquantBypass;
      m_pbIPCMFlag[ui] = 0;
      m_phQP[ui] = qp;
      m_pbMergeFlag[ui] = 0;
      m_puhMergeIndex[ui] = 0;
#if H_3D_VSP
      m_piVSPFlag[ui] = 0;
#endif
#if H_3D_SPIVMP
      m_pbSPIVMPFlag[ui] = 0;
#endif
      m_puhLumaIntraDir[ui] = DC_IDX;
      m_puhChromaIntraDir[ui] = 0;
      m_puhInterDir[ui] = 0;
      m_puhCbf[0][ui] = 0;
      m_puhCbf[1][ui] = 0;
      m_puhCbf[2][ui] = 0;
#if H_3D_ARP
      m_puhARPW[ui] = 0;
#endif
#if H_3D_IC
      m_pbICFlag[ui]  = false;
#endif

#if H_3D_DDD
      m_pucDisparityDerivedDepth[ui] = 0;
      m_pbUseDDD[ui] = 0;
#endif

#if H_3D_DIM
      for( Int i = 0; i < DIM_NUM_TYPE; i++ )
      {
        m_dimDeltaDC[i][0] [ui] = 0;
        m_dimDeltaDC[i][1] [ui] = 0;
      }
#if H_3D_DIM_DMM
      for( Int i = 0; i < DMM_NUM_TYPE; i++ )
      {
        m_dmmWedgeTabIdx[i] [ui] = 0;
      }
#endif
#if H_3D_DIM_SDC
      m_pbSDCFlag           [ui] = false;
      m_apSegmentDCOffset[0][ui] = 0;
      m_apSegmentDCOffset[1][ui] = 0;
#endif
      m_apDmmPredictor[0] = 0;
      m_apDmmPredictor[1] = 0;
#endif
#if H_3D_DBBP
      m_pbDBBPFlag[ui] = false;
#endif
    }
  }

  if(getPic()->getPicSym()->getInverseCUOrderMap(getAddr())*m_pcPic->getNumPartInCU()+m_uiAbsIdxInLCU >= getSlice()->getSliceSegmentCurStartCUAddr())
  {
    m_acCUMvField[0].clearMvField();
    m_acCUMvField[1].clearMvField();
    UInt uiTmp = uhWidth*uhHeight;
    
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

  Int iSizeInUchar = sizeof( UChar  ) * m_uiNumPartition;
  Int iSizeInBool  = sizeof( Bool   ) * m_uiNumPartition;

  Int sizeInChar = sizeof( Char  ) * m_uiNumPartition;
  memset( m_phQP,              qp,  sizeInChar );

  memset( m_pbMergeFlag,        0, iSizeInBool  );
  memset( m_puhMergeIndex,      0, iSizeInUchar );
#if H_3D_VSP
  memset( m_piVSPFlag,          0, sizeof( Char  ) * m_uiNumPartition );
#endif
#if H_3D_SPIVMP
  memset( m_pbSPIVMPFlag,       0, sizeof( Bool  ) * m_uiNumPartition );
#endif
  memset( m_puhLumaIntraDir,    DC_IDX, iSizeInUchar );
  memset( m_puhChromaIntraDir,  0, iSizeInUchar );
  memset( m_puhInterDir,        0, iSizeInUchar );
  memset( m_puhTrIdx,           0, iSizeInUchar );
  memset( m_puhTransformSkip[0], 0, iSizeInUchar );
  memset( m_puhTransformSkip[1], 0, iSizeInUchar );
  memset( m_puhTransformSkip[2], 0, iSizeInUchar );
  memset( m_puhCbf[0],          0, iSizeInUchar );
  memset( m_puhCbf[1],          0, iSizeInUchar );
  memset( m_puhCbf[2],          0, iSizeInUchar );
  memset( m_puhDepth,     uiDepth, iSizeInUchar );
#if H_3D_NBDV
  m_pDvInfo->bDV = false;
#endif
#if H_3D_ARP
  memset( m_puhARPW,            0, iSizeInUchar  );
#endif

#if H_3D_DDD
  memset( m_pucDisparityDerivedDepth,         0, iSizeInUchar );
  memset( m_pbUseDDD,                         0, iSizeInBool );
#endif

  UChar uhWidth  = g_uiMaxCUWidth  >> uiDepth;
  UChar uhHeight = g_uiMaxCUHeight >> uiDepth;
  memset( m_puhWidth,          uhWidth,  iSizeInUchar );
  memset( m_puhHeight,         uhHeight, iSizeInUchar );
  memset( m_pbIPCMFlag,        0, iSizeInBool  );
#if H_3D_IC
  memset( m_pbICFlag,          0, iSizeInBool  );
#endif
#if H_3D_DIM
  for( Int i = 0; i < DIM_NUM_TYPE; i++ )
  {
    memset( m_dimDeltaDC[i][0], 0, sizeof(Pel ) * m_uiNumPartition );
    memset( m_dimDeltaDC[i][1], 0, sizeof(Pel ) * m_uiNumPartition );
  }
#if H_3D_DIM_DMM
  for( Int i = 0; i < DMM_NUM_TYPE; i++ )
  {
    memset( m_dmmWedgeTabIdx[i], 0, sizeof(UInt) * m_uiNumPartition );
  }
#endif
#if H_3D_DIM_SDC
  memset( m_pbSDCFlag,            0, sizeof(Bool) * m_uiNumPartition  );
  memset( m_apSegmentDCOffset[0], 0, sizeof(Pel) * m_uiNumPartition   );
  memset( m_apSegmentDCOffset[1], 0, sizeof(Pel) * m_uiNumPartition   );
#endif
  m_apDmmPredictor[0] = 0;
  m_apDmmPredictor[1] = 0;
#endif
#if H_3D_DBBP
  memset( m_pbDBBPFlag,         0, iSizeInBool  );
#endif

  for (UInt ui = 0; ui < m_uiNumPartition; ui++)
  {
    m_skipFlag[ui]   = false;
#if H_3D_SINGLE_DEPTH
    m_singleDepthFlag[ui]   = false;
    m_apSingleDepthValue[ui]= 0;
#endif
    m_pePartSize[ui] = SIZE_NONE;
    m_pePredMode[ui] = MODE_NONE;
    m_CUTransquantBypass[ui] = false;
    m_apiMVPIdx[0][ui] = -1;
    m_apiMVPIdx[1][ui] = -1;
    m_apiMVPNum[0][ui] = -1;
    m_apiMVPNum[1][ui] = -1;
    if(m_pcPic->getPicSym()->getInverseCUOrderMap(getAddr())*m_pcPic->getNumPartInCU()+m_uiAbsIdxInLCU+ui<getSlice()->getSliceSegmentCurStartCUAddr())
    {
      m_apiMVPIdx[0][ui] = pcCU->m_apiMVPIdx[0][uiPartOffset+ui];
      m_apiMVPIdx[1][ui] = pcCU->m_apiMVPIdx[1][uiPartOffset+ui];;
      m_apiMVPNum[0][ui] = pcCU->m_apiMVPNum[0][uiPartOffset+ui];;
      m_apiMVPNum[1][ui] = pcCU->m_apiMVPNum[1][uiPartOffset+ui];;
      m_puhDepth  [ui] = pcCU->getDepth(uiPartOffset+ui);
      m_puhWidth  [ui] = pcCU->getWidth(uiPartOffset+ui);
      m_puhHeight  [ui] = pcCU->getHeight(uiPartOffset+ui);
      m_puhTrIdx  [ui] = pcCU->getTransformIdx(uiPartOffset+ui);
      m_puhTransformSkip[0][ui] = pcCU->getTransformSkip(uiPartOffset+ui,TEXT_LUMA);
      m_puhTransformSkip[1][ui] = pcCU->getTransformSkip(uiPartOffset+ui,TEXT_CHROMA_U);
      m_puhTransformSkip[2][ui] = pcCU->getTransformSkip(uiPartOffset+ui,TEXT_CHROMA_V);
      m_skipFlag[ui]   = pcCU->getSkipFlag(uiPartOffset+ui);
#if H_3D_SINGLE_DEPTH
      m_singleDepthFlag[ui]    = pcCU->getSingleDepthFlag(uiPartOffset+ui);
      m_apSingleDepthValue[ui] = pcCU->getSingleDepthValue(uiPartOffset+ui);
#endif
      m_pePartSize[ui] = pcCU->getPartitionSize(uiPartOffset+ui);
      m_pePredMode[ui] = pcCU->getPredictionMode(uiPartOffset+ui);
      m_CUTransquantBypass[ui] = pcCU->getCUTransquantBypass(uiPartOffset+ui);
      m_pbIPCMFlag[ui]=pcCU->m_pbIPCMFlag[uiPartOffset+ui];
      m_phQP[ui] = pcCU->m_phQP[uiPartOffset+ui];
      m_pbMergeFlag[ui]=pcCU->m_pbMergeFlag[uiPartOffset+ui];
      m_puhMergeIndex[ui]=pcCU->m_puhMergeIndex[uiPartOffset+ui];
#if H_3D_VSP
      m_piVSPFlag[ui]=pcCU->m_piVSPFlag[uiPartOffset+ui];
      m_pDvInfo[ ui ] = pcCU->m_pDvInfo[uiPartOffset+ui];
#endif
#if H_3D_SPIVMP
      m_pbSPIVMPFlag[ui]=pcCU->m_pbSPIVMPFlag[uiPartOffset+ui];
#endif
      m_puhLumaIntraDir[ui]=pcCU->m_puhLumaIntraDir[uiPartOffset+ui];
      m_puhChromaIntraDir[ui]=pcCU->m_puhChromaIntraDir[uiPartOffset+ui];
      m_puhInterDir[ui]=pcCU->m_puhInterDir[uiPartOffset+ui];
      m_puhCbf[0][ui]=pcCU->m_puhCbf[0][uiPartOffset+ui];
      m_puhCbf[1][ui]=pcCU->m_puhCbf[1][uiPartOffset+ui];
      m_puhCbf[2][ui]=pcCU->m_puhCbf[2][uiPartOffset+ui];

#if H_3D_ARP
      m_puhARPW           [ui] = pcCU->getARPW( uiPartOffset+ui );
#endif
#if H_3D_IC
      m_pbICFlag          [ui] = pcCU->m_pbICFlag[uiPartOffset+ui];
#endif

#if H_3D_DDD
      m_pucDisparityDerivedDepth[ui] = pcCU->m_pucDisparityDerivedDepth[uiPartOffset+ui];
      m_pbUseDDD[ui]                 = pcCU->m_pbUseDDD[uiPartOffset+ui];
#endif

#if H_3D_DIM
      for( Int i = 0; i < DIM_NUM_TYPE; i++ )
      {
        m_dimDeltaDC[i][0] [ui] = pcCU->m_dimDeltaDC[i][0] [uiPartOffset+ui];
        m_dimDeltaDC[i][1] [ui] = pcCU->m_dimDeltaDC[i][1] [uiPartOffset+ui];
      }
#if H_3D_DIM_DMM
      for( Int i = 0; i < DMM_NUM_TYPE; i++ )
      {
        m_dmmWedgeTabIdx[i] [ui] = pcCU->m_dmmWedgeTabIdx[i] [uiPartOffset+ui];
      }
#endif
#if H_3D_DIM_SDC
      m_pbSDCFlag           [ui] = pcCU->m_pbSDCFlag            [ uiPartOffset + ui ];
      m_apSegmentDCOffset[0][ui] = pcCU->m_apSegmentDCOffset[0] [ uiPartOffset + ui ];
      m_apSegmentDCOffset[1][ui] = pcCU->m_apSegmentDCOffset[1] [ uiPartOffset + ui ];
#endif
#endif
#if H_3D_DBBP
      m_pbDBBPFlag[ui]=pcCU->m_pbDBBPFlag[uiPartOffset+ui];
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

  if(m_pcPic->getPicSym()->getInverseCUOrderMap(getAddr())*m_pcPic->getNumPartInCU()+m_uiAbsIdxInLCU<getSlice()->getSliceSegmentCurStartCUAddr())
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
    for(Int i=0; i<uiTmp; i++)
    {
      m_pcTrCoeffY[i]=bigCU->m_pcTrCoeffY[uiCoffOffset+i];
#if ADAPTIVE_QP_SELECTION
      m_pcArlCoeffY[i]=bigCU->m_pcArlCoeffY[uiCoffOffset+i];
#endif
      m_pcIPCMSampleY[i]=bigCU->m_pcIPCMSampleY[uiCoffOffset+i];
    }
    uiTmp>>=2;
    uiCoffOffset>>=2;
    for(Int i=0; i<uiTmp; i++)
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
  memcpy(m_sliceStartCU,pcCU->m_sliceStartCU+uiPartOffset,sizeof(UInt)*m_uiNumPartition);
  memcpy(m_sliceSegmentStartCU,pcCU->m_sliceSegmentStartCU+uiPartOffset,sizeof(UInt)*m_uiNumPartition);
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
  
  m_skipFlag=pcCU->getSkipFlag()          + uiPart;
#if H_3D_SINGLE_DEPTH
  m_singleDepthFlag     = pcCU->getSingleDepthFlag()   + uiPart;
  m_apSingleDepthValue  = pcCU->getSingleDepthValue()  + uiPart;
#endif  
  m_phQP=pcCU->getQP()                    + uiPart;
  m_pePartSize = pcCU->getPartitionSize() + uiPart;
  m_pePredMode=pcCU->getPredictionMode()  + uiPart;
  m_CUTransquantBypass  = pcCU->getCUTransquantBypass()+uiPart;
  
#if H_3D_NBDV
  m_pDvInfo             = pcCU->getDvInfo()           + uiPart;
#endif
  m_pbMergeFlag         = pcCU->getMergeFlag()        + uiPart;
  m_puhMergeIndex       = pcCU->getMergeIndex()       + uiPart;
#if H_3D_VSP
  m_piVSPFlag           = pcCU->getVSPFlag()          + uiPart;
#endif
#if H_3D_SPIVMP
  m_pbSPIVMPFlag        = pcCU->getSPIVMPFlag()          + uiPart;
#endif

#if H_3D_ARP
  m_puhARPW             = pcCU->getARPW()             + uiPart;
#endif
#if H_3D_IC
  m_pbICFlag            = pcCU->getICFlag()           + uiPart;
#endif

#if H_3D_DDD
  m_pucDisparityDerivedDepth          = pcCU->getDDDepth()        + uiPart;
  m_pbUseDDD                          = pcCU->getUseDDD()         + uiPart;
#endif

  m_puhLumaIntraDir     = pcCU->getLumaIntraDir()     + uiPart;
  m_puhChromaIntraDir   = pcCU->getChromaIntraDir()   + uiPart;
  m_puhInterDir         = pcCU->getInterDir()         + uiPart;
  m_puhTrIdx            = pcCU->getTransformIdx()     + uiPart;
  m_puhTransformSkip[0] = pcCU->getTransformSkip(TEXT_LUMA)     + uiPart;
  m_puhTransformSkip[1] = pcCU->getTransformSkip(TEXT_CHROMA_U) + uiPart;
  m_puhTransformSkip[2] = pcCU->getTransformSkip(TEXT_CHROMA_V) + uiPart;

  m_puhCbf[0]= pcCU->getCbf(TEXT_LUMA)            + uiPart;
  m_puhCbf[1]= pcCU->getCbf(TEXT_CHROMA_U)        + uiPart;
  m_puhCbf[2]= pcCU->getCbf(TEXT_CHROMA_V)        + uiPart;
#if H_3D_DIM
  for( Int i = 0; i < DIM_NUM_TYPE; i++ )
  {
    m_dimDeltaDC[i][0] = pcCU->getDimDeltaDC( i, 0 ) + uiPart;
    m_dimDeltaDC[i][1] = pcCU->getDimDeltaDC( i, 1 ) + uiPart;
  }
#if H_3D_DIM_DMM
  for( Int i = 0; i < DMM_NUM_TYPE; i++ )
  {
    m_dmmWedgeTabIdx[i] = pcCU->getDmmWedgeTabIdx( i ) + uiPart;
  }
#endif
#if H_3D_DIM_SDC
  m_pbSDCFlag               = pcCU->getSDCFlag()              + uiPart;
  m_apSegmentDCOffset[0]    = pcCU->getSDCSegmentDCOffset(0)  + uiPart;
  m_apSegmentDCOffset[1]    = pcCU->getSDCSegmentDCOffset(1)  + uiPart;
#endif  
#endif  
#if H_3D_DBBP
  m_pbDBBPFlag              = pcCU->getDBBPFlag()         + uiPart;
#endif
  m_puhDepth=pcCU->getDepth()                     + uiPart;
  m_puhWidth=pcCU->getWidth()                     + uiPart;
  m_puhHeight=pcCU->getHeight()                   + uiPart;
  
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
  
  UInt uiMaxCuWidth=pcCU->getSlice()->getSPS()->getMaxCUWidth();
  UInt uiMaxCuHeight=pcCU->getSlice()->getSPS()->getMaxCUHeight();
  
  UInt uiCoffOffset = uiMaxCuWidth*uiMaxCuHeight*uiAbsPartIdx/pcCU->getPic()->getNumPartInCU();
  
  m_pcTrCoeffY = pcCU->getCoeffY() + uiCoffOffset;
#if ADAPTIVE_QP_SELECTION
  m_pcArlCoeffY= pcCU->getArlCoeffY() + uiCoffOffset;  
#endif
  m_pcIPCMSampleY = pcCU->getPCMSampleY() + uiCoffOffset;

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
  memcpy(m_sliceStartCU,pcCU->m_sliceStartCU+uiPart,sizeof(UInt)*m_uiNumPartition);
  memcpy(m_sliceSegmentStartCU,pcCU->m_sliceSegmentStartCU+uiPart,sizeof(UInt)*m_uiNumPartition);
}
#if H_3D_NBDV
Void TComDataCU::copyDVInfoFrom (TComDataCU* pcCU, UInt uiAbsPartIdx)
{
  m_pDvInfo            = pcCU->getDvInfo()                + uiAbsPartIdx;
}
#endif
// Copy inter prediction info from the biggest CU
Void TComDataCU::copyInterPredInfoFrom    ( TComDataCU* pcCU, UInt uiAbsPartIdx, RefPicList eRefPicList 
#if H_3D_NBDV
  , Bool bNBDV
#endif
  )
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
  
  m_skipFlag           = pcCU->getSkipFlag ()             + uiAbsPartIdx;
#if H_3D_SINGLE_DEPTH
  m_singleDepthFlag     = pcCU->getSingleDepthFlag ()             + uiAbsPartIdx;
  m_apSingleDepthValue  = pcCU->getSingleDepthValue ()            + uiAbsPartIdx;
#endif  
  m_pePartSize         = pcCU->getPartitionSize ()        + uiAbsPartIdx;
#if H_3D_NBDV
  if(bNBDV == true)
  {
    m_puhWidth           = pcCU->getWidth ()                + uiAbsPartIdx;
    m_puhHeight          = pcCU->getHeight()                + uiAbsPartIdx;
    m_puhDepth           = pcCU->getDepth ()                + uiAbsPartIdx;
  }
  else
  {
#endif
  m_pePredMode         = pcCU->getPredictionMode()        + uiAbsPartIdx;
  m_CUTransquantBypass = pcCU->getCUTransquantBypass()    + uiAbsPartIdx;
  m_puhInterDir        = pcCU->getInterDir      ()        + uiAbsPartIdx;
  
  m_puhDepth           = pcCU->getDepth ()                + uiAbsPartIdx;
  m_puhWidth           = pcCU->getWidth ()                + uiAbsPartIdx;
  m_puhHeight          = pcCU->getHeight()                + uiAbsPartIdx;
  
  m_pbMergeFlag        = pcCU->getMergeFlag()             + uiAbsPartIdx;
  m_puhMergeIndex      = pcCU->getMergeIndex()            + uiAbsPartIdx;
#if H_3D_VSP
  m_piVSPFlag          = pcCU->getVSPFlag()               + uiAbsPartIdx;
  m_pDvInfo            = pcCU->getDvInfo()                + uiAbsPartIdx;
#endif
#if H_3D_SPIVMP
  m_pbSPIVMPFlag       = pcCU->getSPIVMPFlag()               + uiAbsPartIdx;
#endif

  m_apiMVPIdx[eRefPicList] = pcCU->getMVPIdx(eRefPicList) + uiAbsPartIdx;
  m_apiMVPNum[eRefPicList] = pcCU->getMVPNum(eRefPicList) + uiAbsPartIdx;
  
#if H_3D_ARP
  m_puhARPW            = pcCU->getARPW()                  + uiAbsPartIdx;
#endif

#if H_3D_DDD
  m_pucDisparityDerivedDepth         = pcCU->getDDDepth()              + uiAbsPartIdx;
  m_pbUseDDD                         = pcCU->getUseDDD()              + uiAbsPartIdx;
#endif
    
#if H_3D_DBBP
  m_pbDBBPFlag       = pcCU->getDBBPFlag()              + uiAbsPartIdx;
#endif

  m_acCUMvField[ eRefPicList ].linkToWithOffset( pcCU->getCUMvField(eRefPicList), uiAbsPartIdx );

  memcpy(m_sliceStartCU,pcCU->m_sliceStartCU+uiAbsPartIdx,sizeof(UInt)*m_uiNumPartition);
  memcpy(m_sliceSegmentStartCU,pcCU->m_sliceSegmentStartCU+uiAbsPartIdx,sizeof(UInt)*m_uiNumPartition);
#if H_3D_NBDV
  }
#endif
#if H_3D_IC
  m_pbICFlag           = pcCU->getICFlag()                + uiAbsPartIdx;
#endif
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
  
  Int sizeInChar  = sizeof( Char ) * uiNumPartition;
  memcpy( m_skipFlag   + uiOffset, pcCU->getSkipFlag(),       sizeof( *m_skipFlag )   * uiNumPartition );
#if H_3D_SINGLE_DEPTH
  memcpy( m_singleDepthFlag     + uiOffset, pcCU->getSingleDepthFlag(),       sizeof( *m_singleDepthFlag )   * uiNumPartition );
  memcpy( m_apSingleDepthValue  + uiOffset, pcCU->getSingleDepthValue(),      sizeof( *m_apSingleDepthValue ) * uiNumPartition);
#endif
  memcpy( m_phQP       + uiOffset, pcCU->getQP(),             sizeInChar                        );
  memcpy( m_pePartSize + uiOffset, pcCU->getPartitionSize(),  sizeof( *m_pePartSize ) * uiNumPartition );
  memcpy( m_pePredMode + uiOffset, pcCU->getPredictionMode(), sizeof( *m_pePredMode ) * uiNumPartition );
  memcpy( m_CUTransquantBypass + uiOffset, pcCU->getCUTransquantBypass(), sizeof( *m_CUTransquantBypass ) * uiNumPartition );
  memcpy( m_pbMergeFlag         + uiOffset, pcCU->getMergeFlag(),         iSizeInBool  );
  memcpy( m_puhMergeIndex       + uiOffset, pcCU->getMergeIndex(),        iSizeInUchar );
#if H_3D_VSP
  memcpy( m_piVSPFlag           + uiOffset, pcCU->getVSPFlag(),           sizeof( Char ) * uiNumPartition );
  memcpy( m_pDvInfo             + uiOffset, pcCU->getDvInfo(),            sizeof( *m_pDvInfo ) * uiNumPartition );

#endif
#if H_3D_SPIVMP
  memcpy( m_pbSPIVMPFlag        + uiOffset, pcCU->getSPIVMPFlag(),        sizeof( Bool ) * uiNumPartition );
#endif
  memcpy( m_puhLumaIntraDir     + uiOffset, pcCU->getLumaIntraDir(),      iSizeInUchar );
  memcpy( m_puhChromaIntraDir   + uiOffset, pcCU->getChromaIntraDir(),    iSizeInUchar );
  memcpy( m_puhInterDir         + uiOffset, pcCU->getInterDir(),          iSizeInUchar );
  memcpy( m_puhTrIdx            + uiOffset, pcCU->getTransformIdx(),      iSizeInUchar );
  memcpy( m_puhTransformSkip[0] + uiOffset, pcCU->getTransformSkip(TEXT_LUMA),     iSizeInUchar );
  memcpy( m_puhTransformSkip[1] + uiOffset, pcCU->getTransformSkip(TEXT_CHROMA_U), iSizeInUchar );
  memcpy( m_puhTransformSkip[2] + uiOffset, pcCU->getTransformSkip(TEXT_CHROMA_V), iSizeInUchar );

  memcpy( m_puhCbf[0] + uiOffset, pcCU->getCbf(TEXT_LUMA)    , iSizeInUchar );
  memcpy( m_puhCbf[1] + uiOffset, pcCU->getCbf(TEXT_CHROMA_U), iSizeInUchar );
  memcpy( m_puhCbf[2] + uiOffset, pcCU->getCbf(TEXT_CHROMA_V), iSizeInUchar );
  
#if H_3D_DDD
  memcpy( m_pucDisparityDerivedDepth          + uiOffset, pcCU->getDDDepth(),         iSizeInUchar );
  memcpy( m_pbUseDDD                          + uiOffset, pcCU->getUseDDD(),          iSizeInBool );
#endif


#if H_3D_DIM
  for( Int i = 0; i < DIM_NUM_TYPE; i++ )
  {
    memcpy( m_dimDeltaDC[i][0] + uiOffset, pcCU->getDimDeltaDC( i, 0 ), sizeof(Pel ) * uiNumPartition );
    memcpy( m_dimDeltaDC[i][1] + uiOffset, pcCU->getDimDeltaDC( i, 1 ), sizeof(Pel ) * uiNumPartition );
  }
#if H_3D_DIM_DMM
  for( Int i = 0; i < DMM_NUM_TYPE; i++ )
  {
    memcpy( m_dmmWedgeTabIdx[i] + uiOffset, pcCU->getDmmWedgeTabIdx( i ), sizeof(UInt) * uiNumPartition );
  }
#endif
#if H_3D_DIM_SDC
  memcpy( m_pbSDCFlag             + uiOffset, pcCU->getSDCFlag(),             iSizeInBool  );
  memcpy( m_apSegmentDCOffset[0]  + uiOffset, pcCU->getSDCSegmentDCOffset(0), sizeof( Pel ) * uiNumPartition);
  memcpy( m_apSegmentDCOffset[1]  + uiOffset, pcCU->getSDCSegmentDCOffset(1), sizeof( Pel ) * uiNumPartition);
#endif
#endif
#if H_3D_DBBP
  memcpy( m_pbDBBPFlag          + uiOffset, pcCU->getDBBPFlag(),          iSizeInBool  );
#endif

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
  memcpy( m_sliceStartCU        + uiOffset, pcCU->m_sliceStartCU,        sizeof( UInt ) * uiNumPartition  );
  memcpy( m_sliceSegmentStartCU + uiOffset, pcCU->m_sliceSegmentStartCU, sizeof( UInt ) * uiNumPartition  );
#if H_3D_ARP
  memcpy( m_puhARPW             + uiOffset, pcCU->getARPW(),              iSizeInUchar );
#endif
#if H_3D_IC
  memcpy( m_pbICFlag            + uiOffset, pcCU->getICFlag(),            iSizeInBool );
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
  
  Int sizeInChar  = sizeof( Char ) * m_uiNumPartition;

  memcpy( rpcCU->getSkipFlag() + m_uiAbsIdxInLCU, m_skipFlag, sizeof( *m_skipFlag ) * m_uiNumPartition );
#if H_3D_SINGLE_DEPTH
  memcpy( rpcCU->getSingleDepthFlag()  + m_uiAbsIdxInLCU, m_singleDepthFlag,    sizeof( *m_singleDepthFlag ) * m_uiNumPartition );
  memcpy( rpcCU->getSingleDepthValue() + m_uiAbsIdxInLCU, m_apSingleDepthValue, sizeof( *m_apSingleDepthValue ) * m_uiNumPartition);
#endif
  memcpy( rpcCU->getQP() + m_uiAbsIdxInLCU, m_phQP, sizeInChar  );
#if H_3D_NBDV
  memcpy( rpcCU->getDvInfo()         + m_uiAbsIdxInLCU, m_pDvInfo,    sizeof(* m_pDvInfo)     * m_uiNumPartition );
#endif

#if H_3D_DDD
  memcpy( rpcCU->getDDDepth()          + m_uiAbsIdxInLCU, m_pucDisparityDerivedDepth,         iSizeInUchar  );
  memcpy( rpcCU->getUseDDD()           + m_uiAbsIdxInLCU, m_pbUseDDD,                         iSizeInBool  );
#endif

  memcpy( rpcCU->getPartitionSize()  + m_uiAbsIdxInLCU, m_pePartSize, sizeof( *m_pePartSize ) * m_uiNumPartition );
  memcpy( rpcCU->getPredictionMode() + m_uiAbsIdxInLCU, m_pePredMode, sizeof( *m_pePredMode ) * m_uiNumPartition );
  memcpy( rpcCU->getCUTransquantBypass()+ m_uiAbsIdxInLCU, m_CUTransquantBypass, sizeof( *m_CUTransquantBypass ) * m_uiNumPartition );
  memcpy( rpcCU->getMergeFlag()         + m_uiAbsIdxInLCU, m_pbMergeFlag,         iSizeInBool  );
  memcpy( rpcCU->getMergeIndex()        + m_uiAbsIdxInLCU, m_puhMergeIndex,       iSizeInUchar );
#if H_3D_VSP
  memcpy( rpcCU->getVSPFlag()           + m_uiAbsIdxInLCU, m_piVSPFlag,           sizeof( Char ) * m_uiNumPartition );
  memcpy( rpcCU->getDvInfo()            + m_uiAbsIdxInLCU, m_pDvInfo,             sizeof( *m_pDvInfo ) * m_uiNumPartition );
#endif
#if H_3D_SPIVMP
  memcpy( rpcCU->getSPIVMPFlag()        + m_uiAbsIdxInLCU, m_pbSPIVMPFlag,        sizeof( Bool ) * m_uiNumPartition );
#endif
  memcpy( rpcCU->getLumaIntraDir()      + m_uiAbsIdxInLCU, m_puhLumaIntraDir,     iSizeInUchar );
  memcpy( rpcCU->getChromaIntraDir()    + m_uiAbsIdxInLCU, m_puhChromaIntraDir,   iSizeInUchar );
  memcpy( rpcCU->getInterDir()          + m_uiAbsIdxInLCU, m_puhInterDir,         iSizeInUchar );
  memcpy( rpcCU->getTransformIdx()      + m_uiAbsIdxInLCU, m_puhTrIdx,            iSizeInUchar );
  memcpy( rpcCU->getTransformSkip(TEXT_LUMA)     + m_uiAbsIdxInLCU, m_puhTransformSkip[0], iSizeInUchar );
  memcpy( rpcCU->getTransformSkip(TEXT_CHROMA_U) + m_uiAbsIdxInLCU, m_puhTransformSkip[1], iSizeInUchar );
  memcpy( rpcCU->getTransformSkip(TEXT_CHROMA_V) + m_uiAbsIdxInLCU, m_puhTransformSkip[2], iSizeInUchar );

  memcpy( rpcCU->getCbf(TEXT_LUMA)     + m_uiAbsIdxInLCU, m_puhCbf[0], iSizeInUchar );
  memcpy( rpcCU->getCbf(TEXT_CHROMA_U) + m_uiAbsIdxInLCU, m_puhCbf[1], iSizeInUchar );
  memcpy( rpcCU->getCbf(TEXT_CHROMA_V) + m_uiAbsIdxInLCU, m_puhCbf[2], iSizeInUchar );
  
#if H_3D_DIM
  for( Int i = 0; i < DIM_NUM_TYPE; i++ )
  {
    memcpy( rpcCU->getDimDeltaDC( i, 0 ) + m_uiAbsIdxInLCU, m_dimDeltaDC[i][0], sizeof(Pel ) * m_uiNumPartition );
    memcpy( rpcCU->getDimDeltaDC( i, 1 ) + m_uiAbsIdxInLCU, m_dimDeltaDC[i][1], sizeof(Pel ) * m_uiNumPartition );
  }
#if H_3D_DIM_DMM
  for( Int i = 0; i < DMM_NUM_TYPE; i++ )
  {
    memcpy( rpcCU->getDmmWedgeTabIdx( i ) + m_uiAbsIdxInLCU, m_dmmWedgeTabIdx[i], sizeof(UInt) * m_uiNumPartition );
  }
#endif
#if H_3D_DIM_SDC
  memcpy( rpcCU->getSDCFlag()             + m_uiAbsIdxInLCU, m_pbSDCFlag,      iSizeInBool  );
  memcpy( rpcCU->getSDCSegmentDCOffset(0) + m_uiAbsIdxInLCU, m_apSegmentDCOffset[0], sizeof( Pel ) * m_uiNumPartition);
  memcpy( rpcCU->getSDCSegmentDCOffset(1) + m_uiAbsIdxInLCU, m_apSegmentDCOffset[1], sizeof( Pel ) * m_uiNumPartition);
#endif
#endif
#if H_3D_DBBP
  memcpy( rpcCU->getDBBPFlag()          + m_uiAbsIdxInLCU, m_pbDBBPFlag,          iSizeInBool  );
#endif
  
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
  memcpy( rpcCU->m_sliceStartCU        + m_uiAbsIdxInLCU, m_sliceStartCU,        sizeof( UInt ) * m_uiNumPartition  );
  memcpy( rpcCU->m_sliceSegmentStartCU + m_uiAbsIdxInLCU, m_sliceSegmentStartCU, sizeof( UInt ) * m_uiNumPartition  );
#if H_3D_ARP
  memcpy( rpcCU->getARPW()             + m_uiAbsIdxInLCU, m_puhARPW,             iSizeInUchar );
#endif
#if H_3D_IC
  memcpy( rpcCU->getICFlag()           + m_uiAbsIdxInLCU, m_pbICFlag,            iSizeInBool );
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
  
  Int sizeInChar  = sizeof( Char ) * uiQNumPart;
  memcpy( rpcCU->getSkipFlag()       + uiPartOffset, m_skipFlag,   sizeof( *m_skipFlag )   * uiQNumPart );
#if H_3D_SINGLE_DEPTH
  memcpy( rpcCU->getSingleDepthFlag()  + uiPartOffset, m_singleDepthFlag,    sizeof( *m_singleDepthFlag )   * uiQNumPart );
  memcpy( rpcCU->getSingleDepthValue() + uiPartOffset, m_apSingleDepthValue, sizeof( *m_apSingleDepthValue ) * uiQNumPart);
#endif
  memcpy( rpcCU->getQP() + uiPartOffset, m_phQP, sizeInChar );
  memcpy( rpcCU->getPartitionSize()  + uiPartOffset, m_pePartSize, sizeof( *m_pePartSize ) * uiQNumPart );
  memcpy( rpcCU->getPredictionMode() + uiPartOffset, m_pePredMode, sizeof( *m_pePredMode ) * uiQNumPart );
  memcpy( rpcCU->getCUTransquantBypass()+ uiPartOffset, m_CUTransquantBypass, sizeof( *m_CUTransquantBypass ) * uiQNumPart );
  memcpy( rpcCU->getMergeFlag()         + uiPartOffset, m_pbMergeFlag,         iSizeInBool  );
  memcpy( rpcCU->getMergeIndex()        + uiPartOffset, m_puhMergeIndex,       iSizeInUchar );
#if H_3D_VSP
  memcpy( rpcCU->getVSPFlag()           + uiPartOffset, m_piVSPFlag,           sizeof(Char) * uiQNumPart );
#endif
#if H_3D_SPIVMP
  memcpy( rpcCU->getSPIVMPFlag()        + uiPartOffset, m_pbSPIVMPFlag,        sizeof(Bool) * uiQNumPart );
#endif
  memcpy( rpcCU->getLumaIntraDir()      + uiPartOffset, m_puhLumaIntraDir,     iSizeInUchar );
  memcpy( rpcCU->getChromaIntraDir()    + uiPartOffset, m_puhChromaIntraDir,   iSizeInUchar );
  memcpy( rpcCU->getInterDir()          + uiPartOffset, m_puhInterDir,         iSizeInUchar );
  memcpy( rpcCU->getTransformIdx()      + uiPartOffset, m_puhTrIdx,            iSizeInUchar );
  memcpy( rpcCU->getTransformSkip(TEXT_LUMA)     + uiPartOffset, m_puhTransformSkip[0], iSizeInUchar );
  memcpy( rpcCU->getTransformSkip(TEXT_CHROMA_U) + uiPartOffset, m_puhTransformSkip[1], iSizeInUchar );
  memcpy( rpcCU->getTransformSkip(TEXT_CHROMA_V) + uiPartOffset, m_puhTransformSkip[2], iSizeInUchar );
  memcpy( rpcCU->getCbf(TEXT_LUMA)     + uiPartOffset, m_puhCbf[0], iSizeInUchar );
  memcpy( rpcCU->getCbf(TEXT_CHROMA_U) + uiPartOffset, m_puhCbf[1], iSizeInUchar );
  memcpy( rpcCU->getCbf(TEXT_CHROMA_V) + uiPartOffset, m_puhCbf[2], iSizeInUchar );
  
#if H_3D_DDD
  memcpy( rpcCU->getDDDepth()          + uiPartOffset, m_pucDisparityDerivedDepth,         iSizeInUchar );
  memcpy( rpcCU->getUseDDD()           + uiPartOffset, m_pbUseDDD,                         iSizeInBool );
#endif

#if H_3D_DIM
  for( Int i = 0; i < DMM_NUM_TYPE; i++ )
  {
    memcpy( rpcCU->getDimDeltaDC( i, 0 ) + uiPartOffset, m_dimDeltaDC[i][0], sizeof(Pel ) * uiQNumPart );
    memcpy( rpcCU->getDimDeltaDC( i, 1 ) + uiPartOffset, m_dimDeltaDC[i][1], sizeof(Pel ) * uiQNumPart );
  }
#if H_3D_DIM_DMM
  for( Int i = 0; i < DMM_NUM_TYPE; i++ )
  {
    memcpy( rpcCU->getDmmWedgeTabIdx( i ) + uiPartOffset, m_dmmWedgeTabIdx[i], sizeof(UInt) * uiQNumPart );
  }
#endif
#if H_3D_DIM_SDC
  memcpy( rpcCU->getSDCFlag()             + uiPartOffset, m_pbSDCFlag,      iSizeInBool  );
  memcpy( rpcCU->getSDCSegmentDCOffset(0) + uiPartOffset, m_apSegmentDCOffset[0], sizeof( Pel ) * uiQNumPart);
  memcpy( rpcCU->getSDCSegmentDCOffset(1) + uiPartOffset, m_apSegmentDCOffset[1], sizeof( Pel ) * uiQNumPart);
#endif
#endif
#if H_3D_DBBP
  memcpy( rpcCU->getDBBPFlag()          + uiPartOffset, m_pbDBBPFlag,          iSizeInBool  );
#endif
  
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
  memcpy( rpcCU->m_sliceStartCU        + uiPartOffset, m_sliceStartCU,        sizeof( UInt ) * uiQNumPart  );
  memcpy( rpcCU->m_sliceSegmentStartCU + uiPartOffset, m_sliceSegmentStartCU, sizeof( UInt ) * uiQNumPart  );
#if H_3D_ARP
  memcpy( rpcCU->getARPW()             + uiPartOffset, m_puhARPW,             iSizeInUchar );
#endif
#if H_3D_IC
  memcpy( rpcCU->getICFlag()           + uiPartOffset, m_pbICFlag,            iSizeInBool );
#endif
}

#if H_3D_DDD
Void TComDataCU::setDDDepthSubParts ( UChar ucDDD, UInt uiAbsPartIdx, UInt uiPartIdx, UInt uiDepth )
{
    setSubPart<UChar>( ucDDD, m_pucDisparityDerivedDepth, uiAbsPartIdx, uiDepth, uiPartIdx );
}

Void TComDataCU::setUseDDD        ( Bool bUseDDD, UInt uiAbsPartIdx, UInt uiPartIdx, UInt uiDepth )
{
    setSubPart<Bool>( bUseDDD, m_pbUseDDD, uiAbsPartIdx, uiDepth, uiPartIdx );
}

Void TComDataCU::setUseDDD( Bool bUseDDD, UInt uiAbsPartIdx, UInt uiDepth )
{
    memset( m_pbUseDDD + uiAbsPartIdx, bUseDDD, (m_pcPic->getNumPartInCU() >> ( 2 * uiDepth ))*sizeof(Bool) );
}

#endif

// --------------------------------------------------------------------------------------------------------------------
// Other public functions
// --------------------------------------------------------------------------------------------------------------------

TComDataCU* TComDataCU::getPULeft( UInt& uiLPartUnitIdx, 
                                   UInt uiCurrPartUnitIdx, 
                                   Bool bEnforceSliceRestriction, 
                                   Bool bEnforceTileRestriction )
{
  UInt uiAbsPartIdx       = g_auiZscanToRaster[uiCurrPartUnitIdx];
  UInt uiAbsZorderCUIdx   = g_auiZscanToRaster[m_uiAbsIdxInLCU];
  UInt uiNumPartInCUWidth = m_pcPic->getNumPartInWidth();
  
  if ( !RasterAddress::isZeroCol( uiAbsPartIdx, uiNumPartInCUWidth ) )
  {
    uiLPartUnitIdx = g_auiRasterToZscan[ uiAbsPartIdx - 1 ];
    if ( RasterAddress::isEqualCol( uiAbsPartIdx, uiAbsZorderCUIdx, uiNumPartInCUWidth ) )
    {
      return m_pcPic->getCU( getAddr() );
    }
    else
    {
      uiLPartUnitIdx -= m_uiAbsIdxInLCU;
      return this;
    }
  }
  
  uiLPartUnitIdx = g_auiRasterToZscan[ uiAbsPartIdx + uiNumPartInCUWidth - 1 ];


  if ( (bEnforceSliceRestriction && (m_pcCULeft==NULL || m_pcCULeft->getSlice()==NULL || m_pcCULeft->getSCUAddr()+uiLPartUnitIdx < m_pcPic->getCU( getAddr() )->getSliceStartCU(uiCurrPartUnitIdx)))
      ||
       (bEnforceTileRestriction && ( m_pcCULeft==NULL || m_pcCULeft->getSlice()==NULL || (m_pcPic->getPicSym()->getTileIdxMap( m_pcCULeft->getAddr() ) != m_pcPic->getPicSym()->getTileIdxMap(getAddr()))  )  )
      )
  {
    return NULL;
  }
  return m_pcCULeft;
}

TComDataCU* TComDataCU::getPUAbove( UInt& uiAPartUnitIdx,
                                    UInt uiCurrPartUnitIdx, 
                                    Bool bEnforceSliceRestriction, 
                                    Bool planarAtLCUBoundary ,
                                    Bool bEnforceTileRestriction )
{
  UInt uiAbsPartIdx       = g_auiZscanToRaster[uiCurrPartUnitIdx];
  UInt uiAbsZorderCUIdx   = g_auiZscanToRaster[m_uiAbsIdxInLCU];
  UInt uiNumPartInCUWidth = m_pcPic->getNumPartInWidth();
  
  if ( !RasterAddress::isZeroRow( uiAbsPartIdx, uiNumPartInCUWidth ) )
  {
    uiAPartUnitIdx = g_auiRasterToZscan[ uiAbsPartIdx - uiNumPartInCUWidth ];
    if ( RasterAddress::isEqualRow( uiAbsPartIdx, uiAbsZorderCUIdx, uiNumPartInCUWidth ) )
    {
      return m_pcPic->getCU( getAddr() );
    }
    else
    {
      uiAPartUnitIdx -= m_uiAbsIdxInLCU;
      return this;
    }
  }

  if(planarAtLCUBoundary)
  {
    return NULL;
  }
  
  uiAPartUnitIdx = g_auiRasterToZscan[ uiAbsPartIdx + m_pcPic->getNumPartInCU() - uiNumPartInCUWidth ];

  if ( (bEnforceSliceRestriction && (m_pcCUAbove==NULL || m_pcCUAbove->getSlice()==NULL || m_pcCUAbove->getSCUAddr()+uiAPartUnitIdx < m_pcPic->getCU( getAddr() )->getSliceStartCU(uiCurrPartUnitIdx)))
      ||
       (bEnforceTileRestriction &&(m_pcCUAbove==NULL || m_pcCUAbove->getSlice()==NULL || (m_pcPic->getPicSym()->getTileIdxMap( m_pcCUAbove->getAddr() ) != m_pcPic->getPicSym()->getTileIdxMap(getAddr()))))
      )
  {
    return NULL;
  }
  return m_pcCUAbove;
}

TComDataCU* TComDataCU::getPUAboveLeft( UInt& uiALPartUnitIdx, UInt uiCurrPartUnitIdx, Bool bEnforceSliceRestriction )
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
        return m_pcPic->getCU( getAddr() );
      }
      else
      {
        uiALPartUnitIdx -= m_uiAbsIdxInLCU;
        return this;
      }
    }
    uiALPartUnitIdx = g_auiRasterToZscan[ uiAbsPartIdx + getPic()->getNumPartInCU() - uiNumPartInCUWidth - 1 ];
    if ( (bEnforceSliceRestriction && (m_pcCUAbove==NULL || m_pcCUAbove->getSlice()==NULL ||
       m_pcCUAbove->getSCUAddr()+uiALPartUnitIdx < m_pcPic->getCU( getAddr() )->getSliceStartCU(uiCurrPartUnitIdx)||
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
       ))
     )
    {
      return NULL;
    }
    return m_pcCULeft;
  }
  
  uiALPartUnitIdx = g_auiRasterToZscan[ m_pcPic->getNumPartInCU() - 1 ];
  if ( (bEnforceSliceRestriction && (m_pcCUAboveLeft==NULL || m_pcCUAboveLeft->getSlice()==NULL ||
       m_pcCUAboveLeft->getSCUAddr()+uiALPartUnitIdx < m_pcPic->getCU( getAddr() )->getSliceStartCU(uiCurrPartUnitIdx)||
       (m_pcPic->getPicSym()->getTileIdxMap( m_pcCUAboveLeft->getAddr() ) != m_pcPic->getPicSym()->getTileIdxMap(getAddr()))
       ))
     )
  {
    return NULL;
  }
  return m_pcCUAboveLeft;
}

TComDataCU* TComDataCU::getPUAboveRight( UInt& uiARPartUnitIdx, UInt uiCurrPartUnitIdx, Bool bEnforceSliceRestriction )
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
          return m_pcPic->getCU( getAddr() );
        }
        else
        {
          uiARPartUnitIdx -= m_uiAbsIdxInLCU;
          return this;
        }
      }
      uiARPartUnitIdx = MAX_UINT;
      return NULL;
    }
    uiARPartUnitIdx = g_auiRasterToZscan[ uiAbsPartIdxRT + m_pcPic->getNumPartInCU() - uiNumPartInCUWidth + 1 ];
    if ( (bEnforceSliceRestriction && (m_pcCUAbove==NULL || m_pcCUAbove->getSlice()==NULL ||
       m_pcCUAbove->getSCUAddr()+uiARPartUnitIdx < m_pcPic->getCU( getAddr() )->getSliceStartCU(uiCurrPartUnitIdx)||
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
  if ( (bEnforceSliceRestriction && (m_pcCUAboveRight==NULL || m_pcCUAboveRight->getSlice()==NULL ||
       m_pcPic->getPicSym()->getInverseCUOrderMap( m_pcCUAboveRight->getAddr()) > m_pcPic->getPicSym()->getInverseCUOrderMap( getAddr()) ||
       m_pcCUAboveRight->getSCUAddr()+uiARPartUnitIdx < m_pcPic->getCU( getAddr() )->getSliceStartCU(uiCurrPartUnitIdx)||
       (m_pcPic->getPicSym()->getTileIdxMap( m_pcCUAboveRight->getAddr() ) != m_pcPic->getPicSym()->getTileIdxMap(getAddr()))
       ))
     )
  {
    return NULL;
  }
  return m_pcCUAboveRight;
}

TComDataCU* TComDataCU::getPUBelowLeft( UInt& uiBLPartUnitIdx, UInt uiCurrPartUnitIdx, Bool bEnforceSliceRestriction )
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
          return m_pcPic->getCU( getAddr() );
        }
        else
        {
          uiBLPartUnitIdx -= m_uiAbsIdxInLCU;
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

TComDataCU* TComDataCU::getPUBelowLeftAdi(UInt& uiBLPartUnitIdx,  UInt uiCurrPartUnitIdx, UInt uiPartUnitOffset, Bool bEnforceSliceRestriction )
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
          return m_pcPic->getCU( getAddr() );
        }
        else
        {
          uiBLPartUnitIdx -= m_uiAbsIdxInLCU;
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

TComDataCU* TComDataCU::getPUAboveRightAdi(UInt&  uiARPartUnitIdx, UInt uiCurrPartUnitIdx, UInt uiPartUnitOffset, Bool bEnforceSliceRestriction )
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
          return m_pcPic->getCU( getAddr() );
        }
        else
        {
          uiARPartUnitIdx -= m_uiAbsIdxInLCU;
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
*\returns TComDataCU*   point of TComDataCU of left QpMinCu
*/
TComDataCU* TComDataCU::getQpMinCuLeft( UInt& uiLPartUnitIdx, UInt uiCurrAbsIdxInLCU)
{
  UInt numPartInCUWidth = m_pcPic->getNumPartInWidth();
  UInt absZorderQpMinCUIdx = (uiCurrAbsIdxInLCU>>((g_uiMaxCUDepth - getSlice()->getPPS()->getMaxCuDQPDepth())<<1))<<((g_uiMaxCUDepth -getSlice()->getPPS()->getMaxCuDQPDepth())<<1);
  UInt absRorderQpMinCUIdx = g_auiZscanToRaster[absZorderQpMinCUIdx];

  // check for left LCU boundary
  if ( RasterAddress::isZeroCol(absRorderQpMinCUIdx, numPartInCUWidth) )
  {
    return NULL;
  }

  // get index of left-CU relative to top-left corner of current quantization group
  uiLPartUnitIdx = g_auiRasterToZscan[absRorderQpMinCUIdx - 1];

  // return pointer to current LCU
  return m_pcPic->getCU( getAddr() );
}

/** Get Above QpMinCu
*\param   aPartUnitIdx
*\param   currAbsIdxInLCU
*\returns TComDataCU*   point of TComDataCU of above QpMinCu
*/
TComDataCU* TComDataCU::getQpMinCuAbove( UInt& aPartUnitIdx, UInt currAbsIdxInLCU )
{
  UInt numPartInCUWidth = m_pcPic->getNumPartInWidth();
  UInt absZorderQpMinCUIdx = (currAbsIdxInLCU>>((g_uiMaxCUDepth - getSlice()->getPPS()->getMaxCuDQPDepth())<<1))<<((g_uiMaxCUDepth - getSlice()->getPPS()->getMaxCuDQPDepth())<<1);
  UInt absRorderQpMinCUIdx = g_auiZscanToRaster[absZorderQpMinCUIdx];

  // check for top LCU boundary
  if ( RasterAddress::isZeroRow( absRorderQpMinCUIdx, numPartInCUWidth) )
  {
    return NULL;
  }

  // get index of top-CU relative to top-left corner of current quantization group
  aPartUnitIdx = g_auiRasterToZscan[absRorderQpMinCUIdx - numPartInCUWidth];

  // return pointer to current LCU
  return m_pcPic->getCU( getAddr() );
}

/** Get reference QP from left QpMinCu or latest coded QP
*\param   uiCurrAbsIdxInLCU
*\returns Char   reference QP value
*/
Char TComDataCU::getRefQP( UInt uiCurrAbsIdxInLCU )
{
  UInt        lPartIdx = 0, aPartIdx = 0;
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
  UInt uiQUPartIdxMask = ~((1<<((g_uiMaxCUDepth - getSlice()->getPPS()->getMaxCuDQPDepth())<<1))-1);
  Int iLastValidPartIdx = getLastValidPartIdx( uiAbsPartIdx&uiQUPartIdxMask );
  if ( uiAbsPartIdx < m_uiNumPartition
    && (getSCUAddr()+iLastValidPartIdx < getSliceStartCU(m_uiAbsIdxInLCU+uiAbsPartIdx)))
  {
    return getSlice()->getSliceQp();
  }
  else if ( iLastValidPartIdx >= 0 )
  {
    return getQP( iLastValidPartIdx );
  }
  else
  {
    if ( getZorderIdxInCU() > 0 )
    {
      return getPic()->getCU( getAddr() )->getLastCodedQP( getZorderIdxInCU() );
    }
    else if ( getPic()->getPicSym()->getInverseCUOrderMap(getAddr()) > 0
      && getPic()->getPicSym()->getTileIdxMap(getAddr()) == getPic()->getPicSym()->getTileIdxMap(getPic()->getPicSym()->getCUOrderMap(getPic()->getPicSym()->getInverseCUOrderMap(getAddr())-1))
      && !( getSlice()->getPPS()->getEntropyCodingSyncEnabledFlag() && getAddr() % getPic()->getFrameWidthInCU() == 0 ) )
    {
      return getPic()->getCU( getPic()->getPicSym()->getCUOrderMap(getPic()->getPicSym()->getInverseCUOrderMap(getAddr())-1) )->getLastCodedQP( getPic()->getNumPartInCU() );
    }
    else
    {
      return getSlice()->getSliceQp();
    }
  }
}
/** Check whether the CU is coded in lossless coding mode
 * \param   uiAbsPartIdx
 * \returns true if the CU is coded in lossless coding mode; false if otherwise 
 */
Bool TComDataCU::isLosslessCoded(UInt absPartIdx)
{
  return (getSlice()->getPPS()->getTransquantBypassEnableFlag() && getCUTransquantBypass (absPartIdx));
}

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
  uiModeList[4] = DM_CHROMA_IDX;

  UInt uiLumaMode = getLumaIntraDir( uiAbsPartIdx );

  for( Int i = 0; i < NUM_CHROMA_MODE - 1; i++ )
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
#if H_3D_DIM
  mapDepthModeToIntraDir( iLeftIntraDir );
#endif
  
  // Get intra direction of above PU
  pcTempCU = getPUAbove( uiTempPartIdx, m_uiAbsIdxInLCU + uiAbsPartIdx, true, true );
  
  iAboveIntraDir = pcTempCU ? ( pcTempCU->isIntra( uiTempPartIdx ) ? pcTempCU->getLumaIntraDir( uiTempPartIdx ) : DC_IDX ) : DC_IDX;
#if H_3D_DIM
  mapDepthModeToIntraDir( iAboveIntraDir );
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

UInt TComDataCU::getCtxQtCbf( TextType eType, UInt uiTrDepth )
{
  if( eType )
  {
    return uiTrDepth;
  }
  else
  {
    const UInt uiCtx = ( uiTrDepth == 0 ? 1 : 0 );
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

#if H_3D_ARP
UInt TComDataCU::getCTXARPWFlag( UInt uiAbsPartIdx )
{
  TComDataCU* pcTempCU;
  UInt        uiTempPartIdx;
  UInt        uiCtx = 0;
  
  pcTempCU = getPULeft( uiTempPartIdx, m_uiAbsIdxInLCU + uiAbsPartIdx );
  uiCtx    = ( pcTempCU ) ? ((pcTempCU->getARPW( uiTempPartIdx )==0)?0:1) : 0;
    return uiCtx;
}
#endif
#if H_3D_DBBP
Pel* TComDataCU::getVirtualDepthBlock(UInt uiAbsPartIdx, UInt uiWidth, UInt uiHeight, UInt& uiDepthStride)
{
  // get coded and reconstructed depth view
  TComPicYuv* depthPicYuv = NULL;
  Pel* pDepthPels = NULL;
  
  // DBBP is a texture coding tool
  if( getSlice()->getIsDepth() )
  {
    return NULL;
  }
  
#if H_3D_FCO
  TComPic* depthPic = getSlice()->getIvPic(true, getSlice()->getViewIndex() );
  
  if( depthPic && depthPic->getPicYuvRec() != NULL && depthPic->getIsDepth() )  // depth first
  {
    depthPicYuv = depthPic->getPicYuvRec();
    depthPicYuv->extendPicBorder();
    
    // get collocated depth block for current CU
    uiDepthStride = depthPicYuv->getStride();
    pDepthPels    = depthPicYuv->getLumaAddr( getAddr(), uiAbsPartIdx );
  }
  else  // texture first
#else
  {
    DisInfo DvInfo = getDvInfo(uiAbsPartIdx);
    
    TComPic* baseDepthPic = getSlice()->getIvPic (true, DvInfo.m_aVIdxCan);
    
    if( baseDepthPic == NULL || baseDepthPic->getPicYuvRec() == NULL )
    {
      return NULL;
    }
    
    depthPicYuv   = baseDepthPic->getPicYuvRec();
    depthPicYuv->extendPicBorder();
    uiDepthStride = depthPicYuv->getStride();
    
    Int iBlkX = ( getAddr() % baseDepthPic->getFrameWidthInCU() ) * g_uiMaxCUWidth  + g_auiRasterToPelX[ g_auiZscanToRaster[ getZorderIdxInCU()+uiAbsPartIdx ] ];
    Int iBlkY = ( getAddr() / baseDepthPic->getFrameWidthInCU() ) * g_uiMaxCUHeight + g_auiRasterToPelY[ g_auiZscanToRaster[ getZorderIdxInCU()+uiAbsPartIdx ] ];
    
    Int iPictureWidth  = depthPicYuv->getWidth();
    Int iPictureHeight = depthPicYuv->getHeight();
    
    Int iWidth  = uiWidth;
    Int iHeight = uiHeight;
    
    Bool depthRefineFlag = false;
#if H_3D_NBDV_REF
    depthRefineFlag = m_pcSlice->getVPS()->getDepthRefinementFlag( m_pcSlice->getLayerIdInVps() );
#endif // H_3D_NBDV_REF
    
    TComMv cDv = depthRefineFlag ? DvInfo.m_acDoNBDV : DvInfo.m_acNBDV;
    if( depthRefineFlag )
    {
      cDv.setVer(0);
    }
    
    Int depthPosX = Clip3(0,   iPictureWidth - iWidth,  iBlkX + ((cDv.getHor()+2)>>2));
    Int depthPosY = Clip3(0,   iPictureHeight- iHeight, iBlkY + ((cDv.getVer()+2)>>2));
    
    pDepthPels = depthPicYuv->getLumaAddr() + depthPosX + depthPosY * uiDepthStride;
  }
#endif
  
  AOF( depthPicYuv != NULL );
  AOF( pDepthPels != NULL );
  AOF( uiDepthStride != 0 );
  
  return pDepthPels;
}
#endif

#if H_3D_DBBP
Void TComDataCU::setDBBPFlagSubParts ( Bool bDBBPFlag, UInt uiAbsPartIdx, UInt uiPartIdx, UInt uiDepth )
{
  setSubPart( bDBBPFlag, m_pbDBBPFlag, uiAbsPartIdx, uiDepth, uiPartIdx );
}
#endif

#if H_3D_DIM_SDC
UInt TComDataCU::getCtxSDCFlag( UInt uiAbsPartIdx )
{
  return 0;
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

Void TComDataCU::setPartSizeSubParts( PartSize eMode, UInt uiAbsPartIdx, UInt uiDepth )
{
  assert( sizeof( *m_pePartSize) == 1 );
  memset( m_pePartSize + uiAbsPartIdx, eMode, m_pcPic->getNumPartInCU() >> ( 2 * uiDepth ) );
}

Void TComDataCU::setCUTransquantBypassSubParts( Bool flag, UInt uiAbsPartIdx, UInt uiDepth )
{
  memset( m_CUTransquantBypass + uiAbsPartIdx, flag, m_pcPic->getNumPartInCU() >> ( 2 * uiDepth ) );
}

Void TComDataCU::setSkipFlagSubParts( Bool skip, UInt absPartIdx, UInt depth )
{
  assert( sizeof( *m_skipFlag) == 1 );
  memset( m_skipFlag + absPartIdx, skip, m_pcPic->getNumPartInCU() >> ( 2 * depth ) );
}
#if H_3D_SINGLE_DEPTH
Void TComDataCU::setSingleDepthFlagSubParts( Bool singleDepth, UInt absPartIdx, UInt depth )
{
  assert( sizeof( *m_singleDepthFlag) == 1 );
  memset( m_singleDepthFlag + absPartIdx, singleDepth, m_pcPic->getNumPartInCU() >> ( 2 * depth ) );
}

Void TComDataCU::setSingleDepthValueSubParts(Pel singleDepthValue, UInt uiAbsPartIdx, UInt uiPUIdx, UInt uiDepth )
{
  setSubPartT<Pel>( singleDepthValue, m_apSingleDepthValue, uiAbsPartIdx, uiDepth, uiPUIdx );
}
#endif
Void TComDataCU::setPredModeSubParts( PredMode eMode, UInt uiAbsPartIdx, UInt uiDepth )
{
  assert( sizeof( *m_pePredMode) == 1 );
  memset( m_pePredMode + uiAbsPartIdx, eMode, m_pcPic->getNumPartInCU() >> ( 2 * uiDepth ) );
}

Void TComDataCU::setQPSubCUs( Int qp, TComDataCU* pcCU, UInt absPartIdx, UInt depth, Bool &foundNonZeroCbf )
{
  UInt currPartNumb = m_pcPic->getNumPartInCU() >> (depth << 1);
  UInt currPartNumQ = currPartNumb >> 2;

  if(!foundNonZeroCbf)
  {
    if(pcCU->getDepth(absPartIdx) > depth)
    {
      for ( UInt partUnitIdx = 0; partUnitIdx < 4; partUnitIdx++ )
      {
        pcCU->setQPSubCUs( qp, pcCU, absPartIdx+partUnitIdx*currPartNumQ, depth+1, foundNonZeroCbf );
      }
    }
    else
    {
      if(pcCU->getCbf( absPartIdx, TEXT_LUMA ) || pcCU->getCbf( absPartIdx, TEXT_CHROMA_U ) || pcCU->getCbf( absPartIdx, TEXT_CHROMA_V ) )
      {
        foundNonZeroCbf = true;
      }
      else
      {
        setQPSubParts(qp, absPartIdx, depth);
      }
    }
  }
}

Void TComDataCU::setQPSubParts( Int qp, UInt uiAbsPartIdx, UInt uiDepth )
{
  UInt uiCurrPartNumb = m_pcPic->getNumPartInCU() >> (uiDepth << 1);
  TComSlice * pcSlice = getPic()->getSlice(getPic()->getCurrSliceIdx());

  for(UInt uiSCUIdx = uiAbsPartIdx; uiSCUIdx < uiAbsPartIdx+uiCurrPartNumb; uiSCUIdx++)
  {
    if( m_pcPic->getCU( getAddr() )->getSliceSegmentStartCU(uiSCUIdx+getZorderIdxInCU()) == pcSlice->getSliceSegmentCurStartCUAddr() )
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

#if H_3D_DIM_SDC
Void TComDataCU::setSDCFlagSubParts ( Bool bSDCFlag, UInt uiAbsPartIdx, UInt uiDepth )
{
  assert( sizeof( *m_pbSDCFlag) == 1 );
  memset( m_pbSDCFlag + uiAbsPartIdx, bSDCFlag, m_pcPic->getNumPartInCU() >> ( 2 * uiDepth ) );
}

Bool TComDataCU::getSDCAvailable( UInt uiAbsPartIdx )
{
  // check general CU information
  if( !getSlice()->getIsDepth() || !isIntra(uiAbsPartIdx) || getPartitionSize(uiAbsPartIdx) != SIZE_2Nx2N )
  {
    return false;
  }

  if( isDimMode( getLumaIntraDir( uiAbsPartIdx ) ) )
  {
    return true;
  }
  
  if( getLumaIntraDir( uiAbsPartIdx ) < NUM_INTRA_MODE )
  {
    return true;
  }

  return false;
  // check prediction mode
  UInt uiLumaPredMode = getLumaIntraDir( uiAbsPartIdx );  
  if( uiLumaPredMode == PLANAR_IDX || ( getDimType( uiLumaPredMode ) == DMM1_IDX  ) )
    return true;
  
  // else
  return false;
}
#endif
Void TComDataCU::setMergeFlagSubParts ( Bool bMergeFlag, UInt uiAbsPartIdx, UInt uiPartIdx, UInt uiDepth )
{
  setSubPart( bMergeFlag, m_pbMergeFlag, uiAbsPartIdx, uiDepth, uiPartIdx );
}

Void TComDataCU::setMergeIndexSubParts ( UInt uiMergeIndex, UInt uiAbsPartIdx, UInt uiPartIdx, UInt uiDepth )
{
  setSubPart<UChar>( uiMergeIndex, m_puhMergeIndex, uiAbsPartIdx, uiDepth, uiPartIdx );
}

#if H_3D_SPIVMP
Void TComDataCU::setSPIVMPFlagSubParts( Bool bSPIVMPFlag, UInt uiAbsPartIdx, UInt uiPartIdx, UInt uiDepth )
{
  setSubPart<Bool>( bSPIVMPFlag, m_pbSPIVMPFlag, uiAbsPartIdx, uiDepth, uiPartIdx );
}
#endif

#if H_3D_VSP
Void TComDataCU::setVSPFlagSubParts( Char iVSPFlag, UInt uiAbsPartIdx, UInt uiPartIdx, UInt uiDepth )
{
  setSubPart<Char>( iVSPFlag, m_piVSPFlag, uiAbsPartIdx, uiDepth, uiPartIdx );
}
#if H_3D_VSP
template<typename T>
Void TComDataCU::setSubPartT( T uiParameter, T* puhBaseLCU, UInt uiCUAddr, UInt uiCUDepth, UInt uiPUIdx )
{
  UInt uiCurrPartNumQ = (m_pcPic->getNumPartInCU() >> (2 * uiCUDepth)) >> 2;
  switch ( m_pePartSize[ uiCUAddr ] )
  {
  case SIZE_2Nx2N:
    for (UInt ui = 0; ui < 4 * uiCurrPartNumQ; ui++)
      puhBaseLCU[uiCUAddr + ui] = uiParameter;

    break;
  case SIZE_2NxN:
    for (UInt ui = 0; ui < 2 * uiCurrPartNumQ; ui++)
      puhBaseLCU[uiCUAddr + ui] = uiParameter;
    break;
  case SIZE_Nx2N:
    for (UInt ui = 0; ui < uiCurrPartNumQ; ui++)
      puhBaseLCU[uiCUAddr + ui] = uiParameter;
    for (UInt ui = 0; ui < uiCurrPartNumQ; ui++)
      puhBaseLCU[uiCUAddr + 2 * uiCurrPartNumQ + ui] = uiParameter;
    break;
  case SIZE_NxN:
    for (UInt ui = 0; ui < uiCurrPartNumQ; ui++)
      puhBaseLCU[uiCUAddr + ui] = uiParameter;
    break;
  case SIZE_2NxnU:
    if ( uiPUIdx == 0 )
    {
      for (UInt ui = 0; ui < (uiCurrPartNumQ >> 1); ui++)
        puhBaseLCU[uiCUAddr + ui] = uiParameter;
      for (UInt ui = 0; ui < (uiCurrPartNumQ >> 1); ui++)
        puhBaseLCU[uiCUAddr + uiCurrPartNumQ + ui] = uiParameter;

    }
    else if ( uiPUIdx == 1 )
    {
      for (UInt ui = 0; ui < (uiCurrPartNumQ >> 1); ui++)
        puhBaseLCU[uiCUAddr + ui] = uiParameter;
      for (UInt ui = 0; ui < (uiCurrPartNumQ >> 1) + (uiCurrPartNumQ << 1); ui++)
        puhBaseLCU[uiCUAddr + uiCurrPartNumQ + ui] = uiParameter;

    }
    else
    {
      assert(0);
    }
    break;
  case SIZE_2NxnD:
    if ( uiPUIdx == 0 )
    {
      for (UInt ui = 0; ui < ((uiCurrPartNumQ << 1) + (uiCurrPartNumQ >> 1)); ui++)
        puhBaseLCU[uiCUAddr + ui] = uiParameter;
      for (UInt ui = 0; ui < (uiCurrPartNumQ >> 1); ui++)
        puhBaseLCU[uiCUAddr + (uiCurrPartNumQ << 1) + uiCurrPartNumQ + ui] = uiParameter;

    }
    else if ( uiPUIdx == 1 )
    {
      for (UInt ui = 0; ui < (uiCurrPartNumQ >> 1); ui++)
        puhBaseLCU[uiCUAddr + ui] = uiParameter;
      for (UInt ui = 0; ui < (uiCurrPartNumQ >> 1); ui++)
        puhBaseLCU[uiCUAddr + uiCurrPartNumQ + ui] = uiParameter;

    }
    else
    {
      assert(0);
    }
    break;
  case SIZE_nLx2N:
    if ( uiPUIdx == 0 )
    {
      for (UInt ui = 0; ui < (uiCurrPartNumQ >> 2); ui++)
        puhBaseLCU[uiCUAddr + ui] = uiParameter;
      for (UInt ui = 0; ui < (uiCurrPartNumQ >> 2); ui++)
        puhBaseLCU[uiCUAddr + (uiCurrPartNumQ >> 1) + ui] = uiParameter;
      for (UInt ui = 0; ui < (uiCurrPartNumQ >> 2); ui++)
        puhBaseLCU[uiCUAddr + (uiCurrPartNumQ << 1) + ui] = uiParameter;
      for (UInt ui = 0; ui < (uiCurrPartNumQ >> 2); ui++)
        puhBaseLCU[uiCUAddr + (uiCurrPartNumQ << 1) + (uiCurrPartNumQ >> 1) + ui] = uiParameter;

    }
    else if ( uiPUIdx == 1 )
    {
      for (UInt ui = 0; ui < (uiCurrPartNumQ >> 2); ui++)
        puhBaseLCU[uiCUAddr + ui] = uiParameter;
      for (UInt ui = 0; ui < (uiCurrPartNumQ + (uiCurrPartNumQ >> 2)); ui++)
        puhBaseLCU[uiCUAddr + (uiCurrPartNumQ >> 1) + ui] = uiParameter;
      for (UInt ui = 0; ui < (uiCurrPartNumQ >> 2); ui++)
        puhBaseLCU[uiCUAddr + (uiCurrPartNumQ << 1) + ui] = uiParameter;
      for (UInt ui = 0; ui < (uiCurrPartNumQ + (uiCurrPartNumQ >> 2)); ui++)
        puhBaseLCU[uiCUAddr + (uiCurrPartNumQ << 1) + (uiCurrPartNumQ >> 1) + ui] = uiParameter;

    }
    else
    {
      assert(0);
    }
    break;
  case SIZE_nRx2N:
    if ( uiPUIdx == 0 )
    {
      for (UInt ui = 0; ui < (uiCurrPartNumQ + (uiCurrPartNumQ >> 2)); ui++)
        puhBaseLCU[uiCUAddr + ui] = uiParameter;
      for (UInt ui = 0; ui < (uiCurrPartNumQ >> 2); ui++)
        puhBaseLCU[uiCUAddr + uiCurrPartNumQ + (uiCurrPartNumQ >> 1) + ui] = uiParameter;
      for (UInt ui = 0; ui < (uiCurrPartNumQ + (uiCurrPartNumQ >> 2)); ui++)
        puhBaseLCU[uiCUAddr + (uiCurrPartNumQ << 1) + ui] = uiParameter;
      for (UInt ui = 0; ui < (uiCurrPartNumQ >> 2); ui++)
        puhBaseLCU[uiCUAddr + (uiCurrPartNumQ << 1) + uiCurrPartNumQ + (uiCurrPartNumQ >> 1) + ui] = uiParameter;

    }
    else if ( uiPUIdx == 1 )
    {
      for (UInt ui = 0; ui < (uiCurrPartNumQ >> 2); ui++)
        puhBaseLCU[uiCUAddr + ui] = uiParameter;
      for (UInt ui = 0; ui < (uiCurrPartNumQ >> 2); ui++)
        puhBaseLCU[uiCUAddr + (uiCurrPartNumQ >> 1) + ui] = uiParameter;
      for (UInt ui = 0; ui < (uiCurrPartNumQ >> 2); ui++)
        puhBaseLCU[uiCUAddr + (uiCurrPartNumQ << 1) + ui] = uiParameter;
      for (UInt ui = 0; ui < (uiCurrPartNumQ >> 2); ui++)
        puhBaseLCU[uiCUAddr + (uiCurrPartNumQ << 1) + (uiCurrPartNumQ >> 1) + ui] = uiParameter;

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
#endif
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

Void TComDataCU::setTransformSkipSubParts( UInt useTransformSkipY, UInt useTransformSkipU, UInt useTransformSkipV, UInt uiAbsPartIdx, UInt uiDepth )
{
  UInt uiCurrPartNumb = m_pcPic->getNumPartInCU() >> (uiDepth << 1);

  memset( m_puhTransformSkip[0] + uiAbsPartIdx, useTransformSkipY, sizeof( UChar ) * uiCurrPartNumb );
  memset( m_puhTransformSkip[1] + uiAbsPartIdx, useTransformSkipU, sizeof( UChar ) * uiCurrPartNumb );
  memset( m_puhTransformSkip[2] + uiAbsPartIdx, useTransformSkipV, sizeof( UChar ) * uiCurrPartNumb );
}

Void TComDataCU::setTransformSkipSubParts( UInt useTransformSkip, TextType eType, UInt uiAbsPartIdx, UInt uiDepth)
{
  UInt uiCurrPartNumb = m_pcPic->getNumPartInCU() >> (uiDepth << 1);

  memset( m_puhTransformSkip[g_aucConvertTxtTypeToIdx[eType]] + uiAbsPartIdx, useTransformSkip, sizeof( UChar ) * uiCurrPartNumb );
}

Void TComDataCU::setSizeSubParts( UInt uiWidth, UInt uiHeight, UInt uiAbsPartIdx, UInt uiDepth )
{
  UInt uiCurrPartNumb = m_pcPic->getNumPartInCU() >> (uiDepth << 1);
  
  memset( m_puhWidth  + uiAbsPartIdx, uiWidth,  sizeof(UChar)*uiCurrPartNumb );
  memset( m_puhHeight + uiAbsPartIdx, uiHeight, sizeof(UChar)*uiCurrPartNumb );
}

UChar TComDataCU::getNumPartitions()
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

#if H_3D_IC
Void TComDataCU::getPartIndexAndSize( UInt uiPartIdx, UInt& ruiPartAddr, Int& riWidth, Int& riHeight, UInt uiAbsPartIdx, Bool bLCU)
{
  UInt uiNumPartition  = bLCU ? (getWidth(uiAbsPartIdx)*getHeight(uiAbsPartIdx) >> 4) : m_uiNumPartition;
  UInt  uiTmpAbsPartIdx  = bLCU ? uiAbsPartIdx : 0;

  switch ( m_pePartSize[uiTmpAbsPartIdx] )
  {
  case SIZE_2NxN:
    riWidth = getWidth( uiTmpAbsPartIdx );      riHeight = getHeight( uiTmpAbsPartIdx ) >> 1; ruiPartAddr = ( uiPartIdx == 0 )? 0 : uiNumPartition >> 1;
    break;
  case SIZE_Nx2N:
    riWidth = getWidth( uiTmpAbsPartIdx ) >> 1; riHeight = getHeight( uiTmpAbsPartIdx );      ruiPartAddr = ( uiPartIdx == 0 )? 0 : uiNumPartition >> 2;
    break;
  case SIZE_NxN:
    riWidth = getWidth( uiTmpAbsPartIdx ) >> 1; riHeight = getHeight( uiTmpAbsPartIdx ) >> 1; ruiPartAddr = ( uiNumPartition >> 2 ) * uiPartIdx;
    break;
  case SIZE_2NxnU:
    riWidth     = getWidth( uiTmpAbsPartIdx );
    riHeight    = ( uiPartIdx == 0 ) ?  getHeight( uiTmpAbsPartIdx ) >> 2 : ( getHeight( uiTmpAbsPartIdx ) >> 2 ) + ( getHeight( uiTmpAbsPartIdx ) >> 1 );
    ruiPartAddr = ( uiPartIdx == 0 ) ? 0 : uiNumPartition >> 3;
    break;
  case SIZE_2NxnD:
    riWidth     = getWidth( uiTmpAbsPartIdx );
    riHeight    = ( uiPartIdx == 0 ) ?  ( getHeight( uiTmpAbsPartIdx ) >> 2 ) + ( getHeight( uiTmpAbsPartIdx ) >> 1 ) : getHeight( uiTmpAbsPartIdx ) >> 2;
    ruiPartAddr = ( uiPartIdx == 0 ) ? 0 : (uiNumPartition >> 1) + (uiNumPartition >> 3);
    break;
  case SIZE_nLx2N:
    riWidth     = ( uiPartIdx == 0 ) ? getWidth( uiTmpAbsPartIdx ) >> 2 : ( getWidth( uiTmpAbsPartIdx ) >> 2 ) + ( getWidth( uiTmpAbsPartIdx ) >> 1 );
    riHeight    = getHeight( uiTmpAbsPartIdx );
    ruiPartAddr = ( uiPartIdx == 0 ) ? 0 : uiNumPartition >> 4;
    break;
  case SIZE_nRx2N:
    riWidth     = ( uiPartIdx == 0 ) ? ( getWidth( uiTmpAbsPartIdx ) >> 2 ) + ( getWidth( uiTmpAbsPartIdx ) >> 1 ) : getWidth( uiTmpAbsPartIdx ) >> 2;
    riHeight    = getHeight( uiTmpAbsPartIdx );
    ruiPartAddr = ( uiPartIdx == 0 ) ? 0 : (uiNumPartition >> 2) + (uiNumPartition >> 4);
    break;
  default:
    assert ( m_pePartSize[uiTmpAbsPartIdx] == SIZE_2Nx2N ); 
    riWidth = getWidth( uiTmpAbsPartIdx );      riHeight = getHeight( uiTmpAbsPartIdx );      ruiPartAddr = 0;
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

Void TComDataCU::deriveLeftRightTopIdxGeneral ( UInt uiAbsPartIdx, UInt uiPartIdx, UInt& ruiPartIdxLT, UInt& ruiPartIdxRT )
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

Void TComDataCU::deriveLeftBottomIdxGeneral( UInt uiAbsPartIdx, UInt uiPartIdx, UInt& ruiPartIdxLB )
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

Void TComDataCU::deriveLeftRightTopIdx ( UInt uiPartIdx, UInt& ruiPartIdxLT, UInt& ruiPartIdxRT )
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

Void TComDataCU::deriveLeftBottomIdx( UInt  uiPartIdx,      UInt&      ruiPartIdxLB )
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
Void TComDataCU::deriveRightBottomIdx( UInt  uiPartIdx,      UInt&      ruiPartIdxRB )
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

#if H_3D_VSP

/** Add a VSP merging candidate
 * \Inputs
 * \param uiPUIdx: PU index within a CU
 * \param ucVspMergePos: Specify the VSP merge candidate position
 * \param mrgCandIdx: Target merge candidate index. At encoder, it is set equal to -1, such that the whole merge candidate list will be constructed.
 * \param pDinfo: The "disparity information" derived from neighboring blocks. Type 1 MV.
 * \param uiCount: The next position to add VSP merge candidate
 *
 * \Outputs
 * \param uiCount: The next position to add merge candidate. Will be updated if VSP is successfully added
 * \param abCandIsInter: abCandIsInter[iCount] tells that VSP candidate is an Inter candidate, if VSP is successfully added
 * \param pcMvFieldNeighbours:   Return combined motion information, then stored to a global buffer
 *                                    1) the "disparity vector". Type 1 MV. To be used to fetch a depth block.
 *                                    2) the ref index /list.    Type 2 reference picture pointer, typically for texture
 * \param puhInterDirNeighbours: Indicate the VSP prediction direction.
 * \param vspFlag: vspFlag[iCount] will be set (equal to 1), if VSP is successfully added. To be used to indicate the actual position of the VSP candidate
 *
 * \Return
 *   true:  if the VSP candidate is added at the target position
 *   false: otherwise
 */
inline Bool TComDataCU::xAddVspCand( Int mrgCandIdx, DisInfo* pDInfo, Int& iCount)
{
  if ( m_pcSlice->getViewIndex() == 0 || !m_pcSlice->getVPS()->getViewSynthesisPredFlag( m_pcSlice->getLayerIdInVps() ) || m_pcSlice->getIsDepth() || pDInfo->m_aVIdxCan == -1)
  {
    return false;
  }

  Int refViewIdx = pDInfo->m_aVIdxCan;
  TComPic* picDepth = getSlice()->getIvPic( true, refViewIdx );

  if( picDepth == NULL ) // No depth reference avail
  {
    // Is this allowed to happen? When not an assertion should be added here!
    return false;
  }

  TComMvField mvVSP[2];
  UChar dirVSP;
  Bool  refViewAvailFlag = false;
  UChar predFlag[2]      = {0, 0};

  for( Int iRefListIdX = 0; iRefListIdX < 2 && !refViewAvailFlag; iRefListIdX++ )
  {
    RefPicList eRefPicListX = RefPicList( iRefListIdX );
    for ( Int i = 0; i < m_pcSlice->getNumRefIdx(eRefPicListX) && !refViewAvailFlag; i++ )
    {
      Int viewIdxRefInListX = m_pcSlice->getRefPic(eRefPicListX, i)->getViewIndex();
      if ( viewIdxRefInListX == refViewIdx )
      {
        refViewAvailFlag      = true;
        predFlag[iRefListIdX] = 1;
        mvVSP[0+iRefListIdX].setMvField( pDInfo->m_acNBDV, i );
#if H_3D_NBDV
        mvVSP[0+iRefListIdX].getMv().setIDVFlag (false);
#endif
      }
    }
  }

  dirVSP = (predFlag[0] | (predFlag[1] << 1));
  m_mergCands[MRG_VSP].setCand( mvVSP, dirVSP, true, false);
  if ( mrgCandIdx == iCount )
  {
    return true;
  }

  iCount++;

  return false;
}

#endif

#if H_3D_IV_MERGE
inline Bool TComDataCU::xAddIvMRGCand( Int mrgCandIdx, Int& iCount, Int* ivCandDir, TComMv* ivCandMv, Int* ivCandRefIdx )
{
  for(Int iLoop = 0; iLoop < 2; iLoop ++ ) 
  {
    /// iLoop = 0 --> IvMCShift
    /// iLoop = 1 --> IvDCShift  (Derived from IvDC)
    if(ivCandDir[iLoop + 2])
    {
      TComMvField tmpMV[2];
      UChar tmpDir = ivCandDir[iLoop + 2];
      if( ( ivCandDir[iLoop + 2] & 1 ) == 1 )
      {
        tmpMV[0].setMvField( ivCandMv[ (iLoop<<1) + 4 ], ivCandRefIdx[ (iLoop<<1) + 4 ] ); 
      }
      if( ( ivCandDir[iLoop + 2] & 2 ) == 2 )
      {
        tmpMV[1].setMvField( ivCandMv[ (iLoop<<1) + 5 ], ivCandRefIdx[ (iLoop<<1) + 5 ] );
      }
     
      // Prune IvMC vs. IvMcShift
      Bool bRemove = false;      
      if( !iLoop && ivCandDir[0] > 0)
      {
        if(tmpDir == m_mergCands[MRG_IVMC].m_uDir && m_mergCands[MRG_IVMC].m_cMvField[0]==tmpMV[0] && m_mergCands[MRG_IVMC].m_cMvField[1]==tmpMV[1])
        {
            bRemove                         = true;
        }
      }
      if(!bRemove)
      {
#if H_3D_NBDV
        if(iLoop) // For IvMcShift candidate
        {
          tmpMV[0].getMv().setIDVFlag (false);
          tmpMV[1].getMv().setIDVFlag (false);
        }
#endif
        m_mergCands[MRG_IVSHIFT].setCand(tmpMV, tmpDir, false, false);
        if( mrgCandIdx == iCount )
        {
          return true;
        }
        iCount++;
      }
      break;
    }
  }
  return false;
} 

#endif

#if H_3D
Void TComDataCU::buildMCL(TComMvField* pcMvFieldNeighbours, UChar* puhInterDirNeighbours
#if H_3D_VSP
  , Int* vspFlag
#endif
#if H_3D_SPIVMP
  , Bool* pbSPIVMPFlag
#endif
  , Int& numValidMergeCand
  )
{
  if (!( getSlice()->getIsDepth() || getSlice()->getViewIndex()>0))  // for only dependent texture
  {
    return;
  }

  Int iCount = 0;
  TComMv cZeroMv;

  // init temporal list
  TComMvField extMergeCandList[MRG_MAX_NUM_CANDS_MEM << 1];
  UChar uhInterDirNeighboursExt[MRG_MAX_NUM_CANDS_MEM];
  for( UInt ui = 0; ui < getSlice()->getMaxNumMergeCand(); ++ui )
  {
    uhInterDirNeighboursExt[ui] = puhInterDirNeighbours[ui];
    extMergeCandList[ui<<1].setMvField(cZeroMv, NOT_VALID);
    extMergeCandList[(ui<<1)+1].setMvField(cZeroMv, NOT_VALID);
    vspFlag[ui] = 0;
  }

  // add candidates to temporal list
  // insert MPI ... IvShift candidate
  for (Int i=0; i<=MRG_IVSHIFT; i++)
  {
    if (m_mergCands[i].m_bAvailable)
    {
      m_mergCands[i].getCand(iCount, extMergeCandList, uhInterDirNeighboursExt, vspFlag, pbSPIVMPFlag);
      iCount++;
      if (iCount >= getSlice()->getMaxNumMergeCand())
        break;
    }
  }

  // insert remaining base candidates
  while (iCount < getSlice()->getMaxNumMergeCand() && m_baseListidc < getSlice()->getMaxNumMergeCand())
  {
    uhInterDirNeighboursExt[iCount] = puhInterDirNeighbours[m_baseListidc];
    extMergeCandList[iCount<<1].setMvField(pcMvFieldNeighbours[m_baseListidc<<1].getMv(), pcMvFieldNeighbours[m_baseListidc<<1].getRefIdx());
    if ( getSlice()->isInterB() )
    {
      extMergeCandList[(iCount<<1)+1].setMvField(pcMvFieldNeighbours[(m_baseListidc<<1)+1].getMv(), pcMvFieldNeighbours[(m_baseListidc<<1)+1].getRefIdx());
    }
    m_baseListidc++;
    iCount++;
  }

  for( UInt ui = 0; ui < getSlice()->getMaxNumMergeCand(); ui++ )
  {
    puhInterDirNeighbours[ui] = 0;
    pcMvFieldNeighbours[ui<<1].setMvField(cZeroMv, NOT_VALID);
    pcMvFieldNeighbours[(ui<<1)+1].setMvField(cZeroMv, NOT_VALID);
  }
  // copy extMergeCandList to output
  for( UInt ui = 0; ui < getSlice()->getMaxNumMergeCand(); ui++ )
  {
    puhInterDirNeighbours[ui] = uhInterDirNeighboursExt[ui];
    pcMvFieldNeighbours[ui<<1].setMvField(extMergeCandList[ui<<1].getMv(), extMergeCandList[ui<<1].getRefIdx());
    if ( getSlice()->isInterB() )
      pcMvFieldNeighbours[(ui<<1)+1].setMvField(extMergeCandList[(ui<<1)+1].getMv(), extMergeCandList[(ui<<1)+1].getRefIdx());
  }
  numValidMergeCand = iCount;
  assert(iCount == getSlice()->getMaxNumMergeCand());
}

/** Constructs a list of merging candidates
 * \param uiAbsPartIdx
 * \param uiPUIdx 
 * \param uiDepth
 * \param pcMvFieldNeighbours
 * \param puhInterDirNeighbours
 * \param numValidMergeCand
 */
// HM 12.0 based merge candidate list construction

Void TComDataCU::getInterMergeCandidates( UInt uiAbsPartIdx, UInt uiPUIdx, TComMvField* pcMvFieldNeighbours, UChar* puhInterDirNeighbours, Int& numValidMergeCand, Int mrgCandIdx )
{

  UInt uiAbsPartAddr = m_uiAbsIdxInLCU + uiAbsPartIdx;
  Bool abCandIsInter[ MRG_MAX_NUM_CANDS_MEM ];
  TComMv cZeroMv;
  for( UInt ui = 0; ui < getSlice()->getMaxNumMergeCand(); ++ui )
  {
    abCandIsInter[ui] = false;
    pcMvFieldNeighbours[ ( ui << 1 )     ].setMvField(cZeroMv, NOT_VALID);
    pcMvFieldNeighbours[ ( ui << 1 ) + 1 ].setMvField(cZeroMv, NOT_VALID);
  }
  numValidMergeCand = getSlice()->getMaxNumMergeCand();
  // compute the location of the current PU
  Int xP, yP, nPSW, nPSH;
  this->getPartPosition(uiPUIdx, xP, yP, nPSW, nPSH);

  Int iCount = 0;

  UInt uiPartIdxLT, uiPartIdxRT, uiPartIdxLB;
  PartSize cCurPS = getPartitionSize( uiAbsPartIdx );
  deriveLeftRightTopIdxGeneral( uiAbsPartIdx, uiPUIdx, uiPartIdxLT, uiPartIdxRT );
  deriveLeftBottomIdxGeneral  ( uiAbsPartIdx, uiPUIdx, uiPartIdxLB );

  //left
  UInt uiLeftPartIdx = 0;
  TComDataCU* pcCULeft = 0;
  pcCULeft = getPULeft( uiLeftPartIdx, uiPartIdxLB );
  Bool isAvailableA1 = pcCULeft &&
    pcCULeft->isDiffMER(xP -1, yP+nPSH-1, xP, yP) &&
    !( uiPUIdx == 1 && (cCurPS == SIZE_Nx2N || cCurPS == SIZE_nLx2N || cCurPS == SIZE_nRx2N) ) &&
    !pcCULeft->isIntra( uiLeftPartIdx ) ;
  if ( isAvailableA1 )
  {
    m_bAvailableFlagA1 = 1;
    abCandIsInter[iCount] = true;
    // get Inter Dir
    puhInterDirNeighbours[iCount] = pcCULeft->getInterDir( uiLeftPartIdx );
    // get Mv from Left
    pcCULeft->getMvField( pcCULeft, uiLeftPartIdx, REF_PIC_LIST_0, pcMvFieldNeighbours[iCount<<1] );
    if ( getSlice()->isInterB() )
    {
      pcCULeft->getMvField( pcCULeft, uiLeftPartIdx, REF_PIC_LIST_1, pcMvFieldNeighbours[(iCount<<1)+1] );
    }

    iCount ++;
  }
  
  // early termination
  if (iCount == getSlice()->getMaxNumMergeCand()) 
  {
    return;
  }
  // above
  UInt uiAbovePartIdx = 0;
  TComDataCU* pcCUAbove = 0;
  pcCUAbove = getPUAbove( uiAbovePartIdx, uiPartIdxRT );
  Bool isAvailableB1 = pcCUAbove &&
  pcCUAbove->isDiffMER(xP+nPSW-1, yP-1, xP, yP) &&
  !( uiPUIdx == 1 && (cCurPS == SIZE_2NxN || cCurPS == SIZE_2NxnU || cCurPS == SIZE_2NxnD) ) &&
  !pcCUAbove->isIntra( uiAbovePartIdx );
  if ( isAvailableB1 && (!isAvailableA1 || !pcCULeft->hasEqualMotion( uiLeftPartIdx, pcCUAbove, uiAbovePartIdx ) ) )
  {
    m_bAvailableFlagB1 = 1;
    abCandIsInter[iCount] = true;
    // get Inter Dir
    puhInterDirNeighbours[iCount] = pcCUAbove->getInterDir( uiAbovePartIdx );
    // get Mv from Left
    pcCUAbove->getMvField( pcCUAbove, uiAbovePartIdx, REF_PIC_LIST_0, pcMvFieldNeighbours[iCount<<1] );
    if ( getSlice()->isInterB() )
    {
      pcCUAbove->getMvField( pcCUAbove, uiAbovePartIdx, REF_PIC_LIST_1, pcMvFieldNeighbours[(iCount<<1)+1] );
    }
    if ( mrgCandIdx == iCount )
    {
      return;
    }
    iCount ++;
  }
  // early termination
  if (iCount == getSlice()->getMaxNumMergeCand()) 
  {
    return;
  }

  // above right
  UInt uiAboveRightPartIdx = 0;
  TComDataCU* pcCUAboveRight = 0;
  pcCUAboveRight = getPUAboveRight( uiAboveRightPartIdx, uiPartIdxRT );
  Bool isAvailableB0 = pcCUAboveRight &&
  pcCUAboveRight->isDiffMER(xP+nPSW, yP-1, xP, yP) &&
  !pcCUAboveRight->isIntra( uiAboveRightPartIdx );
  if ( isAvailableB0 && ( !isAvailableB1 || !pcCUAbove->hasEqualMotion( uiAbovePartIdx, pcCUAboveRight, uiAboveRightPartIdx ) ) )
  {
    m_bAvailableFlagB0 = 1;
    abCandIsInter[iCount] = true;
    // get Inter Dir
    puhInterDirNeighbours[iCount] = pcCUAboveRight->getInterDir( uiAboveRightPartIdx );
    // get Mv from Left
    pcCUAboveRight->getMvField( pcCUAboveRight, uiAboveRightPartIdx, REF_PIC_LIST_0, pcMvFieldNeighbours[iCount<<1] );
    if ( getSlice()->isInterB() )
    {
      pcCUAboveRight->getMvField( pcCUAboveRight, uiAboveRightPartIdx, REF_PIC_LIST_1, pcMvFieldNeighbours[(iCount<<1)+1] );
    }
    if ( mrgCandIdx == iCount )
    {
      return;
    }
    iCount ++;
  }
  // early termination
  if (iCount == getSlice()->getMaxNumMergeCand()) 
  {
    return;
  }

  //left bottom
  UInt uiLeftBottomPartIdx = 0;
  TComDataCU* pcCULeftBottom = 0;
  pcCULeftBottom = this->getPUBelowLeft( uiLeftBottomPartIdx, uiPartIdxLB );
  Bool isAvailableA0 = pcCULeftBottom &&
  pcCULeftBottom->isDiffMER(xP-1, yP+nPSH, xP, yP) &&
  !pcCULeftBottom->isIntra( uiLeftBottomPartIdx ) ;
  if ( isAvailableA0 && ( !isAvailableA1 || !pcCULeft->hasEqualMotion( uiLeftPartIdx, pcCULeftBottom, uiLeftBottomPartIdx ) ) )
  {
    m_bAvailableFlagA0 = 1;
    abCandIsInter[iCount] = true;
    // get Inter Dir
    puhInterDirNeighbours[iCount] = pcCULeftBottom->getInterDir( uiLeftBottomPartIdx );
    // get Mv from Left
    pcCULeftBottom->getMvField( pcCULeftBottom, uiLeftBottomPartIdx, REF_PIC_LIST_0, pcMvFieldNeighbours[iCount<<1] );
    if ( getSlice()->isInterB() )
    {
      pcCULeftBottom->getMvField( pcCULeftBottom, uiLeftBottomPartIdx, REF_PIC_LIST_1, pcMvFieldNeighbours[(iCount<<1)+1] );
    }
    if ( mrgCandIdx == iCount )
    {
      return;
    }
    iCount ++;
  }
  // early termination
  if (iCount == getSlice()->getMaxNumMergeCand()) 
  {
    return;
  }
  // above left 
  if( iCount < 4 )
  {
    UInt uiAboveLeftPartIdx = 0;
    TComDataCU* pcCUAboveLeft = 0;
    pcCUAboveLeft = getPUAboveLeft( uiAboveLeftPartIdx, uiAbsPartAddr );
    Bool isAvailableB2 = pcCUAboveLeft &&
    pcCUAboveLeft->isDiffMER(xP-1, yP-1, xP, yP) &&
    !pcCUAboveLeft->isIntra( uiAboveLeftPartIdx );
    if ( isAvailableB2 && ( !isAvailableA1 || !pcCULeft->hasEqualMotion( uiLeftPartIdx, pcCUAboveLeft, uiAboveLeftPartIdx ) )
        && ( !isAvailableB1 || !pcCUAbove->hasEqualMotion( uiAbovePartIdx, pcCUAboveLeft, uiAboveLeftPartIdx ) ) )
    {
      m_bAvailableFlagB2 = 1;
      abCandIsInter[iCount] = true;
      // get Inter Dir
      puhInterDirNeighbours[iCount] = pcCUAboveLeft->getInterDir( uiAboveLeftPartIdx );
      // get Mv from Left
      pcCUAboveLeft->getMvField( pcCUAboveLeft, uiAboveLeftPartIdx, REF_PIC_LIST_0, pcMvFieldNeighbours[iCount<<1] );
      if ( getSlice()->isInterB() )
      {
        pcCUAboveLeft->getMvField( pcCUAboveLeft, uiAboveLeftPartIdx, REF_PIC_LIST_1, pcMvFieldNeighbours[(iCount<<1)+1] );
      }
      if ( mrgCandIdx == iCount )
      {
        return;
      }
      iCount ++;
    }
  }
  // early termination
  if (iCount == getSlice()->getMaxNumMergeCand()) 
  {
    return;
  }
  if ( getSlice()->getEnableTMVPFlag())
  {
    //>> MTK colocated-RightBottom
    UInt uiPartIdxRB;

    deriveRightBottomIdx( uiPUIdx, uiPartIdxRB );  

    UInt uiAbsPartIdxTmp = g_auiZscanToRaster[uiPartIdxRB];
    UInt uiNumPartInCUWidth = m_pcPic->getNumPartInWidth();

    TComMv cColMv;
    Int iRefIdx;
    Int uiLCUIdx = -1;

    if      ( ( m_pcPic->getCU(m_uiCUAddr)->getCUPelX() + g_auiRasterToPelX[uiAbsPartIdxTmp] + m_pcPic->getMinCUWidth() ) >= m_pcSlice->getSPS()->getPicWidthInLumaSamples() )  // image boundary check
    {
    }
    else if ( ( m_pcPic->getCU(m_uiCUAddr)->getCUPelY() + g_auiRasterToPelY[uiAbsPartIdxTmp] + m_pcPic->getMinCUHeight() ) >= m_pcSlice->getSPS()->getPicHeightInLumaSamples() )
    {
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
      }
      else if ( uiAbsPartIdxTmp / uiNumPartInCUWidth < m_pcPic->getNumPartInHeight() - 1 ) // is not at the last row of LCU But is last column of LCU
      {
        uiAbsPartAddr = g_auiRasterToZscan[ uiAbsPartIdxTmp + 1 ];
        uiLCUIdx = getAddr() + 1;
      }
      else //is the right bottom corner of LCU                       
      {
        uiAbsPartAddr = 0;
      }
    }

    iRefIdx = 0;
    Bool bExistMV = false;
    UInt uiPartIdxCenter;
    UInt uiCurLCUIdx = getAddr();
    Int dir = 0;
    UInt uiArrayAddr = iCount;
    xDeriveCenterIdx( uiPUIdx, uiPartIdxCenter );
    bExistMV = uiLCUIdx >= 0 && xGetColMVP( REF_PIC_LIST_0, uiLCUIdx, uiAbsPartAddr, cColMv, iRefIdx );
    if( bExistMV == false )
    {
      bExistMV = xGetColMVP( REF_PIC_LIST_0, uiCurLCUIdx, uiPartIdxCenter, cColMv, iRefIdx );
    }
    if( bExistMV )
    {
      dir |= 1;
      pcMvFieldNeighbours[ 2 * uiArrayAddr ].setMvField( cColMv, iRefIdx );
    }

    if ( getSlice()->isInterB() )
    {
#if H_3D_TMVP
      iRefIdx = 0;
#endif
      bExistMV = uiLCUIdx >= 0 && xGetColMVP( REF_PIC_LIST_1, uiLCUIdx, uiAbsPartAddr, cColMv, iRefIdx);
      if( bExistMV == false )
      {
        bExistMV = xGetColMVP( REF_PIC_LIST_1, uiCurLCUIdx, uiPartIdxCenter, cColMv, iRefIdx );
      }
      if( bExistMV )
      {
        dir |= 2;
        pcMvFieldNeighbours[ 2 * uiArrayAddr + 1 ].setMvField( cColMv, iRefIdx );
      }
    }

    if (dir != 0)
    {
      puhInterDirNeighbours[uiArrayAddr] = dir;
      abCandIsInter[uiArrayAddr] = true;
#if H_3D_NBDV
      pcMvFieldNeighbours[iCount<<1    ].getMv().setIDVFlag (false);
      pcMvFieldNeighbours[(iCount<<1)+1].getMv().setIDVFlag (false);
#endif
      if ( mrgCandIdx == iCount )
      {
        return;
      }
      iCount++;
    }
  }
  // early termination
  if (iCount == getSlice()->getMaxNumMergeCand()) 
  {
    return;
  }
  UInt uiArrayAddr = iCount;
  UInt uiCutoff = uiArrayAddr;
  
  if ( getSlice()->isInterB() && iCount<5)  // JCT3V-F0129 by Qualcomm
  {
    UInt uiPriorityList0[12] = {0 , 1, 0, 2, 1, 2, 0, 3, 1, 3, 2, 3};
    UInt uiPriorityList1[12] = {1 , 0, 2, 0, 2, 1, 3, 0, 3, 1, 3, 2};

    for (Int idx=0; idx<uiCutoff*(uiCutoff-1) && uiArrayAddr!= getSlice()->getMaxNumMergeCand(); idx++)
    {
      Int i = uiPriorityList0[idx]; Int j = uiPriorityList1[idx];
      if (abCandIsInter[i] && abCandIsInter[j]&& (puhInterDirNeighbours[i]&0x1)&&(puhInterDirNeighbours[j]&0x2))
      {
        abCandIsInter[uiArrayAddr] = true;
        puhInterDirNeighbours[uiArrayAddr] = 3;

        // get Mv from cand[i] and cand[j]
        pcMvFieldNeighbours[uiArrayAddr << 1].setMvField(pcMvFieldNeighbours[i<<1].getMv(), pcMvFieldNeighbours[i<<1].getRefIdx());
        pcMvFieldNeighbours[( uiArrayAddr << 1 ) + 1].setMvField(pcMvFieldNeighbours[(j<<1)+1].getMv(), pcMvFieldNeighbours[(j<<1)+1].getRefIdx());

        Int iRefPOCL0 = m_pcSlice->getRefPOC( REF_PIC_LIST_0, pcMvFieldNeighbours[(uiArrayAddr<<1)].getRefIdx() );
        Int iRefPOCL1 = m_pcSlice->getRefPOC( REF_PIC_LIST_1, pcMvFieldNeighbours[(uiArrayAddr<<1)+1].getRefIdx() );
        if (iRefPOCL0 == iRefPOCL1 && pcMvFieldNeighbours[(uiArrayAddr<<1)].getMv() == pcMvFieldNeighbours[(uiArrayAddr<<1)+1].getMv())
        {
          abCandIsInter[uiArrayAddr] = false;
        }
        else
        {
          uiArrayAddr++;
        }
      }
    }
  }
  // early termination
  if (uiArrayAddr == getSlice()->getMaxNumMergeCand()) 
  {
    return;
  }
  
  Int iNumRefIdx = (getSlice()->isInterB()) ? min(m_pcSlice->getNumRefIdx(REF_PIC_LIST_0), m_pcSlice->getNumRefIdx(REF_PIC_LIST_1)) : m_pcSlice->getNumRefIdx(REF_PIC_LIST_0);
  Int r = 0;
  Int refcnt = 0;
  while (uiArrayAddr < getSlice()->getMaxNumMergeCand())
  {
    abCandIsInter[uiArrayAddr] = true;
    puhInterDirNeighbours[uiArrayAddr] = 1;
    pcMvFieldNeighbours[uiArrayAddr << 1].setMvField( TComMv(0, 0), r);

    if ( getSlice()->isInterB() )
    {
      puhInterDirNeighbours[uiArrayAddr] = 3;
      pcMvFieldNeighbours[(uiArrayAddr << 1) + 1].setMvField(TComMv(0, 0), r);
    }
    uiArrayAddr++;
    if ( refcnt == iNumRefIdx - 1 )
    {
      r = 0;
    }
    else
    {
      ++r;
      ++refcnt;
    }
  }
  
  numValidMergeCand = uiArrayAddr;
}



/** Constructs a list of merging candidates
 * \param uiAbsPartIdx
 * \param uiPUIdx 
 * \param uiDepth
 * \param pcMvFieldNeighbours
 * \param puhInterDirNeighbours
 * \param numValidMergeCand
 */
#if H_3D
Void TComDataCU::xGetInterMergeCandidates( UInt uiAbsPartIdx, UInt uiPUIdx, TComMvField* pcMvFieldNeighbours, UChar* puhInterDirNeighbours
#else
Void TComDataCU::getInterMergeCandidates( UInt uiAbsPartIdx, UInt uiPUIdx, TComMvField* pcMvFieldNeighbours, UChar* puhInterDirNeighbours
#endif
#if H_3D_SPIVMP
      , TComMvField* pcMvFieldSP, UChar* puhInterDirSP
#endif
      , Int& numValidMergeCand, Int mrgCandIdx
)
{
  UInt uiAbsPartAddr = m_uiAbsIdxInLCU + uiAbsPartIdx;
#if H_3D_IV_MERGE
  ////////////////////////////
  //////// INIT LISTS ////////
  ////////////////////////////
  TComMv cZeroMv;
#else
  Bool abCandIsInter[ MRG_MAX_NUM_CANDS ];
#endif
#if H_3D
  TComMvField tmpMV[2];
  UChar tmpDir;

#if H_3D_DDD
  m_iUseDDDCandIdx = -1;
#endif

  //////////////////////////////////
  //////// GET DISPARITIES  ////////
  //////////////////////////////////
  DisInfo cDisInfo = getDvInfo(uiAbsPartIdx);
  m_cDefaultDisInfo = cDisInfo;

  if (!( getSlice()->getIsDepth() || getSlice()->getViewIndex()>0))  // current slice is not both dependent view or depth
  {
    return;
  }
#else
  for( UInt ui = 0; ui < getSlice()->getMaxNumMergeCand(); ++ui )
  {
    abCandIsInter[ui] = false;
    pcMvFieldNeighbours[ ( ui << 1 )     ].setRefIdx(NOT_VALID);
    pcMvFieldNeighbours[ ( ui << 1 ) + 1 ].setRefIdx(NOT_VALID);
  }
#endif

  numValidMergeCand = getSlice()->getMaxNumMergeCand();
#if H_3D
  //////////////////////////////////
  //////// DERIVE LOCATIONS ////////
  //////////////////////////////////
#endif
  // compute the location of the current PU
  Int xP, yP, nPSW, nPSH;
  this->getPartPosition(uiPUIdx, xP, yP, nPSW, nPSH);

  Int iCount = 0;

  UInt uiPartIdxLT, uiPartIdxRT, uiPartIdxLB;
#if !H_3D
  PartSize cCurPS = getPartitionSize( uiAbsPartIdx );
#endif
  deriveLeftRightTopIdxGeneral( uiAbsPartIdx, uiPUIdx, uiPartIdxLT, uiPartIdxRT );
  deriveLeftBottomIdxGeneral  ( uiAbsPartIdx, uiPUIdx, uiPartIdxLB );
#if H_3D
  Bool bMPIFlag   = getSlice()->getVPS()->getMPIFlag( getSlice()->getLayerIdInVps() );
  Bool bIsDepth = getSlice()->getIsDepth();
#endif 

#if H_3D_IC
  Bool bICFlag = getICFlag(uiAbsPartIdx);
#endif
#if H_3D_ARP
  Bool bARPFlag = getARPW(uiAbsPartIdx)>0 ? true : false;
#endif
#if H_3D_DBBP
  Bool bDBBPFlag = getDBBPFlag(uiAbsPartIdx);
  assert(bDBBPFlag == getDBBPFlag(0));  
#endif

#if H_3D
#if H_3D_NBDV
  for(Int i = 0; i < MRG_MAX_NUM_CANDS_MEM; i++)  
  {
    pcMvFieldNeighbours[i<<1    ].getMv().setIDVFlag (false);
    pcMvFieldNeighbours[(i<<1)+1].getMv().setIDVFlag (false);
  }
#endif
  // Clean version for MCL construction align with WD
  // init mergCands list
  for (Int i = 0; i<MRG_IVSHIFT+1; i++)
  {
    m_mergCands[i].init();
  }

  m_baseListidc = 0;

  //left
  UInt uiLeftPartIdx = 0;
  TComDataCU* pcCULeft = 0;
  pcCULeft = getPULeft( uiLeftPartIdx, uiPartIdxLB );  

  if (getAvailableFlagA1())
  {
    m_mergCands[MRG_A1].setCand( &pcMvFieldNeighbours[m_baseListidc<<1], puhInterDirNeighbours[m_baseListidc]
#if H_3D_VSP
    , 
      (pcCULeft->getVSPFlag(uiLeftPartIdx) != 0
#if H_3D_IC
      && !bICFlag
#endif
#if H_3D_ARP
      && !bARPFlag
#endif
#if H_3D_DBBP
      && !bDBBPFlag
#endif
      )
#endif
      , false
      ); 
    m_baseListidc++;
  }

  // above
  UInt uiAbovePartIdx = 0;
  TComDataCU* pcCUAbove = 0;
  pcCUAbove = getPUAbove( uiAbovePartIdx, uiPartIdxRT );

  if (getAvailableFlagB1())
  {
    m_mergCands[MRG_B1].setCand( &pcMvFieldNeighbours[m_baseListidc<<1], puhInterDirNeighbours[m_baseListidc]
#if H_3D_VSP
    ,
      ( ( ( getAddr() - pcCUAbove->getAddr() ) == 0) && (pcCUAbove->getVSPFlag(uiAbovePartIdx) != 0) 
#if H_3D_IC
      && !bICFlag
#endif
#if H_3D_ARP
      && !bARPFlag
#endif
#if H_3D_DBBP
      && !bDBBPFlag
#endif
      )
#endif
      , false
      ); 
    m_baseListidc++;
  }

  // above right
  UInt uiAboveRightPartIdx = 0;
  TComDataCU* pcCUAboveRight = 0;
  pcCUAboveRight = getPUAboveRight( uiAboveRightPartIdx, uiPartIdxRT );

  if (getAvailableFlagB0())
  {
    m_mergCands[MRG_B0].setCand( &pcMvFieldNeighbours[m_baseListidc<<1], puhInterDirNeighbours[m_baseListidc]
#if H_3D_VSP
    ,
      ( ( ( getAddr() - pcCUAboveRight->getAddr() ) == 0) && (pcCUAboveRight->getVSPFlag(uiAboveRightPartIdx) != 0) 
#if H_3D_IC
      && !bICFlag
#endif
#if H_3D_ARP
      && !bARPFlag
#endif
#if H_3D_DBBP
      && !bDBBPFlag
#endif
      )
#endif
      , false
      ); 
    m_baseListidc++;
  }

  // left bottom
  UInt uiLeftBottomPartIdx = 0;
  TComDataCU* pcCULeftBottom = getPUBelowLeft( uiLeftBottomPartIdx, uiPartIdxLB );

  if (getAvailableFlagA0())
  {
    m_mergCands[MRG_A0].setCand( &pcMvFieldNeighbours[m_baseListidc<<1], puhInterDirNeighbours[m_baseListidc]
#if H_3D_VSP
    ,
      (pcCULeftBottom->getVSPFlag(uiLeftBottomPartIdx) != 0
#if H_3D_IC
      && !bICFlag
#endif
#if H_3D_ARP
      && !bARPFlag
#endif
#if H_3D_DBBP
      && !bDBBPFlag
#endif
      )
#endif
      , false
      ); 
    m_baseListidc++;
  }

  // above left
  UInt uiAboveLeftPartIdx = 0;
  TComDataCU* pcCUAboveLeft = 0;
  pcCUAboveLeft = getPUAboveLeft( uiAboveLeftPartIdx, uiAbsPartAddr );

  if (getAvailableFlagB2())
  {
    m_mergCands[MRG_B2].setCand( &pcMvFieldNeighbours[m_baseListidc<<1], puhInterDirNeighbours[m_baseListidc]
#if H_3D_VSP
    ,
      ( ( ( getAddr() - pcCUAboveLeft->getAddr() ) == 0) && (pcCUAboveLeft->getVSPFlag(uiAboveLeftPartIdx) != 0) 
#if H_3D_IC
      && !bICFlag
#endif
#if H_3D_ARP
      && !bARPFlag
#endif
#if H_3D_DBBP
      && !bDBBPFlag
#endif
      )
#endif
      , false
      ); 
    m_baseListidc++;
  }

#endif


#if H_3D_IV_MERGE

  /////////////////////////////////////////////
  //////// TEXTURE MERGE CANDIDATE (T) ////////
  /////////////////////////////////////////////

  bMPIFlag &= (nPSW + nPSH > 12);
  if( bMPIFlag)
  {
    tmpMV[0].setMvField( cZeroMv, NOT_VALID );
    tmpMV[1].setMvField( cZeroMv, NOT_VALID );
    tmpDir        =  0;

    Bool bSPIVMPFlag = false;

    TComPic * pcTexPic = m_pcSlice->getTexturePic();
#if H_3D_FCO
#if H_3D_FCO
    if (pcTexPic && pcTexPic->getReconMark())
#else
    if (pcTexturePic->getReconMark())
#endif
    {
#endif    
      TComPicYuv*   pcTexRec = pcTexPic->getPicYuvRec  ();
      UInt          uiPartAddr;
      Int           iWidth, iHeight;
      Int           iCurrPosX, iCurrPosY;

      this->getPartIndexAndSize( uiPUIdx, uiPartAddr, iWidth, iHeight );
      pcTexRec->getTopLeftSamplePos( this->getAddr(), this->getZorderIdxInCU() + uiPartAddr, iCurrPosX, iCurrPosY );

      Int iPUWidth, iPUHeight, iNumPart, iNumPartLine;
      this->getSPPara(iWidth, iHeight, iNumPart, iNumPartLine, iPUWidth, iPUHeight);

      for (Int i=0; i<iNumPart; i++)
      {
        puhInterDirSP[i] = 0;
        pcMvFieldSP[2*i].getMv().set(0, 0);
        pcMvFieldSP[2*i+1].getMv().set(0, 0);
        pcMvFieldSP[2*i].setRefIdx(-1);
        pcMvFieldSP[2*i+1].setRefIdx(-1);
      }

      Int         iTexCUAddr;
      Int         iTexAbsPartIdx;
      TComDataCU* pcTexCU;
      Int iPartition = 0;
      Int iInterDirSaved = 0;
      TComMvField cMvFieldSaved[2];

      Int iOffsetX = iPUWidth/2;;
      Int iOffsetY = iPUHeight/2;

      Int         iTexPosX, iTexPosY;
      const TComMv cMvRounding( 1 << ( 2 - 1 ), 1 << ( 2 - 1 ) );
      
      Int         iCenterPosX = iCurrPosX + ( ( iWidth /  iPUWidth ) >> 1 )  * iPUWidth + ( iPUWidth >> 1 );
      Int         iCenterPosY = iCurrPosY + ( ( iHeight /  iPUHeight ) >> 1 )  * iPUHeight + (iPUHeight >> 1);
      Int         iTexCenterCUAddr, iTexCenterAbsPartIdx;

      if(iWidth == iPUWidth && iHeight == iPUHeight)
      {
          iCenterPosX = iCurrPosX + (iWidth >> 1);
          iCenterPosY = iCurrPosY + (iHeight >> 1);
      }

      // derivation of center motion parameters from the collocated texture CU

      pcTexRec->getCUAddrAndPartIdx( iCenterPosX , iCenterPosY , iTexCenterCUAddr, iTexCenterAbsPartIdx );
      TComDataCU* pcDefaultCU    = pcTexPic->getCU( iTexCenterCUAddr );

      if( pcDefaultCU->getPredictionMode( iTexCenterAbsPartIdx ) != MODE_INTRA )
      {
        for( UInt uiCurrRefListId = 0; uiCurrRefListId < 2; uiCurrRefListId++ )
        {
          RefPicList  eCurrRefPicList = RefPicList( uiCurrRefListId );

          TComMvField cDefaultMvField;
          pcDefaultCU->getMvField( pcDefaultCU, iTexCenterAbsPartIdx, eCurrRefPicList, cDefaultMvField );
          Int         iDefaultRefIdx     = cDefaultMvField.getRefIdx();
          if (iDefaultRefIdx >= 0)
          {
            Int iDefaultRefPOC = pcDefaultCU->getSlice()->getRefPOC(eCurrRefPicList, iDefaultRefIdx);
            for (Int iRefPicList = 0; iRefPicList < m_pcSlice->getNumRefIdx( eCurrRefPicList ); iRefPicList++)
            {
              if (iDefaultRefPOC == m_pcSlice->getRefPOC(eCurrRefPicList, iRefPicList))
              {
                bSPIVMPFlag = true;
                TComMv cMv = cDefaultMvField.getMv() + cMvRounding;
                cMv >>= 2;
                cMvFieldSaved[eCurrRefPicList].setMvField(cMv, iRefPicList) ;
                break;
              }
            }
          }
        }
      }
      if ( bSPIVMPFlag == true )
      {   
          iInterDirSaved = (cMvFieldSaved[0].getRefIdx()!=-1 ? 1: 0) + (cMvFieldSaved[1].getRefIdx()!=-1 ? 2: 0);
          tmpDir = iInterDirSaved;
          tmpMV[0] = cMvFieldSaved[0];
          tmpMV[1] = cMvFieldSaved[1];
      }

      if ( iInterDirSaved != 0 )
      {
        for (Int i=iCurrPosY; i < iCurrPosY + iHeight; i += iPUHeight)
        {
          for (Int j = iCurrPosX; j < iCurrPosX + iWidth; j += iPUWidth)
          {
            iTexPosX     = j + iOffsetX;
            iTexPosY     = i + iOffsetY; 
            pcTexRec->getCUAddrAndPartIdx( iTexPosX, iTexPosY, iTexCUAddr, iTexAbsPartIdx );
            pcTexCU  = pcTexPic->getCU( iTexCUAddr );

            if( pcTexCU && !pcTexCU->isIntra(iTexAbsPartIdx) )
            {
              for( UInt uiCurrRefListId = 0; uiCurrRefListId < 2; uiCurrRefListId++ )
              {
                RefPicList  eCurrRefPicList = RefPicList( uiCurrRefListId );
                TComMvField cTexMvField;
                pcTexCU->getMvField( pcTexCU, iTexAbsPartIdx, eCurrRefPicList, cTexMvField );
                Int iValidDepRef = getPic()->isTextRefValid( eCurrRefPicList, cTexMvField.getRefIdx() );
                if( (cTexMvField.getRefIdx()>=0) && ( iValidDepRef >= 0 ) )
                {
                  TComMv cMv = cTexMvField.getMv() + cMvRounding;
                  cMv >>=2;          
#if !(NTT_BUG_FIX_TK54)
                  this->clipMv( cMv );
#endif
                  pcMvFieldSP[2*iPartition + uiCurrRefListId].setMvField(cMv, iValidDepRef);
                }
              }
            }
            puhInterDirSP[iPartition] = (pcMvFieldSP[2*iPartition].getRefIdx()!=-1 ? 1: 0) + (pcMvFieldSP[2*iPartition+1].getRefIdx()!=-1 ? 2: 0);
            if (puhInterDirSP[iPartition] == 0)
            {
              if (iInterDirSaved != 0)
              {
                puhInterDirSP[iPartition] = iInterDirSaved;
                pcMvFieldSP[2*iPartition] = cMvFieldSaved[0];
                pcMvFieldSP[2*iPartition + 1] = cMvFieldSaved[1];
              }
            }
            if (iPUHeight + iPUWidth == 12)
            {
              if (puhInterDirSP[iPartition] == 3)
              {
                puhInterDirSP[iPartition] = 1;
                pcMvFieldSP[2*iPartition + 1].setMvField(TComMv(0,0), -1);
              }
            }

            iPartition ++;
          }
        }
#if H_3D
      }
#endif
#if H_3D_FCO
    }
#endif
    if( tmpDir != 0 )
    {
      Int iCnloop = 0;
      for(iCnloop = 0; iCnloop < 2; iCnloop ++)
      {
        if ( !m_mergCands[MRG_A1+iCnloop].m_bAvailable )  // prunning to A1, B1
        {
          continue;
        }
        if (tmpDir == m_mergCands[MRG_A1+iCnloop].m_uDir && tmpMV[0]==m_mergCands[MRG_A1+iCnloop].m_cMvField[0] && tmpMV[1]==m_mergCands[MRG_A1+iCnloop].m_cMvField[1])
        {
          m_mergCands[MRG_A1+iCnloop].m_bAvailable = false;
          break;
        }      
      }
      m_mergCands[MRG_T].setCand( tmpMV, tmpDir, false, bSPIVMPFlag);

      if ( mrgCandIdx == iCount )
      {
        return;
      }
      iCount ++;
    }
  }
#if H_3D_DDD
  if( m_pcSlice->getIsDepth() && m_pcSlice->getViewIndex() != 0  && bMPIFlag )
  {
    UInt uiPartIdx;
    TComDataCU *pcTextureCU = m_pcSlice->getTexturePic()->getCU( getAddr() );
    TComSlice *pcTextureSlice = pcTextureCU->getSlice();  


    tmpMV[0].setMvField( cZeroMv, NOT_VALID );
    tmpMV[1].setMvField( cZeroMv, NOT_VALID );
    tmpDir = 0;

    xDeriveCenterIdx( uiPUIdx, uiPartIdx); 

    if ( pcTextureCU && !pcTextureCU->isIntra( uiPartIdx ) )
    {

      TComMvField cMVField;
      Int iDV = 0;
      Int iViewIdx = 0;
      pcTextureCU->getMvField( pcTextureCU, uiPartIdx, REF_PIC_LIST_0, cMVField );
      if( cMVField.getRefIdx() >= 0 )
      {
        if( pcTextureSlice->getRefPOC( REF_PIC_LIST_0, cMVField.getRefIdx()) == pcTextureSlice->getPOC() )
        {
          iViewIdx = pcTextureSlice->getRefPic( REF_PIC_LIST_0, cMVField.getRefIdx())->getViewIndex();
          iDV = cMVField.getHor();


          Int iValidDepRef = getPic()->isTextRefValid( REF_PIC_LIST_0, cMVField.getRefIdx() );

          if( iValidDepRef >= 0 )
          {
            const TComMv cAdd( 2, 2 );
            cMVField.getMv() += cAdd;
            cMVField.getMv() >>= 2;
#if !(NTT_BUG_FIX_TK54)
            clipMv( cMVField.getMv() );
#endif
            tmpMV[ 0 ].setMvField( cMVField.getMv(), iValidDepRef );
            tmpDir = 1;
          }
        }
      }

      pcTextureCU->getMvField( pcTextureCU, uiPartIdx, REF_PIC_LIST_1, cMVField );

      if( !tmpDir && cMVField.getRefIdx() >= 0 )
      {
        if( pcTextureSlice->getRefPOC( REF_PIC_LIST_1, cMVField.getRefIdx()) == pcTextureSlice->getPOC() )
        {
          iViewIdx = pcTextureSlice->getRefPic( REF_PIC_LIST_1, cMVField.getRefIdx())->getViewIndex();
          iDV = cMVField.getHor();

          Int iValidDepRef = getPic()->isTextRefValid( REF_PIC_LIST_1, cMVField.getRefIdx() );

          if( iValidDepRef >= 0 )
          {
            const TComMv cAdd( 2, 2 );
            cMVField.getMv() += cAdd;
            cMVField.getMv() >>= 2;
#if !(NTT_BUG_FIX_TK54)
            clipMv( cMVField.getMv() );
#endif
            tmpMV[ 1 ].setMvField( cMVField.getMv(), iValidDepRef );
            tmpDir = 2;
          }
        }
      }
      if( tmpDir != 0 )
      {
        m_ucDDTmpDepth = m_pcSlice->getDepthFromDV( iDV,  iViewIdx );
        m_iUseDDDCandIdx = iCount;

        m_mergCands[MRG_D].setCand( tmpMV, tmpDir, false, false);
        if ( mrgCandIdx == iCount )
        {
          return;
        }
        iCount ++;
      }
    }
  }
#endif
  /////////////////////////////////////////////////////////////////
  //////// DERIVE IvMC, IvMCShift,IvDCShift, IvDC  Candidates /////
  /////////////////////////////////////////////////////////////////

  // { IvMCL0, IvMCL1, IvDCL0, IvDCL1, IvMCL0Shift, IvMCL1Shift, IvDCL0Shift, IvDCL1Shift };  
  // An enumerator would be appropriate here! 
  TComMv ivCandMv    [8];
  Int    ivCandRefIdx[8] = {-1, -1, -1, -1, -1, -1, -1, -1};

  // { IvMC, IvDC, IvMCShift, IvDCShift };  
  Int    ivCandDir   [4] = {0, 0, 0, 0};

  Bool ivMvPredFlag   = getSlice()->getVPS()->getIvMvPredFlag( getSlice()->getLayerIdInVps() );

  ivMvPredFlag &= (nPSW + nPSH > 12);
  if ( ivMvPredFlag && cDisInfo.m_aVIdxCan!=-1)
  {
    getInterViewMergeCands(uiPUIdx, ivCandRefIdx, ivCandMv, &cDisInfo, ivCandDir , bIsDepth, pcMvFieldSP, puhInterDirSP, bICFlag );
  }  

  ///////////////////////////////////////////////
  //////// INTER VIEW MOTION COMP(IvMC) /////////
  ///////////////////////////////////////////////
  if( getSlice()->getIsDepth() )
  {
    ivCandDir[1] = ivCandDir[2] = ivCandDir[3] = 0;
  }

  if( ivCandDir[0] )
  {
    tmpMV[0].setMvField( cZeroMv, NOT_VALID );
    tmpMV[1].setMvField( cZeroMv, NOT_VALID );

    if( ( ivCandDir[0] & 1 ) == 1 )
    {
      tmpMV[0].setMvField( ivCandMv[ 0 ], ivCandRefIdx[ 0 ] );
    }
    if( ( ivCandDir[0] & 2 ) == 2 )
    {
      tmpMV[1].setMvField( ivCandMv[ 1 ], ivCandRefIdx[ 1 ] );
    }

    Bool bRemoveSpa = false; //pruning

    if (!bIsDepth)
    {
      for(Int i = 0; i < 2; i ++)
      {
        if ( !m_mergCands[MRG_A1 + i].m_bAvailable ) // prunning to A1, B1
        {
          continue;
        }
        if (ivCandDir[0] == m_mergCands[MRG_A1+i].m_uDir && tmpMV[0]==m_mergCands[MRG_A1+i].m_cMvField[0] && tmpMV[1]==m_mergCands[MRG_A1+i].m_cMvField[1])
        {
          m_mergCands[MRG_A1+i].m_bAvailable = false;
          break;
        }      
      }
    }
    if (bIsDepth)
    {
      if (m_mergCands[MRG_T].m_bAvailable && ivCandDir[0] == m_mergCands[MRG_T].m_uDir && tmpMV[0]==m_mergCands[MRG_T].m_cMvField[0] && tmpMV[1]==m_mergCands[MRG_T].m_cMvField[1])
      {
        bRemoveSpa                      = true;
      }
    }
    if (!bRemoveSpa)
    {
      Bool SPIVMPFlag = false;
      if(!m_pcSlice->getIsDepth())
      {
        SPIVMPFlag = true;
      }
#if H_3D_DBBP
      SPIVMPFlag &= !bDBBPFlag;
#endif

      m_mergCands[MRG_IVMC].setCand( tmpMV, ivCandDir[0], false, SPIVMPFlag);

      if ( mrgCandIdx == iCount )
      {
        return;
      }
      iCount ++;
    }
  } 

  // early termination
  if (iCount == getSlice()->getMaxNumMergeCand()) 
  {
    return;
  }
#endif

#if H_3D
  iCount += m_mergCands[MRG_A1].m_bAvailable + m_mergCands[MRG_B1].m_bAvailable + m_mergCands[MRG_B0].m_bAvailable;
#else
  //left
  UInt uiLeftPartIdx = 0;
  TComDataCU* pcCULeft = 0;
  pcCULeft = getPULeft( uiLeftPartIdx, uiPartIdxLB );
  Bool isAvailableA1 = pcCULeft &&
    pcCULeft->isDiffMER(xP -1, yP+nPSH-1, xP, yP) &&
    !( uiPUIdx == 1 && (cCurPS == SIZE_Nx2N || cCurPS == SIZE_nLx2N || cCurPS == SIZE_nRx2N) ) &&
    !pcCULeft->isIntra( uiLeftPartIdx ) ;
  if ( isAvailableA1 )
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

    if ( mrgCandIdx == iCount )
    {
      return;
    }
    iCount ++;
  }

  // early termination
  if (iCount == getSlice()->getMaxNumMergeCand()) 
  {
    return;
  }

  // above
  UInt uiAbovePartIdx = 0;
  TComDataCU* pcCUAbove = 0;
  pcCUAbove = getPUAbove( uiAbovePartIdx, uiPartIdxRT );
  Bool isAvailableB1 = pcCUAbove &&
    pcCUAbove->isDiffMER(xP+nPSW-1, yP-1, xP, yP) &&
    !( uiPUIdx == 1 && (cCurPS == SIZE_2NxN || cCurPS == SIZE_2NxnU || cCurPS == SIZE_2NxnD) ) &&
    !pcCUAbove->isIntra( uiAbovePartIdx );
  if ( isAvailableB1 && (!isAvailableA1 || !pcCULeft->hasEqualMotion( uiLeftPartIdx, pcCUAbove, uiAbovePartIdx ) ) )
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

    if ( mrgCandIdx == iCount )
    {
      return;
    }
    iCount ++;
  }
  // early termination
  if (iCount == getSlice()->getMaxNumMergeCand()) 
  {
    return;
  }

  // above right
  UInt uiAboveRightPartIdx = 0;
  TComDataCU* pcCUAboveRight = 0;
  pcCUAboveRight = getPUAboveRight( uiAboveRightPartIdx, uiPartIdxRT );
  Bool isAvailableB0 = pcCUAboveRight &&
    pcCUAboveRight->isDiffMER(xP+nPSW, yP-1, xP, yP) &&
    !pcCUAboveRight->isIntra( uiAboveRightPartIdx );
  if ( isAvailableB0 && ( !isAvailableB1 || !pcCUAbove->hasEqualMotion( uiAbovePartIdx, pcCUAboveRight, uiAboveRightPartIdx ) ) )
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

    if ( mrgCandIdx == iCount )
    {
      return;
    }
    iCount ++;
  }
  // early termination
  if (iCount == getSlice()->getMaxNumMergeCand()) 
  {
    return;
  }
#endif

#if H_3D_DDD
  // early termination
  if ( iCount >= getSlice()->getMaxNumMergeCand()) 
  {
      return;
  }
#endif

#if H_3D_IV_MERGE 
  /////////////////////////////////////////////
  //////// INTER VIEW DISP COMP (IvDC) ////////
  /////////////////////////////////////////////
  if( ivCandDir[1] && iCount < getSlice()->getMaxNumMergeCand() && !getSlice()->getIsDepth() )
  {
    assert(iCount < getSlice()->getMaxNumMergeCand());

    tmpMV[0].setMvField( cZeroMv, NOT_VALID );
    tmpMV[1].setMvField( cZeroMv, NOT_VALID );
    if( ( ivCandDir[1] & 1 ) == 1 )
    {
      tmpMV[0].setMvField( ivCandMv[ 2 ], ivCandRefIdx[ 2 ] );
    }
    if( ( ivCandDir[1] & 2 ) == 2 )
    {
      tmpMV[1].setMvField( ivCandMv[ 3 ], ivCandRefIdx[ 3 ] );
    }

    Bool bRemoveSpa = false; //pruning to A1, B1
    for(Int i = 0; i < 2; i ++)
    {
      if ( !m_mergCands[MRG_A1+i].m_bAvailable ) 
      {
        continue;
      }
      if (ivCandDir[1] == m_mergCands[MRG_A1+i].m_uDir && tmpMV[0]==m_mergCands[MRG_A1+i].m_cMvField[0] && tmpMV[1]==m_mergCands[MRG_A1+i].m_cMvField[1])
      {
        bRemoveSpa                      = true;
        break;
      }      
    }
    if(!bRemoveSpa)
    {
#if H_3D_NBDV
      tmpMV[0].getMv().setIDVFlag (false);
      tmpMV[1].getMv().setIDVFlag (false);
#endif
      m_mergCands[MRG_IVDC].setCand( tmpMV, ivCandDir[1], false, false);

      if ( mrgCandIdx == iCount )
        return;
      iCount ++;

      // early termination
      if (iCount == getSlice()->getMaxNumMergeCand()) 
      {
        return;
      }
    }
  } 
#endif // H_3D_IV_MERGE 

#if H_3D_VSP
  /////////////////////////////////////////////////
  //////// VIEW SYNTHESIS PREDICTION (VSP) ////////
  /////////////////////////////////////////////////
  if (iCount<getSlice()->getMaxNumMergeCand())
  {

  if (
#if H_3D_IC
      !bICFlag &&
#endif
#if H_3D_ARP
      !bARPFlag &&
#endif
#if H_3D
      (nPSW + nPSH > 12) &&
#endif
#if H_3D_DBBP
      !bDBBPFlag &&
#endif
      xAddVspCand( mrgCandIdx, &cDisInfo, iCount ) )
  {
    return;
  }

  // early termination
  if (iCount == getSlice()->getMaxNumMergeCand())
  {
    return;
  }
#endif
#if H_3D
  }
#endif

#if H_3D
  iCount += m_mergCands[MRG_A0].m_bAvailable + m_mergCands[MRG_B2].m_bAvailable;
#else
  //left bottom
  UInt uiLeftBottomPartIdx = 0;
  TComDataCU* pcCULeftBottom = 0;
  pcCULeftBottom = this->getPUBelowLeft( uiLeftBottomPartIdx, uiPartIdxLB );
  Bool isAvailableA0 = pcCULeftBottom &&
  pcCULeftBottom->isDiffMER(xP-1, yP+nPSH, xP, yP) &&
  !pcCULeftBottom->isIntra( uiLeftBottomPartIdx ) ;
  if ( isAvailableA0 && ( !isAvailableA1 || !pcCULeft->hasEqualMotion( uiLeftPartIdx, pcCULeftBottom, uiLeftBottomPartIdx ) ) )
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
    if ( mrgCandIdx == iCount )
    {
      return;
    }
    iCount ++;
  }
  // early termination
  if (iCount == getSlice()->getMaxNumMergeCand()) 
  {
    return;
  }

  // above left 
  if( iCount < 4 )
  {
    UInt uiAboveLeftPartIdx = 0;
    TComDataCU* pcCUAboveLeft = 0;
    pcCUAboveLeft = getPUAboveLeft( uiAboveLeftPartIdx, uiAbsPartAddr );
    Bool isAvailableB2 = pcCUAboveLeft &&
    pcCUAboveLeft->isDiffMER(xP-1, yP-1, xP, yP) &&
    !pcCUAboveLeft->isIntra( uiAboveLeftPartIdx );
    if ( isAvailableB2 && ( !isAvailableA1 || !pcCULeft->hasEqualMotion( uiLeftPartIdx, pcCUAboveLeft, uiAboveLeftPartIdx ) )
        && ( !isAvailableB1 || !pcCUAbove->hasEqualMotion( uiAbovePartIdx, pcCUAboveLeft, uiAboveLeftPartIdx ) ) )
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
      if ( mrgCandIdx == iCount )
      {
        return;
      }
      iCount ++;
    }
  }
  // early termination
  if (iCount == getSlice()->getMaxNumMergeCand()) 
  {
    return;
  }
#endif


#if H_3D_IV_MERGE
  ////////////////////////////////////////////////////
  //////// SHIFTED IV (IvMCShift + IvDCShift) ////////
  ////////////////////////////////////////////////////
  if(  ivMvPredFlag && iCount < getSlice()->getMaxNumMergeCand() && !getSlice()->getIsDepth() ) 
  {
    if(xAddIvMRGCand( mrgCandIdx,  iCount, ivCandDir, ivCandMv, ivCandRefIdx ) )
    {
      return;
    }
    //early termination
    if (iCount == getSlice()->getMaxNumMergeCand()) 
    {
      return;
    }
  }
#endif
#if !H_3D
  if ( getSlice()->getEnableTMVPFlag())
  {
    //>> MTK colocated-RightBottom
    UInt uiPartIdxRB;

    deriveRightBottomIdx( uiPUIdx, uiPartIdxRB );  

    UInt uiAbsPartIdxTmp = g_auiZscanToRaster[uiPartIdxRB];
    UInt uiNumPartInCUWidth = m_pcPic->getNumPartInWidth();

    TComMv cColMv;
    Int iRefIdx;
    Int uiLCUIdx = -1;

    if      ( ( m_pcPic->getCU(m_uiCUAddr)->getCUPelX() + g_auiRasterToPelX[uiAbsPartIdxTmp] + m_pcPic->getMinCUWidth() ) >= m_pcSlice->getSPS()->getPicWidthInLumaSamples() )  // image boundary check
    {
    }
    else if ( ( m_pcPic->getCU(m_uiCUAddr)->getCUPelY() + g_auiRasterToPelY[uiAbsPartIdxTmp] + m_pcPic->getMinCUHeight() ) >= m_pcSlice->getSPS()->getPicHeightInLumaSamples() )
    {
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
      }
      else if ( uiAbsPartIdxTmp / uiNumPartInCUWidth < m_pcPic->getNumPartInHeight() - 1 ) // is not at the last row of LCU But is last column of LCU
      {
        uiAbsPartAddr = g_auiRasterToZscan[ uiAbsPartIdxTmp + 1 ];
        uiLCUIdx = getAddr() + 1;
      }
      else //is the right bottom corner of LCU                       
      {
        uiAbsPartAddr = 0;
      }
    }
    
    
    iRefIdx = 0;
    Bool bExistMV = false;
    UInt uiPartIdxCenter;
    UInt uiCurLCUIdx = getAddr();
    Int dir = 0;
    UInt uiArrayAddr = iCount;
    xDeriveCenterIdx( uiPUIdx, uiPartIdxCenter );
    bExistMV = uiLCUIdx >= 0 && xGetColMVP( REF_PIC_LIST_0, uiLCUIdx, uiAbsPartAddr, cColMv, iRefIdx );
    if( bExistMV == false )
    {
      bExistMV = xGetColMVP( REF_PIC_LIST_0, uiCurLCUIdx, uiPartIdxCenter, cColMv, iRefIdx );
    }
    if( bExistMV )
    {
      dir |= 1;
      pcMvFieldNeighbours[ 2 * uiArrayAddr ].setMvField( cColMv, iRefIdx );
    }

    if ( getSlice()->isInterB() )
    {
      bExistMV = uiLCUIdx >= 0 && xGetColMVP( REF_PIC_LIST_1, uiLCUIdx, uiAbsPartAddr, cColMv, iRefIdx);
      if( bExistMV == false )
      {
        bExistMV = xGetColMVP( REF_PIC_LIST_1, uiCurLCUIdx, uiPartIdxCenter, cColMv, iRefIdx );
      }
      if( bExistMV )
      {
        dir |= 2;
        pcMvFieldNeighbours[ 2 * uiArrayAddr + 1 ].setMvField( cColMv, iRefIdx );
      }
    }
    
    if (dir != 0)
    {
      puhInterDirNeighbours[uiArrayAddr] = dir;
      abCandIsInter[uiArrayAddr] = true;
      if ( mrgCandIdx == iCount )
      {
        return;
      }
      iCount++;
    }
  }
  // early termination  
  if (iCount == getSlice()->getMaxNumMergeCand()) 
  {
    return;
  }
  UInt uiArrayAddr = iCount;
  UInt uiCutoff = uiArrayAddr;
    
  if ( getSlice()->isInterB())
  {
    UInt uiPriorityList0[12] = {0 , 1, 0, 2, 1, 2, 0, 3, 1, 3, 2, 3};
    UInt uiPriorityList1[12] = {1 , 0, 2, 0, 2, 1, 3, 0, 3, 1, 3, 2};

    for (Int idx=0; idx<uiCutoff*(uiCutoff-1) && uiArrayAddr!= getSlice()->getMaxNumMergeCand(); idx++)
    {
      Int i = uiPriorityList0[idx]; Int j = uiPriorityList1[idx];
      if (abCandIsInter[i] && abCandIsInter[j]&& (puhInterDirNeighbours[i]&0x1)&&(puhInterDirNeighbours[j]&0x2))
      {
        abCandIsInter[uiArrayAddr] = true;
        puhInterDirNeighbours[uiArrayAddr] = 3;

        // get Mv from cand[i] and cand[j]
        pcMvFieldNeighbours[uiArrayAddr << 1].setMvField(pcMvFieldNeighbours[i<<1].getMv(), pcMvFieldNeighbours[i<<1].getRefIdx());
        pcMvFieldNeighbours[( uiArrayAddr << 1 ) + 1].setMvField(pcMvFieldNeighbours[(j<<1)+1].getMv(), pcMvFieldNeighbours[(j<<1)+1].getRefIdx());

        Int iRefPOCL0 = m_pcSlice->getRefPOC( REF_PIC_LIST_0, pcMvFieldNeighbours[(uiArrayAddr<<1)].getRefIdx() );
        Int iRefPOCL1 = m_pcSlice->getRefPOC( REF_PIC_LIST_1, pcMvFieldNeighbours[(uiArrayAddr<<1)+1].getRefIdx() );
        if (iRefPOCL0 == iRefPOCL1 && pcMvFieldNeighbours[(uiArrayAddr<<1)].getMv() == pcMvFieldNeighbours[(uiArrayAddr<<1)+1].getMv())
        {
          abCandIsInter[uiArrayAddr] = false;
        }
        else
        {
          uiArrayAddr++;
        }
      }
    }
  }
  // early termination
  if (uiArrayAddr == getSlice()->getMaxNumMergeCand()) 
  {
    return;
  }
  Int iNumRefIdx = (getSlice()->isInterB()) ? min(m_pcSlice->getNumRefIdx(REF_PIC_LIST_0), m_pcSlice->getNumRefIdx(REF_PIC_LIST_1)) : m_pcSlice->getNumRefIdx(REF_PIC_LIST_0);
  Int r = 0;
  Int refcnt = 0;
  while (uiArrayAddr < getSlice()->getMaxNumMergeCand())
  {
    abCandIsInter[uiArrayAddr] = true;
    puhInterDirNeighbours[uiArrayAddr] = 1;
    pcMvFieldNeighbours[uiArrayAddr << 1].setMvField( TComMv(0, 0), r);

    if ( getSlice()->isInterB() )
    {
      puhInterDirNeighbours[uiArrayAddr] = 3;
      pcMvFieldNeighbours[(uiArrayAddr << 1) + 1].setMvField(TComMv(0, 0), r);
    }
    uiArrayAddr++;
    if ( refcnt == iNumRefIdx - 1 )
    {
      r = 0;
    }
    else
    {
      ++r;
      ++refcnt;
    }
  }

  numValidMergeCand = uiArrayAddr;
#endif
}
#else

/** Constructs a list of merging candidates
 * \param uiAbsPartIdx
 * \param uiPUIdx 
 * \param uiDepth
 * \param pcMvFieldNeighbours
 * \param puhInterDirNeighbours
 * \param numValidMergeCand
 */
Void TComDataCU::getInterMergeCandidates( UInt uiAbsPartIdx, UInt uiPUIdx, TComMvField* pcMvFieldNeighbours, UChar* puhInterDirNeighbours
      , Int& numValidMergeCand, Int mrgCandIdx
)
{
  UInt uiAbsPartAddr = m_uiAbsIdxInLCU + uiAbsPartIdx;
  Bool abCandIsInter[ MRG_MAX_NUM_CANDS ];
  for( UInt ui = 0; ui < getSlice()->getMaxNumMergeCand(); ++ui )
  {
    abCandIsInter[ui] = false;
    pcMvFieldNeighbours[ ( ui << 1 )     ].setRefIdx(NOT_VALID);
    pcMvFieldNeighbours[ ( ui << 1 ) + 1 ].setRefIdx(NOT_VALID);
  }
  numValidMergeCand = getSlice()->getMaxNumMergeCand();
  // compute the location of the current PU
  Int xP, yP, nPSW, nPSH;
  this->getPartPosition(uiPUIdx, xP, yP, nPSW, nPSH);

  Int iCount = 0;

  UInt uiPartIdxLT, uiPartIdxRT, uiPartIdxLB;
  PartSize cCurPS = getPartitionSize( uiAbsPartIdx );
  deriveLeftRightTopIdxGeneral( uiAbsPartIdx, uiPUIdx, uiPartIdxLT, uiPartIdxRT );
  deriveLeftBottomIdxGeneral  ( uiAbsPartIdx, uiPUIdx, uiPartIdxLB );

  //left
  UInt uiLeftPartIdx = 0;
  TComDataCU* pcCULeft = 0;
  pcCULeft = getPULeft( uiLeftPartIdx, uiPartIdxLB );
  Bool isAvailableA1 = pcCULeft &&
  pcCULeft->isDiffMER(xP -1, yP+nPSH-1, xP, yP) &&
  !( uiPUIdx == 1 && (cCurPS == SIZE_Nx2N || cCurPS == SIZE_nLx2N || cCurPS == SIZE_nRx2N) ) &&
  !pcCULeft->isIntra( uiLeftPartIdx ) ;
  if ( isAvailableA1 )
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
    if ( mrgCandIdx == iCount )
    {
      return;
    }
    iCount ++;
  }
  
  // early termination
  if (iCount == getSlice()->getMaxNumMergeCand()) 
  {
    return;
  }

  // above
  UInt uiAbovePartIdx = 0;
  TComDataCU* pcCUAbove = 0;
  pcCUAbove = getPUAbove( uiAbovePartIdx, uiPartIdxRT );
  Bool isAvailableB1 = pcCUAbove &&
  pcCUAbove->isDiffMER(xP+nPSW-1, yP-1, xP, yP) &&
  !( uiPUIdx == 1 && (cCurPS == SIZE_2NxN || cCurPS == SIZE_2NxnU || cCurPS == SIZE_2NxnD) ) &&
  !pcCUAbove->isIntra( uiAbovePartIdx );
  if ( isAvailableB1 && (!isAvailableA1 || !pcCULeft->hasEqualMotion( uiLeftPartIdx, pcCUAbove, uiAbovePartIdx ) ) )
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

    if ( mrgCandIdx == iCount )
    {
      return;
    }
    iCount ++;
  }
  // early termination
  if (iCount == getSlice()->getMaxNumMergeCand()) 
  {
    return;
  }


  // above right
  UInt uiAboveRightPartIdx = 0;
  TComDataCU* pcCUAboveRight = 0;
  pcCUAboveRight = getPUAboveRight( uiAboveRightPartIdx, uiPartIdxRT );
  Bool isAvailableB0 = pcCUAboveRight &&
  pcCUAboveRight->isDiffMER(xP+nPSW, yP-1, xP, yP) &&
  !pcCUAboveRight->isIntra( uiAboveRightPartIdx );
  if ( isAvailableB0 && ( !isAvailableB1 || !pcCUAbove->hasEqualMotion( uiAbovePartIdx, pcCUAboveRight, uiAboveRightPartIdx ) ) )
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
    if ( mrgCandIdx == iCount )
    {
      return;
    }
    iCount ++;
  }
  // early termination
  if (iCount == getSlice()->getMaxNumMergeCand()) 
  {
    return;
  }

  //left bottom
  UInt uiLeftBottomPartIdx = 0;
  TComDataCU* pcCULeftBottom = 0;
  pcCULeftBottom = this->getPUBelowLeft( uiLeftBottomPartIdx, uiPartIdxLB );
  Bool isAvailableA0 = pcCULeftBottom &&
  pcCULeftBottom->isDiffMER(xP-1, yP+nPSH, xP, yP) &&
  !pcCULeftBottom->isIntra( uiLeftBottomPartIdx ) ;
  if ( isAvailableA0 && ( !isAvailableA1 || !pcCULeft->hasEqualMotion( uiLeftPartIdx, pcCULeftBottom, uiLeftBottomPartIdx ) ) )
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

    if ( mrgCandIdx == iCount )
    {
      return;
    }
    iCount ++;
  }
  // early termination
  if (iCount == getSlice()->getMaxNumMergeCand()) 
  {
    return;
  }

  // above left 
  if( iCount < 4 )
  {
    UInt uiAboveLeftPartIdx = 0;
    TComDataCU* pcCUAboveLeft = 0;
    pcCUAboveLeft = getPUAboveLeft( uiAboveLeftPartIdx, uiAbsPartAddr );
    Bool isAvailableB2 = pcCUAboveLeft &&
    pcCUAboveLeft->isDiffMER(xP-1, yP-1, xP, yP) &&
    !pcCUAboveLeft->isIntra( uiAboveLeftPartIdx );
    if ( isAvailableB2 && ( !isAvailableA1 || !pcCULeft->hasEqualMotion( uiLeftPartIdx, pcCUAboveLeft, uiAboveLeftPartIdx ) )
        && ( !isAvailableB1 || !pcCUAbove->hasEqualMotion( uiAbovePartIdx, pcCUAboveLeft, uiAboveLeftPartIdx ) ) )
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
      if ( mrgCandIdx == iCount )
      {
        return;
      }
      iCount ++;
    }
  }
  // early termination
  if (iCount == getSlice()->getMaxNumMergeCand()) 
  {
    return;
  }

  if ( getSlice()->getEnableTMVPFlag())
  {
    //>> MTK colocated-RightBottom
    UInt uiPartIdxRB;

    deriveRightBottomIdx( uiPUIdx, uiPartIdxRB );  

    UInt uiAbsPartIdxTmp = g_auiZscanToRaster[uiPartIdxRB];
    UInt uiNumPartInCUWidth = m_pcPic->getNumPartInWidth();

    TComMv cColMv;
    Int iRefIdx;
    Int uiLCUIdx = -1;

    if      ( ( m_pcPic->getCU(m_uiCUAddr)->getCUPelX() + g_auiRasterToPelX[uiAbsPartIdxTmp] + m_pcPic->getMinCUWidth() ) >= m_pcSlice->getSPS()->getPicWidthInLumaSamples() )  // image boundary check
    {
    }
    else if ( ( m_pcPic->getCU(m_uiCUAddr)->getCUPelY() + g_auiRasterToPelY[uiAbsPartIdxTmp] + m_pcPic->getMinCUHeight() ) >= m_pcSlice->getSPS()->getPicHeightInLumaSamples() )
    {
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
      }
      else if ( uiAbsPartIdxTmp / uiNumPartInCUWidth < m_pcPic->getNumPartInHeight() - 1 ) // is not at the last row of LCU But is last column of LCU
      {
        uiAbsPartAddr = g_auiRasterToZscan[ uiAbsPartIdxTmp + 1 ];
        uiLCUIdx = getAddr() + 1;
      }
      else //is the right bottom corner of LCU                       
      {
        uiAbsPartAddr = 0;
      }
    }
    
    
    iRefIdx = 0;
    Bool bExistMV = false;
    UInt uiPartIdxCenter;
    UInt uiCurLCUIdx = getAddr();
    Int dir = 0;
    UInt uiArrayAddr = iCount;
    xDeriveCenterIdx( uiPUIdx, uiPartIdxCenter );
    bExistMV = uiLCUIdx >= 0 && xGetColMVP( REF_PIC_LIST_0, uiLCUIdx, uiAbsPartAddr, cColMv, iRefIdx );
    if( bExistMV == false )
    {
      bExistMV = xGetColMVP( REF_PIC_LIST_0, uiCurLCUIdx, uiPartIdxCenter, cColMv, iRefIdx );
    }
    if( bExistMV )
    {
      dir |= 1;
      pcMvFieldNeighbours[ 2 * uiArrayAddr ].setMvField( cColMv, iRefIdx );
    }

    if ( getSlice()->isInterB() )
    {
      bExistMV = uiLCUIdx >= 0 && xGetColMVP( REF_PIC_LIST_1, uiLCUIdx, uiAbsPartAddr, cColMv, iRefIdx);
      if( bExistMV == false )
      {
        bExistMV = xGetColMVP( REF_PIC_LIST_1, uiCurLCUIdx, uiPartIdxCenter, cColMv, iRefIdx );
      }
      if( bExistMV )
      {
        dir |= 2;
        pcMvFieldNeighbours[ 2 * uiArrayAddr + 1 ].setMvField( cColMv, iRefIdx );
      }
    }
    
    if (dir != 0)
    {
      puhInterDirNeighbours[uiArrayAddr] = dir;
      abCandIsInter[uiArrayAddr] = true;
      if ( mrgCandIdx == iCount )
      {
        return;
      }
      iCount++;
    }
  }
  // early termination
  if (iCount == getSlice()->getMaxNumMergeCand()) 
  {
    return;
  }
  UInt uiArrayAddr = iCount;
  UInt uiCutoff = uiArrayAddr;
    
  if ( getSlice()->isInterB())
  {
    UInt uiPriorityList0[12] = {0 , 1, 0, 2, 1, 2, 0, 3, 1, 3, 2, 3};
    UInt uiPriorityList1[12] = {1 , 0, 2, 0, 2, 1, 3, 0, 3, 1, 3, 2};

    for (Int idx=0; idx<uiCutoff*(uiCutoff-1) && uiArrayAddr!= getSlice()->getMaxNumMergeCand(); idx++)
    {
      Int i = uiPriorityList0[idx]; Int j = uiPriorityList1[idx];
      if (abCandIsInter[i] && abCandIsInter[j]&& (puhInterDirNeighbours[i]&0x1)&&(puhInterDirNeighbours[j]&0x2))
      {
        abCandIsInter[uiArrayAddr] = true;
        puhInterDirNeighbours[uiArrayAddr] = 3;

        // get Mv from cand[i] and cand[j]
        pcMvFieldNeighbours[uiArrayAddr << 1].setMvField(pcMvFieldNeighbours[i<<1].getMv(), pcMvFieldNeighbours[i<<1].getRefIdx());
        pcMvFieldNeighbours[( uiArrayAddr << 1 ) + 1].setMvField(pcMvFieldNeighbours[(j<<1)+1].getMv(), pcMvFieldNeighbours[(j<<1)+1].getRefIdx());

        Int iRefPOCL0 = m_pcSlice->getRefPOC( REF_PIC_LIST_0, pcMvFieldNeighbours[(uiArrayAddr<<1)].getRefIdx() );
        Int iRefPOCL1 = m_pcSlice->getRefPOC( REF_PIC_LIST_1, pcMvFieldNeighbours[(uiArrayAddr<<1)+1].getRefIdx() );
        if (iRefPOCL0 == iRefPOCL1 && pcMvFieldNeighbours[(uiArrayAddr<<1)].getMv() == pcMvFieldNeighbours[(uiArrayAddr<<1)+1].getMv())
        {
          abCandIsInter[uiArrayAddr] = false;
        }
        else
        {
          uiArrayAddr++;
        }
      }
    }
  }
  // early termination
  if (uiArrayAddr == getSlice()->getMaxNumMergeCand()) 
  {
    return;
  }
  Int iNumRefIdx = (getSlice()->isInterB()) ? min(m_pcSlice->getNumRefIdx(REF_PIC_LIST_0), m_pcSlice->getNumRefIdx(REF_PIC_LIST_1)) : m_pcSlice->getNumRefIdx(REF_PIC_LIST_0);
  Int r = 0;
  Int refcnt = 0;
  while (uiArrayAddr < getSlice()->getMaxNumMergeCand())
  {
    abCandIsInter[uiArrayAddr] = true;
    puhInterDirNeighbours[uiArrayAddr] = 1;
    pcMvFieldNeighbours[uiArrayAddr << 1].setMvField( TComMv(0, 0), r);

    if ( getSlice()->isInterB() )
    {
      puhInterDirNeighbours[uiArrayAddr] = 3;
      pcMvFieldNeighbours[(uiArrayAddr << 1) + 1].setMvField(TComMv(0, 0), r);
    }
    uiArrayAddr++;
    if ( refcnt == iNumRefIdx - 1 )
    {
      r = 0;
    }
    else
    {
      ++r;
      ++refcnt;
    }
  }

  numValidMergeCand = uiArrayAddr;
}
#endif

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

/** Constructs a list of candidates for AMVP
 * \param uiPartIdx
 * \param uiPartAddr 
 * \param eRefPicList
 * \param iRefIdx
 * \param pInfo
 */
Void TComDataCU::fillMvpCand ( UInt uiPartIdx, UInt uiPartAddr, RefPicList eRefPicList, Int iRefIdx, AMVPInfo* pInfo )
{
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
  
  deriveLeftRightTopIdx( uiPartIdx, uiPartIdxLT, uiPartIdxRT );
  deriveLeftBottomIdx( uiPartIdx, uiPartIdxLB );
  
  TComDataCU* tmpCU = NULL;
  UInt idx;
  tmpCU = getPUBelowLeft(idx, uiPartIdxLB);
  bAddedSmvp = (tmpCU != NULL) && (tmpCU->getPredictionMode(idx) != MODE_INTRA);

  if (!bAddedSmvp)
  {
    tmpCU = getPULeft(idx, uiPartIdxLB);
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
      xAddMVPCandOrder( pInfo, eRefPicList, iRefIdx, uiPartIdxLB, MD_LEFT );
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
    xAddMVPCand( pInfo, eRefPicList, iRefIdx, uiPartIdxLT, MD_ABOVE_LEFT);
  }

  if (!bAddedSmvp)
  {
    bAdded = xAddMVPCandOrder( pInfo, eRefPicList, iRefIdx, uiPartIdxRT, MD_ABOVE_RIGHT);
    if (!bAdded) 
    {
      bAdded = xAddMVPCandOrder( pInfo, eRefPicList, iRefIdx, uiPartIdxRT, MD_ABOVE);
    }

    if(!bAdded)
    {
      xAddMVPCandOrder( pInfo, eRefPicList, iRefIdx, uiPartIdxLT, MD_ABOVE_LEFT);
    }
  }
  
  if ( pInfo->iN == 2 )
  {
    if ( pInfo->m_acMvCand[ 0 ] == pInfo->m_acMvCand[ 1 ] )
    {
      pInfo->iN = 1;
    }
  }

  if ( getSlice()->getEnableTMVPFlag() )
  {
    // Get Temporal Motion Predictor
    Int iRefIdx_Col = iRefIdx;
    TComMv cColMv;
    UInt uiPartIdxRB;
    UInt uiAbsPartIdx;  
    UInt uiAbsPartAddr;

    deriveRightBottomIdx( uiPartIdx, uiPartIdxRB );
    uiAbsPartAddr = m_uiAbsIdxInLCU + uiPartAddr;

    //----  co-located RightBottom Temporal Predictor (H) ---//
    uiAbsPartIdx = g_auiZscanToRaster[uiPartIdxRB];
    Int uiLCUIdx = -1;
    if ( ( m_pcPic->getCU(m_uiCUAddr)->getCUPelX() + g_auiRasterToPelX[uiAbsPartIdx] + m_pcPic->getMinCUWidth() ) >= m_pcSlice->getSPS()->getPicWidthInLumaSamples() )  // image boundary check
    {
    }
    else if ( ( m_pcPic->getCU(m_uiCUAddr)->getCUPelY() + g_auiRasterToPelY[uiAbsPartIdx] + m_pcPic->getMinCUHeight() ) >= m_pcSlice->getSPS()->getPicHeightInLumaSamples() )
    {
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
      }
      else if ( uiAbsPartIdx / uiNumPartInCUWidth < m_pcPic->getNumPartInHeight() - 1 ) // is not at the last row of LCU But is last column of LCU
      {
        uiAbsPartAddr = g_auiRasterToZscan[ uiAbsPartIdx + 1 ];
        uiLCUIdx = getAddr() + 1;
      }
      else //is the right bottom corner of LCU                       
      {
        uiAbsPartAddr = 0;
      }
    }
    if ( uiLCUIdx >= 0 && xGetColMVP( eRefPicList, uiLCUIdx, uiAbsPartAddr, cColMv, iRefIdx_Col 
#if H_3D_TMVP
         , 0
#endif
    ) )
    {
      pInfo->m_acMvCand[pInfo->iN++] = cColMv;
    }
    else 
    {
      UInt uiPartIdxCenter;
      UInt uiCurLCUIdx = getAddr();
      xDeriveCenterIdx( uiPartIdx, uiPartIdxCenter );
      if (xGetColMVP( eRefPicList, uiCurLCUIdx, uiPartIdxCenter,  cColMv, iRefIdx_Col 
#if H_3D_TMVP
         , 0
#endif
      ))
      {
        pInfo->m_acMvCand[pInfo->iN++] = cColMv;
      }
    }
    //----  co-located RightBottom Temporal Predictor  ---//
  }

  if (pInfo->iN > AMVP_MAX_NUM_CANDS)
  {
    pInfo->iN = AMVP_MAX_NUM_CANDS;
  }
  while (pInfo->iN < AMVP_MAX_NUM_CANDS)
  {
      pInfo->m_acMvCand[pInfo->iN].set(0,0);
      pInfo->iN++;
  }
  return ;
}

Bool TComDataCU::isBipredRestriction(UInt puIdx)
{
  Int width = 0;
  Int height = 0;
  UInt partAddr;
  
#if H_3D_DBBP
  if( getDBBPFlag(0) )
  {
    return true;
  }
#endif

  getPartIndexAndSize( puIdx, partAddr, width, height );
  if ( getWidth(0) == 8 && (width < 8 || height < 8) )
  {
    return true;
  }
  return false;
}

Void TComDataCU::clipMv    (TComMv&  rcMv)
{
  Int  iMvShift = 2;
#if H_3D_IC
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

UInt TComDataCU::getIntraSizeIdx(UInt uiAbsPartIdx)
{
  UInt uiShift = ( m_pePartSize[uiAbsPartIdx]==SIZE_NxN ? 1 : 0 );
  
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
  return ( getSkipFlag( uiPartIdx ) );
}

#if H_3D_IC
Bool TComDataCU::isIC( UInt uiPartIdx )
{
    if ( m_pcSlice->isIntra () )
    {
        return false;
    }
    return ( ( getSkipFlag(uiPartIdx) || getPredictionMode(uiPartIdx) == MODE_INTER) && getICFlag( uiPartIdx ) && isICFlagRequired( uiPartIdx ) );
}
#endif

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
      pcTmpCU = getPULeft(uiIdx, uiPartUnitIdx);
      break;
    }
    case MD_ABOVE:
    {
      pcTmpCU = getPUAbove(uiIdx, uiPartUnitIdx );
      break;
    }
    case MD_ABOVE_RIGHT:
    {
      pcTmpCU = getPUAboveRight(uiIdx, uiPartUnitIdx);
      break;
    }
    case MD_BELOW_LEFT:
    {
      pcTmpCU = getPUBelowLeft(uiIdx, uiPartUnitIdx);
      break;
    }
    case MD_ABOVE_LEFT:
    {
      pcTmpCU = getPUAboveLeft(uiIdx, uiPartUnitIdx);
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
  
  if ( pcTmpCU->getCUMvField(eRefPicList)->getRefIdx(uiIdx) >= 0 && m_pcSlice->getRefPic( eRefPicList, iRefIdx)->getPOC() == pcTmpCU->getSlice()->getRefPOC( eRefPicList, pcTmpCU->getCUMvField(eRefPicList)->getRefIdx(uiIdx) ))
  {
    TComMv cMvPred = pcTmpCU->getCUMvField(eRefPicList)->getMv(uiIdx);
    
    pInfo->m_acMvCand[ pInfo->iN++] = cMvPred;
    return true;
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
    iNeibRefPOC = pcTmpCU->getSlice()->getRefPOC( eRefPicList2nd, pcTmpCU->getCUMvField(eRefPicList2nd)->getRefIdx(uiIdx) );
    if( iNeibRefPOC == iCurrRefPOC ) // Same Reference Frame But Diff List//
    {
      TComMv cMvPred = pcTmpCU->getCUMvField(eRefPicList2nd)->getMv(uiIdx);
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
      pcTmpCU = getPULeft(uiIdx, uiPartUnitIdx);
      break;
    }
  case MD_ABOVE:
    {
      pcTmpCU = getPUAbove(uiIdx, uiPartUnitIdx);
      break;
    }
  case MD_ABOVE_RIGHT:
    {
      pcTmpCU = getPUAboveRight(uiIdx, uiPartUnitIdx);
      break;
    }
  case MD_BELOW_LEFT:
    {
      pcTmpCU = getPUBelowLeft(uiIdx, uiPartUnitIdx);
      break;
    }
  case MD_ABOVE_LEFT:
    {
      pcTmpCU = getPUAboveLeft(uiIdx, uiPartUnitIdx);
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

  Bool bIsCurrRefLongTerm = m_pcSlice->getRefPic( eRefPicList, iRefIdx)->getIsLongTerm();
  Bool bIsNeibRefLongTerm = false;
  //---------------  V1 (END) ------------------//
  if( pcTmpCU->getCUMvField(eRefPicList)->getRefIdx(uiIdx) >= 0)
  {
    iNeibRefPOC = pcTmpCU->getSlice()->getRefPOC( eRefPicList, pcTmpCU->getCUMvField(eRefPicList)->getRefIdx(uiIdx) );
    TComMv cMvPred = pcTmpCU->getCUMvField(eRefPicList)->getMv(uiIdx);
    TComMv rcMv;

    bIsNeibRefLongTerm = pcTmpCU->getSlice()->getRefPic( eRefPicList, pcTmpCU->getCUMvField(eRefPicList)->getRefIdx(uiIdx) )->getIsLongTerm();
    if ( bIsCurrRefLongTerm == bIsNeibRefLongTerm ) 
    {
    if ( bIsCurrRefLongTerm || bIsNeibRefLongTerm )
    {
      rcMv = cMvPred;
    }
    else
    {
      Int iScale = xGetDistScaleFactor( iCurrPOC, iCurrRefPOC, iNeibPOC, iNeibRefPOC );
      if ( iScale == 4096 )
      {
        rcMv = cMvPred;
      }
      else
      {
        rcMv = cMvPred.scaleMv( iScale );
      }
    }
    pInfo->m_acMvCand[ pInfo->iN++] = rcMv;
    return true;
    }
  }
  //---------------------- V2(END) --------------------//
  if( pcTmpCU->getCUMvField(eRefPicList2nd)->getRefIdx(uiIdx) >= 0)
  {
    iNeibRefPOC = pcTmpCU->getSlice()->getRefPOC( eRefPicList2nd, pcTmpCU->getCUMvField(eRefPicList2nd)->getRefIdx(uiIdx) );
    TComMv cMvPred = pcTmpCU->getCUMvField(eRefPicList2nd)->getMv(uiIdx);
    TComMv rcMv;

    bIsNeibRefLongTerm = pcTmpCU->getSlice()->getRefPic( eRefPicList2nd, pcTmpCU->getCUMvField(eRefPicList2nd)->getRefIdx(uiIdx) )->getIsLongTerm();
    if ( bIsCurrRefLongTerm == bIsNeibRefLongTerm ) 
    {
    if ( bIsCurrRefLongTerm || bIsNeibRefLongTerm )
    {
      rcMv = cMvPred;
    }
    else
    {
      Int iScale = xGetDistScaleFactor( iCurrPOC, iCurrRefPOC, iNeibPOC, iNeibRefPOC );
      if ( iScale == 4096 )
      {
        rcMv = cMvPred;
      }
      else
      {
        rcMv = cMvPred.scaleMv( iScale );
      }
    }
    pInfo->m_acMvCand[ pInfo->iN++] = rcMv;
    return true;
    }
  }
  //---------------------- V3(END) --------------------//
  return false;
}

/** 
 * \param eRefPicList
 * \param uiCUAddr 
 * \param uiPartUnitIdx
 * \param riRefIdx
 * \returns Bool
 */
Bool TComDataCU::xGetColMVP( RefPicList eRefPicList, Int uiCUAddr, Int uiPartUnitIdx, TComMv& rcMv, Int& riRefIdx 
#if H_3D_TMVP
  , Bool bMRG
#endif
  )
{
  UInt uiAbsPartAddr = uiPartUnitIdx;

  RefPicList  eColRefPicList;
  Int iColPOC, iColRefPOC, iCurrPOC, iCurrRefPOC, iScale;
  TComMv cColMv;

  // use coldir.
  TComPic *pColPic = getSlice()->getRefPic( RefPicList(getSlice()->isInterB() ? 1-getSlice()->getColFromL0Flag() : 0), getSlice()->getColRefIdx());
  TComDataCU *pColCU = pColPic->getCU( uiCUAddr );
  if(pColCU->getPic()==0||pColCU->getPartitionSize(uiPartUnitIdx)==SIZE_NONE)
  {
    return false;
  }
  iCurrPOC = m_pcSlice->getPOC();    
  iColPOC = pColCU->getSlice()->getPOC();  

  if (pColCU->isIntra(uiAbsPartAddr))
  {
    return false;
  }
  eColRefPicList = getSlice()->getCheckLDC() ? eRefPicList : RefPicList(getSlice()->getColFromL0Flag());

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
  cColMv = pColCU->getCUMvField(eColRefPicList)->getMv(uiAbsPartAddr);

  iCurrRefPOC = m_pcSlice->getRefPic(eRefPicList, riRefIdx)->getPOC();
  Bool bIsCurrRefLongTerm = m_pcSlice->getRefPic(eRefPicList, riRefIdx)->getIsLongTerm();
  Bool bIsColRefLongTerm = pColCU->getSlice()->getIsUsedAsLongTerm(eColRefPicList, iColRefIdx);

  if ( bIsCurrRefLongTerm != bIsColRefLongTerm ) 
  {
#if H_3D_TMVP
    Int iAlterRefIdx  = m_pcSlice->getAlterRefIdx(eRefPicList);
    if(bMRG && iAlterRefIdx > 0)
    {
      riRefIdx = iAlterRefIdx;
      bIsCurrRefLongTerm = m_pcSlice->getRefPic(eRefPicList, riRefIdx)->getIsLongTerm();
      iCurrRefPOC = m_pcSlice->getRefPic(eRefPicList, riRefIdx)->getPOC();
      assert(bIsCurrRefLongTerm == bIsColRefLongTerm);
    }
    else
    {
#endif
      return false;
#if H_3D_TMVP
    }
#endif
  }

  if ( bIsCurrRefLongTerm || bIsColRefLongTerm )
  {
#if H_3D_TMVP
    Int iCurrViewId    = m_pcSlice->getViewIndex (); 
    Int iCurrRefViewId = m_pcSlice->getRefPic(eRefPicList, riRefIdx)->getViewIndex (); 
    Int iColViewId     = pColCU->getSlice()->getViewIndex(); 
    Int iColRefViewId  = pColCU->getSlice()->getRefPic( eColRefPicList, pColCU->getCUMvField(eColRefPicList)->getRefIdx(uiAbsPartAddr))->getViewIndex(); 
    iScale = xGetDistScaleFactor( iCurrViewId, iCurrRefViewId, iColViewId, iColRefViewId );

    if ( iScale != 4096 && m_pcSlice->getVPS()->getIvMvScalingFlag(getSlice()->getLayerIdInVps()) ) 
    {
      rcMv = cColMv.scaleMv( iScale );
    }
    else
    {
#endif
       rcMv = cColMv;
#if H_3D_TMVP
    }
#endif
  }
  else
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
  }
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
Void TComDataCU::xDeriveCenterIdx( UInt uiPartIdx, UInt& ruiPartIdxCenter )
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
#if H_3D
Void TComDataCU::compressMV(Int scale)
#else
Void TComDataCU::compressMV()
#endif
{
#if H_3D
  Int scaleFactor = (4 / scale ) * AMVP_DECIMATION_FACTOR / m_unitSize;
#else
  Int scaleFactor = 4 * AMVP_DECIMATION_FACTOR / m_unitSize;
#endif
  if (scaleFactor > 0)
  {
    m_acCUMvField[0].compress(m_pePredMode, scaleFactor);
    m_acCUMvField[1].compress(m_pePredMode, scaleFactor);    
  }
}

UInt TComDataCU::getCoefScanIdx(UInt uiAbsPartIdx, UInt uiWidth, Bool bIsLuma, Bool bIsIntra)
{
  UInt uiCTXIdx;
  UInt uiScanIdx;
  UInt uiDirMode;

  if ( !bIsIntra ) 
  {
    uiScanIdx = SCAN_DIAG;
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
#if H_3D_DIM
    mapDepthModeToIntraDir( uiDirMode );
#endif
    uiScanIdx = SCAN_DIAG;
    if (uiCTXIdx >3 && uiCTXIdx < 6) //if multiple scans supported for transform size
    {
      uiScanIdx = abs((Int) uiDirMode - VER_IDX) < 5 ? SCAN_HOR : (abs((Int)uiDirMode - HOR_IDX) < 5 ? SCAN_VER : SCAN_DIAG);
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
#if H_3D_DIM
      mapDepthModeToIntraDir( uiDirMode );
#endif
    }
    uiScanIdx = SCAN_DIAG;
    if (uiCTXIdx >4 && uiCTXIdx < 7) //if multiple scans supported for transform size
    {
      uiScanIdx = abs((Int) uiDirMode - VER_IDX) < 5 ? SCAN_HOR : (abs((Int)uiDirMode - HOR_IDX) < 5 ? SCAN_VER : SCAN_DIAG);
    }
  }

  return uiScanIdx;
}

UInt TComDataCU::getSCUAddr()
{ 
  return getPic()->getPicSym()->getInverseCUOrderMap(m_uiCUAddr)*(1<<(m_pcSlice->getSPS()->getMaxCUDepth()<<1))+m_uiAbsIdxInLCU; 
}

#if H_3D
Void TComDataCU::getPosInPic( UInt uiAbsPartIndex, Int& riPosX, Int& riPosY )
{
  riPosX = g_auiRasterToPelX[g_auiZscanToRaster[uiAbsPartIndex]] + getCUPelX();
  riPosY = g_auiRasterToPelY[g_auiZscanToRaster[uiAbsPartIndex]] + getCUPelY();  
}
#endif
#if H_3D_IV_MERGE
Bool TComDataCU::getDispforDepth (UInt uiPartIdx, UInt uiPartAddr, DisInfo* pDisp)
{

  assert(getPartitionSize( uiPartAddr ) == SIZE_2Nx2N);

  TComMv cMv; 
#if MTK_I0093
  Int iDisp     = getSlice()->getDepthToDisparityB( 0 )[ (Int64) (1 << ( getSlice()->getSPS()->getBitDepthY() - 1 )) ];
#else
  Int iDisp     = getSlice()->getDepthToDisparityB( 0 )[ 128 ];
#endif
  cMv.setHor(iDisp);
  cMv.setVer(0);
  pDisp->m_acNBDV = cMv;
  pDisp->m_aVIdxCan = 0;

  return true;
}
#endif

#if H_3D_SINGLE_DEPTH
Bool TComDataCU::getNeighDepth (UInt uiPartIdx, UInt uiPartAddr, Pel* pNeighDepth, Int index)
{

  Bool bDepAvail = false;
  Pel *pDepth  = this->getPic()->getPicYuvRec()->getLumaAddr();
  Int iDepStride =  this->getPic()->getPicYuvRec()->getStride();

  Int xP, yP, nPSW, nPSH;
  this->getPartPosition(uiPartIdx, xP, yP, nPSW, nPSH);
  UInt PicHeight=this->getPic()->getPicYuvRec()->getHeight();
  UInt PicWidth=this->getPic()->getPicYuvRec()->getWidth();
  switch(index)
  {
  case 0: // Mid Left
    if( ( xP != 0 ) && ( ( yP + ( nPSH >> 1 ) ) < PicHeight ) )
    {
      *pNeighDepth = pDepth[ (yP+(nPSH>>1)) * iDepStride + (xP-1) ];
      bDepAvail = true;
    }
    break;
  case 1: // Mid Above
    if( ( yP != 0 ) && ( ( xP + ( nPSW >> 1 ) ) < PicWidth ) )
    {
      *pNeighDepth = pDepth[ (yP-1) * iDepStride + (xP + (nPSW>>1)) ];
      bDepAvail = true;
    }
    break;
#if !SINGLE_DEPTH_SIMP_J0115
  case 2: // Above
    if(yP != 0)
    {
      *pNeighDepth = pDepth[ (yP-1) * iDepStride + (xP) ];
      bDepAvail = true;
    }
    break;
  case 3: // Left
    if(xP != 0)
    {
      *pNeighDepth = pDepth[ (yP) * iDepStride + (xP-1) ];
      bDepAvail = true;
    }
    break;
  case 4: // Above_Left
    if(xP != 0 && yP != 0)
    {
      *pNeighDepth = pDepth[ (yP-1) * iDepStride + (xP-1) ];
      bDepAvail = true;
    }
    break;
#endif
  default:
      break;
  }
  return bDepAvail;
}

#endif
#if H_3D_NBDV 
//Notes from QC:
//TBD#1: DoNBDV related contributions are just partially integrated under the marco of H_3D_NBDV_REF, remove this comment once DoNBDV and BVSP are done
//TBD#2: set of DvMCP values need to be done as part of inter-view motion prediction process. Remove this comment once merge related integration is done
//To be checked: Parallel Merge features for NBDV, related to DV_DERIVATION_PARALLEL_B0096 and LGE_IVMP_PARALLEL_MERGE_B0136 are not integrated. The need of these features due to the adoption of CU-based NBDV is not clear. We need confirmation on this, especially by proponents
Bool TComDataCU::getDisMvpCandNBDV( DisInfo* pDInfo
#if H_3D_NBDV_REF
, Bool bDepthRefine
#endif
)
{
  //// ******* Init variables ******* /////
  // Init disparity struct for results
  pDInfo->bDV = false;   
  pDInfo->m_aVIdxCan = -1;

  // Init struct for disparities from MCP neighboring blocks
  IDVInfo cIDVInfo;
  cIDVInfo.m_bFound = false; 
  UInt uiPartIdx = 0;
  UInt uiPartAddr = 0;
  for (UInt iCurDvMcpCand = 0; iCurDvMcpCand < IDV_CANDS; iCurDvMcpCand++)
  {
    for (UInt iList = 0; iList < 2; iList++)
    {
      cIDVInfo.m_acMvCand[iList][iCurDvMcpCand].setZero();
      cIDVInfo.m_aVIdxCan[iList][iCurDvMcpCand] = 0; 
      cIDVInfo.m_bAvailab[iList][iCurDvMcpCand] = false; 
    }
  }
#if H_3D_NBDV_REF
  if( !m_pcSlice->getVPS()->getDepthRefinementFlag( m_pcSlice->getLayerIdInVps() ) )
  {
    bDepthRefine = false;
  }
#endif
  // Get Positions  
  PartSize eCUMode    = getPartitionSize( uiPartAddr );    
  assert(eCUMode == SIZE_2Nx2N);
  UInt uiPartIdxLT, uiPartIdxRT, uiPartIdxLB;  

  deriveLeftRightTopIdxGeneral(uiPartAddr, uiPartIdx, uiPartIdxLT, uiPartIdxRT );
  deriveLeftBottomIdxGeneral  (uiPartAddr, uiPartIdx, uiPartIdxLB );

  //// ******* Get disparity from temporal neighboring blocks ******* /////
  if ( getSlice()->getEnableTMVPFlag() )
  {
    TComMv cColMv;
    Int iTargetViewIdx = 0;
    Int iTStartViewIdx = 0;    

    ///*** Derive center position ***
    UInt uiPartIdxCenter;
    Int  uiLCUIdx   = getAddr();
    xDeriveCenterIdx(uiPartIdx, uiPartIdxCenter );

    ///*** Search temporal candidate pictures for disparity vector ***
    const Int iNumCandPics = getPic()->getNumDdvCandPics();
    for(Int curCandPic = 0; curCandPic < iNumCandPics; curCandPic++)
    {
      RefPicList eCurRefPicList   = REF_PIC_LIST_0 ;
      Int        curCandPicRefIdx = 0;
      if( curCandPic == 0 ) 
      { 
        eCurRefPicList   = RefPicList(getSlice()->isInterB() ? 1-getSlice()->getColFromL0Flag() : 0);
        curCandPicRefIdx = getSlice()->getColRefIdx();
      }
      else                 
      {
        eCurRefPicList   = getPic()->getRapRefList();
        curCandPicRefIdx = getPic()->getRapRefIdx();
      }

      Bool bCheck = xGetColDisMV( curCandPic, eCurRefPicList, curCandPicRefIdx, uiLCUIdx,   uiPartIdxCenter,  cColMv, iTargetViewIdx, iTStartViewIdx );

      if( bCheck )
      {
#if !(NTT_BUG_FIX_TK54)
        clipMv(cColMv);
#endif
        pDInfo->m_acNBDV = cColMv;
        pDInfo->m_aVIdxCan  = iTargetViewIdx;

#if H_3D_NBDV_REF
        TComPic* picDepth = NULL;   
#if H_3D_FCO_VSP_DONBDV_E0163
        picDepth  = getSlice()->getIvPic(true, getSlice()->getViewIndex() );
        if ( picDepth->getPicYuvRec() != NULL  )  
        {
          cColMv.setZero();
        }
        else // Go back with virtual depth
        {
          picDepth = getSlice()->getIvPic( true, iTargetViewIdx );
        }

        assert(picDepth != NULL);
#else
        picDepth = getSlice()->getIvPic( true, iTargetViewIdx );
#if !BUG_FIX_TK65
        assert(picDepth != NULL);
#endif
#endif
        if (picDepth && bDepthRefine)
          estimateDVFromDM(iTargetViewIdx, uiPartIdx, picDepth, uiPartAddr, &cColMv );

        pDInfo->m_acDoNBDV  = cColMv;
#endif //H_3D_NBDV_REF
        return true;
      }
    }
  } 

  UInt uiIdx = 0;
  Bool        bCheckMcpDv = false;   
  TComDataCU* pcTmpCU     = NULL;

  //// ******* Get disparity from left block ******* /////
  pcTmpCU = getPULeft(uiIdx, uiPartIdxLB, true, false);
  bCheckMcpDv = true; 
  if ( xCheckSpatialNBDV( pcTmpCU, uiIdx, pDInfo, bCheckMcpDv, &cIDVInfo, DVFROM_LEFT
#if H_3D_NBDV_REF
    , bDepthRefine 
#endif
    ) )
    return true;

  //// ******* Get disparity from above block ******* /////
  pcTmpCU = getPUAbove(uiIdx, uiPartIdxRT, true, false, true);
  if(pcTmpCU != NULL )
  {
    bCheckMcpDv = ( ( getAddr() - pcTmpCU->getAddr() ) == 0);
    if ( xCheckSpatialNBDV( pcTmpCU, uiIdx, pDInfo, bCheckMcpDv, &cIDVInfo, DVFROM_ABOVE
#if H_3D_NBDV_REF
      , bDepthRefine 
#endif
      ) )
      return true;
  }

  //// ******* Search MCP blocks ******* /////
  if( cIDVInfo.m_bFound ) 
  {
    for( Int curPos = 0 ; curPos < IDV_CANDS ; curPos++ ) 
    {
      for(Int iList = 0; iList < (getSlice()->isInterB() ? 2: 1); iList ++)
      {
        if( cIDVInfo.m_bAvailab[iList][curPos] )
        {
          TComMv cDispVec = cIDVInfo.m_acMvCand[iList][ curPos ];
#if !(NTT_BUG_FIX_TK54)
          clipMv( cDispVec );
#endif
          pDInfo->m_acNBDV = cDispVec;
          pDInfo->m_aVIdxCan = cIDVInfo.m_aVIdxCan[iList][ curPos ];
#if H_3D_NBDV_REF
#if H_3D_FCO_VSP_DONBDV_E0163
          TComPic* picDepth  = NULL;

          picDepth  = getSlice()->getIvPic(true, getSlice()->getViewIndex() );
          if ( picDepth->getPicYuvRec() != NULL )  
          {
            cDispVec.setZero();
          }
          else // Go back with virtual depth
          {
            picDepth = getSlice()->getIvPic( true, pDInfo->m_aVIdxCan );
          }

          assert(picDepth != NULL);
#else
          TComPic* picDepth = getSlice()->getIvPic( true, pDInfo->m_aVIdxCan );
#if !BUG_FIX_TK65
          assert(picDepth!=NULL);
#endif
#endif

          if (picDepth && bDepthRefine)
          {
            estimateDVFromDM (pDInfo->m_aVIdxCan, uiPartIdx, picDepth, uiPartAddr, &cDispVec);
          }
          pDInfo->m_acDoNBDV = cDispVec;
#endif
          return true;
        }
      }
    }
  }

  TComMv defaultDV(0, 0);
  pDInfo->m_acNBDV = defaultDV;

  Int valid = 0;
  Int viewIndex = 0;
  for( UInt uiBId = 0; uiBId < getSlice()->getViewIndex() && valid==0; uiBId++ )
  {
    UInt        uiBaseId    = uiBId;
    TComPic*    pcBasePic   = getSlice()->getIvPic( false, uiBaseId );
    for( Int iRefListId = 0; ( iRefListId < (getSlice()->isInterB()? 2:1) ) && !getSlice()->isIntra() && valid==0; iRefListId++ )
    {
      RefPicList  eRefPicListTest = RefPicList( iRefListId );
      Int         iNumRefPics = getSlice()->getNumRefIdx( eRefPicListTest ) ;
      for( Int iRefIndex = 0; iRefIndex < iNumRefPics; iRefIndex++ )
      { 
        if(pcBasePic->getPOC() == getSlice()->getRefPic( eRefPicListTest, iRefIndex )->getPOC() 
          && pcBasePic->getViewIndex() == getSlice()->getRefPic( eRefPicListTest, iRefIndex )->getViewIndex())
        {
          valid=1;
          viewIndex = uiBaseId;
          break;
        }
      }
    }
  }
  if(valid)
  {
    pDInfo->m_aVIdxCan = viewIndex;
#if H_3D_NBDV_REF
    TComPic* picDepth = NULL;
#if H_3D_FCO_VSP_DONBDV_E0163
    picDepth  = getSlice()->getIvPic(true, getSlice()->getViewIndex() );
    if ( picDepth->getPicYuvRec() != NULL )  
    {
      defaultDV.setZero();
    }
    else // Go back with virtual depth
    {
      picDepth = getSlice()->getIvPic( true, viewIndex );
    }

    assert(picDepth != NULL);
#else
    picDepth = getSlice()->getIvPic( true, viewIndex );
#if !BUG_FIX_TK65
    assert(picDepth!=NULL);
#endif
#endif
    if (picDepth && bDepthRefine)
    {
      estimateDVFromDM(viewIndex, uiPartIdx, picDepth, uiPartAddr, &defaultDV ); // from base view
    }
    pDInfo->m_acDoNBDV = defaultDV;
#endif
  }
  return false; 
}

#if H_3D_NBDV_REF
Pel TComDataCU::getMcpFromDM(TComPicYuv* pcBaseViewDepthPicYuv, TComMv* mv, Int iBlkX, Int iBlkY, Int iBlkWidth, Int iBlkHeight, Int* aiShiftLUT )
{
  Int iPictureWidth  = pcBaseViewDepthPicYuv->getWidth();
  Int iPictureHeight = pcBaseViewDepthPicYuv->getHeight();
  
  Int depthStartPosX = Clip3(0,   iPictureWidth - 1,  iBlkX + ((mv->getHor()+2)>>2));
  Int depthStartPosY = Clip3(0,   iPictureHeight - 1, iBlkY + ((mv->getVer()+2)>>2));
  Int depthEndPosX   = Clip3(0,   iPictureWidth - 1,  iBlkX + iBlkWidth - 1 + ((mv->getHor()+2)>>2));
  Int depthEndPosY   = Clip3(0,   iPictureHeight - 1, iBlkY + iBlkHeight - 1 + ((mv->getVer()+2)>>2));

  Pel* depthTL  = pcBaseViewDepthPicYuv->getLumaAddr();
  Int depStride =  pcBaseViewDepthPicYuv->getStride();

  Pel  maxDepthVal = 0;
  maxDepthVal = std::max( maxDepthVal, depthTL[ (depthStartPosY) * depStride + depthStartPosX ]);      // Left Top
  maxDepthVal = std::max( maxDepthVal, depthTL[ (depthEndPosY)   * depStride + depthStartPosX ]);      // Left Bottom
  maxDepthVal = std::max( maxDepthVal, depthTL[ (depthStartPosY) * depStride + depthEndPosX   ]);      // Right Top
  maxDepthVal = std::max( maxDepthVal, depthTL[ (depthEndPosY)   * depStride + depthEndPosX   ]);      // Right Bottom

  return aiShiftLUT[ maxDepthVal ];
}

Void TComDataCU::estimateDVFromDM(Int refViewIdx, UInt uiPartIdx, TComPic* picDepth, UInt uiPartAddr, TComMv* cMvPred )
{
  if (picDepth)
  {
    UInt uiAbsPartAddrCurrCU = m_uiAbsIdxInLCU + uiPartAddr;
    Int iWidth, iHeight;
    getPartIndexAndSize( uiPartIdx, uiPartAddr, iWidth, iHeight ); // The modified value of uiPartAddr won't be used any more

    TComPicYuv* pcBaseViewDepthPicYuv = picDepth->getPicYuvRec();
    Int iBlkX = ( getAddr() % picDepth->getFrameWidthInCU() ) * g_uiMaxCUWidth  + g_auiRasterToPelX[ g_auiZscanToRaster[ uiAbsPartAddrCurrCU ] ];
    Int iBlkY = ( getAddr() / picDepth->getFrameWidthInCU() ) * g_uiMaxCUHeight + g_auiRasterToPelY[ g_auiZscanToRaster[ uiAbsPartAddrCurrCU ] ];

    Int* aiShiftLUT = getSlice()->getDepthToDisparityB(refViewIdx );

    Pel iDisp = getMcpFromDM( pcBaseViewDepthPicYuv, cMvPred, iBlkX, iBlkY, iWidth, iHeight, aiShiftLUT );
    cMvPred->setHor( iDisp );
#if !(NTT_BUG_FIX_TK54)
    clipMv(*cMvPred);
#endif
  }
}
#endif //H_3D_NBDV_REF


Bool TComDataCU::xCheckSpatialNBDV( TComDataCU* pcTmpCU, UInt uiIdx, DisInfo* pNbDvInfo, Bool bSearchForMvpDv, IDVInfo* paIDVInfo, UInt uiMvpDvPos
#if H_3D_NBDV_REF
, Bool bDepthRefine 
#endif
)
{
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
        Int refViewIdx  = pcTmpCU->getSlice()->getRefPic(eRefPicList, refId)->getViewIndex();
        if (refViewIdx != m_pcSlice->getViewIndex()) 
        {
#if !(NTT_BUG_FIX_TK54)
          clipMv(cMvPred);
#endif
          pNbDvInfo->m_acNBDV = cMvPred;
          pNbDvInfo->m_aVIdxCan = refViewIdx;
#if H_3D_NBDV_REF
          TComPic* picDepth = NULL;
          assert(getSlice()->getRefPic(eRefPicList, refId)->getPOC() == getSlice()->getPOC());          
#if H_3D_FCO_VSP_DONBDV_E0163
          picDepth  = getSlice()->getIvPic(true, getSlice()->getViewIndex() );
          if ( picDepth->getPicYuvRec() != NULL )  
          {
            cMvPred.setZero();
          }
          else// Go back with virtual depth
          {
            picDepth = getSlice()->getIvPic (true, refViewIdx );
          }
          assert(picDepth != NULL);
#else
          picDepth   = getSlice()->getIvPic (true, refViewIdx );
#if !BUG_FIX_TK65
          assert(picDepth != NULL);
#endif
#endif
          UInt uiPartIdx = 0;   //Notes from MTK: Please confirm that using 0 as partition index and partition address is correct for CU-level DoNBDV
          UInt uiPartAddr = 0;  //QC: confirmed

          if (picDepth && bDepthRefine)
            estimateDVFromDM(refViewIdx, uiPartIdx, picDepth, uiPartAddr, &cMvPred );

          pNbDvInfo->m_acDoNBDV = cMvPred;
#endif
          return true;
        }
        else if ( bSearchForMvpDv && cMvPred.getIDVFlag() && bTmpIsSkipped )
        {
          assert( uiMvpDvPos < IDV_CANDS );
          paIDVInfo->m_acMvCand[iList][ uiMvpDvPos ] = TComMv( cMvPred.getIDVHor(), cMvPred.getIDVVer() );
          //Notes from QC: DvMCP is implemented in a way that doesnot carry the reference view identifier as NBDV. It only works for CTC and needs to be fixed to be aligned with other part of the NBDV design.
          paIDVInfo->m_aVIdxCan[iList][ uiMvpDvPos ] = cMvPred.getIDVVId();
          paIDVInfo->m_bAvailab[iList][ uiMvpDvPos ] = true;
          paIDVInfo->m_bFound                        = true; 
        }
      }
    }
  }
  return false; 
}
 
Void TComDataCU::xDeriveRightBottomNbIdx(Int &riLCUIdxRBNb, Int &riPartIdxRBNb )
{
  UInt uiPartIdx = 0;
  UInt uiNumPartInCUWidth = m_pcPic->getNumPartInWidth();  
  Int uiLCUIdx = getAddr();

  UInt uiPartIdxRB;
  deriveRightBottomIdx(uiPartIdx, uiPartIdxRB );  
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
      riLCUIdxRBNb  = uiLCUIdx; 
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


Void TComDataCU::setDvInfoSubParts( DisInfo cDvInfo, UInt uiAbsPartIdx, UInt uiDepth )
{
  UInt uiCurrPartNumb = m_pcPic->getNumPartInCU() >> (uiDepth << 1);
  for (UInt ui = 0; ui < uiCurrPartNumb; ui++ )
  {
    m_pDvInfo[uiAbsPartIdx + ui] = cDvInfo;
  }
}
#if H_3D_VSP
Void TComDataCU::setDvInfoSubParts( DisInfo cDvInfo, UInt uiAbsPartIdx, UInt uiPUIdx, UInt uiDepth )
{
  setSubPartT<DisInfo>( cDvInfo, m_pDvInfo, uiAbsPartIdx, uiDepth, uiPUIdx );
}
#endif

Bool TComDataCU::xGetColDisMV( Int currCandPic, RefPicList eRefPicList, Int refidx, Int uiCUAddr, Int uiPartUnitIdx, TComMv& rcMv , Int & iTargetViewIdx, Int & iStartViewIdx )
{

  RefPicList  eColRefPicList = REF_PIC_LIST_0;
  Int iColViewIdx, iColRefViewIdx;
  TComPic *pColPic = getSlice()->getRefPic( eRefPicList, refidx);
  TComDataCU *pColCU = pColPic->getCU( uiCUAddr );
  iColViewIdx = pColCU->getSlice()->getViewIndex();
  if (pColCU->getPic()==0||pColCU->getPartitionSize(uiPartUnitIdx)==SIZE_NONE||pColCU->isIntra(uiPartUnitIdx))
  {
    return false;
  }
  for (Int ilist = 0; ilist < (pColCU->getSlice()->isInterB()? 2:1); ilist++) 
  {
    if(pColCU->getSlice()->isInterB())
    {
      eColRefPicList = RefPicList(ilist);
    }

    Int iColRefIdx = pColCU->getCUMvField(eColRefPicList)->getRefIdx(uiPartUnitIdx);

    if (iColRefIdx < 0)
    {
      continue;
    }

    iColRefViewIdx = pColCU->getSlice()->getRefPic(eColRefPicList, iColRefIdx)->getViewIndex();

    if ( iColViewIdx    == iColRefViewIdx ) // temporal vector
    {
      continue;
    }
    else 
    {
      if(getPic()->isTempIVRefValid(currCandPic, ilist,  iColRefIdx))
      {
        rcMv = pColCU->getCUMvField(eColRefPicList)->getMv(uiPartUnitIdx);
        rcMv.setIDVFlag(0);
        iTargetViewIdx  = iColRefViewIdx ;
        iStartViewIdx   = iColViewIdx   ;
        return true;    
      }
    }
  }

  return false;
}
#endif 
#if  H_3D_FAST_TEXTURE_ENCODING
Void 
TComDataCU::getIVNStatus       ( UInt uiPartIdx,  DisInfo* pDInfo, Bool& bIVFMerge, Int& iIVFMaxD)
{
  TComSlice*    pcSlice         = getSlice ();  
  Int iViewIndex = pDInfo->m_aVIdxCan;
  //--- get base CU/PU and check prediction mode ---
  TComPic*    pcBasePic   = pcSlice->getIvPic( false, iViewIndex );
  TComPicYuv* pcBaseRec   = pcBasePic->getPicYuvRec   ();

  UInt          uiPartAddr;
  Int           iWidth;
  Int           iHeight;
  getPartIndexAndSize( uiPartIdx, uiPartAddr, iWidth, iHeight );

  Int  iCurrPosX, iCurrPosY;
  pcBaseRec->getTopLeftSamplePos( getAddr(), getZorderIdxInCU() + uiPartAddr, iCurrPosX, iCurrPosY );

  iCurrPosX  += ( ( iWidth  - 1 ) >> 1 );
  iCurrPosY  += ( ( iHeight - 1 ) >> 1 );

  Bool depthRefineFlag = false; 
#if H_3D_NBDV_REF
  depthRefineFlag = m_pcSlice->getVPS()->getDepthRefinementFlag( m_pcSlice->getLayerIdInVps() ); 
#endif // H_3D_NBDV_REF

  TComMv      cDv = depthRefineFlag ? pDInfo->m_acDoNBDV : pDInfo->m_acNBDV; 
  if( depthRefineFlag )
  {
    cDv.setVer(0);
  }

  Int         iBasePosX   = Clip3( 0, pcBaseRec->getWidth () - 1, iCurrPosX + ( (cDv.getHor() + 2 ) >> 2 ) );
  Int         iBasePosY   = Clip3( 0, pcBaseRec->getHeight() - 1, iCurrPosY + ( (cDv.getVer() + 2 ) >> 2 )); 
  Int         iBaseLPosX   = Clip3( 0, pcBaseRec->getWidth () - 1, iCurrPosX - (iWidth >> 1) + ( (cDv.getHor() + 2 ) >> 2 ) );
  Int         iBaseLPosY   = Clip3( 0, pcBaseRec->getHeight() - 1, iCurrPosY + ( (cDv.getVer() + 2 ) >> 2 )); 
  Int         iBaseRPosX   = Clip3( 0, pcBaseRec->getWidth () - 1, iCurrPosX + (iWidth >> 1) + 1 + ( (cDv.getHor() + 2 ) >> 2 ) );
  Int         iBaseRPosY   = Clip3( 0, pcBaseRec->getHeight() - 1, iCurrPosY + ( (cDv.getVer() + 2 ) >> 2 )); 
  Int         iBaseUPosX   = Clip3( 0, pcBaseRec->getWidth () - 1, iCurrPosX + ( (cDv.getHor() + 2 ) >> 2 ) );
  Int         iBaseUPosY   = Clip3( 0, pcBaseRec->getHeight() - 1, iCurrPosY - (iHeight >> 1) + ( (cDv.getVer() + 2 ) >> 2 )); 
  Int         iBaseDPosX   = Clip3( 0, pcBaseRec->getWidth () - 1, iCurrPosX + ( (cDv.getHor() + 2 ) >> 2 ) );
  Int         iBaseDPosY   = Clip3( 0, pcBaseRec->getHeight() - 1, iCurrPosY + (iHeight >> 1) + 1 + ( (cDv.getVer() + 2 ) >> 2 )); 

  Int         iBaseCUAddr;
  Int         iBaseAbsPartIdx;
  Int         iBaseLCUAddr;
  Int         iBaseLAbsPartIdx;
  Int         iBaseRCUAddr;
  Int         iBaseRAbsPartIdx;
  Int         iBaseUCUAddr;
  Int         iBaseUAbsPartIdx;
  Int         iBaseDCUAddr;
  Int         iBaseDAbsPartIdx;
  pcBaseRec->getCUAddrAndPartIdx( iBasePosX , iBasePosY , iBaseCUAddr, iBaseAbsPartIdx );
  pcBaseRec->getCUAddrAndPartIdx( iBaseLPosX , iBaseLPosY , iBaseLCUAddr, iBaseLAbsPartIdx );
  pcBaseRec->getCUAddrAndPartIdx( iBaseRPosX , iBaseRPosY , iBaseRCUAddr, iBaseRAbsPartIdx );
  pcBaseRec->getCUAddrAndPartIdx( iBaseUPosX , iBaseUPosY , iBaseUCUAddr, iBaseUAbsPartIdx );
  pcBaseRec->getCUAddrAndPartIdx( iBaseDPosX , iBaseDPosY , iBaseDCUAddr, iBaseDAbsPartIdx );
  TComDataCU* pcBaseCU    = pcBasePic->getCU( iBaseCUAddr );
  TComDataCU* pcBaseLCU    = pcBasePic->getCU( iBaseLCUAddr );
  TComDataCU* pcBaseRCU    = pcBasePic->getCU( iBaseRCUAddr );
  TComDataCU* pcBaseUCU    = pcBasePic->getCU( iBaseUCUAddr );
  TComDataCU* pcBaseDCU    = pcBasePic->getCU( iBaseDCUAddr );
  bIVFMerge = pcBaseLCU->getMergeFlag( iBaseLAbsPartIdx ) && pcBaseCU->getMergeFlag( iBaseAbsPartIdx ) && pcBaseRCU->getMergeFlag( iBaseRAbsPartIdx ) && pcBaseUCU->getMergeFlag( iBaseUAbsPartIdx ) && pcBaseDCU->getMergeFlag( iBaseDAbsPartIdx );
  Int aiDepthL[5]; //depth level
  aiDepthL[0] = pcBaseCU->getDepth(iBaseAbsPartIdx);
  aiDepthL[1] = pcBaseLCU->getDepth(iBaseLAbsPartIdx);
  aiDepthL[2] = pcBaseRCU->getDepth(iBaseRAbsPartIdx);
  aiDepthL[3] = pcBaseUCU->getDepth(iBaseUAbsPartIdx);
  aiDepthL[4] = pcBaseDCU->getDepth(iBaseDAbsPartIdx);
  for (Int i = 0; i < 5; i++)
  {
    if (iIVFMaxD < aiDepthL[i])
      iIVFMaxD = aiDepthL[i];
  }
}
#endif

#if H_3D_SPIVMP
Void TComDataCU::getSPPara(Int iPUWidth, Int iPUHeight, Int& iNumSP, Int& iNumSPInOneLine, Int& iSPWidth, Int& iSPHeight)
{
  Int iSubPUSize = 1<<getSlice()->getVPS()->getSubPULog2Size(getSlice()->getLayerId());
  if( getSlice()->getIsDepth() )
  {
    iSubPUSize = 1<<getSlice()->getVPS()->getSubPUMPILog2Size(getSlice()->getLayerId());
  }

  iNumSPInOneLine = iPUWidth/iSubPUSize;
#if !HS_SP_SIMP_J0066
  iNumSPInOneLine = iNumSPInOneLine < 1 ? 1: iNumSPInOneLine;
#endif
  Int iNumSPInOneColumn = iPUHeight/iSubPUSize;
#if !HS_SP_SIMP_J0066
  iNumSPInOneColumn = iNumSPInOneColumn < 1 ? 1: iNumSPInOneColumn;
#else
  iNumSPInOneLine = (iPUHeight % iSubPUSize != 0 || iPUWidth % iSubPUSize != 0 ) ? 1 : iNumSPInOneLine;
  iNumSPInOneColumn = (iPUHeight % iSubPUSize != 0  || iPUWidth % iSubPUSize != 0 ) ? 1 : iNumSPInOneColumn;
#endif
  iNumSP = iNumSPInOneLine * iNumSPInOneColumn;

  iSPWidth = iNumSPInOneLine == 1 ? iPUWidth: iSubPUSize; 
  iSPHeight = iNumSPInOneColumn == 1 ? iPUHeight: iSubPUSize; 
}

Void TComDataCU::getSPAbsPartIdx(UInt uiBaseAbsPartIdx, Int iWidth, Int iHeight, Int iPartIdx, Int iNumPartLine, UInt& ruiPartAddr )
{
  uiBaseAbsPartIdx += m_uiAbsIdxInLCU;
  Int iBasePelX = g_auiRasterToPelX[g_auiZscanToRaster[uiBaseAbsPartIdx]];
  Int iBasePelY = g_auiRasterToPelY[g_auiZscanToRaster[uiBaseAbsPartIdx]];
  Int iCurrPelX = iBasePelX + iPartIdx%iNumPartLine * iWidth;
  Int iCurrPelY = iBasePelY + iPartIdx/iNumPartLine * iHeight;
  Int iCurrRaster = iCurrPelY / getPic()->getMinCUHeight() * getPic()->getNumPartInWidth() + iCurrPelX/getPic()->getMinCUWidth();
  ruiPartAddr = g_auiRasterToZscan[iCurrRaster];
  ruiPartAddr -= m_uiAbsIdxInLCU;  
}

Void TComDataCU::setInterDirSP( UInt uiDir, UInt uiAbsPartIdx, Int iWidth, Int iHeight )
{
  uiAbsPartIdx += getZorderIdxInCU();
  Int iStartPelX = g_auiRasterToPelX[g_auiZscanToRaster[uiAbsPartIdx]];
  Int iStartPelY = g_auiRasterToPelY[g_auiZscanToRaster[uiAbsPartIdx]];
  Int iEndPelX = iStartPelX + iWidth;
  Int iEndPelY = iStartPelY + iHeight;

  Int iCurrRaster, uiPartAddr;

  for (Int i=iStartPelY; i<iEndPelY; i+=getPic()->getMinCUHeight())
  {
    for (Int j=iStartPelX; j < iEndPelX; j += getPic()->getMinCUWidth())
    {
      iCurrRaster = i / getPic()->getMinCUHeight() * getPic()->getNumPartInWidth() + j/getPic()->getMinCUWidth();
      uiPartAddr = g_auiRasterToZscan[iCurrRaster];
      uiPartAddr -= getZorderIdxInCU();  

      m_puhInterDir[uiPartAddr] = uiDir;
    }
  }
}
#endif

#if H_3D_IV_MERGE
Bool
TComDataCU::getInterViewMergeCands(UInt uiPartIdx, Int* paiPdmRefIdx, TComMv* pacPdmMv, DisInfo* pDInfo, Int* availableMcDc , Bool bIsDepth           
#if H_3D_SPIVMP
, TComMvField* pcMvFieldSP, UChar* puhInterDirSP
#endif
, Bool bICFlag
)
{
  TComSlice*    pcSlice = getSlice ();  
  Int iViewIndex        = pDInfo->m_aVIdxCan;

  //--- get base CU/PU and check prediction mode ---
  TComPic*    pcBasePic   = pcSlice->getIvPic( bIsDepth, iViewIndex );
  TComPicYuv* pcBaseRec   = pcBasePic->getPicYuvRec   ();

  UInt          uiPartAddr;
  Int           iWidth;
  Int           iHeight;
  getPartIndexAndSize( uiPartIdx, uiPartAddr, iWidth, iHeight );

  Int  iCurrPosX, iCurrPosY;
  pcBaseRec->getTopLeftSamplePos( getAddr(), getZorderIdxInCU() + uiPartAddr, iCurrPosX, iCurrPosY );

#if !H_3D_SPIVMP
  iCurrPosX  += ( iWidth  >> 1 );
  iCurrPosY  += ( iHeight >> 1 );
#endif

  Bool depthRefineFlag = false; 
#if H_3D_NBDV_REF
  depthRefineFlag = m_pcSlice->getVPS()->getDepthRefinementFlag( m_pcSlice->getLayerIdInVps() ); 
#endif // H_3D_NBDV_REF

  TComMv      cDv = depthRefineFlag ? pDInfo->m_acDoNBDV : pDInfo->m_acNBDV; 
  if( depthRefineFlag )
  {
    cDv.setVer(0);
  }

  Bool abPdmAvailable[8] =  {false, false, false, false, false, false, false, false};
#if H_3D_NBDV
  for( Int i = 0; i < 8; i++)
  {
    pacPdmMv[i].setIDVFlag   (false);
  }
#endif

  if(!bICFlag)
  {

#if H_3D_SPIVMP
    ////////////////////////////////
    //////////sub-PU IvMC///////////
    ////////////////////////////////
    if(!m_pcSlice->getIsDepth())
    {
      if (!getDBBPFlag(0))
      {
        Int iNumSPInOneLine, iNumSP, iSPWidth, iSPHeight;
        getSPPara(iWidth, iHeight, iNumSP, iNumSPInOneLine, iSPWidth, iSPHeight);

        for (Int i=0; i<iNumSP; i++)
        {
          puhInterDirSP[i] = 0;
          pcMvFieldSP[2*i].getMv().set(0, 0);
          pcMvFieldSP[2*i+1].getMv().set(0,0);
          pcMvFieldSP[2*i].setRefIdx(-1);
          pcMvFieldSP[2*i+1].setRefIdx(-1);
        }

        Int         iBaseCUAddr;
        Int         iBaseAbsPartIdx;
        TComDataCU* pcBaseCU;
        Int iPartition = 0;

        Int iDelX = iSPWidth/2;
        Int iDelY = iSPHeight/2;

        Int         iCenterPosX = iCurrPosX + ( ( iWidth /  iSPWidth ) >> 1 )  * iSPWidth + ( iSPWidth >> 1 );
        Int         iCenterPosY = iCurrPosY + ( ( iHeight /  iSPHeight ) >> 1 )  * iSPHeight + (iSPHeight >> 1);
        Int         iRefCenterCUAddr, iRefCenterAbsPartIdx;

        if(iWidth == iSPWidth && iHeight == iSPHeight)
        {
          iCenterPosX = iCurrPosX + (iWidth >> 1);
          iCenterPosY = iCurrPosY + (iHeight >> 1);
        }

        Int iRefCenterPosX   = Clip3( 0, pcBaseRec->getWidth () - 1, iCenterPosX + ( (cDv.getHor() + 2 ) >> 2 ) );
        Int iRefCenterPosY   = Clip3( 0, pcBaseRec->getHeight() - 1, iCenterPosY + ( (cDv.getVer() + 2 ) >> 2 ) ); 

        pcBaseRec->getCUAddrAndPartIdx( iRefCenterPosX , iRefCenterPosY , iRefCenterCUAddr, iRefCenterAbsPartIdx );
        TComDataCU* pcDefaultCU    = pcBasePic->getCU( iRefCenterCUAddr );
        if(!( pcDefaultCU->getPredictionMode( iRefCenterAbsPartIdx ) == MODE_INTRA ))
        {
          for( UInt uiCurrRefListId = 0; uiCurrRefListId < 2; uiCurrRefListId++ )       
          {
            RefPicList  eCurrRefPicList = RefPicList( uiCurrRefListId );
            Bool stopLoop = false;
            for(Int iLoop = 0; iLoop < 2 && !stopLoop; ++iLoop)
            {
              RefPicList eDefaultRefPicList = (iLoop ==1)? RefPicList( 1 -  uiCurrRefListId ) : RefPicList( uiCurrRefListId );
              TComMvField cDefaultMvField;
              pcDefaultCU->getMvField( pcDefaultCU, iRefCenterAbsPartIdx, eDefaultRefPicList, cDefaultMvField );
              Int         iDefaultRefIdx     = cDefaultMvField.getRefIdx();
              if (iDefaultRefIdx >= 0)
              {
                Int iDefaultRefPOC = pcDefaultCU->getSlice()->getRefPOC(eDefaultRefPicList, iDefaultRefIdx);
                if (iDefaultRefPOC != pcSlice->getPOC())    
                {
                  for (Int iPdmRefIdx = 0; iPdmRefIdx < pcSlice->getNumRefIdx( eCurrRefPicList ); iPdmRefIdx++)
                  {
                    if (iDefaultRefPOC == pcSlice->getRefPOC(eCurrRefPicList, iPdmRefIdx))
                    {
                      abPdmAvailable[ uiCurrRefListId ] = true;
                      TComMv cMv(cDefaultMvField.getHor(), cDefaultMvField.getVer());
#if H_3D_NBDV 
#if H_3D_IV_MERGE
                      if( !bIsDepth )
                      {
#endif
                        cMv.setIDVFlag   (true);
                        cMv.setIDVHor    (cDv.getHor());                  
                        cMv.setIDVVer    (cDv.getVer());  
                        cMv.setIDVVId    (iViewIndex); 
#if H_3D_IV_MERGE
                      }
#endif
#endif
#if !(NTT_BUG_FIX_TK54)
                      clipMv( cMv );
#endif
                      paiPdmRefIdx  [ uiCurrRefListId ] = iPdmRefIdx;
                      pacPdmMv      [ uiCurrRefListId ] = cMv;
                      stopLoop = true;
                      break;
                    }
                  }
                }
              }
            }
          }
        }
        availableMcDc[0] = ( abPdmAvailable[0]? 1 : 0) + (abPdmAvailable[1]? 2 : 0);

        if(availableMcDc[0])
        {

          Int         iBasePosX, iBasePosY;
          for (Int i=iCurrPosY; i < iCurrPosY + iHeight; i += iSPHeight)
          {
            for (Int j = iCurrPosX; j < iCurrPosX + iWidth; j += iSPWidth)
            {
              iBasePosX   = Clip3( 0, pcBaseRec->getWidth () - 1, j + iDelX + ( (cDv.getHor() + 2 ) >> 2 ));
              iBasePosY   = Clip3( 0, pcBaseRec->getHeight() - 1, i + iDelY + ( (cDv.getVer() + 2 ) >> 2 )); 

              pcBaseRec->getCUAddrAndPartIdx( iBasePosX , iBasePosY, iBaseCUAddr, iBaseAbsPartIdx );
              pcBaseCU    = pcBasePic->getCU( iBaseCUAddr );
              if(!( pcBaseCU->getPredictionMode( iBaseAbsPartIdx ) == MODE_INTRA ))
              {
                for( UInt uiCurrRefListId = 0; uiCurrRefListId < 2; uiCurrRefListId++ )
                {
                  RefPicList  eCurrRefPicList = RefPicList( uiCurrRefListId );
                  Bool bLoop_stop = false;
                  for(Int iLoop = 0; iLoop < 2 && !bLoop_stop; ++iLoop)
                  {
                    RefPicList eBaseRefPicList = (iLoop ==1)? RefPicList( 1 -  uiCurrRefListId ) : RefPicList( uiCurrRefListId );
                    TComMvField cBaseMvField;
                    pcBaseCU->getMvField( pcBaseCU, iBaseAbsPartIdx, eBaseRefPicList, cBaseMvField );
                    Int         iBaseRefIdx     = cBaseMvField.getRefIdx();
                    if (iBaseRefIdx >= 0)
                    {
                      Int iBaseRefPOC = pcBaseCU->getSlice()->getRefPOC(eBaseRefPicList, iBaseRefIdx);
                      if (iBaseRefPOC != pcSlice->getPOC())    
                      {
                        for (Int iPdmRefIdx = 0; iPdmRefIdx < pcSlice->getNumRefIdx( eCurrRefPicList ); iPdmRefIdx++)
                        {
                          if (iBaseRefPOC == pcSlice->getRefPOC(eCurrRefPicList, iPdmRefIdx))
                          {
                            abPdmAvailable[ uiCurrRefListId ] = true;
                            TComMv cMv(cBaseMvField.getHor(), cBaseMvField.getVer());

                            if( !bIsDepth )
                            {
                              cMv.setIDVFlag   (true);
                              cMv.setIDVHor    (cDv.getHor());                  
                              cMv.setIDVVer    (cDv.getVer());  
                              cMv.setIDVVId    (iViewIndex); 
                            }

#if !(NTT_BUG_FIX_TK54)
                            clipMv( cMv );
#endif
                            bLoop_stop = true;

                            pcMvFieldSP[2*iPartition + uiCurrRefListId].setMvField(cMv, iPdmRefIdx);
                            break;
                          }
                        }
                      }
                    }
                  }
                }
              }

              puhInterDirSP[iPartition] = (pcMvFieldSP[2*iPartition].getRefIdx()!=-1 ? 1: 0) + (pcMvFieldSP[2*iPartition+1].getRefIdx()!=-1 ? 2: 0);
              if (puhInterDirSP[iPartition] == 0)
              {
                puhInterDirSP[iPartition] = availableMcDc[0];
                pcMvFieldSP[2*iPartition].setMvField(pacPdmMv[0], paiPdmRefIdx[0]);
                pcMvFieldSP[2*iPartition + 1].setMvField(pacPdmMv[1], paiPdmRefIdx[1]);

              }
              if (iSPHeight + iSPWidth == 12)
              {
                if (puhInterDirSP[iPartition] == 3)
                {
                  puhInterDirSP[iPartition] = 1;
                  pcMvFieldSP[2*iPartition + 1].setMvField(TComMv(0,0), -1);
                }
              }
              iPartition ++;
            }
          }
        }
      }

      iCurrPosX  += ( iWidth  >> 1 );
      iCurrPosY  += ( iHeight >> 1 );
    }
#endif

    ////////////////////////////////
    /////// IvMC + IvMCShift ///////
    ////////////////////////////////

#if H_3D_SPIVMP
    if(m_pcSlice->getIsDepth())
    {
      iCurrPosX  += ( iWidth  >> 1 );
      iCurrPosY  += ( iHeight >> 1 );
    }
    for(Int iLoopCan = ( (m_pcSlice->getIsDepth() || getDBBPFlag(0)) ? 0 : 1 ); iLoopCan < ( 2 - m_pcSlice->getIsDepth() ); iLoopCan ++) 
#else
    for(Int iLoopCan = 0; iLoopCan < 2; iLoopCan ++)
#endif
    {
      // iLoopCan == 0 --> IvMC
      // iLoopCan == 1 --> IvMCShift 

      Int         iBaseCUAddr;
      Int         iBaseAbsPartIdx;

      Int offsetW = (iLoopCan == 0) ? 0 : ( ((iWidth /2)*4) + 4 );
      Int offsetH = (iLoopCan == 0) ? 0 : ( ((iHeight/2)*4) + 4 );

      Int         iBasePosX   = Clip3( 0, pcBaseRec->getWidth () - 1, iCurrPosX + ( (cDv.getHor() + offsetW + 2 ) >> 2 ) );
      Int         iBasePosY   = Clip3( 0, pcBaseRec->getHeight() - 1, iCurrPosY + ( (cDv.getVer() + offsetH + 2 ) >> 2 ) ); 
      pcBaseRec->getCUAddrAndPartIdx( iBasePosX , iBasePosY , iBaseCUAddr, iBaseAbsPartIdx );

      TComDataCU* pcBaseCU    = pcBasePic->getCU( iBaseCUAddr );
      if(!( pcBaseCU->getPredictionMode( iBaseAbsPartIdx ) == MODE_INTRA ))
      {
        // Loop reference picture list of current slice (X in spec). 
        for( UInt uiCurrRefListId = 0; uiCurrRefListId < 2; uiCurrRefListId++ )       
        {
          RefPicList  eCurrRefPicList = RefPicList( uiCurrRefListId );

          Bool stopLoop = false;
          // Loop reference picture list of candidate slice (Y in spec)
          for(Int iLoop = 0; iLoop < 2 && !stopLoop; ++iLoop)
          {
            RefPicList eBaseRefPicList = (iLoop ==1)? RefPicList( 1 -  uiCurrRefListId ) : RefPicList( uiCurrRefListId );
            TComMvField cBaseMvField;
            pcBaseCU->getMvField( pcBaseCU, iBaseAbsPartIdx, eBaseRefPicList, cBaseMvField );
            Int         iBaseRefIdx     = cBaseMvField.getRefIdx();
            if (iBaseRefIdx >= 0)
            {
              Int iBaseRefPOC = pcBaseCU->getSlice()->getRefPOC(eBaseRefPicList, iBaseRefIdx);
              if (iBaseRefPOC != pcSlice->getPOC())    
              {
                for (Int iPdmRefIdx = 0; iPdmRefIdx < pcSlice->getNumRefIdx( eCurrRefPicList ); iPdmRefIdx++)
                {
                  if (iBaseRefPOC == pcSlice->getRefPOC(eCurrRefPicList, iPdmRefIdx))
                  {
                    abPdmAvailable[ (uiCurrRefListId + (iLoopCan<<2)) ] = true;
                    TComMv cMv(cBaseMvField.getHor(), cBaseMvField.getVer());
#if H_3D_NBDV 
#if H_3D_IV_MERGE
                    if( !bIsDepth )
                    {
#endif
                      cMv.setIDVFlag   (true);
                      cMv.setIDVHor    (cDv.getHor());                  
                      cMv.setIDVVer    (cDv.getVer());  
                      cMv.setIDVVId    (iViewIndex); 
#if H_3D_IV_MERGE
                    }
#endif
#endif
#if !(NTT_BUG_FIX_TK54)
                    clipMv( cMv );
#endif
                    paiPdmRefIdx  [ (uiCurrRefListId + (iLoopCan<<2)) ] = iPdmRefIdx;
                    pacPdmMv      [ (uiCurrRefListId + (iLoopCan<<2)) ] = cMv;
                    stopLoop = true;
                    break;
                  }
                }
              }
            }
          }
        }
      }
    }
#if H_3D_SPIVMP
    for(Int iLoopCan = ( (m_pcSlice->getIsDepth() || getDBBPFlag(0)) ? 0 : 1 ); iLoopCan < ( 2 - m_pcSlice->getIsDepth() ); iLoopCan ++)
#else
    for(Int iLoopCan = 0; iLoopCan < 2; iLoopCan ++)
#endif
    {
      availableMcDc[(iLoopCan << 1)] = ( abPdmAvailable[(iLoopCan<<2)] ? 1 : 0 ) + ( abPdmAvailable[1 + (iLoopCan<<2)] ? 2 : 0);
    }

  }

  ////////////////////////////////
  /////// IvDC + IvDCShift ///////
  ////////////////////////////////

  if( !getSlice()->getIsDepth() )
  {
    for( Int iRefListId = 0; iRefListId < 2 ; iRefListId++ )
    {
      RefPicList  eRefPicListDMV       = RefPicList( iRefListId );
      Int         iNumRefPics       = pcSlice->getNumRefIdx( eRefPicListDMV );
      for( Int iPdmRefIdx = 0; iPdmRefIdx < iNumRefPics; iPdmRefIdx++ )
      {
        if(( pcSlice->getRefPOC( eRefPicListDMV, iPdmRefIdx ) == pcSlice->getPOC()) && (pcSlice->getRefPic( eRefPicListDMV, iPdmRefIdx )->getViewIndex() == pDInfo->m_aVIdxCan))
        {
          for(Int iLoopCan = 0; iLoopCan < 2; iLoopCan ++)
          {
            Int ioffsetDV = (iLoopCan == 0) ? 0 : 4;
            abPdmAvailable[ iRefListId + 2 + (iLoopCan<<2) ] = true;
            paiPdmRefIdx  [ iRefListId + 2 + (iLoopCan<<2) ] = iPdmRefIdx;
#if H_3D_NBDV_REF
            TComMv cMv = depthRefineFlag ? pDInfo->m_acDoNBDV : pDInfo->m_acNBDV; 
#endif
            cMv.setHor( cMv.getHor() + ioffsetDV );
#if H_3D_IV_MERGE 
            if( bIsDepth )
            {
              cMv.setHor((cMv.getHor()+2)>>2); 
            }
#endif
            cMv.setVer( 0 );
#if !(NTT_BUG_FIX_TK54)
            clipMv( cMv );
#endif
            pacPdmMv      [iRefListId + 2 + (iLoopCan<<2)] = cMv;
          }
          break;
        }
      }
    }
    for(Int iLoopCan = 0; iLoopCan < 2; iLoopCan ++)
    {
      availableMcDc[1 + (iLoopCan << 1)] = ( abPdmAvailable[2 + (iLoopCan<<2)] ? 1 : 0 ) + ( abPdmAvailable[3 + (iLoopCan<<2)] ? 2 : 0 );
    }
  }
  return false;
}
#endif
#if H_3D_ARP
Void TComDataCU::setARPWSubParts ( UChar w, UInt uiAbsPartIdx, UInt uiDepth )
{
  assert( sizeof( *m_puhARPW) == 1 );
  memset( m_puhARPW + uiAbsPartIdx, w, m_pcPic->getNumPartInCU() >> ( 2 * uiDepth ) );
}
#endif

#if H_3D_IC
Void TComDataCU::setICFlagSubParts( Bool bICFlag, UInt uiAbsPartIdx, UInt uiPartIdx, UInt uiDepth )
{
  memset( m_pbICFlag + uiAbsPartIdx, bICFlag, (m_pcPic->getNumPartInCU() >> ( 2 * uiDepth ))*sizeof(Bool) );
}

Bool TComDataCU::isICFlagRequired( UInt uiAbsPartIdx )
{
  UInt uiPartAddr;
  UInt iNumbPart;

  if( !( getPartitionSize( uiAbsPartIdx ) == SIZE_2Nx2N ) )
  {
    return false;
  }

  if( getSlice()->getIcSkipParseFlag() )
  {
    if( getMergeFlag( uiAbsPartIdx ) && getMergeIndex( uiAbsPartIdx ) == 0 )
    {
      return false;
    }
  }

  if( getMergeFlag( uiAbsPartIdx ) )
  {
    return true;
  }


  Int iWidth, iHeight;

  iNumbPart = ( getPartitionSize( uiAbsPartIdx ) == SIZE_2Nx2N ? 1 : ( getPartitionSize( uiAbsPartIdx ) == SIZE_NxN ? 4 : 2 ) );

  for(UInt i = 0; i < iNumbPart; i++)
  {
    getPartIndexAndSize( i, uiPartAddr, iWidth, iHeight, uiAbsPartIdx, true );
    uiPartAddr += uiAbsPartIdx;

    for(UInt uiRefIdx = 0; uiRefIdx < 2; uiRefIdx++)
    {
      RefPicList eRefList = uiRefIdx ? REF_PIC_LIST_1 : REF_PIC_LIST_0;
      Int iBestRefIdx = getCUMvField(eRefList)->getRefIdx(uiPartAddr);

      if( ( getInterDir( uiPartAddr ) & ( uiRefIdx+1 ) ) && iBestRefIdx >= 0 && getSlice()->getViewIndex() != getSlice()->getRefPic( eRefList, iBestRefIdx )->getViewIndex() )
      {
        return true;
      }
    }
  }

  return false;
}
#endif
#if H_3D_DIM_DMM
Void TComDataCU::setDmmWedgeTabIdxSubParts( UInt tabIdx, UInt dmmType, UInt uiAbsPartIdx, UInt uiDepth )
{
  UInt uiCurrPartNumb = m_pcPic->getNumPartInCU() >> (uiDepth << 1);
  for( UInt ui = 0; ui < uiCurrPartNumb; ui++ ) { m_dmmWedgeTabIdx[dmmType][uiAbsPartIdx+ui] = tabIdx; }
}
#endif

#if H_3D_VSP
Void TComDataCU::setMvFieldPUForVSP( TComDataCU* pcCU, UInt partAddr, Int width, Int height, RefPicList eRefPicList, Int iRefIdx, Int &vspSize )
{
  // Get depth reference
  Int depthRefViewIdx = pcCU->getDvInfo(partAddr).m_aVIdxCan;
  
#if H_3D_FCO_VSP_DONBDV_E0163
  TComPic* pRefPicBaseDepth = 0;
  Bool     bIsCurrDepthCoded = false;
  pRefPicBaseDepth  = pcCU->getSlice()->getIvPic( true, pcCU->getSlice()->getViewIndex() );
  if ( pRefPicBaseDepth->getPicYuvRec() != NULL  ) 
  {
    bIsCurrDepthCoded = true;
  }
  else 
  {
    pRefPicBaseDepth = pcCU->getSlice()->getIvPic (true, depthRefViewIdx );
  }
#else
  TComPic* pRefPicBaseDepth = pcCU->getSlice()->getIvPic (true, depthRefViewIdx );
#endif
  assert(pRefPicBaseDepth != NULL);
  TComPicYuv* pcBaseViewDepthPicYuv = pRefPicBaseDepth->getPicYuvRec();
  assert(pcBaseViewDepthPicYuv != NULL);
  pcBaseViewDepthPicYuv->extendPicBorder();

  // Get texture reference
  assert(iRefIdx >= 0);
  TComPic* pRefPicBaseTxt = pcCU->getSlice()->getRefPic( eRefPicList, iRefIdx );
  TComPicYuv* pcBaseViewTxtPicYuv = pRefPicBaseTxt->getPicYuvRec();
  assert(pcBaseViewTxtPicYuv != NULL);

  // Initialize LUT according to the reference viewIdx
  Int txtRefViewIdx = pRefPicBaseTxt->getViewIndex();
  Int* pShiftLUT    = pcCU->getSlice()->getDepthToDisparityB( txtRefViewIdx );
  assert( txtRefViewIdx < pcCU->getSlice()->getViewIndex() );

  // prepare Dv to access depth map or reference view
  TComMv cDv  = pcCU->getDvInfo(partAddr).m_acNBDV;
  pcCU->clipMv(cDv);

#if H_3D_FCO_VSP_DONBDV_E0163
  if ( bIsCurrDepthCoded )
  {
      cDv.setZero();
  }
#endif

  // fetch virtual depth map & convert depth to motion vector, which are stored in the motion memory
  xSetMvFieldForVSP( pcCU, pcBaseViewDepthPicYuv, &cDv, partAddr, width, height, pShiftLUT, eRefPicList, iRefIdx, pcCU->getSlice()->getIsDepth(), vspSize );
}

Void TComDataCU::xSetMvFieldForVSP( TComDataCU *cu, TComPicYuv *picRefDepth, TComMv *dv, UInt partAddr, Int width, Int height, Int *shiftLUT, RefPicList refPicList, Int refIdx, Bool isDepth, Int &vspSize )
{
  TComCUMvField *cuMvField = cu->getCUMvField( refPicList );
  Int partAddrRasterSubPULine  = g_auiZscanToRaster[ partAddr ];
  Int numPartsLine    = cu->getPic()->getNumPartInWidth();

  Int nTxtPerMvInfoX = 4; // cu->getPic()->getMinCUWidth();
  Int nTxtPerMvInfoY = 4; // cu->getPic()->getMinCUHeight();

  Int refDepStride = picRefDepth->getStride();

  TComMv tmpMv(0, 0);
  tmpMv.setIDVFlag(false);

  Int refDepOffset  = ( (dv->getHor()+2) >> 2 ) + ( (dv->getVer()+2) >> 2 ) * refDepStride;
  Pel *refDepth     = picRefDepth->getLumaAddr( cu->getAddr(), cu->getZorderIdxInCU() + partAddr ) + refDepOffset;

  if ((height % 8))
  {
    vspSize = 1; // 8x4
  }
  else if ((width % 8))
  {
    vspSize = 0; // 4x8
  }
  else
  {
    Bool ULvsBR, URvsBL;
    ULvsBR = refDepth[0]       < refDepth[refDepStride * (height-1) + width-1];
    URvsBL = refDepth[width-1] < refDepth[refDepStride * (height-1)];
    vspSize = ( ULvsBR ^ URvsBL ) ? 0 : 1;
  }
  
  Int subBlockW, subBlockH;
  if (vspSize)
  {
    subBlockW = 8;
    subBlockH = 4;
  }
  else
  {
    subBlockW = 4;
    subBlockH = 8;
  }
  
  Int numPartsInSubPUW = subBlockW / nTxtPerMvInfoX;
  Int numPartsInSubPUH = subBlockH / nTxtPerMvInfoY * numPartsLine;

  for( Int y=0; y<height; y+=subBlockH, partAddrRasterSubPULine+=numPartsInSubPUH )
  {
    Pel *refDepthTmp[4];
    refDepthTmp[0] = refDepth + refDepStride * y;
    refDepthTmp[1] = refDepthTmp[0] + subBlockW - 1;
    refDepthTmp[2] = refDepthTmp[0] + refDepStride * (subBlockH - 1);
    refDepthTmp[3] = refDepthTmp[2] + subBlockW - 1;

    Int partAddrRasterSubPU = partAddrRasterSubPULine;
    for( Int x=0; x<width; x+=subBlockW, partAddrRasterSubPU+=numPartsInSubPUW )
    {
      Pel  maxDepthVal;
      maxDepthVal = refDepthTmp[0][x];
      maxDepthVal = std::max( maxDepthVal, refDepthTmp[1][x]);
      maxDepthVal = std::max( maxDepthVal, refDepthTmp[2][x]);
      maxDepthVal = std::max( maxDepthVal, refDepthTmp[3][x]);
      tmpMv.setHor( (Short) shiftLUT[ maxDepthVal ] );

      Int partAddrRasterPartLine = partAddrRasterSubPU;
      for( Int sY=0; sY<numPartsInSubPUH; sY+=numPartsLine, partAddrRasterPartLine += numPartsLine )
      {
        Int partAddrRasterPart = partAddrRasterPartLine;
        for( Int sX=0; sX<numPartsInSubPUW; sX+=1, partAddrRasterPart++ )
        {
          cuMvField->setMv    ( g_auiRasterToZscan[ partAddrRasterPart ], tmpMv );
          cuMvField->setRefIdx( g_auiRasterToZscan[ partAddrRasterPart ], refIdx );
        }
      }
    }
  }

  vspSize = (vspSize<<2)+1;

}
#endif

//! \}
