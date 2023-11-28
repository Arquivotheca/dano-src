

#ifndef KEY_H
#define KEY_H

#include <View.h>


// Forward References
class SoftKeyboard;
class BBitmap;


struct Cell
{
	BRect	rect;
	
	// Are the sides joined to the adjacent cell?
	unsigned	join_left:1;
	unsigned	join_top:1;
	unsigned	join_right:1;
	unsigned	join_bottom:1;
	
	// Are we a concave corner? This applies to the cell
	// opposite to the one that isn't of the key
	// eg.
	//    _______________
	//   |               |
	//   |   1       2   |
	//   |        _______|
	//   |       |
	//   |   3   |   4
	//   |_______|
	// 
	// Cell 1 has the concave_rb bit set.
	unsigned	concave_LT:1;	// Left Top
	unsigned	concave_RT:1;	// Right Top
	unsigned	concave_RB:1;	// Right Bottom
	unsigned	concave_LB:1;	// Left Bottom
	
	// Are we a normal corner?
	//    _______________
	//   |X             X|
	//   |   1       2   |
	//   |        ______X|
	//   |       |
	//   |   3   |   4
	//   |X_____X|
	// 
	// The corners with the Xes have the corner bit set
	unsigned	corner_LT:1;	// Left Top
	unsigned	corner_RT:1;	// Right Top
	unsigned	corner_RB:1;	// Right Bottom
	unsigned	corner_LB:1;	// Left Bottom
	
	// Index for the key that this cell is a member of
	uint8		key;
};


class Key
{
private:
					Key(uint8 keyCode);
	virtual			~Key();
	
	void			AddCell(Cell * cell);
	//virtual void	Draw(BView * view, SoftKeyboard * kbd);
	
	typedef enum
	{
		UP,
		DOWN,
		STICKY_DOWN
	}cell_state;
	
	uint8		_keyCode;
	int8		_cellCount;
	uint32		_modifierKey;
	Cell		** _cells;
	char		* _label;		// If _label is NULL, use keymap
	BFont		* _font;
	cell_state	_state;
	
	BBitmap		* _upBitmap;
	BBitmap		* _downBitmap;
	
								// Note: Maximum length of a UTF-8 char is
								// 4 bytes, so it's more efficient to store
								// the strings than pointers to strings.
	char		_normalChar[5];
	char		_shiftChar[5];
	char		_capsChar[5];
	char		_capsShiftChar[5];
	char		_optionChar[5];
	char		_optionShiftChar[5];
	char		_optionCapsChar[5];
	char		_optionCapsShiftChar[5];
	char		_controlChar[5];
	
	uint8		_normalCharWidth;
	uint8		_shiftCharWidth;
	uint8		_capsCharWidth;
	uint8		_capsShiftCharWidth;
	uint8		_optionCharWidth;
	uint8		_optionShiftCharWidth;
	uint8		_optionCapsCharWidth;
	uint8		_optionCapsShiftCharWidth;
	uint8		_controlCharWidth;
	
	friend SoftKeyboard;
};

#define NORMAL_CHAR_TABLE				0
#define SHIFT_CHAR_TABLE				1
#define CAPS_CHAR_TABLE					2
#define CAPS_SHIFT_CHAR_TABLE			3
#define OPTION_CHAR_TABLE				4
#define OPTION_SHIFT_CHAR_TABLE			5
#define OPTION_CAPS_CHAR_TABLE			6
#define OPTION_CAPS_SHIFT_CHAR_TABLE	7
#define CONTROL_CHAR_TABLE				8


#endif
