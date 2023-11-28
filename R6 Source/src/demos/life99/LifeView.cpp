/*
	
	HelloView.cpp
	
*/
/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#ifndef HELLO_VIEW_H
#include "LifeView.h"
#endif
///////////////////////////////////////////////////////////////////////////////////////
#include <Window.h>
#include <stdio.h>
///////////////////////////////////////////////////////////////////////////////////////
void	Cell::CheckSelf(void)
{
	if (kNeighbors == 2);
	else if (kNeighbors == 3) kIsAlive = true;
	else kIsAlive = false;
}
///////////////////////////////////////////////////////////////////////////////////////
Grid::Grid	(void)
{
	for ( int32 x = 0; x < XMAX; x++)
	{
		for ( int32 y = 0; y < YMAX; y++)
		{
			if  ( ((x*y)%12) == 0 ) kCells[x][y].kIsAlive = true;
		}
	}
	
	fWrapAround = false;
	fOldAge = true;
}
///////////////////////////////////////////////////////////////////////////////////////
void Grid::CheckNeighbors (void)
{
	bool	kWrapAround = fWrapAround;

	for ( int32 x = 0; x < XMAX; x++)
	{
		for ( int32 y = 0; y < YMAX; y++)
		{
			kCells[x][y].kNeighbors = 0;
			if (x>0)
			{
				if (y>0) 				{if (kCells[x-1][y-1   ].kIsAlive) kCells[x][y].kNeighbors++;}
				else if (kWrapAround) 	 if (kCells[x-1][YMAX-1].kIsAlive) kCells[x][y].kNeighbors++;
										 if (kCells[x-1][y     ].kIsAlive) kCells[x][y].kNeighbors++;
				if (y<(YMAX-1)) 		{if (kCells[x-1][y+1   ].kIsAlive) kCells[x][y].kNeighbors++;}
				else if (kWrapAround) 	 if (kCells[x-1][0     ].kIsAlive) kCells[x][y].kNeighbors++;
			}
			
			else if (kWrapAround)
			{
				if (y>0) 				{if (kCells[XMAX-1][y-1   ].kIsAlive) kCells[x][y].kNeighbors++;}
				else if (kWrapAround) 	 if (kCells[XMAX-1][YMAX-1].kIsAlive) kCells[x][y].kNeighbors++;
										 if (kCells[XMAX-1][y     ].kIsAlive) kCells[x][y].kNeighbors++;
				if (y<(YMAX-1)) 		{if (kCells[XMAX-1][y+1   ].kIsAlive) kCells[x][y].kNeighbors++;}
				else if (kWrapAround) 	 if (kCells[XMAX-1][0     ].kIsAlive) kCells[x][y].kNeighbors++;
			}
			
			if (y>0) 				{if (kCells[x][y-1   ].kIsAlive) kCells[x][y].kNeighbors++;}
			else if (kWrapAround) 	 if (kCells[x][YMAX-1].kIsAlive) kCells[x][y].kNeighbors++;
			if (y<(YMAX-1)) 		{if (kCells[x][y+1   ].kIsAlive) kCells[x][y].kNeighbors++;}
			else if	(kWrapAround) 	 if (kCells[x][0     ].kIsAlive) kCells[x][y].kNeighbors++;
			
			if (x<(XMAX-1))
			{
				if (y>0) 				{if (kCells[x+1][y-1   ].kIsAlive) kCells[x][y].kNeighbors++;}
				else if (kWrapAround)	 if (kCells[x+1][YMAX-1].kIsAlive) kCells[x][y].kNeighbors++;
										 if (kCells[x+1][y     ].kIsAlive) kCells[x][y].kNeighbors++;
				if (y<(YMAX-1))  		{if (kCells[x+1][y+1   ].kIsAlive) kCells[x][y].kNeighbors++;}
				else if (kWrapAround)	 if (kCells[x+1][0     ].kIsAlive) kCells[x][y].kNeighbors++;
			}
			
			else if (kWrapAround)
			{
				if (y>0) 				{if (kCells[0][y-1   ].kIsAlive) kCells[x][y].kNeighbors++;}
				else if (kWrapAround)	 if (kCells[0][YMAX-1].kIsAlive) kCells[x][y].kNeighbors++;
										 if (kCells[0][y     ].kIsAlive) kCells[x][y].kNeighbors++;
				if (y<(YMAX-1))  		{if (kCells[0][y+1   ].kIsAlive) kCells[x][y].kNeighbors++;}
				else if (kWrapAround)	 if (kCells[0][0     ].kIsAlive) kCells[x][y].kNeighbors++;
			}
		}
	}
}
///////////////////////////////////////////////////////////////////////////////////////
void	Grid::Refresh(void)
{
	for ( int32 x = 0; x < XMAX; x++)
	{
		for ( int32 y = 0; y < YMAX; y++)
		{
				
			if ( (kCells[x][y].kAge >= 2550) && fOldAge )			//Death due to old age
			{
				kCells[x][y].kAge = 0;
				kCells[x][y].kIsAlive = false;
			}
			else switch (kCells[x][y].kNeighbors)
			{
				case 2:
					if (kCells[x][y].kIsAlive) kCells[x][y].kAge++;
					break;
				case 3: 
					if (kCells[x][y].kIsAlive) kCells[x][y].kAge++;
					kCells[x][y].kIsAlive = true;
					break;
				default: 
					kCells[x][y].kIsAlive = false;
					kCells[x][y].kAge = 0;
			}
		}
	}
}
///////////////////////////////////////////////////////////////////////////////////////
void	HelloView::DrawGrid(void)
{
	BRect 		cellFrame;
	int32		aging;
	rgb_color	fillColor = {0, 0, 0, 255};
	
	Window()->Lock();
	
	SetLowColor(fillColor);
	
	for (int32 x = 0; x < XMAX; x++)
	{
		for (int32 y = 0; y < YMAX; y++)
		{
			cellFrame.Set( (x*8), (y*8), (7 + x*8), (7 + y*8) );

			if (fGrid.kCells[x][y].kIsAlive)	
			{
				aging = fGrid.kCells[x][y].kAge * 10;
								
				if (aging > 25500)	aging = 25500;
				fillColor.green = aging/100;
				
				if (aging > 255) aging = 255;
				fillColor.red = aging;
				SetHighColor(fillColor);
				
				aging = fGrid.kCells[x][y].kAge * 4;
				if (aging > 255) aging = 255;
				fillColor.red = 255 - aging/2;
				fillColor.blue = aging;
				
				SetHighColor(fillColor);
				FillRect(cellFrame, B_SOLID_HIGH);
			}
			else								FillRect(cellFrame, B_SOLID_LOW);
		}
	}
	Flush();

	Window()->Unlock();
}
///////////////////////////////////////////////////////////////////////////////////////
void	HelloView::Draw(BRect narf)
{
	DrawGrid(); 
}
///////////////////////////////////////////////////////////////////////////////////////
HelloView::HelloView(BRect rect, const char *name, const char *text)
	   	   : BStringView(rect, name, text, B_FOLLOW_ALL, B_WILL_DRAW)
{
	SetFont(be_bold_font);
	SetFontSize(24);
	
	fRuns = 0;
	fMouseDown = false;
}
///////////////////////////////////////////////////////////////////////////////////////
int32	HelloView::_Iterate(void* param) 
{
	return ((HelloView*)param)->Iterate();
}
///////////////////////////////////////////////////////////////////////////////////////
int32	HelloView::Iterate (void)
{
	void*		kBuffer = NULL;
	size_t		kBufferSize = 0;
	thread_id*	kSender = NULL;
	int32		kMessage;
		
	for (int32 i = 0; (i < fRuns) || fEndless; i++)	
	{
		if(has_data(find_thread(NULL)))
		{
			kMessage = receive_data(kSender, kBuffer, kBufferSize);
			if ( kMessage == 'stop') 
			{
				if (!fShow)	DrawGrid();
				exit_thread(0);
			}
		}
		
		fGrid.CheckNeighbors();
		fGrid.Refresh();
		if (fShow) DrawGrid();
	}
	
	if (!fShow)	DrawGrid();
	
	return 0;
}
///////////////////////////////////////////////////////////////////////////////////////
void	HelloView::MouseDown (BPoint point)
{
	point.x /= 8;
	point.y /= 8;
	
	int32	x = (int32) point.x;
	int32	y = (int32) point.y;
	
	if (fGrid.kCells[x][y].kIsAlive)	
	{
		fLazarus = false;
		fGrid.kCells[x][y].kIsAlive = false;
	}
	else								
	{
		fLazarus = true;
		fGrid.kCells[x][y].kIsAlive = true;
	}
	
	fMouseDown = true;
	
	DrawCell(x,y);
}
///////////////////////////////////////////////////////////////////////////////////////
void	HelloView::MouseMoved(BPoint point, uint32 code, const BMessage *a_message)
{
	if (code == B_EXITED_VIEW) fMouseDown = false;
	
	if (fMouseDown)
	{
		thread_id	kThread;
		
		fDone = true;
		kThread = spawn_thread(HelloView::_ToggleCells, "Mouser", 25, this);
		
		int32 outBuffer[2]= {(int32) point.x, (int32) point.y};
		
		send_data(kThread, code, (void*) outBuffer, sizeof(outBuffer));
		resume_thread(kThread);		
	}
}
///////////////////////////////////////////////////////////////////////////////////////
void	HelloView::MouseUp(BPoint where)
{
	fMouseDown = false;
}	
///////////////////////////////////////////////////////////////////////////////////////
void	HelloView::ClearCells(void)
{
	for (int32 x = 0; x < XMAX; x++)
	{
		for (int32 y = 0; y < YMAX; y++)
		{
			fGrid.kCells[x][y].kIsAlive = false;
		}
	}

	DrawGrid();
}
///////////////////////////////////////////////////////////////////////////////////////
int32	HelloView::_ToggleCells(void* param)
{
	return ((HelloView*)param)->ToggleCells();
}
///////////////////////////////////////////////////////////////////////////////////////
int32	HelloView::ToggleCells(void)
{
	uint32		code;
	thread_id*	kSender = NULL;
	
	int32	kBuffer[2];
	
	code = receive_data(kSender, (void*) kBuffer, sizeof(kBuffer));
			
	{
		kBuffer[0] /= 8;		
		kBuffer[1] /= 8;
		
		if ( (kBuffer[0] < 0) || (kBuffer[0]>=XMAX) )	return 0;
		if ( (kBuffer[1] < 0) || (kBuffer[1]>=YMAX) )	return 0;
					
		if (fLazarus)	fGrid.kCells[kBuffer[0]][kBuffer[1]].kIsAlive = true;
		else			fGrid.kCells[kBuffer[0]][kBuffer[1]].kIsAlive = false;
		
		DrawCell(kBuffer[0],kBuffer[1]);
	}
	
	return 0;
}
///////////////////////////////////////////////////////////////////////////////////////
void	HelloView::DrawCell (int32 x, int32 y)
{
	BRect 		cellFrame;
	int32		aging;
	rgb_color	fillColor = {0, 0, 0, 255};
	
	Window()->Lock();
	
	SetLowColor(fillColor);
	
	cellFrame.Set( (x*8), (y*8), (7 + x*8), (7 + y*8) );

	if (fGrid.kCells[x][y].kIsAlive)	
	{
		aging = fGrid.kCells[x][y].kAge * 10;
		
		if (aging > 25500)	aging = 25500;
		fillColor.green = aging/100;
		
		if (aging > 255) aging = 255;
		fillColor.red = aging;
		SetHighColor(fillColor);
			
		aging = fGrid.kCells[x][y].kAge * 4;
		if (aging > 255) aging = 255;
		fillColor.red = 255 - aging/2;
		fillColor.blue = aging;
		
		SetHighColor(fillColor);
		FillRect(cellFrame, B_SOLID_HIGH);
	}
	else								FillRect(cellFrame, B_SOLID_LOW);
	
	Flush();

	Window()->Unlock();
}