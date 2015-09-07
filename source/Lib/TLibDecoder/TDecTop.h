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

/** \file     TDecTop.h
    \brief    decoder class (header)
*/

#ifndef __TDECTOP__
#define __TDECTOP__

#include "TLibCommon/CommonDef.h"
#include "TLibCommon/TComList.h"
#include "TLibCommon/TComPicYuv.h"
#include "TLibCommon/TComPic.h"
#include "TLibCommon/TComTrQuant.h"
#include "TLibCommon/TComPrediction.h"
#include "TLibCommon/SEI.h"

#include "TDecGop.h"
#include "TDecEntropy.h"
#include "TDecSbac.h"
#include "TDecCAVLC.h"
#include "SEIread.h"

class InputNALUnit;

//! \ingroup TLibDecoder
//! \{

// ====================================================================================================================
// Class definition
// ====================================================================================================================

#if NH_MV
class TAppDecTop;
#endif
#if NH_3D
class CamParsCollector
{
public:
  CamParsCollector  ();
  ~CamParsCollector ();

  Void  init        ( const TComVPS* vps );
  Void  setCodeScaleOffsetFile( FILE* pCodedScaleOffsetFile ) { m_pCodedScaleOffsetFile = pCodedScaleOffsetFile; };     

  Void  uninit      ();
  Void setSlice ( const TComSlice* pcSlice );

  Bool  isInitialized() const     { return m_bInitialized; }
  Int**** getBaseViewShiftLUTI()  { return m_aiBaseViewShiftLUT;   }

private:
  Void xResetReceivedIdc( Bool overWriteFlag ); 
  Void  xOutput     ( Int iPOC );

private:
  Bool    m_bInitialized;
  FILE*   m_pCodedScaleOffsetFile;

  Int**   m_aaiCodedOffset;
  Int**   m_aaiCodedScale;
  
  const TComVPS* m_vps; 
  Int**    m_receivedIdc; 
  Int      m_lastPoc; 
  Int      m_firstReceivedPoc; 

  
  Bool    m_bCamParsVaryOverTime;

  UInt    m_uiBitDepthForLUT;
  UInt    m_iLog2Precision;
  // UInt    m_uiInputBitDepth;

  // look-up tables
  Double****   m_adBaseViewShiftLUT;       ///< Disparity LUT
  Int****      m_aiBaseViewShiftLUT;       ///< Disparity LUT
  Void xCreateLUTs( UInt uiNumberSourceViews, UInt uiNumberTargetViews, Double****& radLUT, Int****& raiLUT);
  Void xInitLUTs( UInt uiSourceView, UInt uiTargetView, Int iScale, Int iOffset, Double****& radLUT, Int****& raiLUT);
  template<class T> Void  xDeleteArray  ( T*& rpt, UInt uiSize1, UInt uiSize2, UInt uiSize3 );
  template<class T> Void  xDeleteArray  ( T*& rpt, UInt uiSize1, UInt uiSize2 );
  template<class T> Void  xDeleteArray  ( T*& rpt, UInt uiSize );

};

template <class T>
Void CamParsCollector::xDeleteArray( T*& rpt, UInt uiSize1, UInt uiSize2, UInt uiSize3 )
{
  if( rpt )
  {
    for( UInt uiK = 0; uiK < uiSize1; uiK++ )
    {
      for( UInt uiL = 0; uiL < uiSize2; uiL++ )
      {
        for( UInt uiM = 0; uiM < uiSize3; uiM++ )
        {
          delete[] rpt[ uiK ][ uiL ][ uiM ];
        }
        delete[] rpt[ uiK ][ uiL ];
      }
      delete[] rpt[ uiK ];
    }
    delete[] rpt;
  }
  rpt = NULL;
};


template <class T>
Void CamParsCollector::xDeleteArray( T*& rpt, UInt uiSize1, UInt uiSize2 )
{
  if( rpt )
  {
    for( UInt uiK = 0; uiK < uiSize1; uiK++ )
    {
      for( UInt uiL = 0; uiL < uiSize2; uiL++ )
      {
        delete[] rpt[ uiK ][ uiL ];
      }
      delete[] rpt[ uiK ];
    }
    delete[] rpt;
  }
  rpt = NULL;
};


template <class T>
Void CamParsCollector::xDeleteArray( T*& rpt, UInt uiSize )
{
  if( rpt )
  {
    for( UInt uiK = 0; uiK < uiSize; uiK++ )
    {
      delete[] rpt[ uiK ];
    }
    delete[] rpt;
  }
  rpt = NULL;
};

#endif //NH_3D
/// decoder class
class TDecTop
{
private:
  Int                         m_iMaxRefPicNum;
                              
  NalUnitType                 m_associatedIRAPType; ///< NAL unit type of the associated IRAP picture
#if !NH_MV
  Int                         m_pocCRA;            ///< POC number of the latest CRA picture
  Int                         m_pocRandomAccess;   ///< POC number of the random access point (the first IDR or CRA picture)
  
  TComList<TComPic*>           m_cListPic;         //  Dynamic buffer
  ParameterSetManager          m_parameterSetManager;  // storage for parameter sets
#endif

  TComSlice*              m_apcSlicePilot;

  SEIMessages             m_SEIs; ///< List of SEI messages that have been received before the first slice and between slices, excluding prefix SEIs...

  // functional classes
  TComPrediction          m_cPrediction;
  TComTrQuant             m_cTrQuant;
  TDecGop                 m_cGopDecoder;
  TDecSlice               m_cSliceDecoder;
  TDecCu                  m_cCuDecoder;
  TDecEntropy             m_cEntropyDecoder;
  TDecCavlc               m_cCavlcDecoder;
  TDecSbac                m_cSbacDecoder;
  TDecBinCABAC            m_cBinCABAC;
  SEIReader               m_seiReader;
  TComLoopFilter          m_cLoopFilter;
  TComSampleAdaptiveOffset m_cSAO;

  Bool isSkipPictureForBLA(Int& iPOCLastDisplay);
#if !NH_MV
  Bool isRandomAccessSkipPicture(Int& iSkipFrame,  Int& iPOCLastDisplay);
#endif

  TComPic*                m_pcPic;
  UInt                    m_uiSliceIdx;
#if !NH_MV
  Int                     m_prevPOC;
  Int                     m_prevTid0POC;
  Bool                    m_bFirstSliceInPicture;  
  Bool                    m_bFirstSliceInSequence;
  Bool                    m_prevSliceSkipped;
  Int                     m_skippedPOC;
  Bool                    m_bFirstSliceInBitstream;
  Int                     m_lastPOCNoOutputPriorPics;
  Bool                    m_isNoOutputPriorPics;
  Bool                    m_craNoRaslOutputFlag;    //value of variable NoRaslOutputFlag of the last CRA pic
#endif
  
#if O0043_BEST_EFFORT_DECODING
  UInt                    m_forceDecodeBitDepth;
#endif


  std::ostream           *m_pDecodedSEIOutputStream;

#if !NH_MV
  Bool                    m_warningMessageSkipPicture;
#endif

#if NH_MV
  // Class interface 
  static ParameterSetManager  m_parameterSetManager;  // storage for parameter sets 
  TComPicLists*           m_dpb; 
#if NH_3D
  CamParsCollector*       m_pcCamParsCollector;
#endif

  // Layer identification
  Int                     m_layerId;
  Int                     m_viewId;
#if NH_3D                 
  Int                     m_viewIndex; 
  Bool                    m_isDepth;
#endif

  // Layer set
  Int                     m_targetOlsIdx; 
  Int                     m_smallestLayerId; 
  Bool                    m_isInOwnTargetDecLayerIdList;   

  // Decoding processes 
  DecodingProcess         m_decodingProcess;
  DecodingProcess         m_decProcPocAndRps;

  // Decoding state
  Bool*                   m_firstPicInLayerDecodedFlag;   
    
  Int                     m_prevPicOrderCnt; 
  Int                     m_prevTid0PicPicOrderCntMsb;
  Int                     m_prevTid0PicSlicePicOrderCntLsb;
  
  Int*                    m_lastPresentPocResetIdc;
  Bool*                   m_pocDecrementedInDpbFlag; 

  Int                     m_prevIrapPoc;
  Int64                   m_prevIrapDecodingOrder;
  Int64                   m_prevStsaDecOrder;
  Int                     m_prevStsaTemporalId;
#endif

  std::list<InputNALUnit*> m_prefixSEINALUs; /// Buffered up prefix SEI NAL Units.
public:
  TDecTop();
  virtual ~TDecTop();

  Void  create  ();
  Void  destroy ();

  Void setDecodedPictureHashSEIEnabled(Int enabled) { m_cGopDecoder.setDecodedPictureHashSEIEnabled(enabled); }

  Void  init();
#if !NH_MV
  Bool  decode(InputNALUnit& nalu, Int& iSkipFrame, Int& iPOCLastDisplay);
  Void  deletePicBuffer();

  Void  executeLoopFilters(Int& poc, TComList<TComPic*>*& rpcListPic);
  Void  checkNoOutputPriorPics (TComList<TComPic*>* rpcListPic);

  Bool  getNoOutputPriorPicsFlag () { return m_isNoOutputPriorPics; }
  Void  setNoOutputPriorPicsFlag (Bool val) { m_isNoOutputPriorPics = val; }
  
  Void  setFirstSliceInPicture (bool val)  { m_bFirstSliceInPicture = val; }

  Bool  getFirstSliceInSequence ()         { return m_bFirstSliceInSequence; }
  Void  setFirstSliceInSequence (bool val) { m_bFirstSliceInSequence = val; }
#endif

#if O0043_BEST_EFFORT_DECODING
  Void  setForceDecodeBitDepth(UInt bitDepth) { m_forceDecodeBitDepth = bitDepth; }
#endif

  Void  setDecodedSEIMessageOutputStream(std::ostream *pOpStream) { m_pDecodedSEIOutputStream = pOpStream; }
  UInt  getNumberOfChecksumErrorsDetected() const { return m_cGopDecoder.getNumberOfChecksumErrorsDetected(); }

#if NH_MV    

  /////////////////////////
  // For access from TAppDecTop
  /////////////////////////

  // Non VCL decoding
  Bool       decodeNonVclNalu            ( InputNALUnit& nalu );                                    
                                    
  // Start picture decoding         
  Int        preDecodePoc                ( Bool firstPicInLayerDecodedFlag, Bool isFstPicOfAllLayOfPocResetPer, Bool isPocResettingPicture ); 
  Void       inferPocResetPeriodId       ( );
  Void       decodeSliceHeader           ( InputNALUnit &nalu );    

  // Picture decoding 
  Void       activatePSsAndInitPicOrSlice( TComPic* newPic ); 
  Void       decodePocAndRps             ( );
  Void       genUnavailableRefPics       ( );                               
  Void       decodeSliceSegment          ( InputNALUnit &nalu );
                                    
  // End Picture decoding           
  Void       executeLoopFilters          ( );
  Void       finalizePic( );
  
  //////////////////////////
  // For access from slice 
  /////////////////////////
  Void       initFromActiveVps           ( const TComVPS* vps );

  //////////////////////////
  // General access
  /////////////////////////
  
  // Picture identification 
  Void       setLayerId            ( Int layer )       { m_layerId = layer;   }
  Int        getLayerId            ( )                 { return m_layerId;    }
  Void       setViewId             ( Int viewId )      { m_viewId  = viewId;  }
  Int        getViewId             ( )                 { return m_viewId;     }  
#if NH_3D    
  Void       setViewIndex          ( Int viewIndex )   { m_viewIndex  = viewIndex;  }
  Int        getViewIndex          ( )                 { return m_viewIndex;     }  
  Void       setIsDepth            ( Bool isDepth )    { m_isDepth = isDepth; }
  Bool       getIsDepth            ( )                 { return m_isDepth;    }
#endif

  // Classes
  Void       setDpb                ( TComPicLists* picLists) { m_dpb = picLists; }
#if NH_3D                                       
  Void       setCamParsCollector   ( CamParsCollector* pcCamParsCollector ) { m_pcCamParsCollector = pcCamParsCollector; }
#endif

  // Slice pilot access
  TComSlice* getSlicePilot                ( )               { return m_apcSlicePilot; }
                                                                         
  // Decoding state                                                      
  Bool      getFirstSliceSegementInPicFlag( );              
  Void      setFirstPicInLayerDecodedFlag(Bool* val )      { m_firstPicInLayerDecodedFlag = val;  }
  Void      setPocDecrementedInDPBFlag   (Bool* val )      { m_pocDecrementedInDpbFlag = val;  }  
  Void      setLastPresentPocResetIdc    (Int*  val )      { m_lastPresentPocResetIdc  = val;  }
                                                           
  // Layer sets                                                          
  Void      setTargetOlsIdx        ( Int targetOlsIdx )    { m_targetOlsIdx = targetOlsIdx; }    
  Int       getTargetOlsIdx        ( )                     { return m_targetOlsIdx; }    
  Int       getSmallestLayerId     ( )                     { return m_smallestLayerId; }    
  Bool      getIsInOwnTargetDecLayerIdList()               { return m_isInOwnTargetDecLayerIdList; }
                                                                         
  // Decoding processes identification                                   
  Bool      decProcClause8( )                              { return ( m_decodingProcess == CLAUSE_8 ); }
  Bool      decProcAnnexF ( )                              { return ( decProcAnnexG() || decProcAnnexH() || decProcAnnexI() ); }
  Bool      decProcAnnexG ( )                              { return ( m_decodingProcess == ANNEX_G || decProcAnnexI() ); }
  Bool      decProcAnnexH ( )                              { return ( m_decodingProcess == ANNEX_H  ); }
  Bool      decProcAnnexI ( )                              { return ( m_decodingProcess == ANNEX_I  ); }
                                                                         
  DecodingProcess getDecodingProcess ( ) const                   { return m_decodingProcess;                }
  Void      setDecProcPocAndRps( DecodingProcess decProc ) { m_decProcPocAndRps = decProc; }    
#endif

protected:

#if !NH_MV
  Void      xGetNewPicBuffer  (const TComSPS &sps, const TComPPS &pps, TComPic*& rpcPic, const UInt temporalLayer);
  Void      xCreateLostPicture (Int iLostPOC);
  Void      xActivateParameterSets();
  Bool      xDecodeSlice                   (InputNALUnit &nalu, Int &iSkipFrame, Int iPOCLastDisplay);
#endif

  Void      xDecodeVPS(const std::vector<UChar> &naluData);
  Void      xDecodeSPS(const std::vector<UChar> &naluData);
  Void      xDecodePPS(const std::vector<UChar> &naluData);  
#if !NH_MV
  Void      xUpdatePreviousTid0POC( TComSlice *pSlice ) { if ((pSlice->getTLayer()==0) && (pSlice->isReferenceNalu() && (pSlice->getNalUnitType()!=NAL_UNIT_CODED_SLICE_RASL_R)&& (pSlice->getNalUnitType()!=NAL_UNIT_CODED_SLICE_RADL_R))) { m_prevTid0POC=pSlice->getPOC(); } }
#endif

  Void      xParsePrefixSEImessages();
  Void      xParsePrefixSEIsForUnknownVCLNal();

#if NH_MV
  // POC 
  Void      x831DecProcForPicOrderCount         ( );
  Void      xF831DecProcForPicOrderCount        ( );
  Int       xGetCurrMsb                         ( Int cl, Int pl, Int pm, Int ml ); 

  //RPS                                         
  Void      x832DecProcForRefPicSet             ( Bool annexFModifications ); 
  Void      xF832DecProcForRefPicSet            ( );
  Void      xG813DecProcForInterLayerRefPicSet  ( );

  // Unavailable Pics 
  Void      x8331GenDecProcForGenUnavilRefPics  ( );
  TComPic*  x8332GenOfOneUnavailPic             ( Bool calledFromCl8331 );
  Void      xF817DecProcForGenUnavRefPicForPicsFrstInDecOrderInLay();
  Void      xF833DecProcForGenUnavRefPics       ( );  
  Void      xCheckUnavailableRefPics            ( ); 
#endif

};// END CLASS DEFINITION TDecTop


//! \}
#endif // __TDECTOP__

