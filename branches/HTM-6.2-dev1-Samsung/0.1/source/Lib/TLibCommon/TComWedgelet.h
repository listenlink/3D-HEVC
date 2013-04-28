

#ifndef __TCOMWEDGELET__
#define __TCOMWEDGELET__

// Include files
#include <assert.h>
#include "CommonDef.h"

#include <vector>


enum WedgeResolution
{
  DOUBLE_PEL,
  FULL_PEL,
  HALF_PEL
};

enum WedgeDist
{
  WedgeDist_SAD  = 0,
  WedgeDist_SSE  = 4,
};

// ====================================================================================================================
// Class definition TComWedgelet
// ====================================================================================================================
class TComWedgelet
{
private:
  UChar           m_uhXs;                       // line start X pos
  UChar           m_uhYs;                       // line start Y pos
  UChar           m_uhXe;                       // line end   X pos
  UChar           m_uhYe;                       // line end   Y pos
  UChar           m_uhOri;                      // orientation index
  WedgeResolution m_eWedgeRes;                  // start/end pos resolution

  UInt  m_uiWidth;
  UInt  m_uiHeight;

  Bool* m_pbPattern;

  Void  xGenerateWedgePattern();
  Void  xDrawEdgeLine( UChar uhXs, UChar uhYs, UChar uhXe, UChar uhYe, Bool* pbPattern, Int iPatternStride );

public:
  TComWedgelet( UInt uiWidth, UInt uiHeight );
  TComWedgelet( const TComWedgelet &rcWedge );
  virtual ~TComWedgelet();

  Void  create ( UInt iWidth, UInt iHeight );   ///< create  wedgelet pattern
  Void  destroy();                              ///< destroy wedgelet pattern
  Void  clear  ();                              ///< clear   wedgelet pattern

  UInt            getWidth   () { return m_uiWidth; }
  UInt            getStride  () { return m_uiWidth; }
  UInt            getHeight  () { return m_uiHeight; }
  WedgeResolution getWedgeRes() { return m_eWedgeRes; }
  Bool*           getPattern () { return m_pbPattern; }
  UChar           getStartX  () { return m_uhXs; }
  UChar           getStartY  () { return m_uhYs; }
  UChar           getEndX    () { return m_uhXe; }
  UChar           getEndY    () { return m_uhYe; }
  UChar           getOri     () { return m_uhOri; }

  Void  setWedgelet( UChar uhXs, UChar uhYs, UChar uhXe, UChar uhYe, UChar uhOri, WedgeResolution eWedgeRes );

  Bool  checkNotPlain();
  Bool  checkNotIdentical( Bool* pbRefPattern );
  Bool  checkNotInvIdentical( Bool* pbRefPattern );

  Bool  checkPredDirAbovePossible( UInt uiPredDirBlockSize, UInt uiPredDirBlockOffsett );
  Bool  checkPredDirLeftPossible ( UInt uiPredDirBlockSize, UInt uiPredDirBlockOffsett );

  Void  getPredDirStartEndAbove( UChar& ruhXs, UChar& ruhYs, UChar& ruhXe, UChar& ruhYe, UInt uiPredDirBlockSize, UInt uiPredDirBlockOffset, Int iDeltaEnd );
  Void  getPredDirStartEndLeft ( UChar& ruhXs, UChar& ruhYs, UChar& ruhXe, UChar& ruhYe, UInt uiPredDirBlockSize, UInt uiPredDirBlockOffset, Int iDeltaEnd );
};  // END CLASS DEFINITION TComWedgelet

// type definition wedgelet pattern list
typedef std::vector<TComWedgelet> WedgeList;

// ====================================================================================================================
// Class definition TComWedgeRef
// ====================================================================================================================
class TComWedgeRef
{
private:
  UChar           m_uhXs;                       // line start X pos
  UChar           m_uhYs;                       // line start Y pos
  UChar           m_uhXe;                       // line end   X pos
  UChar           m_uhYe;                       // line end   Y pos
  UInt            m_uiRefIdx;                   // index of corresponding pattern of TComWedgelet object in wedge list

public:
  TComWedgeRef() {}
  virtual ~TComWedgeRef() {}

  UChar           getStartX  () { return m_uhXs; }
  UChar           getStartY  () { return m_uhYs; }
  UChar           getEndX    () { return m_uhXe; }
  UChar           getEndY    () { return m_uhYe; }
  UInt            getRefIdx  () { return m_uiRefIdx; }

  Void  setWedgeRef( UChar uhXs, UChar uhYs, UChar uhXe, UChar uhYe, UInt uiRefIdx ) { m_uhXs = uhXs; m_uhYs = uhYs; m_uhXe = uhXe; m_uhYe = uhYe; m_uiRefIdx = uiRefIdx; }
};  // END CLASS DEFINITION TComWedgeRef

// type definition wedgelet reference list
typedef std::vector<TComWedgeRef> WedgeRefList;

// ====================================================================================================================
// Function definition roftoi (mathematically correct rounding of float to int)
// ====================================================================================================================
__inline Int roftoi( Double value )
{
  (value < -0.5) ? (value -= 0.5) : (value += 0.5);
  return ( (Int)value );
}
#endif // __TCOMWEDGELET__
