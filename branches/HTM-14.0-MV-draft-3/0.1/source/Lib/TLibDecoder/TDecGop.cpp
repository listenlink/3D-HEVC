

/** \file     TDecGop.cpp
    \brief    GOP decoder class
*/

extern bool g_md5_mismatch; ///< top level flag to signal when there is a decode problem

#include "TDecGop.h"
#include "TDecCAVLC.h"
#include "TDecSbac.h"
#include "TDecBinCoder.h"
#include "TDecBinCoderCABAC.h"
#include "../libmd5/MD5.h"
#include "../TLibCommon/SEI.h"

#include <time.h>

static void calcAndPrintMD5Status(TComPicYuv& pic, const SEImessages* seis);

// ====================================================================================================================
// Constructor / destructor / initialization / destroy
// ====================================================================================================================

TDecGop::TDecGop()
{
  m_iGopSize = 0;
  m_dDecTime = 0;
}

TDecGop::~TDecGop()
{
  
}

Void TDecGop::create()
{
  
}


Void TDecGop::destroy()
{

}

Void TDecGop::init( TDecEntropy*            pcEntropyDecoder, 
                   TDecSbac*               pcSbacDecoder, 
                   TDecBinCABAC*           pcBinCABAC,
                   TDecCavlc*              pcCavlcDecoder, 
                   TDecSlice*              pcSliceDecoder, 
                   TComLoopFilter*         pcLoopFilter, 
                   TComAdaptiveLoopFilter* pcAdaptiveLoopFilter, 
#if MTK_SAO
                   TComSampleAdaptiveOffset* pcSAO,
#endif              
                   TComDepthMapGenerator*  pcDepthMapGenerator,
                   TComResidualGenerator*  pcResidualGenerator )
{
  m_pcEntropyDecoder      = pcEntropyDecoder;
  m_pcSbacDecoder         = pcSbacDecoder;
  m_pcBinCABAC            = pcBinCABAC;
  m_pcCavlcDecoder        = pcCavlcDecoder;
  m_pcSliceDecoder        = pcSliceDecoder;
  m_pcLoopFilter          = pcLoopFilter;
  m_pcAdaptiveLoopFilter  = pcAdaptiveLoopFilter;
#if MTK_SAO
  m_pcSAO  = pcSAO;
#endif
  m_pcDepthMapGenerator   = pcDepthMapGenerator;
  m_pcResidualGenerator   = pcResidualGenerator;
}

// ====================================================================================================================
// Public member functions
// ====================================================================================================================

Void TDecGop::decompressGop (Bool bEos, TComBitstream* pcBitstream, TComPic*& rpcPic, Bool bExecuteDeblockAndAlf)
{
  TComSlice*  pcSlice = rpcPic->getSlice(rpcPic->getCurrSliceIdx());

  //-- For time output for each slice
  long iBeforeTime = clock();
  
  UInt uiStartCUAddr   = pcSlice->getEntropySliceCurStartCUAddr();
#if MTK_NONCROSS_INLOOP_FILTER
  static Bool  bFirst = true;
  static UInt  uiILSliceCount;
  static UInt* puiILSliceStartLCU;
  if (!bExecuteDeblockAndAlf)
  {
    if(bFirst)
    {
      uiILSliceCount = 0;
      if(!pcSlice->getSPS()->getLFCrossSliceBoundaryFlag())
      {
        puiILSliceStartLCU = new UInt[rpcPic->getNumCUsInFrame() +1];
      }
      bFirst = false;
    }
    
    if(!pcSlice->getSPS()->getLFCrossSliceBoundaryFlag())
    {
      UInt uiSliceStartCuAddr = pcSlice->getSliceCurStartCUAddr();
      if(uiSliceStartCuAddr == uiStartCUAddr)
      {
        puiILSliceStartLCU[uiILSliceCount] = uiSliceStartCuAddr;
        uiILSliceCount++;
      }
    }
#endif //MTK_NONCROSS_INLOOP_FILTER

    UInt iSymbolMode = pcSlice->getSymbolMode();
    if (iSymbolMode)
    {
      m_pcSbacDecoder->init( (TDecBinIf*)m_pcBinCABAC );
      m_pcEntropyDecoder->setEntropyDecoder (m_pcSbacDecoder);
    }
    else
    {
      m_pcEntropyDecoder->setEntropyDecoder (m_pcCavlcDecoder);
    }
    
    m_pcEntropyDecoder->setBitstream      (pcBitstream);
    m_pcEntropyDecoder->resetEntropy      (pcSlice);
    
    if (uiStartCUAddr==0)  // decode ALF params only from first slice header
    {
#if MTK_SAO
      if( rpcPic->getSlice(0)->getSPS()->getUseSAO() )
      {  
        m_pcSAO->InitSao(&m_cSaoParam);
        m_pcEntropyDecoder->decodeSaoParam(&m_cSaoParam);
      }
#endif

      if ( rpcPic->getSlice(0)->getSPS()->getUseALF() )
      {
#if TSB_ALF_HEADER
        m_pcAdaptiveLoopFilter->setNumCUsInFrame(rpcPic);
#endif
        m_pcAdaptiveLoopFilter->allocALFParam(&m_cAlfParam);
        m_pcEntropyDecoder->decodeAlfParam( &m_cAlfParam );
      }
    }
    
    // init view component and predict virtual depth map
    if( uiStartCUAddr == 0 )
    {
      m_pcDepthMapGenerator->initViewComponent( rpcPic );
      m_pcDepthMapGenerator->predictDepthMap  ( rpcPic );
      m_pcResidualGenerator->initViewComponent( rpcPic );
    }

    // decode slice
    m_pcSliceDecoder->decompressSlice(pcBitstream, rpcPic);
    
    m_dDecTime += (double)(clock()-iBeforeTime) / CLOCKS_PER_SEC;
  }
  else
  {
    // set residual picture
    m_pcResidualGenerator->setRecResidualPic( rpcPic );

    // update virtual depth map
    m_pcDepthMapGenerator->updateDepthMap( rpcPic );

    // deblocking filter
    m_pcLoopFilter->setCfg(pcSlice->getLoopFilterDisable(), 0, 0);
    m_pcLoopFilter->loopFilterPic( rpcPic );
#if MTK_SAO
    {
      if( rpcPic->getSlice(0)->getSPS()->getUseSAO())
      {
        m_pcSAO->SAOProcess(rpcPic, &m_cSaoParam);
      }
    }
#endif
    // adaptive loop filter
    if( pcSlice->getSPS()->getUseALF() )
    {
#if MTK_NONCROSS_INLOOP_FILTER  
      if(pcSlice->getSPS()->getLFCrossSliceBoundaryFlag())
      {
        m_pcAdaptiveLoopFilter->setUseNonCrossAlf(false);
      }
      else
      {
        puiILSliceStartLCU[uiILSliceCount] = rpcPic->getNumCUsInFrame();
        m_pcAdaptiveLoopFilter->setUseNonCrossAlf( (uiILSliceCount > 1) );
        if(m_pcAdaptiveLoopFilter->getUseNonCrossAlf())
        {
          m_pcAdaptiveLoopFilter->setNumSlicesInPic( uiILSliceCount );
          m_pcAdaptiveLoopFilter->createSlice();
          for(UInt i=0; i< uiILSliceCount ; i++)
          {
            (*m_pcAdaptiveLoopFilter)[i].create(rpcPic, i, puiILSliceStartLCU[i], puiILSliceStartLCU[i+1]-1);
          }
        }
      }
#endif
      m_pcAdaptiveLoopFilter->ALFProcess(rpcPic, &m_cAlfParam);
#if MTK_NONCROSS_INLOOP_FILTER
      if(m_pcAdaptiveLoopFilter->getUseNonCrossAlf())
      {
        m_pcAdaptiveLoopFilter->destroySlice();
      }
#endif
      m_pcAdaptiveLoopFilter->freeALFParam(&m_cAlfParam);
    }
    
/*#if AMVP_BUFFERCOMPRESS
    rpcPic->compressMotion(); // move to end of access unit
#endif */
    
    //-- For time output for each slice
    if(rpcPic->getViewIdx()!=-1)
    {
      if( !pcSlice->getSPS()->isDepth())
      {
        printf("\nView \t\t%4d\t POC %4d ( %c-SLICE, QP%3d ) ",
                        rpcPic->getViewIdx(),
                        pcSlice->getPOC(),
                        pcSlice->isIntra() ? 'I' : pcSlice->isInterP() ? 'P' : 'B',
                        pcSlice->getSliceQp() );
      }
      else
      {
        printf("\nDepth View \t%4d\t POC %4d ( %c-SLICE, QP%3d ) ",
                              rpcPic->getViewIdx(),
                              pcSlice->getPOC(),
                              pcSlice->isIntra() ? 'I' : pcSlice->isInterP() ? 'P' : 'B',
                              pcSlice->getSliceQp() );
      }
    }
    else
      printf("\nPOC %4d ( %c-SLICE, QP%3d ) ",
             pcSlice->getPOC(),
             pcSlice->isIntra() ? 'I' : pcSlice->isInterP() ? 'P' : 'B',
             pcSlice->getSliceQp() );
    
    m_dDecTime += (double)(clock()-iBeforeTime) / CLOCKS_PER_SEC;
    printf ("[DT %6.3f] ", m_dDecTime );
    m_dDecTime  = 0;
    
    for (Int iRefList = 0; iRefList < 2; iRefList++)
    {
      printf ("[L%d ", iRefList);
      for (Int iRefIndex = 0; iRefIndex < pcSlice->getNumRefIdx(RefPicList(iRefList)); iRefIndex++)
      {
        if( pcSlice->getViewIdx() != pcSlice->getRefViewIdx( RefPicList(iRefList), iRefIndex ) )
        {
          printf( "V%d", pcSlice->getRefViewIdx( RefPicList(iRefList), iRefIndex ) );
          if( pcSlice->getPOC() != pcSlice->getRefPOC( RefPicList(iRefList), iRefIndex ) )
            printf( "(%d)", pcSlice->getRefPOC( RefPicList(iRefList), iRefIndex ) );
          printf( " " );
        }
        else
          printf ("%d ", pcSlice->getRefPOC(RefPicList(iRefList), iRefIndex));
      }
      printf ("] ");
    }
#if DCM_COMB_LIST
    if(pcSlice->getNumRefIdx(REF_PIC_LIST_C)>0 && !pcSlice->getNoBackPredFlag())
    {
      printf ("[LC ");
      for (Int iRefIndex = 0; iRefIndex < pcSlice->getNumRefIdx(REF_PIC_LIST_C); iRefIndex++)
      {
        if( pcSlice->getViewIdx() != pcSlice->getRefViewIdx( (RefPicList)pcSlice->getListIdFromIdxOfLC(iRefIndex), pcSlice->getRefIdxFromIdxOfLC(iRefIndex) ) )
        {
          printf( "V%d", pcSlice->getRefViewIdx( (RefPicList)pcSlice->getListIdFromIdxOfLC(iRefIndex), pcSlice->getRefIdxFromIdxOfLC(iRefIndex) ) );
          if( pcSlice->getPOC() != pcSlice->getRefPOC( (RefPicList)pcSlice->getListIdFromIdxOfLC(iRefIndex), pcSlice->getRefIdxFromIdxOfLC(iRefIndex) ) )
            printf( "(%d)", pcSlice->getRefPOC( (RefPicList)pcSlice->getListIdFromIdxOfLC(iRefIndex), pcSlice->getRefIdxFromIdxOfLC(iRefIndex) ) );
          printf( " " );
        }
        else
          printf ("%d ", pcSlice->getRefPOC((RefPicList)pcSlice->getListIdFromIdxOfLC(iRefIndex), pcSlice->getRefIdxFromIdxOfLC(iRefIndex)));
      }
      printf ("] ");
    }
#endif
#if FIXED_ROUNDING_FRAME_MEMORY
    rpcPic->getPicYuvRec()->xFixedRoundingPic();
#endif 

    if (m_pictureDigestEnabled) {
      calcAndPrintMD5Status(*rpcPic->getPicYuvRec(), rpcPic->getSEIs());
    }

    rpcPic->setReconMark(true);
#if MTK_NONCROSS_INLOOP_FILTER
    uiILSliceCount = 0;
#endif
  }
}

/**
 * Calculate and print MD5 for @pic, compare to picture_digest SEI if
 * present in @seis.  @seis may be NULL.  MD5 is printed to stdout, in
 * a manner suitable for the status line. Theformat is:
 *  [MD5:xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx,(yyy)]
 * Where, x..x is the md5
 *        yyy has the following meanings:
 *            OK          - calculated MD5 matches the SEI message
 *            ***ERROR*** - calculated MD5 does not match the SEI message
 *            unk         - no SEI message was available for comparison
 */
static void calcAndPrintMD5Status(TComPicYuv& pic, const SEImessages* seis)
{
  /* calculate MD5sum for entire reconstructed picture */
  unsigned char recon_digest[16];
  calcMD5(pic, recon_digest);

  /* compare digest against received version */
  const char* md5_ok = "(unk)";
  bool md5_mismatch = false;

  if (seis && seis->picture_digest)
  {
    md5_ok = "(OK)";
    for (unsigned i = 0; i < 16; i++)
    {
      if (recon_digest[i] != seis->picture_digest->digest[i])
      {
        md5_ok = "(***ERROR***)";
        md5_mismatch = true;
      }
    }
  }

  printf("[MD5:%s,%s] ", digestToString(recon_digest), md5_ok);
  if (md5_mismatch)
  {
    g_md5_mismatch = true;
    printf("[rxMD5:%s] ", digestToString(seis->picture_digest->digest));
  }
}
