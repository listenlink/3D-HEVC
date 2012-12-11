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

/** \file     TAppEncTop.h
    \brief    Encoder application class (header)
*/

#ifndef __TAPPENCTOP__
#define __TAPPENCTOP__

#include <list>
#include <ostream>

#include "TLibEncoder/TEncTop.h"
#include "TLibVideoIO/TVideoIOYuv.h"
#include "TLibCommon/AccessUnit.h"
#include "TAppEncCfg.h"
#include "TLibCommon/TComDepthMapGenerator.h"
#if HHI_VSO || HHI_INTERVIEW_SKIP
#include "../../Lib/TLibRenderer/TRenTop.h"
#endif

//! \ingroup TAppEncoder
//! \{

// ====================================================================================================================
// Class definition
// ====================================================================================================================

/// encoder application class
class TAppEncTop : public TAppEncCfg
{
private:
  // class interface
  std::vector<TEncTop*>      m_acTEncTopList ;
  std::vector<TEncTop*>      m_acTEncDepthTopList ;
  std::vector<TVideoIOYuv*>  m_acTVideoIOYuvInputFileList;  ///< input YUV file
  std::vector<TVideoIOYuv*>  m_acTVideoIOYuvDepthInputFileList;
  std::vector<TVideoIOYuv*>  m_acTVideoIOYuvReconFileList;  ///< output reconstruction file
  std::vector<TVideoIOYuv*>  m_acTVideoIOYuvDepthReconFileList;

  std::vector< TComList<TComPicYuv*>* >  m_picYuvRec;       ///< list of reconstruction YUV files
  std::vector< TComList<TComPicYuv*>* >  m_picYuvDepthRec;         

  std::vector<Int>           m_frameRcvd;                  ///< number of received frames
  std::vector<Int>           m_depthFrameRcvd;   

  unsigned                   m_essentialBytes;
  unsigned                   m_totalBytes;

#if DEPTH_MAP_GENERATION
#if VIDYO_VPS_INTEGRATION
  TComVPSAccess               m_cVPSAccess;
#endif
  TComSPSAccess               m_cSPSAccess;
  TComAUPicAccess             m_cAUPicAccess;
#endif

#if VIDYO_VPS_INTEGRATION|QC_MVHEVC_B0046
  TComVPS                     m_cVPS;
#endif
  
#if HHI_VSO
  TRenTop                     m_cRendererTop; 
  TRenModel                   m_cRendererModel;   
#endif

#if HHI_INTERVIEW_SKIP
  TRenTop  m_cUsedPelsRenderer;                               ///< renderer for used pels map
#endif
protected:
  // initialization
  Void  xCreateLib        ();                               ///< create files & encoder class
  Void  xInitLibCfg       ();                               ///< initialize internal variables
  Void  xInitLib          ();                               ///< initialize encoder class
  Void  xDestroyLib       ();                               ///< destroy encoder class
  
  /// obtain required buffers
  Void xGetBuffer(TComPicYuv*& rpcPicYuvRec, Int iViewIdx, Bool isDepth);
  
  /// delete allocated buffers
  Void  xDeleteBuffer     ();
  
  // file I/O
  Void xWriteOutput(std::ostream& bitstreamFile, Int iNumEncoded, std::list<AccessUnit>& accessUnits, Int iViewIdx, Bool isDepth); ///< write bitstream to file
  void rateStatsAccum(const AccessUnit& au, const std::vector<unsigned>& stats);
  // void printRateSummary();
  
  TComPic* xGetPicFromView( Int viewId, Int iPoc, Bool isDepth );
  TComPicYuv* xGetPicYuvFromView( Int iViewIdx, Int iPoc, Bool bDepth, Bool bRecon );
  
public:
  TAppEncTop();
  virtual ~TAppEncTop();


  Void        encode      ();                               ///< main encoding function
  TEncTop*    getTEncTop( Int viewId, Bool isDepth );   

  std::vector<TComPic*> getInterViewRefPics( Int viewId, Int poc, Bool isDepth, TComSPS* sps );
  TComPic*              getPicFromView     ( Int viewId, Int poc, Bool isDepth ) { return xGetPicFromView( viewId, poc, isDepth ); }
  TComPicYuv*           getPicYuvFromView  ( Int iViewIdx, Int iPoc, bool bDepth, Bool bRecon ) { return xGetPicYuvFromView( iViewIdx, iPoc, bDepth, bRecon ); }

#if HHI_INTERVIEW_SKIP
  Void                  getUsedPelsMap   ( Int iViewIdx, Int iPoc, TComPicYuv* pcPicYuvUsedPelsMap );
#endif
#if HHI_VSO
  Void                  setupRenModel    ( Int iPoc, Int iEncViewIdx, Int iEncContent, Int iHorOffset );
#endif
  
#if QC_MVHEVC_B0046
  TComVPS*          getVPS()  { return &m_cVPS; }
#endif
#if VIDYO_VPS_INTEGRATION
  TComVPS*          getVPS()  { return &m_cVPS; }
  TComVPSAccess*    getVPSAccess  () { return &m_cVPSAccess;   }
#endif
  
#if DEPTH_MAP_GENERATION
  TComSPSAccess*    getSPSAccess  () { return &m_cSPSAccess;   }
  TComAUPicAccess*  getAUPicAccess() { return &m_cAUPicAccess; }
#endif

#if HHI_VSO
  TRenModel* getRenModel    () { return  &m_cRendererModel ; }; 
#endif

#if HHI_VSO
private:
  Void  xStoreVSORefPicsInBuffer();                                                   ///< read in External Ref pic from file and store in buffer
#endif
  
#if RWTH_SDC_DLT_B0036
  Void  xAnalyzeInputBaseDepth(Int iViewIdx, UInt uiNumFrames);
#endif

};// END CLASS DEFINITION TAppEncTop

//! \}

#endif // __TAPPENCTOP__

