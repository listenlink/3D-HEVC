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




#ifndef __TRENMODEL__
#define __TRENMODEL__

#include "TRenImage.h"
#include "TRenSingleModel.h"
#include "../TLibCommon/CommonDef.h"
#include "../TLibCommon/TComPicYuv.h"
#include "../TLibCommon/TypeDef.h"

class TRenModel
{
public:

  TRenModel();
  ~TRenModel();

  // Creation
  Void  create           ( Int iNumOfBaseViews, Int iNumOfModels, Int iWidth, Int iHeight, Int iShiftPrec, Int iHoleMargin );
  Void  createSingleModel( Int iBaseViewNum, Int iContent, Int iModelNum, Int iLeftViewNum, Int iRightViewNum, Bool bUseOrgRef, Int iBlendMode );

  // Set new Frame
  Void  setBaseView      ( Int iViewNum, TComPicYuv* pcPicYuvVideoData, TComPicYuv* pcPicYuvDepthData, TComPicYuv* pcPicYuvOrgVideoData, TComPicYuv* pcPicYuvOrgDepthData  );
  Void  setSingleModel   ( Int iModelNum, Int** ppiShiftLutLeft, Int** ppiBaseShiftLutLeft, Int** ppiShiftLutRight, Int** ppiBaseShiftLutRight, Int iDistToLeft, TComPicYuv* pcPicYuvRefView );

  // Set Mode
  Void  setErrorMode     ( Int iView, Int iContent, int iPlane );

  // Get Distortion, set Data
  Int64 getDist          ( Int iStartPosX, Int iStartPosY, Int iWidth, Int iHeight, Int iStride, Pel* piNewData  );
  Void  setData          ( Int iStartPosX, Int iStartPosY, Int iWidth, Int iHeight, Int iStride, Pel* piNewData  );

  // Get Rendered View
  Void  getSynthVideo    ( Int iModelNum, Int iViewNum, TComPicYuv*& rpcPicYuvSynthVideo );
  Void  getSynthDepth    ( Int iModelNum, Int iViewNum, TComPicYuv*& rpcPicYuvSynthDepth );

  // Get Total Distortion
  Void  getTotalSSE      (Int64& riSSEY, Int64& riSSEU, Int64& riSSEV );

private:
  // helpers
  Void xSetLRViewAndAddModel( Int iModelNum, Int iBaseViewNum, Int iContent, Int iViewPos, Bool bAdd );
  Void xCopy2PicYuv ( Pel** ppiSrcVideoPel, Int* piStrides, TComPicYuv* rpcPicYuvTarget );

  // Settings
  Int    m_iShiftPrec;
  Int**  m_aaaiSubPelShiftLut[2];
  Int    m_iHoleMargin;

  /// Size of Video and Depth
  Int m_iWidth;
  Int m_iHeight;
  Int m_iSampledWidth;
  Int m_iPad;

  Int m_iNumOfBaseViews;

  /// Current Error Type ///
  Int m_iCurrentView;
  Int m_iCurrentContent;
  Int m_iCurrentPlane;

  /// Array of Models used to determine the Current Error ///
  Int                m_iNumOfCurRenModels;
  TRenSingleModel**  m_apcCurRenModels;   // Array of pointers used for determination of current error
  Int*               m_aiCurPosInModels;  // Position of Current View in Model

  /// Array of Models ///
  Int                m_iNumOfRenModels;
  TRenSingleModel**  m_apcRenModels;   // Array of pointers to all created models

  /// Mapping from View number and Content type to models ///
  Int*               m_aiNumOfModelsForDepthView;
  TRenSingleModel*** m_aapcRenModelForDepthView;   // Dim1: ViewNumber
  Int**              m_aaePosInModelForDepthView; // Position in Model ( Left or Right)

  Int*               m_aiNumOfModelsForVideoView;
  TRenSingleModel*** m_aapcRenModelForVideoView;   // Dim1: ViewNumber
  Int**              m_aaePosInModelForVideoView; // Position in Model ( Left or Right) (local model numbering)

  /// Position of Base Views in Models ( global model numbering )
  Int**              m_aaeBaseViewPosInModel;

  /// Current Setup data ///
  Bool*              m_abSetupVideoFromOrgForView;  //: Dim1: ViewNumber, 0 ... use org; 1 ... use coded; 2; use org ref and coded in RDO
  Bool*              m_abSetupDepthFromOrgForView;

  /// DATA //
  // Cur

  /// Number of Base Views
  Pel*** m_aapiCurVideoPel   ; // Dim1: ViewNumber: Plane  0-> Y, 1->U, 2->V
  Int**  m_aaiCurVideoStrides; // Dim1: ViewPosition 0->Left, 1->Right; Dim2: Plane  0-> Y, 1->U, 2->V

  Pel**  m_apiCurDepthPel    ; // Dim1: ViewPosition
  Int*   m_aiCurDepthStrides ; // Dim1: ViewPosition

  Pel*** m_aapiOrgVideoPel   ; // Dim1: ViewPosition  Dim2: Plane  0-> Y, 1->U, 2->V
  Int**  m_aaiOrgVideoStrides; // Dim1: ViewPosition  Dim2: Plane  0-> Y, 1->U, 2->V

  Pel**  m_apiOrgDepthPel    ;    // Dim1: ViewPosition
  Int*   m_aiOrgDepthStrides ;    // Dim1: ViewPosition
};

#endif //__TRENMODEL__
