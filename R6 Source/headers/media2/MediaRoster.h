/***************************************************************************
//
//	File:			media2/MediaRoster.h
//
//	Description:	Allows access to BMediaNodes (and associated endpoints)
//					published locally or within any team, and provides
//					operations for publishing your own nodes.
//
//	Copyright 2001, Be Incorporated, All Rights Reserved.
//
***************************************************************************/

#ifndef C_MEDIA_ROSTER_H

#define C_MEDIA_ROSTER_H

#include <media2/IMediaRoster.h>
#include <media2/ITimeSource.h>
#include <media2/MediaNode.h>
#include <media2/MediaAddon.h>

#include <support2/Locker.h>

namespace B {
namespace Media2 {

class LMediaRoster : public B::Support2::LInterface<B::Media2::IMediaRoster>
{
	protected:
		virtual ~LMediaRoster() {};
		
	public:
		virtual	status_t Called (BValue &in,
									const BValue &outBindings,
									BValue &out);
};

class BMediaRoster : public LMediaRoster
{
	BMediaAddonManager mAddonManager;

	B::Media2::IMediaNode::ptr mAudioMixer;
	B::Media2::ITimeSource::ptr mTimeSource;

	static bool SupportsMediaType (const BMediaConstraint &c, media_type type);
	
	BMediaRoster();

	public:
		B_STANDARD_ATOM_TYPEDEFS(BMediaRoster)
		
		static BMediaRoster::ptr Roster();
		
		virtual status_t GetAudioMixer (B::Media2::IMediaNode::ptr *node);
		virtual status_t SetAudioMixer (B::Media2::IMediaNode::arg node);
		
		virtual status_t GetTimeSource (B::Media2::ITimeSource::ptr *node);
		virtual status_t SetTimeSource (B::Media2::ITimeSource::arg node);
		
		virtual status_t GetDecoderChain (B::Media2::IMediaOutput::arg starting_at,
											B::Media2::IMediaNode::ptr *collective);

		// TODO: make these available through the IMediaRoster interface
		
		virtual status_t FindAddonFlavorByName (const char *name,
												BMediaAddon **addon,
												const flavor_info **flavor);
	
		virtual status_t FindAddonFlavorByConstraint (const BMediaConstraint *input,
														const BMediaConstraint *output,
														BMediaAddon **addon,
														const flavor_info **flavor);
	
	private:
		static BMediaRoster::ptr gRoster;
		static BLocker gRosterLocker;	
};

} } // B::Media2

#endif
