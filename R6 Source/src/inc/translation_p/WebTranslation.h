
#if !defined(_WEB_TRANSLATION_H)
#define _WEB_TRANSLATION_H

#include <DataIO.h>
#include <OS.h>
#include <Locker.h>
#include <TranslatorRoster.h>
#include <Message.h>
#include <Messenger.h>


class BRunningTranslation {
public:
		BRunningTranslation(
				const char * input_mime,
				type_code output_type,
				BPositionIO * input,
				BPositionIO * output,
				BTranslatorRoster * roster = 0,
				int32 id = 0);
		~BRunningTranslation();

		status_t ErrorCheck();
		status_t Go();
		status_t WaitForCompletion();
		status_t SetCompletionMessage(
				const BMessage & inMessage,
				const BMessenger & inTarget);

private:

		BPositionIO * fSource;
		BPositionIO * fDestination;
		BTranslatorRoster * fRoster;
		status_t fError;
		type_code fOutputType;
		translator_id fTranslator;
		thread_id fThread;
		BLocker fLocker;
		BMessage fCompletionMessage;
		BMessenger fCompletionMessenger;

static	status_t translate_thread(
				void * arg);

};

class BProgressiveInputStream : public BPositionIO {
public:
		BProgressiveInputStream(
				off_t knownSize = -1LL);
virtual	~BProgressiveInputStream();

		//	You can't have more than one ReadAt() or Seek() or KnownSize() outstanding
		//	at the same time on the same object. Sorry.
virtual	ssize_t		ReadAt(off_t pos, void *buffer, size_t size);
virtual	ssize_t		WriteAt(off_t pos, const void *buffer, size_t size);

virtual off_t		Seek(off_t position, uint32 seek_mode);
virtual	off_t		Position() const;

virtual status_t	SetSize(off_t size);

		off_t		KnownSize(bool syncWait = true);
		status_t	PutChunk(
							const void * data,
							size_t size,
							bool lastChunk);

private:

		off_t		fKnownSize;
		sem_id		fAmtSem;
		int32		fCurAmt;
		void *		fData;
		size_t		fSize;
		off_t		fPosition;
		BLocker		fLock;
};


#endif	//	_WEB_TRANSLATION_H

