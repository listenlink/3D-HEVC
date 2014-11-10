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

/** \file     TEncTop.h
    \brief    encoder class (header)
*/

#ifndef __TENCTOP__
#define __TENCTOP__

// Include files
#include "TLibCommon/TComList.h"
#include "TLibCommon/TComPrediction.h"
#include "TLibCommon/TComTrQuant.h"
#include "TLibCommon/AccessUnit.h"

#include "TLibVideoIO/TVideoIOYuv.h"

#include "TEncCfg.h"
#include "TEncGOP.h"
#include "TEncSlice.h"
#include "TEncEntropy.h"
#include "TEncCavlc.h"
#include "TEncSbac.h"
#include "TEncSearch.h"
#include "TEncSampleAdaptiveOffset.h"
#include "TEncPreanalyzer.h"
#include "TEncRateCtrl.h"
//! \ingroup TLibEncoder
//! \{

// ====================================================================================================================
// Class definition
// ====================================================================================================================

#if KWU_RC_MADPRED_E0227
class TAppEncTop;
#endif
/// encoder class
class TEncTop : public TEncCfg
{
private:
  // picture
  Int                     m_iPOCLast;                     ///< time index (POC)
  Int                     m_iNumPicRcvd;                  ///< number of received pictures
  UInt                    m_uiNumAllPicCoded;             ///< number of coded pictures
  TComList<TComPic*>      m_cListPic;                     ///< dynamic list of pictures
  
#if H_MV
  TComPicLists*           m_ivPicLists;                   ///< access to picture lists of other layers 
#endif
#if H_3D_IC
  Int *m_aICEnableCandidate;
  Int *m_aICEnableNum;
#endif
  // encoder search
  TEncSearch              m_cSearch;                      ///< encoder search class
  //TEncEntropy*            m_pcEntropyCoder;                     ///< entropy encoder
  TEncCavlc*              m_pcCavlcCoder;                       ///< CAVLC encoder  
  // coding tool
  TComTrQuant             m_cTrQuant;                     ///< transform & quantization class
  TComLoopFilter          m_cLoopFilter;                  ///< deblocking filter class
  TEncSampleAdaptiveOffset m_cEncSAO;                     ///< sample adaptive offset class
  TEncEntropy             m_cEntropyCoder;                ///< entropy encoder
  TEncCavlc               m_cCavlcCoder;                  ///< CAVLC encoder
  TEncSbac                m_cSbacCoder;                   ///< SBAC encoder
  TEncBinCABAC            m_cBinCoderCABAC;               ///< bin coder CABAC
  TEncSbac*               m_pcSbacCoders;                 ///< SBAC encoders (to encode substreams )
  TEncBinCABAC*           m_pcBinCoderCABACs;             ///< bin coders CABAC (one per substream)
  
  // processing unit
  TEncGOP                 m_cGOPEncoder;                  ///< GOP encoder
  TEncSlice               m_cSliceEncoder;                ///< slice encoder
  TEncCu                  m_cCuEncoder;                   ///< CU encoder
  // SPS
  TComSPS                 m_cSPS;                         ///< SPS
  TComPPS                 m_cPPS;                         ///< PPS
  // RD cost computation
  TComBitCounter          m_cBitCounter;                  ///< bit counter for RD optimization
  TComRdCost              m_cRdCost;                      ///< RD cost computation class
  TEncSbac***             m_pppcRDSbacCoder;              ///< temporal storage for RD computation
  TEncSbac                m_cRDGoOnSbacCoder;             ///< going on SBAC model for RD stage
#if FAST_BIT_EST
  TEncBinCABACCounter***  m_pppcBinCoderCABAC;            ///< temporal CABAC state storage for RD computation
  TEncBinCABACCounter     m_cRDGoOnBinCoderCABAC;         ///< going on bin coder CABAC for RD stage
#else
  TEncBinCABAC***         m_pppcBinCoderCABAC;            ///< temporal CABAC state storage for RD computation
  TEncBinCABAC            m_cRDGoOnBinCoderCABAC;         ///< going on bin coder CABAC for RD stage
#endif
  Int                     m_iNumSubstreams;                ///< # of top-level elements allocated.
  TComBitCounter*         m_pcBitCounters;                 ///< bit counters for RD optimization per substream
  TComRdCost*             m_pcRdCosts;                     ///< RD cost computation class per substream
  TEncSbac****            m_ppppcRDSbacCoders;             ///< temporal storage for RD computation per substream
  TEncSbac*               m_pcRDGoOnSbacCoders;            ///< going on SBAC model for RD stage per substream
  TEncBinCABAC****        m_ppppcBinCodersCABAC;           ///< temporal CABAC state storage for RD computation per substream
  TEncBinCABAC*           m_pcRDGoOnBinCodersCABAC;        ///< going on bin coder CABAC for RD stage per substream

  // quality control
  TEncPreanalyzer         m_cPreanalyzer;                 ///< image characteristics analyzer for TM5-step3-like adaptive QP

  TComScalingList         m_scalingList;                 ///< quantization matrix information
  TEncRateCtrl            m_cRateCtrl;                    ///< Rate control class

#if KWU_RC_MADPRED_E0227
  TAppEncTop*             m_pcTAppEncTop;
  TAppComCamPara*         m_cCamParam;
#endif
  
#if H_MV
  TEncAnalyze             m_cAnalyzeAll;
  TEncAnalyze             m_cAnalyzeI;
  TEncAnalyze             m_cAnalyzeP;
  TEncAnalyze             m_cAnalyzeB;  
#endif
protected:
  Void  xGetNewPicBuffer  ( TComPic*& rpcPic );           ///< get picture buffer which will be processed
  Void  xInitSPS          ();                             ///< initialize SPS from encoder options
  Void  xInitPPS          ();                             ///< initialize PPS from encoder options
  
  Void  xInitPPSforTiles  ();
  Void  xInitRPS          (Bool isFieldCoding);           ///< initialize PPS from encoder options
#if H_MV

  template <class T>     Void  xDelete( Bool ar, T inArray ) 
  {
    if( inArray != NULL)
    {
      if ( ar )
      {      
        delete[] inArray; 
      }
      else
      {
        delete inArray;
      }
    }
  }

  template <class T>     Void  xDelete( Bool ar, T inArray, Int idx1 ) 
  {
    if( inArray != NULL)
    {
        xDelete( ar, inArray[idx1]); 
    }
  }

  template <class T>     Void  xDelete( Bool ar, T inArray, Int idx1, Int idx2 ) 
  {
    if( inArray != NULL )
    {
      xDelete( ar, inArray[idx1], idx2 ); 
    }
  }

  template <class T>     Void  xDelete( Bool ar, T inArray, Int idx1, Int idx2, Int idx3 ) 
  {
    if( inArray != NULL )
    {
      xDelete( ar, inArray[idx1], idx2, idx3 ); 
    }
  }

#endif
public:
  TEncTop();
  virtual ~TEncTop();
  
  Void      create          ();
  Void      destroy         ();
#if KWU_RC_MADPRED_E0227
  Void      init            ( TAppEncTop* pcTAppEncTop, Bool isFieldCoding );
#else
  Void      init            (Bool isFieldCoding);
#endif

#if H_MV  
  TComPicLists* getIvPicLists() { return m_ivPicLists; }
#endif
#if H_3D_IC
  Int*      getICEnableCandidate() { return m_aICEnableCandidate; }
  Int*      getICEnableNum() { return m_aICEnableNum; }
#endif
  Void      deletePicBuffer ();

  Void      createWPPCoders(Int iNumSubstreams);
  
#if H_MV
  Void      initNewPic(TComPicYuv* pcPicYuvOrg);
#endif
  // -------------------------------------------------------------------------------------------------------------------
  // member access functions
  // -------------------------------------------------------------------------------------------------------------------
  
  TComList<TComPic*>*     getListPic            () { return  &m_cListPic;             }
  TEncSearch*             getPredSearch         () { return  &m_cSearch;              }
  
  TComTrQuant*            getTrQuant            () { return  &m_cTrQuant;             }
  TComLoopFilter*         getLoopFilter         () { return  &m_cLoopFilter;          }
  TEncSampleAdaptiveOffset* getSAO              () { return  &m_cEncSAO;              }
  TEncGOP*                getGOPEncoder         () { return  &m_cGOPEncoder;          }
  TEncSlice*              getSliceEncoder       () { return  &m_cSliceEncoder;        }
  TEncCu*                 getCuEncoder          () { return  &m_cCuEncoder;           }
  TEncEntropy*            getEntropyCoder       () { return  &m_cEntropyCoder;        }
  TEncCavlc*              getCavlcCoder         () { return  &m_cCavlcCoder;          }
  TEncSbac*               getSbacCoder          () { return  &m_cSbacCoder;           }
  TEncBinCABAC*           getBinCABAC           () { return  &m_cBinCoderCABAC;       }
  TEncSbac*               getSbacCoders     () { return  m_pcSbacCoders;      }
  TEncBinCABAC*           getBinCABACs          () { return  m_pcBinCoderCABACs;      }
  
  TComBitCounter*         getBitCounter         () { return  &m_cBitCounter;          }
  TComRdCost*             getRdCost             () { return  &m_cRdCost;              }
  TEncSbac***             getRDSbacCoder        () { return  m_pppcRDSbacCoder;       }
  TEncSbac*               getRDGoOnSbacCoder    () { return  &m_cRDGoOnSbacCoder;     }
  TComBitCounter*         getBitCounters        () { return  m_pcBitCounters;         }
  TComRdCost*             getRdCosts            () { return  m_pcRdCosts;             }
  TEncSbac****            getRDSbacCoders       () { return  m_ppppcRDSbacCoders;     }
  TEncSbac*               getRDGoOnSbacCoders   () { return  m_pcRDGoOnSbacCoders;   }
  TEncRateCtrl*           getRateCtrl           () { return &m_cRateCtrl;             }
#if KWU_RC_MADPRED_E0227
  TAppEncTop*             getEncTop             () { return m_pcTAppEncTop; }
  TAppComCamPara*         getCamParam()                 { return m_cCamParam;}
  Void                    setCamParam(TAppComCamPara * pCamparam)                 { m_cCamParam = pCamparam;}
#endif
  TComSPS*                getSPS                () { return  &m_cSPS;                 }
  TComPPS*                getPPS                () { return  &m_cPPS;                 }
  Void selectReferencePictureSet(TComSlice* slice, Int POCCurr, Int GOPid );
  Int getReferencePictureSetIdxForSOP(TComSlice* slice, Int POCCurr, Int GOPid );
  TComScalingList*        getScalingList        () { return  &m_scalingList;         }
#if H_MV
  TEncAnalyze*            getAnalyzeAll         () { return &m_cAnalyzeAll; }
  TEncAnalyze*            getAnalyzeI           () { return &m_cAnalyzeI;   }
  TEncAnalyze*            getAnalyzeP           () { return &m_cAnalyzeP;   }
  TEncAnalyze*            getAnalyzeB           () { return &m_cAnalyzeB;   }

  Int                     getNumAllPicCoded     () { return m_uiNumAllPicCoded; }
 
  Int                     getFrameId            (Int iGOPid);
  
  TComPic*                getPic                ( Int poc );
  Void                    setIvPicLists         ( TComPicLists* picLists) { m_ivPicLists = picLists; }
#endif
#if HHI_TOOL_PARAMETERS_I2_J0107
#if H_3D
  Void                    setSps3dExtension     ( TComSps3dExtension sps3dExtension ) { m_cSPS.setSps3dExtension( sps3dExtension );  };
#endif
#endif
#if H_3D_IC
  Void                    setICEnableCandidate         ( Int* ICEnableCandidate) { m_aICEnableCandidate = ICEnableCandidate; }
  Void                    setICEnableNum         ( Int* ICEnableNum) { m_aICEnableNum = ICEnableNum; }
#endif
  // -------------------------------------------------------------------------------------------------------------------
  // encoder function
  // -------------------------------------------------------------------------------------------------------------------

  /// encode several number of pictures until end-of-sequence
#if H_MV
  Void encode( Bool bEos, TComPicYuv* pcPicYuvOrg, TComList<TComPicYuv*>& rcListPicYuvRecOut, std::list<AccessUnit>& accessUnitsOut, Int& iNumEncoded  , Int gopId  );  

   /// encode several number of pictures until end-of-sequence
  Void encode( bool bEos, TComPicYuv* pcPicYuvOrg, TComList<TComPicYuv*>& rcListPicYuvRecOut,
              std::list<AccessUnit>& accessUnitsOut, Int& iNumEncoded, bool isTff  , Int gopId );

#else
  Void encode( Bool bEos, TComPicYuv* pcPicYuvOrg, TComList<TComPicYuv*>& rcListPicYuvRecOut,
              std::list<AccessUnit>& accessUnitsOut, Int& iNumEncoded ); 
   /// encode several number of pictures until end-of-sequence
  Void encode( bool bEos, TComPicYuv* pcPicYuvOrg, TComList<TComPicYuv*>& rcListPicYuvRecOut,
              std::list<AccessUnit>& accessUnitsOut, Int& iNumEncoded, bool isTff);

#endif

#if H_MV
  Void printSummary      ( Int numAllPicCoded, bool isField ); 
#else
  Void printSummary(bool isField) { m_cGOPEncoder.printOutSummary (m_uiNumAllPicCoded, isField); }
#endif

#if H_3D
   Void setupRenModel( Int iPoc, Int iEncViewIdx, Int iEncContent, Int iHorOffset );
#endif
};

//! \}

#endif // __TENCTOP__
