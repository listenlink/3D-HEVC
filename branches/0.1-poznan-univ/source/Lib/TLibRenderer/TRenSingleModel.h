


#ifndef __TRENSINGLEMODEL__
#define __TRENSINGLEMODEL__

#include "TRenImage.h"
#include "../TLibCommon/CommonDef.h"
#include "../TLibCommon/TComPicYuv.h"
#include "../TLibCommon/TypeDef.h"
#include "../../App/TAppCommon/TAppComCamPara.h"


#include <math.h>
#include <errno.h>
#include <iostream>

#include <string>
#include <cstdio>
#include <cstring>


using namespace std;


class TRenSingleModel
{

public:
  TRenSingleModel();
  ~TRenSingleModel();

  // Create Model
  Void   create    ( Int iMode, Int iWidth, Int iHeight, Int iShiftPrec, Int*** aaaiSubPelShiftTable, Int iHoleMargin, Bool bUseOrgRef, Int iBlendMode );

  // Set Frame dependent datas
  Void   setLRView ( Int iViewPos, Pel** apiCurVideoPel, Int* aiCurVideoStride, Pel* piCurDepthPel, Int iCurDepthStride );
  Void   setup     ( TComPicYuv* pcOrgVideo, Int** ppiShiftLutLeft, Int** ppiBaseShiftLutLeft, Int** ppiShiftLutRight,  Int** ppiBaseShiftLutRight,  Int iDistToLeft, Bool bKeepReference );

  // Set Data
  Void   setDepth  ( Int iViewPos,                 Int iStartPosX, Int iStartPosY, Int iWidth, Int iHeight, Int iStride, Pel* piNewData );
  Void   setVideo  ( Int iViewPos,     Int iPlane, Int iStartPosX, Int iStartPosY, Int iWidth, Int iHeight, Int iStride, Pel* piNewData );

  // Get Distortion
  RMDist getDistDepth  ( Int iViewPos,             Int iStartPosX, Int iStartPosY, Int iWidth, Int iHeight, Int iStride, Pel* piNewData );
  RMDist getDistVideo  ( Int iViewPos, Int iPlane, Int iStartPosX, Int iStartPosY, Int iWidth, Int iHeight, Int iStride, Pel* piNewData );

  // Get Rendered Data
  Void   getSynthView( Int iViewPos, Pel**& rppiRenVideoPel, Pel*& rpiRenDepthPel, Int& riStride );

  // Get Reference Data
  Void   getRefView  ( TComPicYuv*& rpcPicYuvRefView, Pel**& rppiRefVideoPel, Int*& raiStrides );

#if GERHARD_RM_DEBUG_MM
  Bool   compare( TRenSingleModel* pcRefModel );
#endif

private:
  // Set and inc Current Row
  __inline Void   xSetViewRow(  Int iPosY );
  __inline Void   xIncViewRow();

  /////  Rendering /////
  // Left to Right
  __inline RMDist xRenderL            ( Int iStartPosX, Int iStartPosY, Int iWidth, Int iHeight, Int iStride, Pel* piNewData, Bool bSet);
  __inline Void   xInitRenderPartL    ( Int iEndChangePos, Int iLastSPos  );
  __inline Void   xRenderRangeL       ( Int iCurSPos, Int iLastSPos, Int iCurPos, RMDist& riError );
  __inline Void   xRenderShiftedRangeL( Int iCurSPos, Int iLastSPos, Int iCurPos, RMDist& riError );
  __inline Void   xFillHoleL          ( Int iCurSPos, Int iLastSPos, Int iCurPos, RMDist& riError );
  __inline Void   xExtrapolateMarginL ( Int iCurSPos,                Int iCurPos, RMDist& riError );
  __inline Int    xRangeLeftL         ( Int iPos );
  __inline Int    xRangeRightL        ( Int iPos );
  __inline Int    xRoundL             ( Int iPos );

  // Right to Left
  __inline RMDist xRenderR            ( Int iStartPosX, Int iStartPosY, Int iWidth, Int iHeight, Int iStride, Pel* piNewData, Bool bSet );
  __inline Void   xInitRenderPartR    ( Int iStartChangePos, Int iLastSPos );
  __inline Void   xRenderShiftedRangeR( Int iCurSPos, Int iLastSPos, Int iCurPos, RMDist& riError );
  __inline Void   xRenderRangeR       ( Int iCurSPos, Int iLastSPos, Int iCurPos, RMDist& riError );
  __inline Void   xFillHoleR          ( Int iCurSPos, Int iLastSPos, Int iCurPos, RMDist& riError );
  __inline Void   xExtrapolateMarginR ( Int iCurSPos,                Int iCurPos, RMDist& riError );
  __inline Int    xRangeLeftR         ( Int iPos );
  __inline Int    xRangeRightR        ( Int iPos );
  __inline Int    xRoundR             ( Int iPos );

  // Blending
  __inline Void   xSetShiftedPelBlend ( Int iSourcePos, Int iTargetSPos, Pel iFilled, RMDist& riError );
  __inline Void   xGetBlendedValueBM1 ( Pel iYL, Pel iYR, Pel iUL, Pel iUR, Pel iVL, Pel iVR, Pel iDepthL, Pel iDepthR, Int iFilledL, Int iFilledR, Pel& riY, Pel& riU, Pel&riV );
  __inline Void   xGetBlendedValueBM2 ( Pel iYL, Pel iYR, Pel iUL, Pel iUR, Pel iVL, Pel iVR, Pel iDepthL, Pel iDepthR, Int iFilledL, Int iFilledR, Pel& riY, Pel& riU, Pel&riV );

#if GERHARD_RM_COLOR_PLANES
  __inline Void   xGetBlendedValue    ( Pel iYL, Pel iYR, Pel iUL, Pel iUR, Pel iVL, Pel iVR, Pel iDepthL, Pel iDepthR, Int iFilledL, Int iFilledR, Pel& riY, Pel& riU, Pel&riV );
#else
  __inline Void   xGetBlendedValue    ( Pel iYL,   Pel iYR, Pel iDepthL, Pel iDepthR, Int iFilledL, Int iFilledR, Pel& riY );
#endif
  __inline Pel    xBlend              ( Pel pVal1, Pel pVal2, Int iWeightVal2 );

// General
  __inline Void   xSetShiftedPel     ( Int iSourcePos, Int iTargetSPos, Pel iFilled, RMDist& riError );
  __inline Int    xShiftNewData      ( Int iPos, Int iPosInNewData );
  __inline Int    xShift             ( Int iPos );
  __inline Int    xShift             ( Int iPos, Int iPosInNewData );
  __inline Int    xGetDist           ( Int iDiffY, Int iDiffU, Int iDiffV );
  __inline Int    xGetDist           ( Int iDiffY );

  // Utilities
  __inline Void   xInitView  ( Int iViewPos );
  __inline Void   xSetPels   ( Pel*  piPelSource , Int iSourceStride, Int iWidth, Int iHeight, Pel iVal );
  __inline Void   xSetBools  ( Bool* pbSource    , Int iSourceStride, Int iWidth, Int iHeight, Bool bVal );
  __inline Void   xSetInts   ( Int*  piPelSource , Int iSourceStride, Int iWidth, Int iHeight, Int iVal );

#if GERHARD_RM_DEBUG_MM
public:
#else
private:
#endif

  // Image sizes
  Int   m_iWidth;
  Int   m_iHeight;
  Int   m_iStride;
  Int   m_iPad;

  Int   m_iSampledWidth;
  Int   m_iSampledHeight;
  Int   m_iSampledStride;

  // Base
  Pel** m_aapiBaseVideoPel     [2]; // Dim1: ViewPosition 0->Left, 1->Right; Dim2: Plane  0-> Y, 1->U, 2->V
  Int*  m_aaiBaseVideoStrides  [2]; // Dim1: ViewPosition 0->Left, 1->Right; Dim2: Plane  0-> Y, 1->U, 2->V

  Pel*  m_apiBaseDepthPel      [2]; // Dim1: ViewPosition
  Int   m_aiBaseDepthStrides   [2]; // Dim1: ViewPosition

  // LUT
  Int** m_appiShiftLut         [2];
  Int** m_ppiCurLUT;
  Int** m_aaiSubPelShiftL;
  Int** m_aaiSubPelShiftR;

  Int*  m_piInvZLUTLeft;
  Int*  m_piInvZLUTRight;


  //// Reference Data  ////
  TComPicYuv* m_pcPicYuvRef       ;    // Reference PIcYuv
  Pel*  m_aapiRefVideoPel      [3];    // Dim1: Plane  0-> Y, 1->U, 2->V
  Int   m_aiRefVideoStrides    [3];    // Dim1: Plane  0-> Y, 1->U, 2->V

  // Renderer State
  Int*  m_piError                 ;
  Pel*  m_apiFilled            [2];    // Dim1: ViewPosition
  Bool* m_apbOccluded          [2];    // Dim1: ViewPosition
  Pel*  m_aapiSynthVideoPel    [3][3]; // Dim1: ViewPosition 0: Left, 1:Right, 2: Merged, Dim2: Plane  0-> Y, 1->U, 2->V
  Pel*  m_apiSynthDepthPel     [3];    // Dim1: ViewPosition 0: Left, 1:Right, 2: Merged, Dim2: Plane  0-> Y, 1->U, 2->V

  // Rendering State
  Bool  m_bInOcclusion;                // Currently rendering in occluded area
  Int   m_iLastOccludedSPos;           // Position of last topmost shifted position
  Int   m_iLastOccludedSPosFP;         // Position of last topmost shifted position in FullPels

  Bool  m_bSet;                        // Set Data, or get Error
  Int   m_iCurViewPos;                 // Current View Position 0: Left, 1: Right
  Int   m_iOtherViewPos;               // Other View Position 0: Left, 1: Right
  Pel*  m_piNewDepthData;              // Pointer to new depth data
  Int   m_iStartChangePosX;            // Start Position of new data
  Int   m_iNewDataWidth;               // Width of new data
  Pel   m_iCurDepth;                   // Current Depth Value
  Pel   m_iLastDepth;                  // Last Depth Value
  Pel   m_iThisDepth;                  // Depth value to use for setting

  //// Settings ////
  // Input
  Int   m_iMode;                       // 0: Left to Right, 1: Right to Left, 2: Merge
  Bool  m_bUseOrgRef;
  Int   m_iShiftPrec;
  Int   m_iHoleMargin;
  Int   m_iBlendMode;

  // Derived settings
  Int   m_iGapTolerance;
  Int   m_iBlendZThres;
  Int   m_iBlendDistWeight;

  //// Current Pointers ////
  Pel*  m_aapiBaseVideoPelRow  [2][3]; // Dim1: ViewPosition 0->Left, 1->Right; Dim2: Plane  0-> Y, 1->U, 2->V
  Pel*  m_apiBaseDepthPelRow   [2];    // Dim1: ViewPosition
  Bool* m_apbOccludedRow       [2];    // Dim1: ViewPosition
  Pel*  m_apiFilledRow         [2];    // Dim1: ViewPosition
  Int*  m_apiErrorRow             ;

  Pel*  m_aapiRefVideoPelRow   [3];    // Dim1: Plane  0-> Y, 1->U, 2->V
  Pel*  m_aapiSynthVideoPelRow [3][3]; // Dim1: ViewPosition 0: Left, 1:Right, 2: Merged, Dim2: Plane  0-> Y, 1->U, 2->V
  Pel*  m_apiSynthDepthPelRow  [3];    // Dim1: ViewPosition 0: Left, 1:Right, 2: Merged


  //// MISC ////
  const Int m_iDistShift;                  // Shift in Distortion computation
};

#endif //__TRENSINGLEMODEL__
