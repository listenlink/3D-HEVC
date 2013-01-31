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



/** \file     TComResidualGenerator.h
    \brief    residual picture generator class (header)
*/


#ifndef __TCOM_RESIDUAL_GENERATOR__
#define __TCOM_RESIDUAL_GENERATOR__


#include "CommonDef.h"
#include "TComPic.h"
#include "TComSlice.h"
#include "TComDepthMapGenerator.h"
#include "TComTrQuant.h"


#if HHI_INTER_VIEW_RESIDUAL_PRED

class TComResidualGenerator
{
  enum { NUM_TMP_YUV_BUFFERS = 3 };

public:
  TComResidualGenerator ();
  ~TComResidualGenerator();

  Void  create                ( Bool bDecoder, UInt uiPicWidth, UInt uiPicHeight, UInt uiMaxCUDepth, UInt uiMaxCUWidth, UInt uiMaxCUHeight, UInt uiOrgBitDepth );
  Void  destroy               ();

  Void  init                  ( TComTrQuant*  pcTrQuant, TComDepthMapGenerator* pcDepthMapGenerator );
  Void  uninit                ();

  Void  initViewComponent     ( TComPic*      pcPic );
  Void  setRecResidualPic     ( TComPic*      pcPic );

#if QC_MULTI_DIS_CAN_A0097
#if MTK_RELEASE_DV_CONSTRAINT
  Bool  getResidualSamples    ( TComDataCU*   pcCU,  UInt uiPUIdx, TComYuv* pcYuv, TComMv iDisp_x
#if QC_SIMPLIFIEDIVRP_M24938
    , Bool bRecon
#endif
);
  Bool  getResidualSamples    ( TComPic* pcPic, UInt uiXPos, UInt uiYPos, UInt uiBlkWidth, UInt uiBlkHeight, TComYuv* pcYuv , TComMv iDisp_x
#if QC_SIMPLIFIEDIVRP_M24938
    , Bool bRecon
#endif  
  );
#else
  Bool  getResidualSamples    ( TComDataCU*   pcCU,  UInt uiPUIdx, TComYuv* pcYuv, Int iDisp 
#if QC_SIMPLIFIEDIVRP_M24938
    , Bool bRecon
#endif
);
  Bool  getResidualSamples    ( TComPic* pcPic, UInt uiXPos, UInt uiYPos, UInt uiBlkWidth, UInt uiBlkHeight, TComYuv* pcYuv , Int iDisp 
#if QC_SIMPLIFIEDIVRP_M24938
    , Bool bRecon
#endif  
  );
#endif
#else
  Bool  getResidualSamples    ( TComDataCU*   pcCU,  UInt uiPUIdx, TComYuv* pcYuv 
#if QC_SIMPLIFIEDIVRP_M24938
    , Bool bRecon
#endif
    );
  Bool  getResidualSamples    ( TComPic* pcPic, UInt uiXPos, UInt uiYPos, UInt uiBlkWidth, UInt uiBlkHeight, TComYuv* pcYuv 
#if QC_SIMPLIFIEDIVRP_M24938
    , Bool bRecon
#endif
    );
#endif

private:
  Void  xSetRecResidualPic    ( TComPic*      pcPic );
  Void  xSetRecResidualCU     ( TComDataCU*   pcCU,  UInt     uiDepth,      UInt uiAbsPartIdx );
  Void  xSetRecResidualIntraCU( TComDataCU*   pcCU,  TComYuv* pcCUResidual );
  Void  xSetRecResidualInterCU( TComDataCU*   pcCU,  TComYuv* pcCUResidual );
  Void  xClearIntViewResidual ( TComDataCU*   pcCU,  TComYuv* pcCUResidual, UInt uiPartIdx    );
  Void  xClearResidual        (                      TComYuv* pcCUResidual, UInt uiAbsPartIdx, UInt uiWidth, UInt uiHeight );
#if QC_MULTI_DIS_CAN_A0097
#if MTK_RELEASE_DV_CONSTRAINT
  Void  xSetPredResidualBlock ( TComPic*      pcPic, UInt uiBaseViewId, UInt uiXPos, UInt uiYPos, UInt uiBlkWidth, UInt uiBlkHeight, TComYuv* pcYuv, TComMv iDisp
#if QC_SIMPLIFIEDIVRP_M24938
    , UInt * puiXPosInRefView , UInt * puiYPosInRefView , Bool bRecon
#endif
  );
#else
  Void  xSetPredResidualBlock ( TComPic*      pcPic, UInt uiBaseViewId, UInt uiXPos, UInt uiYPos, UInt uiBlkWidth, UInt uiBlkHeight, TComYuv* pcYuv, Int iDisp 
#if QC_SIMPLIFIEDIVRP_M24938
    , UInt * puiXPosInRefView , UInt * puiYPosInRefView , Bool bRecon
#endif
  );
#endif
#else
  Void  xSetPredResidualBlock ( TComPic*      pcPic, UInt uiBaseViewId, UInt uiXPos, UInt uiYPos, UInt uiBlkWidth, UInt uiBlkHeight, TComYuv* pcYuv 
#if QC_SIMPLIFIEDIVRP_M24938
    , UInt * puiXPosInRefView , UInt * puiYPosInRefView , Bool bRecon
#endif
    );
#endif
  Bool  xIsNonZero            ( TComYuv*      pcYuv, UInt uiBlkWidth, UInt uiBlkHeight );
#if QC_SIMPLIFIEDIVRP_M24938
  Bool  xIsNonZeroByCBF       ( UInt uiBaseViewId , UInt uiXPos , UInt uiYPos, UInt uiBlkWidth , UInt uiBlkHeight );
#endif

  Void  xDumpResidual         ( TComPic*      pcPic, char* pFilenameBase );

private:
  // general parameters
  Bool                    m_bCreated;
  Bool                    m_bInit;
  Bool                    m_bDecoder;
  TComTrQuant*            m_pcTrQuant;
  TComDepthMapGenerator*  m_pcDepthMapGenerator;
  TComSPSAccess*          m_pcSPSAccess;
  TComAUPicAccess*        m_pcAUPicAccess;
  UInt                    m_uiMaxDepth;
  UInt                    m_uiOrgDepthBitDepth;
  TComYuv**               m_ppcYuvTmp;
  TComYuv**               m_ppcYuv;
  TComDataCU**            m_ppcCU;
  TComPicYuv              m_cTmpPic;
};


#endif // __TCOM_RESIDUAL_GENERATOR__

#endif // HHI_INTER_VIEW_RESIDUAL_PRED




