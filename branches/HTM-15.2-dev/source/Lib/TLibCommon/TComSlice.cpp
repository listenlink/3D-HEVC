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
, m_PicOutputFlag                 ( true )
#if NH_MV
, m_slicePicOrderCntLsb           ( 0 )
#endif
, m_iPOC                          ( 0 )
, m_iLastIDR                      ( 0 )
, m_iAssociatedIRAP               ( 0 )
, m_iAssociatedIRAPType           ( NAL_UNIT_INVALID )
, m_pRPS                          ( 0 )
, m_localRPS                      ( )
, m_rpsIdx                        ( 0 )
, m_RefPicListModification        ( )
, m_eNalUnitType                  ( NAL_UNIT_CODED_SLICE_IDR_W_RADL )
, m_eSliceType                    ( I_SLICE )
, m_iSliceQp                      ( 0 )
, m_dependentSliceSegmentFlag     ( false )
#if ADAPTIVE_QP_SELECTION
, m_iSliceQpBase                  ( 0 )
#endif
, m_ChromaQpAdjEnabled            ( false )
, m_deblockingFilterDisable       ( false )
, m_deblockingFilterOverrideFlag  ( false )
, m_deblockingFilterBetaOffsetDiv2( 0 )
, m_deblockingFilterTcOffsetDiv2  ( 0 )
, m_bCheckLDC                     ( false )
, m_iSliceQpDelta                 ( 0 )
, m_iDepth                        ( 0 )
, m_bRefenced                     ( false )
, m_pcVPS                         ( NULL )
, m_pcSPS                         ( NULL )
, m_pcPPS                         ( NULL )
, m_pcPic                         ( NULL )
, m_colFromL0Flag                 ( true )
, m_noOutputPriorPicsFlag         ( false )
, m_noRaslOutputFlag              ( false )
, m_handleCraAsBlaFlag            ( false )
, m_colRefIdx                     ( 0 )
, m_maxNumMergeCand               ( 0 )
, m_uiTLayer                      ( 0 )
, m_bTLayerSwitchingFlag          ( false )
, m_sliceMode                     ( NO_SLICES )
, m_sliceArgument                 ( 0 )
, m_sliceCurStartCtuTsAddr        ( 0 )
, m_sliceCurEndCtuTsAddr          ( 0 )
, m_sliceIdx                      ( 0 )
, m_sliceSegmentMode              ( NO_SLICES )
, m_sliceSegmentArgument          ( 0 )
, m_sliceSegmentCurStartCtuTsAddr ( 0 )
, m_sliceSegmentCurEndCtuTsAddr   ( 0 )
, m_nextSlice                     ( false )
, m_nextSliceSegment              ( false )
, m_sliceBits                     ( 0 )
, m_sliceSegmentBits              ( 0 )
, m_bFinalized                    ( false )
, m_bTestWeightPred               ( false )
, m_bTestWeightBiPred             ( false )
, m_substreamSizes                ( )
, m_cabacInitFlag                 ( false )
, m_bLMvdL1Zero                   ( false )
, m_temporalLayerNonReferenceFlag ( false )
, m_LFCrossSliceBoundaryFlag      ( false )
, m_enableTMVPFlag                ( true )
, m_encCABACTableIdx              (I_SLICE)
#if NH_MV
, m_refPicSetInterLayer0          ( NULL )
, m_refPicSetInterLayer1          ( NULL )
, m_layerId                       (0)
, m_viewId                        (0)
, m_viewIndex                     (0)
#if NH_3D_VSO
, m_isDepth                       (false)
#endif
#if NH_MV
, m_pocResetFlag                  (false)
, m_crossLayerBlaFlag             (false)
#endif
, m_discardableFlag               (false)
, m_interLayerPredEnabledFlag     (false)
, m_numInterLayerRefPicsMinus1    (0)
#if NH_MV
, m_sliceSegmentHeaderExtensionLength (0)
, m_pocResetIdc                   (0)
, m_pocResetPeriodId              (0)
, m_hasPocResetPeriodIdPresent    (false)
, m_fullPocResetFlag              (false)
, m_pocLsbVal                     (0)
, m_pocMsbCycleValPresentFlag     (false)
, m_pocMsbCycleVal                (0)
, m_pocMsbValRequiredFlag         (false)
#endif
#if NH_3D_IC
, m_bApplyIC                      (false)
, m_icSkipParseFlag               (false)
#endif
#if NH_3D
, m_inCmpPredFlag                 (false)
, m_numViews                      (0)
, m_depthToDisparityB             (NULL)
, m_depthToDisparityF             (NULL)
#endif
#if NH_3D_DIS
, m_bApplyDIS                     (false)
#endif
#endif
{

#if NH_MV
  m_shortTermRefPicSetIdx = 0; 
  m_numLongTermSps        = 0; 
  m_numLongTermPics       = 0; 
  for (Int i = 0; i < MAX_NUM_PICS_RPS; i++)
  {
    m_ltIdxSps          [i] = 0; 
    m_deltaPocMsbCycleLt[i] = 0;
  }
  setSliceTemporalMvpEnabledFlag( false ); 
#endif
  for(UInt i=0; i<NUM_REF_PIC_LIST_01; i++)
  {
    m_aiNumRefIdx[i] = 0;
  }

  for (UInt component = 0; component < MAX_NUM_COMPONENT; component++)
  {
    m_lambdas            [component] = 0.0;
    m_iSliceChromaQpDelta[component] = 0;
  }

  initEqualRef();

  for ( Int idx = 0; idx < MAX_NUM_REF; idx++ )
  {
    m_list1IdxToList0Idx[idx] = -1;
  }

  for(Int iNumCount = 0; iNumCount < MAX_NUM_REF; iNumCount++)
  {
    for(UInt i=0; i<NUM_REF_PIC_LIST_01; i++)
    {
      m_apcRefPicList [i][iNumCount] = NULL;
      m_aiRefPOCList  [i][iNumCount] = 0;
    }
#if NH_MV
    m_aiRefLayerIdList[0][iNumCount] = 0;
    m_aiRefLayerIdList[1][iNumCount] = 0;
#endif
  }

  resetWpScaling();
  initWpAcDcParam();

  for(Int ch=0; ch < MAX_NUM_CHANNEL_TYPE; ch++)
  {
    m_saoEnabledFlag[ch] = false;
  }
#if NH_MV
  for (Int i = 0; i < MAX_NUM_LAYERS; i++ )
  {
    m_interLayerPredLayerIdc[ i ] = -1;
  }
#endif
#if NH_3D
  m_iDefaultRefViewIdx = -1;
  m_bDefaultRefViewIdxAvailableFlag = false;
  m_ivMvPredFlag           = false;
  m_ivMvScalingFlag        = false;
  m_ivResPredFlag          = false;  
  m_depthRefinementFlag    = false;
  m_viewSynthesisPredFlag  = false;
  m_depthBasedBlkPartFlag  = false;
  m_mpiFlag                = false;
  m_intraContourFlag       = false;
  m_intraSdcWedgeFlag      = false;
  m_qtPredFlag             = false;
  m_interSdcFlag           = false;
  m_depthIntraSkipFlag     = false;
  m_subPbSize              =  1 << 6;
  m_mpiSubPbSize           =  1 << 6;

  m_aaiCodedOffset.resize(2);
  m_aaiCodedScale .resize(2);
  for (Int i = 0; i < 2; i++)
  {
    m_aaiCodedOffset[i].resize(MAX_NUM_LAYERS);
    m_aaiCodedScale [i].resize(MAX_NUM_LAYERS);
  }
  
#endif

}

TComSlice::~TComSlice()
{
#if NH_3D
  for( UInt i = 0; i < m_numViews; i++ )
  {
    if ( m_depthToDisparityB && m_depthToDisparityB[ i ] )
    {
      delete[] m_depthToDisparityB [ i ];
    }

    if ( m_depthToDisparityF && m_depthToDisparityF[ i ] ) 
    {
      delete[] m_depthToDisparityF [ i ];
    }
  }

  if ( m_depthToDisparityF )
  {
    delete[] m_depthToDisparityF; 
  }

  m_depthToDisparityF = NULL;

  if ( m_depthToDisparityB )
    delete[] m_depthToDisparityB; 

  m_depthToDisparityB = NULL;
#endif

}


Void TComSlice::initSlice()
{
  for(UInt i=0; i<NUM_REF_PIC_LIST_01; i++)
  {
    m_aiNumRefIdx[i]      = 0;
  }
  m_colFromL0Flag = true;

  m_colRefIdx = 0;
  initEqualRef();

  m_bCheckLDC = false;

  for (UInt component = 0; component < MAX_NUM_COMPONENT; component++)
  {
    m_iSliceChromaQpDelta[component] = 0;
  }
#if NH_3D_IV_MERGE
  m_maxNumMergeCand = MRG_MAX_NUM_CANDS_MEM;
#else
  m_maxNumMergeCand = MRG_MAX_NUM_CANDS;
#endif

  m_bFinalized=false;

  m_substreamSizes.clear();
  m_cabacInitFlag        = false;
  m_enableTMVPFlag = true;
#if NH_3D_TMVP
  m_aiAlterRefIdx[0]                  = -1;
  m_aiAlterRefIdx[1]                  = -1;
#endif
}

Bool TComSlice::getRapPicFlag() const
{
  return getNalUnitType() == NAL_UNIT_CODED_SLICE_IDR_W_RADL
      || getNalUnitType() == NAL_UNIT_CODED_SLICE_IDR_N_LP
      || getNalUnitType() == NAL_UNIT_CODED_SLICE_BLA_N_LP
      || getNalUnitType() == NAL_UNIT_CODED_SLICE_BLA_W_RADL
      || getNalUnitType() == NAL_UNIT_CODED_SLICE_BLA_W_LP
      || getNalUnitType() == NAL_UNIT_CODED_SLICE_CRA;
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
    for (Int j = 0; j < i; j++)
    {
      iterPicExtract++;
    }
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

TComPic* TComSlice::xGetRefPic (TComList<TComPic*>& rcListPic, Int poc)
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
  for (Int iDir = 0; iDir < NUM_REF_PIC_LIST_01; iDir++)
  {
    for (Int iNumRefIdx = 0; iNumRefIdx < m_aiNumRefIdx[iDir]; iNumRefIdx++)
    {
      m_aiRefPOCList[iDir][iNumRefIdx] = m_apcRefPicList[iDir][iNumRefIdx]->getPOC();
#if NH_MV
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

#if !NH_MV
Void TComSlice::setRefPicList( TComList<TComPic*>& rcListPic, Bool checkNumPocTotalCurr )
{
    if (m_eSliceType == I_SLICE)
    {
      ::memset( m_apcRefPicList, 0, sizeof (m_apcRefPicList));
      ::memset( m_aiNumRefIdx,   0, sizeof ( m_aiNumRefIdx ));

    if (!checkNumPocTotalCurr)
    {
      return;
    }
  }

  TComPic*  pcRefPic= NULL;
  static const UInt MAX_NUM_NEGATIVE_PICTURES=16;
  TComPic*  RefPicSetStCurr0[MAX_NUM_NEGATIVE_PICTURES];
  TComPic*  RefPicSetStCurr1[MAX_NUM_NEGATIVE_PICTURES];
  TComPic*  RefPicSetLtCurr[MAX_NUM_NEGATIVE_PICTURES];
  UInt NumPicStCurr0 = 0;
  UInt NumPicStCurr1 = 0;
  UInt NumPicLtCurr = 0;
  Int i;

  for(i=0; i < m_pRPS->getNumberOfNegativePictures(); i++)
  {
    if(m_pRPS->getUsed(i))
    {
      pcRefPic = xGetRefPic(rcListPic, getPOC()+m_pRPS->getDeltaPOC(i));
      pcRefPic->setIsLongTerm(0);
      pcRefPic->getPicYuvRec()->extendPicBorder();
      RefPicSetStCurr0[NumPicStCurr0] = pcRefPic;
      NumPicStCurr0++;
      pcRefPic->setCheckLTMSBPresent(false);
    }
  }

  for(; i < m_pRPS->getNumberOfNegativePictures()+m_pRPS->getNumberOfPositivePictures(); i++)
  {
    if(m_pRPS->getUsed(i))
    {
      pcRefPic = xGetRefPic(rcListPic, getPOC()+m_pRPS->getDeltaPOC(i));
      pcRefPic->setIsLongTerm(0);
      pcRefPic->getPicYuvRec()->extendPicBorder();
      RefPicSetStCurr1[NumPicStCurr1] = pcRefPic;
      NumPicStCurr1++;
      pcRefPic->setCheckLTMSBPresent(false);
    }
  }

  for(i = m_pRPS->getNumberOfNegativePictures()+m_pRPS->getNumberOfPositivePictures()+m_pRPS->getNumberOfLongtermPictures()-1; i > m_pRPS->getNumberOfNegativePictures()+m_pRPS->getNumberOfPositivePictures()-1 ; i--)
  {
    if(m_pRPS->getUsed(i))
    {
      pcRefPic = xGetLongTermRefPic(rcListPic, m_pRPS->getPOC(i), m_pRPS->getCheckLTMSBPresent(i));
      pcRefPic->setIsLongTerm(1);
      pcRefPic->getPicYuvRec()->extendPicBorder();
      RefPicSetLtCurr[NumPicLtCurr] = pcRefPic;
      NumPicLtCurr++;
    }
    if(pcRefPic==NULL)
    {
      pcRefPic = xGetLongTermRefPic(rcListPic, m_pRPS->getPOC(i), m_pRPS->getCheckLTMSBPresent(i));
    }
    pcRefPic->setCheckLTMSBPresent(m_pRPS->getCheckLTMSBPresent(i));
  }

  // ref_pic_list_init
  TComPic*  rpsCurrList0[MAX_NUM_REF+1];
  TComPic*  rpsCurrList1[MAX_NUM_REF+1];
  Int numPicTotalCurr = NumPicStCurr0 + NumPicStCurr1 + NumPicLtCurr;

  if (checkNumPocTotalCurr)
  {
    // The variable NumPocTotalCurr is derived as specified in subclause 7.4.7.2. It is a requirement of bitstream conformance that the following applies to the value of NumPocTotalCurr:
    // - If the current picture is a BLA or CRA picture, the value of NumPocTotalCurr shall be equal to 0.
    // - Otherwise, when the current picture contains a P or B slice, the value of NumPocTotalCurr shall not be equal to 0.
    if (getRapPicFlag())
    {
      assert(numPicTotalCurr == 0);
    }

    if (m_eSliceType == I_SLICE)
    {
      return;
    }

    assert(numPicTotalCurr > 0);
    // general tier and level limit:
    assert(numPicTotalCurr <= 8);
  }

  Int cIdx = 0;
  for ( i=0; i<NumPicStCurr0; i++, cIdx++)
  {
    rpsCurrList0[cIdx] = RefPicSetStCurr0[i];
  }
  for ( i=0; i<NumPicStCurr1; i++, cIdx++)
  {
    rpsCurrList0[cIdx] = RefPicSetStCurr1[i];
  }
  for ( i=0; i<NumPicLtCurr;  i++, cIdx++)
  {
    rpsCurrList0[cIdx] = RefPicSetLtCurr[i];
  }
  assert(cIdx == numPicTotalCurr);

  if (m_eSliceType==B_SLICE)
  {
    cIdx = 0;
    for ( i=0; i<NumPicStCurr1; i++, cIdx++)
    {
      rpsCurrList1[cIdx] = RefPicSetStCurr1[i];
    }
    for ( i=0; i<NumPicStCurr0; i++, cIdx++)
    {
      rpsCurrList1[cIdx] = RefPicSetStCurr0[i];
    }
    for ( i=0; i<NumPicLtCurr;  i++, cIdx++)
    {
      rpsCurrList1[cIdx] = RefPicSetLtCurr[i];
    }
    assert(cIdx == numPicTotalCurr);
  }

  ::memset(m_bIsUsedAsLongTerm, 0, sizeof(m_bIsUsedAsLongTerm));

  for (Int rIdx = 0; rIdx < m_aiNumRefIdx[REF_PIC_LIST_0]; rIdx ++)
  {
    cIdx = m_RefPicListModification.getRefPicListModificationFlagL0() ? m_RefPicListModification.getRefPicSetIdxL0(rIdx) : rIdx % numPicTotalCurr;
    assert(cIdx >= 0 && cIdx < numPicTotalCurr);
    m_apcRefPicList[REF_PIC_LIST_0][rIdx] = rpsCurrList0[ cIdx ];
    m_bIsUsedAsLongTerm[REF_PIC_LIST_0][rIdx] = ( cIdx >= NumPicStCurr0 + NumPicStCurr1 );
  }
  if ( m_eSliceType != B_SLICE )
  {
    m_aiNumRefIdx[REF_PIC_LIST_1] = 0;
    ::memset( m_apcRefPicList[REF_PIC_LIST_1], 0, sizeof(m_apcRefPicList[REF_PIC_LIST_1]));
  }
  else
  {
    for (Int rIdx = 0; rIdx < m_aiNumRefIdx[REF_PIC_LIST_1]; rIdx ++)
    {
      cIdx = m_RefPicListModification.getRefPicListModificationFlagL1() ? m_RefPicListModification.getRefPicSetIdxL1(rIdx) : rIdx % numPicTotalCurr;
      assert(cIdx >= 0 && cIdx < numPicTotalCurr);
      m_apcRefPicList[REF_PIC_LIST_1][rIdx] = rpsCurrList1[ cIdx ];
      m_bIsUsedAsLongTerm[REF_PIC_LIST_1][rIdx] = ( cIdx >= NumPicStCurr0 + NumPicStCurr1 );
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
#if NH_3D
  m_pocsInCurrRPSs.clear();
#endif
  for(i=0; i < m_pRPS->getNumberOfNegativePictures(); i++)
  {
    if(m_pRPS->getUsed(i))
    {
      pcRefPic = xGetRefPic(rcListPic, getPOC()+m_pRPS->getDeltaPOC(i));
      pcRefPic->setIsLongTerm(0);
      pcRefPic->getPicYuvRec()->extendPicBorder();
      RefPicSetStCurr0[NumPocStCurr0] = pcRefPic;
      NumPocStCurr0++;
      pcRefPic->setCheckLTMSBPresent(false);  
#if NH_3D
      m_pocsInCurrRPSs.push_back( pcRefPic->getPOC() ); 
#endif
    }
  }
  
  for(; i < m_pRPS->getNumberOfNegativePictures()+m_pRPS->getNumberOfPositivePictures(); i++)
  {
    if(m_pRPS->getUsed(i))
    {
      pcRefPic = xGetRefPic(rcListPic, getPOC()+m_pRPS->getDeltaPOC(i));
      pcRefPic->setIsLongTerm(0);
      pcRefPic->getPicYuvRec()->extendPicBorder();
      RefPicSetStCurr1[NumPocStCurr1] = pcRefPic;
      NumPocStCurr1++;
      pcRefPic->setCheckLTMSBPresent(false);  
#if NH_3D
      m_pocsInCurrRPSs.push_back( pcRefPic->getPOC() ); 
#endif
    }
  }
  
  for(i = m_pRPS->getNumberOfNegativePictures()+m_pRPS->getNumberOfPositivePictures()+m_pRPS->getNumberOfLongtermPictures()-1; i > m_pRPS->getNumberOfNegativePictures()+m_pRPS->getNumberOfPositivePictures()-1 ; i--)
  {
    if(m_pRPS->getUsed(i))
    {
      pcRefPic = xGetLongTermRefPic(rcListPic, m_pRPS->getPOC(i), m_pRPS->getCheckLTMSBPresent(i));
      pcRefPic->setIsLongTerm(1);
      pcRefPic->getPicYuvRec()->extendPicBorder();
      RefPicSetLtCurr[NumPocLtCurr] = pcRefPic;
      NumPocLtCurr++;
#if NH_3D
      m_pocsInCurrRPSs.push_back( pcRefPic->getPOC() ); 
#endif
    }
    if(pcRefPic==NULL) 
    {
      pcRefPic = xGetLongTermRefPic(rcListPic, m_pRPS->getPOC(i), m_pRPS->getCheckLTMSBPresent(i));
    }
    pcRefPic->setCheckLTMSBPresent(m_pRPS->getCheckLTMSBPresent(i));  
  }

  Int numPocInterCurr = NumPocStCurr0 + NumPocStCurr1 + NumPocLtCurr; 
#if NH_3D
  assert( numPocInterCurr == (Int) m_pocsInCurrRPSs.size() ); 
#endif
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
  Int numPocStCurr[2] = { (Int)NumPocStCurr0, (Int)NumPocStCurr1 }; 

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
        assert( rpsCurrList[li][ orgIdx ]->getSlice(0)->getDiscardableFlag() == 0 );    // Inter-layer RPS shall not contain picture with discardable_flag = 1.
        m_apcRefPicList    [li][rIdx] = rpsCurrList    [li][ orgIdx ];
        m_bIsUsedAsLongTerm[li][rIdx] = usedAsLongTerm [li][ orgIdx ] ; 
      }
    }
  }
}
#endif


Int TComSlice::getNumRpsCurrTempList() const
{
  Int numRpsCurrTempList = 0;

  if (m_eSliceType == I_SLICE)
  {
    return 0;
  }
  for(UInt i=0; i < m_pRPS->getNumberOfNegativePictures()+ m_pRPS->getNumberOfPositivePictures() + m_pRPS->getNumberOfLongtermPictures(); i++)
  {
    if(m_pRPS->getUsed(i))
    {
      numRpsCurrTempList++;
    }
  }
#if NH_MV
    numRpsCurrTempList = numRpsCurrTempList + getNumActiveRefLayerPics();
#endif
  return numRpsCurrTempList;
}

Void TComSlice::initEqualRef()
{
  for (Int iDir = 0; iDir < NUM_REF_PIC_LIST_01; iDir++)
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
#if NH_3D
#if NH_3D_TMVP
Void TComSlice::generateAlterRefforTMVP()
{
  for ( UInt uiRefListIdx = 0; uiRefListIdx < 2; uiRefListIdx++ )
  {        
    if ( this->getNumRefIdx( RefPicList( uiRefListIdx ) ) == 0)
    {
        continue;
    }
    Bool bZeroIdxLtFlag = this->getRefPic(RefPicList(uiRefListIdx), 0)->getIsLongTerm();
    for(Int i = 1; i < this->getNumRefIdx(RefPicList(uiRefListIdx)); i++ )
    {
      if ( ( bZeroIdxLtFlag && !this->getRefPic(RefPicList(uiRefListIdx), i)->getIsLongTerm() ) ||
           (!bZeroIdxLtFlag &&  this->getRefPic(RefPicList(uiRefListIdx), i)->getIsLongTerm() ) )
      {
        this->setAlterRefIdx(RefPicList(uiRefListIdx),i);
        break;
      }
    }
  }
}
#endif
#endif

#if NH_3D
Void TComSlice::setCamparaSlice( Int** aaiScale, Int** aaiOffset )
{  
  Int voiInVps      = m_pcVPS->getVoiInVps(getViewIndex() ); 
  if( m_pcVPS->getNumCp( voiInVps ) > 0 )
  {    
    if( m_pcVPS->getCpInSliceSegmentHeaderFlag( voiInVps ) )
    {
      for( Int m = 0; m < m_pcVPS->getNumCp( voiInVps ); m++ )
      {      
        Int j      = m_pcVPS->getCpRefVoi( voiInVps, m );
        Int jInVps = m_pcVPS->getVoiInVps( j ); 

        setCpScale   ( jInVps , aaiScale [ jInVps   ][ voiInVps ]);
        setCpInvScale( jInVps , aaiScale [ voiInVps ][ jInVps   ]);
        setCpOff     ( jInVps , aaiOffset[ jInVps   ][ voiInVps ]);
        setCpInvOff  ( jInVps , aaiOffset[ voiInVps ][ jInVps   ]);
      }
    }
  } 
}
#endif

Void TComSlice::checkColRefIdx(UInt curSliceIdx, TComPic* pic)
{
  Int i;
  TComSlice* curSlice = pic->getSlice(curSliceIdx);
  Int currColRefPOC =  curSlice->getRefPOC( RefPicList(1 - curSlice->getColFromL0Flag()), curSlice->getColRefIdx());
  TComSlice* preSlice;
  Int preColRefPOC;
  for(i=curSliceIdx-1; i>=0; i--)
  {
    preSlice = pic->getSlice(i);
    if(preSlice->getSliceType() != I_SLICE)
    {
      preColRefPOC  = preSlice->getRefPOC( RefPicList(1 - preSlice->getColFromL0Flag()), preSlice->getColRefIdx());
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

Void TComSlice::checkCRA(const TComReferencePictureSet *pReferencePictureSet, Int& pocCRA, NalUnitType& associatedIRAPType, TComList<TComPic *>& rcListPic)
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
Void TComSlice::decodingRefreshMarking(Int& pocCRA, Bool& bRefreshPending, TComList<TComPic*>& rcListPic, const bool bEfficientFieldIRAPEnabled)
{
  TComPic* rpcPic;
  Int      pocCurr = getPOC();

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
      if (rpcPic->getPOC() != pocCurr)
      {
        rpcPic->getSlice(0)->setReferenced(false);
      }
      iterPic++;
    }
    if ( getNalUnitType() == NAL_UNIT_CODED_SLICE_BLA_W_LP
      || getNalUnitType() == NAL_UNIT_CODED_SLICE_BLA_W_RADL
      || getNalUnitType() == NAL_UNIT_CODED_SLICE_BLA_N_LP )
    {
      pocCRA = pocCurr;
    }
    if (bEfficientFieldIRAPEnabled)
    {
    bRefreshPending = true;
    }
  }
  else // CRA or No DR
  {
    if(bEfficientFieldIRAPEnabled && (getAssociatedIRAPType() == NAL_UNIT_CODED_SLICE_IDR_N_LP || getAssociatedIRAPType() == NAL_UNIT_CODED_SLICE_IDR_W_RADL))
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
      if (bRefreshPending==true && pocCurr > pocCRA) // CRA reference marking pending
      {
        TComList<TComPic*>::iterator iterPic = rcListPic.begin();
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
    }
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
#if NH_MV
  m_layerId              = pSrc->m_layerId;
  // GT: Copying of several other values might be be missing here, or is above not necessary? 
#endif
  m_eSliceType           = pSrc->m_eSliceType;
  m_iSliceQp             = pSrc->m_iSliceQp;
#if ADAPTIVE_QP_SELECTION
  m_iSliceQpBase         = pSrc->m_iSliceQpBase;
#endif
  m_ChromaQpAdjEnabled = pSrc->m_ChromaQpAdjEnabled;
  m_deblockingFilterDisable   = pSrc->m_deblockingFilterDisable;
  m_deblockingFilterOverrideFlag = pSrc->m_deblockingFilterOverrideFlag;
  m_deblockingFilterBetaOffsetDiv2 = pSrc->m_deblockingFilterBetaOffsetDiv2;
  m_deblockingFilterTcOffsetDiv2 = pSrc->m_deblockingFilterTcOffsetDiv2;

  for (i = 0; i < NUM_REF_PIC_LIST_01; i++)
  {
    m_aiNumRefIdx[i]     = pSrc->m_aiNumRefIdx[i];
  }

  for (i = 0; i < MAX_NUM_REF; i++)
  {
    m_list1IdxToList0Idx[i] = pSrc->m_list1IdxToList0Idx[i];
  }

  m_bCheckLDC             = pSrc->m_bCheckLDC;
  m_iSliceQpDelta        = pSrc->m_iSliceQpDelta;
  for (UInt component = 0; component < MAX_NUM_COMPONENT; component++)
  {
    m_iSliceChromaQpDelta[component] = pSrc->m_iSliceChromaQpDelta[component];
  }
  for (i = 0; i < NUM_REF_PIC_LIST_01; i++)
  {
    for (j = 0; j < MAX_NUM_REF; j++)
    {
      m_apcRefPicList[i][j]  = pSrc->m_apcRefPicList[i][j];
      m_aiRefPOCList[i][j]   = pSrc->m_aiRefPOCList[i][j];
      m_bIsUsedAsLongTerm[i][j] = pSrc->m_bIsUsedAsLongTerm[i][j];
#if NH_MV
      m_aiRefLayerIdList[i][j] = pSrc->m_aiRefLayerIdList[i][j];
#endif
    }
    m_bIsUsedAsLongTerm[i][MAX_NUM_REF] = pSrc->m_bIsUsedAsLongTerm[i][MAX_NUM_REF];
  }
  m_iDepth               = pSrc->m_iDepth;

  // referenced slice
  m_bRefenced            = pSrc->m_bRefenced;

  // access channel
#if NH_MV
  m_pcVPS                = pSrc->m_pcVPS;
#endif
  m_pRPS                = pSrc->m_pRPS;
  m_iLastIDR             = pSrc->m_iLastIDR;

  m_pcPic                = pSrc->m_pcPic;

  m_colFromL0Flag        = pSrc->m_colFromL0Flag;
  m_colRefIdx            = pSrc->m_colRefIdx;

  setLambdas(pSrc->getLambdas());

  for (i = 0; i < NUM_REF_PIC_LIST_01; i++)
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

  m_sliceMode                     = pSrc->m_sliceMode;
  m_sliceArgument                 = pSrc->m_sliceArgument;
  m_sliceCurStartCtuTsAddr        = pSrc->m_sliceCurStartCtuTsAddr;
  m_sliceCurEndCtuTsAddr          = pSrc->m_sliceCurEndCtuTsAddr;
  m_sliceIdx                      = pSrc->m_sliceIdx;
  m_sliceSegmentMode              = pSrc->m_sliceSegmentMode;
  m_sliceSegmentArgument          = pSrc->m_sliceSegmentArgument;
  m_sliceSegmentCurStartCtuTsAddr = pSrc->m_sliceSegmentCurStartCtuTsAddr;
  m_sliceSegmentCurEndCtuTsAddr   = pSrc->m_sliceSegmentCurEndCtuTsAddr;
  m_nextSlice                     = pSrc->m_nextSlice;
  m_nextSliceSegment              = pSrc->m_nextSliceSegment;

  for ( UInt e=0 ; e<NUM_REF_PIC_LIST_01 ; e++ )
  {
    for ( UInt n=0 ; n<MAX_NUM_REF ; n++ )
    {
      memcpy(m_weightPredTable[e][n], pSrc->m_weightPredTable[e][n], sizeof(WPScalingParam)*MAX_NUM_COMPONENT );
    }
  }

  for( UInt ch = 0 ; ch < MAX_NUM_CHANNEL_TYPE; ch++)
  {
    m_saoEnabledFlag[ch] = pSrc->m_saoEnabledFlag[ch];
  }

  m_cabacInitFlag                 = pSrc->m_cabacInitFlag;

  m_bLMvdL1Zero                   = pSrc->m_bLMvdL1Zero;
  m_LFCrossSliceBoundaryFlag      = pSrc->m_LFCrossSliceBoundaryFlag;
  m_enableTMVPFlag                = pSrc->m_enableTMVPFlag;
  m_maxNumMergeCand               = pSrc->m_maxNumMergeCand;
  m_encCABACTableIdx              = pSrc->m_encCABACTableIdx;

#if NH_MV
  // Additional slice header syntax elements 
  m_pocResetFlag               = pSrc->m_pocResetFlag; 
  m_discardableFlag            = pSrc->m_discardableFlag; 
  m_interLayerPredEnabledFlag  = pSrc->m_interLayerPredEnabledFlag; 
  m_numInterLayerRefPicsMinus1 = pSrc->m_numInterLayerRefPicsMinus1;

  for (Int layer = 0; layer < MAX_NUM_LAYERS; layer++ )
  {
    m_interLayerPredLayerIdc[ layer ] = pSrc->m_interLayerPredLayerIdc[ layer ]; 
  }
#endif
#if NH_3D_DIS
  m_bApplyDIS = pSrc->m_bApplyDIS;
#endif
#if NH_3D_IC
  m_bApplyIC = pSrc->m_bApplyIC;
  m_icSkipParseFlag = pSrc->m_icSkipParseFlag;
#endif

}


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
Bool TComSlice::isTemporalLayerSwitchingPoint(TComList<TComPic*>& rcListPic)
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
Bool TComSlice::isStepwiseTemporalLayerSwitchingPointCandidate(TComList<TComPic*>& rcListPic)
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
    if(!rpcPic->getReconMark())
    {
      continue;
    }
    if (rpcPic->getPOC() == this->getPOC())
    {
      continue;
    }

    // Any picture that has PicOutputFlag equal to 1 that precedes an IRAP picture
    // in decoding order shall precede the IRAP picture in output order.
    // (Note that any picture following in output order would be present in the DPB)
    if(rpcPic->getSlice(0)->getPicOutputFlag() == 1 && !this->getNoOutputPriorPicsFlag())
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
Void TComSlice::applyReferencePictureSet( TComList<TComPic*>& rcListPic, const TComReferencePictureSet *pReferencePictureSet)
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
#if NH_MV
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
Int TComSlice::checkThatAllRefPicsAreAvailable( TComList<TComPic*>& rcListPic, const TComReferencePictureSet *pReferencePictureSet, Bool printErrors, Int pocRandomAccess, Bool bUseRecoveryPoint)
{
  Int atLeastOneUnabledByRecoveryPoint = 0;
  Int atLeastOneFlushedByPreviousIDR = 0;
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
          if(bUseRecoveryPoint && this->getPOC() > pocRandomAccess && this->getPOC() + pReferencePictureSet->getDeltaPOC(i) < pocRandomAccess)
          {
            isAvailable = 0;
          }
          else
          {
            isAvailable = 1;
          }
        }
      }
      else
      {
        Int pocCycle = 1<<rpcPic->getPicSym()->getSlice(0)->getSPS()->getBitsForPOC();
        Int curPoc = rpcPic->getPicSym()->getSlice(0)->getPOC() & (pocCycle-1);
        Int refPoc = pReferencePictureSet->getPOC(i) & (pocCycle-1);
        if(rpcPic->getIsLongTerm() && curPoc == refPoc && rpcPic->getSlice(0)->isReferenced())
        {
          if(bUseRecoveryPoint && this->getPOC() > pocRandomAccess && this->getPOC() + pReferencePictureSet->getDeltaPOC(i) < pocRandomAccess)
          {
            isAvailable = 0;
          }
          else
          {
            isAvailable = 1;
          }
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
      else if(bUseRecoveryPoint && this->getPOC() > pocRandomAccess)
      {
        atLeastOneUnabledByRecoveryPoint = 1;
      }
      else if(bUseRecoveryPoint && (this->getAssociatedIRAPType()==NAL_UNIT_CODED_SLICE_IDR_N_LP || this->getAssociatedIRAPType()==NAL_UNIT_CODED_SLICE_IDR_W_RADL))
      {
        atLeastOneFlushedByPreviousIDR = 1;
      }
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
        if(bUseRecoveryPoint && this->getPOC() > pocRandomAccess && this->getPOC() + pReferencePictureSet->getDeltaPOC(i) < pocRandomAccess)
        {
          isAvailable = 0;
        }
        else
        {
          isAvailable = 1;
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
      else if(bUseRecoveryPoint && this->getPOC() > pocRandomAccess)
      {
        atLeastOneUnabledByRecoveryPoint = 1;
      }
      else if(bUseRecoveryPoint && (this->getAssociatedIRAPType()==NAL_UNIT_CODED_SLICE_IDR_N_LP || this->getAssociatedIRAPType()==NAL_UNIT_CODED_SLICE_IDR_W_RADL))
      {
        atLeastOneFlushedByPreviousIDR = 1;
      }
    }
  }

  if(atLeastOneUnabledByRecoveryPoint || atLeastOneFlushedByPreviousIDR)
  {
    return -1;
  }    
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
Void TComSlice::createExplicitReferencePictureSetFromReference( TComList<TComPic*>& rcListPic, const TComReferencePictureSet *pReferencePictureSet, Bool isRAP, Int pocRandomAccess, Bool bUseRecoveryPoint, const Bool bEfficientFieldIRAPEnabled)
{
  TComPic* rpcPic;
  Int i, j;
  Int k = 0;
  Int nrOfNegativePictures = 0;
  Int nrOfPositivePictures = 0;
  TComReferencePictureSet* pLocalRPS = this->getLocalRPS();
  (*pLocalRPS)=TComReferencePictureSet();

  Bool irapIsInRPS = false; // Used when bEfficientFieldIRAPEnabled==true

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
        pLocalRPS->setDeltaPOC(k, pReferencePictureSet->getDeltaPOC(i));
        pLocalRPS->setUsed(k, pReferencePictureSet->getUsed(i) && (!isRAP));
        if (bEfficientFieldIRAPEnabled)
        {
          pLocalRPS->setUsed(k, pLocalRPS->getUsed(k) && !(bUseRecoveryPoint && this->getPOC() > pocRandomAccess && this->getPOC() + pReferencePictureSet->getDeltaPOC(i) < pocRandomAccess) );
        }

        if(pLocalRPS->getDeltaPOC(k) < 0)
        {
          nrOfNegativePictures++;
        }
        else
        {
          if(bEfficientFieldIRAPEnabled && rpcPic->getPicSym()->getSlice(0)->getPOC() == this->getAssociatedIRAPPOC() && this->getAssociatedIRAPPOC() == this->getPOC()+1)
          {
            irapIsInRPS = true;
          }
          nrOfPositivePictures++;
        }
        k++;
      }
    }
  }

  Bool useNewRPS = false;
  // if current picture is complimentary field associated to IRAP, add the IRAP to its RPS. 
  if(bEfficientFieldIRAPEnabled && m_pcPic->isField() && !irapIsInRPS)
  {
    TComList<TComPic*>::iterator iterPic = rcListPic.begin();
    while ( iterPic != rcListPic.end())
    {
      rpcPic = *(iterPic++);
      if(rpcPic->getPicSym()->getSlice(0)->getPOC() == this->getAssociatedIRAPPOC() && this->getAssociatedIRAPPOC() == this->getPOC()+1)
      {
        pLocalRPS->setDeltaPOC(k, 1);
        pLocalRPS->setUsed(k, true);
        nrOfPositivePictures++;
        k ++;
        useNewRPS = true;
      }
    }
  }
  pLocalRPS->setNumberOfNegativePictures(nrOfNegativePictures);
  pLocalRPS->setNumberOfPositivePictures(nrOfPositivePictures);
  pLocalRPS->setNumberOfPictures(nrOfNegativePictures+nrOfPositivePictures);
  // This is a simplistic inter rps example. A smarter encoder will look for a better reference RPS to do the
  // inter RPS prediction with.  Here we just use the reference used by pReferencePictureSet.
  // If pReferencePictureSet is not inter_RPS_predicted, then inter_RPS_prediction is for the current RPS also disabled.
  if (!pReferencePictureSet->getInterRPSPrediction() || useNewRPS )
  {
    pLocalRPS->setInterRPSPrediction(false);
    pLocalRPS->setNumRefIdc(0);
  }
  else
  {
    Int rIdx =  this->getRPSidx() - pReferencePictureSet->getDeltaRIdxMinus1() - 1;
    Int deltaRPS = pReferencePictureSet->getDeltaRPS();
    const TComReferencePictureSet* pcRefRPS = this->getSPS()->getRPSList()->getReferencePictureSet(rIdx);
    Int iRefPics = pcRefRPS->getNumberOfPictures();
    Int iNewIdc=0;
    for(i=0; i<= iRefPics; i++)
    {
      Int deltaPOC = ((i != iRefPics)? pcRefRPS->getDeltaPOC(i) : 0);  // check if the reference abs POC is >= 0
      Int iRefIdc = 0;
      for (j=0; j < pLocalRPS->getNumberOfPictures(); j++) // loop through the  pictures in the new RPS
      {
        if ( (deltaPOC + deltaRPS) == pLocalRPS->getDeltaPOC(j))
        {
          if (pLocalRPS->getUsed(j))
          {
            iRefIdc = 1;
          }
          else
          {
            iRefIdc = 2;
          }
        }
      }
      pLocalRPS->setRefIdc(i, iRefIdc);
      iNewIdc++;
    }
    pLocalRPS->setInterRPSPrediction(true);
    pLocalRPS->setNumRefIdc(iNewIdc);
    pLocalRPS->setDeltaRPS(deltaRPS);
    pLocalRPS->setDeltaRIdxMinus1(pReferencePictureSet->getDeltaRIdxMinus1() + this->getSPS()->getRPSList()->getNumberOfReferencePictureSets() - this->getRPSidx());
  }

  this->setRPS(pLocalRPS);
  this->setRPSidx(-1);
}

//! get AC and DC values for weighted pred
Void  TComSlice::getWpAcDcParam(WPACDCParam *&wp)
{
  wp = m_weightACDCParam;
}

//! init AC and DC values for weighted pred
Void  TComSlice::initWpAcDcParam()
{
  for(Int iComp = 0; iComp < MAX_NUM_COMPONENT; iComp++ )
  {
    m_weightACDCParam[iComp].iAC = 0;
    m_weightACDCParam[iComp].iDC = 0;
  }
}

//! get tables for weighted prediction
Void  TComSlice::getWpScaling( RefPicList e, Int iRefIdx, WPScalingParam *&wp )
{
  assert (e<NUM_REF_PIC_LIST_01);
  wp = m_weightPredTable[e][iRefIdx];
}

//! reset Default WP tables settings : no weight.
Void  TComSlice::resetWpScaling()
{
  for ( Int e=0 ; e<NUM_REF_PIC_LIST_01 ; e++ )
  {
    for ( Int i=0 ; i<MAX_NUM_REF ; i++ )
    {
      for ( Int yuv=0 ; yuv<MAX_NUM_COMPONENT ; yuv++ )
      {
        WPScalingParam  *pwp = &(m_weightPredTable[e][i][yuv]);
        pwp->bPresentFlag      = false;
        pwp->uiLog2WeightDenom = 0;
        pwp->uiLog2WeightDenom = 0;
        pwp->iWeight           = 1;
        pwp->iOffset           = 0;
      }
    }
  }
}

//! init WP table
Void  TComSlice::initWpScaling(const TComSPS *sps)
{
  const Bool bUseHighPrecisionPredictionWeighting = sps->getSpsRangeExtension().getHighPrecisionOffsetsEnabledFlag();
  for ( Int e=0 ; e<NUM_REF_PIC_LIST_01 ; e++ )
  {
    for ( Int i=0 ; i<MAX_NUM_REF ; i++ )
    {
      for ( Int yuv=0 ; yuv<MAX_NUM_COMPONENT ; yuv++ )
      {
        WPScalingParam  *pwp = &(m_weightPredTable[e][i][yuv]);
        if ( !pwp->bPresentFlag )
        {
          // Inferring values not present :
          pwp->iWeight = (1 << pwp->uiLog2WeightDenom);
          pwp->iOffset = 0;
        }

        const Int offsetScalingFactor = bUseHighPrecisionPredictionWeighting ? 1 : (1 << (sps->getBitDepth(toChannelType(ComponentID(yuv)))-8));

        pwp->w      = pwp->iWeight;
        pwp->o      = pwp->iOffset * offsetScalingFactor; //NOTE: This value of the ".o" variable is never used - .o is set immediately before it gets used
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
#if NH_MV
, m_uiMaxLayersMinus1         (  0)
#else
, m_uiMaxLayers               (  1)
#endif
, m_bTemporalIdNestingFlag    (false)
, m_numHrdParameters          (  0)
#if NH_MV
, m_maxLayerId             (  0)
#else
, m_maxNuhReservedZeroLayerId (  0)
#endif
, m_hrdParameters             ()
, m_hrdOpSetIdx               ()
, m_cprmsPresentFlag          ()
{
#if NH_MV
  m_vpsBaseLayerInternalFlag = true; 
  m_vpsBaseLayerAvailableFlag = true; 

  m_numViews = 0; 

#endif

  for( Int i = 0; i < MAX_TLAYER; i++)
  {
    m_numReorderPics[i] = 0;
    m_uiMaxDecPicBuffering[i] = 1;
    m_uiMaxLatencyIncrease[i] = 0;
  }
#if NH_MV
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
    for ( Int j = 0; j < MAX_VPS_NUH_LAYER_ID_PLUS1; j++)
    {
      m_profileTierLevelIdx[i][j] = -1; 
      m_outputLayerFlag[i][j] = false; 
    }
    m_altOutputLayerFlag[ i ]       = false; 
  }

  m_repFormatIdxPresentFlag = false; 
  m_maxOneActiveRefLayerFlag = false; 
  m_vpsPocLsbAlignedFlag  = false; 
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
#if NH_3D
    m_numRefListLayers[i] = 0; 
#endif
    m_vpsRepFormatIdx    [i] = 0; 
    m_pocLsbNotPresentFlag[i] = 0;    
    m_viewIdVal          [i] = 0; 

#if NH_3D
    m_viewIndex         [i] = -1; 
#endif

    for( Int j = 0; j < MAX_NUM_LAYERS; j++ )
    {
      m_directDependencyFlag[i][j] = false;
      m_directDependencyType[i][j] = -1; 
      m_dependencyFlag  [i][j]    = false; 
      m_idDirectRefLayer[i][j]    = -1; 
#if NH_3D
      m_idRefListLayer[i][j]    = -1; 
#endif
      m_idPredictedLayer[i][j]    = -1; 
      m_idRefLayer      [i][j]    = -1; 
      m_maxTidIlRefPicsPlus1[i][j]  = 7;
    }

    for( Int j = 0; j < MAX_NUM_SCALABILITY_TYPES; j++ )
    {
      m_dimensionId[i][j] = 0;
    }
#if NH_3D_ARP
#endif
  }  
#endif
}

TComVPS::~TComVPS()
{
}

#if NH_MV

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

  // The value of num_add_layer_sets shall be in the range of 0 to 1023, inclusive. 
  assert( getNumAddLayerSets() >= 0 && getNumAddLayerSets() <= 1023 ); 
  return true; 
}

Int TComVPS::getNumScalabilityTypes() const
{
  return scalTypeToScalIdx( ScalabilityType(MAX_NUM_SCALABILITY_TYPES) );
}

Int TComVPS::scalTypeToScalIdx( ScalabilityType scalType ) const
{
  assert( (Int)scalType >= 0 && (Int)scalType <= MAX_NUM_SCALABILITY_TYPES );
  assert( (Int)scalType == MAX_NUM_SCALABILITY_TYPES || getScalabilityMaskFlag( scalType ) );
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
#if NH_3D
    Int l = 0; 
#endif
    Int r = 0;
    Int p = 0;

    for( Int j = 0; j  <=  getMaxLayersMinus1(); j++ )
    {
      Int jNuhLid = getLayerIdInNuh( j );
      if( getDirectDependencyFlag( i , j ) )
      {
        m_idDirectRefLayer[iNuhLId][d++] = jNuhLid;
      }
#if NH_3D
      if( getDirectDependencyFlag( i , j ) && ( getDepthId( iNuhLId ) == getDepthId( jNuhLid ) ))
      {
        m_idRefListLayer [iNuhLId][l++] = jNuhLid;
      }
#endif

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
#if NH_3D
    m_numRefListLayers[ iNuhLId ] = l; 
#endif

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
}


Void     TComVPS::initNumViews( )
{
  m_numViews = 1; 
#if NH_3D
  AOF( m_viewOIdxList.size() == 0 );   
  m_viewOIdxList.push_back( 0 );        
#endif

  for( Int i = 0; i <=  getMaxLayersMinus1(); i++ )
  {
    Int lId = getLayerIdInNuh( i ); 
    if( i > 0 )
    {
      Bool newViewFlag = true; 
      for( Int j = 0; j < i; j++ )
      {
        if( getViewOrderIdx( lId )  ==  getViewOrderIdx( getLayerIdInNuh( j ) )  )
        {
          newViewFlag = false;
        }
      }
      if( newViewFlag )
      {
        m_numViews++;
#if NH_3D
        m_viewOIdxList.push_back( getViewOrderIdx( lId ) );        
#endif
      }
    }
  }
}


Int TComVPS::getScalabilityId( Int layerIdInVps, ScalabilityType scalType ) const
{
  return getScalabilityMaskFlag( scalType ) ? getDimensionId( layerIdInVps, scalTypeToScalIdx( scalType ) ) : 0;
}

#if NH_3D_VSO
Int TComVPS::getLayerIdInNuh( Int viewIndex, Bool depthFlag, Int auxId ) const
{
  Int foundLayerIdinNuh = -1; 

  for (Int layerIdInVps = 0 ; layerIdInVps <= getMaxLayersMinus1(); layerIdInVps++ )
  {
    Int layerIdInNuh = getLayerIdInNuh( layerIdInVps ); 
#if !NH_3D
    if( ( getViewIndex( layerIdInNuh ) == viewIndex ) && ( getAuxId( layerIdInNuh ) == ( depthFlag ? 2 : 0 ) )  )
#else
    if( ( getViewIndex( layerIdInNuh ) == viewIndex ) && ( getDepthId( layerIdInNuh ) == ( depthFlag ? 1 : 0 ) )  )
#endif
    {
      foundLayerIdinNuh = layerIdInNuh; 
      break; 
    }
  }
  return foundLayerIdinNuh;
}
#endif
#if NH_3D
Void TComVPS::createCamPars(Int iNumViews)
{
  m_numCp                     .resize( iNumViews );
  m_cpRefVoi                  .resize( iNumViews );
  m_cpInSliceSegmentHeaderFlag.resize( iNumViews );
  m_cpPresentFlag             .resize( iNumViews ); 
  m_aaaiCodedScale            .resize( iNumViews ); 
  m_aaaiCodedOffset           .resize( iNumViews ); 

  for ( Int i = 0; i < iNumViews ; i++ )
  {
    m_numCp                     [i] = 0; 
    m_cpRefVoi                  [i].resize( iNumViews );
    m_cpInSliceSegmentHeaderFlag[i] = false;         
    m_aaaiCodedScale            [i].resize( 2 );
    m_aaaiCodedOffset           [i].resize( 2 );        
    m_cpPresentFlag             [i].resize( iNumViews ); 

    for ( Int j = 0; j < iNumViews; j++)
    {
      m_cpRefVoi             [i][j] = 0; 
      m_cpPresentFlag        [i][j] = false; 
    }

    for ( Int j = 0; j < 2; j++ )
    {
      m_aaaiCodedScale       [i][j].resize( MAX_NUM_LAYERS );
      m_aaaiCodedOffset      [i][j].resize( MAX_NUM_LAYERS );

      for ( Int k = 0; k < MAX_NUM_LAYERS; k++ )
      {
        m_aaaiCodedScale [i][j][k] = 0;
        m_aaaiCodedOffset[i][j][k] = 0;
      }
    }
  }
}
#endif // NH_3D


Int TComVPS::xGetDimBitOffset( Int j ) const
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

Int TComVPS::inferDimensionId( Int i, Int j ) const
{
    return ( ( getLayerIdInNuh( i ) & ( (1 << xGetDimBitOffset( j + 1 ) ) - 1) ) >> xGetDimBitOffset( j ) ); 
}

Int TComVPS::inferLastDimsionIdLenMinus1() const
{
  return ( 5 - xGetDimBitOffset( getNumScalabilityTypes() - 1 ) ); 
}

Int TComVPS::getNumLayersInIdList( Int lsIdx ) const
{
  assert( lsIdx >= 0 ); 
  assert( lsIdx <= getNumLayerSets() ); 
  return (Int) m_layerSetLayerIdList[ lsIdx ].size(); 
}

Int    TComVPS::getNumOutputLayerSets() const
{
  return getNumAddOlss() + getNumLayerSets(); 
}



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
    if ( getNecessaryLayerFlag( i , j ))
    {
      m_targetDecLayerIdLists[i].push_back( m_layerSetLayerIdList[ lsIdx ][ j ] ); 
    }

    if( getOutputLayerFlag( i, j  ))
    {
      m_targetOptLayerIdLists[i].push_back( m_layerSetLayerIdList[ lsIdx ][ j ] );
    }
  }  
  assert( getNumOutputLayersInOutputLayerSet( i ) > 0 ); 
}

Bool TComVPS::inferOutputLayerFlag( Int i, Int j ) const
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

Int TComVPS::getMaxSubLayersInLayerSetMinus1( Int i ) const
{
  Int maxSLMinus1 = 0; 
  for( Int k = 0; k < getNumLayersInIdList( i ); k++ )
  {
    Int lId = m_layerSetLayerIdList[i][k];
    maxSLMinus1 = std::max( maxSLMinus1, getSubLayersVpsMaxMinus1( getLayerIdInVps( lId ) ));
  }
  return maxSLMinus1;
}

Bool TComVPS::getAltOutputLayerFlagVar( Int i ) const
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



Int TComVPS::inferProfileTierLevelIdx(Int i, Int j) const
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

Void TComVPS::printPTL() const
{
  std::vector<Int> idx; 
  std::vector<Int> num; 
  IntAry2d ptlInfo; 

  std::cout << std::right << std::setw(60) << std::setfill('-') << " " << std::setfill(' ') << std::endl << "PTLI" << std::endl; 

  for ( Int i = 0; i <= getVpsNumProfileTierLevelMinus1(); i++ )
  {
    std::vector<Int> curPtlInfo;
    const ProfileTierLevel* ptl = getPTL( i )->getGeneralPTL(); 
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

Void TComVPS::printLayerDependencies() const
{
  vector<Int> fullArray;
  vector<Int> range; 

#if NH_3D
  vector<Int> depthId; 
#endif

  vector<Int> viewOrderIndex;
  vector<Int> auxId;
  vector<Int> dependencyId; 
  vector<Int> viewId; 
  for (Int i = 0; i <= getMaxLayersMinus1(); i++ )
  {
    fullArray.push_back( getMaxLayersMinus1() + 1 ); 
    range.push_back( i ); 
    viewOrderIndex.push_back( getViewIndex   ( i ) );
    dependencyId  .push_back( getDependencyId( i ) );
    auxId         .push_back( getAuxId       ( i ) );      
    viewId        .push_back( getViewId      ( getLayerIdInNuh( i ) ) );
#if NH_3D  
    depthId.push_back( getDepthId( i ) );
#endif
  }
  std::cout << std::right << std::setw(60) << std::setfill('-') << " " << std::setfill(' ') << std::endl << "Layer Dependencies" << std::endl; 
  xPrintArray( "direct_dependency_flag", getMaxLayersMinus1()+1, range, fullArray, m_directDependencyFlag, false ); 
  xPrintArray( "DependencyFlag", getMaxLayersMinus1()+1, range, fullArray, m_dependencyFlag, false ); 
  xPrintArray( "layer_id_in_nuh", getMaxLayersMinus1()+1, m_layerIdInNuh, true  );     
  xPrintArray( "IdPredictedLayer", getMaxLayersMinus1() + 1, m_layerIdInNuh, m_numPredictedLayers, m_idPredictedLayer, true );
  xPrintArray( "IdRefLayer"      , getMaxLayersMinus1() + 1, m_layerIdInNuh, m_numRefLayers, m_idRefLayer, true );
  xPrintArray( "IdDirectRefLayer", getMaxLayersMinus1() + 1, m_layerIdInNuh, m_numDirectRefLayers, m_idDirectRefLayer, true );
#if NH_3D
  xPrintArray( "IdRefListLayer", getMaxLayersMinus1() + 1, m_layerIdInNuh, m_numRefListLayers, m_idRefListLayer, true );
#endif

  std::cout << std::endl;
}

Void TComVPS::printScalabilityId() const
{
  vector<Int> layerIdxInVps; 


  vector<Int> depthId; 
  vector<Int> viewOrderIndex;
  vector<Int> auxId;
  vector<Int> dependencyId; 
  vector<Int> viewId; 

  for (Int i = 0; i <= getMaxLayersMinus1(); i++ )
  {
    Int layerIdInNuh = getLayerIdInNuh( i );
    layerIdxInVps  .push_back( i ); 
    depthId       .push_back( getDepthId     ( layerIdInNuh ) );
    viewOrderIndex.push_back( getViewIndex   ( layerIdInNuh ) );
    dependencyId  .push_back( getDependencyId( layerIdInNuh ) );
    auxId         .push_back( getAuxId       ( layerIdInNuh ) );      
    viewId        .push_back( getViewId      ( layerIdInNuh ) );

  }

  std::cout << std::right << std::setw(60) << std::setfill('-') << " " << std::setfill(' ') << std::endl << "Scalability Ids" << std::endl; 
  xPrintArray( "layerIdxInVps"  , getMaxLayersMinus1()+1, layerIdxInVps,          false );
  xPrintArray( "layer_id_in_nuh", getMaxLayersMinus1()+1, m_layerIdInNuh, false );     

  xPrintArray( "DepthLayerFlag", getMaxLayersMinus1()+1, depthId       , false );     
  xPrintArray( "ViewOrderIdx"  , getMaxLayersMinus1()+1, viewOrderIndex, false );     
  xPrintArray( "DependencyId"  , getMaxLayersMinus1()+1, dependencyId  , false );     
  xPrintArray( "AuxId"         , getMaxLayersMinus1()+1, auxId         , false );     
  xPrintArray( "ViewIdVal"     , getMaxLayersMinus1()+1, viewId        , false );     

  std::cout << std::endl;
}

Void TComVPS::printLayerSets() const
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

#if NH_3D
Void TComVPS::initViewCompLayer()
{
  assert( m_viewCompLayerId.size() == 0 && m_viewCompLayerPresentFlag.size() == 0  );
  for( Int i = 0; i < getNumViews(); i++ )
  {
    m_viewCompLayerId         .push_back( std::vector<Int>(0)  );
    m_viewCompLayerPresentFlag.push_back( std::vector<Bool>(0) );      

    for( Int depFlag = 0; depFlag  <=  1; depFlag++ )
    {
      Int iViewOIdx = getViewOIdxList( i );
      Int layerId = -1;
      for( Int j = 0; j  <=  getMaxLayersMinus1(); j++ ) 
      {
        Int jNuhLId = getLayerIdInNuh( j );
        if( getVpsDepthFlag( jNuhLId ) == ( (Bool) depFlag )  &&  getViewOrderIdx( jNuhLId )  ==  iViewOIdx  
          &&  getDependencyId( jNuhLId )  ==  0  &&  getAuxId( jNuhLId )  ==  0 )
        {
          layerId = jNuhLId;
        }
      }
      m_viewCompLayerPresentFlag[ i ].push_back( layerId  !=  -1 );
      m_viewCompLayerId         [ i ].push_back( layerId );        
    }
  }
}

Int TComVPS::getVoiInVps(Int viewOIdx) const
{
  for ( Int i = 0; i < m_viewOIdxList.size(); i++ )
  {
    if  ( m_viewOIdxList[ i ] == viewOIdx )
    {
      return i; 
    }
  }
  assert( 0 );   
  return -1;
}

Void TComVPS::deriveCpPresentFlag()
{
  for( Int nInVps = 0; nInVps < getNumViews(); nInVps++  )
  {
    for( Int mInVps = 0; mInVps < getNumViews(); mInVps++ )
    {
      m_cpPresentFlag[nInVps][mInVps] = 0; 
    }
  }

  for( Int n = 1; n < getNumViews(); n++ )
  {
    Int iInVps = getVoiInVps(  getViewOIdxList( n ) );      
    for( Int m = 0; m < getNumCp( iInVps ); m++ )
    {
      m_cpPresentFlag[ iInVps ][ getVoiInVps( getCpRefVoi( iInVps, m ) ) ] = 1;
    }
  }
}

#endif
#endif // NH_MV

// ------------------------------------------------------------------------------------------------
// Sequence parameter set (SPS)
// ------------------------------------------------------------------------------------------------

TComSPSRExt::TComSPSRExt()
 : m_transformSkipRotationEnabledFlag   (false)
 , m_transformSkipContextEnabledFlag    (false)
// m_rdpcmEnabledFlag initialized below
 , m_extendedPrecisionProcessingFlag    (false)
 , m_intraSmoothingDisabledFlag         (false)
 , m_highPrecisionOffsetsEnabledFlag    (false)
 , m_persistentRiceAdaptationEnabledFlag(false)
 , m_cabacBypassAlignmentEnabledFlag    (false)
{
  for (UInt signallingModeIndex = 0; signallingModeIndex < NUMBER_OF_RDPCM_SIGNALLING_MODES; signallingModeIndex++)
  {
    m_rdpcmEnabledFlag[signallingModeIndex] = false;
  }
}

TComSPS::TComSPS()
: m_SPSId                     (  0)
, m_VPSId                     (  0)
, m_chromaFormatIdc           (CHROMA_420)
, m_uiMaxTLayers              (  1)
// Structure
, m_picWidthInLumaSamples     (352)
, m_picHeightInLumaSamples    (288)
, m_log2MinCodingBlockSize    (  0)
, m_log2DiffMaxMinCodingBlockSize(0)
, m_uiMaxCUWidth              ( 32)
, m_uiMaxCUHeight             ( 32)
, m_uiMaxTotalCUDepth         (  3)
, m_bLongTermRefsPresent      (false)
, m_uiQuadtreeTULog2MaxSize   (  0)
, m_uiQuadtreeTULog2MinSize   (  0)
, m_uiQuadtreeTUMaxDepthInter (  0)
, m_uiQuadtreeTUMaxDepthIntra (  0)
// Tool list
, m_usePCM                    (false)
, m_pcmLog2MaxSize            (  5)
, m_uiPCMLog2MinSize          (  7)
, m_bPCMFilterDisableFlag     (false)
, m_uiBitsForPOC              (  8)
, m_numLongTermRefPicSPS      (  0)
#if NH_MV
, m_numShortTermRefPicSets    (   0)
#endif
, m_uiMaxTrSize               ( 32)
, m_bUseSAO                   (false)
, m_bTemporalIdNestingFlag    (false)
, m_scalingListEnabledFlag    (false)
, m_useStrongIntraSmoothing   (false)
, m_vuiParametersPresentFlag  (false)
, m_vuiParameters             ()
#if NH_MV
, m_pcVPS                     ( NULL )
, m_spsInferScalingListFlag   ( false )
, m_spsScalingListRefLayerId  ( 0 )

, m_updateRepFormatFlag       ( false ) 
, m_spsRepFormatIdx           ( 0 )
, m_interViewMvVertConstraintFlag (false)
#endif

{
  for(Int ch=0; ch<MAX_NUM_CHANNEL_TYPE; ch++)
  {
    m_bitDepths.recon[ch] = 8;
#if O0043_BEST_EFFORT_DECODING
    m_bitDepths.stream[ch] = 8;
#endif
    m_pcmBitDepths[ch] = 8;
    m_qpBDOffset   [ch] = 0;
  }


  for ( Int i = 0; i < MAX_TLAYER; i++ )
  {
#if NH_MV
    m_uiSpsMaxLatencyIncreasePlus1[i] = 0;
#else
    m_uiMaxLatencyIncreasePlus1[i] = 0;
#endif
    m_uiMaxDecPicBuffering[i] = 1;
    m_numReorderPics[i]       = 0;
  }

  ::memset(m_ltRefPicPocLsbSps, 0, sizeof(m_ltRefPicPocLsbSps));
  ::memset(m_usedByCurrPicLtSPSFlag, 0, sizeof(m_usedByCurrPicLtSPSFlag));
#if NH_MV
  m_spsRangeExtensionsFlag     = false;
  m_spsMultilayerExtensionFlag = false;
  m_spsExtension5bits          = 0;
  m_sps3dExtensionFlag         = false; 

#endif

}

TComSPS::~TComSPS()
{
  m_RPSList.destroy();
}

Void  TComSPS::createRPSList( Int numRPS )
{
  m_RPSList.destroy();
  m_RPSList.create(numRPS);
}

const Int TComSPS::m_winUnitX[]={1,2,2,1};
const Int TComSPS::m_winUnitY[]={1,2,1,1};

TComPPSRExt::TComPPSRExt()
: m_log2MaxTransformSkipBlockSize      (2)
, m_crossComponentPredictionEnabledFlag(false)
, m_diffCuChromaQpOffsetDepth          (0)
, m_chromaQpOffsetListLen              (0)
// m_ChromaQpAdjTableIncludingNullEntry initialized below
// m_log2SaoOffsetScale initialized below
{
  m_ChromaQpAdjTableIncludingNullEntry[0].u.comp.CbOffset = 0; // Array includes entry [0] for the null offset used when cu_chroma_qp_offset_flag=0. This is initialised here and never subsequently changed.
  m_ChromaQpAdjTableIncludingNullEntry[0].u.comp.CrOffset = 0;
  for(Int ch=0; ch<MAX_NUM_CHANNEL_TYPE; ch++)
  {
    m_log2SaoOffsetScale[ch] = 0;
  }
}

TComPPS::TComPPS()
: m_PPSId                            (0)
, m_SPSId                            (0)
, m_picInitQPMinus26                 (0)
, m_useDQP                           (false)
, m_bConstrainedIntraPred            (false)
, m_bSliceChromaQpFlag               (false)
, m_uiMaxCuDQPDepth                  (0)
, m_chromaCbQpOffset                 (0)
, m_chromaCrQpOffset                 (0)
, m_numRefIdxL0DefaultActive         (1)
, m_numRefIdxL1DefaultActive         (1)
, m_TransquantBypassEnableFlag       (false)
, m_useTransformSkip                 (false)
, m_dependentSliceSegmentsEnabledFlag(false)
, m_tilesEnabledFlag                 (false)
, m_entropyCodingSyncEnabledFlag     (false)
, m_loopFilterAcrossTilesEnabledFlag (true)
, m_uniformSpacingFlag               (false)
, m_numTileColumnsMinus1             (0)
, m_numTileRowsMinus1                (0)
, m_signHideFlag                     (false)
, m_cabacInitPresentFlag             (false)
, m_sliceHeaderExtensionPresentFlag  (false)
, m_loopFilterAcrossSlicesEnabledFlag(false)
, m_listsModificationPresentFlag     (0)
, m_numExtraSliceHeaderBits          (0)
#if NH_MV
, m_ppsInferScalingListFlag          (false)
, m_ppsScalingListRefLayerId         (0)
, m_pocResetInfoPresentFlag          (false)
#endif
#if NH_3D_DLT
, m_cDLT                             ()
#endif
{
#if NH_MV
  m_ppsRangeExtensionsFlag     = false;
  m_ppsMultilayerExtensionFlag = false;
  m_pps3dExtensionFlag         = false;
  m_ppsExtension5bits          = 0;
#endif

}

TComPPS::~TComPPS()
{
}

#if NH_3D_DLT
TComDLT::TComDLT()
: m_bDltPresentFlag(false)
, m_iNumDepthViews(0)
, m_uiDepthViewBitDepth(8)
{
  for( Int i = 0; i < MAX_NUM_LAYERS; i++ )
  {
    m_bUseDLTFlag                 [i] = false;
    m_bInterViewDltPredEnableFlag [i] = false;
    m_bDltBitMapRepFlag           [i] = false;

    // allocate some memory and initialize with default mapping
    m_iNumDepthmapValues[i] = ((1 << m_uiDepthViewBitDepth)-1)+1;
    m_iDepthValue2Idx[i]    = std::vector<Int>(m_iNumDepthmapValues[i]);
    m_iIdx2DepthValue[i]    = std::vector<Int>(m_iNumDepthmapValues[i]);
    
    m_iDepthIdxToLayerId[i] = i;

    //default mapping
    for (Int d=0; d<m_iNumDepthmapValues[i]; d++)
    {
      m_iDepthValue2Idx[i][d] = d;
      m_iIdx2DepthValue[i][d] = d;
    }
  }
}

TComDLT::~TComDLT()
{
  
}

Void TComDLT::setDepthLUTs(Int layerIdInVps, std::vector<Int> idxToDepthValueTable, Int iNumDepthValues)
{
  if( iNumDepthValues == 0 ) // default mapping only
    return;

  // copy idx2DepthValue to internal array
  m_iIdx2DepthValue[layerIdInVps] = idxToDepthValueTable;

  UInt uiMaxDepthValue = ((1 << m_uiDepthViewBitDepth)-1);
  for(Int p=0; p<=uiMaxDepthValue; p++)
  {
    Int iIdxDown    = 0;
    Int iIdxUp      = iNumDepthValues-1;
    Bool bFound     = false;

    // iterate over indices to find lower closest depth
    Int i = 1;
    while(!bFound && i<iNumDepthValues)
    {
      if( m_iIdx2DepthValue[layerIdInVps][i] > p )
      {
        iIdxDown  = i-1;
        bFound    = true;
      }

      i++;
    }
    iIdxUp = bFound ?  iIdxDown + 1 : iNumDepthValues-1;

    // assign closer depth value/idx
    if( abs(p-m_iIdx2DepthValue[layerIdInVps][iIdxDown]) < abs(p-m_iIdx2DepthValue[layerIdInVps][iIdxUp]) )
    {
      m_iDepthValue2Idx[layerIdInVps][p] = iIdxDown;
    }
    else
    {
      m_iDepthValue2Idx[layerIdInVps][p] = iIdxUp;
    }

  }

  // update DLT variables
  m_iNumDepthmapValues[layerIdInVps] = iNumDepthValues;
}

Void TComDLT::getDeltaDLT( Int layerIdInVps, std::vector<Int> piDLTInRef, UInt uiDLTInRefNum, std::vector<Int>& riDeltaDLTOut, UInt&ruiDeltaDLTOutNum ) const
{
  Bool abBM0[ 256 ];
  Bool abBM1[ 256 ];
  
  memset( abBM0, 0, sizeof( abBM0 ));
  memset( abBM1, 0, sizeof( abBM1 ));
  
  // convert reference DLT to bit string
  for( Int i = 0; i < uiDLTInRefNum; i++ )
  {
    abBM0[ piDLTInRef[ i ] ] = true;
  }
  // convert internal DLT to bit string
  for( Int i = 0; i < m_iNumDepthmapValues[ layerIdInVps ]; i++ )
  {
    abBM1[ m_iIdx2DepthValue[ layerIdInVps ][ i ] ] = true;
  }
  
  ruiDeltaDLTOutNum = 0;
  for( Int i = 0; i < 256; i++ )
  {
    if( abBM0[ i ] ^ abBM1[ i ] )
    {
      riDeltaDLTOut[ ruiDeltaDLTOutNum++ ] = i;
    }
  }
}

Void TComDLT::setDeltaDLT( Int layerIdInVps, std::vector<Int> piDLTInRef, UInt uiDLTInRefNum, std::vector<Int> piDeltaDLTIn, UInt uiDeltaDLTInNum )
{
  Bool abBM0[ 256 ];
  Bool abBM1[ 256 ];
  
  memset( abBM0, 0, sizeof( abBM0 ));
  memset( abBM1, 0, sizeof( abBM1 ));
  
  // convert reference DLT to bit string
  for( Int i = 0; i < uiDLTInRefNum; i++ )
  {
    abBM0[ piDLTInRef[ i ] ] = true;
  }
  // convert delta DLT to bit string
  for( Int i = 0; i < uiDeltaDLTInNum; i++ )
  {
    abBM1[ piDeltaDLTIn[ i ] ] = true;
  }
  
  std::vector<Int> aiIdx2DepthValue(256, 0);
  UInt uiNumDepthValues = 0;
  std::fill(aiIdx2DepthValue.begin(), aiIdx2DepthValue.end(), 0);
  
  for( Int i = 0; i < 256; i++ )
  {
    if( abBM0[ i ] ^ abBM1[ i ] )
    {
      aiIdx2DepthValue[ uiNumDepthValues++ ] = i;
    }
  }
  
  // update internal tables
  setDepthLUTs(layerIdInVps, aiIdx2DepthValue, uiNumDepthValues);
}

#endif

#if NH_MV
Void TComSPS::inferRepFormat( TComVPS* vps, Int layerIdCurr, Bool encoder )
{
  if ( getMultiLayerExtSpsFlag() )
  {    
    Int            repFormatIdx = getUpdateRepFormatFlag() ?  getSpsRepFormatIdx() : vps->getVpsRepFormatIdx( vps->getLayerIdInVps( layerIdCurr ) ) ;
    const TComRepFormat* repFormat    = vps->getRepFormat( repFormatIdx ); 

    if ( encoder )
    {
      assert( getChromaFormatIdc() ==  (ChromaFormat) repFormat->getChromaFormatVpsIdc() );         
      //// ToDo: add when supported: 
      // assert( getSeperateColourPlaneFlag( repFormat->getSeparateColourPlaneVpsFlag() ) ; 

      assert( getPicWidthInLumaSamples()  ==  repFormat->getPicWidthVpsInLumaSamples()  ); 
      assert( getPicHeightInLumaSamples() == repFormat->getPicHeightVpsInLumaSamples() ); 

      assert( getBitDepth              ( CHANNEL_TYPE_LUMA ) == repFormat->getBitDepthVpsLumaMinus8()   + 8 ); 
      assert( getQpBDOffset            ( CHANNEL_TYPE_LUMA ) == (Int) (6*( getBitDepth( CHANNEL_TYPE_LUMA ) - 8 )) );

      assert( getBitDepth              ( CHANNEL_TYPE_CHROMA ) == repFormat->getBitDepthVpsChromaMinus8() + 8 ); 
      assert( getQpBDOffset            ( CHANNEL_TYPE_CHROMA ) == (Int) (6* ( getBitDepth( CHANNEL_TYPE_CHROMA ) -8 ) ) );
    }
    else
    {
      setChromaFormatIdc( (ChromaFormat) repFormat->getChromaFormatVpsIdc() );         
      //// ToDo: add when supported: 
      // setSeperateColourPlaneFlag( repFormat->getSeparateColourPlaneVpsFlag() ) ; 

      setPicWidthInLumaSamples ( repFormat->getPicWidthVpsInLumaSamples()  ); 
      setPicHeightInLumaSamples( repFormat->getPicHeightVpsInLumaSamples() ); 

      setBitDepth              ( CHANNEL_TYPE_LUMA, repFormat->getBitDepthVpsLumaMinus8()   + 8 ); 
      setQpBDOffset            ( CHANNEL_TYPE_LUMA, (Int) (6*( getBitDepth( CHANNEL_TYPE_LUMA ) - 8 )) );

      setBitDepth              ( CHANNEL_TYPE_CHROMA, repFormat->getBitDepthVpsChromaMinus8() + 8 ); 
      setQpBDOffset            ( CHANNEL_TYPE_CHROMA, (Int) (6* ( getBitDepth( CHANNEL_TYPE_CHROMA ) -8 ) ) );
      Window &spsConf    = getConformanceWindow();    

      // Scaled later
      spsConf.setScaledFlag( false ); 
      spsConf.setWindowLeftOffset  ( repFormat->getConfWinVpsLeftOffset()    );
      spsConf.setWindowRightOffset ( repFormat->getConfWinVpsRightOffset()   );
      spsConf.setWindowTopOffset   ( repFormat->getConfWinVpsTopOffset()     );
      spsConf.setWindowBottomOffset( repFormat->getConfWinVpsBottomOffset()  );            
    }

   if ( getMultiLayerExtSpsFlag() && getUpdateRepFormatFlag() )
    {
      assert( getChromaFormatIdc()      <=  repFormat->getChromaFormatVpsIdc()         ); 
      //// ToDo: add when supported: 
      // assert( getSeperateColourPlaneFlag() <=  repFormat->getSeparateColourPlaneVpsFlag() ) ; 

      assert( getPicWidthInLumaSamples()  <= repFormat->getPicWidthVpsInLumaSamples()    ); 
      assert( getPicHeightInLumaSamples() <= repFormat->getPicHeightVpsInLumaSamples()   ); 

      assert( getBitDepth( CHANNEL_TYPE_LUMA   )  <= repFormat->getBitDepthVpsLumaMinus8()   + 8 );         
      assert( getBitDepth( CHANNEL_TYPE_CHROMA )  <= repFormat->getBitDepthVpsChromaMinus8() + 8 ); 
    }
  }

  // Set conformance window
  Int scal = TComSPS::getWinUnitX( getChromaFormatIdc() ) ;
  getConformanceWindow().scaleOffsets( scal );
  getVuiParameters()->getDefaultDisplayWindow().scaleOffsets( scal );

  if (encoder && getMultiLayerExtSpsFlag() )
  { 
    Int            repFormatIdx = getUpdateRepFormatFlag() ?  getSpsRepFormatIdx() : vps->getVpsRepFormatIdx( vps->getLayerIdInVps( layerIdCurr ) ) ;
    const TComRepFormat* repFormat    = vps->getRepFormat( repFormatIdx ); 

    Window &spsConf    = getConformanceWindow();    
    assert( spsConf.getWindowLeftOffset  () == repFormat->getConfWinVpsLeftOffset()    );
    assert( spsConf.getWindowRightOffset () == repFormat->getConfWinVpsRightOffset()   );
    assert( spsConf.getWindowTopOffset   () == repFormat->getConfWinVpsTopOffset()     );
    assert( spsConf.getWindowBottomOffset() == repFormat->getConfWinVpsBottomOffset()  );            
  }
}

Void TComSPS::inferScalingList( const TComSPS* spsSrc )
{
  if ( getSpsInferScalingListFlag() ) 
  {
    assert( spsSrc != NULL ); 
    assert( !spsSrc->getSpsInferScalingListFlag() );             
    getScalingList().inferFrom( (spsSrc->getScalingList()) ); 
  }
}

Void TComSPS::inferSpsMaxDecPicBufferingMinus1( TComVPS* vps, Int targetOptLayerSetIdx, Int currLayerId, Bool encoder )
{
  if ( getMultiLayerExtSpsFlag() )
  {
    const std::vector<Int>& targetDecLayerIdList = vps->getTargetDecLayerIdList( vps->olsIdxToLsIdx( targetOptLayerSetIdx )); 
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

      // This preliminary fix needs to be checked.
      Int maxNumReorderPics       = vps->getDpbSize()->getMaxVpsNumReorderPics( targetOptLayerSetIdx, i ); 
      Int maxLatencyIncreasePlus1 = vps->getDpbSize()->getMaxVpsLatencyIncreasePlus1( targetOptLayerSetIdx, i ); 
      if ( encoder )      
      {
        assert( getMaxDecPicBuffering( i ) - 1 == maxDecPicBufferingMinus1 ); 
        // This preliminary fix needs to be checked.
        assert( getNumReorderPics( i )     == maxNumReorderPics       ); 
        assert( getSpsMaxLatencyIncreasePlus1( i ) == maxLatencyIncreasePlus1 ); 

      }
      else
      {
        // This preliminary fix needs to be checked.
        setMaxDecPicBuffering( maxDecPicBufferingMinus1 + 1 , i); 
        setNumReorderPics    ( maxNumReorderPics, i );
        setSpsMaxLatencyIncreasePlus1( maxLatencyIncreasePlus1 , i); 
      }
    }    
  }
}

Void TComSPS::checkRpsMaxNumPics( const TComVPS* vps, Int currLayerId ) const
{
  for (Int i = 0; i < getRPSList()->getNumberOfReferencePictureSets(); i++ )
  {
    const TComReferencePictureSet* rps = getRPSList()->getReferencePictureSet( i ); 
    if ( !rps->getInterRPSPrediction() )
    {
      rps->checkMaxNumPics( vps->getVpsExtensionFlag(), MAX_INT, getLayerId(), getMaxDecPicBuffering( getSpsMaxSubLayersMinus1() ) - 1 );   // INT_MAX to be replaced by DpbSize
    }
  }
}

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
  ::memset( m_bCheckLTMSB, 0, sizeof(m_bCheckLTMSB) );
  ::memset( m_pocLSBLT, 0, sizeof(m_pocLSBLT) );
  ::memset( m_deltaPOCMSBCycleLT, 0, sizeof(m_deltaPOCMSBCycleLT) );
  ::memset( m_deltaPocMSBPresentFlag, 0, sizeof(m_deltaPocMSBPresentFlag) );
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

Int TComReferencePictureSet::getUsed(Int bufferNum) const
{
  return m_used[bufferNum];
}

Int TComReferencePictureSet::getDeltaPOC(Int bufferNum) const
{
  return m_deltaPOC[bufferNum];
}

Int TComReferencePictureSet::getNumberOfPictures() const
{
  return m_numberOfPictures;
}

Int TComReferencePictureSet::getPOC(Int bufferNum) const
{
  return m_POC[bufferNum];
}

Void TComReferencePictureSet::setPOC(Int bufferNum, Int POC)
{
  m_POC[bufferNum] = POC;
}

Bool TComReferencePictureSet::getCheckLTMSBPresent(Int bufferNum) const
{
  return m_bCheckLTMSB[bufferNum];
}

Void TComReferencePictureSet::setCheckLTMSBPresent(Int bufferNum, Bool b)
{
  m_bCheckLTMSB[bufferNum] = b;
}

//! set the reference idc value at uiBufferNum entry to the value of iRefIdc
Void TComReferencePictureSet::setRefIdc(Int bufferNum, Int refIdc)
{
  m_refIdc[bufferNum] = refIdc;
}

//! get the reference idc value at uiBufferNum
Int  TComReferencePictureSet::getRefIdc(Int bufferNum) const
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
Void TComReferencePictureSet::printDeltaPOC() const
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

#if NH_MV
Void TComReferencePictureSet::checkMaxNumPics( Bool vpsExtensionFlag, Int maxNumPics, Int nuhLayerId, Int spsMaxDecPicBufferingMinus1 ) const
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




TComRefPicListModification::TComRefPicListModification()
: m_refPicListModificationFlagL0 (false)
, m_refPicListModificationFlagL1 (false)
{
  ::memset( m_RefPicSetIdxL0, 0, sizeof(m_RefPicSetIdxL0) );
  ::memset( m_RefPicSetIdxL1, 0, sizeof(m_RefPicSetIdxL1) );
}

TComRefPicListModification::~TComRefPicListModification()
{
}

TComScalingList::TComScalingList()
{
  for(UInt sizeId = 0; sizeId < SCALING_LIST_SIZE_NUM; sizeId++)
  {
    for(UInt listId = 0; listId < SCALING_LIST_NUM; listId++)
    {
      m_scalingListCoef[sizeId][listId].resize(min<Int>(MAX_MATRIX_COEF_NUM,(Int)g_scalingListSize[sizeId]));
    }
  }
}

/** set default quantization matrix to array
*/
Void TComScalingList::setDefaultScalingList()
{
  for(UInt sizeId = 0; sizeId < SCALING_LIST_SIZE_NUM; sizeId++)
  {
    for(UInt listId=0;listId<SCALING_LIST_NUM;listId++)
    {
      processDefaultMatrix(sizeId, listId);
    }
  }
}
/** check if use default quantization matrix
 * \returns true if use default quantization matrix in all size
*/
Bool TComScalingList::checkDefaultScalingList()
{
  UInt defaultCounter=0;

  for(UInt sizeId = 0; sizeId < SCALING_LIST_SIZE_NUM; sizeId++)
  {
    for(UInt listId=0;listId<SCALING_LIST_NUM;listId++)
    {
      if( !memcmp(getScalingListAddress(sizeId,listId), getScalingListDefaultAddress(sizeId, listId),sizeof(Int)*min(MAX_MATRIX_COEF_NUM,(Int)g_scalingListSize[sizeId])) // check value of matrix
     && ((sizeId < SCALING_LIST_16x16) || (getScalingListDC(sizeId,listId) == 16))) // check DC value
      {
        defaultCounter++;
      }
    }
  }

  return (defaultCounter == (SCALING_LIST_NUM * SCALING_LIST_SIZE_NUM )) ? false : true;
}

#if NH_MV
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
      if (rIdx == 0 && li == 0) m_apcRefPicList[li][rIdx]->print( 1 );
      m_apcRefPicList[li][rIdx]->print( 0 );      
        
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

TComPic* TComSlice::getRefPicSetInterLayer( Int setIdc, Int i ) const
{
  TComPic* pic = NULL; 
  if (setIdc == 0 )
  {
    pic = (*m_refPicSetInterLayer0)[ i ]; 
  }
  else if (setIdc == 1 )
  {
    pic = (*m_refPicSetInterLayer1)[ i ]; 
  }

  assert( pic != NULL );   

  return pic; 
}


TComPic* TComSlice::getPicFromRefPicSetInterLayer(Int setIdc, Int layerId ) const
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


Int  TComSlice::getRefLayerPicFlag( Int i ) const
{
  const TComVPS* vps = getVPS(); 
#if NH_3D
  Int refLayerIdx = vps->getLayerIdInVps( vps->getIdRefListLayer( getLayerId(), i ) ); 
#else
  Int refLayerIdx = vps->getLayerIdInVps( vps->getIdDirectRefLayer( getLayerId(), i ) ); 
#endif

  Bool refLayerPicFlag = ( vps->getSubLayersVpsMaxMinus1( refLayerIdx ) >=  getTLayer()  && ( getTLayer() == 0   ||
    vps->getMaxTidIlRefPicsPlus1( refLayerIdx, vps->getLayerIdInVps( getLayerId() )) > getTLayer() )); 
  return refLayerPicFlag;       
}    

Int TComSlice::getRefLayerPicIdc( Int j ) const
{  
  Int refLayerPicIdc = -1; 
  Int curj = 0; 
#if NH_3D
  for( Int i = 0;  i < getVPS()->getNumRefListLayers( getLayerId()) ; i++ )
#else
  for( Int i = 0;  i < getVPS()->getNumDirectRefLayers( getLayerId()) ; i++ )
#endif
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

Int  TComSlice::getNumRefLayerPics( ) const
{  
  Int numRefLayerPics = 0; 
#if NH_3D
  for( Int i = 0;  i < getVPS()->getNumRefListLayers( getLayerId()) ; i++ )
#else
  for( Int i = 0;  i < getVPS()->getNumDirectRefLayers( getLayerId()) ; i++ )
#endif
  {
    numRefLayerPics += getRefLayerPicFlag( i ); 
  }
  return numRefLayerPics; 
}



Int TComSlice::getNumActiveRefLayerPics() const
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
#if NH_3D
  else if( getVPS()->getMaxOneActiveRefLayerFlag() || getVPS()->getNumRefListLayers( getLayerId() ) == 1 )
#else
  else if( getVPS()->getMaxOneActiveRefLayerFlag() || getVPS()->getNumDirectRefLayers( getLayerId() ) == 1 )
#endif
  {
    numActiveRefLayerPics = 1; 
  }
  else
  {
    numActiveRefLayerPics = getNumInterLayerRefPicsMinus1() + 1; 
  }
  return numActiveRefLayerPics;
}

Int TComSlice::getRefPicLayerId( Int i ) const
{
#if NH_3D
  return getVPS()->getIdRefListLayer( getLayerId(), getInterLayerPredLayerIdc( i ) );
#else
  return getVPS()->getIdDirectRefLayer( getLayerId(), getInterLayerPredLayerIdc( i ) );
#endif
}
#endif
#if NH_3D_NBDV
Void TComSlice::setDefaultRefView()
{
  setDefaultRefViewIdx(-1);
  setDefaultRefViewIdxAvailableFlag(false); 

  Int valid = 0;
  Int DefaultRefViewIdx = -1;

  for(UInt curViewIdx = 0; curViewIdx < getViewIndex() && valid == 0; curViewIdx++)
  {
    for(Int iRefListId = 0; (iRefListId < (isInterB() ? 2 : 1)) && !isIntra() && valid == 0; iRefListId++)
    {
      RefPicList eRefPicList = RefPicList(iRefListId);
      Int        iNumRefPics = getNumRefIdx(eRefPicList);

      for(Int i = 0; i < iNumRefPics; i++)
      { 
        if(getPOC() == getRefPic(eRefPicList, i)->getPOC() && curViewIdx == getRefPic(eRefPicList, i)->getViewIndex())
        {
          valid = 1;
          DefaultRefViewIdx = curViewIdx;
          break;
        }
      }
    }
  }

  if(valid)
  {
    setDefaultRefViewIdx(DefaultRefViewIdx);
    setDefaultRefViewIdxAvailableFlag(true);
  }
}
#endif

#if NH_3D_ARP
Void TComSlice::setARPStepNum( TComPicLists*ivPicLists )
{
  Bool tempRefPicInListsFlag = false;
  if( !getIvResPredFlag() || this->isIRAP())
  {
    m_nARPStepNum = 0;
  }
  else
  {
    setFirstTRefIdx (REF_PIC_LIST_0, -1);
    setFirstTRefIdx (REF_PIC_LIST_1, -1);
    for ( Int refListIdx = 0; refListIdx < ((m_eSliceType==B_SLICE) ? 2 : 1); refListIdx++ )
    {
      Int diffPOC=MAX_INT;
      Int idx=-1;
      for(Int i = 0; i < getNumRefIdx(RefPicList(refListIdx)); i++ )
      {
        if ( getRefPic(RefPicList(refListIdx), i)->getPOC() != getPOC() )
        {
          if( abs(getRefPic(RefPicList(refListIdx), i)->getPOC() - getPOC()) < diffPOC)
          {
            diffPOC=abs(getRefPic(RefPicList(refListIdx), i)->getPOC() - getPOC());
            idx=i;
          }
        }
        if(idx>=0)
        {
          setFirstTRefIdx (RefPicList(refListIdx), idx);
        }
      }
    }
    tempRefPicInListsFlag = (getFirstTRefIdx(REF_PIC_LIST_0) >= 0 || getFirstTRefIdx(REF_PIC_LIST_1) >= 0) && getDefaultRefViewIdxAvailableFlag();
    m_nARPStepNum = tempRefPicInListsFlag ? 3 : 0;
  }

  if (tempRefPicInListsFlag)
  {
    for ( Int refListIdx = 0; refListIdx < ((m_eSliceType==B_SLICE) ? 2 : 1); refListIdx++ )
    {
      RefPicList eRefPicList = RefPicList( refListIdx );
      Int prevPOC = getRefPic(eRefPicList, getFirstTRefIdx(eRefPicList) )->getPOC();
      for( Int i = 0; i < getNumActiveRefLayerPics(); i++ )
      {
        Int layerIdInNuh = getRefPicLayerId( i );

        TComPic* picV = getIvPic( getIsDepth(), getVPS()->getViewIndex( layerIdInNuh ) );
        assert( picV != NULL ); 
        IntAry1d pocsInCurrRPSsPicV = picV->getSlice(0)->getPocsInCurrRPSs(); 
        Bool refRpRefAvailFlag = false; 
        for (Int idx = 0; idx < pocsInCurrRPSsPicV.size(); idx++)
        {
          if ( pocsInCurrRPSsPicV[idx] == prevPOC )
          {
            refRpRefAvailFlag = true; 
            break; 
          }
        }

        if (getFirstTRefIdx(eRefPicList) >= 0 && refRpRefAvailFlag )
        {
          m_arpRefPicAvailable[eRefPicList][layerIdInNuh] = true;
        }
        else
        {
          m_arpRefPicAvailable[eRefPicList][layerIdInNuh] = false;
        }
      }
    }
  }
  if( m_nARPStepNum > 1)
  {
    for(Int i = 0; i < getNumActiveRefLayerPics(); i ++ )
    {
      Int  iLayerId = getRefPicLayerId( i );
      Int  iViewIdx =   getVPS()->getViewIndex(iLayerId);
      Bool bIsDepth = ( getVPS()->getDepthId  ( iLayerId ) == 1 );
      if( iViewIdx<getViewIndex() && !bIsDepth )
      {
        setBaseViewRefPicList( ivPicLists->getSubDpb( iLayerId, false ), iViewIdx );
      }
    }
  }
}
#endif

#if NH_3D_IC
// This is an encoder only function and should be moved to TEncSlice or TEncSearch!!
Void TComSlice::xSetApplyIC(Bool bUseLowLatencyICEnc)
{
  if(bUseLowLatencyICEnc)
  {
    Bool existInterViewRef=false;
    TComPic* pcCurrPic = getPic();
    TComPic* pcRefPic = NULL;
    for ( Int i = 0; (i < getNumRefIdx( REF_PIC_LIST_0 )) && !existInterViewRef; i++ )
    {
      pcRefPic = getRefPic( REF_PIC_LIST_0, i );
      if ( pcRefPic != NULL )
      {
        if ( pcCurrPic->getViewIndex() != pcRefPic->getViewIndex() )
        {
          existInterViewRef = true;
        }
      }
    }

    for ( Int i = 0; (i < getNumRefIdx( REF_PIC_LIST_1 )) && !existInterViewRef; i++ )
    {
      pcRefPic = getRefPic( REF_PIC_LIST_1, i );
      if ( pcRefPic != NULL )
      {
        if ( pcCurrPic->getViewIndex() != pcRefPic->getViewIndex() )
        {
          existInterViewRef = true;        
        }
      }
    }

    if(!existInterViewRef)
    {
      m_bApplyIC = false;
    }
    else
    {
      Int curLayer=getDepth();
      if( curLayer>9) curLayer=9; // Max layer is 10

      m_bApplyIC = true;
      Int refLayer = curLayer-1;

      Int ICEnableCandidate = getICEnableCandidate(refLayer);
      Int ICEnableNum = getICEnableNum(refLayer);
      if( (refLayer>=0) && (ICEnableCandidate>0) )
      {    
        Double ratio=Double(ICEnableNum/Double(ICEnableCandidate));

        if( ratio > IC_LOW_LATENCY_ENCODING_THRESHOLD)
        {
          m_bApplyIC=true;
        }
        else
        {
          m_bApplyIC=false;
        }
      }
      setICEnableCandidate(curLayer, 0);
      setICEnableNum(curLayer, 0);
    }
  }
  else
  {        
    TComPic*    pcCurrPic = getPic();
    TComPicYuv* pcCurrPicYuv = pcCurrPic->getPicYuvOrg();
    
    // Get InterView Reference picture
    // !!!!! Assume only one Interview Reference Picture in L0
    // GT: Is this assumption correct?

    TComPicYuv* pcRefPicYuvOrg = NULL;
    for ( Int i = 0; i < getNumRefIdx( REF_PIC_LIST_0 ); i++ )
    {
      TComPic* pcRefPic = getRefPic( REF_PIC_LIST_0, i );
      if ( pcRefPic != NULL )
      {
        if ( pcCurrPic->getViewIndex() != pcRefPic->getViewIndex() )
        {
          pcRefPicYuvOrg = pcRefPic->getPicYuvOrg();
        }
      }
    }

    if ( pcRefPicYuvOrg != NULL )
    {
      // Histogram building - luminance
      Int iMaxPelValue = ( 1 << getSPS()->getBitDepth(CHANNEL_TYPE_LUMA) ); 
      Int *aiRefOrgHist = (Int *) xMalloc( Int,iMaxPelValue );
      Int *aiCurrHist   = (Int *) xMalloc( Int,iMaxPelValue );
      memset( aiRefOrgHist, 0, iMaxPelValue*sizeof(Int) );
      memset( aiCurrHist, 0, iMaxPelValue*sizeof(Int) );

      Int iWidth   = pcCurrPicYuv->getWidth(COMPONENT_Y);
      Int iHeight  = pcCurrPicYuv->getHeight(COMPONENT_Y);
      Pel* pCurrY   = pcCurrPicYuv->getAddr(COMPONENT_Y);
      Pel* pRefOrgY = pcRefPicYuvOrg->getAddr(COMPONENT_Y);
      Int iCurrStride = pcCurrPicYuv->getStride(COMPONENT_Y);
      Int iRefStride = pcRefPicYuvOrg->getStride(COMPONENT_Y);
      for ( Int y = 0; y < iHeight; y++ )
      {
        for ( Int x = 0; x < iWidth; x++ )
        {
          aiCurrHist[pCurrY[x]]++;
          aiRefOrgHist[pRefOrgY[x]]++;
        }
        pCurrY += iCurrStride;
        pRefOrgY += iRefStride;
      }

      // Histogram SAD
      Int iSumOrgSAD = 0;
      for ( Int i = 0; i < iMaxPelValue; i++ )
      {
        iSumOrgSAD += abs( aiCurrHist[i] - aiRefOrgHist[i] );
      }

      // Setting
      Double dThresholdOrgSAD = getIsDepth() ? 0.1 : 0.05;

      if ( iSumOrgSAD > Int( dThresholdOrgSAD * iWidth * iHeight ) )
      {
        m_bApplyIC = true;
      }
      else
      {
        m_bApplyIC = false;
      }

      xFree( aiCurrHist   );
      xFree( aiRefOrgHist );
    }
  }//if(bUseLowLatencyICEnc)
}
#endif
#if NH_3D_QTL
Void TComSlice::setIvPicLists( TComPicLists* m_ivPicLists )
{  
  for (Int i = 0; i < MAX_NUM_LAYERS; i++ )
  { 
    for ( Int depthId = 0; depthId < 2; depthId++ )
    {
      m_ivPicsCurrPoc[ depthId ][ i ] = ( i <= m_viewIndex ) ? m_ivPicLists->getPic( i, ( depthId == 1) , 0, getPOC() ) : NULL;
    }        
  }  
}
#endif
#if NH_3D
Void TComSlice::setDepthToDisparityLUTs()
{ 
  Bool setupLUT = false; 
  
  setupLUT = setupLUT || getViewSynthesisPredFlag( ); 

#if NH_3D_NBDV_REF
  setupLUT = setupLUT || getDepthRefinementFlag( );
#endif  

#if NH_3D_IV_MERGE
  setupLUT = setupLUT || ( getIvMvPredFlag() && getIsDepth() );
#endif

  Int bitDepthY = getSPS()->getBitDepth(CHANNEL_TYPE_LUMA);

  if( !setupLUT )
  {
    return; 
  }

  m_numViews = getVPS()->getNumViews(); 
  /// GT: Allocation should be moved to a better place later; 
  if ( m_depthToDisparityB == NULL )
  {
    m_depthToDisparityB = new Int*[ m_numViews ];
    for ( Int i = 0; i < getVPS()->getNumViews(); i++ )
    {
      m_depthToDisparityB[ i ] = new Int[ Int(1 << bitDepthY) ]; 
    }
  }

  
  if ( m_depthToDisparityF == NULL )
  {
    m_depthToDisparityF = new Int*[ m_numViews ];
    for ( Int i = 0; i < m_numViews; i++ )
    {
      m_depthToDisparityF[ i ] = new Int[ Int(1 << bitDepthY) ]; 
    }
  }

  assert( m_depthToDisparityB != NULL ); 
  assert( m_depthToDisparityF != NULL ); 

  const TComVPS* vps = getVPS(); 

  Int log2Div = bitDepthY - 1 + vps->getCpPrecision();
  Int voiInVps = vps->getVoiInVps( getViewIndex() ); 
  Bool camParaSH = vps->getCpInSliceSegmentHeaderFlag( voiInVps );  

  const IntAry1d codScale     = camParaSH ? m_aaiCodedScale [ 0 ] : vps->getCodedScale    ( voiInVps ); 
  const IntAry1d codOffset    = camParaSH ? m_aaiCodedOffset[ 0 ] : vps->getCodedOffset   ( voiInVps ); 
  const IntAry1d invCodScale  = camParaSH ? m_aaiCodedScale [ 1 ] : vps->getInvCodedScale ( voiInVps ); 
  const IntAry1d invCodOffset = camParaSH ? m_aaiCodedOffset[ 1 ] : vps->getInvCodedOffset( voiInVps ); 


  for (Int i = 0; i < voiInVps; i++)
  {
    Int iInVoi = vps->getVoiInVps( i ); 
#if ENC_DEC_TRACE && NH_MV_ENC_DEC_TRAC
    if ( g_traceCameraParameters )
    {
      std::cout << "Cp: " << codScale   [ iInVoi ] << " " <<    codOffset[ iInVoi ] << " "
                << invCodScale[ iInVoi ] << " " << invCodOffset[ iInVoi ] << " " << log2Div << std::endl ; 
    }
#endif
    for ( Int d = 0; d <= ( ( 1 << bitDepthY ) - 1 ); d++ )
    {      
      Int offset =    ( codOffset  [ iInVoi ] << bitDepthY ) + ( ( 1 << log2Div ) >> 1 );         
      m_depthToDisparityB[ iInVoi ][ d ] = ( codScale [ iInVoi ] * d + offset ) >> log2Div; 

      Int invOffset = ( invCodOffset[ iInVoi ] << bitDepthY ) + ( ( 1 << log2Div ) >> 1 );         
      m_depthToDisparityF[ iInVoi ][ d ] = ( invCodScale[ iInVoi ] * d + invOffset ) >> log2Div; 
    }
  }
}
#endif



#if NH_MV
Void TComSlice::checkCrossLayerBlaFlag() const
{
  // cross_layer_bla_flag shall be equal to 0 for pictures with nal_unit_type not equal to IDR_W_RADL or IDR_N_LP or with nuh_layer_id not equal to 0.
  if ( getLayerId() != 0 || getNalUnitType() != NAL_UNIT_CODED_SLICE_IDR_W_RADL || getNalUnitType() == NAL_UNIT_CODED_SLICE_IDR_N_LP )
  {
    assert( m_crossLayerBlaFlag == 0 ); 
  }
}

Bool TComSlice::inferPocMsbCycleValPresentFlag()
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


Void TComSlice::f834decProcForRefPicListConst()
{
  // This process is invoked at the beginning of the decoding process for each P or B slice.
  assert( getSliceType() == B_SLICE || getSliceType() == P_SLICE );

  // Reference pictures are addressed through reference indices as specified in clause 8.5.3.3.2. A reference index is an index into 
  // a reference picture list. When decoding a P slice, there is a single reference picture list RefPicList0. When decoding a B 
  // slice, there is a second independent reference picture list RefPicList1 in addition to RefPicList0.

  // At the beginning of the decoding process for each slice, the reference picture lists RefPicList0 and, for B slices, RefPicList1 
  // are derived as follows:

  // The variable NumRpsCurrTempList0 is set equal to Max( num_ref_idx_l0_active_minus1 + 1, NumPicTotalCurr )
  Int numRpsCurrTempList0 = std::max( getNumRefIdxL0ActiveMinus1() + 1, getNumPicTotalCurr() );

  // and the list RefPicListTemp0 is constructed as follows:
  std::vector<TComPic*> refPicListTemp0;
  refPicListTemp0.resize((MAX_NUM_REF+1),NULL);

  const TComDecodedRps* decRps = getPic()->getDecodedRps(); 

  const std::vector<TComPic*>& refPicSetStCurrBefore  = decRps->m_refPicSetStCurrBefore; 
  const std::vector<TComPic*>& refPicSetStCurrAfter   = decRps->m_refPicSetStCurrAfter;
  const std::vector<TComPic*>& refPicSetLtCurr        = decRps->m_refPicSetLtCurr; 

  const Int                    numPocStCurrBefore     = decRps->m_numPocStCurrBefore; 
  const Int                    numPocStCurrAfter      = decRps->m_numPocStCurrAfter;
  const Int                    numPocLtCurr           = decRps->m_numPocLtCurr;

  const Int                    numActiveRefLayerPics0 = decRps->m_numActiveRefLayerPics0;
  const Int                    numActiveRefLayerPics1 = decRps->m_numActiveRefLayerPics1;

  const std::vector<TComPic*>& refPicSetInterLayer0   = decRps->m_refPicSetInterLayer0;
  const std::vector<TComPic*>& refPicSetInterLayer1   = decRps->m_refPicSetInterLayer1;

  Int rIdx = 0;
  while( rIdx < numRpsCurrTempList0 )
  {
    for(Int  i = 0; i < numPocStCurrBefore  &&  rIdx < numRpsCurrTempList0; rIdx++, i++ )
    {
      refPicListTemp0[ rIdx ] = refPicSetStCurrBefore[ i ];
    }    

    for(Int  i = 0; i < numActiveRefLayerPics0; rIdx++, i++ )
    {
      refPicListTemp0[ rIdx ] = refPicSetInterLayer0[ i ];
    }

    for(Int  i = 0;  i < numPocStCurrAfter  &&  rIdx < numRpsCurrTempList0; rIdx++, i++ )  // (F 65)
    {
      refPicListTemp0[ rIdx ] = refPicSetStCurrAfter[ i ];
    }

    for(Int  i = 0; i < numPocLtCurr  &&  rIdx < numRpsCurrTempList0; rIdx++, i++ )
    {
      refPicListTemp0[ rIdx ] = refPicSetLtCurr[ i ]; 
    }

    for(Int  i = 0; i < numActiveRefLayerPics1; rIdx++, i++ )
    {
      refPicListTemp0[ rIdx ] = refPicSetInterLayer1[ i ];
    }
  }

  // The list RefPicList0 is constructed as follows:
  TComRefPicListModification* rplm  = getRefPicListModification(); 
  for( rIdx = 0; rIdx  <=  getNumRefIdxL0ActiveMinus1(); rIdx++ )      //  (F 66)
  {
    m_apcRefPicList[ 0 ][ rIdx ] = rplm->getRefPicListModificationFlagL0( ) ? refPicListTemp0[ rplm->getListEntryL0( rIdx )] : refPicListTemp0[ rIdx ];
    // The decoding process below slice level requires the status
    // of the reference pictures, when decoding the RPS. So store it here.
    m_bIsUsedAsLongTerm[ 0 ][ rIdx ] = m_apcRefPicList[ 0 ][ rIdx ]->getIsLongTerm();
    m_aiRefPOCList     [ 0 ][ rIdx ] = m_apcRefPicList[ 0 ][ rIdx ]->getPOC();
    m_aiRefLayerIdList [ 0 ][ rIdx ] = m_apcRefPicList[ 0 ][ rIdx ]->getLayerId();
  }  

  std::vector<TComPic*> refPicListTemp1;
  refPicListTemp1.resize((MAX_NUM_REF+1),NULL);

  if (getSliceType() == B_SLICE )
  {
    // When the slice is a B slice, the variable NumRpsCurrTempList1 is set equal to 
    // Max( num_ref_idx_l1_active_minus1 + 1, NumPicTotalCurr ) and the list RefPicListTemp1 is constructed as follows:       
    Int numRpsCurrTempList1 = std::max( getNumRefIdxL1ActiveMinus1() + 1, getNumPicTotalCurr() );

    rIdx = 0; 
    while( rIdx < numRpsCurrTempList1 )
    {
      for( Int i = 0; i < numPocStCurrAfter  &&  rIdx < numRpsCurrTempList1; rIdx++, i++ )
      {
        refPicListTemp1[ rIdx ] = refPicSetStCurrAfter[ i ]; 
      }
      for( Int i = 0; i< numActiveRefLayerPics1; rIdx++, i++ )
      {
        refPicListTemp1[ rIdx ] = refPicSetInterLayer1[ i ]; 
      }
      for( Int i = 0;  i < numPocStCurrBefore  &&  rIdx < numRpsCurrTempList1; rIdx++, i++ )  // (F 67)
      {
        refPicListTemp1[ rIdx ] = refPicSetStCurrBefore[ i ];
      }         
      for( Int i = 0; i < numPocLtCurr  &&  rIdx < numRpsCurrTempList1; rIdx++, i++ )
      {
        refPicListTemp1[ rIdx ] = refPicSetLtCurr[ i ];
      }
      for( Int i = 0; i< numActiveRefLayerPics0; rIdx++, i++ )
      {
        refPicListTemp1[ rIdx ] = refPicSetInterLayer0[ i ]; 
      }
    }
  }

  if (getSliceType() == B_SLICE )
  {
    //   When the slice is a B slice, the list RefPicList1 is constructed as follows:
    for( rIdx = 0; rIdx  <=  getNumRefIdxL1ActiveMinus1(); rIdx++ )      // (F 68)
    {
      m_apcRefPicList[ 1 ][ rIdx ] = rplm->getRefPicListModificationFlagL1() ? refPicListTemp1[ rplm->getListEntryL1( rIdx ) ] : refPicListTemp1[ rIdx ]; 

      // The decoding process below slice level requires the marking status
      // of the reference pictures, when decoding the RPS. So store it here.
      m_bIsUsedAsLongTerm[ 1 ][ rIdx ] = m_apcRefPicList[ 1 ][ rIdx ]->getIsLongTerm();
      m_aiRefPOCList     [ 1 ][ rIdx ] = m_apcRefPicList[ 1 ][ rIdx ]->getPOC();
      m_aiRefLayerIdList [ 1 ][ rIdx ] = m_apcRefPicList[ 1 ][ rIdx ]->getLayerId();
    }    
  }
}


Void TComSlice::cl834DecProcForRefPicListConst()
{
  // This process is invoked at the beginning of the decoding process for each P or B slice.
  assert( getSliceType() == B_SLICE || getSliceType() == P_SLICE );

  // Reference pictures are addressed through reference indices as specified in clause 8.5.3.3.2. A reference index is an index into 
  // a reference picture list. When decoding a P slice, there is a single reference picture list RefPicList0. When decoding a B 
  // slice, there is a second independent reference picture list RefPicList1 in addition to RefPicList0.

  // At the beginning of the decoding process for each slice, the reference picture lists RefPicList0 and, for B slices, RefPicList1 
  // are derived as follows:

  // The variable NumRpsCurrTempList0 is set equal to Max( num_ref_idx_l0_active_minus1 + 1, NumPicTotalCurr ) 
  Int numRpsCurrTempList0 = std::max( getNumRefIdxL0ActiveMinus1() + 1, getNumPicTotalCurr() );

  // and the list RefPicListTemp0 is constructed as follows:
  std::vector<TComPic*> refPicListTemp0; 
  refPicListTemp0.resize((MAX_NUM_REF+1),NULL);

  const TComDecodedRps* decRps = getPic()->getDecodedRps(); 

  const std::vector<TComPic*>& refPicSetStCurrBefore = decRps->m_refPicSetStCurrBefore; 
  const std::vector<TComPic*>& refPicSetStCurrAfter  = decRps->m_refPicSetStCurrAfter;
  const std::vector<TComPic*>& refPicSetLtCurr       = decRps->m_refPicSetLtCurr; 
    
  const Int                    numPocStCurrBefore    = decRps->m_numPocStCurrBefore; 
  const Int                    numPocStCurrAfter     = decRps->m_numPocStCurrAfter;
  const Int                    numPocLtCurr          = decRps->m_numPocLtCurr;

  Int rIdx = 0;
  while( rIdx < numRpsCurrTempList0 )
  {
    for(Int  i = 0; i < numPocStCurrBefore  &&  rIdx < numRpsCurrTempList0; rIdx++, i++ )
    {
      refPicListTemp0[ rIdx ] = refPicSetStCurrBefore[ i ];
    }    

    for(Int  i = 0;  i < numPocStCurrAfter  &&  rIdx < numRpsCurrTempList0; rIdx++, i++ )  // (8 8)
    {
      refPicListTemp0[ rIdx ] = refPicSetStCurrAfter[ i ];
    }

    for(Int  i = 0; i < numPocLtCurr  &&  rIdx < numRpsCurrTempList0; rIdx++, i++ )
    {
      refPicListTemp0[ rIdx ] = refPicSetLtCurr[ i ]; 
    }
  }

  // The list RefPicList0 is constructed as follows:

  TComRefPicListModification* rplm = getRefPicListModification(); 
  for( rIdx = 0; rIdx  <=  getNumRefIdxL0ActiveMinus1(); rIdx++ )      //   (8-9)
  {
    m_apcRefPicList[0][ rIdx ] = rplm->getRefPicListModificationFlagL0( ) ? refPicListTemp0[ rplm->getListEntryL0( rIdx )] : refPicListTemp0[ rIdx ];

    // The decoding process below slice level requires the marking status 
    // of the reference pictures, when decoding the RPS. So store it here.
    m_bIsUsedAsLongTerm[ 0 ][ rIdx ] = m_apcRefPicList[ 0 ][ rIdx ]->getIsLongTerm();
    m_aiRefPOCList     [ 0 ][ rIdx ] = m_apcRefPicList[ 0 ][ rIdx ]->getPOC();
    m_aiRefLayerIdList [ 0 ][ rIdx ] = m_apcRefPicList[ 0 ][ rIdx ]->getLayerId();
  }

  std::vector<TComPic*> refPicListTemp1;
  refPicListTemp1.resize((MAX_NUM_REF+1),NULL);

  if (getSliceType() == B_SLICE )
  {
    // When the slice is a B slice, the variable NumRpsCurrTempList1 is set equal to 
    // Max( num_ref_idx_l1_active_minus1 + 1, NumPicTotalCurr ) and the list RefPicListTemp1 is constructed as follows:
    Int numRpsCurrTempList1 = std::max( getNumRefIdxL1ActiveMinus1() + 1, getNumPicTotalCurr() );

    rIdx = 0; 
    while( rIdx < numRpsCurrTempList1 )
    {
      for( Int i = 0; i < numPocStCurrAfter  &&  rIdx < numRpsCurrTempList1; rIdx++, i++ )
      {
        refPicListTemp1[ rIdx ] = refPicSetStCurrAfter[ i ]; 
      }
      for( Int i = 0;  i < numPocStCurrBefore  &&  rIdx < numRpsCurrTempList1; rIdx++, i++ )  // (8-10)
      {
        refPicListTemp1[ rIdx ] = refPicSetStCurrBefore[ i ];
      }         
      for( Int i = 0; i < numPocLtCurr  &&  rIdx < numRpsCurrTempList1; rIdx++, i++ )
      {
        refPicListTemp1[ rIdx ] = refPicSetLtCurr[ i ];
      }
    }
  }

  if (getSliceType() == B_SLICE )
  {
    //   When the slice is a B slice, the list RefPicList1 is constructed as follows:
    for( rIdx = 0; rIdx  <=  getNumRefIdxL1ActiveMinus1(); rIdx++ )      // (F 68)
    {
      m_apcRefPicList[ 1 ][ rIdx ] = rplm->getRefPicListModificationFlagL1() ? refPicListTemp1[ rplm->getListEntryL1( rIdx ) ] : refPicListTemp1[ rIdx ]; 

      // The decoding process below slice level requires the marking status 
      // of the reference pictures, when decoding the RPS. So store it here.
      m_bIsUsedAsLongTerm[ 1 ][ rIdx ] = m_apcRefPicList[ 1 ][ rIdx ]->getIsLongTerm();
      m_aiRefPOCList     [ 1 ][ rIdx ] = m_apcRefPicList[ 1 ][ rIdx ]->getPOC();
      m_aiRefLayerIdList [ 1 ][ rIdx ] = m_apcRefPicList[ 1 ][ rIdx ]->getLayerId();
    }
  }
}

Int TComSlice::getNumPicTotalCurr() const
{
  Int numPicTotalCurr = 0;
#if NH_MV_FIX_NUM_POC_TOTAL_CUR
  if ( !isIdr()  )
  {
    const TComStRefPicSet* stRps = getStRps( getCurrRpsIdx() ); 
#endif
    for( Int i = 0; i < stRps->getNumNegativePicsVar(); i++ )
    {
      if( stRps->getUsedByCurrPicS0Var( i ) )
      {
        numPicTotalCurr++;
      }
    }
    for( Int i = 0; i < stRps->getNumPositivePicsVar(); i++)  //(7 55)
    {
      if( stRps->getUsedByCurrPicS1Var(i) )
      {
        numPicTotalCurr++;
      }
    }
    for( Int i = 0; i < getNumLongTermSps() + getNumLongTermPics(); i++ )
    {
      if( getUsedByCurrPicLtVar( i ) )
      {
        numPicTotalCurr++;
      }
    }
#if NH_MV_FIX_NUM_POC_TOTAL_CUR
  }
#endif

  if ( decProcAnnexF() )
  {
    numPicTotalCurr += getNumActiveRefLayerPics(); 
  }
  return numPicTotalCurr;
}



Int TComSlice::getPocLsbLtVar( Int i )
{
  Int pocLsbLtVar; 
  if (i < getNumLongTermSps() )
  {

    pocLsbLtVar = getSPS()->getLtRefPicPocLsbSps( getLtIdxSps( i ) ); 
  }
  else
  {
    pocLsbLtVar = getPocLsbLt( i ); 
  }
  return pocLsbLtVar;
}


Bool TComSlice::getUsedByCurrPicLtVar( Int i ) const
{
  Bool usedByCurrPicLtVar; 
  if (i < getNumLongTermSps() )
  {
    usedByCurrPicLtVar = getSPS()->getUsedByCurrPicLtSPSFlag( getLtIdxSps( i ) ); 
  }
  else
  {
    usedByCurrPicLtVar = getUsedByCurrPicLtFlag( i ); 
  }
  return usedByCurrPicLtVar;
}


Int TComSlice::getDeltaPocMsbCycleLtVar( Int i ) const
{
  Int deltaPocMsbCycleVar; 
  if (i == 0 || i == getNumLongTermSps() )
  {
    deltaPocMsbCycleVar = getDeltaPocMsbCycleLt( i ); 
  }
  else
  {
    deltaPocMsbCycleVar = getDeltaPocMsbCycleLt( i ) + getDeltaPocMsbCycleLtVar( i - 1 ); 
  }
  return deltaPocMsbCycleVar;
}

#endif


#if NH_3D
Void TComSlice::init3dToolParameters()
{
  Bool depthFlag = getIsDepth();

  Bool nRLLG0 =  ( getVPS()->getNumRefListLayers( getLayerId() ) > 0 );     

  const TComSps3dExtension* sps3dExt = getSPS()->getSps3dExtension();

  m_ivMvPredFlag           = sps3dExt->getIvMvPredFlag         ( depthFlag ) && nRLLG0                       ;                             
  m_ivMvScalingFlag        = sps3dExt->getIvMvScalingFlag      ( depthFlag )                                 ;                             
  m_ivResPredFlag          = sps3dExt->getIvResPredFlag        ( depthFlag ) && nRLLG0                       ;                               
  m_depthRefinementFlag    = sps3dExt->getDepthRefinementFlag  ( depthFlag )           && getInCompPredFlag() && m_cpAvailableFlag;
  m_viewSynthesisPredFlag  = sps3dExt->getViewSynthesisPredFlag( depthFlag ) && nRLLG0 && getInCompPredFlag() && m_cpAvailableFlag;
  m_depthBasedBlkPartFlag  = sps3dExt->getDepthBasedBlkPartFlag( depthFlag )           && getInCompPredFlag();                          
  m_mpiFlag                = sps3dExt->getMpiFlag              ( depthFlag )           && getInCompPredFlag();
  m_intraContourFlag       = sps3dExt->getIntraContourFlag     ( depthFlag )           && getInCompPredFlag();
  m_intraSdcWedgeFlag      = sps3dExt->getIntraSdcWedgeFlag    ( depthFlag )                                 ;                          
  m_qtPredFlag             = sps3dExt->getQtPredFlag           ( depthFlag )           && getInCompPredFlag();
  m_interSdcFlag           = sps3dExt->getInterSdcFlag         ( depthFlag )                                 ;  
  m_depthIntraSkipFlag     = sps3dExt->getDepthIntraSkipFlag   ( depthFlag )                                 ;                          

  m_subPbSize              =  1 << ( sps3dExt->getLog2SubPbSizeMinus3   ( depthFlag ) + 3 );  
  m_mpiSubPbSize           =  1 << ( sps3dExt->getLog2MpiSubPbSizeMinus3( depthFlag ) + 3 );


#if NH_3D_OUTPUT_ACTIVE_TOOLS
  std::cout << "Layer:                  :" << getLayerId()             << std::endl;
  std::cout << "DepthFlag:              :" << getIsDepth()             << std::endl;
  std::cout << "ViewOrderIdx:           :" << getViewIndex()           << std::endl;
  std::cout << "InterCmpPredAvailableFlag:" << getInCmpPredAvailFlag() << std::endl;
  std::cout << "InterCompPredFlag       :"  << getInCompPredFlag()     << std::endl;
  
  std::cout << "ivMvPredFlag            :" << m_ivMvPredFlag           << std::endl;
  std::cout << "ivMvScalingFlag         :" << m_ivMvScalingFlag        << std::endl;
  std::cout << "ivResPredFlag           :" << m_ivResPredFlag          << std::endl;
  std::cout << "depthRefinementFlag     :" << m_depthRefinementFlag    << std::endl;
  std::cout << "viewSynthesisPredFlag   :" << m_viewSynthesisPredFlag  << std::endl;
  std::cout << "depthBasedBlkPartFlag   :" << m_depthBasedBlkPartFlag  << std::endl;
  std::cout << "mpiFlag                 :" << m_mpiFlag                << std::endl;
  std::cout << "intraContourFlag        :" << m_intraContourFlag       << std::endl;
  std::cout << "intraSdcWedgeFlag       :" << m_intraSdcWedgeFlag      << std::endl;
  std::cout << "qtPredFlag              :" << m_qtPredFlag             << std::endl;
  std::cout << "interSdcFlag            :" << m_interSdcFlag           << std::endl;
  std::cout << "depthIntraSkipFlag      :" << m_depthIntraSkipFlag     << std::endl;    
  std::cout << "subPbSize               :" << m_subPbSize              << std::endl;
  std::cout << "mpiSubPbSize            :" << m_mpiSubPbSize           << std::endl;
#endif
}

Void TComSlice::deriveInCmpPredAndCpAvailFlag( )
{
  Int numCurCmpLIds = getIsDepth() ? 1 : getNumActiveRefLayerPics(); 
  std::vector<Int> curCmpLIds;
  if ( getIsDepth() )
  {
    curCmpLIds.push_back( getLayerId() );
  }
  else
  {
    for (Int i = 0; i < numCurCmpLIds; i++)
    {
      curCmpLIds.push_back( getRefPicLayerId( i ) );
    }
  }

  m_cpAvailableFlag = true;
  m_inCmpRefViewIdcs.clear();
  Bool allRefCmpLayersAvailFlag = true;

  for( Int i = 0; i <= numCurCmpLIds - 1; i++ )
  {      
    m_inCmpRefViewIdcs.push_back( getVPS()->getViewOrderIdx( curCmpLIds[ i ] ));
    if( !getVPS()->getCpPresentFlag( getVPS()->getVoiInVps( getViewIndex() ),  getVPS()->getVoiInVps( m_inCmpRefViewIdcs[ i ] ) ) )
    {
      m_cpAvailableFlag = false;
    }
    Bool refCmpCurLIdAvailFlag = false;
    if( getVPS()->getViewCompLayerPresentFlag( m_inCmpRefViewIdcs[ i ], !getIsDepth() ) )
    {
      Int j = getVPS()->getLayerIdInVps( getVPS()->getViewCompLayerId( m_inCmpRefViewIdcs[ i ],  !getIsDepth() ) );
      if  ( getVPS()->getDirectDependencyFlag( getVPS()->getLayerIdInVps( getLayerId() ) ,  j ) &&
        getVPS()->getSubLayersVpsMaxMinus1( j ) >= getTemporalId()   &&
        ( getTemporalId() == 0 || getVPS()->getMaxTidIlRefPicsPlus1( j , getVPS()->getLayerIdInVps( getLayerId() ) ) > getTemporalId() )        
        )
      {
        refCmpCurLIdAvailFlag = true;
      }
    }
    if( !refCmpCurLIdAvailFlag )
    {
      allRefCmpLayersAvailFlag = false;
    }
  }

  if( !allRefCmpLayersAvailFlag )
  {
    m_inCmpPredAvailFlag = false;
  }  
  else 
  {
    const TComSps3dExtension* sps3dExt = getSPS()->getSps3dExtension();
    if( !getIsDepth() )
    {
      m_inCmpPredAvailFlag = sps3dExt->getViewSynthesisPredFlag( getIsDepth() ) || 
        sps3dExt->getDepthBasedBlkPartFlag( getIsDepth() ) || 
        sps3dExt->getDepthRefinementFlag  ( getIsDepth() );                            
    }
    else
    {
      m_inCmpPredAvailFlag = sps3dExt->getIntraContourFlag( getIsDepth() ) || 
        sps3dExt->getQtPredFlag( getIsDepth() ) || 
        sps3dExt->getMpiFlag( getIsDepth() );                                  
    }
  }
}

Void TComSlice::checkInCompPredRefLayers()
{  
  if ( getInCompPredFlag() )
  {
    for (Int i = 0; i < getNumCurCmpLIds(); i++ )
    {      
      assert( getIvPic(!getIsDepth(), getInCmpRefViewIdcs( i ) ) != NULL );       
      //  It is a requirement of bitstream conformance that there 
      //  is a picture in the DPB with PicOrderCntVal equal to the PicOrderCntVal of the current picture, 
      //  and a nuh_layer_id value equal to ViewCompLayerId[ inCmpRefViewIdcs[ i ] ][ !DepthFlag ].
    }
  } 
}

Void TComSlice::setPocsInCurrRPSs()
{
  // Currently only needed at decoder side; 
  m_pocsInCurrRPSs.clear();    
  std::vector<TComPic*>** rpsCurr = getPic()->getDecodedRps()->m_refPicSetsCurr;
  for (Int i = 0 ; i < 3; i++ )
  {
    for( Int j = 0; j < rpsCurr[i]->size(); j++ )
    {
      m_pocsInCurrRPSs.push_back( (*rpsCurr[i])[j]->getPOC() ); 
    }
  }
}

#endif

/** get scaling matrix from RefMatrixID
 * \param sizeId    size index
 * \param listId    index of input matrix
 * \param refListId index of reference matrix
 */
Void TComScalingList::processRefMatrix( UInt sizeId, UInt listId , UInt refListId )
{
  ::memcpy(getScalingListAddress(sizeId, listId),((listId == refListId)? getScalingListDefaultAddress(sizeId, refListId): getScalingListAddress(sizeId, refListId)),sizeof(Int)*min(MAX_MATRIX_COEF_NUM,(Int)g_scalingListSize[sizeId]));
}

Void TComScalingList::checkPredMode(UInt sizeId, UInt listId)
{
  Int predListStep = (sizeId == SCALING_LIST_32x32? (SCALING_LIST_NUM/NUMBER_OF_PREDICTION_MODES) : 1); // if 32x32, skip over chroma entries.

  for(Int predListIdx = (Int)listId ; predListIdx >= 0; predListIdx-=predListStep)
  {
    if( !memcmp(getScalingListAddress(sizeId,listId),((listId == predListIdx) ?
      getScalingListDefaultAddress(sizeId, predListIdx): getScalingListAddress(sizeId, predListIdx)),sizeof(Int)*min(MAX_MATRIX_COEF_NUM,(Int)g_scalingListSize[sizeId])) // check value of matrix
     && ((sizeId < SCALING_LIST_16x16) || (getScalingListDC(sizeId,listId) == getScalingListDC(sizeId,predListIdx)))) // check DC value
    {
      setRefMatrixId(sizeId, listId, predListIdx);
      setScalingListPredModeFlag(sizeId, listId, false);
      return;
    }
  }
  setScalingListPredModeFlag(sizeId, listId, true);
}

static Void outputScalingListHelp(std::ostream &os)
{
  os << "The scaling list file specifies all matrices and their DC values; none can be missing,\n"
         "but their order is arbitrary.\n\n"
         "The matrices are specified by:\n"
         "<matrix name><unchecked data>\n"
         "  <value>,<value>,<value>,....\n\n"
         "  Line-feeds can be added arbitrarily between values, and the number of values needs to be\n"
         "  at least the number of entries for the matrix (superfluous entries are ignored).\n"
         "  The <unchecked data> is text on the same line as the matrix that is not checked\n"
         "  except to ensure that the matrix name token is unique. It is recommended that it is ' ='\n"
         "  The values in the matrices are the absolute values (0-255), not the delta values as\n"
         "  exchanged between the encoder and decoder\n\n"
         "The DC values (for matrix sizes larger than 8x8) are specified by:\n"
         "<matrix name>_DC<unchecked data>\n"
         "  <value>\n";

  os << "The permitted matrix names are:\n";
  for(UInt sizeIdc = 0; sizeIdc < SCALING_LIST_SIZE_NUM; sizeIdc++)
  {
    for(UInt listIdc = 0; listIdc < SCALING_LIST_NUM; listIdc++)
    {
      if ((sizeIdc!=SCALING_LIST_32x32) || (listIdc%(SCALING_LIST_NUM/NUMBER_OF_PREDICTION_MODES) == 0))
      {
        os << "  " << MatrixType[sizeIdc][listIdc] << '\n';
      }
    }
  }
}

Void TComScalingList::outputScalingLists(std::ostream &os) const
{
  for(UInt sizeIdc = 0; sizeIdc < SCALING_LIST_SIZE_NUM; sizeIdc++)
  {
    const UInt size = min(8,4<<(sizeIdc));
    for(UInt listIdc = 0; listIdc < SCALING_LIST_NUM; listIdc++)
    {
      if ((sizeIdc!=SCALING_LIST_32x32) || (listIdc%(SCALING_LIST_NUM/NUMBER_OF_PREDICTION_MODES) == 0))
      {
        const Int *src = getScalingListAddress(sizeIdc, listIdc);
        os << (MatrixType[sizeIdc][listIdc]) << " =\n  ";
        for(UInt y=0; y<size; y++)
        {
          for(UInt x=0; x<size; x++, src++)
          {
            os << std::setw(3) << (*src) << ", ";
          }
          os << (y+1<size?"\n  ":"\n");
        }
        if(sizeIdc > SCALING_LIST_8x8)
        {
          os << MatrixType_DC[sizeIdc][listIdc] << " = \n  " << std::setw(3) << getScalingListDC(sizeIdc, listIdc) << "\n";
        }
        os << "\n";
      }
    }
  }
}

Bool TComScalingList::xParseScalingList(const std::string &fileName)
{
  static const Int LINE_SIZE=1024;
  FILE *fp = NULL;
  TChar line[LINE_SIZE];

  if (fileName.empty())
  {
    fprintf(stderr, "Error: no scaling list file specified. Help on scaling lists being output\n");
    outputScalingListHelp(std::cout);
    std::cout << "\n\nExample scaling list file using default values:\n\n";
    outputScalingLists(std::cout);
    exit (1);
    return true;
  }
  else if ((fp = fopen(fileName.c_str(),"r")) == (FILE*)NULL)
  {
    fprintf(stderr, "Error: cannot open scaling list file %s for reading\n", fileName.c_str());
    return true;
  }

  for(UInt sizeIdc = 0; sizeIdc < SCALING_LIST_SIZE_NUM; sizeIdc++)
  {
    const UInt size = min(MAX_MATRIX_COEF_NUM,(Int)g_scalingListSize[sizeIdc]);

    for(UInt listIdc = 0; listIdc < SCALING_LIST_NUM; listIdc++)
    {
      Int * const src = getScalingListAddress(sizeIdc, listIdc);

      if ((sizeIdc==SCALING_LIST_32x32) && (listIdc%(SCALING_LIST_NUM/NUMBER_OF_PREDICTION_MODES) != 0)) // derive chroma32x32 from chroma16x16
      {
        const Int *srcNextSmallerSize = getScalingListAddress(sizeIdc-1, listIdc);
        for(UInt i=0; i<size; i++)
        {
          src[i] = srcNextSmallerSize[i];
        }
        setScalingListDC(sizeIdc,listIdc,(sizeIdc > SCALING_LIST_8x8) ? getScalingListDC(sizeIdc-1, listIdc) : src[0]);
      }
      else
      {
        {
          fseek(fp, 0, SEEK_SET);
          Bool bFound=false;
          while ((!feof(fp)) && (!bFound))
          {
            TChar *ret = fgets(line, LINE_SIZE, fp);
            TChar *findNamePosition= ret==NULL ? NULL : strstr(line, MatrixType[sizeIdc][listIdc]);
            // This could be a match against the DC string as well, so verify it isn't
            if (findNamePosition!= NULL && (MatrixType_DC[sizeIdc][listIdc]==NULL || strstr(line, MatrixType_DC[sizeIdc][listIdc])==NULL))
            {
              bFound=true;
            }
          }
          if (!bFound)
          {
            fprintf(stderr, "Error: cannot find Matrix %s from scaling list file %s\n", MatrixType[sizeIdc][listIdc], fileName.c_str());
            return true;
          }
        }
        for (UInt i=0; i<size; i++)
        {
          Int data;
          if (fscanf(fp, "%d,", &data)!=1)
          {
            fprintf(stderr, "Error: cannot read value #%d for Matrix %s from scaling list file %s at file position %ld\n", i, MatrixType[sizeIdc][listIdc], fileName.c_str(), ftell(fp));
            return true;
          }
          if (data<0 || data>255)
          {
            fprintf(stderr, "Error: QMatrix entry #%d of value %d for Matrix %s from scaling list file %s at file position %ld is out of range (0 to 255)\n", i, data, MatrixType[sizeIdc][listIdc], fileName.c_str(), ftell(fp));
            return true;
          }
          src[i] = data;
        }

        //set DC value for default matrix check
        setScalingListDC(sizeIdc,listIdc,src[0]);

        if(sizeIdc > SCALING_LIST_8x8)
        {
          {
            fseek(fp, 0, SEEK_SET);
            Bool bFound=false;
            while ((!feof(fp)) && (!bFound))
            {
              TChar *ret = fgets(line, LINE_SIZE, fp);
              TChar *findNamePosition= ret==NULL ? NULL : strstr(line, MatrixType_DC[sizeIdc][listIdc]);
              if (findNamePosition!= NULL)
              {
                // This won't be a match against the non-DC string.
                bFound=true;
              }
            }
            if (!bFound)
            {
              fprintf(stderr, "Error: cannot find DC Matrix %s from scaling list file %s\n", MatrixType_DC[sizeIdc][listIdc], fileName.c_str());
              return true;
            }
          }
          Int data;
          if (fscanf(fp, "%d,", &data)!=1)
          {
            fprintf(stderr, "Error: cannot read DC %s from scaling list file %s at file position %ld\n", MatrixType_DC[sizeIdc][listIdc], fileName.c_str(), ftell(fp));
            return true;
          }
          if (data<0 || data>255)
          {
            fprintf(stderr, "Error: DC value %d for Matrix %s from scaling list file %s at file position %ld is out of range (0 to 255)\n", data, MatrixType[sizeIdc][listIdc], fileName.c_str(), ftell(fp));
            return true;
          }
          //overwrite DC value when size of matrix is larger than 16x16
          setScalingListDC(sizeIdc,listIdc,data);
        }
      }
    }
  }
//  std::cout << "\n\nRead scaling lists of:\n\n";
//  outputScalingLists(std::cout);

  fclose(fp);
  return false;
}

#if NH_MV
Void TComScalingList::inferFrom( const TComScalingList& srcScLi )
{
  for(Int sizeId = 0; sizeId < SCALING_LIST_SIZE_NUM; sizeId++)
  {
    for(Int listId = 0; listId <  SCALING_LIST_NUM; listId++)
    {
      setRefMatrixId  (sizeId,listId, srcScLi.getRefMatrixId  (sizeId,listId));
      setScalingListDC(sizeId,listId, srcScLi.getScalingListDC(sizeId,listId));          
      ::memcpy(getScalingListAddress(sizeId, listId),srcScLi.getScalingListAddress(sizeId, listId),sizeof(Int)*min(MAX_MATRIX_COEF_NUM,(Int)g_scalingListSize[sizeId]));
    }
  }
}
#endif


/** get default address of quantization matrix
 * \param sizeId size index
 * \param listId list index
 * \returns pointer of quantization matrix
 */
const Int* TComScalingList::getScalingListDefaultAddress(UInt sizeId, UInt listId)
{
  const Int *src = 0;
  switch(sizeId)
  {
    case SCALING_LIST_4x4:
      src = g_quantTSDefault4x4;
      break;
    case SCALING_LIST_8x8:
    case SCALING_LIST_16x16:
    case SCALING_LIST_32x32:
      src = (listId < (SCALING_LIST_NUM/NUMBER_OF_PREDICTION_MODES) ) ? g_quantIntraDefault8x8 : g_quantInterDefault8x8;
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
 * \param listId index of input matrix
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
    for(UInt listId = 0; listId < SCALING_LIST_NUM; listId++)
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
#if !NH_MV
, m_activeSPSId(-1)
#endif
{
#if NH_MV  
  for (Int i = 0; i < MAX_NUM_LAYERS; i++ )
  {    
    m_activeSPSId[i] = -1; 
  }
#endif
}


ParameterSetManager::~ParameterSetManager()
{
}

//! activate a SPS from a active parameter sets SEI message
//! \returns true, if activation is successful
#if NH_MV
//Bool ParameterSetManager::activateSPSWithSEI(Int spsId, Int layerId )
#else
//Bool ParameterSetManager::activateSPSWithSEI(Int spsId)
#endif
//{
//TComSPS *sps = m_spsMap.getPS(spsId);
//  if (sps)
//  {
//    Int vpsId = sps->getVPSId();
//    TComVPS *vps = m_vpsMap.getPS(vpsId);
//    if (vps)
//    {
#if NH_MV
//      m_activeVPS = *(vps);
//      m_activeSPS[ layerId ] = *(sps);
#else
//      m_activeVPS = *(vps);
//      m_activeSPS = *(sps);
#endif
//      return true;
//    }
//    else
//    {
//      printf("Warning: tried to activate SPS using an Active parameter sets SEI message. Referenced VPS does not exist.");
//    }
//  }
//  else
//  {
//    printf("Warning: tried to activate non-existing SPS using an Active parameter sets SEI message.");
//  }
//  return false;
//}

//! activate a PPS and depending on isIDR parameter also SPS and VPS
//! \returns true, if activation is successful
#if NH_MV
Bool ParameterSetManager::activatePPS(Int ppsId, Bool isIRAP, Int layerId )
#else
Bool ParameterSetManager::activatePPS(Int ppsId, Bool isIRAP)
#endif
{
  TComPPS *pps = m_ppsMap.getPS(ppsId);
  if (pps)
  {
    Int spsId = pps->getSPSId();
#if NH_MV
    if (!isIRAP && (spsId != m_activeSPSId[ layerId ] ))
#else
    if (!isIRAP && (spsId != m_activeSPSId ))
#endif
    {
      printf("Warning: tried to activate PPS referring to a inactive SPS at non-IDR.");
    }
    else
    {
      TComSPS *sps = m_spsMap.getPS(spsId);
      if (sps)
      {
        Int vpsId = sps->getVPSId();
        if (!isIRAP && (vpsId != m_activeVPSId ))
        {
          printf("Warning: tried to activate PPS referring to a inactive VPS at non-IDR.");
        }
        else
        {
          TComVPS *vps =m_vpsMap.getPS(vpsId);
          if (vps)
          {
            m_activeVPSId = vpsId;
#if NH_MV
            m_activeSPSId[layerId] = spsId;
#else
            m_activeSPSId = spsId;
#endif
            return true;
          }
          else
          {
            printf("Warning: tried to activate PPS that refers to a non-existing VPS.");
          }
        }
      }
      else
      {
       printf("Warning: tried to activate a PPS that refers to a non-existing SPS.");
      }
    }
  }
  else
  {
    printf("Warning: tried to activate non-existing PPS.");
  }

  // Failed to activate if reach here.
#if NH_MV
  m_activeSPSId[layerId]=-1;
#else
  m_activeSPSId=-1;
#endif
  m_activeVPSId=-1;

  return false;
}

ProfileTierLevel::ProfileTierLevel()
  : m_profileSpace    (0)
  , m_tierFlag        (Level::MAIN)
  , m_profileIdc      (Profile::NONE)
  , m_levelIdc        (Level::NONE)
  , m_progressiveSourceFlag  (false)
  , m_interlacedSourceFlag   (false)
  , m_nonPackedConstraintFlag(false)
  , m_frameOnlyConstraintFlag(false)
#if NH_MV
  , m_intraConstraintFlag         ( false )
  , m_onePictureOnlyConstraintFlag( false )
  , m_lowerBitRateConstraintFlag  ( false )
  , m_max12bitConstraintFlag      ( false )
  , m_max10bitConstraintFlag      ( false )
  , m_max8bitConstraintFlag       ( false )
  , m_max422chromaConstraintFlag  ( false )
  , m_max420chromaConstraintFlag  ( false )
  , m_maxMonochromeConstraintFlag ( false )  
  , m_inbldFlag                   ( false )
#endif
{
  ::memset(m_profileCompatibilityFlag, 0, sizeof(m_profileCompatibilityFlag));
}

#if NH_MV
Bool ProfileTierLevel::getV2ConstraintsPresentFlag() const
{
  return ( 
    getProfileIdc( ) ==  4 || getProfileCompatibilityFlag( 4 ) || getProfileIdc( ) ==  5 || getProfileCompatibilityFlag( 5 )  ||
    getProfileIdc( ) ==  6 || getProfileCompatibilityFlag( 6 ) || getProfileIdc( ) ==  7 || getProfileCompatibilityFlag( 7 ) 
    );
}

Bool ProfileTierLevel::getInbldPresentFlag() const
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

Void calculateParameterSetChangedFlag(Bool &bChanged, const std::vector<UChar> *pOldData, const std::vector<UChar> &newData)
{
  if (!bChanged)
  {
    if ((pOldData==0 && pOldData!=0) || (pOldData!=0 && pOldData==0))
    {
      bChanged=true;
    }
    else if (pOldData!=0 && pOldData!=0)
    {
      // compare the two
      if (pOldData->size() != pOldData->size())
      {
        bChanged=true;
      }
      else
      {
        const UChar *pNewDataArray=&(newData)[0];
        const UChar *pOldDataArray=&(*pOldData)[0];
        if (memcmp(pOldDataArray, pNewDataArray, pOldData->size()))
        {
          bChanged=true;
        }
      }
    }
  }
}
#if NH_MV
Void TComPTL::inferGeneralValues(Bool profilePresentFlag, Int k, TComPTL* refPTL)
{
  ProfileTierLevel* refProfileTierLevel = NULL; 
  if ( k > 0 )
  {    
    assert( refPTL != NULL);
    refProfileTierLevel = refPTL->getGeneralPTL(); 
  }

  ProfileTierLevel* curProfileTierLevel = getGeneralPTL( ); 
  assert( curProfileTierLevel != NULL ); 

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

    assert( refProfileTierLevel != NULL ); 
    ProfileTierLevel* curProfileTierLevel = getSubLayerPTL( i ); 
    assert( curProfileTierLevel != NULL ); 
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

//! \}

#if NH_MV
Void TComVPSVUI::init( Int numLayerSets, Int maxNumSubLayers, Int maxNumLayers )
{
  m_crossLayerIrapAlignedFlag = true; 
  m_allLayersIdrAlignedFlag   = false; 
  m_bitRatePresentVpsFlag     = false;
  m_picRatePresentVpsFlag     = false;

  m_bitRatePresentFlag          .resize(numLayerSets); 
  m_picRatePresentFlag          .resize(numLayerSets); 
  m_avgBitRate                  .resize(numLayerSets); 
  m_maxBitRate                  .resize(numLayerSets); 
  m_constantPicRateIdc          .resize(numLayerSets); 
  m_avgPicRate                  .resize(numLayerSets); 

  for ( Int i = 0; i < numLayerSets; i++)
  { 
    m_bitRatePresentFlag          [i].resize( maxNumSubLayers); 
    m_picRatePresentFlag          [i].resize( maxNumSubLayers); 
    m_avgBitRate                  [i].resize( maxNumSubLayers); 
    m_maxBitRate                  [i].resize( maxNumSubLayers); 
    m_constantPicRateIdc          [i].resize( maxNumSubLayers); 
    m_avgPicRate                  [i].resize( maxNumSubLayers); 
    for ( Int j = 0; j < maxNumSubLayers; j++)
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

  m_tileBoundariesAlignedFlag             .resize( maxNumLayers );
  m_minSpatialSegmentOffsetPlus1          .resize( maxNumLayers );
  m_ctuBasedOffsetEnabledFlag             .resize( maxNumLayers );
  m_minHorizontalCtuOffsetPlus1           .resize( maxNumLayers );
  m_baseLayerParameterSetCompatibilityFlag.resize( maxNumLayers );

  for ( Int i = 0; i < maxNumLayers; i++)
  {          
    m_tileBoundariesAlignedFlag   [i].resize( maxNumLayers );
    m_minSpatialSegmentOffsetPlus1[i].resize( maxNumLayers );
    m_ctuBasedOffsetEnabledFlag   [i].resize( maxNumLayers );
    m_minHorizontalCtuOffsetPlus1 [i].resize( maxNumLayers );
    for ( Int j = 0; j < maxNumLayers; j++)
    {    
      m_tileBoundariesAlignedFlag   [i][j] = false;
      m_minSpatialSegmentOffsetPlus1[i][j] = 0;
      m_ctuBasedOffsetEnabledFlag   [i][j] = false;
      m_minHorizontalCtuOffsetPlus1 [i][j] = -1;
    }
    m_baseLayerParameterSetCompatibilityFlag[i] = false;
  }
  m_vpsVuiBspHrdPresentFlag = false; 
}

Void TComRepFormat::inferChromaAndBitDepth( const TComRepFormat* prevRepFormat )
{
    setChromaAndBitDepthVpsPresentFlag( prevRepFormat->getChromaAndBitDepthVpsPresentFlag() );
    setSeparateColourPlaneVpsFlag     ( prevRepFormat->getSeparateColourPlaneVpsFlag     () );
    setBitDepthVpsLumaMinus8          ( prevRepFormat->getBitDepthVpsLumaMinus8          () );
    setBitDepthVpsChromaMinus8        ( prevRepFormat->getBitDepthVpsChromaMinus8        () );
}

Void TComRepFormat::checkInferChromaAndBitDepth( const TComRepFormat* prevRepFormat ) const 
{
    assert( getChromaAndBitDepthVpsPresentFlag() == prevRepFormat->getChromaAndBitDepthVpsPresentFlag() );
    assert( getSeparateColourPlaneVpsFlag     () == prevRepFormat->getSeparateColourPlaneVpsFlag     () );
    assert( getBitDepthVpsLumaMinus8          () == prevRepFormat->getBitDepthVpsLumaMinus8          () );
    assert( getBitDepthVpsChromaMinus8        () == prevRepFormat->getBitDepthVpsChromaMinus8        () );
}

Int TComVpsVuiBspHrdParameters::getBspHrdIdxLen( const TComVPS* vps) const
{
  return gCeilLog2( vps->getNumHrdParameters() + getVpsNumAddHrdParams() );
}

Void TComVpsVuiBspHrdParameters::createAfterVpsNumAddHrdParams( const TComVPS* vps )
{
  m_offsetHrdParamIdx = vps->getNumHrdParameters(); 
  m_numHrdParam       = vps->getNumHrdParameters() + getVpsNumAddHrdParams() - m_offsetHrdParamIdx;
  m_numOls            = vps->getNumOutputLayerSets(); 

  m_cprmsAddPresentFlag .resize( m_numHrdParam );
  m_numSubLayerHrdMinus1.resize( m_numHrdParam );
  m_hrdParameters       .resize( m_numHrdParam );

  m_numSignalledPartitioningSchemes .resize( m_numOls ); 
  m_numPartitionsInSchemeMinus1     .resize( m_numOls );
  m_numBspSchedulesMinus1           .resize( m_numOls ); 
  m_bspHrdIdx                       .resize( m_numOls );
  m_bspSchedIdx                     .resize( m_numOls );
}

Void TComVpsVuiBspHrdParameters::createAfterNumSignalledPartitioningSchemes( const TComVPS* vps, Int h )
{
  m_numPartitionsInSchemeMinus1 [h].resize( getNumSignalledPartitioningSchemes(h) );
  m_layerIncludedInPartitionFlag[h].resize( getNumSignalledPartitioningSchemes(h) );    

  m_numBspSchedulesMinus1       [h].resize( getNumSignalledPartitioningSchemes(h) + 1 );
  for (Int i = 0; i < getNumSignalledPartitioningSchemes(h) + 1; i++)
  {
    Int tMax = vps->getMaxSubLayersInLayerSetMinus1( m_vps->olsIdxToLsIdx(h) ) + 1;
    m_numBspSchedulesMinus1[h][i].resize( tMax );
    m_bspHrdIdx            [h][i].resize( tMax );
    m_bspSchedIdx          [h][i].resize( tMax );
  }
}

Void TComVpsVuiBspHrdParameters::createAfterNumPartitionsInSchemeMinus1( const TComVPS* vps, Int h, Int j )
{
  m_layerIncludedInPartitionFlag[h][j].resize( getNumPartitionsInSchemeMinus1(h,j));
  for( Int k = 0; k < getNumPartitionsInSchemeMinus1(h,j); k++ )
  {
    m_layerIncludedInPartitionFlag[h][j][k].resize( m_vps->getNumLayersInIdList( vps->olsIdxToLsIdx(h)));
  }
}

Void TComVpsVuiBspHrdParameters::createAfterNumBspSchedulesMinus1( const TComVPS* vps, Int h, Int i, Int t )
{
  m_bspSchedIdx[h][i][t].resize( getNumBspSchedulesMinus1( h, i, t ) + 1 );
  m_bspHrdIdx  [h][i][t].resize( getNumBspSchedulesMinus1( h, i, t ) + 1 );
  for( Int j = 0; j < getNumBspSchedulesMinus1( h, i, t ) + 1; j++ )
  {
    m_bspSchedIdx[h][i][t][j].resize( getNumPartitionsInSchemeMinus1( h, i ) );
    m_bspHrdIdx  [h][i][t][j].resize( getNumPartitionsInSchemeMinus1( h, i ) );
  }
}


Void TComVUI::inferVideoSignalInfo( const TComVPS* vps, Int layerIdCurr )
{
  if ( layerIdCurr == 0 || !vps->getVpsVuiPresentFlag() ) 
  {
    return; 
  }

  const TComVPSVUI* vpsVui = vps->getVPSVUI(); 
  assert( vpsVui != NULL );  

  const TComVideoSignalInfo* videoSignalInfo = vpsVui->getVideoSignalInfo( vpsVui->getVpsVideoSignalInfoIdx( vps->getLayerIdInVps( layerIdCurr ) ) ); 
  assert( videoSignalInfo != NULL );

  setVideoFormat            ( videoSignalInfo->getVideoVpsFormat            () ); 
  setVideoFullRangeFlag     ( videoSignalInfo->getVideoFullRangeVpsFlag     () );
  setColourPrimaries        ( videoSignalInfo->getColourPrimariesVps        () );
  setTransferCharacteristics( videoSignalInfo->getTransferCharacteristicsVps() );
  setMatrixCoefficients     ( videoSignalInfo->getMatrixCoeffsVps           () );
}

Void TComDpbSize::init( Int numOutputLayerSets, Int maxNumLayerIds, Int maxNumSubLayers )  
{
  m_subLayerFlagInfoPresentFlag.resize( numOutputLayerSets );
  m_subLayerDpbInfoPresentFlag .resize( numOutputLayerSets );
  m_maxVpsDecPicBufferingMinus1.resize( numOutputLayerSets ); 
  m_maxVpsNumReorderPics       .resize( numOutputLayerSets );
  m_maxVpsLatencyIncreasePlus1 .resize( numOutputLayerSets );

  for (Int i = 0; i < numOutputLayerSets; i++ )
  {      
    m_subLayerFlagInfoPresentFlag[i]  = false;

    m_subLayerDpbInfoPresentFlag [i].resize( maxNumSubLayers );
    m_maxVpsDecPicBufferingMinus1[i].resize( maxNumLayerIds ); 
    m_maxVpsNumReorderPics       [i].resize( maxNumSubLayers );
    m_maxVpsLatencyIncreasePlus1 [i].resize( maxNumSubLayers );    

    for (Int j = 0; j < maxNumSubLayers; j++  )
    {        
      m_subLayerDpbInfoPresentFlag [i][j] = ( j == 0) ;
      m_maxVpsNumReorderPics       [i][j] = 0;
      m_maxVpsLatencyIncreasePlus1 [i][j] = 0;
    }

    for (Int k = 0; k < maxNumLayerIds; k++ )
    {
      m_maxVpsDecPicBufferingMinus1[i][k].resize( maxNumSubLayers );
      for (Int j = 0; j < maxNumSubLayers; j++  )
      {        
        m_maxVpsDecPicBufferingMinus1[i][k][j] = MIN_INT; 
      }
    }
  }
}


Int TComDpbSize::getVpsMaxLatencyPictures( Int i, Int j ) const
{
  return getMaxVpsNumReorderPics( i, j ) + getMaxVpsLatencyIncreasePlus1(i, j) - 1; 
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

Void TComStRefPicSet::inferRps( Int stRpsIdx, TComSPS* sps, Bool encoder )
{
  if ( getInterRefPicSetPredictionFlag() )
  {
    // When inter_ref_pic_set_prediction_flag is equal to 1, the variables DeltaPocS0[ stRpsIdx ][ i ], UsedByCurrPicS0[ stRpsIdx ][ i ], 
    // NumNegativePics[ stRpsIdx ], DeltaPocS1[ stRpsIdx ][ i ], UsedByCurrPicS1[ stRpsIdx ][ i ] and NumPositivePics[ stRpsIdx ] are 
    // derived as follows:

    Int i = 0;
    Int refRpsIdx = getRefRpsIdx( stRpsIdx );
    TComStRefPicSet* refRps = sps->getStRefPicSet( refRpsIdx ); 

    for( Int j = refRps->getNumPositivePicsVar( ) - 1; j >= 0; j-- ) 
    {
      Int dPoc = refRps->getDeltaPocS1Var( j ) + getDeltaRps();
      if( dPoc < 0  &&  getUseDeltaFlag( refRps->getNumNegativePicsVar( ) + j ) ) 
      {
        setDeltaPocS0Var     ( i, dPoc );
        setUsedByCurrPicS0Var( i++ , getUsedByCurrPicFlag( refRps->getNumNegativePicsVar( ) + j ) );
      }
    }
    if( getDeltaRps() < 0  && getUseDeltaFlag( refRps->getNumDeltaPocs() ) )   //   (7 59)
    {
      setDeltaPocS0Var( i,  getDeltaRps() ); 
      setUsedByCurrPicS0Var( i++ , getUsedByCurrPicFlag( refRps->getNumDeltaPocs() ) );
    }
    for( Int j = 0; j < refRps->getNumNegativePicsVar(); j++ )
    {
      Int dPoc = refRps->getDeltaPocS0Var( j ) + getDeltaRps(); 
      if( dPoc < 0  &&  getUseDeltaFlag( j ) ) 
      {
        setDeltaPocS0Var( i , dPoc);
        setUsedByCurrPicS0Var( i++ , getUsedByCurrPicFlag( j )) ;
      }
    }

    setNumNegativePicsVar( i );

    i = 0;
    for( Int j = refRps->getNumNegativePicsVar() - 1; j  >=  0; j-- )
    {
      Int dPoc = refRps->getDeltaPocS0Var( j ) + getDeltaRps(); 
      if( dPoc > 0  &&  getUseDeltaFlag( j ) )
      {
        setDeltaPocS1Var( i, dPoc );
        setUsedByCurrPicS1Var(  i++, getUsedByCurrPicFlag( j ) ) ;
      }
    }

    if( getDeltaRps() > 0  &&  getUseDeltaFlag( refRps->getNumDeltaPocs() ) ) //  (7 60)
    { 
      setDeltaPocS1Var( i , getDeltaRps() );
      setUsedByCurrPicS1Var( i++ , getUsedByCurrPicFlag( refRps->getNumDeltaPocs() ));
    }

    for( Int j = 0; j < refRps->getNumPositivePicsVar( ); j++) 
    {
      Int dPoc = refRps->getDeltaPocS1Var( j ) + getDeltaRps(); 
      if( dPoc > 0  &&  getUseDeltaFlag( refRps->getNumNegativePicsVar() + j ) ) 
      {
        setDeltaPocS1Var( i, dPoc);
        setUsedByCurrPicS1Var( i++, getUsedByCurrPicFlag( refRps->getNumNegativePicsVar() + j ));
      }
    }
    setNumPositivePicsVar(  i );
  }
  else
  {
    // When inter_ref_pic_set_prediction_flag is equal to 0, the variables NumNegativePics[ stRpsIdx ], NumPositivePics[ stRpsIdx ], 
    // UsedByCurrPicS0[ stRpsIdx ][ i ], UsedByCurrPicS1[ stRpsIdx ][ i ], DeltaPocS0[ stRpsIdx ][ i ] and DeltaPocS1[ stRpsIdx ][ i ] 
    // are derived as follows:

    setNumNegativePicsVar( getNumNegativePics( ) );        //  (7 61)
    setNumPositivePicsVar( getNumPositivePics( ) );         //  (7 62)

    for (Int i = 0 ; i < getNumNegativePics(); i++ )
    {      
      setUsedByCurrPicS0Var( i,  getUsedByCurrPicS0Flag( i ) ); //  (7 63)
      if (i == 0 )
      {
        setDeltaPocS0Var( i , -( getDeltaPocS0Minus1( i ) + 1 )); // (7 65)
      }
      else
      {
        setDeltaPocS0Var( i , getDeltaPocS0Var( i - 1 ) - ( getDeltaPocS0Minus1( i ) + 1 )); //  (7 67)
      }
    }

    for (Int i = 0 ; i < getNumPositivePics(); i++ )
    {      
      setUsedByCurrPicS1Var( i,  getUsedByCurrPicS1Flag( i ) ); //  (7 64)

      if (i == 0 )
      {
        setDeltaPocS1Var( i , getDeltaPocS1Minus1( i ) + 1    );      // (7 66)
      }
      else
      {
        setDeltaPocS1Var( i , getDeltaPocS1Var( i - 1 ) + ( getDeltaPocS1Minus1( i ) + 1 )); //  (7 68)
      }
    }
  }
}

#endif
