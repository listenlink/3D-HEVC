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


/** \file     TEncTop.h
    \brief    encoder class (header)
*/

#ifndef __TENCTOP__
#define __TENCTOP__

// Include files
#include "../TLibCommon/TComList.h"
#include "../TLibCommon/TComPrediction.h"
#include "../TLibCommon/TComTrQuant.h"
#include "../TLibCommon/TComBitStream.h"
#include "../TLibCommon/TComDepthMapGenerator.h"
#include "../TLibCommon/TComResidualGenerator.h"

#include "../TLibVideoIO/TVideoIOYuv.h"
#include "../TLibVideoIO/TVideoIOBits.h"

#include "TEncCfg.h"
#include "TEncGOP.h"
#include "TEncSlice.h"
#include "TEncEntropy.h"
#include "TEncCavlc.h"
#include "TEncSbac.h"
#include "TEncSearch.h"
#include "TEncAdaptiveLoopFilter.h"

#include "TEncSeqStructure.h"
#include <map>
#include "TEncAnalyze.h"

#if POZNAN_MP
#include "../TLibCommon/TComMP.h"
#endif		

// ====================================================================================================================
// Class definition
// ====================================================================================================================

class TAppEncTop ;

/// encoder class
class TEncTop : public TEncCfg
{
private:
  // picture
  Int                     m_iPOCLast;                     ///< time index (POC)
  Int                     m_iNumPicRcvd;                  ///< number of received pictures
  UInt                    m_uiNumAllPicCoded;             ///< number of coded pictures
  TComList<TComPic*>      m_cListPic;                     ///< dynamic list of pictures

  Bool                    m_bSeqFirst ;
  TEncSeqStructure::Iterator m_cSeqIter;
  std::map<Int, TComPic*> m_acInputPicMap;
  std::map<PicOrderCnt, TComPicYuv*> m_acOutputPicMap;

  // encoder search
  TEncSearch              m_cSearch;                      ///< encoder search class
  TEncEntropy*            m_pcEntropyCoder;                     ///< entropy encoder
  TEncCavlc*              m_pcCavlcCoder;                       ///< CAVLC encoder
  // coding tool
  TComTrQuant             m_cTrQuant;                     ///< transform & quantization class
  TComLoopFilter          m_cLoopFilter;                  ///< deblocking filter class
#if MTK_SAO
  TEncSampleAdaptiveOffset  m_cEncSAO;                    ///< sample adaptive offset class
#endif
  TEncAdaptiveLoopFilter  m_cAdaptiveLoopFilter;          ///< adaptive loop filter class
  TEncEntropy             m_cEntropyCoder;                ///< entropy encoder
  TEncCavlc               m_cCavlcCoder;                  ///< CAVLC encoder
  TEncSbac                m_cSbacCoder;                   ///< SBAC encoder
  TEncBinCABAC            m_cBinCoderCABAC;               ///< bin coder CABAC

  // processing unit
  TEncPic                 m_cPicEncoder;                  ///< Pic encoder
  TEncSlice               m_cSliceEncoder;                ///< slice encoder
  TEncCu                  m_cCuEncoder;                   ///< CU encoder
#if DEPTH_MAP_GENERATION
  TComDepthMapGenerator   m_cDepthMapGenerator;           ///< depth map generator
#endif
#if HHI_INTER_VIEW_RESIDUAL_PRED
  TComResidualGenerator   m_cResidualGenerator;           ///< generator for residual pictures
#endif

  // SPS
  TComSPS                 m_cSPS;                         ///< SPS
  TComPPS                 m_cPPS;                         ///< PPS

  // RD cost computation
  TComBitCounter          m_cBitCounter;                  ///< bit counter for RD optimization
  TComRdCost              m_cRdCost;                      ///< RD cost computation class
  TEncSbac***             m_pppcRDSbacCoder;              ///< temporal storage for RD computation
  TEncSbac                m_cRDGoOnSbacCoder;             ///< going on SBAC model for RD stage
  TEncBinCABAC***         m_pppcBinCoderCABAC;            ///< temporal CABAC state storage for RD computation
  TEncBinCABAC            m_cRDGoOnBinCoderCABAC;         ///< going on bin coder CABAC for RD stage

  std::vector<TEncTop*>*  m_pacTEncTopList;
  TAppEncTop*             m_pcTAppEncTop;                 // SB better: design a new MVTop encoder class, instead of mixing lib and app

  bool                    m_bPicWaitingForCoding;

  PicOrderCnt             m_iFrameNumInCodingOrder;
#if POZNAN_TEXTURE_TU_DELTA_QP_PARAM_IN_CFG_FOR_ENC 
  Double                  m_dTextureCuDeltaQpOffset;
  Double                  m_dTextureCuDeltaQpMul;
  Int                     m_iTextureCuDeltaQpTopBottomRow; 
#endif

#if POZNAN_MP
  TComMP*				  m_pcMP;
#endif

protected:
  Void  xGetNewPicBuffer  ( TComPic*& rpcPic );           ///< get picture buffer which will be processed
  Void  xInitSPS          ();                             ///< initialize SPS from encoder options
#if CONSTRAINED_INTRA_PRED
  Void  xInitPPS          ();                             ///< initialize PPS from encoder options
#endif
  Void  xSetPicProperties( TComPic* pcPic  ) ;
  Void  xSetRefPics( TComPic* pcPic, RefPicList eRefPicList );
  Void  xCheckSliceType(TComPic* pcPic);

public:
  TEncTop();
  virtual ~TEncTop();

  Void      create          ();
  Void      destroy         ();
  Void      init            ( TAppEncTop* pcTAppEncTop );
  Void      deletePicBuffer ();

  Void      deleteExtraPicBuffers   ( Int iPoc );
#if AMVP_BUFFERCOMPRESS
  Void      compressMotion          ( Int iPoc );
#endif

  UInt      getNextFrameId          ()  { return (UInt)m_cSeqIter.getPoc(); }
  Bool      currentPocWillBeCoded   ()  { return ( m_acInputPicMap.find( (Int)m_cSeqIter.getPoc() ) != m_acInputPicMap.end() ); }

  TComList<TComPic*>      getCodedPictureStore(){ return m_cListPic;}

  // -------------------------------------------------------------------------------------------------------------------
  // member access functions
  // -------------------------------------------------------------------------------------------------------------------

  TComList<TComPic*>*     getListPic            () { return  &m_cListPic;             }
  TEncSearch*             getPredSearch         () { return  &m_cSearch;              }

  TComTrQuant*            getTrQuant            () { return  &m_cTrQuant;             }
  TComLoopFilter*         getLoopFilter         () { return  &m_cLoopFilter;          }
  TEncAdaptiveLoopFilter* getAdaptiveLoopFilter () { return  &m_cAdaptiveLoopFilter;  }
#if MTK_SAO
  TEncSampleAdaptiveOffset* getSAO                () { return  &m_cEncSAO;              }
#endif
  TEncPic*                getPicEncoder         () { return  &m_cPicEncoder;          }
  TEncSlice*              getSliceEncoder       () { return  &m_cSliceEncoder;        }
  TEncCu*                 getCuEncoder          () { return  &m_cCuEncoder;           }
  TEncEntropy*            getEntropyCoder       () { return  &m_cEntropyCoder;        }
  TEncCavlc*              getCavlcCoder         () { return  &m_cCavlcCoder;          }
  TEncSbac*               getSbacCoder          () { return  &m_cSbacCoder;           }
  TEncBinCABAC*           getBinCABAC           () { return  &m_cBinCoderCABAC;       }

  TComBitCounter*         getBitCounter         () { return  &m_cBitCounter;          }
  TComRdCost*             getRdCost             () { return  &m_cRdCost;              }
  TEncSbac***             getRDSbacCoder        () { return  m_pppcRDSbacCoder;       }
  TEncSbac*               getRDGoOnSbacCoder    () { return  &m_cRDGoOnSbacCoder;     }
#if DEPTH_MAP_GENERATION
  TComDepthMapGenerator*  getDepthMapGenerator  () { return  &m_cDepthMapGenerator;   }
#endif
#if HHI_INTER_VIEW_RESIDUAL_PRED
  TComResidualGenerator*  getResidualGenerator  () { return  &m_cResidualGenerator;   }
#endif

  TComSPS*                getSPS                () { return  &m_cSPS;                 }
  TComPPS*                getPPS                () { return  &m_cPPS;                 }

  Void                    setTEncTopList        ( std::vector<TEncTop*>* pacTEncTopList );
  TAppEncTop*             getEncTop             () { return m_pcTAppEncTop; }

  Int                     getNumAllPicCoded     () { return m_uiNumAllPicCoded; }

  Void                    printOutSummary       ( UInt uiNumAllPicCoded );

  TEncAnalyze             m_cAnalyzeAll;
  TEncAnalyze             m_cAnalyzeI;
  TEncAnalyze             m_cAnalyzeP;
  TEncAnalyze             m_cAnalyzeB;
  // -------------------------------------------------------------------------------------------------------------------
  // encoder function
  // -------------------------------------------------------------------------------------------------------------------

  /// encode several number of pictures until end-of-sequence


//GT PRE LOAD ENC BUFFER
  Void encode    ( bool bEos, std::map<PicOrderCnt, TComPicYuv*>& rcMapPicYuvRecOut, TComBitstream* pcBitstreamOut, Bool& bNewPicNeeded );
  Void receivePic( bool bEos, TComPicYuv* pcPicYuvOrg, TComPicYuv* pcPicYuvRec, TComPicYuv* pcOrgPdmDepth = 0 );

#if POZNAN_TEXTURE_TU_DELTA_QP_PARAM_IN_CFG_FOR_ENC 
  Double getTextureCuDeltaQpOffset( )      { return m_dTextureCuDeltaQpOffset;}
  Double getTextureCuDeltaQpMul( )         { return m_dTextureCuDeltaQpMul;}
  Int    getTextureCuDeltaQpTopBottomRow( ){ return m_iTextureCuDeltaQpTopBottomRow;}
  Void   setTextureCuDeltaQpOffset      ( Double dTextureCuDeltaQpOffset    ){ m_dTextureCuDeltaQpOffset       = dTextureCuDeltaQpOffset; }
  Void   setTextureCuDeltaQpMul         ( Double dTextureCuDeltaQpMul       ){ m_dTextureCuDeltaQpMul          = dTextureCuDeltaQpMul; }
  Void   setTextureCuDeltaQpTopBottomRow( Int iTextureCuDeltaQpTopBottomRow ){ m_iTextureCuDeltaQpTopBottomRow = iTextureCuDeltaQpTopBottomRow; }  
#endif
};

#endif // __TENCTOP__

