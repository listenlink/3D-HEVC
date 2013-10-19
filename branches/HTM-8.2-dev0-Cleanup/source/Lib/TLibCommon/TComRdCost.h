/* The copyright in this software is being made available under the BSD
 * License, included below. This software may be subject to other third party
 * and contributor rights, including patent rights, and no such rights are
 * granted under this license.  
 *
 * Copyright (c) 2010-2013, ITU/ISO/IEC
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

#include "TComSlice.h"
#include "TComRdCostWeightPrediction.h"
#if H_3D
#include "../TLibRenderer/TRenModel.h"
#endif

//! \ingroup TLibCommon
//! \{

#define FIX203 1

class DistParam;
class TComPattern;
#if H_3D
class TComRdCost; 
#endif

// ====================================================================================================================
// Type definition
// ====================================================================================================================

// for function pointer
typedef UInt (*FpDistFunc) (DistParam*);

#if H_3D
#if H_3D_VSO
typedef Dist (TComRdCost::*FpDistFuncVSO) ( Int, Int, Pel*, Int, Pel*, Int, UInt, UInt, Bool );
#endif
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
#if H_3D_VSO
  // SAIT_VSO_EST_A0033
  Pel*  pVirRec;
  Pel*  pVirOrg;
  Int   iStrideVir;
#endif
#if H_3D_IC
  Bool  bUseIC;
#endif
#if H_3D_INTER_SDC
  Bool  bUseSDCMRSAD;
#endif
  Int   iRows;
  Int   iCols;
  Int   iStep;
  FpDistFunc DistFunc;
  Int   bitDepth;

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
    iStrideOrg = 0;
    iStrideCur = 0;
    iRows = 0;
    iCols = 0;
    iStep = 1;
    DistFunc = NULL;
    iSubShift = 0;
    bitDepth = 0;
#if NS_HAD
    bUseNSHAD = false;
#endif
#if H_3D_VSO
    // SAIT_VSO_EST_A0033
    pVirRec = NULL;
    pVirOrg = NULL;
    iStrideVir = 0;
#endif
#if H_3D_INTER_SDC
    bUseSDCMRSAD = false;
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
  Double                  m_cbDistortionWeight; 
  Double                  m_crDistortionWeight; 
#endif
  Double                  m_dLambda;
  Double                  m_sqrtLambda;
  UInt                    m_uiLambdaMotionSAD;
  UInt                    m_uiLambdaMotionSSE;
  Double                  m_dFrameLambda;
#if H_3D_VSO
  // SAIT_VSO_EST_A0033
  static Double           m_dDisparityCoeff;
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
  
public:
  TComRdCost();
  virtual ~TComRdCost();

#if H_3D_VSO
  Double  calcRdCost  ( UInt   uiBits, Dist   uiDistortion, Bool bFlag = false, DFunc eDFunc = DF_DEFAULT );
  Double  calcRdCost64( UInt64 uiBits, Dist64 uiDistortion, Bool bFlag = false, DFunc eDFunc = DF_DEFAULT );
#else
  Double  calcRdCost  ( UInt   uiBits, UInt   uiDistortion, Bool bFlag = false, DFunc eDFunc = DF_DEFAULT );
  Double  calcRdCost64( UInt64 uiBits, UInt64 uiDistortion, Bool bFlag = false, DFunc eDFunc = DF_DEFAULT );
#endif

  
#if WEIGHTED_CHROMA_DISTORTION
  Void    setCbDistortionWeight      ( Double cbDistortionWeight) { m_cbDistortionWeight = cbDistortionWeight; };
  Void    setCrDistortionWeight      ( Double crDistortionWeight) { m_crDistortionWeight = crDistortionWeight; };
#endif
  Void    setLambda      ( Double dLambda );
  Void    setFrameLambda ( Double dLambda ) { m_dFrameLambda = dLambda; }
  
  Double  getSqrtLambda ()   { return m_sqrtLambda; }
#if H_3D_VSO
  // SAIT_VSO_EST_A0033
  Void    setDisparityCoeff( Double dDisparityCoeff ) { m_dDisparityCoeff = dDisparityCoeff; }
  Double  getDisparityCoeff()                         { return m_dDisparityCoeff; }
#endif

#if RATE_CONTROL_LAMBDA_DOMAIN
  Double  getLambda() { return m_dLambda; }
#if M0036_RC_IMPROVEMENT
  Double  getChromaWeight () {return((m_cbDistortionWeight+m_crDistortionWeight)/2.0);}
#endif
#endif
  
  // Distortion Functions
  Void    init();
  
  Void    setDistParam( UInt uiBlkWidth, UInt uiBlkHeight, DFunc eDFunc, DistParam& rcDistParam );
  Void    setDistParam( TComPattern* pcPatternKey, Pel* piRefY, Int iRefStride,            DistParam& rcDistParam );
#if NS_HAD
  Void    setDistParam( TComPattern* pcPatternKey, Pel* piRefY, Int iRefStride, Int iStep, DistParam& rcDistParam, Bool bHADME=false, Bool bUseNSHAD=false );
  Void    setDistParam( DistParam& rcDP, Int bitDepth, Pel* p1, Int iStride1, Pel* p2, Int iStride2, Int iWidth, Int iHeight, Bool bHadamard = false, Bool bUseNSHAD=false );
#else
  Void    setDistParam( TComPattern* pcPatternKey, Pel* piRefY, Int iRefStride, Int iStep, DistParam& rcDistParam, Bool bHADME=false );
  Void    setDistParam( DistParam& rcDP, Int bitDepth, Pel* p1, Int iStride1, Pel* p2, Int iStride2, Int iWidth, Int iHeight, Bool bHadamard = false );
#endif
  
  UInt    calcHAD(Int bitDepth, Pel* pi0, Int iStride0, Pel* pi1, Int iStride1, Int iWidth, Int iHeight );
  
#if SCU_HS_FAST_DEPTH_INTRA_E0238_HHIFIX
  UInt    calcVAR(Pel* pi0, Int stride, Int width, Int height, Int cuDepth);
#endif  
  // for motion cost
#if !FIX203
  Void    initRateDistortionModel( Int iSubPelSearchLimit );
  Void    xUninit();
#endif
  UInt    xGetComponentBits( Int iVal );
  Void    getMotionCost( Bool bSad, Int iAdd ) { m_uiCost = (bSad ? m_uiLambdaMotionSAD + iAdd : m_uiLambdaMotionSSE + iAdd); }
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
  
private:
  
  static UInt xGetSSE           ( DistParam* pcDtParam );
  static UInt xGetSSE4          ( DistParam* pcDtParam );
  static UInt xGetSSE8          ( DistParam* pcDtParam );
  static UInt xGetSSE16         ( DistParam* pcDtParam );
  static UInt xGetSSE32         ( DistParam* pcDtParam );
  static UInt xGetSSE64         ( DistParam* pcDtParam );
  static UInt xGetSSE16N        ( DistParam* pcDtParam );
#if H_3D_IC || H_3D_INTER_SDC
  static UInt xGetSADic         ( DistParam* pcDtParam );
  static UInt xGetSAD4ic        ( DistParam* pcDtParam );
  static UInt xGetSAD8ic        ( DistParam* pcDtParam );
  static UInt xGetSAD16ic       ( DistParam* pcDtParam );
  static UInt xGetSAD32ic       ( DistParam* pcDtParam );
  static UInt xGetSAD64ic       ( DistParam* pcDtParam );
  static UInt xGetSAD16Nic      ( DistParam* pcDtParam );
#endif
  static UInt xGetSAD           ( DistParam* pcDtParam );
  static UInt xGetSAD4          ( DistParam* pcDtParam );
  static UInt xGetSAD8          ( DistParam* pcDtParam );
  static UInt xGetSAD16         ( DistParam* pcDtParam );
  static UInt xGetSAD32         ( DistParam* pcDtParam );
  static UInt xGetSAD64         ( DistParam* pcDtParam );
  static UInt xGetSAD16N        ( DistParam* pcDtParam );
  
#if H_3D_VSO
  static UInt xGetVSD           ( DistParam* pcDtParam );
  static UInt xGetVSD4          ( DistParam* pcDtParam );
  static UInt xGetVSD8          ( DistParam* pcDtParam );
  static UInt xGetVSD16         ( DistParam* pcDtParam );
  static UInt xGetVSD32         ( DistParam* pcDtParam );
  static UInt xGetVSD64         ( DistParam* pcDtParam );
  static UInt xGetVSD16N        ( DistParam* pcDtParam );
#endif

#if AMP_SAD
#if H_3D_IC || H_3D_INTER_SDC
  static UInt xGetSAD12ic       ( DistParam* pcDtParam );
  static UInt xGetSAD24ic       ( DistParam* pcDtParam );
  static UInt xGetSAD48ic       ( DistParam* pcDtParam );
#endif
  static UInt xGetSAD12         ( DistParam* pcDtParam );
  static UInt xGetSAD24         ( DistParam* pcDtParam );
  static UInt xGetSAD48         ( DistParam* pcDtParam );

#endif

#if H_3D_IC || H_3D_INTER_SDC
  static UInt xGetHADsic          ( DistParam* pcDtParam );
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
#if WEIGHTED_CHROMA_DISTORTION
  UInt   getDistPart(Int bitDepth, Pel* piCur, Int iCurStride,  Pel* piOrg, Int iOrgStride, UInt uiBlkWidth, UInt uiBlkHeight, TextType eText = TEXT_LUMA, DFunc eDFunc = DF_SSE );
#else
  UInt   getDistPart(Int bitDepth, Pel* piCur, Int iCurStride,  Pel* piOrg, Int iOrgStride, UInt uiBlkWidth, UInt uiBlkHeight, DFunc eDFunc = DF_SSE );
#endif

#if (RATE_CONTROL_LAMBDA_DOMAIN && !M0036_RC_IMPROVEMENT) || KWU_RC_MADPRED_E0227
  UInt   getSADPart ( Int bitDepth, Pel* pelCur, Int curStride,  Pel* pelOrg, Int orgStride, UInt width, UInt height );
#endif

#if H_3D_VSO
  // SAIT_VSO_EST_A0033
  UInt        getDistPartVSD( TComDataCU* pcCu, UInt uiPartOffset, Pel* piCur, Int iCurStride,  Pel* piOrg, Int iOrgStride, UInt uiBlkWidth, UInt uiBlkHeight, Bool bHad, DFunc eDFunc = DF_VSD); 
  static UInt getVSDEstimate( Int dDM, Pel* pOrg, Int iOrgStride,  Pel* pVirRec, Pel* pVirOrg, Int iVirStride, Int x, Int y );

private:
  Double                  m_dLambdaVSO;
  Double                  m_dSqrtLambdaVSO;
  UInt                    m_uiLambdaMotionSADVSO;
  UInt                    m_uiLambdaMotionSSEVSO;
  Double                  m_dFrameLambdaVSO;
  Bool                    m_bAllowNegDist;
  Bool                    m_bUseVSO;
  Bool                    m_bUseLambdaScaleVSO;
  UInt                    m_uiVSOMode;

  FpDistFuncVSO           m_fpDistortFuncVSO;
  TRenModel*              m_pcRenModel;


  // SAIT_VSO_EST_A0033
  TComPicYuv *            m_pcVideoRecPicYuv;
  TComPicYuv *            m_pcDepthPicYuv;
  Bool                    m_bUseEstimatedVSD; 

  // LGE_WVSO_A0119
  Int                     m_iDWeight;
  Int                     m_iVSOWeight;
  Int                     m_iVSDWeight;
  Bool                    m_bUseWVSO;

public:

  Void    setRenModel       ( TRenModel* pcRenModel ) { m_pcRenModel = pcRenModel; }
  TRenModel* getRenModel    ( )                       { return m_pcRenModel; }
  Void    setRenModelData   ( TComDataCU* pcCU, UInt uiAbsPartIndex, Pel* piData, Int iStride, Int iBlkWidth, Int iBlkHeight );
  Void    setLambdaVSO      ( Double dLambda );
  Void    setFrameLambdaVSO ( Double dLambda ) { m_dFrameLambdaVSO = dLambda; };


  Void    setUseVSO ( Bool bIn )         { m_bUseVSO = bIn; };
  Bool    getUseVSO ( )                  { return m_bUseVSO;};

  Bool    getUseRenModel ( )             { return (m_bUseVSO && m_uiVSOMode == 4); };
  Void    setUseLambdaScaleVSO(Bool bIn) { m_bUseLambdaScaleVSO = bIn; };
  Bool    getUseLambdaScaleVSO( )        { return m_bUseLambdaScaleVSO; };

  Void    setVSOMode( UInt uiIn);
  UInt    getVSOMode( )                  { return m_uiVSOMode; }
  Void    setAllowNegDist ( Bool bAllowNegDist );

  Double  getSqrtLambdaVSO ()   { return m_dSqrtLambdaVSO; }
  Double  getLambdaVSO ()       { return m_dLambdaVSO; }

  Dist   getDistPartVSO( TComDataCU* pcCU, UInt uiAbsPartIndex, Pel* piCur, Int iCurStride, Pel* piOrg, Int iOrgStride, UInt uiBlkWidth, UInt uiBlkHeight, Bool bSAD );
  Double calcRdCostVSO ( UInt   uiBits, Dist   uiDistortion, Bool bFlag = false, DFunc eDFunc = DF_DEFAULT );

  // SAIT_VSO_EST_A0033
  Bool    getUseEstimatedVSD( )           { return m_bUseEstimatedVSD; };
  Void    setUseEstimatedVSD( Bool bIn )  { m_bUseEstimatedVSD = bIn; };

  TComPicYuv* getVideoRecPicYuv ()                               { return m_pcVideoRecPicYuv; };
  Void        setVideoRecPicYuv ( TComPicYuv* pcVideoRecPicYuv ) { m_pcVideoRecPicYuv = pcVideoRecPicYuv; };
  TComPicYuv* getDepthPicYuv    ()                               { return m_pcDepthPicYuv; };
  Void        setDepthPicYuv    ( TComPicYuv* pcDepthPicYuv )    { m_pcDepthPicYuv = pcDepthPicYuv; };

  // LGE_WVSO_A0119
  Void    setUseWVSO ( Bool bIn )         { m_bUseWVSO = bIn; }; 
  Bool    getUseWVSO ( )                  { return m_bUseWVSO;};
  Void    setDWeight   ( Int iDWeight   ) { m_iDWeight = iDWeight; };
  Int     getDWeight   ()                 { return m_iDWeight; };
  Void    setVSOWeight ( Int iVSOWeight ) { m_iVSOWeight = iVSOWeight; };
  Int     getVSOWeight ()                 { return m_iVSOWeight; };
  Void    setVSDWeight ( Int iVSDWeight ) { m_iVSDWeight = iVSDWeight; };
  Int     getVSDWeight ()                 { return m_iVSDWeight; };

private:
  Dist xGetDistVSOMode4( Int iStartPosX, Int iStartPosY, Pel* piCur, Int iCurStride, Pel* piOrg, Int iOrgStride, UInt uiBlkWidth, UInt uiBlkHeight, Bool bSAD );

#endif // H_3D_VSO

};// END CLASS DEFINITION TComRdCost

//! \}

#endif // __TCOMRDCOST__
