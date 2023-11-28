#ifndef _STEXTFIELD_H
#define _STEXTFIELD_H



class STextField : public BControl
{
public:
			STextField(BRect frame,
						const char *name,
						const char *label, 
						const char *initial_text, 
						BMessage *message,
						ulong resizeMask = B_FOLLOW_LEFT | B_FOLLOW_TOP,
						ulong flags = B_WILL_DRAW | B_NAVIGABLE,
						bool scrolling = FALSE); 
virtual				~STextField();

virtual	void		SetText(const char *text);
		const char	*Text() const;
		long		TextLength() const;
		
virtual	void		SetLabel(const char *text);
		const char	*Label() const;

virtual	void		SetModificationMessage(BMessage *message);
		BMessage	*ModificationMessage() const;

/*********
virtual	void		SetAlignment(alignment label, alignment text);
		void		GetAlignment(alignment *label, alignment *text) const;
*********/		
		// overrides
virtual	void		Draw(BRect updateRect);
virtual	void		AttachedToWindow();
virtual	void		MakeFocus(bool focusState = TRUE);
virtual	void		SetEnabled(bool state);
	const BView		*TextView();

virtual void		Show();
virtual void		Hide();

private:
friend class _TextFieldView_;
	_TextFieldView_		*fTextView;
	const char 			*fLabelText;
	const char 			*fEditText;
	BMessage			*fModMessage;
	bool				fScrolling;
	
	void				DrawFocus(bool state);
};


class _TextFieldView_ : public BTextView
{
public:
		_TextFieldView_(BRect frame,
						const char *name,
						BRect textRect,
						ulong resizeMask,
						ulong flags,
						STextField *_parControl);
						
virtual void		KeyDown(const char *bytes, int32 byteCount);
virtual void		MakeFocus(bool state = TRUE);
virtual void		MouseDown(BPoint where);
virtual void		Draw(BRect up);
virtual void		MouseMoved(BPoint where,
								uint32 code,
								const BMessage *msg);
		void		MakeNaviagable(bool state = TRUE);
		void		SetDirty(bool dirty = TRUE);
		
virtual void		Paste(BClipboard *clip);
virtual void		Cut(BClipboard *clip);

private:
	
	bool				fModified;
	STextField			*parControl;
};

#endif
