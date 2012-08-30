/* The copyright in this software is being made available under the BSD
 * License, included below. This software may be subject to other third party
 * and contributor rights, including patent rights, and no such rights are
 * granted under this license.  
 *
 * Copyright (c) 2010-2012, ITU/ISO/IEC
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
 *  * Neither the name of the ITU/ISO/IEC nor the names of its contributors may
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

/** \file     TComRdCost.h
    \brief    RD cost computation classes (header)
*/

#ifndef __TCOMRDCOST__
#define __TCOMRDCOST__


#include "CommonDef.h"
#include "TComPattern.h"
#include "TComMv.h"

#include "TComRdCost.h"
#include "TComSlice.h"
#include "TComRdCostWeightPrediction.h"
#include "../TLibRenderer/TRenModel.h"

//! \ingroup TLibCommon
//! \{

#define FIX203 1

class DistParam;
class TComPattern;
class TComRdCost; 

// ====================================================================================================================
// Type definition
// ====================================================================================================================

// for function pointer
typedef UInt (*FpDistFunc) (DistParam*);

#if HHI_VSO
typedef Dist (TComRdCost::*FpDistFuncVSO) ( Int, Int, Pel*, Int, Pel*, Int, UInt, UInt, Bool );
#endif
// ====================================================================================================================
// Class definition
// ====================================================================================================================

/// distortion parameter class
class DistParam
{
public:
  Pel*  pOrg;
  Pel*  pCur;
  Int   iStrideOrg;
  Int   iStrideCur;
#if SAIT_VSO_EST_A0033
  Pel*  pVirRec;
  Pel*  pVirOrg;
  Int   iStrideVir;
#endif
  Int   iRows;
  Int   iCols;
  Int   iStep;
  FpDistFunc DistFunc;

#if HHI_INTERVIEW_SKIP
  Pel*  pUsed;
  Int   iStrideUsed;
#endif
  Bool            bApplyWeight;     // whether weithed prediction is used or not
  wpScalingParam  *wpCur;           // weithed prediction scaling parameters for current ref
  UInt            uiComp;           // uiComp = 0 (luma Y), 1 (chroma U), 2 (chroma V)

#if NS_HAD
  Bool            bUseNSHAD;
#endif

  // (vertical) subsampling shift (for reducing complexity)
  // - 0 = no subsampling, 1 = even rows, 2 = every 4th, etc.
  Int   iSubShift;
  
  DistParam()
  {
    pOrg = NULL;
    pCur = NULL;
#if SAIT_VSO_EST_A0033
    pVirRec = NULL;
    pVirOrg = NULL;;
    iStrideVir = 0;
#endif
    iStrideOrg = 0;
    iStrideCur = 0;
    iRows = 0;
    iCols = 0;
    iStep = 1;
    DistFunc = NULL;
    iSubShift = 0;
#if HHI_INTERVIEW_SKIP
    pUsed       = 0;
    iStrideUsed = 0;
#endif
#if NS_HAD
    bUseNSHAD = false;
#endif
  }
};

/// RD cost computation class
class TComRdCost
  : public TComRdCostWeightPrediction
{
private:
  // for distortion
  Int                     m_iBlkWidth;
  Int                     m_iBlkHeight;
  
#if AMP_SAD
  FpDistFunc              m_afpDistortFunc[64]; // [eDFunc]
#else  
  FpDistFunc              m_afpDistortFunc[33]; // [eDFunc]
#endif  
  
#if WEIGHTED_CHROMA_DISTORTION
  Double                  m_chromaDistortionWeight;   
#endif
  Double                  m_dLambda;
  Double                  m_sqrtLambda;
  UInt                    m_uiLambdaMotionSAD;
  UInt                    m_uiLambdaMotionSSE;
  Double                  m_dFrameLambda;
#if SAIT_VSO_EST_A0033
  static Double           m_dDisparityCoeff;
#endif
  
#if HHI_INTERVIEW_SKIP_LAMBDA_SCALE
  Double                  m_dLambdaScale ;
#endif
  // for motion cost
#if FIX203
  TComMv                  m_mvPredictor;
#else
  UInt*                   m_puiComponentCostOriginP;
  UInt*                   m_puiComponentCost;
  UInt*                   m_puiVerCost;
  UInt*                   m_puiHorCost;
#endif
  UInt                    m_uiCost;
  Int                     m_iCostScale;
#if !FIX203
  Int                     m_iSearchLimit;
#endif
  
  Bool                    m_bUseMultiviewReg;
  UInt                    m_uiLambdaMVReg;
  UInt                    m_uiLambdaMVRegSAD;
  UInt                    m_uiLambdaMVRegSSE;
  UInt*                   m_puiMultiviewRegCostHorOrgP;
  UInt*                   m_puiMultiviewRegCostVerOrgP;
  UInt*                   m_puiMultiviewRegCostHor;
  UInt*                   m_puiMultiviewRegCostVer;
  UInt*                   m_puiHorRegCost;
  UInt*                   m_puiVerRegCost;
  TComMv                  m_cMultiviewOrgMvPred;
  
public:
  TComRdCost();
  virtual ~TComRdCost();
  
  Double  calcRdCost  ( UInt   uiBits, UInt   uiDistortion, Bool bFlag = false, DFunc eDFunc = DF_DEFAULT );
  Double  calcRdCost64( UInt64 uiBits, UInt64 uiDistortion, Bool bFlag = false, DFunc eDFunc = DF_DEFAULT );
  
#if WEIGHTED_CHROMA_DISTORTION
  Void    setChromaDistortionWeight      ( Double chromaDistortionWeight) { m_chromaDistortionWeight = chromaDistortionWeight; };
#endif
  Void    setLambda      ( Double dLambda );
#if HHI_INTER_VIEW_MOTION_PRED
  Void    setLambdaMVReg ( Double dLambda );
#endif
  Void    setFrameLambda ( Double dLambda ) { m_dFrameLambda = dLambda; }
  
#if SAIT_VSO_EST_A0033
  Void    setDisparityCoeff( Double dDisparityCoeff ) { m_dDisparityCoeff = dDisparityCoeff; }
  Double  getDisparityCoeff()                         { return m_dDisparityCoeff; }
#endif

#if HHI_INTERVIEW_SKIP_LAMBDA_SCALE
  Void   setLambdaScale  ( Double dLambdaScale) { m_dLambdaScale = dLambdaScale; }
  Double   getLambdaScale  ( ) { return m_dLambdaScale ; }
#endif
  Double  getSqrtLambda ()   { return m_sqrtLambda; }
  
  // Distortion Functions
  Void    init();
  
  Void    setDistParam( UInt uiBlkWidth, UInt uiBlkHeight, DFunc eDFunc, DistParam& rcDistParam );
  Void    setDistParam( TComPattern* pcPatternKey, Pel* piRefY, Int iRefStride,            DistParam& rcDistParam );
#if NS_HAD
  Void    setDistParam( TComPattern* pcPatternKey, Pel* piRefY, Int iRefStride, Int iStep, DistParam& rcDistParam, Bool bHADME=false, Bool bUseNSHAD=false );
  Void    setDistParam( DistParam& rcDP, Pel* p1, Int iStride1, Pel* p2, Int iStride2, Int iWidth, Int iHeight, Bool bHadamard = false, Bool bUseNSHAD=false );
#else
  Void    setDistParam( TComPattern* pcPatternKey, Pel* piRefY, Int iRefStride, Int iStep, DistParam& rcDistParam, Bool bHADME=false );
  Void    setDistParam( DistParam& rcDP, Pel* p1, Int iStride1, Pel* p2, Int iStride2, Int iWidth, Int iHeight, Bool bHadamard = false );
#endif
  
  UInt    calcHAD         ( Pel* pi0, Int iStride0, Pel* pi1, Int iStride1, Int iWidth, Int iHeight );
  
  // for motion cost
#if !FIX203
  Void    initRateDistortionModel( Int iSubPelSearchLimit );
  Void    xUninit();
#endif
  UInt    xGetComponentBits( Int iVal );
  Void    getMotionCost( Bool bSad, Int iAdd ) 
{ 
     m_uiCost = (bSad ? m_uiLambdaMotionSAD + iAdd : m_uiLambdaMotionSSE + iAdd); 
    m_uiLambdaMVReg = ( bSad ? m_uiLambdaMVRegSAD         : m_uiLambdaMVRegSSE         );

   }
  Void    setPredictor( TComMv& rcMv )
  {
#if FIX203
    m_mvPredictor = rcMv;
#else
    m_puiHorCost = m_puiComponentCost - rcMv.getHor();
    m_puiVerCost = m_puiComponentCost - rcMv.getVer();
#endif
  }
  Void    setCostScale( Int iCostScale )    { m_iCostScale = iCostScale; }
  __inline UInt getCost( Int x, Int y )
  {
#if FIX203
    return m_uiCost * getBits(x, y) >> 16;
#else
    return (( m_uiCost * (m_puiHorCost[ x * (1<<m_iCostScale) ] + m_puiVerCost[ y * (1<<m_iCostScale) ]) ) >> 16);
#endif
  }
  UInt    getCost( UInt b )                 { return ( m_uiCost * b ) >> 16; }
  UInt    getBits( Int x, Int y )          
  {
#if FIX203
    return xGetComponentBits((x << m_iCostScale) - m_mvPredictor.getHor())
    +      xGetComponentBits((y << m_iCostScale) - m_mvPredictor.getVer());
#else
    return m_puiHorCost[ x * (1<<m_iCostScale)] + m_puiVerCost[ y * (1<<m_iCostScale) ];
#endif
  }

  Void    setMultiviewReg( TComMv* pcMv )
  {
    if( pcMv )
    {
      m_bUseMultiviewReg    = true;
      m_puiHorRegCost       = m_puiMultiviewRegCostHor - pcMv->getHor();
      m_puiVerRegCost       = m_puiMultiviewRegCostVer - pcMv->getVer();
      m_cMultiviewOrgMvPred = *pcMv;
    }
    else
    {
      m_bUseMultiviewReg    = false;
      m_puiHorRegCost       = 0;
      m_puiVerRegCost       = 0;
      m_cMultiviewOrgMvPred.set( 0, 0 );
    }
  }
  __inline Bool     useMultiviewReg      () { return m_bUseMultiviewReg; }
  __inline TComMv&  getMultiviewOrgMvPred() { return m_cMultiviewOrgMvPred; }
  __inline UInt     getMultiviewRegCost  ( Int x, Int y )
  {
#if FIX203
    return m_uiLambdaMVReg * getBits(x, y) >> 16;
#else
    return ( ( m_uiLambdaMVReg * ( m_puiHorRegCost[ x * ( 1 << m_iCostScale ) ] + m_puiVerRegCost[ y * ( 1 << m_iCostScale ) ] ) ) >> 16 );
#endif
  }
  
private:
  
  static UInt xGetSSE           ( DistParam* pcDtParam );
  static UInt xGetSSE4          ( DistParam* pcDtParam );
  static UInt xGetSSE8          ( DistParam* pcDtParam );
  static UInt xGetSSE16         ( DistParam* pcDtParam );
  static UInt xGetSSE32         ( DistParam* pcDtParam );
  static UInt xGetSSE64         ( DistParam* pcDtParam );
  static UInt xGetSSE16N        ( DistParam* pcDtParam );
  
  static UInt xGetSAD           ( DistParam* pcDtParam );
  static UInt xGetSAD4          ( DistParam* pcDtParam );
  static UInt xGetSAD8          ( DistParam* pcDtParam );
  static UInt xGetSAD16         ( DistParam* pcDtParam );
  static UInt xGetSAD32         ( DistParam* pcDtParam );
  static UInt xGetSAD64         ( DistParam* pcDtParam );
  static UInt xGetSAD16N        ( DistParam* pcDtParam );
  
#if SAIT_VSO_EST_A0033
  static UInt xGetVSD           ( DistParam* pcDtParam );
  static UInt xGetVSD4          ( DistParam* pcDtParam );
  static UInt xGetVSD8          ( DistParam* pcDtParam );
  static UInt xGetVSD16         ( DistParam* pcDtParam );
  static UInt xGetVSD32         ( DistParam* pcDtParam );
  static UInt xGetVSD64         ( DistParam* pcDtParam );
  static UInt xGetVSD16N        ( DistParam* pcDtParam );
#endif

#if AMP_SAD
  static UInt xGetSAD12         ( DistParam* pcDtParam );
  static UInt xGetSAD24         ( DistParam* pcDtParam );
  static UInt xGetSAD48         ( DistParam* pcDtParam );

#endif

  static UInt xGetHADs4         ( DistParam* pcDtParam );
  static UInt xGetHADs8         ( DistParam* pcDtParam );
  static UInt xGetHADs          ( DistParam* pcDtParam );
  static UInt xCalcHADs2x2      ( Pel *piOrg, Pel *piCurr, Int iStrideOrg, Int iStrideCur, Int iStep );
  static UInt xCalcHADs4x4      ( Pel *piOrg, Pel *piCurr, Int iStrideOrg, Int iStrideCur, Int iStep );
  static UInt xCalcHADs8x8      ( Pel *piOrg, Pel *piCurr, Int iStrideOrg, Int iStrideCur, Int iStep );
#if NS_HAD
  static UInt xCalcHADs16x4     ( Pel *piOrg, Pel *piCurr, Int iStrideOrg, Int iStrideCur, Int iStep );
  static UInt xCalcHADs4x16     ( Pel *piOrg, Pel *piCurr, Int iStrideOrg, Int iStrideCur, Int iStep );
#endif
  
public:
#if HHI_INTERVIEW_SKIP
  UInt   getDistPart( Pel* piCur, Int iCurStride,  Pel* piOrg, Int iOrgStride, Pel* piUsed, Int iUsedStride, UInt uiBlkWidth, UInt uiBlkHeight, DFunc eDFunc = DF_SSE );
#endif
#if WEIGHTED_CHROMA_DISTORTION
  UInt   getDistPart( Pel* piCur, Int iCurStride,  Pel* piOrg, Int iOrgStride, UInt uiBlkWidth, UInt uiBlkHeight, Bool bWeighted = false, DFunc eDFunc = DF_SSE );
#else
  UInt   getDistPart( Pel* piCur, Int iCurStride,  Pel* piOrg, Int iOrgStride, UInt uiBlkWidth, UInt uiBlkHeight, DFunc eDFunc = DF_SSE );
#endif
  
#if SAIT_VSO_EST_A0033
  UInt   getDistPart( Pel* piCur, Int iCurStride,  Pel* piOrg, Int iOrgStride, Pel* piVirRec, Pel* piVirOrg, Int iVirStride, UInt uiBlkWidth, UInt uiBlkHeight, DFunc eDFunc = DF_VSD );
  static UInt getVSDEstimate( Int dDM, Pel* pOrg, Int iOrgStride,  Pel* pVirRec, Pel* pVirOrg, Int iVirStride, Int x, Int y );
#endif

#if HHI_VSO
private:
  Double                  m_dLambdaVSO;
  Double                  m_dSqrtLambdaVSO;
  UInt                    m_uiLambdaMotionSADVSO;
  UInt                    m_uiLambdaMotionSSEVSO;
  Double                  m_dFrameLambdaVSO;

#if HHI_VSO_DIST_INT
  Bool                    m_bAllowNegDist;
#endif
#if SAIT_VSO_EST_A0033
  TComPicYuv *            m_pcVideoRecPicYuv;
  TComPicYuv *            m_pcDepthPicYuv;
  Bool                    m_bUseEstimatedVSD; 
#endif
#if LGE_WVSO_A0119
  Int                     m_iDWeight;
  Int                     m_iVSOWeight;
  Int                     m_iVSDWeight;
  Bool                    m_bWVSO;
#endif

  Bool                    m_bUseVSO;
  Bool                    m_bUseLambdaScaleVSO;
  UInt                    m_uiVSOMode;

  FpDistFuncVSO           m_fpDistortFuncVSO;
  TRenModel*              m_pcRenModel;
public:

  Void    setRenModel       ( TRenModel* pcRenModel ) { m_pcRenModel = pcRenModel; }
  Void    setRenModelData   ( TComDataCU* pcCU, UInt uiAbsPartIndex, Pel* piData, Int iStride, Int iBlkWidth, Int iBlkHeight );
  Void    setLambdaVSO      ( Double dLambda );
  Void    setFrameLambdaVSO ( Double dLambda ) { m_dFrameLambdaVSO = dLambda; };


  Void    setUseVSO ( Bool bIn )         { m_bUseVSO = bIn; };
  Bool    getUseVSO ( )                  { return m_bUseVSO;};
#if SAIT_VSO_EST_A0033
  Bool    getUseEstimatedVSD( )           { return m_bUseEstimatedVSD; };
  Void    setUseEstimatedVSD( Bool bIn )  { m_bUseEstimatedVSD = bIn; };

  TComPicYuv* getVideoRecPicYuv ()                               { return m_pcVideoRecPicYuv; };
  Void        setVideoRecPicYuv ( TComPicYuv* pcVideoRecPicYuv ) { m_pcVideoRecPicYuv = pcVideoRecPicYuv; };
  TComPicYuv* getDepthPicYuv    ()                               { return m_pcDepthPicYuv; };
  Void        setDepthPicYuv    ( TComPicYuv* pcDepthPicYuv )    { m_pcDepthPicYuv = pcDepthPicYuv; };
#endif

  Bool    getUseRenModel ( )             { return (m_bUseVSO && m_uiVSOMode == 4); };
  Void    setUseLambdaScaleVSO(bool bIn) { m_bUseLambdaScaleVSO = bIn; };
  Bool    getUseLambdaScaleVSO( )        { return m_bUseLambdaScaleVSO; };

  Void    setVSOMode( UInt uiIn);
  UInt    getVSOMode( )                  { return m_uiVSOMode; }
#if LGE_WVSO_A0119
  Void    setWVSO ( Bool bIn )            { m_bWVSO = bIn; }; 
  Bool    getWVSO ( )                     { return m_bWVSO;};
  Void    setDWeight   ( Int iDWeight   ) { m_iDWeight = iDWeight; };
  Int     getDWeight   ()                 { return m_iDWeight; };
  Void    setVSOWeight ( Int iVSOWeight ) { m_iVSOWeight = iVSOWeight; };
  Int     getVSOWeight ()                 { return m_iVSOWeight; };
  Void    setVSDWeight ( Int iVSDWeight ) { m_iVSDWeight = iVSDWeight; };
  Int     getVSDWeight ()                 { return m_iVSDWeight; };
#endif 
#if HHI_VSO_DIST_INT
  Void    setAllowNegDist ( Bool bAllowNegDist );
#endif


  Double  getSqrtLambdaVSO ()   { return m_dSqrtLambdaVSO; }
  Double  getLambdaVSO ()       { return m_dLambdaVSO; }

  Dist    getDistVS( TComDataCU* pcCU, UInt uiAbsPartIndex, Pel* piCur, Int iCurStride, Pel* piOrg, Int iOrgStride, UInt uiBlkWidth, UInt uiBlkHeight, Bool bSAD, UInt uiPlane );
  Double calcRdCostVSO( UInt   uiBits, Dist   uiDistortion, Bool bFlag = false, DFunc eDFunc = DF_DEFAULT );

private:
  Dist xGetDistVSOMode4( Int iStartPosX, Int iStartPosY, Pel* piCur, Int iCurStride, Pel* piOrg, Int iOrgStride, UInt uiBlkWidth, UInt uiBlkHeight, Bool bSAD );

#endif // HHI_VSO

};// END CLASS DEFINITION TComRdCost

//! \}

#endif // __TCOMRDCOST__
