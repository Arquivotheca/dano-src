
const BRect kWindowFrame (50,50,450,450);

class MainBView: public BView
{

	public:
		MainBView();
		~MainBView();
		virtual void Draw(BRect updateRect);

};