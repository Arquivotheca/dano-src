#include <assert.h>
#include <Debug.h>
#include <stdio.h>
#include <View.h>
#include <Window.h>
#include "ViewTransaction.h"

BWindow *ViewTransaction::_dummyWindow = ((BWindow*)0xFFFFFFFF);

void ViewTransaction::Clear(BView *dfltParent)
{
	m_defaultParent = dfltParent;
#if DEBUG
	if (m_ops.CountItems())
		printf("%08x ViewTransaction::Clear %08x with %d commands pending\n",
			this,dfltParent,m_ops.CountItems());
#endif

	m_ops.SetItems(0);
}

ViewTransaction::~ViewTransaction()
{
	if (m_ops.CountItems()) {
		PRINT(("%08x ViewTransaction::~ViewTransaction with %d commands pending\n",this,m_ops.CountItems()));
		TRESPASS();
	};
}

void ViewTransaction::Other(BView *view, ViewOpFunc func, void *userData)
{
	if (m_window == _dummyWindow) m_window = view->Window();

	ViewOperation op;
	op.view = view;
	op.op = ViewOperation::other;
	op.arg1 = (int32)func;
	op.arg2 = (int32)userData;
	m_ops.AddItem(op);
}

void ViewTransaction::AddChild(BView *view, BView *viewToAdd)
{
	if (!view) view = m_defaultParent;
	if (m_window == _dummyWindow) m_window = view->Window();

	ViewOperation op;
	op.view = view;
	op.arg1 = (int32)viewToAdd;
	op.op = ViewOperation::addTo;
	m_ops.AddItem(op);
};

void ViewTransaction::Remove(BView *view)
{
	if (m_window == _dummyWindow) m_window = view->Window();
	if (m_window == NULL) m_window = view->Window();

	ViewOperation op;
	op.view = view;
	op.op = ViewOperation::remove;
	m_ops.AddItem(op);
};

void ViewTransaction::Delete(BView *view)
{
	ViewOperation op;
	op.view = view;
	op.op = ViewOperation::doDelete;
	m_ops.AddItem(op);
};

void ViewTransaction::Move(BView *view, int32 x, int32 y)
{
	if (m_window == _dummyWindow) m_window = view->Window();
	if (m_window == NULL) m_window = view->Window();

	ViewOperation op;
	op.view = view;
	op.op = ViewOperation::move;
	op.arg1 = x;
	op.arg2 = y;
	m_ops.AddItem(op);
}

void ViewTransaction::Resize(BView *view, int32 width, int32 height)
{
	if (m_window == _dummyWindow) m_window = view->Window();
	if (m_window == NULL) m_window = view->Window();

	ViewOperation op;
	op.view = view;
	op.op = ViewOperation::resize;
	op.arg1 = width;
	op.arg2 = height;
	m_ops.AddItem(op);
}

void ViewTransaction::Commit()
{
	if (!m_ops.CountItems()) return;

	if (m_window && (m_window != _dummyWindow)) {
		m_window->Lock();
		m_window->BeginViewTransaction();
	};

	BView *parent = NULL;
	BRect bounds(0,0,0,0);
	for (int32 i=0;i<m_ops.CountItems();i++) {
		ViewOperation &op = m_ops[i];
		if (op.op == ViewOperation::move) {
			if (op.view->Parent() && (op.view->Parent() != parent)) {
				bounds = op.view->Parent()->Bounds();
				parent = op.view->Parent();
			};
			op.view->MoveTo(op.arg1-bounds.left,op.arg2-bounds.top);
		} else if (op.op == ViewOperation::other) {
			ViewOpFunc func = (ViewOpFunc)op.arg1;
			void *userData = (void*)op.arg2;
			func(op.view,userData);
		} else if (op.op == ViewOperation::resize) {
			op.view->ResizeTo(op.arg1,op.arg2);
		} else if (op.op == ViewOperation::remove) {
			if (op.view->Parent()) {
				op.view->RemoveSelf();
			};
		} else if (op.op == ViewOperation::doDelete) {
			delete op.view;
		} else if (op.op == ViewOperation::addTo) {
			if ((((BView*)op.arg1)->Window() == m_window) && (((BView*)op.arg1)->Parent() == op.view))
				PRINT(("already there!\n"));
			else
				op.view->AddChild((BView*)op.arg1);
		};
	};

	m_ops.SetItems(0);

	if (m_window && (m_window != _dummyWindow)) {
		m_window->EndViewTransaction();
		m_window->Unlock();
	};
}
