#ifndef _VIEW_H
#include <View.h>
#endif

class TWaveView : public BView {
public:
 	 				TWaveView (BPoint where, BView *parent);
  					~TWaveView();
virtual  	void	Draw(BRect r);

private:
};
