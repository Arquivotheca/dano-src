

struct RNode {
		RNode( );
		RNode( RNode &z);
		~RNode( );
	RNode	operator=( const RNode &);
	bool	SetDestination( RNode);
	void	Establish( BHandler *, BLooper * = 0, char * = 0),
		Establish( BHandler *, char *),
		operator>>( BMessage *),
		operator>>( int),
		Shutdown( ),
		Dump( );
	static	int DumpTable( );
	private:
	uint	id;
};
