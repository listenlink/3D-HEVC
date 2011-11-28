

#include <list>
#include <stdio.h>
#include <fcntl.h>
#include <assert.h>


#ifndef __TAppRendererTOP__
#define __TAppRendererTOP__


#include "../../Lib/TLibRenderer/TRenTop.h"
#include "../../Lib/TLibVideoIO/TVideoIOYuv.h"
#include "../../Lib/TLibVideoIO/TVideoIOBits.h"
#include "../../Lib/TLibCommon/TComBitStream.h"
#include "TAppRendererCfg.h"
#include "TAppRendererTop.h"
#include "../../Lib/TLibRenderer/TRenModel.h"


// ====================================================================================================================
// Class definition
// ====================================================================================================================

/// encoder application class
class TAppRendererTop : public TAppRendererCfg
{
private:
  // class interface
  std::vector<TVideoIOYuv*>		 m_apcTVideoIOYuvVideoInput;
  std::vector<TVideoIOYuv*>    m_apcTVideoIOYuvDepthInput;
  std::vector<TVideoIOYuv*>		 m_apcTVideoIOYuvSynthOutput;

  // RendererInterface
  TRenTop*                     m_pcRenTop;

protected:
  // initialization
  Void  xCreateLib        ();                               ///< create renderer class and video io
  Void  xInitLib          ();                               ///< initialize renderer class
  Void  xDestroyLib       ();                               ///< destroy renderer class and video io
  Void  xRenderModelFromString();                           ///< render using model using setup string
  Void  xRenderModelFromNums();                             ///< render using model using synth view numbers


public:
  TAppRendererTop();
  virtual ~TAppRendererTop();

  Void  render      ();                               ///< main encoding function
  Void  renderModel ();
  Void  go          ();
  Void  renderUsedPelsMap();

};// END CLASS DEFINITION TAppRendererTop

#endif // __TAppRendererTOP__

