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


#if DEPTH_MAP_GENERATION
#if VIDYO_VPS_INTEGRATION
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
#endif

class TComSPSAccess // would be better to have a real SPS buffer
{
public:
  TComSPSAccess ()  { clear(); }
  ~TComSPSAccess()  {}

  Void      clear   ()                            { ::memset( m_aacActiveSPS, 0x00, sizeof( m_aacActiveSPS ) ); }
  Void      addSPS  ( TComSPS* pcSPS    )         { m_aacActiveSPS[ pcSPS->isDepth() ? 1 : 0 ][ pcSPS->getViewId() ] = pcSPS; }
  TComSPS*  getSPS  ( UInt     uiViewId, 
                      Bool     bIsDepth = false ) { return m_aacActiveSPS[ bIsDepth ? 1 : 0 ][ uiViewId ]; }

  UInt      getPdm  ()                            { if( m_aacActiveSPS[0][1] ) { return m_aacActiveSPS[0][1]->getPredDepthMapGeneration(); } return 0; }
#if H3D_IVRP
  UInt      getResPrd ()                          { if( m_aacActiveSPS[0][1] ) { return m_aacActiveSPS[0][1]->getMultiviewResPredMode  (); } return 0; }
#endif

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
                                                      m_aacCurrAUPics[ pcPic->getSPS()->isDepth() ? 1 : 0 ][ pcPic->getSPS()->getViewId() ] = pcPic; 
                                                      if( pcPic->getSPS()->getViewId() > m_uiMaxViewId )
                                                      {
                                                        m_uiMaxViewId = pcPic->getSPS()->getViewId();
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

#if VIDYO_VPS_INTEGRATION
  Void  init( TComPrediction* pcPrediction, TComVPSAccess* pcVPSAccess, TComSPSAccess* pcSPSAccess, TComAUPicAccess* pcAUPicAccess );
#endif
  Void  init                  ( TComPrediction* pcPrediction, TComSPSAccess* pcSPSAccess, TComAUPicAccess* pcAUPicAccess );
  Void  uninit                ();

  Void  initViewComponent     ( TComPic*      pcPic );
#if !H3D_NBDV
  Bool  predictDepthMap       ( TComPic*      pcPic );
  Void  updateDepthMap        ( TComPic*      pcPic );
  Void  dumpDepthMap          ( TComPic*      pcPic, char* pFilenameBase );
#endif

#if H3D_IVMP
  Void  covertOrgDepthMap     ( TComPic*      pcPic );
#endif

  UInt  getBaseViewId         ( UInt          uiIdx ) { AOF( uiIdx < m_auiBaseIdList.size() );  return m_auiBaseIdList[uiIdx]; }
  UInt  getSubSampExpX        ()                      { return m_uiSubSampExpX; }
  UInt  getSubSampExpY        ()                      { return m_uiSubSampExpY; }
  Int   getDisparity          ( TComPic*      pcPic, Int iPosX, Int iPosY, UInt uiRefViewId );
#if H3D_IVMP
#if H3D_NBDV
#if QC_AMVP_MRG_UNIFY_IVCAN_C0051
  Bool  getPdmCandidate       ( TComDataCU*   pcCU, UInt uiPartIdx, RefPicList eRefPicList, Int iRefIdx, Int* paiPdmRefIdx, TComMv* pacPdmMv, DisInfo* pDInfo, Int* iPdm, Bool bMerge );
#else
  Int   getPdmMergeCandidate  ( TComDataCU*   pcCU, UInt uiPartIdx, Int* paiPdmRefIdx, TComMv* pacPdmMv, DisInfo* pDInfo, Int* iPdm );
  Bool  getPdmMvPredDisCan    ( TComDataCU*   pcCU, UInt uiPartIdx, RefPicList eRefPicList, Int iRefIdx, TComMv& rcMv, DisInfo* pDInfo, Bool bMerge );
  Bool  getDisCanPdmMvPred    ( TComDataCU*   pcCU, UInt uiPartIdx, RefPicList eRefPicList, Int iRefIdx, TComMv& rcMv, DisInfo* pDInfo, Bool bMerge );
#endif
#else
  Int   getPdmMergeCandidate  ( TComDataCU*   pcCU, UInt uiPartIdx, Int* paiPdmRefIdx, TComMv* pacPdmMv );
  Bool  getPdmMvPred          ( TComDataCU*   pcCU, UInt uiPartIdx, RefPicList eRefPicList, Int iRefIdx, TComMv& rcMv, Bool bMerge );
#endif
  Bool  getIViewOrgDepthMvPred( TComDataCU*   pcCU, UInt uiPartIdx, RefPicList eRefPicList, Int iRefIdx, TComMv& rcMv );
#endif

  TComPrediction*   getPrediction ()  { return m_pcPrediction;  }
#if VIDYO_VPS_INTEGRATION
  TComVPSAccess*    getVPSAccess  ()  { return m_pcVPSAccess;   }
#endif
  TComSPSAccess*    getSPSAccess  ()  { return m_pcSPSAccess;   }
  TComAUPicAccess*  getAUPicAccess()  { return m_pcAUPicAccess; }

private:
  // picture operations
#if !H3D_NBDV
  Bool  xConvertDepthMapCurr2Ref  ( TComPic*    pcRef, TComPic* pcCur );
  Bool  xConvertDepthMapRef2Curr  ( TComPic*    pcCur, TComPic* pcRef );
  Bool  xPredictDepthMap          ( TComPic*    pcPic );
  Bool  xFillDepthMapHoles        ( TComPic*    pcPic );
  Void  xClearDepthMap            ( TComPic*    pcPic, Int      iVal  = PDM_UNDEFINED_DEPTH
#if PDM_REMOVE_DEPENDENCE
  ,
  Int forFirstNonBaseView = 0
#endif
  );
  Void  xSetChroma                ( TComPicYuv* pcPic, Int      iVal  );

  // CU operations  
  Void  xPredictCUDepthMap        ( TComDataCU* pcCU, UInt uiDepth, UInt uiAbsPartIdx );                            // CU depth map prediction
  Void  xIntraPredictCUDepthMap   ( TComDataCU* pcCU, TComYuv* pcCUDepthMap );                                      // CU intra prediction
  Void  xInterPredictCUDepthMap   ( TComDataCU* pcCU, TComYuv* pcCUDepthMap );                                      // CU inter prediction
  
  // PU and block operations  
  Void  xIntraPredictBlkDepthMap  ( TComDataCU* pcCU, TComYuv* pcCUDepthMap, UInt uiAbsPartIdx, UInt uiTrDepth );   // block intra prediction
  Void  xInterPredictPUDepthMap   ( TComDataCU* pcCU, TComYuv* pcCUDepthMap, UInt uiPartIdx );                      // PU inter prediction/update
  Void  xIViewPUDepthMapUpdate    ( TComDataCU* pcCU, TComYuv* pcCUDepthMap, UInt uiPartIdx );                      // PU interview update
  Void  xInterPUDepthMapUpdate    ( TComDataCU* pcCU, TComYuv* pcCUDepthMap, UInt uiPartIdx );                      // PU inter update
  Void  xInterPUDepthMapPrediction( TComDataCU* pcCU, TComYuv* pcCUDepthMap, UInt uiPartIdx );                      // PU inter prediction
  Bool  xGetPredDepth             ( TComDataCU* pcCU, UInt uiPartIdx, Int& riPrdDepth, Int* piPosX = 0, Int* piPosY = 0 );
#endif
  // conversion functions
  Int   xGetVirtDepthFromDisparity( UInt uiBaseId, Int iDisparity )
  {
    AOF( uiBaseId < m_uiCurrViewId );
    Int*   pPar = m_aaiConvParsDisparity2VirtDepth[ uiBaseId ];
    Int    iRes = ( pPar[ 0 ] * iDisparity + pPar[ 1 ] ) >> pPar[ 2 ];
    return iRes;
  }
  Int   xGetDisparityFromVirtDepth( UInt uiBaseId, Int iVirtDepth )
  {
    AOF( uiBaseId < m_uiCurrViewId );
    Int*   pPar = m_aaiConvParsVirtDepth2Disparity[ uiBaseId ];
    Int    iRes = ( pPar[ 0 ] * iVirtDepth + pPar[ 1 ] ) >> pPar[ 2 ];
    return iRes;
  }
  Int   xGetVirtDepthFromOrigDepth( UInt uiViewId, Int iOrigDepth )
  {
    AOF( uiViewId <= m_uiCurrViewId );
    Int*   pPar = m_aaiConvParsOrigDepth2VirtDepth[ uiViewId ];
    Int    iRes = ( pPar[ 0 ] * iOrigDepth + pPar[ 1 ] ) >> pPar[ 2 ];
    return iRes;
  }
  Int   xGetOrigDepthFromVirtDepth( UInt uiViewId, Int iVirtDepth )
  {
    AOF( uiViewId <= m_uiCurrViewId );
    Int*   pPar = m_aaiConvParsVirtDepth2OrigDepth[ uiViewId ];
    Int    iRes = ( pPar[ 0 ] * iVirtDepth + pPar[ 1 ] ) >> pPar[ 2 ];
    return iRes;
  }


private:
  // general parameters
  Bool              m_bCreated;
  Bool              m_bInit;
  Bool              m_bDecoder;
  TComPrediction*   m_pcPrediction;
#if VIDYO_VPS_INTEGRATION|QC_MVHEVC_B0046
  TComVPSAccess*    m_pcVPSAccess;
#endif
  TComSPSAccess*    m_pcSPSAccess;
  TComAUPicAccess*  m_pcAUPicAccess;
  UInt              m_uiMaxDepth;
  UInt              m_uiOrgDepthBitDepth;
  UInt              m_uiSubSampExpX;
  UInt              m_uiSubSampExpY;
  TComYuv**         m_ppcYuv;
  TComDataCU**      m_ppcCU;
  TComPicYuv        m_cTmpPic;

  // conversion parameters
  UInt              m_uiCurrViewId;
  Bool              m_bPDMAvailable;
  std::vector<UInt> m_auiBaseIdList;
  Int               m_aaiConvParsDisparity2VirtDepth[ MAX_VIEW_NUM ][ 3 ];  // disparity      ==> virtual  depth   (0:scale, 1:add, 2:shift)
  Int               m_aaiConvParsVirtDepth2Disparity[ MAX_VIEW_NUM ][ 3 ];  // virtual  depth ==> disparity        (0:scale, 1:add, 2:shift)
  Int               m_aaiConvParsOrigDepth2VirtDepth[ MAX_VIEW_NUM ][ 3 ];  // original depth ==> virtual  depth   (0:scale, 1:add, 2:shift)
  Int               m_aaiConvParsVirtDepth2OrigDepth[ MAX_VIEW_NUM ][ 3 ];  // virtual  depth ==> original depth   (0:scale, 1:add, 2:shift)

  // temporary arrays
  Bool              m_aabDepthSet[ MAX_CU_SIZE >> 2 ][ MAX_CU_SIZE >> 2 ];
  Int               m_aai4x4Depth[ MAX_CU_SIZE >> 2 ][ MAX_CU_SIZE >> 2 ];
};


#endif // DEPTH_MAP_GENERATION

#endif // __TCOM_DEPTH_MAP_GENERATOR__




