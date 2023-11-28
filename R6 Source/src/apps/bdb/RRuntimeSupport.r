Type 'RTLi' {
	longint = $$CountOf (list);
	array list {
		pstring;					// procedure name
		longint start = 0;		// offset in procedure
		longint					// action 
			rtStepOutStepIn = 1,
			rtThrow = 2,
			rtStepOut = 3;
		longint;		// extra info
	}
};

Resource 'RTLi' (0, "runtime support list")
{
	{
		"__builtin_new", start, rtStepOutStepIn, 0,
		"__builtin_delete", start, rtStepOutStepIn, 0,
		"__throw", start, rtThrow, 0,
		"load_add_on", start, rtStepOut, 0
	}
};
