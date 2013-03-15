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



/** \file     TDecTop.h
    \brief    decoder class (header)
*/

#ifndef __TDECTOP__
#define __TDECTOP__

#include "../TLibCommon/CommonDef.h"
#include "../TLibCommon/TComList.h"
#include "../TLibCommon/TComPicYuv.h"
#include "../TLibCommon/TComPic.h"
#include "../TLibCommon/TComTrQuant.h"
#include "../TLibCommon/TComDepthMapGenerator.h"
#include "../TLibCommon/TComResidualGenerator.h"
#include "../TLibCommon/SEI.h"

#include "TDecGop.h"
#include "TDecEntropy.h"
#include "TDecSbac.h"
#include "TDecCAVLC.h"

// ====================================================================================================================
// Class definition
// ====================================================================================================================

class TAppDecTop ;


class CamParsCollector
{
public:
  CamParsCollector  ();
  ~CamParsCollector ();

  Void  init        ( FILE* pCodedScaleOffsetFile );
  Void  uninit      ();
  Void  setSlice    ( TComSlice* pcSlice );

private:
  Bool  xIsComplete ();
  Void  xOutput     ( Int iPOC );

private:
  Bool    m_bInitialized;
  FILE*   m_pCodedScaleOffsetFile;

  Int**   m_aaiCodedOffset;
  Int**   m_aaiCodedScale;
  Int*    m_aiViewOrderIndex;
  Int*    m_aiViewReceived;
  UInt    m_uiCamParsCodedPrecision;
  Bool    m_bCamParsVaryOverTime;
  Int     m_iLastViewId;
  Int     m_iLastPOC;
  UInt    m_uiMaxViewId;
};


/// decoder class
class TDecTop
{
private:
  Int                     m_iGopSize;
  Bool                    m_bGopSizeSet;
  int                     m_iMaxRefPicNum;

#if DCM_DECODING_REFRESH
  Bool                    m_bRefreshPending;    ///< refresh pending flag
  UInt                    m_uiPOCCDR;           ///< temporal reference of the CDR picture
#if DCM_SKIP_DECODING_FRAMES
  UInt                    m_uiPOCRA;            ///< temporal reference of the random access point
#endif
#endif

  UInt                    m_uiValidPS;
  TComList<TComPic*>      m_cListPic;         //  Dynamic buffer
  TComSPS                 m_cSPS;
#if FLEX_CODING_ORDER
  TComSPS                 m_cNewSPS;
#endif
  TComPPS                 m_cPPS;
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
#if MTK_SAO
  TComSampleAdaptiveOffset m_cSAO;
#endif
#if DEPTH_MAP_GENERATION
  TComDepthMapGenerator   m_cDepthMapGenerator;
#endif
#if HHI_INTER_VIEW_RESIDUAL_PRED
  TComResidualGenerator   m_cResidualGenerator;
#endif

  Bool                    m_bIsDepth;
  Int                     m_iViewIdx;
#if SONY_COLPIC_AVAILABILITY
  Int                     m_iViewOrderIdx;
#endif
  TAppDecTop*             m_pcTAppDecTop;
  CamParsCollector*       m_pcCamParsCollector;

#if DCM_SKIP_DECODING_FRAMES
  Bool isRandomAccessSkipPicture(Int& iSkipFrame,  Int& iPOCLastDisplay);
#endif
  TComPic*                m_pcPic;
  UInt                    m_uiSliceIdx;
  UInt                    m_uiLastSliceIdx;
  UInt                    m_uiPrevPOC;
  Bool                    m_bFirstSliceInPicture;
  Bool                    m_bFirstSliceInSequence;

public:
  TDecTop();
  virtual ~TDecTop();

  Void  create  ();
  Void  destroy ();

  void setPictureDigestEnabled(bool enabled) { m_cGopDecoder.setPictureDigestEnabled(enabled); }

  Void  init( TAppDecTop* pcTAppDecTop, Bool bFirstInstance = true );
#if DCM_SKIP_DECODING_FRAMES
#if FLEX_CODING_ORDER
  Bool  decode (Bool bEos, TComBitstream* pcBitstream, UInt& ruiPOC, TComList<TComPic*>*& rpcListPic, NalUnitType& reNalUnitType, TComSPS& cComSPS, Int& iSkipFrame, Int& iPOCLastDisplay, Bool& bNewPictureType);
#else
  Bool  decode (Bool bEos, TComBitstream* pcBitstream, UInt& ruiPOC, TComList<TComPic*>*& rpcListPic, NalUnitType& reNalUnitType, TComSPS& cComSPS, Int& iSkipFrame, Int& iPOCLastDisplay);

#endif
#else
  Void  decode ( Bool bEos, TComBitstream* pcBitstream, UInt& ruiPOC, TComList<TComPic*>*& rpcListPic, NalUnitType& reNalUnitType, TComSPS& cComSPS );
#endif

  TComSPS *getSPS() { return (m_uiValidPS & 1) ? &m_cSPS : NULL; }

  Void  deletePicBuffer();

  Void  deleteExtraPicBuffers( Int iPoc );
#if AMVP_BUFFERCOMPRESS
  Void  compressMotion       ( Int iPoc );
#endif

  Void setViewIdx(Int i)					{ m_iViewIdx = i ;}
  Int  getViewIdx()								{ return m_iViewIdx ; }
#if SONY_COLPIC_AVAILABILITY
  Void setViewOrderIdx(Int i)					{ m_iViewOrderIdx = i ;}
  Int  getViewOrderIdx()							{ return m_iViewOrderIdx ; }
#endif

  Void setToDepth(Bool b)         { m_bIsDepth = b ;}
  Bool getIsDepth()               { return m_bIsDepth ;}

  Void setCamParsCollector( CamParsCollector* pcCamParsCollector ) { m_pcCamParsCollector = pcCamParsCollector; }

  TComList<TComPic*>*     getListPic            () { return  &m_cListPic;             }
  TAppDecTop*             getDecTop           ( ){ return  m_pcTAppDecTop ;}
#if DEPTH_MAP_GENERATION
  TComDepthMapGenerator*  getDepthMapGenerator  () { return &m_cDepthMapGenerator; }
#endif

  Void                    setSPS                (TComSPS cSPS );

  UInt                    getCodedPictureBufferSize() { return m_cSPS.getCodedPictureBufferSize() ; }

  Void executeDeblockAndAlf(Bool bEos, TComBitstream* pcBitstream, UInt& ruiPOC, TComList<TComPic*>*& rpcListPic, Int& iSkipFrame,  Int& iPOCLastDisplay);

protected:
  Void  xGetNewPicBuffer  (TComSlice* pcSlice, TComPic*& rpcPic);

};// END CLASS DEFINITION TDecTop


#endif // __TDECTOP__

