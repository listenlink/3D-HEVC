

/** \file     TComPrediction.cpp
    \brief    prediction class
*/

#include <memory.h>
#include "TComPrediction.h"

// ====================================================================================================================
// Constructor / destructor / initialize
// ====================================================================================================================

TComPrediction::TComPrediction()
: m_pLumaRecBuffer(0)
{
  m_piYuvExt = NULL;
}

TComPrediction::~TComPrediction()
{
  m_cYuvExt.destroy();

  delete[] m_piYuvExt;

  m_acYuvPred[0].destroy();
  m_acYuvPred[1].destroy();

  m_cYuvPredTemp.destroy();

#if LM_CHROMA  
  if( m_pLumaRecBuffer )
    delete [] m_pLumaRecBuffer;  
#endif
}

Void TComPrediction::initTempBuff()
{
  if( m_piYuvExt == NULL )
  {
    m_iYuvExtHeight  = ((g_uiMaxCUHeight + 2) << 4);
    m_iYuvExtStride = ((g_uiMaxCUWidth  + 8) << 4);
    m_cYuvExt.create( m_iYuvExtStride, m_iYuvExtHeight );
    m_piYuvExt = new Int[ m_iYuvExtStride * m_iYuvExtHeight ];

    // new structure
    m_acYuvPred[0] .create( g_uiMaxCUWidth, g_uiMaxCUHeight );
    m_acYuvPred[1] .create( g_uiMaxCUWidth, g_uiMaxCUHeight );

    m_cYuvPredTemp.create( g_uiMaxCUWidth, g_uiMaxCUHeight );
  }

#if LM_CHROMA                      
  m_iLumaRecStride =  (g_uiMaxCUWidth>>1) + 1;
  m_pLumaRecBuffer = new Pel[ m_iLumaRecStride * m_iLumaRecStride ];

  for( Int i = 1; i < 66; i++ )
    m_uiaShift[i-1] = ( (1 << 15) + i/2 ) / i;
#endif
}

// ====================================================================================================================
// Public member functions
// ====================================================================================================================

// Function for calculating DC value of the reference samples used in Intra prediction
Pel TComPrediction::predIntraGetPredValDC( Int* pSrc, Int iSrcStride, UInt iWidth, UInt iHeight, Bool bAbove, Bool bLeft )
{
  Int iInd, iSum = 0;
  Pel pDcVal;

  if (bAbove)
  {
    for (iInd = 0;iInd < iWidth;iInd++)
      iSum += pSrc[iInd-iSrcStride];
  }
  if (bLeft)
  {
    for (iInd = 0;iInd < iHeight;iInd++)
      iSum += pSrc[iInd*iSrcStride-1];
  }

  if (bAbove && bLeft)
    pDcVal = (iSum + iWidth) / (iWidth + iHeight);
  else if (bAbove)
    pDcVal = (iSum + iWidth/2) / iWidth;
  else if (bLeft)
    pDcVal = (iSum + iHeight/2) / iHeight;
  else
    pDcVal = pSrc[-1]; // Default DC value already calculated and placed in the prediction array if no neighbors are available

  return pDcVal;
}

// Function for deriving the angular Intra predictions

/** Function for deriving the simplified angular intra predictions.
 * \param pSrc pointer to reconstructed sample array
 * \param srcStride the stride of the reconstructed sample array
 * \param rpDst reference to pointer for the prediction sample array
 * \param dstStride the stride of the prediction sample array
 * \param width the width of the block
 * \param height the height of the block
 * \param dirMode the intra prediction mode index
 * \param blkAboveAvailable boolean indication if the block above is available
 * \param blkLeftAvailable boolean indication if the block to the left is available
 *
 * This function derives the prediction samples for the angular mode based on the prediction direction indicated by
 * the prediction mode index. The prediction direction is given by the displacement of the bottom row of the block and
 * the reference row above the block in the case of vertical prediction or displacement of the rightmost column
 * of the block and reference column left from the block in the case of the horizontal prediction. The displacement
 * is signalled at 1/32 pixel accuracy. When projection of the predicted pixel falls inbetween reference samples,
 * the predicted value for the pixel is linearly interpolated from the reference samples. All reference samples are taken
 * from the extended main reference.
 */
Void TComPrediction::xPredIntraAng( Int* pSrc, Int srcStride, Pel*& rpDst, Int dstStride, UInt width, UInt height, UInt dirMode, Bool blkAboveAvailable, Bool blkLeftAvailable )
{
  Int k,l;
  Int blkSize        = width;
  Pel* pDst          = rpDst;

  // Map the mode index to main prediction direction and angle
  Bool modeDC        = dirMode == 0;
  Bool modeVer       = !modeDC && (dirMode < 18);
  Bool modeHor       = !modeDC && !modeVer;
  Int intraPredAngle = modeVer ? dirMode - 9 : modeHor ? dirMode - 25 : 0;
  Int absAng         = abs(intraPredAngle);
  Int signAng        = intraPredAngle < 0 ? -1 : 1;

  // Set bitshifts and scale the angle parameter to block size
  Int angTable[9]    = {0,    2,    5,   9,  13,  17,  21,  26,  32};
  Int invAngTable[9] = {0, 4096, 1638, 910, 630, 482, 390, 315, 256}; // (256 * 32) / Angle
  Int invAngle       = invAngTable[absAng];
  absAng             = angTable[absAng];
  intraPredAngle     = signAng * absAng;

  // Do the DC prediction
  if (modeDC)
  {
    Pel dcval = predIntraGetPredValDC(pSrc, srcStride, width, height, blkAboveAvailable, blkLeftAvailable);

    for (k=0;k<blkSize;k++)
    {
      for (l=0;l<blkSize;l++)
      {
        pDst[k*dstStride+l] = dcval;
      }
    }
  }

  // Do angular predictions
  else
  {
    Pel* refMain;
    Pel* refSide;
    Pel  refAbove[2*MAX_CU_SIZE+1];
    Pel  refLeft[2*MAX_CU_SIZE+1];

    // Initialise the Main and Left reference array.
    if (intraPredAngle < 0)
    {
      for (k=0;k<blkSize+1;k++)
      {
        refAbove[k+blkSize-1] = pSrc[k-srcStride-1];
      }
      for (k=0;k<blkSize+1;k++)
      {
        refLeft[k+blkSize-1] = pSrc[(k-1)*srcStride-1];
      }
      refMain = (modeVer ? refAbove : refLeft) + (blkSize-1);
      refSide = (modeVer ? refLeft : refAbove) + (blkSize-1);

      // Extend the Main reference to the left.
      Int invAngleSum    = 128;       // rounding for (shift by 8)
      for (k=-1; k>blkSize*intraPredAngle>>5; k--)
      {
        invAngleSum += invAngle;
        refMain[k] = refSide[invAngleSum>>8];
      }
    }
    else
    {
      for (k=0;k<2*blkSize+1;k++)
      {
        refAbove[k] = pSrc[k-srcStride-1];
      }
      for (k=0;k<2*blkSize+1;k++)
      {
        refLeft[k] = pSrc[(k-1)*srcStride-1];
      }
      refMain = modeVer ? refAbove : refLeft;
    }

    if (intraPredAngle == 0)
    {
      for (k=0;k<blkSize;k++)
      {
        for (l=0;l<blkSize;l++)
        {
          pDst[k*dstStride+l] = refMain[l+1];
        }
      }
    }
    else
    {
      Int deltaPos=0;
      Int deltaInt;
      Int deltaFract;
      Int refMainIndex;

      for (k=0;k<blkSize;k++)
      {
        deltaPos += intraPredAngle;
        deltaInt   = deltaPos >> 5;
        deltaFract = deltaPos & (32 - 1);

        if (deltaFract)
        {
          // Do linear filtering
          for (l=0;l<blkSize;l++)
          {
            refMainIndex        = l+deltaInt+1;
            pDst[k*dstStride+l] = (Pel) ( ((32-deltaFract)*refMain[refMainIndex]+deltaFract*refMain[refMainIndex+1]+16) >> 5 );
          }
        }
        else
        {
          // Just copy the integer samples
          for (l=0;l<blkSize;l++)
          {
            pDst[k*dstStride+l] = refMain[l+deltaInt+1];
          }
        }
      }
    }

    // Flip the block if this is the horizontal mode
    if (modeHor)
    {
      Pel  tmp;
      for (k=0;k<blkSize-1;k++)
      {
        for (l=k+1;l<blkSize;l++)
        {
          tmp                 = pDst[k*dstStride+l];
          pDst[k*dstStride+l] = pDst[l*dstStride+k];
          pDst[l*dstStride+k] = tmp;
        }
      }
    }
  }
}

Void TComPrediction::predIntraLumaAng(TComPattern* pcTComPattern, UInt uiDirMode, Pel* piPred, UInt uiStride, Int iWidth, Int iHeight,  TComDataCU* pcCU, Bool bAbove, Bool bLeft )
{
  Pel *pDst = piPred;
  Int *ptrSrc;

  // only assign variable in debug mode
#ifndef NDEBUG
  // get intra direction
  Int iIntraSizeIdx = g_aucConvertToBit[ iWidth ] + 1;

  assert( iIntraSizeIdx >= 1 ); //   4x  4
  assert( iIntraSizeIdx <= 6 ); // 128x128
  assert( iWidth == iHeight  );
#endif //NDEBUG

#if QC_MDIS
#if HHI_DISABLE_INTRA_SMOOTHING_DEPTH
  ptrSrc = pcTComPattern->getPredictorPtr( uiDirMode, g_aucConvertToBit[ iWidth ] + 1, iWidth, iHeight, m_piYuvExt, pcCU->getSlice()->getSPS()->isDepth() );
#else
  ptrSrc = pcTComPattern->getPredictorPtr( uiDirMode, g_aucConvertToBit[ iWidth ] + 1, iWidth, iHeight, m_piYuvExt );
#endif
#else
  ptrSrc = pcTComPattern->getAdiOrgBuf( iWidth, iHeight, m_piYuvExt );
#endif //QC_MDIS

  // get starting pixel in block
  Int sw = ( iWidth<<1 ) + 1;

#if ADD_PLANAR_MODE
  if ( uiDirMode == PLANAR_IDX )
  {
#if REFERENCE_SAMPLE_PADDING
    xPredIntraPlanar( ptrSrc+sw+1, sw, pDst, uiStride, iWidth, iHeight );
#else
    xPredIntraPlanar( ptrSrc+sw+1, sw, pDst, uiStride, iWidth, iHeight, bAbove, bLeft );
#endif
    return;
  }
#endif

  // get converted direction
  uiDirMode = g_aucAngIntraModeOrder[ uiDirMode ];

  // Create the prediction
  xPredIntraAng( ptrSrc+sw+1, sw, pDst, uiStride, iWidth, iHeight, uiDirMode, bAbove,  bLeft );

#if MN_DC_PRED_FILTER
  if ((uiDirMode == 0) && pcTComPattern->getDCPredFilterFlag())
    xDCPredFiltering( ptrSrc+sw+1, sw, pDst, uiStride, iWidth, iHeight);
#endif
}


Void 
TComPrediction::predIntraDepthAng(TComPattern* pcTComPattern, UInt uiDirMode, Pel* piPred, UInt uiStride, Int iWidth, Int iHeight )
{
  Pel*  pDst    = piPred;
  Int*  ptrSrc  = pcTComPattern->getAdiOrgBuf( iWidth, iHeight, m_piYuvExt );
  Int   sw      = ( iWidth<<1 ) + 1;
  uiDirMode     = g_aucAngIntraModeOrder[ uiDirMode ];
  xPredIntraAngDepth( ptrSrc+sw+1, sw, pDst, uiStride, iWidth, iHeight, uiDirMode );
}

Int
TComPrediction::xGetDCDepth( Int* pSrc, Int iDelta, Int iBlkSize )
{
  Int iDC    = PDM_UNDEFINED_DEPTH;
  Int iSum   = 0;
  Int iNum   = 0;
  for( Int k = 0; k < iBlkSize; k++, pSrc += iDelta )
  {
    if( *pSrc != PDM_UNDEFINED_DEPTH )
    {
      iSum += *pSrc;
      iNum ++;
    }
  }
  if( iNum )
  {
    iDC = ( iSum + ( iNum >> 1 ) ) / iNum;
  }
  return iDC;
}

Int
TComPrediction::xGetDCValDepth( Int iVal1, Int iVal2, Int iVal3, Int iVal4 )
{
  if     ( iVal1 != PDM_UNDEFINED_DEPTH )   return iVal1;
  else if( iVal2 != PDM_UNDEFINED_DEPTH )   return iVal2;
  else if( iVal3 != PDM_UNDEFINED_DEPTH )   return iVal3;
  return   iVal4;
}

Void 
TComPrediction::xPredIntraAngDepth( Int* pSrc, Int srcStride, Pel* pDst, Int dstStride, UInt width, UInt height, UInt dirMode )
{
  AOF( width == height );
  Int blkSize       = width;
  Int iDCAbove      = xGetDCDepth( pSrc - srcStride,                               1, blkSize );
  Int iDCAboveRight = xGetDCDepth( pSrc - srcStride + blkSize,                     1, blkSize );
  Int iDCLeft       = xGetDCDepth( pSrc -         1,                       srcStride, blkSize );
  Int iDCBelowLeft  = xGetDCDepth( pSrc -         1 + blkSize * srcStride, srcStride, blkSize );
  Int iWgt, iDC1, iDC2;
  if( dirMode == 0 ) // 0
  {
    iDC1  = xGetDCValDepth( iDCAbove, iDCAboveRight, iDCLeft,  iDCBelowLeft  );
    iDC2  = xGetDCValDepth( iDCLeft,  iDCBelowLeft,  iDCAbove, iDCAboveRight );
    iWgt  = 8;
  }
  else if( dirMode < 10 ) // 1..9
  {
    iDC1  = xGetDCValDepth( iDCAbove, iDCAboveRight, iDCLeft,  iDCBelowLeft  );
    iDC2  = xGetDCValDepth( iDCLeft,  iDCBelowLeft,  iDCAbove, iDCAboveRight );
    iWgt  = 7 + dirMode;
  }
  else if( dirMode < 18 ) // 10..17
  {
    iDC1  = xGetDCValDepth( iDCAbove, iDCAboveRight, iDCLeft,  iDCBelowLeft  );
    iDC2  = xGetDCValDepth( iDCAboveRight, iDCAbove, iDCLeft,  iDCBelowLeft  );
    iWgt  = 25 - dirMode;
  }
  else if( dirMode < 26 ) // 18..25
  {
    iDC1  = xGetDCValDepth( iDCAbove, iDCAboveRight, iDCLeft,  iDCBelowLeft  );
    iDC2  = xGetDCValDepth( iDCLeft,  iDCBelowLeft,  iDCAbove, iDCAboveRight );
    iWgt  = 25 - dirMode;
  }
  else if( dirMode < 34 )  // 26..33
  {
    iDC1  = xGetDCValDepth( iDCLeft,  iDCBelowLeft,  iDCAbove, iDCAboveRight );
    iDC2  = xGetDCValDepth( iDCBelowLeft,  iDCLeft,  iDCAbove, iDCAboveRight );
    iWgt  = 41 - dirMode;
  }
  else // 34 (wedgelet -> use simple DC prediction
  {
    iDC1  = xGetDCValDepth( iDCAbove, iDCAboveRight, iDCLeft,  iDCBelowLeft  );
    iDC2  = xGetDCValDepth( iDCLeft,  iDCBelowLeft,  iDCAbove, iDCAboveRight );
    iWgt  = 8;
  }
  Int iWgt2   = 16 - iWgt;
  Int iDCVal  = ( iWgt * iDC1 + iWgt2 * iDC2 + 8 ) >> 4;
  
  // set depth
  for( Int iY = 0; iY < blkSize; iY++, pDst += dstStride )
  {
    for( Int iX = 0; iX < blkSize; iX++ )
    {
      pDst[ iX ] = iDCVal;
    }
  }
}


// Angular chroma
Void TComPrediction::predIntraChromaAng( TComPattern* pcTComPattern, Int* piSrc, UInt uiDirMode, Pel* piPred, UInt uiStride, Int iWidth, Int iHeight, TComDataCU* pcCU, Bool bAbove, Bool bLeft )
{
  Pel *pDst = piPred;
  Int *ptrSrc = piSrc;

  // get starting pixel in block
  Int sw = ( iWidth<<1 ) + 1;

#if ADD_PLANAR_MODE
  if ( uiDirMode == PLANAR_IDX )
  {
#if REFERENCE_SAMPLE_PADDING
    xPredIntraPlanar( ptrSrc+sw+1, sw, pDst, uiStride, iWidth, iHeight );
#else
    xPredIntraPlanar( ptrSrc+sw+1, sw, pDst, uiStride, iWidth, iHeight, bAbove, bLeft );
#endif
    return;
  }
#endif

  // get converted direction
  uiDirMode = g_aucAngIntraModeOrder[ uiDirMode ];

  // Create the prediction
  xPredIntraAng( ptrSrc+sw+1, sw, pDst, uiStride, iWidth, iHeight, uiDirMode, bAbove,  bLeft );
}

#if HHI_DMM_INTRA
Void TComPrediction::predIntraLumaDMM( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiMode, Pel* piPred, UInt uiStride, Int iWidth, Int iHeight, Bool bAbove, Bool bLeft, Bool bEncoder )
{
  if( uiMode == DMM_WEDGE_FULL_IDX          ) { xPredIntraWedgeFull      ( pcCU, uiAbsPartIdx, piPred, uiStride, iWidth, iHeight, bAbove, bLeft, bEncoder, false, pcCU->getWedgeFullTabIdx ( uiAbsPartIdx ) ); }
  if( uiMode == DMM_WEDGE_FULL_D_IDX        ) { xPredIntraWedgeFull      ( pcCU, uiAbsPartIdx, piPred, uiStride, iWidth, iHeight, bAbove, bLeft, bEncoder, true,  pcCU->getWedgeFullTabIdx( uiAbsPartIdx ), pcCU->getWedgeFullDeltaDC1( uiAbsPartIdx ), pcCU->getWedgeFullDeltaDC2( uiAbsPartIdx ) ); }
  if( uiMode == DMM_WEDGE_PREDDIR_IDX     ) { xPredIntraWedgeDir       ( pcCU, uiAbsPartIdx, piPred, uiStride, iWidth, iHeight, bAbove, bLeft, bEncoder, false, pcCU->getWedgePredDirDeltaEnd( uiAbsPartIdx ) ); }
  if( uiMode == DMM_WEDGE_PREDDIR_D_IDX   ) { xPredIntraWedgeDir       ( pcCU, uiAbsPartIdx, piPred, uiStride, iWidth, iHeight, bAbove, bLeft, bEncoder, true,  pcCU->getWedgePredDirDeltaEnd( uiAbsPartIdx ), pcCU->getWedgePredDirDeltaDC1( uiAbsPartIdx ), pcCU->getWedgePredDirDeltaDC2( uiAbsPartIdx ) ); }
  if( uiMode == DMM_WEDGE_PREDTEX_IDX       ) { xPredIntraWedgeTex       ( pcCU, uiAbsPartIdx, piPred, uiStride, iWidth, iHeight, bAbove, bLeft, bEncoder, false ); }
  if( uiMode == DMM_WEDGE_PREDTEX_D_IDX     ) { xPredIntraWedgeTex       ( pcCU, uiAbsPartIdx, piPred, uiStride, iWidth, iHeight, bAbove, bLeft, bEncoder, true, pcCU->getWedgePredTexDeltaDC1( uiAbsPartIdx ), pcCU->getWedgePredTexDeltaDC2( uiAbsPartIdx ) ); }
  if( uiMode == DMM_CONTOUR_PREDTEX_IDX     ) { xPredIntraContourTex     ( pcCU, uiAbsPartIdx, piPred, uiStride, iWidth, iHeight, bAbove, bLeft, bEncoder, false ); }
  if( uiMode == DMM_CONTOUR_PREDTEX_D_IDX   ) { xPredIntraContourTex     ( pcCU, uiAbsPartIdx, piPred, uiStride, iWidth, iHeight, bAbove, bLeft, bEncoder, true, pcCU->getContourPredTexDeltaDC1( uiAbsPartIdx ), pcCU->getContourPredTexDeltaDC2( uiAbsPartIdx ) ); }
  }

Void TComPrediction::xDeltaDCQuantScaleUp( TComDataCU* pcCU, Int& riDeltaDC )
{
  Int  iSign  = riDeltaDC < 0 ? -1 : 1;
  UInt uiAbs  = abs( riDeltaDC );

  Int iQp = pcCU->getQP(0);
  Int iMax = ( 1<<( g_uiBitDepth + g_uiBitIncrement - 1) );
  Double dStepSize = Clip3( 1, iMax, pow( 2.0, iQp/10.0 + g_dDeltaDCsQuantOffset ) );

  riDeltaDC = iSign * roftoi( uiAbs * dStepSize );
  return;
}

Void TComPrediction::xPredIntraWedgeFull( TComDataCU* pcCU, UInt uiAbsPartIdx, Pel* piPred, UInt uiStride, Int iWidth, Int iHeight, Bool bAbove, Bool bLeft, Bool bEncoder, Bool bDelta, UInt uiTabIdx, Int iDeltaDC1, Int iDeltaDC2 )
{
  assert( iWidth >= DMM_WEDGEMODEL_MIN_SIZE && iWidth <= DMM_WEDGEMODEL_MAX_SIZE );
  WedgeList* pacWedgeList = &g_aacWedgeLists[(g_aucConvertToBit[iWidth])];
  TComWedgelet* pcWedgelet = &(pacWedgeList->at(uiTabIdx));

  // get wedge pred DCs
  Int iPredDC1 = 0;
  Int iPredDC2 = 0;

  Int* piMask = pcCU->getPattern()->getAdiOrgBuf( iWidth, iHeight, m_piYuvExt );
  Int iMaskStride = ( iWidth<<1 ) + 1;
  piMask += iMaskStride+1;
  getWedgePredDCs( pcWedgelet, piMask, iMaskStride, iPredDC1, iPredDC2, bAbove, bLeft );

  if( bDelta ) 
  {
    xDeltaDCQuantScaleUp( pcCU, iDeltaDC1 );
    xDeltaDCQuantScaleUp( pcCU, iDeltaDC2 );
  }

  // assign wedge pred DCs to prediction
  if( bDelta ) { assignWedgeDCs2Pred( pcWedgelet, piPred, uiStride, Clip( iPredDC1+iDeltaDC1 ), Clip( iPredDC2+iDeltaDC2 ) ); }
  else         { assignWedgeDCs2Pred( pcWedgelet, piPred, uiStride, iPredDC1,           iPredDC2           ); }
}

Void TComPrediction::getWedgePredDCs( TComWedgelet* pcWedgelet, Int* piMask, Int iMaskStride, Int& riPredDC1, Int& riPredDC2, Bool bAbove, Bool bLeft )
{
  riPredDC1 = ( 1<<( g_uiBitDepth + g_uiBitIncrement - 1) ); //pred val, if no neighbors are available
  riPredDC2 = ( 1<<( g_uiBitDepth + g_uiBitIncrement - 1) );

  if( !bAbove && !bLeft ) { return; }

  UInt uiNumSmpDC1 = 0, uiNumSmpDC2 = 0;
  Int iPredDC1 = 0, iPredDC2 = 0;

  Bool* pabWedgePattern = pcWedgelet->getPattern();
  UInt  uiWedgeStride   = pcWedgelet->getStride();

  if( bAbove )
  {
    for( Int k = 0; k < pcWedgelet->getWidth(); k++ )
    {
      if( true == pabWedgePattern[k] )
      {
        iPredDC2 += piMask[k-iMaskStride];
        uiNumSmpDC2++;
      }
      else
      {
        iPredDC1 += piMask[k-iMaskStride];
        uiNumSmpDC1++;
      }
    }
  }
  if( bLeft )
  {
    for( Int k = 0; k < pcWedgelet->getHeight(); k++ )
    {
      if( true == pabWedgePattern[k*uiWedgeStride] )
      {
        iPredDC2 += piMask[k*iMaskStride-1];
        uiNumSmpDC2++;
      } 
      else
      {
        iPredDC1 += piMask[k*iMaskStride-1];
        uiNumSmpDC1++;
      }
    }
  }

  if( uiNumSmpDC1 > 0 )
  {
    iPredDC1 /= uiNumSmpDC1;
    riPredDC1 = iPredDC1;
  }
  if( uiNumSmpDC2 > 0 )
  {
    iPredDC2 /= uiNumSmpDC2;
    riPredDC2 = iPredDC2;
  }
}

Void TComPrediction::getBestContourFromText( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiWidth, UInt uiHeight, TComWedgelet* pcContourWedge )
{
  pcContourWedge->clear();
  Bool* pabContourPattern = pcContourWedge->getPattern();

  // get copy of according texture luma block
  UInt uiPartAddr = 0;
  Int  iBlockWidth, iBlockHeight;

  pcCU->getPartIndexAndSize( uiAbsPartIdx, uiPartAddr, iBlockWidth, iBlockHeight );

  TComPicYuv* pcPicYuvRef = pcCU->getSlice()->getTexturePic()->getPicYuvRec();
  Int     iRefStride = pcPicYuvRef->getStride();
  Pel*    piRefY     = pcPicYuvRef->getLumaAddr( pcCU->getAddr(), pcCU->getZorderIdxInCU() + uiPartAddr );

  TComYuv cTempYuv; cTempYuv.create( pcCU->getWidth(0), pcCU->getHeight(0) ); cTempYuv.clear();
  UInt uiTempStride = cTempYuv.getStride();
  Pel* piTempY      = cTempYuv.getLumaAddr( uiAbsPartIdx, uiWidth );

  for ( Int y = 0; y < iBlockHeight; y++ )
  {
    ::memcpy(piTempY, piRefY, sizeof(Pel)*iBlockWidth);
    piTempY += uiTempStride;
    piRefY += iRefStride;
  }
  piTempY = cTempYuv.getLumaAddr( uiAbsPartIdx, uiWidth );

  // find contour for texture luma block
  UInt iDC = 0;
  for( UInt k = 0; k < (iBlockWidth*iBlockHeight); k++ ) { iDC += piTempY[k]; }
  iDC /= (iBlockWidth*iBlockHeight);

  for( UInt k = 0; k < (iBlockWidth*iBlockHeight); k++ ) 
  { 
    pabContourPattern[k] = (piTempY[k] > iDC) ? true : false;
  }

  cTempYuv.destroy();
}

UInt TComPrediction::getBestContinueWedge( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiWidth, UInt uiHeight, Int iDeltaEnd )
{
  UInt uiThisBlockSize = uiWidth;
  assert( uiThisBlockSize >= DMM_WEDGEMODEL_MIN_SIZE && uiThisBlockSize <= DMM_WEDGEMODEL_MAX_SIZE );
  WedgeList*    pacContDWedgeList    = &g_aacWedgeLists   [(g_aucConvertToBit[uiThisBlockSize])];
  WedgeRefList* pacContDWedgeRefList = &g_aacWedgeRefLists[(g_aucConvertToBit[uiThisBlockSize])];

  UInt uiPredDirWedgeTabIdx = 0;
  TComDataCU* pcTempCU;
  UInt        uiTempPartIdx;
  // 1st: try continue above wedgelet
  pcTempCU = pcCU->getPUAbove( uiTempPartIdx, pcCU->getZorderIdxInCU() + uiAbsPartIdx );
  if( pcTempCU )
  {
    UChar uhLumaIntraDir = pcTempCU->getLumaIntraDir( uiTempPartIdx );
    if( DMM_WEDGE_FULL_IDX      == uhLumaIntraDir || 
        DMM_WEDGE_FULL_D_IDX    == uhLumaIntraDir || 
        DMM_WEDGE_PREDDIR_IDX   == uhLumaIntraDir || 
        DMM_WEDGE_PREDDIR_D_IDX == uhLumaIntraDir ||
        DMM_WEDGE_PREDTEX_IDX   == uhLumaIntraDir ||
        DMM_WEDGE_PREDTEX_D_IDX == uhLumaIntraDir    )
    {
      UInt uiRefWedgeSize = (UInt)g_aucIntraSizeIdxToWedgeSize[pcTempCU->getIntraSizeIdx( uiTempPartIdx )];
      WedgeList* pacWedgeList = &g_aacWedgeLists[(g_aucConvertToBit[uiRefWedgeSize])];

      // get offset between current and reference block
      UInt uiOffsetX = 0;
      UInt uiOffsetY = 0;
      xGetBlockOffset( pcCU, uiAbsPartIdx, pcTempCU, uiTempPartIdx, uiOffsetX, uiOffsetY );

      // get reference wedgelet
  UInt uiRefWedgeTabIdx = 0;
      switch( uhLumaIntraDir )
      {
      case( DMM_WEDGE_FULL_IDX      ): { uiRefWedgeTabIdx = pcTempCU->getWedgeFullTabIdx   ( uiTempPartIdx ); } break;
      case( DMM_WEDGE_FULL_D_IDX    ): { uiRefWedgeTabIdx = pcTempCU->getWedgeFullTabIdx   ( uiTempPartIdx ); } break;
      case( DMM_WEDGE_PREDDIR_IDX   ): { uiRefWedgeTabIdx = pcTempCU->getWedgePredDirTabIdx( uiTempPartIdx ); } break;
      case( DMM_WEDGE_PREDDIR_D_IDX ): { uiRefWedgeTabIdx = pcTempCU->getWedgePredDirTabIdx( uiTempPartIdx ); } break;
      case( DMM_WEDGE_PREDTEX_IDX   ): { uiRefWedgeTabIdx = pcTempCU->getWedgePredTexTabIdx( uiTempPartIdx ); } break;
      case( DMM_WEDGE_PREDTEX_D_IDX ): { uiRefWedgeTabIdx = pcTempCU->getWedgePredTexTabIdx( uiTempPartIdx ); } break;
      default: { assert( 0 ); return uiPredDirWedgeTabIdx; }
      }
      TComWedgelet* pcRefWedgelet;
      pcRefWedgelet = &(pacWedgeList->at( uiRefWedgeTabIdx ));
      
      // find reference wedgelet, if direction is suitable for continue wedge
      if( pcRefWedgelet->checkPredDirAbovePossible( uiThisBlockSize, uiOffsetX ) )
      {
        UChar uhContD_Xs, uhContD_Ys, uhContD_Xe, uhContD_Ye;
        pcRefWedgelet->getPredDirStartEndAbove( uhContD_Xs, uhContD_Ys, uhContD_Xe, uhContD_Ye, uiThisBlockSize, uiOffsetX, iDeltaEnd );
        getWedgeListIdx( pacContDWedgeList, pacContDWedgeRefList, uiPredDirWedgeTabIdx, uhContD_Xs, uhContD_Ys, uhContD_Xe, uhContD_Ye );
        return uiPredDirWedgeTabIdx;
      }
    }
  }

  // 2nd: try continue left wedglelet
  pcTempCU = pcCU->getPULeft( uiTempPartIdx, pcCU->getZorderIdxInCU() + uiAbsPartIdx );
  if( pcTempCU )
  {
  UChar uhLumaIntraDir = pcTempCU->getLumaIntraDir( uiTempPartIdx );
  if( DMM_WEDGE_FULL_IDX          == uhLumaIntraDir || 
      DMM_WEDGE_FULL_D_IDX        == uhLumaIntraDir || 
        DMM_WEDGE_PREDDIR_IDX   == uhLumaIntraDir || 
        DMM_WEDGE_PREDDIR_D_IDX == uhLumaIntraDir ||
      DMM_WEDGE_PREDTEX_IDX       == uhLumaIntraDir ||
      DMM_WEDGE_PREDTEX_D_IDX     == uhLumaIntraDir    )
  {
      UInt uiRefWedgeSize = (UInt)g_aucIntraSizeIdxToWedgeSize[pcTempCU->getIntraSizeIdx( uiTempPartIdx )];
      WedgeList* pacWedgeList = &g_aacWedgeLists[(g_aucConvertToBit[uiRefWedgeSize])];

      // get offset between current and reference block
      UInt uiOffsetX = 0;
      UInt uiOffsetY = 0;
      xGetBlockOffset( pcCU, uiAbsPartIdx, pcTempCU, uiTempPartIdx, uiOffsetX, uiOffsetY );

      // get reference wedgelet
      UInt uiRefWedgeTabIdx = 0;
      switch( uhLumaIntraDir )
      {
      case( DMM_WEDGE_FULL_IDX      ): { uiRefWedgeTabIdx = pcTempCU->getWedgeFullTabIdx   ( uiTempPartIdx ); } break;
      case( DMM_WEDGE_FULL_D_IDX    ): { uiRefWedgeTabIdx = pcTempCU->getWedgeFullTabIdx   ( uiTempPartIdx ); } break;
      case( DMM_WEDGE_PREDDIR_IDX   ): { uiRefWedgeTabIdx = pcTempCU->getWedgePredDirTabIdx( uiTempPartIdx ); } break;
      case( DMM_WEDGE_PREDDIR_D_IDX ): { uiRefWedgeTabIdx = pcTempCU->getWedgePredDirTabIdx( uiTempPartIdx ); } break;
      case( DMM_WEDGE_PREDTEX_IDX   ): { uiRefWedgeTabIdx = pcTempCU->getWedgePredTexTabIdx( uiTempPartIdx ); } break;
      case( DMM_WEDGE_PREDTEX_D_IDX ): { uiRefWedgeTabIdx = pcTempCU->getWedgePredTexTabIdx( uiTempPartIdx ); } break;
      default: { assert( 0 ); return uiPredDirWedgeTabIdx; }
      }
      TComWedgelet* pcRefWedgelet;
      pcRefWedgelet = &(pacWedgeList->at( uiRefWedgeTabIdx ));

      // find reference wedgelet, if direction is suitable for continue wedge
      if( pcRefWedgelet->checkPredDirLeftPossible( uiThisBlockSize, uiOffsetY ) )
      {
        UChar uhContD_Xs, uhContD_Ys, uhContD_Xe, uhContD_Ye;
        pcRefWedgelet->getPredDirStartEndLeft( uhContD_Xs, uhContD_Ys, uhContD_Xe, uhContD_Ye, uiThisBlockSize, uiOffsetY, iDeltaEnd );
        getWedgeListIdx( pacContDWedgeList, pacContDWedgeRefList, uiPredDirWedgeTabIdx, uhContD_Xs, uhContD_Ys, uhContD_Xe, uhContD_Ye );
        return uiPredDirWedgeTabIdx;
      }
    }
  }

  // 3rd: (default) make wedglet from intra dir and max slope point
  Int iSlopeX = 0;
  Int iSlopeY = 0;
  UInt uiStartPosX = 0;
  UInt uiStartPosY = 0;
  if( xGetWedgeIntraDirPredData( pcCU, uiAbsPartIdx, uiThisBlockSize, iSlopeX, iSlopeY, uiStartPosX, uiStartPosY ) )
  {
    UChar uhContD_Xs, uhContD_Ys, uhContD_Xe, uhContD_Ye;
    xGetWedgeIntraDirStartEnd( pcCU, uiAbsPartIdx, uiThisBlockSize, iSlopeX, iSlopeY, uiStartPosX, uiStartPosY, uhContD_Xs, uhContD_Ys, uhContD_Xe, uhContD_Ye, iDeltaEnd );
    getWedgeListIdx( pacContDWedgeList, pacContDWedgeRefList, uiPredDirWedgeTabIdx, uhContD_Xs, uhContD_Ys, uhContD_Xe, uhContD_Ye );
    return uiPredDirWedgeTabIdx;
  }

  return uiPredDirWedgeTabIdx;
}

Void TComPrediction::xGetBlockOffset( TComDataCU* pcCU, UInt uiAbsPartIdx, TComDataCU* pcRefCU, UInt uiRefAbsPartIdx, UInt& ruiOffsetX, UInt& ruiOffsetY )
{
  ruiOffsetX = 0;
  ruiOffsetY = 0;

    // get offset between current and above/left block
    UInt uiThisOriginX = pcCU->getCUPelX() + g_auiRasterToPelX[ g_auiZscanToRaster[uiAbsPartIdx] ];
  UInt uiThisOriginY = pcCU->getCUPelY() + g_auiRasterToPelY[ g_auiZscanToRaster[uiAbsPartIdx] ];

  UInt uiNumPartInRefCU = pcRefCU->getTotalNumPart();
    UInt uiMaxDepthRefCU = 0;
    while( uiNumPartInRefCU > 1 )
    {
      uiNumPartInRefCU >>= 2;
      uiMaxDepthRefCU++;
    }

  UInt uiDepthRefPU = (pcRefCU->getDepth(uiRefAbsPartIdx)) + (pcRefCU->getPartitionSize(uiRefAbsPartIdx) == SIZE_2Nx2N ? 0 : 1);
    UInt uiShifts = (uiMaxDepthRefCU - uiDepthRefPU)*2;
  UInt uiRefBlockOriginPartIdx = (uiRefAbsPartIdx>>uiShifts)<<uiShifts;

  UInt uiRefOriginX = pcRefCU->getCUPelX() + g_auiRasterToPelX[ g_auiZscanToRaster[uiRefBlockOriginPartIdx] ];
  UInt uiRefOriginY = pcRefCU->getCUPelY() + g_auiRasterToPelY[ g_auiZscanToRaster[uiRefBlockOriginPartIdx] ];

  if( (uiThisOriginX - uiRefOriginX) > 0 ) { ruiOffsetX = (UInt)(uiThisOriginX - uiRefOriginX); }
  if( (uiThisOriginY - uiRefOriginY) > 0 ) { ruiOffsetY = (UInt)(uiThisOriginY - uiRefOriginY); }
}

Bool TComPrediction::xGetWedgeIntraDirPredData( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiBlockSize, Int& riSlopeX, Int& riSlopeY, UInt& ruiStartPosX, UInt& ruiStartPosY )
{
  riSlopeX     = 0;
  riSlopeY     = 0;
  ruiStartPosX = 0;
  ruiStartPosY = 0;

  // 1st step: get wedge start point (max. slope)
  Int* piSource = pcCU->getPattern()->getAdiOrgBuf( uiBlockSize, uiBlockSize, m_piYuvExt );
  Int iSourceStride = ( uiBlockSize<<1 ) + 1;

  UInt uiSlopeMaxAbove = 0;
  UInt uiPosSlopeMaxAbove = 0;
  for( UInt uiPosHor = 0; uiPosHor < (uiBlockSize-1); uiPosHor++ )
  {
    if( abs( piSource[uiPosHor+1] - piSource[uiPosHor] ) > uiSlopeMaxAbove )
    {
      uiSlopeMaxAbove = abs( piSource[uiPosHor+1] - piSource[uiPosHor] );
      uiPosSlopeMaxAbove = uiPosHor;
    }
  }

  UInt uiSlopeMaxLeft = 0;
  UInt uiPosSlopeMaxLeft = 0;
  for( UInt uiPosVer = 0; uiPosVer < (uiBlockSize-1); uiPosVer++ )
  {
    if( abs( piSource[(uiPosVer+1)*iSourceStride] - piSource[uiPosVer*iSourceStride] ) > uiSlopeMaxLeft )
    {
      uiSlopeMaxLeft = abs( piSource[(uiPosVer+1)*iSourceStride] - piSource[uiPosVer*iSourceStride] );
      uiPosSlopeMaxLeft = uiPosVer;
    }
  }

  if( uiSlopeMaxAbove == 0 && uiSlopeMaxLeft == 0 ) 
  { 
    return false; 
  }

  if( uiSlopeMaxAbove > uiSlopeMaxLeft )
  {
    ruiStartPosX = uiPosSlopeMaxAbove;
    ruiStartPosY = 0;
  }
  else
  {
    ruiStartPosX = 0;
    ruiStartPosY = uiPosSlopeMaxLeft;
  }

  // 2nd step: derive wedge direction
  Int angTable[9] = {0,2,5,9,13,17,21,26,32};

  Int uiPreds[2] = {-1, -1};
  Int uiPredNum = pcCU->getIntraDirLumaPredictor( uiAbsPartIdx, uiPreds );

  UInt uiDirMode = 0;
  if( uiPredNum == 1 )
  {
    uiDirMode = g_aucAngIntraModeOrder[uiPreds[0]];
  }
  else if( uiPredNum == 2 )
  {
    uiDirMode = g_aucAngIntraModeOrder[uiPreds[1]];
  }

  if( uiDirMode == 0 ) 
  { 
    return false; 
  }

  Bool modeVer       = (uiDirMode < 18);
  Bool modeHor       = !modeVer;
  Int intraPredAngle = modeVer ? uiDirMode - 9 : modeHor ? uiDirMode - 25 : 0;
  Int absAng         = abs(intraPredAngle);
  Int signAng        = intraPredAngle < 0 ? -1 : 1;
  absAng             = angTable[absAng];
  intraPredAngle     = signAng * absAng;

  // 3rd step: set slope for direction
  if( modeHor )
    {
    if( intraPredAngle > 0 )
    {
      riSlopeX = -32;
      riSlopeY = intraPredAngle;
    }
    else
    {
      riSlopeX = 32;
      riSlopeY = -intraPredAngle;
    }
    }
  else if( modeVer )
  {
    if( intraPredAngle > 0 )
    {
      riSlopeX = intraPredAngle;
      riSlopeY = -32;
  }
  else
  {
      riSlopeX = -intraPredAngle;
      riSlopeY = 32;
    }
  }

    return true;
}

Void TComPrediction::xGetWedgeIntraDirStartEnd( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiBlockSize, Int iDeltaX, Int iDeltaY, UInt uiPMSPosX, UInt uiPMSPosY, UChar& ruhXs, UChar& ruhYs, UChar& ruhXe, UChar& ruhYe, Int iDeltaEnd )
{
  ruhXs = 0;
  ruhYs = 0;
  ruhXe = 0;
  ruhYe = 0;
  UInt uiOri;

  // scaling of start pos and block size to wedge resolution
  UInt uiScaledStartPosX = 0;
  UInt uiScaledStartPosY = 0;
  UInt uiScaledBlockSize = 0;
  WedgeResolution eWedgeRes = g_aeWedgeResolutionList[(UInt)g_aucConvertToBit[uiBlockSize]];
  switch( eWedgeRes )
  {
  case( DOUBLE_PEL ): { uiScaledStartPosX = (uiPMSPosX>>1); uiScaledStartPosY = (uiPMSPosY>>1); uiScaledBlockSize = (uiBlockSize>>1); break; }
  case(   FULL_PEL ): { uiScaledStartPosX =  uiPMSPosX;     uiScaledStartPosY =  uiPMSPosY;     uiScaledBlockSize =  uiBlockSize;     break; }
  case(   HALF_PEL ): { uiScaledStartPosX = (uiPMSPosX<<1); uiScaledStartPosY = (uiPMSPosY<<1); uiScaledBlockSize = (uiBlockSize<<1); break; }
}

  // case above
  if( uiScaledStartPosX > 0 && uiScaledStartPosY == 0 )
  {
    ruhXs = (UChar)uiScaledStartPosX;
    ruhYs = 0;

    if( iDeltaY == 0 )
{
      if( iDeltaX < 0 )
      {
        uiOri = 0;
        ruhXe = 0;
        ruhYe = (UChar)Min( Max( iDeltaEnd, 0 ), (uiScaledBlockSize-1) );
        return;
      }
      else
      {
        uiOri = 1;
        ruhXe = (UChar)(uiScaledBlockSize-1); ;
        ruhYe = (UChar)Min( Max( -iDeltaEnd, 0 ), (uiScaledBlockSize-1) );
        std::swap( ruhXs, ruhXe );
        std::swap( ruhYs, ruhYe );
        return;
      }
    }

    // regular case
    Int iVirtualEndX = (Int)ruhXs + roftoi( (Double)(uiScaledBlockSize-1) * ((Double)iDeltaX / (Double)iDeltaY) );

    if( iVirtualEndX < 0 )
    {
      Int iYe = roftoi( (Double)(0 - (Int)ruhXs) * ((Double)iDeltaY / (Double)iDeltaX) ) + iDeltaEnd;
      if( iYe < (Int)uiScaledBlockSize )
      {
        uiOri = 0;
        ruhXe = 0;
        ruhYe = (UChar)Max( iYe, 0 );
        return;
      }
      else
      {
        uiOri = 4;
        ruhXe = (UChar)Min( (iYe - (uiScaledBlockSize-1)), (uiScaledBlockSize-1) );
        ruhYe = (UChar)(uiScaledBlockSize-1);
        return;
      }
    }
    else if( iVirtualEndX > (uiScaledBlockSize-1) )
    {
      Int iYe = roftoi( (Double)((Int)(uiScaledBlockSize-1) - (Int)ruhXs) * ((Double)iDeltaY / (Double)iDeltaX) ) - iDeltaEnd;
      if( iYe < (Int)uiScaledBlockSize )
      {
        uiOri = 1;
        ruhXe = (UChar)(uiScaledBlockSize-1);
        ruhYe = (UChar)Max( iYe, 0 );
        std::swap( ruhXs, ruhXe );
        std::swap( ruhYs, ruhYe );
        return;
      }
      else
      {
        uiOri = 4;
        ruhXe = (UChar)Max( ((uiScaledBlockSize-1) - (iYe - (uiScaledBlockSize-1))), 0 );
        ruhYe = (UChar)(uiScaledBlockSize-1);
        return;
      }
    }
    else
    {
      Int iXe = iVirtualEndX + iDeltaEnd;
      if( iXe < 0 )
      {
        uiOri = 0;
        ruhXe = 0;
        ruhYe = (UChar)Max( ((uiScaledBlockSize-1) + iXe), 0 );
        return;
      }
      else if( iXe > (uiScaledBlockSize-1) )
      {
        uiOri = 1;
        ruhXe = (UChar)(uiScaledBlockSize-1);
        ruhYe = (UChar)Max( ((uiScaledBlockSize-1) - (iXe - (uiScaledBlockSize-1))), 0 );
        std::swap( ruhXs, ruhXe );
        std::swap( ruhYs, ruhYe );
        return;
      }
      else
      {
        uiOri = 4;
        ruhXe = (UChar)iXe;
        ruhYe = (UChar)(uiScaledBlockSize-1);
        return;
      }
    }
  }

  // case left
  if( uiScaledStartPosY > 0 && uiScaledStartPosX == 0 )
  {
    ruhXs = 0;
    ruhYs = (UChar)uiScaledStartPosY;

    if( iDeltaX == 0 )
    {
      if( iDeltaY < 0 )
      {
        uiOri = 0;
        ruhXe = (UChar)Min( Max( -iDeltaEnd, 0 ), (uiScaledBlockSize-1) );
        ruhYe = 0;
        std::swap( ruhXs, ruhXe );
        std::swap( ruhYs, ruhYe );
        return;
      }
      else
      {
        uiOri = 3;
        ruhXe = (UChar)Min( Max( iDeltaEnd, 0 ), (uiScaledBlockSize-1) );
        ruhYe = (UChar)(uiScaledBlockSize-1); 
        return; 
      }
    }

    // regular case
    Int iVirtualEndY = (Int)ruhYs + roftoi( (Double)(uiScaledBlockSize-1) * ((Double)iDeltaY / (Double)iDeltaX) );

    if( iVirtualEndY < 0 )
    {
      Int iXe = roftoi( (Double)(0 - (Int)ruhYs ) * ((Double)iDeltaX / (Double)iDeltaY) ) - iDeltaEnd;
      if( iXe < (Int)uiScaledBlockSize )
      {
        uiOri = 0;
        ruhXe = (UChar)Max( iXe, 0 );
        ruhYe = 0;
        std::swap( ruhXs, ruhXe );
        std::swap( ruhYs, ruhYe );
        return;
      }
      else
      {
        uiOri = 5;
        ruhXe = (UChar)(uiScaledBlockSize-1);
        ruhYe = (UChar)Min( (iXe - (uiScaledBlockSize-1)), (uiScaledBlockSize-1) );
        std::swap( ruhXs, ruhXe );
        std::swap( ruhYs, ruhYe );
        return;
      }
    }
    else if( iVirtualEndY > (uiScaledBlockSize-1) )
    {
      Int iXe = roftoi( (Double)((Int)(uiScaledBlockSize-1) - (Int)ruhYs ) * ((Double)iDeltaX / (Double)iDeltaY) ) + iDeltaEnd;
      if( iXe < (Int)uiScaledBlockSize )
      {
        uiOri = 3;
        ruhXe = (UChar)Max( iXe, 0 );
        ruhYe = (UChar)(uiScaledBlockSize-1);
        return;
      }
      else
      {
        uiOri = 5;
        ruhXe = (UChar)(uiScaledBlockSize-1);
        ruhYe = (UChar)Max( ((uiScaledBlockSize-1) - (iXe - (uiScaledBlockSize-1))), 0 );
        std::swap( ruhXs, ruhXe );
        std::swap( ruhYs, ruhYe );
        return;
      }
    }
    else
    {
      Int iYe = iVirtualEndY - iDeltaEnd;
      if( iYe < 0 )
      {
        uiOri = 0;
        ruhXe = (UChar)Max( ((uiScaledBlockSize-1) + iYe), 0 );
        ruhYe = 0;
        std::swap( ruhXs, ruhXe );
        std::swap( ruhYs, ruhYe );
        return;
      }
      else if( iYe > (uiScaledBlockSize-1) )
      {
        uiOri = 3;
        ruhXe = (UChar)Max( ((uiScaledBlockSize-1) - (iYe - (uiScaledBlockSize-1))), 0 );
        ruhYe = (UChar)(uiScaledBlockSize-1);
        return;
      }
      else
      {
        uiOri = 5;
        ruhXe = (UChar)(uiScaledBlockSize-1);
        ruhYe = (UChar)iYe;
        std::swap( ruhXs, ruhXe );
        std::swap( ruhYs, ruhYe );
        return;
      }
    }
  }

  // case origin
  if( uiScaledStartPosX == 0 && uiScaledStartPosY == 0 )
  {
    if( iDeltaX*iDeltaY < 0 )
    {
      return;
    }

    ruhXs = 0;
    ruhYs = 0;

    if( iDeltaY == 0 )
    {
      uiOri = 1;
      ruhXe = (UChar)(uiScaledBlockSize-1);
      ruhYe = 0;
      std::swap( ruhXs, ruhXe );
      std::swap( ruhYs, ruhYe );
      return;
  }

    if( iDeltaX == 0 )
    {
      uiOri = 0;
      ruhXe = 0;
      ruhYe = (UChar)(uiScaledBlockSize-1);
      return;
  }

    Int iVirtualEndX = (Int)ruhXs + roftoi( (Double)(uiScaledBlockSize-1) * ((Double)iDeltaX / (Double)iDeltaY) );

    if( iVirtualEndX > (uiScaledBlockSize-1) )
    {
      Int iYe = roftoi( (Double)((Int)(uiScaledBlockSize-1) - (Int)ruhXs) * ((Double)iDeltaY / (Double)iDeltaX) ) - iDeltaEnd;
      if( iYe < (Int)uiScaledBlockSize )
      {
        uiOri = 1;
        ruhXe = (UChar)(uiScaledBlockSize-1);
        ruhYe = (UChar)Max( iYe, 0 );
        std::swap( ruhXs, ruhXe );
        std::swap( ruhYs, ruhYe );
        return;
      }
      else
      {
        uiOri = 3;
        ruhXe = (UChar)Max( ((uiScaledBlockSize-1) - (iYe - (uiScaledBlockSize-1))), 0 );
        ruhYe = (UChar)(uiScaledBlockSize-1);
        return;
      }
    }
    else
    {
      Int iXe = iVirtualEndX + iDeltaEnd;
      if( iXe < 0 )
      {
        uiOri = 0;
        ruhXe = 0;
        ruhYe = (UChar)Max( ((uiScaledBlockSize-1) + iXe), 0 );
        return;
      }
      else if( iXe > (uiScaledBlockSize-1) )
      {
        uiOri = 1;
        ruhXe = (UChar)(uiScaledBlockSize-1);
        ruhYe = (UChar)Max( ((uiScaledBlockSize-1) - (iXe - (uiScaledBlockSize-1))), 0 );
        std::swap( ruhXs, ruhXe );
        std::swap( ruhYs, ruhYe );
        return;
      }
      else
      {
        uiOri = 3;
        ruhXe = (UChar)iXe;
        ruhYe = (UChar)(uiScaledBlockSize-1);
        return;
      }
    }
  }
}

Bool TComPrediction::getWedgeListIdx( WedgeList* pcWedgeList, WedgeRefList* pcWedgeRefList, UInt& ruiTabIdx, UChar uhXs, UChar uhYs, UChar uhXe, UChar uhYe )
      {
  ruiTabIdx = 0;

  for( UInt uiIdx = 0; uiIdx < pcWedgeList->size(); uiIdx++ )
      {
    TComWedgelet* pcTestWedge = &(pcWedgeList->at(uiIdx));

    if( pcTestWedge->getStartX() == uhXs &&
        pcTestWedge->getStartY() == uhYs &&
        pcTestWedge->getEndX()   == uhXe &&
        pcTestWedge->getEndY()   == uhYe    )
        {
      ruiTabIdx = uiIdx;
      return true;
        }
      }

  // additionally search in WedgeRef lists of duplicated patterns
  for( UInt uiIdx = 0; uiIdx < pcWedgeRefList->size(); uiIdx++ )
        {
    TComWedgeRef* pcTestWedgeRef = &(pcWedgeRefList->at(uiIdx));

    if( pcTestWedgeRef->getStartX() == uhXs &&
        pcTestWedgeRef->getStartY() == uhYs &&
        pcTestWedgeRef->getEndX()   == uhXe &&
        pcTestWedgeRef->getEndY()   == uhYe    )
          {
      ruiTabIdx = pcTestWedgeRef->getRefIdx();
      return true;
    }
  }

  return false;
}

Void TComPrediction::xPredIntraWedgeDir( TComDataCU* pcCU, UInt uiAbsPartIdx, Pel* piPred, UInt uiStride, Int iWidth, Int iHeight, Bool bAbove, Bool bLeft, Bool bEncoder, Bool bDelta, Int iWedgeDeltaEnd, Int iDeltaDC1, Int iDeltaDC2 )
{
  assert( iWidth >= DMM_WEDGEMODEL_MIN_SIZE && iWidth <= DMM_WEDGEMODEL_MAX_SIZE );
  WedgeList* pacWedgeList = &g_aacWedgeLists[(g_aucConvertToBit[iWidth])];

  // get wedge pattern
  UInt uiDirWedgeTabIdx = 0;
  if( bEncoder )
  {
    // encoder: load stored wedge pattern from CU
    uiDirWedgeTabIdx = pcCU->getWedgePredDirTabIdx( uiAbsPartIdx );
  }
  else
  {
    uiDirWedgeTabIdx = getBestContinueWedge( pcCU, uiAbsPartIdx, iWidth, iHeight, iWedgeDeltaEnd );

    UInt uiDepth = (pcCU->getDepth(0)) + (pcCU->getPartitionSize(0) == SIZE_2Nx2N ? 0 : 1);
    pcCU->setWedgePredDirTabIdxSubParts( uiDirWedgeTabIdx, uiAbsPartIdx, uiDepth );
  }
  TComWedgelet* pcWedgelet = &(pacWedgeList->at(uiDirWedgeTabIdx));

  // get wedge pred DCs
  Int iPredDC1 = 0;
  Int iPredDC2 = 0;

  Int* piMask = pcCU->getPattern()->getAdiOrgBuf( iWidth, iHeight, m_piYuvExt );
  Int iMaskStride = ( iWidth<<1 ) + 1;
  piMask += iMaskStride+1;
  getWedgePredDCs( pcWedgelet, piMask, iMaskStride, iPredDC1, iPredDC2, bAbove, bLeft );

  if( bDelta ) 
  {
    xDeltaDCQuantScaleUp( pcCU, iDeltaDC1 );
    xDeltaDCQuantScaleUp( pcCU, iDeltaDC2 );
  }

  // assign wedge pred DCs to prediction
  if( bDelta ) { assignWedgeDCs2Pred( pcWedgelet, piPred, uiStride, Clip ( iPredDC1+iDeltaDC1 ), Clip( iPredDC2+iDeltaDC2 ) ); }
  else         { assignWedgeDCs2Pred( pcWedgelet, piPred, uiStride,        iPredDC1,                   iPredDC2             ); }
}

Void TComPrediction::xPredIntraWedgeTex( TComDataCU* pcCU, UInt uiAbsPartIdx, Pel* piPred, UInt uiStride, Int iWidth, Int iHeight, Bool bAbove, Bool bLeft, Bool bEncoder, Bool bDelta, Int iDeltaDC1, Int iDeltaDC2 )
{
  assert( iWidth >= DMM_WEDGEMODEL_MIN_SIZE && iWidth <= DMM_WEDGEMODEL_MAX_SIZE );
  WedgeList* pacWedgeList = &g_aacWedgeLists[(g_aucConvertToBit[iWidth])];

  // get wedge pattern
  UInt uiTextureWedgeTabIdx = 0;
  if( bEncoder ) 
  {
     // encoder: load stored wedge pattern from CU
    uiTextureWedgeTabIdx = pcCU->getWedgePredTexTabIdx( uiAbsPartIdx );
  }
  else
  {
    // decoder: get and store wedge pattern in CU
    uiTextureWedgeTabIdx = getBestWedgeFromText( pcCU, uiAbsPartIdx, (UInt)iWidth, (UInt)iHeight );
    UInt uiDepth = (pcCU->getDepth(0)) + (pcCU->getPartitionSize(0) == SIZE_2Nx2N ? 0 : 1);
    pcCU->setWedgePredTexTabIdxSubParts( uiTextureWedgeTabIdx, uiAbsPartIdx, uiDepth );
  }
  TComWedgelet* pcWedgelet = &(pacWedgeList->at(uiTextureWedgeTabIdx));

  // get wedge pred DCs
  Int iPredDC1 = 0;
  Int iPredDC2 = 0;
  Int* piMask = pcCU->getPattern()->getAdiOrgBuf( iWidth, iHeight, m_piYuvExt );
  Int iMaskStride = ( iWidth<<1 ) + 1;
  piMask += iMaskStride+1;
  getWedgePredDCs( pcWedgelet, piMask, iMaskStride, iPredDC1, iPredDC2, bAbove, bLeft );

  if( bDelta ) 
  {
    xDeltaDCQuantScaleUp( pcCU, iDeltaDC1 );
    xDeltaDCQuantScaleUp( pcCU, iDeltaDC2 );
  }

  // assign wedge pred DCs to prediction
  if( bDelta ) { assignWedgeDCs2Pred( pcWedgelet, piPred, uiStride, Clip ( iPredDC1+iDeltaDC1 ), Clip( iPredDC2+iDeltaDC2 ) ); }
  else         { assignWedgeDCs2Pred( pcWedgelet, piPred, uiStride,        iPredDC1,                   iPredDC2           ); }
}

Void TComPrediction::xPredIntraContourTex( TComDataCU* pcCU, UInt uiAbsPartIdx, Pel* piPred, UInt uiStride, Int iWidth, Int iHeight, Bool bAbove, Bool bLeft, Bool bEncoder, Bool bDelta, Int iDeltaDC1, Int iDeltaDC2 )
{
  // get contour pattern
  TComWedgelet* pcContourWedge = new TComWedgelet( iWidth, iHeight );
  getBestContourFromText( pcCU, uiAbsPartIdx, (UInt)iWidth, (UInt)iHeight, pcContourWedge );

  // get wedge pred DCs
  Int iPredDC1 = 0;
  Int iPredDC2 = 0;
  Int* piMask = pcCU->getPattern()->getAdiOrgBuf( iWidth, iHeight, m_piYuvExt );
  Int iMaskStride = ( iWidth<<1 ) + 1;
  piMask += iMaskStride+1;
  getWedgePredDCs( pcContourWedge, piMask, iMaskStride, iPredDC1, iPredDC2, bAbove, bLeft );

  if( bDelta ) 
  {
    xDeltaDCQuantScaleUp( pcCU, iDeltaDC1 );
    xDeltaDCQuantScaleUp( pcCU, iDeltaDC2 );
  }

  // assign wedge pred DCs to prediction
  if( bDelta ) { assignWedgeDCs2Pred( pcContourWedge, piPred, uiStride, Clip ( iPredDC1+iDeltaDC1 ), Clip( iPredDC2+iDeltaDC2 ) ); }
  else         { assignWedgeDCs2Pred( pcContourWedge, piPred, uiStride,        iPredDC1,                   iPredDC2           ); }

  pcContourWedge->destroy();
  delete pcContourWedge;
}
#endif
#if HHI_DMM_INTRA
Void TComPrediction::calcWedgeDCs( TComWedgelet* pcWedgelet, Pel* piOrig, UInt uiStride, Int& riDC1, Int& riDC2 )
{
  UInt uiDC1 = 0;
  UInt uiDC2 = 0;
  UInt uiNumPixDC1 = 0, uiNumPixDC2 = 0;
  Bool* pabWedgePattern = pcWedgelet->getPattern();
  if( uiStride == pcWedgelet->getStride() )
  {
    for( UInt k = 0; k < (pcWedgelet->getWidth() * pcWedgelet->getHeight()); k++ )
    {
      if( true == pabWedgePattern[k] ) 
      {
        uiDC2 += piOrig[k];
        uiNumPixDC2++;
      }
      else
      {
        uiDC1 += piOrig[k];
        uiNumPixDC1++;
      }
    }
  }
  else
  {
    Pel* piTemp = piOrig;
    UInt uiWedgeStride = pcWedgelet->getStride();
    for( UInt uiY = 0; uiY < pcWedgelet->getHeight(); uiY++ )
    {
      for( UInt uiX = 0; uiX < pcWedgelet->getWidth(); uiX++ )
      {
        if( true == pabWedgePattern[uiX] ) 
        {
          uiDC2 += piTemp[uiX];
          uiNumPixDC2++;
        }
        else
        {
          uiDC1 += piTemp[uiX];
          uiNumPixDC1++;
        }
      }
      piTemp          += uiStride;
      pabWedgePattern += uiWedgeStride;
    }
  }

  if( uiNumPixDC1 > 0 ) { riDC1 = uiDC1 / uiNumPixDC1; }
  else                  { riDC1 = ( 1<<( g_uiBitDepth + g_uiBitIncrement - 1) ); }

  if( uiNumPixDC2 > 0 ) { riDC2 = uiDC2 / uiNumPixDC2; }
  else                  { riDC2 = ( 1<<( g_uiBitDepth + g_uiBitIncrement - 1) ); }
}

Void TComPrediction::assignWedgeDCs2Pred( TComWedgelet* pcWedgelet, Pel* piPred, UInt uiStride, Int iDC1, Int iDC2 )
{
  Bool* pabWedgePattern = pcWedgelet->getPattern();

  if( uiStride == pcWedgelet->getStride() )
  {
    for( UInt k = 0; k < (pcWedgelet->getWidth() * pcWedgelet->getHeight()); k++ )
    {
      if( true == pabWedgePattern[k] ) 
      {
        piPred[k] = iDC2;
      }
      else
      {
        piPred[k] = iDC1;
      }
    }
  }
  else
  {
    Pel* piTemp = piPred;
    UInt uiWedgeStride = pcWedgelet->getStride();
    for( UInt uiY = 0; uiY < pcWedgelet->getHeight(); uiY++ )
    {
      for( UInt uiX = 0; uiX < pcWedgelet->getWidth(); uiX++ )
      {
        if( true == pabWedgePattern[uiX] ) 
        {
          piTemp[uiX] = iDC2;
        }
        else
        {
          piTemp[uiX] = iDC1;
        }
      }
      piTemp          += uiStride;
      pabWedgePattern += uiWedgeStride;
    }
  }
}

UInt TComPrediction::getBestWedgeFromText( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiWidth, UInt uiHeight, WedgeDist eWedgeDist )
{
  assert( uiWidth >= DMM_WEDGEMODEL_MIN_SIZE && uiWidth <= DMM_WEDGEMODEL_MAX_SIZE );
  WedgeList* pacWedgeList = &g_aacWedgeLists[(g_aucConvertToBit[uiWidth])];

  // get copy of according texture luma block
  UInt uiPartAddr = 0;
  Int  iBlockWidth, iBlockHeight;
  TComYuv     cTempYuv; 
  UInt        uiTempStride;
  Pel*        piTempY;     

  TComPicYuv* pcPicYuvRef = pcCU->getSlice()->getTexturePic()->getPicYuvRec();
  Int         iRefStride = pcPicYuvRef->getStride();
  Pel*        piRefY;

  pcCU->getPartIndexAndSize( uiAbsPartIdx, uiPartAddr, iBlockWidth, iBlockHeight );

  piRefY = pcPicYuvRef->getLumaAddr( pcCU->getAddr(), pcCU->getZorderIdxInCU() + uiPartAddr );

  cTempYuv.create( pcCU->getWidth(0), pcCU->getHeight(0) ); cTempYuv.clear();
  uiTempStride = cTempYuv.getStride();
  piTempY      = cTempYuv.getLumaAddr( uiAbsPartIdx, uiWidth );

  for ( Int y = 0; y < iBlockHeight; y++ )
  {
    ::memcpy(piTempY, piRefY, sizeof(Pel)*iBlockWidth);
    piTempY += uiTempStride;
    piRefY += iRefStride;
  }

  piTempY = cTempYuv.getLumaAddr( uiAbsPartIdx, uiWidth );

  TComWedgeDist cWedgeDist;
  UInt uiTextureWedgeTabIdx = 0;

  // local pred buffer
  TComYuv cPredYuv; 
  cPredYuv.create( uiWidth, uiHeight ); 
  cPredYuv.clear();

  UInt uiPredStride = cPredYuv.getStride();
  Pel* piPred       = cPredYuv.getLumaAddr();

  Int  iDC1 = 0;
  Int  iDC2 = 0;
  // regular wedge search
  UInt uiBestDist   = MAX_UINT;
  UInt uiBestTabIdx = 0;

  for( UInt uiIdx = 0; uiIdx < pacWedgeList->size(); uiIdx++ )
  {
    calcWedgeDCs       ( &(pacWedgeList->at(uiIdx)), piTempY,  uiTempStride,  iDC1, iDC2 );
    assignWedgeDCs2Pred( &(pacWedgeList->at(uiIdx)), piPred, uiPredStride, iDC1, iDC2 );

    UInt uiActDist = cWedgeDist.getDistPart( piPred, uiPredStride, piTempY, uiTempStride, uiWidth, uiHeight, eWedgeDist );

    if( uiActDist < uiBestDist || uiBestDist == MAX_UINT )
    {
      uiBestDist   = uiActDist;
      uiBestTabIdx = uiIdx;
    }
  }
  uiTextureWedgeTabIdx = uiBestTabIdx;

  cPredYuv.destroy();
  cTempYuv.destroy();
  return uiTextureWedgeTabIdx;
}
#endif

Void TComPrediction::motionCompensation ( TComDataCU* pcCU, TComYuv* pcYuvPred, RefPicList eRefPicList, Int iPartIdx, Bool bPrdDepthMap )
{
  Int         iWidth;
  Int         iHeight;
  UInt        uiPartAddr;

  if ( iPartIdx >= 0 )
  {
    pcCU->getPartIndexAndSize( iPartIdx, uiPartAddr, iWidth, iHeight );
    if ( eRefPicList != REF_PIC_LIST_X )
    {
      xPredInterUni (pcCU, uiPartAddr, iWidth, iHeight, eRefPicList, pcYuvPred, iPartIdx, bPrdDepthMap );
#ifdef WEIGHT_PRED
      if ( pcCU->getSlice()->getPPS()->getUseWP() )
      {
        xWeightedPredictionUni( pcCU, pcYuvPred, uiPartAddr, iWidth, iHeight, eRefPicList, pcYuvPred, iPartIdx );
      }
#endif    
    }
    else
    {
      xPredInterBi  (pcCU, uiPartAddr, iWidth, iHeight, pcYuvPred, iPartIdx, bPrdDepthMap );

    }
    return;
  }

  for ( iPartIdx = 0; iPartIdx < pcCU->getNumPartInter(); iPartIdx++ )
  {
    pcCU->getPartIndexAndSize( iPartIdx, uiPartAddr, iWidth, iHeight );

    if ( eRefPicList != REF_PIC_LIST_X )
    {
      xPredInterUni (pcCU, uiPartAddr, iWidth, iHeight, eRefPicList, pcYuvPred, iPartIdx, bPrdDepthMap );
#ifdef WEIGHT_PRED
      if ( pcCU->getSlice()->getPPS()->getUseWP() )
      {
        xWeightedPredictionUni( pcCU, pcYuvPred, uiPartAddr, iWidth, iHeight, eRefPicList, pcYuvPred, iPartIdx );
      }
#endif
    }
    else
    {
      xPredInterBi  (pcCU, uiPartAddr, iWidth, iHeight, pcYuvPred, iPartIdx, bPrdDepthMap );
    }
  }
  return;
}

#if HIGH_ACCURACY_BI
Void TComPrediction::xPredInterUni ( TComDataCU* pcCU, UInt uiPartAddr, Int iWidth, Int iHeight, RefPicList eRefPicList, TComYuv*& rpcYuvPred, Int iPartIdx, Bool bPrdDepthMap, Bool bi )
#else
Void TComPrediction::xPredInterUni ( TComDataCU* pcCU, UInt uiPartAddr, Int iWidth, Int iHeight, RefPicList eRefPicList, TComYuv*& rpcYuvPred, Int iPartIdx, Bool bPrdDepthMap )
#endif
{
  Int         iRefIdx     = pcCU->getCUMvField( eRefPicList )->getRefIdx( uiPartAddr );           assert (iRefIdx >= 0);
  TComMv      cMv         = pcCU->getCUMvField( eRefPicList )->getMv( uiPartAddr );
  pcCU->clipMv(cMv);

  if( bPrdDepthMap )
  {
#if HIGH_ACCURACY_BI
    UInt uiRShift = ( bi ? 14-g_uiBitDepth-g_uiBitIncrement : 0 );
#else
    UInt uiRShift = 0;
#endif
    xPredInterPrdDepthMap( pcCU, pcCU->getSlice()->getRefPic( eRefPicList, iRefIdx )->getPredDepthMap(), uiPartAddr, &cMv, iWidth, iHeight, rpcYuvPred, uiRShift, PDM_DEPTH_MAP_MCP_FILTER );
    return;
  }

#if MW_DEPTH_MAP_INTERP_FILTER
  if( pcCU->getSlice()->getSPS()->isDepth() )
  {
#if HIGH_ACCURACY_BI
    UInt uiRShift = ( bi ? 14-g_uiBitDepth-g_uiBitIncrement : 0 );
#else
    UInt uiRShift = 0;
#endif
    xPredInterPrdDepthMap( pcCU, pcCU->getSlice()->getRefPic( eRefPicList, iRefIdx )->getPicYuvRec(), uiPartAddr, &cMv, iWidth, iHeight, rpcYuvPred, uiRShift, MW_DEPTH_MAP_INTERP_FILTER );
  }
  else
  {
#endif
#if HIGH_ACCURACY_BI
  if(!bi)
  {
    xPredInterLumaBlk ( pcCU, pcCU->getSlice()->getRefPic( eRefPicList, iRefIdx )->getPicYuvRec()    , uiPartAddr, &cMv, iWidth, iHeight, rpcYuvPred );
  }
  else
  {
    xPredInterLumaBlk_ha  ( pcCU, pcCU->getSlice()->getRefPic( eRefPicList, iRefIdx )->getPicYuvRec()    , uiPartAddr, &cMv, iWidth, iHeight, rpcYuvPred );
  }
#else
  xPredInterLumaBlk       ( pcCU, pcCU->getSlice()->getRefPic( eRefPicList, iRefIdx )->getPicYuvRec(), uiPartAddr, &cMv, iWidth, iHeight, rpcYuvPred );
#endif
#if MW_DEPTH_MAP_INTERP_FILTER
  }
#endif

#if HIGH_ACCURACY_BI
  if (!bi)
  {
    xPredInterChromaBlk     ( pcCU, pcCU->getSlice()->getRefPic( eRefPicList, iRefIdx )->getPicYuvRec(), uiPartAddr, &cMv, iWidth, iHeight, rpcYuvPred );
  }
  else
  {
    xPredInterChromaBlk_ha ( pcCU, pcCU->getSlice()->getRefPic( eRefPicList, iRefIdx )->getPicYuvRec()    , uiPartAddr, &cMv, iWidth, iHeight, rpcYuvPred );
  }
#else
  xPredInterChromaBlk     ( pcCU, pcCU->getSlice()->getRefPic( eRefPicList, iRefIdx )->getPicYuvRec(), uiPartAddr, &cMv, iWidth, iHeight, rpcYuvPred );
#endif
}

Void TComPrediction::xPredInterBi ( TComDataCU* pcCU, UInt uiPartAddr, Int iWidth, Int iHeight, TComYuv*& rpcYuvPred, Int iPartIdx, Bool bPrdDepthMap )
{
  TComYuv* pcMbYuv;
  Int      iRefIdx[2] = {-1, -1};

  for ( Int iRefList = 0; iRefList < 2; iRefList++ )
  {
    RefPicList eRefPicList = (iRefList ? REF_PIC_LIST_1 : REF_PIC_LIST_0);
    iRefIdx[iRefList] = pcCU->getCUMvField( eRefPicList )->getRefIdx( uiPartAddr );

    if ( iRefIdx[iRefList] < 0 )
    {
      continue;
    }

    assert( iRefIdx[iRefList] < pcCU->getSlice()->getNumRefIdx(eRefPicList) );

    pcMbYuv = &m_acYuvPred[iRefList];
#if HIGH_ACCURACY_BI
    if( pcCU->getCUMvField( REF_PIC_LIST_0 )->getRefIdx( uiPartAddr ) >= 0 && pcCU->getCUMvField( REF_PIC_LIST_1 )->getRefIdx( uiPartAddr ) >= 0 )
      xPredInterUni ( pcCU, uiPartAddr, iWidth, iHeight, eRefPicList, pcMbYuv, iPartIdx, bPrdDepthMap, true );
    else
      xPredInterUni ( pcCU, uiPartAddr, iWidth, iHeight, eRefPicList, pcMbYuv, iPartIdx, bPrdDepthMap );
#else
    xPredInterUni ( pcCU, uiPartAddr, iWidth, iHeight, eRefPicList, pcMbYuv, iPartIdx, bPrdDepthMap );
#endif
  }

#ifdef WEIGHT_PRED
  if ( pcCU->getSlice()->getPPS()->getWPBiPredIdc() )
  {
    xWeightedPredictionBi( pcCU, &m_acYuvPred[0], &m_acYuvPred[1], iRefIdx[0], iRefIdx[1], uiPartAddr, iWidth, iHeight, rpcYuvPred );
  }
  else
#endif
  xWeightedAverage( pcCU, &m_acYuvPred[0], &m_acYuvPred[1], iRefIdx[0], iRefIdx[1], uiPartAddr, iWidth, iHeight, rpcYuvPred );
}


Void 
TComPrediction::xPredInterPrdDepthMap( TComDataCU* pcCU, TComPicYuv* pcPicYuvRef, UInt uiPartAddr, TComMv* pcMv, Int iWidth, Int iHeight, TComYuv*& rpcYuv, UInt uiRShift, UInt uiFilterMode ) // 0:std, 1:bilin, 2:nearest neighbour
{
  AOF( uiFilterMode <= 2 );

  Int     iFPelMask   = ~3;
  Int     iRefStride  = pcPicYuvRef->getStride();
  Int     iDstStride  = rpcYuv->getStride();
  Int     iHor        = ( uiFilterMode == 2 ? ( pcMv->getHor() + 2 ) & iFPelMask : pcMv->getHor() );
  Int     iVer        = ( uiFilterMode == 2 ? ( pcMv->getVer() + 2 ) & iFPelMask : pcMv->getVer() );
#if MW_DEPTH_MAP_INTERP_FILTER == 2 && MW_FULL_PEL_DEPTH_MAP_MV_SIGNALLING
  if( pcCU->getSlice()->getSPS()->isDepth() )
  {
    assert( uiFilterMode == 2 );
    iHor = pcMv->getHor() * 4;
    iVer = pcMv->getVer() * 4;
  }
#endif
  Int     iRefOffset  = ( iHor >> 2 ) + ( iVer >> 2 ) * iRefStride;
  Int     ixFrac      = iHor & 0x3;
  Int     iyFrac      = iVer & 0x3;
  Pel*    piRefY      = pcPicYuvRef->getLumaAddr( pcCU->getAddr(), pcCU->getZorderIdxInCU() + uiPartAddr ) + iRefOffset;
  Pel*    piDstY      = rpcYuv->getLumaAddr( uiPartAddr );

  //  Integer position
  if( ixFrac == 0 && iyFrac == 0 )
  {
    for( Int y = 0; y < iHeight; y++, piDstY += iDstStride, piRefY += iRefStride )
    {
      for( Int x = 0; x < iWidth; x++ )
      {
        piDstY[ x ] = piRefY[ x ] << uiRShift;
      }
    }
    return;
  }

  // bi-linear interpolation
  if( uiFilterMode == 1 )
  {
    Int   iW00    = ( 4 - ixFrac ) * ( 4 - iyFrac );
    Int   iW01    = (     ixFrac ) * ( 4 - iyFrac );
    Int   iW10    = ( 4 - ixFrac ) * (     iyFrac );
    Int   iW11    = (     ixFrac ) * (     iyFrac );
    Pel*  piRefY1 = piRefY + iRefStride;
    for( Int y = 0; y < iHeight; y++, piDstY += iDstStride, piRefY += iRefStride, piRefY1 += iRefStride )
    {
      for( Int x = 0; x < iWidth; x++ )
      {
        Int iSV     = iW00 * piRefY [ x ] + iW01 * piRefY [ x + 1 ]
                    + iW10 * piRefY1[ x ] + iW11 * piRefY1[ x + 1 ];
        iSV       <<= uiRShift;
        piDstY[ x ] = ( iSV + 8 ) >> 4;
      }
    }
    return;
  }

  xPredInterLumaBlk( pcCU, pcPicYuvRef, uiPartAddr, pcMv, iWidth, iHeight, rpcYuv );
  return;
}



#if HIGH_ACCURACY_BI

Void  TComPrediction::xPredInterLumaBlk_ha( TComDataCU* pcCU, TComPicYuv* pcPicYuvRef, UInt uiPartAddr, TComMv* pcMv, Int iWidth, Int iHeight, TComYuv*& rpcYuv )
{
  Int     iRefStride = pcPicYuvRef->getStride();
  Int     iDstStride = rpcYuv->getStride();

  Int     iRefOffset = ( pcMv->getHor() >> 2 ) + ( pcMv->getVer() >> 2 ) * iRefStride;
  Pel*    piRefY     = pcPicYuvRef->getLumaAddr( pcCU->getAddr(), pcCU->getZorderIdxInCU() + uiPartAddr ) + iRefOffset;

  Int     ixFrac  = pcMv->getHor() & 0x3;
  Int     iyFrac  = pcMv->getVer() & 0x3;

  Pel* piDstY = rpcYuv->getLumaAddr( uiPartAddr );
    UInt shiftNum = 14-g_uiBitDepth-g_uiBitIncrement;
  //  Integer point
  if ( ixFrac == 0 && iyFrac == 0 )
  {
    for ( Int y = 0; y < iHeight; y++ )
    {
      for(Int x=0; x<iWidth; x++)
        piDstY[x] = piRefY[x]<<shiftNum;
      piDstY += iDstStride;
      piRefY += iRefStride;
    }
    return;
  }

  //  Half-pel horizontal
  if ( ixFrac == 2 && iyFrac == 0 )
  {
    xCTI_FilterHalfHor_ha ( piRefY, iRefStride, 1, iWidth, iHeight, iDstStride, 1, piDstY );
    return;
  }

  //  Half-pel vertical
  if ( ixFrac == 0 && iyFrac == 2 )
  {
    xCTI_FilterHalfVer_ha ( piRefY, iRefStride, 1, iWidth, iHeight, iDstStride, 1, piDstY );
    return;
  }

  Int   iExtStride = m_iYuvExtStride;//m_cYuvExt.getStride();
  Int*  piExtY     = m_piYuvExt;//m_cYuvExt.getLumaAddr();

  //  Half-pel center
  if ( ixFrac == 2 && iyFrac == 2 )
  {
    xCTI_FilterHalfVer (piRefY - 3,  iRefStride, 1, iWidth +7, iHeight, iExtStride, 1, piExtY );
    xCTI_FilterHalfHor_ha (piExtY + 3,  iExtStride, 1, iWidth    , iHeight, iDstStride, 1, piDstY );
    return;
  }

  //  Quater-pel horizontal
  if ( iyFrac == 0)
  {
    if ( ixFrac == 1)
    {
      xCTI_FilterQuarter0Hor_ha( piRefY, iRefStride, 1, iWidth, iHeight, iDstStride, 1, piDstY );
      return;
    }
    if ( ixFrac == 3)
    {
      xCTI_FilterQuarter1Hor_ha( piRefY, iRefStride, 1, iWidth, iHeight, iDstStride, 1, piDstY );
      return;
    }
  }
  if ( iyFrac == 2 )
  {
    if ( ixFrac == 1)
    {
      xCTI_FilterHalfVer (piRefY -3,  iRefStride, 1, iWidth +7, iHeight, iExtStride, 1, piExtY );
      xCTI_FilterQuarter0Hor_ha (piExtY + 3,  iExtStride, 1, iWidth, iHeight, iDstStride, 1, piDstY );
      return;
    }
    if ( ixFrac == 3)
    {
      xCTI_FilterHalfVer (piRefY - 3,  iRefStride, 1, iWidth + 7, iHeight, iExtStride, 1, piExtY );
      xCTI_FilterQuarter1Hor_ha (piExtY + 3,  iExtStride, 1, iWidth, iHeight, iDstStride, 1, piDstY );
      return;
    }
  }

  //  Quater-pel vertical
  if( ixFrac == 0 )
  {
    if( iyFrac == 1 )
    {
      xCTI_FilterQuarter0Ver_ha( piRefY, iRefStride, 1, iWidth, iHeight, iDstStride, 1, piDstY );
      return;
    }
    if( iyFrac == 3 )
    {
      xCTI_FilterQuarter1Ver_ha( piRefY, iRefStride, 1, iWidth, iHeight, iDstStride, 1, piDstY );
      return;
    }
  }

  if( ixFrac == 2 )
  {
    if( iyFrac == 1 )
    {
      xCTI_FilterQuarter0Ver (piRefY - 3,  iRefStride, 1, iWidth + 7, iHeight, iExtStride, 1, piExtY );
      xCTI_FilterHalfHor_ha (piExtY + 3,  iExtStride, 1, iWidth    , iHeight, iDstStride, 1, piDstY );

      return;
    }
    if( iyFrac == 3 )
    {
      xCTI_FilterQuarter1Ver (piRefY -3,  iRefStride, 1, iWidth + 7, iHeight, iExtStride, 1, piExtY );
      xCTI_FilterHalfHor_ha (piExtY + 3,  iExtStride, 1, iWidth    , iHeight, iDstStride, 1, piDstY );
      return;
    }
  }

  /// Quarter-pel center
  if ( iyFrac == 1)
  {
    if ( ixFrac == 1)
    {
      xCTI_FilterQuarter0Ver (piRefY - 3,  iRefStride, 1, iWidth + 7, iHeight, iExtStride, 1, piExtY );
      xCTI_FilterQuarter0Hor_ha (piExtY + 3,  iExtStride, 1, iWidth    , iHeight, iDstStride, 1, piDstY );
      return;
    }
    if ( ixFrac == 3)
    {
      xCTI_FilterQuarter0Ver (piRefY - 3,  iRefStride, 1, iWidth +7, iHeight, iExtStride, 1, piExtY );
      xCTI_FilterQuarter1Hor_ha (piExtY + 3,  iExtStride, 1, iWidth    , iHeight, iDstStride, 1, piDstY );

      return;
    }
  }
  if ( iyFrac == 3 )
  {
    if ( ixFrac == 1)
    {
      xCTI_FilterQuarter1Ver (piRefY - 3,  iRefStride, 1, iWidth + 7, iHeight, iExtStride, 1, piExtY );
      xCTI_FilterQuarter0Hor_ha (piExtY + 3,  iExtStride, 1, iWidth    , iHeight, iDstStride, 1, piDstY );
      return;
    }
    if ( ixFrac == 3)
    {
      xCTI_FilterQuarter1Ver (piRefY - 3,  iRefStride, 1, iWidth + 7, iHeight, iExtStride, 1, piExtY );
      xCTI_FilterQuarter1Hor_ha (piExtY + 3,  iExtStride, 1, iWidth    , iHeight, iDstStride, 1, piDstY );
      return;
    }
  }
}

#endif

Void  TComPrediction::xPredInterLumaBlk( TComDataCU* pcCU, TComPicYuv* pcPicYuvRef, UInt uiPartAddr, TComMv* pcMv, Int iWidth, Int iHeight, TComYuv*& rpcYuv )
{
  Int     iRefStride = pcPicYuvRef->getStride();
  Int     iDstStride = rpcYuv->getStride();

  Int     iRefOffset = ( pcMv->getHor() >> 2 ) + ( pcMv->getVer() >> 2 ) * iRefStride;
  Pel*    piRefY     = pcPicYuvRef->getLumaAddr( pcCU->getAddr(), pcCU->getZorderIdxInCU() + uiPartAddr ) + iRefOffset;

  Int     ixFrac  = pcMv->getHor() & 0x3;
  Int     iyFrac  = pcMv->getVer() & 0x3;

  Pel* piDstY = rpcYuv->getLumaAddr( uiPartAddr );

  //  Integer point
  if ( ixFrac == 0 && iyFrac == 0 )
  {
    for ( Int y = 0; y < iHeight; y++ )
    {
      ::memcpy(piDstY, piRefY, sizeof(Pel)*iWidth);
      piDstY += iDstStride;
      piRefY += iRefStride;
    }
    return;
  }

  //  Half-pel horizontal
  if ( ixFrac == 2 && iyFrac == 0 )
  {
    xCTI_FilterHalfHor ( piRefY, iRefStride, 1, iWidth, iHeight, iDstStride, 1, piDstY );
    return;
  }

  //  Half-pel vertical
  if ( ixFrac == 0 && iyFrac == 2 )
  {
    xCTI_FilterHalfVer ( piRefY, iRefStride, 1, iWidth, iHeight, iDstStride, 1, piDstY );
    return;
  }

  Int   iExtStride = m_iYuvExtStride;//m_cYuvExt.getStride();
  Int*  piExtY     = m_piYuvExt;//m_cYuvExt.getLumaAddr();

  //  Half-pel center
  if ( ixFrac == 2 && iyFrac == 2 )
  {

    xCTI_FilterHalfVer (piRefY - 3,  iRefStride, 1, iWidth +7, iHeight, iExtStride, 1, piExtY );
    xCTI_FilterHalfHor (piExtY + 3,  iExtStride, 1, iWidth    , iHeight, iDstStride, 1, piDstY );
    return;
  }

  //  Quater-pel horizontal
  if ( iyFrac == 0)
  {
    if ( ixFrac == 1)
    {
      xCTI_FilterQuarter0Hor( piRefY, iRefStride, 1, iWidth, iHeight, iDstStride, 1, piDstY );
      return;
    }
    if ( ixFrac == 3)
    {
      xCTI_FilterQuarter1Hor( piRefY, iRefStride, 1, iWidth, iHeight, iDstStride, 1, piDstY );
      return;
    }
  }
  if ( iyFrac == 2 )
  {
    if ( ixFrac == 1)
    {
      xCTI_FilterHalfVer (piRefY -3,  iRefStride, 1, iWidth +7, iHeight, iExtStride, 1, piExtY );
      xCTI_FilterQuarter0Hor (piExtY + 3,  iExtStride, 1, iWidth, iHeight, iDstStride, 1, piDstY );
      return;
    }
    if ( ixFrac == 3)
    {
      xCTI_FilterHalfVer (piRefY - 3,  iRefStride, 1, iWidth + 7, iHeight, iExtStride, 1, piExtY );
      xCTI_FilterQuarter1Hor (piExtY + 3,  iExtStride, 1, iWidth, iHeight, iDstStride, 1, piDstY );
      return;
    }
  }

  //  Quater-pel vertical
  if( ixFrac == 0 )
  {
    if( iyFrac == 1 )
    {
      xCTI_FilterQuarter0Ver( piRefY, iRefStride, 1, iWidth, iHeight, iDstStride, 1, piDstY );
      return;
    }
    if( iyFrac == 3 )
    {
      xCTI_FilterQuarter1Ver( piRefY, iRefStride, 1, iWidth, iHeight, iDstStride, 1, piDstY );
      return;
    }
  }

  if( ixFrac == 2 )
  {
    if( iyFrac == 1 )
    {
      xCTI_FilterQuarter0Ver (piRefY - 3,  iRefStride, 1, iWidth + 7, iHeight, iExtStride, 1, piExtY );
      xCTI_FilterHalfHor (piExtY + 3,  iExtStride, 1, iWidth    , iHeight, iDstStride, 1, piDstY );
      return;
    }
    if( iyFrac == 3 )
    {
      xCTI_FilterQuarter1Ver (piRefY -3,  iRefStride, 1, iWidth + 7, iHeight, iExtStride, 1, piExtY );
      xCTI_FilterHalfHor (piExtY + 3,  iExtStride, 1, iWidth    , iHeight, iDstStride, 1, piDstY );
      return;
    }
  }

  /// Quarter-pel center
  if ( iyFrac == 1)
  {
    if ( ixFrac == 1)
    {
      xCTI_FilterQuarter0Ver (piRefY - 3,  iRefStride, 1, iWidth + 7, iHeight, iExtStride, 1, piExtY );
      xCTI_FilterQuarter0Hor (piExtY + 3,  iExtStride, 1, iWidth    , iHeight, iDstStride, 1, piDstY );
      return;
    }
    if ( ixFrac == 3)
    {
      xCTI_FilterQuarter0Ver (piRefY - 3,  iRefStride, 1, iWidth +7, iHeight, iExtStride, 1, piExtY );
      xCTI_FilterQuarter1Hor (piExtY + 3,  iExtStride, 1, iWidth    , iHeight, iDstStride, 1, piDstY );
      return;
    }
  }
  if ( iyFrac == 3 )
  {
    if ( ixFrac == 1)
    {
      xCTI_FilterQuarter1Ver (piRefY - 3,  iRefStride, 1, iWidth + 7, iHeight, iExtStride, 1, piExtY );
      xCTI_FilterQuarter0Hor (piExtY + 3,  iExtStride, 1, iWidth    , iHeight, iDstStride, 1, piDstY );
      return;
    }
    if ( ixFrac == 3)
    {
      xCTI_FilterQuarter1Ver (piRefY - 3,  iRefStride, 1, iWidth + 7, iHeight, iExtStride, 1, piExtY );
      xCTI_FilterQuarter1Hor (piExtY + 3,  iExtStride, 1, iWidth    , iHeight, iDstStride, 1, piDstY );
      return;
    }
  }
}

#if HIGH_ACCURACY_BI
Void TComPrediction::xPredInterChromaBlk_ha( TComDataCU* pcCU, TComPicYuv* pcPicYuvRef, UInt uiPartAddr, TComMv* pcMv, Int iWidth, Int iHeight, TComYuv*& rpcYuv )
{
  Int     iRefStride  = pcPicYuvRef->getCStride();
  Int     iDstStride  = rpcYuv->getCStride();

  Int     iRefOffset  = (pcMv->getHor() >> 3) + (pcMv->getVer() >> 3) * iRefStride;

  Pel*    piRefCb     = pcPicYuvRef->getCbAddr( pcCU->getAddr(), pcCU->getZorderIdxInCU() + uiPartAddr ) + iRefOffset;
  Pel*    piRefCr     = pcPicYuvRef->getCrAddr( pcCU->getAddr(), pcCU->getZorderIdxInCU() + uiPartAddr ) + iRefOffset;

  Pel* piDstCb = rpcYuv->getCbAddr( uiPartAddr );
  Pel* piDstCr = rpcYuv->getCrAddr( uiPartAddr );

  Int     ixFrac  = pcMv->getHor() & 0x7;
  Int     iyFrac  = pcMv->getVer() & 0x7;
  UInt    uiCWidth  = iWidth  >> 1;
  UInt    uiCHeight = iHeight >> 1;

  xDCTIF_FilterC_ha(piRefCb, iRefStride,piDstCb,iDstStride,uiCWidth,uiCHeight, iyFrac, ixFrac);
  xDCTIF_FilterC_ha(piRefCr, iRefStride,piDstCr,iDstStride,uiCWidth,uiCHeight, iyFrac, ixFrac);
  return;
}
#endif

//--
Void TComPrediction::xPredInterChromaBlk( TComDataCU* pcCU, TComPicYuv* pcPicYuvRef, UInt uiPartAddr, TComMv* pcMv, Int iWidth, Int iHeight, TComYuv*& rpcYuv )
{
  Int     iRefStride  = pcPicYuvRef->getCStride();
  Int     iDstStride  = rpcYuv->getCStride();

  Int     iRefOffset  = (pcMv->getHor() >> 3) + (pcMv->getVer() >> 3) * iRefStride;

  Pel*    piRefCb     = pcPicYuvRef->getCbAddr( pcCU->getAddr(), pcCU->getZorderIdxInCU() + uiPartAddr ) + iRefOffset;
  Pel*    piRefCr     = pcPicYuvRef->getCrAddr( pcCU->getAddr(), pcCU->getZorderIdxInCU() + uiPartAddr ) + iRefOffset;

  Pel* piDstCb = rpcYuv->getCbAddr( uiPartAddr );
  Pel* piDstCr = rpcYuv->getCrAddr( uiPartAddr );

  Int     ixFrac  = pcMv->getHor() & 0x7;
  Int     iyFrac  = pcMv->getVer() & 0x7;
  UInt    uiCWidth  = iWidth  >> 1;
  UInt    uiCHeight = iHeight >> 1;

  xDCTIF_FilterC(piRefCb, iRefStride,piDstCb,iDstStride,uiCWidth,uiCHeight, iyFrac, ixFrac);
  xDCTIF_FilterC(piRefCr, iRefStride,piDstCr,iDstStride,uiCWidth,uiCHeight, iyFrac, ixFrac);
  return;
}

Void  TComPrediction::xDCTIF_FilterC ( Pel*  piRefC, Int iRefStride,Pel*  piDstC,Int iDstStride,
                                       Int iWidth, Int iHeight,Int iMVyFrac,Int iMVxFrac)
{
  // Integer point
  if ( iMVxFrac == 0 && iMVyFrac == 0 )
  {
    for ( Int y = 0; y < iHeight; y++ )
    {
      ::memcpy(piDstC, piRefC, sizeof(Pel)*iWidth);
      piDstC += iDstStride;
      piRefC += iRefStride;
    }
    return;
  }

  if ( iMVyFrac == 0 )
  {
    xCTI_Filter1DHorC (piRefC, iRefStride,  iWidth, iHeight, iDstStride,  piDstC, iMVxFrac );
    return;
  }

  if ( iMVxFrac == 0 )
  {
    xCTI_Filter1DVerC (piRefC, iRefStride,  iWidth, iHeight, iDstStride,  piDstC, iMVyFrac );
    return;
}

  Int   iExtStride = m_iYuvExtStride;
  Int*  piExtC     = m_piYuvExt;

  xCTI_Filter2DVerC (piRefC - 1,  iRefStride,  iWidth + 3, iHeight, iExtStride,  piExtC, iMVyFrac );
  xCTI_Filter2DHorC (piExtC + 1,  iExtStride,  iWidth             , iHeight, iDstStride,  piDstC, iMVxFrac );
}

#if HIGH_ACCURACY_BI

Void  TComPrediction::xDCTIF_FilterC_ha ( Pel*  piRefC, Int iRefStride,Pel*  piDstC,Int iDstStride,
                                       Int iWidth, Int iHeight,Int iMVyFrac,Int iMVxFrac)
{
  UInt    shiftNumOrg = 6 - g_uiBitIncrement + 8 - g_uiBitDepth;
  // Integer point
  if ( iMVxFrac == 0 && iMVyFrac == 0 )
  {
    for (Int y = 0; y < iHeight; y++ )
    {
      for(Int x=0; x<iWidth; x++)
      {
        piDstC[x] = (piRefC[x]<<shiftNumOrg);
      }
      piDstC += iDstStride;
      piRefC += iRefStride;
    }
    return;
  }

  if ( iMVyFrac == 0 )
  {
    xCTI_Filter1DHorC_ha (piRefC, iRefStride,  iWidth, iHeight, iDstStride,  piDstC, iMVxFrac );
    return;

  }

  if ( iMVxFrac == 0 )
  {
    xCTI_Filter1DVerC_ha (piRefC, iRefStride,  iWidth, iHeight, iDstStride,  piDstC, iMVyFrac );
    return;
  }

  Int   iExtStride = m_iYuvExtStride;
  Int*  piExtC     = m_piYuvExt;

  xCTI_Filter2DVerC (piRefC - 1,  iRefStride,  iWidth + 3, iHeight, iExtStride,  piExtC, iMVyFrac );
  xCTI_Filter2DHorC_ha (piExtC + 1,  iExtStride,  iWidth , iHeight, iDstStride,  piDstC, iMVxFrac );
  return;

}

#endif


Void TComPrediction::xWeightedAverage( TComDataCU* pcCU, TComYuv* pcYuvSrc0, TComYuv* pcYuvSrc1, Int iRefIdx0, Int iRefIdx1, UInt uiPartIdx, Int iWidth, Int iHeight, TComYuv*& rpcYuvDst )
{
  if( iRefIdx0 >= 0 && iRefIdx1 >= 0 )
  {
#ifdef ROUNDING_CONTROL_BIPRED
    rpcYuvDst->addAvg( pcYuvSrc0, pcYuvSrc1, uiPartIdx, iWidth, iHeight, pcCU->getSlice()->isRounding());
#else
    rpcYuvDst->addAvg( pcYuvSrc0, pcYuvSrc1, uiPartIdx, iWidth, iHeight );
#endif
  }
  else if ( iRefIdx0 >= 0 && iRefIdx1 <  0 )
  {
    pcYuvSrc0->copyPartToPartYuv( rpcYuvDst, uiPartIdx, iWidth, iHeight );
  }
  else if ( iRefIdx0 <  0 && iRefIdx1 >= 0 )
  {
    pcYuvSrc1->copyPartToPartYuv( rpcYuvDst, uiPartIdx, iWidth, iHeight );
  }
  else
  {
    assert (0);
  }
}

// AMVP
Void TComPrediction::getMvPredAMVP( TComDataCU* pcCU, UInt uiPartIdx, UInt uiPartAddr, RefPicList eRefPicList, Int iRefIdx, TComMv& rcMvPred )
{
  AMVPInfo* pcAMVPInfo = pcCU->getCUMvField(eRefPicList)->getAMVPInfo();

  if( pcCU->getAMVPMode(uiPartAddr) == AM_NONE || (pcAMVPInfo->iN <= 1 && pcCU->getAMVPMode(uiPartAddr) == AM_EXPL) )
  {
    rcMvPred = pcAMVPInfo->m_acMvCand[0];

    pcCU->setMVPIdxSubParts( 0, eRefPicList, uiPartAddr, uiPartIdx, pcCU->getDepth(uiPartAddr));
    pcCU->setMVPNumSubParts( pcAMVPInfo->iN, eRefPicList, uiPartAddr, uiPartIdx, pcCU->getDepth(uiPartAddr));
    return;
  }

  assert(pcCU->getMVPIdx(eRefPicList,uiPartAddr) >= 0);
  rcMvPred = pcAMVPInfo->m_acMvCand[pcCU->getMVPIdx(eRefPicList,uiPartAddr)];
  return;
}

#if ADD_PLANAR_MODE
/** Function for deriving planar intra prediction.
 * \param pSrc pointer to reconstructed sample array
 * \param srcStride the stride of the reconstructed sample array
 * \param rpDst reference to pointer for the prediction sample array
 * \param dstStride the stride of the prediction sample array
 * \param width the width of the block
 * \param height the height of the block
 * \param blkAboveAvailable boolean indication if the block above is available
 * \param blkLeftAvailable boolean indication if the block to the left is available
 *
 * This function derives the prediction samples for planar mode (intra coding).
 */
#if REFERENCE_SAMPLE_PADDING
Void TComPrediction::xPredIntraPlanar( Int* pSrc, Int srcStride, Pel*& rpDst, Int dstStride, UInt width, UInt height )
#else
Void TComPrediction::xPredIntraPlanar( Int* pSrc, Int srcStride, Pel*& rpDst, Int dstStride, UInt width, UInt height, Bool blkAboveAvailable, Bool blkLeftAvailable )
#endif
{
  assert(width == height);

  Int k, l, bottomLeft, topRight;
  Int horPred;
  Int leftColumn[MAX_CU_SIZE], topRow[MAX_CU_SIZE], bottomRow[MAX_CU_SIZE], rightColumn[MAX_CU_SIZE];
  UInt blkSize = width;
  UInt offset2D = width;
  UInt shift1D = g_aucConvertToBit[ width ] + 2;
  UInt shift2D = shift1D + 1;

  // Get left and above reference column and row
#if REFERENCE_SAMPLE_PADDING
  for(k=0;k<blkSize;k++)
  {
    topRow[k] = pSrc[k-srcStride];
    leftColumn[k] = pSrc[k*srcStride-1];
  }
#else
  if (!blkAboveAvailable && !blkLeftAvailable)
  {
    for(k=0;k<blkSize;k++)
    {
      leftColumn[k] = topRow[k] = ( 1 << ( g_uiBitDepth + g_uiBitIncrement - 1 ) );
    }
  }
  else
  {
    if(blkAboveAvailable)
    {
      for(k=0;k<blkSize;k++)
      {
        topRow[k] = pSrc[k-srcStride];
      }
    }
    else
    {
      Int leftSample = pSrc[-1];
      for(k=0;k<blkSize;k++)
      {
        topRow[k] = leftSample;
      }
    }
    if(blkLeftAvailable)
    {
      for(k=0;k<blkSize;k++)
      {
        leftColumn[k] = pSrc[k*srcStride-1];
      }
    }
    else
    {
      Int aboveSample = pSrc[-srcStride];
      for(k=0;k<blkSize;k++)
      {
        leftColumn[k] = aboveSample;
      }
    }
  }
#endif

  // Prepare intermediate variables used in interpolation
  bottomLeft = leftColumn[blkSize-1];
  topRight   = topRow[blkSize-1];
  for (k=0;k<blkSize;k++)
  {
    bottomRow[k]   = bottomLeft - topRow[k];
    rightColumn[k] = topRight   - leftColumn[k];
    topRow[k]      <<= shift1D;
    leftColumn[k]  <<= shift1D;
  }

  // Generate prediction signal
  for (k=0;k<blkSize;k++)
  {
    horPred = leftColumn[k] + offset2D;
    for (l=0;l<blkSize;l++)
    {
      horPred += rightColumn[k];
      topRow[l] += bottomRow[l];
      rpDst[k*dstStride+l] = ( (horPred + topRow[l]) >> shift2D );
    }
  }
}
#endif

#if LM_CHROMA
/** Function for deriving chroma LM intra prediction.
 * \param pcPattern pointer to neighbouring pixel access pattern
 * \param pSrc pointer to reconstructed chroma sample array
 * \param pPred pointer for the prediction sample array
 * \param uiPredStride the stride of the prediction sample array
 * \param uiCWidth the width of the chroma block
 * \param uiCHeight the height of the chroma block
 * \param uiChromaId boolean indication of chroma component

 \ This function derives the prediction samples for chroma LM mode (chroma intra coding)
 */
Void TComPrediction::predLMIntraChroma( TComPattern* pcPattern, Int* piSrc, Pel* pPred, UInt uiPredStride, UInt uiCWidth, UInt uiCHeight, UInt uiChromaId )
{
  UInt uiWidth  = uiCWidth << 1;
  UInt uiHeight = uiCHeight << 1;

  if (uiChromaId == 0)
    xGetRecPixels( pcPattern, pcPattern->getROIY(), pcPattern->getPatternLStride(), m_pLumaRecBuffer + m_iLumaRecStride + 1, m_iLumaRecStride, uiWidth, uiHeight );

  xGetLLSPrediction( pcPattern, piSrc+uiWidth+2, uiWidth+1, pPred, uiPredStride, uiCWidth, uiCHeight, 1 );  
}

/** Function for deriving downsampled luma sample of current chroma block and its above, left causal pixel
 * \param pcPattern pointer to neighbouring pixel access pattern
 * \param pRecSrc pointer to reconstructed luma sample array
 * \param iRecSrcStride the stride of reconstructed luma sample array
 * \param pDst0 pointer to downsampled luma sample array
 * \param iDstStride the stride of downsampled luma sample array
 * \param uiWidth0 the width of the luma block
 * \param uiHeight0 the height of the luma block

 \ This function derives downsampled luma sample of current chroma block and its above, left causal pixel
 */

Void TComPrediction::xGetRecPixels( TComPattern* pcPattern, Pel* pRecSrc, Int iRecSrcStride, Pel* pDst0, Int iDstStride, UInt uiWidth0, UInt uiHeight0 )
{
  Pel* pSrc = pRecSrc;
  Pel* pDst = pDst0;

  Int uiCWidth = uiWidth0/2;
  Int uiCHeight = uiHeight0/2;

  if( pcPattern->isLeftAvailable() )
  {
    pSrc = pSrc - 2;
    pDst = pDst - 1;

    uiCWidth += 1;
  }

  if( pcPattern->isAboveAvailable() )
  {
    pSrc = pSrc - 2*iRecSrcStride;
    pDst = pDst - iDstStride;

    uiCHeight += 1;
  }

  for( Int j = 0; j < uiCHeight; j++ )
    {
      for( Int i = 0, ii = i << 1; i < uiCWidth; i++, ii = i << 1 )
        pDst[i] = (pSrc[ii] + pSrc[ii + iRecSrcStride]) >> 1;

      pDst += iDstStride;
      pSrc += iRecSrcStride*2;
    }  
}

/** Function for deriving the positon of first non-zero binary bit of a value
 * \param x input value
 \ This function derives the positon of first non-zero binary bit of a value
 */
Int GetMSB( UInt x )
{
#if 1
  Int iMSB = 0, bits = ( sizeof( Int ) << 3 ), y = 1;

  while( x > 1 )
  {
    bits >>= 1;
    y = x >> bits;

    if( y )
    {
      x = y;
      iMSB += bits;
    }
  }

  iMSB+=y;

#else

  Int iMSB = 0;
  while( x > 0 )
  {
    x >>= 1;
    iMSB++;
  }
#endif

  return iMSB;
}

/** Function for deriving LM intra prediction.
 * \param pcPattern pointer to neighbouring pixel access pattern
 * \param pSrc0 pointer to reconstructed chroma sample array
 * \param iSrcStride the stride of reconstructed chroma sample array
 * \param pDst0 reference to pointer for the prediction sample array
 * \param iDstStride the stride of the prediction sample array
 * \param uiWidth the width of the chroma block
 * \param uiHeight the height of the chroma block
 * \param uiExt0 line number of neiggboirng pixels for calculating LM model parameter, default value is 1

 \ This function derives the prediction samples for chroma LM mode (chroma intra coding)
 */
Void TComPrediction::xGetLLSPrediction( TComPattern* pcPattern, Int* pSrc0, Int iSrcStride, Pel* pDst0, Int iDstStride, UInt uiWidth, UInt uiHeight, UInt uiExt0 )
{

  Pel  *pDst, *pLuma;
  Int  *pSrc;

  Int  iLumaStride = m_iLumaRecStride;
  Pel* pLuma0 = m_pLumaRecBuffer + uiExt0 * iLumaStride + uiExt0;

  Int i, j, iCountShift = 0;

  UInt uiExt = uiExt0;

  // LLS parameters estimation -->

  Int x = 0, y = 0, xx = 0, xy = 0;

  if( pcPattern->isAboveAvailable() )
  {
    pSrc  = pSrc0  - iSrcStride;
    pLuma = pLuma0 - iLumaStride;

    for( j = 0; j < uiWidth; j++ )
    {
      x += pLuma[j];
      y += pSrc[j];
      xx += pLuma[j] * pLuma[j];
      xy += pLuma[j] * pSrc[j];
    }
    iCountShift += g_aucConvertToBit[ uiWidth ] + 2;
  }

  if( pcPattern->isLeftAvailable() )
  {
    pSrc  = pSrc0 - uiExt;
    pLuma = pLuma0 - uiExt;

    for( i = 0; i < uiHeight; i++ )
    {
      x += pLuma[0];
      y += pSrc[0];
      xx += pLuma[0] * pLuma[0];
      xy += pLuma[0] * pSrc[0];

      pSrc  += iSrcStride;
      pLuma += iLumaStride;
    }
    iCountShift += iCountShift > 0 ? 1 : ( g_aucConvertToBit[ uiWidth ] + 2 );
  }

  Int iBitdepth = ( ( g_uiBitDepth + g_uiBitIncrement ) + g_aucConvertToBit[ uiWidth ] + 3 ) * 2;
  Int iTempShift = Max( ( iBitdepth - 31 + 1) / 2, 0);

  if(iTempShift > 0)
  {
    x  = ( x +  ( 1 << ( iTempShift - 1 ) ) ) >> iTempShift;
    y  = ( y +  ( 1 << ( iTempShift - 1 ) ) ) >> iTempShift;
    xx = ( xx + ( 1 << ( iTempShift - 1 ) ) ) >> iTempShift;
    xy = ( xy + ( 1 << ( iTempShift - 1 ) ) ) >> iTempShift;
    iCountShift -= iTempShift;
  }

  Int a, b, iShift = 13;

  if( iCountShift == 0 )
  {
    a = 0;
    b = 128 << g_uiBitIncrement;
    iShift = 0;
  }
  else
  {
    Int a1 = ( xy << iCountShift ) - y * x;
    Int a2 = ( xx << iCountShift ) - x * x;              

    if( a2 == 0 || a1 == 0 )
    {
      a = 0;
      b = ( y + ( 1 << ( iCountShift - 1 ) ) )>> iCountShift;
      iShift = 0;
    }
    else
    {
      const Int iShiftA2 = 6;
      const Int iShiftA1 = 15;
      const Int iAccuracyShift = 15;

      Int iScaleShiftA2 = 0;
      Int iScaleShiftA1 = 0;
      Int a1s = a1;
      Int a2s = a2;

      iScaleShiftA1 = GetMSB( abs( a1 ) ) - iShiftA1;
      iScaleShiftA2 = GetMSB( abs( a2 ) ) - iShiftA2;  

      if( iScaleShiftA1 < 0 )
        iScaleShiftA1 = 0;

      if( iScaleShiftA2 < 0 )
        iScaleShiftA2 = 0;

      Int iScaleShiftA = iScaleShiftA2 + iAccuracyShift - iShift - iScaleShiftA1;

      a2s = a2 >> iScaleShiftA2;

      a1s = a1 >> iScaleShiftA1;

      a = a1s * m_uiaShift[ abs( a2s ) ];
      
      if( iScaleShiftA < 0 )
        a = a << -iScaleShiftA;
      else
        a = a >> iScaleShiftA;

      if( a > ( 1 << 15 ) - 1 )
        a = ( 1 << 15 ) - 1;
      else if( a < -( 1 << 15 ) )
        a = -( 1 << 15 );

      b = (  y - ( ( a * x ) >> iShift ) + ( 1 << ( iCountShift - 1 ) ) ) >> iCountShift;
    }
  }   

  // <-- end of LLS parameters estimation

  // get prediction -->
  uiExt = uiExt0;
  pLuma = pLuma0;
  pDst = pDst0;

  for( i = 0; i < uiHeight; i++ )
  {
    for( j = 0; j < uiWidth; j++ )
      pDst[j] = Clip( ( ( a * pLuma[j] ) >> iShift ) + b );

    pDst  += iDstStride;
    pLuma += iLumaStride;
  }
  // <-- end of get prediction

}
#endif

#if MN_DC_PRED_FILTER
/** Function for filtering intra DC predictor.
 * \param pSrc pointer to reconstructed sample array
 * \param iSrcStride the stride of the reconstructed sample array
 * \param rpDst reference to pointer for the prediction sample array
 * \param iDstStride the stride of the prediction sample array
 * \param iWidth the width of the block
 * \param iHeight the height of the block
 *
 * This function performs filtering left and top edges of the prediction samples for DC mode (intra coding).
 */
Void TComPrediction::xDCPredFiltering( Int* pSrc, Int iSrcStride, Pel*& rpDst, Int iDstStride, Int iWidth, Int iHeight )
{
  Pel* pDst = rpDst;
  Int x, y, iDstStride2, iSrcStride2;
  Int iIntraSizeIdx = g_aucConvertToBit[ iWidth ] + 1;
  static const UChar g_aucDCPredFilter[7] = { 0, 3, 2, 1, 0, 0, 0};

  switch (g_aucDCPredFilter[iIntraSizeIdx])
  {
  case 0:
    {}
    break;
  case 1:
    {
      // boundary pixels processing
      pDst[0] = (Pel)((pSrc[-iSrcStride] + pSrc[-1] + 6 * pDst[0] + 4) >> 3);

      for ( x = 1; x < iWidth; x++ )
        pDst[x] = (Pel)((pSrc[x - iSrcStride] + 7 * pDst[x] + 4) >> 3);

      for ( y = 1, iDstStride2 = iDstStride, iSrcStride2 = iSrcStride-1; y < iHeight; y++, iDstStride2+=iDstStride, iSrcStride2+=iSrcStride )
        pDst[iDstStride2] = (Pel)((pSrc[iSrcStride2] + 7 * pDst[iDstStride2] + 4) >> 3);
    }
    break;
  case 2:
    {
      // boundary pixels processing
      pDst[0] = (Pel)((pSrc[-iSrcStride] + pSrc[-1] + 2 * pDst[0] + 2) >> 2);

      for ( x = 1; x < iWidth; x++ )
        pDst[x] = (Pel)((pSrc[x - iSrcStride] + 3 * pDst[x] + 2) >> 2);

      for ( y = 1, iDstStride2 = iDstStride, iSrcStride2 = iSrcStride-1; y < iHeight; y++, iDstStride2+=iDstStride, iSrcStride2+=iSrcStride )
        pDst[iDstStride2] = (Pel)((pSrc[iSrcStride2] + 3 * pDst[iDstStride2] + 2) >> 2);
    }
    break;
  case 3:
    {
      // boundary pixels processing
      pDst[0] = (Pel)((3 * (pSrc[-iSrcStride] + pSrc[-1]) + 2 * pDst[0] + 4) >> 3);

      for ( x = 1; x < iWidth; x++ )
        pDst[x] = (Pel)((3 * pSrc[x - iSrcStride] + 5 * pDst[x] + 4) >> 3);

      for ( y = 1, iDstStride2 = iDstStride, iSrcStride2 = iSrcStride-1; y < iHeight; y++, iDstStride2+=iDstStride, iSrcStride2+=iSrcStride )
        pDst[iDstStride2] = (Pel)((3 * pSrc[iSrcStride2] + 5 * pDst[iDstStride2] + 4) >> 3);
    }
    break;
  }

  return;
}
#endif

#if HHI_DMM_INTRA
TComWedgeDist::TComWedgeDist()
{
  init();
}

TComWedgeDist::~TComWedgeDist()
{
}

Void TComWedgeDist::init()
{
  //   m_afpDistortFunc[0]  = NULL;                  // for DF_DEFAULT

  //   m_afpDistortFunc[8]  = TComRdCost::xGetSAD;
  m_afpDistortFunc[0]  = TComWedgeDist::xGetSAD4;
  m_afpDistortFunc[1] = TComWedgeDist::xGetSAD8;
  m_afpDistortFunc[2] = TComWedgeDist::xGetSAD16;
  m_afpDistortFunc[3] = TComWedgeDist::xGetSAD32;

  m_afpDistortFunc[4]  = TComWedgeDist::xGetSSE4;
  m_afpDistortFunc[5]  = TComWedgeDist::xGetSSE8;
  m_afpDistortFunc[6]  = TComWedgeDist::xGetSSE16;
  m_afpDistortFunc[7]  = TComWedgeDist::xGetSSE32;

  //   m_afpDistortFunc[13] = TComRdCost::xGetSAD64;
#ifdef ROUNDING_CONTROL_BIPRED
  //   m_afpDistortFuncRnd[0]  = NULL;
  //   m_afpDistortFuncRnd[8]  = TComRdCost::xGetSAD;
  m_afpDistortFuncRnd[9]  = TComRdCost::xGetSAD4;
  m_afpDistortFuncRnd[10] = TComRdCost::xGetSAD8;
  m_afpDistortFuncRnd[11] = TComRdCost::xGetSAD16;
  m_afpDistortFuncRnd[12] = TComRdCost::xGetSAD32;
  //   m_afpDistortFuncRnd[13] = TComRdCost::xGetSAD64;
#endif
}

UInt TComWedgeDist::xGetSAD4( DistParam* pcDtParam )
{
  Pel* piOrg   = pcDtParam->pOrg;
  Pel* piCur   = pcDtParam->pCur;
  Int  iRows   = pcDtParam->iRows;
  Int  iSubShift  = pcDtParam->iSubShift;
  Int  iSubStep   = ( 1 << iSubShift );
  Int  iStrideCur = pcDtParam->iStrideCur*iSubStep;
  Int  iStrideOrg = pcDtParam->iStrideOrg*iSubStep;

  UInt uiSum = 0;

  for( ; iRows != 0; iRows-=iSubStep )
  {
    uiSum += abs( piOrg[0] - piCur[0] );
    uiSum += abs( piOrg[1] - piCur[1] );
    uiSum += abs( piOrg[2] - piCur[2] );
    uiSum += abs( piOrg[3] - piCur[3] );

    piOrg += iStrideOrg;
    piCur += iStrideCur;
  }

  uiSum <<= iSubShift;
  return ( uiSum >> g_uiBitIncrement );
}

UInt TComWedgeDist::xGetSAD8( DistParam* pcDtParam )
{
  Pel* piOrg      = pcDtParam->pOrg;
  Pel* piCur      = pcDtParam->pCur;
  Int  iRows      = pcDtParam->iRows;
  Int  iSubShift  = pcDtParam->iSubShift;
  Int  iSubStep   = ( 1 << iSubShift );
  Int  iStrideCur = pcDtParam->iStrideCur*iSubStep;
  Int  iStrideOrg = pcDtParam->iStrideOrg*iSubStep;

  UInt uiSum = 0;

  for( ; iRows != 0; iRows-=iSubStep )
  {
    uiSum += abs( piOrg[0] - piCur[0] );
    uiSum += abs( piOrg[1] - piCur[1] );
    uiSum += abs( piOrg[2] - piCur[2] );
    uiSum += abs( piOrg[3] - piCur[3] );
    uiSum += abs( piOrg[4] - piCur[4] );
    uiSum += abs( piOrg[5] - piCur[5] );
    uiSum += abs( piOrg[6] - piCur[6] );
    uiSum += abs( piOrg[7] - piCur[7] );

    piOrg += iStrideOrg;
    piCur += iStrideCur;
  }

  uiSum <<= iSubShift;
  return ( uiSum >> g_uiBitIncrement );
}

UInt TComWedgeDist::xGetSAD16( DistParam* pcDtParam )
{
  Pel* piOrg   = pcDtParam->pOrg;
  Pel* piCur   = pcDtParam->pCur;
  Int  iRows   = pcDtParam->iRows;
  Int  iSubShift  = pcDtParam->iSubShift;
  Int  iSubStep   = ( 1 << iSubShift );
  Int  iStrideCur = pcDtParam->iStrideCur*iSubStep;
  Int  iStrideOrg = pcDtParam->iStrideOrg*iSubStep;

  UInt uiSum = 0;

  for( ; iRows != 0; iRows-=iSubStep )
  {
    uiSum += abs( piOrg[0] - piCur[0] );
    uiSum += abs( piOrg[1] - piCur[1] );
    uiSum += abs( piOrg[2] - piCur[2] );
    uiSum += abs( piOrg[3] - piCur[3] );
    uiSum += abs( piOrg[4] - piCur[4] );
    uiSum += abs( piOrg[5] - piCur[5] );
    uiSum += abs( piOrg[6] - piCur[6] );
    uiSum += abs( piOrg[7] - piCur[7] );
    uiSum += abs( piOrg[8] - piCur[8] );
    uiSum += abs( piOrg[9] - piCur[9] );
    uiSum += abs( piOrg[10] - piCur[10] );
    uiSum += abs( piOrg[11] - piCur[11] );
    uiSum += abs( piOrg[12] - piCur[12] );
    uiSum += abs( piOrg[13] - piCur[13] );
    uiSum += abs( piOrg[14] - piCur[14] );
    uiSum += abs( piOrg[15] - piCur[15] );

    piOrg += iStrideOrg;
    piCur += iStrideCur;
  }

  uiSum <<= iSubShift;
  return ( uiSum >> g_uiBitIncrement );
}

UInt TComWedgeDist::xGetSAD32( DistParam* pcDtParam )
{
  Pel* piOrg   = pcDtParam->pOrg;
  Pel* piCur   = pcDtParam->pCur;
  Int  iRows   = pcDtParam->iRows;
  Int  iSubShift  = pcDtParam->iSubShift;
  Int  iSubStep   = ( 1 << iSubShift );
  Int  iStrideCur = pcDtParam->iStrideCur*iSubStep;
  Int  iStrideOrg = pcDtParam->iStrideOrg*iSubStep;

  UInt uiSum = 0;

  for( ; iRows != 0; iRows-=iSubStep )
  {
    uiSum += abs( piOrg[0] - piCur[0] );
    uiSum += abs( piOrg[1] - piCur[1] );
    uiSum += abs( piOrg[2] - piCur[2] );
    uiSum += abs( piOrg[3] - piCur[3] );
    uiSum += abs( piOrg[4] - piCur[4] );
    uiSum += abs( piOrg[5] - piCur[5] );
    uiSum += abs( piOrg[6] - piCur[6] );
    uiSum += abs( piOrg[7] - piCur[7] );
    uiSum += abs( piOrg[8] - piCur[8] );
    uiSum += abs( piOrg[9] - piCur[9] );
    uiSum += abs( piOrg[10] - piCur[10] );
    uiSum += abs( piOrg[11] - piCur[11] );
    uiSum += abs( piOrg[12] - piCur[12] );
    uiSum += abs( piOrg[13] - piCur[13] );
    uiSum += abs( piOrg[14] - piCur[14] );
    uiSum += abs( piOrg[15] - piCur[15] );
    uiSum += abs( piOrg[16] - piCur[16] );
    uiSum += abs( piOrg[17] - piCur[17] );
    uiSum += abs( piOrg[18] - piCur[18] );
    uiSum += abs( piOrg[19] - piCur[19] );
    uiSum += abs( piOrg[20] - piCur[20] );
    uiSum += abs( piOrg[21] - piCur[21] );
    uiSum += abs( piOrg[22] - piCur[22] );
    uiSum += abs( piOrg[23] - piCur[23] );
    uiSum += abs( piOrg[24] - piCur[24] );
    uiSum += abs( piOrg[25] - piCur[25] );
    uiSum += abs( piOrg[26] - piCur[26] );
    uiSum += abs( piOrg[27] - piCur[27] );
    uiSum += abs( piOrg[28] - piCur[28] );
    uiSum += abs( piOrg[29] - piCur[29] );
    uiSum += abs( piOrg[30] - piCur[30] );
    uiSum += abs( piOrg[31] - piCur[31] );

    piOrg += iStrideOrg;
    piCur += iStrideCur;
  }

  uiSum <<= iSubShift;
  return ( uiSum >> g_uiBitIncrement );
}

UInt TComWedgeDist::xGetSSE4( DistParam* pcDtParam )
{
  Pel* piOrg   = pcDtParam->pOrg;
  Pel* piCur   = pcDtParam->pCur;
  Int  iRows   = pcDtParam->iRows;
  Int  iStrideOrg = pcDtParam->iStrideOrg;
  Int  iStrideCur = pcDtParam->iStrideCur;

  UInt uiSum = 0;
  UInt uiShift = g_uiBitIncrement<<1;

  Int  iTemp;

  for( ; iRows != 0; iRows-- )
  {

    iTemp = piOrg[0] - piCur[0]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[1] - piCur[1]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[2] - piCur[2]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[3] - piCur[3]; uiSum += ( iTemp * iTemp ) >> uiShift;

    piOrg += iStrideOrg;
    piCur += iStrideCur;
  }

  return ( uiSum );
}

UInt TComWedgeDist::xGetSSE8( DistParam* pcDtParam )
{
  Pel* piOrg   = pcDtParam->pOrg;
  Pel* piCur   = pcDtParam->pCur;
  Int  iRows   = pcDtParam->iRows;
  Int  iStrideOrg = pcDtParam->iStrideOrg;
  Int  iStrideCur = pcDtParam->iStrideCur;

  UInt uiSum = 0;
  UInt uiShift = g_uiBitIncrement<<1;

  Int  iTemp;

  for( ; iRows != 0; iRows-- )
  {
    iTemp = piOrg[0] - piCur[0]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[1] - piCur[1]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[2] - piCur[2]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[3] - piCur[3]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[4] - piCur[4]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[5] - piCur[5]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[6] - piCur[6]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[7] - piCur[7]; uiSum += ( iTemp * iTemp ) >> uiShift;

    piOrg += iStrideOrg;
    piCur += iStrideCur;
  }

  return ( uiSum );
}

UInt TComWedgeDist::xGetSSE16( DistParam* pcDtParam )
{
  Pel* piOrg   = pcDtParam->pOrg;
  Pel* piCur   = pcDtParam->pCur;
  Int  iRows   = pcDtParam->iRows;
  Int  iStrideOrg = pcDtParam->iStrideOrg;
  Int  iStrideCur = pcDtParam->iStrideCur;

  UInt uiSum = 0;
  UInt uiShift = g_uiBitIncrement<<1;

  Int  iTemp;

  for( ; iRows != 0; iRows-- )
  {

    iTemp = piOrg[ 0] - piCur[ 0]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[ 1] - piCur[ 1]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[ 2] - piCur[ 2]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[ 3] - piCur[ 3]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[ 4] - piCur[ 4]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[ 5] - piCur[ 5]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[ 6] - piCur[ 6]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[ 7] - piCur[ 7]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[ 8] - piCur[ 8]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[ 9] - piCur[ 9]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[10] - piCur[10]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[11] - piCur[11]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[12] - piCur[12]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[13] - piCur[13]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[14] - piCur[14]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[15] - piCur[15]; uiSum += ( iTemp * iTemp ) >> uiShift;

    piOrg += iStrideOrg;
    piCur += iStrideCur;
  }

  return ( uiSum );
}

UInt TComWedgeDist::xGetSSE32( DistParam* pcDtParam )
{
  Pel* piOrg   = pcDtParam->pOrg;
  Pel* piCur   = pcDtParam->pCur;
  Int  iRows   = pcDtParam->iRows;
  Int  iStrideOrg = pcDtParam->iStrideOrg;
  Int  iStrideCur = pcDtParam->iStrideCur;

  UInt uiSum = 0;
  UInt uiShift = g_uiBitIncrement<<1;
  Int  iTemp;

  for( ; iRows != 0; iRows-- )
  {

    iTemp = piOrg[ 0] - piCur[ 0]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[ 1] - piCur[ 1]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[ 2] - piCur[ 2]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[ 3] - piCur[ 3]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[ 4] - piCur[ 4]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[ 5] - piCur[ 5]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[ 6] - piCur[ 6]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[ 7] - piCur[ 7]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[ 8] - piCur[ 8]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[ 9] - piCur[ 9]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[10] - piCur[10]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[11] - piCur[11]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[12] - piCur[12]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[13] - piCur[13]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[14] - piCur[14]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[15] - piCur[15]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[16] - piCur[16]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[17] - piCur[17]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[18] - piCur[18]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[19] - piCur[19]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[20] - piCur[20]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[21] - piCur[21]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[22] - piCur[22]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[23] - piCur[23]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[24] - piCur[24]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[25] - piCur[25]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[26] - piCur[26]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[27] - piCur[27]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[28] - piCur[28]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[29] - piCur[29]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[30] - piCur[30]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[31] - piCur[31]; uiSum += ( iTemp * iTemp ) >> uiShift;

    piOrg += iStrideOrg;
    piCur += iStrideCur;
  }

  return ( uiSum );
}

Void TComWedgeDist::setDistParam( UInt uiBlkWidth, UInt uiBlkHeight, WedgeDist eWDist, DistParam& rcDistParam )
{
  // set Block Width / Height
  rcDistParam.iCols    = uiBlkWidth;
  rcDistParam.iRows    = uiBlkHeight;
  rcDistParam.DistFunc = m_afpDistortFunc[eWDist + g_aucConvertToBit[ rcDistParam.iCols ] ];

  // initialize
  rcDistParam.iSubShift  = 0;
}

UInt TComWedgeDist::getDistPart( Pel* piCur, Int iCurStride,  Pel* piOrg, Int iOrgStride, UInt uiBlkWidth, UInt uiBlkHeight, WedgeDist eWDist )
{
  DistParam cDtParam;
  setDistParam( uiBlkWidth, uiBlkHeight, eWDist, cDtParam );
  cDtParam.pOrg       = piOrg;
  cDtParam.pCur       = piCur;
  cDtParam.iStrideOrg = iOrgStride;
  cDtParam.iStrideCur = iCurStride;
#ifdef DCM_RDCOST_TEMP_FIX //Temporary fix since DistParam is lacking a constructor and the variable iStep is not initialized
  cDtParam.iStep      = 1;
#endif
  return cDtParam.DistFunc( &cDtParam );
}
#endif
