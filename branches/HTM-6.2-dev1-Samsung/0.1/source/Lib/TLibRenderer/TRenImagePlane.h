



#ifndef __TRENIMAGEPLANE__
#define __TRENIMAGEPLANE__

#include "../TLibCommon/CommonDef.h"
#include "../TLibCommon/TComPicYuv.h"

#define PelImagePlane     TRenImagePlane<Pel>
#define DoubleImagePlane  TRenImagePlane<Double>
#define IntImagePlane     TRenImagePlane<Int>

template<typename T>
class TRenImagePlane
{
public:
  // Construction
  TRenImagePlane();
  TRenImagePlane( UInt uiWidth, UInt uiHeight, UInt uiPad);
  TRenImagePlane( TRenImagePlane* pcInputPlane );
  TRenImagePlane( T* pcDataOrg, UInt uiWidthOrg, UInt uiHeightOrg, UInt uiStride, UInt uiPad );

  ~TRenImagePlane();

  // Get Data
  T*   getPlaneData();
  UInt getWidth    () { return m_uiWidth; };
  UInt getHeight   () { return m_uiHeight; };

  T*   getPlaneDataOrg();
  UInt getWidthOrg () { return m_uiWidthOrg;  };
  UInt getHeightOrg() { return m_uiHeightOrg; };
  UInt getPad      () { return m_uiPad;       };
  UInt getStride   () { return m_uiStride; };

  Void setData ( T* pDataOrg, UInt uiWidthOrg, UInt uiHeightOrg, UInt uiStride, UInt uiPad, Bool bClean /*= false*/ );
  Void setData ( TRenImagePlane<T>* pcInPlane, Bool bClean );
  Void setClean( Bool bClean );
  Void extendMargin();

  // Assignment
  Void assign( Pel*    data, UInt uiSourceStride );
  Void assign( Pel     data );

  Void assign( Double* data, UInt uiSourceStride );
  Void assign( Double  data );

  Void assign( Bool*  data, UInt uiSourceStride );
  Void assign( Bool   data );

  Void assign( Int*   data, UInt uiSourceStride );
  Void assign( Int    data );

  Void assign( TRenImagePlane<T>* pcPlane);

  Void assign( T data , UInt uRow, UInt uStartOffset, UInt uEndOffset);
  Void assign( TRenImagePlane<T>* pcPlane, UInt uRow, UInt uStartOffset, UInt uEndOffset);
  Void assign( TRenImagePlane<T>* pcSourcePlane, UInt uSourceRowStart, UInt uSourceColStart, UInt uWidth, UInt uHeight);

  // Operators
  Void devide(   Double dDevisor );
  Void multiply( Double dMultiplier );

protected:
  T     *m_pcData;
  UInt   m_uiWidth;
  UInt   m_uiHeight;
  UInt   m_uiStride;

  T     *m_pcDataOrg;
  UInt   m_uiWidthOrg;
  UInt   m_uiHeightOrg;
  UInt   m_uiPad;

  Double m_dRatio;
  Bool   m_bClean;

private:
  Void deleteData();
};

template<typename T>
class TRenImagePlanePart : public TRenImagePlane< T >
{
public:
  TRenImagePlanePart( TRenImagePlane<T>* pcPlane, UInt uHorOff, UInt uVerOff, UInt uWidth, UInt uHeight);;
  ~TRenImagePlanePart();;
};

#endif // __TRENIMAGEPLANE__
