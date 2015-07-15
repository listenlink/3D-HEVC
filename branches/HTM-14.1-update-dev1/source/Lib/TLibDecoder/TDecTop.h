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
  Void  setSlice    ( TComSlice* pcSlice );

  Bool  isInitialized() const     { return m_bInitialized; }
  Int**** getBaseViewShiftLUTI()  { return m_aiBaseViewShiftLUT;   }

#if NH_3D_IV_MERGE
  Void  copyCamParamForSlice( TComSlice* pcSlice );
#endif


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

#endif //H_3D
/// decoder class
class TDecTop
{
private:
  Int                     m_iMaxRefPicNum;

  NalUnitType             m_associatedIRAPType; ///< NAL unit type of the associated IRAP picture
  Int                     m_pocCRA;            ///< POC number of the latest CRA picture
  Int                     m_pocRandomAccess;   ///< POC number of the random access point (the first IDR or CRA picture)

  TComList<TComPic*>      m_cListPic;         //  Dynamic buffer
#if NH_MV
  Bool*                    m_layerInitilizedFlag; // initialization Layers
  static ParameterSetManager m_parameterSetManager;  // storage for parameter sets 
  Int                      m_targetOlsIdx; 
#else
  ParameterSetManager     m_parameterSetManager;  // storage for parameter sets
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
#if NH_MV
  Bool isRandomAccessSkipPicture(Int& iSkipFrame,  Int& iPOCLastDisplay, const TComVPS* vps);
#else
  Bool isRandomAccessSkipPicture(Int& iSkipFrame,  Int& iPOCLastDisplay);
#endif
  TComPic*                m_pcPic;
  UInt                    m_uiSliceIdx;
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
#if O0043_BEST_EFFORT_DECODING
  UInt                    m_forceDecodeBitDepth;
#endif
  std::ostream           *m_pDecodedSEIOutputStream;

  Bool                    m_warningMessageSkipPicture;
#if NH_MV
  Bool                    m_isLastNALWasEos;
#endif

#if NH_MV
  // For NH_MV m_bFirstSliceInSequence indicates first slice in sequence of the particular layer  
  Int                     m_layerId;
  Int                     m_viewId;
  TComPicLists*           m_ivPicLists;
  std::vector<TComPic*>   m_refPicSetInterLayer0; 
  std::vector<TComPic*>   m_refPicSetInterLayer1; 
#if NH_3D
  Int                     m_viewIndex; 
  Bool                    m_isDepth;
  CamParsCollector*       m_pcCamParsCollector;
  Int                     m_profileIdc;
#endif
#endif

  std::list<InputNALUnit*> m_prefixSEINALUs; /// Buffered up prefix SEI NAL Units.
public:
  TDecTop();
  virtual ~TDecTop();

  Void  create  ();
  Void  destroy ();

  Void setDecodedPictureHashSEIEnabled(Int enabled) { m_cGopDecoder.setDecodedPictureHashSEIEnabled(enabled); }

  Void  init();
#if NH_MV  
  Bool  decode(InputNALUnit& nalu, Int& iSkipFrame, Int& iPOCLastDisplay, Bool newLayer, Bool& sliceSkippedFlag );
  Bool  decode(InputNALUnit& nalu, Int& iSkipFrame, Int& iPOCLastDisplay, Bool newLayer );
#else  
  Bool  decode(InputNALUnit& nalu, Int& iSkipFrame, Int& iPOCLastDisplay);
#endif
  Void  deletePicBuffer();

#if NH_MV
  const TComVPS* getActiveVPS() { return m_parameterSetManager.getActiveVPS( ); }
  const TComSPS* getActiveSPS() { return m_parameterSetManager.getActiveSPS( m_layerId ); }
#endif

#if NH_MV
  Void endPicDecoding(Int& poc, TComList<TComPic*>*& rpcListPic,  std::vector<Int>& targetDecLayerIdSet);  
#else  
  Void  executeLoopFilters(Int& poc, TComList<TComPic*>*& rpcListPic);
#endif
  Void  checkNoOutputPriorPics (TComList<TComPic*>* rpcListPic);

  Bool  getNoOutputPriorPicsFlag () { return m_isNoOutputPriorPics; }
  Void  setNoOutputPriorPicsFlag (Bool val) { m_isNoOutputPriorPics = val; }
  Void  setFirstSliceInPicture (bool val)  { m_bFirstSliceInPicture = val; }
  Bool  getFirstSliceInSequence ()         { return m_bFirstSliceInSequence; }
  Void  setFirstSliceInSequence (bool val) { m_bFirstSliceInSequence = val; }
#if O0043_BEST_EFFORT_DECODING
  Void  setForceDecodeBitDepth(UInt bitDepth) { m_forceDecodeBitDepth = bitDepth; }
#endif
  Void  setDecodedSEIMessageOutputStream(std::ostream *pOpStream) { m_pDecodedSEIOutputStream = pOpStream; }
  UInt  getNumberOfChecksumErrorsDetected() const { return m_cGopDecoder.getNumberOfChecksumErrorsDetected(); }
#if NH_MV    
  TComPic*                getPic                ( Int poc );
  TComList<TComPic*>*     getListPic            ()               { return &m_cListPic;  }  
  Void                    setIvPicLists         ( TComPicLists* picLists) { m_ivPicLists = picLists; }
  Void                    setLayerInitilizedFlags( Bool* val )    { m_layerInitilizedFlag = val; }
  Void                    setTargetOlsIdx       ( Int targetOlsIdx ) { m_targetOlsIdx = targetOlsIdx; }    
  Int                     getTargetOlsIdx       ( )                  { return m_targetOlsIdx; }    
  Int                     getCurrPoc            ()               { return m_apcSlicePilot->getPOC(); }
  Void                    setLayerId            ( Int layer)     { m_layerId = layer;   }
  Int                     getLayerId            ()               { return m_layerId;    }
  Void                    setViewId             ( Int viewId  )  { m_viewId  = viewId;  }
  Int                     getViewId             ()               { return m_viewId;     }  
  Void                    initFromActiveVps     ( const TComVPS* vps );
#if NH_3D    
  Void                    setViewIndex          ( Int viewIndex  )  { m_viewIndex  = viewIndex;  }
  Int                     getViewIndex          ()               { return m_viewIndex;     }  
  Void                    setIsDepth            ( Bool isDepth ) { m_isDepth = isDepth; }
  Bool                    getIsDepth            ()               { return m_isDepth;    }
  Void                    setCamParsCollector( CamParsCollector* pcCamParsCollector ) { m_pcCamParsCollector = pcCamParsCollector; }


  Bool                    decProcAnnexI()           { assert( m_profileIdc != -1 ); return ( m_profileIdc == 8); }    
#endif
#endif

protected:
  Void  xGetNewPicBuffer  (const TComSPS &sps, const TComPPS &pps, TComPic*& rpcPic, const UInt temporalLayer);
  Void  xCreateLostPicture (Int iLostPOC);

  Void      xActivateParameterSets();
#if NH_MV  
  TComPic*  xGetPic( Int layerId, Int poc ); 
  Bool      xDecodeSlice(InputNALUnit &nalu, Int &iSkipFrame, Int iPOCLastDisplay, Bool newLayerFlag, Bool& sliceSkippedFlag );  
  Void      xResetPocInPicBuffer();
  Void      xCeckNoClrasOutput();

  Bool      xAllRefLayersInitilized( const TComVPS* vps );
#else
  Bool      xDecodeSlice(InputNALUnit &nalu, Int &iSkipFrame, Int iPOCLastDisplay);
#endif
  Void      xDecodeVPS(const std::vector<UChar> &naluData);
  Void      xDecodeSPS(const std::vector<UChar> &naluData);
  Void      xDecodePPS(const std::vector<UChar> &naluData);
  Void      xUpdatePreviousTid0POC( TComSlice *pSlice ) { if ((pSlice->getTLayer()==0) && (pSlice->isReferenceNalu() && (pSlice->getNalUnitType()!=NAL_UNIT_CODED_SLICE_RASL_R)&& (pSlice->getNalUnitType()!=NAL_UNIT_CODED_SLICE_RADL_R))) { m_prevTid0POC=pSlice->getPOC(); } }

  Void      xParsePrefixSEImessages();
  Void      xParsePrefixSEIsForUnknownVCLNal();

};// END CLASS DEFINITION TDecTop


//! \}

#endif // __TDECTOP__

