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

/** \file     TAppDecTop.h
    \brief    Decoder application class (header)
*/

#ifndef __TAPPDECTOP__
#define __TAPPDECTOP__

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "TLibVideoIO/TVideoIOYuv.h"
#include "TLibCommon/TComList.h"
#include "TLibCommon/TComPicYuv.h"
#include "TLibDecoder/TDecTop.h"
#include "TAppDecCfg.h"

//! \ingroup TAppDecoder
//! \{

// ====================================================================================================================
// Class definition
// ====================================================================================================================

/// decoder application class
class TAppDecTop : public TAppDecCfg
{
private:
  // class interface  
#if NH_MV
  TDecTop*                        m_tDecTop             [ MAX_NUM_LAYERS ];    ///< decoder classes
  TVideoIOYuv*                    m_tVideoIOYuvReconFile[ MAX_NUM_LAYERS ];    ///< reconstruction YUV class
  Int                             m_layerIdToDecIdx     [ MAX_NUM_LAYER_IDS ]; ///< mapping from layer id to decoder index
  Int                             m_numDecoders;                               ///< number of decoder instances
  TComPicLists                    m_dpb;                                       ///< picture buffers of decoder instances

  TComPic*                        m_curPic;                                    ///< currently decoded picture
  TComAu                          m_curAu;                                     ///< currently decoded Au
 
  // Random access related 
  Bool                            m_handleCraAsBlaFlag; 
  Bool                            m_handleCraAsBlaFlagSetByExtMeans; 
  Bool                            m_noClrasOutputFlag; 
  Bool                            m_noClrasOutputFlagSetByExtMeans; 
  Bool                            m_noRaslOutputFlagAssocIrap           [ MAX_NUM_LAYER_IDS ]; 

  // Layer wise startup
  Bool                            m_firstPicInLayerDecodedFlag          [ MAX_NUM_LAYER_IDS ];
  Bool                            m_layerInitilizedFlag                 [ MAX_NUM_LAYER_IDS ]; 
  Bool                            m_layerResetFlag; 

  // DPB related variables
  Int                             m_maxNumReorderPics; 
  Int                             m_maxLatencyIncreasePlus1; 
  Int                             m_maxLatencyValue; 
  Int                             m_maxDecPicBufferingMinus1            [ MAX_NUM_LAYER_IDS ];

  // Poc resetting
  Int                             m_lastPresentPocResetIdc              [ MAX_NUM_LAYER_IDS ];
  Bool                            m_firstPicInPocResettingPeriodReceived[ MAX_NUM_LAYER_IDS ];
  Bool                            m_pocDecrementedInDpbFlag             [ MAX_NUM_LAYER_IDS ]; 
  Bool                            m_newPicIsFstPicOfAllLayOfPocResetPer;
  Bool                            m_newPicIsPocResettingPic;

  // General decoding state
  Bool                            m_newVpsActivatedbyCurAu; 
  Bool                            m_newVpsActivatedbyCurPic; 
  Bool                            m_eosInLayer                          [ MAX_NUM_LAYER_IDS ];
  Bool                            m_initilizedFromVPS;
  Bool                            m_firstSliceInBitstream;
  UInt64                          m_decodingOrder                       [ MAX_NUM_LAYER_IDS ];
  UInt64                          m_totalNumofPicsReceived;
  Bool                            m_cvsStartFound; 
  Int                             m_smallestLayerId;

  // Decoding processes for current  picture
  DecodingProcess                 m_decProcPocAndRps;
  DecodingProcess                 m_decProcCvsg; 
  
  // Active parameter sets
  const TComPPS*                  m_pps;                                ///< active PPS
  const TComSPS*                  m_sps;                                ///< active SPS
  const TComVPS*                  m_vps;                                ///< active VPS

  Bool                            m_reconOpen           [ MAX_NUM_LAYERS ]; ///< reconstruction file opened
#else
  TDecTop                         m_cTDecTop;                     ///< decoder class
  TVideoIOYuv                     m_cTVideoIOYuvReconFile;        ///< reconstruction YUV class
  // for output control
  Int                             m_iPOCLastDisplay;              ///< last POC in display order
#endif
  std::ofstream                   m_seiMessageFileStream;         ///< Used for outputing SEI messages.  
public:
  TAppDecTop();
  virtual ~TAppDecTop() {}

  Void  create            (); ///< create internal members
  Void  destroy           (); ///< destroy internal members
  Void  decode            (); ///< main decoding function
#if NH_MV
  UInt  getNumberOfChecksumErrorsDetected( ) const;
  UInt  getNumberOfChecksumErrorsDetected( Int decIdx ) const { return m_tDecTop[decIdx]->getNumberOfChecksumErrorsDetected(); }
#else
  UInt  getNumberOfChecksumErrorsDetected() const { return m_cTDecTop.getNumberOfChecksumErrorsDetected(); }
#endif

protected:

  Void  xCreateDecLib     (); ///< create internal classes
  Void  xDestroyDecLib    (); ///< destroy internal classes
  Void  xInitDecLib       (); ///< initialize decoder class

#if !NH_MV
  Void  xWriteOutput      ( TComList<TComPic*>* pcListPic , UInt tId); ///< write YUV to file
  Void  xFlushOutput      ( TComList<TComPic*>* pcListPic ); ///< flush all remaining decoded pictures to file
  Bool  isNaluWithinTargetDecLayerIdSet    ( InputNALUnit* nalu ); ///< check whether given Nalu is within targetDecLayerIdSet

#else
  // Process NAL units 
  Bool xExtractAndRewrite                  ( InputNALUnit* nalu );
  Void xProcessVclNalu                     ( InputNALUnit nalu );
  Bool xIsSkipVclNalu                      ( InputNALUnit& nalu, Bool isFirstSliceOfPic );
  Void xProcessNonVclNalu                  ( InputNALUnit nalu );
  Void xTerminateDecoding                  ( );

  // Process slice 
  Void xDecodeFirstSliceOfPicture          ( InputNALUnit nalu, Bool sliceIsFirstOfNewAu );  
  Void xDecodeFollowSliceOfPicture         ( InputNALUnit nalu );

  // Process picture  
  Void xFinalizePreviousPictures           ( Bool sliceIsFirstOfNewAU );
  Void xFinalizePic                        ( Bool curPicIsLastInAu );
  Void xFinalizeAU                         ( );
  Void xPicDecoding                        ( DecProcPart curPart, Bool picPosInAuIndication ); 

  // Clause 8
  Void x812CvsgDecodingProcess             ( Int decIdx );
  Void x813decProcForCodPicWithLIdZero     ( DecProcPart curPart );

  // Annex C (DPB)
  Void xC522OutputAndRemOfPicsFromDpb      ( );
  Void xC523PicDecMarkAddBumpAndStor       ( );
  Void xC524Bumping                        ( );

  // Annex F.8
  Void xF811GeneralDecProc                 ( InputNALUnit nalu );
  Void xF812CvsgDecodingProcess            ( Int decIdx );
  Void xF813ComDecProcForACodedPic         ( DecProcPart curPart, Bool picPosInAuIndication );
  Void xF814decProcForCodPicWithLIdZero    ( DecProcPart curPart );
  Void xF816decProcEndDecOfCodPicLIdGrtZero( );

  // Annex F.13 (DPB)
  Void xF13521InitDpb                      ( );
  Void xF13522OutputAndRemOfPicsFromDpb    ( Bool beforePocDerivation );
  Void xF13523PicDecMarkAddBumpAndStor     ( Bool curPicIsLastInAu   );
  Void xF13524Bumping                      ( TComList<TComAu*> aus );

  // Helpers
  TDecTop* xGetDecoder                     ( InputNALUnit& nalu );  
  Int   xGetDecoderIdx                     ( Int layerId, Bool createFlag = false );
  Int   xPreDecodePoc                      ( InputNALUnit& nalu );
  Bool  xDetectNewAu                       ( InputNALUnit& nalu );
  Void  xDetectNewPocResettingPeriod       ( InputNALUnit& nalu );
  Bool  xIsNaluInTargetDecLayerIdSet       ( InputNALUnit* nalu ); ///< check whether given Nalu is within targetDecLayerIdSet
  Bool  xAllRefLayersInitilized            ( Int curLayerId );
  Void  xInitFileIO                        ( );  
  Void  xOpenReconFile                     ( TComPic* curPic );
  Void  xFlushOutput                       ( );
  Void  xCropAndOutput                     ( TComPic* curPic );  
#endif
};

//! \}

#endif
