#include <string.h>
#define __GL_USE_FASTCALL__ 1
#include <GL/gl.h>
#include "context.h"
#include "global.h"
#include "list.h"
#include "immed.h"
#include "g_listop.h"
#include "g_lcomp.h"
#include "listcomp.h"

void __gllc_CallList( __glContext *gc, GLuint list)
{
    __glListMachine *lstate = &gc->list;
    GLbyte *PC=0;

	if (lstate->mode == GL_COMPILE_AND_EXECUTE) 
	   __glim_CallList(gc, list);
	__GL_BEGIN(__glop_CallList);
	__GL_PUT_LONG(list);
	__GL_END(8);
	__GL_CHECK_SPACE(80);
}

void __gllc_CallLists( __glContext *gc, GLsizei n, GLenum type, const GLvoid *lists)
{
    __glListMachine *lstate = &gc->list;
    GLbyte *PC=0;
    GLint _cmdlen=0, _count0=0;

	if (lstate->mode == GL_COMPILE_AND_EXECUTE) 
	   __glim_CallLists(gc, n, type, lists);
	_count0 = __glCallLists_size(n,type);
	if (_count0 < 0) {
	    __gllc_Error(gc, -_count0);
	    return;
	}
	_cmdlen = 12+__GL_PAD(_count0*1);
	__GL_CHECK_SPACE(_cmdlen);
	__GL_BEGIN(__glop_CallLists);
	__GL_PUT_LONG(n);
	__GL_PUT_LONG(type);
	__GL_PUT_VOID_ARRAY(lists,_count0);
	__GL_END(_cmdlen);
	__GL_CHECK_SPACE(80);
}

void __gllc_ListBase( __glContext *gc, GLuint base)
{
    __glListMachine *lstate = &gc->list;
    GLbyte *PC=0;

	if (lstate->mode == GL_COMPILE_AND_EXECUTE) 
	   __glim_ListBase(gc, base);
	__GL_BEGIN(__glop_ListBase);
	__GL_PUT_LONG(base);
	__GL_END(8);
	__GL_CHECK_SPACE(80);
}

void __gllc_Begin( GLenum mode)
{
	__glContext *gc = 0;
    __glListMachine *lstate;
    GLbyte *PC=0;
	
	__asm__ __volatile__ ( "nop \n\t": "=c"(gc), "=d"(mode) );

	lstate = &gc->list;
	if (lstate->mode == GL_COMPILE_AND_EXECUTE) 
	   GLIM_BEGIN(gc, mode);
	__GL_BEGIN(__glop_Begin);
	__GL_PUT_LONG(mode);
	__GL_END(8);
	__GL_CHECK_SPACE(80);
}

void __gllc_Color3bv( __glContext *__gc, const GLbyte *vv)
{
	__glContext *gc = 0;
	GLbyte *v = 0;
	__asm__ __volatile__ ( "": "=c"(gc), "=d"(v) );
	{
	    __glListMachine *lstate = &gc->list;
	    GLbyte *PC=0;
		if (lstate->mode == GL_COMPILE_AND_EXECUTE) 
		   GLIM_COLOR3BV(gc, v);
		__GL_BEGIN(__glop_Color3bv);
		__GL_PUT_CHAR(v[0]);
		__GL_PUT_CHAR(v[1]);
		__GL_PUT_CHAR(v[2]);
		__GL_PUT_PAD(1);
		__GL_END(8);
		__GL_CHECK_SPACE(80);
	}
}

void __gllc_Color3dv( __glContext *__gc, const GLdouble *vv)
{
	__glContext *gc = 0;
	GLdouble *v = 0;
	__asm__ __volatile__ ( "": "=c"(gc), "=d"(v) );
	{
	    __glListMachine *lstate = &gc->list;
	    GLbyte *PC=0;
	
//		if (lstate->mode == GL_COMPILE_AND_EXECUTE)
//		   GLIM_COLOR3DV(gc, v);
		PC = lstate->pc;
		if ((((int) PC) & 0x7) == 0)
		{
		    __GL_BEGIN(__glop_Nop);
		    __GL_END(4);
		}
		__GL_BEGIN(__glop_Color3dv);
		__GL_PUT_DOUBLE(v[0]);
		__GL_PUT_DOUBLE(v[1]);
		__GL_PUT_DOUBLE(v[2]);
		__GL_END(28);
		__GL_CHECK_SPACE(80);
	}
}

void __gllc_Color3fv( __glContext *__gc, const GLfloat *vv)
{
	__glContext *gc = 0;
	GLfloat *v = 0;
	__asm__ __volatile__ ( "": "=c"(gc), "=d"(v) );
	{
	    __glListMachine *lstate = &gc->list;
	    GLbyte *PC=0;
	
		if (lstate->mode == GL_COMPILE_AND_EXECUTE) 
		   GLIM_COLOR3FV(gc, v);
		__GL_BEGIN(__glop_Color3fv);
		__GL_PUT_FLOAT(v[0]);
		__GL_PUT_FLOAT(v[1]);
		__GL_PUT_FLOAT(v[2]);
		__GL_END(16);
		__GL_CHECK_SPACE(80);
	}
}

void __gllc_Color3iv( __glContext *__gc, const GLint *vv)
{
	__glContext *gc = 0;
	GLint *v = 0;
	__asm__ __volatile__ ( "": "=c"(gc), "=d"(v) );
	{
	    __glListMachine *lstate = &gc->list;
	    GLbyte *PC=0;
	
		if (lstate->mode == GL_COMPILE_AND_EXECUTE) 
		   GLIM_COLOR3IV(gc, v);
		__GL_BEGIN(__glop_Color3iv);
		__GL_PUT_LONG(v[0]);
		__GL_PUT_LONG(v[1]);
		__GL_PUT_LONG(v[2]);
		__GL_END(16);
		__GL_CHECK_SPACE(80);
	}
}

void __gllc_Color3sv( __glContext *__gc, const GLshort *vv)
{
	__glContext *gc = 0;
	GLshort *v = 0;
	__asm__ __volatile__ ( "": "=c"(gc), "=d"(v) );
	{
	    __glListMachine *lstate = &gc->list;
	    GLbyte *PC=0;
	
		if (lstate->mode == GL_COMPILE_AND_EXECUTE) 
		   GLIM_COLOR3SV(gc, v);
		__GL_BEGIN(__glop_Color3sv);
		__GL_PUT_SHORT(v[0]);
		__GL_PUT_SHORT(v[1]);
		__GL_PUT_SHORT(v[2]);
		__GL_PUT_PAD(2);
		__GL_END(12);
		__GL_CHECK_SPACE(80);
	}
}

void __gllc_Color3ubv( __glContext *__gc, const GLubyte *vv)
{
	__glContext *gc = 0;
	GLubyte *v = 0;
	__asm__ __volatile__ ( "": "=c"(gc), "=d"(v) );
	{
	    __glListMachine *lstate = &gc->list;
	    GLbyte *PC=0;
	
		if (lstate->mode == GL_COMPILE_AND_EXECUTE) 
		   GLIM_COLOR3UBV(gc, v);
		__GL_BEGIN(__glop_Color3ubv);
		__GL_PUT_CHAR(v[0]);
		__GL_PUT_CHAR(v[1]);
		__GL_PUT_CHAR(v[2]);
		__GL_PUT_PAD(1);
		__GL_END(8);
		__GL_CHECK_SPACE(80);
	}
}

void __gllc_Color3uiv( __glContext *__gc, const GLuint *vv)
{
	__glContext *gc = 0;
	GLuint *v = 0;
	__asm__ __volatile__ ( "": "=c"(gc), "=d"(v) );
	{
	    __glListMachine *lstate = &gc->list;
	    GLbyte *PC=0;
	
		if (lstate->mode == GL_COMPILE_AND_EXECUTE) 
		   GLIM_COLOR3UIV(gc, v);
		__GL_BEGIN(__glop_Color3uiv);
		__GL_PUT_LONG(v[0]);
		__GL_PUT_LONG(v[1]);
		__GL_PUT_LONG(v[2]);
		__GL_END(16);
		__GL_CHECK_SPACE(80);
	}
}

void __gllc_Color3usv( __glContext *__gc, const GLushort *vv)
{
	__glContext *gc = 0;
	GLushort *v = 0;
	__asm__ __volatile__ ( "": "=c"(gc), "=d"(v) );
	{
	    __glListMachine *lstate = &gc->list;
	    GLbyte *PC=0;
	
		if (lstate->mode == GL_COMPILE_AND_EXECUTE) 
		   GLIM_COLOR3USV(gc, v);
		__GL_BEGIN(__glop_Color3usv);
		__GL_PUT_SHORT(v[0]);
		__GL_PUT_SHORT(v[1]);
		__GL_PUT_SHORT(v[2]);
		__GL_PUT_PAD(2);
		__GL_END(12);
		__GL_CHECK_SPACE(80);
	}
}

void __gllc_Color4bv( __glContext *__gc, const GLbyte *vv)
{
	__glContext *gc = 0;
	GLbyte *v = 0;
	__asm__ __volatile__ ( "": "=c"(gc), "=d"(v) );
	{
	    __glListMachine *lstate = &gc->list;
	    GLbyte *PC=0;
	
		if (lstate->mode == GL_COMPILE_AND_EXECUTE) 
		   GLIM_COLOR4BV(gc, v);
		__GL_BEGIN(__glop_Color4bv);
		__GL_PUT_CHAR(v[0]);
		__GL_PUT_CHAR(v[1]);
		__GL_PUT_CHAR(v[2]);
		__GL_PUT_CHAR(v[3]);
		__GL_END(8);
		__GL_CHECK_SPACE(80);
	}
}

void __gllc_Color4dv( __glContext *__gc, const GLdouble *vv)
{
	__glContext *gc = 0;
	GLdouble *v = 0;
	__asm__ __volatile__ ( "": "=c"(gc), "=d"(v) );
	{
	    __glListMachine *lstate = &gc->list;
	    GLbyte *PC=0;
	
//		if (lstate->mode == GL_COMPILE_AND_EXECUTE) 
//		   GLIM_COLOR4DV(gc, v);
		PC = lstate->pc;
		if ((((int) PC) & 0x7) == 0) {
		    __GL_BEGIN(__glop_Nop);
		    __GL_END(4);
		}
		__GL_BEGIN(__glop_Color4dv);
		__GL_PUT_DOUBLE(v[0]);
		__GL_PUT_DOUBLE(v[1]);
		__GL_PUT_DOUBLE(v[2]);
		__GL_PUT_DOUBLE(v[3]);
		__GL_END(36);
		__GL_CHECK_SPACE(80);
	}
}

void __gllc_Color4fv( __glContext *__gc, const GLfloat *vv)
{
	__glContext *gc = 0;
	GLfloat *v = 0;
	__asm__ __volatile__ ( "": "=c"(gc), "=d"(v) );
	{
	    __glListMachine *lstate = &gc->list;
	    GLbyte *PC=0;
	
		if (lstate->mode == GL_COMPILE_AND_EXECUTE) 
		   GLIM_COLOR4FV(gc, v);
		__GL_BEGIN(__glop_Color4fv);
		__GL_PUT_FLOAT(v[0]);
		__GL_PUT_FLOAT(v[1]);
		__GL_PUT_FLOAT(v[2]);
		__GL_PUT_FLOAT(v[3]);
		__GL_END(20);
		__GL_CHECK_SPACE(80);
	}
}

void __gllc_Color4iv( __glContext *__gc, const GLint *vv)
{
	__glContext *gc = 0;
	GLint *v = 0;
	__asm__ __volatile__ ( "": "=c"(gc), "=d"(v) );
	{
	    __glListMachine *lstate = &gc->list;
	    GLbyte *PC=0;
	
		if (lstate->mode == GL_COMPILE_AND_EXECUTE) 
		   GLIM_COLOR4IV(gc, v);
		__GL_BEGIN(__glop_Color4iv);
		__GL_PUT_LONG(v[0]);
		__GL_PUT_LONG(v[1]);
		__GL_PUT_LONG(v[2]);
		__GL_PUT_LONG(v[3]);
		__GL_END(20);
		__GL_CHECK_SPACE(80);
	}
}

void __gllc_Color4sv( __glContext *__gc, const GLshort *vv)
{
	__glContext *gc = 0;
	GLshort *v = 0;
	__asm__ __volatile__ ( "": "=c"(gc), "=d"(v) );
	{
	    __glListMachine *lstate = &gc->list;
	    GLbyte *PC=0;
	
		if (lstate->mode == GL_COMPILE_AND_EXECUTE) 
		   GLIM_COLOR4SV(gc, v);
		__GL_BEGIN(__glop_Color4sv);
		__GL_PUT_SHORT(v[0]);
		__GL_PUT_SHORT(v[1]);
		__GL_PUT_SHORT(v[2]);
		__GL_PUT_SHORT(v[3]);
		__GL_END(12);
		__GL_CHECK_SPACE(80);
	}
}

void __gllc_Color4ubv( __glContext *__gc, const GLubyte *vv)
{
	__glContext *gc = 0;
	GLubyte *v = 0;
	__asm__ __volatile__ ( "": "=c"(gc), "=d"(v) );
	{
	    __glListMachine *lstate = &gc->list;
	    GLbyte *PC=0;
	
		if (lstate->mode == GL_COMPILE_AND_EXECUTE) 
		   GLIM_COLOR4UBV(gc, v);
		__GL_BEGIN(__glop_Color4ubv);
		__GL_PUT_CHAR(v[0]);
		__GL_PUT_CHAR(v[1]);
		__GL_PUT_CHAR(v[2]);
		__GL_PUT_CHAR(v[3]);
		__GL_END(8);
		__GL_CHECK_SPACE(80);
	}
}

void __gllc_Color4uiv( __glContext *__gc, const GLuint *vv)
{
	__glContext *gc = 0;
	GLuint *v = 0;
	__asm__ __volatile__ ( "": "=c"(gc), "=d"(v) );
	{
	    __glListMachine *lstate = &gc->list;
	    GLbyte *PC=0;
	
		if (lstate->mode == GL_COMPILE_AND_EXECUTE) 
		   GLIM_COLOR4UIV(gc, v);
		__GL_BEGIN(__glop_Color4uiv);
		__GL_PUT_LONG(v[0]);
		__GL_PUT_LONG(v[1]);
		__GL_PUT_LONG(v[2]);
		__GL_PUT_LONG(v[3]);
		__GL_END(20);
		__GL_CHECK_SPACE(80);
	}
}

void __gllc_Color4usv( __glContext *__gc, const GLushort *vv)
{
	__glContext *gc = 0;
	GLushort *v = 0;
	__asm__ __volatile__ ( "": "=c"(gc), "=d"(v) );
	{
	    __glListMachine *lstate = &gc->list;
	    GLbyte *PC=0;
	
		if (lstate->mode == GL_COMPILE_AND_EXECUTE) 
		   GLIM_COLOR4USV(gc, v);
		__GL_BEGIN(__glop_Color4usv);
		__GL_PUT_SHORT(v[0]);
		__GL_PUT_SHORT(v[1]);
		__GL_PUT_SHORT(v[2]);
		__GL_PUT_SHORT(v[3]);
		__GL_END(12);
		__GL_CHECK_SPACE(80);
	}
}

void __gllc_EdgeFlag( __glContext *gc, GLboolean flag)
{
    __glListMachine *lstate = &gc->list;
    GLbyte *PC=0;

	if (lstate->mode == GL_COMPILE_AND_EXECUTE) 
	   __glim_EdgeFlag(gc, flag);
	__GL_BEGIN(__glop_EdgeFlagv);
	__GL_PUT_CHAR(flag);
	__GL_PUT_PAD(3);
	__GL_END(8);
	__GL_CHECK_SPACE(80);
}

void __gllc_EdgeFlagv( __glContext *gc, const GLboolean *flag)
{
    __glListMachine *lstate = &gc->list;
    GLbyte *PC=0;

	if (lstate->mode == GL_COMPILE_AND_EXECUTE) 
	   __glim_EdgeFlagv(gc, flag);
	__GL_BEGIN(__glop_EdgeFlagv);
	__GL_PUT_CHAR(flag[0]);
	__GL_PUT_PAD(3);
	__GL_END(8);
	__GL_CHECK_SPACE(80);
}

void __gllc_End()
{
	__glContext *gc = GET_CONTEXT;
    __glListMachine *lstate = &gc->list;
    GLbyte *PC=0;

	if (lstate->mode == GL_COMPILE_AND_EXECUTE) 
	   GLIM_END( gc );
	__GL_BEGIN(__glop_End);
	__GL_END(4);
	__GL_CHECK_SPACE(80);
}

void __gllc_Indexd( __glContext *gc, GLdouble c)
{
    __glListMachine *lstate = &gc->list;
    GLbyte *PC=0;

	if (lstate->mode == GL_COMPILE_AND_EXECUTE) 
	   __glim_Indexd(gc, c);
	PC = lstate->pc;
	if ((((int) PC) & 0x7) == 0) {
	    __GL_BEGIN(__glop_Nop);
	    __GL_END(4);
	}
	__GL_BEGIN(__glop_Indexdv);
	__GL_PUT_DOUBLE(c);
	__GL_END(12);
	__GL_CHECK_SPACE(80);
}

void __gllc_Indexdv( __glContext *gc, const GLdouble *c)
{
    __glListMachine *lstate = &gc->list;
    GLbyte *PC=0;

	if (lstate->mode == GL_COMPILE_AND_EXECUTE) 
	   __glim_Indexdv(gc, c);
	PC = lstate->pc;
	if ((((int) PC) & 0x7) == 0) {
	    __GL_BEGIN(__glop_Nop);
	    __GL_END(4);
	}
	__GL_BEGIN(__glop_Indexdv);
	__GL_PUT_DOUBLE(c[0]);
	__GL_END(12);
	__GL_CHECK_SPACE(80);
}

void __gllc_Indexf( __glContext *gc, GLfloat c)
{
    __glListMachine *lstate = &gc->list;
    GLbyte *PC=0;

	if (lstate->mode == GL_COMPILE_AND_EXECUTE) 
	   __glim_Indexf(gc, c);
	__GL_BEGIN(__glop_Indexfv);
	__GL_PUT_FLOAT(c);
	__GL_END(8);
	__GL_CHECK_SPACE(80);
}

void __gllc_Indexfv( __glContext *gc, const GLfloat *c)
{
    __glListMachine *lstate = &gc->list;
    GLbyte *PC=0;

	if (lstate->mode == GL_COMPILE_AND_EXECUTE) 
	   __glim_Indexfv(gc, c);
	__GL_BEGIN(__glop_Indexfv);
	__GL_PUT_FLOAT(c[0]);
	__GL_END(8);
	__GL_CHECK_SPACE(80);
}

void __gllc_Indexi( __glContext *gc, GLint c)
{
    __glListMachine *lstate = &gc->list;
    GLbyte *PC=0;

	if (lstate->mode == GL_COMPILE_AND_EXECUTE) 
	   __glim_Indexi(gc, c);
	__GL_BEGIN(__glop_Indexiv);
	__GL_PUT_LONG(c);
	__GL_END(8);
	__GL_CHECK_SPACE(80);
}

void __gllc_Indexiv( __glContext *gc, const GLint *c)
{
    __glListMachine *lstate = &gc->list;
    GLbyte *PC=0;

	if (lstate->mode == GL_COMPILE_AND_EXECUTE) 
	   __glim_Indexiv(gc, c);
	__GL_BEGIN(__glop_Indexiv);
	__GL_PUT_LONG(c[0]);
	__GL_END(8);
	__GL_CHECK_SPACE(80);
}

void __gllc_Indexs( __glContext *gc, GLshort c)
{
    __glListMachine *lstate = &gc->list;
    GLbyte *PC=0;

	if (lstate->mode == GL_COMPILE_AND_EXECUTE) 
	   __glim_Indexs(gc, c);
	__GL_BEGIN(__glop_Indexsv);
	__GL_PUT_SHORT(c);
	__GL_PUT_PAD(2);
	__GL_END(8);
	__GL_CHECK_SPACE(80);
}

void __gllc_Indexsv( __glContext *gc, const GLshort *c)
{
    __glListMachine *lstate = &gc->list;
    GLbyte *PC=0;

	if (lstate->mode == GL_COMPILE_AND_EXECUTE) 
	   __glim_Indexsv(gc, c);
	__GL_BEGIN(__glop_Indexsv);
	__GL_PUT_SHORT(c[0]);
	__GL_PUT_PAD(2);
	__GL_END(8);
	__GL_CHECK_SPACE(80);
}

void __gllc_Indexub( __glContext *gc, GLubyte c)
{
    __glListMachine *lstate = &gc->list;
    GLbyte *PC=0;

	if (lstate->mode == GL_COMPILE_AND_EXECUTE) 
	   __glim_Indexub(gc, c);
	__GL_BEGIN(__glop_Indexubv);
	__GL_PUT_CHAR(c);
	__GL_PUT_PAD(3);
	__GL_END(8);
	__GL_CHECK_SPACE(80);
}

void __gllc_Indexubv( __glContext *gc, const GLubyte *c)
{
    __glListMachine *lstate = &gc->list;
    GLbyte *PC=0;

	if (lstate->mode == GL_COMPILE_AND_EXECUTE) 
	   __glim_Indexubv(gc, c);
	__GL_BEGIN(__glop_Indexubv);
	__GL_PUT_CHAR(c[0]);
	__GL_PUT_PAD(3);
	__GL_END(8);
	__GL_CHECK_SPACE(80);
}

void __gllcc_Normal3fv( const GLfloat *vv)
{
	__glContext *gc = GET_CONTEXT;
    __glListMachine *lstate = &gc->list;
    GLbyte *PC=0;
	
	if (lstate->mode == GL_COMPILE_AND_EXECUTE) 
	   GLIM_NORMAL3FV(gc, vv);
	__GL_BEGIN(__glop_Normal3fv);
	__GL_PUT_FLOAT(vv[0]);
	__GL_PUT_FLOAT(vv[1]);
	__GL_PUT_FLOAT(vv[2]);
	__GL_END(16);
	__GL_CHECK_SPACE(80);
}

void __gllc_RasterPos2d( __glContext *gc, GLdouble x, GLdouble y)
{
    __glListMachine *lstate = &gc->list;
    GLbyte *PC=0;

	if (lstate->mode == GL_COMPILE_AND_EXECUTE) 
	   __glim_RasterPos2d(gc, x, y);
	PC = lstate->pc;
	if ((((int) PC) & 0x7) == 0) {
	    __GL_BEGIN(__glop_Nop);
	    __GL_END(4);
	}
	__GL_BEGIN(__glop_RasterPos2dv);
	__GL_PUT_DOUBLE(x);
	__GL_PUT_DOUBLE(y);
	__GL_END(20);
	__GL_CHECK_SPACE(80);
}

void __gllc_RasterPos2dv( __glContext *gc, const GLdouble *v)
{
    __glListMachine *lstate = &gc->list;
    GLbyte *PC=0;

	if (lstate->mode == GL_COMPILE_AND_EXECUTE) 
	   __glim_RasterPos2dv(gc, v);
	PC = lstate->pc;
	if ((((int) PC) & 0x7) == 0) {
	    __GL_BEGIN(__glop_Nop);
	    __GL_END(4);
	}
	__GL_BEGIN(__glop_RasterPos2dv);
	__GL_PUT_DOUBLE(v[0]);
	__GL_PUT_DOUBLE(v[1]);
	__GL_END(20);
	__GL_CHECK_SPACE(80);
}

void __gllc_RasterPos2f( __glContext *gc, GLfloat x, GLfloat y)
{
    __glListMachine *lstate = &gc->list;
    GLbyte *PC=0;

	if (lstate->mode == GL_COMPILE_AND_EXECUTE) 
	   __glim_RasterPos2f(gc, x, y);
	__GL_BEGIN(__glop_RasterPos2fv);
	__GL_PUT_FLOAT(x);
	__GL_PUT_FLOAT(y);
	__GL_END(12);
	__GL_CHECK_SPACE(80);
}

void __gllc_RasterPos2fv( __glContext *gc, const GLfloat *v)
{
    __glListMachine *lstate = &gc->list;
    GLbyte *PC=0;

	if (lstate->mode == GL_COMPILE_AND_EXECUTE) 
	   __glim_RasterPos2fv(gc, v);
	__GL_BEGIN(__glop_RasterPos2fv);
	__GL_PUT_FLOAT(v[0]);
	__GL_PUT_FLOAT(v[1]);
	__GL_END(12);
	__GL_CHECK_SPACE(80);
}

void __gllc_RasterPos2i( __glContext *gc, GLint x, GLint y)
{
    __glListMachine *lstate = &gc->list;
    GLbyte *PC=0;

	if (lstate->mode == GL_COMPILE_AND_EXECUTE) 
	   __glim_RasterPos2i(gc, x, y);
	__GL_BEGIN(__glop_RasterPos2iv);
	__GL_PUT_LONG(x);
	__GL_PUT_LONG(y);
	__GL_END(12);
	__GL_CHECK_SPACE(80);
}

void __gllc_RasterPos2iv( __glContext *gc, const GLint *v)
{
    __glListMachine *lstate = &gc->list;
    GLbyte *PC=0;

	if (lstate->mode == GL_COMPILE_AND_EXECUTE) 
	   __glim_RasterPos2iv(gc, v);
	__GL_BEGIN(__glop_RasterPos2iv);
	__GL_PUT_LONG(v[0]);
	__GL_PUT_LONG(v[1]);
	__GL_END(12);
	__GL_CHECK_SPACE(80);
}

void __gllc_RasterPos2s( __glContext *gc, GLshort x, GLshort y)
{
    __glListMachine *lstate = &gc->list;
    GLbyte *PC=0;

	if (lstate->mode == GL_COMPILE_AND_EXECUTE) 
	   __glim_RasterPos2s(gc, x, y);
	__GL_BEGIN(__glop_RasterPos2sv);
	__GL_PUT_SHORT(x);
	__GL_PUT_SHORT(y);
	__GL_END(8);
	__GL_CHECK_SPACE(80);
}

void __gllc_RasterPos2sv( __glContext *gc, const GLshort *v)
{
    __glListMachine *lstate = &gc->list;
    GLbyte *PC=0;

	if (lstate->mode == GL_COMPILE_AND_EXECUTE) 
	   __glim_RasterPos2sv(gc, v);
	__GL_BEGIN(__glop_RasterPos2sv);
	__GL_PUT_SHORT(v[0]);
	__GL_PUT_SHORT(v[1]);
	__GL_END(8);
	__GL_CHECK_SPACE(80);
}

void __gllc_RasterPos3d( __glContext *gc, GLdouble x, GLdouble y, GLdouble z)
{
    __glListMachine *lstate = &gc->list;
    GLbyte *PC=0;

	if (lstate->mode == GL_COMPILE_AND_EXECUTE) 
	   __glim_RasterPos3d(gc, x, y, z);
	PC = lstate->pc;
	if ((((int) PC) & 0x7) == 0) {
	    __GL_BEGIN(__glop_Nop);
	    __GL_END(4);
	}
	__GL_BEGIN(__glop_RasterPos3dv);
	__GL_PUT_DOUBLE(x);
	__GL_PUT_DOUBLE(y);
	__GL_PUT_DOUBLE(z);
	__GL_END(28);
	__GL_CHECK_SPACE(80);
}

void __gllc_RasterPos3dv( __glContext *gc, const GLdouble *v)
{
    __glListMachine *lstate = &gc->list;
    GLbyte *PC=0;

	if (lstate->mode == GL_COMPILE_AND_EXECUTE) 
	   __glim_RasterPos3dv(gc, v);
	PC = lstate->pc;
	if ((((int) PC) & 0x7) == 0) {
	    __GL_BEGIN(__glop_Nop);
	    __GL_END(4);
	}
	__GL_BEGIN(__glop_RasterPos3dv);
	__GL_PUT_DOUBLE(v[0]);
	__GL_PUT_DOUBLE(v[1]);
	__GL_PUT_DOUBLE(v[2]);
	__GL_END(28);
	__GL_CHECK_SPACE(80);
}

void __gllc_RasterPos3f( __glContext *gc, GLfloat x, GLfloat y, GLfloat z)
{
    __glListMachine *lstate = &gc->list;
    GLbyte *PC=0;

	if (lstate->mode == GL_COMPILE_AND_EXECUTE) 
	   __glim_RasterPos3f(gc, x, y, z);
	__GL_BEGIN(__glop_RasterPos3fv);
	__GL_PUT_FLOAT(x);
	__GL_PUT_FLOAT(y);
	__GL_PUT_FLOAT(z);
	__GL_END(16);
	__GL_CHECK_SPACE(80);
}

void __gllc_RasterPos3fv( __glContext *gc, const GLfloat *v)
{
    __glListMachine *lstate = &gc->list;
    GLbyte *PC=0;

	if (lstate->mode == GL_COMPILE_AND_EXECUTE) 
	   __glim_RasterPos3fv(gc, v);
	__GL_BEGIN(__glop_RasterPos3fv);
	__GL_PUT_FLOAT(v[0]);
	__GL_PUT_FLOAT(v[1]);
	__GL_PUT_FLOAT(v[2]);
	__GL_END(16);
	__GL_CHECK_SPACE(80);
}

void __gllc_RasterPos3i( __glContext *gc, GLint x, GLint y, GLint z)
{
    __glListMachine *lstate = &gc->list;
    GLbyte *PC=0;

	if (lstate->mode == GL_COMPILE_AND_EXECUTE) 
	   __glim_RasterPos3i(gc, x, y, z);
	__GL_BEGIN(__glop_RasterPos3iv);
	__GL_PUT_LONG(x);
	__GL_PUT_LONG(y);
	__GL_PUT_LONG(z);
	__GL_END(16);
	__GL_CHECK_SPACE(80);
}

void __gllc_RasterPos3iv( __glContext *gc, const GLint *v)
{
    __glListMachine *lstate = &gc->list;
    GLbyte *PC=0;

	if (lstate->mode == GL_COMPILE_AND_EXECUTE) 
	   __glim_RasterPos3iv(gc, v);
	__GL_BEGIN(__glop_RasterPos3iv);
	__GL_PUT_LONG(v[0]);
	__GL_PUT_LONG(v[1]);
	__GL_PUT_LONG(v[2]);
	__GL_END(16);
	__GL_CHECK_SPACE(80);
}

void __gllc_RasterPos3s( __glContext *gc, GLshort x, GLshort y, GLshort z)
{
    __glListMachine *lstate = &gc->list;
    GLbyte *PC=0;

	if (lstate->mode == GL_COMPILE_AND_EXECUTE) 
	   __glim_RasterPos3s(gc, x, y, z);
	__GL_BEGIN(__glop_RasterPos3sv);
	__GL_PUT_SHORT(x);
	__GL_PUT_SHORT(y);
	__GL_PUT_SHORT(z);
	__GL_PUT_PAD(2);
	__GL_END(12);
	__GL_CHECK_SPACE(80);
}

void __gllc_RasterPos3sv( __glContext *gc, const GLshort *v)
{
    __glListMachine *lstate = &gc->list;
    GLbyte *PC=0;

	if (lstate->mode == GL_COMPILE_AND_EXECUTE) 
	   __glim_RasterPos3sv(gc, v);
	__GL_BEGIN(__glop_RasterPos3sv);
	__GL_PUT_SHORT(v[0]);
	__GL_PUT_SHORT(v[1]);
	__GL_PUT_SHORT(v[2]);
	__GL_PUT_PAD(2);
	__GL_END(12);
	__GL_CHECK_SPACE(80);
}

void __gllc_RasterPos4d( __glContext *gc, GLdouble x, GLdouble y, GLdouble z, GLdouble w)
{
    __glListMachine *lstate = &gc->list;
    GLbyte *PC=0;

	if (lstate->mode == GL_COMPILE_AND_EXECUTE) 
	   __glim_RasterPos4d(gc, x, y, z, w);
	PC = lstate->pc;
	if ((((int) PC) & 0x7) == 0) {
	    __GL_BEGIN(__glop_Nop);
	    __GL_END(4);
	}
	__GL_BEGIN(__glop_RasterPos4dv);
	__GL_PUT_DOUBLE(x);
	__GL_PUT_DOUBLE(y);
	__GL_PUT_DOUBLE(z);
	__GL_PUT_DOUBLE(w);
	__GL_END(36);
	__GL_CHECK_SPACE(80);
}

void __gllc_RasterPos4dv( __glContext *gc, const GLdouble *v)
{
    __glListMachine *lstate = &gc->list;
    GLbyte *PC=0;

	if (lstate->mode == GL_COMPILE_AND_EXECUTE) 
	   __glim_RasterPos4dv(gc, v);
	PC = lstate->pc;
	if ((((int) PC) & 0x7) == 0) {
	    __GL_BEGIN(__glop_Nop);
	    __GL_END(4);
	}
	__GL_BEGIN(__glop_RasterPos4dv);
	__GL_PUT_DOUBLE(v[0]);
	__GL_PUT_DOUBLE(v[1]);
	__GL_PUT_DOUBLE(v[2]);
	__GL_PUT_DOUBLE(v[3]);
	__GL_END(36);
	__GL_CHECK_SPACE(80);
}

void __gllc_RasterPos4f( __glContext *gc, GLfloat x, GLfloat y, GLfloat z, GLfloat w)
{
    __glListMachine *lstate = &gc->list;
    GLbyte *PC=0;

	if (lstate->mode == GL_COMPILE_AND_EXECUTE) 
	   __glim_RasterPos4f(gc, x, y, z, w);
	__GL_BEGIN(__glop_RasterPos4fv);
	__GL_PUT_FLOAT(x);
	__GL_PUT_FLOAT(y);
	__GL_PUT_FLOAT(z);
	__GL_PUT_FLOAT(w);
	__GL_END(20);
	__GL_CHECK_SPACE(80);
}

void __gllc_RasterPos4fv( __glContext *gc, const GLfloat *v)
{
    __glListMachine *lstate = &gc->list;
    GLbyte *PC=0;

	if (lstate->mode == GL_COMPILE_AND_EXECUTE) 
	   __glim_RasterPos4fv(gc, v);
	__GL_BEGIN(__glop_RasterPos4fv);
	__GL_PUT_FLOAT(v[0]);
	__GL_PUT_FLOAT(v[1]);
	__GL_PUT_FLOAT(v[2]);
	__GL_PUT_FLOAT(v[3]);
	__GL_END(20);
	__GL_CHECK_SPACE(80);
}

void __gllc_RasterPos4i( __glContext *gc, GLint x, GLint y, GLint z, GLint w)
{
    __glListMachine *lstate = &gc->list;
    GLbyte *PC=0;

	if (lstate->mode == GL_COMPILE_AND_EXECUTE) 
	   __glim_RasterPos4i(gc, x, y, z, w);
	__GL_BEGIN(__glop_RasterPos4iv);
	__GL_PUT_LONG(x);
	__GL_PUT_LONG(y);
	__GL_PUT_LONG(z);
	__GL_PUT_LONG(w);
	__GL_END(20);
	__GL_CHECK_SPACE(80);
}

void __gllc_RasterPos4iv( __glContext *gc, const GLint *v)
{
    __glListMachine *lstate = &gc->list;
    GLbyte *PC=0;

	if (lstate->mode == GL_COMPILE_AND_EXECUTE) 
	   __glim_RasterPos4iv(gc, v);
	__GL_BEGIN(__glop_RasterPos4iv);
	__GL_PUT_LONG(v[0]);
	__GL_PUT_LONG(v[1]);
	__GL_PUT_LONG(v[2]);
	__GL_PUT_LONG(v[3]);
	__GL_END(20);
	__GL_CHECK_SPACE(80);
}

void __gllc_RasterPos4s( __glContext *gc, GLshort x, GLshort y, GLshort z, GLshort w)
{
    __glListMachine *lstate = &gc->list;
    GLbyte *PC=0;

	if (lstate->mode == GL_COMPILE_AND_EXECUTE) 
	   __glim_RasterPos4s(gc, x, y, z, w);
	__GL_BEGIN(__glop_RasterPos4sv);
	__GL_PUT_SHORT(x);
	__GL_PUT_SHORT(y);
	__GL_PUT_SHORT(z);
	__GL_PUT_SHORT(w);
	__GL_END(12);
	__GL_CHECK_SPACE(80);
}

void __gllc_RasterPos4sv( __glContext *gc, const GLshort *v)
{
    __glListMachine *lstate = &gc->list;
    GLbyte *PC=0;

	if (lstate->mode == GL_COMPILE_AND_EXECUTE) 
	   __glim_RasterPos4sv(gc, v);
	__GL_BEGIN(__glop_RasterPos4sv);
	__GL_PUT_SHORT(v[0]);
	__GL_PUT_SHORT(v[1]);
	__GL_PUT_SHORT(v[2]);
	__GL_PUT_SHORT(v[3]);
	__GL_END(12);
	__GL_CHECK_SPACE(80);
}

void __gllc_Rectd( __glContext *gc, GLdouble x1, GLdouble y1, GLdouble x2, GLdouble y2)
{
    __glListMachine *lstate = &gc->list;
    GLbyte *PC=0;

	if (lstate->mode == GL_COMPILE_AND_EXECUTE) 
	   __glim_Rectd(gc, x1, y1, x2, y2);
	PC = lstate->pc;
	if ((((int) PC) & 0x7) == 0) {
	    __GL_BEGIN(__glop_Nop);
	    __GL_END(4);
	}
	__GL_BEGIN(__glop_Rectdv);
	__GL_PUT_DOUBLE(x1);
	__GL_PUT_DOUBLE(y1);
	__GL_PUT_DOUBLE(x2);
	__GL_PUT_DOUBLE(y2);
	__GL_END(36);
	__GL_CHECK_SPACE(80);
}

void __gllc_Rectdv( __glContext *gc, const GLdouble *v1, const GLdouble *v2)
{
    __glListMachine *lstate = &gc->list;
    GLbyte *PC=0;

	if (lstate->mode == GL_COMPILE_AND_EXECUTE) 
	   __glim_Rectdv(gc, v1, v2);
	PC = lstate->pc;
	if ((((int) PC) & 0x7) == 0) {
	    __GL_BEGIN(__glop_Nop);
	    __GL_END(4);
	}
	__GL_BEGIN(__glop_Rectdv);
	__GL_PUT_DOUBLE(v1[0]);
	__GL_PUT_DOUBLE(v1[1]);
	__GL_PUT_DOUBLE(v2[0]);
	__GL_PUT_DOUBLE(v2[1]);
	__GL_END(36);
	__GL_CHECK_SPACE(80);
}

void __gllc_Rectf( __glContext *gc, GLfloat x1, GLfloat y1, GLfloat x2, GLfloat y2)
{
    __glListMachine *lstate = &gc->list;
    GLbyte *PC=0;

	if (lstate->mode == GL_COMPILE_AND_EXECUTE) 
	   __glim_Rectf(gc, x1, y1, x2, y2);
	__GL_BEGIN(__glop_Rectfv);
	__GL_PUT_FLOAT(x1);
	__GL_PUT_FLOAT(y1);
	__GL_PUT_FLOAT(x2);
	__GL_PUT_FLOAT(y2);
	__GL_END(20);
	__GL_CHECK_SPACE(80);
}

void __gllc_Rectfv( __glContext *gc, const GLfloat *v1, const GLfloat *v2)
{
    __glListMachine *lstate = &gc->list;
    GLbyte *PC=0;

	if (lstate->mode == GL_COMPILE_AND_EXECUTE) 
	   __glim_Rectfv(gc, v1, v2);
	__GL_BEGIN(__glop_Rectfv);
	__GL_PUT_FLOAT(v1[0]);
	__GL_PUT_FLOAT(v1[1]);
	__GL_PUT_FLOAT(v2[0]);
	__GL_PUT_FLOAT(v2[1]);
	__GL_END(20);
	__GL_CHECK_SPACE(80);
}

void __gllc_Recti( __glContext *gc, GLint x1, GLint y1, GLint x2, GLint y2)
{
    __glListMachine *lstate = &gc->list;
    GLbyte *PC=0;

	if (lstate->mode == GL_COMPILE_AND_EXECUTE) 
	   __glim_Recti(gc, x1, y1, x2, y2);
	__GL_BEGIN(__glop_Rectiv);
	__GL_PUT_LONG(x1);
	__GL_PUT_LONG(y1);
	__GL_PUT_LONG(x2);
	__GL_PUT_LONG(y2);
	__GL_END(20);
	__GL_CHECK_SPACE(80);
}

void __gllc_Rectiv( __glContext *gc, const GLint *v1, const GLint *v2)
{
    __glListMachine *lstate = &gc->list;
    GLbyte *PC=0;

	if (lstate->mode == GL_COMPILE_AND_EXECUTE) 
	   __glim_Rectiv(gc, v1, v2);
	__GL_BEGIN(__glop_Rectiv);
	__GL_PUT_LONG(v1[0]);
	__GL_PUT_LONG(v1[1]);
	__GL_PUT_LONG(v2[0]);
	__GL_PUT_LONG(v2[1]);
	__GL_END(20);
	__GL_CHECK_SPACE(80);
}

void __gllc_Rects( __glContext *gc, GLshort x1, GLshort y1, GLshort x2, GLshort y2)
{
    __glListMachine *lstate = &gc->list;
    GLbyte *PC=0;

	if (lstate->mode == GL_COMPILE_AND_EXECUTE) 
	   __glim_Rects(gc, x1, y1, x2, y2);
	__GL_BEGIN(__glop_Rectsv);
	__GL_PUT_SHORT(x1);
	__GL_PUT_SHORT(y1);
	__GL_PUT_SHORT(x2);
	__GL_PUT_SHORT(y2);
	__GL_END(12);
	__GL_CHECK_SPACE(80);
}

void __gllc_Rectsv( __glContext *gc, const GLshort *v1, const GLshort *v2)
{
    __glListMachine *lstate = &gc->list;
    GLbyte *PC=0;

	if (lstate->mode == GL_COMPILE_AND_EXECUTE) 
	   __glim_Rectsv(gc, v1, v2);
	__GL_BEGIN(__glop_Rectsv);
	__GL_PUT_SHORT(v1[0]);
	__GL_PUT_SHORT(v1[1]);
	__GL_PUT_SHORT(v2[0]);
	__GL_PUT_SHORT(v2[1]);
	__GL_END(12);
	__GL_CHECK_SPACE(80);
}

void __gllc_TexCoord1fv( const GLfloat *v)
{
	GLfloat vv[1];
	__glContext *gc = GET_CONTEXT;
    __glListMachine *lstate = &gc->list;
    GLbyte *PC=0;

	__asm__ __volatile__ (
		"fstps %0 \n\t"
		: : "m"(vv[0]) );

	if (lstate->mode == GL_COMPILE_AND_EXECUTE) 
	   GLIM_TEXCOORD1FV(gc, v);
	__GL_BEGIN(__glop_TexCoord1fv);
	__GL_PUT_FLOAT(v[0]);
	__GL_END(8);
	__GL_CHECK_SPACE(80);
}

void __gllc_TexCoord2fv( const GLfloat *v)
{
	GLfloat vv[2];
	__glContext *gc = GET_CONTEXT;
    __glListMachine *lstate = &gc->list;
    GLbyte *PC=0;

	__asm__ __volatile__ (
		"fstps %0 \n\t"
		"fstps %1 \n\t"
		: : "m"(vv[0]), "m"(vv[1]) );

	if (lstate->mode == GL_COMPILE_AND_EXECUTE) 
	   GLIM_TEXCOORD2FV(gc, v);
	__GL_BEGIN(__glop_TexCoord2fv);
	__GL_PUT_FLOAT(vv[0]);
	__GL_PUT_FLOAT(vv[1]);
	__GL_END(12);
	__GL_CHECK_SPACE(80);
}

void __gllc_TexCoord3fv( const GLfloat *v)
{
	GLfloat vv[3];
	__glContext *gc = GET_CONTEXT;
    __glListMachine *lstate = &gc->list;
    GLbyte *PC=0;

	__asm__ __volatile__ (
		"fstps %0 \n\t"
		"fstps %1 \n\t"
		"fstps %2 \n\t"
		: : "m"(vv[0]), "m"(vv[1]), "m"(vv[2]) );

	if (lstate->mode == GL_COMPILE_AND_EXECUTE) 
	   GLIM_TEXCOORD3FV(gc, v);
	__GL_BEGIN(__glop_TexCoord3fv);
	__GL_PUT_FLOAT(vv[0]);
	__GL_PUT_FLOAT(vv[1]);
	__GL_PUT_FLOAT(vv[2]);
	__GL_END(16);
	__GL_CHECK_SPACE(80);
}

void __gllc_TexCoord4fv( const GLfloat *v)
{
	GLfloat vv[4];
	__glContext *gc = GET_CONTEXT;
    __glListMachine *lstate = &gc->list;
    GLbyte *PC=0;

	__asm__ __volatile__ (
		"fstps %0 \n\t"
		"fstps %1 \n\t"
		"fstps %2 \n\t"
		"fstps %3 \n\t"
		: : "m"(vv[0]), "m"(vv[1]), "m"(vv[2]), "m"(vv[3]) );

	if (lstate->mode == GL_COMPILE_AND_EXECUTE) 
	   GLIM_TEXCOORD4FV(gc, v);
	__GL_BEGIN(__glop_TexCoord4fv);
	__GL_PUT_FLOAT(vv[0]);
	__GL_PUT_FLOAT(vv[1]);
	__GL_PUT_FLOAT(vv[2]);
	__GL_PUT_FLOAT(vv[3]);
	__GL_END(20);
	__GL_CHECK_SPACE(80);
}

void __gllc_Vertex2fv( const GLfloat *v)
{
	GLfloat vv[2];
	__glContext *gc = GET_CONTEXT;
    __glListMachine *lstate = &gc->list;
    GLbyte *PC=0;

	__asm__ __volatile__ (
		"fstps %0 \n\t"
		"fstps %1 \n\t"
		: : "m"(vv[0]), "m"(vv[1]) );

	if (lstate->mode == GL_COMPILE_AND_EXECUTE)
		GLIM_VERTEX2FV(gc, vv);
	__GL_BEGIN(__glop_Vertex2fv);
	__GL_PUT_FLOAT(vv[0]);
	__GL_PUT_FLOAT(vv[1]);
	__GL_END(12);
	__GL_CHECK_SPACE(80);
}

void __gllc_Vertex3fv( const GLfloat *v)
{
	GLfloat vv[3];
	__glContext *gc = GET_CONTEXT;
    __glListMachine *lstate = &gc->list;
    GLbyte *PC=0;

	__asm__ __volatile__ (
		"fstps %0 \n\t"
		"fstps %1 \n\t"
		"fstps %2 \n\t"
		: : "m"(vv[0]), "m"(vv[1]), "m"(vv[2]) );

	if (lstate->mode == GL_COMPILE_AND_EXECUTE) 
		GLIM_VERTEX3FV(gc, vv);
	__GL_BEGIN(__glop_Vertex3fv);
	__GL_PUT_FLOAT(vv[0]);
	__GL_PUT_FLOAT(vv[1]);
	__GL_PUT_FLOAT(vv[2]);
	__GL_END(16);
	__GL_CHECK_SPACE(80);
}

void __gllc_Vertex4fv( const GLfloat *v)
{
	GLfloat vv[4];
	__glContext *gc = GET_CONTEXT;
    __glListMachine *lstate = &gc->list;
    GLbyte *PC=0;

	__asm__ __volatile__ (
		"fstps %0 \n\t"
		"fstps %1 \n\t"
		"fstps %2 \n\t"
		"fstps %3 \n\t"
		: : "m"(vv[0]), "m"(vv[1]), "m"(vv[2]), "m"(vv[3]) );

	if (lstate->mode == GL_COMPILE_AND_EXECUTE) 
		GLIM_VERTEX4FV(gc, vv);
	__GL_BEGIN(__glop_Vertex4fv);
	__GL_PUT_FLOAT(vv[0]);
	__GL_PUT_FLOAT(vv[1]);
	__GL_PUT_FLOAT(vv[2]);
	__GL_PUT_FLOAT(vv[3]);
	__GL_END(20);
	__GL_CHECK_SPACE(80);
}

void __gllc_ClipPlane( __glContext *gc, GLenum plane, const GLdouble *equation)
{
    __glListMachine *lstate = &gc->list;
    GLbyte *PC=0;

	if (lstate->mode == GL_COMPILE_AND_EXECUTE) 
	   __glim_ClipPlane(gc, plane, equation);
	PC = lstate->pc;
	if ((((int) PC) & 0x7) == 0) {
	    __GL_BEGIN(__glop_Nop);
	    __GL_END(4);
	}
	__GL_BEGIN(__glop_ClipPlane);
	__GL_PUT_DOUBLE(equation[0]);
	__GL_PUT_DOUBLE(equation[1]);
	__GL_PUT_DOUBLE(equation[2]);
	__GL_PUT_DOUBLE(equation[3]);
	__GL_PUT_LONG(plane);
	__GL_END(40);
	__GL_CHECK_SPACE(80);
}

void __gllc_ColorMaterial( __glContext *gc, GLenum face, GLenum mode)
{
    __glListMachine *lstate = &gc->list;
    GLbyte *PC=0;

	if (lstate->mode == GL_COMPILE_AND_EXECUTE) 
	   __glim_ColorMaterial(gc, face, mode);
	__GL_BEGIN(__glop_ColorMaterial);
	__GL_PUT_LONG(face);
	__GL_PUT_LONG(mode);
	__GL_END(12);
	__GL_CHECK_SPACE(80);
}

void __gllc_CullFace( __glContext *gc, GLenum mode)
{
    __glListMachine *lstate = &gc->list;
    GLbyte *PC=0;

	if (lstate->mode == GL_COMPILE_AND_EXECUTE) 
	   __glim_CullFace(gc, mode);
	__GL_BEGIN(__glop_CullFace);
	__GL_PUT_LONG(mode);
	__GL_END(8);
	__GL_CHECK_SPACE(80);
}

void __gllc_Fogf( __glContext *gc, GLenum pname, GLfloat param)
{
    __glListMachine *lstate = &gc->list;
    GLbyte *PC=0;

	if (lstate->mode == GL_COMPILE_AND_EXECUTE) 
	   __glim_Fogf(gc, pname, param);
	__GL_BEGIN(__glop_Fogf);
	__GL_PUT_LONG(pname);
	__GL_PUT_FLOAT(param);
	__GL_END(12);
	__GL_CHECK_SPACE(80);
}

void __gllc_Fogfv( __glContext *gc, GLenum pname, const GLfloat *params)
{
    __glListMachine *lstate = &gc->list;
    GLbyte *PC=0;
    GLint _cmdlen=0, _count0=0;

	if (lstate->mode == GL_COMPILE_AND_EXECUTE) 
	   __glim_Fogfv(gc, pname, params);
	_count0 = __glFogfv_size(pname);
	if (_count0 < 0) {
	    __gllc_Error(gc, -_count0);
	    return;
	}
	_cmdlen = 8+__GL_PAD(_count0*4);
	__GL_CHECK_SPACE(_cmdlen);
	__GL_BEGIN(__glop_Fogfv);
	__GL_PUT_LONG(pname);
	__GL_PUT_FLOAT_ARRAY(params,_count0);
	__GL_END(_cmdlen);
	__GL_CHECK_SPACE(80);
}

void __gllc_Fogi( __glContext *gc, GLenum pname, GLint param)
{
    __glListMachine *lstate = &gc->list;
    GLbyte *PC=0;

	if (lstate->mode == GL_COMPILE_AND_EXECUTE) 
	   __glim_Fogi(gc, pname, param);
	__GL_BEGIN(__glop_Fogi);
	__GL_PUT_LONG(pname);
	__GL_PUT_LONG(param);
	__GL_END(12);
	__GL_CHECK_SPACE(80);
}

void __gllc_Fogiv( __glContext *gc, GLenum pname, const GLint *params)
{
    __glListMachine *lstate = &gc->list;
    GLbyte *PC=0;
    GLint _cmdlen=0, _count0=0;

	if (lstate->mode == GL_COMPILE_AND_EXECUTE) 
	   __glim_Fogiv(gc, pname, params);
	_count0 = __glFogiv_size(pname);
	if (_count0 < 0) {
	    __gllc_Error(gc, -_count0);
	    return;
	}
	_cmdlen = 8+__GL_PAD(_count0*4);
	__GL_CHECK_SPACE(_cmdlen);
	__GL_BEGIN(__glop_Fogiv);
	__GL_PUT_LONG(pname);
	__GL_PUT_LONG_ARRAY(params,_count0);
	__GL_END(_cmdlen);
	__GL_CHECK_SPACE(80);
}

void __gllc_FrontFace( __glContext *gc, GLenum mode)
{
    __glListMachine *lstate = &gc->list;
    GLbyte *PC=0;

	if (lstate->mode == GL_COMPILE_AND_EXECUTE) 
	   __glim_FrontFace(gc, mode);
	__GL_BEGIN(__glop_FrontFace);
	__GL_PUT_LONG(mode);
	__GL_END(8);
	__GL_CHECK_SPACE(80);
}

void __gllc_Hint( __glContext *gc, GLenum target, GLenum mode)
{
    __glListMachine *lstate = &gc->list;
    GLbyte *PC=0;

	if (lstate->mode == GL_COMPILE_AND_EXECUTE) 
	   __glim_Hint(gc, target, mode);
	__GL_BEGIN(__glop_Hint);
	__GL_PUT_LONG(target);
	__GL_PUT_LONG(mode);
	__GL_END(12);
	__GL_CHECK_SPACE(80);
}

void __gllc_Lightf( __glContext *gc, GLenum light, GLenum pname, GLfloat param)
{
    __glListMachine *lstate = &gc->list;
    GLbyte *PC=0;

	if (lstate->mode == GL_COMPILE_AND_EXECUTE) 
	   __glim_Lightf(gc, light, pname, param);
	__GL_BEGIN(__glop_Lightf);
	__GL_PUT_LONG(light);
	__GL_PUT_LONG(pname);
	__GL_PUT_FLOAT(param);
	__GL_END(16);
	__GL_CHECK_SPACE(80);
}

void __gllc_Lightfv( __glContext *gc, GLenum light, GLenum pname, const GLfloat *params)
{
    __glListMachine *lstate = &gc->list;
    GLbyte *PC=0;
    GLint _cmdlen=0, _count0=0;

	if (lstate->mode == GL_COMPILE_AND_EXECUTE) 
	   __glim_Lightfv(gc, light, pname, params);
	_count0 = __glLightfv_size(pname);
	if (_count0 < 0) {
	    __gllc_Error(gc, -_count0);
	    return;
	}
	_cmdlen = 12+__GL_PAD(_count0*4);
	__GL_CHECK_SPACE(_cmdlen);
	__GL_BEGIN(__glop_Lightfv);
	__GL_PUT_LONG(light);
	__GL_PUT_LONG(pname);
	__GL_PUT_FLOAT_ARRAY(params,_count0);
	__GL_END(_cmdlen);
	__GL_CHECK_SPACE(80);
}

void __gllc_Lighti( __glContext *gc, GLenum light, GLenum pname, GLint param)
{
    __glListMachine *lstate = &gc->list;
    GLbyte *PC=0;

	if (lstate->mode == GL_COMPILE_AND_EXECUTE) 
	   __glim_Lighti(gc, light, pname, param);
	__GL_BEGIN(__glop_Lighti);
	__GL_PUT_LONG(light);
	__GL_PUT_LONG(pname);
	__GL_PUT_LONG(param);
	__GL_END(16);
	__GL_CHECK_SPACE(80);
}

void __gllc_Lightiv( __glContext *gc, GLenum light, GLenum pname, const GLint *params)
{
    __glListMachine *lstate = &gc->list;
    GLbyte *PC=0;
    GLint _cmdlen=0, _count0=0;

	if (lstate->mode == GL_COMPILE_AND_EXECUTE) 
	   __glim_Lightiv(gc, light, pname, params);
	_count0 = __glLightiv_size(pname);
	if (_count0 < 0) {
	    __gllc_Error(gc, -_count0);
	    return;
	}
	_cmdlen = 12+__GL_PAD(_count0*4);
	__GL_CHECK_SPACE(_cmdlen);
	__GL_BEGIN(__glop_Lightiv);
	__GL_PUT_LONG(light);
	__GL_PUT_LONG(pname);
	__GL_PUT_LONG_ARRAY(params,_count0);
	__GL_END(_cmdlen);
	__GL_CHECK_SPACE(80);
}

void __gllc_LightModelf( __glContext *gc, GLenum pname, GLfloat param)
{
    __glListMachine *lstate = &gc->list;
    GLbyte *PC=0;

	if (lstate->mode == GL_COMPILE_AND_EXECUTE) 
	   __glim_LightModelf(gc, pname, param);
	__GL_BEGIN(__glop_LightModelf);
	__GL_PUT_LONG(pname);
	__GL_PUT_FLOAT(param);
	__GL_END(12);
	__GL_CHECK_SPACE(80);
}

void __gllc_LightModelfv( __glContext *gc, GLenum pname, const GLfloat *params)
{
    __glListMachine *lstate = &gc->list;
    GLbyte *PC=0;
    GLint _cmdlen=0, _count0=0;

	if (lstate->mode == GL_COMPILE_AND_EXECUTE) 
	   __glim_LightModelfv(gc, pname, params);
	_count0 = __glLightModelfv_size(pname);
	if (_count0 < 0) {
	    __gllc_Error(gc, -_count0);
	    return;
	}
	_cmdlen = 8+__GL_PAD(_count0*4);
	__GL_CHECK_SPACE(_cmdlen);
	__GL_BEGIN(__glop_LightModelfv);
	__GL_PUT_LONG(pname);
	__GL_PUT_FLOAT_ARRAY(params,_count0);
	__GL_END(_cmdlen);
	__GL_CHECK_SPACE(80);
}

void __gllc_LightModeli( __glContext *gc, GLenum pname, GLint param)
{
    __glListMachine *lstate = &gc->list;
    GLbyte *PC=0;

	if (lstate->mode == GL_COMPILE_AND_EXECUTE) 
	   __glim_LightModeli(gc, pname, param);
	__GL_BEGIN(__glop_LightModeli);
	__GL_PUT_LONG(pname);
	__GL_PUT_LONG(param);
	__GL_END(12);
	__GL_CHECK_SPACE(80);
}

void __gllc_LightModeliv( __glContext *gc, GLenum pname, const GLint *params)
{
    __glListMachine *lstate = &gc->list;
    GLbyte *PC=0;
    GLint _cmdlen=0, _count0=0;

	if (lstate->mode == GL_COMPILE_AND_EXECUTE) 
	   __glim_LightModeliv(gc, pname, params);
	_count0 = __glLightModeliv_size(pname);
	if (_count0 < 0) {
	    __gllc_Error(gc, -_count0);
	    return;
	}
	_cmdlen = 8+__GL_PAD(_count0*4);
	__GL_CHECK_SPACE(_cmdlen);
	__GL_BEGIN(__glop_LightModeliv);
	__GL_PUT_LONG(pname);
	__GL_PUT_LONG_ARRAY(params,_count0);
	__GL_END(_cmdlen);
	__GL_CHECK_SPACE(80);
}

void __gllc_LineStipple( __glContext *gc, GLint factor, GLushort pattern)
{
    __glListMachine *lstate = &gc->list;
    GLbyte *PC=0;

	if (lstate->mode == GL_COMPILE_AND_EXECUTE) 
	   __glim_LineStipple(gc, factor, pattern);
	__GL_BEGIN(__glop_LineStipple);
	__GL_PUT_LONG(factor);
	__GL_PUT_SHORT(pattern);
	__GL_PUT_PAD(2);
	__GL_END(12);
	__GL_CHECK_SPACE(80);
}

void __gllc_LineWidth( __glContext *gc, GLfloat width)
{
    __glListMachine *lstate = &gc->list;
    GLbyte *PC=0;

	if (lstate->mode == GL_COMPILE_AND_EXECUTE) 
	   __glim_LineWidth(gc, width);
	__GL_BEGIN(__glop_LineWidth);
	__GL_PUT_FLOAT(width);
	__GL_END(8);
	__GL_CHECK_SPACE(80);
}

void __gllc_Materialf( __glContext *gc, GLenum face, GLenum pname, GLfloat param)
{
    __glListMachine *lstate = &gc->list;
    GLbyte *PC=0;

	if (lstate->mode == GL_COMPILE_AND_EXECUTE) 
	   __glim_Materialf(gc, face, pname, param);
	__GL_BEGIN(__glop_Materialf);
	__GL_PUT_LONG(face);
	__GL_PUT_LONG(pname);
	__GL_PUT_FLOAT(param);
	__GL_END(16);
	__GL_CHECK_SPACE(80);
}

void __gllc_Materialfv( __glContext *gc, GLenum face, GLenum pname, const GLfloat *params)
{
    __glListMachine *lstate = &gc->list;
    GLbyte *PC=0;
    GLint _cmdlen=0, _count0=0;

	if (lstate->mode == GL_COMPILE_AND_EXECUTE) 
	   __glim_Materialfv(gc, face, pname, params);
	_count0 = __glMaterialfv_size(pname);
	if (_count0 < 0) {
	    __gllc_Error(gc, -_count0);
	    return;
	}
	_cmdlen = 12+__GL_PAD(_count0*4);
	__GL_CHECK_SPACE(_cmdlen);
	__GL_BEGIN(__glop_Materialfv);
	__GL_PUT_LONG(face);
	__GL_PUT_LONG(pname);
	__GL_PUT_FLOAT_ARRAY(params,_count0);
	__GL_END(_cmdlen);
	__GL_CHECK_SPACE(80);
}

void __gllc_Materiali( __glContext *gc, GLenum face, GLenum pname, GLint param)
{
    __glListMachine *lstate = &gc->list;
    GLbyte *PC=0;

	if (lstate->mode == GL_COMPILE_AND_EXECUTE) 
	   __glim_Materiali(gc, face, pname, param);
	__GL_BEGIN(__glop_Materiali);
	__GL_PUT_LONG(face);
	__GL_PUT_LONG(pname);
	__GL_PUT_LONG(param);
	__GL_END(16);
	__GL_CHECK_SPACE(80);
}

void __gllc_Materialiv( __glContext *gc, GLenum face, GLenum pname, const GLint *params)
{
    __glListMachine *lstate = &gc->list;
    GLbyte *PC=0;
    GLint _cmdlen=0, _count0=0;

	if (lstate->mode == GL_COMPILE_AND_EXECUTE) 
	   __glim_Materialiv(gc, face, pname, params);
	_count0 = __glMaterialiv_size(pname);
	if (_count0 < 0) {
	    __gllc_Error(gc, -_count0);
	    return;
	}
	_cmdlen = 12+__GL_PAD(_count0*4);
	__GL_CHECK_SPACE(_cmdlen);
	__GL_BEGIN(__glop_Materialiv);
	__GL_PUT_LONG(face);
	__GL_PUT_LONG(pname);
	__GL_PUT_LONG_ARRAY(params,_count0);
	__GL_END(_cmdlen);
	__GL_CHECK_SPACE(80);
}

void __gllc_PointSize( __glContext *gc, GLfloat size)
{
    __glListMachine *lstate = &gc->list;
    GLbyte *PC=0;

	if (lstate->mode == GL_COMPILE_AND_EXECUTE) 
	   __glim_PointSize(gc, size);
	__GL_BEGIN(__glop_PointSize);
	__GL_PUT_FLOAT(size);
	__GL_END(8);
	__GL_CHECK_SPACE(80);
}

void __gllc_PolygonMode( __glContext *gc, GLenum face, GLenum mode)
{
    __glListMachine *lstate = &gc->list;
    GLbyte *PC=0;

	if (lstate->mode == GL_COMPILE_AND_EXECUTE) 
	   __glim_PolygonMode(gc, face, mode);
	__GL_BEGIN(__glop_PolygonMode);
	__GL_PUT_LONG(face);
	__GL_PUT_LONG(mode);
	__GL_END(12);
	__GL_CHECK_SPACE(80);
}

void __gllc_Scissor( __glContext *gc, GLint x, GLint y, GLsizei width, GLsizei height)
{
    __glListMachine *lstate = &gc->list;
    GLbyte *PC=0;

	if (lstate->mode == GL_COMPILE_AND_EXECUTE) 
	   __glim_Scissor(gc, x, y, width, height);
	__GL_BEGIN(__glop_Scissor);
	__GL_PUT_LONG(x);
	__GL_PUT_LONG(y);
	__GL_PUT_LONG(width);
	__GL_PUT_LONG(height);
	__GL_END(20);
	__GL_CHECK_SPACE(80);
}

void __gllc_ShadeModel( __glContext *gc, GLenum mode)
{
    __glListMachine *lstate = &gc->list;
    GLbyte *PC=0;

	if (lstate->mode == GL_COMPILE_AND_EXECUTE) 
	   __glim_ShadeModel(gc, mode);
	__GL_BEGIN(__glop_ShadeModel);
	__GL_PUT_LONG(mode);
	__GL_END(8);
	__GL_CHECK_SPACE(80);
}

void __gllc_TexParameterf( __glContext *gc, GLenum target, GLenum pname, GLfloat param)
{
    __glListMachine *lstate = &gc->list;
    GLbyte *PC=0;

	if (lstate->mode == GL_COMPILE_AND_EXECUTE) 
	   __glim_TexParameterf(gc, target, pname, param);
	__GL_BEGIN(__glop_TexParameterf);
	__GL_PUT_LONG(target);
	__GL_PUT_LONG(pname);
	__GL_PUT_FLOAT(param);
	__GL_END(16);
	__GL_CHECK_SPACE(80);
}

void __gllc_TexParameterfv( __glContext *gc, GLenum target, GLenum pname, const GLfloat *params)
{
    __glListMachine *lstate = &gc->list;
    GLbyte *PC=0;
    GLint _cmdlen=0, _count0=0;

	if (lstate->mode == GL_COMPILE_AND_EXECUTE) 
	   __glim_TexParameterfv(gc, target, pname, params);
	_count0 = __glTexParameterfv_size(pname);
	if (_count0 < 0) {
	    __gllc_Error(gc, -_count0);
	    return;
	}
	_cmdlen = 12+__GL_PAD(_count0*4);
	__GL_CHECK_SPACE(_cmdlen);
	__GL_BEGIN(__glop_TexParameterfv);
	__GL_PUT_LONG(target);
	__GL_PUT_LONG(pname);
	__GL_PUT_FLOAT_ARRAY(params,_count0);
	__GL_END(_cmdlen);
	__GL_CHECK_SPACE(80);
}

void __gllc_TexParameteri( __glContext *gc, GLenum target, GLenum pname, GLint param)
{
    __glListMachine *lstate = &gc->list;
    GLbyte *PC=0;

	if (lstate->mode == GL_COMPILE_AND_EXECUTE) 
	   __glim_TexParameteri(gc, target, pname, param);
	__GL_BEGIN(__glop_TexParameteri);
	__GL_PUT_LONG(target);
	__GL_PUT_LONG(pname);
	__GL_PUT_LONG(param);
	__GL_END(16);
	__GL_CHECK_SPACE(80);
}

void __gllc_TexParameteriv( __glContext *gc, GLenum target, GLenum pname, const GLint *params)
{
    __glListMachine *lstate = &gc->list;
    GLbyte *PC=0;
    GLint _cmdlen=0, _count0=0;

	if (lstate->mode == GL_COMPILE_AND_EXECUTE) 
	   __glim_TexParameteriv(gc, target, pname, params);
	_count0 = __glTexParameteriv_size(pname);
	if (_count0 < 0) {
	    __gllc_Error(gc, -_count0);
	    return;
	}
	_cmdlen = 12+__GL_PAD(_count0*4);
	__GL_CHECK_SPACE(_cmdlen);
	__GL_BEGIN(__glop_TexParameteriv);
	__GL_PUT_LONG(target);
	__GL_PUT_LONG(pname);
	__GL_PUT_LONG_ARRAY(params,_count0);
	__GL_END(_cmdlen);
	__GL_CHECK_SPACE(80);
}

void __gllc_TexEnvf( __glContext *gc, GLenum target, GLenum pname, GLfloat param)
{
    __glListMachine *lstate = &gc->list;
    GLbyte *PC=0;

	if (lstate->mode == GL_COMPILE_AND_EXECUTE) 
	   __glim_TexEnvf(gc, target, pname, param);
	__GL_BEGIN(__glop_TexEnvf);
	__GL_PUT_LONG(target);
	__GL_PUT_LONG(pname);
	__GL_PUT_FLOAT(param);
	__GL_END(16);
	__GL_CHECK_SPACE(80);
}

void __gllc_TexEnvfv( __glContext *gc, GLenum target, GLenum pname, const GLfloat *params)
{
    __glListMachine *lstate = &gc->list;
    GLbyte *PC=0;
    GLint _cmdlen=0, _count0=0;

	if (lstate->mode == GL_COMPILE_AND_EXECUTE) 
	   __glim_TexEnvfv(gc, target, pname, params);
	_count0 = __glTexEnvfv_size(pname);
	if (_count0 < 0) {
	    __gllc_Error(gc, -_count0);
	    return;
	}
	_cmdlen = 12+__GL_PAD(_count0*4);
	__GL_CHECK_SPACE(_cmdlen);
	__GL_BEGIN(__glop_TexEnvfv);
	__GL_PUT_LONG(target);
	__GL_PUT_LONG(pname);
	__GL_PUT_FLOAT_ARRAY(params,_count0);
	__GL_END(_cmdlen);
	__GL_CHECK_SPACE(80);
}

void __gllc_TexEnvi( __glContext *gc, GLenum target, GLenum pname, GLint param)
{
    __glListMachine *lstate = &gc->list;
    GLbyte *PC=0;

	if (lstate->mode == GL_COMPILE_AND_EXECUTE) 
	   __glim_TexEnvi(gc, target, pname, param);
	__GL_BEGIN(__glop_TexEnvi);
	__GL_PUT_LONG(target);
	__GL_PUT_LONG(pname);
	__GL_PUT_LONG(param);
	__GL_END(16);
	__GL_CHECK_SPACE(80);
}

void __gllc_TexEnviv( __glContext *gc, GLenum target, GLenum pname, const GLint *params)
{
    __glListMachine *lstate = &gc->list;
    GLbyte *PC=0;
    GLint _cmdlen=0, _count0=0;

	if (lstate->mode == GL_COMPILE_AND_EXECUTE) 
	   __glim_TexEnviv(gc, target, pname, params);
	_count0 = __glTexEnviv_size(pname);
	if (_count0 < 0) {
	    __gllc_Error(gc, -_count0);
	    return;
	}
	_cmdlen = 12+__GL_PAD(_count0*4);
	__GL_CHECK_SPACE(_cmdlen);
	__GL_BEGIN(__glop_TexEnviv);
	__GL_PUT_LONG(target);
	__GL_PUT_LONG(pname);
	__GL_PUT_LONG_ARRAY(params,_count0);
	__GL_END(_cmdlen);
	__GL_CHECK_SPACE(80);
}

void __gllc_TexGend( __glContext *gc, GLenum coord, GLenum pname, GLdouble param)
{
    __glListMachine *lstate = &gc->list;
    GLbyte *PC=0;

	if (lstate->mode == GL_COMPILE_AND_EXECUTE) 
	   __glim_TexGend(gc, coord, pname, param);
	PC = lstate->pc;
	if ((((int) PC) & 0x7) == 0) {
	    __GL_BEGIN(__glop_Nop);
	    __GL_END(4);
	}
	__GL_BEGIN(__glop_TexGend);
	__GL_PUT_DOUBLE(param);
	__GL_PUT_LONG(coord);
	__GL_PUT_LONG(pname);
	__GL_END(20);
	__GL_CHECK_SPACE(80);
}

void __gllc_TexGendv( __glContext *gc, GLenum coord, GLenum pname, const GLdouble *params)
{
    __glListMachine *lstate = &gc->list;
    GLbyte *PC=0;
    GLint _cmdlen=0, _count0=0;

	if (lstate->mode == GL_COMPILE_AND_EXECUTE) 
	   __glim_TexGendv(gc, coord, pname, params);
	_count0 = __glTexGendv_size(pname);
	if (_count0 < 0) {
	    __gllc_Error(gc, -_count0);
	    return;
	}
	_cmdlen = 12+__GL_PAD(_count0*8);
	__GL_CHECK_SPACE(_cmdlen);
	PC = lstate->pc;
	if ((((int) PC) & 0x7) == 0) {
	    __GL_BEGIN(__glop_Nop);
	    __GL_END(4);
	}
	__GL_BEGIN(__glop_TexGendv);
	__GL_PUT_LONG(coord);
	__GL_PUT_LONG(pname);
	__GL_PUT_DOUBLE_ARRAY(params,_count0);
	__GL_END(_cmdlen);
	__GL_CHECK_SPACE(80);
}

void __gllc_TexGenf( __glContext *gc, GLenum coord, GLenum pname, GLfloat param)
{
    __glListMachine *lstate = &gc->list;
    GLbyte *PC=0;

	if (lstate->mode == GL_COMPILE_AND_EXECUTE) 
	   __glim_TexGenf(gc, coord, pname, param);
	__GL_BEGIN(__glop_TexGenf);
	__GL_PUT_LONG(coord);
	__GL_PUT_LONG(pname);
	__GL_PUT_FLOAT(param);
	__GL_END(16);
	__GL_CHECK_SPACE(80);
}

void __gllc_TexGenfv( __glContext *gc, GLenum coord, GLenum pname, const GLfloat *params)
{
    __glListMachine *lstate = &gc->list;
    GLbyte *PC=0;
    GLint _cmdlen=0, _count0=0;

	if (lstate->mode == GL_COMPILE_AND_EXECUTE) 
	   __glim_TexGenfv(gc, coord, pname, params);
	_count0 = __glTexGenfv_size(pname);
	if (_count0 < 0) {
	    __gllc_Error(gc, -_count0);
	    return;
	}
	_cmdlen = 12+__GL_PAD(_count0*4);
	__GL_CHECK_SPACE(_cmdlen);
	__GL_BEGIN(__glop_TexGenfv);
	__GL_PUT_LONG(coord);
	__GL_PUT_LONG(pname);
	__GL_PUT_FLOAT_ARRAY(params,_count0);
	__GL_END(_cmdlen);
	__GL_CHECK_SPACE(80);
}

void __gllc_TexGeni( __glContext *gc, GLenum coord, GLenum pname, GLint param)
{
    __glListMachine *lstate = &gc->list;
    GLbyte *PC=0;

	if (lstate->mode == GL_COMPILE_AND_EXECUTE) 
	   __glim_TexGeni(gc, coord, pname, param);
	__GL_BEGIN(__glop_TexGeni);
	__GL_PUT_LONG(coord);
	__GL_PUT_LONG(pname);
	__GL_PUT_LONG(param);
	__GL_END(16);
	__GL_CHECK_SPACE(80);
}

void __gllc_TexGeniv( __glContext *gc, GLenum coord, GLenum pname, const GLint *params)
{
    __glListMachine *lstate = &gc->list;
    GLbyte *PC=0;
    GLint _cmdlen=0, _count0=0;

	if (lstate->mode == GL_COMPILE_AND_EXECUTE) 
	   __glim_TexGeniv(gc, coord, pname, params);
	_count0 = __glTexGeniv_size(pname);
	if (_count0 < 0) {
	    __gllc_Error(gc, -_count0);
	    return;
	}
	_cmdlen = 12+__GL_PAD(_count0*4);
	__GL_CHECK_SPACE(_cmdlen);
	__GL_BEGIN(__glop_TexGeniv);
	__GL_PUT_LONG(coord);
	__GL_PUT_LONG(pname);
	__GL_PUT_LONG_ARRAY(params,_count0);
	__GL_END(_cmdlen);
	__GL_CHECK_SPACE(80);
}

void __gllc_InitNames( __glContext *gc )
{
    __glListMachine *lstate = &gc->list;
    GLbyte *PC=0;

	if (lstate->mode == GL_COMPILE_AND_EXECUTE) 
	   __glim_InitNames(gc);
	__GL_BEGIN(__glop_InitNames);
	__GL_END(4);
	__GL_CHECK_SPACE(80);
}

void __gllc_LoadName( __glContext *gc, GLuint name)
{
    __glListMachine *lstate = &gc->list;
    GLbyte *PC=0;

	if (lstate->mode == GL_COMPILE_AND_EXECUTE) 
	   __glim_LoadName(gc, name);
	__GL_BEGIN(__glop_LoadName);
	__GL_PUT_LONG(name);
	__GL_END(8);
	__GL_CHECK_SPACE(80);
}

void __gllc_PassThrough( __glContext *gc, GLfloat token)
{
    __glListMachine *lstate = &gc->list;
    GLbyte *PC=0;

	if (lstate->mode == GL_COMPILE_AND_EXECUTE) 
	   __glim_PassThrough(gc, token);
	__GL_BEGIN(__glop_PassThrough);
	__GL_PUT_FLOAT(token);
	__GL_END(8);
	__GL_CHECK_SPACE(80);
}

void __gllc_PopName( __glContext *gc )
{
    __glListMachine *lstate = &gc->list;
    GLbyte *PC=0;

	if (lstate->mode == GL_COMPILE_AND_EXECUTE) 
	   __glim_PopName(gc);
	__GL_BEGIN(__glop_PopName);
	__GL_END(4);
	__GL_CHECK_SPACE(80);
}

void __gllc_PushName( __glContext *gc, GLuint name)
{
    __glListMachine *lstate = &gc->list;
    GLbyte *PC=0;

	if (lstate->mode == GL_COMPILE_AND_EXECUTE) 
	   __glim_PushName(gc, name);
	__GL_BEGIN(__glop_PushName);
	__GL_PUT_LONG(name);
	__GL_END(8);
	__GL_CHECK_SPACE(80);
}

void __gllc_DrawBuffer( __glContext *gc, GLenum mode)
{
    __glListMachine *lstate = &gc->list;
    GLbyte *PC=0;

	if (lstate->mode == GL_COMPILE_AND_EXECUTE) 
	   __glim_DrawBuffer(gc, mode);
	__GL_BEGIN(__glop_DrawBuffer);
	__GL_PUT_LONG(mode);
	__GL_END(8);
	__GL_CHECK_SPACE(80);
}

void __gllc_Clear( __glContext *gc, GLbitfield mask)
{
    __glListMachine *lstate = &gc->list;
    GLbyte *PC=0;

	if (lstate->mode == GL_COMPILE_AND_EXECUTE) 
	   __glim_Clear(gc, mask);
	__GL_BEGIN(__glop_Clear);
	__GL_PUT_LONG(mask);
	__GL_END(8);
	__GL_CHECK_SPACE(80);
}

void __gllc_ClearAccum( __glContext *gc, GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha)
{
    __glListMachine *lstate = &gc->list;
    GLbyte *PC=0;

	if (lstate->mode == GL_COMPILE_AND_EXECUTE) 
	   __glim_ClearAccum(gc, red, green, blue, alpha);
	__GL_BEGIN(__glop_ClearAccum);
	__GL_PUT_FLOAT(red);
	__GL_PUT_FLOAT(green);
	__GL_PUT_FLOAT(blue);
	__GL_PUT_FLOAT(alpha);
	__GL_END(20);
	__GL_CHECK_SPACE(80);
}

void __gllc_ClearIndex( __glContext *gc, GLfloat c)
{
    __glListMachine *lstate = &gc->list;
    GLbyte *PC=0;

	if (lstate->mode == GL_COMPILE_AND_EXECUTE) 
	   __glim_ClearIndex(gc, c);
	__GL_BEGIN(__glop_ClearIndex);
	__GL_PUT_FLOAT(c);
	__GL_END(8);
	__GL_CHECK_SPACE(80);
}

void __gllc_ClearColor( __glContext *gc, GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha)
{
    __glListMachine *lstate = &gc->list;
    GLbyte *PC=0;

	if (lstate->mode == GL_COMPILE_AND_EXECUTE) 
	   __glim_ClearColor(gc, red, green, blue, alpha);
	__GL_BEGIN(__glop_ClearColor);
	__GL_PUT_FLOAT(red);
	__GL_PUT_FLOAT(green);
	__GL_PUT_FLOAT(blue);
	__GL_PUT_FLOAT(alpha);
	__GL_END(20);
	__GL_CHECK_SPACE(80);
}

void __gllc_ClearStencil( __glContext *gc, GLint s)
{
    __glListMachine *lstate = &gc->list;
    GLbyte *PC=0;

	if (lstate->mode == GL_COMPILE_AND_EXECUTE) 
	   __glim_ClearStencil(gc, s);
	__GL_BEGIN(__glop_ClearStencil);
	__GL_PUT_LONG(s);
	__GL_END(8);
	__GL_CHECK_SPACE(80);
}

void __gllc_ClearDepth( __glContext *gc, GLclampd depth)
{
    __glListMachine *lstate = &gc->list;
    GLbyte *PC=0;

	if (lstate->mode == GL_COMPILE_AND_EXECUTE) 
	   __glim_ClearDepth(gc, depth);
	PC = lstate->pc;
	if ((((int) PC) & 0x7) == 0) {
	    __GL_BEGIN(__glop_Nop);
	    __GL_END(4);
	}
	__GL_BEGIN(__glop_ClearDepth);
	__GL_PUT_DOUBLE(depth);
	__GL_END(12);
	__GL_CHECK_SPACE(80);
}

void __gllc_StencilMask( __glContext *gc, GLuint mask)
{
    __glListMachine *lstate = &gc->list;
    GLbyte *PC=0;

	if (lstate->mode == GL_COMPILE_AND_EXECUTE) 
	   __glim_StencilMask(gc, mask);
	__GL_BEGIN(__glop_StencilMask);
	__GL_PUT_LONG(mask);
	__GL_END(8);
	__GL_CHECK_SPACE(80);
}

void __gllc_ColorMask( __glContext *gc, GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha)
{
    __glListMachine *lstate = &gc->list;
    GLbyte *PC=0;

	if (lstate->mode == GL_COMPILE_AND_EXECUTE) 
	   __glim_ColorMask(gc, red, green, blue, alpha);
	__GL_BEGIN(__glop_ColorMask);
	__GL_PUT_CHAR(red);
	__GL_PUT_CHAR(green);
	__GL_PUT_CHAR(blue);
	__GL_PUT_CHAR(alpha);
	__GL_END(8);
	__GL_CHECK_SPACE(80);
}

void __gllc_DepthMask( __glContext *gc, GLboolean flag)
{
    __glListMachine *lstate = &gc->list;
    GLbyte *PC=0;

	if (lstate->mode == GL_COMPILE_AND_EXECUTE) 
	   __glim_DepthMask(gc, flag);
	__GL_BEGIN(__glop_DepthMask);
	__GL_PUT_CHAR(flag);
	__GL_PUT_PAD(3);
	__GL_END(8);
	__GL_CHECK_SPACE(80);
}

void __gllc_IndexMask( __glContext *gc, GLuint mask)
{
    __glListMachine *lstate = &gc->list;
    GLbyte *PC=0;

	if (lstate->mode == GL_COMPILE_AND_EXECUTE) 
	   __glim_IndexMask(gc, mask);
	__GL_BEGIN(__glop_IndexMask);
	__GL_PUT_LONG(mask);
	__GL_END(8);
	__GL_CHECK_SPACE(80);
}

void __gllc_Accum( __glContext *gc, GLenum op, GLfloat value)
{
    __glListMachine *lstate = &gc->list;
    GLbyte *PC=0;

	if (lstate->mode == GL_COMPILE_AND_EXECUTE) 
	   __glim_Accum(gc, op, value);
	__GL_BEGIN(__glop_Accum);
	__GL_PUT_LONG(op);
	__GL_PUT_FLOAT(value);
	__GL_END(12);
	__GL_CHECK_SPACE(80);
}

void __gllc_Disable( __glContext *gc, GLenum cap)
{
    __glListMachine *lstate = &gc->list;
    GLbyte *PC=0;

	if (lstate->mode == GL_COMPILE_AND_EXECUTE) 
	   __glim_Disable(gc, cap);
	__GL_BEGIN(__glop_Disable);
	__GL_PUT_LONG(cap);
	__GL_END(8);
	__GL_CHECK_SPACE(80);
}

void __gllc_Enable( __glContext *gc, GLenum cap)
{
    __glListMachine *lstate = &gc->list;
    GLbyte *PC=0;

	if (lstate->mode == GL_COMPILE_AND_EXECUTE) 
	   __glim_Enable(gc, cap);
	__GL_BEGIN(__glop_Enable);
	__GL_PUT_LONG(cap);
	__GL_END(8);
	__GL_CHECK_SPACE(80);
}

void __gllc_PopAttrib( __glContext *gc )
{
    __glListMachine *lstate = &gc->list;
    GLbyte *PC=0;

	if (lstate->mode == GL_COMPILE_AND_EXECUTE) 
	   __glim_PopAttrib(gc);
	__GL_BEGIN(__glop_PopAttrib);
	__GL_END(4);
	__GL_CHECK_SPACE(80);
}

void __gllc_PushAttrib( __glContext *gc, GLbitfield mask)
{
    __glListMachine *lstate = &gc->list;
    GLbyte *PC=0;

	if (lstate->mode == GL_COMPILE_AND_EXECUTE) 
	   __glim_PushAttrib(gc, mask);
	__GL_BEGIN(__glop_PushAttrib);
	__GL_PUT_LONG(mask);
	__GL_END(8);
	__GL_CHECK_SPACE(80);
}

void __gllc_MapGrid1d( __glContext *gc, GLint un, GLdouble u1, GLdouble u2)
{
    __glListMachine *lstate = &gc->list;
    GLbyte *PC=0;

	if (lstate->mode == GL_COMPILE_AND_EXECUTE) 
	   __glim_MapGrid1d(gc, un, u1, u2);
	PC = lstate->pc;
	if ((((int) PC) & 0x7) == 0) {
	    __GL_BEGIN(__glop_Nop);
	    __GL_END(4);
	}
	__GL_BEGIN(__glop_MapGrid1d);
	__GL_PUT_DOUBLE(u1);
	__GL_PUT_DOUBLE(u2);
	__GL_PUT_LONG(un);
	__GL_END(24);
	__GL_CHECK_SPACE(80);
}

void __gllc_MapGrid1f( __glContext *gc, GLint un, GLfloat u1, GLfloat u2)
{
    __glListMachine *lstate = &gc->list;
    GLbyte *PC=0;

	if (lstate->mode == GL_COMPILE_AND_EXECUTE) 
	   __glim_MapGrid1f(gc, un, u1, u2);
	__GL_BEGIN(__glop_MapGrid1f);
	__GL_PUT_LONG(un);
	__GL_PUT_FLOAT(u1);
	__GL_PUT_FLOAT(u2);
	__GL_END(16);
	__GL_CHECK_SPACE(80);
}

void __gllc_MapGrid2d( __glContext *gc, GLint un, GLdouble u1, GLdouble u2, GLint vn, GLdouble v1, GLdouble v2)
{
    __glListMachine *lstate = &gc->list;
    GLbyte *PC=0;

	if (lstate->mode == GL_COMPILE_AND_EXECUTE) 
	   __glim_MapGrid2d(gc, un, u1, u2, vn, v1, v2);
	PC = lstate->pc;
	if ((((int) PC) & 0x7) == 0) {
	    __GL_BEGIN(__glop_Nop);
	    __GL_END(4);
	}
	__GL_BEGIN(__glop_MapGrid2d);
	__GL_PUT_DOUBLE(u1);
	__GL_PUT_DOUBLE(u2);
	__GL_PUT_DOUBLE(v1);
	__GL_PUT_DOUBLE(v2);
	__GL_PUT_LONG(un);
	__GL_PUT_LONG(vn);
	__GL_END(44);
	__GL_CHECK_SPACE(80);
}

void __gllc_MapGrid2f( __glContext *gc, GLint un, GLfloat u1, GLfloat u2, GLint vn, GLfloat v1, GLfloat v2)
{
    __glListMachine *lstate = &gc->list;
    GLbyte *PC=0;

	if (lstate->mode == GL_COMPILE_AND_EXECUTE) 
	   __glim_MapGrid2f(gc, un, u1, u2, vn, v1, v2);
	__GL_BEGIN(__glop_MapGrid2f);
	__GL_PUT_LONG(un);
	__GL_PUT_FLOAT(u1);
	__GL_PUT_FLOAT(u2);
	__GL_PUT_LONG(vn);
	__GL_PUT_FLOAT(v1);
	__GL_PUT_FLOAT(v2);
	__GL_END(28);
	__GL_CHECK_SPACE(80);
}

void __gllc_EvalCoord1d( __glContext *gc, GLdouble u )
{
    __glListMachine *lstate = &gc->list;
    GLbyte *PC=0;

	if (lstate->mode == GL_COMPILE_AND_EXECUTE) 
	   __glim_EvalCoord1d(gc, u);
	PC = lstate->pc;
	if ((((int) PC) & 0x7) == 0) {
	    __GL_BEGIN(__glop_Nop);
	    __GL_END(4);
	}
	__GL_BEGIN(__glop_EvalCoord1dv);
	__GL_PUT_DOUBLE(u);
	__GL_END(12);
	__GL_CHECK_SPACE(80);
}

void __gllc_EvalCoord1dv( __glContext *gc, const GLdouble *u )
{
    __glListMachine *lstate = &gc->list;
    GLbyte *PC=0;

	if (lstate->mode == GL_COMPILE_AND_EXECUTE) 
	   __glim_EvalCoord1dv(gc, u);
	PC = lstate->pc;
	if ((((int) PC) & 0x7) == 0) {
	    __GL_BEGIN(__glop_Nop);
	    __GL_END(4);
	}
	__GL_BEGIN(__glop_EvalCoord1dv);
	__GL_PUT_DOUBLE(u[0]);
	__GL_END(12);
	__GL_CHECK_SPACE(80);
}

void __gllc_EvalCoord1f( __glContext *gc, GLfloat u )
{
    __glListMachine *lstate = &gc->list;
    GLbyte *PC=0;

	if (lstate->mode == GL_COMPILE_AND_EXECUTE) 
	   __glim_EvalCoord1f(gc, u);
	__GL_BEGIN(__glop_EvalCoord1fv);
	__GL_PUT_FLOAT(u);
	__GL_END(8);
	__GL_CHECK_SPACE(80);
}

void __gllc_EvalCoord1fv( __glContext *gc, const GLfloat *u )
{
    __glListMachine *lstate = &gc->list;
    GLbyte *PC=0;

	if (lstate->mode == GL_COMPILE_AND_EXECUTE) 
	   __glim_EvalCoord1fv(gc, u);
	__GL_BEGIN(__glop_EvalCoord1fv);
	__GL_PUT_FLOAT(u[0]);
	__GL_END(8);
	__GL_CHECK_SPACE(80);
}

void __gllc_EvalCoord2d( __glContext *gc, GLdouble u, GLdouble v )
{
    __glListMachine *lstate = &gc->list;
    GLbyte *PC=0;

	if (lstate->mode == GL_COMPILE_AND_EXECUTE) 
	   __glim_EvalCoord2d(gc, u, v);
	PC = lstate->pc;
	if ((((int) PC) & 0x7) == 0) {
	    __GL_BEGIN(__glop_Nop);
	    __GL_END(4);
	}
	__GL_BEGIN(__glop_EvalCoord2dv);
	__GL_PUT_DOUBLE(u);
	__GL_PUT_DOUBLE(v);
	__GL_END(20);
	__GL_CHECK_SPACE(80);
}

void __gllc_EvalCoord2dv( __glContext *gc, const GLdouble *u )
{
    __glListMachine *lstate = &gc->list;
    GLbyte *PC=0;

	if (lstate->mode == GL_COMPILE_AND_EXECUTE) 
	   __glim_EvalCoord2dv(gc, u);
	PC = lstate->pc;
	if ((((int) PC) & 0x7) == 0) {
	    __GL_BEGIN(__glop_Nop);
	    __GL_END(4);
	}
	__GL_BEGIN(__glop_EvalCoord2dv);
	__GL_PUT_DOUBLE(u[0]);
	__GL_PUT_DOUBLE(u[1]);
	__GL_END(20);
	__GL_CHECK_SPACE(80);
}

void __gllc_EvalCoord2f( __glContext *gc, GLfloat u, GLfloat v )
{
    __glListMachine *lstate = &gc->list;
    GLbyte *PC=0;

	if (lstate->mode == GL_COMPILE_AND_EXECUTE) 
	   __glim_EvalCoord2f(gc, u, v);
	__GL_BEGIN(__glop_EvalCoord2fv);
	__GL_PUT_FLOAT(u);
	__GL_PUT_FLOAT(v);
	__GL_END(12);
	__GL_CHECK_SPACE(80);
}

void __gllc_EvalCoord2fv( __glContext *gc, const GLfloat *u )
{
    __glListMachine *lstate = &gc->list;
    GLbyte *PC=0;

	if (lstate->mode == GL_COMPILE_AND_EXECUTE) 
	   __glim_EvalCoord2fv(gc, u);
	__GL_BEGIN(__glop_EvalCoord2fv);
	__GL_PUT_FLOAT(u[0]);
	__GL_PUT_FLOAT(u[1]);
	__GL_END(12);
	__GL_CHECK_SPACE(80);
}

void __gllc_EvalMesh1( __glContext *gc, GLenum mode, GLint i1, GLint i2 )
{
    __glListMachine *lstate = &gc->list;
    GLbyte *PC=0;

	if (lstate->mode == GL_COMPILE_AND_EXECUTE) 
	   __glim_EvalMesh1(gc, mode, i1, i2);
	__GL_BEGIN(__glop_EvalMesh1);
	__GL_PUT_LONG(mode);
	__GL_PUT_LONG(i1);
	__GL_PUT_LONG(i2);
	__GL_END(16);
	__GL_CHECK_SPACE(80);
}

void __gllc_EvalPoint1( __glContext *gc, GLint i )
{
    __glListMachine *lstate = &gc->list;
    GLbyte *PC=0;

	if (lstate->mode == GL_COMPILE_AND_EXECUTE) 
	   __glim_EvalPoint1(gc, i);
	__GL_BEGIN(__glop_EvalPoint1);
	__GL_PUT_LONG(i);
	__GL_END(8);
	__GL_CHECK_SPACE(80);
}

void __gllc_EvalMesh2( __glContext *gc, GLenum mode, GLint i1, GLint i2, GLint j1, GLint j2 )
{
    __glListMachine *lstate = &gc->list;
    GLbyte *PC=0;

	if (lstate->mode == GL_COMPILE_AND_EXECUTE) 
	   __glim_EvalMesh2(gc, mode, i1, i2, j1, j2);
	__GL_BEGIN(__glop_EvalMesh2);
	__GL_PUT_LONG(mode);
	__GL_PUT_LONG(i1);
	__GL_PUT_LONG(i2);
	__GL_PUT_LONG(j1);
	__GL_PUT_LONG(j2);
	__GL_END(24);
	__GL_CHECK_SPACE(80);
}

void __gllc_EvalPoint2( __glContext *gc, GLint i, GLint j)
{
    __glListMachine *lstate = &gc->list;
    GLbyte *PC=0;

	if (lstate->mode == GL_COMPILE_AND_EXECUTE) 
	   __glim_EvalPoint2(gc, i, j);
	__GL_BEGIN(__glop_EvalPoint2);
	__GL_PUT_LONG(i);
	__GL_PUT_LONG(j);
	__GL_END(12);
	__GL_CHECK_SPACE(80);
}

void __gllc_AlphaFunc( __glContext *gc, GLenum func, GLclampf ref )
{
    __glListMachine *lstate = &gc->list;
    GLbyte *PC=0;

	if (lstate->mode == GL_COMPILE_AND_EXECUTE) 
	   __glim_AlphaFunc(gc, func, ref);
	__GL_BEGIN(__glop_AlphaFunc);
	__GL_PUT_LONG(func);
	__GL_PUT_FLOAT(ref);
	__GL_END(12);
	__GL_CHECK_SPACE(80);
}

void __gllc_BlendFunc( __glContext *gc, GLenum sfactor, GLenum dfactor )
{
    __glListMachine *lstate = &gc->list;
    GLbyte *PC=0;

	if (lstate->mode == GL_COMPILE_AND_EXECUTE) 
	   __glim_BlendFunc(gc, sfactor, dfactor);
	__GL_BEGIN(__glop_BlendFunc);
	__GL_PUT_LONG(sfactor);
	__GL_PUT_LONG(dfactor);
	__GL_END(12);
	__GL_CHECK_SPACE(80);
}

void __gllc_LogicOp( __glContext *gc, GLenum opcode)
{
    __glListMachine *lstate = &gc->list;
    GLbyte *PC=0;

	if (lstate->mode == GL_COMPILE_AND_EXECUTE) 
	   __glim_LogicOp(gc, opcode);
	__GL_BEGIN(__glop_LogicOp);
	__GL_PUT_LONG(opcode);
	__GL_END(8);
	__GL_CHECK_SPACE(80);
}

void __gllc_StencilFunc( __glContext *gc, GLenum func, GLint ref, GLuint mask)
{
    __glListMachine *lstate = &gc->list;
    GLbyte *PC=0;

	if (lstate->mode == GL_COMPILE_AND_EXECUTE) 
	   __glim_StencilFunc(gc, func, ref, mask);
	__GL_BEGIN(__glop_StencilFunc);
	__GL_PUT_LONG(func);
	__GL_PUT_LONG(ref);
	__GL_PUT_LONG(mask);
	__GL_END(16);
	__GL_CHECK_SPACE(80);
}

void __gllc_StencilOp( __glContext *gc, GLenum fail, GLenum zfail, GLenum zpass)
{
    __glListMachine *lstate = &gc->list;
    GLbyte *PC=0;

	if (lstate->mode == GL_COMPILE_AND_EXECUTE) 
	   __glim_StencilOp(gc, fail, zfail, zpass);
	__GL_BEGIN(__glop_StencilOp);
	__GL_PUT_LONG(fail);
	__GL_PUT_LONG(zfail);
	__GL_PUT_LONG(zpass);
	__GL_END(16);
	__GL_CHECK_SPACE(80);
}

void __gllc_DepthFunc( __glContext *gc, GLenum func)
{
    __glListMachine *lstate = &gc->list;
    GLbyte *PC=0;

	if (lstate->mode == GL_COMPILE_AND_EXECUTE) 
	   __glim_DepthFunc(gc, func);
	__GL_BEGIN(__glop_DepthFunc);
	__GL_PUT_LONG(func);
	__GL_END(8);
	__GL_CHECK_SPACE(80);
}

void __gllc_PixelZoom( __glContext *gc, GLfloat xfactor, GLfloat yfactor)
{
    __glListMachine *lstate = &gc->list;
    GLbyte *PC=0;

	if (lstate->mode == GL_COMPILE_AND_EXECUTE) 
	   __glim_PixelZoom(gc, xfactor, yfactor);
	__GL_BEGIN(__glop_PixelZoom);
	__GL_PUT_FLOAT(xfactor);
	__GL_PUT_FLOAT(yfactor);
	__GL_END(12);
	__GL_CHECK_SPACE(80);
}

void __gllc_PixelTransferf( __glContext *gc, GLenum pname, GLfloat param)
{
    __glListMachine *lstate = &gc->list;
    GLbyte *PC=0;

	if (lstate->mode == GL_COMPILE_AND_EXECUTE) 
	   __glim_PixelTransferf(gc, pname, param);
	__GL_BEGIN(__glop_PixelTransferf);
	__GL_PUT_LONG(pname);
	__GL_PUT_FLOAT(param);
	__GL_END(12);
	__GL_CHECK_SPACE(80);
}

void __gllc_PixelTransferi( __glContext *gc, GLenum pname, GLint param)
{
    __glListMachine *lstate = &gc->list;
    GLbyte *PC=0;

	if (lstate->mode == GL_COMPILE_AND_EXECUTE) 
	   __glim_PixelTransferi(gc, pname, param);
	__GL_BEGIN(__glop_PixelTransferi);
	__GL_PUT_LONG(pname);
	__GL_PUT_LONG(param);
	__GL_END(12);
	__GL_CHECK_SPACE(80);
}

void __gllc_PixelMapfv( __glContext *gc, GLenum map, GLsizei mapsize, const GLfloat *values)
{
    __glListMachine *lstate = &gc->list;
    GLbyte *PC=0;
    GLint _cmdlen=0, _count0=0;

	if (lstate->mode == GL_COMPILE_AND_EXECUTE) 
	   __glim_PixelMapfv(gc, map, mapsize, values);
	_count0 = mapsize;
	if (_count0 < 0) {
	    __gllc_Error(gc, -_count0);
	    return;
	}
	_cmdlen = 12+__GL_PAD(_count0*4);
	__GL_CHECK_SPACE(_cmdlen);
	__GL_BEGIN(__glop_PixelMapfv);
	__GL_PUT_LONG(map);
	__GL_PUT_LONG(mapsize);
	__GL_PUT_FLOAT_ARRAY(values,_count0);
	__GL_END(_cmdlen);
	__GL_CHECK_SPACE(80);
}

void __gllc_PixelMapuiv( __glContext *gc, GLenum map, GLsizei mapsize, const GLuint *values)
{
    __glListMachine *lstate = &gc->list;
    GLbyte *PC=0;
    GLint _cmdlen=0, _count0=0;

	if (lstate->mode == GL_COMPILE_AND_EXECUTE) 
	   __glim_PixelMapuiv(gc, map, mapsize, values);
	_count0 = mapsize;
	if (_count0 < 0) {
	    __gllc_Error(gc, -_count0);
	    return;
	}
	_cmdlen = 12+__GL_PAD(_count0*4);
	__GL_CHECK_SPACE(_cmdlen);
	__GL_BEGIN(__glop_PixelMapuiv);
	__GL_PUT_LONG(map);
	__GL_PUT_LONG(mapsize);
	__GL_PUT_LONG_ARRAY(values,_count0);
	__GL_END(_cmdlen);
	__GL_CHECK_SPACE(80);
}

void __gllc_PixelMapusv( __glContext *gc, GLenum map, GLsizei mapsize, const GLushort *values)
{
    __glListMachine *lstate = &gc->list;
    GLbyte *PC=0;
    GLint _cmdlen=0, _count0=0;

	if (lstate->mode == GL_COMPILE_AND_EXECUTE) 
	   __glim_PixelMapusv(gc, map, mapsize, values);
	_count0 = mapsize;
	if (_count0 < 0) {
	    __gllc_Error(gc, -_count0);
	    return;
	}
	_cmdlen = 12+__GL_PAD(_count0*2);
	__GL_CHECK_SPACE(_cmdlen);
	__GL_BEGIN(__glop_PixelMapusv);
	__GL_PUT_LONG(map);
	__GL_PUT_LONG(mapsize);
	__GL_PUT_SHORT_ARRAY(values,_count0);
	__GL_END(_cmdlen);
	__GL_CHECK_SPACE(80);
}

void __gllc_ReadBuffer( __glContext *gc, GLenum mode)
{
    __glListMachine *lstate = &gc->list;
    GLbyte *PC=0;

	if (lstate->mode == GL_COMPILE_AND_EXECUTE) 
	   __glim_ReadBuffer(gc, mode);
	__GL_BEGIN(__glop_ReadBuffer);
	__GL_PUT_LONG(mode);
	__GL_END(8);
	__GL_CHECK_SPACE(80);
}

void __gllc_CopyPixels( __glContext *gc, GLint x, GLint y, GLsizei width, GLsizei height, GLenum type)
{
    __glListMachine *lstate = &gc->list;
    GLbyte *PC=0;

	if (lstate->mode == GL_COMPILE_AND_EXECUTE) 
	   __glim_CopyPixels(gc, x, y, width, height, type);
	__GL_BEGIN(__glop_CopyPixels);
	__GL_PUT_LONG(x);
	__GL_PUT_LONG(y);
	__GL_PUT_LONG(width);
	__GL_PUT_LONG(height);
	__GL_PUTpixel ÛC  #vertexArray ÊH  #¿ Çz  pz  ‹z  
|  __glVertexMachineRec ‹HMaterialNeeds K,  # TransformGroupCurrent   #TransformGroup 
|  #TransformerCacheAge |  #TransformerCacheCode |  #PTransformerCacheGroup "|  #êVapiNeeds K,  #–XformNeeds  K,  #‘CacheAllocation !›<  #ÿ |     "|  K,   .|  ›<   Q}  __glTransformMachineRec Ë'NmaxWindowDimension Ng,  # modelViewStack OQ}  #modelView Pˇ}  #Ñ1projectionStack Q~  #à1projection Rˇ}  #∞@projectionSequence SK,  #¥@textureStack T~  #∏@texture Uˇ}  #‡OmatrixIsIdent V%  #‰O_padding_27e5 W5  #ÂO ]}  ]}    }  __glTransformRec ƒMmatrix F }  # inverseTranspose G }  #@mvp H }  #Äsequence IK,  #¿ Û}  __glMatrixRec @Lmatrix AÛ}  #  ˇ}  Á$   ]}  ~  ]}  	 Ñ~  __glListMachineRec Sshared x  # openList y0  #pc zˆ  #mode {g,  #nesting |g,  # Ê~  __glListMachineSharedRec RhashBuckets qí  # allocated rÍ  #refcount sg,  # 0  __glListRec Qindex jK,  # next k0  #listdata lå  # Ê~  Ä  __glListItemRec Psize cg,  # space dg,  #data eÄ  # å  g,   6  0  Í  __glListAllocationRec Onext \Í  # start ]K,  #number ^g,  # ò  Ñ~  %  mÄ  __glEvaluatorMachineRec ‘Keval1 9mÄ  # eval2 :ŒÄ  #êeval1Data ;fÅ  #åeval2Data <fÅ  #∞ yÄ  yÄ   ŒÄ  __glEvaluator1Rec Ik &g,  # order 'g,  #u1 (Á$  #u2 )Á$  # ⁄Ä  ⁄Ä   fÅ  __glEvaluator2Rec Jk .g,  # majorOrder /g,  #minorOrder 0g,  #u1 1Á$  #u2 2Á$  #v1 3Á$  #v2 4Á$  # rÅ  âv   Ç  __glFeedbackMachineRec ToverFlowed Å%  # _padding_001 Ç5  #resultBase Éâv  #result Ñâv  #resultLength Ög,  #type Üg,  # YÇ  __glLineMachineRec UstipplePosition ãg,  # repeat åg,  # QÉ  __glSelectMachineRec  Whit f%  # _padding_001 g5  #stack hQÉ  #sp irÉ  #ÑoverFlowed j%  #à_padding_209 k5  #âresultBase lrÉ  #åresult mrÉ  #êresultLength nxÉ  #îhits oxÉ  #òz prÉ  #ú ]É  ]É   
long unsigned int ]É  
long int 4Ñ  __glLightMachineRec –	;source å4Ñ  # material ç¶Ü  #Äsources é.á  #†List è4á  #§NumberActive êg,  #ƒUseSlow ëg,  #»_padding_9cc í>  #Ã @Ñ  @Ñ   ¶Ü  __glLightSourceMachineRec 09AmbientR fÇ-  # AmbientG gÇ-  #AmbientB hÇ-  # SpecularR iÇ-  #0SpecularG jÇ-  #@SpecularB kÇ-  #PDiffuseR lÇ-  #`DiffuseG mÇ-  #pDiffuseB nÇ-  #ÄHatX oÇ-  #êHatY pÇ-  #†HatZ qÇ-  #∞PpliHatX rÇ-  #¿PpliHatY sÇ-  #–PpliHatZ tÇ-  #‡ConstantAttenuation uÁ$  #LinearAttenuation vÁ$  #ÙQuadraticAttenuation wÁ$  #¯SpotLightExponent xÁ$  #¸Position y5  #ÄDirection z5  #êCosCutOffAngle {Á$  #†Attenuation |Á$  #§List }g,  #®NoSpot ~%  #¨_padding_12d 5  #≠ ≤Ü  ≤Ü   .á  __glMaterialMachineRec :SceneColorR ÑÁ$  # SceneColorG ÖÁ$  #SceneColorB ÜÁ$  #SceneColorA áÁ$  # @Ñ  @á  g,   Èà  __glVertexArrayMachineRec †XvertexCall uıà  # normalCall vıà  #colorCall wıà  #indexCall xıà  #texCoordCall yıà  #edgeFlagCall zıà  #vertexSkip {g,  #normalSkip |g,  #colorSkip }g,  # indexSkip ~g,  #$texCoordSkip >  #(edgeFlagSkip Äg,  #8drawElementCodeCache Å˚à  #<drawElementNeeds Çâ  #\drawElementAge Éâ  #|drawElementCurrentNeeds ÑK,  #ú ıà  ΩK   Èà  â  ◊<   â  K,   (ä  __glSoftwareScanlineProcsRec YscanlineDepth âNä  # loadBits äK,  #storeBits ãK,  #valid å%  #maskEnabled ç%  #colorWriteEnabled é%  #fastScanline è%  #processorFuncData ê›<  #processorFunc ëæé  #processorNeeds íK,  # Hä  2y  Hä  e  e  í   e  (ä  oä  ˙  oä  zä  í   uä  ¸<  Ää  ±é  __glShadeRec ‰CmmxDB ∂±é  # mmxDG ∑±é  #mmxDR ∏±é  #mmxDA π±é  #xLeft ∫Á$  #xRight ªÁ$  #xLeftFixed ºg,  #xRightFixed Ωg,  #dxdyLeft æÁ$  #dxdyRight øÁ$  #yBottom ¿Á$  # yTop ¡Á$  #$dy ¬Á$  #(iyBottom √g,  #,iyTop ƒg,  #0area ≈Á$  #4dxAC ∆Á$  #8dxBC «Á$  #<dyAC »Á$  #@dyBC …Á$  #DdxdyLeftFixed  g,  #HdxdyRightFixed Àg,  #Lr0 ÃÁ$  #Pg0 ÕÁ$  #Tb0 ŒÁ$  #Xa0 œÁ$  #\drdx –Á$  #`dgdx —Á$  #ddbdx “Á$  #hdadx ”Á$  #ldrdy ‘Á$  #pdgdy ’Á$  #tdbdy ÷Á$  #xdady ◊Á$  #|drdxdy ÿÁ$  #Ädgdxdy ŸÁ$  #Ñdbdxdy ⁄Á$  #àdadxdy €Á$  #åz0 ‹Á$  #êdzdy ›Á$  #îdzdx ﬁÁ$  #òdzdxdy ﬂÁ$  #ús0 ‡Á$  #†t0 ·Á$  #§qw0 ‚Á$  #®dsdx „Á$  #¨dtdx ‰Á$  #∞dqwdx ÂÁ$  #¥dsdy ÊÁ$  #∏dtdy ÁÁ$  #ºdqwdy ËÁ$  #¿dsdxdy ÈÁ$  #ƒdtdxdy ÍÁ$  #»dqwdxdy ÎÁ$  #Ãf0 ÏÁ$  #–dfdy ÌÁ$  #‘dfdx ÓÁ$  #ÿdfdxdy ÔÁ$  #‹ccw %  #‡_padding_0e1 Ò5  #· 
short int Tä  åê  __glInfoRec <astate_h_version ãK,  # textureMaxS åK,  #textureMaxT çK,  #textureMaxR éK,  #vendor èˆ  #renderer êˆ  #version ëˆ  #extensions íˆ  #hwRenderer ìˆ  # hwGeometry îˆ  #$comboStrings ïåê  #(debugLock ñπ3  #4debugDevices óπ3  #5debugDriver òπ3  #6debugModes ôπ3  #7debugOther öπ3  #8debugDisableDCLock õπ3  #9debugDisableTL úπ3  #:_padding_03b ùB  #; òê  ˆ   Âê  __glWindowStateRec bUtilThreadSem ¢g,  # DirectWindow £›<  # 
int32 ë  log_of   val   res    •ë  calc_PP_TXFORMAT `  Äï  hñ  Ugc _˙  ëtex _à=  Vbits a  Pàë  Óê  :ñ  Iñ  ë  Rë  Q 	Óê  Iñ  Yñ  ë  Rë  Q  Ìí  Radeon_TexParameter óhñ   ò  Ugc ñ˙  ëtex ñà=  Pcon ò¶≠  Vrt ô<Æ  ë|<í  ¡ñ  Òñ  buf •ªñ  	BÆ  ¡ñ  Ÿñ  _Æ  ë iÆ  vÆ  W  xí  "ó  7ó  buf ´ªñ  	BÆ  "ó  7ó  _Æ  ë iÆ  vÆ  W  ¥í  ró  áó  buf Æªñ  	BÆ  ró  áó  _Æ  ë iÆ  vÆ  W  æó  ˆó  buf ±ªñ  	BÆ  æó  ”ó  _Æ  ë iÆ  vÆ  W   ˛ï  _rd_Context_rec •fifo ˛ï  # devfd Âê  #<ci_area Õñ  #@ci ˚™  #Dma f≠  #Hbuf_Color l≠  #Lbuf_Depth ¢ù  #Tbuf_VWidth Âê  #Xbuf_VHeight Âê  #\buf_ColorStride  Âê  #`buf_DepthStride !Âê  #dbuf_IsFullScreen "Âê  #hbuf_CurrentFront #Âê  #lbuf_BlitCount $Âê  #pbuf_IsTiled %Âê  #tbuf_HWColorType &  #xbuf_HWDepthType '  #|buf_BytesPerPixel (Âê  #Äbuf_BytesPerPixelLog2 )Âê  #Ñbuf_SingleBufferMode +Âê  #àbuf_ZCompAvailable -Âê  #åbuf_ZDecompEnabled .Âê  #êbuf_ZCompEnabled /Âê  #îtcl_Enabled 3x≠  #òtcl_Color 4  #ôtcl_Normal 5Ä≠  #ùtcl_TexCoord 6å≠  #©tcl_Hdr 7  #Ÿtcl_VxCount 8Âê  #›tcx_WritePtr 9  #·tcl_localLight ;ö≠  #Âreg >Kû  #ÌoldReg ?Kû  #… Øñ  Radeon_FifoRec <buf Øñ  # hwBuf ¡ñ  #writePtr ªñ  #free Âê  #bufNum Âê  # seqNum ¡ñ  #$size Âê  #0useInd Âê  #4timesEmptied Âê  #8 ªñ  ªñ     Õñ     
area_id Tö  CardInfoRec nCardMemory ñTö  # CardRegs óTö  #IO_Base ò  #DMA_Base ôTö  #pciMemBase öcö  #scratch õjö  #FBStride ùÂê  #hwBppNum ûÂê  #pll üuö  # xres °Âê  #Ryres ¢Âê  #Vbitpp £Âê  #Zbytepp §Âê  #^AllocCursor ¶¢ù  #bAllocFrameBuffer ß¢ù  #fbenEngineInt ©Âê  #jbenEngineSem ™®ù  #nprimitivesIssued ¨  #rbufferSyncNum ≠  #vshowCursor ØÂê  #zRingBuf ≤≤ù  #~CCEBMFlag ≥Zö  #ñCCEAGPFlag ¥Zö  #óCCEVBFlag µZö  #òCCEIBFlag ∂Zö  #ôCCERequestedMode ∑  #öCSQmodeIndex ∏Âê  #ûCSQPrimary πÂê  #¢CSQIndirect ∫Âê  #¶bm_save_state ª  #™read_ptr_offset º  #Æreadbuf Ω?û  #≤mc_Regs ¿Kû  #÷mc_Users ¡Âê  #≤mc_ForceReset ¬Âê  #∂mc_LastID √  #∫CPSubmitPackets ∆´  #æCPIndirectSubmitPackets «´  #¬Flush »´  #∆Finish …´  # devname À´  #Œ Zö  
uint8 iö  !pö  "  #éú  2qclock_chip_type [Zö  # struct_size \Zö  #acclerator_entry ]Zö  #VGA_entry ^Zö  #VGA_table_offset _éú  #POST_table_offset `éú  #XCLK aéú  #MCLK béú  #
num_PLL_blocks cZö  #size_PLL_blocks dZö  #PCLK_ref_freq eéú  #PCLK_ref_divider féú  #PCLK_min_freq g  #PCLK_max_freq h  #MCLK_ref_freq iéú  #MCLK_ref_divider jéú  #MCLK_min_freq k  #MCLK_max_freq l  #"XCLK_ref_freq méú  #&XCLK_ref_divider néú  #(XCLK_min_freq o  #*XCLK_max_freq p  #. 
uint16 ¢ù  __mem_AllocationRec 4address   # usr_ui1   #usr_ui2   #usr_vp1 ›<  #usr_vp2 ›<  #start Âê  #end  Âê  #id !  #value "Á$  # priority #Á$  #$lastBound $  #(index %Âê  #,locked &Zö  #0state 'Zö  #1 òú  
sem_id ?û  tagRBINFO 'ReadIndexPtr (jö  # ReadPtrPhysical )  #WriteIndex *  #LinearPtr +ªñ  #Offset ,  #Size -  # Kû  Zö  # »™  _rd_Context_Regs_rec \
r_RB3D_DEPTHOFFSET   # r_RB3D_DEPTHPITCH   #r_RB3D_ZSTENCILCNTL   #r_RB3D_STENCILREFMASK   #r_RB3D_COLOROFFSET   #r_RB3D_COLORPITCH    #r_RB3D_BLENDCNTL !  #r_RB3D_CNTL "  #r_SE_CNTL #  # r_PP_MISC $  #$r_PP_CNTL %  #(r_ISYNC_CNTL &  #,r_PP_FOG_COLOR '  #0r_RE_SOLID_COLOR (  #4r_RE_WIDTH_HEIGHT )  #8r_PP_TXFILTER_0 *  #<r_PP_TXFORMAT_0 +  #@r_PP_TXOFFSET_0 ,  #Dr_PP_TXCBLEND_0 -  #Hr_PP_TXABLEND_0 .  #Lr_PP_TFACTOR_0 /  #Pr_PP_TXFILTER_1 0  #Tr_PP_TXFORMAT_1 1  #Xr_PP_TXOFFSET_1 2  #\r_PP_TXCBLEND_1 3  #`r_PP_TXABLEND_1 4  #dr_PP_TFACTOR_1 5  #hr_PP_TXFILTER_2 6  #lr_PP_TXFORMAT_2 7  #pr_PP_TXOFFSET_2 8  #tr_PP_TXCBLEND_2 9  #xr_PP_TXABLEND_2 :  #|r_PP_TFACTOR_2 ;  #Är_RE_LINE_PATTERN =  #Ñr_RE_LINE_STATE >  #àr_PP_LUM_MATRIX ?  #år_PP_CUBIC_FACES_0 @  #êr_PP_CUBIC_FACES_1 A  #îr_PP_CUBIC_FACES_2 B  #òr_PP_BORDER_COLOR_0 D  #úr_PP_BORDER_COLOR_1 E  #†r_PP_BORDER_COLOR_2 F  #§r_PP_ROT_MATRIX_0 H  #®r_PP_ROT_MATRIX_1 I  #¨r_RB3D_ROPCNTL K  #∞r_RB3D_PLANEMASK L  #¥r_SE_LINE_WIDTH N  #∏r_PP_CUBIC_OFFSET_T0_0 P  #ºr_PP_CUBIC_OFFSET_T0_1 Q  #¿r_PP_CUBIC_OFFSET_T0_2 R  #ƒr_PP_CUBIC_OFFSET_T0_3 S  #»r_PP_CUBIC_OFFSET_T0_4 T  #Ãr_PP_CUBIC_OFFSET_T1_0 U  #–r_PP_CUBIC_OFFSET_T1_1 V  #‘r_PP_CUBIC_OFFSET_T1_2 W  #ÿr_PP_CUBIC_OFFSET_T1_3 X  #‹r_PP_CUBIC_OFFSET_T1_4 Y  #‡r_PP_CUBIC_OFFSET_T2_0 Z  #‰r_PP_CUBIC_OFFSET_T2_1 [  #Ër_PP_CUBIC_OFFSET_T2_2 \  #Ïr_PP_CUBIC_OFFSET_T2_3 ]  #r_PP_CUBIC_OFFSET_T2_4 ^  #Ùr_SE_TCL_OUTPUT_VTX_FMT `  #¯r_SE_TCL_OUTPUT_VTX_SEL a  #¸r_SE_TCL_MATRIX_SELECT_0 b  #Är_SE_TCL_MATRIX_SELECT_1 c  #Ñr_SE_TCL_UCP_VERT_BLEND_CTL e  #àr_SE_TCL_TEXTURE_PROC_CTL f  #år_RE_TOP_LEFT h  #êr_RE_MISC i  #îr_SE_COORD_FMT j  #òr_SE_TCL_MATERIAL_AMBIENT_RED l  #úr_SE_TCL_MATERIAL_AMBIENT_GREEN m  #†r_SE_TCL_MATERIAL_AMBIENT_BLUE n  #§r_SE_TCL_MATERIAL_AMBIENT_ALPHA o  #®r_SE_TCL_MATERIAL_DIFFUSE_RED q  #¨r_SE_TCL_MATERIAL_DIFFUSE_GREEN r  #∞r_SE_TCL_MATERIAL_DIFFUSE_BLUE s  #¥r_SE_TCL_MATERIAL_DIFFUSE_ALPHA t  #∏r_SE_TCL_MATERIAL_SPECULAR_RED v  #ºr_SE_TCL_MATERIAL_SPECULAR_GREEN w  #¿r_SE_TCL_MATERIAL_SPECULAR_BLUE x  #ƒr_SE_TCL_MATERIAL_SPECULAR_ALPHA y  #»r_SE_TCL_MATERIAL_EMISSIVE_RED {  #Ãr_SE_TCL_MATERIAL_EMISSIVE_GREEN |  #–r_SE_TCL_MATERIAL_EMISSIVE_BLUE }  #‘r_SE_TCL_MATERIAL_EMISSIVE_ALPHA ~  #ÿr_SE_TCL_SHININESS Ä  #‹r_SE_TCL_PER_LIGHT_CTL_0 Å  #‡r_SE_TCL_PER_LIGHT_CTL_1 Ç  #‰r_SE_TCL_PER_LIGHT_CTL_2 É  #Ër_SE_TCL_PER_LIGHT_CTL_3 Ñ  #Ïr_SE_TCL_LIGHT_MODEL_CTL Ü  #r_SE_VPORT_XSCALE à  #Ùr_SE_VPORT_XOFFSET â  #¯r_SE_VPORT_YSCALE ä  #¸r_SE_VPORT_YOFFSET ã  #Är_SE_VPORT_ZSCALE å  #Ñr_SE_VPORT_ZOFFSET ç  #àvector è»™  #åscaler ê’™  #å ’™  Á$  ˇ ·™  Á$  3 ˚™  Âê  ˚™  ªñ     ÿñ  ·™  ´  ˚™   ´  %´  %   ~´  __mem_AreaDefRec Kdevfd LÂê  # area M`≠  #memID NÂê  #clone OÕñ  # $É¨  __mem_AreaRec ® 8areaID 9Õñ  # pageMap :É¨  #pageIDs ;É¨  #allocs <¢ù  #clock =  #pageCount >Âê  #allocCount ?Âê  #pageSize @Âê  #baseOffset A  # listLock Câ¨  #$memIDs Eí¨  #(lost Fû¨  #happs G ≠  #ËÅ Âê  
jlock û¨  Âê   ™¨  ™¨   Û¨  __mem_LostListRec +num ,Âê  # list -Û¨  #used .Zö  #Ñ   ≠  ¢ù  ˇ ≠  ≠   U≠  __mem_AppNotifyRec 2start 3®ù  # end 4®ù  #team 5U≠  # 
team_id ~´  %´  x≠  ¢ù   
int8 å≠  Á$   ö≠  Á$   ¶≠  x≠   Ìí  <Æ  Radeon_TextureRec 	rasTex 	à=  # alloc 	¢ù  #size 	
Âê  #base_PP_TXFILTER 	  #base_PP_TXFORMAT 	  #is32 	x≠  # ¨≠  ÇÆ  FIFO_GET_BUFFER Hªñ  gc GÇÆ  space GáÆ  con I¶≠   ˙    R∞  Radeon_TexEnvMode ∏ ò  %õ  Ugc ∑˙  ëcon π¶≠  Vcbits ª  ë|abits º  ëx'Ø  …ò  ˛ò  buf ‘ªñ  	BÆ  …ò  ·ò  _Æ  ë iÆ  vÆ  W  cØ  ô  "ô  buf ’ªñ  	BÆ  ô  "ô  _Æ  ë iÆ  vÆ  W  üØ  πô  Óô  buf Ïªñ  	BÆ  πô  —ô  _Æ  ë iÆ  vÆ  W  €Ø  ˝ô  ö  buf Ìªñ  	BÆ  ˝ô  ö  _Æ  ë iÆ  vÆ  W  ∞  ©ö  ﬁö  %buf ªñ  	BÆ  ©ö  ¡ö  _Æ  ë iÆ  vÆ  W  Èö  õ  %buf ªñ  	BÆ  Èö  ˛ö  _Æ  ë iÆ  vÆ  W   &\±  Radeon_TexEnvColor (õ  \ú  U'gc 
˙  ë(con ¶≠  V(color   W„∞  çõ  ®õ  %buf ªñ  	BÆ  çõ  ®õ  _Æ  ë iÆ  vÆ  ë|  !±  ÿõ  Ûõ  %buf ªñ  	BÆ  ÿõ  Ûõ  _Æ  ë iÆ  vÆ  ëx  ú  Rú  %buf ªñ  	BÆ  ú  5ú  _Æ  ë iÆ  vÆ  ët   )◊≤  loadTextureTiles16 á  \ú  ‚ù  U'gc Ü˙  ë'dest Ü  ë'l Üj  Q'x1 ÜÂê  P'x2 ÜÂê  ë|'y ÜÂê  W(con à¶≠  ëx(src äTö  ët(lstride ãÂê  ëp(tstride ãÂê  V(tx åÂê  Q(ty åÂê  R(d çªñ  R(s éªñ  V(x èÂê  ël%tilesWide êÂê  (size ëÂê  ëX(dmaSrc í  ëh	◊≤  »ú  ù  Ú≤  ë¸≤  ëX*≥  *≥  	BÆ  »ú  ù  +iÆ  _Æ  ë*vÆ     &≥  writeLargeNOP ªñ  gc ÇÆ  payloadSize 