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
#include "TLibCommon/TComDepthMapGenerator.h"
#include "TLibCommon/SEI.h"

#include "TDecGop.h"
#include "TDecEntropy.h"
#include "TDecSbac.h"
#include "TDecCAVLC.h"

struct InputNALUnit;

//! \ingroup TLibDecoder
//! \{

#define APS_RESERVED_BUFFER_SIZE 2 //!< must be equal to or larger than 2 to handle bitstream parsing

// ====================================================================================================================
// Class definition
// ====================================================================================================================

class TAppDecTop;

class CamParsCollector
{
public:
  CamParsCollector  ();
  ~CamParsCollector ();

  Void  init        ( FILE* pCodedScaleOffsetFile );
  Void  uninit      ();
  Void  setSlice    ( TComSlice* pcSlice );

  Bool  isInitialized() const { return m_bInitialized; }

#if MERL_VSP_C0152
  Int**** getBaseViewShiftLUTI()  { return m_aiBaseViewShiftLUT;   }
#endif
private:
  Bool  xIsComplete ();
  Void  xOutput     ( Int iPOC );

private:
  Bool    m_bInitialized;
  FILE*   m_pCodedScaleOffsetFile;

  Int**   m_aaiCodedOffset;
  Int**   m_aaiCodedScale;
  Int*    m_aiViewOrderIndex;
#if QC_MVHEVC_B0046
  Int*    m_aiViewId;
#endif
  Int*    m_aiViewReceived;
  UInt    m_uiCamParsCodedPrecision;
  Bool    m_bCamParsVaryOverTime;
  Int     m_iLastViewId;
  Int     m_iLastPOC;
  UInt    m_uiMaxViewId;

#if MERL_VSP_C0152
  UInt    m_uiBitDepthForLUT;
  UInt    m_iLog2Precision;
  UInt    m_uiInputBitDepth;
  // look-up tables
  Double****   m_adBaseViewShiftLUT;       ///< Disparity LUT
  Int****      m_aiBaseViewShiftLUT;       ///< Disparity LUT
  Void xCreateLUTs( UInt uiNumberSourceViews, UInt uiNumberTargetViews, Double****& radLUT, Int****& raiLUT);
  Void xInitLUTs( UInt uiSourceView, UInt uiTargetView, Int iScale, Int iOffset, Double****& radLUT, Int****& raiLUT);
  template<class T> Void  xDeleteArray  ( T*& rpt, UInt uiSize1, UInt uiSize2, UInt uiSize3 );
  template<class T> Void  xDeleteArray  ( T*& rpt, UInt uiSize1, UInt uiSize2 );
  template<class T> Void  xDeleteArray  ( T*& rpt, UInt uiSize );
#endif

};

#if MERL_VSP_C0152
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

#endif

/// decoder class
class TDecTop
{
private:
  Int                     m_iGopSize;
  Bool                    m_bGopSizeSet;
  int                     m_iMaxRefPicNum;
  
  Bool                    m_bRefreshPending;    ///< refresh pending flag
  Int                     m_pocCRA;            ///< POC number of the latest CRA picture
  Int                     m_pocRandomAccess;   ///< POC number of the random access point (the first IDR or CRA picture)

  UInt                    m_uiValidPS;
  TComList<TComPic*>      m_cListPic;         //  Dynamic buffer
  ParameterSetManagerDecoder m_parameterSetManagerDecoder;  // storage for parameter sets 
  TComRPSList             m_RPSList;
  TComSlice*              m_apcSlicePilot;
  
  SEImessages *m_SEIs; ///< "all" SEI messages.  If not NULL, we own the object.

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
  TComLoopFilter          m_cLoopFilter;
  TComAdaptiveLoopFilter  m_cAdaptiveLoopFilter;
  TComSampleAdaptiveOffset m_cSAO;

#if DEPTH_MAP_GENERATION
  TComDepthMapGenerator   m_cDepthMapGenerator;
#endif
#if HHI_INTER_VIEW_RESIDUAL_PRED
  TComResidualGenerator   m_cResidualGenerator;
#endif

  Bool isRandomAccessSkipPicture(Int& iSkipFrame,  Int& iPOCLastDisplay);
  TComPic*                m_pcPic;
  UInt                    m_uiSliceIdx;
  UInt                    m_uiLastSliceIdx;
  Int                     m_prevPOC;
  Bool                    m_bFirstSliceInPicture;
  Bool                    m_bFirstSliceInSequence;

  Int                     m_viewId;
  Bool                    m_isDepth;
  TAppDecTop*             m_tAppDecTop;
  CamParsCollector*       m_pcCamParsCollector;
  NalUnitType             m_nalUnitTypeBaseView;  

public:
  TDecTop();
  virtual ~TDecTop();
  
  Void  create  ();
  Void  destroy ();

  void setPictureDigestEnabled(bool enabled) { m_cGopDecoder.setPictureDigestEnabled(enabled); }
  
  Void  init( TAppDecTop* pcTAppDecTop, Bool bFirstInstance );
  Bool  decode(InputNALUnit& nalu, Int& iSkipFrame, Int& iPOCLastDisplay);
  
  Void  deletePicBuffer();
#if QC_MVHEVC_B0046
  Void      xCopySPS( TComSPS* pSPSV0);
  Void      xCopyPPS( TComPPS* pPPSV0);
  Void      xCopyVPS( TComVPS* pVPSV0);
#endif
#if HHI_INTER_VIEW_RESIDUAL_PRED
  Void      deleteExtraPicBuffers   ( Int iPoc );
#endif
  Void  compressMotion       ( Int iPoc );

  Void executeDeblockAndAlf(UInt& ruiPOC, TComList<TComPic*>*& rpcListPic, Int& iSkipFrame,  Int& iPOCLastDisplay);

  Void setViewId(Int viewId)      { m_viewId = viewId;}
  Int  getViewId()                { return m_viewId  ;}
  Void setIsDepth( Bool isDepth ) { m_isDepth = isDepth; }

#if DEPTH_MAP_GENERATION
  TComDepthMapGenerator*  getDepthMapGenerator  () { return &m_cDepthMapGenerator; }
#endif

  Void setCamParsCollector( CamParsCollector* pcCamParsCollector ) { m_pcCamParsCollector = pcCamParsCollector; }

  TComList<TComPic*>* getListPic()                              { return &m_cListPic; }
  Void                setTAppDecTop( TAppDecTop* pcTAppDecTop ) { m_tAppDecTop = pcTAppDecTop; }
  TAppDecTop*         getTAppDecTop()                           { return  m_tAppDecTop; }
  NalUnitType         getNalUnitTypeBaseView()                  { return m_nalUnitTypeBaseView; }
#if QC_MVHEVC_B0046
  bool                m_bFirstNal; //used to copy SPS, PPS, VPS
  ParameterSetManagerDecoder* xGetParaSetDec ()        {return  &m_parameterSetManagerDecoder;}
#endif

protected:
  Void  xGetNewPicBuffer  (TComSlice* pcSlice, TComPic*& rpcPic);
  Void  xUpdateGopSize    (TComSlice* pcSlice);
  Void  xCreateLostPicture (Int iLostPOC);

  Void      decodeAPS( TComAPS* cAPS) { m_cEntropyDecoder.decodeAPS(cAPS); };
  Void      xActivateParameterSets();
  Bool      xDecodeSlice(InputNALUnit &nalu, Int &iSkipFrame, Int iPOCLastDisplay);
#if VIDYO_VPS_INTEGRATION|QC_MVHEVC_B0046
  Void      xDecodeVPS();
#endif
  Void      xDecodeSPS();
  Void      xDecodePPS();
  Void      xDecodeAPS();
  Void      xDecodeSEI();

  Void      allocAPS (TComAPS* pAPS); //!< memory allocation for APS
};// END CLASS DEFINITION TDecTop


//! \}

#endif // __TDECTOP__

