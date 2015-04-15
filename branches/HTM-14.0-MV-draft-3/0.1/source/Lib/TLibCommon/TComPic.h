

/** \file     TComPic.h
    \brief    picture class (header)
*/

#ifndef __TCOMPIC__
#define __TCOMPIC__

// Include files
#include "CommonDef.h"
#include "TComPicSym.h"
#include "TComPicYuv.h"
#include "TComBitStream.h"
#include "TComMVDRefData.h"

class SEImessages;

// ====================================================================================================================
// Class definition
// ====================================================================================================================

/// picture class (symbol + YUV buffers)
class TComPic
{
private:
  TComPicSym*           m_apcPicSym;              //  Symbol
  
  TComPicYuv*           m_apcPicYuv[2];           //  Texture,  0:org / 1:rec
  TComPicYuv*           m_pcPredDepthMap;         //  estimated depth map
  TComPicYuv*           m_pcOrgDepthMap;          //  original depth map
  TComPicYuv*           m_pcResidual;             //  residual buffer (coded or inter-view predicted residual)
  
  TComPicYuv*           m_pcPicYuvPred;           //  Prediction
  TComPicYuv*           m_pcPicYuvResi;           //  Residual
#if PARALLEL_MERGED_DEBLK
  TComPicYuv*           m_pcPicYuvDeblkBuf;
#endif
  Bool                  m_bReconstructed;
  UInt                  m_uiCurrSliceIdx;         // Index of current slice
  
  SEImessages* m_SEIs; ///< Any SEI messages that have been received.  If !NULL we own the object.

//  SB //GT; Why? can be accesed by getSlice()->getXXX?
  SliceType             m_eSliceType;
  double                m_dQP;
  Bool                  m_bReferenced;
  UInt                  m_uiColDir;
  Int                   m_aiRefPOCList[2][MAX_NUM_REF];
  Int                   m_aiRefViewIdxList[2][MAX_NUM_REF];
  Int                   m_aiNumRefIdx[2];    //  for multiple reference of current slice

  Int                   m_iViewIdx;
  Int**                 m_aaiCodedScale;
  Int**                 m_aaiCodedOffset;

  //GT
  TComMVDRefData        m_cReferenceInfo;
  TComPicYuv*           m_pcUsedPelsMap; 


public:
  TComPic();
  virtual ~TComPic();
  

  Void          create( Int iWidth, Int iHeight, UInt uiMaxWidth, UInt uiMaxHeight, UInt uiMaxDepth );
  Void          destroy();
  
  TComPicSym*   getPicSym()           { return  m_apcPicSym;    }
  TComSlice*    getSlice(Int i)       { return  m_apcPicSym->getSlice(i);  }
  TComSlice*    getCurrSlice()        { return  m_apcPicSym->getSlice(m_uiCurrSliceIdx);  }
  TComSPS*      getSPS()              { return  m_apcPicSym->getSlice(m_uiCurrSliceIdx)->getSPS();  }
  Int           getPOC()              { return  m_apcPicSym->getSlice(m_uiCurrSliceIdx)->getPOC();  }
#if 0
  Bool          getDRBFlag()          { return  m_apcPicSym->getSlice(m_uiCurrSliceIdx)->getDRBFlag();  }
  Int           getERBIndex()         { return  m_apcPicSym->getSlice(m_uiCurrSliceIdx)->getERBIndex();  }
#endif
  TComDataCU*&  getCU( UInt uiCUAddr )  { return  m_apcPicSym->getCU( uiCUAddr ); }
  
// SB
  SliceType     getSliceType()        { return m_eSliceType ;}
  double        getQP()               { return m_dQP ;}
  Bool          getReferenced()       { return m_bReferenced ;}

  TComPicYuv*   getPicYuvOrg()        { return  m_apcPicYuv[0]; }
  TComPicYuv*   getPicYuvRec()        { return  m_apcPicYuv[1]; }
  
  TComPicYuv*   getPredDepthMap()     { return  m_pcPredDepthMap; }
  TComPicYuv*   getOrgDepthMap()      { return  m_pcOrgDepthMap; }
  TComPicYuv*   getResidual()         { return  m_pcResidual; }
  TComPicYuv*   getUsedPelsMap()      { return  m_pcUsedPelsMap; }


  TComPicYuv*   getPicYuvPred()       { return  m_pcPicYuvPred; }
  TComPicYuv*   getPicYuvResi()       { return  m_pcPicYuvResi; }
  Void          setPicYuvPred( TComPicYuv* pcPicYuv )       { m_pcPicYuvPred = pcPicYuv; }
  Void          setPicYuvResi( TComPicYuv* pcPicYuv )       { m_pcPicYuvResi = pcPicYuv; }
// SB
  Void          setQP( double dQP )   { m_dQP = dQP; }
  Void          setSliceType( SliceType eSliceType ) { m_eSliceType = eSliceType; }
  Void          setReferenced( Bool bReferenced )    { m_bReferenced = bReferenced; }
  Void          setColDir( UInt uiColDir )           { m_uiColDir = uiColDir; }
  UInt          getColDir()                          { return m_uiColDir; }
  Void          setRefPOC( Int i, RefPicList e, Int iRefIdx ) { m_aiRefPOCList[e][iRefIdx] = i; }
  Int           getRefPOC( RefPicList e, Int iRefIdx )        { return m_aiRefPOCList[e][iRefIdx]; }
  Void          setRefViewIdx( Int i, RefPicList e, Int iRefIdx ) { m_aiRefViewIdxList[e][iRefIdx] = i; }
  Int           getRefViewIdx( RefPicList e, Int iRefIdx )        { return m_aiRefViewIdxList[e][iRefIdx]; }
  Int           getNumRefs( RefPicList e )                    { return m_aiNumRefIdx[e]; }
  Void          setNumRefs( Int i, RefPicList e )             { m_aiNumRefIdx[e] = i; }
  Void          setViewIdx( Int i )                           { m_iViewIdx = i; }
  Int           getViewIdx()                                  { return m_iViewIdx; }

  Void          setScaleOffset( Int** pS, Int** pO )  { m_aaiCodedScale = pS; m_aaiCodedOffset = pO; }
  Int**         getCodedScale ()                      { return m_aaiCodedScale;  }
  Int**         getCodedOffset()                      { return m_aaiCodedOffset; }

//GT
  TComMVDRefData* getMVDReferenceInfo() { return &m_cReferenceInfo; }
  
  UInt          getNumCUsInFrame()      { return m_apcPicSym->getNumberOfCUsInFrame(); }
  UInt          getNumPartInWidth()     { return m_apcPicSym->getNumPartInWidth();     }
  UInt          getNumPartInHeight()    { return m_apcPicSym->getNumPartInHeight();    }
  UInt          getNumPartInCU()        { return m_apcPicSym->getNumPartition();       }
  UInt          getFrameWidthInCU()     { return m_apcPicSym->getFrameWidthInCU();     }
  UInt          getFrameHeightInCU()    { return m_apcPicSym->getFrameHeightInCU();    }
  UInt          getMinCUWidth()         { return m_apcPicSym->getMinCUWidth();         }
  UInt          getMinCUHeight()        { return m_apcPicSym->getMinCUHeight();        }
  
  UInt          getParPelX(UChar uhPartIdx) { return getParPelX(uhPartIdx); }
  UInt          getParPelY(UChar uhPartIdx) { return getParPelX(uhPartIdx); }
  
  Int           getStride()           { return m_apcPicYuv[1]->getStride(); }
  Int           getCStride()          { return m_apcPicYuv[1]->getCStride(); }
  
  Void          setReconMark (Bool b) { m_bReconstructed = b;     }
  Bool          getReconMark ()       { return m_bReconstructed;  }
  
#if AMVP_BUFFERCOMPRESS
  Void          compressMotion(); 
#endif 
  UInt          getCurrSliceIdx()            { return m_uiCurrSliceIdx;                }
  Void          setCurrSliceIdx(UInt i)      { m_uiCurrSliceIdx = i;                   }
  UInt          getNumAllocatedSlice()       {return m_apcPicSym->getNumAllocatedSlice();}
  Void          allocateNewSlice()           {m_apcPicSym->allocateNewSlice();         }
  Void          clearSliceBuffer()           {m_apcPicSym->clearSliceBuffer();         }

  Void          addOriginalBuffer       ();
#if PARALLEL_MERGED_DEBLK
  Void          addDeblockBuffer        ();
#endif
  Void          addPrdDepthMapBuffer    ();
  Void          addOrgDepthMapBuffer    ();
  Void          addResidualBuffer       ();
  Void          addUsedPelsMapBuffer    ();
  
  Void          removeOriginalBuffer    ();
#if PARALLEL_MERGED_DEBLK
  Void          removeDeblockBuffer     ();
#endif
  Void          removePrdDepthMapBuffer ();
  Void          removeOrgDepthMapBuffer ();
  Void          removeResidualBuffer    ();
  Void          removeUsedPelsMapBuffer ();
  
#if PARALLEL_MERGED_DEBLK
  TComPicYuv*   getPicYuvDeblkBuf()      { return  m_pcPicYuvDeblkBuf; }
#endif

  /** transfer ownership of @seis to @this picture */
  void setSEIs(SEImessages* seis) { m_SEIs = seis; }

  /**
   * return the current list of SEI messages associated with this picture.
   * Pointer is valid until @this->destroy() is called */
  SEImessages* getSEIs() { return m_SEIs; }

  /**
   * return the current list of SEI messages associated with this picture.
   * Pointer is valid until @this->destroy() is called */
  const SEImessages* getSEIs() const { return m_SEIs; }

};// END CLASS DEFINITION TComPic


#endif // __TCOMPIC__

