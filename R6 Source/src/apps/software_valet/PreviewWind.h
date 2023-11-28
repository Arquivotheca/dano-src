#ifndef _PREVIEWWIND_H_
#define _PREVIEWWIND_H_

#include <Window.h>
#include <View.h>
#include <Rect.h>
#include <Message.h>

//PreviewWind.h
class FileTree;

class PreviewWind : public BWindow
{
public:
	PreviewWind(FileTree *prevTree, bool *cancelInst);
	
	void			Go();
	virtual bool	QuitRequested();
	
	bool			*cancel;
};

class PreviewView : public BView
{
public:
	PreviewView(BRect frame, FileTree *tree);
	
	virtual void	AttachedToWindow();
	virtual void	MessageReceived(BMessage *);
	virtual void	Draw(BRect);
private:
	FileTree	*treeView;
};

#endif

