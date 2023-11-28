

struct BitmapButton: BButton {
			BitmapButton( BPoint, BBitmap *, BBitmap *, BMessage *);
			~BitmapButton( );
	void		Draw( BRect);
	static rgb_color
			gray;
	private:
	BRect		bsize( BBitmap *);
	BBitmap		*ebm,
			*dbm;
};

struct InfoButton: BitmapButton {
	struct BM: BBitmap {
		struct V: BView {
				V( BM *);
			private:
			void	AttachedToWindow( ),
				draw1( ),
				draw2( );
			BM	*bm;
		};
			BM( );
		void	smooth( );
	};
			InfoButton( BPoint, int);
	static BRect	rect;
};

struct ArrowButton: BitmapButton {
	struct BM: BBitmap {
		struct V: BView {
				V( bool, bool);
			private:
			void	AttachedToWindow( );
			bool	flipped,
				enabled;
		};
			BM( bool, bool);
	};
			ArrowButton( BPoint, bool, int);
	static BRect	rect;
};
