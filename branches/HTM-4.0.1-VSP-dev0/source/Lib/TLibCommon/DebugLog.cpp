#include "CommonDef.h"
#include "TComYuv.h"
#include "TComPic.h"
#include "../TLibDecoder/TDecCu.h"
#include "DebugLog.h"


char* pcPartSizeName[] =
{
  "SIZE_2Nx2N",           ///< symmetric motion partition,  2Nx2N
  "SIZE_2NxN",            ///< symmetric motion partition,  2Nx N
  "SIZE_Nx2N",            ///< symmetric motion partition,   Nx2N
  "SIZE_NxN",             ///< symmetric motion partition,   Nx N

  "SIZE_2NxnU",           ///< asymmetric motion partition, 2Nx( N/2) + 2Nx(3N/2)
  "SIZE_2NxnD",           ///< asymmetric motion partition, 2Nx(3N/2) + 2Nx( N/2)
  "SIZE_nLx2N",           ///< asymmetric motion partition, ( N/2)x2N + (3N/2)x2N
  "SIZE_nRx2N"            ///< asymmetric motion partition, (3N/2)x2N + ( N/2)x2N
};

char* pcPredModeName[] = 
{
  "MODE_SKIP",            ///< SKIP mode
  "MODE_INTER",           ///< inter-prediction mode
  "MODE_INTRA"            ///< intra-prediction mode
#if FORCE_REF_VSP==1
  ,"MODE_SYNTH"           ///< vsp skip mode
#endif
};

void printMotionInfo( TComDataCU* pcCU, UInt uiIdx, FILE* pfFile )
{
  UInt iInterDir = pcCU->getInterDir( uiIdx );
  fprintf(pfFile, "interDir,%d, ", iInterDir );
  if (iInterDir != 2) {
    fprintf(pfFile, "L0Idx,%d, ", pcCU->getCUMvField(REF_PIC_LIST_0)->getRefIdx( uiIdx ));
    fprintf(pfFile, "L0Mv(,%d,%d,), ", pcCU->getCUMvField(REF_PIC_LIST_0)->getMv( uiIdx ).getHor(), pcCU->getCUMvField(REF_PIC_LIST_0)->getMv( uiIdx ).getVer() );
  } else {
    fprintf(pfFile, "L0Idx,-, L0Mv(,-,-,), ");
  }
  if (iInterDir != 1) {
    fprintf(pfFile, "L1Idx,%d, ", pcCU->getCUMvField(REF_PIC_LIST_1)->getRefIdx( uiIdx ));
    fprintf(pfFile, "L1Mv(,%d,%d,), ", pcCU->getCUMvField(REF_PIC_LIST_1)->getMv( uiIdx ).getHor(), pcCU->getCUMvField(REF_PIC_LIST_1)->getMv( uiIdx ).getVer() );
  } else {
    fprintf(pfFile, "L1Idx,-, L1Mv(,-,-,), ");
  }
}

bool DebugLog::DebugLogOut( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth )
{
  if ( !m_pfDebugLogFile ) return false;

  int iIdxWidth = (pcCU->getWidth( uiAbsPartIdx ) << uiDepth) >> 2;
  int iXoffset  = (g_auiZscanToRaster[uiAbsPartIdx] % iIdxWidth) << 2;
  int iYoffset  = (g_auiZscanToRaster[uiAbsPartIdx] / iIdxWidth) << 2;
  fprintf(m_pfDebugLogFile, "Pos(,%d,%d,) , PartIdx,%d, ", pcCU->getCUPelX()+iXoffset, pcCU->getCUPelY()+iYoffset, uiAbsPartIdx);
#if (VSP_N)
  {
    char cRefVsp = 0; Int aiRefIdx[2] = {-1, -1};
    Int iRefIdx = pcCU->getCUMvField( RefPicList(0) )->getRefIdx( uiAbsPartIdx ); aiRefIdx[0] = iRefIdx;
    if( iRefIdx >= 0 ) cRefVsp |= (pcCU->getSlice()->getRefViewId( RefPicList(0), iRefIdx ) == NUM_VIEW_VSP ? 0x01 : 0);
    iRefIdx = pcCU->getCUMvField( RefPicList(1) )->getRefIdx( uiAbsPartIdx ); aiRefIdx[1] = iRefIdx;
    if( iRefIdx >= 0 ) cRefVsp |= (pcCU->getSlice()->getRefViewId( RefPicList(1), iRefIdx ) == NUM_VIEW_VSP ? 0x02 : 0);
    fprintf(m_pfDebugLogFile, "VSP,%d, RefIdx(,%d,%d,), ", cRefVsp, aiRefIdx[0], aiRefIdx[1] ); // 0:not VSP, 1:VSP L0, 2:VSP L1, 3:VSP Bi
    fprintf(m_pfDebugLogFile, "MV(");
    if(aiRefIdx[0]>=0)fprintf(m_pfDebugLogFile, "(,%d,%d,)",pcCU->getCUMvField( RefPicList(0) )->getMv(uiAbsPartIdx).getHor(),pcCU->getCUMvField( RefPicList(0) )->getMv(uiAbsPartIdx).getVer());
    else              fprintf(m_pfDebugLogFile, "(,-,-,)");
    if(aiRefIdx[1]>=0)fprintf(m_pfDebugLogFile, "(,%d,%d,)",pcCU->getCUMvField( RefPicList(1) )->getMv(uiAbsPartIdx).getHor(),pcCU->getCUMvField( RefPicList(1) )->getMv(uiAbsPartIdx).getVer());
    else              fprintf(m_pfDebugLogFile, "(,-,-,)");
    fprintf(m_pfDebugLogFile, ")");
  }
#endif
    fprintf(m_pfDebugLogFile, "depth,%d,(,%d x %d,), ", uiDepth, pcCU->getWidth( uiAbsPartIdx ), pcCU->getHeight( uiAbsPartIdx ));
    fprintf(m_pfDebugLogFile, "Predmode,%s, ", 
#if FORCE_REF_VSP==1
    pcCU->getPredictionMode( uiAbsPartIdx ) < 4 ? 
#else
    pcCU->getPredictionMode( uiAbsPartIdx ) < 3 ? 
#endif
    pcPredModeName[(int)pcCU->getPredictionMode( uiAbsPartIdx )] : "SIZE_NONE");
#if FORCE_REF_VSP==1
    fprintf(m_pfDebugLogFile, "VspSkipFlg,%d,", pcCU->getPredictionMode( uiAbsPartIdx )==MODE_SYNTH?1:0);
#endif
#if HHI_MPI
    fprintf(m_pfDebugLogFile, "TextModeDepFlg,%d,", pcCU->getSlice()->getIsDepth() ? (pcCU->getTextureModeDepth( uiAbsPartIdx )==uiDepth) : 0 );
#endif
    fprintf(m_pfDebugLogFile, "Mergeflag,%d,MergeIdx,%d,", pcCU->getMergeFlag( uiAbsPartIdx ), pcCU->getMergeIndex( uiAbsPartIdx ) );
    fprintf(m_pfDebugLogFile, "Partsize,%s, ", pcCU->getPartitionSize( uiAbsPartIdx ) < 8 ? pcPartSizeName[(int)pcCU->getPartitionSize( uiAbsPartIdx )] : "SIZE_NONE");
    fprintf(m_pfDebugLogFile, "Cbf(,%d,%d,%d,), ", pcCU->getCbf(uiAbsPartIdx, TEXT_LUMA), pcCU->getCbf(uiAbsPartIdx, TEXT_CHROMA_U), pcCU->getCbf(uiAbsPartIdx, TEXT_CHROMA_V) );

#if OUT_PRED_INFO
  UInt uiPartOffset = ( pcCU->getPic()->getNumPartInCU() >> ( pcCU->getDepth(uiAbsPartIdx) << 1 ) ) >> 2;
  if (pcCU->getPredictionMode( uiAbsPartIdx ) != MODE_INTRA) {
    //InterDirectionNum,L0Idx,L0Mv,L1Idx,L1Mv
    printMotionInfo( pcCU, uiAbsPartIdx, m_pfDebugLogFile );
    switch ( pcCU->getPartitionSize( uiAbsPartIdx ) )
    {
      case SIZE_2Nx2N:{ break; }
      case SIZE_2NxN: { printMotionInfo( pcCU, uiAbsPartIdx + (uiPartOffset << 1), m_pfDebugLogFile );    break; }
      case SIZE_Nx2N: { printMotionInfo( pcCU, uiAbsPartIdx +  uiPartOffset      , m_pfDebugLogFile );    break; }
      case SIZE_NxN:  
      {
        for ( Int iPartIdx = 0; iPartIdx < 4; iPartIdx++ ) {
          printMotionInfo( pcCU, uiAbsPartIdx + uiPartOffset * iPartIdx, m_pfDebugLogFile );
        }
        break;
      }
#if ASYMMETRIC_USE
      case SIZE_2NxnU:{ printMotionInfo( pcCU, uiAbsPartIdx + (uiPartOffset >> 1)                      , m_pfDebugLogFile );    break; }
      case SIZE_2NxnD:{ printMotionInfo( pcCU, uiAbsPartIdx + (uiPartOffset << 1) + (uiPartOffset >> 1), m_pfDebugLogFile ); break; }
      case SIZE_nLx2N:{ printMotionInfo( pcCU, uiAbsPartIdx + (uiPartOffset >> 2)                      , m_pfDebugLogFile );    break; }
      case SIZE_nRx2N:{ printMotionInfo( pcCU, uiAbsPartIdx + (uiPartOffset     ) + (uiPartOffset >> 2), m_pfDebugLogFile ); break; }
#endif
      default:        { break; }
    }
  } else {
    //IntraPredDirection
    fprintf( m_pfDebugLogFile, "IntraDir(L,%d,C,%d,), ", pcCU->getLumaIntraDir( uiAbsPartIdx ), pcCU->getChromaIntraDir( uiAbsPartIdx ) );
    if (pcCU->getPartitionSize( uiAbsPartIdx ) == SIZE_NxN)
    {
      for (int i=1; i < 4; i++) {
        fprintf( m_pfDebugLogFile, "IntraDir(L,%d,C,%d,), ", pcCU->getLumaIntraDir( uiAbsPartIdx + uiPartOffset * i ), pcCU->getChromaIntraDir( uiAbsPartIdx + uiPartOffset * i ) );
      }
    }
  }
#endif
  fprintf(m_pfDebugLogFile, "\n");
  return true;
}


//Color set function of TComYuv 
#if DEBUGIMGOUT
Void TComYuv::colsetToPicYuv   ( const UChar ucColor[3], Int iOffset[2][2], TComPicYuv* pcPicYuvDst, UInt iCuAddr, UInt uiAbsZorderIdx, UInt uiPartDepth, UInt uiPartIdx, Bool bDrawLine )
{
  colsetToPicLuma  ( ucColor[0],             iOffset, pcPicYuvDst, iCuAddr, uiAbsZorderIdx, uiPartDepth, uiPartIdx, bDrawLine );
  for (int i=0; i < 2; i++)
    for (int j=0; j < 2; j++)
      iOffset[i][j] >>= 1;
  colsetToPicChroma( ucColor[1], ucColor[2], iOffset, pcPicYuvDst, iCuAddr, uiAbsZorderIdx, uiPartDepth, uiPartIdx, bDrawLine );
}

Void TComYuv::colsetToPicLuma  ( const UChar ucLumaOffCol, Int iOffset[2][2], TComPicYuv* pcPicYuvDst, UInt iCuAddr, UInt uiAbsZorderIdx, UInt uiPartDepth, UInt uiPartIdx, Bool bDrawLine )
{
  Int  offset  = (g_uiBitIncrement>0)?(1<<(g_uiBitIncrement-1)):0;
  Int  iStartX = bDrawLine ? 1:0;
  Int  y, x, iWidth, iHeight;
  iWidth  = m_iWidth >>uiPartDepth;
  iHeight = m_iHeight>>uiPartDepth;

  Pel* pSrc     = getLumaAddr(uiPartIdx, iWidth);
  Pel* pDst     = pcPicYuvDst->getLumaAddr ( iCuAddr, uiAbsZorderIdx );

  UInt  iSrcStride  = getStride();
  UInt  iDstStride  = pcPicYuvDst->getStride();

  pSrc += iOffset[0][1] * iSrcStride + iOffset[0][0]; //TopLeft
  pDst += iOffset[0][1] * iDstStride + iOffset[0][0]; //TopLeft
  iWidth  = iOffset[1][0];  //ButtomRight
  iHeight = iOffset[1][1];  //ButtomRight


  if( bDrawLine )
  {
    for( x = 0; x < iWidth; x++ )
      pDst[x] = (((pSrc[x] + offset) >> g_uiBitIncrement) >> 2) + LINE_LUMA_OFF_COL2;  //Top line
    pDst += iDstStride;
    pSrc += iSrcStride;
    iHeight--;
  }

  if( ucLumaOffCol == 0 )
  {
    for ( y = iHeight; y != 0; y-- )
    {
      if( bDrawLine )
        pDst[0] =  (((pSrc[0] + offset) >> g_uiBitIncrement) >> 2) + LINE_LUMA_OFF_COL2;  //Left line
      for( x = iStartX; x < iWidth; x++ )
        pDst[x] = (pSrc[x] + offset) >> g_uiBitIncrement;  //glay scale
      pDst += iDstStride;
      pSrc += iSrcStride;
    }
  } else {
    for ( y = iHeight; y != 0; y-- )
    {
      if( bDrawLine )
        pDst[0] =  (((pSrc[0] + offset) >> g_uiBitIncrement) >> 2) + LINE_LUMA_OFF_COL2;  //Left line
      for( x = iStartX; x < iWidth; x++ )
        pDst[x] = (((pSrc[x] + offset) >> g_uiBitIncrement) >> 2) + ucLumaOffCol;
      pDst += iDstStride;
      pSrc += iSrcStride;
    }
  }
}

Void TComYuv::colsetToPicChroma( const UChar ucUCol, const UChar ucVCol, Int iOffset[2][2], TComPicYuv* pcPicYuvDst, UInt iCuAddr, UInt uiAbsZorderIdx, UInt uiPartDepth, UInt uiPartIdx, Bool bDrawLine )
{
  Int  y, x, iWidth, iHeight;
  iWidth  = m_iCWidth >>uiPartDepth;
  iHeight = m_iCHeight>>uiPartDepth;
  Int  iStartX = bDrawLine ? 1:0;

  Pel* pDstU      = pcPicYuvDst->getCbAddr( iCuAddr, uiAbsZorderIdx );
  Pel* pDstV      = pcPicYuvDst->getCrAddr( iCuAddr, uiAbsZorderIdx );

  UInt  iDstStride = pcPicYuvDst->getCStride();

  pDstU += iOffset[0][1] * iDstStride + iOffset[0][0]; //TopLeft
  pDstV += iOffset[0][1] * iDstStride + iOffset[0][0]; //TopLeft
  iWidth  = iOffset[1][0];  //ButtomRight
  iHeight = iOffset[1][1];  //ButtomRight

  if( bDrawLine )
  {
    for( x = 0; x < iWidth; x++ )
    {
      pDstU[x] = LINE_CB_COL2;  //Top line color
      pDstV[x] = LINE_CR_COL2;  //Top line color
    }
    pDstV += iDstStride;
    pDstU += iDstStride;
    iHeight--;
  }
  for ( y = iHeight; y != 0; y-- )
  {
    if( bDrawLine ) {
      pDstU[0] =  LINE_CB_COL2;  //Left line color
      pDstV[0] =  LINE_CR_COL2;  //Left line color
    }
    for( x = iStartX; x < iWidth; x++ )
    {
      pDstU[x] = ucUCol;
      pDstV[x] = ucVCol;
    }
    pDstU += iDstStride;
    pDstV += iDstStride;
  }
}

Void TComYuv::drawLineToPicYuv ( bool bHor, bool bVer, UInt uiSize, TComPicYuv* pcPicYuvDst, UInt iCuAddr, UInt uiAbsZorderIdx, UInt uiPartDepth, UInt uiPartIdx )
{
  Int  offset  = (g_uiBitIncrement>0)?(1<<(g_uiBitIncrement-1)):0;
  Int  y, x, iWidth, iHeight, iWidthC, iHeightC;

  iWidth   = m_iWidth >>uiPartDepth;
  iHeight  = m_iHeight>>uiPartDepth;
  iWidthC  = m_iCWidth >>uiPartDepth;
  iHeightC = m_iCHeight>>uiPartDepth;


  Pel* pSrc     = getLumaAddr(uiPartIdx, iWidth);
  Pel* pDst     = pcPicYuvDst->getLumaAddr ( iCuAddr, uiAbsZorderIdx );
  Pel* pDstU    = pcPicYuvDst->getCbAddr( iCuAddr, uiAbsZorderIdx );
  Pel* pDstV    = pcPicYuvDst->getCrAddr( iCuAddr, uiAbsZorderIdx );

  UInt  iSrcStride  = getStride();
  UInt  iDstStride  = pcPicYuvDst->getStride();
  UInt  iDstStrideC = pcPicYuvDst->getCStride();

  if ( bHor )
  {
    //Luma
    Pel* pDstHor = pDst + iDstStride * ((iHeight * uiSize) >> 2);
    Pel* pSrcHor = pSrc + iSrcStride * ((iHeight * uiSize) >> 2);
    for( x = 1; x < iWidth; x++ )
      pDstHor[x] = (((pSrcHor[x] + offset) >> g_uiBitIncrement) >> 2) + LINE_LUMA_OFF_COL;
    //CbCr
    Pel* pDstUHor = pDstU + iDstStrideC * ((iHeightC * uiSize) >> 2);
    Pel* pDstVHor = pDstV + iDstStrideC * ((iHeightC * uiSize) >> 2);
    for( x = 1; x < iWidthC; x++ )
    {
      pDstUHor[x] = LINE_CB_COL;
      pDstVHor[x] = LINE_CR_COL;
    }
  }
  if ( bVer )
  {
    //Luma
    Pel* pDstVer = pDst + iDstStride;
    Pel* pSrcVer = pSrc + iSrcStride;
    x = (iWidth * uiSize) >> 2;
    for ( y = iHeight; y != 1; y-- )
    {
      pDstVer[x] = (((pSrcVer[x] + offset) >> g_uiBitIncrement) >> 2) + LINE_LUMA_OFF_COL;
      pDstVer += iDstStride;
      pSrcVer += iSrcStride;
    }
    //CbCr
    Pel* pDstVerU = pDstU + iDstStrideC;
    Pel* pDstVerV = pDstV + iDstStrideC;
    x = (iHeightC * uiSize) >> 2;
    for ( y = iHeightC; y != 1; y-- )
    {
      pDstVerU[x] = LINE_CB_COL;
      pDstVerV[x] = LINE_CR_COL;
      pDstVerU += iDstStrideC;
      pDstVerV += iDstStrideC;
    }
  }
  
}

Void TComYuv::drawRectToPicYuv ( TComPicYuv* pcPicYuvDst, UInt iCuAddr, UInt uiAbsZorderIdx, UInt uiPartDepth, UInt uiPartIdx )
{
  Int  offset  = (g_uiBitIncrement>0)?(1<<(g_uiBitIncrement-1)):0;
  Int  y, x, iWidth, iHeight, iWidthC, iHeightC;

  iWidth   = m_iWidth >>uiPartDepth;
  iHeight  = m_iHeight>>uiPartDepth;
  iWidthC  = m_iCWidth >>uiPartDepth;
  iHeightC = m_iCHeight>>uiPartDepth;


  Pel* pSrc     = getLumaAddr(uiPartIdx, iWidth);
  Pel* pDst     = pcPicYuvDst->getLumaAddr ( iCuAddr, uiAbsZorderIdx );
  Pel* pDstU    = pcPicYuvDst->getCbAddr( iCuAddr, uiAbsZorderIdx );
  Pel* pDstV    = pcPicYuvDst->getCrAddr( iCuAddr, uiAbsZorderIdx );

  UInt  iSrcStride  = getStride();
  UInt  iDstStride  = pcPicYuvDst->getStride();
  UInt  iDstStrideC = pcPicYuvDst->getCStride();

  //X
  {
    //Luma
    Pel* pDstHor = pDst;
    Pel* pSrcHor = pSrc;
    for( x = 0; x < iWidth; x++ )
      pDstHor[x] = (((pSrcHor[x] + offset) >> g_uiBitIncrement) >> 2) + LINE_LUMA_OFF_COL2;
  //CbCr
    Pel* pDstUHor = pDstU;
    Pel* pDstVHor = pDstV;
    for( x = 0; x < iWidthC; x++ )
    {
      pDstUHor[x] = LINE_CB_COL2;
      pDstVHor[x] = LINE_CR_COL2;
    }
  }

  //Y
  {
    //Luma
    Pel* pDstVer = pDst;
    Pel* pSrcVer = pSrc;
    x = 0;
    for ( y = iHeight; y >= 0; y-- )
    {
      pDstVer[x] = (((pSrcVer[x] + offset) >> g_uiBitIncrement) >> 2) + LINE_LUMA_OFF_COL2;
      pDstVer += iDstStride;
      pSrcVer += iSrcStride;
    }
    //CbCr
    Pel* pDstVerU = pDstU;
    Pel* pDstVerV = pDstV;
    x = 0;
    for ( y = iHeightC; y >= 0; y-- )
    {
      pDstVerU[x] = LINE_CB_COL2;
      pDstVerV[x] = LINE_CR_COL2;
      pDstVerU += iDstStrideC;
      pDstVerV += iDstStrideC;
    }
  }
  
}

Void TDecCu::xColsetToPic( TComDataCU* pcCU, TComPicYuv* pcPicYuv, UInt uiZorderIdx, UInt uiDepth )
{
  int       iIdxWidth = (pcCU->getWidth( 0 ) << uiDepth) >> 2;
  UInt      uiCUAddr  = pcCU->getAddr();
  PredMode  ePredMode = pcCU->getPredictionMode( 0 );
  Int       iRect[2][2] = {{0,0}, {0,0}};
  Int       iColorIdx = 0;
  UInt      uiPartAddr;

  if ( ePredMode == MODE_INTER )
  {
    for (int iPartIdx = 0; iPartIdx < pcCU->getNumPartInter(); iPartIdx++ )
    {
      pcCU->getPartIndexAndSize( iPartIdx, uiPartAddr, iRect[1][0], iRect[1][1] );
      iRect[0][0]  = (g_auiZscanToRaster[uiPartAddr] % iIdxWidth) << 2;
      iRect[0][1]  = (g_auiZscanToRaster[uiPartAddr] / iIdxWidth) << 2;

#if (VSP_N)
      {
        Char cRefVsp = 0x00;
        Int iRefIdx = pcCU->getCUMvField( RefPicList(0) )->getRefIdx(iPartIdx);
        if( iRefIdx >= 0 && pcCU->getSlice()->getRefViewId( RefPicList(0), iRefIdx ) == NUM_VIEW_VSP )
          cRefVsp |= 0x01;
        iRefIdx = pcCU->getCUMvField( RefPicList(1) )->getRefIdx(iPartIdx);
        if( iRefIdx >= 0 && pcCU->getSlice()->getRefViewId( RefPicList(1), iRefIdx ) == NUM_VIEW_VSP )
          cRefVsp |= 0x02;
#if FORCE_REF_VSP
        if( pcCU->isVspMode(iPartIdx) )
          cRefVsp |= 0x04;
#endif

        switch( cRefVsp )
        {
        case 0: // INTER
          iColorIdx = 2;
          break;
        case 1: // VSP L0
          iColorIdx = 4;
          break;
        case 2: // VSP L1
          iColorIdx = 6;
          break;
        case 3: // VSP Bi
          iColorIdx = 7;
          break;
#if FORCE_REF_VSP
        case 4: // VSP Skip
          iColorIdx = 5;
          break;
#endif
        default:
          assert(0);
          break;
        }
      }
#else
      iColorIdx = 2;
#endif

      m_ppcYuvReco[uiDepth]->colsetToPicYuv    ( g_ucCol[iColorIdx], iRect, pcPicYuv, uiCUAddr, uiZorderIdx );
    }
  } else {
    iRect[0][0]=iRect[0][1]=0;
    iRect[1][0]=pcCU->getWidth(0);
    iRect[1][1]=pcCU->getHeight(0);

    iColorIdx = ePredMode==MODE_SKIP ? 0 : 1;

    m_ppcYuvReco[uiDepth]->colsetToPicYuv      ( g_ucCol[iColorIdx], iRect, pcPicYuv, uiCUAddr, uiZorderIdx );
  }

  switch ( pcCU->getPartitionSize( 0 ) )
  {
    case SIZE_2Nx2N:
      m_ppcYuvReco[uiDepth]->drawLineToPicYuv  ( false, false, 4, pcPicYuv, uiCUAddr, uiZorderIdx );
      break;
    case SIZE_2NxN:
      m_ppcYuvReco[uiDepth]->drawLineToPicYuv  ( true , false, 2, pcPicYuv, uiCUAddr, uiZorderIdx );
      break;
    case SIZE_Nx2N:
      m_ppcYuvReco[uiDepth]->drawLineToPicYuv  ( false, true , 2, pcPicYuv, uiCUAddr, uiZorderIdx );
      break;
    case SIZE_NxN:
      m_ppcYuvReco[uiDepth]->drawLineToPicYuv  ( true , true , 2, pcPicYuv, uiCUAddr, uiZorderIdx );
      break; 
    case SIZE_2NxnU:
      m_ppcYuvReco[uiDepth]->drawLineToPicYuv  ( true , false, 1, pcPicYuv, uiCUAddr, uiZorderIdx );
      break; 
    case SIZE_2NxnD:
      m_ppcYuvReco[uiDepth]->drawLineToPicYuv  ( true , false, 3, pcPicYuv, uiCUAddr, uiZorderIdx );
      break; 
    case SIZE_nLx2N:
      m_ppcYuvReco[uiDepth]->drawLineToPicYuv  ( false, true , 1, pcPicYuv, uiCUAddr, uiZorderIdx );
      break; 
    case SIZE_nRx2N:
      m_ppcYuvReco[uiDepth]->drawLineToPicYuv  ( false, true , 3, pcPicYuv, uiCUAddr, uiZorderIdx );
      break;
    default:
      break;
  }
}

Void TDecCu::xColsetToPicMerge( TComDataCU* pcCU, TComPicYuv* pcPicYuv, UInt uiZorderIdx, UInt uiDepth )
{
  int       iIdxWidth = (pcCU->getWidth( 0 ) << uiDepth) >> 2;
  UInt      uiCUAddr  = pcCU->getAddr();
  PredMode  ePredMode = pcCU->getPredictionMode( 0 );
  Int       iRect[2][2] = {{0,0}, {0,0}};
  Int       iColorIdx = 0;
  UInt      uiPartAddr;
  UInt      uiMergeIdx;
  Bool      bSkip     = ePredMode == MODE_SKIP ? true : false;

  if ( ePredMode == MODE_INTER || bSkip )
  {
    for (int iPartIdx = 0; iPartIdx < pcCU->getNumPartInter(); iPartIdx++ )
    {
      pcCU->getPartIndexAndSize( iPartIdx, uiPartAddr, iRect[1][0], iRect[1][1] );
      iRect[0][0]  = (g_auiZscanToRaster[uiPartAddr] % iIdxWidth) << 2;
      iRect[0][1]  = (g_auiZscanToRaster[uiPartAddr] / iIdxWidth) << 2;
      Bool bMerge  = pcCU->getMergeFlag( iPartIdx );

      if( bSkip || bMerge ) {
        uiMergeIdx   = pcCU->getMergeIndex( iPartIdx );
#if (FORCE_REF_VSP)
        {
          iColorIdx = bSkip ? 1 : (bMerge ? 5 : 0);
          Int iRefIdx = pcCU->getCUMvField( RefPicList(0) )->getRefIdx(iPartIdx);
          char cRefVsp = 0x00;
          if( iRefIdx >= 0 && pcCU->getSlice()->getRefViewId( RefPicList(0), iRefIdx ) == NUM_VIEW_VSP )
            cRefVsp |= 0x01;
          iRefIdx = pcCU->getCUMvField( RefPicList(1) )->getRefIdx(iPartIdx);
          if( iRefIdx >= 0 && pcCU->getSlice()->getRefViewId( RefPicList(1), iRefIdx ) == NUM_VIEW_VSP )
            cRefVsp |= 0x02;

          if( cRefVsp ) {
            iColorIdx += 1;
          } else if ( uiMergeIdx ) {
            iColorIdx += 3;
          }
        }
#else
        iColorIdx = 2;
#endif
        assert( iColorIdx > 0 && iColorIdx < 10 );

        m_ppcYuvReco[uiDepth]->colsetToPicYuv  ( g_ucCol[iColorIdx], iRect, pcPicYuv, uiCUAddr, uiZorderIdx );

      } else {

        //Inter Not Merge
        m_ppcYuvReco[uiDepth]->colsetToPicYuv  ( g_ucCol[0], iRect, pcPicYuv, uiCUAddr, uiZorderIdx );

      }
    } // end iPartIdx loop

  } else
  {
    //Intra , Not Merge Inter
    iRect[0][0]=iRect[0][1]=0;
    iRect[1][0]=pcCU->getWidth(0);
    iRect[1][1]=pcCU->getHeight(0);

    iColorIdx = 0;

    m_ppcYuvReco[uiDepth]->colsetToPicYuv      ( g_ucCol[iColorIdx], iRect, pcPicYuv, uiCUAddr, uiZorderIdx );
  }

  switch ( pcCU->getPartitionSize( 0 ) )
  {
    case SIZE_2Nx2N:
      m_ppcYuvReco[uiDepth]->drawLineToPicYuv  ( false, false, 4, pcPicYuv, uiCUAddr, uiZorderIdx );
      break;
    case SIZE_2NxN:
      m_ppcYuvReco[uiDepth]->drawLineToPicYuv  ( true , false, 2, pcPicYuv, uiCUAddr, uiZorderIdx );
      break;
    case SIZE_Nx2N:
      m_ppcYuvReco[uiDepth]->drawLineToPicYuv  ( false, true , 2, pcPicYuv, uiCUAddr, uiZorderIdx );
      break;
    case SIZE_NxN:
      m_ppcYuvReco[uiDepth]->drawLineToPicYuv  ( true , true , 2, pcPicYuv, uiCUAddr, uiZorderIdx );
      break; 
    case SIZE_2NxnU:
      m_ppcYuvReco[uiDepth]->drawLineToPicYuv  ( true , false, 1, pcPicYuv, uiCUAddr, uiZorderIdx );
      break; 
    case SIZE_2NxnD:
      m_ppcYuvReco[uiDepth]->drawLineToPicYuv  ( true , false, 3, pcPicYuv, uiCUAddr, uiZorderIdx );
      break; 
    case SIZE_nLx2N:
      m_ppcYuvReco[uiDepth]->drawLineToPicYuv  ( false, true , 1, pcPicYuv, uiCUAddr, uiZorderIdx );
      break; 
    case SIZE_nRx2N:
      m_ppcYuvReco[uiDepth]->drawLineToPicYuv  ( false, true , 3, pcPicYuv, uiCUAddr, uiZorderIdx );
      break;
    default:
      break;
  }
}
#endif //DEBUGIMGOUT
