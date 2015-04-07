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

/** \file     TComDataCU.h
    \brief    CU data structure (header)
    \todo     not all entities are documented
*/

#ifndef _TCOMDATACU_
#define _TCOMDATACU_

#include <assert.h>

// Include files
#include "CommonDef.h"
#include "TComMotionInfo.h"
#include "TComSlice.h"
#include "TComRdCost.h"
#include "TComPattern.h"

#if H_3D_ARP
#include "TComYuv.h"
#endif

#include <algorithm>
#include <vector>

//! \ingroup TLibCommon
//! \{


#if H_3D_DBBP
typedef struct _DBBPTmpData
{
  TComMv      acMvd[2][2];          // for two segments and two lists
  TComMvField acMvField[2][2];      // for two segments and two lists
  Int         aiMvpNum[2][2];       // for two segments and two lists
  Int         aiMvpIdx[2][2];       // for two segments and two lists
  UChar       auhInterDir[2];       // for two segments
  Bool        abMergeFlag[2];       // for two segments
  UChar       auhMergeIndex[2];     // for two segments
  PartSize    eVirtualPartSize;
  UInt        uiVirtualPartIndex;
} DBBPTmpData;
#endif

// ====================================================================================================================
// Class definition
// ====================================================================================================================

/// CU data structure class
class TComDataCU
{
private:
  
  // -------------------------------------------------------------------------------------------------------------------
  // class pointers
  // -------------------------------------------------------------------------------------------------------------------
  
  TComPic*      m_pcPic;              ///< picture class pointer
  TComSlice*    m_pcSlice;            ///< slice header pointer
  TComPattern*  m_pcPattern;          ///< neighbour access class pointer
  
  // -------------------------------------------------------------------------------------------------------------------
  // CU description
  // -------------------------------------------------------------------------------------------------------------------
  
  UInt          m_uiCUAddr;           ///< CU address in a slice
  UInt          m_uiAbsIdxInLCU;      ///< absolute address in a CU. It's Z scan order
  UInt          m_uiCUPelX;           ///< CU position in a pixel (X)
  UInt          m_uiCUPelY;           ///< CU position in a pixel (Y)
  UInt          m_uiNumPartition;     ///< total number of minimum partitions in a CU
  UChar*        m_puhWidth;           ///< array of widths
  UChar*        m_puhHeight;          ///< array of heights
  UChar*        m_puhDepth;           ///< array of depths
  Int           m_unitSize;           ///< size of a "minimum partition"
  
  // -------------------------------------------------------------------------------------------------------------------
  // CU data
  // -------------------------------------------------------------------------------------------------------------------
  Bool*         m_skipFlag;           ///< array of skip flags
#if SEC_DEPTH_INTRA_SKIP_MODE_K0033
  Bool*         m_bDISFlag;         
  UInt*         m_uiDISType;
#else
#if H_3D_SINGLE_DEPTH
  Bool*         m_singleDepthFlag;           ///< array of single depth flags
  Pel*          m_apSingleDepthValue;
#endif
#endif
  Char*         m_pePartSize;         ///< array of partition sizes
  Char*         m_pePredMode;         ///< array of prediction modes
  Bool*         m_CUTransquantBypass;   ///< array of cu_transquant_bypass flags
  Char*         m_phQP;               ///< array of QP values
  UChar*        m_puhTrIdx;           ///< array of transform indices
  UChar*        m_puhTransformSkip[3];///< array of transform skipping flags
  UChar*        m_puhCbf[3];          ///< array of coded block flags (CBF)
  TComCUMvField m_acCUMvField[2];     ///< array of motion vectors
  TCoeff*       m_pcTrCoeffY;         ///< transformed coefficient buffer (Y)
  TCoeff*       m_pcTrCoeffCb;        ///< transformed coefficient buffer (Cb)
  TCoeff*       m_pcTrCoeffCr;        ///< transformed coefficient buffer (Cr)
#if ADAPTIVE_QP_SELECTION
  Int*          m_pcArlCoeffY;        ///< ARL coefficient buffer (Y)
  Int*          m_pcArlCoeffCb;       ///< ARL coefficient buffer (Cb)
  Int*          m_pcArlCoeffCr;       ///< ARL coefficient buffer (Cr)
  Bool          m_ArlCoeffIsAliasedAllocation; ///< ARL coefficient buffer is an alias of the global buffer and must not be free()'d

  static Int*   m_pcGlbArlCoeffY;     ///< ARL coefficient buffer (Y)
  static Int*   m_pcGlbArlCoeffCb;    ///< ARL coefficient buffer (Cb)
  static Int*   m_pcGlbArlCoeffCr;    ///< ARL coefficient buffer (Cr)
#endif
  
  Pel*          m_pcIPCMSampleY;      ///< PCM sample buffer (Y)
  Pel*          m_pcIPCMSampleCb;     ///< PCM sample buffer (Cb)
  Pel*          m_pcIPCMSampleCr;     ///< PCM sample buffer (Cr)

  // -------------------------------------------------------------------------------------------------------------------
  // neighbour access variables
  // -------------------------------------------------------------------------------------------------------------------
  
  TComDataCU*   m_pcCUAboveLeft;      ///< pointer of above-left CU
  TComDataCU*   m_pcCUAboveRight;     ///< pointer of above-right CU
  TComDataCU*   m_pcCUAbove;          ///< pointer of above CU
  TComDataCU*   m_pcCULeft;           ///< pointer of left CU
  TComDataCU*   m_apcCUColocated[2];  ///< pointer of temporally colocated CU's for both directions
  TComMvField   m_cMvFieldA;          ///< motion vector of position A
  TComMvField   m_cMvFieldB;          ///< motion vector of position B
  TComMvField   m_cMvFieldC;          ///< motion vector of position C
  TComMv        m_cMvPred;            ///< motion vector predictor
  
  // -------------------------------------------------------------------------------------------------------------------
  // coding tool information
  // -------------------------------------------------------------------------------------------------------------------
  
  Bool*         m_pbMergeFlag;        ///< array of merge flags
  UChar*        m_puhMergeIndex;      ///< array of merge candidate indices
#if AMP_MRG
  Bool          m_bIsMergeAMP;
#endif
  UChar*        m_puhLumaIntraDir;    ///< array of intra directions (luma)
  UChar*        m_puhChromaIntraDir;  ///< array of intra directions (chroma)
  UChar*        m_puhInterDir;        ///< array of inter directions
  Char*         m_apiMVPIdx[2];       ///< array of motion vector predictor candidates
  Char*         m_apiMVPNum[2];       ///< array of number of possible motion vectors predictors
  Bool*         m_pbIPCMFlag;         ///< array of intra_pcm flags
#if H_3D_NBDV
  DisInfo*      m_pDvInfo;
#endif
#if H_3D_VSP
  Char*         m_piVSPFlag;          ///< array of VSP flags to indicate whehter a block uses VSP or not
                                      ///< 0: non-VSP; 1: VSP
#endif
#if H_3D_SPIVMP
  Bool*         m_pbSPIVMPFlag;       ///< array of sub-PU IVMP flags to indicate whehter a block uses sub-PU IVMP
                                      ///< 0: non-SPIVMP; 1: SPIVMP
#endif
#if H_3D_ARP
  UChar*        m_puhARPW;
#endif
#if H_3D_IC
  Bool*         m_pbICFlag;           ///< array of IC flags
#endif
#if H_3D_DIM
  Pel*          m_dimDeltaDC[DIM_NUM_TYPE][2];
#if H_3D_DIM_DMM
  UInt*         m_dmmWedgeTabIdx[DMM_NUM_TYPE]; 
#endif
#if H_3D_DIM_SDC
  Bool*         m_pbSDCFlag;
  Pel*          m_apSegmentDCOffset[2];
  Pel          m_apDmmPredictor[2];
#endif
#endif
#if H_3D_DBBP
  Bool*         m_pbDBBPFlag;        ///< array of DBBP flags
  DBBPTmpData   m_sDBBPTmpData;
#endif
#if H_3D
  Bool          m_bAvailableFlagA1;    ///< A1 available flag
  Bool          m_bAvailableFlagB1;    ///< B1 available flag
  Bool          m_bAvailableFlagB0;    ///< B0 available flag
  Bool          m_bAvailableFlagA0;    ///< A0 available flag
  Bool          m_bAvailableFlagB2;    ///< B2 available flag
#endif
  // -------------------------------------------------------------------------------------------------------------------
  // misc. variables
  // -------------------------------------------------------------------------------------------------------------------
  
  Bool          m_bDecSubCu;          ///< indicates decoder-mode
  Double        m_dTotalCost;         ///< sum of partition RD costs
#if H_3D_VSO
  Dist          m_uiTotalDistortion;  ///< sum of partition distortion
#else
  UInt          m_uiTotalDistortion;  ///< sum of partition distortion
#endif
  UInt          m_uiTotalBits;        ///< sum of partition bits
  UInt          m_uiTotalBins;       ///< sum of partition bins
  UInt*         m_sliceStartCU;    ///< Start CU address of current slice
  UInt*         m_sliceSegmentStartCU; ///< Start CU address of current slice
  Char          m_codedQP;
#if H_3D
  DisInfo       m_cDefaultDisInfo;    ///< Default disparity information for initializing
#endif
#if H_3D_IV_MERGE
  TComMotionCand  m_mergCands[MRG_IVSHIFT+1];
  Int           m_baseListidc;
#endif

protected:
  
  /// add possible motion vector predictor candidates
  Bool          xAddMVPCand           ( AMVPInfo* pInfo, RefPicList eRefPicList, Int iRefIdx, UInt uiPartUnitIdx, MVP_DIR eDir );
  Bool          xAddMVPCandOrder      ( AMVPInfo* pInfo, RefPicList eRefPicList, Int iRefIdx, UInt uiPartUnitIdx, MVP_DIR eDir );
#if H_3D_VSP
  Bool          xAddVspCand( Int mrgCandIdx, DisInfo* pDInfo, Int& iCount);
#endif
#if H_3D_IV_MERGE
  Bool          xAddIvMRGCand( Int mrgCandIdx, Int& iCount, Int*   ivCandDir, TComMv* ivCandMv, Int* ivCandRefIdx ); 
#endif

  Void          deriveRightBottomIdx        ( UInt uiPartIdx, UInt& ruiPartIdxRB );
  Bool          xGetColMVP( RefPicList eRefPicList, Int uiCUAddr, Int uiPartUnitIdx, TComMv& rcMv, Int& riRefIdx 
#if H_3D_TMVP
  ,
  Bool bMRG = true
#endif
  );
  
  /// compute required bits to encode MVD (used in AMVP)
  UInt          xGetMvdBits           ( TComMv cMvd );
  UInt          xGetComponentBits     ( Int iVal );
  
  /// compute scaling factor from POC difference
#if !H_3D_ARP
  Int           xGetDistScaleFactor   ( Int iCurrPOC, Int iCurrRefPOC, Int iColPOC, Int iColRefPOC );
#endif

  Void xDeriveCenterIdx( UInt uiPartIdx, UInt& ruiPartIdxCenter );

#if H_3D_VSP
  Void xSetMvFieldForVSP  ( TComDataCU *cu, TComPicYuv *picRefDepth, TComMv *dv, UInt partAddr, Int width, Int height, Int *shiftLUT, RefPicList refPicList, Int refIdx, Bool isDepth, Int &vspSize );
#endif

public:
  TComDataCU();
  virtual ~TComDataCU();
  
  // -------------------------------------------------------------------------------------------------------------------
  // create / destroy / initialize / copy
  // -------------------------------------------------------------------------------------------------------------------
#if H_3D_ARP
  Int           xGetDistScaleFactor   ( Int iCurrPOC, Int iCurrRefPOC, Int iColPOC, Int iColRefPOC );
#endif 
  Void          create                ( UInt uiNumPartition, UInt uiWidth, UInt uiHeight, Bool bDecSubCu, Int unitSize
#if ADAPTIVE_QP_SELECTION
    , Bool bGlobalRMARLBuffer = false
#endif  
    );
  Void          destroy               ();
  
  Void          initCU                ( TComPic* pcPic, UInt uiCUAddr );
  Void          initEstData           ( UInt uiDepth, Int qp, Bool bTransquantBypass );
  Void          initSubCU             ( TComDataCU* pcCU, UInt uiPartUnitIdx, UInt uiDepth, Int qp );
  Void          setOutsideCUPart      ( UInt uiAbsPartIdx, UInt uiDepth );
#if H_3D_NBDV
  Void          copyDVInfoFrom (TComDataCU* pcCU, UInt uiAbsPartIdx);
#endif
  Void          copySubCU             ( TComDataCU* pcCU, UInt uiPartUnitIdx, UInt uiDepth );
  Void          copyInterPredInfoFrom ( TComDataCU* pcCU, UInt uiAbsPartIdx, RefPicList eRefPicList
#if H_3D_NBDV
  , Bool bNBDV = false
#endif
  );
  Void          copyPartFrom          ( TComDataCU* pcCU, UInt uiPartUnitIdx, UInt uiDepth );
  
  Void          copyToPic             ( UChar uiDepth );
  Void          copyToPic             ( UChar uiDepth, UInt uiPartIdx, UInt uiPartDepth );
  
  // -------------------------------------------------------------------------------------------------------------------
  // member functions for CU description
  // -------------------------------------------------------------------------------------------------------------------
  
  TComPic*      getPic                ()                        { return m_pcPic;           }
  TComSlice*    getSlice              ()                        { return m_pcSlice;         }
#if H_3D_DISABLE_CHROMA
  Void         setSlice              ( TComSlice* pcSlice)     { m_pcSlice = pcSlice;       }
#endif
  UInt&         getAddr               ()                        { return m_uiCUAddr;        }
  UInt&         getZorderIdxInCU      ()                        { return m_uiAbsIdxInLCU; }
  UInt          getSCUAddr            ();
  UInt          getCUPelX             ()                        { return m_uiCUPelX;        }
  UInt          getCUPelY             ()                        { return m_uiCUPelY;        }
  TComPattern*  getPattern            ()                        { return m_pcPattern;       }
  
  UChar*        getDepth              ()                        { return m_puhDepth;        }
  UChar         getDepth              ( UInt uiIdx )            { return m_puhDepth[uiIdx]; }
  Void          setDepth              ( UInt uiIdx, UChar  uh ) { m_puhDepth[uiIdx] = uh;   }
  
  Void          setDepthSubParts      ( UInt uiDepth, UInt uiAbsPartIdx );
#if H_3D
  Void          getPosInPic           ( UInt uiAbsPartIndex, Int& riPosX, Int& riPosY );
#endif
  
  // -------------------------------------------------------------------------------------------------------------------
  // member functions for CU data
  // -------------------------------------------------------------------------------------------------------------------
  
  Char*         getPartitionSize      ()                        { return m_pePartSize;        }
  PartSize      getPartitionSize      ( UInt uiIdx )            { return static_cast<PartSize>( m_pePartSize[uiIdx] ); }
  Void          setPartitionSize      ( UInt uiIdx, PartSize uh){ m_pePartSize[uiIdx] = uh;   }
  Void          setPartSizeSubParts   ( PartSize eMode, UInt uiAbsPartIdx, UInt uiDepth );
  Void          setCUTransquantBypassSubParts( Bool flag, UInt uiAbsPartIdx, UInt uiDepth );
  
#if H_3D_DBBP
  Pel*          getVirtualDepthBlock(UInt uiAbsPartIdx, UInt uiWidth, UInt uiHeight, UInt& uiDepthStride);
#endif
  
  Bool*        getSkipFlag            ()                        { return m_skipFlag;          }
  Bool         getSkipFlag            (UInt idx)                { return m_skipFlag[idx];     }
  Void         setSkipFlag           ( UInt idx, Bool skip)     { m_skipFlag[idx] = skip;   }
  Void         setSkipFlagSubParts   ( Bool skip, UInt absPartIdx, UInt depth );
#if SEC_DEPTH_INTRA_SKIP_MODE_K0033
  Bool*        getDISFlag            ()                         { return m_bDISFlag;          }
  Bool         getDISFlag            ( UInt idx)                { return m_bDISFlag[idx];     }
  Void         setDISFlag            ( UInt idx, Bool bDIS)     { m_bDISFlag[idx] = bDIS;   }
  Void         setDISFlagSubParts    ( Bool bDIS, UInt absPartIdx, UInt depth );

  UInt*        getDISType            ()                         { return m_uiDISType; }
  UInt         getDISType            ( UInt idx)                { return m_uiDISType[idx];     }
  Void         getDISType            ( UInt idx, UInt uiDISType)     { m_uiDISType[idx] = uiDISType;   }
  Void         setDISTypeSubParts    ( UInt uiDISType, UInt uiAbsPartIdx, UInt uiPUIdx, UInt uiDepth );
#else
#if H_3D_SINGLE_DEPTH
  Bool*        getSingleDepthFlag            ()                        { return m_singleDepthFlag;          }
  Bool         getSingleDepthFlag            (UInt idx)                { return m_singleDepthFlag[idx];     }
  Void         setSingleDepthFlag           ( UInt idx, Bool singleDepth)     { m_singleDepthFlag[idx] = singleDepth;   }
  Void         setSingleDepthFlagSubParts   ( Bool singleDepth, UInt absPartIdx, UInt depth );

  Pel*         getSingleDepthValue( ) { return m_apSingleDepthValue; }
  Pel          getSingleDepthValue            (UInt idx)                { return m_apSingleDepthValue[idx];     }
  Void         setSingleDepthValue           ( UInt idx, Pel pDepthValue)     { m_apSingleDepthValue[idx] = pDepthValue;   }
  Void         setSingleDepthValueSubParts   (Pel singleDepthValue, UInt uiAbsPartIdx, UInt uiPUIdx, UInt uiDepth );
#endif  
#endif
  Char*         getPredictionMode     ()                        { return m_pePredMode;        }
  PredMode      getPredictionMode     ( UInt uiIdx )            { return static_cast<PredMode>( m_pePredMode[uiIdx] ); }
  Bool*         getCUTransquantBypass ()                        { return m_CUTransquantBypass;        }
  Bool          getCUTransquantBypass( UInt uiIdx )             { return m_CUTransquantBypass[uiIdx]; }
  Void          setPredictionMode     ( UInt uiIdx, PredMode uh){ m_pePredMode[uiIdx] = uh;   }
  Void          setPredModeSubParts   ( PredMode eMode, UInt uiAbsPartIdx, UInt uiDepth );
  
#if H_3D_DBBP
  Bool*         getDBBPFlag           ()                        { return m_pbDBBPFlag;               }
  Bool          getDBBPFlag           ( UInt uiIdx )            { return m_pbDBBPFlag[uiIdx];        }
  Void          setDBBPFlag           ( UInt uiIdx, Bool b )    { m_pbDBBPFlag[uiIdx] = b;           }
  Void          setDBBPFlagSubParts   ( Bool bDBBPFlag, UInt uiAbsPartIdx, UInt uiPartIdx, UInt uiDepth );
  DBBPTmpData*  getDBBPTmpData        () { return &m_sDBBPTmpData; }
#endif
  
  UChar*        getWidth              ()                        { return m_puhWidth;          }
  UChar         getWidth              ( UInt uiIdx )            { return m_puhWidth[uiIdx];   }
  Void          setWidth              ( UInt uiIdx, UChar  uh ) { m_puhWidth[uiIdx] = uh;     }
  
  UChar*        getHeight             ()                        { return m_puhHeight;         }
  UChar         getHeight             ( UInt uiIdx )            { return m_puhHeight[uiIdx];  }
  Void          setHeight             ( UInt uiIdx, UChar  uh ) { m_puhHeight[uiIdx] = uh;    }
  
  Void          setSizeSubParts       ( UInt uiWidth, UInt uiHeight, UInt uiAbsPartIdx, UInt uiDepth );
  
  Char*         getQP                 ()                        { return m_phQP;              }
  Char          getQP                 ( UInt uiIdx )            { return m_phQP[uiIdx];       }
  Void          setQP                 ( UInt uiIdx, Char value ){ m_phQP[uiIdx] =  value;     }
  Void          setQPSubParts         ( Int qp,   UInt uiAbsPartIdx, UInt uiDepth );
  Int           getLastValidPartIdx   ( Int iAbsPartIdx );
  Char          getLastCodedQP        ( UInt uiAbsPartIdx );
  Void          setQPSubCUs           ( Int qp, TComDataCU* pcCU, UInt absPartIdx, UInt depth, Bool &foundNonZeroCbf );
  Void          setCodedQP            ( Char qp )               { m_codedQP = qp;             }
  Char          getCodedQP            ()                        { return m_codedQP;           }

  Bool          isLosslessCoded(UInt absPartIdx);
  
  UChar*        getTransformIdx       ()                        { return m_puhTrIdx;          }
  UChar         getTransformIdx       ( UInt uiIdx )            { return m_puhTrIdx[uiIdx];   }
  Void          setTrIdxSubParts      ( UInt uiTrIdx, UInt uiAbsPartIdx, UInt uiDepth );

  UChar*        getTransformSkip      ( TextType eType)    { return m_puhTransformSkip[g_aucConvertTxtTypeToIdx[eType]];}
  UChar         getTransformSkip      ( UInt uiIdx,TextType eType)    { return m_puhTransformSkip[g_aucConvertTxtTypeToIdx[eType]][uiIdx];}
  Void          setTransformSkipSubParts  ( UInt useTransformSkip, TextType eType, UInt uiAbsPartIdx, UInt uiDepth); 
  Void          setTransformSkipSubParts  ( UInt useTransformSkipY, UInt useTransformSkipU, UInt useTransformSkipV, UInt uiAbsPartIdx, UInt uiDepth );

  UInt          getQuadtreeTULog2MinSizeInCU( UInt absPartIdx );
  
  TComCUMvField* getCUMvField         ( RefPicList e )          { return  &m_acCUMvField[e];  }
  
  TCoeff*&      getCoeffY             ()                        { return m_pcTrCoeffY;        }
  TCoeff*&      getCoeffCb            ()                        { return m_pcTrCoeffCb;       }
  TCoeff*&      getCoeffCr            ()                        { return m_pcTrCoeffCr;       }
#if ADAPTIVE_QP_SELECTION
  Int*&         getArlCoeffY          ()                        { return m_pcArlCoeffY;       }
  Int*&         getArlCoeffCb         ()                        { return m_pcArlCoeffCb;      }
  Int*&         getArlCoeffCr         ()                        { return m_pcArlCoeffCr;      }
#endif
  
  Pel*&         getPCMSampleY         ()                        { return m_pcIPCMSampleY;     }
  Pel*&         getPCMSampleCb        ()                        { return m_pcIPCMSampleCb;    }
  Pel*&         getPCMSampleCr        ()                        { return m_pcIPCMSampleCr;    }

#if H_3D_DISABLE_CHROMA
  UChar         getCbf    ( UInt uiIdx, TextType eType )                  { assert( getSlice() != NULL ); assert(  g_aucConvertTxtTypeToIdx[eType] == 0 || !getSlice()->getIsDepth() || m_puhCbf[g_aucConvertTxtTypeToIdx[eType]][uiIdx] == 0 );  return m_puhCbf[g_aucConvertTxtTypeToIdx[eType]][uiIdx];  }
  UChar*        getCbf    ( TextType eType )                              { assert( getSlice() != NULL ); assert(  g_aucConvertTxtTypeToIdx[eType] == 0 || !getSlice()->getIsDepth() || m_puhCbf[g_aucConvertTxtTypeToIdx[eType]] );              return m_puhCbf[g_aucConvertTxtTypeToIdx[eType]];         }  
#else
  UChar         getCbf    ( UInt uiIdx, TextType eType )                  { return m_puhCbf[g_aucConvertTxtTypeToIdx[eType]][uiIdx];  }
  UChar*        getCbf    ( TextType eType )                              { return m_puhCbf[g_aucConvertTxtTypeToIdx[eType]];         }
#endif
  UChar         getCbf    ( UInt uiIdx, TextType eType, UInt uiTrDepth )  { return ( ( getCbf( uiIdx, eType ) >> uiTrDepth ) & 0x1 ); }
#if H_3D_DISABLE_CHROMA
  Void          setCbf    ( UInt uiIdx, TextType eType, UChar uh )        { assert( getSlice() != NULL ); m_puhCbf[g_aucConvertTxtTypeToIdx[eType]][uiIdx] = ( g_aucConvertTxtTypeToIdx[eType] > 0 && getSlice()->getIsDepth() ) ? 0 : uh ;    }
#else
  Void          setCbf    ( UInt uiIdx, TextType eType, UChar uh )        { m_puhCbf[g_aucConvertTxtTypeToIdx[eType]][uiIdx] = uh;    }
#endif
  Void          clearCbf  ( UInt uiIdx, TextType eType, UInt uiNumParts );
  UChar         getQtRootCbf          ( UInt uiIdx )                      { return getCbf( uiIdx, TEXT_LUMA, 0 ) || getCbf( uiIdx, TEXT_CHROMA_U, 0 ) || getCbf( uiIdx, TEXT_CHROMA_V, 0 ); }
  
  Void          setCbfSubParts        ( UInt uiCbfY, UInt uiCbfU, UInt uiCbfV, UInt uiAbsPartIdx, UInt uiDepth          );
  Void          setCbfSubParts        ( UInt uiCbf, TextType eTType, UInt uiAbsPartIdx, UInt uiDepth                    );
  Void          setCbfSubParts        ( UInt uiCbf, TextType eTType, UInt uiAbsPartIdx, UInt uiPartIdx, UInt uiDepth    );
  
  // -------------------------------------------------------------------------------------------------------------------
  // member functions for coding tool information
  // -------------------------------------------------------------------------------------------------------------------
  
  Bool*         getMergeFlag          ()                        { return m_pbMergeFlag;               }
  Bool          getMergeFlag          ( UInt uiIdx )            { return m_pbMergeFlag[uiIdx];        }
  Void          setMergeFlag          ( UInt uiIdx, Bool b )    { m_pbMergeFlag[uiIdx] = b;           }
  Void          setMergeFlagSubParts  ( Bool bMergeFlag, UInt uiAbsPartIdx, UInt uiPartIdx, UInt uiDepth );

  UChar*        getMergeIndex         ()                        { return m_puhMergeIndex;                         }
  UChar         getMergeIndex         ( UInt uiIdx )            { return m_puhMergeIndex[uiIdx];                  }
  Void          setMergeIndex         ( UInt uiIdx, UInt uiMergeIndex ) { m_puhMergeIndex[uiIdx] = uiMergeIndex;  }
  Void          setMergeIndexSubParts ( UInt uiMergeIndex, UInt uiAbsPartIdx, UInt uiPartIdx, UInt uiDepth );
  template <typename T>
  Void          setSubPart            ( T bParameter, T* pbBaseLCU, UInt uiCUAddr, UInt uiCUDepth, UInt uiPUIdx );
#if H_3D_VSP
  template<typename T>
  Void          setSubPartT           ( T uiParameter, T* puhBaseLCU, UInt uiCUAddr, UInt uiCUDepth, UInt uiPUIdx );
#endif
#if AMP_MRG
  Void          setMergeAMP( Bool b )      { m_bIsMergeAMP = b; }
  Bool          getMergeAMP( )             { return m_bIsMergeAMP; }
#endif

  UChar*        getLumaIntraDir       ()                        { return m_puhLumaIntraDir;           }
  UChar         getLumaIntraDir       ( UInt uiIdx )            { return m_puhLumaIntraDir[uiIdx];    }
  Void          setLumaIntraDir       ( UInt uiIdx, UChar  uh ) { m_puhLumaIntraDir[uiIdx] = uh;      }
  Void          setLumaIntraDirSubParts( UInt uiDir,  UInt uiAbsPartIdx, UInt uiDepth );
  
  UChar*        getChromaIntraDir     ()                        { return m_puhChromaIntraDir;         }
  UChar         getChromaIntraDir     ( UInt uiIdx )            { return m_puhChromaIntraDir[uiIdx];  }
  Void          setChromaIntraDir     ( UInt uiIdx, UChar  uh ) { m_puhChromaIntraDir[uiIdx] = uh;    }
  Void          setChromIntraDirSubParts( UInt uiDir,  UInt uiAbsPartIdx, UInt uiDepth );
  
  UChar*        getInterDir           ()                        { return m_puhInterDir;               }
  UChar         getInterDir           ( UInt uiIdx )            { return m_puhInterDir[uiIdx];        }
  Void          setInterDir           ( UInt uiIdx, UChar  uh ) { m_puhInterDir[uiIdx] = uh;          }
  Void          setInterDirSubParts   ( UInt uiDir,  UInt uiAbsPartIdx, UInt uiPartIdx, UInt uiDepth );
  Bool*         getIPCMFlag           ()                        { return m_pbIPCMFlag;               }
  Bool          getIPCMFlag           (UInt uiIdx )             { return m_pbIPCMFlag[uiIdx];        }
  Void          setIPCMFlag           (UInt uiIdx, Bool b )     { m_pbIPCMFlag[uiIdx] = b;           }
  Void          setIPCMFlagSubParts   (Bool bIpcmFlag, UInt uiAbsPartIdx, UInt uiDepth);
#if H_3D_NBDV
  Void          setDvInfoSubParts     ( DisInfo cDvInfo, UInt uiAbsPartIdx, UInt uiDepth );
  Void          setDvInfoSubParts     ( DisInfo cDvInfo, UInt uiAbsPartIdx, UInt uiPartIdx, UInt uiDepth);
  DisInfo*      getDvInfo             ()                        { return m_pDvInfo;                 }
  DisInfo       getDvInfo             (UInt uiIdx)              { return m_pDvInfo[uiIdx];          }
#endif
#if H_3D_NBDV
  Void          xDeriveRightBottomNbIdx(Int &uiLCUIdxRBNb, Int &uiPartIdxRBNb );
  Bool          xCheckSpatialNBDV (TComDataCU* pcTmpCU, UInt uiIdx, DisInfo* pNbDvInfo, Bool bSearchForMvpDv, IDVInfo* paMvpDvInfo,
                                   UInt uiMvpDvPos
#if H_3D_NBDV_REF
  , Bool bDepthRefine = false
#endif
  );
  Bool          xGetColDisMV      ( Int currCandPic, RefPicList eRefPicList, Int refidx, Int uiCUAddr, Int uiPartUnitIdx, TComMv& rcMv, Int & iTargetViewIdx, Int & iStartViewIdx );
#if SEC_ARP_REM_ENC_RESTRICT_K0035
  Void          getDisMvpCandNBDV ( DisInfo* pDInfo
#else
  Bool          getDisMvpCandNBDV ( DisInfo* pDInfo
#endif
#if H_3D_NBDV_REF
   , Bool bDepthRefine = false
#endif
   ); 
   
#if H_3D
#if SEC_ARP_REM_ENC_RESTRICT_K0035
  Void          getDispforDepth  ( UInt uiPartIdx, UInt uiPartAddr, DisInfo* cDisp);
#else
  Bool          getDispforDepth  ( UInt uiPartIdx, UInt uiPartAddr, DisInfo* cDisp);
#endif
  Bool          getDispMvPredCan(UInt uiPartIdx, RefPicList eRefPicList, Int iRefIdx, Int* paiPdmRefIdx, TComMv* pacPdmMv, DisInfo* pDis, Int* iPdm );
#endif
#if SEC_DEPTH_INTRA_SKIP_MODE_K0033
   Bool          getNeighDepth (UInt uiPartIdx, UInt uiPartAddr, Pel* pNeighDepth, Int index);
#else
#if H_3D_SINGLE_DEPTH
   Bool          getNeighDepth (UInt uiPartIdx, UInt uiPartAddr, Pel* pNeighDepth, Int index);
#endif
#endif
#if H_3D_NBDV_REF
  Pel           getMcpFromDM(TComPicYuv* pcBaseViewDepthPicYuv, TComMv* mv, Int iBlkX, Int iBlkY, Int iWidth, Int iHeight, Int* aiShiftLUT );
  Void          estimateDVFromDM(Int refViewIdx, UInt uiPartIdx, TComPic* picDepth, UInt uiPartAddr, TComMv* cMvPred );
#endif //H_3D_NBDV_REF
#endif
#if  H_3D_FAST_TEXTURE_ENCODING
  Void          getIVNStatus       ( UInt uiPartIdx,  DisInfo* pDInfo, Bool& bIVFMerge,  Int& iIVFMaxD);
#endif
#if H_3D_SPIVMP
  Void          getSPPara(Int iPUWidth, Int iPUHeight, Int& iNumSP, Int& iNumSPInOneLine, Int& iSPWidth, Int& iSPHeight);
  Void          getSPAbsPartIdx(UInt uiBaseAbsPartIdx, Int iWidth, Int iHeight, Int iPartIdx, Int iNumPartLine, UInt& ruiPartAddr );
  Void          setInterDirSP( UInt uiDir, UInt uiAbsPartIdx, Int iWidth, Int iHeight );
#endif
#if H_3D_IV_MERGE
  Bool          getInterViewMergeCands          ( UInt uiPartIdx, Int* paiPdmRefIdx, TComMv* pacPdmMv, DisInfo* pDInfo, Int* availableMcDc, Bool bIsDepth           

#if H_3D_SPIVMP
    , TComMvField* pcMFieldSP, UChar* puhInterDirSP
#endif    
    , Bool bICFlag
    );   
#endif
#if H_3D_ARP
  UChar*        getARPW            ()                        { return m_puhARPW;               }
  UChar         getARPW            ( UInt uiIdx )            { return m_puhARPW[uiIdx];        }
  Void          setARPW            ( UInt uiIdx, UChar w )   { m_puhARPW[uiIdx] = w;           }
  Void          setARPWSubParts    ( UChar w, UInt uiAbsPartIdx, UInt uiDepth );
  Double        getARPWFactor      ( UInt uiIdx );
#endif
#if H_3D_IC
  Bool*         getICFlag          ()                        { return m_pbICFlag;               }
  Bool          getICFlag          ( UInt uiIdx )            { return m_pbICFlag[uiIdx];        }
  Void          setICFlag          ( UInt uiIdx, Bool  uh )  { m_pbICFlag[uiIdx] = uh;          }
  Void          setICFlagSubParts  ( Bool bICFlag,  UInt uiAbsPartIdx, UInt uiPartIdx, UInt uiDepth );
  Bool          isICFlagRequired   ( UInt uiAbsPartIdx );
  Void          getPartIndexAndSize( UInt uiPartIdx, UInt& ruiPartAddr, Int& riWidth, Int& riHeight, UInt uiAbsPartIdx = 0, Bool bLCU = false);
#else
  // -------------------------------------------------------------------------------------------------------------------
  // member functions for accessing partition information
  // -------------------------------------------------------------------------------------------------------------------
  
  Void          getPartIndexAndSize   ( UInt uiPartIdx, UInt& ruiPartAddr, Int& riWidth, Int& riHeight );
#endif
UChar         getNumPartitions       ();
  Bool          isFirstAbsZorderIdxInDepth (UInt uiAbsPartIdx, UInt uiDepth);

#if H_3D_DIM
  Pel*  getDimDeltaDC                 ( UInt dimType, UInt segId )                      { return m_dimDeltaDC[dimType][segId];        } 
  Pel   getDimDeltaDC                 ( UInt dimType, UInt segId, UInt uiIdx )          { return m_dimDeltaDC[dimType][segId][uiIdx]; } 
  Void  setDimDeltaDC                 ( UInt dimType, UInt segId, UInt uiIdx, Pel val ) { m_dimDeltaDC[dimType][segId][uiIdx] = val;  }
#if H_3D_DIM_DMM
  UInt* getDmmWedgeTabIdx             ( UInt dmmType )                          { return m_dmmWedgeTabIdx[dmmType];          }        
  UInt  getDmmWedgeTabIdx             ( UInt dmmType, UInt uiIdx )              { return m_dmmWedgeTabIdx[dmmType][uiIdx];   }
  Void  setDmmWedgeTabIdx             ( UInt dmmType, UInt uiIdx, UInt tabIdx ) { m_dmmWedgeTabIdx[dmmType][uiIdx] = tabIdx; }
  Void  setDmmWedgeTabIdxSubParts     ( UInt tabIdx, UInt dmmType, UInt uiAbsPartIdx, UInt uiDepth );

#endif
#if H_3D_DIM_SDC
  Bool*         getSDCFlag          ()                        { return m_pbSDCFlag;               }
  Bool          getSDCFlag          ( UInt uiIdx )            { return m_pbSDCFlag[uiIdx];        }
  Void          setSDCFlagSubParts  ( Bool bSDCFlag, UInt uiAbsPartIdx, UInt uiDepth );
  
  Bool          getSDCAvailable             ( UInt uiAbsPartIdx );
  
  Pel*          getSDCSegmentDCOffset( UInt uiSeg ) { return m_apSegmentDCOffset[uiSeg]; }
  Pel           getSDCSegmentDCOffset( UInt uiSeg, UInt uiPartIdx ) { return m_apSegmentDCOffset[uiSeg][uiPartIdx]; }
  Void          setSDCSegmentDCOffset( Pel pOffset, UInt uiSeg, UInt uiPartIdx) { m_apSegmentDCOffset[uiSeg][uiPartIdx] = pOffset; }
  Void          setDmmPredictor ( Pel pOffset, UInt uiSeg) { m_apDmmPredictor[uiSeg] = pOffset; }
  Pel           getDmmPredictor ( UInt uiSeg) { return m_apDmmPredictor[uiSeg]; }
  UInt          getCtxSDCFlag          ( UInt   uiAbsPartIdx );
#endif
#endif
  
  // -------------------------------------------------------------------------------------------------------------------
  // member functions for motion vector
  // -------------------------------------------------------------------------------------------------------------------
  
  Void          getMvField            ( TComDataCU* pcCU, UInt uiAbsPartIdx, RefPicList eRefPicList, TComMvField& rcMvField );
  
  Void          fillMvpCand           ( UInt uiPartIdx, UInt uiPartAddr, RefPicList eRefPicList, Int iRefIdx, AMVPInfo* pInfo );
  Bool          isDiffMER             ( Int xN, Int yN, Int xP, Int yP);
  Void          getPartPosition       ( UInt partIdx, Int& xP, Int& yP, Int& nPSW, Int& nPSH);
  Void          setMVPIdx             ( RefPicList eRefPicList, UInt uiIdx, Int iMVPIdx)  { m_apiMVPIdx[eRefPicList][uiIdx] = iMVPIdx;  }
  Int           getMVPIdx             ( RefPicList eRefPicList, UInt uiIdx)               { return m_apiMVPIdx[eRefPicList][uiIdx];     }
  Char*         getMVPIdx             ( RefPicList eRefPicList )                          { return m_apiMVPIdx[eRefPicList];            }

  Void          setMVPNum             ( RefPicList eRefPicList, UInt uiIdx, Int iMVPNum ) { m_apiMVPNum[eRefPicList][uiIdx] = iMVPNum;  }
  Int           getMVPNum             ( RefPicList eRefPicList, UInt uiIdx )              { return m_apiMVPNum[eRefPicList][uiIdx];     }
  Char*         getMVPNum             ( RefPicList eRefPicList )                          { return m_apiMVPNum[eRefPicList];            }
  
  Void          setMVPIdxSubParts     ( Int iMVPIdx, RefPicList eRefPicList, UInt uiAbsPartIdx, UInt uiPartIdx, UInt uiDepth );
  Void          setMVPNumSubParts     ( Int iMVPNum, RefPicList eRefPicList, UInt uiAbsPartIdx, UInt uiPartIdx, UInt uiDepth );
  
  Void          clipMv                ( TComMv&     rcMv     );
#if SONY_MV_V_CONST_C0078
  Void          checkMV_V (TComMv&  rcMv,  RefPicList eRefPicList, int iRefIdx );
#endif
  Void          getMvPredLeft         ( TComMv&     rcMvPred )   { rcMvPred = m_cMvFieldA.getMv(); }
  Void          getMvPredAbove        ( TComMv&     rcMvPred )   { rcMvPred = m_cMvFieldB.getMv(); }
  Void          getMvPredAboveRight   ( TComMv&     rcMvPred )   { rcMvPred = m_cMvFieldC.getMv(); }
#if H_3D
  Void          compressMV            ( Int scale );
#else            
  Void          compressMV            ();
#endif  
  // -------------------------------------------------------------------------------------------------------------------
  // utility functions for neighbouring information
  // -------------------------------------------------------------------------------------------------------------------
  
  TComDataCU*   getCULeft                   () { return m_pcCULeft;       }
  TComDataCU*   getCUAbove                  () { return m_pcCUAbove;      }
  TComDataCU*   getCUAboveLeft              () { return m_pcCUAboveLeft;  }
  TComDataCU*   getCUAboveRight             () { return m_pcCUAboveRight; }
  TComDataCU*   getCUColocated              ( RefPicList eRefPicList ) { return m_apcCUColocated[eRefPicList]; }


  TComDataCU*   getPULeft                   ( UInt&  uiLPartUnitIdx, 
                                              UInt uiCurrPartUnitIdx, 
                                              Bool bEnforceSliceRestriction=true, 
                                              Bool bEnforceTileRestriction=true );
  TComDataCU*   getPUAbove                  ( UInt&  uiAPartUnitIdx,
                                              UInt uiCurrPartUnitIdx, 
                                              Bool bEnforceSliceRestriction=true, 
                                              Bool planarAtLCUBoundary = false,
                                              Bool bEnforceTileRestriction=true );
  TComDataCU*   getPUAboveLeft              ( UInt&  uiALPartUnitIdx, UInt uiCurrPartUnitIdx, Bool bEnforceSliceRestriction=true );
  TComDataCU*   getPUAboveRight             ( UInt&  uiARPartUnitIdx, UInt uiCurrPartUnitIdx, Bool bEnforceSliceRestriction=true );
  TComDataCU*   getPUBelowLeft              ( UInt&  uiBLPartUnitIdx, UInt uiCurrPartUnitIdx, Bool bEnforceSliceRestriction=true );

  TComDataCU*   getQpMinCuLeft              ( UInt&  uiLPartUnitIdx , UInt uiCurrAbsIdxInLCU );
  TComDataCU*   getQpMinCuAbove             ( UInt&  aPartUnitIdx , UInt currAbsIdxInLCU );
  Char          getRefQP                    ( UInt   uiCurrAbsIdxInLCU                       );

  TComDataCU*   getPUAboveRightAdi          ( UInt&  uiARPartUnitIdx, UInt uiCurrPartUnitIdx, UInt uiPartUnitOffset = 1, Bool bEnforceSliceRestriction=true );
  TComDataCU*   getPUBelowLeftAdi           ( UInt&  uiBLPartUnitIdx, UInt uiCurrPartUnitIdx, UInt uiPartUnitOffset = 1, Bool bEnforceSliceRestriction=true );
  
  Void          deriveLeftRightTopIdx       ( UInt uiPartIdx, UInt& ruiPartIdxLT, UInt& ruiPartIdxRT );
  Void          deriveLeftBottomIdx         ( UInt uiPartIdx, UInt& ruiPartIdxLB );
  
  Void          deriveLeftRightTopIdxAdi    ( UInt& ruiPartIdxLT, UInt& ruiPartIdxRT, UInt uiPartOffset, UInt uiPartDepth );
  Void          deriveLeftBottomIdxAdi      ( UInt& ruiPartIdxLB, UInt  uiPartOffset, UInt uiPartDepth );
  
  Bool          hasEqualMotion              ( UInt uiAbsPartIdx, TComDataCU* pcCandCU, UInt uiCandAbsPartIdx );

#if H_3D
  Bool          getAvailableFlagA1() { return m_bAvailableFlagA1;}
  Bool          getAvailableFlagB1() { return m_bAvailableFlagB1;}
  Bool          getAvailableFlagB0() { return m_bAvailableFlagB0;}
  Bool          getAvailableFlagA0() { return m_bAvailableFlagA0;}
  Bool          getAvailableFlagB2() { return m_bAvailableFlagB2;}
  Void          initAvailableFlags() { m_bAvailableFlagA1 = m_bAvailableFlagB1 = m_bAvailableFlagB0 = m_bAvailableFlagA0 = m_bAvailableFlagB2 = 0;  }
  Void          buildMCL(TComMvField* pcMFieldNeighbours, UChar* puhInterDirNeighbours
#if H_3D_VSP
    , Int* vspFlag
#endif
#if H_3D_SPIVMP
    , Bool* pbSPIVMPFlag
#endif
    , Int& numValidMergeCand
    );
  Void          getInterMergeCandidates( UInt uiAbsPartIdx, UInt uiPUIdx, TComMvField* pcMvFieldNeighbours, UChar* puhInterDirNeighbours, Int& numValidMergeCand, Int mrgCandIdx = -1);
  Void          xGetInterMergeCandidates ( UInt uiAbsPartIdx, UInt uiPUIdx, TComMvField* pcMFieldNeighbours, UChar* puhInterDirNeighbours
#else
  Void          getInterMergeCandidates ( UInt uiAbsPartIdx, UInt uiPUIdx, TComMvField* pcMFieldNeighbours, UChar* puhInterDirNeighbours
#endif
#if H_3D_SPIVMP
                                            , TComMvField* pcMvFieldSP, UChar* puhInterDirSP
#endif
                                            , Int& numValidMergeCand, Int mrgCandIdx = -1
                                            );

#if H_3D_VSP
#if H_3D_SPIVMP
  Bool*         getSPIVMPFlag        ()                        { return m_pbSPIVMPFlag;          }
  Bool          getSPIVMPFlag        ( UInt uiIdx )            { return m_pbSPIVMPFlag[uiIdx];   }
  Void          setSPIVMPFlag        ( UInt uiIdx, Bool n )     { m_pbSPIVMPFlag[uiIdx] = n;      }
  Void          setSPIVMPFlagSubParts( Bool bSPIVMPFlag, UInt uiAbsPartIdx, UInt uiPartIdx, UInt uiDepth );
#endif

  Char*         getVSPFlag        ()                        { return m_piVSPFlag;          }
  Char          getVSPFlag        ( UInt uiIdx )            { return m_piVSPFlag[uiIdx];   }
  Void          setVSPFlag        ( UInt uiIdx, Int n )     { m_piVSPFlag[uiIdx] = n;      }
  Void          setVSPFlagSubParts( Char iVSPFlag, UInt uiAbsPartIdx, UInt uiPartIdx, UInt uiDepth );
  Void          setMvFieldPUForVSP    ( TComDataCU* cu, UInt partAddr, Int width, Int height, RefPicList refPicList, Int refIdx, Int &vspSize );
#endif
  Void          deriveLeftRightTopIdxGeneral  ( UInt uiAbsPartIdx, UInt uiPartIdx, UInt& ruiPartIdxLT, UInt& ruiPartIdxRT );
  Void          deriveLeftBottomIdxGeneral    ( UInt uiAbsPartIdx, UInt uiPartIdx, UInt& ruiPartIdxLB );
  
  
  // -------------------------------------------------------------------------------------------------------------------
  // member functions for modes
  // -------------------------------------------------------------------------------------------------------------------
  
  Bool          isIntra   ( UInt uiPartIdx )  { return m_pePredMode[ uiPartIdx ] == MODE_INTRA; }
  Bool          isSkipped ( UInt uiPartIdx );                                                     ///< SKIP (no residual)
  Bool          isBipredRestriction( UInt puIdx );

#if H_3D_IC
  Bool          isIC      ( UInt uiPartIdx );
#endif

  // -------------------------------------------------------------------------------------------------------------------
  // member functions for symbol prediction (most probable / mode conversion)
  // -------------------------------------------------------------------------------------------------------------------
  
  UInt          getIntraSizeIdx                 ( UInt uiAbsPartIdx                                       );
  
  Void          getAllowedChromaDir             ( UInt uiAbsPartIdx, UInt* uiModeList );
  Int           getIntraDirLumaPredictor        ( UInt uiAbsPartIdx, Int* uiIntraDirPred, Int* piMode = NULL );
  
  // -------------------------------------------------------------------------------------------------------------------
  // member functions for SBAC context
  // -------------------------------------------------------------------------------------------------------------------
  
  UInt          getCtxSplitFlag                 ( UInt   uiAbsPartIdx, UInt uiDepth                   );
  UInt          getCtxQtCbf                     ( TextType eType, UInt uiTrDepth );

  UInt          getCtxSkipFlag                  ( UInt   uiAbsPartIdx                                 );
  UInt          getCtxInterDir                  ( UInt   uiAbsPartIdx                                 );
  
#if H_3D_ARP
  UInt          getCTXARPWFlag                  ( UInt   uiAbsPartIdx                                 );
#endif  
  UInt          getSliceStartCU         ( UInt pos )                  { return m_sliceStartCU[pos-m_uiAbsIdxInLCU];                                                                                          }
  UInt          getSliceSegmentStartCU  ( UInt pos )                  { return m_sliceSegmentStartCU[pos-m_uiAbsIdxInLCU];                                                                                   }
  UInt&         getTotalBins            ()                            { return m_uiTotalBins;                                                                                                  }
  // -------------------------------------------------------------------------------------------------------------------
  // member functions for RD cost storage
  // -------------------------------------------------------------------------------------------------------------------
  
  Double&       getTotalCost()                  { return m_dTotalCost;        }
#if H_3D_VSO
  Dist&         getTotalDistortion()            { return m_uiTotalDistortion; }
#else
  UInt&         getTotalDistortion()            { return m_uiTotalDistortion; }
#endif
  UInt&         getTotalBits()                  { return m_uiTotalBits;       }
  UInt&         getTotalNumPart()               { return m_uiNumPartition;    }

  UInt          getCoefScanIdx(UInt uiAbsPartIdx, UInt uiWidth, Bool bIsLuma, Bool bIsIntra);


#if H_3D_DIM
  Bool         isDMM1UpscaleMode       ( UInt uiWidth ){ Bool bDMM1UpsampleModeFlag = true; UInt uiBaseWidth = 16; if( uiBaseWidth >= uiWidth ){ bDMM1UpsampleModeFlag = false; } return bDMM1UpsampleModeFlag; };
  UInt         getDMM1BasePatternWidth ( UInt uiWidth ){ UInt uiBaseWidth = 16; if( uiBaseWidth >= uiWidth ){ uiBaseWidth =  uiWidth; } return uiBaseWidth; }
#endif

};

namespace RasterAddress
{
  /** Check whether 2 addresses point to the same column
   * \param addrA          First address in raster scan order
   * \param addrB          Second address in raters scan order
   * \param numUnitsPerRow Number of units in a row
   * \return Result of test
   */
  static inline Bool isEqualCol( Int addrA, Int addrB, Int numUnitsPerRow )
  {
    // addrA % numUnitsPerRow == addrB % numUnitsPerRow
    return (( addrA ^ addrB ) &  ( numUnitsPerRow - 1 ) ) == 0;
  }
  
  /** Check whether 2 addresses point to the same row
   * \param addrA          First address in raster scan order
   * \param addrB          Second address in raters scan order
   * \param numUnitsPerRow Number of units in a row
   * \return Result of test
   */
  static inline Bool isEqualRow( Int addrA, Int addrB, Int numUnitsPerRow )
  {
    // addrA / numUnitsPerRow == addrB / numUnitsPerRow
    return (( addrA ^ addrB ) &~ ( numUnitsPerRow - 1 ) ) == 0;
  }
  
  /** Check whether 2 addresses point to the same row or column
   * \param addrA          First address in raster scan order
   * \param addrB          Second address in raters scan order
   * \param numUnitsPerRow Number of units in a row
   * \return Result of test
   */
  static inline Bool isEqualRowOrCol( Int addrA, Int addrB, Int numUnitsPerRow )
  {
    return isEqualCol( addrA, addrB, numUnitsPerRow ) | isEqualRow( addrA, addrB, numUnitsPerRow );
  }
  
  /** Check whether one address points to the first column
   * \param addr           Address in raster scan order
   * \param numUnitsPerRow Number of units in a row
   * \return Result of test
   */
  static inline Bool isZeroCol( Int addr, Int numUnitsPerRow )
  {
    // addr % numUnitsPerRow == 0
    return ( addr & ( numUnitsPerRow - 1 ) ) == 0;
  }
  
  /** Check whether one address points to the first row
   * \param addr           Address in raster scan order
   * \param numUnitsPerRow Number of units in a row
   * \return Result of test
   */
  static inline Bool isZeroRow( Int addr, Int numUnitsPerRow )
  {
    // addr / numUnitsPerRow == 0
    return ( addr &~ ( numUnitsPerRow - 1 ) ) == 0;
  }
  
  /** Check whether one address points to a column whose index is smaller than a given value
   * \param addr           Address in raster scan order
   * \param val            Given column index value
   * \param numUnitsPerRow Number of units in a row
   * \return Result of test
   */
  static inline Bool lessThanCol( Int addr, Int val, Int numUnitsPerRow )
  {
    // addr % numUnitsPerRow < val
    return ( addr & ( numUnitsPerRow - 1 ) ) < val;
  }
  
  /** Check whether one address points to a row whose index is smaller than a given value
   * \param addr           Address in raster scan order
   * \param val            Given row index value
   * \param numUnitsPerRow Number of units in a row
   * \return Result of test
   */
  static inline Bool lessThanRow( Int addr, Int val, Int numUnitsPerRow )
  {
    // addr / numUnitsPerRow < val
    return addr < val * numUnitsPerRow;
  }
};

//! \}

#endif
