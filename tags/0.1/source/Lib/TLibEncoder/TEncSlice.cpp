

/** \file     TEncSlice.cpp
    \brief    slice encoder class
*/

#include "TEncTop.h"
#include "TEncSlice.h"

// ====================================================================================================================
// Constructor / destructor / create / destroy
// ====================================================================================================================

TEncSlice::TEncSlice()
{
  m_apcPicYuvPred = NULL;
  m_apcPicYuvResi = NULL;
  
  m_pdRdPicLambda = NULL;
  m_pdRdPicQp     = NULL;
  m_piRdPicQp     = NULL;
}

TEncSlice::~TEncSlice()
{
}

Void TEncSlice::create( Int iWidth, Int iHeight, UInt iMaxCUWidth, UInt iMaxCUHeight, UChar uhTotalDepth )
{
  // create prediction picture
  if ( m_apcPicYuvPred == NULL )
  {
    m_apcPicYuvPred  = new TComPicYuv;
    m_apcPicYuvPred->create( iWidth, iHeight, iMaxCUWidth, iMaxCUHeight, uhTotalDepth );
  }
  
  // create residual picture
  if( m_apcPicYuvResi == NULL )
  {
    m_apcPicYuvResi  = new TComPicYuv;
    m_apcPicYuvResi->create( iWidth, iHeight, iMaxCUWidth, iMaxCUHeight, uhTotalDepth );
  }
}

Void TEncSlice::destroy()
{
  // destroy prediction picture
  if ( m_apcPicYuvPred )
  {
    m_apcPicYuvPred->destroy();
    delete m_apcPicYuvPred;
    m_apcPicYuvPred  = NULL;
  }
  
  // destroy residual picture
  if ( m_apcPicYuvResi )
  {
    m_apcPicYuvResi->destroy();
    delete m_apcPicYuvResi;
    m_apcPicYuvResi  = NULL;
  }
  
  // free lambda and QP arrays
  if ( m_pdRdPicLambda ) { xFree( m_pdRdPicLambda ); m_pdRdPicLambda = NULL; }
  if ( m_pdRdPicQp     ) { xFree( m_pdRdPicQp     ); m_pdRdPicQp     = NULL; }
  if ( m_piRdPicQp     ) { xFree( m_piRdPicQp     ); m_piRdPicQp     = NULL; }
}

Void TEncSlice::init( TEncTop* pcEncTop )
{
  m_pcCfg             = pcEncTop;
  m_pcListPic         = pcEncTop->getListPic();
  
  //SB
  m_pcPicEncoder      = pcEncTop->getPicEncoder();
  m_pcCuEncoder       = pcEncTop->getCuEncoder();
  m_pcPredSearch      = pcEncTop->getPredSearch();
  
  m_pcEntropyCoder    = pcEncTop->getEntropyCoder();
  m_pcCavlcCoder      = pcEncTop->getCavlcCoder();
  m_pcSbacCoder       = pcEncTop->getSbacCoder();
  m_pcBinCABAC        = pcEncTop->getBinCABAC();
  m_pcTrQuant         = pcEncTop->getTrQuant();
  
  m_pcBitCounter      = pcEncTop->getBitCounter();
  m_pcRdCost          = pcEncTop->getRdCost();
  m_pppcRDSbacCoder   = pcEncTop->getRDSbacCoder();
  m_pcRDGoOnSbacCoder = pcEncTop->getRDGoOnSbacCoder();
  
  // create lambda and QP arrays
  m_pdRdPicLambda     = (Double*)xMalloc( Double, m_pcCfg->getDeltaQpRD() * 2 + 1 );
  m_pdRdPicQp         = (Double*)xMalloc( Double, m_pcCfg->getDeltaQpRD() * 2 + 1 );
  m_piRdPicQp         = (Int*   )xMalloc( Int,    m_pcCfg->getDeltaQpRD() * 2 + 1 );
}

/**
 - non-referenced frame marking
 - QP computation based on temporal structure
 - lambda computation based on QP
 .
 \param pcPic         picture class
 \param iPOCLast      POC of last picture
 \param uiPOCCurr     current POC
 \param iNumPicRcvd   number of received pictures
 \param iTimeOffset   POC offset for hierarchical structure
 \param iDepth        temporal layer depth
 \param rpcSlice      slice header class
 */
Void TEncSlice::initEncSlice( TComPic* pcPic, TComSlice*& rpcSlice )
{
  Double dQP;
  Double dLambda;
  
  rpcSlice = pcPic->getSlice(0);
  rpcSlice->setSliceBits(0);
  rpcSlice->setPic( pcPic );
  rpcSlice->initSlice();
  rpcSlice->setPOC( pcPic->getPOC() );
  
  // slice type
  SliceType eSliceType  = pcPic->getSliceType()  ;
  rpcSlice->setSliceType    ( eSliceType );
  
  // ------------------------------------------------------------------------------------------------------------------
  // Non-referenced frame marking
  // ------------------------------------------------------------------------------------------------------------------
  
  rpcSlice->setReferenced(pcPic->getReferenced()) ;
  
  // ------------------------------------------------------------------------------------------------------------------
  // QP setting
  // ------------------------------------------------------------------------------------------------------------------

  Double dOrigQP = double(pcPic->getQP());
  
  // ------------------------------------------------------------------------------------------------------------------
  // Lambda computation
  // ------------------------------------------------------------------------------------------------------------------
  
  Int iQP;

  // pre-compute lambda and QP values for all possible QP candidates
#if QC_MOD_LCEC_RDOQ
  m_pcTrQuant->setRDOQOffset(1);
#endif

  for ( Int iDQpIdx = 0; iDQpIdx < 2 * m_pcCfg->getDeltaQpRD() + 1; iDQpIdx++ )
  {
    // compute QP value
    dQP = dOrigQP + ((iDQpIdx+1)>>1)*(iDQpIdx%2 ? -1 : 1);
    
    // compute lambda value
    Int    NumberBFrames = ( m_pcCfg->getRateGOPSize() - 1 );
    Int    SHIFT_QP = 12;
    Double dLambda_scale = 1.0 - Clip3( 0.0, 0.5, 0.05*(Double)NumberBFrames );
#if FULL_NBIT
    Int    bitdepth_luma_qp_scale = 6 * (g_uiBitDepth - 8);
#else
    Int    bitdepth_luma_qp_scale = 0;
#endif
    Double qp_temp = (double) dQP + bitdepth_luma_qp_scale - SHIFT_QP;
#if FULL_NBIT
    Double qp_temp_orig = (double) dQP - SHIFT_QP;
#endif
    // Case #1: I or P-slices (key-frame)
    if(eSliceType == I_SLICE || eSliceType == P_SLICE )
    {
      if ( m_pcCfg->getUseRDOQ() && rpcSlice->isIntra() && dQP == dOrigQP )
      {
        dLambda = 0.57 * pow( 2.0, qp_temp/3.0 );
      }
      else
      {
        if ( NumberBFrames > 0 ) // HB structure or HP structure
        {
          dLambda = 0.68 * pow( 2.0, qp_temp/3.0 );
        }
        else                     // IPP structure
        {
          dLambda = 0.85 * pow( 2.0, qp_temp/3.0 );
        }
      }
      dLambda *= dLambda_scale;
    }
    else // P or B slices for HB or HP structure
    {
      dLambda = 0.68 * pow( 2.0, qp_temp/3.0 );
      if ( pcPic->getSlice(0)->isInterB () )
      {
#if FULL_NBIT
        dLambda *= Clip3( 2.00, 4.00, (qp_temp_orig / 6.0) ); // (j == B_SLICE && p_cur_frm->layer != 0 )
#else
        dLambda *= Clip3( 2.00, 4.00, (qp_temp / 6.0) ); // (j == B_SLICE && p_cur_frm->layer != 0 )
#endif
        if ( rpcSlice->isReferenced() ) // HB structure and referenced
        {
          dLambda *= 0.80;
          dLambda *= dLambda_scale;
        }
      }
      else
      {
        dLambda *= dLambda_scale;
      }
    }
    // if hadamard is used in ME process
    if ( !m_pcCfg->getUseHADME() ) dLambda *= 0.95;
    
    iQP = Max( MIN_QP, Min( MAX_QP, (Int)floor( dQP + 0.5 ) ) );
    
    m_pdRdPicLambda[iDQpIdx] = dLambda;
    m_pdRdPicQp    [iDQpIdx] = dQP;
    m_piRdPicQp    [iDQpIdx] = iQP;
  }
  
  // obtain dQP = 0 case
  dLambda = m_pdRdPicLambda[0];
  dQP     = m_pdRdPicQp    [0];
  iQP     = m_piRdPicQp    [0];
  
  // store lambda
//GT VSO
  m_pcRdCost->setUseLambdaScaleVSO  ( (m_pcCfg->getUseVSO() ||  m_pcCfg->getForceLambdaScaleVSO()) && m_pcCfg->isDepthCoder()  );   
  m_pcRdCost->setLambdaVSO( dLambda * m_pcCfg->getLambdaScaleVSO() ); 
//GT VSO end

  m_pcRdCost ->setLambda      ( dLambda );
  m_pcTrQuant->setLambda      ( dLambda );
  rpcSlice   ->setLambda      ( dLambda );
  m_pcRdCost ->setLambdaMVReg ( dLambda * m_pcCfg->getMultiviewMvRegLambdaScale() );
  
//#if HB_LAMBDA_FOR_LDC
//  // restore original slice type
//  if ( m_pcCfg->getUseLDC() )
//  {
//    eSliceType = P_SLICE;
//  }
//  eSliceType = (iPOCLast == 0 || uiPOCCurr % m_pcCfg->getIntraPeriod() == 0 || m_pcGOPEncoder->getGOPSize() == 0) ? I_SLICE : eSliceType;
//  
//  rpcSlice->setSliceType        ( eSliceType );
//#endif
  
  rpcSlice->setSliceQp          ( iQP );
  rpcSlice->setSliceQpDelta     ( 0 );
  rpcSlice->setNumRefIdx        ( REF_PIC_LIST_0, eSliceType == I_SLICE ? 0 : pcPic->getNumRefs(REF_PIC_LIST_0) ) ;
  rpcSlice->setNumRefIdx        ( REF_PIC_LIST_1, eSliceType == I_SLICE ? 0 : pcPic->getNumRefs(REF_PIC_LIST_1) ) ;
  
  rpcSlice->setSymbolMode       ( m_pcCfg->getSymbolMode());
  rpcSlice->setLoopFilterDisable( m_pcCfg->getLoopFilterDisable() );
  
  rpcSlice->setDepth            ( 0 );
  rpcSlice->setViewIdx          ( pcPic->getViewIdx() );

  rpcSlice->setColDir( pcPic->getColDir());

  assert( m_apcPicYuvPred );
  assert( m_apcPicYuvResi );
  
  pcPic->setPicYuvPred( m_apcPicYuvPred );
  pcPic->setPicYuvResi( m_apcPicYuvResi );
  rpcSlice->setSliceMode            ( m_pcCfg->getSliceMode()            );
  rpcSlice->setSliceArgument        ( m_pcCfg->getSliceArgument()        );
  rpcSlice->setEntropySliceMode     ( m_pcCfg->getEntropySliceMode()     );
  rpcSlice->setEntropySliceArgument ( m_pcCfg->getEntropySliceArgument() );
}

// ====================================================================================================================
// Public member functions
// ====================================================================================================================

Void TEncSlice::setSearchRange( TComSlice* pcSlice )
{
  Int iCurrPOC = pcSlice->getPOC();
  Int iRefPOC;
  Int iRateGOPSize = m_pcCfg->getRateGOPSize();
  Int iOffset = (iRateGOPSize >> 1);
  Int iMaxSR = m_pcCfg->getSearchRange();
  Int iNumPredDir = pcSlice->isInterP() ? 1 : 2;
  
  for (Int iDir = 0; iDir <= iNumPredDir; iDir++)
  {
    RefPicList e = (RefPicList)iDir;
    for (Int iRefIdx = 0; iRefIdx < pcSlice->getNumRefIdx(e); iRefIdx++)
    {
      iRefPOC = pcSlice->getRefPic(e, iRefIdx)->getPOC();
      Int iNewSR = Clip3(8, iMaxSR, (iMaxSR*ADAPT_SR_SCALE*abs(iCurrPOC - iRefPOC)+iOffset)/iRateGOPSize);
      m_pcPredSearch->setAdaptiveSearchRange(iDir, iRefIdx, iNewSR);
    }
  }
}

/**
 - multi-loop slice encoding for different slice QP
 .
 \param rpcPic    picture class
 */
Void TEncSlice::precompressSlice( TComPic*& rpcPic )
{
  // if deltaQP RD is not used, simply return
  if ( m_pcCfg->getDeltaQpRD() == 0 ) return;
  
  TComSlice* pcSlice        = rpcPic->getSlice(getSliceIdx());
  Double     dPicRdCostBest = MAX_DOUBLE;
  Double dSumCURdCostBest;
  UInt64     uiPicDistBest;
  UInt64     uiPicBitsBest;
  UInt       uiQpIdxBest = 0;
  
  Double dFrameLambda;
#if FULL_NBIT
  Int    SHIFT_QP = 12 + 6 * (g_uiBitDepth - 8);
#else
  Int    SHIFT_QP = 12;
#endif
  
  // set frame lambda
  if (m_pcCfg->getGOPSize() > 1)
  {
    dFrameLambda = 0.68 * pow (2, (m_piRdPicQp[0]  - SHIFT_QP) / 3.0) * (pcSlice->isInterB()? 2 : 1);
  }
  else
  {
    dFrameLambda = 0.68 * pow (2, (m_piRdPicQp[0] - SHIFT_QP) / 3.0);
  }
  m_pcRdCost      ->setFrameLambda(dFrameLambda);
  m_pcRdCost      ->setFrameLambdaVSO( dFrameLambda * m_pcCfg->getLambdaScaleVSO() ); 
  
  // for each QP candidate
  for ( UInt uiQpIdx = 0; uiQpIdx < 2 * m_pcCfg->getDeltaQpRD() + 1; uiQpIdx++ )
  {
    pcSlice       ->setSliceQp             ( m_piRdPicQp    [uiQpIdx] );
    m_pcRdCost    ->setLambda              ( m_pdRdPicLambda[uiQpIdx] );
    m_pcTrQuant   ->setLambda              ( m_pdRdPicLambda[uiQpIdx] );
    pcSlice       ->setLambda              ( m_pdRdPicLambda[uiQpIdx] );
    m_pcRdCost    ->setLambdaMVReg         ( m_pdRdPicLambda[uiQpIdx] * m_pcCfg->getMultiviewMvRegLambdaScale() );
    
    // try compress
    compressSlice   ( rpcPic );
    
    Double dPicRdCost;
    UInt64 uiPicDist        = m_uiPicDist;
    UInt64 uiALFBits        = 0;
    
    m_pcPicEncoder->preLoopFilterPicAll( rpcPic, uiPicDist, uiALFBits );
    
    // compute RD cost and choose the best
    dPicRdCost = m_pcRdCost->calcRdCost64( m_uiPicTotalBits + uiALFBits, uiPicDist, true, DF_SSE_FRAME);
    
    if ( dPicRdCost < dPicRdCostBest )
    {
      uiQpIdxBest    = uiQpIdx;
      dPicRdCostBest = dPicRdCost;
      dSumCURdCostBest = m_dPicRdCost;
      
      uiPicBitsBest = m_uiPicTotalBits + uiALFBits;
      uiPicDistBest = uiPicDist;
    }
  }
  
  // set best values
  pcSlice       ->setSliceQp             ( m_piRdPicQp    [uiQpIdxBest] );
  m_pcRdCost    ->setLambda              ( m_pdRdPicLambda[uiQpIdxBest] );
  m_pcTrQuant   ->setLambda              ( m_pdRdPicLambda[uiQpIdxBest] );
  pcSlice       ->setLambda              ( m_pdRdPicLambda[uiQpIdxBest] );
  m_pcRdCost    ->setLambdaMVReg         ( m_pdRdPicLambda[uiQpIdxBest] * m_pcCfg->getMultiviewMvRegLambdaScale() );
}

/** \param rpcPic   picture class
 */
Void TEncSlice::compressSlice( TComPic*& rpcPic )
{
  UInt  uiCUAddr;
  UInt   uiStartCUAddr;
  UInt   uiBoundingCUAddr;
  UInt64 uiBitsCoded            = 0;
  TEncBinCABAC* pppcRDSbacCoder = NULL;
  TComSlice* pcSlice            = rpcPic->getSlice(getSliceIdx());
  xDetermineStartAndBoundingCUAddr ( uiStartCUAddr, uiBoundingCUAddr, rpcPic, false );
  
#ifdef WEIGHT_PRED
  //------------------------------------------------------------------------------
  //  Weighted Prediction parameters estimation.
  //------------------------------------------------------------------------------
  // calculate AC/DC values for current picture
  if( pcSlice->getPPS()->getUseWP() || pcSlice->getPPS()->getWPBiPredIdc() )
    xCalcACDCParamSlice(pcSlice);

  Bool	wp_Explicit =  (pcSlice->getSliceType()==P_SLICE && pcSlice->getPPS()->getUseWP()) || (pcSlice->getSliceType()==B_SLICE && pcSlice->getPPS()->getWPBiPredIdc()==1);
  Bool	wp_Implicit = (pcSlice->getSliceType()==B_SLICE && pcSlice->getPPS()->getWPBiPredIdc()==2);

  if ( wp_Explicit || wp_Implicit )
  {
	//------------------------------------------------------------------------------
	//  Weighted Prediction implemented at Slice level, sliceMode=2 only.
	//------------------------------------------------------------------------------
    if ( pcSlice->getSliceMode()==2 || pcSlice->getEntropySliceMode()==2 )
    {
      printf("Weighted Prediction not implemented with slice mode determined by max number of bins.\n"); exit(0);
    }

    if( wp_Explicit )
      xEstimateWPParamSlice(pcSlice);

    pcSlice->initWpScaling();
    pcSlice->displayWpScaling();
  }
#endif

  // initialize cost values
  m_uiPicTotalBits  = 0;
  m_dPicRdCost      = 0;
  m_uiPicDist       = 0;
  
  // set entropy coder
  if( m_pcCfg->getUseSBACRD() )
  {
    m_pcSbacCoder->init( m_pcBinCABAC );
    m_pcEntropyCoder->setEntropyCoder   ( m_pcSbacCoder, pcSlice );
    m_pcEntropyCoder->resetEntropy      ();
    m_pppcRDSbacCoder[0][CI_CURR_BEST]->load(m_pcSbacCoder);
    pppcRDSbacCoder = (TEncBinCABAC *) m_pppcRDSbacCoder[0][CI_CURR_BEST]->getEncBinIf();
    pppcRDSbacCoder->setBinCountingEnableFlag( false );
    pppcRDSbacCoder->setBinsCoded( 0 );
  }
  else
  {
    m_pcCavlcCoder  ->setAdaptFlag    ( false );
    m_pcEntropyCoder->setEntropyCoder ( m_pcCavlcCoder, pcSlice );
    m_pcEntropyCoder->resetEntropy      ();
    m_pcEntropyCoder->setBitstream    ( m_pcBitCounter );
  }
  
  // initialize ALF parameters
  m_pcEntropyCoder->setAlfCtrl(false);
  m_pcEntropyCoder->setMaxAlfCtrlDepth(0); //unnecessary
  
  // for every CU in slice
  for(  uiCUAddr = uiStartCUAddr; uiCUAddr < uiBoundingCUAddr; uiCUAddr++  )
  {
    // set QP
    m_pcCuEncoder->setQpLast( pcSlice->getSliceQp() );
    // initialize CU encoder
    TComDataCU*& pcCU = rpcPic->getCU( uiCUAddr );
    pcCU->initCU( rpcPic, uiCUAddr );
    
    // if RD based on SBAC is used
    if( m_pcCfg->getUseSBACRD() )
    {
      // set go-on entropy coder
      m_pcEntropyCoder->setEntropyCoder ( m_pcRDGoOnSbacCoder, pcSlice );
      m_pcEntropyCoder->setBitstream    ( m_pcBitCounter );
      
      // run CU encoder
      m_pcCuEncoder->compressCU( pcCU );
      
      // restore entropy coder to an initial stage
      m_pcEntropyCoder->setEntropyCoder ( m_pppcRDSbacCoder[0][CI_CURR_BEST], pcSlice );
      m_pcEntropyCoder->setBitstream    ( m_pcBitCounter );
      pppcRDSbacCoder->setBinCountingEnableFlag( true );
      
      m_pcCuEncoder->encodeCU( pcCU );

      pppcRDSbacCoder->setBinCountingEnableFlag( false );
      uiBitsCoded += m_pcBitCounter->getNumberOfWrittenBits();
      if (m_pcCfg->getSliceMode()==AD_HOC_SLICES_FIXED_NUMBER_OF_BYTES_IN_SLICE && ( ( pcSlice->getSliceBits() + uiBitsCoded ) >> 3 ) > m_pcCfg->getSliceArgument())
      {
        if (uiCUAddr==uiStartCUAddr && pcSlice->getSliceBits()==0)
        {
          // Could not fit even a single LCU within the slice under the defined byte-constraint. Display a warning message and code 1 LCU in the slice.
          fprintf(stdout,"\nSlice overflow warning! codedBits=%6d, limitBytes=%6d", m_pcBitCounter->getNumberOfWrittenBits(), m_pcCfg->getSliceArgument() );
          uiCUAddr = uiCUAddr + 1;
        }
        pcSlice->setNextSlice( true );
        break;
      }
      
      UInt uiBinsCoded = pppcRDSbacCoder->getBinsCoded();
      if (m_pcCfg->getEntropySliceMode()==SHARP_MULTIPLE_CONSTRAINT_BASED_ENTROPY_SLICE && uiBinsCoded > m_pcCfg->getEntropySliceArgument())
      {
        if (uiCUAddr == uiStartCUAddr)
        {
          // Could not fit even a single LCU within the entropy slice under the defined byte-constraint. Display a warning message and code 1 LCU in the entropy slice.
          fprintf(stdout,"\nEntropy Slice overflow warning! codedBins=%6d, limitBins=%6d", uiBinsCoded, m_pcCfg->getEntropySliceArgument() );
          uiCUAddr = uiCUAddr + 1;
        }
        pcSlice->setNextEntropySlice( true );
        break;
      }
    }
    // other case: encodeCU is not called
    else
    {
      m_pcCuEncoder->compressCU( pcCU );
      m_pcCavlcCoder ->setAdaptFlag(true);
      m_pcCuEncoder->encodeCU( pcCU );
      
      uiBitsCoded += m_pcBitCounter->getNumberOfWrittenBits();
      if (m_pcCfg->getSliceMode()==AD_HOC_SLICES_FIXED_NUMBER_OF_BYTES_IN_SLICE && ( ( pcSlice->getSliceBits() + uiBitsCoded ) >> 3 ) > m_pcCfg->getSliceArgument())
      {
        if (uiCUAddr==uiStartCUAddr && pcSlice->getSliceBits()==0)
        {
          // Could not fit even a single LCU within the slice under the defined byte-constraint. Display a warning message and code 1 LCU in the slice.
          fprintf(stdout,"\nSlice overflow warning! codedBits=%6d, limitBytes=%6d", m_pcBitCounter->getNumberOfWrittenBits(), m_pcCfg->getSliceArgument() );
          uiCUAddr = uiCUAddr + 1;
        }
        pcSlice->setNextSlice( true );
        break;
      }

      if (m_pcCfg->getEntropySliceMode()==SHARP_MULTIPLE_CONSTRAINT_BASED_ENTROPY_SLICE && uiBitsCoded > m_pcCfg->getEntropySliceArgument())
      {
        if (uiCUAddr == uiStartCUAddr)
        {
          // Could not fit even a single LCU within the entropy slice under the defined bit/bin-constraint. Display a warning message and code 1 LCU in the entropy slice.
          fprintf(stdout,"\nEntropy Slice overflow warning! codedBits=%6d, limitBits=%6d", m_pcBitCounter->getNumberOfWrittenBits(), m_pcCfg->getEntropySliceArgument() );
          uiCUAddr = uiCUAddr + 1;
        }
        pcSlice->setNextEntropySlice( true );
        break;
      }
      m_pcCavlcCoder ->setAdaptFlag(false);
    }
    
    m_uiPicTotalBits += pcCU->getTotalBits();
    m_dPicRdCost     += pcCU->getTotalCost();
    m_uiPicDist      += pcCU->getTotalDistortion();
  }
  pcSlice->setSliceCurEndCUAddr( uiCUAddr );
  pcSlice->setEntropySliceCurEndCUAddr( uiCUAddr );
  pcSlice->setSliceBits( (UInt)(pcSlice->getSliceBits() + uiBitsCoded) );
}

/**
 \param  rpcPic        picture class
 \retval rpcBitstream  bitstream class
 */
Void TEncSlice::encodeSlice   ( TComPic*& rpcPic, TComBitstream*& rpcBitstream )
{
  UInt       uiCUAddr;
  UInt       uiStartCUAddr;
  UInt       uiBoundingCUAddr;
  xDetermineStartAndBoundingCUAddr  ( uiStartCUAddr, uiBoundingCUAddr, rpcPic, true );
  TComSlice* pcSlice = rpcPic->getSlice(getSliceIdx());

  // choose entropy coder
  Int iSymbolMode = pcSlice->getSymbolMode();
  if (iSymbolMode)
  {
    m_pcSbacCoder->init( (TEncBinIf*)m_pcBinCABAC );
    m_pcEntropyCoder->setEntropyCoder ( m_pcSbacCoder, pcSlice );
  }
  else
  {
    m_pcCavlcCoder  ->setAdaptFlag( true );
    m_pcEntropyCoder->setEntropyCoder ( m_pcCavlcCoder, pcSlice );
    m_pcEntropyCoder->resetEntropy();
  }
  
  // set bitstream
  m_pcEntropyCoder->setBitstream( rpcBitstream );
  // for every CU
#if ENC_DEC_TRACE
  g_bJustDoIt = g_bEncDecTraceEnable;
#endif
  DTRACE_CABAC_V( g_nSymbolCounter++ );
  DTRACE_CABAC_T( "\tPOC: " );
  DTRACE_CABAC_V( rpcPic->getPOC() );
  DTRACE_CABAC_T( "\n" );
#if ENC_DEC_TRACE
  g_bJustDoIt = g_bEncDecTraceDisable;
#endif

  for(  uiCUAddr = uiStartCUAddr; uiCUAddr<uiBoundingCUAddr; uiCUAddr++  )
  {
    m_pcCuEncoder->setQpLast( pcSlice->getSliceQp() );
    TComDataCU*& pcCU = rpcPic->getCU( uiCUAddr );
#if ENC_DEC_TRACE
    g_bJustDoIt = g_bEncDecTraceEnable;
#endif
    if ( (m_pcCfg->getSliceMode()!=0 || m_pcCfg->getEntropySliceMode()!=0) && uiCUAddr==uiBoundingCUAddr-1 )
    {
      m_pcCuEncoder->encodeCU( pcCU, true );
    }
    else
    {
      m_pcCuEncoder->encodeCU( pcCU );
    }
#if ENC_DEC_TRACE
    g_bJustDoIt = g_bEncDecTraceDisable;
#endif    
  }
}

/** Determines the starting and bounding LCU address of current slice / entropy slice
 * \param bEncodeSlice Identifies if the calling function is compressSlice() [false] or encodeSlice() [true]
 * \returns Updates uiStartCUAddr, uiBoundingCUAddr with appropriate LCU address
 */
Void TEncSlice::xDetermineStartAndBoundingCUAddr  ( UInt& uiStartCUAddr, UInt& uiBoundingCUAddr, TComPic*& rpcPic, Bool bEncodeSlice )
{
  TComSlice* pcSlice = rpcPic->getSlice(getSliceIdx());
  UInt uiStartCUAddrSlice, uiBoundingCUAddrSlice;
  uiStartCUAddrSlice        = pcSlice->getSliceCurStartCUAddr();
  UInt uiNumberOfCUsInFrame = rpcPic->getNumCUsInFrame();
  uiBoundingCUAddrSlice     = uiNumberOfCUsInFrame;
  if (bEncodeSlice) 
  {
    UInt uiCUAddrIncrement;
    switch (m_pcCfg->getSliceMode())
    {
    case AD_HOC_SLICES_FIXED_NUMBER_OF_LCU_IN_SLICE:
      uiCUAddrIncrement        = m_pcCfg->getSliceArgument();
      uiBoundingCUAddrSlice    = ((uiStartCUAddrSlice + uiCUAddrIncrement     ) < uiNumberOfCUsInFrame ) ? (uiStartCUAddrSlice + uiCUAddrIncrement     ) : uiNumberOfCUsInFrame;
      break;
    case AD_HOC_SLICES_FIXED_NUMBER_OF_BYTES_IN_SLICE:
      uiCUAddrIncrement        = rpcPic->getNumCUsInFrame();
      uiBoundingCUAddrSlice    = pcSlice->getSliceCurEndCUAddr();
      break;
    default:
      uiCUAddrIncrement        = rpcPic->getNumCUsInFrame();
      uiBoundingCUAddrSlice    = uiNumberOfCUsInFrame;
      break;
    } 
    pcSlice->setSliceCurEndCUAddr( uiBoundingCUAddrSlice );
  }
  else
  {
    UInt uiCUAddrIncrement     ;
    switch (m_pcCfg->getSliceMode())
    {
    case AD_HOC_SLICES_FIXED_NUMBER_OF_LCU_IN_SLICE:
      uiCUAddrIncrement        = m_pcCfg->getSliceArgument();
      uiBoundingCUAddrSlice    = ((uiStartCUAddrSlice + uiCUAddrIncrement     ) < uiNumberOfCUsInFrame ) ? (uiStartCUAddrSlice + uiCUAddrIncrement     ) : uiNumberOfCUsInFrame;
      break;
    default:
      uiCUAddrIncrement        = rpcPic->getNumCUsInFrame();
      uiBoundingCUAddrSlice    = uiNumberOfCUsInFrame;
      break;
    } 
    pcSlice->setSliceCurEndCUAddr( uiBoundingCUAddrSlice );
  }

  // Entropy slice
  UInt uiStartCUAddrEntropySlice, uiBoundingCUAddrEntropySlice;
  uiStartCUAddrEntropySlice    = pcSlice->getEntropySliceCurStartCUAddr();
  uiBoundingCUAddrEntropySlice = uiNumberOfCUsInFrame;
  if (bEncodeSlice) 
  {
    UInt uiCUAddrIncrement;
    switch (m_pcCfg->getEntropySliceMode())
    {
    case SHARP_FIXED_NUMBER_OF_LCU_IN_ENTROPY_SLICE:
      uiCUAddrIncrement               = m_pcCfg->getEntropySliceArgument();
      uiBoundingCUAddrEntropySlice    = ((uiStartCUAddrEntropySlice + uiCUAddrIncrement) < uiNumberOfCUsInFrame ) ? (uiStartCUAddrEntropySlice + uiCUAddrIncrement) : uiNumberOfCUsInFrame;
      break;
    case SHARP_MULTIPLE_CONSTRAINT_BASED_ENTROPY_SLICE:
      uiCUAddrIncrement               = rpcPic->getNumCUsInFrame();
      uiBoundingCUAddrEntropySlice    = pcSlice->getEntropySliceCurEndCUAddr();
      break;
    default:
      uiCUAddrIncrement               = rpcPic->getNumCUsInFrame();
      uiBoundingCUAddrEntropySlice    = uiNumberOfCUsInFrame;
      break;
    } 
    pcSlice->setEntropySliceCurEndCUAddr( uiBoundingCUAddrEntropySlice );
  }
  else
  {
    UInt uiCUAddrIncrement;
    switch (m_pcCfg->getEntropySliceMode())
    {
    case SHARP_FIXED_NUMBER_OF_LCU_IN_ENTROPY_SLICE:
      uiCUAddrIncrement               = m_pcCfg->getEntropySliceArgument();
      uiBoundingCUAddrEntropySlice    = ((uiStartCUAddrEntropySlice + uiCUAddrIncrement) < uiNumberOfCUsInFrame ) ? (uiStartCUAddrEntropySlice + uiCUAddrIncrement) : uiNumberOfCUsInFrame;
      break;
    default:
      uiCUAddrIncrement               = rpcPic->getNumCUsInFrame();
      uiBoundingCUAddrEntropySlice    = uiNumberOfCUsInFrame;
      break;
    } 
    pcSlice->setEntropySliceCurEndCUAddr( uiBoundingCUAddrEntropySlice );
  }

  // Make a joint decision based on reconstruction and entropy slice bounds
  uiStartCUAddr    = max(uiStartCUAddrSlice   , uiStartCUAddrEntropySlice   );
  uiBoundingCUAddr = min(uiBoundingCUAddrSlice, uiBoundingCUAddrEntropySlice);


  if (!bEncodeSlice)
  {
    // For fixed number of LCU within an entropy and reconstruction slice we already know whether we will encounter end of entropy and/or reconstruction slice
    // first. Set the flags accordingly.
    if ( (m_pcCfg->getSliceMode()==AD_HOC_SLICES_FIXED_NUMBER_OF_LCU_IN_SLICE && m_pcCfg->getEntropySliceMode()==SHARP_FIXED_NUMBER_OF_LCU_IN_ENTROPY_SLICE)
      || (m_pcCfg->getSliceMode()==0 && m_pcCfg->getEntropySliceMode()==SHARP_FIXED_NUMBER_OF_LCU_IN_ENTROPY_SLICE)
      || (m_pcCfg->getSliceMode()==AD_HOC_SLICES_FIXED_NUMBER_OF_LCU_IN_SLICE && m_pcCfg->getEntropySliceMode()==0) )
    {
      if (uiBoundingCUAddrSlice < uiBoundingCUAddrEntropySlice)
      {
        pcSlice->setNextSlice       ( true );
        pcSlice->setNextEntropySlice( false );
      }
      else if (uiBoundingCUAddrSlice > uiBoundingCUAddrEntropySlice)
      {
        pcSlice->setNextSlice       ( false );
        pcSlice->setNextEntropySlice( true );
      }
      else
      {
        pcSlice->setNextSlice       ( true );
        pcSlice->setNextEntropySlice( true );
      }
    }
    else
    {
      pcSlice->setNextSlice       ( false );
      pcSlice->setNextEntropySlice( false );
    }
  }
}
