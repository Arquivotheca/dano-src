/*
 * @(#)ctelib.h	2.14 97/12/22

	Contains:       header file for NuBus Function Engine processor evaluation routines

	Written by:     Pawle's Team

	Copyright:      (c) 1991-1994 by Eastman Kodak Company, all rights reserved.

	Change History (most recent first):

				12/7/93		gbp		change PT_eval_cte() to PT_eval_cteDT()
		 <7>    10/18/91        msm             Add KCP_CTE_GRID_TOO_BIG.
		 <6>      8/8/91        gbp             remove A5 arguments stuff
		 <5>      8/2/91        gbp             change PTData to be a handle for
									the new fut library
		 <4>     7/18/91        gbp             add PTProcessorReset_nfe, openKCME1
									prototypes
		 <3>     5/20/91        msm             add PTEvaluators, PTCancel,
									& PTProcessor Reset
 */

#ifndef _CTELIB_H_
#define _CTELIB_H_ 1

#if defined (KPMAC)
#include <Components.h>
#endif
#include "kcptmgr.h"

#define KCP_CTE_NOT_ATTEMPTED 98	/* Did not try ... maybe cmyk to cmyk */
#define KCP_CTE_GRID_TOO_BIG 99		/* This PT did not all fit at once (faster in software!) */

PTErr_t PTEvaluators_cte (int16 * nMCTE);

PTErr_t PTEvalCancel_cte (void);

PTErr_t PTProcessorReset_cte (void);

PTErr_t PTIcmGridSize(KpInt32_t *size_of_grid_tbl);

PTErr_t PTColorSpace_cte(KpInt32_t color_space);

PTErr_t PT_eval_cteDT (threadGlobals_p threadGlobalsP, 
				KcmHandle PTData, PTRefNum_t PTRefNum, PTEvalDTPB_t* evalDef,
				int32 Unit, PTProgress_t progress, int32 aSync);

PTErr_t PTTerminate_cte (void);

#if defined (KPMAC)
PTErr_t cte_proc_send(int32 command, PTAddr_t parmList);
#else
PTErr_t FAR PASCAL cte_proc_send(int command, PTAddr_t parmList);
typedef int (FAR PASCAL *KcpCteFunc)(int command, PTAddr_t parm);
#endif

#endif

ÿ