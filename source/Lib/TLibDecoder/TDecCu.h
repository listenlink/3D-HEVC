

/** \file     TDecCu.h
    \brief    CU decoder class (header)
*/

#ifndef __TDECCU__
#define __TDECCU__

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "../TLibCommon/TComTrQuant.h"
#include "../TLibCommon/TComPrediction.h"
#include "TDecEntropy.h"

// ====================================================================================================================
// Class definition
// ====================================================================================================================

/// CU decoder class
class TDecCu
{
private:
  UInt                m_uiMaxDepth;       ///< max. number of depth
  TComYuv**           m_ppcYuvResi;       ///< array of residual buffer
  TComYuv**           m_ppcYuvReco;       ///< array of prediction & reconstruction buffer
  TComYuv**           m_ppcYuvResPred;    ///< residual prediction buffer
  TComDataCU**        m_ppcCU;            ///< CU data array
  
  // access channel
  TComTrQuant*        m_pcTrQuant;
  TComPrediction*     m_pcPrediction;
  TDecEntropy*        m_pcEntropyDecoder;
  
public:
  TDecCu();
  virtual ~TDecCu();
  
  /// initialize access channels
  Void  init                    ( TDecEntropy* pcEntropyDecoder, TComTrQuant* pcTrQuant, TComPrediction* pcPrediction );
  
  /// create internal buffers
  Void  create                  ( UInt uiMaxDepth, UInt uiMaxWidth, UInt uiMaxHeight );
  
  /// destroy internal buffers
  Void  destroy                 ();
  
  /// decode CU information
  Void  decodeCU                ( TComDataCU* pcCU, UInt& ruiIsLast );
  
  /// reconstruct CU information
  Void  decompressCU            ( TComDataCU* pcCU );
  
protected:
  
  Void xDecodeCU                ( TComDataCU* pcCU,                       UInt uiAbsPartIdx, UInt uiDepth );
  Void xDecompressCU            ( TComDataCU* pcCU, TComDataCU* pcCUCur,  UInt uiAbsPartIdx, UInt uiDepth );
  
  Void xReconInter              ( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth );
  
  Void  xReconIntraQT           ( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth );
  Void  xIntraRecLumaBlk        ( TComDataCU* pcCU, UInt uiTrDepth, UInt uiAbsPartIdx, TComYuv* pcRecoYuv, TComYuv* pcPredYuv, TComYuv* pcResiYuv );
  Void  xIntraRecChromaBlk      ( TComDataCU* pcCU, UInt uiTrDepth, UInt uiAbsPartIdx, TComYuv* pcRecoYuv, TComYuv* pcPredYuv, TComYuv* pcResiYuv, UInt uiChromaId );
  Void  xIntraRecQT             ( TComDataCU* pcCU, UInt uiTrDepth, UInt uiAbsPartIdx, TComYuv* pcRecoYuv, TComYuv* pcPredYuv, TComYuv* pcResiYuv );
  
  Void xDecodeInterTexture      ( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth );
  Void xDecodeIntraTexture      ( TComDataCU* pcCU, UInt uiPartIdx, Pel* piReco, Pel* pPred, Pel* piResi, UInt uiStride, TCoeff* pCoeff, UInt uiWidth, UInt uiHeight, UInt uiCurrDepth );
  Void xRecurIntraInvTransChroma( TComDataCU* pcCU, UInt uiAbsPartIdx, Pel* piResi, Pel* piPred, Pel* piReco, UInt uiStride, TCoeff* piCoeff, UInt uiWidth, UInt uiHeight, UInt uiTrMode, UInt uiCurrTrMode, TextType eText );
  
  Void xCopyToPic               ( TComDataCU* pcCU, TComPic* pcPic, UInt uiZorderIdx, UInt uiDepth );

#if LM_CHROMA
  Void  xIntraLumaRecQT         ( TComDataCU* pcCU, UInt uiTrDepth, UInt uiAbsPartIdx, TComYuv* pcRecoYuv, TComYuv* pcPredYuv, TComYuv* pcResiYuv );
  Void  xIntraChromaRecQT       ( TComDataCU* pcCU, UInt uiTrDepth, UInt uiAbsPartIdx, TComYuv* pcRecoYuv, TComYuv* pcPredYuv, TComYuv* pcResiYuv );
#endif
};

#endif

