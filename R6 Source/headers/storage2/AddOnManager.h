/***************************************************************************
//
//	File:			AddOnManager.h
//
//	Description:	Classes for standard add-on roster implementation.
//
//	Copyright 2001, Be Incorporated, All Rights Reserved.
//
***************************************************************************/

#ifndef _ADD_ON_MANAGER_H
#define _ADD_ON_MANAGER_H

#include <image.h>

#include <storage2/Entry.h>
#include <storage2/Node.h>
#include <storage2/SearchPath.h>
#include <support2/Locker.h>
#include <support2/Value.h>
#include <support2/Vector.h>

namespace B {
namespace Private {
	class AddOnSearchPath;
} } // namespace B::Private

namespace B {
namespace Storage2 {

using namespace B::Private;
using namespace B::Support2;

enum {
	B_ADD_ONS_CHANGED		= 'ADCG'
};

// Flags for Flush().
enum {
	B_FLUSH_KEEP_LOADED		= (1<<0),
	B_FLUSH_DYNAMIC			= (1<<1),
	B_FLUSH_REFERENCED		= (1<<2)
};

class BAddOnHandle : public virtual BAtom
{
public:
	// This is a handle on a single available add-on, which may
	// not actually be loaded yet.  The entry_ref is required
	// to load the add-on from disk; the node_ref is used only
	// for identification during node monitoring.
						BAddOnHandle(const entry_ref* entry=NULL, const node_ref* node=NULL);

	// Where the add-on lives.
	status_t			SetEntryRef(const entry_ref* ref);
	entry_ref			EntryRef() const;

	node_ref			NodeRef() const;

	// Access to the add-on image.  Note that Open() on a handle
	// with no entry_ref WILL succeeded, but won't load an add_on --
	// it will just return the main application image.
	image_id			Open();
	void				Close();
	bool				Flush(uint32 flags=0);

	// Return true if this add-ons image is currently loaded.
	bool				IsLoaded() const;

	// Return true if this is a static add-on -- i.e., it is
	// opened, but there is no associated image file.
	bool				IsStatic() const;

	// Return true to keep this add-on loaded, even if its user
	// count has gone down to zero.  Note that it may still be
	// unloaded if the object is being destroyed.
	virtual	bool		KeepLoaded() const;

	// Return true if the identifiers for this add-on can
	// change, so it must always be loaded instead of storing
	// the identifiers in an attribute.  Note that this
	// implies KeepLoaded(), since the add-on must always be
	// queried for its attributes.
	virtual	bool		IsDynamic() const;

	// Return the amount of memory this -image- uses, if it is
	// loaded.
	virtual	size_t		GetMemoryUsage() const;

	// Retrieve identifying characteristics of this add-on.  If
	// 'quick' is true, only the cached attribute will be used --
	// the add-on image will not be loaded, if it isn't already.
	status_t			GetIdentifiers(BValue* into, bool quick=false) const;

	// Return true if there is an identifier named 'name' with
	// the text 'value'.  This is a caseless comparison.
	bool				MatchIdentifier(const char* name, const char* value, bool quick=false) const;

	// Return the number of seconds since this add-on was last opened.
	time_t				SecondsSinceOpen() const;

protected:
	virtual				~BAddOnHandle();

	// Hook functions.  These are are called with the BAddOnHandle
	// object locked.

	// Called right after the add-on image is loaded.
	virtual	void		ImageLoaded(image_id image);

	// Override this to retrieve information from a loaded add-on
	// image, which will be cached in its attributes.
	virtual	status_t	LoadIdentifiers(BValue* into, image_id from);

	// Called right before the add-on image is unloaded.  At this
	// point you should delete any objects that your add-on created
	// (in particular, whatever top-level add-on API class that
	// you got your identifiers from).
	virtual	void		ImageUnloading(image_id image);

	// Override to change the name of the attribute that its
	// identifier BValue is stored in.
	virtual	const char	*AttrBaseName() const;

	// Quick access to add-on identifiers, locking them down
	// while in use.  Be sure not to acquire any other locks
	// between these two calls.
	const BValue		*LockIdentifiers(bool quick=false) const;
	void				UnlockIdentifiers(const BValue* ident) const;

	// Locking the handle's state.  Do not call any other
	// BAddOnHandle methods while this lock is held.
	void				Lock() const;
	void				Unlock() const;

private:
	/* FBC */
	virtual	void		_HoldTheAddOnHandle1();
	virtual	void		_HoldTheAddOnHandle2();
	virtual	void		_HoldTheAddOnHandle3();
	virtual	void		_HoldTheAddOnHandle4();
	virtual	void		_HoldTheAddOnHandle5();
	virtual	void		_HoldTheAddOnHandle6();
	virtual	void		_HoldTheAddOnHandle7();
	virtual	void		_HoldTheAddOnHandle8();
	virtual	void		_HoldTheAddOnHandle9();
	virtual	void		_HoldTheAddOnHandle10();
	virtual	void		_HoldTheAddOnHandle11();
	virtual	void		_HoldTheAddOnHandle12();
	virtual	void		_HoldTheAddOnHandle13();
	virtual	void		_HoldTheAddOnHandle14();
	virtual	void		_HoldTheAddOnHandle15();
	virtual	void		_HoldTheAddOnHandle16();
	uint32				_holdTheData[8];

	image_id			do_open();
	void				do_close();
	bool				do_flush(uint32 flags=0);
	status_t			do_get_identifiers(BValue* into, bool quick);

	status_t			do_read_identifiers(BValue* into, BNode* from);
	status_t			do_write_identifiers(BNode* into, const BValue* from);

	mutable	BLocker		fAccess;
	node_ref			fNode;
	entry_ref			fEntry;
	image_id			fImage;
	status_t			fImageErr;
	int32				fUsers;
	time_t				fLastUsed;			// Last time this add-on was used.
	bool				fHasIdentifiers;
	bool				fStaticImage;		// Add-on was opened with no entry_ref?

	BValue				fIdentifiers;
};

class BAddOnManager
{
public:
						BAddOnManager(const char* name);
	virtual				~BAddOnManager();

	// Add directories to search (see BSearchPath for details).
	status_t			AddDirectory(const BEntry* dir);
	status_t			AddDirectory(const entry_ref* dir);
	status_t			AddDirectory(const char* dir, const char* leaf=NULL);
	status_t			AddDirectory(directory_which which, const char* leaf=NULL);
	status_t			AddSearchPath(const char* path, const char* leaf=NULL);
	status_t			AddEnvVar(const char* name, const char* leaf=NULL, const char* defEnvVal=NULL);

	// Do a one-time scan of the current search path, installing
	// the add-ons found.
	status_t			Scan();
	bool				IsScanned() const;

	// Begin handling all add-ons found in the previously
	// set search path.
	virtual	status_t	Run();
	bool				IsRunning() const;

	virtual	void		Shutdown(bool force_unload=false);

	// Set up notification of changes.
#warning "Fix {Start,Stop}Watching"
	//		status_t	StartWatching(BMessenger receiver);
	//		status_t	StopWatching(BMessenger receiver);

	// Access control for this object.
	BLocker				*Locker() const;

	// BList-like access to available add-ons.
	int32				CountAddOns() const;
	int32				IndexOfAddon(const BAddOnHandle *handle) const;
	BAddOnHandle 		*AddOnAt(int32 i) const;
	BAddOnHandle		*FindAddOn(const node_ref* node) const;

	// Manually adding and removing add-on handles.
	BAddOnHandle		*InstallAddOn(const entry_ref* entry, const node_ref* node);
	virtual	void		InstallAddOn(BAddOnHandle* addon);
	virtual	bool		RemoveAddOn(BAddOnHandle* addon);

	// Return amount of memory used by all add-ons.
	// NOTE: This is a fairly heavy-weight operation, as it
	// needs to run through every add-on and sum up the
	// results of their own GetMemoryUsage().
	virtual	size_t		GetMemoryUsage() const;

	// Note that we are going to use an add-on.  This will
	// cause the add-on to be shuffled up to the front of the
	// list of all add-ons.
	void				UsingAddOn(int32 i);

	// Flush add-ons that are currently not in-use, until the
	// requested amount of memory is reclaimed.  Add-ons are
	// flushed in order from the least recently used to the
	// latest used.
	// Returns amount of memory that was left over, that is
	// memory_needed - (amount flushed).
	ssize_t				PruneAddOnMemory(ssize_t memory_needed=-1);

	// Flush add-ons that are currently not in-use, based on
	// the amount of time since they were last opened.  Add-ons
	// are flushed in order from the least recently used to the
	// latest used; the minimum time needed for a flush starts
	// at min_seconds, and increases by min_seconds*growth/100
	// for each later used add-on.
	// Returns the number of add-ons that were actually pruned.
	int32				PruneAddOnTime(time_t min_seconds, int32 growth=100);

protected:
	// Override this to implement your own BAddOnHandle subclass.
	virtual	BAddOnHandle* InstantiateHandle(const entry_ref* entry, const node_ref* node);

private:
	/* FBC */
	virtual	void		_AmyTheAddOnManager1();
	virtual	void		_AmyTheAddOnManager2();
	virtual	void		_AmyTheAddOnManager3();
	virtual	void		_AmyTheAddOnManager4();
	virtual	void		_AmyTheAddOnManager5();
	virtual	void		_AmyTheAddOnManager6();
	virtual	void		_AmyTheAddOnManager7();
	virtual	void		_AmyTheAddOnManager8();
	virtual	void		_AmyTheAddOnManager9();
	virtual	void		_AmyTheAddOnManager10();
	virtual	void		_AmyTheAddOnManager11();
	virtual	void		_AmyTheAddOnManager12();
	virtual	void		_AmyTheAddOnManager13();
	virtual	void		_AmyTheAddOnManager14();
	virtual	void		_AmyTheAddOnManager15();
	virtual	void		_AmyTheAddOnManager16();
	uint32				_amyTheData[8];

	friend class		AddOnSearchPath;

	void				mark_changed(bool needWakeup=true);
	void				send_notices();

	mutable	BLocker			fLock;
	// Combined list of directories to search.
	AddOnSearchPath 		*fSearchPath;
	BVector<BAddOnHandle*>	fAddOns;			// List of BAddOnHandle*
	BVector<void*>			fWatchers;			// List of BMessenger*
	bool					fScanned;
	bool					fRunning;
	int32					fChanged;
};

} } // namespace B::Storage2

#endif
