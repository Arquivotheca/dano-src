

struct XString {
		XString( );
		~XString( );
	bool	putm( uchar *, uint),
		putb( uint);
	uint	count( ),
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
