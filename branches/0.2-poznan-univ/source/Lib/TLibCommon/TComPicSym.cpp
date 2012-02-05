

/** \file     TComPicSym.cpp
    \brief    picture symbol class
*/

#include "TComPicSym.h"

// ====================================================================================================================
// Constructor / destructor / create / destroy
// ====================================================================================================================

Void TComPicSym::create  ( Int iPicWidth, Int iPicHeight, UInt uiMaxWidth, UInt uiMaxHeight, UInt uiMaxDepth )
{
  UInt i;

  m_uhTotalDepth      = uiMaxDepth;
  m_uiNumPartitions   = 1<<(m_uhTotalDepth<<1);
  
  m_iPicWidth         = iPicWidth;
  m_iPicHeight        = iPicHeight;

  m_uiMaxCUWidth      = uiMaxWidth;
  m_uiMaxCUHeight     = uiMaxHeight;
  
  m_uiMinCUWidth      = uiMaxWidth  >> m_uhTotalDepth;
  m_uiMinCUHeight     = uiMaxHeight >> m_uhTotalDepth;
  
  m_uiNumPartInWidth  = m_uiMaxCUWidth  / m_uiMinCUWidth;
  m_uiNumPartInHeight = m_uiMaxCUHeight / m_uiMinCUHeight;
  
  m_uiWidthInCU       = ( iPicWidth %m_uiMaxCUWidth  ) ? iPicWidth /m_uiMaxCUWidth  + 1 : iPicWidth /m_uiMaxCUWidth;
  m_uiHeightInCU      = ( iPicHeight%m_uiMaxCUHeight ) ? iPicHeight/m_uiMaxCUHeight + 1 : iPicHeight/m_uiMaxCUHeight;
  
  m_uiNumCUsInFrame   = m_uiWidthInCU * m_uiHeightInCU;
  m_apcTComDataCU     = new TComDataCU*[m_uiNumCUsInFrame];
  
  if (m_uiNumAllocatedSlice>0)
  {
    for ( i=0; i<m_uiNumAllocatedSlice ; i++ )
    {
      delete m_apcTComSlice[i];
    }
    delete [] m_apcTComSlice;
  }
  m_apcTComSlice      = new TComSlice*[m_uiNumCUsInFrame];  
  m_apcTComSlice[0]   = new TComSlice;
  m_uiNumAllocatedSlice = 1;
  for ( i=0; i<m_uiNumCUsInFrame ; i++ )
  {
    m_apcTComDataCU[i] = new TComDataCU;
    m_apcTComDataCU[i]->create( m_uiNumPartitions, m_uiMaxCUWidth, m_uiMaxCUHeight, false );
  }
}

Void TComPicSym::destroy()
{
  Int i;
  
  if (m_uiNumAllocatedSlice>0)
  {
    for ( i=0; i<m_uiNumAllocatedSlice ; i++ )
    {
      delete m_apcTComSlice[i];
    }
    delete [] m_apcTComSlice;
  }
  m_apcTComSlice = NULL;
  
  for (i = 0; i < m_uiNumCUsInFrame; i++)
  {
    m_apcTComDataCU[i]->destroy();
    delete m_apcTComDataCU[i];
    m_apcTComDataCU[i] = NULL;
  }
  delete [] m_apcTComDataCU;
  m_apcTComDataCU = NULL;
}

Void TComPicSym::allocateNewSlice()
{
  assert(m_uiNumCUsInFrame >= m_uiNumAllocatedSlice);
  m_apcTComSlice[m_uiNumAllocatedSlice ++] = new TComSlice;
}

Void TComPicSym::clearSliceBuffer()
{
  UInt i;
  for (i = 1; i < m_uiNumAllocatedSlice; i++)
  {
    delete m_apcTComSlice[i];
  }
  m_uiNumAllocatedSlice = 1;
}
