

/** \file     ContextModel3DBuffer.cpp
    \brief    context model 3D buffer class
*/

#include "ContextModel3DBuffer.h"

// ====================================================================================================================
// Constructor / destructor / initialization / destroy
// ====================================================================================================================

ContextModel3DBuffer::ContextModel3DBuffer( UInt uiSizeZ, UInt uiSizeY, UInt uiSizeX ) :
m_pcContextModel( NULL ),
m_uiSizeX( uiSizeX ),
m_uiSizeY( uiSizeY ),
m_uiSizeZ( uiSizeZ )

{
  // allocate 3D buffer
  m_pcContextModel = new ContextModel[ uiSizeZ * m_uiSizeY * m_uiSizeX ];
}

ContextModel3DBuffer::~ContextModel3DBuffer()
{
  // delete 3D buffer
  delete [] m_pcContextModel;
  m_pcContextModel = NULL;
}

// ====================================================================================================================
// Public member functions
// ====================================================================================================================

/**
 - initialize 3D buffer with respect to slicetype, QP and given initial probability table
 .
 \param  eSliceType      slice type
 \param  iQP             input QP value
 \param  psCtxModel      given probability table
 */
Void ContextModel3DBuffer::initBuffer( SliceType eSliceType, Int iQp, Short* psCtxModel )
{
  UInt n, z, offset = 0;
  
  for ( z = 0; z < m_uiSizeZ; z++ )
  {
    for ( n = 0; n < m_uiSizeY * m_uiSizeX; n++ )
    {
      m_pcContextModel[ offset + n ].init( iQp, psCtxModel + eSliceType * 2 * ( m_uiSizeZ * m_uiSizeY * m_uiSizeX ) + 2 * (n + offset) );
    }
    offset += n;
  }
  return;
}

/**
 - copy from given 3D buffer
 .
 \param  pSrc          given 3D buffer
 */
Void ContextModel3DBuffer::copyFrom( ContextModel3DBuffer* pSrc )
{
  ::memcpy( this->m_pcContextModel, pSrc->m_pcContextModel, sizeof(ContextModel) * m_uiSizeZ * m_uiSizeY * m_uiSizeX );
}

