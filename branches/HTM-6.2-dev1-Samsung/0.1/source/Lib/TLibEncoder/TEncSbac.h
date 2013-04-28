

/** \file     TEncSbac.h
    \brief    Context-adaptive entropy encoder class (header)
*/

#ifndef __TENCSBAC__
#define __TENCSBAC__

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "../TLibCommon/TComBitStream.h"
#include "../TLibCommon/ContextTables.h"
#include "../TLibCommon/ContextModel.h"
#include "../TLibCommon/ContextModel3DBuffer.h"
#include "TEncEntropy.h"
#include "TEncBinCoder.h"
#include "TEncBinCoderCABAC.h"

class TEncTop;

// ====================================================================================================================
// Class definition
// ====================================================================================================================

/// SBAC encoder class
class TEncSbac : public TEncEntropyIf
{
public:
  TEncSbac();
  virtual ~TEncSbac();
  
  Void  init                   ( TEncBinIf* p )  { m_pcBinIf = p; }
  Void  uninit                 ()                { m_pcBinIf = 0; }
  
  //  Virtual list
  Void  resetEntropy           ();
  Void  setBitstream           ( TComBitIf* p )  { m_pcBitIf = p; m_pcBinIf->init( p ); }
  Void  setSlice               ( TComSlice* p )  { m_pcSlice = p;                       }
  
  Bool  getAlfCtrl             ()                         { return m_bAlfCtrl;          }
  UInt  getMaxAlfCtrlDepth     ()                         { return m_uiMaxAlfCtrlDepth; }
  Void  setAlfCtrl             ( Bool bAlfCtrl          ) { m_bAlfCtrl          = bAlfCtrl;          }
  Void  setMaxAlfCtrlDepth     ( UInt uiMaxAlfCtrlDepth ) { m_uiMaxAlfCtrlDepth = uiMaxAlfCtrlDepth; }
  
  // SBAC RD
  Void  resetCoeffCost         ()                { m_uiCoeffCost = 0;  }
  UInt  getCoeffCost           ()                { return  m_uiCoeffCost;  }
  
  Void  load                   ( TEncSbac* pScr  );
  Void  store                  ( TEncSbac* pDest );
  Void  resetBits              ()                { m_pcBinIf->resetBits(); m_pcBitIf->resetBits(); }
  UInt  getNumberOfWrittenBits ()                { return m_pcBinIf->getNumWrittenBits(); }
  //--SBAC RD
  
  Void  codeNALUnitHeader       ( NalUnitType eNalUnitType, NalRefIdc eNalRefIdc, UInt TemporalId = 0, Bool bOutputFlag = true );

  Void  codeSPS                 ( TComSPS* pcSPS     );
  Void  codePPS                 ( TComPPS* pcPPS     );
  void codeSEI(const SEI&);
  Void  codeSliceHeader         ( TComSlice* pcSlice );
  Void  codeTerminatingBit      ( UInt uilsLast      );
  Void  codeSliceFinish         ();
  
  Void  codeAlfFlag       ( UInt uiCode );
  Void  codeAlfUvlc       ( UInt uiCode );
  Void  codeAlfSvlc       ( Int  uiCode );
  Void  codeAlfCtrlDepth  ();
#if TSB_ALF_HEADER
  Void codeAlfFlagNum        ( UInt uiCode, UInt minValue );
  Void codeAlfCtrlFlag       ( UInt uiSymbol );
#endif
#if MTK_SAO
  Void  codeAoFlag       ( UInt uiCode );
  Void  codeAoUvlc       ( UInt uiCode );
  Void  codeAoSvlc       ( Int  uiCode );
#endif

private:
  Void  xWriteUnarySymbol    ( UInt uiSymbol, ContextModel* pcSCModel, Int iOffset );
  Void  xWriteUnaryMaxSymbol ( UInt uiSymbol, ContextModel* pcSCModel, Int iOffset, UInt uiMaxSymbol );
  Void  xWriteEpExGolomb     ( UInt uiSymbol, UInt uiCount );
#if E253
  Void  xWriteGoRiceExGolomb ( UInt uiSymbol, UInt &ruiGoRiceParam );
#if HHI_DMM_INTRA
  Void  xWriteExGolombLevel  ( UInt uiSymbol, ContextModel& rcSCModel  );
#endif
#else
  Void  xWriteExGolombLevel  ( UInt uiSymbol, ContextModel& rcSCModel  );
#endif
  Void  xWriteTerminatingBit ( UInt uiBit );
  
  Void  xCheckCoeff( TCoeff* pcCoef, UInt uiSize, UInt uiDepth, UInt& uiNumofCoeff, UInt& uiPart );

#if MVD_CTX
  Void  xWriteMvd            ( Int iMvd, UInt uiAbsSumL, UInt uiAbsSumA, UInt uiCtx );
#else
  Void  xWriteMvd            ( Int iMvd, UInt uiAbsSum, UInt uiCtx );
#endif
  Void  xWriteExGolombMvd    ( UInt uiSymbol, ContextModel* pcSCModel, UInt uiMaxBin );
  Void  xCopyFrom            ( TEncSbac* pSrc );
  
#if HHI_DMM_INTRA
  Void  xCodeWedgeFullInfo   ( TComDataCU* pcCU, UInt uiAbsPartIdx );
  Void  xCodeWedgeFullDeltaInfo     ( TComDataCU* pcCU, UInt uiAbsPartIdx );
  Void  xCodeWedgePredDirInfo       ( TComDataCU* pcCU, UInt uiAbsPartIdx );
  Void  xCodeWedgePredDirDeltaInfo  ( TComDataCU* pcCU, UInt uiAbsPartIdx );
  Void  xCodeWedgePredTexDeltaInfo  ( TComDataCU* pcCU, UInt uiAbsPartIdx );
  Void  xCodeContourPredTexDeltaInfo( TComDataCU* pcCU, UInt uiAbsPartIdx );
#endif
  
protected:
  TComBitIf*    m_pcBitIf;
  TComSlice*    m_pcSlice;
  TEncBinIf*    m_pcBinIf;
  Bool          m_bAlfCtrl;
  
  //SBAC RD
  UInt          m_uiCoeffCost;
  
  // Adaptive loop filter
  UInt          m_uiMaxAlfCtrlDepth;
  //--Adaptive loop filter

public:
  Void codeAlfCtrlFlag   ( TComDataCU* pcCU, UInt uiAbsPartIdx );
  Void codeSkipFlag      ( TComDataCU* pcCU, UInt uiAbsPartIdx );
  Void codeMergeFlag     ( TComDataCU* pcCU, UInt uiAbsPartIdx );
  Void codeMergeIndex    ( TComDataCU* pcCU, UInt uiAbsPartIdx );
  Void codeMergeIndexMV  ( TComDataCU* pcCU, UInt uiAbsPartIdx );
  Void codeResPredFlag   ( TComDataCU* pcCU, UInt uiAbsPartIdx );
  Void codeSplitFlag     ( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth );
#if MW_MVI_SIGNALLING_MODE == 0
  Void codeMvInheritanceFlag( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth );
#endif
  Void codeMVPIdx        ( TComDataCU* pcCU, UInt uiAbsPartIdx, RefPicList eRefList );
  
  Void codePartSize      ( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth );
  Void codePredMode      ( TComDataCU* pcCU, UInt uiAbsPartIdx );
  
  Void codeTransformSubdivFlag ( UInt uiSymbol, UInt uiCtx );
  Void codeQtCbf               ( TComDataCU* pcCU, UInt uiAbsPartIdx, TextType eType, UInt uiTrDepth );
  Void codeQtRootCbf           ( TComDataCU* pcCU, UInt uiAbsPartIdx );
  
  Void codeIntraDirLumaAng     ( TComDataCU* pcCU, UInt uiAbsPartIdx );
  
  Void codeIntraDirChroma      ( TComDataCU* pcCU, UInt uiAbsPartIdx );
  Void codeInterDir            ( TComDataCU* pcCU, UInt uiAbsPartIdx );
  Void codeRefFrmIdx           ( TComDataCU* pcCU, UInt uiAbsPartIdx, RefPicList eRefList );
  Void codeMvd                 ( TComDataCU* pcCU, UInt uiAbsPartIdx, RefPicList eRefList );
  
  Void codeViewIdx 			 (Int iViewIdx);

  Void codeDeltaQP             ( TComDataCU* pcCU, UInt uiAbsPartIdx );
  Void codeCbf                 ( TComDataCU* pcCU, UInt uiAbsPartIdx, TextType eType, UInt uiTrDepth ) {}
  Void codeBlockCbf            ( TComDataCU* pcCU, UInt uiAbsPartIdx, TextType eType, UInt uiTrDepth, UInt uiQPartNum, Bool bRD = false ) {}
  
#if CAVLC_RQT_CBP
  Void codeCbfTrdiv      ( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth ) {}
  UInt xGetFlagPattern   ( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth ) { return 0; }
#endif
#if PCP_SIGMAP_SIMPLE_LAST
  __inline Void codeLastSignificantXY ( UInt uiPosX, UInt uiPosY, const UInt uiWidth, const TextType eTType, const UInt uiCTXIdx, const UInt uiScanIdx );
#endif
  Void codeCoeffNxN            ( TComDataCU* pcCU, TCoeff* pcCoef, UInt uiAbsPartIdx, UInt uiWidth, UInt uiHeight, UInt uiDepth, TextType eTType, Bool bRD = false );
  
  // -------------------------------------------------------------------------------------------------------------------
  // for RD-optimizatioon
  // -------------------------------------------------------------------------------------------------------------------
  
  Void estBit                        ( estBitsSbacStruct* pcEstBitsSbac, UInt uiCTXIdx, TextType eTType );
  Void estCBFBit                     ( estBitsSbacStruct* pcEstBitsSbac, UInt uiCTXIdx, TextType eTType );
  Void estSignificantMapBit          ( estBitsSbacStruct* pcEstBitsSbac, UInt uiCTXIdx, TextType eTType );
  Void estSignificantCoefficientsBit ( estBitsSbacStruct* pcEstBitsSbac, UInt uiCTXIdx, TextType eTType );
  
  __inline Int  biari_no_bits        ( Short symbol, ContextModel& rcSCModel );
  
  TEncBinIf* getEncBinIf()  { return m_pcBinIf; }
private:
  UInt                 m_uiLastQp;
  ContextModel3DBuffer m_cCUSplitFlagSCModel;
#if MW_MVI_SIGNALLING_MODE == 0
  ContextModel3DBuffer m_cCUMvInheritanceFlagSCModel;
#endif
  ContextModel3DBuffer m_cCUSkipFlagSCModel;
  ContextModel3DBuffer m_cCUMergeFlagExtSCModel;
  ContextModel3DBuffer m_cCUMergeIdxExtSCModel;
  ContextModel3DBuffer m_cCUMVMergeIdxExtSCModel;
  ContextModel3DBuffer m_cResPredFlagSCModel;
  ContextModel3DBuffer m_cCUPartSizeSCModel;
  ContextModel3DBuffer m_cCUPredModeSCModel;
  ContextModel3DBuffer m_cCUAlfCtrlFlagSCModel;
  ContextModel3DBuffer m_cCUIntraPredSCModel;
#if ADD_PLANAR_MODE
  ContextModel3DBuffer m_cPlanarFlagSCModel;
#endif
  ContextModel3DBuffer m_cCUChromaPredSCModel;
  ContextModel3DBuffer m_cCUDeltaQpSCModel;
  ContextModel3DBuffer m_cCUInterDirSCModel;
  ContextModel3DBuffer m_cCURefPicSCModel;
  ContextModel3DBuffer m_cCUMvdSCModel;
  ContextModel3DBuffer m_cCUQtCbfSCModel;
  ContextModel3DBuffer m_cCUTransSubdivFlagSCModel;
  ContextModel3DBuffer m_cCUQtRootCbfSCModel;
  
  ContextModel3DBuffer m_cCUSigSCModel;
#if PCP_SIGMAP_SIMPLE_LAST
  ContextModel3DBuffer m_cCuCtxLastX;
  ContextModel3DBuffer m_cCuCtxLastY;
#else
  ContextModel3DBuffer m_cCULastSCModel;
#endif
  ContextModel3DBuffer m_cCUOneSCModel;
  ContextModel3DBuffer m_cCUAbsSCModel;
  
  ContextModel3DBuffer m_cMVPIdxSCModel;
  
  ContextModel3DBuffer m_cALFFlagSCModel;
  ContextModel3DBuffer m_cALFUvlcSCModel;
  ContextModel3DBuffer m_cALFSvlcSCModel;
#if MTK_SAO
  ContextModel3DBuffer m_cAOFlagSCModel;
  ContextModel3DBuffer m_cAOUvlcSCModel;
  ContextModel3DBuffer m_cAOSvlcSCModel;
#endif
  ContextModel3DBuffer m_cViewIdxSCModel;
#if HHI_DMM_INTRA
  ContextModel3DBuffer m_cIntraDMMSCModel;
  ContextModel3DBuffer m_cIntraWedgeSCModel;
#endif
};

#endif // !defined(AFX_TENCSBAC_H__DDA7CDC4_EDE3_4015_9D32_2156249C82AA__INCLUDED_)
