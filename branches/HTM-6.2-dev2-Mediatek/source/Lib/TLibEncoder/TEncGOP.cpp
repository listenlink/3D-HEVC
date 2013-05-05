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

/** \file     TEncGOP.cpp
    \brief    GOP encoder class
*/

#include <list>
#include <algorithm>

#include "TEncTop.h"
#include "TEncGOP.h"
#include "TEncAnalyze.h"
#include "libmd5/MD5.h"
#include "TLibCommon/SEI.h"
#include "TLibCommon/NAL.h"
#include "NALwrite.h"
#include "../../App/TAppEncoder/TAppEncTop.h"

#include <time.h>
#include <math.h>

using namespace std;

//! \ingroup TLibEncoder
//! \{

// ====================================================================================================================
// Constructor / destructor / initialization / destroy
// ====================================================================================================================

TEncGOP::TEncGOP()
{
  m_iLastIDR            = 0;
  m_iGopSize            = 0;
  m_iNumPicCoded        = 0; //Niko
  m_bFirst              = true;
  
  m_pcCfg               = NULL;
  m_pcSliceEncoder      = NULL;
  m_pcListPic           = NULL;
  
  m_pcEntropyCoder      = NULL;
  m_pcCavlcCoder        = NULL;
  m_pcSbacCoder         = NULL;
  m_pcBinCABAC          = NULL;
#if DEPTH_MAP_GENERATION
  m_pcDepthMapGenerator = NULL;
#endif
#if H3D_IVRP & !QC_ARP_D0177
  m_pcResidualGenerator = NULL;
#endif
  
  m_bSeqFirst           = true;
  
  m_bRefreshPending     = 0;
  m_pocCRA              = 0;

  return;
}

TEncGOP::~TEncGOP()
{
}

/** Create list to contain pointers to LCU start addresses of slice.
 * \param iWidth, iHeight are picture width, height. iMaxCUWidth, iMaxCUHeight are LCU width, height.
 */
Void  TEncGOP::create( Int iWidth, Int iHeight, UInt iMaxCUWidth, UInt iMaxCUHeight )
{
  UInt uiWidthInCU       = ( iWidth %iMaxCUWidth  ) ? iWidth /iMaxCUWidth  + 1 : iWidth /iMaxCUWidth;
  UInt uiHeightInCU      = ( iHeight%iMaxCUHeight ) ? iHeight/iMaxCUHeight + 1 : iHeight/iMaxCUHeight;
  UInt uiNumCUsInFrame   = uiWidthInCU * uiHeightInCU;
  m_uiStoredStartCUAddrForEncodingSlice = new UInt [uiNumCUsInFrame*(1<<(g_uiMaxCUDepth<<1))+1];
  m_uiStoredStartCUAddrForEncodingEntropySlice = new UInt [uiNumCUsInFrame*(1<<(g_uiMaxCUDepth<<1))+1];
  m_bLongtermTestPictureHasBeenCoded = 0;
  m_bLongtermTestPictureHasBeenCoded2 = 0;
}

Void  TEncGOP::destroy()
{
  delete [] m_uiStoredStartCUAddrForEncodingSlice; m_uiStoredStartCUAddrForEncodingSlice = NULL;
  delete [] m_uiStoredStartCUAddrForEncodingEntropySlice; m_uiStoredStartCUAddrForEncodingEntropySlice = NULL;
}

Void TEncGOP::init ( TEncTop* pcTEncTop )
{
  m_pcEncTop     = pcTEncTop;
  m_pcCfg                = pcTEncTop;
  m_pcSliceEncoder       = pcTEncTop->getSliceEncoder();
  m_pcListPic            = pcTEncTop->getListPic();
  
  m_pcEntropyCoder       = pcTEncTop->getEntropyCoder();
  m_pcCavlcCoder         = pcTEncTop->getCavlcCoder();
  m_pcSbacCoder          = pcTEncTop->getSbacCoder();
  m_pcBinCABAC           = pcTEncTop->getBinCABAC();
  m_pcLoopFilter         = pcTEncTop->getLoopFilter();
  m_pcBitCounter         = pcTEncTop->getBitCounter();
  
#if DEPTH_MAP_GENERATION
  m_pcDepthMapGenerator  = pcTEncTop->getDepthMapGenerator();
#endif
#if H3D_IVRP & !QC_ARP_D0177
  m_pcResidualGenerator  = pcTEncTop->getResidualGenerator();
#endif
  
  // Adaptive Loop filter
  m_pcAdaptiveLoopFilter = pcTEncTop->getAdaptiveLoopFilter();
  //--Adaptive Loop filter
  m_pcSAO                = pcTEncTop->getSAO();
  m_pcRdCost             = pcTEncTop->getRdCost();
}

// ====================================================================================================================
// Public member functions
// ====================================================================================================================

Void TEncGOP::initGOP( Int iPOCLast, Int iNumPicRcvd, TComList<TComPic*>& rcListPic, TComList<TComPicYuv*>& rcListPicYuvRecOut, std::list<AccessUnit>& accessUnitsInGOP)
{
  xInitGOP( iPOCLast, iNumPicRcvd, rcListPic, rcListPicYuvRecOut );
  m_iNumPicCoded = 0;
}

Void TEncGOP::compressPicInGOP( Int iPOCLast, Int iNumPicRcvd, TComList<TComPic*>& rcListPic, TComList<TComPicYuv*>& rcListPicYuvRecOut, std::list<AccessUnit>& accessUnitsInGOP, Int iGOPid)
{
  TComPic*        pcPic;
  TComPicYuv*     pcPicYuvRecOut;
  TComSlice*      pcSlice;
  TComOutputBitstream  *pcBitstreamRedirect;
  pcBitstreamRedirect = new TComOutputBitstream;
  AccessUnit::iterator  itLocationToPushSliceHeaderNALU; // used to store location where NALU containing slice header is to be inserted
  UInt                  uiOneBitstreamPerSliceLength = 0;
  TEncSbac* pcSbacCoders = NULL;
  TComOutputBitstream* pcSubstreamsOut = NULL;

  {
      UInt uiColDir = 1;
      //-- For time output for each slice
      long iBeforeTime = clock();
      
      //select uiColDir
      Int iCloseLeft=1, iCloseRight=-1;
      for(Int i = 0; i<m_pcCfg->getGOPEntry(iGOPid).m_numRefPics; i++) 
      {
        Int iRef = m_pcCfg->getGOPEntry(iGOPid).m_referencePics[i];
        if(iRef>0&&(iRef<iCloseRight||iCloseRight==-1))
        {
          iCloseRight=iRef;
        }
        else if(iRef<0&&(iRef>iCloseLeft||iCloseLeft==1))
        {
          iCloseLeft=iRef;
        }
      }
      if(iCloseRight>-1)
      {
        iCloseRight=iCloseRight+m_pcCfg->getGOPEntry(iGOPid).m_POC-1;
      }
      if(iCloseLeft<1) 
      {
        iCloseLeft=iCloseLeft+m_pcCfg->getGOPEntry(iGOPid).m_POC-1;
        while(iCloseLeft<0)
        {
          iCloseLeft+=m_iGopSize;
        }
      }
      Int iLeftQP=0, iRightQP=0;
      for(Int i=0; i<m_iGopSize; i++)
      {
        if(m_pcCfg->getGOPEntry(i).m_POC==(iCloseLeft%m_iGopSize)+1)
        {
          iLeftQP= m_pcCfg->getGOPEntry(i).m_QPOffset;
        }
        if (m_pcCfg->getGOPEntry(i).m_POC==(iCloseRight%m_iGopSize)+1)
        {
          iRightQP=m_pcCfg->getGOPEntry(i).m_QPOffset;
        }
      }
      if(iCloseRight>-1&&iRightQP<iLeftQP)
      {
        uiColDir=0;
      }

      /////////////////////////////////////////////////////////////////////////////////////////////////// Initial to start encoding
      UInt uiPOCCurr = iPOCLast -iNumPicRcvd+ m_pcCfg->getGOPEntry(iGOPid).m_POC;
      Int iTimeOffset = m_pcCfg->getGOPEntry(iGOPid).m_POC;
      if(iPOCLast == 0)
      {
        uiPOCCurr=0;
        iTimeOffset = 1;
      }
      if(uiPOCCurr>=m_pcCfg->getFrameToBeEncoded())
      {
        return;
      }        
      if( getNalUnitTypeBaseViewMvc( uiPOCCurr ) == NAL_UNIT_CODED_SLICE_IDR )
      {
        m_iLastIDR = uiPOCCurr;
      }        

      /* start a new access unit: create an entry in the list of output
       * access units */
      accessUnitsInGOP.push_back(AccessUnit());
      AccessUnit& accessUnit = accessUnitsInGOP.back();
      xGetBuffer( rcListPic, rcListPicYuvRecOut, iNumPicRcvd, iTimeOffset, pcPic, pcPicYuvRecOut, uiPOCCurr );
      
      //  Slice data initialization
      pcPic->clearSliceBuffer();
      assert(pcPic->getNumAllocatedSlice() == 1);
      m_pcSliceEncoder->setSliceIdx(0);
      pcPic->setCurrSliceIdx(0);

      std::vector<TComAPS>& vAPS = m_pcEncTop->getAPS();
#if VIDYO_VPS_INTEGRATION|QC_MVHEVC_B0046
#if MTK_DEPTH_MERGE_TEXTURE_CANDIDATE_C0137
    m_pcSliceEncoder->initEncSlice ( pcPic, iPOCLast, uiPOCCurr, iNumPicRcvd, iGOPid, pcSlice, m_pcEncTop->getEncTop()->getVPS(), m_pcEncTop->getSPS(), m_pcEncTop->getPPS(), m_pcEncTop->getIsDepth() );
#else
    m_pcSliceEncoder->initEncSlice ( pcPic, iPOCLast, uiPOCCurr, iNumPicRcvd, iGOPid, pcSlice, m_pcEncTop->getEncTop()->getVPS(), m_pcEncTop->getSPS(), m_pcEncTop->getPPS() );
#endif
#else
      m_pcSliceEncoder->initEncSlice ( pcPic, iPOCLast, uiPOCCurr, iNumPicRcvd, iGOPid, pcSlice, m_pcEncTop->getSPS(), m_pcEncTop->getPPS() );
#endif
      pcSlice->setLastIDR(m_iLastIDR);
      pcSlice->setSliceIdx(0);
      pcSlice->setViewId( m_pcEncTop->getViewId() );
      pcSlice->setIsDepth( m_pcEncTop->getIsDepth() );
#if INTER_VIEW_VECTOR_SCALING_C0115
      pcSlice->setIVScalingFlag( m_pcEncTop->getUseIVS() );
#endif

      m_pcEncTop->getSPS()->setDisInter4x4(m_pcEncTop->getDisInter4x4());
      pcSlice->setScalingList ( m_pcEncTop->getScalingList()  );
      if(m_pcEncTop->getUseScalingListId() == SCALING_LIST_OFF)
      {
        m_pcEncTop->getTrQuant()->setFlatScalingList();
        m_pcEncTop->getTrQuant()->setUseScalingList(false);
      }
      else if(m_pcEncTop->getUseScalingListId() == SCALING_LIST_DEFAULT)
      {
        pcSlice->setDefaultScalingList ();
        pcSlice->getScalingList()->setScalingListPresentFlag(true);
        m_pcEncTop->getTrQuant()->setScalingList(pcSlice->getScalingList());
        m_pcEncTop->getTrQuant()->setUseScalingList(true);
      }
      else if(m_pcEncTop->getUseScalingListId() == SCALING_LIST_FILE_READ)
      {
        if(pcSlice->getScalingList()->xParseScalingList(m_pcCfg->getScalingListFile()))
        {
          pcSlice->setDefaultScalingList ();
        }
        pcSlice->getScalingList()->checkDcOfMatrix();
        pcSlice->getScalingList()->setScalingListPresentFlag(pcSlice->checkDefaultScalingList());
        m_pcEncTop->getTrQuant()->setScalingList(pcSlice->getScalingList());
        m_pcEncTop->getTrQuant()->setUseScalingList(true);
      }
      else
      {
        printf("error : ScalingList == %d no support\n",m_pcEncTop->getUseScalingListId());
        assert(0);
      }

#if HHI_INTERVIEW_SKIP
      if ( m_pcEncTop->getInterViewSkip() )
      {
        m_pcEncTop->getEncTop()->getUsedPelsMap( pcPic->getViewId(), pcPic->getPOC(), pcPic->getUsedPelsMap() );
      }
#endif
      //  Slice info. refinement
      if( pcSlice->getSliceType() == B_SLICE )
      {
#if QC_REM_IDV_B0046
      if( m_pcCfg->getGOPEntry(pcSlice->getSPS()->getViewId() && ((getNalUnitType(uiPOCCurr) == NAL_UNIT_CODED_SLICE_IDR) || (getNalUnitType(uiPOCCurr) == NAL_UNIT_CODED_SLICE_CRA))? MAX_GOP : iGOPid ).m_sliceType == 'P' ) { pcSlice->setSliceType( P_SLICE ); }
#else
      if( m_pcCfg->getGOPEntry( (getNalUnitType(uiPOCCurr) == NAL_UNIT_CODED_SLICE_IDV) ? MAX_GOP : iGOPid ).m_sliceType == 'P' ) { pcSlice->setSliceType( P_SLICE ); }
#endif
    }

      // Set the nal unit type
      pcSlice->setNalUnitType( getNalUnitType(uiPOCCurr) );
      pcSlice->setNalUnitTypeBaseViewMvc( getNalUnitTypeBaseViewMvc(uiPOCCurr) );

      // Do decoding refresh marking if any 
      pcSlice->decodingRefreshMarking(m_pocCRA, m_bRefreshPending, rcListPic);

      if ( !pcSlice->getPPS()->getEnableTMVPFlag() && pcPic->getTLayer() == 0 )
      {
        pcSlice->decodingMarkingForNoTMVP( rcListPic, pcSlice->getPOC() );
      }

      m_pcEncTop->selectReferencePictureSet(pcSlice, uiPOCCurr, iGOPid,rcListPic);
      pcSlice->getRPS()->setNumberOfLongtermPictures(0);

      if(pcSlice->checkThatAllRefPicsAreAvailable(rcListPic, pcSlice->getRPS(), false) != 0)
      {
         pcSlice->createExplicitReferencePictureSetFromReference(rcListPic, pcSlice->getRPS());
      }
      pcSlice->applyReferencePictureSet(rcListPic, pcSlice->getRPS());

#if H0566_TLA_SET_FOR_SWITCHING_POINTS
      if(pcSlice->getTLayer() > 0)
      {
        if(pcSlice->isTemporalLayerSwitchingPoint(rcListPic, pcSlice->getRPS()))
        {
          pcSlice->setNalUnitType(NAL_UNIT_CODED_SLICE_TLA);
        }
      }
#endif

#if !QC_REM_IDV_B0046
      pcSlice->setNumRefIdx( REF_PIC_LIST_0, min( m_pcCfg->getGOPEntry( (getNalUnitType(uiPOCCurr) == NAL_UNIT_CODED_SLICE_IDV) ? MAX_GOP : iGOPid ).m_numRefPicsActive, (pcSlice->getRPS()->getNumberOfPictures() + pcSlice->getSPS()->getNumberOfUsableInterViewRefs()) ) );
      pcSlice->setNumRefIdx( REF_PIC_LIST_1, min( m_pcCfg->getGOPEntry( (getNalUnitType(uiPOCCurr) == NAL_UNIT_CODED_SLICE_IDV) ? MAX_GOP : iGOPid ).m_numRefPicsActive, (pcSlice->getRPS()->getNumberOfPictures() + pcSlice->getSPS()->getNumberOfUsableInterViewRefs()) ) );
#else

      Bool bNalRAP = ((getNalUnitType(uiPOCCurr) == NAL_UNIT_CODED_SLICE_CRA) || (getNalUnitType(uiPOCCurr) == NAL_UNIT_CODED_SLICE_IDR)) && (pcSlice->getSPS()->getViewId())  ? 1: 0;
      pcSlice->setNumRefIdx( REF_PIC_LIST_0, min( m_pcCfg->getGOPEntry( bNalRAP ? MAX_GOP : iGOPid ).m_numRefPicsActive, (pcSlice->getRPS()->getNumberOfPictures() + pcSlice->getSPS()->getNumberOfUsableInterViewRefs()) ) );
      pcSlice->setNumRefIdx( REF_PIC_LIST_1, min( m_pcCfg->getGOPEntry( bNalRAP ? MAX_GOP : iGOPid ).m_numRefPicsActive, (pcSlice->getRPS()->getNumberOfPictures() + pcSlice->getSPS()->getNumberOfUsableInterViewRefs()) ) );
#endif
      TComRefPicListModification* refPicListModification = pcSlice->getRefPicListModification();
      refPicListModification->setRefPicListModificationFlagL0( false );
      refPicListModification->setRefPicListModificationFlagL1( false );
      xSetRefPicListModificationsMvc( pcSlice, uiPOCCurr, iGOPid );

#if ADAPTIVE_QP_SELECTION
      pcSlice->setTrQuant( m_pcEncTop->getTrQuant() );
#endif      
      //  Set reference list
      TAppEncTop* tAppEncTop = m_pcEncTop->getEncTop();
      assert( tAppEncTop != NULL );


#if FLEX_CODING_ORDER_M23723
      TComPic * pcTexturePic; 
      if(m_pcEncTop->getIsDepth() == 1)
      {
        TComPicYuv * recText;
        recText = tAppEncTop->getPicYuvFromView(m_pcEncTop->getViewId(), pcSlice->getPOC(), false ,true);
        if(recText == NULL)
        {
           pcSlice->setTexturePic(NULL);
        }
        else
        {
           pcTexturePic = m_pcEncTop->getIsDepth() ? tAppEncTop->getPicFromView( m_pcEncTop->getViewId(), pcSlice->getPOC(), false ) : NULL;
           pcSlice->setTexturePic( pcTexturePic );
        }
      }
      else
    {
        pcTexturePic = m_pcEncTop->getIsDepth() ? tAppEncTop->getPicFromView( m_pcEncTop->getViewId(), pcSlice->getPOC(), false ) : NULL;
        assert( !m_pcEncTop->getIsDepth() || pcTexturePic != NULL );
          pcSlice->setTexturePic( pcTexturePic );
      }

#else
      TComPic * const pcTexturePic = m_pcEncTop->getIsDepth() ? tAppEncTop->getPicFromView( m_pcEncTop->getViewId(), pcSlice->getPOC(), false ) : NULL;
      assert( !m_pcEncTop->getIsDepth() || pcTexturePic != NULL );
      pcSlice->setTexturePic( pcTexturePic );

#endif 
      std::vector<TComPic*> apcInterViewRefPics = tAppEncTop->getInterViewRefPics( m_pcEncTop->getViewId(), pcSlice->getPOC(), m_pcEncTop->getIsDepth(), pcSlice->getSPS() );
      pcSlice->setRefPicListMvc( rcListPic, apcInterViewRefPics );
#if QC_ARP_D0177
      pcSlice->setARPStepNum();
      if(pcSlice->getARPStepNum() > 1)
      {
        for(Int iViewIdx = 0; iViewIdx < pcSlice->getViewId(); iViewIdx ++ )
          pcSlice->setBaseViewRefPicList( tAppEncTop->getTEncTop( iViewIdx , false )->getListPic(), iViewIdx );
      }
#endif
      //  Slice info. refinement
      if( pcSlice->getSliceType() == B_SLICE )
      {
#if !QC_REM_IDV_B0046
        if( m_pcCfg->getGOPEntry( (getNalUnitType(uiPOCCurr) == NAL_UNIT_CODED_SLICE_IDV) ? MAX_GOP : iGOPid ).m_sliceType == 'P' ) { pcSlice->setSliceType( P_SLICE ); }
#else
      Bool bRAP = ((getNalUnitType(uiPOCCurr) == NAL_UNIT_CODED_SLICE_CRA) || (getNalUnitType(uiPOCCurr) == NAL_UNIT_CODED_SLICE_IDR)) && (pcSlice->getSPS()->getViewId())  ? 1: 0;
      if( m_pcCfg->getGOPEntry( bRAP ? MAX_GOP : iGOPid ).m_sliceType == 'P' ) { pcSlice->setSliceType( P_SLICE ); }
#endif
      }
      
      if (pcSlice->getSliceType() != B_SLICE || !pcSlice->getSPS()->getUseLComb())
      {
        pcSlice->setNumRefIdx(REF_PIC_LIST_C, 0);
        pcSlice->setRefPicListCombinationFlag(false);
        pcSlice->setRefPicListModificationFlagLC(false);
      }
      else
      {
        pcSlice->setRefPicListCombinationFlag(pcSlice->getSPS()->getUseLComb());
        pcSlice->setRefPicListModificationFlagLC(pcSlice->getSPS()->getLCMod());
        pcSlice->setNumRefIdx(REF_PIC_LIST_C, pcSlice->getNumRefIdx(REF_PIC_LIST_0));
      }
      
      if (pcSlice->getSliceType() == B_SLICE)
      {
        pcSlice->setColDir(uiColDir);
        Bool bLowDelay = true;
        Int  iCurrPOC  = pcSlice->getPOC();
        Int iRefIdx = 0;

        for (iRefIdx = 0; iRefIdx < pcSlice->getNumRefIdx(REF_PIC_LIST_0) && bLowDelay; iRefIdx++)
        {
          if ( pcSlice->getRefPic(REF_PIC_LIST_0, iRefIdx)->getPOC() > iCurrPOC )
          {
            bLowDelay = false;
          }
        }
        for (iRefIdx = 0; iRefIdx < pcSlice->getNumRefIdx(REF_PIC_LIST_1) && bLowDelay; iRefIdx++)
        {
          if ( pcSlice->getRefPic(REF_PIC_LIST_1, iRefIdx)->getPOC() > iCurrPOC )
          {
            bLowDelay = false;
          }
        }

        pcSlice->setCheckLDC(bLowDelay);  
      }
      
      uiColDir = 1-uiColDir;
      
      //-------------------------------------------------------------
      pcSlice->setRefPOCnViewListsMvc();
      
      pcSlice->setNoBackPredFlag( false );
      if ( pcSlice->getSliceType() == B_SLICE && !pcSlice->getRefPicListCombinationFlag())
      {
        if ( pcSlice->getNumRefIdx(RefPicList( 0 ) ) == pcSlice->getNumRefIdx(RefPicList( 1 ) ) )
        {
          pcSlice->setNoBackPredFlag( true );
          int i;
          for ( i=0; i < pcSlice->getNumRefIdx(RefPicList( 1 ) ); i++ )
          {
            if ( pcSlice->getRefPOC(RefPicList(1), i) != pcSlice->getRefPOC(RefPicList(0), i) ) 
            {
              pcSlice->setNoBackPredFlag( false );
              break;
            }
          }
        }
      }

      if(pcSlice->getNoBackPredFlag())
      {
        pcSlice->setNumRefIdx(REF_PIC_LIST_C, 0);
      }
      pcSlice->generateCombinedList();
      
#if HHI_VSO
  Bool bUseVSO = m_pcEncTop->getUseVSO();
  m_pcRdCost->setUseVSO( bUseVSO );
#if SAIT_VSO_EST_A0033
  m_pcRdCost->setUseEstimatedVSD( m_pcEncTop->getUseEstimatedVSD() );
#endif

  if ( bUseVSO )
  {
    Int iVSOMode = m_pcEncTop->getVSOMode();
    m_pcRdCost->setVSOMode( iVSOMode  );

#if HHI_VSO_DIST_INT
    m_pcRdCost->setAllowNegDist( m_pcEncTop->getAllowNegDist() );
#endif


#if SAIT_VSO_EST_A0033
#ifdef FLEX_CODING_ORDER_M23723    
{
  Bool flagRec;
  flagRec =  ((m_pcEncTop->getEncTop()->getPicYuvFromView( pcSlice->getViewId(), pcSlice->getPOC(), false, true) == NULL) ? false: true);
  m_pcRdCost->setVideoRecPicYuv( m_pcEncTop->getEncTop()->getPicYuvFromView( pcSlice->getViewId(), pcSlice->getPOC(), false, flagRec ) );
  m_pcRdCost->setDepthPicYuv   ( m_pcEncTop->getEncTop()->getPicYuvFromView( pcSlice->getViewId(), pcSlice->getPOC(), true, false ) );
}
#else
  m_pcRdCost->setVideoRecPicYuv( m_pcEncTop->getEncTop()->getPicYuvFromView( pcSlice->getViewId(), pcSlice->getPOC(), false, true ) );
  m_pcRdCost->setDepthPicYuv   ( m_pcEncTop->getEncTop()->getPicYuvFromView( pcSlice->getViewId(), pcSlice->getPOC(), true, false ) );
#endif
#endif
#if LGE_WVSO_A0119
    Bool bUseWVSO  = m_pcEncTop->getUseWVSO();
    m_pcRdCost->setUseWVSO( bUseWVSO );
#endif

  }
#endif
      /////////////////////////////////////////////////////////////////////////////////////////////////// Compress a slice
      //  Slice compression
      if (m_pcCfg->getUseASR())
      {
        m_pcSliceEncoder->setSearchRange(pcSlice);
      }

      Bool bGPBcheck=false;
      if ( pcSlice->getSliceType() == B_SLICE)
      {
        if ( pcSlice->getNumRefIdx(RefPicList( 0 ) ) == pcSlice->getNumRefIdx(RefPicList( 1 ) ) )
        {
          bGPBcheck=true;
          int i;
          for ( i=0; i < pcSlice->getNumRefIdx(RefPicList( 1 ) ); i++ )
          {
            if ( pcSlice->getRefPOC(RefPicList(1), i) != pcSlice->getRefPOC(RefPicList(0), i) ) 
            {
              bGPBcheck=false;
              break;
            }
          }
        }
      }
      if(bGPBcheck)
      {
        pcSlice->setMvdL1ZeroFlag(true);
      }
      else
      {
        pcSlice->setMvdL1ZeroFlag(false);
      }
      pcPic->getSlice(pcSlice->getSliceIdx())->setMvdL1ZeroFlag(pcSlice->getMvdL1ZeroFlag());

      UInt uiNumSlices = 1;

      UInt uiInternalAddress = pcPic->getNumPartInCU()-4;
      UInt uiExternalAddress = pcPic->getPicSym()->getNumberOfCUsInFrame()-1;
      UInt uiPosX = ( uiExternalAddress % pcPic->getFrameWidthInCU() ) * g_uiMaxCUWidth+ g_auiRasterToPelX[ g_auiZscanToRaster[uiInternalAddress] ];
      UInt uiPosY = ( uiExternalAddress / pcPic->getFrameWidthInCU() ) * g_uiMaxCUHeight+ g_auiRasterToPelY[ g_auiZscanToRaster[uiInternalAddress] ];
      UInt uiWidth = pcSlice->getSPS()->getPicWidthInLumaSamples();
      UInt uiHeight = pcSlice->getSPS()->getPicHeightInLumaSamples();
      while(uiPosX>=uiWidth||uiPosY>=uiHeight) 
      {
        uiInternalAddress--;
        uiPosX = ( uiExternalAddress % pcPic->getFrameWidthInCU() ) * g_uiMaxCUWidth+ g_auiRasterToPelX[ g_auiZscanToRaster[uiInternalAddress] ];
        uiPosY = ( uiExternalAddress / pcPic->getFrameWidthInCU() ) * g_uiMaxCUHeight+ g_auiRasterToPelY[ g_auiZscanToRaster[uiInternalAddress] ];
      }
      uiInternalAddress++;
      if(uiInternalAddress==pcPic->getNumPartInCU()) 
      {
        uiInternalAddress = 0;
        uiExternalAddress++;
      }
      UInt uiRealEndAddress = uiExternalAddress*pcPic->getNumPartInCU()+uiInternalAddress;

    UInt uiCummulativeTileWidth;
    UInt uiCummulativeTileHeight;
    Int  p, j;
    UInt uiEncCUAddr;
    

    if( pcSlice->getPPS()->getColumnRowInfoPresent() == 1 )    //derive the tile parameters from PPS
    {
      //set NumColumnsMinus1 and NumRowsMinus1
      pcPic->getPicSym()->setNumColumnsMinus1( pcSlice->getPPS()->getNumColumnsMinus1() );
      pcPic->getPicSym()->setNumRowsMinus1( pcSlice->getPPS()->getNumRowsMinus1() );

      //create the TComTileArray
      pcPic->getPicSym()->xCreateTComTileArray();

      if( pcSlice->getPPS()->getUniformSpacingIdr() == 1 )
      {
        //set the width for each tile
        for(j=0; j < pcPic->getPicSym()->getNumRowsMinus1()+1; j++)
        {
          for(p=0; p < pcPic->getPicSym()->getNumColumnsMinus1()+1; p++)
          {
            pcPic->getPicSym()->getTComTile( j * (pcPic->getPicSym()->getNumColumnsMinus1()+1) + p )->
              setTileWidth( (p+1)*pcPic->getPicSym()->getFrameWidthInCU()/(pcPic->getPicSym()->getNumColumnsMinus1()+1) 
              - (p*pcPic->getPicSym()->getFrameWidthInCU())/(pcPic->getPicSym()->getNumColumnsMinus1()+1) );
          }
        }

        //set the height for each tile
        for(j=0; j < pcPic->getPicSym()->getNumColumnsMinus1()+1; j++)
        {
          for(p=0; p < pcPic->getPicSym()->getNumRowsMinus1()+1; p++)
          {
            pcPic->getPicSym()->getTComTile( p * (pcPic->getPicSym()->getNumColumnsMinus1()+1) + j )->
              setTileHeight( (p+1)*pcPic->getPicSym()->getFrameHeightInCU()/(pcPic->getPicSym()->getNumRowsMinus1()+1) 
              - (p*pcPic->getPicSym()->getFrameHeightInCU())/(pcPic->getPicSym()->getNumRowsMinus1()+1) );   
          }
        }
      }
      else
      {
        //set the width for each tile
        for(j=0; j < pcPic->getPicSym()->getNumRowsMinus1()+1; j++)
        {
          uiCummulativeTileWidth = 0;
          for(p=0; p < pcPic->getPicSym()->getNumColumnsMinus1(); p++)
          {
            pcPic->getPicSym()->getTComTile( j * (pcPic->getPicSym()->getNumColumnsMinus1()+1) + p )->setTileWidth( pcSlice->getPPS()->getColumnWidth(p) );
            uiCummulativeTileWidth += pcSlice->getPPS()->getColumnWidth(p);
          }
          pcPic->getPicSym()->getTComTile(j * (pcPic->getPicSym()->getNumColumnsMinus1()+1) + p)->setTileWidth( pcPic->getPicSym()->getFrameWidthInCU()-uiCummulativeTileWidth );
        }

        //set the height for each tile
        for(j=0; j < pcPic->getPicSym()->getNumColumnsMinus1()+1; j++)
        {
          uiCummulativeTileHeight = 0;
          for(p=0; p < pcPic->getPicSym()->getNumRowsMinus1(); p++)
          {
            pcPic->getPicSym()->getTComTile( p * (pcPic->getPicSym()->getNumColumnsMinus1()+1) + j )->setTileHeight( pcSlice->getPPS()->getRowHeight(p) );
            uiCummulativeTileHeight += pcSlice->getPPS()->getRowHeight(p);
          }
          pcPic->getPicSym()->getTComTile(p * (pcPic->getPicSym()->getNumColumnsMinus1()+1) + j)->setTileHeight( pcPic->getPicSym()->getFrameHeightInCU()-uiCummulativeTileHeight );
        }
      }
    }
    else //derive the tile parameters from SPS
    {
      //set NumColumnsMins1 and NumRowsMinus1
      pcPic->getPicSym()->setNumColumnsMinus1( pcSlice->getSPS()->getNumColumnsMinus1() );
      pcPic->getPicSym()->setNumRowsMinus1( pcSlice->getSPS()->getNumRowsMinus1() );

      //create the TComTileArray
      pcPic->getPicSym()->xCreateTComTileArray();

      if( pcSlice->getSPS()->getUniformSpacingIdr() == 1 )
      {
        //set the width for each tile
        for(j=0; j < pcPic->getPicSym()->getNumRowsMinus1()+1; j++)
        {
          for(p=0; p < pcPic->getPicSym()->getNumColumnsMinus1()+1; p++)
          {
            pcPic->getPicSym()->getTComTile( j * (pcPic->getPicSym()->getNumColumnsMinus1()+1) + p )->
              setTileWidth( (p+1)*pcPic->getPicSym()->getFrameWidthInCU()/(pcPic->getPicSym()->getNumColumnsMinus1()+1) 
              - (p*pcPic->getPicSym()->getFrameWidthInCU())/(pcPic->getPicSym()->getNumColumnsMinus1()+1) );
          }
        }

        //set the height for each tile
        for(j=0; j < pcPic->getPicSym()->getNumColumnsMinus1()+1; j++)
        {
          for(p=0; p < pcPic->getPicSym()->getNumRowsMinus1()+1; p++)
          {
            pcPic->getPicSym()->getTComTile( p * (pcPic->getPicSym()->getNumColumnsMinus1()+1) + j )->
              setTileHeight( (p+1)*pcPic->getPicSym()->getFrameHeightInCU()/(pcPic->getPicSym()->getNumRowsMinus1()+1) 
              - (p*pcPic->getPicSym()->getFrameHeightInCU())/(pcPic->getPicSym()->getNumRowsMinus1()+1) );   
          }
        }
      }

      else
      {
        //set the width for each tile
        for(j=0; j < pcPic->getPicSym()->getNumRowsMinus1()+1; j++)
        {
          uiCummulativeTileWidth = 0;
          for(p=0; p < pcPic->getPicSym()->getNumColumnsMinus1(); p++)
          {
            pcPic->getPicSym()->getTComTile( j * (pcPic->getPicSym()->getNumColumnsMinus1()+1) + p )->setTileWidth( pcSlice->getSPS()->getColumnWidth(p) );
            uiCummulativeTileWidth += pcSlice->getSPS()->getColumnWidth(p);
          }
          pcPic->getPicSym()->getTComTile(j * (pcPic->getPicSym()->getNumColumnsMinus1()+1) + p)->setTileWidth( pcPic->getPicSym()->getFrameWidthInCU()-uiCummulativeTileWidth );
        }

        //set the height for each tile
        for(j=0; j < pcPic->getPicSym()->getNumColumnsMinus1()+1; j++)
        {
          uiCummulativeTileHeight = 0;
          for(p=0; p < pcPic->getPicSym()->getNumRowsMinus1(); p++)
          {
            pcPic->getPicSym()->getTComTile( p * (pcPic->getPicSym()->getNumColumnsMinus1()+1) + j )->setTileHeight( pcSlice->getSPS()->getRowHeight(p) );
            uiCummulativeTileHeight += pcSlice->getSPS()->getRowHeight(p);
          }
          pcPic->getPicSym()->getTComTile(p * (pcPic->getPicSym()->getNumColumnsMinus1()+1) + j)->setTileHeight( pcPic->getPicSym()->getFrameHeightInCU()-uiCummulativeTileHeight );
        }
      }
    }

    //initialize each tile of the current picture
    pcPic->getPicSym()->xInitTiles();

    // Allocate some coders, now we know how many tiles there are.
    Int iNumSubstreams = pcSlice->getPPS()->getNumSubstreams();
    
    //generate the Coding Order Map and Inverse Coding Order Map
    for(p=0, uiEncCUAddr=0; p<pcPic->getPicSym()->getNumberOfCUsInFrame(); p++, uiEncCUAddr = pcPic->getPicSym()->xCalculateNxtCUAddr(uiEncCUAddr))
    {
      pcPic->getPicSym()->setCUOrderMap(p, uiEncCUAddr);
      pcPic->getPicSym()->setInverseCUOrderMap(uiEncCUAddr, p);
    }
    pcPic->getPicSym()->setCUOrderMap(pcPic->getPicSym()->getNumberOfCUsInFrame(), pcPic->getPicSym()->getNumberOfCUsInFrame());    
    pcPic->getPicSym()->setInverseCUOrderMap(pcPic->getPicSym()->getNumberOfCUsInFrame(), pcPic->getPicSym()->getNumberOfCUsInFrame());
    if (pcSlice->getPPS()->getEntropyCodingMode())
    {
      // Allocate some coders, now we know how many tiles there are.
      m_pcEncTop->createWPPCoders(iNumSubstreams);
      pcSbacCoders = m_pcEncTop->getSbacCoders();
      pcSubstreamsOut = new TComOutputBitstream[iNumSubstreams];
    }

      UInt uiStartCUAddrSliceIdx = 0; // used to index "m_uiStoredStartCUAddrForEncodingSlice" containing locations of slice boundaries
      UInt uiStartCUAddrSlice    = 0; // used to keep track of current slice's starting CU addr.
      pcSlice->setSliceCurStartCUAddr( uiStartCUAddrSlice ); // Setting "start CU addr" for current slice
      memset(m_uiStoredStartCUAddrForEncodingSlice, 0, sizeof(UInt) * (pcPic->getPicSym()->getNumberOfCUsInFrame()*pcPic->getNumPartInCU()+1));

      UInt uiStartCUAddrEntropySliceIdx = 0; // used to index "m_uiStoredStartCUAddrForEntropyEncodingSlice" containing locations of slice boundaries
      UInt uiStartCUAddrEntropySlice    = 0; // used to keep track of current Entropy slice's starting CU addr.
      pcSlice->setEntropySliceCurStartCUAddr( uiStartCUAddrEntropySlice ); // Setting "start CU addr" for current Entropy slice
      
      memset(m_uiStoredStartCUAddrForEncodingEntropySlice, 0, sizeof(UInt) * (pcPic->getPicSym()->getNumberOfCUsInFrame()*pcPic->getNumPartInCU()+1));
      UInt uiNextCUAddr = 0;
      m_uiStoredStartCUAddrForEncodingSlice[uiStartCUAddrSliceIdx++]                = uiNextCUAddr;
      m_uiStoredStartCUAddrForEncodingEntropySlice[uiStartCUAddrEntropySliceIdx++]  = uiNextCUAddr;

#if FCO_DVP_REFINE_C0132_C0170
      pcPic->setDepthCoded(false);

      if(pcSlice->getViewId() != 0)
      {
        if(pcSlice->getSPS()->isDepth() == 0 )
        {
          TComPic * recDepthMapBuffer;
          recDepthMapBuffer = m_pcEncTop->getEncTop()->getPicFromView( pcSlice->getViewId(), pcSlice->getPOC(), true );
          pcSlice->getPic()->setRecDepthMap(recDepthMapBuffer);
          if(recDepthMapBuffer->getReconMark())
          {
            pcPic->setDepthCoded(true);
          }
        }
      }
#endif

#if DEPTH_MAP_GENERATION
      // init view component and predict virtual depth map
      m_pcDepthMapGenerator->initViewComponent( pcPic );
#if !H3D_NBDV
      m_pcDepthMapGenerator->predictDepthMap  ( pcPic );
#endif
#endif
#if H3D_IVMP
      m_pcDepthMapGenerator->covertOrgDepthMap( pcPic );
#endif
#if H3D_IVRP & !QC_ARP_D0177
      m_pcResidualGenerator->initViewComponent( pcPic );
#endif

#if H3D_NBDV
      if(pcSlice->getViewId() && pcSlice->getSPS()->getMultiviewMvPredMode())
      {
        Int iColPoc = pcSlice->getRefPOC(RefPicList(pcSlice->getColDir()), pcSlice->getColRefIdx());
        pcPic->setRapbCheck(pcPic->getDisCandRefPictures(iColPoc));
      }
#endif
      while(uiNextCUAddr<uiRealEndAddress) // determine slice boundaries
      {
        pcSlice->setNextSlice       ( false );
        pcSlice->setNextEntropySlice( false );
        assert(pcPic->getNumAllocatedSlice() == uiStartCUAddrSliceIdx);
        m_pcSliceEncoder->precompressSlice( pcPic );
        m_pcSliceEncoder->compressSlice   ( pcPic );

        Bool bNoBinBitConstraintViolated = (!pcSlice->isNextSlice() && !pcSlice->isNextEntropySlice());
        if (pcSlice->isNextSlice() || (bNoBinBitConstraintViolated && m_pcCfg->getSliceMode()==AD_HOC_SLICES_FIXED_NUMBER_OF_LCU_IN_SLICE))
        {
          uiStartCUAddrSlice                                              = pcSlice->getSliceCurEndCUAddr();
          // Reconstruction slice
          m_uiStoredStartCUAddrForEncodingSlice[uiStartCUAddrSliceIdx++]  = uiStartCUAddrSlice;
          // Entropy slice
          if (uiStartCUAddrEntropySliceIdx>0 && m_uiStoredStartCUAddrForEncodingEntropySlice[uiStartCUAddrEntropySliceIdx-1] != uiStartCUAddrSlice)
          {
            m_uiStoredStartCUAddrForEncodingEntropySlice[uiStartCUAddrEntropySliceIdx++]  = uiStartCUAddrSlice;
          }
          
          if (uiStartCUAddrSlice < uiRealEndAddress)
          {
            pcPic->allocateNewSlice();          
            pcPic->setCurrSliceIdx                  ( uiStartCUAddrSliceIdx-1 );
            m_pcSliceEncoder->setSliceIdx           ( uiStartCUAddrSliceIdx-1 );
            pcSlice = pcPic->getSlice               ( uiStartCUAddrSliceIdx-1 );
            pcSlice->copySliceInfo                  ( pcPic->getSlice(0)      );
            pcSlice->setSliceIdx                    ( uiStartCUAddrSliceIdx-1 );
            pcSlice->setSliceCurStartCUAddr         ( uiStartCUAddrSlice      );
            pcSlice->setEntropySliceCurStartCUAddr  ( uiStartCUAddrSlice      );
            pcSlice->setSliceBits(0);
            uiNumSlices ++;
          }
        }
        else if (pcSlice->isNextEntropySlice() || (bNoBinBitConstraintViolated && m_pcCfg->getEntropySliceMode()==SHARP_FIXED_NUMBER_OF_LCU_IN_ENTROPY_SLICE))
        {
          uiStartCUAddrEntropySlice                                                     = pcSlice->getEntropySliceCurEndCUAddr();
          m_uiStoredStartCUAddrForEncodingEntropySlice[uiStartCUAddrEntropySliceIdx++]  = uiStartCUAddrEntropySlice;
          pcSlice->setEntropySliceCurStartCUAddr( uiStartCUAddrEntropySlice );
        }
        else
        {
          uiStartCUAddrSlice                                                            = pcSlice->getSliceCurEndCUAddr();
          uiStartCUAddrEntropySlice                                                     = pcSlice->getEntropySliceCurEndCUAddr();
        }        

        uiNextCUAddr = (uiStartCUAddrSlice > uiStartCUAddrEntropySlice) ? uiStartCUAddrSlice : uiStartCUAddrEntropySlice;
      }
      m_uiStoredStartCUAddrForEncodingSlice[uiStartCUAddrSliceIdx++]                = pcSlice->getSliceCurEndCUAddr();
      m_uiStoredStartCUAddrForEncodingEntropySlice[uiStartCUAddrEntropySliceIdx++]  = pcSlice->getSliceCurEndCUAddr();
      
      pcSlice = pcPic->getSlice(0);

#if H3D_IVRP & !QC_ARP_D0177
      // set residual picture
      m_pcResidualGenerator->setRecResidualPic( pcPic );
#endif
#if DEPTH_MAP_GENERATION
#if !H3D_NBDV
      // update virtual depth map
      m_pcDepthMapGenerator->updateDepthMap( pcPic );
#endif
#endif

      //-- Loop filter
      Bool bLFCrossTileBoundary = (pcSlice->getPPS()->getTileBehaviorControlPresentFlag() == 1)?
                                  (pcSlice->getPPS()->getLFCrossTileBoundaryFlag()):(pcSlice->getPPS()->getSPS()->getLFCrossTileBoundaryFlag());
      m_pcLoopFilter->setCfg(pcSlice->getPPS()->getDeblockingFilterControlPresent(), pcSlice->getLoopFilterDisable(), pcSlice->getLoopFilterBetaOffset(), pcSlice->getLoopFilterTcOffset(), bLFCrossTileBoundary);
      m_pcLoopFilter->loopFilterPic( pcPic );

      pcSlice = pcPic->getSlice(0);
      if(pcSlice->getSPS()->getUseSAO() || pcSlice->getSPS()->getUseALF())
      {
        Int sliceGranularity = pcSlice->getPPS()->getSliceGranularity();
        pcPic->createNonDBFilterInfo(m_uiStoredStartCUAddrForEncodingSlice, uiNumSlices, sliceGranularity, pcSlice->getSPS()->getLFCrossSliceBoundaryFlag(),pcPic->getPicSym()->getNumTiles() ,bLFCrossTileBoundary);
      }


      pcSlice = pcPic->getSlice(0);

      if(pcSlice->getSPS()->getUseSAO())
      {
        m_pcSAO->createPicSaoInfo(pcPic, uiNumSlices);
      }

      AlfParamSet* alfSliceParams = NULL;
      std::vector<AlfCUCtrlInfo>* alfCUCtrlParam = NULL;
      pcSlice = pcPic->getSlice(0);

      if(pcSlice->getSPS()->getUseALF())
      {
        m_pcAdaptiveLoopFilter->createPicAlfInfo(pcPic, uiNumSlices, pcSlice->getSliceQp());
        m_pcAdaptiveLoopFilter->initALFEnc(m_pcCfg->getALFParamInSlice(), m_pcCfg->getALFPicBasedEncode(), uiNumSlices, alfSliceParams, alfCUCtrlParam);
      }

      /////////////////////////////////////////////////////////////////////////////////////////////////// File writing
      // Set entropy coder
      m_pcEntropyCoder->setEntropyCoder   ( m_pcCavlcCoder, pcSlice );

      /* write various header sets. */
      if ( m_bSeqFirst )
      {
#if QC_MVHEVC_B0046
      if(!m_pcEncTop->getLayerId())
      {
#endif
#if VIDYO_VPS_INTEGRATION|QC_MVHEVC_B0046
        {
          OutputNALUnit nalu(NAL_UNIT_VPS, true, m_pcEncTop->getLayerId());
          m_pcEntropyCoder->setBitstream(&nalu.m_Bitstream);
          m_pcEntropyCoder->encodeVPS(m_pcEncTop->getEncTop()->getVPS());
          writeRBSPTrailingBits(nalu.m_Bitstream);
          accessUnit.push_back(new NALUnitEBSP(nalu));
        }
#endif
#if VIDYO_VPS_INTEGRATION|QC_MVHEVC_B0046
        OutputNALUnit nalu(NAL_UNIT_SPS, true, m_pcEncTop->getLayerId());
#else
        OutputNALUnit nalu(NAL_UNIT_SPS, true, m_pcEncTop->getViewId(), m_pcEncTop->getIsDepth());
#endif
        m_pcEntropyCoder->setBitstream(&nalu.m_Bitstream);
        pcSlice->getSPS()->setNumSubstreams( pcSlice->getPPS()->getNumSubstreams() );
#if HHI_MPI || H3D_QTL
        m_pcEntropyCoder->encodeSPS(pcSlice->getSPS(), m_pcEncTop->getIsDepth());
#else
        m_pcEntropyCoder->encodeSPS(pcSlice->getSPS());
#endif
        writeRBSPTrailingBits(nalu.m_Bitstream);
        accessUnit.push_back(new NALUnitEBSP(nalu));

#if VIDYO_VPS_INTEGRATION|QC_MVHEVC_B0046 
#if QC_MVHEVC_B0046
        nalu = NALUnit(NAL_UNIT_PPS, true, m_pcEncTop->getLayerId());
#else
        nalu = NALUnit(NAL_UNIT_PPS, true, m_pcEncTop->getLayerId());
#endif
#else
        nalu = NALUnit(NAL_UNIT_PPS, true, m_pcEncTop->getViewId(), m_pcEncTop->getIsDepth());
#endif
        m_pcEntropyCoder->setBitstream(&nalu.m_Bitstream);
        m_pcEntropyCoder->encodePPS(pcSlice->getPPS());
        writeRBSPTrailingBits(nalu.m_Bitstream);
        accessUnit.push_back(new NALUnitEBSP(nalu));
#if QC_MVHEVC_B0046
      }
#endif
      m_bSeqFirst = false;
    }

      /* use the main bitstream buffer for storing the marshalled picture */
      m_pcEntropyCoder->setBitstream(NULL);

      uiStartCUAddrSliceIdx = 0;
      uiStartCUAddrSlice    = 0; 

      uiStartCUAddrEntropySliceIdx = 0;
      uiStartCUAddrEntropySlice    = 0; 
      uiNextCUAddr                 = 0;
      pcSlice = pcPic->getSlice(uiStartCUAddrSliceIdx);

      Int processingState = (pcSlice->getSPS()->getUseALF() || pcSlice->getSPS()->getUseSAO() || pcSlice->getSPS()->getScalingListFlag() || pcSlice->getSPS()->getUseDF())?(EXECUTE_INLOOPFILTER):(ENCODE_SLICE);

      static Int iCurrAPSIdx = 0;
      Int iCodedAPSIdx = 0;
      TComSlice* pcSliceForAPS = NULL;

      bool skippedSlice=false;
      while (uiNextCUAddr < uiRealEndAddress) // Iterate over all slices
      {
        switch(processingState)
        {
        case ENCODE_SLICE:
          {
        pcSlice->setNextSlice       ( false );
        pcSlice->setNextEntropySlice( false );
        if (uiNextCUAddr == m_uiStoredStartCUAddrForEncodingSlice[uiStartCUAddrSliceIdx])
        {
          pcSlice = pcPic->getSlice(uiStartCUAddrSliceIdx);
#if COLLOCATED_REF_IDX
          if(uiStartCUAddrSliceIdx > 0 && pcSlice->getSliceType()!= I_SLICE)
          {
            pcSlice->checkColRefIdx(uiStartCUAddrSliceIdx, pcPic);
          }
#endif
          pcPic->setCurrSliceIdx(uiStartCUAddrSliceIdx);
          m_pcSliceEncoder->setSliceIdx(uiStartCUAddrSliceIdx);
          assert(uiStartCUAddrSliceIdx == pcSlice->getSliceIdx());
          // Reconstruction slice
          pcSlice->setSliceCurStartCUAddr( uiNextCUAddr );  // to be used in encodeSlice() + context restriction
          pcSlice->setSliceCurEndCUAddr  ( m_uiStoredStartCUAddrForEncodingSlice[uiStartCUAddrSliceIdx+1 ] );
          // Entropy slice
          pcSlice->setEntropySliceCurStartCUAddr( uiNextCUAddr );  // to be used in encodeSlice() + context restriction
          pcSlice->setEntropySliceCurEndCUAddr  ( m_uiStoredStartCUAddrForEncodingEntropySlice[uiStartCUAddrEntropySliceIdx+1 ] );

          pcSlice->setNextSlice       ( true );

          uiStartCUAddrSliceIdx++;
          uiStartCUAddrEntropySliceIdx++;
        } 
        else if (uiNextCUAddr == m_uiStoredStartCUAddrForEncodingEntropySlice[uiStartCUAddrEntropySliceIdx])
        {
          // Entropy slice
          pcSlice->setEntropySliceCurStartCUAddr( uiNextCUAddr );  // to be used in encodeSlice() + context restriction
          pcSlice->setEntropySliceCurEndCUAddr  ( m_uiStoredStartCUAddrForEncodingEntropySlice[uiStartCUAddrEntropySliceIdx+1 ] );

          pcSlice->setNextEntropySlice( true );

          uiStartCUAddrEntropySliceIdx++;
        }

      pcSlice->setRPS(pcPic->getSlice(0)->getRPS());
      pcSlice->setRPSidx(pcPic->getSlice(0)->getRPSidx());
        UInt uiDummyStartCUAddr;
        UInt uiDummyBoundingCUAddr;
        m_pcSliceEncoder->xDetermineStartAndBoundingCUAddr(uiDummyStartCUAddr,uiDummyBoundingCUAddr,pcPic,true);

        uiInternalAddress = pcPic->getPicSym()->getPicSCUAddr(pcSlice->getEntropySliceCurEndCUAddr()-1) % pcPic->getNumPartInCU();
        uiExternalAddress = pcPic->getPicSym()->getPicSCUAddr(pcSlice->getEntropySliceCurEndCUAddr()-1) / pcPic->getNumPartInCU();
        uiPosX = ( uiExternalAddress % pcPic->getFrameWidthInCU() ) * g_uiMaxCUWidth+ g_auiRasterToPelX[ g_auiZscanToRaster[uiInternalAddress] ];
        uiPosY = ( uiExternalAddress / pcPic->getFrameWidthInCU() ) * g_uiMaxCUHeight+ g_auiRasterToPelY[ g_auiZscanToRaster[uiInternalAddress] ];
        uiWidth = pcSlice->getSPS()->getPicWidthInLumaSamples();
        uiHeight = pcSlice->getSPS()->getPicHeightInLumaSamples();
        while(uiPosX>=uiWidth||uiPosY>=uiHeight)
        {
          uiInternalAddress--;
          uiPosX = ( uiExternalAddress % pcPic->getFrameWidthInCU() ) * g_uiMaxCUWidth+ g_auiRasterToPelX[ g_auiZscanToRaster[uiInternalAddress] ];
          uiPosY = ( uiExternalAddress / pcPic->getFrameWidthInCU() ) * g_uiMaxCUHeight+ g_auiRasterToPelY[ g_auiZscanToRaster[uiInternalAddress] ];
        }
        uiInternalAddress++;
        if(uiInternalAddress==pcPic->getNumPartInCU())
        {
          uiInternalAddress = 0;
          uiExternalAddress = pcPic->getPicSym()->getCUOrderMap(pcPic->getPicSym()->getInverseCUOrderMap(uiExternalAddress)+1);
        }
        UInt uiEndAddress = pcPic->getPicSym()->getPicSCUEncOrder(uiExternalAddress*pcPic->getNumPartInCU()+uiInternalAddress);
        if(uiEndAddress<=pcSlice->getEntropySliceCurStartCUAddr()) {
          UInt uiBoundingAddrSlice, uiBoundingAddrEntropySlice;
          uiBoundingAddrSlice        = m_uiStoredStartCUAddrForEncodingSlice[uiStartCUAddrSliceIdx];          
          uiBoundingAddrEntropySlice = m_uiStoredStartCUAddrForEncodingEntropySlice[uiStartCUAddrEntropySliceIdx];          
          uiNextCUAddr               = min(uiBoundingAddrSlice, uiBoundingAddrEntropySlice);
          if(pcSlice->isNextSlice())
          {
            skippedSlice=true;
          }
          continue;
        }
        if(skippedSlice) 
        {
          pcSlice->setNextSlice       ( true );
          pcSlice->setNextEntropySlice( false );
        }
        skippedSlice=false;
        if (pcSlice->getPPS()->getEntropyCodingMode())
        {
          pcSlice->allocSubstreamSizes( iNumSubstreams );
          for ( UInt ui = 0 ; ui < iNumSubstreams; ui++ )
          pcSubstreamsOut[ui].clear();
        }

        m_pcEntropyCoder->setEntropyCoder   ( m_pcCavlcCoder, pcSlice );
        m_pcEntropyCoder->resetEntropy      ();
        /* start slice NALunit */
        OutputNALUnit nalu( pcSlice->getNalUnitType(), pcSlice->isReferenced(),
#if !VIDYO_VPS_INTEGRATION &!QC_MVHEVC_B0046
                           m_pcEncTop->getViewId(), m_pcEncTop->getIsDepth(), pcSlice->getTLayer() );
#else
                           m_pcEncTop->getLayerId(), pcSlice->getTLayer() );
#endif
            
        Bool bEntropySlice = (!pcSlice->isNextSlice());
        if (!bEntropySlice)
        {
          uiOneBitstreamPerSliceLength = 0; // start of a new slice
        }

        // used while writing slice header
        Int iTransmitLWHeader = (m_pcCfg->getTileMarkerFlag()==0) ? 0 : 1;
        pcSlice->setTileMarkerFlag ( iTransmitLWHeader );
        m_pcEntropyCoder->setBitstream(&nalu.m_Bitstream);
#if !CABAC_INIT_FLAG
        pcSlice->setCABACinitIDC(pcSlice->getSliceType());
#endif

        m_pcEntropyCoder->encodeSliceHeader(pcSlice);

        if(pcSlice->isNextSlice())
        {
          if (pcSlice->getSPS()->getUseALF())
          {
            if(pcSlice->getAlfEnabledFlag())
            {

              if( pcSlice->getSPS()->getUseALFCoefInSlice())
              {
                Int iNumSUinLCU    = 1<< (g_uiMaxCUDepth << 1); 
                Int firstLCUAddr   = pcSlice->getSliceCurStartCUAddr() / iNumSUinLCU;  
                Bool isAcrossSlice = pcSlice->getSPS()->getLFCrossSliceBoundaryFlag();
                m_pcEntropyCoder->encodeAlfParam( &(alfSliceParams[pcSlice->getSliceIdx()]), false, firstLCUAddr, isAcrossSlice);
              }

              if( !pcSlice->getSPS()->getUseALFCoefInSlice())
              {
                AlfCUCtrlInfo& cAlfCUCtrlParam = (*alfCUCtrlParam)[pcSlice->getSliceIdx()];
              if(cAlfCUCtrlParam.cu_control_flag)
              {
                m_pcEntropyCoder->setAlfCtrl( true );
                m_pcEntropyCoder->setMaxAlfCtrlDepth(cAlfCUCtrlParam.alf_max_depth);
                m_pcCavlcCoder->setAlfCtrl(true);
                m_pcCavlcCoder->setMaxAlfCtrlDepth(cAlfCUCtrlParam.alf_max_depth); 
              }
              else
              {
                m_pcEntropyCoder->setAlfCtrl(false);
              }
              m_pcEntropyCoder->encodeAlfCtrlParam(cAlfCUCtrlParam, m_pcAdaptiveLoopFilter->getNumCUsInPic());
            
              }
            }
          }
        }
        m_pcEntropyCoder->encodeTileMarkerFlag(pcSlice);

        // is it needed?
        {
          if (!bEntropySlice)
          {
            pcBitstreamRedirect->writeAlignOne();
          }
          else
          {
          // We've not completed our slice header info yet, do the alignment later.
          }
          m_pcSbacCoder->init( (TEncBinIf*)m_pcBinCABAC );
          m_pcEntropyCoder->setEntropyCoder ( m_pcSbacCoder, pcSlice );
          m_pcEntropyCoder->resetEntropy    ();
          for ( UInt ui = 0 ; ui < pcSlice->getPPS()->getNumSubstreams() ; ui++ )
          {
            m_pcEntropyCoder->setEntropyCoder ( &pcSbacCoders[ui], pcSlice );
            m_pcEntropyCoder->resetEntropy    ();
          }
        }

        if(pcSlice->isNextSlice())
        {
          // set entropy coder for writing
          m_pcSbacCoder->init( (TEncBinIf*)m_pcBinCABAC );
          {
            for ( UInt ui = 0 ; ui < pcSlice->getPPS()->getNumSubstreams() ; ui++ )
            {
              m_pcEntropyCoder->setEntropyCoder ( &pcSbacCoders[ui], pcSlice );
              m_pcEntropyCoder->resetEntropy    ();
            }
            pcSbacCoders[0].load(m_pcSbacCoder);
            m_pcEntropyCoder->setEntropyCoder ( &pcSbacCoders[0], pcSlice );  //ALF is written in substream #0 with CABAC coder #0 (see ALF param encoding below)
          }
          m_pcEntropyCoder->resetEntropy    ();
          // File writing
          if (!bEntropySlice)
          {
            m_pcEntropyCoder->setBitstream(pcBitstreamRedirect);
          }
          else
          {
            m_pcEntropyCoder->setBitstream(&nalu.m_Bitstream);
          }
          // for now, override the TILES_DECODER setting in order to write substreams.
            m_pcEntropyCoder->setBitstream    ( &pcSubstreamsOut[0] );

        }
        pcSlice->setFinalized(true);

          m_pcSbacCoder->load( &pcSbacCoders[0] );

        pcSlice->setTileOffstForMultES( uiOneBitstreamPerSliceLength );
        if (!bEntropySlice)
        {
          pcSlice->setTileLocationCount ( 0 );
          m_pcSliceEncoder->encodeSlice(pcPic, pcBitstreamRedirect, pcSubstreamsOut); // redirect is only used for CAVLC tile position info.
        }
        else
        {
          m_pcSliceEncoder->encodeSlice(pcPic, &nalu.m_Bitstream, pcSubstreamsOut); // nalu.m_Bitstream is only used for CAVLC tile position info.
        }

        {
          // Construct the final bitstream by flushing and concatenating substreams.
          // The final bitstream is either nalu.m_Bitstream or pcBitstreamRedirect;
          UInt* puiSubstreamSizes = pcSlice->getSubstreamSizes();
          UInt uiTotalCodedSize = 0; // for padding calcs.
          UInt uiNumSubstreamsPerTile = iNumSubstreams;
          if (iNumSubstreams > 1)
          {
            uiNumSubstreamsPerTile /= pcPic->getPicSym()->getNumTiles();
          }
          for ( UInt ui = 0 ; ui < iNumSubstreams; ui++ )
          {
            // Flush all substreams -- this includes empty ones.
            // Terminating bit and flush.
            m_pcEntropyCoder->setEntropyCoder   ( &pcSbacCoders[ui], pcSlice );
            m_pcEntropyCoder->setBitstream      (  &pcSubstreamsOut[ui] );
            m_pcEntropyCoder->encodeTerminatingBit( 1 );
            m_pcEntropyCoder->encodeSliceFinish();
            pcSubstreamsOut[ui].write( 1, 1 ); // stop bit.
            pcSubstreamsOut[ui].writeAlignZero();
            // Byte alignment is necessary between tiles when tiles are independent.
            uiTotalCodedSize += pcSubstreamsOut[ui].getNumberOfWrittenBits();

            {
              Bool bNextSubstreamInNewTile = ((ui+1) < iNumSubstreams)
                                             && ((ui+1)%uiNumSubstreamsPerTile == 0);
              if (bNextSubstreamInNewTile)
              {
                // byte align.
                while (uiTotalCodedSize&0x7)
                {
                  pcSubstreamsOut[ui].write(0, 1);
                  uiTotalCodedSize++;
                }
              }
              Bool bRecordOffsetNext = m_pcCfg->getTileLocationInSliceHeaderFlag()
                                            && bNextSubstreamInNewTile;
              if (bRecordOffsetNext)
                pcSlice->setTileLocation(ui/uiNumSubstreamsPerTile, pcSlice->getTileOffstForMultES()+(uiTotalCodedSize>>3));
            }
            if (ui+1 < pcSlice->getPPS()->getNumSubstreams())
              puiSubstreamSizes[ui] = pcSubstreamsOut[ui].getNumberOfWrittenBits();
          }
          // Complete the slice header info.
          m_pcEntropyCoder->setEntropyCoder   ( m_pcCavlcCoder, pcSlice );
          m_pcEntropyCoder->setBitstream(&nalu.m_Bitstream);
          if (m_pcCfg->getTileLocationInSliceHeaderFlag()==0) 
          {
            pcSlice->setTileLocationCount( 0 );
          }
          m_pcEntropyCoder->encodeTilesWPPEntryPoint( pcSlice );
          // Substreams...
          TComOutputBitstream *pcOut = pcBitstreamRedirect;
          // xWriteTileLocation will perform byte-alignment...
          {
            if (bEntropySlice)
            {
              // In these cases, padding is necessary here.
              pcOut = &nalu.m_Bitstream;
              pcOut->writeAlignOne();
            }
          }
          UInt uiAccumulatedLength = 0;
          for ( UInt ui = 0 ; ui < pcSlice->getPPS()->getNumSubstreams(); ui++ )
          {
            pcOut->addSubstream(&pcSubstreamsOut[ui]);

            // Update tile marker location information
            for (Int uiMrkIdx = 0; uiMrkIdx < pcSubstreamsOut[ui].getTileMarkerLocationCount(); uiMrkIdx++)
            {
              UInt uiBottom = pcOut->getTileMarkerLocationCount();
              pcOut->setTileMarkerLocation      ( uiBottom, uiAccumulatedLength + pcSubstreamsOut[ui].getTileMarkerLocation( uiMrkIdx ) );
              pcOut->setTileMarkerLocationCount ( uiBottom + 1 );
            }
            uiAccumulatedLength = (pcOut->getNumberOfWrittenBits() >> 3);
          }
        }

        UInt uiBoundingAddrSlice, uiBoundingAddrEntropySlice;
        uiBoundingAddrSlice        = m_uiStoredStartCUAddrForEncodingSlice[uiStartCUAddrSliceIdx];          
        uiBoundingAddrEntropySlice = m_uiStoredStartCUAddrForEncodingEntropySlice[uiStartCUAddrEntropySliceIdx];          
        uiNextCUAddr               = min(uiBoundingAddrSlice, uiBoundingAddrEntropySlice);
        // If current NALU is the first NALU of slice (containing slice header) and more NALUs exist (due to multiple entropy slices) then buffer it.
        // If current NALU is the last NALU of slice and a NALU was buffered, then (a) Write current NALU (b) Update an write buffered NALU at approproate location in NALU list.
        Bool bNALUAlignedWrittenToList    = false; // used to ensure current NALU is not written more than once to the NALU list.
        xWriteTileLocationToSliceHeader(nalu, pcBitstreamRedirect, pcSlice);
        writeRBSPTrailingBits(nalu.m_Bitstream);
        accessUnit.push_back(new NALUnitEBSP(nalu));
        bNALUAlignedWrittenToList = true; 
        uiOneBitstreamPerSliceLength += nalu.m_Bitstream.getNumberOfWrittenBits(); // length of bitstream after byte-alignment

        if (!bNALUAlignedWrittenToList)
        {
        {
          nalu.m_Bitstream.writeAlignZero();
        }
        accessUnit.push_back(new NALUnitEBSP(nalu));
        uiOneBitstreamPerSliceLength += nalu.m_Bitstream.getNumberOfWrittenBits() + 24; // length of bitstream after byte-alignment + 3 byte startcode 0x000001
        }


        processingState = ENCODE_SLICE;
          }
          break;
        case EXECUTE_INLOOPFILTER:
          {
            TComAPS cAPS;
            allocAPS(&cAPS, pcSlice->getSPS());
            cAPS.setSaoInterleavingFlag(m_pcCfg->getSaoInterleavingFlag());
            // set entropy coder for RD
            m_pcEntropyCoder->setEntropyCoder ( m_pcCavlcCoder, pcSlice );

            if ( pcSlice->getSPS()->getUseSAO() )
            {
              m_pcEntropyCoder->resetEntropy();
              m_pcEntropyCoder->setBitstream( m_pcBitCounter );
              m_pcSAO->startSaoEnc(pcPic, m_pcEntropyCoder, m_pcEncTop->getRDSbacCoder(), NULL);
              SAOParam& cSaoParam = *(cAPS.getSaoParam());

#if SAO_CHROMA_LAMBDA 
              m_pcSAO->SAOProcess(&cSaoParam, pcPic->getSlice(0)->getLambdaLuma(), pcPic->getSlice(0)->getLambdaChroma());
#else
#if ALF_CHROMA_LAMBDA
              m_pcSAO->SAOProcess(&cSaoParam, pcPic->getSlice(0)->getLambdaLuma());
#else
              m_pcSAO->SAOProcess(&cSaoParam, pcPic->getSlice(0)->getLambda());
#endif
#endif
              m_pcSAO->endSaoEnc();

              m_pcAdaptiveLoopFilter->PCMLFDisableProcess(pcPic);
            }

            // adaptive loop filter
            if ( pcSlice->getSPS()->getUseALF())
            {
              m_pcEntropyCoder->resetEntropy    ();
              m_pcEntropyCoder->setBitstream    ( m_pcBitCounter );
              m_pcAdaptiveLoopFilter->startALFEnc(pcPic, m_pcEntropyCoder );
              AlfParamSet* pAlfEncParam = (pcSlice->getSPS()->getUseALFCoefInSlice())?( alfSliceParams ):( cAPS.getAlfParam());
#if ALF_CHROMA_LAMBDA 
#if HHI_INTERVIEW_SKIP
              m_pcAdaptiveLoopFilter->ALFProcess(pAlfEncParam, alfCUCtrlParam, pcPic->getSlice(0)->getLambdaLuma(), pcPic->getSlice(0)->getLambdaChroma(), m_pcEncTop->getInterViewSkip()  );
#else
              m_pcAdaptiveLoopFilter->ALFProcess(pAlfEncParam, alfCUCtrlParam, pcPic->getSlice(0)->getLambdaLuma(), pcPic->getSlice(0)->getLambdaChroma() );
#endif
#else
#if SAO_CHROMA_LAMBDA
#if HHI_INTERVIEW_SKIP
              m_pcAdaptiveLoopFilter->ALFProcess(pAlfEncParam, alfCUCtrlParam, pcPic->getSlice(0)->getLambdaLuma(), m_pcEncTop->getInterViewSkip());
#else
              m_pcAdaptiveLoopFilter->ALFProcess(pAlfEncParam, alfCUCtrlParam, pcPic->getSlice(0)->getLambdaLuma());
#endif
#else
#if HHI_INTERVIEW_SKIP
              m_pcAdaptiveLoopFilter->ALFProcess(pAlfEncParam, alfCUCtrlParam, pcPic->getSlice(0)->getLambda(), m_pcEncTop->getInterViewSkip() );
#else
              m_pcAdaptiveLoopFilter->ALFProcess(pAlfEncParam, alfCUCtrlParam, pcPic->getSlice(0)->getLambda());
#endif
#endif
#endif

              m_pcAdaptiveLoopFilter->endALFEnc();

              m_pcAdaptiveLoopFilter->PCMLFDisableProcess(pcPic);
            }
            iCodedAPSIdx = iCurrAPSIdx;  
            pcSliceForAPS = pcSlice;

            assignNewAPS(cAPS, iCodedAPSIdx, vAPS, pcSliceForAPS);
            iCurrAPSIdx = (iCurrAPSIdx +1)%MAX_NUM_SUPPORTED_APS;
            processingState = ENCODE_APS;

            //set APS link to the slices
            for(Int s=0; s< uiNumSlices; s++)
            {
              if (pcSlice->getSPS()->getUseALF())
              {
                pcPic->getSlice(s)->setAlfEnabledFlag(  (pcSlice->getSPS()->getUseALFCoefInSlice())?(alfSliceParams[s].isEnabled[ALF_Y]):(cAPS.getAlfEnabled())   );
              }
              if (pcSlice->getSPS()->getUseSAO())
              {
                pcPic->getSlice(s)->setSaoEnabledFlag((cAPS.getSaoParam()->bSaoFlag[0]==1)?true:false);
              }
              pcPic->getSlice(s)->setAPS(&(vAPS[iCodedAPSIdx]));
              pcPic->getSlice(s)->setAPSId(iCodedAPSIdx);
            }
          }
          break;
        case ENCODE_APS:
          {
#if VIDYO_VPS_INTEGRATION | QC_MVHEVC_B0046
            OutputNALUnit nalu(NAL_UNIT_APS, true, m_pcEncTop->getLayerId());
#else
            OutputNALUnit nalu(NAL_UNIT_APS, true, m_pcEncTop->getViewId(), m_pcEncTop->getIsDepth());
#endif
            encodeAPS(&(vAPS[iCodedAPSIdx]), nalu.m_Bitstream, pcSliceForAPS);
            accessUnit.push_back(new NALUnitEBSP(nalu));

            processingState = ENCODE_SLICE;
          }
          break;
        default:
          {
            printf("Not a supported encoding state\n");
            assert(0);
            exit(-1);
          }
        }
      } // end iteration over slices


      if(pcSlice->getSPS()->getUseSAO() || pcSlice->getSPS()->getUseALF())
      {
        if(pcSlice->getSPS()->getUseSAO())
        {
          m_pcSAO->destroyPicSaoInfo();
        }

        if(pcSlice->getSPS()->getUseALF())
        {
          m_pcAdaptiveLoopFilter->uninitALFEnc(alfSliceParams, alfCUCtrlParam);
          m_pcAdaptiveLoopFilter->destroyPicAlfInfo();
        }

        pcPic->destroyNonDBFilterInfo();
      }

#if HHI_INTERVIEW_SKIP
      if (pcPic->getUsedPelsMap())
        pcPic->removeUsedPelsMapBuffer() ;
#endif
#if H3D_IVMP
      pcPic->removeOrgDepthMapBuffer();
#endif
   
   //   pcPic->compressMotion(); 
      m_pocLastCoded = pcPic->getPOC();
      
      //-- For time output for each slice
      Double dEncTime = (double)(clock()-iBeforeTime) / CLOCKS_PER_SEC;

      const char* digestStr = NULL;
      if (m_pcCfg->getPictureDigestEnabled())
      {
        /* calculate MD5sum for entire reconstructed picture */
        SEIpictureDigest sei_recon_picture_digest;
        sei_recon_picture_digest.method = SEIpictureDigest::MD5;
        calcMD5(*pcPic->getPicYuvRec(), sei_recon_picture_digest.digest);
        digestStr = digestToString(sei_recon_picture_digest.digest);

#if VIDYO_VPS_INTEGRATION | QC_MVHEVC_B0046
        OutputNALUnit nalu(NAL_UNIT_SEI, false, m_pcEncTop->getLayerId());
#else
        OutputNALUnit nalu(NAL_UNIT_SEI, false, m_pcEncTop->getViewId(), m_pcEncTop->getIsDepth());
#endif

        /* write the SEI messages */
        m_pcEntropyCoder->setEntropyCoder(m_pcCavlcCoder, pcSlice);
        m_pcEntropyCoder->setBitstream(&nalu.m_Bitstream);
        m_pcEntropyCoder->encodeSEI(sei_recon_picture_digest);
        writeRBSPTrailingBits(nalu.m_Bitstream);

        /* insert the SEI message NALUnit before any Slice NALUnits */
        AccessUnit::iterator it = find_if(accessUnit.begin(), accessUnit.end(), mem_fun(&NALUnit::isSlice));
        accessUnit.insert(it, new NALUnitEBSP(nalu));
      }

      xCalculateAddPSNR( pcPic, pcPic->getPicYuvRec(), accessUnit, dEncTime );
      if (digestStr)
        printf(" [MD5:%s]", digestStr);

#if FIXED_ROUNDING_FRAME_MEMORY
      /* TODO: this should happen after copyToPic(pcPicYuvRecOut) */
      pcPic->getPicYuvRec()->xFixedRoundingPic();
#endif
      pcPic->getPicYuvRec()->copyToPic(pcPicYuvRecOut);
      
      pcPic->setReconMark   ( true );

      pcPic->setUsedForTMVP ( true );

      m_bFirst = false;
      m_iNumPicCoded++;

      /* logging: insert a newline at end of picture period */
      printf("\n");
      fflush(stdout);
  }
  
  delete[] pcSubstreamsOut;
  delete pcBitstreamRedirect;

}

/** Memory allocation for APS
  * \param [out] pAPS APS pointer
  * \param [in] pSPS SPS pointer
  */
Void TEncGOP::allocAPS (TComAPS* pAPS, TComSPS* pSPS)
{
  if(pSPS->getUseSAO())
  {
    pAPS->createSaoParam();
    m_pcSAO->allocSaoParam(pAPS->getSaoParam());
  }
  if(pSPS->getUseALF())
  {
    pAPS->createAlfParam();
    //alf Enabled flag in APS is false after pAPS->createAlfParam();
    if(!pSPS->getUseALFCoefInSlice())
    {
      pAPS->getAlfParam()->create(m_pcAdaptiveLoopFilter->getNumLCUInPicWidth(), m_pcAdaptiveLoopFilter->getNumLCUInPicHeight(), m_pcAdaptiveLoopFilter->getNumCUsInPic());
      pAPS->getAlfParam()->createALFParam();
    }
  }
}

/** Memory deallocation for APS
  * \param [out] pAPS APS pointer
  * \param [in] pSPS SPS pointer
  */
Void TEncGOP::freeAPS (TComAPS* pAPS, TComSPS* pSPS)
{
  if(pSPS->getUseSAO())
  {
    if(pAPS->getSaoParam() != NULL)
    {
      m_pcSAO->freeSaoParam(pAPS->getSaoParam());
      pAPS->destroySaoParam();

    }
  }
  if(pSPS->getUseALF())
  {
    if(pAPS->getAlfParam() != NULL)
    {
      if(!pSPS->getUseALFCoefInSlice())
      {
        pAPS->getAlfParam()->releaseALFParam();
      }
      pAPS->destroyAlfParam();
    }
  }
}

/** Assign APS object into APS container according to APS ID
  * \param [in] cAPS APS object
  * \param [in] apsID APS ID
  * \param [in,out] vAPS APS container
  * \param [in] pcSlice pointer to slice
  */
Void TEncGOP::assignNewAPS(TComAPS& cAPS, Int apsID, std::vector<TComAPS>& vAPS, TComSlice* pcSlice)
{

  cAPS.setAPSID(apsID);
  if(pcSlice->getPOC() == 0)
  {
    cAPS.setScalingListEnabled(pcSlice->getSPS()->getScalingListFlag());
  }
  else
  {
    cAPS.setScalingListEnabled(false);
  }

  cAPS.setSaoEnabled(pcSlice->getSPS()->getUseSAO() ? (cAPS.getSaoParam()->bSaoFlag[0] ):(false));
  cAPS.setAlfEnabled(pcSlice->getSPS()->getUseALF() ? (cAPS.getAlfParam()->isEnabled[0]):(false));
  cAPS.setLoopFilterOffsetInAPS(m_pcCfg->getLoopFilterOffsetInAPS());
  cAPS.setLoopFilterDisable(m_pcCfg->getLoopFilterDisable());
  cAPS.setLoopFilterBetaOffset(m_pcCfg->getLoopFilterBetaOffset());
  cAPS.setLoopFilterTcOffset(m_pcCfg->getLoopFilterTcOffset());

  //assign new APS into APS container
  Int apsBufSize= (Int)vAPS.size();

  if(apsID >= apsBufSize)
  {
    vAPS.resize(apsID +1);
  }

  freeAPS(&(vAPS[apsID]), pcSlice->getSPS());
  vAPS[apsID] = cAPS;
}


/** encode APS syntax elements
  * \param [in] pcAPS APS pointer
  * \param [in, out] APSbs bitstream
  * \param [in] pointer to slice (just used for entropy coder initialization)
  */
Void TEncGOP::encodeAPS(TComAPS* pcAPS, TComOutputBitstream& APSbs, TComSlice* pcSlice)
{
  m_pcEntropyCoder->setEntropyCoder   ( m_pcCavlcCoder, pcSlice);
  m_pcEntropyCoder->resetEntropy      ();
  m_pcEntropyCoder->setBitstream(&APSbs);

  m_pcEntropyCoder->encodeAPSInitInfo(pcAPS);
  if(pcAPS->getScalingListEnabled())
  {
    m_pcEntropyCoder->encodeScalingList( pcSlice->getScalingList() );
  }
  if(pcAPS->getLoopFilterOffsetInAPS())
  {
    m_pcEntropyCoder->encodeDFParams(pcAPS);
  }
  m_pcEntropyCoder->encodeSaoParam(pcAPS);
  m_pcEntropyCoder->encodeAPSAlfFlag( pcAPS->getAlfEnabled()?1:0);
  if(pcAPS->getAlfEnabled())
  {
    m_pcEntropyCoder->encodeAlfParam(pcAPS->getAlfParam());
  }

  m_pcEntropyCoder->encodeApsExtensionFlag();
  //neither SAO and ALF is enabled
  writeRBSPTrailingBits(APSbs);
}

Void TEncGOP::preLoopFilterPicAll( TComPic* pcPic, UInt64& ruiDist, UInt64& ruiBits )
{
  TComSlice* pcSlice = pcPic->getSlice(pcPic->getCurrSliceIdx());
  Bool bCalcDist = false;
  m_pcLoopFilter->setCfg(pcSlice->getPPS()->getDeblockingFilterControlPresent(), pcSlice->getLoopFilterDisable(), m_pcCfg->getLoopFilterBetaOffset(), m_pcCfg->getLoopFilterTcOffset(), m_pcCfg->getLFCrossTileBoundaryFlag());
  m_pcLoopFilter->loopFilterPic( pcPic );
  
  m_pcEntropyCoder->setEntropyCoder ( m_pcEncTop->getRDGoOnSbacCoder(), pcSlice );
  m_pcEntropyCoder->resetEntropy    ();
  m_pcEntropyCoder->setBitstream    ( m_pcBitCounter );
  pcSlice = pcPic->getSlice(0);
  if(pcSlice->getSPS()->getUseSAO() || pcSlice->getSPS()->getUseALF())
  {
    pcPic->createNonDBFilterInfo();
  }
  
  // Adaptive Loop filter
  if( pcSlice->getSPS()->getUseALF() )
  {
    m_pcAdaptiveLoopFilter->createPicAlfInfo(pcPic);

    AlfParamSet* alfParamSet;
    std::vector<AlfCUCtrlInfo>* alfCUCtrlParam = NULL;
    alfParamSet= new AlfParamSet;
    alfParamSet->create( m_pcAdaptiveLoopFilter->getNumLCUInPicWidth(), m_pcAdaptiveLoopFilter->getNumLCUInPicHeight(), m_pcAdaptiveLoopFilter->getNumCUsInPic());
    alfParamSet->createALFParam();
    m_pcAdaptiveLoopFilter->initALFEnc(false, true, 1, alfParamSet, alfCUCtrlParam);
    m_pcAdaptiveLoopFilter->startALFEnc(pcPic, m_pcEntropyCoder);
    


#if ALF_CHROMA_LAMBDA 
#if HHI_INTERVIEW_SKIP
    m_pcAdaptiveLoopFilter->ALFProcess(alfParamSet, NULL, pcPic->getSlice(0)->getLambdaLuma(), pcPic->getSlice(0)->getLambdaChroma(), m_pcEncTop->getInterViewSkip()  );
#else
    m_pcAdaptiveLoopFilter->ALFProcess(alfParamSet, NULL, pcPic->getSlice(0)->getLambdaLuma(), pcPic->getSlice(0)->getLambdaChroma() );
#endif
#else
#if SAO_CHROMA_LAMBDA
    m_pcAdaptiveLoopFilter->ALFProcess(alfParamSet, NULL, pcPic->getSlice(0)->getLambdaLuma(), m_pcEncTop->getInterViewSkip());
#if HHI_INTERVIEW_SKIP
#else
    m_pcAdaptiveLoopFilter->ALFProcess(alfParamSet, NULL, pcPic->getSlice(0)->getLambdaLuma());
#endif
#else
#if HHI_INTERVIEW_SKIP
    m_pcAdaptiveLoopFilter->ALFProcess(alfParamSet, NULL, pcPic->getSlice(0)->getLambda(), m_pcEncTop->getInterViewSkip());
#else
    m_pcAdaptiveLoopFilter->ALFProcess(alfParamSet, NULL, pcPic->getSlice(0)->getLambda());
#endif
#endif
#endif

    m_pcAdaptiveLoopFilter->endALFEnc();

    alfParamSet->releaseALFParam();
    delete alfParamSet;
    delete alfCUCtrlParam;
    m_pcAdaptiveLoopFilter->PCMLFDisableProcess(pcPic);
    m_pcAdaptiveLoopFilter->destroyPicAlfInfo();
  }
  if( pcSlice->getSPS()->getUseSAO() || pcSlice->getSPS()->getUseALF())
  {
    pcPic->destroyNonDBFilterInfo();
  }
  
  m_pcEntropyCoder->resetEntropy    ();
  ruiBits += m_pcEntropyCoder->getNumberOfWrittenBits();
  
  if (!bCalcDist)
    ruiDist = xFindDistortionFrame(pcPic->getPicYuvOrg(), pcPic->getPicYuvRec());
}

// ====================================================================================================================
// Protected member functions
// ====================================================================================================================

Void TEncGOP::xInitGOP( Int iPOCLast, Int iNumPicRcvd, TComList<TComPic*>& rcListPic, TComList<TComPicYuv*>& rcListPicYuvRecOut )
{
  assert( iNumPicRcvd > 0 );
  //  Exception for the first frame
  if ( iPOCLast == 0 )
  {
    m_iGopSize    = 1;
  }
  else
    m_iGopSize    = m_pcCfg->getGOPSize();
  
  assert (m_iGopSize > 0); 

  return;
}

Void TEncGOP::xGetBuffer( TComList<TComPic*>&       rcListPic,
                         TComList<TComPicYuv*>&    rcListPicYuvRecOut,
                         Int                       iNumPicRcvd,
                         Int                       iTimeOffset,
                         TComPic*&                 rpcPic,
                         TComPicYuv*&              rpcPicYuvRecOut,
                         UInt                      uiPOCCurr )
{
  Int i;
  //  Rec. output
  TComList<TComPicYuv*>::iterator     iterPicYuvRec = rcListPicYuvRecOut.end();
  for ( i = 0; i < iNumPicRcvd - iTimeOffset + 1; i++ )
  {
    iterPicYuvRec--;
  }
  
  rpcPicYuvRecOut = *(iterPicYuvRec);
  
  //  Current pic.
  TComList<TComPic*>::iterator        iterPic       = rcListPic.begin();
  while (iterPic != rcListPic.end())
  {
    rpcPic = *(iterPic);
    rpcPic->setCurrSliceIdx(0);
    if (rpcPic->getPOC() == (Int)uiPOCCurr)
    {
      break;
    }
    iterPic++;
  }
  
  assert (rpcPic->getPOC() == (Int)uiPOCCurr);
  
  return;
}

UInt64 TEncGOP::xFindDistortionFrame (TComPicYuv* pcPic0, TComPicYuv* pcPic1)
{
  Int     x, y;
  Pel*  pSrc0   = pcPic0 ->getLumaAddr();
  Pel*  pSrc1   = pcPic1 ->getLumaAddr();
#if IBDI_DISTORTION
  Int  iShift = g_uiBitIncrement;
  Int  iOffset = 1<<(g_uiBitIncrement-1);
#else
  UInt  uiShift = g_uiBitIncrement<<1;
#endif
  Int   iTemp;
  
  Int   iStride = pcPic0->getStride();
  Int   iWidth  = pcPic0->getWidth();
  Int   iHeight = pcPic0->getHeight();
  
  UInt64  uiTotalDiff = 0;
  
  for( y = 0; y < iHeight; y++ )
  {
    for( x = 0; x < iWidth; x++ )
    {
#if IBDI_DISTORTION
      iTemp = ((pSrc0[x]+iOffset)>>iShift) - ((pSrc1[x]+iOffset)>>iShift); uiTotalDiff += iTemp * iTemp;
#else
      iTemp = pSrc0[x] - pSrc1[x]; uiTotalDiff += (iTemp*iTemp) >> uiShift;
#endif
    }
    pSrc0 += iStride;
    pSrc1 += iStride;
  }
  
  iHeight >>= 1;
  iWidth  >>= 1;
  iStride >>= 1;
  
  pSrc0  = pcPic0->getCbAddr();
  pSrc1  = pcPic1->getCbAddr();
  
  for( y = 0; y < iHeight; y++ )
  {
    for( x = 0; x < iWidth; x++ )
    {
#if IBDI_DISTORTION
      iTemp = ((pSrc0[x]+iOffset)>>iShift) - ((pSrc1[x]+iOffset)>>iShift); uiTotalDiff += iTemp * iTemp;
#else
      iTemp = pSrc0[x] - pSrc1[x]; uiTotalDiff += (iTemp*iTemp) >> uiShift;
#endif
    }
    pSrc0 += iStride;
    pSrc1 += iStride;
  }
  
  pSrc0  = pcPic0->getCrAddr();
  pSrc1  = pcPic1->getCrAddr();
  
  for( y = 0; y < iHeight; y++ )
  {
    for( x = 0; x < iWidth; x++ )
    {
#if IBDI_DISTORTION
      iTemp = ((pSrc0[x]+iOffset)>>iShift) - ((pSrc1[x]+iOffset)>>iShift); uiTotalDiff += iTemp * iTemp;
#else
      iTemp = pSrc0[x] - pSrc1[x]; uiTotalDiff += (iTemp*iTemp) >> uiShift;
#endif
    }
    pSrc0 += iStride;
    pSrc1 += iStride;
  }
  
  return uiTotalDiff;
}

#if VERBOSE_RATE
static const char* nalUnitTypeToString(NalUnitType type)
{
  switch (type)
  {
  case NAL_UNIT_CODED_SLICE: return "SLICE";
#if !QC_REM_IDV_B0046
  case NAL_UNIT_CODED_SLICE_IDV: return "IDV";
#endif
  case NAL_UNIT_CODED_SLICE_CRA: return "CRA";
  case NAL_UNIT_CODED_SLICE_TLA: return "TLA";
  case NAL_UNIT_CODED_SLICE_IDR: return "IDR";
  case NAL_UNIT_SEI: return "SEI";
  case NAL_UNIT_SPS: return "SPS";
  case NAL_UNIT_PPS: return "PPS";
  case NAL_UNIT_FILLER_DATA: return "FILLER";
  default: return "UNK";
  }
}
#endif

Void TEncGOP::xCalculateAddPSNR( TComPic* pcPic, TComPicYuv* pcPicD, const AccessUnit& accessUnit, Double dEncTime )
{
  Int     x, y;
  UInt64 uiSSDY  = 0;
  UInt64 uiSSDU  = 0;
  UInt64 uiSSDV  = 0;
  
  Double  dYPSNR  = 0.0;
  Double  dUPSNR  = 0.0;
  Double  dVPSNR  = 0.0;
  
  //===== calculate PSNR =====
  Pel*  pOrg    = pcPic ->getPicYuvOrg()->getLumaAddr();
  Pel*  pRec    = pcPicD->getLumaAddr();
  Int   iStride = pcPicD->getStride();
  
  Int   iWidth;
  Int   iHeight;
  
  iWidth  = pcPicD->getWidth () - m_pcEncTop->getPad(0);
  iHeight = pcPicD->getHeight() - m_pcEncTop->getPad(1);
  
  Int   iSize   = iWidth*iHeight;
  
  for( y = 0; y < iHeight; y++ )
  {
    for( x = 0; x < iWidth; x++ )
    {
      Int iDiff = (Int)( pOrg[x] - pRec[x] );
      uiSSDY   += iDiff * iDiff;
    }
    pOrg += iStride;
    pRec += iStride;
  }
  
#if HHI_VSO
#if HHI_VSO_SYNTH_DIST_OUT
  if ( m_pcRdCost->getUseRenModel() )
  {
    unsigned int maxval = 255 * (1<<(g_uiBitDepth + g_uiBitIncrement -8));
    Double fRefValueY = (double) maxval * maxval * iSize;
    Double fRefValueC = fRefValueY / 4.0;
    TRenModel*  pcRenModel = m_pcEncTop->getEncTop()->getRenModel();
    Int64 iDistVSOY, iDistVSOU, iDistVSOV;
    pcRenModel->getTotalSSE( iDistVSOY, iDistVSOU, iDistVSOV );
    dYPSNR = ( iDistVSOY ? 10.0 * log10( fRefValueY / (Double) iDistVSOY ) : 99.99 );
    dUPSNR = ( iDistVSOU ? 10.0 * log10( fRefValueC / (Double) iDistVSOU ) : 99.99 );
    dVPSNR = ( iDistVSOV ? 10.0 * log10( fRefValueC / (Double) iDistVSOV ) : 99.99 );
  }
  else
#endif
#endif
  {
  iHeight >>= 1;
  iWidth  >>= 1;
  iStride >>= 1;
  pOrg  = pcPic ->getPicYuvOrg()->getCbAddr();
  pRec  = pcPicD->getCbAddr();
  
  for( y = 0; y < iHeight; y++ )
  {
    for( x = 0; x < iWidth; x++ )
    {
      Int iDiff = (Int)( pOrg[x] - pRec[x] );
      uiSSDU   += iDiff * iDiff;
    }
    pOrg += iStride;
    pRec += iStride;
  }
  
  pOrg  = pcPic ->getPicYuvOrg()->getCrAddr();
  pRec  = pcPicD->getCrAddr();
  
  for( y = 0; y < iHeight; y++ )
  {
    for( x = 0; x < iWidth; x++ )
    {
      Int iDiff = (Int)( pOrg[x] - pRec[x] );
      uiSSDV   += iDiff * iDiff;
    }
    pOrg += iStride;
    pRec += iStride;
  }
  
  unsigned int maxval = 255 * (1<<(g_uiBitDepth + g_uiBitIncrement -8));
  Double fRefValueY = (double) maxval * maxval * iSize;
  Double fRefValueC = fRefValueY / 4.0;
  dYPSNR            = ( uiSSDY ? 10.0 * log10( fRefValueY / (Double)uiSSDY ) : 99.99 );
  dUPSNR            = ( uiSSDU ? 10.0 * log10( fRefValueC / (Double)uiSSDU ) : 99.99 );
  dVPSNR            = ( uiSSDV ? 10.0 * log10( fRefValueC / (Double)uiSSDV ) : 99.99 );
  }
  /* calculate the size of the access unit, excluding:
   *  - any AnnexB contributions (start_code_prefix, zero_byte, etc.,)
   *  - SEI NAL units
   */
  unsigned numRBSPBytes = 0;
  for (AccessUnit::const_iterator it = accessUnit.begin(); it != accessUnit.end(); it++)
  {
    unsigned numRBSPBytes_nal = unsigned((*it)->m_nalUnitData.str().size());
#if VERBOSE_RATE
    printf("*** %6s numBytesInNALunit: %u\n", nalUnitTypeToString((*it)->m_nalUnitType), numRBSPBytes_nal);
#endif
    if ((*it)->m_nalUnitType != NAL_UNIT_SEI)
      numRBSPBytes += numRBSPBytes_nal;
  }

  unsigned uibits = numRBSPBytes * 8;
  m_vRVM_RP.push_back( uibits );

  //===== add PSNR =====
  m_pcEncTop->getAnalyzeAll()->addResult (dYPSNR, dUPSNR, dVPSNR, (Double)uibits);
  TComSlice*  pcSlice = pcPic->getSlice(0);
  if (pcSlice->isIntra())
  {
    m_pcEncTop->getAnalyzeI()->addResult (dYPSNR, dUPSNR, dVPSNR, (Double)uibits);
  }
  if (pcSlice->isInterP())
  {
    m_pcEncTop->getAnalyzeP()->addResult (dYPSNR, dUPSNR, dVPSNR, (Double)uibits);
  }
  if (pcSlice->isInterB())
  {
    m_pcEncTop->getAnalyzeB()->addResult (dYPSNR, dUPSNR, dVPSNR, (Double)uibits);
  }

  Char c = (pcSlice->isIntra() ? 'I' : pcSlice->isInterP() ? 'P' : 'B');
  if (!pcSlice->isReferenced()) c += 32;

#if ADAPTIVE_QP_SELECTION
  printf("%s   View %3d POC %4d TId: %1d ( %c-SLICE, nQP %d QP %d ) %10d bits",
         pcSlice->getIsDepth() ? "Depth  " : "Texture",
         pcSlice->getViewId(),
         pcSlice->getPOC(),
         pcSlice->getTLayer(),
         c,
         pcSlice->getSliceQpBase(),
         pcSlice->getSliceQp(),
         uibits );
#else
  printf("%s   View %3d POC %4d TId: %1d ( %c-SLICE, QP %d ) %10d bits",
         pcSlice->getIsDepth() ? "Depth  " : "Texture",
         pcSlice->getViewId(),
         pcSlice->getPOC()-pcSlice->getLastIDR(),
         pcSlice->getTLayer(),
         c,
         pcSlice->getSliceQp(),
         uibits );
#endif

  printf(" [Y %6.4lf dB    U %6.4lf dB    V %6.4lf dB]", dYPSNR, dUPSNR, dVPSNR );
  printf(" [ET %5.0f ]", dEncTime );
  
  for (Int iRefList = 0; iRefList < 2; iRefList++)
  {
    printf(" [L%d ", iRefList);
    for (Int iRefIndex = 0; iRefIndex < pcSlice->getNumRefIdx(RefPicList(iRefList)); iRefIndex++)
    {
      if( pcSlice->getViewId() != pcSlice->getRefViewId( RefPicList(iRefList), iRefIndex ) )
      {
        printf( "V%d ", pcSlice->getRefViewId( RefPicList(iRefList), iRefIndex ) );
      }
      else
      {
        printf ("%d ", pcSlice->getRefPOC(RefPicList(iRefList), iRefIndex)-pcSlice->getLastIDR());
      }
    }
    printf("]");
  }
  if(pcSlice->getNumRefIdx(REF_PIC_LIST_C)>0 && !pcSlice->getNoBackPredFlag())
  {
    printf(" [LC ");
    for (Int iRefIndex = 0; iRefIndex < pcSlice->getNumRefIdx(REF_PIC_LIST_C); iRefIndex++)
    {
      if( pcSlice->getViewId() != pcSlice->getRefViewId( (RefPicList)pcSlice->getListIdFromIdxOfLC(iRefIndex), pcSlice->getRefIdxFromIdxOfLC(iRefIndex) ) )
      {
        printf( "V%d ", pcSlice->getRefViewId( (RefPicList)pcSlice->getListIdFromIdxOfLC(iRefIndex), pcSlice->getRefIdxFromIdxOfLC(iRefIndex) ) );
      }
      else
      {
        printf ("%d ", pcSlice->getRefPOC((RefPicList)pcSlice->getListIdFromIdxOfLC(iRefIndex), pcSlice->getRefIdxFromIdxOfLC(iRefIndex))-pcSlice->getLastIDR());
      }
    }
    printf("]");
  }
}

/** Function for deciding the nal_unit_type.
 * \param uiPOCCurr POC of the current picture
 * \returns the nal_unit type of the picture
 * This function checks the configuration and returns the appropriate nal_unit_type for the picture.
 */
NalUnitType TEncGOP::getNalUnitType(UInt uiPOCCurr)
{
  Bool bInterViewOnlySlice = ( m_pcCfg->getGOPEntry(MAX_GOP).m_POC == 0 && (m_pcCfg->getGOPEntry(MAX_GOP).m_sliceType == 'P' || m_pcCfg->getGOPEntry(MAX_GOP).m_sliceType == 'B') );

  if (uiPOCCurr == 0)
  {
    if( bInterViewOnlySlice ) 
    { 
#if !QC_REM_IDV_B0046
      return NAL_UNIT_CODED_SLICE_IDV; 
#else
      return NAL_UNIT_CODED_SLICE_IDR;
#endif
    }
    else
    { 
      return NAL_UNIT_CODED_SLICE_IDR;
    }
  }
  if (uiPOCCurr % m_pcCfg->getIntraPeriod() == 0)
  {
    if (m_pcCfg->getDecodingRefreshType() == 1)
    {
      if( bInterViewOnlySlice ) 
      { 
#if !QC_REM_IDV_B0046
        return NAL_UNIT_CODED_SLICE_IDV; 
#else
        return NAL_UNIT_CODED_SLICE_CRA; 
#endif
      }
      else
      { 
      return NAL_UNIT_CODED_SLICE_CRA;
      }
    }
    else if (m_pcCfg->getDecodingRefreshType() == 2)
    {
      if( bInterViewOnlySlice ) 
      { 
#if !QC_REM_IDV_B0046
        return NAL_UNIT_CODED_SLICE_IDV; 
#else
        return NAL_UNIT_CODED_SLICE_IDR;
#endif
      }
      else
      { 
        return NAL_UNIT_CODED_SLICE_IDR;
      }
    }
  }
  return NAL_UNIT_CODED_SLICE;
}

NalUnitType TEncGOP::getNalUnitTypeBaseViewMvc(UInt uiPOCCurr)
{
  if( uiPOCCurr == 0 )
  {
    return NAL_UNIT_CODED_SLICE_IDR;
  }
  if( uiPOCCurr % m_pcCfg->getIntraPeriod() == 0 )
  {
    if( m_pcCfg->getDecodingRefreshType() == 1 )
    {
      return NAL_UNIT_CODED_SLICE_CRA;
    }
    else if( m_pcCfg->getDecodingRefreshType() == 2 )
    {
      return NAL_UNIT_CODED_SLICE_IDR;
    }
  }
  return NAL_UNIT_CODED_SLICE;
}

Double TEncGOP::xCalculateRVM()
{
  Double dRVM = 0;
  
  if( m_pcCfg->getGOPSize() == 1 && m_pcCfg->getIntraPeriod() != 1 && m_pcCfg->getFrameToBeEncoded() > RVM_VCEGAM10_M * 2 )
  {
    // calculate RVM only for lowdelay configurations
    std::vector<Double> vRL , vB;
    size_t N = m_vRVM_RP.size();
    vRL.resize( N );
    vB.resize( N );
    
    Int i;
    Double dRavg = 0 , dBavg = 0;
    vB[RVM_VCEGAM10_M] = 0;
    for( i = RVM_VCEGAM10_M + 1 ; i < N - RVM_VCEGAM10_M + 1 ; i++ )
    {
      vRL[i] = 0;
      for( Int j = i - RVM_VCEGAM10_M ; j <= i + RVM_VCEGAM10_M - 1 ; j++ )
        vRL[i] += m_vRVM_RP[j];
      vRL[i] /= ( 2 * RVM_VCEGAM10_M );
      vB[i] = vB[i-1] + m_vRVM_RP[i] - vRL[i];
      dRavg += m_vRVM_RP[i];
      dBavg += vB[i];
    }
    
    dRavg /= ( N - 2 * RVM_VCEGAM10_M );
    dBavg /= ( N - 2 * RVM_VCEGAM10_M );
    
    double dSigamB = 0;
    for( i = RVM_VCEGAM10_M + 1 ; i < N - RVM_VCEGAM10_M + 1 ; i++ )
    {
      Double tmp = vB[i] - dBavg;
      dSigamB += tmp * tmp;
    }
    dSigamB = sqrt( dSigamB / ( N - 2 * RVM_VCEGAM10_M ) );
    
    double f = sqrt( 12.0 * ( RVM_VCEGAM10_M - 1 ) / ( RVM_VCEGAM10_M + 1 ) );
    
    dRVM = dSigamB / dRavg * f;
  }
  
  return( dRVM );
}

/** Determine the difference between consecutive tile sizes (in bytes) and writes it to  bistream rNalu [slice header]
 * \param rpcBitstreamRedirect contains the bitstream to be concatenated to rNalu. rpcBitstreamRedirect contains slice payload. rpcSlice contains tile location information.
 * \returns Updates rNalu to contain concatenated bitstream. rpcBitstreamRedirect is cleared at the end of this function call.
 */
Void TEncGOP::xWriteTileLocationToSliceHeader (OutputNALUnit& rNalu, TComOutputBitstream*& rpcBitstreamRedirect, TComSlice*& rpcSlice)
{
  {
  }

  // Byte-align
  rNalu.m_Bitstream.writeAlignOne();

  // Update tile marker locations
  TComOutputBitstream *pcOut = &rNalu.m_Bitstream;
  UInt uiAccumulatedLength   = pcOut->getNumberOfWrittenBits() >> 3;
  for (Int uiMrkIdx = 0; uiMrkIdx < rpcBitstreamRedirect->getTileMarkerLocationCount(); uiMrkIdx++)
  {
    UInt uiBottom = pcOut->getTileMarkerLocationCount();
    pcOut->setTileMarkerLocation      ( uiBottom, uiAccumulatedLength + rpcBitstreamRedirect->getTileMarkerLocation( uiMrkIdx ) );
    pcOut->setTileMarkerLocationCount ( uiBottom + 1 );
  }

  // Perform bitstream concatenation
  if (rpcBitstreamRedirect->getNumberOfWrittenBits() > 0)
  {
    UInt uiBitCount  = rpcBitstreamRedirect->getNumberOfWrittenBits();
    if (rpcBitstreamRedirect->getByteStreamLength()>0)
    {
      UChar *pucStart  =  reinterpret_cast<UChar*>(rpcBitstreamRedirect->getByteStream());
      UInt uiWriteByteCount = 0;
      while (uiWriteByteCount < (uiBitCount >> 3) )
      {
        UInt uiBits = (*pucStart);
        rNalu.m_Bitstream.write(uiBits, 8);
        pucStart++;
        uiWriteByteCount++;
      }
    }
    UInt uiBitsHeld = (uiBitCount & 0x07);
    for (UInt uiIdx=0; uiIdx < uiBitsHeld; uiIdx++)
    {
      rNalu.m_Bitstream.write((rpcBitstreamRedirect->getHeldBits() & (1 << (7-uiIdx))) >> (7-uiIdx), 1);
    }          
  }

  m_pcEntropyCoder->setBitstream(&rNalu.m_Bitstream);

  delete rpcBitstreamRedirect;
  rpcBitstreamRedirect = new TComOutputBitstream;
}

Void TEncGOP::xSetRefPicListModificationsMvc( TComSlice* pcSlice, UInt uiPOCCurr, UInt iGOPid )
{
  if( pcSlice->getSliceType() == I_SLICE || !(pcSlice->getSPS()->getListsModificationPresentFlag()) || pcSlice->getSPS()->getNumberOfUsableInterViewRefs() == 0 )
  {
    return;
  }

  // analyze inter-view modifications
#if !QC_REM_IDV_B0046
  GOPEntryMvc gem = m_pcCfg->getGOPEntry( (getNalUnitType(uiPOCCurr) == NAL_UNIT_CODED_SLICE_IDV) ? MAX_GOP : iGOPid );
#else
  Bool bRAP = ((getNalUnitType(uiPOCCurr) == NAL_UNIT_CODED_SLICE_IDR) || (getNalUnitType(uiPOCCurr) == NAL_UNIT_CODED_SLICE_CRA)) && (pcSlice->getSPS()->getViewId()) ? 1:0;
  GOPEntryMvc gem = m_pcCfg->getGOPEntry( bRAP ? MAX_GOP : iGOPid );
#endif
  Int numL0Modifications = 0;
  Int numL1Modifications = 0;
  for( Int k = 0; k < gem.m_numInterViewRefPics; k++ )
  {
    if( gem.m_interViewRefPosL0[k] > 0 ) { numL0Modifications++; }
    if( gem.m_interViewRefPosL1[k] > 0 ) { numL1Modifications++; }
  }

  TComRefPicListModification* refPicListModification = pcSlice->getRefPicListModification();
  Int maxRefListSize = pcSlice->getNumPocTotalCurrMvc();
  Int numTemporalRefs = pcSlice->getNumPocTotalCurr();

  // set L0 inter-view modifications
  if( (maxRefListSize > 1) && (numL0Modifications > 0) )
  {
    refPicListModification->setRefPicListModificationFlagL0( true );
    Int tempListEntryL0[16];
    for( Int k = 0; k < 16; k++ ) { tempListEntryL0[k] = -1; }
    
    Bool hasModification = false;
    for( Int k = 0; k < gem.m_numInterViewRefPics; k++ )
    {
      if( gem.m_interViewRefPosL0[k] > 0 )
      {
        for( Int l = 0; l < pcSlice->getSPS()->getNumberOfUsableInterViewRefs(); l++ )
        {
          if( gem.m_interViewRefs[k] == pcSlice->getSPS()->getUsableInterViewRef( l ) && (gem.m_interViewRefPosL0[k] - 1) != (numTemporalRefs + l) )
          {
            tempListEntryL0[gem.m_interViewRefPosL0[k]-1] = numTemporalRefs + l;
            hasModification = true;
          }
        }
      }
    }

    if( hasModification )
    {
      Int temporalRefIdx = 0;
      for( Int i = 0; i < pcSlice->getNumRefIdx( REF_PIC_LIST_0 ); i++ )
      {
        if( tempListEntryL0[i] >= 0 ) 
        {
          refPicListModification->setRefPicSetIdxL0( i, tempListEntryL0[i] );
        }
        else
        {
          refPicListModification->setRefPicSetIdxL0( i, temporalRefIdx );
          temporalRefIdx++;
        }
      }
    }
    else
    {
      refPicListModification->setRefPicListModificationFlagL0( false );
    }
  }

  // set L1 inter-view modifications
  if( (maxRefListSize > 1) && (numL1Modifications > 0) )
  {
    refPicListModification->setRefPicListModificationFlagL1( true );
    Int tempListEntryL1[16];
    for( Int k = 0; k < 16; k++ ) { tempListEntryL1[k] = -1; }

    Bool hasModification = false;
    for( Int k = 0; k < gem.m_numInterViewRefPics; k++ )
    {
      if( gem.m_interViewRefPosL1[k] > 0 )
      {
        for( Int l = 0; l < pcSlice->getSPS()->getNumberOfUsableInterViewRefs(); l++ )
        {
          if( gem.m_interViewRefs[k] == pcSlice->getSPS()->getUsableInterViewRef( l ) && (gem.m_interViewRefPosL1[k] - 1) != (numTemporalRefs + l) )
          {
            tempListEntryL1[gem.m_interViewRefPosL1[k]-1] = numTemporalRefs + l;
            hasModification = true;
          }
        }
      }
    }

    if( hasModification )
    {
      Int temporalRefIdx = 0;
      for( Int i = 0; i < pcSlice->getNumRefIdx( REF_PIC_LIST_1 ); i++ )
      {
        if( tempListEntryL1[i] >= 0 ) 
        {
          refPicListModification->setRefPicSetIdxL1( i, tempListEntryL1[i] );
        }
        else
        {
          refPicListModification->setRefPicSetIdxL1( i, temporalRefIdx );
          temporalRefIdx++;
        }
      }
    } 
    else
    {
      refPicListModification->setRefPicListModificationFlagL1( false );
    }
  }

  return;
}
//! \}
