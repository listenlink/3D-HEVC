/* The copyright in this software is being made available under the BSD
 * License, included below. This software may be subject to other third party
 * and contributor rights, including patent rights, and no such rights are
 * granted under this license.
 *
 * Copyright (c) 2010-2015, ITU/ISO/IEC
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
#if NH_3D_VSO
#include "../TLibRenderer/TRenModel.h"
#include "TComYuv.h"
#include "TComTU.h"
#endif

//! \ingroup TLibCommon
//! \{

class DistParam;
class TComPattern;
#if NH_3D_VSO
class TComRdCost; 
#endif

// ====================================================================================================================
// Type definition
// ====================================================================================================================

// for function pointer
typedef Distortion (*FpDistFunc) (DistParam*); // TODO: can this pointer be replaced with a reference? - there are no NULL checks on pointer.


#if NH_3D_VSO
typedef Dist (TComRdCost::*FpDistFuncVSO) ( Int, Int, Pel*, Int, Pel*, Int, UInt, UInt, Bool );
#endif
// ====================================================================================================================
// Class definition
// ====================================================================================================================

/// distortion parameter class
class DistParam
{
public:
  const Pel*            pOrg;
  const Pel*            pCur;
  Int   iStrideOrg;
  Int   iStrideCur;
#if NH_3D_VSO
  // SAIT_VSO_EST_A0033
  Pel*  pVirRec;
  Pel*  pVirOrg;
  Int   iStrideVir;
#endif
#if NH_3D_IC
  Bool  bUseIC;
#endif
#if NH_3D_SDC_INTER
  Bool  bUseSDCMRSAD;
#endif
  Int   iRows;
  Int   iCols;
  Int   iStep;
  FpDistFunc DistFunc;
  Int   bitDepth;

  Bool            bApplyWeight;     // whether weighted prediction is used or not
  Bool                  bIsBiPred;

  const WPScalingParam *wpCur;           // weighted prediction scaling parameters for current ref
  ComponentID     compIdx;
  Distortion            m_maximumDistortionForEarlyExit; /// During cost calculations, if distortion exceeds this value, cost calculations may early-terminate.

  // (vertical) subsampling shift (for reducing complexity)
  // - 0 = no subsampling, 1 = even rows, 2 = every 4th, etc.
  Int   iSubShift;

  DistParam()
   : pOrg(NULL),
     pCur(NULL),
     iStrideOrg(0),
     iStrideCur(0),
     iRows(0),
     iCols(0),
     iStep(1),
     DistFunc(NULL),
     bitDepth(0),
     bApplyWeight(false),
     bIsBiPred(false),
     wpCur(NULL),
     compIdx(MAX_NUM_COMPONENT),
     m_maximumDistortionForEarlyExit(std::numeric_limits<Distortion>::max()),
     iSubShift(0)
  {
#if NH_3D_VSO
    // SAIT_VSO_EST_A0033
    pVirRec = NULL;
    pVirOrg = NULL;
    iStrideVir = 0;
#endif
#if NH_3D_SDC_INTER
    bUseSDCMRSAD = false;
#endif
  }
};

/// RD cost computation class
class TComRdCost
{
private:
  // for distortion

  FpDistFunc              m_afpDistortFunc[DF_TOTAL_FUNCTIONS]; // [eDFunc]
  CostMode                m_costMode;
  Double                  m_distortionWeight[MAX_NUM_COMPONENT]; // only chroma values are used.
  Double                  m_dLambda;
  Double                  m_sqrtLambda;
  Double                  m_dLambdaMotionSAD[2 /* 0=standard, 1=for transquant bypass when mixed-lossless cost evaluation enabled*/];
  Double                  m_dLambdaMotionSSE[2 /* 0=standard, 1=for transquant bypass when mixed-lossless cost evaluation enabled*/];
  Double                  m_dFrameLambda;
#if NH_3D_VSO
  // SAIT_VSO_EST_A0033
  static Double           m_dDisparityCoeff;
#endif

  // for motion cost
  TComMv                  m_mvPredictor;
  Double                  m_motionLambda;
  Int                     m_iCostScale;
#if NH_3D_DBBP
  Bool                    m_bUseMask;
#endif

public:
  TComRdCost();
  virtual ~TComRdCost();
#if NH_3D_VSO
  Double  calcRdCost64( UInt64 uiBits , Dist64 uiDistortion, Bool bFlag = false, DFunc eDFunc = DF_DEFAULT );
  Double  calcRdCost( Double numBits, Dist intDistortion, DFunc eDFunc  = DF_DEFAULT );
#else
  Double calcRdCost( Double numBits, Double distortion, DFunc eDFunc = DF_DEFAULT );
#endif


  Void    setDistortionWeight  ( const ComponentID compID, const Double distortionWeight ) { m_distortionWeight[compID] = distortionWeight; }
  Void    setLambda      ( Double dLambda, const BitDepths &bitDepths );
  Void    setFrameLambda ( Double dLambda ) { m_dFrameLambda = dLambda; }

  Double  getSqrtLambda ()   { return m_sqrtLambda; }
#if NH_3D_VSO
  // SAIT_VSO_EST_A0033
  Void    setDisparityCoeff( Double dDisparityCoeff ) { m_dDisparityCoeff = dDisparityCoeff; }
  Double  getDisparityCoeff()                         { return m_dDisparityCoeff; }
#endif

  Double  getLambda() { return m_dLambda; }
  Double  getChromaWeight () { return ((m_distortionWeight[COMPONENT_Cb] + m_distortionWeight[COMPONENT_Cr]) / 2.0); }

  Void    setCostMode(CostMode   m )    { m_costMode = m; }

  // Distortion Functions
  Void    init();

  Void    setDistParam( UInt uiBlkWidth, UInt uiBlkHeight, DFunc eDFunc, DistParam& rcDistParam );
  Void    setDistParam( const TComPattern* const pcPatternKey, const Pel* piRefY, Int iRefStride,            DistParam& rcDistParam );
  Void    setDistParam( const TComPattern* const pcPatternKey, const Pel* piRefY, Int iRefStride, Int iStep, DistParam& rcDistParam, Bool bHADME=false );
  Void    setDistParam( DistParam& rcDP, Int bitDepth, const Pel* p1, Int iStride1, const Pel* p2, Int iStride2, Int iWidth, Int iHeight, Bool bHadamard = false );

#if NH_3D_DBBP
  Void    setUseMask(Bool b) { m_bUseMask = b; }
#endif

  Distortion calcHAD(Int bitDepth, const Pel* pi0, Int iStride0, const Pel* pi1, Int iStride1, Int iWidth, Int iHeight );

#if NH_3D_ENC_DEPTH
  UInt    calcVAR(Pel* pi0, Int stride, Int width, Int height, Int cuDepth, UInt maxCuWidth );
#endif  

  // for motion cost
  static UInt    xGetExpGolombNumberOfBits( Int iVal );
  Void    selectMotionLambda( Bool bSad, Int iAdd, Bool bIsTransquantBypass ) { m_motionLambda = (bSad ? m_dLambdaMotionSAD[(bIsTransquantBypass && m_costMode==COST_MIXED_LOSSLESS_LOSSY_CODING) ?1:0] + iAdd : m_dLambdaMotionSSE[(bIsTransquantBypass && m_costMode==COST_MIXED_LOSSLESS_LOSSY_CODING)?1:0] + iAdd); }
  Void    setPredictor( TComMv& rcMv )
  {
    m_mvPredictor = rcMv;
  }
  Void    setCostScale( Int iCostScale )    { m_iCostScale = iCostScale; }
  Distortion getCost( UInt b )                 { return Distortion(( m_motionLambda * b ) / 65536.0); }
  Distortion getCostOfVectorWithPredictor( const Int x, const Int y )
  {
    return Distortion((m_motionLambda * getBitsOfVectorWithPredictor(x, y)) / 65536.0);
  }
  UInt getBitsOfVectorWithPredictor( const Int x, const Int y )
  {
    return xGetExpGolombNumberOfBits((x << m_iCostScale) - m_mvPredictor.getHor())
    +      xGetExpGolombNumberOfBits((y << m_iCostScale) - m_mvPredictor.getVer());
  }

private:

  static Distortion xGetSSE           ( DistParam* pcDtParam );
  static Distortion xGetSSE4          ( DistParam* pcDtParam );
  static Distortion xGetSSE8          ( DistParam* pcDtParam );
  static Distortion xGetSSE16         ( DistParam* pcDtParam );
  static Distortion xGetSSE32         ( DistParam* pcDtParam );
  static Distortion xGetSSE64         ( DistParam* pcDtParam );
  static Distortion xGetSSE16N        ( DistParam* pcDtParam );
#if NH_3D_IC || NH_3D_SDC_INTER
  static UInt xGetSADic         ( DistParam* pcDtParam );
  static UInt xGetSAD4ic        ( DistParam* pcDtParam );
  static UInt xGetSAD8ic        ( DistParam* pcDtParam );
  static UInt xGetSAD16ic       ( DistParam* pcDtParam );
  static UInt xGetSAD32ic       ( DistParam* pcDtParam );
  static UInt xGetSAD64ic       ( DistParam* pcDtParam );
  static UInt xGetSAD16Nic      ( DistParam* pcDtParam );
#endif

  static Distortion xGetSAD           ( DistParam* pcDtParam );
  static Distortion xGetSAD4          ( DistParam* pcDtParam );
  static Distortion xGetSAD8          ( DistParam* pcDtParam );
  static Distortion xGetSAD16         ( DistParam* pcDtParam );
  static Distortion xGetSAD32         ( DistParam* pcDtParam );
  static Distortion xGetSAD64         ( DistParam* pcDtParam );
  static Distortion xGetSAD16N        ( DistParam* pcDtParam );
#if NH_3D_VSO
  static UInt xGetVSD           ( DistParam* pcDtParam );
  static UInt xGetVSD4          ( DistParam* pcDtParam );
  static UInt xGetVSD8          ( DistParam* pcDtParam );
  static UInt xGetVSD16         ( DistParam* pcDtParam );
  static UInt xGetVSD32         ( DistParam* pcDtParam );
  static UInt xGetVSD64         ( DistParam* pcDtParam );
  static UInt xGetVSD16N        ( DistParam* pcDtParam );
#endif

#if NH_3D_IC || NH_3D_SDC_INTER
  static UInt xGetSAD12ic       ( DistParam* pcDtParam );
  static UInt xGetSAD24ic       ( DistParam* pcDtParam );
  static UInt xGetSAD48ic       ( DistParam* pcDtParam );
#endif

  static Distortion xGetSAD12         ( DistParam* pcDtParam );
  static Distortion xGetSAD24         ( DistParam* pcDtParam );
  static Distortion xGetSAD48         ( DistParam* pcDtParam );


#if NH_3D_IC || NH_3D_SDC_INTER
  static UInt xGetHADsic          ( DistParam* pcDtParam );
#endif

  static Distortion xGetHADs          ( DistParam* pcDtParam );
  static Distortion xCalcHADs2x2      ( const Pel *piOrg, const Pel *piCurr, Int iStrideOrg, Int iStrideCur, Int iStep );
  static Distortion xCalcHADs4x4      ( const Pel *piOrg, const Pel *piCurr, Int iStrideOrg, Int iStrideCur, Int iStep );
  static Distortion xCalcHADs8x8      ( const Pel *piOrg, const Pel *piCurr, Int iStrideOrg, Int iStrideCur, Int iStep );
#if NH_3D_DBBP
  static UInt xGetMaskedSSE     ( DistParam* pcDtParam );
  static UInt xGetMaskedSAD     ( DistParam* pcDtParam );
  static UInt xGetMaskedVSD     ( DistParam* pcDtParam );
#endif


public:

  Distortion   getDistPart(Int bitDepth, const Pel* piCur, Int iCurStride, const Pel* piOrg, Int iOrgStride, UInt uiBlkWidth, UInt uiBlkHeight, const ComponentID compID, DFunc eDFunc = DF_SSE );

#if KWU_RC_MADPRED_E0227
  UInt   getSADPart ( Int bitDepth, Pel* pelCur, Int curStride,  Pel* pelOrg, Int orgStride, UInt width, UInt height );
#endif

#if NH_3D_VSO
  // SAIT_VSO_EST_A0033
  UInt        getDistPartVSD( TComDataCU* pcCu, UInt uiPartOffset, Int bitDepth, Pel* piCur, Int iCurStride,  Pel* piOrg, Int iOrgStride, UInt uiBlkWidth, UInt uiBlkHeight, Bool bHad, DFunc eDFunc = DF_VSD); 
  static UInt getVSDEstimate( Int dDM, const Pel* pOrg, Int iOrgStride,  const Pel* pVirRec, const Pel* pVirOrg, Int iVirStride, Int x, Int y );

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
  Void    setRenModelData   ( const TComDataCU* pcCU, UInt uiAbsPartIndex, const TComYuv* pcYuv, const TComTURecurse* tuRecurseWithPU );
  Void    setRenModelData   ( const TComDataCU* pcCU, UInt uiAbsPartIndex, const Pel* piData, Int iStride, Int iBlkWidth, Int iBlkHeight );

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

  Dist    getDistPartVSO( TComDataCU* pcCU, UInt uiAbsPartIndex, Int bitdDepth, Pel* piCur, Int iCurStride, Pel* piOrg, Int iOrgStride, UInt uiBlkWidth, UInt uiBlkHeight, Bool bSAD );
  Double  calcRdCostVSO ( UInt   uiBits, Dist   uiDistortion, Bool bFlag = false, DFunc eDFunc = DF_DEFAULT );

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

#endif // NH_3D_VSO


};// END CLASS DEFINITION TComRdCost

//! \}

#endif // __TCOMRDCOST__
