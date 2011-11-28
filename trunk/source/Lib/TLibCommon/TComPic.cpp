

/** \file     TComPic.cpp
    \brief    picture class
*/

#include "TComPic.h"
#include "SEI.h"

// ====================================================================================================================
// Constructor / destructor / create / destroy
// ====================================================================================================================

TComPic::TComPic()
{
  m_apcPicSym         = NULL;
  m_apcPicYuv[0]      = NULL;
  m_apcPicYuv[1]      = NULL;
  m_pcPredDepthMap    = NULL;
  m_pcOrgDepthMap     = NULL;
  m_pcResidual        = NULL;
  m_pcPicYuvPred      = NULL;
  m_pcPicYuvResi      = NULL;
  m_pcUsedPelsMap     = NULL;
  
#if PARALLEL_MERGED_DEBLK
  m_pcPicYuvDeblkBuf     = NULL;
#endif

  m_bReconstructed    = false;

  m_aiNumRefIdx[0]    = 0;
  m_aiNumRefIdx[1]    = 0;

  m_iViewIdx          = 0;
  m_aaiCodedScale     = 0;
  m_aaiCodedOffset    = 0;
}

TComPic::~TComPic()
{
}

Void TComPic::create( Int iWidth, Int iHeight, UInt uiMaxWidth, UInt uiMaxHeight, UInt uiMaxDepth )
{
  m_apcPicSym     = new TComPicSym;  m_apcPicSym   ->create( iWidth, iHeight, uiMaxWidth, uiMaxHeight, uiMaxDepth );
  m_apcPicYuv[1]  = new TComPicYuv;  m_apcPicYuv[1]->create( iWidth, iHeight, uiMaxWidth, uiMaxHeight, uiMaxDepth );

  /* there are no SEI messages associated with this picture initially */
  m_SEIs = NULL;

  return;
}

Void TComPic::destroy()
{
  if (m_apcPicSym)
  {
    m_apcPicSym->destroy();
    delete m_apcPicSym;
    m_apcPicSym = NULL;
  }
  
  if (m_apcPicYuv[0])
  {
    m_apcPicYuv[0]->destroy();
    delete m_apcPicYuv[0];
    m_apcPicYuv[0]  = NULL;
  }
  
  if (m_apcPicYuv[1])
  {
    m_apcPicYuv[1]->destroy();
    delete m_apcPicYuv[1];
    m_apcPicYuv[1]  = NULL;
  }
  
  if( m_pcPredDepthMap )
  {
    m_pcPredDepthMap->destroy();
    delete m_pcPredDepthMap;
    m_pcPredDepthMap = NULL;
  }

  if( m_pcOrgDepthMap )
  {
    m_pcOrgDepthMap->destroy();
    delete m_pcOrgDepthMap;
    m_pcOrgDepthMap = NULL;
  }

  if( m_pcResidual )
  {
    m_pcResidual->destroy();
    delete m_pcResidual;
    m_pcResidual = NULL;
  }

  if( m_pcUsedPelsMap )
  {
    m_pcUsedPelsMap->destroy();
    delete m_pcUsedPelsMap;
    m_pcUsedPelsMap = NULL;
  }

#if PARALLEL_MERGED_DEBLK
  if (m_pcPicYuvDeblkBuf)
  {
    m_pcPicYuvDeblkBuf->destroy();
    delete m_pcPicYuvDeblkBuf;
    m_pcPicYuvDeblkBuf  = NULL;
  }
#endif

  delete m_SEIs;
}

#if AMVP_BUFFERCOMPRESS
Void TComPic::compressMotion()
{
  TComPicSym* pPicSym = getPicSym(); 
  for ( UInt uiCUAddr = 0; uiCUAddr < pPicSym->getFrameHeightInCU()*pPicSym->getFrameWidthInCU(); uiCUAddr++ )
  {
    TComDataCU* pcCU = pPicSym->getCU(uiCUAddr);
    pcCU->compressMV(); 
  } 
}
#endif



Void
TComPic::addOriginalBuffer()
{
  AOT( m_apcPicYuv[0] );
  AOF( m_apcPicYuv[1] );
  Int   iWidth        = m_apcPicYuv[1]->getWidth      ();
  Int   iHeight       = m_apcPicYuv[1]->getHeight     ();
  UInt  uiMaxCuWidth  = m_apcPicYuv[1]->getMaxCuWidth ();
  UInt  uiMaxCuHeight = m_apcPicYuv[1]->getMaxCuHeight();
  UInt  uiMaxCuDepth  = m_apcPicYuv[1]->getMaxCuDepth ();
  m_apcPicYuv[0]      = new TComPicYuv;
  m_apcPicYuv[0]      ->create( iWidth, iHeight, uiMaxCuWidth, uiMaxCuHeight, uiMaxCuDepth );
}

#if PARALLEL_MERGED_DEBLK
Void
TComPic::addDeblockBuffer()
{
  AOT( m_pcPicYuvDeblkBuf );
  AOF( m_apcPicYuv[1]     );
  Int   iWidth        = m_apcPicYuv[1]->getWidth      ();
  Int   iHeight       = m_apcPicYuv[1]->getHeight     ();
  UInt  uiMaxCuWidth  = m_apcPicYuv[1]->getMaxCuWidth ();
  UInt  uiMaxCuHeight = m_apcPicYuv[1]->getMaxCuHeight();
  UInt  uiMaxCuDepth  = m_apcPicYuv[1]->getMaxCuDepth ();
  m_pcPicYuvDeblkBuf  = new TComPicYuv;
  m_pcPicYuvDeblkBuf  ->create( iWidth, iHeight, uiMaxCuWidth, uiMaxCuHeight, uiMaxCuDepth );
}
#endif

Void
TComPic::addPrdDepthMapBuffer()
{
  AOT( m_pcPredDepthMap );
  AOF( m_apcPicYuv[1]   );
  Int   iWidth        = m_apcPicYuv[1]->getWidth      ();
  Int   iHeight       = m_apcPicYuv[1]->getHeight     ();
  UInt  uiMaxCuWidth  = m_apcPicYuv[1]->getMaxCuWidth ();
  UInt  uiMaxCuHeight = m_apcPicYuv[1]->getMaxCuHeight();
  UInt  uiMaxCuDepth  = m_apcPicYuv[1]->getMaxCuDepth ();
  m_pcPredDepthMap    = new TComPicYuv;
  m_pcPredDepthMap    ->create( iWidth, iHeight, uiMaxCuWidth, uiMaxCuHeight, uiMaxCuDepth );
}

Void
TComPic::addOrgDepthMapBuffer()
{
  AOT( m_pcOrgDepthMap );
  AOF( m_apcPicYuv[1]  );
  Int   iWidth        = m_apcPicYuv[1]->getWidth      ();
  Int   iHeight       = m_apcPicYuv[1]->getHeight     ();
  UInt  uiMaxCuWidth  = m_apcPicYuv[1]->getMaxCuWidth ();
  UInt  uiMaxCuHeight = m_apcPicYuv[1]->getMaxCuHeight();
  UInt  uiMaxCuDepth  = m_apcPicYuv[1]->getMaxCuDepth ();
  m_pcOrgDepthMap     = new TComPicYuv;
  m_pcOrgDepthMap     ->create( iWidth, iHeight, uiMaxCuWidth, uiMaxCuHeight, uiMaxCuDepth );
}

Void
TComPic::addResidualBuffer()
{
  AOT( m_pcResidual   );
  AOF( m_apcPicYuv[1] );
  Int   iWidth        = m_apcPicYuv[1]->getWidth      ();
  Int   iHeight       = m_apcPicYuv[1]->getHeight     ();
  UInt  uiMaxCuWidth  = m_apcPicYuv[1]->getMaxCuWidth ();
  UInt  uiMaxCuHeight = m_apcPicYuv[1]->getMaxCuHeight();
  UInt  uiMaxCuDepth  = m_apcPicYuv[1]->getMaxCuDepth ();
  m_pcResidual        = new TComPicYuv;
  m_pcResidual        ->create( iWidth, iHeight, uiMaxCuWidth, uiMaxCuHeight, uiMaxCuDepth );
}

Void
TComPic::addUsedPelsMapBuffer()
{
  AOT( m_pcUsedPelsMap );
  AOF( m_apcPicYuv[1]  );
  Int   iWidth        = m_apcPicYuv[1]->getWidth      ();
  Int   iHeight       = m_apcPicYuv[1]->getHeight     ();
  UInt  uiMaxCuWidth  = m_apcPicYuv[1]->getMaxCuWidth ();
  UInt  uiMaxCuHeight = m_apcPicYuv[1]->getMaxCuHeight();
  UInt  uiMaxCuDepth  = m_apcPicYuv[1]->getMaxCuDepth ();
  m_pcUsedPelsMap     = new TComPicYuv;
  m_pcUsedPelsMap     ->create( iWidth, iHeight, uiMaxCuWidth, uiMaxCuHeight, uiMaxCuDepth );
}

Void
TComPic::removeOriginalBuffer()
{
  if( m_apcPicYuv[0] )
  {
    m_apcPicYuv[0]->destroy();
    delete m_apcPicYuv[0];
    m_apcPicYuv[0]  = NULL;
  }
}

#if PARALLEL_MERGED_DEBLK
Void
TComPic::removeDeblockBuffer()
{
  if( m_pcPicYuvDeblkBuf )
  {
    m_pcPicYuvDeblkBuf->destroy();
    delete m_pcPicYuvDeblkBuf;
    m_pcPicYuvDeblkBuf  = NULL;
  }
}
#endif

Void
TComPic::removePrdDepthMapBuffer()
{
  if( m_pcPredDepthMap )
  {
    m_pcPredDepthMap->destroy();
    delete m_pcPredDepthMap;
    m_pcPredDepthMap = NULL;
  }
}

Void
TComPic::removeOrgDepthMapBuffer()
{
  if( m_pcOrgDepthMap )
  {
    m_pcOrgDepthMap->destroy();
    delete m_pcOrgDepthMap;
    m_pcOrgDepthMap = NULL;
  }
}

Void
TComPic::removeResidualBuffer()
{
  if( m_pcResidual )
  {
    m_pcResidual->destroy();
    delete m_pcResidual;
    m_pcResidual = NULL;
  }
}

Void
TComPic::removeUsedPelsMapBuffer()
{
  if( m_pcUsedPelsMap )
  {
    m_pcUsedPelsMap->destroy();
    delete m_pcUsedPelsMap;
    m_pcUsedPelsMap = NULL;
  }
}

