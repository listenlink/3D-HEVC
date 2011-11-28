



#ifndef __TRENIMAGE__
#define __TRENIMAGE__

#include "../TLibCommon/CommonDef.h"
#include "../TLibCommon/TComPicYuv.h"
#include "TRenImagePlane.h"


#define PelImage    TRenImage<Pel>
#define DoubleImage TRenImage<Double>
#define IntImage    TRenImage<Int>


template<typename T>
class TRenImage
{
public:

  // Construction
  TRenImage( TRenImage& rcInputImage );
  TRenImage();
//  TRenImage( TRenImagePlane<T>** ppcYPlanes, UInt uiNumberOfFullPlanes, UInt uiNumberOfQuaterPlanes );

  TRenImage( UInt uiWidth, UInt uiHeight, UInt uiNumPlanes, UInt uiNumQPlanes );
  TRenImage( TComPicYuv* pcPicYuvIn, Bool bFirstPlaneOnly = false );

  Void allocatePlanes(UInt uiWidth, UInt uiHeight, UInt uiNumFullPlanes, UInt uiNumQuaterPlanes);
  ~TRenImage();

  TRenImage* create();
  Void       init();

  // Get Planes and data
  TRenImagePlane<T>*  getPlane(UInt uiPlaneNumber) const;
  TRenImagePlane<T>** getPlanes() const;

  UInt getNumberOfPlanes()  const;
  UInt getNumberOfQuaterPlanes() const;
  UInt getNumberOfFullPlanes() const;
  Bool is420() {return m_uiNumberOfFullPlanes == 1 && m_uiNumberOfQuaterPlanes == 2; };
  Bool is444() {return m_uiNumberOfFullPlanes == 3 && m_uiNumberOfQuaterPlanes == 0; };
  Bool is400() {return m_uiNumberOfFullPlanes == 1 && m_uiNumberOfQuaterPlanes == 0; };

  // Assign
  Void assign(Int iVal);
  template<typename S> Void assign(TRenImage<S>* pcSrcImage);
  Void setData( TRenImage* pcInputImage, Bool bClean );

  Void extendMargin();
  // Operators
  Void devide( Double dDevisor );


private:

  UInt m_uiNumberOfFullPlanes;
  UInt m_uiNumberOfQuaterPlanes;
  UInt m_uiNumberOfPlanes;
  TRenImagePlane<T> ** m_apcPlanes;   // First Full Planes, then Quater Planes

  Void xDeletePlanes();
};





#endif // __TRENIMAGE__


