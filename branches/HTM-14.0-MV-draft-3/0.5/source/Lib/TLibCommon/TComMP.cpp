

#include "TComMP.h"
//#include "../TLibSynth/TComMatrix.h"
//#include "../TLibSynth/TSynthD2Z.h"
//#include "../TLibSynth/TSynthZ2D.h"
//#include "../TLibSynth/TSynthD2D.h"

#if POZNAN_MP_USE_DEPTH_MAP_GENERATION
#include "../TLibCommon/TComDepthMapGenerator.h"
#else
#include "../../App/TAppCommon/TAppComCamPara.h"
#endif

#if POZNAN_MP


#if POZNAN_MP_USE_DEPTH_MAP_GENERATION
TComMP::TComMP(UInt uiHeight, UInt uiWidth)
#else
//TComMP::TComMP(UInt uiHeight, UInt uiWidth, TAppComCamPara* pcCameraData)
TComMP::TComMP(UInt uiHeight, UInt uiWidth, Int**** aiBaseViewShiftLUT)
#endif
{
	m_ppiMvPtCorrX = NULL;
	m_ppiMvPtCorrY = NULL;
	m_ppiMvPtCorrZ = NULL;
	m_ppiMvPtCorrRefViewIdx = NULL;
	m_pppcRefCU = NULL;
	m_ppuicRefPartAddr = NULL;

	m_bInit = false;
	m_bEnabled = false;
	m_bDBMPEnabled = false;

	m_uiHeight = 0;
	m_uiWidth = 0;

	m_pcRefPicsList = NULL;
	m_pcDepthRefPicsList = NULL;
#if POZNAN_MP_USE_DEPTH_MAP_GENERATION
	m_pcDepthMapGenerator = NULL;
#else
	//m_pcCameraData = NULL;
	m_aiBaseViewShiftLUT = NULL;
#if POZNAN_MP_USE_CURRENT_VIEW_DEPTH_MAP_IF_AVAILABLE
  m_ppiTempMvPtCorrX = NULL;
  m_ppiTempMvPtCorrY = NULL;
  m_ppiTempMvPtCorrZ = NULL;
#endif
#endif

#if POZNAN_DBMP_CALC_PRED_DATA
  m_piTempL0RefIdx = NULL;
  m_piTempL0MvX = NULL;
  m_piTempL0MvY = NULL;
  m_piTempL1RefIdx = NULL;
  m_piTempL1MvX = NULL;
  m_piTempL1MvY = NULL;

  m_piTempBuff = NULL;
  m_piResBuff = NULL;
#endif

#if POZNAN_MP_USE_DEPTH_MAP_GENERATION
	if(uiHeight>0 && uiWidth>0) init(uiHeight, uiWidth);
#else
	//if(uiHeight>0 && uiWidth>0) init(uiHeight, uiWidth, pcCameraData);
	if(uiHeight>0 && uiWidth>0) init(uiHeight, uiWidth, aiBaseViewShiftLUT);
#endif
}

TComMP::~TComMP()
{
	uninit();
}

#if POZNAN_MP_USE_DEPTH_MAP_GENERATION
Void TComMP::init(UInt uiHeight, UInt uiWidth)
#else
//Void TComMP::init(UInt uiHeight, UInt uiWidth, TAppComCamPara* pcCameraData)
Void TComMP::init(UInt uiHeight, UInt uiWidth, Int**** aiBaseViewShiftLUT)
#endif
{
	if(m_bInit) uninit();

	m_bInit = true;
	m_bEnabled = false;
	m_bDBMPEnabled = false;

	m_uiHeight = uiHeight;
	m_uiWidth = uiWidth;
	
	m_pcRefPicsList = NULL;
	m_pcDepthRefPicsList = NULL;
#if POZNAN_MP_USE_DEPTH_MAP_GENERATION
	m_pcDepthMapGenerator = NULL;
#else
	//m_pcCameraData = pcCameraData;
	m_aiBaseViewShiftLUT = aiBaseViewShiftLUT;
#endif

	Short* piPtr;
	UShort* puiPtr;
	TComDataCU** ppcPtr;

	m_ppiMvPtCorrX = (Short**)malloc(sizeof(Short*)*m_uiHeight);
	piPtr = (Short*)malloc(sizeof(Short)*m_uiHeight*m_uiWidth);
	for(UInt i=0,addr=0;i<m_uiHeight;i++,addr+=m_uiWidth) m_ppiMvPtCorrX[i] = &(piPtr[addr]);

	m_ppiMvPtCorrY = (Short**)malloc(sizeof(Short*)*m_uiHeight);
	piPtr = (Short*)malloc(sizeof(Short)*m_uiHeight*m_uiWidth);
	for(UInt i=0,addr=0;i<m_uiHeight;i++,addr+=m_uiWidth) m_ppiMvPtCorrY[i] = &(piPtr[addr]);

	m_ppiMvPtCorrZ = (Short**)malloc(sizeof(Short*)*m_uiHeight);
	piPtr = (Short*)malloc(sizeof(Short)*m_uiHeight*m_uiWidth);
	for(UInt i=0,addr=0;i<m_uiHeight;i++,addr+=m_uiWidth) m_ppiMvPtCorrZ[i] = &(piPtr[addr]);

	m_ppiMvPtCorrRefViewIdx = (Short**)malloc(sizeof(Short*)*m_uiHeight);
	piPtr = (Short*)malloc(sizeof(Short)*m_uiHeight*m_uiWidth);
	for(UInt i=0,addr=0;i<m_uiHeight;i++,addr+=m_uiWidth) m_ppiMvPtCorrRefViewIdx[i] = &(piPtr[addr]);

	m_pppcRefCU = (TComDataCU***)malloc(sizeof(TComDataCU**)*m_uiHeight);
	ppcPtr = (TComDataCU**)malloc(sizeof(TComDataCU*)*m_uiHeight*m_uiWidth);
	for(UInt i=0,addr=0;i<m_uiHeight;i++,addr+=m_uiWidth) m_pppcRefCU[i] = &(ppcPtr[addr]);

	m_ppuicRefPartAddr = (UShort**)malloc(sizeof(UShort*)*m_uiHeight);
	puiPtr = (UShort*)malloc(sizeof(UShort)*m_uiHeight*m_uiWidth);
	for(UInt i=0,addr=0;i<m_uiHeight;i++,addr+=m_uiWidth) m_ppuicRefPartAddr[i] = &(puiPtr[addr]);

#if !POZNAN_MP_USE_DEPTH_MAP_GENERATION && POZNAN_MP_USE_CURRENT_VIEW_DEPTH_MAP_IF_AVAILABLE
	m_ppiTempMvPtCorrX = (Short**)malloc(sizeof(Short*)*m_uiHeight);
	piPtr = (Short*)malloc(sizeof(Short)*m_uiHeight*m_uiWidth);
	for(UInt i=0,addr=0;i<m_uiHeight;i++,addr+=m_uiWidth) m_ppiTempMvPtCorrX[i] = &(piPtr[addr]);

	m_ppiTempMvPtCorrY = (Short**)malloc(sizeof(Short*)*m_uiHeight);
	piPtr = (Short*)malloc(sizeof(Short)*m_uiHeight*m_uiWidth);
	for(UInt i=0,addr=0;i<m_uiHeight;i++,addr+=m_uiWidth) m_ppiTempMvPtCorrY[i] = &(piPtr[addr]);

	m_ppiTempMvPtCorrZ = (Short**)malloc(sizeof(Short*)*m_uiHeight);
	piPtr = (Short*)malloc(sizeof(Short)*m_uiHeight*m_uiWidth);
	for(UInt i=0,addr=0;i<m_uiHeight;i++,addr+=m_uiWidth) m_ppiTempMvPtCorrZ[i] = &(piPtr[addr]);
#endif

#if POZNAN_DBMP & !POZNAN_DBMP_COMPRESS_ME_DATA
	m_ppiL0RefPOC.clear();
	m_ppiL0MvX.clear();
	m_ppiL0MvY.clear();
	m_ppiL1RefPOC.clear();
	m_ppiL1MvX.clear();
	m_ppiL1MvY.clear();
#endif

#if POZNAN_DBMP_CALC_PRED_DATA
	UInt uiSize = g_uiMaxCUHeight*g_uiMaxCUWidth;
	m_piTempL0RefIdx = new Int[uiSize]; 
	m_piTempL0MvX = new Int[uiSize];
	m_piTempL0MvY = new Int[uiSize];
	m_piTempL1RefIdx = new Int[uiSize];
	m_piTempL1MvX = new Int[uiSize];
	m_piTempL1MvY = new Int[uiSize];
	m_piTempBuff = new Int[uiSize];
	m_piResBuff = new Int[uiSize];
#endif	

#if POZNAN_DBMP_CALC_PRED_DATA
  apcDBMPPredMVField[0] = new TComCUMvField(); apcDBMPPredMVField[0]->create(uiSize);
  apcDBMPPredMVField[1] = new TComCUMvField(); apcDBMPPredMVField[1]->create(uiSize);
#endif

}

#if POZNAN_DBMP & !POZNAN_DBMP_COMPRESS_ME_DATA
Void TComMP::initArrayMap(UInt uiViewId, Bool bIsDepth)
{
	if(bIsDepth) uiViewId += MAX_NUMBER_VIEWS;

	if(m_ppiL0RefPOC.count(uiViewId)) return;

	PUT_MP_ARRAY_TYPE** ppiTemp;
	PUT_MP_ARRAY_TYPE* piTemp;

/*	
	ppiTemp = new PUT_MP_ARRAY_TYPE*[m_uiHeight]; for(Int i=0;i<m_uiHeight;i++) ppiTemp[i] = new PUT_MP_ARRAY_TYPE[m_uiWidth];
	m_ppiL0RefPOC.insert(std::pair<UInt, PUT_MP_ARRAY_TYPE**>(uiViewId, ppiTemp));

	ppiTemp = new PUT_MP_ARRAY_TYPE*[m_uiHeight]; for(Int i=0;i<m_uiHeight;i++) ppiTemp[i] = new PUT_MP_ARRAY_TYPE[m_uiWidth];
	m_ppiL0MvX.insert(std::pair<UInt, PUT_MP_ARRAY_TYPE**>(uiViewId, ppiTemp));

	ppiTemp = new PUT_MP_ARRAY_TYPE*[m_uiHeight]; for(Int i=0;i<m_uiHeight;i++) ppiTemp[i] = new PUT_MP_ARRAY_TYPE[m_uiWidth];
	m_ppiL0MvY.insert(std::pair<UInt, PUT_MP_ARRAY_TYPE**>(uiViewId, ppiTemp));

	ppiTemp = new PUT_MP_ARRAY_TYPE*[m_uiHeight]; for(Int i=0;i<m_uiHeight;i++) ppiTemp[i] = new PUT_MP_ARRAY_TYPE[m_uiWidth];
	m_ppiL1RefPOC.insert(std::pair<UInt, PUT_MP_ARRAY_TYPE**>(uiViewId, ppiTemp));

	ppiTemp = new PUT_MP_ARRAY_TYPE*[m_uiHeight]; for(Int i=0;i<m_uiHeight;i++) ppiTemp[i] = new PUT_MP_ARRAY_TYPE[m_uiWidth];
	m_ppiL1MvX.insert(std::pair<UInt, PUT_MP_ARRAY_TYPE**>(uiViewId, ppiTemp));

	ppiTemp = new PUT_MP_ARRAY_TYPE*[m_uiHeight]; for(Int i=0;i<m_uiHeight;i++) ppiTemp[i] = new PUT_MP_ARRAY_TYPE[m_uiWidth];
	m_ppiL1MvY.insert(std::pair<UInt, PUT_MP_ARRAY_TYPE**>(uiViewId, ppiTemp));
*/

	ppiTemp = (PUT_MP_ARRAY_TYPE**)malloc(sizeof(PUT_MP_ARRAY_TYPE*)*m_uiHeight);
	piTemp = (PUT_MP_ARRAY_TYPE*)malloc(sizeof(PUT_MP_ARRAY_TYPE)*m_uiHeight*m_uiWidth);
	for(UInt i=0, addr=0;i<m_uiHeight;i++,addr+=m_uiWidth) ppiTemp[i] = &(piTemp[addr]);
	m_ppiL0RefPOC.insert(std::pair<UInt, PUT_MP_ARRAY_TYPE**>(uiViewId, ppiTemp));

	ppiTemp = (PUT_MP_ARRAY_TYPE**)malloc(sizeof(PUT_MP_ARRAY_TYPE*)*m_uiHeight);
	piTemp = (PUT_MP_ARRAY_TYPE*)malloc(sizeof(PUT_MP_ARRAY_TYPE)*m_uiHeight*m_uiWidth);
	for(UInt i=0, addr=0;i<m_uiHeight;i++,addr+=m_uiWidth) ppiTemp[i] = &(piTemp[addr]);
	m_ppiL0MvX.insert(std::pair<UInt, PUT_MP_ARRAY_TYPE**>(uiViewId, ppiTemp));

	ppiTemp = (PUT_MP_ARRAY_TYPE**)malloc(sizeof(PUT_MP_ARRAY_TYPE*)*m_uiHeight);
	piTemp = (PUT_MP_ARRAY_TYPE*)malloc(sizeof(PUT_MP_ARRAY_TYPE)*m_uiHeight*m_uiWidth);
	for(UInt i=0, addr=0;i<m_uiHeight;i++,addr+=m_uiWidth) ppiTemp[i] = &(piTemp[addr]);
	m_ppiL0MvY.insert(std::pair<UInt, PUT_MP_ARRAY_TYPE**>(uiViewId, ppiTemp));

	ppiTemp = (PUT_MP_ARRAY_TYPE**)malloc(sizeof(PUT_MP_ARRAY_TYPE*)*m_uiHeight);
	piTemp = (PUT_MP_ARRAY_TYPE*)malloc(sizeof(PUT_MP_ARRAY_TYPE)*m_uiHeight*m_uiWidth);
	for(UInt i=0, addr=0;i<m_uiHeight;i++,addr+=m_uiWidth) ppiTemp[i] = &(piTemp[addr]);
	m_ppiL1RefPOC.insert(std::pair<UInt, PUT_MP_ARRAY_TYPE**>(uiViewId, ppiTemp));

	ppiTemp = (PUT_MP_ARRAY_TYPE**)malloc(sizeof(PUT_MP_ARRAY_TYPE*)*m_uiHeight);
	piTemp = (PUT_MP_ARRAY_TYPE*)malloc(sizeof(PUT_MP_ARRAY_TYPE)*m_uiHeight*m_uiWidth);
	for(UInt i=0, addr=0;i<m_uiHeight;i++,addr+=m_uiWidth) ppiTemp[i] = &(piTemp[addr]);
	m_ppiL1MvX.insert(std::pair<UInt, PUT_MP_ARRAY_TYPE**>(uiViewId, ppiTemp));

	ppiTemp = (PUT_MP_ARRAY_TYPE**)malloc(sizeof(PUT_MP_ARRAY_TYPE*)*m_uiHeight);
	piTemp = (PUT_MP_ARRAY_TYPE*)malloc(sizeof(PUT_MP_ARRAY_TYPE)*m_uiHeight*m_uiWidth);
	for(UInt i=0, addr=0;i<m_uiHeight;i++,addr+=m_uiWidth) ppiTemp[i] = &(piTemp[addr]);
	m_ppiL1MvY.insert(std::pair<UInt, PUT_MP_ARRAY_TYPE**>(uiViewId, ppiTemp));
}
#endif

Void TComMP::uninit()
{
	if(!m_bInit) return;

	m_bInit = false;
	m_bEnabled = false;
	m_bDBMPEnabled = false;
	
	m_pcRefPicsList = NULL;
	m_pcDepthRefPicsList = NULL;
#if POZNAN_MP_USE_DEPTH_MAP_GENERATION
	m_pcDepthMapGenerator = NULL;
#else
	//m_pcCameraData = NULL;
	m_aiBaseViewShiftLUT = NULL;
#endif

	if(m_ppiMvPtCorrX!=NULL)
	{
		//for(Int i=0;i<m_uiHeight;i++) delete [] m_ppiMvPtCorrX[i];
		//delete [] m_ppiMvPtCorrX;
		free(m_ppiMvPtCorrX[0]);
		free(m_ppiMvPtCorrX);
		m_ppiMvPtCorrX = NULL;
	}

	if(m_ppiMvPtCorrY!=NULL)
	{
		//for(Int i=0;i<m_uiHeight;i++) delete [] m_ppiMvPtCorrY[i];
		//delete [] m_ppiMvPtCorrY;
		free(m_ppiMvPtCorrY[0]);
		free(m_ppiMvPtCorrY);
		m_ppiMvPtCorrY = NULL;
	}

	if(m_ppiMvPtCorrZ!=NULL)
	{
		//for(Int i=0;i<m_uiHeight;i++) delete [] m_ppiMvPtCorrZ[i];
		//delete [] m_ppiMvPtCorrZ;
		free(m_ppiMvPtCorrZ[0]);
		free(m_ppiMvPtCorrZ);
		m_ppiMvPtCorrZ = NULL;
	}

	if(m_ppiMvPtCorrRefViewIdx!=NULL)
	{
		//for(Int i=0;i<m_uiHeight;i++) delete [] m_ppiMvPtCorrRefViewIdx[i];
		//delete [] m_ppiMvPtCorrRefViewIdx;
		free(m_ppiMvPtCorrRefViewIdx[0]);
		free(m_ppiMvPtCorrRefViewIdx);
		m_ppiMvPtCorrRefViewIdx = NULL;
	}

	if(m_pppcRefCU!=NULL)
	{
		//for(Int i=0;i<m_uiHeight;i++) delete [] m_pppcRefCU[i];
		//delete [] m_pppcRefCU;
		free(m_pppcRefCU[0]);
		free(m_pppcRefCU);
		m_pppcRefCU = NULL;
	}

	if(m_ppuicRefPartAddr!=NULL)
	{
		//for(Int i=0;i<m_uiHeight;i++) delete [] m_ppuicRefPartAddr[i];
		//delete [] m_ppuicRefPartAddr;
		free(m_ppuicRefPartAddr[0]);
		free(m_ppuicRefPartAddr);
		m_ppuicRefPartAddr = NULL;
	}

#if !POZNAN_MP_USE_DEPTH_MAP_GENERATION && POZNAN_MP_USE_CURRENT_VIEW_DEPTH_MAP_IF_AVAILABLE
	if(m_ppiTempMvPtCorrX!=NULL)
	{
		//for(Int i=0;i<m_uiHeight;i++) delete [] m_ppiTempMvPtCorrX[i];
		//delete [] m_ppiTempMvPtCorrX;
		free(m_ppiTempMvPtCorrX[0]);
		free(m_ppiTempMvPtCorrX);
		m_ppiTempMvPtCorrX = NULL;
	}

	if(m_ppiTempMvPtCorrY!=NULL)
	{
		//for(Int i=0;i<m_uiHeight;i++) delete [] m_ppiTempMvPtCorrY[i];
		//delete [] m_ppiTempMvPtCorrY;
		free(m_ppiTempMvPtCorrY[0]);
		free(m_ppiTempMvPtCorrY);
		m_ppiTempMvPtCorrY = NULL;
	}

	if(m_ppiTempMvPtCorrZ!=NULL)
	{
		//for(Int i=0;i<m_uiHeight;i++) delete [] m_ppiTempMvPtCorrZ[i];
		//delete [] m_ppiTempMvPtCorrZ;
		free(m_ppiTempMvPtCorrZ[0]);
		free(m_ppiTempMvPtCorrZ);
		m_ppiTempMvPtCorrZ = NULL;
	}

#endif

#if POZNAN_DBMP & !POZNAN_DBMP_COMPRESS_ME_DATA
	PUT_MP_ARRAY_TYPE** ppiTemp;
	TComMapppi::const_iterator cMapIter;

    for (cMapIter = m_ppiL0RefPOC.begin(); cMapIter != m_ppiL0RefPOC.end(); cMapIter++)
	{
		ppiTemp = cMapIter->second;
		if(ppiTemp!=NULL)
		{
			//for(Int i=0;i<m_uiHeight;i++) delete [] ppiTemp[i];
			//delete [] ppiTemp;
			free(ppiTemp[0]);
			free(ppiTemp);
			ppiTemp = NULL;
		}
	}
	
	for (cMapIter = m_ppiL0MvX.begin(); cMapIter != m_ppiL0MvX.end(); cMapIter++)
	{
		ppiTemp = cMapIter->second;
		if(ppiTemp!=NULL)
		{
			//for(Int i=0;i<m_uiHeight;i++) delete [] ppiTemp[i];
			//delete [] ppiTemp;
			free(ppiTemp[0]);
			free(ppiTemp);
			ppiTemp = NULL;
		}
	}

	for (cMapIter = m_ppiL0MvY.begin(); cMapIter != m_ppiL0MvY.end(); cMapIter++)
	{
		ppiTemp = cMapIter->second;
		if(ppiTemp!=NULL)
		{
			//for(Int i=0;i<m_uiHeight;i++) delete [] ppiTemp[i];
			//delete [] ppiTemp;
			free(ppiTemp[0]);
			free(ppiTemp);
			ppiTemp = NULL;
		}
	}

	for (cMapIter = m_ppiL1RefPOC.begin(); cMapIter != m_ppiL1RefPOC.end(); cMapIter++)
	{
		ppiTemp = cMapIter->second;
		if(ppiTemp!=NULL)
		{
			//for(Int i=0;i<m_uiHeight;i++) delete [] ppiTemp[i];
			//delete [] ppiTemp;
			free(ppiTemp[0]);
			free(ppiTemp);
			ppiTemp = NULL;
		}
	}

	for (cMapIter = m_ppiL1MvX.begin(); cMapIter != m_ppiL1MvX.end(); cMapIter++)
	{
		ppiTemp = cMapIter->second;
		if(ppiTemp!=NULL)
		{
			//for(Int i=0;i<m_uiHeight;i++) delete [] ppiTemp[i];
			//delete [] ppiTemp;
			free(ppiTemp[0]);
			free(ppiTemp);
			ppiTemp = NULL;
		}
	}

	for (cMapIter = m_ppiL1MvY.begin(); cMapIter != m_ppiL1MvY.end(); cMapIter++)
	{
		ppiTemp = cMapIter->second;
		if(ppiTemp!=NULL)
		{
			//for(Int i=0;i<m_uiHeight;i++) delete [] ppiTemp[i];
			//delete [] ppiTemp;
			free(ppiTemp[0]);
			free(ppiTemp);
			ppiTemp = NULL;
		}
	}
#endif

#if POZNAN_DBMP_CALC_PRED_DATA
	if(m_piTempL0RefIdx!=NULL)	{ delete [] m_piTempL0RefIdx; m_piTempL0RefIdx = NULL; }
	if(m_piTempL0MvX!=NULL)		{ delete [] m_piTempL0MvX; m_piTempL0MvX = NULL; }
	if(m_piTempL0MvY!=NULL)		{ delete [] m_piTempL0MvY; m_piTempL0MvY = NULL; }
	if(m_piTempL1RefIdx!=NULL)	{ delete [] m_piTempL1RefIdx; m_piTempL1RefIdx = NULL; }
	if(m_piTempL1MvX!=NULL)		{ delete [] m_piTempL1MvX; m_piTempL1MvX = NULL; }
	if(m_piTempL1MvY!=NULL)		{ delete [] m_piTempL1MvY; m_piTempL1MvY = NULL; }
	if(m_piTempBuff!=NULL)		{ delete [] m_piTempBuff; m_piTempBuff = NULL; }
	if(m_piResBuff!=NULL)		{ delete [] m_piResBuff; m_piResBuff = NULL; }
#endif	

#if POZNAN_DBMP_CALC_PRED_DATA
  apcDBMPPredMVField[0]->destroy(); delete apcDBMPPredMVField[0]; apcDBMPPredMVField[0] = NULL;
  apcDBMPPredMVField[1]->destroy(); delete apcDBMPPredMVField[1]; apcDBMPPredMVField[1] = NULL;
#endif

	m_uiHeight = 0;
	m_uiWidth = 0;
}

Void TComMP::clear()
{
	Int i,j;

	m_bEnabled = false;
	m_bDBMPEnabled = false;

	for(i=0;i<m_uiHeight;i++)
		for(j=0;j<m_uiWidth;j++)
		{
			m_ppiMvPtCorrX[i][j] = TComMP::OCCLUSION;
			m_ppiMvPtCorrY[i][j] = TComMP::OCCLUSION;
			//m_ppiMvPtCorrZ[i][j] = 0x7FFF; //Depth
			m_ppiMvPtCorrZ[i][j] = TComMP::OCCLUSION; //Disparity
			m_ppiMvPtCorrRefViewIdx[i][j] = TComMP::OCCLUSION;

			m_pppcRefCU[i][j] = NULL;
			m_ppuicRefPartAddr[i][j] = 0;
		}
}

Void TComMP::disable()
{
	m_bEnabled = false;
	m_bDBMPEnabled = false;

	m_pcRefPicsList = NULL;
	m_pcDepthRefPicsList = NULL;
#if POZNAN_MP_USE_DEPTH_MAP_GENERATION
	m_pcDepthMapGenerator = NULL;
#endif
}

#if !POZNAN_MP_USE_DEPTH_MAP_GENERATION && POZNAN_MP_USE_CURRENT_VIEW_DEPTH_MAP_IF_AVAILABLE
Void TComMP::clearTemp()
{
	if(!isInit()) return;

	for(Int i=0;i<m_uiHeight;i++)
		for(Int j=0;j<m_uiWidth;j++)
		{
			m_ppiTempMvPtCorrX[i][j] = TComMP::OCCLUSION;
			m_ppiTempMvPtCorrY[i][j] = TComMP::OCCLUSION;
			//m_ppiTempMvPtCorrZ[i][j] = 0x7FFF; //Depth
			m_ppiTempMvPtCorrZ[i][j] = TComMP::OCCLUSION; //Disparity
		}
}
#endif

Bool TComMP::isInit()
{
	return m_bInit;
}

Bool TComMP::isEnabled()
{
	return m_bEnabled;
}

Bool TComMP::isDBMPEnabled()
{
	return m_bDBMPEnabled;
}

Bool TComMP::pairMultiview(TComPic* pcPic)
{
	m_bEnabled = false;
	m_bDBMPEnabled = false;

	if(!isInit() || pcPic==NULL) return false;
#if POZNAN_MP_USE_DEPTH_MAP_GENERATION
	if(m_pcDepthMapGenerator==NULL) return false;
#else
	//if(m_pcCameraData==NULL) return false;
	if(m_aiBaseViewShiftLUT==NULL) return false;
#endif
	//if(pcPic->getSlice(0)->isIntra()) return true; //no motion compenstation is used!!!

	UInt uiViewId = pcPic->getSPS()->getViewId(); //pcPic->getViewIdx();//???
	UInt uiPOC = pcPic->getPOC();
	Bool bIsDepth = pcPic->getSPS()->isDepth();

	if(uiViewId==0) return true; //no reference view available!!!
	
#if POZNAN_MP_USE_DEPTH_MAP_GENERATION
	if(pcPic->getPredDepthMap()==NULL) return false; //no depth available!!!
#else
	if(m_pcDepthRefPicsList->empty()) return false; //no depth in reference views available!!!
#endif

	Int iRefX, iRefY;
	Int iCurX, iCurY;
	UInt uiRefViewId;	
	TComPic* pcRefPic;
	Int iDisparity;	
	TComDataCU* pcRefCU;

#if POZNAN_MP_USE_DEPTH_MAP_GENERATION
	Int iShiftX;
	Int iAddX;
#else
	Int iSynthViewIdx;
	Int** ppiShiftLUTLeft;
	TComPic* pcDepthPic;
	Int iStride;
	Pel* pDepth;
	//Bool bIsLeft;
	Int  iDepth;
#if POZNAN_MP_USE_CURRENT_VIEW_DEPTH_MAP_IF_AVAILABLE
	Int iSrcX, iSrcY;
#endif	
#endif	

	Bool bIsRefNoISliceAvailable = false;
	Bool bIsAnchorPicture = false;

	clear();

#if POZNAN_MP_USE_DEPTH_MAP_GENERATION
	for( UInt uiIdx = 0; uiIdx < uiViewId; uiIdx++ )
	//for( UInt uiIdx = uiViewId-1; uiIdx >= uiViewId; uiIdx-- )
	{
		uiRefViewId = m_pcDepthMapGenerator->getBaseViewId( uiIdx );

		iShiftX = m_pcDepthMapGenerator->getSubSampExpX() + 2;
		iAddX = ( 1 << iShiftX ) >> 1;
      
		pcRefPic = (*m_pcRefPicsList)[uiRefViewId];

		if(!pcRefPic->getSlice(0)->isIntra()) bIsRefNoISliceAvailable = true;
		else bIsAnchorPicture = true;

		for(iCurY=0; iCurY<(Int)m_uiHeight; iCurY++)
			for(iCurX=0; iCurX<(Int)m_uiWidth; iCurX++)
			if((m_ppiMvPtCorrX[iCurY][iCurX]==TComMP::OCCLUSION || m_ppiMvPtCorrY[iCurY][iCurX]==TComMP::OCCLUSION))
			{
				iDisparity = m_pcDepthMapGenerator->getDisparity( pcPic, iCurX >> m_pcDepthMapGenerator->getSubSampExpX(), iCurY >> m_pcDepthMapGenerator->getSubSampExpY(), uiRefViewId );
				iRefX = iCurX + ( ( iDisparity + iAddX ) >> iShiftX );
				iRefY = iCurY;
				
				if(iRefX>=0 && iRefX<m_uiWidth && iRefY>=0 && iRefY<m_uiHeight)
				{
					//m_ppiMvPtCorrRefViewIdx[iCurY][iCurX] = (Short)uiRefViewId;
					m_ppiMvPtCorrX[iCurY][iCurX] = (Short)iRefX;
					m_ppiMvPtCorrY[iCurY][iCurX] = (Short)iRefY;

					Pel*   piPdmMap    = pcPic->getPredDepthMap()->getLumaAddr( 0 );
					Int    iStride     = pcPic->getPredDepthMap()->getStride  ();
					Int    iPrdDepth   = piPdmMap[ iCurX + iCurY * iStride ];
					m_ppiMvPtCorrZ[iCurY][iCurX] = (Short)iPrdDepth;
					//m_ppiMvPtCorrZ[iCurY][iCurX] = (Short)iDisparity;						

					pcRefCU = pcRefPic->getCU((UInt)(iRefY/pcRefPic->getSlice(0)->getSPS()->getMaxCUHeight()*pcRefPic->getFrameWidthInCU() + iRefX/pcRefPic->getSlice(0)->getSPS()->getMaxCUWidth()));

					m_pppcRefCU[iCurY][iCurX] = pcRefCU;
					m_ppuicRefPartAddr[iCurY][iCurX] = (UShort)g_auiRasterToZscan[(iRefY-pcRefCU->getCUPelY())/pcRefPic->getMinCUHeight()*pcRefPic->getNumPartInWidth()+(iRefX-pcRefCU->getCUPelX())/pcRefPic->getMinCUWidth()];
				}				
			}
	}
#else

#if POZNAN_MP_USE_CURRENT_VIEW_DEPTH_MAP_IF_AVAILABLE
	//pcDepthPic = GetPicFromList(m_pcDepthRefPicsList, uiPOC, uiViewId);
	if(pcPic->getSlice(0)->getDepthPic()!=NULL && pcPic->getSlice(0)->getDepthPic()->getReconMark())
		pcDepthPic = pcPic->getSlice(0)->getDepthPic();
	else
		pcDepthPic = NULL;

	if(pcDepthPic!=NULL)
	{
	  pDepth = pcDepthPic->getPicYuvRec()->getLumaAddr();
	  iStride = pcDepthPic->getPicYuvRec()->getStride();

	  for( UInt uiIdx = 0; uiIdx < uiViewId; uiIdx++ )
	  //for( UInt uiIdx = uiViewId-1; uiIdx >= uiViewId; uiIdx-- )
	  {
	    uiRefViewId = uiIdx;

		pcRefPic = GetPicFromList(m_pcRefPicsList, uiPOC, uiRefViewId);
		assert(pcRefPic!=NULL);//test
		if(pcRefPic==NULL) return false; //No ref pic with current POC and RefView found in ref list!!!

		if(!pcRefPic->getSlice(0)->isIntra()) bIsRefNoISliceAvailable = true;
		else bIsAnchorPicture = true;

		//ppiShiftLUTLeft = m_pcCameraData->getBaseViewShiftLUTI()[uiViewId][uiRefViewId];
		ppiShiftLUTLeft = m_aiBaseViewShiftLUT[uiViewId][uiRefViewId];

		clearTemp();

		for(iCurY=0; iCurY<(Int)m_uiHeight; iCurY++)
			for(iCurX=0; iCurX<(Int)m_uiWidth; iCurX++)
			{
				//Check if point already has its reference:
				if(m_ppiMvPtCorrX[iCurY][iCurX]!=TComMP::OCCLUSION && m_ppiMvPtCorrY[iCurY][iCurX]!=TComMP::OCCLUSION) continue;

				iDepth = pDepth[ iCurX + iCurY * iStride ];
				iDepth = RemoveBitIncrement(iDepth);
				assert( iDepth >= 0 && iDepth <= 256 );					
				
				//if(bIsLeft) iDisparity = ppiShiftLUTLeft[0][iDepth];
				//else iDisparity = -ppiShiftLUTLeft[0][iDepth];
				iDisparity = ppiShiftLUTLeft[0][iDepth]; //!!!

				iRefX = iCurX - ( ( iDisparity + 2 ) >> 2 );
				iRefY = iCurY;

				if(iRefX>=0 && iRefX<m_uiWidth && iRefY>=0 && iRefY<m_uiHeight)
				{
					iSrcX = m_ppiTempMvPtCorrX[iRefY][iRefX];
					iSrcY = m_ppiTempMvPtCorrY[iRefY][iRefX];

					if(iSrcX==TComMP::OCCLUSION || iSrcY==TComMP::OCCLUSION)
					{
						m_ppiMvPtCorrRefViewIdx[iCurY][iCurX] = (Short)uiRefViewId;
						m_ppiMvPtCorrX[iCurY][iCurX] = (Short)iRefX;
						m_ppiMvPtCorrY[iCurY][iCurX] = (Short)iRefY;

						m_ppiMvPtCorrZ[iCurY][iCurX] = (Short)iDepth;
						//m_ppiMvPtCorrZ[iCurY][iCurX] = (Short)iDisparity;						

						pcRefCU = pcRefPic->getCU((UInt)(iRefY/pcRefPic->getSlice(0)->getSPS()->getMaxCUHeight()*pcRefPic->getFrameWidthInCU() + iRefX/pcRefPic->getSlice(0)->getSPS()->getMaxCUWidth()));

						m_pppcRefCU[iCurY][iCurX] = pcRefCU;
						m_ppuicRefPartAddr[iCurY][iCurX] = (UShort)g_auiRasterToZscan[(iRefY-pcRefCU->getCUPelY())/pcRefPic->getMinCUHeight()*pcRefPic->getNumPartInWidth()+(iRefX-pcRefCU->getCUPelX())/pcRefPic->getMinCUWidth()];

						m_ppiTempMvPtCorrX[iRefY][iRefX] = iCurX;
						m_ppiTempMvPtCorrY[iRefY][iRefX] = iCurY;
						m_ppiTempMvPtCorrZ[iRefY][iRefX] = (Short)iDepth;
						//m_ppiTempMvPtCorrZ[iRefY][iRefX] = (Short)iDisparity;
					}
					else if((Short)iDepth>m_ppiTempMvPtCorrZ[iRefY][iRefX]) //Point assigned earlier to this location is occluded
					//else if((Short)iDisparity>m_ppiTempMvPtCorrZ[iRefY][iRefX]) //Point assigned earlier to this location is occluded
					{
						m_ppiMvPtCorrRefViewIdx[iCurY][iCurX] = (Short)uiRefViewId;
						m_ppiMvPtCorrX[iCurY][iCurX] = (Short)iRefX;
						m_ppiMvPtCorrY[iCurY][iCurX] = (Short)iRefY;

						m_ppiMvPtCorrZ[iCurY][iCurX] = (Short)iDepth;
						//m_ppiMvPtCorrZ[iCurY][iCurX] = (Short)iDisparity;						

						pcRefCU = pcRefPic->getCU((UInt)(iRefY/pcRefPic->getSlice(0)->getSPS()->getMaxCUHeight()*pcRefPic->getFrameWidthInCU() + iRefX/pcRefPic->getSlice(0)->getSPS()->getMaxCUWidth()));

						m_pppcRefCU[iCurY][iCurX] = pcRefCU;
						m_ppuicRefPartAddr[iCurY][iCurX] = (UShort)g_auiRasterToZscan[(iRefY-pcRefCU->getCUPelY())/pcRefPic->getMinCUHeight()*pcRefPic->getNumPartInWidth()+(iRefX-pcRefCU->getCUPelX())/pcRefPic->getMinCUWidth()];

						m_ppiTempMvPtCorrX[iRefY][iRefX] = iCurX;
						m_ppiTempMvPtCorrY[iRefY][iRefX] = iCurY;
						m_ppiTempMvPtCorrZ[iRefY][iRefX] = (Short)iDepth;
						//m_ppiTempMvPtCorrZ[iRefY][iRefX] = (Short)iDisparity;
						
						//Mark point assigned earlier to this location as occluded:
						m_ppiMvPtCorrRefViewIdx[iSrcY][iSrcX] = TComMP::OCCLUSION;
						m_ppiMvPtCorrX[iSrcY][iSrcX] = TComMP::OCCLUSION;
						m_ppiMvPtCorrY[iSrcY][iSrcX] = TComMP::OCCLUSION;

						//m_ppiMvPtCorrZ[iSrcY][iSrcX] = 0x7FFF;
						m_ppiMvPtCorrZ[iSrcY][iSrcX] = TComMP::OCCLUSION;						

						m_pppcRefCU[iSrcY][iSrcX] = NULL;
						m_ppuicRefPartAddr[iSrcY][iSrcX] = 0;						
					}
				}
			}
	  }
	}
	else
#endif
	{
	iSynthViewIdx = uiViewId;

	for( UInt uiIdx = 0; uiIdx < uiViewId; uiIdx++ )
	//for( UInt uiIdx = uiViewId-1; uiIdx >= uiViewId; uiIdx-- )
	{
		uiRefViewId = uiIdx;

		pcRefPic = GetPicFromList(m_pcRefPicsList, uiPOC, uiRefViewId);
		assert(pcRefPic!=NULL);//test
		if(pcRefPic==NULL) return false; //No ref pic with current POC and RefView found in ref list!!!

		if(!pcRefPic->getSlice(0)->isIntra()) bIsRefNoISliceAvailable = true;
		else bIsAnchorPicture = true;

		//ppiShiftLUTLeft = m_pcCameraData->getBaseViewShiftLUTI()[uiRefViewId][iSynthViewIdx];
		ppiShiftLUTLeft = m_aiBaseViewShiftLUT[uiRefViewId][iSynthViewIdx];

		pcDepthPic = GetPicFromList(m_pcDepthRefPicsList, uiPOC, uiRefViewId);
		assert(pcDepthPic!=NULL);//test
		if(pcDepthPic==NULL) return false; //No depth with current POC and RefView found in ref list!!!

		pDepth = pcDepthPic->getPicYuvRec()->getLumaAddr();
		iStride = pcDepthPic->getPicYuvRec()->getStride();

		//bIsLeft = m_pcCameraData->isLeftView(iSynthViewIdx, uiRefViewId);

		for(iRefY=0; iRefY<(Int)m_uiHeight; iRefY++)
			for(iRefX=0; iRefX<(Int)m_uiWidth; iRefX++)
			{				
				iDepth = pDepth[ iRefX + iRefY * iStride ];
				iDepth = RemoveBitIncrement(iDepth);
				assert( iDepth >= 0 && iDepth <= 256 );					
				
				//if(bIsLeft) iDisparity = ppiShiftLUTLeft[0][iDepth];
				//else iDisparity = -ppiShiftLUTLeft[0][iDepth];
				iDisparity = ppiShiftLUTLeft[0][iDepth]; //!!!

				iCurX = iRefX - ( ( iDisparity + 2 ) >> 2 );
				iCurY = iRefY;
				
				if(iCurX>=0 && iCurX<m_uiWidth && iCurY>=0 && iCurY<m_uiHeight
					&& ((m_ppiMvPtCorrX[iCurY][iCurX]==TComMP::OCCLUSION || m_ppiMvPtCorrY[iCurY][iCurX]==TComMP::OCCLUSION)
						|| ((m_ppiMvPtCorrRefViewIdx[iCurY][iCurX] == (Short)uiRefViewId) && (m_ppiMvPtCorrZ[iCurY][iCurX]<iDepth))))
				{
					m_ppiMvPtCorrRefViewIdx[iCurY][iCurX] = (Short)uiRefViewId;
					m_ppiMvPtCorrX[iCurY][iCurX] = (Short)iRefX;
					m_ppiMvPtCorrY[iCurY][iCurX] = (Short)iRefY;

					m_ppiMvPtCorrZ[iCurY][iCurX] = (Short)iDepth;
					//m_ppiMvPtCorrZ[iCurY][iCurX] = (Short)iDisparity;						

					pcRefCU = pcRefPic->getCU((UInt)(iRefY/pcRefPic->getSlice(0)->getSPS()->getMaxCUHeight()*pcRefPic->getFrameWidthInCU() + iRefX/pcRefPic->getSlice(0)->getSPS()->getMaxCUWidth()));

					m_pppcRefCU[iCurY][iCurX] = pcRefCU;
					m_ppuicRefPartAddr[iCurY][iCurX] = (UShort)g_auiRasterToZscan[(iRefY-pcRefCU->getCUPelY())/pcRefPic->getMinCUHeight()*pcRefPic->getNumPartInWidth()+(iRefX-pcRefCU->getCUPelX())/pcRefPic->getMinCUWidth()];
				}				
			}
	}
      }
#endif

#if POZNAN_MP_FILL
	fillMultiview(uiViewId);
#endif

#if POZNAN_DBMP & !POZNAN_DBMP_COMPRESS_ME_DATA
	for( UInt uiIdx = 0; uiIdx <= uiViewId; uiIdx++ ) initArrayMap(uiIdx, bIsDepth);
#endif
	
	m_uiView = uiViewId;
	m_uiPOC = uiPOC;
	m_bIsDepth = bIsDepth;
	m_bEnabled = true;

#if POZNAN_DBMP
	m_bDBMPEnabled = (pcPic->getSPS()->getDBMP() > 0) & m_bEnabled & bIsRefNoISliceAvailable; //no reference view with motion data available
#if POZNAN_DBMP_USE_IN_NONANCHOR_PIC_ONLY
	m_bDBMPEnabled &= !bIsAnchorPicture; //is non-anchor picture
#endif
#if !POZNAN_DBMP_USE_FOR_TEXTURE
	if(!m_bIsDepth) m_bDBMPEnabled = false;
#endif
#if !POZNAN_DBMP_USE_FOR_DEPTH
	if(m_bIsDepth) m_bDBMPEnabled = false;
#endif
#else
	m_bDBMPEnabled = false;
#endif

	return true;

}

#if POZNAN_MP_FILL
Void TComMP::fillMultiview(UInt uiViewId)
{
	Int iPicXpos,iPicYpos;
	Int iRefXpos,iRefYpos;
	Int iRefXPos1,iRefYPos1,iRefXPos2,iRefYPos2;

	for(iPicYpos=0; iPicYpos<(Int)m_uiHeight;iPicYpos++)
		for(iPicXpos=0; iPicXpos<(Int)m_uiWidth;iPicXpos++)
			if(m_ppiMvPtCorrX[iPicYpos][iPicXpos] == TComMP::OCCLUSION)
			{
				//Check left-hand  neighbours:
				iRefXPos1=TComMP::OCCLUSION;
				iRefYPos1=TComMP::OCCLUSION;

				iRefYpos=iPicYpos;
				for(iRefXpos=iPicXpos-1;iRefXpos>=0;iRefXpos--) 
					if(m_ppiMvPtCorrX[iRefYpos][iRefXpos]!=TComMP::OCCLUSION)//Counterpart pixel in reference view exists
					{
						iRefXPos1 = iRefXpos;
						iRefYPos1 = iRefYpos;
						break;
					}

				//Check right-hand  neighbours:
				iRefXPos2=TComMP::OCCLUSION;
				iRefYPos2=TComMP::OCCLUSION;

				iRefYpos=iPicYpos;
				for(iRefXpos=iPicXpos+1;iRefXpos<m_uiWidth;iRefXpos++) 
					if(m_ppiMvPtCorrX[iRefYpos][iRefXpos]!=TComMP::OCCLUSION)//Counterpart pixel in reference view exists
					{
						iRefXPos2 = iRefXpos;
						iRefYPos2 = iRefYpos;
						break;
					}

#if POZNAN_MP_FILL_TYPE==0

				//Choose point with smaller disparity:
				if(iRefXPos1!=TComMP::OCCLUSION && iRefYPos1!=TComMP::OCCLUSION)
				{
					if(iRefXPos2!=TComMP::OCCLUSION && iRefYPos2!=TComMP::OCCLUSION && m_ppiMvPtCorrZ[iRefYPos1][iRefXPos1]>m_ppiMvPtCorrZ[iRefYPos2][iRefXPos2])
					{
						iRefXpos=iRefXPos2;
						iRefYpos=iRefYPos2;
					}
					else
					{
						iRefXpos=iRefXPos1;
						iRefYpos=iRefYPos1;
					}
				}
				else
				{
					iRefXpos=iRefXPos2;
					iRefYpos=iRefYPos2;
				}
				if(iRefXpos==TComMP::OCCLUSION && iRefYpos==TComMP::OCCLUSION) continue;

#elif POZNAN_MP_FILL_TYPE==1

				//Choose point with larger disparity:
				if(iRefXPos1!=TComMP::OCCLUSION && iRefYPos1!=TComMP::OCCLUSION)
				{
					if(iRefXPos2!=TComMP::OCCLUSION && iRefYPos2!=TComMP::OCCLUSION && m_ppiMvPtCorrZ[iRefYPos1][iRefXPos1]<m_ppiMvPtCorrZ[iRefYPos2][iRefXPos2])
					{
						iRefXpos=iRefXPos2;
						iRefYpos=iRefYPos2;
					}
					else
					{
						iRefXpos=iRefXPos1;
						iRefYpos=iRefYPos1;
					}
				}
				else
				{
					iRefXpos=iRefXPos2;
					iRefYpos=iRefYPos2;
				}
				if(iRefXpos==TComMP::OCCLUSION && iRefYpos==TComMP::OCCLUSION) continue;

#else
				assert(0);
#endif
				
				//m_ppiMvPtCorrRefViewIdx[iPicYpos][iPicXpos] = m_ppiMvPtCorrRefViewIdx[iRefYpos][iRefXpos];
				m_ppiMvPtCorrX[iPicYpos][iPicXpos] = m_ppiMvPtCorrX[iRefYpos][iRefXpos]; //m_ppiMvPtCorrX[iPicYpos][iPicXpos] = iRefXpos;
				m_ppiMvPtCorrY[iPicYpos][iPicXpos] = m_ppiMvPtCorrY[iRefYpos][iRefXpos]; //m_ppiMvPtCorrY[iPicYpos][iPicXpos] = iRefYpos;
				
				//m_ppiMvPtCorrZ[iPicYpos][iPicXpos] = m_ppiMvPtCorrZ[iRefYpos][iRefXpos];
				m_pppcRefCU[iPicYpos][iPicXpos] = m_pppcRefCU[iRefYpos][iRefXpos];
				m_ppuicRefPartAddr[iPicYpos][iPicXpos] = m_ppuicRefPartAddr[iRefYpos][iRefXpos];
			}

	return;
}
#endif

TComPic* TComMP::GetPicFromList(std::vector<TComPic*>* pcListPic,UInt uiPOC, UInt uiView) //Get Pic from list based on POC ad View
{
  //  find current position
  std::vector<TComPic*>::iterator  iterPic;
  TComPic*                      pcPic   = NULL;
  
  for(iterPic = pcListPic->begin(); iterPic != pcListPic->end(); iterPic++)
  {
	pcPic = *(iterPic);
	if((pcPic->getPOC() == (Int)uiPOC) && (pcPic->getViewIdx() == (Int)uiView) && pcPic->getReconMark()) return pcPic;
	//if((pcPic->getPOC() == (Int)uiPOC) && (pcPic->getViewIdx() == (Int)uiView) ) return pcPic;
  }
  assert (0);

  return NULL;
}

Int TComMP::GetRepresentativeVal(UInt &uiCntMax, Int* piIn, UInt uiLen, Int* piTemp, Bool bUseRestrictedVal, Int iRestrictedVal)
{
	UInt i,j,cnt,idx;

	uiCntMax = 0; idx = -1;
	for(i=0;i<uiLen;i++) piTemp[i] = 0;
	for(i=0;i<uiLen;i++)
	{
		if(piTemp[i] || (bUseRestrictedVal && piIn[i]==iRestrictedVal)) continue;
		cnt = 0; for(j=0;j<uiLen;j++) if(piIn[j]==piIn[i]) {cnt++; piTemp[j] = 1;}
		if(cnt>uiCntMax || (cnt==uiCntMax && abs(piIn[i])<abs(piIn[idx]))) {uiCntMax = cnt; idx = i;}		
	}

	if(idx==-1) return iRestrictedVal;
	return piIn[idx];
}

#if POZNAN_DBMP
Void TComMP::getDBMPPredData(TComDataCU* pcCU, Int x, Int y, 
											Int &ref_frame0, Int &ref_frame0_idx, TComMv &mv0, Int ref_frame0_idx_2nd, TComMv mv0_2nd,
											Int &ref_frame1, Int &ref_frame1_idx, TComMv &mv1, Int ref_frame1_idx_2nd, TComMv mv1_2nd)
{
	UInt uiViewId = pcCU->getSlice()->getSPS()->getViewId();
	Bool bIsDepth = pcCU->getSlice()->getSPS()->isDepth();
	Int iRefIdx;
	TComPic* pcPic;
	TComMP* pcMP = pcCU->getSlice()->getMP();

	TComDataCU*pcRefCU = pcMP->getMPCRefCU()[y][x];
	UInt uiRefPartAddr = pcMP->getMPCRefPartAddr()[y][x];

	Int x_ref = pcMP->getMPCx()[y][x];
	Int y_ref = pcMP->getMPCy()[y][x];

	ref_frame0 = -1;
	ref_frame1 = -1;			

	if(pcRefCU != NULL) //Point has its counterpart in the reference view		
	{			
#if !POZNAN_DBMP_COMPRESS_ME_DATA
		if(pcRefCU->getMergeIndex(uiRefPartAddr)==POZNAN_DBMP_MRG_CAND)
		{
			UInt uiRefViewId = pcRefCU->getSlice()->getSPS()->getViewId();

			ref_frame0 = pcMP->getL0RefPOC(uiRefViewId, bIsDepth)[y_ref][x_ref];
			mv0.setHor(pcMP->getL0MvX(uiRefViewId, bIsDepth)[y_ref][x_ref]);
			mv0.setVer(pcMP->getL0MvY(uiRefViewId, bIsDepth)[y_ref][x_ref]);
			
			ref_frame1 = pcMP->getL1RefPOC(uiRefViewId, bIsDepth)[y_ref][x_ref];
			mv1.setHor(pcMP->getL1MvX(uiRefViewId, bIsDepth)[y_ref][x_ref]);
			mv1.setVer(pcMP->getL1MvY(uiRefViewId, bIsDepth)[y_ref][x_ref]);
		}
		else
		{
#endif
			if(pcRefCU->getCUMvField(REF_PIC_LIST_0)->getRefIdx(uiRefPartAddr)>=0)
			{
				ref_frame0 = pcRefCU->getSlice()->getRefPOC( REF_PIC_LIST_0, pcRefCU->getCUMvField(REF_PIC_LIST_0)->getRefIdx(uiRefPartAddr) );
				mv0 = pcRefCU->getCUMvField( REF_PIC_LIST_0 )->getMv( uiRefPartAddr );
			}

			if(pcRefCU->getCUMvField(REF_PIC_LIST_1)->getRefIdx(uiRefPartAddr)>=0)
			{
				ref_frame1 = pcRefCU->getSlice()->getRefPOC( REF_PIC_LIST_1, pcRefCU->getCUMvField(REF_PIC_LIST_1)->getRefIdx(uiRefPartAddr) );
				mv1 = pcRefCU->getCUMvField( REF_PIC_LIST_1 )->getMv( uiRefPartAddr );
			}
#if !POZNAN_DBMP_COMPRESS_ME_DATA
		}
#endif
	}

	//Find ref_frame0 index on REF_PIC_LIST_0
	ref_frame0_idx = -1;
	if(ref_frame0 != -1)
	{				
		for( iRefIdx = 0; iRefIdx < pcCU->getSlice()->getNumRefIdx(REF_PIC_LIST_0); iRefIdx++ )
		{
			pcPic = pcCU->getSlice()->getRefPic( REF_PIC_LIST_0, iRefIdx );
			if(ref_frame0 == pcPic->getPOC() && uiViewId==pcPic->getSPS()->getViewId()){ ref_frame0_idx = iRefIdx; break;}
		}
		if(ref_frame0_idx == -1) ref_frame0 = -1;
	}
	//Find ref_frame1 index on REF_PIC_LIST_1
	ref_frame1_idx = -1;
	if(ref_frame1 != -1)
	{				
		for( iRefIdx = 0; iRefIdx < pcCU->getSlice()->getNumRefIdx(REF_PIC_LIST_1); iRefIdx++ )
		{
			pcPic = pcCU->getSlice()->getRefPic( REF_PIC_LIST_1, iRefIdx );
			if(ref_frame1 == pcPic->getPOC() && uiViewId==pcCU->getSlice()->getSPS()->getViewId()){ ref_frame1_idx = iRefIdx; break;}
		}
		if(ref_frame1_idx == -1) ref_frame1 = -1;
	}

	if(ref_frame0_idx < 0 && ref_frame1_idx < 0) //No counterpart pixel in reference view found or no motion data was found
	{
		ref_frame0_idx = ref_frame0_idx_2nd;
		mv0 = mv0_2nd;
		if(ref_frame0_idx_2nd == -1) ref_frame0 = -1;
		else ref_frame0 = pcCU->getSlice()->getRefPic( REF_PIC_LIST_0, ref_frame0_idx_2nd )->getPOC();

		ref_frame1_idx = ref_frame1_idx_2nd;
		mv1 = mv1_2nd;
		if(ref_frame1_idx_2nd == -1) ref_frame1 = -1;
		else ref_frame1 = pcCU->getSlice()->getRefPic( REF_PIC_LIST_1, ref_frame1_idx_2nd )->getPOC();
	}
	else //Counterpart point with motion data was found
	{
		if(ref_frame0_idx < 0) 
		{
			ref_frame0_idx = -1;
			mv0.setZero();
		}
		else
		{
			//use mv0 and RefIdx obtained by the DBMP algorithm
		}

		if(ref_frame1_idx < 0) 
		{
			ref_frame1_idx = -1;
			mv1.setZero();
		}
		else
		{
			//use mv0 and RefIdx obtained by the DBMP algorithm
		}
	}
}

#if !POZNAN_DBMP_COMPRESS_ME_DATA
Void TComMP::saveDBMPData(TComDataCU* pcCU)
{	
	if(!isDBMPEnabled()) return;

	Int		  px,py,x,y,iCUBaseX,iCUBaseY;

	Int		  ref_frame0, ref_frame1;
	Int		  ref_frame0_idx, ref_frame1_idx;
	TComMv	  mv0,mv1;

	Int		  ref_frame0_idx_2nd, ref_frame1_idx_2nd;
    TComMv	  mv0_2nd,mv1_2nd;

	UInt	  uiViewId = pcCU->getSlice()->getSPS()->getViewId();
	Bool	  bIsDepth = pcCU->getSlice()->getSPS()->isDepth();
	Int		  iWidth = g_uiMaxCUWidth>>g_uiMaxCUDepth;
	Int		  iHeight = g_uiMaxCUHeight>>g_uiMaxCUDepth;

	for (UInt uiPartAddr = 0; uiPartAddr < pcCU->getTotalNumPart(); uiPartAddr++ )
	{
		if(pcCU->getMergeIndex(uiPartAddr)==POZNAN_DBMP_MRG_CAND)
		{			
#if POZNAN_DBMP_CALC_PRED_DATA
			ref_frame0_idx_2nd = pcCU->getCUMvField2nd(REF_PIC_LIST_0)->getRefIdx(uiPartAddr);
			mv0_2nd = pcCU->getCUMvField2nd( REF_PIC_LIST_0 )->getMv( uiPartAddr );

			ref_frame1_idx_2nd = pcCU->getCUMvField2nd(REF_PIC_LIST_1)->getRefIdx(uiPartAddr);
			mv1_2nd = pcCU->getCUMvField2nd( REF_PIC_LIST_1 )->getMv( uiPartAddr );
#else
			ref_frame0_idx_2nd = pcCU->getCUMvField(REF_PIC_LIST_0)->getRefIdx(uiPartAddr);
			mv0_2nd = pcCU->getCUMvField( REF_PIC_LIST_0 )->getMv( uiPartAddr );

			ref_frame1_idx_2nd = pcCU->getCUMvField(REF_PIC_LIST_1)->getRefIdx(uiPartAddr);
			mv1_2nd = pcCU->getCUMvField( REF_PIC_LIST_1 )->getMv( uiPartAddr );
#endif
			iCUBaseX = pcCU->getCUPelX()+g_auiRasterToPelX[ g_auiZscanToRaster[uiPartAddr] ];
			iCUBaseY = pcCU->getCUPelY()+g_auiRasterToPelY[ g_auiZscanToRaster[uiPartAddr] ];
			for( py = 0; py < iHeight; py++)
			{
				y = iCUBaseY+py;
				for( px = 0; px < iWidth; px++)
				{
					x = iCUBaseX+px;

					getDBMPPredData(pcCU, x, y, ref_frame0, ref_frame0_idx, mv0, ref_frame0_idx_2nd, mv0_2nd, 
										ref_frame1, ref_frame1_idx, mv1, ref_frame1_idx_2nd, mv1_2nd);

					setL0RefPOC(uiViewId,bIsDepth,x,y,ref_frame0);
					setL0MvX(uiViewId,bIsDepth,x,y,mv0.getHor());
					setL0MvY(uiViewId,bIsDepth,x,y,mv0.getVer());

					setL1RefPOC(uiViewId,bIsDepth,x,y,ref_frame1);
					setL1MvX(uiViewId,bIsDepth,x,y,mv1.getHor());
					setL1MvY(uiViewId,bIsDepth,x,y,mv1.getVer());
				}
			}
		}
	}
}
#endif
#endif

#if POZNAN_DBMP_CALC_PRED_DATA
Void TComMP::calcDBMPPredData(TComDataCU* pcCU, UInt uiAbsPartIdx, Int &ref_frame0_idx,  TComMv &mv0, Int &ref_frame1_idx,  TComMv &mv1)
{	
	if(!isDBMPEnabled() || pcCU->getMergeIndex(uiAbsPartIdx)!=POZNAN_DBMP_MRG_CAND) return;

	Int iWidth,iHeight;
	Int x,y,iCUBaseX,iCUBaseY;

	switch(pcCU->getPartitionSize( uiAbsPartIdx ))
	{
		case SIZE_2Nx2N:
			iWidth = pcCU->getWidth(uiAbsPartIdx);
			iHeight = pcCU->getHeight(uiAbsPartIdx);			
			break;
		case SIZE_2NxN:
			iWidth = pcCU->getWidth(uiAbsPartIdx);
			iHeight = pcCU->getHeight(uiAbsPartIdx) >> 1;
			break;
		case SIZE_Nx2N:
			iWidth = pcCU->getWidth(uiAbsPartIdx) >> 1;
			iHeight = pcCU->getHeight(uiAbsPartIdx);
			break;
		case SIZE_NxN:
			iWidth = pcCU->getWidth(uiAbsPartIdx) >> 1;
			iHeight = pcCU->getHeight(uiAbsPartIdx) >> 1;
			break;
		default: assert(0);
	}	

	Int	ref_frame0, ref_frame1;
		
	Int ref_frame0_idx_2nd = pcCU->getCUMvField2nd(REF_PIC_LIST_0)->getRefIdx(uiAbsPartIdx);
	TComMv mv0_2nd = pcCU->getCUMvField2nd( REF_PIC_LIST_0 )->getMv( uiAbsPartIdx );

	Int ref_frame1_idx_2nd = pcCU->getCUMvField2nd(REF_PIC_LIST_1)->getRefIdx(uiAbsPartIdx);
	TComMv mv1_2nd = pcCU->getCUMvField2nd( REF_PIC_LIST_1 )->getMv( uiAbsPartIdx );

	iCUBaseX = pcCU->getCUPelX()+g_auiRasterToPelX[ g_auiZscanToRaster[uiAbsPartIdx] ];
	iCUBaseY = pcCU->getCUPelY()+g_auiRasterToPelY[ g_auiZscanToRaster[uiAbsPartIdx] ];

	UInt	  uiPointCnt = 0;
			
	for( y = iCUBaseY; y < iCUBaseY+iHeight; y++)
		for( x = iCUBaseX; x < iCUBaseX+iWidth; x++)
		{
			getDBMPPredData(pcCU, x, y, ref_frame0, ref_frame0_idx, mv0, ref_frame0_idx_2nd, mv0_2nd, 
								ref_frame1, ref_frame1_idx, mv1, ref_frame1_idx_2nd, mv1_2nd);

			m_piTempL0RefIdx[uiPointCnt] = ref_frame0_idx;
			m_piTempL0MvX[uiPointCnt] = mv0.getHor();
			m_piTempL0MvY[uiPointCnt] = mv0.getVer();

			m_piTempL1RefIdx[uiPointCnt] = ref_frame1_idx;
			m_piTempL1MvX[uiPointCnt] = mv1.getHor();
			m_piTempL1MvY[uiPointCnt] = mv1.getVer();			
	
			uiPointCnt++;
		}

	xCalcDBMPPredData(uiPointCnt, ref_frame0_idx, mv0, ref_frame1_idx, mv1);
}

Void TComMP::setDBMPPredMVField(RefPicList eRefListIdx, TComDataCU* pcCU)
{
	apcDBMPPredMVField[eRefListIdx]->copyFrom(pcCU->getCUMvField(eRefListIdx), pcCU->getTotalNumPart(), 0);
}

Void TComMP::setDBMPPredMVField(RefPicList eRefListIdx, TComDataCU* pcCU, Int iNumPartSrc, Int iPartAddrDst)
{
	apcDBMPPredMVField[eRefListIdx]->copyFrom(pcCU->getCUMvField(eRefListIdx), iNumPartSrc, iPartAddrDst);
}
#endif

#if POZNAN_DBMP_CALC_PRED_DATA
Void TComMP::xCalcDBMPPredData(UInt uiCnt, Int &ref_frame0_idx, TComMv &mv0, Int &ref_frame1_idx, TComMv &mv1)
{	
	if(uiCnt==0) return;

	Int iVal;
	UInt i,uiCntMax,uiCntTemp;

	//for(i=0;i<uiCnt;i++) m_piResBuff[i] = (Int)m_piTempL0RefIdx[i]; iVal = GetRepresentativeVal(uiCntMax, m_piResBuff, uiCnt, m_piTempBuff);	ref_frame0_idx = iVal;
	//for(i=0;i<uiCnt;i++) m_piResBuff[i] = (Int)m_piTempL0MvX[i];	  iVal = GetRepresentativeVal(uiCntMax, m_piResBuff, uiCnt, m_piTempBuff);	mv0.setHor(iVal);
	//for(i=0;i<uiCnt;i++) m_piResBuff[i] = (Int)m_piTempL0MvY[i];	  iVal = GetRepresentativeVal(uiCntMax, m_piResBuff, uiCnt, m_piTempBuff);	mv0.setVer(iVal);
	//for(i=0;i<uiCnt;i++) m_piResBuff[i] = (Int)m_piTempL1RefIdx[i]; iVal = GetRepresentativeVal(uiCntMax, m_piResBuff, uiCnt, m_piTempBuff);	ref_frame1_idx = iVal;
	//for(i=0;i<uiCnt;i++) m_piResBuff[i] = (Int)m_piTempL1MvX[i];	  iVal = GetRepresentativeVal(uiCntMax, m_piResBuff, uiCnt, m_piTempBuff);	mv1.setHor(iVal);
	//for(i=0;i<uiCnt;i++) m_piResBuff[i] = (Int)m_piTempL1MvY[i];	  iVal = GetRepresentativeVal(uiCntMax, m_piResBuff, uiCnt, m_piTempBuff);	mv1.setVer(iVal);
	
	//iVal = GetRepresentativeVal(uiCntMax, m_piTempL0RefIdx, uiCnt, m_piTempBuff);	ref_frame0_idx = iVal;
	//iVal = GetRepresentativeVal(uiCntMax, m_piTempL0MvX, uiCnt, m_piTempBuff);		mv0.setHor(iVal);
	//iVal = GetRepresentativeVal(uiCntMax, m_piTempL0MvY, uiCnt, m_piTempBuff);		mv0.setVer(iVal);
	//iVal = GetRepresentativeVal(uiCntMax, m_piTempL1RefIdx, uiCnt, m_piTempBuff);	ref_frame1_idx = iVal;
	//iVal = GetRepresentativeVal(uiCntMax, m_piTempL1MvX, uiCnt, m_piTempBuff);		mv1.setHor(iVal);
	//iVal = GetRepresentativeVal(uiCntMax, m_piTempL1MvY, uiCnt, m_piTempBuff);		mv1.setVer(iVal);

	iVal = GetRepresentativeVal(uiCntMax, m_piTempL0RefIdx, uiCnt, m_piTempBuff);	ref_frame0_idx = iVal;
	//if(ref_frame0_idx==-1 && (uiCntMax<<1)==uiCnt) 
	//{
	//	iVal = GetRepresentativeVal(uiCntMax, m_piTempL0RefIdx, uiCnt, m_piTempBuff, true, -1);	
	//	if((uiCntMax<<1)==uiCnt) ref_frame0_idx = iVal;
	//}
	//if((uiCntMax<<1)==uiCnt) 
	//{
	//	iVal = GetRepresentativeVal(uiCntMax, m_piTempL0RefIdx, uiCnt, m_piTempBuff, true, ref_frame0_idx);	
	//	if((uiCntMax<<1)==uiCnt && (ref_frame0_idx==-1 || (iVal!=-1 && iVal<ref_frame0_idx))) ref_frame0_idx = iVal;
	//}
	if((uiCntMax<<1)<=uiCnt)
	{
		iVal = GetRepresentativeVal(uiCntTemp, m_piTempL0RefIdx, uiCnt, m_piTempBuff, true, ref_frame0_idx);
		if(uiCntTemp==uiCntMax && (ref_frame0_idx==-1 || (iVal!=-1 && iVal<ref_frame0_idx))) ref_frame0_idx = iVal;
	}

	iVal = GetRepresentativeVal(uiCntMax, m_piTempL1RefIdx, uiCnt, m_piTempBuff);	ref_frame1_idx = iVal;
	//if(ref_frame1_idx==-1 && (uiCntMax<<1)==uiCnt) 
	//{
	//	iVal = GetRepresentativeVal(uiCntMax, m_piTempL1RefIdx, uiCnt, m_piTempBuff, true, -1);	
	//	if((uiCntMax<<1)==uiCnt) ref_frame1_idx = iVal;
	//}
	//if((uiCntMax<<1)==uiCnt) 
	//{
	//	iVal = GetRepresentativeVal(uiCntMax, m_piTempL1RefIdx, uiCnt, m_piTempBuff, true, ref_frame1_idx);	
	//	if((uiCntMax<<1)==uiCnt && (ref_frame1_idx==-1 || (iVal!=-1 && iVal<ref_frame1_idx))) ref_frame1_idx = iVal;
	//}
	if((uiCntMax<<1)<=uiCnt)
	{
		iVal = GetRepresentativeVal(uiCntTemp, m_piTempL1RefIdx, uiCnt, m_piTempBuff, true, ref_frame1_idx);
		if(uiCntTemp==uiCntMax && (ref_frame1_idx==-1 || (iVal!=-1 && iVal<ref_frame1_idx))) ref_frame1_idx = iVal;
	}

	if(ref_frame0_idx==-1 && ref_frame1_idx==-1)
	{
		iVal = GetRepresentativeVal(uiCntMax, m_piTempL0RefIdx, uiCnt, m_piTempBuff, true, -1);	ref_frame0_idx = iVal;
		iVal = GetRepresentativeVal(uiCntMax, m_piTempL1RefIdx, uiCnt, m_piTempBuff, true, -1);	ref_frame1_idx = iVal;
	}

	if(ref_frame0_idx==-1) 
	{
		mv0.setHor(0);
		mv0.setVer(0);
	}
	else
	{
		uiCntTemp=0; for(i=0;i<uiCnt;i++) if(m_piTempL0RefIdx[i]==ref_frame0_idx) m_piResBuff[uiCntTemp++] = (Int)m_piTempL0MvX[i]; 
		iVal = GetRepresentativeVal(uiCntMax, m_piResBuff, uiCntTemp, m_piTempBuff);	mv0.setHor(iVal);

		uiCntTemp=0; for(i=0;i<uiCnt;i++) if(m_piTempL0RefIdx[i]==ref_frame0_idx) m_piResBuff[uiCntTemp++] = (Int)m_piTempL0MvY[i];
		iVal = GetRepresentativeVal(uiCntMax, m_piResBuff, uiCntTemp, m_piTempBuff);	mv0.setVer(iVal);
	}	
	
	if(ref_frame1_idx==-1) 
	{
		mv1.setHor(0);
		mv1.setVer(0);
	}
	else
	{
		uiCntTemp=0; for(i=0;i<uiCnt;i++) if(m_piTempL1RefIdx[i]==ref_frame1_idx) m_piResBuff[uiCntTemp++] = (Int)m_piTempL1MvX[i];	 
		iVal = GetRepresentativeVal(uiCntMax, m_piResBuff, uiCntTemp, m_piTempBuff);	mv1.setHor(iVal);

		uiCntTemp=0; for(i=0;i<uiCnt;i++) if(m_piTempL1RefIdx[i]==ref_frame1_idx) m_piResBuff[uiCntTemp++] = (Int)m_piTempL1MvY[i];	 
		iVal = GetRepresentativeVal(uiCntMax, m_piResBuff, uiCntTemp, m_piTempBuff);	mv1.setVer(iVal);
	}
}
#endif




#endif	// POZNAN_MP