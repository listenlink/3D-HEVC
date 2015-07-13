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

/** \file     TComDataCU.cpp
    \brief    CU data structure
    \todo     not all entities are documented
*/

#include "TComDataCU.h"
#include "TComTU.h"
#include "TComPic.h"

//! \ingroup TLibCommon
//! \{

// ====================================================================================================================
// Constructor / destructor / create / destroy
// ====================================================================================================================

TComDataCU::TComDataCU()
{
  m_pcPic              = NULL;
  m_pcSlice            = NULL;
  m_puhDepth           = NULL;

  m_skipFlag           = NULL;
#if H_3D
  m_bDISFlag           = NULL;
  m_uiDISType          = NULL;
#endif
  m_pePartSize         = NULL;
  m_pePredMode         = NULL;
  m_CUTransquantBypass = NULL;
  m_puhWidth           = NULL;
  m_puhHeight          = NULL;
  m_phQP               = NULL;
  m_ChromaQpAdj        = NULL;
  m_pbMergeFlag        = NULL;
  m_puhMergeIndex      = NULL;
  for(UInt i=0; i<MAX_NUM_CHANNEL_TYPE; i++)
  {
    m_puhIntraDir[i]     = NULL;
  }
  m_puhInterDir        = NULL;
  m_puhTrIdx           = NULL;

  for (UInt comp=0; comp<MAX_NUM_COMPONENT; comp++)
  {
    m_puhCbf[comp]                        = NULL;
    m_crossComponentPredictionAlpha[comp] = NULL;
    m_puhTransformSkip[comp]              = NULL;
    m_pcTrCoeff[comp]                     = NULL;
#if ADAPTIVE_QP_SELECTION
    m_pcArlCoeff[comp]                    = NULL;
#endif
    m_pcIPCMSample[comp]                  = NULL;
    m_explicitRdpcmMode[comp]             = NULL;
  }
#if ADAPTIVE_QP_SELECTION
  m_ArlCoeffIsAliasedAllocation = false;
#endif
  m_pbIPCMFlag         = NULL;

  m_pCtuAboveLeft      = NULL;
  m_pCtuAboveRight     = NULL;
  m_pCtuAbove          = NULL;
  m_pCtuLeft           = NULL;

  for(UInt i=0; i<NUM_REF_PIC_LIST_01; i++)
  {
    m_apcCUColocated[i]  = NULL;
    m_apiMVPIdx[i]       = NULL;
    m_apiMVPNum[i]       = NULL;
  }

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

#if NH_3D_NBDV
  m_pDvInfo              = NULL;
#endif
#if NH_3D_VSP
  m_piVSPFlag            = NULL;
#endif
#if NH_3D_SPIVMP
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

}

TComDataCU::~TComDataCU()
{
}

Void TComDataCU::create( ChromaFormat chromaFormatIDC, UInt uiNumPartition, UInt uiWidth, UInt uiHeight, Bool bDecSubCu, Int unitSize
#if ADAPTIVE_QP_SELECTION
                        , TCoeff *pParentARLBuffer
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

    m_ChromaQpAdj        = new UChar[ uiNumPartition ];
    m_skipFlag           = new Bool[ uiNumPartition ];
#if H_3D
    m_bDISFlag           = new Bool[ uiNumPartition ];
    m_uiDISType          = (UInt*)xMalloc(UInt, uiNumPartition);
#endif
    m_pePartSize         = new Char[ uiNumPartition ];
    memset( m_pePartSize, NUMBER_OF_PART_SIZES,uiNumPartition * sizeof( *m_pePartSize ) );
    m_pePredMode         = new Char[ uiNumPartition ];
    m_CUTransquantBypass = new Bool[ uiNumPartition ];

    m_pbMergeFlag        = (Bool*  )xMalloc(Bool,   uiNumPartition);
    m_puhMergeIndex      = (UChar* )xMalloc(UChar,  uiNumPartition);
#if NH_3D_VSP
    m_piVSPFlag          = (Char*  )xMalloc(Char,   uiNumPartition);
#endif
#if NH_3D_SPIVMP
    m_pbSPIVMPFlag       = (Bool*  )xMalloc(Bool,   uiNumPartition);
#endif

    for (UInt ch=0; ch<MAX_NUM_CHANNEL_TYPE; ch++)
    {
      m_puhIntraDir[ch] = (UChar* )xMalloc(UChar,  uiNumPartition);
    }
    m_puhInterDir        = (UChar* )xMalloc(UChar,  uiNumPartition);

    m_puhTrIdx           = (UChar* )xMalloc(UChar,  uiNumPartition);

    for(UInt i=0; i<NUM_REF_PIC_LIST_01; i++)
    {
      const RefPicList rpl=RefPicList(i);
      m_apiMVPIdx[rpl]       = new Char[ uiNumPartition ];
      m_apiMVPNum[rpl]       = new Char[ uiNumPartition ];
      memset( m_apiMVPIdx[rpl], -1,uiNumPartition * sizeof( Char ) );
    }

#if NH_3D_NBDV 
    m_pDvInfo            = (DisInfo* )xMalloc(DisInfo,  uiNumPartition);
#endif


    for (UInt comp=0; comp<MAX_NUM_COMPONENT; comp++)
    {
      const ComponentID compID = ComponentID(comp);
      const UInt chromaShift = getComponentScaleX(compID, chromaFormatIDC) + getComponentScaleY(compID, chromaFormatIDC);
      const UInt totalSize   = (uiWidth * uiHeight) >> chromaShift;

      m_crossComponentPredictionAlpha[compID] = (Char*  )xMalloc(Char,   uiNumPartition);
      m_puhTransformSkip[compID]              = (UChar* )xMalloc(UChar,  uiNumPartition);
      m_explicitRdpcmMode[compID]             = (UChar* )xMalloc(UChar,  uiNumPartition);
      m_puhCbf[compID]                        = (UChar* )xMalloc(UChar,  uiNumPartition);
      m_pcTrCoeff[compID]                     = (TCoeff*)xMalloc(TCoeff, totalSize);
      memset( m_pcTrCoeff[compID], 0, (totalSize * sizeof( TCoeff )) );

#if ADAPTIVE_QP_SELECTION
      if( pParentARLBuffer != 0 )
      {
        m_pcArlCoeff[compID] = pParentARLBuffer;
        m_ArlCoeffIsAliasedAllocation = true;
        pParentARLBuffer += totalSize;
      }
      else
      {
        m_pcArlCoeff[compID] = (TCoeff*)xMalloc(TCoeff, totalSize);
        m_ArlCoeffIsAliasedAllocation = false;
      }
#endif
      m_pcIPCMSample[compID] = (Pel*   )xMalloc(Pel , totalSize);
    }

    m_pbIPCMFlag         = (Bool*  )xMalloc(Bool, uiNumPartition);

    for(UInt i=0; i<NUM_REF_PIC_LIST_01; i++)
    {
      m_acCUMvField[i].create( uiNumPartition );
    }

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
    for(UInt i=0; i<NUM_REF_PIC_LIST_01; i++)
    {
      m_acCUMvField[i].setNumPartition(uiNumPartition );
    }
  }

  // create motion vector fields

  m_pCtuAboveLeft      = NULL;
  m_pCtuAboveRight     = NULL;
  m_pCtuAbove          = NULL;
  m_pCtuLeft           = NULL;

  for(UInt i=0; i<NUM_REF_PIC_LIST_01; i++)
  {
    m_apcCUColocated[i]  = NULL;
  }
}

Void TComDataCU::destroy()
{
  // encoder-side buffer free
  if ( !m_bDecSubCu )
  {
    if ( m_phQP )
    {
      xFree(m_phQP);
      m_phQP = NULL;
    }
    if ( m_puhDepth )
    {
      xFree(m_puhDepth);
      m_puhDepth = NULL;
    }
    if ( m_puhWidth )
    {
      xFree(m_puhWidth);
      m_puhWidth = NULL;
    }
    if ( m_puhHeight )
    {
      xFree(m_puhHeight);
      m_puhHeight = NULL;
    }

    if ( m_skipFlag )
    {
      delete[] m_skipFlag;
      m_skipFlag = NULL;
    }

#if H_3D
    if ( m_bDISFlag           ) { delete[] m_bDISFlag;   m_bDISFlag     = NULL; }
    if ( m_uiDISType         ) { xFree(m_uiDISType);  m_uiDISType    = NULL; }
#endif

    if ( m_pePartSize )
    {
      delete[] m_pePartSize;
      m_pePartSize = NULL;
    }
    if ( m_pePredMode )
    {
      delete[] m_pePredMode;
      m_pePredMode = NULL;
    }
    if ( m_ChromaQpAdj )
    {
      delete[] m_ChromaQpAdj;
      m_ChromaQpAdj = NULL;
    }
    if ( m_CUTransquantBypass )
    {
      delete[] m_CUTransquantBypass;
      m_CUTransquantBypass = NULL;
    }
    if ( m_puhInterDir )
    {
      xFree(m_puhInterDir);
      m_puhInterDir = NULL;
    }
    if ( m_pbMergeFlag )
    {
      xFree(m_pbMergeFlag);
      m_pbMergeFlag = NULL;
    }
    if ( m_puhMergeIndex )
    {
      xFree(m_puhMergeIndex);
      m_puhMergeIndex  = NULL;
    }

#if NH_3D_VSP
    if ( m_piVSPFlag )
    {
      xFree(m_piVSPFlag);
      m_piVSPFlag = NULL;
    }
#endif
#if NH_3D_SPIVMP
    if ( m_pbSPIVMPFlag       ) { xFree(m_pbSPIVMPFlag);           m_pbSPIVMPFlag         = NULL; }
#endif


    for (UInt ch=0; ch<MAX_NUM_CHANNEL_TYPE; ch++)
    {
      xFree(m_puhIntraDir[ch]);
      m_puhIntraDir[ch] = NULL;
    }

    if ( m_puhTrIdx )
    {
      xFree(m_puhTrIdx);
      m_puhTrIdx = NULL;
    }

    for (UInt comp=0; comp<MAX_NUM_COMPONENT; comp++)
    {
      if ( m_crossComponentPredictionAlpha[comp] )
      {
        xFree(m_crossComponentPredictionAlpha[comp]);
        m_crossComponentPredictionAlpha[comp] = NULL;
      }
      if ( m_puhTransformSkip[comp] )
      {
        xFree(m_puhTransformSkip[comp]);
        m_puhTransformSkip[comp] = NULL;
      }
      if ( m_puhCbf[comp] )
      {
        xFree(m_puhCbf[comp]);
        m_puhCbf[comp] = NULL;
      }
      if ( m_pcTrCoeff[comp] )
      {
        xFree(m_pcTrCoeff[comp]);
        m_pcTrCoeff[comp] = NULL;
      }
      if ( m_explicitRdpcmMode[comp] )
      {
        xFree(m_explicitRdpcmMode[comp]);
        m_explicitRdpcmMode[comp] = NULL;
      }

#if ADAPTIVE_QP_SELECTION
      if (!m_ArlCoeffIsAliasedAllocation)
      {
        if ( m_pcArlCoeff[comp] )
        {
          xFree(m_pcArlCoeff[comp]);
          m_pcArlCoeff[comp] = NULL;
        }
      }
#endif

      if ( m_pcIPCMSample[comp] )
      {
        xFree(m_pcIPCMSample[comp]);
        m_pcIPCMSample[comp] = NULL;
      }
    }
    if ( m_pbIPCMFlag )
    {
      xFree(m_pbIPCMFlag );
      m_pbIPCMFlag = NULL;
    }

    for(UInt i=0; i<NUM_REF_PIC_LIST_01; i++)
    {
      const RefPicList rpl=RefPicList(i);
      if ( m_apiMVPIdx[rpl] )
      {
        delete[] m_apiMVPIdx[rpl];
        m_apiMVPIdx[rpl] = NULL;
      }
      if ( m_apiMVPNum[rpl] )
      {
        delete[] m_apiMVPNum[rpl];
        m_apiMVPNum[rpl] = NULL;
      }
    }

    for(UInt i=0; i<NUM_REF_PIC_LIST_01; i++)
    {
      const RefPicList rpl=RefPicList(i);
      m_acCUMvField[rpl].destroy();
    }
#if NH_3D_NBDV 
    if ( m_pDvInfo            ) { xFree(m_pDvInfo);             m_pDvInfo           = NULL; }
#endif


#if H_3D_ARP
    if ( m_puhARPW            ) { delete[] m_puhARPW;           m_puhARPW           = NULL; }
#endif
#if H_3D_IC
    if ( m_pbICFlag           ) { xFree(m_pbICFlag);            m_pbICFlag          = NULL; }
#endif

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

  m_pcPic              = NULL;
  m_pcSlice            = NULL;

  m_pCtuAboveLeft      = NULL;
  m_pCtuAboveRight     = NULL;
  m_pCtuAbove          = NULL;
  m_pCtuLeft           = NULL;


  for(UInt i=0; i<NUM_REF_PIC_LIST_01; i++)
  {
    m_apcCUColocated[i]  = NULL;
  }

}

Bool TComDataCU::CUIsFromSameTile            ( const TComDataCU *pCU /* Can be NULL */) const
{
  return pCU!=NULL &&
         pCU->getSlice() != NULL &&
         m_pcPic->getPicSym()->getTileIdxMap( pCU->getCtuRsAddr() ) == m_pcPic->getPicSym()->getTileIdxMap(getCtuRsAddr());
}

Bool TComDataCU::CUIsFromSameSliceAndTile    ( const TComDataCU *pCU /* Can be NULL */) const
{
  return pCU!=NULL &&
         pCU->getSlice() != NULL &&
         pCU->getSlice()->getSliceCurStartCtuTsAddr() == getSlice()->getSliceCurStartCtuTsAddr() &&
         m_pcPic->getPicSym()->getTileIdxMap( pCU->getCtuRsAddr() ) == m_pcPic->getPicSym()->getTileIdxMap(getCtuRsAddr())
         ;
}

Bool TComDataCU::CUIsFromSameSliceTileAndWavefrontRow( const TComDataCU *pCU /* Can be NULL */) const
{
  return CUIsFromSameSliceAndTile(pCU)
         && (!getSlice()->getPPS()->getEntropyCodingSyncEnabledFlag() || getPic()->getCtu(getCtuRsAddr())->getCUPelY() == getPic()->getCtu(pCU->getCtuRsAddr())->getCUPelY());
}

Bool TComDataCU::isLastSubCUOfCtu(const UInt absPartIdx)
{
  const TComSPS &sps=*(getSlice()->getSPS());

  const UInt picWidth = sps.getPicWidthInLumaSamples();
  const UInt picHeight = sps.getPicHeightInLumaSamples();
  const UInt granularityWidth = sps.getMaxCUWidth();

  const UInt cuPosX = getCUPelX() + g_auiRasterToPelX[ g_auiZscanToRaster[absPartIdx] ];
  const UInt cuPosY = getCUPelY() + g_auiRasterToPelY[ g_auiZscanToRaster[absPartIdx] ];

  return (((cuPosX+getWidth( absPartIdx))%granularityWidth==0||(cuPosX+getWidth( absPartIdx)==picWidth ))
       && ((cuPosY+getHeight(absPartIdx))%granularityWidth==0||(cuPosY+getHeight(absPartIdx)==picHeight)));
}

// ====================================================================================================================
// Public member functions
// ====================================================================================================================

// --------------------------------------------------------------------------------------------------------------------
// Initialization
// --------------------------------------------------------------------------------------------------------------------

/**
 Initialize top-level CU: create internal buffers and set initial values before encoding the CTU.
 
 \param  pcPic       picture (TComPic) class pointer
 \param  ctuRsAddr   CTU address in raster scan order
 */
Void TComDataCU::initCtu( TComPic* pcPic, UInt ctuRsAddr )
{

  const UInt maxCUWidth = pcPic->getPicSym()->getSPS().getMaxCUWidth();
  const UInt maxCUHeight= pcPic->getPicSym()->getSPS().getMaxCUHeight();
  m_pcPic              = pcPic;
  m_pcSlice            = pcPic->getSlice(pcPic->getCurrSliceIdx());
  m_ctuRsAddr          = ctuRsAddr;
  m_uiCUPelX           = ( ctuRsAddr % pcPic->getFrameWidthInCtus() ) * maxCUWidth;
  m_uiCUPelY           = ( ctuRsAddr / pcPic->getFrameWidthInCtus() ) * maxCUHeight;
  m_absZIdxInCtu       = 0;
  m_dTotalCost         = MAX_DOUBLE;
  m_uiTotalDistortion  = 0;
  m_uiTotalBits        = 0;
  m_uiTotalBins        = 0;
  m_uiNumPartition     = pcPic->getNumPartitionsInCtu();

  memset( m_skipFlag          , false,                      m_uiNumPartition * sizeof( *m_skipFlag ) );

#if H_3D
    m_bDISFlag[ui]   = pcFrom->getDISFlag(ui);
    m_uiDISType[ui]  = pcFrom->getDISType(ui);
#endif

  memset( m_pePartSize        , NUMBER_OF_PART_SIZES,       m_uiNumPartition * sizeof( *m_pePartSize ) );
  memset( m_pePredMode        , NUMBER_OF_PREDICTION_MODES, m_uiNumPartition * sizeof( *m_pePredMode ) );
  memset( m_CUTransquantBypass, false,                      m_uiNumPartition * sizeof( *m_CUTransquantBypass) );
  memset( m_puhDepth          , 0,                          m_uiNumPartition * sizeof( *m_puhDepth ) );
  memset( m_puhTrIdx          , 0,                          m_uiNumPartition * sizeof( *m_puhTrIdx ) );
  memset( m_puhWidth          , maxCUWidth,                 m_uiNumPartition * sizeof( *m_puhWidth ) );
  memset( m_puhHeight         , maxCUHeight,                m_uiNumPartition * sizeof( *m_puhHeight ) );

#if H_3D_ARP
    m_puhARPW   [ui] = pcFrom->getARPW( ui );
#endif
#if H_3D_IC
    m_pbICFlag[ui]   =  pcFrom->m_pbICFlag[ui];
#endif

  for(UInt i=0; i<NUM_REF_PIC_LIST_01; i++)
  {
    const RefPicList rpl=RefPicList(i);
    memset( m_apiMVPIdx[rpl]  , -1,                         m_uiNumPartition * sizeof( *m_apiMVPIdx[rpl] ) );
    memset( m_apiMVPNum[rpl]  , -1,                         m_uiNumPartition * sizeof( *m_apiMVPNum[rpl] ) );
  }
  memset( m_phQP              , getSlice()->getSliceQp(),   m_uiNumPartition * sizeof( *m_phQP ) );
  memset( m_ChromaQpAdj       , 0,                          m_uiNumPartition * sizeof( *m_ChromaQpAdj ) );
  for(UInt comp=0; comp<MAX_NUM_COMPONENT; comp++)
  {
    memset( m_crossComponentPredictionAlpha[comp] , 0,                     m_uiNumPartition * sizeof( *m_crossComponentPredictionAlpha[comp] ) );
    memset( m_puhTransformSkip[comp]              , 0,                     m_uiNumPartition * sizeof( *m_puhTransformSkip[comp]) );
    memset( m_puhCbf[comp]                        , 0,                     m_uiNumPartition * sizeof( *m_puhCbf[comp] ) );
    memset( m_explicitRdpcmMode[comp]             , NUMBER_OF_RDPCM_MODES, m_uiNumPartition * sizeof( *m_explicitRdpcmMode[comp] ) );
  }
  memset( m_pbMergeFlag       , false,                    m_uiNumPartition * sizeof( *m_pbMergeFlag ) );
  memset( m_puhMergeIndex     , 0,                        m_uiNumPartition * sizeof( *m_puhMergeIndex ) );

#if NH_3D_VSP
  memset( m_piVSPFlag         , 0,                        m_uiNumPartition * sizeof( *m_piVSPFlag ) );
#endif
#if NH_3D_SPIVMP
  memset( m_pbSPIVMPFlag      , 0,                     m_uiNumPartition * sizeof( *m_pbSPIVMPFlag ) );   
#endif
#if H_3D_DIM_SDC
    m_pbSDCFlag[ui] = pcFrom->m_pbSDCFlag[ui];
#endif
#if H_3D_DBBP
    m_pbDBBPFlag[ui] = pcFrom->m_pbDBBPFlag[ui];
#endif
#if H_3D
    memset( m_bDISFlag          + firstElement, false,                    numElements * sizeof( *m_bDISFlag ) );
    memset( m_uiDISType         + firstElement,     0,                    numElements * sizeof( *m_uiDISType) );
#endif

  for (UInt ch=0; ch<MAX_NUM_CHANNEL_TYPE; ch++)
  {
    memset( m_puhIntraDir[ch] , ((ch==0) ? DC_IDX : 0),   m_uiNumPartition * sizeof( *(m_puhIntraDir[ch]) ) );
  }

#if H_3D_ARP
    memset( m_puhARPW           + firstElement, 0,                        numElements * sizeof( UChar )         );
#endif
#if H_3D_IC
    memset( m_pbICFlag          + firstElement, false,                    numElements * sizeof( *m_pbICFlag )   );
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

  memset( m_puhInterDir       , 0,                        m_uiNumPartition * sizeof( *m_puhInterDir ) );
  memset( m_pbIPCMFlag        , false,                    m_uiNumPartition * sizeof( *m_pbIPCMFlag ) );

  const UInt numCoeffY    = maxCUWidth*maxCUHeight;
  for (UInt comp=0; comp<MAX_NUM_COMPONENT; comp++)
  {
    const UInt componentShift = m_pcPic->getComponentScaleX(ComponentID(comp)) + m_pcPic->getComponentScaleY(ComponentID(comp));
    memset( m_pcTrCoeff[comp], 0, sizeof(TCoeff)* numCoeffY>>componentShift );
#if ADAPTIVE_QP_SELECTION
    memset( m_pcArlCoeff[comp], 0, sizeof(TCoeff)* numCoeffY>>componentShift );
#endif
  }

  for(UInt i=0; i<NUM_REF_PIC_LIST_01; i++)
  {
    m_acCUMvField[i].clearMvField();
  }

  // Setting neighbor CU
  m_pCtuLeft        = NULL;
  m_pCtuAbove       = NULL;
  m_pCtuAboveLeft   = NULL;
  m_pCtuAboveRight  = NULL;


  for(UInt i=0; i<NUM_REF_PIC_LIST_01; i++)
  {
    m_apcCUColocated[i]  = NULL;
  }

  UInt frameWidthInCtus = pcPic->getFrameWidthInCtus();
  if ( m_ctuRsAddr % frameWidthInCtus )
  {
    m_pCtuLeft = pcPic->getCtu( m_ctuRsAddr - 1 );
  }

  if ( m_ctuRsAddr / frameWidthInCtus )
  {
    m_pCtuAbove = pcPic->getCtu( m_ctuRsAddr - frameWidthInCtus );
  }

  if ( m_pCtuLeft && m_pCtuAbove )
  {
    m_pCtuAboveLeft = pcPic->getCtu( m_ctuRsAddr - frameWidthInCtus - 1 );
  }

  if ( m_pCtuAbove && ( (m_ctuRsAddr%frameWidthInCtus) < (frameWidthInCtus-1) )  )
  {
    m_pCtuAboveRight = pcPic->getCtu( m_ctuRsAddr - frameWidthInCtus + 1 );
  }

  for(UInt i=0; i<NUM_REF_PIC_LIST_01; i++)
  {
    const RefPicList rpl=RefPicList(i);
    if ( getSlice()->getNumRefIdx( rpl ) > 0 )
    {
      m_apcCUColocated[rpl] = getSlice()->getRefPic( rpl, 0)->getCtu( m_ctuRsAddr );
    }
  }
}


/** Initialize prediction data with enabling sub-CTU-level delta QP.
*   - set CU width and CU height according to depth
*   - set qp value according to input qp
*   - set last-coded qp value according to input last-coded qp
*
* \param  uiDepth            depth of the current CU
* \param  qp                 qp for the current CU
* \param  bTransquantBypass  true for transquant bypass
*/
Void TComDataCU::initEstData( const UInt uiDepth, const Int qp, const Bool bTransquantBypass )
{
  m_dTotalCost         = MAX_DOUBLE;
  m_uiTotalDistortion  = 0;
  m_uiTotalBits        = 0;
  m_uiTotalBins        = 0;

  const UChar uhWidth  = getSlice()->getSPS()->getMaxCUWidth()  >> uiDepth;
  const UChar uhHeight = getSlice()->getSPS()->getMaxCUHeight() >> uiDepth;

  for (UInt ui = 0; ui < m_uiNumPartition; ui++)
  {
    for(UInt i=0; i<NUM_REF_PIC_LIST_01; i++)
    {
      const RefPicList rpl=RefPicList(i);
      m_apiMVPIdx[rpl][ui]  = -1;
      m_apiMVPNum[rpl][ui]  = -1;
    }
    m_puhDepth  [ui]    = uiDepth;
    m_puhWidth  [ui]    = uhWidth;
    m_puhHeight [ui]    = uhHeight;
    m_puhTrIdx  [ui]    = 0;
    for(UInt comp=0; comp<MAX_NUM_COMPONENT; comp++)
    {
      m_crossComponentPredictionAlpha[comp][ui] = 0;
      m_puhTransformSkip             [comp][ui] = 0;
      m_explicitRdpcmMode            [comp][ui] = NUMBER_OF_RDPCM_MODES;
    }
    m_skipFlag[ui]      = false;
#if H_3D
      m_bDISFlag[ui]   = false;
      m_uiDISType[ui]  = 0;
#endif
    m_pePartSize[ui]    = NUMBER_OF_PART_SIZES;
    m_pePredMode[ui]    = NUMBER_OF_PREDICTION_MODES;
    m_CUTransquantBypass[ui] = bTransquantBypass;
    m_pbIPCMFlag[ui]    = 0;
    m_phQP[ui]          = qp;
    m_ChromaQpAdj[ui]   = 0;
    m_pbMergeFlag[ui]   = 0;
    m_puhMergeIndex[ui] = 0;
#if NH_3D_VSP
    m_piVSPFlag[ui]     = 0;
#endif
#if NH_3D_SPIVMP
    m_pbSPIVMPFlag[ui] = 0;
#endif

    for (UInt ch=0; ch<MAX_NUM_CHANNEL_TYPE; ch++)
    {
      m_puhIntraDir[ch][ui] = ((ch==0) ? DC_IDX : 0);
    }

    m_puhInterDir[ui] = 0;
    for (UInt comp=0; comp<MAX_NUM_COMPONENT; comp++)
    {
      m_puhCbf[comp][ui] = 0;
    }
#if H_3D_ARP
      m_puhARPW[ui] = 0;
#endif
#if H_3D_IC
      m_pbICFlag[ui]  = false;
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

  for(UInt i=0; i<NUM_REF_PIC_LIST_01; i++)
  {
    m_acCUMvField[i].clearMvField();
  }

  const UInt numCoeffY = uhWidth*uhHeight;

  for (UInt comp=0; comp<MAX_NUM_COMPONENT; comp++)
  {
    const ComponentID component = ComponentID(comp);
    const UInt numCoeff = numCoeffY >> (getPic()->getComponentScaleX(component) + getPic()->getComponentScaleY(component));
    memset( m_pcTrCoeff[comp],    0, numCoeff * sizeof( TCoeff ) );
#if ADAPTIVE_QP_SELECTION
    memset( m_pcArlCoeff[comp],   0, numCoeff * sizeof( TCoeff ) );
#endif
    memset( m_pcIPCMSample[comp], 0, numCoeff * sizeof( Pel) );
  }
}


// initialize Sub partition
Void TComDataCU::initSubCU( TComDataCU* pcCU, UInt uiPartUnitIdx, UInt uiDepth, Int qp )
{
  assert( uiPartUnitIdx<4 );

  UInt uiPartOffset = ( pcCU->getTotalNumPart()>>2 )*uiPartUnitIdx;

  m_pcPic              = pcCU->getPic();
  m_pcSlice            = pcCU->getSlice();
  m_ctuRsAddr          = pcCU->getCtuRsAddr();
  m_absZIdxInCtu       = pcCU->getZorderIdxInCtu() + uiPartOffset;

  const UChar uhWidth  = getSlice()->getSPS()->getMaxCUWidth()  >> uiDepth;
  const UChar uhHeight = getSlice()->getSPS()->getMaxCUHeight() >> uiDepth;

  m_uiCUPelX           = pcCU->getCUPelX() + ( uhWidth )*( uiPartUnitIdx &  1 );
  m_uiCUPelY           = pcCU->getCUPelY() + ( uhHeight)*( uiPartUnitIdx >> 1 );

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
#if NH_3D_VSP
  memset( m_piVSPFlag,          0, sizeof( Char  ) * m_uiNumPartition );
#endif
#if NH_3D_SPIVMP
  memset( m_pbSPIVMPFlag,       0, sizeof( Bool  ) * m_uiNumPartition );
#endif

  for (UInt ch=0; ch<MAX_NUM_CHANNEL_TYPE; ch++)
  {
    memset( m_puhIntraDir[ch],  ((ch==0) ? DC_IDX : 0), iSizeInUchar );
  }

  memset( m_puhInterDir,        0, iSizeInUchar );
  memset( m_puhTrIdx,           0, iSizeInUchar );

  for(UInt comp=0; comp<MAX_NUM_COMPONENT; comp++)
  {
    memset( m_crossComponentPredictionAlpha[comp], 0, iSizeInUchar );
    memset( m_puhTransformSkip[comp],              0, iSizeInUchar );
    memset( m_puhCbf[comp],                        0, iSizeInUchar );
    memset( m_explicitRdpcmMode[comp],             NUMBER_OF_RDPCM_MODES, iSizeInUchar );
  }
#if H_3D_ARP
  memset( m_puhARPW,            0, iSizeInUchar  );
#endif

  memset( m_puhDepth,     uiDepth, iSizeInUchar );
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
#if H_3D
    m_bDISFlag[ui]   = false;
    m_uiDISType[ui]  = 0;
#endif

    m_pePartSize[ui] = NUMBER_OF_PART_SIZES;
    m_pePredMode[ui] = NUMBER_OF_PREDICTION_MODES;
    m_CUTransquantBypass[ui] = false;
    m_ChromaQpAdj[ui] = 0;

    for(UInt i=0; i<NUM_REF_PIC_LIST_01; i++)
    {
      const RefPicList rpl=RefPicList(i);
      m_apiMVPIdx[rpl][ui] = -1;
      m_apiMVPNum[rpl][ui] = -1;
    }
#if H_3D
      m_bDISFlag[ui]    = pcCU->getDISFlag(uiPartOffset+ui);
      m_uiDISType[ui]   = pcCU->getDISType(uiPartOffset+ui);
#endif
#if NH_3D_VSP
    m_piVSPFlag[ui] = pcCU->m_piVSPFlag[uiPartOffset+ui];
    m_pDvInfo[ ui ] = pcCU->m_pDvInfo[uiPartOffset+ui];
#endif
#if NH_3D_SPIVMP
    m_pbSPIVMPFlag[ui]=pcCU->m_pbSPIVMPFlag[uiPartOffset+ui];
#endif
#if H_3D_ARP
      m_puhARPW           [ui] = pcCU->getARPW( uiPartOffset+ui );
#endif
#if H_3D_IC
      m_pbICFlag          [ui] = pcCU->m_pbICFlag[uiPartOffset+ui];
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

  const UInt numCoeffY    = uhWidth*uhHeight;
  for (UInt ch=0; ch<MAX_NUM_COMPONENT; ch++)
  {
    const UInt componentShift = m_pcPic->getComponentScaleX(ComponentID(ch)) + m_pcPic->getComponentScaleY(ComponentID(ch));
    memset( m_pcTrCoeff[ch],  0, sizeof(TCoeff)*(numCoeffY>>componentShift) );
#if ADAPTIVE_QP_SELECTION
    memset( m_pcArlCoeff[ch], 0, sizeof(TCoeff)*(numCoeffY>>componentShift) );
#endif
    memset( m_pcIPCMSample[ch], 0, sizeof(Pel)* (numCoeffY>>componentShift) );
  }

  for(UInt i=0; i<NUM_REF_PIC_LIST_01; i++)
  {
    m_acCUMvField[i].clearMvField();
  }

  m_pCtuLeft        = pcCU->getCtuLeft();
  m_pCtuAbove       = pcCU->getCtuAbove();
  m_pCtuAboveLeft   = pcCU->getCtuAboveLeft();
  m_pCtuAboveRight  = pcCU->getCtuAboveRight();

  for(UInt i=0; i<NUM_REF_PIC_LIST_01; i++)
  {
    m_apcCUColocated[i] = pcCU->getCUColocated(RefPicList(i));
  }
}

Void TComDataCU::setOutsideCUPart( UInt uiAbsPartIdx, UInt uiDepth )
{
  const UInt     uiNumPartition = m_uiNumPartition >> (uiDepth << 1);
  const UInt     uiSizeInUchar  = sizeof( UChar  ) * uiNumPartition;
  const TComSPS &sps            = *(getSlice()->getSPS());
  const UChar    uhWidth        = sps.getMaxCUWidth()  >> uiDepth;
  const UChar    uhHeight       = sps.getMaxCUHeight() >> uiDepth;
  memset( m_puhDepth    + uiAbsPartIdx,     uiDepth,  uiSizeInUchar );
  memset( m_puhWidth    + uiAbsPartIdx,     uhWidth,  uiSizeInUchar );
  memset( m_puhHeight   + uiAbsPartIdx,     uhHeight, uiSizeInUchar );
}

// --------------------------------------------------------------------------------------------------------------------
// Copy
// --------------------------------------------------------------------------------------------------------------------

Void TComDataCU::copySubCU( TComDataCU* pcCU, UInt uiAbsPartIdx )
{
  UInt uiPart = uiAbsPartIdx;

  m_pcPic              = pcCU->getPic();
  m_pcSlice            = pcCU->getSlice();
  m_ctuRsAddr          = pcCU->getCtuRsAddr();
  m_absZIdxInCtu       = uiAbsPartIdx;

  m_uiCUPelX           = pcCU->getCUPelX() + g_auiRasterToPelX[ g_auiZscanToRaster[uiAbsPartIdx] ];
  m_uiCUPelY           = pcCU->getCUPelY() + g_auiRasterToPelY[ g_auiZscanToRaster[uiAbsPartIdx] ];

  m_skipFlag=pcCU->getSkipFlag()          + uiPart;
#if H_3D
  m_bDISFlag     = pcCU->getDISFlag()     + uiPart;
  m_uiDISType    = pcCU->getDISType()     + uiPart;
#endif

  m_phQP=pcCU->getQP()                    + uiPart;
  m_ChromaQpAdj = pcCU->getChromaQpAdj()  + uiPart;
  m_pePartSize = pcCU->getPartitionSize() + uiPart;
  m_pePredMode=pcCU->getPredictionMode()  + uiPart;
  m_CUTransquantBypass  = pcCU->getCUTransquantBypass()+uiPart;
#if NH_3D_NBDV
  m_pDvInfo             = pcCU->getDvInfo()           + uiPart;
#endif

  m_pbMergeFlag         = pcCU->getMergeFlag()        + uiPart;
  m_puhMergeIndex       = pcCU->getMergeIndex()       + uiPart;
#if NH_3D_VSP
  m_piVSPFlag           = pcCU->getVSPFlag()          + uiPart;
#endif
#if NH_3D_SPIVMP
  m_pbSPIVMPFlag        = pcCU->getSPIVMPFlag()          + uiPart;
#endif
#if H_3D_ARP
  m_puhARPW             = pcCU->getARPW()             + uiPart;
#endif
#if H_3D_IC
  m_pbICFlag            = pcCU->getICFlag()           + uiPart;
#endif

  for (UInt ch=0; ch<MAX_NUM_CHANNEL_TYPE; ch++)
  {
    m_puhIntraDir[ch]   = pcCU->getIntraDir(ChannelType(ch)) + uiPart;
  }

  m_puhInterDir         = pcCU->getInterDir()         + uiPart;
  m_puhTrIdx            = pcCU->getTransformIdx()     + uiPart;

  for(UInt comp=0; comp<MAX_NUM_COMPONENT; comp++)
  {
    m_crossComponentPredictionAlpha[comp] = pcCU->getCrossComponentPredictionAlpha(ComponentID(comp)) + uiPart;
    m_puhTransformSkip[comp]              = pcCU->getTransformSkip(ComponentID(comp))                 + uiPart;
    m_puhCbf[comp]                        = pcCU->getCbf(ComponentID(comp))                           + uiPart;
    m_explicitRdpcmMode[comp]             = pcCU->getExplicitRdpcmMode(ComponentID(comp))             + uiPart;
  }
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

  m_pbIPCMFlag         = pcCU->getIPCMFlag()        + uiPart;

  m_pCtuAboveLeft      = pcCU->getCtuAboveLeft();
  m_pCtuAboveRight     = pcCU->getCtuAboveRight();
  m_pCtuAbove          = pcCU->getCtuAbove();
  m_pCtuLeft           = pcCU->getCtuLeft();

  for(UInt i=0; i<NUM_REF_PIC_LIST_01; i++)
  {
    const RefPicList rpl=RefPicList(i);
    m_apcCUColocated[rpl] = pcCU->getCUColocated(rpl);
    m_apiMVPIdx[rpl]=pcCU->getMVPIdx(rpl)  + uiPart;
    m_apiMVPNum[rpl]=pcCU->getMVPNum(rpl)  + uiPart;
  }

  for(UInt i=0; i<NUM_REF_PIC_LIST_01; i++)
  {
    const RefPicList rpl=RefPicList(i);
    m_acCUMvField[rpl].linkToWithOffset( pcCU->getCUMvField(rpl), uiPart );
  }

  UInt uiMaxCuWidth=pcCU->getSlice()->getSPS()->getMaxCUWidth();
  UInt uiMaxCuHeight=pcCU->getSlice()->getSPS()->getMaxCUHeight();

  UInt uiCoffOffset = uiMaxCuWidth*uiMaxCuHeight*uiAbsPartIdx/pcCU->getPic()->getNumPartitionsInCtu();

  for (UInt ch=0; ch<MAX_NUM_COMPONENT; ch++)
  {
    const ComponentID component = ComponentID(ch);
    const UInt componentShift   = m_pcPic->getComponentScaleX(component) + m_pcPic->getComponentScaleY(component);
    const UInt offset           = uiCoffOffset >> componentShift;
    m_pcTrCoeff[ch] = pcCU->getCoeff(component) + offset;
#if ADAPTIVE_QP_SELECTION
    m_pcArlCoeff[ch] = pcCU->getArlCoeff(component) + offset;
#endif
    m_pcIPCMSample[ch] = pcCU->getPCMSample(component) + offset;
  }
}

#if NH_3D_NBDV
Void TComDataCU::copyDVInfoFrom (TComDataCU* pcCU, UInt uiAbsPartIdx)
{
  m_pDvInfo            = pcCU->getDvInfo()                + uiAbsPartIdx;
}
#endif

// Copy inter prediction info from the biggest CU
Void TComDataCU::copyInterPredInfoFrom    ( TComDataCU* pcCU, UInt uiAbsPartIdx, RefPicList eRefPicList 
#if NH_3D_NBDV
  , Bool bNBDV
#endif
)
{
  m_pcPic              = pcCU->getPic();
  m_pcSlice            = pcCU->getSlice();
  m_ctuRsAddr          = pcCU->getCtuRsAddr();
  m_absZIdxInCtu       = uiAbsPartIdx;

  Int iRastPartIdx     = g_auiZscanToRaster[uiAbsPartIdx];
  m_uiCUPelX           = pcCU->getCUPelX() + m_pcPic->getMinCUWidth ()*( iRastPartIdx % m_pcPic->getNumPartInCtuWidth() );
  m_uiCUPelY           = pcCU->getCUPelY() + m_pcPic->getMinCUHeight()*( iRastPartIdx / m_pcPic->getNumPartInCtuWidth() );

  m_pCtuAboveLeft      = pcCU->getCtuAboveLeft();
  m_pCtuAboveRight     = pcCU->getCtuAboveRight();
  m_pCtuAbove          = pcCU->getCtuAbove();
  m_pCtuLeft           = pcCU->getCtuLeft();

  for(UInt i=0; i<NUM_REF_PIC_LIST_01; i++)
  {
    m_apcCUColocated[i]  = pcCU->getCUColocated(RefPicList(i));
  }

  m_skipFlag           = pcCU->getSkipFlag ()             + uiAbsPartIdx;
#if H_3D
  m_bDISFlag           = pcCU->getDISFlag ()              + uiAbsPartIdx;
  m_uiDISType          = pcCU->getDISType()               + uiAbsPartIdx;
#endif

  m_pePartSize         = pcCU->getPartitionSize ()        + uiAbsPartIdx;
#if NH_3D_NBDV
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
  m_ChromaQpAdj        = pcCU->getChromaQpAdj()           + uiAbsPartIdx;
  m_CUTransquantBypass = pcCU->getCUTransquantBypass()    + uiAbsPartIdx;
  m_puhInterDir        = pcCU->getInterDir      ()        + uiAbsPartIdx;

  m_puhDepth           = pcCU->getDepth ()                + uiAbsPartIdx;
  m_puhWidth           = pcCU->getWidth ()                + uiAbsPartIdx;
  m_puhHeight          = pcCU->getHeight()                + uiAbsPartIdx;

  m_pbMergeFlag        = pcCU->getMergeFlag()             + uiAbsPartIdx;
  m_puhMergeIndex      = pcCU->getMergeIndex()            + uiAbsPartIdx;
#if NH_3D_VSP
  m_piVSPFlag          = pcCU->getVSPFlag()               + uiAbsPartIdx;
  m_pDvInfo            = pcCU->getDvInfo()                + uiAbsPartIdx;
#endif
#if NH_3D_SPIVMP
  m_pbSPIVMPFlag       = pcCU->getSPIVMPFlag()            + uiAbsPartIdx;
#endif

  m_apiMVPIdx[eRefPicList] = pcCU->getMVPIdx(eRefPicList) + uiAbsPartIdx;
  m_apiMVPNum[eRefPicList] = pcCU->getMVPNum(eRefPicList) + uiAbsPartIdx;
#if H_3D_ARP
  m_puhARPW            = pcCU->getARPW()                  + uiAbsPartIdx;
#endif    
#if H_3D_DBBP
  m_pbDBBPFlag       = pcCU->getDBBPFlag()              + uiAbsPartIdx;
#endif

  m_acCUMvField[ eRefPicList ].linkToWithOffset( pcCU->getCUMvField(eRefPicList), uiAbsPartIdx );
#if NH_3D_NBDV
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
  const UInt numValidComp=pcCU->getPic()->getNumberValidComponents();
  const UInt numValidChan=pcCU->getPic()->getChromaFormat()==CHROMA_400 ? 1:2;

  UInt uiNumPartition = pcCU->getTotalNumPart();
  Int iSizeInUchar  = sizeof( UChar ) * uiNumPartition;
  Int iSizeInBool   = sizeof( Bool  ) * uiNumPartition;

  Int sizeInChar  = sizeof( Char ) * uiNumPartition;
  memcpy( m_skipFlag   + uiOffset, pcCU->getSkipFlag(),       sizeof( *m_skipFlag )   * uiNumPartition );
#if H_3D
  memcpy( m_bDISFlag   + uiOffset, pcCU->getDISFlag(),       sizeof( *m_bDISFlag )   * uiNumPartition );
  memcpy( m_uiDISType  + uiOffset, pcCU->getDISType(),       sizeof( *m_uiDISType )  * uiNumPartition);
#endif
  memcpy( m_phQP       + uiOffset, pcCU->getQP(),             sizeInChar                        );
  memcpy( m_pePartSize + uiOffset, pcCU->getPartitionSize(),  sizeof( *m_pePartSize ) * uiNumPartition );
  memcpy( m_pePredMode + uiOffset, pcCU->getPredictionMode(), sizeof( *m_pePredMode ) * uiNumPartition );
  memcpy( m_ChromaQpAdj + uiOffset, pcCU->getChromaQpAdj(),   sizeof( *m_ChromaQpAdj ) * uiNumPartition );
  memcpy( m_CUTransquantBypass + uiOffset, pcCU->getCUTransquantBypass(), sizeof( *m_CUTransquantBypass ) * uiNumPartition );
  memcpy( m_pbMergeFlag         + uiOffset, pcCU->getMergeFlag(),         iSizeInBool  );
  memcpy( m_puhMergeIndex       + uiOffset, pcCU->getMergeIndex(),        iSizeInUchar );
#if NH_3D_VSP
  memcpy( m_piVSPFlag           + uiOffset, pcCU->getVSPFlag(),           sizeof( Char ) * uiNumPartition );
  memcpy( m_pDvInfo             + uiOffset, pcCU->getDvInfo(),            sizeof( *m_pDvInfo ) * uiNumPartition );
#endif
#if NH_3D_SPIVMP
  memcpy( m_pbSPIVMPFlag        + uiOffset, pcCU->getSPIVMPFlag(),        sizeof( Bool ) * uiNumPartition );
#endif

  for (UInt ch=0; ch<numValidChan; ch++)
  {
    memcpy( m_puhIntraDir[ch]   + uiOffset, pcCU->getIntraDir(ChannelType(ch)), iSizeInUchar );
  }

  memcpy( m_puhInterDir         + uiOffset, pcCU->getInterDir(),          iSizeInUchar );
  memcpy( m_puhTrIdx            + uiOffset, pcCU->getTransformIdx(),      iSizeInUchar );

  for(UInt comp=0; comp<numValidComp; comp++)
  {
    memcpy( m_crossComponentPredictionAlpha[comp] + uiOffset, pcCU->getCrossComponentPredictionAlpha(ComponentID(comp)), iSizeInUchar );
    memcpy( m_puhTransformSkip[comp]              + uiOffset, pcCU->getTransformSkip(ComponentID(comp))                , iSizeInUchar );
    memcpy( m_puhCbf[comp]                        + uiOffset, pcCU->getCbf(ComponentID(comp))                          , iSizeInUchar );
    memcpy( m_explicitRdpcmMode[comp]             + uiOffset, pcCU->getExplicitRdpcmMode(ComponentID(comp))            , iSizeInUchar );
  }
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

  memcpy( m_pbIPCMFlag + uiOffset, pcCU->getIPCMFlag(), iSizeInBool );

  m_pCtuAboveLeft      = pcCU->getCtuAboveLeft();
  m_pCtuAboveRight     = pcCU->getCtuAboveRight();
  m_pCtuAbove          = pcCU->getCtuAbove();
  m_pCtuLeft           = pcCU->getCtuLeft();

  for(UInt i=0; i<NUM_REF_PIC_LIST_01; i++)
  {
    const RefPicList rpl=RefPicList(i);
    memcpy( m_apiMVPIdx[rpl] + uiOffset, pcCU->getMVPIdx(rpl), iSizeInUchar );
    memcpy( m_apiMVPNum[rpl] + uiOffset, pcCU->getMVPNum(rpl), iSizeInUchar );
    m_apcCUColocated[rpl] = pcCU->getCUColocated(rpl);
  }

  for(UInt i=0; i<NUM_REF_PIC_LIST_01; i++)
  {
    const RefPicList rpl=RefPicList(i);
    m_acCUMvField[rpl].copyFrom( pcCU->getCUMvField( rpl ), pcCU->getTotalNumPart(), uiOffset );
  }

  const UInt numCoeffY = (pcCU->getSlice()->getSPS()->getMaxCUWidth()*pcCU->getSlice()->getSPS()->getMaxCUHeight()) >> (uiDepth<<1);
  const UInt offsetY   = uiPartUnitIdx*numCoeffY;
  for (UInt ch=0; ch<numValidComp; ch++)
  {
    const ComponentID component = ComponentID(ch);
    const UInt componentShift   = m_pcPic->getComponentScaleX(component) + m_pcPic->getComponentScaleY(component);
    const UInt offset           = offsetY>>componentShift;
    memcpy( m_pcTrCoeff [ch] + offset, pcCU->getCoeff(component),    sizeof(TCoeff)*(numCoeffY>>componentShift) );
#if ADAPTIVE_QP_SELECTION
    memcpy( m_pcArlCoeff[ch] + offset, pcCU->getArlCoeff(component), sizeof(TCoeff)*(numCoeffY>>componentShift) );
#endif
    memcpy( m_pcIPCMSample[ch] + offset, pcCU->getPCMSample(component), sizeof(Pel)*(numCoeffY>>componentShift) );
  }

#if H_3D_ARP
  memcpy( m_puhARPW             + uiOffset, pcCU->getARPW(),              iSizeInUchar );
#endif
#if H_3D_IC
  memcpy( m_pbICFlag            + uiOffset, pcCU->getICFlag(),            iSizeInBool );
#endif

  m_uiTotalBins += pcCU->getTotalBins();
}

// Copy current predicted part to a CU in picture.
// It is used to predict for next part
Void TComDataCU::copyToPic( UChar uhDepth )
{
  TComDataCU* pCtu = m_pcPic->getCtu( m_ctuRsAddr );
  const UInt numValidComp=pCtu->getPic()->getNumberValidComponents();
  const UInt numValidChan=pCtu->getPic()->getChromaFormat()==CHROMA_400 ? 1:2;

  pCtu->getTotalCost()       = m_dTotalCost;
  pCtu->getTotalDistortion() = m_uiTotalDistortion;
  pCtu->getTotalBits()       = m_uiTotalBits;

  Int iSizeInUchar  = sizeof( UChar ) * m_uiNumPartition;
  Int iSizeInBool   = sizeof( Bool  ) * m_uiNumPartition;
  Int sizeInChar  = sizeof( Char ) * m_uiNumPartition;

  memcpy( pCtu->getSkipFlag() + m_absZIdxInCtu, m_skipFlag, sizeof( *m_skipFlag ) * m_uiNumPartition );
#if H_3D
  memcpy( rpcCU->getDISFlag()  + m_uiAbsIdxInLCU, m_bDISFlag,    sizeof( *m_bDISFlag )  * m_uiNumPartition );
  memcpy( rpcCU->getDISType()  + m_uiAbsIdxInLCU, m_uiDISType,   sizeof( *m_uiDISType ) * m_uiNumPartition );
#endif

  memcpy( pCtu->getQP() + m_absZIdxInCtu, m_phQP, sizeInChar  );
#if NH_3D_NBDV
  memcpy( pCtu->getDvInfo() + m_absZIdxInCtu, m_pDvInfo, sizeof(* m_pDvInfo) * m_uiNumPartition );
#endif

  memcpy( pCtu->getPartitionSize()  + m_absZIdxInCtu, m_pePartSize, sizeof( *m_pePartSize ) * m_uiNumPartition );
  memcpy( pCtu->getPredictionMode() + m_absZIdxInCtu, m_pePredMode, sizeof( *m_pePredMode ) * m_uiNumPartition );
  memcpy( pCtu->getChromaQpAdj() + m_absZIdxInCtu, m_ChromaQpAdj, sizeof( *m_ChromaQpAdj ) * m_uiNumPartition );
  memcpy( pCtu->getCUTransquantBypass()+ m_absZIdxInCtu, m_CUTransquantBypass, sizeof( *m_CUTransquantBypass ) * m_uiNumPartition );
  memcpy( pCtu->getMergeFlag()         + m_absZIdxInCtu, m_pbMergeFlag,         iSizeInBool  );
  memcpy( pCtu->getMergeIndex()        + m_absZIdxInCtu, m_puhMergeIndex,       iSizeInUchar );
#if NH_3D_VSP
  memcpy( pCtu->getVSPFlag()           + m_absZIdxInCtu, m_piVSPFlag,           sizeof( Char ) * m_uiNumPartition );
#endif
#if NH_3D_SPIVMP
  memcpy( pCtu->getSPIVMPFlag()        + m_absZIdxInCtu, m_pbSPIVMPFlag,        sizeof( Bool ) * m_uiNumPartition );
#endif

for (UInt ch=0; ch<numValidChan; ch++)
  {
    memcpy( pCtu->getIntraDir(ChannelType(ch)) + m_absZIdxInCtu, m_puhIntraDir[ch], iSizeInUchar);
  }

  memcpy( pCtu->getInterDir()          + m_absZIdxInCtu, m_puhInterDir,         iSizeInUchar );
  memcpy( pCtu->getTransformIdx()      + m_absZIdxInCtu, m_puhTrIdx,            iSizeInUchar );

  for(UInt comp=0; comp<numValidComp; comp++)
  {
    memcpy( pCtu->getCrossComponentPredictionAlpha(ComponentID(comp)) + m_absZIdxInCtu, m_crossComponentPredictionAlpha[comp], iSizeInUchar );
    memcpy( pCtu->getTransformSkip(ComponentID(comp))                 + m_absZIdxInCtu, m_puhTransformSkip[comp],              iSizeInUchar );
    memcpy( pCtu->getCbf(ComponentID(comp))                           + m_absZIdxInCtu, m_puhCbf[comp],                        iSizeInUchar );
    memcpy( pCtu->getExplicitRdpcmMode(ComponentID(comp))             + m_absZIdxInCtu, m_explicitRdpcmMode[comp],             iSizeInUchar );
  }

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

  memcpy( pCtu->getDepth()  + m_absZIdxInCtu, m_puhDepth,  iSizeInUchar );
  memcpy( pCtu->getWidth()  + m_absZIdxInCtu, m_puhWidth,  iSizeInUchar );
  memcpy( pCtu->getHeight() + m_absZIdxInCtu, m_puhHeight, iSizeInUchar );

  for(UInt i=0; i<NUM_REF_PIC_LIST_01; i++)
  {
    const RefPicList rpl=RefPicList(i);
    memcpy( pCtu->getMVPIdx(rpl) + m_absZIdxInCtu, m_apiMVPIdx[rpl], iSizeInUchar );
    memcpy( pCtu->getMVPNum(rpl) + m_absZIdxInCtu, m_apiMVPNum[rpl], iSizeInUchar );
  }

  for(UInt i=0; i<NUM_REF_PIC_LIST_01; i++)
  {
    const RefPicList rpl=RefPicList(i);
    m_acCUMvField[rpl].copyTo( pCtu->getCUMvField( rpl ), m_absZIdxInCtu );
  }

  memcpy( pCtu->getIPCMFlag() + m_absZIdxInCtu, m_pbIPCMFlag,         iSizeInBool  );

  const UInt numCoeffY    = (pCtu->getSlice()->getSPS()->getMaxCUWidth()*pCtu->getSlice()->getSPS()->getMaxCUHeight())>>(uhDepth<<1);
  const UInt offsetY      = m_absZIdxInCtu*m_pcPic->getMinCUWidth()*m_pcPic->getMinCUHeight();
  for (UInt comp=0; comp<numValidComp; comp++)
  {
    const ComponentID component = ComponentID(comp);
    const UInt componentShift   = m_pcPic->getComponentScaleX(component) + m_pcPic->getComponentScaleY(component);
    memcpy( pCtu->getCoeff(component)   + (offsetY>>componentShift), m_pcTrCoeff[component], sizeof(TCoeff)*(numCoeffY>>componentShift) );
#if ADAPTIVE_QP_SELECTION
    memcpy( pCtu->getArlCoeff(component) + (offsetY>>componentShift), m_pcArlCoeff[component], sizeof(TCoeff)*(numCoeffY>>componentShift) );
#endif
    memcpy( pCtu->getPCMSample(component) + (offsetY>>componentShift), m_pcIPCMSample[component], sizeof(Pel)*(numCoeffY>>componentShift) );
  }

#if H_3D_ARP
  memcpy( rpcCU->getARPW()             + m_uiAbsIdxInLCU, m_puhARPW,             iSizeInUchar );
#endif
#if H_3D_IC
  memcpy( rpcCU->getICFlag()           + m_uiAbsIdxInLCU, m_pbICFlag,            iSizeInBool );
#endif

  pCtu->getTotalBins() = m_uiTotalBins;
}

#if H_3D
  memcpy( rpcCU->getDISFlag()  + uiPartOffset, m_bDISFlag,    sizeof( *m_bDISFlag )   * uiQNumPart );
  memcpy( rpcCU->getDISType()  + uiPartOffset, m_uiDISType,   sizeof( *m_uiDISType )  * uiQNumPart );
#endif

#if H_3D_SPIVMP
  memcpy( rpcCU->getSPIVMPFlag()        + uiPartOffset, m_pbSPIVMPFlag,        sizeof(Bool) * uiQNumPart );
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
#if H_3D_ARP
  memcpy( rpcCU->getARPW()             + uiPartOffset, m_puhARPW,             iSizeInUchar );
#endif
#if H_3D_IC
  memcpy( rpcCU->getICFlag()           + uiPartOffset, m_pbICFlag,            iSizeInBool );
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
  UInt uiAbsZorderCUIdx   = g_auiZscanToRaster[m_absZIdxInCtu];
  const UInt numPartInCtuWidth = m_pcPic->getNumPartInCtuWidth();

  if ( !RasterAddress::isZeroCol( uiAbsPartIdx, numPartInCtuWidth ) )
  {
    uiLPartUnitIdx = g_auiRasterToZscan[ uiAbsPartIdx - 1 ];
    if ( RasterAddress::isEqualCol( uiAbsPartIdx, uiAbsZorderCUIdx, numPartInCtuWidth ) )
    {
      return m_pcPic->getCtu( getCtuRsAddr() );
    }
    else
    {
      uiLPartUnitIdx -= m_absZIdxInCtu;
      return this;
    }
  }

  uiLPartUnitIdx = g_auiRasterToZscan[ uiAbsPartIdx + numPartInCtuWidth - 1 ];
  if ( (bEnforceSliceRestriction && !CUIsFromSameSlice(m_pCtuLeft)) || (bEnforceTileRestriction && !CUIsFromSameTile(m_pCtuLeft)) )
  {
    return NULL;
  }
  return m_pCtuLeft;
}


TComDataCU* TComDataCU::getPUAbove( UInt& uiAPartUnitIdx,
                                    UInt uiCurrPartUnitIdx,
                                    Bool bEnforceSliceRestriction,
                                    Bool planarAtCtuBoundary,
                                    Bool bEnforceTileRestriction )
{
  UInt uiAbsPartIdx       = g_auiZscanToRaster[uiCurrPartUnitIdx];
  UInt uiAbsZorderCUIdx   = g_auiZscanToRaster[m_absZIdxInCtu];
  const UInt numPartInCtuWidth = m_pcPic->getNumPartInCtuWidth();

  if ( !RasterAddress::isZeroRow( uiAbsPartIdx, numPartInCtuWidth ) )
  {
    uiAPartUnitIdx = g_auiRasterToZscan[ uiAbsPartIdx - numPartInCtuWidth ];
    if ( RasterAddress::isEqualRow( uiAbsPartIdx, uiAbsZorderCUIdx, numPartInCtuWidth ) )
    {
      return m_pcPic->getCtu( getCtuRsAddr() );
    }
    else
    {
      uiAPartUnitIdx -= m_absZIdxInCtu;
      return this;
    }
  }

  if(planarAtCtuBoundary)
  {
    return NULL;
  }

  uiAPartUnitIdx = g_auiRasterToZscan[ uiAbsPartIdx + m_pcPic->getNumPartitionsInCtu() - numPartInCtuWidth ];

  if ( (bEnforceSliceRestriction && !CUIsFromSameSlice(m_pCtuAbove)) || (bEnforceTileRestriction && !CUIsFromSameTile(m_pCtuAbove)) )
  {
    return NULL;
  }
  return m_pCtuAbove;
}

TComDataCU* TComDataCU::getPUAboveLeft( UInt& uiALPartUnitIdx, UInt uiCurrPartUnitIdx, Bool bEnforceSliceRestriction )
{
  UInt uiAbsPartIdx       = g_auiZscanToRaster[uiCurrPartUnitIdx];
  UInt uiAbsZorderCUIdx   = g_auiZscanToRaster[m_absZIdxInCtu];
  const UInt numPartInCtuWidth = m_pcPic->getNumPartInCtuWidth();

  if ( !RasterAddress::isZeroCol( uiAbsPartIdx, numPartInCtuWidth ) )
  {
    if ( !RasterAddress::isZeroRow( uiAbsPartIdx, numPartInCtuWidth ) )
    {
      uiALPartUnitIdx = g_auiRasterToZscan[ uiAbsPartIdx - numPartInCtuWidth - 1 ];
      if ( RasterAddress::isEqualRowOrCol( uiAbsPartIdx, uiAbsZorderCUIdx, numPartInCtuWidth ) )
      {
        return m_pcPic->getCtu( getCtuRsAddr() );
      }
      else
      {
        uiALPartUnitIdx -= m_absZIdxInCtu;
        return this;
      }
    }
    uiALPartUnitIdx = g_auiRasterToZscan[ uiAbsPartIdx + getPic()->getNumPartitionsInCtu() - numPartInCtuWidth - 1 ];
    if ( bEnforceSliceRestriction && !CUIsFromSameSliceAndTile(m_pCtuAbove) )
    {
      return NULL;
    }
    return m_pCtuAbove;
  }

  if ( !RasterAddress::isZeroRow( uiAbsPartIdx, numPartInCtuWidth ) )
  {
    uiALPartUnitIdx = g_auiRasterToZscan[ uiAbsPartIdx - 1 ];
    if ( bEnforceSliceRestriction && !CUIsFromSameSliceAndTile(m_pCtuLeft) )
    {
      return NULL;
    }
    return m_pCtuLeft;
  }

  uiALPartUnitIdx = g_auiRasterToZscan[ m_pcPic->getNumPartitionsInCtu() - 1 ];
  if ( bEnforceSliceRestriction && !CUIsFromSameSliceAndTile(m_pCtuAboveLeft) )
  {
    return NULL;
  }
  return m_pCtuAboveLeft;
}

TComDataCU* TComDataCU::getPUBelowLeft(UInt& uiBLPartUnitIdx,  UInt uiCurrPartUnitIdx, UInt uiPartUnitOffset, Bool bEnforceSliceRestriction)
{
  UInt uiAbsPartIdxLB     = g_auiZscanToRaster[uiCurrPartUnitIdx];
  const UInt numPartInCtuWidth = m_pcPic->getNumPartInCtuWidth();
  UInt uiAbsZorderCUIdxLB = g_auiZscanToRaster[ m_absZIdxInCtu ] + ((m_puhHeight[0] / m_pcPic->getMinCUHeight()) - 1)*numPartInCtuWidth;

  if( ( m_pcPic->getCtu(m_ctuRsAddr)->getCUPelY() + g_auiRasterToPelY[uiAbsPartIdxLB] + (m_pcPic->getPicSym()->getMinCUHeight() * uiPartUnitOffset)) >= m_pcSlice->getSPS()->getPicHeightInLumaSamples())
  {
    uiBLPartUnitIdx = MAX_UINT;
    return NULL;
  }

  if ( RasterAddress::lessThanRow( uiAbsPartIdxLB, m_pcPic->getNumPartInCtuHeight() - uiPartUnitOffset, numPartInCtuWidth ) )
  {
    if ( !RasterAddress::isZeroCol( uiAbsPartIdxLB, numPartInCtuWidth ) )
    {
      if ( uiCurrPartUnitIdx > g_auiRasterToZscan[ uiAbsPartIdxLB + uiPartUnitOffset * numPartInCtuWidth - 1 ] )
      {
        uiBLPartUnitIdx = g_auiRasterToZscan[ uiAbsPartIdxLB + uiPartUnitOffset * numPartInCtuWidth - 1 ];
        if ( RasterAddress::isEqualRowOrCol( uiAbsPartIdxLB, uiAbsZorderCUIdxLB, numPartInCtuWidth ) )
        {
          return m_pcPic->getCtu( getCtuRsAddr() );
        }
        else
        {
          uiBLPartUnitIdx -= m_absZIdxInCtu;
          return this;
        }
      }
      uiBLPartUnitIdx = MAX_UINT;
      return NULL;
    }
    uiBLPartUnitIdx = g_auiRasterToZscan[ uiAbsPartIdxLB + (1+uiPartUnitOffset) * numPartInCtuWidth - 1 ];
    if ( bEnforceSliceRestriction && !CUIsFromSameSliceAndTile(m_pCtuLeft) )
    {
      return NULL;
    }
    return m_pCtuLeft;
  }

  uiBLPartUnitIdx = MAX_UINT;
  return NULL;
}

TComDataCU* TComDataCU::getPUAboveRight(UInt&  uiARPartUnitIdx, UInt uiCurrPartUnitIdx, UInt uiPartUnitOffset, Bool bEnforceSliceRestriction)
{
  UInt uiAbsPartIdxRT     = g_auiZscanToRaster[uiCurrPartUnitIdx];
  UInt uiAbsZorderCUIdx   = g_auiZscanToRaster[ m_absZIdxInCtu ] + (m_puhWidth[0] / m_pcPic->getMinCUWidth()) - 1;
  const UInt numPartInCtuWidth = m_pcPic->getNumPartInCtuWidth();

  if( ( m_pcPic->getCtu(m_ctuRsAddr)->getCUPelX() + g_auiRasterToPelX[uiAbsPartIdxRT] + (m_pcPic->getPicSym()->getMinCUHeight() * uiPartUnitOffset)) >= m_pcSlice->getSPS()->getPicWidthInLumaSamples() )
  {
    uiARPartUnitIdx = MAX_UINT;
    return NULL;
  }

  if ( RasterAddress::lessThanCol( uiAbsPartIdxRT, numPartInCtuWidth - uiPartUnitOffset, numPartInCtuWidth ) )
  {
    if ( !RasterAddress::isZeroRow( uiAbsPartIdxRT, numPartInCtuWidth ) )
    {
      if ( uiCurrPartUnitIdx > g_auiRasterToZscan[ uiAbsPartIdxRT - numPartInCtuWidth + uiPartUnitOffset ] )
      {
        uiARPartUnitIdx = g_auiRasterToZscan[ uiAbsPartIdxRT - numPartInCtuWidth + uiPartUnitOffset ];
        if ( RasterAddress::isEqualRowOrCol( uiAbsPartIdxRT, uiAbsZorderCUIdx, numPartInCtuWidth ) )
        {
          return m_pcPic->getCtu( getCtuRsAddr() );
        }
        else
        {
          uiARPartUnitIdx -= m_absZIdxInCtu;
          return this;
        }
      }
      uiARPartUnitIdx = MAX_UINT;
      return NULL;
    }

    uiARPartUnitIdx = g_auiRasterToZscan[ uiAbsPartIdxRT + m_pcPic->getNumPartitionsInCtu() - numPartInCtuWidth + uiPartUnitOffset ];
    if ( bEnforceSliceRestriction && !CUIsFromSameSliceAndTile(m_pCtuAbove) )
    {
      return NULL;
    }
    return m_pCtuAbove;
  }

  if ( !RasterAddress::isZeroRow( uiAbsPartIdxRT, numPartInCtuWidth ) )
  {
    uiARPartUnitIdx = MAX_UINT;
    return NULL;
  }

  uiARPartUnitIdx = g_auiRasterToZscan[ m_pcPic->getNumPartitionsInCtu() - numPartInCtuWidth + uiPartUnitOffset-1 ];
  if ( bEnforceSliceRestriction && !CUIsFromSameSliceAndTile(m_pCtuAboveRight) )
  {
    return NULL;
  }
  return m_pCtuAboveRight;
}

/** Get left QpMinCu
*\param   uiLPartUnitIdx
*\param   uiCurrAbsIdxInCtu
*\returns TComDataCU*   point of TComDataCU of left QpMinCu
*/
TComDataCU* TComDataCU::getQpMinCuLeft( UInt& uiLPartUnitIdx, UInt uiCurrAbsIdxInCtu )
{
  const UInt numPartInCtuWidth = m_pcPic->getNumPartInCtuWidth();
  const UInt maxCUDepth        = getSlice()->getSPS()->getMaxTotalCUDepth();
  const UInt maxCuDQPDepth     = getSlice()->getPPS()->getMaxCuDQPDepth();
  const UInt doubleDepthDifference = ((maxCUDepth - maxCuDQPDepth)<<1);
  UInt absZorderQpMinCUIdx = (uiCurrAbsIdxInCtu>>doubleDepthDifference)<<doubleDepthDifference;
  UInt absRorderQpMinCUIdx = g_auiZscanToRaster[absZorderQpMinCUIdx];

  // check for left CTU boundary
  if ( RasterAddress::isZeroCol(absRorderQpMinCUIdx, numPartInCtuWidth) )
  {
    return NULL;
  }

  // get index of left-CU relative to top-left corner of current quantization group
  uiLPartUnitIdx = g_auiRasterToZscan[absRorderQpMinCUIdx - 1];

  // return pointer to current CTU
  return m_pcPic->getCtu( getCtuRsAddr() );
}

/** Get Above QpMinCu
*\param   uiAPartUnitIdx
*\param   uiCurrAbsIdxInCtu
*\returns TComDataCU*   point of TComDataCU of above QpMinCu
*/
TComDataCU* TComDataCU::getQpMinCuAbove( UInt& uiAPartUnitIdx, UInt uiCurrAbsIdxInCtu )
{
  const UInt numPartInCtuWidth = m_pcPic->getNumPartInCtuWidth();
  const UInt maxCUDepth        = getSlice()->getSPS()->getMaxTotalCUDepth();
  const UInt maxCuDQPDepth     = getSlice()->getPPS()->getMaxCuDQPDepth();
  const UInt doubleDepthDifference = ((maxCUDepth - maxCuDQPDepth)<<1);
  UInt absZorderQpMinCUIdx = (uiCurrAbsIdxInCtu>>doubleDepthDifference)<<doubleDepthDifference;
  UInt absRorderQpMinCUIdx = g_auiZscanToRaster[absZorderQpMinCUIdx];

  // check for top CTU boundary
  if ( RasterAddress::isZeroRow( absRorderQpMinCUIdx, numPartInCtuWidth) )
  {
    return NULL;
  }

  // get index of top-CU relative to top-left corner of current quantization group
  uiAPartUnitIdx = g_auiRasterToZscan[absRorderQpMinCUIdx - numPartInCtuWidth];

  // return pointer to current CTU
  return m_pcPic->getCtu( getCtuRsAddr() );
}



/** Get reference QP from left QpMinCu or latest coded QP
*\param   uiCurrAbsIdxInCtu
*\returns Char   reference QP value
*/
Char TComDataCU::getRefQP( UInt uiCurrAbsIdxInCtu )
{
  UInt lPartIdx = MAX_UINT;
  UInt aPartIdx = MAX_UINT;
  TComDataCU* cULeft  = getQpMinCuLeft ( lPartIdx, m_absZIdxInCtu + uiCurrAbsIdxInCtu );
  TComDataCU* cUAbove = getQpMinCuAbove( aPartIdx, m_absZIdxInCtu + uiCurrAbsIdxInCtu );
  return (((cULeft? cULeft->getQP( lPartIdx ): getLastCodedQP( uiCurrAbsIdxInCtu )) + (cUAbove? cUAbove->getQP( aPartIdx ): getLastCodedQP( uiCurrAbsIdxInCtu )) + 1) >> 1);
}

Int TComDataCU::getLastValidPartIdx( Int iAbsPartIdx )
{
  Int iLastValidPartIdx = iAbsPartIdx-1;
  while ( iLastValidPartIdx >= 0
       && getPredictionMode( iLastValidPartIdx ) == NUMBER_OF_PREDICTION_MODES )
  {
    UInt uiDepth = getDepth( iLastValidPartIdx );
    iLastValidPartIdx -= m_uiNumPartition>>(uiDepth<<1);
  }
  return iLastValidPartIdx;
}

Char TComDataCU::getLastCodedQP( UInt uiAbsPartIdx )
{
  UInt uiQUPartIdxMask = ~((1<<((getSlice()->getSPS()->getMaxTotalCUDepth() - getSlice()->getPPS()->getMaxCuDQPDepth())<<1))-1);
  Int iLastValidPartIdx = getLastValidPartIdx( uiAbsPartIdx&uiQUPartIdxMask ); // A idx will be invalid if it is off the right or bottom edge of the picture.
  // If this CU is in the first CTU of the slice and there is no valid part before this one, use slice QP
  if ( getPic()->getPicSym()->getCtuTsToRsAddrMap(getSlice()->getSliceCurStartCtuTsAddr()) == getCtuRsAddr() && Int(getZorderIdxInCtu())+iLastValidPartIdx<0)
  {
    return getSlice()->getSliceQp();
  }
  else if ( iLastValidPartIdx >= 0 )
  {
    // If there is a valid part within the current Sub-CU, use it
    return getQP( iLastValidPartIdx );
  }
  else
  {
    if ( getZorderIdxInCtu() > 0 )
    {
      // If this wasn't the first sub-cu within the Ctu, explore the CTU itself.
      return getPic()->getCtu( getCtuRsAddr() )->getLastCodedQP( getZorderIdxInCtu() ); // TODO - remove this recursion
    }
    else if ( getPic()->getPicSym()->getCtuRsToTsAddrMap(getCtuRsAddr()) > 0
      && CUIsFromSameSliceTileAndWavefrontRow(getPic()->getCtu(getPic()->getPicSym()->getCtuTsToRsAddrMap(getPic()->getPicSym()->getCtuRsToTsAddrMap(getCtuRsAddr())-1))) )
    {
      // If this isn't the first Ctu (how can it be due to the first 'if'?), and the previous Ctu is from the same tile, examine the previous Ctu.
      return getPic()->getCtu( getPic()->getPicSym()->getCtuTsToRsAddrMap(getPic()->getPicSym()->getCtuRsToTsAddrMap(getCtuRsAddr())-1) )->getLastCodedQP( getPic()->getNumPartitionsInCtu() );  // TODO - remove this recursion
    }
    else
    {
      // No other options available - use the slice-level QP.
      return getSlice()->getSliceQp();
    }
  }
}


/** Check whether the CU is coded in lossless coding mode.
 * \param   absPartIdx
 * \returns true if the CU is coded in lossless coding mode; false if otherwise
 */
Bool TComDataCU::isLosslessCoded(UInt absPartIdx)
{
  return (getSlice()->getPPS()->getTransquantBypassEnableFlag() && getCUTransquantBypass (absPartIdx));
}


/** Get allowed chroma intra modes
*   - fills uiModeList with chroma intra modes
*
*\param   [in]  uiAbsPartIdx
*\param   [out] uiModeList pointer to chroma intra modes array
*/
Void TComDataCU::getAllowedChromaDir( UInt uiAbsPartIdx, UInt uiModeList[NUM_CHROMA_MODE] )
{
  uiModeList[0] = PLANAR_IDX;
  uiModeList[1] = VER_IDX;
  uiModeList[2] = HOR_IDX;
  uiModeList[3] = DC_IDX;
  uiModeList[4] = DM_CHROMA_IDX;
  assert(4<NUM_CHROMA_MODE);

  UInt uiLumaMode = getIntraDir( CHANNEL_TYPE_LUMA, uiAbsPartIdx );

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
*\param   uiAbsPartIdx    partition index
*\param   uiIntraDirPred  pointer to the array for MPM storage
*\param   compID          colour component ID
*\param   piMode          it is set with MPM mode in case both MPM are equal. It is used to restrict RD search at encode side.
*\returns Number of MPM
*/
Void TComDataCU::getIntraDirPredictor( UInt uiAbsPartIdx, Int uiIntraDirPred[NUM_MOST_PROBABLE_MODES], const ComponentID compID, Int* piMode  )
{
  TComDataCU* pcCULeft, *pcCUAbove;
  UInt        LeftPartIdx  = MAX_UINT;
  UInt        AbovePartIdx = MAX_UINT;
  Int         iLeftIntraDir, iAboveIntraDir;
  const TComSPS *sps=getSlice()->getSPS();
  const UInt partsPerMinCU = 1<<(2*(sps->getMaxTotalCUDepth() - sps->getLog2DiffMaxMinCodingBlockSize()));

  const ChannelType chType = toChannelType(compID);
  const ChromaFormat chForm = getPic()->getChromaFormat();
  // Get intra direction of left PU
  pcCULeft = getPULeft( LeftPartIdx, m_absZIdxInCtu + uiAbsPartIdx );

  if (isChroma(compID))
  {
    LeftPartIdx = getChromasCorrespondingPULumaIdx(LeftPartIdx, chForm, partsPerMinCU);
  }
  iLeftIntraDir  = pcCULeft ? ( pcCULeft->isIntra( LeftPartIdx ) ? pcCULeft->getIntraDir( chType, LeftPartIdx ) : DC_IDX ) : DC_IDX;
#if H_3D_DIM
  mapDepthModeToIntraDir( iLeftIntraDir );
#endif

  // Get intra direction of above PU
  pcCUAbove = getPUAbove( AbovePartIdx, m_absZIdxInCtu + uiAbsPartIdx, true, true );

  if (isChroma(compID))
  {
    AbovePartIdx = getChromasCorrespondingPULumaIdx(AbovePartIdx, chForm, partsPerMinCU);
  }
  iAboveIntraDir = pcCUAbove ? ( pcCUAbove->isIntra( AbovePartIdx ) ? pcCUAbove->getIntraDir( chType, AbovePartIdx ) : DC_IDX ) : DC_IDX;
#if H_3D_DIM
  mapDepthModeToIntraDir( iAboveIntraDir );
#endif


  if (isChroma(chType))
  {
    if (iLeftIntraDir  == DM_CHROMA_IDX)
    {
      iLeftIntraDir  = pcCULeft-> getIntraDir( CHANNEL_TYPE_LUMA, LeftPartIdx  );
    }
    if (iAboveIntraDir == DM_CHROMA_IDX)
    {
      iAboveIntraDir = pcCUAbove->getIntraDir( CHANNEL_TYPE_LUMA, AbovePartIdx );
    }
  }

  assert (2<NUM_MOST_PROBABLE_MODES);
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
  for (UInt i=0; i<NUM_MOST_PROBABLE_MODES; i++)
  {
    assert(uiIntraDirPred[i] < 35);
  }
}

UInt TComDataCU::getCtxSplitFlag( UInt uiAbsPartIdx, UInt uiDepth )
{
  TComDataCU* pcTempCU;
  UInt        uiTempPartIdx;
  UInt        uiCtx;
  // Get left split flag
  pcTempCU = getPULeft( uiTempPartIdx, m_absZIdxInCtu + uiAbsPartIdx );
  uiCtx  = ( pcTempCU ) ? ( ( pcTempCU->getDepth( uiTempPartIdx ) > uiDepth ) ? 1 : 0 ) : 0;

  // Get above split flag
  pcTempCU = getPUAbove( uiTempPartIdx, m_absZIdxInCtu + uiAbsPartIdx );
  uiCtx += ( pcTempCU ) ? ( ( pcTempCU->getDepth( uiTempPartIdx ) > uiDepth ) ? 1 : 0 ) : 0;

  return uiCtx;
}

UInt TComDataCU::getCtxQtCbf( TComTU &rTu, const ChannelType chType )
{
  const UInt transformDepth = rTu.GetTransformDepthRel();

  if (isChroma(chType))
  {
    return transformDepth;
  }
  else
  {
    const UInt uiCtx = ( transformDepth == 0 ? 1 : 0 );
    return uiCtx;
  }
}

UInt TComDataCU::getQuadtreeTULog2MinSizeInCU( UInt absPartIdx )
{
  UInt log2CbSize = g_aucConvertToBit[getWidth( absPartIdx )] + 2;
  PartSize  partSize  = getPartitionSize( absPartIdx );
  UInt quadtreeTUMaxDepth = isIntra( absPartIdx ) ? m_pcSlice->getSPS()->getQuadtreeTUMaxDepthIntra() : m_pcSlice->getSPS()->getQuadtreeTUMaxDepthInter();
  Int intraSplitFlag = ( isIntra( absPartIdx ) && partSize == SIZE_NxN ) ? 1 : 0;
  Int interSplitFlag = ((quadtreeTUMaxDepth == 1) && isInter( absPartIdx ) && (partSize != SIZE_2Nx2N) );

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
  pcTempCU = getPULeft( uiTempPartIdx, m_absZIdxInCtu + uiAbsPartIdx );
  uiCtx    = ( pcTempCU ) ? pcTempCU->isSkipped( uiTempPartIdx ) : 0;

  // Get BCBP of above PU
  pcTempCU = getPUAbove( uiTempPartIdx, m_absZIdxInCtu + uiAbsPartIdx );
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
    
    
    Bool depthRefineFlag = false;
#if NH_3D_NBDV_REF
    depthRefineFlag = m_pcSlice->getDepthRefinementFlag();
#endif // NH_3D_NBDV_REF
    
    TComMv cDv = depthRefineFlag ? DvInfo.m_acDoNBDV : DvInfo.m_acNBDV;
    if( depthRefineFlag )
    {
      cDv.setVer(0);
    }
    
    Int depthPosX = Clip3(0,   iPictureWidth - 1,  iBlkX + ((cDv.getHor()+2)>>2));
    Int depthPosY = Clip3(0,   iPictureHeight - 1, iBlkY + ((cDv.getVer()+2)>>2));
    
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


UChar TComDataCU::getQtRootCbf( UInt uiIdx )
{
  const UInt numberValidComponents = getPic()->getNumberValidComponents();
  return getCbf( uiIdx, COMPONENT_Y, 0 )
          || ((numberValidComponents > COMPONENT_Cb) && getCbf( uiIdx, COMPONENT_Cb, 0 ))
          || ((numberValidComponents > COMPONENT_Cr) && getCbf( uiIdx, COMPONENT_Cr, 0 ));
}

Void TComDataCU::setCbfSubParts( const UInt uiCbf[MAX_NUM_COMPONENT], UInt uiAbsPartIdx, UInt uiDepth )
{
  UInt uiCurrPartNumb = m_pcPic->getNumPartitionsInCtu() >> (uiDepth << 1);
  for(UInt comp=0; comp<MAX_NUM_COMPONENT; comp++)
  {
    memset( m_puhCbf[comp] + uiAbsPartIdx, uiCbf[comp], sizeof( UChar ) * uiCurrPartNumb );
  }
}

Void TComDataCU::setCbfSubParts( UInt uiCbf, ComponentID compID, UInt uiAbsPartIdx, UInt uiDepth )
{
  UInt uiCurrPartNumb = m_pcPic->getNumPartitionsInCtu() >> (uiDepth << 1);
  memset( m_puhCbf[compID] + uiAbsPartIdx, uiCbf, sizeof( UChar ) * uiCurrPartNumb );
}

/** Sets a coded block flag for all sub-partitions of a partition
 * \param uiCbf          The value of the coded block flag to be set
 * \param compID
 * \param uiAbsPartIdx
 * \param uiPartIdx
 * \param uiDepth
 */
Void TComDataCU::setCbfSubParts ( UInt uiCbf, ComponentID compID, UInt uiAbsPartIdx, UInt uiPartIdx, UInt uiDepth )
{
  setSubPart<UChar>( uiCbf, m_puhCbf[compID], uiAbsPartIdx, uiDepth, uiPartIdx );
}

Void TComDataCU::setCbfPartRange ( UInt uiCbf, ComponentID compID, UInt uiAbsPartIdx, UInt uiCoveredPartIdxes )
{
  memset((m_puhCbf[compID] + uiAbsPartIdx), uiCbf, (sizeof(UChar) * uiCoveredPartIdxes));
}

Void TComDataCU::bitwiseOrCbfPartRange( UInt uiCbf, ComponentID compID, UInt uiAbsPartIdx, UInt uiCoveredPartIdxes )
{
  const UInt stopAbsPartIdx = uiAbsPartIdx + uiCoveredPartIdxes;

  for (UInt subPartIdx = uiAbsPartIdx; subPartIdx < stopAbsPartIdx; subPartIdx++)
  {
    m_puhCbf[compID][subPartIdx] |= uiCbf;
  }
}

Void TComDataCU::setDepthSubParts( UInt uiDepth, UInt uiAbsPartIdx )
{
  UInt uiCurrPartNumb = m_pcPic->getNumPartitionsInCtu() >> (uiDepth << 1);
  memset( m_puhDepth + uiAbsPartIdx, uiDepth, sizeof(UChar)*uiCurrPartNumb );
}

Bool TComDataCU::isFirstAbsZorderIdxInDepth (UInt uiAbsPartIdx, UInt uiDepth)
{
  UInt uiPartNumb = m_pcPic->getNumPartitionsInCtu() >> (uiDepth << 1);
  return (((m_absZIdxInCtu + uiAbsPartIdx)% uiPartNumb) == 0);
}

Void TComDataCU::setPartSizeSubParts( PartSize eMode, UInt uiAbsPartIdx, UInt uiDepth )
{
  assert( sizeof( *m_pePartSize) == 1 );
  memset( m_pePartSize + uiAbsPartIdx, eMode, m_pcPic->getNumPartitionsInCtu() >> ( 2 * uiDepth ) );
}

Void TComDataCU::setCUTransquantBypassSubParts( Bool flag, UInt uiAbsPartIdx, UInt uiDepth )
{
  memset( m_CUTransquantBypass + uiAbsPartIdx, flag, m_pcPic->getNumPartitionsInCtu() >> ( 2 * uiDepth ) );
}

Void TComDataCU::setSkipFlagSubParts( Bool skip, UInt absPartIdx, UInt depth )
{
  assert( sizeof( *m_skipFlag) == 1 );
  memset( m_skipFlag + absPartIdx, skip, m_pcPic->getNumPartitionsInCtu() >> ( 2 * depth ) );
}

#if H_3D
Void TComDataCU::setDISFlagSubParts( Bool bDIS, UInt absPartIdx, UInt depth )
{
    assert( sizeof( *m_bDISFlag) == 1 );
    memset( m_bDISFlag + absPartIdx, bDIS, m_pcPic->getNumPartInCU() >> ( 2 * depth ) );
}

Void TComDataCU::setDISTypeSubParts(UInt uiDISType, UInt uiAbsPartIdx, UInt uiPUIdx, UInt uiDepth )
{
    setSubPartT( uiDISType, m_uiDISType, uiAbsPartIdx, uiDepth, uiPUIdx );
}
#endif

Void TComDataCU::setPredModeSubParts( PredMode eMode, UInt uiAbsPartIdx, UInt uiDepth )
{
  assert( sizeof( *m_pePredMode) == 1 );
  memset( m_pePredMode + uiAbsPartIdx, eMode, m_pcPic->getNumPartitionsInCtu() >> ( 2 * uiDepth ) );
}

Void TComDataCU::setChromaQpAdjSubParts( UChar val, Int absPartIdx, Int depth )
{
  assert( sizeof(*m_ChromaQpAdj) == 1 );
  memset( m_ChromaQpAdj + absPartIdx, val, m_pcPic->getNumPartitionsInCtu() >> ( 2 * depth ) );
}

Void TComDataCU::setQPSubCUs( Int qp, UInt absPartIdx, UInt depth, Bool &foundNonZeroCbf )
{
  UInt currPartNumb = m_pcPic->getNumPartitionsInCtu() >> (depth << 1);
  UInt currPartNumQ = currPartNumb >> 2;
  const UInt numValidComp = m_pcPic->getNumberValidComponents();

  if(!foundNonZeroCbf)
  {
    if(getDepth(absPartIdx) > depth)
    {
      for ( UInt partUnitIdx = 0; partUnitIdx < 4; partUnitIdx++ )
      {
        setQPSubCUs( qp, absPartIdx+partUnitIdx*currPartNumQ, depth+1, foundNonZeroCbf );
      }
    }
    else
    {
      if(getCbf( absPartIdx, COMPONENT_Y ) || (numValidComp>COMPONENT_Cb && getCbf( absPartIdx, COMPONENT_Cb )) || (numValidComp>COMPONENT_Cr && getCbf( absPartIdx, COMPONENT_Cr) ) )
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
  const UInt numPart = m_pcPic->getNumPartitionsInCtu() >> (uiDepth << 1);
  memset(m_phQP+uiAbsPartIdx, qp, numPart);
}

Void TComDataCU::setIntraDirSubParts( const ChannelType channelType, const UInt dir, const UInt absPartIdx, const UInt depth )
{
  UInt numPart = m_pcPic->getNumPartitionsInCtu() >> (depth << 1);
  memset( m_puhIntraDir[channelType] + absPartIdx, dir,sizeof(UChar)*numPart );
}

template<typename T>
Void TComDataCU::setSubPart( T uiParameter, T* puhBaseCtu, UInt uiCUAddr, UInt uiCUDepth, UInt uiPUIdx )
{
  assert( sizeof(T) == 1 ); // Using memset() works only for types of size 1

  UInt uiCurrPartNumQ = (m_pcPic->getNumPartitionsInCtu() >> (2 * uiCUDepth)) >> 2;
  switch ( m_pePartSize[ uiCUAddr ] )
  {
    case SIZE_2Nx2N:
      memset( puhBaseCtu + uiCUAddr, uiParameter, 4 * uiCurrPartNumQ );
      break;
    case SIZE_2NxN:
      memset( puhBaseCtu + uiCUAddr, uiParameter, 2 * uiCurrPartNumQ );
      break;
    case SIZE_Nx2N:
      memset( puhBaseCtu + uiCUAddr, uiParameter, uiCurrPartNumQ );
      memset( puhBaseCtu + uiCUAddr + 2 * uiCurrPartNumQ, uiParameter, uiCurrPartNumQ );
      break;
    case SIZE_NxN:
      memset( puhBaseCtu + uiCUAddr, uiParameter, uiCurrPartNumQ );
      break;
    case SIZE_2NxnU:
      if ( uiPUIdx == 0 )
      {
        memset( puhBaseCtu + uiCUAddr, uiParameter, (uiCurrPartNumQ >> 1) );
        memset( puhBaseCtu + uiCUAddr + uiCurrPartNumQ, uiParameter, (uiCurrPartNumQ >> 1) );
      }
      else if ( uiPUIdx == 1 )
      {
        memset( puhBaseCtu + uiCUAddr, uiParameter, (uiCurrPartNumQ >> 1) );
        memset( puhBaseCtu + uiCUAddr + uiCurrPartNumQ, uiParameter, ((uiCurrPartNumQ >> 1) + (uiCurrPartNumQ << 1)) );
      }
      else
      {
        assert(0);
      }
      break;
    case SIZE_2NxnD:
      if ( uiPUIdx == 0 )
      {
        memset( puhBaseCtu + uiCUAddr, uiParameter, ((uiCurrPartNumQ << 1) + (uiCurrPartNumQ >> 1)) );
        memset( puhBaseCtu + uiCUAddr + (uiCurrPartNumQ << 1) + uiCurrPartNumQ, uiParameter, (uiCurrPartNumQ >> 1) );
      }
      else if ( uiPUIdx == 1 )
      {
        memset( puhBaseCtu + uiCUAddr, uiParameter, (uiCurrPartNumQ >> 1) );
        memset( puhBaseCtu + uiCUAddr + uiCurrPartNumQ, uiParameter, (uiCurrPartNumQ >> 1) );
      }
      else
      {
        assert(0);
      }
      break;
    case SIZE_nLx2N:
      if ( uiPUIdx == 0 )
      {
        memset( puhBaseCtu + uiCUAddr, uiParameter, (uiCurrPartNumQ >> 2) );
        memset( puhBaseCtu + uiCUAddr + (uiCurrPartNumQ >> 1), uiParameter, (uiCurrPartNumQ >> 2) );
        memset( puhBaseCtu + uiCUAddr + (uiCurrPartNumQ << 1), uiParameter, (uiCurrPartNumQ >> 2) );
        memset( puhBaseCtu + uiCUAddr + (uiCurrPartNumQ << 1) + (uiCurrPartNumQ >> 1), uiParameter, (uiCurrPartNumQ >> 2) );
      }
      else if ( uiPUIdx == 1 )
      {
        memset( puhBaseCtu + uiCUAddr, uiParameter, (uiCurrPartNumQ >> 2) );
        memset( puhBaseCtu + uiCUAddr + (uiCurrPartNumQ >> 1), uiParameter, (uiCurrPartNumQ + (uiCurrPartNumQ >> 2)) );
        memset( puhBaseCtu + uiCUAddr + (uiCurrPartNumQ << 1), uiParameter, (uiCurrPartNumQ >> 2) );
        memset( puhBaseCtu + uiCUAddr + (uiCurrPartNumQ << 1) + (uiCurrPartNumQ >> 1), uiParameter, (uiCurrPartNumQ + (uiCurrPartNumQ >> 2)) );
      }
      else
      {
        assert(0);
      }
      break;
    case SIZE_nRx2N:
      if ( uiPUIdx == 0 )
      {
        memset( puhBaseCtu + uiCUAddr, uiParameter, (uiCurrPartNumQ + (uiCurrPartNumQ >> 2)) );
        memset( puhBaseCtu + uiCUAddr + uiCurrPartNumQ + (uiCurrPartNumQ >> 1), uiParameter, (uiCurrPartNumQ >> 2) );
        memset( puhBaseCtu + uiCUAddr + (uiCurrPartNumQ << 1), uiParameter, (uiCurrPartNumQ + (uiCurrPartNumQ >> 2)) );
        memset( puhBaseCtu + uiCUAddr + (uiCurrPartNumQ << 1) + uiCurrPartNumQ + (uiCurrPartNumQ >> 1), uiParameter, (uiCurrPartNumQ >> 2) );
      }
      else if ( uiPUIdx == 1 )
      {
        memset( puhBaseCtu + uiCUAddr, uiParameter, (uiCurrPartNumQ >> 2) );
        memset( puhBaseCtu + uiCUAddr + (uiCurrPartNumQ >> 1), uiParameter, (uiCurrPartNumQ >> 2) );
        memset( puhBaseCtu + uiCUAddr + (uiCurrPartNumQ << 1), uiParameter, (uiCurrPartNumQ >> 2) );
        memset( puhBaseCtu + uiCUAddr + (uiCurrPartNumQ << 1) + (uiCurrPartNumQ >> 1), uiParameter, (uiCurrPartNumQ >> 2) );
      }
      else
      {
        assert(0);
      }
      break;
    default:
      assert( 0 );
      break;
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

#if NH_3D_SPIVMP
Void TComDataCU::setSPIVMPFlagSubParts( Bool bSPIVMPFlag, UInt uiAbsPartIdx, UInt uiPartIdx, UInt uiDepth )
{
  setSubPart<Bool>( bSPIVMPFlag, m_pbSPIVMPFlag, uiAbsPartIdx, uiDepth, uiPartIdx );
}
#endif

#if NH_3D_VSP
Void TComDataCU::setVSPFlagSubParts( Char iVSPFlag, UInt uiAbsPartIdx, UInt uiPartIdx, UInt uiDepth )
{
  setSubPart<Char>( iVSPFlag, m_piVSPFlag, uiAbsPartIdx, uiDepth, uiPartIdx );
}
template<typename T>
Void TComDataCU::setSubPartT( T uiParameter, T* puhBaseLCU, UInt uiCUAddr, UInt uiCUDepth, UInt uiPUIdx )
{
  UInt uiCurrPartNumQ = (m_pcPic->getNumPartitionsInCtu() >> (2 * uiCUDepth)) >> 2;
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
  UInt uiCurrPartNumb = m_pcPic->getNumPartitionsInCtu() >> (uiDepth << 1);

  memset( m_puhTrIdx + uiAbsPartIdx, uiTrIdx, sizeof(UChar)*uiCurrPartNumb );
}

Void TComDataCU::setTransformSkipSubParts( const UInt useTransformSkip[MAX_NUM_COMPONENT], UInt uiAbsPartIdx, UInt uiDepth )
{
  UInt uiCurrPartNumb = m_pcPic->getNumPartitionsInCtu() >> (uiDepth << 1);

  for(UInt i=0; i<MAX_NUM_COMPONENT; i++)
  {
    memset( m_puhTransformSkip[i] + uiAbsPartIdx, useTransformSkip[i], sizeof( UChar ) * uiCurrPartNumb );
  }
}

Void TComDataCU::setTransformSkipSubParts( UInt useTransformSkip, ComponentID compID, UInt uiAbsPartIdx, UInt uiDepth)
{
  UInt uiCurrPartNumb = m_pcPic->getNumPartitionsInCtu() >> (uiDepth << 1);

  memset( m_puhTransformSkip[compID] + uiAbsPartIdx, useTransformSkip, sizeof( UChar ) * uiCurrPartNumb );
}

Void TComDataCU::setTransformSkipPartRange ( UInt useTransformSkip, ComponentID compID, UInt uiAbsPartIdx, UInt uiCoveredPartIdxes )
{
  memset((m_puhTransformSkip[compID] + uiAbsPartIdx), useTransformSkip, (sizeof(UChar) * uiCoveredPartIdxes));
}

Void TComDataCU::setCrossComponentPredictionAlphaPartRange( Char alphaValue, ComponentID compID, UInt uiAbsPartIdx, UInt uiCoveredPartIdxes )
{
  memset((m_crossComponentPredictionAlpha[compID] + uiAbsPartIdx), alphaValue, (sizeof(Char) * uiCoveredPartIdxes));
}

Void TComDataCU::setExplicitRdpcmModePartRange ( UInt rdpcmMode, ComponentID compID, UInt uiAbsPartIdx, UInt uiCoveredPartIdxes )
{
  memset((m_explicitRdpcmMode[compID] + uiAbsPartIdx), rdpcmMode, (sizeof(UChar) * uiCoveredPartIdxes));
}

Void TComDataCU::setSizeSubParts( UInt uiWidth, UInt uiHeight, UInt uiAbsPartIdx, UInt uiDepth )
{
  UInt uiCurrPartNumb = m_pcPic->getNumPartitionsInCtu() >> (uiDepth << 1);

  memset( m_puhWidth  + uiAbsPartIdx, uiWidth,  sizeof(UChar)*uiCurrPartNumb );
  memset( m_puhHeight + uiAbsPartIdx, uiHeight, sizeof(UChar)*uiCurrPartNumb );
}

UChar TComDataCU::getNumPartitions(const UInt uiAbsPartIdx)
{
  UChar iNumPart = 0;

  switch ( m_pePartSize[uiAbsPartIdx] )
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

// This is for use by a leaf/sub CU object only, with no additional AbsPartIdx
#if H_3D_IC | NH_3D_VSP
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
  ruiPartIdxLT = m_absZIdxInCtu + uiAbsPartIdx;
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

  ruiPartIdxLB      = g_auiRasterToZscan [g_auiZscanToRaster[ m_absZIdxInCtu + uiAbsPartIdx ] + ((uiPUHeight / m_pcPic->getMinCUHeight()) - 1)*m_pcPic->getNumPartInCtuWidth()];
}

Void TComDataCU::deriveLeftRightTopIdx ( UInt uiPartIdx, UInt& ruiPartIdxLT, UInt& ruiPartIdxRT )
{
  ruiPartIdxLT = m_absZIdxInCtu;
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
  ruiPartIdxLB      = g_auiRasterToZscan [g_auiZscanToRaster[ m_absZIdxInCtu ] + ( ((m_puhHeight[0] / m_pcPic->getMinCUHeight())>>1) - 1)*m_pcPic->getNumPartInCtuWidth()];

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

/** Derive the partition index of neighbouring bottom right block
 * \param [in]  uiPartIdx     current partition index
 * \param [out] ruiPartIdxRB  partition index of neighbouring bottom right block
 */
Void TComDataCU::deriveRightBottomIdx( UInt uiPartIdx, UInt &ruiPartIdxRB )
{
  ruiPartIdxRB      = g_auiRasterToZscan [g_auiZscanToRaster[ m_absZIdxInCtu ] + ( ((m_puhHeight[0] / m_pcPic->getMinCUHeight())>>1) - 1)*m_pcPic->getNumPartInCtuWidth() +  m_puhWidth[0] / m_pcPic->getMinCUWidth() - 1];

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

#if NH_3D_VSP
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
  if ( m_pcSlice->getViewIndex() == 0 || !m_pcSlice->getViewSynthesisPredFlag( ) || m_pcSlice->getIsDepth() || pDInfo->m_aVIdxCan == -1)
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
#if NH_3D_NBDV
        mvVSP[0+iRefListIdX].getMv().setIDVFlag (false);
#endif
      }
    }
  }

  dirVSP = (predFlag[0] | (predFlag[1] << 1));
  m_mergCands[MRG_VSP].setCand( mvVSP, dirVSP, true
#if NH_3D_SPIVMP
    , false
#endif
    );
  if ( mrgCandIdx == iCount )
  {
    return true;
  }

  iCount++;

  return false;
}
#endif

#if NH_3D_IV_MERGE
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
#if NH_3D_NBDV
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
#if NH_3D_MLC
/** Construct a extended list of merging candidates
 * \param pcMvFieldNeighbours
 * \param puhInterDirNeighbours
 * \param vspFlag
 * \param pbSPIVMPFlag
 * \param numValidMergeCand
 */
Void TComDataCU::buildMCL(TComMvField* pcMvFieldNeighbours, UChar* puhInterDirNeighbours
#if NH_3D_VSP
  , Int* vspFlag
#endif
#if NH_3D_SPIVMP
  , Bool* pbSPIVMPFlag
#endif
  , Int& numValidMergeCand
  )
{
  if (!( getSlice()->getIsDepth() || getSlice()->getViewIndex()>0))
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
#if NH_3D_VSP
    vspFlag[ui] = 0;
#endif
  }

  // insert MPI ... IvShift candidate to extMergeCandList
  for (Int i=0; i<=MRG_IVSHIFT; i++)
  {
    if (m_mergCands[i].m_bAvailable)
    {
      m_mergCands[i].getCand(iCount, extMergeCandList, uhInterDirNeighboursExt
#if NH_3D_VSP
        , vspFlag
#endif
#if NH_3D_SPIVMP
        , pbSPIVMPFlag
#endif
        );
      iCount++;
      if (iCount >= getSlice()->getMaxNumMergeCand())
        break;
    }
  }

  Int iCountBase = m_numSpatialCands;
  // insert remaining base candidates to extMergeCandList
  while (iCount < getSlice()->getMaxNumMergeCand() && iCountBase < getSlice()->getMaxNumMergeCand())
  {
    uhInterDirNeighboursExt[iCount] = puhInterDirNeighbours[iCountBase];
    extMergeCandList[iCount<<1].setMvField(pcMvFieldNeighbours[iCountBase<<1].getMv(), pcMvFieldNeighbours[iCountBase<<1].getRefIdx());
    if ( getSlice()->isInterB() )
    {
      extMergeCandList[(iCount<<1)+1].setMvField(pcMvFieldNeighbours[(iCountBase<<1)+1].getMv(), pcMvFieldNeighbours[(iCountBase<<1)+1].getRefIdx());
    }
    iCountBase++;
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



/** Derive 3D merge candidates
 * \param uiAbsPartIdx
 * \param uiPUIdx 
 * \param pcMvFieldNeighbours
 * \param puhInterDirNeighbours
 * \param pcMvFieldSP
 * \param puhInterDirNeighbours
 * \param numValidMergeCand
 */
Void TComDataCU::xGetInterMergeCandidates( UInt uiAbsPartIdx, UInt uiPUIdx, TComMvField* pcMFieldNeighbours, UChar* puhInterDirNeighbours
#if NH_3D_SPIVMP
      , TComMvField* pcMvFieldSP, UChar* puhInterDirSP
#endif
      , Int& numValidMergeCand, Int mrgCandIdx
)
{
#if NH_3D_IV_MERGE
  TComMv cZeroMv;
  TComMvField tmpMV[2];  
#endif

  //////////////////////////////////
  //////// GET DISPARITIES  ////////
  //////////////////////////////////
#if NH_3D_IV_MERGE
  DisInfo cDisInfo = getDvInfo(uiAbsPartIdx);
  m_cDefaultDisInfo = cDisInfo;
#elif NH_3D_VSP
  // for xAddVspCand()
  DisInfo cDisInfo = getDvInfo(uiAbsPartIdx);
#endif

  if (!( getSlice()->getIsDepth() || getSlice()->getViewIndex()>0))
  {
    return;
  }
  numValidMergeCand = getSlice()->getMaxNumMergeCand();
  //////////////////////////////////
  //////// DERIVE LOCATIONS ////////
  //////////////////////////////////
  // compute the location of the current PU
  Int xP, yP, nPSW, nPSH;
  this->getPartPosition(uiPUIdx, xP, yP, nPSW, nPSH);

  Int iCount = 0;
  UInt uiPartIdxLT, uiPartIdxRT, uiPartIdxLB;
  deriveLeftRightTopIdxGeneral( uiAbsPartIdx, uiPUIdx, uiPartIdxLT, uiPartIdxRT );
  deriveLeftBottomIdxGeneral  ( uiAbsPartIdx, uiPUIdx, uiPartIdxLB );
#if NH_3D_TEXT_MERGE
  Bool bMPIFlag   = getSlice()->getMpiFlag(); 
  Int  tmpDir;
#endif 
#if NH_3D_IV_MERGE || NH_3D_TEXT_MERGE
  Bool bIsDepth = getSlice()->getIsDepth();
#endif

#if H_3D_IC
  Bool bICFlag = getICFlag(uiAbsPartIdx);
#endif
#if H_3D_ARP
  Bool bARPFlag = getARPW(uiAbsPartIdx) > 0;
#endif
#if H_3D_DBBP
  Bool bDBBPFlag = getDBBPFlag(uiAbsPartIdx);
  assert(bDBBPFlag == getDBBPFlag(0));  
#endif

#if NH_3D_NBDV
  for(Int i = 0; i < MRG_MAX_NUM_CANDS_MEM; i++)  
  {
    pcMFieldNeighbours[i<<1    ].getMv().setIDVFlag (false);
    pcMFieldNeighbours[(i<<1)+1].getMv().setIDVFlag (false);
  }
#endif
  // init containers
  for (Int i = 0; i<MRG_IVSHIFT+1; i++)
    m_mergCands[i].init();

  m_numSpatialCands = 0;

  //////////////////////////////////
  ///////// GET VSP FLAGS //////////
  //////////////////////////////////
  //left
  UInt uiLeftPartIdx = 0;
  TComDataCU* pcCULeft = 0;
  pcCULeft = getPULeft( uiLeftPartIdx, uiPartIdxLB );  

  if (getAvailableFlagA1())
  {
    m_mergCands[MRG_A1].setCand( &pcMFieldNeighbours[m_numSpatialCands<<1], puhInterDirNeighbours[m_numSpatialCands]
#if NH_3D_VSP
    , (pcCULeft->getVSPFlag(uiLeftPartIdx) != 0
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
#if NH_3D_SPIVMP
      , false
#endif
      ); 
    m_numSpatialCands++;
  }

  // above
  if (getAvailableFlagB1())
  {
    m_mergCands[MRG_B1].setCand( &pcMFieldNeighbours[m_numSpatialCands<<1], puhInterDirNeighbours[m_numSpatialCands]
#if NH_3D_VSP
    , false
#endif
#if NH_3D_SPIVMP
      , false
#endif
      ); 
    m_numSpatialCands++;
  }

  // above right
  if (getAvailableFlagB0())
  {
    m_mergCands[MRG_B0].setCand( &pcMFieldNeighbours[m_numSpatialCands<<1], puhInterDirNeighbours[m_numSpatialCands]
#if NH_3D_VSP
    , false
#endif
#if NH_3D_SPIVMP
      , false
#endif
      ); 
    m_numSpatialCands++;
  }

  // left bottom
  if (getAvailableFlagA0())
  {
    m_mergCands[MRG_A0].setCand( &pcMFieldNeighbours[m_numSpatialCands<<1], puhInterDirNeighbours[m_numSpatialCands]
#if NH_3D_VSP
    , false
#endif
#if NH_3D_SPIVMP
      , false
#endif
      ); 
    m_numSpatialCands++;
  }

  // above left
  if (getAvailableFlagB2())
  {
    m_mergCands[MRG_B2].setCand( &pcMFieldNeighbours[m_numSpatialCands<<1], puhInterDirNeighbours[m_numSpatialCands]
#if NH_3D_VSP
    , false
#endif
#if NH_3D_SPIVMP
      , false
#endif
      ); 
    m_numSpatialCands++;
  }


#if NH_3D_TEXT_MERGE

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
    if (pcTexPic && pcTexPic->getReconMark())
    {
#endif    
      TComPicYuv*   pcTexRec = pcTexPic->getPicYuvRec  ();
      UInt          uiPartAddr;
      Int           iWidth, iHeight;
      Int           iCurrPosX, iCurrPosY;

      this->getPartIndexAndSize( uiPUIdx, uiPartAddr, iWidth, iHeight );
      pcTexRec->getTopLeftSamplePos( this->getCtuRsAddr(), this->getZorderIdxInCtu() + uiPartAddr, iCurrPosX, iCurrPosY );

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
#if NH_3D_INTEGER_MV_DEPTH
      const TComMv cMvRounding( 1 << ( 2 - 1 ), 1 << ( 2 - 1 ) );
#endif
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
      TComDataCU* pcDefaultCU    = pcTexPic->getCtu( iTexCenterCUAddr );

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
#if NH_3D_INTEGER_MV_DEPTH
                TComMv cMv = cDefaultMvField.getMv() + cMvRounding;
                cMv >>= 2;
#else
                TComMv cMv = cDefaultMvField.getMv();
#endif
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
            pcTexCU  = pcTexPic->getCtu( iTexCUAddr );

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
#if NH_3D_INTEGER_MV_DEPTH
                  TComMv cMv = cTexMvField.getMv() + cMvRounding;
                  cMv >>=2;          
#else
                  TComMv cMv = cTexMvField.getMv();
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

            iPartition ++;
          }
        }
      }
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
#endif

#if NH_3D_IV_MERGE
  /////////////////////////////////////////////////////////////////
  //////// DERIVE IvMC, IvMCShift,IvDCShift, IvDC  Candidates /////
  /////////////////////////////////////////////////////////////////

  // { IvMCL0, IvMCL1, IvDCL0, IvDCL1, IvMCL0Shift, IvMCL1Shift, IvDCL0Shift, IvDCL1Shift };  
  // An enumerator would be appropriate here! 
  TComMv ivCandMv    [8];
  Int    ivCandRefIdx[8] = {-1, -1, -1, -1, -1, -1, -1, -1};

  // { IvMC, IvDC, IvMCShift, IvDCShift };  
  Int    ivCandDir   [4] = {0, 0, 0, 0};

  Bool ivMvPredFlag   = getSlice()->getIvMvPredFlag();

  ivMvPredFlag &= (nPSW + nPSH > 12);
  if ( ivMvPredFlag && cDisInfo.m_aVIdxCan!=-1)
  {
#if H_3D_IC
    getInterViewMergeCands(uiPUIdx, ivCandRefIdx, ivCandMv, &cDisInfo, ivCandDir , bIsDepth, pcMvFieldSP, puhInterDirSP, bICFlag );
#else
    getInterViewMergeCands(uiPUIdx, ivCandRefIdx, ivCandMv, &cDisInfo, ivCandDir , bIsDepth, pcMvFieldSP, puhInterDirSP, false );
#endif
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
      Bool spiMvpFlag = false;
      if(!m_pcSlice->getIsDepth())
      {
        spiMvpFlag = true;
      }
#if H_3D_DBBP
      spiMvpFlag &= !bDBBPFlag;
#endif

      m_mergCands[MRG_IVMC].setCand( tmpMV, ivCandDir[0], false, spiMvpFlag);

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

  iCount += m_mergCands[MRG_A1].m_bAvailable + m_mergCands[MRG_B1].m_bAvailable;

#if NH_3D_VSP
  /////////////////////////////////////////////////
  //////// VIEW SYNTHESIS PREDICTION (VSP) ////////
  /////////////////////////////////////////////////
  if (iCount<getSlice()->getMaxNumMergeCand())
  {
    if (
      (!getAvailableFlagA1() || !(pcCULeft->getVSPFlag(uiLeftPartIdx) != 0)) &&
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
  }
#endif

  iCount += m_mergCands[MRG_B0].m_bAvailable;

#if NH_3D_IV_MERGE 
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
#if NH_3D_NBDV
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

  iCount += m_mergCands[MRG_A0].m_bAvailable + m_mergCands[MRG_B2].m_bAvailable;

#if NH_3D_IV_MERGE
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
}
#endif

//! Construct a list of merging candidates
Void TComDataCU::getInterMergeCandidates( UInt uiAbsPartIdx, UInt uiPUIdx, TComMvField* pcMvFieldNeighbours, UChar* puhInterDirNeighbours, Int& numValidMergeCand, Int mrgCandIdx )
{
  UInt uiAbsPartAddr = m_absZIdxInCtu + uiAbsPartIdx;
#if NH_3D_MLC
  Bool abCandIsInter[ MRG_MAX_NUM_CANDS_MEM ];
#else
  Bool abCandIsInter[ MRG_MAX_NUM_CANDS ];
#endif
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
  deriveLeftBottomIdxGeneral( uiAbsPartIdx, uiPUIdx, uiPartIdxLB );

  //left
  UInt uiLeftPartIdx = 0;
  TComDataCU* pcCULeft = 0;
  pcCULeft = getPULeft( uiLeftPartIdx, uiPartIdxLB );

  Bool isAvailableA1 = pcCULeft &&
                       pcCULeft->isDiffMER(xP -1, yP+nPSH-1, xP, yP) &&
                       !( uiPUIdx == 1 && (cCurPS == SIZE_Nx2N || cCurPS == SIZE_nLx2N || cCurPS == SIZE_nRx2N) ) &&
                       pcCULeft->isInter( uiLeftPartIdx ) ;

  if ( isAvailableA1 )
  {
#if NH_3D_MLC
    m_bAvailableFlagA1 = 1;
#endif
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
                       pcCUAbove->isInter( uiAbovePartIdx );

  if ( isAvailableB1 && (!isAvailableA1 || !pcCULeft->hasEqualMotion( uiLeftPartIdx, pcCUAbove, uiAbovePartIdx ) ) )
  {
#if NH_3D_MLC
    m_bAvailableFlagB1 = 1;
#endif
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
                       pcCUAboveRight->isInter( uiAboveRightPartIdx );

  if ( isAvailableB0 && ( !isAvailableB1 || !pcCUAbove->hasEqualMotion( uiAbovePartIdx, pcCUAboveRight, uiAboveRightPartIdx ) ) )
  {
#if NH_3D_MLC
    m_bAvailableFlagB0 = 1;
#endif
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
                       pcCULeftBottom->isInter( uiLeftBottomPartIdx ) ;

  if ( isAvailableA0 && ( !isAvailableA1 || !pcCULeft->hasEqualMotion( uiLeftPartIdx, pcCULeftBottom, uiLeftBottomPartIdx ) ) )
  {
#if NH_3D_MLC
    m_bAvailableFlagA0 = 1;
#endif
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
                         pcCUAboveLeft->isInter( uiAboveLeftPartIdx );

    if ( isAvailableB2 && ( !isAvailableA1 || !pcCULeft->hasEqualMotion( uiLeftPartIdx, pcCUAboveLeft, uiAboveLeftPartIdx ) )
        && ( !isAvailableB1 || !pcCUAbove->hasEqualMotion( uiAbovePartIdx, pcCUAboveLeft, uiAboveLeftPartIdx ) ) )
    {
#if NH_3D_MLC
      m_bAvailableFlagB2 = 1;
#endif
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

  if ( getSlice()->getEnableTMVPFlag() )
  {
    //>> MTK colocated-RightBottom
    UInt uiPartIdxRB;

    deriveRightBottomIdx( uiPUIdx, uiPartIdxRB );

    UInt uiAbsPartIdxTmp = g_auiZscanToRaster[uiPartIdxRB];
    const UInt numPartInCtuWidth  = m_pcPic->getNumPartInCtuWidth();
    const UInt numPartInCtuHeight = m_pcPic->getNumPartInCtuHeight();

    TComMv cColMv;
    Int iRefIdx;
    Int ctuRsAddr = -1;

    if (   ( ( m_pcPic->getCtu(m_ctuRsAddr)->getCUPelX() + g_auiRasterToPelX[uiAbsPartIdxTmp] + m_pcPic->getMinCUWidth () ) < m_pcSlice->getSPS()->getPicWidthInLumaSamples () )  // image boundary check
        && ( ( m_pcPic->getCtu(m_ctuRsAddr)->getCUPelY() + g_auiRasterToPelY[uiAbsPartIdxTmp] + m_pcPic->getMinCUHeight() ) < m_pcSlice->getSPS()->getPicHeightInLumaSamples() ) )
    {
      if ( ( uiAbsPartIdxTmp % numPartInCtuWidth < numPartInCtuWidth - 1 ) &&           // is not at the last column of CTU
        ( uiAbsPartIdxTmp / numPartInCtuWidth < numPartInCtuHeight - 1 ) )              // is not at the last row    of CTU
      {
        uiAbsPartAddr = g_auiRasterToZscan[ uiAbsPartIdxTmp + numPartInCtuWidth + 1 ];
        ctuRsAddr = getCtuRsAddr();
      }
      else if ( uiAbsPartIdxTmp % numPartInCtuWidth < numPartInCtuWidth - 1 )           // is not at the last column of CTU But is last row of CTU
      {
        uiAbsPartAddr = g_auiRasterToZscan[ (uiAbsPartIdxTmp + numPartInCtuWidth + 1) % m_pcPic->getNumPartitionsInCtu() ];
      }
      else if ( uiAbsPartIdxTmp / numPartInCtuWidth < numPartInCtuHeight - 1 )          // is not at the last row of CTU But is last column of CTU
      {
        uiAbsPartAddr = g_auiRasterToZscan[ uiAbsPartIdxTmp + 1 ];
        ctuRsAddr = getCtuRsAddr() + 1;
      }
      else //is the right bottom corner of CTU
      {
        uiAbsPartAddr = 0;
      }
    }

    iRefIdx = 0;

    Bool bExistMV = false;
    UInt uiPartIdxCenter;
    Int dir = 0;
    UInt uiArrayAddr = iCount;
    xDeriveCenterIdx( uiPUIdx, uiPartIdxCenter );
    bExistMV = ctuRsAddr >= 0 && xGetColMVP( REF_PIC_LIST_0, ctuRsAddr, uiAbsPartAddr, cColMv, iRefIdx );
    if( bExistMV == false )
    {
      bExistMV = xGetColMVP( REF_PIC_LIST_0, getCtuRsAddr(), uiPartIdxCenter,  cColMv, iRefIdx );
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
      bExistMV = ctuRsAddr >= 0 && xGetColMVP( REF_PIC_LIST_1, ctuRsAddr, uiAbsPartAddr, cColMv, iRefIdx);
      if( bExistMV == false )
      {
        bExistMV = xGetColMVP( REF_PIC_LIST_1, getCtuRsAddr(), uiPartIdxCenter, cColMv, iRefIdx );
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
#if NH_3D_NBDV
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

#if NH_3D_MLC
  if ( getSlice()->isInterB() && iCount<5)
#else
  if ( getSlice()->isInterB() )
#endif
  {
    static const UInt NUM_PRIORITY_LIST=12;
    static const UInt uiPriorityList0[NUM_PRIORITY_LIST] = {0 , 1, 0, 2, 1, 2, 0, 3, 1, 3, 2, 3};
    static const UInt uiPriorityList1[NUM_PRIORITY_LIST] = {1 , 0, 2, 0, 2, 1, 3, 0, 3, 1, 3, 2};

    for (Int idx=0; idx<uiCutoff*(uiCutoff-1) && uiArrayAddr!= getSlice()->getMaxNumMergeCand(); idx++)
    {
      assert(idx<NUM_PRIORITY_LIST);
      Int i = uiPriorityList0[idx];
      Int j = uiPriorityList1[idx];
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

/** Check whether the current PU and a spatial neighboring PU are in a same ME region.
 * \param xN, yN   location of the upper-left corner pixel of a neighboring PU
 * \param xP, yP   location of the upper-left corner pixel of the current PU
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

/** Calculate the location of upper-left corner pixel and size of the current PU.
 * \param partIdx       PU index within a CU
 * \param xP, yP        location of the upper-left corner pixel of the current PU
 * \param nPSW, nPSH    size of the current PU
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
  const UInt numPartInCtuWidth  = m_pcPic->getNumPartInCtuWidth();
  const UInt numPartInCtuHeight = m_pcPic->getNumPartInCtuHeight();
  Bool bAdded = false;

  deriveLeftRightTopIdx( uiPartIdx, uiPartIdxLT, uiPartIdxRT );
  deriveLeftBottomIdx( uiPartIdx, uiPartIdxLB );

  TComDataCU* tmpCU = NULL;
  UInt idx;
  tmpCU = getPUBelowLeft(idx, uiPartIdxLB);
  bAddedSmvp = (tmpCU != NULL) && (tmpCU->isInter(idx));

  if (!bAddedSmvp)
  {
    tmpCU = getPULeft(idx, uiPartIdxLB);
    bAddedSmvp = (tmpCU != NULL) && (tmpCU->isInter(idx));
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

  if(!bAddedSmvp)
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
    uiAbsPartAddr = m_absZIdxInCtu + uiPartAddr;

    //----  co-located RightBottom Temporal Predictor (H) ---//
    uiAbsPartIdx = g_auiZscanToRaster[uiPartIdxRB];
    Int ctuRsAddr = -1;
    if (  ( ( m_pcPic->getCtu(m_ctuRsAddr)->getCUPelX() + g_auiRasterToPelX[uiAbsPartIdx] + m_pcPic->getMinCUWidth () ) < m_pcSlice->getSPS()->getPicWidthInLumaSamples () )  // image boundary check
       && ( ( m_pcPic->getCtu(m_ctuRsAddr)->getCUPelY() + g_auiRasterToPelY[uiAbsPartIdx] + m_pcPic->getMinCUHeight() ) < m_pcSlice->getSPS()->getPicHeightInLumaSamples() ) )
    {
      if ( ( uiAbsPartIdx % numPartInCtuWidth < numPartInCtuWidth - 1 ) &&  // is not at the last column of CTU
           ( uiAbsPartIdx / numPartInCtuWidth < numPartInCtuHeight - 1 ) )  // is not at the last row    of CTU
      {
        uiAbsPartAddr = g_auiRasterToZscan[ uiAbsPartIdx + numPartInCtuWidth + 1 ];
        ctuRsAddr = getCtuRsAddr();
      }
      else if ( uiAbsPartIdx % numPartInCtuWidth < numPartInCtuWidth - 1 )  // is not at the last column of CTU But is last row of CTU
      {
        uiAbsPartAddr = g_auiRasterToZscan[ (uiAbsPartIdx + numPartInCtuWidth + 1) % m_pcPic->getNumPartitionsInCtu() ];
      }
      else if ( uiAbsPartIdx / numPartInCtuWidth < numPartInCtuHeight - 1 ) // is not at the last row of CTU But is last column of CTU
      {
        uiAbsPartAddr = g_auiRasterToZscan[ uiAbsPartIdx + 1 ];
        ctuRsAddr = getCtuRsAddr() + 1;
      }
      else //is the right bottom corner of CTU
      {
        uiAbsPartAddr = 0;
      }
    }
    if ( ctuRsAddr >= 0 && xGetColMVP( eRefPicList, ctuRsAddr, uiAbsPartAddr, cColMv, iRefIdx_Col
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
      xDeriveCenterIdx( uiPartIdx, uiPartIdxCenter );
      if (xGetColMVP( eRefPicList, getCtuRsAddr(), uiPartIdxCenter,  cColMv, iRefIdx_Col 
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
  const TComSPS &sps=*(m_pcSlice->getSPS());
  Int  iMvShift = 2;
#if NH_3D_INTEGER_MV_DEPTH
  if( getSlice()->getIsDepth() )
    iMvShift = 0;
#endif

  Int iOffset = 8;
  Int iHorMax = ( sps.getPicWidthInLumaSamples() + iOffset - (Int)m_uiCUPelX - 1 ) << iMvShift;
  Int iHorMin = (      -(Int)sps.getMaxCUWidth() - iOffset - (Int)m_uiCUPelX + 1 ) << iMvShift;

  Int iVerMax = ( sps.getPicHeightInLumaSamples() + iOffset - (Int)m_uiCUPelY - 1 ) << iMvShift;
  Int iVerMin = (      -(Int)sps.getMaxCUHeight() - iOffset - (Int)m_uiCUPelY + 1 ) << iMvShift;

  rcMv.setHor( min (iHorMax, max (iHorMin, rcMv.getHor())) );
  rcMv.setVer( min (iVerMax, max (iVerMin, rcMv.getVer())) );
}

#if NH_MV
Void TComDataCU::checkMvVertRest (TComMv&  rcMv,  RefPicList eRefPicList, int iRefIdx )
{
  if ( getSlice()->getSPS()->getInterViewMvVertConstraintFlag() )
  {
    if ( getSlice()->getRefPic( eRefPicList, iRefIdx )->getPOC() == getSlice()->getPOC() )
    {
        //When inter_view_mv_vert_constraint_flag is equal to 1,
        //the vertical component of the motion vectors used for inter-layer prediction 
        //shall be equal to or less than 56 in units of luma samples
        assert ( rcMv.getVer() <= (56<<2) );
    }
  }
}
#endif

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

Void TComDataCU::clearCbf( UInt uiIdx, ComponentID compID, UInt uiNumParts )
{
  memset( &m_puhCbf[compID][uiIdx], 0, sizeof(UChar)*uiNumParts);
}

/** Set a I_PCM flag for all sub-partitions of a partition.
 * \param bIpcmFlag I_PCM flag
 * \param uiAbsPartIdx patition index
 * \param uiDepth CU depth
 * \returns Void
 */
Void TComDataCU::setIPCMFlagSubParts  (Bool bIpcmFlag, UInt uiAbsPartIdx, UInt uiDepth)
{
  UInt uiCurrPartNumb = m_pcPic->getNumPartitionsInCtu() >> (uiDepth << 1);

  memset(m_pbIPCMFlag + uiAbsPartIdx, bIpcmFlag, sizeof(Bool)*uiCurrPartNumb );
}

/** Test whether the block at uiPartIdx is skipped.
 * \param uiPartIdx Partition index
 * \returns true if the current the block is skipped
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

Bool TComDataCU::xGetColMVP( RefPicList eRefPicList, Int ctuRsAddr, Int uiPartUnitIdx, TComMv& rcMv, Int& riRefIdx 
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
  TComDataCU *pColCtu = pColPic->getCtu( ctuRsAddr );
  if(pColCtu->getPic()==0||pColCtu->getPartitionSize(uiPartUnitIdx)==NUMBER_OF_PART_SIZES)
  {
    return false;
  }
  iCurrPOC = m_pcSlice->getPOC();
  iColPOC = pColCtu->getSlice()->getPOC();

  if (!pColCtu->isInter(uiAbsPartAddr))
  {
    return false;
  }

  eColRefPicList = getSlice()->getCheckLDC() ? eRefPicList : RefPicList(getSlice()->getColFromL0Flag());

  Int iColRefIdx = pColCtu->getCUMvField(RefPicList(eColRefPicList))->getRefIdx(uiAbsPartAddr);

  if (iColRefIdx < 0 )
  {
    eColRefPicList = RefPicList(1 - eColRefPicList);
    iColRefIdx = pColCtu->getCUMvField(RefPicList(eColRefPicList))->getRefIdx(uiAbsPartAddr);

    if (iColRefIdx < 0 )
    {
      return false;
    }
  }

  // Scale the vector.
  iColRefPOC = pColCtu->getSlice()->getRefPOC(eColRefPicList, iColRefIdx);
  cColMv = pColCtu->getCUMvField(eColRefPicList)->getMv(uiAbsPartAddr);

  iCurrRefPOC = m_pcSlice->getRefPic(eRefPicList, riRefIdx)->getPOC();

  Bool bIsCurrRefLongTerm = m_pcSlice->getRefPic(eRefPicList, riRefIdx)->getIsLongTerm();
  Bool bIsColRefLongTerm = pColCtu->getSlice()->getIsUsedAsLongTerm(eColRefPicList, iColRefIdx);

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
    Int iCurrViewId    = m_pcSlice->getViewId (); 
    Int iCurrRefViewId = m_pcSlice->getRefPic(eRefPicList, riRefIdx)->getViewId (); 
    Int iColViewId     = pColCU->getSlice()->getViewId(); 
    Int iColRefViewId  = pColCU->getSlice()->getRefPic( eColRefPicList, pColCU->getCUMvField(eColRefPicList)->getRefIdx(uiAbsPartAddr))->getViewId(); 
    iScale = 4096;
    if ( iCurrRefViewId != iCurrViewId && iColViewId != iColRefViewId )
    {
      iScale = xGetDistScaleFactor( iCurrViewId, iCurrRefViewId, iColViewId, iColRefViewId );
    }
    if ( bMRG && iScale != 4096 && m_pcSlice->getIvMvScalingFlag( ) ) 
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

Void TComDataCU::xDeriveCenterIdx( UInt uiPartIdx, UInt& ruiPartIdxCenter )
{
  UInt uiPartAddr;
  Int  iPartWidth;
  Int  iPartHeight;
  getPartIndexAndSize( uiPartIdx, uiPartAddr, iPartWidth, iPartHeight);

  ruiPartIdxCenter = m_absZIdxInCtu+uiPartAddr; // partition origin.
  ruiPartIdxCenter = g_auiRasterToZscan[ g_auiZscanToRaster[ ruiPartIdxCenter ]
                                        + ( iPartHeight/m_pcPic->getMinCUHeight()  )/2*m_pcPic->getNumPartInCtuWidth()
                                        + ( iPartWidth/m_pcPic->getMinCUWidth()  )/2];
}

#if NH_3D
Void TComDataCU::compressMV(Int scale)
{
   Int scaleFactor = (4 / scale ) * AMVP_DECIMATION_FACTOR / m_unitSize;
#else
Void TComDataCU::compressMV()
{
  Int scaleFactor = 4 * AMVP_DECIMATION_FACTOR / m_unitSize;
#endif
  if (scaleFactor > 0)
  {
    for(UInt i=0; i<NUM_REF_PIC_LIST_01; i++)
    {
      m_acCUMvField[i].compress(m_pePredMode, scaleFactor);
    }
  }
}

UInt TComDataCU::getCoefScanIdx(const UInt uiAbsPartIdx, const UInt uiWidth, const UInt uiHeight, const ComponentID compID) const
{
  //------------------------------------------------

  //this mechanism is available for intra only

  if (!isIntra(uiAbsPartIdx))
  {
    return SCAN_DIAG;
  }

  //------------------------------------------------

  //check that MDCS can be used for this TU

  const ChromaFormat format = getPic()->getChromaFormat();

  const UInt maximumWidth  = MDCS_MAXIMUM_WIDTH  >> getComponentScaleX(compID, format);
  const UInt maximumHeight = MDCS_MAXIMUM_HEIGHT >> getComponentScaleY(compID, format);

  if ((uiWidth > maximumWidth) || (uiHeight > maximumHeight))
  {
    return SCAN_DIAG;
  }

  //------------------------------------------------

  //otherwise, select the appropriate mode

  UInt uiDirMode  = getIntraDir(toChannelType(compID), uiAbsPartIdx);

#if H_3D_DIM
    mapDepthModeToIntraDir( uiDirMode );
#endif

  if (uiDirMode==DM_CHROMA_IDX)
  {
    const TComSPS *sps=getSlice()->getSPS();
    const UInt partsPerMinCU = 1<<(2*(sps->getMaxTotalCUDepth() - sps->getLog2DiffMaxMinCodingBlockSize()));
    uiDirMode = getIntraDir(CHANNEL_TYPE_LUMA, getChromasCorrespondingPULumaIdx(uiAbsPartIdx, getPic()->getChromaFormat(), partsPerMinCU));
#if H_3D_DIM
      mapDepthModeToIntraDir( uiDirMode );
#endif
  }

  if (isChroma(compID) && (format == CHROMA_422))
  {
    uiDirMode = g_chroma422IntraAngleMappingTable[uiDirMode];
  }

  //------------------

  if      (abs((Int)uiDirMode - VER_IDX) <= MDCS_ANGLE_LIMIT)
  {
    return SCAN_HOR;
  }
  else if (abs((Int)uiDirMode - HOR_IDX) <= MDCS_ANGLE_LIMIT)
  {
    return SCAN_VER;
  }
  else
  {
    return SCAN_DIAG;
  }
}

#if NH_3D_VSO
Void TComDataCU::getPosInPic( UInt uiAbsPartIndex, Int& riPosX, Int& riPosY ) const
{
  riPosX = g_auiRasterToPelX[g_auiZscanToRaster[uiAbsPartIndex]] + getCUPelX();
  riPosY = g_auiRasterToPelY[g_auiZscanToRaster[uiAbsPartIndex]] + getCUPelY();  
}
#endif

#if NH_3D_IV_MERGE
Void TComDataCU::getDispforDepth (UInt uiPartIdx, UInt uiPartAddr, DisInfo* pDisp)
{
  assert(getPartitionSize( uiPartAddr ) == SIZE_2Nx2N);

  TComMv cMv; 
  if ( getSlice()->getDefaultRefViewIdxAvailableFlag() )
  {
    Int iViewIdx = getSlice()->getDefaultRefViewIdx();
    pDisp->m_aVIdxCan = iViewIdx;
    Int iDisp     = getSlice()->getDepthToDisparityB( iViewIdx )[ (Int64) (1 << ( getSlice()->getSPS()->getBitDepth(CHANNEL_TYPE_LUMA) - 1 )) ];

    cMv.setHor(iDisp);
    cMv.setVer(0);
    pDisp->m_acNBDV = cMv;
    pDisp->m_aVIdxCan = iViewIdx;
  }
}
#endif

#if H_3D
Bool TComDataCU::getNeighDepth ( UInt uiPartIdx, UInt uiPartAddr, Pel* pNeighDepth, Int index )
{
  UInt  uiPartIdxLT, uiPartIdxRT;
  this->deriveLeftRightTopIdxAdi( uiPartIdxLT, uiPartIdxRT, 0, 0 );
  UInt uiMidPart, uiPartNeighbor;  
  TComDataCU* pcCUNeighbor;
  Bool bDepAvail = false;
  Pel *pDepth  = this->getPic()->getPicYuvRec()->getLumaAddr();
  Int iDepStride =  this->getPic()->getPicYuvRec()->getStride();

  Int xP, yP, nPSW, nPSH;
  this->getPartPosition( uiPartIdx, xP, yP, nPSW, nPSH );

  switch( index )
  {
  case 0: // Mid Left
    uiMidPart = g_auiZscanToRaster[uiPartIdxLT] + (nPSH>>1) / this->getPic()->getMinCUHeight() * this->getPic()->getNumPartInWidth();
    pcCUNeighbor = this->getPULeft( uiPartNeighbor, g_auiRasterToZscan[uiMidPart] );
    if ( pcCUNeighbor )
    {
      if( !this->getSlice()->getPPS()->getConstrainedIntraPred() )
      {
        *pNeighDepth = pDepth[ (yP+(nPSH>>1)) * iDepStride + (xP-1) ];
        bDepAvail = true;
      }
      else if ( pcCUNeighbor->getPredictionMode( uiPartNeighbor ) == MODE_INTRA )
      {
        *pNeighDepth = pDepth[ (yP+(nPSH>>1)) * iDepStride + (xP-1) ];
        bDepAvail = true;
      }
    }
    break;
  case 1: // Mid Above
    uiMidPart = g_auiZscanToRaster[uiPartIdxLT] + (nPSW>>1) / this->getPic()->getMinCUWidth();
    pcCUNeighbor = this->getPUAbove( uiPartNeighbor, g_auiRasterToZscan[uiMidPart] );
    if( pcCUNeighbor )
    {
      if( !this->getSlice()->getPPS()->getConstrainedIntraPred() )
      {
        *pNeighDepth = pDepth[ (yP-1) * iDepStride + (xP + (nPSW>>1)) ];
        bDepAvail = true;
      }
      else if ( pcCUNeighbor->getPredictionMode( uiPartNeighbor ) == MODE_INTRA )
      {
        *pNeighDepth = pDepth[ (yP-1) * iDepStride + (xP + (nPSW>>1)) ];
        bDepAvail = true;
      }
    }
    break;
  default:
    break;
  }

  return bDepAvail;
}
#endif
#if NH_3D_NBDV 
//Notes from QC:
//TBD#1: DoNBDV related contributions are just partially integrated under the marco of NH_3D_NBDV_REF, remove this comment once DoNBDV and BVSP are done
//TBD#2: set of DvMCP values need to be done as part of inter-view motion prediction process. Remove this comment once merge related integration is done
//To be checked: Parallel Merge features for NBDV, related to DV_DERIVATION_PARALLEL_B0096 and LGE_IVMP_PARALLEL_MERGE_B0136 are not integrated. The need of these features due to the adoption of CU-based NBDV is not clear. We need confirmation on this, especially by proponents
Void TComDataCU::getDisMvpCandNBDV( DisInfo* pDInfo
#if NH_3D_NBDV_REF
, Bool bDepthRefine
#endif
)
{
  //// ******* Init variables ******* /////
  // Init disparity struct for results
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
#if NH_3D_NBDV_REF
  if( !m_pcSlice->getDepthRefinementFlag( ) )
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
    Int  uiLCUIdx = getCtuRsAddr();
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
        pDInfo->m_acNBDV = cColMv;
        pDInfo->m_aVIdxCan  = iTargetViewIdx;

#if NH_3D_NBDV_REF
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
#endif
        if (picDepth && bDepthRefine)
        {
          estimateDVFromDM(iTargetViewIdx, uiPartIdx, picDepth, uiPartAddr, &cColMv );
        }
        pDInfo->m_acDoNBDV  = cColMv;
#endif //NH_3D_NBDV_REF
        return;
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
#if NH_3D_NBDV_REF
    , bDepthRefine 
#endif
    ) )
    return;

  //// ******* Get disparity from above block ******* /////
  pcTmpCU = getPUAbove(uiIdx, uiPartIdxRT, true, false, true);
  if(pcTmpCU != NULL )
  {
    bCheckMcpDv = ( ( getCtuRsAddr() - pcTmpCU->getCtuRsAddr() ) == 0);
    if ( xCheckSpatialNBDV( pcTmpCU, uiIdx, pDInfo, bCheckMcpDv, &cIDVInfo, DVFROM_ABOVE
#if NH_3D_NBDV_REF
      , bDepthRefine 
#endif
      ) )
      return;
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
          pDInfo->m_acNBDV = cDispVec;
          pDInfo->m_aVIdxCan = cIDVInfo.m_aVIdxCan[iList][ curPos ];
#if NH_3D_NBDV_REF
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
#endif

          if (picDepth && bDepthRefine)
          {
            estimateDVFromDM (pDInfo->m_aVIdxCan, uiPartIdx, picDepth, uiPartAddr, &cDispVec);
          }
          pDInfo->m_acDoNBDV = cDispVec;
#endif
          return;
        }
      }
    }
  }

  TComMv defaultDV(0, 0);
  pDInfo->m_acNBDV = defaultDV;

  if (getSlice()->getDefaultRefViewIdxAvailableFlag())
  {
    pDInfo->m_aVIdxCan = getSlice()->getDefaultRefViewIdx();

#if NH_3D_NBDV_REF
    TComPic* picDepth = NULL;
#if H_3D_FCO_VSP_DONBDV_E0163
    picDepth  = getSlice()->getIvPic(true, getSlice()->getViewIndex() );
    if ( picDepth->getPicYuvRec() != NULL )  
    {
      defaultDV.setZero();
    }
    else // Go back with virtual depth
    {
      picDepth = getSlice()->getIvPic( true, getSlice()->getDefaultRefViewIdx());
    }

    assert(picDepth != NULL);
#else
    picDepth = getSlice()->getIvPic( true, getSlice()->getDefaultRefViewIdx());
#endif
    if (picDepth && bDepthRefine)
    {
      estimateDVFromDM(getSlice()->getDefaultRefViewIdx(), uiPartIdx, picDepth, uiPartAddr, &defaultDV ); // from base view
    }
    pDInfo->m_acDoNBDV = defaultDV;
#endif
  }
}

#if NH_3D_NBDV_REF
Pel TComDataCU::getMcpFromDM(TComPicYuv* pcBaseViewDepthPicYuv, TComMv* mv, Int iBlkX, Int iBlkY, Int iBlkWidth, Int iBlkHeight, Int* aiShiftLUT )
{
  Int iPictureWidth  = pcBaseViewDepthPicYuv->getWidth(COMPONENT_Y);
  Int iPictureHeight = pcBaseViewDepthPicYuv->getHeight(COMPONENT_Y);

  Int depthStartPosX = Clip3(0,   iPictureWidth - 1,  iBlkX + ((mv->getHor()+2)>>2));
  Int depthStartPosY = Clip3(0,   iPictureHeight - 1, iBlkY + ((mv->getVer()+2)>>2));
  Int depthEndPosX   = Clip3(0,   iPictureWidth - 1,  iBlkX + iBlkWidth - 1 + ((mv->getHor()+2)>>2));
  Int depthEndPosY   = Clip3(0,   iPictureHeight - 1, iBlkY + iBlkHeight - 1 + ((mv->getVer()+2)>>2));

  Pel* depthTL  = pcBaseViewDepthPicYuv->getAddr(COMPONENT_Y);
  Int depStride =  pcBaseViewDepthPicYuv->getStride(COMPONENT_Y);

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
    UInt uiAbsPartAddrCurrCU = m_absZIdxInCtu + uiPartAddr;
    Int iWidth, iHeight;
    getPartIndexAndSize( uiPartIdx, uiPartAddr, iWidth, iHeight ); // The modified value of uiPartAddr won't be used any more

    TComPicYuv* pcBaseViewDepthPicYuv = picDepth->getPicYuvRec();
    const TComSPS   &sps =*(getSlice()->getSPS());
    Int iBlkX = ( getCtuRsAddr() % picDepth->getFrameWidthInCtus() ) * sps.getMaxCUWidth()  + g_auiRasterToPelX[ g_auiZscanToRaster[ uiAbsPartAddrCurrCU ] ];
    Int iBlkY = ( getCtuRsAddr() / picDepth->getFrameWidthInCtus() ) * sps.getMaxCUHeight() + g_auiRasterToPelY[ g_auiZscanToRaster[ uiAbsPartAddrCurrCU ] ];

    Int* aiShiftLUT = getSlice()->getDepthToDisparityB(refViewIdx );

    Pel iDisp = getMcpFromDM( pcBaseViewDepthPicYuv, cMvPred, iBlkX, iBlkY, iWidth, iHeight, aiShiftLUT );
    cMvPred->setHor( iDisp );
  }
}
#endif //NH_3D_NBDV_REF


Bool TComDataCU::xCheckSpatialNBDV( TComDataCU* pcTmpCU, UInt uiIdx, DisInfo* pNbDvInfo, Bool bSearchForMvpDv, IDVInfo* paIDVInfo, UInt uiMvpDvPos
#if NH_3D_NBDV_REF
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
          pNbDvInfo->m_acNBDV = cMvPred;
          pNbDvInfo->m_aVIdxCan = refViewIdx;
#if NH_3D_NBDV_REF
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
#endif
          UInt uiPartIdx = 0;   //Notes from MTK: Please confirm that using 0 as partition index and partition address is correct for CU-level DoNBDV
          UInt uiPartAddr = 0;  //QC: confirmed

          if (picDepth && bDepthRefine)
          {
            estimateDVFromDM(refViewIdx, uiPartIdx, picDepth, uiPartAddr, &cMvPred );
          }
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
  UInt uiNumPartInCUWidth = m_pcPic->getNumPartInCtuWidth();  
  Int uiLCUIdx = getCtuRsAddr();

  UInt uiPartIdxRB;
  deriveRightBottomIdx(uiPartIdx, uiPartIdxRB );  
  UInt uiAbsPartIdxTmp = g_auiZscanToRaster[uiPartIdxRB];

  if (( m_pcPic->getCtu(m_ctuRsAddr)->getCUPelX() + g_auiRasterToPelX[uiAbsPartIdxTmp] + m_pcPic->getMinCUWidth() )>= m_pcSlice->getSPS()->getPicWidthInLumaSamples() )
  {
    riLCUIdxRBNb  = -1;
    riPartIdxRBNb = -1;
  }
  else if(( m_pcPic->getCtu(m_ctuRsAddr)->getCUPelY() + g_auiRasterToPelY[uiAbsPartIdxTmp] + m_pcPic->getMinCUHeight() )>= m_pcSlice->getSPS()->getPicHeightInLumaSamples() )
  {
    riLCUIdxRBNb  = -1;
    riPartIdxRBNb = -1;
  }
  else
  {
    if ( ( uiAbsPartIdxTmp % uiNumPartInCUWidth < uiNumPartInCUWidth - 1 ) &&           // is not at the last column of LCU 
      ( uiAbsPartIdxTmp / uiNumPartInCUWidth < m_pcPic->getNumPartInCtuHeight() - 1 ) ) // is not at the last row    of LCU
    {
      riPartIdxRBNb = g_auiRasterToZscan[ uiAbsPartIdxTmp + uiNumPartInCUWidth + 1 ];
      riLCUIdxRBNb  = uiLCUIdx; 
    }
    else if ( uiAbsPartIdxTmp % uiNumPartInCUWidth < uiNumPartInCUWidth - 1 )           // is not at the last column of LCU But is last row of LCU
    {
      riPartIdxRBNb = -1;
      riLCUIdxRBNb  = -1;
    }
    else if ( uiAbsPartIdxTmp / uiNumPartInCUWidth < m_pcPic->getNumPartInCtuHeight() - 1 ) // is not at the last row of LCU But is last column of LCU
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
#if NH_3D_VSP // bug fix
  UInt uiCurrPartNumb = m_pcPic->getNumPartitionsInCtu() >> (uiDepth << 1);
#else
  UInt uiCurrPartNumb = m_pcPic->getNumPartInCtuWidth() >> (uiDepth << 1);
#endif
  for (UInt ui = 0; ui < uiCurrPartNumb; ui++ )
  {
    m_pDvInfo[uiAbsPartIdx + ui] = cDvInfo;
  }
}
#if NH_3D_VSP
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
  TComDataCU *pColCU = pColPic->getCtu( uiCUAddr );
  iColViewIdx = pColCU->getSlice()->getViewIndex();
  if (pColCU->getPic()==0||pColCU->getPartitionSize(uiPartUnitIdx)==NUMBER_OF_PART_SIZES||pColCU->isIntra(uiPartUnitIdx))
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
#if NH_3D_NBDV_REF
  depthRefineFlag = m_pcSlice->getDepthRefinementFlag( ); 
#endif // NH_3D_NBDV_REF

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

#if NH_3D_SPIVMP
Void TComDataCU::getSPPara(Int iPUWidth, Int iPUHeight, Int& iNumSP, Int& iNumSPInOneLine, Int& iSPWidth, Int& iSPHeight)
{
  Int iSubPUSize = ( getSlice()->getIsDepth() ? getSlice()->getMpiSubPbSize() : getSlice()->getSubPbSize() );

  iNumSPInOneLine = iPUWidth/iSubPUSize;
  Int iNumSPInOneColumn = iPUHeight/iSubPUSize;
  iNumSPInOneLine = (iPUHeight % iSubPUSize != 0 || iPUWidth % iSubPUSize != 0 ) ? 1 : iNumSPInOneLine;
  iNumSPInOneColumn = (iPUHeight % iSubPUSize != 0  || iPUWidth % iSubPUSize != 0 ) ? 1 : iNumSPInOneColumn;
  iNumSP = iNumSPInOneLine * iNumSPInOneColumn;

  iSPWidth = iNumSPInOneLine == 1 ? iPUWidth: iSubPUSize; 
  iSPHeight = iNumSPInOneColumn == 1 ? iPUHeight: iSubPUSize; 
}

Void TComDataCU::getSPAbsPartIdx(UInt uiBaseAbsPartIdx, Int iWidth, Int iHeight, Int iPartIdx, Int iNumPartLine, UInt& ruiPartAddr )
{
  uiBaseAbsPartIdx += m_absZIdxInCtu;
  Int iBasePelX = g_auiRasterToPelX[g_auiZscanToRaster[uiBaseAbsPartIdx]];
  Int iBasePelY = g_auiRasterToPelY[g_auiZscanToRaster[uiBaseAbsPartIdx]];
  Int iCurrPelX = iBasePelX + iPartIdx%iNumPartLine * iWidth;
  Int iCurrPelY = iBasePelY + iPartIdx/iNumPartLine * iHeight;
  Int iCurrRaster = iCurrPelY / getPic()->getMinCUHeight() * getPic()->getNumPartInCtuWidth() + iCurrPelX/getPic()->getMinCUWidth();
  ruiPartAddr = g_auiRasterToZscan[iCurrRaster];
  ruiPartAddr -= m_absZIdxInCtu;  
}

Void TComDataCU::setInterDirSP( UInt uiDir, UInt uiAbsPartIdx, Int iWidth, Int iHeight )
{
  uiAbsPartIdx += getZorderIdxInCtu();
  Int iStartPelX = g_auiRasterToPelX[g_auiZscanToRaster[uiAbsPartIdx]];
  Int iStartPelY = g_auiRasterToPelY[g_auiZscanToRaster[uiAbsPartIdx]];
  Int iEndPelX = iStartPelX + iWidth;
  Int iEndPelY = iStartPelY + iHeight;

  Int iCurrRaster, uiPartAddr;

  for (Int i=iStartPelY; i<iEndPelY; i+=getPic()->getMinCUHeight())
  {
    for (Int j=iStartPelX; j < iEndPelX; j += getPic()->getMinCUWidth())
    {
      iCurrRaster = i / getPic()->getMinCUHeight() * getPic()->getNumPartInCtuWidth() + j/getPic()->getMinCUWidth();
      uiPartAddr = g_auiRasterToZscan[iCurrRaster];
      uiPartAddr -= getZorderIdxInCtu();  

      m_puhInterDir[uiPartAddr] = uiDir;
    }
  }
}
#endif

#if NH_3D_IV_MERGE
Bool
TComDataCU::getInterViewMergeCands(UInt uiPartIdx, Int* paiPdmRefIdx, TComMv* pacPdmMv, DisInfo* pDInfo, Int* availableMcDc , Bool bIsDepth           
#if NH_3D_SPIVMP
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
  pcBaseRec->getTopLeftSamplePos( getCtuRsAddr(), getZorderIdxInCtu() + uiPartAddr, iCurrPosX, iCurrPosY );

#if !NH_3D_SPIVMP
  iCurrPosX  += ( iWidth  >> 1 );
  iCurrPosY  += ( iHeight >> 1 );
#endif

  Bool depthRefineFlag = false; 
#if NH_3D_NBDV_REF
  depthRefineFlag = m_pcSlice->getDepthRefinementFlag( ); 
#endif // NH_3D_NBDV_REF

  TComMv      cDv = depthRefineFlag ? pDInfo->m_acDoNBDV : pDInfo->m_acNBDV; 
  if( depthRefineFlag )
  {
    cDv.setVer(0);
  }

  Bool abPdmAvailable[8] =  {false, false, false, false, false, false, false, false};
#if NH_3D_NBDV
  for( Int i = 0; i < 8; i++)
  {
    pacPdmMv[i].setIDVFlag   (false);
  }
#endif

  if(!bICFlag)
  {

#if NH_3D_SPIVMP
    ////////////////////////////////
    //////////sub-PU IvMC///////////
    ////////////////////////////////
    if(!m_pcSlice->getIsDepth())
    {
#if H_3D_DBBP
      if (!getDBBPFlag(0))
#else
      if (1)
#endif
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

        Int iRefCenterPosX   = Clip3( 0, pcBaseRec->getWidth (COMPONENT_Y) - 1, iCenterPosX + ( (cDv.getHor() + 2 ) >> 2 ) );
        Int iRefCenterPosY   = Clip3( 0, pcBaseRec->getHeight(COMPONENT_Y) - 1, iCenterPosY + ( (cDv.getVer() + 2 ) >> 2 ) ); 

        pcBaseRec->getCUAddrAndPartIdx( iRefCenterPosX , iRefCenterPosY , iRefCenterCUAddr, iRefCenterAbsPartIdx );
        TComDataCU* pcDefaultCU    = pcBasePic->getCtu( iRefCenterCUAddr );
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
#if NH_3D_NBDV 
#if NH_3D_IV_MERGE
                      if( !bIsDepth )
                      {
#endif
                        cMv.setIDVFlag   (true);
                        cMv.setIDVHor    (cDv.getHor());                  
                        cMv.setIDVVer    (cDv.getVer());  
                        cMv.setIDVVId    (iViewIndex); 
#if NH_3D_IV_MERGE
                      }
#endif
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
              iBasePosX   = Clip3( 0, pcBaseRec->getWidth (COMPONENT_Y) - 1, j + iDelX + ( (cDv.getHor() + 2 ) >> 2 ));
              iBasePosY   = Clip3( 0, pcBaseRec->getHeight(COMPONENT_Y) - 1, i + iDelY + ( (cDv.getVer() + 2 ) >> 2 )); 

              pcBaseRec->getCUAddrAndPartIdx( iBasePosX , iBasePosY, iBaseCUAddr, iBaseAbsPartIdx );
              pcBaseCU    = pcBasePic->getCtu( iBaseCUAddr );
              if(!( pcBaseCU->getPredictionMode( iBaseAbsPartIdx ) == MODE_INTRA ))
              {
                for( UInt uiCurrRefListId = 0; uiCurrRefListId < 2; uiCurrRefListId++ )
                {
                  RefPicList  eCurrRefPicList = RefPicList( uiCurrRefListId );
                  Bool bLoopStop = false;
                  for(Int iLoop = 0; iLoop < 2 && !bLoopStop; ++iLoop)
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

                            bLoopStop = true;

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

#if NH_3D_SPIVMP
    if(m_pcSlice->getIsDepth())
    {
      iCurrPosX  += ( iWidth  >> 1 );
      iCurrPosY  += ( iHeight >> 1 );
    }
#if H_3D_DBBP
    for(Int iLoopCan = ( (m_pcSlice->getIsDepth() || getDBBPFlag(0)) ? 0 : 1 ); iLoopCan < ( 2 - m_pcSlice->getIsDepth() ); iLoopCan ++) 
#else
    for(Int iLoopCan = ( m_pcSlice->getIsDepth() ? 0 : 1 ); iLoopCan < ( 2 - m_pcSlice->getIsDepth() ); iLoopCan ++) 
#endif
#else
    for(Int iLoopCan = 0; iLoopCan < 2; iLoopCan ++)
#endif
    {
      // iLoopCan == 0 --> IvMC
      // iLoopCan == 1 --> IvMCShift 

      Int         iBaseCUAddr;
      Int         iBaseAbsPartIdx;

      Int offsetW = (iLoopCan == 0) ? 0 : ( iWidth  * 2);
      Int offsetH = (iLoopCan == 0) ? 0 : ( iHeight * 2);

      Int         iBasePosX   = Clip3( 0, pcBaseRec->getWidth (COMPONENT_Y) - 1, iCurrPosX + ( (cDv.getHor() + offsetW + 2 ) >> 2 ) );
      Int         iBasePosY   = Clip3( 0, pcBaseRec->getHeight(COMPONENT_Y) - 1, iCurrPosY + ( (cDv.getVer() + offsetH + 2 ) >> 2 ) ); 
      pcBaseRec->getCUAddrAndPartIdx( iBasePosX , iBasePosY , iBaseCUAddr, iBaseAbsPartIdx );

      TComDataCU* pcBaseCU    = pcBasePic->getCtu( iBaseCUAddr );
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
#if NH_3D_NBDV 
#if NH_3D_IV_MERGE
                    if( !bIsDepth )
                    {
#endif
                      cMv.setIDVFlag   (true);
                      cMv.setIDVHor    (cDv.getHor());                  
                      cMv.setIDVVer    (cDv.getVer());  
                      cMv.setIDVVId    (iViewIndex); 
#if NH_3D_IV_MERGE
                    }
#endif
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
#if NH_3D_SPIVMP
#if H_3D_DBBP
    for(Int iLoopCan = ( (m_pcSlice->getIsDepth() || getDBBPFlag(0)) ? 0 : 1 ); iLoopCan < ( 2 - m_pcSlice->getIsDepth() ); iLoopCan ++)
#else
    for(Int iLoopCan = ( m_pcSlice->getIsDepth()  ? 0 : 1 ); iLoopCan < ( 2 - m_pcSlice->getIsDepth() ); iLoopCan ++)
#endif
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
#if NH_3D_NBDV_REF
            TComMv cMv = depthRefineFlag ? pDInfo->m_acDoNBDV : pDInfo->m_acNBDV; 
#endif
            cMv.setHor( cMv.getHor() + ioffsetDV );
#if NH_3D_IV_MERGE 
            if( bIsDepth )
            {
              cMv.setHor((cMv.getHor()+2)>>2); 
            }
#endif
            cMv.setVer( 0 );
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
  for( UInt ui = 0; ui < uiCurrPartNumb; ui++ )
  { 
    m_dmmWedgeTabIdx[dmmType][uiAbsPartIdx+ui] = tabIdx; 
  }
}
#endif

#if NH_3D_VSP
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
  Int numPartsLine    = cu->getPic()->getNumPartInCtuWidth();

  Int nTxtPerMvInfoX = 4; // cu->getPic()->getMinCUWidth();
  Int nTxtPerMvInfoY = 4; // cu->getPic()->getMinCUHeight();

  Int refDepStride = picRefDepth->getStride( COMPONENT_Y );

  TComMv tmpMv(0, 0);
  tmpMv.setIDVFlag(false);

  Int refDepOffset  = ( (dv->getHor()+2) >> 2 ) + ( (dv->getVer()+2) >> 2 ) * refDepStride;
  Pel *refDepth     = picRefDepth->getAddr( COMPONENT_Y, cu->getCtuRsAddr(), cu->getZorderIdxInCtu() + partAddr ) + refDepOffset;

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
