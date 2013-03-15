

/** \file     WeightedPredAnalysis.cpp
    \brief    encoder class
*/

#include "../TLibCommon/TypeDef.h"
#include "../TLibCommon/TComSlice.h"
#include "../TLibCommon/TComPic.h"
#include "../TLibCommon/TComPicYuv.h"
#include "WeightPredAnalysis.h"

#ifdef WEIGHT_PRED

#define ABS(a)    ((a) < 0 ? - (a) : (a))

WeightPredAnalysis::WeightPredAnalysis()
{
  for ( Int e=0 ; e<2 ; e++ )
    for ( Int iRefIdx=0 ; iRefIdx<MAX_NUM_REF ; iRefIdx++ ) {
      for ( int comp=0 ; comp<3 ;comp++ ) {
        wpScalingParam  *pwp   = &(m_wp[e][iRefIdx][comp]);
        pwp->bPresentFlag      = false;
        pwp->uiLog2WeightDenom = 0;
        pwp->uiLog2WeightDenom = 0;
        pwp->iWeight           = 1;
        pwp->iOffset           = 0;
      }
    }
}

Bool  WeightPredAnalysis::xCalcACDCParamSlice(TComSlice *slice)
{
  //===== calculate AC/DC value =====
  //Int iPoc = slice->getPOC();
  TComPicYuv*   pPic = slice->getPic()->getPicYuvOrg();
  Int   iSample  = 0;

  // calculate DC/AC value for Y
  Pel*  pOrg    = pPic->getLumaAddr();
  Int64  iOrgDCY = xCalcDCValueSlice(slice, pOrg, &iSample);
  Int64  iOrgNormDCY = ((iOrgDCY+(iSample>>1)) / iSample);
  pOrg = pPic->getLumaAddr();
  Int64  iOrgACY  = xCalcACValueSlice(slice, pOrg, iOrgNormDCY);

  // calculate DC/AC value for Cb
  pOrg = pPic->getCbAddr();
  Int64  iOrgDCCb = xCalcDCValueUVSlice(slice, pOrg, &iSample);
  Int64  iOrgNormDCCb = ((iOrgDCCb+(iSample>>1)) / (iSample));
  pOrg = pPic->getCbAddr();
  Int64  iOrgACCb  = xCalcACValueUVSlice(slice, pOrg, iOrgNormDCCb);

  // calculate DC/AC value for Cr
  pOrg = pPic->getCrAddr();
  Int64  iOrgDCCr = xCalcDCValueUVSlice(slice, pOrg, &iSample);
  Int64  iOrgNormDCCr = ((iOrgDCCr+(iSample>>1)) / (iSample));
  pOrg = pPic->getCrAddr();
  Int64  iOrgACCr  = xCalcACValueUVSlice(slice, pOrg, iOrgNormDCCr);

  wpACDCParam weightACDCParam[3];
  weightACDCParam[0].iAC = iOrgACY;
  weightACDCParam[0].iDC = iOrgNormDCY;
  weightACDCParam[1].iAC = iOrgACCb;
  weightACDCParam[1].iDC = iOrgNormDCCb;
  weightACDCParam[2].iAC = iOrgACCr;
  weightACDCParam[2].iDC = iOrgNormDCCr;

  slice->setWpAcDcParam(weightACDCParam);
  return (true);
}
Bool  WeightPredAnalysis::xEstimateWPParamSlice(TComSlice *slice)
{
#define wpTable	m_wp //wpScalingParam	weightPredTable[2][MAX_NUM_REF][3];

  //Int iPoc = slice->getPOC();
  Int iDenom  = 6;
  Int iDefaultWeight = ((Int)1<<iDenom);
  Int iRealDenom = iDenom + (g_uiBitDepth+g_uiBitIncrement-8);
  Int iRealOffset = ((Int)1<<(iRealDenom-1));

  if(slice->getNumRefIdx(REF_PIC_LIST_0)>3)
  {
    iDenom  = 7;
    iDefaultWeight = ((Int)1<<iDenom);
    iRealDenom = iDenom + (g_uiBitDepth+g_uiBitIncrement-8);
    iRealOffset = ((Int)1<<(iRealDenom-1));
  }
  
  Int iNumPredDir = slice->isInterP() ? 1 : 2;
  for ( Int iRefList = 0; iRefList < iNumPredDir; iRefList++ )
  {
    RefPicList  eRefPicList = ( iRefList ? REF_PIC_LIST_1 : REF_PIC_LIST_0 );
    for ( Int iRefIdxTemp = 0; iRefIdxTemp < slice->getNumRefIdx(eRefPicList); iRefIdxTemp++ )
    {
      wpACDCParam *CurrWeightACDCParam, *RefWeightACDCParam;
      slice->getWpAcDcParam(CurrWeightACDCParam);
      slice->getRefPic(eRefPicList, iRefIdxTemp)->getSlice(0)->getWpAcDcParam(RefWeightACDCParam);

	  //Int64 iSADWP=0, iSADnoWP=0;
      for ( Int iComp = 0; iComp < 3; iComp++ )
      {
        //Int irefPoc = slice->getRefPOC(eRefPicList, iRefIdxTemp);
        // current frame
        Int64 iCurrDC = CurrWeightACDCParam[iComp].iDC;
        Int64 iCurrAC = CurrWeightACDCParam[iComp].iAC;
        // reference frame
        Int64 iRefDC = RefWeightACDCParam[iComp].iDC;
        Int64 iRefAC = RefWeightACDCParam[iComp].iAC;

        // calculating iWeight and iOffset params (these two codes are only "Double")
        Double dWeight = (iRefAC==0) ? (Double)1.0 : ( (Double)(iCurrAC) / (Double)iRefAC);
        Int iWeight = (Int)( 0.5 + dWeight * (Double)(1<<iDenom) );
        Int iOffset = (Int)( ((iCurrDC<<iDenom) - ((Int64)iWeight * iRefDC) + (Int64)iRealOffset) >> iRealDenom );

        wpTable[iRefList][iRefIdxTemp][iComp].bPresentFlag = true;
        wpTable[iRefList][iRefIdxTemp][iComp].iWeight = (Int)iWeight;
        wpTable[iRefList][iRefIdxTemp][iComp].iOffset = (Int)iOffset;
        wpTable[iRefList][iRefIdxTemp][iComp].uiLog2WeightDenom = (Int)iDenom;
      }
    }
  }

  // selecting whether WP is used, or not
  xSelectWP(slice, wpTable, iDenom);
  
  slice->setWpScaling( wpTable );

  return (true);
}
Bool WeightPredAnalysis::xSelectWP(TComSlice *slice, wpScalingParam	weightPredTable[2][MAX_NUM_REF][3], Int iDenom)
{
  //Int iPoc = slice->getPOC();
  TComPicYuv*   pPic = slice->getPic()->getPicYuvOrg();
  Int iWidth  = pPic->getWidth();
  Int iHeight = pPic->getHeight();
  Int iDefaultWeight = ((Int)1<<iDenom);
  Int iNumPredDir = slice->isInterP() ? 1 : 2;

  for ( Int iRefList = 0; iRefList < iNumPredDir; iRefList++ )
  {
    Int64 iSADWP = 0, iSADnoWP = 0;
    RefPicList  eRefPicList = ( iRefList ? REF_PIC_LIST_1 : REF_PIC_LIST_0 );
    for ( Int iRefIdxTemp = 0; iRefIdxTemp < slice->getNumRefIdx(eRefPicList); iRefIdxTemp++ )
    {
      Pel*  pOrg    = pPic->getLumaAddr();
      Pel*  pRef    = slice->getRefPic(eRefPicList, iRefIdxTemp)->getPicYuvRec()->getLumaAddr();
      Int   iOrgStride = pPic->getStride();
      Int   iRefStride = slice->getRefPic(eRefPicList, iRefIdxTemp)->getPicYuvRec()->getStride();

      // calculate SAD costs with/without wp for luma
      iSADWP   = this->xCalcSADvalueWP(pOrg, pRef, iWidth, iHeight, iOrgStride, iRefStride, iDenom, weightPredTable[iRefList][iRefIdxTemp][0].iWeight, weightPredTable[iRefList][iRefIdxTemp][0].iOffset);
      iSADnoWP = this->xCalcSADvalueWP(pOrg, pRef, iWidth, iHeight, iOrgStride, iRefStride, iDenom, iDefaultWeight, 0);

      pOrg = pPic->getCbAddr();
      pRef = slice->getRefPic(eRefPicList, iRefIdxTemp)->getPicYuvRec()->getCbAddr();
      iOrgStride = pPic->getCStride();
      iRefStride = slice->getRefPic(eRefPicList, iRefIdxTemp)->getPicYuvRec()->getCStride();

      // calculate SAD costs with/without wp for chroma cb
      iSADWP   += this->xCalcSADvalueWPUV(pOrg, pRef, iWidth>>1, iHeight>>1, iOrgStride, iRefStride, iDenom, weightPredTable[iRefList][iRefIdxTemp][1].iWeight, weightPredTable[iRefList][iRefIdxTemp][1].iOffset);
      iSADnoWP += this->xCalcSADvalueWPUV(pOrg, pRef, iWidth>>1, iHeight>>1, iOrgStride, iRefStride, iDenom, iDefaultWeight, 0);

      pOrg = pPic->getCrAddr();
      pRef = slice->getRefPic(eRefPicList, iRefIdxTemp)->getPicYuvRec()->getCrAddr();

      // calculate SAD costs with/without wp for chroma cr
      iSADWP   += this->xCalcSADvalueWPUV(pOrg, pRef, iWidth>>1, iHeight>>1, iOrgStride, iRefStride, iDenom, weightPredTable[iRefList][iRefIdxTemp][2].iWeight, weightPredTable[iRefList][iRefIdxTemp][2].iOffset);
      iSADnoWP += this->xCalcSADvalueWPUV(pOrg, pRef, iWidth>>1, iHeight>>1, iOrgStride, iRefStride, iDenom, iDefaultWeight, 0);

      if(iSADWP >= iSADnoWP)
      {
        for ( Int iComp = 0; iComp < 3; iComp++ )
        {
          weightPredTable[iRefList][iRefIdxTemp][iComp].bPresentFlag = false;
          weightPredTable[iRefList][iRefIdxTemp][iComp].iOffset = (Int)0;
          weightPredTable[iRefList][iRefIdxTemp][iComp].iWeight = (Int)iDefaultWeight;
          weightPredTable[iRefList][iRefIdxTemp][iComp].uiLog2WeightDenom = (Int)iDenom;
        }
      }
    }
  }
  return (true);
}
Int64 WeightPredAnalysis::xCalcDCValueSlice(TComSlice *slice, Pel *pPel, Int *iSample)
{
  TComPicYuv* pPic = slice->getPic()->getPicYuvOrg();
  Int iStride = pPic->getStride();

  *iSample = 0;
  Int iWidth  = pPic->getWidth();
  Int iHeight = pPic->getHeight();
  *iSample = iWidth*iHeight;
  Int64 iDC = xCalcDCValue(pPel, iWidth, iHeight, iStride);

  return (iDC);
}
Int64 WeightPredAnalysis::xCalcACValueSlice(TComSlice *slice, Pel *pPel, Int64 iDC)
{
  TComPicYuv* pPic = slice->getPic()->getPicYuvOrg();
  Int iStride = pPic->getStride();

  Int iWidth  = pPic->getWidth();
  Int iHeight = pPic->getHeight();
  Int64 iAC = xCalcACValue(pPel, iWidth, iHeight, iStride, iDC);

  return (iAC);
}

Int64 WeightPredAnalysis::xCalcDCValueUVSlice(TComSlice *slice, Pel *pPel, Int *iSample)
{
  TComPicYuv* pPic = slice->getPic()->getPicYuvOrg();
  Int iCStride = pPic->getCStride();

  *iSample = 0;
  Int iWidth  = pPic->getWidth()>>1;
  Int iHeight = pPic->getHeight()>>1;
  *iSample = iWidth*iHeight;
  Int64 iDC = xCalcDCValueUV(pPel, iWidth, iHeight, iCStride);

  return (iDC);
}
Int64 WeightPredAnalysis::xCalcACValueUVSlice(TComSlice *slice, Pel *pPel, Int64 iDC)
{
  TComPicYuv* pPic = slice->getPic()->getPicYuvOrg();
  Int iCStride = pPic->getCStride();

  Int iWidth  = pPic->getWidth()>>1;
  Int iHeight = pPic->getHeight()>>1;
  Int64 iAC = xCalcACValueUV(pPel, iWidth, iHeight, iCStride, iDC);

  return (iAC);
}
Int64 WeightPredAnalysis::xCalcDCValue(Pel *pPel, Int iWidth, Int iHeight, Int iStride)
{
  Int x, y;
  Int64 iDC = 0;
  for( y = 0; y < iHeight; y++ )
  {
    for( x = 0; x < iWidth; x++ )
    {
      iDC += (Int)( pPel[x] );
    }
    pPel += iStride;
  }
  return (iDC);
}
Int64 WeightPredAnalysis::xCalcACValue(Pel *pPel, Int iWidth, Int iHeight, Int iStride, Int64 iDC)
{
  Int x, y;
  Int64 iAC = 0;
  for( y = 0; y < iHeight; y++ )
  {
    for( x = 0; x < iWidth; x++ )
    {
      iAC += abs( (Int)pPel[x] - (Int)iDC);
    }
    pPel += iStride;
  }
  return (iAC);
}
Int64 WeightPredAnalysis::xCalcDCValueUV(Pel *pPel, Int iWidth, Int iHeight, Int iStride)
{
  Int x, y;
  Int64 iDC = 0;

  for( y = 0; y < iHeight; y++ )
  {
    for( x = 0; x < iWidth; x++ )
    {
      iDC += (Int)( pPel[x]);
    }
    pPel += iStride;
  }
  return (iDC);
}
Int64 WeightPredAnalysis::xCalcACValueUV(Pel *pPel, Int iWidth, Int iHeight, Int iStride, Int64 iDC)
{
  Int x, y;
  Int64 iAC = 0;

  for( y = 0; y < iHeight; y++ )
  {
    for( x = 0; x < iWidth; x++ )
    {
      iAC += abs( (Int)pPel[x] - (Int)iDC);
    }
    pPel += iStride;
  }
  return (iAC);
}
Int64 WeightPredAnalysis::xCalcSADvalueWP(Pel *pOrgPel, Pel *pRefPel, Int iWidth, Int iHeight, Int iOrgStride, Int iRefStride, Int iDenom, Int iWeight, Int iOffset)
{
  Int x, y;
  Int64 iSAD = 0;
  Int64 iSize   = iWidth*iHeight;
  Int64 iRealDenom = iDenom + (g_uiBitDepth+g_uiBitIncrement-8);
  for( y = 0; y < iHeight; y++ )
  {
    for( x = 0; x < iWidth; x++ )
    {
      iSAD += ABS(( ((Int64)pOrgPel[x]<<(Int64)iDenom) - ( (Int64)pRefPel[x] * (Int64)iWeight + ((Int64)iOffset<<iRealDenom) ) ) );
    }
    pOrgPel += iOrgStride;
    pRefPel += iRefStride;
  }

  return (iSAD/iSize);
}
Int64 WeightPredAnalysis::xCalcSADvalueWPUV(Pel *pOrgPel, Pel *pRefPel, Int iWidth, Int iHeight, Int iOrgStride, Int iRefStride, Int iDenom, Int iWeight, Int iOffset)
{
  Int x, y;
  //Int64 itmpSAD = 0;
  Int64	iSAD = 0;
  Int64 iSize   = iWidth*iHeight;
  Int64 iRealDenom = iDenom + (g_uiBitDepth+g_uiBitIncrement-8);

  for( y = 0; y < iHeight; y++ )
  {
    for( x = 0; x < iWidth; x++ )
    {
      iSAD += ABS(( ((Int64)pOrgPel[x]<<(Int64)iDenom) - ( (Int64)pRefPel[x] * (Int64)iWeight + ((Int64)iOffset<<iRealDenom) ) ) );
    }
    pOrgPel += iOrgStride;
    pRefPel += iRefStride;
  }

  return (iSAD/iSize);
}

#endif	// WEIGHT_PRED

