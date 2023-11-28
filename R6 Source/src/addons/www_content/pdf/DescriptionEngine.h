#if !defined(__DESCRIPTIONENGINE_H_)
#define __DESCRIPTIONENGINE_H_

#include <StringBuffer.h>
#include "ObjectSink.h"
#include "Object2.h"

namespace BPrivate {

class DescriptionEngine : public ObjectSink {

private:
object_array			fStack;
typedef vector<int32>	index_array;
index_array				fIndicies;
int						fEIstate;
bool					fWantRawData;
bool					fConsumeIDwhitespace;

typedef void (DescriptionEngine::* engine_func)(void);
static engine_func		fFnArray[];

protected:

#ifndef NDEBUG
int32					fCurrentKey;
#endif

StringBuffer			fString;
bigtime_t				fPulseInterval;
bigtime_t				fNextPulse;

inline bool				StackEmpty() { return fStack.empty(); };
inline PDFObject *		PopStack() { PDFObject * obj = fStack.back(); fStack.pop_back(); return obj;};
inline void				PushStack(PDFObject *obj) { fStack.push_back(obj); };
void					eat_stack(size_t count = 0);

public:
						DescriptionEngine();
virtual					~DescriptionEngine();

// From ObjectSink
virtual	ssize_t			Write(const uint8 *buffer, ssize_t length, bool finish = false);
virtual ssize_t			Write(BPrivate::PDFObject *obj);

// new protocol
protected:
virtual void			Pulse(void);
virtual void			UnknownKeyword(PDFObject *keyword);
virtual void			bFn();
virtual void			bstarFn();
virtual void			BFn();
virtual void			BstarFn();
virtual void			BDCFn();
virtual void			BIFn();
virtual void			BMCFn();
virtual void			BTFn();
virtual void			BXFn();
virtual void			cFn();
virtual void			cmFn();
virtual void			csFn();
virtual void			CSFn();
virtual void			dFn();
virtual void			d0Fn();
virtual void			d1Fn();
virtual void			DoFn();
virtual void			DPFn();
virtual void			EIFn();
virtual void			EMCFn();
virtual void			ETFn();
virtual void			EXFn();
virtual void			fFn();
virtual void			fstarFn();
virtual void			FFn();
virtual void			gFn();
virtual void			gsFn();
virtual void			GFn();
virtual void			hFn();
virtual void			iFn();
virtual void			IDFn();
virtual void			jFn();
virtual void			JFn();
virtual void			kFn();
virtual void			KFn();
virtual void			lFn();
virtual void			mFn();
virtual void			MFn();
virtual void			MPFn();
virtual void			nFn();
virtual void			PSFn();
virtual void			qFn();
virtual void			QFn();
virtual void			reFn();
virtual void			rgFn();
virtual void			riFn();
virtual void			RGFn();
virtual void			sFn();
virtual void			scFn();
virtual void			scnFn();
virtual void			shFn();
virtual void			SFn();
virtual void			SCFn();
virtual void			SCNFn();
virtual void			TcFn();
virtual void			TdFn();
virtual void			TDFn();
virtual void			TfFn();
virtual void			TjFn();
virtual void			TJFn();
virtual void			TLFn();
virtual void			TmFn();
virtual void			TrFn();
virtual void			TsFn();
virtual void			TwFn();
virtual void			TzFn();
virtual void			TstarFn();
virtual void			vFn();
virtual void			wFn();
virtual void			WFn();
virtual void			WstarFn();
virtual void			yFn();
virtual void			tickFn();
virtual void			ticktickFn();
virtual void			startarrayFn();
virtual void			endarrayFn();
virtual void			startdictFn();
virtual void			enddictFn();
};

}; // namespace BPrivate

using namespace BPrivate;

#endif
