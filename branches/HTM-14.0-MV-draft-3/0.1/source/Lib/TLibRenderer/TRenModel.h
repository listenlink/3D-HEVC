


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

#if GERHARD_RM_DEBUG_MM
  Bool  compareAll       ( TRenModel* pcModel);
  Bool  compare          ( TRenModel* pcModel);
public:
#else
private:
#endif

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
