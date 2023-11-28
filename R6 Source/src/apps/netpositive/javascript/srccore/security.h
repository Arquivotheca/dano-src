/* security.h - Secure-jse
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

#if !defined(_SECURITY_H) && defined(JSE_SECUREJSE) && (0!=JSE_SECUREJSE)
#define _SECURITY_H

struct Security
{
   VarRead *InitFuncVar,*GuardFuncVar,*TermFuncVar;
   struct Function *InitFunc, *GuardFunc, *TermFunc;
   VarRead *PrivateVariable; /* initialized to NULL to indicate failure to
                                construct */
   jsechar *NotYetInitializedTextOrCode;
   jsebool SecurityHasBeenEnabled;
   jsebool InSecurityCall;  /* set true when calling a security function */
};

struct Security * NEAR_CALL securityNew(const jsechar * jseSecureCode);
   /* security text is remembered as the security code; NULL if no security */
void NEAR_CALL securityDelete(struct Security *);
jsebool NEAR_CALL securityInit(struct Security *,struct Call *call);
   /* will not re-initialize if already initialized */
void NEAR_CALL securityTerm(struct Security *,struct Call *call);
jsebool NEAR_CALL securityGuard(struct Security *,struct Call *call,
                                uint InputVariableCount,VarRead *ThisVar);
   /* True if OK, else false and errors will have been set;
    * FunctionName may be NULL for security init and security term
    */
void NEAR_CALL securityTell(struct Security *,struct Call *call,
                            VarRead *InfoVar);
   /* pass InfoVar to security init */
jsebool NEAR_CALL securityCallInitOrTerm(struct Security *,struct Call *call,
                                         struct Function *InitOrTermFunc,
                                         VarRead *FuncVar,VarRead *var);


struct Function * NEAR_CALL FindFunction(struct Call *call,
                                         const jsechar *FunctionName,
                                         VarRead **func);

#define securityEnabled(this) ((this)->SecurityHasBeenEnabled)

#endif
