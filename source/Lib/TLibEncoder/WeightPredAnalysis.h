

/** \file     WeightedPredAnalysis.h
    \brief    encoder class
*/
#ifndef __WEIGHTPREDANALYSIS__
#define __WEIGHTPREDANALYSIS__

#include "../TLibCommon/TypeDef.h"
#include "../TLibCommon/TComSlice.h"

#ifdef WEIGHT_PRED

#ifdef MSYS_LINUX
  typedef long long LInt;
#else
  typedef __int64 LInt;
#endif

class  WeightPredAnalysis {

  wpScalingParam  m_wp[2][MAX_NUM_REF][3];

  Int64   xCalcDCValueSlice(TComSlice *slice, Pel *pPel,Int *iSample);
  Int64   xCalcACValueSlice(TComSlice *slice, Pel *pPel, Int64 iDC);
  Int64   xCalcDCValueUVSlice(TComSlice *slice, Pel *pPel, Int *iSample);
  Int64   xCalcACValueUVSlice(TComSlice *slice, Pel *pPel, Int64 iDC);
  Int64   xCalcSADvalueWPSlice(TComSlice *slice, Pel *pOrgPel, Pel *pRefPel, Int iDenom, Int iWeight, Int iOffset);

  Int64   xCalcDCValue(Pel *pPel, Int iWidth, Int iHeight, Int iStride);
  Int64   xCalcACValue(Pel *pPel, Int iWidth, Int iHeight, Int iStride, Int64 iDC);
  Int64   xCalcDCValueUV(Pel *pPel, Int iWidth, Int iHeight, Int iStride);
  Int64   xCalcACValueUV(Pel *pPel, Int iWidth, Int iHeight, Int iStride, Int64 iDC);
  Int64   xCalcSADvalueWP(Pel *pOrgPel, Pel *pRefPel, Int iWidth, Int iHeight, Int iOrgStride, Int iRefStride, Int iDenom, Int iWeight, Int iOffset);
  Int64   xCalcSADvalueWPUV(Pel *pOrgPel, Pel *pRefPel, Int iWidth, Int iHeight, Int iOrgStride, Int iRefStride, Int iDenom, Int iWeight, Int iOffset);
  Bool    xSelectWP(TComSlice *slice, wpScalingParam	weightPredTable[2][MAX_NUM_REF][3], Int iDenom);

public:
  WeightPredAnalysis();

  // WP analysis :
  Bool  xCalcACDCParamSlice(TComSlice *slice);
  Bool  xEstimateWPParamSlice(TComSlice *slice);
};

#endif	// WEIGHT_PRED

#endif	// __WEIGHTPREDANALYSIS__


