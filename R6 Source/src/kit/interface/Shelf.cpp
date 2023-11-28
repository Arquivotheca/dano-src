//******************************************************************************
//
//	File:		Shelf.cpp
//
//	Written By:	Peter Potrebic
//
//	Copyright 1996-98, Be Incorporated
//
//******************************************************************************

#include <string.h>
#include <errno.h>
#include <syslog.h>
#include <Debug.h>
#include <fbc.h>

#include <Application.h>
#include <ClassInfo.h>
#include <Shelf.h>
#include <File.h>
#include <PopUpMenu.h>
#include <Window.h>
#include <MenuItem.h>
#include <Alert.h>
#include <MessageFilter.h>
#include <Dragger.h>
#include <AppDefsPrivate.h>
#include <Beep.h>
#include <Box.h>
#include <PropertyInfo.h>

#ifndef _INTERFACE_MISC_H
#include <interface_misc.h>
#endif
#ifndef _MESSAGE_STRINGS_H
#include <message_strings.h>
#endif
#ifndef _ARCHIVE_DEFS_H
#include <archive_defs.h>
#endif
#ifndef _FBC_H
#include <fbc.h>
#endif

#define INVALID_UID		0
#define FIRST_VALID_UID	1

/*------------------------------------------------------------*/

class _TContainerViewFilter_ : public BMessageFilter {
public:
						_TContainerViewFilter_(BShelf *owner, BView *container);
						~_TContainerViewFilter_();

		filter_result	Filter(BMessage *msg, BHandler **target);

private:
		filter_result	ObjectDropFilter(BMessage *msg, BHandler **target);

		BShelf		*fShelf;
		BView		*fContainerView;
};


class _TReplicantViewFilter_ : public BMessageFilter {
public:
						_TReplicantViewFilter_(BShelf *owner, BView *container);
						~_TReplicantViewFilter_();

		filter_result	Filter(BMessage *msg, BHandler **target);

private:
		BShelf		*fShelf;
		BView		*fReplicantView;
};


class _BZombieReplicantView_ : public BBox {
public:
					_BZombieReplicantView_(BRect r, status_t err);
virtual				~_BZombieReplicantView_();
virtual	void		Draw(BRect update);
virtual void		MessageReceived(BMessage *msg);
virtual void		MouseDown(BPoint where);

		void		SetArchive(BMessage *m);

private:
		status_t	fError;
		BMessage	*fArchive;
		BPoint		fLoc;
		bool		fInit;
};

struct _rep_data_ {
		_rep_data_();
		_rep_data_(BMessage *, BView *, BDragger *, BDragger::relation, uint32,
			image_id);
		~_rep_data_();

		BMessage			*archive;
		BView				*view;
		BDragger			*widget;
		BDragger::relation	relation;
		uint32				uid;
		image_id			addon_id;
		status_t			error;
		_BZombieReplicantView_	*zombie_view;

static	_rep_data_	*find(const BList *, const BView *, bool zombie = false);
static	int32		index_of(const BList *, const BView *, bool zombie = false);
static	_rep_data_	*find(const BList *, const BMessage *);
static	int32		index_of(const BList *, const BMessage *);
static	_rep_data_	*find(const BList *, uint32 uid);
static	int32		index_of(const BList *, uint32 uid);
};

/*------------------------------------------------------------*/
/*------------------------------------------------------------*/
/*------------------------------------------------------------*/

BShelf::BShelf(BView *view, bool allow_drag, const char *name)
	: BHandler(name)
{
	InitData(NULL, NULL, view, allow_drag);
}

/*------------------------------------------------------------*/

BShelf::BShelf(BDataIO *stream, BView *view, bool allow_drag, const char *name)
	: BHandler(name)
{
	InitData(NULL, stream, view, allow_drag);
}

/*------------------------------------------------------------*/

BShelf::BShelf(const entry_ref *ref, BView *view, bool allow_drag,
	const char *name)
	: BHandler(name), fReplicants(5)
{
	InitData(new BEntry(ref), NULL, view, allow_drag);
}

/*------------------------------------------------------------*/

void BShelf::InitData(BEntry *entry, BDataIO *stream, BView *view, bool drags)
{
	status_t	err;

	fContainerView = view;
	fStream = NULL;
	fDirty = true;
	fFilter = NULL;
	fAllowDragging = drags;
	fDisplayZombies = false;
	fAllowZombies = true;
	fTypeEnforced = false;
	fGenCount = FIRST_VALID_UID;

	fEntry = entry;
	if (entry) {
		fStream = new BFile(entry, O_RDONLY);
	} else {
		fStream = stream;
	}

	fFilter = new _TContainerViewFilter_(this, fContainerView);
	fContainerView->AddFilter(fFilter);
//+	PRINT(("class=%s, c=%d\n", class_name(fContainerView),
//+		fContainerView->FilterList()->CountItems()));

	fContainerView->set_shelf(this);

	if (fStream) {
		// Try getting archived views out of stream. Stream might be empty
		BMessage	data;
		err = data.Unflatten(fStream);
//+		PRINT(("err = %x, stream pos = %d\n", err, stream->Position()));
		if (err == B_OK) {
			// we've got something.
			long		i = 0;
			bool		b;

			if (data.FindBool(S_DISPLAY_ZOMBIES, &b) != B_OK)
				b = false;
//+			PRINT(("display=%d\n", b));
			SetDisplaysZombies(b);

			if (data.FindBool(S_ALLOW_ZOMBIES, &b) != B_OK)
				b = true;
//+			PRINT(("allow=%d\n", b));
			SetAllowsZombies(b);

			if (data.FindInt32(S_GENERATION_COUNT, (int32*) &fGenCount) != B_OK)
				fGenCount = FIRST_VALID_UID;

			while (1) {
				BMessage *archive = new BMessage();
				status_t err = data.FindMessage("dviews", i++, archive);
				if (err) {
					delete archive;
					break;
				}

				uint32	uid;
				/*
				 We're just initializing a shelf from an archive. So this
				 replicant already belongs to the shelf, so it should already
				 have the correct uid. For compatibility with PR2 need to
				 make sure since uid's didn't exist.
				*/
				if (archive->FindInt32(S_UID, (int32*) &uid) != B_OK) {
					// uids didn't exist in PR2, so create a new one.
					uid = INVALID_UID;
				}
				archive->RemoveName(S_UID);

				err = RealAddReplicant(archive, NULL, uid);
				if (err)
					delete archive;
			}

		// The comment below was typed by a 6 month old friend!
		// iunnn n8
		}
	}

	if (fEntry) {
		// if we have the entry to the file then don't keep around
		// an active/open stream.
		if (fStream) {
			delete fStream;
			fStream = NULL;
		}
	}
}

/*------------------------------------------------------------*/

BShelf::BShelf(BMessage *data)
	: BHandler(data), fReplicants(5)
{
	fContainerView = NULL;
	fStream = NULL;
	fEntry = NULL;
	fFilter = NULL;
	fDirty = true;

	// ???
//+	debugger("This function should never be called\n");
}

/* ---------------------------------------------------------------- */

BShelf::~BShelf()
{
	fContainerView->RemoveFilter(fFilter);
	Save();
	if (fEntry) {
		delete fEntry;
		fEntry = NULL;
	}
}

/* ---------------------------------------------------------------- */

BDataIO *BShelf::SaveLocation(entry_ref *pref) const
{
	entry_ref	ref;
	
	if (fStream) {
		if (pref)
			*pref = ref;
		return fStream;
	}
	
	if (fEntry) {
		fEntry->GetRef(&ref);
	}

	if (pref)
		*pref = ref;

	return NULL;
}

/* ---------------------------------------------------------------- */

status_t BShelf::SetSaveLocation(BDataIO *stream)
{
	fDirty = true;

	if (fEntry) {
		delete fEntry;
		fEntry = NULL;
	}
	fStream = stream;
	
	return B_OK;
}


/* ---------------------------------------------------------------- */

status_t BShelf::SetSaveLocation(const entry_ref *ref)
{
	fDirty = true;

	if (fEntry) {
		delete fEntry;
		fEntry = NULL;
	}
	fStream = NULL;
	fEntry = new BEntry(ref);

	return fEntry ? B_OK : B_NO_MEMORY;
}

/* ---------------------------------------------------------------- */

status_t BShelf::Save()
{
	status_t	err = B_OK;

	if (!fDirty)
		return B_OK;

	if (fEntry) {
		// open a new stream, truncating the file
		BFile	*f = new BFile(fEntry, O_RDWR | O_TRUNC);
		if ((err = f->InitCheck()) != B_OK)
			return err;
		fStream = f;
	}
	// If we were initialized with a stream we're assuming that
	// the stream is currently positioned at the correct location.

	// we won't have an fStream if we tried to open it on a read only
	// volume, don't save in that case
	if (fStream) {
		// being deleted so archive our current state
		BMessage	data;
		err = _Archive(&data);
		if (!err)
			err = data.Flatten(fStream);
	}

//+	??? fDirty isn't fully implemented yet. So don't do the following
//+	if (!err)
//+		fDirty = true;

	if (fEntry) {
		delete fStream;
		fStream = NULL;
	}

	return err;
}

/* ---------------------------------------------------------------- */

status_t BShelf::Archive(BMessage *, bool) const
{
	return B_ERROR;
}

/* ---------------------------------------------------------------- */

status_t BShelf::_Archive(BMessage *data) const
{
	// The BShelf doesn't archive itself in the std way. It doesn't
	// just archive all it views in fContainerViews. Instead, it only
	// archives the 'dropped' views.
	BDragger	*dw;
	long		i = 0;
	long		err;
	_rep_data_	*rdata;

	BHandler::Archive(data);

	data->AddBool(S_DISPLAY_ZOMBIES, DisplaysZombies());
	data->AddBool(S_ALLOW_ZOMBIES, AllowsZombies());
	data->AddInt32(S_GENERATION_COUNT, fGenCount);

//+	PRINT(("Archiving %d views\n", fReplicants.CountItems()));
	while ((rdata = (_rep_data_ *)fReplicants.ItemAt(i++)) != 0) {
		BMessage	archive(B_ARCHIVED_OBJECT);
		
		ASSERT(rdata->uid != INVALID_UID);

		if (!rdata->view) {
			// This replicant wasn't rehydrated. But save the archive
			// anyway, so that it still exists next time around.
			ASSERT(!rdata->view);
			ASSERT(rdata->archive->HasInt32(S_UID) == false);
			rdata->archive->AddInt32(S_UID, rdata->uid);
			data->AddMessage("dviews", rdata->archive);
			continue;
		}

		dw = rdata->widget;
//+		PRINT(("view=%x, dragger=%x\n", rdata->view, rdata->widget));

		ASSERT(rdata->view);
		// must deal with the std 3 cases: PARENT, CHILD, SIBLING
		// remember that the Dragger is optional. It might not exist!
		if (dw && (dw->fRelation == BDragger::TARGET_IS_CHILD)) {
			// simply archive the widget, the child view will get archived
//+			PRINT_OBJECT(dw->Frame());
			err = dw->Archive(&archive, true);
		} else if (dw && (dw->fRelation == BDragger::TARGET_IS_PARENT)) {
			// simply archive the view, the child widget will get archived
//+			PRINT_OBJECT(dw->fTarget->Frame());
			ASSERT(rdata->view == dw->fTarget);
			err = dw->fTarget->Archive(&archive, true);
		} else {
			// must archive each view indendently
			ASSERT(!dw || (dw->fRelation == BDragger::TARGET_IS_SIBLING));
			ASSERT(!dw || (rdata->view == dw->fTarget));
//+			PRINT_OBJECT(dw->fTarget->Frame());
			err = rdata->view->Archive(&archive, true);
			if (!err && dw) {
				BMessage aux(B_ARCHIVED_OBJECT);
//+				PRINT_OBJECT(dw->Frame());
				// put the widget view itself into a field of the archive
				err = dw->Archive(&aux, true);
				if (!err)
					err = archive.AddMessage("__widget", &aux);
			}
		}

		if (err)
			continue;

		ASSERT(archive.HasInt32(S_UID) == false);
		archive.AddInt32(S_UID, rdata->uid);
		data->AddMessage("dviews", &archive);
	}

	return 0;
}

/* ---------------------------------------------------------------- */

BArchivable *BShelf::Instantiate(BMessage *)
{
	return NULL;
}

/* ---------------------------------------------------------------- */

void BShelf::SetDirty(bool state)
{
	fDirty = state;
}

/* ---------------------------------------------------------------- */

bool BShelf::IsDirty() const
{
	return fDirty;
}

/* ---------------------------------------------------------------- */

void BShelf::SetAllowsZombies(bool state)
{
	fAllowZombies = state;
}

/* ---------------------------------------------------------------- */

bool BShelf::AllowsZombies() const
{
	return fAllowZombies;
}

/* ---------------------------------------------------------------- */

void BShelf::SetDisplaysZombies(bool state)
{
	fDisplayZombies = state;
}

/* ---------------------------------------------------------------- */

bool BShelf::DisplaysZombies() const
{
	return fDisplayZombies;
}

/* ---------------------------------------------------------------- */

void BShelf::SetTypeEnforced(bool state)
{
	fTypeEnforced = state;
}

/* ---------------------------------------------------------------- */

bool BShelf::IsTypeEnforced() const
{
	return fTypeEnforced;
}

/* ---------------------------------------------------------------- */

void BShelf::SetAllowsDragging(bool s)
{
	fAllowDragging = s;
}

/* ---------------------------------------------------------------- */

bool BShelf::AllowsDragging() const
{
	return fAllowDragging;
}

/* ---------------------------------------------------------------- */
/* ---------------------------------------------------------------- */
/* ---------------------------------------------------------------- */

_TContainerViewFilter_::_TContainerViewFilter_(BShelf *owner, BView *container)
	: BMessageFilter(B_ANY_DELIVERY, B_ANY_SOURCE)
{
	fShelf = owner;
	ASSERT(fShelf);
	fContainerView = container;
}

/* ---------------------------------------------------------------- */

_TContainerViewFilter_::~_TContainerViewFilter_()
{
}

/* ---------------------------------------------------------------- */

filter_result	_TContainerViewFilter_::Filter(BMessage *msg, BHandler **target)
{
	filter_result result = B_DISPATCH_MESSAGE;

//+	if (msg->what != B_MOUSE_MOVED)
//+		PRINT(("filtering (%.4s)\n", (char*) &(msg->what)));

	switch (msg->what) {
		case B_ABOUT_REQUESTED:
			// If the ABOUT request made it this far it means that the 
			// replicant view didn't handle the message. Redirect to the shelf
			// and it will display a generic AboutBox.
			*target = fShelf;
			break;
		case B_ARCHIVED_OBJECT:
			result = ObjectDropFilter(msg, target);
//+			PRINT(("result=%d\n", result));
			break;
	}

	return result;
}

/* ---------------------------------------------------------------- */

filter_result	_TContainerViewFilter_::ObjectDropFilter(BMessage *msg,
	BHandler **target)
{
	status_t	err;
	BView		*parent;
	parent = cast_as((*target), BView);
	ASSERT(parent);

//+	PRINT(("target=%x, fContainerView=%x\n", *target, fContainerView));
	if ((msg->WasDropped()) && (!fShelf->fAllowDragging)) {
//+		PRINT(("dragging isn't allowed to this shelf\n"));
		syslog(LOG_ERR, "Dragging replicants isn't allowed to this shelf.");
		beep();
		return B_SKIP_MESSAGE;
	}

	BPoint pt = B_ORIGIN;
	if (msg->WasDropped()) {
		BPoint	offset;
		pt = msg->DropPoint(&offset);
//+		PRINT(("drop ")); PRINT_OBJECT(offset);
		pt = pt - offset;
//+		PRINT(("\ndrop (scr) ")); PRINT_OBJECT(pt);
		pt = parent->ConvertFromScreen(pt);
//+		PRINT(("drop (par) ")); PRINT_OBJECT(pt);
//+		PRINT_OBJECT(parent->Bounds());
	}

	// determine the source of the drop! If it was from the same container
	// view then we want to do a move, rather than a copy
	BMessenger mess = msg->ReturnAddress();
	BHandler *srch;
	BLooper	*srcl;
	BPoint adjustByDelta;

	srch = mess.Target(&srcl);
	if (srcl == Looper()) {
		/*
		 In this case we're just moving an existing replicant within the
		 same container. So just do the move and return.
		*/
//+		PRINT_OBJECT(pt);
		BDragger *dw = dynamic_cast<BDragger*>(srch);
		ASSERT(dw);

		// have Shelf constrain if it needs to
		if (dw->fRelation == BDragger::TARGET_IS_CHILD) {
			BRect destination = dw->Frame();
			destination.OffsetTo(pt);
			adjustByDelta = fShelf->AdjustReplicantBy(destination, msg);
		} else {
			BRect destination = dw->fTarget->Frame();
			destination.OffsetTo(pt);
			adjustByDelta = fShelf->AdjustReplicantBy(destination, msg);
		}
//+		PRINT_OBJECT(adjustByDelta);

		if (dw->fRelation == BDragger::TARGET_IS_PARENT)
			dw->fTarget->MoveTo(pt + adjustByDelta);
		else if (dw->fRelation == BDragger::TARGET_IS_CHILD)
			dw->MoveTo(pt + adjustByDelta);
		else {
			BPoint offset;
			offset = dw->Frame().LeftTop() - dw->fTarget->Frame().LeftTop();
			dw->fTarget->MoveTo(pt + adjustByDelta);
			dw->MoveTo(pt + offset + adjustByDelta);
		}
		return B_SKIP_MESSAGE;
	}

	err = fShelf->RealAddReplicant(msg, &pt, INVALID_UID);
	if (!err) {
		// The shelf now owns the msg, so let's detach from the window
		Looper()->DetachCurrentMessage();
	}

	return B_SKIP_MESSAGE;
}

/* ---------------------------------------------------------------- */
/* ---------------------------------------------------------------- */
/* ---------------------------------------------------------------- */

int32 BShelf::CountReplicants() const
{
	return fReplicants.CountItems();
}

/* ---------------------------------------------------------------- */

void BShelf::ReplicantDeleted(int32 /*+index*/, const BMessage *,
	const BView */*+view*/)
{
//+	PRINT(("Deleting replicant index=%d (view=%x)\n", index, view));
}

/* ---------------------------------------------------------------- */

#if _PR2_COMPATIBLE_

#pragma export on

extern "C" void _ReservedShelf1__6BShelfFv(BShelf *const, int32,
	const BMessage *, const BView *);

#pragma export reset

extern "C" void _ReservedShelf1__6BShelfFv(BShelf *const THIS, int32 index,
	const BMessage *msg, const BView *v)
{
	void (BShelf::*func)(int32, const BMessage *, const BView *);
	func = &BShelf::ReplicantDeleted;
	_patch_vtable_(((void **)THIS)[0], _ReservedShelf1__6BShelfFv,
		&func);

	// explicit call to the new function! Don't make another virtual call
	// or we'll potentially end up calling client code again.
	THIS->BShelf::ReplicantDeleted(index, msg, v);
	return;
}

#endif

/* ---------------------------------------------------------------- */

status_t BShelf::DeleteReplicant(int32 index)
{
	_rep_data_	*rdata;
	BView		*view;
	BDragger	*widget;
	bool		unload;
	image_id	iid;
	
	rdata = (_rep_data_ *) fReplicants.ItemAt(index);
	if (!rdata)
		return B_BAD_INDEX;

	rdata->archive->FindBool(B_UNLOAD_ON_DELETE_ENTRY, &unload);
//+	PRINT(("delete rep: index=%d, addon=%d, unload=%d\n",
//+		index, rdata->addon_id, unload));

	view = rdata->view;
	if (!view)
		view = rdata->zombie_view;
	widget = rdata->widget;

	if (view) {
//		view->Hide();		// call to Hide is workaround for bug #9939

		view->RemoveSelf();
	}
	if (widget) {
//		widget->Hide();		// call to Hide is workaround for bug #9939

		widget->RemoveSelf();
	}

	ReplicantDeleted(index, rdata->archive, rdata->view);
	
	iid = rdata->addon_id;
	fReplicants.RemoveItem(rdata);
	delete rdata;

	if (unload && (iid >= B_OK)) {
		unload_add_on(iid);
	}

	return B_OK;
}


/* ---------------------------------------------------------------- */

status_t BShelf::DeleteReplicant(BView *view)
{
	return DeleteReplicant(_rep_data_::index_of(&fReplicants, view, true));
}


/* ---------------------------------------------------------------- */

status_t BShelf::DeleteReplicant(BMessage *msg)
{
	return DeleteReplicant(_rep_data_::index_of(&fReplicants, msg));
}


/* ---------------------------------------------------------------- */

BMessage *BShelf::ReplicantAt(int32 index, BView **pv, uint32 *uid,
	status_t *pe) const
{
	/*
	 There might not be a "view" for every entry in the list. That's because
	 some replicants might have failed during instantiation time. If so
	 then the "error" code will give the reason.
	*/
	BMessage	*msg = NULL;
	BView		*v = NULL;
	status_t	e = B_BAD_INDEX;
	uint32		u = INVALID_UID;
	_rep_data_	*rdata = (_rep_data_ *) fReplicants.ItemAt(index);

	if (rdata) {
		msg = rdata->archive;
		v = rdata->view;
		e = rdata->error;
		u = rdata->uid;
	}

	if (pv)
		*pv = v;
	if (uid)
		*uid = u;
	if (pe)
		*pe = e;
	
	return msg;
}


/* ---------------------------------------------------------------- */

int32 BShelf::IndexOf(const BView *view) const
{
	return _rep_data_::index_of(&fReplicants, view);
}

/* ---------------------------------------------------------------- */

int32 BShelf::IndexOf(const BMessage *arch) const
{
	return _rep_data_::index_of(&fReplicants, arch);
}

/* ---------------------------------------------------------------- */

int32 BShelf::IndexOf(uint32 id) const
{
	return _rep_data_::index_of(&fReplicants, id);
}

/* ---------------------------------------------------------------- */

status_t BShelf::AddReplicant(BMessage *msg, BPoint location)
{
	return RealAddReplicant(msg, &location, INVALID_UID);
}

/* ---------------------------------------------------------------- */

status_t BShelf::RealAddReplicant(BMessage *msg, BPoint *ploc,
	uint32 uid)
{
	status_t		error;
	BPoint			location;
	const char		*shelf_type;
	const char		*this_name;

	BView					*theView = NULL;
	BDragger				*dw = NULL;
	BDragger::relation		rel = BDragger::TARGET_UNKNOWN;
	BMessage				aux;
	BMessage				reply;
	BPoint					adjustByDelta;
	BView					*view = NULL;
	BArchivable				*obj = NULL;
	_BZombieReplicantView_	*zombie_view = NULL;
	bool					was_dropped;
	image_id				addon_id = B_BAD_VALUE;
	image_id				addon2_id = B_BAD_VALUE;
	_rep_data_				*rd;
	
	was_dropped = msg->WasDropped();

//+	PRINT_OBJECT((*msg));
	msg->FindString("shelf_type", &shelf_type);
	this_name = Name();

	// cond 1: Replicant indicated a 'type', but shelf didn't have a type
	if (shelf_type && !this_name) {
		syslog(LOG_ERR, "Replicant was rejected by BShelf: Replicant "
			"indicated a <type> (%s), but the shelf does not.", shelf_type);
		error = B_MISMATCHED_VALUES;
		goto skip;
	}

	// cont 2: Shelf has a type and wants to Enforce By Type, but the
	//		 Replicant didn't have a type
	if (!shelf_type && this_name && IsTypeEnforced()) {
		syslog(LOG_ERR, "Replicant was rejected by BShelf: Shelf is enforcing "
			"a <type> (%s), but the replicant doesn't have a type.",shelf_type);
		error = B_MISMATCHED_VALUES;
		goto skip;
	}

	// cand 3: Both had types, but they didn't match.
	if (shelf_type && this_name && (strcmp(shelf_type, this_name) != 0)) {
		syslog(LOG_ERR, "Replicant was rejected by BShelf: The BShelf's type "
			"and the Replicant's type don't match.");
		error = B_MISMATCHED_VALUES;
		goto skip;
	}

	if (!CanAcceptReplicantMessage(msg)) {
		// the shelf will not accept this view
//+		PRINT(("this shelf rejected the replicant message\n"));
		syslog(LOG_ERR, "Replicant was rejected by "
			"BShelf::CanAcceptReplicantMessage.");
		error = B_ERROR;
		goto skip;
	}

	if (msg->FindBool(B_UNIQUE_REPLICANT_ENTRY) == true) {
		// need to ensure this replicant isn't already in the shelf
		// uniqueness is defined by class_name and app_sig.
		const char *class_name = NULL; 
		const char *app_sig = NULL; 
		if (msg->FindString(B_CLASS_NAME_ENTRY, &class_name) == B_OK) {
			BMessage	*data;
			int32		i = 0;
			msg->FindString(B_ADD_ON_SIGNATURE_ENTRY, &app_sig);

			while ((data = ReplicantAt(i++)) != NULL) {
				const char *cn = NULL;
				const char *as = NULL;
				data->FindString(B_CLASS_NAME_ENTRY, &cn);
				data->FindString(B_ADD_ON_SIGNATURE_ENTRY, &as);
				ASSERT(cn);
//+				PRINT(("comparing (%s,%s)\n", cn, class_name));
				if ((strcmp(cn, class_name) == 0) &&
					((!as && !app_sig) || (as && app_sig && strcmp(as, app_sig) == 0)))
				{
					syslog(LOG_ERR,
						"Replicant was rejected. Unique replicant already in"
						"exists. class=%s, signature=%s", cn, as?as:"null");
					error = B_NAME_IN_USE;
					goto skip;
				}
			}
		}
	}

	obj = instantiate_object(msg, &addon_id);
//+	PRINT(("main case, id=%d\n", addon_id));

	error = errno;		// instantiate_object sets errno.
	if (obj && !(view = dynamic_cast<BView*>(obj))) {
		// what we got wasn't a view!!!
		syslog(LOG_ERR, "Replicant was rejected by BShelf because the Replicant isn't a BView.");
		error = B_ERROR;
		goto skip;
	}

//+	PRINT(("class_name obj=%s\n", obj ? class_name(obj) : "null"));
	if (!obj && AllowsZombies()) {
		/*
		 We couldn't recreate the replicant. Perhaps there is a missing
		 library/executable. In any case we'll still save off the
		 replicant, even without any live views.
		*/
//+		PRINT(("zombie_visible = %d\n", DisplaysZombies()));
		if (DisplaysZombies()) {
			BRect	rect;

			// there should be a S_FRAME, but just in case
			if (msg->FindRect(S_FRAME, &rect) != B_OK)
				rect = BRect(0,0,32,32);

			if (rect.Width() > 64)
				rect.right = rect.left + 64;
			else if (rect.Width() < 16)
				rect.right = rect.left + 16;
			if (rect.Height() > 64)
				rect.bottom = rect.top + 64;
			else if (rect.Height() < 16)
				rect.bottom = rect.top + 16;
			if (msg->WasDropped()) {
				BPoint	offset, drop;
				drop = msg->DropPoint(&offset);
				drop = fContainerView->ConvertFromScreen(drop);
				rect.OffsetTo(B_ORIGIN);
				rect.OffsetTo(drop - rect.RightBottom());
			}
			zombie_view = new _BZombieReplicantView_(rect, error);

			rect.OffsetTo(B_ORIGIN);
			rect.left = rect.right - 7;
			rect.top = rect.bottom - 7;
			dw = new BDragger(rect, zombie_view);
			dw->SetShelf(this);
			dw->SetZombied(true);
			zombie_view->AddChild(dw);
			zombie_view->AddFilter(
				new _TReplicantViewFilter_(this, zombie_view));
			fContainerView->AddChild(zombie_view);
		}
		// skip the normal view initialization since we're created a Zombie.
		goto done;
	} else if (!obj) {
		// aren't allowing zombies
		goto skip;
	}

	if (ploc)
		location = *ploc;
	else
		location = view->Frame().LeftTop();

	// Remember that there are 3 possible relationships between the drag
	// widget and the real view. The PARENT/CHILD cases are easy. When the
	// Dragger is added to the window (AttachedToWindow) it can determine
	// the relationship.

	if (msg->FindMessage("__widget", &aux) == B_NO_ERROR) {
		// we've got the tough case. The Widget is a sibling view, so it
		// was packaged up separately.
		obj = instantiate_object(&aux, &addon2_id);
//+		PRINT(("Tough case, id=%d\n", addon2_id));
		if (obj) {
			dw = dynamic_cast<BDragger*>(obj);
			if (dw) {
				BPoint offset;
				// Both "Frame" rects are relative to the common parent view.
				// They are used to determine their relative positions.
				offset = dw->Frame().LeftTop() - view->Frame().LeftTop();
				dw->SetViewToDrag(view);
				dw->MoveTo(location + offset);
				theView = view;
				rel = BDragger::TARGET_IS_SIBLING;
			}
		}
	} else if ((dw = dynamic_cast<BDragger*>(view)) != NULL) {
		theView = dw->ChildAt(0);
		dw->SetViewToDrag(theView);
		rel = BDragger::TARGET_IS_CHILD;
//+		PRINT(("Child Target = %x\n", theView));
	} else {
		// The Dragger is a child of 'view'
		rel = BDragger::TARGET_IS_PARENT;
		theView = view;
		view = theView->FindView("_dragger_");
		dw = dynamic_cast<BDragger*>(view);
		if (dw)
			dw->SetViewToDrag(theView);
//+		PRINT(("Child Dragger = %x\n", dw));
		/*
		 A dragger doesn't need to exist. In a programtic "send" of a replicant
		 and in a shelf that doesn't allow dragging, there might not be a
		 dragger.
		*/
	}

	if (!theView) {
		if (rel == BDragger::TARGET_IS_SIBLING) {
			delete theView;
			delete dw;
		} else if (rel == BDragger::TARGET_IS_CHILD)
			delete dw;
		else
			delete theView;
		goto skip;
	}

	ASSERT(theView);

	if (!CanAcceptReplicantView(theView->Frame(), theView, msg)) {
		// the shelf will not accept this view
		// delete correctly depending on the hierarchy
		if (rel == BDragger::TARGET_IS_SIBLING) {
			delete theView;
			delete dw;
		} else if (rel == BDragger::TARGET_IS_CHILD)
			delete dw;
		else
			delete theView;

//+		PRINT(("this shelf rejected the replicant view & message\n"));
		syslog(LOG_ERR, "Replicant was rejected by BShelf::CanAcceptReplicantView.");
		error = B_ERROR;
		goto skip;
	}

	// give the shelf a chance to do any aligning; shelves can override
	// this to implement snap to grid, etc.
	if (rel == BDragger::TARGET_IS_CHILD) {
		BRect destRect = dw->Frame();
		destRect.OffsetTo(location);
		adjustByDelta = AdjustReplicantBy(destRect, msg);
	} else {
		// The PARENT and SIBLING cases are treated together
		BRect destRect = theView->Frame();
		destRect.OffsetTo(location);
		adjustByDelta = AdjustReplicantBy(destRect, msg);
	}
	
	if (dw)
		dw->SetShelf(this);		// must do this before adding to window.

	if (rel == BDragger::TARGET_IS_SIBLING) {
		theView->MoveTo(location + adjustByDelta);
		fContainerView->AddChild(theView);
		dw->MoveBy(adjustByDelta.x, adjustByDelta.y);
		fContainerView->AddChild(dw);
	} else if (rel == BDragger::TARGET_IS_CHILD) {
		dw->MoveTo(location + adjustByDelta);
		fContainerView->AddChild(dw);
	} else {
		// TARGET_IS_PARENT
		theView->MoveTo(location + adjustByDelta);
		fContainerView->AddChild(theView);
	}

	theView->AddFilter(new _TReplicantViewFilter_(this, theView));

done:
	ASSERT(msg);
	/*
	 If we have a 'dropped' msg we don't really
	 want to keep these extra fields, so remove them. Doing so is error
	 prone if we ever add any new such fields ???
	*/
	msg->RemoveName("_drop_point_");
	msg->RemoveName("_drop_offset_");

	if (uid == INVALID_UID) {
		/*
		 Each replicant that gets added to a shelf is given a unique 'id' (or
		 a generation count). 
		*/
		uid = fGenCount++;
	}

	ASSERT((addon2_id < B_OK) || (addon2_id == addon_id));

	rd = new _rep_data_(msg, theView, dw, rel, uid, addon_id);
	rd->error = error;
	rd->zombie_view = zombie_view;
	
	if (zombie_view) {
		zombie_view->SetArchive(msg);
	}

	fReplicants.AddItem(rd);

//+	PRINT(("sender waiting(%x)=%d\n", msg, msg->IsSourceWaiting()));
	if (msg->IsSourceWaiting()) {
		reply.AddInt32("id", uid);
		reply.AddInt32("error", B_OK);
		msg->SendReply(&reply);
	}	

	return B_OK;

skip:

	// simple beep to notify user of the error. If the message wasn't dropped
	// then it was programmtically sent. In this case the sender can take care
	// of any error notification.
	if (was_dropped)
		beep();

//+	PRINT(("err=%x (%s)\n", error, strerror(error)));
	if (msg->IsSourceWaiting()) {
		reply.AddInt32("error", error);
		msg->SendReply(&reply);
	}	
	return error;
}

/*-------------------------------------------------------------*/

BShelf::BShelf(const BShelf &)
	:	BHandler()
	{}

BShelf &BShelf::operator=(const BShelf &) {return *this;}

/*-------------------------------------------------------------*/
#if _SUPPORTS_FEATURE_SCRIPTING

enum {
	_UNKNOWN,
	_FOR_SHELF,
	_FOR_REPLICANT,
	_FOR_PREV_REPLICANT,
	_FOR_VIEW
};

static	property_info	replicant_prop_list[] = {
	{"ID",
		{B_GET_PROPERTY},
		{B_DIRECT_SPECIFIER},
		"",
		_FOR_PREV_REPLICANT,
		{ B_INT32_TYPE },
		{},
		{}
	},
	{"Name",
		{B_GET_PROPERTY},
		{B_DIRECT_SPECIFIER},
		"",
		_FOR_PREV_REPLICANT,
		{ B_STRING_TYPE },
		{},
		{}
	},
	{"Signature",
		{B_GET_PROPERTY},
		{B_DIRECT_SPECIFIER},
		"",
		_FOR_PREV_REPLICANT,
		{ B_STRING_TYPE },
		{},
		{}
	},
	{"Suites",
		{B_GET_PROPERTY},
		{B_DIRECT_SPECIFIER},
		"",
		_FOR_PREV_REPLICANT,
		{ B_PROPERTY_INFO_TYPE },
		{},
		{}
	},
	{"View",
		{},
		{B_DIRECT_SPECIFIER},
		"",
		_FOR_VIEW,
		{},
		{},
		{}
	},
	{NULL,
		{},
		{},
		NULL, 0,
		{},
		{},
		{}
	}
};

static	property_info	prop_list[] = {
	{"Replicant",
		{B_COUNT_PROPERTIES, B_CREATE_PROPERTY},
		{B_DIRECT_SPECIFIER},
		"",
		_FOR_SHELF,
		{},
		{},
		{}
	},
	{"Replicant",
		{B_GET_PROPERTY, B_DELETE_PROPERTY},
		{B_INDEX_SPECIFIER, B_REVERSE_INDEX_SPECIFIER, B_NAME_SPECIFIER,
			B_ID_SPECIFIER},
		"",
		_FOR_REPLICANT,
		{},
		{},
		{}
	},
	{"Replicant",
		{},			// allows any command
		{B_INDEX_SPECIFIER, B_REVERSE_INDEX_SPECIFIER, B_NAME_SPECIFIER,
			B_ID_SPECIFIER},
		"... of Replicant {index | name | id} of ...",
		_UNKNOWN,
		{},
		{},
		{}
	},
	{NULL,
		{},
		{},
		NULL, 0,
		{},
		{},
		{}
	}
};
#endif

/*-------------------------------------------------------------*/

BHandler *BShelf::ResolveSpecifier(BMessage *_SCRIPTING_ONLY(msg), int32 _SCRIPTING_ONLY(index),
	BMessage *_SCRIPTING_ONLY(spec), int32 _SCRIPTING_ONLY(form), const char *_SCRIPTING_ONLY(prop))
{
#if _SUPPORTS_FEATURE_SCRIPTING
	status_t	err = B_OK;
	BMessage	emsg(B_MESSAGE_NOT_UNDERSTOOD);
	BHandler	*target = NULL;
	BView		*target_view = NULL;
	BMessage	*archive = NULL;
	int32		target_index = -1;
	uint32		ud;
	
//+	PRINT(("BShelf::Resolve: msg->what=%.4s, index=%d, form=0x%x, prop=%s\n",
//+		(char*) &(msg->what), index, spec->what, prop));

	BPropertyInfo	pi(prop_list);

	int32	i = pi.FindMatch(msg, index, spec, form, prop);
//+	PRINT(("FindMatch=%d\n", i));

	if (i < 0) {
		goto done;
	}

	ud = prop_list[i].extra_data;

	if (ud == _FOR_SHELF) {
//+		PRINT(("going directly to the shelf\n"));
		target = this;
		goto done;
	}

	/*
	 some of the cases use INDEX or NAME, so let's determine
	 the target replicant here.
	 Of course the message might actually get sent to the replicant view
	*/

	switch (form) {

	case B_REVERSE_INDEX_SPECIFIER:
	case B_INDEX_SPECIFIER: {
		int		index = spec->FindInt32("index");
		if (form == B_REVERSE_INDEX_SPECIFIER)
			index = CountReplicants() - index;
		archive = ReplicantAt(index, &target_view);
//+		PRINT(("	by index: (%s)\n",
//+			target_view ? target_view->Name() : "null"));
		if (!archive) {
			err = B_BAD_INDEX;
			emsg.AddString("message", "index out of range");
		} else {
			target_index = index;
		}
		break;
		}

	case B_NAME_SPECIFIER: {
		int32		i = 0;
		BView		*view;
		const char *name = spec->FindString(B_PROPERTY_NAME_ENTRY);
		if (!name) {
			err = B_BAD_SCRIPT_SYNTAX;
			emsg.AddString("message", "missing name");
			break;
		}
		while ((archive = ReplicantAt(i++, &view)) != NULL) {
			if (view && view->Name() && (strcmp(name, view->Name()) == 0)) {
				target_view = view;
				target_index = i-1;
				break;
			}
		}

		if (!archive) {
			err = B_NAME_NOT_FOUND;
			emsg.AddString("message", "replicant with given name not found");
		}

		break;
		}

	case B_ID_SPECIFIER: {
		int32	i = 0;
		uint32	uid = spec->FindInt32("id");
		uint32	u;
		BView	*view;

		while ((archive = ReplicantAt(i++, &view, &u)) != NULL) {
			if (uid == u) {
				target_view = view;
				target_index = i-1;
				break;
			}
		}

		if (!archive) {
			err = B_BAD_VALUE;
			emsg.AddString("message", "id not found");
		}

		break;
		}
	default:
		PRINT(("unexpected case!!!\n"));
		break;
	}

	if (err)
		goto done;
	
	ASSERT(target_index >= 0);
	if (ud == _FOR_REPLICANT) {
		msg->AddInt32("_pjp:rep", target_index);
		target = this;
	} else {
		ASSERT(ud == _UNKNOWN);
		/*
		 Let's process the next specifier here using the replicant_prop_list.
		*/
		msg->PopSpecifier();
		msg->GetCurrentSpecifier(&index, spec, &form, &prop);
		BPropertyInfo	rpi(replicant_prop_list);
		int32	i = rpi.FindMatch(msg, index, spec, form, prop);
//+		PRINT(("index=%d, submatch=%d\n", index, i));
		if (i >= 0) {
			ud = replicant_prop_list[i].extra_data;
			ASSERT((ud == _FOR_PREV_REPLICANT) || (ud == _FOR_VIEW));
			if (ud == _FOR_VIEW) {
//+				PRINT(("forwarding to view\n"));
				if (target_view) {
					msg->PopSpecifier();
					target = target_view;
				} else {
					err = B_BAD_SCRIPT_SYNTAX;
					emsg.AddString("message", "zombie view didn't comprehend");
				}
			} else {
				// Let the shelf handle this command
//+				PRINT(("replicant %d\n", target_index));
				msg->AddInt32("_pjp:rep", target_index);
				target = this;
			}
		} else {
			err = B_BAD_SCRIPT_SYNTAX;
		}
	}

done:
	if (err) {
		emsg.AddInt32("error", err);
		msg->SendReply(&emsg);
		target = NULL;
	} else if (!target) {
		target = BHandler::ResolveSpecifier(msg, index, spec, form, prop);
	}

	return target;
#else
	return NULL;
#endif
}

/*-------------------------------------------------------------*/

status_t BShelf::GetProperty(BMessage *_SCRIPTING_ONLY(msg), BMessage *_SCRIPTING_ONLY(reply))
{
#if _SUPPORTS_FEATURE_SCRIPTING
	status_t	err = B_OK;
	BMessage	specifier;
	int32		form;
	const char	*prop;
	int32		cur;
	BView		*view = NULL;
	BMessage	*archive;
	int32		index;
	uint32		uid;

	msg->FindInt32("_pjp:rep", &index);
	ASSERT(msg->HasInt32("_pjp:rep"));
	archive = ReplicantAt(index, &view, &uid);
	
	if (!archive)
		return B_BAD_INDEX;

	err = msg->GetCurrentSpecifier(&cur, &specifier, &form, &prop);
//+	PRINT(("GetPropert(%s, rep_index=%d)\n", prop, index));
	ASSERT(err == B_OK);
	
	if (strcmp(prop, "Replicant") == 0) {
		if (view) {
			// archive the current state of the replicant.
			BMessage	cur_state;
			err = view->Archive(&cur_state);
			if (err == B_OK) {
				err = reply->AddMessage("result", &cur_state);
			}
		} else {
			// zombie view so archive saved state
			err = reply->AddMessage("result", archive);
		}
	} else if (strcmp(prop, "ID") == 0) {
		err = reply->AddInt32("result", uid);
	} else if (strcmp(prop, "Name") == 0) {
		const char *name;
		if (view) {
			// return current Name of view
			name = view->Name();
		} else {
			// zombie view so return the Name in the archive
			name = archive->FindString(S_NAME);
		}
		if (name)
			err = reply->AddString("result", name);
		else
			err = B_NAME_NOT_FOUND;
	} else if (strcmp(prop, "Signature") == 0) {
		const char *sig;
		sig = archive->FindString(B_ADD_ON_SIGNATURE_ENTRY);
		if (sig)
			err = reply->AddString("result", sig);
		else
			err = B_NAME_NOT_FOUND;
	} else if (strcmp(prop, "Suites") == 0) {
		reply->AddString("suites", "suite/vnd.Be-replicant");
		BPropertyInfo	rpi(replicant_prop_list);
		reply->AddFlat("messages", &rpi);
	}
	return err;
#else
	return B_UNSUPPORTED;
#endif
}

/* ---------------------------------------------------------------- */

void BShelf::MessageReceived(BMessage *msg)
{
	bool 		send_reply = false;
	bool 		handled = false;
	BMessage	reply(B_REPLY);
	status_t	err = B_OK;

#if _SUPPORTS_FEATURE_SCRIPTING
	BArchivable	*obj = NULL;
	BMessage	specifier;
	int32		form;
	const char	*prop;
	int32		cur;

	switch (msg->what) {
		case B_COUNT_PROPERTIES: {
			err = msg->GetCurrentSpecifier(&cur, &specifier, &form, &prop);
			if (!err && strcmp(prop, "Replicant") == 0) {
//+				PRINT(("count = %d\n", CountReplicants()));
				reply.AddInt32("result", CountReplicants());
				send_reply = handled = true;
			}
			break;
		}
		case B_CREATE_PROPERTY: {
			BMessage	*archive = new BMessage();
			BPoint		pt(0,0);
			msg->FindMessage("data", archive);
			msg->FindPoint("location", &pt);
			err = AddReplicant(archive, pt);
			if (err == B_OK)
				reply.AddInt32("reference", (int32) archive);
			else
				delete archive;
			send_reply = handled = true;
			break;
		}

		case B_DELETE_PROPERTY: {
			int32	index;
			msg->FindInt32("_pjp:rep", &index);
			ASSERT(msg->HasInt32("_pjp:rep"));
			err = DeleteReplicant(index);
			send_reply = handled = true;
			break;
		}

		case B_GET_PROPERTY: {
			if (msg->HasInt32("_pjp:rep")) {
				err = GetProperty(msg, &reply);
				send_reply = handled = true;
			}
			break;
		}

		case CMD_DELETE_VIEW: {
			msg->FindPointer("_target", (void **) &obj);
			ASSERT(obj);
			BView *view = dynamic_cast<BView*>(obj);
//+			PRINT(("obj=%x, view=%x\n", obj, view));

			if (view) {
				DeleteReplicant(view);
			}
			handled = true;
			break;
		}
		case B_ABOUT_REQUESTED: {
			const char *view_name;
			msg->FindString("target", &view_name);

			char *buffer = (char *)
				malloc((view_name ? strlen(view_name) : 20) + 120);
			sprintf(buffer, "%s: copyright info goes here.",
				view_name ? view_name : "<unknown>");

			BAlert	*about = new BAlert("", buffer, "OK");
			about->Go(NULL);
			free(buffer);
			handled = true;
			break;
		}
	}
#endif

	if (handled && send_reply) {
		reply.AddInt32("error", err);
		msg->SendReply(&reply);
	} else if (!handled)
		BHandler::MessageReceived(msg);
}

/*---------------------------------------------------------------*/

status_t	BShelf::GetSupportedSuites(BMessage *_SCRIPTING_ONLY(data))
{
#if _SUPPORTS_FEATURE_SCRIPTING
	data->AddString("suites", "suite/vnd.Be-shelf");
	BPropertyInfo	pi(prop_list);
	data->AddFlat("messages", &pi);
	return BHandler::GetSupportedSuites(data);
#else
	return B_UNSUPPORTED;
#endif
}

/*----------------------------------------------------------------*/

status_t BShelf::Perform(perform_code d, void *arg)
{
	return BHandler::Perform(d, arg);
}

/* ---------------------------------------------------------------- */

bool BShelf::CanAcceptReplicantMessage(BMessage *) const
{
	return true;
}

/* ---------------------------------------------------------------- */

bool BShelf::CanAcceptReplicantView(BRect, BView *, BMessage *) const
{
	// override to control accepting items that are too large, 
	// not accept to a shelf that is too full, etc.
	return true;
}

/* ---------------------------------------------------------------- */

BPoint BShelf::AdjustReplicantBy(BRect, BMessage *) const
{
	// override to implement snap to grid, etc.
	return BPoint(0, 0);
}

/* ---------------------------------------------------------------- */

//+void BShelf::_ReservedShelf1() {}
void BShelf::_ReservedShelf2() {}
void BShelf::_ReservedShelf3() {}
void BShelf::_ReservedShelf4() {}
void BShelf::_ReservedShelf5() {}

/* ---------------------------------------------------------------- */
/* ---------------------------------------------------------------- */
/* ---------------------------------------------------------------- */

_rep_data_::_rep_data_(BMessage *m, BView *v, BDragger *d,
	BDragger::relation r, uint32 u, image_id id)
{
	archive = m;
	view = v;
	widget = d;
	relation = r;
	uid = u;
	addon_id = id;
	zombie_view = NULL;
	error = B_OK;
}

/* ---------------------------------------------------------------- */

_rep_data_::_rep_data_()
{
	archive = NULL;
	view = NULL;
	widget = NULL;
	zombie_view = NULL;
	uid = INVALID_UID;
	relation = BDragger::TARGET_UNKNOWN;
	error = B_ERROR;
};

/* ---------------------------------------------------------------- */

_rep_data_::~_rep_data_()
{
	switch (relation) {
		case BDragger::TARGET_IS_SIBLING:
			ASSERT(view);
			ASSERT(widget);
			delete view;
			delete widget;
			break;
		case BDragger::TARGET_IS_CHILD:
			ASSERT(widget);
			delete widget;
			break;
		case BDragger::TARGET_IS_PARENT:
			ASSERT(view);
			delete view;
			break;
		case BDragger::TARGET_UNKNOWN:
			if (view)
				delete view;
			break;
	}

	if (archive)
		delete archive;
	if (zombie_view)
		delete zombie_view;
};

/* ---------------------------------------------------------------- */

int32 _rep_data_::index_of(const BList *list, const BView *view,
	bool incl_zombie)
{
	_rep_data_	*rd;
	int32		i = 0;

	while ((rd = (_rep_data_ *)list->ItemAt(i++)) != 0) {
		if (rd->view == view)
			return i - 1;
		if (incl_zombie && (rd->zombie_view == view))
			return i - 1;
	}
	return -1;
}

/* ---------------------------------------------------------------- */

_rep_data_ *_rep_data_::find(const BList *list, const BView *view,
	bool incl_zombie)
{
	_rep_data_	*rd;
	int32		i = 0;

	while ((rd = (_rep_data_ *)list->ItemAt(i++)) != 0) {
		if (rd->view == view)
			return rd;
		if (incl_zombie && (rd->zombie_view == view))
			return rd;
	}
	return NULL;
}

/* ---------------------------------------------------------------- */

int32 _rep_data_::index_of(const BList *list, const BMessage *msg)
{
	_rep_data_	*rd;
	int32		i = 0;

	while ((rd = (_rep_data_ *)list->ItemAt(i++)) != 0) {
		if (rd->archive == msg)
			return i - 1;
	}
	return -1;
}

/* ---------------------------------------------------------------- */

_rep_data_ *_rep_data_::find(const BList *list, const BMessage *msg)
{
	_rep_data_	*rd;
	int32		i = 0;

	while ((rd = (_rep_data_ *)list->ItemAt(i++)) != 0) {
		if (rd->archive == msg)
			return rd;
	}
	return NULL;
}

/* ---------------------------------------------------------------- */

int32 _rep_data_::index_of(const BList *list, uint32 uid)
{
	_rep_data_	*rd;
	int32		i = 0;

	while ((rd = (_rep_data_ *)list->ItemAt(i++)) != 0) {
		if (rd->uid == uid)
			return i - 1;
	}
	return -1;
}

/* ---------------------------------------------------------------- */

_rep_data_ *_rep_data_::find(const BList *list, uint32 uid)
{
	_rep_data_	*rd;
	int32		i = 0;

	while ((rd = (_rep_data_ *)list->ItemAt(i++)) != 0) {
		if (rd->uid == uid)
			return rd;
	}
	return NULL;
}

/* ---------------------------------------------------------------- */
/* ---------------------------------------------------------------- */
/* ---------------------------------------------------------------- */

_BZombieReplicantView_::_BZombieReplicantView_(BRect rect,
	status_t err)
//+	: BView(rect, "<Zombie>", B_FOLLOW_NONE, B_WILL_DRAW)
	: BBox(rect, "<Zombie>", B_FOLLOW_NONE, B_WILL_DRAW)
{
	fError = err;
	fArchive = NULL;

	BFont	aFont(be_plain_font);
	float	s = rect.Height() * 0.70;

	if (s < 14.0)
		s = 14.0;

	aFont.SetSize(s);
	SetFont(&aFont);
//+	PRINT(("height=%.1f\n", rect.Height()));
	fInit = false;
	SetViewColor(220,220,220,255);
}

/* ---------------------------------------------------------------- */

_BZombieReplicantView_::~_BZombieReplicantView_()
{
}


/* ---------------------------------------------------------------- */

void _BZombieReplicantView_::SetArchive(BMessage *a)
{
	fArchive = a;
}

/* ---------------------------------------------------------------- */

void _BZombieReplicantView_::MessageReceived(BMessage *msg)
{
	switch (msg->what) {
		case B_ABOUT_REQUESTED: {
			BAlert		*alert;
			char		buf[200];
			const char	*sig;
			char		app_name[B_MIME_TYPE_LENGTH];

			ASSERT(fArchive);
			strcpy(app_name, "???");
			if (fArchive->FindString(B_ADD_ON_SIGNATURE_ENTRY, &sig) == B_OK) {
				BMimeType	mtype(sig);
				if (mtype.GetShortDescription(app_name) == B_OK) {
//+					PRINT(("app_name=%s\n", app_name));
				} else {
					strcpy(app_name, sig);
				}
			}

			switch (fError) {
			case B_LAUNCH_FAILED_APP_IN_TRASH:
				sprintf(buf,
					"Can't create the \"%s\" replicant because the library "
					"is in the Trash. (%s)",
					app_name, strerror(fError));
				break;

			case B_BAD_VALUE:
				sprintf(buf,
					"Can't create the \"%s\" replicant because no signature "
					"was specified. (%s)",
					app_name, strerror(fError));
				break;

			case B_MISMATCHED_VALUES:
				sprintf(buf,
					"Can't create the \"%s\" replicant because no class name "
					"was specified. (%s)",
					app_name, strerror(fError));
				break;

			case B_FILE_NOT_FOUND:
			case B_LAUNCH_FAILED_APP_NOT_FOUND:
				sprintf(buf,
					"Can't create the \"%s\" replicant because the library "
					"is missing. (%s)",
					app_name, strerror(fError));
				break;

			default:
				sprintf(buf,
					"There was an error in creating the replicant \"%s\" (%s)",
					app_name, strerror(fError));
				break;
			}

			alert = new BAlert("Error", buf, "OK", NULL, NULL,
				B_WIDTH_AS_USUAL, B_STOP_ALERT);
			alert->Go(NULL);
			break;
		}
		default:
			BView::MessageReceived(msg);
		break;
	}
}

/* ---------------------------------------------------------------- */

void _BZombieReplicantView_::MouseDown(BPoint)
{
}

/* ---------------------------------------------------------------- */

void _BZombieReplicantView_::Draw(BRect update)
{
	BRect	bounds = Bounds();
//+	SetHighColor(0,0,0);
//+	StrokeRect(bounds);
	
	if (!fInit) {
		font_height	f;
		GetFontHeight(&f);
		float	h = f.ascent - f.descent;
//+		PRINT(("h=%.1f, d=%.1f, l=%.1f\n", h, f.descent, f.leading));
		float	w = StringWidth("?");

		fLoc.x = (bounds.Width() - w) / 2.0;
		fLoc.y = bounds.bottom - (bounds.Height() - h) / 2.0;

		fInit = false;
	}
	MovePenTo(fLoc);

//+	PRINT_OBJECT(PenLocation());
	DrawChar('?');
	BBox::Draw(update);
}

/* ---------------------------------------------------------------- */
/* ---------------------------------------------------------------- */
/* ---------------------------------------------------------------- */

_TReplicantViewFilter_::_TReplicantViewFilter_(BShelf *owner, BView *replicant)
	: BMessageFilter(B_ANY_DELIVERY, B_ANY_SOURCE)
{
	fShelf = owner;
	ASSERT(fShelf);
	fReplicantView = replicant;
}

/* ---------------------------------------------------------------- */

_TReplicantViewFilter_::~_TReplicantViewFilter_()
{
}

/* ---------------------------------------------------------------- */

filter_result	_TReplicantViewFilter_::Filter(BMessage *msg, BHandler **target)
{
	filter_result result = B_DISPATCH_MESSAGE;

//+	PRINT(("filtering (%.4s)\n", (char*) &(msg->what)));
	
	switch (msg->what) {
		case CMD_DELETE_VIEW:
			BView *v = dynamic_cast<BView*>(*target);
//+			PRINT(("rep_filter, class=%s\n", v ? class_name(v) : "null"));
			msg->AddPointer("_target", v);
			*target = fShelf;
			break;
	}

	return result;
}


/* ---------------------------------------------------------------- */
/* ---------------------------------------------------------------- */
/* ---------------------------------------------------------------- */

#if !_PR3_COMPATIBLE_

void 
BShelf::_ReservedShelf6()
{
}

void 
BShelf::_ReservedShelf7()
{
}

void 
BShelf::_ReservedShelf8()
{
}

#endif
