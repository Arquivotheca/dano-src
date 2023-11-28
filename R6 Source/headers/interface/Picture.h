/*******************************************************************************
/
/	File:			Picture.h
/
/   Description:    BPicture records a series of drawing instructions that can
/                   be "replayed" later.
/
/	Copyright 1995-98, Be Incorporated, All Rights Reserved
/
*******************************************************************************/

#ifndef	_PICTURE_H
#define	_PICTURE_H

#include <BeBuild.h>
#include <InterfaceDefs.h>
#include <Rect.h>
#include <Archivable.h>

class BView;
struct _BPictureExtent_;
namespace BPrivate {
class IKAccess;
}
namespace B {
namespace Interface2 {
class BDrawable;
}
}

/*----------------------------------------------------------------*/
/*----- BPicture class -------------------------------------------*/

class BPicture : public BArchivable {
public:
							BPicture();
							BPicture(const BPicture &original);
							BPicture(BMessage *data);
virtual						~BPicture();
static	BArchivable			*Instantiate(BMessage *data);
virtual	status_t			Archive(BMessage *data, bool deep = true) const;
virtual	status_t			Perform(perform_code d, void *arg);

		B::Interface2::BDrawable*	BeginDrawing();
		void						EndDrawing(B::Interface2::BDrawable* handle);
		
		status_t			Play(void **callBackTable,
								int32 tableEntries,
								void *userData);

		status_t			Flatten(BDataIO *stream);
		status_t			Unflatten(BDataIO *stream);

/*----- Private or reserved -----------------------------------------*/
private:

friend class BPrivate::IKAccess;
friend class BPrintJob;

virtual	void				_ReservedPicture1();
virtual	void				_ReservedPicture2();
virtual	void				_ReservedPicture3();

		BPicture			&operator=(const BPicture &);

		void				init_data();
		void				import_data(const void *data, int32 size, BPicture **subs, int32 subCount);
		void				import_old_data(const void *data, int32 size);
		void				set_token(int32 token);
		bool				assert_local_copy();
		bool				assert_old_local_copy();
		bool				assert_server_copy();

		/**Deprecated API**/
							BPicture(const void *data, int32 size);
		const void			*Data() const;
		int32				DataSize() const;

		void				usurp(BPicture *lameDuck);
		BPicture			*step_down();

		int32				fToken;
		_BPictureExtent_	*fExtent;
		BPicture			*fUsurped;
		B::Interface2::BDrawable
							*fDrawing;
		uint32				_reserved[2];
};

/*-------------------------------------------------------------*/
/*-------------------------------------------------------------*/

#endif /* _PICTURE_H */
