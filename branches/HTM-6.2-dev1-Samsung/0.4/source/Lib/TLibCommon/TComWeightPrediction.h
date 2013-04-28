

/** \file     TComWeightPrediction.h
    \brief    weighting prediction class (header)
*/

#ifndef __TCOMWEIGHTPREDICTION__
#define __TCOMWEIGHTPREDICTION__


// Include files
#include "TComPic.h"
#include "TComMotionInfo.h"
#include "TComPattern.h"
#include "TComTrQuant.h"

#ifdef WEIGHT_PRED

// ====================================================================================================================
// Class definition
// ====================================================================================================================
/// weighting prediction class
class TComWeightPrediction
{
  wpScalingParam  m_wp0[3], m_wp1[3];
  Int             m_ibdi;

public:
  TComWeightPrediction();

  Void  getWpScaling( TComDataCU* pcCU, Int iRefIdx0, Int iRefIdx1, wpScalingParam *&wp0 , wpScalingParam *&wp1 , Int ibdi=(g_uiBitDepth+g_uiBitIncrement));

  Void  addWeightBi( TComYuv* pcYuvSrc0, TComYuv* pcYuvSrc1, UInt iPartUnitIdx, UInt iWidth, UInt iHeight, wpScalingParam *wp0, wpScalingParam *wp1, TComYuv* rpcYuvDst, Bool bRound=true );
  Void  addWeightUni( TComYuv* pcYuvSrc0, UInt iPartUnitIdx, UInt iWidth, UInt iHeight, wpScalingParam *wp0, TComYuv* rpcYuvDst );

  Void  xWeightedPredictionUni( TComDataCU* pcCU, TComYuv* pcYuvSrc, UInt uiPartAddr, Int iWidth, Int iHeight, RefPicList eRefPicList, TComYuv*& rpcYuvPred, Int iPartIdx );
  Void  xWeightedPredictionBi( TComDataCU* pcCU, TComYuv* pcYuvSrc0, TComYuv* pcYuvSrc1, Int iRefIdx0, Int iRefIdx1, UInt uiPartIdx, Int iWidth, Int iHeight, TComYuv* rpcYuvDst );

  __inline  Pel   xClip  ( Int x );
  __inline  Pel   weightBidir( Int w0, Pel P0, Int w1, Pel P1, Int round, Int shift, Int offset);
  __inline  Pel   weightUnidir( Int w0, Pel P0, Int round, Int shift, Int offset);

};


inline  Pel TComWeightPrediction::xClip( Int x )
{ 
  Int max = (Int)g_uiIBDI_MAX;
  Pel pel = (Pel)( (x < 0) ? 0 : (x > max) ? max : x );

  return( pel );
}

inline  Pel TComWeightPrediction::weightBidir( Int w0, Pel P0, Int w1, Pel P1, Int round, Int shift, Int offset)
{
  return xClip( ( (w0*P0 + w1*P1 + round) >> shift ) + offset );
}
inline  Pel TComWeightPrediction::weightUnidir( Int w0, Pel P0, Int round, Int shift, Int offset) 
{
  return xClip( ( (w0*P0 + round) >> shift ) + offset );
}

#endif  // WEIGHT_PRED

#endif


