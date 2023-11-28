#include "MediaRoster.h"

#include <media2/MediaCollective.h>

#include <support2/Autolock.h>
#include <support2/Debug.h>

using namespace B::Support2;

namespace B {
namespace Media2 {

const BValue IMediaRoster::descriptor(BValue::TypeInfo(typeid(IMediaRoster)));

class RMediaRoster : public RInterface<B::Media2::IMediaRoster>
{
	friend class LMediaRoster;
	
	static const BValue kKeyGetAudioMixer;
	static const BValue kKeyGetAudioMixerNode;
	static const BValue kKeyGetAudioMixerResult;

	static const BValue kKeySetAudioMixer;
	static const BValue kKeySetAudioMixerNode;
	static const BValue kKeySetAudioMixerResult;

	static const BValue kKeyGetTimeSource;
	static const BValue kKeyGetTimeSourceNode;
	static const BValue kKeyGetTimeSourceResult;

	static const BValue kKeySetTimeSource;
	static const BValue kKeySetTimeSourceNode;
	static const BValue kKeySetTimeSourceResult;
	
	static const BValue kKeyGetDecoderChain;
	static const BValue kKeyGetDecoderChainStartingAt;
	static const BValue kKeyGetDecoderChainCollective;
	static const BValue kKeyGetDecoderChainResult;
	
	public:
		RMediaRoster (IBinder::arg o)
			: RInterface<IMediaRoster>(o)
		{
		}
		
		virtual status_t GetAudioMixer (B::Media2::IMediaNode::ptr *node)
		{
			BValue result=Remote()->Invoke(BValue(kKeyGetAudioMixer,BValue::Bool(true)));
			
			*node=B::Media2::IMediaNode::AsInterface(result[kKeyGetAudioMixerNode].AsBinder());
			
			return result[kKeyGetAudioMixerResult].AsInt32();
		}
		
		virtual status_t SetAudioMixer (B::Media2::IMediaNode::arg node)
		{
			return Remote()->Invoke(BValue(kKeySetAudioMixer,BValue::Bool(true))
									.Overlay(BValue(kKeySetAudioMixerNode,BValue::Binder(node->AsBinder()))))
									[kKeySetAudioMixerResult].AsInt32();
		}

		virtual status_t GetTimeSource (B::Media2::ITimeSource::ptr *node)
		{
			BValue result=Remote()->Invoke(BValue(kKeyGetTimeSource,BValue::Bool(true)));
			
			*node=B::Media2::ITimeSource::AsInterface(result[kKeyGetTimeSourceNode].AsBinder());
			
			return result[kKeyGetTimeSourceResult].AsInt32();
		}
		
		virtual status_t SetTimeSource (B::Media2::ITimeSource::arg node)
		{
			return Remote()->Invoke(BValue(kKeySetTimeSource,BValue::Bool(true))
									.Overlay(BValue(kKeySetTimeSourceNode,BValue::Binder(node->AsBinder()))))
									[kKeySetTimeSourceResult].AsInt32();
		}

		virtual status_t GetDecoderChain (B::Media2::IMediaOutput::arg starting_at,
											B::Media2::IMediaNode::ptr *collective)
		{
			BValue result=Remote()->Invoke(BValue(kKeyGetDecoderChain,BValue::Bool(true))
											.Overlay(BValue(kKeyGetDecoderChainStartingAt,BValue::Binder(starting_at->AsBinder()))));

			*collective=IMediaNode::AsInterface(result[kKeyGetDecoderChainCollective].AsBinder());											

			return result[kKeyGetDecoderChainResult].AsInt32();
		}									
};

const BValue RMediaRoster::kKeyGetAudioMixer("GetAudioMixer");
const BValue RMediaRoster::kKeyGetAudioMixerNode("GetAudioMixerNode");
const BValue RMediaRoster::kKeyGetAudioMixerResult("GetAudioMixerResult");

const BValue RMediaRoster::kKeySetAudioMixer("SetAudioMixer");
const BValue RMediaRoster::kKeySetAudioMixerNode("SetAudioMixerNode");
const BValue RMediaRoster::kKeySetAudioMixerResult("SetAudioMixerResult");

const BValue RMediaRoster::kKeyGetTimeSource("GetTimeSource");
const BValue RMediaRoster::kKeyGetTimeSourceNode("GetTimeSourceNode");
const BValue RMediaRoster::kKeyGetTimeSourceResult("GetTimeSourceResult");

const BValue RMediaRoster::kKeySetTimeSource("SetTimeSource");
const BValue RMediaRoster::kKeySetTimeSourceNode("SetTimeSourceNode");
const BValue RMediaRoster::kKeySetTimeSourceResult("SetTimeSourceResult");

const BValue RMediaRoster::kKeyGetDecoderChain("GetDecoderChain");
const BValue RMediaRoster::kKeyGetDecoderChainStartingAt("GetDecoderChainStartingAt");
const BValue RMediaRoster::kKeyGetDecoderChainCollective("GetDecoderChainCollective");
const BValue RMediaRoster::kKeyGetDecoderChainResult("GetDecoderChainResult");

B_IMPLEMENT_META_INTERFACE(MediaRoster)

status_t 
LMediaRoster::Called(BValue &in, const BValue &outBindings, BValue &out)
{
	if (in[RMediaRoster::kKeyGetAudioMixer].IsDefined())
	{
		B::Media2::IMediaNode::ptr node;
		status_t result=GetAudioMixer(&node);
		
		out += outBindings * (BValue(RMediaRoster::kKeyGetAudioMixerNode,BValue::Binder(node->AsBinder()))
								.Overlay(RMediaRoster::kKeyGetAudioMixerResult,BValue::Int32(result)));
	}

	if (in[RMediaRoster::kKeySetAudioMixer].IsDefined())
	{
		status_t result=SetAudioMixer(B::Media2::IMediaNode::AsInterface(in[RMediaRoster::kKeySetAudioMixerNode].AsBinder()));

		out += outBindings * (BValue(RMediaRoster::kKeySetAudioMixerResult,BValue::Int32(result)));		
	}
	
	return B_OK;
}

BMediaRoster::ptr BMediaRoster::gRoster;
BLocker BMediaRoster::gRosterLocker("MediaRosterLocker");	

BMediaRoster::BMediaRoster()
{
}

BMediaRoster::ptr
BMediaRoster::Roster()
{
	BAutolock autolock(gRosterLocker.Lock());
	
	if (gRoster==NULL)
		gRoster=new BMediaRoster;
	
	return gRoster;	
}

status_t 
BMediaRoster::GetAudioMixer (IMediaNode::ptr *node)
{
	*node=mAudioMixer;
	
	return B_OK;
}

status_t 
BMediaRoster::SetAudioMixer (IMediaNode::arg node)
{
	mAudioMixer=node;
	
	return B_OK;
}

status_t 
BMediaRoster::GetTimeSource (ITimeSource::ptr *node)
{
	*node=mTimeSource;
	
	return B_OK;
}

status_t 
BMediaRoster::SetTimeSource (ITimeSource::arg node)
{
	mTimeSource=node;
	
	return B_OK;
}

status_t 
BMediaRoster::GetDecoderChain (IMediaOutput::arg starting_at,
								IMediaNode::ptr *collective)
{
	BMediaCollective::ptr chain=new BMediaCollective;

	IMediaEndpoint::ptr ep=IMediaEndpoint::AsInterface(starting_at->AsBinder());
	
	while (SupportsMediaType(ep->Constraint(),B_MEDIA_ENCODED_VIDEO)
			|| SupportsMediaType(ep->Constraint(),B_MEDIA_ENCODED_AUDIO))
	{
		BMediaAddon *addon;
		const flavor_info *flavor;
		const BMediaConstraint c=ep->Constraint();
		status_t result=FindAddonFlavorByConstraint(&c,NULL,&addon,&flavor);

		if (result<B_OK)
			return result;
			
		BMediaNode::ptr decoder=addon->InstantiateNodeFor(flavor,&result);

		if (result<B_OK)
			return result;
		
		if (decoder==NULL)
			return B_ERROR;

		BMediaEndpointVector decoder_inputs;
		ssize_t count=decoder->ListEndpoints(&decoder_inputs,B_INPUT_ENDPOINT,B_FREE_ENDPOINT);
		
		if (count<B_OK)
			return count;
		else if (count<1)
			return B_ERROR;
			
		IMediaOutput::ptr output=IMediaOutput::AsInterface(ep->AsBinder());
		IMediaInput::ptr decoder_input=IMediaInput::AsInterface(decoder_inputs[0]->AsBinder());
		
		BMediaFormat format;
		result=output->Connect(&decoder_input,&format);
		
		if (result<B_OK)
			return result;
		
		chain->AddNode(decoder);
		
		BMediaEndpointVector decoder_outputs;
		count=decoder->ListEndpoints(&decoder_outputs,B_OUTPUT_ENDPOINT,B_FREE_ENDPOINT);
		
		if (count<B_OK)
			return count;
		else if (count<1)
			return B_ERROR;

		ep=decoder_outputs[0];		
	}
	
	chain->ShowEndpoint(ep);
		
	*collective=chain;
	
	return B_OK;	
}

status_t
BMediaRoster::FindAddonFlavorByName (const char *name,
										BMediaAddon **addon,
										const flavor_info **flavor)
{
	BMediaAddon *add_on;
	int32 index=0;
	
	while ((add_on=mAddonManager.AddonAt(index))!=NULL)
	{
		for (int32 i=add_on->CountFlavors()-1;i>=0;--i)
		{
			if (add_on->GetFlavorAt(i,flavor)>=B_OK
				&& !strcmp(name,(*flavor)->name))
			{
				*addon=add_on;
				return B_OK;
			}
		}
	
		++index;
	}
	
	return B_ENTRY_NOT_FOUND;
}										

status_t
BMediaRoster::FindAddonFlavorByConstraint (const BMediaConstraint *input,
											const BMediaConstraint *output,
											BMediaAddon **addon,
											const flavor_info **flavor)
{
	BMediaAddon *add_on;
	int32 index=0;
	
	while ((add_on=mAddonManager.AddonAt(index))!=NULL)
	{
		for (int32 i=add_on->CountFlavors()-1;i>=0;--i)
		{
			if (add_on->GetFlavorAt(i,flavor)>=B_OK)
			{
				bool input_matches;
				
				if (!input)
					input_matches=true;
				else if (!(*flavor)->in_formats)
					input_matches=false;
				else
				{
					BMediaConstraint c=*((*flavor)->in_formats);
					input_matches=(c.And(*input)).Simplify()>=B_OK;
				}

				bool output_matches;
												
				if (!output)
					output_matches=true;
				else if (!(*flavor)->out_formats)
					output_matches=false;
				else
				{
					BMediaConstraint c=*((*flavor)->out_formats);
					output_matches=(c.And(*output)).Simplify()>=B_OK;
				}

				if (input_matches && output_matches)
				{
					*addon=add_on;
					return B_OK;
				}
			}
		}
	
		++index;
	}
	
	return B_ENTRY_NOT_FOUND;
}											

bool 
BMediaRoster::SupportsMediaType (const BMediaConstraint &c, media_type type)
{
	BMediaConstraint d(B_FORMATKEY_MEDIA_TYPE,BValue::Int32(type));
	
	return (d.And(c)).Simplify()>=B_OK;
}

} } // B::Media2
