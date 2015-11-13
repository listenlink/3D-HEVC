/* The copyright in this software is being made available under the BSD
 * License, included below. This software may be subject to other third party
 * and contributor rights, including patent rights, and no such rights are
 * granted under this license.
 *
 * Copyright (c) 2010-2015, ITU/ISO/IEC
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *  * Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *  * Neither the name of the ISO/IEC nor the names of its contributors may
 *    be used to endorse or promote products derived from this software without
 *    specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */



#ifndef __TCOMWEDGELET__
#define __TCOMWEDGELET__

// Include files
#include <assert.h>
#include "CommonDef.h"

#include <vector>

#if NH_3D_DMM
#define DMM_OFFSET     (NUM_INTRA_MODE+1) // offset for DMM and RBC mode numbers
#define DMM_MIN_SIZE                   4  // min. block size for DMM and RBC modes
#define DMM_MAX_SIZE                  32  // max. block size for DMM and RBC modes

enum DmmID
{
  DMM1_IDX = 0,
  DMM4_IDX = 1,
  NUM_DMM  = 2,
  DMM_NO_IDX = MAX_UINT
};

__inline DmmID getDmmType( Int intraMode ) 
{ 
    Int dmmType = intraMode-DMM_OFFSET; 
    return (dmmType >= 0 && dmmType < NUM_DMM) ? (DmmID)dmmType : DMM_NO_IDX; 
}
__inline Bool isDmmMode( Int intraMode ) { return ((UInt)getDmmType( intraMode ) < NUM_DMM); }


// Wedgelets
#define DMM_NO_WEDGE_IDX       MAX_UINT
#define DMM_NUM_WEDGE_REFINES   8

enum WedgeResolution
{
  FULL_PEL,
  HALF_PEL
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
  Bool            m_bIsCoarse; 

  UInt  m_uiWidth;
  UInt  m_uiHeight;

  Bool* m_pbPattern;
  Bool* m_pbScaledPattern;

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
  Bool            getIsCoarse() { return m_bIsCoarse; }

  Void  generateWedgePatternByRotate(const TComWedgelet &rcWedge, Int rotate);
  Void  setWedgelet( UChar uhXs, UChar uhYs, UChar uhXe, UChar uhYe, UChar uhOri, WedgeResolution eWedgeRes, Bool bIsCoarse = false );

  Bool  checkNotPlain();
  Bool  checkIdentical( Bool* pbRefPattern );
  Bool  checkInvIdentical( Bool* pbRefPattern );

  Bool* getPatternScaled    ( UInt dstSize );
  Void  getPatternScaledCopy( UInt dstSize, Bool* dstBuf );

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


// ====================================================================================================================
// Class definition TComWedgeNode
// ====================================================================================================================
class TComWedgeNode
{
private:
  UInt            m_uiPatternIdx;
  UInt            m_uiRefineIdx[DMM_NUM_WEDGE_REFINES];

public:
  TComWedgeNode();
  virtual ~TComWedgeNode() {}

  UInt            getPatternIdx();
  UInt            getRefineIdx ( UInt uiPos );

  Void            setPatternIdx( UInt uiIdx );
  Void            setRefineIdx ( UInt uiIdx, UInt uiPos );
};  // END CLASS DEFINITION TComWedgeNode

// type definition wedgelet node list
typedef std::vector<TComWedgeNode> WedgeNodeList;
#endif //NH_3D_DMM

#endif // __TCOMWEDGELET__
