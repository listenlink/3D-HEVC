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


#ifndef __TRENIMAGE__
#define __TRENIMAGE__

#include "../TLibCommon/CommonDef.h"
#include "../TLibCommon/TComPicYuv.h"
#include "TRenImagePlane.h"
#if H_3D


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

  Void getDataAndStrides( T**    pptData, Int*  piStrides );
  Void getWidthAndHeight( Int*  piWidths, Int*  piHeights );

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

#endif // H_3D
#endif // __TRENIMAGE__


