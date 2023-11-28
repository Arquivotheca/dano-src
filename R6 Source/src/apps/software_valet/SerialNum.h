#ifndef _SERIALNUM_H_
#define _SERIALNUM_H_

/*	SerialNum.h
/	©1997 StarCode Software
*/

#include <SupportDefs.h>

uint	modpow(uint, uint, uint = INT_MAX);
void	SNgenerate(char *digits, char *serial);
void	SNgencheck(char *digits, char *output);
bool	SNcheck(char *serial);

#endif
