

#include "TRenImage.h"
#include "TRenImagePlane.h"
#include "TRenFilter.h"

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
    m_apcPlanes[0] = new TRenImagePlane<Pel>( pcPicYuv->getBufY(), pcPicYuv->getWidth() + (REN_LUMA_MARGIN << 1),   pcPicYuv->getHeight()+ (REN_LUMA_MARGIN << 1), pcPicYuv->getStride (), REN_LUMA_MARGIN );
  }
  else //420
  {
    m_uiNumberOfPlanes       = 3;
    m_uiNumberOfFullPlanes   = 1;
    m_uiNumberOfQuaterPlanes = 2;

    m_apcPlanes    = new TRenImagePlane<Pel>*[ m_uiNumberOfPlanes ];
    m_apcPlanes[0] = new TRenImagePlane<Pel>( pcPicYuv->getBufY(),   pcPicYuv->getWidth()     + (REN_LUMA_MARGIN << 1),  pcPicYuv->getHeight()      + (REN_LUMA_MARGIN << 1), pcPicYuv->getStride (), REN_LUMA_MARGIN );
    m_apcPlanes[1] = new TRenImagePlane<Pel>( pcPicYuv->getBufU(),   (pcPicYuv->getWidth()>>1)+  REN_LUMA_MARGIN      ,  (pcPicYuv->getHeight()>>1) +  REN_LUMA_MARGIN      , pcPicYuv->getCStride(), REN_LUMA_MARGIN >> 1 );
    m_apcPlanes[2] = new TRenImagePlane<Pel>( pcPicYuv->getBufV(),   (pcPicYuv->getWidth()>>1)+  REN_LUMA_MARGIN      ,  (pcPicYuv->getHeight()>>1) +  REN_LUMA_MARGIN      , pcPicYuv->getCStride(), REN_LUMA_MARGIN >> 1 );
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
    m_apcPlanes[uiCurPlane]->assign( (Pel) ((g_uiIBDI_MAX+1) >> 1) );
  }
}


template<class T>
TRenImage<T>::~TRenImage() {
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


