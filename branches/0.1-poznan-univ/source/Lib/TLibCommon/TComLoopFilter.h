

/** \file     TComLoopFilter.h
    \brief    deblocking filter (header)
*/

#ifndef __TCOMLOOPFILTER__
#define __TCOMLOOPFILTER__

#include "CommonDef.h"
#include "TComPic.h"

#define DEBLOCK_SMALLEST_BLOCK  8

// ====================================================================================================================
// Class definition
// ====================================================================================================================

/// deblocking filter class
class TComLoopFilter
{
private:
  UInt      m_uiDisableDeblockingFilterIdc; ///< deblocking filter idc
  UInt      m_uiNumPartitions;
  UChar*    m_aapucBS[2][3];              ///< Bs for [Ver/Hor][Y/U/V][Blk_Idx]
  Bool*     m_aapbEdgeFilter[2][3];
  LFCUParam m_stLFCUParam;                  ///< status structure
  
#if (PARALLEL_DEBLK_DECISION && !PARALLEL_MERGED_DEBLK)
  UInt m_decisions_D     [MAX_CU_SIZE/DEBLOCK_SMALLEST_BLOCK][MAX_CU_SIZE/DEBLOCK_SMALLEST_BLOCK];
  UInt m_decisions_Sample[MAX_CU_SIZE/DEBLOCK_SMALLEST_BLOCK][MAX_CU_SIZE];
#endif
  
protected:
  /// CU-level deblocking function
#if PARALLEL_MERGED_DEBLK
  Void xDeblockCU                 ( TComDataCU* pcCU, UInt uiAbsZorderIdx, UInt uiDepth, Int Edge );
#else
  Void xDeblockCU                 ( TComDataCU* pcCU, UInt uiAbsZorderIdx, UInt uiDepth );
#endif

  // set / get functions
  Void xSetLoopfilterParam        ( TComDataCU* pcCU, UInt uiAbsZorderIdx );
  // filtering functions
  Void xSetEdgefilterTU           ( TComDataCU* pcCU, UInt uiAbsZorderIdx, UInt uiDepth );
  Void xSetEdgefilterPU           ( TComDataCU* pcCU, UInt uiAbsZorderIdx );
  Void xGetBoundaryStrengthSingle ( TComDataCU* pcCU, UInt uiAbsZorderIdx, Int iDir, UInt uiPartIdx );
  UInt xCalcBsIdx                 ( TComDataCU* pcCU, UInt uiAbsZorderIdx, Int iDir, Int iEdgeIdx, Int iBaseUnitIdx )
  {
    TComPic* const pcPic = pcCU->getPic();
    const UInt uiLCUWidthInBaseUnits = pcPic->getNumPartInWidth();
    if( iDir == 0 )
      return g_auiRasterToZscan[g_auiZscanToRaster[uiAbsZorderIdx] + iBaseUnitIdx * uiLCUWidthInBaseUnits + iEdgeIdx ];
    else
      return g_auiRasterToZscan[g_auiZscanToRaster[uiAbsZorderIdx] + iEdgeIdx * uiLCUWidthInBaseUnits + iBaseUnitIdx ];
  }
  Void xSetEdgefilterMultiple( TComDataCU* pcCU, UInt uiAbsZorderIdx, UInt uiDepth, Int iDir, Int iEdgeIdx, Bool bValue );
  
#if (PARALLEL_DEBLK_DECISION && !PARALLEL_MERGED_DEBLK)
  Void xEdgeFilterLuma            ( TComDataCU* pcCU, UInt uiAbsZorderIdx, UInt uiDepth, Int iDir, Int iEdge, Int iDecideExecute);
#else
  Void xEdgeFilterLuma            ( TComDataCU* pcCU, UInt uiAbsZorderIdx, UInt uiDepth, Int iDir, Int iEdge );
#endif
  Void xEdgeFilterChroma          ( TComDataCU* pcCU, UInt uiAbsZorderIdx, UInt uiDepth, Int iDir, Int iEdge );
  
#if (PARALLEL_DEBLK_DECISION && !PARALLEL_MERGED_DEBLK)
  __inline Void xPelFilterLumaStrong    ( Pel* piSrc, Int iOffset, Pel m0, Pel m1, Pel m2, Pel m3, Pel m4, Pel m5, Pel m6, Pel m7);
  __inline Void xPelFilterLumaWeak      ( Pel* piSrc, Int iOffset, Int tc, Pel m1, Pel m2, Pel m3, Pel m4, Pel m5, Pel m6);
  __inline Void xPelFilterLumaExecution ( Pel* piSrc, Int iOffset, Int tc, Int strongFilter);
  __inline Int  xPelFilterLumaDecision  ( Pel* piSrc, Int iOffset, Int d, Int beta, Int tc);
#endif

#if PARALLEL_MERGED_DEBLK
  __inline Void xPelFilterLuma( Pel* piSrc, Int iOffset, Int d, Int beta, Int tc , Pel* piSrcJudge);
#else
  __inline Void xPelFilterLuma( Pel* piSrc, Int iOffset, Int d, Int beta, Int tc );
#endif
  __inline Void xPelFilterChroma( Pel* piSrc, Int iOffset, Int tc );
  __inline Int xCalcD( Pel* piSrc, Int iOffset);
  
public:
  TComLoopFilter();
  virtual ~TComLoopFilter();
  
  Void  create                    ( UInt uiMaxCUDepth );
  Void  destroy                   ();
  
  /// set configuration
  Void setCfg( UInt uiDisableDblkIdc, Int iAlphaOffset, Int iBetaOffset );
  
  /// picture-level deblocking filter
  Void loopFilterPic( TComPic* pcPic );
};

#endif

