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


#ifndef __TRENIMAGEPLANE__
#define __TRENIMAGEPLANE__

#include "../TLibCommon/CommonDef.h"
#include "../TLibCommon/TComPicYuv.h"

#if H_3D
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

#endif // H_3D
#endif // __TRENIMAGEPLANE__
