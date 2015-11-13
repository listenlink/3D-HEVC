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

/** \file     SyntaxElementParser.h
    \brief    Parsing functionality high level syntax
*/

#ifndef __SYNTAXELEMENTPARSER__
#define __SYNTAXELEMENTPARSER__

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
#include "TLibCommon/TComRom.h"
#if ENC_DEC_TRACE

#define READ_CODE(length, code, name)     xReadCodeTr ( length, code, name )
#define READ_UVLC(        code, name)     xReadUvlcTr (         code, name )
#define READ_SVLC(        code, name)     xReadSvlcTr (         code, name )
#define READ_FLAG(        code, name)     xReadFlagTr (         code, name )
#if NH_MV
#define READ_STRING(bufSize, code, length, name)   xReadStringTr ( bufSize, code, length, name )
#endif
#else

#if RExt__DECODER_DEBUG_BIT_STATISTICS

#define READ_CODE(length, code, name)     xReadCode ( length, code, name )
#define READ_UVLC(        code, name)     xReadUvlc (         code, name )
#define READ_SVLC(        code, name)     xReadSvlc (         code, name )
#define READ_FLAG(        code, name)     xReadFlag (         code, name )
#if NH_MV
#define READ_STRING(bufSize, code, length, name)   xReadString ( bufSize, code, length, name )
#endif
#else

#define READ_CODE(length, code, name)     xReadCode ( length, code )
#define READ_UVLC(        code, name)     xReadUvlc (         code )
#define READ_SVLC(        code, name)     xReadSvlc (         code )
#define READ_FLAG(        code, name)     xReadFlag (         code )
#if NH_MV
#define READ_STRING(bufSize, code, length, name)   xReadString ( bufSize, code, length )
#endif

#endif

#endif

//! \ingroup TLibDecoder
//! \{

// ====================================================================================================================
// Class definition
// ====================================================================================================================

class SyntaxElementParser
{
protected:
  TComInputBitstream*   m_pcBitstream;

  SyntaxElementParser()
  : m_pcBitstream (NULL)
  {};
  virtual ~SyntaxElementParser() {};

#if RExt__DECODER_DEBUG_BIT_STATISTICS
  Void  xReadCode    ( UInt   length, UInt& val, const TChar *pSymbolName );
  Void  xReadUvlc    ( UInt&  val, const TChar *pSymbolName );
  Void  xReadSvlc    ( Int&   val, const TChar *pSymbolName );
  Void  xReadFlag    ( UInt&  val, const TChar *pSymbolName );
#if NH_MV
  Void  xReadString  ( UInt bufSize, UChar *val, UInt& length, const TChar *pSymbolName);
#endif
#else
  Void  xReadCode    ( UInt   length, UInt& val );
  Void  xReadUvlc    ( UInt&  val );
  Void  xReadSvlc    ( Int&   val );
  Void  xReadFlag    ( UInt&  val );
#if NH_MV
  Void  xReadString  ( UInt bufSize, UChar *val, UInt& length);
#endif
#endif
#if ENC_DEC_TRACE
  Void  xReadCodeTr  (UInt  length, UInt& rValue, const TChar *pSymbolName);
  Void  xReadUvlcTr  (              UInt& rValue, const TChar *pSymbolName);
  Void  xReadSvlcTr  (               Int& rValue, const TChar *pSymbolName);
  Void  xReadFlagTr  (              UInt& rValue, const TChar *pSymbolName);
#if NH_MV
  Void  xReadStringTr(UInt bufSize, UChar *pValue, UInt& rLength, const TChar *pSymbolName);
#endif
#endif
public:
  Void  setBitstream ( TComInputBitstream* p )   { m_pcBitstream = p; }
  TComInputBitstream* getBitstream() { return m_pcBitstream; }

protected:
  Void xReadRbspTrailingBits();
};

class AUDReader: public SyntaxElementParser
{
public:
  AUDReader() {};
  virtual ~AUDReader() {};
  Void parseAccessUnitDelimiter(TComInputBitstream* bs, UInt &picType);
};

class FDReader: public SyntaxElementParser
{
public:
  FDReader() {};
  virtual ~FDReader() {};
  Void parseFillerData(TComInputBitstream* bs, UInt &fdSize);
};


//! \}

#endif // !defined(__SYNTAXELEMENTPARSER__)

