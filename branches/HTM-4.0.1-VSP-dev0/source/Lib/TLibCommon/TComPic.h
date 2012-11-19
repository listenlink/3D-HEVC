/* The copyright in this software is being made available under the BSD
 * License, included below. This software may be subject to other third party
 * and contributor rights, including patent rights, and no such rights are
 * granted under this license.  
 *
 * Copyright (c) 2010-2012, ITU/ISO/IEC
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

//! \ingroup TLibCommon
//! \{

class SEImessages;

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
  TComPicSym*           m_apcPicSym;              //  Symbol
  
  TComPicYuv*           m_apcPicYuv[2];           //  Texture,  0:org / 1:rec
  
#if VSP_N
  TComPicYuv*           m_apcPicYuvAvail;         //  Availability Map - Does the given pixel can be synthesised in receiver
  TComPicYuv*           m_apcPicYuvSynth;         //  Sythesied image
#endif

#if DEPTH_MAP_GENERATION
  TComPicYuv*           m_pcPredDepthMap;         //  estimated depth map
#if PDM_REMOVE_DEPENDENCE
  TComPicYuv*           m_pcPredDepthMap_temp;         //  estimated depth map
  Bool                  m_bPDMV2;                       
#endif
#endif

#if LG_ZEROINTRADEPTHRESI_M26039
  Int                   m_uiIntraPeriod;
#endif

#if HHI_INTER_VIEW_MOTION_PRED
  TComPicYuv*           m_pcOrgDepthMap;          //  original depth map
#if QC_MULTI_DIS_CAN
  Bool          m_checked;
#endif
#endif
#if HHI_INTER_VIEW_RESIDUAL_PRED
  TComPicYuv*           m_pcResidual;             //  residual buffer (coded or inter-view predicted residual)
#endif

  TComPicYuv*           m_pcPicYuvPred;           //  Prediction
  TComPicYuv*           m_pcPicYuvResi;           //  Residual
  Bool                  m_bReconstructed;
  Bool                  m_bNeededForOutput;
  UInt                  m_uiCurrSliceIdx;         // Index of current slice

  Bool                  m_usedForTMVP;
  
  Int*                  m_pSliceSUMap;
  Bool*                 m_pbValidSlice;
  Int                   m_sliceGranularityForNDBFilter;
  Bool                  m_bIndependentSliceBoundaryForNDBFilter;
  Bool                  m_bIndependentTileBoundaryForNDBFilter;
  TComPicYuv*           m_pNDBFilterYuvTmp;    //!< temporary picture buffer when non-cross slice/tile boundary in-loop filtering is enabled
  std::vector<std::vector<TComDataCU*> > m_vSliceCUDataLink;

  SEImessages* m_SEIs; ///< Any SEI messages that have been received.  If !NULL we own the object.
#if HHI_INTERVIEW_SKIP
  TComPicYuv*           m_pcUsedPelsMap;
#endif
#if SONY_COLPIC_AVAILABILITY
  Int                   m_iViewOrderIdx;
#endif
  Int**                 m_aaiCodedScale;
  Int**                 m_aaiCodedOffset;

#if OL_DEPTHLIMIT_A0044
  UInt*                 m_texPartInfo;
  UInt                  m_uiTexPartIndex;
#endif

public:
  TComPic();
  virtual ~TComPic();
  
  Void          create( Int iWidth, Int iHeight, UInt uiMaxWidth, UInt uiMaxHeight, UInt uiMaxDepth, Bool bIsVirtual = false );
  virtual Void  destroy();
  
  UInt          getTLayer()                { return m_uiTLayer;   }
  Void          setTLayer( UInt uiTLayer ) { m_uiTLayer = uiTLayer; }

  Bool          getUsedByCurr()             { return m_bUsedByCurr; }
  Void          setUsedByCurr( Bool bUsed ) { m_bUsedByCurr = bUsed; }
  Bool          getIsLongTerm()             { return m_bIsLongTerm; }
  Void          setIsLongTerm( Bool lt ) { m_bIsLongTerm = lt; }

  TComPicSym*   getPicSym()           { return  m_apcPicSym;    }
  TComSlice*    getSlice(Int i)       { return  m_apcPicSym->getSlice(i);  }
  TComSlice*    getCurrSlice()        { return  m_apcPicSym->getSlice(m_uiCurrSliceIdx);  }
#if VIDYO_VPS_INTEGRATION
  TComVPS*      getVPS()              { return  m_apcPicSym->getSlice(m_uiCurrSliceIdx)->getVPS();  }
#endif
#if LG_ZEROINTRADEPTHRESI_M26039
  Int           getIntraPeriod()                           { return  m_uiIntraPeriod; }
  Void          setIntraPeriod(Int uiIntraPeriod)          { m_uiIntraPeriod = uiIntraPeriod; }
#endif
  TComSPS*      getSPS()              { return  m_apcPicSym->getSlice(m_uiCurrSliceIdx)->getSPS();  }
  Int           getPOC()              { return  m_apcPicSym->getSlice(m_uiCurrSliceIdx)->getPOC();  }
  Int           getViewId()           { return  m_apcPicSym->getSlice(m_uiCurrSliceIdx)->getViewId(); }
  TComDataCU*&  getCU( UInt uiCUAddr ){ return  m_apcPicSym->getCU( uiCUAddr ); }
  
  TComPicYuv*   getPicYuvOrg()        { return  m_apcPicYuv[0]; }
  TComPicYuv*   getPicYuvRec()        { return  m_apcPicYuv[1]; }
#if VSP_N
  Void          setPicYuvAvail( TComPicYuv* pc ){ m_apcPicYuvAvail = pc; }
  Void          setPicYuvSynth( TComPicYuv* pc ){ m_apcPicYuvSynth = pc; }
  TComPicYuv*   getPicYuvAvail()      { return  m_apcPicYuvAvail; }
  TComPicYuv*   getPicYuvSynth()      { return  m_apcPicYuvSynth; }
  Void          checkSynthesisAvailability(  /*TComDataCU*& rpcBestCU, */UInt iCuAddr, UInt uiAbsZorderIdx, UInt uiPartDepth, Bool *&rpbCUSynthesied);
#endif
#if HHI_INTERVIEW_SKIP
  TComPicYuv*   getUsedPelsMap()      { return  m_pcUsedPelsMap; }
#endif
  
#if DEPTH_MAP_GENERATION
  TComPicYuv*   getPredDepthMap()     { return  m_pcPredDepthMap; }
#if PDM_REMOVE_DEPENDENCE
  TComPicYuv*   getPredDepthMapTemp()           { return  m_pcPredDepthMap_temp; }
  Void          setStoredPDMforV2  (Bool flag)  { m_bPDMV2 = flag;}
  Bool          getStoredPDMforV2  ()           { return m_bPDMV2;}
#endif

#endif
#if HHI_INTER_VIEW_MOTION_PRED
  TComPicYuv*   getOrgDepthMap()      { return  m_pcOrgDepthMap; }
#if QC_MULTI_DIS_CAN
  Void          setCandPicCheckedFlag (Bool bchecked)   { m_checked = bchecked; }
  Bool          getCandPicCheckedFlag ()                { return m_checked;}
#endif
#endif

#if HHI_INTER_VIEW_RESIDUAL_PRED
  TComPicYuv*   getResidual()         { return  m_pcResidual; }
#endif

#if SONY_COLPIC_AVAILABILITY
  Void          setViewOrderIdx(Int i)                        { m_iViewOrderIdx = i; }
  Int           getViewOrderIdx()                             { return m_iViewOrderIdx; }
#endif
  Void          setScaleOffset( Int** pS, Int** pO )  { m_aaiCodedScale = pS; m_aaiCodedOffset = pO; }
  Int**         getCodedScale ()                      { return m_aaiCodedScale;  }
  Int**         getCodedOffset()                      { return m_aaiCodedOffset; }

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

  Void          setUsedForTMVP( Bool b ) { m_usedForTMVP = b;    }
  Bool          getUsedForTMVP()         { return m_usedForTMVP; }

  Void          setOutputMark (Bool b) { m_bNeededForOutput = b;     }
  Bool          getOutputMark ()       { return m_bNeededForOutput;  }
 
  Void          compressMotion(); 
  UInt          getCurrSliceIdx()            { return m_uiCurrSliceIdx;                }
  Void          setCurrSliceIdx(UInt i)      { m_uiCurrSliceIdx = i;                   }
  UInt          getNumAllocatedSlice()       {return m_apcPicSym->getNumAllocatedSlice();}
  Void          allocateNewSlice()           {m_apcPicSym->allocateNewSlice();         }
  Void          clearSliceBuffer()           {m_apcPicSym->clearSliceBuffer();         }
#if HHI_INTERVIEW_SKIP
  Void          addUsedPelsMapBuffer    ();
  Void          removeUsedPelsMapBuffer ();
#endif
  
  Void          createNonDBFilterInfo   (UInt* pSliceStartAddress = NULL, Int numSlices = 1, Int sliceGranularityDepth= 0
                                        ,Bool bNDBFilterCrossSliceBoundary = true
                                        ,Int  numTiles = 1
                                        ,Bool bNDBFilterCrossTileBoundary = true);
  Void          createNonDBFilterInfoLCU(Int tileID, Int sliceID, TComDataCU* pcCU, UInt startSU, UInt endSU, Int sliceGranularyDepth, UInt picWidth, UInt picHeight);
  Void          destroyNonDBFilterInfo();

#if DEPTH_MAP_GENERATION
  Void          addPrdDepthMapBuffer    ( UInt uiSubSampExpX, UInt uiSubSampExpY );
#endif
#if HHI_INTER_VIEW_MOTION_PRED
  Void          addOrgDepthMapBuffer    ();
#endif
#if HHI_INTER_VIEW_RESIDUAL_PRED
  Void          addResidualBuffer       ();
#endif
#if DEPTH_MAP_GENERATION
  Void          removePrdDepthMapBuffer ();
#endif
#if HHI_INTER_VIEW_MOTION_PRED
  Void          removeOrgDepthMapBuffer ();
#endif
#if HHI_INTER_VIEW_RESIDUAL_PRED
  Void          removeResidualBuffer    ();
#endif

#if OL_DEPTHLIMIT_A0044
  UInt        accessPartInfo        ( UInt count )   { return m_texPartInfo[m_uiTexPartIndex + count]; };
  Void        incrementTexPartIndex (            )   { m_uiTexPartIndex += 2;    };
  UInt        getTexPartIndex       ()               { return m_uiTexPartIndex;  };
  Void        setTexPartIndex       ( UInt idx   )   { m_uiTexPartIndex = idx; };
  Void        setPartInfo           ( UInt* texPart) { m_texPartInfo    = texPart;  };
#endif

  Bool          getValidSlice                                  (Int sliceID)  {return m_pbValidSlice[sliceID];}
  Int           getSliceGranularityForNDBFilter                ()             {return m_sliceGranularityForNDBFilter;}
  Bool          getIndependentSliceBoundaryForNDBFilter        ()             {return m_bIndependentSliceBoundaryForNDBFilter;}
  Bool          getIndependentTileBoundaryForNDBFilter         ()             {return m_bIndependentTileBoundaryForNDBFilter; }
  TComPicYuv*   getYuvPicBufferForIndependentBoundaryProcessing()             {return m_pNDBFilterYuvTmp;}
  std::vector<TComDataCU*>& getOneSliceCUDataForNDBFilter      (Int sliceID) { return m_vSliceCUDataLink[sliceID];}

  /** transfer ownership of seis to this picture */
  void setSEIs(SEImessages* seis) { m_SEIs = seis; }

  /**
   * return the current list of SEI messages associated with this picture.
   * Pointer is valid until this->destroy() is called */
  SEImessages* getSEIs() { return m_SEIs; }

  /**
   * return the current list of SEI messages associated with this picture.
   * Pointer is valid until this->destroy() is called */
  const SEImessages* getSEIs() const { return m_SEIs; }

};// END CLASS DEFINITION TComPic

//! \}

#endif // __TCOMPIC__
