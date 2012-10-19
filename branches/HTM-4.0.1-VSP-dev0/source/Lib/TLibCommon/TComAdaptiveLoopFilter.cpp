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

/** \file     TComAdaptiveLoopFilter.cpp
    \brief    adaptive loop filter class
*/

#include "TComAdaptiveLoopFilter.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

//! \ingroup TLibCommon
//! \{


#if !LCU_SYNTAX_ALF
ALFParam::~ALFParam()
{
  if (coeff_chroma != NULL)
  {
    delete[] coeff_chroma;
  }
  if (coeffmulti)
  {
    for (Int i=0; i<NO_VAR_BINS; i++)
    {
      if (coeffmulti[i] != NULL)
      {
        delete[] coeffmulti[i];
      }
    }
    delete[] coeffmulti;
  }

  if (nbSPred != NULL)
  {
    delete[] nbSPred;
  }
  if (filterPattern != NULL)
  {
    delete[] filterPattern;
  }
}
#endif

// ====================================================================================================================
// Tables
// ====================================================================================================================

#if ALF_SINGLE_FILTER_SHAPE
Int TComAdaptiveLoopFilter::weightsShape1Sym[ALF_MAX_NUM_COEF+1] = 
{ 
              2,
              2,
           2, 2, 2,
  2, 2, 2, 2, 1, 
              1
};
Int* TComAdaptiveLoopFilter::weightsTabShapes[NUM_ALF_FILTER_SHAPE] =
{
  weightsShape1Sym
};
Int TComAdaptiveLoopFilter::m_sqrFiltLengthTab[NUM_ALF_FILTER_SHAPE] =
{
  ALF_FILTER_LEN
};
Int depthIntShape1Sym[ALF_MAX_NUM_COEF+1] = 
{
              6,
              7,
           7, 8, 7,
  5, 6, 7, 8, 9, 
              9  
};
Int* pDepthIntTabShapes[NUM_ALF_FILTER_SHAPE] =
{ 
  depthIntShape1Sym
};
#else
//Shape0: Star5x5
Int TComAdaptiveLoopFilter::weightsShape0Sym[10] = 
{
  2,    2,    2,    
  2, 2, 2,        
  2, 2, 1, 1
};


//Shape1: Cross9x9
Int TComAdaptiveLoopFilter::weightsShape1Sym[10] = 
{ 
              2,
              2,
              2,
              2,
  2, 2, 2, 2, 1, 
              1
};


Int* TComAdaptiveLoopFilter::weightsTabShapes[NUM_ALF_FILTER_SHAPE] =
{
  weightsShape0Sym, weightsShape1Sym
};

Int TComAdaptiveLoopFilter::m_sqrFiltLengthTab[NUM_ALF_FILTER_SHAPE] =
{
  9, 9
};

// Shape0
Int depthIntShape0Sym[10] = 
{
  1,    3,    1,
  3, 4, 3, 
  3, 4, 5, 5                 
};
// Shape1
Int depthIntShape1Sym[10] = 
{
              5,
              6,
              7,
              8,
  5, 6, 7, 8, 9, 
              9  
};


Int* pDepthIntTabShapes[NUM_ALF_FILTER_SHAPE] =
{ 
  depthIntShape0Sym, depthIntShape1Sym
};
#endif

#if LCU_SYNTAX_ALF
Int kTableShape1[ALF_MAX_NUM_COEF+1] = 
{      
              2,
              3,
           3, 4, 3,
  1, 2, 3, 4, 1, 
              3
};
Int* kTableTabShapes[NUM_ALF_FILTER_SHAPE] =
{ 
  kTableShape1
};
#endif

// ====================================================================================================================
// Constructor / destructor / create / destroy
// ====================================================================================================================

const AlfCUCtrlInfo& AlfCUCtrlInfo::operator= (const AlfCUCtrlInfo& src)
{
  this->cu_control_flag = src.cu_control_flag;
  this->alf_max_depth   = src.alf_max_depth;
  this->num_alf_cu_flag = src.num_alf_cu_flag;
  this->alf_cu_flag     = src.alf_cu_flag;
  return *this;
}

#if LCU_SYNTAX_ALF
/// AlfCUCtrlInfo
Void AlfCUCtrlInfo::reset()
{
  cu_control_flag= 0;
  num_alf_cu_flag= 0;
  alf_max_depth = 0;
  alf_cu_flag.clear();
}

/// ALFParam
const ALFParam& ALFParam::operator= (const ALFParam& src)
{
  if(this->componentID < 0)
  {
    this->create(src.componentID);
  }
  this->copy(src);
  return *this;
}

Void ALFParam::create(Int cID)
{
  const Int numCoef = ALF_MAX_NUM_COEF;

  this->componentID       = cID;
  this->alf_flag          = 0;
  this->filters_per_group = 1; // this value keeps 1 for chroma componenet
  this->startSecondFilter = -1;
  this->predMethod        = -1;
  this->minKStart         = -1;
  this->filterPattern     = NULL;
  this->nbSPred           = NULL;
  this->kMinTab           = NULL;
  this->coeffmulti        = NULL;
  this->filter_shape      = 0;
  this->num_coeff         = numCoef;

  switch(cID)
  {
  case ALF_Y:
    {
      this->coeffmulti = new Int*[NO_VAR_BINS];
      for(Int i=0; i< NO_VAR_BINS; i++)
      {
        this->coeffmulti[i] = new Int[numCoef];
        ::memset(this->coeffmulti[i], 0, sizeof(Int)*numCoef);
      }
      this->filterPattern = new Int[NO_VAR_BINS];
      ::memset(this->filterPattern, 0, sizeof(Int)*NO_VAR_BINS);

      this->nbSPred       = new Int[NO_VAR_BINS];
      ::memset(this->nbSPred, 0, sizeof(Int)*NO_VAR_BINS);

      this->kMinTab       = new Int[42];
    }
    break;
  case ALF_Cb:
  case ALF_Cr:
    {
      this->coeffmulti = new Int*[1];
      this->coeffmulti[0] = new Int[numCoef];
    }
    break;
  default:
    {
      printf("Not a legal component ID\n");
      assert(0);
      exit(-1);
    }
  }
}

Void ALFParam::destroy()
{
  if(this->componentID >=0)
  {
    switch(this->componentID)
    {
    case ALF_Y:
      {
        for(Int i=0; i< NO_VAR_BINS; i++)
        {
          delete[] this->coeffmulti[i];
        }
        delete[] this->coeffmulti;
        delete[] this->filterPattern;
        delete[] this->nbSPred;
        delete[] this->kMinTab;
      }
      break;
    case ALF_Cb:
    case ALF_Cr:
      {
        delete[] this->coeffmulti[0];
        delete[] this->coeffmulti;
      }
      break;
    default:
      {
        printf("Not a legal component ID\n");
        assert(0);
        exit(-1);
      }
    }

  }
}

Void ALFParam::copy(const ALFParam& src)
{
  const Int numCoef = ALF_MAX_NUM_COEF;

  this->componentID       = src.componentID;
  this->alf_flag          = src.alf_flag;
  if(this->alf_flag == 1)
  {
    this->filters_per_group = src.filters_per_group;
    this->filter_shape      = src.filter_shape;
    this->num_coeff         = src.num_coeff;

    switch(this->componentID)
    {
    case ALF_Cb:
    case ALF_Cr:
      {
        ::memcpy(this->coeffmulti[0], src.coeffmulti[0], sizeof(Int)*numCoef);
      }
      break;
    case ALF_Y:
      {
        this->startSecondFilter = src.startSecondFilter;
        this->predMethod        = src.predMethod;
        this->minKStart         = src.minKStart;
        ::memcpy(this->filterPattern, src.filterPattern, sizeof(Int)*NO_VAR_BINS);
        ::memcpy(this->nbSPred, src.nbSPred, sizeof(Int)*NO_VAR_BINS);
        ::memcpy(this->kMinTab, src.kMinTab, sizeof(Int)*42);

        for(Int i=0; i< (Int)NO_VAR_BINS; i++)
        {
          ::memcpy(this->coeffmulti[i], src.coeffmulti[i], sizeof(Int)*numCoef);
        }

      }
      break;
    default:
      {
        printf("not a legal component ID\n");
        assert(0);
        exit(-1);
      }

    }
  }
  else
  {
    //reset
    this->filters_per_group = 0;
    this->filter_shape      = 0;
    this->num_coeff         = 0;
  }
}

/// AlfUnitParam
AlfUnitParam::AlfUnitParam()
{
  this->mergeType = ALF_MERGE_DISABLED;
  this->isEnabled = false;
  this->isNewFilt = false;
  this->storedFiltIdx = -1;
  this->alfFiltParam = NULL;

}



const AlfUnitParam& AlfUnitParam::operator= (const AlfUnitParam& src)
{
  this->mergeType = src.mergeType;
  this->isEnabled = src.isEnabled;
  this->isNewFilt = src.isNewFilt;
  this->storedFiltIdx = src.storedFiltIdx;
  this->alfFiltParam = src.alfFiltParam; // just a pointer assignment and no additional memory allocation

  return *this;
}

Bool AlfUnitParam::operator == (const AlfUnitParam& cmp)
{
  if( isEnabled != cmp.isEnabled)
  {
    return false;
  }

  //enable flags are the same
  if(!isEnabled)
  {
    return true;
  }

  //both enabled
  assert(alfFiltParam->alf_flag == 1);

  const Int numCoef = ALF_MAX_NUM_COEF;
  ALFParam* cmpAlfParam = cmp.alfFiltParam;

  if(alfFiltParam->componentID == ALF_Y)
  {
    if(alfFiltParam->filters_per_group != cmpAlfParam->filters_per_group)
    {
      return false;
    }
    if(alfFiltParam->predMethod != cmpAlfParam->predMethod)
    {
      return false;
    }
    if(alfFiltParam->minKStart != cmpAlfParam->minKStart)
    {
      return false;
    }
    if(alfFiltParam->filters_per_group == 2)
    {
      if(alfFiltParam->startSecondFilter != cmpAlfParam->startSecondFilter)
      {
        return false;
      }
    }
    if (alfFiltParam->filters_per_group > 1)
    {
      for (Int i=0; i<NO_VAR_BINS; i++)
      {
        if (alfFiltParam->filterPattern[i] != cmpAlfParam->filterPattern[i])
        {
          return false;
        } 
      }
    }      
    for (Int i=0; i<alfFiltParam->filters_per_group; i++)
    {
      if (alfFiltParam->nbSPred[i] != cmpAlfParam->nbSPred[i])
      {
        return false;
      } 
    }
  }
  for(Int f=0; f< alfFiltParam->filters_per_group; f++)
  {
    for(Int i=0; i< numCoef; i++)
    {
      if(alfFiltParam->coeffmulti[f][i] != cmpAlfParam->coeffmulti[f][i])
      {
        return false;
      }
    }
  }

  return true;
}


/// AlfParamSet
Void AlfParamSet::init()
{
  destroy(); 
  create();
}

Void AlfParamSet::releaseALFParam()
{
  for(Int c=0; c< NUM_ALF_COMPONENT; c++)
  {
    if(alfUnitParam[c] != NULL)
    {
      for(Int n=0; n< numLCU; n++)
      {
        if(alfUnitParam[c][n].alfFiltParam != NULL)
        {
          delete alfUnitParam[c][n].alfFiltParam;
          alfUnitParam[c][n].alfFiltParam = NULL;
        }
      }
    }
  }

}

Void AlfParamSet::createALFParam()
{
  for(Int c=0; c< NUM_ALF_COMPONENT; c++)
  {
    for(Int n=0; n< numLCU; n++)
    {
      alfUnitParam[c][n].alfFiltParam = new ALFParam(c);
    }
  }
}

/** Create ALF parameter set
 * \param [in] width number of LCU in width
 * \param [in] height number of LCU in height
 * \param [in] num number of LCU in one picture
 */
Void AlfParamSet::create(Int width, Int height, Int num)
{
  numLCU = num;
  numLCUInHeight = height;
  numLCUInWidth  = width;

  for(Int c=0; c< NUM_ALF_COMPONENT; c++)
  {
    isEnabled[c] = false;
    isUniParam[c]= false;
    if(num !=0)
    {
      alfUnitParam[c] = new AlfUnitParam[num];
    }
    else
    {
      alfUnitParam[c] = NULL;
    }
  }

}

Void AlfParamSet::destroy()
{
  for(Int c=0; c< NUM_ALF_COMPONENT; c++)
  {
    if(alfUnitParam[c] != NULL)
    {
      delete[] alfUnitParam[c];
      alfUnitParam[c] = NULL;
    }
  }
}

#endif




TComAdaptiveLoopFilter::TComAdaptiveLoopFilter()
{
  m_pcTempPicYuv = NULL;
  m_iSGDepth     = 0;
  m_pcPic        = NULL;
  m_ppSliceAlfLCUs = NULL;
  m_pvpAlfLCU    = NULL;
  m_pvpSliceTileAlfLCU = NULL;
#if LCU_SYNTAX_ALF
  for(Int c=0; c< NUM_ALF_COMPONENT; c++)
  {
    m_alfFiltInfo[c] = NULL;
  }
  m_varImg = NULL;
  m_filterCoeffSym = NULL;
#endif

}

Void TComAdaptiveLoopFilter:: xError(const char *text, int code)
{
  fprintf(stderr, "%s\n", text);
  exit(code);
}

Void TComAdaptiveLoopFilter:: no_mem_exit(const char *where)
{
  char errortext[200];
  sprintf(errortext, "Could not allocate memory: %s",where);
  xError (errortext, 100);
}

Void TComAdaptiveLoopFilter::initMatrix_Pel(Pel ***m2D, int d1, int d2)
{
  int i;
  
  if(!(*m2D = (Pel **) calloc(d1, sizeof(Pel *))))
  {
    FATAL_ERROR_0("initMatrix_Pel: memory allocation problem\n", -1);
  }
  if(!((*m2D)[0] = (Pel *) calloc(d1 * d2, sizeof(Pel))))
  {
    FATAL_ERROR_0("initMatrix_Pel: memory allocation problem\n", -1);
  }
  
  for(i = 1; i < d1; i++)
  {
    (*m2D)[i] = (*m2D)[i-1] + d2;
  }
}

Void TComAdaptiveLoopFilter::initMatrix_int(int ***m2D, int d1, int d2)
{
  int i;
  
  if(!(*m2D = (int **) calloc(d1, sizeof(int *))))
  {
    FATAL_ERROR_0("initMatrix_int: memory allocation problem\n", -1);
  }
  if(!((*m2D)[0] = (int *) calloc(d1 * d2, sizeof(int))))
  {
    FATAL_ERROR_0("initMatrix_int: memory allocation problem\n", -1);
  }
  
  for(i = 1; i < d1; i++)
  {
    (*m2D)[i] = (*m2D)[i-1] + d2;
  }
}

Void TComAdaptiveLoopFilter::destroyMatrix_int(int **m2D)
{
  if(m2D)
  {
    if(m2D[0])
    {
      free(m2D[0]);
    }
    else
    {
      FATAL_ERROR_0("destroyMatrix_int: memory free problem\n", -1);
    }
    free(m2D);
  } 
  else
  {
    FATAL_ERROR_0("destroyMatrix_int: memory free problem\n", -1);
  }
}

Void TComAdaptiveLoopFilter::destroyMatrix_Pel(Pel **m2D)
{
  if(m2D)
  {
    if(m2D[0])
    {
      free(m2D[0]);
    }
    else
    {
      FATAL_ERROR_0("destroyMatrix_Pel: memory free problem\n", -1);
    }
    free(m2D);
  } 
  else
  {
    FATAL_ERROR_0("destroyMatrix_Pel: memory free problem\n", -1);
  }
}

Void TComAdaptiveLoopFilter::initMatrix_double(double ***m2D, int d1, int d2)
{
  int i;
  
  if(!(*m2D = (double **) calloc(d1, sizeof(double *))))
  {
    FATAL_ERROR_0("initMatrix_double: memory allocation problem\n", -1);
  }
  if(!((*m2D)[0] = (double *) calloc(d1 * d2, sizeof(double))))
  {
    FATAL_ERROR_0("initMatrix_double: memory allocation problem\n", -1);
  }
  
  for(i = 1; i < d1; i++)
  {
    (*m2D)[i] = (*m2D)[i-1] + d2;
  }
}

Void TComAdaptiveLoopFilter::initMatrix3D_double(double ****m3D, int d1, int d2, int d3)
{
  int  j;
  
  if(!((*m3D) = (double ***) calloc(d1, sizeof(double **))))
  {
    FATAL_ERROR_0("initMatrix3D_double: memory allocation problem\n", -1);
  }
  
  for(j = 0; j < d1; j++)
  {
    initMatrix_double((*m3D) + j, d2, d3);
  }
}


Void TComAdaptiveLoopFilter::initMatrix4D_double(double *****m4D, int d1, int d2, int d3, int d4)
{
  int  j;
  
  if(!((*m4D) = (double ****) calloc(d1, sizeof(double ***))))
  {
    FATAL_ERROR_0("initMatrix4D_double: memory allocation problem\n", -1);
  }
  
  for(j = 0; j < d1; j++)
  {
    initMatrix3D_double((*m4D) + j, d2, d3, d4);
  }
}


Void TComAdaptiveLoopFilter::destroyMatrix_double(double **m2D)
{
  if(m2D)
  {
    if(m2D[0])
    {
      free(m2D[0]);
    }
    else
    {
      FATAL_ERROR_0("destroyMatrix_double: memory free problem\n", -1);
    }
    free(m2D);
  } 
  else
  {
    FATAL_ERROR_0("destroyMatrix_double: memory free problem\n", -1);
  }
}

Void TComAdaptiveLoopFilter::destroyMatrix3D_double(double ***m3D, int d1)
{
  int i;
  
  if(m3D)
  {
    for(i = 0; i < d1; i++)
    {
      destroyMatrix_double(m3D[i]);
    }
    free(m3D);
  } 
  else
  {
    FATAL_ERROR_0("destroyMatrix3D_double: memory free problem\n", -1);
  }
}


Void TComAdaptiveLoopFilter::destroyMatrix4D_double(double ****m4D, int d1, int d2)
{
  int  j;
  
  if(m4D)
  {
    for(j = 0; j < d1; j++)
    {
      destroyMatrix3D_double(m4D[j], d2);
    }
    free(m4D);
  } 
  else
  {
    FATAL_ERROR_0("destroyMatrix4D_double: memory free problem\n", -1);
  }
}

Void TComAdaptiveLoopFilter::create( Int iPicWidth, Int iPicHeight, UInt uiMaxCUWidth, UInt uiMaxCUHeight, UInt uiMaxCUDepth )
{
#if LCU_SYNTAX_ALF
  destroy();
#endif
  if ( !m_pcTempPicYuv )
  {
    m_pcTempPicYuv = new TComPicYuv;
    m_pcTempPicYuv->create( iPicWidth, iPicHeight, uiMaxCUWidth, uiMaxCUHeight, uiMaxCUDepth );
  }
  m_img_height = iPicHeight;
  m_img_width = iPicWidth;
#if LCU_SYNTAX_ALF
  initMatrix_Pel(&(m_varImg), m_img_height, m_img_width);
#else
  for(Int i=0; i< NUM_ALF_CLASS_METHOD; i++)
  {
    initMatrix_Pel(&(m_varImgMethods[i]), m_img_height, m_img_width);
  }
#endif
  initMatrix_int(&m_filterCoeffSym, NO_VAR_BINS, ALF_MAX_NUM_COEF);
  UInt uiNumLCUsInWidth   = m_img_width  / uiMaxCUWidth;
  UInt uiNumLCUsInHeight  = m_img_height / uiMaxCUHeight;

  uiNumLCUsInWidth  += ( m_img_width % uiMaxCUWidth ) ? 1 : 0;
  uiNumLCUsInHeight += ( m_img_height % uiMaxCUHeight ) ? 1 : 0;

  m_uiNumCUsInFrame = uiNumLCUsInWidth* uiNumLCUsInHeight; 

#if LCU_SYNTAX_ALF
  m_numLCUInPicWidth = uiNumLCUsInWidth;
  m_numLCUInPicHeight= uiNumLCUsInHeight;
  m_lcuHeight = uiMaxCUHeight;
#if ALF_SINGLE_FILTER_SHAPE
  m_lineIdxPadBot = m_lcuHeight - 4 - 3; // DFRegion, Vertical Taps
#else
  m_lineIdxPadBot = m_lcuHeight - 4 - 4; // DFRegion, Vertical Taps
#endif
  m_lineIdxPadTop = m_lcuHeight - 4; // DFRegion

  m_lcuHeightChroma = m_lcuHeight>>1;
#if ALF_SINGLE_FILTER_SHAPE
  m_lineIdxPadBotChroma = m_lcuHeightChroma - 2 - 3; // DFRegion, Vertical Taps
#else
  m_lineIdxPadBotChroma = m_lcuHeightChroma - 2 - 4; // DFRegion, Vertical Taps
#endif
  m_lineIdxPadTopChroma = m_lcuHeightChroma - 2 ; // DFRegion

  createLCUAlfInfo();
#else
  createRegionIndexMap(m_varImgMethods[ALF_RA], m_img_width, m_img_height);
#endif
}

Void TComAdaptiveLoopFilter::destroy()
{
  if ( m_pcTempPicYuv )
  {
    m_pcTempPicYuv->destroy();
    delete m_pcTempPicYuv;
#if LCU_SYNTAX_ALF
    m_pcTempPicYuv = NULL;
#endif
  }
#if LCU_SYNTAX_ALF
  if(m_varImg != NULL)
  {
    destroyMatrix_Pel( m_varImg );
    m_varImg = NULL;
  }
  if(m_filterCoeffSym != NULL)
  {
    destroyMatrix_int(m_filterCoeffSym);
    m_filterCoeffSym = NULL;
  }
#else
  for(Int i=0; i< NUM_ALF_CLASS_METHOD; i++)
  {
    destroyMatrix_Pel(m_varImgMethods[i]);
  }
  destroyMatrix_int(m_filterCoeffSym);
#endif
#if LCU_SYNTAX_ALF
  destroyLCUAlfInfo();
#endif

}

#if !LCU_SYNTAX_ALF
// --------------------------------------------------------------------------------------------------------------------
// allocate / free / copy functions
// --------------------------------------------------------------------------------------------------------------------
Void TComAdaptiveLoopFilter::allocALFParam(ALFParam* pAlfParam)
{
  pAlfParam->alf_flag = 0;
  pAlfParam->coeff_chroma = new Int[ALF_MAX_NUM_COEF];
  ::memset(pAlfParam->coeff_chroma, 0, sizeof(Int)*ALF_MAX_NUM_COEF );
  pAlfParam->coeffmulti = new Int*[NO_VAR_BINS];
  for (int i=0; i<NO_VAR_BINS; i++)
  {
    pAlfParam->coeffmulti[i] = new Int[ALF_MAX_NUM_COEF];
    ::memset(pAlfParam->coeffmulti[i],        0, sizeof(Int)*ALF_MAX_NUM_COEF );
  }
  pAlfParam->nbSPred = new Int[NO_VAR_BINS];
  ::memset(pAlfParam->nbSPred, 0, sizeof(Int)*NO_VAR_BINS);
  pAlfParam->filterPattern = new Int[NO_VAR_BINS];
  ::memset(pAlfParam->filterPattern, 0, sizeof(Int)*NO_VAR_BINS);
  pAlfParam->alf_pcr_region_flag = 0;
}

Void TComAdaptiveLoopFilter::freeALFParam(ALFParam* pAlfParam)
{
  assert(pAlfParam != NULL);
  if (pAlfParam->coeff_chroma != NULL)
  {
    delete[] pAlfParam->coeff_chroma;
    pAlfParam->coeff_chroma = NULL;
  }
  for (int i=0; i<NO_VAR_BINS; i++)
  {
    delete[] pAlfParam->coeffmulti[i];
    pAlfParam->coeffmulti[i] = NULL;
  }
  delete[] pAlfParam->coeffmulti;
  pAlfParam->coeffmulti = NULL;

  delete[] pAlfParam->nbSPred;
  pAlfParam->nbSPred = NULL;

  delete[] pAlfParam->filterPattern;
  pAlfParam->filterPattern = NULL;
}


Void TComAdaptiveLoopFilter::copyALFParam(ALFParam* pDesAlfParam, ALFParam* pSrcAlfParam)
{
  pDesAlfParam->alf_flag = pSrcAlfParam->alf_flag;
  pDesAlfParam->chroma_idc = pSrcAlfParam->chroma_idc;
  pDesAlfParam->num_coeff = pSrcAlfParam->num_coeff;
  pDesAlfParam->filter_shape_chroma = pSrcAlfParam->filter_shape_chroma;
  pDesAlfParam->num_coeff_chroma = pSrcAlfParam->num_coeff_chroma;
  pDesAlfParam->alf_pcr_region_flag = pSrcAlfParam->alf_pcr_region_flag;
  ::memcpy(pDesAlfParam->coeff_chroma, pSrcAlfParam->coeff_chroma, sizeof(Int)*ALF_MAX_NUM_COEF);
  pDesAlfParam->filter_shape = pSrcAlfParam->filter_shape;
  ::memcpy(pDesAlfParam->filterPattern, pSrcAlfParam->filterPattern, sizeof(Int)*NO_VAR_BINS);
  pDesAlfParam->startSecondFilter = pSrcAlfParam->startSecondFilter;

  //Coeff send related
  pDesAlfParam->filters_per_group = pSrcAlfParam->filters_per_group; //this can be updated using codedVarBins
  pDesAlfParam->predMethod = pSrcAlfParam->predMethod;
  ::memcpy(pDesAlfParam->nbSPred, pSrcAlfParam->nbSPred, sizeof(Int)*NO_VAR_BINS);
  for (int i=0; i<NO_VAR_BINS; i++)
  {
    ::memcpy(pDesAlfParam->coeffmulti[i], pSrcAlfParam->coeffmulti[i], sizeof(Int)*ALF_MAX_NUM_COEF);
  }
}
// --------------------------------------------------------------------------------------------------------------------
// prediction of filter coefficients
// --------------------------------------------------------------------------------------------------------------------

Void TComAdaptiveLoopFilter::predictALFCoeffChroma( ALFParam* pAlfParam )
{
  Int i, sum, pred, N;
  const Int* pFiltMag = NULL;
  pFiltMag = weightsTabShapes[pAlfParam->filter_shape_chroma];
  N = pAlfParam->num_coeff_chroma;
  sum=0;
  for(i=0; i<N;i++)
  {
    sum+=pFiltMag[i]*pAlfParam->coeff_chroma[i];
  }
  pred=(1<<ALF_NUM_BIT_SHIFT)-(sum-pAlfParam->coeff_chroma[N-1]);
#if ALF_CHROMA_COEF_PRED_HARMONIZATION
  pAlfParam->coeff_chroma[N-1]=pAlfParam->coeff_chroma[N-1] - pred;
#else
  pAlfParam->coeff_chroma[N-1]=pred-pAlfParam->coeff_chroma[N-1];
#endif
}

#if ALF_CHROMA_COEF_PRED_HARMONIZATION
Void TComAdaptiveLoopFilter::reconstructALFCoeffChroma( ALFParam* pAlfParam )
{
  Int i, sum, pred, N;
  const Int* pFiltMag = NULL;
  pFiltMag = weightsTabShapes[pAlfParam->filter_shape_chroma];
  N = pAlfParam->num_coeff_chroma;
  sum=0;
  for(i=0; i<N;i++)
  {
    sum+=pFiltMag[i]*pAlfParam->coeff_chroma[i];
  }
  pred=(1<<ALF_NUM_BIT_SHIFT)-(sum-pAlfParam->coeff_chroma[N-1]);
  pAlfParam->coeff_chroma[N-1]=pred+ pAlfParam->coeff_chroma[N-1];
}
#endif
#endif

// --------------------------------------------------------------------------------------------------------------------
// interface function for actual ALF process
// --------------------------------------------------------------------------------------------------------------------

#if LCU_SYNTAX_ALF
/** ALF reconstruction process for one picture
 * \param [in, out] pcPic the decoded/filtered picture (input: decoded picture; output filtered picture)
 * \param [in] vAlfCUCtrlParam ALF CU-on/off control parameters
 * \param [in] isAlfCoefInSlice ALF coefficient in slice (true) or ALF coefficient in APS (false) 
 */
Void TComAdaptiveLoopFilter::ALFProcess(TComPic* pcPic, std::vector<AlfCUCtrlInfo>& vAlfCUCtrlParam, Bool isAlfCoefInSlice)
{
  TComPicYuv* pcPicYuvRec    = pcPic->getPicYuvRec();
  TComPicYuv* pcPicYuvExtRec = m_pcTempPicYuv;
  pcPicYuvRec   ->copyToPic          ( pcPicYuvExtRec );
  pcPicYuvExtRec->setBorderExtension ( false );
  pcPicYuvExtRec->extendPicBorder    ();

  Int lumaStride   = pcPicYuvExtRec->getStride();
  Int chromaStride = pcPicYuvExtRec->getCStride();

  for(Int compIdx =0; compIdx < NUM_ALF_COMPONENT; compIdx++)
  {
    Pel* pDec         = getPicBuf(pcPicYuvExtRec, compIdx);
    Pel* pRest        = getPicBuf(pcPicYuvRec, compIdx);
    Int  stride       = (compIdx == ALF_Y)?(lumaStride):(chromaStride);
    Int  formatShift = (compIdx == ALF_Y)?(0):(1);

    ALFParam** alfLCUParamComp = m_alfFiltInfo[compIdx];

    if(!isEnabledComponent(alfLCUParamComp))
    {
      continue;
    }

    switch(compIdx)
    {
    case ALF_Cb:
    case ALF_Cr:
      {
        recALF(compIdx, alfLCUParamComp, pDec, pRest, stride, formatShift, NULL, false);
      }
      break;
    case ALF_Y:
      {
        std::vector<AlfCUCtrlInfo>* alfCUCtrlParam = NULL;
        if(!isAlfCoefInSlice)
        {
          alfCUCtrlParam = &vAlfCUCtrlParam;
          assignAlfOnOffControlFlags(pcPic, *alfCUCtrlParam);
        }

        memset(&m_varImg[0][0], 0, sizeof(Pel)*(m_img_height*m_img_width));

        //calcOneRegionVar(m_varImg, pDec, stride, false, 0, m_img_height, 0, m_img_width);
        recALF(compIdx, alfLCUParamComp, pDec, pRest, stride, formatShift, alfCUCtrlParam, true);
      }
      break;
    default:
      {
        printf("Not a legal component ID for ALF\n");
        assert(0);
        exit(-1);
      }
    }
  }

}

/** Check the filter process of one component is enable
 * \param [in] alfLCUParam ALF parameters
 */
Bool TComAdaptiveLoopFilter::isEnabledComponent(ALFParam** alfLCUParam)
{
  Bool isEnabled = false;
  for(Int n=0; n< m_uiNumCUsInFrame; n++)
  {
    if(alfLCUParam[n]->alf_flag == 1)
    {
      isEnabled = true;
      break;
    }
  }
  return isEnabled;
}


/** ALF Reconstruction for each component
 * \param [in] compIdx color component index
 * \param [in] alfLCUParams alf parameters 
 * \param [in] pDec decoded picture
 * \param [in, out] pRest filtered picture
 * \param [in] stride picture stride in memory
 * \param [in] formatShift luma component (false) or chroma component (1)
 * \param [in] alfCUCtrlParam ALF CU-on/off control parameters 
 * \param [in] caculateBAIdx calculate BA filter index (true) or BA filter index array is ready (false)
 */
Void TComAdaptiveLoopFilter::recALF(Int compIdx, ALFParam** alfLCUParams,Pel* pDec, Pel* pRest, Int stride, Int formatShift
                                  , std::vector<AlfCUCtrlInfo>* alfCUCtrlParam //default: NULL
                                  , Bool caculateBAIdx //default: false
                                    )
{
  for(Int s=0; s< m_uiNumSlicesInPic; s++)
  {
    if(!m_pcPic->getValidSlice(s))
    {
      continue;
    }
    Int numTilesInSlice = (Int)m_pvpSliceTileAlfLCU[s].size();
    for(Int t=0; t< numTilesInSlice; t++)
    {
      std::vector<AlfLCUInfo*> & vpAlfLCU = m_pvpSliceTileAlfLCU[s][t];
      Pel* pSrc = pDec;

      if(m_bUseNonCrossALF)
      {
        pSrc = getPicBuf(m_pcSliceYuvTmp, compIdx);
        copyRegion(vpAlfLCU, pSrc, pDec, stride, formatShift);
        extendRegionBorder(vpAlfLCU, pSrc, stride, formatShift);
      }

      Bool isOnOffCtrlEnabled = (alfCUCtrlParam == NULL)?(false):(  (*alfCUCtrlParam)[s].cu_control_flag == 1 );

      if(isOnOffCtrlEnabled)
      {
        assert(compIdx == ALF_Y);
        filterRegionCUControl(alfLCUParams, vpAlfLCU, pSrc, pRest, stride, caculateBAIdx);
      }
      else
      {
        filterRegion(compIdx, alfLCUParams, vpAlfLCU, pSrc, pRest, stride, formatShift, caculateBAIdx);
      }
    } //tile
  } //slice
}


/** assign ALC CU-On/Off Control parameters
 * \param [in, out] pcPic picture data
 * \param [in] vAlfCUCtrlParam ALF CU-on/off control parameters
 */
Void TComAdaptiveLoopFilter::assignAlfOnOffControlFlags(TComPic* pcPic, std::vector<AlfCUCtrlInfo>& vAlfCUCtrlParam)
{
  if(m_uiNumSlicesInPic == 1)
  {
    AlfCUCtrlInfo* pcAlfCtrlParam = &(vAlfCUCtrlParam[0]);
    if(pcAlfCtrlParam->cu_control_flag)
    {
      UInt idx = 0;
      for(UInt uiCUAddr = 0; uiCUAddr < pcPic->getNumCUsInFrame(); uiCUAddr++)
      {
        TComDataCU *pcCU = pcPic->getCU(uiCUAddr);
        setAlfCtrlFlags(pcAlfCtrlParam, pcCU, 0, 0, idx);
      }
    }
  }
  else
  {
    transferCtrlFlagsFromAlfParam(vAlfCUCtrlParam);
  }

}

#else

/**
 \param [in, out] pcPic         picture (TComPic) class (input/output)
 \param [in] pcAlfParam    ALF parameter
 \param [in,out] vAlfCUCtrlParam ALF CU control parameters
 \todo   for temporal buffer, it uses residual picture buffer, which may not be safe. Make it be safer.
 */
Void TComAdaptiveLoopFilter::ALFProcess(TComPic* pcPic, ALFParam* pcAlfParam, std::vector<AlfCUCtrlInfo>& vAlfCUCtrlParam)
{
  assert(m_uiNumSlicesInPic == vAlfCUCtrlParam.size());
  if(!pcAlfParam->alf_flag)
  {
    return;
  }

  m_lcuHeight     = pcPic->getSlice(0)->getSPS()->getMaxCUHeight();
#if ALF_SINGLE_FILTER_SHAPE
  m_lineIdxPadBot = m_lcuHeight - 4 - 3;             // DFRegion, Vertical Taps
#else
  m_lineIdxPadBot = m_lcuHeight - 4 - 4;             // DFRegion, Vertical Taps
#endif
  m_lineIdxPadTop = m_lcuHeight - 4;                 // DFRegion

  m_lcuHeightChroma     = m_lcuHeight>>1;
#if ALF_SINGLE_FILTER_SHAPE
  m_lineIdxPadBotChroma = m_lcuHeightChroma - 2 - 3; // DFRegion, Vertical Taps
#else
  m_lineIdxPadBotChroma = m_lcuHeightChroma - 2 - 4; // DFRegion, Vertical Taps
#endif
  m_lineIdxPadTopChroma = m_lcuHeightChroma - 2 ;    // DFRegion

  TComPicYuv* pcPicYuvRec    = pcPic->getPicYuvRec();
  TComPicYuv* pcPicYuvExtRec = m_pcTempPicYuv;
    pcPicYuvRec   ->copyToPic          ( pcPicYuvExtRec );
    pcPicYuvExtRec->setBorderExtension ( false );
    pcPicYuvExtRec->extendPicBorder    ();

  if(m_uiNumSlicesInPic == 1)
  {
    AlfCUCtrlInfo* pcAlfCtrlParam = &(vAlfCUCtrlParam[0]);
    if(pcAlfCtrlParam->cu_control_flag)
    {
      UInt idx = 0;
      for(UInt uiCUAddr = 0; uiCUAddr < pcPic->getNumCUsInFrame(); uiCUAddr++)
      {
        TComDataCU *pcCU = pcPic->getCU(uiCUAddr);
        setAlfCtrlFlags(pcAlfCtrlParam, pcCU, 0, 0, idx);
      }
    }
  }
  else
  {
    transferCtrlFlagsFromAlfParam(vAlfCUCtrlParam);
  }
  xALFLuma(pcPic, pcAlfParam, vAlfCUCtrlParam, pcPicYuvExtRec, pcPicYuvRec);
  if(pcAlfParam->chroma_idc)
  {
#if ALF_CHROMA_COEF_PRED_HARMONIZATION
    reconstructALFCoeffChroma(pcAlfParam);
#else
    predictALFCoeffChroma(pcAlfParam);
#endif
    checkFilterCoeffValue(pcAlfParam->coeff_chroma, pcAlfParam->num_coeff_chroma, true );

    xALFChroma( pcAlfParam, pcPicYuvExtRec, pcPicYuvRec);
  }
}
#endif
// ====================================================================================================================
// Protected member functions
// ====================================================================================================================

/** 
 \param filter         filter coefficient
 \param filterLength   filter length
 \param isChroma       1: chroma, 0: luma
 */
Void TComAdaptiveLoopFilter::checkFilterCoeffValue( Int *filter, Int filterLength, Bool isChroma )
{
#if LCUALF_QP_DEPENDENT_BITS
  Int alfPrecisionBit = getAlfPrecisionBit( m_alfQP );

  Int maxValueNonCenter = 1 * (1 << alfPrecisionBit) - 1;
  Int minValueNonCenter = 0 - 1 * (1 << alfPrecisionBit);

  Int maxValueCenter    = 2 * (1 << alfPrecisionBit) - 1;
#else
  Int maxValueNonCenter = 1 * (1 << ALF_NUM_BIT_SHIFT) - 1;
  Int minValueNonCenter = 0 - 1 * (1 << ALF_NUM_BIT_SHIFT);

  Int maxValueCenter    = 2 * (1 << ALF_NUM_BIT_SHIFT) - 1;
#endif
  Int minValueCenter    = 0 ; 

  for(Int i = 0; i < filterLength-1; i++)
  {
    filter[i] = Clip3(minValueNonCenter, maxValueNonCenter, filter[i]);
  }

  filter[filterLength-1] = Clip3(minValueCenter, maxValueCenter, filter[filterLength-1]);
}

#if !LCU_SYNTAX_ALF

Void TComAdaptiveLoopFilter::xALFLuma(TComPic* pcPic, ALFParam* pcAlfParam, std::vector<AlfCUCtrlInfo>& vAlfCUCtrlParam,TComPicYuv* pcPicDec, TComPicYuv* pcPicRest)
{
  Int    LumaStride = pcPicDec->getStride();
  Pel* pDec = pcPicDec->getLumaAddr();
  Pel* pRest = pcPicRest->getLumaAddr();

  decodeFilterSet(pcAlfParam, m_varIndTab, m_filterCoeffSym);

  m_uiVarGenMethod = pcAlfParam->alf_pcr_region_flag;
  m_varImg         = m_varImgMethods[m_uiVarGenMethod];
  calcVar(m_varImg, pRest, LumaStride, pcAlfParam->alf_pcr_region_flag);

  if(!m_bUseNonCrossALF)
  {
    Bool bCUCtrlEnabled = false;
    for(UInt s=0; s< m_uiNumSlicesInPic; s++)
    {
      if(!pcPic->getValidSlice(s))
      {
        continue;
      }
      if( vAlfCUCtrlParam[s].cu_control_flag == 1)
      {
        bCUCtrlEnabled = true;
      }
    }

    if(bCUCtrlEnabled)  
    {
      xCUAdaptive(pcPic, pcAlfParam->filter_shape, pRest, pDec, LumaStride);
    }  
    else
    {
      filterLuma(pRest, pDec, LumaStride, 0, m_img_height-1, 0, m_img_width-1,  pcAlfParam->filter_shape, m_filterCoeffSym, m_varIndTab, m_varImg);
    }
  }
  else
  {
    Pel* pTemp = m_pcSliceYuvTmp->getLumaAddr();
    for(UInt s=0; s< m_uiNumSlicesInPic; s++)
    {
      if(!pcPic->getValidSlice(s)) 
      {
        continue;
      }
      std::vector< std::vector<AlfLCUInfo*> > & vpSliceTileAlfLCU = m_pvpSliceTileAlfLCU[s];

      for(Int t=0; t< (Int)vpSliceTileAlfLCU.size(); t++)
      {
        std::vector<AlfLCUInfo*> & vpAlfLCU = vpSliceTileAlfLCU[t];

        copyRegion(vpAlfLCU, pTemp, pDec, LumaStride);
        extendRegionBorder(vpAlfLCU, pTemp, LumaStride);
        if(vAlfCUCtrlParam[s].cu_control_flag == 1)
        {
          xCUAdaptiveRegion(vpAlfLCU, pTemp, pRest, LumaStride, pcAlfParam->filter_shape, m_filterCoeffSym, m_varIndTab, m_varImg);
        }
        else
        {
          filterLumaRegion(vpAlfLCU, pTemp, pRest, LumaStride, pcAlfParam->filter_shape, m_filterCoeffSym, m_varIndTab, m_varImg);
        }
      }
    }
  }
}

Void TComAdaptiveLoopFilter::decodeFilterSet(ALFParam* pcAlfParam, Int* varIndTab, Int** filterCoeff)
{
  // reconstruct merge table
  memset(m_varIndTab, 0, NO_VAR_BINS * sizeof(Int));
  if(pcAlfParam->filters_per_group > 1)
  {
    for(Int i = 1; i < NO_VAR_BINS; ++i)
    {
      if(pcAlfParam->filterPattern[i])
      {
        varIndTab[i] = varIndTab[i-1] + 1;
      }
      else
      {
        varIndTab[i] = varIndTab[i-1];
      }
    }
  }
  predictALFCoeffLuma( pcAlfParam);
  // reconstruct filter sets
  reconstructFilterCoeffs( pcAlfParam, filterCoeff);

}


Void TComAdaptiveLoopFilter::filterLuma(Pel *pImgRes, Pel *pImgPad, Int stride, 
  Int ypos, Int yposEnd, Int xpos, Int xposEnd, 
  Int filtNo, Int** filterSet, Int* mergeTable, Pel** ppVarImg)
{
  static Int numBitsMinus1= (Int)ALF_NUM_BIT_SHIFT;
  static Int offset       = (1<<( (Int)ALF_NUM_BIT_SHIFT-1));
  static Int shiftHeight  = (Int)(log((double)VAR_SIZE_H)/log(2.0));
  static Int shiftWidth   = (Int)(log((double)VAR_SIZE_W)/log(2.0));

  Pel *pImgPad1,*pImgPad2,*pImgPad3,*pImgPad4;
  Pel *pVar;
  Int i, j, pixelInt;
  Int *coef = NULL;

  pImgPad    += (ypos*stride);
  pImgRes    += (ypos*stride);

  Int yLineInLCU;
  Int paddingLine;
#if !ALF_SINGLE_FILTER_SHAPE
  Int varInd = 0;
#endif
  Int newCenterCoeff[4][NO_VAR_BINS];

  for(i=0; i< 4; i++)
  {
    ::memset(&(newCenterCoeff[i][0]), 0, sizeof(Int)*NO_VAR_BINS);
  }

#if ALF_SINGLE_FILTER_SHAPE
  if(filtNo == ALF_CROSS9x7_SQUARE3x3)
#else
  if(filtNo == ALF_CROSS9x9)
#endif
  {
    for (i=0; i<NO_VAR_BINS; i++)
    {
      coef = filterSet[i];
      //VB line 1
      newCenterCoeff[0][i] = coef[8] + ((coef[0] + coef[1] + coef[2] + coef[3])<<1);
      //VB line 2 
      newCenterCoeff[1][i] = coef[8] + ((coef[0] + coef[1] + coef[2])<<1);
      //VB line 3 
      newCenterCoeff[2][i] = coef[8] + ((coef[0] + coef[1])<<1);
      //VB line 4 
      newCenterCoeff[3][i] = coef[8] + ((coef[0])<<1);
    }
  }


  switch(filtNo)
  {
#if !ALF_SINGLE_FILTER_SHAPE
  case ALF_STAR5x5:
    {
      for(i= ypos; i<= yposEnd; i++)
      {

        yLineInLCU = i % m_lcuHeight;   

        if (yLineInLCU<m_lineIdxPadBot || i-yLineInLCU+m_lcuHeight >= m_img_height)
        {
          pImgPad1 = pImgPad +   stride;
          pImgPad2 = pImgPad -   stride;
          pImgPad3 = pImgPad + 2*stride;
          pImgPad4 = pImgPad - 2*stride;
        }
        else if (yLineInLCU<m_lineIdxPadTop)
        {
          paddingLine = - yLineInLCU + m_lineIdxPadTop - 1;
          pImgPad1 = pImgPad + min(paddingLine, 1)*stride;
          pImgPad2 = pImgPad -   stride;
          pImgPad3 = pImgPad + min(paddingLine, 2)*stride;
          pImgPad4 = pImgPad - 2*stride;
        }
        else
        {
          paddingLine = yLineInLCU - m_lineIdxPadTop ;
          pImgPad1 = pImgPad + stride;
          pImgPad2 = pImgPad - min(paddingLine, 1)*stride;
          pImgPad3 = pImgPad + 2*stride;
          pImgPad4 = pImgPad - min(paddingLine, 2)*stride;
        } 

        pVar = ppVarImg[i>>shiftHeight] + (xpos>>shiftWidth);

        if ( (yLineInLCU == m_lineIdxPadTop || yLineInLCU == m_lineIdxPadTop-1) && i-yLineInLCU+m_lcuHeight < m_img_height ) 
        {
          for(j= xpos; j<= xposEnd; j++)
          {
            pImgRes[j] = pImgPad[j];
          }
        }
        else if ( (yLineInLCU == m_lineIdxPadTop+1 || yLineInLCU == m_lineIdxPadTop-2) && i-yLineInLCU+m_lcuHeight < m_img_height ) 
        {
          for(j= xpos; j<= xposEnd ; j++)
          {
            if (j % VAR_SIZE_W==0) 
            {
              coef = filterSet[mergeTable[*(pVar++)]];
            }

            pixelInt  = 0;

            pixelInt += coef[0]* (pImgPad3[j+2]+pImgPad4[j-2]);
            pixelInt += coef[1]* (pImgPad3[j  ]+pImgPad4[j  ]);
            pixelInt += coef[2]* (pImgPad3[j-2]+pImgPad4[j+2]);

            pixelInt += coef[3]* (pImgPad1[j+1]+pImgPad2[j-1]);
            pixelInt += coef[4]* (pImgPad1[j  ]+pImgPad2[j  ]);
            pixelInt += coef[5]* (pImgPad1[j-1]+pImgPad2[j+1]);

            pixelInt += coef[6]* (pImgPad[j+2]+pImgPad[j-2]);
            pixelInt += coef[7]* (pImgPad[j+1]+pImgPad[j-1]);
            pixelInt += coef[8]* (pImgPad[j  ]);

            pixelInt=(int)((pixelInt+offset) >> numBitsMinus1);
            pImgRes[j] = ( Clip( pixelInt ) + pImgPad[j] ) >> 1;
          }
        }
        else
        {

        for(j= xpos; j<= xposEnd ; j++)
        {
          if (j % VAR_SIZE_W==0) 
          {
            coef = filterSet[mergeTable[*(pVar++)]];
          }

          pixelInt  = 0;

          pixelInt += coef[0]* (pImgPad3[j+2]+pImgPad4[j-2]);
          pixelInt += coef[1]* (pImgPad3[j  ]+pImgPad4[j  ]);
          pixelInt += coef[2]* (pImgPad3[j-2]+pImgPad4[j+2]);

          pixelInt += coef[3]* (pImgPad1[j+1]+pImgPad2[j-1]);
          pixelInt += coef[4]* (pImgPad1[j  ]+pImgPad2[j  ]);
          pixelInt += coef[5]* (pImgPad1[j-1]+pImgPad2[j+1]);

          pixelInt += coef[6]* (pImgPad[j+2]+pImgPad[j-2]);
          pixelInt += coef[7]* (pImgPad[j+1]+pImgPad[j-1]);
          pixelInt += coef[8]* (pImgPad[j  ]);

          pixelInt=(Int)((pixelInt+offset) >> numBitsMinus1);
          pImgRes[j] = Clip( pixelInt );
        }

        }

        pImgPad += stride;
        pImgRes += stride;
      }
    }
    break;
  case ALF_CROSS9x9:
    {
      Pel *pImgPad5, *pImgPad6, *pImgPad7, *pImgPad8;
#else
  case ALF_CROSS9x7_SQUARE3x3:
    {
      Pel *pImgPad5, *pImgPad6;
#endif
      for(i= ypos; i<= yposEnd; i++)
      {
        yLineInLCU = i % m_lcuHeight;   

        if (yLineInLCU<m_lineIdxPadBot || i-yLineInLCU+m_lcuHeight >= m_img_height)
        {
          pImgPad1 = pImgPad +   stride;
          pImgPad2 = pImgPad -   stride;
          pImgPad3 = pImgPad + 2*stride;
          pImgPad4 = pImgPad - 2*stride;
          pImgPad5 = pImgPad + 3*stride;
          pImgPad6 = pImgPad - 3*stride;
#if !ALF_SINGLE_FILTER_SHAPE
          pImgPad7 = pImgPad + 4*stride;
          pImgPad8 = pImgPad - 4*stride;
#endif
        }
        else if (yLineInLCU<m_lineIdxPadTop)
        {
          paddingLine = - yLineInLCU + m_lineIdxPadTop - 1;
#if ALF_SINGLE_FILTER_SHAPE
          pImgPad1 = (paddingLine < 1) ? pImgPad : pImgPad + min(paddingLine, 1)*stride;
          pImgPad2 = (paddingLine < 1) ? pImgPad : pImgPad -   stride;
          pImgPad3 = (paddingLine < 2) ? pImgPad : pImgPad + min(paddingLine, 2)*stride;
          pImgPad4 = (paddingLine < 2) ? pImgPad : pImgPad - 2*stride;
          pImgPad5 = (paddingLine < 3) ? pImgPad : pImgPad + min(paddingLine, 3)*stride;
          pImgPad6 = (paddingLine < 3) ? pImgPad : pImgPad - 3*stride;
#else
          pImgPad1 = pImgPad + min(paddingLine, 1)*stride;
          pImgPad2 = pImgPad -   stride;
          pImgPad3 = pImgPad + min(paddingLine, 2)*stride;
          pImgPad4 = pImgPad - 2*stride;
          pImgPad5 = pImgPad + min(paddingLine, 3)*stride;
          pImgPad6 = pImgPad - 3*stride;
          pImgPad7 = pImgPad + min(paddingLine, 4)*stride;
          pImgPad8 = pImgPad - 4*stride;
#endif
        }
        else
        {
          paddingLine = yLineInLCU - m_lineIdxPadTop ;
#if ALF_SINGLE_FILTER_SHAPE
          pImgPad1 = (paddingLine < 1) ? pImgPad : pImgPad +   stride;
          pImgPad2 = (paddingLine < 1) ? pImgPad : pImgPad - min(paddingLine, 1)*stride;
          pImgPad3 = (paddingLine < 2) ? pImgPad : pImgPad + 2*stride;
          pImgPad4 = (paddingLine < 2) ? pImgPad : pImgPad - min(paddingLine, 2)*stride;
          pImgPad5 = (paddingLine < 3) ? pImgPad : pImgPad + 3*stride;
          pImgPad6 = (paddingLine < 3) ? pImgPad : pImgPad - min(paddingLine, 3)*stride;
#else
          pImgPad1 = pImgPad + stride;
          pImgPad2 = pImgPad - min(paddingLine, 1)*stride;
          pImgPad3 = pImgPad + 2*stride;
          pImgPad4 = pImgPad - min(paddingLine, 2)*stride;
          pImgPad5 = pImgPad + 3*stride;
          pImgPad6 = pImgPad - min(paddingLine, 3)*stride;
          pImgPad7 = pImgPad + 4*stride;
          pImgPad8 = pImgPad - min(paddingLine, 4)*stride;
#endif
        } 

        pVar = ppVarImg[i>>shiftHeight] + (xpos>>shiftWidth);

#if ALF_SINGLE_FILTER_SHAPE
        {
          for(j= xpos; j<= xposEnd ; j++)
          {
            if (j % VAR_SIZE_W==0) 
            {
              coef = filterSet[mergeTable[*(pVar++)]];
            }

            pixelInt  = 0;

            pixelInt += coef[0]* (pImgPad5[j]+pImgPad6[j]);
            pixelInt += coef[1]* (pImgPad3[j]+pImgPad4[j]);
            pixelInt += coef[2]* (pImgPad1[j-1]+pImgPad2[j+1]);
            pixelInt += coef[3]* (pImgPad1[j]+pImgPad2[j]);
            pixelInt += coef[4]* (pImgPad1[j+1]+pImgPad2[j-1]);
            pixelInt += coef[5]* (pImgPad[j+4]+pImgPad[j-4]);
            pixelInt += coef[6]* (pImgPad[j+3]+pImgPad[j-3]);
            pixelInt += coef[7]* (pImgPad[j+2]+pImgPad[j-2]);
            pixelInt += coef[8]* (pImgPad[j+1]+pImgPad[j-1]);
            pixelInt += coef[9]* (pImgPad[j  ]);

            pixelInt=(Int)((pixelInt+offset) >> numBitsMinus1);
            pImgRes[j] = Clip( pixelInt );
          }
        }
#else
        if ( (yLineInLCU == m_lineIdxPadTop || yLineInLCU == m_lineIdxPadTop-1) && i-yLineInLCU+m_lcuHeight < m_img_height ) 
        {
          for(j= xpos; j<= xposEnd ; j++)
          {
            if (j % VAR_SIZE_W==0) 
            {
              varInd = *(pVar++);
              coef = filterSet[mergeTable[varInd]];
            }

            pixelInt  = 0;

            pixelInt += coef[4]* (pImgPad[j+4]+pImgPad[j-4]);
            pixelInt += coef[5]* (pImgPad[j+3]+pImgPad[j-3]);
            pixelInt += coef[6]* (pImgPad[j+2]+pImgPad[j-2]);
            pixelInt += coef[7]* (pImgPad[j+1]+pImgPad[j-1]);
            pixelInt += newCenterCoeff[0][mergeTable[varInd]]* (pImgPad[j]);

            pixelInt=(int)((pixelInt+offset) >> numBitsMinus1);
            pImgRes[j] = Clip( pixelInt );
          }
        }
        else if ( (yLineInLCU == m_lineIdxPadTop+1 || yLineInLCU == m_lineIdxPadTop-2) && i-yLineInLCU+m_lcuHeight < m_img_height ) 
        {
          for(j= xpos; j<= xposEnd ; j++)
          {
            if (j % VAR_SIZE_W==0) 
            {
              varInd = *(pVar++);
              coef = filterSet[mergeTable[varInd]];
            }

            pixelInt  = 0;

            pixelInt += coef[3]* (pImgPad1[j]+pImgPad2[j]);

            pixelInt += coef[4]* (pImgPad[j+4]+pImgPad[j-4]);
            pixelInt += coef[5]* (pImgPad[j+3]+pImgPad[j-3]);
            pixelInt += coef[6]* (pImgPad[j+2]+pImgPad[j-2]);
            pixelInt += coef[7]* (pImgPad[j+1]+pImgPad[j-1]);
            pixelInt += newCenterCoeff[1][mergeTable[varInd]]* (pImgPad[j]);

            pixelInt=(int)((pixelInt+offset) >> numBitsMinus1);
            pImgRes[j] = Clip( pixelInt );
          }
        }
        else if ( (yLineInLCU == m_lineIdxPadTop+2 || yLineInLCU == m_lineIdxPadTop-3) && i-yLineInLCU+m_lcuHeight < m_img_height ) 
        {
          for(j= xpos; j<= xposEnd ; j++)
          {
            if (j % VAR_SIZE_W==0) 
            {
              varInd = *(pVar++);
              coef = filterSet[mergeTable[varInd]];
            }

            pixelInt  = 0;

            pixelInt += coef[2]* (pImgPad3[j]+pImgPad4[j]);

            pixelInt += coef[3]* (pImgPad1[j]+pImgPad2[j]);

            pixelInt += coef[4]* (pImgPad[j+4]+pImgPad[j-4]);
            pixelInt += coef[5]* (pImgPad[j+3]+pImgPad[j-3]);
            pixelInt += coef[6]* (pImgPad[j+2]+pImgPad[j-2]);
            pixelInt += coef[7]* (pImgPad[j+1]+pImgPad[j-1]);
            pixelInt += newCenterCoeff[2][mergeTable[varInd]]* (pImgPad[j  ]);

            pixelInt=(int)((pixelInt+offset) >> numBitsMinus1);
            pImgRes[j] = Clip( pixelInt );
          }
        }
        else if ( (yLineInLCU == m_lineIdxPadTop+3 || yLineInLCU == m_lineIdxPadTop-4) && i-yLineInLCU+m_lcuHeight < m_img_height ) 
        {
          for(j= xpos; j<= xposEnd ; j++)
          {
            if (j % VAR_SIZE_W==0) 
            {
              varInd = *(pVar++);
              coef = filterSet[mergeTable[varInd]];
            }

            pixelInt  = 0;

            pixelInt += coef[1]* (pImgPad5[j]+pImgPad6[j]);

            pixelInt += coef[2]* (pImgPad3[j]+pImgPad4[j]);

            pixelInt += coef[3]* (pImgPad1[j]+pImgPad2[j]);

            pixelInt += coef[4]* (pImgPad[j+4]+pImgPad[j-4]);
            pixelInt += coef[5]* (pImgPad[j+3]+pImgPad[j-3]);
            pixelInt += coef[6]* (pImgPad[j+2]+pImgPad[j-2]);
            pixelInt += coef[7]* (pImgPad[j+1]+pImgPad[j-1]);
            pixelInt += newCenterCoeff[3][mergeTable[varInd]]* (pImgPad[j  ]);

            pixelInt=(int)((pixelInt+offset) >> numBitsMinus1);
            pImgRes[j] = Clip( pixelInt );
          }
        }
        else
        {
          for(j= xpos; j<= xposEnd ; j++)
          {
            if (j % VAR_SIZE_W==0) 
            {
              coef = filterSet[mergeTable[*(pVar++)]];
            }

            pixelInt  = 0;

            pixelInt += coef[0]* (pImgPad7[j]+pImgPad8[j]);

            pixelInt += coef[1]* (pImgPad5[j]+pImgPad6[j]);

            pixelInt += coef[2]* (pImgPad3[j]+pImgPad4[j]);

            pixelInt += coef[3]* (pImgPad1[j]+pImgPad2[j]);

            pixelInt += coef[4]* (pImgPad[j+4]+pImgPad[j-4]);
            pixelInt += coef[5]* (pImgPad[j+3]+pImgPad[j-3]);
            pixelInt += coef[6]* (pImgPad[j+2]+pImgPad[j-2]);
            pixelInt += coef[7]* (pImgPad[j+1]+pImgPad[j-1]);
            pixelInt += coef[8]* (pImgPad[j  ]);

            pixelInt=(Int)((pixelInt+offset) >> numBitsMinus1);
            pImgRes[j] = Clip( pixelInt );
          }
        }
#endif
        pImgPad += stride;
        pImgRes += stride;
      }
    }
    break;
  default:
    {
      printf("Not a supported filter shape\n");
      assert(0);
      exit(1);
    }
  }
}



Void TComAdaptiveLoopFilter::xCUAdaptive(TComPic* pcPic, Int filtNo, Pel *imgYFilt, Pel *imgYRec, Int Stride)
{
  // for every CU, call CU-adaptive ALF process
  for( UInt uiCUAddr = 0; uiCUAddr < pcPic->getNumCUsInFrame() ; uiCUAddr++ )
  {
    TComDataCU* pcCU = pcPic->getCU( uiCUAddr );
    xSubCUAdaptive(pcCU, filtNo, imgYFilt, imgYRec, 0, 0, Stride);
  }
}

Void TComAdaptiveLoopFilter::xSubCUAdaptive(TComDataCU* pcCU, Int filtNo, Pel *imgYFilt, Pel *imgYRec, UInt uiAbsPartIdx, UInt uiDepth, Int Stride)
{
  TComPic* pcPic = pcCU->getPic();

  if(pcPic==NULL)
  {
    return;
  }
  Bool bBoundary = false;
  UInt uiLPelX   = pcCU->getCUPelX() + g_auiRasterToPelX[ g_auiZscanToRaster[uiAbsPartIdx] ];
  UInt uiRPelX   = uiLPelX + (g_uiMaxCUWidth>>uiDepth)  - 1;
  UInt uiTPelY   = pcCU->getCUPelY() + g_auiRasterToPelY[ g_auiZscanToRaster[uiAbsPartIdx] ];
  UInt uiBPelY   = uiTPelY + (g_uiMaxCUHeight>>uiDepth) - 1;

  // check picture boundary
  if ( ( uiRPelX >= m_img_width ) || ( uiBPelY >= m_img_height ) )
  {
    bBoundary = true;
  }

  if ( ( ( uiDepth < pcCU->getDepth( uiAbsPartIdx ) ) && ( uiDepth < (g_uiMaxCUDepth-g_uiAddCUDepth) ) ) || bBoundary )
  {
    UInt uiQNumParts = ( pcPic->getNumPartInCU() >> (uiDepth<<1) )>>2;
    for ( UInt uiPartUnitIdx = 0; uiPartUnitIdx < 4; uiPartUnitIdx++, uiAbsPartIdx+=uiQNumParts )
    {
      uiLPelX   = pcCU->getCUPelX() + g_auiRasterToPelX[ g_auiZscanToRaster[uiAbsPartIdx] ];
      uiTPelY   = pcCU->getCUPelY() + g_auiRasterToPelY[ g_auiZscanToRaster[uiAbsPartIdx] ];

      if( ( uiLPelX < m_img_width ) && ( uiTPelY < m_img_height ) )
        xSubCUAdaptive(pcCU, filtNo, imgYFilt, imgYRec, uiAbsPartIdx, uiDepth+1, Stride);
    }
    return;
  }

  if ( pcCU->getAlfCtrlFlag(uiAbsPartIdx) )
  {
    filterLuma(imgYFilt, imgYRec, Stride, uiTPelY, min(uiBPelY,(unsigned int)(m_img_height-1)), uiLPelX, min(uiRPelX,(unsigned int)(m_img_width-1))
      ,filtNo, m_filterCoeffSym, m_varIndTab, m_varImg);
  }
}

/** Predict ALF luma filter coefficients. Centre coefficient is always predicted. Left neighbour is predicted according to flag.
 */
Void TComAdaptiveLoopFilter::predictALFCoeffLuma(ALFParam* pcAlfParam)
{
  Int sum, coeffPred, ind;
  const Int* pFiltMag = NULL;
  pFiltMag = weightsTabShapes[pcAlfParam->filter_shape];
  for(ind = 0; ind < pcAlfParam->filters_per_group; ++ind)
  {
    sum = 0;
    for(Int i = 0; i < pcAlfParam->num_coeff-2; i++)
    {
      sum +=  pFiltMag[i]*pcAlfParam->coeffmulti[ind][i];
    }
    if(pcAlfParam->nbSPred[ind]==0)
    {
      if((pcAlfParam->predMethod==0)|(ind==0))
      {
        coeffPred = ((1<<ALF_NUM_BIT_SHIFT)-sum) >> 2;
      }
      else
      {
        coeffPred = (0-sum) >> 2;
      }
      pcAlfParam->coeffmulti[ind][pcAlfParam->num_coeff-2] = coeffPred + pcAlfParam->coeffmulti[ind][pcAlfParam->num_coeff-2];
    }
    sum += pFiltMag[pcAlfParam->num_coeff-2]*pcAlfParam->coeffmulti[ind][pcAlfParam->num_coeff-2];
    if((pcAlfParam->predMethod==0)|(ind==0))
    {
      coeffPred = (1<<ALF_NUM_BIT_SHIFT)-sum;
    }
    else
    {
      coeffPred = -sum;
    }
    pcAlfParam->coeffmulti[ind][pcAlfParam->num_coeff-1] = coeffPred + pcAlfParam->coeffmulti[ind][pcAlfParam->num_coeff-1];
  }
}

Void TComAdaptiveLoopFilter::reconstructFilterCoeffs(ALFParam* pcAlfParam,int **pfilterCoeffSym)
{
  int i, ind;

  // Copy non zero filters in filterCoeffTmp
  for(ind = 0; ind < pcAlfParam->filters_per_group; ++ind)
  {
    for(i = 0; i < pcAlfParam->num_coeff; i++)
    {
      pfilterCoeffSym[ind][i] = pcAlfParam->coeffmulti[ind][i];
    }
  }
  // Undo prediction
  for(ind = 1; ind < pcAlfParam->filters_per_group; ++ind)
  {
    if(pcAlfParam->predMethod)
    {
      // Prediction
      for(i = 0; i < pcAlfParam->num_coeff; ++i)
      {
        pfilterCoeffSym[ind][i] = (int)(pfilterCoeffSym[ind][i] + pfilterCoeffSym[ind - 1][i]);
      }
    }
  }

  for(ind = 0; ind < pcAlfParam->filters_per_group; ind++)
  {
    checkFilterCoeffValue(pfilterCoeffSym[ind], pcAlfParam->num_coeff, false );
  }
}


#endif

static Pel Clip_post(int high, int val)
{
  return (Pel)(((val > high)? high: val));
}

#if !LCU_SYNTAX_ALF

/** Calculate ALF grouping indices for block-based (BA) mode
 * \param [out] imgYvar grouping indices buffer
 * \param [in] imgYpad picture buffer
 * \param [in] stride picture stride size
 * \param [in] adaptationMode  ALF_BA or ALF_RA mode
 */
Void TComAdaptiveLoopFilter::calcVar(Pel **imgYvar, Pel *imgYpad, Int stride, Int adaptationMode)
{
  if(adaptationMode == ALF_RA) 
  {
    return;
  }
  static Int shiftH = (Int)(log((double)VAR_SIZE_H)/log(2.0));
  static Int shiftW = (Int)(log((double)VAR_SIZE_W)/log(2.0));
  static Int varmax = (Int)NO_VAR_BINS-1;
#if ALF_16_BA_GROUPS
  static Int th[NO_VAR_BINS] = {0, 1, 2, 2, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 5}; 
  static Int avgVarTab[3][6] = { {0,  1,  2,  3,  4,  5,},
  {0,  6,  7,  8,  9, 10,},
  {0, 11, 12, 13, 14, 15}   };
#else
  static Int step1  = (Int)((Int)(NO_VAR_BINS)/3) - 1;  
  static Int th[NO_VAR_BINS] = {0, 1, 2, 2, 2, 3, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4}; 
#endif  
  Int i, j, avgvar, vertical, horizontal,direction, yoffset;
  Pel *pimgYpad, *pimgYpadup, *pimgYpaddown;

  for(i = 0; i < m_img_height - 3; i=i+4)
  {
    yoffset      = ((i)*stride) + stride;
    pimgYpad     = &imgYpad [yoffset];
    pimgYpadup   = &imgYpad [yoffset + stride];
    pimgYpaddown = &imgYpad [yoffset - stride];

    for(j = 0; j < m_img_width - 3 ; j=j+4)
    {
      // Compute at sub-sample by 2
      vertical   =  abs((pimgYpad[j+1]<<1  ) - pimgYpaddown[j+1]   - pimgYpadup[j+1]);
      horizontal =  abs((pimgYpad[j+1]<<1  ) - pimgYpad    [j+2]   - pimgYpad  [j  ]);

      vertical   += abs((pimgYpad[j+2]<<1  ) - pimgYpaddown[j+2]   - pimgYpadup[j+2]);
      horizontal += abs((pimgYpad[j+2]<<1  ) - pimgYpad    [j+3]   - pimgYpad  [j+1]);

      vertical   += abs((pimgYpad[j+1+stride]<<1) - pimgYpaddown[j+1+stride] - pimgYpadup[j+1+stride]);
      horizontal += abs((pimgYpad[j+1+stride]<<1) - pimgYpad    [j+2+stride] - pimgYpad  [j+stride  ]);

      vertical   += abs((pimgYpad[j+2+stride]<<1) - pimgYpaddown[j+2+stride] - pimgYpadup[j+2+stride]);
      horizontal += abs((pimgYpad[j+2+stride]<<1) - pimgYpad    [j+3+stride] - pimgYpad  [j+1+stride]);

      direction = 0;
      if (vertical > 2*horizontal) 
      {
        direction = 1; //vertical
      }
      if (horizontal > 2*vertical)
      {
        direction = 2; //horizontal
      }

      avgvar = (vertical + horizontal) >> 2;
      avgvar = (Pel) Clip_post(varmax, avgvar >>(g_uiBitIncrement+1));
      avgvar = th[avgvar];
#if ALF_16_BA_GROUPS
      avgvar = avgVarTab[direction][avgvar];
#else      
      avgvar = Clip_post(step1, (Int) avgvar ) + (step1+1)*direction;
#endif
      imgYvar[(i )>>shiftH][(j)>>shiftW] = avgvar;
    }
  }
}

Void TComAdaptiveLoopFilter::createRegionIndexMap(Pel **imgYVar, Int imgWidth, Int imgHeight)
{
  int varStepSizeWidth = VAR_SIZE_W;
  int varStepSizeHeight = VAR_SIZE_H;
  int shiftHeight = (int)(log((double)varStepSizeHeight)/log(2.0));
  int shiftWidth = (int)(log((double)varStepSizeWidth)/log(2.0));

  int i, j;
  int regionTable[NO_VAR_BINS] = {0, 1, 4, 5, 15, 2, 3, 6, 14, 11, 10, 7, 13, 12,  9,  8}; 
  int xInterval;
  int yInterval;
  int yIndex;
  int yIndexOffset;
  int yStartLine;
  int yEndLine;

  xInterval = ((( (imgWidth+63)/64) + 1) / 4 * 64)>>shiftWidth;  
  yInterval = ((((imgHeight+63)/64) + 1) / 4 * 64)>>shiftHeight;

  for (yIndex = 0; yIndex < 4 ; yIndex++)
  {
    yIndexOffset = yIndex * 4;
    yStartLine = yIndex * yInterval;
    yEndLine   = (yIndex == 3) ? imgHeight>>shiftHeight : (yStartLine+yInterval);

    for(i = yStartLine; i < yEndLine ; i++)
    {
      for(j = 0; j < xInterval ; j++)
      {
        imgYVar[i][j] = regionTable[yIndexOffset+0];     
      }

      for(j = xInterval; j < xInterval*2 ; j++)
      {
        imgYVar[i][j] = regionTable[yIndexOffset+1];     
      }

      for(j = xInterval*2; j < xInterval*3 ; j++)
      {
        imgYVar[i][j] = regionTable[yIndexOffset+2];     
      }

      for(j = xInterval*3; j < imgWidth>>shiftWidth ; j++)
      {
        imgYVar[i][j] = regionTable[yIndexOffset+3];     
      }
    }
  }

}

// --------------------------------------------------------------------------------------------------------------------
// ALF for chroma
// --------------------------------------------------------------------------------------------------------------------

/** 
 \param pcPicDec    picture before ALF
 \param pcPicRest   picture after  ALF
 \param qh          filter coefficient
 \param iTap        filter tap
 \param iColor      0 for Cb and 1 for Cr
 */
Void TComAdaptiveLoopFilter::filterChroma(Pel *pImgRes, Pel *pImgPad, Int stride, 
                                          Int ypos, Int yposEnd, Int xpos, Int xposEnd, 
                                          Int filtNo, Int* coef)
{
  static Int numBitsMinus1= (Int)ALF_NUM_BIT_SHIFT;
  static Int offset       = (1<<( (Int)ALF_NUM_BIT_SHIFT-1));

  Pel *pImgPad1,*pImgPad2,*pImgPad3,*pImgPad4;
  Int i, j, pixelInt;

  pImgPad    += (ypos*stride);
  pImgRes    += (ypos*stride);

  Int imgHeightChroma = m_img_height>>1;
  Int yLineInLCU;
  Int paddingline;
  Int newCenterCoeff[4];

  ::memset(newCenterCoeff, 0, sizeof(Int)*4);
#if ALF_SINGLE_FILTER_SHAPE
  if(filtNo == ALF_CROSS9x7_SQUARE3x3)
#else
  if (filtNo == ALF_CROSS9x9)
#endif
  {
    //VB line 1
    newCenterCoeff[0] = coef[8] + ((coef[0] + coef[1] + coef[2] + coef[3])<<1);
    //VB line 2 
    newCenterCoeff[1] = coef[8] + ((coef[0] + coef[1] + coef[2])<<1);
    //VB line 3 
    newCenterCoeff[2] = coef[8] + ((coef[0] + coef[1])<<1);
    //VB line 4 
    newCenterCoeff[3] = coef[8] + ((coef[0])<<1);
  }

  switch(filtNo)
  {
#if !ALF_SINGLE_FILTER_SHAPE
  case ALF_STAR5x5:
    {
      for(i= ypos; i<= yposEnd; i++)
      {
        yLineInLCU = i % m_lcuHeightChroma;

        if (yLineInLCU < m_lineIdxPadBotChroma || i-yLineInLCU+m_lcuHeightChroma >= imgHeightChroma )
        {
          pImgPad1 = pImgPad + stride;
          pImgPad2 = pImgPad - stride;
          pImgPad3 = pImgPad + 2*stride;
          pImgPad4 = pImgPad - 2*stride;
        }
        else if (yLineInLCU < m_lineIdxPadTopChroma)
        {
          paddingline = - yLineInLCU + m_lineIdxPadTopChroma - 1;
          pImgPad1 = pImgPad + min(paddingline, 1)*stride;
          pImgPad2 = pImgPad - stride;
          pImgPad3 = pImgPad + min(paddingline, 2)*stride;
          pImgPad4 = pImgPad - 2*stride;
        }
        else
        {
          paddingline = yLineInLCU - m_lineIdxPadTopChroma ;
          pImgPad1 = pImgPad + stride;
          pImgPad2 = pImgPad - min(paddingline, 1)*stride;
          pImgPad3 = pImgPad + 2*stride;
          pImgPad4 = pImgPad - min(paddingline, 2)*stride;
        }

        if ( (yLineInLCU == m_lineIdxPadTopChroma || yLineInLCU == m_lineIdxPadTopChroma-1) && i-yLineInLCU+m_lcuHeightChroma < imgHeightChroma ) 
        {
          for(j= xpos; j<= xposEnd ; j++)
          {
            pImgRes[j] = pImgPad[j];
          }
        }
        else if ( (yLineInLCU == m_lineIdxPadTopChroma+1 || yLineInLCU == m_lineIdxPadTopChroma-2) && i-yLineInLCU+m_lcuHeightChroma < imgHeightChroma ) 
        {
          for(j= xpos; j<= xposEnd ; j++)
          {
            pixelInt  = 0;

            pixelInt += coef[0]* (pImgPad3[j+2]+pImgPad4[j-2]);
            pixelInt += coef[1]* (pImgPad3[j  ]+pImgPad4[j  ]);
            pixelInt += coef[2]* (pImgPad3[j-2]+pImgPad4[j+2]);

            pixelInt += coef[3]* (pImgPad1[j+1]+pImgPad2[j-1]);
            pixelInt += coef[4]* (pImgPad1[j  ]+pImgPad2[j  ]);
            pixelInt += coef[5]* (pImgPad1[j-1]+pImgPad2[j+1]);

            pixelInt += coef[6]* (pImgPad[j+2]+pImgPad[j-2]);
            pixelInt += coef[7]* (pImgPad[j+1]+pImgPad[j-1]);
            pixelInt += coef[8]* (pImgPad[j  ]);

            pixelInt=(Int)((pixelInt+offset) >> numBitsMinus1);

            pImgRes[j] = (Clip( pixelInt ) + pImgPad[j]) >> 1;
          }
        }
        else
        {

        for(j= xpos; j<= xposEnd ; j++)
        {
          pixelInt  = 0;

          pixelInt += coef[0]* (pImgPad3[j+2]+pImgPad4[j-2]);
          pixelInt += coef[1]* (pImgPad3[j  ]+pImgPad4[j  ]);
          pixelInt += coef[2]* (pImgPad3[j-2]+pImgPad4[j+2]);

          pixelInt += coef[3]* (pImgPad1[j+1]+pImgPad2[j-1]);
          pixelInt += coef[4]* (pImgPad1[j  ]+pImgPad2[j  ]);
          pixelInt += coef[5]* (pImgPad1[j-1]+pImgPad2[j+1]);

          pixelInt += coef[6]* (pImgPad[j+2]+pImgPad[j-2]);
          pixelInt += coef[7]* (pImgPad[j+1]+pImgPad[j-1]);
          pixelInt += coef[8]* (pImgPad[j  ]);

          pixelInt=(Int)((pixelInt+offset) >> numBitsMinus1);

          pImgRes[j] = Clip( pixelInt );
        }

        }
        pImgPad += stride;
        pImgRes += stride;
      }
    }
    break;
  case ALF_CROSS9x9:
    {
      Pel *pImgPad5, *pImgPad6, *pImgPad7, *pImgPad8;
#else
  case ALF_CROSS9x7_SQUARE3x3:
    {
      Pel *pImgPad5, *pImgPad6;
#endif
      for(i= ypos; i<= yposEnd; i++)
      {
        yLineInLCU = i % m_lcuHeightChroma;
#if ALF_SINGLE_FILTER_SHAPE
        if (yLineInLCU<2 && i> 2)
#else
        if (yLineInLCU<2)
#endif 
        {
          paddingline = yLineInLCU + 2 ;
          pImgPad1 = pImgPad + stride;
          pImgPad2 = pImgPad - stride;
          pImgPad3 = pImgPad + 2*stride;
          pImgPad4 = pImgPad - 2*stride;
#if ALF_SINGLE_FILTER_SHAPE
          pImgPad5 = (paddingline < 3) ? pImgPad : pImgPad + 3*stride;
          pImgPad6 = (paddingline < 3) ? pImgPad : pImgPad - min(paddingline, 3)*stride; 
#else
          pImgPad5 = pImgPad + 3*stride;
          pImgPad6 = pImgPad - min(paddingline, 3)*stride;
          pImgPad7 = pImgPad + 4*stride;
          pImgPad8 = pImgPad - min(paddingline, 4)*stride;
#endif
        }
        else if (yLineInLCU < m_lineIdxPadBotChroma || i-yLineInLCU+m_lcuHeightChroma >= imgHeightChroma )
        {
          pImgPad1 = pImgPad + stride;
          pImgPad2 = pImgPad - stride;
          pImgPad3 = pImgPad + 2*stride;
          pImgPad4 = pImgPad - 2*stride;
          pImgPad5 = pImgPad + 3*stride;
          pImgPad6 = pImgPad - 3*stride;
#if !ALF_SINGLE_FILTER_SHAPE
          pImgPad7 = pImgPad + 4*stride;
          pImgPad8 = pImgPad - 4*stride;
#endif
        }
        else if (yLineInLCU < m_lineIdxPadTopChroma)
        {
          paddingline = - yLineInLCU + m_lineIdxPadTopChroma - 1;
#if ALF_SINGLE_FILTER_SHAPE
          pImgPad1 = (paddingline < 1) ? pImgPad : pImgPad + min(paddingline, 1)*stride;
          pImgPad2 = (paddingline < 1) ? pImgPad : pImgPad -   stride;
          pImgPad3 = (paddingline < 2) ? pImgPad : pImgPad + min(paddingline, 2)*stride;
          pImgPad4 = (paddingline < 2) ? pImgPad : pImgPad - 2*stride;
          pImgPad5 = (paddingline < 3) ? pImgPad : pImgPad + min(paddingline, 3)*stride;
          pImgPad6 = (paddingline < 3) ? pImgPad : pImgPad - 3*stride;
#else
          pImgPad1 = pImgPad + min(paddingline, 1)*stride;
          pImgPad2 = pImgPad - stride;
          pImgPad3 = pImgPad + min(paddingline, 2)*stride;
          pImgPad4 = pImgPad - 2*stride;
          pImgPad5 = pImgPad + min(paddingline, 3)*stride;
          pImgPad6 = pImgPad - 3*stride;
          pImgPad7 = pImgPad + min(paddingline, 4)*stride;
          pImgPad8 = pImgPad - 4*stride;
#endif
        }
        else
        {
          paddingline = yLineInLCU - m_lineIdxPadTopChroma ;
#if ALF_SINGLE_FILTER_SHAPE
          pImgPad1 = (paddingline < 1) ? pImgPad : pImgPad +   stride;
          pImgPad2 = (paddingline < 1) ? pImgPad : pImgPad - min(paddingline, 1)*stride;
          pImgPad3 = (paddingline < 2) ? pImgPad : pImgPad + 2*stride;
          pImgPad4 = (paddingline < 2) ? pImgPad : pImgPad - min(paddingline, 2)*stride;
          pImgPad5 = (paddingline < 3) ? pImgPad : pImgPad + 3*stride;
          pImgPad6 = (paddingline < 3) ? pImgPad : pImgPad - min(paddingline, 3)*stride;
#else
          pImgPad1 = pImgPad + stride;
          pImgPad2 = pImgPad - min(paddingline, 1)*stride;
          pImgPad3 = pImgPad + 2*stride;
          pImgPad4 = pImgPad - min(paddingline, 2)*stride;
          pImgPad5 = pImgPad + 3*stride;
          pImgPad6 = pImgPad - min(paddingline, 3)*stride;
          pImgPad7 = pImgPad + 4*stride;
          pImgPad8 = pImgPad - min(paddingline, 4)*stride;
#endif
        }

#if ALF_SINGLE_FILTER_SHAPE
          for(j= xpos; j<= xposEnd ; j++)
          {
            pixelInt  = 0;

            pixelInt += coef[0]* (pImgPad5[j]+pImgPad6[j]);
            pixelInt += coef[1]* (pImgPad3[j]+pImgPad4[j]);
            pixelInt += coef[2]* (pImgPad1[j-1]+pImgPad2[j+1]);
            pixelInt += coef[3]* (pImgPad1[j]+pImgPad2[j]);
            pixelInt += coef[4]* (pImgPad1[j+1]+pImgPad2[j-1]);
            pixelInt += coef[5]* (pImgPad[j+4]+pImgPad[j-4]);
            pixelInt += coef[6]* (pImgPad[j+3]+pImgPad[j-3]);
            pixelInt += coef[7]* (pImgPad[j+2]+pImgPad[j-2]);
            pixelInt += coef[8]* (pImgPad[j+1]+pImgPad[j-1]);
            pixelInt += coef[9]* (pImgPad[j  ]);

            pixelInt=(Int)((pixelInt+offset) >> numBitsMinus1);

            pImgRes[j] = Clip( pixelInt );
          }
#else
        if ( (yLineInLCU == m_lineIdxPadTopChroma || yLineInLCU == m_lineIdxPadTopChroma-1) && i-yLineInLCU+m_lcuHeightChroma < imgHeightChroma ) 
        {
          for(j= xpos; j<= xposEnd ; j++)
          {
            pixelInt  = 0;

            pixelInt += coef[4]* (pImgPad[j+4]+pImgPad[j-4]);
            pixelInt += coef[5]* (pImgPad[j+3]+pImgPad[j-3]);
            pixelInt += coef[6]* (pImgPad[j+2]+pImgPad[j-2]);
            pixelInt += coef[7]* (pImgPad[j+1]+pImgPad[j-1]);
            pixelInt += newCenterCoeff[0]* (pImgPad[j  ]);

            pixelInt=(Int)((pixelInt+offset) >> numBitsMinus1);

            pImgRes[j] = Clip( pixelInt );
          }
        }
        else if ( (yLineInLCU == m_lineIdxPadTopChroma+1 || yLineInLCU == m_lineIdxPadTopChroma-2) && i-yLineInLCU+m_lcuHeightChroma < imgHeightChroma ) 
        {
          for(j= xpos; j<= xposEnd ; j++)
          {
            pixelInt  = 0;
            
            pixelInt += coef[3]* (pImgPad1[j]+pImgPad2[j]);

            pixelInt += coef[4]* (pImgPad[j+4]+pImgPad[j-4]);
            pixelInt += coef[5]* (pImgPad[j+3]+pImgPad[j-3]);
            pixelInt += coef[6]* (pImgPad[j+2]+pImgPad[j-2]);
            pixelInt += coef[7]* (pImgPad[j+1]+pImgPad[j-1]);
            pixelInt += newCenterCoeff[1]* (pImgPad[j  ]);

            pixelInt=(Int)((pixelInt+offset) >> numBitsMinus1);

            pImgRes[j] = Clip( pixelInt );
          }
        }
        else if ( (yLineInLCU == 0 && i>0) || (yLineInLCU == m_lineIdxPadTopChroma-3 && i-yLineInLCU+m_lcuHeightChroma < imgHeightChroma) )
        {
          for(j= xpos; j<= xposEnd ; j++)
          {
            pixelInt  = 0;

            pixelInt += coef[2]* (pImgPad3[j]+pImgPad4[j]);

            pixelInt += coef[3]* (pImgPad1[j]+pImgPad2[j]);

            pixelInt += coef[4]* (pImgPad[j+4]+pImgPad[j-4]);
            pixelInt += coef[5]* (pImgPad[j+3]+pImgPad[j-3]);
            pixelInt += coef[6]* (pImgPad[j+2]+pImgPad[j-2]);
            pixelInt += coef[7]* (pImgPad[j+1]+pImgPad[j-1]);
            pixelInt += newCenterCoeff[2]* (pImgPad[j  ]);

            pixelInt=(Int)((pixelInt+offset) >> numBitsMinus1);

            pImgRes[j] = Clip( pixelInt );
          }
        }
        else if ( (yLineInLCU == 1 && i>1) || (yLineInLCU == m_lineIdxPadTopChroma-4 && i-yLineInLCU+m_lcuHeightChroma < imgHeightChroma) ) 
        {
          for(j= xpos; j<= xposEnd ; j++)
          {
            pixelInt  = 0;

            pixelInt += coef[1]* (pImgPad5[j]+pImgPad6[j]);

            pixelInt += coef[2]* (pImgPad3[j]+pImgPad4[j]);

            pixelInt += coef[3]* (pImgPad1[j]+pImgPad2[j]);

            pixelInt += coef[4]* (pImgPad[j+4]+pImgPad[j-4]);
            pixelInt += coef[5]* (pImgPad[j+3]+pImgPad[j-3]);
            pixelInt += coef[6]* (pImgPad[j+2]+pImgPad[j-2]);
            pixelInt += coef[7]* (pImgPad[j+1]+pImgPad[j-1]);
            pixelInt += newCenterCoeff[3]* (pImgPad[j  ]);

            pixelInt=(Int)((pixelInt+offset) >> numBitsMinus1);

            pImgRes[j] = Clip( pixelInt );
          }
        }
        else
        {          
          for(j= xpos; j<= xposEnd ; j++)
          {
            pixelInt  = 0;

            pixelInt += coef[0]* (pImgPad7[j]+pImgPad8[j]);

            pixelInt += coef[1]* (pImgPad5[j]+pImgPad6[j]);

            pixelInt += coef[2]* (pImgPad3[j]+pImgPad4[j]);

            pixelInt += coef[3]* (pImgPad1[j]+pImgPad2[j]);

            pixelInt += coef[4]* (pImgPad[j+4]+pImgPad[j-4]);
            pixelInt += coef[5]* (pImgPad[j+3]+pImgPad[j-3]);
            pixelInt += coef[6]* (pImgPad[j+2]+pImgPad[j-2]);
            pixelInt += coef[7]* (pImgPad[j+1]+pImgPad[j-1]);
            pixelInt += coef[8]* (pImgPad[j  ]);

            pixelInt=(Int)((pixelInt+offset) >> numBitsMinus1);

            pImgRes[j] = Clip( pixelInt );
          }
        }
#endif
        pImgPad += stride;
        pImgRes += stride;

      }
    }

    break;
  default:
    {
      printf("Not a supported filter shape\n");
      assert(0);
      exit(1);
    }
  }

}

/** Chroma filtering for multi-slice picture
 * \param componentID slice parameters
 * \param pcPicDecYuv original picture
 * \param pcPicRestYuv picture before filtering
 * \param coeff filter coefficients
 * \param filtNo  filter shape
 * \param chromaFormatShift size adjustment for chroma (1 for 4:2:0 format)
 */
Void TComAdaptiveLoopFilter::xFilterChromaSlices(Int componentID, TComPicYuv* pcPicDecYuv, TComPicYuv* pcPicRestYuv, Int *coeff, Int filtNo, Int chromaFormatShift)
{
  Pel* pPicDec   = (componentID == ALF_Cb)?(    pcPicDecYuv->getCbAddr()):(    pcPicDecYuv->getCrAddr());
  Pel* pPicSlice = (componentID == ALF_Cb)?(m_pcSliceYuvTmp->getCbAddr()):(m_pcSliceYuvTmp->getCrAddr());
  Pel* pRest     = (componentID == ALF_Cb)?(   pcPicRestYuv->getCbAddr()):(   pcPicRestYuv->getCrAddr());
  Int  stride    = pcPicDecYuv->getCStride();

  for(UInt s=0; s< m_uiNumSlicesInPic; s++)
  {
    if(!m_pcPic->getValidSlice(s))
    {
      continue;
    }
    std::vector< std::vector<AlfLCUInfo*> > & vpSliceTileAlfLCU = m_pvpSliceTileAlfLCU[s];

    for(Int t=0; t< (Int)vpSliceTileAlfLCU.size(); t++)
    {
      std::vector<AlfLCUInfo*> & vpAlfLCU = vpSliceTileAlfLCU[t];

      copyRegion(vpAlfLCU, pPicSlice, pPicDec, stride, chromaFormatShift);
      extendRegionBorder(vpAlfLCU, pPicSlice, stride, chromaFormatShift);
      filterChromaRegion(vpAlfLCU, pPicSlice, pRest, stride, coeff, filtNo, chromaFormatShift);
    }
  }
}

/** Chroma filtering for one component in multi-slice picture
 * \param componentID slice parameters
 * \param pcPicDecYuv original picture
 * \param pcPicRestYuv picture before filtering
 * \param shape  filter shape
 * \param pCoeff filter coefficients
 */
Void TComAdaptiveLoopFilter::xFilterChromaOneCmp(Int componentID, TComPicYuv *pDecYuv, TComPicYuv *pRestYuv, Int shape, Int *pCoeff)
{
  Int chromaFormatShift = 1;
  if(!m_bUseNonCrossALF)
  {
    Pel* pDec    = (componentID == ALF_Cb)?(pDecYuv->getCbAddr()): (pDecYuv->getCrAddr());
    Pel* pRest   = (componentID == ALF_Cb)?(pRestYuv->getCbAddr()):(pRestYuv->getCrAddr());
    Int  iStride = pDecYuv->getCStride();
    filterChroma(pRest, pDec, iStride, 0, (Int)(m_img_height>>1) -1, 0, (Int)(m_img_width>>1)-1, shape, pCoeff);
  }
  else
  {
    xFilterChromaSlices(componentID, pDecYuv, pRestYuv, pCoeff, shape, chromaFormatShift);
  }
}

/** Chroma filtering for  multi-slice picture
 * \param pcAlfParam ALF parameters
 * \param pcPicDec to-be-filtered picture buffer 
 * \param pcPicRest filtered picture buffer
 */
Void TComAdaptiveLoopFilter::xALFChroma(ALFParam* pcAlfParam, TComPicYuv* pcPicDec, TComPicYuv* pcPicRest)
{
  if((pcAlfParam->chroma_idc>>1)&0x01)
  {
    xFilterChromaOneCmp(ALF_Cb, pcPicDec, pcPicRest, pcAlfParam->filter_shape_chroma, pcAlfParam->coeff_chroma);
  }

  if(pcAlfParam->chroma_idc&0x01)
  {
    xFilterChromaOneCmp(ALF_Cr, pcPicDec, pcPicRest, pcAlfParam->filter_shape_chroma, pcAlfParam->coeff_chroma);
  }
}

#endif

Void TComAdaptiveLoopFilter::setAlfCtrlFlags(AlfCUCtrlInfo* pAlfParam, TComDataCU *pcCU, UInt uiAbsPartIdx, UInt uiDepth, UInt &idx)
{
  TComPic* pcPic = pcCU->getPic();
  UInt uiCurNumParts    = pcPic->getNumPartInCU() >> (uiDepth<<1);
  UInt uiQNumParts      = uiCurNumParts>>2;
  
  Bool bBoundary = false;
  UInt uiLPelX   = pcCU->getCUPelX() + g_auiRasterToPelX[ g_auiZscanToRaster[uiAbsPartIdx] ];
  UInt uiRPelX   = uiLPelX + (g_uiMaxCUWidth>>uiDepth)  - 1;
  UInt uiTPelY   = pcCU->getCUPelY() + g_auiRasterToPelY[ g_auiZscanToRaster[uiAbsPartIdx] ];
  UInt uiBPelY   = uiTPelY + (g_uiMaxCUHeight>>uiDepth) - 1;
  
  if( ( uiRPelX >= pcCU->getSlice()->getSPS()->getPicWidthInLumaSamples() ) || ( uiBPelY >= pcCU->getSlice()->getSPS()->getPicHeightInLumaSamples() ) )
  {
    bBoundary = true;
  }
  
  if( ( ( uiDepth < pcCU->getDepth( uiAbsPartIdx ) ) && ( uiDepth < g_uiMaxCUDepth - g_uiAddCUDepth ) ) || bBoundary )
  {
    UInt uiIdx = uiAbsPartIdx;
    for ( UInt uiPartUnitIdx = 0; uiPartUnitIdx < 4; uiPartUnitIdx++ )
    {
      uiLPelX   = pcCU->getCUPelX() + g_auiRasterToPelX[ g_auiZscanToRaster[uiIdx] ];
      uiTPelY   = pcCU->getCUPelY() + g_auiRasterToPelY[ g_auiZscanToRaster[uiIdx] ];
      
      if( ( uiLPelX < pcCU->getSlice()->getSPS()->getPicWidthInLumaSamples() ) && ( uiTPelY < pcCU->getSlice()->getSPS()->getPicHeightInLumaSamples() ) )
      {
        setAlfCtrlFlags(pAlfParam, pcCU, uiIdx, uiDepth+1, idx);
      }
      uiIdx += uiQNumParts;
    }
    
    return;
  }
  
  if( uiDepth <= pAlfParam->alf_max_depth || pcCU->isFirstAbsZorderIdxInDepth(uiAbsPartIdx, pAlfParam->alf_max_depth))
  {
    if (uiDepth > pAlfParam->alf_max_depth)
    {
      pcCU->setAlfCtrlFlagSubParts(pAlfParam->alf_cu_flag[idx], uiAbsPartIdx, pAlfParam->alf_max_depth);
    }
    else
    {
      pcCU->setAlfCtrlFlagSubParts(pAlfParam->alf_cu_flag[idx], uiAbsPartIdx, uiDepth );
    }
    idx++;
  }
}

/** Initialize the variables for one ALF LCU
 * \param rAlfLCU to-be-initialized ALF LCU
 * \param sliceID slice index
 * \param tileID tile index
 * \param pcCU CU data pointer
 * \param maxNumSUInLCU maximum number of SUs in one LCU
 */
Void TComAdaptiveLoopFilter::InitAlfLCUInfo(AlfLCUInfo& rAlfLCU, Int sliceID, Int tileID, TComDataCU* pcCU, UInt maxNumSUInLCU)
{
  //pcCU
  rAlfLCU.pcCU     = pcCU;
  //sliceID
  rAlfLCU.sliceID = sliceID;
  //tileID
  rAlfLCU.tileID  = tileID;

  //numSGU, vpAlfBLock;
  std::vector<NDBFBlockInfo>& vNDBFBlock = *(pcCU->getNDBFilterBlocks());
  rAlfLCU.vpAlfBlock.clear();
  rAlfLCU.numSGU = 0;
  for(Int i=0; i< vNDBFBlock.size(); i++)
  {
    if( vNDBFBlock[i].sliceID == sliceID)
    {
      rAlfLCU.vpAlfBlock.push_back( &(vNDBFBlock[i])  );
      rAlfLCU.numSGU ++;
    }
  }
  //startSU
  rAlfLCU.startSU = rAlfLCU.vpAlfBlock.front()->startSU;
  //endSU
  rAlfLCU.endSU   = rAlfLCU.vpAlfBlock.back()->endSU;
  //bAllSUsInLCUInSameSlice
  rAlfLCU.bAllSUsInLCUInSameSlice = (rAlfLCU.startSU == 0)&&( rAlfLCU.endSU == maxNumSUInLCU -1);
}

/** create and initialize variables for picture ALF processing
 * \param pcPic picture-level data pointer
 * \param numSlicesInPic number of slices in picture
 */
#if LCU_SYNTAX_ALF
Void TComAdaptiveLoopFilter::createPicAlfInfo(TComPic* pcPic, Int numSlicesInPic, Int alfQP)
#else
Void TComAdaptiveLoopFilter::createPicAlfInfo(TComPic* pcPic, Int numSlicesInPic)
#endif
{
  m_uiNumSlicesInPic = numSlicesInPic;
  m_iSGDepth         = pcPic->getSliceGranularityForNDBFilter();

  m_bUseNonCrossALF = ( pcPic->getIndependentSliceBoundaryForNDBFilter() || pcPic->getIndependentTileBoundaryForNDBFilter());
  m_pcPic = pcPic;

#if LCU_SYNTAX_ALF
  m_isNonCrossSlice = pcPic->getIndependentSliceBoundaryForNDBFilter(); 
  m_suWidth = pcPic->getMinCUWidth();
  m_suHeight= pcPic->getMinCUHeight();
  m_alfQP = alfQP; 
#endif
#if !LCU_SYNTAX_ALF  
  if(m_uiNumSlicesInPic > 1 || m_bUseNonCrossALF)
  {
#endif
    m_ppSliceAlfLCUs = new AlfLCUInfo*[m_uiNumSlicesInPic];
    m_pvpAlfLCU = new std::vector< AlfLCUInfo* >[m_uiNumSlicesInPic];
    m_pvpSliceTileAlfLCU = new std::vector< std::vector< AlfLCUInfo* > >[m_uiNumSlicesInPic];

    for(Int s=0; s< m_uiNumSlicesInPic; s++)
    {
      m_ppSliceAlfLCUs[s] = NULL;
      if(!pcPic->getValidSlice(s))
      {
        continue;
      }

      std::vector< TComDataCU* >& vSliceLCUPointers = pcPic->getOneSliceCUDataForNDBFilter(s);
      Int                         numLCU           = (Int)vSliceLCUPointers.size();

      //create Alf LCU info
      m_ppSliceAlfLCUs[s] = new AlfLCUInfo[numLCU];
      for(Int i=0; i< numLCU; i++)
      {
        TComDataCU* pcCU       = vSliceLCUPointers[i];
        if(pcCU->getPic()==0)
        {
          continue;
        }
        Int         currTileID = pcPic->getPicSym()->getTileIdxMap(pcCU->getAddr());

        InitAlfLCUInfo(m_ppSliceAlfLCUs[s][i], s, currTileID, pcCU, pcPic->getNumPartInCU());
      }

      //distribute Alf LCU info pointers to slice container
      std::vector< AlfLCUInfo* >&    vpSliceAlfLCU     = m_pvpAlfLCU[s]; 
      vpSliceAlfLCU.reserve(numLCU);
      vpSliceAlfLCU.resize(0);
      std::vector< std::vector< AlfLCUInfo* > > &vpSliceTileAlfLCU = m_pvpSliceTileAlfLCU[s];
      Int prevTileID = -1;
      Int numValidTilesInSlice = 0;

      for(Int i=0; i< numLCU; i++)
      {
        AlfLCUInfo* pcAlfLCU = &(m_ppSliceAlfLCUs[s][i]);

        //container of Alf LCU pointers for slice processing
        vpSliceAlfLCU.push_back( pcAlfLCU);

        if(pcAlfLCU->tileID != prevTileID)
        {
          if(prevTileID == -1 || pcPic->getIndependentTileBoundaryForNDBFilter())
          {
            prevTileID = pcAlfLCU->tileID;
            numValidTilesInSlice ++;
            vpSliceTileAlfLCU.resize(numValidTilesInSlice);
          }
        }
        //container of Alf LCU pointers for tile processing 
        vpSliceTileAlfLCU[numValidTilesInSlice-1].push_back(pcAlfLCU);
      }

      assert( vpSliceAlfLCU.size() == numLCU);
    }
 
    if(m_bUseNonCrossALF)
    {
      m_pcSliceYuvTmp = pcPic->getYuvPicBufferForIndependentBoundaryProcessing();
    }
#if !LCU_SYNTAX_ALF
  }
#endif

}

/** Destroy ALF slice units
 */
Void TComAdaptiveLoopFilter::destroyPicAlfInfo()
{
#if !LCU_SYNTAX_ALF
  if(m_bUseNonCrossALF)
  {
#endif
    for(Int s=0; s< m_uiNumSlicesInPic; s++)
    {
      if(m_ppSliceAlfLCUs[s] != NULL)
      {
        delete[] m_ppSliceAlfLCUs[s];
        m_ppSliceAlfLCUs[s] = NULL;
      }
    }
    delete[] m_ppSliceAlfLCUs;
    m_ppSliceAlfLCUs = NULL;

    delete[] m_pvpAlfLCU;
    m_pvpAlfLCU = NULL;

    delete[] m_pvpSliceTileAlfLCU;
    m_pvpSliceTileAlfLCU = NULL;
#if !LCU_SYNTAX_ALF
  }
#endif
}

#if !LCU_SYNTAX_ALF
/** ALF for cu-on/off-controlled region
 * \param vpAlfLCU ALF LCU information container
 * \param imgDec to-be-filtered picture buffer
 * \param imgRest filtered picture buffer
 * \param stride picture buffer stride size
 * \param filtNo filter shape
 * \param filterCoeff filter coefficients
 * \param mergeTable merge table for filter set
 * \param varImg BA index 
 */
Void TComAdaptiveLoopFilter::xCUAdaptiveRegion(std::vector<AlfLCUInfo*> &vpAlfLCU, Pel* imgDec, Pel* imgRest, Int stride, Int filtNo, Int** filterCoeff, Int* mergeTable, Pel** varImg)
{
  UInt SUWidth   = m_pcPic->getMinCUWidth();
  UInt SUHeight  = m_pcPic->getMinCUHeight();
  UInt idx, startSU, endSU, currSU, LCUX, LCUY, LPelX, TPelY;
  TComDataCU* pcCU;

  for(idx=0; idx< vpAlfLCU.size(); idx++)
  {
    AlfLCUInfo&    rAlfLCU = *(vpAlfLCU[idx]);
    pcCU                   = rAlfLCU.pcCU;
    startSU              = rAlfLCU.startSU;
    endSU                = rAlfLCU.endSU;
    LCUX                 = pcCU->getCUPelX();
    LCUY                 = pcCU->getCUPelY();

    for(currSU= startSU; currSU<= endSU; currSU++)
    {
      LPelX   = LCUX + g_auiRasterToPelX[ g_auiZscanToRaster[currSU] ];
      TPelY   = LCUY + g_auiRasterToPelY[ g_auiZscanToRaster[currSU] ];
      if( !( LPelX < m_img_width )  || !( TPelY < m_img_height )  )
      {
        continue;
      }
      if(pcCU->getAlfCtrlFlag(currSU))
      {
        filterLuma(imgRest, imgDec, stride, TPelY, TPelY+ SUHeight-1, LPelX, LPelX+ SUWidth-1,  filtNo, m_filterCoeffSym, m_varIndTab, m_varImg);
      }
    }
  }

}

/** ALF for "non" cu-on/off-controlled region
 * \param vpAlfLCU ALF LCU information container
 * \param imgDec to-be-filtered picture buffer
 * \param imgRest filtered picture buffer
 * \param stride picture buffer stride size
 * \param filtNo filter shape
 * \param filterCoeff filter coefficients
 * \param mergeTable merge table for filter set
 * \param varImg BA index 
 */
Void TComAdaptiveLoopFilter::filterLumaRegion(std::vector<AlfLCUInfo*> &vpAlfLCU, Pel* imgDec, Pel* imgRest, Int stride, Int filtNo, Int** filterCoeff, Int* mergeTable, Pel** varImg)
{

  Int height, width;
  Int ypos, xpos;

  for(Int i=0; i< vpAlfLCU.size(); i++)
  {
    AlfLCUInfo& rAlfLCU = *(vpAlfLCU[i]); 
    for(UInt j=0; j< rAlfLCU.numSGU; j++)
    {
      ypos   = (Int)(rAlfLCU[j].posY  );
      xpos   = (Int)(rAlfLCU[j].posX  );
      height = (Int)(rAlfLCU[j].height);
      width  = (Int)(rAlfLCU[j].width );

      filterLuma(imgRest, imgDec, stride, ypos, ypos+ height-1, xpos, xpos+ width-1,  filtNo, filterCoeff, mergeTable, varImg);
    }
  }
}


/** Perform ALF for one chroma region
 * \param vpAlfLCU ALF LCU data container
 * \param pDec to-be-filtered picture buffer
 * \param pRest filtered picture buffer
 * \param stride picture buffer stride
 * \param coeff  filter coefficients
 * \param filtNo filter shape
 * \param chromaFormatShift chroma component size adjustment (1 for 4:2:0)
 */
Void TComAdaptiveLoopFilter::filterChromaRegion(std::vector<AlfLCUInfo*> &vpAlfLCU, Pel* pDec, Pel* pRest, Int stride, Int *coeff, Int filtNo, Int chromaFormatShift)
{
  Int height, width;
  Int ypos, xpos;

  for(Int i=0; i< vpAlfLCU.size(); i++)
  {
    AlfLCUInfo& cAlfLCU = *(vpAlfLCU[i]);
    for(Int j=0; j< cAlfLCU.numSGU; j++)
    {
      ypos   = (Int)(cAlfLCU[j].posY   >> chromaFormatShift);
      xpos   = (Int)(cAlfLCU[j].posX   >> chromaFormatShift);
      height = (Int)(cAlfLCU[j].height >> chromaFormatShift);
      width  = (Int)(cAlfLCU[j].width  >> chromaFormatShift);

      filterChroma(pRest, pDec, stride, ypos, ypos+ height -1, xpos, xpos+ width-1, filtNo, coeff);
    }
  }
}

#endif

/** Copy ALF CU control flags from ALF parameters for slices
 * \param [in] vAlfParamSlices ALF CU control parameters
 */
Void TComAdaptiveLoopFilter::transferCtrlFlagsFromAlfParam(std::vector<AlfCUCtrlInfo>& vAlfParamSlices)
{
  assert(m_uiNumSlicesInPic == vAlfParamSlices.size());

  for(UInt s=0; s< m_uiNumSlicesInPic; s++)
  {
    AlfCUCtrlInfo& rAlfParam = vAlfParamSlices[s];
    transferCtrlFlagsFromAlfParamOneSlice( m_pvpAlfLCU[s], 
      (rAlfParam.cu_control_flag ==1)?true:false, 
      rAlfParam.alf_max_depth, 
      rAlfParam.alf_cu_flag
      );
  }
}
/** Copy ALF CU control flags from ALF CU control parameters for one slice
 * \param [in] vpAlfLCU ALF LCU data container 
 * \param [in] bCUCtrlEnabled true for ALF CU control enabled
 * \param [in] iAlfDepth ALF CU control depth
 * \param [in] vCtrlFlags ALF CU control flags
 */
Void TComAdaptiveLoopFilter::transferCtrlFlagsFromAlfParamOneSlice(std::vector< AlfLCUInfo* > &vpAlfLCU, Bool bCUCtrlEnabled, Int iAlfDepth, std::vector<UInt>& vCtrlFlags)
{

  if(!bCUCtrlEnabled)
  {
    for(Int idx=0; idx< vpAlfLCU.size(); idx++)
    {
      AlfLCUInfo& cAlfLCU = *(vpAlfLCU[idx]);
      if(cAlfLCU.pcCU==0)
      {
        return;
      }
      if( cAlfLCU.bAllSUsInLCUInSameSlice)
      {
        cAlfLCU.pcCU->setAlfCtrlFlagSubParts(1, 0, 0);
      }
      else
      {
        for(UInt uiCurrSU= cAlfLCU.startSU; uiCurrSU<= cAlfLCU.endSU; uiCurrSU++)
        {
          cAlfLCU.pcCU->setAlfCtrlFlag(uiCurrSU, 1);
        }
      }
    }
    return;
  }

  UInt uiNumCtrlFlags = 0;
  for(Int idx=0; idx< vpAlfLCU.size(); idx++)
  {
    AlfLCUInfo& cAlfLCU = *(vpAlfLCU[idx]);

    if(cAlfLCU.pcCU==NULL)
    {
      continue;
    }
    uiNumCtrlFlags += (UInt)getCtrlFlagsFromAlfParam(&cAlfLCU, iAlfDepth, &(vCtrlFlags[uiNumCtrlFlags]) );
  }
}

/** Copy region pixels
 * \param vpAlfLCU ALF LCU data container
 * \param pPicDst destination picture buffer
 * \param pPicSrc source picture buffer
 * \param stride stride size of picture buffer
 * \param formatShift region size adjustment according to component size
 */
Void TComAdaptiveLoopFilter::copyRegion(std::vector< AlfLCUInfo* > &vpAlfLCU, Pel* pPicDst, Pel* pPicSrc, Int stride, Int formatShift)
{
  Int extSize = 4;
  Int posX, posY, width, height, offset;
  Pel *pPelDst, *pPelSrc;
  
  for(Int idx =0; idx < vpAlfLCU.size(); idx++)
  {
    AlfLCUInfo& cAlfLCU = *(vpAlfLCU[idx]);
    for(Int n=0; n < cAlfLCU.numSGU; n++)
    {
      NDBFBlockInfo& rSGU = cAlfLCU[n];

      posX     = (Int)(rSGU.posX   >> formatShift);
      posY     = (Int)(rSGU.posY   >> formatShift);
      width    = (Int)(rSGU.width  >> formatShift);
      height   = (Int)(rSGU.height >> formatShift);
      offset   = ( (posY- extSize) * stride)+ (posX -extSize);
      pPelDst  = pPicDst + offset;    
      pPelSrc  = pPicSrc + offset;    

      for(Int j=0; j< (height + (extSize<<1)); j++)
      {
        ::memcpy(pPelDst, pPelSrc, sizeof(Pel)*(width + (extSize<<1)));
        pPelDst += stride;
        pPelSrc += stride;
      }
    }
  }
}

/** Extend region boundary 
 * \param [in] vpAlfLCU ALF LCU data container
 * \param [in,out] pPelSrc picture buffer
 * \param [in] stride stride size of picture buffer
 * \param [in] formatShift region size adjustment according to component size
 */
Void TComAdaptiveLoopFilter::extendRegionBorder(std::vector< AlfLCUInfo* > &vpAlfLCU, Pel* pPelSrc, Int stride, Int formatShift)
{
  UInt extSize = 4;
  UInt width, height;
  UInt posX, posY;
  Pel* pPel;
  Bool* pbAvail;
  for(Int idx = 0; idx < vpAlfLCU.size(); idx++)
  {
    AlfLCUInfo& rAlfLCU = *(vpAlfLCU[idx]);
    for(Int n =0; n < rAlfLCU.numSGU; n++)
    {
      NDBFBlockInfo& rSGU = rAlfLCU[n];
#if LCU_SYNTAX_ALF
      if(rSGU.allBordersAvailable)
      {
        continue;
      }
#endif
      posX     = rSGU.posX >> formatShift;
      posY     = rSGU.posY >> formatShift;
      width    = rSGU.width >> formatShift;
      height   = rSGU.height >> formatShift;
      pbAvail  = rSGU.isBorderAvailable;    
      pPel     = pPelSrc + (posY * stride)+ posX;    
      extendBorderCoreFunction(pPel, stride, pbAvail, width, height, extSize);
    }
  }
}

/** Core function for extending slice/tile boundary 
 * \param [in, out] pPel processing block pointer
 * \param [in] stride picture buffer stride
 * \param [in] pbAvail neighboring blocks availabilities
 * \param [in] width block width
 * \param [in] height block height
 * \param [in] extSize boundary extension size
 */
Void TComAdaptiveLoopFilter::extendBorderCoreFunction(Pel* pPel, Int stride, Bool* pbAvail, UInt width, UInt height, UInt extSize)
{
  Pel* pPelDst;
  Pel* pPelSrc;
  Int i, j;

  for(Int pos =0; pos < NUM_SGU_BORDER; pos++)
  {
    if(pbAvail[pos])
    {
      continue;
    }

    switch(pos)
    {
    case SGU_L:
      {
        pPelDst = pPel - extSize;
        pPelSrc = pPel;
        for(j=0; j< height; j++)
        {
          for(i=0; i< extSize; i++)
          {
            pPelDst[i] = *pPelSrc;
          }
          pPelDst += stride;
          pPelSrc += stride;
        }
      }
      break;
    case SGU_R:
      {
        pPelDst = pPel + width;
        pPelSrc = pPelDst -1;
        for(j=0; j< height; j++)
        {
          for(i=0; i< extSize; i++)
          {
            pPelDst[i] = *pPelSrc;
          }
          pPelDst += stride;
          pPelSrc += stride;
        }

      }
      break;
    case SGU_T:
      {
        pPelSrc = pPel;
        pPelDst = pPel - stride;
        for(j=0; j< extSize; j++)
        {
          ::memcpy(pPelDst, pPelSrc, sizeof(Pel)*width);
          pPelDst -= stride;
        }
      }
      break;
    case SGU_B:
      {
        pPelDst = pPel + height*stride;
        pPelSrc = pPelDst - stride;
        for(j=0; j< extSize; j++)
        {
          ::memcpy(pPelDst, pPelSrc, sizeof(Pel)*width);
          pPelDst += stride;
        }

      }
      break;
    case SGU_TL:
      {
        if( (!pbAvail[SGU_T]) && (!pbAvail[SGU_L]))
        {
          pPelSrc = pPel  - extSize;
          pPelDst = pPelSrc - stride;
          for(j=0; j< extSize; j++)
          {
            ::memcpy(pPelDst, pPelSrc, sizeof(Pel)*extSize);
            pPelDst -= stride;
          }         
        }
      }
      break;
    case SGU_TR:
      {
        if( (!pbAvail[SGU_T]) && (!pbAvail[SGU_R]))
        {
          pPelSrc = pPel + width;
          pPelDst = pPelSrc - stride;
          for(j=0; j< extSize; j++)
          {
            ::memcpy(pPelDst, pPelSrc, sizeof(Pel)*extSize);
            pPelDst -= stride;
          }

        }

      }
      break;
    case SGU_BL:
      {
        if( (!pbAvail[SGU_B]) && (!pbAvail[SGU_L]))
        {
          pPelDst = pPel + height*stride; pPelDst-= extSize;
          pPelSrc = pPelDst - stride;
          for(j=0; j< extSize; j++)
          {
            ::memcpy(pPelDst, pPelSrc, sizeof(Pel)*extSize);
            pPelDst += stride;
          }

        }
      }
      break;
    case SGU_BR:
      {
        if( (!pbAvail[SGU_B]) && (!pbAvail[SGU_R]))
        {
          pPelDst = pPel + height*stride; pPelDst += width;
          pPelSrc = pPelDst - stride;
          for(j=0; j< extSize; j++)
          {
            ::memcpy(pPelDst, pPelSrc, sizeof(Pel)*extSize);
            pPelDst += stride;
          }
        }
      }
      break;
    default:
      {
        printf("Not a legal neighboring availability\n");
        assert(0);
        exit(-1);
      }
    }
  }
}

/** Assign ALF on/off-control flags from ALF parameters to CU data
 * \param [in] pcAlfLCU processing ALF LCU data pointer
 * \param [in] alfDepth ALF on/off-control depth
 * \param [in] pFlags on/off-control flags buffer in ALF parameters
 */
Int TComAdaptiveLoopFilter::getCtrlFlagsFromAlfParam(AlfLCUInfo* pcAlfLCU, Int alfDepth, UInt* pFlags)
{

  const UInt startSU               = pcAlfLCU->startSU;
  const UInt endSU                 = pcAlfLCU->endSU;
  const Bool bAllSUsInLCUInSameSlice = pcAlfLCU->bAllSUsInLCUInSameSlice;
  const UInt maxNumSUInLCU         = m_pcPic->getNumPartInCU();

  TComDataCU* pcCU = pcAlfLCU->pcCU;
  Int   numCUCtrlFlags = 0;

  UInt  currSU, CUDepth, setDepth, ctrlNumSU;
  UInt  alfFlag;

  currSU = startSU;
  if( bAllSUsInLCUInSameSlice ) 
  {
    while(currSU < maxNumSUInLCU)
    {
      //depth of this CU
      CUDepth = pcCU->getDepth(currSU);

      //choose the min. depth for ALF
      setDepth   = (alfDepth < CUDepth)?(alfDepth):(CUDepth);
      ctrlNumSU = maxNumSUInLCU >> (setDepth << 1);

      alfFlag= pFlags[numCUCtrlFlags];

      pcCU->setAlfCtrlFlagSubParts(alfFlag, currSU, (UInt)setDepth);

      numCUCtrlFlags++;
      currSU += ctrlNumSU;
    }
    return numCUCtrlFlags;
  }


  UInt  LCUX    = pcCU->getCUPelX();
  UInt  LCUY    = pcCU->getCUPelY();

  Bool  bFirst, bValidCU;
  UInt  idx, LPelXSU, TPelYSU;

  bFirst= true;
  while(currSU <= endSU)
  {
    //check picture boundary
    while(!( LCUX + g_auiRasterToPelX[ g_auiZscanToRaster[currSU] ] < m_img_width  ) || 
          !( LCUY + g_auiRasterToPelY[ g_auiZscanToRaster[currSU] ] < m_img_height )
      )
    {
      currSU++;
      if(currSU >= maxNumSUInLCU || currSU > endSU)
      {
        break;
      }
    }

    if(currSU >= maxNumSUInLCU || currSU > endSU)
    {
      break;
    }

    //depth of this CU
    CUDepth = pcCU->getDepth(currSU);

    //choose the min. depth for ALF
    setDepth   = (alfDepth < CUDepth)?(alfDepth):(CUDepth);
    ctrlNumSU = maxNumSUInLCU >> (setDepth << 1);

    if(bFirst)
    {
      if(currSU !=0 )
      {
        currSU = ((UInt)(currSU/ctrlNumSU))* ctrlNumSU;
      }
      bFirst = false;
    }

    //alf flag for this CU
    alfFlag= pFlags[numCUCtrlFlags];

    bValidCU = false;
    for(idx = currSU; idx < currSU + ctrlNumSU; idx++)
    {
      if(idx < startSU || idx > endSU)
      {
        continue;
      }

      LPelXSU   = LCUX + g_auiRasterToPelX[ g_auiZscanToRaster[idx] ];
      TPelYSU   = LCUY + g_auiRasterToPelY[ g_auiZscanToRaster[idx] ];

      if( !( LPelXSU < m_img_width )  || !( TPelYSU < m_img_height )  )
      {
        continue;
      }

      bValidCU = true;
      pcCU->setAlfCtrlFlag(idx, alfFlag);
    }

    if(bValidCU)
    {
      numCUCtrlFlags++;
    }

    currSU += ctrlNumSU;
  }

  return numCUCtrlFlags;
}

#if LCU_SYNTAX_ALF
/** reconstruct ALF luma coefficient
 * \param [in] alfLCUParam ALF parameters 
 * \param [out] filterCoeff reconstructed luma coefficients
 */
Void TComAdaptiveLoopFilter::reconstructLumaCoefficients(ALFParam* alfLCUParam, Int** filterCoeff)
{
  Int sum, coeffPred, ind;
#if LCUALF_QP_DEPENDENT_BITS
  Int alfPrecisionBit = getAlfPrecisionBit( m_alfQP );
#endif
  const Int* filtMag = NULL;
  filtMag = weightsTabShapes[0];

  // Undo intra-filter prediction
  for(ind = 0; ind < alfLCUParam->filters_per_group; ind++)
  {
    sum = 0;

    for(Int i = 0; i < alfLCUParam->num_coeff-2; i++)
    {
      sum += (filtMag[i] * alfLCUParam->coeffmulti[ind][i]);
      filterCoeff[ind][i] = alfLCUParam->coeffmulti[ind][i];
    }

    if(alfLCUParam->nbSPred[ind] == 0)
    {
      if((alfLCUParam->predMethod==0)|(ind==0))
      {
#if LCUALF_QP_DEPENDENT_BITS
        coeffPred = ((1<<alfPrecisionBit)-sum) >> 2;
#else
        coeffPred = ((1<<ALF_NUM_BIT_SHIFT)-sum) >> 2;
#endif
      }
      else
      {
        coeffPred = (0-sum) >> 2;
      }

      filterCoeff[ind][alfLCUParam->num_coeff-2] = coeffPred + alfLCUParam->coeffmulti[ind][alfLCUParam->num_coeff-2];
    }
    else
    {
      filterCoeff[ind][alfLCUParam->num_coeff-2] = alfLCUParam->coeffmulti[ind][alfLCUParam->num_coeff-2];
    }

    sum += filtMag[alfLCUParam->num_coeff-2] * filterCoeff[ind][alfLCUParam->num_coeff-2];

    if((alfLCUParam->predMethod==0)|(ind==0))
    {
#if LCUALF_QP_DEPENDENT_BITS
      coeffPred = (1<<alfPrecisionBit)-sum;
#else
      coeffPred = (1<<ALF_NUM_BIT_SHIFT)-sum;
#endif
    }
    else
    {
      coeffPred = -sum;
    }

    filterCoeff[ind][alfLCUParam->num_coeff-1] = coeffPred + alfLCUParam->coeffmulti[ind][alfLCUParam->num_coeff-1];
  }


  // Undo inter-filter prediction
  for(ind = 1; ind < alfLCUParam->filters_per_group; ind++)
  {
    if(alfLCUParam->predMethod)
    {
      // Prediction
      for(Int i = 0; i < alfLCUParam->num_coeff; i++)
      {
        filterCoeff[ind][i] = (int)(filterCoeff[ind][i] + filterCoeff[ind - 1][i]);
      }
    }
  }

}


/** reconstruct ALF chroma coefficient
 * \param [in] alfLCUParam ALF parameters 
 * \param [out] filterCoeff reconstructed chroma coefficients
 */
Void TComAdaptiveLoopFilter::reconstructChromaCoefficients(ALFParam* alfLCUParam, Int** filterCoeff)
{
  Int sum = 0;
  Int i, coeffPred;
  const Int* filtMag = NULL;
  filtMag = weightsTabShapes[0];

  for(i=0; i<alfLCUParam->num_coeff-1; i++)
  {
    sum += (filtMag[i] * alfLCUParam->coeffmulti[0][i]);
    filterCoeff[0][i] = alfLCUParam->coeffmulti[0][i];
  }

#if LCUALF_QP_DEPENDENT_BITS
  Int alfPrecisionBit = getAlfPrecisionBit( m_alfQP );
  coeffPred = (1<<alfPrecisionBit) - sum;
#else
  coeffPred = (1<<ALF_NUM_BIT_SHIFT) - sum;
#endif
#if ALF_CHROMA_COEF_PRED_HARMONIZATION
  filterCoeff[0][alfLCUParam->num_coeff-1] = coeffPred + alfLCUParam->coeffmulti[0][alfLCUParam->num_coeff-1];
#else
  filterCoeff[0][alfLCUParam->num_coeff-1] = coeffPred - alfLCUParam->coeffmulti[0][alfLCUParam->num_coeff-1];
#endif
}


/** reconstruct ALF coefficient
 * \param [in] compIdx component index
 * \param [in] alfLCUParam ALF parameters 
 * \param [out] filterCoeff reconstructed coefficients
 * \param [out] varIndTab the merged groups in block-based adaptation mode
 */
Void TComAdaptiveLoopFilter::reconstructCoefInfo(Int compIdx, ALFParam* alfLCUParam, Int** filterCoeff, Int* varIndTab)
{
  switch(compIdx)
  {
  case ALF_Cb:
  case ALF_Cr:
    {
      alfLCUParam->filters_per_group = 1;
      reconstructChromaCoefficients(alfLCUParam, filterCoeff);
    }
    break;
  case ALF_Y:
    {
      ::memset(varIndTab, 0, NO_VAR_BINS * sizeof(Int));
      if(alfLCUParam->filters_per_group > 1)
      {
        for(Int i = 1; i < NO_VAR_BINS; ++i)
        {
          if(alfLCUParam->filterPattern[i])
          {
            varIndTab[i] = varIndTab[i-1] + 1;
          }
          else
          {
            varIndTab[i] = varIndTab[i-1];
          }
        }
      }
      reconstructLumaCoefficients(alfLCUParam, filterCoeff);
    }
    break;
  default:
    {
      printf("not legal component ID for ALF\n");
      assert(0);
      exit(-1);
    }
  }

}


/** filter process with CU-On/Off control
 * \param [in] alfLCUParam ALF parameters 
 * \param [in] regionLCUInfo ALF CU-on/off control parameters 
 * \param [in] pDec decoded picture
 * \param [out] pRest filtered picture
 * \param [in] stride picture stride in memory
 * \param [in] caculateBAIdx calculate BA filter index (true) or BA filter index array is ready (false)
 */
Void TComAdaptiveLoopFilter::filterRegionCUControl(ALFParam** alfLCUParams, std::vector<AlfLCUInfo*>& regionLCUInfo, Pel* pDec, Pel* pRest, Int stride, Bool caculateBAIdx)
{
  Int ypos, xpos, currSU, startSU, endSU, lcuX, lcuY;
  Int yposEnd, xposEnd;

  for(Int i=0; i< (Int)regionLCUInfo.size(); i++)
  {
    AlfLCUInfo& alfLCUinfo = *(regionLCUInfo[i]); 
    TComDataCU* pcCU = alfLCUinfo.pcCU;
    Int addr = pcCU->getAddr();

    ALFParam* alfParam = alfLCUParams[addr];

    if(alfParam->alf_flag == 1)
    {
      lcuX    = pcCU->getCUPelX();
      lcuY    = pcCU->getCUPelY();

      if(caculateBAIdx)
      {
        xposEnd = lcuX + g_uiMaxCUWidth;
        yposEnd = lcuY + g_uiMaxCUHeight;

        if(xposEnd > m_img_width)
        {
          xposEnd = m_img_width;
        }

        if(yposEnd > m_img_height)
        {
          yposEnd = m_img_height;
        }

        calcOneRegionVar(m_varImg, pDec, stride, (alfParam->filters_per_group == 1), lcuY, yposEnd, lcuX, xposEnd);        
      }

      //reconstruct ALF coefficients & related parameters 
      reconstructCoefInfo(ALF_Y, alfParam, m_filterCoeffSym, m_varIndTab);

      startSU = alfLCUinfo.startSU;
      endSU   = alfLCUinfo.endSU;


      for(currSU= startSU; currSU<= endSU; currSU++)
      {
        xpos  = lcuX + g_auiRasterToPelX[ g_auiZscanToRaster[currSU] ];
        ypos  = lcuY + g_auiRasterToPelY[ g_auiZscanToRaster[currSU] ];
        if( !( xpos < m_img_width )  || !( ypos < m_img_height )  )
        {
          continue;
        }

        if(pcCU->getAlfCtrlFlag(currSU))
        {
          filterOneCompRegion(pRest, pDec, stride, false, ypos, ypos+m_suHeight, xpos, xpos+m_suWidth, m_filterCoeffSym, m_varIndTab, m_varImg);
        }
      }

    } //alf_flag == 1
  }

}


/** filter process without CU-On/Off control
 * \param [in] alfLCUParam ALF parameters 
 * \param [in] regionLCUInfo ALF CU-on/off control parameters 
 * \param [in] pDec decoded picture
 * \param [out] pRest filtered picture
 * \param [in] stride picture stride in memory
 * \param [in] formatShift luma component (0) or chroma component (1)
 * \param [in] caculateBAIdx calculate BA filter index (true) or BA filter index array is ready (false)
 */
Void TComAdaptiveLoopFilter::filterRegion(Int compIdx, ALFParam** alfLCUParams, std::vector<AlfLCUInfo*>& regionLCUInfo, Pel* pDec, Pel* pRest, Int stride, Int formatShift, Bool caculateBAIdx)
{
  Int height, width;
  Int ypos, xpos,  lcuX, lcuY;
  Int yposEnd, xposEnd;

  for(Int i=0; i< regionLCUInfo.size(); i++)
  {
    AlfLCUInfo& alfLCUinfo = *(regionLCUInfo[i]); 
    TComDataCU* pcCU = alfLCUinfo.pcCU;
    Int addr = pcCU->getAddr();

    ALFParam* alfParam = alfLCUParams[addr];

    if(alfParam->alf_flag == 1)
    {
      lcuX    = pcCU->getCUPelX();
      lcuY    = pcCU->getCUPelY();

      if(caculateBAIdx)
      {
        assert(compIdx == ALF_Y);

        xposEnd = lcuX + g_uiMaxCUWidth;
        yposEnd = lcuY + g_uiMaxCUHeight;

        if(xposEnd > m_img_width)
        {
          xposEnd = m_img_width;
        }

        if(yposEnd > m_img_height)
        {
          yposEnd = m_img_height;
        }

        calcOneRegionVar(m_varImg, pDec, stride, (alfParam->filters_per_group == 1), lcuY, yposEnd, lcuX, xposEnd);        
      }

      //reconstruct ALF coefficients & related parameters 
      reconstructCoefInfo(compIdx, alfParam, m_filterCoeffSym, m_varIndTab);

      //filtering process
      for(Int j=0; j< alfLCUinfo.numSGU; j++)
      {
        ypos    = (Int)(alfLCUinfo[j].posY   >> formatShift);
        xpos    = (Int)(alfLCUinfo[j].posX   >> formatShift);
        height = (Int)(alfLCUinfo[j].height >> formatShift);
        width  = (Int)(alfLCUinfo[j].width  >> formatShift);

        filterOneCompRegion(pRest, pDec, stride, (compIdx!=ALF_Y), ypos, ypos+height, xpos, xpos+width, m_filterCoeffSym, m_varIndTab, m_varImg);
      }
    } //alf_flag == 1
  }
}


/** predict chroma center coefficient
 * \param [in] coeff ALF chroma coefficient
 * \param [in] numCoef number of chroma coefficients
 */
Void TComAdaptiveLoopFilter::predictALFCoeffChroma(Int* coeff, Int numCoef)
{
  Int sum=0;
  for(Int i=0; i< numCoef-1;i++)
  {
    sum += (2* coeff[i]);
  }

#if LCUALF_QP_DEPENDENT_BITS 
  Int alfPrecisionBit = getAlfPrecisionBit( m_alfQP );
  Int pred = (1<<alfPrecisionBit) - (sum);
#else
  Int pred = (1<<ALF_NUM_BIT_SHIFT) - (sum);
#endif
#if ALF_CHROMA_COEF_PRED_HARMONIZATION
  coeff[numCoef-1] = coeff[numCoef-1] - pred;
#else
  coeff[numCoef-1] = pred- coeff[numCoef-1];
#endif
}

#if ALF_SINGLE_FILTER_SHAPE 
/** filtering pixels
 * \param [out] imgRes filtered picture
 * \param [in] imgPad decoded picture 
 * \param [in] stride picture stride in memory
 * \param [in] isChroma chroma component (true) or luma component (false)
 * \param [in] yPos y position of the top-left pixel in one to-be-filtered region
 * \param [in] yPosEnd y position of the right-bottom pixel in one to-be-filtered region
 * \param [in] xPos x position of the top-left pixel in one to-be-filtered region
 * \param [in] xPosEnd x position of the right-bottom pixel in one to-be-filtered region
 * \param [in] filterSet filter coefficients
 * \param [in] mergeTable the merged groups in block-based adaptation mode
 * \param [in] varImg BA filter index array 
 */
Void TComAdaptiveLoopFilter::filterOneCompRegion(Pel *imgRes, Pel *imgPad, Int stride, Bool isChroma
                                                , Int yPos, Int yPosEnd, Int xPos, Int xPosEnd
                                                , Int** filterSet, Int* mergeTable, Pel** varImg
                                                )
{
#if LCUALF_QP_DEPENDENT_BITS  
  Int alfPrecisionBit = getAlfPrecisionBit( m_alfQP );
  Int numBitsMinus1   = alfPrecisionBit;
  Int offset          = (1<<(alfPrecisionBit-1));
#else
  static Int numBitsMinus1= (Int)ALF_NUM_BIT_SHIFT;
  static Int offset       = (1<<( (Int)ALF_NUM_BIT_SHIFT-1));
#endif
  static Int shiftHeight  = (Int)(log((double)VAR_SIZE_H)/log(2.0));
  static Int shiftWidth   = (Int)(log((double)VAR_SIZE_W)/log(2.0));

  Pel *imgPad1,*imgPad2,*imgPad3, *imgPad4, *imgPad5, *imgPad6;
  Pel *var = varImg[yPos>>shiftHeight] + (xPos>>shiftWidth);;
  Int i, j, pixelInt;
  Int *coef = NULL;

  coef    = filterSet[0];
  imgPad += (yPos*stride);
  imgRes += (yPos*stride);

  Int yLineInLCU;
  Int paddingLine;
  //Int varInd = 0;
  Int lcuHeight     = isChroma ? m_lcuHeightChroma     : m_lcuHeight;
  Int lineIdxPadBot = isChroma ? m_lineIdxPadBotChroma : m_lineIdxPadBot;
  Int lineIdxPadTop = isChroma ? m_lineIdxPadTopChroma : m_lineIdxPadTop;
  Int img_height    = isChroma ? m_img_height>>1       : m_img_height;

  for(i= yPos; i< yPosEnd; i++)
  {
    yLineInLCU = i % lcuHeight;

    if(isChroma && yLineInLCU == 0 && i>0)
    {
      paddingLine = yLineInLCU + 2;
      imgPad1 = imgPad + stride;
      imgPad2 = imgPad - stride;
      imgPad3 = imgPad + 2*stride;
      imgPad4 = imgPad - 2*stride;
      imgPad5 = (paddingLine < 3) ? imgPad : imgPad + 3*stride;
      imgPad6 = (paddingLine < 3) ? imgPad : imgPad - min(paddingLine, 3)*stride;
    }
    else if(yLineInLCU<lineIdxPadBot || i-yLineInLCU+lcuHeight >= img_height)
    {
      imgPad1 = imgPad +   stride;
      imgPad2 = imgPad -   stride;
      imgPad3 = imgPad + 2*stride;
      imgPad4 = imgPad - 2*stride;
      imgPad5 = imgPad + 3*stride;
      imgPad6 = imgPad - 3*stride;
    }
    else if (yLineInLCU<lineIdxPadTop)
    {
      paddingLine = - yLineInLCU + lineIdxPadTop - 1;
      imgPad1 = (paddingLine < 1) ? imgPad : imgPad + min(paddingLine, 1)*stride;
      imgPad2 = (paddingLine < 1) ? imgPad : imgPad - stride;
      imgPad3 = (paddingLine < 2) ? imgPad : imgPad + min(paddingLine, 2)*stride;
      imgPad4 = (paddingLine < 2) ? imgPad : imgPad - 2*stride;
      imgPad5 = (paddingLine < 3) ? imgPad : imgPad + min(paddingLine, 3)*stride;
      imgPad6 = (paddingLine < 3) ? imgPad : imgPad - 3*stride;
    }
    else
    {
      paddingLine = yLineInLCU - lineIdxPadTop ;
      imgPad1 = (paddingLine < 1) ? imgPad : imgPad + stride;
      imgPad2 = (paddingLine < 1) ? imgPad : imgPad - min(paddingLine, 1)*stride;
      imgPad3 = (paddingLine < 2) ? imgPad : imgPad + 2*stride;
      imgPad4 = (paddingLine < 2) ? imgPad : imgPad - min(paddingLine, 2)*stride;
      imgPad5 = (paddingLine < 3) ? imgPad : imgPad + 3*stride;
      imgPad6 = (paddingLine < 3) ? imgPad : imgPad - min(paddingLine, 3)*stride;
    } 

    if(!isChroma)
    {
      var = varImg[i>>shiftHeight] + (xPos>>shiftWidth);
    }

    for(j= xPos; j< xPosEnd ; j++)
    {
      if (!isChroma && j % VAR_SIZE_W==0) 
      {
        coef = filterSet[mergeTable[*(var++)]];
      }

      pixelInt  = coef[0]* (imgPad5[j  ] + imgPad6[j  ]);

      pixelInt += coef[1]* (imgPad3[j  ] + imgPad4[j  ]);

      pixelInt += coef[2]* (imgPad1[j-1] + imgPad2[j+1]);
      pixelInt += coef[3]* (imgPad1[j  ] + imgPad2[j  ]);
      pixelInt += coef[4]* (imgPad1[j+1] + imgPad2[j-1]);

      pixelInt += coef[5]* (imgPad[j+4] + imgPad[j-4]);
      pixelInt += coef[6]* (imgPad[j+3] + imgPad[j-3]);
      pixelInt += coef[7]* (imgPad[j+2] + imgPad[j-2]);
      pixelInt += coef[8]* (imgPad[j+1] + imgPad[j-1]);
      pixelInt += coef[9]* (imgPad[j  ]);

      pixelInt=(Int)((pixelInt+offset) >> numBitsMinus1);
      imgRes[j] = Clip( pixelInt );
    }

    imgPad += stride;
    imgRes += stride;
  }  
}
#endif

#if LCUALF_QP_DEPENDENT_BITS
/** filtering pixels
 * \param [in] qp quantization parameter
 */
Int  TComAdaptiveLoopFilter::getAlfPrecisionBit(Int qp)
{
  Int alfPrecisionBit = 8;

  if (qp < ALF_QP1)
  {
    alfPrecisionBit = 8;
  }
  else if (qp < ALF_QP2)
  {
    alfPrecisionBit = 7;
  }
  else if (qp < ALF_QP3)
  {
    alfPrecisionBit = 6;
  }
  else
  {
    alfPrecisionBit = 5;
  }

  return alfPrecisionBit;
}
#endif

/** filtering pixels
 * \param [out] imgYvar BA filter index array
 * \param [in] imgYpad decoded picture 
 * \param [in] stride picture stride in memory
 * \param [in] isOnlyOneGroup only one filter is used (true) or multiple filters are used (false)
 * \param [in] yPos y position of the top-left pixel in one to-be-filtered region
 * \param [in] yPosEnd y position of the right-bottom pixel in one to-be-filtered region
 * \param [in] xPos x position of the top-left pixel in one to-be-filtered region
 * \param [in] xPosEnd x position of the right-bottom pixel in one to-be-filtered region
 */
Void TComAdaptiveLoopFilter::calcOneRegionVar(Pel **imgYvar, Pel *imgYpad, Int stride, Bool isOnlyOneGroup, Int yPos, Int yPosEnd, Int xPos, Int xPosEnd)
{

  static Int shiftH = (Int)(log((double)VAR_SIZE_H)/log(2.0));
  static Int shiftW = (Int)(log((double)VAR_SIZE_W)/log(2.0));
  static Int varMax = (Int)NO_VAR_BINS-1;  
#if ALF_16_BA_GROUPS
  static Int th[NO_VAR_BINS] = {0, 1, 2, 2, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 5}; 
  static Int avgVarTab[3][6] = { {0,  1,  2,  3,  4,  5,},
  {0,  6,  7,  8,  9, 10,},
  {0, 11, 12, 13, 14, 15}   };
#else
  static Int step   = (Int)((Int)(NO_VAR_BINS)/3) - 1;  
  static Int th[NO_VAR_BINS] = {0, 1, 2, 2, 2, 3, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4}; 
#endif

  Int i, j, avgVar, vertical, horizontal, direction, yOffset;
  Pel *imgYPadCur, *imgYPadUp, *imgYPadDown;

  if (isOnlyOneGroup)
  {
    for(i = yPos; i < yPosEnd; i=i+4)
    {
      for(j = xPos; j < xPosEnd; j=j+4)
      {
        imgYvar[i>>shiftH][j>>shiftW] = 0;
      }
    }
    return;
  }

  for(i = yPos; i < yPosEnd; i=i+4)
  {
    yOffset     = (i*stride) + stride;
    imgYPadCur  = &imgYpad[yOffset];
    imgYPadUp   = &imgYpad[yOffset + stride];
    imgYPadDown = &imgYpad[yOffset - stride];

    for(j = xPos; j < xPosEnd; j=j+4)
    {
      // Compute at sub-sample by 2
      vertical   =  abs((imgYPadCur[j+1]<<1) - imgYPadDown[j+1] - imgYPadUp [j+1]);
      horizontal =  abs((imgYPadCur[j+1]<<1) - imgYPadCur [j+2] - imgYPadCur[j  ]);

      vertical   += abs((imgYPadCur[j+2]<<1) - imgYPadDown[j+2] - imgYPadUp [j+2]);
      horizontal += abs((imgYPadCur[j+2]<<1) - imgYPadCur [j+3] - imgYPadCur[j+1]);

      vertical   += abs((imgYPadCur[j+1+stride]<<1) - imgYPadDown[j+1+stride] - imgYPadUp [j+1+stride]);
      horizontal += abs((imgYPadCur[j+1+stride]<<1) - imgYPadCur [j+2+stride] - imgYPadCur[j+stride  ]);

      vertical   += abs((imgYPadCur[j+2+stride]<<1) - imgYPadDown[j+2+stride] - imgYPadUp [j+2+stride]);
      horizontal += abs((imgYPadCur[j+2+stride]<<1) - imgYPadCur [j+3+stride] - imgYPadCur[j+1+stride]);

      direction = 0;
      if (vertical > 2*horizontal) 
      {
        direction = 1; //vertical
      }
      if (horizontal > 2*vertical)
      {
        direction = 2; //horizontal
      }

      avgVar = (vertical + horizontal) >> 2;
      avgVar = (Pel) Clip_post( varMax, avgVar>>(g_uiBitIncrement+1) );
      avgVar = th[avgVar];
#if ALF_16_BA_GROUPS
      avgVar = avgVarTab[direction][avgVar];
#else      
      avgVar = Clip_post(step, (Int)avgVar) + (step+1)*direction;
#endif
      imgYvar[i>>shiftH][j>>shiftW] = avgVar;
    }
  }
}


Void TComAdaptiveLoopFilter::resetLCUAlfInfo()
{
  //reset to all off
  for(Int compIdx = 0; compIdx < NUM_ALF_COMPONENT; compIdx++)
  {
    for(Int n=0; n< m_uiNumCUsInFrame; n++)
    {
      m_alfFiltInfo[compIdx][n]->alf_flag = 0;
    }
  }

}

Void TComAdaptiveLoopFilter::createLCUAlfInfo()
{
  for(Int compIdx = 0; compIdx < NUM_ALF_COMPONENT; compIdx++)
  {
    m_alfFiltInfo[compIdx] = new ALFParam*[m_uiNumCUsInFrame];
    for(Int n=0; n< m_uiNumCUsInFrame; n++)
    {
      m_alfFiltInfo[compIdx][n] = new ALFParam(compIdx);
    }
  }

  resetLCUAlfInfo();
}

Void TComAdaptiveLoopFilter::destroyLCUAlfInfo()
{
  for(Int compIdx = 0; compIdx < NUM_ALF_COMPONENT; compIdx++)
  {
    if(m_alfFiltInfo[compIdx] != NULL)
    {
      for(Int n=0; n< m_uiNumCUsInFrame; n++)
      {
        delete m_alfFiltInfo[compIdx][n];
      }
      delete[] m_alfFiltInfo[compIdx];
      m_alfFiltInfo[compIdx] = NULL;
    }
  }
}

Pel* TComAdaptiveLoopFilter::getPicBuf(TComPicYuv* pPicYuv, Int compIdx)
{
  Pel* pBuf = NULL;
  switch(compIdx)
  {
  case ALF_Y:
    {
      pBuf = pPicYuv->getLumaAddr();
    }
    break;
  case ALF_Cb:
    {
      pBuf = pPicYuv->getCbAddr();
    }
    break;
  case ALF_Cr:
    {
      pBuf = pPicYuv->getCrAddr();
    }
    break;
  default:
    {
      printf("Not a legal component ID for ALF\n");
      assert(0);
      exit(-1);
    }
  }

  return pBuf;
}
#endif



/** PCM LF disable process. 
 * \param pcPic picture (TComPic) pointer
 * \returns Void
 *
 * \note Replace filtered sample values of PCM mode blocks with the transmitted and reconstructed ones.
 */
Void TComAdaptiveLoopFilter::PCMLFDisableProcess (TComPic* pcPic)
{
  xPCMRestoration(pcPic);
}

/** Picture-level PCM restoration. 
 * \param pcPic picture (TComPic) pointer
 * \returns Void
 */
Void TComAdaptiveLoopFilter::xPCMRestoration(TComPic* pcPic)
{
  Bool  bPCMFilter = (pcPic->getSlice(0)->getSPS()->getUsePCM() && pcPic->getSlice(0)->getSPS()->getPCMFilterDisableFlag())? true : false;

#if LOSSLESS_CODING
  if(bPCMFilter || pcPic->getSlice(0)->getSPS()->getUseLossless())
#else
  if(bPCMFilter)
#endif
  {
    for( UInt uiCUAddr = 0; uiCUAddr < pcPic->getNumCUsInFrame() ; uiCUAddr++ )
    {
      TComDataCU* pcCU = pcPic->getCU(uiCUAddr);

      xPCMCURestoration(pcCU, 0, 0); 
    } 
  }
}

/** PCM CU restoration. 
 * \param pcCU pointer to current CU
 * \param uiAbsPartIdx part index
 * \param uiDepth CU depth
 * \returns Void
 */
Void TComAdaptiveLoopFilter::xPCMCURestoration ( TComDataCU* pcCU, UInt uiAbsZorderIdx, UInt uiDepth )
{
  TComPic* pcPic     = pcCU->getPic();
  UInt uiCurNumParts = pcPic->getNumPartInCU() >> (uiDepth<<1);
  UInt uiQNumParts   = uiCurNumParts>>2;

  // go to sub-CU
  if( pcCU->getDepth(uiAbsZorderIdx) > uiDepth )
  {
    for ( UInt uiPartIdx = 0; uiPartIdx < 4; uiPartIdx++, uiAbsZorderIdx+=uiQNumParts )
    {
      UInt uiLPelX   = pcCU->getCUPelX() + g_auiRasterToPelX[ g_auiZscanToRaster[uiAbsZorderIdx] ];
      UInt uiTPelY   = pcCU->getCUPelY() + g_auiRasterToPelY[ g_auiZscanToRaster[uiAbsZorderIdx] ];
      if( ( uiLPelX < pcCU->getSlice()->getSPS()->getPicWidthInLumaSamples() ) && ( uiTPelY < pcCU->getSlice()->getSPS()->getPicHeightInLumaSamples() ) )
        xPCMCURestoration( pcCU, uiAbsZorderIdx, uiDepth+1 );
    }
    return;
  }

  // restore PCM samples
#if LOSSLESS_CODING 
  if ((pcCU->getIPCMFlag(uiAbsZorderIdx)) || pcCU->isLosslessCoded( uiAbsZorderIdx))
#else
  if (pcCU->getIPCMFlag(uiAbsZorderIdx))
#endif
  {
    xPCMSampleRestoration (pcCU, uiAbsZorderIdx, uiDepth, TEXT_LUMA    );
    xPCMSampleRestoration (pcCU, uiAbsZorderIdx, uiDepth, TEXT_CHROMA_U);
    xPCMSampleRestoration (pcCU, uiAbsZorderIdx, uiDepth, TEXT_CHROMA_V);
  }
}

/** PCM sample restoration. 
 * \param pcCU pointer to current CU
 * \param uiAbsPartIdx part index
 * \param uiDepth CU depth
 * \param ttText texture component type
 * \returns Void
 */
Void TComAdaptiveLoopFilter::xPCMSampleRestoration (TComDataCU* pcCU, UInt uiAbsZorderIdx, UInt uiDepth, TextType ttText)
{
  TComPicYuv* pcPicYuvRec = pcCU->getPic()->getPicYuvRec();
  Pel* piSrc;
  Pel* piPcm;
  UInt uiStride;
  UInt uiWidth;
  UInt uiHeight;
  UInt uiPcmLeftShiftBit; 
  UInt uiX, uiY;
  UInt uiMinCoeffSize = pcCU->getPic()->getMinCUWidth()*pcCU->getPic()->getMinCUHeight();
  UInt uiLumaOffset   = uiMinCoeffSize*uiAbsZorderIdx;
  UInt uiChromaOffset = uiLumaOffset>>2;

  if( ttText == TEXT_LUMA )
  {
    piSrc = pcPicYuvRec->getLumaAddr( pcCU->getAddr(), uiAbsZorderIdx);
    piPcm = pcCU->getPCMSampleY() + uiLumaOffset;
    uiStride  = pcPicYuvRec->getStride();
    uiWidth  = (g_uiMaxCUWidth >> uiDepth);
    uiHeight = (g_uiMaxCUHeight >> uiDepth);
#if LOSSLESS_CODING 
    if ( pcCU->isLosslessCoded(uiAbsZorderIdx) )
    {
      uiPcmLeftShiftBit = 0;
    }
    else
#endif
    {
        uiPcmLeftShiftBit = g_uiBitDepth + g_uiBitIncrement - pcCU->getSlice()->getSPS()->getPCMBitDepthLuma();
    }
  }
  else
  {
    if( ttText == TEXT_CHROMA_U )
    {
      piSrc = pcPicYuvRec->getCbAddr( pcCU->getAddr(), uiAbsZorderIdx );
      piPcm = pcCU->getPCMSampleCb() + uiChromaOffset;
    }
    else
    {
      piSrc = pcPicYuvRec->getCrAddr( pcCU->getAddr(), uiAbsZorderIdx );
      piPcm = pcCU->getPCMSampleCr() + uiChromaOffset;
    }

    uiStride = pcPicYuvRec->getCStride();
    uiWidth  = ((g_uiMaxCUWidth >> uiDepth)/2);
    uiHeight = ((g_uiMaxCUWidth >> uiDepth)/2);
#if LOSSLESS_CODING 
    if ( pcCU->isLosslessCoded(uiAbsZorderIdx) )
    {
      uiPcmLeftShiftBit = 0;
    }
    else
#endif
    {
      uiPcmLeftShiftBit = g_uiBitDepth + g_uiBitIncrement - pcCU->getSlice()->getSPS()->getPCMBitDepthChroma();
    }
  }

  for( uiY = 0; uiY < uiHeight; uiY++ )
  {
    for( uiX = 0; uiX < uiWidth; uiX++ )
    {
      piSrc[uiX] = (piPcm[uiX] << uiPcmLeftShiftBit);
    }
    piPcm += uiWidth;
    piSrc += uiStride;
  }
}
//! \}
