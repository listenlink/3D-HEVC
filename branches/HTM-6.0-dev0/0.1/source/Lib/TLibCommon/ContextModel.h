


/** \file     ContextModel.h
    \brief    context model class (header)
*/

#ifndef __CONTEXT_MODEL__
#define __CONTEXT_MODEL__

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "CommonDef.h"


// ====================================================================================================================
// Class definition
// ====================================================================================================================

/// context model class
class ContextModel
{
public:
  ContextModel  ()                        { m_ucState = 0;             }
  ~ContextModel ()                        {}
  
  const UChar getState  ()                { return ( m_ucState >> 1 ); }                    ///< get current state
  const UChar getMps    ()                { return ( m_ucState  & 1 ); }                    ///< get curret MPS
  
  Void        init      ( Int   iQp, 
                         Short asCtxInit[] );                                              ///< initialize state with initial prob.
  
  Void        updateLPS ()
  {
    UChar ucMPS = ( m_ucState > 1    ? m_ucState  & 1 :    1   - ( m_ucState & 1 ) );
    m_ucState   = ( m_aucNextStateLPS[ m_ucState >> 1 ] << 1 ) + ucMPS;
  }
  
  Void        updateMPS ()
  {
    m_ucState   = ( m_aucNextStateMPS[ m_ucState >> 1 ] << 1 ) + ( m_ucState & 1 );
  }
  
private:
  UChar         m_ucState;                                                                  ///< internal state variable
  static const  UChar m_aucNextStateMPS[ 64 ];
  static const  UChar m_aucNextStateLPS[ 64 ];
};

#endif

