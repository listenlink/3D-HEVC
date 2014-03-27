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

/** \file     TComSlice.h
    \brief    slice header and SPS class (header)
*/

#ifndef __TCOMSLICE__
#define __TCOMSLICE__

#include <cstring>
#include <map>
#include <vector>
#include "CommonDef.h"
#include "TComRom.h"
#include "TComList.h"

//! \ingroup TLibCommon
//! \{

class TComPic;
class TComTrQuant;
#if H_MV
class TComPicLists; 
class TComVPS; 
#endif
// ====================================================================================================================
// Constants
// ====================================================================================================================

// ====================================================================================================================
// Class definition
// ====================================================================================================================

/// Reference Picture Set class
class TComReferencePictureSet
{
private:
  Int  m_numberOfPictures;
  Int  m_numberOfNegativePictures;
  Int  m_numberOfPositivePictures;
  Int  m_numberOfLongtermPictures;
  Int  m_deltaPOC[MAX_NUM_REF_PICS];
  Int  m_POC[MAX_NUM_REF_PICS];
  Bool m_used[MAX_NUM_REF_PICS];
  Bool m_interRPSPrediction;
  Int  m_deltaRIdxMinus1;   
  Int  m_deltaRPS; 
  Int  m_numRefIdc; 
  Int  m_refIdc[MAX_NUM_REF_PICS+1];
  Bool m_bCheckLTMSB[MAX_NUM_REF_PICS];
  Int  m_pocLSBLT[MAX_NUM_REF_PICS];
  Int  m_deltaPOCMSBCycleLT[MAX_NUM_REF_PICS];
  Bool m_deltaPocMSBPresentFlag[MAX_NUM_REF_PICS];

public:
  TComReferencePictureSet();
  virtual ~TComReferencePictureSet();
  Int   getPocLSBLT(Int i)                       { return m_pocLSBLT[i]; }
  Void  setPocLSBLT(Int i, Int x)                { m_pocLSBLT[i] = x; }
  Int   getDeltaPocMSBCycleLT(Int i)             { return m_deltaPOCMSBCycleLT[i]; }
  Void  setDeltaPocMSBCycleLT(Int i, Int x)      { m_deltaPOCMSBCycleLT[i] = x; }
  Bool  getDeltaPocMSBPresentFlag(Int i)         { return m_deltaPocMSBPresentFlag[i]; }
  Void  setDeltaPocMSBPresentFlag(Int i, Bool x) { m_deltaPocMSBPresentFlag[i] = x;    }
  Void setUsed(Int bufferNum, Bool used);
  Void setDeltaPOC(Int bufferNum, Int deltaPOC);
  Void setPOC(Int bufferNum, Int deltaPOC);
  Void setNumberOfPictures(Int numberOfPictures);
  Void setCheckLTMSBPresent(Int bufferNum, Bool b );
  Bool getCheckLTMSBPresent(Int bufferNum);

  Int  getUsed(Int bufferNum);
  Int  getDeltaPOC(Int bufferNum);
  Int  getPOC(Int bufferNum);
  Int  getNumberOfPictures();

  Void setNumberOfNegativePictures(Int number)  { m_numberOfNegativePictures = number; }
  Int  getNumberOfNegativePictures()            { return m_numberOfNegativePictures; }
  Void setNumberOfPositivePictures(Int number)  { m_numberOfPositivePictures = number; }
  Int  getNumberOfPositivePictures()            { return m_numberOfPositivePictures; }
  Void setNumberOfLongtermPictures(Int number)  { m_numberOfLongtermPictures = number; }
  Int  getNumberOfLongtermPictures()            { return m_numberOfLongtermPictures; }

  Void setInterRPSPrediction(Bool flag)         { m_interRPSPrediction = flag; }
  Bool getInterRPSPrediction()                  { return m_interRPSPrediction; }
  Void setDeltaRIdxMinus1(Int x)                { m_deltaRIdxMinus1 = x; }
  Int  getDeltaRIdxMinus1()                     { return m_deltaRIdxMinus1; }
  Void setDeltaRPS(Int x)                       { m_deltaRPS = x; }
  Int  getDeltaRPS()                            { return m_deltaRPS; }
  Void setNumRefIdc(Int x)                      { m_numRefIdc = x; }
  Int  getNumRefIdc()                           { return m_numRefIdc; }

  Void setRefIdc(Int bufferNum, Int refIdc);
  Int  getRefIdc(Int bufferNum);

  Void sortDeltaPOC();
  Void printDeltaPOC();
};

/// Reference Picture Set set class
class TComRPSList
{
private:
  Int  m_numberOfReferencePictureSets;
  TComReferencePictureSet* m_referencePictureSets;
  
public:
  TComRPSList();
  virtual ~TComRPSList();
  
  Void  create  (Int numberOfEntries);
  Void  destroy ();


  TComReferencePictureSet* getReferencePictureSet(Int referencePictureSetNum);
  Int getNumberOfReferencePictureSets();
  Void setNumberOfReferencePictureSets(Int numberOfReferencePictureSets);
};

/// SCALING_LIST class
class TComScalingList
{
public:
  TComScalingList();
  virtual ~TComScalingList();
  Void     setScalingListPresentFlag    (Bool b)                               { m_scalingListPresentFlag = b;    }
  Bool     getScalingListPresentFlag    ()                                     { return m_scalingListPresentFlag; }
  Int*     getScalingListAddress          (UInt sizeId, UInt listId)           { return m_scalingListCoef[sizeId][listId]; } //!< get matrix coefficient
  Bool     checkPredMode                  (UInt sizeId, UInt listId);
  Void     setRefMatrixId                 (UInt sizeId, UInt listId, UInt u)   { m_refMatrixId[sizeId][listId] = u;    }     //!< set reference matrix ID
  UInt     getRefMatrixId                 (UInt sizeId, UInt listId)           { return m_refMatrixId[sizeId][listId]; }     //!< get reference matrix ID
  Int*     getScalingListDefaultAddress   (UInt sizeId, UInt listId);                                                        //!< get default matrix coefficient
  Void     processDefaultMatrix            (UInt sizeId, UInt listId);
  Void     setScalingListDC               (UInt sizeId, UInt listId, UInt u)   { m_scalingListDC[sizeId][listId] = u; }      //!< set DC value

  Int      getScalingListDC               (UInt sizeId, UInt listId)           { return m_scalingListDC[sizeId][listId]; }   //!< get DC value
  Void     checkDcOfMatrix                ();
  Void     processRefMatrix               (UInt sizeId, UInt listId , UInt refListId );
  Bool     xParseScalingList              (Char* pchFile);
#if H_MV
  Void     inferFrom                      ( TComScalingList* srcScLi );
#endif

private:
  Void     init                    ();
  Void     destroy                 ();
  Int      m_scalingListDC               [SCALING_LIST_SIZE_NUM][SCALING_LIST_NUM]; //!< the DC value of the matrix coefficient for 16x16
  Bool     m_useDefaultScalingMatrixFlag [SCALING_LIST_SIZE_NUM][SCALING_LIST_NUM]; //!< UseDefaultScalingMatrixFlag
  UInt     m_refMatrixId                 [SCALING_LIST_SIZE_NUM][SCALING_LIST_NUM]; //!< RefMatrixID
  Bool     m_scalingListPresentFlag;                                                //!< flag for using default matrix
  UInt     m_predMatrixId                [SCALING_LIST_SIZE_NUM][SCALING_LIST_NUM]; //!< reference list index
  Int      *m_scalingListCoef            [SCALING_LIST_SIZE_NUM][SCALING_LIST_NUM]; //!< quantization matrix
};

class ProfileTierLevel
{
  Int     m_profileSpace;
  Bool    m_tierFlag;
  Int     m_profileIdc;
  Bool    m_profileCompatibilityFlag[32];
  Int     m_levelIdc;

  Bool m_progressiveSourceFlag;
  Bool m_interlacedSourceFlag;
  Bool m_nonPackedConstraintFlag;
  Bool m_frameOnlyConstraintFlag;
  
public:
  ProfileTierLevel();

  Int   getProfileSpace() const   { return m_profileSpace; }
  Void  setProfileSpace(Int x)    { m_profileSpace = x; }

  Bool  getTierFlag()     const   { return m_tierFlag; }
  Void  setTierFlag(Bool x)       { m_tierFlag = x; }

  Int   getProfileIdc()   const   { return m_profileIdc; }
  Void  setProfileIdc(Int x)      { m_profileIdc = x; }

  Bool  getProfileCompatibilityFlag(Int i) const    { return m_profileCompatibilityFlag[i]; }
  Void  setProfileCompatibilityFlag(Int i, Bool x)  { m_profileCompatibilityFlag[i] = x; }

  Int   getLevelIdc()   const   { return m_levelIdc; }
  Void  setLevelIdc(Int x)      { m_levelIdc = x; }
  
  Bool getProgressiveSourceFlag() const { return m_progressiveSourceFlag; }
  Void setProgressiveSourceFlag(Bool b) { m_progressiveSourceFlag = b; }
  
  Bool getInterlacedSourceFlag() const { return m_interlacedSourceFlag; }
  Void setInterlacedSourceFlag(Bool b) { m_interlacedSourceFlag = b; }
  
  Bool getNonPackedConstraintFlag() const { return m_nonPackedConstraintFlag; }
  Void setNonPackedConstraintFlag(Bool b) { m_nonPackedConstraintFlag = b; }
  
  Bool getFrameOnlyConstraintFlag() const { return m_frameOnlyConstraintFlag; }
  Void setFrameOnlyConstraintFlag(Bool b) { m_frameOnlyConstraintFlag = b; }
};


class TComPTL
{
  ProfileTierLevel m_generalPTL;
  ProfileTierLevel m_subLayerPTL[6];      // max. value of max_sub_layers_minus1 is 6
  Bool m_subLayerProfilePresentFlag[6];
  Bool m_subLayerLevelPresentFlag[6];

public:
  TComPTL();
  Bool getSubLayerProfilePresentFlag(Int i) const { return m_subLayerProfilePresentFlag[i]; }
  Void setSubLayerProfilePresentFlag(Int i, Bool x) { m_subLayerProfilePresentFlag[i] = x; }
  
  Bool getSubLayerLevelPresentFlag(Int i) const { return m_subLayerLevelPresentFlag[i]; }
  Void setSubLayerLevelPresentFlag(Int i, Bool x) { m_subLayerLevelPresentFlag[i] = x; }

  ProfileTierLevel* getGeneralPTL()  { return &m_generalPTL; }
  ProfileTierLevel* getSubLayerPTL(Int i)  { return &m_subLayerPTL[i]; }
#if H_MV
  Void copyLevelFrom( TComPTL* source );
#endif
};
/// VPS class

struct HrdSubLayerInfo
{
  Bool fixedPicRateFlag;
  Bool fixedPicRateWithinCvsFlag;
  UInt picDurationInTcMinus1;
  Bool lowDelayHrdFlag;
  UInt cpbCntMinus1;
  UInt bitRateValueMinus1[MAX_CPB_CNT][2];
  UInt cpbSizeValue      [MAX_CPB_CNT][2];
  UInt ducpbSizeValue    [MAX_CPB_CNT][2];
  UInt cbrFlag           [MAX_CPB_CNT][2];
  UInt duBitRateValue    [MAX_CPB_CNT][2];
};

class TComHRD
{
private:
  Bool m_nalHrdParametersPresentFlag;
  Bool m_vclHrdParametersPresentFlag;
  Bool m_subPicCpbParamsPresentFlag;
  UInt m_tickDivisorMinus2;
  UInt m_duCpbRemovalDelayLengthMinus1;
  Bool m_subPicCpbParamsInPicTimingSEIFlag;
  UInt m_dpbOutputDelayDuLengthMinus1;
  UInt m_bitRateScale;
  UInt m_cpbSizeScale;
  UInt m_ducpbSizeScale;
  UInt m_initialCpbRemovalDelayLengthMinus1;
  UInt m_cpbRemovalDelayLengthMinus1;
  UInt m_dpbOutputDelayLengthMinus1;
  UInt m_numDU;
  HrdSubLayerInfo m_HRD[MAX_TLAYER];

public:
  TComHRD()
  :m_nalHrdParametersPresentFlag(0)
  ,m_vclHrdParametersPresentFlag(0)
  ,m_subPicCpbParamsPresentFlag(false)
  ,m_tickDivisorMinus2(0)
  ,m_duCpbRemovalDelayLengthMinus1(0)
  ,m_subPicCpbParamsInPicTimingSEIFlag(false)
  ,m_dpbOutputDelayDuLengthMinus1(0)
  ,m_bitRateScale(0)
  ,m_cpbSizeScale(0)
  ,m_initialCpbRemovalDelayLengthMinus1(0)
  ,m_cpbRemovalDelayLengthMinus1(0)
  ,m_dpbOutputDelayLengthMinus1(0)
  {}

  virtual ~TComHRD() {}

  Void setNalHrdParametersPresentFlag       ( Bool flag )  { m_nalHrdParametersPresentFlag = flag;         }
  Bool getNalHrdParametersPresentFlag       ( )            { return m_nalHrdParametersPresentFlag;         }

  Void setVclHrdParametersPresentFlag       ( Bool flag )  { m_vclHrdParametersPresentFlag = flag;         }
  Bool getVclHrdParametersPresentFlag       ( )            { return m_vclHrdParametersPresentFlag;         }

  Void setSubPicCpbParamsPresentFlag        ( Bool flag )  { m_subPicCpbParamsPresentFlag = flag;          }
  Bool getSubPicCpbParamsPresentFlag        ( )            { return m_subPicCpbParamsPresentFlag;          }
  
  Void setTickDivisorMinus2                 ( UInt value ) { m_tickDivisorMinus2 = value;                  }
  UInt getTickDivisorMinus2                 ( )            { return m_tickDivisorMinus2;                   }

  Void setDuCpbRemovalDelayLengthMinus1     ( UInt value ) { m_duCpbRemovalDelayLengthMinus1 = value;      }
  UInt getDuCpbRemovalDelayLengthMinus1     ( )            { return m_duCpbRemovalDelayLengthMinus1;       }

  Void setSubPicCpbParamsInPicTimingSEIFlag ( Bool flag)   { m_subPicCpbParamsInPicTimingSEIFlag = flag;   }
  Bool getSubPicCpbParamsInPicTimingSEIFlag ()             { return m_subPicCpbParamsInPicTimingSEIFlag;   }

  Void setDpbOutputDelayDuLengthMinus1      (UInt value )  { m_dpbOutputDelayDuLengthMinus1 = value;       }
  UInt getDpbOutputDelayDuLengthMinus1      ()             { return m_dpbOutputDelayDuLengthMinus1;        }

  Void setBitRateScale                      ( UInt value ) { m_bitRateScale = value;                       }
  UInt getBitRateScale                      ( )            { return m_bitRateScale;                        }

  Void setCpbSizeScale                      ( UInt value ) { m_cpbSizeScale = value;                       }
  UInt getCpbSizeScale                      ( )            { return m_cpbSizeScale;                        }
  Void setDuCpbSizeScale                    ( UInt value ) { m_ducpbSizeScale = value;                     }
  UInt getDuCpbSizeScale                    ( )            { return m_ducpbSizeScale;                      }

  Void setInitialCpbRemovalDelayLengthMinus1( UInt value ) { m_initialCpbRemovalDelayLengthMinus1 = value; }
  UInt getInitialCpbRemovalDelayLengthMinus1( )            { return m_initialCpbRemovalDelayLengthMinus1;  }

  Void setCpbRemovalDelayLengthMinus1       ( UInt value ) { m_cpbRemovalDelayLengthMinus1 = value;        }
  UInt getCpbRemovalDelayLengthMinus1       ( )            { return m_cpbRemovalDelayLengthMinus1;         }

  Void setDpbOutputDelayLengthMinus1        ( UInt value ) { m_dpbOutputDelayLengthMinus1 = value;         }
  UInt getDpbOutputDelayLengthMinus1        ( )            { return m_dpbOutputDelayLengthMinus1;          }

  Void setFixedPicRateFlag       ( Int layer, Bool flag )  { m_HRD[layer].fixedPicRateFlag = flag;         }
  Bool getFixedPicRateFlag       ( Int layer            )  { return m_HRD[layer].fixedPicRateFlag;         }

  Void setFixedPicRateWithinCvsFlag       ( Int layer, Bool flag )  { m_HRD[layer].fixedPicRateWithinCvsFlag = flag;         }
  Bool getFixedPicRateWithinCvsFlag       ( Int layer            )  { return m_HRD[layer].fixedPicRateWithinCvsFlag;         }

  Void setPicDurationInTcMinus1  ( Int layer, UInt value ) { m_HRD[layer].picDurationInTcMinus1 = value;   }
  UInt getPicDurationInTcMinus1  ( Int layer             ) { return m_HRD[layer].picDurationInTcMinus1;    }

  Void setLowDelayHrdFlag        ( Int layer, Bool flag )  { m_HRD[layer].lowDelayHrdFlag = flag;          }
  Bool getLowDelayHrdFlag        ( Int layer            )  { return m_HRD[layer].lowDelayHrdFlag;          }

  Void setCpbCntMinus1           ( Int layer, UInt value ) { m_HRD[layer].cpbCntMinus1 = value; }
  UInt getCpbCntMinus1           ( Int layer            )  { return m_HRD[layer].cpbCntMinus1; }

  Void setBitRateValueMinus1     ( Int layer, Int cpbcnt, Int nalOrVcl, UInt value ) { m_HRD[layer].bitRateValueMinus1[cpbcnt][nalOrVcl] = value; }
  UInt getBitRateValueMinus1     ( Int layer, Int cpbcnt, Int nalOrVcl             ) { return m_HRD[layer].bitRateValueMinus1[cpbcnt][nalOrVcl];  }

  Void setCpbSizeValueMinus1     ( Int layer, Int cpbcnt, Int nalOrVcl, UInt value ) { m_HRD[layer].cpbSizeValue[cpbcnt][nalOrVcl] = value;       }
  UInt getCpbSizeValueMinus1     ( Int layer, Int cpbcnt, Int nalOrVcl            )  { return m_HRD[layer].cpbSizeValue[cpbcnt][nalOrVcl];        }
  Void setDuCpbSizeValueMinus1     ( Int layer, Int cpbcnt, Int nalOrVcl, UInt value ) { m_HRD[layer].ducpbSizeValue[cpbcnt][nalOrVcl] = value;       }
  UInt getDuCpbSizeValueMinus1     ( Int layer, Int cpbcnt, Int nalOrVcl            )  { return m_HRD[layer].ducpbSizeValue[cpbcnt][nalOrVcl];        }
  Void setDuBitRateValueMinus1     ( Int layer, Int cpbcnt, Int nalOrVcl, UInt value ) { m_HRD[layer].duBitRateValue[cpbcnt][nalOrVcl] = value;       }
  UInt getDuBitRateValueMinus1     (Int layer, Int cpbcnt, Int nalOrVcl )              { return m_HRD[layer].duBitRateValue[cpbcnt][nalOrVcl];        }
  Void setCbrFlag                ( Int layer, Int cpbcnt, Int nalOrVcl, UInt value ) { m_HRD[layer].cbrFlag[cpbcnt][nalOrVcl] = value;            }
  Bool getCbrFlag                ( Int layer, Int cpbcnt, Int nalOrVcl             ) { return m_HRD[layer].cbrFlag[cpbcnt][nalOrVcl];             }

  Void setNumDU                              ( UInt value ) { m_numDU = value;                            }
  UInt getNumDU                              ( )            { return m_numDU;          }
  Bool getCpbDpbDelaysPresentFlag() { return getNalHrdParametersPresentFlag() || getVclHrdParametersPresentFlag(); }
};

class TimingInfo
{
  Bool m_timingInfoPresentFlag;
  UInt m_numUnitsInTick;
  UInt m_timeScale;
  Bool m_pocProportionalToTimingFlag;
  Int  m_numTicksPocDiffOneMinus1;
public:
  TimingInfo()
  : m_timingInfoPresentFlag(false)
  , m_numUnitsInTick(1001)
  , m_timeScale(60000)
  , m_pocProportionalToTimingFlag(false)
  , m_numTicksPocDiffOneMinus1(0) {}

  Void setTimingInfoPresentFlag             ( Bool flag )  { m_timingInfoPresentFlag = flag;               }
  Bool getTimingInfoPresentFlag             ( )            { return m_timingInfoPresentFlag;               }

  Void setNumUnitsInTick                    ( UInt value ) { m_numUnitsInTick = value;                     }
  UInt getNumUnitsInTick                    ( )            { return m_numUnitsInTick;                      }

  Void setTimeScale                         ( UInt value ) { m_timeScale = value;                          }
  UInt getTimeScale                         ( )            { return m_timeScale;                           }
  
  Bool getPocProportionalToTimingFlag       ( )            { return m_pocProportionalToTimingFlag;         }
  Void setPocProportionalToTimingFlag       (Bool x      ) { m_pocProportionalToTimingFlag = x;            }
  
  Int  getNumTicksPocDiffOneMinus1          ( )            { return m_numTicksPocDiffOneMinus1;            }
  Void setNumTicksPocDiffOneMinus1          (Int x       ) { m_numTicksPocDiffOneMinus1 = x;               }
};

#if H_MV

class TComVideoSignalInfo
{
private: 
  Int  m_videoVpsFormat;
  Bool m_videoFullRangeVpsFlag;
  Int  m_colourPrimariesVps;
  Int  m_transferCharacteristicsVps;
  Int  m_matrixCoeffsVps;
public: 
  Void setVideoVpsFormat( Int  val ) { m_videoVpsFormat = val; } 
  Int  getVideoVpsFormat(  ) { return m_videoVpsFormat; } 

  Void setVideoFullRangeVpsFlag( Bool flag ) { m_videoFullRangeVpsFlag = flag; } 
  Bool getVideoFullRangeVpsFlag(  ) { return m_videoFullRangeVpsFlag; } 

  Void setColourPrimariesVps( Int  val ) { m_colourPrimariesVps = val; } 
  Int  getColourPrimariesVps(  ) { return m_colourPrimariesVps; } 

  Void setTransferCharacteristicsVps( Int  val ) { m_transferCharacteristicsVps = val; } 
  Int  getTransferCharacteristicsVps(  ) { return m_transferCharacteristicsVps; } 

  Void setMatrixCoeffsVps( Int  val ) { m_matrixCoeffsVps = val; } 
  Int  getMatrixCoeffsVps(  ) { return m_matrixCoeffsVps; } 
};
class TComVpsVuiBspHrdParameters
{
private: 
  Int  m_vpsNumBspHrdParametersMinus1;
  Bool m_bspCprmsPresentFlag[MAX_NUM_BSP_HRD_PARAMETERS];
  Int  m_numBitstreamPartitions[MAX_VPS_OP_SETS_PLUS1];
  Bool m_layerInBspFlag[MAX_VPS_OP_SETS_PLUS1][MAX_NUM_BSP_HRD_PARAMETERS][MAX_NUM_LAYERS];
  Int  m_numBspSchedCombinations[MAX_VPS_OP_SETS_PLUS1];
  Int  m_bspCombHrdIdx[MAX_VPS_OP_SETS_PLUS1][MAX_NUM_BSP_HRD_PARAMETERS][MAX_NUM_BSP_SCHED_COMBINATION];
  Int  m_bspCombSchedIdx[MAX_VPS_OP_SETS_PLUS1][MAX_NUM_BSP_HRD_PARAMETERS][MAX_NUM_BSP_SCHED_COMBINATION];
  TComHRD* m_hrdParameters[MAX_NUM_BSP_HRD_PARAMETERS]; 
public: 

  Void setVpsNumBspHrdParametersMinus1( Int  val ) { m_vpsNumBspHrdParametersMinus1 = val; } 
  Int  getVpsNumBspHrdParametersMinus1(  ) { return m_vpsNumBspHrdParametersMinus1; } 

  Void setBspCprmsPresentFlag( Int i, Bool flag ) { m_bspCprmsPresentFlag[i] = flag; } 
  Bool getBspCprmsPresentFlag( Int i ) { return m_bspCprmsPresentFlag[i]; } 

  Void setNumBitstreamPartitions( Int h, Int  val ) { m_numBitstreamPartitions[h] = val; } 
  Int  getNumBitstreamPartitions( Int h ) { return m_numBitstreamPartitions[h]; } 

  Void setLayerInBspFlag( Int h, Int i, Int j, Bool flag ) { m_layerInBspFlag[h][i][j] = flag; } 
  Bool getLayerInBspFlag( Int h, Int i, Int j ) { return m_layerInBspFlag[h][i][j]; } 
  Void checkLayerInBspFlag ( TComVPS* vps, Int h );  

  Void setNumBspSchedCombinations( Int h, Int  val ) { m_numBspSchedCombinations[h] = val; } 
  Int  getNumBspSchedCombinations( Int h ) { return m_numBspSchedCombinations[h]; } 

  Void setBspCombHrdIdx( Int h, Int i, Int j, Int  val ) { m_bspCombHrdIdx[h][i][j] = val; } 
  Int  getBspCombHrdIdx( Int h, Int i, Int j ) { return m_bspCombHrdIdx[h][i][j]; } 

  Void setBspCombSchedIdx( Int h, Int i, Int j, Int  val ) { m_bspCombSchedIdx[h][i][j] = val; } 
  Int  getBspCombSchedIdx( Int h, Int i, Int j ) { return m_bspCombSchedIdx[h][i][j]; } 

  Void setHrdParametermeters( Int k, TComHRD* val  ) {  m_hrdParameters[k] = val; }; 
  TComHRD* getHrdParametermeters( Int k ) {  return m_hrdParameters[k]; }; 
};

class TComVPSVUI
{
private:
  Bool m_crossLayerPicTypeAlignedFlag;
  Bool m_crossLayerIrapAlignedFlag;
#if H_MV_HLS_7_MISC_P0068_21
  Bool m_allLayersIdrAlignedFlag;
#endif
  Bool m_bitRatePresentVpsFlag;
  Bool m_picRatePresentVpsFlag;
  Bool m_bitRatePresentFlag          [MAX_VPS_OP_SETS_PLUS1][MAX_TLAYER];
  Bool m_picRatePresentFlag          [MAX_VPS_OP_SETS_PLUS1][MAX_TLAYER];
  Int  m_avgBitRate                  [MAX_VPS_OP_SETS_PLUS1][MAX_TLAYER];
  Int  m_maxBitRate                  [MAX_VPS_OP_SETS_PLUS1][MAX_TLAYER];
  Int  m_constantPicRateIdc          [MAX_VPS_OP_SETS_PLUS1][MAX_TLAYER];
  Int  m_avgPicRate                  [MAX_VPS_OP_SETS_PLUS1][MAX_TLAYER];
#if H_MV_HLS_7_VPS_P0076_15
  Bool m_videoSignalInfoIdxPresentFlag;
  Int  m_vpsNumVideoSignalInfoMinus1;
  TComVideoSignalInfo* m_videoSignalInfo[MAX_NUM_VIDEO_SIGNAL_INFO];   
  Int  m_vpsVideoSignalInfoIdx       [MAX_NUM_VIDEO_SIGNAL_INFO];
#endif
  Bool m_tilesNotInUseFlag;
  Bool m_tilesInUseFlag              [MAX_NUM_LAYERS];
  Bool m_loopFilterNotAcrossTilesFlag[MAX_NUM_LAYERS];
  Bool m_tileBoundariesAlignedFlag   [MAX_NUM_LAYERS][MAX_NUM_LAYERS];
  Bool m_wppNotInUseFlag;
  Bool m_wppInUseFlag                [MAX_NUM_LAYERS];
  Bool m_ilpRestrictedRefLayersFlag;
  Int  m_minSpatialSegmentOffsetPlus1[MAX_NUM_LAYERS][MAX_NUM_LAYERS];
  Bool m_ctuBasedOffsetEnabledFlag   [MAX_NUM_LAYERS][MAX_NUM_LAYERS];
  Int  m_minHorizontalCtuOffsetPlus1 [MAX_NUM_LAYERS][MAX_NUM_LAYERS];
#if !H_MV_HLS_7_VPS_P0076_15
  Bool m_videoSignalInfoIdxPresentFlag;
  Int  m_vpsNumVideoSignalInfoMinus1;
  TComVideoSignalInfo* m_videoSignalInfo[MAX_NUM_VIDEO_SIGNAL_INFO];   
  Int  m_vpsVideoSignalInfoIdx       [MAX_NUM_VIDEO_SIGNAL_INFO];
#endif
  Bool m_vpsVuiBspHrdPresentFlag;
  TComVpsVuiBspHrdParameters* m_vpsVuiBspHrdParameters; 
#if H_MV_HLS_7_MISC_P0182_13
  Bool m_baseLayerParameterSetCompatibilityFlag[MAX_NUM_LAYERS];
#endif

public: 
  TComVPSVUI();
  ~TComVPSVUI(); 
  Void setCrossLayerPicTypeAlignedFlag( Bool flag ) { m_crossLayerPicTypeAlignedFlag = flag; } 
  Bool getCrossLayerPicTypeAlignedFlag(  ) { return m_crossLayerPicTypeAlignedFlag; } 

  Void setCrossLayerIrapAlignedFlag( Bool flag ) { m_crossLayerIrapAlignedFlag = flag; } 
  Bool getCrossLayerIrapAlignedFlag(  ) { return m_crossLayerIrapAlignedFlag; } 

#if H_MV_HLS_7_MISC_P0068_21
  Void setAllLayersIdrAlignedFlag( Bool flag ) { m_allLayersIdrAlignedFlag = flag; } 
  Bool getAllLayersIdrAlignedFlag(  ) { return m_allLayersIdrAlignedFlag; } 
#endif

  Void setBitRatePresentVpsFlag( Bool flag ) { m_bitRatePresentVpsFlag = flag; } 
  Bool getBitRatePresentVpsFlag(  ) { return m_bitRatePresentVpsFlag; } 

  Void setPicRatePresentVpsFlag( Bool flag ) { m_picRatePresentVpsFlag = flag; } 
  Bool getPicRatePresentVpsFlag(  ) { return m_picRatePresentVpsFlag; } 

  Void setBitRatePresentFlag( Int i, Int j, Bool flag ) { m_bitRatePresentFlag[i][j] = flag; } 
  Bool getBitRatePresentFlag( Int i, Int j ) { return m_bitRatePresentFlag[i][j]; } 

  Void setPicRatePresentFlag( Int i, Int j, Bool flag ) { m_picRatePresentFlag[i][j] = flag; } 
  Bool getPicRatePresentFlag( Int i, Int j ) { return m_picRatePresentFlag[i][j]; } 

  Void setAvgBitRate( Int i, Int j, Int  val ) { m_avgBitRate[i][j] = val; } 
  Int  getAvgBitRate( Int i, Int j ) { return m_avgBitRate[i][j]; } 

  Void setMaxBitRate( Int i, Int j, Int  val ) { m_maxBitRate[i][j] = val; } 
  Int  getMaxBitRate( Int i, Int j ) { return m_maxBitRate[i][j]; } 

  Void setConstantPicRateIdc( Int i, Int j, Int  val ) { m_constantPicRateIdc[i][j] = val; } 
  Int  getConstantPicRateIdc( Int i, Int j ) { return m_constantPicRateIdc[i][j]; } 

  Void setAvgPicRate( Int i, Int j, Int  val ) { m_avgPicRate[i][j] = val; } 
  Int  getAvgPicRate( Int i, Int j ) { return m_avgPicRate[i][j]; } 

#if H_MV_HLS_7_VPS_P0076_15
  Void setVideoSignalInfoIdxPresentFlag( Bool flag ) { m_videoSignalInfoIdxPresentFlag = flag; } 
  Bool getVideoSignalInfoIdxPresentFlag(  ) { return m_videoSignalInfoIdxPresentFlag; } 

  Void    setVideoSignalInfo( Int i, TComVideoSignalInfo* val )                        { m_videoSignalInfo[i] = val;  }  
  TComVideoSignalInfo* getVideoSignalInfo( Int i )                                     { return m_videoSignalInfo[i]; }

  Void setVpsNumVideoSignalInfoMinus1( Int  val ) { m_vpsNumVideoSignalInfoMinus1 = val; } 
  Int  getVpsNumVideoSignalInfoMinus1(  ) { return m_vpsNumVideoSignalInfoMinus1; } 

  Void setVpsVideoSignalInfoIdx( Int i, Int  val ) { m_vpsVideoSignalInfoIdx[i] = val; } 
  Int  getVpsVideoSignalInfoIdx( Int i ) { return m_vpsVideoSignalInfoIdx[i]; } 
#endif

  Void setTilesNotInUseFlag( Bool flag ) { m_tilesNotInUseFlag = flag; } 
  Bool getTilesNotInUseFlag(  ) { return m_tilesNotInUseFlag; } 

  Void setTilesInUseFlag( Int i, Bool flag ) { m_tilesInUseFlag[i] = flag; } 
  Bool getTilesInUseFlag( Int i ) { return m_tilesInUseFlag[i]; } 

  Void setLoopFilterNotAcrossTilesFlag( Int i, Int  val ) { m_loopFilterNotAcrossTilesFlag[i] = val; } 
  Bool getLoopFilterNotAcrossTilesFlag( Int i ) { return m_loopFilterNotAcrossTilesFlag[i]; } 

  Void setTileBoundariesAlignedFlag( Int i, Int j, Bool flag ) { m_tileBoundariesAlignedFlag[i][j] = flag; } 
  Bool getTileBoundariesAlignedFlag( Int i, Int j ) { return m_tileBoundariesAlignedFlag[i][j]; } 

  Void setWppNotInUseFlag( Bool flag ) { m_wppNotInUseFlag = flag; } 
  Bool getWppNotInUseFlag(  ) { return m_wppNotInUseFlag; } 

  Void setWppInUseFlag( Int i, Bool flag ) { m_wppInUseFlag[i] = flag; } 
  Bool getWppInUseFlag( Int i ) { return m_wppInUseFlag[i]; } 

  Void setIlpRestrictedRefLayersFlag( Bool flag ) { m_ilpRestrictedRefLayersFlag = flag; } 
  Bool getIlpRestrictedRefLayersFlag(  ) { return m_ilpRestrictedRefLayersFlag; } 

  Void setMinSpatialSegmentOffsetPlus1( Int i, Int j, Int  val ) { m_minSpatialSegmentOffsetPlus1[i][j] = val; } 
  Int  getMinSpatialSegmentOffsetPlus1( Int i, Int j ) { return m_minSpatialSegmentOffsetPlus1[i][j]; } 

  Void setCtuBasedOffsetEnabledFlag( Int i, Int j, Bool flag ) { m_ctuBasedOffsetEnabledFlag[i][j] = flag; } 
  Bool getCtuBasedOffsetEnabledFlag( Int i, Int j ) { return m_ctuBasedOffsetEnabledFlag[i][j]; } 

  Void setMinHorizontalCtuOffsetPlus1( Int i, Int j, Int  val ) { m_minHorizontalCtuOffsetPlus1[i][j] = val; } 
  Int  getMinHorizontalCtuOffsetPlus1( Int i, Int j ) { return m_minHorizontalCtuOffsetPlus1[i][j]; } 
#if !H_MV_HLS_7_VPS_P0076_15
  Void setVideoSignalInfoIdxPresentFlag( Bool flag ) { m_videoSignalInfoIdxPresentFlag = flag; } 
  Bool getVideoSignalInfoIdxPresentFlag(  ) { return m_videoSignalInfoIdxPresentFlag; } 

  Void    setVideoSignalInfo( Int i, TComVideoSignalInfo* val )                        { m_videoSignalInfo[i] = val;  }  
  TComVideoSignalInfo* getVideoSignalInfo( Int i )                                     { return m_videoSignalInfo[i]; }

  Void setVpsNumVideoSignalInfoMinus1( Int  val ) { m_vpsNumVideoSignalInfoMinus1 = val; } 
  Int  getVpsNumVideoSignalInfoMinus1(  ) { return m_vpsNumVideoSignalInfoMinus1; } 

  Void setVpsVideoSignalInfoIdx( Int i, Int  val ) { m_vpsVideoSignalInfoIdx[i] = val; } 
  Int  getVpsVideoSignalInfoIdx( Int i ) { return m_vpsVideoSignalInfoIdx[i]; } 
#endif
  Void setVpsVuiBspHrdPresentFlag( Bool flag ) { m_vpsVuiBspHrdPresentFlag = flag; } 
  Bool getVpsVuiBspHrdPresentFlag(  ) { return m_vpsVuiBspHrdPresentFlag; }

  Void setVpsVuiBspHrdParameters( TComVpsVuiBspHrdParameters* val) {  m_vpsVuiBspHrdParameters = val; } 
  TComVpsVuiBspHrdParameters* getVpsVuiBspHrdParameters(  ) { return m_vpsVuiBspHrdParameters; }

#if H_MV_HLS_7_MISC_P0182_13
  Void setBaseLayerParameterSetCompatibilityFlag( Int i, Bool flag ) { m_baseLayerParameterSetCompatibilityFlag[i] = flag; } 
  Bool getBaseLayerParameterSetCompatibilityFlag( Int i ) { return m_baseLayerParameterSetCompatibilityFlag[i]; } 
#endif

#if H_MV_HLS_7_FIX_INFER_CROSS_LAYER_IRAP_ALIGNED_FLAG
  Void inferVpsVui( Bool encoderFlag )
  {
    // inference of syntax elements that differ from default inference (as done in constructor), when VPS VUI is not present
    if (!encoderFlag )
    {
      setCrossLayerIrapAlignedFlag( false ); 
    }
    else
    {
      assert( !getCrossLayerIrapAlignedFlag() ); 
    }

  }
#endif
};

class TComRepFormat
{
private:
  Bool m_chromaAndBitDepthVpsPresentFlag;
  Int  m_chromaFormatVpsIdc;
  Bool m_separateColourPlaneVpsFlag;
  Int  m_picWidthVpsInLumaSamples;
  Int  m_picHeightVpsInLumaSamples;
  Int  m_bitDepthVpsLumaMinus8;
  Int  m_bitDepthVpsChromaMinus8;

public: 
  TComRepFormat() { };  

  Void setChromaAndBitDepthVpsPresentFlag( Bool flag ) { m_chromaAndBitDepthVpsPresentFlag = flag; } 
  Bool getChromaAndBitDepthVpsPresentFlag(  ) { return m_chromaAndBitDepthVpsPresentFlag; } 
  Void checkChromaAndBitDepthVpsPresentFlag( Int i ) { assert( i != 0 || m_chromaAndBitDepthVpsPresentFlag ); } // The value of chroma_and_bit_depth_vps_present_flag of the first rep_format( ) syntax structure in the VPS shall be equal to 1.  
  Void inferChromaAndBitDepth( TComRepFormat* prevRepFormat, Bool encoderFlag );

  Void setChromaFormatVpsIdc( Int  val ) { m_chromaFormatVpsIdc = val; } 
  Int  getChromaFormatVpsIdc(  ) { return m_chromaFormatVpsIdc; } 

  Void setSeparateColourPlaneVpsFlag( Bool flag ) { m_separateColourPlaneVpsFlag = flag; } 
  Bool getSeparateColourPlaneVpsFlag(  ) { return m_separateColourPlaneVpsFlag; } 

  Void setPicWidthVpsInLumaSamples( Int  val ) { m_picWidthVpsInLumaSamples = val; } 
  Int  getPicWidthVpsInLumaSamples(  ) { return m_picWidthVpsInLumaSamples; } 

  Void setPicHeightVpsInLumaSamples( Int  val ) { m_picHeightVpsInLumaSamples = val; } 
  Int  getPicHeightVpsInLumaSamples(  ) { return m_picHeightVpsInLumaSamples; } 

  Void setBitDepthVpsLumaMinus8( Int  val ) { m_bitDepthVpsLumaMinus8 = val; } 
  Int  getBitDepthVpsLumaMinus8(  ) { return m_bitDepthVpsLumaMinus8; } 

  Void setBitDepthVpsChromaMinus8( Int  val ) { m_bitDepthVpsChromaMinus8 = val; } 
  Int  getBitDepthVpsChromaMinus8(  ) { return m_bitDepthVpsChromaMinus8; } 
};


class TComDpbSize
{
private:
  Bool  m_subLayerFlagInfoPresentFlag[MAX_VPS_OUTPUTLAYER_SETS];
  Bool  m_subLayerDpbInfoPresentFlag [MAX_VPS_OUTPUTLAYER_SETS][MAX_TLAYER];
  Int   m_maxVpsDecPicBufferingMinus1[MAX_VPS_OUTPUTLAYER_SETS][MAX_NUM_LAYER_IDS][MAX_TLAYER];; 
  Int   m_maxVpsNumReorderPics       [MAX_VPS_OUTPUTLAYER_SETS][MAX_TLAYER];
#if H_MV_HLS7_GEN
  Int  m_maxVpsLayerDecPicBuffMinus1[TO_BE_SPECIFIED][TO_BE_SPECIFIED][TO_BE_SPECIFIED];
#endif
  Int   m_maxVpsLatencyIncreasePlus1 [MAX_VPS_OUTPUTLAYER_SETS][MAX_TLAYER];

public: 
  TComDpbSize( )
  {
    for (Int i = 0; i < MAX_VPS_OUTPUTLAYER_SETS; i++ )
    {      
      m_subLayerFlagInfoPresentFlag[i]  = false;

      for (Int j = 0; j < MAX_TLAYER; j++  )
      {        
        m_subLayerDpbInfoPresentFlag [i][j] = ( j == 0) ;
        m_maxVpsNumReorderPics       [i][j] = 0;
        m_maxVpsLatencyIncreasePlus1 [i][j] = 0;

        for (Int k = 0; k < MAX_NUM_LAYER_IDS; k++ )
        {
          m_maxVpsDecPicBufferingMinus1[i][k][j] = 0; 
        }
      }
    }  
  }  

  Void setSubLayerFlagInfoPresentFlag( Int i, Bool flag ) { m_subLayerFlagInfoPresentFlag[i] = flag; } 
  Bool getSubLayerFlagInfoPresentFlag( Int i ) { return m_subLayerFlagInfoPresentFlag[i]; } 

  Void setSubLayerDpbInfoPresentFlag( Int i, Int j, Bool flag ) { m_subLayerDpbInfoPresentFlag[i][j] = flag; } 
  Bool getSubLayerDpbInfoPresentFlag( Int i, Int j ) { return m_subLayerDpbInfoPresentFlag[i][j]; } 

  Void setMaxVpsDecPicBufferingMinus1( Int i, Int k, Int j, Int  val ) { m_maxVpsDecPicBufferingMinus1[i][k][j] = val; } 
  Int  getMaxVpsDecPicBufferingMinus1( Int i, Int k, Int j ) { return m_maxVpsDecPicBufferingMinus1[i][k][j]; } 

  Void setMaxVpsNumReorderPics( Int i, Int j, Int  val ) { m_maxVpsNumReorderPics[i][j] = val; } 
  Int  getMaxVpsNumReorderPics( Int i, Int j ) { return m_maxVpsNumReorderPics[i][j]; } 
#if H_MV_HLS7_GEN
  Void setMaxVpsLayerDecPicBuffMinus1( Int i, Int k, Int j, Int  val ) { m_maxVpsLayerDecPicBuffMinus1[i][k][j] = val; } 
  Int  getMaxVpsLayerDecPicBuffMinus1( Int i, Int k, Int j ) { return m_maxVpsLayerDecPicBuffMinus1[i][k][j]; } 
#endif
  Void setMaxVpsLatencyIncreasePlus1( Int i, Int j, Int  val ) { m_maxVpsLatencyIncreasePlus1[i][j] = val; } 
  Int  getMaxVpsLatencyIncreasePlus1( Int i, Int j ) { return m_maxVpsLatencyIncreasePlus1[i][j]; } 
};
#endif
class TComVPS
{
private:
  Int         m_VPSId;
  UInt        m_uiMaxTLayers;

#if H_MV
  UInt        m_uiMaxLayersMinus1;
#else
  UInt        m_uiMaxLayers;
#endif
  Bool        m_bTemporalIdNestingFlag;
  
  UInt        m_numReorderPics[MAX_TLAYER];
  UInt        m_uiMaxDecPicBuffering[MAX_TLAYER]; 
  UInt        m_uiMaxLatencyIncrease[MAX_TLAYER]; // Really max latency increase plus 1 (value 0 expresses no limit)

  UInt        m_numHrdParameters;
#if H_MV
  UInt        m_maxLayerId;
#else
  UInt        m_maxNuhReservedZeroLayerId;
#endif
  TComHRD*    m_hrdParameters;
  UInt*       m_hrdOpSetIdx;
  Bool*       m_cprmsPresentFlag;
#if H_MV
  UInt        m_vpsNumLayerSetsMinus1;
  Bool        m_layerIdIncludedFlag[MAX_VPS_OP_SETS_PLUS1][MAX_VPS_NUH_LAYER_ID_PLUS1];
#else
  UInt        m_numOpSets;
  Bool        m_layerIdIncludedFlag[MAX_VPS_OP_SETS_PLUS1][MAX_VPS_NUH_RESERVED_ZERO_LAYER_ID_PLUS1];
#endif

#if H_MV
  TComPTL     m_pcPTL[MAX_VPS_OP_SETS_PLUS1];
#else
  TComPTL     m_pcPTL;
#endif
  TimingInfo  m_timingInfo;
#if H_MV
  /// VPS EXTENSION SYNTAX ELEMENTS
  Bool        m_avcBaseLayerFlag;
#if H_MV_HLS_7_VPS_P0307_23
  Int         m_vpsNonVuiExtensionLength;
#else
  Int         m_vpsVuiOffset;
#endif
  Bool        m_splittingFlag;
  Bool        m_scalabilityMaskFlag          [MAX_NUM_SCALABILITY_TYPES];
  Int         m_dimensionIdLen           [MAX_NUM_SCALABILITY_TYPES];
  Bool        m_vpsNuhLayerIdPresentFlag;
  Int         m_layerIdInNuh             [MAX_NUM_LAYER_IDS];
  Int         m_dimensionId              [MAX_NUM_LAYER_IDS][MAX_NUM_SCALABILITY_TYPES];  

  Int         m_viewIdLen;
  Int         m_viewIdVal                [MAX_NUM_LAYERS];
  Bool        m_directDependencyFlag     [MAX_NUM_LAYER_IDS][MAX_NUM_LAYER_IDS];
  Bool        m_vpsSubLayersMaxMinus1PresentFlag;
  Int         m_subLayersVpsMaxMinus1    [MAX_NUM_LAYERS];
  Bool        m_maxTidRefPresentFlag;
  Int         m_maxTidIlRefPicsPlus1     [MAX_NUM_LAYERS][MAX_NUM_LAYERS];
  Bool        m_allRefLayersActiveFlag;
#if !H_MV_HLS_7_OUTPUT_LAYERS_5_10_22_27
  Int         m_vpsNumberLayerSetsMinus1; 
#endif
  Int         m_vpsNumProfileTierLevelMinus1;   
  Bool        m_vpsProfilePresentFlag    [MAX_VPS_OP_SETS_PLUS1];

#if !H_MV_HLS_7_VPS_P0048_14
  Int         m_profileRefMinus1         [MAX_VPS_PROFILE_TIER_LEVEL];
#endif
#if H_MV_HLS_7_OUTPUT_LAYERS_5_10_22_27
  Int         m_numAddOutputLayerSets;    
  Int         m_defaultTargetOutputLayerIdc;
#else  
  Bool        m_moreOutputLayerSetsThanDefaultFlag;
  Int         m_numAddOutputLayerSetsMinus1;    
  Int         m_defaultOneTargetOutputLayerIdc;
#endif

  Int         m_outputLayerSetIdxMinus1  [MAX_VPS_OUTPUTLAYER_SETS];  
  Bool        m_outputLayerFlag          [MAX_VPS_OUTPUTLAYER_SETS][MAX_VPS_NUH_LAYER_ID_PLUS1];
  Int         m_profileLevelTierIdx      [MAX_VPS_OUTPUTLAYER_SETS ];
#if H_MV_HLS_7_OUTPUT_LAYERS_5_10_22_27
  Bool        m_altOutputLayerFlag       [MAX_VPS_OUTPUTLAYER_SETS];
#else
  Bool        m_altOutputLayerFlag;
#endif
  Bool        m_repFormatIdxPresentFlag;
  Int         m_vpsNumRepFormatsMinus1;
  Int         m_vpsRepFormatIdx          [MAX_NUM_LAYERS];
  TComRepFormat* m_repFormat             [MAX_NUM_LAYERS]; 
  Bool        m_maxOneActiveRefLayerFlag;       
#if H_MV_HLS7_GEN
  Bool        m_vpsPocLsbAlignedFlag;
#endif
  Bool        m_pocLsbNotPresentFlag     [MAX_NUM_LAYERS];

  TComDpbSize* m_dpbSize; 
  Int         m_directDepTypeLenMinus2;         
  Bool        m_defaultDirectDependencyFlag;
  Int         m_defaultDirectDependencyType;
  
#if H_MV_HLS7_GEN
  Int         m_directDependencyType     [MAX_NUM_LAYERS] [MAX_NUM_LAYERS];
#endif
  Bool        m_vpsVuiPresentFlag;
  TComVPSVUI* m_vpsVUI; 
#if !H_MV_HLS7_GEN
  Int         m_directDependencyType     [MAX_NUM_LAYERS] [MAX_NUM_LAYERS];
#endif

  // VPS EXTENSION SEMANTICS VARIABLES
  Int         m_layerIdInVps             [MAX_NUM_LAYERS   ];

  Int         m_numDirectRefLayers       [MAX_NUM_LAYERS];
  Int         m_refLayerId               [MAX_NUM_LAYERS][MAX_NUM_LAYERS];  

  Int         m_numSamplePredRefLayers   [MAX_NUM_LAYERS];
  Bool        m_samplePredEnabledFlag    [MAX_NUM_LAYERS][MAX_NUM_LAYERS];
  Int         m_samplePredRefLayerId     [MAX_NUM_LAYERS][MAX_NUM_LAYERS];

  Int         m_numMotionPredRefLayers   [MAX_NUM_LAYERS];
  Bool        m_motionPredEnabledFlag    [MAX_NUM_LAYERS][MAX_NUM_LAYERS];
  Int         m_motionPredRefLayerId     [MAX_NUM_LAYERS][MAX_NUM_LAYERS];
  Int         m_viewIndex                [MAX_NUM_LAYERS   ];
  
  std::vector< std::vector< Int> >       m_targetDecLayerIdLists;   //[TargetOptLayerSetIdx][i]
  std::vector< std::vector< Int> >       m_targetOptLayerIdLists; 
  std::vector< std::vector< Int> >       m_layerSetLayerIdList; 


  Int         xGetDimBitOffset( Int j );
  
  // VPS EXTENSION 2 SYNTAX ELEMENTS
#if H_3D_ARP
  UInt        m_uiUseAdvResPred          [MAX_NUM_LAYERS   ];
  UInt        m_uiARPStepNum             [MAX_NUM_LAYERS   ];
#endif
#if H_3D_IV_MERGE
  Bool        m_ivMvPredFlag             [ MAX_NUM_LAYERS ]; 
#if H_3D_SPIVMP
  Int         m_iSubPULog2Size           [MAX_NUM_LAYERS   ];
  Int         m_iSubPUMPILog2Size;
#endif
#endif
#if H_3D_VSP
  Bool        m_viewSynthesisPredFlag    [ MAX_NUM_LAYERS ];
#endif
#if H_3D_NBDV_REF
  Bool        m_depthRefinementFlag      [ MAX_NUM_LAYERS ]; 
#endif
  Bool        m_vpsDepthModesFlag        [MAX_NUM_LAYERS   ];

#if H_3D
  UInt        m_uiCamParPrecision;
  Bool*       m_bCamParInSliceHeader;
  Bool*       m_bCamParPresent;
  Int         ***m_aaaiCodedScale ;
  Int         ***m_aaaiCodedOffset;
  Bool        m_ivMvScalingFlag; 
#endif
#if H_3D_INTER_SDC
  Bool        m_bInterSDCFlag[MAX_NUM_LAYERS   ];
#endif
#if H_3D_DBBP
  Bool        m_dbbpFlag[MAX_NUM_LAYERS];
#endif
#if H_3D_IV_MERGE
  Bool        m_bMPIFlag[MAX_NUM_LAYERS   ];
#endif

#endif
public:
  TComVPS();
  virtual ~TComVPS();

  Void    createHrdParamBuffer()
  {
    m_hrdParameters    = new TComHRD[ getNumHrdParameters() ];
    m_hrdOpSetIdx      = new UInt   [ getNumHrdParameters() ];
    m_cprmsPresentFlag = new Bool   [ getNumHrdParameters() ];
  }

  TComHRD* getHrdParameters   ( UInt i )             { return &m_hrdParameters[ i ]; }
  UInt    getHrdOpSetIdx      ( UInt i )             { return m_hrdOpSetIdx[ i ]; }
  Void    setHrdOpSetIdx      ( UInt val, UInt i )   { m_hrdOpSetIdx[ i ] = val;  }
  Bool    getCprmsPresentFlag ( UInt i )             { return m_cprmsPresentFlag[ i ]; }
  Void    setCprmsPresentFlag ( Bool val, UInt i )   { m_cprmsPresentFlag[ i ] = val;  }

  Int     getVPSId       ()                   { return m_VPSId;          }
  Void    setVPSId       (Int i)              { m_VPSId = i;             }

  UInt    getMaxTLayers  ()                   { return m_uiMaxTLayers;   }
  Void    setMaxTLayers  (UInt t)             { m_uiMaxTLayers = t; }

#if H_MV    
  UInt    getMaxSubLayersMinus1()             { return m_uiMaxTLayers - 1;  }  // For consistency with draft spec
  UInt    getMaxLayersMinus1()                { return m_uiMaxLayersMinus1;  }; 
  Void    setMaxLayersMinus1(UInt l)          { m_uiMaxLayersMinus1 = l; }
#else
  UInt    getMaxLayers   ()                   { return m_uiMaxLayers;   }
  Void    setMaxLayers   (UInt l)             { m_uiMaxLayers = l; }
#endif

  Bool    getTemporalNestingFlag   ()         { return m_bTemporalIdNestingFlag;   }
  Void    setTemporalNestingFlag   (Bool t)   { m_bTemporalIdNestingFlag = t; }
  
  Void    setNumReorderPics(UInt v, UInt tLayer)                { m_numReorderPics[tLayer] = v;    }
  UInt    getNumReorderPics(UInt tLayer)                        { return m_numReorderPics[tLayer]; }
  
  Void    setMaxDecPicBuffering(UInt v, UInt tLayer)            { assert(tLayer < MAX_TLAYER); m_uiMaxDecPicBuffering[tLayer] = v;    }
  UInt    getMaxDecPicBuffering(UInt tLayer)                    { return m_uiMaxDecPicBuffering[tLayer]; }
  
  Void    setMaxLatencyIncrease(UInt v, UInt tLayer)            { m_uiMaxLatencyIncrease[tLayer] = v;    }
  UInt    getMaxLatencyIncrease(UInt tLayer)                    { return m_uiMaxLatencyIncrease[tLayer]; }

  UInt    getNumHrdParameters()                                 { return m_numHrdParameters; }
  Void    setNumHrdParameters(UInt v)                           { m_numHrdParameters = v;    }

#if H_MV
  UInt    getVpsMaxLayerId()                                    { return m_maxLayerId; }
  Void    setVpsMaxLayerId(UInt v)                              { m_maxLayerId = v;    }

  UInt    getVpsNumLayerSetsMinus1()                            { return m_vpsNumLayerSetsMinus1; }
  Void    setVpsNumLayerSetsMinus1(UInt v)                      { m_vpsNumLayerSetsMinus1 = v;    }
#else
  UInt    getMaxNuhReservedZeroLayerId()                        { return m_maxNuhReservedZeroLayerId; }
  Void    setMaxNuhReservedZeroLayerId(UInt v)                  { m_maxNuhReservedZeroLayerId = v;    }

  UInt    getMaxOpSets()                                        { return m_numOpSets; }
  Void    setMaxOpSets(UInt v)                                  { m_numOpSets = v;    }
#endif
  Bool    getLayerIdIncludedFlag(UInt opsIdx, UInt id)          { return m_layerIdIncludedFlag[opsIdx][id]; }
  Void    setLayerIdIncludedFlag(Bool v, UInt opsIdx, UInt id)  { m_layerIdIncludedFlag[opsIdx][id] = v;    }

#if H_MV
  TComPTL* getPTL( Int layerSet = 0 ) { return &m_pcPTL[layerSet]; }
#else
  TComPTL* getPTL() { return &m_pcPTL; }
#endif
  TimingInfo* getTimingInfo() { return &m_timingInfo; }
#if H_MV
  Void    setAvcBaseLayerFlag( Bool val )                                  { m_avcBaseLayerFlag = val;  }
  Bool    getAvcBaseLayerFlag()                                            { return m_avcBaseLayerFlag; } 

#if H_MV_HLS_7_VPS_P0307_23
  Void    setVpsNonVuiExtensionLength( Int  val )                          { m_vpsNonVuiExtensionLength = val; } 
  Int     getVpsNonVuiExtensionLength(  )                                  { return m_vpsNonVuiExtensionLength; } 
#else
  Void    setVpsVuiOffset( Int  val )                                      { m_vpsVuiOffset = val; } 
  Int     getVpsVuiOffset(  )                                              { return m_vpsVuiOffset; } 
#endif

  Void    setSplittingFlag( Bool val )                                     { m_splittingFlag = val;  }
  Bool    getSplittingFlag()                                               { return m_splittingFlag; }

  Void    setScalabilityMaskFlag( UInt val );
  Void    setScalabilityMaskFlag( Int scalType, Bool val )                     { m_scalabilityMaskFlag[scalType] = val;  }
  Bool    getScalabilityMaskFlag( Int scalType )                               { return m_scalabilityMaskFlag[scalType]; }
  Int     getNumScalabilityTypes( );

  Void    setDimensionIdLen( Int sIdx, Int val )                           { m_dimensionIdLen[sIdx] = val;  }
  Int     getDimensionIdLen( Int sIdx )                                    { assert( m_dimensionIdLen[sIdx] > 0) ; return m_dimensionIdLen[sIdx]; }  

  Void    setVpsNuhLayerIdPresentFlag( Bool val )                          { m_vpsNuhLayerIdPresentFlag = val; }
  Bool    getVpsNuhLayerIdPresentFlag()                                    { return m_vpsNuhLayerIdPresentFlag; }

  Void    setLayerIdInNuh( Int layerIdInVps, Int val )                     { m_layerIdInNuh[layerIdInVps] = val;  }
  Int     getLayerIdInNuh( Int layerIdInVps )                              { assert( m_layerIdInNuh[layerIdInVps] >= 0 ); return m_layerIdInNuh[layerIdInVps]; }

  Bool    nuhLayerIdIncluded( Int layerIdinNuh )                           { return ( m_layerIdInVps[ layerIdinNuh ] > 0 );  }

  Void    setDimensionId( Int layerIdInVps, Int scalIdx, Int val )         { m_dimensionId[layerIdInVps][scalIdx] = val;  }
  Int     getDimensionId( Int layerIdInVps, Int scalIdx )                  { return m_dimensionId[layerIdInVps][scalIdx]; }

  Void    setViewIdLen( Int  val )                                         { m_viewIdLen = val; } 
  Int     getViewIdLen(  )                                                 { return m_viewIdLen; } 

  Void    setViewIdVal( Int viewOrderIndex, Int  val )                     { m_viewIdVal[viewOrderIndex] = val; } 
  Int     getViewIdVal( Int viewOrderIndex )                               { return m_viewIdVal[viewOrderIndex]; } 
  Void    setDirectDependencyFlag( Int depLayeridInVps, Int refLayeridInVps, Bool val ) { m_directDependencyFlag[depLayeridInVps][refLayeridInVps] = val;  }
  Bool    getDirectDependencyFlag( Int depLayeridInVps, Int refLayeridInVps )           { return m_directDependencyFlag[depLayeridInVps][refLayeridInVps]; }
  Void    setVpsSubLayersMaxMinus1PresentFlag( Bool flag )                 { m_vpsSubLayersMaxMinus1PresentFlag = flag; } 
  Bool    getVpsSubLayersMaxMinus1PresentFlag(  )                          { return m_vpsSubLayersMaxMinus1PresentFlag; } 
  Void    setSubLayersVpsMaxMinus1( Int i, Int  val )                      { m_subLayersVpsMaxMinus1[i] = val; } 
  Int     getSubLayersVpsMaxMinus1( Int i )                                { return m_subLayersVpsMaxMinus1[i]; } 
  Void    checkSubLayersVpsMaxMinus1( Int i )                              { assert( m_subLayersVpsMaxMinus1[i] >= 0 && m_subLayersVpsMaxMinus1[i] <= m_uiMaxTLayers - 1 ); }
  Void    setMaxTidRefPresentFlag( Bool flag )                             { m_maxTidRefPresentFlag = flag; } 
  Bool    getMaxTidRefPresentFlag(  )                                      { return m_maxTidRefPresentFlag; } 
  Void    setMaxTidIlRefPicsPlus1( Int i, Int j, Int  val )                { m_maxTidIlRefPicsPlus1[i][j] = val; } 
  Int     getMaxTidIlRefPicsPlus1( Int i, Int j )                          { return m_maxTidIlRefPicsPlus1[i][j]; } 
  Void    setAllRefLayersActiveFlag( Bool flag )                           { m_allRefLayersActiveFlag = flag; } 
  Bool    getAllRefLayersActiveFlag(  )                                    { return m_allRefLayersActiveFlag; } 
  
#if !H_MV_HLS_7_OUTPUT_LAYERS_5_10_22_27
  Void    setVpsNumberLayerSetsMinus1( Int val )                           { m_vpsNumberLayerSetsMinus1 = val;  } 
  Int     getVpsNumberLayerSetsMinus1( )                                   { return m_vpsNumberLayerSetsMinus1; } 
#endif
  
  Void    setVpsNumProfileTierLevelMinus1( Int val )                       { m_vpsNumProfileTierLevelMinus1 = val;  } 
  Int     getVpsNumProfileTierLevelMinus1( )                               { return m_vpsNumProfileTierLevelMinus1; } 
  
  Void    setVpsProfilePresentFlag( Int idx, Bool val )                    { m_vpsProfilePresentFlag[idx] = val;  }
  Bool    getVpsProfilePresentFlag( Int idx )                              { return m_vpsProfilePresentFlag[idx]; }

#if !H_MV_HLS_7_VPS_P0048_14
  Void    setProfileRefMinus1( Int profileTierLevelIdx, Int val )          { m_profileRefMinus1[ profileTierLevelIdx ] = val;  } 
  Int     getProfileRefMinus1( Int profileTierLevelIdx )                   { return m_profileRefMinus1[ profileTierLevelIdx ]; } 
  Void    checkProfileRefMinus1( Int i )                                   { assert( getProfileRefMinus1( i ) + 1 <= i ); };  //  The value of profile_ref_minus1[ i ] + 1 shall be less than or equal to i.
#endif

#if !H_MV_HLS_7_OUTPUT_LAYERS_5_10_22_27
  Void    setMoreOutputLayerSetsThanDefaultFlag( Bool flag )               { m_moreOutputLayerSetsThanDefaultFlag = flag; } 
  Bool    getMoreOutputLayerSetsThanDefaultFlag()                          { return m_moreOutputLayerSetsThanDefaultFlag; } 
#endif
  
#if H_MV_HLS_7_OUTPUT_LAYERS_5_10_22_27
  Void    setNumAddOutputLayerSets( Int val )                              { m_numAddOutputLayerSets = val; } 
  Int     getNumAddOutputLayerSets( )                                      { return m_numAddOutputLayerSets; } 
#else
  Void    setNumAddOutputLayerSetsMinus1( Int val )                        { m_numAddOutputLayerSetsMinus1 = val; } 
  Int     getNumAddOutputLayerSetsMinus1( )                                { return m_numAddOutputLayerSetsMinus1; } 
#endif
  
#if H_MV_HLS_7_OUTPUT_LAYERS_5_10_22_27
  Void    setDefaultTargetOutputLayerIdc( Int  val )                       { m_defaultTargetOutputLayerIdc = val; } 
  Int     getDefaultTargetOutputLayerIdc(  )                               { return m_defaultTargetOutputLayerIdc; }   
#else
  Void    setDefaultOneTargetOutputLayerIdc( Int  val )                    { m_defaultOneTargetOutputLayerIdc = val; } 
  Int     getDefaultOneTargetOutputLayerIdc(  )                            { return m_defaultOneTargetOutputLayerIdc; } 
  Void    checkDefaultOneTargetOutputLayerIdc( )                           { assert( m_defaultOneTargetOutputLayerIdc >= 0 && m_defaultOneTargetOutputLayerIdc <= 1 ); }
#endif
  
  Void    setOutputLayerSetIdxMinus1( Int outLayerSetIdx, Int val )        { m_outputLayerSetIdxMinus1[ outLayerSetIdx ]  = val; } 
  Int     getOutputLayerSetIdxMinus1( Int outLayerSetIdx )                 { return m_outputLayerSetIdxMinus1[ outLayerSetIdx ]; } 

  Void    setOutputLayerFlag( Int outLayerSetIdx, Int i, Bool flag )       { m_outputLayerFlag[ outLayerSetIdx ][ i ] = flag; } 
  Bool    getOutputLayerFlag( Int outLayerSetIdx, Int i )                  { return m_outputLayerFlag[ outLayerSetIdx ][ i ]; } 
#if H_MV_HLS_7_OUTPUT_LAYERS_5_10_22_27
  Bool    inferOutputLayerFlag( Int i, Int j )                             
  { 
    Bool outputLayerFlag; 
    switch ( getDefaultTargetOutputLayerIdc( ) )
    {
    case 0:
      outputLayerFlag = true; 
      break; 
    case 1:
      outputLayerFlag = ( j == m_layerSetLayerIdList[ getLayerSetIdxForOutputLayerSet( i ) ].size() - 1 );  
      break;
    case 2:
      if ( i == 0 && j == 0)
      {     
        outputLayerFlag = true;  // This is a software only fix for a bug in the spec. In spec outputLayerFlag neither present nor inferred. 
      }
      else
      {
        assert( 0 ); 
      }
      break; 
    default:      
      assert( 0 );
      break; 
    }
    return outputLayerFlag;
  }
#else
  Bool    inferOutputLayerFlag( Int layerSetIdx, Int i )                   { return ( getDefaultOneTargetOutputLayerIdc( ) == 0 || ( ( getDefaultOneTargetOutputLayerIdc( ) == 1 ) && ( i == m_layerSetLayerIdList[layerSetIdx].size() - 1  ) ));  }
#endif

  Void    setProfileLevelTierIdx( Int outLayerSetIdx, Int val )            { m_profileLevelTierIdx[ outLayerSetIdx ] = val; }
  Int     getProfileLevelTierIdx( Int outLayerSetIdx )                     { return m_profileLevelTierIdx[ outLayerSetIdx ]; } 
  
#if H_MV_HLS_7_OUTPUT_LAYERS_5_10_22_27
  Void    setAltOutputLayerFlag( Int i, Bool flag )                        { m_altOutputLayerFlag[i] = flag; } 
  Bool    getAltOutputLayerFlag( Int i )                                   { return m_altOutputLayerFlag[i]; } 
#else
  Void    setAltOutputLayerFlag( Bool flag )                               { m_altOutputLayerFlag = flag; } 
  Bool    getAltOutputLayerFlag(  )                                        { return m_altOutputLayerFlag; } 
#endif

  Void    setRepFormatIdxPresentFlag( Bool flag )                          { m_repFormatIdxPresentFlag = flag; } 
  Bool    getRepFormatIdxPresentFlag(  )                                   { return m_repFormatIdxPresentFlag; } 

  Void    setVpsNumRepFormatsMinus1( Int  val )                            { m_vpsNumRepFormatsMinus1 = val; } 
  Int     getVpsNumRepFormatsMinus1(  )                                    { return m_vpsNumRepFormatsMinus1; } 

  Void    setVpsRepFormatIdx( Int i, Int  val )                            { m_vpsRepFormatIdx[i] = val; } 
  Int     getVpsRepFormatIdx( Int i )                                      { return m_vpsRepFormatIdx[i]; } 

  Void    setRepFormat( Int i, TComRepFormat* val )                        { m_repFormat[i] = val;  }
  TComRepFormat* getRepFormat( Int i )                                     { return m_repFormat[i]; }
  Void    setMaxOneActiveRefLayerFlag( Bool flag)                          { m_maxOneActiveRefLayerFlag = flag; } 
  Bool    getMaxOneActiveRefLayerFlag( )                                   { return m_maxOneActiveRefLayerFlag; } 

#if H_MV_HLS7_GEN
  Void    setVpsPocLsbAlignedFlag( Bool flag )                             { m_vpsPocLsbAlignedFlag = flag; } 
  Bool    getVpsPocLsbAlignedFlag(  )                                      { return m_vpsPocLsbAlignedFlag; } 
#endif

  Void    setDpbSize( TComDpbSize* val )                                   { assert( m_dpbSize != 0 ); m_dpbSize = val; } 
  TComDpbSize* getDpbSize( )                                               { return m_dpbSize;} 

  Void    setPocLsbNotPresentFlag( Int i, Bool flag )                      { m_pocLsbNotPresentFlag[i] = flag; } 
  Bool    getPocLsbNotPresentFlag( Int i )                                 { return m_pocLsbNotPresentFlag[i]; } 
  Void    setDirectDepTypeLenMinus2( Int val)                              { m_directDepTypeLenMinus2 = val; } 
  Int     getDirectDepTypeLenMinus2( )                                     { return m_directDepTypeLenMinus2; } 

  Void    setDefaultDirectDependencyFlag( Bool flag )                      { m_defaultDirectDependencyFlag = flag; } 
  Bool    getDefaultDirectDependencyFlag(  )                               { return m_defaultDirectDependencyFlag; } 

  Void    setDefaultDirectDependencyType( Int  val )                       { m_defaultDirectDependencyType = val; } 
  Int     getDefaultDirectDependencyType(  )                               { return m_defaultDirectDependencyType; } 
  Void    setDirectDependencyType( Int depLayeridInVps, Int refLayeridInVps, Int val) { m_directDependencyType[ depLayeridInVps ][ refLayeridInVps ] = val; } 
  Int     getDirectDependencyType( Int depLayeridInVps, Int refLayeridInVps)   { return m_directDependencyType[ depLayeridInVps ][ refLayeridInVps ]; } 

  Void    setVpsVuiPresentFlag( Bool flag )                                { m_vpsVuiPresentFlag = flag; } 
  Bool    getVpsVuiPresentFlag(  )                                         { return m_vpsVuiPresentFlag; } 

  TComVPSVUI* getVPSVUI(  )                                                { return m_vpsVUI;  }
 
 // VPS EXTENSION SEMANTICS VARIABLES
  Void    setLayerIdInVps( Int layerIdInNuh, Int val )                     { m_layerIdInVps[layerIdInNuh] = val;  }
  Int     getLayerIdInVps( Int layerIdInNuh )                              { assert( m_layerIdInVps[layerIdInNuh] >= 0 ); return m_layerIdInVps[layerIdInNuh]; }

  Int     getScalabilityId ( Int layerIdInVps, ScalabilityType scalType );
  Int     getViewId        ( Int layerIdInNuh )                            { return m_viewIdVal[ getViewIndex( layerIdInNuh )]; }
  Void    setRefLayers(); 

  Int     getViewIndex    ( Int layerIdInNuh )                             { return getScalabilityId( getLayerIdInVps(layerIdInNuh), VIEW_ORDER_INDEX  ); }    
  Int     getNumViews();

  Int     getNumDirectRefLayers( Int layerIdInNuh )                        { return m_numDirectRefLayers[ layerIdInNuh ];  };                               
  Int     getRefLayerId        ( Int layerIdInNuh, Int idx );; 
  Bool    checkVPSExtensionSyntax(); 
  Int     scalTypeToScalIdx   ( ScalabilityType scalType );

  Int     getProfileLevelTierIdxLen()                                      { return gCeilLog2( getVpsNumProfileTierLevelMinus1() + 1 ); };       

#if H_MV_HLS_7_VPS_P0306_22
  Int     getVpsRepFormatIdxLen()                                          { return gCeilLog2( getVpsNumRepFormatsMinus1() + 1 ); };       
#endif
  Int     getNumLayersInIdList ( Int lsIdx );

  Int     getNumOutputLayerSets() ;   
  Int     getNumSubDpbs( Int i )                                           { return getNumLayersInIdList( i ); };  
  Bool    isOutputLayer( Int outLayerSetIdx, Int layerIdInNuh );   
  Void    deriveLayerSetLayerIdList();

#if H_MV_HLS_7_OUTPUT_LAYERS_5_10_22_27
  Int     getLayerSetIdxForOutputLayerSet( Int i )                         { return ( i <= getVpsNumLayerSetsMinus1() ) ? i  : getOutputLayerSetIdxMinus1( i ) + 1 ; };

  Void    initTargetLayerIdLists  ( );
  Void    deriveTargetLayerIdList ( Int i );

  std::vector<Int> getTargetDecLayerIdList( Int targetDecLayerSetIdx )     { return m_targetDecLayerIdLists[targetDecLayerSetIdx]; }; 
  std::vector<Int> getTargetOptLayerIdList( Int targetOptLayerSetIdx )     { return m_targetOptLayerIdLists[targetOptLayerSetIdx]; }; 

  Int     getNumOutputLayersInOutputLayerSet( Int i )                      { return (Int) getTargetOptLayerIdList( i ).size(); }; 
  Int     getOlsHighestOutputLayerId( Int i )                              { return getTargetOptLayerIdList( i ).back(); };  
#else
  Void    deriveTargetLayerIdLists();
  std::vector<Int> getTargetDecLayerIdList( Int targetOptLayerSetIdx )     { return m_targetDecLayerIdLists[targetOptLayerSetIdx]; }; 
  std::vector<Int> getTargetOptLayerIdList( Int targetOptLayerSetIdx )     { return m_targetDecLayerIdLists[targetOptLayerSetIdx]; }; 
#endif

#if H_MV_HLS_7_HRD_P0156_7
  Int     getMaxSubLayersInLayerSetMinus1( Int i )
  { 
    Int maxSLMinus1 = 0; 
    Int optLsIdx    = getLayerSetIdxForOutputLayerSet( i );
    for( Int k = 0; k < getNumLayersInIdList( optLsIdx ); k++ )
    {
      Int lId = m_layerSetLayerIdList[optLsIdx][k];
      maxSLMinus1 = std::max( maxSLMinus1, getSubLayersVpsMaxMinus1( getLayerIdInVps( lId ) ));
    }
    return maxSLMinus1; 
  }
#endif

  // inference
  Int     inferDimensionId     ( Int i, Int j );
  Int     inferLastDimsionIdLenMinus1();

  // helpers
  Bool    getInDirectDependencyFlag( Int depLayeridInVps, Int refLayeridInVps, Int depth = 0 );
  /// VPS EXTENSION 2 SYNTAX ELEMENTS
#if H_3D  
  Int     getDepthId      ( Int layerIdInNuh)                             { return getScalabilityId( getLayerIdInVps(layerIdInNuh), DEPTH_ID ); }
  Int     getLayerIdInNuh( Int viewIndex, Bool depthFlag );   

#if H_3D_ARP
  UInt    getUseAdvRP  ( Int layerIdInVps )                                { return m_uiUseAdvResPred[layerIdInVps];    }
  UInt    getARPStepNum( Int layerIdInVps )                                { return m_uiARPStepNum[layerIdInVps];       }
  Void    setUseAdvRP  ( Int layerIdInVps, UInt val )                      { m_uiUseAdvResPred[layerIdInVps] = val;     }
  Void    setARPStepNum( Int layerIdInVps, UInt val )                      { m_uiARPStepNum[layerIdInVps]    = val;     }
#endif

  Void createCamPars(Int iNumViews);
  Void deleteCamPars();
  Void initCamParaVPS      (  UInt uiViewIndex, Bool bCamParPresent = false, UInt uiCamParPrecision = 0, Bool bCamParSlice = false, Int** aaiScale = 0, Int** aaiOffset = 0 );
  UInt getCamParPrecision    ()  { return m_uiCamParPrecision; }
  Bool getCamParPresent      ( Int viewIndex )  { return m_bCamParPresent[viewIndex]; }
#if FIX_CAM_PARS_COLLECTOR
  Void setCamParPresent      ( Int viewIndex, Bool val )  { m_bCamParPresent[viewIndex] = val; }
#endif
  Bool hasCamParInSliceHeader( Int viewIndex )  { return m_bCamParInSliceHeader[viewIndex]; }
  Void setHasCamParInSliceHeader( Int viewIndex, Bool b )  { m_bCamParInSliceHeader[viewIndex] = b; }
  Int* getCodedScale         ( Int viewIndex )  { return m_aaaiCodedScale [viewIndex][0]; }
  Int* getCodedOffset        ( Int viewIndex )  { return m_aaaiCodedOffset[viewIndex][0]; }
  Int* getInvCodedScale      ( Int viewIndex )  { return m_aaaiCodedScale [viewIndex][1]; }
  Int* getInvCodedOffset     ( Int viewIndex )  { return m_aaaiCodedOffset[viewIndex][1]; }

#if H_3D_IV_MERGE
  Void    setIvMvPredFlag     ( Int layerIdInVps, Bool val )  { m_ivMvPredFlag[ layerIdInVps ] = val; }
  Bool    getIvMvPredFlag     ( Int layerIdInVps )            { return m_ivMvPredFlag[ layerIdInVps ]; }; 
#if H_3D_SPIVMP
  Int     getSubPULog2Size(Int layerIdInVps)           { return m_iSubPULog2Size[layerIdInVps]; }
  Void    setSubPULog2Size(Int layerIdInVps, Int u)    { m_iSubPULog2Size[layerIdInVps] = u;}
  Int     getSubPUMPILog2Size( )           { return m_iSubPUMPILog2Size; }
  Void    setSubPUMPILog2Size( Int u )     { m_iSubPUMPILog2Size = u;    }
#endif
#endif
#if H_3D_VSP
  Void    setViewSynthesisPredFlag  ( Int layerIdInVps, Bool val )  { m_viewSynthesisPredFlag[ layerIdInVps ] = val; }
  Bool    getViewSynthesisPredFlag  ( Int layerIdInVps )            { return m_viewSynthesisPredFlag[ layerIdInVps ]; }; 
#endif
#if H_3D_NBDV_REF
  Void    setDepthRefinementFlag  ( Int layerIdInVps, Bool val )  { m_depthRefinementFlag[ layerIdInVps ] = val; }
  Bool    getDepthRefinementFlag  ( Int layerIdInVps )            { return m_depthRefinementFlag[ layerIdInVps ]; }; 
#endif
  Void    setVpsDepthModesFlag( Int layerIdInVps, Bool val )               { m_vpsDepthModesFlag[ layerIdInVps ] = val; }
  Bool    getVpsDepthModesFlag( Int layerIdInVps )                         { return m_vpsDepthModesFlag[ layerIdInVps ]; }

  Bool    getIvMvScalingFlag   (  )                       { return m_ivMvScalingFlag; }
  Void    setIvMvScalingFlag   ( Bool b )                 { m_ivMvScalingFlag = b;    }  
#if H_3D_INTER_SDC
  Bool    getInterSDCFlag      ( Int layerIdInVps )           { return m_bInterSDCFlag[layerIdInVps]; }
  Void    setInterSDCFlag      ( Int layerIdInVps, Bool bval ){ m_bInterSDCFlag[layerIdInVps] = bval; }
#endif
#if H_3D_DBBP
  Bool getUseDBBP              ( Int layerIdInVps )         { return m_dbbpFlag[layerIdInVps]; }
  Void setUseDBBP              ( Int layerIdInVps, Bool bval ){ m_dbbpFlag[layerIdInVps] = bval; }
#endif
#if H_3D_IV_MERGE
  Bool    getMPIFlag      ( Int layerIdInVps )           { return m_bMPIFlag[layerIdInVps]; }
  Void    setMPIFlag      ( Int layerIdInVps, Bool bval ){ m_bMPIFlag[layerIdInVps] = bval; }
#endif
#endif  
#endif
};

#if H_3D
class TComDLT
{
private:
  Bool        m_bDltPresentFlag;
  Bool        m_bUseDLTFlag              [ MAX_NUM_LAYERS ];
  Bool        m_bInterViewDltPredEnableFlag[ MAX_NUM_LAYERS ];

  Int         m_iBitsPerDepthValue       [ MAX_NUM_LAYERS ];
  Int         m_iNumDepthmapValues       [ MAX_NUM_LAYERS ];
  Int*        m_iDepthValue2Idx          [ MAX_NUM_LAYERS ];
  Int*        m_iIdx2DepthValue          [ MAX_NUM_LAYERS ];

  Int         m_iNumDepthViews;
  UInt        m_uiDepthViewBitDepth;

public:
  TComDLT();
  ~TComDLT(); 

  Bool    getDltPresentFlag  ()                           { return m_bDltPresentFlag; }
  Void    setDltPresentFlag  ( Bool b )                   { m_bDltPresentFlag = b;    }

  Bool    getUseDLTFlag      ( Int layerIdInVps )         { return m_bUseDLTFlag[ layerIdInVps ]; }
  Void    setUseDLTFlag      ( Int layerIdInVps, Bool b ) { m_bUseDLTFlag[ layerIdInVps ]  = b;   }
  
  Bool    getInterViewDltPredEnableFlag( Int layerIdInVps )         { return m_bInterViewDltPredEnableFlag[ layerIdInVps ]; }
  Void    setInterViewDltPredEnableFlag( Int layerIdInVps, Bool b ) { m_bInterViewDltPredEnableFlag[ layerIdInVps ] = b;    }

  Void    setNumDepthViews   ( Int n )                    { m_iNumDepthViews = n; }
  Int     getNumDepthViews   ()                           { return m_iNumDepthViews; }

  Void    setDepthViewBitDepth( UInt n )                  { m_uiDepthViewBitDepth = n; }
  UInt    getDepthViewBitDepth()                          { return m_uiDepthViewBitDepth; }

  Int     getBitsPerDepthValue( Int layerIdInVps )        { return getUseDLTFlag(layerIdInVps)?m_iBitsPerDepthValue[layerIdInVps]:g_bitDepthY; }
  Int     getNumDepthValues( Int layerIdInVps )           { return getUseDLTFlag(layerIdInVps)?m_iNumDepthmapValues[layerIdInVps]:((1 << g_bitDepthY)-1); }
  Int     depthValue2idx( Int layerIdInVps, Pel value )   { return getUseDLTFlag(layerIdInVps)?m_iDepthValue2Idx[layerIdInVps][value]:value; }
  Pel     idx2DepthValue( Int layerIdInVps, UInt uiIdx )  { return getUseDLTFlag(layerIdInVps)?m_iIdx2DepthValue[layerIdInVps][uiIdx]:uiIdx; }
  Void    setDepthLUTs( Int layerIdInVps, Int* idx2DepthValue = NULL, Int iNumDepthValues = 0 );
#if H_3D_DELTA_DLT
  Int*    idx2DepthValue( Int layerIdInVps )  { return m_iIdx2DepthValue[layerIdInVps]; }
  Void    getDeltaDLT( Int layerIdInVps, Int* piDLTInRef, UInt uiDLTInRefNum, Int* piDeltaDLTOut, UInt *puiDeltaDLTOutNum );
  Void    setDeltaDLT( Int layerIdInVps, Int* piDLTInRef, UInt uiDLTInRefNum, Int* piDeltaDLTIn, UInt uiDeltaDLTInNum );
#endif
};
#endif

class Window
{
private:
  Bool          m_enabledFlag;
  Int           m_winLeftOffset;
  Int           m_winRightOffset;
  Int           m_winTopOffset;
  Int           m_winBottomOffset;
#if H_MV
  Bool          m_scaledFlag; 
#endif
public:
  Window()
  : m_enabledFlag (false)
  , m_winLeftOffset     (0)
  , m_winRightOffset    (0)
  , m_winTopOffset      (0)
  , m_winBottomOffset   (0)
#if H_MV
  , m_scaledFlag(true)
#endif
  { }

  Bool          getWindowEnabledFlag() const      { return m_enabledFlag; }
  Void          resetWindow()                     { m_enabledFlag = false; m_winLeftOffset = m_winRightOffset = m_winTopOffset = m_winBottomOffset = 0; }
  Int           getWindowLeftOffset() const       { return m_enabledFlag ? m_winLeftOffset : 0; }
  Void          setWindowLeftOffset(Int val)      { m_winLeftOffset = val; m_enabledFlag = true; }
  Int           getWindowRightOffset() const      { return m_enabledFlag ? m_winRightOffset : 0; }
  Void          setWindowRightOffset(Int val)     { m_winRightOffset = val; m_enabledFlag = true; }
  Int           getWindowTopOffset() const        { return m_enabledFlag ? m_winTopOffset : 0; }
  Void          setWindowTopOffset(Int val)       { m_winTopOffset = val; m_enabledFlag = true; }
  Int           getWindowBottomOffset() const     { return m_enabledFlag ? m_winBottomOffset: 0; }
  Void          setWindowBottomOffset(Int val)    { m_winBottomOffset = val; m_enabledFlag = true; }

#if H_MV
  Void          setScaledFlag(Bool flag)          { m_scaledFlag = flag;  } 
  Bool          getScaledFlag() const             { return m_scaledFlag;  } 
  Void          scaleOffsets( Int scal )          
  {
    if (! m_scaledFlag )
    {
      m_scaledFlag         = true; 
      m_winLeftOffset     *= scal; 
      m_winRightOffset    *= scal; 
      m_winTopOffset      *= scal; 
      m_winBottomOffset   *= scal; 
    }
  }
#endif
  Void          setWindow(Int offsetLeft, Int offsetLRight, Int offsetLTop, Int offsetLBottom)
  {
    m_enabledFlag       = true;
    m_winLeftOffset     = offsetLeft;
    m_winRightOffset    = offsetLRight;
    m_winTopOffset      = offsetLTop;
    m_winBottomOffset   = offsetLBottom;
  }
};


class TComVUI
{
private:
  Bool m_aspectRatioInfoPresentFlag;
  Int  m_aspectRatioIdc;
  Int  m_sarWidth;
  Int  m_sarHeight;
  Bool m_overscanInfoPresentFlag;
  Bool m_overscanAppropriateFlag;
  Bool m_videoSignalTypePresentFlag;
  Int  m_videoFormat;
  Bool m_videoFullRangeFlag;
  Bool m_colourDescriptionPresentFlag;
  Int  m_colourPrimaries;
  Int  m_transferCharacteristics;
  Int  m_matrixCoefficients;
  Bool m_chromaLocInfoPresentFlag;
  Int  m_chromaSampleLocTypeTopField;
  Int  m_chromaSampleLocTypeBottomField;
  Bool m_neutralChromaIndicationFlag;
  Bool m_fieldSeqFlag;

  Window m_defaultDisplayWindow;
  Bool m_frameFieldInfoPresentFlag;
  Bool m_hrdParametersPresentFlag;
  Bool m_bitstreamRestrictionFlag;
  Bool m_tilesFixedStructureFlag;
  Bool m_motionVectorsOverPicBoundariesFlag;
  Bool m_restrictedRefPicListsFlag;
  Int  m_minSpatialSegmentationIdc;
  Int  m_maxBytesPerPicDenom;
  Int  m_maxBitsPerMinCuDenom;
  Int  m_log2MaxMvLengthHorizontal;
  Int  m_log2MaxMvLengthVertical;
  TComHRD m_hrdParameters;
  TimingInfo m_timingInfo;

public:
  TComVUI()
    :m_aspectRatioInfoPresentFlag(false)
    ,m_aspectRatioIdc(0)
    ,m_sarWidth(0)
    ,m_sarHeight(0)
    ,m_overscanInfoPresentFlag(false)
    ,m_overscanAppropriateFlag(false)
    ,m_videoSignalTypePresentFlag(false)
    ,m_videoFormat(5)
    ,m_videoFullRangeFlag(false)
    ,m_colourDescriptionPresentFlag(false)
    ,m_colourPrimaries(2)
    ,m_transferCharacteristics(2)
    ,m_matrixCoefficients(2)
    ,m_chromaLocInfoPresentFlag(false)
    ,m_chromaSampleLocTypeTopField(0)
    ,m_chromaSampleLocTypeBottomField(0)
    ,m_neutralChromaIndicationFlag(false)
    ,m_fieldSeqFlag(false)
    ,m_frameFieldInfoPresentFlag(false)
    ,m_hrdParametersPresentFlag(false)
    ,m_bitstreamRestrictionFlag(false)
    ,m_tilesFixedStructureFlag(false)
    ,m_motionVectorsOverPicBoundariesFlag(true)
    ,m_restrictedRefPicListsFlag(1)
    ,m_minSpatialSegmentationIdc(0)
    ,m_maxBytesPerPicDenom(2)
    ,m_maxBitsPerMinCuDenom(1)
    ,m_log2MaxMvLengthHorizontal(15)
    ,m_log2MaxMvLengthVertical(15)
  {}

  virtual ~TComVUI() {}

  Bool getAspectRatioInfoPresentFlag() { return m_aspectRatioInfoPresentFlag; }
  Void setAspectRatioInfoPresentFlag(Bool i) { m_aspectRatioInfoPresentFlag = i; }

  Int getAspectRatioIdc() { return m_aspectRatioIdc; }
  Void setAspectRatioIdc(Int i) { m_aspectRatioIdc = i; }

  Int getSarWidth() { return m_sarWidth; }
  Void setSarWidth(Int i) { m_sarWidth = i; }

  Int getSarHeight() { return m_sarHeight; }
  Void setSarHeight(Int i) { m_sarHeight = i; }

  Bool getOverscanInfoPresentFlag() { return m_overscanInfoPresentFlag; }
  Void setOverscanInfoPresentFlag(Bool i) { m_overscanInfoPresentFlag = i; }

  Bool getOverscanAppropriateFlag() { return m_overscanAppropriateFlag; }
  Void setOverscanAppropriateFlag(Bool i) { m_overscanAppropriateFlag = i; }

  Bool getVideoSignalTypePresentFlag() { return m_videoSignalTypePresentFlag; }
  Void setVideoSignalTypePresentFlag(Bool i) { m_videoSignalTypePresentFlag = i; }

  Int getVideoFormat() { return m_videoFormat; }
  Void setVideoFormat(Int i) { m_videoFormat = i; }

  Bool getVideoFullRangeFlag() { return m_videoFullRangeFlag; }
  Void setVideoFullRangeFlag(Bool i) { m_videoFullRangeFlag = i; }

  Bool getColourDescriptionPresentFlag() { return m_colourDescriptionPresentFlag; }
  Void setColourDescriptionPresentFlag(Bool i) { m_colourDescriptionPresentFlag = i; }

  Int getColourPrimaries() { return m_colourPrimaries; }
  Void setColourPrimaries(Int i) { m_colourPrimaries = i; }

  Int getTransferCharacteristics() { return m_transferCharacteristics; }
  Void setTransferCharacteristics(Int i) { m_transferCharacteristics = i; }

  Int getMatrixCoefficients() { return m_matrixCoefficients; }
  Void setMatrixCoefficients(Int i) { m_matrixCoefficients = i; }

  Bool getChromaLocInfoPresentFlag() { return m_chromaLocInfoPresentFlag; }
  Void setChromaLocInfoPresentFlag(Bool i) { m_chromaLocInfoPresentFlag = i; }

  Int getChromaSampleLocTypeTopField() { return m_chromaSampleLocTypeTopField; }
  Void setChromaSampleLocTypeTopField(Int i) { m_chromaSampleLocTypeTopField = i; }

  Int getChromaSampleLocTypeBottomField() { return m_chromaSampleLocTypeBottomField; }
  Void setChromaSampleLocTypeBottomField(Int i) { m_chromaSampleLocTypeBottomField = i; }

  Bool getNeutralChromaIndicationFlag() { return m_neutralChromaIndicationFlag; }
  Void setNeutralChromaIndicationFlag(Bool i) { m_neutralChromaIndicationFlag = i; }

  Bool getFieldSeqFlag() { return m_fieldSeqFlag; }
  Void setFieldSeqFlag(Bool i) { m_fieldSeqFlag = i; }

  Bool getFrameFieldInfoPresentFlag() { return m_frameFieldInfoPresentFlag; }
  Void setFrameFieldInfoPresentFlag(Bool i) { m_frameFieldInfoPresentFlag = i; }

  Window& getDefaultDisplayWindow()                              { return m_defaultDisplayWindow;                }
  Void    setDefaultDisplayWindow(Window& defaultDisplayWindow ) { m_defaultDisplayWindow = defaultDisplayWindow; }

  Bool getHrdParametersPresentFlag() { return m_hrdParametersPresentFlag; }
  Void setHrdParametersPresentFlag(Bool i) { m_hrdParametersPresentFlag = i; }

  Bool getBitstreamRestrictionFlag() { return m_bitstreamRestrictionFlag; }
  Void setBitstreamRestrictionFlag(Bool i) { m_bitstreamRestrictionFlag = i; }

  Bool getTilesFixedStructureFlag() { return m_tilesFixedStructureFlag; }
  Void setTilesFixedStructureFlag(Bool i) { m_tilesFixedStructureFlag = i; }

  Bool getMotionVectorsOverPicBoundariesFlag() { return m_motionVectorsOverPicBoundariesFlag; }
  Void setMotionVectorsOverPicBoundariesFlag(Bool i) { m_motionVectorsOverPicBoundariesFlag = i; }

  Bool getRestrictedRefPicListsFlag() { return m_restrictedRefPicListsFlag; }
  Void setRestrictedRefPicListsFlag(Bool b) { m_restrictedRefPicListsFlag = b; }

  Int getMinSpatialSegmentationIdc() { return m_minSpatialSegmentationIdc; }
  Void setMinSpatialSegmentationIdc(Int i) { m_minSpatialSegmentationIdc = i; }
  Int getMaxBytesPerPicDenom() { return m_maxBytesPerPicDenom; }
  Void setMaxBytesPerPicDenom(Int i) { m_maxBytesPerPicDenom = i; }

  Int getMaxBitsPerMinCuDenom() { return m_maxBitsPerMinCuDenom; }
  Void setMaxBitsPerMinCuDenom(Int i) { m_maxBitsPerMinCuDenom = i; }

  Int getLog2MaxMvLengthHorizontal() { return m_log2MaxMvLengthHorizontal; }
  Void setLog2MaxMvLengthHorizontal(Int i) { m_log2MaxMvLengthHorizontal = i; }

  Int getLog2MaxMvLengthVertical() { return m_log2MaxMvLengthVertical; }
  Void setLog2MaxMvLengthVertical(Int i) { m_log2MaxMvLengthVertical = i; }

  TComHRD* getHrdParameters                 ()             { return &m_hrdParameters; }
  TimingInfo* getTimingInfo() { return &m_timingInfo; }
#if H_MV
  Void inferVideoSignalInfo( TComVPS* vps, Int layerIdCurr );
#endif
};

/// SPS class
class TComSPS
{
private:
  Int         m_SPSId;
  Int         m_VPSId;
  Int         m_chromaFormatIdc;

  UInt        m_uiMaxTLayers;           // maximum number of temporal layers

  // Structure
  UInt        m_picWidthInLumaSamples;
  UInt        m_picHeightInLumaSamples;
  
  Int         m_log2MinCodingBlockSize;
  Int         m_log2DiffMaxMinCodingBlockSize;
  UInt        m_uiMaxCUWidth;
  UInt        m_uiMaxCUHeight;
  UInt        m_uiMaxCUDepth;

  Window      m_conformanceWindow;

  TComRPSList m_RPSList;
  Bool        m_bLongTermRefsPresent;
  Bool        m_TMVPFlagsPresent;
  Int         m_numReorderPics[MAX_TLAYER];
  
  // Tool list
  UInt        m_uiQuadtreeTULog2MaxSize;
  UInt        m_uiQuadtreeTULog2MinSize;
  UInt        m_uiQuadtreeTUMaxDepthInter;
  UInt        m_uiQuadtreeTUMaxDepthIntra;
  Bool        m_usePCM;
  UInt        m_pcmLog2MaxSize;
  UInt        m_uiPCMLog2MinSize;
  Bool        m_useAMP;

#if H_3D_QTLPC
  Bool        m_bUseQTL;
  Bool        m_bUsePC;
#endif
  // Parameter
  Int         m_bitDepthY;
  Int         m_bitDepthC;
  Int         m_qpBDOffsetY;
  Int         m_qpBDOffsetC;

  UInt        m_uiPCMBitDepthLuma;
  UInt        m_uiPCMBitDepthChroma;
  Bool        m_bPCMFilterDisableFlag;

  UInt        m_uiBitsForPOC;
  UInt        m_numLongTermRefPicSPS;
  UInt        m_ltRefPicPocLsbSps[33];
  Bool        m_usedByCurrPicLtSPSFlag[33];
  // Max physical transform size
  UInt        m_uiMaxTrSize;
  
  Int m_iAMPAcc[MAX_CU_DEPTH];
  Bool        m_bUseSAO; 

  Bool        m_bTemporalIdNestingFlag; // temporal_id_nesting_flag

  Bool        m_scalingListEnabledFlag;
  Bool        m_scalingListPresentFlag;
  TComScalingList*     m_scalingList;   //!< ScalingList class pointer
  UInt        m_uiMaxDecPicBuffering[MAX_TLAYER]; 
  UInt        m_uiMaxLatencyIncrease[MAX_TLAYER];  // Really max latency increase plus 1 (value 0 expresses no limit)

  Bool        m_useDF;
  Bool        m_useStrongIntraSmoothing;

  Bool        m_vuiParametersPresentFlag;
  TComVUI     m_vuiParameters;

  static const Int   m_winUnitX[MAX_CHROMA_FORMAT_IDC+1];
  static const Int   m_winUnitY[MAX_CHROMA_FORMAT_IDC+1];
  TComPTL     m_pcPTL;
#if H_MV
  TComVPS*    m_pcVPS; 
  // SPS 
  Bool        m_spsExtensionFlag; 
  Bool        m_spsExtensionTypeFlag[PS_EX_T_MAX_NUM];
  Bool        m_spsInferScalingListFlag;
  Int         m_spsScalingListRefLayerId;
  Bool        m_updateRepFormatFlag;
  Int         m_spsRepFormatIdx;
  // SPS Extension 
  Bool        m_interViewMvVertConstraintFlag;
  Int         m_numScaledRefLayerOffsets;
  Int         m_scaledRefLayerId          [MAX_NUM_SCALED_REF_LAYERS];
  Int         m_scaledRefLayerLeftOffset  [MAX_NUM_LAYERS];
  Int         m_scaledRefLayerTopOffset   [MAX_NUM_LAYERS];
  Int         m_scaledRefLayerRightOffset [MAX_NUM_LAYERS];
  Int         m_scaledRefLayerBottomOffset[MAX_NUM_LAYERS];
#endif
#if H_3D
  UInt        m_uiCamParPrecision;
  Bool        m_bCamParInSliceHeader;
  Int         m_aaiCodedScale [2][MAX_NUM_LAYERS];
  Int         m_aaiCodedOffset[2][MAX_NUM_LAYERS];
#endif
#if H_MV
  Int         m_layerId; 
#endif
public:
  TComSPS();
  virtual ~TComSPS();

  Int  getVPSId       ()         { return m_VPSId;          }
  Void setVPSId       (Int i)    { m_VPSId = i;             }
  Int  getSPSId       ()         { return m_SPSId;          }
  Void setSPSId       (Int i)    { m_SPSId = i;             }
  Int  getChromaFormatIdc ()         { return m_chromaFormatIdc;       }
  Void setChromaFormatIdc (Int i)    { m_chromaFormatIdc = i;          }

  static Int getWinUnitX (Int chromaFormatIdc) { assert (chromaFormatIdc > 0 && chromaFormatIdc <= MAX_CHROMA_FORMAT_IDC); return m_winUnitX[chromaFormatIdc];      }
  static Int getWinUnitY (Int chromaFormatIdc) { assert (chromaFormatIdc > 0 && chromaFormatIdc <= MAX_CHROMA_FORMAT_IDC); return m_winUnitY[chromaFormatIdc];      }
  
  // structure
  Void setPicWidthInLumaSamples       ( UInt u ) { m_picWidthInLumaSamples = u;        }
  UInt getPicWidthInLumaSamples       ()         { return  m_picWidthInLumaSamples;    }
  Void setPicHeightInLumaSamples      ( UInt u ) { m_picHeightInLumaSamples = u;       }
  UInt getPicHeightInLumaSamples      ()         { return  m_picHeightInLumaSamples;   }

  Window& getConformanceWindow()                           { return  m_conformanceWindow;             }
  Void    setConformanceWindow(Window& conformanceWindow ) { m_conformanceWindow = conformanceWindow; }

  UInt  getNumLongTermRefPicSPS()             { return m_numLongTermRefPicSPS; }
  Void  setNumLongTermRefPicSPS(UInt val)     { m_numLongTermRefPicSPS = val; }

  UInt  getLtRefPicPocLsbSps(UInt index)             { return m_ltRefPicPocLsbSps[index]; }
  Void  setLtRefPicPocLsbSps(UInt index, UInt val)     { m_ltRefPicPocLsbSps[index] = val; }

  Bool getUsedByCurrPicLtSPSFlag(Int i)        {return m_usedByCurrPicLtSPSFlag[i];}
  Void setUsedByCurrPicLtSPSFlag(Int i, Bool x)      { m_usedByCurrPicLtSPSFlag[i] = x;}

  Int  getLog2MinCodingBlockSize() const           { return m_log2MinCodingBlockSize; }
  Void setLog2MinCodingBlockSize(Int val)          { m_log2MinCodingBlockSize = val; }
  Int  getLog2DiffMaxMinCodingBlockSize() const    { return m_log2DiffMaxMinCodingBlockSize; }
  Void setLog2DiffMaxMinCodingBlockSize(Int val)   { m_log2DiffMaxMinCodingBlockSize = val; }

  Void setMaxCUWidth  ( UInt u ) { m_uiMaxCUWidth = u;      }
  UInt getMaxCUWidth  ()         { return  m_uiMaxCUWidth;  }
  Void setMaxCUHeight ( UInt u ) { m_uiMaxCUHeight = u;     }
  UInt getMaxCUHeight ()         { return  m_uiMaxCUHeight; }
  Void setMaxCUDepth  ( UInt u ) { m_uiMaxCUDepth = u;      }
  UInt getMaxCUDepth  ()         { return  m_uiMaxCUDepth;  }
  Void setUsePCM      ( Bool b ) { m_usePCM = b;           }
  Bool getUsePCM      ()         { return m_usePCM;        }
  Void setPCMLog2MaxSize  ( UInt u ) { m_pcmLog2MaxSize = u;      }
  UInt getPCMLog2MaxSize  ()         { return  m_pcmLog2MaxSize;  }
  Void setPCMLog2MinSize  ( UInt u ) { m_uiPCMLog2MinSize = u;      }
  UInt getPCMLog2MinSize  ()         { return  m_uiPCMLog2MinSize;  }
  Void setBitsForPOC  ( UInt u ) { m_uiBitsForPOC = u;      }
  UInt getBitsForPOC  ()         { return m_uiBitsForPOC;   }
  Bool getUseAMP() { return m_useAMP; }
  Void setUseAMP( Bool b ) { m_useAMP = b; }
  Void setQuadtreeTULog2MaxSize( UInt u ) { m_uiQuadtreeTULog2MaxSize = u;    }
  UInt getQuadtreeTULog2MaxSize()         { return m_uiQuadtreeTULog2MaxSize; }
  Void setQuadtreeTULog2MinSize( UInt u ) { m_uiQuadtreeTULog2MinSize = u;    }
  UInt getQuadtreeTULog2MinSize()         { return m_uiQuadtreeTULog2MinSize; }
  Void setQuadtreeTUMaxDepthInter( UInt u ) { m_uiQuadtreeTUMaxDepthInter = u;    }
  Void setQuadtreeTUMaxDepthIntra( UInt u ) { m_uiQuadtreeTUMaxDepthIntra = u;    }
  UInt getQuadtreeTUMaxDepthInter()         { return m_uiQuadtreeTUMaxDepthInter; }
  UInt getQuadtreeTUMaxDepthIntra()         { return m_uiQuadtreeTUMaxDepthIntra; }
  Void setNumReorderPics(Int i, UInt tlayer)              { m_numReorderPics[tlayer] = i;    }
  Int  getNumReorderPics(UInt tlayer)                     { return m_numReorderPics[tlayer]; }
  Void         createRPSList( Int numRPS );
  TComRPSList* getRPSList()                      { return &m_RPSList;          }
  Bool      getLongTermRefsPresent()         { return m_bLongTermRefsPresent; }
  Void      setLongTermRefsPresent(Bool b)   { m_bLongTermRefsPresent=b;      }
  Bool      getTMVPFlagsPresent()         { return m_TMVPFlagsPresent; }
  Void      setTMVPFlagsPresent(Bool b)   { m_TMVPFlagsPresent=b;      }  
  // physical transform
  Void setMaxTrSize   ( UInt u ) { m_uiMaxTrSize = u;       }
  UInt getMaxTrSize   ()         { return  m_uiMaxTrSize;   }
  
  // AMP accuracy
  Int       getAMPAcc   ( UInt uiDepth ) { return m_iAMPAcc[uiDepth]; }
  Void      setAMPAcc   ( UInt uiDepth, Int iAccu ) { assert( uiDepth < g_uiMaxCUDepth);  m_iAMPAcc[uiDepth] = iAccu; }

  // Bit-depth
  Int      getBitDepthY() { return m_bitDepthY; }
  Void     setBitDepthY(Int u) { m_bitDepthY = u; }
  Int      getBitDepthC() { return m_bitDepthC; }
  Void     setBitDepthC(Int u) { m_bitDepthC = u; }
  Int       getQpBDOffsetY  ()             { return m_qpBDOffsetY;   }
  Void      setQpBDOffsetY  ( Int value  ) { m_qpBDOffsetY = value;  }
  Int       getQpBDOffsetC  ()             { return m_qpBDOffsetC;   }
  Void      setQpBDOffsetC  ( Int value  ) { m_qpBDOffsetC = value;  }
  Void setUseSAO                  (Bool bVal)  {m_bUseSAO = bVal;}
  Bool getUseSAO                  ()           {return m_bUseSAO;}

  UInt      getMaxTLayers()                           { return m_uiMaxTLayers; }
  Void      setMaxTLayers( UInt uiMaxTLayers )        { assert( uiMaxTLayers <= MAX_TLAYER ); m_uiMaxTLayers = uiMaxTLayers; }

  Bool      getTemporalIdNestingFlag()                { return m_bTemporalIdNestingFlag; }
  Void      setTemporalIdNestingFlag( Bool bValue )   { m_bTemporalIdNestingFlag = bValue; }
  UInt      getPCMBitDepthLuma     ()         { return m_uiPCMBitDepthLuma;     }
  Void      setPCMBitDepthLuma     ( UInt u ) { m_uiPCMBitDepthLuma = u;        }
  UInt      getPCMBitDepthChroma   ()         { return m_uiPCMBitDepthChroma;   }
  Void      setPCMBitDepthChroma   ( UInt u ) { m_uiPCMBitDepthChroma = u;      }
  Void      setPCMFilterDisableFlag     ( Bool   bValue  )    { m_bPCMFilterDisableFlag = bValue; }
  Bool      getPCMFilterDisableFlag     ()                    { return m_bPCMFilterDisableFlag;   } 

  Bool getScalingListFlag       ()         { return m_scalingListEnabledFlag;     }
  Void setScalingListFlag       ( Bool b ) { m_scalingListEnabledFlag  = b;       }
  Bool getScalingListPresentFlag()         { return m_scalingListPresentFlag;     }
  Void setScalingListPresentFlag( Bool b ) { m_scalingListPresentFlag  = b;       }
  Void setScalingList      ( TComScalingList *scalingList);
  TComScalingList* getScalingList ()       { return m_scalingList; }               //!< get ScalingList class pointer in SPS
  UInt getMaxDecPicBuffering  (UInt tlayer)            { return m_uiMaxDecPicBuffering[tlayer]; }
  Void setMaxDecPicBuffering  ( UInt ui, UInt tlayer ) { assert(tlayer < MAX_TLAYER);  m_uiMaxDecPicBuffering[tlayer] = ui;   }
  UInt getMaxLatencyIncrease  (UInt tlayer)            { return m_uiMaxLatencyIncrease[tlayer];   }
  Void setMaxLatencyIncrease  ( UInt ui , UInt tlayer) { m_uiMaxLatencyIncrease[tlayer] = ui;      }

  Void setUseStrongIntraSmoothing (Bool bVal)  {m_useStrongIntraSmoothing = bVal;}
  Bool getUseStrongIntraSmoothing ()           {return m_useStrongIntraSmoothing;}

  Bool getVuiParametersPresentFlag() { return m_vuiParametersPresentFlag; }
  Void setVuiParametersPresentFlag(Bool b) { m_vuiParametersPresentFlag = b; }
  TComVUI* getVuiParameters() { return &m_vuiParameters; }
  Void setHrdParameters( UInt frameRate, UInt numDU, UInt bitRate, Bool randomAccess );

  TComPTL* getPTL()     { return &m_pcPTL; }
#if H_MV
  Void setSpsExtensionFlag( Bool flag ) { m_spsExtensionFlag = flag; } 
  Bool getSpsExtensionFlag( )  { return m_spsExtensionFlag; }

  Void setSpsExtensionTypeFlag( Int i, Bool flag ) { m_spsExtensionTypeFlag[i] = flag; } 
  Bool getSpsExtensionTypeFlag( Int i ) { return m_spsExtensionTypeFlag[i]; }
  Void      setVPS          ( TComVPS* pcVPS ) { m_pcVPS = pcVPS; }
  TComVPS*  getVPS          () { return m_pcVPS; }

  Void setSpsInferScalingListFlag( Bool flag ) { m_spsInferScalingListFlag = flag; } 
  Bool getSpsInferScalingListFlag(  )          { return m_spsInferScalingListFlag; } 

  Void setSpsScalingListRefLayerId( Int  val ) { m_spsScalingListRefLayerId = val; } 
  Int  getSpsScalingListRefLayerId(  )         { return m_spsScalingListRefLayerId; } 

  Void setUpdateRepFormatFlag( Bool flag )     { m_updateRepFormatFlag = flag; } 
  Bool getUpdateRepFormatFlag(  )              { return m_updateRepFormatFlag; } 
  Void setSpsRepFormatIdx( Int  val )          { m_spsRepFormatIdx = val; } 
  Int  getSpsRepFormatIdx(  )                  { return m_spsRepFormatIdx; } 
  // SPS Extension 
  Void setInterViewMvVertConstraintFlag(Bool val) { m_interViewMvVertConstraintFlag = val; }
  Bool getInterViewMvVertConstraintFlag()         { return m_interViewMvVertConstraintFlag;}

  Void setNumScaledRefLayerOffsets( Int  val )    { m_numScaledRefLayerOffsets = val; } 
  Int  getNumScaledRefLayerOffsets(  )            { return m_numScaledRefLayerOffsets; } 

  Void setScaledRefLayerId( Int i, Int  val )     { m_scaledRefLayerId[i] = val; } 
  Int  getScaledRefLayerId( Int i )               { return m_scaledRefLayerId[i]; } 

  Void setScaledRefLayerLeftOffset( Int j, Int  val ) { m_scaledRefLayerLeftOffset[j] = val; } 
  Int  getScaledRefLayerLeftOffset( Int j )           { return m_scaledRefLayerLeftOffset[j]; } 

  Void setScaledRefLayerTopOffset( Int j, Int  val )  { m_scaledRefLayerTopOffset[j] = val; } 
  Int  getScaledRefLayerTopOffset( Int j )            { return m_scaledRefLayerTopOffset[j]; } 

  Void setScaledRefLayerRightOffset( Int j, Int  val ) { m_scaledRefLayerRightOffset[j] = val; } 
  Int  getScaledRefLayerRightOffset( Int j )           { return m_scaledRefLayerRightOffset[j]; } 

  Void setScaledRefLayerBottomOffset( Int j, Int  val ) { m_scaledRefLayerBottomOffset[j] = val; } 
  Int  getScaledRefLayerBottomOffset( Int j )           { return m_scaledRefLayerBottomOffset[j]; } 
  // Inference 
  Void inferRepFormat( TComVPS* vps, Int layerIdCurr );

  Void inferScalingList( TComSPS* spsSrc );
#endif
#if H_3D_QTLPC
  Void setUseQTL( Bool b ) { m_bUseQTL = b;    }
  Bool getUseQTL()         { return m_bUseQTL; }
  Void setUsePC ( Bool b ) { m_bUsePC  = b;    }
  Bool getUsePC ()         { return m_bUsePC;  }
#endif
#if H_MV
  Int  getLayerId            ()           { return m_layerId; }
  Void setLayerId            ( Int val )  { m_layerId = val; }
#endif

};

/// Reference Picture Lists class
class TComRefPicListModification
{
private:
  UInt      m_bRefPicListModificationFlagL0;  
  UInt      m_bRefPicListModificationFlagL1;  
  UInt      m_RefPicSetIdxL0[32];
  UInt      m_RefPicSetIdxL1[32];
    
public:
  TComRefPicListModification();
  virtual ~TComRefPicListModification();
  
  Void  create                    ();
  Void  destroy                   ();

  Bool       getRefPicListModificationFlagL0() { return m_bRefPicListModificationFlagL0; }
  Void       setRefPicListModificationFlagL0(Bool flag) { m_bRefPicListModificationFlagL0 = flag; }
  Bool       getRefPicListModificationFlagL1() { return m_bRefPicListModificationFlagL1; }
  Void       setRefPicListModificationFlagL1(Bool flag) { m_bRefPicListModificationFlagL1 = flag; }
  Void       setRefPicSetIdxL0(UInt idx, UInt refPicSetIdx) { m_RefPicSetIdxL0[idx] = refPicSetIdx; }
  UInt       getRefPicSetIdxL0(UInt idx) { return m_RefPicSetIdxL0[idx]; }
  Void       setRefPicSetIdxL1(UInt idx, UInt refPicSetIdx) { m_RefPicSetIdxL1[idx] = refPicSetIdx; }
  UInt       getRefPicSetIdxL1(UInt idx) { return m_RefPicSetIdxL1[idx]; }
#if H_MV
  // Why not an listIdx for all members, would avoid code duplication?? 
  Void       setRefPicSetIdxL(UInt li, UInt idx, UInt refPicSetIdx) {( li==0 ? m_RefPicSetIdxL0[idx] : m_RefPicSetIdxL1[idx] ) = refPicSetIdx; }
  UInt       getRefPicSetIdxL(UInt li, UInt idx ) { return ( li == 0 ) ? m_RefPicSetIdxL0[idx] : m_RefPicSetIdxL1[idx] ; }
  Void       setRefPicListModificationFlagL(UInt li, Bool flag) { ( li==0  ? m_bRefPicListModificationFlagL0 : m_bRefPicListModificationFlagL1 ) = flag;  }
  Bool       getRefPicListModificationFlagL(UInt li ) { return ( li== 0) ? m_bRefPicListModificationFlagL0 : m_bRefPicListModificationFlagL1; }
#endif
};

/// PPS class
class TComPPS
{
private:
  Int         m_PPSId;                    // pic_parameter_set_id
  Int         m_SPSId;                    // seq_parameter_set_id
  Int         m_picInitQPMinus26;
  Bool        m_useDQP;
  Bool        m_bConstrainedIntraPred;    // constrained_intra_pred_flag
  Bool        m_bSliceChromaQpFlag;       // slicelevel_chroma_qp_flag

  // access channel
  TComSPS*    m_pcSPS;
  UInt        m_uiMaxCuDQPDepth;
  UInt        m_uiMinCuDQPSize;

  Int         m_chromaCbQpOffset;
  Int         m_chromaCrQpOffset;

  UInt        m_numRefIdxL0DefaultActive;
  UInt        m_numRefIdxL1DefaultActive;

  Bool        m_bUseWeightPred;           // Use of Weighting Prediction (P_SLICE)
  Bool        m_useWeightedBiPred;        // Use of Weighting Bi-Prediction (B_SLICE)
  Bool        m_OutputFlagPresentFlag;   // Indicates the presence of output_flag in slice header

  Bool        m_TransquantBypassEnableFlag; // Indicates presence of cu_transquant_bypass_flag in CUs.
  Bool        m_useTransformSkip;
  Bool        m_dependentSliceSegmentsEnabledFlag;     //!< Indicates the presence of dependent slices
  Bool        m_tilesEnabledFlag;              //!< Indicates the presence of tiles
  Bool        m_entropyCodingSyncEnabledFlag;  //!< Indicates the presence of wavefronts
  
  Bool     m_loopFilterAcrossTilesEnabledFlag;
  Int      m_uniformSpacingFlag;
  Int      m_iNumColumnsMinus1;
  UInt*    m_puiColumnWidth;
  Int      m_iNumRowsMinus1;
  UInt*    m_puiRowHeight;

  Int      m_iNumSubstreams;

  Int      m_signHideFlag;

  Bool     m_cabacInitPresentFlag;
  UInt     m_encCABACTableIdx;           // Used to transmit table selection across slices

  Bool     m_sliceHeaderExtensionPresentFlag;
  Bool     m_loopFilterAcrossSlicesEnabledFlag;
  Bool     m_deblockingFilterControlPresentFlag;
  Bool     m_deblockingFilterOverrideEnabledFlag;
  Bool     m_picDisableDeblockingFilterFlag;
  Int      m_deblockingFilterBetaOffsetDiv2;    //< beta offset for deblocking filter
  Int      m_deblockingFilterTcOffsetDiv2;      //< tc offset for deblocking filter
  Bool     m_scalingListPresentFlag;
  TComScalingList*     m_scalingList;   //!< ScalingList class pointer
  Bool m_listsModificationPresentFlag;
  UInt m_log2ParallelMergeLevelMinus2;
  Int m_numExtraSliceHeaderBits;

#if H_MV
  Int  m_layerId; 
  Bool m_ppsInferScalingListFlag;
  Int  m_ppsScalingListRefLayerId;

#if H_MV_HLS_7_GEN_P0166_PPS_EXTENSION
  Bool m_ppsExtensionTypeFlag[PS_EX_T_MAX_NUM];
#endif
#if H_MV_HLS_7_POC_P0041
  Bool m_pocResetInfoPresentFlag;
#endif
#endif

#if H_3D
  TComDLT*  m_pcDLT; 
#endif

public:
  TComPPS();
  virtual ~TComPPS();
  
  Int       getPPSId ()      { return m_PPSId; }
  Void      setPPSId (Int i) { m_PPSId = i; }
  Int       getSPSId ()      { return m_SPSId; }
  Void      setSPSId (Int i) { m_SPSId = i; }
  
  Int       getPicInitQPMinus26 ()         { return  m_picInitQPMinus26; }
  Void      setPicInitQPMinus26 ( Int i )  { m_picInitQPMinus26 = i;     }
  Bool      getUseDQP ()                   { return m_useDQP;        }
  Void      setUseDQP ( Bool b )           { m_useDQP   = b;         }
  Bool      getConstrainedIntraPred ()         { return  m_bConstrainedIntraPred; }
  Void      setConstrainedIntraPred ( Bool b ) { m_bConstrainedIntraPred = b;     }
  Bool      getSliceChromaQpFlag ()         { return  m_bSliceChromaQpFlag; }
  Void      setSliceChromaQpFlag ( Bool b ) { m_bSliceChromaQpFlag = b;     }

  Void      setSPS              ( TComSPS* pcSPS ) { m_pcSPS = pcSPS; }
  TComSPS*  getSPS              ()         { return m_pcSPS;          }
  Void      setMaxCuDQPDepth    ( UInt u ) { m_uiMaxCuDQPDepth = u;   }
  UInt      getMaxCuDQPDepth    ()         { return m_uiMaxCuDQPDepth;}
  Void      setMinCuDQPSize     ( UInt u ) { m_uiMinCuDQPSize = u;    }
  UInt      getMinCuDQPSize     ()         { return m_uiMinCuDQPSize; }

#if H_3D
  Void      setDLT              ( TComDLT* pcDLT ) { m_pcDLT = pcDLT; }
  TComDLT*  getDLT              ()                 { return m_pcDLT; }
#endif

  Void      setChromaCbQpOffset( Int i ) { m_chromaCbQpOffset = i;    }
  Int       getChromaCbQpOffset()        { return m_chromaCbQpOffset; }
  Void      setChromaCrQpOffset( Int i ) { m_chromaCrQpOffset = i;    }
  Int       getChromaCrQpOffset()        { return m_chromaCrQpOffset; }

  Void      setNumRefIdxL0DefaultActive(UInt ui)    { m_numRefIdxL0DefaultActive=ui;     }
  UInt      getNumRefIdxL0DefaultActive()           { return m_numRefIdxL0DefaultActive; }
  Void      setNumRefIdxL1DefaultActive(UInt ui)    { m_numRefIdxL1DefaultActive=ui;     }
  UInt      getNumRefIdxL1DefaultActive()           { return m_numRefIdxL1DefaultActive; }

  Bool getUseWP                     ()          { return m_bUseWeightPred;  }
  Bool getWPBiPred                  ()          { return m_useWeightedBiPred;     }
  Void setUseWP                     ( Bool b )  { m_bUseWeightPred = b;     }
  Void setWPBiPred                  ( Bool b )  { m_useWeightedBiPred = b;  }
  Void      setOutputFlagPresentFlag( Bool b )  { m_OutputFlagPresentFlag = b;    }
  Bool      getOutputFlagPresentFlag()          { return m_OutputFlagPresentFlag; }
  Void      setTransquantBypassEnableFlag( Bool b ) { m_TransquantBypassEnableFlag = b; }
  Bool      getTransquantBypassEnableFlag()         { return m_TransquantBypassEnableFlag; }

  Bool      getUseTransformSkip       ()         { return m_useTransformSkip;     }
  Void      setUseTransformSkip       ( Bool b ) { m_useTransformSkip  = b;       }

  Void    setLoopFilterAcrossTilesEnabledFlag  (Bool b)    { m_loopFilterAcrossTilesEnabledFlag = b; }
  Bool    getLoopFilterAcrossTilesEnabledFlag  ()          { return m_loopFilterAcrossTilesEnabledFlag;   }
  Bool    getDependentSliceSegmentsEnabledFlag() const     { return m_dependentSliceSegmentsEnabledFlag; }
  Void    setDependentSliceSegmentsEnabledFlag(Bool val)   { m_dependentSliceSegmentsEnabledFlag = val; }
  Bool    getTilesEnabledFlag() const                      { return m_tilesEnabledFlag; }
  Void    setTilesEnabledFlag(Bool val)                    { m_tilesEnabledFlag = val; }
  Bool    getEntropyCodingSyncEnabledFlag() const          { return m_entropyCodingSyncEnabledFlag; }
  Void    setEntropyCodingSyncEnabledFlag(Bool val)        { m_entropyCodingSyncEnabledFlag = val; }
  Void     setUniformSpacingFlag            ( Bool b )          { m_uniformSpacingFlag = b; }
  Bool     getUniformSpacingFlag            ()                  { return m_uniformSpacingFlag; }
  Void     setNumColumnsMinus1              ( Int i )           { m_iNumColumnsMinus1 = i; }
  Int      getNumColumnsMinus1              ()                  { return m_iNumColumnsMinus1; }
  Void     setColumnWidth ( UInt* columnWidth )
  {
    if( m_uniformSpacingFlag == 0 && m_iNumColumnsMinus1 > 0 )
    {
      m_puiColumnWidth = new UInt[ m_iNumColumnsMinus1 ];

      for(Int i=0; i<m_iNumColumnsMinus1; i++)
      {
        m_puiColumnWidth[i] = columnWidth[i];
      }
    }
  }
  UInt     getColumnWidth  (UInt columnIdx) { return *( m_puiColumnWidth + columnIdx ); }
  Void     setNumRowsMinus1( Int i )        { m_iNumRowsMinus1 = i; }
  Int      getNumRowsMinus1()               { return m_iNumRowsMinus1; }
  Void     setRowHeight    ( UInt* rowHeight )
  {
    if( m_uniformSpacingFlag == 0 && m_iNumRowsMinus1 > 0 )
    {
      m_puiRowHeight = new UInt[ m_iNumRowsMinus1 ];

      for(Int i=0; i<m_iNumRowsMinus1; i++)
      {
        m_puiRowHeight[i] = rowHeight[i];
      }
    }
  }
  UInt     getRowHeight           (UInt rowIdx)    { return *( m_puiRowHeight + rowIdx ); }
  Void     setNumSubstreams(Int iNumSubstreams)               { m_iNumSubstreams = iNumSubstreams; }
  Int      getNumSubstreams()                                 { return m_iNumSubstreams; }

  Void      setSignHideFlag( Int signHideFlag ) { m_signHideFlag = signHideFlag; }
  Int       getSignHideFlag()                    { return m_signHideFlag; }

  Void     setCabacInitPresentFlag( Bool flag )     { m_cabacInitPresentFlag = flag;    }
  Void     setEncCABACTableIdx( Int idx )           { m_encCABACTableIdx = idx;         }
  Bool     getCabacInitPresentFlag()                { return m_cabacInitPresentFlag;    }
  UInt     getEncCABACTableIdx()                    { return m_encCABACTableIdx;        }
  Void     setDeblockingFilterControlPresentFlag( Bool val )  { m_deblockingFilterControlPresentFlag = val; }
  Bool     getDeblockingFilterControlPresentFlag()            { return m_deblockingFilterControlPresentFlag; }
  Void     setDeblockingFilterOverrideEnabledFlag( Bool val ) { m_deblockingFilterOverrideEnabledFlag = val; }
  Bool     getDeblockingFilterOverrideEnabledFlag()           { return m_deblockingFilterOverrideEnabledFlag; }
  Void     setPicDisableDeblockingFilterFlag(Bool val)        { m_picDisableDeblockingFilterFlag = val; }       //!< set offset for deblocking filter disabled
  Bool     getPicDisableDeblockingFilterFlag()                { return m_picDisableDeblockingFilterFlag; }      //!< get offset for deblocking filter disabled
  Void     setDeblockingFilterBetaOffsetDiv2(Int val)         { m_deblockingFilterBetaOffsetDiv2 = val; }       //!< set beta offset for deblocking filter
  Int      getDeblockingFilterBetaOffsetDiv2()                { return m_deblockingFilterBetaOffsetDiv2; }      //!< get beta offset for deblocking filter
  Void     setDeblockingFilterTcOffsetDiv2(Int val)           { m_deblockingFilterTcOffsetDiv2 = val; }               //!< set tc offset for deblocking filter
  Int      getDeblockingFilterTcOffsetDiv2()                  { return m_deblockingFilterTcOffsetDiv2; }              //!< get tc offset for deblocking filter
  Bool     getScalingListPresentFlag()         { return m_scalingListPresentFlag;     }
  Void     setScalingListPresentFlag( Bool b ) { m_scalingListPresentFlag  = b;       }
  Void     setScalingList      ( TComScalingList *scalingList);
  TComScalingList* getScalingList ()          { return m_scalingList; }         //!< get ScalingList class pointer in PPS
  Bool getListsModificationPresentFlag ()          { return m_listsModificationPresentFlag; }
  Void setListsModificationPresentFlag ( Bool b )  { m_listsModificationPresentFlag = b;    }
  UInt getLog2ParallelMergeLevelMinus2      ()                    { return m_log2ParallelMergeLevelMinus2; }
  Void setLog2ParallelMergeLevelMinus2      (UInt mrgLevel)       { m_log2ParallelMergeLevelMinus2 = mrgLevel; }
  Int getNumExtraSliceHeaderBits() { return m_numExtraSliceHeaderBits; }
  Void setNumExtraSliceHeaderBits(Int i) { m_numExtraSliceHeaderBits = i; }
  Void      setLoopFilterAcrossSlicesEnabledFlag ( Bool   bValue  )    { m_loopFilterAcrossSlicesEnabledFlag = bValue; }
  Bool      getLoopFilterAcrossSlicesEnabledFlag ()                    { return m_loopFilterAcrossSlicesEnabledFlag;   } 
  Bool getSliceHeaderExtensionPresentFlag   ()                    { return m_sliceHeaderExtensionPresentFlag; }
  Void setSliceHeaderExtensionPresentFlag   (Bool val)            { m_sliceHeaderExtensionPresentFlag = val; }
#if H_MV
  Void setLayerId( Int  val ) { m_layerId = val; } 
  Int  getLayerId(  ) { return m_layerId; } 

  Void setPpsInferScalingListFlag( Bool flag ) { m_ppsInferScalingListFlag = flag; } 
  Bool getPpsInferScalingListFlag(  ) { return m_ppsInferScalingListFlag; } 

  Void setPpsScalingListRefLayerId( Int  val ) { m_ppsScalingListRefLayerId = val; } 
  Int  getPpsScalingListRefLayerId(  ) { return m_ppsScalingListRefLayerId; } 

#if H_MV_HLS_7_GEN_P0166_PPS_EXTENSION
  Void setPpsExtensionTypeFlag( Int i, Bool flag ) { m_ppsExtensionTypeFlag[i] = flag; } 
  Bool getPpsExtensionTypeFlag( Int i ) { return m_ppsExtensionTypeFlag[i]; } 
#endif

#if H_MV_HLS_7_POC_P0041
  Void setPocResetInfoPresentFlag( Bool flag ) { m_pocResetInfoPresentFlag = flag; } 
  Bool getPocResetInfoPresentFlag(  ) { return m_pocResetInfoPresentFlag; } 
#endif

#endif
};

typedef struct
{
  // Explicit weighted prediction parameters parsed in slice header,
  // or Implicit weighted prediction parameters (8 bits depth values).
  Bool        bPresentFlag;
  UInt        uiLog2WeightDenom;
  Int         iWeight;
  Int         iOffset;

  // Weighted prediction scaling values built from above parameters (bitdepth scaled):
  Int         w, o, offset, shift, round;
} wpScalingParam;

typedef struct
{
  Int64 iAC;
  Int64 iDC;
} wpACDCParam;

/// slice header class
class TComSlice
{
  
private:
  //  Bitstream writing
  Bool       m_saoEnabledFlag;
  Bool       m_saoEnabledFlagChroma;      ///< SAO Cb&Cr enabled flag
  Int         m_iPPSId;               ///< picture parameter set ID
  Bool        m_PicOutputFlag;        ///< pic_output_flag 
  Int         m_iPOC;
#if H_MV
  Int         m_iPOCBeforeReset; 
#endif
  Int         m_iLastIDR;
  Int         m_iAssociatedIRAP;
  NalUnitType m_iAssociatedIRAPType;
  static Int  m_prevTid0POC;
  TComReferencePictureSet *m_pcRPS;
  TComReferencePictureSet m_LocalRPS;
  Int         m_iBDidx; 
  TComRefPicListModification m_RefPicListModification;
  NalUnitType m_eNalUnitType;         ///< Nal unit type for the slice
  SliceType   m_eSliceType;
  Int         m_iSliceQp;
  Bool        m_dependentSliceSegmentFlag;
#if ADAPTIVE_QP_SELECTION
  Int         m_iSliceQpBase;
#endif
  Bool        m_deblockingFilterDisable;
  Bool        m_deblockingFilterOverrideFlag;      //< offsets for deblocking filter inherit from PPS
  Int         m_deblockingFilterBetaOffsetDiv2;    //< beta offset for deblocking filter
  Int         m_deblockingFilterTcOffsetDiv2;      //< tc offset for deblocking filter
  Int         m_list1IdxToList0Idx[MAX_NUM_REF];
  Int         m_aiNumRefIdx   [2];    //  for multiple reference of current slice

  Bool        m_bCheckLDC;

  //  Data
  Int         m_iSliceQpDelta;
  Int         m_iSliceQpDeltaCb;
  Int         m_iSliceQpDeltaCr;
  TComPic*    m_apcRefPicList [2][MAX_NUM_REF+1];
  Int         m_aiRefPOCList  [2][MAX_NUM_REF+1];
#if H_MV
  Int         m_aiRefLayerIdList[2][MAX_NUM_REF+1];
#endif
  Bool        m_bIsUsedAsLongTerm[2][MAX_NUM_REF+1];
  Int         m_iDepth;
  
  // referenced slice?
  Bool        m_bRefenced;
  
  // access channel
  TComVPS*    m_pcVPS;
  TComSPS*    m_pcSPS;
  TComPPS*    m_pcPPS;
  TComPic*    m_pcPic;
#if ADAPTIVE_QP_SELECTION
  TComTrQuant* m_pcTrQuant;
#endif  
  UInt        m_colFromL0Flag;  // collocated picture from List0 flag
  
  UInt        m_colRefIdx;
  UInt        m_maxNumMergeCand;

  Double      m_lambdas[3];

  Bool        m_abEqualRef  [2][MAX_NUM_REF][MAX_NUM_REF];
  UInt        m_uiTLayer;
  Bool        m_bTLayerSwitchingFlag;

  UInt        m_sliceMode;
  UInt        m_sliceArgument;
  UInt        m_sliceCurStartCUAddr;
  UInt        m_sliceCurEndCUAddr;
  UInt        m_sliceIdx;
  UInt        m_sliceSegmentMode;
  UInt        m_sliceSegmentArgument;
  UInt        m_sliceSegmentCurStartCUAddr;
  UInt        m_sliceSegmentCurEndCUAddr;
  Bool        m_nextSlice;
  Bool        m_nextSliceSegment;
  UInt        m_sliceBits;
  UInt        m_sliceSegmentBits;
  Bool        m_bFinalized;

  wpScalingParam  m_weightPredTable[2][MAX_NUM_REF][3]; // [REF_PIC_LIST_0 or REF_PIC_LIST_1][refIdx][0:Y, 1:U, 2:V]
  wpACDCParam    m_weightACDCParam[3];                 // [0:Y, 1:U, 2:V]

  std::vector<UInt> m_tileByteLocation;
  UInt        m_uiTileOffstForMultES;

  UInt*       m_puiSubstreamSizes;
  TComScalingList*     m_scalingList;                 //!< pointer of quantization matrix
  Bool        m_cabacInitFlag; 

  Bool       m_bLMvdL1Zero;
  Int         m_numEntryPointOffsets;
  Bool       m_temporalLayerNonReferenceFlag;
  Bool       m_LFCrossSliceBoundaryFlag;

  Bool       m_enableTMVPFlag;
#if H_MV
  std::vector<TComPic*>* m_refPicSetInterLayer0; 
  std::vector<TComPic*>* m_refPicSetInterLayer1; 
  Int        m_layerId; 
  Int        m_viewId;
  Int        m_viewIndex; 
#if H_3D
  Bool       m_isDepth;
#endif

// Additional slice header syntax elements 
#if !H_MV_HLS7_GEN
  Bool       m_pocResetFlag; 
#endif
  Bool       m_crossLayerBlaFlag;
  Bool       m_discardableFlag;
  Bool       m_interLayerPredEnabledFlag;
  Int        m_numInterLayerRefPicsMinus1;
  Int        m_interLayerPredLayerIdc       [MAX_NUM_LAYERS];

#if H_MV_HLS_7_POC_P0041
  Int        m_sliceSegmentHeaderExtensionLength;
  Int        m_pocResetIdc;
  Int        m_pocResetPeriodId;
  Bool       m_fullPocResetFlag;
  Int        m_pocLsbVal;
  Bool       m_pocMsbValPresentFlag;
  Int        m_pocMsbVal;
  Bool       m_pocMsbValRequiredFlag;
#endif

#if H_3D
  Int        m_aaiCodedScale [2][MAX_NUM_LAYERS];
  Int        m_aaiCodedOffset[2][MAX_NUM_LAYERS];
#endif
#if H_3D_TMVP
  Int        m_aiAlterRefIdx   [2]; 
#endif
#if H_3D_ARP
  Bool m_arpRefPicAvailable[2][MAX_NUM_LAYERS];
  TComList<TComPic*> * m_pBaseViewRefPicList[MAX_NUM_LAYERS];
  UInt                 m_nARPStepNum; 
  Int         m_aiFirstTRefIdx    [2]; 
#endif
#if H_3D_IC
  Bool      m_bApplyIC;
  Bool      m_icSkipParseFlag;
#endif
#if H_3D
  TComPic*   m_ivPicsCurrPoc [2][MAX_NUM_LAYERS];  
  Int**      m_depthToDisparityB; 
  Int**      m_depthToDisparityF; 
#endif
#endif

#if H_3D_DDD
  Int          m_aiDDDInvScale [MAX_NUM_LAYERS];
  Int          m_aiDDDInvOffset[MAX_NUM_LAYERS];
  UInt         m_aiDDDShift    [MAX_NUM_LAYERS];
#endif

public:
  TComSlice();
  virtual ~TComSlice(); 
  Void      initSlice       ();

  Void      setVPS          ( TComVPS* pcVPS ) { m_pcVPS = pcVPS; }
  TComVPS*  getVPS          () { return m_pcVPS; }
  Void      setSPS          ( TComSPS* pcSPS ) { m_pcSPS = pcSPS; }
  TComSPS*  getSPS          () { return m_pcSPS; }
  
  Void      setPPS          ( TComPPS* pcPPS )         { assert(pcPPS!=NULL); m_pcPPS = pcPPS; m_iPPSId = pcPPS->getPPSId(); }
  TComPPS*  getPPS          () { return m_pcPPS; }

#if ADAPTIVE_QP_SELECTION
  Void          setTrQuant          ( TComTrQuant* pcTrQuant ) { m_pcTrQuant = pcTrQuant; }
  TComTrQuant*  getTrQuant          () { return m_pcTrQuant; }
#endif

  Void      setPPSId        ( Int PPSId )         { m_iPPSId = PPSId; }
  Int       getPPSId        () { return m_iPPSId; }
  Void      setPicOutputFlag( Bool b )         { m_PicOutputFlag = b;    }
  Bool      getPicOutputFlag()                 { return m_PicOutputFlag; }
  Void      setSaoEnabledFlag(Bool s) {m_saoEnabledFlag =s; }
  Bool      getSaoEnabledFlag() { return m_saoEnabledFlag; }
  Void      setSaoEnabledFlagChroma(Bool s) {m_saoEnabledFlagChroma =s; }       //!< set SAO Cb&Cr enabled flag
  Bool      getSaoEnabledFlagChroma() { return m_saoEnabledFlagChroma; }        //!< get SAO Cb&Cr enabled flag
  Void      setRPS          ( TComReferencePictureSet *pcRPS ) { m_pcRPS = pcRPS; }
  TComReferencePictureSet*  getRPS          () { return m_pcRPS; }
  TComReferencePictureSet*  getLocalRPS     () { return &m_LocalRPS; }

  Void      setRPSidx          ( Int iBDidx ) { m_iBDidx = iBDidx; }
  Int       getRPSidx          () { return m_iBDidx; }
  Int       getPrevTid0POC      ()                        { return  m_prevTid0POC;       }
  TComRefPicListModification* getRefPicListModification() { return &m_RefPicListModification; }
  Void      setLastIDR(Int iIDRPOC)                       { m_iLastIDR = iIDRPOC; }
  Int       getLastIDR()                                  { return m_iLastIDR; }
  Void      setAssociatedIRAPPOC(Int iAssociatedIRAPPOC)             { m_iAssociatedIRAP = iAssociatedIRAPPOC; }
  Int       getAssociatedIRAPPOC()                        { return m_iAssociatedIRAP; }
  Void      setAssociatedIRAPType(NalUnitType associatedIRAPType)    { m_iAssociatedIRAPType = associatedIRAPType; }
  NalUnitType getAssociatedIRAPType()                        { return m_iAssociatedIRAPType; }
  SliceType getSliceType    ()                          { return  m_eSliceType;         }
  Int       getPOC          ()                          { return  m_iPOC;           }
  Int       getSliceQp      ()                          { return  m_iSliceQp;           }
  Bool      getDependentSliceSegmentFlag() const        { return m_dependentSliceSegmentFlag; }
  void      setDependentSliceSegmentFlag(Bool val)      { m_dependentSliceSegmentFlag = val; }
#if ADAPTIVE_QP_SELECTION
  Int       getSliceQpBase  ()                          { return  m_iSliceQpBase;       }
#endif
  Int       getSliceQpDelta ()                          { return  m_iSliceQpDelta;      }
  Int       getSliceQpDeltaCb ()                          { return  m_iSliceQpDeltaCb;      }
  Int       getSliceQpDeltaCr ()                          { return  m_iSliceQpDeltaCr;      }
  Bool      getDeblockingFilterDisable()                { return  m_deblockingFilterDisable; }
  Bool      getDeblockingFilterOverrideFlag()           { return  m_deblockingFilterOverrideFlag; }
  Int       getDeblockingFilterBetaOffsetDiv2()         { return  m_deblockingFilterBetaOffsetDiv2; }
  Int       getDeblockingFilterTcOffsetDiv2()           { return  m_deblockingFilterTcOffsetDiv2; }

  Int       getNumRefIdx        ( RefPicList e )                { return  m_aiNumRefIdx[e];             }
  TComPic*  getPic              ()                              { return  m_pcPic;                      }
  TComPic*  getRefPic           ( RefPicList e, Int iRefIdx)    { return  m_apcRefPicList[e][iRefIdx];  }
  Int       getRefPOC           ( RefPicList e, Int iRefIdx)    { return  m_aiRefPOCList[e][iRefIdx];   }
#if H_3D
  TComPic*  getIvPic            ( Bool depthFlag, Int viewIndex){ return  m_ivPicsCurrPoc[ depthFlag ? 1 : 0 ][ viewIndex ]; }
#endif
#if H_3D
  TComPic*  getTexturePic       ()                              { return  m_ivPicsCurrPoc[0][ m_viewIndex ]; }
#endif
#if H_3D_IC
  Void      setApplyIC( Bool b )                                { m_bApplyIC = b; }
  Bool      getApplyIC()                                        { return m_bApplyIC; }
  Void      xSetApplyIC();
  Void      setIcSkipParseFlag( Bool b )                        { m_icSkipParseFlag = b; }
  Bool      getIcSkipParseFlag()                                { return m_icSkipParseFlag; }
#endif
#if H_3D_ARP
  Void      setBaseViewRefPicList( TComList<TComPic*> *pListPic, Int iViewIdx )      { m_pBaseViewRefPicList[iViewIdx] = pListPic;                   }
  Void      setARPStepNum( TComPicLists*ivPicLists );
  TComPic*  getBaseViewRefPic    ( UInt uiPOC , Int iViewIdx )                       { return xGetRefPic( *m_pBaseViewRefPicList[iViewIdx], uiPOC ); }
  UInt      getARPStepNum( )                                                         { return m_nARPStepNum;                                         }  
#endif
  Int       getDepth            ()                              { return  m_iDepth;                     }
  UInt      getColFromL0Flag    ()                              { return  m_colFromL0Flag;              }
  UInt      getColRefIdx        ()                              { return  m_colRefIdx;                  }
  Void      checkColRefIdx      (UInt curSliceIdx, TComPic* pic);
  Bool      getIsUsedAsLongTerm (Int i, Int j)                  { return m_bIsUsedAsLongTerm[i][j]; }
  Void      setIsUsedAsLongTerm (Int i, Int j, Bool value)      { m_bIsUsedAsLongTerm[i][j] = value; }
  Bool      getCheckLDC     ()                                  { return m_bCheckLDC; }
  Bool      getMvdL1ZeroFlag ()                                  { return m_bLMvdL1Zero;    }
  Int       getNumRpsCurrTempList();
  Int       getList1IdxToList0Idx ( Int list1Idx )               { return m_list1IdxToList0Idx[list1Idx]; }
  Void      setReferenced(Bool b)                               { m_bRefenced = b; }
  Bool      isReferenced()                                      { return m_bRefenced; }
  Bool      isReferenceNalu()                                   { return ((getNalUnitType() <= NAL_UNIT_RESERVED_VCL_R15) && (getNalUnitType()%2 != 0)) || ((getNalUnitType() >= NAL_UNIT_CODED_SLICE_BLA_W_LP) && (getNalUnitType() <= NAL_UNIT_RESERVED_IRAP_VCL23) ); }
  Void      setPOC              ( Int i )                       { m_iPOC              = i; if ((getTLayer()==0) && (isReferenceNalu() && (getNalUnitType()!=NAL_UNIT_CODED_SLICE_RASL_R)&& (getNalUnitType()!=NAL_UNIT_CODED_SLICE_RADL_R))) {m_prevTid0POC=i;} }
  Void      setNalUnitType      ( NalUnitType e )               { m_eNalUnitType      = e;      }
  NalUnitType getNalUnitType    () const                        { return m_eNalUnitType;        }
  Bool      getRapPicFlag       ();  
  Bool      getIdrPicFlag       ()                              { return getNalUnitType() == NAL_UNIT_CODED_SLICE_IDR_W_RADL || getNalUnitType() == NAL_UNIT_CODED_SLICE_IDR_N_LP; }
  Bool      isIRAP              () const                        { return (getNalUnitType() >= 16) && (getNalUnitType() <= 23); }  
  Void      checkCRA(TComReferencePictureSet *pReferencePictureSet, Int& pocCRA, NalUnitType& associatedIRAPType, TComList<TComPic *>& rcListPic);
  Void      decodingRefreshMarking(Int& pocCRA, Bool& bRefreshPending, TComList<TComPic*>& rcListPic);
  Void      setSliceType        ( SliceType e )                 { m_eSliceType        = e;      }
  Void      setSliceQp          ( Int i )                       { m_iSliceQp          = i;      }
#if ADAPTIVE_QP_SELECTION
  Void      setSliceQpBase      ( Int i )                       { m_iSliceQpBase      = i;      }
#endif
  Void      setSliceQpDelta     ( Int i )                       { m_iSliceQpDelta     = i;      }
  Void      setSliceQpDeltaCb   ( Int i )                       { m_iSliceQpDeltaCb   = i;      }
  Void      setSliceQpDeltaCr   ( Int i )                       { m_iSliceQpDeltaCr   = i;      }
  Void      setDeblockingFilterDisable( Bool b )                { m_deblockingFilterDisable= b;      }
  Void      setDeblockingFilterOverrideFlag( Bool b )           { m_deblockingFilterOverrideFlag = b; }
  Void      setDeblockingFilterBetaOffsetDiv2( Int i )          { m_deblockingFilterBetaOffsetDiv2 = i; }
  Void      setDeblockingFilterTcOffsetDiv2( Int i )            { m_deblockingFilterTcOffsetDiv2 = i; }
  
  Void      setRefPic           ( TComPic* p, RefPicList e, Int iRefIdx ) { m_apcRefPicList[e][iRefIdx] = p; }
  Void      setRefPOC           ( Int i, RefPicList e, Int iRefIdx ) { m_aiRefPOCList[e][iRefIdx] = i; }
  Void      setNumRefIdx        ( RefPicList e, Int i )         { m_aiNumRefIdx[e]    = i;      }
  Void      setPic              ( TComPic* p )                  { m_pcPic             = p;      }
  Void      setDepth            ( Int iDepth )                  { m_iDepth            = iDepth; }
  
#if H_MV
  Void      setPocBeforeReset   ( Int i )                       { m_iPOCBeforeReset = i; }
  Int       getPocBeforeReset   ( )                             { return m_iPOCBeforeReset; }
  Int       getRefLayerId        ( RefPicList e, Int iRefIdx)    { return  m_aiRefLayerIdList[e][iRefIdx]; }
  Void      setRefLayerId        ( Int i, RefPicList e, Int iRefIdx ) { m_aiRefLayerIdList[e][iRefIdx] = i; }
  Void      getTempRefPicLists   ( TComList<TComPic*>& rcListPic, std::vector<TComPic*>& refPicSetInterLayer0, std::vector<TComPic*>& refPicSetInterLayer1,                                     
                                   std::vector<TComPic*> rpsCurrList[2], std::vector<Bool> usedAsLongTerm[2], Int& numPocTotalCurr, Bool checkNumPocTotalCurr = false );

  Void      setRefPicList        ( std::vector<TComPic*> rpsCurrList[2], std::vector<Bool> usedAsLongTerm[2], Int numPocTotalCurr, Bool checkNumPocTotalCurr = false ); 
#else
  Void      setRefPicList       ( TComList<TComPic*>& rcListPic, Bool checkNumPocTotalCurr = false );
#endif
  Void      setRefPOCList       ();
  Void      setColFromL0Flag    ( UInt colFromL0 ) { m_colFromL0Flag = colFromL0; }
  Void      setColRefIdx        ( UInt refIdx) { m_colRefIdx = refIdx; }
  Void      setCheckLDC         ( Bool b )                      { m_bCheckLDC = b; }
  Void      setMvdL1ZeroFlag     ( Bool b)                       { m_bLMvdL1Zero = b; }

  Bool      isIntra         ()                          { return  m_eSliceType == I_SLICE;  }
  Bool      isInterB        ()                          { return  m_eSliceType == B_SLICE;  }
  Bool      isInterP        ()                          { return  m_eSliceType == P_SLICE;  }
  
  Void      setLambdas ( const Double lambdas[3] ) { for (Int component = 0; component < 3; component++) m_lambdas[component] = lambdas[component]; }
  const Double* getLambdas() const { return m_lambdas; }
  
  Void      initEqualRef();
  Bool      isEqualRef  ( RefPicList e, Int iRefIdx1, Int iRefIdx2 )
  {
    if (iRefIdx1 < 0 || iRefIdx2 < 0) return false;
    return m_abEqualRef[e][iRefIdx1][iRefIdx2];
  }
  
  Void setEqualRef( RefPicList e, Int iRefIdx1, Int iRefIdx2, Bool b)
  {
    m_abEqualRef[e][iRefIdx1][iRefIdx2] = m_abEqualRef[e][iRefIdx2][iRefIdx1] = b;
  }
  
  static Void      sortPicList         ( TComList<TComPic*>& rcListPic );
  Void setList1IdxToList0Idx();

  UInt getTLayer             ()                            { return m_uiTLayer;                      }
  Void setTLayer             ( UInt uiTLayer )             { m_uiTLayer = uiTLayer;                  }

  Void setTLayerInfo( UInt uiTLayer );
  Void decodingMarking( TComList<TComPic*>& rcListPic, Int iGOPSIze, Int& iMaxRefPicNum ); 
  Void checkLeadingPictureRestrictions( TComList<TComPic*>& rcListPic );
  Void applyReferencePictureSet( TComList<TComPic*>& rcListPic, TComReferencePictureSet *RPSList);
#if H_MV
  Void createInterLayerReferencePictureSet( TComPicLists* ivPicLists, std::vector<TComPic*>& refPicSetInterLayer0, std::vector<TComPic*>& refPicSetInterLayer1 );
  static Void markIvRefPicsAsShortTerm    ( std::vector<TComPic*> refPicSetInterLayer0, std::vector<TComPic*> refPicSetInterLayer1 );
  static Void markCurrPic                 ( TComPic* currPic );; 
  static Void markIvRefPicsAsUnused       ( TComPicLists* ivPicLists, std::vector<Int> targetDecLayerIdSet, TComVPS* vps, Int curLayerId, Int curPoc  );
  Void        printRefPicList();
#endif
  Bool isTemporalLayerSwitchingPoint( TComList<TComPic*>& rcListPic );
  Bool isStepwiseTemporalLayerSwitchingPointCandidate( TComList<TComPic*>& rcListPic );
  Int       checkThatAllRefPicsAreAvailable( TComList<TComPic*>& rcListPic, TComReferencePictureSet *pReferencePictureSet, Bool printErrors, Int pocRandomAccess = 0);
  Void      createExplicitReferencePictureSetFromReference( TComList<TComPic*>& rcListPic, TComReferencePictureSet *pReferencePictureSet, Bool isRAP);

  Void setMaxNumMergeCand               (UInt val )         { m_maxNumMergeCand = val;                    }
  UInt getMaxNumMergeCand               ()                  { return m_maxNumMergeCand;                   }

  Void setSliceMode                     ( UInt uiMode )     { m_sliceMode = uiMode;                     }
  UInt getSliceMode                     ()                  { return m_sliceMode;                       }
  Void setSliceArgument                 ( UInt uiArgument ) { m_sliceArgument = uiArgument;             }
  UInt getSliceArgument                 ()                  { return m_sliceArgument;                   }
  Void setSliceCurStartCUAddr           ( UInt uiAddr )     { m_sliceCurStartCUAddr = uiAddr;           }
  UInt getSliceCurStartCUAddr           ()                  { return m_sliceCurStartCUAddr;             }
  Void setSliceCurEndCUAddr             ( UInt uiAddr )     { m_sliceCurEndCUAddr = uiAddr;             }
  UInt getSliceCurEndCUAddr             ()                  { return m_sliceCurEndCUAddr;               }
  Void setSliceIdx                      ( UInt i)           { m_sliceIdx = i;                           }
  UInt getSliceIdx                      ()                  { return  m_sliceIdx;                       }
  Void copySliceInfo                    (TComSlice *pcSliceSrc);
  Void setSliceSegmentMode              ( UInt uiMode )     { m_sliceSegmentMode = uiMode;              }
  UInt getSliceSegmentMode              ()                  { return m_sliceSegmentMode;                }
  Void setSliceSegmentArgument          ( UInt uiArgument ) { m_sliceSegmentArgument = uiArgument;      }
  UInt getSliceSegmentArgument          ()                  { return m_sliceSegmentArgument;            }
  Void setSliceSegmentCurStartCUAddr    ( UInt uiAddr )     { m_sliceSegmentCurStartCUAddr = uiAddr;    }
  UInt getSliceSegmentCurStartCUAddr    ()                  { return m_sliceSegmentCurStartCUAddr;      }
  Void setSliceSegmentCurEndCUAddr      ( UInt uiAddr )     { m_sliceSegmentCurEndCUAddr = uiAddr;      }
  UInt getSliceSegmentCurEndCUAddr      ()                  { return m_sliceSegmentCurEndCUAddr;        }
  Void setNextSlice                     ( Bool b )          { m_nextSlice = b;                           }
  Bool isNextSlice                      ()                  { return m_nextSlice;                        }
  Void setNextSliceSegment              ( Bool b )          { m_nextSliceSegment = b;                    }
  Bool isNextSliceSegment               ()                  { return m_nextSliceSegment;                 }
  Void setSliceBits                     ( UInt uiVal )      { m_sliceBits = uiVal;                      }
  UInt getSliceBits                     ()                  { return m_sliceBits;                       }  
  Void setSliceSegmentBits              ( UInt uiVal )      { m_sliceSegmentBits = uiVal;            }
  UInt getSliceSegmentBits              ()                  { return m_sliceSegmentBits;             }
  Void setFinalized                     ( Bool uiVal )      { m_bFinalized = uiVal;                       }
  Bool getFinalized                     ()                  { return m_bFinalized;                        }
  Void  setWpScaling    ( wpScalingParam  wp[2][MAX_NUM_REF][3] ) { memcpy(m_weightPredTable, wp, sizeof(wpScalingParam)*2*MAX_NUM_REF*3); }
  Void  getWpScaling    ( RefPicList e, Int iRefIdx, wpScalingParam *&wp);

  Void  resetWpScaling  ();
  Void  initWpScaling   ();
  inline Bool applyWP   () { return( (m_eSliceType==P_SLICE && m_pcPPS->getUseWP()) || (m_eSliceType==B_SLICE && m_pcPPS->getWPBiPred()) ); }

  Void  setWpAcDcParam  ( wpACDCParam wp[3] ) { memcpy(m_weightACDCParam, wp, sizeof(wpACDCParam)*3); }
  Void  getWpAcDcParam  ( wpACDCParam *&wp );
  Void  initWpAcDcParam ();
  
  Void setTileLocationCount             ( UInt cnt )               { return m_tileByteLocation.resize(cnt);    }
  UInt getTileLocationCount             ()                         { return (UInt) m_tileByteLocation.size();  }
  Void setTileLocation                  ( Int idx, UInt location ) { assert (idx<m_tileByteLocation.size());
                                                                     m_tileByteLocation[idx] = location;       }
  Void addTileLocation                  ( UInt location )          { m_tileByteLocation.push_back(location);   }
  UInt getTileLocation                  ( Int idx )                { return m_tileByteLocation[idx];           }

  Void setTileOffstForMultES            (UInt uiOffset )      { m_uiTileOffstForMultES = uiOffset;        }
  UInt getTileOffstForMultES            ()                    { return m_uiTileOffstForMultES;            }
  Void allocSubstreamSizes              ( UInt uiNumSubstreams );
  UInt* getSubstreamSizes               ()                  { return m_puiSubstreamSizes; }
  Void  setScalingList              ( TComScalingList* scalingList ) { m_scalingList = scalingList; }
  TComScalingList*   getScalingList ()                               { return m_scalingList; }
  Void  setDefaultScalingList       ();
  Bool  checkDefaultScalingList     ();
  Void      setCabacInitFlag  ( Bool val ) { m_cabacInitFlag = val;      }  //!< set CABAC initial flag 
  Bool      getCabacInitFlag  ()           { return m_cabacInitFlag;     }  //!< get CABAC initial flag 
  Void      setNumEntryPointOffsets(Int val)  { m_numEntryPointOffsets = val;     }
  Int       getNumEntryPointOffsets()         { return m_numEntryPointOffsets;    }
  Bool      getTemporalLayerNonReferenceFlag()       { return m_temporalLayerNonReferenceFlag;}
  Void      setTemporalLayerNonReferenceFlag(Bool x) { m_temporalLayerNonReferenceFlag = x;}
  Void      setLFCrossSliceBoundaryFlag     ( Bool   val )    { m_LFCrossSliceBoundaryFlag = val; }
  Bool      getLFCrossSliceBoundaryFlag     ()                { return m_LFCrossSliceBoundaryFlag;} 

  Void      setEnableTMVPFlag     ( Bool   b )    { m_enableTMVPFlag = b; }
  Bool      getEnableTMVPFlag     ()              { return m_enableTMVPFlag;}

#if H_MV
  Void      setLayerId            ( Int layerId )    { m_layerId      = layerId; }
  Int       getLayerId            ()                 { return m_layerId;    }
  Int       getLayerIdInVps       ()                 { return getVPS()->getLayerIdInVps( m_layerId ); }; 
  Void      setViewId             ( Int viewId )     { m_viewId = viewId;   }
  Int       getViewId             ()                 { return m_viewId;     }
  Void      setViewIndex          ( Int viewIndex )  { m_viewIndex = viewIndex;   }
  Int       getViewIndex          ()                 { return m_viewIndex;     }
#if H_3D
#if H_3D_TMVP
  Void      generateAlterRefforTMVP ();   
  Void      setAlterRefIdx          ( RefPicList e, Int i ) { m_aiAlterRefIdx[e]    = i;      }
  Int       getAlterRefIdx          ( RefPicList e )        { return  m_aiAlterRefIdx[e];     }
#endif
#if H_3D_ARP
  Int       getFirstTRefIdx        ( RefPicList e )                { return  m_aiFirstTRefIdx[e];     }
  Void      setFirstTRefIdx        ( RefPicList e, Int i )         { m_aiFirstTRefIdx[e]    = i;      }
  Bool      getArpRefPicAvailable( RefPicList e, Int viewIdx) {return m_arpRefPicAvailable[e][getVPS()->getLayerIdInNuh(viewIdx, 0)]; }
#endif
  Void      setIsDepth            ( Bool isDepth )   { m_isDepth = isDepth; }
  Bool      getIsDepth            ()                 { return m_isDepth; }
  Void      setCamparaSlice       ( Int** aaiScale = 0, Int** aaiOffset = 0 );
  Int*      getCodedScale         ()  { return m_aaiCodedScale [0]; }
  Int*      getCodedOffset        ()  { return m_aaiCodedOffset[0]; }
  Int*      getInvCodedScale      ()  { return m_aaiCodedScale [1]; }
  Int*      getInvCodedOffset     ()  { return m_aaiCodedOffset[1]; }
#endif
#endif
#if H_3D
  Void    setIvPicLists( TComPicLists* m_ivPicLists );
  Void    setDepthToDisparityLUTs();

  Int* getDepthToDisparityB( Int refViewIdx ) { return m_depthToDisparityB[ refViewIdx ]; }; 
  Int* getDepthToDisparityF( Int refViewIdx ) { return m_depthToDisparityF[ refViewIdx ]; }; 
  Bool getVpsDepthModesFlag  ()  { return getVPS()->getVpsDepthModesFlag( getVPS()->getLayerIdInVps( m_layerId ) ); }

#endif
#if H_MV
// Additional slice header syntax elements

  Void setCrossLayerBlaFlag( Bool flag ) { m_crossLayerBlaFlag = flag; } 
  Bool getCrossLayerBlaFlag(  ) { return m_crossLayerBlaFlag; } 
  Void checkCrossLayerBlaFlag ( )
  {
    // cross_layer_bla_flag shall be equal to 0 for pictures with nal_unit_type not equal to IDR_W_RADL or IDR_N_LP or with nuh_layer_id not equal to 0.
    if ( getLayerId() != 0 || getNalUnitType() != NAL_UNIT_CODED_SLICE_IDR_W_RADL || getNalUnitType() == NAL_UNIT_CODED_SLICE_IDR_N_LP )
    {
      assert( m_crossLayerBlaFlag == 0 ); 
    }
  }

#if !H_MV_HLS7_GEN
  Void setPocResetFlag( Bool flag ) { m_pocResetFlag = flag; } 
  Bool getPocResetFlag(  ) { return m_pocResetFlag; } 
#endif

  Void setDiscardableFlag( Bool flag ) { m_discardableFlag = flag; } 
  Bool getDiscardableFlag(  ) { return m_discardableFlag; } 

  Void setInterLayerPredEnabledFlag( Bool flag ) { m_interLayerPredEnabledFlag = flag; } 
  Bool getInterLayerPredEnabledFlag(  ) { return m_interLayerPredEnabledFlag; } 

  Void setNumInterLayerRefPicsMinus1( Int  val ) { m_numInterLayerRefPicsMinus1 = val; } 
  Int  getNumInterLayerRefPicsMinus1(  ) { return m_numInterLayerRefPicsMinus1; } 

  Void setInterLayerPredLayerIdc( Int i, Int  val ) { m_interLayerPredLayerIdc[i] = val; } 
  Int  getInterLayerPredLayerIdc( Int i ) { return m_interLayerPredLayerIdc[i]; } 

#if H_MV_HLS_7_POC_P0041
  Void setSliceSegmentHeaderExtensionLength( Int  val ) { m_sliceSegmentHeaderExtensionLength = val; } 
  Int  getSliceSegmentHeaderExtensionLength(  ) { return m_sliceSegmentHeaderExtensionLength; } 

  Void setPocResetIdc( Int  val ) { m_pocResetIdc = val; } 
  Int  getPocResetIdc(  ) { return m_pocResetIdc; } 

  Void setPocResetPeriodId( Int  val ) { m_pocResetPeriodId = val; } 
  Int  getPocResetPeriodId(  ) { return m_pocResetPeriodId; } 

  Void setFullPocResetFlag( Bool flag ) { m_fullPocResetFlag = flag; } 
  Bool getFullPocResetFlag(  ) { return m_fullPocResetFlag; } 

  Void setPocLsbVal( Int  val ) { m_pocLsbVal = val; } 
  Int  getPocLsbVal(  ) { return m_pocLsbVal; } 

  Void setPocMsbValPresentFlag( Bool flag ) { m_pocMsbValPresentFlag = flag; } 
  Bool getPocMsbValPresentFlag(  ) { return m_pocMsbValPresentFlag; } 

  Void setPocMsbVal( Int  val ) { m_pocMsbVal = val; } 
  Int  getPocMsbVal(  ) { return m_pocMsbVal; } 

  Bool getPocMsbValRequiredFlag() { return m_pocMsbValRequiredFlag; }
  Void setPocMsbValRequiredFlag(Bool x) { m_pocMsbValRequiredFlag = x; }

  UInt getPocLsbValLen() { return getSPS()->getBitsForPOC(); }; //log2_max_pic_order_cnt_lsb_minus4 + 4  

  Bool getBlaPicFlag       ()
  {
    return  getNalUnitType() == NAL_UNIT_CODED_SLICE_BLA_N_LP
    || getNalUnitType() == NAL_UNIT_CODED_SLICE_BLA_W_RADL
    || getNalUnitType() == NAL_UNIT_CODED_SLICE_BLA_W_LP;
  }
  Bool getCraPicFlag       ()
  {
    return getNalUnitType() == NAL_UNIT_CODED_SLICE_CRA;
  }
#endif

  // Additional variables derived in slice header semantics 
  Int  getNumInterLayerRefPicsMinus1Len( ) { return gCeilLog2(  getVPS()->getNumDirectRefLayers( getLayerId() )); }
  Int  getInterLayerPredLayerIdcLen    ( ) { return gCeilLog2(  getVPS()->getNumDirectRefLayers( getLayerId() )); }

  Int  getRefLayerPicFlag( Int i ); 
  Int  getRefLayerPicIdc ( Int j ); 
  Int  getNumRefLayerPics( ); 

  Int  getNumActiveRefLayerPics( );

  Int  getNumActiveRefLayerPics0( )        { return (Int) m_refPicSetInterLayer0->size();  };
  Int  getNumActiveRefLayerPics1( )        { return (Int) m_refPicSetInterLayer1->size();  };

  Int  getRefPicLayerId               ( Int i );

  Void     setRefPicSetInterLayer       ( std::vector<TComPic*>* refPicSetInterLayer0, std::vector<TComPic*>* refPicSetInterLayer1);
  TComPic* getPicFromRefPicSetInterLayer( Int setIdc, Int layerId );

#endif
#if H_3D_DDD
  Void InitializeDDDPara( UInt uiCamParsCodedPrecision, Int  iCodedScale,Int  iCodedOffset, Int iBaseViewIdx );
  Int  getDepthFromDV( Int iDV, Int iBaseViewIdx )
  {
      return ClipY(( iDV * m_aiDDDInvScale[ iBaseViewIdx ] + m_aiDDDInvOffset[ iBaseViewIdx ] ) >> m_aiDDDShift[ iBaseViewIdx ]);
  }
#endif

protected:
  TComPic*  xGetRefPic  (TComList<TComPic*>& rcListPic,
                         Int                 poc);
TComPic*  xGetLongTermRefPic(TComList<TComPic*>& rcListPic, Int poc, Bool pocHasMsb);
#if H_MV
  TComPic*  xGetInterLayerRefPic( std::vector<TComPic*>& rcListIlPic, Int layerId );  
#endif
};// END CLASS DEFINITION TComSlice


template <class T> class ParameterSetMap
{
public:
  ParameterSetMap(Int maxId)
  :m_maxId (maxId)
  {}

  ~ParameterSetMap()
  {
    for (typename std::map<Int,T *>::iterator i = m_paramsetMap.begin(); i!= m_paramsetMap.end(); i++)
    {
      delete (*i).second;
    }
  }

  Void storePS(Int psId, T *ps)
  {
    assert ( psId < m_maxId );
    if ( m_paramsetMap.find(psId) != m_paramsetMap.end() )
    {
      delete m_paramsetMap[psId];
    }
    m_paramsetMap[psId] = ps; 
  }

  Void mergePSList(ParameterSetMap<T> &rPsList)
  {
    for (typename std::map<Int,T *>::iterator i = rPsList.m_paramsetMap.begin(); i!= rPsList.m_paramsetMap.end(); i++)
    {
      storePS(i->first, i->second);
    }
    rPsList.m_paramsetMap.clear();
  }


  T* getPS(Int psId)
  {
    return ( m_paramsetMap.find(psId) == m_paramsetMap.end() ) ? NULL : m_paramsetMap[psId];
  }

  T* getFirstPS()
  {
    return (m_paramsetMap.begin() == m_paramsetMap.end() ) ? NULL : m_paramsetMap.begin()->second;
  }

private:
  std::map<Int,T *> m_paramsetMap;
  Int               m_maxId;
};

class ParameterSetManager
{
public:
  ParameterSetManager();
  virtual ~ParameterSetManager();

  //! store sequence parameter set and take ownership of it 
  Void storeVPS(TComVPS *vps) { m_vpsMap.storePS( vps->getVPSId(), vps); };
  //! get pointer to existing video parameter set  
  TComVPS* getVPS(Int vpsId)  { return m_vpsMap.getPS(vpsId); };
  TComVPS* getFirstVPS()      { return m_vpsMap.getFirstPS(); };
  
  //! store sequence parameter set and take ownership of it 
  Void storeSPS(TComSPS *sps) { m_spsMap.storePS( sps->getSPSId(), sps); };
  //! get pointer to existing sequence parameter set  
  TComSPS* getSPS(Int spsId)  { return m_spsMap.getPS(spsId); };
  TComSPS* getFirstSPS()      { return m_spsMap.getFirstPS(); };

  //! store picture parameter set and take ownership of it 
  Void storePPS(TComPPS *pps) { m_ppsMap.storePS( pps->getPPSId(), pps); };
  //! get pointer to existing picture parameter set  
  TComPPS* getPPS(Int ppsId)  { return m_ppsMap.getPS(ppsId); };
  TComPPS* getFirstPPS()      { return m_ppsMap.getFirstPS(); };

  //! activate a SPS from a active parameter sets SEI message
  //! \returns true, if activation is successful
#if H_MV
  Bool activateSPSWithSEI(Int SPSId, Int layerId );
#else
  Bool activateSPSWithSEI(Int SPSId);
#endif

  //! activate a PPS and depending on isIDR parameter also SPS and VPS
  //! \returns true, if activation is successful
#if H_MV
  Bool activatePPS(Int ppsId, Bool isIRAP, Int layerId );
#else
  Bool activatePPS(Int ppsId, Bool isIRAP);
#endif

  TComVPS* getActiveVPS(){ return m_vpsMap.getPS(m_activeVPSId); };
#if H_MV
  TComSPS* getActiveSPS( Int layerId ){ return m_spsMap.getPS( m_activeSPSId[ layerId ] ); };
  TComPPS* getActivePPS( Int layerId ){ return m_ppsMap.getPS( m_activePPSId[ layerId ] ); };
#else
  TComSPS* getActiveSPS(){ return m_spsMap.getPS(m_activeSPSId); };
  TComPPS* getActivePPS(){ return m_ppsMap.getPS(m_activePPSId); };
#endif
protected:
  
  ParameterSetMap<TComVPS> m_vpsMap;
  ParameterSetMap<TComSPS> m_spsMap; 
  ParameterSetMap<TComPPS> m_ppsMap;

  Int m_activeVPSId;
#if H_MV
  Int m_activeSPSId[ MAX_NUM_LAYERS ];
  Int m_activePPSId[ MAX_NUM_LAYERS ];
#else
  Int m_activeSPSId;
  Int m_activePPSId;
#endif

};

//! \}

#endif // __TCOMSLICE__
