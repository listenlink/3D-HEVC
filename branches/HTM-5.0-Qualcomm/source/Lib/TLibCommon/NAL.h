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

#pragma once

#include <vector>
#include <sstream>
#include "CommonDef.h"

class TComOutputBitstream;

/**
 * Represents a single NALunit header and the associated RBSPayload
 */
struct NALUnit
{
  NalUnitType m_nalUnitType; ///< nal_unit_type
#if NAL_REF_FLAG
  Bool        m_nalRefFlag;  ///< nal_ref_flag
#else
  NalRefIdc   m_nalRefIDC;   ///< nal_ref_idc
#endif
#if VIDYO_VPS_INTEGRATION|QC_MVHEVC_B0046
  unsigned    m_layerId;
  unsigned    m_temporalId;  ///< temporal_id
#else
  Int         m_viewId;      ///< view_id
  Bool        m_isDepth;     ///< is_depth
  unsigned    m_temporalId;  ///< temporal_id
#if !H0388
  bool        m_OutputFlag;  ///< output_flag
#endif
#endif

  /** construct an NALunit structure with given header values. */
#if H0388
#if NAL_REF_FLAG
  NALUnit(
    NalUnitType nalUnitType,
    Bool        nalRefFlag,
#if !VIDYO_VPS_INTEGRATION & !QC_MVHEVC_B0046    
    Int         viewId,
    Bool        isDepth,
#else
    unsigned    layerId,
#endif
    unsigned       temporalId = 0)
    :m_nalUnitType (nalUnitType)
    ,m_nalRefFlag  (nalRefFlag)
#if !VIDYO_VPS_INTEGRATION & !QC_MVHEVC_B0046
    ,m_viewId      (viewId)
    ,m_isDepth     (isDepth)
#else
    ,m_layerId     (layerId)
#endif
    ,m_temporalId  (temporalId)
  {}
#else
  NALUnit(
    NalUnitType  nalUnitType,
    NalRefIdc    nalRefIDC,
    Int          viewId,
    Bool         isDepth,
    unsigned temporalID = 0)
  {
    m_nalUnitType = nalUnitType;
    m_nalRefIDC   = nalRefIDC;
#if !VIDYO_VPS_INTEGRATION
    m_viewId      = viewId;
    m_isDepth     = isDepth;
#else
    m_layerId = layerId;
#endif
    m_temporalId  = temporalID;
  }
#endif
#else
  NALUnit(
    NalUnitType  nalUnitType,
    NalRefIdc    nalRefIDC,
#if !VIDYO_VPS_INTEGRATION    
    Int          viewId,
    Bool         isDepth,
#else
    unsigned         layerId,
#endif
    unsigned     temporalID = 0,
    bool         outputFlag = true)
  {
    m_nalUnitType = nalUnitType;
    m_nalRefIDC   = nalRefIDC;
#if !VIDYO_VPS_INTEGRATION
    m_viewId      = viewId;
    m_isDepth     = isDepth;
#else
    m_layerId = layerId;
#endif
    m_temporalId  = temporalID;
    m_OutputFlag  = outputFlag;
  }
#endif

  /** default constructor - no initialization; must be perfomed by user */
  NALUnit() {}

  /** returns true if the NALunit is a slice NALunit */
  bool isSlice()
  {
    return m_nalUnitType == NAL_UNIT_CODED_SLICE_IDR
#if H0566_TLA
#if !QC_REM_IDV_B0046    
        || m_nalUnitType == NAL_UNIT_CODED_SLICE_IDV
#endif
        || m_nalUnitType == NAL_UNIT_CODED_SLICE_CRA
        || m_nalUnitType == NAL_UNIT_CODED_SLICE_TLA
#else
        || m_nalUnitType == NAL_UNIT_CODED_SLICE_CDR
#endif
        || m_nalUnitType == NAL_UNIT_CODED_SLICE;
  }
};

struct OutputNALUnit;

/**
 * A single NALunit, with complete payload in EBSP format.
 */
struct NALUnitEBSP : public NALUnit
{
  std::ostringstream m_nalUnitData;

  /**
   * convert the OutputNALUnit #nalu# into EBSP format by writing out
   * the NALUnit header, then the rbsp_bytes including any
   * emulation_prevention_three_byte symbols.
   */
  NALUnitEBSP(OutputNALUnit& nalu);
};
//! \}
//! \}
