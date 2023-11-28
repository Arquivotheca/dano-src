//******************************************************************************
//
//	File:		Convert.cpp
//
//	Description:	Convert object for PostScript driver
//
//******************************************************************************

#include "Convert.h"
#include "FontHandler.h"


#include <Application.h>
#include <String.h>
#include <AppFileInfo.h>
#include <Picture.h>
#include <Roster.h>

#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <time.h>
#include <sys/stat.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

#include <InterfaceDefs.h>
#include <File.h>
#include <Path.h>
#include <FindDirectory.h>
#include <Debug.h>
#include <UTF8.h>
#include <Bitmap.h>
#include <Region.h>


//#define P _sPrintf
#define PP //printf
#define P //for(int z=0;z<statedepth;z++){printf("\t");}printf



const int32 BAND_SIZE = 50;

/*-- Private undocumented structs, API and enums (Beginning) --*/

int statedepth = 0;

#define NB_CALLBACK_FUNCTIONS	48


/*-- Private undocumented structs, API and enums (End) --*/

extern void SendOutput(BDataIO*, const char*);
extern void SendOutput(BDataIO*, const char*, long size);
extern int compress(char *outrow, char *inrow, long bytes);

int32 ps_MovePen(Convert *cp, BPoint delta);
int32 ps_StrokeLine(Convert *cp, BPoint p1, BPoint p2);
int32 ps_StrokeRect(Convert *cp, BRect r);
int32 ps_FillRect(Convert *cp, BRect r);
int32 ps_StrokeRoundRect(Convert *cp, BRect r, BPoint radius);
int32 ps_FillRoundRect(Convert *cp, BRect r, BPoint radius);
int32 ps_StrokeBezier(Convert *cp, BPoint *pt);
int32 ps_FillBezier(Convert *cp, BPoint *pt);
int32 ps_StrokeArc(Convert *cp, BPoint center, BPoint radius, float startAngle, float endAngle);
int32 ps_FillArc(Convert *cp, BPoint center, BPoint radius, float startAngle, float endAngle);
int32 ps_StrokeEllipse(Convert *cp, BPoint center, BPoint radius);
int32 ps_FillEllipse(Convert *cp, BPoint center, BPoint radius);
int32 ps_StrokePolygon(Convert *cp, int32 ptCount, BPoint *p, bool closed);
int32 ps_FillPolygon(Convert *cp, int32 ptCount, BPoint *p);
int32 ps_StrokeShape(Convert *cp, BShape *shape);
int32 ps_FillShape(Convert *cp, BShape *shape);
int32 ps_DrawString(Convert *cp, char *str_ptr, float float1, float float2);
int32 ps_ClipToRects(Convert *cp, BRect *rects, int32 count);
int32 ps_ClipToPicture(Convert *cp, BPicture *pic, BPoint origin, uint32 inverse);
int32 ps_PushState(Convert *cp);
int32 ps_PopState(Convert *cp);
int32 ps_SetOrigin(Convert *cp, BPoint p);
int32 ps_SetLocation(Convert *cp, BPoint p);
int32 ps_SetDrawOp(Convert *cp, int32 drawOp);
int32 ps_SetPenSize(Convert *cp, float penSize);
int32 ps_SetScale(Convert *cp, float scale);
int32 ps_SetForeColor(Convert *cp, rgb_color color);
int32 ps_SetBackColor(Convert *cp, rgb_color color);
int32 ps_SetPattern(Convert *cp, pattern pat);
int32 ps_SetFontFamily(Convert *cp, char *string);
int32 ps_SetFontStyle(Convert *cp, char *string);
int32 ps_SetFontSpacing(Convert *cp, int32 spacing);
int32 ps_SetFontEncoding(Convert *cp, int32 encoding);
int32 ps_SetFontFlags(Convert *cp, int32 flags);
int32 ps_SetFontFaces(Convert *cp, int32 faces);
int32 ps_SetFontBPP(Convert *cp, int32 bpp);
int32 ps_SetFontSize(Convert *cp, float size);
int32 ps_SetFontShear(Convert *cp, float shear);
int32 ps_SetFontRotate(Convert *cp, float rotate);
int32 ps_DrawPixels(Convert *cp, 
	BRect src_rect, BRect dst_rect,
	int32 width, int32 height, int32 rowbyte,
	int32 format, int32 flags, uint8 *pixels);
int32 ps_EnterStateChange(Convert *cp);
int32 ps_ExitStateChange(Convert *cp);


//==============================================================================
//
//	context()
//
//	Constructor for context
//
//	Initialize a new context object, with default values
//
//	context objects are used to manipulate the context stack for BPicture
//
//==============================================================================

context::context() 
{
	previous = NULL;
}

context::context(context *oldOne)
{
	previous=oldOne;

	// initialize drawing parameters
	foreColor.red = oldOne->foreColor.red;
	foreColor.green = oldOne->foreColor.green;
	foreColor.blue = oldOne->foreColor.blue;
	backColor.red = oldOne->backColor.red;
	backColor.green = oldOne->backColor.green;
	backColor.blue = oldOne->backColor.blue;
	font = oldOne->font;
	thePattern = oldOne->thePattern;
	drawOp = oldOne->drawOp;
	origin.Set(0,0);
	scaling = 1.0;
	penSize = oldOne->penSize;
}

//==============================================================================
//
//	Convert (
//			)
//
//	Constructor for Convert
//
//	Initialize a new Convert object, without any reference to a file
//
//==============================================================================

Convert::Convert(BFile *NewInput, FontHandler *fonthandler)
{
	fPathOnly = 0;
	fPageCounter = 0;
	
	ctx = NULL;
	fCurOutputFile = NULL;
	fRasterBitmap = NULL;
	fRasterView = NULL;
	fReqResolution = 300;	
	fFontHandler = fonthandler;

	fStartClipIsActive = false;

	fInColor = false;
	fHighQuality = false;
		
	if (NewInput == NULL) {
		fprintf(stderr, "NULL pointer given to Convert constructor\n");
		error = -1;
		return;
	}
	else error = B_NO_ERROR;

	fIsRasterizing = false;

	mustWriteCmap = true/*false*/;

	// Default for page size
	xPageSize = 612;
	yPageSize = 792;

	// Default for lanscape flag and scale factor
	landscape = false;
	scale = 1.;
	rotation = 0.;

	input = NewInput;
	if(input->IsReadable() == false) {
		PP("Unable to open input file!\n");
		error = -2;
		return;
	}
	
	fCurOutputFile = CreateTmpFile(&fOutputFilePath);	
	fShapeIterator = new PsShapeIterator(this, fCurOutputFile);
}

//==============================================================================
//
//	~Convert()
//
//	Destructor.
//
//			Do close
//
//==============================================================================

Convert::~Convert()
{
	delete fCurOutputFile;
	delete fShapeIterator;
	delete fRasterBitmap;	// removes and deletes fRasterView
	unlink(DocumentBodyFile());
}

void
Convert::PSComment(const char *comment)
{
	char str[256];
	sprintf(str, "%% %s\n", comment);
	SendOutput(fCurOutputFile, str);
}

void
Convert::DscComment(const char *comment)
{
	char str[256];
	sprintf(str, "\n%%%%%s\n", comment);
	SendOutput(fCurOutputFile, str);
}

const char*
Convert::DocumentBodyFile()
{
	return fOutputFilePath.String();
}

//==============================================================================
//
//	WriteHeader 	(
//					)
//
//	Write the header of the postcript file. This will include two parts in the
//  final release:
//
//		- comments corresponding to the choices made by the user, after reading
//		  of the description of what is possible in the ppd file of the choosen
//		  printer,
//
//		- preamble containing the functions used by the translation module.
//
//	as for today, the first part is really basic, but the second part is OK.
//		
//==============================================================================

void Convert::WriteHeader(BDataIO *toFile)
{
	const short	DSC_NAME_LEN = 220;

	BFile		header("/system/add-ons/Print/Header", O_RDONLY);
	char		string[256];
	char		dateTime[256];
	char		name[DSC_NAME_LEN];
	tm			*asTime;
	time_t		secTime;
	BString 	v_info;

	// get the descriptive name for the job
	status_t err = input->ReadAttr("_spool/Description", B_STRING_TYPE, 0, name, DSC_NAME_LEN);
	if(err <= 0){
		sprintf(name, "<None>");
	}
	
	// get the BeOS version name
	v_info = VersionName();
		
	// Write comments	
	sprintf(string,"%%!PS-Adobe-3.0\n"); 							SendOutput(toFile, string); 
	sprintf(string,"%%%%Creator: %s\n", v_info.String()); 			SendOutput(toFile, string); 
	sprintf(string,"%%%%For: BeOS\n");		 						SendOutput(toFile, string); 
	sprintf(string,"%%%%Title: %s\n",name); 						SendOutput(toFile, string); 
	time (&secTime);
	asTime = localtime (&secTime);
	strftime(dateTime,256,"%m/%d/%y %I:%M %p",asTime);
	sprintf(string,"%%%%CreationDate: %s\n",dateTime); 				SendOutput(toFile, string); 
	// Printable surface has to be read in the ppd file
	sprintf(string,"%%%%BoundingBox: %d %d %ld %ld\n",0,0,xPageSize, yPageSize);
	 																SendOutput(toFile, string); 

	if (landscape) {
		sprintf(string,"%%%%Orientation: Landscape\n");				SendOutput(toFile, string); 
	}
	else {
		sprintf(string,"%%%%Orientation: Portrait\n");				SendOutput(toFile, string); 
	}
	
	sprintf(string,"%%%%EndComments\n\n"); 							SendOutput(toFile, string); 

	// read and write header file
	long size = header.Seek(0, SEEK_END);
	char *buffer = (char *) malloc(size*sizeof(char));
	header.Seek(0, SEEK_SET);
	header.Read(buffer,size);
	SendOutput(toFile, buffer, size); 

	free(buffer);

	if(mustWriteCmap) {
		WriteSystemCMap(toFile);
	}

//	WriteLevel2ImageFunction(toFile);
	WriteImageFunction(toFile);
}

void Convert::WriteContext()
{
	char string[256];
	sprintf(string, "%8.4f %8.4f sc "
					"%8.4f %8.4f t "
					"%8.4f w ",
					ctx->scaling, ctx->scaling,
					ctx->origin.x,ctx->origin.y,
					ctx->penSize);			
	SendOutput(fCurOutputFile, string);
	WriteColor(ctx->foreColor.red, ctx->foreColor.green, ctx->foreColor.blue);
}

void Convert::Restack()
{
	char string[1024];
	sprintf(string,	"currentlinewidth currentfont currentpoint currentrgbcolor _fs _fn _fh _fsh _fr\n"
					"gr /_fr exch def /_fsh exch def /_fh exch def /_fn exch def /_fs exch def setrgbcolor m setfont w\n"
					"gs %8.4f %8.4f sc %8.4f %8.4f t %% Restack\n",
					ctx->scaling,ctx->scaling,
					ctx->origin.x,-ctx->origin.y);
	SendOutput(fCurOutputFile, string); 
};

void Convert::ConvertCoord(float *x,float *y)
{
	float newx = (*x);
	float newy = (*y);

	// Take the origin into account
	// It's tricky but it seems to work very well!
	if (ctx)
	{ // In rasterizing mode, ctx is NULL
		context *c = ctx;
		BPoint delta(c->origin);
		while ((c = c->previous) != NULL)
			delta += c->origin;
		newx += delta.x;
		newy += delta.y;
	}

	if(!fIsRasterizing && (newy > Imageable.Height())) {
		newy -= fClipRect.top;
	}

	newy = yPageSize/scale - Imageable.top - newy;
	newx += Imageable.left;

	*x = newx;
	*y = newy;
}

BString Convert::VersionName()
{
	BString Name = "BeOS"; // in case version info isn't found

	app_info info;
	if (be_app->GetAppInfo(&info) == B_NO_ERROR) {
		BFile pserver(&info.ref, O_RDONLY);
		if (pserver.InitCheck() == B_NO_ERROR) {
			BAppFileInfo info(&pserver);
			if (info.InitCheck() == B_NO_ERROR) {
				version_info version;
				if (info.GetVersionInfo(&version, B_APP_VERSION_KIND)
					== B_NO_ERROR) {
						Name = version.short_info;
				}
			}
		}
	}
	
	return Name;
}

void Convert::DrawPrimitive(const char *drawString)
{
	char string[256];
	pattern p = ctx->thePattern;
	
	if ((p.data[0] == 0xff) && 
		(p.data[1] == 0xff) && 
		(p.data[2] == 0xff) && 
		(p.data[3] == 0xff) && 
		(p.data[4] == 0xff) && 
		(p.data[5] == 0xff) && 
		(p.data[6] == 0xff) && 
		(p.data[7] == 0xff)) {							// B_SOLID_HIGH
		if ((cur_color.red != ctx->foreColor.red) || (cur_color.green != ctx->foreColor.green) ||(cur_color.blue != ctx->foreColor.blue)) {
			WriteColor(ctx->foreColor.red,ctx->foreColor.green,ctx->foreColor.blue);
			cur_color = ctx->foreColor;
		}
		SendOutput(fCurOutputFile, drawString); 
	}
	else if ((p.data[0] == 0x00) && 
		(p.data[1] == 0x00) && 
		(p.data[2] == 0x00) && 
		(p.data[3] == 0x00) && 
		(p.data[4] == 0x00) && 
		(p.data[5] == 0x00) && 
		(p.data[6] == 0x00) && 
		(p.data[7] == 0x00)) {							// B_SOLID_LOW
		if ((cur_color.red != ctx->backColor.red) || (cur_color.green != ctx->backColor.green) ||(cur_color.blue != ctx->backColor.blue)) {
			WriteColor(ctx->backColor.red,ctx->backColor.green,ctx->backColor.blue);
			cur_color = ctx->backColor;
		}
		SendOutput(fCurOutputFile, drawString); 
	}
	else {												// We use pattern
		if (ctx->drawOp != B_OP_OVER) {
			if ((cur_color.red != ctx->backColor.red) || (cur_color.green != ctx->backColor.green) ||(cur_color.blue != ctx->backColor.blue)) {
				WriteColor(ctx->backColor.red,ctx->backColor.green,ctx->backColor.blue);
				cur_color = ctx->backColor;
			}
			SendOutput(fCurOutputFile, drawString); 
		}
		// Set the correct pattern, according to p
		sprintf(string,"gs %% drawprimitive\n");
		SendOutput(fCurOutputFile, string); 	

		sprintf(string,"[\n");
		SendOutput(fCurOutputFile, string); 
	
		unsigned char val,test;
		for (long i=0;i<8;i++) {
			val = p.data[i];
			test = 0x01;
			for (long j=0;j<8;j++) {
				if (val & test) {
					sprintf(string,"1 ");
				}
				else {
					sprintf(string,"0 ");
				}
				SendOutput(fCurOutputFile, string); 
				test = test << 1;		
			}	
			sprintf(string,"\n");
			SendOutput(fCurOutputFile, string); 
		}
		sprintf(string,"] CreatePattern\n");
		SendOutput(fCurOutputFile, string); 

		sprintf(string,"[/Pattern [/DeviceRGB]] setcolorspace\n");
		SendOutput(fCurOutputFile, string); 

		sprintf(string,"%8.2f %8.2f %8.2f CurPattern setcolor\n",ctx->foreColor.red,ctx->foreColor.green,ctx->foreColor.blue);
		SendOutput(fCurOutputFile, string); 

		SendOutput(fCurOutputFile, drawString); 

		sprintf(string,"gr %% drawprimitive\n");
		SendOutput(fCurOutputFile, string); 
	}	
}

//==============================================================================
//
//	InitConvertion 	(
//				)
//
//	Write header.
//		
//==============================================================================

void Convert::InitConvertion(BMessage *setupMessage)
{
	// Paper rect
	BRect paper_rect;
	setupMessage->FindRect("paper_rect",&paper_rect);
	if (paper_rect.IsValid()) {
		xPageSize = (long)paper_rect.Width();
		yPageSize = (long)paper_rect.Height();
	}

	// Get imageable rect
	BRect printable_rect;
	if(setupMessage->HasRect("printable_rect")) {
		printable_rect = setupMessage->FindRect("printable_rect");
		if(printable_rect.IsValid()) {
			Imageable = printable_rect;
		}
	}
	
	if (setupMessage->HasFloat("scaling")) {
		scale = setupMessage->FindFloat("scaling") / 100.;
	}

	if (setupMessage->HasInt32("orientation")) {
		int32 orient;
		setupMessage->FindInt32("orientation", &orient);
		landscape = (orient != 0);
		if(landscape) {
			int32 tmp = xPageSize;
			xPageSize = yPageSize;
			yPageSize = tmp;
			
			BRect rectmp((xPageSize - Imageable.bottom),
							Imageable.left,
							(xPageSize - Imageable.top),
							Imageable.right);
			Imageable = rectmp;
		}
	}
	
	if (setupMessage->HasBool("in_color"))
	{
		fInColor = setupMessage->FindBool("in_color");
		fInColor = fInColor && (languageLevel != 1);
		PP("languageLevel == %d, fInColor == %d\n", languageLevel, fInColor);
		if (fInColor)
		{
			if (setupMessage->FindBool("HighQuality", &fHighQuality) != B_OK)
				fHighQuality = false;
		}
	}	
}

int32 Convert::EnterStateChange()
{
	P("PostscriptEnterStateChange\n");
	return 0;
};

int32 Convert::ExitStateChange()
{
	P("PostscriptExitStateChange\n");
	return 0;
};

int32 Convert::MovePen(BPoint delta)
{
	P("PostscriptMovePen\n");
	char string[1024];
	delta.y = -delta.y;
	sprintf(string,"%8.8f %8.8f rmoveto\n",delta.x,delta.y);
	SendOutput(fCurOutputFile, string); 
	return 0;
};

int32 Convert::StrokeLine(BPoint p1, BPoint p2)
{
	P("PostscriptStrokeLine\n");
	char string[1024];
	ConvertCoord(&(p1.x),&(p1.y));
	ConvertCoord(&(p2.x),&(p2.y));
	sprintf(string,"%8.8f %8.8f m ",p1.x,p1.y);
	SendOutput(fCurOutputFile, string); 
	sprintf(string,"%8.8f %8.8f l %s\n",p2.x,p2.y, (fPathOnly>0)?"":"s");
	SendOutput(fCurOutputFile, string);
	return 0;
};

int32 Convert::StrokeRect(BRect r)
{
	P("PostscriptStrokeRect\n");
	CommonRect(r, false);
	return 0;
};

int32 Convert::FillRect(BRect r)
{
	P("PostscriptFillRect\n");
	CommonRect(r, true);
	return 0;
};

int32 Convert::CommonRect(BRect r, bool isFilled)
{
	P("PostscriptCommonRect\n");
	char string[1024];
	ConvertCoord(&(r.left),&(r.top));
	ConvertCoord(&(r.right),&(r.bottom));
	sprintf(string,"%8.8f %8.8f %8.8f %8.8f r_po ",r.left,r.top,r.right,r.bottom);
	SendOutput(fCurOutputFile, string);

	if(isFilled) {
		if(fPathOnly == 0)
			SendOutput(fCurOutputFile, "f");
	} else {
		if(fPathOnly == 0)
			SendOutput(fCurOutputFile, "s");
		else
			SendOutput(fCurOutputFile, "sp");
	}

	SendOutput(fCurOutputFile, "\n");
	return 0;
}

int32 Convert::StrokeRoundRect(BRect r, BPoint radius)
{
	P("PostscriptStrokeRoundRect\n");
	CommonRoundRect(r, radius, false);
	return 0;
};

int32 Convert::FillRoundRect(BRect r, BPoint radius)
{
	P("PostscriptFillRoundRect\n");
	CommonRoundRect(r, radius, true);
	return 0;
};

int32 Convert::CommonRoundRect(BRect r, BPoint radius, bool isFilled)
{
	P("PostscriptFillRoundRect\n");
	char string[1024];
	ConvertCoord(&(r.left),&(r.top));
	ConvertCoord(&(r.right),&(r.bottom));
	sprintf(string,"%8.8f %8.8f %8.8f %8.8f %8.8f %8.8f rr_po ",radius.x, radius.y,
				r.right-r.left, r.bottom-r.top, r.left, r.top);
	SendOutput(fCurOutputFile, string);

	if(isFilled) {
		if(fPathOnly == 0)
			SendOutput(fCurOutputFile, "f");
	} else {
		if(fPathOnly == 0)
			SendOutput(fCurOutputFile, "s");
		else
			SendOutput(fCurOutputFile, "sp");
	}

	SendOutput(fCurOutputFile, "\n");
	return 0;
}

int32 Convert::StrokeBezier(BPoint *pt)
{
	P("PostscriptStrokeBezier\n");
	CommonBezier(pt, false);
	return 0;
}

int32 Convert::FillBezier(BPoint *pt)
{
	P("PostscriptFillBezier\n");
	CommonBezier(pt, true);
	return 0;
}

int32 Convert::CommonBezier(BPoint *pt, bool isFilled)
{
	P("PostscriptCommonBezier\n");

	BPoint *curPt = pt;
	ConvertCoord(&(curPt->x), &(curPt->y));
	
	BPoint *ctlPt1 = pt+1;
	ConvertCoord(&(ctlPt1->x), &(ctlPt1->y));

	BPoint *ctlPt2 = pt+2;
	ConvertCoord(&(ctlPt2->x), &(ctlPt2->y));

	BPoint *endPt = pt + 3;
	ConvertCoord(&(endPt->x), &(endPt->y));

	char string[1024];
	sprintf(string, "%4.8f %4.8f m\n", curPt->x, curPt->y);
	SendOutput(fCurOutputFile, string);
	
	sprintf(string, "%4.8f %4.8f %4.8f %4.8f %4.8f %4.8f curveto\n",
			ctlPt1->x, ctlPt1->y, ctlPt2->x, ctlPt2->y, endPt->x, endPt->y);
	SendOutput(fCurOutputFile, string);	

	if(isFilled) {
		if(fPathOnly == 0)
			SendOutput(fCurOutputFile, "f\n");
	} else {
		if(fPathOnly == 0)
			SendOutput(fCurOutputFile, "s\n");
		else
			SendOutput(fCurOutputFile, "sp\n");
	}

	return 0;
}

int32 Convert::StrokeArc(BPoint center, BPoint radius, 
	float startAngle, float endAngle)
{
	P("PostscriptStrokeArc\n");
	CommonArc(center, radius, startAngle, endAngle, false);
	return 0;
};

int32 Convert::FillArc(BPoint center, BPoint radius, 
	float startAngle, float endAngle)
{
	P("PostscriptFillArc\n");
	CommonArc(center, radius, startAngle, endAngle, true);
	return 0;
};

int32 Convert::CommonArc(BPoint center, BPoint radius, 
	float startAngle, float endAngle, bool isFilled)
{
	P("PostscriptCommonArc\n");

	char string[1024];
	ConvertCoord(&(center.x), &(center.y));
	sprintf(string,"%8.8f %8.8f %8.8f %8.8f %8.8f %8.8f a_po\n", center.x, center.y,
			radius.x, radius.y, startAngle, endAngle);
	SendOutput(fCurOutputFile, string);

	if(isFilled) {
		if(fPathOnly == 0)
			SendOutput(fCurOutputFile, "f\n");
	} else {
		if(fPathOnly == 0)
			SendOutput(fCurOutputFile, "s\n");
		else
			SendOutput(fCurOutputFile, "sp\n");
	}

	return 0;
};


int32 Convert::StrokeEllipse(BPoint center, BPoint radius)
{
	P("PostscriptStrokeEllipse\n");
	CommonEllipse(center, radius, false);
	return 0;
};

int32 Convert::FillEllipse(BPoint center, BPoint radius)
{
	P("PostscriptFillEllipse\n");
	CommonEllipse(center, radius, true);
	return 0;
};

int32 Convert::CommonEllipse(BPoint center, BPoint radius, bool isFilled)
{
	P("PostscriptCommonEllipse\n");
	CommonArc(center, radius, 0, 360, isFilled);
	return 0;
};

int32 Convert::StrokePolygon(int32 ptCount, BPoint *p, bool closed)
{
	P("PostscriptStrokePolygon\n");
	CommonPolygon(ptCount, p, closed, false);
	return 0;
};

int32 Convert::FillPolygon(int32 ptCount, BPoint *p)
{
	P("PostscriptFillPolygon\n");
	CommonPolygon(ptCount, p, true, true);
	return 0;
};

int32 Convert::CommonPolygon(int32 ptCount, BPoint *p, bool closed,
								bool isFilled)
{
	P("PostscriptCommonPolygon\n");

	char *string,*ptr;
	float x,y;

	ptr = string = (char*)malloc((ptCount*36) + 10);
	if(ptr == NULL) return B_ERROR;

	x=p[0].x; y=p[0].y;
	ConvertCoord(&x,&y);
	sprintf(ptr,"%8.8f %8.8f m\n",x,y);
	ptr+=strlen(ptr);
	for (int32 i=1;i<ptCount;i++) {
		x=p[i].x; y=p[i].y;
		ConvertCoord(&x,&y);
		sprintf(ptr,"%8.8f %8.8f l\n",x,y);
		ptr+=strlen(ptr);
	};
	if (closed) {
		sprintf(ptr,"cp\n");
	}
	SendOutput(fCurOutputFile, string);
	free(string);

	if(isFilled) {
		if(fPathOnly == 0)
			SendOutput(fCurOutputFile, "f\n");
	} else {
		if(fPathOnly == 0)
			SendOutput(fCurOutputFile, "s\n");
		else
			SendOutput(fCurOutputFile, "sp\n");
	}

	return 0;
};


int32 Convert::StrokeShape(BShape *shape)
{
	P("PostscriptStrokeShape\n");
	CommonShape(shape, false);
	return 0;
}

int32 Convert::FillShape(BShape *shape)
{
	P("PostscriptFillShape\n");
	CommonShape(shape, true);
	return 0;
}

int32 Convert::CommonShape(BShape *shape, bool isFilled)
{
	P("PostscriptCommonShape\n");

	// Save the currentpoint
	SendOutput(fCurOutputFile, "currentpoint\n");

	SendOutput(fCurOutputFile, "currentpoint dup /ytran exch -1 mul def exch dup /xtran exch -1 mul def exch t\n");
	fShapeIterator->Iterate(shape);
	SendOutput(fCurOutputFile, "xtran ytran t\n");

	if(isFilled) {
		if(fPathOnly == 0)
			SendOutput(fCurOutputFile, "f\n");
	} else {
		if(fPathOnly == 0)
			SendOutput(fCurOutputFile, "s\n");
		else
			SendOutput(fCurOutputFile, "sp\n");
	}

	// Restore the currentpoint
	SendOutput(fCurOutputFile, "moveto\n");

	return 0;
}

void
Convert::WriteFontStuff()
{
	char string[1024];
	
	char *fontname = fFontHandler->GetFontAlias(&(ctx->font));			
	sprintf(string, "%s findfont %f scalefont setfont\n", fontname, ctx->font.Size());
	SendOutput(fCurOutputFile, string);
	free(fontname);
	fontModified=false;
}

int32 Convert::DrawString(char *str_ptr, float float1, float float2)
{
	P("PostscriptDrawString\n");
	
	if (fontModified)
		WriteFontStuff();

	char string[2048];
	int32 string_length = strlen(str_ptr);

	if ((strlen(str_ptr) > 5) && (strncmp(str_ptr,"%PS%",strlen("%PS%")) == 0))
	{ // Hack to send PostScript code...
		sprintf(string,"%s\n",str_ptr+4);
		SendOutput(fCurOutputFile, string); 
	}
	else
	{
		// And now, Unicode rules...

		// 1) We have to convert the string from UTF8 to ISO1
		// 2) We have to escape '(', ')', '\', and any char > 127 [resp: 0x28, 0x29, 0x5c]

		unsigned char *str_iso1 = new unsigned char[string_length*4+1];
		int32 srclen = string_length;
		int32 isolen = string_length*4+1;
		int32 state = 0;
		
		if (convert_from_utf8(B_ISO1_CONVERSION, str_ptr, &srclen, (char *)str_iso1, &isolen, &state) == B_OK)
		{
			unsigned char *str_ps = new unsigned char[isolen*4+1];
			unsigned char *s = str_iso1;
			unsigned char *d = str_ps;
			for (int32 i=0 ; i<isolen ; i++)
			{
				unsigned char c = *s++;
				if ((c == 0x28) || (c == 0x29) || (c == 0x5c) || (c > 127))
					d += sprintf((char *)d, "\\%03o", c);
				else
					*d++ = c;
			}
			*d = 0;
			float len = ctx->font.StringWidth(str_ptr);
			sprintf(string, "(%s) %8.2f sh\n", str_ps, len);
			DrawPrimitive(string);

			delete [] str_ps;
		}

		delete [] str_iso1;
	}
	
	return 0;
};


int32 Convert::PushState()
{
	P("PostscriptPushState\n");
	
//	Restack();
	SendOutput(fCurOutputFile, "gs %% pushstate\n");

	ctx = new context(ctx);
	return 0;
};

int32 Convert::PopState()
{
	P("PostscriptPopState\n");

	context *c = ctx->previous;
	delete ctx;
	ctx = c;
	SendOutput(fCurOutputFile, "gr %% popstate\n");

	// Assume the font has been modified.
	// This will force DrawString to re-set the font (findfont)
	// and prevent an "invalid font" error
	fontModified = true;

	return 0;
};

int32 Convert::SetOrigin(BPoint p)
{
	P("PostscriptSetOrigin (%f,%f)\n",p.x,p.y);
	ctx->origin = p;
//	Restack();
	return 0;
};

int32 Convert::SetLocation(BPoint p)
{
	P("PostscriptSetLocation\n");
	char string[1024];
	ConvertCoord(&(p.x),&(p.y));
	sprintf(string,"%8.8f %8.8f m\n",p.x,p.y);
	SendOutput(fCurOutputFile, string); 
	return 0;
};

int32 Convert::SetDrawOp(int32 drawOp)
{
	P("PostscriptSetDrawOp\n");
	ctx->drawOp = drawOp;
	return 0;
};

int32 Convert::SetPenSize(float penSize)
{
	P("PostscriptPenSize\n");
	ctx->penSize = penSize;
	char string[1024];
	sprintf(string,"%8.8f w\n",penSize);
	SendOutput(fCurOutputFile, string); 
	return 0;
};

int32 Convert::SetScale(float scale)
{
	P("PostscriptSetScale %f\n",scale);
	ctx->scaling = scale;
	return 0;
};

int32 Convert::SetForeColor(rgb_color color)
{
	P("PostscriptSetForeColor\n");
	ctx->foreColor.red = color.red/255.0f;
	ctx->foreColor.green = color.green/255.0f;
	ctx->foreColor.blue = color.blue/255.0f;
	WriteColor(ctx->foreColor.red, ctx->foreColor.green, ctx->foreColor.blue);
	return 0;
};

int32 Convert::SetBackColor(rgb_color color)
{
	P("PostscriptSetBackColor\n");
	ctx->backColor.red = color.red/255.0f;
	ctx->backColor.green = color.green/255.0f;
	ctx->backColor.blue = color.blue/255.0f;
	return 0;
};

int32 Convert::SetPattern(pattern pat)
{
	P("PostscriptSetPattern\n");
	ctx->thePattern = pat;
	return 0;
};

int32 Convert::SetFontFamily(char *string)
{
	P("PostscriptSetFontFamily: %s\n", string);
	ctx->font.SetFamilyAndStyle(string,NULL);
	fontModified = true;
	return 0;
};

int32 Convert::SetFontStyle(char *string)
{
	P("PostscriptSetFontStyle: %s\n", string);
	ctx->font.SetFamilyAndStyle(NULL,string);
	fontModified = true;
	return 0;
};

int32 Convert::SetFontSpacing(int32 spacing)
{
	P("PostscriptSetFontSpacing\n");
	ctx->font.SetSpacing(spacing);
	return 0;
};

int32 Convert::SetFontEncoding(int32 encoding)
{
	P("PostscriptSetFontEncoding\n");
	ctx->font.SetEncoding(encoding);
	return 0;
};

int32 Convert::SetFontFlags(int32 flags)
{
	P("PostscriptSetFontFlags\n");
	ctx->font.SetFlags(flags);
	return 0;
};

int32 Convert::SetFontFaces(int32 faces)
{
	P("PostscriptSetFontFaces\n");
	ctx->font.SetFace(faces);
	fontModified = true;
	return 0;
};

int32 Convert::SetFontBPP(int32 bpp)
{
	P("PostscriptSetFontBPP\n");
	return 0;
};

int32 Convert::SetFontSize(float size)
{
	P("PostscriptSetFontSize\n");
	char string[1024];
	sprintf(string,"%f fs\n",size);
	SendOutput(fCurOutputFile, string); 
	ctx->font.SetSize(size);
	fontModified = true;
	return 0;
};

int32 Convert::SetFontShear(float shear)
{
	P("PostscriptSetFontShear\n");
	char string[1024];
	sprintf(string,"%f fh\n",shear*180/3.1415927);
	SendOutput(fCurOutputFile, string); 
	ctx->font.SetShear(shear);
	fontModified = true;
	return 0;
};

int32 Convert::SetFontRotate(float rotate)
{
	P("PostscriptSetFontRotate\n");
	char string[1024];
	sprintf(string,"%f fr\n",rotate*180/3.1415927);
	SendOutput(fCurOutputFile, string); 
	ctx->font.SetRotation(rotate);
	fontModified = true;
	return 0;
};

void Convert::WriteSystemCMap(BDataIO *toFile)
{
	char *ptr;
	const color_map *cmap;
	char string[256];
	char buffer[256];
	cmap = system_colors();
	sprintf(string,"%% Be color map\n");									SendOutput(toFile, string); 
	sprintf(string,"/setBeColorMap {\n");									SendOutput(toFile, string); 
	sprintf(string,"[/Indexed /DeviceRGB 255 <\n");							SendOutput(toFile, string); 
	for (long i = 0;i<32;i++) {
		ptr = buffer;
		for (long j = 0;j<8;j++) {
			sprintf(ptr,"%02X%02X%02X ",(cmap->color_list[i*8+j]).red,(cmap->color_list[i*8+j]).green,(cmap->color_list[i*8+j]).blue);
			ptr = ptr + 7;
		}
		*ptr++ = '\n';
		SendOutput(toFile, buffer, (ptr - buffer)); 
	}
	sprintf(string,"> ] setcolorspace } bd\n");							SendOutput(toFile, string); 
}

int32 Convert::DrawPixels(
	BRect src_rect, BRect dst_rect,
	int32 width, int32 height, int32 rowbyte,
	int32 format, uint8 *pixels)
{
	P("PostscriptSetDrawPixels\n");
	switch(format)
	{
		 case B_CMAP8:
			Color8DrawPixels(src_rect, dst_rect, width, height, rowbyte, pixels);		
			break;		
	
		 case B_RGB15:
		 case B_RGB16:
			RGB16DrawPixels(src_rect, dst_rect, width, height, rowbyte, pixels);		
			break;
	
		 case B_RGB32:
			RGB32DrawPixels(src_rect, dst_rect, width, height, rowbyte, pixels);		
			break;
	}	
	return 0;
}

void  Convert::Color8DrawPixels(
	BRect src_rect, BRect dst_rect, 
	int32 width, int32 height, int32 rowbyte,
	uint8 *pixels)
{
	int32 leftSource, rightSource, topSource, bottomSource;
	char string[256];

	// cast to integers
	leftSource = (int32)src_rect.left;
	rightSource = (int32)src_rect.right;
	topSource = (int32)src_rect.top;
	bottomSource = (int32)src_rect.bottom;

	// comnpute ancillaries values
	long wis = src_rect.IntegerWidth() + 1;
	long his = src_rect.IntegerHeight() + 1;
	float ws = src_rect.Width() + 1.0f;
	float hs = src_rect.Height() + 1.0f;
	float wd = dst_rect.Width() + 1.0f;
	float hd = dst_rect.Height() + 1.0f;
	int32 rowSize = (wis + 7) / 8;
	float x = dst_rect.left;
	float y = dst_rect.bottom;
	ConvertCoord(&x,&y);

	// Initialize indexed color space
	sprintf(string,"gs setBeColorMap %% drawpixels\n");
	SendOutput(fCurOutputFile, string); 
	sprintf(string, "\n%8.8f %8.8f %8.8f %8.8f %ld %ld %d DoColorImage\n", x, y, wd, hd, width, height, 8);
	SendOutput(fCurOutputFile, string);

	// Write datas
	uint8 *bitdata = new uint8[wis*his];
	uint8 *bitptr = bitdata;

	for (long i=topSource ; i<=bottomSource ; i++)
	{
		uint8 *ptr = pixels + rowbyte*i + leftSource;
		for (long j=leftSource ; j<=rightSource ; j++)
			*bitptr++ = *ptr++;
	}

	OutputRawData((char *)bitdata, (bitptr-bitdata));
	delete [] bitdata;

	sprintf(string,"gr %% drawpixels8\n");
	SendOutput(fCurOutputFile, string); 
}

void
Convert::RGB16DrawPixels(
		BRect src_rect, BRect dst_rect, int32 width,
		int32 height, int32 rowbyte,
		uint8 *pixels)
{
	int32 leftSource, rightSource, topSource, bottomSource;
	char string[256];

	// cast to integers
	leftSource = (int32)src_rect.left;
	rightSource = (int32)src_rect.right;
	topSource = (int32)src_rect.top;
	bottomSource = (int32)src_rect.bottom;

	// comnpute ancillaries values
	long wis = src_rect.IntegerWidth() + 1;
	long his = src_rect.IntegerHeight() + 1;
	float ws = src_rect.Width() + 1.0f;
	float hs = src_rect.Height() + 1.0f;
	float wd = dst_rect.Width() + 1.0f;
	float hd = dst_rect.Height() + 1.0f;
	int32 rowSize = (wis + 7) / 8;
	uint8 *bitdata = new uint8[wis*his*3];
	uint8 *bitptr = bitdata;

	float x = dst_rect.left;
	float y = dst_rect.bottom;
	ConvertCoord(&x,&y);

	sprintf(string,"gs /DeviceRGB setcolorspace %% drawpixels\n");
	SendOutput(fCurOutputFile, string); 
	sprintf(string, "\n%8.8f %8.8f %8.8f %8.8f %ld %ld %d DoColorImage24\n", x, y, wd, hd, width, height, 8);
	SendOutput(fCurOutputFile, string);

	for (long i=topSource ; i<=bottomSource ; i++)
	{
		const uint16 *ptr = (uint16 *)(pixels + rowbyte*i + leftSource * 2);
		for (long j=leftSource ; j<=rightSource ; j++)
		{
			uint16 c = *ptr++;			
			*bitptr++ = (((c & 0x0000F800) >> 8) | ((c & 0x0000E000) >> 13));
			*bitptr++ = (((c & 0x000007E0) >> 3) | ((c & 0x00000600) >> 9));
			*bitptr++ = (((c & 0x0000001F) << 3) | ((c & 0x0000001C) >> 2));
		}
	}

	OutputRawData((char *)bitdata, (bitptr-bitdata));
	delete [] bitdata;

	sprintf(string,"gr %% drawpixels16\n");
	SendOutput(fCurOutputFile, string); 
}



void 
Convert::RGB32DrawPixels(
		BRect src_rect, BRect dst_rect, int32 width,
		int32 height, int32 rowbyte,
		uint8 *pixels)
{
	int32 leftSource, rightSource, topSource, bottomSource;
	char string[256];

	// cast to integers
	leftSource = (int32)src_rect.left;
	rightSource = (int32)src_rect.right;
	topSource = (int32)src_rect.top;
	bottomSource = (int32)src_rect.bottom;

	// comnpute ancillaries values
	long wis = src_rect.IntegerWidth() + 1;
	long his = src_rect.IntegerHeight() + 1;
	float ws = src_rect.Width() + 1.0f;
	float hs = src_rect.Height() + 1.0f;
	float wd = dst_rect.Width() + 1.0f;
	float hd = dst_rect.Height() + 1.0f;
	int32 rowSize = (wis + 7) / 8;

	float x = dst_rect.left;
	float y = dst_rect.bottom;
	ConvertCoord(&x,&y);

	sprintf(string,"gs /DeviceRGB setcolorspace %% drawpixels\n");
	SendOutput(fCurOutputFile, string); 
	sprintf(string, "\n%8.8f %8.8f %8.8f %8.8f %ld %ld %d DoColorImage24\n", x, y, wd, hd, width, height, 8);
	SendOutput(fCurOutputFile, string);

	uint8 *bitdata = new uint8[wis*his*3];
	uint8 *bitptr = bitdata;

	// TODO: Hack here. If we are in B_OP_OVER, we treat transparent pixels as background color
	// Postscript doesn't allow transparency !

	if (ctx->drawOp == B_OP_OVER)
	{
		rgb_color background;
		background.red = (uint8)(ctx->backColor.red * 255.0f);
		background.green = (uint8)(ctx->backColor.green * 255.0f);
		background.blue = (uint8)(ctx->backColor.blue * 255.0f);
		background.alpha = 255;
		
		for (long i=topSource ; i<=bottomSource ; i++)
		{
			const uint8 *ptr = pixels + rowbyte*i + leftSource * 4;
			for (long j=leftSource ; j<=rightSource ; j++, ptr+=4)
			{
				if (	(ptr[3] == B_TRANSPARENT_32_BIT.alpha) &&
						(ptr[2] == B_TRANSPARENT_32_BIT.red) &&
						(ptr[1] == B_TRANSPARENT_32_BIT.green) &&
						(ptr[0] == B_TRANSPARENT_32_BIT.blue))
				{
					*bitptr++ = background.red;
					*bitptr++ = background.green;
					*bitptr++ = background.blue;
				}
				else
				{
					*bitptr++ = ptr[2];
					*bitptr++ = ptr[1];
					*bitptr++ = ptr[0];
				}
			}
		}
	}
	else
	{
		for (long i=topSource ; i<=bottomSource ; i++)
		{
			const uint8 *ptr = pixels + rowbyte*i + leftSource * 4;
			for (long j=leftSource ; j<=rightSource ; j++, ptr+=4)
			{
				*bitptr++ = ptr[2];
				*bitptr++ = ptr[1];
				*bitptr++ = ptr[0];
			}
		}
	}

	OutputRawData((char *)bitdata, (bitptr-bitdata));
	delete [] bitdata;

	sprintf(string,"gr %% drawpixels24\n");
	SendOutput(fCurOutputFile, string); 
}





void *PostscriptCallbacks[NB_CALLBACK_FUNCTIONS] = {
	NULL,
	ps_MovePen,	
	ps_StrokeLine,
	ps_StrokeRect,
	ps_FillRect,
	ps_StrokeRoundRect,
	ps_FillRoundRect,
	ps_StrokeBezier,
	ps_FillBezier,
	ps_StrokeArc,
	ps_FillArc,
	ps_StrokeEllipse,
	ps_FillEllipse,
	ps_StrokePolygon,
	ps_FillPolygon,
	ps_StrokeShape,
	ps_FillShape,
	ps_DrawString,
	ps_DrawPixels,
	NULL,		// -- (blit)
	ps_ClipToRects,
	ps_ClipToPicture,
	ps_PushState,
	ps_PopState,
	ps_EnterStateChange,
	ps_ExitStateChange,
	NULL,		// enter font state
	NULL,		// exit font state
	ps_SetOrigin,
	ps_SetLocation,
	ps_SetDrawOp,
	NULL,		// set line mode
	ps_SetPenSize,
	ps_SetForeColor,
	ps_SetBackColor,
	ps_SetPattern,
	ps_SetScale,
	ps_SetFontFamily,
	ps_SetFontStyle,
	ps_SetFontSpacing,
	ps_SetFontSize,
	ps_SetFontRotate,
	ps_SetFontEncoding,
	ps_SetFontFlags,
	ps_SetFontShear,
	ps_SetFontBPP,
	NULL,		//ps_SetFontFaces (BFont::SetFace)
	NULL		// DrawPicture
};

int32 Convert::ClipToRects(BRect *rects, int32 count)
{
	P("PostscriptClipToRects(%p, %ld)\n", rects, count);
	P("{\n");

	if ((rects) && (count>0))
	{ // Must preserve the currentpoint
		SendOutput(fCurOutputFile, "currentpoint\n");
		fInverseClip = false;
		fPathOnly++;
		StartClipElement();
		for (int32 rectNum=0 ; rectNum<count ; rectNum++)
			FillRect(rects[rectNum]);
		EndClipElement();
		fPathOnly--;
		SendOutput(fCurOutputFile, "moveto\n");
	}
	else
	{
		SendOutput(fCurOutputFile, "initclip\n");
	}
	
	P("}\n");
	return 0;
}

int32 Convert::ClipToPicture(BPicture *pic, BPoint origin, uint32 inverse)
{
	P("PostscriptClipToPicture\n");
	P("{\n");

	fInverseClip = (inverse != 0);
	fPathOnly++;
	StartClipElement();
	pic->Play(PostscriptCallbacks, NB_CALLBACK_FUNCTIONS, this);
	EndClipElement();
	fPathOnly--;

	P("}\n");
	return 0;
}

//==============================================================================
//
//	DoConvertionForPicture 	(
//							)
//
//	Read the BPicture file referenced by picture_ref, and write the equivalent
//  postscript code to a file.
//		
//==============================================================================

long Convert::DoConvertionForPicture()
{
	BPoint	where;
	char	string[1024];


	ctx = new context();
	ctx->font = be_plain_font;
	fontModified = true;
	ctx->origin.x = ctx->origin.y = 0;
	ctx->scaling = 1.0;
	
	input->Read(&where, sizeof(BPoint));
	input->Read(&fClipRect, sizeof(BRect));

	BPicture *p = new BPicture();
	p->Unflatten(input);

	sprintf(string,"gs %% doconvertionforpic\n");
	SendOutput(fCurOutputFile, string); 

	if(where.x != 0 || where.y != 0) {
		sprintf(string,"%8.2f %8.2f t\n",where.x,-where.y);
		SendOutput(fCurOutputFile, string);
	}
	
	if (scale != 1.0) {
		sprintf(string,"%8.2f %8.2f sc\n",scale,scale);
		SendOutput(fCurOutputFile, string);
	}
	
	p->Play(PostscriptCallbacks, NB_CALLBACK_FUNCTIONS, this);

	sprintf(string,"gr %% doconvertionforpic\n");
	SendOutput(fCurOutputFile, string); 

	delete p;
	delete ctx;

	return 0;
}

//==============================================================================
//
//	NewPage 	(
//					)
//		
//	Issue a "new page" on the PS code
//	
//==============================================================================

void Convert::NewPage()
{
	EndOfPage();
	fPageCounter++;
}

void Convert::TopOfPage()
{
	char string[128];
	sprintf(string, "Page: %ld %ld\n", fPageCounter+1, fPageCounter+1);
	DscComment(string);
	// Deal with landscape	
	if (landscape) {
		char string[256];
		sprintf(string,"90 r\n");
		SendOutput(fCurOutputFile, string); 
		sprintf(string,"0 %ld t\n",0 - yPageSize);
		SendOutput(fCurOutputFile, string); 
	}
}

void Convert::EndOfPage()
{
	char string[128];
	sprintf(string,"showpage\n");
	SendOutput(fCurOutputFile, string); 
}


void add_hex(char *p, uchar v)
{
	const char s[] = "0123456789ABCDEF";
	*p++ = s[v>>4];
	*p++ = s[v&0x0f];
}

//==============================================================================

#define R_RATIO 19661  /* 30% */
#define G_RATIO 38666  /* 59% */
#define B_RATIO 7209   /* 11% */

#define R_F_RATIO (0.30f)  /* 30% */
#define G_F_RATIO (0.59f)  /* 59% */
#define B_F_RATIO (0.11f)  /* 11% */

uint8 ditherTable256[256];

void generate_dither(uint8 *dither, int32 value, int32 level, int32 size)
{
	int32 half = (size>>(level+1));
	int32 valueInc = 1<<(level<<1);

	if (half > 1) {
		generate_dither(dither,value,level+1,size);
		generate_dither(dither+half*size+half,value+valueInc,level+1,size);
		generate_dither(dither+half,value+valueInc+valueInc,level+1,size);
		generate_dither(dither+half*size,value+valueInc+valueInc+valueInc,level+1,size);
	} else {
		dither[0] 		= value;
		dither[size+1] 	= value+valueInc;
		dither[1]		= value+valueInc+valueInc;
		dither[size]	= value+valueInc+valueInc+valueInc;
	};
}

void generate_dither_256()
{
	generate_dither(ditherTable256,0,0,16);
};

void Convert::do_line_color_24(uint8 *input_buffer, long rowSize, long columns, long vp, long left, long right, long cnt)
{
	uint8 *ptrBuf;
	uint8 *buffer = (uint8 *)malloc(3*columns*cnt);
	if (buffer == NULL)
		return;
	ptrBuf = buffer;

	for(int32 j=0 ; j<cnt; j++)
	{
		uint8 *ptr = (uint8 *)(input_buffer + rowSize*(vp+j));
		for (int i=0 ; i<columns ; i++, ptr+=4)
		{
			*ptrBuf++ = ptr[2];
			*ptrBuf++ = ptr[1];
			*ptrBuf++ = ptr[0];
		}
	}

	// do packbits conversion		
	long length = ptrBuf-buffer;
	OutputRawData2((char *)buffer, length);
	free(buffer);
}

void Convert::do_line_grey(uint8 *input_buffer, long rowSize, long columns, long vp, long left, long right, long cnt)
{
	uint8 *ptrBuf;
	uint8 * const buffer = (uint8 *)malloc(columns*cnt);
	if (buffer == NULL)
		return;
	ptrBuf = buffer;

	for (int32 j=0 ; j<cnt; j++)
	{
		uint8 *ptr = (uint8 *)(input_buffer + rowSize*(vp+j));
		for (int i=0 ; i<columns ; i++, ptr+=4)
		{
			float intensity;
			intensity  = ptr[2] * R_F_RATIO;
			intensity += ptr[1] * G_F_RATIO;
			intensity += ptr[0] * B_F_RATIO;
			intensity += 0.5f;
			*ptrBuf++ = (uint8)floor(intensity);
		}
	}

	// do packbits conversion		
	long length = ptrBuf-buffer;
	OutputRawData2((char *)buffer, length);
	free(buffer);
}

void Convert::do_line_color_8(uint8 *input_buffer, long rowSize, long columns, long vp, long left, long right, long cnt)
{
	uint8 *ptrBuf;
	uint8 *buffer = (uint8 *)malloc(columns*cnt);
	if (buffer == NULL)
		return;
	ptrBuf = buffer;

	for (int32 j=0 ; j<cnt; j++)
	{
		uint8 *ptr = (uint8 *)(input_buffer + rowSize*(vp+j));
		memcpy(ptrBuf, ptr, columns);
		ptrBuf += columns;
	}

	// do packbits conversion		
	long length = ptrBuf-buffer;
	OutputRawData2((char *)buffer, length);
	free(buffer);
}


void Convert::do_line(uint16 *input_buffer, long rowSize, long columns, long vp, long left, long right, long cnt)
{
	uint16 				*ptr;
	char				*ptrBuf;
	uint8 *				dither;
	long				xx;
	long				j,k;
	long				intensity;	
	uint32				c;
	uchar				cur;	
	long				i;

	char *buffer = (char*) malloc(columns * cnt);
	if (buffer == NULL)
		return;
	ptrBuf = buffer;
	
	for (i=0 ; i<cnt ; i++)
	{
		xx = 0;
		dither = ditherTable256 + ((vp+i) & 0x0F)*16;
		ptr = (uint16 *)(((uint8 *)input_buffer) + rowSize*i);

		ptr += left;
		j = left;

		while (j < right)
		{
			cur = 0;
			for (k = 0; k < 8; k++)
			{
				xx++;
				c = *ptr++;
				intensity =	 R_RATIO * (((c & 0x0000F800) >> 8) | ((c & 0x0000E000) >> 13));
				intensity += G_RATIO * (((c & 0x000007E0) >> 3) | ((c & 0x00000600) >> 9));
				intensity += B_RATIO * (((c & 0x0000001F) << 3) | ((c & 0x0000001C) >> 2));
				intensity = 255*65536 - intensity;
				if ((intensity >= (255*65536-1)) || (intensity > (((int32)dither[xx&0x0F])<<16)))
					cur &= ~(0x80 >> k);
				else
					cur |= (0x80 >> k);
				j++;
				if (j == columns)
					break;
			}
			*ptrBuf++ = cur;
		}
	}

	int32 length = ptrBuf-buffer;
	OutputRawData2(buffer, length);
	free(buffer);
}

//==============================================================================
void Convert::WriteImageFunction(BDataIO *toFile)
{
	if (languageLevel == 1)		WriteLevel1ImageFunction(toFile);
	else						WriteLevel2ImageFunction(toFile);
}

void Convert::WriteLevel1ImageFunction(BDataIO *toFile)
{
/*	Writes the following image function:

	/DoImage
	{
		/depth exch def
		/height exch def
		/width exch def
		/yscale exch def
		/xscale exch def
		/ytrans exch def
		/xtrans exch def
		
		/picline width 7 add 8 idiv string def
		
		gs
			xtrans ytrans t
			xscale yscale sc
			width height depth
			[width 0 0 -1 height mul 0 height] 
			{currentfile picline readhexstring pop} image
		gr	
	} bind def

	Stack: xTrans yTrans xScale yScale width height depth DoImage ---

*/
	char string[256];
	sprintf(string, "/DoGreyImage { /depth exch def /height exch def /width exch def\n");
	SendOutput(toFile, string); 
	sprintf(string, "/yscale exch def /xscale exch def /ytrans exch def /xtrans exch def /picline width 7 add 8 idiv string def\n");
	SendOutput(toFile, string); 
	sprintf(string, "gs	xtrans ytrans t	xscale yscale sc\n");
	SendOutput(toFile, string); 
	sprintf(string, "width height 8	[width 0 0 -1 height mul 0 height] {currentfile picline readhexstring pop} image\n");
	SendOutput(toFile, string); 
	sprintf(string, "gr } bind def\n");
	SendOutput(toFile, string);
}

void Convert::WriteLevel2ImageFunction(BDataIO *toFile)
{
	char string[256];
/*
/DoImage
{
	/depth exch def
	/height exch def
	/width exch def
	/yscale exch def
	/xscale exch def
	/ytrans exch def
	/xtrans exch def
	/DeviceGray setcolorspace
	gs
		xtrans ytrans t
		xscale yscale sc
	<<
		/ImageType 1
		/Width width
		/Height height
		/BitsPerComponent 1
		/Decode [0 1]
		/ImageMatrix [width 0 0 -1 height mul 0 height]
		/DataSource currentfile /ASCIIHexDecode filter /RunLengthDecode filter
	>> image
	gr
} bind def

*/	

	sprintf(string, "\n/DoGreyImage {	/depth exch def	/height exch def /width exch def \n");
	SendOutput(toFile, string); 
	sprintf(string, "/yscale exch def /xscale exch def /ytrans exch def /xtrans exch def /DeviceGray setcolorspace \n");
	SendOutput(toFile, string); 
	sprintf(string, "gs xtrans ytrans t	xscale yscale sc \n");
	SendOutput(toFile, string); 
	sprintf(string, "<<	/ImageType 1 /Width width /Height height \n");
	SendOutput(toFile, string); 
	sprintf(string, "/BitsPerComponent 8 /Decode [0 1] /ImageMatrix [width 0 0 -1 height mul 0 height] /DataSource currentfile /ASCIIHexDecode filter /RunLengthDecode filter\n");
	SendOutput(toFile, string); 
	sprintf(string, ">> image gr } bind def\n\n");
	SendOutput(toFile, string); 

	sprintf(string, "\n/DoColorImage {	/depth exch def	/height exch def /width exch def \n");
	SendOutput(toFile, string); 
	sprintf(string, "/yscale exch def /xscale exch def /ytrans exch def /xtrans exch def /DeviceGray setcolorspace \n");
	SendOutput(toFile, string); 
	sprintf(string, "gs	setBeColorMap xtrans ytrans t	xscale yscale sc \n");
	SendOutput(toFile, string); 
	sprintf(string, "<<	/ImageType 1 /Width width /Height height \n");
	SendOutput(toFile, string); 
	sprintf(string, "/BitsPerComponent 8 /Decode [0 255] /ImageMatrix [width 0 0 -1 height mul 0 height] /DataSource currentfile /ASCIIHexDecode filter /RunLengthDecode filter\n");
	SendOutput(toFile, string); 
	sprintf(string, ">> image gr } bind def\n\n");
	SendOutput(toFile, string); 

	sprintf(string, "\n/DoColorImage24 {	/depth exch def	/height exch def /width exch def \n");
	SendOutput(toFile, string); 
	sprintf(string, "/yscale exch def /xscale exch def /ytrans exch def /xtrans exch def /DeviceRGB setcolorspace \n");
	SendOutput(toFile, string); 
	sprintf(string, "gs	xtrans ytrans t	xscale yscale sc \n");
	SendOutput(toFile, string); 
	sprintf(string, "<<	/ImageType 1 /Width width /Height height \n");
	SendOutput(toFile, string); 
	sprintf(string, "/BitsPerComponent 8 /Decode [0 1 0 1 0 1] /ImageMatrix [width 0 0 -1 height mul 0 height] /DataSource currentfile /ASCIIHexDecode filter /RunLengthDecode filter\n");
	SendOutput(toFile, string); 
	sprintf(string, ">> image gr } bind def\n\n");
	SendOutput(toFile, string);
}

//==============================================================================

void Convert::SetLanguageLevel(int lev)
{
	languageLevel = lev;
}

void Convert::SetImageableArea(BRect R)
{
	Imageable = R;
}


void Convert::StartClipElement()
{
	if (fStartClipIsActive)
		return;
	fStartClipIsActive = true;

	ctx = new context(ctx);
		
	/* need to check for inverse clip */
	SendOutput(fCurOutputFile, "initclip /_cl 1 d %% startclipelement\n");

	if (fInverseClip)
	{
		BRect R(fClipRect);
		R.InsetBy(-100,-100);
		FillRect(R);
		SendOutput(fCurOutputFile, "eoclip\n");
	}
}

void Convert::EndClipElement()
{	
	/* need to check for inverse clip */
	char string[256];
	if (fInverseClip)	sprintf(string, "eoclip p\n");
	else				sprintf(string, "clip p \n");	
	SendOutput(fCurOutputFile, string);

	context *c = ctx->previous;
	delete ctx;
	ctx = c;

// I'm not sure WriteContext() is usefull
// I think it is, but I've seen no difference with it!
// WriteContext();	
	
	SendOutput(fCurOutputFile, "/_cl 0 d %% endclipelement\n");
	fStartClipIsActive = false;
}


BFile* Convert::CreateTmpFile(BString *path)
{
	const char *tmpfile = tmpnam(NULL);
	if(path) {
		*path = tmpfile;
	}
	
	if(strlen(tmpfile) == 0) {
		fprintf(stderr, "Unable to open output file!\n");
		return NULL;		
	}

	BFile *retFile = new BFile(tmpfile, B_READ_WRITE | B_CREATE_FILE);
	if(retFile->InitCheck() != B_OK){
		fprintf(stderr, "Unable to open output file!\n");
		return NULL;		
	}
	
	return retFile;
}

status_t Convert::SetupBitmap(BRect rect, int32 resolution)
{
	if (fRasterBitmap != NULL)
		return B_OK;

	generate_dither_256();

	long w = (long)((rect.Width()+1.0f)*((float)resolution/72.0));
	w = ((w + 7)/8) * 8;

	BRect a_rect(0, 0, w, BAND_SIZE-1);

	if (fInColor)
	{
		if (!fHighQuality)	fRasterBitmap = new BBitmap(a_rect, B_BITMAP_IS_AREA | B_BITMAP_ACCEPTS_VIEWS, B_COLOR_8_BIT);
		else				fRasterBitmap = new BBitmap(a_rect, B_BITMAP_IS_AREA | B_BITMAP_ACCEPTS_VIEWS, B_RGB32_LITTLE);
	} 
	else
	{
		fRasterBitmap = new BBitmap(a_rect, B_BITMAP_IS_AREA | B_BITMAP_ACCEPTS_VIEWS, B_RGB32_LITTLE);
	}

	// Add the temporary imaging view to the bitmap.
	fRasterView = new BView(a_rect, "RasterView", B_FOLLOW_ALL, B_WILL_DRAW);	
	fRasterBitmap->AddChild(fRasterView);

	return B_OK;
}

status_t Convert::RasterizePictures(int32 numPics, BPicture *pics[], BPoint pos[], BRect clips[], BRect printableRect, int32 resolution)
{
	fIsRasterizing = true;
	SetupBitmap(printableRect, resolution);
	fReqResolution = resolution;
	RasterizeRect(printableRect, numPics, pics, pos, clips, resolution);
	fIsRasterizing = false;
	return error;
}

void Convert::RasterizeRect(BRect RRect, int32 numPics, BPicture *pics[], BPoint pos[], BRect clips[], int32 resolution)
{
	fRasterBitmap->Lock();

	const long columns = fRasterBitmap->Bounds().IntegerWidth()+1;
	const long lines = fRasterBitmap->Bounds().IntegerHeight()+1;
	const float factor = resolution / 72.0f;
	const int nb_strips = (int)(RRect.Height()*factor + BAND_SIZE - 1)/BAND_SIZE;

	const long width = columns;
	const long height = lines*nb_strips;
	float x = Imageable.left + RRect.left;
	float y = 0; //RRect.top; It's strange, I don't know why RRect.top produce misplaced picture?
	const float scalex = RRect.Width()+1.0f;
	const float scaley = RRect.Height()+1.0f;

	char string[256];
	if (fInColor)
	{
		if (fHighQuality)	sprintf(string, "\n%8.8f %8.8f %8.8f %8.8f %ld %ld %d DoColorImage24\n", x, y, scalex, scaley, width, height, 8);
		else				sprintf(string, "\n%8.8f %8.8f %8.8f %8.8f %ld %ld %d DoColorImage\n", x, y, scalex, scaley, width, height, 8);
	}
	else
	{
		sprintf(string, "\n%8.8f %8.8f %8.8f %8.8f %ld %ld %d DoGreyImage\n", x, y, scalex, scaley, width, height, 1);
	}
	SendOutput(fCurOutputFile, string);


	fRasterView->ForceFontAliasing(true);
	fRasterView->SetScale(factor);

	for (int i=0 ; i<nb_strips ; i++)
	{
		// First erase the page to white... nobody will do it for you !
		fRasterView->SetHighColor(255, 255, 255);
		fRasterView->FillRect(fRasterView->Bounds());

		// Got thru the list of pictures...
		for (int p=0 ; p<numPics ; p++)
		{	
			BRect tmp_clip = clips[p];	// get the current clip for this picture
			tmp_clip.OffsetTo(pos[p]);	// Handle the rectangular clipping for a given picture.

			const float hp1 = -RRect.left;
			const float vp1 = -(((i * BAND_SIZE)/factor) + RRect.top);
			tmp_clip.OffsetBy(hp1, vp1);
			tmp_clip.bottom = tmp_clip.bottom;

			BRegion tmp_region;
			tmp_region.Set(tmp_clip);	// create a region set to that clip rect

			// Draw the picture at the correct place.
			fRasterView->ConstrainClippingRegion(&tmp_region);
			fRasterView->DrawPicture(pics[p], BPoint(pos[p].x+hp1 , pos[p].y+vp1));
			fRasterView->Sync();
			fRasterView->ConstrainClippingRegion(NULL);

			// We can now send that content of the bitmap to the postscript file
			if (fInColor)
			{
				if (fHighQuality)	WriteColor24Raster((uint8 *)fRasterBitmap->Bits(), fRasterBitmap->BytesPerRow(), columns, lines);
				else				WriteColor8Raster((uint8 *)fRasterBitmap->Bits(), fRasterBitmap->BytesPerRow(), columns, lines);
			}
			else
			{
				WriteGreyRaster((uint8 *)fRasterBitmap->Bits(), fRasterBitmap->BytesPerRow(), columns, lines);
			}	
		}
	}

	if (languageLevel == 1)			SendOutput(fCurOutputFile, ">\n");
	else if (languageLevel >= 2)	SendOutput(fCurOutputFile, "80>\n");

	fRasterBitmap->Unlock();
}


void Convert::WriteColor24Raster(uint8 *input_buffer, long rowbyte, long columns, long lines)
{
	do_line_color_24(input_buffer, rowbyte, columns, 0, 0, 0, lines);
}

void Convert::WriteColor8Raster(uint8 *input_buffer, long rowbyte, long columns, long lines)
{
	do_line_color_8(input_buffer, rowbyte, columns, 0, 0, 0, lines);
}

void Convert::WriteGreyRaster(uint8 *input_buffer, long rowbyte, long columns, long lines)
{
	do_line_grey(input_buffer, rowbyte, columns, 0, 0, 0, lines);
}

void Convert::WriteBlackAndWhiteRaster(uint16 *input_buffer, long bpr, long columns, long lines)
{
	static int vp = 0;	// Line dither count
	do_line(input_buffer, bpr, columns, vp, 0, columns, lines); 
	vp += lines;
	vp &= 0xF;
}



status_t Convert::OutputRawData(char *data, int32 length)
{
	char *packed = NULL;
	int32 plength = 0;

	if (languageLevel == 1)
	{
		packed = data;
		plength = length;	
	}
	else if (languageLevel >= 2)
	{
		if ((packed = (char*)malloc(2*length)) == NULL)
			return B_ERROR;
		plength = compress(packed, data, length);
	}

	char buf[82];
	char *bufPtr;
	int32 chunk = 0;
	for (int32 i=0 ; i < plength; i += chunk)
	{
		bufPtr = buf;
		chunk = (plength - i > 39) ? 39 : (plength-i);
		for (int32 j=0; j < chunk; j++)
		{
			add_hex(bufPtr, packed[i+j]);
			bufPtr += 2;
		}
		*bufPtr = '\n';	bufPtr++;
		SendOutput(fCurOutputFile, buf, (bufPtr - buf));
	}	

	if (languageLevel == 1) {
		SendOutput(fCurOutputFile, ">\n");
	} else if (languageLevel >= 2) {
		SendOutput(fCurOutputFile, "80>\n");
		free(packed);
	}

	return B_OK;
}

status_t Convert::OutputRawData2(char *data, int32 length)
{
	char *packed = NULL;
	int32 plength = 0;

	if (languageLevel == 1)
	{
		plength = length;	
	}
	else if (languageLevel >= 2)
	{
		if ((packed = (char *)malloc(2*length)) == NULL)
			return B_ERROR;
		plength = compress(packed, data, length);
		data = packed;
	}

	char buf[82];
	char *bufPtr;
	int32 chunk = 0;
	for(int32 i=0; i < plength; i += chunk)	{
		bufPtr = buf;
		chunk = (plength - i > 39) ? 39 : (plength-i);
		for(int32 j=0; j < chunk; j++) {
			add_hex(bufPtr, data[i+j]);
			bufPtr += 2;
		}
		*bufPtr = '\n'; bufPtr++;
		SendOutput(fCurOutputFile, buf, (bufPtr - buf));
	}	


	free(packed);

	return B_OK;
}



float Convert::ColorToGray(float R, float G, float B)
{
	float intensity;
	intensity  = R * R_RATIO;
	intensity += G * G_RATIO;
	intensity += B * B_RATIO;
	intensity /= (R_RATIO+G_RATIO+B_RATIO);	
	return intensity;
}

void Convert::WriteColor(float red, float green, float blue)
{
	char outStr[256];
	if (fInColor) {
		sprintf(outStr, "%8.4f %8.4f %8.4f c\n", red, green, blue);
	} else {
		float grayVal = ColorToGray(red, green, blue);
		sprintf(outStr, "%8.4f setgray\n", grayVal);
	}

	SendOutput(fCurOutputFile, outStr);
}
//==============================================================================

int32 ps_MovePen(Convert *cp, BPoint delta)
{
	return cp->MovePen(delta);
}

int32 ps_StrokeLine(Convert *cp, BPoint p1, BPoint p2)
{
	return cp->StrokeLine(p1,p2);
}

int32 ps_StrokeRect(Convert *cp, BRect r)
{
	return cp->StrokeRect(r);
}

int32 ps_FillRect(Convert *cp, BRect r)
{
	return cp->FillRect(r);
}

int32 ps_StrokeRoundRect(Convert *cp, BRect r, BPoint radius)
{
	return cp->StrokeRoundRect(r,radius);
}

int32 ps_StrokeBezier(Convert *cp, BPoint *pt)
{
	return cp->StrokeBezier(pt);
}

int32 ps_FillBezier(Convert *cp, BPoint *pt)
{
	return cp->FillBezier(pt);
}

int32 ps_FillRoundRect(Convert *cp, BRect r, BPoint radius)
{
	return cp->FillRoundRect(r,radius);
}

int32 ps_StrokeArc(Convert *cp, BPoint center, BPoint radius, float startAngle, float endAngle)
{
	return cp->StrokeArc(center,radius,startAngle,endAngle);
}

int32 ps_FillArc(Convert *cp, BPoint center, BPoint radius, float startAngle, float endAngle)
{
	return cp->FillArc(center,radius,startAngle,endAngle);
}

int32 ps_StrokeEllipse(Convert *cp, BPoint center, BPoint radius)
{
	return cp->StrokeEllipse(center,radius);
}

int32 ps_FillEllipse(Convert *cp, BPoint center, BPoint radius)
{
	return cp->FillEllipse(center,radius);
}

int32 ps_StrokePolygon(Convert *cp, int32 ptCount, BPoint *p, bool closed)
{
	return cp->StrokePolygon(ptCount,p,closed);
}

int32 ps_FillPolygon(Convert *cp, int32 ptCount, BPoint *p)
{
	return cp->FillPolygon(ptCount,p);
}

int32 ps_StrokeShape(Convert *cp, BShape *shape)
{
	return cp->StrokeShape(shape);
}

int32 ps_FillShape(Convert *cp, BShape *shape)
{
	return cp->FillShape(shape);
}

int32 ps_DrawString(Convert *cp, char *str_ptr, float float1, float float2)
{
	return cp->DrawString(str_ptr,float1,float2);
}

int32 ps_ClipToRects(Convert *cp, BRect *rects, int32 count)
{
	return cp->ClipToRects(rects, count);
}

int32 ps_ClipToPicture(Convert *cp, BPicture *pic, BPoint origin, uint32 inverse)
{
	return cp->ClipToPicture(pic, origin, inverse);
}

int32 ps_PushState(Convert *cp)
{
	cp->PushState();
	statedepth++;
	return 0;
}

int32 ps_PopState(Convert *cp)
{
	statedepth--;
	return cp->PopState();
}

int32 ps_SetOrigin(Convert *cp, BPoint p)
{
	return cp->SetOrigin(p);
}

int32 ps_SetLocation(Convert *cp, BPoint p)
{
	return cp->SetLocation(p);
}

int32 ps_SetDrawOp(Convert *cp, int32 drawOp)
{
	return cp->SetDrawOp(drawOp);
}

int32 ps_SetPenSize(Convert *cp, float penSize)
{
	return cp->SetPenSize(penSize);
}

int32 ps_SetScale(Convert *cp, float scale)
{
	return cp->SetScale(scale);
}

int32 ps_SetForeColor(Convert *cp, rgb_color color)
{
	return cp->SetForeColor(color);
}

int32 ps_SetBackColor(Convert *cp, rgb_color color)
{
	return cp->SetBackColor(color);
}

int32 ps_SetPattern(Convert *cp, pattern pat)
{
	return cp->SetPattern(pat);
}

int32 ps_SetFontFamily(Convert *cp, char *string)
{
	return cp->SetFontFamily(string);
}

int32 ps_SetFontStyle(Convert *cp, char *string)
{
	return cp->SetFontStyle(string);
}

int32 ps_SetFontSpacing(Convert *cp, int32 spacing)
{
	return cp->SetFontSpacing(spacing);
}

int32 ps_SetFontEncoding(Convert *cp, int32 encoding)
{
	return cp->SetFontEncoding(encoding);
}

int32 ps_SetFontFlags(Convert *cp, int32 flags)
{
	return cp->SetFontFlags(flags);
}

int32 ps_SetFontFaces(Convert *cp, int32 faces)
{
	return cp->SetFontFaces(faces);
}

int32 ps_SetFontBPP(Convert *cp, int32 bpp)
{
	return cp->SetFontBPP(bpp);
}

int32 ps_SetFontSize(Convert *cp, float size)
{
	return cp->SetFontSize(size);
}

int32 ps_SetFontShear(Convert *cp, float shear)
{
	return cp->SetFontShear(shear);
}

int32 ps_SetFontRotate(Convert *cp, float rotate)
{
	return cp->SetFontRotate(rotate);
}

int32 ps_DrawPixels(Convert *cp, 
	BRect src_rect, BRect dst_rect,
	int32 width, int32 height, int32 rowbyte,
	int32 format, int32 flags, uint8 *pixels)
{
	return cp->DrawPixels(src_rect,dst_rect,width,height,rowbyte,format,pixels);
}

int32 ps_EnterStateChange(Convert *cp)
{
	return cp->EnterStateChange();
}

int32 ps_ExitStateChange(Convert *cp)
{
	return cp->ExitStateChange();
}


PsShapeIterator::PsShapeIterator(Convert *c, BFile *f)
{
	fConvert = c;
	fCurOutputFile = f;
}

status_t
PsShapeIterator::IterateMoveTo(BPoint *p)
{
	char string[512];
	sprintf(string, "%8.2f %8.2f m\n", p->x, -p->y);
	SendOutput(fCurOutputFile, string);
	return B_OK;
}

status_t
PsShapeIterator::IterateLineTo(int32 lineCount, BPoint *linePts)
{
	char string[512];
	for(int i=0; i < lineCount; i++, linePts++)	{
		sprintf(string, "%8.2f %8.2f l\n", linePts->x, -linePts->y);
		SendOutput(fCurOutputFile, string);
	}
	return B_OK;
}

status_t
PsShapeIterator::IterateBezierTo(int32 bezierCount, BPoint *bezierPts)
{
	char string[512];
	BPoint *ctlPt1, *ctlPt2, *endPt;
	
	for(int i=0; i < bezierCount; i++, bezierPts += 3) {
		ctlPt1 = bezierPts;
		ctlPt2 = bezierPts+1;
		endPt = bezierPts + 2;
		
		sprintf(string, "%8.2f %8.2f %8.2f %8.2f %8.2f %8.2f curveto\n",
			ctlPt1->x, -ctlPt1->y, ctlPt2->x, -ctlPt2->y, endPt->x, -endPt->y);
		SendOutput(fCurOutputFile, string);	
	}
	
	return B_OK;
}

status_t
PsShapeIterator::IterateClose()
{
	SendOutput(fCurOutputFile, "closepath\n");
	return B_OK;
}

