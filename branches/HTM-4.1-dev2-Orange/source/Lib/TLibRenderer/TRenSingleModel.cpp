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

#include "TRenImage.h"
#include "TRenFilter.h"
#include "TRenSingleModel.h"

////////////// TRENSINGLE MODEL ///////////////
template <BlenMod iBM, Bool bBitInc>
TRenSingleModelC<iBM,bBitInc>::TRenSingleModelC()
:  m_iDistShift ( g_uiBitIncrement << 1 )
{
  m_iWidth  = -1;
  m_iHeight = -1;
  m_iStride = -1;
#if FIX_VSO_SETUP
  m_iUsedHeight = -1; 
  m_iHorOffset  = -1; 
#endif
  m_iMode   = -1;
  m_iPad    = PICYUV_PAD;
  m_iGapTolerance = -1;
  m_bUseOrgRef = false;

  m_pcPicYuvRef          = NULL;

  m_pcOutputSamples      = NULL; 
  m_pcOutputSamplesRow   = NULL;   
  m_iOutputSamplesStride = -1; 

  m_ppiCurLUT            = NULL;
  m_piInvZLUTLeft        = NULL;
  m_piInvZLUTRight       = NULL;

  m_aapiRefVideoPel[0]   = NULL;
  m_aapiRefVideoPel[1]   = NULL;
  m_aapiRefVideoPel[2]   = NULL;

  m_aiRefVideoStrides[0] = -1;
  m_aiRefVideoStrides[1] = -1;
  m_aiRefVideoStrides[2] = -1;


  for (UInt uiViewNum = 0 ; uiViewNum < 2; uiViewNum++)
  {
    // LUT
    m_appiShiftLut[uiViewNum] = NULL;

    m_pcInputSamples[uiViewNum] = NULL; 
    m_iInputSamplesStride       = -1; 

    m_ppiCurLUT               = NULL;
    m_piInvZLUTLeft           = NULL;
    m_piInvZLUTRight          = NULL;
  }

#ifdef LGE_VSO_EARLY_SKIP_A0093
  m_pbHorSkip = NULL;
#endif
}

template <BlenMod iBM, Bool bBitInc>
TRenSingleModelC<iBM,bBitInc>::~TRenSingleModelC()
{
#ifdef LGE_VSO_EARLY_SKIP_A0093
  if ( m_pbHorSkip ) 
  {
    delete[] m_pbHorSkip;
    m_pbHorSkip = NULL;
  }
#endif

  if ( m_pcInputSamples [0] ) delete[] m_pcInputSamples [0];
  if ( m_pcInputSamples [1] ) delete[] m_pcInputSamples [1];

#if FIX_MEM_LEAKS
  if ( m_pcOutputSamples    ) delete[] m_pcOutputSamples   ;
#else
  if ( m_pcOutputSamples    ) delete   m_pcOutputSamples   ;
#endif

#if FIX_MEM_LEAKS
  if ( m_piInvZLUTLeft  ) delete[] m_piInvZLUTLeft ;
  if ( m_piInvZLUTRight ) delete[] m_piInvZLUTRight;

  if ( m_aapiRefVideoPel[0] ) delete[] ( m_aapiRefVideoPel[0] - ( m_aiRefVideoStrides[0] * m_iPad + m_iPad ) );
  if ( m_aapiRefVideoPel[1] ) delete[] ( m_aapiRefVideoPel[1] - ( m_aiRefVideoStrides[1] * m_iPad + m_iPad ) );
  if ( m_aapiRefVideoPel[2] ) delete[] ( m_aapiRefVideoPel[2] - ( m_aiRefVideoStrides[2] * m_iPad + m_iPad ) );
#endif
}

template <BlenMod iBM, Bool bBitInc> Void
#if LGE_VSO_EARLY_SKIP_A0093
TRenSingleModelC<iBM,bBitInc>::create( Int iMode, Int iWidth, Int iHeight, Int iShiftPrec, Int*** aaaiSubPelShiftTable, Int iHoleMargin, Bool bUseOrgRef, Int iBlendMode, Bool bEarlySkip )
#else
TRenSingleModelC<iBM,bBitInc>::create( Int iMode, Int iWidth, Int iHeight, Int iShiftPrec, Int*** aaaiSubPelShiftTable, Int iHoleMargin, Bool bUseOrgRef, Int iBlendMode )
#endif

{
#if LGE_VSO_EARLY_SKIP_A0093
  m_pbHorSkip     = new Bool [MAX_CU_SIZE];
  m_bEarlySkip    = bEarlySkip; 
#endif

  AOF( iBlendMode == iBM ); 

  m_iMode = iMode;

  m_iWidth  = iWidth;
  m_iHeight = iHeight;
  m_iStride = iWidth;

  m_iSampledWidth  = m_iWidth  << iShiftPrec;
  m_iSampledStride = m_iStride << iShiftPrec;

  m_iShiftPrec     = iShiftPrec;
  m_aaiSubPelShiftL = aaaiSubPelShiftTable[0];
  m_aaiSubPelShiftR = aaaiSubPelShiftTable[1];

  if (m_iMode == 2)
  {
    m_piInvZLUTLeft  = new Int[257];
    m_piInvZLUTRight = new Int[257];
  }

  m_iGapTolerance  = ( 2 << iShiftPrec );
  m_iHoleMargin    =  iHoleMargin;

  m_bUseOrgRef = bUseOrgRef;

  m_aiRefVideoStrides[0] = m_iStride + (m_iPad << 1);
  m_aiRefVideoStrides[1] = m_iStride + (m_iPad << 1);
  m_aiRefVideoStrides[2] = m_iStride + (m_iPad << 1);

  m_aapiRefVideoPel  [0] = new Pel[ m_aiRefVideoStrides[0] * (m_iHeight + (m_iPad << 1))];
  m_aapiRefVideoPel  [1] = new Pel[ m_aiRefVideoStrides[1] * (m_iHeight + (m_iPad << 1))];
  m_aapiRefVideoPel  [2] = new Pel[ m_aiRefVideoStrides[2] * (m_iHeight + (m_iPad << 1))];

  m_aapiRefVideoPel  [0] += m_aiRefVideoStrides[0] * m_iPad + m_iPad;
  m_aapiRefVideoPel  [1] += m_aiRefVideoStrides[1] * m_iPad + m_iPad;
  m_aapiRefVideoPel  [2] += m_aiRefVideoStrides[2] * m_iPad + m_iPad;

  m_iInputSamplesStride  = m_iWidth+1;
  m_iOutputSamplesStride = m_iWidth;

  m_pcInputSamples[0]     = new RenModelInPels[m_iInputSamplesStride*m_iHeight];
  m_pcInputSamples[1]     = new RenModelInPels[m_iInputSamplesStride*m_iHeight];

  m_pcOutputSamples       = new RenModelOutPels[m_iOutputSamplesStride*m_iHeight];  
}

template <BlenMod iBM, Bool bBitInc> Void
TRenSingleModelC<iBM,bBitInc>::setLRView( Int iViewPos, Pel** apiCurVideoPel, Int* aiCurVideoStride, Pel* piCurDepthPel, Int iCurDepthStride )
{
  AOF(( iViewPos == 0) || (iViewPos == 1) );

  RenModelInPels* pcCurInputSampleRow = m_pcInputSamples[iViewPos];
  
  Pel* piDRow = piCurDepthPel;
  Pel* piYRow = apiCurVideoPel[0];
#if HHI_VSO_COLOR_PLANES
  Pel* piURow = apiCurVideoPel[1];
  Pel* piVRow = apiCurVideoPel[2];
#endif  


  Int iOffsetX = ( iViewPos == VIEWPOS_RIGHT ) ? 1 : 0;

#if FIX_VSO_SETUP
  for ( Int iPosY = 0; iPosY < m_iUsedHeight; iPosY++ )
#else
  for ( Int iPosY = 0; iPosY < m_iHeight; iPosY++ )
#endif
  {
    if ( iViewPos == VIEWPOS_RIGHT )
    {
      Int iSubPosX = (1 << m_iShiftPrec); 
      pcCurInputSampleRow[0].aiY[iSubPosX] = piYRow[0];
#if HHI_VSO_COLOR_PLANES 
      pcCurInputSampleRow[0].aiU[iSubPosX] = piURow[0];
      pcCurInputSampleRow[0].aiV[iSubPosX] = piVRow[0];
#endif
    }

    for ( Int iPosX = 0; iPosX < m_iWidth; iPosX++ )
    {
      pcCurInputSampleRow[iPosX].iD = piDRow[iPosX];

      for (Int iSubPosX = 0; iSubPosX < (1 << m_iShiftPrec)+1; iSubPosX++ )
      { 
        Int iShift = (iPosX << m_iShiftPrec) + iSubPosX;
        pcCurInputSampleRow[iPosX+iOffsetX].aiY[iSubPosX] = piYRow[iShift];
#if HHI_VSO_COLOR_PLANES 
        pcCurInputSampleRow[iPosX+iOffsetX].aiU[iSubPosX] = piURow[iShift];
        pcCurInputSampleRow[iPosX+iOffsetX].aiV[iSubPosX] = piVRow[iShift];
#endif
      }
    } 

    pcCurInputSampleRow += m_iInputSamplesStride; 

    piDRow += iCurDepthStride;
    piYRow += aiCurVideoStride[0];
#if HHI_VSO_COLOR_PLANES
    piURow += aiCurVideoStride[1];
    piVRow += aiCurVideoStride[2];
#endif
  }

  
  m_aapiBaseVideoPel      [iViewPos] = apiCurVideoPel;
  m_aaiBaseVideoStrides   [iViewPos] = aiCurVideoStride;
  m_apiBaseDepthPel       [iViewPos] = piCurDepthPel;
  m_aiBaseDepthStrides    [iViewPos] = iCurDepthStride;

}
#if FIX_VSO_SETUP
template <BlenMod iBM, Bool bBitInc> Void
TRenSingleModelC<iBM,bBitInc>::setupPart ( UInt uiHorOffset,       Int iUsedHeight )
{
  AOT( iUsedHeight > m_iHeight );   

  m_iUsedHeight =       iUsedHeight; 
  m_iHorOffset  = (Int) uiHorOffset;
}
#endif

template <BlenMod iBM, Bool bBitInc> Void
#if FIX_VSO_SETUP
TRenSingleModelC<iBM,bBitInc>::setup( TComPicYuv* pcOrgVideo, Int** ppiShiftLutLeft, Int** ppiBaseShiftLutLeft, Int** ppiShiftLutRight,  Int** ppiBaseShiftLutRight,  Int iDistToLeft, Bool bKeepReference )
#else
TRenSingleModelC<iBM,bBitInc>::setup( TComPicYuv* pcOrgVideo, Int** ppiShiftLutLeft, Int** ppiBaseShiftLutLeft, Int** ppiShiftLutRight,  Int** ppiBaseShiftLutRight,  Int iDistToLeft, Bool bKeepReference, UInt uiHorOff )
#endif
{
  AOT( !m_bUseOrgRef && pcOrgVideo );
  AOT( (ppiShiftLutLeft  == NULL) && (m_iMode == 0 || m_iMode == 2) );
  AOT( (ppiShiftLutRight == NULL) && (m_iMode == 1 || m_iMode == 2) );

  m_appiShiftLut[0] = ppiShiftLutLeft;
  m_appiShiftLut[1] = ppiShiftLutRight;

  // Copy Reference
  m_pcPicYuvRef = pcOrgVideo;

  if ( pcOrgVideo && !bKeepReference )
  {
#if FIX_VSO_SETUP
    TRenFilter::copy(             pcOrgVideo->getLumaAddr() +  m_iHorOffset       * pcOrgVideo->getStride() , pcOrgVideo->getStride() , m_iWidth,      m_iUsedHeight,      m_aapiRefVideoPel[0], m_aiRefVideoStrides[0]);
    TRenFilter::sampleCUpHorUp(0, pcOrgVideo->getCbAddr()   + (m_iHorOffset >> 1) * pcOrgVideo->getCStride(), pcOrgVideo->getCStride(), m_iWidth >> 1, m_iUsedHeight >> 1, m_aapiRefVideoPel[1], m_aiRefVideoStrides[1]);
    TRenFilter::sampleCUpHorUp(0, pcOrgVideo->getCrAddr()   + (m_iHorOffset >> 1) * pcOrgVideo->getCStride(), pcOrgVideo->getCStride(), m_iWidth >> 1, m_iUsedHeight >> 1, m_aapiRefVideoPel[2], m_aiRefVideoStrides[2]);    
#else
    TRenFilter::copy(             pcOrgVideo->getLumaAddr() +  uiHorOff       * pcOrgVideo->getStride() , pcOrgVideo->getStride() , m_iWidth,      m_iHeight,      m_aapiRefVideoPel[0], m_aiRefVideoStrides[0]);
    TRenFilter::sampleCUpHorUp(0, pcOrgVideo->getCbAddr()   + (uiHorOff >> 1) * pcOrgVideo->getCStride(), pcOrgVideo->getCStride(), m_iWidth >> 1, m_iHeight >> 1, m_aapiRefVideoPel[1], m_aiRefVideoStrides[1]);
    TRenFilter::sampleCUpHorUp(0, pcOrgVideo->getCrAddr()   + (uiHorOff >> 1) * pcOrgVideo->getCStride(), pcOrgVideo->getCStride(), m_iWidth >> 1, m_iHeight >> 1, m_aapiRefVideoPel[2], m_aiRefVideoStrides[2]);    
#endif
    xSetStructRefView();
  }

  // Initial Rendering
  xResetStructError();
  xInitSampleStructs();
#if FIX_VSO_SETUP
  switch ( m_iMode )
  {  
  case 0:   
#if LGE_VSO_EARLY_SKIP_A0093
    xRenderL<true>( 0, 0, m_iWidth, m_iUsedHeight, m_aiBaseDepthStrides[0], m_apiBaseDepthPel[0],false );
#else
    xRenderL<true>( 0, 0, m_iWidth, m_iUsedHeight, m_aiBaseDepthStrides[0], m_apiBaseDepthPel[0] );
#endif   
    break;
  case 1:    
#ifdef LGE_VSO_EARLY_SKIP_A0093
    xRenderR<true>( 0, 0, m_iWidth, m_iUsedHeight, m_aiBaseDepthStrides[1], m_apiBaseDepthPel[1],false);
#else
    xRenderR<true>( 0, 0, m_iWidth, m_iUsedHeight, m_aiBaseDepthStrides[1], m_apiBaseDepthPel[1] );
#endif
    break;
  case 2:
    TRenFilter::setupZLUT( true, 30, iDistToLeft, ppiBaseShiftLutLeft, ppiBaseShiftLutRight, m_iBlendZThres, m_iBlendDistWeight, m_piInvZLUTLeft, m_piInvZLUTRight );
#ifdef LGE_VSO_EARLY_SKIP_A0093
    xRenderL<true>( 0, 0, m_iWidth, m_iUsedHeight, m_aiBaseDepthStrides[0], m_apiBaseDepthPel[0],false);
    xRenderR<true>( 0, 0, m_iWidth, m_iUsedHeight, m_aiBaseDepthStrides[1], m_apiBaseDepthPel[1],false);
#else      
    xRenderL<true>( 0, 0, m_iWidth, m_iUsedHeight, m_aiBaseDepthStrides[0], m_apiBaseDepthPel[0] );
    xRenderR<true>( 0, 0, m_iWidth, m_iUsedHeight, m_aiBaseDepthStrides[1], m_apiBaseDepthPel[1] );
#endif
    break;
  default:
    AOT(true);
  }
#else
  switch ( m_iMode )
  {  
  case 0:   
#if LGE_VSO_EARLY_SKIP_A0093
    xRenderL<true>( 0, 0, m_iWidth, m_iHeight, m_aiBaseDepthStrides[0], m_apiBaseDepthPel[0],false );
#else
    xRenderL<true>( 0, 0, m_iWidth, m_iHeight, m_aiBaseDepthStrides[0], m_apiBaseDepthPel[0] );
#endif   
    break;
  case 1:    
#ifdef LGE_VSO_EARLY_SKIP_A0093
    xRenderR<true>( 0, 0, m_iWidth, m_iHeight, m_aiBaseDepthStrides[1], m_apiBaseDepthPel[1],false);
#else
    xRenderR<true>( 0, 0, m_iWidth, m_iHeight, m_aiBaseDepthStrides[1], m_apiBaseDepthPel[1] );
#endif
    break;
  case 2:
    TRenFilter::setupZLUT( true, 30, iDistToLeft, ppiBaseShiftLutLeft, ppiBaseShiftLutRight, m_iBlendZThres, m_iBlendDistWeight, m_piInvZLUTLeft, m_piInvZLUTRight );
#ifdef LGE_VSO_EARLY_SKIP_A0093
    xRenderL<true>( 0, 0, m_iWidth, m_iHeight, m_aiBaseDepthStrides[0], m_apiBaseDepthPel[0],false);
    xRenderR<true>( 0, 0, m_iWidth, m_iHeight, m_aiBaseDepthStrides[1], m_apiBaseDepthPel[1],false);
#else      
    xRenderL<true>( 0, 0, m_iWidth, m_iHeight, m_aiBaseDepthStrides[0], m_apiBaseDepthPel[0] );
    xRenderR<true>( 0, 0, m_iWidth, m_iHeight, m_aiBaseDepthStrides[1], m_apiBaseDepthPel[1] );
#endif
    break;
  default:
    AOT(true);
  }
#endif

  // Get Rendered View as Reference
  if ( !pcOrgVideo && !bKeepReference )
  {
    xResetStructError();
    xSetStructSynthViewAsRefView();
  }
}

template <BlenMod iBM, Bool bBitInc> Void
#if HHI_VSO_COLOR_PLANES
TRenSingleModelC<iBM,bBitInc>::xGetSampleStrTextPtrs( Int iViewNum, Pel RenModelOutPels::*& rpiSrcY, Pel RenModelOutPels::*& rpiSrcU, Pel RenModelOutPels::*& rpiSrcV )
#else
TRenSingleModelC<iBM,bBitInc>::xGetSampleStrTextPtrs( Int iViewNum, Pel RenModelOutPels::*& rpiSrcY )
#endif
{
  switch ( iViewNum )
  {
  case 0:
    rpiSrcY = &RenModelOutPels::iYLeft;
#if HHI_VSO_COLOR_PLANES  
    rpiSrcU = &RenModelOutPels::iULeft;
    rpiSrcV = &RenModelOutPels::iVLeft;
#endif
    break;
  case 1:
    rpiSrcY = &RenModelOutPels::iYRight;
#if HHI_VSO_COLOR_PLANES  
    rpiSrcU = &RenModelOutPels::iURight;
    rpiSrcV = &RenModelOutPels::iVRight;
#endif
    break;
  case 2:
    rpiSrcY = &RenModelOutPels::iYBlended;
#if HHI_VSO_COLOR_PLANES  
    rpiSrcU = &RenModelOutPels::iUBlended;
    rpiSrcV = &RenModelOutPels::iVBlended;
#endif
    break;
  }
}


template <BlenMod iBM, Bool bBitInc> Void
TRenSingleModelC<iBM,bBitInc>::xGetSampleStrDepthPtrs( Int iViewNum, Pel RenModelOutPels::*& rpiSrcD )
{
  AOT(iViewNum != 0 && iViewNum != 1);  
  rpiSrcD = (iViewNum == 1) ? &RenModelOutPels::iDRight : &RenModelOutPels::iDLeft; 
}


template <BlenMod iBM, Bool bBitInc> Void
TRenSingleModelC<iBM,bBitInc>::xSetStructRefView( )
{
  RenModelOutPels* pcCurOutSampleRow = m_pcOutputSamples;
  
  Pel* piYRow = m_aapiRefVideoPel[0];
#if HHI_VSO_COLOR_PLANES
  Pel* piURow = m_aapiRefVideoPel[1];
  Pel* piVRow = m_aapiRefVideoPel[2];
#endif  

#if FIX_VSO_SETUP
  for ( Int iPosY = 0; iPosY < m_iUsedHeight; iPosY++ )
#else
  for ( Int iPosY = 0; iPosY < m_iHeight; iPosY++ )
#endif
  {
    for ( Int iPosX = 0; iPosX < m_iWidth; iPosX++ )
    {      
      pcCurOutSampleRow[iPosX].iYRef = piYRow[iPosX];
#if HHI_VSO_COLOR_PLANES
      pcCurOutSampleRow[iPosX].iURef = piURow[iPosX];
      pcCurOutSampleRow[iPosX].iVRef = piVRow[iPosX];
#endif
    } 

    pcCurOutSampleRow += m_iOutputSamplesStride; 
    
    piYRow += m_aiRefVideoStrides[0];
#if HHI_VSO_COLOR_PLANES
    piURow += m_aiRefVideoStrides[1];
    piVRow += m_aiRefVideoStrides[2];
#endif
  }
}

template <BlenMod iBM, Bool bBitInc> Void
TRenSingleModelC<iBM,bBitInc>::xResetStructError( )
{
  RenModelOutPels* pcCurOutSampleRow = m_pcOutputSamples;

  for ( Int iPosY = 0; iPosY < m_iHeight; iPosY++ )
  {
    for ( Int iPosX = 0; iPosX < m_iWidth; iPosX++ )
    {      
      pcCurOutSampleRow[iPosX].iError = 0;
    } 
    pcCurOutSampleRow += m_iOutputSamplesStride; 
  }
}

template <BlenMod iBM, Bool bBitInc> Void
TRenSingleModelC<iBM,bBitInc>::xSetStructSynthViewAsRefView( )
{
  AOT( m_iMode < 0 || m_iMode > 2);

  RenModelOutPels* pcCurOutSampleRow = m_pcOutputSamples;

  Pel RenModelOutPels::* piSrcY = NULL;

#if HHI_VSO_COLOR_PLANES  
  Pel RenModelOutPels::* piSrcU = NULL;
  Pel RenModelOutPels::* piSrcV = NULL;
  xGetSampleStrTextPtrs( m_iMode, piSrcY, piSrcU, piSrcV );
#else
  xGetSampleStrTextPtrs( m_iMode, piSrcY );
#endif

#if FIX_VSO_SETUP
  for ( Int iPosY = 0; iPosY < m_iUsedHeight; iPosY++ )
#else
  for ( Int iPosY = 0; iPosY < m_iHeight; iPosY++ )
#endif
  {
    for ( Int iPosX = 0; iPosX < m_iWidth; iPosX++ )
    {      
      pcCurOutSampleRow[iPosX].iYRef = pcCurOutSampleRow[iPosX].*piSrcY;
#if HHI_VSO_COLOR_PLANES
      pcCurOutSampleRow[iPosX].iURef = pcCurOutSampleRow[iPosX].*piSrcU;
      pcCurOutSampleRow[iPosX].iVRef = pcCurOutSampleRow[iPosX].*piSrcV;
#endif
    } 
    pcCurOutSampleRow += m_iOutputSamplesStride; 
  }
}

template <BlenMod iBM, Bool bBitInc> Void
TRenSingleModelC<iBM,bBitInc>::xInitSampleStructs()
{
  RenModelOutPels* pcOutSampleRow      = m_pcOutputSamples;
  RenModelInPels * pcLeftInSampleRow   = m_pcInputSamples[0];
  RenModelInPels * pcRightInSampleRow  = m_pcInputSamples[1];


  for (Int iPosY = 0; iPosY < m_iHeight; iPosY++)
  {
    for (Int iPosX = 0; iPosX < m_iWidth; iPosX++)
    {
      //// Output Samples
      pcOutSampleRow[iPosX].iFilledLeft   = REN_IS_HOLE;
      pcOutSampleRow[iPosX].iFilledRight  = REN_IS_HOLE;

      pcOutSampleRow[iPosX].iDLeft        = 0;
      pcOutSampleRow[iPosX].iDRight       = 0;
      pcOutSampleRow[iPosX].iDBlended     = 0;      
                                     
      // Y Planes                    
      pcOutSampleRow[iPosX].iYLeft        = 0;
      pcOutSampleRow[iPosX].iYRight       = 0;
      pcOutSampleRow[iPosX].iYBlended     = 0;
#if HHI_VSO_COLOR_PLANES             
      // U Planes                    
      pcOutSampleRow[iPosX].iULeft        = 128 << g_uiBitIncrement;
      pcOutSampleRow[iPosX].iURight       = 128 << g_uiBitIncrement;
      pcOutSampleRow[iPosX].iUBlended     = 128 << g_uiBitIncrement;
                                     
      // V Planes                    
      pcOutSampleRow[iPosX].iVLeft        = 128 << g_uiBitIncrement;
      pcOutSampleRow[iPosX].iVRight       = 128 << g_uiBitIncrement;
      pcOutSampleRow[iPosX].iVBlended     = 128 << g_uiBitIncrement;
#endif
      //// Input Samples
      pcLeftInSampleRow [iPosX].bOccluded = false;
      pcRightInSampleRow[iPosX].bOccluded = false;
    }

    pcOutSampleRow     += m_iOutputSamplesStride;
    pcLeftInSampleRow  += m_iInputSamplesStride;
    pcRightInSampleRow += m_iInputSamplesStride;
  }  
}


#ifdef LGE_VSO_EARLY_SKIP_A0093
template <BlenMod iBM, Bool bBitInc> RMDist
TRenSingleModelC<iBM,bBitInc>::getDistDepth( Int iViewPos, Int iStartPosX, Int iStartPosY, Int iWidth, Int iHeight, Int iStride, Pel* piNewData , Pel * piOrgData, Int iOrgStride )
#else
template <BlenMod iBM, Bool bBitInc> RMDist
TRenSingleModelC<iBM,bBitInc>::getDistDepth( Int iViewPos, Int iStartPosX, Int iStartPosY, Int iWidth, Int iHeight, Int iStride, Pel* piNewData )
#endif
{
  RMDist iSSE = 0;
#ifdef LGE_VSO_EARLY_SKIP_A0093
  Bool   bEarlySkip;
#endif
  switch ( iViewPos )
  {
  case 0:
#ifdef LGE_VSO_EARLY_SKIP_A0093
    bEarlySkip = m_bEarlySkip ? xDetectEarlySkipL(iStartPosX,   iStartPosY,   iWidth,   iHeight,   iStride, piNewData, piOrgData, iOrgStride) : false;
    if( !bEarlySkip )
    {
      iSSE = xRenderL<false>( iStartPosX,   iStartPosY,   iWidth,   iHeight,   iStride, piNewData,true );
    }    
#else
    iSSE = xRenderL<false>( iStartPosX,   iStartPosY,   iWidth,   iHeight,   iStride, piNewData );
#endif
    break;
  case 1:
#ifdef LGE_VSO_EARLY_SKIP_A0093
    bEarlySkip = m_bEarlySkip ? xDetectEarlySkipR(iStartPosX,   iStartPosY,   iWidth,   iHeight,   iStride, piNewData, piOrgData, iOrgStride) : false;
    if( !bEarlySkip )
    {
      iSSE = xRenderR<false>( iStartPosX,   iStartPosY,   iWidth,   iHeight,   iStride, piNewData,true );
    }    
#else
    iSSE = xRenderR<false>( iStartPosX,   iStartPosY,   iWidth,   iHeight,   iStride, piNewData );
#endif
    break;
  default:
    assert(0);
  }

  return iSSE;
}
#ifdef LGE_VSO_EARLY_SKIP_A0093
template <BlenMod iBM, Bool bBitInc> Void
TRenSingleModelC<iBM,bBitInc>::setDepth( Int iViewPos, Int iStartPosX, Int iStartPosY, Int iWidth, Int iHeight, Int iStride, Pel* piNewData, Pel* piOrgData, Int iOrgStride )
#else
template <BlenMod iBM, Bool bBitInc> Void
TRenSingleModelC<iBM,bBitInc>::setDepth( Int iViewPos, Int iStartPosX, Int iStartPosY, Int iWidth, Int iHeight, Int iStride, Pel* piNewData )
#endif
{
#ifdef  LGE_VSO_EARLY_SKIP_A0093
  Bool bEarlySkip;
#endif
  switch ( iViewPos )
  {
  case 0:
#ifdef LGE_VSO_EARLY_SKIP_A0093
    bEarlySkip = m_bEarlySkip ? xDetectEarlySkipL(iStartPosX,   iStartPosY,   iWidth,   iHeight,   iStride, piNewData, piOrgData,iOrgStride) : false;
    if( !bEarlySkip )
    {
      xRenderL<true>( iStartPosX,   iStartPosY,   iWidth,   iHeight,   iStride, piNewData,true );
    }    
#else
    xRenderL<true>( iStartPosX,   iStartPosY,   iWidth,   iHeight,   iStride, piNewData );
#endif     
    break;
  case 1:
#ifdef LGE_VSO_EARLY_SKIP_A0093
    bEarlySkip = m_bEarlySkip ? xDetectEarlySkipR(iStartPosX,   iStartPosY,   iWidth,   iHeight,   iStride, piNewData, piOrgData,iOrgStride) : false;
    if( !bEarlySkip )
    {
      xRenderR<true>( iStartPosX,   iStartPosY,   iWidth,   iHeight,   iStride, piNewData,true ); 
    }    
#else
    xRenderR<true>( iStartPosX,   iStartPosY,   iWidth,   iHeight,   iStride, piNewData );
#endif     
    break;
  default:
    assert(0);
  }
}

#if FIX_VSO_SETUP
template <BlenMod iBM, Bool bBitInc> Void
TRenSingleModelC<iBM,bBitInc>::getSynthVideo( Int iViewPos, TComPicYuv* pcPicYuv )
{  
  AOT( pcPicYuv->getWidth() != m_iWidth );
  AOT( pcPicYuv->getHeight() < m_iUsedHeight + m_iHorOffset );

#if HHI_VSO_COLOR_PLANES
  Pel RenModelOutPels::* piText[3] = { NULL, NULL, NULL };
  xGetSampleStrTextPtrs(iViewPos, piText[0], piText[1], piText[2]); 

  // Temp image for chroma down sampling
  PelImage cTempImage( m_iWidth, m_iUsedHeight, 3, 0);

  Int  aiStrides[3]; 
  Pel* apiData  [3]; 

  cTempImage.getDataAndStrides( apiData, aiStrides ); 

  for (UInt uiCurPlane = 0; uiCurPlane < 3; uiCurPlane++ )
  {
    xCopyFromSampleStruct( m_pcOutputSamples, m_iOutputSamplesStride, piText[uiCurPlane], apiData[uiCurPlane], aiStrides[uiCurPlane] , m_iWidth, m_iUsedHeight);
  }  
  xCopy2PicYuv( apiData, aiStrides, pcPicYuv );
#else
  Pel RenModelOutPels::* piY;
  xGetSampleStrTextPtrs(iViewPos, piY); 
  xCopyFromSampleStruct( m_pcOutputSamples, m_iOutputSamplesStride, piY, pcPicYuv->getLumaAddr() + m_iHorOffset * pcPicYuv->getStride(), pcPicYuv->getStride(), m_iWidth, m_iUsedHeight );
  pcPicYuv->setChromaTo( 128 << g_uiBitIncrement );   
#endif  
}
#else
template <BlenMod iBM, Bool bBitInc> Void
TRenSingleModelC<iBM,bBitInc>::getSynthVideo( Int iViewPos, TComPicYuv* pcPicYuv, UInt uiHorOffset )
{  
  AOT( pcPicYuv->getWidth() != m_iWidth );
  AOT( pcPicYuv->getHeight() > m_iHeight + uiHorOffset );

#if HHI_VSO_COLOR_PLANES
  Pel RenModelOutPels::* piText[3] = { NULL, NULL, NULL };
  xGetSampleStrTextPtrs(iViewPos, piText[0], piText[1], piText[2]); 

  // Temp image for chroma down sampling
  PelImage cTempImage( m_iWidth, m_iHeight, 3, 0);

  Int  aiStrides[3]; 
  Pel* apiData  [3]; 

  cTempImage.getDataAndStrides( apiData, aiStrides ); 

  for (UInt uiCurPlane = 0; uiCurPlane < 3; uiCurPlane++ )
  {
    xCopyFromSampleStruct( m_pcOutputSamples, m_iOutputSamplesStride, piText[uiCurPlane], apiData[uiCurPlane], aiStrides[uiCurPlane] , m_iWidth, m_iHeight);
  }  
  xCopy2PicYuv( apiData, aiStrides, pcPicYuv, uiHorOffset );
#else
  Pel RenModelOutPels::* piY;
  xGetSampleStrTextPtrs(iViewPos, piY); 
  xCopyFromSampleStruct( m_pcOutputSamples, m_iOutputSamplesStride, piY, pcPicYuv->getLumaAddr() + uiHorOffset * pcPicYuv->getStride(), pcPicYuv->getStride(), m_iWidth, m_iHeight );
  pcPicYuv->setChromaTo( 128 << g_uiBitIncrement );   
#endif  
}
#endif

#if FIX_VSO_SETUP
template <BlenMod iBM, Bool bBitInc> Void
TRenSingleModelC<iBM,bBitInc>::getSynthDepth( Int iViewPos, TComPicYuv* pcPicYuv )
{  
  AOT( iViewPos != 0 && iViewPos != 1); 
  AOT( pcPicYuv->getWidth()  != m_iWidth  );
  AOT( pcPicYuv->getHeight() < m_iUsedHeight + m_iHorOffset );

  Pel RenModelOutPels::* piD = 0;
  xGetSampleStrDepthPtrs(iViewPos, piD); 
  xCopyFromSampleStruct( m_pcOutputSamples, m_iOutputSamplesStride, piD, pcPicYuv->getLumaAddr() + pcPicYuv->getStride() * m_iHorOffset, pcPicYuv->getStride(), m_iWidth, m_iUsedHeight );
  pcPicYuv->setChromaTo( 128 << g_uiBitIncrement );   
}

#else
template <BlenMod iBM, Bool bBitInc> Void
TRenSingleModelC<iBM,bBitInc>::getSynthDepth( Int iViewPos, TComPicYuv* pcPicYuv, UInt uiHorOff )
{  
  AOT( iViewPos != 0 && iViewPos != 1); 
  AOT( pcPicYuv->getWidth()  != m_iWidth  );
  AOT( pcPicYuv->getHeight() > m_iHeight + uiHorOff );

  Pel RenModelOutPels::* piD = 0;
  xGetSampleStrDepthPtrs(iViewPos, piD); 
  xCopyFromSampleStruct( m_pcOutputSamples, m_iOutputSamplesStride, piD, pcPicYuv->getLumaAddr() + pcPicYuv->getStride() * uiHorOff, pcPicYuv->getStride(), m_iWidth, m_iHeight );
  pcPicYuv->setChromaTo( 128 << g_uiBitIncrement );   
}
#endif

#if FIX_VSO_SETUP
template <BlenMod iBM, Bool bBitInc> Void
TRenSingleModelC<iBM,bBitInc>::getRefVideo ( Int iViewPos, TComPicYuv* pcPicYuv )
{  
  AOT( pcPicYuv->getWidth()  != m_iWidth  );
  AOT( pcPicYuv->getHeight() <  m_iUsedHeight + m_iHorOffset);

#if HHI_VSO_COLOR_PLANES
  Pel RenModelOutPels::* piText[3];
  piText[0] = &RenModelOutPels::iYRef;
  piText[1] = &RenModelOutPels::iURef;
  piText[2] = &RenModelOutPels::iVRef;

  // Temp image for chroma down sampling

  PelImage cTempImage( m_iWidth, m_iUsedHeight, 3, 0);
  Int  aiStrides[3]; 
  Pel* apiData  [3]; 

  cTempImage.getDataAndStrides( apiData, aiStrides ); 

  for (UInt uiCurPlane = 0; uiCurPlane < 3; uiCurPlane++ )
  {
    xCopyFromSampleStruct( m_pcOutputSamples, m_iOutputSamplesStride, piText[uiCurPlane], apiData[uiCurPlane], aiStrides[uiCurPlane] , m_iWidth, m_iUsedHeight);
  }  

  xCopy2PicYuv( apiData, aiStrides, pcPicYuv );
#else
  xCopyFromSampleStruct( m_pcOutputSamples, m_iOutputSamplesStride, &RenModelOutPels::iYRef, pcPicYuv->getLumaAddr() *  pcPicYuv->getStride() + m_iHorOffset, pcPicYuv->getStride(), m_iWidth, m_iUsedHeight );
  pcPicYuv->setChromaTo( 128 << g_uiBitIncrement );   
#endif  
}
#else
template <BlenMod iBM, Bool bBitInc> Void
TRenSingleModelC<iBM,bBitInc>::getRefVideo ( Int iViewPos, TComPicYuv* pcPicYuv, UInt uiHorOffset )
{  
  AOT( pcPicYuv->getWidth()  != m_iWidth  );
  AOT( pcPicYuv->getHeight() >  m_iHeight + uiHorOffset);

#if HHI_VSO_COLOR_PLANES
  Pel RenModelOutPels::* piText[3];
  piText[0] = &RenModelOutPels::iYRef;
  piText[1] = &RenModelOutPels::iURef;
  piText[2] = &RenModelOutPels::iVRef;

  // Temp image for chroma down sampling

  PelImage cTempImage( m_iWidth, m_iHeight, 3, 0);
  Int  aiStrides[3]; 
  Pel* apiData  [3]; 

  cTempImage.getDataAndStrides( apiData, aiStrides ); 

  for (UInt uiCurPlane = 0; uiCurPlane < 3; uiCurPlane++ )
  {
    xCopyFromSampleStruct( m_pcOutputSamples, m_iOutputSamplesStride, piText[uiCurPlane], apiData[uiCurPlane], aiStrides[uiCurPlane] , m_iWidth, m_iHeight);
  } 

  xCopy2PicYuv( apiData, aiStrides, pcPicYuv, uiHorOffset );
#else
  xCopyFromSampleStruct( m_pcOutputSamples, m_iOutputSamplesStride, &RenModelOutPels::iYRef, pcPicYuv->getLumaAddr() *  pcPicYuv->getStride() + uiHorOffset, pcPicYuv->getStride(), m_iWidth, m_iHeight );
  pcPicYuv->setChromaTo( 128 << g_uiBitIncrement );   
#endif  
}
#endif

template <BlenMod iBM, Bool bBitInc> RMDist
TRenSingleModelC<iBM,bBitInc>::getDistVideo( Int iViewPos, Int iPlane, Int iStartPosX, Int iStartPosY, Int iWidth, Int iHeight, Int iStride, Pel* piNewData )
{
  AOF(false);
  return 0;
}

template <BlenMod iBM, Bool bBitInc> Void
TRenSingleModelC<iBM,bBitInc>::setVideo( Int iViewPos, Int iPlane, Int iStartPosX, Int iStartPosY, Int iWidth, Int iHeight, Int iStride, Pel* piNewData )
{
  AOF(false);
}



template <BlenMod iBM, Bool bBitInc> __inline Void
TRenSingleModelC<iBM,bBitInc>::xSetViewRow( Int iPosY )
{
  m_pcInputSamplesRow[0] = m_pcInputSamples[0] + m_iInputSamplesStride  * iPosY;
  m_pcInputSamplesRow[1] = m_pcInputSamples[1] + m_iInputSamplesStride  * iPosY;
  m_pcOutputSamplesRow   = m_pcOutputSamples   + m_iOutputSamplesStride * iPosY;  

}

template <BlenMod iBM, Bool bBitInc> __inline Void
TRenSingleModelC<iBM,bBitInc>::xIncViewRow( )
{
  m_pcInputSamplesRow[0] += m_iInputSamplesStride ;
  m_pcInputSamplesRow[1] += m_iInputSamplesStride ;
  m_pcOutputSamplesRow   += m_iOutputSamplesStride;  
}
#if LGE_VSO_EARLY_SKIP_A0093
template <BlenMod iBM, Bool bBitInc> template<Bool bSet> __inline RMDist 
TRenSingleModelC<iBM,bBitInc>::xRenderL( Int iStartPosX, Int iStartPosY, Int iWidth, Int iHeight, Int iStride, Pel* piNewData, Bool bFast)
#else
template <BlenMod iBM, Bool bBitInc> template<Bool bSet> __inline RMDist 
TRenSingleModelC<iBM,bBitInc>::xRenderL( Int iStartPosX, Int iStartPosY, Int iWidth, Int iHeight, Int iStride, Pel* piNewData)
#endif
{
  const Int iCurViewPos   = 0;
  const Int iOtherViewPos = 1;

  m_iCurViewPos   = iCurViewPos  ; 
  m_iOtherViewPos = iOtherViewPos;

  m_piNewDepthData   = piNewData;
  m_iNewDataWidth    = iWidth;
  m_iStartChangePosX = iStartPosX;

  if ((iWidth == 0) || (iHeight == 0))
    return 0;

  // Get Data
  m_ppiCurLUT      = m_appiShiftLut   [iCurViewPos];
  xSetViewRow      ( iStartPosY);

  // Init Start
  RMDist iError = 0;
  Int   iStartChangePos;

  iStartChangePos = m_iStartChangePosX;

  for (Int iPosY = iStartPosY; iPosY < iStartPosY + iHeight; iPosY++ )
  {
#ifdef LGE_VSO_EARLY_SKIP_A0093
    if( m_bEarlySkip && bFast )
    {
      if ( m_pbHorSkip[iPosY-iStartPosY] )
      {
        xIncViewRow();
        m_piNewDepthData += iStride;
        continue;
      }
    }
#endif
    m_bInOcclusion = false;

    Int iLastSPos;
    Int iEndChangePos         = m_iStartChangePosX + iWidth - 1;
    Int iPosXinNewData        = iWidth - 1;
    Int iMinChangedSPos       = m_iSampledWidth;

    if ( iEndChangePos == ( m_iWidth -1 )) // Special processing for rightmost depth sample
    {
      m_iCurDepth           = m_piNewDepthData[iPosXinNewData];
      Int iCurSPos          = xShiftNewData(iEndChangePos, iPosXinNewData);
      m_iLastOccludedSPos   = iCurSPos + 1;
      m_iLastOccludedSPosFP = xRangeLeftL( m_iLastOccludedSPos );
      xExtrapolateMarginL<bSet>  ( iCurSPos, iEndChangePos, iError );

      iMinChangedSPos       = Min( iMinChangedSPos, (iEndChangePos << m_iShiftPrec) - m_ppiCurLUT[0][ RenModRemoveBitInc( Max(m_pcInputSamplesRow[iCurViewPos][iEndChangePos].iD, m_piNewDepthData[iPosXinNewData] )) ]);
      iLastSPos             = iCurSPos;
      m_iLastDepth          = m_iCurDepth;

      if ( bSet )
      {
        m_pcInputSamplesRow[iCurViewPos][iEndChangePos].iD = m_piNewDepthData[iPosXinNewData];
      }

      iPosXinNewData--;
      iEndChangePos--;
    }
    else
    {
      iLastSPos    = xShift(iEndChangePos+1);
      m_iLastDepth = m_pcInputSamplesRow [iCurViewPos][iEndChangePos+1].iD;
      xInitRenderPartL( iEndChangePos, iLastSPos );
    }

    //// RENDER NEW DATA
    Int iCurPosX;
    for ( iCurPosX = iEndChangePos; iCurPosX >= iStartChangePos; iCurPosX-- )
    {
      // Get minimal changed sample position

      iMinChangedSPos = Min( iMinChangedSPos, (iCurPosX << m_iShiftPrec) - m_ppiCurLUT[0][ RenModRemoveBitInc( Max(m_pcInputSamplesRow[iCurViewPos][iCurPosX].iD, m_piNewDepthData[iPosXinNewData] )) ]);
      Int iCurSPos    = xShiftNewData(iCurPosX,iPosXinNewData);
      m_iCurDepth     = m_piNewDepthData[iPosXinNewData];
      xRenderRangeL<bSet>(iCurSPos, iLastSPos, iCurPosX, iError );
      iLastSPos       = iCurSPos;
      m_iLastDepth    = m_iCurDepth;

      if ( bSet )
      {
        m_pcInputSamplesRow[iCurViewPos][iCurPosX].iD = m_piNewDepthData[iPosXinNewData];
      }

      iPosXinNewData--;
    }

    //// RE-RENDER DATA LEFT TO NEW DATA
    while ( iCurPosX >= 0 )
    {
      Int iCurSPos = xShift(iCurPosX);

      m_iCurDepth  = m_pcInputSamplesRow[iCurViewPos][iCurPosX].iD;
      xRenderRangeL<bSet>( iCurSPos, iLastSPos, iCurPosX, iError );

      if ( iCurSPos < iMinChangedSPos )
      {
          break;
      }

      iCurPosX--;
      iLastSPos    = iCurSPos;
      m_iLastDepth = m_iCurDepth;
    }


    xIncViewRow();
    m_piNewDepthData += iStride;
  }
  return iError;
}

#ifdef  LGE_VSO_EARLY_SKIP_A0093
template <BlenMod iBM, Bool bBitInc> template<Bool bSet> __inline RMDist
TRenSingleModelC<iBM,bBitInc>::xRenderR( Int iStartPosX, Int iStartPosY, Int iWidth, Int iHeight, Int iStride, Pel* piNewData , Bool bFast)
#else
template <BlenMod iBM, Bool bBitInc> template<Bool bSet> __inline RMDist
TRenSingleModelC<iBM,bBitInc>::xRenderR( Int iStartPosX, Int iStartPosY, Int iWidth, Int iHeight, Int iStride, Pel* piNewData )
#endif
{

  const Int iCurViewPos   = 1;
  const Int iOtherViewPos = 0;

  m_iCurViewPos      = iCurViewPos;
  m_iOtherViewPos    = iOtherViewPos;

  m_piNewDepthData   = piNewData;
  m_iNewDataWidth    = iWidth;
  m_iStartChangePosX = iStartPosX;

  if ((iWidth == 0) || (iHeight == 0))
    return 0;

  // Get Data
  m_ppiCurLUT      = m_appiShiftLut   [iCurViewPos];
  xSetViewRow      ( iStartPosY);

  // Init Start
  RMDist iError = 0;
  Int   iEndChangePos;

  iEndChangePos = m_iStartChangePosX + iWidth - 1;

  for (Int iPosY = iStartPosY; iPosY < iStartPosY + iHeight; iPosY++ )
  {
#ifdef LGE_VSO_EARLY_SKIP_A0093
    if( m_bEarlySkip && bFast )
    {
      if ( m_pbHorSkip[iPosY-iStartPosY] )
      {
        xIncViewRow();
        m_piNewDepthData += iStride;
        continue;
      }
    }
#endif
    m_bInOcclusion = false;

    Int iLastSPos;
    Int iStartChangePos       = m_iStartChangePosX;
    Int iPosXinNewData        = 0;
    Int iMaxChangedSPos = -1;

    if ( iStartChangePos == 0 ) // Special processing for leftmost depth sample
    {
      m_iCurDepth           = m_piNewDepthData[iPosXinNewData];
      Int iCurSPos          = xShiftNewData(iStartChangePos, iPosXinNewData);
      m_iLastOccludedSPos   = iCurSPos - 1;
      m_iLastOccludedSPosFP = xRangeRightR( m_iLastOccludedSPos );
      xExtrapolateMarginR<bSet>     ( iCurSPos, iStartChangePos, iError );

      iMaxChangedSPos       = Max( iMaxChangedSPos, (iStartChangePos << m_iShiftPrec) - m_ppiCurLUT[0][ RenModRemoveBitInc( Max(m_pcInputSamplesRow[iCurViewPos][iStartChangePos].iD, m_piNewDepthData[iPosXinNewData] )) ]);
      iLastSPos             = iCurSPos;
      m_iLastDepth          = m_iCurDepth;
      if ( bSet )
      {
        m_pcInputSamplesRow[iCurViewPos][iStartChangePos].iD = m_piNewDepthData[iPosXinNewData];
      }


      iPosXinNewData++;
      iStartChangePos++;
    }
    else
    {
      iLastSPos   = xShift(iStartChangePos-1);

      m_iLastDepth = m_pcInputSamplesRow[iCurViewPos][iStartChangePos-1].iD;
      xInitRenderPartR( iStartChangePos, iLastSPos );
    }

    //// RENDER NEW DATA
    Int iCurPosX;
    for ( iCurPosX = iStartChangePos; iCurPosX <= iEndChangePos; iCurPosX++ )
    {
      // Get minimal changed sample position

      iMaxChangedSPos = Max( iMaxChangedSPos, (iCurPosX << m_iShiftPrec) - m_ppiCurLUT[0][ RenModRemoveBitInc( Max(m_pcInputSamplesRow[iCurViewPos][iCurPosX].iD, m_piNewDepthData[iPosXinNewData] )) ]);
      Int iCurSPos    = xShiftNewData(iCurPosX,iPosXinNewData);
      m_iCurDepth     = m_piNewDepthData[iPosXinNewData];
      xRenderRangeR<bSet>(iCurSPos, iLastSPos, iCurPosX, iError );
      iLastSPos      = iCurSPos;
      m_iLastDepth    = m_iCurDepth;

      if ( bSet )
      {
        m_pcInputSamplesRow[iCurViewPos][iCurPosX].iD = m_piNewDepthData[iPosXinNewData];
      }

      iPosXinNewData++;
    }

    //// RE-RENDER DATA LEFT TO NEW DATA
    while ( iCurPosX < m_iWidth )
    {
      Int iCurSPos = xShift(iCurPosX);

      m_iCurDepth  = m_pcInputSamplesRow[iCurViewPos][iCurPosX].iD;
      xRenderRangeR<bSet>( iCurSPos, iLastSPos, iCurPosX, iError );

      if ( iCurSPos > iMaxChangedSPos )
      {
          break;
        }
      iCurPosX++;
      iLastSPos    = iCurSPos;
      m_iLastDepth = m_iCurDepth;
    }

    xIncViewRow();
    m_piNewDepthData += iStride;
  }
  return iError;
}


template <BlenMod iBM, Bool bBitInc> __inline Void
TRenSingleModelC<iBM,bBitInc>::xInitRenderPartL(  Int iEndChangePos, Int iLastSPos )
{
  const Int iCurViewPos = 0; 
  // GET MINIMAL OCCLUDED SAMPLE POSITION
  Int iCurPosX           = iEndChangePos;


  if ( ( iCurPosX + 1 < m_iWidth ) && (m_pcInputSamplesRow[iCurViewPos][ iCurPosX + 1].bOccluded ) )
  {
    iCurPosX++;

    while ( (iCurPosX + 1 < m_iWidth) &&  (m_pcInputSamplesRow[iCurViewPos][ iCurPosX + 1].bOccluded  )  )

      iCurPosX++;

    if ( iCurPosX + 1 < m_iWidth )
    {
      iCurPosX++;
      m_iLastOccludedSPos = xShift(iCurPosX);
    }
    else
    {
      m_iLastOccludedSPos = xShift(iCurPosX) + 1;
    }

    m_iLastOccludedSPosFP = xRoundL( m_iLastOccludedSPos );
  }
  else
  {
    m_iLastOccludedSPos   = iLastSPos+1;
    m_iLastOccludedSPosFP = xRangeLeftL( m_iLastOccludedSPos );
  }

  m_bInOcclusion = iLastSPos >= m_iLastOccludedSPos;
};

template <BlenMod iBM, Bool bBitInc> __inline Void
TRenSingleModelC<iBM,bBitInc>::xInitRenderPartR(  Int iStartChangePos, Int iLastSPos )
{
    const Int iCurViewPos = 1; 
  // GET MINIMAL OCCLUDED SAMPLE POSITION
  Int iCurPosX           = iStartChangePos;

  if ( ( iCurPosX - 1 > -1 ) && (m_pcInputSamplesRow[iCurViewPos][ iCurPosX - 1].bOccluded  ) )
  {
    iCurPosX--;

    while ( (iCurPosX - 1 > -1 ) &&  (m_pcInputSamplesRow[iCurViewPos][ iCurPosX - 1].bOccluded  )  )
      iCurPosX--;

    if ( iCurPosX - 1 > -1 )
    {
      iCurPosX--;
      m_iLastOccludedSPos = xShift(iCurPosX);
    }
    else
    {
      m_iLastOccludedSPos = xShift(iCurPosX) - 1;
    }
    m_iLastOccludedSPosFP = xRoundR( m_iLastOccludedSPos );
  }
  else
  {
    m_iLastOccludedSPos   = iLastSPos-1;
    m_iLastOccludedSPosFP = xRangeRightR( m_iLastOccludedSPos );
  }

  m_bInOcclusion = iLastSPos <= m_iLastOccludedSPos;
};


template <BlenMod iBM, Bool bBitInc> template<Bool bSet> __inline Void
TRenSingleModelC<iBM,bBitInc>::xRenderShiftedRangeL(Int iCurSPos, Int iLastSPos, Int iCurPos, RMDist& riError )
{
  assert( iCurSPos <= iLastSPos );
  //assert( iRightSPos < m_iWidth );

  Int iDeltaSPos = iLastSPos - iCurSPos;
  if ( iDeltaSPos > m_iGapTolerance )
  {
    xFillHoleL<bSet>( iCurSPos, iLastSPos, iCurPos, riError );
  }
  else
  {
    if (iLastSPos < 0 )
      return;

    RM_AOT( iDeltaSPos    > m_iGapTolerance );

    m_iThisDepth = m_iCurDepth;
    for (Int iFillSPos = Max(0, xRangeLeftL(iCurSPos) ); iFillSPos <= min(xRangeRightL( iLastSPos ) ,m_iLastOccludedSPosFP-1); iFillSPos++ )
    {
      Int iDeltaCurSPos  = (iFillSPos << m_iShiftPrec) - iCurSPos;

      RM_AOT( iDeltaCurSPos > iDeltaSPos );
      RM_AOT( iDeltaCurSPos < 0 );
      RM_AOT( m_aaiSubPelShiftL[iDeltaSPos][iDeltaCurSPos] == 0xdeaddead);

      xSetShiftedPelL<bSet>( iCurPos, m_aaiSubPelShiftL[iDeltaSPos][iDeltaCurSPos], iFillSPos, REN_IS_FILLED, riError );
    }
  };
}

template <BlenMod iBM, Bool bBitInc> template<Bool bSet> __inline Void
TRenSingleModelC<iBM,bBitInc>::xRenderShiftedRangeR(Int iCurSPos, Int iLastSPos, Int iCurPos, RMDist& riError )
{
  assert( iCurSPos >= iLastSPos );

  Int iDeltaSPos = iCurSPos - iLastSPos;
  if ( iDeltaSPos > m_iGapTolerance )
  {
    xFillHoleR<bSet>( iCurSPos, iLastSPos, iCurPos, riError );
  }
  else
  {
    if (iLastSPos > m_iSampledWidth - 1 )
      return;

    m_iThisDepth = m_iCurDepth;
    RM_AOT( iDeltaSPos    > m_iGapTolerance );
    for (Int iFillSPos = max(m_iLastOccludedSPosFP+1, xRangeLeftR(iLastSPos) ); iFillSPos <= min(xRangeRightR( iCurSPos ) ,m_iWidth -1); iFillSPos++ )
    {
      Int iDeltaCurSPos  = (iFillSPos << m_iShiftPrec) - iLastSPos;

      RM_AOT( iDeltaCurSPos > iDeltaSPos );
      RM_AOT( iDeltaCurSPos < 0 );
      RM_AOT( m_aaiSubPelShiftR[iDeltaSPos][iDeltaCurSPos] == 0xdeaddead);

      xSetShiftedPelR<bSet>( iCurPos, m_aaiSubPelShiftR[iDeltaSPos][iDeltaCurSPos], iFillSPos, REN_IS_FILLED, riError );
    }
  };
}



template <BlenMod iBM, Bool bBitInc> template<Bool bSet> __inline Void
TRenSingleModelC<iBM,bBitInc>::xRenderRangeL(Int iCurSPos, Int iLastSPos, Int iCurPos, RMDist& riError )
{
  const Int iCurViewPos = 0; 
  if (  !m_bInOcclusion )
  {
    if ( iCurSPos >= iLastSPos )
    {
      m_iLastOccludedSPos = iLastSPos;

      Int iRightSPosFP = xRoundL( iLastSPos );
      if ( ( iRightSPosFP == xRangeRightL(iLastSPos)) && (iRightSPosFP >= 0) )
      {
        m_iThisDepth = m_iLastDepth;

        xSetShiftedPelL<bSet>( iCurPos+1, 0, iRightSPosFP, REN_IS_FILLED, riError );
      }
      m_iLastOccludedSPosFP = iRightSPosFP;

      m_bInOcclusion = true;

      if ( bSet )
      {
        m_pcInputSamplesRow[iCurViewPos][ iCurPos ].bOccluded  = true;
      }
    }
    else
    {
      if ( bSet )
      {
        m_pcInputSamplesRow[iCurViewPos][ iCurPos ].bOccluded  = false;
      }

      xRenderShiftedRangeL<bSet>(iCurSPos, iLastSPos, iCurPos, riError );
    }
  }
  else
  {
    if ( iCurSPos < m_iLastOccludedSPos )
    {
      m_bInOcclusion = false;
      if ( bSet )
      {
        m_pcInputSamplesRow[iCurViewPos][ iCurPos ].bOccluded  = false;
      }

      xRenderShiftedRangeL<bSet>(iCurSPos, iLastSPos, iCurPos, riError );
    }
    else
    {
      if ( bSet )
      {
        m_pcInputSamplesRow[iCurViewPos][ iCurPos ].bOccluded  = true;
      }
    }
  }
}

template <BlenMod iBM, Bool bBitInc> template<Bool bSet> __inline Void
TRenSingleModelC<iBM,bBitInc>::xRenderRangeR(Int iCurSPos, Int iLastSPos, Int iCurPos, RMDist& riError )
{
  const Int iCurViewPos = 1; 
  // Find out if current sample is occluded
  if (  !m_bInOcclusion )
  {
    if ( iCurSPos <= iLastSPos )
    {
      m_iLastOccludedSPos = iLastSPos;

      Int iLeftSPosFP = xRoundR( iLastSPos );
      if ( ( iLeftSPosFP == xRangeLeftR(iLastSPos)) && (iLeftSPosFP <= m_iWidth - 1) )
      {
        m_iThisDepth = m_iLastDepth;
        xSetShiftedPelR<bSet>( iCurPos-1,1 << m_iShiftPrec , iLeftSPosFP, REN_IS_FILLED, riError );
      }
      m_iLastOccludedSPosFP = iLeftSPosFP;

      m_bInOcclusion = true;

      if ( bSet )
      {
        m_pcInputSamplesRow[iCurViewPos][ iCurPos ].bOccluded  = true;
      }
    }
    else
    {
      if ( bSet )
      {
        m_pcInputSamplesRow[iCurViewPos][ iCurPos ].bOccluded  = false;
      }

      xRenderShiftedRangeR<bSet>(iCurSPos, iLastSPos, iCurPos, riError );
    }
  }
  else
  {
    if ( iCurSPos > m_iLastOccludedSPos )
    {
      m_bInOcclusion = false;
      if ( bSet )
      {
        m_pcInputSamplesRow[iCurViewPos][ iCurPos ].bOccluded  = false;
      }

      xRenderShiftedRangeR<bSet>(iCurSPos, iLastSPos, iCurPos, riError );
    }
    else
    {
      if ( bSet )
      {
        m_pcInputSamplesRow[iCurViewPos][ iCurPos ].bOccluded  = true;
      }
    }
  }
}

template <BlenMod iBM, Bool bBitInc> template<Bool bSet> __inline Void
TRenSingleModelC<iBM,bBitInc>::xFillHoleL( Int iCurSPos, Int iLastSPos, Int iCurPos, RMDist& riError )
{
  if (iLastSPos < 0)
    return;

  Int iStartFillSPos = iCurSPos;
  Int iStartFillPos  = iCurPos;
  Int iLastPos      = iCurPos + 1;

  Int iStartFillSPosFP = xRangeLeftL(iStartFillSPos);

  if (iStartFillSPosFP == xRoundL(iStartFillSPos))
  {
    if ((iStartFillSPosFP >= 0) && (iStartFillSPosFP < m_iLastOccludedSPosFP) )
    {
      m_iThisDepth = m_iCurDepth;
      xSetShiftedPelL<bSet>    ( iStartFillPos, 0, iStartFillSPosFP, REN_IS_FILLED, riError );
    }
  }
  else
  {
    iStartFillSPosFP--;
  }

  m_iThisDepth = m_iLastDepth;
  for (Int iFillSPos = Max(iStartFillSPosFP+1,0); iFillSPos <= min(xRangeRightL( iLastSPos ), m_iLastOccludedSPosFP-1 ); iFillSPos++ )
  {
    xSetShiftedPelL<bSet>( iLastPos, 0,  iFillSPos, REN_IS_HOLE, riError );
  }
}

template <BlenMod iBM, Bool bBitInc> template<Bool bSet> __inline Void
TRenSingleModelC<iBM,bBitInc>::xFillHoleR( Int iCurSPos, Int iLastSPos, Int iCurPos, RMDist& riError )
{
  if (iLastSPos < 0)
    return;

  Int iStartFillSPos = iCurSPos;
  Int iEndFillPos    = iCurPos;
  Int iLastPos       = iCurPos - 1;

  Int iStartFillSPosFP = xRangeRightR(iStartFillSPos);

  if (iStartFillSPosFP == xRoundR(iStartFillSPos))
  {
    if ((iStartFillSPosFP < m_iWidth) && (iStartFillSPosFP > m_iLastOccludedSPosFP) )
    {
      m_iThisDepth = m_iCurDepth;
      xSetShiftedPelR<bSet>( iEndFillPos, 1 << m_iShiftPrec , iStartFillSPosFP, REN_IS_FILLED, riError );
    }
  }
  else
  {
    iStartFillSPosFP++;
  }

  m_iThisDepth = m_iLastDepth;
  for (Int iFillSPos = max(xRangeLeftR( iLastSPos ), m_iLastOccludedSPosFP+1); iFillSPos <= min(iStartFillSPosFP,m_iWidth)-1 ; iFillSPos++ )
  {
    xSetShiftedPelR<bSet>( iLastPos, 1 << m_iShiftPrec, iFillSPos, REN_IS_HOLE, riError );
  }
}

template <BlenMod iBM, Bool bBitInc> template<Bool bSet> __inline Void
TRenSingleModelC<iBM,bBitInc>::xExtrapolateMarginL(Int iCurSPos, Int iCurPos, RMDist& riError )
{
//  if (iLeftSPos < 0 )
//    return;

  Int iSPosFullPel = Max(0,xRangeLeftL(iCurSPos));

  m_iThisDepth = m_iCurDepth;
  if (iSPosFullPel < m_iWidth)
  {
    xSetShiftedPelL<bSet>( iCurPos, 0, iSPosFullPel, REN_IS_FILLED, riError );
  }

  for (Int iFillSPos = iSPosFullPel +1; iFillSPos < m_iWidth; iFillSPos++ )
  {
    xSetShiftedPelL<bSet>( iCurPos, 0, iFillSPos, REN_IS_HOLE, riError );
  }
}

template <BlenMod iBM, Bool bBitInc> template<Bool bSet> __inline Void
TRenSingleModelC<iBM,bBitInc>::xExtrapolateMarginR(Int iCurSPos, Int iCurPos, RMDist& riError )
{
  //  if (iLeftSPos < 0 )
  //    return;

  Int iSPosFullPel = Min(m_iWidth-1,xRangeRightR(iCurSPos));

  m_iThisDepth = m_iCurDepth;
  if (iSPosFullPel > -1)
  {
    xSetShiftedPelR<bSet>( iCurPos, 1 << m_iShiftPrec, iSPosFullPel, REN_IS_FILLED, riError );
  }

  for (Int iFillSPos = iSPosFullPel -1; iFillSPos > -1; iFillSPos-- )
  {
    xSetShiftedPelR<bSet>( iCurPos , 1 << m_iShiftPrec, iFillSPos, REN_IS_HOLE, riError );
  }
}

template <BlenMod iBM, Bool bBitInc> __inline Int
TRenSingleModelC<iBM,bBitInc>::xShiftNewData( Int iPosX, Int iPosInNewData )
{
  RM_AOT( iPosInNewData <               0 );
  RM_AOF( iPosInNewData < m_iNewDataWidth );

  return (iPosX << m_iShiftPrec) - m_ppiCurLUT[0][ RenModRemoveBitInc( m_piNewDepthData[iPosInNewData] )];
}

template <BlenMod iBM, Bool bBitInc> __inline Int
TRenSingleModelC<iBM,bBitInc>::xShift( Int iPosX )
{
 RM_AOT( iPosX <        0);
 RM_AOF( iPosX < m_iWidth);

 return (iPosX  << m_iShiftPrec) - m_ppiCurLUT[0][ RenModRemoveBitInc( m_pcInputSamplesRow[m_iCurViewPos][iPosX].iD )];
}


template <BlenMod iBM, Bool bBitInc> __inline Int
TRenSingleModelC<iBM,bBitInc>::xShift( Int iPos, Int iPosInNewData )
{
  if ( (iPosInNewData >= 0) && (iPosInNewData < m_iNewDataWidth) )
  {
    return xShiftNewData(iPos ,iPosInNewData );
  }
  else
  {
    return xShift(iPos);
  }
}

template <BlenMod iBM, Bool bBitInc> __inline Int
TRenSingleModelC<iBM,bBitInc>::xRangeLeftL( Int iPos )
{
  return  ( iPos +  (1 << m_iShiftPrec) - 1) >> m_iShiftPrec;
}


template <BlenMod iBM, Bool bBitInc> __inline Int
TRenSingleModelC<iBM,bBitInc>::xRangeLeftR( Int iPos )
{

  return  xRangeRightR( iPos ) + 1;
}


template <BlenMod iBM, Bool bBitInc> __inline Int
TRenSingleModelC<iBM,bBitInc>::xRangeRightL( Int iPos )
{
  return xRangeLeftL(iPos) - 1;
}

template <BlenMod iBM, Bool bBitInc> __inline Int
TRenSingleModelC<iBM,bBitInc>::xRangeRightR( Int iPos )
{
  return iPos >> m_iShiftPrec;
}


template <BlenMod iBM, Bool bBitInc> __inline Int
TRenSingleModelC<iBM,bBitInc>::xRoundL( Int iPos )
{
  return  (iPos + (( 1 << m_iShiftPrec ) >> 1 )) >> m_iShiftPrec;
}

template <BlenMod iBM, Bool bBitInc> __inline Int
TRenSingleModelC<iBM,bBitInc>::xRoundR( Int iPos )
{
  return  (m_iShiftPrec == 0) ? iPos : xRoundL(iPos - 1);
}


template <BlenMod iBM, Bool bBitInc> Void
TRenSingleModelC<iBM,bBitInc>::xSetPels( Pel* piPelSource , Int iSourceStride, Int iWidth, Int iHeight, Pel iVal )
{
  for (Int iYPos = 0; iYPos < iHeight; iYPos++)
  {
    for (Int iXPos = 0; iXPos < iWidth; iXPos++)
    {
      piPelSource[iXPos] = iVal;
    }
    piPelSource += iSourceStride;
  }
}

template <BlenMod iBM, Bool bBitInc> Void
TRenSingleModelC<iBM,bBitInc>::xSetInts( Int* piPelSource , Int iSourceStride, Int iWidth, Int iHeight, Int iVal )
{
  for (Int iYPos = 0; iYPos < iHeight; iYPos++)
  {
    for (Int iXPos = 0; iXPos < iWidth; iXPos++)
    {
      piPelSource[iXPos] = iVal;
    }
    piPelSource += iSourceStride;
  }
}


template <BlenMod iBM, Bool bBitInc> Void
TRenSingleModelC<iBM,bBitInc>::xSetBools( Bool* pbPelSource , Int iSourceStride, Int iWidth, Int iHeight, Bool bVal )
{
  for (Int iYPos = 0; iYPos < iHeight; iYPos++)
  {
    for (Int iXPos = 0; iXPos < iWidth; iXPos++)
    {
      pbPelSource[iXPos] = bVal;
    }
    pbPelSource += iSourceStride;
  }
}

template <BlenMod iBM, Bool bBitInc> template<Bool bSet> __inline Void
TRenSingleModelC<iBM,bBitInc>::xSetShiftedPelL(Int iSourcePos, Int iSubSourcePos, Int iTargetSPos, Pel iFilled, RMDist& riError )
{
  RM_AOT( iSourcePos    <  0                   );
  RM_AOT( iSourcePos    >= m_iWidth            );
  RM_AOT( iSubSourcePos < 0                    );
  RM_AOT( iSubSourcePos >  (1 << m_iShiftPrec) );
  RM_AOT( iTargetSPos   < 0                    );
  RM_AOT( iTargetSPos   >= m_iWidth            );  

  RenModelOutPels* pcOutSample = m_pcOutputSamplesRow              + iTargetSPos;
  RenModelInPels * pcInSample  = m_pcInputSamplesRow[VIEWPOS_LEFT] + iSourcePos ;

  if ( iBM != BLEND_NONE )
  {
    xSetShiftedPelBlendL<bSet>  (pcInSample, iSubSourcePos, pcOutSample, iFilled, riError);
  }
  else
  {
    xSetShiftedPelNoBlendL<bSet>(pcInSample, iSubSourcePos, pcOutSample, iFilled, riError);
  }
}

template <BlenMod iBM, Bool bBitInc> template<Bool bSet> __inline Void
TRenSingleModelC<iBM,bBitInc>::xSetShiftedPelNoBlendL(RenModelInPels* pcInSample, Int iSubSourcePos, RenModelOutPels* pcOutSample, Pel iFilled, RMDist& riError )
{
  if ( bSet )
  { 
    // Filled
    pcOutSample->iFilledLeft = iFilled;

    // Yuv
    pcOutSample->iYLeft  = pcInSample->aiY[iSubSourcePos];
#if HHI_VSO_COLOR_PLANES
    pcOutSample->iULeft  = pcInSample->aiU[iSubSourcePos];
    pcOutSample->iVLeft  = pcInSample->aiV[iSubSourcePos];

    pcOutSample->iError = xGetDist( pcOutSample->iYLeft - pcOutSample->iYRef,
                                    pcOutSample->iULeft - pcOutSample->iURef,    
                                    pcOutSample->iVLeft - pcOutSample->iVRef
                                  );    
#else
    pcOutSample->iError = xGetDist( pcOutSample->iYLeft - pcOutSample->iYRef );    
#endif    
    
  }
  else
  { 
#if HHI_VSO_COLOR_PLANES
    riError += xGetDist( pcInSample->aiY[iSubSourcePos] - pcOutSample->iYRef,
                         pcInSample->aiU[iSubSourcePos] - pcOutSample->iURef,
                         pcInSample->aiV[iSubSourcePos] - pcOutSample->iVRef
                       );
#else               
    riError += xGetDist( pcInSample->aiY[iSubSourcePos] - pcOutSample->iYRef );
#endif

    riError -= pcOutSample->iError;
  }
}

template <BlenMod iBM, Bool bBitInc> template<Bool bSet> __inline Void
TRenSingleModelC<iBM,bBitInc>::xSetShiftedPelBlendL(RenModelInPels* pcInSample, Int iSubSourcePos, RenModelOutPels* pcOutSample, Pel iFilled, RMDist& riError )
{
  Pel piBlendedValueY;
#if HHI_VSO_COLOR_PLANES
  Pel piBlendedValueU;
  Pel piBlendedValueV;
#endif

  xGetBlendedValue (
    pcInSample ->aiY[iSubSourcePos],
    pcOutSample->iYRight,    
#if HHI_VSO_COLOR_PLANES
    pcInSample ->aiU[iSubSourcePos],
    pcOutSample->iURight,    
    pcInSample ->aiV[iSubSourcePos],
    pcOutSample->iVRight,    
#endif
    m_piInvZLUTLeft [RenModRemoveBitInc(m_iThisDepth)        ],
    m_piInvZLUTRight[RenModRemoveBitInc(pcOutSample->iDRight)],
    iFilled,
    pcOutSample->iFilledRight  ,
    piBlendedValueY
#if HHI_VSO_COLOR_PLANES
    , piBlendedValueU,
    piBlendedValueV
#endif
    );

  if ( bSet )
  {    
    // Set values
    pcOutSample->iDLeft      = m_iThisDepth;
    pcOutSample->iYLeft      = pcInSample ->aiY[iSubSourcePos];
    pcOutSample->iYBlended   = piBlendedValueY;    
#if HHI_VSO_COLOR_PLANES  
    pcOutSample->iULeft      = pcInSample ->aiU[iSubSourcePos];
    pcOutSample->iUBlended   = piBlendedValueU;    
    pcOutSample->iVLeft      = pcInSample ->aiV[iSubSourcePos];
    pcOutSample->iVBlended   = piBlendedValueV;    
#endif
    pcOutSample->iFilledLeft = iFilled;

    // Get Error
    Int iDiffY = pcOutSample->iYRef - piBlendedValueY;
#if HHI_VSO_COLOR_PLANES
    Int iDiffU = pcOutSample->iURef - piBlendedValueU;
    Int iDiffV = pcOutSample->iVRef - piBlendedValueV;
    pcOutSample->iError  = xGetDist(iDiffY, iDiffU, iDiffV );
#else
    pcOutSample->iError  = xGetDist(iDiffY );
#endif
  }
  else
  {
    Int iDiffY = pcOutSample->iYRef - piBlendedValueY;
#if HHI_VSO_COLOR_PLANES
    Int iDiffU = pcOutSample->iURef - piBlendedValueU;
    Int iDiffV = pcOutSample->iVRef - piBlendedValueV;
    riError   += ( xGetDist( iDiffY, iDiffU, iDiffV ) - pcOutSample->iError );

#else
    riError   += ( xGetDist( iDiffY ) - pcOutSample->iError  );
#endif

  }
}


template <BlenMod iBM, Bool bBitInc> template<Bool bSet> __inline Void
TRenSingleModelC<iBM,bBitInc>::xSetShiftedPelR(Int iSourcePos, Int iSubSourcePos, Int iTargetSPos, Pel iFilled, RMDist& riError )
{
  RM_AOT( iSourcePos    <  0                     );
  RM_AOT( iSourcePos    >= m_iWidth              );
  RM_AOT( iSubSourcePos <  0                     );
  RM_AOT( iSubSourcePos >= (1 << m_iShiftPrec)+1 );
  RM_AOT( iTargetSPos   < 0                      );
  RM_AOT( iTargetSPos   >= m_iWidth              );  

  RenModelOutPels* pcOutSample = m_pcOutputSamplesRow               + iTargetSPos;
  RenModelInPels * pcInSample  = m_pcInputSamplesRow[VIEWPOS_RIGHT] + iSourcePos ;

  if ( iBM != BLEND_NONE )
  {
    xSetShiftedPelBlendR<bSet>   (pcInSample, iSubSourcePos, pcOutSample, iFilled, riError);
  }
  else
  {
    xSetShiftedPelNoBlendR<bSet> (pcInSample, iSubSourcePos, pcOutSample, iFilled, riError);
  }
}

template <BlenMod iBM, Bool bBitInc> template<Bool bSet> __inline Void
TRenSingleModelC<iBM,bBitInc>::xSetShiftedPelNoBlendR(RenModelInPels* pcInSample, Int iSubSourcePos, RenModelOutPels* pcOutSample, Pel iFilled, RMDist& riError )
{
  if ( bSet )
  { 
    // Filled
    pcOutSample->iFilledRight = iFilled;

    // Yuv
    pcOutSample->iYRight  = pcInSample->aiY[iSubSourcePos];
#if HHI_VSO_COLOR_PLANES
    pcOutSample->iURight  = pcInSample->aiU[iSubSourcePos];
    pcOutSample->iVRight  = pcInSample->aiV[iSubSourcePos];

    pcOutSample->iError = xGetDist( 
      pcOutSample->iYRight - pcOutSample->iYRef,
      pcOutSample->iURight - pcOutSample->iURef,    
      pcOutSample->iVRight - pcOutSample->iVRef
      );    
#else
    pcOutSample->iError = xGetDist( pcOutSample->iYRight - pcOutSample->iYRef );    
#endif    

  }
  else
  { 
#if HHI_VSO_COLOR_PLANES
    riError += xGetDist( pcInSample->aiY[iSubSourcePos] - pcOutSample->iYRef,
      pcInSample->aiU[iSubSourcePos] - pcOutSample->iURef,
      pcInSample->aiV[iSubSourcePos] - pcOutSample->iVRef
      );
#else               
    riError += xGetDist( pcInSample->aiY[iSubSourcePos] - pcOutSample->iYRef );
#endif

    riError -= pcOutSample->iError;
  }
}

template <BlenMod iBM, Bool bBitInc> template<Bool bSet> __inline Void
TRenSingleModelC<iBM,bBitInc>::xSetShiftedPelBlendR(RenModelInPels* pcInSample, Int iSubSourcePos, RenModelOutPels* pcOutSample, Pel iFilled, RMDist& riError )
{
  Pel piBlendedValueY;
#if HHI_VSO_COLOR_PLANES
  Pel piBlendedValueU;
  Pel piBlendedValueV;
#endif

  xGetBlendedValue (
    pcOutSample->iYLeft, 
    pcInSample ->aiY[iSubSourcePos],        
#if HHI_VSO_COLOR_PLANES
    pcOutSample->iULeft,    
    pcInSample ->aiU[iSubSourcePos],
    pcOutSample->iVLeft,    
    pcInSample ->aiV[iSubSourcePos],
#endif
    m_piInvZLUTLeft  [RenModRemoveBitInc(pcOutSample->iDLeft)],
    m_piInvZLUTRight [RenModRemoveBitInc(m_iThisDepth)       ],
    pcOutSample->iFilledLeft,
    iFilled,
    piBlendedValueY
#if HHI_VSO_COLOR_PLANES
    , piBlendedValueU,
    piBlendedValueV
#endif
    );

  if ( bSet )
  {    
    // Set values
    pcOutSample->iDRight     = m_iThisDepth;
    pcOutSample->iYRight     = pcInSample ->aiY[iSubSourcePos];
    pcOutSample->iYBlended   = piBlendedValueY;    
#if HHI_VSO_COLOR_PLANES  
    pcOutSample->iURight     = pcInSample ->aiU[iSubSourcePos];
    pcOutSample->iUBlended   = piBlendedValueU;    
    pcOutSample->iVRight     = pcInSample ->aiV[iSubSourcePos];
    pcOutSample->iVBlended   = piBlendedValueV;    
#endif
    pcOutSample->iFilledRight = iFilled;

    // Get Error
    Int iDiffY = pcOutSample->iYRef - piBlendedValueY;
#if HHI_VSO_COLOR_PLANES
    Int iDiffU = pcOutSample->iURef - piBlendedValueU;
    Int iDiffV = pcOutSample->iVRef - piBlendedValueV;
    pcOutSample->iError  = xGetDist(iDiffY, iDiffU, iDiffV );
#else
    pcOutSample->iError  = xGetDist(iDiffY );
#endif
  }
  else
  {
    Int iDiffY = pcOutSample->iYRef - piBlendedValueY;
#if HHI_VSO_COLOR_PLANES
    Int iDiffU = pcOutSample->iURef - piBlendedValueU;
    Int iDiffV = pcOutSample->iVRef - piBlendedValueV;
    riError   += ( xGetDist( iDiffY, iDiffU, iDiffV ) - pcOutSample->iError );
#else
    riError   += ( xGetDist( iDiffY ) -  pcOutSample->iError  );
#endif
  }
}

template <BlenMod iBM, Bool bBitInc> __inline Int
TRenSingleModelC<iBM,bBitInc>::xGetDist( Int iDiffY, Int iDiffU, Int iDiffV )
{

  if ( !bBitInc )
  {
    return (          (iDiffY * iDiffY )
               +  ((( (iDiffU * iDiffU )
                     +(iDiffV * iDiffV )
                    )
                   ) >> 2
                  )
           );
  }
  else
  {
    return (          ((iDiffY * iDiffY) >> m_iDistShift)
               +  ((( ((iDiffU * iDiffU) >> m_iDistShift)
                     +((iDiffV * iDiffV) >> m_iDistShift)
                    )
                   ) >> 2
                  )
           );
  
  }
}

template <BlenMod iBM, Bool bBitInc> __inline Int
TRenSingleModelC<iBM,bBitInc>::xGetDist( Int iDiffY )
{
  if ( !bBitInc )
  {
    return (iDiffY * iDiffY);
  }
  else
  {
    return ((iDiffY * iDiffY) >> m_iDistShift);
  }

}


#if HHI_VSO_COLOR_PLANES
template <BlenMod iBM, Bool bBitInc>  __inline Void
TRenSingleModelC<iBM,bBitInc>::xGetBlendedValue( Pel iYL, Pel iYR, Pel iUL, Pel iUR, Pel iVL, Pel iVR, Pel iDepthL, Pel iDepthR, Int iFilledL, Int iFilledR, Pel& riY, Pel& riU, Pel&riV )
#else
template <BlenMod iBM, Bool bBitInc>  __inline Void
TRenSingleModelC<iBM,bBitInc>::xGetBlendedValue( Pel iYL, Pel iYR, Pel iDepthL, Pel iDepthR, Int iFilledL, Int iFilledR, Pel& riY )
#endif
{

  RM_AOT( iBM != BLEND_AVRG && iBM != BLEND_LEFT && iBM != BLEND_RIGHT );

  if (iBM != BLEND_AVRG )
  {
    if (iBM == BLEND_LEFT )
    {
#if HHI_VSO_COLOR_PLANES
      xGetBlendedValueBM1(  iYL,  iYR,  iUL,  iUR,  iVL,  iVR,  iDepthL,  iDepthR,  iFilledL,  iFilledR,  riY,  riU, riV );
#else
      xGetBlendedValueBM1(  iYL,  iYR,  iDepthL,  iDepthR,  iFilledL,  iFilledR,  riY );
#endif
    }
    else
    {
#if HHI_VSO_COLOR_PLANES
      xGetBlendedValueBM2(  iYL,  iYR,  iUL,  iUR,  iVL,  iVR,  iDepthL,  iDepthR,  iFilledL,  iFilledR,  riY,  riU, riV );
#else
      xGetBlendedValueBM2(  iYL,  iYR, iDepthL,  iDepthR,  iFilledL,  iFilledR,  riY );
#endif
    }
    return;
  }

  if (  (iFilledL != REN_IS_HOLE ) && ( iFilledR != REN_IS_HOLE) )
  {
    Int iDepthDifference = iDepthR - iDepthL;

    if ( abs ( iDepthDifference ) <= m_iBlendZThres )
    {
      if      ((iFilledL == REN_IS_FILLED) && ( iFilledR != REN_IS_FILLED))
      {
        riY = xBlend( iYL, iYR, iFilledR >> 1 );
#if HHI_VSO_COLOR_PLANES
        riU = xBlend( iUL, iUR, iFilledR >> 1 );
        riV = xBlend( iVL, iVR, iFilledR >> 1 );
#endif

      }
      else if ((iFilledL != REN_IS_FILLED) && ( iFilledR == REN_IS_FILLED))
      {
        riY = xBlend( iYR, iYL, (iFilledL >> 1) );
#if HHI_VSO_COLOR_PLANES
        riU = xBlend( iUR, iUL, (iFilledL >> 1) );
        riV = xBlend( iVR, iVL, (iFilledL >> 1) );
#endif
      }
      else
      {
        riY = xBlend( iYL, iYR, m_iBlendDistWeight );
#if HHI_VSO_COLOR_PLANES    
        riU = xBlend( iUL, iUR, m_iBlendDistWeight );
        riV = xBlend( iVL, iVR, m_iBlendDistWeight );
#endif
      }
    }
    else if ( iDepthDifference < 0 )
    {
      riY = iYL;
#if HHI_VSO_COLOR_PLANES
      riU = iUL;
      riV = iVL;
#endif
    }
    else
    {
      riY = iYR;
#if HHI_VSO_COLOR_PLANES
      riU = iUR;
      riV = iVR;
#endif
    }
  }
  else if ( (iFilledL == REN_IS_HOLE) && (iFilledR == REN_IS_HOLE))
  {
    if ( iDepthR < iDepthL )
    {
        riY =  iYR;
#if HHI_VSO_COLOR_PLANES
        riU =  iUR;
        riV =  iVR;
#endif
    }
    else
    {
        riY =  iYL;
#if HHI_VSO_COLOR_PLANES
        riU =  iUL;
        riV =  iVL;
#endif
    }
  }
  else
  {
    if (iFilledR == REN_IS_HOLE)
    {
        riY = iYL;
#if HHI_VSO_COLOR_PLANES
        riU = iUL;
        riV = iVL;
#endif
    }
    else
    {
      riY = iYR;
#if HHI_VSO_COLOR_PLANES
      riU = iUR;
      riV = iVR;
#endif
    }
  }
}

template <BlenMod iBM, Bool bBitInc> __inline Void
#if HHI_VSO_COLOR_PLANES
TRenSingleModelC<iBM,bBitInc>::xGetBlendedValueBM1( Pel iYL, Pel iYR, Pel iUL, Pel iUR, Pel iVL, Pel iVR, Pel iDepthL, Pel iDepthR, Int iFilledL, Int iFilledR, Pel& riY, Pel& riU, Pel&riV )
#else
TRenSingleModelC<iBM,bBitInc>::xGetBlendedValueBM1( Pel iYL, Pel iYR, Pel iDepthL, Pel iDepthR, Int iFilledL, Int iFilledR, Pel& riY )
#endif
{
  if ( iFilledL == REN_IS_FILLED ||  iFilledR == REN_IS_HOLE )
  {
    riY = iYL;
#if HHI_VSO_COLOR_PLANES
    riU = iUL;
    riV = iVL;
#endif
  }
  else if ( iFilledL == REN_IS_HOLE  )
  {
    riY = iYR;
#if HHI_VSO_COLOR_PLANES
    riU = iUR;
    riV = iVR;
#endif
  }
  else
  {
    riY = xBlend( iYR, iYL, iFilledL );
#if HHI_VSO_COLOR_PLANES
    riU = xBlend( iUR, iUL, iFilledL );
    riV = xBlend( iVR, iUL, iFilledL );
#endif
  }
}

template <BlenMod iBM, Bool bBitInc> __inline Void
#if HHI_VSO_COLOR_PLANES
TRenSingleModelC<iBM,bBitInc>::xGetBlendedValueBM2( Pel iYL, Pel iYR, Pel iUL, Pel iUR, Pel iVL, Pel iVR, Pel iDepthL, Pel iDepthR, Int iFilledL, Int iFilledR, Pel& riY, Pel& riU, Pel&riV )
#else
TRenSingleModelC<iBM,bBitInc>::xGetBlendedValueBM2( Pel iYL, Pel iYR, Pel iDepthL, Pel iDepthR, Int iFilledL, Int iFilledR, Pel& riY )
#endif
{
  if      ( iFilledR == REN_IS_FILLED ||  iFilledL == REN_IS_HOLE )
  {
    riY = iYR;
#if HHI_VSO_COLOR_PLANES
    riU = iUR;
    riV = iVR;
#endif
  }
  else if ( iFilledR == REN_IS_HOLE  )
  {
    riY = iYL;
#if HHI_VSO_COLOR_PLANES
    riU = iUL;
    riV = iVL;
#endif
  }
  else
  {
    riY = xBlend( iYL, iYR, iFilledR );
#if HHI_VSO_COLOR_PLANES
    riU = xBlend( iUL, iUR, iFilledR );
    riV = xBlend( iVL, iUR, iFilledR );
#endif
  }
}

template <BlenMod iBM, Bool bBitInc> __inline Pel
TRenSingleModelC<iBM,bBitInc>::xBlend( Pel pVal1, Pel pVal2, Int iWeightVal2 )
{
  return pVal1  +  (Pel) (  ( (Int) ( pVal2 - pVal1) * iWeightVal2 + (1 << (REN_VDWEIGHT_PREC - 1)) ) >> REN_VDWEIGHT_PREC );
}

#if FIX_VSO_SETUP
template <BlenMod iBM, Bool bBitInc> Void
TRenSingleModelC<iBM,bBitInc>::xCopy2PicYuv( Pel** ppiSrcVideoPel, Int* piStrides, TComPicYuv* rpcPicYuvTarget )
{
  TRenFilter::copy            ( ppiSrcVideoPel[0], piStrides[0], m_iWidth, m_iUsedHeight, rpcPicYuvTarget->getLumaAddr() +  m_iHorOffset       * rpcPicYuvTarget->getStride() , rpcPicYuvTarget->getStride () );
  TRenFilter::sampleDown2Tap13( ppiSrcVideoPel[1], piStrides[1], m_iWidth, m_iUsedHeight, rpcPicYuvTarget->getCbAddr  () + (m_iHorOffset >> 1) * rpcPicYuvTarget->getCStride(), rpcPicYuvTarget->getCStride() );
  TRenFilter::sampleDown2Tap13( ppiSrcVideoPel[2], piStrides[2], m_iWidth, m_iUsedHeight, rpcPicYuvTarget->getCrAddr  () + (m_iHorOffset >> 1) * rpcPicYuvTarget->getCStride(), rpcPicYuvTarget->getCStride() );
}
#else
template <BlenMod iBM, Bool bBitInc> Void
TRenSingleModelC<iBM,bBitInc>::xCopy2PicYuv( Pel** ppiSrcVideoPel, Int* piStrides, TComPicYuv* rpcPicYuvTarget, UInt uiHorOffset )
{
  TRenFilter::copy            ( ppiSrcVideoPel[0], piStrides[0], m_iWidth, m_iHeight, rpcPicYuvTarget->getLumaAddr() +  uiHorOffset       * rpcPicYuvTarget->getStride() , rpcPicYuvTarget->getStride () );
  TRenFilter::sampleDown2Tap13( ppiSrcVideoPel[1], piStrides[1], m_iWidth, m_iHeight, rpcPicYuvTarget->getCbAddr  () + (uiHorOffset >> 1) * rpcPicYuvTarget->getCStride(), rpcPicYuvTarget->getCStride() );
  TRenFilter::sampleDown2Tap13( ppiSrcVideoPel[2], piStrides[2], m_iWidth, m_iHeight, rpcPicYuvTarget->getCrAddr  () + (uiHorOffset >> 1) * rpcPicYuvTarget->getCStride(), rpcPicYuvTarget->getCStride() );
}
#endif

template class TRenSingleModelC<BLEND_NONE ,true>;
template class TRenSingleModelC<BLEND_AVRG ,true>;
template class TRenSingleModelC<BLEND_LEFT ,true>;
template class TRenSingleModelC<BLEND_RIGHT,true>;

template class TRenSingleModelC<BLEND_NONE ,false>;
template class TRenSingleModelC<BLEND_AVRG ,false>;
template class TRenSingleModelC<BLEND_LEFT ,false>;
template class TRenSingleModelC<BLEND_RIGHT,false>;

#ifdef LGE_VSO_EARLY_SKIP_A0093
template <BlenMod iBM, Bool bBitInc> 
__inline Bool
TRenSingleModelC<iBM,bBitInc>::xDetectEarlySkipL( Int iStartPosX, Int iStartPosY, Int iWidth, Int iHeight, Int iStride, Pel* piNewData,Pel* piOrgData, Int iOrgStride)
{
  RM_AOF( m_bEarlySkip ); 
  const Int iCurViewPos = 0;
  Int** ppiCurLUT       = m_appiShiftLut   [ iCurViewPos ];
  
  Bool bNoDiff          = true;   
  
  for (Int iPosY=0; iPosY < iHeight; iPosY++)
  {
    m_pbHorSkip[iPosY] = true;

    for (Int iPosX = 0; iPosX < iWidth; iPosX++)
    {
      Int iDisparityRec = abs(ppiCurLUT[0][ RenModRemoveBitInc(piNewData[iPosX])]);
      Int iDispartyOrg  = abs(ppiCurLUT[0][ RenModRemoveBitInc(piOrgData[iPosX])]);

      if( iDispartyOrg != iDisparityRec)
      {
        m_pbHorSkip[iPosY] = false;
        bNoDiff            = false;
        break;
      }
    }
    piNewData += iStride;
    piOrgData += iOrgStride;
  }
  return bNoDiff;
}

template <BlenMod iBM, Bool bBitInc> 
__inline Bool
TRenSingleModelC<iBM,bBitInc>::xDetectEarlySkipR( Int iStartPosX, Int iStartPosY, Int iWidth, Int iHeight, Int iStride, Pel* piNewData,Pel* piOrgData, Int iOrgStride)
{
  RM_AOF( m_bEarlySkip ); 
  Bool bNoDiff  = true;

  const Int iCurViewPos = 1;
  Int** ppiCurLUT       = m_appiShiftLut   [ iCurViewPos ];

  for ( Int iPosY = 0; iPosY < iHeight; iPosY++ )
  {
    m_pbHorSkip[iPosY] = true;

    for (Int iPosX = 0; iPosX < iWidth; iPosX++)
    {
      Int iDisparityRec = abs( ppiCurLUT[0][ RenModRemoveBitInc(piNewData[iPosX])] );
      Int iDisparityOrg = abs( ppiCurLUT[0][ RenModRemoveBitInc(piOrgData[iPosX])] );

      if( iDisparityRec != iDisparityOrg )
      {
        m_pbHorSkip[iPosY] = false;
        bNoDiff            = false;
        break;
      }
    }

    piNewData += iStride;
    piOrgData += iOrgStride;
  }
  return bNoDiff;
}
#endif
