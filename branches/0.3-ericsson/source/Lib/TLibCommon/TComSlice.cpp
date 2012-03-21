/* The copyright in this software is being made available under the BSD
 * License, included below. This software may be subject to other third party
 * and contributor rights, including patent rights, and no such rights are
 * granted under this license.
 *
 * Copyright (c) 2010-2011, ISO/IEC
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



/** \file     TComSlice.cpp
    \brief    slice header and SPS class
*/

#include "CommonDef.h"
#include "TComSlice.h"
#include "TComPic.h"

TComSlice::TComSlice()
{
  m_uiPPSId             = 0;
  m_iPOC                = 0;
  m_eSliceType          = I_SLICE;
  m_iSliceQp            = 0;
  m_iSymbolMode         = 1;
  m_aiNumRefIdx[0]      = 0;
  m_aiNumRefIdx[1]      = 0;
  m_bLoopFilterDisable  = false;
  
  m_iSliceQpDelta       = 0;
  
  m_iDepth              = 0;
  
  m_pcPic               = NULL;
  m_bRefenced           = false;
#ifdef ROUNDING_CONTROL_BIPRED
  m_bRounding           = false;
#endif
  m_uiColDir = 0;
  
  m_iViewIdx = 0 ;

#if SONY_COLPIC_AVAILABILITY
  m_iViewOrderIdx = 0;
#endif

  initEqualRef();
  m_bNoBackPredFlag = false;
#if MS_LCEC_LOOKUP_TABLE_EXCEPTION
  m_bRefIdxCombineCoding = false;
#endif
#if DCM_COMB_LIST 
  m_bRefPicListCombinationFlag = false;
  m_bRefPicListModificationFlagLC = false;
#endif
  m_uiSliceCurStartCUAddr        = 0;
  m_uiEntropySliceCurStartCUAddr = 0;

  ::memset( m_aaiCodedScale,  0x00, sizeof( m_aaiCodedScale  ) );
  ::memset( m_aaiCodedOffset, 0x00, sizeof( m_aaiCodedOffset ) );
  m_pcTexturePic = NULL;
#ifdef WEIGHT_PRED
  resetWpScaling(m_weightPredTable);
  initWpAcDcParam();
#endif
}

TComSlice::~TComSlice()
{
}


Void TComSlice::initSlice()
{
  m_aiNumRefIdx[0]      = 0;
  m_aiNumRefIdx[1]      = 0;
  
  m_uiColDir = 0;

  ::memset( m_apcRefPicList, 0, sizeof (m_apcRefPicList));
  ::memset( m_aiNumRefIdx,   0, sizeof ( m_aiNumRefIdx ));
  m_pcTexturePic = NULL;
  
  initEqualRef();
  m_bNoBackPredFlag = false;
#if MS_LCEC_LOOKUP_TABLE_EXCEPTION
  m_bRefIdxCombineCoding = false;
#endif
#if DCM_COMB_LIST 
  m_bRefPicListCombinationFlag = false;
  m_bRefPicListModificationFlagLC = false;
#endif
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

Void TComSlice::setRefPOCList       ()
{
  for (Int iDir = 0; iDir < 2; iDir++)
  {
    for (Int iNumRefIdx = 0; iNumRefIdx < m_aiNumRefIdx[iDir]; iNumRefIdx++)
    {
      m_aiRefPOCList[iDir][iNumRefIdx] = m_apcRefPicList[iDir][iNumRefIdx]->getPOC();
    }
  }

}

#if DCM_COMB_LIST 
Void TComSlice::generateCombinedList()
{
  if(m_aiNumRefIdx[REF_PIC_LIST_C] > 0)
  {
    m_aiNumRefIdx[REF_PIC_LIST_C]=0;
    for(Int iNumCount = 0; iNumCount < MAX_NUM_REF_LC; iNumCount++)
    {
      m_iRefIdxOfLC[REF_PIC_LIST_0][iNumCount]=-1;
      m_iRefIdxOfLC[REF_PIC_LIST_1][iNumCount]=-1;
      m_eListIdFromIdxOfLC[iNumCount]=0;
      m_iRefIdxFromIdxOfLC[iNumCount]=0;
      m_iRefIdxOfL0FromRefIdxOfL1[iNumCount] = -1;
      m_iRefIdxOfL1FromRefIdxOfL0[iNumCount] = -1;
    }

    for (Int iNumRefIdx = 0; iNumRefIdx < MAX_NUM_REF; iNumRefIdx++)
    {
      if(iNumRefIdx < m_aiNumRefIdx[REF_PIC_LIST_0]){
        Bool bTempRefIdxInL2 = true;
        for ( Int iRefIdxLC = 0; iRefIdxLC < m_aiNumRefIdx[REF_PIC_LIST_C]; iRefIdxLC++ )
        {
          if (( m_apcRefPicList[REF_PIC_LIST_0][iNumRefIdx]->getPOC() == m_apcRefPicList[m_eListIdFromIdxOfLC[iRefIdxLC]][m_iRefIdxFromIdxOfLC[iRefIdxLC]]->getPOC() ) &&
              ( m_apcRefPicList[REF_PIC_LIST_0][iNumRefIdx]->getViewIdx() == m_apcRefPicList[m_eListIdFromIdxOfLC[iRefIdxLC]][m_iRefIdxFromIdxOfLC[iRefIdxLC]]->getViewIdx() ))
          {
            m_iRefIdxOfL1FromRefIdxOfL0[iNumRefIdx] = m_iRefIdxFromIdxOfLC[iRefIdxLC];
            m_iRefIdxOfL0FromRefIdxOfL1[m_iRefIdxFromIdxOfLC[iRefIdxLC]] = iNumRefIdx;
            bTempRefIdxInL2 = false;
            assert(m_eListIdFromIdxOfLC[iRefIdxLC]==REF_PIC_LIST_1);
            break;
          }
        }

        if(bTempRefIdxInL2 == true)
        { 
          m_eListIdFromIdxOfLC[m_aiNumRefIdx[REF_PIC_LIST_C]] = REF_PIC_LIST_0;
          m_iRefIdxFromIdxOfLC[m_aiNumRefIdx[REF_PIC_LIST_C]] = iNumRefIdx;
          m_iRefIdxOfLC[REF_PIC_LIST_0][iNumRefIdx] = m_aiNumRefIdx[REF_PIC_LIST_C]++;
        }
      }

      if(iNumRefIdx < m_aiNumRefIdx[REF_PIC_LIST_1]){
        Bool bTempRefIdxInL2 = true;
        for ( Int iRefIdxLC = 0; iRefIdxLC < m_aiNumRefIdx[REF_PIC_LIST_C]; iRefIdxLC++ )
        {
          if (( m_apcRefPicList[REF_PIC_LIST_1][iNumRefIdx]->getPOC() == m_apcRefPicList[m_eListIdFromIdxOfLC[iRefIdxLC]][m_iRefIdxFromIdxOfLC[iRefIdxLC]]->getPOC() ) &&
              ( m_apcRefPicList[REF_PIC_LIST_1][iNumRefIdx]->getViewIdx() == m_apcRefPicList[m_eListIdFromIdxOfLC[iRefIdxLC]][m_iRefIdxFromIdxOfLC[iRefIdxLC]]->getViewIdx() ))
          {
            m_iRefIdxOfL0FromRefIdxOfL1[iNumRefIdx] = m_iRefIdxFromIdxOfLC[iRefIdxLC];
            m_iRefIdxOfL1FromRefIdxOfL0[m_iRefIdxFromIdxOfLC[iRefIdxLC]] = iNumRefIdx;
            bTempRefIdxInL2 = false;
            assert(m_eListIdFromIdxOfLC[iRefIdxLC]==REF_PIC_LIST_0);
            break;
          }
        }
        if(bTempRefIdxInL2 == true)
        {
          m_eListIdFromIdxOfLC[m_aiNumRefIdx[REF_PIC_LIST_C]] = REF_PIC_LIST_1;
          m_iRefIdxFromIdxOfLC[m_aiNumRefIdx[REF_PIC_LIST_C]] = iNumRefIdx;
          m_iRefIdxOfLC[REF_PIC_LIST_1][iNumRefIdx] = m_aiNumRefIdx[REF_PIC_LIST_C]++;
        }
      }
    }
  }
}
#endif

Void TComSlice::setRefPicListFromGOPSTring( TComList<TComPic*>& rcListPic, std::vector<TComPic*>& rapcSpatRefPics )
{

  if (m_eSliceType == I_SLICE)
  {
    ::memset( m_apcRefPicList, 0, sizeof (m_apcRefPicList));
    ::memset( m_aiNumRefIdx,   0, sizeof ( m_aiNumRefIdx ));

    return;
  }

  sortPicList(rcListPic);
  assert( m_eSliceType != P_SLICE || m_pcPic->getNumRefs( REF_PIC_LIST_1 ) == 0 );
  for(Int iRefList = 0; iRefList<2; iRefList++ )
  {
    RefPicList eRefList = (RefPicList) iRefList;
    m_aiNumRefIdx[eRefList] = m_pcPic->getNumRefs( eRefList );
    for( Int i =0; i<m_pcPic->getNumRefs(eRefList); i++ )
    {
      const int iRefPoc = m_pcPic->getRefPOC( eRefList, i );
      const int iRefViewIdx = m_pcPic->getRefViewIdx( eRefList, i );
      m_aiRefPOCList[eRefList][i] = iRefPoc;
      m_aiRefViewList[eRefList][i] = iRefViewIdx;

      TComPic* pcRefPic = NULL;
      if( iRefViewIdx == m_iViewIdx )
      {
        // temporal prediction from current view
        for( TComList<TComPic*>::iterator it = rcListPic.begin(); it!=rcListPic.end(); it++)
        {
          if((*it)->getPOC() == iRefPoc)
          {
            pcRefPic = *it;
            break;
          }
        }
        assert( pcRefPic );
        if( pcRefPic == NULL && iRefPoc > m_iPOC )
        {
          const int iAltRefPoc = m_iPOC-(iRefPoc-m_iPOC) ;
          m_aiRefPOCList[eRefList][i] = iAltRefPoc ;
          for( TComList<TComPic*>::iterator it = rcListPic.begin(); it!=rcListPic.end(); it++)
          {
            if((*it)->getPOC() == iAltRefPoc)
            {
              pcRefPic = *it ;
              break;
            }
          }
        }
      }
      else
      {
        // inter-view prediction
        assert( iRefPoc == m_iPOC );
        assert( 0 <= iRefViewIdx && iRefViewIdx < rapcSpatRefPics.size() );
        pcRefPic = rapcSpatRefPics[iRefViewIdx];
      }
      if( pcRefPic )
      {
        m_apcRefPicList[eRefList][i] = pcRefPic;
        pcRefPic->getPicYuvRec()->extendPicBorder();
      }
      else
      {
        printf("\n inconsistence in gop string!") ; // gop string inconsistent
        assert(0) ;
      }
    }
  }
}
Void TComSlice::setRefPicListExplicitlyDecoderSided( TComList<TComPic*>& rcListPic, std::vector<TComPic*>& rapcSpatRefPics )
{
  sortPicList(rcListPic);
  TComPic*  pcRefPic = NULL;

  for(Int iRefList = 0; iRefList<2; iRefList++)
  {
    RefPicList eRefList = (RefPicList) iRefList;
    for( Int i=0; i<m_aiNumRefIdx[eRefList]; i++ )
    {
      const int iRefPoc = m_aiRefPOCList[ eRefList][i ];
      const int iRefViewIdx = m_aiRefViewList[ eRefList][i ];
      if( iRefViewIdx == m_iViewIdx )
      {
        for( TComList<TComPic*>::iterator it = rcListPic.begin(); it!=rcListPic.end(); it++)
        {
          if((*it)->getPOC() == iRefPoc)
          {
            pcRefPic = *it;
            break;
          }
        }
      }
      else
      {
        // inter-view prediction
        assert( iRefPoc == m_iPOC );
        assert( 0 <= iRefViewIdx && iRefViewIdx < rapcSpatRefPics.size() );
        pcRefPic = rapcSpatRefPics[iRefViewIdx];
      }
      if( pcRefPic )
      {
        m_apcRefPicList[eRefList][i] = pcRefPic;
        pcRefPic->getPicYuvRec()->extendPicBorder();
      }
      else
      {
        printf("\n inconsistence in gop string!") ; // gop string inconsistent
        assert(0) ;
      }
    }
  }
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

Void TComSlice::initMultiviewSlice( Int** aaiScale, Int** aaiOffset )
{
  if( m_pcSPS->hasCamParInSliceHeader() )
  {
    UInt uiViewId = m_pcSPS->getViewId();
    for( UInt uiBaseViewId = 0; uiBaseViewId < uiViewId; uiBaseViewId++ )
    {
      m_aaiCodedScale [ 0 ][ uiBaseViewId ] = aaiScale [ uiBaseViewId ][     uiViewId ];
      m_aaiCodedScale [ 1 ][ uiBaseViewId ] = aaiScale [     uiViewId ][ uiBaseViewId ];
      m_aaiCodedOffset[ 0 ][ uiBaseViewId ] = aaiOffset[ uiBaseViewId ][     uiViewId ];
      m_aaiCodedOffset[ 1 ][ uiBaseViewId ] = aaiOffset[     uiViewId ][ uiBaseViewId ];
    }
  }
}


#if DCM_DECODING_REFRESH
/** Function for marking the reference pictures when an IDR and CDR is encountered.
 * \param uiPOCCDR POC of the CDR picture
 * \param bRefreshPending flag indicating if a deferred decoding refresh is pending
 * \param rcListPic reference to the reference picture list
 * This function marks the reference pictures as "unused for reference" in the following conditions.
 * If the nal_unit_type is IDR all pictures in the reference picture list  
 * is marked as "unused for reference" 
 * Otherwise do for the CDR case (non CDR case has no effect since both if conditions below will not be true)
 *    If the bRefreshPending flag is true (a deferred decoding refresh is pending) and the current 
 *    temporal reference is greater than the temporal reference of the latest CDR picture (uiPOCCDR), 
 *    mark all reference pictures except the latest CDR picture as "unused for reference" and set 
 *    the bRefreshPending flag to false.
 *    If the nal_unit_type is CDR, set the bRefreshPending flag to true and iPOCCDR to the temporal 
 *    reference of the current picture.
 * Note that the current picture is already placed in the reference list and its marking is not changed.
 * If the current picture has a nal_ref_idc that is not 0, it will remain marked as "used for reference".
 */
Void TComSlice::decodingRefreshMarking(UInt& uiPOCCDR, Bool& bRefreshPending, TComList<TComPic*>& rcListPic)
{
  TComPic*                 rpcPic;
  UInt uiPOCCurr = getPOC(); 

  if (getNalUnitType() == NAL_UNIT_CODED_SLICE_IDR)  // IDR
  {
    // mark all pictures as not used for reference
    TComList<TComPic*>::iterator        iterPic       = rcListPic.begin();
    while (iterPic != rcListPic.end())
    {
      rpcPic = *(iterPic);
      rpcPic->setCurrSliceIdx(0);
      if (rpcPic->getPOC() != uiPOCCurr) rpcPic->getSlice(0)->setReferenced(false);
      iterPic++;
    }
  }
  else // CDR or No DR
  {
    if (bRefreshPending==true && uiPOCCurr > uiPOCCDR) // CDR reference marking pending 
    {
      TComList<TComPic*>::iterator        iterPic       = rcListPic.begin();
      while (iterPic != rcListPic.end())
      {
        rpcPic = *(iterPic);
        if (rpcPic->getPOC() != uiPOCCurr && rpcPic->getPOC() != uiPOCCDR) rpcPic->getSlice(0)->setReferenced(false);
        iterPic++;
      }
      bRefreshPending = false; 
    }
    if (getNalUnitType() == NAL_UNIT_CODED_SLICE_CDR) // CDR picture found
    {
      bRefreshPending = true; 
      uiPOCCDR = uiPOCCurr;
    }
  }
}
#endif

Void TComSlice::copySliceInfo(TComSlice *pSrc)
{
  assert( pSrc != NULL );

  Int i, j, k;

  m_iPOC                 = pSrc->m_iPOC;
  m_iViewIdx             = pSrc->m_iViewIdx;
#if SONY_COLPIC_AVAILABILITY
  m_iViewOrderIdx        = pSrc->m_iViewOrderIdx;
#endif
#if DCM_DECODING_REFRESH
  m_eNalUnitType         = pSrc->m_eNalUnitType;
#endif  
  m_eSliceType           = pSrc->m_eSliceType;
  m_iSliceQp             = pSrc->m_iSliceQp;
  m_iSymbolMode          = pSrc->m_iSymbolMode;
  m_bLoopFilterDisable   = pSrc->m_bLoopFilterDisable;
  
#if DCM_COMB_LIST  
  for (i = 0; i < 3; i++)
  {
    m_aiNumRefIdx[i]     = pSrc->m_aiNumRefIdx[i];
  }

  for (i = 0; i < 2; i++)
  {
    for (j = 0; j < MAX_NUM_REF_LC; j++)
    {
       m_iRefIdxOfLC[i][j]  = pSrc->m_iRefIdxOfLC[i][j];
    }
  }
  for (i = 0; i < MAX_NUM_REF_LC; i++)
  {
    m_eListIdFromIdxOfLC[i] = pSrc->m_eListIdFromIdxOfLC[i];
    m_iRefIdxFromIdxOfLC[i] = pSrc->m_iRefIdxFromIdxOfLC[i];
    m_iRefIdxOfL1FromRefIdxOfL0[i] = pSrc->m_iRefIdxOfL1FromRefIdxOfL0[i];
    m_iRefIdxOfL0FromRefIdxOfL1[i] = pSrc->m_iRefIdxOfL0FromRefIdxOfL1[i];
  }
  m_bRefPicListModificationFlagLC = pSrc->m_bRefPicListModificationFlagLC;
  m_bRefPicListCombinationFlag    = pSrc->m_bRefPicListCombinationFlag;
#else
  for (i = 0; i < 2; i++)
  {
    m_aiNumRefIdx[i]     = pSrc->m_aiNumRefIdx[i];
  }
#endif  

  m_iSliceQpDelta        = pSrc->m_iSliceQpDelta;
  for (i = 0; i < 2; i++)
  {
    for (j = 0; j < MAX_NUM_REF; j++)
    {
      m_apcRefPicList[i][j]  = pSrc->m_apcRefPicList[i][j];
      m_aiRefPOCList[i][j]   = pSrc->m_aiRefPOCList[i][j];
      m_aiRefViewList[i][j]  = pSrc->m_aiRefViewList[i][j];
    }
  }  
  m_iDepth               = pSrc->m_iDepth;

  // referenced slice
  m_bRefenced            = pSrc->m_bRefenced;
#ifdef ROUNDING_CONTROL_BIPRED
  m_bRounding            = pSrc->m_bRounding;
#endif

  // access channel
  m_pcSPS                = pSrc->m_pcSPS;
  m_pcPPS                = pSrc->m_pcPPS;
  m_uiPPSId              = pSrc->m_uiPPSId;
  m_pcPic                = pSrc->m_pcPic;

  m_uiColDir             = pSrc->m_uiColDir;
  m_dLambda              = pSrc->m_dLambda;
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

  m_bNoBackPredFlag      = pSrc->m_bNoBackPredFlag;
#if MS_LCEC_LOOKUP_TABLE_EXCEPTION
  m_bRefIdxCombineCoding = pSrc->m_bRefIdxCombineCoding;
#endif
  m_uiSliceMode                   = pSrc->m_uiSliceMode;
  m_uiSliceArgument               = pSrc->m_uiSliceArgument;
  m_uiSliceCurStartCUAddr         = pSrc->m_uiSliceCurStartCUAddr;
  m_uiSliceCurEndCUAddr           = pSrc->m_uiSliceCurEndCUAddr;
  m_uiSliceIdx                    = pSrc->m_uiSliceIdx;
  m_uiEntropySliceMode            = pSrc->m_uiEntropySliceMode;
  m_uiEntropySliceArgument        = pSrc->m_uiEntropySliceArgument; 
  m_uiEntropySliceCurStartCUAddr  = pSrc->m_uiEntropySliceCurStartCUAddr;
  m_uiEntropySliceCurEndCUAddr    = pSrc->m_uiEntropySliceCurEndCUAddr;
  m_bNextSlice                    = pSrc->m_bNextSlice;
  m_bNextEntropySlice             = pSrc->m_bNextEntropySlice;
#ifdef WEIGHT_PRED
  for ( int e=0 ; e<2 ; e++ )
  for ( int n=0 ; n<MAX_NUM_REF ; n++ )
    memcpy(m_weightPredTable[e][n], pSrc->m_weightPredTable[e][n], sizeof(wpScalingParam)*3 );
#endif
}

#ifdef WEIGHT_PRED
Void  TComSlice::getWpScaling( RefPicList e, Int iRefIdx, wpScalingParam *&wp )
{
  wp = m_weightPredTable[e][iRefIdx];
}

Void  TComSlice::displayWpScaling()
{
  Bool  bFound = false;
  for ( int e=0 ; e<2 ; e++ ) {
    for ( int i=0 ; i<MAX_NUM_REF ; i++ )
    for ( int yuv=0 ; yuv<3 ; yuv++ )
      if ( m_weightPredTable[e][i][yuv].bPresentFlag ) {
        if ( !bFound ) {
          printf("\tluma_log2_weight_denom = %d\n",   m_weightPredTable[0][0][0].uiLog2WeightDenom);
          printf("\tchroma_log2_weight_denom = %d\n", m_weightPredTable[0][0][1].uiLog2WeightDenom);
          bFound = true;
        }
        Double  weight = (Double)m_weightPredTable[e][i][yuv].iWeight / (Double)(1<<m_weightPredTable[0][0][0].uiLog2WeightDenom);
        if ( yuv == 0 ) {
          printf("\tluma_weight_l%d_flag = 1\n", e);
          printf("\t luma_weight_l%d[%d] = %d => w = %g\n", e, i, m_weightPredTable[e][i][yuv].iWeight, weight);
          printf("\t luma_offset_l%d[%d] = %d\n", e, i, m_weightPredTable[e][i][yuv].iOffset);
        }
        else {
          if ( yuv == 1 ) printf("\tchroma_weight_l%d_flag = 1\n", e);
          printf("\t chroma_weight_l%d[%d][%d] = %d => w = %g\n", e, i, yuv-1, m_weightPredTable[e][i][yuv].iWeight, weight);
          printf("\t chroma_offset_l%d[%d][%d] = %d\n", e, i, yuv-1, m_weightPredTable[e][i][yuv].iOffset);
        }
      }
  }
}

// Default WP values settings : no weight. 
Void  TComSlice::resetWpScaling(wpScalingParam  wp[2][MAX_NUM_REF][3])
{
  for ( int e=0 ; e<2 ; e++ ) {
    for ( int i=0 ; i<MAX_NUM_REF ; i++ )
      for ( int yuv=0 ; yuv<3 ; yuv++ ) {
        wpScalingParam  *pwp = &(wp[e][i][yuv]);
        pwp->bPresentFlag      = false;
        pwp->uiLog2WeightDenom = 0;
        pwp->uiLog2WeightDenom = 0;
        pwp->iWeight           = 1;
        pwp->iOffset           = 0;
      }
  }
}

Void  TComSlice::initWpScaling()
{
  initWpScaling(m_weightPredTable);
}

Void  TComSlice::initWpScaling(wpScalingParam  wp[2][MAX_NUM_REF][3])
{
  for ( int e=0 ; e<2 ; e++ ) {
    for ( int i=0 ; i<MAX_NUM_REF ; i++ )
      for ( int yuv=0 ; yuv<3 ; yuv++ ) {
        wpScalingParam  *pwp = &(wp[e][i][yuv]);
        if ( !pwp->bPresentFlag ) {
          // Inferring values not present :
          pwp->iWeight = (1 << pwp->uiLog2WeightDenom);
          pwp->iOffset = 0;
        }

        pwp->w      = pwp->iWeight;
        pwp->o      = pwp->iOffset * (1 << (g_uiBitDepth-8));
        pwp->shift  = pwp->uiLog2WeightDenom;
        pwp->round  = (pwp->uiLog2WeightDenom>=1) ? (1 << (pwp->uiLog2WeightDenom-1)) : (0);
      }
  }
}
#endif

#ifdef WEIGHT_PRED
Void  TComSlice::getWpAcDcParam(wpACDCParam *&wp)
{
  wp = m_weightACDCParam;
}

Void  TComSlice::initWpAcDcParam()
{
  for(Int iComp = 0; iComp < 3; iComp++ )
  {
    m_weightACDCParam[iComp].iAC = 0;
    m_weightACDCParam[iComp].iDC = 0;
  }
}
#endif

// ------------------------------------------------------------------------------------------------
// Sequence parameter set (SPS)
// ------------------------------------------------------------------------------------------------

TComSPS::TComSPS()
{
  // Structure
  m_uiSPSId       = 0;
  m_uiWidth       = 352;
  m_uiHeight      = 288;
  m_uiMaxCUWidth  = 32;
  m_uiMaxCUHeight = 32;
  m_uiMaxCUDepth  = 3;
  m_uiMinTrDepth  = 0;
  m_uiMaxTrDepth  = 1;
  m_uiMaxTrSize   = 32;
  
  // Tool list
  m_bUseALF       = false;
  m_bUseDQP       = false;
  
  m_bUseMRG      = false; // SOPH:
#if HHI_MPI
  m_bUseMVI = false;
#endif
  
  m_uiViewId              = 0;
  m_iViewOrderIdx         = 0;
  m_bDepth                = false;
  m_uiCamParPrecision     = 0;
  m_bCamParInSliceHeader  = false;
  ::memset( m_aaiCodedScale,  0x00, sizeof( m_aaiCodedScale  ) );
  ::memset( m_aaiCodedOffset, 0x00, sizeof( m_aaiCodedOffset ) );

#if DEPTH_MAP_GENERATION
  m_uiPredDepthMapGeneration = 0;
  m_uiPdmPrecision           = 0;
  ::memset( m_aiPdmScaleNomDelta, 0x00, sizeof( m_aiPdmScaleNomDelta  ) );
  ::memset( m_aiPdmOffset,        0x00, sizeof( m_aiPdmOffset         ) );
#endif
#if HHI_INTER_VIEW_MOTION_PRED
  m_uiMultiviewMvPredMode    = 0;
#endif
#if HHI_INTER_VIEW_RESIDUAL_PRED
  m_uiMultiviewResPredMode   = 0;
#endif

  // AMVP parameter
  ::memset( m_aeAMVPMode, 0, sizeof( m_aeAMVPMode ) );

#if ( HHI_DMM_WEDGE_INTRA || HHI_DMM_PRED_TEX )
  m_bUseDMM = false;
#endif
#if HHI_DMM_PRED_TEX && FLEX_CODING_ORDER
   m_bUseDMM34 = false;
#endif
}

TComSPS::~TComSPS()
{
}

TComPPS::TComPPS()
{
#if CONSTRAINED_INTRA_PRED
  m_bConstrainedIntraPred = false;
#endif
  m_uiSPSId = 0;
  m_uiPPSId = 0;
}

TComPPS::~TComPPS()
{
}

Void
TComSPS::initMultiviewSPS( UInt uiViewId, Int iViewOrderIdx, UInt uiCamParPrecision, Bool bCamParSlice, Int** aaiScale, Int** aaiOffset )
{
  AOT( uiViewId == 0 && iViewOrderIdx != 0 );
  AOT( uiViewId != 0 && iViewOrderIdx == 0 );
  AOT( uiViewId != 0 && !bCamParSlice && ( aaiScale == 0 || aaiOffset == 0 ) );

  m_uiViewId              = uiViewId;
  m_iViewOrderIdx         = iViewOrderIdx;
  m_bDepth                = false;
  m_uiCamParPrecision     = ( m_uiViewId ? uiCamParPrecision : 0 );
  m_bCamParInSliceHeader  = ( m_uiViewId ? bCamParSlice  : false );
  ::memset( m_aaiCodedScale,  0x00, sizeof( m_aaiCodedScale  ) );
  ::memset( m_aaiCodedOffset, 0x00, sizeof( m_aaiCodedOffset ) );
  if( !m_bCamParInSliceHeader )
  {
    for( UInt uiBaseViewId = 0; uiBaseViewId < m_uiViewId; uiBaseViewId++ )
    {
      m_aaiCodedScale [ 0 ][ uiBaseViewId ] = aaiScale [ uiBaseViewId ][   m_uiViewId ];
      m_aaiCodedScale [ 1 ][ uiBaseViewId ] = aaiScale [   m_uiViewId ][ uiBaseViewId ];
      m_aaiCodedOffset[ 0 ][ uiBaseViewId ] = aaiOffset[ uiBaseViewId ][   m_uiViewId ];
      m_aaiCodedOffset[ 1 ][ uiBaseViewId ] = aaiOffset[   m_uiViewId ][ uiBaseViewId ];
    }
  }
}

Void
TComSPS::initMultiviewSPSDepth( UInt uiViewId, Int iViewOrderIdx )
{
  AOT( uiViewId == 0 && iViewOrderIdx != 0 );
  AOT( uiViewId != 0 && iViewOrderIdx == 0 );

  m_uiViewId              = uiViewId;
  m_iViewOrderIdx         = iViewOrderIdx;
  m_bDepth                = true;
  m_uiCamParPrecision     = 0;
  m_bCamParInSliceHeader  = false;
  ::memset( m_aaiCodedScale,  0x00, sizeof( m_aaiCodedScale  ) );
  ::memset( m_aaiCodedOffset, 0x00, sizeof( m_aaiCodedOffset ) );
}


#if DEPTH_MAP_GENERATION
Void
TComSPS::setPredDepthMapGeneration( UInt uiViewId, Bool bIsDepth, UInt uiPdmGenMode, UInt uiPdmMvPredMode, UInt uiPdmPrec, Int** aaiPdmScaleNomDelta, Int** aaiPdmOffset )
{ 
  AOF( m_uiViewId == uiViewId );
  AOF( m_bDepth   == bIsDepth );
  AOT( ( uiViewId == 0 || bIsDepth ) && uiPdmGenMode );
  AOT( uiPdmGenMode && ( aaiPdmScaleNomDelta == 0 || aaiPdmOffset == 0 ) );
  AOT( uiPdmMvPredMode && uiPdmGenMode == 0 );
  
  m_uiPredDepthMapGeneration = uiPdmGenMode;
#if HHI_INTER_VIEW_MOTION_PRED
  m_uiMultiviewMvPredMode    = uiPdmMvPredMode;
#endif
  m_uiPdmPrecision           = ( m_uiPredDepthMapGeneration ? uiPdmPrec : 0 );
  ::memset( m_aiPdmScaleNomDelta, 0x00, sizeof( m_aiPdmScaleNomDelta  ) );
  ::memset( m_aiPdmOffset,        0x00, sizeof( m_aiPdmOffset         ) );
  if( m_uiPredDepthMapGeneration )
  {
    for( UInt uiBaseId = 0; uiBaseId < m_uiViewId; uiBaseId++ )
    {
      m_aiPdmScaleNomDelta[ uiBaseId ]  = aaiPdmScaleNomDelta[ m_uiViewId ][ uiBaseId ];
      m_aiPdmOffset       [ uiBaseId ]  = aaiPdmOffset       [ m_uiViewId ][ uiBaseId ];
    }
  }
}
#endif
