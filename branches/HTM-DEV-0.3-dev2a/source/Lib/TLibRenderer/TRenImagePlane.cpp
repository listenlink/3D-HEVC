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


#include "TRenImagePlane.h"
#include "TRenFilter.h"
#include <string.h>
#if H_3D

/////// TRenImagePlane ///////

template<class T>
TRenImagePlane<T>::TRenImagePlane() { m_bClean = true; }

template<class T>
TRenImagePlane<T>::TRenImagePlane(UInt uiWidth, UInt uiHeight, UInt uiPad)
: m_uiWidth(uiWidth), m_uiHeight(uiHeight), m_uiStride(uiWidth+2*uiPad), m_uiWidthOrg(uiWidth+2*uiPad), m_uiHeightOrg(uiHeight+2*uiPad), m_uiPad(uiPad)
{
  m_pcDataOrg = new T[ m_uiWidthOrg * m_uiHeightOrg ];
  m_pcData    = m_pcDataOrg + m_uiPad * m_uiStride + m_uiPad;
  m_bClean    = true;
}

template<class T>
TRenImagePlane<T>::TRenImagePlane(TRenImagePlane* pcPlane)
: m_uiWidth   (pcPlane->getWidth   ())
, m_uiHeight  (pcPlane->getHeight  ())
, m_uiStride  (pcPlane->getStride  ())
, m_uiWidthOrg(pcPlane->getWidthOrg())
, m_uiHeightOrg(pcPlane->getHeightOrg())
, m_uiPad     (pcPlane->getPad     ())
{
  m_pcData = new T[m_uiWidthOrg*m_uiHeightOrg];
  m_bClean = true;
  assign( pcPlane );
}

template<typename T>
TRenImagePlane<T>::TRenImagePlane( T* pcDataOrg, UInt uiWidthOrg, UInt uiHeightOrg, UInt uiStride, UInt uiPad )
: m_pcData     (pcDataOrg + uiStride * uiPad + uiPad )
, m_uiWidth    (uiWidthOrg  - 2* uiPad )
, m_uiHeight   (uiHeightOrg - 2* uiPad )
, m_uiStride   (uiStride   )
, m_pcDataOrg  (pcDataOrg  )
, m_uiWidthOrg (uiWidthOrg )
, m_uiHeightOrg(uiHeightOrg)
, m_uiPad      (uiPad      )
, m_bClean     (false      )
{

}

template<typename T>
Void TRenImagePlane<T>::setData( T* pDataOrg, UInt uiWidthOrg, UInt uiHeightOrg, UInt uiStride, UInt uiPad, Bool bClean /*= false*/ ) 
{
  deleteData();
  m_uiPad       = uiPad;
  m_pcDataOrg   = pDataOrg;
  m_uiWidthOrg  = uiWidthOrg;
  m_uiHeightOrg = uiHeightOrg;
  m_uiWidth     = uiWidthOrg  - 2* uiPad;
  m_uiHeight    = uiHeightOrg - 2* uiPad;
  m_uiStride    = uiStride;
  m_pcData      = m_pcDataOrg + uiPad * m_uiStride + uiPad;
  m_bClean      = bClean;
}

template<typename T>
Void TRenImagePlane<T>::setData( TRenImagePlane<T>* pcInPlane, Bool bClean )
{
  deleteData();
  m_uiPad       = pcInPlane->getPad();
  m_pcDataOrg   = pcInPlane->getPlaneDataOrg();
  m_uiWidthOrg  = pcInPlane->getWidthOrg();
  m_uiHeightOrg = pcInPlane->getHeightOrg();
  m_uiWidth     = pcInPlane->getWidth();
  m_uiHeight    = pcInPlane->getHeight();
  m_uiStride    = pcInPlane->getStride();
  m_pcData      = pcInPlane->getPlaneData();
  m_bClean      = bClean;
  pcInPlane->setClean( !m_bClean );
}

template<typename T>
Void TRenImagePlane<T>::setClean( Bool bClean )
{
  m_bClean = bClean;
}

template<class T>
T* TRenImagePlane<T>::getPlaneData()
{
  return m_pcData;
}


template<class T>
T* TRenImagePlane<T>::getPlaneDataOrg()
{
  return m_pcDataOrg;
}


template<class T>
Void TRenImagePlane<T>::assign(Pel* pcSourceData, UInt uiSourceStride )
{
  T* pcTargetData = m_pcDataOrg;
  for (UInt uiYPos = 0; uiYPos < m_uiHeightOrg; uiYPos++)
  {
    for (UInt uiXPos = 0; uiXPos < m_uiWidthOrg; uiXPos++)
    {
      pcTargetData[uiXPos] = (T) pcSourceData[uiXPos];
    }
    pcTargetData += m_uiStride;
    pcSourceData += uiSourceStride;
  }
}

template<class T>
Void TRenImagePlane<T>::assign(Pel cData)
{
  T* pcTargetData = m_pcDataOrg;
  for (UInt uiYPos = 0; uiYPos < m_uiHeightOrg; uiYPos++)
  {
    for (UInt uiXPos = 0; uiXPos < m_uiWidthOrg; uiXPos++)
    {
      pcTargetData[uiXPos] = (T) cData;
    }
    pcTargetData  += m_uiStride;
  }
}

template<class T>
Void TRenImagePlane<T>::assign(Double* pdData, UInt uiSourceStride )
{
  T* pcTargetData = m_pcDataOrg;
  for (UInt uiYPos = 0; uiYPos < m_uiHeightOrg; uiYPos++)
  {
    for (UInt uiXPos = 0; uiXPos < m_uiWidthOrg; uiXPos++)
    {
      pcTargetData[uiXPos] = (T) pdData[uiXPos];
    }
    pcTargetData += m_uiStride;
    pdData       +=  uiSourceStride;

  }
}

template<class T>
Void TRenImagePlane<T>::assign(Double dData)
{
  T* pcTargetData = m_pcDataOrg;
  for (UInt uiYPos = 0; uiYPos < m_uiHeightOrg; uiYPos++)
  {
    for (UInt uiXPos = 0; uiXPos < m_uiWidthOrg; uiXPos++)
    {
      pcTargetData[uiXPos] = (T) dData;
    }
    pcTargetData  += m_uiStride;
  }
}


template<class T>
Void TRenImagePlane<T>::assign(Bool* pbData, UInt uiSourceStride )
{
  T* pcTargetData = m_pcDataOrg;
  for (UInt uiYPos = 0; uiYPos < m_uiHeightOrg; uiYPos++)
  {
    for (UInt uiXPos = 0; uiXPos < m_uiWidthOrg; uiXPos++)
    {
      pcTargetData[uiXPos] = (T) pbData[uiXPos];
    }
    pcTargetData += m_uiStride;
    pbData       += uiSourceStride;
  }
}

template<class T>
Void TRenImagePlane<T>::assign(Int iData)
{
  T* pcTargetData = m_pcDataOrg;
  for (UInt uiYPos = 0; uiYPos < m_uiHeightOrg; uiYPos++)
  {
    for (UInt uiXPos = 0; uiXPos < m_uiWidthOrg; uiXPos++)
    {
      pcTargetData[uiXPos] = (T) iData;
    }
    pcTargetData += m_uiStride;
  }
}

template<class T>
Void TRenImagePlane<T>::assign(Int* piData, UInt uiSourceStride )
{
  T* pcTargetData = m_pcDataOrg;
  for (UInt uiYPos = 0; uiYPos < m_uiHeightOrg; uiYPos++)
  {
    for (UInt uiXPos = 0; uiXPos < m_uiWidthOrg; uiXPos++)
    {
      pcTargetData[uiXPos] = (T) piData[uiXPos];
    }
    pcTargetData += m_uiStride;
    piData       += uiSourceStride;
  }
}

template<class T>
Void TRenImagePlane<T>::assign(Bool data)
{
  T* pcTargetData = m_pcDataOrg;
  for (UInt uiYPos = 0; uiYPos < m_uiHeightOrg; uiYPos++)
  {
    for (UInt uiXPos = 0; uiXPos < m_uiWidthOrg; uiXPos++)
    {
      pcTargetData[uiXPos] = (T) data;
    }
    pcTargetData += m_uiStride;
  }
}

// Assignments to Bool

template<>
Void TRenImagePlane<Bool>::assign(Int* piData, UInt uiSourceStride )
{
  Bool* pcTargetData = m_pcDataOrg;
  for (UInt uiYPos = 0; uiYPos < m_uiHeightOrg; uiYPos++)
  {
    for (UInt uiXPos = 0; uiXPos < m_uiWidthOrg; uiXPos++)
    {
      pcTargetData[uiXPos] = (piData[uiXPos] == 0);
    }
    pcTargetData  += m_uiStride;
    piData        += uiSourceStride;

  }
}

template<>
Void TRenImagePlane<Bool>::assign(Int iData)
{
  Bool* pcTargetData = m_pcDataOrg;
  for (UInt uiYPos = 0; uiYPos < m_uiHeightOrg; uiYPos++)
  {
    for (UInt uiXPos = 0; uiXPos < m_uiWidthOrg; uiXPos++)
    {
      pcTargetData[uiXPos] = (iData == 0);
    }
    pcTargetData += m_uiStride;
  }
}

template<>
Void TRenImagePlane<Bool>::assign(Pel* pcData, UInt uiSourceStride )
{
  Bool* pcTargetData = m_pcDataOrg;
  for (UInt uiYPos = 0; uiYPos < m_uiHeightOrg; uiYPos++)
  {
    for (UInt uiXPos = 0; uiXPos < m_uiWidthOrg; uiXPos++)
    {
      pcTargetData[uiXPos] = (pcData[uiXPos] == 0);
    }
    pcTargetData += m_uiStride;
    pcData       +=  uiSourceStride;

  }
}

template<>
Void TRenImagePlane<Bool>::assign(Pel cData)
{
  Bool* pcTargetData = m_pcDataOrg;
  for (UInt uiYPos = 0; uiYPos < m_uiHeightOrg; uiYPos++)
  {
    for (UInt uiXPos = 0; uiXPos < m_uiWidthOrg; uiXPos++)
    {
      pcTargetData[uiXPos] = (cData == 0);
    }
    pcTargetData += m_uiStride;
  }
}

template<>
Void TRenImagePlane<Bool>::assign(Double* pdData, UInt uiSourceStride )
{
  Bool* pcTargetData = m_pcDataOrg;
  for (UInt uiYPos = 0; uiYPos < m_uiHeightOrg; uiYPos++)
  {
    for (UInt uiXPos = 0; uiXPos < m_uiWidthOrg; uiXPos++)
    {
      pcTargetData[uiXPos] = ( pdData[uiXPos] == 0);
    }
    pcTargetData += m_uiStride;
    pdData       += uiSourceStride;

  }
}



template<>
Void TRenImagePlane<Bool>::assign(Double dData)
{
  Bool* pcTargetData = m_pcDataOrg;
  for (UInt uiYPos = 0; uiYPos < m_uiHeightOrg; uiYPos++)
  {
    for (UInt uiXPos = 0; uiXPos < m_uiWidthOrg; uiXPos++)
    {
      pcTargetData [uiXPos] = (dData == 0);
    }
    pcTargetData  += m_uiStride;
  }
}


// Assignments to Pel
template<>
Void TRenImagePlane<Pel>::assign(Double* pdData, UInt uiSourceStride )
{
  Pel* pcTargetData = m_pcDataOrg;
  for (UInt uiYPos = 0; uiYPos < m_uiHeightOrg; uiYPos++)
  {
    for (UInt uiXPos = 0; uiXPos < m_uiWidthOrg; uiXPos++)
    {
      pcTargetData[uiXPos] = (Pel) ( pdData[uiXPos] + pdData[uiXPos] < 0 ? -0.5 : 0.5 ) ;
    }
    pcTargetData += m_uiStride;
    pdData       += uiSourceStride;
  }
}

template<class T>
Void TRenImagePlane<T>::assign(TRenImagePlane<T>* pcPlane)
{
  assign(pcPlane->getPlaneDataOrg(), pcPlane->getStride());
}

template<class T>
Void TRenImagePlane<T>::assign(TRenImagePlane<T>* pcPlane, UInt uiRow, UInt uiStartOffset, UInt uiEndOffset)
{
  T* pcTargetData = m_pcData                + uiRow * m_uiStride;
  T* pcSourceData = pcPlane->getPlaneData() + uiRow * pcPlane->getStride();

  for (UInt uiPosX = uiStartOffset; uiPosX <= uiEndOffset; uiPosX++)
  {
    pcTargetData[uiPosX] = pcSourceData[uiPosX];
  }

}

template<class T>
Void TRenImagePlane<T>::assign(TRenImagePlane<T>* pcSourcePlane, UInt uiSourceRowStart, UInt uiSourceColStart, UInt uiWidth, UInt uiHeight)
{
  T* acSourceData;
  T* acDestData;

  acSourceData = pcSourcePlane->getPlaneData();
  acSourceData += uiSourceRowStart * pcSourcePlane->getStride() + uiSourceColStart;
  acDestData    = m_pcData;

  for (UInt uiPosY = 0; uiPosY < uiHeight ; uiPosY++)
  {
    for (UInt uiPosX = 0; uiPosX < uiWidth ; uiPosX++)
    {
      acDestData[uiPosX] = acSourceData[uiPosX];
    }
    acSourceData += pcSourcePlane->getStride();
    acDestData   += this        ->getStride();
  };
}



template<class T>
Void TRenImagePlane<T>::assign( T data , UInt uiRow, UInt uiStartOffset, UInt uiEndOffset)
{
  T* pcTargetData = m_pcData + uiRow * m_uiStride;
  for (UInt uiPosX = uiStartOffset; uiPosX <= uiEndOffset; uiPosX++)
  {
    pcTargetData[uiPosX] = data;
  }
}


template<class T>
Void TRenImagePlane<T>::devide( Double dDevisor )
{
  T* pcTargetData = m_pcDataOrg;
  for (UInt uiPosY = 0; uiPosY < (m_uiHeightOrg); uiPosY++)
  {
    for (UInt uiPosX = 0; uiPosX < m_uiWidthOrg; uiPosX++)
    {
      pcTargetData[uiPosX] = (T)  ( ( Double )pcTargetData[uiPosX] / dDevisor );
    }
    pcTargetData += m_uiStride;
  }
};

template<class T>
Void TRenImagePlane<T>::multiply( Double dMultiplier ) {
  T* pcTargetData = m_pcDataOrg;
  for (UInt uiPosY = 0; uiPosY < (m_uiHeightOrg); uiPosY++)
  {
    for (UInt uiPosX = 0; uiPosX < m_uiWidthOrg; uiPosX++)
    {
      pcTargetData[uiPosX] = (T)  ( ( Double )pcTargetData[uiPosX] * dMultiplier );
    }
    pcTargetData += m_uiStride;
  }
};


template<>
Void TRenImagePlane<Bool>::devide( Double dDevisor )
{
  assert(0);
};

template<>
Void TRenImagePlane<Bool>::multiply( Double dMultiplier )
{
  assert(0);
};


template<class T>
Void TRenImagePlane<T>::deleteData()
{
  if (m_bClean)
  {
    if (m_pcDataOrg)
    {
      delete[] m_pcDataOrg;
    };
  }
}

template<class T>
TRenImagePlane<T>::~TRenImagePlane()
{
  deleteData();
}


template<typename T>
Void TRenImagePlane<T>::extendMargin()
{
  Int iPad = (Int) m_uiPad;
  T* pcData = m_pcData;

  for ( Int iPosY = 0; iPosY < (Int) m_uiHeight; iPosY++)
  {
    for ( Int iPosX = 0; iPosX < (Int) iPad; iPosX++ )
    {
      pcData[ -iPad + iPosX ]  = pcData[0];
      pcData[m_uiWidth + iPosX ]  = pcData[m_uiWidth -1 ];
    }
    pcData += m_uiStride;
  }


  pcData -= (m_uiStride + iPad);
  for ( Int iPosY = 0; iPosY < iPad; iPosY++ )
  {
    memcpy( pcData + (iPosY+1)*m_uiStride, pcData, sizeof(T)*(m_uiWidth + (iPad<<1)) );
  }

  pcData -= ((m_uiHeight-1) * m_uiStride);
  for ( Int iPosY = 0; iPosY < iPad; iPosY++ )
  {
    memcpy( pcData - (iPosY+1)*m_uiStride, pcData, sizeof(T)*(m_uiWidth + (iPad<<1)) );
  }
}

template class TRenImagePlane<Pel>;
template class TRenImagePlane<Double>;
template class TRenImagePlane<Bool>;
template class TRenImagePlane<Int>;

/////// TRenImagePlanePart ///////

template<typename T>
TRenImagePlanePart<T>::TRenImagePlanePart( TRenImagePlane<T>* pPlane, UInt uHorOff, UInt uVerOff, UInt uWidth, UInt uHeight )
: TRenImagePlane<T>( pPlane->getPlaneData() + uHorOff + uVerOff * pPlane->getStride(), uWidth, uHeight, pPlane->getStride(),0)
{

}

template<typename T>
TRenImagePlanePart<T>::~TRenImagePlanePart()
{
  this->m_pcData = NULL;
}

template class TRenImagePlanePart<Pel>;
template class TRenImagePlanePart<Double>;
template class TRenImagePlanePart<Bool>;
template class TRenImagePlanePart<Int>;
#endif // H_3D
