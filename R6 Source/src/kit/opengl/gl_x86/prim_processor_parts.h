#ifndef __PRIM_PROCESSOR_PARTS_H__
#define __PRIM_PROCESSOR_PARTS_H__

extern GLuint calcPrimitiveProcessorNeeds( __glContext *gc );
extern void makePrimitiveProcessor( __glContext *gc, GLenum primType );


#define __POLYFIN_CULL_ALL		0x00000001
#define __POLYFIN_FACING		0x00000002
#define __POLYFIN_CULL_FRONT	0x00000004
#define __POLYFIN_CULL_BACK		0x00000008
#define __POLYFIN_FRONT_CCW		0x00000010
#define __POLYFIN_FRONT_FILL	0x00000020
#define __POLYFIN_FRONT_LINE	0x00000040
#define __POLYFIN_FRONT_POINT	0x00000080
#define __POLYFIN_BACK_FILL		0x00000100
#define __POLYFIN_BACK_LINE		0x00000200
#define __POLYFIN_BACK_POINT	0x00000400
#define __POLYFIN_EWNT			0x00000800
#define __POLYFIN_PROVOKING		0x00001000
#define __POLYFIN_TWO_SIDED		0x00002000


#define DEF_PART(p) extern GLubyte primParts_##p;  extern GLubyte primParts_##p##_end

DEF_PART( KatmaiEndPrefix1 );
DEF_PART( KatmaiEndPrefix2 );

DEF_PART( pop_nop );

DEF_PART( Clipper_Get_FillFront );
DEF_PART( Clipper_Get_LineFront );
DEF_PART( Clipper_Get_PointFront );
DEF_PART( Clipper_Get_FillBack );
DEF_PART( Clipper_Get_LineBack );
DEF_PART( Clipper_Get_PointBack );
DEF_PART( Clipper_Put_Front );
DEF_PART( Clipper_Put_Back );


DEF_PART( Polygon_Init );
DEF_PART( Polygon_Loop1 );
DEF_PART( Polygon_Loop2 );
DEF_PART( Polygon_SetProvoking );
DEF_PART( Polygon_TestClipCode );
DEF_PART( Polygon_Facing );
DEF_PART( Polygon_EWNT_Front );
DEF_PART( Polygon_EWNT_Back );
DEF_PART( Polygon_Clipped1 );
DEF_PART( Polygon_Clipped2 );
DEF_PART( Polygon_Clipped3 );
DEF_PART( Polygon_Finish );
DEF_PART( Polygon_Front_Fill );
DEF_PART( Polygon_Front_Line );
DEF_PART( Polygon_Front_Point );
DEF_PART( Polygon_Back_Fill );
DEF_PART( Polygon_Back_Line );
DEF_PART( Polygon_Back_Point );

DEF_PART( Fan_Init );
DEF_PART( Fan_Loop1 );
DEF_PART( Fan_Loop2 );
DEF_PART( Fan_SetProvoking );
DEF_PART( Fan_Facing );
DEF_PART( Fan_EWNT_Front );
DEF_PART( Fan_EWNT_Back );
DEF_PART( Fan_Clipped1 );
DEF_PART( Fan_Clipped2 );
DEF_PART( Fan_Clipped3 );
DEF_PART( Fan_Finish );
DEF_PART( Fan_Front_Fill );
DEF_PART( Fan_Front_Line );
DEF_PART( Fan_Front_Point );
DEF_PART( Fan_Back_Fill );
DEF_PART( Fan_Back_Line );
DEF_PART( Fan_Back_Point );

DEF_PART( Quad_Init );
DEF_PART( Quad_SetProvoking );
DEF_PART( Quad_Facing );
DEF_PART( Quad_EWNT_Front );
DEF_PART( Quad_EWNT_Back );
DEF_PART( Quad_Clipped1 );
DEF_PART( Quad_Clipped2 );
DEF_PART( Quad_Clipped3 );
DEF_PART( Quad_Finish );
DEF_PART( Quad_Front_Fill );
DEF_PART( Quad_Front_Line );
DEF_PART( Quad_Front_Point );
DEF_PART( Quad_Back_Fill );
DEF_PART( Quad_Back_Line );
DEF_PART( Quad_Back_Point );

DEF_PART( QuadStrip_Init );
DEF_PART( QuadStrip_Loop1 );
DEF_PART( QuadStrip_Loop2 );
DEF_PART( QuadStrip_Loop3 );
DEF_PART( QuadStrip_SetProvoking );
DEF_PART( QuadStrip_Facing );
DEF_PART( QuadStrip_Clipped1 );
DEF_PART( QuadStrip_Clipped2 );
DEF_PART( QuadStrip_Clipped3 );
DEF_PART( QuadStrip_Finish );
DEF_PART( QuadStrip_Front_Fill );
DEF_PART( QuadStrip_Front_Line );
DEF_PART( QuadStrip_Front_Point );
DEF_PART( QuadStrip_Back_Fill );
DEF_PART( QuadStrip_Back_Line );
DEF_PART( QuadStrip_Back_Point );
DEF_PART( QuadStrip_EWNT_Front );
DEF_PART( QuadStrip_EWNT_Back );

DEF_PART( Triangles_Init );
DEF_PART( Triangles_Loop1 );
DEF_PART( Triangles_Loop2 );
DEF_PART( Triangles_SetProvoking );
DEF_PART( Triangles_Facing );
DEF_PART( Triangles_EWNT_Front );
DEF_PART( Triangles_EWNT_Back );
DEF_PART( Triangles_Clipped1 );
DEF_PART( Triangles_Clipped2 );
DEF_PART( Triangles_Clipped3 );
DEF_PART( Triangles_Finish );
DEF_PART( Triangles_Front_Fill );
DEF_PART( Triangles_Front_Line );
DEF_PART( Triangles_Front_Point );
DEF_PART( Triangles_Back_Fill );
DEF_PART( Triangles_Back_Line );
DEF_PART( Triangles_Back_Point );

DEF_PART( TriangleStrip_Init );
DEF_PART( TriangleStrip_Loop1 );
DEF_PART( TriangleStrip_Loop2 );
DEF_PART( TriangleStrip_Loop3 );
DEF_PART( TriangleStrip_Loop4 );
DEF_PART( TriangleStrip_EWNT_Front );
DEF_PART( TriangleStrip_EWNT_Back );
DEF_PART( TriangleStrip_SetProvoking );
DEF_PART( TriangleStrip_Facing1 );
DEF_PART( TriangleStrip_Facing2 );
DEF_PART( TriangleStrip_Clipped11 );
DEF_PART( TriangleStrip_Clipped12 );
DEF_PART( TriangleStrip_Clipped13 );
DEF_PART( TriangleStrip_Clipped21 );
DEF_PART( TriangleStrip_Clipped22 );
DEF_PART( TriangleStrip_Clipped23 );
DEF_PART( TriangleStrip_Finish );
DEF_PART( TriangleStrip_Front_Fill1 );
DEF_PART( TriangleStrip_Front_Line1 );
DEF_PART( TriangleStrip_Front_Point1 );
DEF_PART( TriangleStrip_Front_Fill2 );
DEF_PART( TriangleStrip_Front_Line2 );
DEF_PART( TriangleStrip_Front_Point2 );
DEF_PART( TriangleStrip_Back_Fill1 );
DEF_PART( TriangleStrip_Back_Line1 );
DEF_PART( TriangleStrip_Back_Point1 );
DEF_PART( TriangleStrip_Back_Fill2 );
DEF_PART( TriangleStrip_Back_Line2 );
DEF_PART( TriangleStrip_Back_Point2 );

DEF_PART( LineStrip_Init );
DEF_PART( LineStrip_Loop1 );
DEF_PART( LineStrip_Loop2 );
DEF_PART( LineStrip_SetProvoking );
DEF_PART( LineStrip_EWNT );
DEF_PART( LineStrip_Line );
DEF_PART( LineStrip_Clipped1 );
DEF_PART( LineStrip_Clipped2 );
DEF_PART( LineStrip_Clipped3 );
DEF_PART( LineStrip_Finish );

DEF_PART( Lines_Init );
DEF_PART( Lines_Loop1 );
DEF_PART( Lines_Loop2 );
DEF_PART( Lines_SetProvoking );
DEF_PART( Lines_EWNT );
DEF_PART( Lines_Line );
DEF_PART( Lines_Clipped1 );
DEF_PART( Lines_Clipped2 );
DEF_PART( Lines_Clipped3 );
DEF_PART( Lines_Finish );

DEF_PART( LineLoop_CloseLoop1 );
DEF_PART( LineLoop_CloseLoop2 );
DEF_PART( LineLoop_CloseLoopEWNT );
DEF_PART( LineLoop_Init );
DEF_PART( LineLoop_Loop1 );
DEF_PART( LineLoop_Loop2 );
DEF_PART( LineLoop_SetProvoking );
DEF_PART( LineLoop_EWNT );
DEF_PART( LineLoop_Line );
DEF_PART( LineLoop_Clipped1 );
DEF_PART( LineLoop_Clipped2 );
DEF_PART( LineLoop_Clipped3 );
DEF_PART( LineLoop_Finish );

DEF_PART( Points_Init );
DEF_PART( Points_Loop1 );
DEF_PART( Points_Loop2 );
DEF_PART( Points_EWNT );
DEF_PART( Points_Point );
DEF_PART( Points_Finish );

#undef DEF_PART

#endif
