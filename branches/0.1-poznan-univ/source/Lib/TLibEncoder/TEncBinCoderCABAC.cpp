

/** \file     TEncBinCoderCABAC.cpp
    \brief    binary entropy encoder of CABAC
*/

#include "TEncBinCoderCABAC.h"


TEncBinCABAC::TEncBinCABAC()
: m_pcTComBitIf( 0 )
, m_bBinCountingEnabled(0)
{
}

TEncBinCABAC::~TEncBinCABAC()
{
}

Void
TEncBinCABAC::init( TComBitIf* pcTComBitIf )
{
  m_pcTComBitIf = pcTComBitIf;
}

Void
TEncBinCABAC::uninit()
{
  m_pcTComBitIf = 0;
}

Void
TEncBinCABAC::start()
{
  m_uiLow           = 0;
  m_uiRange         = 510;
  m_uiBitsToFollow  = 0;
  m_uiByte          = 0;
  m_uiBitsLeft      = 9;
}

Void
TEncBinCABAC::finish()
{
  xWriteBitAndBitsToFollow( ( m_uiLow >> 9 ) & 1 );
  xWriteBit               ( ( m_uiLow >> 8 ) & 1 );
  // xWriteBit               ( 1 ); // stop bit, already written in TEncGOP::compressGOP
  if( 8 - m_uiBitsLeft != 0 )
  {
    m_pcTComBitIf->write  ( m_uiByte, 8 - m_uiBitsLeft );
  }
}

Void
TEncBinCABAC::copyState( TEncBinIf* pcTEncBinIf )
{
  TEncBinCABAC* pcTEncBinCABAC = pcTEncBinIf->getTEncBinCABAC();
  m_uiLow           = pcTEncBinCABAC->m_uiLow;
  m_uiRange         = pcTEncBinCABAC->m_uiRange;
  m_uiBitsToFollow  = pcTEncBinCABAC->m_uiBitsToFollow;
  m_uiByte          = pcTEncBinCABAC->m_uiByte;
  m_uiBitsLeft      = pcTEncBinCABAC->m_uiBitsLeft;
}

Void  
TEncBinCABAC::resetBits()
{
  m_uiLow          &= 255;
  m_uiBitsToFollow  = 0;
  m_uiByte          = 0;
  m_uiBitsLeft      = 9;
}

UInt
TEncBinCABAC::getNumWrittenBits()
{
  return m_pcTComBitIf->getNumberOfWrittenBits() + 8 + m_uiBitsToFollow - m_uiBitsLeft + 1;
}

Void
TEncBinCABAC::encodeBin( UInt uiBin, ContextModel &rcCtxModel )
{
  {
    DTRACE_CABAC_V( g_nSymbolCounter++ )
    DTRACE_CABAC_T( "\tstate=" )
    DTRACE_CABAC_V( ( rcCtxModel.getState() << 1 ) + rcCtxModel.getMps() )
    DTRACE_CABAC_T( "\tsymbol=" )
    DTRACE_CABAC_V( uiBin )
    DTRACE_CABAC_T( "\n" )
  }
  if (m_bBinCountingEnabled) 
  {
    m_uiBinsCoded++;
  }
  UInt  uiLPS   = TComCABACTables::sm_aucLPSTable[ rcCtxModel.getState() ][ ( m_uiRange >> 6 ) & 3 ];
  m_uiRange    -= uiLPS;
  if( uiBin != rcCtxModel.getMps() )
  {
    m_uiLow    += m_uiRange;
    m_uiRange   = uiLPS;
    rcCtxModel.updateLPS();
  }
  else
  {
    rcCtxModel.updateMPS();
  }
  while( m_uiRange < 256 )
  {
    if( m_uiLow >= 512 )
    {
      xWriteBitAndBitsToFollow( 1 );
      m_uiLow -= 512;
    }
    else if( m_uiLow < 256 )
    {
      xWriteBitAndBitsToFollow( 0 );
    }
    else
    {
      m_uiBitsToFollow++;
      m_uiLow         -= 256;
    }
    m_uiLow   <<= 1;
    m_uiRange <<= 1;
  }
}

Void
TEncBinCABAC::encodeBinEP( UInt uiBin )
{
  {
    DTRACE_CABAC_V( g_nSymbolCounter++ )
    DTRACE_CABAC_T( "\tEPsymbol=" )
    DTRACE_CABAC_V( uiBin )
    DTRACE_CABAC_T( "\n" )
  }
  if (m_bBinCountingEnabled)
  {
    m_uiBinsCoded++;
  }
  m_uiLow <<= 1;
  if( uiBin )
  {
    m_uiLow += m_uiRange;
  }
  if( m_uiLow >= 1024 )
  {
    xWriteBitAndBitsToFollow( 1 );
    m_uiLow -= 1024;
  }
  else if( m_uiLow < 512 )
  {
    xWriteBitAndBitsToFollow( 0 );
  }
  else
  {
    m_uiBitsToFollow++;
    m_uiLow         -= 512;
  }
}

Void
TEncBinCABAC::encodeBinTrm( UInt uiBin )
{
  if (m_bBinCountingEnabled)
  {
    m_uiBinsCoded++;
  }
  m_uiRange -= 2;
  if( uiBin )
  {
    m_uiLow  += m_uiRange;
    m_uiRange = 2;
  }
  while( m_uiRange < 256 )
  {
    if( m_uiLow >= 512 )
    {
      xWriteBitAndBitsToFollow( 1 );
      m_uiLow -= 512;
    }
    else if( m_uiLow < 256 )
    {
      xWriteBitAndBitsToFollow( 0 );
    }
    else
    {
      m_uiBitsToFollow++;
      m_uiLow         -= 256;
    }
    m_uiLow   <<= 1;
    m_uiRange <<= 1;
  }
}

Void  
TEncBinCABAC::xWriteBit( UInt uiBit )
{
  m_uiByte += m_uiByte + uiBit;
  if( ! --m_uiBitsLeft )
  {
    m_pcTComBitIf->write( m_uiByte, 8 );
    m_uiBitsLeft  = 8;
    m_uiByte      = 0;
  }
}

Void  
TEncBinCABAC::xWriteBitAndBitsToFollow( UInt uiBit )
{
  xWriteBit( uiBit );
  uiBit = 1 - uiBit;
  while( m_uiBitsToFollow > 0 )
  {
    m_uiBitsToFollow--;
    xWriteBit( uiBit );
  }
}
