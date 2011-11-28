

/** \file     TComDepthMapGenerator.h
    \brief    depth map generator class (header)
*/


#ifndef __TCOM_DEPTH_MAP_GENERATOR__
#define __TCOM_DEPTH_MAP_GENERATOR__


#include "CommonDef.h"
#include "TComPic.h"
#include "TComPrediction.h"
#include "TComSlice.h"



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
  UInt      getResPrd ()                          { if( m_aacActiveSPS[0][1] ) { return m_aacActiveSPS[0][1]->getMultiviewResPredMode  (); } return 0; }

private:
  TComSPS*  m_aacActiveSPS[ 2 ][ MAX_NUMBER_VIEWS ];
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
  TComPic*  m_aacCurrAUPics[ 2 ][ MAX_NUMBER_VIEWS ];
};



class TComDepthMapGenerator
{
public:
  TComDepthMapGenerator ();
  ~TComDepthMapGenerator();

  Void  create                ( Bool bDecoder, UInt uiPicWidth, UInt uiPicHeight, UInt uiMaxCUDepth, UInt uiMaxCUWidth, UInt uiMaxCUHeight, UInt uiOrgBitDepth );
  Void  destroy               ();

  Void  init                  ( TComPrediction* pcPrediction, TComSPSAccess* pcSPSAccess, TComAUPicAccess* pcAUPicAccess );
  Void  uninit                ();

  Void  initViewComponent     ( TComPic*      pcPic );
  Bool  predictDepthMap       ( TComPic*      pcPic );
  Void  updateDepthMap        ( TComPic*      pcPic );
  Void  dumpDepthMap          ( TComPic*      pcPic, char* pFilenameBase );

  Void  covertOrgDepthMap     ( TComPic*      pcPic );

  UInt  getBaseViewId         ( UInt          uiIdx ) { AOF( uiIdx < m_auiBaseIdList.size() );  return m_auiBaseIdList[uiIdx]; }
  Int   getDisparity          ( TComPic*      pcPic, Int iPosX, Int iPosY, UInt uiRefViewId );
  Int   getPdmMergeCandidate  ( TComDataCU*   pcCU, UInt uiPartIdx, Int* paiPdmRefIdx, TComMv* pacPdmMv );
  Bool  getPdmMvPred          ( TComDataCU*   pcCU, UInt uiPartIdx, RefPicList eRefPicList, Int iRefIdx, TComMv& rcMv, Bool bMerge );
  Bool  getIViewOrgDepthMvPred( TComDataCU*   pcCU, UInt uiPartIdx, RefPicList eRefPicList, Int iRefIdx, TComMv& rcMv );

  TComPrediction*   getPrediction ()  { return m_pcPrediction;  }
  TComSPSAccess*    getSPSAccess  ()  { return m_pcSPSAccess;   }
  TComAUPicAccess*  getAUPicAccess()  { return m_pcAUPicAccess; }

private:
  // picture operations
  Bool  xConvertDepthMapCurr2Ref  ( TComPic*    pcRef, TComPic* pcCur );
  Bool  xConvertDepthMapRef2Curr  ( TComPic*    pcCur, TComPic* pcRef );
  Bool  xPredictDepthMap          ( TComPic*    pcPic );
  Bool  xFillDepthMapHoles        ( TComPic*    pcPic );
  Void  xClearDepthMap            ( TComPic*    pcPic, Int      iVal  = PDM_UNDEFINED_DEPTH );
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
  TComSPSAccess*    m_pcSPSAccess;
  TComAUPicAccess*  m_pcAUPicAccess;
  UInt              m_uiMaxDepth;
  UInt              m_uiOrgDepthBitDepth;
  TComYuv**         m_ppcYuv;
  TComDataCU**      m_ppcCU;
  TComPicYuv        m_cTmpPic;

  // conversion parameters
  UInt              m_uiCurrViewId;
  Bool              m_bPDMAvailable;
  std::vector<UInt> m_auiBaseIdList;
  Int               m_aaiConvParsDisparity2VirtDepth[ MAX_NUMBER_VIEWS ][ 3 ];  // disparity      ==> virtual  depth   (0:scale, 1:add, 2:shift)
  Int               m_aaiConvParsVirtDepth2Disparity[ MAX_NUMBER_VIEWS ][ 3 ];  // virtual  depth ==> disparity        (0:scale, 1:add, 2:shift)
  Int               m_aaiConvParsOrigDepth2VirtDepth[ MAX_NUMBER_VIEWS ][ 3 ];  // original depth ==> virtual  depth   (0:scale, 1:add, 2:shift)
  Int               m_aaiConvParsVirtDepth2OrigDepth[ MAX_NUMBER_VIEWS ][ 3 ];  // virtual  depth ==> original depth   (0:scale, 1:add, 2:shift)

  // temporary arrays
  Bool              m_aabDepthSet[ MAX_CU_SIZE >> 2 ][ MAX_CU_SIZE >> 2 ];
  Int               m_aai4x4Depth[ MAX_CU_SIZE >> 2 ][ MAX_CU_SIZE >> 2 ];
};

#endif // __TCOM_DEPTH_MAP_GENERATOR__




