

/** \file     TDecSlice.cpp
    \brief    slice decoder class
*/

#include "TDecSlice.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

TDecSlice::TDecSlice()
{
}

TDecSlice::~TDecSlice()
{
}

Void TDecSlice::create( TComSlice* pcSlice, Int iWidth, Int iHeight, UInt uiMaxWidth, UInt uiMaxHeight, UInt uiMaxDepth )
{
}

Void TDecSlice::destroy()
{
}

Void TDecSlice::init(TDecEntropy* pcEntropyDecoder, TDecCu* pcCuDecoder)
{
  m_pcEntropyDecoder  = pcEntropyDecoder;
  m_pcCuDecoder       = pcCuDecoder;
}

Void TDecSlice::decompressSlice(TComBitstream* pcBitstream, TComPic*& rpcPic)
{
  TComDataCU* pcCU;
  UInt        uiIsLast = 0;
  Int   iStartCUAddr = max(rpcPic->getSlice(rpcPic->getCurrSliceIdx())->getSliceCurStartCUAddr(), rpcPic->getSlice(rpcPic->getCurrSliceIdx())->getEntropySliceCurStartCUAddr());

  // decoder don't need prediction & residual frame buffer
  rpcPic->setPicYuvPred( 0 );
  rpcPic->setPicYuvResi( 0 );
  
#if ENC_DEC_TRACE
  g_bJustDoIt = g_bEncDecTraceEnable;
#endif
  DTRACE_CABAC_V( g_nSymbolCounter++ );
  DTRACE_CABAC_T( "\tPOC: " );
  DTRACE_CABAC_V( rpcPic->getPOC() );
  DTRACE_CABAC_T( "\n" );

#if ENC_DEC_TRACE
  g_bJustDoIt = g_bEncDecTraceDisable;
#endif

  // for all CUs in slice
  UInt  uiLastCUAddr = iStartCUAddr;
  for( Int iCUAddr = iStartCUAddr; !uiIsLast && iCUAddr < rpcPic->getNumCUsInFrame(); iCUAddr++, uiLastCUAddr++ )
  {
    pcCU = rpcPic->getCU( iCUAddr );
    pcCU->initCU( rpcPic, iCUAddr );
    
#if ENC_DEC_TRACE
    g_bJustDoIt = g_bEncDecTraceEnable;
#endif
    m_pcCuDecoder->decodeCU     ( pcCU, uiIsLast );
    m_pcCuDecoder->decompressCU ( pcCU );
    
#if ENC_DEC_TRACE
    g_bJustDoIt = g_bEncDecTraceDisable;
#endif
  }


}
