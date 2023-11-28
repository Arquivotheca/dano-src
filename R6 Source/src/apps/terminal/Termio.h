

struct BTermio {
	struct ReqIn: RNodeHandler {
			ReqIn( BTermio *, RNode);
		private:
		BTermio	*termio;
		void	MessageReceived( BMessage *);
	};
	struct ReqOut: RNodeHandler {
			ReqOut( BTermio *, RNode);
		private:
		BTermio	*termio;
		void	MessageReceived( BMessage *);
	};
	struct DataIn: RNodeHandler {
			DataIn( BTermio *, RNode);
		private:
		BTermio	*termio;
		void	MessageReceived( BMessage *),
			process( );
		bool	full,
			stopped;
		XString	x;
		friend	class ReqOut;
	};
	struct DataOut: RNodeHandler {
			DataOut( BTermio *, RNode);
		private:
		BTermio	*termio;
		void	MessageReceived( BMessage *),
			process( );
		bool	full,
			stopped;
		XString	x;
		friend	class DataIn;
		friend	class ReqIn;
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
		BTermio	*termio;
		friend	class DataIn;
		friend	class DataOut;
	};
		BTermio( RNode, RNode, RNode, RNode, RNode, RNode);
	private:
	DataIn	din;
	DataOut	dout;
	ReqIn	rin;
	ReqOut	rout;
	CtlIn	cin;
	CtlOut	cout;
	friend	class DataIn;
	friend	class DataOut;
	friend	class ReqIn;
	friend	class ReqOut;
	friend	class CtlOut;
};
