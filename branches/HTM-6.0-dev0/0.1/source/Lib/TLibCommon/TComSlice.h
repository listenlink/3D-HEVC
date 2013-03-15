

/** \file     TComSlice.h
    \brief    slice header and SPS class (header)
*/

#ifndef __TCOMSLICE__
#define __TCOMSLICE__


#include "CommonDef.h"
#ifdef WEIGHT_PRED
  #include <string.h>	// To avoid compilation failure with some g++ linux compilators...
#endif
#include "TComList.h"
#include <math.h>

class TComPic;
class TComDepthMapGenerator;
class TComResidualGenerator;

// ====================================================================================================================
// Class definition
// ====================================================================================================================

/// SPS class
class TComSPS
{
private:
  // Structure
  UInt        m_uiSPSId;
  UInt        m_uiWidth;
  UInt        m_uiHeight;
  Int         m_aiPad[2];
  UInt        m_uiMaxCUWidth;
  UInt        m_uiMaxCUHeight;
  UInt        m_uiMaxCUDepth;
  UInt        m_uiMinTrDepth;
  UInt        m_uiMaxTrDepth;
  
  UInt        m_uiViewId;
  Int         m_iViewOrderIdx;
  Bool        m_bDepth;
  UInt        m_uiCamParPrecision;
  Bool        m_bCamParInSliceHeader;
  Int         m_aaiCodedScale [2][MAX_NUMBER_VIEWS];
  Int         m_aaiCodedOffset[2][MAX_NUMBER_VIEWS];

  // Tool list
  UInt        m_uiQuadtreeTULog2MaxSize;
  UInt        m_uiQuadtreeTULog2MinSize;
  UInt        m_uiQuadtreeTUMaxDepthInter;
  UInt        m_uiQuadtreeTUMaxDepthIntra;
  Bool        m_bUseALF;
  Bool        m_bUseDQP;
#if !SB_NO_LowDelayCoding
  Bool        m_bUseLDC;
#endif
  Bool        m_bUsePAD;
  Bool        m_bUseMRG; // SOPH:

#if LM_CHROMA 
  Bool        m_bUseLMChroma; // JL:
#endif

#if DCM_COMB_LIST
  Bool        m_bUseLComb;
  Bool        m_bLCMod;
#endif
  
#if HHI_RMP_SWITCH
  Bool        m_bUseRMP;
#endif
  // Parameter
  AMVP_MODE   m_aeAMVPMode[MAX_CU_DEPTH];
  UInt        m_uiBitDepth;
  UInt        m_uiBitIncrement;
  
  // Max physical transform size
  UInt        m_uiMaxTrSize;
  
  Int m_iAMPAcc[MAX_CU_DEPTH];

#if MTK_NONCROSS_INLOOP_FILTER
  Bool        m_bLFCrossSliceBoundaryFlag;
#endif
#if MTK_SAO
  Bool        m_bUseSAO; 
#endif
  Bool        m_bUseMVI;

  UInt m_uiCodedPictureBufferSize ;

#if HHI_DMM_INTRA
  Bool  m_bUseDepthModelModes;
#endif

  UInt  m_uiPredDepthMapGeneration;
  UInt  m_uiMultiviewMvPredMode;
  UInt  m_uiPdmPrecision;
  Int   m_aiPdmScaleNomDelta[MAX_NUMBER_VIEWS];
  Int   m_aiPdmOffset       [MAX_NUMBER_VIEWS];
  UInt  m_uiMultiviewResPredMode;

  TComDepthMapGenerator* m_pcDepthMapGenerator;
  TComResidualGenerator* m_pcResidualGenerator;

public:
  TComSPS();
  virtual ~TComSPS();
  
  // structure
  Void setSPSId       ( UInt u ) { m_uiSPSId = u;           }
  UInt getSPSId       ()         { return m_uiSPSId;        }
  Void setWidth       ( UInt u ) { m_uiWidth = u;           }
  UInt getWidth       ()         { return  m_uiWidth;       }
  Void setHeight      ( UInt u ) { m_uiHeight = u;          }
  UInt getHeight      ()         { return  m_uiHeight;      }
  Void setMaxCUWidth  ( UInt u ) { m_uiMaxCUWidth = u;      }
  UInt getMaxCUWidth  ()         { return  m_uiMaxCUWidth;  }
  Void setMaxCUHeight ( UInt u ) { m_uiMaxCUHeight = u;     }
  UInt getMaxCUHeight ()         { return  m_uiMaxCUHeight; }
  Void setMaxCUDepth  ( UInt u ) { m_uiMaxCUDepth = u;      }
  UInt getMaxCUDepth  ()         { return  m_uiMaxCUDepth;  }
  Void setMinTrDepth  ( UInt u ) { m_uiMinTrDepth = u;      }
  UInt getMinTrDepth  ()         { return  m_uiMinTrDepth;  }
  Void setMaxTrDepth  ( UInt u ) { m_uiMaxTrDepth = u;      }
  UInt getMaxTrDepth  ()         { return  m_uiMaxTrDepth;  }
  Void setQuadtreeTULog2MaxSize( UInt u ) { m_uiQuadtreeTULog2MaxSize = u;    }
  UInt getQuadtreeTULog2MaxSize()         { return m_uiQuadtreeTULog2MaxSize; }
  Void setQuadtreeTULog2MinSize( UInt u ) { m_uiQuadtreeTULog2MinSize = u;    }
  UInt getQuadtreeTULog2MinSize()         { return m_uiQuadtreeTULog2MinSize; }
  Void setQuadtreeTUMaxDepthInter( UInt u ) { m_uiQuadtreeTUMaxDepthInter = u;    }
  Void setQuadtreeTUMaxDepthIntra( UInt u ) { m_uiQuadtreeTUMaxDepthIntra = u;    }
  UInt getQuadtreeTUMaxDepthInter()         { return m_uiQuadtreeTUMaxDepthInter; }
  UInt getQuadtreeTUMaxDepthIntra()         { return m_uiQuadtreeTUMaxDepthIntra; }
  Void setPad         (Int iPad[2]) { m_aiPad[0] = iPad[0]; m_aiPad[1] = iPad[1]; }
  Void setPadX        ( Int  u ) { m_aiPad[0] = u; }
  Void setPadY        ( Int  u ) { m_aiPad[1] = u; }
  Int  getPad         ( Int  u ) { assert(u < 2); return m_aiPad[u];}
  Int* getPad         ( )        { return m_aiPad; }
  
  Void        initMultiviewSPS      ( UInt uiViewId, Int iViewOrderIdx = 0, UInt uiCamParPrecision = 0, Bool bCamParSlice = false, Int** aaiScale = 0, Int** aaiOffset = 0 );
  Void        initMultiviewSPSDepth ( UInt uiViewId, Int iViewOrderIdx );

  UInt        getViewId             ()  { return m_uiViewId; }
  Int         getViewOrderIdx       ()  { return m_iViewOrderIdx; }
  Bool        isDepth               ()  { return m_bDepth; }
  UInt        getCamParPrecision    ()  { return m_uiCamParPrecision; }
  Bool        hasCamParInSliceHeader()  { return m_bCamParInSliceHeader; }
  Int*        getCodedScale         ()  { return m_aaiCodedScale [0]; }
  Int*        getCodedOffset        ()  { return m_aaiCodedOffset[0]; }
  Int*        getInvCodedScale      ()  { return m_aaiCodedScale [1]; }
  Int*        getInvCodedOffset     ()  { return m_aaiCodedOffset[1]; }

  // physical transform
  Void setMaxTrSize   ( UInt u ) { m_uiMaxTrSize = u;       }
  UInt getMaxTrSize   ()         { return  m_uiMaxTrSize;   }
  
  // Tool list
  Bool getUseALF      ()         { return m_bUseALF;        }
  Bool getUseDQP      ()         { return m_bUseDQP;        }
  
#if !SB_NO_LowDelayCoding
  Bool getUseLDC      ()         { return m_bUseLDC;        }
#endif
  Bool getUsePAD      ()         { return m_bUsePAD;        }
  Bool getUseMRG      ()         { return m_bUseMRG;        } // SOPH:
  
  Void setUseALF      ( Bool b ) { m_bUseALF  = b;          }
  Void setUseDQP      ( Bool b ) { m_bUseDQP   = b;         }
  
#if !SB_NO_LowDelayCoding
  Void setUseLDC      ( Bool b ) { m_bUseLDC   = b;         }
#endif
  Void setUsePAD      ( Bool b ) { m_bUsePAD   = b;         }
  Void setUseMRG      ( Bool b ) { m_bUseMRG  = b;          } // SOPH:
  
#if HHI_DMM_INTRA
  Bool getUseDepthModelModes()         { return m_bUseDepthModelModes; }
  Void setUseDepthModelModes( Bool b ) { m_bUseDepthModelModes = b;    }
#endif


#if DCM_COMB_LIST
  Void setUseLComb    (Bool b)   { m_bUseLComb = b;         }
  Bool getUseLComb    ()         { return m_bUseLComb;      }
  Void setLCMod       (Bool b)   { m_bLCMod = b;     }
  Bool getLCMod       ()         { return m_bLCMod;  }
#endif

#if HHI_RMP_SWITCH
  Bool getUseRMP     ()         { return m_bUseRMP; }
  Void setUseRMP     ( Bool b ) { m_bUseRMP = b;    }
#endif
  
#if LM_CHROMA 
  Bool getUseLMChroma ()         { return m_bUseLMChroma;        }
  Void setUseLMChroma ( Bool b ) { m_bUseLMChroma  = b;          }
#endif

  // AMVP mode (for each depth)
  AMVP_MODE getAMVPMode ( UInt uiDepth ) { assert(uiDepth < g_uiMaxCUDepth);  return m_aeAMVPMode[uiDepth]; }
  Void      setAMVPMode ( UInt uiDepth, AMVP_MODE eMode) { assert(uiDepth < g_uiMaxCUDepth);  m_aeAMVPMode[uiDepth] = eMode; }
  
  // Bit-depth
  UInt      getBitDepth     ()         { return m_uiBitDepth;     }
  Void      setBitDepth     ( UInt u ) { m_uiBitDepth = u;        }
  UInt      getBitIncrement ()         { return m_uiBitIncrement; }
  Void      setBitIncrement ( UInt u ) { m_uiBitIncrement = u;    }

#if MTK_NONCROSS_INLOOP_FILTER
  Void      setLFCrossSliceBoundaryFlag     ( Bool   bValue  )    { m_bLFCrossSliceBoundaryFlag = bValue; }
  Bool      getLFCrossSliceBoundaryFlag     ()                    { return m_bLFCrossSliceBoundaryFlag;   } 
#endif

#if MTK_SAO
  Void setUseSAO                  (Bool bVal)  {m_bUseSAO = bVal;}
  Bool getUseSAO                  ()           {return m_bUseSAO;}
#endif

  Void setUseMVI                  (Bool bVal)  {m_bUseMVI = bVal;}
  Bool getUseMVI                  ()           {return m_bUseMVI;}

  Void      setCodedPictureBufferSize( UInt u ) { m_uiCodedPictureBufferSize = u ;}
  UInt      getCodedPictureBufferSize( )        { return m_uiCodedPictureBufferSize ;}

  Void  setPredDepthMapGeneration( UInt uiViewId, Bool bIsDepth, UInt uiPdmGenMode = 0, UInt uiPdmMvPredMode = 0, UInt uiPdmPrec = 0, Int** aaiPdmScaleNomDelta = 0, Int** aaiPdmOffset = 0 );
  Void  setMultiviewResPredMode  ( UInt uiResPrdMode ) { m_uiMultiviewResPredMode = uiResPrdMode; }
  
  UInt  getPredDepthMapGeneration()          { return m_uiPredDepthMapGeneration; }
  UInt  getMultiviewMvPredMode   ()          { return m_uiMultiviewMvPredMode;    }
  UInt  getPdmPrecision          ()          { return m_uiPdmPrecision;           }
  Int*  getPdmScaleNomDelta      ()          { return m_aiPdmScaleNomDelta;       }
  Int*  getPdmOffset             ()          { return m_aiPdmOffset;              }
  UInt  getMultiviewResPredMode  ()          { return m_uiMultiviewResPredMode;   }

  Void                    setDepthMapGenerator( TComDepthMapGenerator* pcDepthMapGenerator )  { m_pcDepthMapGenerator = pcDepthMapGenerator; }
  TComDepthMapGenerator*  getDepthMapGenerator()                                              { return m_pcDepthMapGenerator; }

  Void                    setResidualGenerator( TComResidualGenerator* pcResidualGenerator )  { m_pcResidualGenerator = pcResidualGenerator; }
  TComResidualGenerator*  getResidualGenerator()                                              { return m_pcResidualGenerator; }
};

/// PPS class
class TComPPS
{
private:
#if CONSTRAINED_INTRA_PRED
  Bool        m_bConstrainedIntraPred;    //  constrained_intra_pred_flag
#endif
#ifdef WEIGHT_PRED
  Bool        m_bUseWeightPred;           // Use of Weighting Prediction (P_SLICE)
  UInt        m_uiBiPredIdc;              // Use of Weighting Bi-Prediction (B_SLICE)
#endif
  UInt        m_uiPPSId;
  UInt        m_uiSPSId;
  
public:
  TComPPS();
  virtual ~TComPPS();
  
  Void      setPPSId                ( UInt u ) { m_uiPPSId = u; }
  UInt      getPPSId                ()         { return m_uiPPSId; }
  Void      setSPSId                ( UInt u ) { m_uiSPSId = u; }
  UInt      getSPSId                ()         { return m_uiSPSId; }
#if CONSTRAINED_INTRA_PRED
  Bool      getConstrainedIntraPred ()         { return  m_bConstrainedIntraPred; }
  Void      setConstrainedIntraPred ( Bool b ) { m_bConstrainedIntraPred = b;     }
#endif

#ifdef WEIGHT_PRED
  Bool getUseWP                     ()          { return m_bUseWeightPred;      }
  UInt getWPBiPredIdc               ()          { return m_uiBiPredIdc;         }

  Void setUseWP                     ( Bool b )  { m_bUseWeightPred = b;       }
  Void setWPBiPredIdc               ( UInt u )  { m_uiBiPredIdc = u;          }
#endif
};

#ifdef WEIGHT_PRED
typedef struct {
  // Explicit weighted prediction parameters parsed in slice header,
  // or Implicit weighted prediction parameters (8 bits depth values).
  Bool        bPresentFlag;
  UInt        uiLog2WeightDenom;
  Int         iWeight;
  Int         iOffset;

  // Weighted prediction scaling values built from above parameters (bitdepth scaled):
  Int         w, o, offset, shift, round;
} wpScalingParam;

typedef struct {
  Int64 iAC;
  Int64 iDC;
} wpACDCParam;
#endif

/// slice header class
class TComSlice
{
  
private:
  //  Bitstream writing
  UInt        m_uiPPSId;
  Int         m_iPOC;
#if DCM_DECODING_REFRESH
  NalUnitType m_eNalUnitType;         ///< Nal unit type for the slice
#endif
  SliceType   m_eSliceType;
  Int         m_iSliceQp;
  Int         m_iSymbolMode;
  Bool        m_bLoopFilterDisable;
  
#if DCM_COMB_LIST
  Int         m_aiNumRefIdx   [3];    //  for multiple reference of current slice

  Int         m_iRefIdxOfLC[2][MAX_NUM_REF_LC];
  Int         m_eListIdFromIdxOfLC[MAX_NUM_REF_LC];
  Int         m_iRefIdxFromIdxOfLC[MAX_NUM_REF_LC];
  Int         m_iRefIdxOfL1FromRefIdxOfL0[MAX_NUM_REF_LC];
  Int         m_iRefIdxOfL0FromRefIdxOfL1[MAX_NUM_REF_LC];
  Bool        m_bRefPicListModificationFlagLC;
  Bool        m_bRefPicListCombinationFlag;
#else
  Int         m_aiNumRefIdx   [2];    //  for multiple reference of current slice
#endif  

  //  Data
  Int         m_iSliceQpDelta;
  TComPic*    m_apcRefPicList [2][MAX_NUM_REF];
  Int         m_aiRefPOCList  [2][MAX_NUM_REF];
  Int         m_iDepth;
  TComPic*    m_pcTexturePic;
  
  // referenced slice?
  Bool        m_bRefenced;
#ifdef ROUNDING_CONTROL_BIPRED
  Bool        m_bRounding;
#endif
  
  // access channel
  TComSPS*    m_pcSPS;
  TComPPS*    m_pcPPS;
  TComPic*    m_pcPic;
  
  UInt        m_uiColDir;  // direction to get colocated CUs
  
  Double      m_dLambda;
  
  Bool        m_abEqualRef  [2][MAX_NUM_REF][MAX_NUM_REF];

  // SB
  Int         m_iViewIdx;
  Int         m_aiRefViewList[2][MAX_INPUT_VIEW_NUM];
  
  Bool        m_bNoBackPredFlag;
#if MS_LCEC_LOOKUP_TABLE_EXCEPTION
  Bool        m_bRefIdxCombineCoding;
#endif

  Int         m_aaiCodedScale [2][MAX_NUMBER_VIEWS];
  Int         m_aaiCodedOffset[2][MAX_NUMBER_VIEWS];

  UInt        m_uiSliceMode;
  UInt        m_uiSliceArgument;
  UInt        m_uiSliceCurStartCUAddr;
  UInt        m_uiSliceCurEndCUAddr;
  UInt        m_uiSliceIdx;
  UInt        m_uiEntropySliceMode;
  UInt        m_uiEntropySliceArgument;
  UInt        m_uiEntropySliceCurStartCUAddr;
  UInt        m_uiEntropySliceCurEndCUAddr;
  Bool        m_bNextSlice;
  Bool        m_bNextEntropySlice;
  UInt        m_uiSliceBits;

#ifdef WEIGHT_PRED
  wpScalingParam  m_weightPredTable[2][MAX_NUM_REF][3]; // [REF_PIC_LIST_0 or REF_PIC_LIST_1][refIdx][0:Y, 1:U, 2:V]
  wpACDCParam	m_weightACDCParam[3]; // [0:Y, 1:U, 2:V]
#endif
  
public:
  TComSlice();
  virtual ~TComSlice();
  
  Void      initSlice       ();
  
  Void      setSPS          ( TComSPS* pcSPS ) { m_pcSPS = pcSPS; }
  TComSPS*  getSPS          () { return m_pcSPS; }
  
  Void      setPPS          ( TComPPS* pcPPS ) { m_pcPPS = pcPPS; }
  TComPPS*  getPPS          () { return m_pcPPS; }
  
  UInt      getPPSId        ()                          { return  m_uiPPSId;            }
  SliceType getSliceType    ()                          { return  m_eSliceType;         }
  Int       getPOC          ()                          { return  m_iPOC;           }
  Int       getSliceQp      ()                          { return  m_iSliceQp;           }
  Int       getSliceQpDelta ()                          { return  m_iSliceQpDelta;      }
  Int       getSymbolMode   ()                          { return  m_iSymbolMode;        }
  Bool      getLoopFilterDisable()                      { return  m_bLoopFilterDisable; }
  Int       getNumRefIdx        ( RefPicList e )                { return  m_aiNumRefIdx[e];             }
  TComPic*  getPic              ()                              { return  m_pcPic;                      }
  TComPic*  getRefPic           ( RefPicList e, Int iRefIdx)    { return  m_apcRefPicList[e][iRefIdx];  }
  Int       getRefPOC           ( RefPicList e, Int iRefIdx)    { return  m_aiRefPOCList[e][iRefIdx];   }
  Int       getDepth            ()                              { return  m_iDepth;                     }
  UInt      getColDir           ()                              { return  m_uiColDir;                   }
  
#if DCM_COMB_LIST 
  Int       getRefIdxOfLC       (RefPicList e, Int iRefIdx)     { return m_iRefIdxOfLC[e][iRefIdx];           }
  Int       getListIdFromIdxOfLC(Int iRefIdx)                   { return m_eListIdFromIdxOfLC[iRefIdx];       }
  Int       getRefIdxFromIdxOfLC(Int iRefIdx)                   { return m_iRefIdxFromIdxOfLC[iRefIdx];       }
  Int       getRefIdxOfL0FromRefIdxOfL1(Int iRefIdx)            { return m_iRefIdxOfL0FromRefIdxOfL1[iRefIdx];}
  Int       getRefIdxOfL1FromRefIdxOfL0(Int iRefIdx)            { return m_iRefIdxOfL1FromRefIdxOfL0[iRefIdx];}
  Bool      getRefPicListModificationFlagLC()                   {return m_bRefPicListModificationFlagLC;}
  Void      setRefPicListModificationFlagLC(Bool bflag)         {m_bRefPicListModificationFlagLC=bflag;}     
  Bool      getRefPicListCombinationFlag()                      {return m_bRefPicListCombinationFlag;}
  Void      setRefPicListCombinationFlag(Bool bflag)            {m_bRefPicListCombinationFlag=bflag;}     
  Void      setListIdFromIdxOfLC(Int  iRefIdx, UInt uiVal)      { m_eListIdFromIdxOfLC[iRefIdx]=uiVal; }
  Void      setRefIdxFromIdxOfLC(Int  iRefIdx, UInt uiVal)      { m_iRefIdxFromIdxOfLC[iRefIdx]=uiVal; }
  Void      setRefIdxOfLC       (RefPicList e, Int iRefIdx, Int RefIdxLC)     { m_iRefIdxOfLC[e][iRefIdx]=RefIdxLC;}
#endif

  Void      setReferenced(Bool b)                               { m_bRefenced = b; }
  Bool      isReferenced()                                      { return m_bRefenced; }
#ifdef ROUNDING_CONTROL_BIPRED
  Void      setRounding(Bool bRound)                            { m_bRounding = bRound; }
  Bool      isRounding()                                        { return m_bRounding; }
#endif
  
  Void      setPPSId            ( UInt u )                      { m_uiPPSId           = u;      }
  Void      setPOC              ( Int i )                       { m_iPOC              = i;      }
#if DCM_DECODING_REFRESH
  Void      setNalUnitType      ( NalUnitType e )               { m_eNalUnitType      = e;      }
  NalUnitType getNalUnitType    ()                              { return m_eNalUnitType;        }
  Void      decodingRefreshMarking(UInt& uiPOCCDR, Bool& bRefreshPending, TComList<TComPic*>& rcListPic);
#endif
  Void      setSliceType        ( SliceType e )                 { m_eSliceType        = e;      }
  Void      setSliceQp          ( Int i )                       { m_iSliceQp          = i;      }
  Void      setSliceQpDelta     ( Int i )                       { m_iSliceQpDelta     = i;      }
  Void      setSymbolMode       ( Int b  )                      { m_iSymbolMode       = b;      }
  Void      setLoopFilterDisable( Bool b )                      { m_bLoopFilterDisable= b;      }
  
  Void      setRefPic           ( TComPic* p, RefPicList e, Int iRefIdx ) { m_apcRefPicList[e][iRefIdx] = p; }
  Void      setRefPOC           ( Int i, RefPicList e, Int iRefIdx ) { m_aiRefPOCList[e][iRefIdx] = i; }
  Void      setNumRefIdx        ( RefPicList e, Int i )         { m_aiNumRefIdx[e]    = i;      }
  Void      setPic              ( TComPic* p )                  { m_pcPic             = p;      }
  Void      setDepth            ( Int iDepth )                  { m_iDepth            = iDepth; }
  
  Void      setRefPicList       ( TComList<TComPic*>& rcListPic );
  Void      setRefPOCList       ();
  Void      setColDir           ( UInt uiDir ) { m_uiColDir = uiDir; }
  
  Void      setRefPicListFromGOPSTring( TComList<TComPic*>& rcListPic, std::vector<TComPic*>& rapcSpatRefPics );
  Void      setRefPicListExplicitlyDecoderSided( TComList<TComPic*>& rcListPic, std::vector<TComPic*>& rapcSpatRefPics );

  Bool      isIntra         ()                          { return  m_eSliceType == I_SLICE;  }
  Bool      isInterB        ()                          { return  m_eSliceType == B_SLICE;  }
  Bool      isInterP        ()                          { return  m_eSliceType == P_SLICE;  }
  
  Void      setLambda( Double d ) { m_dLambda = d; }
  Double    getLambda() { return m_dLambda;        }
  
  //SB
  Void      setViewIdx(Int i)                           { m_iViewIdx = i; }
  Int       getViewIdx()                                { return m_iViewIdx; }

  Void      setRefViewIdx       ( Int i, RefPicList e, Int iRefIdx ) { m_aiRefViewList[e][iRefIdx] = i; }
  Int       getRefViewIdx       ( RefPicList e, Int iRefIdx)    { return  m_aiRefViewList[e][iRefIdx]; }

  Void      initEqualRef();
  Bool      isEqualRef  ( RefPicList e, Int iRefIdx1, Int iRefIdx2 )
  {
    if (iRefIdx1 < 0 || iRefIdx2 < 0) return false;
    return m_abEqualRef[e][iRefIdx1][iRefIdx2];
  }
  
  Void setEqualRef( RefPicList e, Int iRefIdx1, Int iRefIdx2, Bool b)
  {
    m_abEqualRef[e][iRefIdx1][iRefIdx2] = m_abEqualRef[e][iRefIdx2][iRefIdx1] = b;
  }
  
  static Void      sortPicList         ( TComList<TComPic*>& rcListPic );
  
  Bool getNoBackPredFlag() { return m_bNoBackPredFlag; }
  Void setNoBackPredFlag( Bool b ) { m_bNoBackPredFlag = b; }
#if MS_LCEC_LOOKUP_TABLE_EXCEPTION
  Bool getRefIdxCombineCoding() { return m_bRefIdxCombineCoding; }
  Void setRefIdxCombineCoding( Bool b ) { m_bRefIdxCombineCoding = b; }
#endif
#if DCM_COMB_LIST
  Void      generateCombinedList       ();
#endif
  Void setSliceMode                     ( UInt uiMode )     { m_uiSliceMode = uiMode;                     }
  UInt getSliceMode                     ()                  { return m_uiSliceMode;                       }
  Void setSliceArgument                 ( UInt uiArgument ) { m_uiSliceArgument = uiArgument;             }
  UInt getSliceArgument                 ()                  { return m_uiSliceArgument;                   }
  Void setSliceCurStartCUAddr           ( UInt uiAddr )     { m_uiSliceCurStartCUAddr = uiAddr;           }
  UInt getSliceCurStartCUAddr           ()                  { return m_uiSliceCurStartCUAddr;             }
  Void setSliceCurEndCUAddr             ( UInt uiAddr )     { m_uiSliceCurEndCUAddr = uiAddr;             }
  UInt getSliceCurEndCUAddr             ()                  { return m_uiSliceCurEndCUAddr;               }
  Void setSliceIdx                      ( UInt i)           { m_uiSliceIdx = i;                           }
  UInt getSliceIdx                      ()                  { return  m_uiSliceIdx;                       }
  Void copySliceInfo                    (TComSlice *pcSliceSrc);
  Void setEntropySliceMode              ( UInt uiMode )     { m_uiEntropySliceMode = uiMode;              }
  UInt getEntropySliceMode              ()                  { return m_uiEntropySliceMode;                }
  Void setEntropySliceArgument          ( UInt uiArgument ) { m_uiEntropySliceArgument = uiArgument;      }
  UInt getEntropySliceArgument          ()                  { return m_uiEntropySliceArgument;            }
  Void setEntropySliceCurStartCUAddr    ( UInt uiAddr )     { m_uiEntropySliceCurStartCUAddr = uiAddr;    }
  UInt getEntropySliceCurStartCUAddr    ()                  { return m_uiEntropySliceCurStartCUAddr;      }
  Void setEntropySliceCurEndCUAddr      ( UInt uiAddr )     { m_uiEntropySliceCurEndCUAddr = uiAddr;      }
  UInt getEntropySliceCurEndCUAddr      ()                  { return m_uiEntropySliceCurEndCUAddr;        }
  Void setNextSlice                     ( Bool b )          { m_bNextSlice = b;                           }
  Bool isNextSlice                      ()                  { return m_bNextSlice;                        }
  Void setNextEntropySlice              ( Bool b )          { m_bNextEntropySlice = b;                    }
  Bool isNextEntropySlice               ()                  { return m_bNextEntropySlice;                 }
  Void setSliceBits                     ( UInt uiVal )      { m_uiSliceBits = uiVal;                      }
  UInt getSliceBits                     ()                  { return m_uiSliceBits;                       }  
  
  Void      initMultiviewSlice    ( Int** aaiScale = 0, Int** aaiOffset = 0 );

  Int*      getCodedScale         ()  { return m_aaiCodedScale [0]; }
  Int*      getCodedOffset        ()  { return m_aaiCodedOffset[0]; }
  Int*      getInvCodedScale      ()  { return m_aaiCodedScale [1]; }
  Int*      getInvCodedOffset     ()  { return m_aaiCodedOffset[1]; }

  Void setTexturePic( TComPic *pcTexturePic ) { m_pcTexturePic = pcTexturePic; }
  TComPic *getTexturePic() const { return m_pcTexturePic; }

#ifdef WEIGHT_PRED
  Void  setWpScaling( wpScalingParam  wp[2][MAX_NUM_REF][3] ) { memcpy(m_weightPredTable, wp, sizeof(wpScalingParam)*2*MAX_NUM_REF*3); }
  Void  getWpScaling( RefPicList e, Int iRefIdx, wpScalingParam *&wp);
  Void  displayWpScaling();
  Void  resetWpScaling(wpScalingParam  wp[2][MAX_NUM_REF][3]);
  Void  initWpScaling(wpScalingParam  wp[2][MAX_NUM_REF][3]);
  Void  initWpScaling();
  inline Bool applyWP() { return( (m_eSliceType==P_SLICE && m_pcPPS->getUseWP()) || (m_eSliceType==B_SLICE && m_pcPPS->getWPBiPredIdc()) ); }
  
  Void  setWpAcDcParam ( wpACDCParam wp[3] ) { memcpy(m_weightACDCParam, wp, sizeof(wpACDCParam)*3); }
  Void  getWpAcDcParam ( wpACDCParam *&wp );
  Void  initWpAcDcParam();
#endif

protected:
#if 0
  TComPic*  xGetRefPic  (TComList<TComPic*>& rcListPic,
                         Bool                bDRBFlag,
                         ERBIndex            eERBIndex,
                         UInt                uiPOCCurr,
                         RefPicList          eRefPicList,
                         UInt                uiNthRefPic );
#endif
  
};// END CLASS DEFINITION TComSlice


#endif // __TCOMSLICE__

