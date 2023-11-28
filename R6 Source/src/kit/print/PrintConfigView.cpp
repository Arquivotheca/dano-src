// ***********************************************************************
// libbe.so
//
// (c) 2000, Be Incorporated
//
// Written by : Mathias Agopian
//
// ***********************************************************************

#include <stdio.h>
#include <View.h>
#include <Message.h>
#include <print/PrintConfigView.h>

#include "PrintWindows.h"

#define m	_m_rprivate

namespace BPrivate
{
	struct _configview_private
	{
		// Just here for future extension
		uint32 dummy;
	};
} using namespace BPrivate;


BPrintConfigView::BPrintConfigView(	BRect frame,
									const char *name,
									uint32 resizeMask,
									uint32 flags)
	: 	BView(frame, name, resizeMask, flags),
		fPrivate(new _configview_private),
		_m_rprivate(*fPrivate)
{
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
}


BPrintConfigView::BPrintConfigView(BMessage *data)
	:	BView(data),
		fPrivate(new _configview_private),
		m(*fPrivate)
{
}

BPrintConfigView::~BPrintConfigView()
{
	delete fPrivate;
}


BArchivable *BPrintConfigView::Instantiate(BMessage *data)
{
	return new BPrintConfigView(data);
}

status_t BPrintConfigView::Archive(BMessage *data, bool deep) const
{
	status_t result;
	if ((result = BView::Archive(data, deep)) != B_OK)
		return result;
	return B_OK;
}


status_t BPrintConfigView::Save()
{
	return B_OK;
}

status_t BPrintConfigView::SetSaveEnabled(bool enable)
{
	if (LockLooper())
	{
		Window()->PostMessage(enable ? (ControlView::ENABLE_OK) : (ControlView::DISABLE_OK));
		UnlockLooper();
		return B_OK;
	}
	return B_ERROR;
}


void BPrintConfigView::GetMinimumSize(float *width, float *height) {
	const BRect b = Bounds();
	*width = b.Width();
	*height = b.Height();
}

// --------------------------------------------------
// methods overides for future compatibility
// --------------------------------------------------
#pragma mark -

void BPrintConfigView::AttachedToWindow() {
	BView::AttachedToWindow();
}

void BPrintConfigView::AllAttached() {
	BView::AllAttached();
}

void BPrintConfigView::DetachedFromWindow() {
	BView::DetachedFromWindow();
}

void BPrintConfigView::AllDetached() {
	BView::AllDetached();
}

void BPrintConfigView::MessageReceived(BMessage *msg) {
	BView::MessageReceived(msg);
}

void BPrintConfigView::Draw(BRect updateRect) {
	BView::Draw(updateRect);
}

void BPrintConfigView::MouseDown(BPoint where) {
	BView::MouseDown(where);
}

void BPrintConfigView::MouseUp(BPoint where) {
	BView::MouseUp(where);
}

void BPrintConfigView::MouseMoved(BPoint where, uint32 code, const BMessage *a_message) {
	BView::MouseMoved(where, code, a_message);
}

void BPrintConfigView::WindowActivated(bool state) {
	BView::WindowActivated(state);
}

void BPrintConfigView::KeyDown(const char *bytes, int32 numBytes) {
	BView::KeyDown(bytes, numBytes);
}

void BPrintConfigView::KeyUp(const char *bytes, int32 numBytes) {
	BView::KeyUp(bytes, numBytes);
}

void BPrintConfigView::Pulse() {
	BView::Pulse();
}

void BPrintConfigView::FrameMoved(BPoint new_position) {
	BView::FrameMoved(new_position);
}

void BPrintConfigView::FrameResized(float new_width, float new_height) {
	BView::FrameResized(new_width, new_height);
}

void BPrintConfigView::TargetedByScrollView(BScrollView *scroll_view) {
	BView::TargetedByScrollView(scroll_view);
}

void BPrintConfigView::SetFlags(uint32 flags) {
	BView::SetFlags(flags);
}

void BPrintConfigView::SetResizingMode(uint32 mode) {
	BView::SetResizingMode(mode);
}

void BPrintConfigView::MakeFocus(bool focusState) {
	BView::MakeFocus(focusState);
}

void BPrintConfigView::Show() {
	BView::Show();
}

void BPrintConfigView::Hide() {
	BView::Hide();
}

void BPrintConfigView::GetPreferredSize(float *width, float *height) {
	BView::GetPreferredSize(width, height);
}

void BPrintConfigView::ResizeToPreferred() {
	BView::ResizeToPreferred();
}

BHandler *BPrintConfigView::ResolveSpecifier(BMessage *msg, int32 index, BMessage *specifier, int32 form, const char *property) {
	return BView::ResolveSpecifier(msg, index, specifier, form, property);
}

status_t BPrintConfigView::GetSupportedSuites(BMessage *data) {
	return BView::GetSupportedSuites(data);
}

status_t BPrintConfigView::Perform(perform_code d, void *arg) {
	return BView::Perform(d, arg);
}

void BPrintConfigView::DrawAfterChildren(BRect r) {
	BView::DrawAfterChildren(r);
}

// And the FBC
status_t BPrintConfigView::_Reserved_BPrintConfigView_0(int32 arg, ...) { return B_ERROR; }
status_t BPrintConfigView::_Reserved_BPrintConfigView_1(int32 arg, ...) { return B_ERROR; }
status_t BPrintConfigView::_Reserved_BPrintConfigView_2(int32 arg, ...) { return B_ERROR; }
status_t BPrintConfigView::_Reserved_BPrintConfigView_3(int32 arg, ...) { return B_ERROR; }
status_t BPrintConfigView::_Reserved_BPrintConfigView_4(int32 arg, ...) { return B_ERROR; }
status_t BPrintConfigView::_Reserved_BPrintConfigView_5(int32 arg, ...) { return B_ERROR; }
status_t BPrintConfigView::_Reserved_BPrintConfigView_6(int32 arg, ...) { return B_ERROR; }
status_t BPrintConfigView::_Reserved_BPrintConfigView_7(int32 arg, ...) { return B_ERROR; }

