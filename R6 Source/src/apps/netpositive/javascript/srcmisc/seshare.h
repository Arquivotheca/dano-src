/* seshare.h  Copyright 1998 Nombas, Inc.  All rights reserved
 *
 * Contains the prototypes for implemented shared data.  This may be
 * replaced by API calls in the future.
 */

/* (c) COPYRIGHT 1993-98           NOMBAS, INC.
 *                                 64 SALEM ST.
 *                                 MEDFORD, MA 02155  USA
 * 
 * ALL RIGHTS RESERVED
 * 
 * This software is the property of Nombas, Inc. and is furnished under
 * license by Nombas, Inc.; this software may be used only in accordance
 * with the terms of said license.  This copyright notice may not be removed,
 * modified or obliterated without the prior written permission of Nombas, Inc.
 * 
 * This software is a Trade Secret of Nombas, Inc.
 * 
 * This software may not be copied, transmitted, provided to or otherwise made
 * available to any other person, company, corporation or other entity except
 * as specified in the terms of said license.
 * 
 * No right, title, ownership or other interest in the software is hereby
 * granted or transferred.
 * 
 * The information contained herein is subject to change without notice and
 * should not be construed as a commitment by Nombas, Inc.
 */

typedef void (JSE_CFUNC FAR_CALL *jseShareCleanupFunc)(jseContext jsecontext,void _FAR_ *data);

void _FAR_ * jseGetSharedData(jseContext jsecontext, const jsechar *name);
void jseSetSharedData(jseContext jsecontext, const jsechar *name,
                      void _FAR_ * data,jseShareCleanupFunc cleanupFunc);
