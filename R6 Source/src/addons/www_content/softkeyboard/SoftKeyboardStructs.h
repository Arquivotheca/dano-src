

#ifndef SOFTKEYBOARDSTRUCTS_H
#define SOFTKEYBOARDSTRUCTS_H


struct kbd_file_header
{
	char		magic_1[8];				// "kbdfile"
	int32		kbd_info_start;
	int32		kbd_cell_count;
	int32		kbd_cell_start;
	int32		drawing_info_count;
	int32		drawing_info_start;
	int32		kbd_key_count;
	int32		kbd_key_start;
	char		magic_2[8];				// "kbdfile"
};

struct kbd_info
{
	// How far apart the keys are
	float		borderWidth;
	
	// Colors
	rgb_color	keyUpColor;
	rgb_color	keyDownColor;
	rgb_color	specialKeyUpColor;
	rgb_color	specialKeyDownColor;
	rgb_color	borderColor;
	rgb_color	textColor;
	
	// Default Font (can be overridden by special_drawing_info)
	font_family	fontFamily;
	font_style	fontStyle;
	float		normalFontSize;
	
	// Default Font for the Special Text Things
	font_family	specialFontFamily;
	font_style	specialFontStyle;
	float		specialFontSize;
	
	// How many cells there are
	int8		xCellCount;
	int8		yCellCount;
	int16		cellCount;		// Double check
	
	// How big the cells are
	float		cellWidth;
	float		cellHeight;
	
	// Which type of drawing do we do?
	uint8		drawingMode;	//	0	Normal
								//	1 	Rounded
								//	2	3d
	
	// For Rounded mode only:
	
	// How big the radius
	float		roundedRadius;	
	
	
	// For 3d mode only:
	
	// Bevel Width
	int32		bevelWidth;
	
	// Bevel Colors
	rgb_color	bevelLeftColorUp;
	rgb_color	bevelLeftColorDown;
	rgb_color	bevelTopColorUp;
	rgb_color	bevelTopColorDown;
	rgb_color	bevelRightColorUp;
	rgb_color	bevelRightColorDown;
	rgb_color	bevelBottomColorUp;
	rgb_color	bevelBottomColorDown;
	
	// How much the glyph, etc. is shifted in the up and down states
	// helps to give the illusion of pushing
	int32		pressShiftUpX;
	int32		pressShiftUpY;
	int32		pressShiftDownX;
	int32		pressShiftDownY;
	
};

// The cell is the unit of drawing
struct kbd_cell_info
{
	// An index for the key into the key_info structure
	uint8		key;
	
	// Should we join this with the cell to the top, etc
	unsigned	join_left:1;
	unsigned	join_top:1;
	unsigned	join_right:1;
	unsigned	join_bottom:1;
};

// The key is the unit of motion
// When the real key objects are constructed, they know which
// cells the keys occupy.  The soft_key_info struct doesn't
struct soft_key_info
{
	// Be hardware independent key code
	uint8		keyCode;
	
	// Special drawing information
	// index of the drawing info structure to use
	int8		specialDrawingInfo;
	
};


// Type codes:
// 		0x01		Text
// 		0x02		Graphic

struct special_drawing_info
{
	char		specialLabel[256];
	uint32		type;
	float		fontSize;
	font_family	fontFamily;
	font_style	fontStyle;
};


#endif // SOFTKEYBOARDSTRUCTS_H
