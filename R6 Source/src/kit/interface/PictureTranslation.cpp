
//******************************************************************************
//
//	File:		PictureTranslation.h
//
//	Description:	Deals with translation between new and old
//				picture formats.
//	
//	Written by:	George Hoffman
//
//	Copyright 1997, Be Incorporated
//
//******************************************************************************/

#include <Bitmap.h>
#include <ByteOrder.h>
#include <Shape.h>

#include "PictureTranslation.h"
#include "shared_picture.h"
#include "shared_support.h"
#include <math.h>
#include <Debug.h>

/*	This file consists of one large routine to translate from the old
	format to the new format, and a set of small routines which together
	make up a callback table which should be passed to BPicture::Play
	in order to convert from the new format to the old format, as well as
	the playback routine itself. */

enum OldPictureOps {
	PIC_MOVETO = 100,			/* 100 */
	PIC_LINETO,					/* 101 */
	PIC_RECTFILL,				/* 102 */
	PIC_RECT_INVERT,			/* 103 */
	PIC_SET_DRAW_MODE,			/* 104 */
	PIC_DRAW_STRING,			/* 105 */
	PIC_SET_PEN_SIZE,			/* 106 */
	PIC_ELLIPSE_FRAME,			/* 107 */
	PIC_ELLIPSE_FILL,			/* 108 */
	PIC_ARC_FRAME,				/* 109 */
	PIC_ARC_FILL,				/* 110 */
	PIC_ROUND_RECT_FRAME,		/* 111 */
	PIC_ROUND_RECT_FILL,		/* 112 */
	PIC_FORE_COLOR,				/* 113 */
	PIC_BACK_COLOR,				/* 114 */
	PIC_LINETO_PAT,				/* 115 */
	PIC_SET_FONT_FAMILY,		/* 116 */
	PIC_SET_FONT_SPACING,		/* 117 */
	PIC_SET_FONT_SIZE,			/* 118 */
	PIC_SET_FONT_ROTATE,		/* 119 */
	PIC_MOVEBY,					/* 120 */
	PIC_SET_SCALE,				/* 121 */
	PIC_SET_FONT_ENCODING,		/* 122 */
	PIC_RECTFRAME,				/* 123 */
	PIC_RECTFRAME_PAT,			/* 124 */
	PIC_FILLPOLY,				/* 125 */
	PIC_FRAMEPOLY,				/* 126 */
	PIC_BLIT,					/* 127 */
	PIC_VARRAY,					/* 128 */
	PIC_END_PICTURE,			/* 129 */
	PIC_OFFSET_FRAME,			/* 130 */
	PIC_SUB_PICTURE,			/* 131 */
	PIC_SET_FONT_FLAGS,			/* 132 */
	PIC_SET_FONT_STYLE,			/* 133 */
	PIC_SET_FONT_SHEAR,			/* 134 */
	PIC_SET_FONT_BPP,			/* 135 */
	PIC_SET_FONT_FACES			/* 136 */
};

#define SIZE_INC 256

class TPicture {
public:
							TPicture(
								void *_inData, int32 _inDataSize,
								BArray<BPicture*> *_pictureLib);
							~TPicture();
		
		void				BeginOp(int32 opCode);
		void				EndOp();
		void				EndAndPossiblyForgetOp();

		void				AddData(void *data, int32 len);
		void				AddRect(BRect value);
		void				AddColor(rgb_color value);
		void				AddCoord(BPoint value);
		void				AddFloat(float value);
		void				AddInt64(int64 value);
		void				AddInt32(int32 value);
		void				AddInt16(int16 value);
		void				AddInt8(int8 value);
		void				AddString(char *str);
		
		void				Rewind();
		void				AssertSpace(int32 size);
		
		int16				SwapOp();
		void				SwapString();
		void				SwapRect();
		void				SwapIRect();
		void				SwapColor();
		void				SwapCoord();
		void				SwapFloat();
		void				SwapInt64();
		int32				SwapInt32();
		void				SwapInt16();
		void				SwapInt8();

		int16				GetOp();
		BRect				GetRect();
		rgb_color			GetColor();
		BPoint				GetCoord();
		float				GetFloat();
		int64				GetInt64();
		int32				GetInt32();
		int16				GetInt16();
		int8				GetInt8();
		void				GetString(char *str);
		void *				GetData(int32 size);
		void				GetData(void *buf, int32 size);
		
		void				Trim();
		void				Swap();
		void				New2Old();
		void				Old2New();
		void				Play(
							void **callbackTable,
							int32 tableEntries,
							void *context);

		void				CheckPattern();
		void				EnterFontChange();
		void				ExitFontChange();
		void				EnterStateChange();
		void				ExitStateChange();
		
		uint8 *				data;
		int32				dataPtr;
		int32				dataLen;
		BArray<int32>		opBegin;

		uint8 *				inData;
		int32				inDataSize;
		int32				ip;
		
		bool				stateChange,fontChange;
		pattern				curPattern;
		BArray<BPicture*> *	pictureLib;
};

void convert_old_to_new(
	void *oldPtr, int32 oldSize,
	void **newPtr, int32 *newSize)
{
	TPicture p(oldPtr,oldSize,NULL);
	p.Old2New();
	p.Trim();
	*newPtr = p.data;
	*newSize = p.dataPtr;
}

TPicture::TPicture(void *_inData, int32 _inDataSize, BArray<BPicture*> *_pictureLib)
{
	inData = (uint8*)_inData;
	inDataSize = _inDataSize;
	pictureLib = _pictureLib;
	ip = 0;
	stateChange = fontChange = false;
	*((uint64 *)&curPattern) = 0xFFFFFFFFFFFFFFFFLL;
	dataLen = dataPtr = 0;
	data = NULL;
}

TPicture::~TPicture()
{
}

void TPicture::Trim()
{
	if (dataPtr != dataLen)
		data = (uint8*)realloc(data,dataLen=dataPtr);
}

void TPicture::BeginOp(int32 opCode)
{
	opBegin.AddItem(dataPtr);
	AddInt16(opCode);
	AddInt32(0);
}

void TPicture::EndOp()
{
	int32 begin = opBegin[opBegin.CountItems()-1];
	opBegin.SetItems(opBegin.CountItems()-1);
	int32 *p = (int32*)(data+begin+2);
	*p = dataPtr - begin - 6;
};

void TPicture::EndAndPossiblyForgetOp()
{
	int32 begin = opBegin[opBegin.CountItems()-1];
	opBegin.SetItems(opBegin.CountItems()-1);
	if ((dataPtr - begin - 6) == 0) {
		dataPtr = begin;
	} else {
		int32 *p = (int32*)(data+begin+2);
		*p = dataPtr - begin - 6;
	};
};

void TPicture::AddData(void *_data, int32 len)
{
	AssertSpace(len);
	memcpy(data+dataPtr,_data,len);	
	dataPtr += len;
};

void TPicture::AddRect(BRect value)
{
	AddData(&value,sizeof(BRect));
};

void TPicture::AddColor(rgb_color value)
{
	AddData(&value,sizeof(rgb_color));
};

void TPicture::AddCoord(BPoint value)
{
	AddData(&value,sizeof(BPoint));
};

void TPicture::AddFloat(float value)
{
	AddData(&value,sizeof(float));
};

void TPicture::AddInt64(int64 value)
{
	AddData(&value,sizeof(int64));
};

void TPicture::AddInt32(int32 value)
{
	AddData(&value,sizeof(int32));
};

void TPicture::AddInt16(int16 value)
{
	AddData(&value,sizeof(int16));
};

void TPicture::AddInt8(int8 value)
{
	AddData(&value,sizeof(int8));
};

void TPicture::AddString(char *str)
{
	int32 len = strlen(str);
	AddInt32(len);
	AddData(str,len);
	PRINT(("AddString: \"%s\"\n",str));
};

void TPicture::AssertSpace(int32 size)
{
	if ((dataLen-dataPtr) < size) {
		while ((dataLen-dataPtr) < size) {
			if (!dataLen)
				dataLen = 4;
			else
				dataLen *= 2;
		};
		data = (uint8*)realloc(data,dataLen);
	};
};

void TPicture::Rewind()
{
	ip = 0;
};

void * TPicture::GetData(int32 size)
{
	void *p = (inData+ip);
	ip += size;
	return p;
};

void TPicture::GetData(void *v, int32 size)
{
	void *p = (inData+ip);
	memcpy(v,p,size);
	ip += size;
};

BRect TPicture::GetRect()
{
	BRect f;
	GetData(&f,sizeof(BRect));
	return f;
};

rgb_color TPicture::GetColor()
{
	rgb_color f;
	GetData(&f,sizeof(rgb_color));
	return f;
};

BPoint TPicture::GetCoord()
{
	BPoint p;
	GetData(&p,sizeof(BPoint));
	return p;
};

float TPicture::GetFloat()
{
	float f;
	GetData(&f,sizeof(float));
	return f;
};

int64 TPicture::GetInt64()
{
	int64 f;
	GetData(&f,sizeof(int64));
	return f;
};

int32 TPicture::GetInt32()
{
	int32 f;
	GetData(&f,sizeof(int32));
	return f;
};

int16 TPicture::GetInt16()
{
	int16 f;
	GetData(&f,sizeof(int16));
	return f;
};

int8 TPicture::GetInt8()
{
	int8 f;
	GetData(&f,sizeof(int8));
	return f;
};

void TPicture::SwapRect()
{
	SwapFloat();
	SwapFloat();
	SwapFloat();
	SwapFloat();
};

void TPicture::SwapIRect()
{
	SwapInt32();
	SwapInt32();
	SwapInt32();
	SwapInt32();
};

void TPicture::SwapColor()
{
	ip+=4;
};

void TPicture::SwapCoord()
{
	SwapFloat();
	SwapFloat();
};

void TPicture::SwapFloat()
{
	float f;
	memcpy(&f,inData+ip,4);
	f = __swap_float(f);
	memcpy(inData+ip,&f,4);
	ip+=4;
};

void TPicture::SwapInt64()
{
	uint64 *p = (uint64*)(inData+ip);
	*p = __swap_int64(*p);
	ip+=8;
};

int32 TPicture::SwapInt32()
{
	uint32 *p = (uint32*)(inData+ip);
	*p = __swap_int32(*p);
	ip+=4;
	return *p;
};

void TPicture::SwapInt16()
{
	uint16 *p = (uint16*)(inData+ip);
	*p = __swap_int16(*p);
	ip+=2;
};

void TPicture::SwapInt8()
{
	ip+=1;
};

int16 TPicture::SwapOp()
{
	if (ip > (inDataSize-6)) return 0;
	uint16 *p = (uint16*)(inData+ip);
	*p = __swap_int16(*p);
	ip+=2;
	return *p;
};

void TPicture::SwapString()
{
	int32 len = SwapInt32();
	ip += len;
};

void TPicture::GetString(char *str)
{
	int32 len = GetInt32();
	GetData(str,len);
	str[len] = 0;
	PRINT(("GetString: \"%s\"\n",str));
};

int16 TPicture::GetOp()
{
	PRINT(("GetOp %d %d\n",ip,inDataSize));
	if (ip > (inDataSize-6)) return 0;
	return GetInt16();
};

void TPicture::CheckPattern()
{
	pattern pat = *((pattern*)GetData(sizeof(pattern)));
	if (*((int64*)&curPattern) != *((int64*)&pat)) {
		*((int64*)&curPattern) = *((int64*)&pat);
		EnterStateChange();
		BeginOp(SPIC_STIPPLE);
		AddInt64(*((int64*)&pat));
		EndOp();
	};
};

void TPicture::EnterFontChange()
{
	if (fontChange) return;
	EnterStateChange();
	PRINT(("Entering font change\n"));
	BeginOp(SPIC_FONT);
	fontChange = true;
};

void TPicture::ExitFontChange()
{
	if (!fontChange) return;
	PRINT(("Exitting font change\n"));
	EndOp();
	fontChange = false;
};

void TPicture::EnterStateChange()
{
	if (stateChange) return;
	PRINT(("Entering state change\n"));
	BeginOp(SPIC_SET_STATE);
	stateChange = true;
};

void TPicture::ExitStateChange()
{
	if (!stateChange) return;
	ExitFontChange();
	PRINT(("Exitting state change\n"));
	EndOp();
	stateChange = false;
};

void	TPicture::Old2New()
{
	int32	opcode;
	char	str[4096];
	float	f[4];
	int32	i=0;
	int16 *	i16p;
	BPoint	p[4];
	BPoint 	stack[64];
	BPoint 	pen,origin;
	int32	stack_pointer;
	BRect	r;
	pattern	pat;
	int32	drawOp=B_OP_OVER;
	bool		b;
	int32	word32[16];
	rgb_color	c;

	PRINT(("old to new\n"));

	stack_pointer = 0;
	pen.x = pen.y = origin.x = origin.y = 0;

	Rewind();
	while(1) {
		opcode = GetInt32();
		i++;
		//xprintf("opcode=%ld %lx\n", opcode, opcode);
		switch(opcode) {
			case PIC_MOVETO:
				PRINT(("PIC_MOVETO\n"));
				EnterStateChange();
				ExitFontChange();
				BeginOp(SPIC_LOCATION);
				AddCoord(pen=GetCoord());
				EndOp();
				break;
			case PIC_MOVEBY:
				PRINT(("PIC_MOVEBY\n"));
				ExitStateChange();
				BeginOp(SPIC_MOVE_PEN);
				AddCoord(p[0]=GetCoord());
				pen+=p[0];
				EndOp();
				break;
			case PIC_LINETO:
			case PIC_LINETO_PAT:
				PRINT(("PIC_LINETO\n"));
				p[0] = GetCoord();
				if (opcode== PIC_LINETO_PAT) CheckPattern();
				ExitStateChange();
				BeginOp(SPIC_STROKE_LINE);
				AddCoord(pen);
				AddCoord(p[0]);
				EndOp();
				pen = p[0];
				break;
			case PIC_RECTFRAME:	
			case PIC_RECTFRAME_PAT:
			case PIC_RECTFILL:
				PRINT(("PIC_RECT\n"));
				r = GetRect();
				if (opcode != PIC_RECTFRAME) CheckPattern();
				ExitStateChange();
				BeginOp((opcode==PIC_RECTFILL)?SPIC_FILL_RECT:SPIC_STROKE_RECT);
				AddRect(r);
				EndOp();
				break;
			case PIC_RECT_INVERT:
				PRINT(("PIC_RECT_INVERT\n"));
				r = GetRect();
				pat = *((pattern*)GetData(sizeof(pattern)));
				b = false;
				if (drawOp != B_OP_INVERT) {
					b = true;
					EnterStateChange();
					BeginOp(SPIC_DRAW_OP);
					AddInt32(B_OP_INVERT);
					EndOp();
				};
				ExitStateChange();
				BeginOp(SPIC_FILL_RECT);
				AddRect(r);
				EndOp();
				if (b) {
					EnterStateChange();
					BeginOp(SPIC_DRAW_OP);
					AddInt32(drawOp);
					EndOp();
				};
				break;
			case PIC_SET_DRAW_MODE:
				PRINT(("PIC_SET_DRAW_MODE\n"));
				drawOp = GetInt32();
				EnterStateChange();
				BeginOp(SPIC_DRAW_OP);
				AddInt16(drawOp);
				EndOp();
				break;
			case PIC_DRAW_STRING:
				PRINT(("PIC_DRAW_STRING\n"));
				GetString(str);
				f[0] = GetFloat();
				f[1] = GetFloat();
				ExitStateChange();
				BeginOp(SPIC_DRAW_STRING);
				AddString(str);
				AddFloat(f[0]);
				AddFloat(f[1]);
				EndOp();
				break;
			case PIC_SET_PEN_SIZE:
			case PIC_SET_SCALE:
				PRINT(("PIC_SET_SCALE\n"));
				EnterStateChange();
				BeginOp(
					(opcode==PIC_SET_PEN_SIZE) ? SPIC_PEN_SIZE :
					SPIC_SCALE);
				AddFloat(GetFloat());
				EndOp();
				break;
			case PIC_ELLIPSE_FRAME:
			case PIC_ELLIPSE_FILL:
				PRINT(("PIC_ELLIPSE\n"));
				r = GetRect();
				CheckPattern();
				p[0].x = (r.left+r.right)/2;
				p[0].y = (r.top+r.bottom)/2;
				p[1].x = (r.right-r.left)/2;
				p[1].y = (r.bottom-r.top)/2;
				ExitStateChange();
				BeginOp(
					(opcode==PIC_ELLIPSE_FRAME)?
					SPIC_STROKE_ELLIPSE:
					SPIC_FILL_ELLIPSE);
				AddCoord(p[0]);
				AddCoord(p[1]);
				EndOp();
				break;
			case PIC_ARC_FRAME:
			case PIC_ARC_FILL:
				PRINT(("PIC_ARC\n"));
				r = GetRect();
				p[0].x = (r.left+r.right)/2;
				p[0].y = (r.top+r.bottom)/2;
				p[1].x = (r.right-r.left)/2;
				p[1].y = (r.bottom-r.top)/2;
				word32[0] = GetInt32();
				word32[1] = GetInt32();
				CheckPattern();
				ExitStateChange();
				BeginOp(
					(opcode==PIC_ARC_FRAME)?
					SPIC_STROKE_ARC:
					SPIC_FILL_ARC);
				AddCoord(p[0]);
				AddCoord(p[1]);
				AddFloat(word32[0]);
				AddFloat(word32[1]);
				EndOp();
				break;
			case PIC_ROUND_RECT_FRAME:
			case PIC_ROUND_RECT_FILL:
				PRINT(("PIC_ROUND_RECT\n"));
				r = GetRect();
				p[0] = GetCoord();
				CheckPattern();
				ExitStateChange();
				BeginOp(
					(opcode==PIC_ROUND_RECT_FRAME)?
					SPIC_STROKE_ROUNDRECT:
					SPIC_FILL_ROUNDRECT);
				AddRect(r);
				AddCoord(p[0]);
				EndOp();
				break;
			case PIC_FORE_COLOR:
			case PIC_BACK_COLOR:
				PRINT(("PIC_SET_COLOR\n"));
				c = GetColor();
				EnterStateChange();
				BeginOp(
					(opcode==PIC_FORE_COLOR)?
					SPIC_FORE_COLOR:
					SPIC_BACK_COLOR);
				AddColor(c);
				EndOp();				
				break;
			case PIC_SET_FONT_FAMILY:
			case PIC_SET_FONT_STYLE:
				PRINT(("PIC_SET_FONT_NAME\n"));
				GetString(str);
				EnterFontChange();
				BeginOp(
					(opcode==PIC_SET_FONT_FAMILY)?
					SPIC_FAMILY:
					SPIC_STYLE);
				AddString(str);
				EndOp();
				break;
			case PIC_SET_FONT_SIZE:
			case PIC_SET_FONT_ROTATE:
			case PIC_SET_FONT_SHEAR:
				PRINT(("PIC_SET_FONT_SIZE\n"));
				EnterFontChange();
				BeginOp(
					(opcode==PIC_SET_FONT_SIZE) ? SPIC_SIZE :
					(opcode==PIC_SET_FONT_ROTATE) ? SPIC_ROTATE :
					SPIC_SHEAR);
				f[0] = GetFloat();
				AddFloat(f[0]);
				EndOp();
				break;
			case PIC_SET_FONT_SPACING:
			case PIC_SET_FONT_ENCODING:
			case PIC_SET_FONT_FLAGS:
			case PIC_SET_FONT_FACES:
			case PIC_SET_FONT_BPP:
				PRINT(("PIC_SET_FONT_OTHER\n"));
				EnterFontChange();
				BeginOp(
					(opcode==PIC_SET_FONT_SPACING) ? SPIC_SPACING :
					(opcode==PIC_SET_FONT_ENCODING) ? SPIC_ENCODING :
					(opcode==PIC_SET_FONT_FLAGS) ? SPIC_FLAGS :
					(opcode==PIC_SET_FONT_FACES) ? SPIC_FACES :
					SPIC_BPP);
				AddInt32(GetInt32());
				EndOp();
				break;
			case PIC_FRAMEPOLY :	
			case PIC_FILLPOLY:
			{
				PRINT(("PIC_POLY\n"));
				r = GetRect();
				if (opcode == PIC_FRAMEPOLY) {
					PRINT(("frame\n"));
					CheckPattern();
					word32[2] = GetInt32();
				};
				word32[0] = GetInt32();
				PRINT(("count=%d\n",word32[0]));
				
				word32[1] = word32[0] * sizeof(int32) * 2;
				int32 *pts = (int32*)malloc(word32[1]);
				int32 *i32p=pts;
				GetData(pts,word32[1]);
				if (opcode == PIC_FILLPOLY) CheckPattern();
				if (word32[0] > 2) {
					ExitStateChange();
					p[0].x = *i32p++;
					p[0].y = *i32p++;
					BeginOp((opcode == PIC_FILLPOLY)?
						SPIC_FILL_POLYGON : SPIC_STROKE_POLYGON);
					AddInt32(word32[0]);
					AddCoord(p[0]);
					word32[0]--;
					while (word32[0] > 0) {
						p[0].x = *i32p++;
						p[0].y = *i32p++;
						AddCoord(p[0]);
						word32[0]--;
					};
					if (opcode==PIC_FRAMEPOLY) AddInt8(word32[2]);
					EndOp();
				};
				free(pts);
				break;
			};
			case PIC_BLIT :
			{
				PRINT(("PIC_BLIT\n"));
				BRect	srcRect,dstRect;
				void *	pixels;
				
				srcRect = GetRect();
				dstRect = GetRect();

				word32[7] = GetInt32(); // Port type
				word32[0] = GetInt32(); // Bits per pixel
				word32[1] = GetInt32(); // Bytes per row
				word32[2] = GetInt32(); // Left
				word32[3] = GetInt32(); // Top
				word32[4] = GetInt32(); // Right
				word32[5] = GetInt32(); // Bottom
				word32[6] = GetInt32(); // Size of image
				pixels = inData+ip;
				ip += word32[6];
				
				ExitStateChange();

				BeginOp(SPIC_PIXELS);
				AddRect(srcRect);			/* Source rect		*/
				AddRect(dstRect);			/* Destination rect	*/
				AddInt32(word32[4]+1);		/* Width			*/
				AddInt32(word32[5]+1);		/* Height			*/
				AddInt32(word32[1]);		/* Bytes per row	*/
				AddInt32(word32[7]);		/* Pixel format		*/
				AddInt32(0);				/* Flags			*/
				AddInt32(word32[6]);		/* Pixel data size	*/
				AddData(pixels,word32[6]);	/* Pixel data		*/
				EndOp();
				break;
			}
			case PIC_VARRAY :
				PRINT(("PIC_VARRAY\n"));
				ExitStateChange();
				word32[0] = GetInt32();
				i16p = (int16*)GetData(word32[0] * sizeof(int16) * 4);
				while (word32[0] > 0) {
					p[0].x = *i16p++;
					p[0].y = *i16p++;
					p[1].x = *i16p++;
					p[1].y = *i16p++;
					BeginOp(SPIC_STROKE_LINE);
					AddCoord(p[0]);
					AddCoord(p[1]);
					EndOp();
					word32[0]--;
				}
				break;
			case PIC_END_PICTURE:
				PRINT(("PIC_END_PICTURE\n"));
				if (stack_pointer == 0) {
					ExitStateChange();
					return;
				};
				stack_pointer--;
				origin = stack[stack_pointer];
				EnterStateChange();
				BeginOp(SPIC_ORIGIN);
				AddCoord(origin);
				EndOp();
				break;
			case PIC_SUB_PICTURE:
				PRINT(("PIC_SUB_PICTURE\n"));
				break;
			case PIC_OFFSET_FRAME:
				PRINT(("PIC_OFFSET_FRAME\n"));
				stack[stack_pointer] = origin;
				stack_pointer++;
				origin.x += GetInt32();
				origin.y += GetInt32();
				EnterStateChange();
				BeginOp(SPIC_ORIGIN);
				AddCoord(origin);
				EndOp();
				break;
			default:
				PRINT(("unknown old pic opcode!\n"));
				ExitStateChange();
				return;
		}
	}
	ExitStateChange();
}

typedef int32 (*fN)(void *);
typedef int32 (*fI)(void *, int32);
typedef int32 (*fF)(void *, float);
typedef int32 (*fC)(void *, rgb_color);
typedef int32 (*fS)(void *, char *);
typedef int32 (*fSp)(void *, pattern);
typedef int32 (*fPt)(void *, BPoint);
typedef int32 (*fPtP)(void *, BPoint *);
typedef int32 (*fIPtP)(void *, int32, BPoint *);
typedef int32 (*fIPtPB)(void *, int32, BPoint *, bool);
typedef int32 (*fSFF)(void *, char *, float, float);
typedef int32 (*fPtPt)(void *, BPoint, BPoint);
typedef int32 (*fPtPtFF)(void *, BPoint, BPoint, float, float);
typedef int32 (*fR)(void *, BRect);
typedef int32 (*fRPt)(void *, BRect, BPoint);
typedef int32 (*fRPI)(void *, BRect *, int32);
typedef int32 (*fIIF)(void *, int32, int32, float);
typedef int32 (*fPixels)(void *, BRect, BRect, int32, int32, int32, int32, int32, void *);
typedef int32 (*fShape)(void *, BShape *);
typedef int32 (*fPicPt)(void *, BPicture *, BPoint);
typedef int32 (*fPicPtI)(void *, BPicture *, BPoint, uint32);

enum PictureCallbacks {
	cbNull=0,
	
	cbMovePen,				// fPt
	
	cbStrokeLine,			// fPtPt

	cbStrokeRect,			// fR
	cbFillRect,				// fR

	cbStrokeRoundRect,		// fRPt
	cbFillRoundRect,		// fRPt

	cbStrokeBezier,			// fPtP
	cbFillBezier,			// fPtP

	cbStrokeArc,			// fPtPtFF
	cbFillArc,				// fPtPtFF

	cbStrokeEllipse,		// fPtPt
	cbFillEllipse,			// fPtPt

	cbStrokePolygon,		// fIPtP
	cbFillPolygon,			// fIPtP

	cbStrokeShape,			// fShape
	cbFillShape,			// fShape

	cbDrawString,			// fPtSFF
	cbDrawPixels,			// fPixels
	cbBlit,					// -

	cbClipToRects,			// fRPI
	cbClipToPicture,		// fPicPtI
	cbPushState,			// fN
	cbPopState,				// fN

	cbEnterStateChange,		// fN
	cbExitStateChange,		// fN
	cbEnterFontState,		// fN
	cbExitFontState,		// fN

	cbSetOrigin,			// fPt
	cbSetPenLocation,		// fPt
	cbSetDrawOp,			// fF
	cbSetLineMode,			// fIIF
	cbSetPenSize,			// fF
	cbSetForeColor,			// fC
	cbSetBackColor,			// fC
	cbSetStipplePattern,	// fSp
	cbSetScale,				// fF

	cbFontFamily,			// fS
	cbFontStyle,			// fS
	cbFontSpacing,			// fI
	cbFontSize,				// fF
	cbFontRotate,			// fF
	cbFontEncoding,			// fI
	cbFontFlags,			// fI
	cbFontShear,			// fF
	cbFontBPP,				// fI
	cbFontFaces,			// fI

	cbDrawPicture			// fPicPt
};


void TPicture::Play(void **cb, int32 tableEntries, void *context)
{
	BPoint p[4];
	BRect r;
	float startTheta,endTheta;
	int32 op;
	int32 size[8];
	int32 starts[8];
	int16 word16[4];
	int32 word32[4];
	void *ptr[4];
	uint64 i64;
	float f[4];
	char str[1024];
	BShape path;

	union {
		fN			cbN;
		fI			cbI;
		fC			cbC;
		fS			cbS;
		fF			cbF;
		fSp			cbSp;
		fPt			cbPt;
		fPtP		cbPtP;
		fIPtP		cbIPtP;
		fIPtPB		cbIPtPB;
		fSFF		cbSFF;
		fPtPt		cbPtPt;
		fPtPtFF		cbPtPtFF;
		fR			cbR;
		fRPt		cbRPt;
		fRPI		cbRPI;
		fIIF		cbIIF;
		fPixels		cbPixels;
		fShape		cbShape;
		fPicPtI		cbPicPtI;
		fPicPt		cbPicPt;
	};

#define LoadCB(index) (cbN = (fN)((index<tableEntries)?cb[index]:NULL))
		
	PRINT(("Playback...\n"));
	Rewind();
	while ((op=GetOp()) != 0) {
		size[0] = GetInt32();
		starts[0] = ip;
		PRINT(("op=%d\n",op));
		switch (op) {
			case SPIC_MOVE_PEN:
				if (!LoadCB(cbMovePen)) goto abort;
				p[0] = GetCoord();
				cbPt(context,p[0]);
				break;
			case SPIC_STROKE_LINE:
				if (!LoadCB(cbStrokeLine)) goto abort;
				p[0] = GetCoord();
				p[1] = GetCoord();
				cbPtPt(context,p[0],p[1]);
				break;
			case SPIC_STROKE_RECT:
				if (!LoadCB(cbStrokeRect)) goto abort;
				goto commonRect;
			case SPIC_FILL_RECT:
				if (!LoadCB(cbFillRect)) goto abort;
				goto commonRect;
			commonRect:
				r = GetRect();
				cbR(context,r);
				break;
			case SPIC_STROKE_ROUNDRECT:
				if (!LoadCB(cbStrokeRoundRect)) goto abort;
				goto commonRoundRect;
			case SPIC_FILL_ROUNDRECT:
				if (!LoadCB(cbFillRoundRect)) goto abort;
				goto commonRoundRect;
			commonRoundRect:
				r = GetRect();
				p[0] = GetCoord();
				cbRPt(context,r,p[0]);
				break;
			case SPIC_STROKE_BEZIER:
				if (!LoadCB(cbStrokeBezier)) goto abort;
				goto commonBezier;
			case SPIC_FILL_BEZIER:
				if (!LoadCB(cbFillBezier)) goto abort;
				goto commonBezier;
			commonBezier:
				p[0] = GetCoord();
				p[1] = GetCoord();
				p[2] = GetCoord();
				p[3] = GetCoord();
				cbPtP(context,p);
				break;
			case SPIC_INSCRIBE_STROKE_ARC:
				if (!LoadCB(cbStrokeArc)) goto abort;
				r = GetRect();
				p[0].x = (r.left+r.right)/2.0;
				p[0].y = (r.top+r.bottom)/2.0;
				p[1].x = r.right - p[0].x;
				p[1].y = r.bottom - p[0].y;
				goto inscribeArc;
			case SPIC_STROKE_ARC:
				if (!LoadCB(cbStrokeArc)) goto abort;
				goto commonArc;
			case SPIC_INSCRIBE_FILL_ARC:
				if (!LoadCB(cbFillArc)) goto abort;
				r = GetRect();
				p[0].x = (r.left+r.right)/2.0;
				p[0].y = (r.top+r.bottom)/2.0;
				p[1].x = r.right - p[0].x;
				p[1].y = r.bottom - p[0].y;
				goto inscribeArc;
			case SPIC_FILL_ARC:
				if (!LoadCB(cbFillArc)) goto abort;
				goto commonArc;
			commonArc:
				p[0] = GetCoord();
				p[1] = GetCoord();
			inscribeArc:
				startTheta = GetFloat();
				endTheta = GetFloat();
				cbPtPtFF(context,p[0],p[1],startTheta,endTheta);
				break;
			case SPIC_INSCRIBE_STROKE_ELLIPSE:
				if (!LoadCB(cbStrokeEllipse)) goto abort;
				r = GetRect();
				p[0].x = (r.left+r.right)/2.0;
				p[0].y = (r.top+r.bottom)/2.0;
				p[1].x = r.right - p[0].x;
				p[1].y = r.bottom - p[0].y;
				goto inscribeEllipse;
			case SPIC_STROKE_ELLIPSE:
				if (!LoadCB(cbStrokeEllipse)) goto abort;
				goto commonEllipse;
			case SPIC_INSCRIBE_FILL_ELLIPSE:
				if (!LoadCB(cbFillEllipse)) goto abort;
				r = GetRect();
				p[0].x = (r.left+r.right)/2.0;
				p[0].y = (r.top+r.bottom)/2.0;
				p[1].x = r.right - p[0].x;
				p[1].y = r.bottom - p[0].y;
				goto inscribeEllipse;
			case SPIC_FILL_ELLIPSE:
				if (!LoadCB(cbFillEllipse)) goto abort;
				goto commonEllipse;
			commonEllipse:
				p[0] = GetCoord();
				p[1] = GetCoord();
			inscribeEllipse:
				cbPtPt(context,p[0],p[1]);
				break;
			case SPIC_STROKE_POLYGON:
			{
				if (!LoadCB(cbStrokePolygon)) goto abort;
				word32[0] = GetInt32();
				word32[1] = sizeof(BPoint)*word32[0];
				BPoint *pts = (BPoint*)malloc(word32[1]);
				GetData(pts,word32[1]);
				cbIPtPB(context,word32[0],pts,GetInt8());
				free(pts);
				break;
			}
			case SPIC_FILL_POLYGON:
			{
				if (!LoadCB(cbFillPolygon)) goto abort;
				word32[0] = GetInt32();
				word32[1] = sizeof(BPoint)*word32[0];
				BPoint *pts = (BPoint*)malloc(word32[1]);
				GetData(pts,word32[1]);
				cbIPtP(context,word32[0],pts);
				free(pts);
				break;
			}
			case SPIC_STROKE_PATH:
				if (!LoadCB(cbStrokeShape)) goto abort;
				goto commonPath;
			case SPIC_FILL_PATH:
				if (!LoadCB(cbFillShape)) goto abort;
				goto commonPath;
			commonPath:					
				word32[0] = GetInt32();
				ptr[0] = GetData(word32[0]*4);
				word32[1] = GetInt32();
				ptr[1] = GetData(word32[1]*sizeof(BPoint));
				path.SetData(word32[0],word32[1],(uint32*)ptr[0],(BPoint*)ptr[1]);
				cbShape(context,&path);
				break;
			case SPIC_DRAW_STRING:
				if (!LoadCB(cbDrawString)) goto abort;
				GetString(str);
				f[0] = GetFloat();
				f[1] = GetFloat();
				cbSFF(context,str,f[0],f[1]);
				break;
			case SPIC_PLAY_PICTURE:
				if (!LoadCB(cbDrawPicture)) goto abort;
				p[0] = GetCoord();
				word32[0] = GetInt32();
				if ((word32[0]>=0) && (word32[0]<pictureLib->CountItems()))
					cbPicPt(context,(*pictureLib)[word32[0]],p[0]);
				break;
			case SPIC_SET_STATE:
			{
				if ((LoadCB(cbEnterStateChange)) != 0) cbN(context);
				PRINT(("begin state %d %d %d %d\n",ip,starts[0],size[0],starts[0]+size[0]));
				while (ip < (starts[0]+size[0])) {
					op = GetOp();
					size[1] = GetInt32();
					starts[1] = ip;
					switch (op) {
						case SPIC_ORIGIN:
							if (!LoadCB(cbSetOrigin)) goto stateAbort;
							goto commonCoordState;
						case SPIC_LOCATION:
							if (!LoadCB(cbSetPenLocation)) goto stateAbort;
							goto commonCoordState;
						commonCoordState:
							cbPt(context,GetCoord());
							break;
						case SPIC_DRAW_OP:
							if (!LoadCB(cbSetDrawOp)) goto stateAbort;
							cbI(context,GetInt16());
							break;
						case SPIC_LINE_MODE:
							if (!LoadCB(cbSetLineMode)) goto stateAbort;
							word16[0] = GetInt16();
							word16[1] = GetInt16();
							cbIIF(context,word16[0],word16[1],GetFloat());
							break;
						case SPIC_PEN_SIZE:
							if (!LoadCB(cbSetPenSize)) goto stateAbort;
							goto commonFloatState;
						case SPIC_SCALE:
							if (!LoadCB(cbSetScale)) goto stateAbort;
							goto commonFloatState;
						commonFloatState:
							cbF(context,GetFloat());
							break;
						case SPIC_FORE_COLOR:
							if (!LoadCB(cbSetForeColor)) goto stateAbort;
							goto commonColorState;
						case SPIC_BACK_COLOR:
							if (!LoadCB(cbSetBackColor)) goto stateAbort;
							goto commonColorState;
						commonColorState:
							cbC(context,GetColor());
							break;
						case SPIC_STIPPLE:
							if (!LoadCB(cbSetStipplePattern)) goto stateAbort;
							i64 = GetInt64();
							cbSp(context,*((pattern*)&i64));
							break;
						case SPIC_CLEAR_CLIP:
							if (!LoadCB(cbClipToRects)) goto stateAbort;
							cbRPI(context,NULL,0);
							break;
						case SPIC_CLIP_TO_RECTS:
						{
							if (!LoadCB(cbClipToRects)) goto stateAbort;
							word32[0] = size[1]/(sizeof(int32)*4) - 1;
							if (word32[0] == 0) {
								cbRPI(context,NULL,0);
							} else {
								BRect *rects = (BRect*)malloc(word32[0]*sizeof(BRect));
								GetInt32();
								GetInt32();
								GetInt32();
								GetInt32();
								for (int32 i=0;i<word32[0];i++) {
									rects[i].left = GetInt32();
									rects[i].top = GetInt32();
									rects[i].right = GetInt32();
									rects[i].bottom = GetInt32();
								}
								cbRPI(context,rects,word32[0]);
								free(rects);
							}
							break;
						}
						case SPIC_CLIP_TO_PICTURE:
							if (!LoadCB(cbClipToPicture)) goto stateAbort;
							word32[0] = GetInt32();
							p[0] = GetCoord();
							word32[1] = GetInt32();
							if ((word32[0]>=0) && (word32[0]<pictureLib->CountItems()))
								cbPicPtI(context,(*pictureLib)[word32[0]],p[0],word32[1]);
							break;
						case SPIC_FONT:
						{
							if ((LoadCB(cbEnterFontState)) != 0) cbN(context);
							bool famSet = false;
							bool stySet = false;
							PRINT(("begin font %d %d %d %d\n",ip,starts[1],size[1],starts[1]+size[1]));
							while (ip < (starts[1]+size[1])) {
								op = GetOp();
								size[2] = GetInt32();
								starts[2] = ip;
								switch (op) {
									case SPIC_FAMILY:
										if (!LoadCB(cbFontFamily)) goto fontAbort;
										famSet = true;
										GetString(str);
										cbS(context,str);
										break;
									case SPIC_STYLE:
										if (!LoadCB(cbFontStyle)) goto fontAbort;
										stySet = true;
										GetString(str);
										cbS(context,str);
										break;
									case SPIC_SPACING:
										if (!LoadCB(cbFontSpacing)) goto fontAbort;
										goto commonFontIntState;
									case SPIC_ENCODING:
										if (!LoadCB(cbFontEncoding)) goto fontAbort;
										goto commonFontIntState;
									case SPIC_FLAGS:
										if (!LoadCB(cbFontFlags)) goto fontAbort;
										goto commonFontIntState;
									case SPIC_FACES:
										if (!LoadCB(cbFontFaces)) goto fontAbort;
										goto commonFontIntState;
									commonFontIntState:
										cbI(context,GetInt32());
										break;
									case SPIC_SIZE:
										if (!LoadCB(cbFontSize)) goto fontAbort;
										goto commonFontFloatState;
									case SPIC_ROTATE:
										if (!LoadCB(cbFontRotate)) goto fontAbort;
										goto commonFontFloatState;
									commonFontFloatState:
										cbF(context,GetFloat());
										break;
									case SPIC_SHEAR:
										if (!LoadCB(cbFontShear)) goto fontAbort;
										f[0] = GetFloat();
										// This converts from radians to degrees
										cbF(context,(f[0]*(180.0/3.1415925635)+90.0));
										break;
									case SPIC_BPP:
										goto fontAbort;
										break;
									default:
									fontAbort:
										ip = starts[2]+size[2];
										break;
								}
							}
							if ((LoadCB(cbExitFontState)) != 0) cbN(context);
							PRINT(("exit font change\n"));
							break;
						}
						default:
						stateAbort:
							ip = starts[1]+size[1];
							break;
					}
				}
				if ((LoadCB(cbExitStateChange)) != 0) cbN(context);
				PRINT(("exit state change\n"));
				break;
			}
			case SPIC_PUSH_STATE:
				if (!LoadCB(cbPushState)) goto abort;
				cbN(context);
				break;
			case SPIC_POP_STATE:
				if (!LoadCB(cbPopState)) goto abort;
				cbN(context);
				break;
			case SPIC_PIXELS:
			{
				if (!LoadCB(cbDrawPixels)) goto abort;

				void *ptr;
				BRect src,dst;

				src = GetRect();			/* Source rect			*/
				dst = GetRect();			/* Destination rect		*/
				word32[0] = GetInt32();		/* Width				*/
				word32[1] = GetInt32();		/* Height				*/
				word32[2] = GetInt32();		/* Bytes per row		*/
				word32[3] = GetInt32();		/* Pixel format			*/
				word32[4] = GetInt32();		/* Flags				*/
				word32[5] = GetInt32();		/* Pixel data size		*/
				ptr = GetData(word32[5]);	/* Pixel data			*/
				cbPixels(context,src,dst,
					word32[0],word32[1],word32[2],
					word32[3],0,ptr);
				break;
			}
			case SPIC_BLIT:
				goto abort;
				break;
			default:
			abort:
				ip += size[0];
		};
	};
};

void TPicture::Swap()
{
	int32 op;
	int32 size[8];
	int32 starts[8];
	int32 word32[4];

	Rewind();
	while ((op = SwapOp()) != 0) {
		size[0] = SwapInt32();
		starts[0] = ip;
		switch (op) {
			case SPIC_MOVE_PEN:
				SwapCoord();
				break;
			case SPIC_STROKE_LINE:
				SwapCoord();
				SwapCoord();
				break;
			case SPIC_INSCRIBE_STROKE_ELLIPSE:
			case SPIC_INSCRIBE_FILL_ELLIPSE:
			case SPIC_STROKE_RECT:
			case SPIC_FILL_RECT:
				SwapRect();
				break;
			case SPIC_INSCRIBE_STROKE_ARC:
			case SPIC_INSCRIBE_FILL_ARC:
			case SPIC_STROKE_ROUNDRECT:
			case SPIC_FILL_ROUNDRECT:
				SwapRect();
				SwapCoord();
				break;
			case SPIC_STROKE_BEZIER:
			case SPIC_FILL_BEZIER:
				SwapCoord();
				SwapCoord();
				SwapCoord();
				SwapCoord();
				break;
			case SPIC_STROKE_ARC:
			case SPIC_FILL_ARC:
				SwapCoord();
				SwapCoord();
				SwapFloat();
				SwapFloat();
				break;
			case SPIC_STROKE_ELLIPSE:
			case SPIC_FILL_ELLIPSE:
				SwapCoord();
				SwapCoord();
				break;
			case SPIC_STROKE_POLYGON:
				word32[0] = SwapInt32();
				while (word32[0]--) SwapCoord();
				SwapInt8();
				break;
			case SPIC_FILL_POLYGON:
				word32[0] = SwapInt32();
				while (word32[0]--) SwapCoord();
				break;
			case SPIC_STROKE_PATH:
			case SPIC_FILL_PATH:
				word32[0] = SwapInt32();
				while (word32[0]--) SwapInt32();
				word32[0] = SwapInt32();
				while (word32[0]--) SwapCoord();
				break;
			case SPIC_DRAW_STRING:
				SwapString();
				SwapFloat();
				SwapFloat();
				break;
			case SPIC_PLAY_PICTURE:
				SwapCoord();
				SwapInt32();
				break;
			case SPIC_SET_STATE:
			{
				while (ip < (starts[0]+size[0])) {
					op = SwapOp();
					size[1] = SwapInt32();
					starts[1] = ip;
					switch (op) {
						case SPIC_ORIGIN:
						case SPIC_LOCATION:
							SwapCoord();
							break;
						case SPIC_DRAW_OP:
							SwapInt16();
							break;
						case SPIC_LINE_MODE:
							SwapInt16();
							SwapInt16();
							SwapFloat();
							break;
						case SPIC_PEN_SIZE:
						case SPIC_SCALE:
							SwapFloat();
							break;
						case SPIC_FORE_COLOR:
						case SPIC_BACK_COLOR:
							SwapColor();
							break;
						case SPIC_STIPPLE:
							SwapInt64();
							break;
						case SPIC_CLEAR_CLIP:
							break;
						case SPIC_CLIP_TO_RECTS:
							word32[0] = size[1]/sizeof(BRect);
							while (word32[0]--) SwapRect();
							break;
						case SPIC_CLIP_TO_PICTURE:
							SwapInt32();
							SwapCoord();
							SwapInt32();
							break;
						case SPIC_FONT:
						{
							while (ip < (starts[1]+size[1])) {
								op = SwapOp();
								size[2] = SwapInt32();
								starts[2] = ip;
								switch (op) {
									case SPIC_FAMILY:
									case SPIC_STYLE:
										SwapString();
										break;
									case SPIC_SPACING:
									case SPIC_ENCODING:
									case SPIC_FLAGS:
									case SPIC_FACES:
										SwapInt32();
										break;
									case SPIC_SIZE:
									case SPIC_ROTATE:
									case SPIC_SHEAR:
										SwapFloat();
										break;
									case SPIC_BPP:
										goto fontAbort;
										break;
									default:
									fontAbort:
										ip = starts[2]+size[2];
										break;
								};
							};
							break;
						};
						default:
							ip = starts[1]+size[1];
							break;
					};
				};
				break;
			}
			case SPIC_PUSH_STATE:
			case SPIC_POP_STATE:
				break;
			case SPIC_PIXELS:
				SwapRect();			/* Source rect			*/
				SwapRect();			/* Destination rect		*/
				SwapInt32();		/* Width				*/
				SwapInt32();		/* Height				*/
				SwapInt32();		/* Bytes per row		*/
				SwapInt32();		/* Pixel format			*/
				SwapInt32();		/* Flags				*/
				SwapInt32();		/* Pixel data size		*/
				ip = starts[0]+size[0];
				break;
			case SPIC_BLIT:
				goto abort;
				break;
			default:
			abort:
				ip = starts[0]+size[0];
		}
	}
}

static int32 OldPicMovePen(TPicture *pic, BPoint delta)
{
	pic->AddInt32(PIC_MOVEBY);
	pic->AddCoord(delta);
	return 0;
}

static int32 OldPicStrokeLine(TPicture *pic, BPoint p1, BPoint p2)
{
	PRINT(("OldPicStrokeLine\n"));
	pic->AddInt32(PIC_MOVETO);
	pic->AddCoord(p1);
	pic->AddInt32(PIC_LINETO);
	pic->AddCoord(p2);
	return 0;
}

static int32 OldPicStrokeRect(TPicture *pic, BRect r)
{
	PRINT(("OldPicStrokeRect\n"));
	pic->AddInt32(PIC_RECTFRAME_PAT);
	pic->AddRect(r);
	pic->AddInt64(*((uint64*)&pic->curPattern));
	return 0;
}

static int32 OldPicFillRect(TPicture *pic, BRect r)
{
	PRINT(("OldPicFillRect\n"));
	pic->AddInt32(PIC_RECTFILL);
	pic->AddRect(r);
	pic->AddInt64(*((uint64*)&pic->curPattern));
	return 0;
}

static int32 OldPicStrokeRoundRect(TPicture *pic, BRect r, BPoint radius)
{
	PRINT(("OldPicStrokeRoundRect\n"));
	pic->AddInt32(PIC_ROUND_RECT_FRAME);
	pic->AddRect(r);
	pic->AddCoord(radius);
	pic->AddInt64(*((uint64*)&pic->curPattern));
	return 0;
}

static int32 OldPicFillRoundRect(TPicture *pic, BRect r, BPoint radius)
{
	PRINT(("OldPicFillRoundRect\n"));
	pic->AddInt32(PIC_ROUND_RECT_FILL);
	pic->AddRect(r);
	pic->AddCoord(radius);
	pic->AddInt64(*((uint64*)&pic->curPattern));
	return 0;
}

static int32 OldPicStrokeArc(TPicture *pic, BPoint center, BPoint radius, 
	float startAngle, float endAngle)
{
	PRINT(("OldPicStrokeArc\n"));
	BRect r;
	r.top = center.y - radius.y;
	r.bottom = center.y + radius.y;
	r.left = center.x - radius.x;
	r.right = center.x + radius.x;
	pic->AddInt32(PIC_ARC_FRAME);
	pic->AddRect(r);
	pic->AddInt32(rint(startAngle));
	pic->AddInt32(rint(endAngle));
	pic->AddInt64(*((uint64*)&pic->curPattern));
	return 0;
}

static int32 OldPicFillArc(TPicture *pic, BPoint center, BPoint radius, 
	float startAngle, float endAngle)
{
	PRINT(("OldPicFillArc\n"));
	BRect r;
	r.top = center.y - radius.y;
	r.bottom = center.y + radius.y;
	r.left = center.x - radius.x;
	r.right = center.x + radius.x;
	pic->AddInt32(PIC_ARC_FILL);
	pic->AddRect(r);
	pic->AddInt32(rint(startAngle));
	pic->AddInt32(rint(endAngle));
	pic->AddInt64(*((uint64*)&pic->curPattern));
	return 0;
}

static int32 OldPicStrokeEllipse(TPicture *pic, BPoint center, BPoint radius)
{
	PRINT(("OldPicStrokeEllipse\n"));
	BRect r;
	r.top = center.y - radius.y;
	r.bottom = center.y + radius.y;
	r.left = center.x - radius.x;
	r.right = center.x + radius.x;
	pic->AddInt32(PIC_ELLIPSE_FRAME);
	pic->AddRect(r);
	pic->AddInt64(*((uint64*)&pic->curPattern));
	return 0;
}

static int32 OldPicFillEllipse(TPicture *pic, BPoint center, BPoint radius)
{
	PRINT(("OldPicFillEllipse\n"));
	BRect r;
	r.top = center.y - radius.y;
	r.bottom = center.y + radius.y;
	r.left = center.x - radius.x;
	r.right = center.x + radius.x;
	pic->AddInt32(PIC_ELLIPSE_FILL);
	pic->AddRect(r);
	pic->AddInt64(*((uint64*)&pic->curPattern));
	return 0;
}

static int32 OldPicStrokePolygon(TPicture *pic, int32 ptCount, BPoint *p, bool closed)
{
	PRINT(("OldPicStrokePolygon\n"));
	BRect r;
	r.left = r.right = p->x;
	r.top = r.bottom = p->y;
	for (int32 i=1;i<ptCount;i++) {
		if (p[i].x < r.left) r.left = p[i].x;
		if (p[i].x > r.right) r.right = p[i].x;
		if (p[i].y < r.top) r.top = p[i].y;
		if (p[i].y > r.bottom) r.bottom = p[i].y;
	};
	pic->AddInt32(PIC_FRAMEPOLY);
	pic->AddRect(r);
	pic->AddInt64(*((uint64*)&pic->curPattern));
	pic->AddInt32(closed);
	pic->AddInt32(ptCount);
	for (int32 i=0;i<ptCount;i++) {
		pic->AddInt32(rint(p[i].x));
		pic->AddInt32(rint(p[i].y));
	};
	return 0;
}

static int32 OldPicFillPolygon(TPicture *pic, int32 ptCount, BPoint *p)
{
	PRINT(("OldPicFillPolygon\n"));
	BRect r;
	r.left = r.right = p->x;
	r.top = r.bottom = p->y;
	for (int32 i=1;i<ptCount;i++) {
		if (p[i].x < r.left) r.left = p[i].x;
		if (p[i].x > r.right) r.right = p[i].x;
		if (p[i].y < r.top) r.top = p[i].y;
		if (p[i].y > r.bottom) r.bottom = p[i].y;
	};
	pic->AddInt32(PIC_FILLPOLY);
	pic->AddRect(r);
	pic->AddInt32(ptCount);
	for (int32 i=0;i<ptCount;i++) {
		pic->AddInt32(rint(p[i].x));
		pic->AddInt32(rint(p[i].y));
	};
	pic->AddInt64(*((uint64*)&pic->curPattern));
	return 0;
}

static int32 OldPicDrawString(TPicture *pic, char *string, float spacing1, float spacing2)
{
	PRINT(("OldPicDrawString\n"));
	pic->AddInt32(PIC_DRAW_STRING);
	pic->AddString(string);
	pic->AddFloat(spacing1);
	pic->AddFloat(spacing2);
	return 0;
}

static int32 OldPicSetLocation(TPicture *pic, BPoint p)
{
	PRINT(("OldPicSetLocation\n"));
	pic->AddInt32(PIC_MOVETO);
	pic->AddCoord(p);
	return 0;
}

static int32 OldPicSetDrawOp(TPicture *pic, int32 drawOp)
{
	PRINT(("OldPicSetDrawOp\n"));
	pic->AddInt32(PIC_SET_DRAW_MODE);
	pic->AddInt32(drawOp);
	return 0;
}

static int32 OldPicSetPenSize(TPicture *pic, float penSize)
{
	PRINT(("OldPicPenSize\n"));
	pic->AddInt32(PIC_SET_PEN_SIZE);
	pic->AddFloat(penSize);
	return 0;
}

static int32 OldPicSetScale(TPicture *pic, float scale)
{
	PRINT(("OldPicSetScale\n"));
	pic->AddInt32(PIC_SET_SCALE);
	pic->AddFloat(scale);
	return 0;
}

static int32 OldPicSetForeColor(TPicture *pic, rgb_color color)
{
	PRINT(("OldPicSetForeColor\n"));
	pic->AddInt32(PIC_FORE_COLOR);
	pic->AddColor(color);
	return 0;
}

static int32 OldPicSetBackColor(TPicture *pic, rgb_color color)
{
	PRINT(("OldPicSetBackColor\n"));
	pic->AddInt32(PIC_BACK_COLOR);
	pic->AddColor(color);
	return 0;
}

static int32 OldPicSetPattern(TPicture *pic, pattern pat)
{
	PRINT(("OldPicSetPatterm\n"));
	pic->curPattern = pat;
	return 0;
}

static int32 OldPicSetFontFamily(TPicture *pic, char *string)
{
	PRINT(("OldPicSetFontFamily\n"));
	pic->AddInt32(PIC_SET_FONT_FAMILY);
	pic->AddString(string);
	return 0;
}

static int32 OldPicSetFontStyle(TPicture *pic, char *string)
{
	PRINT(("OldPicSetFontStyle\n"));
	pic->AddInt32(PIC_SET_FONT_STYLE);
	pic->AddString(string);
	return 0;
}

static int32 OldPicSetFontSpacing(TPicture *pic, int32 spacing)
{
	PRINT(("OldPicSetFontSpacing\n"));
	pic->AddInt32(PIC_SET_FONT_SPACING);
	pic->AddInt32(spacing);
	return 0;
}

static int32 OldPicSetFontEncoding(TPicture *pic, int32 encoding)
{
	PRINT(("OldPicSetFontEncoding\n"));
	pic->AddInt32(PIC_SET_FONT_ENCODING);
	pic->AddInt32(encoding);
	return 0;
}

static int32 OldPicSetFontFlags(TPicture *pic, int32 flags)
{
	PRINT(("OldPicSetFontFlags\n"));
	pic->AddInt32(PIC_SET_FONT_FLAGS);
	pic->AddInt32(flags);
	return 0;
}

static int32 OldPicSetFontFaces(TPicture *pic, int32 faces)
{
	PRINT(("OldPicSetFontFaces\n"));
	pic->AddInt32(PIC_SET_FONT_FACES);
	pic->AddInt32(faces);
	return 0;
}

static int32 OldPicSetFontBPP(TPicture *pic, int32 bpp)
{
	PRINT(("OldPicSetFontBPP\n"));
	pic->AddInt32(PIC_SET_FONT_BPP);
	pic->AddInt32(bpp);
	return 0;
}

static int32 OldPicSetFontSize(TPicture *pic, float size)
{
	PRINT(("OldPicSetFontSize\n"));
	pic->AddInt32(PIC_SET_FONT_SIZE);
	pic->AddFloat(size);
	return 0;
}

static int32 OldPicSetFontShear(TPicture *pic, float shear)
{
	PRINT(("OldPicSetFontShear\n"));
	pic->AddInt32(PIC_SET_FONT_SHEAR);
	pic->AddFloat(shear);
	return 0;
}

static int32 OldPicSetFontRotate(TPicture *pic, float rotate)
{
	PRINT(("OldPicSetFontRotate\n"));
	pic->AddInt32(PIC_SET_FONT_ROTATE);
	pic->AddFloat(rotate);
	return 0;
}

static int32 OldPicDrawPixels(TPicture *pic,
	BRect srcRect, BRect dstRect,
	int32 width, int32 height, int32 bytesPerRow,
	int32 format, int32, void *pixels)
{
	PRINT(("OldPicSetDrawPixels\n"));
	pic->AddInt32(PIC_BLIT);
	pic->AddRect(srcRect);
	pic->AddRect(dstRect);
	pic->AddInt32(format);
	if ((format&0xFF) == B_RGB32) {
		pic->AddInt32(32);
	} else if ((format&0xFF) == B_RGB15) {
		pic->AddInt32(16);
	} else if ((format&0xFF) == B_RGB16) {
		pic->AddInt32(16);
	} else if ((format&0xFF) == B_GRAY1) {
		pic->AddInt32(1);
	} else {
		pic->AddInt32(8);
	};
	pic->AddInt32(bytesPerRow);
	pic->AddInt32(0);
	pic->AddInt32(0);
	pic->AddInt32(width-1);
	pic->AddInt32(height-1);
	pic->AddInt32(height*bytesPerRow);
	pic->AddData(pixels,height*bytesPerRow);
	return 0;
}

void* OldPicCallbacks[] = {
	NULL,

	(void*)OldPicMovePen,
	
	(void*)OldPicStrokeLine,

	(void*)OldPicStrokeRect,
	(void*)OldPicFillRect,

	(void*)OldPicStrokeRoundRect,
	(void*)OldPicFillRoundRect,

	NULL,
	NULL,

	(void*)OldPicStrokeArc,
	(void*)OldPicFillArc,

	(void*)OldPicStrokeEllipse,
	(void*)OldPicFillEllipse,

	(void*)OldPicStrokePolygon,
	(void*)OldPicFillPolygon,

	NULL,
	NULL,

	(void*)OldPicDrawString,
	(void*)OldPicDrawPixels,
	NULL,

	NULL,
	NULL,
	NULL,
	NULL,

	NULL,
	NULL,
	NULL,
	NULL,

	NULL,
	(void*)OldPicSetLocation,
	(void*)OldPicSetDrawOp,
	NULL,
	(void*)OldPicSetPenSize,
	(void*)OldPicSetForeColor,
	(void*)OldPicSetBackColor,
	(void*)OldPicSetPattern,
	(void*)OldPicSetScale,

	(void*)OldPicSetFontFamily,
	(void*)OldPicSetFontStyle,
	(void*)OldPicSetFontSpacing,
	(void*)OldPicSetFontSize,
	(void*)OldPicSetFontRotate,
	(void*)OldPicSetFontEncoding,
	(void*)OldPicSetFontFlags,
	(void*)OldPicSetFontShear,
	(void*)OldPicSetFontBPP,
	(void*)OldPicSetFontFaces
};

void convert_new_to_old(
	void *newPtr, int32 newSize,
	void **oldPtr, int32 *oldSize)
{
	PRINT(("Playing new (%08x,%d) to old...\n",newPtr,newSize));
	if (!newPtr || !newSize) return;
	TPicture p(newPtr,newSize,NULL);
	p.Play(OldPicCallbacks,46,&p);
	p.AddInt32(PIC_END_PICTURE);
	p.Trim();
	*oldPtr = p.data;
	*oldSize = p.dataPtr;
};

void swap_data(void *ptr, int32 size)
{
	if (!ptr || !size) return;
	TPicture p(ptr,size,NULL);
	p.Swap();
};

status_t do_playback(
	void *ptr, int32 size, BArray<BPicture*> &pictureLib,
	void **callbacks, int32 callbackCount,
	void *userData)
{
	TPicture p(ptr,size,&pictureLib);
	p.Play(callbacks,callbackCount,userData);
	return B_OK;
};
	
