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

/** \file     TEncAdaptiveLoopFilter.h
 \brief    estimation part of adaptive loop filter class (header)
 */

#ifndef __TENCADAPTIVELOOPFILTER__
#define __TENCADAPTIVELOOPFILTER__

#include "TLibCommon/TComAdaptiveLoopFilter.h"
#include "TLibCommon/TComPic.h"

#include "TEncEntropy.h"
#include "TEncSbac.h"
#include "TLibCommon/TComBitCounter.h"

//! \ingroup TLibEncoder
//! \{

#if LCU_SYNTAX_ALF
#define LCUALF_FILTER_BUDGET_CONTROL_ENC        1 //!< filter budget control 
#define LCUALF_AVOID_USING_BOTTOM_LINES_ENCODER 1 //!< avoid using LCU bottom lines when lcu-based encoder RDO is used
#endif

// ====================================================================================================================
// Class definition
// ====================================================================================================================

#if LCU_SYNTAX_ALF
/// correlation info
struct AlfCorrData
{
  Double*** ECorr; //!< auto-correlation matrix
  Double**  yCorr; //!< cross-correlation
  Double*   pixAcc;
  Int componentID;

  //constructor & operator
  AlfCorrData();
  AlfCorrData(Int cIdx);
  ~AlfCorrData();
  Void reset();
  Void mergeFrom(const AlfCorrData& src, Int* mergeTable, Bool doPixAccMerge);
  AlfCorrData& operator += (const AlfCorrData& src);
};

/// picture quad-tree info
struct AlfPicQTPart
{
  Int         componentID;
  Int         partCUXS;
  Int         partCUYS;
  Int         partCUXE;
  Int         partCUYE;
  Int         partIdx;
  Int         partLevel;
  Int         partCol;
  Int         partRow;
  Int         childPartIdx[4];
  Int         parentPartIdx;
  Bool        isBottomLevel;
  Bool        isSplit;
  Bool        isProcessed;
  Double      splitMinCost;
  Int64       splitMinDist;
  Int64       splitMinRate;
  Double      selfMinCost;
  Int64       selfMinDist;
  Int64       selfMinRate;
  Int         numFilterBudget;

  AlfUnitParam* alfUnitParam; 
  AlfCorrData*  alfCorr;

  //constructor & operator
  AlfPicQTPart();
  ~AlfPicQTPart();
  AlfPicQTPart& operator= (const AlfPicQTPart& src);
};

#endif


/// estimation part of adaptive loop filter class
class TEncAdaptiveLoopFilter : public TComAdaptiveLoopFilter
{
private:
  ///
  /// variables for correlation calculation
  ///
#if !LCU_SYNTAX_ALF
  Double** m_ppdAlfCorr;
  Double* m_pdDoubleAlfCoeff;
  Double** m_ppdAlfCorrCb;
  Double** m_ppdAlfCorrCr;
  double ***m_yGlobalSym;
  double ****m_EGlobalSym;
  double *m_pixAcc;
#endif
  double **m_y_merged;
  double ***m_E_merged;
  double *m_pixAcc_merged;
  double *m_y_temp;
  double **m_E_temp;
#if LCU_SYNTAX_ALF
  static const Int  m_alfNumPartsInRowTab[5];
  static const Int  m_alfNumPartsLevelTab[5];
  static const Int  m_alfNumCulPartsLevelTab[5];

  Int    m_lastSliceIdx;
  Bool   m_picBasedALFEncode;
  Bool   m_alfCoefInSlice;
  Int*   m_numSlicesDataInOneLCU;
  Int*   m_coeffNoFilter[NO_VAR_BINS]; //!< used for RDO
  AlfParamSet* m_bestAlfParamSet;
  AlfCorrData** m_alfCorr[NUM_ALF_COMPONENT];
  AlfCorrData*  m_alfCorrMerged[NUM_ALF_COMPONENT]; //!< used for RDO
  AlfUnitParam* m_alfPicFiltUnits[NUM_ALF_COMPONENT];
  Int    m_alfPQTMaxDepth;
  AlfPicQTPart* m_alfPQTPart[NUM_ALF_COMPONENT]; 
#if LCUALF_FILTER_BUDGET_CONTROL_ENC
  Double m_alfFiltBudgetPerLcu;
  Int    m_alfUsedFilterNum;
#endif
#endif

  ///
  /// ALF parameters
  ///
#if !LCU_SYNTAX_ALF
  ALFParam* m_pcBestAlfParam;
  ALFParam* m_pcTempAlfParam;
  ALFParam* pcAlfParamShape0;
  ALFParam* pcAlfParamShape1;
#endif
  ALFParam *m_tempALFp;

  ///
  /// temporary picture buffers or pointers
  ///
  TComPicYuv* m_pcPicYuvBest;
  TComPicYuv* m_pcPicYuvTmp;
#if !LCU_SYNTAX_ALF
  TComPicYuv* pcPicYuvRecShape0;
  TComPicYuv* pcPicYuvRecShape1;
#endif

  ///
  /// temporary filter buffers or pointers
  ///
  Int    *m_filterCoeffQuantMod;
  double *m_filterCoeff;
  Int    *m_filterCoeffQuant;
  Int    **m_filterCoeffSymQuant;
  Int    **m_diffFilterCoeffQuant;
  Int    **m_FilterCoeffQuantTemp;
#if !LCU_SYNTAX_ALF
  Int**  m_mergeTableSavedMethods[NUM_ALF_CLASS_METHOD];
  Int*** m_aiFilterCoeffSavedMethods[NUM_ALF_CLASS_METHOD];  //!< time-delayed filter set buffer
  Int*   m_iPreviousFilterShapeMethods[NUM_ALF_CLASS_METHOD];
#endif
  ///
  /// coding control parameters
  ///
  Double m_dLambdaLuma;
  Double m_dLambdaChroma;
#if !LCU_SYNTAX_ALF
  Int  m_iUsePreviousFilter;     //!< for N-pass encoding- 1: time-delayed filtering is allowed. 0: not allowed.
  Int  m_iDesignCurrentFilter;   //!< for N-pass encoding- 1: design filters for current slice. 0: design filters for future slice reference
  Int  m_iGOPSize;                //!< GOP size
  Int  m_iCurrentPOC;             //!< POC
#endif
  Int  m_iALFEncodePassReduction; //!< 0: 16-pass encoding, 1: 1-pass encoding, 2: 2-pass encoding

  Int  m_iALFMaxNumberFilters;    //!< ALF Max Number Filters per unit

  Int  m_iALFNumOfRedesign;       //!< number of redesigning filter for each CU control depth

  ///
  /// variables for on/off control
  ///
  Pel **m_maskImg;
  Bool m_bAlfCUCtrlEnabled;                         //!< if input ALF CU control param is NULL, this variable is set to be false (Disable CU control)
  std::vector<AlfCUCtrlInfo> m_vBestAlfCUCtrlParam; //!< ALF CU control parameters container to store the ALF CU control parameters after RDO

  ///
  /// miscs. 
  ///
  TEncEntropy* m_pcEntropyCoder;
private:

#if LCU_SYNTAX_ALF
  Void disableComponentAlfParam(Int compIdx, AlfParamSet* alfParamSet, AlfUnitParam* alfUnitPic);
  Void copyAlfParamSet(AlfParamSet* dst, AlfParamSet* src);
  Void initALFEncoderParam(AlfParamSet* alfParamSet, std::vector<AlfCUCtrlInfo>* alfCtrlParam);
  Void assignALFEncoderParam(AlfParamSet* alfParamSet, std::vector<AlfCUCtrlInfo>* alfCtrlParam);
  Void getStatistics(TComPicYuv* pPicOrg, TComPicYuv* pPicDec);
  Void getOneCompStatistics(AlfCorrData** alfCorrComp, Int compIdx, Pel* imgOrg, Pel* imgDec, Int stride, Int formatShift, Bool isRedesignPhase);
  Void getStatisticsOneLCU(Bool skipLCUBottomLines, Int compIdx, AlfLCUInfo* alfLCU, AlfCorrData* alfCorr, Pel* pPicOrg, Pel* pPicSrc, Int stride, Int formatShift, Bool isRedesignPhase);
#if HHI_INTERVIEW_SKIP
  Void decideParameters(TComPicYuv* pPicOrg, TComPicYuv* pPicDec, TComPicYuv* pPicRest, TComPicYuv* pUsedPelMap, AlfParamSet* alfParamSet, std::vector<AlfCUCtrlInfo>* alfCtrlParam) ;
#else
  Void decideParameters(TComPicYuv* pPicOrg, TComPicYuv* pPicDec, TComPicYuv* pPicRest, AlfParamSet* alfParamSet, std::vector<AlfCUCtrlInfo>* alfCtrlParam);
#endif
  Void deriveFilterInfo(Int compIdx, AlfCorrData* alfCorr, ALFParam* alfFiltParam, Int maxNumFilters);
#if HHI_INTERVIEW_SKIP
  Void decideBlockControl(Pel* pOrg, Pel* pDec, Pel* pRest, Pel* pUsed, Int stride, AlfPicQTPart* alfPicQTPart, AlfParamSet* & alfParamSet, Int64 &minRate, Int64 &minDist, Double &minCost);
#else
  Void decideBlockControl(Pel* pOrg, Pel* pDec, Pel* pRest, Int stride, AlfPicQTPart* alfPicQTPart, AlfParamSet* & alfParamSet, Int64 &minRate, Int64 &minDist, Double &minCost);
#endif
  Void copyPicQT(AlfPicQTPart* alfPicQTPartDest, AlfPicQTPart* alfPicQTPartSrc);
  Void copyPixelsInOneRegion(Pel* imgDest, Pel* imgSrc, Int stride, Int yPos, Int height, Int xPos, Int width);
  Void reDesignQT(AlfPicQTPart *alfPicQTPart, Int partIdx, Int partLevel);
#if HHI_INTERVIEW_SKIP
  Void setCUAlfCtrlFlags(UInt uiAlfCtrlDepth, Pel* imgOrg, Pel* imgDec, Pel* imgRest, Pel* imgUsed, Int stride, UInt64& ruiDist, std::vector<AlfCUCtrlInfo>& vAlfCUCtrlParam);
  Void setCUAlfCtrlFlag(TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth, UInt uiAlfCtrlDepth, Pel* imgOrg, Pel* imgDec, Pel* imgRest, Pel* imgUsed, Int stride, UInt64& ruiDist, std::vector<UInt>& vCUCtrlFlag) ;
#else
  Void setCUAlfCtrlFlags(UInt uiAlfCtrlDepth, Pel* imgOrg, Pel* imgDec, Pel* imgRest, Int stride, UInt64& ruiDist, std::vector<AlfCUCtrlInfo>& vAlfCUCtrlParam);
  Void setCUAlfCtrlFlag(TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth, UInt uiAlfCtrlDepth, Pel* imgOrg, Pel* imgDec, Pel* imgRest, Int stride, UInt64& ruiDist, std::vector<UInt>& vCUCtrlFlag);
#endif
  Void xCopyDecToRestCUs( Pel* imgDec, Pel* imgRest, Int stride );
  Void xCopyDecToRestCU( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth, Pel* imgDec, Pel* imgRest, Int stride );
#if ALF_SINGLE_FILTER_SHAPE 
  Void calcCorrOneCompRegionChma(Pel* imgOrg, Pel* imgPad, Int stride, Int yPos, Int xPos, Int height, Int width, Double **eCorr, Double *yCorr, Bool isSymmCopyBlockMatrix); //!< Calculate correlations for chroma                                        
  Void calcCorrOneCompRegionLuma(Pel* imgOrg, Pel* imgPad, Int stride, Int yPos, Int xPos, Int height, Int width, Double ***eCorr, Double **yCorr, Double *pixAcc, Bool isforceCollection, Bool isSymmCopyBlockMatrix);
#endif

  //LCU-based mode decision
#if HHI_INTERVIEW_SKIP
  Void  executeLCUBasedModeDecision(AlfParamSet* alfParamSet, Int compIdx, Pel* pOrg, Pel* pDec, Pel* pRest, Pel* pUsed, Int stride, Int formatShift, AlfCorrData** alfCorrLCUs);
#else
  Void  executeLCUBasedModeDecision(AlfParamSet* alfParamSet, Int compIdx, Pel* pOrg, Pel* pDec, Pel* pRest, Int stride, Int formatShift, AlfCorrData** alfCorrLCUs);
#endif
  Void  decideLCUALFUnitParam(Int compIdx, AlfUnitParam* alfUnitPic, Int lcuIdx, Int lcuPos, Int numLCUWidth, AlfUnitParam* alfUnitParams, AlfCorrData* alfCorr, std::vector<ALFParam*>& storedFilters, Int maxNumFilter, Double lambda, Bool isLeftUnitAvailable, Bool isUpUnitAvailable);
  Void  getFiltOffAlfUnitParam(AlfUnitParam* alfFiltOffParam, Int lcuPos, AlfUnitParam* alfUnitPic, Bool isLeftUnitAvailable, Bool isUpUnitAvailable);
  Int64 estimateFilterDistortion(Int compIdx, AlfCorrData* alfCorr, Int** coeff = NULL, Int filterSetSize = 1, Int* mergeTable = NULL, Bool doPixAccMerge = false);
  Int   calculateAlfUnitRateRDO(AlfUnitParam* alfUnitParam, Int numStoredFilters = 0);
#if HHI_INTERVIEW_SKIP
  Int64 calcAlfLCUDist(Bool skipLCUBottomLines, Int compIdx, AlfLCUInfo& alfLCUInfo, Pel* picSrc, Pel* picCmp, Pel* picUsed, Int stride, Int formatShift) ;
#else
  Int64 calcAlfLCUDist(Bool skipLCUBottomLines, Int compIdx, AlfLCUInfo& alfLCUInfo, Pel* picSrc, Pel* picCmp, Int stride, Int formatShift);
#endif
  Void  reconstructOneAlfLCU(Int compIdx, AlfLCUInfo& alfLCUInfo, AlfUnitParam* alfUnitParam, Pel* picDec, Pel* picRest, Int stride, Int formatShift);
  Void  copyOneAlfLCU(AlfLCUInfo& alfLCUInfo, Pel* picDst, Pel* picSrc, Int stride, Int formatShift);

  //picture-based mode decision
#if HHI_INTERVIEW_SKIP
  Void executePicBasedModeDecision(AlfParamSet* alfParamSet, AlfPicQTPart* alfPicQTPart, Int compIdx, Pel* pOrg, Pel* pDec, Pel* pRest, Pel* pUsed, Int stride, Int formatShift, AlfCorrData** alfCorrLCUs);
#else
  Void executePicBasedModeDecision(AlfParamSet* alfParamSet, AlfPicQTPart* alfPicQTPart, Int compIdx, Pel* pOrg, Pel* pDec, Pel* pRest, Int stride, Int formatShift, AlfCorrData** alfCorrLCUs);
#endif
  Void creatPQTPart               (Int partLevel, Int partRow, Int partCol, Int parentPartIdx, Int partCUXS, Int partCUXE, Int partCUYS, Int partCUYE);
  Void resetPQTPart               ();
  Int  convertLevelRowCol2Idx     (Int level, Int row, Int col);
  Void convertIdx2LevelRowCol     (Int idx, Int *level, Int *row, Int *col);
  Void executeModeDecisionOnePart (AlfPicQTPart *alfPicQTPart, AlfCorrData** alfPicLCUCorr, Int partIdx, Int partLevel);
  Void decideQTPartition          (AlfPicQTPart* alfPicQTPart, AlfCorrData** alfPicLCUCorr, Int partIdx, Int partLevel, Double &cost, Int64 &dist, Int64 &rate);
  Void patchAlfUnitParams(AlfPicQTPart* alfPicQTPart, Int partIdx, AlfUnitParam* alfUnitPic);
  Void checkMerge(Int compIdx, AlfUnitParam* alfUnitPic);
  Void transferToAlfParamSet(Int compIdx, AlfUnitParam* alfUnitPic, AlfParamSet* & alfParamSet);
  Int  calculateAlfParamSetRateRDO(Int compIdx, AlfParamSet* alfParamSet, std::vector<AlfCUCtrlInfo>* alfCUCtrlParam);
#endif
#if !LCU_SYNTAX_ALF
  // init / uninit internal variables
  Void xInitParam      ();
  Void xUninitParam    ();
#endif
  // ALF on/off control related functions
  Void xCreateTmpAlfCtrlFlags   ();
  Void xDestroyTmpAlfCtrlFlags  ();
  Void xCopyTmpAlfCtrlFlagsTo   ();
  Void xCopyTmpAlfCtrlFlagsFrom ();
  Void getCtrlFlagsFromCU(AlfLCUInfo* pcAlfLCU, std::vector<UInt> *pvFlags, Int iAlfDepth, UInt uiMaxNumSUInLCU);
  Void xEncodeCUAlfCtrlFlags  (std::vector<AlfCUCtrlInfo> &vAlfCUCtrlParam);
  Void xEncodeCUAlfCtrlFlag   ( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth);
#if !LCU_SYNTAX_ALF  
  Void xCUAdaptiveControl_qc           ( TComPicYuv* pcPicOrg, TComPicYuv* pcPicDec, TComPicYuv* pcPicRest, UInt64& ruiMinRate, UInt64& ruiMinDist, Double& rdMinCost );
  Void xSetCUAlfCtrlFlags_qc            (UInt uiAlfCtrlDepth, TComPicYuv* pcPicOrg, TComPicYuv* pcPicDec, TComPicYuv* pcPicRest, UInt64& ruiDist, std::vector<AlfCUCtrlInfo>& vAlfCUCtrlParam);
  Void xSetCUAlfCtrlFlag_qc             (TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth, UInt uiAlfCtrlDepth, TComPicYuv* pcPicOrg, TComPicYuv* pcPicDec, TComPicYuv* pcPicRest, UInt64& ruiDist, std::vector<UInt>& vCUCtrlFlag);

  // functions related to correlation computation
  Void xstoreInBlockMatrix(Int ypos, Int xpos, Int iheight, Int iwidth, Bool bResetBlockMatrix, Bool bSymmCopyBlockMatrix, Pel* pImgOrg, Pel* pImgPad, Int filtNo, Int stride); //!< Calculate correlations for luma
  Void xstoreInBlockMatrixforRegion(std::vector< AlfLCUInfo* > &vpAlfLCU, Pel* ImgOrg, Pel* ImgDec, Int tap, Int iStride, Bool bFirstSlice, Bool bLastSlice); //!< Calculate block autocorrelations and crosscorrelations for one ALF slice
  Void xstoreInBlockMatrixforSlices  (Pel* ImgOrg, Pel* ImgDec, Int tap, Int iStride); //!< Calculate block autocorrelations and crosscorrelations for ALF slices
  Void xCalcCorrelationFunc(Int ypos, Int xpos, Pel* pImgOrg, Pel* pImgPad, Int filtNo, Int iWidth, Int iHeight, Int iOrgStride, Int iCmpStride, Bool bSymmCopyBlockMatrix); //!< Calculate correlations for chroma
  Void xCalcCorrelationFuncforChromaRegion(std::vector< AlfLCUInfo* > &vpAlfLCU, Pel* pOrg, Pel* pCmp, Int filtNo, Int iStride, Bool bLastSlice, Int iFormatShift); //!< Calculate autocorrelations and crosscorrelations for one chroma slice
  Void xCalcCorrelationFuncforChromaSlices  (Int ComponentID, Pel* pOrg, Pel* pCmp, Int iTap, Int iOrgStride, Int iCmpStride); //!< Calculate autocorrelations and crosscorrelations for chroma slices
#endif
  // functions related to filtering
  Void xFilterCoefQuickSort   ( Double *coef_data, Int *coef_num, Int upper, Int lower );
  Void xQuantFilterCoef       ( Double* h, Int* qh, Int tap, int bit_depth );
#if !LCU_SYNTAX_ALF
  Void xClearFilterCoefInt    ( Int* qh, Int N );
  Void xCopyDecToRestCUs      ( TComPicYuv* pcPicDec, TComPicYuv* pcPicRest );
  Void xCopyDecToRestCU       ( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth, TComPicYuv* pcPicDec, TComPicYuv* pcPicRest );
  Void xFilteringFrameChroma  (ALFParam* pcAlfParam, TComPicYuv* pcPicOrg, TComPicYuv* pcPicDec, TComPicYuv* pcPicRest);
  Int  setFilterIdx(Int index); //!< set filter buffer index
  Void setInitialMask(TComPicYuv* pcPicOrg, TComPicYuv* pcPicDec); //!< set initial m_maskImg
  Void saveFilterCoeffToBuffer(Int **filterSet, Int numFilter, Int* mergeTable, Int mode, Int filtNo); //!< save filter coefficients to buffer
  Void setMaskWithTimeDelayedResults(TComPicYuv* pcPicOrg, TComPicYuv* pcPicDec); //!< set initial m_maskImg with previous (time-delayed) filters
  Void decideFilterShapeLuma(Pel* ImgOrg, Pel* ImgDec, Int Stride, ALFParam* pcAlfSaved, UInt64& ruiRate, UInt64& ruiDist,Double& rdCost); //!< Estimate RD cost of all filter size & store the best one
  Void   xFilterTapDecisionChroma      (UInt64 uiLumaRate, TComPicYuv* pcPicOrg, TComPicYuv* pcPicDec, TComPicYuv* pcPicRest, UInt64& ruiDist, UInt64& ruiBits);
  Int64  xFastFiltDistEstimationChroma (Double** ppdCorr, Int* piCoeff, Int iSqrFiltLength);
  Void  xfilterSlicesEncoder(Pel* ImgDec, Pel* ImgRest, Int iStride, Int filtNo, Int** filterCoeff, Int* mergeTable, Pel** varImg); //!< Calculate ALF grouping indices for ALF slices
  Void  setALFEncodingParam(TComPic *pcPic); //!< set ALF encoding parameters
  Void xReDesignFilterCoeff_qc          (TComPicYuv* pcPicOrg, TComPicYuv* pcPicDec,  TComPicYuv* pcPicRest, Bool bReadCorr);
  Void xFirstFilteringFrameLuma (Pel* imgOrg, Pel* imgDec, Pel* imgRest, ALFParam* ALFp, Int filtNo, Int stride);
  Void xFilteringFrameLuma(Pel* imgOrg, Pel* imgPad, Pel* imgFilt, ALFParam* ALFp, Int filtNo, Int stride);
  Void xEncALFLuma  ( TComPicYuv* pcPicOrg, TComPicYuv* pcPicDec, TComPicYuv* pcPicRest, UInt64& ruiMinRate, UInt64& ruiMinDist, Double& rdMinCost ); //!< estimate adaptation mode and filter shape
#endif
  // distortion / misc functions
#if HHI_INTERVIEW_SKIP
  UInt64 xCalcSSD             ( Pel* pOrg, Pel* pCmp, Pel* pUsed, Int iWidth, Int iHeight, Int iStride );
#else
  UInt64 xCalcSSD             ( Pel* pOrg, Pel* pCmp, Int iWidth, Int iHeight, Int iStride );
#endif
#if !LCU_SYNTAX_ALF
#if HHI_INTERVIEW_SKIP
  Void  xCalcRDCost          ( TComPicYuv* pcPicOrg, TComPicYuv* pcPicCmp, ALFParam* pAlfParam, UInt64& ruiRate, UInt64& ruiDist, Double& rdCost, std::vector<AlfCUCtrlInfo>* pvAlfCUCtrlParam = NULL);
  Void  xCalcRDCost          ( ALFParam* pAlfParam, UInt64& ruiRate, UInt64 uiDist, Double& rdCost, std::vector<AlfCUCtrlInfo>* pvAlfCUCtrlParam = NULL);
  Void  xCalcRDCostChroma    ( TComPicYuv* pcPicOrg, TComPicYuv* pcPicCmp, ALFParam* pAlfParam, UInt64& ruiRate, UInt64& ruiDist, Double& rdCost );
#else
  Void  xCalcRDCost          ( TComPicYuv* pcPicOrg, TComPicYuv* pcPicCmp, ALFParam* pAlfParam, UInt64& ruiRate, UInt64& ruiDist, Double& rdCost, std::vector<AlfCUCtrlInfo>* pvAlfCUCtrlParam = NULL);
  Void  xCalcRDCost          ( ALFParam* pAlfParam, UInt64& ruiRate, UInt64 uiDist, Double& rdCost, std::vector<AlfCUCtrlInfo>* pvAlfCUCtrlParam = NULL);
  Void  xCalcRDCostChroma    ( TComPicYuv* pcPicOrg, TComPicYuv* pcPicCmp, ALFParam* pAlfParam, UInt64& ruiRate, UInt64& ruiDist, Double& rdCost );
#endif
#endif
  Int64 xFastFiltDistEstimation(Double** ppdE, Double* pdy, Int* piCoeff, Int iFiltLength); //!< Estimate filtering distortion by correlation values and filter coefficients
#if !LCU_SYNTAX_ALF
  Int64 xEstimateFiltDist      (Int filters_per_fr, Int* VarIndTab, Double*** pppdE, Double** ppdy, Int** ppiCoeffSet, Int iFiltLength); //!< Estimate total filtering cost of all groups  
  UInt64 xCalcRateChroma               (ALFParam* pAlfParam);
  Void   xCalcALFCoeffChroma           (Int iChromaIdc, Int iShape, Int* piCoeff);
#endif

  /// code filter coefficients
  UInt xcodeFiltCoeff(Int **filterCoeffSymQuant, Int filter_shape, Int varIndTab[], Int filters_per_fr_best, ALFParam* ALFp);
#if LCU_SYNTAX_ALF
  Void xfindBestFilterVarPred(double **ySym, double ***ESym, double *pixAcc, Int **filterCoeffSym, Int **filterCoeffSymQuant,Int filter_shape, Int *filters_per_fr_best, Int varIndTab[], Pel **imgY_rec, Pel **varImg, Pel **maskImg, Pel **imgY_pad, double lambda_val, Int numMaxFilters = NO_FILTERS);
#else
  Void xfindBestFilterVarPred(double **ySym, double ***ESym, double *pixAcc, Int **filterCoeffSym, Int **filterCoeffSymQuant,Int filter_shape, Int *filters_per_fr_best, Int varIndTab[], Pel **imgY_rec, Pel **varImg, Pel **maskImg, Pel **imgY_pad, double lambda_val);
#endif
  double xfindBestCoeffCodMethod(int **filterCoeffSymQuant, int filter_shape, int sqrFiltLength, int filters_per_fr, double errorForce0CoeffTab[NO_VAR_BINS][2], double lambda);
  Int xsendAllFiltersPPPred(int **FilterCoeffQuant, int filter_shape, int sqrFiltLength, int filters_per_group, int createBistream, ALFParam* ALFp);
  Int xcodeAuxInfo(int filters_per_fr, int varIndTab[NO_VAR_BINS], int filter_shape, ALFParam* ALFp);
  Int xcodeFilterCoeff(int **pDiffQFilterCoeffIntPP, int filter_shape, int sqrFiltLength, int filters_per_group, int createBitstream);
  Int lengthGolomb(int coeffVal, int k);
  Int lengthPredFlags(int force0, int predMethod, int codedVarBins[NO_VAR_BINS], int filters_per_group, int createBitstream);
  Int lengthFilterCoeffs(int sqrFiltLength, int filters_per_group, int pDepthInt[], int **FilterCoeff, int kMinTab[], int createBitstream);
  Void predictALFCoeffLumaEnc(ALFParam* pcAlfParam, Int **pfilterCoeffSym, Int filter_shape); //!< prediction of luma ALF coefficients
  //cholesky related
  Int   xGauss( Double **a, Int N );
  Double findFilterCoeff(double ***EGlobalSeq, double **yGlobalSeq, double *pixAccGlobalSeq, int **filterCoeffSeq,int **filterCoeffQuantSeq, int intervalBest[NO_VAR_BINS][2], int varIndTab[NO_VAR_BINS], int sqrFiltLength, int filters_per_fr, int *weights, double errorTabForce0Coeff[NO_VAR_BINS][2]);
  Double QuantizeIntegerFilterPP(Double *filterCoeff, Int *filterCoeffQuant, Double **E, Double *y, Int sqrFiltLength, Int *weights);
  Void roundFiltCoeff(int *FilterCoeffQuan, double *FilterCoeff, int sqrFiltLength, int factor);
  double mergeFiltersGreedy(double **yGlobalSeq, double ***EGlobalSeq, double *pixAccGlobalSeq, int intervalBest[NO_VAR_BINS][2], int sqrFiltLength, int noIntervals);
  double calculateErrorAbs(double **A, double *b, double y, int size);
  double calculateErrorCoeffProvided(double **A, double *b, double *c, int size);
  Void add_b(double *bmerged, double **b, int start, int stop, int size);
  Void add_A(double **Amerged, double ***A, int start, int stop, int size);
  Int  gnsSolveByChol(double **LHS, double *rhs, double *x, int noEq);
  Void gnsBacksubstitution(double R[ALF_MAX_NUM_COEF][ALF_MAX_NUM_COEF], double z[ALF_MAX_NUM_COEF], int R_size, double A[ALF_MAX_NUM_COEF]);
  Void gnsTransposeBacksubstitution(double U[ALF_MAX_NUM_COEF][ALF_MAX_NUM_COEF], double rhs[], double x[],int order);
  Int  gnsCholeskyDec(double **inpMatr, double outMatr[ALF_MAX_NUM_COEF][ALF_MAX_NUM_COEF], int noEq);

public:
  TEncAdaptiveLoopFilter          ();
  virtual ~TEncAdaptiveLoopFilter () {}

  Void startALFEnc(TComPic* pcPic, TEncEntropy* pcEntropyCoder); //!< allocate temporal memory
  Void endALFEnc(); //!< destroy temporal memory
#if LCU_SYNTAX_ALF
#if ALF_CHROMA_LAMBDA
#if HHI_INTERVIEW_SKIP
  Void ALFProcess( AlfParamSet* alfParamSet, std::vector<AlfCUCtrlInfo>* alfCtrlParam, Double lambdaLuma, Double lambdaChroma, Bool bInterviewSkip);
#else
  Void ALFProcess( AlfParamSet* alfParamSet, std::vector<AlfCUCtrlInfo>* alfCtrlParam, Double lambdaLuma, Double lambdaChroma);
#endif
#else
#if HHI_INTERVIEW_SKIP
  Void ALFProcess( AlfParamSet* alfParamSet, std::vector<AlfCUCtrlInfo>* alfCtrlParam, Double lambda, Bool bInterviewSkip);
#else
  Void ALFProcess( AlfParamSet* alfParamSet, std::vector<AlfCUCtrlInfo>* alfCtrlParam, Double lambda);
#endif
#endif
  Void initALFEnc(Bool isAlfParamInSlice, Bool isPicBasedEncode, Int numSlices, AlfParamSet* & alfParams, std::vector<AlfCUCtrlInfo>* & alfCUCtrlParam);
  Void uninitALFEnc(AlfParamSet* & alfParams, std::vector<AlfCUCtrlInfo>* & alfCUCtrlParam);
  Void resetPicAlfUnit();
  Void setAlfCoefInSlice(Bool b) {m_alfCoefInSlice = b;}
#else
#if ALF_CHROMA_LAMBDA  
#if HHI_INTERVIEW_SKIP
  Void ALFProcess(ALFParam* pcAlfParam, std::vector<AlfCUCtrlInfo>* pvAlfCtrlParam, Double dLambdaLuma, Double dLambdaChroma, UInt64& ruiDist, UInt64& ruiBits, Bool bInterviewSkip); //!< estimate ALF parameters
#else
  Void ALFProcess(ALFParam* pcAlfParam, std::vector<AlfCUCtrlInfo>* pvAlfCtrlParam, Double dLambdaLuma, Double dLambdaChroma, UInt64& ruiDist, UInt64& ruiBits); //!< estimate ALF parameters
#endif
#else
#if HHI_INTERVIEW_SKIP
  Void ALFProcess(ALFParam* pcAlfParam, std::vector<AlfCUCtrlInfo>* pvAlfCtrlParam, Double dLambda, UInt64& ruiDist, UInt64& ruiBits, Bool bInterviewSkip); //!< estimate ALF parameters
#else
  Void ALFProcess(ALFParam* pcAlfParam, std::vector<AlfCUCtrlInfo>* pvAlfCtrlParam, Double dLambda, UInt64& ruiDist, UInt64& ruiBits); //!< estimate ALF parameters
#endif
#endif

  Void setGOPSize(Int val) { m_iGOPSize = val; } //!< set GOP size
#endif
  Void setALFEncodePassReduction (Int iVal) {m_iALFEncodePassReduction = iVal;} //!< set N-pass encoding. 0: 16(14)-pass encoding, 1: 1-pass encoding, 2: 2-pass encoding

  Void setALFMaxNumberFilters    (Int iVal) {m_iALFMaxNumberFilters = iVal;} //!< set ALF Max Number of Filters

#if LCU_SYNTAX_ALF
  Void createAlfGlobalBuffers(); //!< create ALF global buffers
  Void initPicQuadTreePartition(Bool isPicBasedEncode);
#else
  Void createAlfGlobalBuffers(Int iALFEncodePassReduction); //!< create ALF global buffers
#endif
  Void destroyAlfGlobalBuffers(); //!< destroy ALF global buffers
  Void PCMLFDisableProcess (TComPic* pcPic);
};

//! \}

#endif