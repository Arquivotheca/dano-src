#include <GL/gl.h>

void * asm_Jcc( void * pc, void * target, GLuint condition )
{
	GLubyte *b = (GLubyte *)pc;
	GLint offset = ((GLubyte *)target) - b;
	*b++ = 0x0f;
	*b++ = condition;
	*((GLint *)b) = offset -6;
	b += 4;
	return b;
}


void * asm_Call( void * pc, void *target )
{
	GLubyte *b = (GLubyte *)pc;
	GLint offset = ((GLubyte *)target) - b;
	*b++ = 0xE8;
	*((GLint *)b) = offset -5;
	b += 4;
	return b;
}


void * asm_Jmp( void * pc, void * target )
{
	GLubyte *b = (GLubyte *)pc;
	GLint offset = ((GLubyte *)target) - b;
	*b++ = 0xE9;
	*((GLint *)b) = offset -5;
	b += 4;
	return b;
}


void * asm_CopyBlock( void * pc, void *start, void *end )
{
	GLint len = ((GLubyte *)end) - ((GLubyte *)start);
	memcpy( pc, start, len );
	return ((GLubyte *)pc) + len;
}

void * asm_Align( void * pc )
{
	GLuint pcb = (GLuint)pc;
	GLint padding;
	static GLubyte a1[] = { 0x90 };
	static GLubyte a2[] = { 0x87, 0xf6 };
	static GLubyte a3[] = { 0x8d, 0x76, 0x00 };
	static GLubyte a4[] = { 0x89, 0xc0, 0x87, 0xf6 };
	static GLubyte a5[] = { 0x89, 0xc0, 0x8d, 0x76, 0x00 };
	static GLubyte a6[] = { 0x8d, 0xb6, 0x00, 0x00, 0x00, 0x00 };
	static GLubyte a7[] = { 0x8d, 0x80, 0x00, 0x00, 0x00, 0x00, 0x90 };
	static GLubyte a8[] = { 0x8d, 0x80, 0x00, 0x00, 0x00, 0x00, 0x87, 0xf6 };
	static GLubyte a9[] = { 0x8d, 0x80, 0x00, 0x00, 0x00, 0x00, 0x8d, 0x76, 0x00 };
	static GLubyte a10[] ={ 0x8d, 0x9b, 0x00, 0x00, 0x00, 0x00, 0x89, 0xc0, 0x87, 0xf6 };
	static GLubyte a11[] ={ 0x8d, 0x9b, 0x00, 0x00, 0x00, 0x00, 0x89, 0xc0, 0x8d, 0x76, 0x00 };
	static GLubyte a12[] ={ 0x8d, 0x9b, 0x00, 0x00, 0x00, 0x00, 0x8d, 0x80, 0x00, 0x00, 0x00, 0x00 };
	static GLubyte a13[] ={ 0x8d, 0x9b, 0x00, 0x00, 0x00, 0x00, 0x90, 0x8d, 0xb6, 0x00, 0x00, 0x00, 0x00 };
	static GLubyte a14[] ={ 0x8d, 0x9b, 0x00, 0x00, 0x00, 0x00, 0x89, 0xc0, 0x8d, 0xb6, 0x00, 0x00, 0x00, 0x00 };
	static GLubyte a15[] ={ 0x8d, 0x9b, 0x00, 0x00, 0x00, 0x00, 0x8d, 0x40, 0x00, 0x8d, 0xb6, 0x00, 0x00, 0x00, 0x00 };
	
	padding = 0x10 - (pcb & 0x0f);
	padding &= 0x0f;
	switch ( padding )
	{
		case 0: return pc;
		case 1: memcpy( pc, a1, 1 ); return ((GLubyte *)pc)+1;
		case 2: memcpy( pc, a2, 2 ); return ((GLubyte *)pc)+2;
		case 3: memcpy( pc, a3, 3 ); return ((GLubyte *)pc)+3;
		case 4: memcpy( pc, a4, 4 ); return ((GLubyte *)pc)+4;
		case 5: memcpy( pc, a5, 5 ); return ((GLubyte *)pc)+5;
		case 6: memcpy( pc, a6, 6 ); return ((GLubyte *)pc)+6;
		case 7: memcpy( pc, a7, 7 ); return ((GLubyte *)pc)+7;
		case 8: memcpy( pc, a8, 8 ); return ((GLubyte *)pc)+8;
		case 9: memcpy( pc, a9, 9 ); return ((GLubyte *)pc)+9;
		case 10: memcpy( pc, a10, 10 ); return ((GLubyte *)pc)+10;
		case 11: memcpy( pc, a11, 11 ); return ((GLubyte *)pc)+11;
		case 12: memcpy( pc, a12, 12 ); return ((GLubyte *)pc)+12;
		case 13: memcpy( pc, a13, 13 ); return ((GLubyte *)pc)+13;
		case 14: memcpy( pc, a14, 14 ); return ((GLubyte *)pc)+14;
		case 15: memcpy( pc, a15, 15 ); return ((GLubyte *)pc)+15;
	}
	return pc;	
}


