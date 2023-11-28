#ifndef _SERIALNUM_H
#define _SERIALNUM_H


/*	SerialNum.h
/	©1997 StarCode Software
*/

uint	modpow(uint, uint, uint = INT_MAX);
void	SNgenerate(char *digits, char *serial);
void	SNgencheck(char *digits, char *output);
bool	SNcheck(char *serial);

#endif
