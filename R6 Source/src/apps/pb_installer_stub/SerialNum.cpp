#include <stdio.h>
#include <string.h>
#include "SerialNum.h"

uint modpow(uint base, uint exp, uint mod)
{
	if (exp == 0)
		return 1;
	else {
		uint half = modpow(base,exp/2,mod);
		half = half*half;
		if (exp % 2) half = half*base;
		return half % mod;
	}
}

/* generate check string */
void	SNgencheck(char *digits, char *output)
{
	int len = strlen(digits);
	
	uint base, col, exp;
	base = 1011;		// large base
	col = 1;
	for (int i = len/2-1; i >= 0; i--) {
		base += (digits[i] - '0') * col;
		col *= 10;
	}
	exp = 3;			// minimum exponent
	col = 1;
	for (int i = len-1; i >= len/2; i--) {
		exp += (digits[i] - '0') * col;
		col *= 10;
	}	
	uint check, modulus;
	modulus = 99999997;		// size dependency!
	check = modpow(base,exp,modulus);

	sprintf(output,"%08d",check);	// size dependency
}

/* generate full serial number */
void	SNgenerate(char *digits, char *serial)
{
	char	cdigits[9];
	SNgencheck(digits,cdigits);
	
	// even check digits
	for (int i = 0; i < 8; i++)
		serial[i*2] = cdigits[i];	
	// odd original digits
	for (int i = 0; i < 8; i++)
		serial[(i*2)+1] = digits[i];
		
	serial[16] = 0;
}

bool	SNcheck(char *serial)
{
	if (strlen(serial) != 16)
		return false;
		
	char digits[9];
	char cdigits[9];
		
	// odd originals
	for (int i = 0; i < 8; i++)
		digits[i] = serial[(i*2)+1];
	digits[8] = 0;
			
	SNgencheck(digits,cdigits);
		
	bool valid = true;
	// even check digits
	for (int i = 0; i < 8 && valid; i++) {
		if (cdigits[i] != serial[i*2]) {
			valid = false;
			break;
		}
	}
	return valid;
}
