/* Define.h    Handle all the #define statements coming
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

#if !defined(_DEFINE_H) && defined(JSE_DEFINE) && (0!=JSE_DEFINE)
#define _DEFINE_H


struct Link
{
   struct Link *Prev;
   jsechar *Find;
   jsechar *Replace;
};


struct Define
{
   struct Link *RecentLink;
   struct Define *Parent;
};


struct Define * defineNew(struct Define *Parent);
void defineDelete(struct Define *);
void defineAdd(struct Define *def,const jsechar *FindString,
               const jsechar *ReplaceString);
void defineAddInt(struct Define *def,const jsechar *FindString,slong l);
#if defined(JSE_FLOATING_POINT) && (0!=JSE_FLOATING_POINT)
   void defineAddFloat(struct Define *def,const jsechar *FindString,jsenumber f);
#endif
void defineAddLen(struct Define *def,const jsechar *FindString,int FindStringLen,
                  const jsechar *ReplaceString,int ReplaceStringLen);
const jsechar * defineFindReplacement(struct Define *def,const jsechar *FindString);
   /* search here or in parents */
jsebool defineProcessSourceStatement(struct Source **source,struct Call *call);
   /* if error then print error and return False, else return True */

#endif
