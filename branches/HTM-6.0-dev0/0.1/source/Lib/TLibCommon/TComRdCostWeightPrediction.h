

/** \file     TComRdCostWeightPrediction.h
    \brief    RD cost computation classes (header)
*/

#ifndef __TCOMRDCOSTWEIGHTPREDICTION__
#define __TCOMRDCOSTWEIGHTPREDICTION__


#include "CommonDef.h"
#include "TComPattern.h"
#include "TComMv.h"
#include "TComRdCost.h"
#ifdef WEIGHT_PRED
  #include "TComSlice.h"
#endif

class DistParam;
class TComPattern;

// ====================================================================================================================
// Class definition
// ====================================================================================================================

/// RD cost computation class, with Weighted Prediction
class TComRdCostWeightPrediction
{
private:
  static  Int   m_w0, m_w1; // current wp scaling values
  static  Int   m_shift;
  static  Int   m_offset;
  static  Int   m_round;
  static  Bool  m_xSetDone;

public:
  TComRdCostWeightPrediction();
  virtual ~TComRdCostWeightPrediction();
  
protected:
    
  static inline Void  xSetWPscale(Int w0, Int w1, Int shift, Int offset, Int round);

  static UInt xGetSSEw          ( DistParam* pcDtParam );
  static UInt xGetSSE4w         ( DistParam* pcDtParam );
  static UInt xGetSSE8w         ( DistParam* pcDtParam );
  static UInt xGetSSE16w        ( DistParam* pcDtParam );
  static UInt xGetSSE32w        ( DistParam* pcDtParam );
  static UInt xGetSSE64w        ( DistParam* pcDtParam );
  static UInt xGetSSE16Nw       ( DistParam* pcDtParam );
  
  static UInt xGetSADw          ( DistParam* pcDtParam );
  static UInt xGetSAD4w         ( DistParam* pcDtParam );
  static UInt xGetSAD8w         ( DistParam* pcDtParam );
  static UInt xGetSAD16w        ( DistParam* pcDtParam );
  static UInt xGetSAD32w        ( DistParam* pcDtParam );
  static UInt xGetSAD64w        ( DistParam* pcDtParam );
  static UInt xGetSAD16Nw       ( DistParam* pcDtParam );
  
  static UInt xGetSADsw         ( DistParam* pcDtParam );
  static UInt xGetSADs4w        ( DistParam* pcDtParam );
  static UInt xGetSADs8w        ( DistParam* pcDtParam );
  static UInt xGetSADs16w       ( DistParam* pcDtParam );
  static UInt xGetSADs32w       ( DistParam* pcDtParam );
  static UInt xGetSADs64w       ( DistParam* pcDtParam );
  static UInt xGetSADs16Nw      ( DistParam* pcDtParam );
  
  static UInt xGetHADs4w        ( DistParam* pcDtParam );
  static UInt xGetHADs8w        ( DistParam* pcDtParam );
  static UInt xGetHADsw         ( DistParam* pcDtParam );
  static UInt xCalcHADs2x2w     ( Pel *piOrg, Pel *piCurr, Int iStrideOrg, Int iStrideCur, Int iStep );
  static UInt xCalcHADs4x4w     ( Pel *piOrg, Pel *piCurr, Int iStrideOrg, Int iStrideCur, Int iStep );
  static UInt xCalcHADs8x8w     ( Pel *piOrg, Pel *piCurr, Int iStrideOrg, Int iStrideCur, Int iStep );
  
};// END CLASS DEFINITION TComRdCostWeightPrediction

inline Void  TComRdCostWeightPrediction::xSetWPscale(Int w0, Int w1, Int shift, Int offset, Int round)
{
  m_w0        = w0;
  m_w1        = w1;
  m_shift     = shift;
  m_offset    = offset;
  m_round     = round;

  m_xSetDone  = true;
}

#endif // __TCOMRDCOSTWEIGHTPREDICTION__

