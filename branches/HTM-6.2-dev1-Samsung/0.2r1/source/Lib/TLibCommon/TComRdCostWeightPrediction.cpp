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



/** \file     TComRdCostWeightPrediction.cpp
    \brief    RD cost computation class with Weighted-Prediction
*/

#include <math.h>
#include <assert.h>
#include "TComRdCost.h"
#include "TComRdCostWeightPrediction.h"

#ifdef WEIGHT_PRED

Int   TComRdCostWeightPrediction::m_w0        = 0,
      TComRdCostWeightPrediction::m_w1        = 0;
Int   TComRdCostWeightPrediction::m_shift     = 0;
Int   TComRdCostWeightPrediction::m_offset    = 0;
Int   TComRdCostWeightPrediction::m_round     = 0;
Bool  TComRdCostWeightPrediction::m_xSetDone  = false;

// ====================================================================================================================
// Distortion functions
// ====================================================================================================================

TComRdCostWeightPrediction::TComRdCostWeightPrediction()
{
}

TComRdCostWeightPrediction::~TComRdCostWeightPrediction()
{
}

// --------------------------------------------------------------------------------------------------------------------
// SAD
// --------------------------------------------------------------------------------------------------------------------

UInt TComRdCostWeightPrediction::xGetSADw( DistParam* pcDtParam )
{
  Pel* piOrg   = pcDtParam->pOrg;
  Pel* piCur   = pcDtParam->pCur;
  Pel  pred;
  Int  iRows   = pcDtParam->iRows;
  Int  iCols   = pcDtParam->iCols;
  Int  iSubShift  = pcDtParam->iSubShift;
  Int  iSubStep   = ( 1 << iSubShift );
  Int  iStrideCur = pcDtParam->iStrideCur*iSubStep;
  Int  iStrideOrg = pcDtParam->iStrideOrg*iSubStep;

  UInt            uiComp    = pcDtParam->uiComp;
  assert(uiComp<3);
  wpScalingParam  *wpCur    = &(pcDtParam->wpCur[uiComp]);
  Int   w0      = wpCur->w,
        offset  = wpCur->offset,
        shift   = wpCur->shift,
        round   = wpCur->round;

  UInt uiSum = 0;
#if HHI_INTERVIEW_SKIP
  if( pcDtParam->pUsed )
  {
    Pel*  piUsed      = pcDtParam->pUsed;
    Int   iStrideUsed = pcDtParam->iStrideUsed;
  for( ; iRows != 0; iRows-=iSubStep )
  {
    for (Int n = 0; n < iCols; n++ )
    {
        if( piUsed[n])
        {
      pred = ( (w0*piCur[n] + round) >> shift ) + offset ;
      //uiSum += abs( piOrg[n] - piCur[n] );
      uiSum += abs( piOrg[n] - pred );
    }
      }
      piOrg += iStrideOrg;
      piCur += iStrideCur;
      piUsed += iStrideUsed;
    }
#else
    for( ; iRows != 0; iRows-=iSubStep )
    {
      for (Int n = 0; n < iCols; n++ )
      {
        pred = ( (w0*piCur[n] + round) >> shift ) + offset ;

        //uiSum += abs( piOrg[n] - piCur[n] );
        uiSum += abs( piOrg[n] - pred );
      }
      piOrg += iStrideOrg;
      piCur += iStrideCur;
    }
#endif
#if HHI_INTERVIEW_SKIP
  }
#endif
  pcDtParam->uiComp = 255;  // reset for DEBUG (assert test)

  uiSum <<= iSubShift;
  return ( uiSum >> g_uiBitIncrement );
}

UInt TComRdCostWeightPrediction::xGetSAD4w( DistParam* pcDtParam )
{
  Pel* piOrg   = pcDtParam->pOrg;
  Pel* piCur   = pcDtParam->pCur;
  Int  iRows   = pcDtParam->iRows;
  Int  iSubShift  = pcDtParam->iSubShift;
  Int  iSubStep   = ( 1 << iSubShift );
  Int  iStrideCur = pcDtParam->iStrideCur*iSubStep;
  Int  iStrideOrg = pcDtParam->iStrideOrg*iSubStep;

  UInt uiSum = 0;

  printf("TComRdCostWeightPrediction::xGetSAD4w():\tnot implemented, use xGetSADw().\n"); fflush(stdout); exit(0);

  for( ; iRows != 0; iRows-=iSubStep )
  {
    uiSum += abs( piOrg[0] - piCur[0] );
    uiSum += abs( piOrg[1] - piCur[1] );
    uiSum += abs( piOrg[2] - piCur[2] );
    uiSum += abs( piOrg[3] - piCur[3] );

    piOrg += iStrideOrg;
    piCur += iStrideCur;
  }

  uiSum <<= iSubShift;
  return ( uiSum >> g_uiBitIncrement );
}

UInt TComRdCostWeightPrediction::xGetSAD8w( DistParam* pcDtParam )
{
  Pel* piOrg      = pcDtParam->pOrg;
  Pel* piCur      = pcDtParam->pCur;
  Int  iRows      = pcDtParam->iRows;
  Int  iSubShift  = pcDtParam->iSubShift;
  Int  iSubStep   = ( 1 << iSubShift );
  Int  iStrideCur = pcDtParam->iStrideCur*iSubStep;
  Int  iStrideOrg = pcDtParam->iStrideOrg*iSubStep;

  UInt uiSum = 0;

  printf("TComRdCostWeightPrediction::xGetSAD8w():\tnot implemented, use xGetSADw().\n"); fflush(stdout); exit(0);

  for( ; iRows != 0; iRows-=iSubStep )
  {
    uiSum += abs( piOrg[0] - piCur[0] );
    uiSum += abs( piOrg[1] - piCur[1] );
    uiSum += abs( piOrg[2] - piCur[2] );
    uiSum += abs( piOrg[3] - piCur[3] );
    uiSum += abs( piOrg[4] - piCur[4] );
    uiSum += abs( piOrg[5] - piCur[5] );
    uiSum += abs( piOrg[6] - piCur[6] );
    uiSum += abs( piOrg[7] - piCur[7] );

    piOrg += iStrideOrg;
    piCur += iStrideCur;
  }



  uiSum <<= iSubShift;
  return ( uiSum >> g_uiBitIncrement );
}

UInt TComRdCostWeightPrediction::xGetSAD16w( DistParam* pcDtParam )
{
  Pel* piOrg   = pcDtParam->pOrg;
  Pel* piCur   = pcDtParam->pCur;
  Int  iRows   = pcDtParam->iRows;
  Int  iSubShift  = pcDtParam->iSubShift;
  Int  iSubStep   = ( 1 << iSubShift );
  Int  iStrideCur = pcDtParam->iStrideCur*iSubStep;
  Int  iStrideOrg = pcDtParam->iStrideOrg*iSubStep;

  UInt uiSum = 0;

  printf("TComRdCostWeightPrediction::xGetSAD16w():\tnot implemented, use xGetSADw().\n"); fflush(stdout); exit(0);

  for( ; iRows != 0; iRows-=iSubStep )
  {
    uiSum += abs( piOrg[0] - piCur[0] );
    uiSum += abs( piOrg[1] - piCur[1] );
    uiSum += abs( piOrg[2] - piCur[2] );
    uiSum += abs( piOrg[3] - piCur[3] );
    uiSum += abs( piOrg[4] - piCur[4] );
    uiSum += abs( piOrg[5] - piCur[5] );
    uiSum += abs( piOrg[6] - piCur[6] );
    uiSum += abs( piOrg[7] - piCur[7] );
    uiSum += abs( piOrg[8] - piCur[8] );
    uiSum += abs( piOrg[9] - piCur[9] );
    uiSum += abs( piOrg[10] - piCur[10] );
    uiSum += abs( piOrg[11] - piCur[11] );
    uiSum += abs( piOrg[12] - piCur[12] );
    uiSum += abs( piOrg[13] - piCur[13] );
    uiSum += abs( piOrg[14] - piCur[14] );
    uiSum += abs( piOrg[15] - piCur[15] );

    piOrg += iStrideOrg;
    piCur += iStrideCur;
  }



  uiSum <<= iSubShift;
  return ( uiSum >> g_uiBitIncrement );
}

UInt TComRdCostWeightPrediction::xGetSAD16Nw( DistParam* pcDtParam )
{
  Pel* piOrg   = pcDtParam->pOrg;
  Pel* piCur   = pcDtParam->pCur;
  Int  iRows   = pcDtParam->iRows;
  Int  iCols   = pcDtParam->iCols;
  Int  iSubShift  = pcDtParam->iSubShift;
  Int  iSubStep   = ( 1 << iSubShift );
  Int  iStrideCur = pcDtParam->iStrideCur*iSubStep;
  Int  iStrideOrg = pcDtParam->iStrideOrg*iSubStep;

  UInt uiSum = 0;

  printf("TComRdCostWeightPrediction::xGetSAD16Nw():\tnot implemented, use xGetSADw().\n"); fflush(stdout); exit(0);

  for( ; iRows != 0; iRows-=iSubStep )
  {
    for (Int n = 0; n < iCols; n+=16 )
    {
      uiSum += abs( piOrg[n+ 0] - piCur[n+ 0] );
      uiSum += abs( piOrg[n+ 1] - piCur[n+ 1] );
      uiSum += abs( piOrg[n+ 2] - piCur[n+ 2] );
      uiSum += abs( piOrg[n+ 3] - piCur[n+ 3] );
      uiSum += abs( piOrg[n+ 4] - piCur[n+ 4] );
      uiSum += abs( piOrg[n+ 5] - piCur[n+ 5] );
      uiSum += abs( piOrg[n+ 6] - piCur[n+ 6] );
      uiSum += abs( piOrg[n+ 7] - piCur[n+ 7] );
      uiSum += abs( piOrg[n+ 8] - piCur[n+ 8] );
      uiSum += abs( piOrg[n+ 9] - piCur[n+ 9] );
      uiSum += abs( piOrg[n+10] - piCur[n+10] );
      uiSum += abs( piOrg[n+11] - piCur[n+11] );
      uiSum += abs( piOrg[n+12] - piCur[n+12] );
      uiSum += abs( piOrg[n+13] - piCur[n+13] );
      uiSum += abs( piOrg[n+14] - piCur[n+14] );
      uiSum += abs( piOrg[n+15] - piCur[n+15] );
    }
    piOrg += iStrideOrg;
    piCur += iStrideCur;
  }

  uiSum <<= iSubShift;
  return ( uiSum >> g_uiBitIncrement );
}

UInt TComRdCostWeightPrediction::xGetSAD32w( DistParam* pcDtParam )
{
  Pel* piOrg   = pcDtParam->pOrg;
  Pel* piCur   = pcDtParam->pCur;
  Int  iRows   = pcDtParam->iRows;
  Int  iSubShift  = pcDtParam->iSubShift;
  Int  iSubStep   = ( 1 << iSubShift );
  Int  iStrideCur = pcDtParam->iStrideCur*iSubStep;
  Int  iStrideOrg = pcDtParam->iStrideOrg*iSubStep;

  UInt uiSum = 0;

  printf("TComRdCostWeightPrediction::xGetSAD32w():\tnot implemented, use xGetSADw().\n"); fflush(stdout); exit(0);

  for( ; iRows != 0; iRows-=iSubStep )
  {
    uiSum += abs( piOrg[0] - piCur[0] );
    uiSum += abs( piOrg[1] - piCur[1] );
    uiSum += abs( piOrg[2] - piCur[2] );
    uiSum += abs( piOrg[3] - piCur[3] );
    uiSum += abs( piOrg[4] - piCur[4] );
    uiSum += abs( piOrg[5] - piCur[5] );
    uiSum += abs( piOrg[6] - piCur[6] );
    uiSum += abs( piOrg[7] - piCur[7] );
    uiSum += abs( piOrg[8] - piCur[8] );
    uiSum += abs( piOrg[9] - piCur[9] );
    uiSum += abs( piOrg[10] - piCur[10] );
    uiSum += abs( piOrg[11] - piCur[11] );
    uiSum += abs( piOrg[12] - piCur[12] );
    uiSum += abs( piOrg[13] - piCur[13] );
    uiSum += abs( piOrg[14] - piCur[14] );
    uiSum += abs( piOrg[15] - piCur[15] );
    uiSum += abs( piOrg[16] - piCur[16] );
    uiSum += abs( piOrg[17] - piCur[17] );
    uiSum += abs( piOrg[18] - piCur[18] );
    uiSum += abs( piOrg[19] - piCur[19] );
    uiSum += abs( piOrg[20] - piCur[20] );
    uiSum += abs( piOrg[21] - piCur[21] );
    uiSum += abs( piOrg[22] - piCur[22] );
    uiSum += abs( piOrg[23] - piCur[23] );
    uiSum += abs( piOrg[24] - piCur[24] );
    uiSum += abs( piOrg[25] - piCur[25] );
    uiSum += abs( piOrg[26] - piCur[26] );
    uiSum += abs( piOrg[27] - piCur[27] );
    uiSum += abs( piOrg[28] - piCur[28] );
    uiSum += abs( piOrg[29] - piCur[29] );
    uiSum += abs( piOrg[30] - piCur[30] );
    uiSum += abs( piOrg[31] - piCur[31] );

    piOrg += iStrideOrg;
    piCur += iStrideCur;
  }

  uiSum <<= iSubShift;
  return ( uiSum >> g_uiBitIncrement );
}

UInt TComRdCostWeightPrediction::xGetSAD64w( DistParam* pcDtParam )
{
  Pel* piOrg   = pcDtParam->pOrg;
  Pel* piCur   = pcDtParam->pCur;
  Int  iRows   = pcDtParam->iRows;
  Int  iSubShift  = pcDtParam->iSubShift;
  Int  iSubStep   = ( 1 << iSubShift );
  Int  iStrideCur = pcDtParam->iStrideCur*iSubStep;
  Int  iStrideOrg = pcDtParam->iStrideOrg*iSubStep;

  UInt uiSum = 0;

  printf("TComRdCostWeightPrediction::xGetSAD64w():\tnot implemented, use xGetSADw().\n"); fflush(stdout); exit(0);

  for( ; iRows != 0; iRows-=iSubStep )
  {
    uiSum += abs( piOrg[0] - piCur[0] );
    uiSum += abs( piOrg[1] - piCur[1] );
    uiSum += abs( piOrg[2] - piCur[2] );
    uiSum += abs( piOrg[3] - piCur[3] );
    uiSum += abs( piOrg[4] - piCur[4] );
    uiSum += abs( piOrg[5] - piCur[5] );
    uiSum += abs( piOrg[6] - piCur[6] );
    uiSum += abs( piOrg[7] - piCur[7] );
    uiSum += abs( piOrg[8] - piCur[8] );
    uiSum += abs( piOrg[9] - piCur[9] );
    uiSum += abs( piOrg[10] - piCur[10] );
    uiSum += abs( piOrg[11] - piCur[11] );
    uiSum += abs( piOrg[12] - piCur[12] );
    uiSum += abs( piOrg[13] - piCur[13] );
    uiSum += abs( piOrg[14] - piCur[14] );
    uiSum += abs( piOrg[15] - piCur[15] );
    uiSum += abs( piOrg[16] - piCur[16] );
    uiSum += abs( piOrg[17] - piCur[17] );
    uiSum += abs( piOrg[18] - piCur[18] );
    uiSum += abs( piOrg[19] - piCur[19] );
    uiSum += abs( piOrg[20] - piCur[20] );
    uiSum += abs( piOrg[21] - piCur[21] );
    uiSum += abs( piOrg[22] - piCur[22] );
    uiSum += abs( piOrg[23] - piCur[23] );
    uiSum += abs( piOrg[24] - piCur[24] );
    uiSum += abs( piOrg[25] - piCur[25] );
    uiSum += abs( piOrg[26] - piCur[26] );
    uiSum += abs( piOrg[27] - piCur[27] );
    uiSum += abs( piOrg[28] - piCur[28] );
    uiSum += abs( piOrg[29] - piCur[29] );
    uiSum += abs( piOrg[30] - piCur[30] );
    uiSum += abs( piOrg[31] - piCur[31] );
    uiSum += abs( piOrg[32] - piCur[32] );
    uiSum += abs( piOrg[33] - piCur[33] );
    uiSum += abs( piOrg[34] - piCur[34] );
    uiSum += abs( piOrg[35] - piCur[35] );
    uiSum += abs( piOrg[36] - piCur[36] );
    uiSum += abs( piOrg[37] - piCur[37] );
    uiSum += abs( piOrg[38] - piCur[38] );
    uiSum += abs( piOrg[39] - piCur[39] );
    uiSum += abs( piOrg[40] - piCur[40] );
    uiSum += abs( piOrg[41] - piCur[41] );
    uiSum += abs( piOrg[42] - piCur[42] );
    uiSum += abs( piOrg[43] - piCur[43] );
    uiSum += abs( piOrg[44] - piCur[44] );
    uiSum += abs( piOrg[45] - piCur[45] );
    uiSum += abs( piOrg[46] - piCur[46] );
    uiSum += abs( piOrg[47] - piCur[47] );
    uiSum += abs( piOrg[48] - piCur[48] );
    uiSum += abs( piOrg[49] - piCur[49] );
    uiSum += abs( piOrg[50] - piCur[50] );
    uiSum += abs( piOrg[51] - piCur[51] );
    uiSum += abs( piOrg[52] - piCur[52] );
    uiSum += abs( piOrg[53] - piCur[53] );
    uiSum += abs( piOrg[54] - piCur[54] );
    uiSum += abs( piOrg[55] - piCur[55] );
    uiSum += abs( piOrg[56] - piCur[56] );
    uiSum += abs( piOrg[57] - piCur[57] );
    uiSum += abs( piOrg[58] - piCur[58] );
    uiSum += abs( piOrg[59] - piCur[59] );
    uiSum += abs( piOrg[60] - piCur[60] );
    uiSum += abs( piOrg[61] - piCur[61] );
    uiSum += abs( piOrg[62] - piCur[62] );
    uiSum += abs( piOrg[63] - piCur[63] );

    piOrg += iStrideOrg;
    piCur += iStrideCur;
  }

  uiSum <<= iSubShift;
  return ( uiSum >> g_uiBitIncrement );
}

// --------------------------------------------------------------------------------------------------------------------
// SAD with step (used in fractional search)
// --------------------------------------------------------------------------------------------------------------------

UInt TComRdCostWeightPrediction::xGetSADsw( DistParam* pcDtParam )
{
  Pel* piOrg   = pcDtParam->pOrg;
  Pel* piCur   = pcDtParam->pCur;
  Int  iRows   = pcDtParam->iRows;
  Int  iCols   = pcDtParam->iCols;
  Int  iStrideCur = pcDtParam->iStrideCur;
  Int  iStrideOrg = pcDtParam->iStrideOrg;
  Int  iStep  = pcDtParam->iStep;

  UInt uiSum = 0;

  printf("TComRdCostWeightPrediction::xGetSADsw():\tnot implemented, use xGetSADw().\n"); fflush(stdout); exit(0);

  for( ; iRows != 0; iRows-- )
  {
    for (Int n = 0; n < iCols; n++ )
    {
      uiSum += abs( piOrg[n] - piCur[n*iStep] );
    }
    piOrg += iStrideOrg;
    piCur += iStrideCur;
  }

  return ( uiSum >> g_uiBitIncrement );
}

UInt TComRdCostWeightPrediction::xGetSADs4w( DistParam* pcDtParam )
{
  Pel* piOrg   = pcDtParam->pOrg;
  Pel* piCur   = pcDtParam->pCur;
  Int  iRows   = pcDtParam->iRows;
  Int  iStrideCur = pcDtParam->iStrideCur;
  Int  iStrideOrg = pcDtParam->iStrideOrg;
  Int  iStep  = pcDtParam->iStep;
  Int  iStep2 = iStep<<1;
  Int  iStep3 = iStep2 + iStep;

  UInt uiSum = 0;

  printf("TComRdCostWeightPrediction::xGetSADs4w():\tnot implemented, use xGetSADw().\n"); fflush(stdout); exit(0);

  for( ; iRows != 0; iRows-- )
  {
    uiSum += abs( piOrg[0] - piCur[     0] );
    uiSum += abs( piOrg[1] - piCur[iStep ] );
    uiSum += abs( piOrg[2] - piCur[iStep2] );
    uiSum += abs( piOrg[3] - piCur[iStep3] );

    piOrg += iStrideOrg;
    piCur += iStrideCur;
  }

  return ( uiSum >> g_uiBitIncrement );
}

UInt TComRdCostWeightPrediction::xGetSADs8w( DistParam* pcDtParam )
{
  Pel* piOrg   = pcDtParam->pOrg;
  Pel* piCur   = pcDtParam->pCur;
  Int  iRows   = pcDtParam->iRows;
  Int  iStrideCur = pcDtParam->iStrideCur;
  Int  iStrideOrg = pcDtParam->iStrideOrg;
  Int  iStep  = pcDtParam->iStep;
  Int  iStep2 = iStep<<1;
  Int  iStep3 = iStep2 + iStep;
  Int  iStep4 = iStep3 + iStep;
  Int  iStep5 = iStep4 + iStep;
  Int  iStep6 = iStep5 + iStep;
  Int  iStep7 = iStep6 + iStep;

  UInt uiSum = 0;

  printf("TComRdCostWeightPrediction::xGetSADs8w():\tnot implemented, use xGetSADw().\n"); fflush(stdout); exit(0);

  for( ; iRows != 0; iRows-- )
  {
    uiSum += abs( piOrg[0] - piCur[     0] );
    uiSum += abs( piOrg[1] - piCur[iStep ] );
    uiSum += abs( piOrg[2] - piCur[iStep2] );
    uiSum += abs( piOrg[3] - piCur[iStep3] );
    uiSum += abs( piOrg[4] - piCur[iStep4] );
    uiSum += abs( piOrg[5] - piCur[iStep5] );
    uiSum += abs( piOrg[6] - piCur[iStep6] );
    uiSum += abs( piOrg[7] - piCur[iStep7] );

    piOrg += iStrideOrg;
    piCur += iStrideCur;
  }

  return ( uiSum >> g_uiBitIncrement );
}

UInt TComRdCostWeightPrediction::xGetSADs16w( DistParam* pcDtParam )
{
  Pel* piOrg   = pcDtParam->pOrg;
  Pel* piCur   = pcDtParam->pCur;
  Int  iRows   = pcDtParam->iRows;
  Int  iStrideCur = pcDtParam->iStrideCur;
  Int  iStrideOrg = pcDtParam->iStrideOrg;
  Int  iStep   = pcDtParam->iStep;
  Int  iStep2  = iStep<<1;
  Int  iStep3  = iStep2  + iStep;
  Int  iStep4  = iStep3  + iStep;
  Int  iStep5  = iStep4  + iStep;
  Int  iStep6  = iStep5  + iStep;
  Int  iStep7  = iStep6  + iStep;
  Int  iStep8  = iStep7  + iStep;
  Int  iStep9  = iStep8  + iStep;
  Int  iStep10 = iStep9  + iStep;
  Int  iStep11 = iStep10 + iStep;
  Int  iStep12 = iStep11 + iStep;
  Int  iStep13 = iStep12 + iStep;
  Int  iStep14 = iStep13 + iStep;
  Int  iStep15 = iStep14 + iStep;

  UInt uiSum = 0;

  printf("TComRdCostWeightPrediction::xGetSADs16w():\tnot implemented, use xGetSADw().\n"); fflush(stdout); exit(0);

  for( ; iRows != 0; iRows-- )
  {
    uiSum += abs( piOrg[ 0] - piCur[      0] );
    uiSum += abs( piOrg[ 1] - piCur[iStep  ] );
    uiSum += abs( piOrg[ 2] - piCur[iStep2 ] );
    uiSum += abs( piOrg[ 3] - piCur[iStep3 ] );
    uiSum += abs( piOrg[ 4] - piCur[iStep4 ] );
    uiSum += abs( piOrg[ 5] - piCur[iStep5 ] );
    uiSum += abs( piOrg[ 6] - piCur[iStep6 ] );
    uiSum += abs( piOrg[ 7] - piCur[iStep7 ] );
    uiSum += abs( piOrg[ 8] - piCur[iStep8 ] );
    uiSum += abs( piOrg[ 9] - piCur[iStep9 ] );
    uiSum += abs( piOrg[10] - piCur[iStep10] );
    uiSum += abs( piOrg[11] - piCur[iStep11] );
    uiSum += abs( piOrg[12] - piCur[iStep12] );
    uiSum += abs( piOrg[13] - piCur[iStep13] );
    uiSum += abs( piOrg[14] - piCur[iStep14] );
    uiSum += abs( piOrg[15] - piCur[iStep15] );

    piOrg += iStrideOrg;
    piCur += iStrideCur;
  }

  return ( uiSum >> g_uiBitIncrement );
}

UInt TComRdCostWeightPrediction::xGetSADs16Nw( DistParam* pcDtParam )
{
  Pel* piOrg   = pcDtParam->pOrg;
  Pel* piCur   = pcDtParam->pCur;
  Int  iRows   = pcDtParam->iRows;
  Int  iCols   = pcDtParam->iCols;
  Int  iStrideCur = pcDtParam->iStrideCur;
  Int  iStrideOrg = pcDtParam->iStrideOrg;
  Int  iStep  = pcDtParam->iStep;

  UInt uiSum = 0;

  printf("TComRdCostWeightPrediction::xGetSADs16Nw():\tnot implemented, use xGetSADw().\n"); fflush(stdout); exit(0);

  for( ; iRows != 0; iRows-- )
  {
    for (Int n = 0; n < iCols; n+=16 )
    {
      uiSum += abs( piOrg[n +0] - piCur[iStep*(n +0)] );
      uiSum += abs( piOrg[n +1] - piCur[iStep*(n +1)] );
      uiSum += abs( piOrg[n +2] - piCur[iStep*(n +2)] );
      uiSum += abs( piOrg[n +3] - piCur[iStep*(n +3)] );
      uiSum += abs( piOrg[n +4] - piCur[iStep*(n +4)] );
      uiSum += abs( piOrg[n +5] - piCur[iStep*(n +5)] );
      uiSum += abs( piOrg[n +6] - piCur[iStep*(n +6)] );
      uiSum += abs( piOrg[n +7] - piCur[iStep*(n +7)] );
      uiSum += abs( piOrg[n +8] - piCur[iStep*(n +8)] );
      uiSum += abs( piOrg[n +9] - piCur[iStep*(n +9)] );
      uiSum += abs( piOrg[n+10] - piCur[iStep*(n+10)] );
      uiSum += abs( piOrg[n+11] - piCur[iStep*(n+11)] );
      uiSum += abs( piOrg[n+12] - piCur[iStep*(n+12)] );
      uiSum += abs( piOrg[n+13] - piCur[iStep*(n+13)] );
      uiSum += abs( piOrg[n+14] - piCur[iStep*(n+14)] );
      uiSum += abs( piOrg[n+15] - piCur[iStep*(n+15)] );
    }
    piOrg += iStrideOrg;
    piCur += iStrideCur;
  }

  return ( uiSum >> g_uiBitIncrement );
}

UInt TComRdCostWeightPrediction::xGetSADs32w( DistParam* pcDtParam )
{
  Pel* piOrg   = pcDtParam->pOrg;
  Pel* piCur   = pcDtParam->pCur;
  Int  iRows   = pcDtParam->iRows;
  Int  iStrideCur = pcDtParam->iStrideCur;
  Int  iStrideOrg = pcDtParam->iStrideOrg;
  Int  iStep  = pcDtParam->iStep;
  Int  iStep2  = iStep<<1;
  Int  iStep3  = iStep2  + iStep;
  Int  iStep4  = iStep3  + iStep;
  Int  iStep5  = iStep4  + iStep;
  Int  iStep6  = iStep5  + iStep;
  Int  iStep7  = iStep6  + iStep;
  Int  iStep8  = iStep7  + iStep;
  Int  iStep9  = iStep8  + iStep;
  Int  iStep10 = iStep9  + iStep;
  Int  iStep11 = iStep10 + iStep;
  Int  iStep12 = iStep11 + iStep;
  Int  iStep13 = iStep12 + iStep;
  Int  iStep14 = iStep13 + iStep;
  Int  iStep15 = iStep14 + iStep;
  Int  iStep16 = iStep15 + iStep;
  Int  iStep17 = iStep16 + iStep;
  Int  iStep18 = iStep17 + iStep;
  Int  iStep19 = iStep18 + iStep;
  Int  iStep20 = iStep19 + iStep;
  Int  iStep21 = iStep20 + iStep;
  Int  iStep22 = iStep21 + iStep;
  Int  iStep23 = iStep22 + iStep;
  Int  iStep24 = iStep23 + iStep;
  Int  iStep25 = iStep24 + iStep;
  Int  iStep26 = iStep25 + iStep;
  Int  iStep27 = iStep26 + iStep;
  Int  iStep28 = iStep27 + iStep;
  Int  iStep29 = iStep28 + iStep;
  Int  iStep30 = iStep29 + iStep;
  Int  iStep31 = iStep30 + iStep;

  UInt uiSum = 0;

  printf("TComRdCostWeightPrediction::xGetSADs32w():\tnot implemented, use xGetSADw().\n"); fflush(stdout); exit(0);

  for( ; iRows != 0; iRows-- )
  {
    uiSum += abs( piOrg[ 0] - piCur[      0] );
    uiSum += abs( piOrg[ 1] - piCur[iStep  ] );
    uiSum += abs( piOrg[ 2] - piCur[iStep2 ] );
    uiSum += abs( piOrg[ 3] - piCur[iStep3 ] );
    uiSum += abs( piOrg[ 4] - piCur[iStep4 ] );
    uiSum += abs( piOrg[ 5] - piCur[iStep5 ] );
    uiSum += abs( piOrg[ 6] - piCur[iStep6 ] );
    uiSum += abs( piOrg[ 7] - piCur[iStep7 ] );
    uiSum += abs( piOrg[ 8] - piCur[iStep8 ] );
    uiSum += abs( piOrg[ 9] - piCur[iStep9 ] );
    uiSum += abs( piOrg[10] - piCur[iStep10] );
    uiSum += abs( piOrg[11] - piCur[iStep11] );
    uiSum += abs( piOrg[12] - piCur[iStep12] );
    uiSum += abs( piOrg[13] - piCur[iStep13] );
    uiSum += abs( piOrg[14] - piCur[iStep14] );
    uiSum += abs( piOrg[15] - piCur[iStep15] );
    uiSum += abs( piOrg[16] - piCur[iStep16] );
    uiSum += abs( piOrg[17] - piCur[iStep17] );
    uiSum += abs( piOrg[18] - piCur[iStep18] );
    uiSum += abs( piOrg[19] - piCur[iStep19] );
    uiSum += abs( piOrg[20] - piCur[iStep20] );
    uiSum += abs( piOrg[21] - piCur[iStep21] );
    uiSum += abs( piOrg[22] - piCur[iStep22] );
    uiSum += abs( piOrg[23] - piCur[iStep23] );
    uiSum += abs( piOrg[24] - piCur[iStep24] );
    uiSum += abs( piOrg[25] - piCur[iStep25] );
    uiSum += abs( piOrg[26] - piCur[iStep26] );
    uiSum += abs( piOrg[27] - piCur[iStep27] );
    uiSum += abs( piOrg[28] - piCur[iStep28] );
    uiSum += abs( piOrg[29] - piCur[iStep29] );
    uiSum += abs( piOrg[30] - piCur[iStep30] );
    uiSum += abs( piOrg[31] - piCur[iStep31] );

    piOrg += iStrideOrg;
    piCur += iStrideCur;
  }

  return ( uiSum >> g_uiBitIncrement );
}

UInt TComRdCostWeightPrediction::xGetSADs64w( DistParam* pcDtParam )
{
  Pel* piOrg   = pcDtParam->pOrg;
  Pel* piCur   = pcDtParam->pCur;
  Int  iRows   = pcDtParam->iRows;
  Int  iStrideCur = pcDtParam->iStrideCur;
  Int  iStrideOrg = pcDtParam->iStrideOrg;
  Int  iStep  = pcDtParam->iStep;
  Int  iStep2  = iStep<<1;
  Int  iStep3  = iStep2  + iStep;
  Int  iStep4  = iStep3  + iStep;
  Int  iStep5  = iStep4  + iStep;
  Int  iStep6  = iStep5  + iStep;
  Int  iStep7  = iStep6  + iStep;
  Int  iStep8  = iStep7  + iStep;
  Int  iStep9  = iStep8  + iStep;
  Int  iStep10 = iStep9  + iStep;
  Int  iStep11 = iStep10 + iStep;
  Int  iStep12 = iStep11 + iStep;
  Int  iStep13 = iStep12 + iStep;
  Int  iStep14 = iStep13 + iStep;
  Int  iStep15 = iStep14 + iStep;
  Int  iStep16 = iStep15 + iStep;
  Int  iStep17 = iStep16 + iStep;
  Int  iStep18 = iStep17 + iStep;
  Int  iStep19 = iStep18 + iStep;
  Int  iStep20 = iStep19 + iStep;
  Int  iStep21 = iStep20 + iStep;
  Int  iStep22 = iStep21 + iStep;
  Int  iStep23 = iStep22 + iStep;
  Int  iStep24 = iStep23 + iStep;
  Int  iStep25 = iStep24 + iStep;
  Int  iStep26 = iStep25 + iStep;
  Int  iStep27 = iStep26 + iStep;
  Int  iStep28 = iStep27 + iStep;
  Int  iStep29 = iStep28 + iStep;
  Int  iStep30 = iStep29 + iStep;
  Int  iStep31 = iStep30 + iStep;
  Int  iStep32 = iStep31 + iStep;
  Int  iStep33 = iStep32 + iStep;
  Int  iStep34 = iStep33 + iStep;
  Int  iStep35 = iStep34 + iStep;
  Int  iStep36 = iStep35 + iStep;
  Int  iStep37 = iStep36 + iStep;
  Int  iStep38 = iStep37 + iStep;
  Int  iStep39 = iStep38 + iStep;
  Int  iStep40 = iStep39 + iStep;
  Int  iStep41 = iStep40 + iStep;
  Int  iStep42 = iStep41 + iStep;
  Int  iStep43 = iStep42 + iStep;
  Int  iStep44 = iStep43 + iStep;
  Int  iStep45 = iStep44 + iStep;
  Int  iStep46 = iStep45 + iStep;
  Int  iStep47 = iStep46 + iStep;
  Int  iStep48 = iStep47 + iStep;
  Int  iStep49 = iStep48 + iStep;
  Int  iStep50 = iStep49 + iStep;
  Int  iStep51 = iStep50 + iStep;
  Int  iStep52 = iStep51 + iStep;
  Int  iStep53 = iStep52 + iStep;
  Int  iStep54 = iStep53 + iStep;
  Int  iStep55 = iStep54 + iStep;
  Int  iStep56 = iStep55 + iStep;
  Int  iStep57 = iStep56 + iStep;
  Int  iStep58 = iStep57 + iStep;
  Int  iStep59 = iStep58 + iStep;
  Int  iStep60 = iStep59 + iStep;
  Int  iStep61 = iStep60 + iStep;
  Int  iStep62 = iStep61 + iStep;
  Int  iStep63 = iStep62 + iStep;

  UInt uiSum = 0;

  printf("TComRdCostWeightPrediction::xGetSADs64w():\tnot implemented, use xGetSADw().\n"); fflush(stdout); exit(0);

  for( ; iRows != 0; iRows-- )
  {
    uiSum += abs( piOrg[ 0] - piCur[      0] );
    uiSum += abs( piOrg[ 1] - piCur[iStep  ] );
    uiSum += abs( piOrg[ 2] - piCur[iStep2 ] );
    uiSum += abs( piOrg[ 3] - piCur[iStep3 ] );
    uiSum += abs( piOrg[ 4] - piCur[iStep4 ] );
    uiSum += abs( piOrg[ 5] - piCur[iStep5 ] );
    uiSum += abs( piOrg[ 6] - piCur[iStep6 ] );
    uiSum += abs( piOrg[ 7] - piCur[iStep7 ] );
    uiSum += abs( piOrg[ 8] - piCur[iStep8 ] );
    uiSum += abs( piOrg[ 9] - piCur[iStep9 ] );
    uiSum += abs( piOrg[10] - piCur[iStep10] );
    uiSum += abs( piOrg[11] - piCur[iStep11] );
    uiSum += abs( piOrg[12] - piCur[iStep12] );
    uiSum += abs( piOrg[13] - piCur[iStep13] );
    uiSum += abs( piOrg[14] - piCur[iStep14] );
    uiSum += abs( piOrg[15] - piCur[iStep15] );
    uiSum += abs( piOrg[16] - piCur[iStep16] );
    uiSum += abs( piOrg[17] - piCur[iStep17] );
    uiSum += abs( piOrg[18] - piCur[iStep18] );
    uiSum += abs( piOrg[19] - piCur[iStep19] );
    uiSum += abs( piOrg[20] - piCur[iStep20] );
    uiSum += abs( piOrg[21] - piCur[iStep21] );
    uiSum += abs( piOrg[22] - piCur[iStep22] );
    uiSum += abs( piOrg[23] - piCur[iStep23] );
    uiSum += abs( piOrg[24] - piCur[iStep24] );
    uiSum += abs( piOrg[25] - piCur[iStep25] );
    uiSum += abs( piOrg[26] - piCur[iStep26] );
    uiSum += abs( piOrg[27] - piCur[iStep27] );
    uiSum += abs( piOrg[28] - piCur[iStep28] );
    uiSum += abs( piOrg[29] - piCur[iStep29] );
    uiSum += abs( piOrg[30] - piCur[iStep30] );
    uiSum += abs( piOrg[31] - piCur[iStep31] );
    uiSum += abs( piOrg[32] - piCur[iStep32] );
    uiSum += abs( piOrg[33] - piCur[iStep33] );
    uiSum += abs( piOrg[34] - piCur[iStep34] );
    uiSum += abs( piOrg[35] - piCur[iStep35] );
    uiSum += abs( piOrg[36] - piCur[iStep36] );
    uiSum += abs( piOrg[37] - piCur[iStep37] );
    uiSum += abs( piOrg[38] - piCur[iStep38] );
    uiSum += abs( piOrg[39] - piCur[iStep39] );
    uiSum += abs( piOrg[40] - piCur[iStep40] );
    uiSum += abs( piOrg[41] - piCur[iStep41] );
    uiSum += abs( piOrg[42] - piCur[iStep42] );
    uiSum += abs( piOrg[43] - piCur[iStep43] );
    uiSum += abs( piOrg[44] - piCur[iStep44] );
    uiSum += abs( piOrg[45] - piCur[iStep45] );
    uiSum += abs( piOrg[46] - piCur[iStep46] );
    uiSum += abs( piOrg[47] - piCur[iStep47] );
    uiSum += abs( piOrg[48] - piCur[iStep48] );
    uiSum += abs( piOrg[49] - piCur[iStep49] );
    uiSum += abs( piOrg[50] - piCur[iStep50] );
    uiSum += abs( piOrg[51] - piCur[iStep51] );
    uiSum += abs( piOrg[52] - piCur[iStep52] );
    uiSum += abs( piOrg[53] - piCur[iStep53] );
    uiSum += abs( piOrg[54] - piCur[iStep54] );
    uiSum += abs( piOrg[55] - piCur[iStep55] );
    uiSum += abs( piOrg[56] - piCur[iStep56] );
    uiSum += abs( piOrg[57] - piCur[iStep57] );
    uiSum += abs( piOrg[58] - piCur[iStep58] );
    uiSum += abs( piOrg[59] - piCur[iStep59] );
    uiSum += abs( piOrg[60] - piCur[iStep60] );
    uiSum += abs( piOrg[61] - piCur[iStep61] );
    uiSum += abs( piOrg[62] - piCur[iStep62] );
    uiSum += abs( piOrg[63] - piCur[iStep63] );

    piOrg += iStrideOrg;
    piCur += iStrideCur;
  }

  return ( uiSum >> g_uiBitIncrement );
}

// --------------------------------------------------------------------------------------------------------------------
// SSE
// --------------------------------------------------------------------------------------------------------------------

UInt TComRdCostWeightPrediction::xGetSSEw( DistParam* pcDtParam )
{
  Pel* piOrg   = pcDtParam->pOrg;
  Pel* piCur   = pcDtParam->pCur;
  Pel  pred;
  Int  iRows   = pcDtParam->iRows;
  Int  iCols   = pcDtParam->iCols;
  Int  iStrideOrg = pcDtParam->iStrideOrg;
  Int  iStrideCur = pcDtParam->iStrideCur;

  assert( pcDtParam->iSubShift == 0 );

  UInt            uiComp    = pcDtParam->uiComp;
  assert(uiComp<3);
  wpScalingParam  *wpCur    = &(pcDtParam->wpCur[uiComp]);
  Int   w0      = wpCur->w,
        offset  = wpCur->offset,
        shift   = wpCur->shift,
        round   = wpCur->round;

  UInt uiSum = 0;
  UInt uiShift = g_uiBitIncrement<<1;

  Int iTemp;

  for( ; iRows != 0; iRows-- )
  {
    for (Int n = 0; n < iCols; n++ )
    {
      pred = ( (w0*piCur[n] + round) >> shift ) + offset ;

      //iTemp = piOrg[n  ] - piCur[n  ];
      iTemp = piOrg[n  ] - pred;
      uiSum += ( iTemp * iTemp ) >> uiShift;
    }
    piOrg += iStrideOrg;
    piCur += iStrideCur;
  }

  pcDtParam->uiComp = 255;  // reset for DEBUG (assert test)

  return ( uiSum );
}

UInt TComRdCostWeightPrediction::xGetSSE4w( DistParam* pcDtParam )
{
  Pel* piOrg   = pcDtParam->pOrg;
  Pel* piCur   = pcDtParam->pCur;
  Int  iRows   = pcDtParam->iRows;
  Int  iStrideOrg = pcDtParam->iStrideOrg;
  Int  iStrideCur = pcDtParam->iStrideCur;

  UInt uiSum = 0;
  UInt uiShift = g_uiBitIncrement<<1;

  Int  iTemp;

  printf("TComRdCostWeightPrediction::xGetSSE4w():\tnot implemented, use xGetSSEw().\n"); fflush(stdout); exit(0);

  for( ; iRows != 0; iRows-- )
  {

    iTemp = piOrg[0] - piCur[0]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[1] - piCur[1]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[2] - piCur[2]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[3] - piCur[3]; uiSum += ( iTemp * iTemp ) >> uiShift;

    piOrg += iStrideOrg;
    piCur += iStrideCur;
  }

  return ( uiSum );
}

UInt TComRdCostWeightPrediction::xGetSSE8w( DistParam* pcDtParam )
{
  Pel* piOrg   = pcDtParam->pOrg;
  Pel* piCur   = pcDtParam->pCur;
  Int  iRows   = pcDtParam->iRows;
  Int  iStrideOrg = pcDtParam->iStrideOrg;
  Int  iStrideCur = pcDtParam->iStrideCur;

  UInt uiSum = 0;
  UInt uiShift = g_uiBitIncrement<<1;

  Int  iTemp;

  printf("TComRdCostWeightPrediction::xGetSSE8w():\tnot implemented, use xGetSSEw().\n"); fflush(stdout); exit(0);

  for( ; iRows != 0; iRows-- )
  {
    iTemp = piOrg[0] - piCur[0]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[1] - piCur[1]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[2] - piCur[2]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[3] - piCur[3]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[4] - piCur[4]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[5] - piCur[5]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[6] - piCur[6]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[7] - piCur[7]; uiSum += ( iTemp * iTemp ) >> uiShift;

    piOrg += iStrideOrg;
    piCur += iStrideCur;
  }

  return ( uiSum );
}

UInt TComRdCostWeightPrediction::xGetSSE16w( DistParam* pcDtParam )
{
  Pel* piOrg   = pcDtParam->pOrg;
  Pel* piCur   = pcDtParam->pCur;
  Int  iRows   = pcDtParam->iRows;
  Int  iStrideOrg = pcDtParam->iStrideOrg;
  Int  iStrideCur = pcDtParam->iStrideCur;

  UInt uiSum = 0;
  UInt uiShift = g_uiBitIncrement<<1;

  Int  iTemp;

  printf("TComRdCostWeightPrediction::xGetSSE16w():\tnot implemented, use xGetSSEw().\n"); fflush(stdout); exit(0);

  for( ; iRows != 0; iRows-- )
  {

    iTemp = piOrg[ 0] - piCur[ 0]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[ 1] - piCur[ 1]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[ 2] - piCur[ 2]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[ 3] - piCur[ 3]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[ 4] - piCur[ 4]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[ 5] - piCur[ 5]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[ 6] - piCur[ 6]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[ 7] - piCur[ 7]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[ 8] - piCur[ 8]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[ 9] - piCur[ 9]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[10] - piCur[10]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[11] - piCur[11]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[12] - piCur[12]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[13] - piCur[13]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[14] - piCur[14]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[15] - piCur[15]; uiSum += ( iTemp * iTemp ) >> uiShift;

    piOrg += iStrideOrg;
    piCur += iStrideCur;
  }

  return ( uiSum );
}

UInt TComRdCostWeightPrediction::xGetSSE16Nw( DistParam* pcDtParam )
{
  Pel* piOrg   = pcDtParam->pOrg;
  Pel* piCur   = pcDtParam->pCur;
  Int  iRows   = pcDtParam->iRows;
  Int  iCols   = pcDtParam->iCols;
  Int  iStrideOrg = pcDtParam->iStrideOrg;
  Int  iStrideCur = pcDtParam->iStrideCur;

  UInt uiSum = 0;
  UInt uiShift = g_uiBitIncrement<<1;
  Int  iTemp;

  printf("TComRdCostWeightPrediction::xGetSSE16Nw():\tnot implemented, use xGetSSEw().\n"); fflush(stdout); exit(0);

  for( ; iRows != 0; iRows-- )
  {
    for (Int n = 0; n < iCols; n+=16 )
    {

      iTemp = piOrg[n+ 0] - piCur[n+ 0]; uiSum += ( iTemp * iTemp ) >> uiShift;
      iTemp = piOrg[n+ 1] - piCur[n+ 1]; uiSum += ( iTemp * iTemp ) >> uiShift;
      iTemp = piOrg[n+ 2] - piCur[n+ 2]; uiSum += ( iTemp * iTemp ) >> uiShift;
      iTemp = piOrg[n+ 3] - piCur[n+ 3]; uiSum += ( iTemp * iTemp ) >> uiShift;
      iTemp = piOrg[n+ 4] - piCur[n+ 4]; uiSum += ( iTemp * iTemp ) >> uiShift;
      iTemp = piOrg[n+ 5] - piCur[n+ 5]; uiSum += ( iTemp * iTemp ) >> uiShift;
      iTemp = piOrg[n+ 6] - piCur[n+ 6]; uiSum += ( iTemp * iTemp ) >> uiShift;
      iTemp = piOrg[n+ 7] - piCur[n+ 7]; uiSum += ( iTemp * iTemp ) >> uiShift;
      iTemp = piOrg[n+ 8] - piCur[n+ 8]; uiSum += ( iTemp * iTemp ) >> uiShift;
      iTemp = piOrg[n+ 9] - piCur[n+ 9]; uiSum += ( iTemp * iTemp ) >> uiShift;
      iTemp = piOrg[n+10] - piCur[n+10]; uiSum += ( iTemp * iTemp ) >> uiShift;
      iTemp = piOrg[n+11] - piCur[n+11]; uiSum += ( iTemp * iTemp ) >> uiShift;
      iTemp = piOrg[n+12] - piCur[n+12]; uiSum += ( iTemp * iTemp ) >> uiShift;
      iTemp = piOrg[n+13] - piCur[n+13]; uiSum += ( iTemp * iTemp ) >> uiShift;
      iTemp = piOrg[n+14] - piCur[n+14]; uiSum += ( iTemp * iTemp ) >> uiShift;
      iTemp = piOrg[n+15] - piCur[n+15]; uiSum += ( iTemp * iTemp ) >> uiShift;

    }
    piOrg += iStrideOrg;
    piCur += iStrideCur;
  }

  return ( uiSum );
}

UInt TComRdCostWeightPrediction::xGetSSE32w( DistParam* pcDtParam )
{
  Pel* piOrg   = pcDtParam->pOrg;
  Pel* piCur   = pcDtParam->pCur;
  Int  iRows   = pcDtParam->iRows;
  Int  iStrideOrg = pcDtParam->iStrideOrg;
  Int  iStrideCur = pcDtParam->iStrideCur;

  UInt uiSum = 0;
  UInt uiShift = g_uiBitIncrement<<1;
  Int  iTemp;

  printf("TComRdCostWeightPrediction::xGetSSE32w():\tnot implemented, use xGetSSEw().\n"); fflush(stdout); exit(0);

  for( ; iRows != 0; iRows-- )
  {

    iTemp = piOrg[ 0] - piCur[ 0]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[ 1] - piCur[ 1]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[ 2] - piCur[ 2]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[ 3] - piCur[ 3]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[ 4] - piCur[ 4]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[ 5] - piCur[ 5]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[ 6] - piCur[ 6]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[ 7] - piCur[ 7]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[ 8] - piCur[ 8]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[ 9] - piCur[ 9]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[10] - piCur[10]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[11] - piCur[11]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[12] - piCur[12]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[13] - piCur[13]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[14] - piCur[14]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[15] - piCur[15]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[16] - piCur[16]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[17] - piCur[17]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[18] - piCur[18]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[19] - piCur[19]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[20] - piCur[20]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[21] - piCur[21]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[22] - piCur[22]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[23] - piCur[23]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[24] - piCur[24]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[25] - piCur[25]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[26] - piCur[26]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[27] - piCur[27]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[28] - piCur[28]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[29] - piCur[29]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[30] - piCur[30]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[31] - piCur[31]; uiSum += ( iTemp * iTemp ) >> uiShift;

    piOrg += iStrideOrg;
    piCur += iStrideCur;
  }

  return ( uiSum );
}

UInt TComRdCostWeightPrediction::xGetSSE64w( DistParam* pcDtParam )
{
  Pel* piOrg   = pcDtParam->pOrg;
  Pel* piCur   = pcDtParam->pCur;
  Int  iRows   = pcDtParam->iRows;
  Int  iStrideOrg = pcDtParam->iStrideOrg;
  Int  iStrideCur = pcDtParam->iStrideCur;

  UInt uiSum = 0;
  UInt uiShift = g_uiBitIncrement<<1;
  Int  iTemp;

  printf("TComRdCostWeightPrediction::xGetSSE64w():\tnot implemented, use xGetSSEw().\n"); fflush(stdout); exit(0);

  for( ; iRows != 0; iRows-- )
  {
    iTemp = piOrg[ 0] - piCur[ 0]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[ 1] - piCur[ 1]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[ 2] - piCur[ 2]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[ 3] - piCur[ 3]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[ 4] - piCur[ 4]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[ 5] - piCur[ 5]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[ 6] - piCur[ 6]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[ 7] - piCur[ 7]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[ 8] - piCur[ 8]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[ 9] - piCur[ 9]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[10] - piCur[10]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[11] - piCur[11]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[12] - piCur[12]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[13] - piCur[13]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[14] - piCur[14]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[15] - piCur[15]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[16] - piCur[16]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[17] - piCur[17]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[18] - piCur[18]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[19] - piCur[19]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[20] - piCur[20]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[21] - piCur[21]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[22] - piCur[22]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[23] - piCur[23]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[24] - piCur[24]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[25] - piCur[25]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[26] - piCur[26]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[27] - piCur[27]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[28] - piCur[28]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[29] - piCur[29]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[30] - piCur[30]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[31] - piCur[31]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[32] - piCur[32]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[33] - piCur[33]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[34] - piCur[34]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[35] - piCur[35]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[36] - piCur[36]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[37] - piCur[37]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[38] - piCur[38]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[39] - piCur[39]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[40] - piCur[40]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[41] - piCur[41]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[42] - piCur[42]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[43] - piCur[43]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[44] - piCur[44]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[45] - piCur[45]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[46] - piCur[46]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[47] - piCur[47]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[48] - piCur[48]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[49] - piCur[49]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[50] - piCur[50]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[51] - piCur[51]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[52] - piCur[52]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[53] - piCur[53]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[54] - piCur[54]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[55] - piCur[55]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[56] - piCur[56]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[57] - piCur[57]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[58] - piCur[58]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[59] - piCur[59]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[60] - piCur[60]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[61] - piCur[61]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[62] - piCur[62]; uiSum += ( iTemp * iTemp ) >> uiShift;
    iTemp = piOrg[63] - piCur[63]; uiSum += ( iTemp * iTemp ) >> uiShift;

    piOrg += iStrideOrg;
    piCur += iStrideCur;
  }

  return ( uiSum );
}

// --------------------------------------------------------------------------------------------------------------------
// HADAMARD with step (used in fractional search)
// --------------------------------------------------------------------------------------------------------------------

UInt TComRdCostWeightPrediction::xCalcHADs2x2w( Pel *piOrg, Pel *piCur, Int iStrideOrg, Int iStrideCur, Int iStep )
{
  Int satd = 0, diff[4], m[4];
  diff[0] = piOrg[0             ] - piCur[0*iStep];
  diff[1] = piOrg[1             ] - piCur[1*iStep];
  diff[2] = piOrg[iStrideOrg    ] - piCur[0*iStep + iStrideCur];
  diff[3] = piOrg[iStrideOrg + 1] - piCur[1*iStep + iStrideCur];

  m[0] = diff[0] + diff[2];
  m[1] = diff[1] + diff[3];
  m[2] = diff[0] - diff[2];
  m[3] = diff[1] - diff[3];

  satd += abs(m[0] + m[1]);
  satd += abs(m[0] - m[1]);
  satd += abs(m[2] + m[3]);
  satd += abs(m[2] - m[3]);

  return satd;
}

UInt TComRdCostWeightPrediction::xCalcHADs4x4w( Pel *piOrg, Pel *piCur, Int iStrideOrg, Int iStrideCur, Int iStep )
{
  Int k, satd = 0, diff[16], m[16], d[16];

  assert( m_xSetDone );
  Pel   pred;

  for( k = 0; k < 16; k+=4 )
  {
/*
    diff[k+0] = piOrg[0] - piCur[0*iStep];
    diff[k+1] = piOrg[1] - piCur[1*iStep];
    diff[k+2] = piOrg[2] - piCur[2*iStep];
    diff[k+3] = piOrg[3] - piCur[3*iStep];
*/
    pred      = ( (m_w0*piCur[0*iStep] + m_round) >> m_shift ) + m_offset ;
    diff[k+0] = piOrg[0] - pred;
    pred      = ( (m_w0*piCur[1*iStep] + m_round) >> m_shift ) + m_offset ;
    diff[k+1] = piOrg[1] - pred;
    pred      = ( (m_w0*piCur[2*iStep] + m_round) >> m_shift ) + m_offset ;
    diff[k+2] = piOrg[2] - pred;
    pred      = ( (m_w0*piCur[3*iStep] + m_round) >> m_shift ) + m_offset ;
    diff[k+3] = piOrg[3] - pred;

    piCur += iStrideCur;
    piOrg += iStrideOrg;
  }

  /*===== hadamard transform =====*/
  m[ 0] = diff[ 0] + diff[12];
  m[ 1] = diff[ 1] + diff[13];
  m[ 2] = diff[ 2] + diff[14];
  m[ 3] = diff[ 3] + diff[15];
  m[ 4] = diff[ 4] + diff[ 8];
  m[ 5] = diff[ 5] + diff[ 9];
  m[ 6] = diff[ 6] + diff[10];
  m[ 7] = diff[ 7] + diff[11];
  m[ 8] = diff[ 4] - diff[ 8];
  m[ 9] = diff[ 5] - diff[ 9];
  m[10] = diff[ 6] - diff[10];
  m[11] = diff[ 7] - diff[11];
  m[12] = diff[ 0] - diff[12];
  m[13] = diff[ 1] - diff[13];
  m[14] = diff[ 2] - diff[14];
  m[15] = diff[ 3] - diff[15];

  d[ 0] = m[ 0] + m[ 4];
  d[ 1] = m[ 1] + m[ 5];
  d[ 2] = m[ 2] + m[ 6];
  d[ 3] = m[ 3] + m[ 7];
  d[ 4] = m[ 8] + m[12];
  d[ 5] = m[ 9] + m[13];
  d[ 6] = m[10] + m[14];
  d[ 7] = m[11] + m[15];
  d[ 8] = m[ 0] - m[ 4];
  d[ 9] = m[ 1] - m[ 5];
  d[10] = m[ 2] - m[ 6];
  d[11] = m[ 3] - m[ 7];
  d[12] = m[12] - m[ 8];
  d[13] = m[13] - m[ 9];
  d[14] = m[14] - m[10];
  d[15] = m[15] - m[11];

  m[ 0] = d[ 0] + d[ 3];
  m[ 1] = d[ 1] + d[ 2];
  m[ 2] = d[ 1] - d[ 2];
  m[ 3] = d[ 0] - d[ 3];
  m[ 4] = d[ 4] + d[ 7];
  m[ 5] = d[ 5] + d[ 6];
  m[ 6] = d[ 5] - d[ 6];
  m[ 7] = d[ 4] - d[ 7];
  m[ 8] = d[ 8] + d[11];
  m[ 9] = d[ 9] + d[10];
  m[10] = d[ 9] - d[10];
  m[11] = d[ 8] - d[11];
  m[12] = d[12] + d[15];
  m[13] = d[13] + d[14];
  m[14] = d[13] - d[14];
  m[15] = d[12] - d[15];

  d[ 0] = m[ 0] + m[ 1];
  d[ 1] = m[ 0] - m[ 1];
  d[ 2] = m[ 2] + m[ 3];
  d[ 3] = m[ 3] - m[ 2];
  d[ 4] = m[ 4] + m[ 5];
  d[ 5] = m[ 4] - m[ 5];
  d[ 6] = m[ 6] + m[ 7];
  d[ 7] = m[ 7] - m[ 6];
  d[ 8] = m[ 8] + m[ 9];
  d[ 9] = m[ 8] - m[ 9];
  d[10] = m[10] + m[11];
  d[11] = m[11] - m[10];
  d[12] = m[12] + m[13];
  d[13] = m[12] - m[13];
  d[14] = m[14] + m[15];
  d[15] = m[15] - m[14];

  for (k=0; k<16; ++k)
  {
    satd += abs(d[k]);
  }
  satd = ((satd+1)>>1);

  return satd;
}

UInt TComRdCostWeightPrediction::xCalcHADs8x8w( Pel *piOrg, Pel *piCur, Int iStrideOrg, Int iStrideCur, Int iStep )
{
  Int k, i, j, jj, sad=0;
  Int diff[64], m1[8][8], m2[8][8], m3[8][8];
  Int iStep2 = iStep<<1;
  Int iStep3 = iStep2 + iStep;
  Int iStep4 = iStep3 + iStep;
  Int iStep5 = iStep4 + iStep;
  Int iStep6 = iStep5 + iStep;
  Int iStep7 = iStep6 + iStep;

  assert( m_xSetDone );
  Pel   pred;

  for( k = 0; k < 64; k+=8 )
  {
/*
    diff[k+0] = piOrg[0] - piCur[     0];
    diff[k+1] = piOrg[1] - piCur[iStep ];
    diff[k+2] = piOrg[2] - piCur[iStep2];
    diff[k+3] = piOrg[3] - piCur[iStep3];
    diff[k+4] = piOrg[4] - piCur[iStep4];
    diff[k+5] = piOrg[5] - piCur[iStep5];
    diff[k+6] = piOrg[6] - piCur[iStep6];
    diff[k+7] = piOrg[7] - piCur[iStep7];
*/
    pred      = ( (m_w0*piCur[     0] + m_round) >> m_shift ) + m_offset ;
    diff[k+0] = piOrg[0] - pred;
    pred      = ( (m_w0*piCur[iStep ] + m_round) >> m_shift ) + m_offset ;
    diff[k+1] = piOrg[1] - pred;
    pred      = ( (m_w0*piCur[iStep2] + m_round) >> m_shift ) + m_offset ;
    diff[k+2] = piOrg[2] - pred;
    pred      = ( (m_w0*piCur[iStep3] + m_round) >> m_shift ) + m_offset ;
    diff[k+3] = piOrg[3] - pred;
    pred      = ( (m_w0*piCur[iStep4] + m_round) >> m_shift ) + m_offset ;
    diff[k+4] = piOrg[4] - pred;
    pred      = ( (m_w0*piCur[iStep5] + m_round) >> m_shift ) + m_offset ;
    diff[k+5] = piOrg[5] - pred;
    pred      = ( (m_w0*piCur[iStep6] + m_round) >> m_shift ) + m_offset ;
    diff[k+6] = piOrg[6] - pred;
    pred      = ( (m_w0*piCur[iStep7] + m_round) >> m_shift ) + m_offset ;
    diff[k+7] = piOrg[7] - pred;

    piCur += iStrideCur;
    piOrg += iStrideOrg;
  }

  //horizontal
  for (j=0; j < 8; j++)
  {
    jj = j << 3;
    m2[j][0] = diff[jj  ] + diff[jj+4];
    m2[j][1] = diff[jj+1] + diff[jj+5];
    m2[j][2] = diff[jj+2] + diff[jj+6];
    m2[j][3] = diff[jj+3] + diff[jj+7];
    m2[j][4] = diff[jj  ] - diff[jj+4];
    m2[j][5] = diff[jj+1] - diff[jj+5];
    m2[j][6] = diff[jj+2] - diff[jj+6];
    m2[j][7] = diff[jj+3] - diff[jj+7];

    m1[j][0] = m2[j][0] + m2[j][2];
    m1[j][1] = m2[j][1] + m2[j][3];
    m1[j][2] = m2[j][0] - m2[j][2];
    m1[j][3] = m2[j][1] - m2[j][3];
    m1[j][4] = m2[j][4] + m2[j][6];
    m1[j][5] = m2[j][5] + m2[j][7];
    m1[j][6] = m2[j][4] - m2[j][6];
    m1[j][7] = m2[j][5] - m2[j][7];

    m2[j][0] = m1[j][0] + m1[j][1];
    m2[j][1] = m1[j][0] - m1[j][1];
    m2[j][2] = m1[j][2] + m1[j][3];
    m2[j][3] = m1[j][2] - m1[j][3];
    m2[j][4] = m1[j][4] + m1[j][5];
    m2[j][5] = m1[j][4] - m1[j][5];
    m2[j][6] = m1[j][6] + m1[j][7];
    m2[j][7] = m1[j][6] - m1[j][7];
  }

  //vertical
  for (i=0; i < 8; i++)
  {
    m3[0][i] = m2[0][i] + m2[4][i];
    m3[1][i] = m2[1][i] + m2[5][i];
    m3[2][i] = m2[2][i] + m2[6][i];
    m3[3][i] = m2[3][i] + m2[7][i];
    m3[4][i] = m2[0][i] - m2[4][i];
    m3[5][i] = m2[1][i] - m2[5][i];
    m3[6][i] = m2[2][i] - m2[6][i];
    m3[7][i] = m2[3][i] - m2[7][i];

    m1[0][i] = m3[0][i] + m3[2][i];
    m1[1][i] = m3[1][i] + m3[3][i];
    m1[2][i] = m3[0][i] - m3[2][i];
    m1[3][i] = m3[1][i] - m3[3][i];
    m1[4][i] = m3[4][i] + m3[6][i];
    m1[5][i] = m3[5][i] + m3[7][i];
    m1[6][i] = m3[4][i] - m3[6][i];
    m1[7][i] = m3[5][i] - m3[7][i];

    m2[0][i] = m1[0][i] + m1[1][i];
    m2[1][i] = m1[0][i] - m1[1][i];
    m2[2][i] = m1[2][i] + m1[3][i];
    m2[3][i] = m1[2][i] - m1[3][i];
    m2[4][i] = m1[4][i] + m1[5][i];
    m2[5][i] = m1[4][i] - m1[5][i];
    m2[6][i] = m1[6][i] + m1[7][i];
    m2[7][i] = m1[6][i] - m1[7][i];
  }

  for (j=0; j < 8; j++)
  {
    for (i=0; i < 8; i++)
      sad += (abs(m2[j][i]));
  }

  sad=((sad+2)>>2);

  return sad;
}

UInt TComRdCostWeightPrediction::xGetHADs4w( DistParam* pcDtParam )
{
  Pel* piOrg   = pcDtParam->pOrg;
  Pel* piCur   = pcDtParam->pCur;
  Int  iRows   = pcDtParam->iRows;
  Int  iStrideCur = pcDtParam->iStrideCur;
  Int  iStrideOrg = pcDtParam->iStrideOrg;
  Int  iStep  = pcDtParam->iStep;
  Int  y;
  Int  iOffsetOrg = iStrideOrg<<2;
  Int  iOffsetCur = iStrideCur<<2;

  UInt uiSum = 0;

  for ( y=0; y<iRows; y+= 4 )
  {
    uiSum += xCalcHADs4x4w( piOrg, piCur, iStrideOrg, iStrideCur, iStep );
    piOrg += iOffsetOrg;
    piCur += iOffsetCur;
  }

  return ( uiSum >> g_uiBitIncrement );
}

UInt TComRdCostWeightPrediction::xGetHADs8w( DistParam* pcDtParam )
{
  Pel* piOrg   = pcDtParam->pOrg;
  Pel* piCur   = pcDtParam->pCur;
  Int  iRows   = pcDtParam->iRows;
  Int  iStrideCur = pcDtParam->iStrideCur;
  Int  iStrideOrg = pcDtParam->iStrideOrg;
  Int  iStep  = pcDtParam->iStep;
  Int  y;

  UInt uiSum = 0;

  if ( iRows == 4 )
  {
    uiSum += xCalcHADs4x4w( piOrg+0, piCur        , iStrideOrg, iStrideCur, iStep );
    uiSum += xCalcHADs4x4w( piOrg+4, piCur+4*iStep, iStrideOrg, iStrideCur, iStep );
  }
  else
  {
    Int  iOffsetOrg = iStrideOrg<<3;
    Int  iOffsetCur = iStrideCur<<3;
    for ( y=0; y<iRows; y+= 8 )
    {
      uiSum += xCalcHADs8x8w( piOrg, piCur, iStrideOrg, iStrideCur, iStep );
      piOrg += iOffsetOrg;
      piCur += iOffsetCur;
    }
  }

  return ( uiSum >> g_uiBitIncrement );
}

UInt TComRdCostWeightPrediction::xGetHADsw( DistParam* pcDtParam )
{
  Pel* piOrg   = pcDtParam->pOrg;
  Pel* piCur   = pcDtParam->pCur;
  Int  iRows   = pcDtParam->iRows;
  Int  iCols   = pcDtParam->iCols;
  Int  iStrideCur = pcDtParam->iStrideCur;
  Int  iStrideOrg = pcDtParam->iStrideOrg;
  Int  iStep  = pcDtParam->iStep;

  Int  x, y;

  UInt            uiComp    = pcDtParam->uiComp;
  assert(uiComp<3);
  wpScalingParam  *wpCur    = &(pcDtParam->wpCur[uiComp]);
  Int   w0      = wpCur->w,
        offset  = wpCur->offset,
        shift   = wpCur->shift,
        round   = wpCur->round;
  xSetWPscale(w0, 0, shift, offset, round);

  UInt uiSum = 0;

  if( ( iRows % 8 == 0) && (iCols % 8 == 0) )
  {
    Int  iOffsetOrg = iStrideOrg<<3;
    Int  iOffsetCur = iStrideCur<<3;
    for ( y=0; y<iRows; y+= 8 )
    {
      for ( x=0; x<iCols; x+= 8 )
      {
        uiSum += xCalcHADs8x8w( &piOrg[x], &piCur[x*iStep], iStrideOrg, iStrideCur, iStep );
      }
      piOrg += iOffsetOrg;
      piCur += iOffsetCur;
    }
  }
  else if( ( iRows % 4 == 0) && (iCols % 4 == 0) )
  {
    Int  iOffsetOrg = iStrideOrg<<2;
    Int  iOffsetCur = iStrideCur<<2;

    for ( y=0; y<iRows; y+= 4 )
    {
      for ( x=0; x<iCols; x+= 4 )
      {
        uiSum += xCalcHADs4x4w( &piOrg[x], &piCur[x*iStep], iStrideOrg, iStrideCur, iStep );
      }
      piOrg += iOffsetOrg;
      piCur += iOffsetCur;
    }
  }
#ifdef DCM_RDCOST_TEMP_FIX //Temporary fix since row size can be 1 or 3 for chroma (such a case does not occur under current encoder settings)
  else if( ( iRows % 2 == 0) && (iCols % 2 == 0) )
  {
    Int  iOffsetOrg = iStrideOrg<<1;
    Int  iOffsetCur = iStrideCur<<1;
#else
  else
  {
#endif
    for ( y=0; y<iRows; y+=2 )
    {
      for ( x=0; x<iCols; x+=2 )
      {
        uiSum += xCalcHADs2x2w( &piOrg[x], &piCur[x*iStep], iStrideOrg, iStrideCur, iStep );
      }
#ifdef DCM_RDCOST_TEMP_FIX //Temporary fix since we need to increment by 2*iStride instead of iStride
      piOrg += iOffsetOrg;
      piCur += iOffsetCur;
#else
      piOrg += iStrideOrg;
      piCur += iStrideCur;
#endif
    }
  }
#ifdef DCM_RDCOST_TEMP_FIX //Temporary fix to return MAX_UINT until this case is properly handled
  else
  {
    printf("xGetHADsw not supported for this dimension. Skipping computation of HAD and returning MAX_UINT\n");
    return (MAX_UINT);
  }
#endif

  m_xSetDone  = false;

  return ( uiSum >> g_uiBitIncrement );
}

#endif

