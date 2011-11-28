

/** \file     ContextModel3DBuffer.h
    \brief    context model 3D buffer class (header)
*/

#ifndef __CONTEXT_MODEL_3DBUFFER__
#define __CONTEXT_MODEL_3DBUFFER__

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <stdio.h>
#include <assert.h>
#include <memory.h>

#include "CommonDef.h"
#include "ContextModel.h"

// ====================================================================================================================
// Class definition
// ====================================================================================================================

/// context model 3D buffer class
class ContextModel3DBuffer
{
protected:
  ContextModel* m_pcContextModel;                                         ///< array of context models
  const UInt    m_uiSizeX;                                                ///< X size of 3D buffer
  const UInt    m_uiSizeY;                                                ///< Y size of 3D buffer
  const UInt    m_uiSizeZ;                                                ///< Z size of 3D buffer
  
public:
  ContextModel3DBuffer  ( UInt uiSizeZ, UInt uiSizeY, UInt uiSizeX );
  ~ContextModel3DBuffer ();
  
  // access functions
  ContextModel& get( UInt uiZ, UInt uiY, UInt uiX )
  {
    return  m_pcContextModel[ ( uiZ * m_uiSizeY + uiY ) * m_uiSizeX + uiX ];
  }
  ContextModel* get( UInt uiZ, UInt uiY )
  {
    return &m_pcContextModel[ ( uiZ * m_uiSizeY + uiY ) * m_uiSizeX       ];
  }
  ContextModel* get( UInt uiZ )
  {
    return &m_pcContextModel[ ( uiZ * m_uiSizeY       ) * m_uiSizeX       ];
  }
  
  // initialization & copy functions
  Void initBuffer( SliceType eSliceType, Int iQp, Short* psCtxModel );          ///< initialize 3D buffer by slice type & QP
  Void copyFrom  ( ContextModel3DBuffer* pSrc                       );          ///< copy from given 3D buffer
};

#endif

