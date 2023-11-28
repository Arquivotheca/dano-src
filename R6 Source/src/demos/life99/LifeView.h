/*
	
	HelloView.h
	
*/
/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#ifndef HELLO_VIEW_H
#define HELLO_VIEW_H

#ifndef _STRING_VIEW_H
#include <StringView.h>
#endif

#include <Point.h>

#define XMAX 73
#define YMAX 49

class Cell
{
public:
	int32	kNeighbors;
	int32	kAge;
	bool	kIsAlive;
	Cell(void)	{kIsAlive = false; kNeighbors = 0; kAge = 0;}
	void	CheckSelf (void);
};

class Grid
{
public:
	Cell	kCells[XMAX][YMAX];
	Grid 	(void);
	void	CheckNeighbors(void);
	void	Refresh(void);
	bool	fWrapAround;
	bool	fOldAge;
};

class HelloView : public BStringView 
{
public:
		HelloView(BRect frame, const char *name, const char *text);
		void	Draw(BRect);
		Grid	fGrid;
		int32	fRuns;
		bool	fShow;
		bool	fEndless;
		bool	fMouseDown;
		bool	fLazarus;
		bool	fDone;
		void	DrawGrid (void);
		void	DrawCell (int32, int32);
static	int32	_Iterate (void*);
		int32	Iterate (void);
virtual	void	MouseDown (BPoint);
virtual	void	MouseUp	(BPoint);
virtual	void	MouseMoved	(BPoint, uint32, const BMessage*);
		void	ClearCells(void);
static	int32	_ToggleCells(void*);
		int32	ToggleCells(void);
};

#endif //HELLO_VIEW_H
