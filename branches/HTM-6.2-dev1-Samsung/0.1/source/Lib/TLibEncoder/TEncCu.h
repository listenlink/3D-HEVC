

/** \file     TEncCU.h
    \brief    CU encoder class (header)
*/

#ifndef __TENCCU__
#define __TENCCU__

// Include files
#include "../TLibCommon/CommonDef.h"
#include "../TLibCommon/TComYuv.h"
#include "../TLibCommon/TComPrediction.h"
#include "../TLibCommon/TComTrQuant.h"
#include "../TLibCommon/TComBitCounter.h"
#include "../TLibCommon/TComDataCU.h"

#include "TEncEntropy.h"
#include "TEncSearch.h"

class TEncTop;
class TEncSbac;
class TEncCavlc;
class TEncSlice;

// ====================================================================================================================
// Class definition
// ====================================================================================================================

/// CU encoder class
class TEncCu
{
private:
  
  TComDataCU**            m_ppcBestCU;      ///< Best CUs in each depth
  TComDataCU**            m_ppcTempCU;      ///< Temporary CUs in each depth
  UChar                   m_uhTotalDepth;
  
  TComYuv**               m_ppcPredYuvBest; ///< Best Prediction Yuv for each depth
  TComYuv**               m_ppcResiYuvBest; ///< Best Residual Yuv for each depth
  TComYuv**               m_ppcRecoYuvBest; ///< Best Reconstruction Yuv for each depth
  TComYuv**               m_ppcPredYuvTemp; ///< Temporary Prediction Yuv for each depth
  TComYuv**               m_ppcResiYuvTemp; ///< Temporary Residual Yuv for each depth
  TComYuv**               m_ppcRecoYuvTemp; ///< Temporary Reconstruction Yuv for each depth
  TComYuv**               m_ppcOrigYuv;     ///< Original Yuv for each depth
  TComYuv**               m_ppcResPredTmp;  ///< Temporary residual prediction for each depth
  
  //  Data : encoder control
  Int                     m_iQp;            ///< Last QP
  
  //  Access channel
  TEncCfg*                m_pcEncCfg;
  TEncTop*                m_pcEncTop;
  TComPrediction*         m_pcPrediction;
  TEncSearch*             m_pcPredSearch;
  TComTrQuant*            m_pcTrQuant;
  TComBitCounter*         m_pcBitCounter;
  TComRdCost*             m_pcRdCost;
  
  TEncEntropy*            m_pcEntropyCoder;
  TEncCavlc*              m_pcCavlcCoder;
  TEncSbac*               m_pcSbacCoder;
  TEncBinCABAC*           m_pcBinCABAC;
  
  // SBAC RD
  TEncSbac***             m_pppcRDSbacCoder;
  TEncSbac*               m_pcRDGoOnSbacCoder;
  Bool                    m_bUseSBACRD;
  
#if MW_MVI_SIGNALLING_MODE == 1
  UChar *m_puhDepthSaved;
  UChar *m_puhWidthSaved;
  UChar *m_puhHeightSaved;
#endif

public:
  /// copy parameters from encoder class
  Void  init                ( TEncTop* pcEncTop );
  
  /// create internal buffers
  Void  create              ( UChar uhTotalDepth, UInt iMaxWidth, UInt iMaxHeight );
  
  /// destroy internal buffers
  Void  destroy             ();
  
  /// CU analysis function
  Void  compressCU          ( TComDataCU*&  rpcCU );
  
  /// CU encoding function
  Void  encodeCU            ( TComDataCU*    pcCU, Bool bForceTerminate = false  );
  
  /// set QP value
  Void  setQpLast           ( Int iQp ) { m_iQp = iQp; }
  
protected:
  Void  xCompressCU         ( TComDataCU*& rpcBestCU, TComDataCU*& rpcTempCU, UInt uiDepth        );
  Void  xEncodeCU           ( TComDataCU*  pcCU, UInt uiAbsPartIdx,           UInt uiDepth        );
  
  Void  xCheckRDCostAMVPSkip( TComDataCU*& rpcBestCU, TComDataCU*& rpcTempCU                      );
  
#if SB_INTERVIEW_SKIP
  Void xCheckRDCostMerge2Nx2N( TComDataCU*& rpcBestCU, TComDataCU*& rpcTempCU, Bool bFullyRendered ) ;
#else
  Void  xCheckRDCostMerge2Nx2N( TComDataCU*& rpcBestCU, TComDataCU*& rpcTempCU                  );
#endif
  
  Void  xCheckRDCostSkip    ( TComDataCU*& rpcBestCU, TComDataCU*& rpcTempCU, Bool bBSkipRes      );
#if SB_INTERVIEW_SKIP
  Void xCheckRDCostInter( TComDataCU*& rpcBestCU, TComDataCU*& rpcTempCU, PartSize ePartSize, Bool bFullyRendered ) ;
#else
  Void  xCheckRDCostInter   ( TComDataCU*& rpcBestCU, TComDataCU*& rpcTempCU, PartSize ePartSize  );
#endif
  Void  xCheckRDCostIntra   ( TComDataCU*& rpcBestCU, TComDataCU*& rpcTempCU, PartSize ePartSize  );
  Void  xCheckBestMode      ( TComDataCU*& rpcBestCU, TComDataCU*& rpcTempCU, UChar uhDepth       );
  
  Void  xCopyAMVPInfo       ( AMVPInfo* pSrc, AMVPInfo* pDst );
  Void  xCopyYuv2Pic        ( TComPic* rpcPic, UInt uiCUAddr, UInt uiAbsZorderIdx, UInt uiDepth );
  Void  xCopyYuv2Tmp        ( UInt uhPartUnitIdx, UInt uiDepth );
  Void  xAddMVISignallingBits( TComDataCU* pcCU );
#if MW_MVI_SIGNALLING_MODE == 0
  Void  xCheckRDCostMvInheritance( TComDataCU*& rpcBestCU, TComDataCU*& rpcTempCU, UChar uhTextureModeDepth, Bool bRecursiveCall );
#elif MW_MVI_SIGNALLING_MODE == 1
  Void  xCheckRDCostMvInheritance( TComDataCU*& rpcBestCU, TComDataCU*& rpcTempCU, UChar uhTextureModeDepth, Bool bSkipResidual, Bool bRecursiveCall );
  Void  xSaveDepthWidthHeight( TComDataCU* pcCU );
  Void  xRestoreDepthWidthHeight( TComDataCU* pcCU );
#endif
};


#endif // __TENCMB__

