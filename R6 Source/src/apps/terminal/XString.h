

struct XString {
		XString( );
		XString( const XString &);
		~XString( );
	XString	&operator=( const XString &);
	bool	putm( uchar *, uint),
		putw( ushort),
		putb( uint);
	uint	count( ),
		getw( ),
		getb( ),
		unputb( );
	uchar	*getm( uint);
	void	ungetm( uint),
		clear( );
	private:
	uchar	*buffer;
	uint	bufsize,
		head,
		tail;
};
