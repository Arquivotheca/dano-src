/************************************************************
*  validate.c
*
*  Contains routines for calculating state dependent data.
*
*************************************************************/

#include "context.h"
#include "global.h"
#include "immed.h"
#include "mathLib.h"
#include "processor.h"

#include "prim_processor_parts.h"

#if __PROCESSOR_USE_C__
_STATIC_ void __expandMatrix( __glMatrixSIMD *to, GLfloat *from )
{
	to[0] = from[0];
	to[4] = from[1];
	to[8] = from[2];
	to[12] = from[3];
	to[16] = from[4];
	to[20] = from[5];
	to[24] = from[6];
	to[28] = from[7];
	to[32] = from[8];
	to[36] = from[9];
	to[40] = from[10];
	to[44] = from[11];
	to[48] = from[12];
	to[52] = from[13];
	to[56] = from[14];
	to[60] = from[15];

#if __PROCESSOR_KATMAI__
	to[1] = from[0];
	to[2] = from[0];
	to[3] = from[0];
	to[5] = from[1];
	to[6] = from[1];
	to[7] = from[1];
	to[9] = from[2];
	to[10] = from[2];
	to[11] = from[2];
	to[13] = from[3];
	to[14] = from[3];
	to[15] = from[3];
	to[17] = from[4];
	to[18] = from[4];
	to[19] = from[4];
	to[21] = from[5];
	to[22] = from[5];
	to[23] = from[5];
	to[25] = from[6];
	to[26] = from[6];
	to[27] = from[6];
	to[29] = from[7];
	to[30] = from[7];
	to[31] = from[7];
	to[33] = from[8];
	to[34] = from[8];
	to[35] = from[8];
	to[37] = from[9];
	to[38] = from[9];
	to[39] = from[9];
	to[41] = from[10];
	to[42] = from[10];
	to[43] = from[10];
	to[45] = from[11];
	to[46] = from[11];
	to[47] = from[11];
	to[49] = from[12];
	to[50] = from[12];
	to[51] = from[12];
	to[53] = from[13];
	to[54] = from[13];
	to[55] = from[13];
	to[57] = from[14];
	to[58] = from[14];
	to[59] = from[14];
	to[61] = from[15];
	to[62] = from[15];
	to[63] = from[15];
#endif
}
#else
extern void __expandMatrix( __glMatrixSIMD *to, GLfloat *from );
#endif



/*********************************************************************/

void validateModelMatrix( __glContext *gc )
{
	__expandMatrix ( &gc->model, gc->transform.modelView->matrix.matrix );
	if( gc->state.light.LightingEnabled )
	{
		__glFloat temp[16];
		mathInvertMatrix2 (gc->transform.modelView->matrix.matrix, temp);
		mathTransposeMatrix2 (temp, gc->transform.modelView->inverseTranspose.matrix);
		__expandMatrix ( &gc->invModel, gc->transform.modelView->inverseTranspose.matrix );
	}
	
	DRIVERPROC_MATRIX_MODEL(gc);
}

void validateProjectionMatrix( __glContext *gc )
{
	DRIVERPROC_MATRIX_PROJ(gc);
}

void validateMVPMatrix( __glContext *gc )
{
	mathMultMatrix3( gc->transform.modelView->matrix.matrix,
					 gc->transform.projection->matrix.matrix,
					 gc->transform.modelView->mvp.matrix);
	__expandMatrix ( &gc->mvp, gc->transform.modelView->mvp.matrix );

	gc->valid.MVPMatrixType = ( (gc->mvp.M03[0] == 0) &&
					   (gc->mvp.M13[0] == 0) &&
					   (gc->mvp.M23[0] == 0) &&
					   (gc->mvp.M33[0] == 1));
	DRIVERPROC_MATRIX_MVP(gc);
}



extern GLuint calcPolyfinNeeds( __glContext *gc );
extern GLuint calcVapiNeeds( __glContext *gc );
extern GLuint getPolyfinProcessorIndex( __glContext *gc, GLuint needs, GLenum primType );
extern void * beginProcs[10];
extern void * endProcs[10];
void LinkLights (__glContext * gc);
extern void vapi_MakeTransformer( __glContext *gc, GLuint axisCount );


extern void scanline_makeRasterizer( __glContext *gc );


void validateAll( __glContext *gc )
{
	GLuint temp = 0;
	GLuint newNeeds;

	if( gc->valid.ModelMatrix )
	{
		validateModelMatrix( gc );
		temp = 1;
		gc->valid.ModelMatrix = 0;
	}

	if( gc->valid.ProjectionMatrix )
	{
		validateProjectionMatrix( gc );
		temp = 1;
		gc->valid.ProjectionMatrix = 0;
	}
	
	if( temp )
	{
		GLuint old = gc->valid.MVPMatrixType;
		validateMVPMatrix( gc );
		if( old != gc->valid.MVPMatrixType )
		{
			gc->valid.VertexProcs = 1;
			gc->valid.VapiProcessor = 1;
		}
	}

	if( gc->valid.VertexProcs )
	{
		gc->valid.VertexProcs = 0;
	}

	if( gc->valid.PrimProcessor )
	{
		newNeeds = calcPrimitiveProcessorNeeds( gc );
		if( newNeeds != gc->primitive.CacheCurrentNeeds )
		{
			GLuint ct;
			gc->primitive.CacheCurrentNeeds = newNeeds;
			/* Lets zero out all of the pointers so that
				glBegin will know to get new procs. */
			for( ct=0; ct<10; ct++ )
			{
				gc->primitive.CacheProcess[ct] = 0;
				gc->primitive.CacheEnd[ct] = 0;
			}
		}
		gc->valid.PrimProcessor = 0;
	}

	if( gc->valid.VapiProcessor )
	{
		newNeeds = calcVapiNeeds( gc );
		if( newNeeds != gc->vertex.VapiNeeds )
		{
			gc->vertex.VapiNeeds = newNeeds;
			vapi_MakeTransformer( gc, 3 );
		}
		gc->valid.VapiProcessor = 0;
	}

	if( gc->valid.LightData )
	{
		LinkLights( gc );
		gc->valid.LightData = 0;
	}

	gc->valid.All = 0;
	
	scanline_makeRasterizer(gc);
}

