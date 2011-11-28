

/** \file     TComCABACTables.h
    \brief    static class for CABAC tables
*/

#ifndef __TCOM_CABAC_TABLES__
#define __TCOM_CABAC_TABLES__

#include "../TLibCommon/CommonDef.h"

/**
 * \brief static class for CABAC tables
 */

class TComCABACTables
{
public:
  const static UChar  sm_aucLPSTable[64][4];
};


#endif

