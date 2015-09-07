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

// ====================================================================================================================
// Class definition
// ====================================================================================================================

/// picture class (symbol + YUV buffers)



#if NH_MV
class TComPic; 

class TComDecodedRps
{
public: 

  TComDecodedRps()
  {
    m_refPicSetsCurr[0] = &m_refPicSetStCurrBefore;
    m_refPicSetsCurr[1] = &m_refPicSetStCurrAfter ;
    m_refPicSetsCurr[2] = &m_refPicSetLtCurr      ; 

    m_refPicSetsLt  [0] = &m_refPicSetLtCurr      ; 
    m_refPicSetsLt  [1] = &m_refPicSetLtFoll      ;

    m_refPicSetsAll [0] = &m_refPicSetStCurrBefore;
    m_refPicSetsAll [1] = &m_refPicSetStCurrAfter ;
    m_refPicSetsAll [2] = &m_refPicSetStFoll      ;
    m_refPicSetsAll [3] = &m_refPicSetLtCurr      ;
    m_refPicSetsAll [4] = &m_refPicSetLtFoll      ;
  };    

  std::vector<Int>       m_pocStCurrBefore;
  std::vector<Int>       m_pocStCurrAfter;
  std::vector<Int>       m_pocStFoll;    
  std::vector<Int>       m_pocLtCurr;
  std::vector<Int>       m_pocLtFoll; 

  Int                    m_numPocStCurrBefore;
  Int                    m_numPocStCurrAfter;
  Int                    m_numPocStFoll;
  Int                    m_numPocLtCurr;
  Int                    m_numPocLtFoll;

  std::vector<TComPic*>  m_refPicSetStCurrBefore; 
  std::vector<TComPic*>  m_refPicSetStCurrAfter; 
  std::vector<TComPic*>  m_refPicSetStFoll; 
  std::vector<TComPic*>  m_refPicSetLtCurr; 
  std::vector<TComPic*>  m_refPicSetLtFoll;   

  std::vector<TComPic*>* m_refPicSetsCurr[3];
  std::vector<TComPic*>* m_refPicSetsLt  [2];
  std::vector<TComPic*>* m_refPicSetsAll [5];

  // Annex F
  Int                    m_numActiveRefLayerPics0;
  Int                    m_numActiveRefLayerPics1;     

  std::vector<TComPic*>  m_refPicSetInterLayer0;
  std::vector<TComPic*>  m_refPicSetInterLayer1;
};
#endif

class TComPic
{
public:
  typedef enum { PIC_YUV_ORG=0, PIC_YUV_REC=1, PIC_YUV_TRUE_ORG=2, NUM_PIC_YUV=3 } PIC_YUV_T;
     // TRUE_ORG is the input file without any pre-encoder colour space conversion (but with possible bit depth increment)
  TComPicYuv*   getPicYuvTrueOrg()        { return  m_apcPicYuv[PIC_YUV_TRUE_ORG]; }

private:
  UInt                  m_uiTLayer;               //  Temporal layer
  Bool                  m_bUsedByCurr;            //  Used by current picture
  Bool                  m_bIsLongTerm;            //  IS long term picture
  TComPicSym            m_picSym;                 //  Symbol
  TComPicYuv*           m_apcPicYuv[NUM_PIC_YUV];

  TComPicYuv*           m_pcPicYuvPred;           //  Prediction
  TComPicYuv*           m_pcPicYuvResi;           //  Residual
  Bool                  m_bReconstructed;
  Bool                  m_bNeededForOutput;

  UInt                  m_uiCurrSliceIdx;         // Index of current slice
  Bool                  m_bCheckLTMSB;

  Bool                  m_isTop;
  Bool                  m_isField;

  std::vector<std::vector<TComDataCU*> > m_vSliceCUDataLink;

  SEIMessages  m_SEIs; ///< Any SEI messages that have been received.  If !NULL we own the object.
#if NH_MV
  Int                   m_layerId;
  Int                   m_viewId;
  Bool                  m_bPicOutputFlag;                // Semantics variable 
  Bool                  m_hasGeneratedRefPics; 
  Bool                  m_isPocResettingPic; 
  Bool                  m_isFstPicOfAllLayOfPocResetPer; 
  Int64                 m_decodingOrder; 
  Bool                  m_noRaslOutputFlag; 
  Bool                  m_noClrasOutputFlag; 
  Int                   m_picLatencyCount; 
  Bool                  m_isGenerated;
  Bool                  m_isGeneratedCl833; 
  Bool                  m_activatesNewVps;
  TComDecodedRps        m_decodedRps;
#endif
#if NH_3D
  Int                   m_viewIndex;
  Bool                  m_isDepth;
  Int**                 m_aaiCodedScale;
  Int**                 m_aaiCodedOffset;
#if NH_3D_QTLPC
  Bool                  m_bReduceBitsQTL;
#endif
#if NH_3D_NBDV
  UInt                   m_uiRapRefIdx;
  RefPicList             m_eRapRefList;
  Int                    m_iNumDdvCandPics;
  Bool                   m_abTIVRINCurrRL [2][2][MAX_NUM_REF]; //whether an inter-view reference picture with the same view index of the inter-view reference picture of temporal reference picture of current picture exists in current reference picture lists
  Int                    m_aiTexToDepRef  [2][MAX_NUM_REF];
#endif
#endif
public:
  TComPic();
  virtual ~TComPic();

  Void          create( const TComSPS &sps, const TComPPS &pps, const Bool bIsVirtual /*= false*/ );

  virtual Void  destroy();

  UInt          getTLayer() const               { return m_uiTLayer;   }
  Void          setTLayer( UInt uiTLayer ) { m_uiTLayer = uiTLayer; }

  Bool          getUsedByCurr() const            { return m_bUsedByCurr; }
  Void          setUsedByCurr( Bool bUsed ) { m_bUsedByCurr = bUsed; }
  Bool          getIsLongTerm() const            { return m_bIsLongTerm; }
  Void          setIsLongTerm( Bool lt ) { m_bIsLongTerm = lt; }
  Void          setCheckLTMSBPresent     (Bool b ) {m_bCheckLTMSB=b;}
  Bool          getCheckLTMSBPresent     () { return m_bCheckLTMSB;}

  TComPicSym*   getPicSym()           { return  &m_picSym;    }
  TComSlice*    getSlice(Int i)       { return  m_picSym.getSlice(i);  }
  Int           getPOC() const        { return  m_picSym.getSlice(m_uiCurrSliceIdx)->getPOC();  }
  TComDataCU*   getCtu( UInt ctuRsAddr )           { return  m_picSym.getCtu( ctuRsAddr ); }
  const TComDataCU* getCtu( UInt ctuRsAddr ) const { return  m_picSym.getCtu( ctuRsAddr ); }

  TComPicYuv*   getPicYuvOrg()        { return  m_apcPicYuv[PIC_YUV_ORG]; }
  TComPicYuv*   getPicYuvRec()        { return  m_apcPicYuv[PIC_YUV_REC]; }

  TComPicYuv*   getPicYuvPred()       { return  m_pcPicYuvPred; }
  TComPicYuv*   getPicYuvResi()       { return  m_pcPicYuvResi; }
  Void          setPicYuvPred( TComPicYuv* pcPicYuv )       { m_pcPicYuvPred = pcPicYuv; }
  Void          setPicYuvResi( TComPicYuv* pcPicYuv )       { m_pcPicYuvResi = pcPicYuv; }

  UInt          getNumberOfCtusInFrame() const     { return m_picSym.getNumberOfCtusInFrame(); }
  UInt          getNumPartInCtuWidth() const       { return m_picSym.getNumPartInCtuWidth();   }
  UInt          getNumPartInCtuHeight() const      { return m_picSym.getNumPartInCtuHeight();  }
  UInt          getNumPartitionsInCtu() const      { return m_picSym.getNumPartitionsInCtu();  }
  UInt          getFrameWidthInCtus() const        { return m_picSym.getFrameWidthInCtus();    }
  UInt          getFrameHeightInCtus() const       { return m_picSym.getFrameHeightInCtus();   }
  UInt          getMinCUWidth() const              { return m_picSym.getMinCUWidth();          }
  UInt          getMinCUHeight() const             { return m_picSym.getMinCUHeight();         }

  Int           getStride(const ComponentID id) const          { return m_apcPicYuv[PIC_YUV_REC]->getStride(id); }
  Int           getComponentScaleX(const ComponentID id) const    { return m_apcPicYuv[PIC_YUV_REC]->getComponentScaleX(id); }
  Int           getComponentScaleY(const ComponentID id) const    { return m_apcPicYuv[PIC_YUV_REC]->getComponentScaleY(id); }
  ChromaFormat  getChromaFormat() const                           { return m_apcPicYuv[PIC_YUV_REC]->getChromaFormat(); }
  Int           getNumberValidComponents() const                  { return m_apcPicYuv[PIC_YUV_REC]->getNumberValidComponents(); }

  Void          setReconMark (Bool b) { m_bReconstructed = b;     }
  Bool          getReconMark () const      { return m_bReconstructed;  }
  Void          setOutputMark (Bool b) { m_bNeededForOutput = b;     }
  Bool          getOutputMark () const      { return m_bNeededForOutput;  }

#if !NH_3D
  Void          compressMotion();
#endif
  UInt          getCurrSliceIdx() const           { return m_uiCurrSliceIdx;                }
  Void          setCurrSliceIdx(UInt i)      { m_uiCurrSliceIdx = i;                   }
  UInt          getNumAllocatedSlice() const      {return m_picSym.getNumAllocatedSlice();}
  Void          allocateNewSlice()           {m_picSym.allocateNewSlice();         }
  Void          clearSliceBuffer()           {m_picSym.clearSliceBuffer();         }

  const Window& getConformanceWindow() const { return m_picSym.getSPS().getConformanceWindow(); }
  Window        getDefDisplayWindow() const  { return m_picSym.getSPS().getVuiParametersPresentFlag() ? m_picSym.getSPS().getVuiParameters()->getDefaultDisplayWindow() : Window(); }

  Bool          getSAOMergeAvailability(Int currAddr, Int mergeAddr);

  UInt          getSubstreamForCtuAddr(const UInt ctuAddr, const Bool bAddressInRaster, TComSlice *pcSlice);

  /* field coding parameters*/

   Void              setTopField(Bool b)                  {m_isTop = b;}
   Bool              isTopField()                         {return m_isTop;}
   Void              setField(Bool b)                     {m_isField = b;}
   Bool              isField()                            {return m_isField;}

#if NH_MV
   Void          setLayerId            ( Int layerId )    { m_layerId      = layerId; }
   Int           getLayerId            ()                 { return m_layerId;    }
   
   Void          setViewId             ( Int viewId )     { m_viewId = viewId;   }
   Int           getViewId             ()                 { return m_viewId;     }

   Void          setPicOutputFlag(Bool b)                 { m_bPicOutputFlag = b;      }
   Bool          getPicOutputFlag()                       { return m_bPicOutputFlag ;  }

   Bool          getPocResetPeriodId();

   Void          markAsUsedForShortTermReference();
   Void          markAsUsedForLongTermReference(); 
   Void          markAsUnusedForReference(); 

   Bool          getMarkedUnUsedForReference();
   Bool          getMarkedAsShortTerm();

   Void          setHasGeneratedRefPics(Bool val)       { m_hasGeneratedRefPics  = val;    }
   Bool          getHasGeneratedRefPics( )              { return m_hasGeneratedRefPics;   }

   Void          setIsPocResettingPic(Bool val)         { m_isPocResettingPic = val;    }
   Bool          getIsPocResettingPic( )                { return m_isPocResettingPic;   }

   Void          setIsFstPicOfAllLayOfPocResetPer(Bool val) { m_isFstPicOfAllLayOfPocResetPer = val;  }
   Bool          getIsFstPicOfAllLayOfPocResetPer( )        { return m_isFstPicOfAllLayOfPocResetPer; }

   Int64         getDecodingOrder( )                    { return m_decodingOrder;       }
   Void          setDecodingOrder( UInt64 val  )        { m_decodingOrder = val;        }

   Bool          getNoRaslOutputFlag()                  { return m_noRaslOutputFlag;     }
   Void          setNoRaslOutputFlag( Bool b )          { m_noRaslOutputFlag = b;        }

   Bool          getNoClrasOutputFlag()                 { return m_noClrasOutputFlag;    }
   Void          setNoClrasOutputFlag( Bool b )         { m_noClrasOutputFlag = b;       }

   Int           getPicLatencyCount()                   { return m_picLatencyCount;      }
   Void          setPicLatencyCount( Int val )          { m_picLatencyCount = val;       }

   Bool          getIsGenerated() const                 { return m_isGenerated;          }
   Void          setIsGenerated( Bool b )               { m_isGenerated = b;             }

   Bool          getIsGeneratedCl833() const            { return m_isGeneratedCl833;     }
   Void          setIsGeneratedCl833( Bool b )          { m_isGeneratedCl833 = b;        }

   Int           getTemporalId( )                       { return getSlice(0)->getTemporalId(); }

   Bool          getActivatesNewVps()                   { return m_activatesNewVps;      }
   Void          setActivatesNewVps( Bool b )           { m_activatesNewVps = b;         }

   TComDecodedRps* getDecodedRps()                      { return &m_decodedRps;          }

   Bool          isIrap()                               { return getSlice(0)->isIRAP(); } 
   Bool          isBla ()                               { return getSlice(0)->isBla (); } 
   Bool          isIdr ()                               { return getSlice(0)->isIdr (); } 
   Bool          isCra ()                               { return getSlice(0)->isCra (); } 
   Bool          isSlnr ()                              { return getSlice(0)->isSlnr (); }
   Bool          isRasl ()                              { return getSlice(0)->isRasl (); }
   Bool          isRadl ()                              { return getSlice(0)->isRadl (); }
   Bool          isStsa ()                              { return getSlice(0)->isStsa (); }
   Bool          isTsa ()                               { return getSlice(0)->isTsa  (); }

   Void          print( Int outputLevel );

#if NH_3D
   Void          setViewIndex          ( Int viewIndex )  { m_viewIndex = viewIndex;   }
   Int           getViewIndex          ()                 { return m_viewIndex;     }

   Void          setIsDepth            ( Bool isDepth )   { m_isDepth = isDepth; }
   Bool          getIsDepth            ()                 { return m_isDepth; }

   Void          setScaleOffset( Int** pS, Int** pO )     { m_aaiCodedScale = pS; m_aaiCodedOffset = pO; }
   Int**         getCodedScale ()                         { return m_aaiCodedScale;  }
   Int**         getCodedOffset()                         { return m_aaiCodedOffset; }

   Void          compressMotion(Int scale); 
   Void          printMotion( );
#if NH_3D_ARP
   Void          getCUAddrAndPartIdx( Int iX, Int iY, Int& riCuAddr, Int& riAbsZorderIdx );
#endif
#if NH_3D_QTLPC
   Bool          getReduceBitsFlag ()                     { return m_bReduceBitsQTL;     }
   Void          setReduceBitsFlag ( Bool bFlag )         { m_bReduceBitsQTL = bFlag;    }
#endif
#if NH_3D_NBDV
  Int            getNumDdvCandPics()                      { return m_iNumDdvCandPics;    }
  Int            getDisCandRefPictures(Int iColPOC);        
  Void           setRapRefIdx(UInt uiRapRefIdx)           { m_uiRapRefIdx = uiRapRefIdx; }
  Void           setRapRefList(RefPicList eRefPicList)    { m_eRapRefList = eRefPicList; }
  Void           setNumDdvCandPics (Int i)                { m_iNumDdvCandPics = i;       }
  UInt           getRapRefIdx()                           { return m_uiRapRefIdx;        }
  RefPicList     getRapRefList()                          { return m_eRapRefList;        }
  Void           checkTemporalIVRef();                     
  Bool           isTempIVRefValid(Int currCandPic, Int iTempRefDir, Int iTempRefIdx);
  Void           checkTextureRef(  );
  Int            isTextRefValid(Int iTextRefDir, Int iTextRefIdx);
#endif
#endif
#endif

  /** transfer ownership of seis to this picture */
  Void setSEIs(SEIMessages& seis) { m_SEIs = seis; }

  /**
   * return the current list of SEI messages associated with this picture.
   * Pointer is valid until this->destroy() is called */
  SEIMessages& getSEIs() { return m_SEIs; }

  /**
   * return the current list of SEI messages associated with this picture.
   * Pointer is valid until this->destroy() is called */
  const SEIMessages& getSEIs() const { return m_SEIs; }
};// END CLASS DEFINITION TComPic

#if NH_MV

class TComAu : public TComList<TComPic*>
{
  
public:

  Int                 getPoc            ( )                     {  assert(!empty()); return back()->getPOC            ();  }
  Void                setPicLatencyCount( Int picLatenyCount );
  Int                 getPicLatencyCount( )                     {  assert(!empty()); return back()->getPicLatencyCount();  }  
  TComPic*            getPic            ( Int nuhLayerId  );
  Void                addPic            ( TComPic* pic, Bool pocUnkown );
  Bool                containsPic       ( TComPic* pic );  
};


class TComSubDpb : public TComList<TComPic*>
{
private: 
  Int m_nuhLayerId; 
public:  
  TComSubDpb( Int nuhLayerid );

  Int                 getLayerId                      ( ) { return m_nuhLayerId; }

  TComPic*            getPic                          ( Int poc  );
  TComPic*            getPicFromLsb                   ( Int pocLsb, Int maxPicOrderCntLsb );
  TComPic*            getShortTermRefPic              ( Int poc  );
  TComList<TComPic*>  getPicsMarkedNeedForOutput      ( );

  Void                markAllAsUnusedForReference     ( );

  Void                addPic                          ( TComPic* pic );
  Void                removePics                      ( std::vector<TComPic*> picToRemove );
  Bool                areAllPicsMarkedNotNeedForOutput( );
};

class TComPicLists 
{
private: 
  TComList<TComAu*    >       m_aus;  
  TComList<TComSubDpb*>       m_subDpbs; 
  Bool                        m_printPicOutput; 
#if NH_3D                     
  const TComVPS*              m_vps; 
#endif
public: 
  TComPicLists() { m_printPicOutput = false; };
  ~TComPicLists();

  // Add and remove single pictures
  Void                   addNewPic( TComPic* pic );
  Void                   removePic( TComPic* pic );

  // Get Pics
  TComPic*               getPic                         ( Int layerIdInNuh, Int poc );
  TComPicYuv*            getPicYuv                      ( Int layerIdInNuh, Int poc, Bool recon );

  // Get and AUs and SubDPBs
  TComSubDpb*            getSubDpb                      ( Int nuhLayerId, Bool create );
  TComList<TComSubDpb*>* getSubDpbs                     ( );
                                                        
  TComAu*                addAu                          ( Int poc  );
  TComAu*                getAu                          ( Int poc, Bool create );
  TComList<TComAu*>*     getAus                         ( );
  TComList<TComAu*>      getAusHavingPicsMarkedForOutput( );

  // Mark pictures and set POCs
  Void                   markSubDpbAsUnusedForReference ( Int layerIdInNuh );
  Void                   markSubDpbAsUnusedForReference ( TComSubDpb& subDpb );
  Void                   markAllSubDpbAsUnusedForReference(  );
  Void                   decrementPocsInSubDpb          ( Int nuhLayerId, Int deltaPocVal );
  
  // Empty Sub DPBs
  Void                   emptyAllSubDpbs                ( );
  Void                   emptySubDpbs                   ( TComList<TComSubDpb*>* subDpbs);
  Void                   emptySubDpb                    ( TComSubDpb* subDpb);
  Void                   emptySubDpb                    ( Int nuhLayerId );

  Void                   emptyNotNeedForOutputAndUnusedForRef      ( );
  Void                   emptySubDpbNotNeedForOutputAndUnusedForRef( Int layerId  );
  Void                   emptySubDpbNotNeedForOutputAndUnusedForRef( TComSubDpb subDpb );  

  // For printing to std::out 
  Void                   setPrintPicOutput ( Bool printPicOutput ) { m_printPicOutput = printPicOutput; };
  Void                   print(); 

#if NH_3D                                   
  Void                   setVPS                        ( const TComVPS* vps ) { m_vps = vps;  }; 
  TComPic*               getPic                        ( Int viewIndex, Bool depthFlag, Int poc );
  TComPicYuv*            getPicYuv                     ( Int viewIndex, Bool depthFlag, Int poc, Bool recon );
#endif  

}; 

// END CLASS DEFINITION TComPicLists

#endif


//! \}

#endif // __TCOMPIC__
