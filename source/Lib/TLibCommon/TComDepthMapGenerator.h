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



/** \file     TComDepthMapGenerator.h
    \brief    depth map generator class (header)
*/


#ifndef __TCOM_DEPTH_MAP_GENERATOR__
#define __TCOM_DEPTH_MAP_GENERATOR__


#include "CommonDef.h"
#include "TComPic.h"
#include "TComPrediction.h"
#include "TComSlice.h"



class TComVPSAccess // would be better to have a real VPS buffer
{
public:
  TComVPSAccess ()  { clear(); }
  ~TComVPSAccess()  {}

  Void      clear   ()                                { ::memset( m_aacVPS, 0x00, sizeof( m_aacVPS ) ); m_uiActiceVPSId = 0;}
  Void      addVPS  ( TComVPS* pcVPS )                { m_aacVPS[ pcVPS->getVPSId() ] = pcVPS; }
  TComVPS*  getVPS  ( UInt uiVPSId )                  { return m_aacVPS[ uiVPSId ]; }
  TComVPS*  getActiveVPS  ( )                         { return m_aacVPS[ m_uiActiceVPSId ]; }
  Void      setActiveVPSId  ( UInt activeVPSId )      { m_uiActiceVPSId = activeVPSId; }
private:
  TComVPS*  m_aacVPS[ MAX_NUM_VPS ];
  UInt      m_uiActiceVPSId;
};


class TComSPSAccess // would be better to have a real SPS buffer
{
public:
  TComSPSAccess ()  { clear(); }
  ~TComSPSAccess()  {}

  Void      clear   ()                            { ::memset( m_aacActiveSPS, 0x00, sizeof( m_aacActiveSPS ) ); }
  Void      addSPS  ( TComSPS* pcSPS    )         { m_aacActiveSPS[ pcSPS->isDepth() ? 1 : 0 ][ pcSPS->getViewIndex() ] = pcSPS; }
  TComSPS*  getSPS  ( UInt     uiViewId, 
                      Bool     bIsDepth = false ) { return m_aacActiveSPS[ bIsDepth ? 1 : 0 ][ uiViewId ]; }

  UInt      getPdm  ()                            { if( m_aacActiveSPS[0][1] ) { return m_aacActiveSPS[0][1]->getPredDepthMapGeneration(); } return 0; }

private:
  TComSPS*  m_aacActiveSPS[ 2 ][ MAX_VIEW_NUM ];
};



class TComAUPicAccess // would be better to have a real decoded picture buffer
{
public:
  TComAUPicAccess () : m_iLastPoc( 0 ), m_uiMaxViewId( 0 ) { clear(); }
  ~TComAUPicAccess()                   {}

  Void      clear   ()                            { ::memset( m_aacCurrAUPics, 0x00, sizeof( m_aacCurrAUPics ) ); }
  Void      addPic    ( TComPic* pcPic    )         { if( m_iLastPoc != pcPic->getPOC() ) 
                                                      { 
                                                        clear(); 
                                                        m_iLastPoc = pcPic->getPOC(); 
                                                      } 
                                                      m_aacCurrAUPics[ pcPic->getSPS()->isDepth() ? 1 : 0 ][ pcPic->getSPS()->getViewIndex() ] = pcPic; 
                                                      if( pcPic->getSPS()->getViewIndex() > m_uiMaxViewId )
                                                      {
                                                        m_uiMaxViewId = pcPic->getSPS()->getViewIndex();
                                                      }
                                                    }
  TComPic*  getPic  ( UInt     uiViewId,
                      Bool     bIsDepth = false ) { return m_aacCurrAUPics[ bIsDepth ? 1 : 0 ][ uiViewId ]; }
  UInt      getMaxVId ()                            { return m_uiMaxViewId; }

private:
  Int       m_iLastPoc;
  UInt      m_uiMaxViewId;
  TComPic*  m_aacCurrAUPics[ 2 ][ MAX_VIEW_NUM ];
};



class TComDepthMapGenerator
{
public:
  TComDepthMapGenerator ();
  ~TComDepthMapGenerator();

  Void  create                ( Bool bDecoder, UInt uiPicWidth, UInt uiPicHeight, UInt uiMaxCUDepth, UInt uiMaxCUWidth, UInt uiMaxCUHeight, UInt uiOrgBitDepth, UInt uiSubSampExpX, UInt uiSubSampExpY );
  Void  destroy               ();

  Void  init( TComPrediction* pcPrediction, TComVPSAccess* pcVPSAccess, TComSPSAccess* pcSPSAccess, TComAUPicAccess* pcAUPicAccess );
  Void  uninit                ();

  Void  initViewComponent     ( TComPic*      pcPic );

  UInt  getSubSampExpX        ()                      { return m_uiSubSampExpX; }
  UInt  getSubSampExpY        ()                      { return m_uiSubSampExpY; }
  

  Bool  getPdmCandidate       ( TComDataCU*   pcCU, UInt uiPartIdx, RefPicList eRefPicList, Int iRefIdx, Int* paiPdmRefIdx, TComMv* pacPdmMv, DisInfo* pDInfo, Int* iPdm, Bool bMerge );

  
private:
  // general parameters
  Bool              m_bCreated;
  Bool              m_bInit;
  Bool              m_bDecoder;
  TComPrediction*   m_pcPrediction;
  TComVPSAccess*    m_pcVPSAccess;
  TComSPSAccess*    m_pcSPSAccess;
  TComAUPicAccess*  m_pcAUPicAccess;
  UInt              m_uiSubSampExpX;
  UInt              m_uiSubSampExpY;

  // conversion parameters
  UInt              m_uiCurrViewIndex;
  Bool              m_bPDMAvailable;
};




#endif // __TCOM_DEPTH_MAP_GENERATOR__




