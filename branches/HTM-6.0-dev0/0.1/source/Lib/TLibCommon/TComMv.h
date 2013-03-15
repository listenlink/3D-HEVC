

/** \file     TComMv.h
    \brief    motion vector class (header)
*/

#ifndef __TCOMMV__
#define __TCOMMV__

#include <math.h>
#include "CommonDef.h"

#include <cstdlib>
using namespace std;

// ====================================================================================================================
// Class definition
// ====================================================================================================================

/// basic motion vector class
class TComMv
{
private:
  Int   m_iHor;     ///< horizontal component of motion vector
  Int   m_iVer;     ///< vertical component of motion vector
  
public:
  
  // ------------------------------------------------------------------------------------------------------------------
  // constructors
  // ------------------------------------------------------------------------------------------------------------------
  
  TComMv() :
  m_iHor(0),
  m_iVer(0)
  {
  }
  
  TComMv( Int iHor, Int iVer ) :
  m_iHor(iHor),
  m_iVer(iVer)
  {
  }
  
  // ------------------------------------------------------------------------------------------------------------------
  // set
  // ------------------------------------------------------------------------------------------------------------------
  
  Void  set       ( Int iHor, Int iVer)     { m_iHor = iHor;  m_iVer = iVer;            }
  Void  setHor    ( Int i )                 { m_iHor = i;                               }
  Void  setVer    ( Int i )                 { m_iVer = i;                               }
  Void  setZero   ()                        { m_iHor = m_iVer = 0;  }
  
  // ------------------------------------------------------------------------------------------------------------------
  // get
  // ------------------------------------------------------------------------------------------------------------------
  
  Int   getHor    ()  { return m_iHor;          }
  Int   getVer    ()  { return m_iVer;          }
  Int   getAbsHor ()  { return abs( m_iHor );   }
  Int   getAbsVer ()  { return abs( m_iVer );   }
  
  // ------------------------------------------------------------------------------------------------------------------
  // operations
  // ------------------------------------------------------------------------------------------------------------------
  
  const TComMv& operator = (const TComMv& rcMv)
  {
    m_iHor = rcMv.m_iHor;
    m_iVer = rcMv.m_iVer;
    return  *this;
  }
  
  const TComMv& operator += (const TComMv& rcMv)
  {
    m_iHor += rcMv.m_iHor;
    m_iVer += rcMv.m_iVer;
    return  *this;
  }
  
  const TComMv& operator-= (const TComMv& rcMv)
  {
    m_iHor -= rcMv.m_iHor;
    m_iVer -= rcMv.m_iVer;
    return  *this;
  }
  
  const TComMv& operator>>= (const Int i)
  {
    m_iHor >>= i;
    m_iVer >>= i;
    return  *this;
  }
  
  const TComMv& operator<<= (const Int i)
  {
    m_iHor <<= i;
    m_iVer <<= i;
    return  *this;
  }
  
  const TComMv operator - ( const TComMv& rcMv ) const
  {
    return TComMv( m_iHor - rcMv.m_iHor, m_iVer - rcMv.m_iVer );
  }
  
  const TComMv operator + ( const TComMv& rcMv )
  {
    return TComMv( m_iHor + rcMv.m_iHor, m_iVer + rcMv.m_iVer );
  }
  
  Bool operator== ( const TComMv& rcMv )
  {
    return (m_iHor==rcMv.m_iHor && m_iVer==rcMv.m_iVer);
  }
  
  Bool operator!= ( const TComMv& rcMv )
  {
    return (m_iHor!=rcMv.m_iHor || m_iVer!=rcMv.m_iVer);
  }
  
  const TComMv scaleMv( Int iScale )
  {
    return TComMv( (iScale * getHor() + 128) >> 8, (iScale * getVer() + 128) >> 8);
  }
};// END CLASS DEFINITION TComMV


#endif // __TCOMMV__

