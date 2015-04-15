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

/** \file     TEncGOP.cpp
    \brief    GOP encoder class
*/

#include <list>
#include <algorithm>
#include <functional>

#include "TEncTop.h"
#include "TEncGOP.h"
#include "TEncAnalyze.h"
#include "libmd5/MD5.h"
#include "TLibCommon/SEI.h"
#include "TLibCommon/NAL.h"
#include "NALwrite.h"
#include <time.h>
#include <math.h>

using namespace std;
//! \ingroup TLibEncoder
//! \{

// ====================================================================================================================
// Constructor / destructor / initialization / destroy
// ====================================================================================================================
Int getLSB(Int poc, Int maxLSB)
{
  if (poc >= 0)
  {
    return poc % maxLSB;
  }
  else
  {
    return (maxLSB - ((-poc) % maxLSB)) % maxLSB;
  }
}

TEncGOP::TEncGOP()
{
  m_iLastIDR            = 0;
  m_iGopSize            = 0;
  m_iNumPicCoded        = 0; //Niko
  m_bFirst              = true;
#if ALLOW_RECOVERY_POINT_AS_RAP
  m_iLastRecoveryPicPOC = 0;
#endif
  
  m_pcCfg               = NULL;
  m_pcSliceEncoder      = NULL;
  m_pcListPic           = NULL;
  
  m_pcEntropyCoder      = NULL;
  m_pcCavlcCoder        = NULL;
  m_pcSbacCoder         = NULL;
  m_pcBinCABAC          = NULL;
  
  m_bSeqFirst           = true;
  
  m_bRefreshPending     = 0;
  m_pocCRA            = 0;
  m_numLongTermRefPicSPS = 0;
  ::memset(m_ltRefPicPocLsbSps, 0, sizeof(m_ltRefPicPocLsbSps));
  ::memset(m_ltRefPicUsedByCurrPicFlag, 0, sizeof(m_ltRefPicUsedByCurrPicFlag));
  m_cpbRemovalDelay   = 0;
  m_lastBPSEI         = 0;
  xResetNonNestedSEIPresentFlags();
  xResetNestedSEIPresentFlags();
#if H_MV
  m_layerId      = 0;
  m_viewId       = 0;
  m_pocLastCoded = -1; 
#if H_3D
  m_viewIndex  =   0; 
  m_isDepth = false;
#endif
#endif
#if FIX1172
  m_associatedIRAPType = NAL_UNIT_CODED_SLICE_IDR_N_LP;
  m_associatedIRAPPOC  = 0;
#endif
  return;
}

TEncGOP::~TEncGOP()
{
}

/** Create list to contain pointers to LCU start addresses of slice.
 */
Void  TEncGOP::create()
{
  m_bLongtermTestPictureHasBeenCoded = 0;
  m_bLongtermTestPictureHasBeenCoded2 = 0;
}

Void  TEncGOP::destroy()
{
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
  
  //--Adaptive Loop filter
  m_pcSAO                = pcTEncTop->getSAO();
  m_pcRateCtrl           = pcTEncTop->getRateCtrl();
  m_lastBPSEI          = 0;
  m_totalCoded         = 0;

#if H_MV
  m_ivPicLists           = pcTEncTop->getIvPicLists(); 
  m_layerId              = pcTEncTop->getLayerId();
  m_viewId               = pcTEncTop->getViewId();
#if H_3D
  m_viewIndex            = pcTEncTop->getViewIndex();
  m_isDepth              = pcTEncTop->getIsDepth();
#endif
#endif
#if H_3D_IC
  m_aICEnableCandidate   = pcTEncTop->getICEnableCandidate(); 
  m_aICEnableNum         = pcTEncTop->getICEnableNum(); 
#endif
#if KWU_FIX_URQ
  m_pcRateCtrl           = pcTEncTop->getRateCtrl();
#endif
}

SEIActiveParameterSets* TEncGOP::xCreateSEIActiveParameterSets (TComSPS *sps)
{
  SEIActiveParameterSets *seiActiveParameterSets = new SEIActiveParameterSets(); 
  seiActiveParameterSets->activeVPSId = m_pcCfg->getVPS()->getVPSId(); 
  seiActiveParameterSets->m_selfContainedCvsFlag = false;
  seiActiveParameterSets->m_noParameterSetUpdateFlag = false;
  seiActiveParameterSets->numSpsIdsMinus1 = 0;
  seiActiveParameterSets->activeSeqParameterSetId.resize(seiActiveParameterSets->numSpsIdsMinus1 + 1); 
  seiActiveParameterSets->activeSeqParameterSetId[0] = sps->getSPSId();
  return seiActiveParameterSets;
}

SEIFramePacking* TEncGOP::xCreateSEIFramePacking()
{
  SEIFramePacking *seiFramePacking = new SEIFramePacking();
  seiFramePacking->m_arrangementId = m_pcCfg->getFramePackingArrangementSEIId();
  seiFramePacking->m_arrangementCancelFlag = 0;
  seiFramePacking->m_arrangementType = m_pcCfg->getFramePackingArrangementSEIType();
  assert((seiFramePacking->m_arrangementType > 2) && (seiFramePacking->m_arrangementType < 6) );
  seiFramePacking->m_quincunxSamplingFlag = m_pcCfg->getFramePackingArrangementSEIQuincunx();
  seiFramePacking->m_contentInterpretationType = m_pcCfg->getFramePackingArrangementSEIInterpretation();
  seiFramePacking->m_spatialFlippingFlag = 0;
  seiFramePacking->m_frame0FlippedFlag = 0;
  seiFramePacking->m_fieldViewsFlag = (seiFramePacking->m_arrangementType == 2);
  seiFramePacking->m_currentFrameIsFrame0Flag = ((seiFramePacking->m_arrangementType == 5) && m_iNumPicCoded&1);
  seiFramePacking->m_frame0SelfContainedFlag = 0;
  seiFramePacking->m_frame1SelfContainedFlag = 0;
  seiFramePacking->m_frame0GridPositionX = 0;
  seiFramePacking->m_frame0GridPositionY = 0;
  seiFramePacking->m_frame1GridPositionX = 0;
  seiFramePacking->m_frame1GridPositionY = 0;
  seiFramePacking->m_arrangementReservedByte = 0;
  seiFramePacking->m_arrangementPersistenceFlag = true;
  seiFramePacking->m_upsampledAspectRatio = 0;
  return seiFramePacking;
}

SEIDisplayOrientation* TEncGOP::xCreateSEIDisplayOrientation()
{
  SEIDisplayOrientation *seiDisplayOrientation = new SEIDisplayOrientation();
  seiDisplayOrientation->cancelFlag = false;
  seiDisplayOrientation->horFlip = false;
  seiDisplayOrientation->verFlip = false;
  seiDisplayOrientation->anticlockwiseRotation = m_pcCfg->getDisplayOrientationSEIAngle();
  return seiDisplayOrientation;
}

SEIToneMappingInfo*  TEncGOP::xCreateSEIToneMappingInfo()
{
  SEIToneMappingInfo *seiToneMappingInfo = new SEIToneMappingInfo();
  seiToneMappingInfo->m_toneMapId = m_pcCfg->getTMISEIToneMapId();
  seiToneMappingInfo->m_toneMapCancelFlag = m_pcCfg->getTMISEIToneMapCancelFlag();
  seiToneMappingInfo->m_toneMapPersistenceFlag = m_pcCfg->getTMISEIToneMapPersistenceFlag();

  seiToneMappingInfo->m_codedDataBitDepth = m_pcCfg->getTMISEICodedDataBitDepth();
  assert(seiToneMappingInfo->m_codedDataBitDepth >= 8 && seiToneMappingInfo->m_codedDataBitDepth <= 14);
  seiToneMappingInfo->m_targetBitDepth = m_pcCfg->getTMISEITargetBitDepth();
  assert( seiToneMappingInfo->m_targetBitDepth >= 1 && seiToneMappingInfo->m_targetBitDepth <= 17 );
  seiToneMappingInfo->m_modelId = m_pcCfg->getTMISEIModelID();
  assert(seiToneMappingInfo->m_modelId >=0 &&seiToneMappingInfo->m_modelId<=4);

  switch( seiToneMappingInfo->m_modelId)
  {
  case 0:
    {
      seiToneMappingInfo->m_minValue = m_pcCfg->getTMISEIMinValue();
      seiToneMappingInfo->m_maxValue = m_pcCfg->getTMISEIMaxValue();
      break;
    }
  case 1:
    {
      seiToneMappingInfo->m_sigmoidMidpoint = m_pcCfg->getTMISEISigmoidMidpoint();
      seiToneMappingInfo->m_sigmoidWidth = m_pcCfg->getTMISEISigmoidWidth();
      break;
    }
  case 2:
    {
      UInt num = 1u<<(seiToneMappingInfo->m_targetBitDepth);
      seiToneMappingInfo->m_startOfCodedInterval.resize(num);
      Int* ptmp = m_pcCfg->getTMISEIStartOfCodedInterva();
      if(ptmp)
      {
        for(int i=0; i<num;i++)
        {
          seiToneMappingInfo->m_startOfCodedInterval[i] = ptmp[i];
        }
      }
      break;
    }
  case 3:
    {
      seiToneMappingInfo->m_numPivots = m_pcCfg->getTMISEINumPivots();
      seiToneMappingInfo->m_codedPivotValue.resize(seiToneMappingInfo->m_numPivots);
      seiToneMappingInfo->m_targetPivotValue.resize(seiToneMappingInfo->m_numPivots);
      Int* ptmpcoded = m_pcCfg->getTMISEICodedPivotValue();
      Int* ptmptarget = m_pcCfg->getTMISEITargetPivotValue();
      if(ptmpcoded&&ptmptarget)
      {
        for(int i=0; i<(seiToneMappingInfo->m_numPivots);i++)
        {
          seiToneMappingInfo->m_codedPivotValue[i]=ptmpcoded[i];
          seiToneMappingInfo->m_targetPivotValue[i]=ptmptarget[i];
         }
       }
       break;
     }
  case 4:
     {
       seiToneMappingInfo->m_cameraIsoSpeedIdc = m_pcCfg->getTMISEICameraIsoSpeedIdc();
       seiToneMappingInfo->m_cameraIsoSpeedValue = m_pcCfg->getTMISEICameraIsoSpeedValue();
       assert( seiToneMappingInfo->m_cameraIsoSpeedValue !=0 );
       seiToneMappingInfo->m_exposureIndexIdc = m_pcCfg->getTMISEIExposurIndexIdc();
       seiToneMappingInfo->m_exposureIndexValue = m_pcCfg->getTMISEIExposurIndexValue();
       assert( seiToneMappingInfo->m_exposureIndexValue !=0 );
       seiToneMappingInfo->m_exposureCompensationValueSignFlag = m_pcCfg->getTMISEIExposureCompensationValueSignFlag();
       seiToneMappingInfo->m_exposureCompensationValueNumerator = m_pcCfg->getTMISEIExposureCompensationValueNumerator();
       seiToneMappingInfo->m_exposureCompensationValueDenomIdc = m_pcCfg->getTMISEIExposureCompensationValueDenomIdc();
       seiToneMappingInfo->m_refScreenLuminanceWhite = m_pcCfg->getTMISEIRefScreenLuminanceWhite();
       seiToneMappingInfo->m_extendedRangeWhiteLevel = m_pcCfg->getTMISEIExtendedRangeWhiteLevel();
       assert( seiToneMappingInfo->m_extendedRangeWhiteLevel >= 100 );
       seiToneMappingInfo->m_nominalBlackLevelLumaCodeValue = m_pcCfg->getTMISEINominalBlackLevelLumaCodeValue();
       seiToneMappingInfo->m_nominalWhiteLevelLumaCodeValue = m_pcCfg->getTMISEINominalWhiteLevelLumaCodeValue();
       assert( seiToneMappingInfo->m_nominalWhiteLevelLumaCodeValue > seiToneMappingInfo->m_nominalBlackLevelLumaCodeValue );
       seiToneMappingInfo->m_extendedWhiteLevelLumaCodeValue = m_pcCfg->getTMISEIExtendedWhiteLevelLumaCodeValue();
       assert( seiToneMappingInfo->m_extendedWhiteLevelLumaCodeValue >= seiToneMappingInfo->m_nominalWhiteLevelLumaCodeValue );
       break;
    }
  default:
    {
      assert(!"Undefined SEIToneMapModelId");
      break;
    }
  }
  return seiToneMappingInfo;
}

#if H_MV
SEISubBitstreamProperty *TEncGOP::xCreateSEISubBitstreamProperty( TComSPS *sps)
{
  SEISubBitstreamProperty *seiSubBitstreamProperty = new SEISubBitstreamProperty();

  seiSubBitstreamProperty->m_activeVpsId = sps->getVPSId();
  /* These values can be determined by the encoder; for now we will use the input parameter */
  TEncTop *encTop = this->m_pcEncTop;
  seiSubBitstreamProperty->m_numAdditionalSubStreams = encTop->getNumAdditionalSubStreams();
  seiSubBitstreamProperty->m_subBitstreamMode        = encTop->getSubBitstreamMode();
  seiSubBitstreamProperty->m_outputLayerSetIdxToVps  = encTop->getOutputLayerSetIdxToVps();
  seiSubBitstreamProperty->m_highestSublayerId       = encTop->getHighestSublayerId();
  seiSubBitstreamProperty->m_avgBitRate              = encTop->getAvgBitRate();
  seiSubBitstreamProperty->m_maxBitRate              = encTop->getMaxBitRate();

  return seiSubBitstreamProperty;
}
#endif

Void TEncGOP::xCreateLeadingSEIMessages (/*SEIMessages seiMessages,*/ AccessUnit &accessUnit, TComSPS *sps)
{
  OutputNALUnit nalu(NAL_UNIT_PREFIX_SEI);

  if(m_pcCfg->getActiveParameterSetsSEIEnabled())
  {
    SEIActiveParameterSets *sei = xCreateSEIActiveParameterSets (sps);

    //nalu = NALUnit(NAL_UNIT_SEI); 
    m_pcEntropyCoder->setBitstream(&nalu.m_Bitstream);
    m_seiWriter.writeSEImessage(nalu.m_Bitstream, *sei, sps); 
    writeRBSPTrailingBits(nalu.m_Bitstream);
    accessUnit.push_back(new NALUnitEBSP(nalu));
    delete sei;
    m_activeParameterSetSEIPresentInAU = true;
  }

  if(m_pcCfg->getFramePackingArrangementSEIEnabled())
  {
    SEIFramePacking *sei = xCreateSEIFramePacking ();

    nalu = NALUnit(NAL_UNIT_PREFIX_SEI);
    m_pcEntropyCoder->setBitstream(&nalu.m_Bitstream);
    m_seiWriter.writeSEImessage(nalu.m_Bitstream, *sei, sps);
    writeRBSPTrailingBits(nalu.m_Bitstream);
    accessUnit.push_back(new NALUnitEBSP(nalu));
    delete sei;
  }
  if (m_pcCfg->getDisplayOrientationSEIAngle())
  {
    SEIDisplayOrientation *sei = xCreateSEIDisplayOrientation();

    nalu = NALUnit(NAL_UNIT_PREFIX_SEI); 
    m_pcEntropyCoder->setBitstream(&nalu.m_Bitstream);
    m_seiWriter.writeSEImessage(nalu.m_Bitstream, *sei, sps); 
    writeRBSPTrailingBits(nalu.m_Bitstream);
    accessUnit.push_back(new NALUnitEBSP(nalu));
    delete sei;
  }
  if(m_pcCfg->getToneMappingInfoSEIEnabled())
  {
    SEIToneMappingInfo *sei = xCreateSEIToneMappingInfo ();
      
    nalu = NALUnit(NAL_UNIT_PREFIX_SEI); 
    m_pcEntropyCoder->setBitstream(&nalu.m_Bitstream);
    m_seiWriter.writeSEImessage(nalu.m_Bitstream, *sei, sps); 
    writeRBSPTrailingBits(nalu.m_Bitstream);
    accessUnit.push_back(new NALUnitEBSP(nalu));
    delete sei;
  }
#if H_MV
  if( m_pcCfg->getSubBitstreamPropSEIEnabled() )
  {
    SEISubBitstreamProperty *sei = xCreateSEISubBitstreamProperty ( sps );

    nalu = NALUnit(NAL_UNIT_PREFIX_SEI); 
    m_pcEntropyCoder->setBitstream(&nalu.m_Bitstream);
    m_seiWriter.writeSEImessage(nalu.m_Bitstream, *sei, sps); 
    writeRBSPTrailingBits(nalu.m_Bitstream);
    accessUnit.push_back(new NALUnitEBSP(nalu));
    delete sei;
  }
#endif
}

// ====================================================================================================================
// Public member functions
// ====================================================================================================================
#if H_MV
Void TEncGOP::initGOP( Int iPOCLast, Int iNumPicRcvd, TComList<TComPic*>& rcListPic, TComList<TComPicYuv*>& rcListPicYuvRecOut, std::list<AccessUnit>& accessUnitsInGOP)
{
#if H_MV_ALIGN_HM_15
  xInitGOP( iPOCLast, iNumPicRcvd, rcListPic, rcListPicYuvRecOut, false );
#else
  xInitGOP( iPOCLast, iNumPicRcvd, rcListPic, rcListPicYuvRecOut );
#endif
  m_iNumPicCoded = 0;
}
#endif
#if H_MV
Void TEncGOP::compressPicInGOP( Int iPOCLast, Int iNumPicRcvd, TComList<TComPic*>& rcListPic, TComList<TComPicYuv*>& rcListPicYuvRecOut, std::list<AccessUnit>& accessUnitsInGOP, Int iGOPid, bool isField, bool isTff)
#else
Void TEncGOP::compressGOP( Int iPOCLast, Int iNumPicRcvd, TComList<TComPic*>& rcListPic, TComList<TComPicYuv*>& rcListPicYuvRecOut, std::list<AccessUnit>& accessUnitsInGOP, bool isField, bool isTff)
#endif
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

#if !H_MV
  xInitGOP( iPOCLast, iNumPicRcvd, rcListPic, rcListPicYuvRecOut, isField );

  
  m_iNumPicCoded = 0;
#endif
  SEIPictureTiming pictureTimingSEI;
  Bool writeSOP = m_pcCfg->getSOPDescriptionSEIEnabled();
  // Initialize Scalable Nesting SEI with single layer values
  SEIScalableNesting scalableNestingSEI;
  scalableNestingSEI.m_bitStreamSubsetFlag           = 1;      // If the nested SEI messages are picture buffereing SEI mesages, picure timing SEI messages or sub-picture timing SEI messages, bitstream_subset_flag shall be equal to 1
  scalableNestingSEI.m_nestingOpFlag                 = 0;
  scalableNestingSEI.m_nestingNumOpsMinus1           = 0;      //nesting_num_ops_minus1
  scalableNestingSEI.m_allLayersFlag                 = 0;
  scalableNestingSEI.m_nestingNoOpMaxTemporalIdPlus1 = 6 + 1;  //nesting_no_op_max_temporal_id_plus1
  scalableNestingSEI.m_nestingNumLayersMinus1        = 1 - 1;  //nesting_num_layers_minus1
  scalableNestingSEI.m_nestingLayerId[0]             = 0;
  scalableNestingSEI.m_callerOwnsSEIs                = true;
  Int picSptDpbOutputDuDelay = 0;
  UInt *accumBitsDU = NULL;
  UInt *accumNalsDU = NULL;
  SEIDecodingUnitInfo decodingUnitInfoSEI;
#if EFFICIENT_FIELD_IRAP
  Int IRAPGOPid = -1;
  Bool IRAPtoReorder = false;
  Bool swapIRAPForward = false;
  if(isField)
  {
    Int pocCurr;
#if !H_MV
    for ( Int iGOPid=0; iGOPid < m_iGopSize; iGOPid++ )
#endif
    {
      // determine actual POC
      if(iPOCLast == 0) //case first frame or first top field
      {
        pocCurr=0;
      }
      else if(iPOCLast == 1 && isField) //case first bottom field, just like the first frame, the poc computation is not right anymore, we set the right value
      {
        pocCurr = 1;
      }
      else
      {
        pocCurr = iPOCLast - iNumPicRcvd + m_pcCfg->getGOPEntry(iGOPid).m_POC - isField;
      }

      // check if POC corresponds to IRAP
      NalUnitType tmpUnitType = getNalUnitType(pocCurr, m_iLastIDR, isField);
      if(tmpUnitType >= NAL_UNIT_CODED_SLICE_BLA_W_LP && tmpUnitType <= NAL_UNIT_CODED_SLICE_CRA) // if picture is an IRAP
      {
        if(pocCurr%2 == 0 && iGOPid < m_iGopSize-1 && m_pcCfg->getGOPEntry(iGOPid).m_POC == m_pcCfg->getGOPEntry(iGOPid+1).m_POC-1)
        { // if top field and following picture in enc order is associated bottom field
          IRAPGOPid = iGOPid;
          IRAPtoReorder = true;
          swapIRAPForward = true; 
          break;
        }
        if(pocCurr%2 != 0 && iGOPid > 0 && m_pcCfg->getGOPEntry(iGOPid).m_POC == m_pcCfg->getGOPEntry(iGOPid-1).m_POC+1)
        {
          // if picture is an IRAP remember to process it first
          IRAPGOPid = iGOPid;
          IRAPtoReorder = true;
          swapIRAPForward = false; 
          break;
        }
      }
    }
  }
#endif
#if !H_MV
  for ( Int iGOPid=0; iGOPid < m_iGopSize; iGOPid++ )
#endif
  {
#if EFFICIENT_FIELD_IRAP
    if(IRAPtoReorder)
    {
      if(swapIRAPForward)
      {
        if(iGOPid == IRAPGOPid)
        {
          iGOPid = IRAPGOPid +1;
        }
        else if(iGOPid == IRAPGOPid +1)
        {
          iGOPid = IRAPGOPid;
        }
      }
      else
      {
        if(iGOPid == IRAPGOPid -1)
        {
          iGOPid = IRAPGOPid;
        }
        else if(iGOPid == IRAPGOPid)
        {
          iGOPid = IRAPGOPid -1;
        }
      }
    }
#endif
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
    Int iTimeOffset;
    Int pocCurr;
    
    if(iPOCLast == 0) //case first frame or first top field
    {
      pocCurr=0;
      iTimeOffset = 1;
    }
    else if(iPOCLast == 1 && isField) //case first bottom field, just like the first frame, the poc computation is not right anymore, we set the right value
    {
      pocCurr = 1;
      iTimeOffset = 1;
    }
    else
    {
      pocCurr = iPOCLast - iNumPicRcvd + m_pcCfg->getGOPEntry(iGOPid).m_POC - isField;
      iTimeOffset = m_pcCfg->getGOPEntry(iGOPid).m_POC;
    }
    if(pocCurr>=m_pcCfg->getFramesToBeEncoded())
    {
#if EFFICIENT_FIELD_IRAP
      if(IRAPtoReorder)
      {
        if(swapIRAPForward)
        {
          if(iGOPid == IRAPGOPid)
          {
            iGOPid = IRAPGOPid +1;
            IRAPtoReorder = false;
          }
          else if(iGOPid == IRAPGOPid +1)
          {
            iGOPid --;
          }
        }
        else
        {
          if(iGOPid == IRAPGOPid)
          {
            iGOPid = IRAPGOPid -1;
          }
          else if(iGOPid == IRAPGOPid -1)
          {
            iGOPid = IRAPGOPid;
            IRAPtoReorder = false;
          }
        }
      }
#endif
#if H_MV
      delete pcBitstreamRedirect;
      return;
#else
      continue;
#endif
    }

    if( getNalUnitType(pocCurr, m_iLastIDR, isField) == NAL_UNIT_CODED_SLICE_IDR_W_RADL || getNalUnitType(pocCurr, m_iLastIDR, isField) == NAL_UNIT_CODED_SLICE_IDR_N_LP )
    {
      m_iLastIDR = pocCurr;
    }        
    // start a new access unit: create an entry in the list of output access units
    accessUnitsInGOP.push_back(AccessUnit());
    AccessUnit& accessUnit = accessUnitsInGOP.back();
    xGetBuffer( rcListPic, rcListPicYuvRecOut, iNumPicRcvd, iTimeOffset, pcPic, pcPicYuvRecOut, pocCurr, isField);

    //  Slice data initialization
    pcPic->clearSliceBuffer();
    assert(pcPic->getNumAllocatedSlice() == 1);
    m_pcSliceEncoder->setSliceIdx(0);
    pcPic->setCurrSliceIdx(0);


#if H_MV
    m_pcSliceEncoder->initEncSlice ( pcPic, iPOCLast, pocCurr, iNumPicRcvd, iGOPid, pcSlice, m_pcEncTop->getVPS(), m_pcEncTop->getSPS(), m_pcEncTop->getPPS(), getLayerId(), isField  );     
#else
    m_pcSliceEncoder->initEncSlice ( pcPic, iPOCLast, pocCurr, iNumPicRcvd, iGOPid, pcSlice, m_pcEncTop->getSPS(), m_pcEncTop->getPPS(), isField  );
#endif
    
    //Set Frame/Field coding
    pcSlice->getPic()->setField(isField);

    pcSlice->setLastIDR(m_iLastIDR);
    pcSlice->setSliceIdx(0);
#if H_MV
    pcSlice->setRefPicSetInterLayer ( &m_refPicSetInterLayer0, &m_refPicSetInterLayer1 ); 
    pcPic  ->setLayerId     ( getLayerId()   );
    pcPic  ->setViewId      ( getViewId()    );    
#if !H_3D
    pcSlice->setLayerId     ( getLayerId() );
    pcSlice->setViewId      ( getViewId()  );    
    pcSlice->setVPS         ( m_pcEncTop->getVPS() );
#else
    pcPic  ->setViewIndex   ( getViewIndex() ); 
    pcPic  ->setIsDepth( getIsDepth() );
    pcSlice->setCamparaSlice( pcPic->getCodedScale(), pcPic->getCodedOffset() );    
#endif
#endif 
    //set default slice level flag to the same as SPS level flag
    pcSlice->setLFCrossSliceBoundaryFlag(  pcSlice->getPPS()->getLoopFilterAcrossSlicesEnabledFlag()  );
    pcSlice->setScalingList ( m_pcEncTop->getScalingList()  );
    if(m_pcEncTop->getUseScalingListId() == SCALING_LIST_OFF)
    {
      m_pcEncTop->getTrQuant()->setFlatScalingList();
      m_pcEncTop->getTrQuant()->setUseScalingList(false);
      m_pcEncTop->getSPS()->setScalingListPresentFlag(false);
      m_pcEncTop->getPPS()->setScalingListPresentFlag(false);
    }
    else if(m_pcEncTop->getUseScalingListId() == SCALING_LIST_DEFAULT)
    {
      pcSlice->setDefaultScalingList ();
      m_pcEncTop->getSPS()->setScalingListPresentFlag(false);
      m_pcEncTop->getPPS()->setScalingListPresentFlag(false);
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
      m_pcEncTop->getSPS()->setScalingListPresentFlag(pcSlice->checkDefaultScalingList());
      m_pcEncTop->getPPS()->setScalingListPresentFlag(false);
      m_pcEncTop->getTrQuant()->setScalingList(pcSlice->getScalingList());
      m_pcEncTop->getTrQuant()->setUseScalingList(true);
    }
    else
    {
      printf("error : ScalingList == %d no support\n",m_pcEncTop->getUseScalingListId());
      assert(0);
    }

#if H_MV
    // Set the nal unit type
    pcSlice->setNalUnitType(getNalUnitType(pocCurr, m_iLastIDR, isField));
    if( pcSlice->getSliceType() == B_SLICE )
    {
      if( m_pcCfg->getGOPEntry( ( pcSlice->getRapPicFlag() && getLayerId() > 0 ) ? MAX_GOP : iGOPid ).m_sliceType == 'P' ) 
      { 
        pcSlice->setSliceType( P_SLICE );
      }
    }

// To be checked!
    if( pcSlice->getSliceType() == B_SLICE )
    {
      if( m_pcCfg->getGOPEntry( ( pcSlice->getRapPicFlag() && getLayerId() > 0 ) ? MAX_GOP : iGOPid ).m_sliceType == 'I' ) 
      { 
        pcSlice->setSliceType( I_SLICE );
      }
    }
#else
    if(pcSlice->getSliceType()==B_SLICE&&m_pcCfg->getGOPEntry(iGOPid).m_sliceType=='P')
    {
      pcSlice->setSliceType(P_SLICE);
    }
    if(pcSlice->getSliceType()==B_SLICE&&m_pcCfg->getGOPEntry(iGOPid).m_sliceType=='I')
    {
      pcSlice->setSliceType(I_SLICE);
    }
    
    // Set the nal unit type
    pcSlice->setNalUnitType(getNalUnitType(pocCurr, m_iLastIDR, isField));
#endif
    if(pcSlice->getTemporalLayerNonReferenceFlag())
    {
      if (pcSlice->getNalUnitType() == NAL_UNIT_CODED_SLICE_TRAIL_R &&
          !(m_iGopSize == 1 && pcSlice->getSliceType() == I_SLICE))
        // Add this condition to avoid POC issues with encoder_intra_main.cfg configuration (see #1127 in bug tracker)
      {
        pcSlice->setNalUnitType(NAL_UNIT_CODED_SLICE_TRAIL_N);
      }
      if(pcSlice->getNalUnitType()==NAL_UNIT_CODED_SLICE_RADL_R)
      {
        pcSlice->setNalUnitType(NAL_UNIT_CODED_SLICE_RADL_N);
      }
      if(pcSlice->getNalUnitType()==NAL_UNIT_CODED_SLICE_RASL_R)
      {
        pcSlice->setNalUnitType(NAL_UNIT_CODED_SLICE_RASL_N);
      }
    }

#if EFFICIENT_FIELD_IRAP
#if FIX1172
    if ( pcSlice->getNalUnitType() == NAL_UNIT_CODED_SLICE_BLA_W_LP
      || pcSlice->getNalUnitType() == NAL_UNIT_CODED_SLICE_BLA_W_RADL
      || pcSlice->getNalUnitType() == NAL_UNIT_CODED_SLICE_BLA_N_LP
      || pcSlice->getNalUnitType() == NAL_UNIT_CODED_SLICE_IDR_W_RADL
      || pcSlice->getNalUnitType() == NAL_UNIT_CODED_SLICE_IDR_N_LP
      || pcSlice->getNalUnitType() == NAL_UNIT_CODED_SLICE_CRA )  // IRAP picture
    {
      m_associatedIRAPType = pcSlice->getNalUnitType();
      m_associatedIRAPPOC = pocCurr;
    }
    pcSlice->setAssociatedIRAPType(m_associatedIRAPType);
    pcSlice->setAssociatedIRAPPOC(m_associatedIRAPPOC);
#endif
#endif
    // Do decoding refresh marking if any 
    pcSlice->decodingRefreshMarking(m_pocCRA, m_bRefreshPending, rcListPic);
    m_pcEncTop->selectReferencePictureSet(pcSlice, pocCurr, iGOPid);
    pcSlice->getRPS()->setNumberOfLongtermPictures(0);
#if EFFICIENT_FIELD_IRAP
#else
#if FIX1172
    if ( pcSlice->getNalUnitType() == NAL_UNIT_CODED_SLICE_BLA_W_LP
      || pcSlice->getNalUnitType() == NAL_UNIT_CODED_SLICE_BLA_W_RADL
      || pcSlice->getNalUnitType() == NAL_UNIT_CODED_SLICE_BLA_N_LP
      || pcSlice->getNalUnitType() == NAL_UNIT_CODED_SLICE_IDR_W_RADL
      || pcSlice->getNalUnitType() == NAL_UNIT_CODED_SLICE_IDR_N_LP
      || pcSlice->getNalUnitType() == NAL_UNIT_CODED_SLICE_CRA )  // IRAP picture
    {
      m_associatedIRAPType = pcSlice->getNalUnitType();
      m_associatedIRAPPOC = pocCurr;
    }
    pcSlice->setAssociatedIRAPType(m_associatedIRAPType);
    pcSlice->setAssociatedIRAPPOC(m_associatedIRAPPOC);
#endif
#endif

#if ALLOW_RECOVERY_POINT_AS_RAP
    if ((pcSlice->checkThatAllRefPicsAreAvailable(rcListPic, pcSlice->getRPS(), false, m_iLastRecoveryPicPOC, m_pcCfg->getDecodingRefreshType() == 3) != 0) || (pcSlice->isIRAP()) 
#if EFFICIENT_FIELD_IRAP
      || (isField && pcSlice->getAssociatedIRAPType() >= NAL_UNIT_CODED_SLICE_BLA_W_LP && pcSlice->getAssociatedIRAPType() <= NAL_UNIT_CODED_SLICE_CRA && pcSlice->getAssociatedIRAPPOC() == pcSlice->getPOC()+1)
#endif
      )
    {
      pcSlice->createExplicitReferencePictureSetFromReference(rcListPic, pcSlice->getRPS(), pcSlice->isIRAP(), m_iLastRecoveryPicPOC, m_pcCfg->getDecodingRefreshType() == 3);
    }
#else
    if ((pcSlice->checkThatAllRefPicsAreAvailable(rcListPic, pcSlice->getRPS(), false) != 0) || (pcSlice->isIRAP()))
    {
      pcSlice->createExplicitReferencePictureSetFromReference(rcListPic, pcSlice->getRPS(), pcSlice->isIRAP());
    }
#endif
    pcSlice->applyReferencePictureSet(rcListPic, pcSlice->getRPS());

    if(pcSlice->getTLayer() > 0 
      &&  !( pcSlice->getNalUnitType() == NAL_UNIT_CODED_SLICE_RADL_N     // Check if not a leading picture
          || pcSlice->getNalUnitType() == NAL_UNIT_CODED_SLICE_RADL_R
          || pcSlice->getNalUnitType() == NAL_UNIT_CODED_SLICE_RASL_N
          || pcSlice->getNalUnitType() == NAL_UNIT_CODED_SLICE_RASL_R )
        )
    {
      if(pcSlice->isTemporalLayerSwitchingPoint(rcListPic) || pcSlice->getSPS()->getTemporalIdNestingFlag())
      {
        if(pcSlice->getTemporalLayerNonReferenceFlag())
        {
          pcSlice->setNalUnitType(NAL_UNIT_CODED_SLICE_TSA_N);
        }
        else
        {
          pcSlice->setNalUnitType(NAL_UNIT_CODED_SLICE_TSA_R);
        }
      }
      else if(pcSlice->isStepwiseTemporalLayerSwitchingPointCandidate(rcListPic))
      {
        Bool isSTSA=true;
        for(Int ii=iGOPid+1;(ii<m_pcCfg->getGOPSize() && isSTSA==true);ii++)
        {
          Int lTid= m_pcCfg->getGOPEntry(ii).m_temporalId;
          if(lTid==pcSlice->getTLayer()) 
          {
            TComReferencePictureSet* nRPS = pcSlice->getSPS()->getRPSList()->getReferencePictureSet(ii);
            for(Int jj=0;jj<nRPS->getNumberOfPictures();jj++)
            {
              if(nRPS->getUsed(jj)) 
              {
                Int tPoc=m_pcCfg->getGOPEntry(ii).m_POC+nRPS->getDeltaPOC(jj);
                Int kk=0;
                for(kk=0;kk<m_pcCfg->getGOPSize();kk++)
                {
                  if(m_pcCfg->getGOPEntry(kk).m_POC==tPoc)
                    break;
                }
                Int tTid=m_pcCfg->getGOPEntry(kk).m_temporalId;
                if(tTid >= pcSlice->getTLayer())
                {
                  isSTSA=false;
                  break;
                }
              }
            }
          }
        }
        if(isSTSA==true)
        {    
          if(pcSlice->getTemporalLayerNonReferenceFlag())
          {
            pcSlice->setNalUnitType(NAL_UNIT_CODED_SLICE_STSA_N);
          }
          else
          {
            pcSlice->setNalUnitType(NAL_UNIT_CODED_SLICE_STSA_R);
          }
        }
      }
    }
    arrangeLongtermPicturesInRPS(pcSlice, rcListPic);
    TComRefPicListModification* refPicListModification = pcSlice->getRefPicListModification();
    refPicListModification->setRefPicListModificationFlagL0(0);
    refPicListModification->setRefPicListModificationFlagL1(0);
#if H_MV
    if ( pcSlice->getPPS()->getNumExtraSliceHeaderBits() > 0 )
    {
      // Some more sophisticated algorithm to determine discardable_flag might be added here. 
      pcSlice->setDiscardableFlag           ( false );     
    }    

    TComVPS*           vps = pcSlice->getVPS();     
#if HHI_DEPENDENCY_SIGNALLING_I1_J0107
#if H_3D
    Int numDirectRefLayers = vps    ->getNumRefListLayers( getLayerId() ); 
#else
    Int numDirectRefLayers = vps    ->getNumDirectRefLayers( getLayerId() ); 
#endif
#else
    Int numDirectRefLayers = vps    ->getNumDirectRefLayers( getLayerId() ); 
#endif
    GOPEntry gopEntry      = m_pcCfg->getGOPEntry( (pcSlice->getRapPicFlag() && getLayerId() > 0) ? MAX_GOP : iGOPid );     
    
    Bool interLayerPredLayerIdcPresentFlag = false; 
    if ( getLayerId() > 0 && !vps->getAllRefLayersActiveFlag() && numDirectRefLayers > 0 )
    {         
      pcSlice->setInterLayerPredEnabledFlag ( gopEntry.m_numActiveRefLayerPics > 0 );     
      if ( pcSlice->getInterLayerPredEnabledFlag() && numDirectRefLayers > 1 )
      {
        if ( !vps->getMaxOneActiveRefLayerFlag() )
        {    
          pcSlice->setNumInterLayerRefPicsMinus1( gopEntry.m_numActiveRefLayerPics - 1 ); 
        }
#if HHI_DEPENDENCY_SIGNALLING_I1_J0107
#if H_3D
        if ( gopEntry.m_numActiveRefLayerPics != vps->getNumRefListLayers( getLayerId() ) )
#else
        if ( gopEntry.m_numActiveRefLayerPics != vps->getNumDirectRefLayers( getLayerId() ) )
#endif
#else
        if ( gopEntry.m_numActiveRefLayerPics != vps->getNumDirectRefLayers( getLayerId() ) )
#endif
        {        
          interLayerPredLayerIdcPresentFlag = true; 
          for (Int i = 0; i < gopEntry.m_numActiveRefLayerPics; i++ )
          {
            pcSlice->setInterLayerPredLayerIdc( i, gopEntry.m_interLayerPredLayerIdc[ i ] ); 
          }
        }
      }
    }
    if ( !interLayerPredLayerIdcPresentFlag )
    {
      for( Int i = 0; i < pcSlice->getNumActiveRefLayerPics(); i++ )   
      {
        pcSlice->setInterLayerPredLayerIdc(i, pcSlice->getRefLayerPicIdc( i ) );
      }
    }


    assert( pcSlice->getNumActiveRefLayerPics() == gopEntry.m_numActiveRefLayerPics ); 
    
    pcSlice->createInterLayerReferencePictureSet( m_ivPicLists, m_refPicSetInterLayer0, m_refPicSetInterLayer1 ); 
    pcSlice->setNumRefIdx(REF_PIC_LIST_0,min(gopEntry.m_numRefPicsActive,( pcSlice->getRPS()->getNumberOfPictures() + (Int) m_refPicSetInterLayer0.size() + (Int) m_refPicSetInterLayer1.size()) ) );
    pcSlice->setNumRefIdx(REF_PIC_LIST_1,min(gopEntry.m_numRefPicsActive,( pcSlice->getRPS()->getNumberOfPictures() + (Int) m_refPicSetInterLayer0.size() + (Int) m_refPicSetInterLayer1.size()) ) );

    std::vector< TComPic* >    tempRefPicLists[2];
    std::vector< Bool     >    usedAsLongTerm [2];
    Int       numPocTotalCurr;

    pcSlice->getTempRefPicLists( rcListPic, m_refPicSetInterLayer0, m_refPicSetInterLayer1, tempRefPicLists, usedAsLongTerm, numPocTotalCurr, true );
    

    xSetRefPicListModificationsMv( tempRefPicLists, pcSlice, iGOPid );    
#else
    pcSlice->setNumRefIdx(REF_PIC_LIST_0,min(m_pcCfg->getGOPEntry(iGOPid).m_numRefPicsActive,pcSlice->getRPS()->getNumberOfPictures()));
    pcSlice->setNumRefIdx(REF_PIC_LIST_1,min(m_pcCfg->getGOPEntry(iGOPid).m_numRefPicsActive,pcSlice->getRPS()->getNumberOfPictures()));
#endif

#if ADAPTIVE_QP_SELECTION
    pcSlice->setTrQuant( m_pcEncTop->getTrQuant() );
#endif      

    //  Set reference list
#if H_MV    
    pcSlice->setRefPicList( tempRefPicLists, usedAsLongTerm, numPocTotalCurr ); 
#else
    pcSlice->setRefPicList ( rcListPic );
#endif
#if !MTK_SINGLE_DEPTH_VPS_FLAG_J0060
#if H_3D_SINGLE_DEPTH
#if HHI_TOOL_PARAMETERS_I2_J0107
    pcSlice->setApplySingleDepthMode( pcSlice->getIntraSingleFlag() );
#else
    TEncTop* pcEncTop = (TEncTop*) m_pcCfg;
    bool enableSingleDepthMode=false;
    if(pcEncTop->getUseSingleDepthMode())
    {
      if(pcSlice->getIsDepth())
      {
        enableSingleDepthMode=true;
      }
    }
    pcSlice->setApplySingleDepthMode(enableSingleDepthMode);
#endif
#endif   
#endif 
#if SEC_ARP_VIEW_REF_CHECK_J0037 || SEC_DBBP_VIEW_REF_CHECK_J0037
    pcSlice->setDefaultRefView();
#endif
#if H_3D_ARP
    //GT: This seems to be broken when layerId in vps is not equal to layerId in nuh
    pcSlice->setARPStepNum(m_ivPicLists);
    if(pcSlice->getARPStepNum() > 1)
    {
      for(Int iLayerId = 0; iLayerId < getLayerId(); iLayerId ++ )
      {
        Int  iViewIdx =   pcSlice->getVPS()->getViewIndex(iLayerId);
        Bool bIsDepth = ( pcSlice->getVPS()->getDepthId  ( iLayerId ) == 1 );
        if( iViewIdx<getViewIndex() && !bIsDepth )
        {
          pcSlice->setBaseViewRefPicList( m_ivPicLists->getPicList( iLayerId ), iViewIdx );
        }
      }
    }
#endif
#if H_3D
    pcSlice->setIvPicLists( m_ivPicLists );         
#if H_3D_IV_MERGE    
    assert( !m_pcEncTop->getIsDepth() || ( pcSlice->getTexturePic() != 0 ) );
#endif    
#endif
#if H_3D_IC
    pcSlice->setICEnableCandidate( m_aICEnableCandidate );         
    pcSlice->setICEnableNum( m_aICEnableNum );         
#endif
    //  Slice info. refinement
#if H_MV
    if ( pcSlice->getSliceType() == B_SLICE )
    {
      if( m_pcCfg->getGOPEntry( ( pcSlice->getRapPicFlag() == true && getLayerId() > 0 ) ? MAX_GOP : iGOPid ).m_sliceType == 'P' ) 
      { 
        pcSlice->setSliceType( P_SLICE ); 
      }
    }
#else
    if ( (pcSlice->getSliceType() == B_SLICE) && (pcSlice->getNumRefIdx(REF_PIC_LIST_1) == 0) )
    {
      pcSlice->setSliceType ( P_SLICE );
    }
#endif
    if (pcSlice->getSliceType() == B_SLICE)
    {
      pcSlice->setColFromL0Flag(1-uiColDir);
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
    else
    {
      pcSlice->setCheckLDC(true);  
    }

    uiColDir = 1-uiColDir;

    //-------------------------------------------------------------
    pcSlice->setRefPOCList();

    pcSlice->setList1IdxToList0Idx();
#if H_3D_TMVP
    if(pcSlice->getLayerId())
      pcSlice->generateAlterRefforTMVP();
#endif
    if (m_pcEncTop->getTMVPModeId() == 2)
    {
      if (iGOPid == 0) // first picture in SOP (i.e. forward B)
      {
        pcSlice->setEnableTMVPFlag(0);
      }
      else
      {
        // Note: pcSlice->getColFromL0Flag() is assumed to be always 0 and getcolRefIdx() is always 0.
        pcSlice->setEnableTMVPFlag(1);
      }
      pcSlice->getSPS()->setTMVPFlagsPresent(1);
    }
    else if (m_pcEncTop->getTMVPModeId() == 1)
    {
      pcSlice->getSPS()->setTMVPFlagsPresent(1);
      pcSlice->setEnableTMVPFlag(1);
    }
    else
    {
      pcSlice->getSPS()->setTMVPFlagsPresent(0);
      pcSlice->setEnableTMVPFlag(0);
    }
#if H_MV
    if( pcSlice->getIdrPicFlag() )
    {
      pcSlice->setEnableTMVPFlag(0);
    }
#endif

#if H_3D_VSO
  // Should be moved to TEncTop !!! 
  Bool bUseVSO = m_pcEncTop->getUseVSO();
  
  TComRdCost* pcRdCost = m_pcEncTop->getRdCost();   

  pcRdCost->setUseVSO( bUseVSO );

  // SAIT_VSO_EST_A0033
  pcRdCost->setUseEstimatedVSD( m_pcEncTop->getUseEstimatedVSD() );

  if ( bUseVSO )
  {
    Int iVSOMode = m_pcEncTop->getVSOMode();
    pcRdCost->setVSOMode( iVSOMode  );
    pcRdCost->setAllowNegDist( m_pcEncTop->getAllowNegDist() );

    // SAIT_VSO_EST_A0033
#if H_3D_FCO
    Bool flagRec;
    flagRec =  ((m_pcEncTop->getIvPicLists()->getPicYuv( pcSlice->getViewIndex(), false, pcSlice->getPOC(), true) == NULL) ? false: true);
    pcRdCost->setVideoRecPicYuv( m_pcEncTop->getIvPicLists()->getPicYuv( pcSlice->getViewIndex(), false, pcSlice->getPOC(), flagRec ) );
    pcRdCost->setDepthPicYuv   ( m_pcEncTop->getIvPicLists()->getPicYuv( pcSlice->getViewIndex(), true, pcSlice->getPOC(), false ) );
#else
    pcRdCost->setVideoRecPicYuv( m_pcEncTop->getIvPicLists()->getPicYuv( pcSlice->getViewIndex(), false , pcSlice->getPOC(), true ) );
    pcRdCost->setDepthPicYuv   ( m_pcEncTop->getIvPicLists()->getPicYuv( pcSlice->getViewIndex(), true  , pcSlice->getPOC(), false ) );
#endif

    // LGE_WVSO_A0119
    Bool bUseWVSO  = m_pcEncTop->getUseWVSO();
    pcRdCost->setUseWVSO( bUseWVSO );

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
        Int i;
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

    Double lambda            = 0.0;
    Int actualHeadBits       = 0;
    Int actualTotalBits      = 0;
    Int estimatedBits        = 0;
    Int tmpBitsBeforeWriting = 0;
    if ( m_pcCfg->getUseRateCtrl() )
    {
      Int frameLevel = m_pcRateCtrl->getRCSeq()->getGOPID2Level( iGOPid );
      if ( pcPic->getSlice(0)->getSliceType() == I_SLICE )
      {
        frameLevel = 0;
      }
      m_pcRateCtrl->initRCPic( frameLevel );

#if KWU_RC_MADPRED_E0227
      if(m_pcCfg->getLayerId() != 0)
      {
        m_pcRateCtrl->getRCPic()->setIVPic( m_pcEncTop->getEncTop()->getTEncTop(0)->getRateCtrl()->getRCPic() );
      }
#endif

      estimatedBits = m_pcRateCtrl->getRCPic()->getTargetBits();

      Int sliceQP = m_pcCfg->getInitialQP();
      if ( ( pcSlice->getPOC() == 0 && m_pcCfg->getInitialQP() > 0 ) || ( frameLevel == 0 && m_pcCfg->getForceIntraQP() ) ) // QP is specified
      {
        Int    NumberBFrames = ( m_pcCfg->getGOPSize() - 1 );
        Double dLambda_scale = 1.0 - Clip3( 0.0, 0.5, 0.05*(Double)NumberBFrames );
        Double dQPFactor     = 0.57*dLambda_scale;
        Int    SHIFT_QP      = 12;
        Int    bitdepth_luma_qp_scale = 0;
        Double qp_temp = (Double) sliceQP + bitdepth_luma_qp_scale - SHIFT_QP;
        lambda = dQPFactor*pow( 2.0, qp_temp/3.0 );
      }
      else if ( frameLevel == 0 )   // intra case, but use the model
      {
        m_pcSliceEncoder->calCostSliceI(pcPic);
        if ( m_pcCfg->getIntraPeriod() != 1 )   // do not refine allocated bits for all intra case
        {
          Int bits = m_pcRateCtrl->getRCSeq()->getLeftAverageBits();
          bits = m_pcRateCtrl->getRCPic()->getRefineBitsForIntra( bits );
          if ( bits < 200 )
          {
            bits = 200;
          }
          m_pcRateCtrl->getRCPic()->setTargetBits( bits );
        }

        list<TEncRCPic*> listPreviousPicture = m_pcRateCtrl->getPicList();
        m_pcRateCtrl->getRCPic()->getLCUInitTargetBits();
        lambda  = m_pcRateCtrl->getRCPic()->estimatePicLambda( listPreviousPicture, pcSlice->getSliceType());
        sliceQP = m_pcRateCtrl->getRCPic()->estimatePicQP( lambda, listPreviousPicture );
      }
      else    // normal case
      {
#if KWU_RC_MADPRED_E0227
        if(m_pcRateCtrl->getLayerID() != 0)
        {
          list<TEncRCPic*> listPreviousPicture = m_pcRateCtrl->getPicList();
          lambda  = m_pcRateCtrl->getRCPic()->estimatePicLambdaIV( listPreviousPicture, pcSlice->getPOC() );
          sliceQP = m_pcRateCtrl->getRCPic()->estimatePicQP( lambda, listPreviousPicture );
        }
        else
        {
#endif
        list<TEncRCPic*> listPreviousPicture = m_pcRateCtrl->getPicList();
        lambda  = m_pcRateCtrl->getRCPic()->estimatePicLambda( listPreviousPicture, pcSlice->getSliceType());
        sliceQP = m_pcRateCtrl->getRCPic()->estimatePicQP( lambda, listPreviousPicture );
#if KWU_RC_MADPRED_E0227
        }
#endif
      }

      sliceQP = Clip3( -pcSlice->getSPS()->getQpBDOffsetY(), MAX_QP, sliceQP );
      m_pcRateCtrl->getRCPic()->setPicEstQP( sliceQP );

      m_pcSliceEncoder->resetQP( pcPic, sliceQP, lambda );
    }

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

    Int  p, j;
    UInt uiEncCUAddr;

    pcPic->getPicSym()->initTiles(pcSlice->getPPS());

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

    // Allocate some coders, now we know how many tiles there are.
    m_pcEncTop->createWPPCoders(iNumSubstreams);
    pcSbacCoders = m_pcEncTop->getSbacCoders();
    pcSubstreamsOut = new TComOutputBitstream[iNumSubstreams];

    UInt startCUAddrSliceIdx = 0; // used to index "m_uiStoredStartCUAddrForEncodingSlice" containing locations of slice boundaries
    UInt startCUAddrSlice    = 0; // used to keep track of current slice's starting CU addr.
    pcSlice->setSliceCurStartCUAddr( startCUAddrSlice ); // Setting "start CU addr" for current slice
    m_storedStartCUAddrForEncodingSlice.clear();

    UInt startCUAddrSliceSegmentIdx = 0; // used to index "m_uiStoredStartCUAddrForEntropyEncodingSlice" containing locations of slice boundaries
    UInt startCUAddrSliceSegment    = 0; // used to keep track of current Dependent slice's starting CU addr.
    pcSlice->setSliceSegmentCurStartCUAddr( startCUAddrSliceSegment ); // Setting "start CU addr" for current Dependent slice

    m_storedStartCUAddrForEncodingSliceSegment.clear();
    UInt nextCUAddr = 0;
    m_storedStartCUAddrForEncodingSlice.push_back (nextCUAddr);
    startCUAddrSliceIdx++;
    m_storedStartCUAddrForEncodingSliceSegment.push_back(nextCUAddr);
    startCUAddrSliceSegmentIdx++;
#if H_3D_NBDV
      if(pcSlice->getViewIndex() && !pcSlice->getIsDepth()) //Notes from QC: this condition shall be changed once the configuration is completed, e.g. in pcSlice->getSPS()->getMultiviewMvPredMode() || ARP in prev. HTM. Remove this comment once it is done.
      {
        Int iColPoc = pcSlice->getRefPOC(RefPicList(1-pcSlice->getColFromL0Flag()), pcSlice->getColRefIdx());
        pcPic->setNumDdvCandPics(pcPic->getDisCandRefPictures(iColPoc));
      }
#endif
#if H_3D
      pcSlice->setDepthToDisparityLUTs(); 

#endif

#if H_3D_NBDV
      if(pcSlice->getViewIndex() && !pcSlice->getIsDepth() && !pcSlice->isIntra()) //Notes from QC: this condition shall be changed once the configuration is completed, e.g. in pcSlice->getSPS()->getMultiviewMvPredMode() || ARP in prev. HTM. Remove this comment once it is done.
      {
        pcPic->checkTemporalIVRef();
      }

      if(pcSlice->getIsDepth())
      {
        pcPic->checkTextureRef();
      }
#endif
    while(nextCUAddr<uiRealEndAddress) // determine slice boundaries
    {
      pcSlice->setNextSlice       ( false );
      pcSlice->setNextSliceSegment( false );
      assert(pcPic->getNumAllocatedSlice() == startCUAddrSliceIdx);
      m_pcSliceEncoder->precompressSlice( pcPic );
      m_pcSliceEncoder->compressSlice   ( pcPic );

      Bool bNoBinBitConstraintViolated = (!pcSlice->isNextSlice() && !pcSlice->isNextSliceSegment());
      if (pcSlice->isNextSlice() || (bNoBinBitConstraintViolated && m_pcCfg->getSliceMode()==FIXED_NUMBER_OF_LCU))
      {
        startCUAddrSlice = pcSlice->getSliceCurEndCUAddr();
        // Reconstruction slice
        m_storedStartCUAddrForEncodingSlice.push_back(startCUAddrSlice);
        startCUAddrSliceIdx++;
        // Dependent slice
        if (startCUAddrSliceSegmentIdx>0 && m_storedStartCUAddrForEncodingSliceSegment[startCUAddrSliceSegmentIdx-1] != startCUAddrSlice)
        {
          m_storedStartCUAddrForEncodingSliceSegment.push_back(startCUAddrSlice);
          startCUAddrSliceSegmentIdx++;
        }

        if (startCUAddrSlice < uiRealEndAddress)
        {
          pcPic->allocateNewSlice();          
          pcPic->setCurrSliceIdx                  ( startCUAddrSliceIdx-1 );
          m_pcSliceEncoder->setSliceIdx           ( startCUAddrSliceIdx-1 );
          pcSlice = pcPic->getSlice               ( startCUAddrSliceIdx-1 );
          pcSlice->copySliceInfo                  ( pcPic->getSlice(0)      );
          pcSlice->setSliceIdx                    ( startCUAddrSliceIdx-1 );
          pcSlice->setSliceCurStartCUAddr         ( startCUAddrSlice      );
          pcSlice->setSliceSegmentCurStartCUAddr  ( startCUAddrSlice      );
          pcSlice->setSliceBits(0);
          uiNumSlices ++;
        }
      }
      else if (pcSlice->isNextSliceSegment() || (bNoBinBitConstraintViolated && m_pcCfg->getSliceSegmentMode()==FIXED_NUMBER_OF_LCU))
      {
        startCUAddrSliceSegment                                                     = pcSlice->getSliceSegmentCurEndCUAddr();
        m_storedStartCUAddrForEncodingSliceSegment.push_back(startCUAddrSliceSegment);
        startCUAddrSliceSegmentIdx++;
        pcSlice->setSliceSegmentCurStartCUAddr( startCUAddrSliceSegment );
      }
      else
      {
        startCUAddrSlice                                                            = pcSlice->getSliceCurEndCUAddr();
        startCUAddrSliceSegment                                                     = pcSlice->getSliceSegmentCurEndCUAddr();
      }        

      nextCUAddr = (startCUAddrSlice > startCUAddrSliceSegment) ? startCUAddrSlice : startCUAddrSliceSegment;
    }
    m_storedStartCUAddrForEncodingSlice.push_back( pcSlice->getSliceCurEndCUAddr());
    startCUAddrSliceIdx++;
    m_storedStartCUAddrForEncodingSliceSegment.push_back(pcSlice->getSliceCurEndCUAddr());
    startCUAddrSliceSegmentIdx++;

    pcSlice = pcPic->getSlice(0);

    // SAO parameter estimation using non-deblocked pixels for LCU bottom and right boundary areas
    if( pcSlice->getSPS()->getUseSAO() && m_pcCfg->getSaoLcuBoundary() )
    {
      m_pcSAO->getPreDBFStatistics(pcPic);
    }

    //-- Loop filter
    Bool bLFCrossTileBoundary = pcSlice->getPPS()->getLoopFilterAcrossTilesEnabledFlag();
    m_pcLoopFilter->setCfg(bLFCrossTileBoundary);
    if ( m_pcCfg->getDeblockingFilterMetric() )
    {
      dblMetric(pcPic, uiNumSlices);
    }
    m_pcLoopFilter->loopFilterPic( pcPic );

    /////////////////////////////////////////////////////////////////////////////////////////////////// File writing
    // Set entropy coder
    m_pcEntropyCoder->setEntropyCoder   ( m_pcCavlcCoder, pcSlice );

    /* write various header sets. */
    if ( m_bSeqFirst )
    {
      OutputNALUnit nalu(NAL_UNIT_VPS);
#if H_MV
      if( getLayerId() == 0 )
      {
#endif
      m_pcEntropyCoder->setBitstream(&nalu.m_Bitstream);
      m_pcEntropyCoder->encodeVPS(m_pcEncTop->getVPS());
      writeRBSPTrailingBits(nalu.m_Bitstream);
      accessUnit.push_back(new NALUnitEBSP(nalu));
      actualTotalBits += UInt(accessUnit.back()->m_nalUnitData.str().size()) * 8;

#if H_MV
      }
      nalu = NALUnit(NAL_UNIT_SPS, 0, getLayerId());
#else
      nalu = NALUnit(NAL_UNIT_SPS);
#endif
      m_pcEntropyCoder->setBitstream(&nalu.m_Bitstream);
      if (m_bSeqFirst)
      {
        pcSlice->getSPS()->setNumLongTermRefPicSPS(m_numLongTermRefPicSPS);
        for (Int k = 0; k < m_numLongTermRefPicSPS; k++)
        {
          pcSlice->getSPS()->setLtRefPicPocLsbSps(k, m_ltRefPicPocLsbSps[k]);
          pcSlice->getSPS()->setUsedByCurrPicLtSPSFlag(k, m_ltRefPicUsedByCurrPicFlag[k]);
        }
      }
      if( m_pcCfg->getPictureTimingSEIEnabled() || m_pcCfg->getDecodingUnitInfoSEIEnabled() )
      {
        UInt maxCU = m_pcCfg->getSliceArgument() >> ( pcSlice->getSPS()->getMaxCUDepth() << 1);
        UInt numDU = ( m_pcCfg->getSliceMode() == 1 ) ? ( pcPic->getNumCUsInFrame() / maxCU ) : ( 0 );
        if( pcPic->getNumCUsInFrame() % maxCU != 0 || numDU == 0 )
        {
          numDU ++;
        }
        pcSlice->getSPS()->getVuiParameters()->getHrdParameters()->setNumDU( numDU );
        pcSlice->getSPS()->setHrdParameters( m_pcCfg->getFrameRate(), numDU, m_pcCfg->getTargetBitrate(), ( m_pcCfg->getIntraPeriod() > 0 ) );
      }
      if( m_pcCfg->getBufferingPeriodSEIEnabled() || m_pcCfg->getPictureTimingSEIEnabled() || m_pcCfg->getDecodingUnitInfoSEIEnabled() )
      {
        pcSlice->getSPS()->getVuiParameters()->setHrdParametersPresentFlag( true );
      }
#if HHI_TOOL_PARAMETERS_I2_J0107
      m_pcEntropyCoder->encodeSPS(pcSlice->getSPS());
#else
#if !H_3D
      m_pcEntropyCoder->encodeSPS(pcSlice->getSPS());
#else
      m_pcEntropyCoder->encodeSPS(pcSlice->getSPS(), pcSlice->getViewIndex(), pcSlice->getIsDepth() );
#endif
#endif
      writeRBSPTrailingBits(nalu.m_Bitstream);
      accessUnit.push_back(new NALUnitEBSP(nalu));
      actualTotalBits += UInt(accessUnit.back()->m_nalUnitData.str().size()) * 8;

#if H_MV
      nalu = NALUnit(NAL_UNIT_PPS, 0, getLayerId());
#else
      nalu = NALUnit(NAL_UNIT_PPS);
#endif
#if PPS_FIX_DEPTH
      if(!pcSlice->getIsDepth() || !pcSlice->getViewIndex() )
      {
#endif
      m_pcEntropyCoder->setBitstream(&nalu.m_Bitstream);
      m_pcEntropyCoder->encodePPS(pcSlice->getPPS());
      writeRBSPTrailingBits(nalu.m_Bitstream);
      accessUnit.push_back(new NALUnitEBSP(nalu));
      actualTotalBits += UInt(accessUnit.back()->m_nalUnitData.str().size()) * 8;
      
#if PPS_FIX_DEPTH
      }
#endif
      xCreateLeadingSEIMessages(accessUnit, pcSlice->getSPS());

      m_bSeqFirst = false;
    }

    if (writeSOP) // write SOP description SEI (if enabled) at the beginning of GOP
    {
      Int SOPcurrPOC = pocCurr;

      OutputNALUnit nalu(NAL_UNIT_PREFIX_SEI);
      m_pcEntropyCoder->setEntropyCoder(m_pcCavlcCoder, pcSlice);
      m_pcEntropyCoder->setBitstream(&nalu.m_Bitstream);

      SEISOPDescription SOPDescriptionSEI;
      SOPDescriptionSEI.m_sopSeqParameterSetId = pcSlice->getSPS()->getSPSId();

      UInt i = 0;
      UInt prevEntryId = iGOPid;
      for (j = iGOPid; j < m_iGopSize; j++)
      {
        Int deltaPOC = m_pcCfg->getGOPEntry(j).m_POC - m_pcCfg->getGOPEntry(prevEntryId).m_POC;
        if ((SOPcurrPOC + deltaPOC) < m_pcCfg->getFramesToBeEncoded())
        {
          SOPcurrPOC += deltaPOC;
          SOPDescriptionSEI.m_sopDescVclNaluType[i] = getNalUnitType(SOPcurrPOC, m_iLastIDR, isField);
          SOPDescriptionSEI.m_sopDescTemporalId[i] = m_pcCfg->getGOPEntry(j).m_temporalId;
          SOPDescriptionSEI.m_sopDescStRpsIdx[i] = m_pcEncTop->getReferencePictureSetIdxForSOP(pcSlice, SOPcurrPOC, j);
          SOPDescriptionSEI.m_sopDescPocDelta[i] = deltaPOC;

          prevEntryId = j;
          i++;
        }
      }

      SOPDescriptionSEI.m_numPicsInSopMinus1 = i - 1;

      m_seiWriter.writeSEImessage( nalu.m_Bitstream, SOPDescriptionSEI, pcSlice->getSPS());
      writeRBSPTrailingBits(nalu.m_Bitstream);
      accessUnit.push_back(new NALUnitEBSP(nalu));

      writeSOP = false;
    }

    if( ( m_pcCfg->getPictureTimingSEIEnabled() || m_pcCfg->getDecodingUnitInfoSEIEnabled() ) &&
        ( pcSlice->getSPS()->getVuiParametersPresentFlag() ) &&
        ( ( pcSlice->getSPS()->getVuiParameters()->getHrdParameters()->getNalHrdParametersPresentFlag() ) 
       || ( pcSlice->getSPS()->getVuiParameters()->getHrdParameters()->getVclHrdParametersPresentFlag() ) ) )
    {
      if( pcSlice->getSPS()->getVuiParameters()->getHrdParameters()->getSubPicCpbParamsPresentFlag() )
      {
        UInt numDU = pcSlice->getSPS()->getVuiParameters()->getHrdParameters()->getNumDU();
        pictureTimingSEI.m_numDecodingUnitsMinus1     = ( numDU - 1 );
        pictureTimingSEI.m_duCommonCpbRemovalDelayFlag = false;

        if( pictureTimingSEI.m_numNalusInDuMinus1 == NULL )
        {
          pictureTimingSEI.m_numNalusInDuMinus1       = new UInt[ numDU ];
        }
        if( pictureTimingSEI.m_duCpbRemovalDelayMinus1  == NULL )
        {
          pictureTimingSEI.m_duCpbRemovalDelayMinus1  = new UInt[ numDU ];
        }
        if( accumBitsDU == NULL )
        {
          accumBitsDU                                  = new UInt[ numDU ];
        }
        if( accumNalsDU == NULL )
        {
          accumNalsDU                                  = new UInt[ numDU ];
        }
      }
      pictureTimingSEI.m_auCpbRemovalDelay = std::min<Int>(std::max<Int>(1, m_totalCoded - m_lastBPSEI), static_cast<Int>(pow(2, static_cast<double>(pcSlice->getSPS()->getVuiParameters()->getHrdParameters()->getCpbRemovalDelayLengthMinus1()+1)))); // Syntax element signalled as minus, hence the .
      pictureTimingSEI.m_picDpbOutputDelay = pcSlice->getSPS()->getNumReorderPics(pcSlice->getSPS()->getMaxTLayers()-1) + pcSlice->getPOC() - m_totalCoded;
#if EFFICIENT_FIELD_IRAP
      if(IRAPGOPid > 0 && IRAPGOPid < m_iGopSize)
      {
        // if pictures have been swapped there is likely one more picture delay on their tid. Very rough approximation
        pictureTimingSEI.m_picDpbOutputDelay ++;
      }
#endif
      Int factor = pcSlice->getSPS()->getVuiParameters()->getHrdParameters()->getTickDivisorMinus2() + 2;
      pictureTimingSEI.m_picDpbOutputDuDelay = factor * pictureTimingSEI.m_picDpbOutputDelay;
      if( m_pcCfg->getDecodingUnitInfoSEIEnabled() )
      {
        picSptDpbOutputDuDelay = factor * pictureTimingSEI.m_picDpbOutputDelay;
      }
    }

    if( ( m_pcCfg->getBufferingPeriodSEIEnabled() ) && ( pcSlice->getSliceType() == I_SLICE ) &&
        ( pcSlice->getSPS()->getVuiParametersPresentFlag() ) && 
        ( ( pcSlice->getSPS()->getVuiParameters()->getHrdParameters()->getNalHrdParametersPresentFlag() ) 
       || ( pcSlice->getSPS()->getVuiParameters()->getHrdParameters()->getVclHrdParametersPresentFlag() ) ) )
    {
      OutputNALUnit nalu(NAL_UNIT_PREFIX_SEI);
      m_pcEntropyCoder->setEntropyCoder(m_pcCavlcCoder, pcSlice);
      m_pcEntropyCoder->setBitstream(&nalu.m_Bitstream);

      SEIBufferingPeriod sei_buffering_period;
      
      UInt uiInitialCpbRemovalDelay = (90000/2);                      // 0.5 sec
      sei_buffering_period.m_initialCpbRemovalDelay      [0][0]     = uiInitialCpbRemovalDelay;
      sei_buffering_period.m_initialCpbRemovalDelayOffset[0][0]     = uiInitialCpbRemovalDelay;
      sei_buffering_period.m_initialCpbRemovalDelay      [0][1]     = uiInitialCpbRemovalDelay;
      sei_buffering_period.m_initialCpbRemovalDelayOffset[0][1]     = uiInitialCpbRemovalDelay;

      Double dTmp = (Double)pcSlice->getSPS()->getVuiParameters()->getTimingInfo()->getNumUnitsInTick() / (Double)pcSlice->getSPS()->getVuiParameters()->getTimingInfo()->getTimeScale();

      UInt uiTmp = (UInt)( dTmp * 90000.0 ); 
      uiInitialCpbRemovalDelay -= uiTmp;
      uiInitialCpbRemovalDelay -= uiTmp / ( pcSlice->getSPS()->getVuiParameters()->getHrdParameters()->getTickDivisorMinus2() + 2 );
      sei_buffering_period.m_initialAltCpbRemovalDelay      [0][0]  = uiInitialCpbRemovalDelay;
      sei_buffering_period.m_initialAltCpbRemovalDelayOffset[0][0]  = uiInitialCpbRemovalDelay;
      sei_buffering_period.m_initialAltCpbRemovalDelay      [0][1]  = uiInitialCpbRemovalDelay;
      sei_buffering_period.m_initialAltCpbRemovalDelayOffset[0][1]  = uiInitialCpbRemovalDelay;

      sei_buffering_period.m_rapCpbParamsPresentFlag              = 0;
      //for the concatenation, it can be set to one during splicing.
      sei_buffering_period.m_concatenationFlag = 0;
      //since the temporal layer HRD is not ready, we assumed it is fixed
      sei_buffering_period.m_auCpbRemovalDelayDelta = 1;
      sei_buffering_period.m_cpbDelayOffset = 0;
      sei_buffering_period.m_dpbDelayOffset = 0;

      m_seiWriter.writeSEImessage( nalu.m_Bitstream, sei_buffering_period, pcSlice->getSPS());
      writeRBSPTrailingBits(nalu.m_Bitstream);
      {
      UInt seiPositionInAu = xGetFirstSeiLocation(accessUnit);
      UInt offsetPosition = m_activeParameterSetSEIPresentInAU;   // Insert BP SEI after APS SEI
      AccessUnit::iterator it;
      for(j = 0, it = accessUnit.begin(); j < seiPositionInAu + offsetPosition; j++)
      {
        it++;
      }
      accessUnit.insert(it, new NALUnitEBSP(nalu));
      m_bufferingPeriodSEIPresentInAU = true;
      }

      if (m_pcCfg->getScalableNestingSEIEnabled())
      {
        OutputNALUnit naluTmp(NAL_UNIT_PREFIX_SEI);
        m_pcEntropyCoder->setEntropyCoder(m_pcCavlcCoder, pcSlice);
        m_pcEntropyCoder->setBitstream(&naluTmp.m_Bitstream);
        scalableNestingSEI.m_nestedSEIs.clear();
        scalableNestingSEI.m_nestedSEIs.push_back(&sei_buffering_period);
        m_seiWriter.writeSEImessage( naluTmp.m_Bitstream, scalableNestingSEI, pcSlice->getSPS());
        writeRBSPTrailingBits(naluTmp.m_Bitstream);
        UInt seiPositionInAu = xGetFirstSeiLocation(accessUnit);
        UInt offsetPosition = m_activeParameterSetSEIPresentInAU + m_bufferingPeriodSEIPresentInAU + m_pictureTimingSEIPresentInAU;   // Insert BP SEI after non-nested APS, BP and PT SEIs
        AccessUnit::iterator it;
        for(j = 0, it = accessUnit.begin(); j < seiPositionInAu + offsetPosition; j++)
        {
          it++;
        }
        accessUnit.insert(it, new NALUnitEBSP(naluTmp));
        m_nestedBufferingPeriodSEIPresentInAU = true;
      }

      m_lastBPSEI = m_totalCoded;
      m_cpbRemovalDelay = 0;
    }
    m_cpbRemovalDelay ++;
    if( ( m_pcEncTop->getRecoveryPointSEIEnabled() ) && ( pcSlice->getSliceType() == I_SLICE ) )
    {
      if( m_pcEncTop->getGradualDecodingRefreshInfoEnabled() && !pcSlice->getRapPicFlag() )
      {
        // Gradual decoding refresh SEI 
        OutputNALUnit nalu(NAL_UNIT_PREFIX_SEI);
        m_pcEntropyCoder->setEntropyCoder(m_pcCavlcCoder, pcSlice);
        m_pcEntropyCoder->setBitstream(&nalu.m_Bitstream);

        SEIGradualDecodingRefreshInfo seiGradualDecodingRefreshInfo;
        seiGradualDecodingRefreshInfo.m_gdrForegroundFlag = true; // Indicating all "foreground"

        m_seiWriter.writeSEImessage( nalu.m_Bitstream, seiGradualDecodingRefreshInfo, pcSlice->getSPS() );
        writeRBSPTrailingBits(nalu.m_Bitstream);
        accessUnit.push_back(new NALUnitEBSP(nalu));
      }
    // Recovery point SEI
      OutputNALUnit nalu(NAL_UNIT_PREFIX_SEI);
      m_pcEntropyCoder->setEntropyCoder(m_pcCavlcCoder, pcSlice);
      m_pcEntropyCoder->setBitstream(&nalu.m_Bitstream);

      SEIRecoveryPoint sei_recovery_point;
      sei_recovery_point.m_recoveryPocCnt    = 0;
      sei_recovery_point.m_exactMatchingFlag = ( pcSlice->getPOC() == 0 ) ? (true) : (false);
      sei_recovery_point.m_brokenLinkFlag    = false;
#if ALLOW_RECOVERY_POINT_AS_RAP
      if(m_pcCfg->getDecodingRefreshType() == 3)
      {
        m_iLastRecoveryPicPOC = pocCurr;
      }
#endif

      m_seiWriter.writeSEImessage( nalu.m_Bitstream, sei_recovery_point, pcSlice->getSPS() );
      writeRBSPTrailingBits(nalu.m_Bitstream);
      accessUnit.push_back(new NALUnitEBSP(nalu));
    }

    /* use the main bitstream buffer for storing the marshalled picture */
    m_pcEntropyCoder->setBitstream(NULL);

    startCUAddrSliceIdx = 0;
    startCUAddrSlice    = 0; 

    startCUAddrSliceSegmentIdx = 0;
    startCUAddrSliceSegment    = 0; 
    nextCUAddr                 = 0;
    pcSlice = pcPic->getSlice(startCUAddrSliceIdx);

    Int processingState = (pcSlice->getSPS()->getUseSAO())?(EXECUTE_INLOOPFILTER):(ENCODE_SLICE);
    Bool skippedSlice=false;
    while (nextCUAddr < uiRealEndAddress) // Iterate over all slices
    {
      switch(processingState)
      {
      case ENCODE_SLICE:
        {
          pcSlice->setNextSlice       ( false );
          pcSlice->setNextSliceSegment( false );
          if (nextCUAddr == m_storedStartCUAddrForEncodingSlice[startCUAddrSliceIdx])
          {
            pcSlice = pcPic->getSlice(startCUAddrSliceIdx);
            if(startCUAddrSliceIdx > 0 && pcSlice->getSliceType()!= I_SLICE)
            {
              pcSlice->checkColRefIdx(startCUAddrSliceIdx, pcPic);
            }
            pcPic->setCurrSliceIdx(startCUAddrSliceIdx);
            m_pcSliceEncoder->setSliceIdx(startCUAddrSliceIdx);
            assert(startCUAddrSliceIdx == pcSlice->getSliceIdx());
            // Reconstruction slice
            pcSlice->setSliceCurStartCUAddr( nextCUAddr );  // to be used in encodeSlice() + context restriction
            pcSlice->setSliceCurEndCUAddr  ( m_storedStartCUAddrForEncodingSlice[startCUAddrSliceIdx+1 ] );
            // Dependent slice
            pcSlice->setSliceSegmentCurStartCUAddr( nextCUAddr );  // to be used in encodeSlice() + context restriction
            pcSlice->setSliceSegmentCurEndCUAddr  ( m_storedStartCUAddrForEncodingSliceSegment[startCUAddrSliceSegmentIdx+1 ] );

            pcSlice->setNextSlice       ( true );

            startCUAddrSliceIdx++;
            startCUAddrSliceSegmentIdx++;
          } 
          else if (nextCUAddr == m_storedStartCUAddrForEncodingSliceSegment[startCUAddrSliceSegmentIdx])
          {
            // Dependent slice
            pcSlice->setSliceSegmentCurStartCUAddr( nextCUAddr );  // to be used in encodeSlice() + context restriction
            pcSlice->setSliceSegmentCurEndCUAddr  ( m_storedStartCUAddrForEncodingSliceSegment[startCUAddrSliceSegmentIdx+1 ] );

            pcSlice->setNextSliceSegment( true );

            startCUAddrSliceSegmentIdx++;
          }

          pcSlice->setRPS(pcPic->getSlice(0)->getRPS());
          pcSlice->setRPSidx(pcPic->getSlice(0)->getRPSidx());
          UInt uiDummyStartCUAddr;
          UInt uiDummyBoundingCUAddr;
          m_pcSliceEncoder->xDetermineStartAndBoundingCUAddr(uiDummyStartCUAddr,uiDummyBoundingCUAddr,pcPic,true);

          uiInternalAddress = pcPic->getPicSym()->getPicSCUAddr(pcSlice->getSliceSegmentCurEndCUAddr()-1) % pcPic->getNumPartInCU();
          uiExternalAddress = pcPic->getPicSym()->getPicSCUAddr(pcSlice->getSliceSegmentCurEndCUAddr()-1) / pcPic->getNumPartInCU();
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
          UInt endAddress = pcPic->getPicSym()->getPicSCUEncOrder(uiExternalAddress*pcPic->getNumPartInCU()+uiInternalAddress);
          if(endAddress<=pcSlice->getSliceSegmentCurStartCUAddr()) 
          {
            UInt boundingAddrSlice, boundingAddrSliceSegment;
            boundingAddrSlice          = m_storedStartCUAddrForEncodingSlice[startCUAddrSliceIdx];          
            boundingAddrSliceSegment = m_storedStartCUAddrForEncodingSliceSegment[startCUAddrSliceSegmentIdx];          
            nextCUAddr               = min(boundingAddrSlice, boundingAddrSliceSegment);
            if(pcSlice->isNextSlice())
            {
              skippedSlice=true;
            }
            continue;
          }
          if(skippedSlice) 
          {
            pcSlice->setNextSlice       ( true );
            pcSlice->setNextSliceSegment( false );
          }
          skippedSlice=false;
          pcSlice->allocSubstreamSizes( iNumSubstreams );
          for ( UInt ui = 0 ; ui < iNumSubstreams; ui++ )
          {
            pcSubstreamsOut[ui].clear();
          }

          m_pcEntropyCoder->setEntropyCoder   ( m_pcCavlcCoder, pcSlice );
          m_pcEntropyCoder->resetEntropy      ();
          /* start slice NALunit */
#if H_MV
          OutputNALUnit nalu( pcSlice->getNalUnitType(), pcSlice->getTLayer(), getLayerId() );
#else
          OutputNALUnit nalu( pcSlice->getNalUnitType(), pcSlice->getTLayer() );
#endif
          Bool sliceSegment = (!pcSlice->isNextSlice());
          if (!sliceSegment)
          {
            uiOneBitstreamPerSliceLength = 0; // start of a new slice
          }
          m_pcEntropyCoder->setBitstream(&nalu.m_Bitstream);

#if SETTING_NO_OUT_PIC_PRIOR
          pcSlice->setNoRaslOutputFlag(false);
          if (pcSlice->isIRAP())
          {
            if (pcSlice->getNalUnitType() >= NAL_UNIT_CODED_SLICE_BLA_W_LP && pcSlice->getNalUnitType() <= NAL_UNIT_CODED_SLICE_IDR_N_LP)
            {
              pcSlice->setNoRaslOutputFlag(true);
            }
            //the inference for NoOutputPriorPicsFlag
            // KJS: This cannot happen at the encoder
            if (!m_bFirst && pcSlice->isIRAP() && pcSlice->getNoRaslOutputFlag())
            {
              if (pcSlice->getNalUnitType() == NAL_UNIT_CODED_SLICE_CRA)
              {
                pcSlice->setNoOutputPriorPicsFlag(true);
              }
            }
          }
#endif

          tmpBitsBeforeWriting = m_pcEntropyCoder->getNumberOfWrittenBits();
          m_pcEntropyCoder->encodeSliceHeader(pcSlice);
          actualHeadBits += ( m_pcEntropyCoder->getNumberOfWrittenBits() - tmpBitsBeforeWriting );

          // is it needed?
          {
            if (!sliceSegment)
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
            if (!sliceSegment)
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
            pcSlice->setTileLocationCount ( 0 );
          m_pcSliceEncoder->encodeSlice(pcPic, pcSubstreamsOut);

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

              pcSubstreamsOut[ui].writeByteAlignment();   // Byte-alignment in slice_data() at end of sub-stream
              // Byte alignment is necessary between tiles when tiles are independent.
              uiTotalCodedSize += pcSubstreamsOut[ui].getNumberOfWrittenBits();

              Bool bNextSubstreamInNewTile = ((ui+1) < iNumSubstreams)&& ((ui+1)%uiNumSubstreamsPerTile == 0);
              if (bNextSubstreamInNewTile)
              {
                pcSlice->setTileLocation(ui/uiNumSubstreamsPerTile, pcSlice->getTileOffstForMultES()+(uiTotalCodedSize>>3));
              }
              if (ui+1 < pcSlice->getPPS()->getNumSubstreams())
              {
                puiSubstreamSizes[ui] = pcSubstreamsOut[ui].getNumberOfWrittenBits() + (pcSubstreamsOut[ui].countStartCodeEmulations()<<3);
              }
            }

            // Complete the slice header info.
            m_pcEntropyCoder->setEntropyCoder   ( m_pcCavlcCoder, pcSlice );
            m_pcEntropyCoder->setBitstream(&nalu.m_Bitstream);
            m_pcEntropyCoder->encodeTilesWPPEntryPoint( pcSlice );

            // Substreams...
            TComOutputBitstream *pcOut = pcBitstreamRedirect;
          Int offs = 0;
          Int nss = pcSlice->getPPS()->getNumSubstreams();
          if (pcSlice->getPPS()->getEntropyCodingSyncEnabledFlag())
          {
            // 1st line present for WPP.
            offs = pcSlice->getSliceSegmentCurStartCUAddr()/pcSlice->getPic()->getNumPartInCU()/pcSlice->getPic()->getFrameWidthInCU();
            nss  = pcSlice->getNumEntryPointOffsets()+1;
          }
          for ( UInt ui = 0 ; ui < nss; ui++ )
          {
            pcOut->addSubstream(&pcSubstreamsOut[ui+offs]);
            }
          }

          UInt boundingAddrSlice, boundingAddrSliceSegment;
          boundingAddrSlice        = m_storedStartCUAddrForEncodingSlice[startCUAddrSliceIdx];          
          boundingAddrSliceSegment = m_storedStartCUAddrForEncodingSliceSegment[startCUAddrSliceSegmentIdx];          
          nextCUAddr               = min(boundingAddrSlice, boundingAddrSliceSegment);
          // If current NALU is the first NALU of slice (containing slice header) and more NALUs exist (due to multiple dependent slices) then buffer it.
          // If current NALU is the last NALU of slice and a NALU was buffered, then (a) Write current NALU (b) Update an write buffered NALU at approproate location in NALU list.
          Bool bNALUAlignedWrittenToList    = false; // used to ensure current NALU is not written more than once to the NALU list.
          xAttachSliceDataToNalUnit(nalu, pcBitstreamRedirect);
          accessUnit.push_back(new NALUnitEBSP(nalu));
          actualTotalBits += UInt(accessUnit.back()->m_nalUnitData.str().size()) * 8;
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

          if( ( m_pcCfg->getPictureTimingSEIEnabled() || m_pcCfg->getDecodingUnitInfoSEIEnabled() ) &&
              ( pcSlice->getSPS()->getVuiParametersPresentFlag() ) &&
              ( ( pcSlice->getSPS()->getVuiParameters()->getHrdParameters()->getNalHrdParametersPresentFlag() ) 
             || ( pcSlice->getSPS()->getVuiParameters()->getHrdParameters()->getVclHrdParametersPresentFlag() ) ) &&
              ( pcSlice->getSPS()->getVuiParameters()->getHrdParameters()->getSubPicCpbParamsPresentFlag() ) )
          {
              UInt numNalus = 0;
            UInt numRBSPBytes = 0;
            for (AccessUnit::const_iterator it = accessUnit.begin(); it != accessUnit.end(); it++)
            {
              UInt numRBSPBytes_nal = UInt((*it)->m_nalUnitData.str().size());
              if ((*it)->m_nalUnitType != NAL_UNIT_PREFIX_SEI && (*it)->m_nalUnitType != NAL_UNIT_SUFFIX_SEI)
              {
                numRBSPBytes += numRBSPBytes_nal;
                numNalus ++;
              }
            }
            accumBitsDU[ pcSlice->getSliceIdx() ] = ( numRBSPBytes << 3 );
            accumNalsDU[ pcSlice->getSliceIdx() ] = numNalus;   // SEI not counted for bit count; hence shouldn't be counted for # of NALUs - only for consistency
          }
          processingState = ENCODE_SLICE;
          }
          break;
        case EXECUTE_INLOOPFILTER:
          {
            // set entropy coder for RD
            m_pcEntropyCoder->setEntropyCoder ( m_pcSbacCoder, pcSlice );
            if ( pcSlice->getSPS()->getUseSAO() )
            {
              m_pcEntropyCoder->resetEntropy();
              m_pcEntropyCoder->setBitstream( m_pcBitCounter );
            Bool sliceEnabled[NUM_SAO_COMPONENTS];
            m_pcSAO->initRDOCabacCoder(m_pcEncTop->getRDGoOnSbacCoder(), pcSlice);
            m_pcSAO->SAOProcess(pcPic
              , sliceEnabled
              , pcPic->getSlice(0)->getLambdas()
#if SAO_ENCODE_ALLOW_USE_PREDEBLOCK
              , m_pcCfg->getSaoLcuBoundary()
#endif
              );
              m_pcSAO->PCMLFDisableProcess(pcPic);

            //assign SAO slice header
            for(Int s=0; s< uiNumSlices; s++)
            {
              pcPic->getSlice(s)->setSaoEnabledFlag(sliceEnabled[SAO_Y]);
              assert(sliceEnabled[SAO_Cb] == sliceEnabled[SAO_Cr]);
              pcPic->getSlice(s)->setSaoEnabledFlagChroma(sliceEnabled[SAO_Cb]);
              }
            }
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
#if H_3D
      pcPic->compressMotion(2); 
#endif
#if !H_3D
      pcPic->compressMotion(); 
#endif
#if H_MV
      m_pocLastCoded = pcPic->getPOC();
#endif

      //-- For time output for each slice
      Double dEncTime = (Double)(clock()-iBeforeTime) / CLOCKS_PER_SEC;

      const Char* digestStr = NULL;
      if (m_pcCfg->getDecodedPictureHashSEIEnabled())
      {
        /* calculate MD5sum for entire reconstructed picture */
        SEIDecodedPictureHash sei_recon_picture_digest;
        if(m_pcCfg->getDecodedPictureHashSEIEnabled() == 1)
        {
          sei_recon_picture_digest.method = SEIDecodedPictureHash::MD5;
          calcMD5(*pcPic->getPicYuvRec(), sei_recon_picture_digest.digest);
          digestStr = digestToString(sei_recon_picture_digest.digest, 16);
        }
        else if(m_pcCfg->getDecodedPictureHashSEIEnabled() == 2)
        {
          sei_recon_picture_digest.method = SEIDecodedPictureHash::CRC;
          calcCRC(*pcPic->getPicYuvRec(), sei_recon_picture_digest.digest);
          digestStr = digestToString(sei_recon_picture_digest.digest, 2);
        }
        else if(m_pcCfg->getDecodedPictureHashSEIEnabled() == 3)
        {
          sei_recon_picture_digest.method = SEIDecodedPictureHash::CHECKSUM;
          calcChecksum(*pcPic->getPicYuvRec(), sei_recon_picture_digest.digest);
          digestStr = digestToString(sei_recon_picture_digest.digest, 4);
        }
#if H_MV
        OutputNALUnit nalu(NAL_UNIT_SUFFIX_SEI, pcSlice->getTLayer(), getLayerId() );
#else
        OutputNALUnit nalu(NAL_UNIT_SUFFIX_SEI, pcSlice->getTLayer());
#endif

        /* write the SEI messages */
        m_pcEntropyCoder->setEntropyCoder(m_pcCavlcCoder, pcSlice);
        m_seiWriter.writeSEImessage(nalu.m_Bitstream, sei_recon_picture_digest, pcSlice->getSPS());
        writeRBSPTrailingBits(nalu.m_Bitstream);

        accessUnit.insert(accessUnit.end(), new NALUnitEBSP(nalu));
      }
      if (m_pcCfg->getTemporalLevel0IndexSEIEnabled())
      {
        SEITemporalLevel0Index sei_temporal_level0_index;
        if (pcSlice->getRapPicFlag())
        {
          m_tl0Idx = 0;
          m_rapIdx = (m_rapIdx + 1) & 0xFF;
        }
        else
        {
          m_tl0Idx = (m_tl0Idx + (pcSlice->getTLayer() ? 0 : 1)) & 0xFF;
        }
        sei_temporal_level0_index.tl0Idx = m_tl0Idx;
        sei_temporal_level0_index.rapIdx = m_rapIdx;

        OutputNALUnit nalu(NAL_UNIT_PREFIX_SEI); 

        /* write the SEI messages */
        m_pcEntropyCoder->setEntropyCoder(m_pcCavlcCoder, pcSlice);
        m_seiWriter.writeSEImessage(nalu.m_Bitstream, sei_temporal_level0_index, pcSlice->getSPS());
        writeRBSPTrailingBits(nalu.m_Bitstream);

        /* insert the SEI message NALUnit before any Slice NALUnits */
        AccessUnit::iterator it = find_if(accessUnit.begin(), accessUnit.end(), mem_fun(&NALUnit::isSlice));
        accessUnit.insert(it, new NALUnitEBSP(nalu));
      }

      xCalculateAddPSNR( pcPic, pcPic->getPicYuvRec(), accessUnit, dEncTime );

    //In case of field coding, compute the interlaced PSNR for both fields
    if (isField && ((!pcPic->isTopField() && isTff) || (pcPic->isTopField() && !isTff)) && (pcPic->getPOC()%m_iGopSize != 1))
    {
      //get complementary top field
      TComPic* pcPicTop;
      TComList<TComPic*>::iterator   iterPic = rcListPic.begin();
      while ((*iterPic)->getPOC() != pcPic->getPOC()-1)
      {
        iterPic ++;
      }
      pcPicTop = *(iterPic);
      xCalculateInterlacedAddPSNR(pcPicTop, pcPic, pcPicTop->getPicYuvRec(), pcPic->getPicYuvRec(), accessUnit, dEncTime );
    }
    else if (isField && pcPic->getPOC()!= 0 && (pcPic->getPOC()%m_iGopSize == 0))
    {
      //get complementary bottom field
      TComPic* pcPicBottom;
      TComList<TComPic*>::iterator   iterPic = rcListPic.begin();
      while ((*iterPic)->getPOC() != pcPic->getPOC()+1)
      {
        iterPic ++;
      }
      pcPicBottom = *(iterPic);
      xCalculateInterlacedAddPSNR(pcPic, pcPicBottom, pcPic->getPicYuvRec(), pcPicBottom->getPicYuvRec(), accessUnit, dEncTime );
    }
    
      if (digestStr)
      {
        if(m_pcCfg->getDecodedPictureHashSEIEnabled() == 1)
        {
          printf(" [MD5:%s]", digestStr);
        }
        else if(m_pcCfg->getDecodedPictureHashSEIEnabled() == 2)
        {
          printf(" [CRC:%s]", digestStr);
        }
        else if(m_pcCfg->getDecodedPictureHashSEIEnabled() == 3)
        {
          printf(" [Checksum:%s]", digestStr);
        }
      }
      if ( m_pcCfg->getUseRateCtrl() )
      {
        Double avgQP     = m_pcRateCtrl->getRCPic()->calAverageQP();
        Double avgLambda = m_pcRateCtrl->getRCPic()->calAverageLambda();
        if ( avgLambda < 0.0 )
        {
          avgLambda = lambda;
        }
        m_pcRateCtrl->getRCPic()->updateAfterPicture( actualHeadBits, actualTotalBits, avgQP, avgLambda, pcSlice->getSliceType());
        m_pcRateCtrl->getRCPic()->addToPictureLsit( m_pcRateCtrl->getPicList() );

        m_pcRateCtrl->getRCSeq()->updateAfterPic( actualTotalBits );
        if ( pcSlice->getSliceType() != I_SLICE )
        {
          m_pcRateCtrl->getRCGOP()->updateAfterPicture( actualTotalBits );
        }
        else    // for intra picture, the estimated bits are used to update the current status in the GOP
        {
          m_pcRateCtrl->getRCGOP()->updateAfterPicture( estimatedBits );
        }
      }

      if( ( m_pcCfg->getPictureTimingSEIEnabled() || m_pcCfg->getDecodingUnitInfoSEIEnabled() ) &&
          ( pcSlice->getSPS()->getVuiParametersPresentFlag() ) &&
          ( ( pcSlice->getSPS()->getVuiParameters()->getHrdParameters()->getNalHrdParametersPresentFlag() ) 
         || ( pcSlice->getSPS()->getVuiParameters()->getHrdParameters()->getVclHrdParametersPresentFlag() ) ) )
      {
        TComVUI *vui = pcSlice->getSPS()->getVuiParameters();
        TComHRD *hrd = vui->getHrdParameters();

        if( hrd->getSubPicCpbParamsPresentFlag() )
        {
          Int i;
          UInt64 ui64Tmp;
          UInt uiPrev = 0;
          UInt numDU = ( pictureTimingSEI.m_numDecodingUnitsMinus1 + 1 );
          UInt *pCRD = &pictureTimingSEI.m_duCpbRemovalDelayMinus1[0];
          UInt maxDiff = ( hrd->getTickDivisorMinus2() + 2 ) - 1;

          for( i = 0; i < numDU; i ++ )
          {
            pictureTimingSEI.m_numNalusInDuMinus1[ i ]       = ( i == 0 ) ? ( accumNalsDU[ i ] - 1 ) : ( accumNalsDU[ i ] - accumNalsDU[ i - 1] - 1 );
          }

          if( numDU == 1 )
          {
            pCRD[ 0 ] = 0; /* don't care */
          }
          else
          {
            pCRD[ numDU - 1 ] = 0;/* by definition */
            UInt tmp = 0;
            UInt accum = 0;

            for( i = ( numDU - 2 ); i >= 0; i -- )
            {
              ui64Tmp = ( ( ( accumBitsDU[ numDU - 1 ]  - accumBitsDU[ i ] ) * ( vui->getTimingInfo()->getTimeScale() / vui->getTimingInfo()->getNumUnitsInTick() ) * ( hrd->getTickDivisorMinus2() + 2 ) ) / ( m_pcCfg->getTargetBitrate() ) );
              if( (UInt)ui64Tmp > maxDiff )
              {
                tmp ++;
              }
            }
            uiPrev = 0;

            UInt flag = 0;
            for( i = ( numDU - 2 ); i >= 0; i -- )
            {
              flag = 0;
              ui64Tmp = ( ( ( accumBitsDU[ numDU - 1 ]  - accumBitsDU[ i ] ) * ( vui->getTimingInfo()->getTimeScale() / vui->getTimingInfo()->getNumUnitsInTick() ) * ( hrd->getTickDivisorMinus2() + 2 ) ) / ( m_pcCfg->getTargetBitrate() ) );

              if( (UInt)ui64Tmp > maxDiff )
              {
                if(uiPrev >= maxDiff - tmp)
                {
                  ui64Tmp = uiPrev + 1;
                  flag = 1;
                }
                else                            ui64Tmp = maxDiff - tmp + 1;
              }
              pCRD[ i ] = (UInt)ui64Tmp - uiPrev - 1;
              if( (Int)pCRD[ i ] < 0 )
              {
                pCRD[ i ] = 0;
              }
              else if (tmp > 0 && flag == 1) 
              {
                tmp --;
              }
              accum += pCRD[ i ] + 1;
              uiPrev = accum;
            }
          }
        }
        if( m_pcCfg->getPictureTimingSEIEnabled() )
        {
          {
            OutputNALUnit nalu(NAL_UNIT_PREFIX_SEI, pcSlice->getTLayer());
          m_pcEntropyCoder->setEntropyCoder(m_pcCavlcCoder, pcSlice);
          pictureTimingSEI.m_picStruct = (isField && pcSlice->getPic()->isTopField())? 1 : isField? 2 : 0;
          m_seiWriter.writeSEImessage(nalu.m_Bitstream, pictureTimingSEI, pcSlice->getSPS());
          writeRBSPTrailingBits(nalu.m_Bitstream);
          UInt seiPositionInAu = xGetFirstSeiLocation(accessUnit);
          UInt offsetPosition = m_activeParameterSetSEIPresentInAU 
                                    + m_bufferingPeriodSEIPresentInAU;    // Insert PT SEI after APS and BP SEI
          AccessUnit::iterator it;
          for(j = 0, it = accessUnit.begin(); j < seiPositionInAu + offsetPosition; j++)
          {
            it++;
          }
          accessUnit.insert(it, new NALUnitEBSP(nalu));
          m_pictureTimingSEIPresentInAU = true;
        }
          if ( m_pcCfg->getScalableNestingSEIEnabled() ) // put picture timing SEI into scalable nesting SEI
          {
            OutputNALUnit nalu(NAL_UNIT_PREFIX_SEI, pcSlice->getTLayer());
            m_pcEntropyCoder->setEntropyCoder(m_pcCavlcCoder, pcSlice);
            scalableNestingSEI.m_nestedSEIs.clear();
            scalableNestingSEI.m_nestedSEIs.push_back(&pictureTimingSEI);
            m_seiWriter.writeSEImessage(nalu.m_Bitstream, scalableNestingSEI, pcSlice->getSPS());
            writeRBSPTrailingBits(nalu.m_Bitstream);
            UInt seiPositionInAu = xGetFirstSeiLocation(accessUnit);
            UInt offsetPosition = m_activeParameterSetSEIPresentInAU 
              + m_bufferingPeriodSEIPresentInAU + m_pictureTimingSEIPresentInAU + m_nestedBufferingPeriodSEIPresentInAU;    // Insert PT SEI after APS and BP SEI
            AccessUnit::iterator it;
            for(j = 0, it = accessUnit.begin(); j < seiPositionInAu + offsetPosition; j++)
            {
              it++;
            }
            accessUnit.insert(it, new NALUnitEBSP(nalu));
            m_nestedPictureTimingSEIPresentInAU = true;
          }
        }
        if( m_pcCfg->getDecodingUnitInfoSEIEnabled() && hrd->getSubPicCpbParamsPresentFlag() )
        {             
          m_pcEntropyCoder->setEntropyCoder(m_pcCavlcCoder, pcSlice);
          for( Int i = 0; i < ( pictureTimingSEI.m_numDecodingUnitsMinus1 + 1 ); i ++ )
          {
            OutputNALUnit nalu(NAL_UNIT_PREFIX_SEI, pcSlice->getTLayer());

            SEIDecodingUnitInfo tempSEI;
            tempSEI.m_decodingUnitIdx = i;
            tempSEI.m_duSptCpbRemovalDelay = pictureTimingSEI.m_duCpbRemovalDelayMinus1[i] + 1;
            tempSEI.m_dpbOutputDuDelayPresentFlag = false;
            tempSEI.m_picSptDpbOutputDuDelay = picSptDpbOutputDuDelay;

            AccessUnit::iterator it;
            // Insert the first one in the right location, before the first slice
            if(i == 0)
            {
              // Insert before the first slice. 
              m_seiWriter.writeSEImessage(nalu.m_Bitstream, tempSEI, pcSlice->getSPS());
              writeRBSPTrailingBits(nalu.m_Bitstream);

              UInt seiPositionInAu = xGetFirstSeiLocation(accessUnit);
              UInt offsetPosition = m_activeParameterSetSEIPresentInAU 
                                    + m_bufferingPeriodSEIPresentInAU 
                                    + m_pictureTimingSEIPresentInAU;  // Insert DU info SEI after APS, BP and PT SEI
              for(j = 0, it = accessUnit.begin(); j < seiPositionInAu + offsetPosition; j++)
              {
                it++;
              }
              accessUnit.insert(it, new NALUnitEBSP(nalu));
            }
            else
            {
              Int ctr;
              // For the second decoding unit onwards we know how many NALUs are present
              for (ctr = 0, it = accessUnit.begin(); it != accessUnit.end(); it++)
              {            
                if(ctr == accumNalsDU[ i - 1 ])
                {
                  // Insert before the first slice. 
                  m_seiWriter.writeSEImessage(nalu.m_Bitstream, tempSEI, pcSlice->getSPS());
                  writeRBSPTrailingBits(nalu.m_Bitstream);

                  accessUnit.insert(it, new NALUnitEBSP(nalu));
                  break;
                }
                if ((*it)->m_nalUnitType != NAL_UNIT_PREFIX_SEI && (*it)->m_nalUnitType != NAL_UNIT_SUFFIX_SEI)
                {
                  ctr++;
                }
              }
            }            
          }
        }
      }
      xResetNonNestedSEIPresentFlags();
      xResetNestedSEIPresentFlags();
      pcPic->getPicYuvRec()->copyToPic(pcPicYuvRecOut);

      pcPic->setReconMark   ( true );
#if H_MV
      TComSlice::markIvRefPicsAsShortTerm( m_refPicSetInterLayer0, m_refPicSetInterLayer1 );  
      std::vector<Int> temp; 
      TComSlice::markCurrPic( pcPic ); 
#endif
      m_bFirst = false;
      m_iNumPicCoded++;
      m_totalCoded ++;
      /* logging: insert a newline at end of picture period */
      printf("\n");
      fflush(stdout);

      delete[] pcSubstreamsOut;

#if EFFICIENT_FIELD_IRAP
    if(IRAPtoReorder)
    {
      if(swapIRAPForward)
      {
        if(iGOPid == IRAPGOPid)
        {
          iGOPid = IRAPGOPid +1;
          IRAPtoReorder = false;
        }
        else if(iGOPid == IRAPGOPid +1)
        {
          iGOPid --;
        }
      }
      else
      {
        if(iGOPid == IRAPGOPid)
        {
          iGOPid = IRAPGOPid -1;
        }
        else if(iGOPid == IRAPGOPid -1)
        {
          iGOPid = IRAPGOPid;
          IRAPtoReorder = false;
        }
      }
    }
#endif
  }
  delete pcBitstreamRedirect;

  if( accumBitsDU != NULL) delete accumBitsDU;
  if( accumNalsDU != NULL) delete accumNalsDU;

#if !H_MV
  assert ( (m_iNumPicCoded == iNumPicRcvd) || (isField && iPOCLast == 1) );
#endif
}

#if !H_MV
Void TEncGOP::printOutSummary(UInt uiNumAllPicCoded, bool isField)
{
  assert (uiNumAllPicCoded == m_gcAnalyzeAll.getNumPic());
  
    
  //--CFG_KDY
  if(isField)
  {
    m_gcAnalyzeAll.setFrmRate( m_pcCfg->getFrameRate() * 2);
    m_gcAnalyzeI.setFrmRate( m_pcCfg->getFrameRate() * 2);
    m_gcAnalyzeP.setFrmRate( m_pcCfg->getFrameRate() * 2);
    m_gcAnalyzeB.setFrmRate( m_pcCfg->getFrameRate() * 2);
  }
  else
  {
  m_gcAnalyzeAll.setFrmRate( m_pcCfg->getFrameRate() );
  m_gcAnalyzeI.setFrmRate( m_pcCfg->getFrameRate() );
  m_gcAnalyzeP.setFrmRate( m_pcCfg->getFrameRate() );
  m_gcAnalyzeB.setFrmRate( m_pcCfg->getFrameRate() );
  }
  
  //-- all
  printf( "\n\nSUMMARY --------------------------------------------------------\n" );
  m_gcAnalyzeAll.printOut('a');
  
  printf( "\n\nI Slices--------------------------------------------------------\n" );
  m_gcAnalyzeI.printOut('i');
  
  printf( "\n\nP Slices--------------------------------------------------------\n" );
  m_gcAnalyzeP.printOut('p');
  
  printf( "\n\nB Slices--------------------------------------------------------\n" );
  m_gcAnalyzeB.printOut('b');
  
#if _SUMMARY_OUT_
  m_gcAnalyzeAll.printSummaryOut();
#endif
#if _SUMMARY_PIC_
  m_gcAnalyzeI.printSummary('I');
  m_gcAnalyzeP.printSummary('P');
  m_gcAnalyzeB.printSummary('B');
#endif

  if(isField)
  {
    //-- interlaced summary
    m_gcAnalyzeAll_in.setFrmRate( m_pcCfg->getFrameRate());
    printf( "\n\nSUMMARY INTERLACED ---------------------------------------------\n" );
    m_gcAnalyzeAll_in.printOutInterlaced('a',  m_gcAnalyzeAll.getBits());
    
#if _SUMMARY_OUT_
    m_gcAnalyzeAll_in.printSummaryOutInterlaced();
#endif
  }

  printf("\nRVM: %.3lf\n" , xCalculateRVM());
}
#endif
#if H_3D_VSO
Void TEncGOP::preLoopFilterPicAll( TComPic* pcPic, Dist64& ruiDist, UInt64& ruiBits )
#else
Void TEncGOP::preLoopFilterPicAll( TComPic* pcPic, UInt64& ruiDist, UInt64& ruiBits )
#endif
{
  TComSlice* pcSlice = pcPic->getSlice(pcPic->getCurrSliceIdx());
  Bool bCalcDist = false;
  m_pcLoopFilter->setCfg(m_pcCfg->getLFCrossTileBoundaryFlag());
  m_pcLoopFilter->loopFilterPic( pcPic );
  
  m_pcEntropyCoder->setEntropyCoder ( m_pcEncTop->getRDGoOnSbacCoder(), pcSlice );
  m_pcEntropyCoder->resetEntropy    ();
  m_pcEntropyCoder->setBitstream    ( m_pcBitCounter );
  m_pcEntropyCoder->resetEntropy    ();
  ruiBits += m_pcEntropyCoder->getNumberOfWrittenBits();
  
  if (!bCalcDist)
    ruiDist = xFindDistortionFrame(pcPic->getPicYuvOrg(), pcPic->getPicYuvRec());
}

// ====================================================================================================================
// Protected member functions
// ====================================================================================================================


Void TEncGOP::xInitGOP( Int iPOCLast, Int iNumPicRcvd, TComList<TComPic*>& rcListPic, TComList<TComPicYuv*>& rcListPicYuvRecOut, bool isField )
{
  assert( iNumPicRcvd > 0 );
  //  Exception for the first frames
  if ( ( isField && (iPOCLast == 0 || iPOCLast == 1) ) || (!isField  && (iPOCLast == 0))  )
  {
    m_iGopSize    = 1;
  }
  else
  {
    m_iGopSize    = m_pcCfg->getGOPSize();
  }
  assert (m_iGopSize > 0);
  
  return;
}

Void TEncGOP::xGetBuffer( TComList<TComPic*>&      rcListPic,
                         TComList<TComPicYuv*>&    rcListPicYuvRecOut,
                         Int                       iNumPicRcvd,
                         Int                       iTimeOffset,
                         TComPic*&                 rpcPic,
                         TComPicYuv*&              rpcPicYuvRecOut,
                         Int                       pocCurr,
                         bool                      isField)
{
  Int i;
  //  Rec. output
  TComList<TComPicYuv*>::iterator     iterPicYuvRec = rcListPicYuvRecOut.end();
  
  if (isField)
  {
    for ( i = 0; i < ( (pocCurr == 0 ) || (pocCurr == 1 ) ? (iNumPicRcvd - iTimeOffset + 1) : (iNumPicRcvd - iTimeOffset + 2) ); i++ )
    {
      iterPicYuvRec--;
    }
  }
  else
  {
    for ( i = 0; i < (iNumPicRcvd - iTimeOffset + 1); i++ )
  {
    iterPicYuvRec--;
  }
  
  }
  
  if (isField)
  {
    if(pocCurr == 1)
    {
      iterPicYuvRec++;
    }
  }
  rpcPicYuvRecOut = *(iterPicYuvRec);
  
  //  Current pic.
  TComList<TComPic*>::iterator        iterPic       = rcListPic.begin();
  while (iterPic != rcListPic.end())
  {
    rpcPic = *(iterPic);
    rpcPic->setCurrSliceIdx(0);
    if (rpcPic->getPOC() == pocCurr)
    {
      break;
    }
    iterPic++;
  }

#if !H_MV
  assert( rpcPic != NULL );
#endif
  assert (rpcPic->getPOC() == pocCurr);
  
  return;
}

#if H_3D_VSO
Dist64 TEncGOP::xFindDistortionFrame (TComPicYuv* pcPic0, TComPicYuv* pcPic1)
#else
UInt64 TEncGOP::xFindDistortionFrame (TComPicYuv* pcPic0, TComPicYuv* pcPic1)
#endif
{
  Int     x, y;
  Pel*  pSrc0   = pcPic0 ->getLumaAddr();
  Pel*  pSrc1   = pcPic1 ->getLumaAddr();
  UInt  uiShift = 2 * DISTORTION_PRECISION_ADJUSTMENT(g_bitDepthY-8);
  Int   iTemp;
  
  Int   iStride = pcPic0->getStride();
  Int   iWidth  = pcPic0->getWidth();
  Int   iHeight = pcPic0->getHeight();
  
#if H_3D_VSO
  Dist64  uiTotalDiff = 0;
#else
  UInt64  uiTotalDiff = 0;
#endif
  
  for( y = 0; y < iHeight; y++ )
  {
    for( x = 0; x < iWidth; x++ )
    {
      iTemp = pSrc0[x] - pSrc1[x]; uiTotalDiff += (iTemp*iTemp) >> uiShift;
    }
    pSrc0 += iStride;
    pSrc1 += iStride;
  }
  
  uiShift = 2 * DISTORTION_PRECISION_ADJUSTMENT(g_bitDepthC-8);
  iHeight >>= 1;
  iWidth  >>= 1;
  iStride >>= 1;
  
  pSrc0  = pcPic0->getCbAddr();
  pSrc1  = pcPic1->getCbAddr();
  
  for( y = 0; y < iHeight; y++ )
  {
    for( x = 0; x < iWidth; x++ )
    {
      iTemp = pSrc0[x] - pSrc1[x]; uiTotalDiff += (iTemp*iTemp) >> uiShift;
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
      iTemp = pSrc0[x] - pSrc1[x]; uiTotalDiff += (iTemp*iTemp) >> uiShift;
    }
    pSrc0 += iStride;
    pSrc1 += iStride;
  }
  
  return uiTotalDiff;
}

#if VERBOSE_RATE
static const Char* nalUnitTypeToString(NalUnitType type)
{
  switch (type)
  {
    case NAL_UNIT_CODED_SLICE_TRAIL_R: return "TRAIL_R";
    case NAL_UNIT_CODED_SLICE_TRAIL_N: return "TRAIL_N";
    case NAL_UNIT_CODED_SLICE_TSA_R:      return "TSA_R";
    case NAL_UNIT_CODED_SLICE_TSA_N: return "TSA_N";
    case NAL_UNIT_CODED_SLICE_STSA_R: return "STSA_R";
    case NAL_UNIT_CODED_SLICE_STSA_N: return "STSA_N";
    case NAL_UNIT_CODED_SLICE_BLA_W_LP:   return "BLA_W_LP";
    case NAL_UNIT_CODED_SLICE_BLA_W_RADL: return "BLA_W_RADL";
    case NAL_UNIT_CODED_SLICE_BLA_N_LP: return "BLA_N_LP";
    case NAL_UNIT_CODED_SLICE_IDR_W_RADL: return "IDR_W_RADL";
    case NAL_UNIT_CODED_SLICE_IDR_N_LP: return "IDR_N_LP";
    case NAL_UNIT_CODED_SLICE_CRA: return "CRA";
    case NAL_UNIT_CODED_SLICE_RADL_R:     return "RADL_R";
    case NAL_UNIT_CODED_SLICE_RADL_N:     return "RADL_N";
    case NAL_UNIT_CODED_SLICE_RASL_R:     return "RASL_R";
    case NAL_UNIT_CODED_SLICE_RASL_N:     return "RASL_N";
    case NAL_UNIT_VPS: return "VPS";
    case NAL_UNIT_SPS: return "SPS";
    case NAL_UNIT_PPS: return "PPS";
    case NAL_UNIT_ACCESS_UNIT_DELIMITER: return "AUD";
    case NAL_UNIT_EOS: return "EOS";
    case NAL_UNIT_EOB: return "EOB";
    case NAL_UNIT_FILLER_DATA: return "FILLER";
    case NAL_UNIT_PREFIX_SEI:             return "SEI";
    case NAL_UNIT_SUFFIX_SEI:             return "SEI";
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
  
#if H_3D_VSO
#if H_3D_VSO_SYNTH_DIST_OUT
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
  {
#endif
#endif
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
  
  Int maxvalY = 255 << (g_bitDepthY-8);
  Int maxvalC = 255 << (g_bitDepthC-8);
  Double fRefValueY = (Double) maxvalY * maxvalY * iSize;
  Double fRefValueC = (Double) maxvalC * maxvalC * iSize / 4.0;
  dYPSNR            = ( uiSSDY ? 10.0 * log10( fRefValueY / (Double)uiSSDY ) : 99.99 );
  dUPSNR            = ( uiSSDU ? 10.0 * log10( fRefValueC / (Double)uiSSDU ) : 99.99 );
  dVPSNR            = ( uiSSDV ? 10.0 * log10( fRefValueC / (Double)uiSSDV ) : 99.99 );
#if H_3D_VSO
#if H_3D_VSO_SYNTH_DIST_OUT
}
#endif 
#endif
  /* calculate the size of the access unit, excluding:
   *  - any AnnexB contributions (start_code_prefix, zero_byte, etc.,)
   *  - SEI NAL units
   */
  UInt numRBSPBytes = 0;
  for (AccessUnit::const_iterator it = accessUnit.begin(); it != accessUnit.end(); it++)
  {
    UInt numRBSPBytes_nal = UInt((*it)->m_nalUnitData.str().size());
#if VERBOSE_RATE
    printf("*** %6s numBytesInNALunit: %u\n", nalUnitTypeToString((*it)->m_nalUnitType), numRBSPBytes_nal);
#endif
    if ((*it)->m_nalUnitType != NAL_UNIT_PREFIX_SEI && (*it)->m_nalUnitType != NAL_UNIT_SUFFIX_SEI)
    {
      numRBSPBytes += numRBSPBytes_nal;
    }
  }

  UInt uibits = numRBSPBytes * 8;
  m_vRVM_RP.push_back( uibits );

  //===== add PSNR =====
#if H_MV
  m_pcEncTop->getAnalyzeAll()->addResult (dYPSNR, dUPSNR, dVPSNR, (Double)uibits);
#else
  m_gcAnalyzeAll.addResult (dYPSNR, dUPSNR, dVPSNR, (Double)uibits);
#endif
  TComSlice*  pcSlice = pcPic->getSlice(0);
  if (pcSlice->isIntra())
  {
#if H_MV
    m_pcEncTop->getAnalyzeI()->addResult (dYPSNR, dUPSNR, dVPSNR, (Double)uibits);
#else
    m_gcAnalyzeI.addResult (dYPSNR, dUPSNR, dVPSNR, (Double)uibits);
#endif
  }
  if (pcSlice->isInterP())
  {
#if H_MV
    m_pcEncTop->getAnalyzeP()->addResult (dYPSNR, dUPSNR, dVPSNR, (Double)uibits);
#else
    m_gcAnalyzeP.addResult (dYPSNR, dUPSNR, dVPSNR, (Double)uibits);
#endif
  }
  if (pcSlice->isInterB())
  {
#if H_MV
    m_pcEncTop->getAnalyzeB()->addResult (dYPSNR, dUPSNR, dVPSNR, (Double)uibits);
#else
    m_gcAnalyzeB.addResult (dYPSNR, dUPSNR, dVPSNR, (Double)uibits);
#endif
  }

  Char c = (pcSlice->isIntra() ? 'I' : pcSlice->isInterP() ? 'P' : 'B');
  if (!pcSlice->isReferenced()) c += 32;

#if ADAPTIVE_QP_SELECTION
#if H_MV
  printf("Layer %3d   POC %4d TId: %1d ( %c-SLICE, nQP %d QP %d ) %10d  bits",
    pcSlice->getLayerId(),
    pcSlice->getPOC(),
    pcSlice->getTLayer(),
    c,
    pcSlice->getSliceQpBase(),
    pcSlice->getSliceQp(),
    uibits );
#else
  printf("POC %4d TId: %1d ( %c-SLICE, nQP %d QP %d ) %10d bits",
         pcSlice->getPOC(),
         pcSlice->getTLayer(),
         c,
         pcSlice->getSliceQpBase(),
         pcSlice->getSliceQp(),
         uibits );
#endif
#else
#if H_MV
  printf("Layer %3d   POC %4d TId: %1d ( %c-SLICE, QP %d ) %10d bits",
    pcSlice->getLayerId(),
    pcSlice->getPOC()-pcSlice->getLastIDR(),
    pcSlice->getTLayer(),
    c,
    pcSlice->getSliceQp(),
    uibits );
#else
  printf("POC %4d TId: %1d ( %c-SLICE, QP %d ) %10d bits",
         pcSlice->getPOC()-pcSlice->getLastIDR(),
         pcSlice->getTLayer(),
         c,
         pcSlice->getSliceQp(),
         uibits );
#endif
#endif

  printf(" [Y %6.4lf dB    U %6.4lf dB    V %6.4lf dB]", dYPSNR, dUPSNR, dVPSNR );
  printf(" [ET %5.0f ]", dEncTime );
  
  for (Int iRefList = 0; iRefList < 2; iRefList++)
  {
    printf(" [L%d ", iRefList);
    for (Int iRefIndex = 0; iRefIndex < pcSlice->getNumRefIdx(RefPicList(iRefList)); iRefIndex++)
    {
#if H_MV
      if( pcSlice->getLayerId() != pcSlice->getRefLayerId( RefPicList(iRefList), iRefIndex ) )
      {
        printf( "V%d ", pcSlice->getRefLayerId( RefPicList(iRefList), iRefIndex ) );
      }
      else
      {
#endif
      printf ("%d ", pcSlice->getRefPOC(RefPicList(iRefList), iRefIndex)-pcSlice->getLastIDR());
#if H_MV
      }
#endif
    }
    printf("]");
  }
}


Void reinterlace(Pel* top, Pel* bottom, Pel* dst, UInt stride, UInt width, UInt height, bool isTff)
{
  
  for (Int y = 0; y < height; y++)
  {
    for (Int x = 0; x < width; x++)
    {
      dst[x] = isTff ? top[x] : bottom[x];
      dst[stride+x] = isTff ? bottom[x] : top[x];
    }
    top += stride;
    bottom += stride;
    dst += stride*2;
  }
}


Void TEncGOP::xCalculateInterlacedAddPSNR( TComPic* pcPicOrgTop, TComPic* pcPicOrgBottom, TComPicYuv* pcPicRecTop, TComPicYuv* pcPicRecBottom, const AccessUnit& accessUnit, Double dEncTime )
{
#if  H_MV
  assert( 0 ); // Field coding and MV need to be aligned. 
#else
  Int     x, y;
  
  UInt64 uiSSDY_in  = 0;
  UInt64 uiSSDU_in  = 0;
  UInt64 uiSSDV_in  = 0;
  
  Double  dYPSNR_in  = 0.0;
  Double  dUPSNR_in  = 0.0;
  Double  dVPSNR_in  = 0.0;
  
  /*------ INTERLACED PSNR -----------*/
  
  /* Luma */
  
  Pel*  pOrgTop = pcPicOrgTop->getPicYuvOrg()->getLumaAddr();
  Pel*  pOrgBottom = pcPicOrgBottom->getPicYuvOrg()->getLumaAddr();
  Pel*  pRecTop = pcPicRecTop->getLumaAddr();
  Pel*  pRecBottom = pcPicRecBottom->getLumaAddr();
  
  Int   iWidth;
  Int   iHeight;
  Int iStride;
  
  iWidth  = pcPicOrgTop->getPicYuvOrg()->getWidth () - m_pcEncTop->getPad(0);
  iHeight = pcPicOrgTop->getPicYuvOrg()->getHeight() - m_pcEncTop->getPad(1);
  iStride = pcPicOrgTop->getPicYuvOrg()->getStride();
  Int   iSize   = iWidth*iHeight;
  bool isTff = pcPicOrgTop->isTopField();
  
  TComPicYuv* pcOrgInterlaced = new TComPicYuv;
  pcOrgInterlaced->create( iWidth, iHeight << 1, g_uiMaxCUWidth, g_uiMaxCUHeight, g_uiMaxCUDepth );
  
  TComPicYuv* pcRecInterlaced = new TComPicYuv;
  pcRecInterlaced->create( iWidth, iHeight << 1, g_uiMaxCUWidth, g_uiMaxCUHeight, g_uiMaxCUDepth );
  
  Pel* pOrgInterlaced = pcOrgInterlaced->getLumaAddr();
  Pel* pRecInterlaced = pcRecInterlaced->getLumaAddr();
  
  //=== Interlace fields ====
  reinterlace(pOrgTop, pOrgBottom, pOrgInterlaced, iStride, iWidth, iHeight, isTff);
  reinterlace(pRecTop, pRecBottom, pRecInterlaced, iStride, iWidth, iHeight, isTff);
  
  //===== calculate PSNR =====
  for( y = 0; y < iHeight << 1; y++ )
  {
    for( x = 0; x < iWidth; x++ )
    {
      Int iDiff = (Int)( pOrgInterlaced[x] - pRecInterlaced[x] );
      uiSSDY_in   += iDiff * iDiff;
    }
    pOrgInterlaced += iStride;
    pRecInterlaced += iStride;
  }
  
  /*Chroma*/
  
  iHeight >>= 1;
  iWidth  >>= 1;
  iStride >>= 1;
  
  pOrgTop = pcPicOrgTop->getPicYuvOrg()->getCbAddr();
  pOrgBottom = pcPicOrgBottom->getPicYuvOrg()->getCbAddr();
  pRecTop = pcPicRecTop->getCbAddr();
  pRecBottom = pcPicRecBottom->getCbAddr();
  pOrgInterlaced = pcOrgInterlaced->getCbAddr();
  pRecInterlaced = pcRecInterlaced->getCbAddr();
  
  //=== Interlace fields ====
  reinterlace(pOrgTop, pOrgBottom, pOrgInterlaced, iStride, iWidth, iHeight, isTff);
  reinterlace(pRecTop, pRecBottom, pRecInterlaced, iStride, iWidth, iHeight, isTff);
  
  //===== calculate PSNR =====
  for( y = 0; y < iHeight << 1; y++ )
  {
    for( x = 0; x < iWidth; x++ )
    {
      Int iDiff = (Int)( pOrgInterlaced[x] - pRecInterlaced[x] );
      uiSSDU_in   += iDiff * iDiff;
    }
    pOrgInterlaced += iStride;
    pRecInterlaced += iStride;
  }
  
  pOrgTop = pcPicOrgTop->getPicYuvOrg()->getCrAddr();
  pOrgBottom = pcPicOrgBottom->getPicYuvOrg()->getCrAddr();
  pRecTop = pcPicRecTop->getCrAddr();
  pRecBottom = pcPicRecBottom->getCrAddr();
  pOrgInterlaced = pcOrgInterlaced->getCrAddr();
  pRecInterlaced = pcRecInterlaced->getCrAddr();
  
  //=== Interlace fields ====
  reinterlace(pOrgTop, pOrgBottom, pOrgInterlaced, iStride, iWidth, iHeight, isTff);
  reinterlace(pRecTop, pRecBottom, pRecInterlaced, iStride, iWidth, iHeight, isTff);
  
  //===== calculate PSNR =====
  for( y = 0; y < iHeight << 1; y++ )
  {
    for( x = 0; x < iWidth; x++ )
    {
      Int iDiff = (Int)( pOrgInterlaced[x] - pRecInterlaced[x] );
      uiSSDV_in   += iDiff * iDiff;
    }
    pOrgInterlaced += iStride;
    pRecInterlaced += iStride;
  }
  
  Int maxvalY = 255 << (g_bitDepthY-8);
  Int maxvalC = 255 << (g_bitDepthC-8);
  Double fRefValueY = (Double) maxvalY * maxvalY * iSize*2;
  Double fRefValueC = (Double) maxvalC * maxvalC * iSize*2 / 4.0;
  dYPSNR_in            = ( uiSSDY_in ? 10.0 * log10( fRefValueY / (Double)uiSSDY_in ) : 99.99 );
  dUPSNR_in            = ( uiSSDU_in ? 10.0 * log10( fRefValueC / (Double)uiSSDU_in ) : 99.99 );
  dVPSNR_in            = ( uiSSDV_in ? 10.0 * log10( fRefValueC / (Double)uiSSDV_in ) : 99.99 );
  
  /* calculate the size of the access unit, excluding:
   *  - any AnnexB contributions (start_code_prefix, zero_byte, etc.,)
   *  - SEI NAL units
   */
  UInt numRBSPBytes = 0;
  for (AccessUnit::const_iterator it = accessUnit.begin(); it != accessUnit.end(); it++)
  {
    UInt numRBSPBytes_nal = UInt((*it)->m_nalUnitData.str().size());
    
    if ((*it)->m_nalUnitType != NAL_UNIT_PREFIX_SEI && (*it)->m_nalUnitType != NAL_UNIT_SUFFIX_SEI)
      numRBSPBytes += numRBSPBytes_nal;
  }
  
  UInt uibits = numRBSPBytes * 8 ;
  
  //===== add PSNR =====
  m_gcAnalyzeAll_in.addResult (dYPSNR_in, dUPSNR_in, dVPSNR_in, (Double)uibits);
  
  printf("\n                                      Interlaced frame %d: [Y %6.4lf dB    U %6.4lf dB    V %6.4lf dB]", pcPicOrgBottom->getPOC()/2 , dYPSNR_in, dUPSNR_in, dVPSNR_in );
  
  pcOrgInterlaced->destroy();
  delete pcOrgInterlaced;
  pcRecInterlaced->destroy();
  delete pcRecInterlaced;
#endif
}
/** Function for deciding the nal_unit_type.
 * \param pocCurr POC of the current picture
 * \returns the nal unit type of the picture
 * This function checks the configuration and returns the appropriate nal_unit_type for the picture.
 */
NalUnitType TEncGOP::getNalUnitType(Int pocCurr, Int lastIDR, Bool isField)
{
  if (pocCurr == 0)
  {
    return NAL_UNIT_CODED_SLICE_IDR_W_RADL;
  }
#if EFFICIENT_FIELD_IRAP
  if(isField && pocCurr == 1)
  {
    // to avoid the picture becoming an IRAP
    return NAL_UNIT_CODED_SLICE_TRAIL_R;
  }
#endif

#if ALLOW_RECOVERY_POINT_AS_RAP
  if(m_pcCfg->getDecodingRefreshType() != 3 && (pocCurr - isField) % m_pcCfg->getIntraPeriod() == 0)
#else
  if ((pocCurr - isField) % m_pcCfg->getIntraPeriod() == 0)
#endif
  {
    if (m_pcCfg->getDecodingRefreshType() == 1)
    {
      return NAL_UNIT_CODED_SLICE_CRA;
    }
    else if (m_pcCfg->getDecodingRefreshType() == 2)
    {
      return NAL_UNIT_CODED_SLICE_IDR_W_RADL;
    }
  }
  if(m_pocCRA>0)
  {
    if(pocCurr<m_pocCRA)
    {
      // All leading pictures are being marked as TFD pictures here since current encoder uses all 
      // reference pictures while encoding leading pictures. An encoder can ensure that a leading 
      // picture can be still decodable when random accessing to a CRA/CRANT/BLA/BLANT picture by 
      // controlling the reference pictures used for encoding that leading picture. Such a leading 
      // picture need not be marked as a TFD picture.
      return NAL_UNIT_CODED_SLICE_RASL_R;
    }
  }
  if (lastIDR>0)
  {
    if (pocCurr < lastIDR)
    {
      return NAL_UNIT_CODED_SLICE_RADL_R;
    }
  }
  return NAL_UNIT_CODED_SLICE_TRAIL_R;
}

Double TEncGOP::xCalculateRVM()
{
  Double dRVM = 0;
  
  if( m_pcCfg->getGOPSize() == 1 && m_pcCfg->getIntraPeriod() != 1 && m_pcCfg->getFramesToBeEncoded() > RVM_VCEGAM10_M * 2 )
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
    
    Double dSigamB = 0;
    for( i = RVM_VCEGAM10_M + 1 ; i < N - RVM_VCEGAM10_M + 1 ; i++ )
    {
      Double tmp = vB[i] - dBavg;
      dSigamB += tmp * tmp;
    }
    dSigamB = sqrt( dSigamB / ( N - 2 * RVM_VCEGAM10_M ) );
    
    Double f = sqrt( 12.0 * ( RVM_VCEGAM10_M - 1 ) / ( RVM_VCEGAM10_M + 1 ) );
    
    dRVM = dSigamB / dRavg * f;
  }
  
  return( dRVM );
}

/** Attaches the input bitstream to the stream in the output NAL unit
    Updates rNalu to contain concatenated bitstream. rpcBitstreamRedirect is cleared at the end of this function call.
 *  \param codedSliceData contains the coded slice data (bitstream) to be concatenated to rNalu
 *  \param rNalu          target NAL unit
 */
Void TEncGOP::xAttachSliceDataToNalUnit (OutputNALUnit& rNalu, TComOutputBitstream*& codedSliceData)
{
  // Byte-align
  rNalu.m_Bitstream.writeByteAlignment();   // Slice header byte-alignment

  // Perform bitstream concatenation
  if (codedSliceData->getNumberOfWrittenBits() > 0)
    {
    rNalu.m_Bitstream.addSubstream(codedSliceData);
  }

  m_pcEntropyCoder->setBitstream(&rNalu.m_Bitstream);

  codedSliceData->clear();
}

// Function will arrange the long-term pictures in the decreasing order of poc_lsb_lt, 
// and among the pictures with the same lsb, it arranges them in increasing delta_poc_msb_cycle_lt value
Void TEncGOP::arrangeLongtermPicturesInRPS(TComSlice *pcSlice, TComList<TComPic*>& rcListPic)
{
  TComReferencePictureSet *rps = pcSlice->getRPS();
  if(!rps->getNumberOfLongtermPictures())
  {
    return;
  }

  // Arrange long-term reference pictures in the correct order of LSB and MSB,
  // and assign values for pocLSBLT and MSB present flag
  Int longtermPicsPoc[MAX_NUM_REF_PICS], longtermPicsLSB[MAX_NUM_REF_PICS], indices[MAX_NUM_REF_PICS];
  Int longtermPicsMSB[MAX_NUM_REF_PICS];
  Bool mSBPresentFlag[MAX_NUM_REF_PICS];
  ::memset(longtermPicsPoc, 0, sizeof(longtermPicsPoc));    // Store POC values of LTRP
  ::memset(longtermPicsLSB, 0, sizeof(longtermPicsLSB));    // Store POC LSB values of LTRP
  ::memset(longtermPicsMSB, 0, sizeof(longtermPicsMSB));    // Store POC LSB values of LTRP
  ::memset(indices        , 0, sizeof(indices));            // Indices to aid in tracking sorted LTRPs
  ::memset(mSBPresentFlag , 0, sizeof(mSBPresentFlag));     // Indicate if MSB needs to be present

  // Get the long-term reference pictures 
  Int offset = rps->getNumberOfNegativePictures() + rps->getNumberOfPositivePictures();
  Int i, ctr = 0;
  Int maxPicOrderCntLSB = 1 << pcSlice->getSPS()->getBitsForPOC();
  for(i = rps->getNumberOfPictures() - 1; i >= offset; i--, ctr++)
  {
    longtermPicsPoc[ctr] = rps->getPOC(i);                                  // LTRP POC
    longtermPicsLSB[ctr] = getLSB(longtermPicsPoc[ctr], maxPicOrderCntLSB); // LTRP POC LSB
    indices[ctr]      = i; 
    longtermPicsMSB[ctr] = longtermPicsPoc[ctr] - longtermPicsLSB[ctr];
  }
  Int numLongPics = rps->getNumberOfLongtermPictures();
  assert(ctr == numLongPics);

  // Arrange pictures in decreasing order of MSB; 
  for(i = 0; i < numLongPics; i++)
  {
    for(Int j = 0; j < numLongPics - 1; j++)
    {
      if(longtermPicsMSB[j] < longtermPicsMSB[j+1])
      {
        std::swap(longtermPicsPoc[j], longtermPicsPoc[j+1]);
        std::swap(longtermPicsLSB[j], longtermPicsLSB[j+1]);
        std::swap(longtermPicsMSB[j], longtermPicsMSB[j+1]);
        std::swap(indices[j]        , indices[j+1]        );
      }
    }
  }

  for(i = 0; i < numLongPics; i++)
  {
    // Check if MSB present flag should be enabled.
    // Check if the buffer contains any pictures that have the same LSB.
    TComList<TComPic*>::iterator  iterPic = rcListPic.begin();  
    TComPic*                      pcPic;
    while ( iterPic != rcListPic.end() )
    {
      pcPic = *iterPic;
      if( (getLSB(pcPic->getPOC(), maxPicOrderCntLSB) == longtermPicsLSB[i])   &&     // Same LSB
                                      (pcPic->getSlice(0)->isReferenced())     &&    // Reference picture
                                        (pcPic->getPOC() != longtermPicsPoc[i])    )  // Not the LTRP itself
      {
        mSBPresentFlag[i] = true;
        break;
      }
      iterPic++;      
    }
  }

  // tempArray for usedByCurr flag
  Bool tempArray[MAX_NUM_REF_PICS]; ::memset(tempArray, 0, sizeof(tempArray));
  for(i = 0; i < numLongPics; i++)
  {
    tempArray[i] = rps->getUsed(indices[i]);
  }
  // Now write the final values;
  ctr = 0;
  Int currMSB = 0, currLSB = 0;
  // currPicPoc = currMSB + currLSB
  currLSB = getLSB(pcSlice->getPOC(), maxPicOrderCntLSB);  
  currMSB = pcSlice->getPOC() - currLSB;

  for(i = rps->getNumberOfPictures() - 1; i >= offset; i--, ctr++)
  {
    rps->setPOC                   (i, longtermPicsPoc[ctr]);
    rps->setDeltaPOC              (i, - pcSlice->getPOC() + longtermPicsPoc[ctr]);
    rps->setUsed                  (i, tempArray[ctr]);
    rps->setPocLSBLT              (i, longtermPicsLSB[ctr]);
    rps->setDeltaPocMSBCycleLT    (i, (currMSB - (longtermPicsPoc[ctr] - longtermPicsLSB[ctr])) / maxPicOrderCntLSB);
    rps->setDeltaPocMSBPresentFlag(i, mSBPresentFlag[ctr]);     

    assert(rps->getDeltaPocMSBCycleLT(i) >= 0);   // Non-negative value
  }
  for(i = rps->getNumberOfPictures() - 1, ctr = 1; i >= offset; i--, ctr++)
  {
    for(Int j = rps->getNumberOfPictures() - 1 - ctr; j >= offset; j--)
    {
      // Here at the encoder we know that we have set the full POC value for the LTRPs, hence we 
      // don't have to check the MSB present flag values for this constraint.
      assert( rps->getPOC(i) != rps->getPOC(j) ); // If assert fails, LTRP entry repeated in RPS!!!
    }
  }
}

/** Function for finding the position to insert the first of APS and non-nested BP, PT, DU info SEI messages.
 * \param accessUnit Access Unit of the current picture
 * This function finds the position to insert the first of APS and non-nested BP, PT, DU info SEI messages.
 */
Int TEncGOP::xGetFirstSeiLocation(AccessUnit &accessUnit)
{
  // Find the location of the first SEI message
  AccessUnit::iterator it;
  Int seiStartPos = 0;
  for(it = accessUnit.begin(); it != accessUnit.end(); it++, seiStartPos++)
  {
     if ((*it)->isSei() || (*it)->isVcl())
     {
       break;
     }               
  }
//  assert(it != accessUnit.end());  // Triggers with some legit configurations
  return seiStartPos;
}

Void TEncGOP::dblMetric( TComPic* pcPic, UInt uiNumSlices )
{
  TComPicYuv* pcPicYuvRec = pcPic->getPicYuvRec();
  Pel* Rec    = pcPicYuvRec->getLumaAddr( 0 );
  Pel* tempRec = Rec;
  Int  stride = pcPicYuvRec->getStride();
  UInt log2maxTB = pcPic->getSlice(0)->getSPS()->getQuadtreeTULog2MaxSize();
  UInt maxTBsize = (1<<log2maxTB);
  const UInt minBlockArtSize = 8;
  const UInt picWidth = pcPicYuvRec->getWidth();
  const UInt picHeight = pcPicYuvRec->getHeight();
  const UInt noCol = (picWidth>>log2maxTB);
  const UInt noRows = (picHeight>>log2maxTB);
  assert(noCol > 1);
  assert(noRows > 1);
  UInt64 *colSAD = (UInt64*)malloc(noCol*sizeof(UInt64));
  UInt64 *rowSAD = (UInt64*)malloc(noRows*sizeof(UInt64));
  UInt colIdx = 0;
  UInt rowIdx = 0;
  Pel p0, p1, p2, q0, q1, q2;
  
  Int qp = pcPic->getSlice(0)->getSliceQp();
  Int bitdepthScale = 1 << (g_bitDepthY-8);
  Int beta = TComLoopFilter::getBeta( qp ) * bitdepthScale;
  const Int thr2 = (beta>>2);
  const Int thr1 = 2*bitdepthScale;
  UInt a = 0;
  
  memset(colSAD, 0, noCol*sizeof(UInt64));
  memset(rowSAD, 0, noRows*sizeof(UInt64));
  
  if (maxTBsize > minBlockArtSize)
  {
    // Analyze vertical artifact edges
    for(Int c = maxTBsize; c < picWidth; c += maxTBsize)
    {
      for(Int r = 0; r < picHeight; r++)
      {
        p2 = Rec[c-3];
        p1 = Rec[c-2];
        p0 = Rec[c-1];
        q0 = Rec[c];
        q1 = Rec[c+1];
        q2 = Rec[c+2];
        a = ((abs(p2-(p1<<1)+p0)+abs(q0-(q1<<1)+q2))<<1);
        if ( thr1 < a && a < thr2)
        {
          colSAD[colIdx] += abs(p0 - q0);
        }
        Rec += stride;
      }
      colIdx++;
      Rec = tempRec;
    }
    
    // Analyze horizontal artifact edges
    for(Int r = maxTBsize; r < picHeight; r += maxTBsize)
    {
      for(Int c = 0; c < picWidth; c++)
      {
        p2 = Rec[c + (r-3)*stride];
        p1 = Rec[c + (r-2)*stride];
        p0 = Rec[c + (r-1)*stride];
        q0 = Rec[c + r*stride];
        q1 = Rec[c + (r+1)*stride];
        q2 = Rec[c + (r+2)*stride];
        a = ((abs(p2-(p1<<1)+p0)+abs(q0-(q1<<1)+q2))<<1);
        if (thr1 < a && a < thr2)
        {
          rowSAD[rowIdx] += abs(p0 - q0);
        }
      }
      rowIdx++;
    }
  }
  
  UInt64 colSADsum = 0;
  UInt64 rowSADsum = 0;
  for(Int c = 0; c < noCol-1; c++)
  {
    colSADsum += colSAD[c];
  }
  for(Int r = 0; r < noRows-1; r++)
  {
    rowSADsum += rowSAD[r];
  }
  
  colSADsum <<= 10;
  rowSADsum <<= 10;
  colSADsum /= (noCol-1);
  colSADsum /= picHeight;
  rowSADsum /= (noRows-1);
  rowSADsum /= picWidth;
  
  UInt64 avgSAD = ((colSADsum + rowSADsum)>>1);
  avgSAD >>= (g_bitDepthY-8);
  
  if ( avgSAD > 2048 )
  {
    avgSAD >>= 9;
    Int offset = Clip3(2,6,(Int)avgSAD);
    for (Int i=0; i<uiNumSlices; i++)
    {
      pcPic->getSlice(i)->setDeblockingFilterOverrideFlag(true);
      pcPic->getSlice(i)->setDeblockingFilterDisable(false);
      pcPic->getSlice(i)->setDeblockingFilterBetaOffsetDiv2( offset );
      pcPic->getSlice(i)->setDeblockingFilterTcOffsetDiv2( offset );
    }
  }
  else
  {
    for (Int i=0; i<uiNumSlices; i++)
    {
      pcPic->getSlice(i)->setDeblockingFilterOverrideFlag(false);
      pcPic->getSlice(i)->setDeblockingFilterDisable(        pcPic->getSlice(i)->getPPS()->getPicDisableDeblockingFilterFlag() );
      pcPic->getSlice(i)->setDeblockingFilterBetaOffsetDiv2( pcPic->getSlice(i)->getPPS()->getDeblockingFilterBetaOffsetDiv2() );
      pcPic->getSlice(i)->setDeblockingFilterTcOffsetDiv2(   pcPic->getSlice(i)->getPPS()->getDeblockingFilterTcOffsetDiv2()   );
    }
  }
  
  free(colSAD);
  free(rowSAD);
}

#if H_MV
Void TEncGOP::xSetRefPicListModificationsMv( std::vector<TComPic*> tempPicLists[2], TComSlice* pcSlice, UInt iGOPid )
{ 
  
  if( pcSlice->getSliceType() == I_SLICE || !(pcSlice->getPPS()->getListsModificationPresentFlag()) || pcSlice->getNumActiveRefLayerPics() == 0 )
  {
    return;
  }
  
  GOPEntry ge = m_pcCfg->getGOPEntry( (pcSlice->getRapPicFlag() && ( pcSlice->getLayerId( ) > 0) ) ? MAX_GOP : iGOPid );
  assert( ge.m_numActiveRefLayerPics == pcSlice->getNumActiveRefLayerPics() ); 

  Int numPicsInTempList     = pcSlice->getNumRpsCurrTempList();  

  // GT: check if SliceType should be checked here. 
  for (Int li = 0; li < 2; li ++) // Loop over lists L0 and L1
  {
    Int numPicsInFinalRefList = pcSlice->getNumRefIdx( ( li == 0 ) ? REF_PIC_LIST_0 : REF_PIC_LIST_1 ); 
            
    Int finalIdxToTempIdxMap[16];
    for( Int k = 0; k < 16; k++ )
    {
      finalIdxToTempIdxMap[ k ] = -1;
    }

    Bool isModified = false;
    if ( numPicsInTempList > 1 )
    {
      for( Int k = 0; k < pcSlice->getNumActiveRefLayerPics(); k++ )
      {
        // get position in temp. list
        Int refPicLayerId = pcSlice->getRefPicLayerId(k);
        Int idxInTempList = 0; 
        for (; idxInTempList < numPicsInTempList; idxInTempList++)
        {
          if ( (tempPicLists[li][idxInTempList])->getLayerId() == refPicLayerId )
          {
            break; 
          }
        }

        Int idxInFinalList = ge.m_interViewRefPosL[ li ][ k ];
        
        // Add negative from behind 
        idxInFinalList = ( idxInFinalList < 0 )? ( numPicsInTempList + idxInFinalList ) : idxInFinalList; 
        
        Bool curIsModified = ( idxInFinalList != idxInTempList ) && ( ( idxInTempList < numPicsInFinalRefList ) || ( idxInFinalList < numPicsInFinalRefList ) ) ;
        if ( curIsModified )
        {
          isModified = true; 
          assert( finalIdxToTempIdxMap[ idxInFinalList ] == -1 ); // Assert when two inter layer reference pictures are sorted to the same position
        }
        finalIdxToTempIdxMap[ idxInFinalList ] = idxInTempList;              
      }
    }

    TComRefPicListModification* refPicListModification = pcSlice->getRefPicListModification();
    refPicListModification->setRefPicListModificationFlagL( li, isModified );  

    if( isModified )
    {
      Int refIdx = 0;
      
      for( Int i = 0; i < numPicsInFinalRefList; i++ )
      {
        if( finalIdxToTempIdxMap[i] >= 0 ) 
        {
          refPicListModification->setRefPicSetIdxL( li, i, finalIdxToTempIdxMap[i] );
        }
        else
        {
          ///* Fill gaps with temporal references *///
          // Forward inter layer reference pictures
          while( ( refIdx < numPicsInTempList ) && ( tempPicLists[li][refIdx]->getLayerId() != getLayerId())  )
          {
            refIdx++; 
          }
          refPicListModification->setRefPicSetIdxL( li, i, refIdx );
          refIdx++;
        }
      }
    }
  }
}
#endif
//! \}
