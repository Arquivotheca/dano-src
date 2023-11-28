//******************************************************************************
//
//	File:		Convert.h
//
//	Description:	Convert object for PostScript driver
//
//	Copyright 1996, International Lorienne Inc.
//
//******************************************************************************

#if !defined(CONVERT)
#define CONVERT

#include <InterfaceDefs.h>
#include <File.h>
#include <Message.h>
#include <Window.h>
#include <View.h>

#include <List.h>
#include <String.h>
#include <Shape.h>
#include <Path.h>



typedef struct
{
	float	red;
	float	green;
	float	blue;
} pscolor;


class context
{
public:
	context();
	context(context *oldOne);
	~context(){};
	context *		previous;
	pscolor			foreColor;
	pscolor			backColor;
	pattern			thePattern;
	int32			drawOp;
	BFont			font;
	BPoint			origin;
	float			scaling;
	float			penSize;
};

class FontHandler;
class Convert;

class PsShapeIterator : public BShapeIterator
{
 public:
						PsShapeIterator(Convert*, BFile*);
	status_t			IterateMoveTo(BPoint*);
	status_t			IterateLineTo(int32, BPoint*);
	status_t			IterateBezierTo(int32, BPoint*);
	status_t			IterateClose();
	
 private:
	Convert				*fConvert;
	BFile				*fCurOutputFile;
};

class Convert  {
public:
					Convert(BFile *NewInput, FontHandler *fonthandler);
	virtual			~Convert();
	
	virtual	void	InitConvertion(BMessage *setupMessage);
	virtual	long	DoConvertionForPicture();
	
	virtual void 	WriteHeader(BDataIO *toFile);
	virtual void 	ConvertCoord(float *x,float *y);
	virtual void	DrawPrimitive(const char *drawString);
	virtual long	Error() {return error;};
	virtual void	NewPage();
	virtual	void 	TopOfPage();
	virtual	void 	EndOfPage();

	virtual void	WriteBlackAndWhiteRaster(uint16 *input_buffer, long bpr, long columns, long lines);
			void 	WriteColor8Raster(uint8 *input_buffer, long bpr, long columns, long lines);
			void 	WriteColor24Raster(uint8 *input_buffer, long bpr, long columns, long lines);
			void 	WriteGreyRaster(uint8 *input_buffer, long bpr, long columns, long lines);

			void 	do_line(uint16 *input_buffer, long rowSize, long columns, long vp, long left, long right, long cnt);
			void 	do_line_color_8(uint8 *input_buffer, long rowSize, long columns, long vp, long left, long right, long cnt);
			void 	do_line_color_24(uint8 *input_buffer, long rowSize, long columns, long vp, long left, long right, long cnt);
			void 	do_line_grey(uint8 *input_buffer, long rowSize, long columns, long vp, long left, long right, long cnt);

	const char*		DocumentBodyFile();

			void	WriteImageFunction(BDataIO*);
			void	WriteLevel1ImageFunction(BDataIO*);
			void	WriteLevel2ImageFunction(BDataIO*);

			void	Restack();
			
			void	WriteFontStuff();
			
			int32	EnterStateChange();
			int32	ExitStateChange();

			int32	MovePen(BPoint delta);
			
			int32	StrokeLine(BPoint p1, BPoint p2);

			int32	StrokeRect(BRect r);
			int32	FillRect(BRect r);
			int32 	CommonRect(BRect r, bool isFilled);

			int32	StrokeRoundRect(BRect r, BPoint radius);
			int32	FillRoundRect(BRect r, BPoint radius);
			int32	CommonRoundRect(BRect r, BPoint radius, bool isFilled);
			
			int32 	StrokeBezier(BPoint *pt);
			int32 	FillBezier(BPoint *pt);
			int32	CommonBezier(BPoint *pt, bool isFilled);

			int32	StrokeArc(BPoint center, BPoint radius, 
						float startAngle, float endAngle);
			int32	FillArc(BPoint center, BPoint radius, 
						float startAngle, float endAngle);
			int32	CommonArc(BPoint center, BPoint radius, 
						float startAngle, float endAngle, bool isFilled);

						
			int32	StrokeEllipse(BPoint center, BPoint radius);
			int32	FillEllipse(BPoint center, BPoint radius);
			int32	CommonEllipse(BPoint center, BPoint radius, bool isFilled);
			
			int32	StrokePolygon(int32 ptCount, BPoint *p, bool closed);
			int32	FillPolygon(int32 ptCount, BPoint *p);
			int32	CommonPolygon(int32 ptCount, BPoint *p, bool closed = true,
									bool isFilled = true);
			
			int32 	StrokeShape(BShape*);
			int32	FillShape(BShape*);
			int32 	CommonShape(BShape*, bool isFilled);
			
			int32	DrawString(char *string, float spacing1, float spacing2);

			int32	ClipToRects(BRect *rects, int32 count);
			int32	ClipToPicture(BPicture *pic, BPoint origin, uint32 inverse);

			int32	PushState();
			int32	PopState();

			int32	SetOrigin(BPoint p);
			int32	SetLocation(BPoint p);

			int32	SetDrawOp(int32 drawOp);

			int32	SetPenSize(float penSize);

			int32	SetScale(float scale);

			int32	SetForeColor(rgb_color color);
			int32	SetBackColor(rgb_color color);

			int32	SetPattern(pattern pat);

			int32	SetFontFamily(char *string);
			int32	SetFontStyle(char *string);
			int32	SetFontSpacing(int32 spacing);
			int32	SetFontEncoding(int32 encoding);
			int32	SetFontFlags(int32 flags);
			int32	SetFontFaces(int32 faces);
			int32	SetFontBPP(int32 bpp);
			int32	SetFontSize(float size);
			int32	SetFontShear(float shear);
			int32	SetFontRotate(float rotate);

			int32 	DrawPixels(
						BRect src_rect, BRect dst_rect,
						int32 width, int32 height, int32 rowbyte,
						int32 format, uint8 *pixels);
			void 	Color8DrawPixels(
						BRect src_rect, BRect dst_rect, int32 width,
						int32 height, int32 rowbyte,
						uint8 *pixels);
			void 	RGB16DrawPixels(
						BRect src_rect, BRect dst_rect, int32 width,
						int32 height, int32 rowbyte,
						uint8 *pixels);
			void 	RGB32DrawPixels(
						BRect src_rect, BRect dst_rect, int32 width,
						int32 height, int32 rowbyte,
						uint8 *pixels);

	long			xPageSize;
	long			yPageSize;
	bool 			landscape;
	float			scale;
	float			rotation;
	pscolor			cur_color;
	context *		ctx;

	bool			fontModified;
	bool			fIsRasterizing;
	bool			mustWriteCmap;

	void			SetLanguageLevel(int);
	
	void			SetImageableArea(BRect);

	status_t		TestRasterizePictures(int32 numPics, BPicture *pics[], BPoint pos[],
								BRect clips[], BRect printableRect, int32 resolution);


	status_t		RasterizePictures(int32 numPics, BPicture *pics[], BPoint pos[],
								BRect clips[], BRect printableRect, int32 resolution);
								
	void			RasterizeRect(BRect RRect, int32 numPics, BPicture *pics[], BPoint pos[],
									BRect clips[], int32 resolution);
protected:

	bool			fInColor;
	bool			fHighQuality;

	void			PSComment(const char *comment);
	void 			DscComment(const char *comment);
	status_t 		OutputRawData(char *data, int32 length);
	status_t 		OutputRawData2(char *data, int32 length);

	status_t		SetupBitmap(BRect, int32 resolution);
	BBitmap			*fRasterBitmap;
	BView			*fRasterView;

	int32			fReqResolution;

	void			WriteSystemCMap(BDataIO*);
	float 			ColorToGray(float R, float G, float B);
	void 			WriteColor(float red, float green, float blue);

	void			StartClipElement();
	void			EndClipElement();

	bool			fStartClipIsActive;
	bool			fInverseClip;

	BFile*			CreateTmpFile(BString *path=NULL);
	void			WriteContext();
	FontHandler		*fFontHandler;

	int32			fPageCounter;

	BFile			*input;
	BFile			*fCurOutputFile;

	BString			fOutputFilePath;
	
	PsShapeIterator	*fShapeIterator;
	
	int32			fPathOnly;
	BRect			fClipRect;
	
	long			error;
	BRect			Imageable;
	BList			pages;
	
	int				languageLevel;
	
	BString			VersionName();
};


#endif
