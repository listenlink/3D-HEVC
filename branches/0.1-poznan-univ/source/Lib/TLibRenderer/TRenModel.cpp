

#include "TRenImage.h"
#include "TRenFilter.h"
#include "TRenModel.h"

///////////  TRENMODEL //////////////////////
TRenModel::TRenModel()
{
  m_iPad               = 12;
  m_iWidth             = -1;
  m_iHeight            = -1;
  m_iNumOfBaseViews    = -1;
  m_iSampledWidth      = -1;
  m_iShiftPrec         =  0;
  m_iHoleMargin        =  1;

  // Current Error Type ///
  m_iCurrentView       = -1;
  m_iCurrentContent    = -1;
  m_iCurrentPlane      = -1;

  // Array of Models used to determine the Current Error ///
  m_iNumOfCurRenModels = -1;
  m_apcCurRenModels    = NULL;
  m_aiCurPosInModels   = NULL;

  // Array of Models ///
  m_iNumOfRenModels    = -1;
  m_apcRenModels       = NULL;

  // Mapping from View number to models ///
  m_aiNumOfModelsForDepthView = NULL;
  m_aapcRenModelForDepthView  = NULL;
  m_aaePosInModelForDepthView = NULL;

  m_aiNumOfModelsForVideoView = NULL;
  m_aapcRenModelForVideoView  = NULL;
  m_aaePosInModelForVideoView = NULL;
  m_aaeBaseViewPosInModel     = NULL;

  // Data
  m_aapiCurVideoPel      = NULL;
  m_aaiCurVideoStrides   = NULL;
  m_apiCurDepthPel       = NULL;
  m_aiCurDepthStrides    = NULL;

  m_aapiOrgVideoPel      = NULL;
  m_aaiOrgVideoStrides   = NULL;
  m_apiOrgDepthPel       = NULL;
  m_aiOrgDepthStrides    = NULL;

  m_aaaiSubPelShiftLut[0]= NULL;
  m_aaaiSubPelShiftLut[1]= NULL;

  /// Current Setup data ///
  m_abSetupVideoFromOrgForView = NULL;
  m_abSetupDepthFromOrgForView = NULL;
}

TRenModel::~TRenModel()
{
  if ( m_apcRenModels )
  {
    for (Int iNumModel = 0; iNumModel < m_iNumOfRenModels; iNumModel++)
    {
      if ( m_apcRenModels[iNumModel] ) delete m_apcRenModels[iNumModel];
    }

    delete[] m_apcRenModels;
  }

  for (Int iViewNum = 0; iViewNum < m_iNumOfBaseViews; iViewNum++)
  {
    if ( m_aapcRenModelForDepthView && m_aapcRenModelForDepthView [iViewNum] )
    {
      delete[]   m_aapcRenModelForDepthView [iViewNum];
    }

    if ( m_aaePosInModelForDepthView && m_aaePosInModelForDepthView[iViewNum] )
    {
      delete[]   m_aaePosInModelForDepthView[iViewNum];
    }

    if ( m_aapcRenModelForVideoView && m_aapcRenModelForVideoView[iViewNum] )
    {
      delete[]   m_aapcRenModelForVideoView[iViewNum];
    }

    if ( m_aaePosInModelForVideoView && m_aaePosInModelForVideoView[iViewNum] )
    {
      delete[]   m_aaePosInModelForVideoView[iViewNum];
    }

    if ( m_aaeBaseViewPosInModel && m_aaeBaseViewPosInModel[iViewNum] )
    {
      delete[]   m_aaeBaseViewPosInModel[iViewNum];
    }

    if ( m_aapiCurVideoPel && m_aapiCurVideoPel    [iViewNum] )
    {
      delete[] ( m_aapiCurVideoPel    [iViewNum][0] - m_iPad * m_aaiCurVideoStrides[iViewNum][0] - m_iPad );
      delete[] ( m_aapiCurVideoPel    [iViewNum][1] - m_iPad * m_aaiCurVideoStrides[iViewNum][1] - m_iPad );
      delete[] ( m_aapiCurVideoPel    [iViewNum][2] - m_iPad * m_aaiCurVideoStrides[iViewNum][2] - m_iPad );
      delete[]   m_aapiCurVideoPel    [iViewNum];
    }

    if ( m_aaiCurVideoStrides && m_aaiCurVideoStrides [iViewNum] )
    {
      delete[]   m_aaiCurVideoStrides [iViewNum];
    }

    if ( m_apiCurDepthPel )
    {
      delete[] ( m_apiCurDepthPel     [iViewNum]    - m_iPad * m_aiCurDepthStrides [iViewNum]    - m_iPad );
    }

    if ( m_aapiOrgVideoPel && m_aapiOrgVideoPel    [iViewNum] )
    {
      delete[] ( m_aapiOrgVideoPel    [iViewNum][0] - m_iPad * m_aaiOrgVideoStrides[iViewNum][0] - m_iPad );
      delete[] ( m_aapiOrgVideoPel    [iViewNum][1] - m_iPad * m_aaiOrgVideoStrides[iViewNum][1] - m_iPad );
      delete[] ( m_aapiOrgVideoPel    [iViewNum][2] - m_iPad * m_aaiOrgVideoStrides[iViewNum][2] - m_iPad );
      delete[]   m_aapiOrgVideoPel    [iViewNum];
    }

    if ( m_aaiOrgVideoStrides && m_aaiOrgVideoStrides [iViewNum] )
    {
      delete[]   m_aaiOrgVideoStrides [iViewNum];
    }

    if ( m_apiOrgDepthPel && m_apiOrgDepthPel     [iViewNum ] )
    {
      delete[] ( m_apiOrgDepthPel     [iViewNum]    - m_iPad * m_aiOrgDepthStrides [iViewNum]    - m_iPad );
    }
  }

  if(m_aiNumOfModelsForDepthView) delete[] m_aiNumOfModelsForDepthView;
  if(m_aapcRenModelForDepthView ) delete[] m_aapcRenModelForDepthView ;
  if(m_aaePosInModelForDepthView) delete[] m_aaePosInModelForDepthView;

  if(m_aiNumOfModelsForVideoView) delete[] m_aiNumOfModelsForVideoView;
  if(m_aapcRenModelForVideoView ) delete[] m_aapcRenModelForVideoView ;
  if(m_aaePosInModelForVideoView) delete[] m_aaePosInModelForVideoView;


  if(m_aaeBaseViewPosInModel    ) delete[] m_aaeBaseViewPosInModel    ;
  if(m_aapiCurVideoPel          ) delete[] m_aapiCurVideoPel          ;
  if(m_aaiCurVideoStrides       ) delete[] m_aaiCurVideoStrides       ;

  if(m_abSetupVideoFromOrgForView) delete[] m_abSetupVideoFromOrgForView;
  if(m_abSetupDepthFromOrgForView) delete[] m_abSetupDepthFromOrgForView;

  if(m_aapiOrgVideoPel          ) delete[] m_aapiOrgVideoPel          ;
  if(m_aaiOrgVideoStrides       ) delete[] m_aaiOrgVideoStrides       ;

  if(m_apiOrgDepthPel           ) delete[] m_apiOrgDepthPel           ;
  if(m_aiOrgDepthStrides        ) delete[] m_aiOrgDepthStrides        ;

  if(m_apiCurDepthPel           ) delete[] m_apiCurDepthPel           ;
  if(m_aiCurDepthStrides        ) delete[] m_aiCurDepthStrides        ;

  Int iNumEntries = (1 << ( m_iShiftPrec + 1) ) + 1 ;

  for (UInt uiEntry = 0; uiEntry < iNumEntries; uiEntry++)
  {
    if ( m_aaaiSubPelShiftLut[0] && m_aaaiSubPelShiftLut[0][uiEntry] )
      delete[] m_aaaiSubPelShiftLut[0][uiEntry];

    if ( m_aaaiSubPelShiftLut[1] && m_aaaiSubPelShiftLut[1][uiEntry] )
      delete[] m_aaaiSubPelShiftLut[1][uiEntry];
  }

  if( m_aaaiSubPelShiftLut[0] ) delete[] m_aaaiSubPelShiftLut[0];
  if( m_aaaiSubPelShiftLut[1] ) delete[] m_aaaiSubPelShiftLut[1];
}



Void
TRenModel::create( Int iNumOfBaseViews, Int iNumOfModels, Int iWidth, Int iHeight, Int iShiftPrec, Int iHoleMargin )
{
  m_iNumOfBaseViews     = iNumOfBaseViews;
  m_iNumOfRenModels     = iNumOfModels;
  m_iWidth              = iWidth;
  m_iHeight             = iHeight;
  m_iShiftPrec          = iShiftPrec;
  m_iHoleMargin         = iHoleMargin;

// LUTs for sub pel shifting
  Int iNumEntries = (1 << ( m_iShiftPrec + 1) ) + 1 ;
  m_aaaiSubPelShiftLut[0] = new Int*[ iNumEntries ];
  m_aaaiSubPelShiftLut[1] = new Int*[ iNumEntries ];
  for (UInt uiEntry = 0; uiEntry < iNumEntries; uiEntry++)
  {
    m_aaaiSubPelShiftLut[0][uiEntry] = new Int[ iNumEntries ];
    m_aaaiSubPelShiftLut[1][uiEntry] = new Int[ iNumEntries ];
  }

  TRenFilter::setSubPelShiftLUT( m_iShiftPrec, m_aaaiSubPelShiftLut[0], 0 );
  TRenFilter::setSubPelShiftLUT( m_iShiftPrec, m_aaaiSubPelShiftLut[1], -1);

  m_iSampledWidth       = iWidth << m_iShiftPrec;


  m_aapiCurVideoPel     = new Pel**     [m_iNumOfBaseViews];
  m_aaiCurVideoStrides  = new Int*      [m_iNumOfBaseViews];
  m_apiCurDepthPel      = new Pel*      [m_iNumOfBaseViews];
  m_aiCurDepthStrides   = new Int       [m_iNumOfBaseViews];

  m_aapiOrgVideoPel     = new Pel**     [m_iNumOfBaseViews];
  m_aaiOrgVideoStrides  = new Int*      [m_iNumOfBaseViews];

  m_apiOrgDepthPel      = new Pel*      [m_iNumOfBaseViews];
  m_aiOrgDepthStrides   = new Int       [m_iNumOfBaseViews];

  m_abSetupVideoFromOrgForView = new Bool[m_iNumOfBaseViews];
  m_abSetupDepthFromOrgForView = new Bool[m_iNumOfBaseViews];

  m_iNumOfCurRenModels   = 0;
  m_apcCurRenModels      = NULL;
  m_aiCurPosInModels     = NULL;

  m_apcRenModels         = new TRenSingleModel*       [m_iNumOfRenModels];

  m_aiNumOfModelsForDepthView = new Int               [m_iNumOfBaseViews];
  m_aapcRenModelForDepthView  = new TRenSingleModel** [m_iNumOfBaseViews];
  m_aaePosInModelForDepthView = new Int*              [m_iNumOfBaseViews];

  m_aiNumOfModelsForVideoView = new Int               [m_iNumOfBaseViews];
  m_aapcRenModelForVideoView  = new TRenSingleModel** [m_iNumOfBaseViews];
  m_aaePosInModelForVideoView = new Int*              [m_iNumOfBaseViews];
  m_aaeBaseViewPosInModel     = new Int*              [m_iNumOfBaseViews];


  for (Int iModelNum = 0; iModelNum < m_iNumOfRenModels; iModelNum++)
  {
    m_apcRenModels         [iModelNum] = new TRenSingleModel();
  }

  for (Int iViewNum = 0; iViewNum < m_iNumOfBaseViews; iViewNum++ )
  {
    m_aiNumOfModelsForDepthView[ iViewNum ] = 0;
    m_aiNumOfModelsForVideoView[ iViewNum ] = 0;

    m_aapcRenModelForDepthView [iViewNum] = new TRenSingleModel*[m_iNumOfRenModels];
    m_aapcRenModelForVideoView [iViewNum] = new TRenSingleModel*[m_iNumOfRenModels];

    m_aaePosInModelForDepthView[iViewNum] = new Int             [m_iNumOfRenModels];
    m_aaePosInModelForVideoView[iViewNum] = new Int             [m_iNumOfRenModels];
    m_aaeBaseViewPosInModel    [iViewNum] = new Int             [m_iNumOfRenModels];

    for (Int iModelNum = 0; iModelNum< m_iNumOfRenModels; iModelNum++)
    {
      m_aapcRenModelForDepthView [iViewNum] [iModelNum] = NULL;
      m_aapcRenModelForVideoView [iViewNum] [iModelNum] = NULL;
      m_aaePosInModelForDepthView[iViewNum] [iModelNum] = VIEWPOS_INVALID;
      m_aaePosInModelForVideoView[iViewNum] [iModelNum] = VIEWPOS_INVALID;
      m_aaeBaseViewPosInModel    [iViewNum] [iModelNum] = VIEWPOS_INVALID;
    };

    m_aaiCurVideoStrides  [iViewNum]     =  new Int[3];
    m_aaiCurVideoStrides  [iViewNum][0]  =  m_iSampledWidth + (m_iPad << 1);
    m_aaiCurVideoStrides  [iViewNum][1]  =  m_iSampledWidth + (m_iPad << 1);
    m_aaiCurVideoStrides  [iViewNum][2]  =  m_iSampledWidth + (m_iPad << 1);

    m_aapiCurVideoPel     [iViewNum]     = new Pel*[3];
    m_aapiCurVideoPel     [iViewNum][0]  = new Pel [ m_aaiCurVideoStrides[iViewNum][0] * ( m_iHeight  + (m_iPad << 1) )];
    m_aapiCurVideoPel     [iViewNum][1]  = new Pel [ m_aaiCurVideoStrides[iViewNum][1] * ( m_iHeight  + (m_iPad << 1) )];
    m_aapiCurVideoPel     [iViewNum][2]  = new Pel [ m_aaiCurVideoStrides[iViewNum][2] * ( m_iHeight  + (m_iPad << 1) )];

    m_aapiCurVideoPel     [iViewNum][0] += m_aaiCurVideoStrides[iViewNum][0] * m_iPad + m_iPad;
    m_aapiCurVideoPel     [iViewNum][1] += m_aaiCurVideoStrides[iViewNum][1] * m_iPad + m_iPad;
    m_aapiCurVideoPel     [iViewNum][2] += m_aaiCurVideoStrides[iViewNum][2] * m_iPad + m_iPad;

    m_aiCurDepthStrides   [iViewNum]     = m_iWidth + (m_iPad << 1);
    m_apiCurDepthPel      [iViewNum]     = new Pel[ m_aiCurDepthStrides[iViewNum] * ( m_iHeight  + (m_iPad << 1) ) ];
    m_apiCurDepthPel      [iViewNum]    += m_aiCurDepthStrides[iViewNum] * m_iPad + m_iPad;

    m_aaiOrgVideoStrides  [iViewNum]    =  new Int[3];
    m_aaiOrgVideoStrides  [iViewNum][0] = m_iSampledWidth + (m_iPad << 1);
    m_aaiOrgVideoStrides  [iViewNum][1] = m_iSampledWidth + (m_iPad << 1);
    m_aaiOrgVideoStrides  [iViewNum][2] = m_iSampledWidth + (m_iPad << 1);

    m_aapiOrgVideoPel     [iViewNum]     = new Pel*[3];
    m_aapiOrgVideoPel     [iViewNum][0]  = new Pel [ m_aaiOrgVideoStrides[iViewNum][0] * ( m_iHeight  + (m_iPad << 1) )];
    m_aapiOrgVideoPel     [iViewNum][1]  = new Pel [ m_aaiOrgVideoStrides[iViewNum][1] * ( m_iHeight  + (m_iPad << 1) )];
    m_aapiOrgVideoPel     [iViewNum][2]  = new Pel [ m_aaiOrgVideoStrides[iViewNum][2] * ( m_iHeight  + (m_iPad << 1) )];

    m_aapiOrgVideoPel     [iViewNum][0] += m_aaiOrgVideoStrides[iViewNum][0] * m_iPad + m_iPad;
    m_aapiOrgVideoPel     [iViewNum][1] += m_aaiOrgVideoStrides[iViewNum][1] * m_iPad + m_iPad;
    m_aapiOrgVideoPel     [iViewNum][2] += m_aaiOrgVideoStrides[iViewNum][2] * m_iPad + m_iPad;

    m_aiOrgDepthStrides   [iViewNum]     = m_iWidth + (m_iPad << 1);
    m_apiOrgDepthPel      [iViewNum]     = new Pel[ m_aiOrgDepthStrides[iViewNum] * ( m_iHeight  + (m_iPad << 1) ) ];
    m_apiOrgDepthPel      [iViewNum]    += m_aiOrgDepthStrides[iViewNum] * m_iPad + m_iPad;

    m_abSetupVideoFromOrgForView[iViewNum] = false;
    m_abSetupDepthFromOrgForView[iViewNum] = false;
  }
}

Void
TRenModel::createSingleModel( Int iBaseViewNum, Int iContent, Int iModelNum, Int iLeftViewNum, Int iRightViewNum, Bool bUseOrgRef, Int iBlendMode )
{
  Int iMode = ( (iLeftViewNum != -1) && ( iRightViewNum != -1 ) ) ? 2 : ( iLeftViewNum != -1 ? 0 : ( iRightViewNum != -1  ? 1 : -1 ) );

  AOT( iMode == -1);
  AOT( iModelNum < 0 || iModelNum > m_iNumOfRenModels );
  AOF( m_apcRenModels[iModelNum] );
  AOT( iLeftViewNum  < -1 || iLeftViewNum  > m_iNumOfBaseViews );
  AOT( iRightViewNum < -1 || iRightViewNum > m_iNumOfBaseViews );
  AOT( iBaseViewNum  < -1 || iBaseViewNum  > m_iNumOfBaseViews );
  AOT( iBaseViewNum != -1 && iBaseViewNum != iLeftViewNum && iBaseViewNum != iRightViewNum );
  AOT( iContent      < -1 || iContent > 1 );
  AOT( iBlendMode < -1 || iBlendMode > 2 );

  m_apcRenModels[iModelNum]->create( iMode ,m_iWidth, m_iHeight, m_iShiftPrec, m_aaaiSubPelShiftLut, m_iHoleMargin,  bUseOrgRef, iBlendMode );

  if ( iLeftViewNum != -1 )
  {
    xSetLRViewAndAddModel( iModelNum, iLeftViewNum, iContent,  VIEWPOS_LEFT,  (iBaseViewNum == -1  || iBaseViewNum == iLeftViewNum   ) );
  }

  if ( iRightViewNum != -1)
  {
    xSetLRViewAndAddModel( iModelNum, iRightViewNum, iContent, VIEWPOS_RIGHT, (iBaseViewNum == -1  || iBaseViewNum == iRightViewNum  ) );
  }
}


Void
TRenModel::setBaseView( Int iViewNum, TComPicYuv* pcPicYuvVideoData, TComPicYuv* pcPicYuvDepthData, TComPicYuv* pcPicYuvOrgVideoData, TComPicYuv* pcPicYuvOrgDepthData )
{
  AOT( iViewNum < 0 || iViewNum > m_iNumOfBaseViews );
  AOF( pcPicYuvVideoData->getHeight() == m_iHeight || pcPicYuvVideoData->getWidth() == m_iWidth );
  AOF( pcPicYuvDepthData->getHeight() == m_iHeight || pcPicYuvDepthData->getWidth() == m_iWidth );

  pcPicYuvVideoData->extendPicBorder();
  TRenFilter::sampleHorUp   ( m_iShiftPrec, pcPicYuvVideoData->getLumaAddr(), pcPicYuvVideoData->getStride() , m_iWidth,      m_iHeight,      m_aapiCurVideoPel[ iViewNum ][0], m_aaiCurVideoStrides[iViewNum][0] );
  TRenFilter::sampleCUpHorUp( m_iShiftPrec, pcPicYuvVideoData->getCbAddr()  , pcPicYuvVideoData->getCStride(), m_iWidth >> 1, m_iHeight >> 1, m_aapiCurVideoPel[ iViewNum ][1], m_aaiCurVideoStrides[iViewNum][1] );
  TRenFilter::sampleCUpHorUp( m_iShiftPrec, pcPicYuvVideoData->getCrAddr()  , pcPicYuvVideoData->getCStride(), m_iWidth >> 1, m_iHeight >> 1, m_aapiCurVideoPel[ iViewNum ][2], m_aaiCurVideoStrides[iViewNum][2] );
  TRenFilter::copy          (               pcPicYuvDepthData->getLumaAddr(), pcPicYuvDepthData->getStride(),   m_iWidth,     m_iHeight,      m_apiCurDepthPel [ iViewNum],     m_aiCurDepthStrides [iViewNum]    );

  // Used for rendering reference pic from original video data
  m_abSetupVideoFromOrgForView[iViewNum] = (pcPicYuvOrgVideoData != NULL);
  m_abSetupDepthFromOrgForView[iViewNum] = (pcPicYuvOrgDepthData != NULL);

  if ( m_abSetupVideoFromOrgForView[iViewNum] )
  {
    AOF( pcPicYuvOrgVideoData->getHeight() == m_iHeight || pcPicYuvOrgVideoData->getWidth() == m_iWidth );
    pcPicYuvOrgVideoData->extendPicBorder();
    TRenFilter::sampleHorUp   ( m_iShiftPrec, pcPicYuvOrgVideoData->getLumaAddr(), pcPicYuvOrgVideoData->getStride() , m_iWidth,      m_iHeight,      m_aapiOrgVideoPel[ iViewNum ][0], m_aaiOrgVideoStrides[iViewNum][0] );
    TRenFilter::sampleCUpHorUp( m_iShiftPrec, pcPicYuvOrgVideoData->getCbAddr()  , pcPicYuvOrgVideoData->getCStride(), m_iWidth >> 1, m_iHeight >> 1, m_aapiOrgVideoPel[ iViewNum ][1], m_aaiOrgVideoStrides[iViewNum][1] );
    TRenFilter::sampleCUpHorUp( m_iShiftPrec, pcPicYuvOrgVideoData->getCrAddr()  , pcPicYuvOrgVideoData->getCStride(), m_iWidth >> 1, m_iHeight >> 1, m_aapiOrgVideoPel[ iViewNum ][2], m_aaiOrgVideoStrides[iViewNum][2] );
  }

  if ( m_abSetupDepthFromOrgForView[iViewNum] )
  {
    AOF( pcPicYuvOrgDepthData->getHeight() == m_iHeight || pcPicYuvOrgDepthData->getWidth() == m_iWidth );
    TRenFilter::copy          (               pcPicYuvOrgDepthData->getLumaAddr(), pcPicYuvOrgDepthData->getStride(),  m_iWidth,     m_iHeight,      m_apiOrgDepthPel [ iViewNum],     m_aiOrgDepthStrides [iViewNum]    );
  }
}

Void
TRenModel::setSingleModel( Int iModelNum, Int** ppiShiftLutLeft, Int** ppiBaseShiftLutLeft, Int** ppiShiftLutRight, Int** ppiBaseShiftLutRight, Int iDistToLeft, TComPicYuv* pcPicYuvRefView )
{
  AOT( iModelNum < 0 || iModelNum > m_iNumOfRenModels );

  // Switch model  to original data for setup if given to render reference
  Bool bAnyRefFromOrg = false;
  for (Int iBaseViewIdx = 0; iBaseViewIdx < m_iNumOfBaseViews; iBaseViewIdx++ )
  {
    Bool bSetupFromOrgVideo = m_abSetupVideoFromOrgForView[iBaseViewIdx];
    Bool bSetupFromOrgDepth = m_abSetupDepthFromOrgForView[iBaseViewIdx];

    if ( ( bSetupFromOrgVideo || bSetupFromOrgDepth ) && m_aaeBaseViewPosInModel[iBaseViewIdx][iModelNum] != VIEWPOS_INVALID )
    {
      bAnyRefFromOrg = true;
      m_apcRenModels[iModelNum]->setLRView( m_aaeBaseViewPosInModel[iBaseViewIdx][iModelNum],
        ( bSetupFromOrgVideo ? m_aapiOrgVideoPel   : m_aapiCurVideoPel   ) [iBaseViewIdx],
        ( bSetupFromOrgVideo ? m_aaiOrgVideoStrides: m_aaiCurVideoStrides) [iBaseViewIdx],
        ( bSetupFromOrgDepth ? m_apiOrgDepthPel    : m_apiCurDepthPel    ) [iBaseViewIdx],
        ( bSetupFromOrgDepth ? m_aiOrgDepthStrides : m_aiCurDepthStrides ) [iBaseViewIdx] );
    }
  }

  m_apcRenModels[iModelNum]->setup     ( pcPicYuvRefView, ppiShiftLutLeft, ppiBaseShiftLutLeft, ppiShiftLutRight, ppiBaseShiftLutRight, iDistToLeft, false );

  // Setup to Org
  if ( bAnyRefFromOrg )
  {
    // Restore old values
    for (Int iBaseViewIdx = 0; iBaseViewIdx < m_iNumOfBaseViews; iBaseViewIdx++ )
    {
      if ( m_aaeBaseViewPosInModel[iBaseViewIdx][iModelNum] != VIEWPOS_INVALID )
      {
        m_apcRenModels[iModelNum]->setLRView(
          m_aaeBaseViewPosInModel[iBaseViewIdx][iModelNum],
          m_aapiCurVideoPel      [iBaseViewIdx],
          m_aaiCurVideoStrides   [iBaseViewIdx],
          m_apiCurDepthPel       [iBaseViewIdx],
          m_aiCurDepthStrides    [iBaseViewIdx]
        );
      }
    }

    // setup keeping reference from original data
    m_apcRenModels[iModelNum]->setup     ( pcPicYuvRefView, ppiShiftLutLeft, ppiBaseShiftLutLeft, ppiShiftLutRight, ppiBaseShiftLutRight, iDistToLeft, true );
  }
}

Void
TRenModel::setErrorMode( Int iView, Int iContent, int iPlane )
{
  AOT(iView > m_iNumOfBaseViews || iView < 0);
  AOT(iContent != 0  &&  iContent != 1);
  AOT(iPlane < 0     || iPlane > 3);

  m_iCurrentView    = iView;
  m_iCurrentContent = iContent;
  m_iCurrentPlane   = iPlane;

  if ( iContent == 1 )
  {
    m_iNumOfCurRenModels  = m_aiNumOfModelsForDepthView[iView];
    m_apcCurRenModels     = m_aapcRenModelForDepthView [iView];
    m_aiCurPosInModels    = m_aaePosInModelForDepthView[iView];
  }
  else
  {
    m_iNumOfCurRenModels  = m_aiNumOfModelsForVideoView[iView];
    m_apcCurRenModels     = m_aapcRenModelForVideoView [iView];
    m_aiCurPosInModels    = m_aaePosInModelForVideoView[iView];
  }
}

RMDist
TRenModel::getDist( Int iStartPosX, Int iStartPosY, Int iWidth, Int iHeight, Int iStride, Pel* piNewData )
{
  AOT( iWidth  + iStartPosX > m_iWidth  );
  AOT( iHeight + iStartPosY > m_iHeight );
  AOT( iStartPosX < 0);
  AOT( iStartPosY < 0);
  AOT( iWidth     < 0);
  AOT( iHeight    < 0);

  RMDist iDist = 0;

  for (Int iModelNum = 0; iModelNum < m_iNumOfCurRenModels; iModelNum++ )
  {
    if (m_iCurrentContent == 1)
    {
      iDist +=  m_apcCurRenModels[iModelNum]->getDistDepth  ( m_aiCurPosInModels[iModelNum], iStartPosX, iStartPosY,  iWidth,  iHeight,  iStride,  piNewData );
    }
    else
    {
      iDist +=  m_apcCurRenModels[iModelNum]->getDistVideo  ( m_aiCurPosInModels[iModelNum], m_iCurrentPlane ,iStartPosX, iStartPosY, iWidth, iHeight, iStride, piNewData );
    }
  }

  return ( iDist + (m_iNumOfCurRenModels >> 1) ) / m_iNumOfCurRenModels;
}

Void
TRenModel::setData( Int iStartPosX, Int iStartPosY, Int iWidth, Int iHeight, Int iStride, Pel* piNewData )
{
  iWidth  = min(iWidth , m_iWidth  - iStartPosX );
  iHeight = min(iHeight, m_iHeight - iStartPosY );

  AOT( iStartPosX < 0);
  AOT( iStartPosY < 0);
  AOT( iWidth     < 0);
  AOT( iHeight    < 0);

  for (Int iModelNum = 0; iModelNum < m_iNumOfCurRenModels; iModelNum++ )
  {
    if (m_iCurrentContent == 1)
    {
      m_apcCurRenModels[iModelNum]->setDepth  ( m_aiCurPosInModels[iModelNum], iStartPosX, iStartPosY, iWidth, iHeight, iStride, piNewData );

    }
    else
    {
      m_apcCurRenModels[iModelNum]->setVideo  ( m_aiCurPosInModels[iModelNum], m_iCurrentPlane ,iStartPosX, iStartPosY,  iWidth,  iHeight,  iStride, piNewData );
    }
  }

  if (m_iCurrentContent == 1)
  {
    Int iTargetStride = m_aiCurDepthStrides[ m_iCurrentView ];
    TRenFilter::copy( piNewData, iStride, iWidth, iHeight,  m_apiCurDepthPel[ m_iCurrentView ] + iStartPosY * iTargetStride + iStartPosX, iTargetStride );
  }
  else
  {
    Int iTargetStride = m_aaiCurVideoStrides[ m_iCurrentView ][m_iCurrentPlane];
    TRenFilter::copy( piNewData, iStride, iWidth, iHeight,  m_aapiCurVideoPel[ m_iCurrentView ][m_iCurrentPlane] + iStartPosY * iTargetStride + iStartPosX, iTargetStride);
  }
}

Void
TRenModel::getSynthVideo( Int iModelNum, Int iViewNum, TComPicYuv*& rpcPicYuvSynthVideo )
{
  Pel** ppiSynthVideoPel = NULL;
  Pel*  ppiSynthDepthPel = NULL;
  Int   iStride          = -1;
  Int   aiStrides[3];

  m_apcRenModels[iModelNum]->getSynthView(iViewNum, ppiSynthVideoPel, ppiSynthDepthPel, iStride );

  aiStrides[0] = iStride; aiStrides[1] = iStride; aiStrides[2] = iStride;
  xCopy2PicYuv( ppiSynthVideoPel, aiStrides, rpcPicYuvSynthVideo  );
}

Void
TRenModel::getSynthDepth( Int iModelNum, Int iViewNum, TComPicYuv*& rpcPicYuvSynthDepth )
{
  Pel** ppiSynthVideoPel = NULL;
  Pel*  piSynthDepthPel  = NULL;
  Int   iStride          = -1;

  m_apcRenModels[iModelNum]->getSynthView(iViewNum, ppiSynthVideoPel, piSynthDepthPel, iStride );
  TRenFilter::copy( piSynthDepthPel, iStride, m_iWidth, m_iHeight, rpcPicYuvSynthDepth->getLumaAddr(), rpcPicYuvSynthDepth->getStride() );
  rpcPicYuvSynthDepth->setChromaTo( 128 << g_uiBitIncrement );
}


Void
TRenModel::getTotalSSE( Int64& riSSEY, Int64& riSSEU, Int64& riSSEV )
{
  TComPicYuv cPicYuvSynth;
  cPicYuvSynth.create( m_iWidth, m_iHeight, g_uiMaxCUWidth, g_uiMaxCUHeight, g_uiMaxCUDepth );

  TComPicYuv cPicYuvTempRef;
  cPicYuvTempRef.create( m_iWidth, m_iHeight, g_uiMaxCUWidth, g_uiMaxCUHeight, g_uiMaxCUDepth);

  Int64 iSSEY = 0;
  Int64 iSSEU = 0;
  Int64 iSSEV = 0;

  for (Int iCurModel = 0; iCurModel < m_iNumOfCurRenModels; iCurModel++)
  {
    Pel** ppiSynthVideoPel = NULL;
    Pel*  ppiSynthDepthPel = NULL;
    Pel** ppiRefVideoPel   = NULL;
    Int   iSynStride       = -1;
    Int*  piRefStrides     = NULL;
    Int   aiSynStrides[3];

    m_apcCurRenModels[iCurModel]->getSynthView( m_aiCurPosInModels[iCurModel], ppiSynthVideoPel, ppiSynthDepthPel, iSynStride );
    aiSynStrides[0] = iSynStride; aiSynStrides[1] = iSynStride; aiSynStrides[2] = iSynStride;
    xCopy2PicYuv( ppiSynthVideoPel, aiSynStrides,  &cPicYuvSynth );

    TComPicYuv* pcPicYuvOrgRef;
    TComPicYuv* pcPicYuvRef;
    m_apcCurRenModels[iCurModel]->getRefView  ( pcPicYuvOrgRef, ppiRefVideoPel, piRefStrides );

    if (!pcPicYuvOrgRef )
    {
      xCopy2PicYuv( ppiRefVideoPel, piRefStrides,  &cPicYuvTempRef);
      pcPicYuvRef = &cPicYuvTempRef;
    }
    else
    {
      pcPicYuvRef = pcPicYuvOrgRef;
    }

    iSSEY += TRenFilter::SSE( cPicYuvSynth.getLumaAddr(), cPicYuvSynth.getStride(),  m_iWidth,      m_iHeight     , pcPicYuvRef->getLumaAddr(), pcPicYuvRef->getStride() );
    iSSEU += TRenFilter::SSE( cPicYuvSynth.getCbAddr()  , cPicYuvSynth.getCStride(), m_iWidth >> 1, m_iHeight >> 1, pcPicYuvRef->getCbAddr()  , pcPicYuvRef->getCStride());
    iSSEV += TRenFilter::SSE( cPicYuvSynth.getCrAddr()  , cPicYuvSynth.getCStride(), m_iWidth >> 1, m_iHeight >> 1, pcPicYuvRef->getCrAddr()  , pcPicYuvRef->getCStride());
  }

  riSSEY = ( iSSEY + (m_iNumOfCurRenModels >> 1) ) / m_iNumOfCurRenModels;
  riSSEU = ( iSSEU + (m_iNumOfCurRenModels >> 1) ) / m_iNumOfCurRenModels;
  riSSEV = ( iSSEV + (m_iNumOfCurRenModels >> 1) ) / m_iNumOfCurRenModels;

  cPicYuvTempRef.destroy();
  cPicYuvSynth  .destroy();
}

Void
TRenModel::xSetLRViewAndAddModel( Int iModelNum, Int iBaseViewNum, Int iContent, Int iViewPos, Bool bAdd )
{
  AOF(iViewPos == VIEWPOS_LEFT  || iViewPos == VIEWPOS_RIGHT);
  AOF(iContent == -1 || iContent == 0 || iContent == 1);
  AOT( iBaseViewNum  < 0 || iBaseViewNum  > m_iNumOfBaseViews );
  AOT( m_aaeBaseViewPosInModel[iBaseViewNum][iModelNum] != VIEWPOS_INVALID );
  m_aaeBaseViewPosInModel[iBaseViewNum][iModelNum] = iViewPos;

  m_apcRenModels[iModelNum]->setLRView( iViewPos, m_aapiCurVideoPel[iBaseViewNum], m_aaiCurVideoStrides[iBaseViewNum], m_apiCurDepthPel[iBaseViewNum], m_aiCurDepthStrides [iBaseViewNum] );

  if (bAdd)
  {
    if (iContent == 0 || iContent == -1 )
    {
      Int iNewModelIdxForView = m_aiNumOfModelsForVideoView[iBaseViewNum]++;
      m_aapcRenModelForVideoView [ iBaseViewNum ][ iNewModelIdxForView ] = m_apcRenModels[iModelNum];
      m_aaePosInModelForVideoView[ iBaseViewNum ][ iNewModelIdxForView ] = iViewPos;
    }

    if (iContent == 1 || iContent == -1 )
    {
      Int iNewModelIdxForView = m_aiNumOfModelsForDepthView[iBaseViewNum]++;
      m_aapcRenModelForDepthView [ iBaseViewNum ][ iNewModelIdxForView ] = m_apcRenModels[iModelNum];
      m_aaePosInModelForDepthView[ iBaseViewNum ][ iNewModelIdxForView ] = iViewPos;
    }
  }
}

Void
TRenModel::xCopy2PicYuv( Pel** ppiSrcVideoPel, Int* piStrides, TComPicYuv* rpcPicYuvTarget )
{
  TRenFilter::copy            ( ppiSrcVideoPel[0], piStrides[0], m_iWidth, m_iHeight, rpcPicYuvTarget->getLumaAddr(), rpcPicYuvTarget->getStride() );
  TRenFilter::sampleDown2Tap13( ppiSrcVideoPel[1], piStrides[1], m_iWidth, m_iHeight, rpcPicYuvTarget->getCbAddr  (), rpcPicYuvTarget->getCStride() );
  TRenFilter::sampleDown2Tap13( ppiSrcVideoPel[2], piStrides[2], m_iWidth, m_iHeight, rpcPicYuvTarget->getCrAddr  (), rpcPicYuvTarget->getCStride() );
}

#if GERHARD_RM_DEBUG_MM
Bool
TRenModel::compareAll( TRenModel* pcModel )
{
  Bool bEqual = true;
  for (Int iCurModel = 0; iCurModel < m_iNumOfRenModels; iCurModel++)
  {
    bEqual = bEqual &&  m_apcRenModels[iCurModel]->compare( pcModel->m_apcRenModels[iCurModel]  );
  }
  return bEqual;
}

Bool
TRenModel::compare( TRenModel* pcModel )
{
  Bool bEqual = true;
  for (Int iCurModel = 0; iCurModel < m_iNumOfCurRenModels; iCurModel++)
  {
    bEqual = bEqual &&  m_apcCurRenModels[iCurModel]->compare( pcModel->m_apcCurRenModels[iCurModel]  );
  }
  return bEqual;
}
#endif