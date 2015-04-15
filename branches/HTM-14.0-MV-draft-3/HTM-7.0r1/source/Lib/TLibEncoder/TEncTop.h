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
#include "TLibCommon/TComDepthMapGenerator.h"
#include "../TLibCommon/TComResidualGenerator.h"

#include "TLibVideoIO/TVideoIOYuv.h"

#include "TEncCfg.h"
#include "TEncGOP.h"
#include "TEncSlice.h"
#include "TEncEntropy.h"
#include "TEncCavlc.h"
#include "TEncSbac.h"
#include "TEncSearch.h"
#include "TEncAdaptiveLoopFilter.h"
#include "TEncSampleAdaptiveOffset.h"
#include "TEncPreanalyzer.h"

//! \ingroup TLibEncoder
//! \{

// ====================================================================================================================
// Class definition
// ====================================================================================================================

class TAppEncTop;

/// encoder class
class TEncTop : public TEncCfg
{
private:
  // picture
  Int                     m_iPOCLast;                     ///< time index (POC)
  Int                     m_iNumPicRcvd;                  ///< number of received pictures
  UInt                    m_uiNumAllPicCoded;             ///< number of coded pictures
  TComList<TComPic*>      m_cListPic;                     ///< dynamic list of pictures
  
  // encoder search
  TEncSearch              m_cSearch;                      ///< encoder search class
  TEncEntropy*            m_pcEntropyCoder;                     ///< entropy encoder 
  TEncCavlc*              m_pcCavlcCoder;                       ///< CAVLC encoder  
  // coding tool
  TComTrQuant             m_cTrQuant;                     ///< transform & quantization class
  TComLoopFilter          m_cLoopFilter;                  ///< deblocking filter class
  TEncSampleAdaptiveOffset m_cEncSAO;                     ///< sample adaptive offset class
  TEncAdaptiveLoopFilter  m_cAdaptiveLoopFilter;          ///< adaptive loop filter class
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
 
#if DEPTH_MAP_GENERATION
  TComDepthMapGenerator   m_cDepthMapGenerator;           ///< depth map generator
#endif
#if H3D_IVRP & !QC_ARP_D0177
  TComResidualGenerator   m_cResidualGenerator;           ///< generator for residual pictures
#endif
  // SPS
  TComSPS                 m_cSPS;                         ///< SPS
  TComPPS                 m_cPPS;                         ///< PPS
  std::vector<TComAPS>    m_vAPS;  //!< APS container
  TComRPSList             m_RPSList;                         ///< RPS
  
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

  std::vector<TEncTop*>*  m_pacTEncTopList;
  TAppEncTop*             m_pcTAppEncTop;

  TEncAnalyze             m_cAnalyzeAll;
  TEncAnalyze             m_cAnalyzeI;
  TEncAnalyze             m_cAnalyzeP;
  TEncAnalyze             m_cAnalyzeB;

protected:
  Void  xGetNewPicBuffer  ( TComPic*& rpcPic );           ///< get picture buffer which will be processed
  Void  xInitSPS          ();                             ///< initialize SPS from encoder options
  Void  xInitPPS          ();                             ///< initialize PPS from encoder options
  
  Void  xInitPPSforTiles  ();
  Void  xInitRPS          ();                             ///< initialize PPS from encoder options
  Void  xInitSPSforInterViewRefs();

public:
  TEncTop();
  virtual ~TEncTop();
  
  Void      create          ();
  Void      destroy         ();
  Void      init            ( TAppEncTop* pcTAppEncTop );
  Void      deletePicBuffer ();

  UInt      getFrameId          (Int iGOPid)  {
    if(m_iPOCLast == 0)
    {
      return(0 );
    }
    else
    {
      return m_iPOCLast -m_iNumPicRcvd+ getGOPEntry(iGOPid).m_POC ;
    }
  }
#if HHI_INTERVIEW_SKIP || H3D_IVMP || H3D_IVRP
  Void      deleteExtraPicBuffers   ( Int iPoc );
#endif

  Void      compressMotion          ( Int iPoc );

  Void      initNewPic(TComPicYuv* pcPicYuvOrg, TComPicYuv* pcOrgPdmDepth = 0);

  Void      createWPPCoders(Int iNumSubstreams);
  
  // -------------------------------------------------------------------------------------------------------------------
  // member access functions
  // -------------------------------------------------------------------------------------------------------------------
  
  TComList<TComPic*>*     getListPic            () { return  &m_cListPic;             }
  TEncSearch*             getPredSearch         () { return  &m_cSearch;              }
  
  TComTrQuant*            getTrQuant            () { return  &m_cTrQuant;             }
  TComLoopFilter*         getLoopFilter         () { return  &m_cLoopFilter;          }
  TEncAdaptiveLoopFilter* getAdaptiveLoopFilter () { return  &m_cAdaptiveLoopFilter;  }
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
  
  Void                    setTEncTopList        ( std::vector<TEncTop*>* pacTEncTopList );
  TAppEncTop*             getEncTop             () { return m_pcTAppEncTop; }
  Int                     getNumAllPicCoded     () { return m_uiNumAllPicCoded; }
  Void                    printOutSummary       ( UInt uiNumAllPicCoded );

  TComSPS*                getSPS                () { return  &m_cSPS;                 }
  TComPPS*                getPPS                () { return  &m_cPPS;                 }
  std::vector<TComAPS>&   getAPS                () { return m_vAPS; }
  TComRPSList*            getRPSList            () { return  &m_RPSList;                 }
  
  Void selectReferencePictureSet(TComSlice* slice, Int POCCurr, Int GOPid,TComList<TComPic*>& listPic );
  TComScalingList*        getScalingList        () { return  &m_scalingList;         }
  
  TEncAnalyze*            getAnalyzeAll         () { return &m_cAnalyzeAll; }
  TEncAnalyze*            getAnalyzeI           () { return &m_cAnalyzeI;   }
  TEncAnalyze*            getAnalyzeP           () { return &m_cAnalyzeP;   }
  TEncAnalyze*            getAnalyzeB           () { return &m_cAnalyzeB;   }

#if DEPTH_MAP_GENERATION
  TComDepthMapGenerator*  getDepthMapGenerator  () { return  &m_cDepthMapGenerator;   }
#endif
#if H3D_IVRP & !QC_ARP_D0177
  TComResidualGenerator*  getResidualGenerator  () { return  &m_cResidualGenerator;   }
#endif

  // -------------------------------------------------------------------------------------------------------------------
  // encoder function
  // -------------------------------------------------------------------------------------------------------------------

  /// encode several number of pictures until end-of-sequence
  Void encode( bool bEos, TComPicYuv* pcPicYuvOrg, TComList<TComPicYuv*>& rcListPicYuvRecOut,
              std::list<AccessUnit>& accessUnitsOut, Int& iNumEncoded, Int gopId );
};

//! \}

#endif // __TENCTOP__
