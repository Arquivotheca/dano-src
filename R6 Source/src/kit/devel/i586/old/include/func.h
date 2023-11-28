#ifndef __FUNC_H__
#define __FUNC_H__

		/* FILE: iel.c */
char IEL_mul(U128 *xr, U128 *y, U128 *z);
char IEL_rem(U128 *x, U128 *y, U128 *z);
char IEL_div(U128 *x, U128 *y, U128 *z);
IEL_Err IEL_U128tostr(const U128 *x, char *strptr, long base, const long length);
IEL_Err IEL_U64tostr(const U64 *x, char *strptr, long base, const long length);
IEL_Err IEL_S128tostr(const S128 *x, char *strptr, long base,const long length);
IEL_Err IEL_S64tostr(const S64 *x, char *strptr, long base,const long length);
int IEL_strtoU128( char *str1, char **endptr, long base, U128 *x);
int IEL_strtoU64(char *str1, char **endptr, long base, U64 *x);
int IEL_strtoS128(char *str1, char **endptr, long base, S128 *x);
int IEL_strtoS64(char *str1, char **endptr, long base, S64 *x);
int IEL_c0(void *x, int sx);
int IEL_c1(void *x, int sx);
int IEL_c2(void *x, int sx);
int IEL_c3(void *x, int sx);
int IEL_au(void *x, void *y, int sx, int sy);
IEL_Err IEL_as(void *x, void *y, int sx, int sy);

#endif /* __FUNC_H__ */
