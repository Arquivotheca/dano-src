
#ifndef _VIEW_TRANSACTION_H
#define _VIEW_TRANSACTION_H

#include <SupportDefs.h>
#include "BArray.h"

class BView;

struct ViewOperation {
	enum view_op {
		nothing=0,
		move,
		resize,
		remove,
		addTo,
		doDelete,
		other
	};

	BView *view;
	view_op op;
	int32 arg1,arg2;
};

typedef void (*ViewOpFunc)(BView *, void *);

class ViewTransaction
{
	private:
		BWindow *				m_window;
		BView *					m_defaultParent;
		BArray<ViewOperation>	m_ops;

		static BWindow *		_dummyWindow;
		
	public:

		void					Clear(BView *dfltParent=NULL);
		void					Commit();

								ViewTransaction(BView *dfltParent=NULL)
									: m_window(_dummyWindow), m_ops(10) { Clear(dfltParent); };
								~ViewTransaction();
								
		void					Move(BView *view, int32 x, int32 y);
		void					Resize(BView *view, int32 width, int32 height);
		void					AddChild(BView *view, BView *viewToAdd);
		void					Remove(BView *view);
		void					Delete(BView *view);
		void					Other(BView *view, ViewOpFunc func, void *userData);

};

#endif
