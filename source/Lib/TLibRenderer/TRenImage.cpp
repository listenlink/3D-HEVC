/* The copyright in this software is being made available under the BSD
 * License, included below. This software may be subject to other third party
 * and contributor rights, including patent rights, and no such rights are
 * granted under this license.
 *
 * Copyright (c) 2010-2015, ITU/ISO/IEC
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


#include "TRenImage.h"
#include "TRenImagePlane.h"
#include "TRenFilter.h"
#include "assert.h"
#if NH_3D_VSO


template<typename T>
TRenImage<T>::TRenImage( TRenImage& rcIn )
{  
  allocatePlanes( rcIn.getPlane(0)->getWidth(), rcIn.getPlane(0)->getHeight(), rcIn.getNumberOfFullPlanes(), rcIn.getNumberOfQuaterPlanes() ) ; assign(&rcIn);
}

template<typename T>
TRenImage<T>::TRenImage( UInt uiWidth, UInt uiHeight, UInt uiNumberOfFullPlanes, UInt uiNumberOfQuaterPlanes )
{  
  allocatePlanes( uiWidth, uiHeight, uiNumberOfFullPlanes, uiNumberOfQuaterPlanes );
}

template<typename T>
TRenImage<T>::TRenImage() : m_uiNumberOfFullPlanes(0), m_uiNumberOfQuaterPlanes(0), m_uiNumberOfPlanes(0), m_apcPlanes(0)
{
  
}


template<>
TRenImage<Pel>::TRenImage( TComPicYuv* pcPicYuv, Bool bFirstPlaneOnly )
{ 
  if (bFirstPlaneOnly) //400
  {
    m_uiNumberOfPlanes       = 1;
    m_uiNumberOfFullPlanes   = 1;
    m_uiNumberOfQuaterPlanes = 0;
    m_apcPlanes    = new TRenImagePlane<Pel>*[ m_uiNumberOfPlanes ];
    m_apcPlanes[0] = new TRenImagePlane<Pel>( pcPicYuv->getBuf( COMPONENT_Y ), pcPicYuv->getWidth( COMPONENT_Y  ) + (REN_LUMA_MARGIN << 1),   pcPicYuv->getHeight( COMPONENT_Y  ) + (REN_LUMA_MARGIN << 1), pcPicYuv->getStride( COMPONENT_Y ), REN_LUMA_MARGIN );
  }
  else //420
  {
    m_uiNumberOfPlanes       = 3;
    m_uiNumberOfFullPlanes   = 1;
    m_uiNumberOfQuaterPlanes = 2;

    m_apcPlanes    = new TRenImagePlane<Pel>*[ m_uiNumberOfPlanes ];
    m_apcPlanes[0] = new TRenImagePlane<Pel>( pcPicYuv->getBuf( COMPONENT_Y  ), pcPicYuv->getWidth( COMPONENT_Y  ) + (REN_LUMA_MARGIN << 1),  pcPicYuv->getHeight( COMPONENT_Y  ) + (REN_LUMA_MARGIN << 1), pcPicYuv->getStride( COMPONENT_Y ), REN_LUMA_MARGIN );
    m_apcPlanes[1] = new TRenImagePlane<Pel>( pcPicYuv->getBuf( COMPONENT_Cb ), pcPicYuv->getWidth( COMPONENT_Cb ) +  REN_LUMA_MARGIN      ,  pcPicYuv->getHeight( COMPONENT_Cb ) +  REN_LUMA_MARGIN      , pcPicYuv->getStride( COMPONENT_Cb), REN_LUMA_MARGIN >> 1 );
    m_apcPlanes[2] = new TRenImagePlane<Pel>( pcPicYuv->getBuf( COMPONENT_Cr ), pcPicYuv->getWidth( COMPONENT_Cr ) +  REN_LUMA_MARGIN      ,  pcPicYuv->getHeight( COMPONENT_Cr ) +  REN_LUMA_MARGIN      , pcPicYuv->getStride( COMPONENT_Cr), REN_LUMA_MARGIN >> 1 );
  }
}

template<typename T>
TRenImage<T>* TRenImage<T>::create()
{
  return new TRenImage( m_apcPlanes[0]->getWidth(), m_apcPlanes[0]->getHeight(), m_uiNumberOfFullPlanes, m_uiNumberOfQuaterPlanes );
}


template<typename T>
TRenImage<T>::TRenImage( TComPicYuv* pcPicYuv, Bool bFirstPlaneOnly )
{
  assert(0);
}

template<class T>
TRenImagePlane<T>* TRenImage<T>::getPlane(UInt uiPlaneNumber) const
{
  return m_apcPlanes[uiPlaneNumber];
}

template<class T>
TRenImagePlane<T>** TRenImage<T>::getPlanes() const
{
  return m_apcPlanes;
}

template<typename T>
Void TRenImage<T>::getDataAndStrides( T** pptData, Int* piStrides ) const
{
  for (UInt uiCurPlane = 0; uiCurPlane < m_uiNumberOfPlanes; uiCurPlane++ )
  {
    piStrides[uiCurPlane] = m_apcPlanes[uiCurPlane]->getStride   ();
    pptData  [uiCurPlane] = m_apcPlanes[uiCurPlane]->getPlaneData();
  }
}


template<typename T>
Void TRenImage<T>::getWidthAndHeight( Int* ppiWidths, Int* ppiHeights ) const
{
  for (UInt uiCurPlane = 0; uiCurPlane < m_uiNumberOfPlanes; uiCurPlane++ )
  {
    ppiWidths [uiCurPlane] = m_apcPlanes[uiCurPlane]->getWidth ();
    ppiHeights[uiCurPlane] = m_apcPlanes[uiCurPlane]->getHeight();
  }
}

template<typename T>
Void TRenImage<T>::allocatePlanes( UInt uiWidth, UInt uiHeight, UInt uiNumberOfFullPlanes, UInt uiNumberOfQuaterPlanes )
{
  assert( uiNumberOfFullPlanes + uiNumberOfQuaterPlanes);

  UInt uiHalfWidth  = uiWidth  / 2;
  UInt uiHalfHeight = uiHeight / 2;

  uiHalfWidth  = (uiHalfWidth  == 0) ? 1 : uiHalfWidth ;
  uiHalfHeight = (uiHalfHeight == 0) ? 1 : uiHalfHeight;

  m_uiNumberOfPlanes       = uiNumberOfFullPlanes + uiNumberOfQuaterPlanes; ;
  m_uiNumberOfFullPlanes   = uiNumberOfFullPlanes;
  m_uiNumberOfQuaterPlanes = uiNumberOfQuaterPlanes;

  this->m_apcPlanes    = new TRenImagePlane<T>*[m_uiNumberOfPlanes];

  for (UInt uiCurPlane = 0; uiCurPlane < uiNumberOfFullPlanes; uiCurPlane++)
  {
    this->m_apcPlanes[uiCurPlane] = new TRenImagePlane<T>(uiWidth, uiHeight, REN_LUMA_MARGIN);
  };

  for (UInt uiCurPlane = 0; uiCurPlane < uiNumberOfQuaterPlanes; uiCurPlane++)
  {
    this->m_apcPlanes[uiCurPlane+uiNumberOfFullPlanes] = new TRenImagePlane<T>(uiHalfWidth, uiHalfHeight, REN_LUMA_MARGIN >> 1);
  };
}


template<class T>
Void TRenImage<T>::assign(Int iVal)
{
  for (UInt uiCurPlane = 0; uiCurPlane < m_uiNumberOfPlanes; uiCurPlane++)
  {
    m_apcPlanes[uiCurPlane]->assign( iVal);
  }
}


template<class T>
Void TRenImage<T>::devide( Double dDevisor )
{
  for (UInt uiCurPlane = 0; uiCurPlane < m_uiNumberOfPlanes; uiCurPlane++)
  {
    m_apcPlanes[uiCurPlane]->devide(dDevisor);
  }
}


template<class T> template<class S>
Void TRenImage<T>::assign( TRenImage<S>* pcSrcImage )
{
  if (pcSrcImage->getNumberOfPlanes() != m_uiNumberOfPlanes )
  {
    assert(0);
  }

  for (UInt uiCurPlane = 0; uiCurPlane < m_uiNumberOfPlanes; uiCurPlane++)
  {
    m_apcPlanes[uiCurPlane]->assign(pcSrcImage->getPlane(uiCurPlane)->getPlaneDataOrg(),pcSrcImage->getPlane(uiCurPlane)->getStride());
  }
}


template<typename T>
Void TRenImage<T>::setData( TRenImage* pcInputImage, Bool bClean )
{
  for (UInt uiPlane = 0; uiPlane < m_uiNumberOfPlanes; uiPlane++)
  {
    m_apcPlanes[uiPlane]->setData( pcInputImage->getPlane( uiPlane ), bClean );
  }
}

template<typename T>
Void TRenImage<T>::extendMargin()
{
  for (UInt uiPlane = 0; uiPlane < m_uiNumberOfPlanes; uiPlane++)
  {
    m_apcPlanes[uiPlane]->extendMargin();
  }
}

template<class T>
Void TRenImage<T>::xDeletePlanes()
{
  for (UInt uiCurPlane = 0; uiCurPlane < m_uiNumberOfPlanes; uiCurPlane++)
  {
    if ( m_apcPlanes[uiCurPlane])
    {
      delete m_apcPlanes[uiCurPlane];
    }
    m_apcPlanes[uiCurPlane] = 0;
  }
}


template<class T>
Void TRenImage<T>::init()
{
  // YUV-init
  m_apcPlanes[0]->assign((Pel) 0 );

  for (UInt uiCurPlane = 1; uiCurPlane < m_uiNumberOfPlanes; uiCurPlane++)
  {
    m_apcPlanes[uiCurPlane]->assign( (Pel) ( 1 << ( REN_BIT_DEPTH - 1 ) ) );
  }
}


template<class T>
TRenImage<T>::~TRenImage()
{
  xDeletePlanes();
  delete[] m_apcPlanes;
}



template<class T>
UInt TRenImage<T>::getNumberOfPlanes() const
{
  return m_uiNumberOfPlanes;
}

template<class T>
UInt TRenImage<T>::getNumberOfQuaterPlanes() const
{
  return m_uiNumberOfQuaterPlanes;
}

template<class T>
UInt TRenImage<T>::getNumberOfFullPlanes() const
{
  return m_uiNumberOfFullPlanes;
}

template class TRenImage<Pel>;
template class TRenImage<Int>;
template class TRenImage<Double>;
template class TRenImage<Bool>;


template Void TRenImage<Pel>::assign<Pel>    (TRenImage<Pel>*   );

#endif // NH_3D
