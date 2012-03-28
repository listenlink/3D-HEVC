

#ifndef __TCOMMP__
#define __TCOMMP__

class TComMP;

#include "../TLibCommon/TypeDef.h"
#include "../TLibCommon/CommonDef.h"

#if POZNAN_MP_USE_DEPTH_MAP_GENERATION
class TComDepthMapGenerator;
//#include "../TLibCommon/TComDepthMapGenerator.h"
#else
class TAppComCamPara;
//#include "../../App/TAppCommon/TAppComCamPara.h"
#endif

#include "../TLibCommon/TComList.h"
#include "../TLibCommon/TComSlice.h"
#include "../TLibCommon/TComPic.h"
#include "../TLibCommon/TComDataCU.h"
#include "../TLibCommon/TComMotionInfo.h"

#include <map>
//#include <cstdlib>
//using namespace std;

#if POZNAN_MP

// ====================================================================================================================
// Class definition
// ====================================================================================================================


/// MP class
class TComMP
{
//typedef Int PUT_MP_ARRAY_TYPE;
typedef Short PUT_MP_ARRAY_TYPE;
typedef std::map<UInt,PUT_MP_ARRAY_TYPE**> TComMapppi;

static const int OCCLUSION = -1;

private:

  Bool m_bInit;
  Bool m_bEnabled;
  Bool m_bDBMPEnabled;

  UInt m_uiHeight;
  UInt m_uiWidth;

  UInt m_uiPOC;
  UInt m_uiView;
  Bool m_bIsDepth;

#if POZNAN_MP_USE_DEPTH_MAP_GENERATION
  TComDepthMapGenerator* m_pcDepthMapGenerator;
#else
  //TAppComCamPara* m_pcCameraData;
  Int**** m_aiBaseViewShiftLUT;
#endif
  std::vector<TComPic*>* m_pcRefPicsList;
  std::vector<TComPic*>* m_pcDepthRefPicsList;

  Short** m_ppiMvPtCorrX;
  Short** m_ppiMvPtCorrY;
  Short** m_ppiMvPtCorrZ;
  Short** m_ppiMvPtCorrRefViewIdx;

#if !POZNAN_MP_USE_DEPTH_MAP_GENERATION && POZNAN_MP_USE_CURRENT_VIEW_DEPTH_MAP_IF_AVAILABLE
  Short** m_ppiTempMvPtCorrX;
  Short** m_ppiTempMvPtCorrY;
  Short** m_ppiTempMvPtCorrZ;
#endif

  TComDataCU*** m_pppcRefCU; //XY array of pointers to reference CU for each point of picture
  UShort** m_ppuicRefPartAddr; //XY array of part addresses for each reference CU for each point of picture

#if POZNAN_DBMP & !POZNAN_DBMP_COMPRESS_ME_DATA
  TComMapppi m_ppiL0RefPOC;
  TComMapppi m_ppiL0MvX;
  TComMapppi m_ppiL0MvY;
  TComMapppi m_ppiL1RefPOC;
  TComMapppi m_ppiL1MvX;
  TComMapppi m_ppiL1MvY;
#endif

#if POZNAN_DBMP_CALC_PRED_DATA
  Int* m_piTempL0RefIdx;
  Int* m_piTempL0MvX;
  Int* m_piTempL0MvY;
  Int* m_piTempL1RefIdx;
  Int* m_piTempL1MvX;
  Int* m_piTempL1MvY;

  Int*	m_piTempBuff;
  Int*	m_piResBuff;
#endif

#if POZNAN_DBMP_CALC_PRED_DATA
  TComCUMvField* apcDBMPPredMVField[2];
#endif

#if POZNAN_DBMP & !POZNAN_DBMP_COMPRESS_ME_DATA
  Void initArrayMap(UInt uiViewId, Bool bIsDepth);
#endif

public:

#if POZNAN_MP_USE_DEPTH_MAP_GENERATION
  TComMP(UInt uiHeight=0, UInt uiWidth=0);
#else
  //TComMP(UInt uiHeight=0, UInt uiWidth=0, TAppComCamPara* pcCameraData=NULL);
  TComMP(UInt uiHeight=0, UInt uiWidth=0, Int**** aiBaseViewShiftLUT=NULL);
#endif
  ~TComMP();

#if POZNAN_MP_USE_DEPTH_MAP_GENERATION
  Void			init(UInt uiHeight, UInt uiWidth);
#else
  //Void			init(UInt uiHeight, UInt uiWidth, TAppComCamPara* pcCameraData);
  Void			init(UInt uiHeight, UInt uiWidth, Int**** aiBaseViewShiftLUT);
#if POZNAN_MP_USE_CURRENT_VIEW_DEPTH_MAP_IF_AVAILABLE
  Void			clearTemp();
#endif
#endif
  Void			uninit();
  Void			clear();
  Void			disable();

  Bool			isInit();
  Bool			isEnabled();
  Bool			isDBMPEnabled();

#if POZNAN_MP_USE_DEPTH_MAP_GENERATION
  Void			setDepthMapGenerator(TComDepthMapGenerator*  pcDepthMapGenerator){m_pcDepthMapGenerator = pcDepthMapGenerator;}
#endif  
  Void			setRefPicsList(std::vector<TComPic*>* pcListPic){m_pcRefPicsList = pcListPic;}
  Void			setDepthRefPicsList(std::vector<TComPic*>* pcListPic){m_pcDepthRefPicsList = pcListPic;}
  Bool			pairMultiview(TComPic* pcPic);

  Short**		getMPCx() {return m_ppiMvPtCorrX;}
  Short**		getMPCy() {return m_ppiMvPtCorrY;}
  Short**		getMPCz() {return m_ppiMvPtCorrZ;}
  Short**		getMPCRefViewIdx() {return m_ppiMvPtCorrRefViewIdx;}
  TComDataCU***	getMPCRefCU() {return m_pppcRefCU;}
  UShort**		getMPCRefPartAddr() {return m_ppuicRefPartAddr;}

#if POZNAN_MP_FILL
  Void			fillMultiview(UInt uiViewId);
#endif

#if POZNAN_DBMP
#if !POZNAN_DBMP_COMPRESS_ME_DATA
  PUT_MP_ARRAY_TYPE**			getL0RefPOC(UInt uiViewID, Bool bIsDepth) {return m_ppiL0RefPOC[(bIsDepth)? (uiViewID+MAX_NUMBER_VIEWS) : uiViewID];}
  PUT_MP_ARRAY_TYPE**			getL0MvX(UInt uiViewID, Bool bIsDepth) {return m_ppiL0MvX[(bIsDepth)? (uiViewID+MAX_NUMBER_VIEWS) : uiViewID];}
  PUT_MP_ARRAY_TYPE**			getL0MvY(UInt uiViewID, Bool bIsDepth) {return m_ppiL0MvY[(bIsDepth)? (uiViewID+MAX_NUMBER_VIEWS) : uiViewID];}
  PUT_MP_ARRAY_TYPE**			getL1RefPOC(UInt uiViewID, Bool bIsDepth) {return m_ppiL1RefPOC[(bIsDepth)? (uiViewID+MAX_NUMBER_VIEWS) : uiViewID];}
  PUT_MP_ARRAY_TYPE**			getL1MvX(UInt uiViewID, Bool bIsDepth) {return m_ppiL1MvX[(bIsDepth)? (uiViewID+MAX_NUMBER_VIEWS) : uiViewID];}
  PUT_MP_ARRAY_TYPE**			getL1MvY(UInt uiViewID, Bool bIsDepth) {return m_ppiL1MvY[(bIsDepth)? (uiViewID+MAX_NUMBER_VIEWS) : uiViewID];}

  Void			setL0RefPOC(UInt uiViewID, Bool bIsDepth, UInt uiX, UInt uiY, PUT_MP_ARRAY_TYPE iVal) {m_ppiL0RefPOC[(bIsDepth)? (uiViewID+MAX_NUMBER_VIEWS) : uiViewID][uiY][uiX]=iVal;}
  Void			setL0MvX(UInt uiViewID, Bool bIsDepth, UInt uiX, UInt uiY, PUT_MP_ARRAY_TYPE iVal) {m_ppiL0MvX[(bIsDepth)? (uiViewID+MAX_NUMBER_VIEWS) : uiViewID][uiY][uiX]=iVal;}
  Void			setL0MvY(UInt uiViewID, Bool bIsDepth, UInt uiX, UInt uiY, PUT_MP_ARRAY_TYPE iVal) {m_ppiL0MvY[(bIsDepth)? (uiViewID+MAX_NUMBER_VIEWS) : uiViewID][uiY][uiX]=iVal;}
  Void			setL1RefPOC(UInt uiViewID, Bool bIsDepth, UInt uiX, UInt uiY, PUT_MP_ARRAY_TYPE iVal) {m_ppiL1RefPOC[(bIsDepth)? (uiViewID+MAX_NUMBER_VIEWS) : uiViewID][uiY][uiX]=iVal;}
  Void			setL1MvX(UInt uiViewID, Bool bIsDepth, UInt uiX, UInt uiY, PUT_MP_ARRAY_TYPE iVal) {m_ppiL1MvX[(bIsDepth)? (uiViewID+MAX_NUMBER_VIEWS) : uiViewID][uiY][uiX]=iVal;}
  Void			setL1MvY(UInt uiViewID, Bool bIsDepth, UInt uiX, UInt uiY, PUT_MP_ARRAY_TYPE iVal) {m_ppiL1MvY[(bIsDepth)? (uiViewID+MAX_NUMBER_VIEWS) : uiViewID][uiY][uiX]=iVal;}

  Void saveDBMPData(TComDataCU* pcCU);
#endif

  Void getDBMPPredData(TComDataCU* pcCU, Int x, Int y, 
							  Int &ref_frame0, Int &ref_frame0_idx, TComMv &mv0, Int ref_frame0_idx_2nd, TComMv mv0_2nd,
							  Int &ref_frame1, Int &ref_frame1_idx, TComMv &mv1, Int ref_frame1_idx_2nd, TComMv mv1_2nd);
#endif

#if POZNAN_DBMP_CALC_PRED_DATA
  Int*			getTempL0RefIdx() {return m_piTempL0RefIdx;}
  Int*			getTempL0MvX() {return m_piTempL0MvX;}
  Int*			getTempL0MvY() {return m_piTempL0MvY;}
  Int*			getTempL1RefIdx() {return m_piTempL1RefIdx;}
  Int*			getTempL1MvX() {return m_piTempL1MvX;}
  Int*			getTempL1MvY() {return m_piTempL1MvY;}
  
  Void xCalcDBMPPredData(UInt uiCnt, Int &ref_frame0_idx, TComMv &mv0, Int &ref_frame1_idx, TComMv &mv1);
#endif

#if POZNAN_DBMP_CALC_PRED_DATA
  Void calcDBMPPredData(TComDataCU* pcCU, UInt uiAbsPartIdx, Int &ref_frame0_idx,  TComMv &mv0, Int &ref_frame1_idx,  TComMv &mv1);
  TComCUMvField* getDBMPPredMVField(RefPicList eRefListIdx){return apcDBMPPredMVField[eRefListIdx];}
  Void setDBMPPredMVField(RefPicList eRefListIdx, TComDataCU* pcCU);
  Void setDBMPPredMVField(RefPicList eRefListIdx, TComDataCU* pcCU, Int iNumPartSrc, Int iPartAddrDst);
#endif

  TComPic*		GetPicFromList(std::vector<TComPic*>* rcListPic, UInt uiPOC, UInt uiView);
  Int			GetRepresentativeVal(UInt &uiCntMax, Int* puiIn, UInt uiLen, Int* puiTemp, Bool bUseRestrictedVal=false, Int uiRestrictedVal=0);

};




//#endif

#endif // __TCOMMP__


#endif // POZNAN_MP