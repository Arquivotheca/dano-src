#ifndef _DOC_FRAMEWORK_H_
#define _DOC_FRAMEWORK_H_

#include <View.h>

enum {
	SET_SCALE		=	'scal',
	SCALE_TO_FIT	=	'sclf',
	GO_TO_PAGE		=	'gtpg',
	GO_TO_NEXT		=	'gnex',
	GO_TO_PREV		=	'gpre',
	GO_TO_FIRST		=	'gfir',
	GO_TO_LAST		=	'glas'
};

// scale flags
enum {
	SCALE_TO_WIDTH	=	1,
	SCALE_TO_HEIGHT	=	2
};

class DocView;
class ToolView;
class BScrollBar;

class DocFramework : public BView
{
	public:
									DocFramework(BRect frame, const char *name, ToolView *toolbar, DocView *doc = NULL);
		virtual						~DocFramework();
	
		status_t					SetDocument(DocView *doc, bool deleteOld = true);
		DocView *					Document() const;
	
		ToolView *					Toolbar() const;
	
		// notifications for doc view changes
		void						DocSizeChanged(float fScale);
		void						DocPageChanged(uint32 pageNum);
		virtual	void				GetPreferredSize(float *width, float *height);
		virtual void				ResizeToPreferred();
		virtual	void				FrameResized(float new_width, float new_height);
		void						Draw(BRect updateRect);
		void						MessageReceived(BMessage *msg);

	private:
		void						AdjustScrollBars(void);
		DocView *					fDocument;
		ToolView *					fToolbar;
		BScrollBar *				fVertScroll;
		BScrollBar *				fHorzScroll;
};

class DocView : public BView
{
	public:
									DocView(BRect frame, const char *name);
		virtual						~DocView();
	
		virtual const char *		Title();
		
		status_t					SetFramework(DocFramework *framework);
		DocFramework *				Framework() const;		
	
		virtual status_t			ScaleToFit(BRect frame, uint32 scaleFlags);
		virtual status_t			SetScale(float x, float y);
		virtual status_t			GetScale(float *x, float *y) const;

		virtual	void				GetPreferredSize(float *width, float *height);
		virtual void				ResizeToPreferred();	
		
		// page changes
		uint32						PageCount() const;
		uint32						CurrentPage() const;
		virtual uint32				GoToPage(uint32 pageNum);
		uint32						GoToNextPage();
		uint32						GoToPreviousPage();
		uint32						GoToFirstPage();
		uint32						GoToLastPage();
		virtual void				MessageReceived(BMessage *msg);
		virtual bool				IsPrinting();
		virtual void				Print();
	
	protected:
		uint32						fPageCount;
		uint32						fCurrentPage;
	private:
		DocFramework *				fFramework;
};


#endif
