#ifndef _VIEW_H
#include <View.h>
#endif

class TTitleView : public BView {
public:
 	 				TTitleView (BPoint where, BView *parent);
  					~TTitleView();
virtual  	void	Draw(BRect r);

private:
};
