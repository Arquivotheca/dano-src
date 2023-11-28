//******************************************************************************
//
//	File:		Convert.cpp
//
//	Description:	Convert object for PostScript driver
//
//	Copyright 1996, International Lorienne Inc.
//	Licensed to Be Inc. for BeOS
//
//	11/04/96 : Test purpose. pre prototype
//	12/06/96 : Remodeling for real use
//
//			
//	Marc Verstaen
//
//******************************************************************************

#include "Convert.h"
#include "ConvertApp.h"
#include "ScrollText.h"
#include <Alert.h>
#include <time.h>



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
	hh=0;
	vv=0;
	previous = NULL;

	// initialize drawing parameters
	cur_color.red = -1.;
	cur_color.green = -1.;
	cur_color.blue = -1.;
	drawMode = B_OP_COPY;
}

context::context(context *oldOne)
{
	hh=oldOne->hh;
	vv=oldOne->vv;
	previous=oldOne;

	// initialize drawing parameters
	cur_color.red = oldOne->cur_color.red;
	cur_color.green = oldOne->cur_color.green;
	cur_color.blue = oldOne->cur_color.blue;
	drawMode = oldOne->drawMode;
}



//==============================================================================
//
//	Convert()
//
//	Constructor for Convert
//
//	Initialize a new Convert object, without any reference to a file
//
//
//==============================================================================

Convert::Convert()
{
	// Load AppSketcher module 
	module = new LRModule ("Picture2PS");
	if (!module->LoadModule()) {
		BAlert *alert = new BAlert("Module problem",
			"Unable to load module Picture2PS","Sorry...",
			NULL,NULL,B_WIDTH_AS_USUAL,B_STOP_ALERT);
		alert->Go();
	 	return;
	}
	loaded = false;

	// Initialize pointer to the message view	
	ScrollText *scrollText = (ScrollText *) module->FindInstance("PSScrollText");
	textResult = scrollText->textView;
	
	// Hard coded page size
	xPageSize = 612;
	yPageSize = 792;

	// Ask be_app for lanscape flag and scale factor
	landscape = !(((ConvertApp *) be_app)->IsPortrait());
	scale = (((ConvertApp *) be_app)->Scale());
	rotation = 0.;
}

//==============================================================================
//
//	Convert (
//				record_ref picture_ref
//			)
//
//	Constructor for Convert
//
//	Initialize a new Convert object, without any reference to a file
//
//==============================================================================

Convert::Convert(record_ref picture_ref)
{
	// Load AppSketcher module 
	module = new LRModule ("Picture2PS");
	if (!module->LoadModule()) {
		BAlert *alert = new BAlert("Module problem",
			"Unable to load module Picture2PS","Sorry...",
			NULL,NULL,B_WIDTH_AS_USUAL,B_STOP_ALERT);
		alert->Go();
	 	return;
	}

	// Initialize pointer to the message view	
	ScrollText *scrollText = (ScrollText *) module->FindInstance("PSScrollText");
	textResult = scrollText->textView;

	// Hard coded page size
	xPageSize = 612;
	yPageSize = 792;

	// Ask be_app for lanscape flag and scale factor
	landscape = false;
	scale = 1.;
	rotation = 0.;

	// launch conversion of the BPicture given through picture_ref
	DoConvertion (picture_ref);
	loaded = true;
}

//==============================================================================
//
//	~Convert()
//
//	Destructor.
//
//			Do nothing
//
//==============================================================================

Convert::~Convert()
{
}

//==============================================================================
//
//	HasRef()
//
//	Return true if a file is known through its record ref, false if not
//
//==============================================================================

bool Convert::HasRef()
{
	return loaded;
}

//==============================================================================
//
//	SetRef()
//
//	Launch conversion for the BPicture file known through its record ref
//
//==============================================================================

void Convert::SetRef(record_ref picture_ref)
{
	DoConvertion (picture_ref);
	loaded = true;
}

//==============================================================================
//
//	Output 	(
//				const char *string
//			)
//	
//	Convenience function. String string to the message view initialized in
//	the constructor. 	
//		
//==============================================================================

void Convert::Output(const char *string)
{
	textResult->Window()->Lock();
	textResult->Insert(string);
	textResult->Window()->Unlock();
}

//==============================================================================
//
//	long get_long 	(
//					)
//		
//	Read a long value in the BPicture file and return it
//	
//==============================================================================

long Convert::get_long()
{
	long val;
	
	file.Read(&val,sizeof(long));
	return val;
}

//==============================================================================
//
//	float get_float 	(
//						)
//		
//	Read a float value in the BPicture file and return it
//		
//==============================================================================

float Convert::get_float()
{
	float val;
	
	file.Read(&val,sizeof(float));
	return val;
}

//==============================================================================
//
//	frect get_rect 	(
//					)
//		
//	Read an frect structure in the BPicture file and return it
//		
//==============================================================================

frect Convert::get_rect()
{
	frect rect;
	
	file.Read(&(rect.left),sizeof(float));
	file.Read(&(rect.top),sizeof(float));
	file.Read(&(rect.right),sizeof(float));
	file.Read(&(rect.bottom),sizeof(float));
	return rect;
}

//==============================================================================
//
//	frect get_irect 	(
//					)
//		
//	Read an frect structure in the BPicture file and return it
//  The structure is read using a long->float conversion
//		
//==============================================================================

frect Convert::get_irect()
{
	frect rect;
	long	left,top,right,bottom;
	
	file.Read(&left,sizeof(long));
	file.Read(&top,sizeof(long));
	file.Read(&right,sizeof(long));
	file.Read(&bottom,sizeof(long));
	
	rect.left = left;
	rect.top = top;
	rect.right = right;
	rect.bottom = bottom;

	return rect;
}

//==============================================================================
//
//	void get_data 	(
//					)
//		
//	Read "size" byte of data and return it
//		
//==============================================================================

void Convert::get_data(long size,void *ptr)
{
	file.Read(ptr,size);
	return;
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

void Convert::WriteHeader()
{
	BFile		header;
	BDirectory	directory;
	char		string[256];
	char		dateTime[256];
	char		name[B_FILE_NAME_LENGTH];
	tm			*asTime;
	time_t		secTime;

	// search for the file containing the descrition of the preamble.	
	app_info info;
	be_app->GetAppInfo(&info);
	BFile app_file;
	BDirectory appDir;
	app_file.SetRef(info.ref);
	app_file.GetParent(&appDir);
	
	if (appDir.GetFile("Header",&header) != B_NO_ERROR) {
		fprintf(stderr,"Can't file header file\n");
		return;
	}
	
	// Build the BFile for the resulting postscript file. For final release, this file
	// will be in the spooler folder.	
	file.GetParent(&directory);
	file.GetName(name);
	if (strlen(name) > (B_FILE_NAME_LENGTH+4)) {		// File name too long!
		name[B_FILE_NAME_LENGTH-4] = '\0';
	}
	strcat(name,".ps");
	if (directory.GetFile(name,&filePS) != B_NO_ERROR) {
		directory.Create(name,&filePS);
	}

	// Open the file containing the descrition of the preamble.	
	header.Open(B_READ_ONLY);
	// Open the postscript file
	filePS.Open(B_READ_WRITE);
	filePS.SetSize(0);

	// Write comments	
	sprintf(string,"%%!PS-Adobe-3.0\n"); 							filePS.Write(string,strlen(string));
	sprintf(string,"%%%%Creator: BeOS DR8.2\n"); 					filePS.Write(string,strlen(string));
	sprintf(string,"%%%%For: Convertor\n"); 						filePS.Write(string,strlen(string));
	sprintf(string,"%%%%Title: %d\n",name); 						filePS.Write(string,strlen(string));
	time (&secTime);
	asTime = localtime (&secTime);
	strftime(dateTime,256,"%m/%d/%y %I:%M %p",asTime);
	sprintf(string,"%%CreationDate: %s\n",dateTime); 				filePS.Write(string,strlen(string));
	// Printable surface has to be read in the ppd file
	sprintf(string,"%%%%BoundingBox: %d %d %ld %ld\n",10,10,xPageSize - 10, yPageSize - 10);
	 																filePS.Write(string,strlen(string));
	sprintf(string,"%%%%EndComments\n\n"); 							filePS.Write(string,strlen(string));

	// read and write header file
	long size = header.Size();	
	char *buffer = (char *) malloc(size*sizeof(char));
	header.Read(buffer,size);
	filePS.Write(buffer,size);

	header.Close();
	free(buffer);

	// Deal with landscape	
	if (landscape) {
		sprintf(string,"90 rotate\n"); 								filePS.Write(string,strlen(string));
		sprintf(string,"0 %d translate\n",0 - xPageSize); 			filePS.Write(string,strlen(string));
	}
}

//==============================================================================
//
//	ConvertCoord 	(
//						float *x,
//						float *y
//					)
//
//	Convert from BPicture coordinate to PS coordinate according to landscape and
//	to the fact that geometric systems are inverted.
//	
//		
//==============================================================================

void Convert::ConvertCoord(float *x,float *y)
{
	BPoint retPoint;

	if (!landscape) {
		retPoint.x = *x;
		retPoint.y = yPageSize - *y;
	}
	else {
		retPoint.x = *x;
		retPoint.y = xPageSize - *y;
	}
	*x = retPoint.x;
	*y = retPoint.y;
}

//==============================================================================
//
//	offset_frect 	(
//						frect *r,
//						float hh,
//						float vv
//					)
//
//	Offset a rectangle from (hh,vv)		
//		
//==============================================================================

void Convert::offset_frect(frect *rect,float hh,float vv)
{
	rect->left += hh;
	rect->right += hh;
	rect->top += vv;
	rect->bottom += vv;
}

//==============================================================================
//
//	SetColor 	(
//						pattern *p					// Pattern used for filling
//						const char *drawString,		// string containing the path to draw
//						long stringLength			// length of the string
//				)
//
//	This function deal with both pattern and color. According to the type of pattern,
//  we set back or front color, and set the correct colorspace for the pattern.
//
//	This function try to avoid sending not usefull invocation of setcolor. 
//
//	In the case of pattern use, the pattern matrix is send for every path. 	
//		
//==============================================================================

void Convert::SetColor(const pattern p,const char *drawString,long stringLength)
{
	char string[256];
	
	if ((p.data[0] == 0xff) && 
		(p.data[1] == 0xff) && 
		(p.data[2] == 0xff) && 
		(p.data[3] == 0xff) && 
		(p.data[4] == 0xff) && 
		(p.data[5] == 0xff) && 
		(p.data[6] == 0xff) && 
		(p.data[7] == 0xff)) {							// B_SOLID_HIGH
		if ((currentContext->cur_color.red != for_color.red) || (currentContext->cur_color.green != for_color.green) ||(currentContext->cur_color.blue != for_color.blue)) {
			sprintf(string,"%f %f %f c\n",for_color.red,for_color.green,for_color.blue);
			filePS.Write(string,strlen(string));
			currentContext->cur_color = for_color;
		}
		filePS.Write(drawString,stringLength);
	}
	else if ((p.data[0] == 0x00) && 
		(p.data[1] == 0x00) && 
		(p.data[2] == 0x00) && 
		(p.data[3] == 0x00) && 
		(p.data[4] == 0x00) && 
		(p.data[5] == 0x00) && 
		(p.data[6] == 0x00) && 
		(p.data[7] == 0x00)) {							// B_SOLID_LOW
		if ((currentContext->cur_color.red != back_color.red) || (currentContext->cur_color.green != back_color.green) ||(currentContext->cur_color.blue != back_color.blue)) {
			sprintf(string,"%f %f %f c\n",back_color.red,back_color.green,back_color.blue);
			filePS.Write(string,strlen(string));
			currentContext->cur_color = back_color;
		}
		filePS.Write(drawString,stringLength);
	}
	else {												// We use pattern
		if (currentContext->drawMode != B_OP_OVER) {
			if ((currentContext->cur_color.red != back_color.red) || (currentContext->cur_color.green != back_color.green) ||(currentContext->cur_color.blue != back_color.blue)) {
				sprintf(string,"%f %f %f c\n",back_color.red,back_color.green,back_color.blue);
				filePS.Write(string,strlen(string));
				currentContext->cur_color = back_color;
			}
			filePS.Write(drawString,stringLength);
		}
		// Set the correct pattern, according to p
		sprintf(string,"gs\n");
		filePS.Write(string,strlen(string));			

		sprintf(string,"[\n");
		filePS.Write(string,strlen(string));			
	
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
				filePS.Write(string,strlen(string));			
				test = test << 1;		
			}	
			sprintf(string,"\n");
			filePS.Write(string,strlen(string));			
		}
		sprintf(string,"] CreatePattern\n");
		filePS.Write(string,strlen(string));			

		sprintf(string,"[/Pattern [/DeviceRGB]] setcolorspace\n");
		filePS.Write(string,strlen(string));			

		sprintf(string,"%f %f %f CurPattern setcolor\n",for_color.red,for_color.green,for_color.blue);
		filePS.Write(string,strlen(string));

		filePS.Write(drawString,stringLength);

		sprintf(string,"gr\n");
		filePS.Write(string,strlen(string));
	}	
}

//==============================================================================
//
//	DoConvertion 	(
//						record_ref picture_ref
//					)
//
//	Read the BPicture file referenced by picture_ref, and write the equivalent
//  postscript code to a file.
//		
//==============================================================================

void Convert::DoConvertion(record_ref picture_ref)
{
	long	opcode;
	float	pen_h,pen_v;
	bool	endOfPicture;
	char	string[256];
	long	indFont = 0;
	bool	fontModified = true;
	long	currentFontIndex;
	float	x,y;
	bool	cmapWriten = false;

	// Initializing stack
	currentContext = new context();
	
	pen_h = 0.;
	pen_v = 0.;
	
	
	// Initialize a BWindow and a BView to use StringWidth(), and keep it locked
	BRect rect;
	rect.Set(0.,0.,10.,10.);
	BWindow *win = new BWindow(rect,"tempo",B_BORDERED_WINDOW,0);
	BView *view = new BView(rect,"tempo",0,0);
	win->Lock();
	win->AddChild(view);
	

	// Set file to the BPicture file	
	file.SetRef(picture_ref);
	if (file.Error() != B_NO_ERROR) {
		BAlert *alert = new BAlert("File problem",
			"Unable to open this file","Sorry...",
			NULL,NULL,B_WIDTH_AS_USUAL,B_STOP_ALERT);
		alert->Go();
	 	return;
	}
	file.Open(B_READ_ONLY);	
	if (file.Error() != B_NO_ERROR) {
		BAlert *alert = new BAlert("File problem",
			"Unable to open this file","Sorry...",
			NULL,NULL,B_WIDTH_AS_USUAL,B_STOP_ALERT);
		alert->Go();
	 	return;
	}

	// Write comments and preamble	
	WriteHeader();

	endOfPicture = false;
	
	textResult->Window()->Lock();
	
	while (!endOfPicture) {
		// Read opcode		
		if (file.Read(&opcode,sizeof(long)) != sizeof(long)) break;

		switch(opcode) {						// Do what's required according to opcode
		
//-------------------------------------------------------------
// moveto absolute position.
//
// 4 bytes. GR_MOVETO
// 4 bytes. x position
// 4 bytes. y position
//-------------------------------------------------------------

			case PIC_MOVETO :			//501	//done
				{
					pen_h = currentContext->hh+get_float();
					pen_v = currentContext->vv+get_float();
					x = pen_h;
					y = pen_v;
					ConvertCoord(&x,&y);					
					sprintf(string,"%f %f m\n",x,y);
					filePS.Write(string,strlen(string));
				}
				break;

//-------------------------------------------------------------
// Line to absolute position.
//
// 4 bytes. PIC_LINETO
// 4 bytes. x position
// 4 bytes. y position
//-------------------------------------------------------------

			case PIC_LINETO :		//502 done
				{
					float	tmp1;
					float	tmp2;
				
					tmp1 = currentContext->hh+get_float();
					tmp2 = currentContext->vv+get_float();
					pen_h = tmp1;
					pen_v = tmp2;
					x = pen_h;
					y = pen_v;
					ConvertCoord(&x,&y);
					// Set color in case of...
					if ((currentContext->cur_color.red != for_color.red) || (currentContext->cur_color.green != for_color.green) ||(currentContext->cur_color.blue != for_color.blue)) {
						sprintf(string,"%f %f %f c\n",for_color.red,for_color.green,for_color.blue);
						filePS.Write(string,strlen(string));
						currentContext->cur_color = for_color;
					}

					sprintf(string,"%f %f l s\n",x,y);
					filePS.Write(string,strlen(string));
				}
				break;

//-------------------------------------------------------------
// rect fill. 
//
// 4 bytes. PIC_RECTFILL
//16 bytes. absolute rectangle
// 8 bytes. pattern
//-------------------------------------------------------------

			case PIC_RECTFILL :		//507
				{
					frect	r;
					pattern	p;

					r = get_rect();
					offset_frect(&r, currentContext->hh, currentContext->vv);
					get_data(sizeof(pattern), &p);
					ConvertCoord(&(r.left),&(r.top));
					ConvertCoord(&(r.right),&(r.bottom));
					sprintf(string,"%f %f %f %f rf\n",r.left,r.top,r.right,r.bottom);
					SetColor(p,string,strlen(string));
				}
				break;

//-------------------------------------------------------------
// rect invert. 
//
// 4 bytes. PIC_RECT_INVERT
//16 bytes. absolute rectangle
//-------------------------------------------------------------

			case PIC_RECT_INVERT :		//509
				{
					// We're in trouble, we can't do anything.
					frect	r;
					r = get_rect();
				}
				break;


//-------------------------------------------------------------
// set draw mode. 
//
// 4 bytes. PIC_SET_DRAW_MODE
// 4 bytes. draw mode
//-------------------------------------------------------------


			case PIC_SET_DRAW_MODE :		//50a
				{
					currentContext->drawMode = (drawing_mode) get_long();
				}
				break;


//-------------------------------------------------------------
// draw string. 
//
// 4 bytes. PIC_DRAW_STRING
// 4 bytes. string length
// x bytes. string data
//-------------------------------------------------------------


			case PIC_DRAW_STRING :		//50c
				{
					long		string_length;
					char		*str_ptr;
			
					if (fontModified) {									// Current font has changed
						sprintf(string,"F%d\n",currentFontIndex);
						filePS.Write(string,strlen(string));
						fontModified = false;					
					}

					if ((currentContext->cur_color.red != for_color.red) || (currentContext->cur_color.green != for_color.green) ||(currentContext->cur_color.blue != for_color.blue)) {
						sprintf(string,"%f %f %f c\n",for_color.red,for_color.green,for_color.blue);
						filePS.Write(string,strlen(string));
						currentContext->cur_color = for_color;
					}

					string_length = get_long();
					str_ptr = (char *) malloc(string_length+1);
					get_data(string_length, str_ptr);
					str_ptr[string_length] = '\0';

					float len = view->StringWidth(str_ptr);
					sprintf(string,"(%s) %f sh\n",str_ptr,len);
					filePS.Write(string,strlen(string));

					free(str_ptr);
				}
				break;

//-------------------------------------------------------------
// set pen size. 
//
// 4 bytes. PIC_SET_PEN_SIZE
// 4 bytes. new pen size
//-------------------------------------------------------------

			case PIC_SET_PEN_SIZE :		//522
				{
					float	pensize;			
					pensize = get_float();
					sprintf(string,"%.2f w\n",pensize);
					filePS.Write(string,strlen(string));					
				}
				break;


//-------------------------------------------------------------
// ellipse frame. 
//
// 4 bytes. PIC_ELLIPSE_FRAME
//16 bytes. absolute rectangle
// 8 bytes. pattern
//-------------------------------------------------------------

			case PIC_ELLIPSE_FRAME :	//524
				{
					frect	r;
					pattern	p;

					r = get_rect();
					offset_frect(&r, currentContext->hh, currentContext->vv);
					ConvertCoord(&(r.left),&(r.top));
					ConvertCoord(&(r.right),&(r.bottom));
					get_data(sizeof(pattern), &p);
					sprintf(string,"%f %f %f %f es\n",r.left,r.top,r.right-r.left,r.bottom-r.top);
					SetColor(p,string,strlen(string));
				}
				break;


//-------------------------------------------------------------
// ellipse fill. 
//
// 4 bytes. PIC_ELLIPSE_FILL
//16 bytes. absolute rectangle
// 8 bytes. pattern
//-------------------------------------------------------------

			case PIC_ELLIPSE_FILL :	//533
				{
					frect	r;
					pattern	p;

					r = get_rect();
					offset_frect(&r, currentContext->hh, currentContext->vv);
					ConvertCoord(&(r.left),&(r.top));
					ConvertCoord(&(r.right),&(r.bottom));
					get_data(sizeof(pattern), &p);
					sprintf(string,"%f %f %f %f ef\n",r.left,r.top,r.right-r.left,r.bottom-r.top);
					SetColor(p,string,strlen(string));
					break;
				}



//-------------------------------------------------------------
// arc frame. 
//
// 4 bytes. PIC_ARC_FILL
//16 bytes. absolute rectangle
// 4 bytes. Start Angle
// 4 bytes. End Angle
// 8 bytes. pattern
//-------------------------------------------------------------

			case PIC_ARC_FRAME :		//53a
				{
					frect	r;
					pattern	p;
					long	start_angle;
					long	end_angle;

					r = get_rect();
					start_angle = get_long();
					end_angle = get_long();
					get_data(sizeof(pattern), &p);

					offset_frect(&r, currentContext->hh, currentContext->vv);
					ConvertCoord(&(r.left),&(r.top));
					ConvertCoord(&(r.right),&(r.bottom));
					sprintf(string,"%d %d %f %f %f %f as\n",start_angle,end_angle,r.left,r.top,r.right-r.left,r.bottom-r.top);
					SetColor(p,string,strlen(string));
				}
				break;


//-------------------------------------------------------------
// arc fill. 
//
// 4 bytes. PIC_ARC_FILL
//16 bytes. absolute rectangle
// 4 bytes. Start Angle
// 4 bytes. End Angle
// 8 bytes. pattern
//-------------------------------------------------------------

			case PIC_ARC_FILL :		//53b
				{
					frect	r;
					pattern	p;
					long	start_angle;
					long	end_angle;	

					r = get_rect();
					start_angle = get_long();
					end_angle = get_long();
					get_data(sizeof(pattern), &p);

					offset_frect(&r, currentContext->hh, currentContext->vv);
					ConvertCoord(&(r.left),&(r.top));
					ConvertCoord(&(r.right),&(r.bottom));
					sprintf(string,"%d %d %f %f %f %f af\n",start_angle,end_angle,r.left,r.top,r.right-r.left,r.bottom-r.top);
					SetColor(p,string,strlen(string));
				}
				break;


//-------------------------------------------------------------
// round rect frame.
//
// 4 bytes. PIC_ROUND_RECT_FRAME
//16 bytes. absolute rectangle
// 4 bytes. x radius
// 4 bytes. y radius
// 8 bytes. pattern
//-------------------------------------------------------------

			case PIC_ROUND_RECT_FRAME :	//53c
				{
					frect	r;
					pattern	p;
					float	xRadius;
					float	yRadius;

					r = get_rect();
					xRadius = get_float();
					yRadius = get_float();
					get_data(sizeof(pattern), &p);

					offset_frect(&r, currentContext->hh, currentContext->vv);
					ConvertCoord(&(r.left),&(r.top));
					ConvertCoord(&(r.right),&(r.bottom));
					sprintf(string,"%f %f %f %f %f %f rrs\n",xRadius,yRadius,r.left,r.top,r.right-r.left,r.bottom-r.top);
					SetColor(p,string,strlen(string));
				}
				break;


//-------------------------------------------------------------
// round rect fill.
//
// 4 bytes. PIC_ROUND_RECT_FILL
//16 bytes. absolute rectangle
// 4 bytes. x radius
// 4 bytes. y radius
// 8 bytes. pattern
//-------------------------------------------------------------

			case PIC_ROUND_RECT_FILL :		//53d
				{
					frect	r;
					pattern	p;
					float	xRadius;
					float	yRadius;

					r = get_rect();
					xRadius = get_float();
					yRadius = get_float();
					get_data(sizeof(pattern), &p);

					offset_frect(&r, currentContext->hh, currentContext->vv);
					ConvertCoord(&(r.left),&(r.top));
					ConvertCoord(&(r.right),&(r.bottom));
					sprintf(string,"%f %f %f %f %f %f rrf\n",xRadius,yRadius,r.left,r.top,r.right-r.left,r.bottom-r.top);
					SetColor(p,string,strlen(string));
				}
				break;

//-------------------------------------------------------------
// set foreground color. 
//
// 4 bytes. PIC_FORE_COLOR
// 4 bytes. rgb color
//-------------------------------------------------------------

			case PIC_FORE_COLOR :	//540
				{
					rgb_color	a_color;

					get_data(sizeof(rgb_color), &a_color);
					for_color.red = ((float) a_color.red) / 255.;
					for_color.green = ((float) a_color.green) / 255.;
					for_color.blue = ((float) a_color.blue) / 255.;
				}
				break;

//-------------------------------------------------------------
// set background color. 
//
// 4 bytes. PIC_BACK_COLOR
// 4 bytes. rgb color
//-------------------------------------------------------------

			case PIC_BACK_COLOR :		//541
				{
					rgb_color	a_color;

					get_data(sizeof(rgb_color), &a_color);
					back_color.red = ((float) a_color.red) / 255.;
					back_color.green = ((float) a_color.green) / 255.;
					back_color.blue = ((float) a_color.blue) / 255.;
				}
				break;



//-------------------------------------------------------------
// Line to absolute position.
//
// 4 bytes. PIC_LINETO_PAT
// 4 bytes. x position
// 4 bytes. y position
// 8 bytes. pattern
//-------------------------------------------------------------

			case PIC_LINETO_PAT :	//546 //done
				{
					float	tmp1;
					float	tmp2;
					pattern	p;

					tmp1 = currentContext->hh+get_float();
					tmp2 = currentContext->vv+get_float();
					get_data(sizeof(pattern), &p);

					ConvertCoord(&tmp1,&tmp2);
				
					sprintf(string,"%f %f l s\n",tmp1,tmp2);

					SetColor(p,string,strlen(string));
				}
				break;

//-------------------------------------------------------------
// set font name. 
//
// 4 bytes. PIC_SET_FONT_NAME
// 4 bytes. name length
// x bytes. name data
//-------------------------------------------------------------

			case PIC_SET_FONT_NAME :		//54b
			{
				long		string_length;
				char		*str_ptr;
				char		fontName[256];

				string_length = get_long();
				str_ptr = (char *) malloc(string_length+1);
				get_data(string_length, str_ptr);
				str_ptr[string_length] = '\0';
				indFont ++;	
				
				view->SetFontName(str_ptr);
				
				// Find corresponding font
				if (strcmp(str_ptr,"Times New Roman") == 0) strcpy(fontName,"Times-Roman");
				else if (strcmp(str_ptr,"Times New Roman Bold") == 0) strcpy(fontName,"Times-Bold");
				else if (strcmp(str_ptr,"Times New Roman Bold Italic") == 0) strcpy(fontName,"Times-BoldItalic");
				else if (strcmp(str_ptr,"Times New Roman Italic") == 0) strcpy(fontName,"Times-Italic");
				
				else if (strcmp(str_ptr,"Courier New") == 0) strcpy(fontName,"Times-Roman");
				else if (strcmp(str_ptr,"Courier New Bold Italic") == 0) strcpy(fontName,"Courier-BoldOblique");
				else if (strcmp(str_ptr,"Courier New Italic") == 0) strcpy(fontName,"Courier-Oblique");
				else if (strcmp(str_ptr,"Courier New Bold") == 0) strcpy(fontName,"Courier-Bold");
				
				else if (strcmp(str_ptr,"Arial MT New") == 0) strcpy(fontName,"Helvetica");
				else if (strcmp(str_ptr,"Arial MT Bold Italic") == 0) strcpy(fontName,"Helvetica-BoldOblique");
				else if (strcmp(str_ptr,"Arial MT Italic") == 0) strcpy(fontName,"Helvetica-Oblique");
				else if (strcmp(str_ptr,"Arial MT Bold") == 0) strcpy(fontName,"Helvetica-Bold");
				
				else if (strcmp(str_ptr,"Baskerville MT New") == 0) strcpy(fontName,"Courier");
				else if (strcmp(str_ptr,"Baskerville MT Bold Italic") == 0) strcpy(fontName,"Courier-BoldOblique");
				else if (strcmp(str_ptr,"Baskerville MT Italic") == 0) strcpy(fontName,"Courier-Oblique");
				else if (strcmp(str_ptr,"Baskerville MT Bold") == 0) strcpy(fontName,"Courier-Bold");
				
				else strcpy(fontName,"Times-Roman");
				sprintf(string,"/F%d {/%s findfont [_fs 0 _fh _fs 0 0] makefont setfont} bdef\n",indFont,fontName);
				fontModified = true;
				currentFontIndex = indFont;

				filePS.Write(string,strlen(string));					

				free(str_ptr);

			}
			break;


//-------------------------------------------------------------
// set font size. 
//
// 4 bytes. PIC_SET_FONT_SIZE
// 4 bytes. size
//-------------------------------------------------------------

			case PIC_SET_FONT_SIZE :		//54c
			{
				long	size;

				size = get_long();
				size = size / 8;
				view->SetFontSize(size);
				sprintf(string,"%ld fs\n",size);
				filePS.Write(string,strlen(string));					
				fontModified = true;
			}
			break;


//-------------------------------------------------------------
// set font shear. 
//
// 4 bytes. PIC_SET_FONT_SHEAR
// 4 bytes. size
//-------------------------------------------------------------

			case PIC_SET_FONT_SHEAR :		//54d
			{
				long	shear;

				shear = get_long();
				view->SetFontShear(shear);
				sprintf(string,"%d fh\n",shear);
				filePS.Write(string,strlen(string));					
				fontModified = true;
			}
			break;


//-------------------------------------------------------------
// set font rotate. 
//
// 4 bytes. PIC_SET_FONT_ROTATE
// 4 bytes. size
//-------------------------------------------------------------

			case PIC_SET_FONT_ROTATE :			//54e
			{
				long angle;
				
				angle = get_long();
				sprintf(string,"%d fr\n",angle);
				filePS.Write(string,strlen(string));					
			}
			break;


//-------------------------------------------------------------
// moveby relative position.
//
// 4 bytes. PIC_MOVEBY
// 4 bytes. x position
// 4 bytes. y position
//-------------------------------------------------------------

			case PIC_MOVEBY :			//54f	//done
				{
					pen_h += get_float();
					pen_v += get_float();
				}	
				break;

//-------------------------------------------------------------
// set scale. 
//
// 4 bytes. PIC_SET_SCALE
// 4 bytes. float scale
//-------------------------------------------------------------

			case PIC_SET_SCALE :		//573
				{
					float	scale;
					scale = get_float();
				}
				break;

//-------------------------------------------------------------
// set font set. 
//
// 4 bytes. PIC_SET_FONT_SET
// 4 bytes. name length
// x bytes. name data
//-------------------------------------------------------------

			case PIC_SET_FONT_SET :			//570
			{
				long		string_length;
				char		*str_ptr;

				// Don't know what to do with this...
				string_length = get_long();
				str_ptr = (char *) malloc(string_length+1);
				get_data(string_length, str_ptr);
				str_ptr[string_length] = '\0';
				view->SetSymbolSet(str_ptr);
				free((char *)str_ptr);
			}
			break;

//-------------------------------------------------------------
// rect frame. 
//
// 4 bytes. PIC_RECTFRAME_PAT
//16 bytes. absolute rectangle
// 8 bytes. pattern
//-------------------------------------------------------------

			case PIC_RECTFRAME_PAT :	
				{
					frect	r;
					pattern	p;

					r = get_rect();
					offset_frect(&r, currentContext->hh, currentContext->vv);
					get_data(sizeof(pattern), &p);
					ConvertCoord(&(r.left),&(r.top));
					ConvertCoord(&(r.right),&(r.bottom));
					sprintf(string,"%f %f %f %f rs\n",r.left,r.top,r.right,r.bottom);
					SetColor(p,string,strlen(string));
				}
				break;

//-------------------------------------------------------------
// rect frame. 
//
// 4 bytes. PIC_RECTFRAME
//16 bytes. absolute rectangle
//-------------------------------------------------------------

			case PIC_RECTFRAME :	
				{
					frect	r;

					r = get_rect();
					offset_frect(&r, currentContext->hh, currentContext->vv);
					if ((currentContext->cur_color.red != for_color.red) || (currentContext->cur_color.green != for_color.green) ||(currentContext->cur_color.blue != for_color.blue)) {
						sprintf(string,"%f %f %f c\n",for_color.red,for_color.green,for_color.blue);
						filePS.Write(string,strlen(string));
						currentContext->cur_color = for_color;
					}
					sprintf(string,"%f %f %f %f rs\n",r.left,r.top,r.right,r.bottom);
					filePS.Write(string,sizeof(string));
				}
				break;

//-------------------------------------------------------------
// fill polygon 
//
// 4 bytes. PIC_FILLPOLY
//16 bytes. rectangle containing the polygon
// 4 bytes. number of points
// 8 bytes * number of points : points coordinates
// 8 bytes  pattern
// 
//-------------------------------------------------------------
			case PIC_FILLPOLY :	
				{
					frect	r;
					pattern	p;
					long	num_pt;
					long	i;
					point	*pt_list;
					float x,y;
					
					r = get_rect();
					num_pt = get_long();
					pt_list = (point *)malloc(num_pt * PTSIZE);
					get_data(PTSIZE * num_pt, pt_list);
					get_data(sizeof(pattern), &p);
					
					// Alloc buffer containing path
					sprintf(string,"%.2f %.2f m\n",1.,1.);
					long len = strlen(string) + 1;
					char *buffer = (char *) malloc (num_pt * len + 4);
					char *ptr;
					ptr = buffer;
				
					x = pt_list[0].h + currentContext->hh;
					y = pt_list[0].v + currentContext->vv;
					ConvertCoord(&x,&y);
					sprintf(ptr,"%.2f %.2f m\n",x,y);
					ptr = ptr + strlen(ptr);
					for (i=1;i<num_pt;i++) {
						x = pt_list[i].h + currentContext->hh;
						y = pt_list[i].v + currentContext->vv;
						ConvertCoord(&x,&y);
						sprintf(ptr,"%.2f %.2f l\n",x,y);
						ptr = ptr + strlen(ptr);
					}
					sprintf(ptr,"f\n");
					ptr = ptr + strlen(ptr);			
					SetColor(p,buffer,(long)(ptr - buffer));
					free(buffer);
					free((char *)pt_list);
				}
				break;

//-------------------------------------------------------------
// stroke polygon 
//
// 4 bytes. PIC_FILLPOLY
//16 bytes. rectangle containing the polygon
// 8 bytes  pattern
// 4 bytes. flag closed or not
// 4 bytes. number of points
// 8 bytes * number of points : points coordinates
// 
//-------------------------------------------------------------
			case PIC_FRAMEPOLY :	
			{
				frect	r;
				pattern	p;
				long	num_pt;
				long	i,closed;
				point	*pt_list;
				float	x,y;

				r = get_rect();
				get_data(sizeof(pattern), &p);
				closed = get_long();
				num_pt = get_long();
				pt_list = (point *)malloc(num_pt * PTSIZE);
				get_data(PTSIZE * num_pt, pt_list);
					
				// Alloc buffer containing path			
				sprintf(string,"%.2f %.2f m\n",1.,1.);
				long len = strlen(string) + 1;
				char *buffer = (char *) malloc (num_pt * len + 6);
				char *ptr;
				ptr = buffer;
				
				x = pt_list[0].h + currentContext->hh;
				y = pt_list[0].v + currentContext->vv;
				ConvertCoord(&x,&y);
				sprintf(ptr,"%.2f %.2f m\n",x,y);
				ptr = ptr + strlen(ptr);
				for (i=1;i<num_pt;i++) {
					x = pt_list[i].h + currentContext->hh;
					y = pt_list[i].v + currentContext->vv;
					ConvertCoord(&x,&y);
					sprintf(ptr,"%.2f %.2f l\n",x,y);
					ptr = ptr + strlen(ptr);
				}
				if (closed) sprintf(ptr,"cp s\n");
				else sprintf(ptr,"s\n");
				ptr = ptr + strlen(ptr);
				SetColor(p,buffer,(long)(ptr - buffer));
				free((char *)pt_list);
				free(buffer);
			}
			break;

//-------------------------------------------------------------
// blit bitmap 
//
//16 bytes. source rectangle
//16 bytes. dest rectangle
// 4 bytes. ????
// 4 bytes  bit per pixels
// 4 bytes. byte per row
//16 bytes. ????
// 4 bytes. size of datas
// datas
// 
//-------------------------------------------------------------

			case PIC_BLIT :
			{
				frect	src_rect;
				frect	dst_rect;
				long	ptype;
				long	bit_per_pixel;
				long	rowbyte;
				frect	bbox;
				long	bits_size;
				int		leftSource,rightSource,topSource,bottomSource;
				
				// Read description of the bitmap
				src_rect = get_rect();
				dst_rect = get_rect();

				ptype = get_long();
				bit_per_pixel = get_long();
				rowbyte = get_long();
				bbox = get_irect();
				bits_size = get_long();

				uchar *data = (uchar *) malloc(sizeof(uchar) * bits_size);
				get_data(bits_size,data); 
				
				// cast to integers
				leftSource = src_rect.left;
				rightSource = src_rect.right;
				topSource = src_rect.top;
				bottomSource = src_rect.bottom;

				// comnpute ancillaries values
				long wis = rightSource - leftSource;
				long his = bottomSource - topSource;
				float ws = src_rect.right - src_rect.left;
				float hs = src_rect.bottom - src_rect.top;
				float wd = dst_rect.right - dst_rect.left;
				float hd = dst_rect.bottom - dst_rect.top;
				long rowSize = (wis + 7) /8;
				
				if (bit_per_pixel == 1) {						// Monochrome bitmap
					sprintf(string,"gs \n");													filePS.Write(string,strlen(string));
					float x,y;
					x = dst_rect.left;
					y = dst_rect.bottom;
					ConvertCoord(&x,&y);
					sprintf(string,"%f %f t\n",x,y);											filePS.Write(string,strlen(string));
					sprintf(string,"%f %f sc\n",wd,hd);											filePS.Write(string,strlen(string));
					if (currentContext->drawMode != B_OP_OVER) {				// We have to draw the 0 component with the background color
						sprintf(string,"%f %f %f c\n",back_color.red,back_color.green,back_color.blue);
						filePS.Write(string,strlen(string));
						sprintf(string,"0 0 m\n");												filePS.Write(string,strlen(string));
						sprintf(string,"1 0 l\n");												filePS.Write(string,strlen(string));
						sprintf(string,"1 1 l\n");												filePS.Write(string,strlen(string));
						sprintf(string,"0 1 l\n");												filePS.Write(string,strlen(string));
						sprintf(string,"cp\n");													filePS.Write(string,strlen(string));
						sprintf(string,"f\n");													filePS.Write(string,strlen(string));
					}
					// Initialize mask to draw foregrounds
					sprintf(string,"%f %f %f c\n",for_color.red,for_color.green,for_color.blue);filePS.Write(string,strlen(string));
					sprintf(string,"/picline %d string def\n",rowSize);							filePS.Write(string,strlen(string));
					sprintf(string,"20 dict begin\n");											filePS.Write(string,strlen(string));
					sprintf(string,"/ImageType 1 def\n");										filePS.Write(string,strlen(string));
					sprintf(string,"/Width %d def\n",wis);										filePS.Write(string,strlen(string));
					sprintf(string,"/Height %d def\n",his);										filePS.Write(string,strlen(string));
					sprintf(string,"/ImageMatrix [%f 0. 0. %f 0. %f] def\n",ws,-hs,hs);			filePS.Write(string,strlen(string));
					sprintf(string,"/DataSource {currentfile picline readhexstring pop} def\n");	filePS.Write(string,strlen(string));
					sprintf(string,"/BitsPerComponent 1 def\n");								filePS.Write(string,strlen(string));
					sprintf(string,"/Decode [1 0] def\n");										filePS.Write(string,strlen(string));
					sprintf(string,"currentdict end\n");										filePS.Write(string,strlen(string));
					sprintf(string,"imagemask\n");													filePS.Write(string,strlen(string));	

					uchar *ptr;
					ptr = data;
					char buffer[181];
					char *ptrBuf;
					uchar patSource,patDest;
					uchar valDest;
					
					// Write values for the mask, converted to hexadecimal form. No more than 256 character per lines
					for (long i=topSource;i<bottomSource;i++) {
						ptr = data + rowbyte*i + leftSource/8;
						patSource = 0x01 << (7 - (leftSource % 8));
						ptrBuf = buffer;
						valDest = 0;
						patDest = 0x01 << 7;
						for (long j=leftSource;j<rightSource;j++) {
							if (patSource & *ptr) {
								valDest = valDest | patDest;
							}
							patSource = patSource >> 1;						
							patDest = patDest >> 1;						
							if (patSource == 0) {
								patSource = 0x01 << 7;
								ptr++;
							}
							if (patDest == 0) {
								patDest = 0x01 << 7;
								sprintf(ptrBuf,"%02X",valDest);
								ptrBuf = ptrBuf + 2;
								valDest = 0;
							}
						
							if ((ptrBuf-buffer)  >= 255) {
								*ptrBuf = '\n';
								filePS.Write(buffer,256);
								ptrBuf = buffer;
							}
						}
						if (ptrBuf != buffer) {
							*ptrBuf = '\n';
							ptrBuf++;
							filePS.Write(buffer,(long)(ptrBuf-buffer));
						}
					}

					sprintf(string,"gr\n");
					filePS.Write(string,strlen(string));
				}
				else if (bit_per_pixel == 8) {					// Indexed bitmap
					// We use an indexed colorspace, if it is for the first time, we write the be color map
					if (!cmapWriten) {
						char buffer[49];
						char *ptr;
						color_map *cmap;
						cmap = system_colors();
						cmapWriten = true;
						sprintf(string,"% Be color map\n");										filePS.Write(string,strlen(string));
						sprintf(string,"/setBeColorMap {\n");									filePS.Write(string,strlen(string));
						sprintf(string,"[/Indexed /DeviceRGB 255 <\n");							filePS.Write(string,strlen(string));
						for (long i = 0;i<32;i++) {
							ptr = buffer;
							for (long j = 0;j<8;j++) {
								sprintf(ptr,"%02X%02X%02X",cmap->color_list[i*8+j].red,cmap->color_list[i*8+j].green,cmap->color_list[i*8+j].blue);
								ptr = ptr + 6;
							}
							*ptr = '\n';
							filePS.Write(buffer,49);
						}
						sprintf(string,"> ] setcolorspace } bdef\n");							filePS.Write(string,strlen(string));
					}				

					sprintf(string,"gs setBeColorMap\n");										filePS.Write(string,strlen(string));
					float x,y;
					x = dst_rect.left;
					y = dst_rect.bottom;
					ConvertCoord(&x,&y);
					// Initialize indexed color space
					sprintf(string,"%f %f t\n",x,y);											filePS.Write(string,strlen(string));
					sprintf(string,"%f %f sc\n",wd,hd);											filePS.Write(string,strlen(string));
					sprintf(string,"/picline %d string def\n",wis);								filePS.Write(string,strlen(string));
					sprintf(string,"20 dict begin\n");											filePS.Write(string,strlen(string));
					sprintf(string,"/ImageType 1 def\n");										filePS.Write(string,strlen(string));
					sprintf(string,"/Width %d def\n",wis);										filePS.Write(string,strlen(string));
					sprintf(string,"/Height %d def\n",his);										filePS.Write(string,strlen(string));
					sprintf(string,"/ImageMatrix [%f 0. 0. %f 0. %f] def\n",ws,-hs,hs);			filePS.Write(string,strlen(string));
					sprintf(string,"/DataSource {currentfile picline readhexstring pop} def\n");	filePS.Write(string,strlen(string));
					sprintf(string,"/BitsPerComponent 8 def\n");								filePS.Write(string,strlen(string));
					sprintf(string,"/Decode [0 255] def\n");									filePS.Write(string,strlen(string));
					sprintf(string,"currentdict end\n");										filePS.Write(string,strlen(string));
					sprintf(string,"image\n");													filePS.Write(string,strlen(string));	

					// Write datas
					uchar *ptr;
					ptr = data;
					char buffer[201];
					char *ptrBuf;
					for (long i=topSource;i<bottomSource;i++) {
						ptr = data + rowbyte*i + (long) leftSource;
						ptrBuf = buffer;
						for (long j=leftSource;j<rightSource;j++,ptr++) {
							sprintf(ptrBuf,"%02X",*ptr);
							ptrBuf = ptrBuf + 2;
							if ((j-leftSource+1)%100 == 0) {
								*ptrBuf = '\n';
								filePS.Write(buffer,201);
								ptrBuf = buffer;
							}
						}
						if (wis%100 != 0) {
							*ptrBuf = '\n';
							filePS.Write(buffer,(wis%100)*2+1);
						}
					}

					sprintf(string,"gr\n");
					filePS.Write(string,strlen(string));
				}
				else if (bit_per_pixel == 32) {
					// We are in an RGBA color space
					sprintf(string,"gs /DeviceRGB setcolorspace\n");							filePS.Write(string,strlen(string));
					float x,y;
					x = dst_rect.left;
					y = dst_rect.bottom;
					ConvertCoord(&x,&y);
					sprintf(string,"%f %f t\n",x,y);											filePS.Write(string,strlen(string));
					sprintf(string,"%f %f sc\n",wd,hd);											filePS.Write(string,strlen(string));
					sprintf(string,"/picline %d string def\n",wis*3);							filePS.Write(string,strlen(string));
					sprintf(string,"20 dict begin\n");											filePS.Write(string,strlen(string));
					sprintf(string,"/ImageType 1 def\n");										filePS.Write(string,strlen(string));
					sprintf(string,"/Width %d def\n",wis);										filePS.Write(string,strlen(string));
					sprintf(string,"/Height %d def\n",his);										filePS.Write(string,strlen(string));
					sprintf(string,"/ImageMatrix [%f 0. 0. %f 0. %f] def\n",ws,-hs,hs);			filePS.Write(string,strlen(string));
					sprintf(string,"/DataSource {currentfile picline readhexstring pop} def\n");	filePS.Write(string,strlen(string));
					sprintf(string,"/BitsPerComponent 8 def\n");								filePS.Write(string,strlen(string));
					sprintf(string,"/Decode [0 1 0 1 0 1] def\n");								filePS.Write(string,strlen(string));
					sprintf(string,"currentdict end\n");										filePS.Write(string,strlen(string));
					sprintf(string,"image\n");													filePS.Write(string,strlen(string));	

					uchar *ptr;
					ptr = data;
					char buffer[181];
					char *ptrBuf;
					for (long i=topSource;i<bottomSource;i++) {
						ptr = data + rowbyte*i + leftSource * 4;
						ptrBuf = buffer;
						for (long j=leftSource;j<rightSource;j++,ptr= ptr+4) {
							sprintf(ptrBuf,"%02X%02X%02X",*ptr,*(ptr+1),*(ptr+2));
							ptrBuf = ptrBuf + 6;
							if ((j-leftSource+1)%30 == 0) {
								*ptrBuf = '\n';
								filePS.Write(buffer,181);
								ptrBuf = buffer;
							}
						}
						if (wis%30 != 0) {
							*ptrBuf = '\n';
							filePS.Write(buffer,(wis%30)*6+1);
						}
					}

					sprintf(string,"gr\n");
					filePS.Write(string,strlen(string));
				}
				free(data);
			}
			break;
			

//-------------------------------------------------------------
// line arrays 
//
// 4 bytes. number of points
// 		4 bytes  x0
// 		4 bytes  y0
// 		4 bytes  x1
// 		4 bytes  y1
// 		1 bytes  red
// 		1 bytes  green
// 		1 bytes  blue
// 
//-------------------------------------------------------------
			case PIC_VARRAY :
			{
				long		num_pt, i;
				a_line		the_line;
				pscolor		color,currentColor;

				num_pt = get_long();
				for (i = 0; i < num_pt; i++) {
					get_data(sizeof(a_line), &the_line);
					color.red = ((float) the_line.col.red) / 255.;
					color.green = ((float) the_line.col.green) / 255.;
					color.blue = ((float) the_line.col.blue) / 255.;
					if (	(i>0) ||
						 	(currentColor.red != color.red) || 
						 	(currentColor.green != color.green) || 
						 	(currentColor.blue != color.blue)) {
						sprintf(string,"%f %f %f c\n",color.red,color.green,color.blue);
						filePS.Write(string,strlen(string));
						currentColor = color;			 	
					}
					the_line.x0 += currentContext->hh;
					the_line.x1 += currentContext->hh;
					the_line.y0 += currentContext->vv;
					the_line.y1 += currentContext->vv;
					ConvertCoord(&(the_line.x0),&(the_line.y0));
					ConvertCoord(&(the_line.x1),&(the_line.y1));
					sprintf(string,"%.2f %.2f m\n",the_line.x0,the_line.y0);
					filePS.Write(string,strlen(string));
					sprintf(string,"%.2f %.2f l s\n",the_line.x1,the_line.y1);
					filePS.Write(string,strlen(string));
				}
			}
			break;


//-------------------------------------------------------------
// End of picture 
//
// 4 bytes. PIC_END_PICTURE
//
//-------------------------------------------------------------

			case PIC_END_PICTURE :			//575
			{
				Output("PIC_END_PICTURE\n");
				if (currentContext->previous == NULL) {
					Output("End\n");
					endOfPicture = true;
				}
				else {
					Output("Pop\n");
					context *oldContext;
					oldContext = currentContext;
					currentContext = currentContext->previous;
					delete oldContext;
					sprintf(string,"gr\n");
					filePS.Write(string,strlen(string));
					fontModified = true;
				}
				break;
			}

//-------------------------------------------------------------

			case PIC_SUB_PICTURE :
				fontModified = true;
				Output("PIC_SUB_PICTURE\n");
				break;

//-------------------------------------------------------------
// Offset frame 
//
// 4 bytes. PIC_OFFSET_FRAME
// 4 bytes. x offset
// 4 bytes. y offset
//
//-------------------------------------------------------------
			
			case PIC_OFFSET_FRAME :			//576
			{
				Output("PIC_OFFSET_FRAME\n");
				float	ddh, ddv;
				context *newContext = new context(currentContext);
				currentContext = newContext;

				ddh = get_float();
				ddv = get_float();
				currentContext->hh = currentContext->hh + ddh;
				currentContext->vv = currentContext->vv + ddv;

				sprintf(string,"gs\n");
				filePS.Write(string,strlen(string));
			}
			break;
//-------------------------------------------------------------
// Bug or new feature????
//
//-------------------------------------------------------------
			default:
				Output("----------->UNKNOWN OPCODE<---------\n");
		}
	}
	Output("End Of ALL");
	delete currentContext;
	file.Close();
	sprintf(string,"showpage\n");
	filePS.Write(string,strlen(string));
	filePS.Close();

	win->Unlock();
	win->Quit();
	textResult->Window()->Unlock();
}

