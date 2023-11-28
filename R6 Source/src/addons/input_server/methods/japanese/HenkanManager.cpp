#include <SupportDefs.h>
#include <string.h>
#include <malloc.h>
#include <File.h>
#include <Entry.h>
#include <ByteOrder.h>

#include "HenkanManager.h"
#include "KouhoWindow.h"
#include "KanaKan.h"
#include "JapaneseCommon.h"

int32 HenkanManager::sHenkanWindowThreshold = J_DEFAULT_HENKAN_WINDOW_THRESHOLD;


HenkanManager::HenkanManager()
{
	fKanaKan = NULL;
	fHenkanCount = 0;
	fKouhoWindow = NULL;
	fKouhoView = NULL;
}


HenkanManager::~HenkanManager()
{
	delete (fKanaKan);
	StopKouhoWindow();
}


void
HenkanManager::Append(
	const char*	theKey,
	uint32 keyLen,
	uint32	modifiers)
{
	if ((theKey[0] != B_SPACE) && (theKey[0] != B_TAB))
		fHenkanCount = 0;
	
	switch (theKey[0]) {
		case B_ESCAPE:
			DoRevert();
			break;

		case B_BACKSPACE:
		case B_DELETE:
			DoBackspaceDelete(theKey[0] == B_BACKSPACE);
			break;

		case B_LEFT_ARROW:
			if (fKanaKan == NULL)
				DoMoveInsertionPoint(true);
			else {
				if (modifiers & B_SHIFT_KEY)
					DoShrinkClause();
				else
					DoPrevClause();
			}
			break;

		case B_RIGHT_ARROW:
			if (fKanaKan == NULL)
				DoMoveInsertionPoint(false);
			else {
				if (modifiers & B_SHIFT_KEY)
					DoGrowClause();
				else
					DoNextClause();
			}
			break;

		case B_UP_ARROW:
			DoHenkan(!IsBottomUpKouhoList());
			StartKouhoWindow();
			break;

		case B_DOWN_ARROW:
			DoHenkan(IsBottomUpKouhoList());
			StartKouhoWindow();
			break;

		case B_ENTER:
			DoKakutei(true);
			break;

		case B_SPACE:
		case B_TAB:
			DoHenkan(modifiers & B_SHIFT_KEY);
			IncrementHenkanCount();
			break;

		default:
			DoKey(theKey, keyLen);
			break;
	}
}


void
HenkanManager::OpenInput()
{
}


void
HenkanManager::CloseInput()
{
}


bool
HenkanManager::ClauseLocation(
	BPoint	*where,
	float	*height)
{
	*where = B_ORIGIN;
	*height = 0.0;
	return (true);
}


void
HenkanManager::Kakutei()
{
}


void
HenkanManager::Update()
{
}


const KanaKan*
HenkanManager::PeekKanaKan() const
{
	return (fKanaKan);
}


void
HenkanManager::DoBackspaceDelete(
	bool	backspace)
{
	if (fKanaKan == NULL) {
		if (fKanaString.Length() < 1)
			return;

		if (backspace)
			fKanaString.Backspace();
		else
			fKanaString.Delete();
		Update();

		if (fKanaString.Length() < 1)
			CloseInput();
	}
	else {
		StopKouhoWindow();

		delete (fKanaKan);
		fKanaKan = NULL;
		Update();
	}
}


void
HenkanManager::DoHenkan(
	bool	prev)
{
	if (fKanaString.Length() < 1) {
		OpenInput();
		fKanaString.Append(" ", 1);
		DoKakutei(true);
		return;
	}

	if (fKanaKan == NULL) {
		fKanaKan = new KanaKan(fKanaString.String(true));
		Update();
	}
	else {
		if (prev)
			DoPrevKouho();
		else
			DoNextKouho();
	}
}


void
HenkanManager::DoKakutei(
	bool	close)
{
	if (fKanaKan != NULL) {
		if (StopKouhoWindow())
			return;
	}

	Kakutei();

	if (fKanaKan != NULL) {
		fKanaKan->Kakutei();
		delete (fKanaKan);
		fKanaKan = NULL;
	}
	fKanaString.Clear();

	if (close)
		CloseInput();
}


void
HenkanManager::DoMoveInsertionPoint(
	bool	left)
{
	if (fKanaKan != NULL)
		return;

	int32 oldInsertionPoint = fKanaString.InsertionPoint();
	if (fKanaString.MoveInsertionPoint(left) != oldInsertionPoint)
		Update();
}


void
HenkanManager::DoPrevClause()
{
	if (fKanaKan == NULL)
		return;

	StopKouhoWindow();

	int32 oldClause = fKanaKan->ActiveClause();
	if (fKanaKan->PrevClause() != oldClause)
		Update();
}


void
HenkanManager::DoNextClause()
{
	if (fKanaKan == NULL)
		return;

	StopKouhoWindow();

	int32 oldClause = fKanaKan->ActiveClause();
	if (fKanaKan->NextClause() != oldClause)
		Update();
}


void
HenkanManager::DoShrinkClause()
{
	if (fKanaKan == NULL)
		return;

	StopKouhoWindow();

	int32 oldSize = fKanaKan->ClauseWordLength(fKanaKan->ActiveClause());
	if (fKanaKan->ShrinkClause() != oldSize)
		Update();
}


void
HenkanManager::DoGrowClause()
{
	if (fKanaKan == NULL)
		return;
	
	StopKouhoWindow();

	int32 oldSize = fKanaKan->ClauseWordLength(fKanaKan->ActiveClause());
	if (fKanaKan->GrowClause() != oldSize)
		Update();
}


void
HenkanManager::DoPrevKouho()
{
	if (fKanaKan == NULL)
		return;

	int32 oldKouho = fKanaKan->SelectedKouho(fKanaKan->ActiveClause());
	int32 newKouho = fKanaKan->PrevKouho();
	if (newKouho != oldKouho) {
		SelectInKouhoWindow(newKouho, false, false);
		Update();
	}
}


void
HenkanManager::DoNextKouho()
{
	if (fKanaKan == NULL)
		return;

	int32 oldKouho = fKanaKan->SelectedKouho(fKanaKan->ActiveClause());
	int32 newKouho = fKanaKan->NextKouho();
	if (newKouho != oldKouho) {
		SelectInKouhoWindow(newKouho, false, false);
		Update();
	}
}


void
HenkanManager::DoRevert()
{
	if (fKanaKan == NULL) {
		if (fKanaString.Length() < 1)
			return;

		fKanaString.Clear();
		Update();
		CloseInput();
	}	
	else {
		if (StopKouhoWindow())
			return;

		delete (fKanaKan);
		fKanaKan = NULL;
		Update();
	}
}


void
HenkanManager::DoKey(const char* theKey, uint32 keyLen)
{
	if (fKanaKan != NULL) {
		if ((theKey[0] >= '0') && (theKey[0] <= '9')) {
			if (SelectInKouhoWindow((theKey[0] > '0') ? theKey[0] - '1' : 9, true, true)) {
				StopKouhoWindow();
				return;
			}
		}

		StopKouhoWindow();
		DoKakutei(false);
	}

	if (fKanaString.Length() < 1)
		OpenInput();

	fKanaString.Append(theKey, keyLen);
	Update();
}


void
HenkanManager::IncrementHenkanCount()
{
	if (sHenkanWindowThreshold == 0)
		StartKouhoWindow();
	else {
		if (++fHenkanCount > sHenkanWindowThreshold) {
			StartKouhoWindow();
			fHenkanCount = 0;
		}
	}	
}


void
HenkanManager::StartKouhoWindow(
	BPoint	*where,
	float	height)
{
	if ((fKouhoWindow != NULL) || (fKanaKan == NULL)) 
		return;

	BPoint openLoc = B_ORIGIN;
	if (where != NULL) 
		openLoc = *where;
	else {
		if (!ClauseLocation(&openLoc, &height))
			return;
	}
			
	//BMessenger msngr("application/x-vnd.Web");
	//if (msngr.IsValid())
	//	openLoc=BPoint(-1024,0);

	fKouhoWindow = new KouhoWindow(openLoc, height, this, &fKouhoView);
	fKouhoWindow->Show();

	return;
}


bool
HenkanManager::StopKouhoWindow()
{
	if (fKouhoWindow == NULL)
		return (false);

	fKouhoWindow->PostMessage(B_CLOSE_REQUESTED);
	fKouhoWindow = NULL;
	fKouhoView = NULL;

	return (true);
}


bool
HenkanManager::IsBottomUpKouhoList() const
{
	if (fKouhoView != NULL)
		return (fKouhoView->IsBottomUp());

	return (false);
}


bool
HenkanManager::SelectInKouhoWindow(
	int32	kouho, 
	bool	visible,
	bool	select)
{
	if (fKouhoWindow == NULL)
		return (false);

	int32 selected = fKouhoView->SelectKouho(kouho, visible);
	if (select)
		KouhoChanged(selected);

	return (true);
}


void
HenkanManager::KouhoChanged(
	int32	kouho)
{
	if (fKanaKan == NULL) 
		return;

	fKanaKan->SelectKouho(kouho);
	Update();
}
