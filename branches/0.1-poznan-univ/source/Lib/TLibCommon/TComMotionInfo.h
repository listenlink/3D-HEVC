

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

