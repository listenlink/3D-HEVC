

/** \file     TComTrQuant.h
    \brief    transform and quantization class (header)
*/

#ifndef __TCOMTRQUANT__
#define __TCOMTRQUANT__

#include "CommonDef.h"
#include "TComYuv.h"
#include "TComDataCU.h"

// ====================================================================================================================
// Constants
// ====================================================================================================================

#define QP_BITS                 15

// AQO Parameter
#define QOFFSET_BITS            15
#define QOFFSET_BITS_LTR        9

// LTR Butterfly Paramter
#define ECore16Shift            10
#define DCore16Shift            10
#define ECore32Shift            10
#define DCore32Shift            10

#define DenShift16              6
#define DenShift32              8

// ====================================================================================================================
// Type definition
// ====================================================================================================================

typedef struct
{
  Int significantBits[16][2];
#if PCP_SIGMAP_SIMPLE_LAST
  Int lastXBits[32];
  Int lastYBits[32];
#else  
  Int lastBits[16][2];
#endif
  Int greaterOneBits[6][2][5][2];
  Int blockCbpBits[45][2];
  Int blockRootCbpBits[4][2];
  Int scanZigzag[2];            ///< flag for zigzag scan
  Int scanNonZigzag[2];         ///< flag for non zigzag scan
} estBitsSbacStruct;

#if QC_MOD_LCEC_RDOQ
typedef struct
{
  Int level[4];
  Int pre_level;
  Int coeff_ctr;
  Long levelDouble;
  Double errLevel[4];
  Int noLevels;
  Long levelQ;
  Bool lowerInt;
  UInt quantInd;
} levelDataStruct;

typedef struct
{
  Int run;
  Int maxrun;
  Int nextLev;
  Int nexLevelVal;
} quantLevelStruct;
#endif



class TEncCavlc;

// ====================================================================================================================
// Class definition
// ====================================================================================================================

/// QP class
class QpParam
{
public:
  QpParam();
  
  Int m_iQP;
  Int m_iPer;
  Int m_iRem;
  
  Int m_iAdd2x2;
  Int m_iAdd4x4;
  Int m_iAdd8x8;
  Int m_iAdd16x16;
  Int m_iAdd32x32;
private:
  Int m_aiAdd2x2[MAX_QP+1][3];
  Int m_aiAdd4x4[MAX_QP+1][3];
  Int m_aiAdd8x8[MAX_QP+1][3];
  Int m_aiAdd16x16[MAX_QP+1][3];
  Int m_aiAdd32x32[MAX_QP+1][3];
public:
  Int m_iBits;
  
  Void initOffsetParam(Int iStartQP = MIN_QP, Int iEndQP = MAX_QP );
  Void setQOffset( Int iQP, SliceType eSliceType )
  {
    m_iAdd2x2 = m_aiAdd2x2[iQP][eSliceType];
    m_iAdd4x4 = m_aiAdd4x4[iQP][eSliceType];
    m_iAdd8x8 = m_aiAdd8x8[iQP][eSliceType];
    m_iAdd16x16 = m_aiAdd16x16[iQP][eSliceType];
    m_iAdd32x32 = m_aiAdd32x32[iQP][eSliceType];
  }
  
  Void setQpParam( Int iQP, Bool bLowpass, SliceType eSliceType, Bool bEnc )
  {
    assert ( iQP >= MIN_QP && iQP <= MAX_QP );
    m_iQP   = iQP;
    
    m_iPer  = (iQP + 6*g_uiBitIncrement)/6;
#if FULL_NBIT
    m_iPer += g_uiBitDepth - 8;
#endif
    m_iRem  = (iQP + 6*g_uiBitIncrement)%6;
    
    m_iBits = QP_BITS + m_iPer;
    
    if ( bEnc )
    {
      setQOffset(iQP, eSliceType);
    }
  }
  
  Void clear()
  {
    m_iQP   = 0;
    m_iPer  = 0;
    m_iRem  = 0;
    m_iBits = 0;
  }
  
  
  const Int per()   const { return m_iPer; }
  const Int rem()   const { return m_iRem; }
  const Int bits()  const { return m_iBits; }
  
  Int qp() {return m_iQP;}
}; // END CLASS DEFINITION QpParam

/// transform and quantization class
class TComTrQuant
{
public:
  TComTrQuant();
  ~TComTrQuant();
  
  // initialize class
#if QC_MOD_LCEC
  Void init                 ( UInt uiMaxWidth, UInt uiMaxHeight, UInt uiMaxTrSize, Int iSymbolMode = 0, UInt *aTable4 = NULL, UInt *aTable8 = NULL, UInt *aTableLastPosVlcIndex=NULL, Bool bUseRDOQ = false,  Bool bEnc = false );
#else
  Void init                 ( UInt uiMaxWidth, UInt uiMaxHeight, UInt uiMaxTrSize, Int iSymbolMode = 0, UInt *aTable4 = NULL, UInt *aTable8 = NULL, Bool bUseRDOQ = false,  Bool bEnc = false );
#endif
  
  // transform & inverse transform functions
  Void transformNxN         ( TComDataCU* pcCU, Pel*   pcResidual, UInt uiStride, TCoeff*& rpcCoeff, UInt uiWidth, UInt uiHeight,
                             UInt& uiAbsSum, TextType eTType, UInt uiAbsPartIdx );
#if INTRA_DST_TYPE_7
  Void invtransformNxN      (TextType eText, UInt uiMode,Pel*& rpcResidual, UInt uiStride, TCoeff*   pcCoeff, UInt uiWidth, UInt uiHeight);
#else
  Void invtransformNxN      ( Pel*& rpcResidual, UInt uiStride, TCoeff*   pcCoeff, UInt uiWidth, UInt uiHeight );
#endif
  Void invRecurTransformNxN ( TComDataCU* pcCU, UInt uiAbsPartIdx, TextType eTxt, Pel*& rpcResidual, UInt uiAddr,   UInt uiStride, UInt uiWidth, UInt uiHeight,
                             UInt uiMaxTrMode,  UInt uiTrMode, TCoeff* rpcCoeff );
  
  // Misc functions
  Void setQPforQuant( Int iQP, Bool bLowpass, SliceType eSliceType, TextType eTxtType);
  Void setLambda(Double dLambda) { m_dLambda = dLambda;}
#if QC_MOD_LCEC_RDOQ
  Void    setRDOQOffset ( UInt uiRDOQOffset ) { m_uiRDOQOffset = uiRDOQOffset; }
#endif
  estBitsSbacStruct* m_pcEstBitsSbac;
  
  static UInt     getSigCtxInc     ( TCoeff*                         pcCoeff,
                                     const UInt                      uiPosX,
                                     const UInt                      uiPosY,
                                     const UInt                      uiLog2BlkSize,
                                     const UInt                      uiStride );
#if !PCP_SIGMAP_SIMPLE_LAST
  static UInt     getLastCtxInc    ( const UInt                      uiPosX,
                                     const UInt                      uiPosY,
                                     const UInt                      uiLog2BlkSize );
#endif
protected:
  Long*    m_plTempCoeff;
  UInt*    m_puiQuantMtx;
  
  QpParam  m_cQP;
  Double   m_dLambda;
#if QC_MOD_LCEC_RDOQ
  UInt     m_uiRDOQOffset;
#endif
  UInt     m_uiMaxTrSize;
  Bool     m_bEnc;
  Bool     m_bUseRDOQ;
  
  UInt     *m_uiLPTableE8;
  UInt     *m_uiLPTableE4;
  Int      m_iSymbolMode;
#if QC_MOD_LCEC
  UInt     *m_uiLastPosVlcIndex;
#endif
  
private:
  // forward Transform
#if INTRA_DST_TYPE_7
  Void xT   ( UInt uiMode,Pel* pResidual, UInt uiStride, Long* plCoeff, Int iSize );
#else
  Void xT   ( Pel* pResidual, UInt uiStride, Long* plCoeff, Int iSize );
#endif
  Void xT2  ( Pel* pResidual, UInt uiStride, Long* plCoeff );
  Void xT4  ( Pel* pResidual, UInt uiStride, Long* plCoeff );
  Void xT8  ( Pel* pResidual, UInt uiStride, Long* plCoeff );
  Void xT16 ( Pel* pResidual, UInt uiStride, Long* plCoeff );
  Void xT32 ( Pel* pResidual, UInt uiStride, Long* plCoeff );
  
  // quantization
  Void xQuant     ( TComDataCU* pcCU, Long* pSrc, TCoeff*& pDes, Int iWidth, Int iHeight, UInt& uiAcSum, TextType eTType, UInt uiAbsPartIdx );
  Void xQuantLTR  ( TComDataCU* pcCU, Long* pSrc, TCoeff*& pDes, Int iWidth, Int iHeight, UInt& uiAcSum, TextType eTType, UInt uiAbsPartIdx );
  Void xQuant2x2  ( Long* plSrcCoef, TCoeff*& pDstCoef, UInt& uiAbsSum );
  Void xQuant4x4  ( TComDataCU* pcCU, Long* plSrcCoef, TCoeff*& pDstCoef, UInt& uiAbsSum, TextType eTType, UInt uiAbsPartIdx );
  Void xQuant8x8  ( TComDataCU* pcCU, Long* plSrcCoef, TCoeff*& pDstCoef, UInt& uiAbsSum, TextType eTType, UInt uiAbsPartIdx );


  // RDOQ functions

#if QC_MOD_LCEC_RDOQ
  Int            xCodeCoeffCountBitsLast(TCoeff* scoeff, levelDataStruct* levelData, Int nTab, UInt uiNoCoeff);
  UInt           xCountVlcBits(UInt uiTableNumber, UInt uiCodeNumber);
#if CAVLC_COEF_LRG_BLK
  Int            bitCountRDOQ(Int coeff, Int pos, Int nTab, Int lastCoeffFlag,Int levelMode,Int run, Int maxrun, Int vlc_adaptive, Int N, 
                              UInt uiTr1, Int iSum_big_coef, Int iBlockType, TComDataCU* pcCU, const UInt **pLumaRunTr1);
#else
  Int            bitCountRDOQ(Int coeff, Int pos, Int nTab, Int lastCoeffFlag,Int levelMode,Int run, Int maxrun, Int vlc_adaptive, Int N, 
                              UInt uiTr1, Int iSum_big_coef, Int iBlockType, TComDataCU* pcCU);
#endif
#else
#if QC_MOD_LCEC 
  Int            bitCount_LCEC(Int k,Int pos,Int nTab, Int lpflag,Int levelMode,Int run, Int maxrun, Int vlc_adaptive, Int N, UInt uiTr1);
#else
  Int            bitCount_LCEC(Int k,Int pos,Int n,Int lpflag,Int levelMode,Int run,Int maxrun,Int vlc_adaptive,Int N);
#endif
#endif
#if QC_MDCS
UInt             getCurrLineNum(UInt uiScanIdx, UInt uiPosX, UInt uiPosY);
#endif
  Void           xRateDistOptQuant_LCEC ( TComDataCU*                     pcCU,
                                          Long*                           plSrcCoeff,
                                          TCoeff*&                        piDstCoeff,
                                          UInt                            uiWidth,
                                          UInt                            uiHeight,
                                          UInt&                           uiAbsSum,
                                          TextType                        eTType,
                                          UInt                            uiAbsPartIdx );
  
  Void           xRateDistOptQuant ( TComDataCU*                     pcCU,
                                     Long*                           plSrcCoeff,
                                     TCoeff*&                        piDstCoeff,
                                     UInt                            uiWidth,
                                     UInt                            uiHeight,
                                     UInt&                           uiAbsSum,
                                     TextType                        eTType,
                                     UInt                            uiAbsPartIdx );
  __inline UInt  xGetCodedLevel    ( Double&                         rd64UncodedCost,
                                     Double&                         rd64CodedCost,
#if PCP_SIGMAP_SIMPLE_LAST
                                     Double&                         rd64CodedLastCost,
                                     UInt&                           ruiBestNonZeroLevel,
                                     Long                            lLevelDouble,
                                     UInt                            uiMaxAbsLevel,
#else
                                     Long                            lLevelDouble,
                                     UInt                            uiMaxAbsLevel,
                                     bool                            bLastScanPos,
#endif
                                     UShort                          ui16CtxNumSig,
                                     UShort                          ui16CtxNumOne,
                                     UShort                          ui16CtxNumAbs,
#if E253
                                     UShort                          ui16AbsGoRice,
#endif
                                     Int                             iQBits,
                                     Double                          dTemp,
                                     UShort                          ui16CtxBase   ) const;
  __inline Double xGetICRateCost   ( UInt                            uiAbsLevel,
#if !PCP_SIGMAP_SIMPLE_LAST
                                     Bool                            bLastScanPos,
                                     UShort                          ui16CtxNumSig,
#endif
                                     UShort                          ui16CtxNumOne,
                                     UShort                          ui16CtxNumAbs,
#if E253
                                     UShort                          ui16AbsGoRice,
#endif
                                     UShort                          ui16CtxBase   ) const;
#if PCP_SIGMAP_SIMPLE_LAST
  __inline Double xGetRateLast     ( UInt                            uiPosX,
                                     UInt                            uiPosY        ) const;
  __inline Double xGetRateSigCoef (  UShort                          uiSignificance,
                                     UShort                          ui16CtxNumSig ) const;
#endif
  __inline Double xGetICost        ( Double                          dRate         ) const; 
  __inline Double xGetIEPRate      (                                               ) const;
  
  
  __inline Int          xRound   ( Int i )   { return ((i)+(1<<5))>>6; }
  __inline static Long  xTrRound ( Long i, UInt uiShift ) { return ((i)>>uiShift); }
  
  // dequantization
  Void xDeQuant         ( TCoeff* pSrc,     Long*& pDes,       Int iWidth, Int iHeight );
  Void xDeQuantLTR      ( TCoeff* pSrc,     Long*&  pDes,      Int iWidth, Int iHeight );
  Void xDeQuant2x2      ( TCoeff* pSrcCoef, Long*& rplDstCoef );
  Void xDeQuant4x4      ( TCoeff* pSrcCoef, Long*& rplDstCoef );
  Void xDeQuant8x8      ( TCoeff* pSrcCoef, Long*& rplDstCoef );
  
  // inverse transform
#if INTRA_DST_TYPE_7
  Void xIT    ( UInt uiMode, Long* plCoef, Pel* pResidual, UInt uiStride, Int iSize );
#else
  Void xIT    ( Long* plCoef, Pel* pResidual, UInt uiStride, Int iSize );
#endif
  Void xIT2   ( Long* plCoef, Pel* pResidual, UInt uiStride );
  Void xIT4   ( Long* plCoef, Pel* pResidual, UInt uiStride );
  Void xIT8   ( Long* plCoef, Pel* pResidual, UInt uiStride );
  Void xIT16  ( Long* plCoef, Pel* pResidual, UInt uiStride );
  Void xIT32  ( Long* plCoef, Pel* pResidual, UInt uiStride );


};// END CLASS DEFINITION TComTrQuant


#endif // __TCOMTRQUANT__

