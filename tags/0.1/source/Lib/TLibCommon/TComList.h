

/** \file     TComList.h
    \brief    general list class (header)
*/

#ifndef _TCOMLIST_
#define _TCOMLIST_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <list>
#include <assert.h>
#include "CommonDef.h"

#include <cstdlib>
using namespace std;

// ====================================================================================================================
// Class definition
// ====================================================================================================================

/// list template
template< class C >
class TComList : public std::list< C >
{
public:
  typedef typename std::list<C>::iterator TComIterator;
  
  TComList& operator += ( const TComList& rcTComList)
  {
    if( ! rcTComList.empty() )
    {
      insert( this->end(), rcTComList.begin(), rcTComList.end());
    }
    return *this;
  } // leszek
  
  C popBack()
  {
    C cT = this->back();
    this->pop_back();
    return cT;
  }
  
  C popFront()
  {
    C cT = this->front();
    this->pop_front();
    return cT;
  }
  
  Void pushBack( const C& rcT )
  {
    /*assert( sizeof(C) == 4);*/
    if( rcT != NULL )
    {
      this->push_back( rcT);
    }
  }
  
  Void pushFront( const C& rcT )
  {
    /*assert( sizeof(C) == 4);*/
    if( rcT != NULL )
    {
      this->push_front( rcT);
    }
  }
  
  TComIterator find( const C& rcT ) // leszek
  {
    return find( this->begin(), this->end(), rcT );
  }
};

#endif

