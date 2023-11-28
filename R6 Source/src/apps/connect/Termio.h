

struct BTermio {
	struct ReqIn: RNodeHandler {
			ReqIn( BTermio *, RNode);
		private:
		BTermio	*trmio;
		void	MessageReceived( BMessage *);
	};
	struct ReqOut: RNodeHandler {
			ReqOut( BTermio *, RNode);
		private:
		BTermio	*trmio;
		void	MessageReceived( BMessage *);
	};
	struct DataIn: RNodeHandler {
			DataIn( BTermio *, RNode);
		private:
		BTermio	*trmio;
		void	MessageReceived( BMessage *),
			process( ),
			send( uint),
			echo( uint);
		bool	full,
			stopped;
		XString	x;
		friend struct BTermio::ReqOut;
	};
	struct DataOut: RNodeHandler {
			DataOut( BTermio *, RNode);
		private:
		BTermio	*trmio;
		void	MessageReceived( BMessage *),
			store( uint),
			process( );
		bool	full,
			stopped;
		XString	x;
		friend	struct BTermio::DataIn;
		friend	struct BTermio::ReqIn;
	};
	struct CtlIn: RNodeHandler {
			CtlIn( RNode);
		private:
		void	MessageReceived( BMessage *);
	};
	struct CtlOut: RNodeHandler {
			CtlOut( BTermio *, RNode);
		private:
		void	MessageReceived( BMessage *);
		termio	t;
		BTermio	*trmio;
		friend	struct BTermio::DataIn;
		friend	struct BTermio::DataOut;
	};
		BTermio( RNode, RNode, RNode, RNode, RNode, RNode);
	private:
	DataIn	din;
	DataOut	dout;
	ReqIn	rin;
	ReqOut	rout;
	CtlIn	cin;
	CtlOut	cout;
	friend	struct BTermio::DataIn;
	friend	struct BTermio::DataOut;
	friend	struct BTermio::ReqIn;
	friend	struct BTermio::ReqOut;
	friend	struct BTermio::CtlOut;
};
