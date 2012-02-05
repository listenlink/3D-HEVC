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
#include "TComYuv.h"

#include <algorithm>
#include <vector>

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
#if HHI_MPI
  Int*          m_piTextureModeDepth; ///< at which depth is prediction data inherited from texture picture ( -1 : none )
#endif
  
  // -------------------------------------------------------------------------------------------------------------------
  // CU data
  // -------------------------------------------------------------------------------------------------------------------
  
  PartSize*     m_pePartSize;         ///< array of partition sizes
  PredMode*     m_pePredMode;         ///< array of prediction modes
  UChar*        m_phQP;               ///< array of QP values
  UChar*        m_puhTrIdx;           ///< array of transform indices
  UChar*        m_puhCbf[3];          ///< array of coded block flags (CBF)
  TComCUMvField m_acCUMvField[2];     ///< array of motion vectors
  TCoeff*       m_pcTrCoeffY;         ///< transformed coefficient buffer (Y)
  TCoeff*       m_pcTrCoeffCb;        ///< transformed coefficient buffer (Cb)
  TCoeff*       m_pcTrCoeffCr;        ///< transformed coefficient buffer (Cr)
#if SNY_DQP 
  Bool          m_bdQP;               ///< signal if LCU dQP encoded
#endif//SNY_DQP
  
#if POZNAN_EIVD_CALC_PRED_DATA
  TComCUMvField m_acCUMvField2nd[2];              ///< array of motion vectors selected for points with no MP prediction available
#endif
  
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
#if HHI_INTER_VIEW_RESIDUAL_PRED
  Bool*         m_pbResPredAvailable; ///< array of residual prediction available flags
  Bool*         m_pbResPredFlag;      ///< array of residual prediction flags
#endif
  UChar*        m_apuhNeighbourCandIdx[ MRG_MAX_NUM_CANDS ];///< array of motion vector predictor candidates indices
  UChar*        m_puhLumaIntraDir;    ///< array of intra directions (luma)
  UChar*        m_puhChromaIntraDir;  ///< array of intra directions (chroma)
  UChar*        m_puhInterDir;        ///< array of inter directions
  Int*          m_apiMVPIdx[2];       ///< array of motion vector predictor candidates
  Int*          m_apiMVPNum[2];       ///< array of number of possible motion vectors predictors
  UInt*         m_puiAlfCtrlFlag;     ///< array of ALF flags
  UInt*         m_puiTmpAlfCtrlFlag;  ///< temporal array of ALF flags
#if HHI_DMM_WEDGE_INTRA
  UInt*         m_puiWedgeFullTabIdx;
  Int*          m_piWedgeFullDeltaDC1;
  Int*          m_piWedgeFullDeltaDC2;

  UInt*         m_puiWedgePredDirTabIdx;
  Int*          m_piWedgePredDirDeltaDC1;
  Int*          m_piWedgePredDirDeltaDC2;
  Int*          m_piWedgePredDirDeltaEnd;
#endif
#if HHI_DMM_PRED_TEX
  UInt*         m_puiWedgePredTexTabIdx;
  Int*          m_piWedgePredTexDeltaDC1;
  Int*          m_piWedgePredTexDeltaDC2;

  Int*          m_piContourPredTexDeltaDC1;
  Int*          m_piContourPredTexDeltaDC2;
#endif
  
  // -------------------------------------------------------------------------------------------------------------------
  // misc. variables
  // -------------------------------------------------------------------------------------------------------------------
  
  Bool          m_bDecSubCu;          ///< indicates decoder-mode
  Double        m_dTotalCost;         ///< sum of partition RD costs
  Dist          m_uiTotalDistortion;  ///< sum of partition distortion
  UInt          m_uiTotalBits;        ///< sum of partition bits
  UInt          m_uiSliceStartCU;    ///< Start CU address of current slice
  UInt          m_uiEntropySliceStartCU; ///< Start CU address of current slice
  
protected:
  
  /// add possible motion vector predictor candidates
  Bool          xAddMVPCand           ( AMVPInfo* pInfo, RefPicList eRefPicList, Int iRefIdx, UInt uiPartUnitIdx, MVP_DIR eDir );
#if MTK_AMVP_SMVP_DERIVATION
  Bool          xAddMVPCandOrder      ( AMVPInfo* pInfo, RefPicList eRefPicList, Int iRefIdx, UInt uiPartUnitIdx, MVP_DIR eDir );
#endif  

#if MTK_TMVP_H_MRG || MTK_TMVP_H_AMVP
  Void          deriveRightBottomIdx        ( PartSize eCUMode, UInt uiPartIdx, UInt& ruiPartIdxRB );
  Bool          xGetColMVP( RefPicList eRefPicList, Int uiCUAddr, Int uiPartUnitIdx, TComMv& rcMv, Int& riRefIdx );
#endif
  
  /// remove redundant candidates
  Void          xUniqueMVPCand        ( AMVPInfo* pInfo );

  Void xCheckCornerCand( TComDataCU* pcCorner, UInt uiCornerIdx, UInt uiIter, Bool& rbValidCand );
  /// compute required bits to encode MVD (used in AMVP)
  UInt          xGetMvdBits           ( TComMv cMvd );
  UInt          xGetComponentBits     ( Int iVal );
  
  /// compute scaling factor from POC difference
  Int           xGetDistScaleFactor   ( Int iCurrPOC, Int iCurrRefPOC, Int iColPOC, Int iColRefPOC );
  
  /// calculate all CBF's from coefficients
  Void          xCalcCuCbf            ( UChar* puhCbf, UInt uiTrDepth, UInt uiCbfDepth, UInt uiCuDepth );
  
#if FT_TCTR_AMVP || FT_TCTR_MRG
  Void xDeriveCenterIdx( PartSize eCUMode, UInt uiPartIdx, UInt& ruiPartIdxCenter );
  Bool xGetCenterCol( UInt uiPartIdx, RefPicList eRefPicList, int iRefIdx, TComMv *pcMv );
#endif
  
public:
  TComDataCU();
  virtual ~TComDataCU();
  
  // -------------------------------------------------------------------------------------------------------------------
  // create / destroy / initialize / copy
  // -------------------------------------------------------------------------------------------------------------------
  
  Void          create                ( UInt uiNumPartition, UInt uiWidth, UInt uiHeight, Bool bDecSubCu );
  Void          destroy               ();
  
  Void          initCU                ( TComPic* pcPic, UInt uiCUAddr );
  Void          initEstData           ();
  Void          initSubCU             ( TComDataCU* pcCU, UInt uiPartUnitIdx, UInt uiDepth );
  
  Void          copySubCU             ( TComDataCU* pcCU, UInt uiPartUnitIdx, UInt uiDepth );
  Void          copyInterPredInfoFrom ( TComDataCU* pcCU, UInt uiAbsPartIdx, RefPicList eRefPicList );
  Void          copyPartFrom          ( TComDataCU* pcCU, UInt uiPartUnitIdx, UInt uiDepth );
  
  Void          copyToPic             ( UChar uiDepth );
  Void          copyToPic             ( UChar uiDepth, UInt uiPartIdx, UInt uiPartDepth );
  
  // -------------------------------------------------------------------------------------------------------------------
  // member functions for CU description
  // -------------------------------------------------------------------------------------------------------------------
  
  TComPic*      getPic                ()                        { return m_pcPic;           }
  TComSlice*    getSlice              ()                        { return m_pcSlice;         }
  UInt&         getAddr               ()                        { return m_uiCUAddr;        }
  UInt&         getZorderIdxInCU      ()                        { return m_uiAbsIdxInLCU; }
  UInt          getCUPelX             ()                        { return m_uiCUPelX;        }
  UInt          getCUPelY             ()                        { return m_uiCUPelY;        }
  TComPattern*  getPattern            ()                        { return m_pcPattern;       }
  
  UChar*        getDepth              ()                        { return m_puhDepth;        }
  UChar         getDepth              ( UInt uiIdx )            { return m_puhDepth[uiIdx]; }
  Void          setDepth              ( UInt uiIdx, UChar  uh ) { m_puhDepth[uiIdx] = uh;   }
  
#if HHI_MPI
  Int*          getTextureModeDepth   ()                        { return m_piTextureModeDepth; }
  Int           getTextureModeDepth   ( UInt uiIdx )            { return m_piTextureModeDepth[uiIdx]; }
  Void          setTextureModeDepth   ( UInt uiIdx, Int iTextureModeDepth ){ m_piTextureModeDepth[uiIdx] = iTextureModeDepth; }
  Void          setTextureModeDepthSubParts( Int iTextureModeDepth, UInt uiAbsPartIdx, UInt uiDepth );
  Void          copyTextureMotionDataFrom( TComDataCU* pcCU, UInt uiDepth, UInt uiAbsPartIdxSrc, UInt uiAbsPartIdxDst = 0 );
#endif

  Void          setDepthSubParts      ( UInt uiDepth, UInt uiAbsPartIdx );
  Void          getPosInPic           ( UInt uiAbsPartIndex, Int& riPosX, Int& riPosY );
  
  // -------------------------------------------------------------------------------------------------------------------
  // member functions for CU data
  // -------------------------------------------------------------------------------------------------------------------
  
  PartSize*     getPartitionSize      ()                        { return m_pePartSize;        }
  PartSize      getPartitionSize      ( UInt uiIdx )            { return m_pePartSize[uiIdx]; }
  Void          setPartitionSize      ( UInt uiIdx, PartSize uh){ m_pePartSize[uiIdx] = uh;   }
  Void          setPartSizeSubParts   ( PartSize eMode, UInt uiAbsPartIdx, UInt uiDepth );
  
  PredMode*     getPredictionMode     ()                        { return m_pePredMode;        }
  PredMode      getPredictionMode     ( UInt uiIdx )            { return m_pePredMode[uiIdx]; }
  Void          setPredictionMode     ( UInt uiIdx, PredMode uh){ m_pePredMode[uiIdx] = uh;   }
  Void          setPredModeSubParts   ( PredMode eMode, UInt uiAbsPartIdx, UInt uiDepth );
  
  UChar*        getWidth              ()                        { return m_puhWidth;          }
  UChar         getWidth              ( UInt uiIdx )            { return m_puhWidth[uiIdx];   }
  Void          setWidth              ( UInt uiIdx, UChar  uh ) { m_puhWidth[uiIdx] = uh;     }
  
  UChar*        getHeight             ()                        { return m_puhHeight;         }
  UChar         getHeight             ( UInt uiIdx )            { return m_puhHeight[uiIdx];  }
  Void          setHeight             ( UInt uiIdx, UChar  uh ) { m_puhHeight[uiIdx] = uh;    }
  
  Void          setSizeSubParts       ( UInt uiWidth, UInt uiHeight, UInt uiAbsPartIdx, UInt uiDepth );
  
  UChar*        getQP                 ()                        { return m_phQP;              }
  UChar         getQP                 ( UInt uiIdx )            { return m_phQP[uiIdx];       }
  Void          setQP                 ( UInt uiIdx, UChar  uh ) { m_phQP[uiIdx] = uh;         }
  Void          setQPSubParts         ( UInt uiQP,   UInt uiAbsPartIdx, UInt uiDepth );
#if SNY_DQP
  Bool          getdQPFlag            ()                        { return m_bdQP;              }
  Void          setdQPFlag            ( Bool b )                { m_bdQP = b;                 }
#endif//SNY_DQP
  
  UChar*        getTransformIdx       ()                        { return m_puhTrIdx;          }
  UChar         getTransformIdx       ( UInt uiIdx )            { return m_puhTrIdx[uiIdx];   }
  Void          setTrIdxSubParts      ( UInt uiTrIdx, UInt uiAbsPartIdx, UInt uiDepth );
  
  UInt          getQuadtreeTULog2MinSizeInCU( UInt uiIdx );
  
#if HHI_RQT_FORCE_SPLIT_ACC2_PU || HHI_RQT_DISABLE_SUB
  UInt          getQuadtreeTULog2RootSizeInCU( UInt uiIdx );
#endif
  
  TComCUMvField* getCUMvField         ( RefPicList e )          { return  &m_acCUMvField[e];  }
  
  TCoeff*&      getCoeffY             ()                        { return m_pcTrCoeffY;        }
  TCoeff*&      getCoeffCb            ()                        { return m_pcTrCoeffCb;       }
  TCoeff*&      getCoeffCr            ()                        { return m_pcTrCoeffCr;       }
  
  UChar         getCbf    ( UInt uiIdx, TextType eType )                  { return m_puhCbf[g_aucConvertTxtTypeToIdx[eType]][uiIdx];  }
  UChar*        getCbf    ( TextType eType )                              { return m_puhCbf[g_aucConvertTxtTypeToIdx[eType]];         }
  UChar         getCbf    ( UInt uiIdx, TextType eType, UInt uiTrDepth )  { return ( ( getCbf( uiIdx, eType ) >> uiTrDepth ) & 0x1 ); }
  Void          setCbf    ( UInt uiIdx, TextType eType, UChar uh )        { m_puhCbf[g_aucConvertTxtTypeToIdx[eType]][uiIdx] = uh;    }
  Void          clearCbf  ( UInt uiIdx, TextType eType, UInt uiNumParts );
  UChar         getQtRootCbf          ( UInt uiIdx )                      { return getCbf( uiIdx, TEXT_LUMA, 0 ) || getCbf( uiIdx, TEXT_CHROMA_U, 0 ) || getCbf( uiIdx, TEXT_CHROMA_V, 0 ); }
  
  Void          setCbfSubParts        ( UInt uiCbfY, UInt uiCbfU, UInt uiCbfV, UInt uiAbsPartIdx, UInt uiDepth          );
  Void          setCbfSubParts        ( UInt uiCbf, TextType eTType, UInt uiAbsPartIdx, UInt uiDepth                    );
#if HHI_MRG_SKIP
  Void          setCbfSubParts        ( UInt uiCbf, TextType eTType, UInt uiAbsPartIdx, UInt uiPartIdx, UInt uiDepth    );
#endif
  
#if POZNAN_EIVD_CALC_PRED_DATA
  TComCUMvField* getCUMvField2nd         ( RefPicList e )          { return  &m_acCUMvField2nd[e];  }
#endif
  
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

  UChar*        getNeighbourCandIdx         ( UInt uiCandIdx )            { return m_apuhNeighbourCandIdx[uiCandIdx];           }
  UChar         getNeighbourCandIdx         ( UInt uiCandIdx, UInt uiIdx ){ return m_apuhNeighbourCandIdx[uiCandIdx][uiIdx];           }
  Void          setNeighbourCandIdx         ( UInt uiCandIdx, UInt uiIdx, UChar uhNeighCands ) { m_apuhNeighbourCandIdx[uiCandIdx][uiIdx] = uhNeighCands;}
  Void          setNeighbourCandIdxSubParts ( UInt uiCandIdx, UChar uhNumCands, UInt uiAbsPartIdx, UInt uiPartIdx, UInt uiDepth );

#if HHI_INTER_VIEW_RESIDUAL_PRED
  Bool*         getResPredAvail         ()                        { return m_pbResPredAvailable;        }
  Bool          getResPredAvail         ( UInt uiIdx )            { return m_pbResPredAvailable[uiIdx]; }
  Void          setResPredAvail         ( UInt uiIdx, Bool b )    { m_pbResPredAvailable[uiIdx] = b;    }
  Void          setResPredAvailSubParts ( Bool b, UInt uiAbsPartIdx, UInt uiPartIdx, UInt uiDepth );
  
  Bool*         getResPredFlag          ()                        { return m_pbResPredFlag;        }
  Bool          getResPredFlag          ( UInt uiIdx )            { return m_pbResPredFlag[uiIdx]; }
  Void          setResPredFlag          ( UInt uiIdx, Bool b )    { m_pbResPredFlag[uiIdx] = b;    }
  Void          setResPredFlagSubParts  ( Bool b, UInt uiAbsPartIdx, UInt uiPartIdx, UInt uiDepth );

  Void          setResPredIndicator     ( Bool bAv, Bool bRP )    { m_pbResPredAvailable[0] = bAv; m_pbResPredFlag[0] = bRP; }
#endif

  Void          setSubPartBool        ( Bool bParameter, Bool* pbBaseLCU, UInt uiCUAddr, UInt uiCUDepth, UInt uiPUIdx );
  Void          setSubPartUChar       ( UInt bParameter, UChar* pbBaseLCU, UInt uiCUAddr, UInt uiCUDepth, UInt uiPUIdx );

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
  
  UInt*         getAlfCtrlFlag        ()                        { return m_puiAlfCtrlFlag;            }
  UInt          getAlfCtrlFlag        ( UInt uiIdx )            { return m_puiAlfCtrlFlag[uiIdx];     }
  Void          setAlfCtrlFlag        ( UInt uiIdx, UInt uiFlag){ m_puiAlfCtrlFlag[uiIdx] = uiFlag;   }
  Void          setAlfCtrlFlagSubParts( UInt uiFlag, UInt uiAbsPartIdx, UInt uiDepth );
  
  Void          createTmpAlfCtrlFlag  ();
  Void          destroyTmpAlfCtrlFlag ();
  Void          copyAlfCtrlFlagToTmp  ();
  Void          copyAlfCtrlFlagFromTmp();
  
#if HHI_DMM_WEDGE_INTRA
  UInt*         getWedgeFullTabIdx         ()                        { return m_puiWedgeFullTabIdx;        }
  UInt          getWedgeFullTabIdx         ( UInt uiIdx )            { return m_puiWedgeFullTabIdx[uiIdx]; }
  Void          setWedgeFullTabIdx         ( UInt uiIdx, UInt uh )   { m_puiWedgeFullTabIdx[uiIdx] = uh;   }
  Void          setWedgeFullTabIdxSubParts ( UInt uiTIdx, UInt uiAbsPartIdx, UInt uiDepth );

  Int*          getWedgeFullDeltaDC1       ()                        { return m_piWedgeFullDeltaDC1;             }
  Int           getWedgeFullDeltaDC1       ( UInt uiIdx )            { return m_piWedgeFullDeltaDC1[uiIdx];      }
  Void          setWedgeFullDeltaDC1       ( UInt uiIdx, Int i )     { m_piWedgeFullDeltaDC1[uiIdx] = i;         }
  Void          setWedgeFullDeltaDC1SubParts( Int iDC1, UInt uiAbsPartIdx, UInt uiDepth );

  Int*          getWedgeFullDeltaDC2       ()                        { return m_piWedgeFullDeltaDC2;             }
  Int           getWedgeFullDeltaDC2       ( UInt uiIdx )            { return m_piWedgeFullDeltaDC2[uiIdx];      }
  Void          setWedgeFullDeltaDC2       ( UInt uiIdx, Int i )     { m_piWedgeFullDeltaDC2[uiIdx] = i;         }
  Void          setWedgeFullDeltaDC2SubParts( Int iDC2, UInt uiAbsPartIdx, UInt uiDepth );

  UInt*         getWedgePredDirTabIdx        ()                      { return m_puiWedgePredDirTabIdx;        }
  UInt          getWedgePredDirTabIdx        ( UInt uiIdx )          { return m_puiWedgePredDirTabIdx[uiIdx]; }
  Void          setWedgePredDirTabIdx        ( UInt uiIdx, UInt uh ) { m_puiWedgePredDirTabIdx[uiIdx] = uh;   }
  Void          setWedgePredDirTabIdxSubParts( UInt uiTIdx, UInt uiAbsPartIdx, UInt uiDepth );

  Int*          getWedgePredDirDeltaDC1       ()                        { return m_piWedgePredDirDeltaDC1;             }
  Int           getWedgePredDirDeltaDC1       ( UInt uiIdx )            { return m_piWedgePredDirDeltaDC1[uiIdx];      }
  Void          setWedgePredDirDeltaDC1       ( UInt uiIdx, Int i )     { m_piWedgePredDirDeltaDC1[uiIdx] = i;         }
  Void          setWedgePredDirDeltaDC1SubParts( Int iDC1, UInt uiAbsPartIdx, UInt uiDepth );

  Int*          getWedgePredDirDeltaDC2       ()                        { return m_piWedgePredDirDeltaDC2;             }
  Int           getWedgePredDirDeltaDC2       ( UInt uiIdx )            { return m_piWedgePredDirDeltaDC2[uiIdx];      }
  Void          setWedgePredDirDeltaDC2       ( UInt uiIdx, Int i )     { m_piWedgePredDirDeltaDC2[uiIdx] = i;         }
  Void          setWedgePredDirDeltaDC2SubParts( Int iDC2, UInt uiAbsPartIdx, UInt uiDepth );

  Int*          getWedgePredDirDeltaEnd        ()                     { return m_piWedgePredDirDeltaEnd;        }
  Int           getWedgePredDirDeltaEnd        ( UInt uiIdx )         { return m_piWedgePredDirDeltaEnd[uiIdx]; }
  Void          setWedgePredDirDeltaEnd        ( UInt uiIdx, Int iD ) { m_piWedgePredDirDeltaEnd[uiIdx] = iD;   }
  Void          setWedgePredDirDeltaEndSubParts( Int iDelta, UInt uiAbsPartIdx, UInt uiDepth );
#endif
#if HHI_DMM_PRED_TEX
  UInt*         getWedgePredTexTabIdx       ()                       { return m_puiWedgePredTexTabIdx;           }
  UInt          getWedgePredTexTabIdx       ( UInt uiIdx )           { return m_puiWedgePredTexTabIdx[uiIdx];    }
  Void          setWedgePredTexTabIdx       ( UInt uiIdx, UInt uh )  { m_puiWedgePredTexTabIdx[uiIdx] = uh;      }
  Void          setWedgePredTexTabIdxSubParts( UInt uiTIdx, UInt uiAbsPartIdx, UInt uiDepth );

  Int*          getWedgePredTexDeltaDC1       ()                     { return m_piWedgePredTexDeltaDC1;          }
  Int           getWedgePredTexDeltaDC1       ( UInt uiIdx )         { return m_piWedgePredTexDeltaDC1[uiIdx];   }
  Void          setWedgePredTexDeltaDC1       ( UInt uiIdx, Int i )  { m_piWedgePredTexDeltaDC1[uiIdx] = i;      }
  Void          setWedgePredTexDeltaDC1SubParts( Int iDC1, UInt uiAbsPartIdx, UInt uiDepth );

  Int*          getWedgePredTexDeltaDC2       ()                     { return m_piWedgePredTexDeltaDC2;          }
  Int           getWedgePredTexDeltaDC2       ( UInt uiIdx )         { return m_piWedgePredTexDeltaDC2[uiIdx];   }
  Void          setWedgePredTexDeltaDC2       ( UInt uiIdx, Int i )  { m_piWedgePredTexDeltaDC2[uiIdx] = i;      }
  Void          setWedgePredTexDeltaDC2SubParts( Int iDC2, UInt uiAbsPartIdx, UInt uiDepth );

  Int*          getContourPredTexDeltaDC1       ()                     { return m_piContourPredTexDeltaDC1;          }
  Int           getContourPredTexDeltaDC1       ( UInt uiIdx )         { return m_piContourPredTexDeltaDC1[uiIdx];   }
  Void          setContourPredTexDeltaDC1       ( UInt uiIdx, Int i )  { m_piContourPredTexDeltaDC1[uiIdx] = i;      }
  Void          setContourPredTexDeltaDC1SubParts( Int iDC1, UInt uiAbsPartIdx, UInt uiDepth );

  Int*          getContourPredTexDeltaDC2       ()                     { return m_piContourPredTexDeltaDC2;          }
  Int           getContourPredTexDeltaDC2       ( UInt uiIdx )         { return m_piContourPredTexDeltaDC2[uiIdx];   }
  Void          setContourPredTexDeltaDC2       ( UInt uiIdx, Int i )  { m_piContourPredTexDeltaDC2[uiIdx] = i;      }
  Void          setContourPredTexDeltaDC2SubParts( Int iDC2, UInt uiAbsPartIdx, UInt uiDepth );
#endif

#if HHI_INTER_VIEW_MOTION_PRED
  Int           getPdmMergeCandidate( UInt uiPartIdx, Int* paiPdmRefIdx, TComMv* pacPdmMv );
  Bool          getPdmMvPred( UInt uiPartIdx, RefPicList eRefPicList, Int iRefIdx, TComMv& rcMv, Bool bMerge = false );
  Bool          getIViewOrgDepthMvPred( UInt uiPartIdx, RefPicList eRefPicList, Int iRefIdx, TComMv& rcMv );
#endif
#if HHI_INTER_VIEW_RESIDUAL_PRED
  Bool          getResidualSamples( UInt uiPartIdx, TComYuv* pcYuv = 0 );
#endif
  
  // -------------------------------------------------------------------------------------------------------------------
  // member functions for accessing partition information
  // -------------------------------------------------------------------------------------------------------------------
  
  Void          getPartIndexAndSize   ( UInt uiPartIdx, UInt& ruiPartAddr, Int& riWidth, Int& riHeight );
  UChar         getNumPartInter       ();
  Bool          isFirstAbsZorderIdxInDepth (UInt uiAbsPartIdx, UInt uiDepth);
  
  // -------------------------------------------------------------------------------------------------------------------
  // member functions for motion vector
  // -------------------------------------------------------------------------------------------------------------------
  
  Void          getMvField            ( TComDataCU* pcCU, UInt uiAbsPartIdx, RefPicList eRefPicList, TComMvField& rcMvField );
  
  AMVP_MODE     getAMVPMode           ( UInt uiIdx );
  Void          fillMvpCand           ( UInt uiPartIdx, UInt uiPartAddr, RefPicList eRefPicList, Int iRefIdx, AMVPInfo* pInfo );
#if DCM_SIMPLIFIED_MVP==0
  Bool          clearMVPCand          ( TComMv cMvd, AMVPInfo* pInfo );
#endif
  Int           searchMVPIdx          ( TComMv cMv,  AMVPInfo* pInfo );

  Void          setMVPIdx             ( RefPicList eRefPicList, UInt uiIdx, Int iMVPIdx)  { m_apiMVPIdx[eRefPicList][uiIdx] = iMVPIdx;  }
  Int           getMVPIdx             ( RefPicList eRefPicList, UInt uiIdx)               { return m_apiMVPIdx[eRefPicList][uiIdx];     }
  Int*          getMVPIdx             ( RefPicList eRefPicList )                          { return m_apiMVPIdx[eRefPicList];            }
  
  Void          setMVPNum             ( RefPicList eRefPicList, UInt uiIdx, Int iMVPNum ) { m_apiMVPNum[eRefPicList][uiIdx] = iMVPNum;  }
  Int           getMVPNum             ( RefPicList eRefPicList, UInt uiIdx )              { return m_apiMVPNum[eRefPicList][uiIdx];     }
  Int*          getMVPNum             ( RefPicList eRefPicList )                          { return m_apiMVPNum[eRefPicList];            }
  
  Void          setMVPIdxSubParts     ( Int iMVPIdx, RefPicList eRefPicList, UInt uiAbsPartIdx, UInt uiPartIdx, UInt uiDepth );
  Void          setMVPNumSubParts     ( Int iMVPNum, RefPicList eRefPicList, UInt uiAbsPartIdx, UInt uiPartIdx, UInt uiDepth );
  
  Void          clipMv                ( TComMv&     rcMv     );
  Void          getMvPredLeft         ( TComMv&     rcMvPred )   { rcMvPred = m_cMvFieldA.getMv(); }
  Void          getMvPredAbove        ( TComMv&     rcMvPred )   { rcMvPred = m_cMvFieldB.getMv(); }
  Void          getMvPredAboveRight   ( TComMv&     rcMvPred )   { rcMvPred = m_cMvFieldC.getMv(); }
  
#if AMVP_BUFFERCOMPRESS
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
  
  TComDataCU*   getPULeft                   ( UInt&  uiLPartUnitIdx , UInt uiCurrPartUnitIdx, Bool bEnforceSliceRestriction=true, Bool bEnforceEntropySliceRestriction=true );
  TComDataCU*   getPUAbove                  ( UInt&  uiAPartUnitIdx , UInt uiCurrPartUnitIdx, Bool bEnforceSliceRestriction=true, Bool bEnforceEntropySliceRestriction=true );
  TComDataCU*   getPUAboveLeft              ( UInt&  uiALPartUnitIdx, UInt uiCurrPartUnitIdx, Bool bEnforceSliceRestriction=true, Bool bEnforceEntropySliceRestriction=true );
  TComDataCU*   getPUAboveRight             ( UInt&  uiARPartUnitIdx, UInt uiCurrPartUnitIdx, Bool bEnforceSliceRestriction=true, Bool bEnforceEntropySliceRestriction=true );
  TComDataCU*   getPUBelowLeft              ( UInt& uiBLPartUnitIdx, UInt uiCurrPartUnitIdx, Bool bEnforceSliceRestriction=true, Bool bEnforceEntropySliceRestriction=true );

#if CONSTRAINED_INTRA_PRED
  TComDataCU*   getPUAboveRightAdi          ( UInt&  uiARPartUnitIdx, UInt uiPuWidth, UInt uiCurrPartUnitIdx, UInt uiPartUnitOffset = 1, Bool bEnforceSliceRestriction=true, Bool bEnforceEntropySliceRestriction=true );
  TComDataCU*   getPUBelowLeftAdi           ( UInt& uiBLPartUnitIdx, UInt uiPuHeight, UInt uiCurrPartUnitIdx, UInt uiPartUnitOffset = 1, Bool bEnforceSliceRestriction=true, Bool bEnforceEntropySliceRestriction=true );
#else
  TComDataCU*   getPUAboveRightAdi          ( UInt&  uiARPartUnitIdx, UInt uiPuWidth, UInt uiCurrPartUnitIdx, Bool bEnforceSliceRestriction=true, Bool bEnforceEntropySliceRestriction=true );
  TComDataCU*   getPUBelowLeftAdi           ( UInt& uiBLPartUnitIdx, UInt uiPuHeight, UInt uiCurrPartUnitIdx, Bool bEnforceSliceRestriction=true, Bool bEnforceEntropySliceRestriction=true );
#endif
  
  Void          deriveLeftRightTopIdx       ( PartSize eCUMode, UInt uiPartIdx, UInt& ruiPartIdxLT, UInt& ruiPartIdxRT );
  Void          deriveLeftBottomIdx         ( PartSize eCUMode, UInt uiPartIdx, UInt& ruiPartIdxLB );
  
  Void          deriveLeftRightTopIdxAdi    ( UInt& ruiPartIdxLT, UInt& ruiPartIdxRT, UInt uiPartOffset, UInt uiPartDepth );
  Void          deriveLeftBottomIdxAdi      ( UInt& ruiPartIdxLB, UInt  uiPartOffset, UInt uiPartDepth );
  
  Bool          hasEqualMotion              ( UInt uiAbsPartIdx, TComDataCU* pcCandCU, UInt uiCandAbsPartIdx );
  Bool          avoidMergeCandidate         ( UInt uiAbsPartIdx, UInt uiPUIdx, UInt uiDepth, TComDataCU* pcCandCU, UInt uiCandAbsPartIdx );
  Bool          hasEqualMotion              ( UInt uiAbsPartIdx, UInt uiCandInterDir, Int* paiCandRefIdx, TComMv* pacCandMv );
  Bool          avoidMergeCandidate         ( UInt uiAbsPartIdx, UInt uiPUIdx, UInt uiDepth, UInt uiCandInterDir, Int* paiCandRefIdx, TComMv* pacCandMv );
  Void          getInterMergeCandidates       ( UInt uiAbsPartIdx, UInt uiPUIdx, UInt uiDepth, TComMvField* pcMFieldNeighbours, UChar* puhInterDirNeighbours, UInt* puiNeighbourCandIdx );
  Void          deriveLeftRightTopIdxGeneral  ( PartSize eCUMode, UInt uiAbsPartIdx, UInt uiPartIdx, UInt& ruiPartIdxLT, UInt& ruiPartIdxRT );
  Void          deriveLeftBottomIdxGeneral    ( PartSize eCUMode, UInt uiAbsPartIdx, UInt uiPartIdx, UInt& ruiPartIdxLB );
  
  
  // -------------------------------------------------------------------------------------------------------------------
  // member functions for modes
  // -------------------------------------------------------------------------------------------------------------------
  
  Bool          isIntra   ( UInt uiPartIdx )  { return m_pePredMode[ uiPartIdx ] == MODE_INTRA; }
  Bool          isSkipped ( UInt uiPartIdx );                                                     ///< SKIP (no residual)
#if POZNAN_CU_SKIP
  Bool          isCUSkiped( UInt uiPartIdx )  { return m_pePredMode[ uiPartIdx ] == MODE_SYNTH; }
#endif
  
  // -------------------------------------------------------------------------------------------------------------------
  // member functions for symbol prediction (most probable / mode conversion)
  // -------------------------------------------------------------------------------------------------------------------
  
  Int           getMostProbableIntraDirLuma     ( UInt uiAbsPartIdx                                       );
  
  UInt          getIntraSizeIdx                 ( UInt uiAbsPartIdx                                       );
  Void          convertTransIdx                 ( UInt uiAbsPartIdx, UInt uiTrIdx, UInt& ruiLumaTrMode, UInt& ruiChromaTrMode );
  
#if LCEC_INTRA_MODE
  Int           getLeftIntraDirLuma             ( UInt uiAbsPartIdx );
  Int           getAboveIntraDirLuma            ( UInt uiAbsPartIdx );
#endif

#if MTK_DCM_MPM
  Int           getIntraDirLumaPredictor        ( UInt uiAbsPartIdx, Int uiIntraDirPred[]                 );
#endif

#if MS_LCEC_LOOKUP_TABLE_EXCEPTION
  Bool          isSuroundingRefIdxException     ( UInt   uiAbsPartIdx );
#endif
  
  // -------------------------------------------------------------------------------------------------------------------
  // member functions for SBAC context
  // -------------------------------------------------------------------------------------------------------------------
  
  UInt          getCtxSplitFlag                 ( UInt   uiAbsPartIdx, UInt uiDepth                   );
  UInt          getCtxCbf                       ( UInt   uiAbsPartIdx, TextType eType, UInt uiTrDepth );
  UInt          getCtxQtCbf                     ( UInt   uiAbsPartIdx, TextType eType, UInt uiTrDepth );
  UInt          getCtxQtRootCbf                 ( UInt   uiAbsPartIdx                                 );
  UInt          getCtxRefIdx                    ( UInt   uiAbsPartIdx, RefPicList eRefPicList         );
  UInt          getCtxSkipFlag                  ( UInt   uiAbsPartIdx                                 );
  UInt          getCtxAlfCtrlFlag               ( UInt   uiAbsPartIdx                                 );
  UInt          getCtxInterDir                  ( UInt   uiAbsPartIdx                                 );
  UInt          getCtxIntraDirChroma            ( UInt   uiAbsPartIdx                                 );
  UInt          getCtxMergeFlag                 ( UInt uiAbsPartIdx                                   );
  
  Void          setSliceStartCU  ( UInt uiStartCU )    { m_uiSliceStartCU = uiStartCU;    }  
  UInt          getSliceStartCU  ()                    { return m_uiSliceStartCU;         }
  Void          setEntropySliceStartCU ( UInt uiStartCU ) { m_uiEntropySliceStartCU = uiStartCU;     }  
  UInt          getEntropySliceStartCU ()                 { return m_uiEntropySliceStartCU;          }

  // -------------------------------------------------------------------------------------------------------------------
  // member functions for RD cost storage
  // -------------------------------------------------------------------------------------------------------------------
  
  Double&       getTotalCost()                  { return m_dTotalCost;        }
  Dist&         getTotalDistortion()            { return m_uiTotalDistortion; }
  UInt&         getTotalBits()                  { return m_uiTotalBits;       }
  UInt&         getTotalNumPart()               { return m_uiNumPartition;    }

#if QC_MDCS
  UInt          getCoefScanIdx(UInt uiAbsPartIdx, UInt uiWidth, Bool bIsLuma, Bool bIsIntra);
#endif //QC_MDCS

#if POZNAN_TEXTURE_TU_DELTA_QP_ACCORDING_TO_DEPTH
 Int            CuQpIncrementFunction  ( Pel uiBlockMax );
 Int            getQpOffsetForTextCU   ( UInt uiPartIdx, Bool bIsIntra );
 Pel            getDepthLumaCodingBlockMedian    ( TComPicYuv * pcDepthPicYUV/*TComDataCU* rpcCUDepth*/, UInt iCuAddr, UInt uiPartIdx );
 Pel            getDepthLumaCodingBlockMax       ( TComPicYuv * pcDepthPicYUV/*TComDataCU* rpcCUDepth*/, UInt iCuAddr, UInt uiPartIdx );
 Void           sortDepthLumaCodingBlock         ( TComPicYuv * pcDepthPicYUV/*TComDataCU* rpcCUDepth*/, UInt iCuAddr, UInt uiPartIdx, Pel * pSortTable, Int& TUWidth, Int& TUHeight);
 Pel            maxDepthLumaCodingBlock          ( TComPicYuv * pcDepthPicYUV/*TComDataCU* rpcCUDepth*/, UInt iCuAddr, UInt uiPartIdx );
 Pel            getDepthLumaTransformBlockMedian ( TComPicYuv * pcDepthPicYUV/*TComDataCU* rpcCUDepth*/, UInt iCuAddr, UInt uiPartIdx );
 Pel            getDepthLumaTransformBlockMax    ( TComPicYuv * pcDepthPicYUV/*TComDataCU* rpcCUDepth*/, UInt iCuAddr, UInt uiPartIdx );
 Void           sortDepthLumaTransformBlock      ( TComPicYuv * pcDepthPicYUV/*TComDataCU* rpcCUDepth*/, UInt iCuAddr, UInt uiPartIdx , Pel * pSortTable, Int& TUWidth, Int& TUHeight);
 Pel            maxDepthLumaTransformBlock       ( TComPicYuv * pcDepthPicYUV/*TComDataCU* rpcCUDepth*/, UInt iCuAddr, UInt uiPartIdx );
 Pel            getDepthLumaPredictionBlockMedian( TComPicYuv * pcDepthPicYUV/*TComDataCU* rpcCUDepth*/, UInt iCuAddr, UInt uiPartIdx );
 Pel            getDepthLumaPredictionBlockMax   ( TComPicYuv * pcDepthPicYUV/*TComDataCU* rpcCUDepth*/, UInt iCuAddr, UInt uiPartIdx );
 Void           sortDepthLumaPredictionBlock     ( TComPicYuv * pcDepthPicYUV/*TComDataCU* rpcCUDepth*/, UInt iCuAddr, UInt uiPartIdx , Pel * pSortTable, Int& PUWidth, Int& PUHeight);
#endif
};

#endif

