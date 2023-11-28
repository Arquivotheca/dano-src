#ifndef _HENKAN_MANAGER
#define _HENKAN_MANAGER

#include <Messenger.h>
#include <SupportDefs.h>
#include "KanaString.h"


class BPoint;
class KanaKan;
class KouhoWindow;
class KouhoView;


class HenkanManager {
public:
						HenkanManager();
	virtual				~HenkanManager();

	void				Append(const char* theKey, uint32 keyLen, uint32 modifiers);

	virtual void		OpenInput();
	virtual void		CloseInput();
	virtual bool		ClauseLocation(BPoint *where, float *height);
	virtual void		Kakutei();
	virtual void		Update();

	const KanaKan*		PeekKanaKan() const;

protected:
	void				DoBackspaceDelete(bool backspace);
	void				DoHenkan(bool prev);
	void				DoKakutei(bool close);
	void				DoMoveInsertionPoint(bool left);
	void				DoPrevClause();
	void				DoNextClause();
	void				DoShrinkClause();
	void				DoGrowClause();
	void				DoPrevKouho();
	void				DoNextKouho();
	void				DoRevert();
	void				DoKey(const char* theKey, uint32 keyLen);

	void				IncrementHenkanCount();

	void				StartKouhoWindow(BPoint *where = NULL, float height = 0.0);
	bool				StopKouhoWindow();
	bool				IsBottomUpKouhoList() const;

	bool				SelectInKouhoWindow(int32 kouho, bool visible, bool select);
	void				KouhoChanged(int32 kouho);

protected:
	friend class KouhoView;
	friend class JapaneseLooper;

	KanaString			fKanaString;
	KanaKan*			fKanaKan;
	int32				fHenkanCount;
	KouhoWindow*		fKouhoWindow;
	KouhoView*			fKouhoView;

public:
	static int32		sHenkanWindowThreshold;
};


#endif