/*
 * @(#)chnstub.c	1.4 97/12/22

        Contains:       KCMS PT chaining function stubs

        Written by:     Drivin' Team

        Copyright:      (c) 1994-1995 by Eastman Kodak Company, Inc., all rights reserved.

        Change History (most recent first):

                 <1>      6/3/94       sek	first checked in
*/

#include "kcmsos.h"
#include "kcmptlib.h"
#include "kcptmgr.h"

PTErr_t PTChain(PTRefNum_t PTRefNum){
	if (PTRefNum){}
	return (KCP_SYSERR_1);
}

PTErr_t PTChainInit
		(int32 nPT, PTRefNum_t *PTList, int32 validate, int32 *index){
	if (nPT){}
	if (PTList){}
	if (validate){}
	if (index){}
	return (KCP_SYSERR_1);
}

PTErr_t PTChainInitM
		(int32 nPT, PTRefNum_t *PTList, int32 compMode, int32 rulesKey){
	if (nPT){}
	if (PTList){}
	if (compMode){}
	if (rulesKey){}
	return (KCP_SYSERR_1);
}

PTErr_t PTChainEnd (PTRefNum_t *PTRefNum){
	if (PTRefNum){}
	return (KCP_SYSERR_1);
}

PTErr_t PTChainValidate (int32 nPT, PTRefNum_t* PTList, int32* index){
	if (nPT){}
	if (PTList){}
	if (index){}
	return (KCP_SYSERR_1);
}

PTErr_t
	getResizeAuxPT(	threadGlobals_p	threadGlobalsP,
					PTRefNum_t		PTRefNum,
					PTRefNum_p		auxPTRefNum)
{
	if (threadGlobalsP){}
	if (PTRefNum){}
	if (auxPTRefNum){}
	return (KCP_SYSERR_1);
}

