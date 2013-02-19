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

#if HHI_INTER_VIEW_RESIDUAL_PRED
#include "TComYuv.h"
#endif

#include <algorithm>
#include <vector>

//! \ingroup TLibCommon
//! \{

// ====================================================================================================================
// Non-deblocking in-loop filter processing block data structure
// ====================================================================================================================

/// Non-deblocking filter processing block border tag
enum NDBFBlockBorderTag
{
  SGU_L = 0,
  SGU_R,
  SGU_T,
  SGU_B,
  SGU_TL,
  SGU_TR,
  SGU_BL,
  SGU_BR,
  NUM_SGU_BORDER
};

/// Non-deblocking filter processing block information
struct NDBFBlockInfo
{
  Int   tileID;   //!< tile ID
  Int   sliceID;  //!< slice ID
  UInt  startSU;  //!< starting SU z-scan address in LCU
  UInt  endSU;    //!< ending SU z-scan address in LCU
  UInt  widthSU;  //!< number of SUs in width
  UInt  heightSU; //!< number of SUs in height
  UInt  posX;     //!< top-left X coordinate in picture
  UInt  posY;     //!< top-left Y coordinate in picture
  UInt  width;    //!< number of pixels in width
  UInt  height;   //!< number of pixels in height
  Bool  isBorderAvailable[NUM_SGU_BORDER];  //!< the border availabilities
#if LCU_SYNTAX_ALF
  Bool  allBordersAvailable;
#endif

  NDBFBlockInfo():tileID(0), sliceID(0), startSU(0), endSU(0) {} //!< constructor
  const NDBFBlockInfo& operator= (const NDBFBlockInfo& src);  //!< "=" operator
};


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
#if HHI_MPI
  Int*          m_piTextureModeDepth; ///< at which depth are prediction data inherited from texture picture ( -1 : none )
#endif
  
  // -------------------------------------------------------------------------------------------------------------------
  // CU data
  // -------------------------------------------------------------------------------------------------------------------
  
  Char*         m_pePartSize;         ///< array of partition sizes
#if HHI_INTERVIEW_SKIP
  Bool*         m_pbRenderable;        ///< array of merge flags
#endif
  Char*         m_pePredMode;         ///< array of prediction modes
#if H0736_AVC_STYLE_QP_RANGE
  Char*         m_phQP;               ///< array of QP values
#else
  UChar*        m_phQP;               ///< array of QP values
#endif
  UChar*        m_puhTrIdx;           ///< array of transform indices
  UChar*        m_nsqtPartIdx;        ///< array of absPartIdx mapping table, map zigzag to NSQT
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

  Int*          m_piSliceSUMap;       ///< pointer of slice ID map
  std::vector<NDBFBlockInfo> m_vNDFBlock;

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
#if LGE_ILLUCOMP_B0045
  Bool*         m_pbICFlag;           ///< array of IC flags
#endif
  UChar*        m_puhMergeIndex;      ///< array of merge candidate indices
#if MERL_VSP_C0152
  Char*         m_piVSPIndex;         ///< array of VSP flags to indicate the current block uses synthetic predictor or not
                                      ///< value 0: non-VSP, value 1: VSP
#endif
#if AMP_MRG
  Bool          m_bIsMergeAMP;
#endif
  UChar*        m_puhLumaIntraDir;    ///< array of intra directions (luma)
  UChar*        m_puhChromaIntraDir;  ///< array of intra directions (chroma)
  UChar*        m_puhInterDir;        ///< array of inter directions
  Char*         m_apiMVPIdx[2];       ///< array of motion vector predictor candidates
  Char*         m_apiMVPNum[2];       ///< array of number of possible motion vectors predictors
  Bool*         m_puiAlfCtrlFlag;     ///< array of ALF flags
  Bool*         m_puiTmpAlfCtrlFlag;  ///< temporal array of ALF flags
  
  Bool*         m_pbIPCMFlag;         ///< array of intra_pcm flags

#if BURST_IPCM
  Int           m_numSucIPCM;         ///< the number of succesive IPCM blocks associated with the current log2CUSize
  Bool          m_lastCUSucIPCMFlag;  ///< True indicates that the last CU is IPCM and shares the same root as the current CU.  
#endif
#if HHI_INTER_VIEW_RESIDUAL_PRED
  Bool*         m_pbResPredAvailable; ///< array of residual prediction available flags
  Bool*         m_pbResPredFlag;      ///< array of residual prediction flags
#endif

#if LGE_EDGE_INTRA_A0070
  UChar*        m_pucEdgeCode;          ///< array of edge code
  UChar*        m_pucEdgeNumber;        ///< total number of edge
  UChar*        m_pucEdgeStartPos;      ///< starting point position
  Bool*         m_pbEdgeLeftFirst;      ///< true if edge should be checked in left boundary first
  Bool*         m_pbEdgePartition;      ///< true if it belongs to region 1, otherwise, region 0
#if LGE_EDGE_INTRA_DELTA_DC
  Int*          m_piEdgeDeltaDC0;
  Int*          m_piEdgeDeltaDC1;
#endif
#endif

  // -------------------------------------------------------------------------------------------------------------------
  // misc. variables
  // -------------------------------------------------------------------------------------------------------------------
  
  Bool          m_bDecSubCu;          ///< indicates decoder-mode
  Double        m_dTotalCost;         ///< sum of partition RD costs
  Dist          m_uiTotalDistortion;  ///< sum of partition distortion
  UInt          m_uiTotalBits;        ///< sum of partition bits
  UInt          m_uiTotalBins;       ///< sum of partition bins
  UInt*         m_uiSliceStartCU;    ///< Start CU address of current slice
  UInt*         m_uiEntropySliceStartCU; ///< Start CU address of current slice
  
  // -------------------------------------------------------------------------------------------------------------------
  // depth model mode data
  // -------------------------------------------------------------------------------------------------------------------
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
#if LGE_DMM3_SIMP_C0044
  UInt*         m_puiWedgePredTexIntraTabIdx;
#endif
  Int*          m_piWedgePredTexDeltaDC1;
  Int*          m_piWedgePredTexDeltaDC2;

  Int*          m_piContourPredTexDeltaDC1;
  Int*          m_piContourPredTexDeltaDC2;
#endif
  
#if RWTH_SDC_DLT_B0036
  Bool*         m_pbSDCFlag;
  Pel*          m_apSegmentDCOffset[2];
#endif

protected:
  
  /// add possible motion vector predictor candidates
  Bool          xAddMVPCand           ( AMVPInfo* pInfo, RefPicList eRefPicList, Int iRefIdx, UInt uiPartUnitIdx, MVP_DIR eDir );
  Bool          xAddMVPCandOrder      ( AMVPInfo* pInfo, RefPicList eRefPicList, Int iRefIdx, UInt uiPartUnitIdx, MVP_DIR eDir );
#if MERL_VSP_C0152
  inline Bool   xAddVspMergeCand      ( UChar ucVspMergePos, Int vspIdx, Bool* bVspMvZeroDone, UInt uiDepth, Bool* abCandIsInter, Int& iCount,
                                        UChar* puhInterDirNeighbours, TComMvField* pcMvFieldNeighbours, Int* iVSPIndexTrue, Int mrgCandIdx, DisInfo* pDisInfo );
  inline Void   xInheritVspMode       ( TComDataCU* pcCURef, UInt uiIdx, Bool* bVspMvZeroDone, Int iCount, Int* iVSPIndexTrue, TComMvField* pcMvFieldNeighbours, DisInfo* pDInfo ) ;
#endif
  Void          deriveRightBottomIdx        ( PartSize eCUMode, UInt uiPartIdx, UInt& ruiPartIdxRB );
  Bool          xGetColMVP( RefPicList eRefPicList, Int uiCUAddr, Int uiPartUnitIdx, TComMv& rcMv, Int& riRefIdx 
#if QC_TMVP_MRG_REFIDX_C0047
  ,
  Bool bMRG = 0
#endif
  );
#if H3D_NBDV
  Bool          xGetColDisMV( RefPicList eRefPicList, Int refidx, Int uiCUAddr, Int uiPartUnitIdx, TComMv& rcMv, Int & iTargetViewIdx, Int & iStartViewIdx );
#endif 
  
#if !AMVP_PRUNING_SIMPLIFICATION
  /// remove redundant candidates
  Void          xUniqueMVPCand        ( AMVPInfo* pInfo );
#endif

  Void xCheckCornerCand( TComDataCU* pcCorner, UInt uiCornerIdx, UInt uiIter, Bool& rbValidCand );
  /// compute required bits to encode MVD (used in AMVP)
  UInt          xGetMvdBits           ( TComMv cMvd );
  UInt          xGetComponentBits     ( Int iVal );
  
  /// compute scaling factor from POC difference
  Int           xGetDistScaleFactor   ( Int iCurrPOC, Int iCurrRefPOC, Int iColPOC, Int iColRefPOC );
  
  Void xDeriveCenterIdx( PartSize eCUMode, UInt uiPartIdx, UInt& ruiPartIdxCenter );
  Bool xGetCenterCol( UInt uiPartIdx, RefPicList eRefPicList, int iRefIdx, TComMv *pcMv );
  
  Void xCheckDuplicateCand(TComMvField* pcMvFieldNeighbours, UChar* puhInterDirNeighbours, bool* pbCandIsInter, UInt& ruiArrayAddr);

#if !BURST_IPCM
  Int           getLastValidPartIdx   ( Int iAbsPartIdx );
#endif

public:
  TComDataCU();
  virtual ~TComDataCU();
  
  // -------------------------------------------------------------------------------------------------------------------
  // create / destroy / initialize / copy
  // -------------------------------------------------------------------------------------------------------------------
  
  Void          create                ( UInt uiNumPartition, UInt uiWidth, UInt uiHeight, Bool bDecSubCu, Int unitSize
#if ADAPTIVE_QP_SELECTION
    , Bool bGlobalRMARLBuffer = false
#endif  
    );
  Void          destroy               ();
  
  Void          initCU                ( TComPic* pcPic, UInt uiCUAddr );
#if H0736_AVC_STYLE_QP_RANGE
  Void          initEstData           ( UInt uiDepth, Int qp );
  Void          initSubCU             ( TComDataCU* pcCU, UInt uiPartUnitIdx, UInt uiDepth, Int qp );
#else
  Void          initEstData           ( UInt uiDepth, UInt uiQP );
  Void          initSubCU             ( TComDataCU* pcCU, UInt uiPartUnitIdx, UInt uiDepth, UInt uiQP );
#endif
  Void          setOutsideCUPart      ( UInt uiAbsPartIdx, UInt uiDepth );

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
  UInt          getSCUAddr            ();
  UInt          getCUPelX             ()                        { return m_uiCUPelX;        }
  UInt          getCUPelY             ()                        { return m_uiCUPelY;        }
  TComPattern*  getPattern            ()                        { return m_pcPattern;       }
  
  UChar*        getDepth              ()                        { return m_puhDepth;        }
  UChar         getDepth              ( UInt uiIdx )            { return m_puhDepth[uiIdx]; }
  Void          setDepth              ( UInt uiIdx, UChar  uh ) { m_puhDepth[uiIdx] = uh;   }
  
  Void          setDepthSubParts      ( UInt uiDepth, UInt uiAbsPartIdx );
  Void          getPosInPic           ( UInt uiAbsPartIndex, Int& riPosX, Int& riPosY );
#if HHI_MPI
  Int*          getTextureModeDepth   ()                        { return m_piTextureModeDepth; }
  Int           getTextureModeDepth   ( UInt uiIdx )            { return m_piTextureModeDepth[uiIdx]; }
  Void          setTextureModeDepth   ( UInt uiIdx, Int iTextureModeDepth ){ m_piTextureModeDepth[uiIdx] = iTextureModeDepth; }
  Void          setTextureModeDepthSubParts( Int iTextureModeDepth, UInt uiAbsPartIdx, UInt uiDepth );
  Void          copyTextureMotionDataFrom( TComDataCU* pcCU, UInt uiDepth, UInt uiAbsPartIdxSrc, UInt uiAbsPartIdxDst = 0 );
#endif

   // -------------------------------------------------------------------------------------------------------------------
  // member functions for CU data
  // -------------------------------------------------------------------------------------------------------------------
  
  Char*         getPartitionSize      ()                        { return m_pePartSize;        }
  PartSize      getPartitionSize      ( UInt uiIdx )            { return static_cast<PartSize>( m_pePartSize[uiIdx] ); }
  Void          setPartitionSize      ( UInt uiIdx, PartSize uh){ m_pePartSize[uiIdx] = uh;   }
  Void          setPartSizeSubParts   ( PartSize eMode, UInt uiAbsPartIdx, UInt uiDepth );
  
#if HHI_INTERVIEW_SKIP
  Bool*         getRenderable          ()                        { return m_pbRenderable;               }
    Bool          getRenderable          ( UInt uiIdx )            { return m_pbRenderable[uiIdx];        }
    Void          setRenderable          ( UInt uiIdx, Bool b )    { m_pbRenderable[uiIdx] = b;           }
    Void          setRenderableSubParts  ( Bool bRenderable, UInt uiAbsPartIdx, UInt uiDepth );
#endif
  
  Char*         getPredictionMode     ()                        { return m_pePredMode;        }
  PredMode      getPredictionMode     ( UInt uiIdx )            { return static_cast<PredMode>( m_pePredMode[uiIdx] ); }
  Void          setPredictionMode     ( UInt uiIdx, PredMode uh){ m_pePredMode[uiIdx] = uh;   }
  Void          setPredModeSubParts   ( PredMode eMode, UInt uiAbsPartIdx, UInt uiDepth );
  
  UChar*        getWidth              ()                        { return m_puhWidth;          }
  UChar         getWidth              ( UInt uiIdx )            { return m_puhWidth[uiIdx];   }
  Void          setWidth              ( UInt uiIdx, UChar  uh ) { m_puhWidth[uiIdx] = uh;     }
  
  UChar*        getHeight             ()                        { return m_puhHeight;         }
  UChar         getHeight             ( UInt uiIdx )            { return m_puhHeight[uiIdx];  }
  Void          setHeight             ( UInt uiIdx, UChar  uh ) { m_puhHeight[uiIdx] = uh;    }
  
  Void          setSizeSubParts       ( UInt uiWidth, UInt uiHeight, UInt uiAbsPartIdx, UInt uiDepth );
  
#if H0736_AVC_STYLE_QP_RANGE
  Char*         getQP                 ()                        { return m_phQP;              }
  Char          getQP                 ( UInt uiIdx )            { return m_phQP[uiIdx];       }
  Void          setQP                 ( UInt uiIdx, Char value ){ m_phQP[uiIdx] =  value;     }
  Void          setQPSubParts         ( Int qp,   UInt uiAbsPartIdx, UInt uiDepth );
#if BURST_IPCM
  Int           getLastValidPartIdx   ( Int iAbsPartIdx );
#endif
  Char          getLastCodedQP        ( UInt uiAbsPartIdx );
#else
  UChar*        getQP                 ()                        { return m_phQP;              }
  UChar         getQP                 ( UInt uiIdx )            { return m_phQP[uiIdx];       }
  Void          setQP                 ( UInt uiIdx, UChar  uh ) { m_phQP[uiIdx] = uh;         }
  Void          setQPSubParts         ( UInt uiQP,   UInt uiAbsPartIdx, UInt uiDepth );
#if BURST_IPCM
  Int           getLastValidPartIdx   ( Int iAbsPartIdx );
#endif
  UChar         getLastCodedQP        ( UInt uiAbsPartIdx );
#endif

#if LOSSLESS_CODING
  Bool          isLosslessCoded(UInt absPartIdx);
#endif
  UChar*        getNSQTPartIdx        ()                        { return m_nsqtPartIdx;        }
  UChar         getNSQTPartIdx        ( UInt idx )              { return m_nsqtPartIdx[idx];   }
  Void          setNSQTIdxSubParts    ( UInt absPartIdx, UInt depth );
  Void          setNSQTIdxSubParts    ( UInt log2TrafoSize, UInt absPartIdx, UInt absTUPartIdx, UInt trMode );
  
  UChar*        getTransformIdx       ()                        { return m_puhTrIdx;          }
  UChar         getTransformIdx       ( UInt uiIdx )            { return m_puhTrIdx[uiIdx];   }
  Void          setTrIdxSubParts      ( UInt uiTrIdx, UInt uiAbsPartIdx, UInt uiDepth );
  
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

  UChar         getCbf    ( UInt uiIdx, TextType eType )                  { return m_puhCbf[g_aucConvertTxtTypeToIdx[eType]][uiIdx];  }
  UChar*        getCbf    ( TextType eType )                              { return m_puhCbf[g_aucConvertTxtTypeToIdx[eType]];         }
  UChar         getCbf    ( UInt uiIdx, TextType eType, UInt uiTrDepth )  { return ( ( getCbf( uiIdx, eType ) >> uiTrDepth ) & 0x1 ); }
  Void          setCbf    ( UInt uiIdx, TextType eType, UChar uh )        { m_puhCbf[g_aucConvertTxtTypeToIdx[eType]][uiIdx] = uh;    }
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

#if MERL_VSP_C0152
  Char*         getVSPIndex        ()                        { return m_piVSPIndex;        }
  Char          getVSPIndex        ( UInt uiIdx )            { return m_piVSPIndex[uiIdx]; }
  Void          setVSPIndex        ( UInt uiIdx, Int n )     { m_piVSPIndex[uiIdx] = n;    }
  Void          setVSPIndexSubParts( Char bVSPIndex, UInt uiAbsPartIdx, UInt uiPartIdx, UInt uiDepth );
#endif

  template <typename T>
  Void          setSubPart            ( T bParameter, T* pbBaseLCU, UInt uiCUAddr, UInt uiCUDepth, UInt uiPUIdx );

#if LGE_ILLUCOMP_B0045
  Bool*         getICFlag             ()                        { return m_pbICFlag;               }
  Bool          getICFlag             ( UInt uiIdx )            { return m_pbICFlag[uiIdx];        }
  Void          setICFlag             ( UInt uiIdx, Bool  uh )  { m_pbICFlag[uiIdx] = uh;          }
  Void          setICFlagSubParts     ( Bool bICFlag,  UInt uiAbsPartIdx, UInt uiPartIdx, UInt uiDepth );
#if LGE_ILLUCOMP_DEPTH_C0046
  Bool          isICFlagRequired      (UInt uiAbsPartIdx, UInt uiDepth); //This modification is not needed after integrating JCT3V-C0137
#else
  Bool          isICFlagRequired      (UInt uiAbsPartIdx);
#endif
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
  
  Bool*         getAlfCtrlFlag        ()                        { return m_puiAlfCtrlFlag;            }
  Bool          getAlfCtrlFlag        ( UInt uiIdx )            { return m_puiAlfCtrlFlag[uiIdx];     }
  Void          setAlfCtrlFlag        ( UInt uiIdx, Bool uiFlag){ m_puiAlfCtrlFlag[uiIdx] = uiFlag;   }
  Void          setAlfCtrlFlagSubParts( Bool uiFlag, UInt uiAbsPartIdx, UInt uiDepth );
  
  Void          createTmpAlfCtrlFlag  ();
  Void          destroyTmpAlfCtrlFlag ();
  Void          copyAlfCtrlFlagToTmp  ();
  Void          copyAlfCtrlFlagFromTmp();
  
  Bool*         getIPCMFlag           ()                        { return m_pbIPCMFlag;               }
  Bool          getIPCMFlag           (UInt uiIdx )             { return m_pbIPCMFlag[uiIdx];        }
  Void          setIPCMFlag           (UInt uiIdx, Bool b )     { m_pbIPCMFlag[uiIdx] = b;           }
  Void          setIPCMFlagSubParts   (Bool bIpcmFlag, UInt uiAbsPartIdx, UInt uiDepth);

#if BURST_IPCM
  Int           getNumSucIPCM         ()                        { return m_numSucIPCM;             }
  Void          setNumSucIPCM         ( Int num )               { m_numSucIPCM = num;              }
  Bool          getLastCUSucIPCMFlag  ()                        { return m_lastCUSucIPCMFlag;        }
  Void          setLastCUSucIPCMFlag  ( Bool flg )              { m_lastCUSucIPCMFlag = flg;         }
#endif

  /// get slice ID for SU
  Int           getSUSliceID          (UInt uiIdx)              {return m_piSliceSUMap[uiIdx];      } 

  /// get the pointer of slice ID map
  Int*          getSliceSUMap         ()                        {return m_piSliceSUMap;             }

  /// set the pointer of slice ID map
  Void          setSliceSUMap         (Int *pi)                 {m_piSliceSUMap = pi;               }

  std::vector<NDBFBlockInfo>* getNDBFilterBlocks()      {return &m_vNDFBlock;}
  Void setNDBFilterBlockBorderAvailability(UInt numLCUInPicWidth, UInt numLCUInPicHeight, UInt numSUInLCUWidth, UInt numSUInLCUHeight, UInt picWidth, UInt picHeight
                                          ,Bool bIndependentSliceBoundaryEnabled
                                          ,Bool bTopTileBoundary, Bool bDownTileBoundary, Bool bLeftTileBoundary, Bool bRightTileBoundary
                                          ,Bool bIndependentTileBoundaryEnabled );

#if H3D_IVMP
#if !H3D_NBDV
  Int           getPdmMergeCandidate      ( UInt uiPartIdx, Int* paiPdmRefIdx, TComMv* pacPdmMv );
  Bool          getPdmMvPred              ( UInt uiPartIdx, RefPicList eRefPicList, Int iRefIdx, TComMv& rcMv, Bool bMerge = false );
#else //!H3D_NBDV
#if QC_AMVP_MRG_UNIFY_IVCAN_C0051
  Bool          getUnifiedMvPredCan       ( UInt uiPartIdx, RefPicList eRefPicList, Int iRefIdx, Int* paiPdmRefIdx, TComMv* pacPdmMv, DisInfo* pDInfo, Int* iPdm, Bool bMerge );
#else //QC_AMVP_MRG_UNIFY_IVCAN_C0051
  Bool          getPdmMvPredDisCan        ( UInt uiPartIdx, RefPicList eRefPicList, Int iRefIdx, TComMv& rcMv, DisInfo* pDInfo, Bool bMerge = false );
  Int           getPdmMergeCandidateDisCan( UInt uiPartIdx, Int* paiPdmRefIdx, TComMv* pacPdmMv, DisInfo* pDInfo, Int* iPdm );
#endif //QC_AMVP_MRG_UNIFY_IVCAN_C0051
  Void          getDisMvpCand        ( UInt uiPartIdx, UInt uiPartAddr, DisInfo* pDInfo );
  Void          getDisMvpCandNBDV( UInt uiPartIdx, UInt uiPartAddr, DisInfo* pDInfo , Bool bParMerg = false
#if MERL_VSP_C0152
                              , Bool bDepthRefine = false
#endif //MERL_VSP_C0152
    );
#endif // !H3D_NBDV



#if MERL_VSP_C0152
#if LGE_SIMP_DVP_REFINE_C0112
  Pel           getMcpFromDM(TComPicYuv* pcBaseViewDepthPicYuv, TComMv* mv, Int iBlkX, Int iBlkY, Int iWidth, Int iHeight, Int* aiShiftLUT, Int iShiftPrec, Bool bSimpleDvpRefine = false);
  Void          estimateDVFromDM(UInt uiPartIdx, TComPic* picDepth, UInt uiPartAddr, TComMv* cMvPred, Bool bSimpleDvpRefine = false);
#else
  Pel           getMcpFromDM(TComPicYuv* pcBaseViewDepthPicYuv, TComMv* mv, Int iBlkX, Int iBlkY, Int iWidth, Int iHeight, Int* aiShiftLUT, Int iShiftPrec);
  Void          estimateDVFromDM(UInt uiPartIdx, TComPic* picDepth, UInt uiPartAddr, TComMv* cMvPred);
#endif
#endif
  Bool          getIViewOrgDepthMvPred( UInt uiPartIdx, RefPicList eRefPicList, Int iRefIdx, TComMv& rcMv );
#endif //  H3D_IVMP 
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
#if HHI_INTER_VIEW_RESIDUAL_PRED
  Bool          getResidualSamples( UInt uiPartIdx, Bool bRecon, TComYuv* pcYuv = 0 );
#endif
  // -------------------------------------------------------------------------------------------------------------------
  // member functions for accessing partition information
  // -------------------------------------------------------------------------------------------------------------------
#if LG_RESTRICTEDRESPRED_M24766
  Void          getPartIndexAndSize( UInt uiPartIdx, UInt& ruiPartAddr, Int& riWidth, Int& riHeight, UInt uiAbsPartIdx = 0, Bool bLCU = false);
#else
  Void          getPartIndexAndSize   ( UInt uiPartIdx, UInt& ruiPartAddr, Int& riWidth, Int& riHeight );
#endif
#if LG_RESTRICTEDRESPRED_M24766 && !MTK_MDIVRP_C0138
  Int           getResiPredMode(UInt uiPartAddr);
  Void          getPUResiPredShift (Int *iPUPredResiShift, UInt uiAbsPartIndex);
#endif
  UChar         getNumPartInter       ();
  Bool          isFirstAbsZorderIdxInDepth (UInt uiAbsPartIdx, UInt uiDepth);
  
  // -------------------------------------------------------------------------------------------------------------------
  // member functions for motion vector
  // -------------------------------------------------------------------------------------------------------------------
  
  Void          getMvField            ( TComDataCU* pcCU, UInt uiAbsPartIdx, RefPicList eRefPicList, TComMvField& rcMvField );
  
  AMVP_MODE     getAMVPMode           ( UInt uiIdx );
#if H3D_IVMP
  Void          fillMvpCandBase       ( UInt uiPartIdx, UInt uiPartAddr, RefPicList eRefPicList, Int iRefIdx, AMVPInfo* pInfo );
  Void          fillMvpCand           ( UInt uiPartIdx, UInt uiPartAddr, RefPicList eRefPicList, Int iRefIdx, AMVPInfo* pInfo , Int iMVPIdx=-1);
#endif
#if PARALLEL_MERGE 
  Bool          isDiffMER             ( Int xN, Int yN, Int xP, Int yP);
  Void          getPartPosition       ( UInt partIdx, Int& xP, Int& yP, Int& nPSW, Int& nPSH);
#endif 
  Void          setMVPIdx             ( RefPicList eRefPicList, UInt uiIdx, Int iMVPIdx)  { m_apiMVPIdx[eRefPicList][uiIdx] = iMVPIdx;  }
  Int           getMVPIdx             ( RefPicList eRefPicList, UInt uiIdx)               { return m_apiMVPIdx[eRefPicList][uiIdx];     }
  Char*         getMVPIdx             ( RefPicList eRefPicList )                          { return m_apiMVPIdx[eRefPicList];            }

  Void          setMVPNum             ( RefPicList eRefPicList, UInt uiIdx, Int iMVPNum ) { m_apiMVPNum[eRefPicList][uiIdx] = iMVPNum;  }
  Int           getMVPNum             ( RefPicList eRefPicList, UInt uiIdx )              { return m_apiMVPNum[eRefPicList][uiIdx];     }
  Char*         getMVPNum             ( RefPicList eRefPicList )                          { return m_apiMVPNum[eRefPicList];            }
  
  Void          setMVPIdxSubParts     ( Int iMVPIdx, RefPicList eRefPicList, UInt uiAbsPartIdx, UInt uiPartIdx, UInt uiDepth );
  Void          setMVPNumSubParts     ( Int iMVPNum, RefPicList eRefPicList, UInt uiAbsPartIdx, UInt uiPartIdx, UInt uiDepth );
  
  Void          clipMv                ( TComMv&     rcMv     );
  Void          getMvPredLeft         ( TComMv&     rcMvPred )   { rcMvPred = m_cMvFieldA.getMv(); }
  Void          getMvPredAbove        ( TComMv&     rcMvPred )   { rcMvPred = m_cMvFieldB.getMv(); }
  Void          getMvPredAboveRight   ( TComMv&     rcMvPred )   { rcMvPred = m_cMvFieldC.getMv(); }
  
  Void          compressMV            ();
  
  // -------------------------------------------------------------------------------------------------------------------
  // utility functions for neighbouring information
  // -------------------------------------------------------------------------------------------------------------------
  
  TComDataCU*   getCULeft                   () { return m_pcCULeft;       }
  TComDataCU*   getCUAbove                  () { return m_pcCUAbove;      }
  TComDataCU*   getCUAboveLeft              () { return m_pcCUAboveLeft;  }
  TComDataCU*   getCUAboveRight             () { return m_pcCUAboveRight; }
  TComDataCU*   getCUColocated              ( RefPicList eRefPicList ) { return m_apcCUColocated[eRefPicList]; }


  TComDataCU*   getPULeft                   ( UInt&  uiLPartUnitIdx , UInt uiCurrPartUnitIdx, Bool bEnforceSliceRestriction=true, Bool bEnforceEntropySliceRestriction=true
                                            , Bool bEnforceTileRestriction=true 
                                            );

  TComDataCU*   getPUAbove                  ( UInt&  uiAPartUnitIdx , UInt uiCurrPartUnitIdx, Bool bEnforceSliceRestriction=true, Bool bEnforceEntropySliceRestriction=true, Bool MotionDataCompresssion = false
                                            , Bool planarAtLCUBoundary = false 
                                            , Bool bEnforceTileRestriction=true 
                                            );

  TComDataCU*   getPUAboveLeft              ( UInt&  uiALPartUnitIdx, UInt uiCurrPartUnitIdx, Bool bEnforceSliceRestriction=true, Bool bEnforceEntropySliceRestriction=true, Bool MotionDataCompresssion = false );
  TComDataCU*   getPUAboveRight             ( UInt&  uiARPartUnitIdx, UInt uiCurrPartUnitIdx, Bool bEnforceSliceRestriction=true, Bool bEnforceEntropySliceRestriction=true, Bool MotionDataCompresssion = false );
  TComDataCU*   getPUBelowLeft              ( UInt& uiBLPartUnitIdx, UInt uiCurrPartUnitIdx, Bool bEnforceSliceRestriction=true, Bool bEnforceEntropySliceRestriction=true );

  TComDataCU*   getQpMinCuLeft              ( UInt&  uiLPartUnitIdx , UInt uiCurrAbsIdxInLCU, Bool bEnforceSliceRestriction=true, Bool bEnforceEntropySliceRestriction=true );
#if H0204_QP_PREDICTION
  TComDataCU*   getQpMinCuAbove             ( UInt&  aPartUnitIdx , UInt currAbsIdxInLCU, Bool enforceSliceRestriction=true, Bool enforceEntropySliceRestriction=true );
#endif
#if H0736_AVC_STYLE_QP_RANGE
  Char          getRefQP                    ( UInt   uiCurrAbsIdxInLCU                       );
#else
  UChar         getRefQP                    ( UInt   uiCurrAbsIdxInLCU                       );
#endif

  TComDataCU*   getPUAboveRightAdi          ( UInt&  uiARPartUnitIdx, UInt uiPuWidth, UInt uiCurrPartUnitIdx, UInt uiPartUnitOffset = 1, Bool bEnforceSliceRestriction=true, Bool bEnforceEntropySliceRestriction=true );
  TComDataCU*   getPUBelowLeftAdi           ( UInt& uiBLPartUnitIdx, UInt uiPuHeight, UInt uiCurrPartUnitIdx, UInt uiPartUnitOffset = 1, Bool bEnforceSliceRestriction=true, Bool bEnforceEntropySliceRestriction=true );
  
  Void          deriveLeftRightTopIdx       ( PartSize eCUMode, UInt uiPartIdx, UInt& ruiPartIdxLT, UInt& ruiPartIdxRT );
  Void          deriveLeftBottomIdx         ( PartSize eCUMode, UInt uiPartIdx, UInt& ruiPartIdxLB );
  
  Void          deriveLeftRightTopIdxAdi    ( UInt& ruiPartIdxLT, UInt& ruiPartIdxRT, UInt uiPartOffset, UInt uiPartDepth );
  Void          deriveLeftBottomIdxAdi      ( UInt& ruiPartIdxLB, UInt  uiPartOffset, UInt uiPartDepth );
  
  Bool          hasEqualMotion              ( UInt uiAbsPartIdx, TComDataCU* pcCandCU, UInt uiCandAbsPartIdx );
#if SIMP_MRG_PRUN
#if MERL_VSP_C0152
  Void          getInterMergeCandidates     ( UInt uiAbsPartIdx, UInt uiPUIdx, UInt uiDepth, TComMvField* pcMFieldNeighbours, UChar* puhInterDirNeighbours, Int& numValidMergeCand, Int* uiVSPIndexTrue, Int mrgCandIdx = -1 );
#else
  Void          getInterMergeCandidates     ( UInt uiAbsPartIdx, UInt uiPUIdx, UInt uiDepth, TComMvField* pcMFieldNeighbours, UChar* puhInterDirNeighbours, Int& numValidMergeCand, Int mrgCandIdx = -1 );
#endif
#else
  Void          getInterMergeCandidates     ( UInt uiAbsPartIdx, UInt uiPUIdx, UInt uiDepth, TComMvField* pcMFieldNeighbours, UChar* puhInterDirNeighbours, Int& numValidMergeCand );
#endif
  Void          deriveLeftRightTopIdxGeneral( PartSize eCUMode, UInt uiAbsPartIdx, UInt uiPartIdx, UInt& ruiPartIdxLT, UInt& ruiPartIdxRT );
  Void          deriveLeftBottomIdxGeneral  ( PartSize eCUMode, UInt uiAbsPartIdx, UInt uiPartIdx, UInt& ruiPartIdxLB );
  
  
  // -------------------------------------------------------------------------------------------------------------------
  // member functions for modes
  // -------------------------------------------------------------------------------------------------------------------
  
  Bool          isIntra   ( UInt uiPartIdx )  { return m_pePredMode[ uiPartIdx ] == MODE_INTRA; }
  Bool          isSkipped ( UInt uiPartIdx );                                                     ///< SKIP (no residual)
  
  // -------------------------------------------------------------------------------------------------------------------
  // member functions for symbol prediction (most probable / mode conversion)
  // -------------------------------------------------------------------------------------------------------------------
  
  UInt          getIntraSizeIdx                 ( UInt uiAbsPartIdx                                       );
  Void          convertTransIdx                 ( UInt uiAbsPartIdx, UInt uiTrIdx, UInt& ruiLumaTrMode, UInt& ruiChromaTrMode );
  
  Void          getAllowedChromaDir             ( UInt uiAbsPartIdx, UInt* uiModeList );
  Int           getIntraDirLumaPredictor        ( UInt uiAbsPartIdx, Int* uiIntraDirPred, Int* piMode = NULL );
  
  // -------------------------------------------------------------------------------------------------------------------
  // member functions for SBAC context
  // -------------------------------------------------------------------------------------------------------------------
  
  UInt          getCtxSplitFlag                 ( UInt   uiAbsPartIdx, UInt uiDepth                   );
  UInt          getCtxQtCbf                     ( UInt   uiAbsPartIdx, TextType eType, UInt uiTrDepth );

  UInt          getCtxSkipFlag                  ( UInt   uiAbsPartIdx                                 );
#if LGE_ILLUCOMP_B0045
  UInt          getCtxICFlag                    ( UInt   uiAbsPartIdx                                 );
#endif
  UInt          getCtxInterDir                  ( UInt   uiAbsPartIdx                                 );

#if HHI_INTER_VIEW_RESIDUAL_PRED
  UInt          getCtxResPredFlag               ( UInt   uiAbsPartIdx                                 );
#endif
  
  UInt          getSliceStartCU         ( UInt pos )                  { return m_uiSliceStartCU[pos-m_uiAbsIdxInLCU];                                                                                          }
  UInt          getEntropySliceStartCU  ( UInt pos )                  { return m_uiEntropySliceStartCU[pos-m_uiAbsIdxInLCU];                                                                                   }
  UInt&         getTotalBins            ()                            { return m_uiTotalBins;                                                                                                  }

#if LGE_EDGE_INTRA_A0070
  UInt          getCtxEdgeIntra ( UInt uiAbsPartIdx );
#endif

  // -------------------------------------------------------------------------------------------------------------------
  // member functions for RD cost storage
  // -------------------------------------------------------------------------------------------------------------------
  
  Double&       getTotalCost()                  { return m_dTotalCost;        }
  Dist&         getTotalDistortion()            { return m_uiTotalDistortion; }
  UInt&         getTotalBits()                  { return m_uiTotalBits;       }
  UInt&         getTotalNumPart()               { return m_uiNumPartition;    }

  UInt          getCoefScanIdx(UInt uiAbsPartIdx, UInt uiWidth, Bool bIsLuma, Bool bIsIntra);

  Bool          useNonSquareTrans( UInt uiTrMode, Int absPartIdx );
  Void          getNSQTSize(Int trMode, Int absPartIdx, Int &trWidth, Int &trHeight);
  Bool          useNonSquarePU   ( UInt absPartIdx);
  UInt          getInterTUSplitDirection ( Int width, Int height, Int trLastWidth, Int trLastHeight );
  UInt          getNSAbsPartIdx  ( UInt log2TrafoSize, UInt absPartIdx, UInt absTUPartIdx, UInt innerQuadIdx, UInt trMode );
  UInt          getNSAddrChroma   ( UInt uiLog2TrSizeC, UInt uiTrModeC, UInt uiQuadrant, UInt absTUPartIdx );
  
// -------------------------------------------------------------------------------------------------------------------
  // member functions for depth model modes
  // -------------------------------------------------------------------------------------------------------------------
#if HHI_DMM_WEDGE_INTRA
  UInt* getWedgeFullTabIdx                ()                      { return m_puiWedgeFullTabIdx;              }
  UInt  getWedgeFullTabIdx                ( UInt uiIdx )          { return m_puiWedgeFullTabIdx[uiIdx];       }
  Void  setWedgeFullTabIdx                ( UInt uiIdx, UInt uh ) { m_puiWedgeFullTabIdx[uiIdx] = uh;         }
  Void  setWedgeFullTabIdxSubParts        ( UInt uiTIdx, UInt uiAbsPartIdx, UInt uiDepth );
  Int*  getWedgeFullDeltaDC1              ()                      { return m_piWedgeFullDeltaDC1;             }
  Int   getWedgeFullDeltaDC1              ( UInt uiIdx )          { return m_piWedgeFullDeltaDC1[uiIdx];      }
  Void  setWedgeFullDeltaDC1              ( UInt uiIdx, Int i )   { m_piWedgeFullDeltaDC1[uiIdx] = i;         }
  Void  setWedgeFullDeltaDC1SubParts      ( Int iDC1, UInt uiAbsPartIdx, UInt uiDepth );
  Int*  getWedgeFullDeltaDC2              ()                      { return m_piWedgeFullDeltaDC2;             }
  Int   getWedgeFullDeltaDC2              ( UInt uiIdx )          { return m_piWedgeFullDeltaDC2[uiIdx];      }
  Void  setWedgeFullDeltaDC2              ( UInt uiIdx, Int i )   { m_piWedgeFullDeltaDC2[uiIdx] = i;         }
  Void  setWedgeFullDeltaDC2SubParts      ( Int iDC2, UInt uiAbsPartIdx, UInt uiDepth );

  UInt* getWedgePredDirTabIdx             ()                      { return m_puiWedgePredDirTabIdx;           }
  UInt  getWedgePredDirTabIdx             ( UInt uiIdx )          { return m_puiWedgePredDirTabIdx[uiIdx];    }
  Void  setWedgePredDirTabIdx             ( UInt uiIdx, UInt uh ) { m_puiWedgePredDirTabIdx[uiIdx] = uh;      }
  Void  setWedgePredDirTabIdxSubParts     ( UInt uiTIdx, UInt uiAbsPartIdx, UInt uiDepth );
  Int*  getWedgePredDirDeltaDC1           ()                      { return m_piWedgePredDirDeltaDC1;          }
  Int   getWedgePredDirDeltaDC1           ( UInt uiIdx )          { return m_piWedgePredDirDeltaDC1[uiIdx];   }
  Void  setWedgePredDirDeltaDC1           ( UInt uiIdx, Int i )   { m_piWedgePredDirDeltaDC1[uiIdx] = i;      }
  Void  setWedgePredDirDeltaDC1SubParts   ( Int iDC1, UInt uiAbsPartIdx, UInt uiDepth );
  Int*  getWedgePredDirDeltaDC2           ()                      { return m_piWedgePredDirDeltaDC2;          }
  Int   getWedgePredDirDeltaDC2           ( UInt uiIdx )          { return m_piWedgePredDirDeltaDC2[uiIdx];   }
  Void  setWedgePredDirDeltaDC2           ( UInt uiIdx, Int i )   { m_piWedgePredDirDeltaDC2[uiIdx] = i;      }
  Void  setWedgePredDirDeltaDC2SubParts   ( Int iDC2, UInt uiAbsPartIdx, UInt uiDepth );
  Int*  getWedgePredDirDeltaEnd           ()                      { return m_piWedgePredDirDeltaEnd;          }
  Int   getWedgePredDirDeltaEnd           ( UInt uiIdx )          { return m_piWedgePredDirDeltaEnd[uiIdx];   }
  Void  setWedgePredDirDeltaEnd           ( UInt uiIdx, Int iD )  { m_piWedgePredDirDeltaEnd[uiIdx] = iD;     }
  Void  setWedgePredDirDeltaEndSubParts   ( Int iDelta, UInt uiAbsPartIdx, UInt uiDepth );
#endif
#if HHI_DMM_PRED_TEX
  UInt* getWedgePredTexTabIdx             ()                      { return m_puiWedgePredTexTabIdx;           }
  UInt  getWedgePredTexTabIdx             ( UInt uiIdx )          { return m_puiWedgePredTexTabIdx[uiIdx];    }
  Void  setWedgePredTexTabIdx             ( UInt uiIdx, UInt uh ) { m_puiWedgePredTexTabIdx[uiIdx] = uh;      }
  Void  setWedgePredTexTabIdxSubParts     ( UInt uiTIdx, UInt uiAbsPartIdx, UInt uiDepth );
#if LGE_DMM3_SIMP_C0044
  UInt* getWedgePredTexIntraTabIdx             ()                      { return m_puiWedgePredTexIntraTabIdx;           }
  UInt  getWedgePredTexIntraTabIdx             ( UInt uiIdx )          { return m_puiWedgePredTexIntraTabIdx[uiIdx];    }
  Void  setWedgePredTexIntraTabIdx             ( UInt uiIdx, UInt uh ) { m_puiWedgePredTexIntraTabIdx[uiIdx] = uh;      }
  Void  setWedgePredTexIntraTabIdxSubParts     ( UInt uiTIdx, UInt uiAbsPartIdx, UInt uiDepth );
#endif 
  Int*  getWedgePredTexDeltaDC1           ()                      { return m_piWedgePredTexDeltaDC1;          }
  Int   getWedgePredTexDeltaDC1           ( UInt uiIdx )          { return m_piWedgePredTexDeltaDC1[uiIdx];   }
  Void  setWedgePredTexDeltaDC1           ( UInt uiIdx, Int i )   { m_piWedgePredTexDeltaDC1[uiIdx] = i;      }
  Void  setWedgePredTexDeltaDC1SubParts   ( Int iDC1, UInt uiAbsPartIdx, UInt uiDepth );
  Int*  getWedgePredTexDeltaDC2           ()                      { return m_piWedgePredTexDeltaDC2;          }
  Int   getWedgePredTexDeltaDC2           ( UInt uiIdx )          { return m_piWedgePredTexDeltaDC2[uiIdx];   }
  Void  setWedgePredTexDeltaDC2           ( UInt uiIdx, Int i )   { m_piWedgePredTexDeltaDC2[uiIdx] = i;      }
  Void  setWedgePredTexDeltaDC2SubParts   ( Int iDC2, UInt uiAbsPartIdx, UInt uiDepth );

  Int*  getContourPredTexDeltaDC1         ()                      { return m_piContourPredTexDeltaDC1;        }
  Int   getContourPredTexDeltaDC1         ( UInt uiIdx )          { return m_piContourPredTexDeltaDC1[uiIdx]; }
  Void  setContourPredTexDeltaDC1         ( UInt uiIdx, Int i )   { m_piContourPredTexDeltaDC1[uiIdx] = i;    }
  Void  setContourPredTexDeltaDC1SubParts ( Int iDC1, UInt uiAbsPartIdx, UInt uiDepth );
  Int*  getContourPredTexDeltaDC2         ()                      { return m_piContourPredTexDeltaDC2;        }
  Int   getContourPredTexDeltaDC2         ( UInt uiIdx )          { return m_piContourPredTexDeltaDC2[uiIdx]; }
  Void  setContourPredTexDeltaDC2         ( UInt uiIdx, Int i )   { m_piContourPredTexDeltaDC2[uiIdx] = i;    }
  Void  setContourPredTexDeltaDC2SubParts ( Int iDC2, UInt uiAbsPartIdx, UInt uiDepth );
#endif

#if LGE_EDGE_INTRA_A0070
  UChar*        getEdgeCode( UInt uiIdx )                 { return &m_pucEdgeCode[uiIdx * LGE_EDGE_INTRA_MAX_EDGE_NUM_PER_4x4]; }
  UChar*        getEdgeNumber( )                          { return m_pucEdgeNumber;           }
  UChar         getEdgeNumber( UInt uiIdx )               { return m_pucEdgeNumber[uiIdx];    }
  Void          setEdgeNumber( UInt uiIdx, UChar val )    { m_pucEdgeNumber[uiIdx] = val;     }
  UChar*        getEdgeStartPos( )                        { return m_pucEdgeStartPos;         }
  UChar         getEdgeStartPos( UInt uiIdx )             { return m_pucEdgeStartPos[uiIdx];  }
  Void          setEdgeStartPos( UInt uiIdx, UChar val )  { m_pucEdgeStartPos[uiIdx] = val;   }
  Bool*         getEdgeLeftFirst( )                       { return m_pbEdgeLeftFirst;         }
  Bool          getEdgeLeftFirst( UInt uiIdx )            { return m_pbEdgeLeftFirst[uiIdx];  }
  Void          setEdgeLeftFirst( UInt uiIdx, Bool val )  { m_pbEdgeLeftFirst[uiIdx] = val;   }
  Bool*         getEdgePartition( UInt uiIdx )              { return &m_pbEdgePartition[uiIdx * 16]; }
  Void          reconPartition( UInt uiAbsPartIdx, UInt uiDepth, Bool bLeft, UChar ucStartPos, UChar ucNumEdge, UChar* pucEdgeCode, Bool* pbRegion );

#if LGE_EDGE_INTRA_DELTA_DC
  Int*          getEdgeDeltaDC0( )                          { return m_piEdgeDeltaDC0; }
  Int*          getEdgeDeltaDC1( )                          { return m_piEdgeDeltaDC1; }
  Int           getEdgeDeltaDC0( UInt uiIdx )               { return m_piEdgeDeltaDC0[uiIdx]; }
  Int           getEdgeDeltaDC1( UInt uiIdx )               { return m_piEdgeDeltaDC1[uiIdx]; }
  Void          setEdgeDeltaDC0( UInt uiIdx, Int val )      { m_piEdgeDeltaDC0[uiIdx] = val;  }
  Void          setEdgeDeltaDC1( UInt uiIdx, Int val )      { m_piEdgeDeltaDC1[uiIdx] = val;  }
#endif
#endif
  
#if RWTH_SDC_DLT_B0036
  Bool*         getSDCFlag          ()                        { return m_pbSDCFlag;               }
  Bool          getSDCFlag          ( UInt uiIdx )            { return m_pbSDCFlag[uiIdx];        }
  Void          setSDCFlagSubParts  ( Bool bSDCFlag, UInt uiAbsPartIdx, UInt uiPartIdx, UInt uiDepth );
  
  UInt          getCtxSDCFlag              ( UInt uiAbsPartIdx );
  
  Bool          getSDCAvailable             ( UInt uiAbsPartIdx );
  
  Pel*          getSDCSegmentDCOffset( UInt uiSeg ) { return m_apSegmentDCOffset[uiSeg]; }
  Pel           getSDCSegmentDCOffset( UInt uiSeg, UInt uiPartIdx ) { return m_apSegmentDCOffset[uiSeg][uiPartIdx]; }
  Void          setSDCSegmentDCOffset( Pel pOffset, UInt uiSeg, UInt uiPartIdx) { m_apSegmentDCOffset[uiSeg][uiPartIdx] = pOffset; }
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
