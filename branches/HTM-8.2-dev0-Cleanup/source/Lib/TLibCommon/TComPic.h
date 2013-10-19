/* The copyright in this software is being made available under the BSD
 * License, included below. This software may be subject to other third party
 * and contributor rights, including patent rights, and no such rights are
 * granted under this license.  
 *
 * Copyright (c) 2010-2013, ITU/ISO/IEC
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
#include "SEI.h"

//! \ingroup TLibCommon
//! \{

// ====================================================================================================================
// Class definition
// ====================================================================================================================

/// picture class (symbol + YUV buffers)
class TComPic
{
private:
  UInt                  m_uiTLayer;               //  Temporal layer
  Bool                  m_bUsedByCurr;            //  Used by current picture
  Bool                  m_bIsLongTerm;            //  IS long term picture
  Bool                  m_bIsUsedAsLongTerm;      //  long term picture is used as reference before
  TComPicSym*           m_apcPicSym;              //  Symbol
  
  TComPicYuv*           m_apcPicYuv[2];           //  Texture,  0:org / 1:rec
  
  TComPicYuv*           m_pcPicYuvPred;           //  Prediction
  TComPicYuv*           m_pcPicYuvResi;           //  Residual
  Bool                  m_bReconstructed;
  Bool                  m_bNeededForOutput;
  UInt                  m_uiCurrSliceIdx;         // Index of current slice
  Int*                  m_pSliceSUMap;
  Bool*                 m_pbValidSlice;
  Int                   m_sliceGranularityForNDBFilter;
  Bool                  m_bIndependentSliceBoundaryForNDBFilter;
  Bool                  m_bIndependentTileBoundaryForNDBFilter;
  TComPicYuv*           m_pNDBFilterYuvTmp;    //!< temporary picture buffer when non-cross slice/tile boundary in-loop filtering is enabled
  Bool                  m_bCheckLTMSB;
  
  Int                   m_numReorderPics[MAX_TLAYER];
  Window                m_conformanceWindow;
  Window                m_defaultDisplayWindow;

  std::vector<std::vector<TComDataCU*> > m_vSliceCUDataLink;

  SEIMessages  m_SEIs; ///< Any SEI messages that have been received.  If !NULL we own the object.

#if H_MV
  Int                   m_layerId;
  Int                   m_viewId;
#if H_3D
  Int                   m_viewIndex;
  Bool                  m_isDepth;
  Int**                 m_aaiCodedScale;
  Int**                 m_aaiCodedOffset;
#endif
#endif
#if H_3D_QTLPC
  Bool                  m_bReduceBitsQTL;
#endif
#if H_3D_NBDV
  UInt        m_uiRapRefIdx;
  RefPicList  m_eRapRefList;
  Int         m_iNumDdvCandPics;
  Bool        m_abTIVRINCurrRL  [2][2][MAX_NUM_REF]; //whether an inter-view reference picture with the same view index of the inter-view reference picture of temporal reference picture of current picture exists in current reference picture lists
  Int         m_aiTexToDepRef  [2][MAX_NUM_REF];
#endif
public:
  TComPic();
  virtual ~TComPic();
  
  Void          create( Int iWidth, Int iHeight, UInt uiMaxWidth, UInt uiMaxHeight, UInt uiMaxDepth, Window &conformanceWindow, Window &defaultDisplayWindow, 
                        Int *numReorderPics, Bool bIsVirtual = false );
                        
  virtual Void  destroy();
  
  UInt          getTLayer()                { return m_uiTLayer;   }
  Void          setTLayer( UInt uiTLayer ) { m_uiTLayer = uiTLayer; }

#if H_MV
  Void          setLayerId            ( Int layerId )    { m_layerId      = layerId; }
  Int           getLayerId            ()                 { return m_layerId;    }
  Void          setViewId             ( Int viewId )     { m_viewId = viewId;   }
  Int           getViewId             ()                 { return m_viewId;     }
#if H_3D
  Void          setViewIndex          ( Int viewIndex )  { m_viewIndex = viewIndex;   }
  Int           getViewIndex          ()                 { return m_viewIndex;     }

  Void          setIsDepth            ( Bool isDepth )   { m_isDepth = isDepth; }
  Bool          getIsDepth            ()                 { return m_isDepth; }

  Void          setScaleOffset( Int** pS, Int** pO )  { m_aaiCodedScale = pS; m_aaiCodedOffset = pO; }
  Int**         getCodedScale ()                      { return m_aaiCodedScale;  }
  Int**         getCodedOffset()                      { return m_aaiCodedOffset; }
#endif
#endif
#if H_3D_QTLPC
  Bool          getReduceBitsFlag ()             { return m_bReduceBitsQTL;     }
  Void          setReduceBitsFlag ( Bool bFlag ) { m_bReduceBitsQTL = bFlag;    }
#endif
  Bool          getUsedByCurr()             { return m_bUsedByCurr; }
  Void          setUsedByCurr( Bool bUsed ) { m_bUsedByCurr = bUsed; }
  Bool          getIsLongTerm()             { return m_bIsLongTerm; }
  Void          setIsLongTerm( Bool lt ) { m_bIsLongTerm = lt; }
  Void          setCheckLTMSBPresent     (Bool b ) {m_bCheckLTMSB=b;}
  Bool          getCheckLTMSBPresent     () { return m_bCheckLTMSB;}

  TComPicSym*   getPicSym()           { return  m_apcPicSym;    }
  TComSlice*    getSlice(Int i)       { return  m_apcPicSym->getSlice(i);  }
  Int           getPOC()              { return  m_apcPicSym->getSlice(m_uiCurrSliceIdx)->getPOC();  }
  TComDataCU*&  getCU( UInt uiCUAddr )  { return  m_apcPicSym->getCU( uiCUAddr ); }
  
  TComPicYuv*   getPicYuvOrg()        { return  m_apcPicYuv[0]; }
  TComPicYuv*   getPicYuvRec()        { return  m_apcPicYuv[1]; }
  
  TComPicYuv*   getPicYuvPred()       { return  m_pcPicYuvPred; }
  TComPicYuv*   getPicYuvResi()       { return  m_pcPicYuvResi; }
  Void          setPicYuvPred( TComPicYuv* pcPicYuv )       { m_pcPicYuvPred = pcPicYuv; }
  Void          setPicYuvResi( TComPicYuv* pcPicYuv )       { m_pcPicYuvResi = pcPicYuv; }
  
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
  Void          setOutputMark (Bool b) { m_bNeededForOutput = b;     }
  Bool          getOutputMark ()       { return m_bNeededForOutput;  }
 
  Void          setNumReorderPics(Int i, UInt tlayer) { m_numReorderPics[tlayer] = i;    }
  Int           getNumReorderPics(UInt tlayer)        { return m_numReorderPics[tlayer]; }
#if H_3D
  Void          compressMotion(Int scale); 
#else   
  Void          compressMotion(); 
#endif
  UInt          getCurrSliceIdx()            { return m_uiCurrSliceIdx;                }
  Void          setCurrSliceIdx(UInt i)      { m_uiCurrSliceIdx = i;                   }
  UInt          getNumAllocatedSlice()       {return m_apcPicSym->getNumAllocatedSlice();}
  Void          allocateNewSlice()           {m_apcPicSym->allocateNewSlice();         }
  Void          clearSliceBuffer()           {m_apcPicSym->clearSliceBuffer();         }

  Window&       getConformanceWindow()  { return m_conformanceWindow; }
  Window&       getDefDisplayWindow()   { return m_defaultDisplayWindow; }

  Void          createNonDBFilterInfo   (std::vector<Int> sliceStartAddress, Int sliceGranularityDepth
                                        ,std::vector<Bool>* LFCrossSliceBoundary
                                        ,Int  numTiles = 1
                                        ,Bool bNDBFilterCrossTileBoundary = true);
  Void          createNonDBFilterInfoLCU(Int tileID, Int sliceID, TComDataCU* pcCU, UInt startSU, UInt endSU, Int sliceGranularyDepth, UInt picWidth, UInt picHeight);
  Void          destroyNonDBFilterInfo();

  Bool          getValidSlice                                  (Int sliceID)  {return m_pbValidSlice[sliceID];}
  Bool          getIndependentSliceBoundaryForNDBFilter        ()             {return m_bIndependentSliceBoundaryForNDBFilter;}
  Bool          getIndependentTileBoundaryForNDBFilter         ()             {return m_bIndependentTileBoundaryForNDBFilter; }
  TComPicYuv*   getYuvPicBufferForIndependentBoundaryProcessing()             {return m_pNDBFilterYuvTmp;}
  std::vector<TComDataCU*>& getOneSliceCUDataForNDBFilter      (Int sliceID) { return m_vSliceCUDataLink[sliceID];}

#if H_MV
  Void          print( Bool legend );
#endif
#if H_3D_NBDV
  Int           getNumDdvCandPics()                    {return m_iNumDdvCandPics;   }
  Int           getDisCandRefPictures(Int iColPOC);
  Void          setRapRefIdx(UInt uiRapRefIdx)         {m_uiRapRefIdx = uiRapRefIdx;}
  Void          setRapRefList(RefPicList eRefPicList)  {m_eRapRefList = eRefPicList;}
  Void          setNumDdvCandPics (Int i)              {m_iNumDdvCandPics = i;       }
  UInt          getRapRefIdx()                         {return m_uiRapRefIdx;       }
  RefPicList    getRapRefList()                        {return m_eRapRefList;       }
  Void          checkTemporalIVRef();
  Bool          isTempIVRefValid(Int currCandPic, Int iTempRefDir, Int iTempRefIdx);
  Void          checkTextureRef(  );
  Int           isTextRefValid(Int iTextRefDir, Int iTextRefIdx);
#endif
  /** transfer ownership of seis to this picture */
  void setSEIs(SEIMessages& seis) { m_SEIs = seis; }

  /**
   * return the current list of SEI messages associated with this picture.
   * Pointer is valid until this->destroy() is called */
  SEIMessages& getSEIs() { return m_SEIs; }

  /**
   * return the current list of SEI messages associated with this picture.
   * Pointer is valid until this->destroy() is called */
  const SEIMessages& getSEIs() const { return m_SEIs; }

};// END CLASS DEFINITION TComPic

#if H_MV
class TComPicLists 
{
private: 
  TComList<TComList<TComPic*>*> m_lists; 
#if H_3D
  TComVPS*                     m_vps; 
#endif
public: 
  Void        push_back( TComList<TComPic*>* list ) { m_lists.push_back( list );   }
  Int         size     ()                           { return (Int) m_lists.size(); } 
#if H_3D_ARP
  TComList<TComPic*>*  getPicList   ( Int layerIdInNuh );
#endif
  TComPic*    getPic   ( Int layerIdInNuh,              Int poc );    
  TComPicYuv* getPicYuv( Int layerIdInNuh,              Int poc, Bool recon ); 
#if H_3D
  Void        setVPS   ( TComVPS* vps ) { m_vps = vps;  }; 
  TComPic*    getPic   ( Int viewIndex, Bool depthFlag, Int poc );
  TComPicYuv* getPicYuv( Int viewIndex, Bool depthFlag, Int poc, Bool recon );
#endif  

  Void print( );  

}; // END CLASS DEFINITION TComPicLists

#endif 
//! \}

#endif // __TCOMPIC__
