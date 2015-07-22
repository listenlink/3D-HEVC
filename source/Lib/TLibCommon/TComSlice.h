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
#include "TComChromaFormat.h"

//! \ingroup TLibCommon
//! \{

class TComPic;
class TComTrQuant;
#if NH_MV
class TComPicLists; 
class TComVPS; 
class TComSPS; 
#endif
// ====================================================================================================================
// Constants
// ====================================================================================================================

static const UInt REF_PIC_LIST_NUM_IDX=32;

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
  Int     getPocLSBLT(Int i) const                     { return m_pocLSBLT[i];               }
  Void    setPocLSBLT(Int i, Int x)                    { m_pocLSBLT[i] = x;                  }
  Int     getDeltaPocMSBCycleLT(Int i) const           { return m_deltaPOCMSBCycleLT[i];     }
  Void    setDeltaPocMSBCycleLT(Int i, Int x)          { m_deltaPOCMSBCycleLT[i] = x;        }
  Bool    getDeltaPocMSBPresentFlag(Int i) const       { return m_deltaPocMSBPresentFlag[i]; }
  Void    setDeltaPocMSBPresentFlag(Int i, Bool x)     { m_deltaPocMSBPresentFlag[i] = x;    }
  Void    setUsed(Int bufferNum, Bool used);
  Void    setDeltaPOC(Int bufferNum, Int deltaPOC);
  Void    setPOC(Int bufferNum, Int deltaPOC);
  Void    setNumberOfPictures(Int numberOfPictures);
  Void    setCheckLTMSBPresent(Int bufferNum, Bool b );
  Bool    getCheckLTMSBPresent(Int bufferNum) const;

  Int     getUsed(Int bufferNum) const;
  Int     getDeltaPOC(Int bufferNum) const;
  Int     getPOC(Int bufferNum) const;
  Int     getNumberOfPictures() const;

  Void    setNumberOfNegativePictures(Int number)      { m_numberOfNegativePictures = number; }
  Int     getNumberOfNegativePictures() const          { return m_numberOfNegativePictures;   }
  Void    setNumberOfPositivePictures(Int number)      { m_numberOfPositivePictures = number; }
  Int     getNumberOfPositivePictures() const          { return m_numberOfPositivePictures;   }
  Void    setNumberOfLongtermPictures(Int number)      { m_numberOfLongtermPictures = number; }
  Int     getNumberOfLongtermPictures() const          { return m_numberOfLongtermPictures;   }

  Void    setInterRPSPrediction(Bool flag)             { m_interRPSPrediction = flag;         }
  Bool    getInterRPSPrediction() const                { return m_interRPSPrediction;         }
  Void    setDeltaRIdxMinus1(Int x)                    { m_deltaRIdxMinus1 = x;               }
  Int     getDeltaRIdxMinus1() const                   { return m_deltaRIdxMinus1;            }
  Void    setDeltaRPS(Int x)                           { m_deltaRPS = x;                      }
  Int     getDeltaRPS() const                          { return m_deltaRPS;                   }
  Void    setNumRefIdc(Int x)                          { m_numRefIdc = x;                     }
  Int     getNumRefIdc() const                         { return m_numRefIdc;                  }

  Void    setRefIdc(Int bufferNum, Int refIdc);
  Int     getRefIdc(Int bufferNum) const ;

  Void    sortDeltaPOC();
  Void    printDeltaPOC() const;

#if NH_MV
  Void checkMaxNumPics( Bool vpsExtensionFlag, Int maxNumPics, Int nuhLayerId, Int spsMaxDecPicBufferingMinus1 ) const;
#endif

};

/// Reference Picture Set set class
class TComRPSList
{
private:
  std::vector<TComReferencePictureSet> m_referencePictureSets;

public:
                                 TComRPSList()                                            { }
  virtual                        ~TComRPSList()                                           { }

  Void                           create  (Int numberOfEntries)                            { m_referencePictureSets.resize(numberOfEntries);         }
  Void                           destroy ()                                               { }


  TComReferencePictureSet*       getReferencePictureSet(Int referencePictureSetNum)       { return &m_referencePictureSets[referencePictureSetNum]; }
  const TComReferencePictureSet* getReferencePictureSet(Int referencePictureSetNum) const { return &m_referencePictureSets[referencePictureSetNum]; }

  Int                            getNumberOfReferencePictureSets() const                  { return Int(m_referencePictureSets.size());              }
};

/// SCALING_LIST class
class TComScalingList
{
public:
             TComScalingList();
  virtual    ~TComScalingList()                                                 { }
  Int*       getScalingListAddress(UInt sizeId, UInt listId)                    { return &(m_scalingListCoef[sizeId][listId][0]);            } //!< get matrix coefficient
  const Int* getScalingListAddress(UInt sizeId, UInt listId) const              { return &(m_scalingListCoef[sizeId][listId][0]);            } //!< get matrix coefficient
  Void       checkPredMode(UInt sizeId, UInt listId);

  Void       setRefMatrixId(UInt sizeId, UInt listId, UInt u)                   { m_refMatrixId[sizeId][listId] = u;                         } //!< set reference matrix ID
  UInt       getRefMatrixId(UInt sizeId, UInt listId) const                     { return m_refMatrixId[sizeId][listId];                      } //!< get reference matrix ID

  const Int* getScalingListDefaultAddress(UInt sizeId, UInt listId);                                                                           //!< get default matrix coefficient
  Void       processDefaultMatrix(UInt sizeId, UInt listId);

  Void       setScalingListDC(UInt sizeId, UInt listId, UInt u)                 { m_scalingListDC[sizeId][listId] = u;                       } //!< set DC value
  Int        getScalingListDC(UInt sizeId, UInt listId) const                   { return m_scalingListDC[sizeId][listId];                    } //!< get DC value

  Void       setScalingListPredModeFlag(UInt sizeId, UInt listId, Bool bIsDPCM) { m_scalingListPredModeFlagIsDPCM[sizeId][listId] = bIsDPCM; }
  Bool       getScalingListPredModeFlag(UInt sizeId, UInt listId) const         { return m_scalingListPredModeFlagIsDPCM[sizeId][listId];    }

  Void       checkDcOfMatrix();
  Void       processRefMatrix(UInt sizeId, UInt listId , UInt refListId );
  Bool       xParseScalingList(Char* pchFile);
#if NH_MV
  Void       inferFrom                      ( const TComScalingList& srcScLi );
#endif
  Void       setDefaultScalingList();
  Bool       checkDefaultScalingList();

private:
  Void       outputScalingLists(std::ostream &os) const;
  Bool             m_scalingListPredModeFlagIsDPCM [SCALING_LIST_SIZE_NUM][SCALING_LIST_NUM]; //!< reference list index
  Int              m_scalingListDC                 [SCALING_LIST_SIZE_NUM][SCALING_LIST_NUM]; //!< the DC value of the matrix coefficient for 16x16
  UInt             m_refMatrixId                   [SCALING_LIST_SIZE_NUM][SCALING_LIST_NUM]; //!< RefMatrixID
  std::vector<Int> m_scalingListCoef               [SCALING_LIST_SIZE_NUM][SCALING_LIST_NUM]; //!< quantization matrix
};

class ProfileTierLevel
{
  Int               m_profileSpace;
  Level::Tier       m_tierFlag;
  Profile::Name     m_profileIdc;
  Bool              m_profileCompatibilityFlag[32];
  Level::Name       m_levelIdc;

  Bool              m_progressiveSourceFlag;
  Bool              m_interlacedSourceFlag;
  Bool              m_nonPackedConstraintFlag;
  Bool              m_frameOnlyConstraintFlag;
  UInt              m_bitDepthConstraintValue;
  ChromaFormat      m_chromaFormatConstraintValue;
  Bool              m_intraConstraintFlag;
  Bool              m_onePictureOnlyConstraintFlag;
  Bool              m_lowerBitRateConstraintFlag;
#if NH_MV
  Bool              m_max12bitConstraintFlag;
  Bool              m_max10bitConstraintFlag;
  Bool              m_max8bitConstraintFlag;
  Bool              m_max422chromaConstraintFlag;
  Bool              m_max420chromaConstraintFlag;
  Bool              m_maxMonochromeConstraintFlag;
  Bool              m_inbldFlag;
#endif
public:
                ProfileTierLevel();

  Int           getProfileSpace() const                     { return m_profileSpace;                }
  Void          setProfileSpace(Int x)                      { m_profileSpace = x;                   }

  Level::Tier   getTierFlag() const                         { return m_tierFlag;                    }
  Void          setTierFlag(Level::Tier x)                  { m_tierFlag = x;                       }

  Profile::Name getProfileIdc() const                       { return m_profileIdc;                  }
  Void          setProfileIdc(Profile::Name x)              { m_profileIdc = x;                     }

  Bool          getProfileCompatibilityFlag(Int i) const    { return m_profileCompatibilityFlag[i]; }
  Void          setProfileCompatibilityFlag(Int i, Bool x)  { m_profileCompatibilityFlag[i] = x;    }

  Level::Name   getLevelIdc() const                         { return m_levelIdc;                    }
  Void          setLevelIdc(Level::Name x)                  { m_levelIdc = x;                       }

  Bool          getProgressiveSourceFlag() const            { return m_progressiveSourceFlag;       }
  Void          setProgressiveSourceFlag(Bool b)            { m_progressiveSourceFlag = b;          }

  Bool          getInterlacedSourceFlag() const             { return m_interlacedSourceFlag;        }
  Void          setInterlacedSourceFlag(Bool b)             { m_interlacedSourceFlag = b;           }

  Bool          getNonPackedConstraintFlag() const          { return m_nonPackedConstraintFlag;     }
  Void          setNonPackedConstraintFlag(Bool b)          { m_nonPackedConstraintFlag = b;        }

  Bool          getFrameOnlyConstraintFlag() const          { return m_frameOnlyConstraintFlag;     }
  Void          setFrameOnlyConstraintFlag(Bool b)          { m_frameOnlyConstraintFlag = b;        }

  UInt          getBitDepthConstraint() const               { return m_bitDepthConstraintValue;     }
  Void          setBitDepthConstraint(UInt bitDepth)        { m_bitDepthConstraintValue=bitDepth;   }

  ChromaFormat  getChromaFormatConstraint() const           { return m_chromaFormatConstraintValue; }
  Void          setChromaFormatConstraint(ChromaFormat fmt) { m_chromaFormatConstraintValue=fmt;    }

  Bool          getIntraConstraintFlag() const              { return m_intraConstraintFlag;         }
  Void          setIntraConstraintFlag(Bool b)              { m_intraConstraintFlag = b;            }

  Bool          getOnePictureOnlyConstraintFlag() const     { return m_onePictureOnlyConstraintFlag;}
  Void          setOnePictureOnlyConstraintFlag(Bool b)     { m_onePictureOnlyConstraintFlag = b;   }

  Bool          getLowerBitRateConstraintFlag() const       { return m_lowerBitRateConstraintFlag;  }
  Void          setLowerBitRateConstraintFlag(Bool b)       { m_lowerBitRateConstraintFlag = b;     }

#if NH_MV
  Void          setMax12bitConstraintFlag( Bool flag )      { m_max12bitConstraintFlag = flag;      }
  Bool          getMax12bitConstraintFlag(  ) const         { return m_max12bitConstraintFlag;      }

  Void          setMax10bitConstraintFlag( Bool flag )      { m_max10bitConstraintFlag = flag;      }
  Bool          getMax10bitConstraintFlag(  ) const         { return m_max10bitConstraintFlag;      }

  Void          setMax8bitConstraintFlag( Bool flag )       { m_max8bitConstraintFlag = flag;       }
  Bool          getMax8bitConstraintFlag(  ) const          { return m_max8bitConstraintFlag;       }

  Void          setMax422chromaConstraintFlag( Bool flag )  { m_max422chromaConstraintFlag = flag;  }
  Bool          getMax422chromaConstraintFlag(  ) const     { return m_max422chromaConstraintFlag;  }

  Void          setMax420chromaConstraintFlag( Bool flag )  { m_max420chromaConstraintFlag = flag;  }
  Bool          getMax420chromaConstraintFlag(  ) const     { return m_max420chromaConstraintFlag;  }

  Void          setMaxMonochromeConstraintFlag( Bool flag ) { m_maxMonochromeConstraintFlag = flag; }
  Bool          getMaxMonochromeConstraintFlag(  ) const    { return m_maxMonochromeConstraintFlag; }

  Void          setInbldFlag( Bool flag )                   { m_inbldFlag = flag;                   }
  Bool          getInbldFlag(  ) const                      { return m_inbldFlag;                   }

  Bool          getV2ConstraintsPresentFlag() const;
  Bool          getInbldPresentFlag() const;

  Void          copyV2ConstraintFlags( ProfileTierLevel* ptlRef );
  Void          copyProfile( ProfileTierLevel* ptlRef );
#endif

};


class TComPTL
{
  ProfileTierLevel m_generalPTL;
  ProfileTierLevel m_subLayerPTL    [MAX_TLAYER-1];      // max. value of max_sub_layers_minus1 is MAX_TLAYER-1 (= 6)
  Bool m_subLayerProfilePresentFlag [MAX_TLAYER-1];
  Bool m_subLayerLevelPresentFlag   [MAX_TLAYER-1];

public:
                          TComPTL();
  Bool                    getSubLayerProfilePresentFlag(Int i) const   { return m_subLayerProfilePresentFlag[i]; }
  Void                    setSubLayerProfilePresentFlag(Int i, Bool x) { m_subLayerProfilePresentFlag[i] = x;    }

  Bool                    getSubLayerLevelPresentFlag(Int i) const     { return m_subLayerLevelPresentFlag[i];   }
  Void                    setSubLayerLevelPresentFlag(Int i, Bool x)   { m_subLayerLevelPresentFlag[i] = x;      }

  ProfileTierLevel*       getGeneralPTL()                              { return &m_generalPTL;                   }
  const ProfileTierLevel* getGeneralPTL() const                        { return &m_generalPTL;                   }
  ProfileTierLevel*       getSubLayerPTL(Int i)                        { return &m_subLayerPTL[i];               }
  const ProfileTierLevel* getSubLayerPTL(Int i) const                  { return &m_subLayerPTL[i];               }

#if NH_MV
  Void                    inferGeneralValues ( Bool profilePresentFlag  , Int k, TComPTL* refPTL );; 
  Void                    inferSubLayerValues( Int maxNumSubLayersMinus1, Int k, TComPTL* refPTL );; 
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
  Bool cbrFlag           [MAX_CPB_CNT][2];
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
  HrdSubLayerInfo m_HRD[MAX_TLAYER];

public:
  TComHRD()
  :m_nalHrdParametersPresentFlag       (0)
  ,m_vclHrdParametersPresentFlag       (0)
  ,m_subPicCpbParamsPresentFlag        (false)
  ,m_tickDivisorMinus2                 (0)
  ,m_duCpbRemovalDelayLengthMinus1     (0)
  ,m_subPicCpbParamsInPicTimingSEIFlag (false)
  ,m_dpbOutputDelayDuLengthMinus1      (0)
  ,m_bitRateScale                      (0)
  ,m_cpbSizeScale                      (0)
  ,m_initialCpbRemovalDelayLengthMinus1(23)
  ,m_cpbRemovalDelayLengthMinus1       (23)
  ,m_dpbOutputDelayLengthMinus1        (23)
  {}

  virtual ~TComHRD() {}

  Void    setNalHrdParametersPresentFlag( Bool flag )                                { m_nalHrdParametersPresentFlag = flag;                      }
  Bool    getNalHrdParametersPresentFlag( ) const                                    { return m_nalHrdParametersPresentFlag;                      }

  Void    setVclHrdParametersPresentFlag( Bool flag )                                { m_vclHrdParametersPresentFlag = flag;                      }
  Bool    getVclHrdParametersPresentFlag( ) const                                    { return m_vclHrdParametersPresentFlag;                      }

  Void    setSubPicCpbParamsPresentFlag( Bool flag )                                 { m_subPicCpbParamsPresentFlag = flag;                       }
  Bool    getSubPicCpbParamsPresentFlag( ) const                                     { return m_subPicCpbParamsPresentFlag;                       }

  Void    setTickDivisorMinus2( UInt value )                                         { m_tickDivisorMinus2 = value;                               }
  UInt    getTickDivisorMinus2( ) const                                              { return m_tickDivisorMinus2;                                }

  Void    setDuCpbRemovalDelayLengthMinus1( UInt value )                             { m_duCpbRemovalDelayLengthMinus1 = value;                   }
  UInt    getDuCpbRemovalDelayLengthMinus1( ) const                                  { return m_duCpbRemovalDelayLengthMinus1;                    }

  Void    setSubPicCpbParamsInPicTimingSEIFlag( Bool flag)                           { m_subPicCpbParamsInPicTimingSEIFlag = flag;                }
  Bool    getSubPicCpbParamsInPicTimingSEIFlag( ) const                              { return m_subPicCpbParamsInPicTimingSEIFlag;                }

  Void    setDpbOutputDelayDuLengthMinus1(UInt value )                               { m_dpbOutputDelayDuLengthMinus1 = value;                    }
  UInt    getDpbOutputDelayDuLengthMinus1( ) const                                   { return m_dpbOutputDelayDuLengthMinus1;                     }

  Void    setBitRateScale( UInt value )                                              { m_bitRateScale = value;                                    }
  UInt    getBitRateScale( ) const                                                   { return m_bitRateScale;                                     }

  Void    setCpbSizeScale( UInt value )                                              { m_cpbSizeScale = value;                                    }
  UInt    getCpbSizeScale( ) const                                                   { return m_cpbSizeScale;                                     }
  Void    setDuCpbSizeScale( UInt value )                                            { m_ducpbSizeScale = value;                                  }
  UInt    getDuCpbSizeScale( ) const                                                 { return m_ducpbSizeScale;                                   }

  Void    setInitialCpbRemovalDelayLengthMinus1( UInt value )                        { m_initialCpbRemovalDelayLengthMinus1 = value;              }
  UInt    getInitialCpbRemovalDelayLengthMinus1( ) const                             { return m_initialCpbRemovalDelayLengthMinus1;               }

  Void    setCpbRemovalDelayLengthMinus1( UInt value )                               { m_cpbRemovalDelayLengthMinus1 = value;                     }
  UInt    getCpbRemovalDelayLengthMinus1( ) const                                    { return m_cpbRemovalDelayLengthMinus1;                      }

  Void    setDpbOutputDelayLengthMinus1( UInt value )                                { m_dpbOutputDelayLengthMinus1 = value;                      }
  UInt    getDpbOutputDelayLengthMinus1( ) const                                     { return m_dpbOutputDelayLengthMinus1;                       }

  Void    setFixedPicRateFlag( Int layer, Bool flag )                                { m_HRD[layer].fixedPicRateFlag = flag;                      }
  Bool    getFixedPicRateFlag( Int layer ) const                                     { return m_HRD[layer].fixedPicRateFlag;                      }

  Void    setFixedPicRateWithinCvsFlag( Int layer, Bool flag )                       { m_HRD[layer].fixedPicRateWithinCvsFlag = flag;             }
  Bool    getFixedPicRateWithinCvsFlag( Int layer ) const                            { return m_HRD[layer].fixedPicRateWithinCvsFlag;             }

  Void    setPicDurationInTcMinus1( Int layer, UInt value )                          { m_HRD[layer].picDurationInTcMinus1 = value;                }
  UInt    getPicDurationInTcMinus1( Int layer ) const                                { return m_HRD[layer].picDurationInTcMinus1;                 }

  Void    setLowDelayHrdFlag( Int layer, Bool flag )                                 { m_HRD[layer].lowDelayHrdFlag = flag;                       }
  Bool    getLowDelayHrdFlag( Int layer ) const                                      { return m_HRD[layer].lowDelayHrdFlag;                       }

  Void    setCpbCntMinus1( Int layer, UInt value )                                   { m_HRD[layer].cpbCntMinus1 = value;                         }
  UInt    getCpbCntMinus1( Int layer ) const                                         { return m_HRD[layer].cpbCntMinus1;                          }

  Void    setBitRateValueMinus1( Int layer, Int cpbcnt, Int nalOrVcl, UInt value )   { m_HRD[layer].bitRateValueMinus1[cpbcnt][nalOrVcl] = value; }
  UInt    getBitRateValueMinus1( Int layer, Int cpbcnt, Int nalOrVcl ) const         { return m_HRD[layer].bitRateValueMinus1[cpbcnt][nalOrVcl];  }

  Void    setCpbSizeValueMinus1( Int layer, Int cpbcnt, Int nalOrVcl, UInt value )   { m_HRD[layer].cpbSizeValue[cpbcnt][nalOrVcl] = value;       }
  UInt    getCpbSizeValueMinus1( Int layer, Int cpbcnt, Int nalOrVcl ) const         { return m_HRD[layer].cpbSizeValue[cpbcnt][nalOrVcl];        }
  Void    setDuCpbSizeValueMinus1( Int layer, Int cpbcnt, Int nalOrVcl, UInt value ) { m_HRD[layer].ducpbSizeValue[cpbcnt][nalOrVcl] = value;     }
  UInt    getDuCpbSizeValueMinus1( Int layer, Int cpbcnt, Int nalOrVcl ) const       { return m_HRD[layer].ducpbSizeValue[cpbcnt][nalOrVcl];      }
  Void    setDuBitRateValueMinus1( Int layer, Int cpbcnt, Int nalOrVcl, UInt value ) { m_HRD[layer].duBitRateValue[cpbcnt][nalOrVcl] = value;     }
  UInt    getDuBitRateValueMinus1(Int layer, Int cpbcnt, Int nalOrVcl ) const        { return m_HRD[layer].duBitRateValue[cpbcnt][nalOrVcl];      }
  Void    setCbrFlag( Int layer, Int cpbcnt, Int nalOrVcl, Bool value )              { m_HRD[layer].cbrFlag[cpbcnt][nalOrVcl] = value;            }
  Bool    getCbrFlag( Int layer, Int cpbcnt, Int nalOrVcl ) const                    { return m_HRD[layer].cbrFlag[cpbcnt][nalOrVcl];             }

  Bool    getCpbDpbDelaysPresentFlag( ) const                      { return getNalHrdParametersPresentFlag() || getVclHrdParametersPresentFlag(); }
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
  : m_timingInfoPresentFlag      (false)
  , m_numUnitsInTick             (1001)
  , m_timeScale                  (60000)
  , m_pocProportionalToTimingFlag(false)
  , m_numTicksPocDiffOneMinus1   (0)
  {}

  Void setTimingInfoPresentFlag( Bool flag )   { m_timingInfoPresentFlag = flag;       }
  Bool getTimingInfoPresentFlag( ) const       { return m_timingInfoPresentFlag;       }

  Void setNumUnitsInTick( UInt value )         { m_numUnitsInTick = value;             }
  UInt getNumUnitsInTick( ) const              { return m_numUnitsInTick;              }

  Void setTimeScale( UInt value )              { m_timeScale = value;                  }
  UInt getTimeScale( ) const                   { return m_timeScale;                   }

  Void setPocProportionalToTimingFlag(Bool x)  { m_pocProportionalToTimingFlag = x;    }
  Bool getPocProportionalToTimingFlag( ) const { return m_pocProportionalToTimingFlag; }

  Void setNumTicksPocDiffOneMinus1(Int x)      { m_numTicksPocDiffOneMinus1 = x;       }
  Int  getNumTicksPocDiffOneMinus1( ) const    { return m_numTicksPocDiffOneMinus1;    }
};

struct ChromaQpAdj
{
  union
  {
    struct {
      Int CbOffset;
      Int CrOffset;
    } comp;
    Int offset[2]; /* two chroma components */
  } u;
};

#if NH_MV

class TComVideoSignalInfo
{
private: 
  Int  m_videoVpsFormat;
  Bool m_videoFullRangeVpsFlag;
  Int  m_colourPrimariesVps;
  Int  m_transferCharacteristicsVps;
  Int  m_matrixCoeffsVps;
public: 
  Void    setVideoVpsFormat( Int  val )                                              { m_videoVpsFormat = val;                                    }
  Int     getVideoVpsFormat(  ) const                                                { return m_videoVpsFormat;                                   }

  Void    setVideoFullRangeVpsFlag( Bool flag )                                      { m_videoFullRangeVpsFlag = flag;                            }
  Bool    getVideoFullRangeVpsFlag(  ) const                                         { return m_videoFullRangeVpsFlag;                            }

  Void    setColourPrimariesVps( Int  val )                                          { m_colourPrimariesVps = val;                                }
  Int     getColourPrimariesVps(  ) const                                            { return m_colourPrimariesVps;                               }

  Void    setTransferCharacteristicsVps( Int  val )                                  { m_transferCharacteristicsVps = val;                        }
  Int     getTransferCharacteristicsVps(  ) const                                    { return m_transferCharacteristicsVps;                       }

  Void    setMatrixCoeffsVps( Int  val )                                             { m_matrixCoeffsVps = val;                                   }
  Int     getMatrixCoeffsVps(  ) const                                               { return m_matrixCoeffsVps;                                  }
};

class TComVpsVuiBspHrdParameters
{
  /* Not yet tested */
private: 

  Int       m_vpsNumAddHrdParams;
  BoolAry1d m_cprmsAddPresentFlag;
  std::vector<TComHRD> m_hrdParameters; 
  IntAry1d  m_numSubLayerHrdMinus1;
  IntAry1d  m_numSignalledPartitioningSchemes;
  IntAry2d  m_numPartitionsInSchemeMinus1;

  BoolAry4d m_layerIncludedInPartitionFlag;
  IntAry3d  m_numBspSchedulesMinus1;
  IntAry5d  m_bspHrdIdx;
  IntAry5d  m_bspSchedIdx;
    
  // Array sizes
  Int      m_offsetHrdParamIdx;
  Int      m_numHrdParam; 
  Int      m_numOls; 
  TComVPS* m_vps; 
public:     
  
  Void    createAfterVpsNumAddHrdParams( const TComVPS* vps );
  Void    createAfterNumSignalledPartitioningSchemes(const TComVPS* vps, Int h );
  Void    createAfterNumPartitionsInSchemeMinus1(const TComVPS* vps, Int h, Int j);
  Void    createAfterNumBspSchedulesMinus1(const TComVPS* vps, Int h, Int i, Int t);

  Void    setVpsNumAddHrdParams( Int  val )                                          { m_vpsNumAddHrdParams = val;                                }
  Int     getVpsNumAddHrdParams(  ) const                                            { return m_vpsNumAddHrdParams;                               }

  Void    setCprmsAddPresentFlag( Int i, Bool flag )                                 { m_cprmsAddPresentFlag[i - m_offsetHrdParamIdx] = flag;     }
  Bool    getCprmsAddPresentFlag( Int i ) const                                      { return m_cprmsAddPresentFlag[i  - m_offsetHrdParamIdx];    }

  Void    setNumSubLayerHrdMinus1( Int i, Int  val )                                 { m_numSubLayerHrdMinus1[i  - m_offsetHrdParamIdx] = val;    }
  Int     getNumSubLayerHrdMinus1( Int i ) const                                     { return m_numSubLayerHrdMinus1[i  - m_offsetHrdParamIdx];   }

  Void    setNumSignalledPartitioningSchemes( Int h, Int  val )                      { m_numSignalledPartitioningSchemes[h] = val;                }
  Int     getNumSignalledPartitioningSchemes( Int h ) const                          { return m_numSignalledPartitioningSchemes[h];               }

  Void    setNumPartitionsInSchemeMinus1( Int h, Int j, Int  val )                   { m_numPartitionsInSchemeMinus1[h][j] = val;                 }
  Int     getNumPartitionsInSchemeMinus1( Int h, Int j ) const                       { return m_numPartitionsInSchemeMinus1[h][j];                }

  Void    setLayerIncludedInPartitionFlag( Int h, Int j, Int k, Int r, Bool flag )   { m_layerIncludedInPartitionFlag[h][j][k][r] = flag;         }
  Bool    getLayerIncludedInPartitionFlag( Int h, Int j, Int k, Int r ) const        { return m_layerIncludedInPartitionFlag[h][j][k][r];         }

  Void    setNumBspSchedulesMinus1( Int h, Int i, Int t, Int  val )                  { m_numBspSchedulesMinus1[h][i][t] = val;                    }
  Int     getNumBspSchedulesMinus1( Int h, Int i, Int t ) const                      { return m_numBspSchedulesMinus1[h][i][t];                   }

  Void    setBspHrdIdx( Int h, Int i, Int t, Int j, Int k, Int  val )                { m_bspHrdIdx[h][i  - m_offsetHrdParamIdx][t][j][k] = val;   }
  Int     getBspHrdIdx( Int h, Int i, Int t, Int j, Int k ) const                    { return m_bspHrdIdx[h][i  - m_offsetHrdParamIdx][t][j][k];  }

  Int     getBspHrdIdxLen ( const TComVPS* vps ) const ;

  Void    setBspSchedIdx( Int h, Int i, Int t, Int j, Int k, Int  val )              { m_bspSchedIdx[h][i - m_offsetHrdParamIdx][t][j][k] = val;  }
  Int     getBspSchedIdx( Int h, Int i, Int t, Int j, Int k ) const                  { return m_bspSchedIdx[h][i - m_offsetHrdParamIdx][t][j][k]; }

  Void    setHrdParametermeters( Int k, TComHRD val  )                               {  m_hrdParameters[k] = val;                                 }
  const   TComHRD* getHrdParametermeters( Int k ) const                              {  return &m_hrdParameters[k];                               }
};

class TComVPSVUI
{
private:
  Bool                               m_crossLayerPicTypeAlignedFlag          ;
  Bool                               m_crossLayerIrapAlignedFlag             ;
  Bool                               m_allLayersIdrAlignedFlag               ;
  Bool                               m_bitRatePresentVpsFlag                 ;
  Bool                               m_picRatePresentVpsFlag                 ;                                     
  BoolAry2d                          m_bitRatePresentFlag                    ;
  BoolAry2d                          m_picRatePresentFlag                    ;
  IntAry2d                           m_avgBitRate                            ;
  IntAry2d                           m_maxBitRate                            ;
  IntAry2d                           m_constantPicRateIdc                    ;
  IntAry2d                           m_avgPicRate                            ;
  Bool                               m_videoSignalInfoIdxPresentFlag         ;
  Int                                m_vpsNumVideoSignalInfoMinus1           ;
  std::vector< TComVideoSignalInfo > m_videoSignalInfo                       ;
  IntAry1d                           m_vpsVideoSignalInfoIdx                 ;
  Bool                               m_tilesNotInUseFlag;                    ;
  BoolAry1d                          m_tilesInUseFlag                        ; 
  BoolAry1d                          m_loopFilterNotAcrossTilesFlag          ; 
  BoolAry2d                          m_tileBoundariesAlignedFlag             ;
  Bool                               m_wppNotInUseFlag                       ;             
  BoolAry1d                          m_wppInUseFlag                          ;                
  Bool                               m_singleLayerForNonIrapFlag             ;
  Bool                               m_higherLayerIrapSkipFlag               ;
  Bool                               m_ilpRestrictedRefLayersFlag            ;
  IntAry2d                           m_minSpatialSegmentOffsetPlus1          ;
  BoolAry2d                          m_ctuBasedOffsetEnabledFlag             ;
  IntAry2d                           m_minHorizontalCtuOffsetPlus1           ;
  Bool                               m_vpsVuiBspHrdPresentFlag               ;
  TComVpsVuiBspHrdParameters         m_vpsVuiBspHrdParameters                ; 
  BoolAry1d                          m_baseLayerParameterSetCompatibilityFlag;

public: 

  Void    init( Int numLayerSets, Int maxNumSubLayers, Int maxNumLayers );

  Void    setCrossLayerPicTypeAlignedFlag( Bool flag )                               { m_crossLayerPicTypeAlignedFlag = flag;                     }
  Bool    getCrossLayerPicTypeAlignedFlag(  ) const                                  { return m_crossLayerPicTypeAlignedFlag;                     }

  Void    setCrossLayerIrapAlignedFlag( Bool flag )                                  { m_crossLayerIrapAlignedFlag = flag;                        }
  Bool    getCrossLayerIrapAlignedFlag(  ) const                                     { return m_crossLayerIrapAlignedFlag;                        }

  Void    setAllLayersIdrAlignedFlag( Bool flag )                                    { m_allLayersIdrAlignedFlag = flag;                          }
  Bool    getAllLayersIdrAlignedFlag(  ) const                                       { return m_allLayersIdrAlignedFlag;                          }

  Void    setBitRatePresentVpsFlag( Bool flag )                                      { m_bitRatePresentVpsFlag = flag;                            }
  Bool    getBitRatePresentVpsFlag(  ) const                                         { return m_bitRatePresentVpsFlag;                            }

  Void    setPicRatePresentVpsFlag( Bool flag )                                      { m_picRatePresentVpsFlag = flag;                            }
  Bool    getPicRatePresentVpsFlag(  ) const                                         { return m_picRatePresentVpsFlag;                            }

  Void    setBitRatePresentFlag( Int i, Int j, Bool flag )                           { m_bitRatePresentFlag[i][j] = flag;                         }
  Bool    getBitRatePresentFlag( Int i, Int j ) const                                { return m_bitRatePresentFlag[i][j];                         }

  Void    setPicRatePresentFlag( Int i, Int j, Bool flag )                           { m_picRatePresentFlag[i][j] = flag;                         }
  Bool    getPicRatePresentFlag( Int i, Int j ) const                                { return m_picRatePresentFlag[i][j];                         }

  Void    setAvgBitRate( Int i, Int j, Int  val )                                    { m_avgBitRate[i][j] = val;                                  }
  Int     getAvgBitRate( Int i, Int j ) const                                        { return m_avgBitRate[i][j];                                 }

  Void    setMaxBitRate( Int i, Int j, Int  val )                                    { m_maxBitRate[i][j] = val;                                  }
  Int     getMaxBitRate( Int i, Int j ) const                                        { return m_maxBitRate[i][j];                                 }

  Void    setConstantPicRateIdc( Int i, Int j, Int  val )                            { m_constantPicRateIdc[i][j] = val;                          }
  Int     getConstantPicRateIdc( Int i, Int j ) const                                { return m_constantPicRateIdc[i][j];                         }

  Void    setAvgPicRate( Int i, Int j, Int  val )                                    { m_avgPicRate[i][j] = val;                                  }
  Int     getAvgPicRate( Int i, Int j ) const                                        { return m_avgPicRate[i][j];                                 }

  Void    setVideoSignalInfoIdxPresentFlag( Bool flag )                              { m_videoSignalInfoIdxPresentFlag = flag;                    }
  Bool    getVideoSignalInfoIdxPresentFlag(  ) const                                 { return m_videoSignalInfoIdxPresentFlag;                    }

  Void    setVideoSignalInfo( std::vector<TComVideoSignalInfo> val )                  { m_videoSignalInfo = val;                                  }
  const   TComVideoSignalInfo* getVideoSignalInfo( Int i ) const                      { return &m_videoSignalInfo[i];                             }

  Void    setVpsNumVideoSignalInfoMinus1( Int  val )                                 { m_vpsNumVideoSignalInfoMinus1 = val;                       }
  Int     getVpsNumVideoSignalInfoMinus1(  ) const                                   { return m_vpsNumVideoSignalInfoMinus1;                      }

  Void    setVpsVideoSignalInfoIdx( Int i, Int  val )                                { m_vpsVideoSignalInfoIdx[i] = val;                          }
  Int     getVpsVideoSignalInfoIdx( Int i ) const                                    { return m_vpsVideoSignalInfoIdx[i];                         }

  Void    setTilesNotInUseFlag( Bool flag )                                          { m_tilesNotInUseFlag = flag;                                }
  Bool    getTilesNotInUseFlag(  ) const                                             { return m_tilesNotInUseFlag;                                }

  Void    setTilesInUseFlag( Int i, Bool flag )                                      { m_tilesInUseFlag[i] = flag;                                }
  Bool    getTilesInUseFlag( Int i ) const                                           { return m_tilesInUseFlag[i];                                }

  Void    setLoopFilterNotAcrossTilesFlag( Int i, Int  val )                         { m_loopFilterNotAcrossTilesFlag[i] = val;                   }
  Bool    getLoopFilterNotAcrossTilesFlag( Int i ) const                             { return m_loopFilterNotAcrossTilesFlag[i];                  }

  Void    setTileBoundariesAlignedFlag( Int i, Int j, Bool flag )                    { m_tileBoundariesAlignedFlag[i][j] = flag;                  }
  Bool    getTileBoundariesAlignedFlag( Int i, Int j ) const                         { return m_tileBoundariesAlignedFlag[i][j];                  }

  Void    setWppNotInUseFlag( Bool flag )                                            { m_wppNotInUseFlag = flag;                                  }
  Bool    getWppNotInUseFlag(  ) const                                               { return m_wppNotInUseFlag;                                  }

  Void    setWppInUseFlag( Int i, Bool flag )                                        { m_wppInUseFlag[i] = flag;                                  }
  Bool    getWppInUseFlag( Int i ) const                                             { return m_wppInUseFlag[i];                                  }

  Void    setSingleLayerForNonIrapFlag( Bool flag )                                  { m_singleLayerForNonIrapFlag = flag;                        }
  Bool    getSingleLayerForNonIrapFlag(  ) const                                     { return m_singleLayerForNonIrapFlag;                        }

  Void    setHigherLayerIrapSkipFlag( Bool flag )                                    { m_higherLayerIrapSkipFlag = flag;                          }
  Bool    getHigherLayerIrapSkipFlag(  ) const                                       { return m_higherLayerIrapSkipFlag;                          }

  Void    setIlpRestrictedRefLayersFlag( Bool flag )                                 { m_ilpRestrictedRefLayersFlag = flag;                       }
  Bool    getIlpRestrictedRefLayersFlag(  ) const                                    { return m_ilpRestrictedRefLayersFlag;                       }

  Void    setMinSpatialSegmentOffsetPlus1( Int i, Int j, Int  val )                  { m_minSpatialSegmentOffsetPlus1[i][j] = val;                }
  Int     getMinSpatialSegmentOffsetPlus1( Int i, Int j ) const                      { return m_minSpatialSegmentOffsetPlus1[i][j];               }

  Void    setCtuBasedOffsetEnabledFlag( Int i, Int j, Bool flag )                    { m_ctuBasedOffsetEnabledFlag[i][j] = flag;                  }
  Bool    getCtuBasedOffsetEnabledFlag( Int i, Int j ) const                         { return m_ctuBasedOffsetEnabledFlag[i][j];                  }

  Void    setMinHorizontalCtuOffsetPlus1( Int i, Int j, Int  val )                   { m_minHorizontalCtuOffsetPlus1[i][j] = val;                 }
  Int     getMinHorizontalCtuOffsetPlus1( Int i, Int j ) const                       { return m_minHorizontalCtuOffsetPlus1[i][j];                }

  Void    setVpsVuiBspHrdPresentFlag( Bool flag )                                    { m_vpsVuiBspHrdPresentFlag = flag;                          }
  Bool    getVpsVuiBspHrdPresentFlag(  ) const                                       { return m_vpsVuiBspHrdPresentFlag;                          }

  Void    setVpsVuiBspHrdParameters( TComVpsVuiBspHrdParameters val)                 {  m_vpsVuiBspHrdParameters = val;                           }
  const   TComVpsVuiBspHrdParameters* getVpsVuiBspHrdParameters(  ) const            { return &m_vpsVuiBspHrdParameters;                          }

  Void    setBaseLayerParameterSetCompatibilityFlag( Int i, Bool flag )              { m_baseLayerParameterSetCompatibilityFlag[i] = flag;        }
  Bool    getBaseLayerParameterSetCompatibilityFlag( Int i ) const                   { return m_baseLayerParameterSetCompatibilityFlag[i];        }
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
  Bool m_conformanceWindowVpsFlag;
  Int  m_confWinVpsLeftOffset;
  Int  m_confWinVpsRightOffset;
  Int  m_confWinVpsTopOffset;
  Int  m_confWinVpsBottomOffset;

public: 

  TComRepFormat()
  { 
    m_conformanceWindowVpsFlag = 0;
    m_confWinVpsLeftOffset     = 0;
    m_confWinVpsRightOffset    = 0;
    m_confWinVpsTopOffset      = 0;
    m_confWinVpsBottomOffset   = 0;
  };  

  Void    setChromaAndBitDepthVpsPresentFlag( Bool flag )                            { m_chromaAndBitDepthVpsPresentFlag = flag;                  }
  Bool    getChromaAndBitDepthVpsPresentFlag(  ) const                               { return m_chromaAndBitDepthVpsPresentFlag;                  }
  Void    checkChromaAndBitDepthVpsPresentFlag( Int i ) const                        { assert( i != 0 || m_chromaAndBitDepthVpsPresentFlag );     } // The value of chroma_and_bit_depth_vps_present_flag of the first rep_format( ) syntax structure in the VPS shall be equal to 1.

  Void    inferChromaAndBitDepth     ( const TComRepFormat* prevRepFormat );
  Void    checkInferChromaAndBitDepth( const TComRepFormat* prevRepFormat ) const;

  Void    setChromaFormatVpsIdc( Int  val )                                          { m_chromaFormatVpsIdc = val;                                }
  Int     getChromaFormatVpsIdc(  ) const                                            { return m_chromaFormatVpsIdc;                               }

  Void    setSeparateColourPlaneVpsFlag( Bool flag )                                 { m_separateColourPlaneVpsFlag = flag;                       }
  Bool    getSeparateColourPlaneVpsFlag(  ) const                                    { return m_separateColourPlaneVpsFlag;                       }

  Void    setPicWidthVpsInLumaSamples( Int  val )                                    { m_picWidthVpsInLumaSamples = val;                          }
  Int     getPicWidthVpsInLumaSamples(  ) const                                      { return m_picWidthVpsInLumaSamples;                         }

  Void    setPicHeightVpsInLumaSamples( Int  val )                                   { m_picHeightVpsInLumaSamples = val;                         }
  Int     getPicHeightVpsInLumaSamples(  ) const                                     { return m_picHeightVpsInLumaSamples;                        }

  Void    setBitDepthVpsLumaMinus8( Int  val )                                       { m_bitDepthVpsLumaMinus8 = val;                             }
  Int     getBitDepthVpsLumaMinus8(  ) const                                         { return m_bitDepthVpsLumaMinus8;                            }

  Void    setBitDepthVpsChromaMinus8( Int  val )                                     { m_bitDepthVpsChromaMinus8 = val;                           }
  Int     getBitDepthVpsChromaMinus8(  ) const                                       { return m_bitDepthVpsChromaMinus8;                          }

  Void    setConformanceWindowVpsFlag( Bool flag )                                   { m_conformanceWindowVpsFlag = flag;                         }
  Bool    getConformanceWindowVpsFlag(  ) const                                      { return m_conformanceWindowVpsFlag;                         }

  Void    setConfWinVpsLeftOffset( Int  val )                                        { m_confWinVpsLeftOffset = val;                              }
  Int     getConfWinVpsLeftOffset(  ) const                                          { return m_confWinVpsLeftOffset;                             }

  Void    setConfWinVpsRightOffset( Int  val )                                       { m_confWinVpsRightOffset = val;                             }
  Int     getConfWinVpsRightOffset(  ) const                                         { return m_confWinVpsRightOffset;                            }

  Void    setConfWinVpsTopOffset( Int  val )                                         { m_confWinVpsTopOffset = val;                               }
  Int     getConfWinVpsTopOffset(  ) const                                           { return m_confWinVpsTopOffset;                              }

  Void    setConfWinVpsBottomOffset( Int  val )                                      { m_confWinVpsBottomOffset = val;                            }
  Int     getConfWinVpsBottomOffset(  ) const                                        { return m_confWinVpsBottomOffset;                           }
};


class TComDpbSize
{
private:
  BoolAry1d  m_subLayerFlagInfoPresentFlag;
  BoolAry2d  m_subLayerDpbInfoPresentFlag ;
  IntAry3d   m_maxVpsDecPicBufferingMinus1;
  IntAry2d   m_maxVpsNumReorderPics       ;
  IntAry2d   m_maxVpsLatencyIncreasePlus1 ;

public: 
  TComDpbSize() {}; 

  Void          init( Int numOutputLayerSets, Int maxNumLayerIds, Int maxNumSubLayers );

  Void          setSubLayerFlagInfoPresentFlag( Int i, Bool flag )                   { m_subLayerFlagInfoPresentFlag[i] = flag;                   }
  Bool          getSubLayerFlagInfoPresentFlag( Int i ) const                        { return m_subLayerFlagInfoPresentFlag[i];                   }

  Void          setSubLayerDpbInfoPresentFlag( Int i, Int j, Bool flag )             { m_subLayerDpbInfoPresentFlag[i][j] = flag;                 }
  Bool          getSubLayerDpbInfoPresentFlag( Int i, Int j ) const                  { return m_subLayerDpbInfoPresentFlag[i][j];                 }

  Void          setMaxVpsDecPicBufferingMinus1( Int i, Int k, Int j, Int  val )      { m_maxVpsDecPicBufferingMinus1[i][k][j] = val;              }
  Int           getMaxVpsDecPicBufferingMinus1( Int i, Int k, Int j ) const          { assert( m_maxVpsDecPicBufferingMinus1[i][k][j] >= 0 ); return m_maxVpsDecPicBufferingMinus1[i][k][j]; }

  Void          setMaxVpsNumReorderPics( Int i, Int j, Int  val )                    { m_maxVpsNumReorderPics[i][j] = val;                        }
  Int           getMaxVpsNumReorderPics( Int i, Int j ) const                        { return m_maxVpsNumReorderPics[i][j];                       }

  Void          setMaxVpsLatencyIncreasePlus1( Int i, Int j, Int  val )              { m_maxVpsLatencyIncreasePlus1[i][j] = val;                  }
  Int           getMaxVpsLatencyIncreasePlus1( Int i, Int j ) const                  { return m_maxVpsLatencyIncreasePlus1[i][j];                 }
};
#endif

class TComVPS
{
private:
  Int                   m_VPSId;
#if NH_MV
  Bool                  m_vpsBaseLayerInternalFlag;
  Bool                  m_vpsBaseLayerAvailableFlag;
#endif

  UInt                  m_uiMaxTLayers;

#if NH_MV
  UInt                  m_uiMaxLayersMinus1;
#else
  UInt                  m_uiMaxLayers;
#endif
  Bool                  m_bTemporalIdNestingFlag;

  UInt                  m_numReorderPics[MAX_TLAYER];
  UInt                  m_uiMaxDecPicBuffering[MAX_TLAYER];
  UInt                  m_uiMaxLatencyIncrease[MAX_TLAYER]; // Really max latency increase plus 1 (value 0 expresses no limit)

  UInt                  m_numHrdParameters;
#if NH_MV
  UInt                  m_maxLayerId;
#else
  UInt                  m_maxNuhReservedZeroLayerId;
#endif
  std::vector<TComHRD>  m_hrdParameters;
  std::vector<UInt>     m_hrdOpSetIdx;
  std::vector<Bool>     m_cprmsPresentFlag;
#if NH_MV
  UInt                  m_vpsNumLayerSetsMinus1;
  Bool                  m_layerIdIncludedFlag[MAX_VPS_OP_SETS_PLUS1][MAX_VPS_NUH_LAYER_ID_PLUS1];
#else
  UInt                  m_numOpSets;
  Bool                  m_layerIdIncludedFlag[MAX_VPS_OP_SETS_PLUS1][MAX_VPS_NUH_RESERVED_ZERO_LAYER_ID_PLUS1];
#endif

#if NH_MV
  TComPTL               m_pcPTL[MAX_VPS_OP_SETS_PLUS1];
#else
  TComPTL               m_pcPTL;
#endif
  TimingInfo            m_timingInfo;
#if NH_MV
  Bool                  m_vpsExtensionFlag;

  /// VPS EXTENSION SYNTAX ELEMENTS
  Int         m_vpsNonVuiExtensionLength;
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
  Int         m_vpsNumProfileTierLevelMinus1;   
  Bool        m_vpsProfilePresentFlag    [MAX_VPS_OP_SETS_PLUS1];

  Int         m_numAddLayerSets;    
  Int         m_highestLayerIdxPlus1     [MAX_VPS_ADD_OUTPUT_LAYER_SETS][MAX_NUM_LAYERS];
  Int         m_numAddOlss;

  Int         m_defaultOutputLayerIdc;

  Int         m_layerSetIdxForOlsMinus1  [MAX_VPS_OUTPUTLAYER_SETS];  
  Bool        m_outputLayerFlag          [MAX_VPS_OUTPUTLAYER_SETS][MAX_VPS_NUH_LAYER_ID_PLUS1];
  Int         m_profileTierLevelIdx      [MAX_VPS_OUTPUTLAYER_SETS ][MAX_NUM_LAYERS];
  Bool        m_altOutputLayerFlag       [MAX_VPS_OUTPUTLAYER_SETS];
  Bool        m_repFormatIdxPresentFlag;

  Int         m_vpsNumRepFormatsMinus1;
  Int         m_vpsRepFormatIdx          [MAX_NUM_LAYERS];

  std::vector<TComRepFormat> m_repFormat; 
  Bool        m_maxOneActiveRefLayerFlag;       
  Bool        m_vpsPocLsbAlignedFlag;
  Bool        m_pocLsbNotPresentFlag     [MAX_NUM_LAYERS];

  TComDpbSize m_dpbSize; 
  Int         m_directDepTypeLenMinus2;         
  Bool        m_defaultDirectDependencyFlag;
  Int         m_defaultDirectDependencyType;
  
#if H_MV_HLS7_GEN
  Int         m_directDependencyType     [MAX_NUM_LAYERS] [MAX_NUM_LAYERS];
#endif
  Bool        m_vpsVuiPresentFlag;
  TComVPSVUI  m_vpsVUI; 
#if !H_MV_HLS7_GEN
  Int         m_directDependencyType     [MAX_NUM_LAYERS] [MAX_NUM_LAYERS];
#endif

  // VPS EXTENSION SEMANTICS VARIABLES
  Int         m_layerIdInVps             [MAX_NUM_LAYERS   ];
  Int         m_dependencyFlag           [MAX_NUM_LAYERS][MAX_NUM_LAYERS]; 

  Int         m_numViews; 
  Int         m_numDirectRefLayers       [MAX_NUM_LAYERS];
  Int         m_idDirectRefLayer         [MAX_NUM_LAYERS][MAX_NUM_LAYERS];  
#if NH_3D
  Int         m_numRefListLayers         [MAX_NUM_LAYERS];
  Int         m_idRefListLayer           [MAX_NUM_LAYERS][MAX_NUM_LAYERS];  
#endif


  Int         m_numRefLayers             [MAX_NUM_LAYER_IDS]; 
  Int         m_idRefLayer               [MAX_NUM_LAYERS][MAX_NUM_LAYERS];  


  Int         m_numPredictedLayers       [MAX_NUM_LAYERS ]; 
  Int         m_idPredictedLayer         [MAX_NUM_LAYERS][MAX_NUM_LAYER_IDS];
  Int         m_numIndependentLayers; 
  Int         m_numLayersInTreePartition [MAX_NUM_LAYER_IDS];
  Int         m_treePartitionLayerIdList [MAX_NUM_LAYERS][MAX_NUM_LAYER_IDS];
  Bool        m_recursiveRefLayerFlag    [MAX_NUM_LAYER_IDS][MAX_NUM_LAYER_IDS]; 
  Int         m_viewIndex                [MAX_NUM_LAYERS   ];
  
  IntAry2d    m_targetDecLayerIdLists;   //[TargetOptLayerSetIdx][i]
  IntAry2d    m_targetOptLayerIdLists; 
  IntAry2d    m_layerSetLayerIdList; 

  Int         m_numNecessaryLayers        [MAX_VPS_OUTPUTLAYER_SETS];
  Bool        m_necessaryLayerFlag        [MAX_VPS_OUTPUTLAYER_SETS][MAX_NUM_LAYERS]; 

  Int         xGetDimBitOffset( Int j ) const;
  Void        xSetRefLayerFlags( Int currLayerId );

  // VPS EXTENSION 2 SYNTAX ELEMENTS
#if NH_3D
  IntAry1d       m_numCp;  
  IntAry2d       m_cpRefVoi;
  BoolAry2d      m_cpPresentFlag; 
  Int            m_cpPrecision;
  BoolAry1d      m_cpInSliceSegmentHeaderFlag;
  IntAry3d       m_aaaiCodedScale ;
  IntAry3d       m_aaaiCodedOffset;

  IntAry1d       m_viewOIdxList;
  BoolAry2d      m_viewCompLayerPresentFlag;
  IntAry2d       m_viewCompLayerId;
#endif

#endif

public:
                    TComVPS();

  virtual           ~TComVPS();

  Void              createHrdParamBuffer()
  {
    m_hrdParameters.resize(getNumHrdParameters());
    m_hrdOpSetIdx.resize(getNumHrdParameters());
    m_cprmsPresentFlag.resize(getNumHrdParameters());
  }

  TComHRD*          getHrdParameters( UInt i )                           { return &m_hrdParameters[ i ];                                    }
  const TComHRD*    getHrdParameters( UInt i ) const                     { return &m_hrdParameters[ i ];                                    }
  UInt              getHrdOpSetIdx( UInt i ) const                       { return m_hrdOpSetIdx[ i ];                                       }
  Void              setHrdOpSetIdx( UInt val, UInt i )                   { m_hrdOpSetIdx[ i ] = val;                                        }
  Bool              getCprmsPresentFlag( UInt i ) const                  { return m_cprmsPresentFlag[ i ];                                  }
  Void              setCprmsPresentFlag( Bool val, UInt i )              { m_cprmsPresentFlag[ i ] = val;                                   }

  Int               getVPSId() const                                     { return m_VPSId;                                                  }
  Void              setVPSId(Int i)                                      { m_VPSId = i;                                                     }
#if NH_MV
  Void              setVpsBaseLayerInternalFlag( Bool flag )             { m_vpsBaseLayerInternalFlag = flag;                               } 
  Bool              getVpsBaseLayerInternalFlag(  )           const      { return m_vpsBaseLayerInternalFlag;                               } 
  Void              setVpsBaseLayerAvailableFlag( Bool flag )            { m_vpsBaseLayerAvailableFlag = flag;                              } 
  Bool              getVpsBaseLayerAvailableFlag(  )          const      { return m_vpsBaseLayerAvailableFlag;                              } 
#endif

  UInt              getMaxTLayers() const                                { return m_uiMaxTLayers;                                           }
  Void              setMaxTLayers(UInt t)                                { m_uiMaxTLayers = t;                                              }
#if NH_MV    
  UInt              getMaxSubLayersMinus1()            const             { return m_uiMaxTLayers - 1;   }  // For consistency with draft spec
  Void              setMaxSubLayersMinus1(UInt val)                      { m_uiMaxTLayers = (val + 1);  }
  UInt              getMaxLayersMinus1()               const             { return m_uiMaxLayersMinus1;  } 
  Void              setMaxLayersMinus1(UInt l)                           { m_uiMaxLayersMinus1 = l;     }
#else

  UInt              getMaxLayers() const                                 { return m_uiMaxLayers;                                            }
  Void              setMaxLayers(UInt l)                                 { m_uiMaxLayers = l;                                               }
#endif

  Bool              getTemporalNestingFlag() const                       { return m_bTemporalIdNestingFlag;                                 }
  Void              setTemporalNestingFlag(Bool t)                       { m_bTemporalIdNestingFlag = t;                                    }

  Void              setNumReorderPics(UInt v, UInt tLayer)               { m_numReorderPics[tLayer] = v;                                    }
  UInt              getNumReorderPics(UInt tLayer) const                 { return m_numReorderPics[tLayer];                                 }

  Void              setMaxDecPicBuffering(UInt v, UInt tLayer)           { assert(tLayer < MAX_TLAYER); m_uiMaxDecPicBuffering[tLayer] = v; }
  UInt              getMaxDecPicBuffering(UInt tLayer) const             { return m_uiMaxDecPicBuffering[tLayer];                           }

  Void              setMaxLatencyIncrease(UInt v, UInt tLayer)           { m_uiMaxLatencyIncrease[tLayer] = v;                              }
  UInt              getMaxLatencyIncrease(UInt tLayer) const             { return m_uiMaxLatencyIncrease[tLayer];                           }

  UInt              getNumHrdParameters() const                          { return m_numHrdParameters;                                       }
  Void              setNumHrdParameters(UInt v)                          { m_numHrdParameters = v;                                          }
#if NH_MV
  UInt              getVpsMaxLayerId()                           const   { return m_maxLayerId; }
  Void              setVpsMaxLayerId(UInt v)                             { m_maxLayerId = v;    }
                    
  UInt              getVpsNumLayerSetsMinus1()                   const   { return m_vpsNumLayerSetsMinus1; }
  Void              setVpsNumLayerSetsMinus1(UInt v)                     { m_vpsNumLayerSetsMinus1 = v;    }
#else  
  UInt              getMaxNuhReservedZeroLayerId() const                 { return m_maxNuhReservedZeroLayerId;                              }
  Void              setMaxNuhReservedZeroLayerId(UInt v)                 { m_maxNuhReservedZeroLayerId = v;                                 }

  UInt              getMaxOpSets() const                                 { return m_numOpSets;                                              }
  Void              setMaxOpSets(UInt v)                                 { m_numOpSets = v;                                                 }
#endif
  Bool              getLayerIdIncludedFlag(UInt opsIdx, UInt id) const   { return m_layerIdIncludedFlag[opsIdx][id];                        }
  Void              setLayerIdIncludedFlag(Bool v, UInt opsIdx, UInt id) { m_layerIdIncludedFlag[opsIdx][id] = v;                           }

#if NH_MV
  TComPTL*          getPTL( Int idx = 0 )                                { return &m_pcPTL[idx];                                            }
  const TComPTL*    getPTL( Int idx = 0 ) const                          { return &m_pcPTL[idx];                                            }
#else
  TComPTL*          getPTL()                                             { return &m_pcPTL;                                                 }
  const TComPTL*    getPTL() const                                       { return &m_pcPTL;                                                 }
#endif
  TimingInfo*       getTimingInfo()                                      { return &m_timingInfo;                                            }
  const TimingInfo* getTimingInfo() const                                { return &m_timingInfo;                                            }

#if NH_MV
  Void    setVpsExtensionFlag( Bool flag )                                      { m_vpsExtensionFlag = flag; } 
  Bool    getVpsExtensionFlag(  )                                   const       { return m_vpsExtensionFlag; } 

  Void    setVpsNonVuiExtensionLength( Int  val )                               { m_vpsNonVuiExtensionLength = val; } 
  Int     getVpsNonVuiExtensionLength(  )                           const       { return m_vpsNonVuiExtensionLength; } 
  
  // VPS Extension
  Void    setSplittingFlag( Bool val )                                          { m_splittingFlag = val;  }
  Bool    getSplittingFlag()                                        const       { return m_splittingFlag; }

  Void    setScalabilityMaskFlag( UInt val );
  Void    setScalabilityMaskFlag( Int scalType, Bool val )                          { m_scalabilityMaskFlag[scalType] = val;  }
  Bool    getScalabilityMaskFlag( Int scalType )                    const           { return m_scalabilityMaskFlag[scalType]; }
  
  Int     getNumScalabilityTypes( ) const;                                

  Void    setDimensionIdLen( Int sIdx, Int val )                                { m_dimensionIdLen[sIdx] = val;  }
  Int     getDimensionIdLen( Int sIdx )                             const       { assert( m_dimensionIdLen[sIdx] > 0) ; return m_dimensionIdLen[sIdx]; }  

  Void    setVpsNuhLayerIdPresentFlag( Bool val )                               { m_vpsNuhLayerIdPresentFlag = val; }
  Bool    getVpsNuhLayerIdPresentFlag()                             const       { return m_vpsNuhLayerIdPresentFlag; }

  Void    setLayerIdInNuh( Int layerIdInVps, Int val )                          { m_layerIdInNuh[layerIdInVps] = val;  }
  Int     getLayerIdInNuh( Int layerIdInVps )                       const       { assert( m_layerIdInNuh[layerIdInVps] >= 0 ); return m_layerIdInNuh[layerIdInVps]; }

  Bool    nuhLayerIdIncluded( Int layerIdinNuh )                    const       { return ( m_layerIdInVps[ layerIdinNuh ] > 0 );  }

  Void    setDimensionId( Int layerIdInVps, Int scalIdx, Int val )              { m_dimensionId[layerIdInVps][scalIdx] = val;  }
  Int     getDimensionId( Int layerIdInVps, Int scalIdx )           const       { return m_dimensionId[layerIdInVps][scalIdx]; }

  Void    setViewIdLen( Int  val )                                              { m_viewIdLen = val; } 
  Int     getViewIdLen(  )                                          const       { return m_viewIdLen; } 

  Void    setViewIdVal( Int viewOrderIndex, Int  val )                          { m_viewIdVal[viewOrderIndex] = val; } 
  Int     getViewIdVal( Int viewOrderIndex )                        const       { return m_viewIdVal[viewOrderIndex]; } 
  
  Void    setDirectDependencyFlag( Int depLayeridInVps, Int refLayeridInVps, Bool val )       { m_directDependencyFlag[depLayeridInVps][refLayeridInVps] = val;  }
  Bool    getDirectDependencyFlag( Int depLayeridInVps, Int refLayeridInVps )           const { return m_directDependencyFlag[depLayeridInVps][refLayeridInVps]; }
  
  Void    setVpsSubLayersMaxMinus1PresentFlag( Bool flag )                      { m_vpsSubLayersMaxMinus1PresentFlag = flag; } 
  Bool    getVpsSubLayersMaxMinus1PresentFlag(  )                    const      { return m_vpsSubLayersMaxMinus1PresentFlag; } 
  
  Void    setSubLayersVpsMaxMinus1( Int i, Int  val )                           { m_subLayersVpsMaxMinus1[i] = val; } 
  Int     getSubLayersVpsMaxMinus1( Int i )                          const      { return m_subLayersVpsMaxMinus1[i]; } 
  Void    checkSubLayersVpsMaxMinus1( Int i )                        const      { assert( m_subLayersVpsMaxMinus1[i] >= 0 && m_subLayersVpsMaxMinus1[i] <= m_uiMaxTLayers - 1 ); }

  Void    setMaxTidRefPresentFlag( Bool flag )                                  { m_maxTidRefPresentFlag = flag; } 
  Bool    getMaxTidRefPresentFlag(  )                                const      { return m_maxTidRefPresentFlag; } 

  Void    setMaxTidIlRefPicsPlus1( Int i, Int j, Int  val )                     { m_maxTidIlRefPicsPlus1[i][j] = val; } 
  Int     getMaxTidIlRefPicsPlus1( Int i, Int j )                    const      { return m_maxTidIlRefPicsPlus1[i][j]; } 
 
  Void    setAllRefLayersActiveFlag( Bool flag )                                { m_allRefLayersActiveFlag = flag; } 
  Bool    getAllRefLayersActiveFlag(  )                              const      { return m_allRefLayersActiveFlag; } 
  
  Void    setVpsNumProfileTierLevelMinus1( Int val )                            { m_vpsNumProfileTierLevelMinus1 = val;  } 
  Int     getVpsNumProfileTierLevelMinus1( )                         const      { return m_vpsNumProfileTierLevelMinus1; } 
  
  Void    setVpsProfilePresentFlag( Int idx, Bool val )                         { m_vpsProfilePresentFlag[idx] = val;  }
  Bool    getVpsProfilePresentFlag( Int idx )                        const      { return m_vpsProfilePresentFlag[idx]; }

  Void    setNumAddLayerSets( Int val )                                         { m_numAddLayerSets = val; } 
  Int     getNumAddLayerSets( )                                      const      { return m_numAddLayerSets; } 
  
  Void    setHighestLayerIdxPlus1( Int i, Int j, Int  val )                     { m_highestLayerIdxPlus1[i][j] = val; } 
  Int     getHighestLayerIdxPlus1( Int i, Int j )                    const      { return m_highestLayerIdxPlus1[i][j]; } 

  Void    setNumAddOlss( Int  val )                                             { m_numAddOlss = val; } 
  Int     getNumAddOlss(  )                                          const      { return m_numAddOlss; } 

  Void    setDefaultOutputLayerIdc( Int  val )                                  { m_defaultOutputLayerIdc = val; } 
  Int     getDefaultOutputLayerIdc(  )                               const      { return m_defaultOutputLayerIdc; }   
  
  Void    setLayerSetIdxForOlsMinus1( Int outLayerSetIdx, Int val )             { m_layerSetIdxForOlsMinus1[ outLayerSetIdx ]  = val; } 
  Int     getLayerSetIdxForOlsMinus1( Int outLayerSetIdx )           const      { return m_layerSetIdxForOlsMinus1[ outLayerSetIdx ]; } 
  Int     getLayerSetIdxForOlsMinus1Len( Int outLayerSetIdx )        const      { return gCeilLog2( getNumLayerSets() ); } 

  Void    setOutputLayerFlag( Int outLayerSetIdx, Int i, Bool flag )            { m_outputLayerFlag[ outLayerSetIdx ][ i ] = flag; } 
  Bool    getOutputLayerFlag( Int outLayerSetIdx, Int i )            const      { return m_outputLayerFlag[ outLayerSetIdx ][ i ]; } 

  Bool    inferOutputLayerFlag( Int i, Int j )                       const;

  Void    setProfileTierLevelIdx( Int i, Int j, Int val )                       { m_profileTierLevelIdx[ i ][ j ] = val; }
  Int     getProfileTierLevelIdx( Int i, Int j )                     const      { return m_profileTierLevelIdx[ i ][ j ]; } 
  Int     inferProfileTierLevelIdx( Int i, Int j )                   const; 
                                                                     
  Void    setAltOutputLayerFlag( Int i, Bool flag )                             { m_altOutputLayerFlag[i] = flag; } 
  Bool    getAltOutputLayerFlag( Int i )                             const      { return m_altOutputLayerFlag[i]; } 
                                                                     
  Void    setRepFormatIdxPresentFlag( Bool flag )                               { m_repFormatIdxPresentFlag = flag; } 
  Bool    getRepFormatIdxPresentFlag(  )                             const      { return m_repFormatIdxPresentFlag; } 
                                                                     
  Void    setVpsNumRepFormatsMinus1( Int  val )                                 { m_vpsNumRepFormatsMinus1 = val; } 
  Int     getVpsNumRepFormatsMinus1(  )                              const      { return m_vpsNumRepFormatsMinus1; } 
                                                                     
  Void    setVpsRepFormatIdx( Int i, Int  val )                                 { m_vpsRepFormatIdx[i] = val; } 
  Int     getVpsRepFormatIdx( Int i )                                const      { return m_vpsRepFormatIdx[i]; } 

  Int     inferVpsRepFormatIdx( Int i )                              const      { return std::min( i, getVpsNumRepFormatsMinus1()  );  }

  Void    setRepFormat( Int i, TComRepFormat val )                              { m_repFormat[i] = val;  }
  Void    setRepFormat( std::vector<TComRepFormat> val )                        { m_repFormat = val;  }
  const TComRepFormat* getRepFormat( Int i )                         const      { return &m_repFormat[i]; }
                                                                     
  Void    setMaxOneActiveRefLayerFlag( Bool flag)                               { m_maxOneActiveRefLayerFlag = flag; } 
  Bool    getMaxOneActiveRefLayerFlag( )                             const      { return m_maxOneActiveRefLayerFlag; } 
                                                                     
  Void    setVpsPocLsbAlignedFlag( Bool flag )                                  { m_vpsPocLsbAlignedFlag = flag; } 
  Bool    getVpsPocLsbAlignedFlag(  )                                const      { return m_vpsPocLsbAlignedFlag; } 
                                                                     
  Void    setDpbSize( TComDpbSize val )                                         { m_dpbSize = val; } 
  const TComDpbSize * getDpbSize( )                                  const      { return &m_dpbSize; } 
                                                                     
                                                                     
  Void    setPocLsbNotPresentFlag( Int i, Bool flag )                           { m_pocLsbNotPresentFlag[i] = flag; } 
  Bool    getPocLsbNotPresentFlag( Int i )                           const      { return m_pocLsbNotPresentFlag[i]; } 
                                                                     
  Void    setDirectDepTypeLenMinus2( Int val)                                   { m_directDepTypeLenMinus2 = val; } 
  Int     getDirectDepTypeLenMinus2( )                               const      { return m_directDepTypeLenMinus2; } 
                                                                     
  Void    setDefaultDirectDependencyFlag( Bool flag )                           { m_defaultDirectDependencyFlag = flag; } 
  Bool    getDefaultDirectDependencyFlag(  )                         const      { return m_defaultDirectDependencyFlag; } 
                                                                     
  Void    setDefaultDirectDependencyType( Int  val )                            { m_defaultDirectDependencyType = val; } 
  Int     getDefaultDirectDependencyType(  )                         const      { return m_defaultDirectDependencyType; } 
  
  Void    setDirectDependencyType( Int depLayeridInVps, Int refLayeridInVps, Int val) { m_directDependencyType[ depLayeridInVps ][ refLayeridInVps ] = val; } 
  Int     getDirectDependencyType( Int depLayeridInVps, Int refLayeridInVps) const { return m_directDependencyType[ depLayeridInVps ][ refLayeridInVps ]; } 

  Void    setVpsVuiPresentFlag( Bool flag )                                     { m_vpsVuiPresentFlag = flag; } 
  Bool    getVpsVuiPresentFlag(  )                                   const      { return m_vpsVuiPresentFlag; } 
                                                         
  const TComVPSVUI* getVPSVUI(  )                                    const      { return &m_vpsVUI;  }
  Void  setVPSVUI( TComVPSVUI val )                                             { m_vpsVUI = val;  }
 
 // VPS EXTENSION SEMANTICS VARIABLES
  Void    setLayerIdInVps( Int layerIdInNuh, Int val )                          { m_layerIdInVps[layerIdInNuh] = val;  }
  Int     getLayerIdInVps( Int layerIdInNuh )                        const      { assert( m_layerIdInVps[layerIdInNuh] >= 0 ); return m_layerIdInVps[layerIdInNuh]; }

  Int     getScalabilityId ( Int layerIdInVps, ScalabilityType scalType ) const ;
  Int     getViewId        ( Int layerIdInNuh )                 const           { return m_viewIdVal[ getViewIndex( layerIdInNuh )]; }
  Void    setRefLayers(); 

  // To be aligned with spec naming, getViewIndex will be removed in future versions
  Int     getViewOrderIdx ( Int layerIdInNuh ) const                            { return getScalabilityId( getLayerIdInVps(layerIdInNuh), VIEW_ORDER_INDEX  ); }    
  Int     getViewIndex    ( Int layerIdInNuh ) const                            { return getViewOrderIdx( layerIdInNuh ); }    
  Int     getAuxId        ( Int layerIdInNuh ) const                            { return getScalabilityId( getLayerIdInVps(layerIdInNuh), AUX_ID  ); }    
  Int     getDependencyId ( Int layerIdInNuh ) const                            { return getScalabilityId( getLayerIdInVps(layerIdInNuh), DEPENDENCY_ID  ); }    
  Int     getNumViews()                        const                            { return m_numViews; }
  Void    initNumViews();
#if NH_3D
  Void    initViewCompLayer( );
  Int     getViewOIdxList( Int i )             const                            { return m_viewOIdxList[i]; }
  std::vector<Int> getViewOIdxList( )          const                            { return m_viewOIdxList; }

  Int     getVoiInVps( Int viewOIdx )          const;

  Bool    getViewCompLayerPresentFlag (Int i, Bool d ) const                    { return  m_viewCompLayerPresentFlag[ getVoiInVps(i) ][d]; }
  Bool    getViewCompLayerId          (Int i, Bool d ) const                    { return  m_viewCompLayerId         [ getVoiInVps(i) ][d]; }
#endif
  Bool    getDependencyFlag( Int i, Int j ) const                               { return m_dependencyFlag[i][j]; }
  Int     getNumDirectRefLayers( Int layerIdInNuh ) const                       { return m_numDirectRefLayers[ layerIdInNuh ];  };                               
#if NH_3D                                                                      
  Int     getNumRefListLayers( Int layerIdInNuh )   const                       { return m_numRefListLayers[ layerIdInNuh ];  };                               
#endif

  Int     getNumRefLayers            ( Int i )      const                       { return m_numRefLayers[i]; } 
  Int     getNumPredictedLayers      ( Int i )      const                       { return m_numPredictedLayers[i]; } 
                                                                                
  Int     getIdRefLayer              ( Int i, Int j ) const                     { assert( j >= 0 && j < getNumRefLayers      ( i )); return m_idRefLayer      [i][j]; } 
  Int     getIdPredictedLayer        ( Int i, Int j ) const                     { assert( j >= 0 && j < getNumPredictedLayers( i )); return m_idPredictedLayer[i][j]; } 
  Int     getIdDirectRefLayer        ( Int i, Int j ) const                     { assert( j >= 0 && j < getNumDirectRefLayers( i )); return m_idDirectRefLayer[i][j]; } 
#if NH_3D                                                                      
  Int     getIdRefListLayer          ( Int i, Int j ) const                     { assert( j >= 0 && j < getNumRefListLayers   ( i )); return m_idRefListLayer[i][j]; } 
#endif                                                                          
  Int     getNumIndependentLayers    (  )             const                     { return m_numIndependentLayers; } 
  Int     getNumLayersInTreePartition( Int i )        const                     { return m_numLayersInTreePartition[i]; } 
  Int     getTreePartitionLayerIdList( Int i, Int j ) const                     { return m_treePartitionLayerIdList[i][j]; } 
  Bool    getRecursiveRefLayerFlag   ( Int i, Int j ) const                     { return m_recursiveRefLayerFlag[i][j]; } 
  Int     getNumLayerSets( )                          const                     { return getVpsNumLayerSetsMinus1() + 1 + getNumAddLayerSets();  };  
                                                                                
  Int     getFirstAddLayerSetIdx()                    const                     { return getVpsNumLayerSetsMinus1() + 1; } 
  Int     getLastAddLayerSetIdx()                     const                     { return getFirstAddLayerSetIdx() + getNumAddLayerSets() - 1; }
  Bool    checkVPSExtensionSyntax(); 
  Int     scalTypeToScalIdx   ( ScalabilityType scalType ) const ;

  Int     getProfileTierLevelIdxLen()                 const                     { return gCeilLog2( getVpsNumProfileTierLevelMinus1() + 1 ); };       
  Int     getVpsRepFormatIdxLen()                     const                     { return gCeilLog2( getVpsNumRepFormatsMinus1() + 1 ); };       

  Int     getNumLayersInIdList ( Int lsIdx )          const; 
  Int     getLayerSetLayerIdList(Int lsIdx, Int j )   const                     { return m_layerSetLayerIdList[ lsIdx ][ j ]; }; 

  Int     getNumOutputLayerSets()                     const; 

  Bool    isOutputLayer( Int outLayerSetIdx, Int layerIdInNuh )  const;    
  Void    deriveLayerSetLayerIdList();

  Int     olsIdxToLsIdx( Int i )                                       const    { return ( i < getNumLayerSets() ) ? i  : getLayerSetIdxForOlsMinus1( i ) + 1 ; };
  Void    initTargetLayerIdLists  ( );
  Void    deriveTargetLayerIdList ( Int i );
  std::vector<Int> getTargetDecLayerIdList( Int targetDecLayerSetIdx ) const     { return m_targetDecLayerIdLists[targetDecLayerSetIdx]; }; 
  std::vector<Int> getTargetOptLayerIdList( Int targetOptLayerSetIdx ) const     { return m_targetOptLayerIdLists[targetOptLayerSetIdx]; }; 

  Int     getNumOutputLayersInOutputLayerSet( Int i )                  const     { return (Int) getTargetOptLayerIdList( i ).size(); }; 
  Int     getOlsHighestOutputLayerId( Int i )                          const     { return getTargetOptLayerIdList( i ).back(); };  

  Void    deriveAddLayerSetLayerIdList( Int i );
  Void    deriveNecessaryLayerFlags( Int olsIdx );
  Int     getNecessaryLayerFlag( Int i, Int j )                        const     { AOF( i >= 0 && i < getNumOutputLayerSets() ); AOF( j >= 0 && j < getNumLayersInIdList( olsIdxToLsIdx( i ) )  );  return m_necessaryLayerFlag[i][j]; };  

  Int     getMaxSubLayersInLayerSetMinus1( Int i )                     const;
  Int     getHighestLayerIdxPlus1Len( Int j )                          const     { return gCeilLog2( getNumLayersInTreePartition( j ) + 1 );   }; 
  Bool    getAltOutputLayerFlagVar( Int i )                            const;

  // inference
  Int     inferDimensionId     ( Int i, Int j )                        const;
  Int     inferLastDimsionIdLenMinus1()                                const;

  // helpers
  Void    printPTL()                                                   const;
  Void    printLayerDependencies()                                     const;
  Void    printScalabilityId()                                         const;
  Void    printLayerSets()                                             const;


  /// VPS EXTENSION 2 SYNTAX ELEMENTS
  Int     getDepthId                   ( Int layerIdInNuh)             const    { return getScalabilityId( getLayerIdInVps(layerIdInNuh), DEPTH_ID ); }
#if NH_3D                                                             
  Bool    getVpsDepthFlag              ( Int layerIdInNuh)             const    { return (getDepthId( layerIdInNuh ) > 0);  }
  Int     getLayerIdInNuh              ( Int viewIndex, Bool depthFlag ) const;   
          
  Void    createCamPars                ( Int iNumViews );  
  Void    initCamParaVPS               ( Int vOIdxInVps, Int numCp, Bool cpInSliceSegmentHeaderFlag, Int* cpRefVoi, Int** aaiScale, Int** aaiOffset );                                                
                                       
  Void    setCpPrecision               ( Int  val )                             { m_cpPrecision = val;                       }
  Int     getCpPrecision               (  )                            const    { return m_cpPrecision;                      }
                                                                                                                
  Void    setNumCp                     ( Int i, Int  val )                      { m_numCp[i] = val;                          }
  Int     getNumCp                     ( Int i )                       const    { return m_numCp[i];                         }
                                                                                                                
  Void    setCpRefVoi                  ( Int i, Int m, Int  val )               { m_cpRefVoi[i][m] = val;                    }
  Int     getCpRefVoi                  ( Int i, Int m )                const    { return m_cpRefVoi[i][m];                   }
                                                                                                                
  Void    setCpInSliceSegmentHeaderFlag( Int i, Bool flag )                     { m_cpInSliceSegmentHeaderFlag[i] = flag;    }
  Bool    getCpInSliceSegmentHeaderFlag( Int i )                       const    { return m_cpInSliceSegmentHeaderFlag[i];    }
                                                                                                                
  Void    setVpsCpScale                ( Int i, Int j, Int val )                { m_aaaiCodedScale [i][0][j] = val;          }
  Int     getVpsCpScale                ( Int i, Int j )                const    { return m_aaaiCodedScale[i][0][j];          }
                                                                                                                
  Void    setVpsCpOff                  ( Int i, Int j, Int val )                { m_aaaiCodedOffset[i][0][j] = val;          }
  Int     getVpsCpOff                  ( Int i, Int j )                const    { return m_aaaiCodedOffset[i][0][j];         }
                                                                                                                
  Void    setVpsCpInvScale             ( Int i, Int j, Int val )                { m_aaaiCodedScale[i][1][j] = val;           }
  Int     getVpsCpInvScale             ( Int i, Int j )                const    { return m_aaaiCodedScale[i][1][j];          }
                                                                                                                
  Void    setVpsCpInvOff               ( Int i, Int j, Int val )                { m_aaaiCodedOffset[i][1][j] = val;          }
  Int     getVpsCpInvOff               ( Int i, Int j )                const    { return m_aaaiCodedOffset[i][1][j];         }

// Derived
  Void    deriveCpPresentFlag          ( );                  
  Void    setCpPresentFlag             ( Int i, Int m, Bool flag )              { m_cpPresentFlag[i][m] = flag;             } 
  Bool    getCpPresentFlag             ( Int i, Int m )                const    { return m_cpPresentFlag[i][m];             }   
                                                                       
  const IntAry1d& getCodedScale        ( Int viewIndex )               const    { return m_aaaiCodedScale [viewIndex][0];   }
  const IntAry1d& getCodedOffset       ( Int viewIndex )               const    { return m_aaaiCodedOffset[viewIndex][0];   }
  const IntAry1d& getInvCodedScale     ( Int viewIndex )               const    { return m_aaaiCodedScale [viewIndex][1];   }
  const IntAry1d& getInvCodedOffset    ( Int viewIndex )               const    { return m_aaaiCodedOffset[viewIndex][1];   }
#endif

  template <typename T, typename S, typename U> Void xPrintArray( const Char* name, Int numElemDim1, U idx, S numElemDim2, T vec, Bool printNumber, Bool printIdx = true ) const
  {
    std::cout << std::endl; 
    for (Int j = 0; j < numElemDim1; j++ )
    { 
      std::cout << std::right << std::setw(27) << name; 
      if (printIdx)
      {
        std::cout << "[" << std::right << std::setw(3) << idx[ j ] << "]" ; 
      }
      else
      {
        std::cout << std::right << std::setw(5) << " "; 
      }

      if ( printNumber )
      {
        std::cout << " (" << std::right << std::setw(3) << numElemDim2[j] << ")";        
      }          
      else
      {
        std::cout << std::right << std::setw(6) << " ";              
      }

      std::cout << ":";
      for (Int i = 0; i < numElemDim2[j]; i++)
      {
        std::cout << std::right << std::setw(3) << vec[j][i];
      }   
      std::cout << std::endl; 
    }
  }

  template <typename T> Void xPrintArray( const char* name, Int numElem, T vec, Bool printNumber  )  const
  {
    std::vector<Int> numElemDim2(1, numElem);    
    std::vector<T>   vec2       (1,  vec    );
    std::vector<Int> idx2       (0); 
    xPrintArray( name, 1, idx2, numElemDim2, vec2, printNumber, false );
  }

#endif
};

#if NH_3D_DLT
class TComDLT
{
private:
  Bool              m_bDltPresentFlag;
  Bool              m_bUseDLTFlag                 [ MAX_NUM_LAYERS ];
  Bool              m_bInterViewDltPredEnableFlag [ MAX_NUM_LAYERS ];
  Bool              m_bDltBitMapRepFlag           [ MAX_NUM_LAYERS ];

  Int               m_iNumDepthmapValues          [ MAX_NUM_LAYERS ];
  std::vector<Int>  m_iDepthValue2Idx             [ MAX_NUM_LAYERS ];
  std::vector<Int>  m_iIdx2DepthValue             [ MAX_NUM_LAYERS ];

  Int               m_iNumDepthViews;
  UInt              m_uiDepthViewBitDepth;
  
  // mapping
  Int               m_iDepthIdxToLayerId          [ MAX_NUM_LAYERS ];

public:
  TComDLT();
  ~TComDLT();
  
  Int     getDepthIdxToLayerId(Int depthIdx) const        { return m_iDepthIdxToLayerId[depthIdx]; }
  Void    setDepthIdxToLayerId(Int depthIdx, Int layerId) { m_iDepthIdxToLayerId[depthIdx] = layerId; }

  Bool    getDltPresentFlag  ()                           const   { return m_bDltPresentFlag; }
  Void    setDltPresentFlag  ( Bool b )                   { m_bDltPresentFlag = b;    }

  Bool    getUseDLTFlag      ( Int layerIdInVps )         const   { return m_bUseDLTFlag[ layerIdInVps ]; }
  Void    setUseDLTFlag      ( Int layerIdInVps, Bool b ) { m_bUseDLTFlag[ layerIdInVps ]  = b;   }
  
  Bool    getInterViewDltPredEnableFlag( Int layerIdInVps )         const   { return m_bInterViewDltPredEnableFlag[ layerIdInVps ]; }
  Void    setInterViewDltPredEnableFlag( Int layerIdInVps, Bool b ) { m_bInterViewDltPredEnableFlag[ layerIdInVps ] = b;    }
  
  Bool    getUseBitmapRep      ( Int layerIdInVps )         const   { return m_bDltBitMapRepFlag[ layerIdInVps ]; }
  Void    setUseBitmapRep      ( Int layerIdInVps, Bool b ) { m_bDltBitMapRepFlag[ layerIdInVps ]  = b;   }

  Void    setNumDepthViews   ( Int n )                    { m_iNumDepthViews = n; }
  Int     getNumDepthViews   ()                           const   { return m_iNumDepthViews; }

  Void    setDepthViewBitDepth( UInt n )                  { m_uiDepthViewBitDepth = n; }
  UInt    getDepthViewBitDepth()                          const   { return m_uiDepthViewBitDepth; }

  Int     getNumDepthValues( Int layerIdInVps )           const   { return getUseDLTFlag(layerIdInVps)?m_iNumDepthmapValues[layerIdInVps]:(1 << m_uiDepthViewBitDepth); }
  Int     depthValue2idx( Int layerIdInVps, Pel value )   const   { return getUseDLTFlag(layerIdInVps)?m_iDepthValue2Idx[layerIdInVps][value]:value; }
  Pel     idx2DepthValue( Int layerIdInVps, UInt uiIdx )  const   { return getUseDLTFlag(layerIdInVps)?m_iIdx2DepthValue[layerIdInVps][ClipBD(uiIdx,m_uiDepthViewBitDepth)]:uiIdx; }
  Void    setDepthLUTs( Int layerIdInVps, std::vector<Int> idx2DepthValue, Int iNumDepthValues = 0 );
  std::vector<Int>    idx2DepthValue( Int layerIdInVps )  const   { return m_iIdx2DepthValue[layerIdInVps]; }
  Void    getDeltaDLT( Int layerIdInVps, std::vector<Int> piDLTInRef, UInt uiDLTInRefNum, std::vector<Int>& riDeltaDLTOut, UInt& ruiDeltaDLTOutNum ) const;
  Void    setDeltaDLT( Int layerIdInVps, std::vector<Int> piDLTInRef, UInt uiDLTInRefNum, std::vector<Int> piDeltaDLTIn, UInt uiDeltaDLTInNum );
};
#endif


class Window
{
private:
  Bool m_enabledFlag;
  Int  m_winLeftOffset;
  Int  m_winRightOffset;
  Int  m_winTopOffset;
  Int  m_winBottomOffset;
#if NH_MV
  Bool          m_scaledFlag; 
#endif
public:
  Window()
  : m_enabledFlag    (false)
  , m_winLeftOffset  (0)
  , m_winRightOffset (0)
  , m_winTopOffset   (0)
  , m_winBottomOffset(0)
#if NH_MV
  , m_scaledFlag(true)
#endif
  { }

  Bool getWindowEnabledFlag() const   { return m_enabledFlag;                          }
  Int  getWindowLeftOffset() const    { return m_enabledFlag ? m_winLeftOffset : 0;    }
  Void setWindowLeftOffset(Int val)   { m_winLeftOffset = val; m_enabledFlag = true;   }
  Int  getWindowRightOffset() const   { return m_enabledFlag ? m_winRightOffset : 0;   }
  Void setWindowRightOffset(Int val)  { m_winRightOffset = val; m_enabledFlag = true;  }
  Int  getWindowTopOffset() const     { return m_enabledFlag ? m_winTopOffset : 0;     }
  Void setWindowTopOffset(Int val)    { m_winTopOffset = val; m_enabledFlag = true;    }
  Int  getWindowBottomOffset() const  { return m_enabledFlag ? m_winBottomOffset: 0;   }
  Void setWindowBottomOffset(Int val) { m_winBottomOffset = val; m_enabledFlag = true; }

#if NH_MV
  Void          setScaledFlag(Bool flag)          { m_scaledFlag = flag;  } 
  Bool          getScaledFlag() const             { return m_scaledFlag;  } 
  Void          scaleOffsets( Int scal );
#endif
  Void setWindow(Int offsetLeft, Int offsetLRight, Int offsetLTop, Int offsetLBottom)
  {
    m_enabledFlag     = true;
    m_winLeftOffset   = offsetLeft;
    m_winRightOffset  = offsetLRight;
    m_winTopOffset    = offsetLTop;
    m_winBottomOffset = offsetLBottom;
  }
};


class TComVUI
{
private:
  Bool       m_aspectRatioInfoPresentFlag;
  Int        m_aspectRatioIdc;
  Int        m_sarWidth;
  Int        m_sarHeight;
  Bool       m_overscanInfoPresentFlag;
  Bool       m_overscanAppropriateFlag;
  Bool       m_videoSignalTypePresentFlag;
  Int        m_videoFormat;
  Bool       m_videoFullRangeFlag;
  Bool       m_colourDescriptionPresentFlag;
  Int        m_colourPrimaries;
  Int        m_transferCharacteristics;
  Int        m_matrixCoefficients;
  Bool       m_chromaLocInfoPresentFlag;
  Int        m_chromaSampleLocTypeTopField;
  Int        m_chromaSampleLocTypeBottomField;
  Bool       m_neutralChromaIndicationFlag;
  Bool       m_fieldSeqFlag;
  Window     m_defaultDisplayWindow;
  Bool       m_frameFieldInfoPresentFlag;
  Bool       m_hrdParametersPresentFlag;
  Bool       m_bitstreamRestrictionFlag;
  Bool       m_tilesFixedStructureFlag;
  Bool       m_motionVectorsOverPicBoundariesFlag;
  Bool       m_restrictedRefPicListsFlag;
  Int        m_minSpatialSegmentationIdc;
  Int        m_maxBytesPerPicDenom;
  Int        m_maxBitsPerMinCuDenom;
  Int        m_log2MaxMvLengthHorizontal;
  Int        m_log2MaxMvLengthVertical;
  TComHRD    m_hrdParameters;
  TimingInfo m_timingInfo;

public:
  TComVUI()
    : m_aspectRatioInfoPresentFlag        (false) //TODO: This initialiser list contains magic numbers
    , m_aspectRatioIdc                    (0)
    , m_sarWidth                          (0)
    , m_sarHeight                         (0)
    , m_overscanInfoPresentFlag           (false)
    , m_overscanAppropriateFlag           (false)
    , m_videoSignalTypePresentFlag        (false)
    , m_videoFormat                       (5)
    , m_videoFullRangeFlag                (false)
    , m_colourDescriptionPresentFlag      (false)
    , m_colourPrimaries                   (2)
    , m_transferCharacteristics           (2)
    , m_matrixCoefficients                (2)
    , m_chromaLocInfoPresentFlag          (false)
    , m_chromaSampleLocTypeTopField       (0)
    , m_chromaSampleLocTypeBottomField    (0)
    , m_neutralChromaIndicationFlag       (false)
    , m_fieldSeqFlag                      (false)
    , m_frameFieldInfoPresentFlag         (false)
    , m_hrdParametersPresentFlag          (false)
    , m_bitstreamRestrictionFlag          (false)
    , m_tilesFixedStructureFlag           (false)
    , m_motionVectorsOverPicBoundariesFlag(true)
    , m_restrictedRefPicListsFlag         (1)
    , m_minSpatialSegmentationIdc         (0)
    , m_maxBytesPerPicDenom               (2)
    , m_maxBitsPerMinCuDenom              (1)
    , m_log2MaxMvLengthHorizontal         (15)
    , m_log2MaxMvLengthVertical           (15)
  {}

  virtual           ~TComVUI() {}

  Bool              getAspectRatioInfoPresentFlag() const                  { return m_aspectRatioInfoPresentFlag;           }
  Void              setAspectRatioInfoPresentFlag(Bool i)                  { m_aspectRatioInfoPresentFlag = i;              }

  Int               getAspectRatioIdc() const                              { return m_aspectRatioIdc;                       }
  Void              setAspectRatioIdc(Int i)                               { m_aspectRatioIdc = i;                          }

  Int               getSarWidth() const                                    { return m_sarWidth;                             }
  Void              setSarWidth(Int i)                                     { m_sarWidth = i;                                }

  Int               getSarHeight() const                                   { return m_sarHeight;                            }
  Void              setSarHeight(Int i)                                    { m_sarHeight = i;                               }

  Bool              getOverscanInfoPresentFlag() const                     { return m_overscanInfoPresentFlag;              }
  Void              setOverscanInfoPresentFlag(Bool i)                     { m_overscanInfoPresentFlag = i;                 }

  Bool              getOverscanAppropriateFlag() const                     { return m_overscanAppropriateFlag;              }
  Void              setOverscanAppropriateFlag(Bool i)                     { m_overscanAppropriateFlag = i;                 }

  Bool              getVideoSignalTypePresentFlag() const                  { return m_videoSignalTypePresentFlag;           }
  Void              setVideoSignalTypePresentFlag(Bool i)                  { m_videoSignalTypePresentFlag = i;              }

  Int               getVideoFormat() const                                 { return m_videoFormat;                          }
  Void              setVideoFormat(Int i)                                  { m_videoFormat = i;                             }

  Bool              getVideoFullRangeFlag() const                          { return m_videoFullRangeFlag;                   }
  Void              setVideoFullRangeFlag(Bool i)                          { m_videoFullRangeFlag = i;                      }

  Bool              getColourDescriptionPresentFlag() const                { return m_colourDescriptionPresentFlag;         }
  Void              setColourDescriptionPresentFlag(Bool i)                { m_colourDescriptionPresentFlag = i;            }

  Int               getColourPrimaries() const                             { return m_colourPrimaries;                      }
  Void              setColourPrimaries(Int i)                              { m_colourPrimaries = i;                         }

  Int               getTransferCharacteristics() const                     { return m_transferCharacteristics;              }
  Void              setTransferCharacteristics(Int i)                      { m_transferCharacteristics = i;                 }

  Int               getMatrixCoefficients() const                          { return m_matrixCoefficients;                   }
  Void              setMatrixCoefficients(Int i)                           { m_matrixCoefficients = i;                      }

  Bool              getChromaLocInfoPresentFlag() const                    { return m_chromaLocInfoPresentFlag;             }
  Void              setChromaLocInfoPresentFlag(Bool i)                    { m_chromaLocInfoPresentFlag = i;                }

  Int               getChromaSampleLocTypeTopField() const                 { return m_chromaSampleLocTypeTopField;          }
  Void              setChromaSampleLocTypeTopField(Int i)                  { m_chromaSampleLocTypeTopField = i;             }

  Int               getChromaSampleLocTypeBottomField() const              { return m_chromaSampleLocTypeBottomField;       }
  Void              setChromaSampleLocTypeBottomField(Int i)               { m_chromaSampleLocTypeBottomField = i;          }

  Bool              getNeutralChromaIndicationFlag() const                 { return m_neutralChromaIndicationFlag;          }
  Void              setNeutralChromaIndicationFlag(Bool i)                 { m_neutralChromaIndicationFlag = i;             }

  Bool              getFieldSeqFlag() const                                { return m_fieldSeqFlag;                         }
  Void              setFieldSeqFlag(Bool i)                                { m_fieldSeqFlag = i;                            }

  Bool              getFrameFieldInfoPresentFlag() const                   { return m_frameFieldInfoPresentFlag;            }
  Void              setFrameFieldInfoPresentFlag(Bool i)                   { m_frameFieldInfoPresentFlag = i;               }

  Window&           getDefaultDisplayWindow()                              { return m_defaultDisplayWindow;                 }
  const Window&     getDefaultDisplayWindow() const                        { return m_defaultDisplayWindow;                 }
  Void              setDefaultDisplayWindow(Window& defaultDisplayWindow ) { m_defaultDisplayWindow = defaultDisplayWindow; }

  Bool              getHrdParametersPresentFlag() const                    { return m_hrdParametersPresentFlag;             }
  Void              setHrdParametersPresentFlag(Bool i)                    { m_hrdParametersPresentFlag = i;                }

  Bool              getBitstreamRestrictionFlag() const                    { return m_bitstreamRestrictionFlag;             }
  Void              setBitstreamRestrictionFlag(Bool i)                    { m_bitstreamRestrictionFlag = i;                }

  Bool              getTilesFixedStructureFlag() const                     { return m_tilesFixedStructureFlag;              }
  Void              setTilesFixedStructureFlag(Bool i)                     { m_tilesFixedStructureFlag = i;                 }

  Bool              getMotionVectorsOverPicBoundariesFlag() const          { return m_motionVectorsOverPicBoundariesFlag;   }
  Void              setMotionVectorsOverPicBoundariesFlag(Bool i)          { m_motionVectorsOverPicBoundariesFlag = i;      }

  Bool              getRestrictedRefPicListsFlag() const                   { return m_restrictedRefPicListsFlag;            }
  Void              setRestrictedRefPicListsFlag(Bool b)                   { m_restrictedRefPicListsFlag = b;               }

  Int               getMinSpatialSegmentationIdc() const                   { return m_minSpatialSegmentationIdc;            }
  Void              setMinSpatialSegmentationIdc(Int i)                    { m_minSpatialSegmentationIdc = i;               }

  Int               getMaxBytesPerPicDenom() const                         { return m_maxBytesPerPicDenom;                  }
  Void              setMaxBytesPerPicDenom(Int i)                          { m_maxBytesPerPicDenom = i;                     }

  Int               getMaxBitsPerMinCuDenom() const                        { return m_maxBitsPerMinCuDenom;                 }
  Void              setMaxBitsPerMinCuDenom(Int i)                         { m_maxBitsPerMinCuDenom = i;                    }

  Int               getLog2MaxMvLengthHorizontal() const                   { return m_log2MaxMvLengthHorizontal;            }
  Void              setLog2MaxMvLengthHorizontal(Int i)                    { m_log2MaxMvLengthHorizontal = i;               }

  Int               getLog2MaxMvLengthVertical() const                     { return m_log2MaxMvLengthVertical;              }
  Void              setLog2MaxMvLengthVertical(Int i)                      { m_log2MaxMvLengthVertical = i;                 }

  TComHRD*          getHrdParameters()                                     { return &m_hrdParameters;                       }
  const TComHRD*    getHrdParameters()  const                              { return &m_hrdParameters;                       }

  TimingInfo*       getTimingInfo()                                        { return &m_timingInfo;                          }
  const TimingInfo* getTimingInfo() const                                  { return &m_timingInfo;                          }
#if NH_MV
  Void              inferVideoSignalInfo( const TComVPS* vps, Int layerIdCurr );
#endif
};

/// SPS RExt class
class TComSPSRExt // Names aligned to text specification
{
private:
  Bool             m_transformSkipRotationEnabledFlag;
  Bool             m_transformSkipContextEnabledFlag;
  Bool             m_rdpcmEnabledFlag[NUMBER_OF_RDPCM_SIGNALLING_MODES];
  Bool             m_extendedPrecisionProcessingFlag;
  Bool             m_intraSmoothingDisabledFlag;
  Bool             m_highPrecisionOffsetsEnabledFlag;
  Bool             m_persistentRiceAdaptationEnabledFlag;
  Bool             m_cabacBypassAlignmentEnabledFlag;

public:
  TComSPSRExt();

  Bool settingsDifferFromDefaults() const
  {
    return getTransformSkipRotationEnabledFlag()
        || getTransformSkipContextEnabledFlag()
        || getRdpcmEnabledFlag(RDPCM_SIGNAL_IMPLICIT)
        || getRdpcmEnabledFlag(RDPCM_SIGNAL_EXPLICIT)
        || getExtendedPrecisionProcessingFlag()
        || getIntraSmoothingDisabledFlag()
        || getHighPrecisionOffsetsEnabledFlag()
        || getPersistentRiceAdaptationEnabledFlag()
        || getCabacBypassAlignmentEnabledFlag();
  }


  Bool getTransformSkipRotationEnabledFlag() const                                     { return m_transformSkipRotationEnabledFlag;     }
  Void setTransformSkipRotationEnabledFlag(const Bool value)                           { m_transformSkipRotationEnabledFlag = value;    }

  Bool getTransformSkipContextEnabledFlag() const                                      { return m_transformSkipContextEnabledFlag;      }
  Void setTransformSkipContextEnabledFlag(const Bool value)                            { m_transformSkipContextEnabledFlag = value;     }

  Bool getRdpcmEnabledFlag(const RDPCMSignallingMode signallingMode) const             { return m_rdpcmEnabledFlag[signallingMode];     }
  Void setRdpcmEnabledFlag(const RDPCMSignallingMode signallingMode, const Bool value) { m_rdpcmEnabledFlag[signallingMode] = value;    }

  Bool getExtendedPrecisionProcessingFlag() const                                      { return m_extendedPrecisionProcessingFlag;      }
  Void setExtendedPrecisionProcessingFlag(Bool value)                                  { m_extendedPrecisionProcessingFlag = value;     }

  Bool getIntraSmoothingDisabledFlag() const                                           { return m_intraSmoothingDisabledFlag;           }
  Void setIntraSmoothingDisabledFlag(Bool bValue)                                      { m_intraSmoothingDisabledFlag=bValue;           }

  Bool getHighPrecisionOffsetsEnabledFlag() const                                      { return m_highPrecisionOffsetsEnabledFlag;      }
  Void setHighPrecisionOffsetsEnabledFlag(Bool value)                                  { m_highPrecisionOffsetsEnabledFlag = value;     }

  Bool getPersistentRiceAdaptationEnabledFlag() const                                  { return m_persistentRiceAdaptationEnabledFlag;  }
  Void setPersistentRiceAdaptationEnabledFlag(const Bool value)                        { m_persistentRiceAdaptationEnabledFlag = value; }

  Bool getCabacBypassAlignmentEnabledFlag() const                                      { return m_cabacBypassAlignmentEnabledFlag;      }
  Void setCabacBypassAlignmentEnabledFlag(const Bool value)                            { m_cabacBypassAlignmentEnabledFlag = value;     }
};


#if NH_3D
class TComSps3dExtension
{
public:
  TComSps3dExtension()
  {
    for (Int d = 0; d < 2; d++)
    {
      m_ivMvPredFlag          [d] = false; 
      m_ivMvScalingFlag       [d] = false; 
      m_log2SubPbSizeMinus3   [d] = 3; 
      m_ivResPredFlag         [d] = false; 
      m_depthRefinementFlag   [d] = false; 
      m_viewSynthesisPredFlag [d] = false; 
      m_depthBasedBlkPartFlag [d] = false; 
      m_mpiFlag               [d] = false; 
      m_log2MpiSubPbSizeMinus3[d] = 3; 
      m_intraContourFlag      [d] = false; 
      m_intraSdcWedgeFlag     [d] = false; 
      m_qtPredFlag            [d] = false; 
      m_interSdcFlag          [d] = false; 
      m_depthIntraSkipFlag    [d] = false;   
    }
  }

  Void          setIvMvPredFlag( Int d, Bool flag )         { m_ivMvPredFlag[d] = flag;             }
  Bool          getIvMvPredFlag( Int d ) const              { return m_ivMvPredFlag[d];             }

  Void          setIvMvScalingFlag( Int d, Bool flag )      { m_ivMvScalingFlag[d] = flag;          }
  Bool          getIvMvScalingFlag( Int d ) const           { return m_ivMvScalingFlag[d];          }

  Void          setLog2SubPbSizeMinus3( Int d, Int  val )   { m_log2SubPbSizeMinus3[d] = val;       }
  Int           getLog2SubPbSizeMinus3( Int d ) const       { return m_log2SubPbSizeMinus3[d];      }

  Void          setIvResPredFlag( Int d, Bool flag )        { m_ivResPredFlag[d] = flag;            }
  Bool          getIvResPredFlag( Int d ) const             { return m_ivResPredFlag[d];            }

  Void          setDepthRefinementFlag( Int d, Bool flag )  { m_depthRefinementFlag[d] = flag;      }
  Bool          getDepthRefinementFlag( Int d ) const       { return m_depthRefinementFlag[d];      }

  Void          setViewSynthesisPredFlag( Int d, Bool flag ) { m_viewSynthesisPredFlag[d] = flag;   }
  Bool          getViewSynthesisPredFlag( Int d ) const     { return m_viewSynthesisPredFlag[d];    }

  Void          setDepthBasedBlkPartFlag( Int d, Bool flag ) { m_depthBasedBlkPartFlag[d] = flag;   }
  Bool          getDepthBasedBlkPartFlag( Int d ) const     { return m_depthBasedBlkPartFlag[d];    }

  Void          setMpiFlag( Int d, Bool flag )              { m_mpiFlag[d] = flag;                  }
  Bool          getMpiFlag( Int d ) const                   { return m_mpiFlag[d];                  }

  Void          setLog2MpiSubPbSizeMinus3( Int d, Int  val ) { m_log2MpiSubPbSizeMinus3[d] = val;   }
  Int           getLog2MpiSubPbSizeMinus3( Int d ) const    { return m_log2MpiSubPbSizeMinus3[d];   }

  Void          setIntraContourFlag( Int d, Bool flag )     { m_intraContourFlag[d] = flag;         }
  Bool          getIntraContourFlag( Int d ) const          { return m_intraContourFlag[d];         }

  Void          setIntraSdcWedgeFlag( Int d, Bool flag )    { m_intraSdcWedgeFlag[d] = flag;        }
  Bool          getIntraSdcWedgeFlag( Int d ) const         { return m_intraSdcWedgeFlag[d];        }

  Void          setQtPredFlag( Int d, Bool flag )           { m_qtPredFlag[d] = flag;               }
  Bool          getQtPredFlag( Int d ) const                { return m_qtPredFlag[d];               }

  Void          setInterSdcFlag( Int d, Bool flag )         { m_interSdcFlag[d] = flag;             }
  Bool          getInterSdcFlag( Int d ) const              { return m_interSdcFlag[d];             }

  Void          setDepthIntraSkipFlag( Int d, Bool flag )   { m_depthIntraSkipFlag[d] = flag;       }
  Bool          getDepthIntraSkipFlag( Int d ) const        { return m_depthIntraSkipFlag[d];       }
private:

  Bool        m_ivMvPredFlag          [2];
  Bool        m_ivMvScalingFlag       [2];
  Int         m_log2SubPbSizeMinus3   [2];
  Bool        m_ivResPredFlag         [2];
  Bool        m_depthRefinementFlag   [2];
  Bool        m_viewSynthesisPredFlag [2];
  Bool        m_depthBasedBlkPartFlag [2];
  Bool        m_mpiFlag               [2];
  Int         m_log2MpiSubPbSizeMinus3[2];
  Bool        m_intraContourFlag      [2];
  Bool        m_intraSdcWedgeFlag     [2];
  Bool        m_qtPredFlag            [2];
  Bool        m_interSdcFlag          [2];
  Bool        m_depthIntraSkipFlag    [2];  
};

#endif


/// SPS class
class TComSPS
{
private:
  Int              m_SPSId;
  Int              m_VPSId;
  ChromaFormat     m_chromaFormatIdc;

  UInt             m_uiMaxTLayers;           // maximum number of temporal layers

  // Structure
  UInt             m_picWidthInLumaSamples;
  UInt             m_picHeightInLumaSamples;

  Int              m_log2MinCodingBlockSize;
  Int              m_log2DiffMaxMinCodingBlockSize;
  UInt             m_uiMaxCUWidth;
  UInt             m_uiMaxCUHeight;
  UInt             m_uiMaxTotalCUDepth; ///< Total CU depth, relative to the smallest possible transform block size.

  Window           m_conformanceWindow;

  TComRPSList      m_RPSList;
  Bool             m_bLongTermRefsPresent;
  Bool             m_TMVPFlagsPresent;
  Int              m_numReorderPics[MAX_TLAYER];

  // Tool list
  UInt             m_uiQuadtreeTULog2MaxSize;
  UInt             m_uiQuadtreeTULog2MinSize;
  UInt             m_uiQuadtreeTUMaxDepthInter;
  UInt             m_uiQuadtreeTUMaxDepthIntra;
  Bool             m_usePCM;
  UInt             m_pcmLog2MaxSize;
  UInt             m_uiPCMLog2MinSize;
  Bool             m_useAMP;

  // Parameter
  BitDepths        m_bitDepths;
  Int              m_qpBDOffset[MAX_NUM_CHANNEL_TYPE];
  Int              m_pcmBitDepths[MAX_NUM_CHANNEL_TYPE];
  Bool             m_bPCMFilterDisableFlag;

  UInt             m_uiBitsForPOC;
  UInt             m_numLongTermRefPicSPS;
  UInt             m_ltRefPicPocLsbSps[MAX_NUM_LONG_TERM_REF_PICS];
  Bool             m_usedByCurrPicLtSPSFlag[MAX_NUM_LONG_TERM_REF_PICS];
  // Max physical transform size
  UInt             m_uiMaxTrSize;

  Bool             m_bUseSAO;

  Bool             m_bTemporalIdNestingFlag; // temporal_id_nesting_flag

  Bool             m_scalingListEnabledFlag;
  Bool             m_scalingListPresentFlag;
  TComScalingList  m_scalingList;
  UInt             m_uiMaxDecPicBuffering[MAX_TLAYER];
  UInt             m_uiMaxLatencyIncrease[MAX_TLAYER];  // Really max latency increase plus 1 (value 0 expresses no limit)

  Bool             m_useStrongIntraSmoothing;

  Bool             m_vuiParametersPresentFlag;
  TComVUI          m_vuiParameters;

  TComSPSRExt      m_spsRangeExtension;

  static const Int m_winUnitX[NUM_CHROMA_FORMAT];
  static const Int m_winUnitY[NUM_CHROMA_FORMAT];
  TComPTL          m_pcPTL;

#if O0043_BEST_EFFORT_DECODING
  UInt             m_forceDecodeBitDepth; // 0 = do not force the decoder's bit depth, other = force the decoder's bit depth to this value (best effort decoding)
#endif
#if NH_MV
  TComVPS*         m_pcVPS; 
  // SPS           
  Int              m_spsMaxSubLayersMinus1;
  Int              m_spsExtOrMaxSubLayersMinus1;
  Bool             m_spsExtensionPresentFlag; 
                   
  Bool             m_spsRangeExtensionsFlag;
  Bool             m_spsMultilayerExtensionFlag;

  Bool             m_sps3dExtensionFlag;
  Int              m_spsExtension5bits;

  Bool             m_spsInferScalingListFlag;
  Int              m_spsScalingListRefLayerId;
  Bool             m_updateRepFormatFlag;
  Int              m_spsRepFormatIdx;
  // SPS Extension 
  Bool             m_interViewMvVertConstraintFlag;
#endif
#if NH_3D
  TComSps3dExtension m_sps3dExtension; 
  Int              m_aaiCodedScale [2][MAX_NUM_LAYERS];
  Int              m_aaiCodedOffset[2][MAX_NUM_LAYERS];
#endif             
#if NH_MV           
  Int              m_layerId; 
#endif

public:
                         TComSPS();
  virtual                ~TComSPS();
#if O0043_BEST_EFFORT_DECODING
  Void                   setForceDecodeBitDepth(UInt bitDepth)                                           { m_forceDecodeBitDepth = bitDepth;                                    }
  UInt                   getForceDecodeBitDepth()        const                                           { return m_forceDecodeBitDepth;                                        }
#endif

  Int                    getVPSId() const                                                                { return m_VPSId;                                                      }
  Void                   setVPSId(Int i)                                                                 { m_VPSId = i;                                                         }
  Int                    getSPSId() const                                                                { return m_SPSId;                                                      }
  Void                   setSPSId(Int i)                                                                 { m_SPSId = i;                                                         }
  ChromaFormat           getChromaFormatIdc () const                                                     { return m_chromaFormatIdc;                                            }
  Void                   setChromaFormatIdc (ChromaFormat i)                                             { m_chromaFormatIdc = i;                                               }

  static Int             getWinUnitX (Int chromaFormatIdc)                                               { assert (chromaFormatIdc >= 0 && chromaFormatIdc < NUM_CHROMA_FORMAT); return m_winUnitX[chromaFormatIdc]; }
  static Int             getWinUnitY (Int chromaFormatIdc)                                               { assert (chromaFormatIdc >= 0 && chromaFormatIdc < NUM_CHROMA_FORMAT); return m_winUnitY[chromaFormatIdc]; }

  // structure
  Void                   setPicWidthInLumaSamples( UInt u )                                              { m_picWidthInLumaSamples = u;                                         }
  UInt                   getPicWidthInLumaSamples() const                                                { return  m_picWidthInLumaSamples;                                     }
  Void                   setPicHeightInLumaSamples( UInt u )                                             { m_picHeightInLumaSamples = u;                                        }
  UInt                   getPicHeightInLumaSamples() const                                               { return  m_picHeightInLumaSamples;                                    }

  Window&                getConformanceWindow()                                                          { return  m_conformanceWindow;                                         }
  const Window&          getConformanceWindow() const                                                    { return  m_conformanceWindow;                                         }
  Void                   setConformanceWindow(Window& conformanceWindow )                                { m_conformanceWindow = conformanceWindow;                             }

  UInt                   getNumLongTermRefPicSPS() const                                                 { return m_numLongTermRefPicSPS;                                       }
  Void                   setNumLongTermRefPicSPS(UInt val)                                               { m_numLongTermRefPicSPS = val;                                        }

  UInt                   getLtRefPicPocLsbSps(UInt index) const                                          { assert( index < MAX_NUM_LONG_TERM_REF_PICS ); return m_ltRefPicPocLsbSps[index]; }
  Void                   setLtRefPicPocLsbSps(UInt index, UInt val)                                      { assert( index < MAX_NUM_LONG_TERM_REF_PICS ); m_ltRefPicPocLsbSps[index] = val;  }

  Bool                   getUsedByCurrPicLtSPSFlag(Int i) const                                          { assert( i < MAX_NUM_LONG_TERM_REF_PICS ); return m_usedByCurrPicLtSPSFlag[i];    }
  Void                   setUsedByCurrPicLtSPSFlag(Int i, Bool x)                                        { assert( i < MAX_NUM_LONG_TERM_REF_PICS ); m_usedByCurrPicLtSPSFlag[i] = x;       }

  Int                    getLog2MinCodingBlockSize() const                                               { return m_log2MinCodingBlockSize;                                     }
  Void                   setLog2MinCodingBlockSize(Int val)                                              { m_log2MinCodingBlockSize = val;                                      }
  Int                    getLog2DiffMaxMinCodingBlockSize() const                                        { return m_log2DiffMaxMinCodingBlockSize;                              }
  Void                   setLog2DiffMaxMinCodingBlockSize(Int val)                                       { m_log2DiffMaxMinCodingBlockSize = val;                               }

  Void                   setMaxCUWidth( UInt u )                                                         { m_uiMaxCUWidth = u;                                                  }
  UInt                   getMaxCUWidth() const                                                           { return  m_uiMaxCUWidth;                                              }
  Void                   setMaxCUHeight( UInt u )                                                        { m_uiMaxCUHeight = u;                                                 }
  UInt                   getMaxCUHeight() const                                                          { return  m_uiMaxCUHeight;                                             }
  Void                   setMaxTotalCUDepth( UInt u )                                                    { m_uiMaxTotalCUDepth = u;                                             }
  UInt                   getMaxTotalCUDepth() const                                                      { return  m_uiMaxTotalCUDepth;                                         }
  Void                   setUsePCM( Bool b )                                                             { m_usePCM = b;                                                        }
  Bool                   getUsePCM() const                                                               { return m_usePCM;                                                     }
  Void                   setPCMLog2MaxSize( UInt u )                                                     { m_pcmLog2MaxSize = u;                                                }
  UInt                   getPCMLog2MaxSize() const                                                       { return  m_pcmLog2MaxSize;                                            }
  Void                   setPCMLog2MinSize( UInt u )                                                     { m_uiPCMLog2MinSize = u;                                              }
  UInt                   getPCMLog2MinSize() const                                                       { return  m_uiPCMLog2MinSize;                                          }
  Void                   setBitsForPOC( UInt u )                                                         { m_uiBitsForPOC = u;                                                  }
  UInt                   getBitsForPOC() const                                                           { return m_uiBitsForPOC;                                               }
  Bool                   getUseAMP() const                                                               { return m_useAMP;                                                     }
  Void                   setUseAMP( Bool b )                                                             { m_useAMP = b;                                                        }
  Void                   setQuadtreeTULog2MaxSize( UInt u )                                              { m_uiQuadtreeTULog2MaxSize = u;                                       }
  UInt                   getQuadtreeTULog2MaxSize() const                                                { return m_uiQuadtreeTULog2MaxSize;                                    }
  Void                   setQuadtreeTULog2MinSize( UInt u )                                              { m_uiQuadtreeTULog2MinSize = u;                                       }
  UInt                   getQuadtreeTULog2MinSize() const                                                { return m_uiQuadtreeTULog2MinSize;                                    }
  Void                   setQuadtreeTUMaxDepthInter( UInt u )                                            { m_uiQuadtreeTUMaxDepthInter = u;                                     }
  Void                   setQuadtreeTUMaxDepthIntra( UInt u )                                            { m_uiQuadtreeTUMaxDepthIntra = u;                                     }
  UInt                   getQuadtreeTUMaxDepthInter() const                                              { return m_uiQuadtreeTUMaxDepthInter;                                  }
  UInt                   getQuadtreeTUMaxDepthIntra() const                                              { return m_uiQuadtreeTUMaxDepthIntra;                                  }
  Void                   setNumReorderPics(Int i, UInt tlayer)                                           { m_numReorderPics[tlayer] = i;                                        }
  Int                    getNumReorderPics(UInt tlayer) const                                            { return m_numReorderPics[tlayer];                                     }
  Void                   createRPSList( Int numRPS );
  const TComRPSList*     getRPSList() const                                                              { return &m_RPSList;                                                   }
  TComRPSList*           getRPSList()                                                                    { return &m_RPSList;                                                   }
  Bool                   getLongTermRefsPresent() const                                                  { return m_bLongTermRefsPresent;                                       }
  Void                   setLongTermRefsPresent(Bool b)                                                  { m_bLongTermRefsPresent=b;                                            }
  Bool                   getTMVPFlagsPresent() const                                                     { return m_TMVPFlagsPresent;                                           }
  Void                   setTMVPFlagsPresent(Bool b)                                                     { m_TMVPFlagsPresent=b;                                                }
  // physical transform
  Void                   setMaxTrSize( UInt u )                                                          { m_uiMaxTrSize = u;                                                   }
  UInt                   getMaxTrSize() const                                                            { return  m_uiMaxTrSize;                                               }

  // Bit-depth
  Int                    getBitDepth(ChannelType type) const                                             { return m_bitDepths.recon[type];                                      }
  Void                   setBitDepth(ChannelType type, Int u )                                           { m_bitDepths.recon[type] = u;                                         }
#if O0043_BEST_EFFORT_DECODING
  Int                    getStreamBitDepth(ChannelType type) const                                       { return m_bitDepths.stream[type];                                     }
  Void                   setStreamBitDepth(ChannelType type, Int u )                                     { m_bitDepths.stream[type] = u;                                        }
#endif
  const BitDepths&       getBitDepths() const                                                            { return m_bitDepths;                                                  }
  Int                    getMaxLog2TrDynamicRange(ChannelType channelType) const                         { return getSpsRangeExtension().getExtendedPrecisionProcessingFlag() ? std::max<Int>(15, Int(m_bitDepths.recon[channelType] + 6)) : 15; }

  Int                    getDifferentialLumaChromaBitDepth() const                                       { return Int(m_bitDepths.recon[CHANNEL_TYPE_LUMA]) - Int(m_bitDepths.recon[CHANNEL_TYPE_CHROMA]); }
  Int                    getQpBDOffset(ChannelType type) const                                           { return m_qpBDOffset[type];                                           }
  Void                   setQpBDOffset(ChannelType type, Int i)                                          { m_qpBDOffset[type] = i;                                              }

  Void                   setUseSAO(Bool bVal)                                                            { m_bUseSAO = bVal;                                                    }
  Bool                   getUseSAO() const                                                               { return m_bUseSAO;                                                    }

  UInt                   getMaxTLayers() const                                                           { return m_uiMaxTLayers; }
  Void                   setMaxTLayers( UInt uiMaxTLayers )                                              { assert( uiMaxTLayers <= MAX_TLAYER ); m_uiMaxTLayers = uiMaxTLayers; }

  Bool                   getTemporalIdNestingFlag() const                                                { return m_bTemporalIdNestingFlag;                                     }
  Void                   setTemporalIdNestingFlag( Bool bValue )                                         { m_bTemporalIdNestingFlag = bValue;                                   }
  UInt                   getPCMBitDepth(ChannelType type) const                                          { return m_pcmBitDepths[type];                                         }
  Void                   setPCMBitDepth(ChannelType type, UInt u)                                        { m_pcmBitDepths[type] = u;                                            }
  Void                   setPCMFilterDisableFlag( Bool bValue )                                          { m_bPCMFilterDisableFlag = bValue;                                    }
  Bool                   getPCMFilterDisableFlag() const                                                 { return m_bPCMFilterDisableFlag;                                      }

  Bool                   getScalingListFlag() const                                                      { return m_scalingListEnabledFlag;                                     }
  Void                   setScalingListFlag( Bool b )                                                    { m_scalingListEnabledFlag  = b;                                       }
  Bool                   getScalingListPresentFlag() const                                               { return m_scalingListPresentFlag;                                     }
  Void                   setScalingListPresentFlag( Bool b )                                             { m_scalingListPresentFlag  = b;                                       }
  Void                   setScalingList( TComScalingList *scalingList);
  TComScalingList&       getScalingList()                                                                { return m_scalingList;                                                }
  const TComScalingList& getScalingList() const                                                          { return m_scalingList;                                                }
  UInt                   getMaxDecPicBuffering(UInt tlayer) const                                        { return m_uiMaxDecPicBuffering[tlayer];                               }
  Void                   setMaxDecPicBuffering( UInt ui, UInt tlayer )                                   { assert(tlayer < MAX_TLAYER); m_uiMaxDecPicBuffering[tlayer] = ui;    }
  UInt                   getMaxLatencyIncrease(UInt tlayer) const                                        { return m_uiMaxLatencyIncrease[tlayer];                               }
  Void                   setMaxLatencyIncrease( UInt ui , UInt tlayer)                                   { m_uiMaxLatencyIncrease[tlayer] = ui;                                 }

  Void                   setUseStrongIntraSmoothing(Bool bVal)                                           { m_useStrongIntraSmoothing = bVal;                                    }
  Bool                   getUseStrongIntraSmoothing() const                                              { return m_useStrongIntraSmoothing;                                    }

  Bool                   getVuiParametersPresentFlag() const                                             { return m_vuiParametersPresentFlag;                                   }
  Void                   setVuiParametersPresentFlag(Bool b)                                             { m_vuiParametersPresentFlag = b;                                      }
  TComVUI*               getVuiParameters()                                                              { return &m_vuiParameters;                                             }
  const TComVUI*         getVuiParameters() const                                                        { return &m_vuiParameters;                                             }
  const TComPTL*         getPTL() const                                                                  { return &m_pcPTL;                                                     }
  TComPTL*               getPTL()                                                                        { return &m_pcPTL;                                                     }

  const TComSPSRExt&     getSpsRangeExtension() const                                                    { return m_spsRangeExtension;                                          }
  TComSPSRExt&           getSpsRangeExtension()                                                          { return m_spsRangeExtension;                                          }

  // Sequence parameter set range extension syntax
  // WAS: getUseResidualRotation and setUseResidualRotation
  // Now getSpsRangeExtension().getTransformSkipRotationEnabledFlag and getSpsRangeExtension().setTransformSkipRotationEnabledFlag

  // WAS: getUseSingleSignificanceMapContext and setUseSingleSignificanceMapContext
  // Now: getSpsRangeExtension().getTransformSkipContextEnabledFlag and getSpsRangeExtension().setTransformSkipContextEnabledFlag

  // WAS: getUseResidualDPCM and setUseResidualDPCM
  // Now: getSpsRangeExtension().getRdpcmEnabledFlag and getSpsRangeExtension().setRdpcmEnabledFlag and

  // WAS: getUseExtendedPrecision and setUseExtendedPrecision
  // Now: getSpsRangeExtension().getExtendedPrecisionProcessingFlag and getSpsRangeExtension().setExtendedPrecisionProcessingFlag

  // WAS: getDisableIntraReferenceSmoothing and setDisableIntraReferenceSmoothing
  // Now: getSpsRangeExtension().getIntraSmoothingDisabledFlag and getSpsRangeExtension().setIntraSmoothingDisabledFlag

  // WAS: getUseHighPrecisionPredictionWeighting and setUseHighPrecisionPredictionWeighting
  // Now: getSpsRangeExtension().getHighPrecisionOffsetsEnabledFlag and getSpsRangeExtension().setHighPrecisionOffsetsEnabledFlag

  // WAS: getUseGolombRiceParameterAdaptation and setUseGolombRiceParameterAdaptation
  // Now: getSpsRangeExtension().getPersistentRiceAdaptationEnabledFlag and getSpsRangeExtension().setPersistentRiceAdaptationEnabledFlag

  // WAS: getAlignCABACBeforeBypass and setAlignCABACBeforeBypass
  // Now: getSpsRangeExtension().getCabacBypassAlignmentEnabledFlag and getSpsRangeExtension().setCabacBypassAlignmentEnabledFlag


#if NH_MV

  UInt                   getSpsMaxSubLayersMinus1() const                                                { return ( m_uiMaxTLayers - 1);                                        }
  Void                   setSpsMaxSubLayersMinus1( UInt val )                                            { setMaxTLayers( val + 1 );                                            }

  Void                   setSpsExtOrMaxSubLayersMinus1( Int  val )                                       { m_spsExtOrMaxSubLayersMinus1 = val;                                  }
  Int                    getSpsExtOrMaxSubLayersMinus1(  )         const                                 { return m_spsExtOrMaxSubLayersMinus1;                                 }
  Void                   inferSpsMaxSubLayersMinus1( Bool atPsActivation, TComVPS* vps  );                                                                                      
                                                                                                                                                                                
  Bool                   getMultiLayerExtSpsFlag()            const { return ( getLayerId() != 0  &&  getSpsExtOrMaxSubLayersMinus1() == 7 );                                   }
  Void                   inferSpsMaxDecPicBufferingMinus1( TComVPS* vps, Int targetOptLayerSetIdx, Int currLayerId, Bool encoder );                                             
                                                                                                                                                                                
  Void                   setSpsExtensionPresentFlag( Bool flag )                                         { m_spsExtensionPresentFlag = flag;                                    }
  Bool                   getSpsExtensionPresentFlag( )           const                                   { return m_spsExtensionPresentFlag;                                    }
                                                                                                                                                                                
  Void                   setSpsRangeExtensionsFlag( Bool flag )                                          { m_spsRangeExtensionsFlag = flag;                                     }
  Bool                   getSpsRangeExtensionsFlag(  )                     const                         { return m_spsRangeExtensionsFlag;                                     }
                                                                                                                                                                                
  Void                   setSpsMultilayerExtensionFlag( Bool flag )                                      { m_spsMultilayerExtensionFlag = flag;                                 }
  Bool                   getSpsMultilayerExtensionFlag( )                  const                         { return m_spsMultilayerExtensionFlag;                                 }

  Void                   setSps3dExtensionFlag( Bool flag )                                              { m_sps3dExtensionFlag = flag;                                         }
  Bool                   getSps3dExtensionFlag(  )                         const                         { return m_sps3dExtensionFlag;                                         }
                                                                                                                                                                                
  Void                   setSpsExtension5bits( Int  val )                                                { m_spsExtension5bits = val;                                           }
  Int                    getSpsExtension5bits(  )                          const                         { return m_spsExtension5bits;                                          }
                                                                                                                                                                                
  Void                   setVPS          ( TComVPS* pcVPS )                                              { m_pcVPS = pcVPS;                                                     }
  TComVPS*               getVPS          ()                 const                                        { return m_pcVPS;                                                      }
                                                                                                                                                                                
  Void                   setSpsInferScalingListFlag( Bool flag )                                         { m_spsInferScalingListFlag = flag;                                    }
  Bool                   getSpsInferScalingListFlag(  )          const                                   { return m_spsInferScalingListFlag;                                    }
                                                                                                                                                                                
  Void                   setSpsScalingListRefLayerId( Int  val )                                         { m_spsScalingListRefLayerId = val;                                    }
  Int                    getSpsScalingListRefLayerId(  )         const                                   { return m_spsScalingListRefLayerId;                                   }
                                                                                                                                                                                
  Void                   setUpdateRepFormatFlag( Bool flag )                                             { m_updateRepFormatFlag = flag;                                        }
  Bool                   getUpdateRepFormatFlag(  )              const                                   { return m_updateRepFormatFlag;                                        }
  
  Void                   setSpsRepFormatIdx( Int  val )                                                  { m_spsRepFormatIdx = val;                                             }
  Int                    getSpsRepFormatIdx(  )                  const                                   { return m_spsRepFormatIdx;                                            }
                                                                                                                                                                                
// SPS Extension                                                                                                                                                                
  Void                   setInterViewMvVertConstraintFlag(Bool val)                                      { m_interViewMvVertConstraintFlag = val;                               }
  Bool                   getInterViewMvVertConstraintFlag()         const                                { return m_interViewMvVertConstraintFlag;                              }
                                                                                                                                                                                
#if NH_3D
  Void                   setSps3dExtension ( TComSps3dExtension& sps3dExtension )                        { m_sps3dExtension = sps3dExtension;                                   }
  const TComSps3dExtension* getSps3dExtension ( )  const                                                 { return &m_sps3dExtension;                                            } 
#endif                  
                        
  // Inference          
                        
  Void                   inferRepFormat( TComVPS* vps, Int layerIdCurr, Bool encoder );
  Void                   inferScalingList( const TComSPS* spsSrc );
                        
  // others             
  Void                   checkRpsMaxNumPics( const TComVPS* vps, Int currLayerId ) const;
                        
  Int                    getLayerId            ()           const                                        { return m_layerId;                                                    }
  Void                   setLayerId            ( Int val )                                               { m_layerId = val;                                                     }

#endif

};


/// Reference Picture Lists class

class TComRefPicListModification
{
private:
  Bool m_refPicListModificationFlagL0;
  Bool m_refPicListModificationFlagL1;
  UInt m_RefPicSetIdxL0[REF_PIC_LIST_NUM_IDX];
  UInt m_RefPicSetIdxL1[REF_PIC_LIST_NUM_IDX];

public:
          TComRefPicListModification();
  virtual ~TComRefPicListModification();

  Void    create();
  Void    destroy();

  Bool    getRefPicListModificationFlagL0() const        { return m_refPicListModificationFlagL0;                                  }
  Void    setRefPicListModificationFlagL0(Bool flag)     { m_refPicListModificationFlagL0 = flag;                                  }
  Bool    getRefPicListModificationFlagL1() const        { return m_refPicListModificationFlagL1;                                  }
  Void    setRefPicListModificationFlagL1(Bool flag)     { m_refPicListModificationFlagL1 = flag;                                  }
  UInt    getRefPicSetIdxL0(UInt idx) const              { assert(idx<REF_PIC_LIST_NUM_IDX); return m_RefPicSetIdxL0[idx];         }
  Void    setRefPicSetIdxL0(UInt idx, UInt refPicSetIdx) { assert(idx<REF_PIC_LIST_NUM_IDX); m_RefPicSetIdxL0[idx] = refPicSetIdx; }
  UInt    getRefPicSetIdxL1(UInt idx) const              { assert(idx<REF_PIC_LIST_NUM_IDX); return m_RefPicSetIdxL1[idx];         }
  Void    setRefPicSetIdxL1(UInt idx, UInt refPicSetIdx) { assert(idx<REF_PIC_LIST_NUM_IDX); m_RefPicSetIdxL1[idx] = refPicSetIdx; }
#if NH_MV
  // Why not a listIdx for all members, would avoid code duplication?? 
  Void    setRefPicSetIdxL(UInt li, UInt idx, UInt refPicSetIdx) {( li==0 ? m_RefPicSetIdxL0[idx] : m_RefPicSetIdxL1[idx] ) = refPicSetIdx;              }
  UInt    getRefPicSetIdxL(UInt li, UInt idx )                   { return ( li == 0 ) ? m_RefPicSetIdxL0[idx] : m_RefPicSetIdxL1[idx] ;                  }
  Void    setRefPicListModificationFlagL(UInt li, Bool flag)     { ( li==0  ? m_refPicListModificationFlagL0 : m_refPicListModificationFlagL1 ) = flag;  }
  Bool    getRefPicListModificationFlagL(UInt li )               { return ( li== 0) ? m_refPicListModificationFlagL0 : m_refPicListModificationFlagL1;   }
#endif
};

/// PPS RExt class
class TComPPSRExt // Names aligned to text specification
{
private:
  Int              m_log2MaxTransformSkipBlockSize;
  Bool             m_crossComponentPredictionEnabledFlag;

  // Chroma QP Adjustments
  Int              m_diffCuChromaQpOffsetDepth;
  Int              m_chromaQpOffsetListLen; // size (excludes the null entry used in the following array).
  ChromaQpAdj      m_ChromaQpAdjTableIncludingNullEntry[1+MAX_QP_OFFSET_LIST_SIZE]; //!< Array includes entry [0] for the null offset used when cu_chroma_qp_offset_flag=0, and entries [cu_chroma_qp_offset_idx+1...] otherwise

  UInt             m_log2SaoOffsetScale[MAX_NUM_CHANNEL_TYPE];

public:
  TComPPSRExt();

  Bool settingsDifferFromDefaults(const bool bTransformSkipEnabledFlag) const
  {
    return (bTransformSkipEnabledFlag && (getLog2MaxTransformSkipBlockSize() !=2))
        || (getCrossComponentPredictionEnabledFlag() )
        || (getChromaQpOffsetListEnabledFlag() )
        || (getLog2SaoOffsetScale(CHANNEL_TYPE_LUMA) !=0 )
        || (getLog2SaoOffsetScale(CHANNEL_TYPE_CHROMA) !=0 );
  }

  UInt                   getLog2MaxTransformSkipBlockSize() const                         { return m_log2MaxTransformSkipBlockSize;         }
  Void                   setLog2MaxTransformSkipBlockSize( UInt u )                       { m_log2MaxTransformSkipBlockSize  = u;           }

  Bool                   getCrossComponentPredictionEnabledFlag() const                   { return m_crossComponentPredictionEnabledFlag;   }
  Void                   setCrossComponentPredictionEnabledFlag(Bool value)               { m_crossComponentPredictionEnabledFlag = value;  }

  Void                   clearChromaQpOffsetList()                                        { m_chromaQpOffsetListLen = 0;                    }

  UInt                   getDiffCuChromaQpOffsetDepth () const                            { return m_diffCuChromaQpOffsetDepth;             }
  Void                   setDiffCuChromaQpOffsetDepth ( UInt u )                          { m_diffCuChromaQpOffsetDepth = u;                }

  Bool                   getChromaQpOffsetListEnabledFlag() const                         { return getChromaQpOffsetListLen()>0;            }
  Int                    getChromaQpOffsetListLen() const                                 { return m_chromaQpOffsetListLen;                 }

  const ChromaQpAdj&     getChromaQpOffsetListEntry( Int cuChromaQpOffsetIdxPlus1 ) const
  {
    assert(cuChromaQpOffsetIdxPlus1 < m_chromaQpOffsetListLen+1);
    return m_ChromaQpAdjTableIncludingNullEntry[cuChromaQpOffsetIdxPlus1]; // Array includes entry [0] for the null offset used when cu_chroma_qp_offset_flag=0, and entries [cu_chroma_qp_offset_idx+1...] otherwise
  }

  Void                   setChromaQpOffsetListEntry( Int cuChromaQpOffsetIdxPlus1, Int cbOffset, Int crOffset )
  {
    assert (cuChromaQpOffsetIdxPlus1 != 0 && cuChromaQpOffsetIdxPlus1 <= MAX_QP_OFFSET_LIST_SIZE);
    m_ChromaQpAdjTableIncludingNullEntry[cuChromaQpOffsetIdxPlus1].u.comp.CbOffset = cbOffset; // Array includes entry [0] for the null offset used when cu_chroma_qp_offset_flag=0, and entries [cu_chroma_qp_offset_idx+1...] otherwise
    m_ChromaQpAdjTableIncludingNullEntry[cuChromaQpOffsetIdxPlus1].u.comp.CrOffset = crOffset;
    m_chromaQpOffsetListLen = max(m_chromaQpOffsetListLen, cuChromaQpOffsetIdxPlus1);
  }

  // Now: getPpsRangeExtension().getLog2SaoOffsetScale and getPpsRangeExtension().setLog2SaoOffsetScale
  UInt                   getLog2SaoOffsetScale(ChannelType type) const                    { return m_log2SaoOffsetScale[type];             }
  Void                   setLog2SaoOffsetScale(ChannelType type, UInt uiBitShift)         { m_log2SaoOffsetScale[type] = uiBitShift;       }

};


/// PPS class
class TComPPS
{
private:
  Int              m_PPSId;                    // pic_parameter_set_id
  Int              m_SPSId;                    // seq_parameter_set_id
  Int              m_picInitQPMinus26;
  Bool             m_useDQP;
  Bool             m_bConstrainedIntraPred;    // constrained_intra_pred_flag
  Bool             m_bSliceChromaQpFlag;       // slicelevel_chroma_qp_flag

  // access channel
  UInt             m_uiMaxCuDQPDepth;

  Int              m_chromaCbQpOffset;
  Int              m_chromaCrQpOffset;

  UInt             m_numRefIdxL0DefaultActive;
  UInt             m_numRefIdxL1DefaultActive;

  Bool             m_bUseWeightPred;                    //!< Use of Weighting Prediction (P_SLICE)
  Bool             m_useWeightedBiPred;                 //!< Use of Weighting Bi-Prediction (B_SLICE)
  Bool             m_OutputFlagPresentFlag;             //!< Indicates the presence of output_flag in slice header
  Bool             m_TransquantBypassEnableFlag;        //!< Indicates presence of cu_transquant_bypass_flag in CUs.
  Bool             m_useTransformSkip;
  Bool             m_dependentSliceSegmentsEnabledFlag; //!< Indicates the presence of dependent slices
  Bool             m_tilesEnabledFlag;                  //!< Indicates the presence of tiles
  Bool             m_entropyCodingSyncEnabledFlag;      //!< Indicates the presence of wavefronts

  Bool             m_loopFilterAcrossTilesEnabledFlag;
  Bool             m_uniformSpacingFlag;
  Int              m_numTileColumnsMinus1;
  Int              m_numTileRowsMinus1;
  std::vector<Int> m_tileColumnWidth;
  std::vector<Int> m_tileRowHeight;

  Bool             m_signHideFlag;

  Bool             m_cabacInitPresentFlag;

  Bool             m_sliceHeaderExtensionPresentFlag;
  Bool             m_loopFilterAcrossSlicesEnabledFlag;
  Bool             m_deblockingFilterControlPresentFlag;
  Bool             m_deblockingFilterOverrideEnabledFlag;
  Bool             m_picDisableDeblockingFilterFlag;
  Int              m_deblockingFilterBetaOffsetDiv2;    //< beta offset for deblocking filter
  Int              m_deblockingFilterTcOffsetDiv2;      //< tc offset for deblocking filter
  Bool             m_scalingListPresentFlag;
  TComScalingList  m_scalingList;                       //!< ScalingList class
  Bool             m_listsModificationPresentFlag;
  UInt             m_log2ParallelMergeLevelMinus2;
  Int              m_numExtraSliceHeaderBits;

  TComPPSRExt      m_ppsRangeExtension;

#if NH_MV
  Int              m_layerId; 
  Bool             m_ppsInferScalingListFlag;
  Int              m_ppsScalingListRefLayerId;
                   
  Bool             m_ppsRangeExtensionsFlag;
  Bool             m_ppsMultilayerExtensionFlag;
  Bool             m_pps3dExtensionFlag;
  Int              m_ppsExtension5bits;

  Bool             m_pocResetInfoPresentFlag;
#endif

#if NH_3D_DLT
  TComDLT                m_cDLT;
#endif

public:
                         TComPPS();
  virtual                ~TComPPS();

  Int                    getPPSId() const                                                 { return m_PPSId;                               }
  Void                   setPPSId(Int i)                                                  { m_PPSId = i;                                  }
  Int                    getSPSId() const                                                 { return m_SPSId;                               }
  Void                   setSPSId(Int i)                                                  { m_SPSId = i;                                  }

  Int                    getPicInitQPMinus26() const                                      { return  m_picInitQPMinus26;                   }
  Void                   setPicInitQPMinus26( Int i )                                     { m_picInitQPMinus26 = i;                       }
  Bool                   getUseDQP() const                                                { return m_useDQP;                              }
  Void                   setUseDQP( Bool b )                                              { m_useDQP   = b;                               }
  Bool                   getConstrainedIntraPred() const                                  { return  m_bConstrainedIntraPred;              }
  Void                   setConstrainedIntraPred( Bool b )                                { m_bConstrainedIntraPred = b;                  }
  Bool                   getSliceChromaQpFlag() const                                     { return  m_bSliceChromaQpFlag;                 }
  Void                   setSliceChromaQpFlag( Bool b )                                   { m_bSliceChromaQpFlag = b;                     }

  Void                   setMaxCuDQPDepth( UInt u )                                       { m_uiMaxCuDQPDepth = u;                        }
  UInt                   getMaxCuDQPDepth() const                                         { return m_uiMaxCuDQPDepth;                     }

#if NH_3D_DLT
  Void                   setDLT( TComDLT cDLT )                                           { m_cDLT = cDLT;                                }
  const TComDLT*         getDLT() const                                                   { return &m_cDLT;                               }
  TComDLT*               getDLT()                                                         { return &m_cDLT;                               }
#endif


  Void                   setQpOffset(ComponentID compID, Int i )
  {
    if      (compID==COMPONENT_Cb)
    {
      m_chromaCbQpOffset = i;
    }
    else if (compID==COMPONENT_Cr)
    {
      m_chromaCrQpOffset = i;
    }
    else
    {
      assert(0);
    }
  }
  Int                    getQpOffset(ComponentID compID) const
  {
    return (compID==COMPONENT_Y) ? 0 : (compID==COMPONENT_Cb ? m_chromaCbQpOffset : m_chromaCrQpOffset );
  }

  Void                   setNumRefIdxL0DefaultActive(UInt ui)                             { m_numRefIdxL0DefaultActive=ui;                }
  UInt                   getNumRefIdxL0DefaultActive() const                              { return m_numRefIdxL0DefaultActive;            }
  Void                   setNumRefIdxL1DefaultActive(UInt ui)                             { m_numRefIdxL1DefaultActive=ui;                }
  UInt                   getNumRefIdxL1DefaultActive() const                              { return m_numRefIdxL1DefaultActive;            }

  Bool                   getUseWP() const                                                 { return m_bUseWeightPred;                      }
  Bool                   getWPBiPred() const                                              { return m_useWeightedBiPred;                   }
  Void                   setUseWP( Bool b )                                               { m_bUseWeightPred = b;                         }
  Void                   setWPBiPred( Bool b )                                            { m_useWeightedBiPred = b;                      }

  Void                   setOutputFlagPresentFlag( Bool b )                               { m_OutputFlagPresentFlag = b;                  }
  Bool                   getOutputFlagPresentFlag() const                                 { return m_OutputFlagPresentFlag;               }
  Void                   setTransquantBypassEnableFlag( Bool b )                          { m_TransquantBypassEnableFlag = b;             }
  Bool                   getTransquantBypassEnableFlag() const                            { return m_TransquantBypassEnableFlag;          }

  Bool                   getUseTransformSkip() const                                      { return m_useTransformSkip;                    }
  Void                   setUseTransformSkip( Bool b )                                    { m_useTransformSkip  = b;                      }

  Void                   setLoopFilterAcrossTilesEnabledFlag(Bool b)                      { m_loopFilterAcrossTilesEnabledFlag = b;       }
  Bool                   getLoopFilterAcrossTilesEnabledFlag() const                      { return m_loopFilterAcrossTilesEnabledFlag;    }
  Bool                   getDependentSliceSegmentsEnabledFlag() const                     { return m_dependentSliceSegmentsEnabledFlag;   }
  Void                   setDependentSliceSegmentsEnabledFlag(Bool val)                   { m_dependentSliceSegmentsEnabledFlag = val;    }
  Bool                   getEntropyCodingSyncEnabledFlag() const                          { return m_entropyCodingSyncEnabledFlag;        }
  Void                   setEntropyCodingSyncEnabledFlag(Bool val)                        { m_entropyCodingSyncEnabledFlag = val;         }

  Void                   setTilesEnabledFlag(Bool val)                                    { m_tilesEnabledFlag = val;                     }
  Bool                   getTilesEnabledFlag() const                                      { return m_tilesEnabledFlag;                    }
  Void                   setTileUniformSpacingFlag(Bool b)                                { m_uniformSpacingFlag = b;                     }
  Bool                   getTileUniformSpacingFlag() const                                { return m_uniformSpacingFlag;                  }
  Void                   setNumTileColumnsMinus1(Int i)                                   { m_numTileColumnsMinus1 = i;                   }
  Int                    getNumTileColumnsMinus1() const                                  { return m_numTileColumnsMinus1;                }
  Void                   setTileColumnWidth(const std::vector<Int>& columnWidth )         { m_tileColumnWidth = columnWidth;              }
  UInt                   getTileColumnWidth(UInt columnIdx) const                         { return  m_tileColumnWidth[columnIdx];         }
  Void                   setNumTileRowsMinus1(Int i)                                      { m_numTileRowsMinus1 = i;                      }
  Int                    getNumTileRowsMinus1() const                                     { return m_numTileRowsMinus1;                   }
  Void                   setTileRowHeight(const std::vector<Int>& rowHeight)              { m_tileRowHeight = rowHeight;                  }
  UInt                   getTileRowHeight(UInt rowIdx) const                              { return m_tileRowHeight[rowIdx];               }

  Void                   setSignHideFlag( Bool signHideFlag )                             { m_signHideFlag = signHideFlag;                }
  Bool                   getSignHideFlag() const                                          { return m_signHideFlag;                        }

  Void                   setCabacInitPresentFlag( Bool flag )                             { m_cabacInitPresentFlag = flag;                }
  Bool                   getCabacInitPresentFlag() const                                  { return m_cabacInitPresentFlag;                }
  Void                   setDeblockingFilterControlPresentFlag( Bool val )                { m_deblockingFilterControlPresentFlag = val;   }
  Bool                   getDeblockingFilterControlPresentFlag() const                    { return m_deblockingFilterControlPresentFlag;  }
  Void                   setDeblockingFilterOverrideEnabledFlag( Bool val )               { m_deblockingFilterOverrideEnabledFlag = val;  }
  Bool                   getDeblockingFilterOverrideEnabledFlag() const                   { return m_deblockingFilterOverrideEnabledFlag; }
  Void                   setPicDisableDeblockingFilterFlag(Bool val)                      { m_picDisableDeblockingFilterFlag = val;       } //!< set offset for deblocking filter disabled
  Bool                   getPicDisableDeblockingFilterFlag() const                        { return m_picDisableDeblockingFilterFlag;      } //!< get offset for deblocking filter disabled
  Void                   setDeblockingFilterBetaOffsetDiv2(Int val)                       { m_deblockingFilterBetaOffsetDiv2 = val;       } //!< set beta offset for deblocking filter
  Int                    getDeblockingFilterBetaOffsetDiv2() const                        { return m_deblockingFilterBetaOffsetDiv2;      } //!< get beta offset for deblocking filter
  Void                   setDeblockingFilterTcOffsetDiv2(Int val)                         { m_deblockingFilterTcOffsetDiv2 = val;         } //!< set tc offset for deblocking filter
  Int                    getDeblockingFilterTcOffsetDiv2() const                          { return m_deblockingFilterTcOffsetDiv2;        } //!< get tc offset for deblocking filter
  Bool                   getScalingListPresentFlag() const                                { return m_scalingListPresentFlag;              }
  Void                   setScalingListPresentFlag( Bool b )                              { m_scalingListPresentFlag  = b;                }
  TComScalingList&       getScalingList()                                                 { return m_scalingList;                         }
  const TComScalingList& getScalingList() const                                           { return m_scalingList;                         }
  Bool                   getListsModificationPresentFlag() const                          { return m_listsModificationPresentFlag;        }
  Void                   setListsModificationPresentFlag( Bool b )                        { m_listsModificationPresentFlag = b;           }
  UInt                   getLog2ParallelMergeLevelMinus2() const                          { return m_log2ParallelMergeLevelMinus2;        }
  Void                   setLog2ParallelMergeLevelMinus2(UInt mrgLevel)                   { m_log2ParallelMergeLevelMinus2 = mrgLevel;    }
  Int                    getNumExtraSliceHeaderBits() const                               { return m_numExtraSliceHeaderBits;             }
  Void                   setNumExtraSliceHeaderBits(Int i)                                { m_numExtraSliceHeaderBits = i;                }
  Void                   setLoopFilterAcrossSlicesEnabledFlag( Bool bValue )              { m_loopFilterAcrossSlicesEnabledFlag = bValue; }
  Bool                   getLoopFilterAcrossSlicesEnabledFlag() const                     { return m_loopFilterAcrossSlicesEnabledFlag;   }
  Bool                   getSliceHeaderExtensionPresentFlag() const                       { return m_sliceHeaderExtensionPresentFlag;     }
  Void                   setSliceHeaderExtensionPresentFlag(Bool val)                     { m_sliceHeaderExtensionPresentFlag = val;      }


  const TComPPSRExt&     getPpsRangeExtension() const                                     { return m_ppsRangeExtension;                   }
  TComPPSRExt&           getPpsRangeExtension()                                           { return m_ppsRangeExtension;                   }

  // WAS: getTransformSkipLog2MaxSize and setTransformSkipLog2MaxSize
  // Now: getPpsRangeExtension().getLog2MaxTransformSkipBlockSize and getPpsRangeExtension().setLog2MaxTransformSkipBlockSize

  // WAS: getUseCrossComponentPrediction and setUseCrossComponentPrediction
  // Now: getPpsRangeExtension().getCrossComponentPredictionEnabledFlag and getPpsRangeExtension().setCrossComponentPredictionEnabledFlag

  // WAS: clearChromaQpAdjTable
  // Now: getPpsRangeExtension().clearChromaQpOffsetList

  // WAS: getMaxCuChromaQpAdjDepth and setMaxCuChromaQpAdjDepth
  // Now: getPpsRangeExtension().getDiffCuChromaQpOffsetDepth and getPpsRangeExtension().setDiffCuChromaQpOffsetDepth

  // WAS: getChromaQpAdjTableSize
  // Now: getPpsRangeExtension().getChromaQpOffsetListLen

  // WAS: getChromaQpAdjTableAt and setChromaQpAdjTableAt
  // Now: getPpsRangeExtension().getChromaQpOffsetListEntry and getPpsRangeExtension().setChromaQpOffsetListEntry

  // WAS: getSaoOffsetBitShift and setSaoOffsetBitShift
  // Now: getPpsRangeExtension().getLog2SaoOffsetScale and getPpsRangeExtension().setLog2SaoOffsetScale

#if NH_MV
  Void    setLayerId( Int  val )                                                     { m_layerId = val;                                           }
  Int     getLayerId(  ) const                                                       { return m_layerId;                                          }

  Void    setPpsInferScalingListFlag( Bool flag )                                    { m_ppsInferScalingListFlag = flag;                          }
  Bool    getPpsInferScalingListFlag(  ) const                                       { return m_ppsInferScalingListFlag;                          }

  Void    setPpsScalingListRefLayerId( Int  val )                                    { m_ppsScalingListRefLayerId = val;                          }
  Int     getPpsScalingListRefLayerId(  ) const                                      { return m_ppsScalingListRefLayerId;                         }

  Void    setPpsRangeExtensionsFlag( Bool flag )                                     { m_ppsRangeExtensionsFlag = flag;                           }
  Bool    getPpsRangeExtensionsFlag(  ) const                                        { return m_ppsRangeExtensionsFlag;                           }

  Void    setPpsMultilayerExtensionFlag( Bool flag )                                 { m_ppsMultilayerExtensionFlag = flag;                       }
  Bool    getPpsMultilayerExtensionFlag(  ) const                                    { return m_ppsMultilayerExtensionFlag;                       }

  Void    setPps3dExtensionFlag( Bool flag )                                         { m_pps3dExtensionFlag = flag;                               }
  Bool    getPps3dExtensionFlag(  ) const                                            { return m_pps3dExtensionFlag;                               }

  Void    setPpsExtension5bits( Int  val )                                           { m_ppsExtension5bits = val;                                 }
  Int     getPpsExtension5bits(  ) const                                             { return m_ppsExtension5bits;                                }

  Void    setPocResetInfoPresentFlag( Bool flag )                                    { m_pocResetInfoPresentFlag = flag;                          }
  Bool    getPocResetInfoPresentFlag(  ) const                                       { return m_pocResetInfoPresentFlag;                          }
#endif
};
struct WPScalingParam
{
  // Explicit weighted prediction parameters parsed in slice header,
  // or Implicit weighted prediction parameters (8 bits depth values).
  Bool bPresentFlag;
  UInt uiLog2WeightDenom;
  Int  iWeight;
  Int  iOffset;

  // Weighted prediction scaling values built from above parameters (bitdepth scaled):
  Int  w;
  Int  o;
  Int  offset;
  Int  shift;
  Int  round;
};

struct WPACDCParam
{
  Int64 iAC;
  Int64 iDC;
};

/// slice header class
class TComSlice
{

private:
  //  Bitstream writing
  Bool                       m_saoEnabledFlag[MAX_NUM_CHANNEL_TYPE];
  Int                        m_iPPSId;               ///< picture parameter set ID
  Bool                       m_PicOutputFlag;        ///< pic_output_flag
#if NH_MV
  Int                        m_slicePicOrderCntLsb;   
#endif  
  Int                        m_iPOC;
#if NH_MV
  Int                        m_iPOCBeforeReset; 
#endif
  Int                        m_iLastIDR;
  Int                        m_iAssociatedIRAP;
  NalUnitType                m_iAssociatedIRAPType;
  const TComReferencePictureSet* m_pRPS;             //< pointer to RPS, either in the SPS or the local RPS in the same slice header
  TComReferencePictureSet    m_localRPS;             //< RPS when present in slice header
  Int                        m_rpsIdx;               //< index of used RPS in the SPS or -1 for local RPS in the slice header
  TComRefPicListModification m_RefPicListModification;
  NalUnitType                m_eNalUnitType;         ///< Nal unit type for the slice
  SliceType                  m_eSliceType;
  Int                        m_iSliceQp;
  Bool                       m_dependentSliceSegmentFlag;
#if ADAPTIVE_QP_SELECTION
  Int                        m_iSliceQpBase;
#endif
  Bool                       m_ChromaQpAdjEnabled;
  Bool                       m_deblockingFilterDisable;
  Bool                       m_deblockingFilterOverrideFlag;      //< offsets for deblocking filter inherit from PPS
  Int                        m_deblockingFilterBetaOffsetDiv2;    //< beta offset for deblocking filter
  Int                        m_deblockingFilterTcOffsetDiv2;      //< tc offset for deblocking filter
  Int                        m_list1IdxToList0Idx[MAX_NUM_REF];
  Int                        m_aiNumRefIdx   [NUM_REF_PIC_LIST_01];    //  for multiple reference of current slice

  Bool                       m_bCheckLDC;

  //  Data
  Int                        m_iSliceQpDelta;
  Int                        m_iSliceChromaQpDelta[MAX_NUM_COMPONENT];
  TComPic*                   m_apcRefPicList [NUM_REF_PIC_LIST_01][MAX_NUM_REF+1];
  Int                        m_aiRefPOCList  [NUM_REF_PIC_LIST_01][MAX_NUM_REF+1];
#if NH_MV
  Int         m_aiRefLayerIdList[2][MAX_NUM_REF+1];
#endif
  Bool                       m_bIsUsedAsLongTerm[NUM_REF_PIC_LIST_01][MAX_NUM_REF+1];
  Int                        m_iDepth;

  // referenced slice?
  Bool                       m_bRefenced;

  // access channel
  const TComVPS*             m_pcVPS;
  const TComSPS*             m_pcSPS;
  const TComPPS*             m_pcPPS;
  TComPic*                   m_pcPic;
  Bool                       m_colFromL0Flag;  // collocated picture from List0 flag

  Bool                       m_noOutputPriorPicsFlag;
  Bool                       m_noRaslOutputFlag;
  Bool                       m_handleCraAsBlaFlag;

  UInt                       m_colRefIdx;
  UInt                       m_maxNumMergeCand;

  Double                     m_lambdas[MAX_NUM_COMPONENT];

  Bool                       m_abEqualRef  [NUM_REF_PIC_LIST_01][MAX_NUM_REF][MAX_NUM_REF];
  UInt                       m_uiTLayer;
  Bool                       m_bTLayerSwitchingFlag;

  SliceConstraint            m_sliceMode;
  UInt                       m_sliceArgument;
  UInt                       m_sliceCurStartCtuTsAddr;
  UInt                       m_sliceCurEndCtuTsAddr;
  UInt                       m_sliceIdx;
  SliceConstraint            m_sliceSegmentMode;
  UInt                       m_sliceSegmentArgument;
  UInt                       m_sliceSegmentCurStartCtuTsAddr;
  UInt                       m_sliceSegmentCurEndCtuTsAddr;
  Bool                       m_nextSlice;
  Bool                       m_nextSliceSegment;
  UInt                       m_sliceBits;
  UInt                       m_sliceSegmentBits;
  Bool                       m_bFinalized;

  Bool                       m_bTestWeightPred;
  Bool                       m_bTestWeightBiPred;
  WPScalingParam             m_weightPredTable[NUM_REF_PIC_LIST_01][MAX_NUM_REF][MAX_NUM_COMPONENT]; // [REF_PIC_LIST_0 or REF_PIC_LIST_1][refIdx][0:Y, 1:U, 2:V]
  WPACDCParam                m_weightACDCParam[MAX_NUM_COMPONENT];

  std::vector<UInt>          m_substreamSizes;

  Bool                       m_cabacInitFlag;

  Bool                       m_bLMvdL1Zero;
  Bool                       m_temporalLayerNonReferenceFlag;
  Bool                       m_LFCrossSliceBoundaryFlag;

  Bool                       m_enableTMVPFlag;

  SliceType                  m_encCABACTableIdx;           // Used to transmit table selection across slices.
#if NH_MV
  Bool       m_availableForTMVPRefFlag;
#endif

#if NH_MV
  std::vector<TComPic*>* m_refPicSetInterLayer0; 
  std::vector<TComPic*>* m_refPicSetInterLayer1; 
  Int        m_layerId; 
  Int        m_viewId;
  Int        m_viewIndex; 
#if NH_3D
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

  Int        m_sliceSegmentHeaderExtensionLength;
  Int        m_pocResetIdc;
  Int        m_pocResetPeriodId;
  Bool       m_fullPocResetFlag;
  Int        m_pocLsbVal;
  Bool       m_pocMsbValPresentFlag;
  Int        m_pocMsbVal;
  Bool       m_pocMsbValRequiredFlag;

#if NH_3D
  IntAry2d   m_aaiCodedScale ;
  IntAry2d   m_aaiCodedOffset;
#endif
#if NH_3D_TMVP
  Int        m_aiAlterRefIdx   [2]; 
#endif
#if NH_3D_ARP
  Bool       m_arpRefPicAvailable[2][MAX_NUM_LAYERS];
  TComList<TComPic*> * m_pBaseViewRefPicList[MAX_NUM_LAYERS];
  UInt        m_nARPStepNum; 
  Int         m_aiFirstTRefIdx    [2];   
#endif
#if NH_3D
  std::vector<Int> m_pocsInCurrRPSs; 
#endif
#if NH_3D_IC
  Bool       m_bApplyIC;
  Bool       m_icSkipParseFlag;
#endif
#if NH_3D
  std::vector<Int> m_inCmpRefViewIdcs;
  Bool       m_inCmpPredAvailFlag; 
  Bool       m_inCmpPredFlag; 
  Bool       m_cpAvailableFlag; 
  Int        m_numViews; 
  TComPic*   m_ivPicsCurrPoc [2][MAX_NUM_LAYERS];  
  Int**      m_depthToDisparityB; 
  Int**      m_depthToDisparityF; 
  Bool       m_bApplyDIS;
#endif
#endif
#if NH_3D_IC
  Int*       m_aICEnableCandidate;
  Int*       m_aICEnableNum;
#endif       
#if NH_3D   
  Int        m_iDefaultRefViewIdx;
  Bool       m_bDefaultRefViewIdxAvailableFlag;
             
  Bool       m_ivMvPredFlag         ;
  Bool       m_ivMvScalingFlag      ;
  Bool       m_ivResPredFlag        ;
  Bool       m_depthRefinementFlag  ;
  Bool       m_viewSynthesisPredFlag;
  Bool       m_depthBasedBlkPartFlag;
  Bool       m_mpiFlag              ;
  Bool       m_intraContourFlag     ;
  Bool       m_intraSdcWedgeFlag    ;
  Bool       m_qtPredFlag           ;
  Bool       m_interSdcFlag         ;
  Bool       m_depthIntraSkipFlag   ;
  Int        m_mpiSubPbSize         ; 
  Int        m_subPbSize            ; 
#endif
public:
                              TComSlice();
  virtual                     ~TComSlice();
  Void                        initSlice();

  Void                        setVPS( TComVPS* pcVPS )                               { m_pcVPS = pcVPS;                                              }
  const TComVPS*              getVPS() const                                         { return m_pcVPS;                                               }
  Void                        setSPS( const TComSPS* pcSPS )                         { m_pcSPS = pcSPS;                                              }
  const TComSPS*              getSPS() const                                         { return m_pcSPS;                                               }

  Void                        setPPS( const TComPPS* pcPPS )                         { m_pcPPS = pcPPS; m_iPPSId = (pcPPS) ? pcPPS->getPPSId() : -1; }
  const TComPPS*              getPPS() const                                         { return m_pcPPS;                                               }

  Void                        setPPSId( Int PPSId )                                  { m_iPPSId = PPSId;                                             }
  Int                         getPPSId() const                                       { return m_iPPSId;                                              }
  Void                        setPicOutputFlag( Bool b   )                           { m_PicOutputFlag = b;                                          }
#if NH_MV
  Void                        setSlicePicOrderCntLsb( Int i )                        { m_slicePicOrderCntLsb = i;                                    }
  Int                         getSlicePicOrderCntLsb(  )  const                      { return m_slicePicOrderCntLsb;                                 }
#endif
  Bool                        getPicOutputFlag() const                               { return m_PicOutputFlag;                                       }
  Void                        setSaoEnabledFlag(ChannelType chType, Bool s)          {m_saoEnabledFlag[chType] =s;                                   }
  Bool                        getSaoEnabledFlag(ChannelType chType) const            { return m_saoEnabledFlag[chType];                              }
  Void                        setRPS( const TComReferencePictureSet *pcRPS )         { m_pRPS = pcRPS;                                               }
  const TComReferencePictureSet* getRPS()                                            { return m_pRPS;                                                }
  TComReferencePictureSet*    getLocalRPS()                                          { return &m_localRPS;                                           }

  Void                        setRPSidx( Int rpsIdx )                                { m_rpsIdx = rpsIdx;                                            }
  Int                         getRPSidx() const                                      { return m_rpsIdx;                                              }
  TComRefPicListModification* getRefPicListModification()                            { return &m_RefPicListModification;                             }
  Void                        setLastIDR(Int iIDRPOC)                                { m_iLastIDR = iIDRPOC;                                         }
  Int                         getLastIDR() const                                     { return m_iLastIDR;                                            }
  Void                        setAssociatedIRAPPOC(Int iAssociatedIRAPPOC)           { m_iAssociatedIRAP = iAssociatedIRAPPOC;                       }
  Int                         getAssociatedIRAPPOC() const                           { return m_iAssociatedIRAP;                                     }
  Void                        setAssociatedIRAPType(NalUnitType associatedIRAPType)  { m_iAssociatedIRAPType = associatedIRAPType;                   }
  NalUnitType                 getAssociatedIRAPType() const                          { return m_iAssociatedIRAPType;                                 }
  SliceType                   getSliceType() const                                   { return m_eSliceType;                                          }
  Int                         getPOC() const                                         { return m_iPOC;                                                }
  Int                         getSliceQp() const                                     { return m_iSliceQp;                                            }
  Bool                        getDependentSliceSegmentFlag() const                   { return m_dependentSliceSegmentFlag;                           }
  Void                        setDependentSliceSegmentFlag(Bool val)                 { m_dependentSliceSegmentFlag = val;                            }
#if ADAPTIVE_QP_SELECTION
  Int                         getSliceQpBase() const                                 { return m_iSliceQpBase;                                        }
#endif
  Int                         getSliceQpDelta() const                                { return m_iSliceQpDelta;                                       }
  Int                         getSliceChromaQpDelta(ComponentID compID) const        { return isLuma(compID) ? 0 : m_iSliceChromaQpDelta[compID];    }
  Bool                        getUseChromaQpAdj() const                              { return m_ChromaQpAdjEnabled;                                  }
  Bool                        getDeblockingFilterDisable() const                     { return m_deblockingFilterDisable;                             }
  Bool                        getDeblockingFilterOverrideFlag() const                { return m_deblockingFilterOverrideFlag;                        }
  Int                         getDeblockingFilterBetaOffsetDiv2()const               { return m_deblockingFilterBetaOffsetDiv2;                      }
  Int                         getDeblockingFilterTcOffsetDiv2() const                { return m_deblockingFilterTcOffsetDiv2;                        }

  Int                         getNumRefIdx( RefPicList e ) const                     { return m_aiNumRefIdx[e];                                      }
  TComPic*                    getPic()                                               { return m_pcPic;                                               }
  TComPic*                    getRefPic( RefPicList e, Int iRefIdx)                  { return m_apcRefPicList[e][iRefIdx];                           }
  Int                         getRefPOC( RefPicList e, Int iRefIdx)                  { return m_aiRefPOCList[e][iRefIdx];                            }
#if NH_3D
  Bool                        getInCmpPredAvailFlag( )                 const         { return m_inCmpPredAvailFlag;                                  }
  Bool                        getCpAvailableFlag( )                    const         { return m_cpAvailableFlag;                                     }
  Bool                        getInCompPredFlag( )                     const         { return m_inCmpPredFlag;                                       }
  Void                        setInCompPredFlag( Bool b )                            { m_inCmpPredFlag = b;                                          }
  Int                         getInCmpRefViewIdcs( Int i )             const         { return m_inCmpRefViewIdcs  [i];                               }
  Int                         getNumCurCmpLIds( )                      const         { return (Int) m_inCmpRefViewIdcs.size();                       }
  TComPic*                    getIvPic( Bool depthFlag, Int viewIndex) const         { return  m_ivPicsCurrPoc[ depthFlag ? 1 : 0 ][ viewIndex ];    }
  TComPic*                    getTexturePic       ()                                 { return  m_ivPicsCurrPoc[0][ m_viewIndex ];                    }
#endif                            
#if NH_3D_IC                                                                                                                                          
  Void                        setApplyIC( Bool b )                                   { m_bApplyIC = b;                                               }
  Bool                        getApplyIC()                                           { return m_bApplyIC;                                            }
  Void                        xSetApplyIC();                                                                                                         
  Void                        xSetApplyIC(Bool bUseLowLatencyICEnc);                                                                                 
  Void                        setIcSkipParseFlag( Bool b )                           { m_icSkipParseFlag = b;                                        }
  Bool                        getIcSkipParseFlag()                                   { return m_icSkipParseFlag;                                     }
#endif                                                                                                                                               
#if NH_3D_ARP                                                                                                                                         
  Void                        setBaseViewRefPicList( TComList<TComPic*> *pListPic, Int iViewIdx )      { m_pBaseViewRefPicList[iViewIdx] = pListPic; }                  
  Void                        setARPStepNum( TComPicLists*ivPicLists );                                                                              
  TComPic*                    getBaseViewRefPic    ( UInt uiPOC , Int iViewIdx )     { return xGetRefPic( *m_pBaseViewRefPicList[iViewIdx], uiPOC ); }
  UInt                        getARPStepNum( )                                       { return m_nARPStepNum;                                         }  
#endif
  Int                         getDepth() const                                       { return m_iDepth;                                              }
  Bool                        getColFromL0Flag() const                               { return m_colFromL0Flag;                                       }
  UInt                        getColRefIdx() const                                   { return m_colRefIdx;                                           }
  Void                        checkColRefIdx(UInt curSliceIdx, TComPic* pic);
  Bool                        getIsUsedAsLongTerm(Int i, Int j) const                { return m_bIsUsedAsLongTerm[i][j];                             }
  Void                        setIsUsedAsLongTerm(Int i, Int j, Bool value)          { m_bIsUsedAsLongTerm[i][j] = value;                            }
  Bool                        getCheckLDC() const                                    { return m_bCheckLDC;                                           }
  Bool                        getMvdL1ZeroFlag() const                               { return m_bLMvdL1Zero;                                         }
  Int                         getNumRpsCurrTempList() const;
  Int                         getList1IdxToList0Idx( Int list1Idx ) const            { return m_list1IdxToList0Idx[list1Idx];                        }
  Void                        setReferenced(Bool b)                                  { m_bRefenced = b;                                              }
  Bool                        isReferenced() const                                   { return m_bRefenced;                                           }
  Bool                        isReferenceNalu() const                                { return ((getNalUnitType() <= NAL_UNIT_RESERVED_VCL_R15) && (getNalUnitType()%2 != 0)) || ((getNalUnitType() >= NAL_UNIT_CODED_SLICE_BLA_W_LP) && (getNalUnitType() <= NAL_UNIT_RESERVED_IRAP_VCL23) ); }
  Void                        setPOC( Int i )                                        { m_iPOC              = i; }
  Void                        setNalUnitType( NalUnitType e )                        { m_eNalUnitType      = e;                                      }
  NalUnitType                 getNalUnitType() const                                 { return m_eNalUnitType;                                        }
  Bool                        getRapPicFlag() const;
  Bool                        getIdrPicFlag() const                                  { return getNalUnitType() == NAL_UNIT_CODED_SLICE_IDR_W_RADL || getNalUnitType() == NAL_UNIT_CODED_SLICE_IDR_N_LP; }
  Bool                        isIRAP() const                                         { return (getNalUnitType() >= 16) && (getNalUnitType() <= 23);  }
  Void                        checkCRA(const TComReferencePictureSet *pReferencePictureSet, Int& pocCRA, NalUnitType& associatedIRAPType, TComList<TComPic *>& rcListPic);
  Void                        decodingRefreshMarking(Int& pocCRA, Bool& bRefreshPending, TComList<TComPic*>& rcListPic, const bool bEfficientFieldIRAPEnabled);
  Void                        setSliceType( SliceType e )                            { m_eSliceType        = e;                                      }
  Void                        setSliceQp( Int i )                                    { m_iSliceQp          = i;                                      }
#if ADAPTIVE_QP_SELECTION
  Void                        setSliceQpBase( Int i )                                { m_iSliceQpBase      = i;                                      }
#endif
  Void                        setSliceQpDelta( Int i )                               { m_iSliceQpDelta     = i;                                      }
  Void                        setSliceChromaQpDelta( ComponentID compID, Int i )     { m_iSliceChromaQpDelta[compID] = isLuma(compID) ? 0 : i;       }
  Void                        setUseChromaQpAdj( Bool b )                            { m_ChromaQpAdjEnabled = b;                                     }
  Void                        setDeblockingFilterDisable( Bool b )                   { m_deblockingFilterDisable= b;                                 }
  Void                        setDeblockingFilterOverrideFlag( Bool b )              { m_deblockingFilterOverrideFlag = b;                           }
  Void                        setDeblockingFilterBetaOffsetDiv2( Int i )             { m_deblockingFilterBetaOffsetDiv2 = i;                         }
  Void                        setDeblockingFilterTcOffsetDiv2( Int i )               { m_deblockingFilterTcOffsetDiv2 = i;                           }

  Void                        setRefPic( TComPic* p, RefPicList e, Int iRefIdx )     { m_apcRefPicList[e][iRefIdx] = p;                              }
  Void                        setRefPOC( Int i, RefPicList e, Int iRefIdx )          { m_aiRefPOCList[e][iRefIdx] = i;                               }
  Void                        setNumRefIdx( RefPicList e, Int i )                    { m_aiNumRefIdx[e]    = i;                                      }
  Void                        setPic( TComPic* p )                                   { m_pcPic             = p;                                      }
  Void                        setDepth( Int iDepth )                                 { m_iDepth            = iDepth;                                 }
#if NH_MV
  Void                        setPocBeforeReset( Int i )                             { m_iPOCBeforeReset = i;                                        }
  Int                         getPocBeforeReset( )                                   { return m_iPOCBeforeReset;                                     }
  Int                         getRefLayerId( RefPicList e, Int iRefIdx)              { return  m_aiRefLayerIdList[e][iRefIdx];                       }
  Void                        setRefLayerId( Int i, RefPicList e, Int iRefIdx )      { m_aiRefLayerIdList[e][iRefIdx] = i;                        }
  Void                        getTempRefPicLists   ( TComList<TComPic*>& rcListPic, std::vector<TComPic*>& refPicSetInterLayer0, std::vector<TComPic*>& refPicSetInterLayer1,                                     
                                                     std::vector<TComPic*> rpsCurrList[2], BoolAry1d usedAsLongTerm[2], Int& numPocTotalCurr, Bool checkNumPocTotalCurr = false );
                              
  Void                        setRefPicList        ( std::vector<TComPic*> rpsCurrList[2], BoolAry1d usedAsLongTerm[2], Int numPocTotalCurr, Bool checkNumPocTotalCurr = false ); 
#else
  Void                        setRefPicList( TComList<TComPic*>& rcListPic, Bool checkNumPocTotalCurr = false );
#endif
  Void                        setRefPOCList();
  Void                        setColFromL0Flag( Bool colFromL0 )                     { m_colFromL0Flag = colFromL0;                                  }
  Void                        setColRefIdx( UInt refIdx)                             { m_colRefIdx = refIdx;                                         }
  Void                        setCheckLDC( Bool b )                                  { m_bCheckLDC = b;                                              }
  Void                        setMvdL1ZeroFlag( Bool b)                              { m_bLMvdL1Zero = b;                                            }

  Bool                        isIntra() const                                        { return m_eSliceType == I_SLICE;                               }
  Bool                        isInterB() const                                       { return m_eSliceType == B_SLICE;                               }
  Bool                        isInterP() const                                       { return m_eSliceType == P_SLICE;                               }

  Void                        setLambdas( const Double lambdas[MAX_NUM_COMPONENT] )  { for (Int component = 0; component < MAX_NUM_COMPONENT; component++) m_lambdas[component] = lambdas[component]; }
  const Double*               getLambdas() const                                     { return m_lambdas;                                             }

  Void                        initEqualRef();
  Bool                        isEqualRef( RefPicList e, Int iRefIdx1, Int iRefIdx2 )
  {
    assert(e<NUM_REF_PIC_LIST_01);
    if (iRefIdx1 < 0 || iRefIdx2 < 0)
    {
      return false;
    }
    else
    {
      return m_abEqualRef[e][iRefIdx1][iRefIdx2];
    }
  }

  Void                        setEqualRef( RefPicList e, Int iRefIdx1, Int iRefIdx2, Bool b)
  {
    assert(e<NUM_REF_PIC_LIST_01);
    m_abEqualRef[e][iRefIdx1][iRefIdx2] = m_abEqualRef[e][iRefIdx2][iRefIdx1] = b;
  }

  static Void                 sortPicList( TComList<TComPic*>& rcListPic );
  Void                        setList1IdxToList0Idx();

  UInt                        getTLayer() const                                      { return m_uiTLayer;                                            }
  Void                        setTLayer( UInt uiTLayer )                             { m_uiTLayer = uiTLayer;                                        }
#if NH_MV
  Int                         getTemporalId          ( )                             { return (Int) m_uiTLayer;                                      }
#endif

  Void                        setTLayerInfo( UInt uiTLayer );
  Void                        decodingMarking( TComList<TComPic*>& rcListPic, Int iGOPSIze, Int& iMaxRefPicNum );
  Void                        checkLeadingPictureRestrictions( TComList<TComPic*>& rcListPic );
  Void                        applyReferencePictureSet( TComList<TComPic*>& rcListPic, const TComReferencePictureSet *RPSList);
#if NH_MV
  Void                        createInterLayerReferencePictureSet( TComPicLists* ivPicLists, std::vector<TComPic*>& refPicSetInterLayer0, std::vector<TComPic*>& refPicSetInterLayer1 );
  static Void                 markIvRefPicsAsShortTerm    ( std::vector<TComPic*> refPicSetInterLayer0, std::vector<TComPic*> refPicSetInterLayer1 );
  static Void                 markCurrPic                 ( TComPic* currPic );
  Void                        printRefPicList();
#endif
  Bool                        isTemporalLayerSwitchingPoint( TComList<TComPic*>& rcListPic );
  Bool                        isStepwiseTemporalLayerSwitchingPointCandidate( TComList<TComPic*>& rcListPic );
  Int                         checkThatAllRefPicsAreAvailable( TComList<TComPic*>& rcListPic, const TComReferencePictureSet *pReferencePictureSet, Bool printErrors, Int pocRandomAccess = 0, Bool bUseRecoveryPoint = false);
  Void                        createExplicitReferencePictureSetFromReference( TComList<TComPic*>& rcListPic, const TComReferencePictureSet *pReferencePictureSet, Bool isRAP, Int pocRandomAccess, Bool bUseRecoveryPoint, const Bool bEfficientFieldIRAPEnabled);
  Void                        setMaxNumMergeCand(UInt val )                          { m_maxNumMergeCand = val;                                      }
  UInt                        getMaxNumMergeCand() const                             { return m_maxNumMergeCand;                                     }

  Void                        setNoOutputPriorPicsFlag( Bool val )                   { m_noOutputPriorPicsFlag = val;                                }
  Bool                        getNoOutputPriorPicsFlag() const                       { return m_noOutputPriorPicsFlag;                               }

  Void                        setNoRaslOutputFlag( Bool val )                        { m_noRaslOutputFlag = val;                                     }
  Bool                        getNoRaslOutputFlag() const                            { return m_noRaslOutputFlag;                                    }

  Void                        setHandleCraAsBlaFlag( Bool val )                      { m_handleCraAsBlaFlag = val;                                   }
  Bool                        getHandleCraAsBlaFlag() const                          { return m_handleCraAsBlaFlag;                                  }

  Void                        setSliceMode( SliceConstraint mode )                   { m_sliceMode = mode;                                           }
  SliceConstraint             getSliceMode() const                                   { return m_sliceMode;                                           }
  Void                        setSliceArgument( UInt uiArgument )                    { m_sliceArgument = uiArgument;                                 }
  UInt                        getSliceArgument() const                               { return m_sliceArgument;                                       }
  Void                        setSliceCurStartCtuTsAddr( UInt ctuTsAddr )            { m_sliceCurStartCtuTsAddr = ctuTsAddr;                         } // CTU Tile-scan address (as opposed to raster-scan)
  UInt                        getSliceCurStartCtuTsAddr() const                      { return m_sliceCurStartCtuTsAddr;                              } // CTU Tile-scan address (as opposed to raster-scan)
  Void                        setSliceCurEndCtuTsAddr( UInt ctuTsAddr )              { m_sliceCurEndCtuTsAddr = ctuTsAddr;                           } // CTU Tile-scan address (as opposed to raster-scan)
  UInt                        getSliceCurEndCtuTsAddr() const                        { return m_sliceCurEndCtuTsAddr;                                } // CTU Tile-scan address (as opposed to raster-scan)
  Void                        setSliceIdx( UInt i)                                   { m_sliceIdx = i;                                               }
  UInt                        getSliceIdx() const                                    { return  m_sliceIdx;                                           }
  Void                        copySliceInfo(TComSlice *pcSliceSrc);
  Void                        setSliceSegmentMode( SliceConstraint mode )            { m_sliceSegmentMode = mode;                                    }
  SliceConstraint             getSliceSegmentMode() const                            { return m_sliceSegmentMode;                                    }
  Void                        setSliceSegmentArgument( UInt uiArgument )             { m_sliceSegmentArgument = uiArgument;                          }
  UInt                        getSliceSegmentArgument() const                        { return m_sliceSegmentArgument;                                }
  Void                        setSliceSegmentCurStartCtuTsAddr( UInt ctuTsAddr )     { m_sliceSegmentCurStartCtuTsAddr = ctuTsAddr;                  } // CTU Tile-scan address (as opposed to raster-scan)
  UInt                        getSliceSegmentCurStartCtuTsAddr() const               { return m_sliceSegmentCurStartCtuTsAddr;                       } // CTU Tile-scan address (as opposed to raster-scan)
  Void                        setSliceSegmentCurEndCtuTsAddr( UInt ctuTsAddr )       { m_sliceSegmentCurEndCtuTsAddr = ctuTsAddr;                    } // CTU Tile-scan address (as opposed to raster-scan)
  UInt                        getSliceSegmentCurEndCtuTsAddr() const                 { return m_sliceSegmentCurEndCtuTsAddr;                         } // CTU Tile-scan address (as opposed to raster-scan)
  Void                        setSliceBits( UInt uiVal )                             { m_sliceBits = uiVal;                                          }
  UInt                        getSliceBits() const                                   { return m_sliceBits;                                           }
  Void                        setSliceSegmentBits( UInt uiVal )                      { m_sliceSegmentBits = uiVal;                                   }
  UInt                        getSliceSegmentBits() const                            { return m_sliceSegmentBits;                                    }
  Void                        setFinalized( Bool uiVal )                             { m_bFinalized = uiVal;                                         }
  Bool                        getFinalized() const                                   { return m_bFinalized;                                          }
  Bool                        testWeightPred( ) const                                { return m_bTestWeightPred;                                     }
  Void                        setTestWeightPred( Bool bValue )                       { m_bTestWeightPred = bValue;                                   }
  Bool                        testWeightBiPred( ) const                              { return m_bTestWeightBiPred;                                   }
  Void                        setTestWeightBiPred( Bool bValue )                     { m_bTestWeightBiPred = bValue;                                 }
  Void                        setWpScaling( WPScalingParam  wp[NUM_REF_PIC_LIST_01][MAX_NUM_REF][MAX_NUM_COMPONENT] )
  {
    memcpy(m_weightPredTable, wp, sizeof(WPScalingParam)*NUM_REF_PIC_LIST_01*MAX_NUM_REF*MAX_NUM_COMPONENT);
  }

  Void                        getWpScaling( RefPicList e, Int iRefIdx, WPScalingParam *&wp);

  Void                        resetWpScaling();
  Void                        initWpScaling(const TComSPS *sps);

  Void                        setWpAcDcParam( WPACDCParam wp[MAX_NUM_COMPONENT] )
  {
    memcpy(m_weightACDCParam, wp, sizeof(WPACDCParam)*MAX_NUM_COMPONENT);
  }

  Void                        getWpAcDcParam( WPACDCParam *&wp );
  Void                        initWpAcDcParam();

  Void                        clearSubstreamSizes( )                                 { return m_substreamSizes.clear();                              }
  UInt                        getNumberOfSubstreamSizes( )                           { return (UInt) m_substreamSizes.size();                        }
  Void                        addSubstreamSize( UInt size )                          { m_substreamSizes.push_back(size);                             }
  UInt                        getSubstreamSize( Int idx )                            { assert(idx<getNumberOfSubstreamSizes()); return m_substreamSizes[idx]; }

  Void                        setCabacInitFlag( Bool val )                           { m_cabacInitFlag = val;                                        } //!< set CABAC initial flag
  Bool                        getCabacInitFlag()                                     { return m_cabacInitFlag;                                       } //!< get CABAC initial flag
  Bool                        getTemporalLayerNonReferenceFlag()                     { return m_temporalLayerNonReferenceFlag;                       }
  Void                        setTemporalLayerNonReferenceFlag(Bool x)               { m_temporalLayerNonReferenceFlag = x;                          }
  Void                        setLFCrossSliceBoundaryFlag( Bool   val )              { m_LFCrossSliceBoundaryFlag = val;                             }
  Bool                        getLFCrossSliceBoundaryFlag()                          { return m_LFCrossSliceBoundaryFlag;                            }

  Void                        setEnableTMVPFlag( Bool   b )                          { m_enableTMVPFlag = b;                                         }
  Bool                        getEnableTMVPFlag()                                    { return m_enableTMVPFlag;                                      }

  Void                        setEncCABACTableIdx( SliceType idx )                   { m_encCABACTableIdx = idx;                                     }
  SliceType                   getEncCABACTableIdx() const                            { return m_encCABACTableIdx;                                    }

#if NH_MV
  Void                        setAvailableForTMVPRefFlag( Bool   b )                 { m_availableForTMVPRefFlag = b;                                }
  Bool                        getAvailableForTMVPRefFlag()                           { return m_availableForTMVPRefFlag;                             }

  Void                        setLayerId     ( Int layerId )                         { m_layerId      = layerId;                                     }
  Int                         getLayerId     ()                 const                { return m_layerId;                                             }
  Int                         getLayerIdInVps()                 const                { return getVPS()->getLayerIdInVps( m_layerId );                }
  Void                        setViewId      ( Int viewId )                          { m_viewId = viewId;                                            }
  Int                         getViewId      ()                 const                { return m_viewId;                                              }
  Void                        setViewIndex   ( Int viewIndex )                       { m_viewIndex = viewIndex;                                      }
  Int                         getViewIndex   ()                 const                { return m_viewIndex;                                           }
#if NH_3D
#if NH_3D_TMVP
  Void                        generateAlterRefforTMVP ();   
  Void                        setAlterRefIdx          ( RefPicList e, Int i )        { m_aiAlterRefIdx[e]    = i;                                    }
  Int                         getAlterRefIdx          ( RefPicList e )               { return  m_aiAlterRefIdx[e];                                   }
#endif                                                                                                                                               
#if NH_3D_ARP                                                                                                                                         
  Int                         getFirstTRefIdx        ( RefPicList e )                { return  m_aiFirstTRefIdx[e];                                  }
  Void                        setFirstTRefIdx        ( RefPicList e, Int i )         { m_aiFirstTRefIdx[e]    = i;                                   }
  Bool                        getArpRefPicAvailable  ( RefPicList e, Int viewIdx)    { return m_arpRefPicAvailable[e][getVPS()->getLayerIdInNuh(viewIdx, 0)]; }
  IntAry1d                    getPocsInCurrRPSs()                                    { return m_pocsInCurrRPSs;                                      } 
#endif                                                                                                                                               
  Void                        setIsDepth            ( Bool isDepth )                 { m_isDepth = isDepth;                                          }
  Bool                        getIsDepth            () const                         { return m_isDepth;                                             }
  Void                        setCamparaSlice       ( Int** aaiScale = 0, Int** aaiOffset = 0 );      

  IntAry1d                    getCodedScale         ()                               { return m_aaiCodedScale [0];                                   }
  IntAry1d                    getCodedOffset        ()                               { return m_aaiCodedOffset[0];                                   }
  IntAry1d                    getInvCodedScale      ()                               { return m_aaiCodedScale [1];                                   }
  IntAry1d                    getInvCodedOffset     ()                               { return m_aaiCodedOffset[1];                                   }
                                                                                                                                                     
  Void                        setCpScale( Int j, Int  val )                          { m_aaiCodedScale[0][j] = val;                                  }
  Int                         getCpScale( Int j )                                    { return m_aaiCodedScale[0][j];                                 }
                                                                                                                                                     
  Void                        setCpOff( Int j, Int  val )                            { m_aaiCodedOffset[0][j] = val;                                 }
  Int                         getCpOff( Int j )                                      { return m_aaiCodedOffset[0][j];                                }
                                                                                                                                                     
  Void                        setCpInvScale( Int j, Int  val )                       { m_aaiCodedScale[1][j] = val;                                  }
  Int                         getCpInvScale( Int j )                                 { return m_aaiCodedScale[1][j];                                 }
                                                                                                                                                     
  Void                        setCpInvOff( Int j, Int  val )                         { m_aaiCodedOffset[1][j] = val;                                 }
  Int                         getCpInvOff( Int j )                                   { return m_aaiCodedOffset[1][j];                                }
                                                                                                                                                        
  Void                        setIvPicLists( TComPicLists* m_ivPicLists );                                                                              
  Void                        setDepthToDisparityLUTs();                                                                                                
                                                                                                                                                        
  Int*                        getDepthToDisparityB( Int refViewIdx )                 { return m_depthToDisparityB[ getVPS()->getVoiInVps( refViewIdx) ];}
  Int*                        getDepthToDisparityF( Int refViewIdx )                 { return m_depthToDisparityF[ getVPS()->getVoiInVps( refViewIdx) ];}
#if NH_3D_IC                                                                                                                                             
  Void                        setICEnableCandidate( Int* icEnableCandidate)          { m_aICEnableCandidate = icEnableCandidate;                     }
  Void                        setICEnableNum( Int* icEnableNum)                      { m_aICEnableNum = icEnableNum;                                 }
  Void                        setICEnableCandidate( UInt layer, Int value)           { m_aICEnableCandidate[ layer ] = value;                        }
  Void                        setICEnableNum( UInt layer, Int value)                 { m_aICEnableNum[ layer ] = value; ;                            }
                                                                                                                                                     
  Int                         getICEnableCandidate( Int layer)                       { return  m_aICEnableCandidate[ layer ];                        }
  Int                         getICEnableNum( Int layer)                             { return m_aICEnableNum[ layer ];                               }
#endif
#endif
// Additional slice header syntax elements

  Void                        setCrossLayerBlaFlag( Bool flag )                      { m_crossLayerBlaFlag = flag;                                   }
  Bool                        getCrossLayerBlaFlag(  ) const                         { return m_crossLayerBlaFlag;                                   }
  Void                        checkCrossLayerBlaFlag ( ) const ;

#if !H_MV_HLS7_GEN
  Void                        setPocResetFlag( Bool flag )                           { m_pocResetFlag = flag;                                        }
  Bool                        getPocResetFlag(  ) const                              { return m_pocResetFlag;                                        }
#endif

  Void                        setDiscardableFlag( Bool flag )                        { m_discardableFlag = flag;                                     }
  Bool                        getDiscardableFlag(  ) const                           { return m_discardableFlag;                                     }

  Void                        setInterLayerPredEnabledFlag( Bool flag )              { m_interLayerPredEnabledFlag = flag;                           }
  Bool                        getInterLayerPredEnabledFlag(  ) const                 { return m_interLayerPredEnabledFlag;                           }

  Void                        setNumInterLayerRefPicsMinus1( Int  val )              { m_numInterLayerRefPicsMinus1 = val;                           }
  Int                         getNumInterLayerRefPicsMinus1(  ) const                { return m_numInterLayerRefPicsMinus1;                          }

  Void                        setInterLayerPredLayerIdc( Int i, Int  val )           { m_interLayerPredLayerIdc[i] = val;                            }
  Int                         getInterLayerPredLayerIdc( Int i ) const               { return m_interLayerPredLayerIdc[i];                           }

  Void                        setSliceSegmentHeaderExtensionLength( Int  val )       { m_sliceSegmentHeaderExtensionLength = val;                    }
  Int                         getSliceSegmentHeaderExtensionLength(  ) const         { return m_sliceSegmentHeaderExtensionLength;                   }

  Void                        setPocResetIdc( Int  val )                             { m_pocResetIdc = val;                                          }
  Int                         getPocResetIdc(  ) const                               { return m_pocResetIdc;                                         }
  Void                        checkPocResetIdc( ) const                              { assert( !(getVPS()->getPocLsbNotPresentFlag( getLayerIdInVps() ) )  || !(getSlicePicOrderCntLsb() > 0 ) || !( getPocResetIdc() == 2) ); }

  Void                        setPocResetPeriodId( Int  val )                        { m_pocResetPeriodId = val;                                     }
  Int                         getPocResetPeriodId(  ) const                          { return m_pocResetPeriodId;                                    }

  Void                        setFullPocResetFlag( Bool flag )                       { m_fullPocResetFlag = flag;                                    }
  Bool                        getFullPocResetFlag(  ) const                          { return m_fullPocResetFlag;                                    }

  Void                        setPocLsbVal( Int  val )                               { m_pocLsbVal = val;                                            }
  Int                         getPocLsbVal(  )  const                                { return m_pocLsbVal;                                           }
  Void                        checkPocLsbVal( ) const                                { assert( !(getVPS()->getPocLsbNotPresentFlag( getLayerIdInVps() ) )  || !getFullPocResetFlag() || ( getPocLsbVal() == 0 ) ); }

  Void                        setPocMsbValPresentFlag( Bool flag )                   { m_pocMsbValPresentFlag = flag;                                }
  Bool                        getPocMsbValPresentFlag(  )          const             { return m_pocMsbValPresentFlag;                                }

  Void                        setPocMsbVal( Int  val )                               { m_pocMsbVal = val;                                            }
  Int                         getPocMsbVal(  )         const                         { return m_pocMsbVal;                                           }

  Bool                        getCraOrBlaPicFlag()       const                       { return ( getCraPicFlag() || getBlaPicFlag() );                }
  Bool                        getPocMsbValRequiredFlag() const                       { return ( getCraOrBlaPicFlag() && ( getVPS()->getVpsPocLsbAlignedFlag() || getVPS()->getNumDirectRefLayers( getLayerIdInVps() ) == 0 ) );  }

  UInt                        getPocLsbValLen() const                                { return getSPS()->getBitsForPOC();                             }; //log2_max_pic_order_cnt_lsb_minus4 + 4

  Bool getBlaPicFlag() const
  {
    return  getNalUnitType() == NAL_UNIT_CODED_SLICE_BLA_N_LP
    || getNalUnitType() == NAL_UNIT_CODED_SLICE_BLA_W_RADL
    || getNalUnitType() == NAL_UNIT_CODED_SLICE_BLA_W_LP;
  }
  Bool getCraPicFlag () const  
  {
    return getNalUnitType() == NAL_UNIT_CODED_SLICE_CRA;
  }

  // Additional variables derived in slice header semantics 
#if NH_3D
  Int                         getNumInterLayerRefPicsMinus1Len( ) const              { return gCeilLog2(  getVPS()->getNumRefListLayers( getLayerId() )); }
  Int                         getInterLayerPredLayerIdcLen    ( ) const              { return gCeilLog2(  getVPS()->getNumRefListLayers( getLayerId() )); }
#else
  Int                         getNumInterLayerRefPicsMinus1Len( ) const              { return gCeilLog2(  getVPS()->getNumDirectRefLayers( getLayerId() )); }
  Int                         getInterLayerPredLayerIdcLen    ( ) const              { return gCeilLog2(  getVPS()->getNumDirectRefLayers( getLayerId() )); }
#endif

  Int                         getRefLayerPicFlag( Int i ) const;
  Int                         getRefLayerPicIdc ( Int j ) const;
  Int                         getNumRefLayerPics( ) const;

  Int                         getNumActiveRefLayerPics( ) const;

  Int                         getNumActiveRefLayerPics0( ) const                     { return (Int) m_refPicSetInterLayer0->size();                  };
  Int                         getNumActiveRefLayerPics1( ) const                     { return (Int) m_refPicSetInterLayer1->size();                  };

  Int                         getRefPicLayerId             ( Int i ) const;

  Void                        setRefPicSetInterLayer       ( std::vector<TComPic*>* refPicSetInterLayer0, std::vector<TComPic*>* refPicSetInterLayer1);
  TComPic*                    getPicFromRefPicSetInterLayer ( Int setIdc, Int layerId ) const ;

#if NH_3D
  // 3D-HEVC tool parameters
  Void                        deriveInCmpPredAndCpAvailFlag( );
  Void                        init3dToolParameters();
  Void                        checkInCompPredRefLayers();;

  Bool                        getIvMvPredFlag           ( )                          { return m_ivMvPredFlag           ;                             };
  Bool                        getIvMvScalingFlag        ( )                          { return m_ivMvScalingFlag        ;                             };
  Bool                        getIvResPredFlag          ( )                          { return m_ivResPredFlag          ;                             };
  Bool                        getDepthRefinementFlag    ( )                          { return m_depthRefinementFlag    ;                             };
  Bool                        getViewSynthesisPredFlag  ( )                          { return m_viewSynthesisPredFlag  ;                             };
  Bool                        getDepthBasedBlkPartFlag  ( )                          { return m_depthBasedBlkPartFlag  ;                             };
  Bool                        getMpiFlag                ( )                          { return m_mpiFlag                ;                             };
  Bool                        getIntraContourFlag       ( )                          { return m_intraContourFlag       ;                             };
  Bool                        getIntraSdcWedgeFlag      ( )                          { return m_intraSdcWedgeFlag      ;                             };
  Bool                        getQtPredFlag             ( )                          { return m_qtPredFlag             ;                             };
  Bool                        getInterSdcFlag           ( )                          { return m_interSdcFlag           ;                             };
  Bool                        getDepthIntraSkipFlag     ( )                          { return m_depthIntraSkipFlag     ;                             };

  Int                         getMpiSubPbSize           ( )                          { return m_mpiSubPbSize           ;                             };
  Int                         getSubPbSize              ( )                          { return m_subPbSize              ;                             };
#if NH_3D_NBDV
  Int                         getDefaultRefViewIdx()                                 { return m_iDefaultRefViewIdx;                                  }
  Void                        setDefaultRefViewIdx(Int iViewIdx)                     { m_iDefaultRefViewIdx = iViewIdx;                              }

  Bool                        getDefaultRefViewIdxAvailableFlag()                    { return m_bDefaultRefViewIdxAvailableFlag;                     }
  Void                        setDefaultRefViewIdxAvailableFlag(Bool bViewIdx)       { m_bDefaultRefViewIdxAvailableFlag = bViewIdx;                 }
  Void                        setDefaultRefView( );
#endif
#endif
  // Inference 
  Bool                        inferPocMsbValPresentFlag();
#endif
#if H_3D
  Int                         getDefaultRefViewIdx()                                 { return m_iDefaultRefViewIdx;                                  }
  Void                        setDefaultRefViewIdx(Int iViewIdx)                     { m_iDefaultRefViewIdx = iViewIdx;                              }

  Bool                        getDefaultRefViewIdxAvailableFlag()                    { return m_bDefaultRefViewIdxAvailableFlag;                     }
  Void                        setDefaultRefViewIdxAvailableFlag(Bool bViewIdx)       { m_bDefaultRefViewIdxAvailableFlag = bViewIdx;                 }
  Void                        setDefaultRefView( );
#endif
protected:
  TComPic*                    xGetRefPic        (TComList<TComPic*>& rcListPic, Int poc);
  TComPic*                    xGetLongTermRefPic(TComList<TComPic*>& rcListPic, Int poc, Bool pocHasMsb);
#if NH_MV
  TComPic*                    xGetInterLayerRefPic( std::vector<TComPic*>& rcListIlPic, Int layerId );  
#endif
};// END CLASS DEFINITION TComSlice


Void calculateParameterSetChangedFlag(Bool &bChanged, const std::vector<UChar> *pOldData, const std::vector<UChar> &newData);

template <class T> class ParameterSetMap
{
public:
  template <class Tm>
  struct MapData
  {
    Bool                  bChanged;
    std::vector<UChar>   *pNaluData; // Can be null
    Tm*                   parameterSet;
  };

  ParameterSetMap(Int maxId)
  :m_maxId (maxId)
  {}

  ~ParameterSetMap()
  {
    for (typename std::map<Int,MapData<T> >::iterator i = m_paramsetMap.begin(); i!= m_paramsetMap.end(); i++)
    {
      delete (*i).second.pNaluData;
      delete (*i).second.parameterSet;
    }
  }

  Void storePS(Int psId, T *ps, const std::vector<UChar> &naluData)
  {
    assert ( psId < m_maxId );
    if ( m_paramsetMap.find(psId) != m_paramsetMap.end() )
    {
      MapData<T> &mapData=m_paramsetMap[psId];

      // work out changed flag
      calculateParameterSetChangedFlag(mapData.bChanged, mapData.pNaluData, naluData);
      delete m_paramsetMap[psId].pNaluData;
      delete m_paramsetMap[psId].parameterSet;

      m_paramsetMap[psId].parameterSet = ps;
    }
    else
    {
      m_paramsetMap[psId].parameterSet = ps;
      m_paramsetMap[psId].bChanged = false;
    }
      m_paramsetMap[psId].pNaluData=new std::vector<UChar>;
    *(m_paramsetMap[psId].pNaluData) = naluData;
  }

  Void clearChangedFlag(Int psId)
  {
    if ( m_paramsetMap.find(psId) != m_paramsetMap.end() )
    {
      m_paramsetMap[psId].bChanged=false;
    }
  }

  Bool getChangedFlag(Int psId) const
  {
    const typename std::map<Int,MapData<T> >::const_iterator constit=m_paramsetMap.find(psId);
    if ( constit != m_paramsetMap.end() )
    {
      return constit->second.bChanged;
    }
    return false;
  }

  T* getPS(Int psId)
  {
    typename std::map<Int,MapData<T> >::iterator it=m_paramsetMap.find(psId);
    return ( it == m_paramsetMap.end() ) ? NULL : (it)->second.parameterSet;
  }

  const T* getPS(Int psId) const
  {
    typename std::map<Int,MapData<T> >::const_iterator it=m_paramsetMap.find(psId);
    return ( it == m_paramsetMap.end() ) ? NULL : (it)->second.parameterSet;
  }

  T* getFirstPS()
  {
    return (m_paramsetMap.begin() == m_paramsetMap.end() ) ? NULL : m_paramsetMap.begin()->second.parameterSet;
  }

private:
  std::map<Int,MapData<T> > m_paramsetMap;
  Int                       m_maxId;
};

class ParameterSetManager
{
public:
                 ParameterSetManager();
  virtual        ~ParameterSetManager();

  //! store sequence parameter set and take ownership of it
  Void           storeVPS(TComVPS *vps, const std::vector<UChar> &naluData) { m_vpsMap.storePS( vps->getVPSId(), vps, naluData); };
  //! get pointer to existing video parameter set
  TComVPS*       getVPS(Int vpsId)                                           { return m_vpsMap.getPS(vpsId); };
  Bool           getVPSChangedFlag(Int vpsId) const                          { return m_vpsMap.getChangedFlag(vpsId); }
  Void           clearVPSChangedFlag(Int vpsId)                              { m_vpsMap.clearChangedFlag(vpsId); }
  TComVPS*       getFirstVPS()                                               { return m_vpsMap.getFirstPS(); };

  //! store sequence parameter set and take ownership of it
  Void           storeSPS(TComSPS *sps, const std::vector<UChar> &naluData) { m_spsMap.storePS( sps->getSPSId(), sps, naluData); };
  //! get pointer to existing sequence parameter set
  TComSPS*       getSPS(Int spsId)                                           { return m_spsMap.getPS(spsId); };
  Bool           getSPSChangedFlag(Int spsId) const                          { return m_spsMap.getChangedFlag(spsId); }
  Void           clearSPSChangedFlag(Int spsId)                              { m_spsMap.clearChangedFlag(spsId); }
  TComSPS*       getFirstSPS()                                               { return m_spsMap.getFirstPS(); };

  //! store picture parameter set and take ownership of it
  Void           storePPS(TComPPS *pps, const std::vector<UChar> &naluData) { m_ppsMap.storePS( pps->getPPSId(), pps, naluData); };
  //! get pointer to existing picture parameter set
  TComPPS*       getPPS(Int ppsId)                                           { return m_ppsMap.getPS(ppsId); };
  Bool           getPPSChangedFlag(Int ppsId) const                          { return m_ppsMap.getChangedFlag(ppsId); }
  Void           clearPPSChangedFlag(Int ppsId)                              { m_ppsMap.clearChangedFlag(ppsId); }
  TComPPS*       getFirstPPS()                                               { return m_ppsMap.getFirstPS(); };

  //! activate a SPS from a active parameter sets SEI message
  //! \returns true, if activation is successful
#if NH_MV
  // Bool           activateSPSWithSEI(Int SPSId, Int layerId );
#else
  // Bool           activateSPSWithSEI(Int SPSId);
#endif

  //! activate a PPS and depending on isIDR parameter also SPS and VPS
  //! \returns true, if activation is successful
#if NH_MV
  Bool           activatePPS(Int ppsId, Bool isIRAP, Int layerId );
#else
  Bool           activatePPS(Int ppsId, Bool isIRAP);
#endif

  const TComVPS* getActiveVPS()const { return m_vpsMap.getPS(m_activeVPSId); };

#if NH_MV  
  const TComSPS* getActiveSPS( Int layer )const { return m_spsMap.getPS(m_activeSPSId[layer]); };
#else  
  const TComSPS* getActiveSPS()const { return m_spsMap.getPS(m_activeSPSId); };
#endif

protected:
  ParameterSetMap<TComVPS> m_vpsMap;
  ParameterSetMap<TComSPS> m_spsMap;
  ParameterSetMap<TComPPS> m_ppsMap;

  Int m_activeVPSId; // -1 for nothing active
#if NH_MV  
  Int m_activeSPSId[ MAX_NUM_LAYERS ]; // -1 for nothing active
#else  
  Int m_activeSPSId; // -1 for nothing active
#endif
};

//! \}

#endif // __TCOMSLICE__
