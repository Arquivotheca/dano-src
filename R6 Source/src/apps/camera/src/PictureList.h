/*
	PictureList.h
	A BView-derived class that displays a list of pictures.
	Shows thumbnails with picture names. Handles selection,
	scrolling, and drag & drop of the images to the Tracker.
*/

#ifndef PICTURELIST_H
#define PICTURELIST_H

#include <View.h>
#include <List.h>
#include "CameraStrings.h"

#define TWIDTH			64
#define THEIGHT			64
#define ITEM_WIDTH		(TWIDTH + 16)
#define ITEM_HEIGHT		(THEIGHT + 24)
#define BUFFER_WIDTH	8

class PictureList : public BView {
public:
	PictureList(BRect r, const char *name);
	~PictureList();

	void Empty();
	void CreateNewList(int32 length);
	void AddFileName(char *name);
	int32 CountPictures();
	int32 CountSelection();
	bool IsSelected(int32 item);
	void Select(int32 item);
	bool HasThumbnail(int32 item);
	char *PictureName(int32 item);
	void DeletePicture(int32 item);
	void SetThumbnail(int32 item, BBitmap *thumbnail);

	void Draw(BRect updateRect);
	void FrameResized(float width, float height);
	void MouseDown(BPoint point);
	void MouseMoved(BPoint point, uint32 transit, const BMessage *message);
	void MouseUp(BPoint point);
private:
	void FixupScrollbar();
	int32 Pick(BPoint point);
	BRect PictureBounds(int32 id);

	BList	fNameList, fImgList;
	bool	fExistingFilenames;
	bool	*fSelected;
	bool	*fHasThumbnail;
	bool	fDragging, fDragSel;
	BPoint	fDragStartPt;
	bool	*fPreDragSel;
};

#endif
