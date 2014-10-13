/*  The copyright in this software is being made available under the BSD
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

/** \file     TComSlice.cpp
    \brief    slice header and SPS class
*/

#include "CommonDef.h"
#include "TComSlice.h"
#include "TComPic.h"
#include "TLibEncoder/TEncSbac.h"
#include "TLibDecoder/TDecSbac.h"

//! \ingroup TLibCommon
//! \{


TComSlice::TComSlice()
: m_iPPSId                        ( -1 )
#if H_MV
, m_slicePicOrderCntLsb           ( 0 )
#endif
, m_iPOC                          ( 0 )
, m_iLastIDR                      ( 0 )
, m_eNalUnitType                  ( NAL_UNIT_CODED_SLICE_IDR_W_RADL )
, m_eSliceType                    ( I_SLICE )
, m_iSliceQp                      ( 0 )
, m_dependentSliceSegmentFlag            ( false )
#if ADAPTIVE_QP_SELECTION
, m_iSliceQpBase                  ( 0 )
#endif
, m_deblockingFilterDisable        ( false )
, m_deblockingFilterOverrideFlag   ( false )
, m_deblockingFilterBetaOffsetDiv2 ( 0 )
, m_deblockingFilterTcOffsetDiv2   ( 0 )
, m_bCheckLDC                     ( false )
, m_iSliceQpDelta                 ( 0 )
, m_iSliceQpDeltaCb               ( 0 )
, m_iSliceQpDeltaCr               ( 0 )
, m_iDepth                        ( 0 )
, m_bRefenced                     ( false )
, m_pcSPS                         ( NULL )
, m_pcPPS                         ( NULL )
, m_pcPic                         ( NULL )
, m_colFromL0Flag                 ( 1 )
#if SETTING_NO_OUT_PIC_PRIOR
, m_noOutputPriorPicsFlag         ( false )
, m_noRaslOutputFlag              ( false )
, m_handleCraAsBlaFlag              ( false )
#endif
, m_colRefIdx                     ( 0 )
, m_uiTLayer                      ( 0 )
, m_bTLayerSwitchingFlag          ( false )
, m_sliceMode                   ( 0 )
, m_sliceArgument               ( 0 )
, m_sliceCurStartCUAddr         ( 0 )
, m_sliceCurEndCUAddr           ( 0 )
, m_sliceIdx                    ( 0 )
, m_sliceSegmentMode            ( 0 )
, m_sliceSegmentArgument        ( 0 )
, m_sliceSegmentCurStartCUAddr  ( 0 )
, m_sliceSegmentCurEndCUAddr    ( 0 )
, m_nextSlice                    ( false )
, m_nextSliceSegment             ( false )
, m_sliceBits                   ( 0 )
, m_sliceSegmentBits         ( 0 )
, m_bFinalized                    ( false )
, m_uiTileOffstForMultES          ( 0 )
, m_puiSubstreamSizes             ( NULL )
, m_cabacInitFlag                 ( false )
, m_bLMvdL1Zero                   ( false )
, m_numEntryPointOffsets          ( 0 )
, m_temporalLayerNonReferenceFlag ( false )
, m_enableTMVPFlag                ( true )
#if I0044_SLICE_TMVP
, m_availableForTMVPRefFlag       ( true )
#endif
#if H_MV
, m_refPicSetInterLayer0           ( NULL )
, m_refPicSetInterLayer1           ( NULL )
, m_layerId                       (0)
, m_viewId                        (0)
, m_viewIndex                     (0)
#if !H_MV_HLS7_GEN
, m_pocResetFlag                  (false)
#endif
#if H_MV
, m_crossLayerBlaFlag             (false)
#endif
, m_discardableFlag               (false)
, m_interLayerPredEnabledFlag     (false)
, m_numInterLayerRefPicsMinus1    (0)
#if H_MV
, m_sliceSegmentHeaderExtensionLength (0)
, m_pocResetIdc                       (0)
, m_pocResetPeriodId                  (0)
, m_fullPocResetFlag                  (false)
, m_pocLsbVal                         (0)
, m_pocMsbValPresentFlag              (false)
, m_pocMsbVal                         (0)
, m_pocMsbValRequiredFlag         ( false )
#endif
#endif
{
  m_aiNumRefIdx[0] = m_aiNumRefIdx[1] = 0;
  
  initEqualRef();
  
  for (Int component = 0; component < 3; component++)
  {
    m_lambdas[component] = 0.0;
  }
  
  for ( Int idx = 0; idx < MAX_NUM_REF; idx++ )
  {
    m_list1IdxToList0Idx[idx] = -1;
  }
  for(Int iNumCount = 0; iNumCount < MAX_NUM_REF; iNumCount++)
  {
    m_apcRefPicList [0][iNumCount] = NULL;
    m_apcRefPicList [1][iNumCount] = NULL;
    m_aiRefPOCList  [0][iNumCount] = 0;
    m_aiRefPOCList  [1][iNumCount] = 0;
#if H_MV
    m_aiRefLayerIdList[0][iNumCount] = 0;
    m_aiRefLayerIdList[1][iNumCount] = 0;
#endif
  }
  resetWpScaling();
  initWpAcDcParam();
  m_saoEnabledFlag = false;
  m_saoEnabledFlagChroma = false;
#if H_MV
  for (Int i = 0; i < MAX_NUM_LAYERS; i++ )
  {
    m_interLayerPredLayerIdc[ i ] = -1;
  }
#endif
}

TComSlice::~TComSlice()
{
  delete[] m_puiSubstreamSizes;
  m_puiSubstreamSizes = NULL;
}


Void TComSlice::initSlice()
{
  m_aiNumRefIdx[0]      = 0;
  m_aiNumRefIdx[1]      = 0;
  
  m_colFromL0Flag = 1;
  
  m_colRefIdx = 0;
  initEqualRef();
  m_bCheckLDC = false;
  m_iSliceQpDeltaCb = 0;
  m_iSliceQpDeltaCr = 0;

  m_maxNumMergeCand = MRG_MAX_NUM_CANDS;

  m_bFinalized=false;

  m_tileByteLocation.clear();
  m_cabacInitFlag        = false;
  m_numEntryPointOffsets = 0;
  m_enableTMVPFlag = true;
}

Bool TComSlice::getRapPicFlag()
{
  return getNalUnitType() == NAL_UNIT_CODED_SLICE_IDR_W_RADL
      || getNalUnitType() == NAL_UNIT_CODED_SLICE_IDR_N_LP
      || getNalUnitType() == NAL_UNIT_CODED_SLICE_BLA_N_LP
      || getNalUnitType() == NAL_UNIT_CODED_SLICE_BLA_W_RADL
      || getNalUnitType() == NAL_UNIT_CODED_SLICE_BLA_W_LP
      || getNalUnitType() == NAL_UNIT_CODED_SLICE_CRA;
}

/**
 - allocate table to contain substream sizes to be written to the slice header.
 .
 \param uiNumSubstreams Number of substreams -- the allocation will be this value - 1.
 */
Void  TComSlice::allocSubstreamSizes(UInt uiNumSubstreams)
{
  delete[] m_puiSubstreamSizes;
  m_puiSubstreamSizes = new UInt[uiNumSubstreams > 0 ? uiNumSubstreams-1 : 0];
}

Void  TComSlice::sortPicList        (TComList<TComPic*>& rcListPic)
{
  TComPic*    pcPicExtract;
  TComPic*    pcPicInsert;
  
  TComList<TComPic*>::iterator    iterPicExtract;
  TComList<TComPic*>::iterator    iterPicExtract_1;
  TComList<TComPic*>::iterator    iterPicInsert;
  
  for (Int i = 1; i < (Int)(rcListPic.size()); i++)
  {
    iterPicExtract = rcListPic.begin();
    for (Int j = 0; j < i; j++) iterPicExtract++;
    pcPicExtract = *(iterPicExtract);
    pcPicExtract->setCurrSliceIdx(0);
    
    iterPicInsert = rcListPic.begin();
    while (iterPicInsert != iterPicExtract)
    {
      pcPicInsert = *(iterPicInsert);
      pcPicInsert->setCurrSliceIdx(0);
      if (pcPicInsert->getPOC() >= pcPicExtract->getPOC())
      {
        break;
      }
      
      iterPicInsert++;
    }
    
    iterPicExtract_1 = iterPicExtract;    iterPicExtract_1++;
    
    //  swap iterPicExtract and iterPicInsert, iterPicExtract = curr. / iterPicInsert = insertion position
    rcListPic.insert (iterPicInsert, iterPicExtract, iterPicExtract_1);
    rcListPic.erase  (iterPicExtract);
  }
}

TComPic* TComSlice::xGetRefPic (TComList<TComPic*>& rcListPic,
                                Int                 poc)
{
  TComList<TComPic*>::iterator  iterPic = rcListPic.begin();  
  TComPic*                      pcPic = *(iterPic);
  while ( iterPic != rcListPic.end() )
  {
    if(pcPic->getPOC() == poc)
    {
      break;
    }
    iterPic++;
    pcPic = *(iterPic);
  }
  return  pcPic;
}


TComPic* TComSlice::xGetLongTermRefPic(TComList<TComPic*>& rcListPic, Int poc, Bool pocHasMsb)
{
  TComList<TComPic*>::iterator  iterPic = rcListPic.begin();  
  TComPic*                      pcPic = *(iterPic);
  TComPic*                      pcStPic = pcPic;
  
  Int pocCycle = 1 << getSPS()->getBitsForPOC();
  if (!pocHasMsb)
  {
    poc = poc & (pocCycle - 1);
  }
  
  while ( iterPic != rcListPic.end() )
  {
    pcPic = *(iterPic);
    if (pcPic && pcPic->getPOC()!=this->getPOC() && pcPic->getSlice( 0 )->isReferenced())
    {
      Int picPoc = pcPic->getPOC();
      if (!pocHasMsb)
      {
        picPoc = picPoc & (pocCycle - 1);
      }
      
      if (poc == picPoc)
    {
      if(pcPic->getIsLongTerm())
      {
        return pcPic;
      }
      else
      {
        pcStPic = pcPic;
      }
      break;
    }
    }

    iterPic++;
  }
  
  return  pcStPic;
}

Void TComSlice::setRefPOCList       ()
{
  for (Int iDir = 0; iDir < 2; iDir++)
  {
    for (Int iNumRefIdx = 0; iNumRefIdx < m_aiNumRefIdx[iDir]; iNumRefIdx++)
    {
      m_aiRefPOCList[iDir][iNumRefIdx] = m_apcRefPicList[iDir][iNumRefIdx]->getPOC();
#if H_MV
      m_aiRefLayerIdList[iDir][iNumRefIdx] = m_apcRefPicList[iDir][iNumRefIdx]->getLayerId();
#endif
    }
  }

}

Void TComSlice::setList1IdxToList0Idx()
{
  Int idxL0, idxL1;
  for ( idxL1 = 0; idxL1 < getNumRefIdx( REF_PIC_LIST_1 ); idxL1++ )
  {
    m_list1IdxToList0Idx[idxL1] = -1;
    for ( idxL0 = 0; idxL0 < getNumRefIdx( REF_PIC_LIST_0 ); idxL0++ )
    {
      if ( m_apcRefPicList[REF_PIC_LIST_0][idxL0]->getPOC() == m_apcRefPicList[REF_PIC_LIST_1][idxL1]->getPOC() )
      {
        m_list1IdxToList0Idx[idxL1] = idxL0;
        break;
      }
    }
  }
}

#if !H_MV
Void TComSlice::setRefPicList( TComList<TComPic*>& rcListPic, Bool checkNumPocTotalCurr )
{
  if (!checkNumPocTotalCurr)
  {
    if (m_eSliceType == I_SLICE)
    {
      ::memset( m_apcRefPicList, 0, sizeof (m_apcRefPicList));
      ::memset( m_aiNumRefIdx,   0, sizeof ( m_aiNumRefIdx ));

      return;
    }

    m_aiNumRefIdx[0] = getNumRefIdx(REF_PIC_LIST_0);
    m_aiNumRefIdx[1] = getNumRefIdx(REF_PIC_LIST_1);
  }

  TComPic*  pcRefPic= NULL;
  TComPic*  RefPicSetStCurr0[16];
  TComPic*  RefPicSetStCurr1[16];
  TComPic*  RefPicSetLtCurr[16];
  UInt NumPocStCurr0 = 0;
  UInt NumPocStCurr1 = 0;
  UInt NumPocLtCurr = 0;
  Int i;

  for(i=0; i < m_pcRPS->getNumberOfNegativePictures(); i++)
  {
    if(m_pcRPS->getUsed(i))
    {
      pcRefPic = xGetRefPic(rcListPic, getPOC()+m_pcRPS->getDeltaPOC(i));
      pcRefPic->setIsLongTerm(0);
      pcRefPic->getPicYuvRec()->extendPicBorder();
      RefPicSetStCurr0[NumPocStCurr0] = pcRefPic;
      NumPocStCurr0++;
      pcRefPic->setCheckLTMSBPresent(false);  
    }
  }

  for(; i < m_pcRPS->getNumberOfNegativePictures()+m_pcRPS->getNumberOfPositivePictures(); i++)
  {
    if(m_pcRPS->getUsed(i))
    {
      pcRefPic = xGetRefPic(rcListPic, getPOC()+m_pcRPS->getDeltaPOC(i));
      pcRefPic->setIsLongTerm(0);
      pcRefPic->getPicYuvRec()->extendPicBorder();
      RefPicSetStCurr1[NumPocStCurr1] = pcRefPic;
      NumPocStCurr1++;
      pcRefPic->setCheckLTMSBPresent(false);  
    }
  }

  for(i = m_pcRPS->getNumberOfNegativePictures()+m_pcRPS->getNumberOfPositivePictures()+m_pcRPS->getNumberOfLongtermPictures()-1; i > m_pcRPS->getNumberOfNegativePictures()+m_pcRPS->getNumberOfPositivePictures()-1 ; i--)
  {
    if(m_pcRPS->getUsed(i))
    {
      pcRefPic = xGetLongTermRefPic(rcListPic, m_pcRPS->getPOC(i), m_pcRPS->getCheckLTMSBPresent(i));
      pcRefPic->setIsLongTerm(1);
      pcRefPic->getPicYuvRec()->extendPicBorder();
      RefPicSetLtCurr[NumPocLtCurr] = pcRefPic;
      NumPocLtCurr++;
    }
    if(pcRefPic==NULL) 
    {
      pcRefPic = xGetLongTermRefPic(rcListPic, m_pcRPS->getPOC(i), m_pcRPS->getCheckLTMSBPresent(i));
    }
    pcRefPic->setCheckLTMSBPresent(m_pcRPS->getCheckLTMSBPresent(i));  
  }

  // ref_pic_list_init
  TComPic*  rpsCurrList0[MAX_NUM_REF+1];
  TComPic*  rpsCurrList1[MAX_NUM_REF+1];
  Int numPocTotalCurr = NumPocStCurr0 + NumPocStCurr1 + NumPocLtCurr;
  if (checkNumPocTotalCurr)
  {
    // The variable NumPocTotalCurr is derived as specified in subclause 7.4.7.2. It is a requirement of bitstream conformance that the following applies to the value of NumPocTotalCurr:
    // - If the current picture is a BLA or CRA picture, the value of NumPocTotalCurr shall be equal to 0.
    // - Otherwise, when the current picture contains a P or B slice, the value of NumPocTotalCurr shall not be equal to 0.
    if (getRapPicFlag())
    {
      assert(numPocTotalCurr == 0);
    }

    if (m_eSliceType == I_SLICE)
    {
      ::memset( m_apcRefPicList, 0, sizeof (m_apcRefPicList));
      ::memset( m_aiNumRefIdx,   0, sizeof ( m_aiNumRefIdx ));

      return;
    }

    assert(numPocTotalCurr > 0);

    m_aiNumRefIdx[0] = getNumRefIdx(REF_PIC_LIST_0);
    m_aiNumRefIdx[1] = getNumRefIdx(REF_PIC_LIST_1);
  }

  Int cIdx = 0;
  for ( i=0; i<NumPocStCurr0; i++, cIdx++)
  {
    rpsCurrList0[cIdx] = RefPicSetStCurr0[i];
  }
  for ( i=0; i<NumPocStCurr1; i++, cIdx++)
  {
    rpsCurrList0[cIdx] = RefPicSetStCurr1[i];
  }
  for ( i=0; i<NumPocLtCurr;  i++, cIdx++)
  {
    rpsCurrList0[cIdx] = RefPicSetLtCurr[i];
  }
  assert(cIdx == numPocTotalCurr);

  if (m_eSliceType==B_SLICE)
  {
    cIdx = 0;
    for ( i=0; i<NumPocStCurr1; i++, cIdx++)
    {
      rpsCurrList1[cIdx] = RefPicSetStCurr1[i];
    }
    for ( i=0; i<NumPocStCurr0; i++, cIdx++)
    {
      rpsCurrList1[cIdx] = RefPicSetStCurr0[i];
    }
    for ( i=0; i<NumPocLtCurr;  i++, cIdx++)
    {
      rpsCurrList1[cIdx] = RefPicSetLtCurr[i];
    }
    assert(cIdx == numPocTotalCurr);
  }

  ::memset(m_bIsUsedAsLongTerm, 0, sizeof(m_bIsUsedAsLongTerm));

  for (Int rIdx = 0; rIdx < m_aiNumRefIdx[0]; rIdx ++)
  {
    cIdx = m_RefPicListModification.getRefPicListModificationFlagL0() ? m_RefPicListModification.getRefPicSetIdxL0(rIdx) : rIdx % numPocTotalCurr;
    assert(cIdx >= 0 && cIdx < numPocTotalCurr);
    m_apcRefPicList[0][rIdx] = rpsCurrList0[ cIdx ];
    m_bIsUsedAsLongTerm[0][rIdx] = ( cIdx >= NumPocStCurr0 + NumPocStCurr1 );
  }
  if ( m_eSliceType != B_SLICE )
  {
    m_aiNumRefIdx[1] = 0;
    ::memset( m_apcRefPicList[1], 0, sizeof(m_apcRefPicList[1]));
  }
  else
  {
    for (Int rIdx = 0; rIdx < m_aiNumRefIdx[1]; rIdx ++)
    {
      cIdx = m_RefPicListModification.getRefPicListModificationFlagL1() ? m_RefPicListModification.getRefPicSetIdxL1(rIdx) : rIdx % numPocTotalCurr;
      assert(cIdx >= 0 && cIdx < numPocTotalCurr);
      m_apcRefPicList[1][rIdx] = rpsCurrList1[ cIdx ];
      m_bIsUsedAsLongTerm[1][rIdx] = ( cIdx >= NumPocStCurr0 + NumPocStCurr1 );
    }
  }
}

#else
Void TComSlice::getTempRefPicLists( TComList<TComPic*>& rcListPic, std::vector<TComPic*>& refPicSetInterLayer0, std::vector<TComPic*>& refPicSetInterLayer1,                                     
                                   std::vector<TComPic*> rpsCurrList[2], std::vector<Bool> usedAsLongTerm[2], Int& numPocTotalCurr, Bool checkNumPocTotalCurr )
{
  if (!checkNumPocTotalCurr)
  {
    if (m_eSliceType == I_SLICE)
    {      
      return;
    }    
  }

  TComPic*  pcRefPic= NULL;
  TComPic*  RefPicSetStCurr0[16];
  TComPic*  RefPicSetStCurr1[16];
  TComPic*  RefPicSetLtCurr[16];
  UInt NumPocStCurr0 = 0;
  UInt NumPocStCurr1 = 0;
  UInt NumPocLtCurr = 0;
  Int i;

  for(i=0; i < m_pcRPS->getNumberOfNegativePictures(); i++)
  {
    if(m_pcRPS->getUsed(i))
    {
      pcRefPic = xGetRefPic(rcListPic, getPOC()+m_pcRPS->getDeltaPOC(i));
      pcRefPic->setIsLongTerm(0);
      pcRefPic->getPicYuvRec()->extendPicBorder();
      RefPicSetStCurr0[NumPocStCurr0] = pcRefPic;
      NumPocStCurr0++;
      pcRefPic->setCheckLTMSBPresent(false);  
    }
  }
  
  for(; i < m_pcRPS->getNumberOfNegativePictures()+m_pcRPS->getNumberOfPositivePictures(); i++)
  {
    if(m_pcRPS->getUsed(i))
    {
      pcRefPic = xGetRefPic(rcListPic, getPOC()+m_pcRPS->getDeltaPOC(i));
      pcRefPic->setIsLongTerm(0);
      pcRefPic->getPicYuvRec()->extendPicBorder();
      RefPicSetStCurr1[NumPocStCurr1] = pcRefPic;
      NumPocStCurr1++;
      pcRefPic->setCheckLTMSBPresent(false);  
    }
  }
  
  for(i = m_pcRPS->getNumberOfNegativePictures()+m_pcRPS->getNumberOfPositivePictures()+m_pcRPS->getNumberOfLongtermPictures()-1; i > m_pcRPS->getNumberOfNegativePictures()+m_pcRPS->getNumberOfPositivePictures()-1 ; i--)
  {
    if(m_pcRPS->getUsed(i))
    {
      pcRefPic = xGetLongTermRefPic(rcListPic, m_pcRPS->getPOC(i), m_pcRPS->getCheckLTMSBPresent(i));
      pcRefPic->setIsLongTerm(1);
      pcRefPic->getPicYuvRec()->extendPicBorder();
      RefPicSetLtCurr[NumPocLtCurr] = pcRefPic;
      NumPocLtCurr++;
    }
    if(pcRefPic==NULL) 
    {
      pcRefPic = xGetLongTermRefPic(rcListPic, m_pcRPS->getPOC(i), m_pcRPS->getCheckLTMSBPresent(i));
    }
    pcRefPic->setCheckLTMSBPresent(m_pcRPS->getCheckLTMSBPresent(i));  
  }

  Int numPocInterCurr = NumPocStCurr0 + NumPocStCurr1 + NumPocLtCurr; 
  numPocTotalCurr = numPocInterCurr + getNumActiveRefLayerPics( );
  assert( numPocTotalCurr == getNumRpsCurrTempList() );

  if (checkNumPocTotalCurr)
  {
    // The variable NumPocTotalCurr is derived as specified in subclause 7.4.7.2. It is a requirement of bitstream conformance that the following applies to the value of NumPocTotalCurr:
    // - If nuh_layer_id is equal to 0 and the current picture is a BLA picture or a CRA picture, the value of NumPocTotalCurr shall be equal to 0.
    // - Otherwise, when the current picture contains a P or B slice, the value of NumPocTotalCurr shall not be equal to 0.
    if ( getRapPicFlag() && m_layerId == 0 )
    {
      assert(numPocTotalCurr == 0);
    }

    if (m_eSliceType == I_SLICE)
    {
      ::memset( m_apcRefPicList, 0, sizeof (m_apcRefPicList));
      ::memset( m_aiNumRefIdx,   0, sizeof ( m_aiNumRefIdx ));
      
      return;
    }
    
    assert(numPocTotalCurr > 0);
    
    m_aiNumRefIdx[0] = getNumRefIdx(REF_PIC_LIST_0);
    m_aiNumRefIdx[1] = getNumRefIdx(REF_PIC_LIST_1);
  }

  std::vector<TComPic*>* refPicSetInterLayer[2] = { &refPicSetInterLayer0, &refPicSetInterLayer1}; 
  Int numPocInterLayer[2] = { getNumActiveRefLayerPics0( ), getNumActiveRefLayerPics1( ) }; 
  
  TComPic**             refPicSetStCurr    [2] = { RefPicSetStCurr0, RefPicSetStCurr1 };
  Int numPocStCurr[2] = { NumPocStCurr0, NumPocStCurr1 }; 

  for (Int li = 0; li < ((m_eSliceType==B_SLICE) ? 2 : 1); li++)
  { 
    rpsCurrList   [li].resize(MAX_NUM_REF+1,NULL ); 
    usedAsLongTerm[li].resize(MAX_NUM_REF+1,false); 

    Int cIdx = 0;
    for ( i=0; i < numPocStCurr[li]; i++, cIdx++)
    {
      rpsCurrList[li][cIdx] = refPicSetStCurr[li][i];
      usedAsLongTerm [li][cIdx] = false;  
    }

    for ( i=0; i < numPocInterLayer[li];  i++, cIdx++)
    {    
      rpsCurrList[li][cIdx] = (*refPicSetInterLayer[li])[i];
      usedAsLongTerm [li][cIdx] = true;  
    }

    for ( i=0; i < numPocStCurr[1-li]; i++, cIdx++)
    {
      rpsCurrList[li][cIdx] = refPicSetStCurr[1-li][i];
      usedAsLongTerm [li][cIdx] = false;  
    }

    for ( i=0; i<NumPocLtCurr;  i++, cIdx++)
    {
      rpsCurrList[li][cIdx] = RefPicSetLtCurr[i];
      usedAsLongTerm [li][cIdx] = true;  
    }

    for ( i=0; i < numPocInterLayer[1-li];  i++, cIdx++)
    {    
      assert( cIdx < MAX_NUM_REF );    
      rpsCurrList[li][cIdx] = (*refPicSetInterLayer[1-li])[i];
      usedAsLongTerm [li][cIdx] = true;  
    }

    assert(cIdx == numPocTotalCurr);
  }
}

Void TComSlice::setRefPicList( std::vector<TComPic*> rpsCurrList[2], std::vector<Bool> usedAsLongTerm[2], Int numPocTotalCurr, Bool checkNumPocTotalCurr )

{
  if (!checkNumPocTotalCurr)
  {
    if (m_eSliceType == I_SLICE)
    {
      ::memset( m_apcRefPicList, 0, sizeof (m_apcRefPicList));
      ::memset( m_aiNumRefIdx,   0, sizeof ( m_aiNumRefIdx ));

      return;
    }    
  }

  ::memset(m_bIsUsedAsLongTerm, 0, sizeof(m_bIsUsedAsLongTerm));

  for (Int li = 0; li < 2; li++)
  {
    if ( m_eSliceType == P_SLICE && li == 1 )
    {
      m_aiNumRefIdx[1] = 0;
      ::memset( m_apcRefPicList[1], 0, sizeof(m_apcRefPicList[1]));
    } 
    else
    {
      for (Int rIdx = 0; rIdx <= (m_aiNumRefIdx[ li ] - 1 ); rIdx ++)
      { 
        Bool listModified             =                m_RefPicListModification.getRefPicListModificationFlagL( li ); 
        Int orgIdx                    = listModified ? m_RefPicListModification.getRefPicSetIdxL(li, rIdx) : (rIdx % numPocTotalCurr); 

        assert( rpsCurrList[li][ orgIdx ] != NULL ); 
#if DISCARDABLE_PIC_RPS
        assert( rpsCurrList[li][ orgIdx ]->getSlice(0)->getDiscardableFlag() == 0 );    // Inter-layer RPS shall not contain picture with discardable_flag = 1.
#endif
        m_apcRefPicList    [li][rIdx] = rpsCurrList    [li][ orgIdx ];
        m_bIsUsedAsLongTerm[li][rIdx] = usedAsLongTerm [li][ orgIdx ] ; 
      }
    }
  }
}
#endif
Int TComSlice::getNumRpsCurrTempList()
{
  Int numRpsCurrTempList = 0;

  if (m_eSliceType == I_SLICE) 
  {
    return 0;
  }
  for(UInt i=0; i < m_pcRPS->getNumberOfNegativePictures()+ m_pcRPS->getNumberOfPositivePictures() + m_pcRPS->getNumberOfLongtermPictures(); i++)
  {
    if(m_pcRPS->getUsed(i))
    {
      numRpsCurrTempList++;
    }
  }
#if H_MV
  numRpsCurrTempList = numRpsCurrTempList + getNumActiveRefLayerPics();
#endif
  return numRpsCurrTempList;
}

Void TComSlice::initEqualRef()
{
  for (Int iDir = 0; iDir < 2; iDir++)
  {
    for (Int iRefIdx1 = 0; iRefIdx1 < MAX_NUM_REF; iRefIdx1++)
    {
      for (Int iRefIdx2 = iRefIdx1; iRefIdx2 < MAX_NUM_REF; iRefIdx2++)
      {
        m_abEqualRef[iDir][iRefIdx1][iRefIdx2] = m_abEqualRef[iDir][iRefIdx2][iRefIdx1] = (iRefIdx1 == iRefIdx2? true : false);
      }
    }
  }
}

Void TComSlice::checkColRefIdx(UInt curSliceIdx, TComPic* pic)
{
  Int i;
  TComSlice* curSlice = pic->getSlice(curSliceIdx);
  Int currColRefPOC =  curSlice->getRefPOC( RefPicList(1-curSlice->getColFromL0Flag()), curSlice->getColRefIdx());
  TComSlice* preSlice;
  Int preColRefPOC;
  for(i=curSliceIdx-1; i>=0; i--)
  {
    preSlice = pic->getSlice(i);
    if(preSlice->getSliceType() != I_SLICE)
    {
      preColRefPOC  = preSlice->getRefPOC( RefPicList(1-preSlice->getColFromL0Flag()), preSlice->getColRefIdx());
      if(currColRefPOC != preColRefPOC)
      {
        printf("Collocated_ref_idx shall always be the same for all slices of a coded picture!\n");
        exit(EXIT_FAILURE);
      }
      else
      {
        break;
      }
    }
  }
}

Void TComSlice::checkCRA(TComReferencePictureSet *pReferencePictureSet, Int& pocCRA, NalUnitType& associatedIRAPType, TComList<TComPic *>& rcListPic)
{
  for(Int i = 0; i < pReferencePictureSet->getNumberOfNegativePictures()+pReferencePictureSet->getNumberOfPositivePictures(); i++)
  {
    if(pocCRA < MAX_UINT && getPOC() > pocCRA)
    {
      assert(getPOC()+pReferencePictureSet->getDeltaPOC(i) >= pocCRA);
    }
  }
  for(Int i = pReferencePictureSet->getNumberOfNegativePictures()+pReferencePictureSet->getNumberOfPositivePictures(); i < pReferencePictureSet->getNumberOfPictures(); i++)
  {
    if(pocCRA < MAX_UINT && getPOC() > pocCRA)
    {
      if (!pReferencePictureSet->getCheckLTMSBPresent(i))
      {
        assert(xGetLongTermRefPic(rcListPic, pReferencePictureSet->getPOC(i), false)->getPOC() >= pocCRA);
      }
      else
      {
      assert(pReferencePictureSet->getPOC(i) >= pocCRA);
    }
  }
  }
  if ( getNalUnitType() == NAL_UNIT_CODED_SLICE_IDR_W_RADL || getNalUnitType() == NAL_UNIT_CODED_SLICE_IDR_N_LP ) // IDR picture found
  {
    pocCRA = getPOC();
    associatedIRAPType = getNalUnitType();
  }
  else if ( getNalUnitType() == NAL_UNIT_CODED_SLICE_CRA ) // CRA picture found
  {
    pocCRA = getPOC();
    associatedIRAPType = getNalUnitType();
  }
  else if ( getNalUnitType() == NAL_UNIT_CODED_SLICE_BLA_W_LP
         || getNalUnitType() == NAL_UNIT_CODED_SLICE_BLA_W_RADL
         || getNalUnitType() == NAL_UNIT_CODED_SLICE_BLA_N_LP ) // BLA picture found
  {
    pocCRA = getPOC();
    associatedIRAPType = getNalUnitType();
  }
}

/** Function for marking the reference pictures when an IDR/CRA/CRANT/BLA/BLANT is encountered.
 * \param pocCRA POC of the CRA/CRANT/BLA/BLANT picture
 * \param bRefreshPending flag indicating if a deferred decoding refresh is pending
 * \param rcListPic reference to the reference picture list
 * This function marks the reference pictures as "unused for reference" in the following conditions.
 * If the nal_unit_type is IDR/BLA/BLANT, all pictures in the reference picture list  
 * are marked as "unused for reference"
 *    If the nal_unit_type is BLA/BLANT, set the pocCRA to the temporal reference of the current picture.
 * Otherwise
 *    If the bRefreshPending flag is true (a deferred decoding refresh is pending) and the current 
 *    temporal reference is greater than the temporal reference of the latest CRA/CRANT/BLA/BLANT picture (pocCRA), 
 *    mark all reference pictures except the latest CRA/CRANT/BLA/BLANT picture as "unused for reference" and set 
 *    the bRefreshPending flag to false.
 *    If the nal_unit_type is CRA/CRANT, set the bRefreshPending flag to true and pocCRA to the temporal 
 *    reference of the current picture.
 * Note that the current picture is already placed in the reference list and its marking is not changed.
 * If the current picture has a nal_ref_idc that is not 0, it will remain marked as "used for reference".
 */
Void TComSlice::decodingRefreshMarking(Int& pocCRA, Bool& bRefreshPending, TComList<TComPic*>& rcListPic)
{
  TComPic*                 rpcPic;
#if !FIX1172
  setAssociatedIRAPPOC(pocCRA);
#endif
  Int pocCurr = getPOC(); 

  if ( getNalUnitType() == NAL_UNIT_CODED_SLICE_BLA_W_LP
    || getNalUnitType() == NAL_UNIT_CODED_SLICE_BLA_W_RADL
    || getNalUnitType() == NAL_UNIT_CODED_SLICE_BLA_N_LP
    || getNalUnitType() == NAL_UNIT_CODED_SLICE_IDR_W_RADL
    || getNalUnitType() == NAL_UNIT_CODED_SLICE_IDR_N_LP )  // IDR or BLA picture
  {
    // mark all pictures as not used for reference
    TComList<TComPic*>::iterator        iterPic       = rcListPic.begin();
    while (iterPic != rcListPic.end())
    {
      rpcPic = *(iterPic);
      rpcPic->setCurrSliceIdx(0);
      if (rpcPic->getPOC() != pocCurr) rpcPic->getSlice(0)->setReferenced(false);
      iterPic++;
    }
    if ( getNalUnitType() == NAL_UNIT_CODED_SLICE_BLA_W_LP
      || getNalUnitType() == NAL_UNIT_CODED_SLICE_BLA_W_RADL
      || getNalUnitType() == NAL_UNIT_CODED_SLICE_BLA_N_LP )
    {
      pocCRA = pocCurr;
    }
#if EFFICIENT_FIELD_IRAP
    bRefreshPending = true;
#endif
  }
  else // CRA or No DR
  {
#if EFFICIENT_FIELD_IRAP
    if(getAssociatedIRAPType() == NAL_UNIT_CODED_SLICE_IDR_N_LP || getAssociatedIRAPType() == NAL_UNIT_CODED_SLICE_IDR_W_RADL)
    {
      if (bRefreshPending==true && pocCurr > m_iLastIDR) // IDR reference marking pending 
      {
        TComList<TComPic*>::iterator        iterPic       = rcListPic.begin();
        while (iterPic != rcListPic.end())
        {
          rpcPic = *(iterPic);
          if (rpcPic->getPOC() != pocCurr && rpcPic->getPOC() != m_iLastIDR)
          {
            rpcPic->getSlice(0)->setReferenced(false);
          }
          iterPic++;
        }
        bRefreshPending = false; 
      }
    }
    else
    {
#endif
    if (bRefreshPending==true && pocCurr > pocCRA) // CRA reference marking pending 
    {
      TComList<TComPic*>::iterator        iterPic       = rcListPic.begin();
      while (iterPic != rcListPic.end())
      {
        rpcPic = *(iterPic);
        if (rpcPic->getPOC() != pocCurr && rpcPic->getPOC() != pocCRA)
        {
          rpcPic->getSlice(0)->setReferenced(false);
        }
        iterPic++;
      }
      bRefreshPending = false; 
    }
#if EFFICIENT_FIELD_IRAP
    }
#endif
    if ( getNalUnitType() == NAL_UNIT_CODED_SLICE_CRA ) // CRA picture found
    {
      bRefreshPending = true; 
      pocCRA = pocCurr;
    }
  }
}

Void TComSlice::copySliceInfo(TComSlice *pSrc)
{
  assert( pSrc != NULL );

  Int i, j, k;

  m_iPOC                 = pSrc->m_iPOC;
  m_eNalUnitType         = pSrc->m_eNalUnitType;
#if H_MV
  m_layerId              = pSrc->m_layerId;
  // GT: Copying of several other values might be be missing here, or is above not necessary? 
#endif
  m_eSliceType           = pSrc->m_eSliceType;
  m_iSliceQp             = pSrc->m_iSliceQp;
#if ADAPTIVE_QP_SELECTION
  m_iSliceQpBase         = pSrc->m_iSliceQpBase;
#endif
  m_deblockingFilterDisable   = pSrc->m_deblockingFilterDisable;
  m_deblockingFilterOverrideFlag = pSrc->m_deblockingFilterOverrideFlag;
  m_deblockingFilterBetaOffsetDiv2 = pSrc->m_deblockingFilterBetaOffsetDiv2;
  m_deblockingFilterTcOffsetDiv2 = pSrc->m_deblockingFilterTcOffsetDiv2;
  
  for (i = 0; i < 2; i++)
  {
    m_aiNumRefIdx[i]     = pSrc->m_aiNumRefIdx[i];
  }

  for (i = 0; i < MAX_NUM_REF; i++)
  {
    m_list1IdxToList0Idx[i] = pSrc->m_list1IdxToList0Idx[i];
  } 
  m_bCheckLDC             = pSrc->m_bCheckLDC;
  m_iSliceQpDelta        = pSrc->m_iSliceQpDelta;
  m_iSliceQpDeltaCb      = pSrc->m_iSliceQpDeltaCb;
  m_iSliceQpDeltaCr      = pSrc->m_iSliceQpDeltaCr;
  for (i = 0; i < 2; i++)
  {
    for (j = 0; j < MAX_NUM_REF; j++)
    {
      m_apcRefPicList[i][j]  = pSrc->m_apcRefPicList[i][j];
      m_aiRefPOCList[i][j]   = pSrc->m_aiRefPOCList[i][j];
#if H_MV
      m_aiRefLayerIdList[i][j] = pSrc->m_aiRefLayerIdList[i][j];
#endif
    }
  }
  for (i = 0; i < 2; i++)
  {
    for (j = 0; j < MAX_NUM_REF + 1; j++)
    {
      m_bIsUsedAsLongTerm[i][j] = pSrc->m_bIsUsedAsLongTerm[i][j];
    }
  }
  m_iDepth               = pSrc->m_iDepth;

  // referenced slice
  m_bRefenced            = pSrc->m_bRefenced;

  // access channel
#if H_MV
  m_pcVPS                = pSrc->m_pcVPS;
#endif
  m_pcSPS                = pSrc->m_pcSPS;
  m_pcPPS                = pSrc->m_pcPPS;
  m_pcRPS                = pSrc->m_pcRPS;
  m_iLastIDR             = pSrc->m_iLastIDR;

  m_pcPic                = pSrc->m_pcPic;

  m_colFromL0Flag        = pSrc->m_colFromL0Flag;
  m_colRefIdx            = pSrc->m_colRefIdx;
  setLambdas(pSrc->getLambdas());
  for (i = 0; i < 2; i++)
  {
    for (j = 0; j < MAX_NUM_REF; j++)
    {
      for (k =0; k < MAX_NUM_REF; k++)
      {
        m_abEqualRef[i][j][k] = pSrc->m_abEqualRef[i][j][k];
      }
    }
  }

  m_uiTLayer                      = pSrc->m_uiTLayer;
  m_bTLayerSwitchingFlag          = pSrc->m_bTLayerSwitchingFlag;

  m_sliceMode                   = pSrc->m_sliceMode;
  m_sliceArgument               = pSrc->m_sliceArgument;
  m_sliceCurStartCUAddr         = pSrc->m_sliceCurStartCUAddr;
  m_sliceCurEndCUAddr           = pSrc->m_sliceCurEndCUAddr;
  m_sliceIdx                    = pSrc->m_sliceIdx;
  m_sliceSegmentMode            = pSrc->m_sliceSegmentMode;
  m_sliceSegmentArgument        = pSrc->m_sliceSegmentArgument; 
  m_sliceSegmentCurStartCUAddr  = pSrc->m_sliceSegmentCurStartCUAddr;
  m_sliceSegmentCurEndCUAddr    = pSrc->m_sliceSegmentCurEndCUAddr;
  m_nextSlice                    = pSrc->m_nextSlice;
  m_nextSliceSegment             = pSrc->m_nextSliceSegment;
  for ( Int e=0 ; e<2 ; e++ )
  {
    for ( Int n=0 ; n<MAX_NUM_REF ; n++ )
    {
      memcpy(m_weightPredTable[e][n], pSrc->m_weightPredTable[e][n], sizeof(wpScalingParam)*3 );
    }
  }
  m_saoEnabledFlag = pSrc->m_saoEnabledFlag; 
  m_saoEnabledFlagChroma = pSrc->m_saoEnabledFlagChroma;
  m_cabacInitFlag                = pSrc->m_cabacInitFlag;
  m_numEntryPointOffsets  = pSrc->m_numEntryPointOffsets;

  m_bLMvdL1Zero = pSrc->m_bLMvdL1Zero;
  m_LFCrossSliceBoundaryFlag = pSrc->m_LFCrossSliceBoundaryFlag;
  m_enableTMVPFlag                = pSrc->m_enableTMVPFlag;
  m_maxNumMergeCand               = pSrc->m_maxNumMergeCand;

#if H_MV
  // Additional slice header syntax elements 
#if !H_MV_HLS7_GEN
  m_pocResetFlag               = pSrc->m_pocResetFlag; 
#endif
  m_discardableFlag            = pSrc->m_discardableFlag; 
  m_interLayerPredEnabledFlag  = pSrc->m_interLayerPredEnabledFlag; 
  m_numInterLayerRefPicsMinus1 = pSrc->m_numInterLayerRefPicsMinus1;

  for (Int layer = 0; layer < MAX_NUM_LAYERS; layer++ )
  {
    m_interLayerPredLayerIdc[ layer ] = pSrc->m_interLayerPredLayerIdc[ layer ]; 
  }
#endif
}

Int TComSlice::m_prevTid0POC = 0;

/** Function for setting the slice's temporal layer ID and corresponding temporal_layer_switching_point_flag.
 * \param uiTLayer Temporal layer ID of the current slice
 * The decoder calls this function to set temporal_layer_switching_point_flag for each temporal layer based on 
 * the SPS's temporal_id_nesting_flag and the parsed PPS.  Then, current slice's temporal layer ID and 
 * temporal_layer_switching_point_flag is set accordingly.
 */
Void TComSlice::setTLayerInfo( UInt uiTLayer )
{
  m_uiTLayer = uiTLayer;
}

/** Function for checking if this is a switching-point
*/
Bool TComSlice::isTemporalLayerSwitchingPoint( TComList<TComPic*>& rcListPic )
{
  TComPic* rpcPic;
  // loop through all pictures in the reference picture buffer
  TComList<TComPic*>::iterator iterPic = rcListPic.begin();
  while ( iterPic != rcListPic.end())
  {
    rpcPic = *(iterPic++);
    if(rpcPic->getSlice(0)->isReferenced() && rpcPic->getPOC() != getPOC())
    {
      if(rpcPic->getTLayer() >= getTLayer())
      {
        return false;
      }
    }
  }
  return true;
}

/** Function for checking if this is a STSA candidate 
 */
Bool TComSlice::isStepwiseTemporalLayerSwitchingPointCandidate( TComList<TComPic*>& rcListPic )
{
    TComPic* rpcPic;
    
    TComList<TComPic*>::iterator iterPic = rcListPic.begin();
    while ( iterPic != rcListPic.end())
    {
        rpcPic = *(iterPic++);
        if(rpcPic->getSlice(0)->isReferenced() &&  (rpcPic->getUsedByCurr()==true) && rpcPic->getPOC() != getPOC())
        {
            if(rpcPic->getTLayer() >= getTLayer())
            {
                return false;
            }
        }
    }
    return true;
}


Void TComSlice::checkLeadingPictureRestrictions(TComList<TComPic*>& rcListPic)
{
  TComPic* rpcPic;

  Int nalUnitType = this->getNalUnitType();

  // When a picture is a leading picture, it shall be a RADL or RASL picture.
  if(this->getAssociatedIRAPPOC() > this->getPOC())
  {
    // Do not check IRAP pictures since they may get a POC lower than their associated IRAP
    if(nalUnitType < NAL_UNIT_CODED_SLICE_BLA_W_LP ||
       nalUnitType > NAL_UNIT_RESERVED_IRAP_VCL23)
    {
      assert(nalUnitType == NAL_UNIT_CODED_SLICE_RASL_N ||
             nalUnitType == NAL_UNIT_CODED_SLICE_RASL_R ||
             nalUnitType == NAL_UNIT_CODED_SLICE_RADL_N ||
             nalUnitType == NAL_UNIT_CODED_SLICE_RADL_R);
    }
  }

  // When a picture is a trailing picture, it shall not be a RADL or RASL picture.
  if(this->getAssociatedIRAPPOC() < this->getPOC())
  {
    assert(nalUnitType != NAL_UNIT_CODED_SLICE_RASL_N &&
           nalUnitType != NAL_UNIT_CODED_SLICE_RASL_R &&
           nalUnitType != NAL_UNIT_CODED_SLICE_RADL_N &&
           nalUnitType != NAL_UNIT_CODED_SLICE_RADL_R);
  }

  // No RASL pictures shall be present in the bitstream that are associated
  // with a BLA picture having nal_unit_type equal to BLA_W_RADL or BLA_N_LP.
  if(nalUnitType == NAL_UNIT_CODED_SLICE_RASL_N ||
     nalUnitType == NAL_UNIT_CODED_SLICE_RASL_R)
  {
    assert(this->getAssociatedIRAPType() != NAL_UNIT_CODED_SLICE_BLA_W_RADL &&
           this->getAssociatedIRAPType() != NAL_UNIT_CODED_SLICE_BLA_N_LP);
  }

  // No RASL pictures shall be present in the bitstream that are associated with
  // an IDR picture.
  if(nalUnitType == NAL_UNIT_CODED_SLICE_RASL_N ||
     nalUnitType == NAL_UNIT_CODED_SLICE_RASL_R)
  {
    assert(this->getAssociatedIRAPType() != NAL_UNIT_CODED_SLICE_IDR_N_LP   &&
           this->getAssociatedIRAPType() != NAL_UNIT_CODED_SLICE_IDR_W_RADL);
  }

  // No RADL pictures shall be present in the bitstream that are associated with
  // a BLA picture having nal_unit_type equal to BLA_N_LP or that are associated
  // with an IDR picture having nal_unit_type equal to IDR_N_LP.
  if(nalUnitType == NAL_UNIT_CODED_SLICE_RADL_N ||
     nalUnitType == NAL_UNIT_CODED_SLICE_RADL_R)
  {
    assert(this->getAssociatedIRAPType() != NAL_UNIT_CODED_SLICE_BLA_N_LP   &&
           this->getAssociatedIRAPType() != NAL_UNIT_CODED_SLICE_IDR_N_LP);
  }

  // loop through all pictures in the reference picture buffer
  TComList<TComPic*>::iterator iterPic = rcListPic.begin();
  while ( iterPic != rcListPic.end())
  {
    rpcPic = *(iterPic++);
#if BUGFIX_INTRAPERIOD
    if(!rpcPic->getReconMark())
    {
      continue;
    }
#endif
    if (rpcPic->getPOC() == this->getPOC())
    {
      continue;
    }

    // Any picture that has PicOutputFlag equal to 1 that precedes an IRAP picture
    // in decoding order shall precede the IRAP picture in output order.
    // (Note that any picture following in output order would be present in the DPB)
#if !SETTING_NO_OUT_PIC_PRIOR
    if(rpcPic->getSlice(0)->getPicOutputFlag() == 1)
#else
    if(rpcPic->getSlice(0)->getPicOutputFlag() == 1 && !this->getNoOutputPriorPicsFlag())
#endif
    {
      if(nalUnitType == NAL_UNIT_CODED_SLICE_BLA_N_LP    ||
         nalUnitType == NAL_UNIT_CODED_SLICE_BLA_W_LP    ||
         nalUnitType == NAL_UNIT_CODED_SLICE_BLA_W_RADL  ||
         nalUnitType == NAL_UNIT_CODED_SLICE_CRA         ||
         nalUnitType == NAL_UNIT_CODED_SLICE_IDR_N_LP    ||
         nalUnitType == NAL_UNIT_CODED_SLICE_IDR_W_RADL)
      {
        assert(rpcPic->getPOC() < this->getPOC());
      }
    }

    // Any picture that has PicOutputFlag equal to 1 that precedes an IRAP picture
    // in decoding order shall precede any RADL picture associated with the IRAP
    // picture in output order.
    if(rpcPic->getSlice(0)->getPicOutputFlag() == 1)
    {
      if((nalUnitType == NAL_UNIT_CODED_SLICE_RADL_N ||
          nalUnitType == NAL_UNIT_CODED_SLICE_RADL_R))
      {
        // rpcPic precedes the IRAP in decoding order
        if(this->getAssociatedIRAPPOC() > rpcPic->getSlice(0)->getAssociatedIRAPPOC())
        {
          // rpcPic must not be the IRAP picture
          if(this->getAssociatedIRAPPOC() != rpcPic->getPOC())
          {
            assert(rpcPic->getPOC() < this->getPOC());
          }
        }
      }
    }

    // When a picture is a leading picture, it shall precede, in decoding order,
    // all trailing pictures that are associated with the same IRAP picture.
    if(nalUnitType == NAL_UNIT_CODED_SLICE_RASL_N ||
       nalUnitType == NAL_UNIT_CODED_SLICE_RASL_R ||
       nalUnitType == NAL_UNIT_CODED_SLICE_RADL_N ||
       nalUnitType == NAL_UNIT_CODED_SLICE_RADL_R)
    {
      if(rpcPic->getSlice(0)->getAssociatedIRAPPOC() == this->getAssociatedIRAPPOC())
      {
        // rpcPic is a picture that preceded the leading in decoding order since it exist in the DPB
        // rpcPic would violate the constraint if it was a trailing picture
        assert(rpcPic->getPOC() <= this->getAssociatedIRAPPOC());
      }
    }

    // Any RASL picture associated with a CRA or BLA picture shall precede any
    // RADL picture associated with the CRA or BLA picture in output order
    if(nalUnitType == NAL_UNIT_CODED_SLICE_RASL_N ||
       nalUnitType == NAL_UNIT_CODED_SLICE_RASL_R)
    { 
      if((this->getAssociatedIRAPType() == NAL_UNIT_CODED_SLICE_BLA_N_LP   ||
          this->getAssociatedIRAPType() == NAL_UNIT_CODED_SLICE_BLA_W_LP   ||
          this->getAssociatedIRAPType() == NAL_UNIT_CODED_SLICE_BLA_W_RADL ||
          this->getAssociatedIRAPType() == NAL_UNIT_CODED_SLICE_CRA)       &&
          this->getAssociatedIRAPPOC() == rpcPic->getSlice(0)->getAssociatedIRAPPOC())
      {
        if(rpcPic->getSlice(0)->getNalUnitType() == NAL_UNIT_CODED_SLICE_RADL_N ||
           rpcPic->getSlice(0)->getNalUnitType() == NAL_UNIT_CODED_SLICE_RADL_R)
        {
          assert(rpcPic->getPOC() > this->getPOC());
        }
      }
    }

    // Any RASL picture associated with a CRA picture shall follow, in output
    // order, any IRAP picture that precedes the CRA picture in decoding order.
    if(nalUnitType == NAL_UNIT_CODED_SLICE_RASL_N ||
       nalUnitType == NAL_UNIT_CODED_SLICE_RASL_R)
    {
      if(this->getAssociatedIRAPType() == NAL_UNIT_CODED_SLICE_CRA)
      {
        if(rpcPic->getSlice(0)->getPOC() < this->getAssociatedIRAPPOC() &&
           (rpcPic->getSlice(0)->getNalUnitType() == NAL_UNIT_CODED_SLICE_BLA_N_LP   ||
            rpcPic->getSlice(0)->getNalUnitType() == NAL_UNIT_CODED_SLICE_BLA_W_LP   ||
            rpcPic->getSlice(0)->getNalUnitType() == NAL_UNIT_CODED_SLICE_BLA_W_RADL ||
            rpcPic->getSlice(0)->getNalUnitType() == NAL_UNIT_CODED_SLICE_IDR_N_LP   ||
            rpcPic->getSlice(0)->getNalUnitType() == NAL_UNIT_CODED_SLICE_IDR_W_RADL ||
            rpcPic->getSlice(0)->getNalUnitType() == NAL_UNIT_CODED_SLICE_CRA))
        {
          assert(this->getPOC() > rpcPic->getSlice(0)->getPOC());
        }
      }
    }
  }
}


/** Function for applying picture marking based on the Reference Picture Set in pReferencePictureSet.
*/
Void TComSlice::applyReferencePictureSet( TComList<TComPic*>& rcListPic, TComReferencePictureSet *pReferencePictureSet)
{
  TComPic* rpcPic;
  Int i, isReference;

  checkLeadingPictureRestrictions(rcListPic);

  // loop through all pictures in the reference picture buffer
  TComList<TComPic*>::iterator iterPic = rcListPic.begin();
  while ( iterPic != rcListPic.end())
  {
    rpcPic = *(iterPic++);

    if(!rpcPic->getSlice( 0 )->isReferenced())
    {
      continue;
    }

    isReference = 0;
    // loop through all pictures in the Reference Picture Set
    // to see if the picture should be kept as reference picture
    for(i=0;i<pReferencePictureSet->getNumberOfPositivePictures()+pReferencePictureSet->getNumberOfNegativePictures();i++)
    {
      if(!rpcPic->getIsLongTerm() && rpcPic->getPicSym()->getSlice(0)->getPOC() == this->getPOC() + pReferencePictureSet->getDeltaPOC(i))
      {
        isReference = 1;
        rpcPic->setUsedByCurr(pReferencePictureSet->getUsed(i));
        rpcPic->setIsLongTerm(0);
      }
    }
    for(;i<pReferencePictureSet->getNumberOfPictures();i++)
    {
      if(pReferencePictureSet->getCheckLTMSBPresent(i)==true)
      {
        if(rpcPic->getIsLongTerm() && (rpcPic->getPicSym()->getSlice(0)->getPOC()) == pReferencePictureSet->getPOC(i))
        {
          isReference = 1;
          rpcPic->setUsedByCurr(pReferencePictureSet->getUsed(i));
        }
      }
      else 
      {
        Int pocCycle = 1<<rpcPic->getPicSym()->getSlice(0)->getSPS()->getBitsForPOC();
        Int curPoc = rpcPic->getPicSym()->getSlice(0)->getPOC() & (pocCycle-1);
        Int refPoc = pReferencePictureSet->getPOC(i) & (pocCycle-1);
        if(rpcPic->getIsLongTerm() && curPoc == refPoc)
        {
          isReference = 1;
          rpcPic->setUsedByCurr(pReferencePictureSet->getUsed(i));
        }
      }

    }
#if H_MV
    if( isReference ) // Current picture is in the temporal RPS
    {
      assert( rpcPic->getSlice(0)->getDiscardableFlag() == 0 ); // Temporal RPS shall not contain picture with discardable_flag equal to 1
    }
#endif
    // mark the picture as "unused for reference" if it is not in
    // the Reference Picture Set
    if(rpcPic->getPicSym()->getSlice(0)->getPOC() != this->getPOC() && isReference == 0)    
    {            
      rpcPic->getSlice( 0 )->setReferenced( false );   
      rpcPic->setUsedByCurr(0);
      rpcPic->setIsLongTerm(0);
    }
    //check that pictures of higher temporal layers are not used
    assert(rpcPic->getSlice( 0 )->isReferenced()==0||rpcPic->getUsedByCurr()==0||rpcPic->getTLayer()<=this->getTLayer());
    //check that pictures of higher or equal temporal layer are not in the RPS if the current picture is a TSA picture
    if(this->getNalUnitType() == NAL_UNIT_CODED_SLICE_TSA_R || this->getNalUnitType() == NAL_UNIT_CODED_SLICE_TSA_N)
    {
      assert(rpcPic->getSlice( 0 )->isReferenced()==0||rpcPic->getTLayer()<this->getTLayer());
    }
    //check that pictures marked as temporal layer non-reference pictures are not used for reference
    if(rpcPic->getPicSym()->getSlice(0)->getPOC() != this->getPOC() && rpcPic->getTLayer()==this->getTLayer())
    {
      assert(rpcPic->getSlice( 0 )->isReferenced()==0||rpcPic->getUsedByCurr()==0||rpcPic->getSlice( 0 )->getTemporalLayerNonReferenceFlag()==false);
    }
  }
}

/** Function for applying picture marking based on the Reference Picture Set in pReferencePictureSet.
*/
#if ALLOW_RECOVERY_POINT_AS_RAP
Int TComSlice::checkThatAllRefPicsAreAvailable( TComList<TComPic*>& rcListPic, TComReferencePictureSet *pReferencePictureSet, Bool printErrors, Int pocRandomAccess, Bool bUseRecoveryPoint)
#else
Int TComSlice::checkThatAllRefPicsAreAvailable( TComList<TComPic*>& rcListPic, TComReferencePictureSet *pReferencePictureSet, Bool printErrors, Int pocRandomAccess)
#endif
{
#if ALLOW_RECOVERY_POINT_AS_RAP
  Int atLeastOneUnabledByRecoveryPoint = 0;
  Int atLeastOneFlushedByPreviousIDR = 0;
#endif
  TComPic* rpcPic;
  Int i, isAvailable;
  Int atLeastOneLost = 0;
  Int atLeastOneRemoved = 0;
  Int iPocLost = 0;

  // loop through all long-term pictures in the Reference Picture Set
  // to see if the picture should be kept as reference picture
  for(i=pReferencePictureSet->getNumberOfNegativePictures()+pReferencePictureSet->getNumberOfPositivePictures();i<pReferencePictureSet->getNumberOfPictures();i++)
  {
    isAvailable = 0;
    // loop through all pictures in the reference picture buffer
    TComList<TComPic*>::iterator iterPic = rcListPic.begin();
    while ( iterPic != rcListPic.end())
    {
      rpcPic = *(iterPic++);
      if(pReferencePictureSet->getCheckLTMSBPresent(i)==true)
      {
        if(rpcPic->getIsLongTerm() && (rpcPic->getPicSym()->getSlice(0)->getPOC()) == pReferencePictureSet->getPOC(i) && rpcPic->getSlice(0)->isReferenced())
        {
#if ALLOW_RECOVERY_POINT_AS_RAP
          if(bUseRecoveryPoint && this->getPOC() > pocRandomAccess && this->getPOC() + pReferencePictureSet->getDeltaPOC(i) < pocRandomAccess)
          {
            isAvailable = 0;
          }
          else
          {
          isAvailable = 1;
        }
#else
          isAvailable = 1;
#endif
        }
      }
      else 
      {
        Int pocCycle = 1<<rpcPic->getPicSym()->getSlice(0)->getSPS()->getBitsForPOC();
        Int curPoc = rpcPic->getPicSym()->getSlice(0)->getPOC() & (pocCycle-1);
        Int refPoc = pReferencePictureSet->getPOC(i) & (pocCycle-1);
        if(rpcPic->getIsLongTerm() && curPoc == refPoc && rpcPic->getSlice(0)->isReferenced())
        {
#if ALLOW_RECOVERY_POINT_AS_RAP
          if(bUseRecoveryPoint && this->getPOC() > pocRandomAccess && this->getPOC() + pReferencePictureSet->getDeltaPOC(i) < pocRandomAccess)
          {
            isAvailable = 0;
          }
          else
          {
          isAvailable = 1;
        }
#else
          isAvailable = 1;
#endif
        }
      }
    }
    // if there was no such long-term check the short terms
    if(!isAvailable)
    {
      iterPic = rcListPic.begin();
      while ( iterPic != rcListPic.end())
      {
        rpcPic = *(iterPic++);

        Int pocCycle = 1 << rpcPic->getPicSym()->getSlice(0)->getSPS()->getBitsForPOC();
        Int curPoc = rpcPic->getPicSym()->getSlice(0)->getPOC();
        Int refPoc = pReferencePictureSet->getPOC(i);
        if (!pReferencePictureSet->getCheckLTMSBPresent(i))
        {
          curPoc = curPoc & (pocCycle - 1);
          refPoc = refPoc & (pocCycle - 1);
        }
        
        if (rpcPic->getSlice(0)->isReferenced() && curPoc == refPoc)
        {
#if ALLOW_RECOVERY_POINT_AS_RAP
          if(bUseRecoveryPoint && this->getPOC() > pocRandomAccess && this->getPOC() + pReferencePictureSet->getDeltaPOC(i) < pocRandomAccess)
          {
            isAvailable = 0;
          }
          else
          {
          isAvailable = 1;
          rpcPic->setIsLongTerm(1);
          break;
        }
#else
          isAvailable = 1;
          rpcPic->setIsLongTerm(1);
          break;
#endif
        }
      }
    }
    // report that a picture is lost if it is in the Reference Picture Set
    // but not available as reference picture
    if(isAvailable == 0)    
    {            
      if (this->getPOC() + pReferencePictureSet->getDeltaPOC(i) >= pocRandomAccess)
      {
        if(!pReferencePictureSet->getUsed(i) )
        {
          if(printErrors)
          {
            printf("\nLong-term reference picture with POC = %3d seems to have been removed or not correctly decoded.", this->getPOC() + pReferencePictureSet->getDeltaPOC(i));
          }
          atLeastOneRemoved = 1;
        }
        else
        {
          if(printErrors)
          {
            printf("\nLong-term reference picture with POC = %3d is lost or not correctly decoded!", this->getPOC() + pReferencePictureSet->getDeltaPOC(i));
          }
          atLeastOneLost = 1;
          iPocLost=this->getPOC() + pReferencePictureSet->getDeltaPOC(i);
        }
      }
#if ALLOW_RECOVERY_POINT_AS_RAP
      else if(bUseRecoveryPoint && this->getPOC() > pocRandomAccess)
      {
        atLeastOneUnabledByRecoveryPoint = 1;
      }
      else if(bUseRecoveryPoint && (this->getAssociatedIRAPType()==NAL_UNIT_CODED_SLICE_IDR_N_LP || this->getAssociatedIRAPType()==NAL_UNIT_CODED_SLICE_IDR_W_RADL))
      {
        atLeastOneFlushedByPreviousIDR = 1;
      }
#endif
    }
  }  
  // loop through all short-term pictures in the Reference Picture Set
  // to see if the picture should be kept as reference picture
  for(i=0;i<pReferencePictureSet->getNumberOfNegativePictures()+pReferencePictureSet->getNumberOfPositivePictures();i++)
  {
    isAvailable = 0;
    // loop through all pictures in the reference picture buffer
    TComList<TComPic*>::iterator iterPic = rcListPic.begin();
    while ( iterPic != rcListPic.end())
    {
      rpcPic = *(iterPic++);

      if(!rpcPic->getIsLongTerm() && rpcPic->getPicSym()->getSlice(0)->getPOC() == this->getPOC() + pReferencePictureSet->getDeltaPOC(i) && rpcPic->getSlice(0)->isReferenced())
      {
#if ALLOW_RECOVERY_POINT_AS_RAP
        if(bUseRecoveryPoint && this->getPOC() > pocRandomAccess && this->getPOC() + pReferencePictureSet->getDeltaPOC(i) < pocRandomAccess)
        {
          isAvailable = 0;
        }
        else
        {
        isAvailable = 1;
      }
#else
        isAvailable = 1;
#endif
      }
    }
    // report that a picture is lost if it is in the Reference Picture Set
    // but not available as reference picture
    if(isAvailable == 0)    
    {            
      if (this->getPOC() + pReferencePictureSet->getDeltaPOC(i) >= pocRandomAccess)
      {
        if(!pReferencePictureSet->getUsed(i) )
        {
          if(printErrors)
          {
            printf("\nShort-term reference picture with POC = %3d seems to have been removed or not correctly decoded.", this->getPOC() + pReferencePictureSet->getDeltaPOC(i));
          }
          atLeastOneRemoved = 1;
        }
        else
        {
          if(printErrors)
          {
            printf("\nShort-term reference picture with POC = %3d is lost or not correctly decoded!", this->getPOC() + pReferencePictureSet->getDeltaPOC(i));
          }
          atLeastOneLost = 1;
          iPocLost=this->getPOC() + pReferencePictureSet->getDeltaPOC(i);
        }
      }
#if ALLOW_RECOVERY_POINT_AS_RAP
      else if(bUseRecoveryPoint && this->getPOC() > pocRandomAccess)
      {
        atLeastOneUnabledByRecoveryPoint = 1;
    }
      else if(bUseRecoveryPoint && (this->getAssociatedIRAPType()==NAL_UNIT_CODED_SLICE_IDR_N_LP || this->getAssociatedIRAPType()==NAL_UNIT_CODED_SLICE_IDR_W_RADL))
      {
        atLeastOneFlushedByPreviousIDR = 1;
  }    
#endif
    }
    }
#if ALLOW_RECOVERY_POINT_AS_RAP
  if(atLeastOneUnabledByRecoveryPoint || atLeastOneFlushedByPreviousIDR)
  {
    return -1;
  }    
#endif
  if(atLeastOneLost)
  {
    return iPocLost+1;
  }
  if(atLeastOneRemoved)
  {
    return -2;
  }
  else
  {
    return 0;
  }
}

/** Function for constructing an explicit Reference Picture Set out of the available pictures in a referenced Reference Picture Set
*/
#if ALLOW_RECOVERY_POINT_AS_RAP
Void TComSlice::createExplicitReferencePictureSetFromReference( TComList<TComPic*>& rcListPic, TComReferencePictureSet *pReferencePictureSet, Bool isRAP, Int pocRandomAccess, Bool bUseRecoveryPoint)
#else
Void TComSlice::createExplicitReferencePictureSetFromReference( TComList<TComPic*>& rcListPic, TComReferencePictureSet *pReferencePictureSet, Bool isRAP)
#endif
{
  TComPic* rpcPic;
  Int i, j;
  Int k = 0;
  Int nrOfNegativePictures = 0;
  Int nrOfPositivePictures = 0;
  TComReferencePictureSet* pcRPS = this->getLocalRPS();

  // loop through all pictures in the Reference Picture Set
  for(i=0;i<pReferencePictureSet->getNumberOfPictures();i++)
  {
    j = 0;
    // loop through all pictures in the reference picture buffer
    TComList<TComPic*>::iterator iterPic = rcListPic.begin();
    while ( iterPic != rcListPic.end())
    {
      j++;
      rpcPic = *(iterPic++);

      if(rpcPic->getPicSym()->getSlice(0)->getPOC() == this->getPOC() + pReferencePictureSet->getDeltaPOC(i) && rpcPic->getSlice(0)->isReferenced())
      {
        // This picture exists as a reference picture
        // and should be added to the explicit Reference Picture Set
        pcRPS->setDeltaPOC(k, pReferencePictureSet->getDeltaPOC(i));
        pcRPS->setUsed(k, pReferencePictureSet->getUsed(i) && (!isRAP));
#if ALLOW_RECOVERY_POINT_AS_RAP
        pcRPS->setUsed(k, pcRPS->getUsed(k) && !(bUseRecoveryPoint && this->getPOC() > pocRandomAccess && this->getPOC() + pReferencePictureSet->getDeltaPOC(i) < pocRandomAccess) ); 
#endif
        if(pcRPS->getDeltaPOC(k) < 0)
        {
          nrOfNegativePictures++;
        }
        else
        {
          nrOfPositivePictures++;
        }
        k++;
      }
    }
  }
#if EFFICIENT_FIELD_IRAP
  Bool useNewRPS = false;
  // if current picture is complimentary field associated to IRAP, add the IRAP to its RPS. 
  if(m_pcPic->isField())
  {
    TComList<TComPic*>::iterator iterPic = rcListPic.begin();
    while ( iterPic != rcListPic.end())
    {
      rpcPic = *(iterPic++);
      if(rpcPic->getPicSym()->getSlice(0)->getPOC() == this->getAssociatedIRAPPOC() && this->getAssociatedIRAPPOC() == this->getPOC()+1)
      {
        pcRPS->setDeltaPOC(k, 1);
        pcRPS->setUsed(k, true);
        nrOfPositivePictures++;
        k ++;
        useNewRPS = true;
      }
    }
  }
#endif
  pcRPS->setNumberOfNegativePictures(nrOfNegativePictures);
  pcRPS->setNumberOfPositivePictures(nrOfPositivePictures);
  pcRPS->setNumberOfPictures(nrOfNegativePictures+nrOfPositivePictures);
  // This is a simplistic inter rps example. A smarter encoder will look for a better reference RPS to do the 
  // inter RPS prediction with.  Here we just use the reference used by pReferencePictureSet.
  // If pReferencePictureSet is not inter_RPS_predicted, then inter_RPS_prediction is for the current RPS also disabled.
  if (!pReferencePictureSet->getInterRPSPrediction()
#if EFFICIENT_FIELD_IRAP
    || useNewRPS
#endif
    )
  {
    pcRPS->setInterRPSPrediction(false);
    pcRPS->setNumRefIdc(0);
  }
  else
  {
    Int rIdx =  this->getRPSidx() - pReferencePictureSet->getDeltaRIdxMinus1() - 1;
    Int deltaRPS = pReferencePictureSet->getDeltaRPS();
    TComReferencePictureSet* pcRefRPS = this->getSPS()->getRPSList()->getReferencePictureSet(rIdx);
    Int iRefPics = pcRefRPS->getNumberOfPictures();
    Int iNewIdc=0;
    for(i=0; i<= iRefPics; i++) 
    {
      Int deltaPOC = ((i != iRefPics)? pcRefRPS->getDeltaPOC(i) : 0);  // check if the reference abs POC is >= 0
      Int iRefIdc = 0;
      for (j=0; j < pcRPS->getNumberOfPictures(); j++) // loop through the  pictures in the new RPS
      {
        if ( (deltaPOC + deltaRPS) == pcRPS->getDeltaPOC(j))
        {
          if (pcRPS->getUsed(j))
          {
            iRefIdc = 1;
          }
          else
          {
            iRefIdc = 2;
          }
        }
      }
      pcRPS->setRefIdc(i, iRefIdc);
      iNewIdc++;
    }
    pcRPS->setInterRPSPrediction(true);
    pcRPS->setNumRefIdc(iNewIdc);
    pcRPS->setDeltaRPS(deltaRPS); 
    pcRPS->setDeltaRIdxMinus1(pReferencePictureSet->getDeltaRIdxMinus1() + this->getSPS()->getRPSList()->getNumberOfReferencePictureSets() - this->getRPSidx());
  }

  this->setRPS(pcRPS);
  this->setRPSidx(-1);
}

/** get AC and DC values for weighted pred
 * \param *wp
 * \returns Void
 */
Void  TComSlice::getWpAcDcParam(wpACDCParam *&wp)
{
  wp = m_weightACDCParam;
}

/** init AC and DC values for weighted pred
 * \returns Void
 */
Void  TComSlice::initWpAcDcParam()
{
  for(Int iComp = 0; iComp < 3; iComp++ )
  {
    m_weightACDCParam[iComp].iAC = 0;
    m_weightACDCParam[iComp].iDC = 0;
  }
}

/** get WP tables for weighted pred
 * \param RefPicList
 * \param iRefIdx
 * \param *&wpScalingParam
 * \returns Void
 */
Void  TComSlice::getWpScaling( RefPicList e, Int iRefIdx, wpScalingParam *&wp )
{
  wp = m_weightPredTable[e][iRefIdx];
}

/** reset Default WP tables settings : no weight. 
 * \param wpScalingParam
 * \returns Void
 */
Void  TComSlice::resetWpScaling()
{
  for ( Int e=0 ; e<2 ; e++ )
  {
    for ( Int i=0 ; i<MAX_NUM_REF ; i++ )
    {
      for ( Int yuv=0 ; yuv<3 ; yuv++ )
      {
        wpScalingParam  *pwp = &(m_weightPredTable[e][i][yuv]);
        pwp->bPresentFlag      = false;
        pwp->uiLog2WeightDenom = 0;
        pwp->uiLog2WeightDenom = 0;
        pwp->iWeight           = 1;
        pwp->iOffset           = 0;
      }
    }
  }
}

/** init WP table
 * \returns Void
 */
Void  TComSlice::initWpScaling()
{
  for ( Int e=0 ; e<2 ; e++ )
  {
    for ( Int i=0 ; i<MAX_NUM_REF ; i++ )
    {
      for ( Int yuv=0 ; yuv<3 ; yuv++ )
      {
        wpScalingParam  *pwp = &(m_weightPredTable[e][i][yuv]);
        if ( !pwp->bPresentFlag ) 
        {
          // Inferring values not present :
          pwp->iWeight = (1 << pwp->uiLog2WeightDenom);
          pwp->iOffset = 0;
        }

        pwp->w      = pwp->iWeight;
        Int bitDepth = yuv ? g_bitDepthC : g_bitDepthY;
        pwp->o      = pwp->iOffset << (bitDepth-8);
        pwp->shift  = pwp->uiLog2WeightDenom;
        pwp->round  = (pwp->uiLog2WeightDenom>=1) ? (1 << (pwp->uiLog2WeightDenom-1)) : (0);
      }
    }
  }
}

// ------------------------------------------------------------------------------------------------
// Video parameter set (VPS)
// ------------------------------------------------------------------------------------------------
TComVPS::TComVPS()
: m_VPSId                     (  0)
, m_uiMaxTLayers              (  1)
#if H_MV
, m_uiMaxLayersMinus1         (  0)
#else
, m_uiMaxLayers               (  1)
#endif
, m_bTemporalIdNestingFlag    (false)
, m_numHrdParameters          (  0)
#if H_MV
, m_maxLayerId             (  0)
#else
, m_maxNuhReservedZeroLayerId (  0)
#endif
, m_hrdParameters             (NULL)
, m_hrdOpSetIdx               (NULL)
, m_cprmsPresentFlag          (NULL)
#if H_MV
, m_dpbSize                   (NULL)
, m_vpsVUI                 (  NULL )
#endif
{
#if H_MV
  m_vpsBaseLayerInternalFlag = true; 
#if H_MV_HLS10_GEN_VSP_BASE_LAYER_AVAIL
  m_vpsBaseLayerAvailableFlag = true; 
#endif
#endif

  for( Int i = 0; i < MAX_TLAYER; i++)
  {
    m_numReorderPics[i] = 0;
    m_uiMaxDecPicBuffering[i] = 1; 
    m_uiMaxLatencyIncrease[i] = 0;
  }
#if H_MV
  for (Int lsIdx = 0; lsIdx < MAX_VPS_OP_SETS_PLUS1; lsIdx++ )
  {  
    for( Int layerId = 0; layerId < MAX_VPS_NUH_LAYER_ID_PLUS1; layerId++ )
    {
      m_layerIdIncludedFlag[lsIdx][layerId] = (( lsIdx == 0 ) && ( layerId == 0 )) ; 
    }
  } 
  m_vpsNumProfileTierLevelMinus1 = -1; 
    
  m_numAddLayerSets              = 0;   
  m_numAddOlss                   = 0; 
  m_defaultOutputLayerIdc     = 0; 
  
  for ( Int i = 0; i < MAX_VPS_OUTPUTLAYER_SETS; i++)
  {
    m_layerSetIdxForOlsMinus1[i]  = -1; 
#if !H_MV_HLS10_PTL
    m_profileLevelTierIdx[i]      = 0; 
#endif
    for ( Int j = 0; j < MAX_VPS_NUH_LAYER_ID_PLUS1; j++)
    {
#if H_MV_HLS10_PTL
#if H_MV_HLS10_PTL_FIX
      m_profileTierLevelIdx[i][j] = -1; 
#else
      m_profileTierLevelIdx[i][j] = false; 
#endif
#endif
      m_outputLayerFlag[i][j] = false; 
    }
    m_altOutputLayerFlag[ i ]       = false; 
  }

  m_repFormatIdxPresentFlag = false; 
  m_maxOneActiveRefLayerFlag = false; 
  m_directDepTypeLenMinus2   = 0;         
  

  m_vpsExtensionFlag = true; 
  m_vpsNonVuiExtensionLength = 0;
  m_splittingFlag    = false;
  
  for( Int i = 0; i < MAX_NUM_SCALABILITY_TYPES; i++ )
  {
    m_scalabilityMaskFlag[i] = false;
    m_dimensionIdLen [i]  = -1; 
  }

  m_vpsNuhLayerIdPresentFlag = false;

  for( Int i = 0; i < MAX_VPS_OP_SETS_PLUS1; i++ )
  {
    m_vpsProfilePresentFlag   [i] = false;
    m_layerSetIdxForOlsMinus1       [i] = 0;
    for( Int j = 0; j < MAX_VPS_NUH_LAYER_ID_PLUS1; j++ )
    {
      m_outputLayerFlag[i][j] = false;
    }
  }

  for( Int i = 0; i < MAX_NUM_LAYER_IDS; i++ )
  {
    m_layerIdInVps[i] =  (i == 0 ) ? 0 : -1;         
  }

  for( Int i = 0; i < MAX_NUM_LAYERS; i++ )
  {
    m_layerIdInNuh      [i] = ( i == 0 ) ? 0 : -1; 
    m_numDirectRefLayers[i] = 0; 
    m_vpsRepFormatIdx    [i] = 0; 
    m_pocLsbNotPresentFlag[i] = 0;
    m_repFormat          [i] = NULL; 
    m_viewIdVal          [i] = 0; 


    for( Int j = 0; j < MAX_NUM_LAYERS; j++ )
    {
      m_directDependencyFlag[i][j] = false;
      m_directDependencyType[i][j] = -1; 
#if H_MV_HLS10_REF_PRED_LAYERS
      m_dependencyFlag  [i][j]    = false; 
      m_idDirectRefLayer[i][j]    = -1; 
      m_idPredictedLayer[i][j]    = -1; 
      m_idRefLayer      [i][j]    = -1; 
#else
      m_refLayerId[i][j]           = -1; 
#endif
      m_maxTidIlRefPicsPlus1[i][j]  = 7;
    }

    for( Int j = 0; j < MAX_NUM_SCALABILITY_TYPES; j++ )
    {
      m_dimensionId[i][j] = 0;
    }
  }
  m_vpsVUI = new TComVPSVUI; 
  m_dpbSize = new TComDpbSize; 

#endif
}

TComVPS::~TComVPS()
{
  if( m_hrdParameters    != NULL )     delete[] m_hrdParameters;
  if( m_hrdOpSetIdx      != NULL )     delete[] m_hrdOpSetIdx;
  if( m_cprmsPresentFlag != NULL )     delete[] m_cprmsPresentFlag;
#if H_MV
  if ( m_vpsVUI          != NULL )     delete m_vpsVUI; 
  if ( m_dpbSize         != NULL )     delete m_dpbSize; 

  for( Int i = 0; i < MAX_NUM_LAYERS; i++ )
  {
    if (m_repFormat[ i ] != NULL )      delete m_repFormat[ i ];    
  }
#endif
}

#if H_MV

Bool TComVPS::checkVPSExtensionSyntax()
{
  for( Int layer = 1; layer <= getMaxLayersMinus1(); layer++ )
  {
    // check layer_id_in_nuh constraint
    assert( getLayerIdInNuh( layer ) > getLayerIdInNuh( layer -1 ) );
  }

  //The value of vps_num_rep_formats_minus1 shall be in the range of 0 to 255, inclusive.
  assert( getVpsNumRepFormatsMinus1() >= 0 ); 
  assert( getVpsNumRepFormatsMinus1() <= 255 ); 

#if H_MV_HLS10_ADD_LAYERSETS
  // The value of num_add_layer_sets shall be in the range of 0 to 1023, inclusive. 
  assert( getNumAddLayerSets() >= 0 && getNumAddLayerSets() <= 1023 ); 
#endif
  return true; 
}

Int TComVPS::getNumScalabilityTypes()
{
  return scalTypeToScalIdx( ScalabilityType(MAX_NUM_SCALABILITY_TYPES) );
}

Int TComVPS::scalTypeToScalIdx( ScalabilityType scalType )
{
  assert( scalType >= 0 && scalType <= MAX_NUM_SCALABILITY_TYPES ); 
  assert( scalType == MAX_NUM_SCALABILITY_TYPES || getScalabilityMaskFlag( scalType ) );
  Int scalIdx = 0; 
  for( Int curScalType = 0; curScalType < scalType; curScalType++ )
  {
    scalIdx += ( getScalabilityMaskFlag( curScalType ) ? 1 : 0 );

  }

  return scalIdx; 
}
Void TComVPS::setScalabilityMaskFlag( UInt val )
{
  for ( Int scalType = 0; scalType < MAX_NUM_SCALABILITY_TYPES; scalType++ ) 
  {
    setScalabilityMaskFlag( scalType, ( val & (1 << scalType ) ) != 0 );
  }
}

Void TComVPS::setRefLayers()
{

#if H_MV_HLS10_REF_PRED_LAYERS
  for( Int i = 0; i  <=  getMaxLayersMinus1(); i++ )
  {
    for( Int j = 0; j  <=  getMaxLayersMinus1(); j++ )
    {
      m_dependencyFlag[ i ][ j ] = getDirectDependencyFlag( i , j );
      for( Int k = 0; k < i; k++ )
      {
        if( getDirectDependencyFlag(i , k )  &&  m_dependencyFlag[k][j] )
        {
          m_dependencyFlag[ i ][ j ] = true;
        }
      }
    }
  }

  for( Int i = 0; i  <=  getMaxLayersMinus1(); i++ )
  {
    Int iNuhLId = getLayerIdInNuh( i );
    Int d = 0;
    Int r = 0;
    Int p = 0;
    for( Int j = 0; j  <=  getMaxLayersMinus1(); j++ )
    {
      Int jNuhLid = getLayerIdInNuh( j );
      if( getDirectDependencyFlag( i , j ) )
      {
        m_idDirectRefLayer[iNuhLId][d++] = jNuhLid;
      }
      if( getDependencyFlag( i , j ) )
      {
        m_idRefLayer      [iNuhLId][r++] = jNuhLid;
      }
      if( getDependencyFlag( j , i ) )
      {
        m_idPredictedLayer[iNuhLId][p++] = jNuhLid;
      }
    }
    m_numDirectRefLayers[ iNuhLId ] = d;
    m_numRefLayers      [ iNuhLId ] = r;
    m_numPredictedLayers[ iNuhLId ] = p;
  }
  
  Bool layerIdInListFlag[ 64 ]; 
  for( Int i = 0; i  <=  63; i++ )
  {
    layerIdInListFlag[ i ] = 0;
  }

  Int k = 0; 
  for( Int i = 0; i  <=  getMaxLayersMinus1(); i++ )
  {
    Int iNuhLId = getLayerIdInNuh( i );
    if( getNumDirectRefLayers( iNuhLId )  ==  0 )
    {
      m_treePartitionLayerIdList[ k ][ 0 ] = iNuhLId;
      Int h = 1;  
      for( Int j = 0; j < getNumPredictedLayers( iNuhLId ); j++ )  
      {
        Int predLId = getIdPredictedLayer( iNuhLId, j );
        if ( !layerIdInListFlag[ predLId ] )
        {
          m_treePartitionLayerIdList[ k ][ h++ ] = predLId;
          layerIdInListFlag[ predLId ] = 1; 
        }          
      }
      m_numLayersInTreePartition[ k++ ] = h;
    }
  }
  m_numIndependentLayers = k;
#else // H_MV_HLS10_GEN

  for( Int i = 0; i  <= getMaxLayersMinus1(); i++ )
  {
    Int iNuhLId = getLayerIdInNuh( i ); 
    m_numDirectRefLayers[ iNuhLId ] = 0; 
    for( Int j = 0; j < i; j++ )
    {
      if( getDirectDependencyFlag(i , j) )
      {
        m_refLayerId[ iNuhLId ][m_numDirectRefLayers[ iNuhLId ]++ ] = getLayerIdInNuh( j );
      }
    }
  }

  for (Int i = 0 ; i < MAX_NUM_LAYER_IDS; i++ )
  {
    m_numRefLayers[i] = 0; 
  }

  for (Int currLayerId = 0; currLayerId <= 62; currLayerId++ ) 
  {
    for (Int i = 0 ; i < MAX_NUM_LAYER_IDS; i++ )
    {
      m_recursiveRefLayerFlag[currLayerId][i] = 0; 
    }
  }

  for( Int i = 0; i  <=  getMaxLayersMinus1(); i++ )
  {
    Int iNuhLId = getLayerIdInNuh( i );
    xSetRefLayerFlags( iNuhLId );
    for( Int j = 0; j < 63; j++ )
    {
      m_numRefLayers[ iNuhLId ]  +=  m_recursiveRefLayerFlag[ iNuhLId ][ j ];
    }
  }
  
  for( Int i = 0; i <= getMaxLayersMinus1(); i++ )  // Bug in spec "<" instead of "<=" 
  {
    Int iNuhLId = getLayerIdInNuh( i );
    Int predIdx = 0;
    for( Int j = iNuhLId + 1; j < 63; j++ )
    {
      if( m_recursiveRefLayerFlag[ j ][ iNuhLId ] )
      {
        m_predictedLayerId[ iNuhLId ][ predIdx++ ] = j;
      }
    }
    m_numPredictedLayers[ iNuhLId ] = predIdx;
  }
  
  Bool countedLayerIdxFlag[ MAX_NUM_LAYERS ]; 
  for( Int i = 0; i  <=  getMaxLayersMinus1(); i++ )
  {
    countedLayerIdxFlag[ i ] = 0;
  }
  for( Int i = 0, k = 0; i  <=  getMaxLayersMinus1(); i++ )
  {
    Int iNuhLId = getLayerIdInNuh( i );
    if( m_numDirectRefLayers[ iNuhLId ]  ==  0 )
    {
      m_treePartitionLayerIdList[ k ][ 0 ] = iNuhLId;
      m_numLayersInTreePartition[ k ]      = 1;

      for( Int j = 0; j < m_numPredictedLayers[ iNuhLId ]; j++ )  
      {
        if( !countedLayerIdxFlag[ getLayerIdInVps( m_predictedLayerId[ iNuhLId ][ j ] ) ] )    
        {
          m_treePartitionLayerIdList[ k ][ m_numLayersInTreePartition[ k ] ] = m_predictedLayerId[ iNuhLId ][ j ];
          m_numLayersInTreePartition[ k ]++;
          countedLayerIdxFlag[ getLayerIdInVps( m_predictedLayerId[ iNuhLId ][ j ] ) ] = 1;
        }
      }
      k++;

      m_numIndependentLayers = k;
    }
  }
#endif // H_MV_HLS10_GEN
}

#if !H_MV_HLS10_REF_PRED_LAYERS
Int TComVPS::getRefLayerId( Int layerIdInNuh, Int idx )
{
  assert( idx >= 0 && idx < m_numDirectRefLayers[layerIdInNuh] );     
  Int refLayerIdInNuh = m_refLayerId[ layerIdInNuh ][ idx ];    
  assert ( refLayerIdInNuh >= 0 ); 
  return refLayerIdInNuh;
}
#endif

Int TComVPS::getScalabilityId( Int layerIdInVps, ScalabilityType scalType )
{
  return getScalabilityMaskFlag( scalType ) ? getDimensionId( layerIdInVps, scalTypeToScalIdx( scalType ) ) : 0;
}



Int TComVPS::xGetDimBitOffset( Int j )
{
  Int dimBitOffset = 0; 
  if ( getSplittingFlag() && j == getNumScalabilityTypes() )
  {
     dimBitOffset = 6; 
  }
  else
  {
    for (Int dimIdx = 0; dimIdx <= j-1; dimIdx++)
    {
      dimBitOffset += getDimensionIdLen( dimIdx ); 
    }
  }
  return dimBitOffset; 
}

Int TComVPS::inferDimensionId( Int i, Int j )
{
    return ( ( getLayerIdInNuh( i ) & ( (1 << xGetDimBitOffset( j + 1 ) ) - 1) ) >> xGetDimBitOffset( j ) ); 
}

Int TComVPS::inferLastDimsionIdLenMinus1()
{
  return ( 5 - xGetDimBitOffset( getNumScalabilityTypes() - 1 ) ); 
}

Int TComVPS::getNumLayersInIdList( Int lsIdx )
{
  assert( lsIdx >= 0 ); 
#if H_MV_HLS10_ADD_LAYERSETS
  assert( lsIdx <= getNumLayerSets() ); 
#else
  assert( lsIdx <= getVpsNumLayerSetsMinus1() ); 
#endif
  return (Int) m_layerSetLayerIdList[ lsIdx ].size(); 
}

Int    TComVPS::getNumOutputLayerSets() 
{
  return getNumAddOlss() + getNumLayerSets(); 
}

Int TComVPS::getNumViews()
{
  Int numViews = 1; 
  for( Int i = 0; i <=  getMaxLayersMinus1(); i++ )
  {
    Int lId = getLayerIdInNuh( i ); 
    if ( i > 0 && ( getViewIndex( lId ) != getScalabilityId( i - 1, VIEW_ORDER_INDEX ) ) )
    {
      numViews++; 
    }    
  }

  return numViews;
}

#if !H_MV_HLS10_REF_PRED_LAYERS
Bool TComVPS::getInDirectDependencyFlag( Int depLayeridInVps, Int refLayeridInVps, Int depth /*= 0 */ )
{
#if H_MV_HLS10_REF_PRED_LAYERS
  // TBD: Remove getInDirectDependencyFlag entirely.
  return getDependencyFlag( depLayeridInVps, refLayeridInVps );
#else
  assert( depth < 65 ); 
  Bool dependentFlag = getDirectDependencyFlag( depLayeridInVps, refLayeridInVps ); 

  for( Int i = 0; i < depLayeridInVps && !dependentFlag; i++ )
  {
    if ( getDirectDependencyFlag( depLayeridInVps, i ) )
    {
      dependentFlag = getInDirectDependencyFlag( i, refLayeridInVps, depth++ ); 
    }
  }
  return dependentFlag;
#endif
}
#endif

Void TComVPS::deriveLayerSetLayerIdList()
{
  m_layerSetLayerIdList.resize( getVpsNumLayerSetsMinus1() + 1 ); 
  for (Int i = 0; i <= getVpsNumLayerSetsMinus1(); i++ )
  {
    for( Int m = 0; m  <= getVpsMaxLayerId(); m++ )
    {
      if( getLayerIdIncludedFlag( i, m) ) 
      {
        m_layerSetLayerIdList[ i ].push_back( m );        
      }
    }
  }
}

Void TComVPS::initTargetLayerIdLists()
{
  m_targetDecLayerIdLists.resize( getNumOutputLayerSets() ); 
  m_targetOptLayerIdLists.resize( getNumOutputLayerSets() ); 
}

Void TComVPS::deriveTargetLayerIdList( Int i )
{  
  Int lsIdx = olsIdxToLsIdx( i );     

  for( Int j = 0; j < getNumLayersInIdList( lsIdx ); j++ )
  {
#if H_MV_HLS10_NESSECARY_LAYER
    if ( getNecessaryLayerFlag( i , j ))
    {
#endif
      m_targetDecLayerIdLists[i].push_back( m_layerSetLayerIdList[ lsIdx ][ j ] ); 
#if H_MV_HLS10_NESSECARY_LAYER
    }
#endif

    if( getOutputLayerFlag( i, j  ))
    {
      m_targetOptLayerIdLists[i].push_back( m_layerSetLayerIdList[ lsIdx ][ j ] );
    }
  }  
  assert( getNumOutputLayersInOutputLayerSet( i ) > 0 ); 
}

Bool TComVPS::inferOutputLayerFlag( Int i, Int j )
{
  Bool outputLayerFlag; 
  switch ( getDefaultOutputLayerIdc( ) )
  {
  case 0:
    outputLayerFlag = true; 
    break; 
  case 1:
    outputLayerFlag = ( j == m_layerSetLayerIdList[ olsIdxToLsIdx( i ) ].size() - 1 );  
    break;
  case 2:
    if ( i == 0 && j == 0)
    {     
      outputLayerFlag = true;  // This is a software only fix for a bug in the spec. In spec outputLayerFlag is neither present nor inferred. 
    }
    else
    {
      assert( 0 ); 
    }
    break; 
  default:      
    assert( 0 );
    break; 
  }
  return outputLayerFlag;
}

Int TComVPS::getMaxSubLayersInLayerSetMinus1( Int i )
{
  Int maxSLMinus1 = 0; 
  for( Int k = 0; k < getNumLayersInIdList( i ); k++ )
  {
    Int lId = m_layerSetLayerIdList[i][k];
    maxSLMinus1 = std::max( maxSLMinus1, getSubLayersVpsMaxMinus1( getLayerIdInVps( lId ) ));
  }
  return maxSLMinus1;
}

#if !H_MV_HLS10_ADD_LAYERSETS
Void TComVPS::inferDbpSizeLayerSetZero( TComSPS* sps, Bool encoder )
{
  for( Int j = 0; j <= getMaxSubLayersInLayerSetMinus1( 0 ); j++ )
  {
    Int maxDecPicBufferingMinus1 = sps->getMaxDecPicBuffering( j ) - 1; 
    Int numReorderPics           = sps->getNumReorderPics    ( j ); 
    Int maxLatencyIncreasePlus1  = sps->getMaxLatencyIncrease( j ); 

    if ( encoder )
    {
      assert( getDpbSize()->getMaxVpsDecPicBufferingMinus1(0, 0, j ) == maxDecPicBufferingMinus1 );
      assert( getDpbSize()->getMaxVpsNumReorderPics       (0,    j ) == numReorderPics           );
      assert( getDpbSize()->getMaxVpsLatencyIncreasePlus1 (0,    j ) == maxLatencyIncreasePlus1  );
    }
    else
    {
      getDpbSize()->setMaxVpsDecPicBufferingMinus1(0, 0, j, maxDecPicBufferingMinus1 );
      getDpbSize()->setMaxVpsNumReorderPics       (0,    j, numReorderPics           );
      getDpbSize()->setMaxVpsLatencyIncreasePlus1 (0,    j, maxLatencyIncreasePlus1  );        
    }     
  }
}
#endif
Bool TComVPS::getAltOutputLayerFlagVar( Int i )
{
  // Semantics variable not syntax element !

  Bool altOptLayerFlag = false;     
  if ( i > 0 && getNumOutputLayersInOutputLayerSet( i ) == 1 && 
    getNumDirectRefLayers( getOlsHighestOutputLayerId( i ) ) > 0 )
  {
    altOptLayerFlag = getAltOutputLayerFlag( i ); 
  }
  return altOptLayerFlag;
}

#if !H_MV_HLS10_MAXNUMPICS
Int TComVPS::getMaxNumPics( Int layerId )
{
  Int maxNumPics = MAX_INT; 
  for( Int olsIdx = 0; olsIdx < getNumOutputLayerSets(); olsIdx++)
  {
    Int lsIdx = olsIdxToLsIdx( olsIdx ); 
    for( Int j = 0; j < getNumLayersInIdList( lsIdx ); j++ )
    {
      if( getLayerSetLayerIdList(lsIdx, j ) ==  layerId )
      {
        Int maxSL = getMaxSubLayersInLayerSetMinus1( lsIdx ); 
        maxNumPics = std::min( maxNumPics, getDpbSize()->getMaxVpsDecPicBufferingMinus1( olsIdx , j,  maxSL ) );
      }
    }
  }
  assert( maxNumPics != MAX_INT ); 
  return maxNumPics;
}
#endif

#if !H_MV_HLS10_REF_PRED_LAYERS
Void TComVPS::xSetRefLayerFlags( Int currLayerId )
{
  for( Int j = 0; j < getNumDirectRefLayers( currLayerId ); j++ )
  {
    Int refLayerId = m_refLayerId[ currLayerId ][ j ];
    m_recursiveRefLayerFlag[ currLayerId ][ refLayerId ] = 1;
    for( Int k = 0; k < MAX_NUM_LAYER_IDS; k++ )
    {
      m_recursiveRefLayerFlag[ currLayerId ][ k ] = m_recursiveRefLayerFlag[ currLayerId ][ k ]  ||  m_recursiveRefLayerFlag[ refLayerId ][ k ];
    }
  }
}

#endif

#if H_MV_HLS10_PTL_FIX
Int TComVPS::inferProfileTierLevelIdx(Int i, Int j)
{
  Bool inferZero        = ( i == 0 && j == 0 &&  getVpsBaseLayerInternalFlag() );
  Bool inferGreaterZero = getNecessaryLayerFlag(i,j) && ( getVpsNumProfileTierLevelMinus1() == 0 ); 
  assert( inferZero || inferGreaterZero );

  Bool ptlIdx = 0; // inference for greaterZero
  if ( inferZero )
  {
    ptlIdx = getMaxLayersMinus1() > 0 ? 1 : 0; 
    if ( inferGreaterZero )
    {
      assert( ptlIdx == 0 );  
      // This should never happen since :
      // When vps_max_layers_minus1 is greater than 0, the value of vps_num_profile_tier_level_minus1 shall be greater than or equal to 1.
    }
  }
  return ptlIdx;
}
#endif

#if H_MV_HLS10_ADD_LAYERSETS
Void TComVPS::deriveAddLayerSetLayerIdList(Int i)
{
  assert( m_layerSetLayerIdList.size() ==  ( getVpsNumLayerSetsMinus1() + 1 + i ) ); 
  std::vector<Int> layerSetLayerIdList;

  for( Int treeIdx = 1; treeIdx < getNumIndependentLayers(); treeIdx++ )
  { 
    // The value of highest_layer_idx_plus1[ i ][ j ] shall be in the range of 0 to NumLayersInTreePartition[ j ], inclusive.
    assert( getHighestLayerIdxPlus1( i, treeIdx ) >= 0 && getHighestLayerIdxPlus1( i, treeIdx ) <= getNumLayersInTreePartition( treeIdx ) );

    for( Int layerCnt = 0; layerCnt < getHighestLayerIdxPlus1( i, treeIdx ); layerCnt++ )
    {
      layerSetLayerIdList.push_back( getTreePartitionLayerIdList( treeIdx, layerCnt ) );
    }
  }
  m_layerSetLayerIdList.push_back( layerSetLayerIdList ); 

  //It is a requirement of bitstream conformance that 
  //NumLayersInIdList[ vps_num_layer_sets_minus1 + 1 + i ] shall be greater than 0.
  assert( getNumLayersInIdList( getVpsNumLayerSetsMinus1() + 1 + i ) > 0 );
}

#endif

#if H_MV_HLS10_NESSECARY_LAYER
Void TComVPS::deriveNecessaryLayerFlags(Int olsIdx)
{
  AOF( olsIdx >= 0 && olsIdx < getNumOutputLayerSets() ); 
  Int lsIdx = olsIdxToLsIdx( olsIdx );
  for( Int lsLayerIdx = 0; lsLayerIdx < getNumLayersInIdList( lsIdx) ; lsLayerIdx++ )
  {
    m_necessaryLayerFlag[ olsIdx ][ lsLayerIdx ] = 0;
  }
  for( Int lsLayerIdx = 0; lsLayerIdx < getNumLayersInIdList( lsIdx ); lsLayerIdx++ )
  {
    if( getOutputLayerFlag( olsIdx, lsLayerIdx  ))
    {
      m_necessaryLayerFlag[ olsIdx ][ lsLayerIdx ] = 1;
      Int currLayerId = getLayerSetLayerIdList( lsIdx, lsLayerIdx );
      for( Int rLsLayerIdx = 0; rLsLayerIdx < lsLayerIdx; rLsLayerIdx++ )
      {
        Int refLayerId = getLayerSetLayerIdList( lsIdx, rLsLayerIdx );
        if( getDependencyFlag( getLayerIdInVps( currLayerId ), getLayerIdInVps( refLayerId ) ) )
        {
          m_necessaryLayerFlag[ olsIdx ][ rLsLayerIdx ] = 1;
        }
      }
    }
  }
  m_numNecessaryLayers[ olsIdx ] = 0;
  for( Int lsLayerIdx = 0; lsLayerIdx < getNumLayersInIdList( lsIdx ); lsLayerIdx++ ) 
  {
    m_numNecessaryLayers[ olsIdx ]  +=  m_necessaryLayerFlag[ olsIdx ][ lsLayerIdx ];
  }
}
#endif

#if H_MV_HLS10_ADD_LAYERSETS
Void TComVPS::printPTL()
{
  std::vector<Int> idx; 
  std::vector<Int> num; 
  std::vector< std::vector<Int> > ptlInfo; 

  std::cout << std::right << std::setw(60) << std::setfill('-') << " " << std::setfill(' ') << std::endl << "PTLI" << std::endl; 

  for ( Int i = 0; i <= getVpsNumProfileTierLevelMinus1(); i++ )
  {
    std::vector<Int> curPtlInfo;
    ProfileTierLevel* ptl = getPTL( i )->getGeneralPTL(); 
    curPtlInfo.push_back( (Int) ptl->getProfileIdc()  );
    curPtlInfo.push_back( (Int) ptl->getTierFlag()    );
    curPtlInfo.push_back( (Int) ptl->getLevelIdc()    );
    curPtlInfo.push_back( (Int) ptl->getInbldFlag()   );

    idx.push_back ( i );
    num.push_back ( 4 ); 
    ptlInfo.push_back( curPtlInfo );
  } 

  xPrintArray( "VpsProfileTierLevel", getVpsNumProfileTierLevelMinus1() + 1, idx, num, ptlInfo, false  ); 

  num.clear(); 
  idx.clear(); 
  for (Int i = 0; i < getNumOutputLayerSets(); i++)
  {
    num.push_back ( getNumLayersInIdList( olsIdxToLsIdx( i ))  ); 
    idx.push_back( i ); 
  }

  xPrintArray( "profile_tier_level_idx", getNumOutputLayerSets(), idx, num, m_profileTierLevelIdx, true );
  std::cout << std::endl;
}

Void TComVPS::printLayerDependencies()
{
  vector<Int> fullArray;
  vector<Int> range; 


#if H_MV_HLS10_AUX
  vector<Int> viewOrderIndex;
  vector<Int> auxId;
  vector<Int> dependencyId; 
  vector<Int> viewId; 
#endif
  for (Int i = 0; i <= getMaxLayersMinus1(); i++ )
  {
    fullArray.push_back( getMaxLayersMinus1() + 1 ); 
    range.push_back( i ); 
#if H_MV_HLS10_AUX      
    viewOrderIndex.push_back( getViewIndex   ( i ) );
    dependencyId  .push_back( getDependencyId( i ) );
    auxId         .push_back( getAuxId       ( i ) );      
    viewId        .push_back( getViewId      ( getLayerIdInNuh( i ) ) );
#endif
  }
  std::cout << std::right << std::setw(60) << std::setfill('-') << " " << std::setfill(' ') << std::endl << "Layer Dependencies" << std::endl; 
  xPrintArray( "direct_dependency_flag", getMaxLayersMinus1()+1, range, fullArray, m_directDependencyFlag, false ); 
  xPrintArray( "DependencyFlag", getMaxLayersMinus1()+1, range, fullArray, m_dependencyFlag, false ); 
  xPrintArray( "layer_id_in_nuh", getMaxLayersMinus1()+1, m_layerIdInNuh, true  );     
  xPrintArray( "IdPredictedLayer", getMaxLayersMinus1() + 1, m_layerIdInNuh, m_numPredictedLayers, m_idPredictedLayer, true );
  xPrintArray( "IdRefLayer"      , getMaxLayersMinus1() + 1, m_layerIdInNuh, m_numRefLayers, m_idRefLayer, true );
  xPrintArray( "IdDirectRefLayer", getMaxLayersMinus1() + 1, m_layerIdInNuh, m_numDirectRefLayers, m_idDirectRefLayer, true );
  std::cout << std::endl;
}

#if H_MV_HLS10_AUX
Void TComVPS::printScalabilityId()
{
  vector<Int> layerIdxInVps; 


  vector<Int> viewOrderIndex;
  vector<Int> auxId;
  vector<Int> dependencyId; 
  vector<Int> viewId; 

  for (Int i = 0; i <= getMaxLayersMinus1(); i++ )
  {
    Int layerIdInNuh = getLayerIdInNuh( i );
    layerIdxInVps  .push_back( i ); 
    viewOrderIndex.push_back( getViewIndex   ( layerIdInNuh ) );
    dependencyId  .push_back( getDependencyId( layerIdInNuh ) );
    auxId         .push_back( getAuxId       ( layerIdInNuh ) );      
    viewId        .push_back( getViewId      ( layerIdInNuh ) );
  }

  std::cout << std::right << std::setw(60) << std::setfill('-') << " " << std::setfill(' ') << std::endl << "Scalability Ids" << std::endl; 
  xPrintArray( "layerIdxInVps"  , getMaxLayersMinus1()+1, layerIdxInVps,          false );
  xPrintArray( "layer_id_in_nuh", getMaxLayersMinus1()+1, m_layerIdInNuh, false );     

  xPrintArray( "ViewOrderIndex", getMaxLayersMinus1()+1, viewOrderIndex, false );     
  xPrintArray( "DependencyId"  , getMaxLayersMinus1()+1, dependencyId  , false );     
  xPrintArray( "AuxId"         , getMaxLayersMinus1()+1, auxId         , false );     
  xPrintArray( "ViewId"        , getMaxLayersMinus1()+1, viewId        , false );     

  std::cout << std::endl;
}
#endif

Void TComVPS::printLayerSets()
{
  vector<Int> fullArray;
  vector<Int> numLayersInIdList; 
  vector<Int> rangeLayerSets; 


  for (Int i = 0; i < getNumLayerSets(); i++ )
  {
    numLayersInIdList.push_back( getNumLayersInIdList( i ) );       
    rangeLayerSets.push_back( i ); 
  }

  vector<Int> rangeOutputLayerSets; 
  vector<Int> numOutputLayersInOutputLayerSet; 
  vector<Int> numDecLayer; 
  vector<Int> numLayersInLayerSetForOutputLayerSet; 
  vector<Int> vOlsIdxToLsIdx;
  for (Int i = 0; i < getNumOutputLayerSets(); i++ )
  {
    vOlsIdxToLsIdx.push_back( olsIdxToLsIdx(i));
    numOutputLayersInOutputLayerSet.push_back( getNumOutputLayersInOutputLayerSet( i ) );       
    numDecLayer.push_back( (Int) m_targetDecLayerIdLists[ i ].size() );
    rangeOutputLayerSets.push_back( i ); 
    numLayersInLayerSetForOutputLayerSet.push_back( getNumLayersInIdList( olsIdxToLsIdx( i ) ) );
  }

  vector<Int> rangeIndependentLayers;
  for(Int i = 0; i < getNumIndependentLayers(); i++ )
  {
    rangeIndependentLayers.push_back( i );    
  }

  vector<Int> rangeAddLayerSets;
  vector<Int> numHighestLayerIdxPlus1; 
  for(Int i = 0; i < getNumAddLayerSets(); i++ )
  {
    rangeAddLayerSets.push_back( i );    
    numHighestLayerIdxPlus1.push_back( getNumIndependentLayers() );
  }

  std::cout << std::right << std::setw(60) << std::setfill('-') << " " << std::setfill(' ') << std::endl << "Layer Sets" << std::endl;     
  xPrintArray( "TreePartitionLayerIdList", getNumIndependentLayers(), rangeIndependentLayers, m_numLayersInTreePartition, m_treePartitionLayerIdList, true );
  xPrintArray( "highest_layer_idx_plus1", getNumAddLayerSets(), rangeAddLayerSets, numHighestLayerIdxPlus1, m_highestLayerIdxPlus1, true ); 
  xPrintArray( "LayerSetLayerIdList" , (Int) getNumLayerSets()      , rangeLayerSets      , numLayersInIdList, m_layerSetLayerIdList, true );
  xPrintArray( "OlsIdxToLsIdx", (Int) vOlsIdxToLsIdx.size(), vOlsIdxToLsIdx, true ); 
  xPrintArray( "OutputLayerFlag"     , getNumOutputLayerSets(), rangeOutputLayerSets, numLayersInLayerSetForOutputLayerSet, m_outputLayerFlag, true );
  xPrintArray( "TargetOptLayerIdList", getNumOutputLayerSets(), rangeOutputLayerSets, numOutputLayersInOutputLayerSet, m_targetOptLayerIdLists, true );
  xPrintArray( "NecessaryLayerFlag"  , getNumOutputLayerSets(), rangeOutputLayerSets, numLayersInLayerSetForOutputLayerSet, m_necessaryLayerFlag   , true );
  xPrintArray( "TargetDecLayerIdList", getNumOutputLayerSets(), rangeOutputLayerSets, numDecLayer,                     m_targetDecLayerIdLists, true );
  std::cout << endl;
}

#endif

#endif // H_MV

// ------------------------------------------------------------------------------------------------
// Sequence parameter set (SPS)
// ------------------------------------------------------------------------------------------------

TComSPS::TComSPS()
: m_SPSId                     (  0)
, m_VPSId                     (  0)
, m_chromaFormatIdc           (CHROMA_420)
, m_uiMaxTLayers              (  1)
// Structure
, m_picWidthInLumaSamples     (352)
, m_picHeightInLumaSamples    (288)
, m_log2MinCodingBlockSize    (  0)
, m_log2DiffMaxMinCodingBlockSize (0)
, m_uiMaxCUWidth              ( 32)
, m_uiMaxCUHeight             ( 32)
, m_uiMaxCUDepth              (  3)
, m_bLongTermRefsPresent      (false)
, m_uiQuadtreeTULog2MaxSize   (  0)
, m_uiQuadtreeTULog2MinSize   (  0)
, m_uiQuadtreeTUMaxDepthInter (  0)
, m_uiQuadtreeTUMaxDepthIntra (  0)
// Tool list
, m_usePCM                   (false)
, m_pcmLog2MaxSize            (  5)
, m_uiPCMLog2MinSize          (  7)
, m_bitDepthY                 (  8)
, m_bitDepthC                 (  8)
, m_qpBDOffsetY               (  0)
, m_qpBDOffsetC               (  0)
, m_uiPCMBitDepthLuma         (  8)
, m_uiPCMBitDepthChroma       (  8)
, m_bPCMFilterDisableFlag     (false)
, m_uiBitsForPOC              (  8)
, m_numLongTermRefPicSPS    (  0)  
, m_uiMaxTrSize               ( 32)
, m_bUseSAO                   (false) 
, m_bTemporalIdNestingFlag    (false)
, m_scalingListEnabledFlag    (false)
, m_useStrongIntraSmoothing   (false)
, m_vuiParametersPresentFlag  (false)
, m_vuiParameters             ()
#if H_MV
, m_pcVPS                     ( NULL )
, m_spsInferScalingListFlag   ( false )
, m_spsScalingListRefLayerId  ( 0 )

, m_updateRepFormatFlag       ( false ) 
, m_spsRepFormatIdx           ( 0 )
, m_interViewMvVertConstraintFlag (false)
#endif
{
  for ( Int i = 0; i < MAX_TLAYER; i++ )
  {
    m_uiMaxLatencyIncrease[i] = 0;
    m_uiMaxDecPicBuffering[i] = 1;
    m_numReorderPics[i]       = 0;
  }
  m_scalingList = new TComScalingList;
  ::memset(m_ltRefPicPocLsbSps, 0, sizeof(m_ltRefPicPocLsbSps));
  ::memset(m_usedByCurrPicLtSPSFlag, 0, sizeof(m_usedByCurrPicLtSPSFlag));
#if H_MV
  m_spsRangeExtensionsFlag     = false;
  m_spsMultilayerExtensionFlag = false;
  m_spsExtension6bits          = 0;

#if !H_MV_HLS10_PPS
  m_numScaledRefLayerOffsets = 0; 

  for (Int i = 0; i < MAX_NUM_SCALED_REF_LAYERS; i++ )
  {
    m_scaledRefLayerId             [i] = -1;
  }

  for (Int i = 0; i < MAX_NUM_LAYERS; i++ )
  {
    m_scaledRefLayerLeftOffset     [i] = 0;
    m_scaledRefLayerTopOffset      [i] = 0;
    m_scaledRefLayerRiFghtOffset    [i] = 0;
    m_scaledRefLayerBottomOffset   [i] = 0;
  }
#endif
#endif
}

TComSPS::~TComSPS()
{
  delete m_scalingList;
  m_RPSList.destroy();
}

Void  TComSPS::createRPSList( Int numRPS )
{ 
  m_RPSList.destroy();
  m_RPSList.create(numRPS);
}

Void TComSPS::setHrdParameters( UInt frameRate, UInt numDU, UInt bitRate, Bool randomAccess )
{
  if( !getVuiParametersPresentFlag() )
  {
    return;
  }

  TComVUI *vui = getVuiParameters();
  TComHRD *hrd = vui->getHrdParameters();

  TimingInfo *timingInfo = vui->getTimingInfo();
  timingInfo->setTimingInfoPresentFlag( true );
  switch( frameRate )
  {
  case 24:
    timingInfo->setNumUnitsInTick( 1125000 );    timingInfo->setTimeScale    ( 27000000 );
    break;
  case 25:
    timingInfo->setNumUnitsInTick( 1080000 );    timingInfo->setTimeScale    ( 27000000 );
    break;
  case 30:
    timingInfo->setNumUnitsInTick( 900900 );     timingInfo->setTimeScale    ( 27000000 );
    break;
  case 50:
    timingInfo->setNumUnitsInTick( 540000 );     timingInfo->setTimeScale    ( 27000000 );
    break;
  case 60:
    timingInfo->setNumUnitsInTick( 450450 );     timingInfo->setTimeScale    ( 27000000 );
    break;
  default:
    timingInfo->setNumUnitsInTick( 1001 );       timingInfo->setTimeScale    ( 60000 );
    break;
  }

  Bool rateCnt = ( bitRate > 0 );
  hrd->setNalHrdParametersPresentFlag( rateCnt );
  hrd->setVclHrdParametersPresentFlag( rateCnt );

  hrd->setSubPicCpbParamsPresentFlag( ( numDU > 1 ) );

  if( hrd->getSubPicCpbParamsPresentFlag() )
  {
    hrd->setTickDivisorMinus2( 100 - 2 );                          // 
    hrd->setDuCpbRemovalDelayLengthMinus1( 7 );                    // 8-bit precision ( plus 1 for last DU in AU )
    hrd->setSubPicCpbParamsInPicTimingSEIFlag( true );
    hrd->setDpbOutputDelayDuLengthMinus1( 5 + 7 );                 // With sub-clock tick factor of 100, at least 7 bits to have the same value as AU dpb delay
  }
  else
  {
    hrd->setSubPicCpbParamsInPicTimingSEIFlag( false );  
  }

  hrd->setBitRateScale( 4 );                                       // in units of 2~( 6 + 4 ) = 1,024 bps
  hrd->setCpbSizeScale( 6 );                                       // in units of 2~( 4 + 4 ) = 1,024 bit
  hrd->setDuCpbSizeScale( 6 );                                       // in units of 2~( 4 + 4 ) = 1,024 bit
  
  hrd->setInitialCpbRemovalDelayLengthMinus1(15);                  // assuming 0.5 sec, log2( 90,000 * 0.5 ) = 16-bit
  if( randomAccess )
  {
    hrd->setCpbRemovalDelayLengthMinus1(5);                        // 32 = 2^5 (plus 1)
    hrd->setDpbOutputDelayLengthMinus1 (5);                        // 32 + 3 = 2^6
  }
  else
  {
    hrd->setCpbRemovalDelayLengthMinus1(9);                        // max. 2^10
    hrd->setDpbOutputDelayLengthMinus1 (9);                        // max. 2^10
  }

/*
   Note: only the case of "vps_max_temporal_layers_minus1 = 0" is supported.
*/
  Int i, j;
  UInt birateValue, cpbSizeValue;
  UInt ducpbSizeValue;
  UInt duBitRateValue = 0;

  for( i = 0; i < MAX_TLAYER; i ++ )
  {
    hrd->setFixedPicRateFlag( i, 1 );
    hrd->setPicDurationInTcMinus1( i, 0 );
    hrd->setLowDelayHrdFlag( i, 0 );
    hrd->setCpbCntMinus1( i, 0 );

    birateValue  = bitRate;
    cpbSizeValue = bitRate;                                     // 1 second
    ducpbSizeValue = bitRate/numDU;
    duBitRateValue = bitRate;
    for( j = 0; j < ( hrd->getCpbCntMinus1( i ) + 1 ); j ++ )
    {
      hrd->setBitRateValueMinus1( i, j, 0, ( birateValue  - 1 ) );
      hrd->setCpbSizeValueMinus1( i, j, 0, ( cpbSizeValue - 1 ) );
      hrd->setDuCpbSizeValueMinus1( i, j, 0, ( ducpbSizeValue - 1 ) );
      hrd->setCbrFlag( i, j, 0, ( j == 0 ) );

      hrd->setBitRateValueMinus1( i, j, 1, ( birateValue  - 1) );
      hrd->setCpbSizeValueMinus1( i, j, 1, ( cpbSizeValue - 1 ) );
      hrd->setDuCpbSizeValueMinus1( i, j, 1, ( ducpbSizeValue - 1 ) );
      hrd->setDuBitRateValueMinus1( i, j, 1, ( duBitRateValue - 1 ) );
      hrd->setCbrFlag( i, j, 1, ( j == 0 ) );
    }
  }
}
const Int TComSPS::m_winUnitX[]={1,2,2,1};
const Int TComSPS::m_winUnitY[]={1,2,1,1};

TComPPS::TComPPS()
: m_PPSId                       (0)
, m_SPSId                       (0)
, m_picInitQPMinus26            (0)
, m_useDQP                      (false)
, m_bConstrainedIntraPred       (false)
, m_bSliceChromaQpFlag          (false)
, m_pcSPS                       (NULL)
, m_uiMaxCuDQPDepth             (0)
, m_uiMinCuDQPSize              (0)
, m_chromaCbQpOffset            (0)
, m_chromaCrQpOffset            (0)
, m_numRefIdxL0DefaultActive    (1)
, m_numRefIdxL1DefaultActive    (1)
, m_TransquantBypassEnableFlag  (false)
, m_useTransformSkip             (false)
, m_dependentSliceSegmentsEnabledFlag    (false)
, m_tilesEnabledFlag               (false)
, m_entropyCodingSyncEnabledFlag   (false)
, m_loopFilterAcrossTilesEnabledFlag  (true)
, m_uniformSpacingFlag           (0)
, m_iNumColumnsMinus1            (0)
, m_puiColumnWidth               (NULL)
, m_iNumRowsMinus1               (0)
, m_puiRowHeight                 (NULL)
, m_iNumSubstreams             (1)
, m_signHideFlag(0)
, m_cabacInitPresentFlag        (false)
, m_encCABACTableIdx            (I_SLICE)
, m_sliceHeaderExtensionPresentFlag    (false)
, m_loopFilterAcrossSlicesEnabledFlag (false)
, m_listsModificationPresentFlag(  0)
, m_numExtraSliceHeaderBits(0)
#if H_MV
, m_ppsInferScalingListFlag(false)
, m_ppsScalingListRefLayerId(0)
, m_pocResetInfoPresentFlag(false)
#endif
{
  m_scalingList = new TComScalingList;

#if H_MV
  m_ppsRangeExtensionsFlag     = false;
  m_ppsMultilayerExtensionFlag = false;
  m_ppsExtension6bits          = 0;
#endif
}

TComPPS::~TComPPS()
{
  if( m_iNumColumnsMinus1 > 0 && m_uniformSpacingFlag == 0 )
  {
    if (m_puiColumnWidth) delete [] m_puiColumnWidth; 
    m_puiColumnWidth = NULL;
  }
  if( m_iNumRowsMinus1 > 0 && m_uniformSpacingFlag == 0 )
  {
    if (m_puiRowHeight) delete [] m_puiRowHeight;
    m_puiRowHeight = NULL;
  }
  delete m_scalingList;
}


#if H_MV
Void TComSPS::inferRepFormat( TComVPS* vps, Int layerIdCurr )
{
#if H_MV_HLS10_MULTILAYERSPS
  if ( getMultiLayerExtSpsFlag() )
#else
  if ( layerIdCurr > 0 )
#endif
  { 
    Int            repFormatIdx = getUpdateRepFormatFlag() ?  getSpsRepFormatIdx() : vps->getVpsRepFormatIdx( vps->getLayerIdInVps( layerIdCurr ) ) ;
    TComRepFormat* repFormat    = vps->getRepFormat( repFormatIdx ); 
    setChromaFormatIdc( repFormat->getChromaFormatVpsIdc() );         
    //// ToDo: add when supported: 
    // setSeperateColourPlaneFlag( repFormat->getSeparateColourPlaneVpsFlag() ) ; 

    setPicWidthInLumaSamples ( repFormat->getPicWidthVpsInLumaSamples()  ); 
    setPicHeightInLumaSamples( repFormat->getPicHeightVpsInLumaSamples() ); 

    setBitDepthY             ( repFormat->getBitDepthVpsLumaMinus8()   + 8 ); 
    setQpBDOffsetY           ( (Int) (6*( getBitDepthY() - 8 )) );

    setBitDepthC             ( repFormat->getBitDepthVpsChromaMinus8() + 8 ); 
    setQpBDOffsetC           ( (Int) (6* ( getBitDepthC() -8 ) ) );
 #if H_MV_HLS10_GEN_VSP_CONF_WIN
    Window &spsConf    = getConformanceWindow();    

    // Scaled later
    spsConf.setScaledFlag( false ); 
    spsConf.setWindowLeftOffset  ( repFormat->getConfWinVpsLeftOffset()    );
    spsConf.setWindowRightOffset ( repFormat->getConfWinVpsRightOffset()   );
    spsConf.setWindowTopOffset   ( repFormat->getConfWinVpsTopOffset()     );
    spsConf.setWindowBottomOffset( repFormat->getConfWinVpsBottomOffset()  );    
#endif

#if H_MV_HLS10_MULTILAYERSPS
   if ( getMultiLayerExtSpsFlag() && getUpdateRepFormatFlag() )
#else 
   if ( getLayerId() > 0 && getUpdateRepFormatFlag() )
#endif
    {
      assert( getChromaFormatIdc()      <=  repFormat->getChromaFormatVpsIdc()         ); 
      //// ToDo: add when supported: 
      // assert( getSeperateColourPlaneFlag() <=  repFormat->getSeparateColourPlaneVpsFlag() ) ; 

      assert( getPicWidthInLumaSamples()  <= repFormat->getPicWidthVpsInLumaSamples()    ); 
      assert( getPicHeightInLumaSamples() <= repFormat->getPicHeightVpsInLumaSamples()   ); 

      assert( getBitDepthY()              <= repFormat->getBitDepthVpsLumaMinus8()   + 8 );         
      assert( getBitDepthC()              <= repFormat->getBitDepthVpsChromaMinus8() + 8 ); 
    }
  }

  // Set conformance window
  Int scal = TComSPS::getWinUnitX( getChromaFormatIdc() ) ;
  getConformanceWindow().scaleOffsets( scal );
  getVuiParameters()->getDefaultDisplayWindow().scaleOffsets( scal );
}

Void TComSPS::inferScalingList( TComSPS* spsSrc )
{
  if ( getSpsInferScalingListFlag() ) 
  {
    assert( spsSrc != NULL ); 
    assert( !spsSrc->getSpsInferScalingListFlag() );             
    getScalingList()->inferFrom( spsSrc->getScalingList() ); 
  }
}

Void TComSPS::inferSpsMaxDecPicBufferingMinus1( TComVPS* vps, Int targetOptLayerSetIdx, Int currLayerId, Bool encoder )
{
  const std::vector<Int>& targetDecLayerIdList = vps->getTargetDecLayerIdList( vps->olsIdxToLsIdx( targetOptLayerSetIdx )); 

#if H_MV_HLS10_MULTILAYERSPS
  if ( getMultiLayerExtSpsFlag() )
#else
  if (getLayerId() > 0 )
#endif
  {
    Int layerIdx = 0;         
    while (layerIdx < (Int) targetDecLayerIdList.size() )
    {
      if ( targetDecLayerIdList[layerIdx] == currLayerId )
      { 
        break; 
      }
      layerIdx++; 
    }

    assert( layerIdx < (Int) targetDecLayerIdList.size() ); 

    for (Int i = 0; i <= getSpsMaxSubLayersMinus1(); i++ ) 
    {
      Int maxDecPicBufferingMinus1 = vps->getDpbSize()->getMaxVpsDecPicBufferingMinus1( targetOptLayerSetIdx, layerIdx, i ) ; 

#if H_MV_HLS10_MULTILAYERSPS
      // This preliminary fix needs to be checked.
      Int maxNumReorderPics       = vps->getDpbSize()->getMaxVpsNumReorderPics( targetOptLayerSetIdx, i ); 
      Int maxLatencyIncreasePlus1 = vps->getDpbSize()->getMaxVpsLatencyIncreasePlus1( targetOptLayerSetIdx, i ); 
#endif
      if ( encoder )      
      {
        assert( getMaxDecPicBuffering( i ) - 1 == maxDecPicBufferingMinus1 ); 
#if H_MV_HLS10_MULTILAYERSPS
        // This preliminary fix needs to be checked.
        assert( getNumReorderPics( i )     == maxNumReorderPics       ); 
        assert( getMaxLatencyIncrease( i ) == maxLatencyIncreasePlus1 ); 
#endif    

      }
      else
      {
#if !H_MV_HLS10_MULTILAYERSPS
        setMaxDecPicBuffering(i, maxDecPicBufferingMinus1 + 1 ); 
#else
        // This preliminary fix needs to be checked.
        setMaxDecPicBuffering( maxDecPicBufferingMinus1 + 1 , i); 
        setNumReorderPics    ( maxNumReorderPics, i );
        setMaxLatencyIncrease( maxLatencyIncreasePlus1 - 1 , i); 
#endif
      }
    }    
  }
}

Void TComSPS::checkRpsMaxNumPics( TComVPS* vps, Int currLayerId )
{
#if !H_MV_HLS10_MAXNUMPICS
  // In spec, when rps is in SPS, nuh_layer_id of SPS is used instead 
  // of nuh_layer_id of slice (currLayerId), this seems to be a bug.
#endif
  for (Int i = 0; i < getRPSList()->getNumberOfReferencePictureSets(); i++ )
  {
    TComReferencePictureSet* rps = getRPSList()->getReferencePictureSet( i ); 
    if ( !rps->getInterRPSPrediction() )
    {
#if H_MV_HLS10_MAXNUMPICS
      rps->checkMaxNumPics( vps->getVpsExtensionFlag(), MAX_INT, getLayerId(), getMaxDecPicBuffering( getSpsMaxSubLayersMinus1() ) - 1 );   // INT_MAX to be replaced by DpbSize
#else
      rps->checkMaxNumPics( vps->getVpsExtensionFlag(), vps->getMaxNumPics( currLayerId ), 
        getLayerId(), getMaxDecPicBuffering( getSpsMaxSubLayersMinus1() ) - 1 );  
#endif
    }
  }
}

#if H_MV_HLS10_MULTILAYERSPS
Void TComSPS::inferSpsMaxSubLayersMinus1(Bool atPsActivation, TComVPS* vps)
{
  assert( getLayerId() != 0 ); 
  if ( !atPsActivation   )
  {
    assert( vps == NULL );
    if (getSpsExtOrMaxSubLayersMinus1() != 7)
    {
      setSpsMaxSubLayersMinus1( getSpsExtOrMaxSubLayersMinus1() );
    }
  }
  else
  {
    assert( vps != NULL );
    if (getSpsExtOrMaxSubLayersMinus1() == 7)
    {
      setSpsMaxSubLayersMinus1( vps->getMaxSubLayersMinus1() );
    }
  }
}
#endif
#endif

TComReferencePictureSet::TComReferencePictureSet()
: m_numberOfPictures (0)
, m_numberOfNegativePictures (0)
, m_numberOfPositivePictures (0)
, m_numberOfLongtermPictures (0)
, m_interRPSPrediction (0) 
, m_deltaRIdxMinus1 (0)   
, m_deltaRPS (0) 
, m_numRefIdc (0) 
{
  ::memset( m_deltaPOC, 0, sizeof(m_deltaPOC) );
  ::memset( m_POC, 0, sizeof(m_POC) );
  ::memset( m_used, 0, sizeof(m_used) );
  ::memset( m_refIdc, 0, sizeof(m_refIdc) );
}

TComReferencePictureSet::~TComReferencePictureSet()
{
}

Void TComReferencePictureSet::setUsed(Int bufferNum, Bool used)
{
  m_used[bufferNum] = used;
}

Void TComReferencePictureSet::setDeltaPOC(Int bufferNum, Int deltaPOC)
{
  m_deltaPOC[bufferNum] = deltaPOC;
}

Void TComReferencePictureSet::setNumberOfPictures(Int numberOfPictures)
{
  m_numberOfPictures = numberOfPictures;
}

Int TComReferencePictureSet::getUsed(Int bufferNum)
{
  return m_used[bufferNum];
}

Int TComReferencePictureSet::getDeltaPOC(Int bufferNum)
{
  return m_deltaPOC[bufferNum];
}

Int TComReferencePictureSet::getNumberOfPictures()
{
  return m_numberOfPictures;
}

Int TComReferencePictureSet::getPOC(Int bufferNum)
{
  return m_POC[bufferNum];
}

Void TComReferencePictureSet::setPOC(Int bufferNum, Int POC)
{
  m_POC[bufferNum] = POC;
}

Bool TComReferencePictureSet::getCheckLTMSBPresent(Int bufferNum)
{
  return m_bCheckLTMSB[bufferNum];
}

Void TComReferencePictureSet::setCheckLTMSBPresent(Int bufferNum, Bool b)
{
  m_bCheckLTMSB[bufferNum] = b;
}

/** set the reference idc value at uiBufferNum entry to the value of iRefIdc
 * \param uiBufferNum
 * \param iRefIdc
 * \returns Void
 */
Void TComReferencePictureSet::setRefIdc(Int bufferNum, Int refIdc)
{
  m_refIdc[bufferNum] = refIdc;
}

/** get the reference idc value at uiBufferNum
 * \param uiBufferNum
 * \returns Int
 */
Int  TComReferencePictureSet::getRefIdc(Int bufferNum)
{
  return m_refIdc[bufferNum];
}

/** Sorts the deltaPOC and Used by current values in the RPS based on the deltaPOC values.
 *  deltaPOC values are sorted with -ve values before the +ve values.  -ve values are in decreasing order.
 *  +ve values are in increasing order.
 * \returns Void
 */
Void TComReferencePictureSet::sortDeltaPOC()
{
  // sort in increasing order (smallest first)
  for(Int j=1; j < getNumberOfPictures(); j++)
  { 
    Int deltaPOC = getDeltaPOC(j);
    Bool used = getUsed(j);
    for (Int k=j-1; k >= 0; k--)
    {
      Int temp = getDeltaPOC(k);
      if (deltaPOC < temp)
      {
        setDeltaPOC(k+1, temp);
        setUsed(k+1, getUsed(k));
        setDeltaPOC(k, deltaPOC);
        setUsed(k, used);
      }
    }
  }
  // flip the negative values to largest first
  Int numNegPics = getNumberOfNegativePictures();
  for(Int j=0, k=numNegPics-1; j < numNegPics>>1; j++, k--)
  { 
    Int deltaPOC = getDeltaPOC(j);
    Bool used = getUsed(j);
    setDeltaPOC(j, getDeltaPOC(k));
    setUsed(j, getUsed(k));
    setDeltaPOC(k, deltaPOC);
    setUsed(k, used);
  }
}

/** Prints the deltaPOC and RefIdc (if available) values in the RPS.
 *  A "*" is added to the deltaPOC value if it is Used bu current.
 * \returns Void
 */
Void TComReferencePictureSet::printDeltaPOC()
{
  printf("DeltaPOC = { ");
  for(Int j=0; j < getNumberOfPictures(); j++)
  {
    printf("%d%s ", getDeltaPOC(j), (getUsed(j)==1)?"*":"");
  } 
  if (getInterRPSPrediction()) 
  {
    printf("}, RefIdc = { ");
    for(Int j=0; j < getNumRefIdc(); j++)
    {
      printf("%d ", getRefIdc(j));
    } 
  }
  printf("}\n");
}
#if H_MV
Void TComReferencePictureSet::checkMaxNumPics( Bool vpsExtensionFlag, Int maxNumPics, Int nuhLayerId, Int spsMaxDecPicBufferingMinus1 )
{
  assert( getNumberOfPictures() >= 0 ); 
  if ( nuhLayerId == 0 )
  {
    assert( getNumberOfPictures() <= spsMaxDecPicBufferingMinus1 ); 
  }

  if ( vpsExtensionFlag )
  {
    assert( getNumberOfPictures() <= maxNumPics );
  }
}
#endif

TComRPSList::TComRPSList()
:m_referencePictureSets (NULL)
{
}

TComRPSList::~TComRPSList()
{
}

Void TComRPSList::create( Int numberOfReferencePictureSets)
{
  m_numberOfReferencePictureSets = numberOfReferencePictureSets;
  m_referencePictureSets = new TComReferencePictureSet[numberOfReferencePictureSets];
}

Void TComRPSList::destroy()
{
  if (m_referencePictureSets)
  {
    delete [] m_referencePictureSets;
  }
  m_numberOfReferencePictureSets = 0;
  m_referencePictureSets = NULL;
}



TComReferencePictureSet* TComRPSList::getReferencePictureSet(Int referencePictureSetNum)
{
  return &m_referencePictureSets[referencePictureSetNum];
}

Int TComRPSList::getNumberOfReferencePictureSets()
{
  return m_numberOfReferencePictureSets;
}

Void TComRPSList::setNumberOfReferencePictureSets(Int numberOfReferencePictureSets)
{
  m_numberOfReferencePictureSets = numberOfReferencePictureSets;
}

TComRefPicListModification::TComRefPicListModification()
: m_bRefPicListModificationFlagL0 (false)
, m_bRefPicListModificationFlagL1 (false)
{
  ::memset( m_RefPicSetIdxL0, 0, sizeof(m_RefPicSetIdxL0) );
  ::memset( m_RefPicSetIdxL1, 0, sizeof(m_RefPicSetIdxL1) );
}

TComRefPicListModification::~TComRefPicListModification()
{
}

TComScalingList::TComScalingList()
{
  init();
}
TComScalingList::~TComScalingList()
{
  destroy();
}

/** set default quantization matrix to array
*/
Void TComSlice::setDefaultScalingList()
{
  for(UInt sizeId = 0; sizeId < SCALING_LIST_SIZE_NUM; sizeId++)
  {
    for(UInt listId=0;listId<g_scalingListNum[sizeId];listId++)
    {
      getScalingList()->processDefaultMatrix(sizeId, listId);
    }
  }
}
/** check if use default quantization matrix
 * \returns true if use default quantization matrix in all size
*/
Bool TComSlice::checkDefaultScalingList()
{
  UInt defaultCounter=0;

  for(UInt sizeId = 0; sizeId < SCALING_LIST_SIZE_NUM; sizeId++)
  {
    for(UInt listId=0;listId<g_scalingListNum[sizeId];listId++)
    {
      if( !memcmp(getScalingList()->getScalingListAddress(sizeId,listId), getScalingList()->getScalingListDefaultAddress(sizeId, listId),sizeof(Int)*min(MAX_MATRIX_COEF_NUM,(Int)g_scalingListSize[sizeId])) // check value of matrix
     && ((sizeId < SCALING_LIST_16x16) || (getScalingList()->getScalingListDC(sizeId,listId) == 16))) // check DC value
      {
        defaultCounter++;
      }
    }
  }
  return (defaultCounter == (SCALING_LIST_NUM * SCALING_LIST_SIZE_NUM - 4)) ? false : true; // -4 for 32x32
}

#if H_MV
Void TComSlice::createInterLayerReferencePictureSet( TComPicLists* ivPicLists, std::vector<TComPic*>& refPicSetInterLayer0, std::vector<TComPic*>& refPicSetInterLayer1 )
{
  refPicSetInterLayer0.clear(); 
  refPicSetInterLayer1.clear(); 

  for( Int i = 0; i < getNumActiveRefLayerPics(); i++ ) 
  {
    Int layerIdRef = getRefPicLayerId( i ); 
    TComPic* picRef = ivPicLists->getPic( layerIdRef, getPOC() ) ; 
    assert ( picRef != 0 ); // There shall be no entry equal to "no reference picture" in RefPicSetInterLayer0 or RefPicSetInterLayer1.

    picRef->getPicYuvRec()->extendPicBorder(); 
    picRef->setIsLongTerm( true );        
    picRef->getSlice(0)->setReferenced( true );       

    Int viewIdCur  = getVPS()->getViewId( getLayerId() ); 
    Int viewIdZero = getVPS()->getViewId( 0 );
    Int viewIdRef  = getVPS()->getViewId( layerIdRef ); 

    if (  ( viewIdCur <= viewIdZero && viewIdCur <= viewIdRef ) || ( viewIdCur >= viewIdZero && viewIdCur >= viewIdRef ) )
    {
      refPicSetInterLayer0.push_back( picRef ); 
    }
    else
    {
      refPicSetInterLayer1.push_back( picRef ); 
    }
    // Consider to check here: 
    // "If the current picture is a RADL picture, there shall be no entry in the RefPicSetInterLayer0 and RefPicSetInterLayer1 that is a RASL picture. "    
    assert( picRef->getSlice(0)->getDiscardableFlag() == false ); // "There shall be no picture that has discardable_flag equal to 1 in RefPicSetInterLayer0 or RefPicSetInterLayer1".        
  }
}

Void TComSlice::markIvRefPicsAsShortTerm( std::vector<TComPic*> refPicSetInterLayer0, std::vector<TComPic*> refPicSetInterLayer1 )
{
  // Mark as short-term 
  for ( Int i = 0; i < refPicSetInterLayer0.size(); i++ ) 
  {
    refPicSetInterLayer0[i]->setIsLongTerm( false ); 
  }

  for ( Int i = 0; i < refPicSetInterLayer1.size(); i++ ) 
  {
    refPicSetInterLayer1[i]->setIsLongTerm( false ); 
  }

}

Void TComSlice::printRefPicList()
{  
  for ( Int li = 0; li < 2; li++)
  {    
    std::cout << std::endl << "RefPicListL" <<  li << ":" << std::endl; 
    for (Int rIdx = 0; rIdx <= (m_aiNumRefIdx[li]-1); rIdx ++)
    {      
      if (rIdx == 0 && li == 0) m_apcRefPicList[li][rIdx]->print( true );
        
      m_apcRefPicList[li][rIdx]->print( false );
    }
  }
}

Void TComSlice::markCurrPic( TComPic* currPic )
{
  currPic->getSlice(0)->setReferenced( true ) ; 
  currPic->setIsLongTerm( false ); 

  currPic->setReconMark( true );
  currPic->setPicOutputFlag( currPic->getSlice(0)->getPicOutputFlag() );
}

Void TComSlice::setRefPicSetInterLayer( std::vector<TComPic*>* refPicSetInterLayer0, std::vector<TComPic*>* refPicSetInterLayer1 )
{
  m_refPicSetInterLayer0 = refPicSetInterLayer0; 
  m_refPicSetInterLayer1 = refPicSetInterLayer1; 
}

TComPic* TComSlice::getPicFromRefPicSetInterLayer(Int setIdc, Int layerId )
{
  assert ( setIdc == 0 || setIdc == 1);   
  std::vector<TComPic*>* refPicSetInterLayer = ( setIdc == 0 ? m_refPicSetInterLayer0 : m_refPicSetInterLayer1);   
  assert( refPicSetInterLayer != 0 ); 
  
  TComPic* pcPic = NULL; 
  for ( Int i = 0; i < (*refPicSetInterLayer).size(); i++ )
  {
    if ((*refPicSetInterLayer)[ i ]->getLayerId() == layerId )
    {
      pcPic = (*refPicSetInterLayer)[ i ]; 
    }
  }

  assert(pcPic != NULL); 
  return pcPic;
}


Int  TComSlice::getRefLayerPicFlag( Int i ) 
{
  TComVPS* vps = getVPS(); 
#if H_MV_HLS10_REF_PRED_LAYERS
  Int refLayerIdx = vps->getLayerIdInVps( vps->getIdDirectRefLayer( getLayerId(), i ) ); 
#else
  Int refLayerIdx = vps->getLayerIdInVps( vps->getRefLayerId( getLayerId(), i ) ); 
#endif

  Bool refLayerPicFlag = ( vps->getSubLayersVpsMaxMinus1( refLayerIdx ) >=  getTLayer() )  && ( getTLayer() == 0  ) &&
    ( vps->getMaxTidIlRefPicsPlus1( refLayerIdx, vps->getLayerIdInVps( getLayerId() )) > getTLayer() ); 

  return refLayerPicFlag;       
}    

Int TComSlice::getRefLayerPicIdc( Int j ) 
{  
  Int refLayerPicIdc = -1; 
  Int curj = 0; 
  for( Int i = 0;  i < getVPS()->getNumDirectRefLayers( getLayerId()) ; i++ )
  {
    if( getRefLayerPicFlag( i ) )
    {
      if ( curj == j ) 
      {
        refLayerPicIdc = i;         
        break;
      }
      curj++; 
    }
  }

  assert( curj == j ); 
  assert( refLayerPicIdc != -1 ); 
  return refLayerPicIdc; 
}

Int  TComSlice::getNumRefLayerPics( )
{  
  Int numRefLayerPics = 0; 
  for( Int i = 0;  i < getVPS()->getNumDirectRefLayers( getLayerId()) ; i++ )
  {
    numRefLayerPics += getRefLayerPicFlag( i ); 
  }
  return numRefLayerPics; 
}



Int TComSlice::getNumActiveRefLayerPics()
{
  Int numActiveRefLayerPics; 

  if( getLayerId() == 0 || getNumRefLayerPics() ==  0 )
  {
    numActiveRefLayerPics = 0; 
  }
  else if (getVPS()->getAllRefLayersActiveFlag() )
  {
    numActiveRefLayerPics = getNumRefLayerPics(); 
  }
  else if ( !getInterLayerPredEnabledFlag() )
  {
    numActiveRefLayerPics = 0; 
  }
  else if( getVPS()->getMaxOneActiveRefLayerFlag() || getVPS()->getNumDirectRefLayers( getLayerId() ) == 1 )
  {
    numActiveRefLayerPics = 1; 
  }
  else
  {
    numActiveRefLayerPics = getNumInterLayerRefPicsMinus1() + 1; 
  }
  return numActiveRefLayerPics;
}

Int TComSlice::getRefPicLayerId( Int i )
{
#if H_MV_HLS10_REF_PRED_LAYERS
  return getVPS()->getIdDirectRefLayer( getLayerId(), getInterLayerPredLayerIdc( i ) );
#else
  return getVPS()->getRefLayerId( getLayerId(), getInterLayerPredLayerIdc( i ) );
#endif
}

#endif


#if H_MV
Void TComSlice::checkCrossLayerBlaFlag()
{
  // cross_layer_bla_flag shall be equal to 0 for pictures with nal_unit_type not equal to IDR_W_RADL or IDR_N_LP or with nuh_layer_id not equal to 0.
  if ( getLayerId() != 0 || getNalUnitType() != NAL_UNIT_CODED_SLICE_IDR_W_RADL || getNalUnitType() == NAL_UNIT_CODED_SLICE_IDR_N_LP )
  {
    assert( m_crossLayerBlaFlag == 0 ); 
  }
}

Bool TComSlice::inferPocMsbValPresentFlag()
{
  Bool pocMsbValPresentFlag; 
  if( getSliceSegmentHeaderExtensionLength() == 0 ) 
  {
    pocMsbValPresentFlag = false; 
  }
  else if ( getPocMsbValRequiredFlag() )
  {
    pocMsbValPresentFlag = true; 
  }
  else
  {
    pocMsbValPresentFlag = false; 
  }

  return pocMsbValPresentFlag;
}


#endif


/** get scaling matrix from RefMatrixID
 * \param sizeId size index
 * \param Index of input matrix
 * \param Index of reference matrix
 */
Void TComScalingList::processRefMatrix( UInt sizeId, UInt listId , UInt refListId )
{
  ::memcpy(getScalingListAddress(sizeId, listId),((listId == refListId)? getScalingListDefaultAddress(sizeId, refListId): getScalingListAddress(sizeId, refListId)),sizeof(Int)*min(MAX_MATRIX_COEF_NUM,(Int)g_scalingListSize[sizeId]));
}

/** parse syntax infomation 
 *  \param pchFile syntax infomation
 *  \returns false if successful
 */
Bool TComScalingList::xParseScalingList(Char* pchFile)
{
  FILE *fp;
  Char line[1024];
  UInt sizeIdc,listIdc;
  UInt i,size = 0;
  Int *src=0,data;
  Char *ret;
  UInt  retval;

  if((fp = fopen(pchFile,"r")) == (FILE*)NULL)
  {
    printf("can't open file %s :: set Default Matrix\n",pchFile);
    return true;
  }

  for(sizeIdc = 0; sizeIdc < SCALING_LIST_SIZE_NUM; sizeIdc++)
  {
    size = min(MAX_MATRIX_COEF_NUM,(Int)g_scalingListSize[sizeIdc]);
    for(listIdc = 0; listIdc < g_scalingListNum[sizeIdc]; listIdc++)
    {
      src = getScalingListAddress(sizeIdc, listIdc);

      fseek(fp,0,0);
      do 
      {
        ret = fgets(line, 1024, fp);
        if ((ret==NULL)||(strstr(line, MatrixType[sizeIdc][listIdc])==NULL && feof(fp)))
        {
          printf("Error: can't read Matrix :: set Default Matrix\n");
          return true;
        }
      }
      while (strstr(line, MatrixType[sizeIdc][listIdc]) == NULL);
      for (i=0; i<size; i++)
      {
        retval = fscanf(fp, "%d,", &data);
        if (retval!=1)
        {
          printf("Error: can't read Matrix :: set Default Matrix\n");
          return true;
        }
        src[i] = data;
      }
      //set DC value for default matrix check
      setScalingListDC(sizeIdc,listIdc,src[0]);

      if(sizeIdc > SCALING_LIST_8x8)
      {
        fseek(fp,0,0);
        do 
        {
          ret = fgets(line, 1024, fp);
          if ((ret==NULL)||(strstr(line, MatrixType_DC[sizeIdc][listIdc])==NULL && feof(fp)))
          {
            printf("Error: can't read DC :: set Default Matrix\n");
            return true;
          }
        }
        while (strstr(line, MatrixType_DC[sizeIdc][listIdc]) == NULL);
        retval = fscanf(fp, "%d,", &data);
        if (retval!=1)
        {
          printf("Error: can't read Matrix :: set Default Matrix\n");
          return true;
        }
        //overwrite DC value when size of matrix is larger than 16x16
        setScalingListDC(sizeIdc,listIdc,data);
      }
    }
  }
  fclose(fp);
  return false;
}

#if H_MV
Void TComScalingList::inferFrom( TComScalingList* srcScLi )
{
  for(Int sizeId = 0; sizeId < SCALING_LIST_SIZE_NUM; sizeId++)
  {
    for(Int listId = 0; listId <  g_scalingListNum[sizeId]; listId++)
    {
      setRefMatrixId  (sizeId,listId, srcScLi->getRefMatrixId  (sizeId,listId));
      setScalingListDC(sizeId,listId, srcScLi->getScalingListDC(sizeId,listId));          
      ::memcpy(getScalingListAddress(sizeId, listId),srcScLi->getScalingListAddress(sizeId, listId),sizeof(Int)*min(MAX_MATRIX_COEF_NUM,(Int)g_scalingListSize[sizeId]));
    }
  }
}
#endif
/** initialization process of quantization matrix array
 */
Void TComScalingList::init()
{
  for(UInt sizeId = 0; sizeId < SCALING_LIST_SIZE_NUM; sizeId++)
  {
    for(UInt listId = 0; listId < g_scalingListNum[sizeId]; listId++)
    {
      m_scalingListCoef[sizeId][listId] = new Int [min(MAX_MATRIX_COEF_NUM,(Int)g_scalingListSize[sizeId])];
    }
  }
  m_scalingListCoef[SCALING_LIST_32x32][3] = m_scalingListCoef[SCALING_LIST_32x32][1]; // copy address for 32x32
}

/** destroy quantization matrix array
 */
Void TComScalingList::destroy()
{
  for(UInt sizeId = 0; sizeId < SCALING_LIST_SIZE_NUM; sizeId++)
  {
    for(UInt listId = 0; listId < g_scalingListNum[sizeId]; listId++)
    {
      if(m_scalingListCoef[sizeId][listId]) delete [] m_scalingListCoef[sizeId][listId];
    }
  }
}

/** get default address of quantization matrix 
 * \param sizeId size index
 * \param listId list index
 * \returns pointer of quantization matrix
 */
Int* TComScalingList::getScalingListDefaultAddress(UInt sizeId, UInt listId)
{
  Int *src = 0;
  switch(sizeId)
  {
    case SCALING_LIST_4x4:
      src = g_quantTSDefault4x4;
      break;
    case SCALING_LIST_8x8:
      src = (listId<3) ? g_quantIntraDefault8x8 : g_quantInterDefault8x8;
      break;
    case SCALING_LIST_16x16:
      src = (listId<3) ? g_quantIntraDefault8x8 : g_quantInterDefault8x8;
      break;
    case SCALING_LIST_32x32:
      src = (listId<1) ? g_quantIntraDefault8x8 : g_quantInterDefault8x8;
      break;
    default:
      assert(0);
      src = NULL;
      break;
  }
  return src;
}

/** process of default matrix
 * \param sizeId size index
 * \param Index of input matrix
 */
Void TComScalingList::processDefaultMatrix(UInt sizeId, UInt listId)
{
  ::memcpy(getScalingListAddress(sizeId, listId),getScalingListDefaultAddress(sizeId,listId),sizeof(Int)*min(MAX_MATRIX_COEF_NUM,(Int)g_scalingListSize[sizeId]));
  setScalingListDC(sizeId,listId,SCALING_LIST_DC);
}

/** check DC value of matrix for default matrix signaling
 */
Void TComScalingList::checkDcOfMatrix()
{
  for(UInt sizeId = 0; sizeId < SCALING_LIST_SIZE_NUM; sizeId++)
  {
    for(UInt listId = 0; listId < g_scalingListNum[sizeId]; listId++)
    {
      //check default matrix?
      if(getScalingListDC(sizeId,listId) == 0)
      {
        processDefaultMatrix(sizeId, listId);
      }
    }
  }
}

ParameterSetManager::ParameterSetManager()
: m_vpsMap(MAX_NUM_VPS)
, m_spsMap(MAX_NUM_SPS)
, m_ppsMap(MAX_NUM_PPS)
, m_activeVPSId(-1)
#if !H_MV
, m_activeSPSId(-1)
, m_activePPSId(-1)
{
#else
{
  for (Int i = 0; i < MAX_NUM_LAYERS; i++ )
  {
    m_activeSPSId[ i ] = -1; 
    m_activePPSId[ i ] = -1; 
  }
#endif
}


ParameterSetManager::~ParameterSetManager()
{
}

//! activate a SPS from a active parameter sets SEI message
//! \returns true, if activation is successful
#if H_MV
Bool ParameterSetManager::activateSPSWithSEI(Int spsId, Int layerId )
#else
Bool ParameterSetManager::activateSPSWithSEI(Int spsId)
#endif
{
  TComSPS *sps = m_spsMap.getPS(spsId);
  if (sps)
  {
    Int vpsId = sps->getVPSId();
    if (m_vpsMap.getPS(vpsId))
    {
      m_activeVPSId = vpsId;
#if H_MV
      m_activeSPSId[ layerId ] = spsId;
#else
      m_activeSPSId = spsId;
#endif
      return true;
    }
    else
    {
      printf("Warning: tried to activate SPS using an Active parameter sets SEI message. Referenced VPS does not exist.");
    }
  }
  else
  {
    printf("Warning: tried to activate non-existing SPS using an Active parameter sets SEI message.");
  }
  return false;
}

//! activate a PPS and depending on isIDR parameter also SPS and VPS
//! \returns true, if activation is successful
#if H_MV
Bool ParameterSetManager::activatePPS(Int ppsId, Bool isIRAP, Int layerId )
#else
Bool ParameterSetManager::activatePPS(Int ppsId, Bool isIRAP)
#endif
{
  TComPPS *pps = m_ppsMap.getPS(ppsId);
  if (pps)
  {
    Int spsId = pps->getSPSId();
#if H_MV
    if (!isIRAP && (spsId != m_activeSPSId[ layerId ]))
#else
    if (!isIRAP && (spsId != m_activeSPSId))
#endif
    {
      printf("Warning: tried to activate PPS referring to a inactive SPS at non-IRAP.");
      return false;
    }

    TComSPS *sps = m_spsMap.getPS(spsId);
    if (sps)
    {
      Int vpsId = sps->getVPSId();
      if (!isIRAP && (vpsId != m_activeVPSId))
      {
        printf("Warning: tried to activate PPS referring to a inactive VPS at non-IRAP.");
        return false;
      }
      if (m_vpsMap.getPS(vpsId))
      {
#if H_MV
        m_activePPSId[ layerId ] = ppsId;
        m_activeVPSId = vpsId;
        m_activeSPSId[ layerId ] = spsId;
#else
        m_activePPSId = ppsId;
        m_activeVPSId = vpsId;
        m_activeSPSId = spsId;
#endif
        return true;
      }
      else
      {
        printf("Warning: tried to activate PPS that refers to a non-existing VPS.");
      }
    }
    else
    {
      printf("Warning: tried to activate a PPS that refers to a non-existing SPS.");
    }
  }
  else
  {
    printf("Warning: tried to activate non-existing PPS.");
  }
  return false;
}

ProfileTierLevel::ProfileTierLevel()
  : m_profileSpace    (0)
  , m_tierFlag        (false)
  , m_profileIdc      (0)
  , m_levelIdc        (0)
, m_progressiveSourceFlag  (false)
, m_interlacedSourceFlag   (false)
, m_nonPackedConstraintFlag(false)
, m_frameOnlyConstraintFlag(false)
#if H_MV_HLS10_PTL
  , m_max12bitConstraintFlag      ( false )
  , m_max10bitConstraintFlag      ( false )
  , m_max8bitConstraintFlag       ( false )
  , m_max422chromaConstraintFlag  ( false )
  , m_max420chromaConstraintFlag  ( false )
  , m_maxMonochromeConstraintFlag ( false )
  , m_intraConstraintFlag         ( false )
  , m_onePictureOnlyConstraintFlag( false )
  , m_lowerBitRateConstraintFlag  ( false )
  , m_inbldFlag                   ( false )
#endif
{
  ::memset(m_profileCompatibilityFlag, 0, sizeof(m_profileCompatibilityFlag));
}

#if H_MV_HLS10_PTL_INFER_FIX
Bool ProfileTierLevel::getV2ConstraintsPresentFlag()
{
  return ( 
    getProfileIdc( ) ==  4 || getProfileCompatibilityFlag( 4 ) || getProfileIdc( ) ==  5 || getProfileCompatibilityFlag( 5 )  ||
    getProfileIdc( ) ==  6 || getProfileCompatibilityFlag( 6 ) || getProfileIdc( ) ==  7 || getProfileCompatibilityFlag( 7 ) 
    );
}

Bool ProfileTierLevel::getInbldPresentFlag()
{
  return ( 
    ( getProfileIdc() >= 1 && getProfileIdc() <= 5 )  || getProfileCompatibilityFlag( 1 ) || getProfileCompatibilityFlag( 2 ) || 
    getProfileCompatibilityFlag( 3 ) || getProfileCompatibilityFlag( 4 )  ||   getProfileCompatibilityFlag( 5 ) 
    );
}

Void ProfileTierLevel::copyV2ConstraintFlags(ProfileTierLevel* ptlRef)
{
  setMax12bitConstraintFlag         ( ptlRef->getMax12bitConstraintFlag       ( ) );
  setMax10bitConstraintFlag         ( ptlRef->getMax10bitConstraintFlag       ( ) );
  setMax8bitConstraintFlag          ( ptlRef->getMax8bitConstraintFlag        ( ) );
  setMax422chromaConstraintFlag     ( ptlRef->getMax422chromaConstraintFlag   ( ) );
  setMax420chromaConstraintFlag     ( ptlRef->getMax420chromaConstraintFlag   ( ) );
  setMaxMonochromeConstraintFlag    ( ptlRef->getMaxMonochromeConstraintFlag  ( ) );
  setIntraConstraintFlag            ( ptlRef->getIntraConstraintFlag          ( ) );
  setOnePictureOnlyConstraintFlag   ( ptlRef->getOnePictureOnlyConstraintFlag ( ) );
  setLowerBitRateConstraintFlag     ( ptlRef->getLowerBitRateConstraintFlag   ( ) );
}

Void ProfileTierLevel::copyProfile(ProfileTierLevel* ptlRef)
{
  setProfileSpace            ( ptlRef->getProfileSpace              ( ) );
  setTierFlag                ( ptlRef->getTierFlag                  ( ) );
  setProfileIdc              ( ptlRef->getProfileIdc                ( ) );
  for (Int j = 0; j < 32; j++)
  {      
    setProfileCompatibilityFlag(j, ptlRef->getProfileCompatibilityFlag  ( j ) );            
  }
  setProgressiveSourceFlag   ( ptlRef->getProgressiveSourceFlag     ( ) );
  setInterlacedSourceFlag    ( ptlRef->getInterlacedSourceFlag      ( ) );
  setNonPackedConstraintFlag ( ptlRef->getNonPackedConstraintFlag   ( ) );
  setFrameOnlyConstraintFlag ( ptlRef->getFrameOnlyConstraintFlag   ( ) );
  copyV2ConstraintFlags      ( ptlRef );
}

#endif

TComPTL::TComPTL()
{
  ::memset(m_subLayerProfilePresentFlag, 0, sizeof(m_subLayerProfilePresentFlag));
  ::memset(m_subLayerLevelPresentFlag,   0, sizeof(m_subLayerLevelPresentFlag  ));
}

#if H_MV
#if !H_MV_HLS10_PTL_INFER_FIX
Void TComPTL::copyLevelFrom( TComPTL* source )
{
  getGeneralPTL()->setLevelIdc( source->getGeneralPTL()->getLevelIdc() );
  for( Int subLayer = 0; subLayer < 6; subLayer++ )
  {
    setSubLayerLevelPresentFlag( subLayer, source->getSubLayerLevelPresentFlag( subLayer ) );
    getSubLayerPTL( subLayer )->setLevelIdc( source->getSubLayerPTL( subLayer )->getLevelIdc() );
  }
}
#else
Void TComPTL::inferGeneralValues(Bool profilePresentFlag, Int k, TComPTL* refPTL)
{
  ProfileTierLevel* refProfileTierLevel = NULL; 
  if ( k > 0 )
  {    
    assert( refPTL != NULL);
    refProfileTierLevel = refPTL->getGeneralPTL(); 
  }

  ProfileTierLevel* curProfileTierLevel = getGeneralPTL( ); 

  if( !profilePresentFlag )
  {
    assert( k > 0 ); 
    assert( refProfileTierLevel != NULL ); 
    curProfileTierLevel->copyProfile( refProfileTierLevel);
  }
  else
  {
    if ( !curProfileTierLevel->getV2ConstraintsPresentFlag() )
    {
      curProfileTierLevel->setMax12bitConstraintFlag         ( false );
      curProfileTierLevel->setMax10bitConstraintFlag         ( false );
      curProfileTierLevel->setMax8bitConstraintFlag          ( false );
      curProfileTierLevel->setMax422chromaConstraintFlag     ( false );
      curProfileTierLevel->setMax420chromaConstraintFlag     ( false );
      curProfileTierLevel->setMaxMonochromeConstraintFlag    ( false );
      curProfileTierLevel->setIntraConstraintFlag            ( false );
      curProfileTierLevel->setOnePictureOnlyConstraintFlag   ( false );
      curProfileTierLevel->setLowerBitRateConstraintFlag     ( false );   
    }

    if ( !curProfileTierLevel->getInbldPresentFlag() )
    {
      curProfileTierLevel->setInbldFlag( false ); 
    }      
  }
}

Void TComPTL::inferSubLayerValues(Int maxNumSubLayersMinus1, Int k, TComPTL* refPTL)
{
  assert( k == 0 || refPTL != NULL ); 

  for (Int i = maxNumSubLayersMinus1; i >= 0; i--)
  {
    ProfileTierLevel* refProfileTierLevel;
    if ( k != 0 )
    {
      refProfileTierLevel = refPTL->getSubLayerPTL( i );
    }
    else
    {
      if ( i == maxNumSubLayersMinus1)      
      {
        refProfileTierLevel = getGeneralPTL();
      }
      else
      {
        refProfileTierLevel = getSubLayerPTL( i + 1 );
      }
    }    

    ProfileTierLevel* curProfileTierLevel = getSubLayerPTL( i ); 
    if( !getSubLayerLevelPresentFlag( i ) )
    {
      curProfileTierLevel->setLevelIdc( refProfileTierLevel->getLevelIdc() ); 
    }

    if( !getSubLayerProfilePresentFlag( i ) )
    {
      curProfileTierLevel->copyProfile( refProfileTierLevel);
    }
    else
    {
      if ( !curProfileTierLevel->getV2ConstraintsPresentFlag() )
      {
        curProfileTierLevel->copyV2ConstraintFlags( refProfileTierLevel ); 
      }

      if ( !curProfileTierLevel->getInbldPresentFlag() )
      {
        curProfileTierLevel->setInbldFlag( refProfileTierLevel->getInbldFlag() ); 
      }      
    }     
  }
}

#endif
#endif
//! \}

#if H_MV
TComVPSVUI::TComVPSVUI()
{
  m_crossLayerIrapAlignedFlag = true; 
  m_allLayersIdrAlignedFlag   = false; 
  m_bitRatePresentVpsFlag = false;
  m_picRatePresentVpsFlag = false;
  for ( Int i = 0; i < MAX_VPS_OP_SETS_PLUS1; i++)
  {    
    for ( Int j = 0; j < MAX_TLAYER; j++)
    {    
      m_bitRatePresentFlag          [i][j] = false;
      m_picRatePresentFlag          [i][j] = false;
      m_avgBitRate                  [i][j] = -1;
      m_maxBitRate                  [i][j] = -1;
      m_constantPicRateIdc          [i][j] = -1;
      m_avgPicRate                  [i][j] = -1;
    }
  }

  m_ilpRestrictedRefLayersFlag = false;

  for ( Int i = 0; i < MAX_NUM_LAYERS; i++)
  {          
    for ( Int j = 0; j < MAX_NUM_LAYERS; j++)
    {    
      m_tileBoundariesAlignedFlag   [i][j] = false;
      m_minSpatialSegmentOffsetPlus1[i][j] = 0;
      m_ctuBasedOffsetEnabledFlag   [i][j] = false;
      m_minHorizontalCtuOffsetPlus1 [i][j] = -1;
    }
    m_baseLayerParameterSetCompatibilityFlag[i] = false;
  }
  for ( Int i = 0; i < MAX_NUM_VIDEO_SIGNAL_INFO; i++ )
  {
    m_videoSignalInfo          [i] = NULL;     
  }

  m_vpsVuiBspHrdPresentFlag = false; 
#if H_MV_HLS10_VPS_VUI_BSP
  m_vpsVuiBspHrdParameters  = NULL;
#else
  m_vpsVuiBspHrdParameters  = new TComVpsVuiBspHrdParameters();
#endif
}

TComVPSVUI::~TComVPSVUI()
{
  for ( Int i = 0; i < MAX_NUM_VIDEO_SIGNAL_INFO; i++ )
  {
    if (m_videoSignalInfo[ i ] != NULL )      delete m_videoSignalInfo[ i ];    
    m_videoSignalInfo    [ i ] = NULL; 
  }

  if ( m_vpsVuiBspHrdParameters ) delete m_vpsVuiBspHrdParameters;
  m_vpsVuiBspHrdParameters = NULL; 
}

Void TComVPSVUI::inferVpsVui( Bool encoderFlag )
{
  // inference of syntax elements that differ from default inference (as done in constructor), when VPS VUI is not present
  if (!encoderFlag )
  {
    setCrossLayerIrapAlignedFlag( false ); 
  }
  else
  {
    assert( !getCrossLayerIrapAlignedFlag() ); 
  }
}

Void TComRepFormat::inferChromaAndBitDepth( TComRepFormat* prevRepFormat, Bool encoderFlag )
{
  if ( !encoderFlag )
  {
    setChromaAndBitDepthVpsPresentFlag( prevRepFormat->getChromaAndBitDepthVpsPresentFlag() );
    setSeparateColourPlaneVpsFlag     ( prevRepFormat->getSeparateColourPlaneVpsFlag     () );
    setBitDepthVpsLumaMinus8          ( prevRepFormat->getBitDepthVpsLumaMinus8          () );
    setBitDepthVpsChromaMinus8        ( prevRepFormat->getBitDepthVpsChromaMinus8        () );
  }
  else
  {
    assert( getChromaAndBitDepthVpsPresentFlag() == prevRepFormat->getChromaAndBitDepthVpsPresentFlag() );
    assert( getSeparateColourPlaneVpsFlag     () == prevRepFormat->getSeparateColourPlaneVpsFlag     () );
    assert( getBitDepthVpsLumaMinus8          () == prevRepFormat->getBitDepthVpsLumaMinus8          () );
    assert( getBitDepthVpsChromaMinus8        () == prevRepFormat->getBitDepthVpsChromaMinus8        () );
  }
}

#if !H_MV_HLS10_VPS_VUI_BSP
Void TComVpsVuiBspHrdParameters::checkLayerInBspFlag( TComVPS* vps, Int h )
{
  // It is a requirement of bitstream conformance that bitstream partition with index j shall not include 
  // direct or indirect reference layers of any layers in bitstream partition i for any values of i and j 
  // in the range of 0 to num_bitstream_partitions[ h ] ?1, inclusive, such that i is less than j.

  for ( Int partJ = 0; partJ < getNumBitstreamPartitions( h ); partJ++ )
  {        
    for ( Int partI = 0; partI < partJ; partI++ )
    {
      for ( Int layerJ = 0; layerJ < vps->getMaxLayersMinus1(); layerJ++ )
      {
        if ( m_layerInBspFlag[ h ][partJ][layerJ ] )
        {
          for ( Int layerI = 0; layerI < vps->getMaxLayersMinus1(); layerI++ )
          {
            if ( m_layerInBspFlag[ h ][partI][layerI] )
            {
              assert( !vps->getInDirectDependencyFlag( layerI, layerJ ) ); 
            }
          }
        }
      }
    }
  }

  // ---------------
  // To be added: 
  // When vps_base_layer_internal_flag is equal to 0 and layer_in_bsp_flag[ h ][ i ][ 0 ] is equal to 1 for any value of h in the 
  // range of 1 to vps_num_layer_sets_minus1, inclusive, and any value of i in the range of 0 to num_bitstream_partitions[ h ] - 1, 
  // inclusive, the value of layer_in_bsp_flag[ h ][ i ][ j ] for at least one value of j in the range of 1 to 
  // NumLayersInIdList[ h ] - 1, inclusive, shall be equal to 1.
  // ---------------


  // When num_bitstream_partitions[ h ] is equal to 1 for any value of h in the range 1 to vps_num_layer_set_minus1, inclusive, 
  // the value of layer_in_bsp_flag[ h ][ 0 ][ j ] should be equal to 0 for at least one value of j in the range 0 to 
  // NumLayersInIdList[ h ] ? 1, inclusive. 


  if ( getNumBitstreamPartitions( h ) == 1 )
  {
    Bool atLeastOneZero = false; 
    for ( Int j = 0; j <= vps->getNumLayersInIdList( h ) - 1; j++ )
    {
      atLeastOneZero = atLeastOneZero || !getLayerInBspFlag( h, 0, j ); 
    }    
    assert( atLeastOneZero ); 
  }

  
  // For any value of h in the range 1 to vps_num_layer_set_minus1, inclusive, the value of layer_in_bsp_flag[ h ][ i ][ j ] 
  // shall be equal to 1 for at most one value of i in the range of 0 to num_bitstream_partitions[ h ] ? 1, inclusive. 

  for ( Int j = 0; j <= vps->getNumLayersInIdList( h ) - 1; j++ )
  {
    Int numLayerInBsp = 0; 
    for ( Int i = 0; i <= getNumBitstreamPartitions( h ) - 1; i++ )
    {
      numLayerInBsp += ( getLayerInBspFlag( h, i, j ) ? 1 : 0 );  
    }    
    assert( numLayerInBsp <= 1 ); 
  }

}

Void TComVpsVuiBspHrdParameters::checkBspCombHrdAndShedIdx( TComVPS* vps, Int h, Int i, Int j )
{
  // bsp_comb_hrd_idx
  assert( getBspCombSchedIdx(h, i, j ) >= 0 );
  assert( getBspCombSchedIdx(h, i, j ) <= getVpsNumBspHrdParametersMinus1() );

  // bsp_comb_sched_idx
  assert( getBspCombSchedIdx(h, i, j ) >= 0 );

  //* This check needs to activated,  when HighestTid is available here    
  //  assert(  getBspCombSchedIdx(h, i, j ) <= vps->getHrdParameters( getBspCombHrdIdx( h, i, j ) )->getCpbCntMinus1( highestTid ) );
}
#endif

#if H_MV_HLS10_VPS_VUI_BSP

TComVpsVuiBspHrdParameters::~TComVpsVuiBspHrdParameters()
{
  delete[] m_cprmsAddPresentFlag; 
  delete[] m_numSubLayerHrdMinus1; 
  delete[] m_hrdParameters; 

  for (Int h = 0; h < m_numOls; h++)
  {
    if ( h > 0 )
    {    
      for (Int i = 0; i < getNumSignalledPartitioningSchemes(h)+1; i++)
      {
        for (Int t = 0; t < m_vps->getMaxSubLayersInLayerSetMinus1( m_vps->olsIdxToLsIdx(h) ) + 1; t++)
        {        
          for ( Int j = 0; j <= getNumBspSchedulesMinus1(h,i,j);j++ )
          {
            delete[] m_bspHrdIdx  [h][i][t][j]; 
            delete[] m_bspSchedIdx[h][i][t][j];
          }
        }
        delete[] m_numBspSchedulesMinus1[h][i];
      }
    }

    for (Int j = 0; j <= getNumSignalledPartitioningSchemes(h ) + 1; j++ )
    {
      for (Int k = 0; k <= getNumPartitionsInSchemeMinus1(h,j); k++ )
      {
        delete[] m_layerIncludedInPartitionFlag[h][j][k];
      }        
    }
    delete[] m_numPartitionsInSchemeMinus1[h];       
  }
  delete[] m_numSignalledPartitioningSchemes;
}


Int TComVpsVuiBspHrdParameters::getBspHrdIdxLen(TComVPS* vps)
{
  return gCeilLog2( vps->getNumHrdParameters() + getVpsNumAddHrdParams() );
}

Void TComVpsVuiBspHrdParameters::createAfterVpsNumAddHrdParams( TComVPS* vps )
{
  assert( vps == NULL ); 
  m_vps = vps; 
  m_offsetHrdParamIdx = vps->getNumHrdParameters(); 
  m_numHrdParam       = vps->getNumHrdParameters() + getVpsNumAddHrdParams() - m_offsetHrdParamIdx;
  m_numOls            = vps->getNumOutputLayerSets(); 

  m_cprmsAddPresentFlag   = new Bool    [ m_numHrdParam ];
  m_numSubLayerHrdMinus1  = new Int     [ m_numHrdParam ];
  m_hrdParameters         = new TComHRD [ m_numHrdParam ];

  m_numSignalledPartitioningSchemes = new Int    [ m_numOls ]; 
  m_numPartitionsInSchemeMinus1     = new Int*   [ m_numOls ];
  m_numBspSchedulesMinus1           = new Int**  [ m_numOls ]; 
  m_bspHrdIdx                       = new Int****[ m_numOls ];
  m_bspSchedIdx                     = new Int****[ m_numOls ];
}

Void TComVpsVuiBspHrdParameters::createAfterNumSignalledPartitioningSchemes( Int h )
{
  m_numPartitionsInSchemeMinus1 [h]    = new Int    [ getNumSignalledPartitioningSchemes(h) ];
  m_layerIncludedInPartitionFlag[h]    = new Bool** [ getNumSignalledPartitioningSchemes(h) ];    

  m_numBspSchedulesMinus1[h]           = new Int*   [ getNumSignalledPartitioningSchemes(h) + 1 ];
  for (Int i = 0; i < getNumSignalledPartitioningSchemes(h) + 1; i++)
  {
    Int tMax = m_vps->getMaxSubLayersInLayerSetMinus1( m_vps->olsIdxToLsIdx(h) ) + 1;
    m_numBspSchedulesMinus1[h][i] = new Int  [ tMax ];
    m_bspHrdIdx            [h][i] = new Int**[ tMax ];
    m_bspSchedIdx          [h][i] = new Int**[ tMax ];
  }
}

Void TComVpsVuiBspHrdParameters::createAfterNumPartitionsInSchemeMinus1( Int h, Int j )
{
  m_layerIncludedInPartitionFlag[h][j] = new Bool*[ getNumPartitionsInSchemeMinus1(h,j)];
  for( Int k = 0; k < getNumPartitionsInSchemeMinus1(h,j); k++ )
  {
    m_layerIncludedInPartitionFlag[h][j][k] = new Bool[ m_vps->getNumLayersInIdList( m_vps->olsIdxToLsIdx(h))];
  }
}

Void TComVpsVuiBspHrdParameters::createAfterNumBspSchedulesMinus1( Int h, Int i, Int t )
{
  m_bspSchedIdx[h][i][t] = new Int* [ getNumBspSchedulesMinus1( h, i, t ) + 1 ];
  m_bspHrdIdx  [h][i][t] = new Int* [ getNumBspSchedulesMinus1( h, i, t ) + 1 ];
  for( Int j = 0; j < getNumBspSchedulesMinus1( h, i, t ) + 1; j++ )
  {
    m_bspSchedIdx[h][i][t][j] = new Int[ getNumPartitionsInSchemeMinus1( h, i ) ];
    m_bspHrdIdx  [h][i][t][j] = new Int[ getNumPartitionsInSchemeMinus1( h, i ) ];
  }
}

#endif

Void TComVUI::inferVideoSignalInfo( TComVPS* vps, Int layerIdCurr )
{
  if ( layerIdCurr == 0 || !vps->getVpsVuiPresentFlag() ) 
  {
    return; 
  }

  TComVPSVUI* vpsVUI = vps->getVPSVUI(); 
  assert( vpsVUI != NULL );  

  TComVideoSignalInfo* videoSignalInfo = vpsVUI->getVideoSignalInfo( vpsVUI->getVpsVideoSignalInfoIdx( vps->getLayerIdInVps( layerIdCurr ) ) ); 
  assert( videoSignalInfo != NULL );

  setVideoFormat            ( videoSignalInfo->getVideoVpsFormat            () ); 
  setVideoFullRangeFlag     ( videoSignalInfo->getVideoFullRangeVpsFlag     () );
  setColourPrimaries        ( videoSignalInfo->getColourPrimariesVps        () );
  setTransferCharacteristics( videoSignalInfo->getTransferCharacteristicsVps() );
  setMatrixCoefficients     ( videoSignalInfo->getMatrixCoeffsVps           () );
}

TComDpbSize::TComDpbSize()
{
  for (Int i = 0; i < MAX_VPS_OUTPUTLAYER_SETS; i++ )
  {      
    m_subLayerFlagInfoPresentFlag[i]  = false;

    for (Int j = 0; j < MAX_TLAYER; j++  )
    {        
      m_subLayerDpbInfoPresentFlag [i][j] = ( j == 0) ;
      m_maxVpsNumReorderPics       [i][j] = 0;
      m_maxVpsLatencyIncreasePlus1 [i][j] = 0;

      for (Int k = 0; k < MAX_NUM_LAYER_IDS; k++ )
      {
#if H_MV_HLS10_ADD_LAYERSETS
        m_maxVpsDecPicBufferingMinus1[i][k][j] = MIN_INT; 
#else
        m_maxVpsDecPicBufferingMinus1[i][k][j] = 0; 
#endif
      }
    }
  }
}

Void Window::scaleOffsets( Int scal )
{
  if (! m_scaledFlag )
  {
    m_scaledFlag         = true; 
    m_winLeftOffset     *= scal; 
    m_winRightOffset    *= scal; 
    m_winTopOffset      *= scal; 
    m_winBottomOffset   *= scal; 
  }
}
#endif

