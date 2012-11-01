#ifndef __DEBUGLOG_H__
#define __DEBUGLOG_H__

#define OUT_PRED_INFO  0  //pred info(IntraDir/InterDirNum,Idx,Mv) output
#define ASYMMETRIC_USE 1

#include "TComDataCU.h"

#define DECLOGFILENAME  "debuglogDec.txt"

class DebugLog 
{
private:
    FILE*  m_pfDebugLogFile;

public:
    DebugLog() { m_pfDebugLogFile = NULL; }
    ~DebugLog() {}

    bool   DebugLogFileOpen( char* pcFname ) { m_pfDebugLogFile = fopen( pcFname, "wt" ); return (m_pfDebugLogFile!=NULL); }
    void   DebugLogFileClose()               { if (m_pfDebugLogFile) fclose(m_pfDebugLogFile); m_pfDebugLogFile=NULL; }

    bool   DebugLogOut( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth );
};


//Colors
//     Y=Org/2 + YOffset      YOffset  U    V        
const UChar g_ucCol[11][3] = { {000, 128, 128}, //0 gray scale
                               {128, 255, 000}, //1 light blue
                               {128, 255, 128}, //2 lavender
                               {128, 000, 000}, //3 green yellow
                               {128, 000, 128}, //4 yellow
                               {128, 255, 255}, //5 pink
                               {128, 000, 255}, //6 orange
                               {064, 000, 255}, //7 red 
                               {064, 255, 000}, //8 blue
                               {016, 000, 000}, //9 green
                               {064, 128, 128}  //10 light gray scale
                             };

//CU
#define LINE_LUMA_OFF_COL   g_ucCol[9][0] //9 green
#define LINE_CB_COL         g_ucCol[9][1]
#define LINE_CR_COL         g_ucCol[9][2]

//PU
#define LINE_LUMA_OFF_COL2  g_ucCol[7][0] //7 red
#define LINE_CB_COL2        g_ucCol[7][1]
#define LINE_CR_COL2        g_ucCol[7][2]


#endif //__DEBUGLOG_H__
