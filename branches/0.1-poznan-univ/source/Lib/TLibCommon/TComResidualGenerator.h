

/** \file     TComResidualGenerator.h
    \brief    residual picture generator class (header)
*/


#ifndef __TCOM_RESIDUAL_GENERATOR__
#define __TCOM_RESIDUAL_GENERATOR__


#include "CommonDef.h"
#include "TComPic.h"
#include "TComSlice.h"
#include "TComDepthMapGenerator.h"
#include "TComTrQuant.h"



class TComResidualGenerator
{
  enum { NUM_TMP_YUV_BUFFERS = 3 };

public:
  TComResidualGenerator ();
  ~TComResidualGenerator();

  Void  create                ( Bool bDecoder, UInt uiPicWidth, UInt uiPicHeight, UInt uiMaxCUDepth, UInt uiMaxCUWidth, UInt uiMaxCUHeight, UInt uiOrgBitDepth );
  Void  destroy               ();

  Void  init                  ( TComTrQuant*  pcTrQuant, TComDepthMapGenerator* pcDepthMapGenerator );
  Void  uninit                ();

  Void  initViewComponent     ( TComPic*      pcPic );
  Void  setRecResidualPic     ( TComPic*      pcPic );

  Bool  getResidualSamples    ( TComDataCU*   pcCU,  UInt uiPUIdx, TComYuv* pcYuv );
  Bool  getResidualSamples    ( TComPic* pcPic, UInt uiXPos, UInt uiYPos, UInt uiBlkWidth, UInt uiBlkHeight, TComYuv* pcYuv );

private:
  Void  xSetRecResidualPic    ( TComPic*      pcPic );
  Void  xSetRecResidualCU     ( TComDataCU*   pcCU,  UInt     uiDepth,      UInt uiAbsPartIdx );
  Void  xSetRecResidualIntraCU( TComDataCU*   pcCU,  TComYuv* pcCUResidual );
  Void  xSetRecResidualInterCU( TComDataCU*   pcCU,  TComYuv* pcCUResidual );
  Void  xClearIntViewResidual ( TComDataCU*   pcCU,  TComYuv* pcCUResidual, UInt uiPartIdx    );
  Void  xClearResidual        (                      TComYuv* pcCUResidual, UInt uiAbsPartIdx, UInt uiWidth, UInt uiHeight );

  Void  xSetPredResidualBlock ( TComPic*      pcPic, UInt uiBaseViewId, UInt uiXPos, UInt uiYPos, UInt uiBlkWidth, UInt uiBlkHeight, TComYuv* pcYuv );
  Bool  xIsNonZero            ( TComYuv*      pcYuv, UInt uiBlkWidth, UInt uiBlkHeight );

  Void  xDumpResidual         ( TComPic*      pcPic, char* pFilenameBase );

private:
  // general parameters
  Bool                    m_bCreated;
  Bool                    m_bInit;
  Bool                    m_bDecoder;
  TComTrQuant*            m_pcTrQuant;
  TComDepthMapGenerator*  m_pcDepthMapGenerator;
  TComSPSAccess*          m_pcSPSAccess;
  TComAUPicAccess*        m_pcAUPicAccess;
  UInt                    m_uiMaxDepth;
  UInt                    m_uiOrgDepthBitDepth;
  TComYuv**               m_ppcYuvTmp;
  TComYuv**               m_ppcYuv;
  TComDataCU**            m_ppcCU;
  TComPicYuv              m_cTmpPic;
};



#endif // __TCOM_RESIDUAL_GENERATOR__




