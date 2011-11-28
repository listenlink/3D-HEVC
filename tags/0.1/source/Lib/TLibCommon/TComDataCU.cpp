

/** \file     TComDataCU.cpp
    \brief    CU data structure
    \todo     not all entities are documented
*/

#include "TComDataCU.h"
#include "TComPic.h"
#include "TComDepthMapGenerator.h"
#include "TComResidualGenerator.h"

// ====================================================================================================================
// Constructor / destructor / create / destroy
// ====================================================================================================================

TComDataCU::TComDataCU()
{
  m_pcPic              = NULL;
  m_pcSlice            = NULL;
  m_puhDepth           = NULL;
  m_piTextureModeDepth = NULL;
  
  m_pePartSize         = NULL;
  m_pePredMode         = NULL;
  m_puiAlfCtrlFlag     = NULL;
  m_puiTmpAlfCtrlFlag  = NULL;
  m_puhWidth           = NULL;
  m_puhHeight          = NULL;
  m_phQP               = NULL;
  m_pbMergeFlag        = NULL;
  m_puhMergeIndex      = NULL;
  m_pbResPredAvailable = NULL;
  m_pbResPredFlag      = NULL;
  for( UInt ui = 0; ui < MRG_MAX_NUM_CANDS; ui++ )
  {
    m_apuhNeighbourCandIdx[ui] = NULL;
  }
  m_puhLumaIntraDir    = NULL;
  m_puhChromaIntraDir  = NULL;
  m_puhInterDir        = NULL;
  m_puhTrIdx           = NULL;
  m_puhCbf[0]          = NULL;
  m_puhCbf[1]          = NULL;
  m_puhCbf[2]          = NULL;
  m_pcTrCoeffY         = NULL;
  m_pcTrCoeffCb        = NULL;
  m_pcTrCoeffCr        = NULL;
  
  m_pcPattern          = NULL;
  
  m_pcCUAboveLeft      = NULL;
  m_pcCUAboveRight     = NULL;
  m_pcCUAbove          = NULL;
  m_pcCULeft           = NULL;
  
  m_apcCUColocated[0]  = NULL;
  m_apcCUColocated[1]  = NULL;
  
  m_apiMVPIdx[0]       = NULL;
  m_apiMVPIdx[1]       = NULL;
  m_apiMVPNum[0]       = NULL;
  m_apiMVPNum[1]       = NULL;
  
  m_bDecSubCu          = false;
  m_uiSliceStartCU        = 0;
  m_uiEntropySliceStartCU = 0;

#if SNY_DQP 
  m_bdQP               = false;               
#endif//SNY_DQP

#if HHI_DMM_INTRA
  m_puiWedgeFullTabIdx  = NULL;
  m_piWedgeFullDeltaDC1       = NULL;
  m_piWedgeFullDeltaDC2       = NULL;

  m_puiWedgePredDirTabIdx    = NULL;
  m_piWedgePredDirDeltaDC1   = NULL;
  m_piWedgePredDirDeltaDC2   = NULL;
  m_piWedgePredDirDeltaEnd   = NULL;

  m_puiWedgePredTexTabIdx     = NULL;
  m_piWedgePredTexDeltaDC1    = NULL;
  m_piWedgePredTexDeltaDC2    = NULL;

  m_piContourPredTexDeltaDC1  = NULL;
  m_piContourPredTexDeltaDC2  = NULL;

  m_pbTextureModesAllowed     = NULL;
#endif
}

TComDataCU::~TComDataCU()
{
}

Void TComDataCU::create(UInt uiNumPartition, UInt uiWidth, UInt uiHeight, Bool bDecSubCu)
{
  m_bDecSubCu = bDecSubCu;
  
  m_pcPic              = NULL;
  m_pcSlice            = NULL;
  m_uiNumPartition     = uiNumPartition;
  
  if ( !bDecSubCu )
  {
    m_phQP               = (UChar*    )xMalloc(UChar,    uiNumPartition);
    m_puhDepth           = (UChar*    )xMalloc(UChar,    uiNumPartition);
    m_piTextureModeDepth = (Int*      )xMalloc(Int,      uiNumPartition);
    m_puhWidth           = (UChar*    )xMalloc(UChar,    uiNumPartition);
    m_puhHeight          = (UChar*    )xMalloc(UChar,    uiNumPartition);
    m_pePartSize         = (PartSize* )xMalloc(PartSize, uiNumPartition);
    m_pePredMode         = (PredMode* )xMalloc(PredMode, uiNumPartition);
    
    m_puiAlfCtrlFlag     = (UInt*  )xMalloc(UInt,   uiNumPartition);
    
    m_pbMergeFlag        = (Bool*  )xMalloc(Bool,   uiNumPartition);
    m_puhMergeIndex      = (UChar* )xMalloc(UChar,  uiNumPartition);
    m_pbResPredAvailable = (Bool*  )xMalloc(Bool,   uiNumPartition);
    m_pbResPredFlag      = (Bool*  )xMalloc(Bool,   uiNumPartition);
    for( UInt ui = 0; ui < MRG_MAX_NUM_CANDS; ui++ )
    {
      m_apuhNeighbourCandIdx[ ui ] = (UChar* )xMalloc(UChar, uiNumPartition);
    }
    
    m_puhLumaIntraDir    = (UChar* )xMalloc(UChar,  uiNumPartition);
    m_puhChromaIntraDir  = (UChar* )xMalloc(UChar,  uiNumPartition);
    m_puhInterDir        = (UChar* )xMalloc(UChar,  uiNumPartition);
    
    m_puhTrIdx           = (UChar* )xMalloc(UChar,  uiNumPartition);
    
    m_puhCbf[0]          = (UChar* )xMalloc(UChar,  uiNumPartition);
    m_puhCbf[1]          = (UChar* )xMalloc(UChar,  uiNumPartition);
    m_puhCbf[2]          = (UChar* )xMalloc(UChar,  uiNumPartition);
    
    m_apiMVPIdx[0]       = (Int*   )xMalloc(Int,  uiNumPartition);
    m_apiMVPIdx[1]       = (Int*   )xMalloc(Int,  uiNumPartition);
    m_apiMVPNum[0]       = (Int*   )xMalloc(Int,  uiNumPartition);
    m_apiMVPNum[1]       = (Int*   )xMalloc(Int,  uiNumPartition);
    
    m_pcTrCoeffY         = (TCoeff*)xMalloc(TCoeff, uiWidth*uiHeight);
    m_pcTrCoeffCb        = (TCoeff*)xMalloc(TCoeff, uiWidth*uiHeight/4);
    m_pcTrCoeffCr        = (TCoeff*)xMalloc(TCoeff, uiWidth*uiHeight/4);
    
    m_acCUMvField[0].create( uiNumPartition );
    m_acCUMvField[1].create( uiNumPartition );
    
#if HHI_DMM_INTRA
    m_puiWedgeFullTabIdx  = (UInt*   )xMalloc(UInt,    uiNumPartition);
    m_piWedgeFullDeltaDC1       = (Int* )xMalloc(Int,  uiNumPartition);
    m_piWedgeFullDeltaDC2       = (Int* )xMalloc(Int,  uiNumPartition);

    m_puiWedgePredDirTabIdx    = (UInt*)xMalloc(UInt, uiNumPartition);
    m_piWedgePredDirDeltaDC1   = (Int* )xMalloc(Int,  uiNumPartition);
    m_piWedgePredDirDeltaDC2   = (Int* )xMalloc(Int,  uiNumPartition);
    m_piWedgePredDirDeltaEnd   = (Int* )xMalloc(Int,  uiNumPartition);

    m_puiWedgePredTexTabIdx     = (UInt*)xMalloc(UInt, uiNumPartition);
    m_piWedgePredTexDeltaDC1    = (Int* )xMalloc(Int,  uiNumPartition);
    m_piWedgePredTexDeltaDC2    = (Int* )xMalloc(Int,  uiNumPartition);

    m_piContourPredTexDeltaDC1  = (Int* )xMalloc(Int,  uiNumPartition);
    m_piContourPredTexDeltaDC2  = (Int* )xMalloc(Int,  uiNumPartition);

    m_pbTextureModesAllowed     = (Bool*)xMalloc(Bool, uiNumPartition);
#endif
  }
  else
  {
    m_acCUMvField[0].setNumPartition(uiNumPartition );
    m_acCUMvField[1].setNumPartition(uiNumPartition );
  }
  
  // create pattern memory
  m_pcPattern            = (TComPattern*)xMalloc(TComPattern, 1);
  
  // create motion vector fields
  
  m_pcCUAboveLeft      = NULL;
  m_pcCUAboveRight     = NULL;
  m_pcCUAbove          = NULL;
  m_pcCULeft           = NULL;
  
  m_apcCUColocated[0]  = NULL;
  m_apcCUColocated[1]  = NULL;
}

Void TComDataCU::destroy()
{
  m_pcPic              = NULL;
  m_pcSlice            = NULL;
  
  if ( m_pcPattern )
  { 
    xFree(m_pcPattern);
    m_pcPattern = NULL;
  }
  
  // encoder-side buffer free
  if ( !m_bDecSubCu )
  {
    if ( m_phQP               ) { xFree(m_phQP);                m_phQP              = NULL; }
    if ( m_puhDepth           ) { xFree(m_puhDepth);            m_puhDepth          = NULL; }
    if ( m_piTextureModeDepth ) { xFree(m_piTextureModeDepth);  m_piTextureModeDepth= NULL; }
    if ( m_puhWidth           ) { xFree(m_puhWidth);            m_puhWidth          = NULL; }
    if ( m_puhHeight          ) { xFree(m_puhHeight);           m_puhHeight         = NULL; }
    if ( m_pePartSize         ) { xFree(m_pePartSize);          m_pePartSize        = NULL; }
    if ( m_pePredMode         ) { xFree(m_pePredMode);          m_pePredMode        = NULL; }
    if ( m_puhCbf[0]          ) { xFree(m_puhCbf[0]);           m_puhCbf[0]         = NULL; }
    if ( m_puhCbf[1]          ) { xFree(m_puhCbf[1]);           m_puhCbf[1]         = NULL; }
    if ( m_puhCbf[2]          ) { xFree(m_puhCbf[2]);           m_puhCbf[2]         = NULL; }
    if ( m_puiAlfCtrlFlag     ) { xFree(m_puiAlfCtrlFlag);      m_puiAlfCtrlFlag    = NULL; }
    if ( m_puhInterDir        ) { xFree(m_puhInterDir);         m_puhInterDir       = NULL; }
    if ( m_pbMergeFlag        ) { xFree(m_pbMergeFlag);         m_pbMergeFlag       = NULL; }
    if ( m_puhMergeIndex      ) { xFree(m_puhMergeIndex);       m_puhMergeIndex     = NULL; }
    if ( m_pbResPredAvailable ) { xFree(m_pbResPredAvailable);  m_pbResPredAvailable= NULL; }
    if ( m_pbResPredFlag      ) { xFree(m_pbResPredFlag);       m_pbResPredFlag     = NULL; }
    for( UInt ui = 0; ui < MRG_MAX_NUM_CANDS; ui++ )
    {
      if( m_apuhNeighbourCandIdx[ ui ] ) { xFree(m_apuhNeighbourCandIdx[ ui ]); m_apuhNeighbourCandIdx[ ui ] = NULL; }
    }
    if ( m_puhLumaIntraDir    ) { xFree(m_puhLumaIntraDir);     m_puhLumaIntraDir   = NULL; }
    if ( m_puhChromaIntraDir  ) { xFree(m_puhChromaIntraDir);   m_puhChromaIntraDir = NULL; }
    if ( m_puhTrIdx           ) { xFree(m_puhTrIdx);            m_puhTrIdx          = NULL; }
    if ( m_pcTrCoeffY         ) { xFree(m_pcTrCoeffY);          m_pcTrCoeffY        = NULL; }
    if ( m_pcTrCoeffCb        ) { xFree(m_pcTrCoeffCb);         m_pcTrCoeffCb       = NULL; }
    if ( m_pcTrCoeffCr        ) { xFree(m_pcTrCoeffCr);         m_pcTrCoeffCr       = NULL; }
    if ( m_apiMVPIdx[0]       ) { xFree(m_apiMVPIdx[0]);        m_apiMVPIdx[0]      = NULL; }
    if ( m_apiMVPIdx[1]       ) { xFree(m_apiMVPIdx[1]);        m_apiMVPIdx[1]      = NULL; }
    if ( m_apiMVPNum[0]       ) { xFree(m_apiMVPNum[0]);        m_apiMVPNum[0]      = NULL; }
    if ( m_apiMVPNum[1]       ) { xFree(m_apiMVPNum[1]);        m_apiMVPNum[1]      = NULL; }
    
    m_acCUMvField[0].destroy();
    m_acCUMvField[1].destroy();
    
#if HHI_DMM_INTRA
    if ( m_puiWedgeFullTabIdx  ) { xFree(m_puiWedgeFullTabIdx);   m_puiWedgeFullTabIdx  = NULL; }
    if ( m_piWedgeFullDeltaDC1  ) { xFree(m_piWedgeFullDeltaDC1);   m_piWedgeFullDeltaDC1  = NULL; }
    if ( m_piWedgeFullDeltaDC2  ) { xFree(m_piWedgeFullDeltaDC2);   m_piWedgeFullDeltaDC2  = NULL; }

    if ( m_puiWedgePredDirTabIdx  ) { xFree(m_puiWedgePredDirTabIdx);  m_puiWedgePredDirTabIdx  = NULL; }
    if ( m_piWedgePredDirDeltaEnd ) { xFree(m_piWedgePredDirDeltaEnd); m_piWedgePredDirDeltaEnd = NULL; }
    if ( m_piWedgePredDirDeltaDC1 ) { xFree(m_piWedgePredDirDeltaDC1); m_piWedgePredDirDeltaDC1 = NULL; }
    if ( m_piWedgePredDirDeltaDC2 ) { xFree(m_piWedgePredDirDeltaDC2); m_piWedgePredDirDeltaDC2 = NULL; }

    if ( m_puiWedgePredTexTabIdx     ) { xFree(m_puiWedgePredTexTabIdx);     m_puiWedgePredTexTabIdx     = NULL; }
    if ( m_piWedgePredTexDeltaDC1    ) { xFree(m_piWedgePredTexDeltaDC1);    m_piWedgePredTexDeltaDC1    = NULL; }
    if ( m_piWedgePredTexDeltaDC2    ) { xFree(m_piWedgePredTexDeltaDC2);    m_piWedgePredTexDeltaDC2    = NULL; }

    if ( m_piContourPredTexDeltaDC1  ) { xFree(m_piContourPredTexDeltaDC1);  m_piContourPredTexDeltaDC1  = NULL; }
    if ( m_piContourPredTexDeltaDC2  ) { xFree(m_piContourPredTexDeltaDC2);  m_piContourPredTexDeltaDC2  = NULL; }

    if ( m_pbTextureModesAllowed     ) { xFree(m_pbTextureModesAllowed   );  m_pbTextureModesAllowed     = NULL; }
#endif    
  }
  
  m_pcCUAboveLeft       = NULL;
  m_pcCUAboveRight      = NULL;
  m_pcCUAbove           = NULL;
  m_pcCULeft            = NULL;
  
  m_apcCUColocated[0]   = NULL;
  m_apcCUColocated[1]   = NULL;
}

// ====================================================================================================================
// Public member functions
// ====================================================================================================================

// --------------------------------------------------------------------------------------------------------------------
// Initialization
// --------------------------------------------------------------------------------------------------------------------

/**
 - initialize top-level CU
 - internal buffers are already created
 - set values before encoding a CU
 .
 \param  pcPic     picture (TComPic) class pointer
 \param  iCUAddr   CU address
 */
Void TComDataCU::initCU( TComPic* pcPic, UInt iCUAddr )
{
  TComSlice* pcSlice   = pcPic->getSlice(pcPic->getCurrSliceIdx());
  m_pcSlice            = pcSlice;
  m_uiSliceStartCU     = pcSlice->getSliceCurStartCUAddr();
  m_uiEntropySliceStartCU  = pcSlice->getEntropySliceCurStartCUAddr();
  m_pcPic              = pcPic;
  m_uiCUAddr           = iCUAddr;
  m_uiCUPelX           = ( iCUAddr % pcPic->getFrameWidthInCU() ) * g_uiMaxCUWidth;
  m_uiCUPelY           = ( iCUAddr / pcPic->getFrameWidthInCU() ) * g_uiMaxCUHeight;
  m_uiAbsIdxInLCU      = 0;
  
  m_dTotalCost         = MAX_DOUBLE;
  m_uiTotalDistortion  = 0;
  m_uiTotalBits        = 0;
  m_uiNumPartition     = pcPic->getNumPartInCU();
  
  Int iSizeInUchar = sizeof( UChar ) * m_uiNumPartition;
  Int iSizeInUInt  = sizeof( UInt  ) * m_uiNumPartition;
  Int iSizeInBool  = sizeof( Bool  ) * m_uiNumPartition;

  Int iSizeInInt   = sizeof( Int   ) * m_uiNumPartition;
  
  memset( m_phQP,               pcSlice->getSliceQp(), iSizeInUchar );
  memset( m_puiAlfCtrlFlag,     0, iSizeInUInt  );
  memset( m_pbMergeFlag,        0, iSizeInBool  );
  memset( m_puhMergeIndex,      0, iSizeInUchar );
  memset( m_pbResPredAvailable, 0, iSizeInBool  );
  memset( m_pbResPredFlag,      0, iSizeInBool  );
  for( UInt ui = 0; ui < MRG_MAX_NUM_CANDS; ui++ )
  {
    memset( m_apuhNeighbourCandIdx[ ui ], 0, iSizeInUchar );
  }
  memset( m_puhLumaIntraDir,    2, iSizeInUchar );
  memset( m_puhChromaIntraDir,  0, iSizeInUchar );
  memset( m_puhInterDir,        0, iSizeInUchar );
  memset( m_puhTrIdx,           0, iSizeInUchar );
  memset( m_puhCbf[0],          0, iSizeInUchar );
  memset( m_puhCbf[1],          0, iSizeInUchar );
  memset( m_puhCbf[2],          0, iSizeInUchar );
  memset( m_puhDepth,           0, iSizeInUchar );
  memset( m_piTextureModeDepth,-1, iSizeInInt );
  
  UChar uhWidth  = g_uiMaxCUWidth;
  UChar uhHeight = g_uiMaxCUHeight;
  memset( m_puhWidth,          uhWidth,  iSizeInUchar );
  memset( m_puhHeight,         uhHeight, iSizeInUchar );
  
  for (UInt ui = 0; ui < m_uiNumPartition; ui++)
  {
    m_pePartSize[ui] = SIZE_NONE;
    m_pePredMode[ui] = MODE_NONE;
    
    m_apiMVPIdx[0][ui] = -1;
    m_apiMVPIdx[1][ui] = -1;
    m_apiMVPNum[0][ui] = -1;
    m_apiMVPNum[1][ui] = -1;
  }
  
  m_acCUMvField[0].clearMvField();
  m_acCUMvField[1].clearMvField();
  
  UInt uiTmp = m_puhWidth[0]*m_puhHeight[0];
  memset( m_pcTrCoeffY , 0, sizeof( TCoeff ) * uiTmp );
  
  uiTmp  >>= 2;
  memset( m_pcTrCoeffCb, 0, sizeof( TCoeff ) * uiTmp );
  memset( m_pcTrCoeffCr, 0, sizeof( TCoeff ) * uiTmp );
  
  // setting neighbor CU
  m_pcCULeft        = NULL;
  m_pcCUAbove       = NULL;
  m_pcCUAboveLeft   = NULL;
  m_pcCUAboveRight  = NULL;
  
  m_apcCUColocated[0] = NULL;
  m_apcCUColocated[1] = NULL;
  
  UInt uiWidthInCU = pcPic->getFrameWidthInCU();
  if ( m_uiCUAddr % uiWidthInCU )
  {
    m_pcCULeft = pcPic->getCU( m_uiCUAddr - 1 );
  }
  
  if ( m_uiCUAddr / uiWidthInCU )
  {
    m_pcCUAbove = pcPic->getCU( m_uiCUAddr - uiWidthInCU );
  }
  
  if ( m_pcCULeft && m_pcCUAbove )
  {
    m_pcCUAboveLeft = pcPic->getCU( m_uiCUAddr - uiWidthInCU - 1 );
  }
  
  if ( m_pcCUAbove && ( (m_uiCUAddr%uiWidthInCU) < (uiWidthInCU-1) )  )
  {
    m_pcCUAboveRight = pcPic->getCU( m_uiCUAddr - uiWidthInCU + 1 );
  }
  
  if ( pcSlice->getNumRefIdx( REF_PIC_LIST_0 ) > 0 )
  {
    m_apcCUColocated[0] = pcSlice->getRefPic( REF_PIC_LIST_0, 0)->getCU( m_uiCUAddr );
  }

  if ( pcSlice->getNumRefIdx( REF_PIC_LIST_1 ) > 0 )
  {
    m_apcCUColocated[1] = pcSlice->getRefPic( REF_PIC_LIST_1, 0)->getCU( m_uiCUAddr );
  }

#if HHI_DMM_INTRA
  memset( m_puiWedgeFullTabIdx,  0, iSizeInUInt   );
  memset( m_piWedgeFullDeltaDC1,       0, iSizeInInt  );
  memset( m_piWedgeFullDeltaDC2,       0, iSizeInInt  );

  memset( m_puiWedgePredDirTabIdx,    0, iSizeInUInt );
  memset( m_piWedgePredDirDeltaDC1,   0, iSizeInInt  );
  memset( m_piWedgePredDirDeltaDC2,   0, iSizeInInt  );
  memset( m_piWedgePredDirDeltaEnd,   0, iSizeInInt  );

  memset( m_puiWedgePredTexTabIdx,     0, iSizeInUInt );
  memset( m_piWedgePredTexDeltaDC1,    0, iSizeInInt  );
  memset( m_piWedgePredTexDeltaDC2,    0, iSizeInInt  );

  memset( m_piContourPredTexDeltaDC1,  0, iSizeInInt  );
  memset( m_piContourPredTexDeltaDC2,  0, iSizeInInt  );

  memset( m_pbTextureModesAllowed,  false, iSizeInBool );
#endif    
}

// initialize prediction data
Void TComDataCU::initEstData()
{
  m_dTotalCost         = MAX_DOUBLE;
  m_uiTotalDistortion  = 0;
  m_uiTotalBits        = 0;
  
  Int iSizeInUchar = sizeof( UChar  ) * m_uiNumPartition;
  Int iSizeInUInt  = sizeof( UInt   ) * m_uiNumPartition;
  Int iSizeInBool  = sizeof( Bool   ) * m_uiNumPartition;
  Int iSizeInInt   = sizeof( Int   ) * m_uiNumPartition;
  memset( m_phQP,              getSlice()->getSliceQp(), iSizeInUchar );
  memset( m_puiAlfCtrlFlag,     0, iSizeInUInt );
  memset( m_pbMergeFlag,        0, iSizeInBool  );
  memset( m_puhMergeIndex,      0, iSizeInUchar );
  memset( m_pbResPredAvailable, 0, iSizeInBool  );
  memset( m_pbResPredFlag,      0, iSizeInBool  );
  for( UInt ui = 0; ui < MRG_MAX_NUM_CANDS; ui++ )
  {
    memset( m_apuhNeighbourCandIdx[ ui ], 0, iSizeInUchar );
  }
  memset( m_puhLumaIntraDir,    2, iSizeInUchar );
  memset( m_puhChromaIntraDir,  0, iSizeInUchar );
  memset( m_puhInterDir,        0, iSizeInUchar );
  memset( m_puhTrIdx,           0, iSizeInUchar );
  memset( m_puhCbf[0],          0, iSizeInUchar );
  memset( m_puhCbf[1],          0, iSizeInUchar );
  memset( m_puhCbf[2],          0, iSizeInUchar );
  
  for (UInt ui = 0; ui < m_uiNumPartition; ui++)
  {
    m_pePartSize[ui] = SIZE_NONE;
    m_pePredMode[ui] = MODE_NONE;
    
    m_apiMVPIdx[0][ui] = -1;
    m_apiMVPIdx[1][ui] = -1;
    m_apiMVPNum[0][ui] = -1;
    m_apiMVPNum[1][ui] = -1;
  }
  
  UInt uiTmp = m_puhWidth[0]*m_puhHeight[0];
  memset( m_pcTrCoeffY , 0, sizeof(TCoeff)*uiTmp );
  
  uiTmp >>= 2;
  memset( m_pcTrCoeffCb, 0, sizeof(TCoeff)*uiTmp );
  memset( m_pcTrCoeffCr, 0, sizeof(TCoeff)*uiTmp );
  
  m_acCUMvField[0].clearMvField();
  m_acCUMvField[1].clearMvField();

#if HHI_DMM_INTRA
  memset( m_puiWedgeFullTabIdx,  0, iSizeInUInt   );
  memset( m_piWedgeFullDeltaDC1,       0, iSizeInInt  );
  memset( m_piWedgeFullDeltaDC2,       0, iSizeInInt  );

  memset( m_puiWedgePredDirTabIdx,    0, iSizeInUInt );
  memset( m_piWedgePredDirDeltaDC1,   0, iSizeInInt  );
  memset( m_piWedgePredDirDeltaDC2,   0, iSizeInInt  );
  memset( m_piWedgePredDirDeltaEnd,   0, iSizeInInt  );

  memset( m_puiWedgePredTexTabIdx,     0, iSizeInUInt );
  memset( m_piWedgePredTexDeltaDC1,    0, iSizeInInt  );
  memset( m_piWedgePredTexDeltaDC2,    0, iSizeInInt  );

  memset( m_piContourPredTexDeltaDC1,  0, iSizeInInt  );
  memset( m_piContourPredTexDeltaDC2,  0, iSizeInInt  );

  memset( m_pbTextureModesAllowed,  false, iSizeInBool );
#endif    

  memset( m_piTextureModeDepth, -1, iSizeInInt );
}

// initialize Sub partition
Void TComDataCU::initSubCU( TComDataCU* pcCU, UInt uiPartUnitIdx, UInt uiDepth )
{
  assert( uiPartUnitIdx<4 );
  
  UInt uiPartOffset = ( pcCU->getTotalNumPart()>>2 )*uiPartUnitIdx;
  
  m_pcPic              = pcCU->getPic();
  m_pcSlice            = pcCU->getSlice();
  m_uiCUAddr           = pcCU->getAddr();
  m_uiAbsIdxInLCU      = pcCU->getZorderIdxInCU() + uiPartOffset;
  m_uiCUPelX           = pcCU->getCUPelX() + ( g_uiMaxCUWidth  >> uiDepth )*( uiPartUnitIdx &  1 );
  m_uiCUPelY           = pcCU->getCUPelY() + ( g_uiMaxCUHeight >> uiDepth )*( uiPartUnitIdx >> 1 );
  
  m_dTotalCost         = MAX_DOUBLE;
  m_uiTotalDistortion  = 0;
  m_uiTotalBits        = 0;
  
  m_uiNumPartition     = pcCU->getTotalNumPart() >> 2;
  
  Int iSizeInUchar = sizeof( UChar  ) * m_uiNumPartition;
  Int iSizeInUInt  = sizeof( UInt   ) * m_uiNumPartition;
  Int iSizeInBool  = sizeof( Bool   ) * m_uiNumPartition;
  Int iSizeInInt   = sizeof( Int   ) * m_uiNumPartition;
  
  memset( m_phQP,              getSlice()->getSliceQp(), iSizeInUchar );
  memset( m_puiAlfCtrlFlag,     0, iSizeInUInt );
  memset( m_pbMergeFlag,        0, iSizeInBool  );
  memset( m_puhMergeIndex,      0, iSizeInUchar );
  memset( m_pbResPredAvailable, 0, iSizeInBool  );
  memset( m_pbResPredFlag,      0, iSizeInBool  );
  for( UInt ui = 0; ui < MRG_MAX_NUM_CANDS; ui++ )
  {
    memset( m_apuhNeighbourCandIdx[ ui ], 0, iSizeInUchar );
  }
  memset( m_puhLumaIntraDir,    2, iSizeInUchar );
  memset( m_puhChromaIntraDir,  0, iSizeInUchar );
  memset( m_puhInterDir,        0, iSizeInUchar );
  memset( m_puhTrIdx,           0, iSizeInUchar );
  memset( m_puhCbf[0],          0, iSizeInUchar );
  memset( m_puhCbf[1],          0, iSizeInUchar );
  memset( m_puhCbf[2],          0, iSizeInUchar );
  memset( m_puhDepth,     uiDepth, iSizeInUchar );
  memset( m_piTextureModeDepth, -1, iSizeInInt );
  
  UChar uhWidth  = g_uiMaxCUWidth  >> uiDepth;
  UChar uhHeight = g_uiMaxCUHeight >> uiDepth;
  memset( m_puhWidth,          uhWidth,  iSizeInUchar );
  memset( m_puhHeight,         uhHeight, iSizeInUchar );
  
  for (UInt ui = 0; ui < m_uiNumPartition; ui++)
  {
    m_pePartSize[ui] = SIZE_NONE;
    m_pePredMode[ui] = MODE_NONE;
    
    m_apiMVPIdx[0][ui] = -1;
    m_apiMVPIdx[1][ui] = -1;
    m_apiMVPNum[0][ui] = -1;
    m_apiMVPNum[1][ui] = -1;
  }
  
  UInt uiTmp = m_puhWidth[0]*m_puhHeight[0];
  memset( m_pcTrCoeffY , 0, sizeof(TCoeff)*uiTmp );
  
  uiTmp >>= 2;
  memset( m_pcTrCoeffCb, 0, sizeof(TCoeff)*uiTmp );
  memset( m_pcTrCoeffCr, 0, sizeof(TCoeff)*uiTmp );
  
  m_pcCULeft        = pcCU->getCULeft();
  m_pcCUAbove       = pcCU->getCUAbove();
  m_pcCUAboveLeft   = pcCU->getCUAboveLeft();
  m_pcCUAboveRight  = pcCU->getCUAboveRight();
  
  m_apcCUColocated[0] = pcCU->getCUColocated(REF_PIC_LIST_0);
  m_apcCUColocated[1] = pcCU->getCUColocated(REF_PIC_LIST_1);
  
  m_acCUMvField[0].clearMvField();
  m_acCUMvField[1].clearMvField();
  m_uiSliceStartCU          = pcCU->getSliceStartCU();
  m_uiEntropySliceStartCU   = pcCU->getEntropySliceStartCU();

#if HHI_DMM_INTRA
  memset( m_puiWedgeFullTabIdx,  0, iSizeInUInt   );
  memset( m_piWedgeFullDeltaDC1,       0, iSizeInInt  );
  memset( m_piWedgeFullDeltaDC2,       0, iSizeInInt  );

  memset( m_puiWedgePredDirTabIdx,    0, iSizeInUInt );
  memset( m_piWedgePredDirDeltaDC1,   0, iSizeInInt  );
  memset( m_piWedgePredDirDeltaDC2,   0, iSizeInInt  );
  memset( m_piWedgePredDirDeltaEnd,   0, iSizeInInt  );

  memset( m_puiWedgePredTexTabIdx,     0, iSizeInUInt );
  memset( m_piWedgePredTexDeltaDC1,    0, iSizeInInt  );
  memset( m_piWedgePredTexDeltaDC2,    0, iSizeInInt  );

  memset( m_piContourPredTexDeltaDC1,  0, iSizeInInt  );
  memset( m_piContourPredTexDeltaDC2,  0, iSizeInInt  );

  memset( m_pbTextureModesAllowed,  false, iSizeInBool );
#endif
}

// --------------------------------------------------------------------------------------------------------------------
// Copy
// --------------------------------------------------------------------------------------------------------------------

Void TComDataCU::copySubCU( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth )
{
  UInt uiPart = uiAbsPartIdx;
  
  m_pcPic              = pcCU->getPic();
  m_pcSlice            = pcCU->getSlice();
  m_uiCUAddr           = pcCU->getAddr();
  m_uiAbsIdxInLCU      = uiAbsPartIdx;
  
  m_uiCUPelX           = pcCU->getCUPelX() + g_auiRasterToPelX[ g_auiZscanToRaster[uiAbsPartIdx] ];
  m_uiCUPelY           = pcCU->getCUPelY() + g_auiRasterToPelY[ g_auiZscanToRaster[uiAbsPartIdx] ];
  
  UInt uiWidth         = g_uiMaxCUWidth  >> uiDepth;
  UInt uiHeight        = g_uiMaxCUHeight >> uiDepth;
  
  m_phQP=pcCU->getQP()                    + uiPart;
  m_pePartSize=pcCU->getPartitionSize()   + uiPart;
  m_pePredMode=pcCU->getPredictionMode()  + uiPart;
  
  m_pbMergeFlag         = pcCU->getMergeFlag()        + uiPart;
  m_puhMergeIndex       = pcCU->getMergeIndex()       + uiPart;
  m_pbResPredAvailable  = pcCU->getResPredAvail()     + uiPart;
  m_pbResPredFlag       = pcCU->getResPredFlag ()     + uiPart;
  for( UInt ui = 0; ui < MRG_MAX_NUM_CANDS; ui++ )
  {
    m_apuhNeighbourCandIdx[ ui ] = pcCU->getNeighbourCandIdx( ui ) + uiPart;
  }

  m_puhLumaIntraDir     = pcCU->getLumaIntraDir()     + uiPart;
  m_puhChromaIntraDir   = pcCU->getChromaIntraDir()   + uiPart;
  m_puhInterDir         = pcCU->getInterDir()         + uiPart;
  m_puhTrIdx            = pcCU->getTransformIdx()     + uiPart;
  
  m_puhCbf[0]= pcCU->getCbf(TEXT_LUMA)            + uiPart;
  m_puhCbf[1]= pcCU->getCbf(TEXT_CHROMA_U)        + uiPart;
  m_puhCbf[2]= pcCU->getCbf(TEXT_CHROMA_V)        + uiPart;
  
  m_puhDepth=pcCU->getDepth()                     + uiPart;
  m_puhWidth=pcCU->getWidth()                     + uiPart;
  m_puhHeight=pcCU->getHeight()                   + uiPart;
  m_piTextureModeDepth=pcCU->getTextureModeDepth()+ uiPart;
  
  m_apiMVPIdx[0]=pcCU->getMVPIdx(REF_PIC_LIST_0)  + uiPart;
  m_apiMVPIdx[1]=pcCU->getMVPIdx(REF_PIC_LIST_1)  + uiPart;
  m_apiMVPNum[0]=pcCU->getMVPNum(REF_PIC_LIST_0)  + uiPart;
  m_apiMVPNum[1]=pcCU->getMVPNum(REF_PIC_LIST_1)  + uiPart;
  
  m_pcCUAboveLeft      = pcCU->getCUAboveLeft();
  m_pcCUAboveRight     = pcCU->getCUAboveRight();
  m_pcCUAbove          = pcCU->getCUAbove();
  m_pcCULeft           = pcCU->getCULeft();
  
  m_apcCUColocated[0] = pcCU->getCUColocated(REF_PIC_LIST_0);
  m_apcCUColocated[1] = pcCU->getCUColocated(REF_PIC_LIST_1);
  
  UInt uiTmp = uiWidth*uiHeight;
  UInt uiMaxCuWidth=pcCU->getSlice()->getSPS()->getMaxCUWidth();
  UInt uiMaxCuHeight=pcCU->getSlice()->getSPS()->getMaxCUHeight();
  
  UInt uiCoffOffset = uiMaxCuWidth*uiMaxCuHeight*uiAbsPartIdx/pcCU->getPic()->getNumPartInCU();
  
  m_pcTrCoeffY=pcCU->getCoeffY()  + uiCoffOffset;
  
  uiTmp >>= 2;
  uiCoffOffset >>=2;
  m_pcTrCoeffCb=pcCU->getCoeffCb() + uiCoffOffset;
  m_pcTrCoeffCr=pcCU->getCoeffCr() + uiCoffOffset;
  
  m_acCUMvField[0].setMvPtr(pcCU->getCUMvField(REF_PIC_LIST_0)->getMv()     + uiPart);
  m_acCUMvField[0].setMvdPtr(pcCU->getCUMvField(REF_PIC_LIST_0)->getMvd()    + uiPart);
  m_acCUMvField[0].setRefIdxPtr(pcCU->getCUMvField(REF_PIC_LIST_0)->getRefIdx() + uiPart);
  m_acCUMvField[1].setMvPtr(pcCU->getCUMvField(REF_PIC_LIST_1)->getMv()     + uiPart);
  m_acCUMvField[1].setMvdPtr(pcCU->getCUMvField(REF_PIC_LIST_1)->getMvd()    + uiPart);
  m_acCUMvField[1].setRefIdxPtr(pcCU->getCUMvField(REF_PIC_LIST_1)->getRefIdx() + uiPart);
  m_uiSliceStartCU        = pcCU->getSliceStartCU();
  m_uiEntropySliceStartCU = pcCU->getEntropySliceStartCU();

#if HHI_DMM_INTRA
  m_puiWedgeFullTabIdx  = pcCU->getWedgeFullTabIdx()  + uiPart;   
  m_piWedgeFullDeltaDC1       = pcCU->getWedgeFullDeltaDC1()      + uiPart;   
  m_piWedgeFullDeltaDC2       = pcCU->getWedgeFullDeltaDC2()      + uiPart;   

  m_puiWedgePredDirTabIdx    = pcCU->getWedgePredDirTabIdx()     + uiPart;   
  m_piWedgePredDirDeltaDC1   = pcCU->getWedgePredDirDeltaDC1()   + uiPart;   
  m_piWedgePredDirDeltaDC2   = pcCU->getWedgePredDirDeltaDC2()   + uiPart;   
  m_piWedgePredDirDeltaEnd   = pcCU->getWedgePredDirDeltaEnd()   + uiPart;

  m_puiWedgePredTexTabIdx     = pcCU->getWedgePredTexTabIdx()     + uiPart;   
  m_piWedgePredTexDeltaDC1    = pcCU->getWedgePredTexDeltaDC1()   + uiPart;   
  m_piWedgePredTexDeltaDC2    = pcCU->getWedgePredTexDeltaDC2()   + uiPart;   

  m_piContourPredTexDeltaDC1  = pcCU->getContourPredTexDeltaDC1() + uiPart;   
  m_piContourPredTexDeltaDC2  = pcCU->getContourPredTexDeltaDC2() + uiPart;   

  m_pbTextureModesAllowed     = pcCU->getTextureModeAllowance()   + uiPart;   
#endif
}

// Copy inter prediction info from the biggest CU
Void TComDataCU::copyInterPredInfoFrom    ( TComDataCU* pcCU, UInt uiAbsPartIdx, RefPicList eRefPicList )
{
  m_pcPic              = pcCU->getPic();
  m_pcSlice            = pcCU->getSlice();
  m_uiCUAddr           = pcCU->getAddr();
  m_uiAbsIdxInLCU      = uiAbsPartIdx;
  
  Int iRastPartIdx     = g_auiZscanToRaster[uiAbsPartIdx];
  m_uiCUPelX           = pcCU->getCUPelX() + m_pcPic->getMinCUWidth ()*( iRastPartIdx % m_pcPic->getNumPartInWidth() );
  m_uiCUPelY           = pcCU->getCUPelY() + m_pcPic->getMinCUHeight()*( iRastPartIdx / m_pcPic->getNumPartInWidth() );
  
  m_pcCUAboveLeft      = pcCU->getCUAboveLeft();
  m_pcCUAboveRight     = pcCU->getCUAboveRight();
  m_pcCUAbove          = pcCU->getCUAbove();
  m_pcCULeft           = pcCU->getCULeft();
  
  m_apcCUColocated[0]  = pcCU->getCUColocated(REF_PIC_LIST_0);
  m_apcCUColocated[1]  = pcCU->getCUColocated(REF_PIC_LIST_1);
  
  m_pePartSize         = pcCU->getPartitionSize ()        + uiAbsPartIdx;
  m_pePredMode         = pcCU->getPredictionMode()        + uiAbsPartIdx;
  m_puhInterDir        = pcCU->getInterDir      ()        + uiAbsPartIdx;
  
  m_puhDepth           = pcCU->getDepth ()                + uiAbsPartIdx;
  m_puhWidth           = pcCU->getWidth ()                + uiAbsPartIdx;
  m_puhHeight          = pcCU->getHeight()                + uiAbsPartIdx;
  
  m_pbMergeFlag        = pcCU->getMergeFlag()             + uiAbsPartIdx;
  m_puhMergeIndex      = pcCU->getMergeIndex()            + uiAbsPartIdx;
  m_pbResPredAvailable = pcCU->getResPredAvail()          + uiAbsPartIdx;
  m_pbResPredFlag      = pcCU->getResPredFlag ()          + uiAbsPartIdx;
  for( UInt ui = 0; ui < MRG_MAX_NUM_CANDS; ui ++ )
  {
    m_apuhNeighbourCandIdx[ui] = pcCU->getNeighbourCandIdx( ui ) + uiAbsPartIdx;
  }

  m_apiMVPIdx[eRefPicList] = pcCU->getMVPIdx(eRefPicList) + uiAbsPartIdx;
  m_apiMVPNum[eRefPicList] = pcCU->getMVPNum(eRefPicList) + uiAbsPartIdx;
  
  m_acCUMvField[eRefPicList].setMvPtr(pcCU->getCUMvField(eRefPicList)->getMv()     + uiAbsPartIdx);
  m_acCUMvField[eRefPicList].setMvdPtr(pcCU->getCUMvField(eRefPicList)->getMvd()    + uiAbsPartIdx);
  m_acCUMvField[eRefPicList].setRefIdxPtr(pcCU->getCUMvField(eRefPicList)->getRefIdx() + uiAbsPartIdx);
  m_piTextureModeDepth = pcCU->getTextureModeDepth() + uiAbsPartIdx;

  m_uiSliceStartCU        = pcCU->getSliceStartCU();
  m_uiEntropySliceStartCU = pcCU->getEntropySliceStartCU();
}

// Copy small CU to bigger CU.
// One of quarter parts overwritten by predicted sub part.
Void TComDataCU::copyPartFrom( TComDataCU* pcCU, UInt uiPartUnitIdx, UInt uiDepth )
{
  assert( uiPartUnitIdx<4 );
  
  m_dTotalCost         += pcCU->getTotalCost();
  m_uiTotalDistortion  += pcCU->getTotalDistortion();
  m_uiTotalBits        += pcCU->getTotalBits();
  
  UInt uiOffset         = pcCU->getTotalNumPart()*uiPartUnitIdx;
  
  UInt uiNumPartition = pcCU->getTotalNumPart();
  Int iSizeInUchar  = sizeof( UChar ) * uiNumPartition;
  Int iSizeInUInt   = sizeof( UInt  ) * uiNumPartition;
  Int iSizeInInt    = sizeof( Int   ) * uiNumPartition;
  Int iSizeInBool   = sizeof( Bool  ) * uiNumPartition;
  
  memcpy( m_phQP       + uiOffset, pcCU->getQP(),             iSizeInUchar                        );
  memcpy( m_pePartSize + uiOffset, pcCU->getPartitionSize(),  sizeof( PartSize ) * uiNumPartition );
  memcpy( m_pePredMode + uiOffset, pcCU->getPredictionMode(), sizeof( PredMode ) * uiNumPartition );
  
  memcpy( m_puiAlfCtrlFlag      + uiOffset, pcCU->getAlfCtrlFlag(),       iSizeInUInt  );
  memcpy( m_pbMergeFlag         + uiOffset, pcCU->getMergeFlag(),         iSizeInBool  );
  memcpy( m_puhMergeIndex       + uiOffset, pcCU->getMergeIndex(),        iSizeInUchar );
  memcpy( m_pbResPredAvailable  + uiOffset, pcCU->getResPredAvail(),      iSizeInBool  );
  memcpy( m_pbResPredFlag       + uiOffset, pcCU->getResPredFlag(),       iSizeInBool  );
  for( UInt ui = 0; ui < MRG_MAX_NUM_CANDS; ui++ )
  {
    memcpy( m_apuhNeighbourCandIdx[ ui ] + uiOffset, pcCU->getNeighbourCandIdx( ui ), iSizeInUchar );
  }
  memcpy( m_puhLumaIntraDir     + uiOffset, pcCU->getLumaIntraDir(),      iSizeInUchar );
  memcpy( m_puhChromaIntraDir   + uiOffset, pcCU->getChromaIntraDir(),    iSizeInUchar );
  memcpy( m_puhInterDir         + uiOffset, pcCU->getInterDir(),          iSizeInUchar );
  memcpy( m_puhTrIdx            + uiOffset, pcCU->getTransformIdx(),      iSizeInUchar );
  
  memcpy( m_puhCbf[0] + uiOffset, pcCU->getCbf(TEXT_LUMA)    , iSizeInUchar );
  memcpy( m_puhCbf[1] + uiOffset, pcCU->getCbf(TEXT_CHROMA_U), iSizeInUchar );
  memcpy( m_puhCbf[2] + uiOffset, pcCU->getCbf(TEXT_CHROMA_V), iSizeInUchar );
  
  memcpy( m_puhDepth  + uiOffset, pcCU->getDepth(),  iSizeInUchar );
  memcpy( m_puhWidth  + uiOffset, pcCU->getWidth(),  iSizeInUchar );
  memcpy( m_puhHeight + uiOffset, pcCU->getHeight(), iSizeInUchar );
  
  memcpy( m_apiMVPIdx[0] + uiOffset, pcCU->getMVPIdx(REF_PIC_LIST_0), iSizeInInt );
  memcpy( m_apiMVPIdx[1] + uiOffset, pcCU->getMVPIdx(REF_PIC_LIST_1), iSizeInInt );
  memcpy( m_apiMVPNum[0] + uiOffset, pcCU->getMVPNum(REF_PIC_LIST_0), iSizeInInt );
  memcpy( m_apiMVPNum[1] + uiOffset, pcCU->getMVPNum(REF_PIC_LIST_1), iSizeInInt );
  
  m_pcCUAboveLeft      = pcCU->getCUAboveLeft();
  m_pcCUAboveRight     = pcCU->getCUAboveRight();
  m_pcCUAbove          = pcCU->getCUAbove();
  m_pcCULeft           = pcCU->getCULeft();
  
  m_apcCUColocated[0] = pcCU->getCUColocated(REF_PIC_LIST_0);
  m_apcCUColocated[1] = pcCU->getCUColocated(REF_PIC_LIST_1);
  
  m_acCUMvField[0].copyFrom( pcCU->getCUMvField( REF_PIC_LIST_0 ), pcCU->getTotalNumPart(), uiOffset );
  m_acCUMvField[1].copyFrom( pcCU->getCUMvField( REF_PIC_LIST_1 ), pcCU->getTotalNumPart(), uiOffset );
  
  UInt uiTmp  = g_uiMaxCUWidth*g_uiMaxCUHeight >> (uiDepth<<1);
  UInt uiTmp2 = uiPartUnitIdx*uiTmp;
  memcpy( m_pcTrCoeffY  + uiTmp2, pcCU->getCoeffY(),  sizeof(TCoeff)*uiTmp );
  
  uiTmp >>= 2; uiTmp2>>= 2;
  memcpy( m_pcTrCoeffCb + uiTmp2, pcCU->getCoeffCb(), sizeof(TCoeff)*uiTmp );
  memcpy( m_pcTrCoeffCr + uiTmp2, pcCU->getCoeffCr(), sizeof(TCoeff)*uiTmp );
  m_uiSliceStartCU        = pcCU->getSliceStartCU();
  m_uiEntropySliceStartCU = pcCU->getEntropySliceStartCU();

#if HHI_DMM_INTRA
  memcpy( m_puiWedgeFullTabIdx   + uiOffset, pcCU->getWedgeFullTabIdx(),    iSizeInUInt    );
  memcpy( m_piWedgeFullDeltaDC1       + uiOffset, pcCU->getWedgeFullDeltaDC1(),      iSizeInInt  );
  memcpy( m_piWedgeFullDeltaDC2       + uiOffset, pcCU->getWedgeFullDeltaDC2(),      iSizeInInt  );

  memcpy( m_puiWedgePredDirTabIdx    + uiOffset, pcCU->getWedgePredDirTabIdx(),     iSizeInUInt );
  memcpy( m_piWedgePredDirDeltaDC1   + uiOffset, pcCU->getWedgePredDirDeltaDC1(),   iSizeInInt  );
  memcpy( m_piWedgePredDirDeltaDC2   + uiOffset, pcCU->getWedgePredDirDeltaDC2(),   iSizeInInt  );
  memcpy( m_piWedgePredDirDeltaEnd   + uiOffset, pcCU->getWedgePredDirDeltaEnd(),   iSizeInInt  );

  memcpy( m_puiWedgePredTexTabIdx     + uiOffset, pcCU->getWedgePredTexTabIdx(),     iSizeInUInt );
  memcpy( m_piWedgePredTexDeltaDC1    + uiOffset, pcCU->getWedgePredTexDeltaDC1(),   iSizeInInt  );
  memcpy( m_piWedgePredTexDeltaDC2    + uiOffset, pcCU->getWedgePredTexDeltaDC2(),   iSizeInInt  );

  memcpy( m_piContourPredTexDeltaDC1  + uiOffset, pcCU->getContourPredTexDeltaDC1(), iSizeInInt  );
  memcpy( m_piContourPredTexDeltaDC2  + uiOffset, pcCU->getContourPredTexDeltaDC2(), iSizeInInt  );

  memcpy( m_pbTextureModesAllowed     + uiOffset, pcCU->getTextureModeAllowance(),   iSizeInBool );
#endif

  memcpy( m_piTextureModeDepth + uiOffset, pcCU->getTextureModeDepth(), iSizeInInt );
}

// Copy current predicted part to a CU in picture.
// It is used to predict for next part
Void TComDataCU::copyToPic( UChar uhDepth )
{
  TComDataCU*& rpcCU = m_pcPic->getCU( m_uiCUAddr );
  
  rpcCU->getTotalCost()       = m_dTotalCost;
  rpcCU->getTotalDistortion() = m_uiTotalDistortion;
  rpcCU->getTotalBits()       = m_uiTotalBits;
  
  Int iSizeInUchar  = sizeof( UChar ) * m_uiNumPartition;
  Int iSizeInUInt   = sizeof( UInt  ) * m_uiNumPartition;
  Int iSizeInInt    = sizeof( Int   ) * m_uiNumPartition;
  Int iSizeInBool   = sizeof( Bool  ) * m_uiNumPartition;
  
  memcpy( rpcCU->getQP() + m_uiAbsIdxInLCU, m_phQP, iSizeInUchar );
  
  memcpy( rpcCU->getPartitionSize()  + m_uiAbsIdxInLCU, m_pePartSize, sizeof( PartSize ) * m_uiNumPartition );
  memcpy( rpcCU->getPredictionMode() + m_uiAbsIdxInLCU, m_pePredMode, sizeof( PredMode ) * m_uiNumPartition );
  
  memcpy( rpcCU->getAlfCtrlFlag()    + m_uiAbsIdxInLCU, m_puiAlfCtrlFlag,    iSizeInUInt  );
  
  memcpy( rpcCU->getMergeFlag()         + m_uiAbsIdxInLCU, m_pbMergeFlag,         iSizeInBool  );
  memcpy( rpcCU->getMergeIndex()        + m_uiAbsIdxInLCU, m_puhMergeIndex,       iSizeInUchar );
  memcpy( rpcCU->getResPredAvail()      + m_uiAbsIdxInLCU, m_pbResPredAvailable,  iSizeInBool  );
  memcpy( rpcCU->getResPredFlag()       + m_uiAbsIdxInLCU, m_pbResPredFlag,       iSizeInBool  );
  for( UInt ui = 0; ui < MRG_MAX_NUM_CANDS; ui++ )
  {
    memcpy( rpcCU->getNeighbourCandIdx( ui ) + m_uiAbsIdxInLCU, m_apuhNeighbourCandIdx[ui], iSizeInUchar );
  }
  memcpy( rpcCU->getLumaIntraDir()      + m_uiAbsIdxInLCU, m_puhLumaIntraDir,     iSizeInUchar );
  memcpy( rpcCU->getChromaIntraDir()    + m_uiAbsIdxInLCU, m_puhChromaIntraDir,   iSizeInUchar );
  memcpy( rpcCU->getInterDir()          + m_uiAbsIdxInLCU, m_puhInterDir,         iSizeInUchar );
  memcpy( rpcCU->getTransformIdx()      + m_uiAbsIdxInLCU, m_puhTrIdx,            iSizeInUchar );
  
  memcpy( rpcCU->getCbf(TEXT_LUMA)     + m_uiAbsIdxInLCU, m_puhCbf[0], iSizeInUchar );
  memcpy( rpcCU->getCbf(TEXT_CHROMA_U) + m_uiAbsIdxInLCU, m_puhCbf[1], iSizeInUchar );
  memcpy( rpcCU->getCbf(TEXT_CHROMA_V) + m_uiAbsIdxInLCU, m_puhCbf[2], iSizeInUchar );
  
  memcpy( rpcCU->getDepth()  + m_uiAbsIdxInLCU, m_puhDepth,  iSizeInUchar );
  memcpy( rpcCU->getWidth()  + m_uiAbsIdxInLCU, m_puhWidth,  iSizeInUchar );
  memcpy( rpcCU->getHeight() + m_uiAbsIdxInLCU, m_puhHeight, iSizeInUchar );
  
  memcpy( rpcCU->getMVPIdx(REF_PIC_LIST_0) + m_uiAbsIdxInLCU, m_apiMVPIdx[0], iSizeInInt );
  memcpy( rpcCU->getMVPIdx(REF_PIC_LIST_1) + m_uiAbsIdxInLCU, m_apiMVPIdx[1], iSizeInInt );
  memcpy( rpcCU->getMVPNum(REF_PIC_LIST_0) + m_uiAbsIdxInLCU, m_apiMVPNum[0], iSizeInInt );
  memcpy( rpcCU->getMVPNum(REF_PIC_LIST_1) + m_uiAbsIdxInLCU, m_apiMVPNum[1], iSizeInInt );
  
  m_acCUMvField[0].copyTo( rpcCU->getCUMvField( REF_PIC_LIST_0 ), m_uiAbsIdxInLCU );
  m_acCUMvField[1].copyTo( rpcCU->getCUMvField( REF_PIC_LIST_1 ), m_uiAbsIdxInLCU );
  
  UInt uiTmp  = (g_uiMaxCUWidth*g_uiMaxCUHeight)>>(uhDepth<<1);
  UInt uiTmp2 = m_uiAbsIdxInLCU*m_pcPic->getMinCUWidth()*m_pcPic->getMinCUHeight();
  memcpy( rpcCU->getCoeffY()  + uiTmp2, m_pcTrCoeffY,  sizeof(TCoeff)*uiTmp  );
  
  uiTmp >>= 2; uiTmp2 >>= 2;
  memcpy( rpcCU->getCoeffCb() + uiTmp2, m_pcTrCoeffCb, sizeof(TCoeff)*uiTmp  );
  memcpy( rpcCU->getCoeffCr() + uiTmp2, m_pcTrCoeffCr, sizeof(TCoeff)*uiTmp  );
  rpcCU->setSliceStartCU( m_uiSliceStartCU );
  rpcCU->setEntropySliceStartCU( m_uiEntropySliceStartCU );

#if HHI_DMM_INTRA
  memcpy( rpcCU->getWedgeFullTabIdx()  + m_uiAbsIdxInLCU, m_puiWedgeFullTabIdx,  iSizeInUInt   );
  memcpy( rpcCU->getWedgeFullDeltaDC1()      + m_uiAbsIdxInLCU, m_piWedgeFullDeltaDC1,       iSizeInInt  );
  memcpy( rpcCU->getWedgeFullDeltaDC2()      + m_uiAbsIdxInLCU, m_piWedgeFullDeltaDC2,       iSizeInInt  );

  memcpy( rpcCU->getWedgePredDirTabIdx()     + m_uiAbsIdxInLCU, m_puiWedgePredDirTabIdx,    iSizeInUInt );
  memcpy( rpcCU->getWedgePredDirDeltaDC1()   + m_uiAbsIdxInLCU, m_piWedgePredDirDeltaDC1,   iSizeInInt  );
  memcpy( rpcCU->getWedgePredDirDeltaDC2()   + m_uiAbsIdxInLCU, m_piWedgePredDirDeltaDC2,   iSizeInInt  );
  memcpy( rpcCU->getWedgePredDirDeltaEnd()   + m_uiAbsIdxInLCU, m_piWedgePredDirDeltaEnd,   iSizeInInt  );

  memcpy( rpcCU->getWedgePredTexTabIdx()     + m_uiAbsIdxInLCU, m_puiWedgePredTexTabIdx,     iSizeInUInt );
  memcpy( rpcCU->getWedgePredTexDeltaDC1()   + m_uiAbsIdxInLCU, m_piWedgePredTexDeltaDC1,    iSizeInInt  );
  memcpy( rpcCU->getWedgePredTexDeltaDC2()   + m_uiAbsIdxInLCU, m_piWedgePredTexDeltaDC2,    iSizeInInt  );

  memcpy( rpcCU->getContourPredTexDeltaDC1() + m_uiAbsIdxInLCU, m_piContourPredTexDeltaDC1,  iSizeInInt  );
  memcpy( rpcCU->getContourPredTexDeltaDC2() + m_uiAbsIdxInLCU, m_piContourPredTexDeltaDC2,  iSizeInInt  );

  memcpy( rpcCU->getTextureModeAllowance()   + m_uiAbsIdxInLCU, m_pbTextureModesAllowed,     iSizeInBool  );
#endif

  memcpy( rpcCU->getTextureModeDepth() + m_uiAbsIdxInLCU, m_piTextureModeDepth, iSizeInInt );
}

Void TComDataCU::copyToPic( UChar uhDepth, UInt uiPartIdx, UInt uiPartDepth )
{
  TComDataCU*&  rpcCU       = m_pcPic->getCU( m_uiCUAddr );
  UInt          uiQNumPart  = m_uiNumPartition>>(uiPartDepth<<1);
  
  UInt uiPartStart          = uiPartIdx*uiQNumPart;
  UInt uiPartOffset         = m_uiAbsIdxInLCU + uiPartStart;
  
  rpcCU->getTotalCost()       = m_dTotalCost;
  rpcCU->getTotalDistortion() = m_uiTotalDistortion;
  rpcCU->getTotalBits()       = m_uiTotalBits;
  
  Int iSizeInUchar  = sizeof( UChar  ) * uiQNumPart;
  Int iSizeInUInt   = sizeof( UInt   ) * uiQNumPart;
  Int iSizeInInt    = sizeof( Int    ) * uiQNumPart;
  Int iSizeInBool   = sizeof( Bool   ) * uiQNumPart;
  
  memcpy( rpcCU->getQP() + uiPartOffset, m_phQP, iSizeInUchar );
  
  memcpy( rpcCU->getPartitionSize()  + uiPartOffset, m_pePartSize, sizeof( PartSize ) * uiQNumPart );
  memcpy( rpcCU->getPredictionMode() + uiPartOffset, m_pePredMode, sizeof( PredMode ) * uiQNumPart );
  
  memcpy( rpcCU->getAlfCtrlFlag()       + uiPartOffset, m_puiAlfCtrlFlag,      iSizeInUInt  );
  memcpy( rpcCU->getMergeFlag()         + uiPartOffset, m_pbMergeFlag,         iSizeInBool  );
  memcpy( rpcCU->getMergeIndex()        + uiPartOffset, m_puhMergeIndex,       iSizeInUchar );
  memcpy( rpcCU->getResPredAvail()      + uiPartOffset, m_pbResPredAvailable,  iSizeInBool  );
  memcpy( rpcCU->getResPredFlag()       + uiPartOffset, m_pbResPredFlag,       iSizeInBool  );
  for( UInt ui = 0; ui < MRG_MAX_NUM_CANDS; ui++ )
  {
    memcpy( rpcCU->getNeighbourCandIdx( ui ) + uiPartOffset, m_apuhNeighbourCandIdx[ui], iSizeInUchar );
  }
  memcpy( rpcCU->getLumaIntraDir()      + uiPartOffset, m_puhLumaIntraDir,     iSizeInUchar );
  memcpy( rpcCU->getChromaIntraDir()    + uiPartOffset, m_puhChromaIntraDir,   iSizeInUchar );
  memcpy( rpcCU->getInterDir()          + uiPartOffset, m_puhInterDir,         iSizeInUchar );
  memcpy( rpcCU->getTransformIdx()      + uiPartOffset, m_puhTrIdx,            iSizeInUchar );
  
  memcpy( rpcCU->getCbf(TEXT_LUMA)     + uiPartOffset, m_puhCbf[0], iSizeInUchar );
  memcpy( rpcCU->getCbf(TEXT_CHROMA_U) + uiPartOffset, m_puhCbf[1], iSizeInUchar );
  memcpy( rpcCU->getCbf(TEXT_CHROMA_V) + uiPartOffset, m_puhCbf[2], iSizeInUchar );
  
  memcpy( rpcCU->getDepth()  + uiPartOffset, m_puhDepth,  iSizeInUchar );
  memcpy( rpcCU->getWidth()  + uiPartOffset, m_puhWidth,  iSizeInUchar );
  memcpy( rpcCU->getHeight() + uiPartOffset, m_puhHeight, iSizeInUchar );
  
  memcpy( rpcCU->getMVPIdx(REF_PIC_LIST_0) + uiPartOffset, m_apiMVPIdx[0], iSizeInInt );
  memcpy( rpcCU->getMVPIdx(REF_PIC_LIST_1) + uiPartOffset, m_apiMVPIdx[1], iSizeInInt );
  memcpy( rpcCU->getMVPNum(REF_PIC_LIST_0) + uiPartOffset, m_apiMVPNum[0], iSizeInInt );
  memcpy( rpcCU->getMVPNum(REF_PIC_LIST_1) + uiPartOffset, m_apiMVPNum[1], iSizeInInt );
  m_acCUMvField[0].copyTo( rpcCU->getCUMvField( REF_PIC_LIST_0 ), m_uiAbsIdxInLCU, uiPartStart, uiQNumPart );
  m_acCUMvField[1].copyTo( rpcCU->getCUMvField( REF_PIC_LIST_1 ), m_uiAbsIdxInLCU, uiPartStart, uiQNumPart );
  
  UInt uiTmp  = (g_uiMaxCUWidth*g_uiMaxCUHeight)>>((uhDepth+uiPartDepth)<<1);
  UInt uiTmp2 = uiPartOffset*m_pcPic->getMinCUWidth()*m_pcPic->getMinCUHeight();
  memcpy( rpcCU->getCoeffY()  + uiTmp2, m_pcTrCoeffY,  sizeof(TCoeff)*uiTmp  );
  
  uiTmp >>= 2; uiTmp2 >>= 2;
  memcpy( rpcCU->getCoeffCb() + uiTmp2, m_pcTrCoeffCb, sizeof(TCoeff)*uiTmp  );
  memcpy( rpcCU->getCoeffCr() + uiTmp2, m_pcTrCoeffCr, sizeof(TCoeff)*uiTmp  );
  rpcCU->setSliceStartCU( m_uiSliceStartCU );
  rpcCU->setEntropySliceStartCU( m_uiEntropySliceStartCU );

#if HHI_DMM_INTRA
  memcpy( rpcCU->getWedgeFullTabIdx()  + uiPartOffset, m_puiWedgeFullTabIdx,  iSizeInUInt   );
  memcpy( rpcCU->getWedgeFullDeltaDC1()      + uiPartOffset, m_piWedgeFullDeltaDC1,       iSizeInInt  );
  memcpy( rpcCU->getWedgeFullDeltaDC2()      + uiPartOffset, m_piWedgeFullDeltaDC2,       iSizeInInt  );

  memcpy( rpcCU->getWedgePredDirTabIdx()     + uiPartOffset, m_puiWedgePredDirTabIdx,    iSizeInUInt );
  memcpy( rpcCU->getWedgePredDirDeltaDC1()   + uiPartOffset, m_piWedgePredDirDeltaDC1,   iSizeInInt  );
  memcpy( rpcCU->getWedgePredDirDeltaDC2()   + uiPartOffset, m_piWedgePredDirDeltaDC2,   iSizeInInt  );
  memcpy( rpcCU->getWedgePredDirDeltaEnd()   + uiPartOffset, m_piWedgePredDirDeltaEnd,   iSizeInInt  );

  memcpy( rpcCU->getWedgePredTexTabIdx()     + uiPartOffset, m_puiWedgePredTexTabIdx,     iSizeInUInt );
  memcpy( rpcCU->getWedgePredTexDeltaDC1()   + uiPartOffset, m_piWedgePredTexDeltaDC1,    iSizeInInt  );
  memcpy( rpcCU->getWedgePredTexDeltaDC2()   + uiPartOffset, m_piWedgePredTexDeltaDC2,    iSizeInInt  );

  memcpy( rpcCU->getContourPredTexDeltaDC1() + uiPartOffset, m_piContourPredTexDeltaDC1,  iSizeInInt  );
  memcpy( rpcCU->getContourPredTexDeltaDC2() + uiPartOffset, m_piContourPredTexDeltaDC2,  iSizeInInt  );

  memcpy( rpcCU->getTextureModeAllowance()   + uiPartOffset, m_pbTextureModesAllowed,     iSizeInBool  );
#endif

  memcpy( rpcCU->getTextureModeDepth() + uiPartOffset, m_piTextureModeDepth, iSizeInInt );
}

// --------------------------------------------------------------------------------------------------------------------
// Other public functions
// --------------------------------------------------------------------------------------------------------------------

TComDataCU* TComDataCU::getPULeft( UInt& uiLPartUnitIdx, UInt uiCurrPartUnitIdx, Bool bEnforceSliceRestriction, Bool bEnforceEntropySliceRestriction )
{
  UInt uiAbsPartIdx       = g_auiZscanToRaster[uiCurrPartUnitIdx];
  UInt uiAbsZorderCUIdx   = g_auiZscanToRaster[m_uiAbsIdxInLCU];
  UInt uiNumPartInCUWidth = m_pcPic->getNumPartInWidth();
  
  if( uiAbsPartIdx % uiNumPartInCUWidth )
  {
    uiLPartUnitIdx = g_auiRasterToZscan[ uiAbsPartIdx - 1 ];
    if ( uiAbsPartIdx % uiNumPartInCUWidth == uiAbsZorderCUIdx % uiNumPartInCUWidth )
    {
      return m_pcPic->getCU( getAddr() );
    }
    else
    {
      uiLPartUnitIdx -= m_uiAbsIdxInLCU;
      return this;
    }
  }
  
  uiLPartUnitIdx = g_auiRasterToZscan[ uiAbsPartIdx + uiNumPartInCUWidth - 1 ];
  if ( (bEnforceSliceRestriction && (m_pcCULeft==NULL || m_pcCULeft->getSlice()==NULL || m_pcCULeft->getAddr() < m_uiSliceStartCU)) ||
       (bEnforceEntropySliceRestriction && (m_pcCULeft==NULL || m_pcCULeft->getSlice()==NULL || m_pcCULeft->getAddr() < m_uiEntropySliceStartCU)) )
  {
    return NULL;
  }
  return m_pcCULeft;
}

TComDataCU* TComDataCU::getPUAbove( UInt& uiAPartUnitIdx, UInt uiCurrPartUnitIdx, Bool bEnforceSliceRestriction, Bool bEnforceEntropySliceRestriction )
{
  UInt uiAbsPartIdx       = g_auiZscanToRaster[uiCurrPartUnitIdx];
  UInt uiAbsZorderCUIdx   = g_auiZscanToRaster[m_uiAbsIdxInLCU];
  UInt uiNumPartInCUWidth = m_pcPic->getNumPartInWidth();
  
  if( uiAbsPartIdx / uiNumPartInCUWidth )
  {
    uiAPartUnitIdx = g_auiRasterToZscan[ uiAbsPartIdx - uiNumPartInCUWidth ];
    if ( uiAbsPartIdx / uiNumPartInCUWidth == uiAbsZorderCUIdx / uiNumPartInCUWidth )
    {
      return m_pcPic->getCU( getAddr() );
    }
    else
    {
      uiAPartUnitIdx -= m_uiAbsIdxInLCU;
      return this;
    }
  }
  
  uiAPartUnitIdx = g_auiRasterToZscan[ uiAbsPartIdx + m_pcPic->getNumPartInCU() - uiNumPartInCUWidth ];
  if ( (bEnforceSliceRestriction && (m_pcCUAbove==NULL || m_pcCUAbove->getSlice()==NULL || m_pcCUAbove->getAddr() < m_uiSliceStartCU)) ||
       (bEnforceEntropySliceRestriction && (m_pcCUAbove==NULL || m_pcCUAbove->getSlice()==NULL || m_pcCUAbove->getAddr() < m_uiEntropySliceStartCU)) )
  {
    return NULL;
  }
  return m_pcCUAbove;
}

TComDataCU* TComDataCU::getPUAboveLeft( UInt& uiALPartUnitIdx, UInt uiCurrPartUnitIdx, Bool bEnforceSliceRestriction, Bool bEnforceEntropySliceRestriction )
{
  UInt uiAbsPartIdx       = g_auiZscanToRaster[uiCurrPartUnitIdx];
  UInt uiAbsZorderCUIdx   = g_auiZscanToRaster[m_uiAbsIdxInLCU];
  UInt uiNumPartInCUWidth = m_pcPic->getNumPartInWidth();
  
  if( uiAbsPartIdx % uiNumPartInCUWidth )
  {
    if( uiAbsPartIdx / uiNumPartInCUWidth )
    {
      uiALPartUnitIdx = g_auiRasterToZscan[ uiAbsPartIdx - uiNumPartInCUWidth - 1 ];
      if ( ( uiAbsPartIdx % uiNumPartInCUWidth == uiAbsZorderCUIdx % uiNumPartInCUWidth ) || ( uiAbsPartIdx / uiNumPartInCUWidth == uiAbsZorderCUIdx / uiNumPartInCUWidth ) )
      {
        return m_pcPic->getCU( getAddr() );
      }
      else
      {
        uiALPartUnitIdx -= m_uiAbsIdxInLCU;
        return this;
      }
    }
    uiALPartUnitIdx = g_auiRasterToZscan[ uiAbsPartIdx + getPic()->getNumPartInCU() - uiNumPartInCUWidth - 1 ];
    if ( (bEnforceSliceRestriction && (m_pcCUAbove==NULL || m_pcCUAbove->getSlice()==NULL || m_pcCUAbove->getAddr() < m_uiSliceStartCU)) ||
         (bEnforceEntropySliceRestriction && (m_pcCUAbove==NULL || m_pcCUAbove->getSlice()==NULL || m_pcCUAbove->getAddr() < m_uiEntropySliceStartCU)) )
    {
      return NULL;
    }
    return m_pcCUAbove;
  }
  
  if( uiAbsPartIdx / uiNumPartInCUWidth )
  {
    uiALPartUnitIdx = g_auiRasterToZscan[ uiAbsPartIdx - 1 ];
    if ( (bEnforceSliceRestriction && (m_pcCULeft==NULL || m_pcCULeft->getSlice()==NULL || m_pcCULeft->getAddr() < m_uiSliceStartCU)) ||
         (bEnforceEntropySliceRestriction && (m_pcCULeft==NULL || m_pcCULeft->getSlice()==NULL || m_pcCULeft->getAddr() < m_uiEntropySliceStartCU)) )
    {
      return NULL;
    }
    return m_pcCULeft;
  }
  
  uiALPartUnitIdx = g_auiRasterToZscan[ m_pcPic->getNumPartInCU() - 1 ];
  if ( (bEnforceSliceRestriction && (m_pcCUAboveLeft==NULL || m_pcCUAboveLeft->getSlice()==NULL || m_pcCUAboveLeft->getAddr() < m_uiSliceStartCU)) ||
       (bEnforceEntropySliceRestriction && (m_pcCUAboveLeft==NULL || m_pcCUAboveLeft->getSlice()==NULL|| m_pcCUAboveLeft->getAddr() < m_uiEntropySliceStartCU)) )
  {
    return NULL;
  }
  return m_pcCUAboveLeft;
}

TComDataCU* TComDataCU::getPUAboveRight( UInt& uiARPartUnitIdx, UInt uiCurrPartUnitIdx, Bool bEnforceSliceRestriction, Bool bEnforceEntropySliceRestriction )
{
  UInt uiAbsPartIdxRT     = g_auiZscanToRaster[uiCurrPartUnitIdx];
  UInt uiAbsZorderCUIdx   = g_auiZscanToRaster[ m_uiAbsIdxInLCU ] + m_puhWidth[0] / m_pcPic->getMinCUWidth() - 1;
  UInt uiNumPartInCUWidth = m_pcPic->getNumPartInWidth();
  
  if( ( m_pcPic->getCU(m_uiCUAddr)->getCUPelX() + g_auiRasterToPelX[uiAbsPartIdxRT] + m_pcPic->getMinCUWidth() ) >= m_pcSlice->getSPS()->getWidth() )
  {
    uiARPartUnitIdx = MAX_UINT;
    return NULL;
  }
  
  if ( uiAbsPartIdxRT % uiNumPartInCUWidth < uiNumPartInCUWidth - 1 )
  {
    if ( uiAbsPartIdxRT / uiNumPartInCUWidth )
    {
      if ( uiCurrPartUnitIdx > g_auiRasterToZscan[ uiAbsPartIdxRT - uiNumPartInCUWidth + 1 ] )
      {
        uiARPartUnitIdx = g_auiRasterToZscan[ uiAbsPartIdxRT - uiNumPartInCUWidth + 1 ];
        if ( ( uiAbsPartIdxRT % uiNumPartInCUWidth == uiAbsZorderCUIdx % uiNumPartInCUWidth ) || ( uiAbsPartIdxRT / uiNumPartInCUWidth == uiAbsZorderCUIdx / uiNumPartInCUWidth ) )
        {
          return m_pcPic->getCU( getAddr() );
        }
        else
        {
          uiARPartUnitIdx -= m_uiAbsIdxInLCU;
          return this;
        }
      }
      uiARPartUnitIdx = MAX_UINT;
      return NULL;
    }
    uiARPartUnitIdx = g_auiRasterToZscan[ uiAbsPartIdxRT + m_pcPic->getNumPartInCU() - uiNumPartInCUWidth + 1 ];
    if ( (bEnforceSliceRestriction && (m_pcCUAbove==NULL || m_pcCUAbove->getSlice()==NULL || m_pcCUAbove->getAddr() < m_uiSliceStartCU)) ||
         (bEnforceEntropySliceRestriction && (m_pcCUAbove==NULL || m_pcCUAbove->getSlice()==NULL || m_pcCUAbove->getAddr() < m_uiEntropySliceStartCU)) )
    {
      return NULL;
    }
    return m_pcCUAbove;
  }
  
  if ( uiAbsPartIdxRT / uiNumPartInCUWidth )
  {
    uiARPartUnitIdx = MAX_UINT;
    return NULL;
  }
  
  uiARPartUnitIdx = g_auiRasterToZscan[ m_pcPic->getNumPartInCU() - uiNumPartInCUWidth ];
  if ( (bEnforceSliceRestriction && (m_pcCUAboveRight==NULL || m_pcCUAboveRight->getSlice()==NULL || m_pcCUAboveRight->getAddr() < m_uiSliceStartCU)) ||
       (bEnforceEntropySliceRestriction && (m_pcCUAboveRight==NULL || m_pcCUAboveRight->getSlice()==NULL || m_pcCUAboveRight->getAddr() < m_uiEntropySliceStartCU)) )
  {
    return NULL;
  }
  return m_pcCUAboveRight;
}

TComDataCU* TComDataCU::getPUBelowLeft( UInt& uiBLPartUnitIdx, UInt uiCurrPartUnitIdx, Bool bEnforceSliceRestriction, Bool bEnforceEntropySliceRestriction )
{
  UInt uiAbsPartIdxLB     = g_auiZscanToRaster[uiCurrPartUnitIdx];
  UInt uiAbsZorderCUIdxLB = g_auiZscanToRaster[ m_uiAbsIdxInLCU ] + (m_puhHeight[0] / m_pcPic->getMinCUHeight() - 1)*m_pcPic->getNumPartInWidth();
  UInt uiNumPartInCUWidth = m_pcPic->getNumPartInWidth();
  
  if( ( m_pcPic->getCU(m_uiCUAddr)->getCUPelY() + g_auiRasterToPelY[uiAbsPartIdxLB] + m_pcPic->getMinCUHeight() ) >= m_pcSlice->getSPS()->getHeight() )
  {
    uiBLPartUnitIdx = MAX_UINT;
    return NULL;
  }
  
  if ( uiAbsPartIdxLB / uiNumPartInCUWidth < m_pcPic->getNumPartInHeight() - 1 )
  {
    if ( uiAbsPartIdxLB % uiNumPartInCUWidth )
    {
      if ( uiCurrPartUnitIdx > g_auiRasterToZscan[ uiAbsPartIdxLB + uiNumPartInCUWidth - 1 ] )
      {
        uiBLPartUnitIdx = g_auiRasterToZscan[ uiAbsPartIdxLB + uiNumPartInCUWidth - 1 ];
        if ( ( (uiAbsPartIdxLB % uiNumPartInCUWidth) == (uiAbsZorderCUIdxLB % uiNumPartInCUWidth) ) || ( (uiAbsPartIdxLB / uiNumPartInCUWidth) == (uiAbsZorderCUIdxLB / uiNumPartInCUWidth) ) )
        {
          return m_pcPic->getCU( getAddr() );
        }
        else
        {
          uiBLPartUnitIdx -= m_uiAbsIdxInLCU;
          return this;
        }
      }
      uiBLPartUnitIdx = MAX_UINT;
      return NULL;
    }
    uiBLPartUnitIdx = g_auiRasterToZscan[ uiAbsPartIdxLB + uiNumPartInCUWidth*2 - 1 ];
    if ( (bEnforceSliceRestriction && (m_pcCULeft==NULL || m_pcCULeft->getSlice()==NULL || m_pcCULeft->getAddr() < m_uiSliceStartCU)) ||
         (bEnforceEntropySliceRestriction && (m_pcCULeft==NULL || m_pcCULeft->getSlice()==NULL || m_pcCULeft->getAddr() < m_uiEntropySliceStartCU)) )
    {
      return NULL;
    }
    return m_pcCULeft;
  }
  
  uiBLPartUnitIdx = MAX_UINT;
  return NULL;
}

#if CONSTRAINED_INTRA_PRED
TComDataCU* TComDataCU::getPUBelowLeftAdi(UInt& uiBLPartUnitIdx, UInt uiPuHeight,  UInt uiCurrPartUnitIdx, UInt uiPartUnitOffset, Bool bEnforceSliceRestriction, Bool bEnforceEntropySliceRestriction )
#else //CONSTRAINED_INTRA_PRED
TComDataCU* TComDataCU::getPUBelowLeftAdi(UInt& uiBLPartUnitIdx, UInt uiPuHeight,  UInt uiCurrPartUnitIdx, Bool bEnforceSliceRestriction, Bool bEnforceEntropySliceRestriction )
#endif //CONSTRAINED_INTRA_PRED
{
  UInt uiAbsPartIdxLB     = g_auiZscanToRaster[uiCurrPartUnitIdx];
  UInt uiAbsZorderCUIdxLB = g_auiZscanToRaster[ m_uiAbsIdxInLCU ] + ((m_puhHeight[0] / m_pcPic->getMinCUHeight()) - 1)*m_pcPic->getNumPartInWidth();
  UInt uiNumPartInCUWidth = m_pcPic->getNumPartInWidth();
  
  if( ( m_pcPic->getCU(m_uiCUAddr)->getCUPelY() + g_auiRasterToPelY[uiAbsPartIdxLB] + uiPuHeight ) >= m_pcSlice->getSPS()->getHeight() )
  {
    uiBLPartUnitIdx = MAX_UINT;
    return NULL;
  }
  
#if CONSTRAINED_INTRA_PRED
  if ( uiAbsPartIdxLB / uiNumPartInCUWidth < m_pcPic->getNumPartInHeight() - uiPartUnitOffset )
#else
  if ( uiAbsPartIdxLB / uiNumPartInCUWidth < m_pcPic->getNumPartInHeight() - 1 )
#endif
  {
    if ( uiAbsPartIdxLB % uiNumPartInCUWidth )
    {
#if CONSTRAINED_INTRA_PRED
      if ( uiCurrPartUnitIdx > g_auiRasterToZscan[ uiAbsPartIdxLB + uiPartUnitOffset * uiNumPartInCUWidth - 1 ] )
#else
      if ( uiCurrPartUnitIdx > g_auiRasterToZscan[ uiAbsPartIdxLB + uiNumPartInCUWidth - 1 ] )
#endif
      {
#if CONSTRAINED_INTRA_PRED
        uiBLPartUnitIdx = g_auiRasterToZscan[ uiAbsPartIdxLB + uiPartUnitOffset * uiNumPartInCUWidth - 1 ];
#else
        uiBLPartUnitIdx = g_auiRasterToZscan[ uiAbsPartIdxLB + uiNumPartInCUWidth - 1 ];
#endif
        if ( ( (uiAbsPartIdxLB % uiNumPartInCUWidth) == (uiAbsZorderCUIdxLB % uiNumPartInCUWidth) ) || ( (uiAbsPartIdxLB / uiNumPartInCUWidth) == (uiAbsZorderCUIdxLB / uiNumPartInCUWidth) ) )
        {
          return m_pcPic->getCU( getAddr() );
        }
        else
        {
          uiBLPartUnitIdx -= m_uiAbsIdxInLCU;
          return this;
        }
      }
      uiBLPartUnitIdx = MAX_UINT;
      return NULL;
    }
#if CONSTRAINED_INTRA_PRED
    uiBLPartUnitIdx = g_auiRasterToZscan[ uiAbsPartIdxLB + (1+uiPartUnitOffset) * uiNumPartInCUWidth - 1 ];
#else
    uiBLPartUnitIdx = g_auiRasterToZscan[ uiAbsPartIdxLB + uiNumPartInCUWidth*2 - 1 ];
#endif
    if ( (bEnforceSliceRestriction && (m_pcCULeft==NULL || m_pcCULeft->getSlice()==NULL || m_pcCULeft->getAddr() < m_uiSliceStartCU)) ||
         (bEnforceEntropySliceRestriction && (m_pcCULeft==NULL || m_pcCULeft->getSlice()==NULL || m_pcCULeft->getAddr() < m_uiEntropySliceStartCU)) )
    {
      return NULL;
    }
    return m_pcCULeft;
  }
  
  uiBLPartUnitIdx = MAX_UINT;
  return NULL;
}

#if CONSTRAINED_INTRA_PRED
TComDataCU* TComDataCU::getPUAboveRightAdi(UInt&  uiARPartUnitIdx, UInt uiPuWidth, UInt uiCurrPartUnitIdx, UInt uiPartUnitOffset, Bool bEnforceSliceRestriction, Bool bEnforceEntropySliceRestriction )
#else //CONSTRAINED_INTRA_PRED
TComDataCU* TComDataCU::getPUAboveRightAdi(UInt&  uiARPartUnitIdx, UInt uiPuWidth, UInt uiCurrPartUnitIdx, Bool bEnforceSliceRestriction, Bool bEnforceEntropySliceRestriction )
#endif //CONSTRAINED_INTRA_PRED
{
  UInt uiAbsPartIdxRT     = g_auiZscanToRaster[uiCurrPartUnitIdx];
  UInt uiAbsZorderCUIdx   = g_auiZscanToRaster[ m_uiAbsIdxInLCU ] + (m_puhWidth[0] / m_pcPic->getMinCUWidth()) - 1;
  UInt uiNumPartInCUWidth = m_pcPic->getNumPartInWidth();
  
  if( ( m_pcPic->getCU(m_uiCUAddr)->getCUPelX() + g_auiRasterToPelX[uiAbsPartIdxRT] + uiPuWidth ) >= m_pcSlice->getSPS()->getWidth() )
  {
    uiARPartUnitIdx = MAX_UINT;
    return NULL;
  }
  
#if CONSTRAINED_INTRA_PRED
  if ( uiAbsPartIdxRT % uiNumPartInCUWidth < uiNumPartInCUWidth - uiPartUnitOffset )
#else
  if ( uiAbsPartIdxRT % uiNumPartInCUWidth < uiNumPartInCUWidth - 1 )
#endif
  {
    if ( uiAbsPartIdxRT / uiNumPartInCUWidth )
    {
#if CONSTRAINED_INTRA_PRED
      if ( uiCurrPartUnitIdx > g_auiRasterToZscan[ uiAbsPartIdxRT - uiNumPartInCUWidth + uiPartUnitOffset ] )
#else
      if ( uiCurrPartUnitIdx > g_auiRasterToZscan[ uiAbsPartIdxRT - uiNumPartInCUWidth + 1 ] )
#endif
      {
#if CONSTRAINED_INTRA_PRED
        uiARPartUnitIdx = g_auiRasterToZscan[ uiAbsPartIdxRT - uiNumPartInCUWidth + uiPartUnitOffset ];
#else
        uiARPartUnitIdx = g_auiRasterToZscan[ uiAbsPartIdxRT - uiNumPartInCUWidth + 1 ];
#endif
        if ( ( uiAbsPartIdxRT % uiNumPartInCUWidth == uiAbsZorderCUIdx % uiNumPartInCUWidth ) || ( uiAbsPartIdxRT / uiNumPartInCUWidth == uiAbsZorderCUIdx / uiNumPartInCUWidth ) )
        {
          return m_pcPic->getCU( getAddr() );
        }
        else
        {
          uiARPartUnitIdx -= m_uiAbsIdxInLCU;
          return this;
        }
      }
      uiARPartUnitIdx = MAX_UINT;
      return NULL;
    }
#if CONSTRAINED_INTRA_PRED
    uiARPartUnitIdx = g_auiRasterToZscan[ uiAbsPartIdxRT + m_pcPic->getNumPartInCU() - uiNumPartInCUWidth + uiPartUnitOffset ];
#else
    uiARPartUnitIdx = g_auiRasterToZscan[ uiAbsPartIdxRT + m_pcPic->getNumPartInCU() - uiNumPartInCUWidth + 1 ];
#endif
    if ( (bEnforceSliceRestriction && (m_pcCUAbove==NULL || m_pcCUAbove->getSlice()==NULL || m_pcCUAbove->getAddr() < m_uiSliceStartCU)) ||
         (bEnforceEntropySliceRestriction && (m_pcCUAbove==NULL || m_pcCUAbove->getSlice()==NULL || m_pcCUAbove->getAddr() < m_uiEntropySliceStartCU)) )
    {
      return NULL;
    }
    return m_pcCUAbove;
  }
  
  if ( uiAbsPartIdxRT / uiNumPartInCUWidth )
  {
    uiARPartUnitIdx = MAX_UINT;
    return NULL;
  }
  
#if CONSTRAINED_INTRA_PRED
  uiARPartUnitIdx = g_auiRasterToZscan[ m_pcPic->getNumPartInCU() - uiNumPartInCUWidth + uiPartUnitOffset-1 ];
#else
  uiARPartUnitIdx = g_auiRasterToZscan[ m_pcPic->getNumPartInCU() - uiNumPartInCUWidth ];
#endif
  if ( (bEnforceSliceRestriction && (m_pcCUAboveRight==NULL || m_pcCUAboveRight->getSlice()==NULL || m_pcCUAboveRight->getAddr() < m_uiSliceStartCU)) ||
       (bEnforceEntropySliceRestriction && (m_pcCUAboveRight==NULL || m_pcCUAboveRight->getSlice()==NULL || m_pcCUAboveRight->getAddr() < m_uiEntropySliceStartCU)) )
  {
    return NULL;
  }
  return m_pcCUAboveRight;
}


#if HHI_DMM_INTRA
Void TComDataCU::setWedgeFullTabIdxSubParts( UInt uiTIdx, UInt uiAbsPartIdx, UInt uiDepth )
{
  UInt uiCurrPartNumb = m_pcPic->getNumPartInCU() >> (uiDepth << 1);

  for ( UInt ui = 0; ui < uiCurrPartNumb; ui++ )
  {
    m_puiWedgeFullTabIdx[uiAbsPartIdx+ui] = uiTIdx;
  }
}

Void TComDataCU::setWedgeFullDeltaDC1SubParts( Int iDC1, UInt uiAbsPartIdx, UInt uiDepth )
{
  UInt uiCurrPartNumb = m_pcPic->getNumPartInCU() >> (uiDepth << 1);

  for ( UInt ui = 0; ui < uiCurrPartNumb; ui++ )
  {
    m_piWedgeFullDeltaDC1[uiAbsPartIdx+ui] = iDC1;
  }
}

Void TComDataCU::setWedgeFullDeltaDC2SubParts( Int iDC2, UInt uiAbsPartIdx, UInt uiDepth )
{
  UInt uiCurrPartNumb = m_pcPic->getNumPartInCU() >> (uiDepth << 1);

  for ( UInt ui = 0; ui < uiCurrPartNumb; ui++ )
  {
    m_piWedgeFullDeltaDC2[uiAbsPartIdx+ui] = iDC2;
  }
}

Void TComDataCU::setWedgePredDirTabIdxSubParts( UInt uiTIdx, UInt uiAbsPartIdx, UInt uiDepth )
{
  UInt uiCurrPartNumb = m_pcPic->getNumPartInCU() >> (uiDepth << 1);

  for ( UInt ui = 0; ui < uiCurrPartNumb; ui++ )
  {
    m_puiWedgePredDirTabIdx[uiAbsPartIdx+ui] = uiTIdx;
  }
}

Void TComDataCU::setWedgePredDirDeltaDC1SubParts( Int iDC1, UInt uiAbsPartIdx, UInt uiDepth )
{
  UInt uiCurrPartNumb = m_pcPic->getNumPartInCU() >> (uiDepth << 1);

  for ( UInt ui = 0; ui < uiCurrPartNumb; ui++ )
  {
    m_piWedgePredDirDeltaDC1[uiAbsPartIdx+ui] = iDC1;
  }
}

Void TComDataCU::setWedgePredDirDeltaDC2SubParts( Int iDC2, UInt uiAbsPartIdx, UInt uiDepth )
{
  UInt uiCurrPartNumb = m_pcPic->getNumPartInCU() >> (uiDepth << 1);

  for ( UInt ui = 0; ui < uiCurrPartNumb; ui++ )
  {
    m_piWedgePredDirDeltaDC2[uiAbsPartIdx+ui] = iDC2;
  }
}

Void TComDataCU::setWedgePredDirDeltaEndSubParts( Int iDelta, UInt uiAbsPartIdx, UInt uiDepth )
{
  UInt uiCurrPartNumb = m_pcPic->getNumPartInCU() >> (uiDepth << 1);

  for ( UInt ui = 0; ui < uiCurrPartNumb; ui++ )
  {
    m_piWedgePredDirDeltaEnd[uiAbsPartIdx+ui] = iDelta;
  }
}

Void TComDataCU::setWedgePredTexTabIdxSubParts( UInt uiTIdx, UInt uiAbsPartIdx, UInt uiDepth )
{
  UInt uiCurrPartNumb = m_pcPic->getNumPartInCU() >> (uiDepth << 1);

  for ( UInt ui = 0; ui < uiCurrPartNumb; ui++ )
  {
    m_puiWedgePredTexTabIdx[uiAbsPartIdx+ui] = uiTIdx;
  }
}

Void TComDataCU::setWedgePredTexDeltaDC1SubParts( Int iDC1, UInt uiAbsPartIdx, UInt uiDepth )
{
  UInt uiCurrPartNumb = m_pcPic->getNumPartInCU() >> (uiDepth << 1);

  for ( UInt ui = 0; ui < uiCurrPartNumb; ui++ )
  {
    m_piWedgePredTexDeltaDC1[uiAbsPartIdx+ui] = iDC1;
  }
}

Void TComDataCU::setWedgePredTexDeltaDC2SubParts( Int iDC2, UInt uiAbsPartIdx, UInt uiDepth )
{
  UInt uiCurrPartNumb = m_pcPic->getNumPartInCU() >> (uiDepth << 1);

  for ( UInt ui = 0; ui < uiCurrPartNumb; ui++ )
  {
    m_piWedgePredTexDeltaDC2[uiAbsPartIdx+ui] = iDC2;
  }
}

Void TComDataCU::setContourPredTexDeltaDC1SubParts( Int iDC1, UInt uiAbsPartIdx, UInt uiDepth )
{
  UInt uiCurrPartNumb = m_pcPic->getNumPartInCU() >> (uiDepth << 1);

  for ( UInt ui = 0; ui < uiCurrPartNumb; ui++ )
  {
    m_piContourPredTexDeltaDC1[uiAbsPartIdx+ui] = iDC1;
  }
}

Void TComDataCU::setContourPredTexDeltaDC2SubParts( Int iDC2, UInt uiAbsPartIdx, UInt uiDepth )
{
  UInt uiCurrPartNumb = m_pcPic->getNumPartInCU() >> (uiDepth << 1);

  for ( UInt ui = 0; ui < uiCurrPartNumb; ui++ )
  {
    m_piContourPredTexDeltaDC2[uiAbsPartIdx+ui] = iDC2;
  }
}

Void TComDataCU::setTextureModeAllowanceSubParts ( Bool bTMAllowed, UInt uiAbsPartIdx, UInt uiDepth )
{
  UInt uiCurrPartNumb = m_pcPic->getNumPartInCU() >> (uiDepth << 1);

  memset( m_pbTextureModesAllowed + uiAbsPartIdx, bTMAllowed, sizeof(Bool)*uiCurrPartNumb );
}
#endif

Int
TComDataCU::getPdmMergeCandidate( UInt uiPartIdx, Int* paiPdmRefIdx, TComMv* pacPdmMv )
{
  TComDepthMapGenerator* pcDepthMapGenerator = m_pcSlice->getSPS()->getDepthMapGenerator();
  ROFRS( pcDepthMapGenerator, 0 );
  return pcDepthMapGenerator->getPdmMergeCandidate( this, uiPartIdx, paiPdmRefIdx, pacPdmMv );
}


Bool
TComDataCU::getPdmMvPred( UInt uiPartIdx, RefPicList eRefPicList, Int iRefIdx, TComMv& rcMv, Bool bMerge )
{
  TComDepthMapGenerator* pcDepthMapGenerator = m_pcSlice->getSPS()->getDepthMapGenerator();
  ROFRS( pcDepthMapGenerator, false );
  return pcDepthMapGenerator->getPdmMvPred( this, uiPartIdx, eRefPicList, iRefIdx, rcMv, bMerge );
}


Bool      
TComDataCU::getIViewOrgDepthMvPred( UInt uiPartIdx, RefPicList eRefPicList, Int iRefIdx, TComMv& rcMv )
{
  TComDepthMapGenerator* pcDepthMapGenerator = m_pcSlice->getSPS()->getDepthMapGenerator();
  ROFRS( pcDepthMapGenerator, false );
  return pcDepthMapGenerator->getIViewOrgDepthMvPred( this, uiPartIdx, eRefPicList, iRefIdx, rcMv );
}

Bool
TComDataCU::getResidualSamples( UInt uiPartIdx, TComYuv* pcYuv )
{
  TComResidualGenerator*  pcResidualGenerator = m_pcSlice->getSPS()->getResidualGenerator();
  ROFRS( pcResidualGenerator, false );
  return pcResidualGenerator->getResidualSamples( this, uiPartIdx, pcYuv );
}


Int TComDataCU::getMostProbableIntraDirLuma( UInt uiAbsPartIdx )
{
  TComDataCU* pcTempCU;
  UInt        uiTempPartIdx;
  Int         iLeftIntraDir, iAboveIntraDir, iMostProbable;
  
  // Get intra direction of left PU
  pcTempCU = getPULeft( uiTempPartIdx, m_uiAbsIdxInLCU + uiAbsPartIdx, true, false );
  iLeftIntraDir  = pcTempCU ? ( pcTempCU->isIntra( uiTempPartIdx ) ? pcTempCU->getLumaIntraDir( uiTempPartIdx ) : 2 ) : NOT_VALID;
#if ADD_PLANAR_MODE
  mapPlanartoDC( iLeftIntraDir );
#endif
#if HHI_DMM_INTRA
  mapDMMtoDC( iLeftIntraDir );
#endif
  
  // Get intra direction of above PU
  pcTempCU = getPUAbove( uiTempPartIdx, m_uiAbsIdxInLCU + uiAbsPartIdx, true, false );
  iAboveIntraDir = pcTempCU ? ( pcTempCU->isIntra( uiTempPartIdx ) ? pcTempCU->getLumaIntraDir( uiTempPartIdx ) : 2 ) : NOT_VALID;
#if ADD_PLANAR_MODE
  mapPlanartoDC( iAboveIntraDir );
#endif
#if HHI_DMM_INTRA
  mapDMMtoDC( iAboveIntraDir );
#endif
  
  iMostProbable  = Min( iLeftIntraDir, iAboveIntraDir );
  
  // Mode conversion process for blocks with different number of available prediction directions
  Int iIdx  = getIntraSizeIdx(uiAbsPartIdx);
  
  if ( iMostProbable >= g_aucIntraModeNumAng[iIdx] )
  {
    if ( g_aucIntraModeNumAng[iIdx] == 5 )
      iMostProbable = g_aucAngModeMapping[0][g_aucAngIntraModeOrder[iMostProbable]];
    if ( g_aucIntraModeNumAng[iIdx] == 3 )
      iMostProbable = g_aucAngModeMapping[3][g_aucAngIntraModeOrder[iMostProbable]];
    else
      iMostProbable = g_aucAngModeMapping[1][g_aucAngIntraModeOrder[iMostProbable]]; 
  } 
  
  return ( NOT_VALID == iMostProbable ) ? 2 : iMostProbable;
}
#if MTK_DCM_MPM
Int TComDataCU::getIntraDirLumaPredictor( UInt uiAbsPartIdx, Int uiIntraDirPred[] )
{
  TComDataCU* pcTempCU;
  UInt        uiTempPartIdx;
  Int         iLeftIntraDir, iAboveIntraDir;
  Int         uiPredNum = 0;

  // Get intra direction of left PU
  pcTempCU = getPULeft( uiTempPartIdx, m_uiAbsIdxInLCU + uiAbsPartIdx );
  iLeftIntraDir  = pcTempCU ? ( pcTempCU->isIntra( uiTempPartIdx ) ? pcTempCU->getLumaIntraDir( uiTempPartIdx ) : 2 ) : 2;
#if ADD_PLANAR_MODE
  mapPlanartoDC( iLeftIntraDir );
#endif
#if HHI_DMM_INTRA
  mapDMMtoDC( iLeftIntraDir );
#endif

  // Get intra direction of above PU
  pcTempCU = getPUAbove( uiTempPartIdx, m_uiAbsIdxInLCU + uiAbsPartIdx );
  iAboveIntraDir = pcTempCU ? ( pcTempCU->isIntra( uiTempPartIdx ) ? pcTempCU->getLumaIntraDir( uiTempPartIdx ) : 2 ) : 2;
#if ADD_PLANAR_MODE
  mapPlanartoDC( iAboveIntraDir );
#endif
#if HHI_DMM_INTRA
  mapDMMtoDC( iAboveIntraDir );
#endif

  Int iIdx  = getIntraSizeIdx(uiAbsPartIdx);

  if ( iLeftIntraDir >= g_aucIntraModeNumAng[iIdx] ) 
  {
   if ( g_aucIntraModeNumAng[iIdx] == 5 )
   {
      iLeftIntraDir = g_aucAngModeMapping[0][g_aucAngIntraModeOrder[iLeftIntraDir]];
   }
   if ( g_aucIntraModeNumAng[iIdx] == 3 )
   {
      iLeftIntraDir = g_aucAngModeMapping[3][g_aucAngIntraModeOrder[iLeftIntraDir]];
   }
   else
   {
      iLeftIntraDir = g_aucAngModeMapping[1][g_aucAngIntraModeOrder[iLeftIntraDir]]; 
   }
  }
   
   
 if ( iAboveIntraDir >= g_aucIntraModeNumAng[iIdx] ) 
 {
   if ( g_aucIntraModeNumAng[iIdx] == 5 )
   {
      iAboveIntraDir = g_aucAngModeMapping[0][g_aucAngIntraModeOrder[iAboveIntraDir]];
   }
   if ( g_aucIntraModeNumAng[iIdx] == 3 )
   {
      iAboveIntraDir = g_aucAngModeMapping[3][g_aucAngIntraModeOrder[iAboveIntraDir]];
   }
   else
   {
      iAboveIntraDir = g_aucAngModeMapping[1][g_aucAngIntraModeOrder[iAboveIntraDir]]; 
   }
 }
   
 if(iLeftIntraDir == iAboveIntraDir)
 {
   uiPredNum = 1;
   uiIntraDirPred[0] = iLeftIntraDir ;
 }
 else
 {
   uiPredNum = 2;
   uiIntraDirPred[0] = Min(iLeftIntraDir, iAboveIntraDir);
   uiIntraDirPred[1] = Max(iLeftIntraDir, iAboveIntraDir);
 }


  return uiPredNum;
}
#endif
#if LCEC_INTRA_MODE
Int TComDataCU::getLeftIntraDirLuma( UInt uiAbsPartIdx )
{
  TComDataCU* pcTempCU;
  UInt        uiTempPartIdx;
  Int         iLeftIntraDir;

  // Get intra direction of left PU
  pcTempCU = getPULeft( uiTempPartIdx, m_uiAbsIdxInLCU + uiAbsPartIdx );
  iLeftIntraDir  = pcTempCU ? ( pcTempCU->isIntra( uiTempPartIdx ) ? pcTempCU->getLumaIntraDir( uiTempPartIdx ) : 2 ) : NOT_VALID;
#if ADD_PLANAR_MODE
  mapPlanartoDC( iLeftIntraDir );
#endif

  return iLeftIntraDir;
}

Int TComDataCU::getAboveIntraDirLuma( UInt uiAbsPartIdx )
{
  TComDataCU* pcTempCU;
  UInt        uiTempPartIdx;
  Int         iAboveIntraDir;

  // Get intra direction of above PU
  pcTempCU = getPUAbove( uiTempPartIdx, m_uiAbsIdxInLCU + uiAbsPartIdx );
  iAboveIntraDir = pcTempCU ? ( pcTempCU->isIntra( uiTempPartIdx ) ? pcTempCU->getLumaIntraDir( uiTempPartIdx ) : 2 ) : NOT_VALID;
#if ADD_PLANAR_MODE
  mapPlanartoDC( iAboveIntraDir );
#endif

  return iAboveIntraDir;
}
#endif

UInt TComDataCU::getCtxSplitFlag( UInt uiAbsPartIdx, UInt uiDepth )
{
  TComDataCU* pcTempCU;
  UInt        uiTempPartIdx;
  UInt        uiCtx;
  // Get left split flag
  pcTempCU = getPULeft( uiTempPartIdx, m_uiAbsIdxInLCU + uiAbsPartIdx );
  uiCtx  = ( pcTempCU ) ? ( ( pcTempCU->getDepth( uiTempPartIdx ) > uiDepth ) ? 1 : 0 ) : 0;
  
  // Get above split flag
  pcTempCU = getPUAbove( uiTempPartIdx, m_uiAbsIdxInLCU + uiAbsPartIdx );
  uiCtx += ( pcTempCU ) ? ( ( pcTempCU->getDepth( uiTempPartIdx ) > uiDepth ) ? 1 : 0 ) : 0;
  
  return uiCtx;
}

#if MW_MVI_SIGNALLING_MODE == 0
UInt TComDataCU::getCtxMvInheritanceFlag( UInt uiAbsPartIdx, UInt uiDepth )
{
//  return 0; // MW
  TComDataCU* pcTempCU;
  UInt        uiTempPartIdx;
  UInt        uiCtx;
  // Get left split flag
  pcTempCU = getPULeft( uiTempPartIdx, m_uiAbsIdxInLCU + uiAbsPartIdx );
  uiCtx  = ( pcTempCU ) ? ( ( pcTempCU->getTextureModeDepth( uiTempPartIdx ) == uiDepth ) ? 1 : 0 ) : 0;

  // Get above split flag
  pcTempCU = getPUAbove( uiTempPartIdx, m_uiAbsIdxInLCU + uiAbsPartIdx );
  uiCtx += ( pcTempCU ) ? ( ( pcTempCU->getTextureModeDepth( uiTempPartIdx ) == uiDepth ) ? 1 : 0 ) : 0;

  return uiCtx;
}
#endif

UInt TComDataCU::getCtxIntraDirChroma( UInt uiAbsPartIdx )
{
  TComDataCU* pcTempCU;
  UInt        uiTempPartIdx;
  UInt        uiCtx;
  
  // Get intra direction of left PU
  pcTempCU = getPULeft( uiTempPartIdx, m_uiAbsIdxInLCU + uiAbsPartIdx );
#if CHROMA_CODEWORD
  uiCtx  = ( pcTempCU ) ? ( ( pcTempCU->isIntra( uiTempPartIdx ) && pcTempCU->getChromaIntraDir( uiTempPartIdx ) == 4 ) ? 1 : 0 ) : 0;
#else
  uiCtx  = ( pcTempCU ) ? ( ( pcTempCU->isIntra( uiTempPartIdx ) && pcTempCU->getChromaIntraDir( uiTempPartIdx ) > 0 ) ? 1 : 0 ) : 0;
#endif
  
  // Get intra direction of above PU
  pcTempCU = getPUAbove( uiTempPartIdx, m_uiAbsIdxInLCU + uiAbsPartIdx );
#if CHROMA_CODEWORD
  uiCtx += ( pcTempCU ) ? ( ( pcTempCU->isIntra( uiTempPartIdx ) && pcTempCU->getChromaIntraDir( uiTempPartIdx ) == 4 ) ? 1 : 0 ) : 0;
#else
  uiCtx += ( pcTempCU ) ? ( ( pcTempCU->isIntra( uiTempPartIdx ) && pcTempCU->getChromaIntraDir( uiTempPartIdx ) > 0 ) ? 1 : 0 ) : 0;
#endif
  
  return uiCtx;
}

UInt TComDataCU::getCtxCbf( UInt uiAbsPartIdx, TextType eType, UInt uiTrDepth )
{
  TComDataCU* pcTempCU;
  UInt        uiTempPartIdx, uiTempTrDepth;
  UInt        uiErrRet = !isIntra(uiAbsPartIdx) ? 0 : 1;
  UInt        uiCtx = 0;
  
  // Get Cbf of left PU
  pcTempCU = getPULeft( uiTempPartIdx, m_uiAbsIdxInLCU + uiAbsPartIdx );
  if ( pcTempCU )
  {
    uiTempTrDepth = pcTempCU->getTransformIdx( uiTempPartIdx );
    uiCtx = pcTempCU->getCbf( uiTempPartIdx, eType, uiTempTrDepth < uiTrDepth ? uiTempTrDepth : uiTrDepth );
  }
  else
  {
    uiCtx = uiErrRet;
  }
  
  // Get Cbf of above PU
  pcTempCU = getPUAbove( uiTempPartIdx, m_uiAbsIdxInLCU + uiAbsPartIdx );
  if ( pcTempCU )
  {
    uiTempTrDepth = pcTempCU->getTransformIdx( uiTempPartIdx );
    uiCtx += pcTempCU->getCbf( uiTempPartIdx, eType, uiTempTrDepth < uiTrDepth ? uiTempTrDepth : uiTrDepth ) << 1;
  }
  else
  {
    uiCtx += uiErrRet << 1;
  }
  
  return uiCtx;
}

UInt TComDataCU::getCtxQtCbf( UInt uiAbsPartIdx, TextType eType, UInt uiTrDepth )
{
  if( getPredictionMode( uiAbsPartIdx ) != MODE_INTRA && eType != TEXT_LUMA )
  {
    return uiTrDepth;
  }
  UInt uiCtx = 0;
  const UInt uiDepth = getDepth( uiAbsPartIdx );
  const UInt uiLog2TrafoSize = g_aucConvertToBit[getSlice()->getSPS()->getMaxCUWidth()]+2 - uiDepth - uiTrDepth;
  
  if( uiTrDepth == 0 || uiLog2TrafoSize == getSlice()->getSPS()->getQuadtreeTULog2MaxSize() )
  {
    TComDataCU* pcTempCU;
    UInt        uiTempPartIdx, uiTempTrDepth;
    
    // Get Cbf of left PU
    pcTempCU = getPULeft( uiTempPartIdx, m_uiAbsIdxInLCU + uiAbsPartIdx );
    if ( pcTempCU )
    {
      uiTempTrDepth = pcTempCU->getTransformIdx( uiTempPartIdx );
      uiCtx = pcTempCU->getCbf( uiTempPartIdx, eType, uiTempTrDepth );
    }
    
    // Get Cbf of above PU
    pcTempCU = getPUAbove( uiTempPartIdx, m_uiAbsIdxInLCU + uiAbsPartIdx );
    if ( pcTempCU )
    {
      uiTempTrDepth = pcTempCU->getTransformIdx( uiTempPartIdx );
      uiCtx += pcTempCU->getCbf( uiTempPartIdx, eType, uiTempTrDepth ) << 1;
    }
    uiCtx++;
  }
  return uiCtx;
}

UInt TComDataCU::getCtxQtRootCbf( UInt uiAbsPartIdx )
{
  UInt uiCtx = 0;
  TComDataCU* pcTempCU;
  UInt        uiTempPartIdx;
  
  // Get RootCbf of left PU
  pcTempCU = getPULeft( uiTempPartIdx, m_uiAbsIdxInLCU + uiAbsPartIdx );
  if ( pcTempCU )
  {
    uiCtx = pcTempCU->getQtRootCbf( uiTempPartIdx );
  }
  
  // Get RootCbf of above PU
  pcTempCU = getPUAbove( uiTempPartIdx, m_uiAbsIdxInLCU + uiAbsPartIdx );
  if ( pcTempCU )
  {
    uiCtx += pcTempCU->getQtRootCbf( uiTempPartIdx ) << 1;
  }
  
  return uiCtx;
}

UInt TComDataCU::getQuadtreeTULog2MinSizeInCU( UInt uiIdx )
{
#if HHI_RQT_DISABLE_SUB
  return getQuadtreeTULog2RootSizeInCU(uiIdx);
#else
#if HHI_RQT_FORCE_SPLIT_ACC2_PU
  UInt uiLog2MinTUSizeInCU = getQuadtreeTULog2RootSizeInCU(uiIdx);
#else
  UInt uiLog2MinTUSizeInCU = g_aucConvertToBit[getWidth( uiIdx )] + 2;
  
  if ( getPredictionMode( uiIdx ) == MODE_INTRA && getPartitionSize( uiIdx ) == SIZE_NxN )
  {
    uiLog2MinTUSizeInCU--;
  }
#endif  
  
  UInt uiQuadtreeTUMaxDepth = getPredictionMode( uiIdx ) == MODE_INTRA ? m_pcSlice->getSPS()->getQuadtreeTUMaxDepthIntra() : m_pcSlice->getSPS()->getQuadtreeTUMaxDepthInter();
  if (uiLog2MinTUSizeInCU < m_pcSlice->getSPS()->getQuadtreeTULog2MinSize() + uiQuadtreeTUMaxDepth - 1)
  {
    uiLog2MinTUSizeInCU = m_pcSlice->getSPS()->getQuadtreeTULog2MinSize();  
  }
  else
  {
    uiLog2MinTUSizeInCU -= uiQuadtreeTUMaxDepth - 1;  
  }
  
  if ( uiLog2MinTUSizeInCU > m_pcSlice->getSPS()->getQuadtreeTULog2MaxSize())
  {
    uiLog2MinTUSizeInCU = m_pcSlice->getSPS()->getQuadtreeTULog2MaxSize();
  }  
  return uiLog2MinTUSizeInCU;
#endif
}

#if HHI_RQT_FORCE_SPLIT_ACC2_PU || HHI_RQT_DISABLE_SUB
UInt TComDataCU::getQuadtreeTULog2RootSizeInCU( UInt uiIdx )
{
  UInt uiLog2RootTUSizeInCU = g_aucConvertToBit[getWidth( uiIdx )] +2;
  
#if !HHI_RQT_FORCE_SPLIT_NxN && !HHI_RQT_DISABLE_SUB
  if ( getPredictionMode( uiIdx ) == MODE_INTRA && getPartitionSize( uiIdx ) == SIZE_NxN )
  {
    uiLog2RootTUSizeInCU--;
    return (uiLog2RootTUSizeInCU > m_pcSlice->getSPS()->getQuadtreeTULog2MaxSize() ? m_pcSlice->getSPS()->getQuadtreeTULog2MaxSize() : uiLog2RootTUSizeInCU);
  }
#endif
  
  if (m_pePartSize[ uiIdx ] == SIZE_2Nx2N)
  {
    return (uiLog2RootTUSizeInCU > m_pcSlice->getSPS()->getQuadtreeTULog2MaxSize() ? m_pcSlice->getSPS()->getQuadtreeTULog2MaxSize() : uiLog2RootTUSizeInCU);
  }
  else if (m_pePartSize[ uiIdx ] >= SIZE_2NxN && m_pePartSize[ uiIdx ] <= SIZE_Nx2N)  
  {
#if HHI_RQT_FORCE_SPLIT_RECT || HHI_RQT_DISABLE_SUB
    uiLog2RootTUSizeInCU--;
#endif
    return (uiLog2RootTUSizeInCU > m_pcSlice->getSPS()->getQuadtreeTULog2MaxSize() ? m_pcSlice->getSPS()->getQuadtreeTULog2MaxSize() : uiLog2RootTUSizeInCU);
  }
  else if (m_pePartSize[ uiIdx ] == SIZE_NxN)  
  {
#if HHI_RQT_FORCE_SPLIT_NxN || HHI_RQT_DISABLE_SUB
    uiLog2RootTUSizeInCU--;
#endif
    return (uiLog2RootTUSizeInCU > m_pcSlice->getSPS()->getQuadtreeTULog2MaxSize() ? m_pcSlice->getSPS()->getQuadtreeTULog2MaxSize() : uiLog2RootTUSizeInCU);
  }
  
  return (uiLog2RootTUSizeInCU > m_pcSlice->getSPS()->getQuadtreeTULog2MaxSize() ? m_pcSlice->getSPS()->getQuadtreeTULog2MaxSize() : uiLog2RootTUSizeInCU);
}
#endif

UInt TComDataCU::getCtxAlfCtrlFlag( UInt uiAbsPartIdx )
{
  TComDataCU* pcTempCU;
  UInt        uiTempPartIdx;
  UInt        uiCtx = 0;
  
  // Get BCBP of left PU
  pcTempCU = getPULeft( uiTempPartIdx, m_uiAbsIdxInLCU + uiAbsPartIdx );
  uiCtx    = ( pcTempCU ) ? pcTempCU->getAlfCtrlFlag( uiTempPartIdx ) : 0;
  
  // Get BCBP of above PU
  pcTempCU = getPUAbove( uiTempPartIdx, m_uiAbsIdxInLCU + uiAbsPartIdx );
  uiCtx   += ( pcTempCU ) ? pcTempCU->getAlfCtrlFlag( uiTempPartIdx ) : 0;
  
  return uiCtx;
}

UInt TComDataCU::getCtxSkipFlag( UInt uiAbsPartIdx )
{
  TComDataCU* pcTempCU;
  UInt        uiTempPartIdx;
  UInt        uiCtx = 0;
  
  // Get BCBP of left PU
  pcTempCU = getPULeft( uiTempPartIdx, m_uiAbsIdxInLCU + uiAbsPartIdx );
  uiCtx    = ( pcTempCU ) ? pcTempCU->isSkipped( uiTempPartIdx ) : 0;
  
  // Get BCBP of above PU
  pcTempCU = getPUAbove( uiTempPartIdx, m_uiAbsIdxInLCU + uiAbsPartIdx );
  uiCtx   += ( pcTempCU ) ? pcTempCU->isSkipped( uiTempPartIdx ) : 0;
  
  return uiCtx;
}

/** CABAC context derivation for merge flag
 * \param uiAbsPartIdx
 * \returns context offset
 */
UInt TComDataCU::getCtxMergeFlag( UInt uiAbsPartIdx )
{
  UInt uiCtx = 0;
#if CHANGE_MERGE_CONTEXT
  TComDataCU* pcTempCU;
  UInt        uiTempPartIdx;

  // Get BCBP of left PU
  pcTempCU = getPULeft( uiTempPartIdx, m_uiAbsIdxInLCU + uiAbsPartIdx );
  uiCtx    = ( pcTempCU ) ? pcTempCU->getMergeFlag( uiTempPartIdx ) : 0;

  // Get BCBP of above PU
  pcTempCU = getPUAbove( uiTempPartIdx, m_uiAbsIdxInLCU + uiAbsPartIdx );
  uiCtx   += ( pcTempCU ) ? pcTempCU->getMergeFlag( uiTempPartIdx ) : 0;
#endif
  return uiCtx;
}

UInt TComDataCU::getCtxInterDir( UInt uiAbsPartIdx )
{
  TComDataCU* pcTempCU;
  UInt        uiTempPartIdx;
  UInt        uiCtx = 0;
  
  // Get BCBP of left PU
  pcTempCU = getPULeft( uiTempPartIdx, m_uiAbsIdxInLCU + uiAbsPartIdx );
  uiCtx += ( pcTempCU ) ? ( ( pcTempCU->getInterDir( uiTempPartIdx ) % 3 ) ? 0 : 1 ) : 0;
  
  // Get BCBP of Above PU
  pcTempCU = getPUAbove( uiTempPartIdx, m_uiAbsIdxInLCU + uiAbsPartIdx );
  uiCtx += ( pcTempCU ) ? ( ( pcTempCU->getInterDir( uiTempPartIdx ) % 3 ) ? 0 : 1 ) : 0;
  
  return uiCtx;
}

UInt TComDataCU::getCtxRefIdx( UInt uiAbsPartIdx, RefPicList eRefPicList )
{
  TComDataCU* pcTempCU;
  UInt        uiTempPartIdx;
  TComMvField cMvFieldTemp;
  UInt        uiCtx = 0;
  
  // Get BCBP of left PU
  pcTempCU = getPULeft( uiTempPartIdx, m_uiAbsIdxInLCU + uiAbsPartIdx );
  getMvField( pcTempCU, uiTempPartIdx, eRefPicList, cMvFieldTemp );
  uiCtx += cMvFieldTemp.getRefIdx() > 0 ? 1 : 0;
  
  // Get BCBP of Above PU
  pcTempCU = getPUAbove( uiTempPartIdx, m_uiAbsIdxInLCU + uiAbsPartIdx );
  getMvField( pcTempCU, uiTempPartIdx, eRefPicList, cMvFieldTemp );
  uiCtx += cMvFieldTemp.getRefIdx() > 0 ? 2 : 0;
  
  return uiCtx;
}

Void TComDataCU::setCbfSubParts( UInt uiCbfY, UInt uiCbfU, UInt uiCbfV, UInt uiAbsPartIdx, UInt uiDepth )
{
  UInt uiCurrPartNumb = m_pcPic->getNumPartInCU() >> (uiDepth << 1);
  memset( m_puhCbf[0] + uiAbsPartIdx, uiCbfY, sizeof( UChar ) * uiCurrPartNumb );
  memset( m_puhCbf[1] + uiAbsPartIdx, uiCbfU, sizeof( UChar ) * uiCurrPartNumb );
  memset( m_puhCbf[2] + uiAbsPartIdx, uiCbfV, sizeof( UChar ) * uiCurrPartNumb );
}

Void TComDataCU::setCbfSubParts( UInt uiCbf, TextType eTType, UInt uiAbsPartIdx, UInt uiDepth )
{
  UInt uiCurrPartNumb = m_pcPic->getNumPartInCU() >> (uiDepth << 1);
  memset( m_puhCbf[g_aucConvertTxtTypeToIdx[eTType]] + uiAbsPartIdx, uiCbf, sizeof( UChar ) * uiCurrPartNumb );
}

#if HHI_MRG_SKIP
/** Sets a coded block flag for all sub-partitions of a partition
 * \param uiCbf The value of the coded block flag to be set
 * \param eTType
 * \param uiAbsPartIdx
 * \param uiPartIdx
 * \param uiDepth
 * \returns Void
 */
Void TComDataCU::setCbfSubParts ( UInt uiCbf, TextType eTType, UInt uiAbsPartIdx, UInt uiPartIdx, UInt uiDepth )
{
  setSubPartUChar( uiCbf, m_puhCbf[g_aucConvertTxtTypeToIdx[eTType]], uiAbsPartIdx, uiDepth, uiPartIdx );
}
#endif

Void TComDataCU::setDepthSubParts( UInt uiDepth, UInt uiAbsPartIdx )
{
  UInt uiCurrPartNumb = m_pcPic->getNumPartInCU() >> (uiDepth << 1);
  memset( m_puhDepth + uiAbsPartIdx, uiDepth, sizeof(UChar)*uiCurrPartNumb );
}

Bool TComDataCU::isFirstAbsZorderIdxInDepth (UInt uiAbsPartIdx, UInt uiDepth)
{
  UInt uiPartNumb = m_pcPic->getNumPartInCU() >> (uiDepth << 1);
  return (((m_uiAbsIdxInLCU + uiAbsPartIdx)% uiPartNumb) == 0);
}

Void TComDataCU::setAlfCtrlFlagSubParts         ( UInt uiFlag, UInt uiAbsPartIdx, UInt uiDepth )
{
  UInt uiCurrPartNumb = m_pcPic->getNumPartInCU() >> (uiDepth << 1);
  
  for (UInt ui = 0; ui < uiCurrPartNumb; ui++ )
  {
    m_puiAlfCtrlFlag[uiAbsPartIdx + ui] = uiFlag;
  }
}

Void TComDataCU::createTmpAlfCtrlFlag()
{
  m_puiTmpAlfCtrlFlag = (UInt* )xMalloc(UInt, m_uiNumPartition);
}

Void TComDataCU::destroyTmpAlfCtrlFlag()
{
  if(m_puiTmpAlfCtrlFlag)
  {
    xFree(m_puiTmpAlfCtrlFlag);        m_puiTmpAlfCtrlFlag = NULL;
  }
}

Void TComDataCU::copyAlfCtrlFlagToTmp()
{
  memcpy( m_puiTmpAlfCtrlFlag, m_puiAlfCtrlFlag, sizeof(UInt)*m_uiNumPartition );
}

Void TComDataCU::copyAlfCtrlFlagFromTmp()
{
  memcpy( m_puiAlfCtrlFlag, m_puiTmpAlfCtrlFlag, sizeof(UInt)*m_uiNumPartition );
}

Void TComDataCU::setPartSizeSubParts( PartSize eMode, UInt uiAbsPartIdx, UInt uiDepth )
{
  UInt uiCurrPartNumb = m_pcPic->getNumPartInCU() >> (uiDepth << 1);
  
  for (UInt ui = 0; ui < uiCurrPartNumb; ui++ )
  {
    m_pePartSize[uiAbsPartIdx + ui] = eMode;
  }
}

Void TComDataCU::setPredModeSubParts( PredMode eMode, UInt uiAbsPartIdx, UInt uiDepth )
{
  UInt uiCurrPartNumb = m_pcPic->getNumPartInCU() >> (uiDepth << 1);
  
  for (UInt ui = 0; ui < uiCurrPartNumb; ui++ )
  {
    m_pePredMode[uiAbsPartIdx + ui] = eMode;
  }
}

Void TComDataCU::setQPSubParts( UInt uiQP, UInt uiAbsPartIdx, UInt uiDepth )
{
  UInt uiCurrPartNumb = m_pcPic->getNumPartInCU() >> (uiDepth << 1);
  
  memset( m_phQP + uiAbsPartIdx, uiQP, sizeof(Char)*uiCurrPartNumb );
}

Void TComDataCU::setLumaIntraDirSubParts( UInt uiDir, UInt uiAbsPartIdx, UInt uiDepth )
{
  UInt uiCurrPartNumb = m_pcPic->getNumPartInCU() >> (uiDepth << 1);
  
  memset( m_puhLumaIntraDir + uiAbsPartIdx, uiDir, sizeof(UChar)*uiCurrPartNumb );
}

Void TComDataCU::setSubPartUChar( UInt uiParameter, UChar* puhBaseLCU, UInt uiCUAddr, UInt uiCUDepth, UInt uiPUIdx )
{
  UInt uiCurrPartNumQ = (m_pcPic->getNumPartInCU() >> (uiCUDepth << 1)) >> 2;
  switch ( m_pePartSize[ uiCUAddr ] )
  {
  case SIZE_2Nx2N:
    memset( puhBaseLCU + uiCUAddr, uiParameter, sizeof(UChar)*uiCurrPartNumQ << 2 );                      break;
  case SIZE_2NxN:
    memset( puhBaseLCU + uiCUAddr, uiParameter, sizeof(UChar)*uiCurrPartNumQ << 1 );                      break;
  case SIZE_Nx2N:
    memset( puhBaseLCU + uiCUAddr, uiParameter, sizeof(UChar)*uiCurrPartNumQ );
    memset( puhBaseLCU + uiCUAddr + ( uiCurrPartNumQ << 1 ), uiParameter, sizeof(UChar)*uiCurrPartNumQ ); break;
  case SIZE_NxN:
    memset( puhBaseLCU + uiCUAddr, uiParameter, sizeof(UChar)*uiCurrPartNumQ );                           break;
  default:
    assert( 0 );
  }
}

Void TComDataCU::setSubPartBool( Bool bParameter, Bool* pbBaseLCU, UInt uiCUAddr, UInt uiCUDepth, UInt uiPUIdx )
{
  UInt uiQuaterCUPartNum = (m_pcPic->getNumPartInCU() >> (uiCUDepth << 1)) >> 2;
  switch ( m_pePartSize[ uiCUAddr ] )
  {
  case SIZE_2Nx2N:
    memset( pbBaseLCU + uiCUAddr, bParameter, sizeof(Bool)*uiQuaterCUPartNum << 2 );                      break;
  case SIZE_2NxN:
    memset( pbBaseLCU + uiCUAddr, bParameter, sizeof(Bool)*uiQuaterCUPartNum << 1 );                      break;
  case SIZE_Nx2N:
    memset( pbBaseLCU + uiCUAddr, bParameter, sizeof(Bool)*uiQuaterCUPartNum );
    memset( pbBaseLCU + uiCUAddr + ( uiQuaterCUPartNum << 1 ), bParameter, sizeof(Bool)*uiQuaterCUPartNum ); break;
  case SIZE_NxN:
    memset( pbBaseLCU + uiCUAddr, bParameter, sizeof(Bool)*uiQuaterCUPartNum );                           break;
  default:
    assert( 0 );
  }
}

Void TComDataCU::setMergeFlagSubParts ( Bool bMergeFlag, UInt uiAbsPartIdx, UInt uiPartIdx, UInt uiDepth )
{
  setSubPartBool( bMergeFlag, m_pbMergeFlag, uiAbsPartIdx, uiDepth, uiPartIdx );
}

Void TComDataCU::setMergeIndexSubParts ( UInt uiMergeIndex, UInt uiAbsPartIdx, UInt uiPartIdx, UInt uiDepth )
{
  setSubPartUChar( uiMergeIndex, m_puhMergeIndex, uiAbsPartIdx, uiDepth, uiPartIdx );
}

Void TComDataCU::setResPredAvailSubParts( Bool bResPredAvailable, UInt uiAbsPartIdx, UInt uiPartIdx, UInt uiDepth )
{
  setSubPartBool( bResPredAvailable, m_pbResPredAvailable, uiAbsPartIdx, uiDepth, uiPartIdx );
}

Void TComDataCU::setResPredFlagSubParts( Bool bResPredFlag, UInt uiAbsPartIdx, UInt uiPartIdx, UInt uiDepth )
{
  setSubPartBool( bResPredFlag, m_pbResPredFlag, uiAbsPartIdx, uiDepth, uiPartIdx );
}

Void TComDataCU::setChromIntraDirSubParts( UInt uiDir, UInt uiAbsPartIdx, UInt uiDepth )
{
  UInt uiCurrPartNumb = m_pcPic->getNumPartInCU() >> (uiDepth << 1);
  
  memset( m_puhChromaIntraDir + uiAbsPartIdx, uiDir, sizeof(UChar)*uiCurrPartNumb );
}

Void TComDataCU::setInterDirSubParts( UInt uiDir, UInt uiAbsPartIdx, UInt uiPartIdx, UInt uiDepth )
{
  setSubPartUChar( uiDir, m_puhInterDir, uiAbsPartIdx, uiDepth, uiPartIdx );
}

Void TComDataCU::setMVPIdxSubParts( Int iMVPIdx, RefPicList eRefPicList, UInt uiAbsPartIdx, UInt uiPartIdx, UInt uiDepth )
{
  UInt uiCurrPartNumQ = m_pcPic->getNumPartInCU() >> (uiDepth << 1) >> 2;
  UInt i;
  Int* pi;
  switch ( m_pePartSize[ uiAbsPartIdx ] )
  {
    case SIZE_2Nx2N:
    {
      pi = m_apiMVPIdx[eRefPicList] + uiAbsPartIdx;
      for (i = 0; i < (uiCurrPartNumQ << 2); i++)
      {
        pi[i] = iMVPIdx;
      }
    }
      break;
    case SIZE_2NxN:
    {
      pi = m_apiMVPIdx[eRefPicList] + uiAbsPartIdx;
      for (i = 0; i < (uiCurrPartNumQ << 1); i++)
      {
        pi[i] = iMVPIdx;
      }
    }
      break;
    case SIZE_Nx2N:
    {
      Int* pi2 = m_apiMVPIdx[eRefPicList] + uiAbsPartIdx + ( uiCurrPartNumQ << 1 );
      pi = m_apiMVPIdx[eRefPicList] + uiAbsPartIdx;
      for (i = 0; i < uiCurrPartNumQ; i++)
      {
        pi [i] = iMVPIdx;
        pi2[i] = iMVPIdx;
      }
      break;
    }
    case SIZE_NxN:
    {
      pi = m_apiMVPIdx[eRefPicList] + uiAbsPartIdx;
      for (i = 0; i < uiCurrPartNumQ; i++)
      {
        pi[i] = iMVPIdx;
      }
      break;
    }
    default:
      assert( 0 );
  }
}

Void TComDataCU::setMVPNumSubParts( Int iMVPNum, RefPicList eRefPicList, UInt uiAbsPartIdx, UInt uiPartIdx, UInt uiDepth )
{
  UInt uiCurrPartNumQ = m_pcPic->getNumPartInCU() >> (uiDepth << 1) >> 2;
  UInt i;
  Int* pi;
  switch ( m_pePartSize[ uiAbsPartIdx ] )
  {
    case SIZE_2Nx2N:
    {
      pi = m_apiMVPNum[eRefPicList] + uiAbsPartIdx;
      for (i = 0; i < (uiCurrPartNumQ << 2); i++)
      {
        pi[i] = iMVPNum;
      }
    }
      break;
    case SIZE_2NxN:
    {
      pi = m_apiMVPNum[eRefPicList] + uiAbsPartIdx;
      for (i = 0; i < (uiCurrPartNumQ << 1); i++)
      {
        pi[i] = iMVPNum;
      }
    }
      break;
    case SIZE_Nx2N:
    {
      pi = m_apiMVPNum[eRefPicList] + uiAbsPartIdx;
      for (i = 0; i < uiCurrPartNumQ; i++)
      {
        pi[i] = iMVPNum;
      }
      pi = m_apiMVPNum[eRefPicList] + uiAbsPartIdx + ( uiCurrPartNumQ << 1 );
      for (i = 0; i < uiCurrPartNumQ; i++)
      {
        pi[i] = iMVPNum;
      }
    }
      break;
    case SIZE_NxN:
    {
      pi = m_apiMVPNum[eRefPicList] + uiAbsPartIdx;
      for (i = 0; i < uiCurrPartNumQ; i++)
      {
        pi[i] = iMVPNum;
      }
    }
      break;
    default:
      assert( 0 );
  }
}


Void TComDataCU::setTrIdxSubParts( UInt uiTrIdx, UInt uiAbsPartIdx, UInt uiDepth )
{
  UInt uiCurrPartNumb = m_pcPic->getNumPartInCU() >> (uiDepth << 1);
  
  memset( m_puhTrIdx + uiAbsPartIdx, uiTrIdx, sizeof(UChar)*uiCurrPartNumb );
}

Void TComDataCU::setSizeSubParts( UInt uiWidth, UInt uiHeight, UInt uiAbsPartIdx, UInt uiDepth )
{
  UInt uiCurrPartNumb = m_pcPic->getNumPartInCU() >> (uiDepth << 1);
  
  memset( m_puhWidth  + uiAbsPartIdx, uiWidth,  sizeof(UChar)*uiCurrPartNumb );
  memset( m_puhHeight + uiAbsPartIdx, uiHeight, sizeof(UChar)*uiCurrPartNumb );
}

UChar TComDataCU::getNumPartInter()
{
  UChar iNumPart = 0;
  
  switch ( m_pePartSize[0] )
  {
    case SIZE_2Nx2N:    iNumPart = 1; break;
    case SIZE_2NxN:     iNumPart = 2; break;
    case SIZE_Nx2N:     iNumPart = 2; break;
    case SIZE_NxN:      iNumPart = 4; break;
    default:            assert (0);   break;
  }
  
  return  iNumPart;
}

Void TComDataCU::getPartIndexAndSize( UInt uiPartIdx, UInt& ruiPartAddr, Int& riWidth, Int& riHeight )
{
  switch ( m_pePartSize[0] )
  {
    case SIZE_2NxN:
      riWidth = getWidth(0);      riHeight = getHeight(0) >> 1; ruiPartAddr = ( uiPartIdx == 0 )? 0 : m_uiNumPartition >> 1;
      break;
    case SIZE_Nx2N:
      riWidth = getWidth(0) >> 1; riHeight = getHeight(0);      ruiPartAddr = ( uiPartIdx == 0 )? 0 : m_uiNumPartition >> 2;
      break;
    case SIZE_NxN:
      riWidth = getWidth(0) >> 1; riHeight = getHeight(0) >> 1; ruiPartAddr = ( m_uiNumPartition >> 2 ) * uiPartIdx;
      break;
    default:
      assert ( m_pePartSize[0] == SIZE_2Nx2N );
      riWidth = getWidth(0);      riHeight = getHeight(0);      ruiPartAddr = 0;
      break;
  }
}


Void TComDataCU::getMvField ( TComDataCU* pcCU, UInt uiAbsPartIdx, RefPicList eRefPicList, TComMvField& rcMvField )
{
  if ( pcCU == NULL )  // OUT OF BOUNDARY
  {
    TComMv  cZeroMv;
    rcMvField.setMvField( cZeroMv, NOT_VALID );
    return;
  }
  
  TComCUMvField*  pcCUMvField = pcCU->getCUMvField( eRefPicList );
  rcMvField.setMvField( pcCUMvField->getMv( uiAbsPartIdx ), pcCUMvField->getRefIdx( uiAbsPartIdx ) );
}

Void TComDataCU::deriveLeftRightTopIdxGeneral ( PartSize eCUMode, UInt uiAbsPartIdx, UInt uiPartIdx, UInt& ruiPartIdxLT, UInt& ruiPartIdxRT )
{
  ruiPartIdxLT = m_uiAbsIdxInLCU + uiAbsPartIdx;
  UInt uiPUWidth = 0;
  
  switch ( m_pePartSize[uiAbsPartIdx] )
  {
    case SIZE_2Nx2N: uiPUWidth = m_puhWidth[uiAbsPartIdx];  break;
    case SIZE_2NxN:  uiPUWidth = m_puhWidth[uiAbsPartIdx];   break;
    case SIZE_Nx2N:  uiPUWidth = m_puhWidth[uiAbsPartIdx]  >> 1;  break;
    case SIZE_NxN:   uiPUWidth = m_puhWidth[uiAbsPartIdx]  >> 1; break;
    default:
      assert (0);
      break;
  }
  
  ruiPartIdxRT = g_auiRasterToZscan [g_auiZscanToRaster[ ruiPartIdxLT ] + uiPUWidth / m_pcPic->getMinCUWidth() - 1 ];
}

Void TComDataCU::deriveLeftBottomIdxGeneral( PartSize eCUMode, UInt uiAbsPartIdx, UInt uiPartIdx, UInt& ruiPartIdxLB )
{
  UInt uiPUHeight = 0;
  switch ( m_pePartSize[uiAbsPartIdx] )
  {
    case SIZE_2Nx2N: uiPUHeight = m_puhHeight[uiAbsPartIdx];    break;
    case SIZE_2NxN:  uiPUHeight = m_puhHeight[uiAbsPartIdx] >> 1;    break;
    case SIZE_Nx2N:  uiPUHeight = m_puhHeight[uiAbsPartIdx];  break;
    case SIZE_NxN:   uiPUHeight = m_puhHeight[uiAbsPartIdx] >> 1;    break;
    default:
      assert (0);
      break;
  }
  
  ruiPartIdxLB      = g_auiRasterToZscan [g_auiZscanToRaster[ m_uiAbsIdxInLCU + uiAbsPartIdx ] + ((uiPUHeight / m_pcPic->getMinCUHeight()) - 1)*m_pcPic->getNumPartInWidth()];
}

Void TComDataCU::deriveLeftRightTopIdx ( PartSize eCUMode, UInt uiPartIdx, UInt& ruiPartIdxLT, UInt& ruiPartIdxRT )
{
  ruiPartIdxLT = m_uiAbsIdxInLCU;
  ruiPartIdxRT = g_auiRasterToZscan [g_auiZscanToRaster[ ruiPartIdxLT ] + m_puhWidth[0] / m_pcPic->getMinCUWidth() - 1 ];
  
  switch ( m_pePartSize[0] )
  {
    case SIZE_2Nx2N:                                                                                                                                break;
    case SIZE_2NxN:
      ruiPartIdxLT += ( uiPartIdx == 0 )? 0 : m_uiNumPartition >> 1; ruiPartIdxRT += ( uiPartIdx == 0 )? 0 : m_uiNumPartition >> 1;
      break;
    case SIZE_Nx2N:
      ruiPartIdxLT += ( uiPartIdx == 0 )? 0 : m_uiNumPartition >> 2; ruiPartIdxRT -= ( uiPartIdx == 1 )? 0 : m_uiNumPartition >> 2;
      break;
    case SIZE_NxN:
      ruiPartIdxLT += ( m_uiNumPartition >> 2 ) * uiPartIdx;         ruiPartIdxRT +=  ( m_uiNumPartition >> 2 ) * ( uiPartIdx - 1 );
      break;
    default:
      assert (0);
      break;
  }
  
}

Void TComDataCU::deriveLeftBottomIdx( PartSize      eCUMode,   UInt  uiPartIdx,      UInt&      ruiPartIdxLB )
{
  ruiPartIdxLB      = g_auiRasterToZscan [g_auiZscanToRaster[ m_uiAbsIdxInLCU ] + ( ((m_puhHeight[0] / m_pcPic->getMinCUHeight())>>1) - 1)*m_pcPic->getNumPartInWidth()];
  
  switch ( m_pePartSize[0] )
  {
    case SIZE_2Nx2N:
      ruiPartIdxLB += m_uiNumPartition >> 1;
      break;
    case SIZE_2NxN:
      ruiPartIdxLB += ( uiPartIdx == 0 )? 0 : m_uiNumPartition >> 1;
      break;
    case SIZE_Nx2N:
      ruiPartIdxLB += ( uiPartIdx == 0 )? m_uiNumPartition >> 1 : (m_uiNumPartition >> 2)*3;
      break;
    case SIZE_NxN:
      ruiPartIdxLB += ( m_uiNumPartition >> 2 ) * uiPartIdx;
      break;
    default:
      assert (0);
      break;
  }
}

#if MTK_TMVP_H_MRG || MTK_TMVP_H_AMVP
/** Derives the partition index of neighbouring bottom right block
 * \param in eCUMode
 * \param in uiPartIdx 
 * \param out ruiPartIdx 
 * \returns Void
 */
Void TComDataCU::deriveRightBottomIdx( PartSize      eCUMode,   UInt  uiPartIdx,      UInt&      ruiPartIdxRB )
{
  ruiPartIdxRB      = g_auiRasterToZscan [g_auiZscanToRaster[ m_uiAbsIdxInLCU ] + ( ((m_puhHeight[0] / m_pcPic->getMinCUHeight())>>1) - 1)*m_pcPic->getNumPartInWidth() +  m_puhWidth[0] / m_pcPic->getMinCUWidth() - 1];

  switch ( m_pePartSize[0] )
  {
    case SIZE_2Nx2N:  
      ruiPartIdxRB += m_uiNumPartition >> 1;    
      break;
    case SIZE_2NxN:  
      ruiPartIdxRB += ( uiPartIdx == 0 )? 0 : m_uiNumPartition >> 1;   
      break;
    case SIZE_Nx2N:  
      ruiPartIdxRB += ( uiPartIdx == 0 )? m_uiNumPartition >> 2 : (m_uiNumPartition >> 1);   
      break;
    case SIZE_NxN:   
      ruiPartIdxRB += ( m_uiNumPartition >> 2 ) * ( uiPartIdx - 1 );   
      break;
    default:
      assert (0);
      break;
  }
}
#endif

Void TComDataCU::deriveLeftRightTopIdxAdi ( UInt& ruiPartIdxLT, UInt& ruiPartIdxRT, UInt uiPartOffset, UInt uiPartDepth )
{
  UInt uiNumPartInWidth = (m_puhWidth[0]/m_pcPic->getMinCUWidth())>>uiPartDepth;
  ruiPartIdxLT = m_uiAbsIdxInLCU + uiPartOffset;
  ruiPartIdxRT = g_auiRasterToZscan[ g_auiZscanToRaster[ ruiPartIdxLT ] + uiNumPartInWidth - 1 ];
}

Void TComDataCU::deriveLeftBottomIdxAdi( UInt& ruiPartIdxLB, UInt uiPartOffset, UInt uiPartDepth )
{
  UInt uiAbsIdx;
  UInt uiMinCuWidth, uiWidthInMinCus;
  
  uiMinCuWidth    = getPic()->getMinCUWidth();
  uiWidthInMinCus = (getWidth(0)/uiMinCuWidth)>>uiPartDepth;
  uiAbsIdx        = getZorderIdxInCU()+uiPartOffset+(m_uiNumPartition>>(uiPartDepth<<1))-1;
  uiAbsIdx        = g_auiZscanToRaster[uiAbsIdx]-(uiWidthInMinCus-1);
  ruiPartIdxLB    = g_auiRasterToZscan[uiAbsIdx];
}

Bool TComDataCU::hasEqualMotion( UInt uiAbsPartIdx, UInt uiCandInterDir, Int* paiCandRefIdx, TComMv* pacCandMv )
{
  AOT  ( getInterDir( uiAbsPartIdx ) == 0 );
  ROTRS( getInterDir( uiAbsPartIdx ) == uiCandInterDir, false );
  for  ( UInt uiRefListIdx = 0; uiRefListIdx < 2; uiRefListIdx++ )
  {
    if ( getInterDir( uiAbsPartIdx ) & ( 1 << uiRefListIdx ) )
    {
      if ( getCUMvField( RefPicList( uiRefListIdx ) )->getRefIdx( uiAbsPartIdx ) != paiCandRefIdx[ uiRefListIdx ] ||
           getCUMvField( RefPicList( uiRefListIdx ) )->getMv    ( uiAbsPartIdx ) != pacCandMv    [ uiRefListIdx ]    )
      {
        return false;
      }
    }
  }
  return true;
}

Bool TComDataCU::hasEqualMotion( UInt uiAbsPartIdx, TComDataCU* pcCandCU, UInt uiCandAbsPartIdx )
{
  assert( getInterDir( uiAbsPartIdx ) != 0 );

  if ( getInterDir( uiAbsPartIdx ) != pcCandCU->getInterDir( uiCandAbsPartIdx ) )
  {
    return false;
  }

  for ( UInt uiRefListIdx = 0; uiRefListIdx < 2; uiRefListIdx++ )
  {
    if ( getInterDir( uiAbsPartIdx ) & ( 1 << uiRefListIdx ) )
    {
      if ( getCUMvField( RefPicList( uiRefListIdx ) )->getMv( uiAbsPartIdx )     != pcCandCU->getCUMvField( RefPicList( uiRefListIdx ) )->getMv( uiCandAbsPartIdx ) || 
        getCUMvField( RefPicList( uiRefListIdx ) )->getRefIdx( uiAbsPartIdx ) != pcCandCU->getCUMvField( RefPicList( uiRefListIdx ) )->getRefIdx( uiCandAbsPartIdx ) )
      {
        return false;
      }
    }
  }

  return true;
}

Bool TComDataCU::avoidMergeCandidate( UInt uiAbsPartIdx, UInt uiPUIdx, UInt uiDepth, UInt uiCandInterDir, Int* paiCandRefIdx, TComMv* pacCandMv )
{
  // uiAbsPartIdx has to be the first index of the current PU
  PartSize ePartSize = getPartitionSize( uiAbsPartIdx );

  if ( ePartSize != SIZE_NxN && ePartSize != SIZE_2Nx2N )
  {
    // we have exactly 2 PUs. Avoid imitating 2Nx2N.
    if ( uiPUIdx == 1 )
    {
      // we would merge the second PU
      UInt uiPUOffset = ( g_auiPUOffset[UInt( ePartSize )] << ( ( getSlice()->getSPS()->getMaxCUDepth() - uiDepth ) << 1 ) ) >> 4;
      UInt uiAbsPartIdxOfFirstPU = uiAbsPartIdx - uiPUOffset;
      if ( hasEqualMotion( uiAbsPartIdxOfFirstPU, uiCandInterDir, paiCandRefIdx, pacCandMv ) )
      {
        // don't merge
        return true;
      }
    }
  }
  if ( ePartSize == SIZE_NxN )
  {
    if ( uiPUIdx == 3 )
    {
      UInt uiPUOffset = ( g_auiPUOffset[UInt( ePartSize )] << ( ( getSlice()->getSPS()->getMaxCUDepth() - uiDepth ) << 1 ) ) >> 4;
      UInt uiPUOffsetIdx0 = uiAbsPartIdx - 3*uiPUOffset;
      UInt uiPUOffsetIdx1 = uiAbsPartIdx - 2*uiPUOffset;
      UInt uiPUOffsetIdx2 = uiAbsPartIdx -   uiPUOffset;

      // avoid imitating 2Nx2N and Nx2N partitioning.
      if ( hasEqualMotion( uiPUOffsetIdx0, this, uiPUOffsetIdx2 ) &&
           hasEqualMotion( uiPUOffsetIdx1, uiCandInterDir, paiCandRefIdx, pacCandMv ) )
      {
        // don't merge
        return true;
      }

      // avoid imitating 2NxN partitioning.
      if ( hasEqualMotion( uiPUOffsetIdx0, this, uiPUOffsetIdx1 ) &&
           hasEqualMotion( uiPUOffsetIdx2, uiCandInterDir, paiCandRefIdx, pacCandMv ) )
      {
        // don't merge
        return true;
      }
    }
  }
  return false;
}

Bool TComDataCU::avoidMergeCandidate( UInt uiAbsPartIdx, UInt uiPUIdx, UInt uiDepth, TComDataCU* pcCandCU, UInt uiCandAbsPartIdx )
{
  // uiAbsPartIdx has to be the first index of the current PU

  if ( !pcCandCU )
  {
    return true;
  }

  PartSize ePartSize = getPartitionSize( uiAbsPartIdx );

  if ( ePartSize != SIZE_NxN && ePartSize != SIZE_2Nx2N )
  {
    // we have exactly 2 PUs. Avoid imitating 2Nx2N.
    if ( uiPUIdx == 1 )
    {
      // we would merge the second PU
      UInt uiPUOffset = ( g_auiPUOffset[UInt( ePartSize )] << ( ( getSlice()->getSPS()->getMaxCUDepth() - uiDepth ) << 1 ) ) >> 4;
      UInt uiAbsPartIdxOfFirstPU = uiAbsPartIdx - uiPUOffset;
      if ( hasEqualMotion( uiAbsPartIdxOfFirstPU, pcCandCU, uiCandAbsPartIdx ) )
      {
        // don't merge
        return true;
      }
    }
  }
  if ( ePartSize == SIZE_NxN )
  {
    if ( uiPUIdx == 3 )
    {
      UInt uiPUOffset = ( g_auiPUOffset[UInt( ePartSize )] << ( ( getSlice()->getSPS()->getMaxCUDepth() - uiDepth ) << 1 ) ) >> 4;
      UInt uiPUOffsetIdx0 = uiAbsPartIdx - 3*uiPUOffset;
      UInt uiPUOffsetIdx1 = uiAbsPartIdx - 2*uiPUOffset;
      UInt uiPUOffsetIdx2 = uiAbsPartIdx -   uiPUOffset;

      // avoid imitating 2Nx2N and Nx2N partitioning.
      if ( hasEqualMotion( uiPUOffsetIdx0, this, uiPUOffsetIdx2 ) &&
           hasEqualMotion( uiPUOffsetIdx1, pcCandCU, uiCandAbsPartIdx ) )
      {
        // don't merge
        return true;
      }

      // avoid imitating 2NxN partitioning.
      if ( hasEqualMotion( uiPUOffsetIdx0, this, uiPUOffsetIdx1 ) &&
           hasEqualMotion( uiPUOffsetIdx2, pcCandCU, uiCandAbsPartIdx ) )
      {
        // don't merge
        return true;
      }
    }
  }
  return false;
}

/** Constructs a list of merging candidates
 * \param uiAbsPartIdx
 * \param uiPUIdx 
 * \param uiDepth
 * \param pcMvFieldNeighbours
 * \param puhInterDirNeighbours
 * \param puiNeighbourCandIdx
 * \returns Void
 */
Void TComDataCU::getInterMergeCandidates( UInt uiAbsPartIdx, UInt uiPUIdx, UInt uiDepth, TComMvField* pcMvFieldNeighbours, UChar* puhInterDirNeighbours, UInt* puiNeighbourCandIdx )
{
  UInt uiAbsPartAddr = m_uiAbsIdxInLCU + uiAbsPartIdx;
  Bool bNoPdmMerge   = ( m_pcSlice->getSPS()->getViewId() == 0 || ( m_pcSlice->getSPS()->getMultiviewMvPredMode() & PDM_USE_FOR_MERGE ) != PDM_USE_FOR_MERGE );
  UInt uiPdmMergePos = ( bNoPdmMerge ? 5 : PDM_MERGE_POS );
  UInt uiLeftAddr    = ( uiPdmMergePos < 1 ? 1 : 0 );
  UInt uiAboveAddr   = ( uiPdmMergePos < 2 ? 2 : 1 );
  UInt uiColocAddr   = ( uiPdmMergePos < 3 ? 3 : 2 );
  UInt uiCorRTAddr   = ( uiPdmMergePos < 4 ? 4 : 3 );
  UInt uiCorBLAddr   = ( uiPdmMergePos < 5 ? 5 : 4 );
  UInt uiPdmAddr     =   uiPdmMergePos;
  
  bool abCandIsInter[ MRG_MAX_NUM_CANDS ];
  for( UInt ui = 0; ui < MRG_MAX_NUM_CANDS; ++ui )
  {
    abCandIsInter[ui] = false;
  }
  
  //===== left =====
  UInt uiLeftPartIdx = 0;
  TComDataCU* pcCULeft = 0;
  pcCULeft  = getPULeft( uiLeftPartIdx, uiAbsPartAddr );
  if( avoidMergeCandidate( uiAbsPartIdx, uiPUIdx, uiDepth, pcCULeft, uiLeftPartIdx ) )
  {
    pcCULeft = NULL;
  }
  if( pcCULeft && !pcCULeft->isIntra( uiLeftPartIdx ) )
  {
    abCandIsInter[uiLeftAddr] = true;
    puiNeighbourCandIdx[uiLeftAddr] = uiLeftAddr+1;
  }
  if( abCandIsInter[uiLeftAddr] )
  {
    // get Inter Dir
    puhInterDirNeighbours[uiLeftAddr] = pcCULeft->getInterDir( uiLeftPartIdx );
    // get Mv from Left
    pcCULeft->getMvField( pcCULeft, uiLeftPartIdx, REF_PIC_LIST_0, pcMvFieldNeighbours[uiLeftAddr<<1] );
    if ( getSlice()->isInterB() )
    {
      pcCULeft->getMvField( pcCULeft, uiLeftPartIdx, REF_PIC_LIST_1, pcMvFieldNeighbours[(uiLeftAddr<<1)+1] );
    }
  }


  //===== top =====
  UInt uiAbovePartIdx = 0;
  TComDataCU* pcCUAbove = 0;
  pcCUAbove = getPUAbove( uiAbovePartIdx, uiAbsPartAddr );
  if( avoidMergeCandidate( uiAbsPartIdx, uiPUIdx, uiDepth, pcCUAbove, uiAbovePartIdx ) )
  {
    pcCUAbove = NULL;
  }
  if ( pcCUAbove && !pcCUAbove->isIntra( uiAbovePartIdx ) )
  {
    abCandIsInter[uiAboveAddr] = true;
    puiNeighbourCandIdx[uiAboveAddr] = uiAboveAddr+1;
  }
  if ( abCandIsInter[uiAboveAddr] )
  {
    // get Inter Dir
    puhInterDirNeighbours[uiAboveAddr] = pcCUAbove->getInterDir( uiAbovePartIdx );
    // get Mv from Above
    pcCUAbove->getMvField( pcCUAbove, uiAbovePartIdx, REF_PIC_LIST_0, pcMvFieldNeighbours[uiAboveAddr<<1] );
    if ( getSlice()->isInterB() )
    {
      pcCUAbove->getMvField( pcCUAbove, uiAbovePartIdx, REF_PIC_LIST_1, pcMvFieldNeighbours[(uiAboveAddr<<1)+1] );
    }
  }


  //===== colocated =====
#if MRG_NEIGH_COL
#if PANASONIC_MRG_TMVP_REFIDX
  Int iRefIdxSkip[2] = {-1, -1};
  TComDataCU* pcTmpCU = NULL;
  UInt uiIdxblk;
  Int iRefIdxLeft[2] = {-1, -1};
  Int iRefIdxAbove[2] = {-1, -1};
  Int iRefIdxCor[2] = {-1, -1};

  UInt uiPUIdxLT = 0;
  UInt uiPUIdxRT  = 0;
  UInt uiPUIdxLB = 0;
  PartSize cCurPS = m_pePartSize[uiAbsPartIdx];
  deriveLeftRightTopIdxGeneral( cCurPS, uiAbsPartIdx, uiPUIdx, uiPUIdxLT, uiPUIdxRT );
  deriveLeftBottomIdxGeneral( cCurPS, uiAbsPartIdx, uiPUIdx, uiPUIdxLB );

  for (int i=0; i<2; i++) 
  {
    RefPicList  eRefPicList = ( i==1 ? REF_PIC_LIST_1 : REF_PIC_LIST_0 );
  
    pcTmpCU = getPULeft(uiIdxblk, uiPUIdxLT);
    iRefIdxLeft[i] = (pcTmpCU != NULL) ? pcTmpCU->getCUMvField(eRefPicList)->getRefIdx(uiIdxblk) : -1;
    pcTmpCU = getPUAbove(uiIdxblk, uiPUIdxLT);
    iRefIdxAbove[i] = (pcTmpCU != NULL) ? pcTmpCU->getCUMvField(eRefPicList)->getRefIdx(uiIdxblk) : -1;
    pcTmpCU = getPUAboveRight( uiIdxblk, uiPUIdxRT );
    if (pcTmpCU == NULL) 
    {
      pcTmpCU = getPUBelowLeft( uiIdxblk, uiPUIdxLB );
    }
    if (pcTmpCU == NULL) 
    {
      pcTmpCU = getPUAboveLeft( uiIdxblk, uiPUIdxLT );
    }
    iRefIdxCor[i] = (pcTmpCU != NULL) ? pcTmpCU->getCUMvField(eRefPicList)->getRefIdx(uiIdxblk) : -1;

    if (iRefIdxLeft[i] == iRefIdxAbove[i] && iRefIdxAbove[i] == iRefIdxCor[i])
    {
      iRefIdxSkip[i] = (iRefIdxLeft[i] == -1) ? 0 : iRefIdxLeft[i];
    }
    else if (iRefIdxLeft[i] == iRefIdxAbove[i])
    {
      iRefIdxSkip[i] = (iRefIdxLeft[i] == -1) ? iRefIdxCor[i] : iRefIdxLeft[i];
    }
    else if (iRefIdxAbove[i] == iRefIdxCor[i])
    {
      iRefIdxSkip[i] = (iRefIdxAbove[i] == -1) ? iRefIdxLeft[i] : iRefIdxAbove[i];
    }
    else if (iRefIdxLeft[i] == iRefIdxCor[i])
    {
      iRefIdxSkip[i] = (iRefIdxLeft[i] == -1) ? iRefIdxAbove[i] : iRefIdxLeft[i];
    }
    else if (iRefIdxLeft[i] == -1)
    {
      iRefIdxSkip[i] = min(iRefIdxAbove[i], iRefIdxCor[i]);
    }
    else if (iRefIdxAbove[i] == -1)
    {
      iRefIdxSkip[i] = min(iRefIdxLeft[i], iRefIdxCor[i]);
    }
    else if (iRefIdxCor[i] == -1)
    {
      iRefIdxSkip[i] = min(iRefIdxLeft[i], iRefIdxAbove[i]);
    }
    else
    {
      iRefIdxSkip[i] = min( min(iRefIdxLeft[i], iRefIdxAbove[i]), iRefIdxCor[i]);
    }
  }
#endif
#if MTK_TMVP_H_MRG
  //>> MTK colocated-RightBottom
  UInt uiPartIdxRB;
  Int uiLCUIdx = getAddr();
  PartSize eCUMode = m_pePartSize[0];

  deriveRightBottomIdx( eCUMode, uiPUIdx, uiPartIdxRB );  

  UInt uiAbsPartIdxTmp = g_auiZscanToRaster[uiPartIdxRB];
  UInt uiNumPartInCUWidth = m_pcPic->getNumPartInWidth();

  TComMv cColMv;
  Int iRefIdx;

  if      ( ( m_pcPic->getCU(m_uiCUAddr)->getCUPelX() + g_auiRasterToPelX[uiAbsPartIdxTmp] + m_pcPic->getMinCUWidth() ) >= m_pcSlice->getSPS()->getWidth() )  // image boundary check
  {
    uiLCUIdx = -1;
  }
  else if ( ( m_pcPic->getCU(m_uiCUAddr)->getCUPelY() + g_auiRasterToPelY[uiAbsPartIdxTmp] + m_pcPic->getMinCUHeight() ) >= m_pcSlice->getSPS()->getHeight() )
  {
    uiLCUIdx = -1;
  }
  else
  {
    if ( ( uiAbsPartIdxTmp % uiNumPartInCUWidth < uiNumPartInCUWidth - 1 ) &&           // is not at the last column of LCU 
         ( uiAbsPartIdxTmp / uiNumPartInCUWidth < m_pcPic->getNumPartInHeight() - 1 ) ) // is not at the last row    of LCU
    {
      uiAbsPartAddr = g_auiRasterToZscan[ uiAbsPartIdxTmp + uiNumPartInCUWidth + 1 ];
      uiLCUIdx = getAddr();
    }
    else if ( uiAbsPartIdxTmp % uiNumPartInCUWidth < uiNumPartInCUWidth - 1 )           // is not at the last column of LCU But is last row of LCU
    {
      uiAbsPartAddr = g_auiRasterToZscan[ (uiAbsPartIdxTmp + uiNumPartInCUWidth + 1) % m_pcPic->getNumPartInCU() ];
      uiLCUIdx = getAddr() + m_pcPic->getFrameWidthInCU();
    }
    else if ( uiAbsPartIdxTmp / uiNumPartInCUWidth < m_pcPic->getNumPartInHeight() - 1 ) // is not at the last row of LCU But is last column of LCU
    {
      uiAbsPartAddr = g_auiRasterToZscan[ uiAbsPartIdxTmp + 1 ];
      uiLCUIdx = getAddr() + 1;
    }
    else //is the right bottom corner of LCU                       
    {
      uiAbsPartAddr = 0;
      uiLCUIdx = getAddr() + m_pcPic->getFrameWidthInCU() + 1;
    }
  }
#if PANASONIC_MRG_TMVP_REFIDX
  iRefIdx = iRefIdxSkip[0];
#else
  iRefIdx = 0; // scaled to 1st ref pic for List0/List1
#endif
  
#if FT_TCTR_MRG
  Bool bExistMV = false;
  UInt uiPartIdxCenter;
  UInt uiCurLCUIdx = getAddr();
  xDeriveCenterIdx( eCUMode, uiPUIdx, uiPartIdxCenter );
  bExistMV = uiLCUIdx >= 0 && xGetColMVP( REF_PIC_LIST_0, uiLCUIdx, uiAbsPartAddr, cColMv, iRefIdx );
  if( bExistMV == false )
  {
    bExistMV = xGetColMVP( REF_PIC_LIST_0, uiCurLCUIdx, uiPartIdxCenter,  cColMv, iRefIdx );
  }
  if( bExistMV )
#else
  if (uiLCUIdx >= 0 && xGetColMVP( REF_PIC_LIST_0, uiLCUIdx, uiAbsPartAddr, cColMv, iRefIdx ) )
#endif
  {
    abCandIsInter[uiColocAddr] = true;
#if PANASONIC_MRG_TMVP_REFIDX
    pcMvFieldNeighbours[uiColocAddr << 1].setMvField( cColMv, iRefIdx );
#else
    pcMvFieldNeighbours[uiColocAddr << 1].setMvField( cColMv, 0 );
#endif
    puiNeighbourCandIdx[uiColocAddr] = uiColocAddr+1;

    if ( getSlice()->isInterB() )
    {       
#if PANASONIC_MRG_TMVP_REFIDX
      iRefIdx = iRefIdxSkip[1];
#endif
#if FT_TCTR_MRG
      bExistMV = uiLCUIdx >= 0 && xGetColMVP( REF_PIC_LIST_1, uiLCUIdx, uiAbsPartAddr, cColMv, iRefIdx);
      if( bExistMV == false )
      {
        bExistMV = xGetColMVP( REF_PIC_LIST_1, uiCurLCUIdx, uiPartIdxCenter,  cColMv, iRefIdx );
      }
      if( bExistMV )
#else
      if (xGetColMVP( REF_PIC_LIST_1, uiLCUIdx, uiAbsPartAddr, cColMv, iRefIdx) )
#endif
  {
#if PANASONIC_MRG_TMVP_REFIDX
        pcMvFieldNeighbours[ ( uiColocAddr << 1 ) + 1 ].setMvField( cColMv, iRefIdx );
#else
        pcMvFieldNeighbours[ ( uiColocAddr << 1 ) + 1 ].setMvField( cColMv, 0 );
#endif
        puhInterDirNeighbours[uiColocAddr] = 3;
      }
      else
      {
        puhInterDirNeighbours[uiColocAddr] = 1;
      }
    }
    else
    {
      puhInterDirNeighbours[uiColocAddr] = 1;
    }
  }
#else //MTK_TMVP_H_MRG
#if FT_TCTR_MRG
    TComMv cMvTCenter[2];
#if PANASONIC_MRG_TMVP_REFIDX
  Int iRefIdx = iRefIdxSkip[0];
  if (xGetCenterCol( uiPUIdx, REF_PIC_LIST_0, iRefIdx, &cMvTCenter[0] ))
#else
    if (xGetCenterCol( uiPUIdx, REF_PIC_LIST_0, 0, &cMvTCenter[0] ))
#endif
    {
      abCandIsInter[uiColocAddr] = true;
#if PANASONIC_MRG_TMVP_REFIDX
    pcMvFieldNeighbours[uiColocAddr << 1].setMvField( cMvTCenter[0], iRefIdx );
#else
      pcMvFieldNeighbours[uiColocAddr << 1].setMvField( cMvTCenter[0], 0 );
#endif
      puiNeighbourCandIdx[uiColocAddr] = uiIdx;

      if ( getSlice()->isInterB() )
      {       
#if PANASONIC_MRG_TMVP_REFIDX
      if ( xGetCenterCol( uiPUIdx, REF_PIC_LIST_1, iRefIdx, &cMvTCenter[1] ) )
#else
        if ( xGetCenterCol( uiPUIdx, REF_PIC_LIST_1, 0, &cMvTCenter[1] ) )
#endif
        {
#if PANASONIC_MRG_TMVP_REFIDX
        pcMvFieldNeighbours[ ( uiColocAddr << 1 ) + 1 ].setMvField( cMvTCenter[1], iRefIdx );
#else
          pcMvFieldNeighbours[ ( uiColocAddr << 1 ) + 1 ].setMvField( cMvTCenter[1], 0 );
#endif
          puhInterDirNeighbours[uiColocAddr] = 3;
        }
        else
        {
          puhInterDirNeighbours[uiColocAddr] = 1;
        }
      }
      else
      {
        puhInterDirNeighbours[uiColocAddr] = 1;
      }
    }
#else //FT_TCTR_MRG
  UInt uiColDir = ( m_pcSlice->isInterB()? m_pcSlice->getColDir() : 0 );
  TComDataCU* pcCUColocated = getCUColocated( RefPicList( uiColDir ) );
  RefPicList eColRefPicList = ( m_pcSlice->isInterB()? RefPicList( 1-uiColDir ) : REF_PIC_LIST_0 );
#if PANASONIC_MERGETEMPORALEXT
  RefPicList eColRefPicList2 = (m_pcSlice->isInterB()? RefPicList(uiColDir) : REF_PIC_LIST_0);
#endif
  if( pcCUColocated && !pcCUColocated->isIntra( uiAbsPartAddr ) &&
    pcCUColocated->getCUMvField( eColRefPicList )->getRefIdx( uiAbsPartAddr ) >= 0 )
  {
    Int iColPOC = pcCUColocated->getSlice()->getPOC();
    Int iColRefPOC = pcCUColocated->getSlice()->getRefPOC( eColRefPicList, pcCUColocated->getCUMvField( eColRefPicList )->getRefIdx( uiAbsPartAddr ) );
    TComMv cColMv = pcCUColocated->getCUMvField( eColRefPicList )->getMv( uiAbsPartAddr );

    Int iCurrPOC = m_pcSlice->getPOC();
    Int iCurrRefPOC = m_pcSlice->getRefPic( REF_PIC_LIST_0, 0 )->getPOC();

    TComMv cMv;
    Int iScale = xGetDistScaleFactor( iCurrPOC, iCurrRefPOC, iColPOC, iColRefPOC );

    if( iScale == 1024 )
    {
      cMv = cColMv;
    }
    else
    {
      cMv = cColMv.scaleMv( iScale );
    }
    clipMv( cMv );
    abCandIsInter[uiColocAddr] = true;
    pcMvFieldNeighbours[uiColocAddr << 1].setMvField( cMv, 0 );
    puiNeighbourCandIdx[uiColocAddr] = uiIdx;
    if ( getSlice()->isInterB() )
    {
      iCurrRefPOC = m_pcSlice->getRefPic( REF_PIC_LIST_1, 0 )->getPOC();
      TComMv cMvB;
      iScale = xGetDistScaleFactor( iCurrPOC, iCurrRefPOC, iColPOC, iColRefPOC );
      if( iScale == 1024 )
      {
        cMvB = cColMv;
      }
      else
      {
        cMvB = cColMv.scaleMv( iScale );
      }
      clipMv( cMvB );
      pcMvFieldNeighbours[ ( uiColocAddr << 1 ) + 1 ].setMvField( cMvB, 0 );
      puhInterDirNeighbours[uiColocAddr] = 3;
    }
    else
    {
      puhInterDirNeighbours[uiColocAddr] = 1;
    }
  }
#if PANASONIC_MERGETEMPORALEXT
  else if( pcCUColocated && !pcCUColocated->isIntra( uiAbsPartAddr ) &&
          pcCUColocated->getCUMvField( eColRefPicList2 )->getRefIdx( uiAbsPartAddr ) >= 0 )
  {
    Int iColPOC = pcCUColocated->getSlice()->getPOC();
    Int iColRefPOC = pcCUColocated->getSlice()->getRefPOC( eColRefPicList2, pcCUColocated->getCUMvField( eColRefPicList2 )->getRefIdx( uiAbsPartAddr ) );
    TComMv cColMv = pcCUColocated->getCUMvField( eColRefPicList2 )->getMv( uiAbsPartAddr );
    
    Int iCurrPOC = m_pcSlice->getPOC();
    Int iCurrRefPOC = m_pcSlice->getRefPic( REF_PIC_LIST_0, 0 )->getPOC();
    
    TComMv cMv;
    Int iScale = xGetDistScaleFactor( iCurrPOC, iCurrRefPOC, iColPOC, iColRefPOC );
    
    if( iScale == 1024 )
    {
      cMv = cColMv;
    }
    else
    {
      cMv = cColMv.scaleMv( iScale );
    }
    clipMv( cMv );
    abCandIsInter[uiColocAddr] = true;
    pcMvFieldNeighbours[uiColocAddr << 1].setMvField( cMv, 0 );
    puiNeighbourCandIdx[uiColocAddr] = uiIdx;
    if ( getSlice()->isInterB() )
    {
      iCurrRefPOC = m_pcSlice->getRefPic( REF_PIC_LIST_1, 0 )->getPOC();
      TComMv cMvB;
      iScale = xGetDistScaleFactor( iCurrPOC, iCurrRefPOC, iColPOC, iColRefPOC );
      if( iScale == 1024 )
      {
        cMvB = cColMv;
      }
      else
      {
        cMvB = cColMv.scaleMv( iScale );
      }
      clipMv( cMvB );
      pcMvFieldNeighbours[ ( uiColocAddr << 1 ) + 1 ].setMvField( cMvB, 0 );
      puhInterDirNeighbours[uiColocAddr] = 3;
    }
    else
    {
      puhInterDirNeighbours[uiColocAddr] = 1;
    }
  }
#endif // PANASONIC_MERGETEMPORALEXT
#endif // FT_TCTR_MRG
#endif // MTK_TMVP_H_MRG 
#endif // MRG_NEIGH_COL

  
  //===== corners =====
#if MRG_NEIGH_COL && PANASONIC_MRG_TMVP_REFIDX
  uiPUIdxLT = 0;
  uiPUIdxRT  = 0;
  uiPUIdxLB = 0;
  cCurPS = m_pePartSize[uiAbsPartIdx];
#else
  UInt uiPUIdxLT = 0;
  UInt uiPUIdxRT  = 0;
  UInt uiPUIdxLB = 0;
  PartSize cCurPS = m_pePartSize[uiAbsPartIdx];
#endif

  deriveLeftRightTopIdxGeneral( cCurPS, uiAbsPartIdx, uiPUIdx, uiPUIdxLT, uiPUIdxRT );
  deriveLeftBottomIdxGeneral( cCurPS, uiAbsPartIdx, uiPUIdx, uiPUIdxLB );
  
  TComDataCU* pcCorner = 0;
  UInt uiCornerIdx = 0;

  Bool bRTCornerCand = false;
  Bool bLBCornerCand = false;
  for( UInt uiIter = 0; uiIter < 2; uiIter++ )
  {
    if( !bRTCornerCand )
    {
      pcCorner = getPUAboveRight( uiCornerIdx, uiPUIdxRT );
      xCheckCornerCand( pcCorner, uiCornerIdx, uiIter, bRTCornerCand );
      if( avoidMergeCandidate( uiAbsPartIdx, uiPUIdx, uiDepth, pcCorner, uiCornerIdx ) )
      {
        bRTCornerCand = false;
      }
      if( bRTCornerCand )
      {
        abCandIsInter[uiCorRTAddr] = true;
        puiNeighbourCandIdx[uiCorRTAddr] = uiCorRTAddr+1;
        // get Inter Dir
        puhInterDirNeighbours[uiCorRTAddr] = pcCorner->getInterDir( uiCornerIdx );
        // get Mv from Left
        pcCorner->getMvField( pcCorner, uiCornerIdx, REF_PIC_LIST_0, pcMvFieldNeighbours[uiCorRTAddr << 1] );
        if ( getSlice()->isInterB() )
        {
          pcCorner->getMvField( pcCorner, uiCornerIdx, REF_PIC_LIST_1, pcMvFieldNeighbours[( uiCorRTAddr << 1 ) + 1] );
        }
      }
    }
    if( !bLBCornerCand )
    {
      pcCorner = getPUBelowLeft( uiCornerIdx, uiPUIdxLB );
      xCheckCornerCand( pcCorner, uiCornerIdx, uiIter, bLBCornerCand );
      if( avoidMergeCandidate( uiAbsPartIdx, uiPUIdx, uiDepth, pcCorner, uiCornerIdx ) )
      {
        bLBCornerCand = false;
      }
      if( bLBCornerCand )
      {
        uiCorBLAddr = 4;
        abCandIsInter[uiCorBLAddr] = true;
        puiNeighbourCandIdx[uiCorBLAddr] = uiCorBLAddr+1;
        // get Inter Dir
        puhInterDirNeighbours[uiCorBLAddr] = pcCorner->getInterDir( uiCornerIdx );
        // get Mv from Left
        pcCorner->getMvField( pcCorner, uiCornerIdx, REF_PIC_LIST_0, pcMvFieldNeighbours[uiCorBLAddr << 1] );
        if ( getSlice()->isInterB() )
        {
          pcCorner->getMvField( pcCorner, uiCornerIdx, REF_PIC_LIST_1, pcMvFieldNeighbours[( uiCorBLAddr << 1 ) + 1] );
        }
      }
    }
  }


  //===== add merge with predicted depth maps =====
  TComMv  acPdmMv       [2];
  Int     aiPdmRefIdx   [2] = {-1,-1};
  Int     iPdmInterDir      = getPdmMergeCandidate( uiPUIdx, aiPdmRefIdx, acPdmMv );
  if( iPdmInterDir )
  {
    if( avoidMergeCandidate( uiAbsPartIdx, uiPUIdx, uiDepth, iPdmInterDir, aiPdmRefIdx, acPdmMv ) )
    {
      iPdmInterDir = 0;
    }
  }
  if( iPdmInterDir )
  {
    abCandIsInter        [ uiPdmAddr ] = true;
    puiNeighbourCandIdx  [ uiPdmAddr ] = uiPdmAddr + 1;
    puhInterDirNeighbours[ uiPdmAddr ] = iPdmInterDir;
    if( ( iPdmInterDir & 1 ) == 1 )
    {
      pcMvFieldNeighbours[ uiPdmAddr<<1    ].setMvField( acPdmMv[ 0 ], aiPdmRefIdx[ 0 ] );
    }
    if( ( iPdmInterDir & 2 ) == 2 )
    {
      pcMvFieldNeighbours[(uiPdmAddr<<1)+1 ].setMvField( acPdmMv[ 1 ], aiPdmRefIdx[ 1 ] );
    }
  }


  //===== remove duplicates =====
  for( UInt uiOuter = 0; uiOuter < MRG_MAX_NUM_CANDS; uiOuter++ )
  {
    if( abCandIsInter[ uiOuter ] && ( puiNeighbourCandIdx[uiOuter] == uiOuter + 1 ) )
    {
      for( UInt uiIter = uiOuter + 1; uiIter < MRG_MAX_NUM_CANDS; uiIter++ )
      {
        if( abCandIsInter[ uiIter ] )
        {
          UInt uiMvFieldNeighIdxCurr = uiOuter << 1;
          UInt uiMvFieldNeighIdxComp = uiIter << 1;
          if( getSlice()->isInterB() )
          {
            if( puhInterDirNeighbours[ uiOuter ] == 3 )
            {
              if( pcMvFieldNeighbours[ uiMvFieldNeighIdxCurr ].getRefIdx() == pcMvFieldNeighbours[ uiMvFieldNeighIdxComp ].getRefIdx() && pcMvFieldNeighbours[ uiMvFieldNeighIdxCurr + 1 ].getRefIdx() == pcMvFieldNeighbours[ uiMvFieldNeighIdxComp + 1 ].getRefIdx() &&
                pcMvFieldNeighbours[ uiMvFieldNeighIdxCurr ].getMv() == pcMvFieldNeighbours[ uiMvFieldNeighIdxComp ].getMv() && pcMvFieldNeighbours[ uiMvFieldNeighIdxCurr + 1 ].getMv() == pcMvFieldNeighbours[ uiMvFieldNeighIdxComp + 1 ].getMv() &&
                puhInterDirNeighbours[ uiOuter ] == puhInterDirNeighbours[ uiIter ] )
              {
                puiNeighbourCandIdx[ uiIter ] = puiNeighbourCandIdx[ uiOuter ];
                abCandIsInter[ uiIter ] = false;
              }
            }
            else if( puhInterDirNeighbours[ uiOuter ] == 2 )
            {
              if( pcMvFieldNeighbours[ uiMvFieldNeighIdxCurr + 1 ].getRefIdx() == pcMvFieldNeighbours[ uiMvFieldNeighIdxComp + 1 ].getRefIdx() &&
                pcMvFieldNeighbours[ uiMvFieldNeighIdxCurr + 1 ].getMv() == pcMvFieldNeighbours[ uiMvFieldNeighIdxComp + 1 ].getMv() &&
                puhInterDirNeighbours[ uiOuter ] == puhInterDirNeighbours[ uiIter ] )
              {
                puiNeighbourCandIdx[ uiIter ] = puiNeighbourCandIdx[ uiOuter ];
                abCandIsInter[ uiIter ] = false;
              }
            }
            else
            {
              if( pcMvFieldNeighbours[ uiMvFieldNeighIdxCurr ].getRefIdx() == pcMvFieldNeighbours[ uiMvFieldNeighIdxComp ].getRefIdx() &&
                pcMvFieldNeighbours[ uiMvFieldNeighIdxCurr ].getMv() == pcMvFieldNeighbours[ uiMvFieldNeighIdxComp ].getMv() &&
                puhInterDirNeighbours[ uiOuter ] == puhInterDirNeighbours[ uiIter ] )
              {
                puiNeighbourCandIdx[ uiIter ] = puiNeighbourCandIdx[ uiOuter ];
                abCandIsInter[ uiIter ] = false;
              }
            }
          }
          else
          {
            if( pcMvFieldNeighbours[ uiMvFieldNeighIdxCurr ].getRefIdx() == pcMvFieldNeighbours[ uiMvFieldNeighIdxComp ].getRefIdx() && 
                pcMvFieldNeighbours[ uiMvFieldNeighIdxCurr ].getMv() == pcMvFieldNeighbours[ uiMvFieldNeighIdxComp ].getMv() )
            {
              assert( puhInterDirNeighbours[ uiOuter ] == puhInterDirNeighbours[ uiIter ] );
              puiNeighbourCandIdx[ uiIter ] = puiNeighbourCandIdx[ uiOuter ];
              abCandIsInter[ uiIter ] = false;
            }
          }
        }
      }
    }
  }
#if AVOID_ZERO_MERGE_CANDIDATE
  // if all merge candidate
  int i;
  for ( i=0; i<MRG_MAX_NUM_CANDS; i++ )
  {
    if ( abCandIsInter[i] )
    {
      break;
    }
  }
  if ( i==MRG_MAX_NUM_CANDS ) // no merge candidate
  {
    abCandIsInter[0] = true;
    puiNeighbourCandIdx[0] = 1;
    puhInterDirNeighbours[0] = 1;
    pcMvFieldNeighbours[0].setMvField( TComMv(0, 0), 0 );
    if ( getSlice()->isInterB() )
    {
      puhInterDirNeighbours[0] = 3;
      pcMvFieldNeighbours[1].setMvField( TComMv(0, 0), 0 );
    }
  }
#endif
}

Void TComDataCU::xCheckCornerCand( TComDataCU* pcCorner, UInt uiCornerPUIdx, UInt uiIter, Bool& rbValidCand )
{
  if( uiIter == 0 )
  {
    if( pcCorner && !pcCorner->isIntra( uiCornerPUIdx ) )
    {
      rbValidCand = true;
      if( getSlice()->isInterB() )
      {
        if ( pcCorner->getInterDir( uiCornerPUIdx ) == 1 )
        {
          if( pcCorner->getCUMvField(REF_PIC_LIST_0)->getRefIdx( uiCornerPUIdx ) != 0 )
          {
            rbValidCand = false;
          }
        }
        else if ( pcCorner->getInterDir( uiCornerPUIdx ) == 2 )
        {
          if( pcCorner->getCUMvField(REF_PIC_LIST_1)->getRefIdx( uiCornerPUIdx ) != 0 )
          {
            rbValidCand = false;
          }
        }
        else
        {
          if( pcCorner->getCUMvField(REF_PIC_LIST_0)->getRefIdx( uiCornerPUIdx ) != 0 || pcCorner->getCUMvField(REF_PIC_LIST_1)->getRefIdx( uiCornerPUIdx ) != 0 )
          {
            rbValidCand = false;
          }
        }
      }
      else if( pcCorner->getCUMvField(REF_PIC_LIST_0)->getRefIdx( uiCornerPUIdx ) != 0 )
      {
        rbValidCand = false;
      }
    }
  }
  else
  {
    if( pcCorner && !pcCorner->isIntra( uiCornerPUIdx ) )
    {
      rbValidCand = true;
      if( getSlice()->isInterB() )
      {
        if ( pcCorner->getInterDir( uiCornerPUIdx ) == 1 )
        {
          if( pcCorner->getCUMvField(REF_PIC_LIST_0)->getRefIdx( uiCornerPUIdx ) < 0 )
          {
            rbValidCand = false;
          }
        }
        else if ( pcCorner->getInterDir( uiCornerPUIdx ) == 2 )
        {
          if( pcCorner->getCUMvField(REF_PIC_LIST_1)->getRefIdx( uiCornerPUIdx ) < 0 )
          {
            rbValidCand = false;
          }
        }
        else
        {
          if( pcCorner->getCUMvField(REF_PIC_LIST_0)->getRefIdx( uiCornerPUIdx ) < 0 || pcCorner->getCUMvField(REF_PIC_LIST_1)->getRefIdx( uiCornerPUIdx ) < 0 )
          {
            rbValidCand = false;
          }
        }
      }
      else if( pcCorner->getCUMvField(REF_PIC_LIST_0)->getRefIdx( uiCornerPUIdx ) < 0 )
      {
        rbValidCand = false;
      }
    }
  }
}

Void TComDataCU::setNeighbourCandIdxSubParts( UInt uiCandIdx, UChar uhNumCands, UInt uiAbsPartIdx, UInt uiPUIdx, UInt uiDepth )
{
  setSubPartUChar( uhNumCands, m_apuhNeighbourCandIdx[uiCandIdx], uiAbsPartIdx, uiDepth, uiPUIdx );
}

AMVP_MODE TComDataCU::getAMVPMode(UInt uiIdx)
{
  return m_pcSlice->getSPS()->getAMVPMode(m_puhDepth[uiIdx]);
}

/** Constructs a list of candidates for AMVP
 * \param uiPartIdx
 * \param uiPartAddr 
 * \param eRefPicList
 * \param iRefIdx
 * \param pInfo
 * \param puiNeighbourCandIdx
 * \returns Void
 */
Void TComDataCU::fillMvpCand ( UInt uiPartIdx, UInt uiPartAddr, RefPicList eRefPicList, Int iRefIdx, AMVPInfo* pInfo )
{
  PartSize eCUMode = m_pePartSize[0];
  
  TComMv cMvPred;
  
  pInfo->iN = 0;
#if !DCM_SIMPLIFIED_MVP
  UInt uiIdx;
#endif
  
  if (iRefIdx < 0)
  {
    return;
  }

  Int iLeftIdx = -1;

#if ( PDM_AMVP_POS == 0 )
  // get inter-view mv predictor (at position 0)
  TComMv  cPdmMvPred;
  if( getPdmMvPred( uiPartIdx, eRefPicList, iRefIdx, cPdmMvPred ) )
  {
    clipMv( cPdmMvPred );
    pInfo->m_acMvCand[ pInfo->iN++ ] = cPdmMvPred;
  }
#endif

#if !DCM_SIMPLIFIED_MVP
  pInfo->m_acMvCand[pInfo->iN++] = cMvPred;   //dummy mv
#endif
  
  //-- Get Spatial MV
  UInt uiPartIdxLT, uiPartIdxRT, uiPartIdxLB;
#if (DCM_SIMPLIFIED_MVP && !TI_AMVP_SMVP_SIMPLIFIED) || (!DCM_SIMPLIFIED_MVP) || MTK_TMVP_H_AMVP
  UInt uiNumPartInCUWidth = m_pcPic->getNumPartInWidth();
#endif
  Bool bAdded = false;
#if !DCM_SIMPLIFIED_MVP
  Int iLeftMvIdx = -1;
  Int iAboveMvIdx = -1;
  Int iCornerMvIdx = -1;
#endif
  
  deriveLeftRightTopIdx( eCUMode, uiPartIdx, uiPartIdxLT, uiPartIdxRT );
  deriveLeftBottomIdx( eCUMode, uiPartIdx, uiPartIdxLB );
  
#if DCM_SIMPLIFIED_MVP
  // Left predictor search
  bAdded = xAddMVPCand( pInfo, eRefPicList, iRefIdx, uiPartIdxLB, MD_BELOW_LEFT);
#if TI_AMVP_SMVP_SIMPLIFIED
  if (!bAdded) 
  {
    bAdded = xAddMVPCand( pInfo, eRefPicList, iRefIdx, uiPartIdxLB, MD_LEFT );
  }
#else
  for ( Int idx = (Int)g_auiZscanToRaster[uiPartIdxLB]; !bAdded && idx >= (Int)g_auiZscanToRaster[uiPartIdxLT]; idx-= uiNumPartInCUWidth )
  {
    bAdded = xAddMVPCand( pInfo, eRefPicList, iRefIdx, g_auiRasterToZscan[idx], MD_LEFT );
  }
#endif
  
#if MTK_AMVP_SMVP_DERIVATION
  if(!bAdded)
  {
    bAdded = xAddMVPCandOrder( pInfo, eRefPicList, iRefIdx, uiPartIdxLB, MD_BELOW_LEFT);
#if TI_AMVP_SMVP_SIMPLIFIED
    if (!bAdded) 
    {
      bAdded = xAddMVPCandOrder( pInfo, eRefPicList, iRefIdx, uiPartIdxLB, MD_LEFT );
    }
#else
    for ( Int idx = (Int)g_auiZscanToRaster[uiPartIdxLB]; !bAdded && idx >= (Int)g_auiZscanToRaster[uiPartIdxLT]; idx-= uiNumPartInCUWidth )
    {
      bAdded = xAddMVPCandOrder( pInfo, eRefPicList, iRefIdx, g_auiRasterToZscan[idx], MD_LEFT );
    }
#endif
  }
#endif
  if( bAdded )
  {
    iLeftIdx = pInfo->iN - 1;
  }
  


#if ( PDM_AMVP_POS == 1 )
  // get inter-view mv predictor (at position 1)
  TComMv  cPdmMvPred;
  if( getPdmMvPred( uiPartIdx, eRefPicList, iRefIdx, cPdmMvPred ) )
  {
    clipMv( cPdmMvPred );
    pInfo->m_acMvCand[ pInfo->iN++ ] = cPdmMvPred;
  }
#endif


  
  // Above predictor search
  bAdded = xAddMVPCand( pInfo, eRefPicList, iRefIdx, uiPartIdxRT, MD_ABOVE_RIGHT);
  if (bAdded && iLeftIdx >= 0 && pInfo->m_acMvCand[iLeftIdx] == pInfo->m_acMvCand[pInfo->iN-1])
  {
    pInfo->iN--; //remove duplicate entries
    bAdded = false;
  }
#if TI_AMVP_SMVP_SIMPLIFIED
  if (!bAdded) 
  {
    bAdded = xAddMVPCand( pInfo, eRefPicList, iRefIdx, uiPartIdxRT, MD_ABOVE);
  }
  if (bAdded && iLeftIdx >= 0 && pInfo->m_acMvCand[iLeftIdx] == pInfo->m_acMvCand[pInfo->iN-1])
  {
    pInfo->iN--; //remove duplicate entries
    bAdded = false;
  }
#else  
  for ( Int idx = (Int)g_auiZscanToRaster[uiPartIdxRT]; !bAdded && idx >= (Int)g_auiZscanToRaster[uiPartIdxLT]; idx-- )
  {
    bAdded = xAddMVPCand( pInfo, eRefPicList, iRefIdx, g_auiRasterToZscan[idx], MD_ABOVE);
    if (bAdded && iLeftIdx >= 0 && pInfo->m_acMvCand[iLeftIdx] == pInfo->m_acMvCand[pInfo->iN-1])
    {
      pInfo->iN--; //remove duplicate entries
      bAdded = false;
    }
  }
#endif
  if(!bAdded)
  {
    bAdded = xAddMVPCand( pInfo, eRefPicList, iRefIdx, uiPartIdxLT, MD_ABOVE_LEFT);
    if (bAdded && iLeftIdx >= 0 && pInfo->m_acMvCand[iLeftIdx] == pInfo->m_acMvCand[pInfo->iN-1])
    {
      pInfo->iN--; //remove duplicate entries
    }
  }
#if MTK_AMVP_SMVP_DERIVATION
  if(!bAdded)
  {
    bAdded = xAddMVPCandOrder( pInfo, eRefPicList, iRefIdx, uiPartIdxRT, MD_ABOVE_RIGHT);
    if (bAdded && iLeftIdx >= 0 && pInfo->m_acMvCand[iLeftIdx] == pInfo->m_acMvCand[pInfo->iN-1])
    {
      pInfo->iN--; //remove duplicate entries
      bAdded = false;
    }
#if TI_AMVP_SMVP_SIMPLIFIED
    if (!bAdded) 
    {
      bAdded = xAddMVPCandOrder( pInfo, eRefPicList, iRefIdx, uiPartIdxRT, MD_ABOVE);
    }
    if (bAdded && iLeftIdx >= 0 && pInfo->m_acMvCand[iLeftIdx] == pInfo->m_acMvCand[pInfo->iN-1])
    {
      pInfo->iN--; //remove duplicate entries
      bAdded = false;
    }
#else
    for ( Int idx = (Int)g_auiZscanToRaster[uiPartIdxRT]; !bAdded && idx >= (Int)g_auiZscanToRaster[uiPartIdxLT]; idx-- )
    {
      bAdded = xAddMVPCandOrder( pInfo, eRefPicList, iRefIdx, g_auiRasterToZscan[idx], MD_ABOVE);
      if (bAdded && iLeftIdx >= 0 && pInfo->m_acMvCand[iLeftIdx] == pInfo->m_acMvCand[pInfo->iN-1])
      {
        pInfo->iN--; //remove duplicate entries
        bAdded = false;
      }
    }
#endif
    if(!bAdded)
    {
      bAdded = xAddMVPCandOrder( pInfo, eRefPicList, iRefIdx, uiPartIdxLT, MD_ABOVE_LEFT);
      if (bAdded && iLeftIdx >= 0 && pInfo->m_acMvCand[iLeftIdx] == pInfo->m_acMvCand[pInfo->iN-1])
      {
        pInfo->iN--; //remove duplicate entries
      }
    }
  }
#endif
#else  
  //Left
  for ( uiIdx = g_auiZscanToRaster[uiPartIdxLT]; uiIdx <= g_auiZscanToRaster[uiPartIdxLB]; uiIdx+= uiNumPartInCUWidth )
  {
    bAdded = xAddMVPCand( pInfo, eRefPicList, iRefIdx, g_auiRasterToZscan[uiIdx], MD_LEFT );
    if (bAdded && iLeftMvIdx < 0)
    {
      iLeftMvIdx = pInfo->iN-1;
    }
    if (bAdded) break;
  }
  
  bAdded = false;
  //Above
  for ( uiIdx = g_auiZscanToRaster[uiPartIdxLT]; uiIdx <= g_auiZscanToRaster[uiPartIdxRT]; uiIdx++ )
  {
    bAdded = xAddMVPCand( pInfo, eRefPicList, iRefIdx, g_auiRasterToZscan[uiIdx], MD_ABOVE);
    if (bAdded && iAboveMvIdx < 0)
    {
      iAboveMvIdx = pInfo->iN-1;
    }
    if (bAdded) break;
  }
  
  bAdded = false;
  //Above Right
  bAdded = xAddMVPCand( pInfo, eRefPicList, iRefIdx, uiPartIdxRT, MD_ABOVE_RIGHT);
  if (bAdded && iCornerMvIdx < 0)
  {
    iCornerMvIdx = pInfo->iN-1;
  }
  
  //Below Left
  if (!bAdded)
  {
    bAdded = xAddMVPCand( pInfo, eRefPicList, iRefIdx, uiPartIdxLB, MD_BELOW_LEFT);
  }
  if (bAdded && iCornerMvIdx < 0)
  {
    iCornerMvIdx = pInfo->iN-1;
  }
  
  //Above Left
  if (!bAdded)
  {
    bAdded = xAddMVPCand( pInfo, eRefPicList, iRefIdx, uiPartIdxLT, MD_ABOVE_LEFT);
  }
  
  if (bAdded && iCornerMvIdx < 0)
  {
    iCornerMvIdx = pInfo->iN-1;
  }
  
  assert(iLeftMvIdx!=0 && iAboveMvIdx!=0 && iCornerMvIdx!=0);
  
  if (iLeftMvIdx < 0 && iAboveMvIdx < 0 && iCornerMvIdx < 0)
  {
    //done --> already zero Mv
  }
  else if ( (iLeftMvIdx > 0 && iAboveMvIdx > 0 && iCornerMvIdx > 0) || iLeftMvIdx*iAboveMvIdx*iCornerMvIdx < 0)
  {
    TComMv cLeftMv, cAboveMv, cCornerMv;
    
    if (iLeftMvIdx > 0)
    {
      cLeftMv = pInfo->m_acMvCand[iLeftMvIdx];
    }
    if (iAboveMvIdx > 0)
    {
      cAboveMv = pInfo->m_acMvCand[iAboveMvIdx];
    }
    if (iCornerMvIdx > 0)
    {
      cCornerMv = pInfo->m_acMvCand[iCornerMvIdx];
    }
    pInfo->m_acMvCand[0].setHor ( Median (cLeftMv.getHor(), cAboveMv.getHor(), cCornerMv.getHor()) );
    pInfo->m_acMvCand[0].setVer ( Median (cLeftMv.getVer(), cAboveMv.getVer(), cCornerMv.getVer()) );
  }
  else //only one is available among three candidates
  {
    if (iLeftMvIdx > 0)
    {
      pInfo->m_acMvCand[0] = pInfo->m_acMvCand[iLeftMvIdx];
    }
    else if (iAboveMvIdx > 0)
    {
      pInfo->m_acMvCand[0] = pInfo->m_acMvCand[iAboveMvIdx];
    }
    else if (iCornerMvIdx > 0)
    {
      pInfo->m_acMvCand[0] = pInfo->m_acMvCand[iCornerMvIdx];
    }
    else
    {
      assert(0);
    }
  }
  
  clipMv(pInfo->m_acMvCand[0]);
  TComMv cTempMv;
  if ( ( ( ((eCUMode == SIZE_2NxN)) && uiPartIdx == 1 ) ||
        ( ((eCUMode == SIZE_Nx2N)) && uiPartIdx == 0 ) )
      && iLeftMvIdx > 0 )
  {
    cTempMv = pInfo->m_acMvCand[0];
    pInfo->m_acMvCand[0] = pInfo->m_acMvCand[iLeftMvIdx];
    pInfo->m_acMvCand[iLeftMvIdx] = cTempMv;
  }
#endif
  
  if (getAMVPMode(uiPartAddr) == AM_NONE)  //Should be optimized later for special cases
  {
    assert(pInfo->iN > 0);
    pInfo->iN = 1;
    return;
  }



#if ( PDM_AMVP_POS == 2 )
  // get inter-view mv predictor (at position 2)
  TComMv  cPdmMvPred;
  if( getPdmMvPred( uiPartIdx, eRefPicList, iRefIdx, cPdmMvPred ) )
  {
    clipMv( cPdmMvPred );
    pInfo->m_acMvCand[ pInfo->iN++ ] = cPdmMvPred;
  }
#endif


  // Get Temporal Motion Predictor
#if AMVP_NEIGH_COL
#if MTK_TMVP_H_AMVP
  int iRefIdx_Col = iRefIdx;
  TComMv cColMv;
  UInt uiPartIdxRB;
  UInt uiAbsPartIdx;  
  UInt uiAbsPartAddr;
  int uiLCUIdx = getAddr();

  deriveRightBottomIdx( eCUMode, uiPartIdx, uiPartIdxRB );
  uiAbsPartAddr = m_uiAbsIdxInLCU + uiPartAddr;

  //----  co-located RightBottom Temporal Predictor (H) ---//
  uiAbsPartIdx = g_auiZscanToRaster[uiPartIdxRB];
  if ( ( m_pcPic->getCU(m_uiCUAddr)->getCUPelX() + g_auiRasterToPelX[uiAbsPartIdx] + m_pcPic->getMinCUWidth() ) >= m_pcSlice->getSPS()->getWidth() )  // image boundary check
  {
    uiLCUIdx = -1;
  }
  else if ( ( m_pcPic->getCU(m_uiCUAddr)->getCUPelY() + g_auiRasterToPelY[uiAbsPartIdx] + m_pcPic->getMinCUHeight() ) >= m_pcSlice->getSPS()->getHeight() )
  {
    uiLCUIdx = -1;
  }
  else
  {
    if ( ( uiAbsPartIdx % uiNumPartInCUWidth < uiNumPartInCUWidth - 1 ) &&           // is not at the last column of LCU 
      ( uiAbsPartIdx / uiNumPartInCUWidth < m_pcPic->getNumPartInHeight() - 1 ) ) // is not at the last row    of LCU
    {
      uiAbsPartAddr = g_auiRasterToZscan[ uiAbsPartIdx + uiNumPartInCUWidth + 1 ];
      uiLCUIdx = getAddr();
    }
    else if ( uiAbsPartIdx % uiNumPartInCUWidth < uiNumPartInCUWidth - 1 )           // is not at the last column of LCU But is last row of LCU
    {
      uiAbsPartAddr = g_auiRasterToZscan[ (uiAbsPartIdx + uiNumPartInCUWidth + 1) % m_pcPic->getNumPartInCU() ];
      uiLCUIdx = getAddr() + m_pcPic->getFrameWidthInCU();
    }
    else if ( uiAbsPartIdx / uiNumPartInCUWidth < m_pcPic->getNumPartInHeight() - 1 ) // is not at the last row of LCU But is last column of LCU
    {
      uiAbsPartAddr = g_auiRasterToZscan[ uiAbsPartIdx + 1 ];
      uiLCUIdx = getAddr() + 1;
    }
    else //is the right bottom corner of LCU                       
    {
      uiAbsPartAddr = 0;
      uiLCUIdx = getAddr() + m_pcPic->getFrameWidthInCU() + 1;
    }
  }
  if ( uiLCUIdx >= 0 && xGetColMVP( eRefPicList, uiLCUIdx, uiAbsPartAddr, cColMv, iRefIdx_Col ) )
  {
    pInfo->m_acMvCand[pInfo->iN++] = cColMv;
  }
#if FT_TCTR_AMVP
  else 
  {
    UInt uiPartIdxCenter;
    UInt uiCurLCUIdx = getAddr();
    xDeriveCenterIdx( eCUMode, uiPartIdx, uiPartIdxCenter );
    if (xGetColMVP( eRefPicList, uiCurLCUIdx, uiPartIdxCenter,  cColMv, iRefIdx_Col ))
  {
      pInfo->m_acMvCand[pInfo->iN++] = cColMv;
    }
  }
#endif
  //----  co-located RightBottom Temporal Predictor  ---//

#else //MTK_TMVP_H_AMVP
#if FT_TCTR_AMVP
    TComMv cMvTCenter;
    if (xGetCenterCol( uiPartIdx, eRefPicList, iRefIdx, &cMvTCenter ))
    {
      for (int i = pInfo->iN-1; i >= 0; i--)
      {
        pInfo->m_acMvCand[i+1] = pInfo->m_acMvCand[i];
      }
      pInfo->m_acMvCand[0] = cMvTCenter;
      pInfo->iN++;
    }
#else //FT_TCTR_AMVP
  UInt uiAbsPartAddr = m_uiAbsIdxInLCU + uiPartAddr;
  
  UInt uiColDir = (m_pcSlice->isInterB()? m_pcSlice->getColDir() : 0);
  
  TComDataCU* pcCUColocated = getCUColocated(RefPicList(uiColDir));
  
  RefPicList eColRefPicList = (m_pcSlice->isInterB()? RefPicList(1-uiColDir) : REF_PIC_LIST_0);
#if PANASONIC_AMVPTEMPORALEXT
  RefPicList eColRefPicList2 = (m_pcSlice->isInterB()? RefPicList(uiColDir) : REF_PIC_LIST_0);
#endif
  
  if ( pcCUColocated && !pcCUColocated->isIntra(uiAbsPartAddr) &&
      pcCUColocated->getCUMvField(eColRefPicList)->getRefIdx(uiAbsPartAddr) >= 0 )
  {
    Int iColPOC = pcCUColocated->getSlice()->getPOC();
    Int iColRefPOC = pcCUColocated->getSlice()->getRefPOC(eColRefPicList, pcCUColocated->getCUMvField(eColRefPicList)->getRefIdx(uiAbsPartAddr));
    TComMv cColMv = pcCUColocated->getCUMvField(eColRefPicList)->getMv(uiAbsPartAddr);
    
    Int iCurrPOC = m_pcSlice->getPOC();
    Int iCurrRefPOC = m_pcSlice->getRefPic(eRefPicList, iRefIdx)->getPOC();
    
    TComMv cMv;
    Int iScale = xGetDistScaleFactor(iCurrPOC, iCurrRefPOC, iColPOC, iColRefPOC);
    
    if (iScale == 1024)
    {
      cMv = cColMv;
    }
    else
    {
      cMv = cColMv.scaleMv( iScale );
    }
    
    clipMv(cMv);
    
    pInfo->m_acMvCand[pInfo->iN++] = cMv ;
  }
#if PANASONIC_AMVPTEMPORALEXT
  else if ( pcCUColocated && !pcCUColocated->isIntra(uiAbsPartAddr) &&
           pcCUColocated->getCUMvField(eColRefPicList2)->getRefIdx(uiAbsPartAddr) >= 0 )
  {
    Int iColPOC = pcCUColocated->getSlice()->getPOC();
    Int iColRefPOC = pcCUColocated->getSlice()->getRefPOC(eColRefPicList2, pcCUColocated->getCUMvField(eColRefPicList2)->getRefIdx(uiAbsPartAddr));
    TComMv cColMv = pcCUColocated->getCUMvField(eColRefPicList2)->getMv(uiAbsPartAddr);
    
    Int iCurrPOC = m_pcSlice->getPOC();
    Int iCurrRefPOC = m_pcSlice->getRefPic(eRefPicList, iRefIdx)->getPOC();
    
    TComMv cMv;
    Int iScale = xGetDistScaleFactor(iCurrPOC, iCurrRefPOC, iColPOC, iColRefPOC);
    
    if (iScale == 1024)
    {
      cMv = cColMv;
    }
    else
    {
      cMv = cColMv.scaleMv( iScale );
    }
    
    clipMv(cMv);
    
    pInfo->m_acMvCand[pInfo->iN++] = cMv ;
  }
#endif // PANASONIC_AMVPTEMPORALEXT
#endif // FT_TCTR_AMVP
#endif // MTK_TMVP_H_AMVP
#endif // AMVP_NEIGH_COL


#if ( PDM_AMVP_POS == 3 )
  // get inter-view mv predictor (at position 3)
  TComMv  cPdmMvPred;
  if( getPdmMvPred( uiPartIdx, eRefPicList, iRefIdx, cPdmMvPred ) )
  {
    clipMv( cPdmMvPred );
    pInfo->m_acMvCand[ pInfo->iN++ ] = cPdmMvPred;
  }
#endif


  // Check No MV Candidate
  xUniqueMVPCand( pInfo );
  return ;
}

#if DCM_SIMPLIFIED_MVP==0
Bool TComDataCU::clearMVPCand( TComMv cMvd, AMVPInfo* pInfo )
{
  // only works for multiple candidates
  if (pInfo->iN <= 1)
  {
    return false;
  }
  
  // only works for non-zero mvd case
  if (cMvd.getHor() == 0 && cMvd.getVer() == 0)
  {
    return false;
  }
  
  TComMv  acMv[ AMVP_MAX_NUM_CANDS ];
  Int aiValid [ AMVP_MAX_NUM_CANDS ];
  
  Int iNTmp, i, j;
  
  for ( i=0; i<pInfo->iN; i++ )
  {
    aiValid[i] = 1;
  }
  
  for ( i=0; i<pInfo->iN; i++ )
  {
    TComMv cMvCand = pInfo->m_acMvCand[i] + cMvd;
    
    UInt uiBestBits = xGetMvdBits(cMvd);
    for ( j=0; j<pInfo->iN; j++ )
    {
      if (aiValid[j] && i!=j && xGetMvdBits(cMvCand-pInfo->m_acMvCand[j]) < uiBestBits)
      {
        aiValid[i] = 0;
      }
    }
  }
  
  iNTmp = 0;
  for ( i=0; i<pInfo->iN; i++ )
  {
    if (aiValid[i])
      iNTmp++;
  }
  
  if (iNTmp == pInfo->iN)
  {
    return false;
  }
  
  assert(iNTmp > 0);
  
  iNTmp = 0;
  for ( i=0; i<pInfo->iN; i++ )
  {
    if (aiValid[i])
    {
      acMv[iNTmp++] = pInfo->m_acMvCand[i];
    }
  }
  
  for ( i=0; i<iNTmp; i++ ) pInfo->m_acMvCand[i] = acMv[i];
  pInfo->iN = iNTmp;
  
  return true;
}
#endif

Int TComDataCU::searchMVPIdx(TComMv cMv, AMVPInfo* pInfo)
{
  for ( Int i=0; i<pInfo->iN; i++ )
  {
    if (cMv == pInfo->m_acMvCand[i])
      return i;
  }
  
  assert(0);
  return -1;
}

Void TComDataCU::clipMv    (TComMv&  rcMv)
{
  Int  iMvShift = 2;
#if MW_DEPTH_MAP_INTERP_FILTER == 2 && MW_FULL_PEL_DEPTH_MAP_MV_SIGNALLING
  if( getSlice()->getSPS()->isDepth() )
    iMvShift = 0;
#endif
  Int iHorMax = (m_pcSlice->getSPS()->getWidth() - m_uiCUPelX - 1 )<<iMvShift;
  Int iHorMin = (      -(Int)g_uiMaxCUWidth - (Int)m_uiCUPelX + 1 )<<iMvShift;
  
  Int iVerMax = (m_pcSlice->getSPS()->getHeight() - m_uiCUPelY - 1 )<<iMvShift;
  Int iVerMin = (      -(Int)g_uiMaxCUHeight - (Int)m_uiCUPelY + 1 )<<iMvShift;
  
  rcMv.setHor( Min (iHorMax, Max (iHorMin, rcMv.getHor())) );
  rcMv.setVer( Min (iVerMax, Max (iVerMin, rcMv.getVer())) );
}


Void TComDataCU::convertTransIdx( UInt uiAbsPartIdx, UInt uiTrIdx, UInt& ruiLumaTrMode, UInt& ruiChromaTrMode )
{
  ruiLumaTrMode   = uiTrIdx;
  ruiChromaTrMode = uiTrIdx;
  return;
}

UInt TComDataCU::getIntraSizeIdx(UInt uiAbsPartIdx)
{
  UInt uiShift = ( (m_puhTrIdx[uiAbsPartIdx]==0) && (m_pePartSize[uiAbsPartIdx]==SIZE_NxN) ) ? m_puhTrIdx[uiAbsPartIdx]+1 : m_puhTrIdx[uiAbsPartIdx];
  uiShift = ( m_pePartSize[uiAbsPartIdx]==SIZE_NxN ? 1 : 0 );
  
  UChar uiWidth = m_puhWidth[uiAbsPartIdx]>>uiShift;
  UInt  uiCnt = 0;
  while( uiWidth )
  {
    uiCnt++;
    uiWidth>>=1;
  }
  uiCnt-=2;
  return uiCnt > 6 ? 6 : uiCnt;
}

Void TComDataCU::clearCbf( UInt uiIdx, TextType eType, UInt uiNumParts )
{
  ::memset( &m_puhCbf[g_aucConvertTxtTypeToIdx[eType]][uiIdx], 0, sizeof(UChar)*uiNumParts);
}

/** Test if the current block is skipped or not
 * \param uiPartIdx
 * \returns Bool
 */
Bool TComDataCU::isSkipped( UInt uiPartIdx )
{
  if ( m_pcSlice->isIntra () )
  {
    return false;
  }
#if HHI_MRG_SKIP
  return ( m_pePredMode[ uiPartIdx ] == MODE_SKIP && getMergeFlag( uiPartIdx ) && !getQtRootCbf( uiPartIdx ) );
#else
  if ( m_pcSlice->isInterP() )
  {
    return ( ( m_pePredMode[ uiPartIdx ] == MODE_SKIP ) && ( ( m_puhCbf[0][uiPartIdx] & 0x1 ) + ( m_puhCbf[1][uiPartIdx] & 0x1 ) + ( m_puhCbf[2][uiPartIdx] & 0x1 ) == 0) );
  }
  else //if ( m_pcSlice->isInterB()  )
  {
    return ( ( m_pePredMode[ uiPartIdx ] == MODE_SKIP ) && ( ( m_puhCbf[0][uiPartIdx] & 0x1 ) + ( m_puhCbf[1][uiPartIdx] & 0x1 ) + ( m_puhCbf[2][uiPartIdx] & 0x1 ) == 0) && (m_puhInterDir[uiPartIdx] == 3) );
  }
#endif
}

// ====================================================================================================================
// Protected member functions
// ====================================================================================================================

Bool TComDataCU::xAddMVPCand( AMVPInfo* pInfo, RefPicList eRefPicList, Int iRefIdx, UInt uiPartUnitIdx, MVP_DIR eDir )
{
  TComDataCU* pcTmpCU = NULL;
  UInt uiIdx;
  switch( eDir )
  {
    case MD_LEFT:
    {
      pcTmpCU = getPULeft(uiIdx, uiPartUnitIdx);
      break;
    }
    case MD_ABOVE:
    {
      pcTmpCU = getPUAbove(uiIdx, uiPartUnitIdx);
      break;
    }
    case MD_ABOVE_RIGHT:
    {
      pcTmpCU = getPUAboveRight(uiIdx, uiPartUnitIdx);
      break;
    }
    case MD_BELOW_LEFT:
    {
      pcTmpCU = getPUBelowLeft(uiIdx, uiPartUnitIdx);
      break;
    }
    case MD_ABOVE_LEFT:
    {
      pcTmpCU = getPUAboveLeft(uiIdx, uiPartUnitIdx);
      break;
    }
    default:
    {
      break;
    }
  }
 
  if ( pcTmpCU != NULL && m_pcSlice->isEqualRef(eRefPicList, pcTmpCU->getCUMvField(eRefPicList)->getRefIdx(uiIdx), iRefIdx) )
  {
    TComMv cMvPred = pcTmpCU->getCUMvField(eRefPicList)->getMv(uiIdx);
    clipMv(cMvPred);
    
    pInfo->m_acMvCand[ pInfo->iN++] = cMvPred;
    return true;
  }
  return false;
}

Void TComDataCU::xUniqueMVPCand(AMVPInfo* pInfo)
{
  if ( pInfo->iN == 0 )
  {
    pInfo->m_acMvCand[ pInfo->iN++ ].setZero();
    return;
  }
  
  TComMv  acMv[ AMVP_MAX_NUM_CANDS ];
  Int iNTmp, i, j;
  
  // make it be unique
  iNTmp = 0;
  acMv[ iNTmp++ ] = pInfo->m_acMvCand[0];
  for ( i=1; i<pInfo->iN; i++ )
  {
    // BugFix for 1603
    for ( j=iNTmp - 1; j>=0; j-- )
    {
      if ( pInfo->m_acMvCand[i] == acMv[j] ) break;
    }
    if ( j<0 )
    {
      acMv[ iNTmp++ ] = pInfo->m_acMvCand[i];
    }
  }
  for ( i=0; i<iNTmp; i++ ) pInfo->m_acMvCand[i] = acMv[i];
  pInfo->iN = iNTmp;
  
  return ;
}

#if MTK_AMVP_SMVP_DERIVATION
/** 
 * \param pInfo
 * \param eRefPicList 
 * \param iRefIdx
 * \param uiPartUnitIdx
 * \param eDir
 * \returns Bool
 */
Bool TComDataCU::xAddMVPCandOrder( AMVPInfo* pInfo, RefPicList eRefPicList, Int iRefIdx, UInt uiPartUnitIdx, MVP_DIR eDir )
{
  TComDataCU* pcTmpCU = NULL;
  UInt uiIdx;
  switch( eDir )
  {
  case MD_LEFT:
    {
      pcTmpCU = getPULeft(uiIdx, uiPartUnitIdx);
      break;
    }
  case MD_ABOVE:
    {
      pcTmpCU = getPUAbove(uiIdx, uiPartUnitIdx);
      break;
    }
  case MD_ABOVE_RIGHT:
    {
      pcTmpCU = getPUAboveRight(uiIdx, uiPartUnitIdx);
      break;
    }
  case MD_BELOW_LEFT:
    {
      pcTmpCU = getPUBelowLeft(uiIdx, uiPartUnitIdx);
      break;
    }
  case MD_ABOVE_LEFT:
    {
      pcTmpCU = getPUAboveLeft(uiIdx, uiPartUnitIdx);
      break;
    }
  default:
    {
      break;
    }
  }

  if ( pcTmpCU == NULL ) 
    return false;
    
  RefPicList eRefPicList2nd = REF_PIC_LIST_0;
  if(       eRefPicList == REF_PIC_LIST_0 )
    eRefPicList2nd = REF_PIC_LIST_1;
  else if ( eRefPicList == REF_PIC_LIST_1)
    eRefPicList2nd = REF_PIC_LIST_0;


  Int iCurrPOC = m_pcSlice->getPOC();
  Int iCurrRefPOC = m_pcSlice->getRefPic( eRefPicList, iRefIdx)->getPOC();
  Int iNeibPOC = iCurrPOC;
  Int iNeibRefPOC;


  if( pcTmpCU->getCUMvField(eRefPicList2nd)->getRefIdx(uiIdx) >= 0 )
  {
    iNeibRefPOC = pcTmpCU->getSlice()->getRefPOC( eRefPicList2nd, pcTmpCU->getCUMvField(eRefPicList2nd)->getRefIdx(uiIdx) );
    if( pcTmpCU->getSlice()->getRefViewIdx( eRefPicList2nd, pcTmpCU->getCUMvField(eRefPicList2nd)->getRefIdx(uiIdx) ) != m_pcSlice->getRefViewIdx( eRefPicList, iRefIdx ) )
      return false;
    if( iNeibRefPOC == iCurrRefPOC ) // Same Reference Frame But Diff List//
    {
      TComMv cMvPred = pcTmpCU->getCUMvField(eRefPicList2nd)->getMv(uiIdx);

      clipMv(cMvPred);
      pInfo->m_acMvCand[ pInfo->iN++] = cMvPred;
      return true;
    }
  }
  //---------------  V1 (END) ------------------//
  if( pcTmpCU->getCUMvField(eRefPicList)->getRefIdx(uiIdx) >= 0)
  {
    iNeibRefPOC = pcTmpCU->getSlice()->getRefPOC( eRefPicList, pcTmpCU->getCUMvField(eRefPicList)->getRefIdx(uiIdx) );
    TComMv cMvPred = pcTmpCU->getCUMvField(eRefPicList)->getMv(uiIdx);
    TComMv rcMv;

    if( pcTmpCU->getSlice()->getRefViewIdx( eRefPicList, pcTmpCU->getCUMvField(eRefPicList)->getRefIdx(uiIdx) ) != m_pcSlice->getRefViewIdx( eRefPicList, iRefIdx ) )
      return false;

    Int iScale = xGetDistScaleFactor( iCurrPOC, iCurrRefPOC, iNeibPOC, iNeibRefPOC );
    if ( iScale == 1024 )
    {
      rcMv = cMvPred;
    }
    else
    {
      rcMv = cMvPred.scaleMv( iScale );
    }

    clipMv(rcMv);

    pInfo->m_acMvCand[ pInfo->iN++] = rcMv;
    return true;
  }
  //---------------------- V2(END) --------------------//
  if( pcTmpCU->getCUMvField(eRefPicList2nd)->getRefIdx(uiIdx) >= 0)
  {
    iNeibRefPOC = pcTmpCU->getSlice()->getRefPOC( eRefPicList2nd, pcTmpCU->getCUMvField(eRefPicList2nd)->getRefIdx(uiIdx) );
    TComMv cMvPred = pcTmpCU->getCUMvField(eRefPicList2nd)->getMv(uiIdx);
    TComMv rcMv;

    if( pcTmpCU->getSlice()->getRefViewIdx( eRefPicList2nd, pcTmpCU->getCUMvField(eRefPicList2nd)->getRefIdx(uiIdx) ) != m_pcSlice->getRefViewIdx( eRefPicList, iRefIdx ) )
      return false;

    Int iScale = xGetDistScaleFactor( iCurrPOC, iCurrRefPOC, iNeibPOC, iNeibRefPOC );
    if ( iScale == 1024 )
    {
      rcMv = cMvPred;
    }
    else
    {
      rcMv = cMvPred.scaleMv( iScale );
    }

    clipMv(rcMv);

    pInfo->m_acMvCand[ pInfo->iN++] = rcMv;
    return true;
  }
  //---------------------- V3(END) --------------------//
  return false;
}
#endif

/** 
 * \param eRefPicList
 * \param uiCUAddr 
 * \param uiPartUnitIdx
 * \param riRefIdx
 * \returns Bool
 */
#if MTK_TMVP_H_MRG || MTK_TMVP_H_AMVP
Bool TComDataCU::xGetColMVP( RefPicList eRefPicList, Int uiCUAddr, Int uiPartUnitIdx, TComMv& rcMv, Int& riRefIdx )
{
  UInt uiAbsPartAddr = uiPartUnitIdx;

  RefPicList  eColRefPicList;
  Int iColPOC, iColRefPOC, iCurrPOC, iCurrRefPOC, iScale;
  TComMv cColMv;

  iCurrPOC = m_pcSlice->getPOC();    
  iCurrRefPOC = m_pcSlice->getRefPic(eRefPicList, riRefIdx)->getPOC();

  // use coldir.
  TComPic *pColPic = getSlice()->getRefPic( RefPicList(getSlice()->isInterB() ? getSlice()->getColDir() : 0), 0);
  TComDataCU *pColCU = pColPic->getCU( uiCUAddr );
  iColPOC = pColCU->getSlice()->getPOC();  

  if (pColCU->isIntra(uiAbsPartAddr))
  {
    return false;
  }

  if( m_pcSlice->getRefPic(eRefPicList, riRefIdx)->getViewIdx() != m_pcSlice->getViewIdx() )
    return false;

  // Prefer a vector crossing us.  Prefer shortest.
  eColRefPicList = REF_PIC_LIST_0;
  bool bFirstCrosses = false;
  Int  iFirstColDist = -1;

  for (Int l = 0; l < 2; l++)
  {
    bool bSaveIt = false;
    int iColRefIdx = pColCU->getCUMvField(RefPicList(l))->getRefIdx(uiAbsPartAddr);
    if (iColRefIdx < 0)
    {
      continue;
    }
    iColRefPOC = pColCU->getSlice()->getRefPOC(RefPicList(l), iColRefIdx);
    if( pColCU->getSlice()->getRefViewIdx(RefPicList(l), iColRefIdx) != pColCU->getSlice()->getViewIdx() )
      continue;
    int iColDist = abs(iColRefPOC - iColPOC);
    bool bCrosses = iColPOC < iCurrPOC ? iColRefPOC > iCurrPOC : iColRefPOC < iCurrPOC;
    if (iFirstColDist < 0)
    {
      bSaveIt = true;
    }
    else if (bCrosses && !bFirstCrosses)
    {
      bSaveIt = true;
    }
#if PANASONIC_AMVPTEMPORALMOD
    else if (bCrosses == bFirstCrosses && l == eRefPicList)
#else
    else if (bCrosses == bFirstCrosses && iColDist < iFirstColDist)
#endif
    {
      bSaveIt = true;
    }
    if (bSaveIt)
    {
      bFirstCrosses = bCrosses;
      iFirstColDist = iColDist;
      eColRefPicList = RefPicList(l);
    }
  }
  if( iFirstColDist < 0 )
    return false;
  // Scale the vector.
  iColRefPOC = pColCU->getSlice()->getRefPOC(eColRefPicList, pColCU->getCUMvField(eColRefPicList)->getRefIdx(uiAbsPartAddr));
  cColMv = pColCU->getCUMvField(eColRefPicList)->getMv(uiAbsPartAddr);

  iCurrRefPOC = m_pcSlice->getRefPic(eRefPicList, riRefIdx)->getPOC();
  iScale = xGetDistScaleFactor(iCurrPOC, iCurrRefPOC, iColPOC, iColRefPOC);
  if (iScale == 1024)
  {
    rcMv = cColMv;
  }
  else
  {
    rcMv = cColMv.scaleMv( iScale );
  }
  clipMv(rcMv);
  return true;
}
#endif

UInt TComDataCU::xGetMvdBits(TComMv cMvd)
{
  return ( xGetComponentBits(cMvd.getHor()) + xGetComponentBits(cMvd.getVer()) );
}

UInt TComDataCU::xGetComponentBits(Int iVal)
{
  UInt uiLength = 1;
  UInt uiTemp   = ( iVal <= 0) ? (-iVal<<1)+1: (iVal<<1);
  
  assert ( uiTemp );
  
  while ( 1 != uiTemp )
  {
    uiTemp >>= 1;
    uiLength += 2;
  }
  
  return uiLength;
}


Int TComDataCU::xGetDistScaleFactor(Int iCurrPOC, Int iCurrRefPOC, Int iColPOC, Int iColRefPOC)
{
  Int iDiffPocD = iColPOC - iColRefPOC;
  Int iDiffPocB = iCurrPOC - iCurrRefPOC;
  
  if( iDiffPocD == iDiffPocB )
  {
    return 1024;
  }
  else
  {
    Int iTDB      = Clip3( -128, 127, iDiffPocB );
    Int iTDD      = Clip3( -128, 127, iDiffPocD );
    Int iX        = (0x4000 + abs(iTDD/2)) / iTDD;
    Int iScale    = Clip3( -1024, 1023, (iTDB * iX + 32) >> 6 );
    return iScale;
  }
}

Void TComDataCU::xCalcCuCbf( UChar* puhCbf, UInt uiTrDepth, UInt uiCbfDepth, UInt uiCuDepth )
{
  if ( uiTrDepth == 0 )
    return;
  
  UInt ui, uiNumSig = 0;
  
  UInt uiNumPart  = m_pcPic->getNumPartInCU() >> ( uiCuDepth << 1 );
  UInt uiQNumPart = uiNumPart >> 2;
  
  UInt uiCbfDepth1 = uiCbfDepth + 1;
  if( uiNumPart == 1 )
  {
    if ( ( puhCbf[0] >> uiCbfDepth1 ) & 0x1 )
    {
      uiNumSig = 1;
    }
    puhCbf[0] |= uiNumSig << uiCbfDepth;
    
    return;
  }
  assert( uiQNumPart );
  
  if ( uiCbfDepth < ( uiTrDepth - 1 ) )
  {
    UChar* puhNextCbf = puhCbf;
    xCalcCuCbf( puhNextCbf, uiTrDepth, uiCbfDepth1, uiCuDepth+1 ); puhNextCbf += uiQNumPart;
    xCalcCuCbf( puhNextCbf, uiTrDepth, uiCbfDepth1, uiCuDepth+1 ); puhNextCbf += uiQNumPart;
    xCalcCuCbf( puhNextCbf, uiTrDepth, uiCbfDepth1, uiCuDepth+1 ); puhNextCbf += uiQNumPart;
    xCalcCuCbf( puhNextCbf, uiTrDepth, uiCbfDepth1, uiCuDepth+1 );
  }
  
  for ( ui = 0; ui < uiNumPart; ui += uiQNumPart )
  {
    if ( ( puhCbf[ui] >> uiCbfDepth1 ) & 0x1 )
    {
      uiNumSig = 1;
      break;
    }
  }
  
  uiNumSig <<= uiCbfDepth;
  for ( ui = 0; ui < uiNumPart; ui++ )
  {
    puhCbf[ui] |= uiNumSig;
  }
}

#if FT_TCTR_AMVP || FT_TCTR_MRG
/** 
 * \param eCUMode
 * \param uiPartIdx 
 * \param ruiPartIdxCenter
 * \returns Void
 */
Void TComDataCU::xDeriveCenterIdx( PartSize eCUMode, UInt uiPartIdx, UInt& ruiPartIdxCenter )
{
  UInt uiPartAddr;
  Int  iPartWidth;
  Int  iPartHeight;
  getPartIndexAndSize( uiPartIdx, uiPartAddr, iPartWidth, iPartHeight);
  
  ruiPartIdxCenter = m_uiAbsIdxInLCU+uiPartAddr; // partition origin.
  ruiPartIdxCenter = g_auiRasterToZscan[ g_auiZscanToRaster[ ruiPartIdxCenter ]
                                        + ( iPartHeight/m_pcPic->getMinCUHeight() -1 )/2*m_pcPic->getNumPartInWidth()
                                        + ( iPartWidth/m_pcPic->getMinCUWidth()  -1 )/2];
}

/** 
 * \param uiPartIdx
 * \param eRefPicList 
 * \param iRefIdx
 * \param pcMv
 * \returns Bool
 */
Bool TComDataCU::xGetCenterCol( UInt uiPartIdx, RefPicList eRefPicList, int iRefIdx, TComMv *pcMv )
{
  PartSize eCUMode = m_pePartSize[0];
  
  Int iCurrPOC = m_pcSlice->getPOC();
  
  // use coldir.
  TComPic *pColPic = getSlice()->getRefPic( RefPicList(getSlice()->isInterB() ? getSlice()->getColDir() : 0), 0);
  TComDataCU *pColCU = pColPic->getCU( m_uiCUAddr );
  
  Int iColPOC = pColCU->getSlice()->getPOC();
  UInt uiPartIdxCenter;
  xDeriveCenterIdx( eCUMode, uiPartIdx, uiPartIdxCenter );
  
  if (pColCU->isIntra(uiPartIdxCenter))
    return false;
  
  if( m_pcSlice->getRefPic(eRefPicList, iRefIdx)->getViewIdx() != m_pcSlice->getViewIdx() )
    return false;

  // Prefer a vector crossing us.  Prefer shortest.
  RefPicList eColRefPicList = REF_PIC_LIST_0;
  bool bFirstCrosses = false;
  Int  iFirstColDist = -1;
  for (Int l = 0; l < 2; l++)
  {
    bool bSaveIt = false;
    int iColRefIdx = pColCU->getCUMvField(RefPicList(l))->getRefIdx(uiPartIdxCenter);
    if (iColRefIdx < 0)
      continue;
    int iColRefPOC = pColCU->getSlice()->getRefPOC(RefPicList(l), iColRefIdx);
    if( pColCU->getSlice()->getRefViewIdx(RefPicList(l), iColRefIdx) != pColCU->getSlice()->getViewIdx() )
      continue;
    int iColDist = abs(iColRefPOC - iColPOC);
    bool bCrosses = iColPOC < iCurrPOC ? iColRefPOC > iCurrPOC : iColRefPOC < iCurrPOC;
    if (iFirstColDist < 0)
      bSaveIt = true;
    else if (bCrosses && !bFirstCrosses)
      bSaveIt = true;
#if PANASONIC_AMVPTEMPORALMOD
    else if (bCrosses == bFirstCrosses && l == eRefPicList)
#else
    else if (bCrosses == bFirstCrosses && iColDist < iFirstColDist)
#endif
      bSaveIt = true;
    
    if (bSaveIt)
    {
      bFirstCrosses = bCrosses;
      iFirstColDist = iColDist;
      eColRefPicList = RefPicList(l);
    }
  }
  if( iFirstColDist < 0 )
    return false;

  // Scale the vector.
  Int iColRefPOC = pColCU->getSlice()->getRefPOC(eColRefPicList, pColCU->getCUMvField(eColRefPicList)->getRefIdx(uiPartIdxCenter));
  TComMv cColMv = pColCU->getCUMvField(eColRefPicList)->getMv(uiPartIdxCenter);
  
  Int iCurrRefPOC = m_pcSlice->getRefPic(eRefPicList, iRefIdx)->getPOC();
  Int iScale = xGetDistScaleFactor(iCurrPOC, iCurrRefPOC, iColPOC, iColRefPOC);
  if (iScale == 1024)
  {
    pcMv[0] = cColMv;
  }
  else
  {
    pcMv[0] = cColMv.scaleMv( iScale );
  }
  clipMv(pcMv[0]);
  
  return true;
}
#endif

#if AMVP_BUFFERCOMPRESS
Void TComDataCU::compressMV()
{
  m_acCUMvField[0].compress(m_pePredMode,m_puhInterDir);
  m_acCUMvField[1].compress(m_pePredMode,m_puhInterDir);
}
#endif 

#if QC_MDCS
UInt TComDataCU::getCoefScanIdx(UInt uiAbsPartIdx, UInt uiWidth, Bool bIsLuma, Bool bIsIntra)
{
  static const UChar aucIntraLumaDirToScanIdx[MAX_CU_DEPTH][34] =
  {
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, },
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, },
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, },
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, },
    {1, 2, 0, 0, 1, 1, 0, 2, 2, 0, 0, 1, 1, 0, 0, 2, 2, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 2, 2, 2, 2, 0, 0, },
    {1, 2, 0, 0, 1, 1, 0, 2, 2, 0, 0, 1, 1, 0, 0, 2, 2, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 2, 2, 2, 2, 0, 0, },
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, },
  };
  static const UChar aucIntraChromaDirToScanIdx[MAX_CU_DEPTH][5] = 
  {
    {0, 0, 0, 0, 0, },
    {0, 0, 0, 0, 0, },
    {0, 0, 0, 0, 0, },
    {0, 0, 0, 0, 0, },
    {0, 0, 0, 0, 0, },
    {1, 2, 0, 0, 0, },
    {1, 2, 0, 0, 0, },
  };

  UInt uiCTXIdx;
  UInt uiScanIdx;
  UInt uiDirMode;

  if ( !bIsIntra ) 
  {
    uiScanIdx = SCAN_ZIGZAG;
    return uiScanIdx;
  }

  switch(uiWidth)
  {
    case  2: uiCTXIdx = 6; break;
    case  4: uiCTXIdx = 5; break;
    case  8: uiCTXIdx = 4; break;
    case 16: uiCTXIdx = 3; break;
    case 32: uiCTXIdx = 2; break;
    case 64: uiCTXIdx = 1; break;
    default: uiCTXIdx = 0; break;
  }

  if ( bIsLuma )
  {
    uiDirMode = getLumaIntraDir(uiAbsPartIdx);
#if ADD_PLANAR_MODE
    mapPlanartoDC( uiDirMode );
#endif
    uiScanIdx = aucIntraLumaDirToScanIdx[uiCTXIdx][uiDirMode];
  }
  else
  {
       uiDirMode = getChromaIntraDir(uiAbsPartIdx);
#if ADD_PLANAR_MODE
       mapPlanartoDC( uiDirMode );
#endif
       if (uiDirMode < 4)
       {
         uiScanIdx = (aucIntraChromaDirToScanIdx[uiCTXIdx][uiDirMode]);
       }
       else
       {
         uiDirMode = getLumaIntraDir(uiAbsPartIdx);
#if ADD_PLANAR_MODE
         mapPlanartoDC( uiDirMode );
#endif
         uiScanIdx = aucIntraLumaDirToScanIdx[max<UInt>(uiCTXIdx-1,0)][uiDirMode];
       }
  }

  return uiScanIdx;
}
#endif //QC_MDCS

#if MS_LCEC_LOOKUP_TABLE_EXCEPTION
Bool TComDataCU::isSuroundingRefIdxException     ( UInt   uiAbsPartIdx )
{
  if ( !getSlice()->isInterB() )
    return true;
  
  if ( getSlice()->getNumRefIdx( REF_PIC_LIST_0 ) <= 2 && getSlice()->getNumRefIdx( REF_PIC_LIST_1 ) <= 2)
  {
    return false;
  }
  
  TComDataCU* pcTempCU;
  UInt        uiTempPartIdx;
  TComMvField cMvFieldTemp;
  
  if ( getSlice()->getNumRefIdx ( REF_PIC_LIST_C ) > 0 )  // combined list case
  {
    // Left PU
    pcTempCU = getPULeft( uiTempPartIdx, m_uiAbsIdxInLCU + uiAbsPartIdx );
    if ( pcTempCU )
    {
      if (pcTempCU->getInterDir(uiTempPartIdx) == 1 )
      {
        if ( pcTempCU->getSlice()->getRefIdxOfLC(REF_PIC_LIST_0, pcTempCU->getCUMvField( REF_PIC_LIST_0 )->getRefIdx( uiTempPartIdx )) > 3 )
          return true;
      }
      else if (pcTempCU->getInterDir(uiTempPartIdx) == 2 )
      {
        if ( pcTempCU->getSlice()->getRefIdxOfLC(REF_PIC_LIST_1, pcTempCU->getCUMvField( REF_PIC_LIST_1 )->getRefIdx( uiTempPartIdx )) > 3 )
          return true;
      }
      else if ( pcTempCU->getInterDir(uiTempPartIdx) == 3 )
      {
        if ( pcTempCU->getCUMvField( REF_PIC_LIST_0 )->getRefIdx( uiTempPartIdx ) > 1 )
          return true;
        
        if ( pcTempCU->getCUMvField( REF_PIC_LIST_1 )->getRefIdx( uiTempPartIdx ) > 1 )
          return true;
      }
    }
    
    // Above PU
    pcTempCU = getPUAbove( uiTempPartIdx, m_uiAbsIdxInLCU + uiAbsPartIdx );
    if ( pcTempCU )
    {
      if (pcTempCU->getInterDir(uiTempPartIdx) == 1 )
      {
        if ( pcTempCU->getSlice()->getRefIdxOfLC(REF_PIC_LIST_0, pcTempCU->getCUMvField( REF_PIC_LIST_0 )->getRefIdx( uiTempPartIdx )) > 3 )
          return true;
      }
      else if (pcTempCU->getInterDir(uiTempPartIdx) == 2 )
      {
        if ( pcTempCU->getSlice()->getRefIdxOfLC(REF_PIC_LIST_1, pcTempCU->getCUMvField( REF_PIC_LIST_1 )->getRefIdx( uiTempPartIdx )) > 3 )
          return true;
      }
      else if ( pcTempCU->getInterDir(uiTempPartIdx) == 3 )
      {
        if ( pcTempCU->getCUMvField( REF_PIC_LIST_0 )->getRefIdx( uiTempPartIdx ) > 1 )
          return true;
        
        if ( pcTempCU->getCUMvField( REF_PIC_LIST_1 )->getRefIdx( uiTempPartIdx ) > 1 )
          return true;
      }
    }
    
    // Above left PU
    pcTempCU = getPUAboveLeft( uiTempPartIdx, m_uiAbsIdxInLCU + uiAbsPartIdx );
    if ( pcTempCU )
    {
      if (pcTempCU->getInterDir(uiTempPartIdx) == 1 )
      {
        if ( pcTempCU->getSlice()->getRefIdxOfLC(REF_PIC_LIST_0, pcTempCU->getCUMvField( REF_PIC_LIST_0 )->getRefIdx( uiTempPartIdx )) > 3 )
          return true;
      }
      else if (pcTempCU->getInterDir(uiTempPartIdx) == 2 )
      {
        if ( pcTempCU->getSlice()->getRefIdxOfLC(REF_PIC_LIST_1, pcTempCU->getCUMvField( REF_PIC_LIST_1 )->getRefIdx( uiTempPartIdx )) > 3 )
          return true;
      }
      else if ( pcTempCU->getInterDir(uiTempPartIdx) == 3 )
      {
        if ( pcTempCU->getCUMvField( REF_PIC_LIST_0 )->getRefIdx( uiTempPartIdx ) > 1 )
          return true;
        
        if ( pcTempCU->getCUMvField( REF_PIC_LIST_1 )->getRefIdx( uiTempPartIdx ) > 1 )
          return true;
      }
    }
    
    return false;
  }
  
  // not combined list case
  // Left PU
  pcTempCU = getPULeft( uiTempPartIdx, m_uiAbsIdxInLCU + uiAbsPartIdx );
  if ( pcTempCU )
  {
    if (pcTempCU->getInterDir(uiTempPartIdx) == 1 )
    {
      if ( pcTempCU->getCUMvField( REF_PIC_LIST_0 )->getRefIdx( uiTempPartIdx ) > MS_LCEC_UNI_EXCEPTION_THRES )
        return true;
    }
    else if (pcTempCU->getInterDir(uiTempPartIdx) == 2 )
    {
      if ( pcTempCU->getCUMvField( REF_PIC_LIST_1 )->getRefIdx( uiTempPartIdx ) > MS_LCEC_UNI_EXCEPTION_THRES )
        return true;
    }
    else if ( pcTempCU->getInterDir(uiTempPartIdx) == 3 )
    {
      if ( pcTempCU->getCUMvField( REF_PIC_LIST_0 )->getRefIdx( uiTempPartIdx ) > 1 )
        return true;
      
      if ( pcTempCU->getCUMvField( REF_PIC_LIST_1 )->getRefIdx( uiTempPartIdx ) > 1 )
        return true;
    }
  }
  
  // Above PU
  pcTempCU = getPUAbove( uiTempPartIdx, m_uiAbsIdxInLCU + uiAbsPartIdx );
  if ( pcTempCU )
  {
    if (pcTempCU->getInterDir(uiTempPartIdx) == 1 )
    {
      if ( pcTempCU->getCUMvField( REF_PIC_LIST_0 )->getRefIdx( uiTempPartIdx ) > MS_LCEC_UNI_EXCEPTION_THRES )
        return true;
    }
    else if (pcTempCU->getInterDir(uiTempPartIdx) == 2 )
    {
      if ( pcTempCU->getCUMvField( REF_PIC_LIST_1 )->getRefIdx( uiTempPartIdx ) > MS_LCEC_UNI_EXCEPTION_THRES )
        return true;
    }
    else if ( pcTempCU->getInterDir(uiTempPartIdx) == 3 )
    {
      if ( pcTempCU->getCUMvField( REF_PIC_LIST_0 )->getRefIdx( uiTempPartIdx ) > 1 )
        return true;
      
      if ( pcTempCU->getCUMvField( REF_PIC_LIST_1 )->getRefIdx( uiTempPartIdx ) > 1 )
        return true;
    }
  }
  
  // Above left PU
  pcTempCU = getPUAboveLeft( uiTempPartIdx, m_uiAbsIdxInLCU + uiAbsPartIdx );
  if ( pcTempCU )
  {
    if (pcTempCU->getInterDir(uiTempPartIdx) == 1 )
    {
      if ( pcTempCU->getCUMvField( REF_PIC_LIST_0 )->getRefIdx( uiTempPartIdx ) > MS_LCEC_UNI_EXCEPTION_THRES )
        return true;
    }
    else if (pcTempCU->getInterDir(uiTempPartIdx) == 2 )
    {
      if ( pcTempCU->getCUMvField( REF_PIC_LIST_1 )->getRefIdx( uiTempPartIdx ) > MS_LCEC_UNI_EXCEPTION_THRES )
        return true;
    }
    else if ( pcTempCU->getInterDir(uiTempPartIdx) == 3 )
    {
      if ( pcTempCU->getCUMvField( REF_PIC_LIST_0 )->getRefIdx( uiTempPartIdx ) > 1 )
        return true;
      
      if ( pcTempCU->getCUMvField( REF_PIC_LIST_1 )->getRefIdx( uiTempPartIdx ) > 1 )
        return true;
    }
  }
  
  return false;
}
#endif

Void TComDataCU::setTextureModeDepthSubParts( Int iTextureModeDepth, UInt uiAbsPartIdx, UInt uiDepth )
{
  UInt uiCurrPartNumb = m_pcPic->getNumPartInCU() >> (uiDepth << 1);

  for (UInt ui = 0; ui < uiCurrPartNumb; ui++ )
  {
    m_piTextureModeDepth[uiAbsPartIdx + ui] = iTextureModeDepth;
  }
}

Void TComDataCU::copyTextureMotionDataFrom( TComDataCU* pcCU, UInt uiDepth, UInt uiAbsPartIdxSrc, UInt uiAbsPartIdxDst )
{
  assert( getSlice()->getSPS()->isDepth() && ! pcCU->getSlice()->getSPS()->isDepth() );
  UInt uiNumPartition = m_pcPic->getNumPartInCU() >> (uiDepth << 1);
  Int iSizeInUchar  = sizeof( UChar ) * uiNumPartition;
  Int iSizeInInt    = sizeof( Int   ) * uiNumPartition;
//#if HHI_MRG
//  Int iSizeInBool   = sizeof( Bool  ) * uiNumPartition;
//#endif

//  memcpy( m_pePartSize + uiAbsPartIdxDst,  pcCU->getPartitionSize() + uiAbsPartIdxSrc,  sizeof( PartSize ) * uiNumPartition );
  memcpy( m_pePredMode + uiAbsPartIdxDst,  pcCU->getPredictionMode() + uiAbsPartIdxSrc, sizeof( PredMode ) * uiNumPartition );
  memcpy( m_puhInterDir + uiAbsPartIdxDst, pcCU->getInterDir() + uiAbsPartIdxSrc,       iSizeInUchar );

//#if HHI_MRG
//  m_pbMergeFlag        = pcCU->getMergeFlag()             + uiAbsPartIdx;
//  m_puhMergeIndex      = pcCU->getMergeIndex()            + uiAbsPartIdx;
//  for( UInt ui = 0; ui < HHI_NUM_MRG_CAND; ui ++ )
//  {
//    m_apuhNeighbourCandIdx[ui] = pcCU->getNeighbourCandIdx( ui ) + uiAbsPartIdx;
//  }
//#endif

  memcpy( m_apiMVPIdx[0] + uiAbsPartIdxDst, pcCU->getMVPIdx(REF_PIC_LIST_0) + uiAbsPartIdxSrc, iSizeInInt );
  memcpy( m_apiMVPIdx[1] + uiAbsPartIdxDst, pcCU->getMVPIdx(REF_PIC_LIST_1) + uiAbsPartIdxSrc, iSizeInInt );
  memcpy( m_apiMVPNum[0] + uiAbsPartIdxDst, pcCU->getMVPNum(REF_PIC_LIST_0) + uiAbsPartIdxSrc, iSizeInInt );
  memcpy( m_apiMVPNum[1] + uiAbsPartIdxDst, pcCU->getMVPNum(REF_PIC_LIST_1) + uiAbsPartIdxSrc, iSizeInInt );

//  m_acCUMvField[0].copyFrom( pcCU->getCUMvField( REF_PIC_LIST_0 ), pcCU->getTotalNumPart(), 0 );
//  m_acCUMvField[1].copyFrom( pcCU->getCUMvField( REF_PIC_LIST_1 ), pcCU->getTotalNumPart(), 0 );

  pcCU->getCUMvField( REF_PIC_LIST_0 )->copyTo( &m_acCUMvField[0], -Int(uiAbsPartIdxSrc) + uiAbsPartIdxDst, uiAbsPartIdxSrc, uiNumPartition );
  pcCU->getCUMvField( REF_PIC_LIST_1 )->copyTo( &m_acCUMvField[1], -Int(uiAbsPartIdxSrc) + uiAbsPartIdxDst, uiAbsPartIdxSrc, uiNumPartition );
#if MW_DEPTH_MAP_INTERP_FILTER == 2 && MW_FULL_PEL_DEPTH_MAP_MV_SIGNALLING
  for( UInt ui = 0; ui < uiNumPartition; ui++ )
  {
    m_acCUMvField[0].getMv( uiAbsPartIdxDst + ui ) += TComMv( 2, 2 );
    m_acCUMvField[0].getMv( uiAbsPartIdxDst + ui ) >>= 2;

    m_acCUMvField[1].getMv( uiAbsPartIdxDst + ui ) += TComMv( 2, 2 );
    m_acCUMvField[1].getMv( uiAbsPartIdxDst + ui ) >>= 2;

    m_acCUMvField[0].getMvd( uiAbsPartIdxDst + ui ) += TComMv( 2, 2 );
    m_acCUMvField[0].getMvd( uiAbsPartIdxDst + ui ) >>= 2;

    m_acCUMvField[1].getMvd( uiAbsPartIdxDst + ui ) += TComMv( 2, 2 );
    m_acCUMvField[1].getMvd( uiAbsPartIdxDst + ui ) >>= 2;
  }
#endif
}

Void TComDataCU::getPosInPic( UInt uiAbsPartIndex, Int& riPosX, Int& riPosY )
{
  riPosX = g_auiRasterToPelX[g_auiZscanToRaster[uiAbsPartIndex]] + getCUPelX();
  riPosY = g_auiRasterToPelY[g_auiZscanToRaster[uiAbsPartIndex]] + getCUPelY();  
}
