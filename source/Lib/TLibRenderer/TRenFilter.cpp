

#include "TRenImage.h"
#include "TRenFilter.h"
#include "../TLibCommon/TComPredFilter.h"


///// COMMON /////
Void TRenFilter::setSubPelShiftLUT( Int iLutPrec, Int** piSubPelShiftLUT, Int iShift )
{
  //ToDo: use same rounding for left and right
  AOT( iLutPrec < 0 || iLutPrec > 2 );
  Int iStep = 1 << iLutPrec;
  for (Int iDelta = 0; iDelta < (iStep << 1)+1; iDelta++ )
  {
    for (Int iCurDelta = 0; iCurDelta < (iStep << 1)+1; iCurDelta++ )
    {
      if (iCurDelta <= iDelta)
      {
        piSubPelShiftLUT[iDelta][iCurDelta] =  (iDelta != 0) ?( ( iStep * iCurDelta + ( iDelta >> 1) )/ iDelta) + iShift * iStep :  iShift * iStep;
      }
      else
      {
        piSubPelShiftLUT[iDelta][iCurDelta] = 0xdeaddead;
      }
    }
  }
}

Void TRenFilter::setupZLUT( Bool bBlendUseDistWeight, Int iBlendZThresPerc, Int iRelDistToLeft, Int** ppiBaseShiftLUTLeft, Int** ppiBaseShiftLUTRight, Int& riBlendZThres, Int& riBlendDistWeight, Int* piInvZLUTLeft, Int* piInvZLUTRight )
{
  AOT( iRelDistToLeft == -1 );
  riBlendDistWeight = bBlendUseDistWeight ? iRelDistToLeft :  1 << (REN_VDWEIGHT_PREC - 1);

  for (UInt uiDepthValue = 0; uiDepthValue <= 256; uiDepthValue++)
  {
    //GT: retrieve depth approx from shift
    piInvZLUTLeft [uiDepthValue] = abs( ppiBaseShiftLUTLeft [0][uiDepthValue] );
    piInvZLUTRight[uiDepthValue] = abs( ppiBaseShiftLUTRight[0][uiDepthValue] );
  }
  // Set Threshold
  riBlendZThres  = ( max( abs(piInvZLUTLeft[0]- piInvZLUTLeft[255]), abs(piInvZLUTRight[0]- piInvZLUTRight[255]) ) * iBlendZThresPerc + 50)  / 100;
}

Void TRenFilter::filledToUsedPelMap( PelImage* pcFilledImage, PelImage* pcUsedPelsImage, Int iUsedPelMapMarExt )
{
  // Convert to binary map
  Int iWidth  = pcFilledImage      ->getPlane(0)->getWidth ();
  Int iHeight = pcFilledImage      ->getPlane(0)->getHeight();

  AOT( iWidth  != pcUsedPelsImage  ->getPlane(0)->getWidth () );
  AOT( iHeight != pcUsedPelsImage  ->getPlane(0)->getHeight() );
  AOF( pcUsedPelsImage->is420() );

  Int iSrcStride  = pcFilledImage  ->getPlane(0)->getStride();
  Int iDstStrideY = pcUsedPelsImage->getPlane(0)->getStride();
  Int iDstStrideU = pcUsedPelsImage->getPlane(1)->getStride();
  Int iDstStrideV = pcUsedPelsImage->getPlane(2)->getStride();

  Pel* pcSrcData  = pcFilledImage  ->getPlane(0)->getPlaneData();
  Pel* pcDstDataY = pcUsedPelsImage->getPlane(0)->getPlaneData();
  Pel* pcDstDataU = pcUsedPelsImage->getPlane(1)->getPlaneData();
  Pel* pcDstDataV = pcUsedPelsImage->getPlane(2)->getPlaneData(); // Only used as buffer

  for (Int iPosY = 0; iPosY < iHeight; iPosY++ )
  {
    for (Int iPosX = 0; iPosX < iWidth; iPosX++ )
    {
      pcDstDataY[iPosX] = ( pcSrcData[iPosX] != REN_IS_FILLED ) ? REN_USED_PEL : REN_UNUSED_PEL;

      if ((iPosX & 1) && (iPosY & 1))
      {
        pcDstDataU[iPosX >> 1] = (   ( pcSrcData[iPosX                 ] != REN_IS_FILLED )
          || ( pcSrcData[iPosX - 1             ] != REN_IS_FILLED )
          || ( pcSrcData[iPosX     - iSrcStride] != REN_IS_FILLED )
          || ( pcSrcData[iPosX - 1 - iSrcStride] != REN_IS_FILLED )
          ) ? REN_USED_PEL : REN_UNUSED_PEL;
      }
    }

    if ( iPosY & 1 )
    {
      pcDstDataU += iDstStrideU;
    }

    pcDstDataY += iDstStrideY;
    pcSrcData  += iSrcStride;
  }

  //// Dilatation for Interpolation Filters ////
  //GT: should better be defined somewhere else ...
  const Int iLumaIntFiltHalfSize   = 4;
  const Int iChromaIntFiltHalfSize = 2;

  Int iDilateSizeLuma   = iLumaIntFiltHalfSize   +   iUsedPelMapMarExt      ;
  Int iDilateSizeChroma = iChromaIntFiltHalfSize + ( iUsedPelMapMarExt >> 1);

  pcDstDataY = pcUsedPelsImage->getPlane(0)->getPlaneData();
  pcDstDataU = pcUsedPelsImage->getPlane(1)->getPlaneData();
  pcDstDataV = pcUsedPelsImage->getPlane(2)->getPlaneData();

  // Dilate Luma horizontally
  xDilate( pcDstDataY, iDstStrideY, iWidth, iHeight, pcDstDataY, iDstStrideY, iDilateSizeLuma, false, true  );
  xDilate( pcDstDataY, iDstStrideY, iWidth, iHeight, pcDstDataY, iDstStrideY, iDilateSizeLuma, false, false );

  // Dilate Chorma vertically and horizontally (for UV-up)
  xDilate( pcDstDataU, iDstStrideU, iWidth>>1, iHeight>>1, pcDstDataU, iDstStrideU, iDilateSizeChroma, false, true  );
  xDilate( pcDstDataU, iDstStrideU, iWidth>>1, iHeight>>1, pcDstDataU, iDstStrideU, iDilateSizeChroma, false, false );

  xDilate( pcDstDataU, iDstStrideU, iWidth>>1, iHeight>>1, pcDstDataU, iDstStrideU, iChromaIntFiltHalfSize, true, true  );
  xDilate( pcDstDataU, iDstStrideU, iWidth>>1, iHeight>>1, pcDstDataU, iDstStrideU, iChromaIntFiltHalfSize, true, false );

  for (Int iPosY = 0; iPosY < (iHeight >> 1); iPosY++ )
  {
    for (Int iPosX = 0; iPosX < (iWidth >> 1); iPosX++ )
    {
      pcDstDataV[iPosX] = pcDstDataU[iPosX];
    }

    pcDstDataU += iDstStrideU;
    pcDstDataV += iDstStrideV;
  }
}

/////////// Copy /////////////
Void TRenFilter::copy(Pel* pcInputPlaneData, Int iInputStride, Int iWidth, Int iHeight, Pel* pcOutputPlaneData, Int iOutputStride  )
{
  xDistributeArray(pcInputPlaneData, iInputStride, 1, 1, iWidth, iHeight ,pcOutputPlaneData, iOutputStride, 1 , 1 );

}

/////////// Horizontal Mirror ///////////
template <typename T>
Void TRenFilter::mirrorHor( TRenImage<T> *pcImage )
{
  for (UInt uCurPlane = 0 ; uCurPlane < pcImage->getNumberOfPlanes(); uCurPlane++ )
  {
    mirrorHor( pcImage->getPlane(uCurPlane) );
  }
}

template <typename T>
Void TRenFilter::mirrorHor( TRenImagePlane<T> *pcImagePlane )
{
  T* pcPlaneData = pcImagePlane->getPlaneDataOrg();
  T cTemp;
  UInt uiStride = pcImagePlane->getStride();
  UInt uiHeight = pcImagePlane->getHeightOrg();
  UInt uiWidth  = pcImagePlane->getWidthOrg();

  for (UInt uiPosY = 0; uiPosY < uiHeight; uiPosY++)
  {
    for (UInt uiPosX = 0; uiPosX < ( (uiWidth+1) >> 1); uiPosX++ )
    {
      cTemp = pcPlaneData[uiPosX];
      pcPlaneData[uiPosX] = pcPlaneData[uiWidth - uiPosX-1];
      pcPlaneData[uiWidth-uiPosX-1] = cTemp;
    }
    pcPlaneData += uiStride;
  }
}

/////////// Comparison ///////////
Int64 TRenFilter::SSE   (PelImagePlane* pcInputPlane1, PelImagePlane* pcInputPlane2   )
{
  UInt uiWidth     = pcInputPlane1->getWidth();
  UInt uiHeight    = pcInputPlane1->getHeight();

  UInt uiStride1   = pcInputPlane1->getStride();
  UInt uiStride2   = pcInputPlane2->getStride();

  Pel* pucImData1  = pcInputPlane1->getPlaneData();
  Pel* pucImData2  = pcInputPlane2->getPlaneData();

  return SSE( pucImData1, (Int) uiStride1, (Int) uiWidth, (Int) uiHeight, pucImData2, (Int) uiStride2 );
}

Int64 TRenFilter::SSE( Pel* piSrc1, Int iSrcStride1, Int iWidth, Int iHeight,  Pel* piSrc2, Int iSrcStride2 )
{
  Int64 iSSE = 0;

  Int iShift = g_uiBitIncrement << 1;
  for(Int iPosY = 0; iPosY < iHeight; iPosY++)
  {
    for(Int iPosX = 0; iPosX < iWidth; iPosX++)
    {
      Int iDiff = ( piSrc1[iPosX] - piSrc2[iPosX] );
      iSSE += (( iDiff * iDiff ) >> iShift);
    }
    piSrc1 += iSrcStride1;
    piSrc2 += iSrcStride2;
  }
  return iSSE;
}

template <typename T>
Bool TRenFilter::compare( TRenImage<T> *pInputImage1, TRenImage<T> *pInputImage2 )
{
  Bool bIsEqual = true;
  for (UInt uiCurPlane = 0 ; uiCurPlane < pInputImage1->getNumberOfPlanes(); uiCurPlane++ )
  {
    bIsEqual = bIsEqual && compare(pInputImage1->getPlane(uiCurPlane), pInputImage2->getPlane(uiCurPlane));
  }
  return bIsEqual;
}

template <typename T>
Bool TRenFilter::compare   (TRenImagePlane<T>* pcInputPlane1  , TRenImagePlane<T>* pcInputPlane2   )
{
  UInt uiWidth  = pcInputPlane1->getWidth();
  UInt uiHeight = pcInputPlane1->getHeight();

  UInt uiStride1 = pcInputPlane1->getStride();
  UInt uiStride2 = pcInputPlane2->getStride();

  T* pucImData1  = pcInputPlane1->getPlaneData();
  T* pucImData2  = pcInputPlane2->getPlaneData();

  Bool bEqual = true;
  for(UInt uiPosY = 0; uiPosY < uiHeight; uiPosY++)
  {

    for(UInt uiPosX = 0; uiPosX < uiWidth; uiPosX++)
    {
      bEqual = bEqual && ( pucImData1[uiPosX] == pucImData2[uiPosX]);
    }
    pucImData1 += uiStride1;
    pucImData2 += uiStride2;
  }
  return bEqual;
}

/////////// Sampling ///////////

inline Void TRenFilter::sampleUp2Tap13(PelImage* pcInputImage, PelImage* pcOutputImage)
{// UpSampling from JSVM Software (DownConvertStatic) ???
  UInt uiNumPlanes = pcInputImage->getNumberOfPlanes();

  for (UInt uiCurPlane = 0; uiCurPlane < uiNumPlanes; uiCurPlane++)
  {
    PelImagePlane* pcInputPlane  = pcInputImage ->getPlane(uiCurPlane);
    PelImagePlane* pcOutputPlane = pcOutputImage->getPlane(uiCurPlane);

    Int iWidth  = pcInputPlane->getWidth();
    Int iHeight = pcInputPlane->getHeight();

    Int iInputStride  = pcInputPlane->getStride();
    Int iOutputStride = pcOutputPlane->getStride();

    assert( iWidth  == 2 * pcOutputPlane->getWidth ());
    assert( iHeight == 2 * pcOutputPlane->getHeight());

    Int iOffset;

    Pel *pcInputPlaneData   = pcInputPlane->getPlaneData();
    Int *piDataVerUp        = new Int[iWidth * iHeight * 2];
    Pel *pcOutputPlaneData  = pcOutputPlane->getPlaneData();

    // Up sampling filter.
    Int aiFilterCoeff[16] = { 0, 0, 1, 0, -5, 0, 20, 32, 20, 0, -5,  0, 1, 0, 0, 32 };

    // Normalization factors for filtered values.
    Int iDivH = 1, iDivV = 1, iAddH = 0, iAddV = 0;

    // Factors after horizontal and vertical filtering.
    iDivH = (aiFilterCoeff[15]*aiFilterCoeff[15]); iAddH = iDivH / 2;

    Int* piDst = new Int[2*iWidth];
    //1) VERTICAL UPSAMPLING.

    // Process all cols.
    for(Int i=0; i<iWidth; i++ )
    {
      // Set source (col) poInter.
      Pel* pcSrc = &pcInputPlaneData[i];

      // Process all rows.
      for( Int j=0; j<iHeight; j++ )
      {
        // Adjust indices of border samples.
        Int i00 = ((j <   3) ? 0   : j-3) * iInputStride;
        Int i01 = ((j <   2) ? 0   : j-2) * iInputStride;
        Int i02 = ((j <   1) ? 0   : j-1) * iInputStride;
        Int i03 = ((j < iHeight  ) ? j   : j-1) * iInputStride;
        Int i04 = ((j < iHeight-1) ? j+1 : j-1) * iInputStride;
        Int i05 = ((j < iHeight-2) ? j+2 : j-1) * iInputStride;
        Int i06 = ((j < iHeight-3) ? j+3 : j-1) * iInputStride;
        Int i07 = ((j < iHeight-4) ? j+4 : j-1) * iInputStride;

        // Calculate filtered (even) sample.
        piDst[2*j+0] = aiFilterCoeff[13] * pcSrc[i00]
        + aiFilterCoeff[11] * pcSrc[i01]
        + aiFilterCoeff[ 9] * pcSrc[i02]
        + aiFilterCoeff[ 7] * pcSrc[i03]
        + aiFilterCoeff[ 5] * pcSrc[i04]
        + aiFilterCoeff[ 3] * pcSrc[i05]
        + aiFilterCoeff[ 1] * pcSrc[i06];

        // Calculate filtered (odd) sample.
        piDst[2*j+1] = aiFilterCoeff[14] * pcSrc[i00]
        + aiFilterCoeff[12] * pcSrc[i01]
        + aiFilterCoeff[10] * pcSrc[i02]
        + aiFilterCoeff[ 8] * pcSrc[i03]
        + aiFilterCoeff[ 6] * pcSrc[i04]
        + aiFilterCoeff[ 4] * pcSrc[i05]
        + aiFilterCoeff[ 2] * pcSrc[i06]
        + aiFilterCoeff[ 0] * pcSrc[i07];
      }

      // Process all filtered samples.
      for(Int j=0; j<(2*iHeight); j++ )
      {
        // Scale and copy to image buffer.
        piDataVerUp[iWidth*j+i] = (piDst[j] + iAddV) / iDivV;
      }
    }

    // Update h
    iHeight *= 2;

    // 2) HORIZONTAL UPSAMPLING.

    // Process all rows.
    for( Int j=0; j<iHeight; j++ )
    {
      // Set source (row) poInter.
      Int* piSrc = &piDataVerUp[iWidth*j];

      // Process all cols.
      for( Int i=0; i<iWidth; i++ )
      {
        // Adjust indices of border samples.
        Int i00 = (i <   3) ? 0   : i-3;
        Int i01 = (i <   2) ? 0   : i-2;
        Int i02 = (i <   1) ? 0   : i-1;
        Int i03 = (i < iWidth  ) ? i   : iWidth-1;
        Int i04 = (i < iWidth-1) ? i+1 : iWidth-1;
        Int i05 = (i < iWidth-2) ? i+2 : iWidth-1;
        Int i06 = (i < iWidth-3) ? i+3 : iWidth-1;
        Int i07 = (i < iWidth-4) ? i+4 : iWidth-1;

        // Calculate filtered (even) sample.
        piDst[2*i+0] =   aiFilterCoeff[13] * piSrc[i00]
        + aiFilterCoeff[11] * piSrc[i01]
        + aiFilterCoeff[ 9] * piSrc[i02]
        + aiFilterCoeff[ 7] * piSrc[i03]
        + aiFilterCoeff[ 5] * piSrc[i04]
        + aiFilterCoeff[ 3] * piSrc[i05]
        + aiFilterCoeff[ 1] * piSrc[i06];

        // Calculate filtered (odd) sample.
        piDst[2*i+1] = aiFilterCoeff[14] * piSrc[i00]
        + aiFilterCoeff[12] * piSrc[i01]
        + aiFilterCoeff[10] * piSrc[i02]
        + aiFilterCoeff[ 8] * piSrc[i03]
        + aiFilterCoeff[ 6] * piSrc[i04]
        + aiFilterCoeff[ 4] * piSrc[i05]
        + aiFilterCoeff[ 2] * piSrc[i06]
        + aiFilterCoeff[ 0] * piSrc[i07];
      }

      iOffset = 2* iOutputStride * j;
      // Process all filtered samples.
      for(Int i=0; i<iWidth*2; i++ )
      {
        // Scale and copy to image buffer.
        pcOutputPlaneData[iOffset+i] = Max(Min((Pel) ((piDst[i] + iAddH) / iDivH), g_uiBASE_MAX ),0);
      }
    }

    delete [] piDataVerUp;
    delete [] piDst;

  }
}


Void TRenFilter::sampleDown2Tap13(PelImage* pcInputImage, PelImage* pcOutputImage)
{ // DownSampling from JSVM Software (DownConvertStatic) ??

  UInt uiNumPlanes = pcInputImage->getNumberOfPlanes();

  for (UInt uiCurPlane = 0; uiCurPlane < uiNumPlanes; uiCurPlane++)
  {
    sampleDown2Tap13( pcInputImage ->getPlane(uiCurPlane),  pcOutputImage->getPlane(uiCurPlane) );
  }
};

Void TRenFilter::sampleDown2Tap13( Pel* pcInputPlaneData, Int iInputStride, Int iWidth, Int iHeight, Pel* pcOutputPlaneData, Int iOutputStride  )
{ // DownSampling from JSVM Software (DownConvertStatic) ??
  Int iOffset, iPosX, iPosY, k;
  Int* piDataHorDown = new Int[(Int)(iWidth * iHeight / 2)];

  // Downsampling filter.
  Int aiFilterCoeff[16] = { 0, 2, 0, -4, -3, 5, 19, 26, 19, 5, -3, -4, 0, 2, 0, 64 };

  // Normalization factors for filtered values.
  Int iDivH = 1, iDivV = 1, iAddH = 0, iAddV = 0;

  iDivV = (aiFilterCoeff[15]*aiFilterCoeff[15]); iAddV = iDivV / 2;

  // Allocate and init single row of filtered samples.
  Int* piDst = new Int[iWidth];

  // 1) HORIZONTAL DOWNSAMPLING.

  // Process all rows.
  for( iPosY=0; iPosY<iHeight; iPosY++ )
  {
    // Set source (row) poInter.
    Pel* pcSrc = &pcInputPlaneData[iInputStride*iPosY];

    // Process all cols.
    for( iPosX=0, k=0; iPosX<(iWidth/2); iPosX++, k+=2 )
    {
      // Adjust indices of border samples.
      Int i00 = (k <       7) ? 0   : k    -7;
      Int i01 = (k <       6) ? 0   : k    -6;
      Int i02 = (k <       5) ? 0   : k    -5;
      Int i03 = (k <       4) ? 0   : k    -4;
      Int i04 = (k <       3) ? 0   : k    -3;
      Int i05 = (k <       2) ? 0   : k    -2;
      Int i06 = (k <       1) ? 0   : k    -1;
      Int i07 = (k < iWidth  ) ? k   : iWidth-1;
      Int i08 = (k < iWidth-1) ? k+1 : iWidth-1;
      Int i09 = (k < iWidth-2) ? k+2 : iWidth-1;
      Int i10 = (k < iWidth-3) ? k+3 : iWidth-1;
      Int i11 = (k < iWidth-4) ? k+4 : iWidth-1;
      Int i12 = (k < iWidth-5) ? k+5 : iWidth-1;
      Int i13 = (k < iWidth-6) ? k+6 : iWidth-1;
      Int i14 = (k < iWidth-7) ? k+7 : iWidth-1;

      // Calculate filtered sample.
      piDst[iPosX] =     aiFilterCoeff[ 0] * pcSrc[i00]
      + aiFilterCoeff[ 1] * pcSrc[i01]
      + aiFilterCoeff[ 2] * pcSrc[i02]
      + aiFilterCoeff[ 3] * pcSrc[i03]
      + aiFilterCoeff[ 4] * pcSrc[i04]
      + aiFilterCoeff[ 5] * pcSrc[i05]
      + aiFilterCoeff[ 6] * pcSrc[i06]
      + aiFilterCoeff[ 7] * pcSrc[i07]
      + aiFilterCoeff[ 8] * pcSrc[i08]
      + aiFilterCoeff[ 9] * pcSrc[i09]
      + aiFilterCoeff[10] * pcSrc[i10]
      + aiFilterCoeff[11] * pcSrc[i11]
      + aiFilterCoeff[12] * pcSrc[i12]
      + aiFilterCoeff[13] * pcSrc[i13]
      + aiFilterCoeff[14] * pcSrc[i14];
    }

    iOffset = iPosY * iWidth/2;
    // Process all filtered samples.
    for( iPosX=0; iPosX<(iWidth/2); iPosX++ )
    {
      // Scale and copy back to image buffer.
      piDataHorDown[iOffset+iPosX] = (piDst[iPosX] + iAddH) / iDivH;
    }
  }

  // Update w.
  iWidth >>= 1;

  // 2) VERTICAL DOWNSAMPLING.

  // Process all cols.
  for(  iPosX=0; iPosX<iWidth; iPosX++ )
  {
    // Set source (col) poInter.
    Int* piSrc = &piDataHorDown[iPosX];

    // Process all rows.
    for(  iPosY=0, k=0; iPosY<(iHeight/2); iPosY++, k+=2 )
    {
      // Adjust indices of border samples.
      Int i00 = ((k <       7) ? 0   : k    -7) * iWidth;
      Int i01 = ((k <       6) ? 0   : k    -6) * iWidth;
      Int i02 = ((k <       5) ? 0   : k    -5) * iWidth;
      Int i03 = ((k <       4) ? 0   : k    -4) * iWidth;
      Int i04 = ((k <       3) ? 0   : k    -3) * iWidth;
      Int i05 = ((k <       2) ? 0   : k    -2) * iWidth;
      Int i06 = ((k <       1) ? 0   : k    -1) * iWidth;
      Int i07 = ((k < iHeight  ) ? k   : iHeight-1) * iWidth;
      Int i08 = ((k < iHeight-1) ? k+1 : iHeight-1) * iWidth;
      Int i09 = ((k < iHeight-2) ? k+2 : iHeight-1) * iWidth;
      Int i10 = ((k < iHeight-3) ? k+3 : iHeight-1) * iWidth;
      Int i11 = ((k < iHeight-4) ? k+4 : iHeight-1) * iWidth;
      Int i12 = ((k < iHeight-5) ? k+5 : iHeight-1) * iWidth;
      Int i13 = ((k < iHeight-6) ? k+6 : iHeight-1) * iWidth;
      Int i14 = ((k < iHeight-7) ? k+7 : iHeight-1) * iWidth;

      // Calculate filtered sample.
      piDst[iPosY] =     aiFilterCoeff[ 0] * piSrc[i00]
      + aiFilterCoeff[ 1] * piSrc[i01]
      + aiFilterCoeff[ 2] * piSrc[i02]
      + aiFilterCoeff[ 3] * piSrc[i03]
      + aiFilterCoeff[ 4] * piSrc[i04]
      + aiFilterCoeff[ 5] * piSrc[i05]
      + aiFilterCoeff[ 6] * piSrc[i06]
      + aiFilterCoeff[ 7] * piSrc[i07]
      + aiFilterCoeff[ 8] * piSrc[i08]
      + aiFilterCoeff[ 9] * piSrc[i09]
      + aiFilterCoeff[10] * piSrc[i10]
      + aiFilterCoeff[11] * piSrc[i11]
      + aiFilterCoeff[12] * piSrc[i12]
      + aiFilterCoeff[13] * piSrc[i13]
      + aiFilterCoeff[14] * piSrc[i14];
    }

    // Process all filtered samples.
    for( iPosY=0; iPosY<(iHeight/2); iPosY++ )
    {
      // Scale and copy back to image buffer.
      pcOutputPlaneData[iOutputStride*iPosY+iPosX] = Max(Min( ( Pel) ( (piDst[iPosY] + iAddV) / iDivV), g_uiBASE_MAX ),0);
    }
  }

  delete [] piDataHorDown;
  delete [] piDst;
}

Void TRenFilter::sampleDown2Tap13(PelImagePlane* pcInputPlane, PelImagePlane* pcOutputPlane)
{ // DownSampling from JSVM Software (DownConvertStatic) ??
  Int iWidth       = pcInputPlane->getWidth();
  Int iHeight      = pcInputPlane->getHeight();

  assert( pcOutputPlane->getWidth () == (iWidth  >> 1 ));
  assert( pcOutputPlane->getHeight() == (iHeight >> 1 ));

  Int iInputStride  = pcInputPlane->getStride();
  Int iOutputStride = pcOutputPlane->getStride();

  Pel* pcInputPlaneData = pcInputPlane ->getPlaneData();
  Pel* pcOutputPlaneData = pcOutputPlane->getPlaneData();

  sampleDown2Tap13( pcInputPlaneData, iInputStride, iWidth, iHeight, pcOutputPlaneData, iOutputStride );
};

Void TRenFilter::sampleVerDown2Tap13( PelImagePlane* pcInputPlane, PelImagePlane* pcOutputPlane, Int uiPad)
{ // DownSampling from JSVM Software (DownConvertStatic) ??
  Int iWidth       = pcInputPlane->getWidth();
  Int iHeight      = pcInputPlane->getHeight();

  assert( pcOutputPlane->getWidth () ==  iWidth       );
  assert( pcOutputPlane->getHeight() == (iHeight >> 1));
  assert (pcInputPlane ->getPad()    >=            12 );
  assert (pcOutputPlane->getPad()    >=         uiPad );

  Int iInputStride  = pcInputPlane->getStride();
  Int iOutputStride = pcOutputPlane->getStride();

  Pel* pcInputPlaneData  = pcInputPlane ->getPlaneData();
  Pel* pcOutputPlaneData = pcOutputPlane->getPlaneData();

  Int iStr0  = 0;
  Int iStr1  = iInputStride;
  Int iStr2  = iStr1  + iInputStride;
  Int iStr3  = iStr2  + iInputStride;
  Int iStr4  = iStr3  + iInputStride;
  Int iStr5  = iStr4  + iInputStride;
  Int iStr6  = iStr5  + iInputStride;
  Int iStr7  = iStr6  + iInputStride;
  Int iStr8  = iStr7  + iInputStride;
  Int iStr9  = iStr8  + iInputStride;
  Int iStr10 = iStr9  + iInputStride;
  Int iStr11 = iStr10 + iInputStride;
  Int iStr12 = iStr11 + iInputStride;;


  // Downsampling filter { 0, 2, 0, -4, -3, 5, 19, 26, 19, 5, -3, -4, 0, 2, 0, 64 };
  for ( Int iYPos = 0; iYPos < (iHeight >> 1); iYPos++)
  {
    Pel* pcTmpIn = pcInputPlaneData - 12 * iInputStride - uiPad;
    for ( Int iXPos = -uiPad; iXPos < iWidth + uiPad; iXPos++)
    {
      Int iTmp0, iTmp1, iTmp2, iTmp3, iTmp4, iTmp5;
      iTmp0 = pcTmpIn[iStr0] + pcTmpIn[iStr12];
      iTmp1 = pcTmpIn[iStr2] + pcTmpIn[iStr10];
      iTmp2 = pcTmpIn[iStr3] + pcTmpIn[iStr9 ];
      iTmp3 = pcTmpIn[iStr4] + pcTmpIn[iStr8 ];
      iTmp4 = pcTmpIn[iStr5] + pcTmpIn[iStr7 ];
      iTmp5 = pcTmpIn[iStr6];

      Int iSum = iTmp4 + iTmp3 - iTmp2 + ((iTmp0 + iTmp4 + iTmp5 - iTmp2) << 1) + ( ( iTmp3 - iTmp1)  << 2) + (  iTmp5 << 3 ) + (( iTmp4 + iTmp5 ) << 4);
      pcOutputPlaneData[ iXPos ] = (Pel) Clip((iSum + 32) >> 6);
      pcTmpIn++;
    }
    pcOutputPlaneData += iOutputStride;
    pcInputPlaneData  += (iInputStride << 1);
  }
};

Void TRenFilter::sampleHorDown2Tap13( PelImagePlane* pcInputPlane, PelImagePlane* pcOutputPlane, Int uiPad )
{ // DownSampling from JSVM Software (DownConvertStatic) ??
  Int iWidth       = pcInputPlane->getWidth();
  Int iHeight      = pcInputPlane->getHeight();

  assert( pcOutputPlane->getWidth () == (iWidth  >> 1));
  assert( pcOutputPlane->getHeight() ==  iHeight      );
  assert (pcInputPlane ->getPad()    >=            12 );
  assert (pcOutputPlane->getPad()    >=         uiPad );

  Int iInputStride  = pcInputPlane ->getStride();
  Int iOutputStride = pcOutputPlane->getStride();

  Pel* pcInputPlaneData  = pcInputPlane ->getPlaneData()- uiPad * iInputStride ;
  Pel* pcOutputPlaneData = pcOutputPlane->getPlaneData()- uiPad * iOutputStride;

  // Downsampling filter { 0, 2, 0, -4, -3, 5, 19, 26, 19, 5, -3, -4, 0, 2, 0, 64 };
  for ( Int iYPos = 0; iYPos < iHeight + 2*uiPad; iYPos++)
  {
    Pel* pcTmpIn = pcInputPlaneData - 12;
    for ( Int iXPos = 0; iXPos < ( iWidth >> 1); iXPos++)
    {
      Int iTmp0, iTmp1, iTmp2, iTmp3, iTmp4, iTmp5;
      iTmp0 = pcTmpIn[0]+ pcTmpIn[12];
      iTmp1 = pcTmpIn[2]+ pcTmpIn[10];
      iTmp2 = pcTmpIn[3]+ pcTmpIn[9 ];
      iTmp3 = pcTmpIn[4]+ pcTmpIn[8 ];
      iTmp4 = pcTmpIn[5]+ pcTmpIn[7 ];
      iTmp5 = pcTmpIn[6];

      Int iSum = iTmp4 + iTmp3 - iTmp2 + ((iTmp0 + iTmp4 + iTmp5 - iTmp2) << 1) + ( ( iTmp3 - iTmp1)  << 2) + (  iTmp5 << 3 ) + (( iTmp4 + iTmp5 ) << 4);
      pcOutputPlaneData[ iXPos ] = (Pel) Clip((iSum + 32) >> 6);
      pcTmpIn += 2;
    }
    pcOutputPlaneData += iOutputStride;
    pcInputPlaneData  += iInputStride ;
  }
};

inline Pel TRenFilter::xMedian3(Pel* pcData)
{
  Bool bGT01 = pcData[0] >  pcData[1];
  Bool bGT12 = pcData[1] >  pcData[2];
  Bool bGT20 = pcData[2] >  pcData[0];

  return ( (bGT01 && bGT20) || (!bGT01 && !bGT20) ) ?  pcData[0] : ( ( (bGT12 && bGT01) || (!bGT12 && !bGT01) ) ?  pcData[1] : pcData[2]) ;
}


Void TRenFilter::lineMedian3( PelImage* pcImage )
{

  PelImage* pcTemp = pcImage->create();

  for (UInt uiCurPlane = 0; uiCurPlane < pcImage->getNumberOfPlanes(); uiCurPlane++)
  {
    PelImagePlane* pcImPlane   = pcImage->getPlane(uiCurPlane);
    PelImagePlane* pcTempPlane = pcTemp ->getPlane(uiCurPlane);

    UInt uiWidth  = pcImPlane->getWidth();
    UInt uiHeight = pcImPlane->getHeight();

    Pel* pcImData   = pcImPlane  ->getPlaneData();
    Pel* pcTempData = pcTempPlane->getPlaneData();

    UInt uiImDataStride   = pcImPlane  ->getStride();
    UInt uiTempDataStride = pcTempPlane->getStride();

    for(UInt uiPosY = 0; uiPosY < uiHeight; uiPosY++)
    {
      for(UInt uiPosX = 0; uiPosX < uiWidth; uiPosX++)
      {
        if ( (uiPosX >= 1) && (uiPosX < (uiWidth - 2)) )
        {
          pcTempData[uiPosX] = xMedian3(pcImData + uiPosX - 1);
        }
        else
        {
          pcTempData[uiPosX] = pcImData[uiPosX];
        }
      }
      pcTempData += uiTempDataStride;
      pcImData   += uiImDataStride;
    }
  }

  pcImage->assign(pcTemp);
  delete pcTemp;
}


Void TRenFilter::convRect( PelImage* pcImage, UInt uiSize )
{
  DoubleImage cKernel(uiSize, uiSize,1,0);
  cKernel.getPlane(0)->assign( 1 / ( Double( uiSize )  * Double( uiSize) ));
  conv(pcImage, &cKernel);
}

Void TRenFilter::binominal( PelImage* pcInputImage, PelImage* pcOutputImage, UInt uiSize )
{
  assert( pcInputImage->getNumberOfFullPlanes()   == pcOutputImage->getNumberOfFullPlanes  () );
  assert( pcInputImage->getNumberOfQuaterPlanes() == pcOutputImage->getNumberOfQuaterPlanes() );

  UInt uiPlane;
  for (uiPlane = 0; uiPlane < pcInputImage->getNumberOfPlanes(); uiPlane ++)
  {
    binominal( pcInputImage->getPlane(uiPlane), pcOutputImage->getPlane(uiPlane), uiSize );
  }

  for (  ; uiPlane < pcInputImage->getNumberOfPlanes(); uiPlane ++)
  {
    binominal( pcInputImage->getPlane(uiPlane), pcOutputImage->getPlane(uiPlane), uiSize >> 1 );
 }
}

Void TRenFilter::binominal( PelImagePlane* pcInputPlane, PelImagePlane* pcOutputPlane, UInt uiSize )
{
  Int iWidth  = pcInputPlane ->getWidth ();
  Int iHeight = pcInputPlane ->getHeight();

  assert( pcOutputPlane->getWidth () == iWidth  );
  assert( pcOutputPlane->getHeight() == iHeight );
  assert( pcInputPlane ->getPad   () >= uiSize );
  assert( pcOutputPlane->getPad   () >= uiSize );

  if (uiSize == 0)
  {
    pcOutputPlane->assign( pcInputPlane );
    return;
  };

  Int iInputStride  = pcInputPlane ->getStride();
  Int iOutputStride = pcOutputPlane->getStride();
  Int iTempStride   = iWidth + (uiSize << 1);


  Pel* pcCurInputData  = pcInputPlane ->getPlaneData() - uiSize;
  Pel* pcTempData      = new Pel[iTempStride * iHeight];
  Pel* pcCurTempData   = pcTempData;

  Pel (*fpFilter) ( Pel*, Int ) = NULL;

  switch( uiSize )
  {
  case 1:
    fpFilter = &TRenFilter::xFiltBinom3;
    break;
  case 2:
    fpFilter = &TRenFilter::xFiltBinom5;
    break;
  case 3:
    fpFilter = &TRenFilter::xFiltBinom7;
    break;
  case 4:
    fpFilter = &TRenFilter::xFiltBinom9;
    break;
  default:
      AOT(true);
  }

  for (Int iPosY = 0; iPosY < iHeight; iPosY++ )
  {
    for (Int iPosX = 0; iPosX < iWidth + (uiSize << 1); iPosX++)
    {
      pcCurTempData[iPosX] = (*fpFilter)(pcCurInputData + iPosX, iInputStride );
    }
    pcCurTempData   += iTempStride;
    pcCurInputData  += iInputStride;
  }

  pcCurTempData   = pcTempData + uiSize;
  Pel* pcCurOutputData = pcOutputPlane->getPlaneData();

  for (Int iPosY = 0; iPosY < iHeight; iPosY++ )
  {
    for (Int iPosX = 0; iPosX < iWidth; iPosX++)
    {
      pcCurOutputData[iPosX] = (*fpFilter)(pcCurTempData + iPosX, 1);
    }
    pcCurTempData    += iTempStride;
    pcCurOutputData  += iOutputStride;
  }

  delete[] pcTempData;
}

Pel TRenFilter::xFiltBinom3( Pel* pcInputData, Int iStride )
{
  Int iSum = pcInputData[-1 * iStride ] + pcInputData[ 0 ] +  (pcInputData[iStride ] << 1 );
  return Clip( (iSum +  2) >>  2 );
}

Pel TRenFilter::xFiltBinom5( Pel* pcInputData, Int iStride )
{
  // { 1,4,6,4,1 }
  Int iStride0  = 0;
  Int iStrideM1 = iStride0  - iStride;
  Int iStrideM2 = iStrideM1 - iStride;
  Int iStrideP1 = iStride0  + iStride;
  Int iStrideP2 = iStrideP1 + iStride;

  Int iTmp0 = pcInputData[iStrideM2] + pcInputData[iStrideP2];
  Int iTmp1 = pcInputData[iStrideM1] + pcInputData[iStrideP1];
  Int iTmp2 = pcInputData[iStride0 ];

  Int iSum = iTmp0 +  (iTmp2 << 1) + ((iTmp1 + iTmp2) << 2);
  return Clip( (iSum +  8) >>  4 );
}

Pel TRenFilter::xFiltBinom7( Pel* pcInputData, Int iStride )
{
  // { 1,6,15,20,15,6,1 }
  Int iStride0  = 0;
  Int iStrideM1 = iStride0  - iStride;
  Int iStrideM2 = iStrideM1 - iStride;
  Int iStrideM3 = iStrideM1 - iStride;
  Int iStrideP1 = iStride0  + iStride;
  Int iStrideP2 = iStrideP1 + iStride;
  Int iStrideP3 = iStrideP1 + iStride;

  Int iTmp0 = pcInputData[iStrideM3] + pcInputData[iStrideP3];
  Int iTmp1 = pcInputData[iStrideM2] + pcInputData[iStrideP2];
  Int iTmp2 = pcInputData[iStrideM1] + pcInputData[iStrideP1];
  Int iTmp3 = pcInputData[iStride0];

  Int iSum = iTmp0 - iTmp2 + ( iTmp1  << 1) + ( (iTmp1 + iTmp3) << 2) + ((iTmp2 + iTmp3) << 4);

  return Clip( (iSum +  32) >>  6 );
}

Pel TRenFilter::xFiltBinom9( Pel* pcInputData, Int iStride )
{
  // {  1     8    28    56    70    56    28     8     1 }
  Int iStride0  = 0;
  Int iStrideM1 = iStride0  - iStride;
  Int iStrideM2 = iStrideM1 - iStride;
  Int iStrideM3 = iStrideM1 - iStride;
  Int iStrideM4 = iStrideM1 - iStride;
  Int iStrideP1 = iStride0  + iStride;
  Int iStrideP2 = iStrideP1 + iStride;
  Int iStrideP3 = iStrideP1 + iStride;
  Int iStrideP4 = iStrideP1 + iStride;

  Int iTmp0 = pcInputData[iStrideM4] + pcInputData[iStrideP4];
  Int iTmp1 = pcInputData[iStrideM3] + pcInputData[iStrideP3];
  Int iTmp2 = pcInputData[iStrideM2] + pcInputData[iStrideP2];
  Int iTmp3 = pcInputData[iStrideM1] + pcInputData[iStrideP1];
  Int iTmp4 = pcInputData[iStride0];

  Int iSum = iTmp0 + ((iTmp4 ) << 1) + ( ( iTmp4 - iTmp2 ) << 2) +  ( (iTmp1 - iTmp3) << 3 ) +  ((iTmp2 ) << 5) + ((iTmp3+ iTmp4 ) << 6);

  return Clip( (iSum +  128) >>  8 );
}


Pel TRenFilter::interpCHSpline(Double dX, Double dS0, Double dS1, Int iQ0, Int iQ1, Int iQ2, Int iQ3)
{
  Double dSq = (dX - dS0) / (dS1 - dS0);

  Double adP[4];
  Double dSqP2 = dSq * dSq;
  Double dSqP3 = dSqP2 * dSq;

  adP[0] = 1 - 3 * dSqP2 + 2 * dSqP3;
  adP[1] = dSq - 2 * dSqP2 + dSqP3;
  adP[2] = 3 * dSqP2 - 2 * dSqP3;
  adP[3] = -dSqP2 + dSqP3;

  Double dQ  = adP[0] * iQ0 + adP[1] * iQ1 + adP[2] * iQ2 + adP[3] * iQ3 ;

  Pel cQ = (Pel) ( dQ + 0.5);

  cQ = ( cQ < 0   ? 0   : cQ );
  cQ = ( cQ > 255 ? 255 : cQ );

  return cQ;

}

Void TRenFilter::diffHorSym(PelImage* pcInputImage, IntImage* pcOutputImage)
{
  for (UInt uiCurPlane = 0; uiCurPlane < pcInputImage->getNumberOfPlanes(); uiCurPlane++)
  {
    diffHorSym( pcInputImage->getPlane(uiCurPlane), pcOutputImage->getPlane(uiCurPlane));
  };
}

Void TRenFilter::diffHorSym(PelImagePlane* pcInputPlane, IntImagePlane* pcOutputPlane)
{
  UInt uiInputStride = pcInputPlane ->getStride();
  UInt uiOutputStride = pcOutputPlane->getStride();
  UInt uiWidth        = pcInputPlane ->getWidth();
  UInt uiHeight       = pcInputPlane ->getHeight();

  Pel*   pcInputData   = pcInputPlane ->getPlaneData();
  Int*   piOutputData  = pcOutputPlane->getPlaneData();

  for (UInt uiPosY = 0; uiPosY < uiHeight; uiPosY++)
  {
    for (UInt uiPosX = 1; uiPosX < uiWidth-1; uiPosX++)
    {
      piOutputData[uiPosX] = ((Int) pcInputData[uiPosX+1] - (Int) pcInputData[uiPosX-1]);
      piOutputData[uiPosX] /= 2;
    };

    piOutputData[0] = piOutputData[1];
    piOutputData[uiWidth-1] = piOutputData[uiWidth-2];
    pcInputData += uiInputStride;
    piOutputData  += uiOutputStride;

  };
}

Void TRenFilter::laplace( DoubleImage* pcInputImage, DoubleImage* pcOutputImage )
{
  for (UInt uiCurPlane = 0; uiCurPlane < pcInputImage->getNumberOfPlanes(); uiCurPlane++)
  {
    DoubleImagePlane* pcInputPlane  = pcInputImage  ->getPlane(uiCurPlane);
    DoubleImagePlane* pcOutputPlane = pcOutputImage ->getPlane(uiCurPlane);

    UInt uiWidth  = pcInputPlane->getWidth();
    UInt uiHeight = pcInputPlane->getHeight();

    Double* pdInputData  = pcInputPlane  ->getPlaneData();
    Double* pdOutputData = pcOutputPlane ->getPlaneData();

    for (UInt uiPosY = 1; uiPosY < uiHeight-1; uiPosY++)
    {
      UInt uOff = uiPosY * uiWidth;
      for(UInt uiPosX = 1; uiPosX < uiWidth-1; uiPosX++)
      {
        UInt uOff2 = uOff + uiPosX;
        pdOutputData[uOff2] =     4 * pdInputData[uOff2]
        -   pdInputData[uOff2 - 1]
        -   pdInputData[uOff2 + 1]
        -   pdInputData[uOff2 - uiWidth]
        -   pdInputData[uOff2 + uiWidth];
      }
    };

    // left and right margin
    for (UInt uiPosY = 1; uiPosY < uiHeight-1; uiPosY++)
    {
      UInt uOff  = uiPosY * uiWidth;
      pdOutputData[uOff] = 3 * pdInputData[uOff]
      -   pdInputData[uOff + 1]
      -   pdInputData[uOff - uiWidth]
      -   pdInputData[uOff + uiWidth];


      uOff = (uiPosY + 1) * uiWidth - 1;
      pdOutputData[uOff] = 3 * pdInputData[uOff]
      -   pdInputData[uOff - 1]
      -   pdInputData[uOff - uiWidth]
      -   pdInputData[uOff + uiWidth];
    }

    for (UInt uiPosX = 1; uiPosX < uiWidth-1; uiPosX++)
    {
      UInt uOff  = uiPosX;
      pdOutputData[uOff] = 3 * pdInputData[uOff]
      -   pdInputData[uOff + 1]
      -   pdInputData[uOff - 1]
      -   pdInputData[uOff + uiWidth];


      uOff = (uiHeight - 1) * uiWidth + uiPosX;
      pdOutputData[uOff] = 3 * pdInputData[uOff]
      -   pdInputData[uOff + 1]
      -   pdInputData[uOff - 1]
      -   pdInputData[uOff - uiWidth];
    }

    UInt uOff = 0;
    pdOutputData[uOff] = 2 * pdInputData[uOff] - pdInputData[uOff+1] - pdInputData[ uOff + uiWidth];
    uOff = uiWidth - 1;
    pdOutputData[uOff] = 2 * pdInputData[uOff] - pdInputData[uOff-1] - pdInputData[ uOff + uiWidth ];
    uOff = (uiHeight - 1) * uiWidth;
    pdOutputData[uOff] = 2 * pdInputData[uOff] - pdInputData[uOff+1] - pdInputData[ uOff - uiWidth];
    uOff = uiHeight * uiWidth - 1;
    pdOutputData[uOff] = 2 * pdInputData[uOff] - pdInputData[uOff-1] - pdInputData[ uOff - uiWidth];

  }
}


Void TRenFilter::conv( PelImage* pcImage, DoubleImage* pcKernel )
{
  PelImage* pcTemp = pcImage->create();

  DoubleImagePlane* pcKernelPlane = 0;
  for (UInt uiCurPlane = 0; uiCurPlane < pcImage->getNumberOfPlanes(); uiCurPlane++) {

    PelImagePlane* pcPlane     = pcImage->getPlane(uiCurPlane);
    PelImagePlane* pcTempPlane = pcTemp ->getPlane(uiCurPlane);

    if ( uiCurPlane <= pcKernel->getNumberOfPlanes() )
    {
      pcKernelPlane = pcKernel->getPlane(uiCurPlane);
    };

    UInt uiWidth  = pcPlane->getWidth();
    UInt uiHeight = pcPlane->getHeight();

    UInt uiKernelWidth  = pcKernelPlane->getWidth();
    UInt uiKernelHeight = pcKernelPlane->getHeight();

    Pel*    pcData         = pcPlane      ->getPlaneData();
    Pel*    pcTempData     = pcTempPlane  ->getPlaneData();
    Double* pdKernelData   = pcKernelPlane->getPlaneData();

    UInt uiDataStride       = pcPlane      ->getStride();
    UInt uiTempDataStride   = pcTempPlane  ->getStride();
    UInt uiKernelDataStride = pcKernelPlane->getStride();

    for(UInt uiPosY = 0; uiPosY < uiHeight; uiPosY++)
    {
      UInt uOff = uiPosY * uiTempDataStride;
      for(UInt uiPosX = 0; uiPosX < uiWidth; uiPosX++)
      {
        Double dSum = 0;
        for(UInt uKY = 0; uKY < uiKernelHeight; uKY++)
        {
          UInt uKOff = uKY * uiKernelDataStride;

          Int iYSrc = uiPosY - (uiKernelHeight/2) + uKY;

          if (iYSrc < 0)
            iYSrc = -iYSrc;

          if (iYSrc >= (Int)uiHeight)
            iYSrc = 2*uiHeight - iYSrc - 1;

          UInt uSrcOff = iYSrc * uiDataStride;

          for(UInt uKX = 0; uKX < uiKernelWidth; uKX++)
          {
            Int iXSrc = uiPosX - (uiKernelWidth/2) + uKX;

            if (iXSrc < 0)
              iXSrc = -iXSrc;

            if (iXSrc >= (Int)uiWidth)
              iXSrc = 2*uiWidth - iXSrc - 1;

            dSum += pcData[uSrcOff + iXSrc] * pdKernelData[uKOff + uKX];
          }
        }
        pcTempData[uOff + uiPosX] = (Pel) (dSum + ( ( dSum < 0 ) ? -0.5 : 0.5)  );
      }
    }
  }

  pcImage->assign(pcTemp);
  delete pcTemp;
}


// Horizontal Up sampling luma
Void TRenFilter::sampleHorUp( Int iLog2HorSampFac, Pel* pcInputPlaneData, Int iInputStride, Int iInputWidth, Int iHeight, Pel* pcOutputPlaneData, Int iOutputStride  )
{
  TComPredFilter cFilter;
  switch ( iLog2HorSampFac )
  {
  case 0:
    xDistributeArray              ( pcInputPlaneData, iInputStride, 1 , 1, iInputWidth, iHeight, pcOutputPlaneData, iOutputStride, 1, 1 );
    break;
  case 1:
    xDistributeArray              ( pcInputPlaneData, iInputStride, 1 , 1, iInputWidth, iHeight, pcOutputPlaneData, iOutputStride, 2, 1 );
    cFilter.xCTI_FilterHalfHor    ( pcInputPlaneData, iInputStride,     1, iInputWidth, iHeight, iOutputStride, 2, ++pcOutputPlaneData );
    break;
  case 2:
    xDistributeArray              ( pcInputPlaneData, iInputStride, 1 , 1, iInputWidth, iHeight, pcOutputPlaneData, iOutputStride, 4, 1 );
    cFilter.xCTI_FilterQuarter0Hor( pcInputPlaneData, iInputStride, 1, iInputWidth, iHeight, iOutputStride, 4, ++pcOutputPlaneData );
    cFilter.xCTI_FilterHalfHor    ( pcInputPlaneData, iInputStride, 1, iInputWidth, iHeight, iOutputStride, 4, ++pcOutputPlaneData );
    cFilter.xCTI_FilterQuarter1Hor( pcInputPlaneData, iInputStride, 1, iInputWidth, iHeight, iOutputStride, 4, ++pcOutputPlaneData );
    break;
  }
}

// horizontal up sampling chroma
Void TRenFilter::sampleCHorUp(Int iLog2HorSampFac, Pel* pcInputPlaneData, Int iInputStride, Int iInputWidth, Int iHeight, Pel* pcOutputPlaneData, Int iOutputStride  )
{
  switch ( iLog2HorSampFac )
  {
  case 0:
    xDistributeArray( pcInputPlaneData,   iInputStride  , 1, 1, iInputWidth,   iHeight   , pcOutputPlaneData                  , iOutputStride, 1 , 1 );
    break;
  case 1:
    xDistributeArray( pcInputPlaneData,   iInputStride  , 1, 1, iInputWidth,   iHeight   , pcOutputPlaneData                  , iOutputStride, 2 , 1 );
    xInterpHorChroma( pcInputPlaneData  , iInputStride  , 1, 1, iInputWidth,   iHeight   , pcOutputPlaneData                +1, iOutputStride, 2 , 1, &TComPredFilter::xCTI_Filter_VPS04_C_HAL );
    break;
  case 2:
    xDistributeArray( pcInputPlaneData,   iInputStride  , 1, 1, iInputWidth,   iHeight   , pcOutputPlaneData                  , iOutputStride, 4 , 1 );
    xInterpHorChroma( pcInputPlaneData  , iInputStride  , 1, 1, iInputWidth,   iHeight   , pcOutputPlaneData                +1, iOutputStride, 4 , 1, &TComPredFilter::xCTI_Filter_VP04_C_QUA0 );
    xInterpHorChroma( pcInputPlaneData  , iInputStride  , 1, 1, iInputWidth,   iHeight   , pcOutputPlaneData                +2, iOutputStride, 4 , 1, &TComPredFilter::xCTI_Filter_VPS04_C_HAL );
    xInterpHorChroma( pcInputPlaneData  , iInputStride  , 1, 1, iInputWidth,   iHeight   , pcOutputPlaneData                +3, iOutputStride, 4 , 1, &TComPredFilter::xCTI_Filter_VP04_C_QUA1 );
    break;
  }
}

Void TRenFilter::sampleCUpHorUp( Int iLog2HorSampFac, Pel* pcInputPlaneData, Int iInputStride, Int iInputWidth, Int iHeight, Pel* pcOutputPlaneData, Int iOutputStride  )
{

  switch ( iLog2HorSampFac )
  {
  case 0:
    xDistributeArray( pcInputPlaneData-1, iInputStride  , 1, 1, iInputWidth+3, iHeight   , pcOutputPlaneData                -2, iOutputStride, 2,  2 );
    xInterpVerChroma( pcInputPlaneData-1, iInputStride  , 1, 1, iInputWidth+3, iHeight   , pcOutputPlaneData+1*iOutputStride-2, iOutputStride, 2 , 2, &TComPredFilter::xCTI_Filter_VPS04_C_HAL );
    xInterpHorChroma( pcOutputPlaneData , iOutputStride , 2, 1, iInputWidth,   iHeight*2 , pcOutputPlaneData+1                , iOutputStride, 2 , 1, &TComPredFilter::xCTI_Filter_VPS04_C_HAL );
    break;
  case 1:
    xDistributeArray( pcInputPlaneData-1, iInputStride  , 1, 1, iInputWidth+3, iHeight   , pcOutputPlaneData                -4, iOutputStride, 4 , 2 );
    xInterpVerChroma( pcInputPlaneData-1, iInputStride  , 1, 1, iInputWidth+3, iHeight   , pcOutputPlaneData+1*iOutputStride-4, iOutputStride, 4 , 2, &TComPredFilter::xCTI_Filter_VPS04_C_HAL );
    xInterpHorChroma( pcOutputPlaneData , iOutputStride , 4, 1, iInputWidth, iHeight*2 , pcOutputPlaneData                +1, iOutputStride, 4 , 1, &TComPredFilter::xCTI_Filter_VP04_C_QUA0 );
    xInterpHorChroma( pcOutputPlaneData , iOutputStride , 4, 1, iInputWidth, iHeight*2 , pcOutputPlaneData                +2, iOutputStride, 4 , 1, &TComPredFilter::xCTI_Filter_VPS04_C_HAL );
    xInterpHorChroma( pcOutputPlaneData , iOutputStride , 4, 1, iInputWidth, iHeight*2 , pcOutputPlaneData                +3, iOutputStride, 4 , 1, &TComPredFilter::xCTI_Filter_VP04_C_QUA1 );
    break;
  case 2:
    xDistributeArray( pcInputPlaneData-1, iInputStride  , 1, 1, iInputWidth+3, iHeight   , pcOutputPlaneData                -8, iOutputStride, 8 , 2 );
    xInterpVerChroma( pcInputPlaneData-1, iInputStride  , 1, 1, iInputWidth+3, iHeight   , pcOutputPlaneData+1*iOutputStride-8, iOutputStride, 8 , 2, &TComPredFilter::xCTI_Filter_VPS04_C_HAL );
    xInterpHorChroma( pcOutputPlaneData , iOutputStride , 8, 1, iInputWidth,   iHeight*2 , pcOutputPlaneData                +1, iOutputStride, 8 , 1, &TComPredFilter::xCTI_Filter_VP04_C_OCT0 );
    xInterpHorChroma( pcOutputPlaneData , iOutputStride , 8, 1, iInputWidth,   iHeight*2 , pcOutputPlaneData                +2, iOutputStride, 8 , 1, &TComPredFilter::xCTI_Filter_VP04_C_QUA0 );
    xInterpHorChroma( pcOutputPlaneData , iOutputStride , 8, 1, iInputWidth,   iHeight*2 , pcOutputPlaneData                +3, iOutputStride, 8 , 1, &TComPredFilter::xCTI_Filter_VP04_C_OCT1 );
    xInterpHorChroma( pcOutputPlaneData , iOutputStride , 8, 1, iInputWidth,   iHeight*2 , pcOutputPlaneData                +4, iOutputStride, 8 , 1, &TComPredFilter::xCTI_Filter_VPS04_C_HAL );
    xInterpHorChroma( pcOutputPlaneData , iOutputStride , 8, 1, iInputWidth,   iHeight*2 , pcOutputPlaneData                +5, iOutputStride, 8 , 1, &TComPredFilter::xCTI_Filter_VP04_C_OCT2 );
    xInterpHorChroma( pcOutputPlaneData , iOutputStride , 8, 1, iInputWidth,   iHeight*2 , pcOutputPlaneData                +6, iOutputStride, 8 , 1, &TComPredFilter::xCTI_Filter_VP04_C_QUA1 );
    xInterpHorChroma( pcOutputPlaneData , iOutputStride , 8, 1, iInputWidth,   iHeight*2 , pcOutputPlaneData                +7, iOutputStride, 8 , 1, &TComPredFilter::xCTI_Filter_VP04_C_OCT3 );
    break;
  }
}


// Down Sampling
// Down sample luma
Void TRenFilter::sampleHorDown(Int iLog2HorSampFac,  Pel* pcInputPlaneData, Int iInputStride, Int iInputWidth, Int iHeight, Pel* pcOutputPlaneData, Int iOutputStride  )
{
  switch ( iLog2HorSampFac )
  {
  case 0:
    xDistributeArray( pcInputPlaneData, iInputStride, 1, 1, iInputWidth,iHeight, pcOutputPlaneData, iOutputStride, 1 , 1 );
    break;
  case 1:
    xSampleDownHor2(pcInputPlaneData, iInputStride, iInputWidth, iHeight, pcOutputPlaneData, iOutputStride);
    break;
  case 2:
    xSampleDownHor4(pcInputPlaneData, iInputStride, iInputWidth, iHeight, pcOutputPlaneData, iOutputStride);
    break;
  }
}


Void TRenFilter::sampleCHorDown(Int iLog2HorSampFac,  Pel* pcInputPlaneData, Int iInputStride, Int iInputWidth, Int iHeight, Pel* pcOutputPlaneData, Int iOutputStride  )
{
  //GT: currently the same as for luma
  sampleHorDown( iLog2HorSampFac, pcInputPlaneData, iInputStride, iInputWidth, iHeight, pcOutputPlaneData, iOutputStride);
}



// Up sampling chroma
Void TRenFilter::sampleCDownHorDown( Int iLog2HorSampFac,  Pel* pcInputPlaneData, Int iInputStride, Int iInputWidth, Int iInputHeight, Pel* pcOutputPlaneData, Int iOutputStride  )
{
  // create buffer
  Int iBufferStride   = iInputWidth >> (iLog2HorSampFac + 1);
  Pel* piBuffer       = new Pel[ iBufferStride * (iInputHeight+2) ];

  switch ( iLog2HorSampFac )
  {
  case 0:
    xSampleDownHor2( pcInputPlaneData - iInputStride,  iInputStride, iInputWidth  , iInputHeight+1, piBuffer,  iBufferStride);
    break;
  case 1:
    xSampleDownHor4( pcInputPlaneData - iInputStride , iInputStride,  iInputWidth , iInputHeight+1, piBuffer, iBufferStride);
    break;
  case 2:
    xSampleDownHor8( pcInputPlaneData - iInputStride , iInputStride,  iInputWidth  , iInputHeight+1, piBuffer, iBufferStride);
    break;
  }
  xSampleDownVer2( piBuffer + iBufferStride       , iBufferStride, iBufferStride, iInputHeight,   pcOutputPlaneData, iOutputStride);
  delete[] piBuffer;
}

Void TRenFilter::xDistributeArray(Pel* pcSrc, Int iSrcStride, Int iSrcStepX, Int iSrcStepY, Int iWidth, Int iHeight, Pel* pcDst, Int iDstStride, Int iDstStepX, Int iDstStepY)
{
  iDstStride *= iDstStepY;
  iSrcStride *= iSrcStepY;
  for (Int iYPos = 0; iYPos < iHeight; iYPos++ )
  {
    Pel* pcCurDst = pcDst;
    Pel* pcCurSrc  = pcSrc;
    for (Int iXPos = 0; iXPos < iWidth; iXPos ++)
    {
      *pcCurDst = *pcCurSrc;

      pcCurDst += iDstStepX;
      pcCurSrc += iSrcStepX;
    }
    pcDst  += iDstStride;
    pcSrc  += iSrcStride;
  }
}


Void TRenFilter::xInterpHorChroma( Pel* piSrc, Int iSrcStride, Int iSrcStepX, Int iSrcStepY, Int iWidth, Int iHeight, Pel* piDst, Int iDstStride, Int iDstStepX, Int iDstStepY, FpChromaIntFilt fpFilter )
{
  Int   iSum;
  Pel*  piSrcTmp;

  TComPredFilter cFilter;
  for ( Int y = iHeight; y != 0; y-- )
  {
    piSrcTmp = piSrc - iSrcStepX;
    for ( Int x = 0; x < iWidth; x++ )
    {
      iSum      = (cFilter.*fpFilter)( piSrcTmp,  iSrcStepX );
      piDst[x * iDstStepX ] =  Clip ((iSum +  32) >>  6 );
      piSrcTmp+= iSrcStepX;
    }
    piSrc += iSrcStride * iSrcStepY;
    piDst += iDstStride * iDstStepY;
  }
}

Void TRenFilter::xInterpVerChroma( Pel* piSrc, Int iSrcStride, Int iSrcStepX, Int iSrcStepY, Int iWidth, Int iHeight, Pel* piDst, Int iDstStride, Int iDstStepX, Int iDstStepY, FpChromaIntFilt fpFilter )
{
  Int   iSum;
  Pel*  piSrcTmp;

  TComPredFilter cFilter;
  for ( Int y = iHeight; y != 0; y-- )
  {
    piSrcTmp = piSrc - iSrcStepY * iSrcStride;
    for ( Int x = 0; x < iWidth; x++ )
    {
      iSum      = (cFilter.*fpFilter)( piSrcTmp,  iSrcStepY * iSrcStride );
      piDst[x * iDstStepX ]  =  Clip ((iSum +  32) >>  6 );
      piSrcTmp += iSrcStepX;
    }
    piSrc += iSrcStride * iSrcStepY;
    piDst += iDstStride * iDstStepY;
  }
}


Void TRenFilter::xSampleDownHor2( Pel* piSrc, Int iSrcStride, Int iSrcWidth, Int iHeight, Pel* piDst, Int iDstStride  )
{
  Int   iSum;
  Pel*  piSrcTmp;


  for ( Int y = iHeight; y != 0; y-- )
  {
    piSrcTmp = piSrc - 1 ;
    for ( Int x = 0; x < (iSrcWidth >> 1); x++ )
    {
      // { 1,2,1 }
      iSum = piSrcTmp[0] + piSrcTmp[2] +  (piSrcTmp[1] << 1);
      piDst[x] = Clip( (iSum +  2) >>  2 );
      piSrcTmp += 2;
    }
    piSrc += iSrcStride;
    piDst += iDstStride;
  }
};

Void TRenFilter::xSampleDownVer2( Pel* piSrc, Int iSrcStride, Int iSrcWidth, Int iSrcHeight, Pel* piDst, Int iDstStride  )
{
  Int   iSum;
  Pel*  piSrcTmp;

  for ( Int y = (iSrcHeight >> 1); y != 0; y-- )
  {
    piSrcTmp = piSrc -1 * iSrcStride;
    for ( Int x = 0; x < iSrcWidth; x++ )
    {
      // { 1,2,1 }
      iSum = piSrcTmp[0] + piSrcTmp[ iSrcStride << 1] +  (piSrcTmp[ iSrcStride ] << 1);
      piDst[x] = Clip( (iSum +  2) >>  2 );
      piSrcTmp += 1;
    }
    piSrc += (iSrcStride << 1);
    piDst += iDstStride;
  }
};

Void TRenFilter::xSampleDownHor4( Pel* piSrc, Int iSrcStride, Int iSrcWidth, Int iHeight, Pel* piDst, Int iDstStride  )
{
  Int   iSum;
  Pel*  piSrcTmp;

  Int iTmp0, iTmp1, iTmp2;

  for ( Int y = iHeight; y != 0; y-- )
  {
    piSrcTmp = piSrc -2 ;
    for ( Int x = 0; x < (iSrcWidth >> 2); x++ )
    {
      // { 1,4,6,4,1 }
      iTmp0 = piSrcTmp[0] + piSrcTmp[4];
      iTmp1 = piSrcTmp[1] + piSrcTmp[3];
      iTmp2 = piSrcTmp[2];

      iSum = iTmp0 +  (iTmp2 << 1) + ((iTmp1 + iTmp2) << 2);
      piDst[x] = Clip( (iSum +  8) >>  4 );
      piSrcTmp += 4;
    }
    piSrc += iSrcStride;
    piDst += iDstStride;
  }
};

Void TRenFilter::xSampleDownHor8( Pel* piSrc, Int iSrcStride, Int iSrcWidth, Int iHeight, Pel* piDst, Int iDstStride  )
{
  Int   iSum;
  Pel*  piSrcTmp;

  Int iTmp0, iTmp1, iTmp2, iTmp3;

  for ( Int y = iHeight; y != 0; y-- )
  {
    piSrcTmp = piSrc -3;
    for ( Int x = 0; x < (iSrcWidth >> 3); x++ )
    {
      // { 1,6,15,20,15,6,1 }
      iTmp0 = piSrcTmp[0] + piSrcTmp[6];
      iTmp1 = piSrcTmp[1] + piSrcTmp[5];
      iTmp2 = piSrcTmp[2] + piSrcTmp[4];
      iTmp3 = piSrcTmp[3];

      iSum = iTmp0 - iTmp2 + ( iTmp1  << 1) + ( (iTmp1 + iTmp3) << 2) + ((iTmp2 + iTmp3) << 4);
      piDst[x] = Clip( (iSum +  32) >>  6 );
      piSrcTmp += 8;
    }
    piSrc += iSrcStride;
    piDst += iDstStride;
  }
};

Void TRenFilter::xDilate( Pel* piSrc, Int iSrcStride, Int iWidth, Int iHeight, Pel* piDst, Int iDstStride, Int iSize, Bool bVerticalDir, Bool bToTopOrLeft )
{
  Int iFDimStart   = 0;
  Int iInc         = 1;
  Int iSDimStart   = 0;

  Int iFDimSrcStrd = bVerticalDir ? 1          : iSrcStride;
  Int iFDimDstStrd = bVerticalDir ? 1          : iDstStride;

  Int iSDimSrcStrd = bVerticalDir ? iSrcStride : 1;
  Int iSDimDstStrd = bVerticalDir ? iDstStride : 1;

  Int iFDimEnd     = bVerticalDir ? iWidth -1  : iHeight - 1;
  Int iSDimEnd     = bVerticalDir ? iHeight-1  : iWidth  - 1;

  if ( bToTopOrLeft )
  {
    iSDimStart    = iSDimEnd;
    iSDimEnd      = 0;
    iInc         *= -1;
  }

  for (Int iPosFDim = iFDimStart; iPosFDim <= iFDimEnd; iPosFDim++ )
  {
    Int  iCount      = 0;
    Bool bLastWasOne = false;
    Bool bDilate     = false;
    Int  iPosSDim    = iSDimStart;
    Bool bContinue   = true;

    while ( bContinue )
    {
      if ( iCount == iSize )
      {
        iCount  = 0;
        bDilate = false;
      }

      Pel iVal = piSrc[iPosSDim*iSDimSrcStrd];
      if( iVal == 0 && bLastWasOne )
      {
        iCount  = 0;
        bDilate = true;
      }

      if( bDilate )
      {
        piDst[iPosSDim*iSDimDstStrd] = REN_USED_PEL;
        iCount++;
      }
      else
      {
        piDst[iPosSDim*iSDimDstStrd] = iVal;
      }


      bLastWasOne = (iVal == REN_USED_PEL);
      bContinue   = (iPosSDim != iSDimEnd);
      iPosSDim    += iInc;
    }

    piSrc += iFDimSrcStrd;
    piDst += iFDimDstStrd;
  }
};


template Bool TRenFilter::compare   (TRenImage<Pel     >*, TRenImage<Pel>*      );
template Bool TRenFilter::compare   (TRenImagePlane<Pel>*, TRenImagePlane<Pel>* );

template Void TRenFilter::mirrorHor(        TRenImage<Double>        *pcImage );
template Void TRenFilter::mirrorHor(        TRenImage<Pel>           *pcImage );
template Void TRenFilter::mirrorHor(        TRenImage<Int>           *pcImage );
template Void TRenFilter::mirrorHor(        TRenImagePlane<Pel>      *pcImagePlane );
