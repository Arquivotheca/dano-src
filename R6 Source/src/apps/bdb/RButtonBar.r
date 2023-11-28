type 'BtnB' {
	longint
		dragger = 1,
		acceptFirstClick = 2;	// flags
	longint = $$CountOf(buttons);
	array buttons {
		longint
			new = -1,
			open = -2;			// resID for icon
		longint;					// cmd
		longint
			menu = 1,
			toggle = 2,
			space = 4,
			dual = 8;				// flags
		cstring;					// help string
	};
};

