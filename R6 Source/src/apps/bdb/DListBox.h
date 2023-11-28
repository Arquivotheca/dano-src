/*	$Id: DListBox.h,v 1.6 1998/11/19 21:50:17 maarten Exp $
	
	Copyright Hekkelman Programmatuur
	Maarten L. Hekkelman
*/

#ifndef DLISTBOX_H
#define DLISTBOX_H

#include <ListItem.h>

#include <vector>

class DListItem;
class DListBox;
class DListView;
class DScroller;
class DEditBox;

class DListItem : public BStringItem
{
	friend class DListBox;
  public:
	DListItem(const char *txt);

	virtual void DrawItem(BView *owner, BRect bounds, bool complete, int column);
	
	virtual const char* GetColumnText(int column);
	virtual void SetColumnText(int column, const char *newText);
	
	virtual void Expanded(BOutlineListView *list);
	virtual void Collapsed(BOutlineListView *list);
	
	virtual bool ClickItem(BView *owner, BRect frame, BPoint where);
	
	bool operator< (const DListItem& item) const
		{ return strcasecmp(Text(), item.Text()) < 0; }

  private:
  	virtual void DrawItem(BView *owner, BRect bounds, bool complete);
	DListBox *fBox;
};

struct DColumn
{
	char *fTitle;
	float fWidth;
	bool fEditable;
};

class DListBox : public BView
{
	friend class DListItem;
	friend class DListView;
  public:
	DListBox(BRect frame, const char *name);
	
	void AddColumn(const char *title, float width);
	
	BOutlineListView *List() const;

	void Changed();
	
	virtual void Draw(BRect update);
	virtual void FrameResized(float newWidth, float newHeight);
	
	virtual void MouseDown(BPoint where);
	
	virtual void SetDeleteMessage(long msg);
	virtual void SetToggleMessage(long msg);

  protected:
  	
  	int ColumnDividerHitBy(BPoint where);
  
 	std::vector<DColumn> fColumns;
	DListView *fList;
	DEditBox *fEdit;
};

#endif
