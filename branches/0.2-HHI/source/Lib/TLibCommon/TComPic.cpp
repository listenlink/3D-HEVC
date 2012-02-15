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
#if DEPTH_MAP_GENERATION
  m_pcPredDepthMap    = NULL;
#endif
#if HHI_INTER_VIEW_MOTION_PRED
  m_pcOrgDepthMap     = NULL;
#endif
#if HHI_INTER_VIEW_RESIDUAL_PRED
  m_pcResidual        = NULL;
#endif
  m_pcPicYuvPred      = NULL;
  m_pcPicYuvResi      = NULL;
#if HHI_INTERVIEW_SKIP
  m_pcUsedPelsMap     = NULL;
#endif
  
#if PARALLEL_MERGED_DEBLK
  m_pcPicYuvDeblkBuf     = NULL;
#endif

  m_bReconstructed    = false;

  m_aiNumRefIdx[0]    = 0;
  m_aiNumRefIdx[1]    = 0;
#if SONY_COLPIC_AVAILABILITY
  m_iViewOrderIdx     = 0;
#endif
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
  
#if DEPTH_MAP_GENERATION
  if( m_pcPredDepthMap )
  {
    m_pcPredDepthMap->destroy();
    delete m_pcPredDepthMap;
    m_pcPredDepthMap = NULL;
  }
#endif

#if HHI_INTER_VIEW_MOTION_PRED
  if( m_pcOrgDepthMap )
  {
    m_pcOrgDepthMap->destroy();
    delete m_pcOrgDepthMap;
    m_pcOrgDepthMap = NULL;
  }
#endif

#if HHI_INTER_VIEW_RESIDUAL_PRED
  if( m_pcResidual )
  {
    m_pcResidual->destroy();
    delete m_pcResidual;
    m_pcResidual = NULL;
  }
#endif

#if HHI_INTERVIEW_SKIP
  if( m_pcUsedPelsMap )
  {
    m_pcUsedPelsMap->destroy();
    delete m_pcUsedPelsMap;
    m_pcUsedPelsMap = NULL;
  }
#endif

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

#if DEPTH_MAP_GENERATION
Void
TComPic::addPrdDepthMapBuffer( UInt uiSubSampExpX, UInt uiSubSampExpY )
{
  AOT( m_pcPredDepthMap );
  AOF( m_apcPicYuv[1]   );
  Int   iWidth        = m_apcPicYuv[1]->getWidth      ();
  Int   iHeight       = m_apcPicYuv[1]->getHeight     ();
  UInt  uiMaxCuWidth  = m_apcPicYuv[1]->getMaxCuWidth ();
  UInt  uiMaxCuHeight = m_apcPicYuv[1]->getMaxCuHeight();
  UInt  uiMaxCuDepth  = m_apcPicYuv[1]->getMaxCuDepth ();
  m_pcPredDepthMap    = new TComPicYuv;
  m_pcPredDepthMap    ->create( iWidth >> uiSubSampExpX, iHeight >> uiSubSampExpY, uiMaxCuWidth >> uiSubSampExpX, uiMaxCuHeight >> uiSubSampExpY, uiMaxCuDepth );
}
#endif

#if HHI_INTER_VIEW_MOTION_PRED
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
#endif

#if HHI_INTER_VIEW_RESIDUAL_PRED
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
#endif

#if HHI_INTERVIEW_SKIP
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
#endif

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

#if DEPTH_MAP_GENERATION
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
#endif

#if HHI_INTER_VIEW_MOTION_PRED
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
#endif

#if HHI_INTER_VIEW_RESIDUAL_PRED
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
#endif

#if HHI_INTERVIEW_SKIP
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
#endif

