

/** \file     TEncPic.h
    \brief    GOP encoder class (header)
*/

#ifndef __TEncPic__
#define __TEncPic__

#include <stdlib.h>

#include "../TLibCommon/TComList.h"
#include "../TLibCommon/TComPic.h"
#include "../TLibCommon/TComBitStream.h"
#include "../TLibCommon/TComBitCounter.h"
#include "../TLibCommon/TComLoopFilter.h"
#include "../TLibCommon/TComDepthMapGenerator.h"
#include "../TLibCommon/TComResidualGenerator.h"
#include "TEncAdaptiveLoopFilter.h"
#include "TEncSlice.h"
#include "TEncEntropy.h"
#include "TEncCavlc.h"
#include "TEncSbac.h"

#include "TEncAnalyze.h"

#ifdef WEIGHT_PRED
#include "WeightPredAnalysis.h"
#endif

#if RVM_VCEGAM10
#include <vector>
#endif

class TEncTop;

// ====================================================================================================================
// Class definition
// ====================================================================================================================

/// GOP encoder class
class TEncPic
{
private:
  //  Data
  Int                     m_iHrchDepth;
  
  //  Access channel
  TEncTop*                m_pcEncTop;
  TEncCfg*                m_pcCfg;
  TEncSlice*              m_pcSliceEncoder;
  TComList<TComPic*>*     m_pcListPic;
  
  TEncEntropy*            m_pcEntropyCoder;
  TEncCavlc*              m_pcCavlcCoder;
  TEncSbac*               m_pcSbacCoder;
  TEncBinCABAC*           m_pcBinCABAC;
  TComLoopFilter*         m_pcLoopFilter;
  TComDepthMapGenerator*  m_pcDepthMapGenerator;
  TComResidualGenerator*  m_pcResidualGenerator;
  
  // Adaptive Loop filter
  TEncAdaptiveLoopFilter* m_pcAdaptiveLoopFilter;
  //--Adaptive Loop filter
#if MTK_SAO
  TEncSampleAdaptiveOffset*  m_pcSAO;
#endif
  TComBitCounter*         m_pcBitCounter;
  
  TComRdCost*             m_pcRdCost;                           ///< RD cost computation
  
#if DCM_DECODING_REFRESH
  // clean decoding refresh
  Bool                    m_bRefreshPending;
  UInt                    m_uiPOCCDR;
#endif
  UInt*                   m_uiStoredStartCUAddrForEncodingSlice;
  UInt*                   m_uiStoredStartCUAddrForEncodingEntropySlice;

// #if MTK_NONCROSS_INLOOP_FILTER
//   UInt                    m_uiILSliceCount;
//   UInt*                   m_puiILSliceStartLCU;
//   UInt*                   m_puiILSliceEndLCU;
// #endif

#if RVM_VCEGAM10
  std::vector<Int> m_vRVM_RP;
#endif

public:
  TEncPic();
  virtual ~TEncPic();
  
  Void  create      ( Int iWidth, Int iHeight, UInt iMaxCUWidth, UInt iMaxCUHeight );
  Void  destroy     ();
  
  Void  init        ( TEncTop* pcTEncTop );
  Void compressPic( TComBitstream* pcBitstreamOut, TComPicYuv cPicOrg, TComPic* pcPic, TComPicYuv* pcPicYuvRecOut,
                 TComPic* pcOrgRefList[2][MAX_REF_PIC_NUM], Bool&  rbSeqFirst, TComList<TComPic*>& rcListPic  );
  TComList<TComPic*>*   getListPic()      { return m_pcListPic; }
  
  Void  preLoopFilterPicAll  ( TComPic* pcPic, UInt64& ruiDist, UInt64& ruiBits );

  
protected:
#if DCM_DECODING_REFRESH
  NalUnitType getNalUnitType( UInt uiPOCCurr );
#endif

  Void  xCalculateAddPSNR ( TComPic* pcPic, TComPicYuv* pcPicD, UInt uiBits, Double dEncTime );
  
  UInt64 xFindDistortionFrame (TComPicYuv* pcPic0, TComPicYuv* pcPic1);

#if RVM_VCEGAM10
  Double xCalculateRVM();
#endif
};// END CLASS DEFINITION TEncPic


#endif // __TEncPic__

