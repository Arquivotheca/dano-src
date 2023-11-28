

struct BShell {
	struct V {
		int	ptyfd;
		pid_t	pgid;
		virtual void	dsend( BMessage *)		= 0,
				rsend( BMessage *)		= 0,
				trsend( BMessage *)		= 0,
				twsend( BMessage *)		= 0;
	};
	struct Data: BHandler, RNode, virtual V {
			Data( RNode);
		void	dinit( ),
			dsend( BMessage *),
			MessageReceived( BMessage *);
	};
	struct Req: BHandler, RNode, virtual V {
			Req( RNode);
		void	rinit( ),
			rsend( BMessage *),
			MessageReceived( BMessage *);
	};
	struct Ctl: BHandler, RNode, virtual V {
			Ctl( RNode);
		void	cinit( ),
			MessageReceived( BMessage *);
	};
	struct TR: BHandler, RNode, virtual V {
		void	trinit( ),
			trsend( BMessage *),
			MessageReceived( BMessage *);
	};
	struct TW: BHandler, RNode, virtual V {
		void	twinit( ),
			twsend( BMessage *),
			MessageReceived( BMessage *);
	};
	struct ttyReader: BLooper, RNode {
			ttyReader( int, RNode);
		void	MessageReceived( BMessage *);
		int	ptyfd;
	};
	struct ttyWriter: BLooper, RNode {
			ttyWriter( int, RNode);
		void	MessageReceived( BMessage *);
		int	ptyfd;
	};
	struct Obj: Data, Req, Ctl, TR, TW, virtual V {
			Obj( RNode, RNode, RNode);
		void	report( char *);
	};
};
