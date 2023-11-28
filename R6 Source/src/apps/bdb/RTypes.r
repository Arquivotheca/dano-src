type 'MENU' {
	cstring;						// name
	array {
		switch {
			case Item:
				key byte = 1;
				cstring;				 	// label
				longint;					// command
				integer none = 0,			// modifier keys
						shift = 1,
						control = 4,
						option = 64;
				byte noKey = 0;				// command key
			case ColorItem:
				key byte = 2;
				cstring;					// label
				longint;					// command
				integer none = 0;			// modifiers
				byte noKey = 0;				// key
				byte;						// color red
				byte;						//		 green
				byte;						//		 blue
			case Separator:
				key byte = 3;
			case Submenu:
				key byte = 4;
				longint;					// submenuid
		}
	};
	byte = 0;
};

type 'MBAR' {
	array {
		integer;		// menu id's
	};
};

type 'DLOG' {
	rect;										// bounds
	cstring;									// name
	longint B_TITLED_WINDOW = 1, B_MODAL_WINDOW = 3,
		B_DOCUMENT_WINDOW = 11, B_BORDERED_WINDOW = 20;
	longint NORMAL = 0x00004042,
		B_WILL_ACCEPT_FIRST_CLICK = 0x00000010;
	longint = $$countof(items);
	array items {
		switch {
			case Button:
				key longint = 'btn ';
				rect;							// bounds
				cstring;						// name
				cstring;						// label
				longint;						// command
			case RadioButton:
				key longint = 'radb';
				rect;							// bounds
				cstring;						// name
				cstring;						// label
			case CheckBox:
				key longint = 'chkb';
				rect;							// bounds
				cstring;						// name
				cstring;						// label
			case Edit:
				key longint = 'edit';
				rect;							// bounds
				cstring;						// name
				cstring;						// label
				cstring;						// Initial value
				cstring;						// allowed chars
				integer;						// max length
				integer;						// label width
			case Caption:
				key longint = 'capt';
				rect;							// bounds
				cstring;						// name
				cstring;						// label
			case PopupMenu:
				key longint = 'popu';
				rect;							// bounds
				cstring;						// name
				cstring;						// label
				integer;						// Menu ID
				integer;						// divider
			case List:
				key longint = 'list';
				rect;							// bounds
				cstring;						// name
			case OutlineList:
				key longint = 'olst';
				rect;							// bounds
				cstring;						// name
			case ColorControl:
				key longint = 'clct';
				rect;							// bounds
				cstring;						// name
				cstring;						// label
			case ColorSquare:
				key longint = 'csqr';
				rect;							// bounds
				cstring;						// name
			case ColorSlider:
				key longint = 'csld';
				rect;							// bounds
				cstring;						// name
			case ColorDemo:
				key longint = 'cdmo';
				rect;							// bounds
				cstring;						// name
			case ListBox:
				key longint = 'lbox';
				rect;							// bounds
				cstring;						// name
			case GrepListBox:
				key longint = 'gbox';
				rect;							// bounds
				cstring;						// name
			case StringListBox:
				key longint = 'slbx';
				rect;							// bounds
				cstring;						// name
			case PathBox:
				key longint = 'pbox';
				rect;							// bounds
				cstring;						// name
			case StdErrBox:
				key longint = 'ebox';
				rect;							// bounds
				cstring;						// name
			case KeyCapture:
				key longint = 'keyc';
				rect;							// bounds
				cstring;						// name
			case Line:
				key longint = 'line';
				rect;							// bounds
			case TabbedBook:
				key longint = 'tabb';
				rect;							// bounds
				cstring;						// name
			case TabbedBookEnd:
				key longint = 'tabe';
			case TabSheet:
				key longint = 'shet';
				cstring;						// name
				cstring;						// description
			case TabSheetEnd:
				key longint = 'shte';
			case Box:
				key longint = 'box ';
				rect;							// bounds
				cstring;						// name
			case Line:
				key longint = 'line';
				rect;							// bounds van de lijn
			case Slider:
				key longint = 'sldr';
				rect;							// bounds
				cstring;						// name
				cstring;						// label
				longint;						// msg
				longint;						// min
				longint;						// max
				longint block = 0, triangle = 1;						// thumb
			case BoxEnd:
				key longint = 'boxe';
		};
	};
};

Type 'MICN' {
	hexstring;
};

Type 'CURS' {
	byte;			// size
	byte;			// depth
	array {
		byte;
	};
	array {
		byte;			// image
	};
	array {
		byte;			// mask
	}
};

