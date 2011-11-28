

// Include files
#include "TComMVDRefData.h"
#include "TComPicYuv.h"
#include <math.h>
#include <vector>
#include <iostream>
#include <fstream>

Void TComMVDRefData::setPicYuvBaseView( InterViewReference eView, Bool bDepth, TComPicYuv* pcOrgView, TComPicYuv* pcRecView )
{
  if ( bDepth )
  {
    m_apcDepthRec[eView] = pcRecView;
    m_apcDepthOrg[eView] = pcOrgView;
  }
  else
  {
    m_apcVideoRec[eView] = pcRecView;
    m_apcVideoOrg[eView] = pcOrgView;
  }
}

Void TComMVDRefData::setShiftLUTsBaseView( InterViewReference eView, Double** adInLut, Int** aiInLut )
{
  m_aiBaseViewShiftLUT[eView] = aiInLut;
  m_adBaseViewShiftLUT[eView] = adInLut;
}

TComMVDRefData::TComMVDRefData()
{
//GT VSO
  m_adERViewShiftLUT = 0;
  m_aiERViewShiftLUT = 0;
  m_apcExternalReferenceViews.resize(0);
//GT VSO end

  for (UInt uiView = 0; uiView < 3; uiView++)
  {
    m_apcVideoOrg[uiView] = NULL;
    m_apcDepthOrg[uiView] = NULL;
    m_apcVideoRec[uiView] = NULL;
    m_apcDepthRec[uiView] = NULL;

   //GT VSO
    m_adBaseViewShiftLUT [uiView] = NULL;
    m_aiBaseViewShiftLUT [uiView] = NULL;
    //GT VSO end
  };
}

//GT VSO
Void TComMVDRefData::getRefPicYuvAndLUT( TComPicYuv**& rpacDistRefPicList, Int***& rppaiShiftLut )
{
  //GT

  UInt uiNextEntry = 0;
  for ( UInt uiViewIdx = 0; uiViewIdx < m_aiExtViewRefInd.size(); uiViewIdx++ )
  {
    rppaiShiftLut[uiNextEntry]       = m_aiERViewShiftLUT         [ m_aiExtViewRefLUTInd[uiViewIdx] ];
    rpacDistRefPicList[uiNextEntry]  = m_apcExternalReferenceViews[ m_aiExtViewRefInd   [uiViewIdx] ];
    uiNextEntry++;
  }

  for ( UInt uiViewIdx = 0; uiViewIdx < m_aiBaseViewRefInd.size(); uiViewIdx++ )
  {
    rppaiShiftLut[uiNextEntry]      = m_aiBaseViewShiftLUT[ m_aiBaseViewRefInd[uiViewIdx] + 1 ];
    rpacDistRefPicList[uiNextEntry] = getPicYuvRecVideo( (InterViewReference) (m_aiBaseViewRefInd[uiViewIdx] + 1) ) ?  getPicYuvRecVideo( (InterViewReference) (m_aiBaseViewRefInd[uiViewIdx] + 1) ) : getPicYuvOrgVideo( (InterViewReference) (m_aiBaseViewRefInd[uiViewIdx] + 1) );
    uiNextEntry++;
  }
}

Void TComMVDRefData::getRefPicYuvAndLUTMode1( TComPicYuv**& rpacDistRefPicList, Int***& rppaiShiftLut )
{
  rppaiShiftLut[0] = m_aiBaseViewShiftLUT[ PREVVIEW ];
  rppaiShiftLut[1] = NULL;
  rppaiShiftLut[2] = m_aiBaseViewShiftLUT[ NEXTVIEW ];

  //GT: Only orginals of this view needed for Mode 1;
  rpacDistRefPicList[0] = NULL;
  rpacDistRefPicList[1] = NULL;
  rpacDistRefPicList[2] = NULL;
}
//GT VSO end




//Void TComMVDRefData::render( TComPicYuv* pOut )
////Void TComMVDRefData::render( TComPicYuv* pIn, TComPicYuv* pOut, TComPicYuv* pDepth, Long** aalLUT )
//{
//
//  std::cout << "InRender" << std::endl;
//
//  TComPicYuv* pIn    = getPicYuvOrgVideo(CURRVIEW);
//  //  TComPicYuv* pOut   = getRecVideo(CURRVIEW);
//  TComPicYuv* pDepth = getPicYuvOrgDepth(CURRVIEW);
//
//  Int** aaiLUT = getShiftLUTIERView(0);
//
//  Int iHeight = (Int) pIn->getHeight();
//  Int iWidth  = (Int) pIn->getWidth();
//
//  Pel* piSourceLumaData = pIn   ->getLumaAddr();
//  Pel* piTargetLumaData = pOut  ->getLumaAddr();
//  Pel* piDepthData      = pDepth->getLumaAddr();
//
//  Int iSourceLumaStride = pIn   ->getStride();
//  Int iTargetLumaStride = pOut  ->getStride();
//  Int      iDepthStride = pDepth->getStride();
//
//
//  for (Int iY = 0; iY < iHeight; iY++)
//  {
//    for (Int iX = 0; iX < iWidth; iX++)
//    {
//      piTargetLumaData[iX] = 0;
//    }
//    piTargetLumaData += iTargetLumaStride;
//  }
//
//  piTargetLumaData = pOut  ->getLumaAddr();
//
//  Int iNewPos;
//  for (Int iY = 0; iY < iHeight; iY++)
//  {
//    for (Int iX = 0; iX < iWidth; iX++)
//    {
//      iNewPos = iX - aaiLUT[0][(piDepthData[iX] >> g_uiBitIncrement)];
//      iNewPos = (iNewPos <  0     ) ? 0          : iNewPos;
//      iNewPos = (iNewPos >= iWidth) ? iWidth - 1 : iNewPos;
//
//      piTargetLumaData[iNewPos] = piSourceLumaData[iX];
//    }
//    piSourceLumaData += iSourceLumaStride;
//    piTargetLumaData += iTargetLumaStride;
//    piDepthData      += iDepthStride;
//  }
//}
//
//
//
//
//TComRendererModel::TComRendererModel()
//{
//  m_pcInputSamples  = NULL;
//  m_pcOutputSamples = NULL;
//  m_ppiShiftLUT    = NULL;
//}
//
//TComRendererModel::~TComRendererModel()
//{
//  if ( m_pcOutputSamples )
//    delete[] m_pcOutputSamples;
//
//  if ( m_pcInputSamples )
//    delete[] m_pcInputSamples;
//
//}
//
//Void TComRendererModel::createModel( TComPicYuv* pcYuvDepth, TComPicYuv* pcYuvVideo, TComPicYuv* pcYuvRef, Int** ppiShiftLUT )
//{
//  createModel(pcYuvDepth->getLumaAddr(), pcYuvDepth->getStride(), pcYuvVideo->getLumaAddr(), pcYuvVideo->getStride(), pcYuvVideo->getCbAddr(), pcYuvVideo->getCrAddr(), pcYuvVideo->getCStride(), pcYuvVideo->getWidth(), pcYuvVideo->getHeight(), ppiShiftLUT );
//}
//
//Void TComRendererModel::createModel( TComPicYuv* pcYuvDepth, TComPicYuv* pcYuvVideo, Int** ppiShiftLUT )
//{
//
//}
//
//Void TComRendererModel::createModel( Pel* pcPelDisp, Int iStrideDisp, Pel* pcPelY, Int iStrideY, Pel* pcPelU, Pel* pcPelV, Int iStrideC, Int iWidth, Int iHeight, Int** ppiShiftLUT )
//{
//
//}
//
//Void TComRendererModel::xCreateRenderedView( Pel* pcPelDisp, Int iDispStride, Pel* pcPelY, Int iLStride, Pel* pcPelU, Pel* pcPelV, Int iCStride, Int iWidth, Int iHeight, Int** ppiShiftLUT,Bool bSAD )
//{
//  m_ppiShiftLUT = ppiShiftLUT;
//  m_iWidth       = iWidth;
//  m_iHeight      = iHeight;
//  m_iStride      = iWidth;
//
//  m_pcOutputSamples = new RendererOutputSample[ iWidth * iHeight];
//  m_pcInputSamples  = new RendererInputSample [ iWidth * iHeight];
//
//  RendererInputSample*  pcInputSamples  = m_pcInputSamples ;
//  RendererOutputSample* pcOutputSamples = m_pcOutputSamples ;
//
//  for ( Int iRow = 0; iRow < iHeight; iRow++ )
//  {
//    for ( Int iCol = 0; iCol < iWidth; iCol++ )
//    {
//      pcInputSamples[iCol].m_uiPosX = iCol;
//      pcInputSamples[iCol].m_uiY    = pcPelY[ iCol ];
//      pcInputSamples[iCol].m_uiU    = pcPelU[ iCol >> 1 ];
//      pcInputSamples[iCol].m_uiV    = pcPelV[ iCol >> 1 ];
//      pcOutputSamples[iCol].m_iErr  = 0;
//
//      pcInputSamples[iCol].m_iPosShifted = iCol + m_ppiShiftLUT[0][ pcPelDisp[ iCol ] ];
//      pcInputSamples[iCol].m_uiDisp      = pcPelDisp[ iCol ];
//
//    }
//
//    pcInputSamples  += m_iStride;
//    pcOutputSamples += m_iStride;
//    pcPelY          += iLStride;
//    pcPelDisp       += iDispStride;
//
//    if ( iRow & 1 )
//    {
//      pcPelU += iCStride;
//      pcPelV += iCStride;
//    }
//
//  }
//
//  m_lErr = 0;
//  Long lDump;
//  changeModel(0,0,m_iWidth, m_iHeight, pcPelDisp, iDispStride, lDump, bSAD);
//}
//
//Void TComRendererModel::changeModel( Int iPosX, Int iPosY, Int iWidth, Int iHeight, Pel* pcPelDisp, Int iDispStride, Long& rlErr, Bool bSAD )
//{
//  RendererInputSample*  pcInputSamples  = m_pcInputSamples ;
//  RendererOutputSample* pcOutputSamples = m_pcOutputSamples ;
//
//  Int iMinChangedPosition = 0;
//  Int iMaxChangedPosition = m_iWidth - 1;
//
//  rlErr = 0;
//
//  for ( Int iRow = iPosX; iRow < iHeight; iRow++ )
//  {
//    // Add and Remove samples (Update Model)
//    for ( Int iCol = iPosY; iCol < iWidth; iCol++ )
//    {
//      // Remove from old position
//      Int iOldPosShifted  = pcInputSamples[iCol].m_iPosShifted;
//
//      iMinChangedPosition = Min( iOldPosShifted, iMinChangedPosition );
//      iMaxChangedPosition = Max( iOldPosShifted, iMaxChangedPosition );
//
//      pcOutputSamples[ iOldPosShifted ].removeInputSample( &(pcInputSamples[iCol]) );
//
//      // Add to new position
//      Int iShift = m_ppiShiftLUT[0][ pcPelDisp[ iCol ] ];
//
//      Int iPosShifted = iPosX + iShift;
//
//      iPosShifted = Min(Max(iPosShifted, 0), m_iWidth - 1);
//      iMinChangedPosition = Min(iPosShifted, iMinChangedPosition);
//      iMaxChangedPosition = Max(iPosShifted, iMaxChangedPosition);
//
//      pcInputSamples[iCol].m_uiDisp = pcPelDisp[ iCol ];
//      pcInputSamples[iCol].m_iPosShifted = iPosShifted;
//
//      pcOutputSamples[ iPosShifted ].addInputSample( &(pcInputSamples[iCol]) );
//    }
//
//    // ReRender
//
//    // Get Top Plane
//    Int iMaxInd = -1;
//
//    if ( pcOutputSamples[iMinChangedPosition].m_pcTopSample )
//    {
//      iMaxInd = pcOutputSamples[iMinChangedPosition].m_pcTopSample->m_uiPosX;
//    }
//
//    if ( iMinChangedPosition > 0 && pcOutputSamples[iMinChangedPosition].m_pcTopSample )
//    {
//      iMaxInd = Max( iMaxInd, pcOutputSamples[iMinChangedPosition-1].m_pcTopSample->m_uiPosX );
//    }
//
//    for ( Int iCol = iMinChangedPosition; iCol <= iMaxChangedPosition; iCol++ )
//    {
//      if ( pcOutputSamples[iCol].m_pcTopSample == NULL )
//      {
//        if ( pcOutputSamples[iCol+1].m_pcTopSample == NULL )
//        {
//          // Hole Filling
//          Int iFillStartCol = iCol;
//          iCol++;
//
//          while( ( pcOutputSamples[iCol].m_pcTopSample == NULL ) && (iMinChangedPosition < iMaxChangedPosition) ) iCol++;
//
//          for ( int iFillCol = iFillStartCol; iFillCol < iCol; iFillCol++ )
//          {
//            pcOutputSamples[iFillCol].assignValues( pcOutputSamples[iCol], rlErr, bSAD );
//          }
//
//          if ( iMaxInd == -1 )
//          {
//            iMaxInd = pcOutputSamples[iCol].m_pcTopSample->m_uiPosX;
//          }
//        }
//        else
//        {
//          // Fill Gap here
//          pcOutputSamples[iCol].assignValues( pcOutputSamples[iCol-1], rlErr, bSAD  );
//
//        };
//      }
//      else
//      {
//        if ( pcOutputSamples[iCol].m_pcTopSample->m_uiPosX > iMaxInd)
//        {
//          iMaxInd = pcOutputSamples[iCol].m_pcTopSample->m_uiPosX;
//
//          //assign this Sample
//          pcOutputSamples[iCol].assignValues( pcOutputSamples[iCol],   rlErr, bSAD );
//        }
//        else
//        {
//          //assign last sample
//          pcOutputSamples[iCol].assignValues( pcOutputSamples[iCol-1], rlErr, bSAD );
//        }
//      }
//    }
//
//    pcInputSamples  += m_iStride;
//    pcPelDisp       += iDispStride;
//
//  }
//}
//
//Void RendererOutputSample::assignValues( RendererOutputSample& rpcOutputSample, Long& rlErr, Bool bSAD )
//{
//  m_uiY = rpcOutputSample.m_uiYRef;
//  m_uiU = rpcOutputSample.m_uiURef;
//  m_uiV = rpcOutputSample.m_uiVRef;
//
//  rlErr -= m_iErr;
//
//  int iErr = m_uiY - m_uiYRef;
//
//  if (bSAD)
//  {
//    rlErr += abs(iErr);
//  }
//  else
//  {
//    rlErr += iErr * iErr;
//  }
//}
//
//Void RendererOutputSample::addInputSample( RendererInputSample* pcInputSample )
//{
//  if ( m_acInputSamples.size() == 0 )
//  {
//    m_acInputSamples.push_back( pcInputSample );
//    m_pcTopSample = pcInputSample;
//    pcInputSample->cIter = m_acInputSamples.begin();
//  }
//  else
//  {
//    std::vector <RendererInputSample*>::iterator cIter;
//
//    for ( cIter = m_acInputSamples.begin( ) ; cIter != m_acInputSamples.end( ) ; cIter++ )
//    {
//      if ( (*cIter)->m_uiPosX <= pcInputSample->m_uiPosX ) break;
//    }
//
//    m_acInputSamples.insert( cIter, 1, pcInputSample );
//
//    pcInputSample->cIter = cIter;
//
//    if ( cIter == m_acInputSamples.begin() )
//    {
//      m_pcTopSample = pcInputSample;
//    }
//  }
//}
//
//RendererOutputSample::RendererOutputSample()
//{
//  m_pcTopSample = 0;
//}
//
//Void RendererOutputSample::removeInputSample( RendererInputSample* pcInputSample )
//{
//  m_acInputSamples.erase( pcInputSample->cIter );
//}
//
