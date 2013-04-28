

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

  TComSPSAccess               m_cSPSAccess;
  TComAUPicAccess             m_cAUPicAccess;
  
  TRenTop                     m_cRendererTop; 
#if GERHARD_RM_DEBUG_MM
  TRenModel                   m_cMMCheckModel; 
#endif
  TRenModel                   m_cRendererModel;   

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
  Void  xSetBaseLUT   ( Int iViewIdxSource, Int iViewIdxTarget, TComMVDRefData* pcRefInfo, InterViewReference eView );
  Void  xSetBasePicYuv( Int iViewIdx, Int iPoc, TComMVDRefData* pcRefInfo, InterViewReference eView, bool bDepth ); ///< store pics from buffers in pcRefInfo

  
public:
  TAppEncTop();
  virtual ~TAppEncTop();
  
  Void        encode      ();                               ///< main encoding function
  Void                  setupRenModel    ( Int iPoc, Int iEncViewIdx, Int iEncContent );
  Void                  setMVDPic        ( Int iViewIdx, Int iPoc, TComMVDRefData* pcReferenceInfo ); // set MultiView References
  Void                  getUsedPelsMap   ( Int iViewIdx, Int iPoc, TComPicYuv* pcPicYuvUsedPelsMap );
  std::vector<TComPic*> getSpatialRefPics( Int iViewIdx, Int iPoc, Bool bIsDepthCoder );              // only for mvc functionality yet 
  TComPic*              getPicFromView   ( Int iViewIdx, Int iPoc, Bool bDepth ) { return xGetPicFromView( iViewIdx, iPoc, bDepth ); }

#if GERHARD_RM_DEBUG_MM
  TRenModel* getMMCheckModel() { return  &m_cMMCheckModel  ; };   
#endif
  TRenModel* getRenModel    () { return  &m_cRendererModel ; }; 

  TComSPSAccess*    getSPSAccess  () { return &m_cSPSAccess;   }
  TComAUPicAccess*  getAUPicAccess() { return &m_cAUPicAccess; }

//GT VSO
private:
  std::vector<TVideoIOYuv*>		               m_acTVideoIOYuvERFileList;
  std::map< Int, std::vector< TComPicYuv*> > m_cMapPicExtRefView;             /// Buffer for external Reference files

  Int                                        m_iLastFramePutInERViewBuffer;   ///< Poc of last frame put to ERView Buffer
  Void  xSetERPicYuvs(  Int iViewIdx, Int iPoc, TComMVDRefData* pcReferenceInfo );    ///< store pic from buffer in pcReferenceInfo    
  Void  xStoreVSORefPicsInBuffer();                                                   ///< read in External Ref pic from file and store in buffer
//GT VSO end

  
};// END CLASS DEFINITION TAppEncTop

#endif // __TAPPENCTOP__

