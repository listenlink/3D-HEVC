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

#include "TComPicYuv.h"
#include "libmd5/MD5.h"

//! \ingroup TLibCommon
//! \{

/**
 * Update md5 using n samples from plane, each sample is adjusted to
 * OUTBIT_BITDEPTH_DIV8.
 */
template<unsigned OUTPUT_BITDEPTH_DIV8>
static void md5_block(MD5& md5, const Pel* plane, unsigned n)
{
  /* create a 64 byte buffer for packing Pel's into */
  unsigned char buf[64/OUTPUT_BITDEPTH_DIV8][OUTPUT_BITDEPTH_DIV8];
  for (unsigned i = 0; i < n; i++)
  {
    Pel pel = plane[i];
    /* perform bitdepth and endian conversion */
    for (unsigned d = 0; d < OUTPUT_BITDEPTH_DIV8; d++)
    {
      buf[i][d] = pel >> (d*8);
    }
  }
  md5.update((unsigned char*)buf, n * OUTPUT_BITDEPTH_DIV8);
}

/**
 * Update md5 with all samples in plane in raster order, each sample
 * is adjusted to OUTBIT_BITDEPTH_DIV8.
 */
template<unsigned OUTPUT_BITDEPTH_DIV8>
static void md5_plane(MD5& md5, const Pel* plane, unsigned width, unsigned height, unsigned stride)
{
  /* N is the number of samples to process per md5 update.
   * All N samples must fit in buf */
  unsigned N = 32;
  unsigned width_modN = width % N;
  unsigned width_less_modN = width - width_modN;

  for (unsigned y = 0; y < height; y++)
  {
    /* convert pel's into unsigned chars in little endian byte order.
     * NB, for 8bit data, data is truncated to 8bits. */
    for (unsigned x = 0; x < width_less_modN; x += N)
      md5_block<OUTPUT_BITDEPTH_DIV8>(md5, &plane[y*stride + x], N);

    /* mop up any of the remaining line */
    md5_block<OUTPUT_BITDEPTH_DIV8>(md5, &plane[y*stride + width_less_modN], width_modN);
  }
}

/**
 * Calculate the MD5sum of pic, storing the result in digest.
 * MD5 calculation is performed on Y' then Cb, then Cr; each in raster order.
 * Pel data is inserted into the MD5 function in little-endian byte order,
 * using sufficient bytes to represent the picture bitdepth.  Eg, 10bit data
 * uses little-endian two byte words; 8bit data uses single byte words.
 */
void calcMD5(TComPicYuv& pic, unsigned char digest[16])
{
  unsigned bitdepth = g_uiBitDepth + g_uiBitIncrement;
  /* choose an md5_plane packing function based on the system bitdepth */
  typedef void (*MD5PlaneFunc)(MD5&, const Pel*, unsigned, unsigned, unsigned);
  MD5PlaneFunc md5_plane_func;
  md5_plane_func = bitdepth <= 8 ? (MD5PlaneFunc)md5_plane<1> : (MD5PlaneFunc)md5_plane<2>;

  MD5 md5;
  unsigned width = pic.getWidth();
  unsigned height = pic.getHeight();
  unsigned stride = pic.getStride();

  md5_plane_func(md5, pic.getLumaAddr(), width, height, stride);

  width >>= 1;
  height >>= 1;
  stride >>= 1;

  md5_plane_func(md5, pic.getCbAddr(), width, height, stride);
  md5_plane_func(md5, pic.getCrAddr(), width, height, stride);

  md5.finalize(digest);
}
//! \}
