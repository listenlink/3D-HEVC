

/** \file     TVideoIOYuv.h
    \brief    YUV file I/O class (header)
*/

#ifndef __TVIDEOIOYUV__
#define __TVIDEOIOYUV__

#include <stdio.h>
#include <fstream>
#include <iostream>
#include "../TLibCommon/CommonDef.h"
#include "../TLibCommon/TComPicYuv.h"

using namespace std;

// ====================================================================================================================
// Class definition
// ====================================================================================================================

/// YUV file I/O class
class TVideoIOYuv
{
private:
  fstream   m_cHandle;                                      ///< file handle
  unsigned int m_fileBitdepth; ///< bitdepth of input/output video file
  int m_bitdepthShift;  ///< number of bits to increase or decrease image by before/after write/read
  
public:
  TVideoIOYuv()           {}
  virtual ~TVideoIOYuv()  {}
  
  Void  open  ( char* pchFile, Bool bWriteMode, unsigned int fileBitDepth, unsigned int internalBitDepth ); ///< open or create file
  Void  close ();                                           ///< close file

  void skipFrames(unsigned int numFrames, unsigned int width, unsigned int height);
  
  Void  read  ( TComPicYuv*& rpcPicYuv, Int aiPad[2], Bool bRewind = false );     ///< read  one YUV frame with padding parameter
  Void  write ( TComPicYuv*   pcPicYuv, Int aiPad[2] );     ///< write one YUV frame with padding parameter
  
  Bool  isEof ();                                           ///< check end-of-file
  
};

#endif // __TVIDEOIOYUV__

