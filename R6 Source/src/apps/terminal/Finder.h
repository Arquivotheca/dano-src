

struct Finder {
	enum status {
		SEARCHING,
		FOUND,
		FAILED,
		BADSYNTAX,
		NOMEM
	};
		Finder( ushort *, bool, bool, uint);
		~Finder( );
	static XString	getfindstr( XString, bool, bool, bool);
	bool	search( ushort *, uint);
	status	status;
	uint	col,
		len;
	private:
	ushort	*foldcase( ushort *);
	regprog	*pattern;
	bool	forward,
		fold;
	uint	line;
	XString	x;
};
