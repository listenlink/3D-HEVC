/* The copyright in this software is being made available under the BSD
 * License, included below. This software may be subject to other third party
 * and contributor rights, including patent rights, and no such rights are
 * granted under this license.
 *
 * Copyright (c) 2010-2011, ISO/IEC
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



/** \file     TComMotionInfo.h
    \brief    motion information handling classes (header)
    \todo     TComMvField seems to be better to be inherited from TComMv
*/

#ifndef __TCOMMOTIONINFO__
#define __TCOMMOTIONINFO__

#include <memory.h>
#include "CommonDef.h"
#include "TComMv.h"

// ====================================================================================================================
// Type definition
// ====================================================================================================================

/// parameters for AMVP
typedef struct _AMVPInfo
{
  TComMv m_acMvCand[ AMVP_MAX_NUM_CANDS ];  ///< array of motion vector predictor candidates
  Int    iN;                                ///< number of motion vector predictor candidates
} AMVPInfo;

// ====================================================================================================================
// Class definition
// ====================================================================================================================

/// class for motion vector with reference index
class TComMvField
{
private:
  TComMv    m_acMv;
  Int       m_iRefIdx;
  
public:
  TComMvField() :
  m_iRefIdx (-1)
  {
  }
  
  Void setMvField ( TComMv cMv, Int iRefIdx )
  {
    m_acMv    = cMv;
    m_iRefIdx = iRefIdx;
  }
  
  TComMv& getMv     ()      { return  m_acMv;             }
  Int     getRefIdx ()      { return  m_iRefIdx;          }
  
  Int     getHor    ()      { return  m_acMv.getHor();    }
  Int     getVer    ()      { return  m_acMv.getVer();    }
};

/// class for motion information in one CU
class TComCUMvField
{
private:
  TComMv*   m_pcMv;
  TComMv*   m_pcMvd;
  Int*      m_piRefIdx;
  UInt      m_uiNumPartition;
  AMVPInfo  m_cAMVPInfo;
public:
  TComCUMvField()
  {
    m_pcMv     = NULL;
    m_pcMvd    = NULL;
    m_piRefIdx = NULL;
  }
  ~TComCUMvField()
  {
    m_pcMv     = NULL;
    m_pcMvd    = NULL;
    m_piRefIdx = NULL;
  }
  
  // ------------------------------------------------------------------------------------------------------------------
  // create / destroy
  // ------------------------------------------------------------------------------------------------------------------
  
  Void    create        ( UInt uiNumPartition );
  Void    destroy       ();
  
  // ------------------------------------------------------------------------------------------------------------------
  // clear / copy
  // ------------------------------------------------------------------------------------------------------------------
  
  Void    clearMv       ( Int iPartAddr, UInt uiDepth );
  Void    clearMvd      ( Int iPartAddr, UInt uiDepth );
  Void    clearMvField  ();
  
  Void    copyFrom          ( TComCUMvField* pcCUMvFieldSrc, Int iNumPartSrc, Int iPartAddrDst );
  Void    copyTo            ( TComCUMvField* pcCUMvFieldDst, Int iPartAddrDst );
  Void    copyTo            ( TComCUMvField* pcCUMvFieldDst, Int iPartAddrDst, UInt uiOffset, UInt uiNumPart );
  Void    copyMvTo          ( TComCUMvField* pcCUMvFieldDst, Int iPartAddrDst );
  
  // ------------------------------------------------------------------------------------------------------------------
  // get
  // ------------------------------------------------------------------------------------------------------------------
  
  TComMv& getMv             ( Int iIdx )               { return  m_pcMv    [iIdx]; }
  TComMv* getMv             ()                         { return  m_pcMv;           }
  TComMv& getMvd            ( Int iIdx )               { return  m_pcMvd   [iIdx]; }
  TComMv* getMvd            ()                         { return  m_pcMvd;          }
  Int     getRefIdx         ( Int iIdx )               { return  m_piRefIdx[iIdx]; }
  Int*    getRefIdx         ()                         { return  m_piRefIdx;       }
  
  AMVPInfo* getAMVPInfo () { return &m_cAMVPInfo; }
  // ------------------------------------------------------------------------------------------------------------------
  // set
  // ------------------------------------------------------------------------------------------------------------------
  
  Void    setMv             ( TComMv  cMv,     Int iIdx ) { m_pcMv    [iIdx] = cMv;     }
  Void    setMvd            ( TComMv  cMvd,    Int iIdx ) { m_pcMvd   [iIdx] = cMvd;    }
  Void    setRefIdx         ( Int     iRefIdx, Int iIdx ) { m_piRefIdx[iIdx] = iRefIdx; }
  
  Void    setMvPtr          ( TComMv*  cMvPtr     ) { m_pcMv    = cMvPtr;         }
  Void    setMvdPtr         ( TComMv*  cMvdPtr    ) { m_pcMvd  = cMvdPtr;         }
  Void    setRefIdxPtr      ( Int*     iRefIdxPtr ) { m_piRefIdx = iRefIdxPtr;    }
  Void    setNumPartition   ( Int      iNumPart   ) { m_uiNumPartition=iNumPart;  }
  
  Void    setAllMv          ( TComMv& rcMv,    PartSize eCUMode, Int iPartAddr, Int iPartIdx, UInt uiDepth );
  Void    setAllMvd         ( TComMv& rcMvd,   PartSize eCUMode, Int iPartAddr, Int iPartIdx, UInt uiDepth );
  Void    setAllRefIdx      ( Int     iRefIdx, PartSize eMbMode, Int iPartAddr, Int iPartIdx, UInt uiDepth );
  Void    setAllMvField     ( TComMv& rcMv, Int iRefIdx, PartSize eMbMode, Int iPartAddr, Int iPartIdx, UInt uiDepth );
  
#if AMVP_BUFFERCOMPRESS
  Void    compress          (PredMode* pePredMode,UChar* puhInterDir);
#endif 
  
};

#endif // __TCOMMOTIONINFO__

