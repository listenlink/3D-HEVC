

/** \file     TComPattern.h
    \brief    neighbouring pixel access classes (header)
*/

#ifndef __TCOMPATTERN__
#define __TCOMPATTERN__

// Include files
#include <stdio.h>
#include "CommonDef.h"

// ====================================================================================================================
// Class definition
// ====================================================================================================================

class TComDataCU;

/// neighbouring pixel access class for one component
class TComPatternParam
{
private:
  Int   m_iOffsetLeft;
  Int   m_iOffsetRight;
  Int   m_iOffsetAbove;
  Int   m_iOffsetBottom;
  Pel*  m_piPatternOrigin;
  
public:
  Int   m_iROIWidth;
  Int   m_iROIHeight;
  Int   m_iPatternStride;
  
  /// return starting position of buffer
  Pel*  getPatternOrigin()        { return  m_piPatternOrigin; }
  
  /// return starting position of ROI (ROI = &pattern[AboveOffset][LeftOffset])
  __inline Pel*  getROIOrigin()
  {
    return  m_piPatternOrigin + m_iPatternStride * m_iOffsetAbove + m_iOffsetLeft;
  }
  
  /// set parameters from Pel buffer for accessing neighbouring pixels
  Void setPatternParamPel ( Pel*        piTexture,
                           Int         iRoiWidth,
                           Int         iRoiHeight,
                           Int         iStride,
                           Int         iOffsetLeft,
                           Int         iOffsetRight,
                           Int         iOffsetAbove,
                           Int         iOffsetBottom );
  
  /// set parameters of one color component from CU data for accessing neighbouring pixels
  Void setPatternParamCU  ( TComDataCU* pcCU,
                           UChar       iComp,
                           UChar       iRoiWidth,
                           UChar       iRoiHeight,
                           Int         iOffsetLeft,
                           Int         iOffsetRight,
                           Int         iOffsetAbove,
                           Int         iOffsetBottom,
                           UInt        uiPartDepth,
                           UInt        uiAbsZorderIdx,
                           Bool        bPrdDepthMap = false );
};

/// neighbouring pixel access class for all components
class TComPattern
{
private:
  TComPatternParam  m_cPatternY;
  TComPatternParam  m_cPatternCb;
  TComPatternParam  m_cPatternCr;
#if MN_DC_PRED_FILTER
  Bool m_bAboveFlagForDCFilt;
  Bool m_bLeftFlagForDCFilt;
  Bool m_bDCPredFilterFlag;
#endif

#if LM_CHROMA 
  Bool m_bLeftAvailable;
  Bool m_bAboveAvailable;
#endif

public:
  
  // ROI & pattern information, (ROI = &pattern[AboveOffset][LeftOffset])
  Pel*  getROIY()                 { return m_cPatternY.getROIOrigin();    }
  Int   getROIYWidth()            { return m_cPatternY.m_iROIWidth;       }
  Int   getROIYHeight()           { return m_cPatternY.m_iROIHeight;      }
  Int   getPatternLStride()       { return m_cPatternY.m_iPatternStride;  }
#if MN_DC_PRED_FILTER
  Bool  getDCPredFilterFlag()     { return m_bDCPredFilterFlag; }
#endif

  // access functions of ADI buffers
  Int*  getAdiOrgBuf              ( Int iCuWidth, Int iCuHeight, Int* piAdiBuf );
  Int*  getAdiCbBuf               ( Int iCuWidth, Int iCuHeight, Int* piAdiBuf );
  Int*  getAdiCrBuf               ( Int iCuWidth, Int iCuHeight, Int* piAdiBuf );
  
#if QC_MDIS
#if HHI_DISABLE_INTRA_SMOOTHING_DEPTH
  Int*  getPredictorPtr           ( UInt uiDirMode, UInt uiWidthBits, Int iCuWidth, Int iCuHeight, Int* piAdiBuf, Bool bDepth = false );
#else
  Int*  getPredictorPtr           ( UInt uiDirMode, UInt uiWidthBits, Int iCuWidth, Int iCuHeight, Int* piAdiBuf );
#endif
#endif //QC_MDIS
  // -------------------------------------------------------------------------------------------------------------------
  // initialization functions
  // -------------------------------------------------------------------------------------------------------------------
  
  /// set parameters from Pel buffers for accessing neighbouring pixels
  Void initPattern            ( Pel*        piY,
                                Pel*        piCb,
                                Pel*        piCr,
                                Int         iRoiWidth,
                                Int         iRoiHeight,
                                Int         iStride,
                                Int         iOffsetLeft,
                                Int         iOffsetRight,
                                Int         iOffsetAbove,
                                Int         iOffsetBottom );
  
  /// set parameters from CU data for accessing neighbouring pixels
  Void  initPattern           ( TComDataCU* pcCU,
                                UInt        uiPartDepth,
                                UInt        uiAbsPartIdx,
                                Bool        bPrdDepthMap = false );
  
  /// set luma parameters from CU data for accessing ADI data
  Void  initAdiPattern        ( TComDataCU* pcCU,
                                UInt        uiZorderIdxInPart,
                                UInt        uiPartDepth,
                                Int*        piAdiBuf,
                                Int         iOrgBufStride,
                                Int         iOrgBufHeight,
                                Bool&       bAbove,
                                Bool&       bLeft,
                                Bool        bPrdDepthMap = false );
  
  /// set chroma parameters from CU data for accessing ADI data
  Void  initAdiPatternChroma  ( TComDataCU* pcCU,
                               UInt        uiZorderIdxInPart,
                               UInt        uiPartDepth,
                               Int*        piAdiBuf,
                               Int         iOrgBufStride,
                               Int         iOrgBufHeight,
                               Bool&       bAbove,
                               Bool&       bLeft );

#if LM_CHROMA 
  Bool  isLeftAvailable()         { return m_bLeftAvailable; }
  Bool  isAboveAvailable()        { return m_bAboveAvailable; }
#endif

#if (CONSTRAINED_INTRA_PRED || REFERENCE_SAMPLE_PADDING)
private:

#if REFERENCE_SAMPLE_PADDING
  /// padding of unavailable reference samples for intra prediction
  Void  fillReferenceSamples        ( TComDataCU* pcCU, Pel* piRoiOrigin, Int* piAdiTemp, Bool* bNeighborFlags, Int iNumIntraNeighbor, Int iUnitSize, Int iNumUnitsInCu, Int iTotalUnits, UInt uiCuWidth, UInt uiCuHeight, UInt uiWidth, UInt uiHeight, Int iPicStride, Bool bPrdDepthMap );
#endif
#if CONSTRAINED_INTRA_PRED
  /// constrained intra prediction
  Bool  isAboveLeftAvailableForCIP  ( TComDataCU* pcCU, UInt uiPartIdxLT );
#if REFERENCE_SAMPLE_PADDING
  Int   isAboveAvailableForCIP      ( TComDataCU* pcCU, UInt uiPartIdxLT, UInt uiPartIdxRT, Bool* bValidFlags );
  Int   isLeftAvailableForCIP       ( TComDataCU* pcCU, UInt uiPartIdxLT, UInt uiPartIdxLB, Bool* bValidFlags );
  Int   isAboveRightAvailableForCIP ( TComDataCU* pcCU, UInt uiPartIdxLT, UInt uiPartIdxRT, Bool* bValidFlags );
  Int   isBelowLeftAvailableForCIP  ( TComDataCU* pcCU, UInt uiPartIdxLT, UInt uiPartIdxLB, Bool* bValidFlags );
#else
  Bool  isAboveAvailableForCIP      ( TComDataCU* pcCU, UInt uiPartIdxLT, UInt uiPartIdxRT );
  Bool  isLeftAvailableForCIP       ( TComDataCU* pcCU, UInt uiPartIdxLT, UInt uiPartIdxLB );
  Bool  isAboveRightAvailableForCIP ( TComDataCU* pcCU, UInt uiPartIdxLT, UInt uiPartIdxRT );
  Bool  isBelowLeftAvailableForCIP  ( TComDataCU* pcCU, UInt uiPartIdxLT, UInt uiPartIdxLB );
#endif
#endif // CONSTRAINED_INTRA_PRED
#endif
};

#endif // __TCOMPATTERN__

