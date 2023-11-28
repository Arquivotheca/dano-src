// ===========================================================================
//	FileSystemHTML.cpp
//  Copyright 1998 by Be Incorporated.
//=======================================================================

#include "FileSystemHTML.h"
#include "UResource.h"

#include <stdio.h>

extern const char *kInternalURLPrefix;
extern const char *kFSHTMLMBLabel;
extern const char *kFSHTMLKBLabel;
extern const char *kFSHTMLOneByteLabel;
extern const char *kFSHTMLManyBytesLabel;
extern const char *kFSHTMLUpLevel;
extern const char *kFSHTMLEmptyLabel;

//=======================================================================
//	Class that turns the ftp output into a nice HTML document

FileSystemHTML::FileSystemHTML(UResourceImp* resImp) : mResImp(resImp), mCount(0)
{
}

FileSystemHTML::~FileSystemHTML()
{
}

//	(Sep 20 18:01 or Aug  7  1994)

long FTPDate(const char*)
{
	return 0;
}


//	Add a file to list to be displayed

void FileSystemHTML::AddFile(char fType, long size, long date, const char* name)
{
	mType[mCount] = fType;
	mSize[mCount] = size;
	mDate[mCount] = date;
	mName[mCount] = name;
	mCount++;
}

//	Add a file or directory to HTML

void FileSystemHTML::WriteFile(char fType, long size, long, const char* name)
{
	char str[1024];
	char slash[6];
	
	if (!mResImp->Lock()) return;
	strcpy(slash,"/");
	
//	Icon and name cell

	char *image;
	if (fType == 'd') {
		image = "Folder.gif";	// Different icons for different types,folder link has slash
		if (name[strlen(name)-1] == '/')
			slash[0] = 0;
	} else {
		image = "File.gif";
		if (strstr(name,"gif") || strstr(name,"jpg") || strstr(name,"GIF") || strstr(name,"JPG"))
			image = "Image.gif";
		else if (strstr(name,"htm") || strstr(name,"HTM"))
			image = "HTML.gif";
		else if (strstr(name,"txt") || strstr(name,"doc") || strstr(name,"TXT") || strstr(name,"DOC"))
			image = "Text.gif";
		slash[0] = 0;
	}
	sprintf(str,"<TR><TD WIDTH=80%% BGCOLOR=#F0F0F0><A HREF=\"%s%s\"><IMG SRC=\"%s%s\" ALIGN=ABSMIDDLE BORDER=0 HSPACE=6>%s</A></TD>",name,slash,kInternalURLPrefix,image, name);
	HTML(str,false);

//	Size Cell

	HTML("<TD ALIGN=RIGHT WIDTH=20%>",false);
	if (fType == 'd') {
		strcpy(str,"</TD>");
	} else {
		if (size >= 1024)
			if (size >= 1024*1024)
				sprintf(str,kFSHTMLMBLabel,(float)size/(1024*1024));		// Meg
			else
				sprintf(str,kFSHTMLKBLabel,(size + 512)/1024);					// Kilo
		else {
			if (size == 1)
				strcpy(str,kFSHTMLOneByteLabel);
			else
				sprintf(str,kFSHTMLManyBytesLabel,size);							// Bytes
		}
	}
	HTML(str,false);
	HTML("</TR>");
	
	mResImp->Unlock();
}

//	Write a line of HTML to resource

void FileSystemHTML::HTML(const char* html, bool cr)
{
//	NP_ASSERT(mResImp->Write(html,strlen(html), false) == 0);
	mResImp->Write(html,strlen(html), false);
	if (cr)
//		NP_ASSERT(mResImp->Write("\r\n",2, false) == 0);
		mResImp->Write("\r\n",2, false);
}

//	Write an html page of this directory

void FileSystemHTML::WriteHeader()
{
	char str[1024];
	char frag[1024];
	char pathAnchor[1024];
	char *s,*p;

	if (!mResImp->Lock()) return;
	mResImp->SetContentType("text/html");
	
//	Heading if the current path name (or perhaps path?)

	const char *url = mResImp->GetURL();
	sprintf(str,"<HTML><HEAD><TITLE>%s</TITLE></HEAD><BODY BGCOLOR=#F0F0F0>",url + 6);
	HTML(str);

//	Draw the path at the top of the directory, make each level in the path clickable

	HTML("<HR>");
	HTML("<TABLE WIDTH=100% CELLSPACING=0 BORDER=0><TR><TD WIDTH=85% BGCOLOR=#E0E0E0>");
	pathAnchor[0] = 0;
	
	char path[1024];
	strcpy(path,url);
	p = strchr(path + 7,'/');
	
	if (p) {
		while ((bool)(s = strchr(p,'/'))) {	// file://NewDisk/
			if (s > p) {
				strcpy(frag,p);
				frag[s-p] = 0;		// Fragment of path
				s[0] = 0;			// Crop mPath
				if (s[1] == 0)
					sprintf(str,"<A HREF=\"%s/\"><B>%s</B></A>",path,frag);	// Add current directory
				else {
					sprintf(pathAnchor,"<A HREF=\"%s/\">",path);
					sprintf(str,"%s%s</A>&nbsp;|&nbsp;",pathAnchor,frag); 	// Add cropped path
				}
				HTML(str,false);
				s[0] = '/';
			}
			p = s + 1;
		}
	} else {
		sprintf(str, "&nbsp;|&nbsp;"); 	// At Top, just show divider
		HTML(str);
	}
	
//	Up one level

	if (pathAnchor[0]) {
		sprintf(str,"</TD><TD ALIGN=RIGHT WIDTH=15%% BGCOLOR=#E0E0E0>%s%s</A><IMG SRC=\"%sUpArrow.gif\" BORDER=0 ALIGN=ABSMIDDLE HSPACE=2>",pathAnchor, kFSHTMLUpLevel, kInternalURLPrefix);
		HTML(str,false);
	}
	
	HTML("</TD></TR></TABLE>");
	
//	Table Showing names

	HTML("<HR>");
	HTML("<TABLE WIDTH=100% BORDER=0>");

	mResImp->Unlock();
}

void FileSystemHTML::WriteTrailer()
{
	if (!mResImp->Lock()) return;
	
	if (mCount == 0)
		HTML(kFSHTMLEmptyLabel);
	else {
		int i;
		if (mCount < 20) {					// Single col
			for (i = 0; i < mCount; i++)
				WriteFile(mType[i], mSize[i], mDate[i], mName[i].String());
		} else {							// Double col
			 	HTML("<TD WIDTH=50%><TABLE BORDER=0>");
				for (i = 0; i < (mCount+1)/2; i++)
					WriteFile(mType[i], mSize[i], mDate[i], mName[i].String());
				HTML("</TABLE></TD>");
				
			 	HTML("<TD WIDTH=50%><TABLE BORDER=0>");
				for (; i < mCount; i++)
					WriteFile(mType[i], mSize[i], mDate[i], mName[i].String());
				HTML("</TABLE></TD>");
			
		}
	}
		
	HTML("</TABLE>");	
	HTML("<HR>");
	HTML("</BODY>");
	
	mResImp->SetContentLength(mResImp->GetLength());
	mResImp->Unlock();
	mResImp->NotifyListeners(msg_ResourceChanged);
}
