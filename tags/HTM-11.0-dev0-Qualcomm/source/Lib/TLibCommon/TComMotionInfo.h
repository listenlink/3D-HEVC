/* The copyright in this software is being made available under the BSD
 * License, included below. This software may be subject to other third party
 * and contributor rights, including patent rights, and no such rights are
 * granted under this license.  
 *
* Copyright (c) 2010-2014, ITU/ISO/IEC
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
 *  * Neither the name of the ITU/ISO/IEC nor the names of its contributors may
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
//! \ingroup TLibCommon
//! \{

// ====================================================================================================================
// Type definition
// ====================================================================================================================

#if H_3D_SPIVMP
class TComDataCU;
#endif
/// parameters for AMVP
typedef struct _AMVPInfo
{
  TComMv m_acMvCand[ AMVP_MAX_NUM_CANDS_MEM ];  ///< array of motion vector predictor candidates
  Int    iN;                                ///< number of motion vector predictor candidates
} AMVPInfo;

#if H_3D_NBDV
typedef struct _DisCand 
{
  Bool bDV;
  TComMv m_acNBDV;              // DV from NBDV
#if H_3D_NBDV_REF 
  TComMv m_acDoNBDV;            // DV from DoNBDV
#endif  
  Int    m_aVIdxCan;            // View order index (the same with the NBDV and the DoNBDV)
} DisInfo;

typedef struct _IDVCand // IDV
{
  TComMv m_acMvCand[2][ IDV_CANDS ];            
  Int    m_aVIdxCan[2][ IDV_CANDS ];            
  Bool   m_bAvailab[2][ IDV_CANDS ];
  Bool   m_bFound;                                
} IDVInfo;
#endif
#if H_3D_VSP
typedef struct _InheritedVSPDisCand 
{
  DisInfo m_acDvInfo;
} InheritedVSPDisInfo;
#endif
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
  TComMvField() : m_iRefIdx( NOT_VALID ) {}
  
  Void setMvField( TComMv const & cMv, Int iRefIdx )
  {
    m_acMv    = cMv;
    m_iRefIdx = iRefIdx;
  }
  
  Void setRefIdx( Int refIdx ) { m_iRefIdx = refIdx; }
  
  TComMv const & getMv() const { return  m_acMv; }
  TComMv       & getMv()       { return  m_acMv; }
  
  Int getRefIdx() const { return  m_iRefIdx;       }
  Int getHor   () const { return  m_acMv.getHor(); }
  Int getVer   () const { return  m_acMv.getVer(); }
#if H_3D_IV_MERGE
  Bool operator== ( const TComMvField& rcMv ) const
  {
    return (m_acMv.getHor()==rcMv.getHor() && m_acMv.getVer()==rcMv.getVer() && m_iRefIdx == rcMv.getRefIdx());
  }
#endif
};

/// class for motion information in one CU
class TComCUMvField
{
private:
  TComMv*   m_pcMv;
  TComMv*   m_pcMvd;
  Char*     m_piRefIdx;
  UInt      m_uiNumPartition;
  AMVPInfo  m_cAMVPInfo;
    
  template <typename T>
  Void setAll( T *p, T const & val, PartSize eCUMode, Int iPartAddr, UInt uiDepth, Int iPartIdx );

public:
  TComCUMvField() : m_pcMv(NULL), m_pcMvd(NULL), m_piRefIdx(NULL), m_uiNumPartition(0) {}
  ~TComCUMvField() {}

  // ------------------------------------------------------------------------------------------------------------------
  // create / destroy
  // ------------------------------------------------------------------------------------------------------------------
  
  Void    create( UInt uiNumPartition );
  Void    destroy();
  
  // ------------------------------------------------------------------------------------------------------------------
  // clear / copy
  // ------------------------------------------------------------------------------------------------------------------

  Void    clearMvField();
  
  Void    copyFrom( TComCUMvField const * pcCUMvFieldSrc, Int iNumPartSrc, Int iPartAddrDst );
  Void    copyTo  ( TComCUMvField* pcCUMvFieldDst, Int iPartAddrDst ) const;
  Void    copyTo  ( TComCUMvField* pcCUMvFieldDst, Int iPartAddrDst, UInt uiOffset, UInt uiNumPart ) const;
  
  // ------------------------------------------------------------------------------------------------------------------
  // get
  // ------------------------------------------------------------------------------------------------------------------
  
  TComMv const & getMv    ( Int iIdx ) const { return  m_pcMv    [iIdx]; }
  TComMv const & getMvd   ( Int iIdx ) const { return  m_pcMvd   [iIdx]; }
  Int            getRefIdx( Int iIdx ) const { return  m_piRefIdx[iIdx]; }
  
  AMVPInfo* getAMVPInfo () { return &m_cAMVPInfo; }
  
  // ------------------------------------------------------------------------------------------------------------------
  // set
  // ------------------------------------------------------------------------------------------------------------------
  
  Void    setAllMv     ( TComMv const & rcMv,         PartSize eCUMode, Int iPartAddr, UInt uiDepth, Int iPartIdx=0 );
  Void    setAllMvd    ( TComMv const & rcMvd,        PartSize eCUMode, Int iPartAddr, UInt uiDepth, Int iPartIdx=0 );
  Void    setAllRefIdx ( Int iRefIdx,                 PartSize eMbMode, Int iPartAddr, UInt uiDepth, Int iPartIdx=0 );
  Void    setAllMvField( TComMvField const & mvField, PartSize eMbMode, Int iPartAddr, UInt uiDepth, Int iPartIdx=0 );
#if H_3D_SPIVMP
  Void    setMvFieldSP ( TComDataCU* pcCU, UInt uiAbsPartIdx, TComMvField cMvField, Int iWidth, Int iHeight  );
#endif
#if H_3D_VSP
  Void    setMv         ( Int iIdx, TComMv const & rcMv ) { m_pcMv[iIdx] = rcMv; }
  Void    setRefIdx     ( Int iIdx, Int iRefIdx )         { m_piRefIdx[iIdx] = iRefIdx; }
#endif

  Void setNumPartition( Int iNumPart )
  {
    m_uiNumPartition = iNumPart;
  }
  
  Void linkToWithOffset( TComCUMvField const * src, Int offset )
  {
    m_pcMv     = src->m_pcMv     + offset;
    m_pcMvd    = src->m_pcMvd    + offset;
    m_piRefIdx = src->m_piRefIdx + offset;
  }
  
  Void compress(Char* pePredMode, Int scale); 
};

//! \}

#if ETRIKHU_CLEANUP_H0083
class TComMotionCand
{
public:
  Bool                  m_bAvailable;
  TComMvField           m_cMvField[2];
  UChar                 m_uDir;
#if H_3D_VSP
  Int                   m_iVspFlag;
#endif  
  Bool                  m_bSPIVMPFlag;

public:
  TComMotionCand()
  {
    m_bAvailable = false;
    m_uDir = 0;
#if H_3D_VSP
    m_iVspFlag = 0;
#endif
    m_bSPIVMPFlag = false;
  }

  ~TComMotionCand()
  {

  }

  Void init()
  {
    TComMv cZeroMv;

    m_bAvailable = false;
    m_uDir = 0;
#if H_3D_VSP
    m_iVspFlag = 0;
#endif
    m_bSPIVMPFlag = false;
    m_cMvField[0].setMvField(cZeroMv, NOT_VALID);
    m_cMvField[1].setMvField(cZeroMv, NOT_VALID);
  }

  Void setCand(TComMvField* pcMvFieldNeighbours, UChar uhInterDirNeighbours
#if H_3D_VSP
    , Int vspFlag
#endif
    , Bool bSPIVMPFlag
    )
  {
    m_bAvailable = true;
    m_cMvField[0] = pcMvFieldNeighbours[0];
    m_cMvField[1] = pcMvFieldNeighbours[1];
    m_uDir = uhInterDirNeighbours;
#if H_3D_VSP
    m_iVspFlag = vspFlag;
#endif
    m_bSPIVMPFlag = bSPIVMPFlag;
  }


  Void getCand(Int iCount, TComMvField* pcMvFieldNeighbours, UChar* puhInterDirNeighbours
#if H_3D_VSP
    , Int* vspFlag
#endif
    , Bool* pbSPIVMPFlag
    )
  {
    pcMvFieldNeighbours[iCount<<1] = m_cMvField[0];
    pcMvFieldNeighbours[(iCount<<1) + 1] = m_cMvField[1];
    puhInterDirNeighbours[iCount] = m_uDir;
#if H_3D_VSP
    vspFlag[iCount] = m_iVspFlag;
#endif
    pbSPIVMPFlag[iCount] = m_bSPIVMPFlag;
  }
};
#endif


#endif // __TCOMMOTIONINFO__
