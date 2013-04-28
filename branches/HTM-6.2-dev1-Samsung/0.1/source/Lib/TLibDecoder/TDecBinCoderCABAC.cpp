

/** \file     TDecBinCoderCABAC.cpp
    \brief    binary entropy decoder of CABAC
*/

#include "TDecBinCoderCABAC.h"


TDecBinCABAC::TDecBinCABAC()
: m_pcTComBitstream( 0 )
{
}

TDecBinCABAC::~TDecBinCABAC()
{
}

Void
TDecBinCABAC::init( TComBitstream* pcTComBitstream )
{
  m_pcTComBitstream = pcTComBitstream;
}

Void
TDecBinCABAC::uninit()
{
  m_pcTComBitstream = 0;
}

Void
TDecBinCABAC::start()
{
  m_pcTComBitstream->setModeSbac();
  
  m_uiRange    = 510;
  m_uiValue    = 0;
  for( UInt ui = 0; ui < 9; ui++ )
  {
    xReadBit( m_uiValue );
  }
}

Void
TDecBinCABAC::finish()
{
}

Void
TDecBinCABAC::decodeBin( UInt& ruiBin, ContextModel &rcCtxModel )
{
  UInt  uiLPS   = TComCABACTables::sm_aucLPSTable[ rcCtxModel.getState() ][ ( m_uiRange >> 6 ) & 3 ];
  m_uiRange    -= uiLPS;
  if( m_uiValue < m_uiRange )
  {
    ruiBin      = rcCtxModel.getMps();
    rcCtxModel.updateMPS();
  }
  else
  {
    m_uiValue  -= m_uiRange;
    m_uiRange   = uiLPS;
    ruiBin      = 1 - rcCtxModel.getMps();
    rcCtxModel.updateLPS();
  }
  while( m_uiRange < 256 )
  {
    m_uiRange  += m_uiRange;
    xReadBit( m_uiValue );
  }
}

Void
TDecBinCABAC::decodeBinEP( UInt& ruiBin )
{
  xReadBit( m_uiValue );
  if( m_uiValue >= m_uiRange )
  {
    ruiBin      = 1;
    m_uiValue  -= m_uiRange;
  }
  else
  {
    ruiBin      = 0;
  }
}

Void
TDecBinCABAC::decodeBinTrm( UInt& ruiBin )
{
  m_uiRange -= 2;
  if( m_uiValue >= m_uiRange )
  {
    ruiBin = 1;
  }
  else
  {
    ruiBin = 0;
    while( m_uiRange < 256 )
    {
      m_uiRange += m_uiRange;
      xReadBit( m_uiValue );
    }
  }
}

Void  
TDecBinCABAC::xReadBit( UInt& ruiVal )
{
  UInt uiBit = 0;
  m_pcTComBitstream->read( 1, uiBit );
  ruiVal  = ( ruiVal << 1 ) | uiBit;
}
