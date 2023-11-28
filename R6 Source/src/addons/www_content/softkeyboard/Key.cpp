
#include "Key.h"
#include "SoftKeyboard.h"

#include <Bitmap.h>

#include <math.h>
#include <stdlib.h>
#include <stdio.h>

#define SFTKBD_DRAW_COLORED_BOXES 0

Key::Key(uint8 keyCode)
	:_keyCode(keyCode),
	 _cellCount(0),
	 _modifierKey(0),
	 _cells(NULL),
	 _label(NULL),
	 _font(NULL),
	 _state(UP),
	 _upBitmap(NULL),
	 _downBitmap(NULL)
{
	
}


Key::~Key()
{
	
	// The actual cells are deleted by SoftKeyboard, but
	// we delete our array of pointers to them
	free(_cells);
	
	delete _font;
	
	// _upBitmap and _downBitmap get deleted when the BResourceSet is deleted
}


void
Key::AddCell(Cell * cell)
{
	++_cellCount;
	
	if (_cells)
		_cells = (Cell**) realloc(_cells, _cellCount * sizeof(Cell *));
	else
		_cells = (Cell**) malloc(sizeof(Cell *));
	
	_cells[_cellCount - 1] = cell;
}

#if 0
void
Key::Draw(BView * view, SoftKeyboard * kbd)
{
}
#endif 
