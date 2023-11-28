#ifndef BASIC_KEYBOARD_H
#define BASIC_KEYBOARD_H

#include <Bitmap.h>
#include <Control.h>
#include <View.h>
#include <Window.h>

// ******************************************************************************************

class TBasicKeyboard;
class TKey : public BInvoker {
public:
							TKey(BRect frame, const char* label, BMessage *message,
								char** strings, int32 rawChar, int32 key,
								bool isModifierKey, bool useLabel,
								BView* parent, TBasicKeyboard* realParent);
							~TKey();
							
			void			Draw();
			
			status_t		Invoke(BMessage* message=NULL);
			
			const char*		GetLabel();
			
			BRect			Bounds() { return fFrame; }
			BRect			Frame() { return fFrame; }
			char*			Label() { return fLabel; }
			int32			Value() { return fValue; }
			void			SetValue(int32 v) { fValue = v; }
			
			bool			IsModifierKey() { return fIsModifierKey; }
			int32			ModifierKey() { return fRawChar; }

private:
			int32			fRawChar;
			int32			fKey;
			int32			fButtonState;
			BPoint			fTextLoc;
			BRect			fTextFrame;
			rgb_color		fHiliteColor;
			bigtime_t		fKeyDelay;
			bigtime_t		fFirstClickTime;
			bool			fIsModifierKey;
			char*			fStrings[9];
			bool			fUseLabel;
			
			TBasicKeyboard*	fRealParent;
			BView*			fParent;
			BRect			fFrame;
			char*			fLabel;
			int32			fValue;
};

// ******************************************************************************************

class TBasicKeyboard : public BView {
public:
							TBasicKeyboard(BPoint loc);
							~TBasicKeyboard();
							
			void			GetPreferredSize(float *width, float *height);
			void			ResizeToPreferred();
			
			void			AttachedToWindow();
			void			DetachedFromWindow();

			void			MessageReceived(BMessage*);
			
			void			MouseDown(BPoint);
			void			MouseUp(BPoint);
			void			MouseMoved(	BPoint where, uint32 code,
								const BMessage *message);

			void			Draw(BRect);
			
			void			ResetModifiers(bool state);				
			int32			Modifiers() const;
			
			bool			IsTracking() { return fTracking; }
			void			SetTracking(bool t) { fTracking = t; }
			
private:
			void			AddParts();
			
			bool			SendEvent();
			
			TKey*			KeyAtPoint(BPoint pt);

			const int32*	GetKeyMapping(int32 row);
			char*			GetKeyInfo(int32 key, int32 modifier);
			void			GetKeyStrings(int32 key, char** keychars);
			bool			GetKeyInfo(int32 row, int32 column,
								float *size, int32 *rawChar,
								const char** label, bool *useLabel, bool *isModifier);
			
			void			SetKey(TKey* key, bool state);
			
			bool			fTracking;
			
			key_map* 		fKeyList;
			char* 			fCharList;
			int32			fModifiers;
			
			BList*			fButtonList;

			BView*			fOffscreenView;
			BBitmap*		fOffscreenBits;

			TKey*			fCurrentKey;
};

inline int32
TBasicKeyboard::Modifiers() const
{
	return fModifiers;
}

// ******************************************************************************************

#ifdef INPUT_METHOD
class TSoftKeyboardLooper;
#endif
class TBasicKeyboardWindow : public BWindow {
public:
#ifdef INPUT_METHOD
							TBasicKeyboardWindow(TSoftKeyboardLooper *owner, uint32 mode);
#else
							TBasicKeyboardWindow();
#endif
#if _SUPPORTS_SOFT_KEYBOARD
			bool			QuitRequested();
#endif			

	virtual void			MessageReceived(BMessage *message);
	
			void			MakeActive(bool active, bool customLoc, BRect relativeFrame);
			bool			IsActive() { return fIsActive; }
	
private:
	void					AddParts();

#ifdef INPUT_METHOD
	TSoftKeyboardLooper*	fOwnerLooper;
	uint32					fMode;
#endif	
	bool					fIsActive;
	TBasicKeyboard*			fBasicKeyboard;
};

#endif