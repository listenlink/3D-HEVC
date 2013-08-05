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

/** \file     TExtrTop.cpp
    \brief    extractor class
*/

#include "TExtrTop.h"
#if H_MV
TExtrTop::TExtrTop()
{
}

TExtrTop::~TExtrTop()
{
}

Void TExtrTop::init()
{
  m_cEntropyDecoder.init(&m_cPrediction);

  m_acSPSBuffer.clear();
}

Bool TExtrTop::extract( InputNALUnit& nalu, std::set<UInt>& rsuiExtractLayerIds )
{
  //extraction now has to be done using layer_id
  UInt uiLayerId      = nalu.m_layerId;

  
  // Initialize entropy decoder
  m_cEntropyDecoder.setEntropyDecoder( &m_cCavlcDecoder );
  m_cEntropyDecoder.setBitstream     ( nalu.m_Bitstream );
  
  if ( nalu.m_nalUnitType == NAL_UNIT_VPS )
  {
    // a hack for now assuming there's only one VPS in the bitstream
    m_cEntropyDecoder.decodeVPS( &m_cVPS );      
  }

  if ( nalu.m_nalUnitType == NAL_UNIT_SPS )
  {
     TComSPS     cSPS;
#if H_3D
     Int layerIdInVPS = m_cVPS.getLayerIdInVps( uiLayerId ); 
     m_cEntropyDecoder   .decodeSPS( &cSPS, m_cVPS.getViewIndex( layerIdInVPS ), ( m_cVPS.getDepthId( layerIdInVPS ) == 1 ) );
#else
     m_cEntropyDecoder   .decodeSPS( &cSPS );
#endif
     m_acSPSBuffer       .push_back( cSPS );
  }

  return ( rsuiExtractLayerIds.find( uiLayerId ) != rsuiExtractLayerIds.end() );
}


Void TExtrTop::dumpSpsInfo( std::ostream& rcSpsInfoHandle )
{ 
  rcSpsInfoHandle << "NumSPS = " << m_acSPSBuffer.size() << std::endl;
  
  std::list<Int>::iterator iterSPSLayerId = m_aiSPSLayerIdBuffer.begin(); 

  for( std::list<TComSPS>::iterator iterSPS = m_acSPSBuffer.begin(); iterSPS != m_acSPSBuffer.end(); iterSPS++, iterSPSLayerId++ )
  {     
     rcSpsInfoHandle << "layer_id = "              << *iterSPSLayerId << std::endl;
     rcSpsInfoHandle << "seq_parameter_set_id = "  << iterSPS->getSPSId() << std::endl;
  }
}

Void TExtrTop::dumpVpsInfo( std::ostream& rcVpsInfoHandle )
{ 
  rcVpsInfoHandle << "MaxLayers = "     << m_cVPS.getMaxLayers()     << std::endl; 
  rcVpsInfoHandle << "MaxNuhLayerId = " << m_cVPS.getVpsMaxLayerId() << std::endl; 
  
  for ( Int layerIdxInVps = 0; layerIdxInVps < m_cVPS.getMaxLayers(); layerIdxInVps++ )
  {  
    rcVpsInfoHandle << "LayerIdxInVps =  " << layerIdxInVps                           << std::endl; 
    rcVpsInfoHandle << "LayerIdInNuh = "   << m_cVPS.getLayerIdInNuh( layerIdxInVps ) << std::endl; 
    rcVpsInfoHandle << "ViewId = "         << m_cVPS.getViewId      ( layerIdxInVps ) << std::endl; 
#if H_3D
    rcVpsInfoHandle << "DepthFlag = "      << m_cVPS.getViewIndex   ( layerIdxInVps ) << std::endl;     
    rcVpsInfoHandle << "DepthFlag = "      << m_cVPS.getDepthId     ( layerIdxInVps ) << std::endl;     
#endif
  }
}
#endif
