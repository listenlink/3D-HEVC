/* The copyright in this software is being made available under the BSD
 * License, included below. This software may be subject to other third party
 * and contributor rights, including patent rights, and no such rights are
 * granted under this license.
 *
 * Copyright (c) 2010-2011, ISO/IEC
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
 *  * Neither the name of the ISO/IEC nor the names of its contributors may
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


#ifndef __TRENSINGLEMODEL__
#define __TRENSINGLEMODEL__

#include "TRenImage.h"
#include "../TLibCommon/CommonDef.h"
#include "../TLibCommon/TComPicYuv.h"
#include "../TLibCommon/TypeDef.h"
#include "../TAppCommon/TAppComCamPara.h"


#include <math.h>
#include <errno.h>
#include <iostream>

#include <string>
#include <cstdio>
#include <cstring>


using namespace std;


#if HHI_VSO_RM_ASSERTIONS
#define RM_AOT( exp ) AOT ( exp )
#define RM_AOF( exp ) AOF ( exp )
#else
#define RM_AOT( exp ) ((void)0)
#define RM_AOF( exp ) ((void)0)
#endif

#define RenModRemoveBitInc( exp ) bBitInc ? ( RemoveBitIncrement( exp ) ) : ( exp ) 

class TRenSingleModel
{
public: 

#if FIX_VIRT_DESTRUCTOR
  virtual ~TRenSingleModel() { }  
#endif
#if LGE_VSO_EARLY_SKIP_A0093
  virtual Void   create    ( Int iMode, Int iWidth, Int iHeight, Int iShiftPrec, Int*** aaaiSubPelShiftTable, Int iHoleMargin, Bool bUseOrgRef, Int iBlendMode, Bool bEarlySkip ) = 0;
#else
  virtual Void   create    ( Int iMode, Int iWidth, Int iHeight, Int iShiftPrec, Int*** aaaiSubPelShiftTable, Int iHoleMargin, Bool bUseOrgRef, Int iBlendMode ) = 0;
#endif

  // Set Frame dependent data
  virtual Void   setLRView ( Int iViewPos, Pel** apiCurVideoPel, Int* aiCurVideoStride, Pel* piCurDepthPel, Int iCurDepthStride ) = 0;
#if FIX_VSO_SETUP
  virtual Void   setupPart ( UInt uiHorOffset,       Int iUsedHeight ) = 0;
  virtual Void   setup     ( TComPicYuv* pcOrgVideo, Int** ppiShiftLutLeft, Int** ppiBaseShiftLutLeft, Int** ppiShiftLutRight,  Int** ppiBaseShiftLutRight,  Int iDistToLeft, Bool bKeepReference ) = 0;
#else
  virtual Void   setup     ( TComPicYuv* pcOrgVideo, Int** ppiShiftLutLeft, Int** ppiBaseShiftLutLeft, Int** ppiShiftLutRight,  Int** ppiBaseShiftLutRight,  Int iDistToLeft, Bool bKeepReference, UInt uiHorOffset ) = 0;
#endif

  // Set Data
#ifdef LGE_VSO_EARLY_SKIP_A0093
  virtual Void   setDepth  ( Int iViewPos,                 Int iStartPosX, Int iStartPosY, Int iWidth, Int iHeight, Int iStride, Pel* piNewData, Pel* piOrgData, Int iOrgStride )  = 0;
#else
  virtual Void   setDepth  ( Int iViewPos,                 Int iStartPosX, Int iStartPosY, Int iWidth, Int iHeight, Int iStride, Pel* piNewData )  = 0;
#endif
  virtual Void   setVideo  ( Int iViewPos,     Int iPlane, Int iStartPosX, Int iStartPosY, Int iWidth, Int iHeight, Int iStride, Pel* piNewData ) = 0;

  // Get Distortion
#ifdef LGE_VSO_EARLY_SKIP_A0093
  virtual RMDist getDistDepth  ( Int iViewPos,             Int iStartPosX, Int iStartPosY, Int iWidth, Int iHeight, Int iStride, Pel* piNewData, Pel * piOrgData , Int iOrgStride)=0;
#else
  virtual RMDist getDistDepth  ( Int iViewPos,             Int iStartPosX, Int iStartPosY, Int iWidth, Int iHeight, Int iStride, Pel* piNewData ) = 0;
#endif
  virtual RMDist getDistVideo  ( Int iViewPos, Int iPlane, Int iStartPosX, Int iStartPosY, Int iWidth, Int iHeight, Int iStride, Pel* piNewData ) = 0;

#if FIX_VSO_SETUP
  virtual Void   getSynthVideo  ( Int iViewPos, TComPicYuv* pcPicYuv ) = 0;  
  virtual Void   getSynthDepth  ( Int iViewPos, TComPicYuv* pcPicYuv ) = 0;
  virtual Void   getRefVideo    ( Int iViewPos, TComPicYuv* pcPicYuv ) = 0;
#else
  virtual Void   getSynthVideo  ( Int iViewPos, TComPicYuv* pcPicYuv, UInt uiHorOffset  ) = 0;  
  virtual Void   getSynthDepth  ( Int iViewPos, TComPicYuv* pcPicYuv, UInt uiHorOffset  ) = 0;
  virtual Void   getRefVideo    ( Int iViewPos, TComPicYuv* pcPicYuv, UInt uiHorOffset  ) = 0;
#endif
};

template < BlenMod iBM, Bool bBitInc >
class TRenSingleModelC : public TRenSingleModel
{
  struct RenModelInPels
  {
    // video
    Pel aiY[5]    ; // y-value
#if HHI_VSO_COLOR_PLANES
    Pel aiU[5]    ; // u-value
    Pel aiV[5]    ; // v-value
#endif
    // depth
    Pel iD        ; // depth

    // state
    Bool bOccluded; // Occluded
  };

  struct RenModelOutPels
  {
    // video
    Pel iYLeft    ; 
    Pel iYRight   ; 
    Pel iYBlended ; 
#if HHI_VSO_COLOR_PLANES
    Pel iULeft    ; 
    Pel iURight   ; 
    Pel iUBlended ; 
    Pel iVLeft    ; 
    Pel iVRight   ; 
    Pel iVBlended ; 
#endif
    // depth
    Pel iDLeft    ;
    Pel iDRight   ; 
    Pel iDBlended ; 

    // state
    Int iFilledLeft ; 
    Int iFilledRight; 

    // error
    Int  iError   ;

    // reference
    Pel iYRef    ; 
#if HHI_VSO_COLOR_PLANES
    Pel iURef    ; 
    Pel iVRef    ; 
#endif        
  };



public:
  TRenSingleModelC();
  ~TRenSingleModelC();

  // Create Model
#if LGE_VSO_EARLY_SKIP_A0093
  Void   create    ( Int iMode, Int iWidth, Int iHeight, Int iShiftPrec, Int*** aaaiSubPelShiftTable, Int iHoleMargin, Bool bUseOrgRef, Int iBlendMode, Bool bEarlySkip  );
#else
  Void   create    ( Int iMode, Int iWidth, Int iHeight, Int iShiftPrec, Int*** aaaiSubPelShiftTable, Int iHoleMargin, Bool bUseOrgRef, Int iBlendMode );
#endif

  // Set Frame dependent data
  Void   setLRView ( Int iViewPos, Pel** apiCurVideoPel, Int* aiCurVideoStride, Pel* piCurDepthPel, Int iCurDepthStride );
#if FIX_VSO_SETUP
  Void   setupPart ( UInt uiHorOffset,       Int uiUsedHeight );
  Void   setup     ( TComPicYuv* pcOrgVideo, Int** ppiShiftLutLeft, Int** ppiBaseShiftLutLeft, Int** ppiShiftLutRight,  Int** ppiBaseShiftLutRight,  Int iDistToLeft, Bool bKeepReference );
#else
  Void   setup     ( TComPicYuv* pcOrgVideo, Int** ppiShiftLutLeft, Int** ppiBaseShiftLutLeft, Int** ppiShiftLutRight,  Int** ppiBaseShiftLutRight,  Int iDistToLeft, Bool bKeepReference, UInt uiHorOffset );
#endif

#if LGE_VSO_EARLY_SKIP_A0093
  Void   setDepth  ( Int iViewPos,                 Int iStartPosX, Int iStartPosY, Int iWidth, Int iHeight, Int iStride, Pel* piNewData, Pel* piOrgData, Int iOrgStride );
#else
  Void   setDepth  ( Int iViewPos,                 Int iStartPosX, Int iStartPosY, Int iWidth, Int iHeight, Int iStride, Pel* piNewData );
#endif
  Void   setVideo  ( Int iViewPos,     Int iPlane, Int iStartPosX, Int iStartPosY, Int iWidth, Int iHeight, Int iStride, Pel* piNewData );

  // Get Distortion
#ifdef LGE_VSO_EARLY_SKIP_A0093
  RMDist getDistDepth  ( Int iViewPos,             Int iStartPosX, Int iStartPosY, Int iWidth, Int iHeight, Int iStride, Pel* piNewData, Pel * piOrgData , Int iOrgStride);
#else
  RMDist getDistDepth  ( Int iViewPos,             Int iStartPosX, Int iStartPosY, Int iWidth, Int iHeight, Int iStride, Pel* piNewData );
#endif
  RMDist getDistVideo  ( Int iViewPos, Int iPlane, Int iStartPosX, Int iStartPosY, Int iWidth, Int iHeight, Int iStride, Pel* piNewData );

#if FIX_VSO_SETUP
  Void   getSynthVideo  ( Int iViewPos, TComPicYuv* pcPicYuv );  
  Void   getSynthDepth  ( Int iViewPos, TComPicYuv* pcPicYuv );
  Void   getRefVideo    ( Int iViewPos, TComPicYuv* pcPicYuv );
#else
  Void   getSynthVideo  ( Int iViewPos, TComPicYuv* pcPicYuv, UInt uiHorOffset );  
  Void   getSynthDepth  ( Int iViewPos, TComPicYuv* pcPicYuv, UInt uiHorOffset  );
  Void   getRefVideo    ( Int iViewPos, TComPicYuv* pcPicYuv, UInt uiHorOffset  );
#endif

private:
  // Set and inc Current Row
  __inline Void   xSetViewRow(  Int iPosY );
  __inline Void   xIncViewRow();

  /////  Rendering /////
  // Left to Right
#if LGE_VSO_EARLY_SKIP_A0093
                      __inline Bool   xDetectEarlySkipL   ( Int iStartPosX, Int iStartPosY, Int iWidth, Int iHeight, Int iStride, Pel* piNewData,Pel* piOrgData, Int iOrgStride );
                      __inline Bool   xDetectEarlySkipR   ( Int iStartPosX, Int iStartPosY, Int iWidth, Int iHeight, Int iStride, Pel* piNewData,Pel* piOrgData, Int iOrgStride );
  template<Bool bSet> __inline RMDist xRenderL            ( Int iStartPosX, Int iStartPosY, Int iWidth, Int iHeight, Int iStride, Pel* piNewData, Bool bFast );
  template<Bool bSet> __inline RMDist xRenderR            ( Int iStartPosX, Int iStartPosY, Int iWidth, Int iHeight, Int iStride, Pel* piNewData, Bool bFast );
#else
  template<Bool bSet> __inline RMDist xRenderR            ( Int iStartPosX, Int iStartPosY, Int iWidth, Int iHeight, Int iStride, Pel* piNewData );
  template<Bool bSet> __inline RMDist xRenderL            ( Int iStartPosX, Int iStartPosY, Int iWidth, Int iHeight, Int iStride, Pel* piNewData );
#endif
                      __inline Void   xInitRenderPartL    ( Int iEndChangePos, Int iLastSPos  );
  template<Bool bSet> __inline Void   xRenderRangeL       ( Int iCurSPos, Int iLastSPos, Int iCurPos, RMDist& riError );
  template<Bool bSet> __inline Void   xRenderShiftedRangeL( Int iCurSPos, Int iLastSPos, Int iCurPos, RMDist& riError );
  template<Bool bSet> __inline Void   xFillHoleL          ( Int iCurSPos, Int iLastSPos, Int iCurPos, RMDist& riError );
  template<Bool bSet> __inline Void   xExtrapolateMarginL ( Int iCurSPos,                Int iCurPos, RMDist& riError );
                      __inline Int    xRangeLeftL         ( Int iPos );
                      __inline Int    xRangeRightL        ( Int iPos );
                      __inline Int    xRoundL             ( Int iPos );

  // Right to Left
                      __inline Void   xInitRenderPartR    ( Int iStartChangePos, Int iLastSPos );
  template<Bool bSet> __inline Void   xRenderShiftedRangeR( Int iCurSPos, Int iLastSPos, Int iCurPos, RMDist& riError );
  template<Bool bSet> __inline Void   xRenderRangeR       ( Int iCurSPos, Int iLastSPos, Int iCurPos, RMDist& riError );
  template<Bool bSet> __inline Void   xFillHoleR          ( Int iCurSPos, Int iLastSPos, Int iCurPos, RMDist& riError );
  template<Bool bSet> __inline Void   xExtrapolateMarginR ( Int iCurSPos,                Int iCurPos, RMDist& riError );
                      __inline Int    xRangeLeftR         ( Int iPos );
                      __inline Int    xRangeRightR        ( Int iPos );
                      __inline Int    xRoundR             ( Int iPos );

  // Blending
  template<Bool bSet> __inline Void   xSetShiftedPelBlend ( Int iSourcePos, Int iTargetSPos, Pel iFilled, RMDist& riError );

#if HHI_VSO_COLOR_PLANES
  __inline Void   xGetBlendedValue    ( Pel iYL, Pel iYR, Pel iUL, Pel iUR, Pel iVL, Pel iVR, Pel iDepthL, Pel iDepthR, Int iFilledL, Int iFilledR, Pel& riY, Pel& riU, Pel&riV );
  __inline Void   xGetBlendedValueBM1 ( Pel iYL, Pel iYR, Pel iUL, Pel iUR, Pel iVL, Pel iVR, Pel iDepthL, Pel iDepthR, Int iFilledL, Int iFilledR, Pel& riY, Pel& riU, Pel&riV );
  __inline Void   xGetBlendedValueBM2 ( Pel iYL, Pel iYR, Pel iUL, Pel iUR, Pel iVL, Pel iVR, Pel iDepthL, Pel iDepthR, Int iFilledL, Int iFilledR, Pel& riY, Pel& riU, Pel&riV );
#else
  __inline Void   xGetBlendedValue    ( Pel iYL, Pel iYR, Pel iDepthL, Pel iDepthR, Int iFilledL, Int iFilledR, Pel& riY );
  __inline Void   xGetBlendedValueBM1 ( Pel iYL, Pel iYR, Pel iDepthL, Pel iDepthR, Int iFilledL, Int iFilledR, Pel& riY );
  __inline Void   xGetBlendedValueBM2 ( Pel iYL, Pel iYR, Pel iDepthL, Pel iDepthR, Int iFilledL, Int iFilledR, Pel& riY );
#endif
  __inline Pel    xBlend              ( Pel pVal1, Pel pVal2, Int iWeightVal2 );

  // General
  template<Bool bSet> __inline Void xSetShiftedPelL       (Int iSourcePos,             Int iSubSourcePos, Int iTargetSPos,              Pel iFilled, RMDist& riError );
  template<Bool bSet> __inline Void xSetShiftedPelBlendL  (RenModelInPels* pcInSample, Int iSubSourcePos, RenModelOutPels* pcOutSample, Pel iFilled, RMDist& riError );
  template<Bool bSet> __inline Void xSetShiftedPelNoBlendL(RenModelInPels* pcInSample, Int iSubSourcePos, RenModelOutPels* pcOutSample, Pel iFilled, RMDist& riError );

  template<Bool bSet> __inline Void xSetShiftedPelR       (Int iSourcePos,             Int iSubSourcePos, Int iTargetSPos,              Pel iFilled, RMDist& riError );
  template<Bool bSet> __inline Void xSetShiftedPelBlendR  (RenModelInPels* pcInSample, Int iSubSourcePos, RenModelOutPels* pcOutSample, Pel iFilled, RMDist& riError );
  template<Bool bSet> __inline Void xSetShiftedPelNoBlendR(RenModelInPels* pcInSample, Int iSubSourcePos, RenModelOutPels* pcOutSample, Pel iFilled, RMDist& riError );

  __inline Int    xShiftNewData      ( Int iPos, Int iPosInNewData );
  __inline Int    xShift             ( Int iPos );
  __inline Int    xShift             ( Int iPos, Int iPosInNewData );
  __inline Int    xGetDist           ( Int iDiffY, Int iDiffU, Int iDiffV );
  __inline Int    xGetDist           ( Int iDiffY );

  // Utilities
  __inline Void   xSetPels   ( Pel*  piPelSource , Int iSourceStride, Int iWidth, Int iHeight, Pel iVal );
  __inline Void   xSetBools  ( Bool* pbSource    , Int iSourceStride, Int iWidth, Int iHeight, Bool bVal );
  __inline Void   xSetInts   ( Int*  piPelSource , Int iSourceStride, Int iWidth, Int iHeight, Int iVal );

#if HHI_VSO_COLOR_PLANES
  Void            xGetSampleStrTextPtrs ( Int iViewNum, Pel RenModelOutPels::*& rpiSrcY, Pel RenModelOutPels::*& rpiSrcU, Pel RenModelOutPels::*& rpiSrcV );
#else  
  Void            xGetSampleStrTextPtrs ( Int iViewNum, Pel RenModelOutPels::*& rpiSrcY );
#endif 
  Void            xGetSampleStrDepthPtrs( Int iViewNum, Pel RenModelOutPels::*& rpiSrcD );
       
  Void            xSetStructRefView            ();
  Void            xResetStructError            ();
  Void            xInitSampleStructs           ();
  Void            xSetStructSynthViewAsRefView ();
#if FIX_VSO_SETUP
  Void            xCopy2PicYuv                ( Pel** ppiSrcVideoPel, Int* piStrides, TComPicYuv* rpcPicYuvTarget );
#else
  Void            xCopy2PicYuv                ( Pel** ppiSrcVideoPel, Int* piStrides, TComPicYuv* rpcPicYuvTarget, UInt uiHorOffset );
#endif

  template< typename S, typename T> 
  Void   xCopyFromSampleStruct ( S* ptSource , Int iSourceStride, T S::* ptSourceElement, T* ptTarget, Int iTargetStride, Int iWidth, Int iHeight )
  {
#if FIX_VSO_SETUP
    AOT( iWidth != m_iWidth ); 
    for (Int iPosY = 0; iPosY < iHeight; iPosY++)
#else
    for (Int iPosY = 0; iPosY < m_iHeight; iPosY++)
#endif
    {
      for (Int iPosX = 0; iPosX < m_iWidth; iPosX++)
      {
        ptTarget[iPosX] = ptSource[iPosX].*ptSourceElement;
      }
      ptSource += iSourceStride;
      ptTarget += iTargetStride;
    }    
  }  

  template< typename S, typename T> 
  Void   xCopyToSampleStruct ( T* ptSource , Int iSourceStride, S* ptTarget, Int iTargetStride, T S::* ptSourceElement, Int iWidth, Int iHeight )
  {
#if FIX_VSO_SETUP
    AOT( iWidth != m_iWidth ); 
    for (Int iPosY = 0; iPosY < iHeight; iPosY++)
#else
    for (Int iPosY = 0; iPosY < m_iHeight; iPosY++)
#endif
    {
      for (Int iPosX = 0; iPosX < m_iWidth; iPosX++)
      {
        ptTarget[iPosX] = ptSource[iPosX].*ptSourceElement;
      }
      ptSource += iSourceStride;
      ptTarget += iTargetStride;
    }    
  }   

private:

  // Image sizes
  Int   m_iWidth;
  Int   m_iHeight;
  Int   m_iStride;
  Int   m_iPad;
#if FIX_VSO_SETUP
  Int   m_iUsedHeight;
  Int   m_iHorOffset; 
#endif

  Int   m_iSampledWidth;
#if FIX_VSO_SETUP
#else
  Int   m_iSampledHeight;
#endif
  Int   m_iSampledStride;

  RenModelInPels* m_pcInputSamples[2];
  Int             m_iInputSamplesStride;

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

  //// Output Samples
  RenModelOutPels* m_pcOutputSamples;
  Int                   m_iOutputSamplesStride;

  Pel*  m_aapiRefVideoPel      [3];    // Dim1: Plane  0-> Y, 1->U, 2->V
  Int   m_aiRefVideoStrides    [3];    // Dim1: Plane  0-> Y, 1->U, 2->V

  // Rendering State
  Bool  m_bInOcclusion;                // Currently rendering in occluded area
  Int   m_iLastOccludedSPos;           // Position of last topmost shifted position
  Int   m_iLastOccludedSPosFP;         // Position of last topmost shifted position in FullPels

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
#ifdef LGE_VSO_EARLY_SKIP_A0093
  Bool  m_bEarlySkip; 
#endif

  // Derived settings
  Int   m_iGapTolerance;
  Int   m_iBlendZThres;
  Int   m_iBlendDistWeight;

  //// Current Pointers ////

  RenModelInPels*  m_pcInputSamplesRow [2];
  RenModelOutPels* m_pcOutputSamplesRow;

  //// MISC ////
  const Int m_iDistShift;                  // Shift in Distortion computation

  //// Early Skip 
#ifdef LGE_VSO_EARLY_SKIP_A0093
  Bool* m_pbHorSkip;
#endif
};

#endif //__TRENSINGLEMODEL__
