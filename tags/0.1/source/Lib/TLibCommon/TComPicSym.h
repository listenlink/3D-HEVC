

/** \file     TComPicSym.h
    \brief    picture symbol class (header)
*/

#ifndef __TCOMPICSYM__
#define __TCOMPICSYM__


// Include files
#include "CommonDef.h"
#include "TComSlice.h"
#include "TComDataCU.h"

// ====================================================================================================================
// Class definition
// ====================================================================================================================

/// picture symbol class
class TComPicSym
{
private:
  UInt          m_uiWidthInCU;
  UInt          m_uiHeightInCU;
  
  UInt          m_uiMaxCUWidth;
  UInt          m_uiMaxCUHeight;
  UInt          m_uiMinCUWidth;
  UInt          m_uiMinCUHeight;
  
  UChar         m_uhTotalDepth;       ///< max. depth
  UInt          m_uiNumPartitions;
  UInt          m_uiNumPartInWidth;
  UInt          m_uiNumPartInHeight;
  UInt          m_uiNumCUsInFrame;
  
  TComSlice**   m_apcTComSlice;
  UInt          m_uiNumAllocatedSlice;
  TComDataCU**  m_apcTComDataCU;        ///< array of CU data
  
public:
  Void        create  ( Int iPicWidth, Int iPicHeight, UInt uiMaxWidth, UInt uiMaxHeight, UInt uiMaxDepth );
  Void        destroy ();
  
  TComPicSym  ()                        { m_uiNumAllocatedSlice = 0;            }
  TComSlice*  getSlice(UInt i)          { return  m_apcTComSlice[i];            }
  UInt        getFrameWidthInCU()       { return m_uiWidthInCU;                 }
  UInt        getFrameHeightInCU()      { return m_uiHeightInCU;                }
  UInt        getMinCUWidth()           { return m_uiMinCUWidth;                }
  UInt        getMinCUHeight()          { return m_uiMinCUHeight;               }
  UInt        getNumberOfCUsInFrame()   { return m_uiNumCUsInFrame;  }
  TComDataCU*&  getCU( UInt uiCUAddr )  { return m_apcTComDataCU[uiCUAddr];     }
  
  Void        setSlice(TComSlice* p, UInt i) { m_apcTComSlice[i] = p;           }
  UInt        getNumAllocatedSlice()    { return m_uiNumAllocatedSlice;         }
  Void        allocateNewSlice();
  Void        clearSliceBuffer();
  UInt        getNumPartition()         { return m_uiNumPartitions;             }
  UInt        getNumPartInWidth()       { return m_uiNumPartInWidth;            }
  UInt        getNumPartInHeight()      { return m_uiNumPartInHeight;           }
};// END CLASS DEFINITION TComPicSym


#endif // __TCOMPICSYM__

