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



/** \file     TAppEncTop.h
    \brief    Encoder application class (header)
*/

#ifndef __TAPPENCTOP__
#define __TAPPENCTOP__

#include "../../Lib/TLibEncoder/TEncTop.h"
#include "../../Lib/TLibVideoIO/TVideoIOYuv.h"
#include "../../Lib/TLibVideoIO/TVideoIOBits.h"
#include "../../Lib/TLibCommon/TComBitStream.h"
#include "../../Lib/TLibCommon/TComDepthMapGenerator.h"
#include "TAppEncCfg.h"
//GT VSO
#include "../../Lib/TLibRenderer/TRenTop.h"
//GT VSO end

#if POZNAN_MP
#include "../../Lib/TLibCommon/TComMP.h"
#endif	

// ====================================================================================================================
// Class definition
// ====================================================================================================================

/// encoder application class
class TAppEncTop : public TAppEncCfg
{
private:
  // class interface

  std::vector<TEncTop*>				 m_acTEncTopList ;
  std::vector<TEncTop*>        m_acTEncDepthTopList ;
  std::vector<TVideoIOYuv*>		 m_acTVideoIOYuvInputFileList;
  std::vector<TVideoIOYuv*>    m_acTVideoIOYuvDepthInputFileList;
  std::vector<TVideoIOYuv*>		 m_acTVideoIOYuvReconFileList;
  std::vector<TVideoIOYuv*>    m_acTVideoIOYuvDepthReconFileList;

  TVideoIOBitsStartCode      m_cTVideoIOBitsFile;           ///< output bitstream file
  
  std::vector< TComList<TComPicYuv*>* >  m_cListPicYuvRecList;              ///< list of reconstruction YUV files
  std::vector< TComList<TComPicYuv*>* >  m_cListPicYuvDepthRecList;         ///< list of reconstruction YUV files
  std::vector<PicOrderCnt> m_aiNextPocToDump;
  std::vector<PicOrderCnt> m_aiNextDepthPocToDump;
  std::vector< std::map<PicOrderCnt, TComPicYuv*> > m_cListPicYuvRecMap;
  std::vector< std::map<PicOrderCnt, TComPicYuv*> > m_cListPicYuvDepthRecMap;

  TComList<TComBitstream*>   m_cListBitstream;              ///< list of bitstreams
  
  std::vector<Int>           	m_iFrameRcvdVector;                  ///< number of received frames
  std::vector<Int>            m_iDepthFrameRcvdVector;             ///< number of received frames

#if DEPTH_MAP_GENERATION
  TComSPSAccess               m_cSPSAccess;
  TComAUPicAccess             m_cAUPicAccess;
#endif
  
#if HHI_VSO
  TRenTop                     m_cRendererTop; 
  TRenModel                   m_cRendererModel;   
#endif

#if POZNAN_CU_SKIP||POZNAN_CU_SYNTH
  TRenTop                     m_cAvailabilityRenderer;
#endif

#if POZNAN_MP
  TComMP*				  m_pcMP;
#endif

#if POZNAN_STAT_JK
  std::vector<FILE*>       m_cStatFileList;					    ///< texure statistics file handles
  std::vector<FILE*>       m_cDepthStatFileList;				///< depth statistics file handles
#endif

protected:
  // initialization
  Void  xCreateLib        ();                               ///< create files & encoder class
  Void  xInitLibCfg       ();                               ///< initialize internal variables
  Void  xInitLib          ();                               ///< initialize encoder class
  Void  xDestroyLib       ();                               ///< destroy encoder class
  
  /// obtain required buffers
  Void  xGetBuffer        ( TComPicYuv*& rpcPicYuvRec,			//GT: unused?
                           TComBitstream*& rpcBitStream );
  
  Void  xGetBuffer        ( TComPicYuv*& rpcPicYuvRec, Int iViewIdx, std::vector< TComList<TComPicYuv*>*>& racBuffer  );

  /// delete allocated buffers
  Void  xDeleteBuffer     ();
  
  // file I/O
  Void  xWriteOutput      ( Int iViewIdx, Bool isDepth = false );              ///< write bitstream to file


  // util  
  TComPic*    xGetPicFromView   ( Int iViewIdx, Int iPoc, Bool bDepth );
  TComPicYuv* xGetPicYuvFromView( Int iViewIdx, Int iPoc, Bool bDepth, Bool bRecon );


  // Ref Data
#if HHI_VSO
  Void  xSetBaseLUT   ( Int iViewIdxSource, Int iViewIdxTarget, TComMVDRefData* pcRefInfo, InterViewReference eView );
  Void  xSetBasePicYuv( Int iViewIdx, Int iPoc, TComMVDRefData* pcRefInfo, InterViewReference eView, bool bDepth ); ///< store pics from buffers in pcRefInfo
#endif
  
public:
  TAppEncTop();
  virtual ~TAppEncTop();
  
  Void        encode      ();                               ///< main encoding function
#if HHI_VSO
  Void                  setupRenModel    ( Int iPoc, Int iEncViewIdx, Int iEncContent );
  Void                  setMVDPic        ( Int iViewIdx, Int iPoc, TComMVDRefData* pcReferenceInfo ); // set MultiView References
#endif
  Void                  getUsedPelsMap   ( Int iViewIdx, Int iPoc, TComPicYuv* pcPicYuvUsedPelsMap );
  std::vector<TComPic*> getSpatialRefPics( Int iViewIdx, Int iPoc, Bool bIsDepthCoder );              // only for mvc functionality yet 
  TComPic*              getPicFromView   ( Int iViewIdx, Int iPoc, Bool bDepth ) { return xGetPicFromView( iViewIdx, iPoc, bDepth ); }

#if HHI_VSO
  TRenModel* getRenModel    () { return  &m_cRendererModel ; }; 
#endif

#if DEPTH_MAP_GENERATION
  TComSPSAccess*    getSPSAccess  () { return &m_cSPSAccess;   }
  TComAUPicAccess*  getAUPicAccess() { return &m_cAUPicAccess; }
#endif

#if POZNAN_MP
  TComMP* getMP() {return m_pcMP;}
#endif

#if HHI_VSO
private:
  std::vector<TVideoIOYuv*>		               m_acTVideoIOYuvERFileList;
  std::map< Int, std::vector< TComPicYuv*> > m_cMapPicExtRefView;             /// Buffer for external Reference files

  Int                                        m_iLastFramePutInERViewBuffer;   ///< Poc of last frame put to ERView Buffer
  Void  xSetERPicYuvs(  Int iViewIdx, Int iPoc, TComMVDRefData* pcReferenceInfo );    ///< store pic from buffer in pcReferenceInfo    
  Void  xStoreVSORefPicsInBuffer();                                                   ///< read in External Ref pic from file and store in buffer
#endif


#if POZNAN_CU_SYNTH
private:
  Void  xStoreSynthPicsInBuffer(Int iCoddedViewIdx, Bool bDepth);
#endif

};// END CLASS DEFINITION TAppEncTop

#endif // __TAPPENCTOP__

