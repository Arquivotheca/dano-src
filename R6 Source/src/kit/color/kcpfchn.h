/*
 * @(#)kcpfchn.h	2.26 97/12/22

	Contains:	header file for fut chaining in KCM driver

	Written by:	Drivin' Team

	Copyright:	(c) 1991-1997 by Eastman Kodak Company, all rights reserved.

	Change History (most recent first):

		 <9>	 11/5/91	gbp		add chkPropRule07() prototype
		 <8>	10/31/91	gbp		add ComposeAttrFut() and moveAttrList()
		 <7>	  9/9/91	gbp		remove ptutild.c
		 <6>	  9/5/91	gbp		add ComposeAttr prototype; use ARGS()
		 <5>	 8/28/91	gbp		add PTutilD.c
		 <4>	 8/27/91	gbp		add KCPChainSetup prototype; remove KCMFchain.c
		 <3>	  8/2/91	gbp		include stuff from comp driver
		 <2>	 6/24/91	gbp		change main functions to use actual args instead of arg
									list pointer; make rules arrays instead of pointers to pointers
		 <1>	 6/14/91	gbp		first checked in
*/

#ifndef _KCMFCHAIN_H_
#define _KCMFCHAIN_H_ 1

#include "fut.h"
#include "fut_util.h"
#include "fut_io.h"

#if defined (KCP_COMP_2)
	#include "auxpt.h"
#endif

/* constants */
#define MAX_COMP_ATTR_SIZE 4	/* maximum # of characters in compose attribute */
#define RULE_PRE_COMP '+'		/* rule control to indicate Trojan Horse PT */
#define RULE_POST_COMP '_'		/* rule control to indicate Trojan Horse PT */
#define RULE_SERIAL_EVAL 'S'	/* rule control to indicate Serial Evaluation */
#define PT_SERIAL_EVAL -2		/* flags PT for Serial Evaluation */

/* the dimensions of the futs */ 
#define	TWO_CUBE	2			/* 2 cube min. */
#define SIXTEEN_CUBE	16

/* number of input variables */
#define	THREE_CLR	3			/* 3 input colors */
#define	FOUR_CLR	4			/* 4 input colors */

/* prototypes */
PTErr_t doChainInit ARGS((threadGlobals_p threadGlobalsP,
						int32 nPT, PTRefNum_p PTList, int32 compMode, int32 rulesKey));
PTErr_t doChainEnd ARGS((threadGlobals_p threadGlobalsP,
						PTRefNum_p PTRefNum, int32 compMode, int32 chainKey));
PTErr_t doChain ARGS((threadGlobals_p threadGlobalsP, PTRefNum_t, int32));
void clearChain ARGS((threadGlobals_p threadGlobalsP));
PTErr_t ComposeAttr ARGS((threadGlobals_p threadGlobalsP,
							PTRefNum_t, PTRefNum_t, int32, PTRefNum_t));
PTErr_t ComposeAttrFut ARGS((threadGlobals_p threadGlobalsP,
							PTRefNum_t, PTRefNum_t, PTRefNum_t));
PTErr_t moveAttrList ARGS((threadGlobals_p threadGlobalsP,
							PTRefNum_t, PTRefNum_t, int32_p, PTRefNum_t));
PTErr_t chkPropRule07 ARGS((int32, PTRefNum_t, PTRefNum_t, char_p));
fut_ptr_t get_lin3d ARGS((int));
fut_ptr_t get_lin4d ARGS((int));
fut_ptr_t get_linlab ARGS((int));
fut_ptr_t get_lab2xyz ARGS((int));
fut_ptr_t get_xyz2lab ARGS((int));
void strcatNMA ARGS((const char *s1, const char *s2, char *s3));
void makeTempFileName (KpChar_p path, KpChar_p tempName);


/* routines in ptutild.c */
PTErr_t PTLoadD ARGS((char *PTName, PTRefNum_t *PTRefNum_p));
PTErr_t PTUnloadD ARGS((PTRefNum_t PTrefnum));
PTErr_t PTStoreD ARGS((PTRefNum_t PTRefNum, char *PTName));

#if defined (KCP_COMP_2)

/* routines in auxpt.c */
typedef double (*iFunc_p)(double) ;
typedef fut_gtbldat_t (*gFunc_p)(double *);
typedef fut_otbldat_t (*oFunc_p)(fut_gtbldat_t);

PTErr_t loadAuxPT(const char_p PTName, kcpindex_t invert, int32 compMode, PTRefNum_p PTRefNum_p);
PTErr_t getAuxBuild (int32 compMode, char_p PTName, auxPTBuild_p auxBuildPtr);
PTErr_t getMaxGridDim (int32 compMode, int16_p maxDimPtr);
PTErr_t getAuxName (auxPTBuild_pc auxBuild_Ptr, int32 compMode, char_p PTName);
PTErr_t copyAuxBuild (auxPTBuild_pc auxBuild1_Ptr, int32 compMode, auxPTBuild_p auxBuild2_Ptr);
iFunc_p getAuxIFunc (auxBuildFuncs_e func); 
gFunc_p getAuxGFunc (auxBuildFuncs_e func); 
oFunc_p getAuxOFunc (auxBuildFuncs_e func); 

#endif /* KCP_COMP_2 */

#endif	/* _KCMFCHAIN_H_ */

